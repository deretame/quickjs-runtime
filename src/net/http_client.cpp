// qjsbind_net 网络层实现 —— 见 http_client.hpp 设计注释
#include "net/http_backend.hpp"
#include "net/http_client.hpp"

#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url/parse.hpp>
#include <exec/asio/use_sender.hpp>

#include <memory>
#include <optional>
#include <stdexcept>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "net/cacert_embedded.hpp" // 脚本生成：qjsbind::net::embedded_cacert_pem

namespace qjsbind::net {
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
// 校验失败抛 std::invalid_argument。
ParsedUrl parse_url(const std::string& url) {
    auto r = boost::urls::parse_uri_reference(url);
    if (r.has_error())
        throw std::invalid_argument("url: 无法解析");
    const auto& uv = *r;
    if (uv.scheme() != "http" && uv.scheme() != "https")
        throw std::invalid_argument("url: 仅支持 http/https scheme");
    if (uv.host().empty())
        throw std::invalid_argument("url: 缺少 host");
    ParsedUrl out;
    out.scheme = std::string(uv.scheme());
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

// 组装 beast 请求并完成 write + read。Stream 为 tcp::socket 或 ssl::stream<tcp::socket>。
template <class Stream>
exec::task<HttpResponse> do_exchange(Stream& stream, const HttpRequest& req,
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

    co_await http::async_write(stream, hreq, exec::asio::use_sender);

    beast::flat_buffer buffer;
    http::response<http::string_body> hres;
    co_await http::async_read(stream, buffer, hres, exec::asio::use_sender);

    HttpResponse out;
    out.status = hres.result_int();
    out.reason = std::string(hres.reason());
    out.body = std::move(hres.body());
    for (const auto& f : hres.base())
        out.headers.push_back({std::string(f.name_string()), std::string(f.value())});
    co_return out;
}

} // namespace

exec::task<HttpResponse> http_request(boost::asio::io_context& io, HttpRequest req,
                                      TlsOptions tls, std::stop_token st) {
    const ParsedUrl url = parse_url(req.url);

    // 共享 socket：stop_callback 可能在其他线程触发（AbortController 走 JS 线程，
    // Runtime::stop 走任意线程），cancel 期间 socket 必须存活。
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
        co_await resolver.async_resolve(url.host, url.port, exec::asio::use_sender);

    if (url.scheme == "https") {
        ssl::context ctx = make_ssl_context(tls);
        ssl::stream<tcp::socket> stream(io, ctx);
        stream.set_verify_callback(ssl::host_name_verification(url.host));
        co_await stream.next_layer().async_connect(*results.begin(), exec::asio::use_sender);
        co_await stream.async_handshake(ssl::stream_base::client, exec::asio::use_sender);
        co_return co_await do_exchange(stream, req, url);
    }
    co_await sock->async_connect(*results.begin(), exec::asio::use_sender);
    co_return co_await do_exchange(*sock, req, url);
}

// ---- BeastFetchBackend：FetchBackend 适配（net 层类型 → web 层类型）----
exec::task<web::HttpResponse> BeastFetchBackend::request(web::HttpRequest req,
                                                         std::stop_token st) {
    HttpRequest r;
    r.method = std::move(req.method);
    r.url = std::move(req.url);
    for (auto& h : req.headers)
        r.headers.push_back({std::move(h.name), std::move(h.value)});
    r.body = std::move(req.body);
    HttpResponse resp = co_await http_request(io_, std::move(r), TlsOptions{}, std::move(st));
    web::HttpResponse out;
    out.status = resp.status;
    out.reason = std::move(resp.reason);
    for (auto& h : resp.headers)
        out.headers.push_back({std::move(h.name), std::move(h.value)});
    out.body = std::move(resp.body);
    co_return out;
}

} // namespace qjsbind::net
