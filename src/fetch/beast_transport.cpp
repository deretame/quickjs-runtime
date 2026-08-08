// fetchcore —— BeastTransport 实现（原 qjsbind_net http_client.cpp 演进）
//
// 设计注释见 include/fetch/beast_transport.hpp。本文件是 fetchcore 的实现侧：
// 只依赖 fetch 公共类型 + asio/beast/OpenSSL，无任何 quickjs/qjsbind 依赖。
#include "socks5.hpp"
#include "cacert_embedded.hpp" // 脚本生成：fetch::embedded_cacert_pem

#include <fetch/beast_transport.hpp>
#include <fetch/body.hpp> // BodySource 完整定义（BeastBodySource 继承 + shared_ptr 转换）

#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url/parse.hpp>
#include <exec/asio/use_sender.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>

#include <algorithm>
#include <cctype>
#include <cstring>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace fetch {
namespace {

using boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;
namespace asio = boost::asio;

struct ParsedUrl {
    std::string scheme; // "http" / "https"
    std::string host;   // 不含端口；IPv6 文字地址去掉方括号
    std::string port;   // 端口字符串
    std::string target; // /path?query（空则 "/"）
};

// URL 解析：boost::urls（RFC 3986/WHATWG 兼容）；仅 http/https。
// scheme 大小写不敏感比较（HTTP:// 等大写形式与 WHATWG 语义一致）。
// 校验失败抛 std::invalid_argument。
ParsedUrl parse_url(const std::string& url) {
    auto r = boost::urls::parse_uri_reference(url);
    if (r.has_error())
        throw std::invalid_argument("url: 无法解析");
    const auto& uv = *r;
    auto scheme_eq = [&](const char* s) {
        const std::string_view sc = uv.scheme();
        return sc.size() == std::strlen(s) &&
               std::equal(sc.begin(), sc.end(), s,
                          [](char a, char b) { return (a | 32) == b; });
    };
    if (!scheme_eq("http") && !scheme_eq("https"))
        throw std::invalid_argument("url: 仅支持 http/https scheme");
    if (uv.host().empty())
        throw std::invalid_argument("url: 缺少 host");
    ParsedUrl out;
    out.scheme = std::string(uv.scheme());
    for (auto& c : out.scheme)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    out.host = std::string(uv.host()); // IPv6 文字地址不带方括号（host_name_verification 需要）
    out.port = uv.has_port() ? std::string(uv.port())
                             : (out.scheme == "https" ? "443" : "80");
    if (uv.has_port() && uv.port().empty())
        throw std::invalid_argument("url: 端口非法");
    out.target = std::string(uv.encoded_target()); // path?query（无则 ""）
    if (out.target.empty())
        out.target = "/";
    return out;
}

// 把 PEM 中的全部证书加载进 context 的 X509_STORE（内存加载，不落盘）。
void load_pem_into_store(ssl::context& ctx, std::string_view pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio)
        throw std::runtime_error("TLS: BIO_new_mem_buf 失败");
    X509_STORE* store = SSL_CTX_get_cert_store(ctx.native_handle());
    X509* cert = nullptr;
    while ((cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) != nullptr) {
        if (X509_STORE_add_cert(store, cert) != 1) {
            X509_free(cert);
            BIO_free(bio);
            throw std::runtime_error("TLS: X509_STORE_add_cert 失败");
        }
        X509_free(cert);
    }
    const unsigned long err = ERR_peek_last_error();
    BIO_free(bio);
    // 正常结束：读到文件尾（无错误，或最后一个错误是 NO_START_LINE）。
    // 其余 PEM 错误（如 base64 损坏）视为解析失败。
    if (err != 0 && (ERR_GET_LIB(err) != ERR_LIB_PEM || ERR_GET_REASON(err) != PEM_R_NO_START_LINE))
        throw std::runtime_error("TLS: PEM 解析失败");
    ERR_clear_error(); // 成功路径清理线程局部错误队列（NO_START_LINE 属正常 EOF 标记）
}

// 共享的嵌入 CA store（进程级缓存；up_ref 一次自持，各 context 再 up_ref 后 set）
X509_STORE* shared_ca_store() {
    static X509_STORE* store = [] {
        ssl::context tmp(ssl::context::tls_client);
        load_pem_into_store(tmp, embedded_cacert_pem);
        X509_STORE* s = SSL_CTX_get_cert_store(tmp.native_handle());
        X509_STORE_up_ref(s); // 自持一份（static 缓存，进程级存活）
        return s;
    }();
    return store;
}

// 构建 TLS context（boost 1.91 的 ssl::context 是 move-only，无法拷贝共享）
ssl::context make_ssl_context(const TlsOptions& tls) {
    ssl::context c(ssl::context::tls_client);
    c.set_verify_mode(tls.verify ? ssl::verify_peer : ssl::verify_none);
    if (tls.verify) {
        X509_STORE* s = shared_ca_store();
        X509_STORE_up_ref(s); // ctx 接管一份
        SSL_CTX_set_cert_store(c.native_handle(), s);
    }
    for (const auto& pem : tls.extra_trust_pem)
        load_pem_into_store(c, pem);
    return c;
}

// 组装 beast 请求（不含 IO）
http::request<http::string_body> make_beast_request(const Request& req,
                                                    const ParsedUrl& url) {
    http::request<http::string_body> hreq;
    hreq.method_string(req.method);
    hreq.target(url.target);
    hreq.version(11);
    for (const auto& h : req.headers)
        hreq.set(h.name, h.value);
    hreq.set(http::field::host, url.host + (url.port == (url.scheme == "https" ? "443" : "80")
                                               ? ""
                                               : ":" + url.port));
    hreq.set(http::field::user_agent, "qjs-runtime/0.1 (+wpt)");
    // 默认 Accept/Accept-Language 只在用户未设置时生效（wpt accept-header 测试）
    if (hreq.find(http::field::accept) == hreq.end())
        hreq.set(http::field::accept, "*/*");
    if (hreq.find(http::field::accept_language) == hreq.end())
        hreq.set(http::field::accept_language, "en-US,en;q=0.9");
    // 不设 Connection 头：wpt inspect-headers 测试要求 fetch 请求不含连接管理头；
    // 响应读完 socket 直接析构关闭。
    if (!req.body.empty()) {
        hreq.body() = req.body;
        hreq.prepare_payload();
    }
    return hreq;
}

// 响应头（不含 body）
struct ResponseHead {
    int status = 0;
    std::string reason;
    Headers headers;
};

// 完成 write + read_header（v1 do_exchange 的头部段）。返回 head + 移交 body 解析器。
// parser 以 shared_ptr 传入：beast 的 response_parser 不可移动，且 body 阶段需要
// 同一 parser 继续 async_read_some（由 BeastBodySource 持有）。
template <class Stream>
std_exec::task<std::pair<ResponseHead, beast::flat_buffer>>
do_exchange_head(Stream& stream, const Request& req, const ParsedUrl& url,
                 std::shared_ptr<http::response_parser<http::buffer_body>> parser) {
    http::request<http::string_body> hreq = make_beast_request(req, url);
    co_await http::async_write(stream, hreq, exec::asio::use_sender);

    beast::flat_buffer buffer;
    co_await http::async_read_header(stream, buffer, *parser, exec::asio::use_sender);

    const auto& hres = parser->get();
    ResponseHead head;
    head.status = hres.result_int();
    head.reason = std::string(hres.reason());
    for (const auto& f : hres.base())
        head.headers.push_back({std::string(f.name_string()), std::string(f.value())});
    co_return std::pair<ResponseHead, beast::flat_buffer>{std::move(head),
                                                          std::move(buffer)};
}

// ---- BeastBodySource：流式 body 源（fetch::BodySource 的 beast 实现）----
// 持有 stream（shared_ptr：头阶段与 body 阶段的 stop_callback 共享同一对象）、
// flat_buffer、response_parser<buffer_body> 与 64 KiB 读缓冲。
// read() = http::async_read_some → 返回本次消费的字节；parser.is_done() → nullopt
// （chunked / content-length / need_eof 连接关闭终止都由 beast 处理）。
// cancel() = lowest_layer().close()（同时唤醒挂起的 read，以 operation_aborted 完成）。
// stop_callback 构造时注册（arm_stop）、析构时注销；回调跨线程，仅触碰 socket。
template <class Stream>
class BeastBodySource : public BodySource,
                        public std::enable_shared_from_this<BeastBodySource<Stream>> {
public:
    BeastBodySource(std::shared_ptr<Stream> stream, beast::flat_buffer buffer,
                    std::shared_ptr<http::response_parser<http::buffer_body>> parser,
                    std::shared_ptr<ssl::context> ctx = nullptr)
        : stream_(std::move(stream)), buffer_(std::move(buffer)), parser_(std::move(parser)),
          ctx_(std::move(ctx))
    {
    }

    ~BeastBodySource() override
    {
        // 注销 stop_callback（成员析构在前），关闭 socket 释放连接
        boost::system::error_code ec;
        stream_->lowest_layer().close(ec);
    }

    // 注册取消回调（weak 自持：回调执行期间 source 不会被析构）。
    // 注册时若已 stop_requested → 立即回调（cancel）。
    void arm_stop(std::stop_token st, std::weak_ptr<BeastBodySource> weak)
    {
        if (!st.stop_possible())
            return;
        stop_cb_.emplace(st, [weak] {
            if (auto self = weak.lock())
                self->cancel();
        });
    }

    std_exec::task<std::optional<std::string>> read() override
    {
        for (;;) {
            if (parser_->is_done()) {
                co_return std::nullopt;
            }
            chunk_.resize(kChunkSize);
            parser_->get().body().data = chunk_.data();
            parser_->get().body().size = chunk_.size();
            // 按值传 use_sender（async_compose 要求 CompletionToken 为可移动的值类型）
            auto use_sender = exec::asio::use_sender;
            bool aborted = false;
            try {
                co_await http::async_read_some(*stream_, buffer_, *parser_, use_sender);
            } catch (const boost::system::system_error&) {
                // 中止（stop）已请求后的一切读取失败都算 abort：挂起中的读被
                // cancel() 唤醒后 asio 报 operation_aborted（use_sender 转 stopped，
                // 协程在此终止，不走这里）；但"先 close 后新发起读"会报
                // bad_descriptor 等错误 → 统一转 stopped → reject AbortError。
                // （catch 块内不能 co_await，用标志延后）
                if (cancelled_.load(std::memory_order_acquire))
                    aborted = true;
                else
                    throw;
            }
            if (aborted)
                co_await stdexec::just_stopped();
            const size_t used = chunk_.size() - parser_->get().body().size;
            chunk_.resize(used);
            if (used > 0)
                co_return std::move(chunk_);
            // used == 0 且未 done：只消费了控制字节（chunk 边界），继续读
        }
    }

    void cancel() override
    {
        cancelled_.store(true, std::memory_order_release);
        boost::system::error_code ec;
        stream_->lowest_layer().close(ec);
    }

private:
    static constexpr size_t kChunkSize = 64 * 1024;
    std::shared_ptr<Stream> stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<http::response_parser<http::buffer_body>> parser_;
    std::shared_ptr<ssl::context> ctx_; // https：持有 ssl::context（须比 stream 活得久）
    std::string chunk_;
    std::atomic<bool> cancelled_{false};
    std::optional<std::stop_callback<std::function<void()>>> stop_cb_;
};

} // namespace

// 建立到目标的 TCP 连接：直连（resolver + connect）或经 SOCKS5 隧道。
// 返回共享 socket：调用方在其上挂取消回调（connect / socks5 握手 / TLS 握手 /
// 读头全程可取消；body 阶段由 BeastBodySource 接管）。
std_exec::task<std::shared_ptr<tcp::socket>>
connect_tcp(boost::asio::io_context& io, const std::string& host, const std::string& port,
            std::stop_token st, const std::optional<Socks5Proxy>& proxy)
{
    if (proxy) {
        // 与直连路径一致：端口非法/越界 → 抛（不静默默认 80）
        int p = 0;
        try {
            p = std::stoi(port);
        } catch (const std::exception&) {
            throw std::invalid_argument("socks5: 非法端口 '" + port + "'");
        }
        if (p < 1 || p > 65535)
            throw std::invalid_argument("socks5: 端口越界 '" + port + "'");
        co_return co_await socks5_connect(io, *proxy, host, static_cast<uint16_t>(p), st);
    }
    auto sock = std::make_shared<tcp::socket>(io);
    std::optional<std::stop_callback<std::function<void()>>> stop_cb;
    if (st.stop_possible()) {
        stop_cb.emplace(st, [sock] {
            boost::system::error_code ec;
            sock->cancel(ec); // operation_aborted → use_sender 转 set_stopped → AbortError
        });
    }
    tcp::resolver resolver(io);
    const auto results =
        co_await resolver.async_resolve(host, port, exec::asio::use_sender);
    co_await sock->async_connect(*results.begin(), exec::asio::use_sender);
    co_return sock;
}

std_exec::task<Response> BeastTransport::request(const Request& req, std::stop_token st) {
    const ParsedUrl url = parse_url(req.url);

    auto sock = co_await connect_tcp(io_, url.host, url.port, st, std::nullopt);
    // 取消回调挂共享 socket（同 connect_tcp 内的回调，双保险覆盖 connect 与
    // 后续阶段之间的窗口；cancel 幂等）。
    std::optional<std::stop_callback<std::function<void()>>> head_stop_cb;
    if (st.stop_possible()) {
        head_stop_cb.emplace(st, [sock] {
            boost::system::error_code ec;
            sock->cancel(ec);
        });
    }

    std::shared_ptr<BodySource> body;
    ResponseHead head;
    if (url.scheme == "https") {
        // ctx 随 body 源存活（boost 契约：context 须比 stream 活得久，BeastBodySource 持有）
        auto ctx = std::make_shared<ssl::context>(make_ssl_context(tls_));
        // 隧道/直连 socket 直接移交 ssl::stream（SOCKS5 隧道上照常 TLS handshake）
        auto stream = std::make_shared<ssl::stream<tcp::socket>>(std::move(*sock), *ctx);
        stream->set_verify_callback(ssl::host_name_verification(url.host));
        // sock 已 move 进 stream：取消回调改绑 stream 底层 socket（否则 abort 失效）
        if (head_stop_cb)
            head_stop_cb.emplace(st, [stream] {
                boost::system::error_code ec;
                stream->next_layer().cancel(ec);
            });
        co_await stream->async_handshake(ssl::stream_base::client, exec::asio::use_sender);
        auto parser = std::make_shared<http::response_parser<http::buffer_body>>();
        auto [h, buffer] = co_await do_exchange_head(*stream, req, url, parser);
        head = std::move(h);
        // 无 body 场景（HEAD / 204 / 205 / 304）：body = nullptr
        if (req.method != "HEAD" && head.status != 204 && head.status != 205 &&
            head.status != 304) {
            auto src = std::make_shared<BeastBodySource<ssl::stream<tcp::socket>>>(
                std::move(stream), std::move(buffer), std::move(parser), std::move(ctx));
            src->arm_stop(st, src);
            body = std::move(src);
        }
    } else {
        auto parser = std::make_shared<http::response_parser<http::buffer_body>>();
        auto [h, buffer] = co_await do_exchange_head(*sock, req, url, parser);
        head = std::move(h);
        if (req.method != "HEAD" && head.status != 204 && head.status != 205 &&
            head.status != 304) {
            auto src = std::make_shared<BeastBodySource<tcp::socket>>(
                std::move(sock), std::move(buffer), std::move(parser));
            src->arm_stop(st, src);
            body = std::move(src);
        }
    }

    Response out;
    out.status = head.status;
    out.reason = std::move(head.reason);
    out.headers = std::move(head.headers);
    out.body = std::move(body);
    co_return out;
}

std_exec::task<Response> BeastTransport::request_via_socks5(const Request& req,
                                                            const Socks5Proxy& proxy,
                                                            std::stop_token st) {
    const ParsedUrl url = parse_url(req.url);

    auto sock = co_await connect_tcp(io_, url.host, url.port, st, proxy);
    std::optional<std::stop_callback<std::function<void()>>> head_stop_cb;
    if (st.stop_possible()) {
        head_stop_cb.emplace(st, [sock] {
            boost::system::error_code ec;
            sock->cancel(ec);
        });
    }

    std::shared_ptr<BodySource> body;
    ResponseHead head;
    if (url.scheme == "https") {
        auto ctx = std::make_shared<ssl::context>(make_ssl_context(tls_));
        auto stream = std::make_shared<ssl::stream<tcp::socket>>(std::move(*sock), *ctx);
        stream->set_verify_callback(ssl::host_name_verification(url.host));
        if (head_stop_cb)
            head_stop_cb.emplace(st, [stream] {
                boost::system::error_code ec;
                stream->next_layer().cancel(ec);
            });
        co_await stream->async_handshake(ssl::stream_base::client, exec::asio::use_sender);
        auto parser = std::make_shared<http::response_parser<http::buffer_body>>();
        auto [h, buffer] = co_await do_exchange_head(*stream, req, url, parser);
        head = std::move(h);
        if (req.method != "HEAD" && head.status != 204 && head.status != 205 &&
            head.status != 304) {
            auto src = std::make_shared<BeastBodySource<ssl::stream<tcp::socket>>>(
                std::move(stream), std::move(buffer), std::move(parser), std::move(ctx));
            src->arm_stop(st, src);
            body = std::move(src);
        }
    } else {
        auto parser = std::make_shared<http::response_parser<http::buffer_body>>();
        auto [h, buffer] = co_await do_exchange_head(*sock, req, url, parser);
        head = std::move(h);
        if (req.method != "HEAD" && head.status != 204 && head.status != 205 &&
            head.status != 304) {
            auto src = std::make_shared<BeastBodySource<tcp::socket>>(
                std::move(sock), std::move(buffer), std::move(parser));
            src->arm_stop(st, src);
            body = std::move(src);
        }
    }

    Response out;
    out.status = head.status;
    out.reason = std::move(head.reason);
    out.headers = std::move(head.headers);
    out.body = std::move(body);
    co_return out;
}

} // namespace fetch
