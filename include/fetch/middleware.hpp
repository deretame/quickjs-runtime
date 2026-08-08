// fetchcore —— C++ 中间件框架 + 内建中间件（原 qjsbind::web/interceptor.hpp 迁入）
//
// 中间件是 around 模型的协程链：每个中间件收到内层 next，co_await next 之前是
// 前置相位、之后是后置相位（设计文档 fetch_design.md §5）。
//
// 注册顺序 = 嵌套顺序（洋葱）：先注册者在最外层——前置相位按注册顺序进入，
// 后置相位逆序返回；read 路径上外层 source 先执行（解压在最外层）。
//
// 边界：中间件注册入口只有 fetch::Client::use()（C++ API），不向 JS 暴露
// 任何注册/枚举/移除能力（docs/fetch_cpp_decoupling.md §5）。
//
// 内建清单：
//   AcceptEncodingMiddleware：自动加 Accept-Encoding: gzip, deflate, br；
//     响应命中 content-encoding → 剥头 + 包 DecompressSource。
//   DecompressSource：gzip（inflateInit2(15+16)）/ deflate（zlib 流，首个块
//     Z_DATA_ERROR 时回退裸 deflate）/ br（BrotliDecoderDecompressStream）。
//   IntegritySource：SRI 末端校验（摘要对解码后字节；read 时增量算，EOF 比对）。
//   Socks5ProxyMiddleware：SOCKS5 选路（Route 回调；命中 → 直接走传输隧道，
//     不调 next；未命中 → 直连）。面向 Transport 抽象，不依赖具体实现。
#pragma once

#include <fetch/task.hpp>
#include <fetch/types.hpp>
#include <fetch/transport.hpp>

#include <brotli/decode.h>
#include <openssl/evp.h>
#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fetch {

// 链尾处理器：最终落到 Transport::request（Client 组装，每跳重走全链）
using Handler =
    std::function<std_exec::task<Response>(const Request& req, std::stop_token st)>;

struct Middleware {
    virtual ~Middleware() = default;
    // co_await next 之前 = 前置相位；之后 = 后置相位。
    // req 为 const 引用、贯穿整个调用：后置相位仍可读。
    // 短路 = 不调 next 直接 co_return；重试 = 多次调 next；换传输 = 调别的 handler。
    virtual std_exec::task<Response> intercept(const Request& req, std::stop_token st,
                                               Handler next) = 0;
};

// 链组装：链尾 = transport，逐层包中间件；handler 可重入（next 拷贝传递）
inline Handler make_chain(const std::vector<std::shared_ptr<Middleware>>& middlewares,
                          const std::shared_ptr<Transport>& transport)
{
    Handler h = [transport](const Request& req, std::stop_token st) {
        return transport->request(req, std::move(st));
    };
    for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
        std::shared_ptr<Middleware> mw = *it;
        Handler next = std::move(h);
        h = [mw, next](const Request& req, std::stop_token st) {
            return mw->intercept(req, std::move(st), next); // next 拷贝：可重入
        };
    }
    return h;
}

// 单层包裹（Client 组装内建 Accept-Encoding 用）
inline Handler wrap_middleware(const std::shared_ptr<Middleware>& mw, Handler next)
{
    return [mw, next = std::move(next)](const Request& req, std::stop_token st) {
        return mw->intercept(req, std::move(st), next);
    };
}

// ---- DecompressSource：解码 body 流（fetch_design.md §5.8）----
// gzip：inflateInit2(15+16)；deflate：zlib 流（15），首个块 Z_DATA_ERROR 时回退裸
// deflate（-15）；br：BrotliDecoderDecompressStream。解码错误/流截断 → 抛
// std::runtime_error → read() reject TypeError（沿网络错误同一路径）。
class DecompressSource : public BodySource {
public:
    enum class Kind { Gzip, Deflate, Brotli };

    // 单块输出上限：防高压缩比输入在单次 read() 内无限膨胀
    //（64 KiB 输入最坏可解出 ~66 MB；限制后内存峰值受控，流语义不变）
    static constexpr size_t kMaxChunk = 1024 * 1024;

    // total_limit：可选总解压字节上限（0 = 无限制；超限 → read 抛异常）
    DecompressSource(std::shared_ptr<BodySource> upstream, Kind kind,
                     size_t total_limit = 0)
        : upstream_(std::move(upstream)), kind_(kind), total_limit_(total_limit)
    {
        if (kind_ == Kind::Brotli) {
            br_state_ = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
            if (!br_state_)
                throw std::runtime_error("brotli: BrotliDecoderCreateInstance 失败");
        } else {
            std::memset(&z_, 0, sizeof(z_));
            const int window = kind_ == Kind::Gzip ? 15 + 16 : 15;
            int rc = inflateInit2(&z_, window);
            if (rc != Z_OK)
                throw std::runtime_error("zlib: inflateInit2 失败");
            // zlib 流模式的裸 deflate 回退标志
            fallback_ = kind_ == Kind::Deflate;
        }
    }

    ~DecompressSource() override
    {
        if (kind_ == Kind::Brotli) {
            if (br_state_)
                BrotliDecoderDestroyInstance(br_state_);
        } else {
            inflateEnd(&z_);
        }
    }

    void cancel() override
    {
        // 尽力释放上游（socket 关闭由上游析构链完成）
        if (upstream_)
            upstream_->cancel();
    }

    std_exec::task<std::optional<std::string>> read() override
    {
        // 已收尾（上游 EOF 且解码器完成）→ EOF
        if (finished_)
            co_return std::nullopt;
        std::string out;
        out.reserve(64 * 1024);
        for (;;) {
            if (!eof_upstream_ && in_.empty()) {
                auto block = co_await upstream_->read();
                if (block)
                    in_ = std::move(*block);
                else
                    eof_upstream_ = true;
            }
            if (kind_ == Kind::Brotli) {
                if (!pump_brotli(out))
                    throw std::runtime_error("brotli: 解码失败");
            } else {
                if (!pump_zlib(out))
                    throw std::runtime_error("zlib: 解码失败");
            }
            if (stream_end_)
                in_.clear(); // 流尾后的 trailing 字节丢弃（zlib/brotli 均允许）
            if (!out.empty()) {
                total_ += out.size();
                if (total_limit_ && total_ > total_limit_)
                    throw std::runtime_error("解压总量超过上限");
                co_return out;
            }
            if (eof_upstream_ && in_.empty()) {
                // 上游 EOF：解码器收尾检查
                finish(out);
                if (!out.empty())
                    co_return out;
                finished_ = true;
                co_return std::nullopt;
            }
            if (eof_upstream_ && !in_.empty() && !stream_end_) {
                // 有剩余输入但解码器不再消费（流截断）
                throw std::runtime_error("zlib: 流截断（未到 stream end）");
            }
        }
    }

private:
    // zlib 一次推进：从 in_ 消费，产出到 out；返回 false = 解码错误
    bool pump_zlib(std::string& out)
    {
        if (stream_end_)
            return true; // 已到流尾：不再 inflate（trailing 由 read() 丢弃）
        if (in_.empty())
            return true;
        const uInt chunk = static_cast<uInt>(std::min<size_t>(in_.size(), 64 * 1024));
        z_.next_in = reinterpret_cast<Bytef*>(in_.data());
        z_.avail_in = chunk;
        do {
            char buf[64 * 1024];
            z_.next_out = reinterpret_cast<Bytef*>(buf);
            z_.avail_out = sizeof(buf);
            int rc = inflate(&z_, Z_NO_FLUSH);
            if (rc == Z_STREAM_ERROR)
                return false;
            if (rc == Z_NEED_DICT || (rc == Z_DATA_ERROR && z_.msg && std::strstr(z_.msg, "incorrect header check"))) {
                // 裸 deflate 回退：zlib 流首个块头错误 → 用 -15 重试（fetch_design.md §5.8）
                if (fallback_ && !fallback_tried_) {
                    fallback_tried_ = true;
                    inflateEnd(&z_);
                    std::memset(&z_, 0, sizeof(z_));
                    if (inflateInit2(&z_, -15) != Z_OK)
                        return false;
                    z_.next_in = reinterpret_cast<Bytef*>(in_.data());
                    z_.avail_in = chunk;
                    continue; // 重试同一块
                }
                return false;
            }
            if (rc == Z_BUF_ERROR && (z_.avail_in == 0 || z_.avail_out == 0))
                break; // 输入/输出恰好耗尽：正常边界，等待更多输入或换输出缓冲
            if (rc == Z_DATA_ERROR || rc == Z_MEM_ERROR || rc == Z_BUF_ERROR)
                return false;
            const size_t produced = sizeof(buf) - z_.avail_out;
            if (produced > 0)
                out.append(buf, produced);
            if (rc == Z_STREAM_END) {
                stream_end_ = true;
                // 消费掉已处理的输入
                const size_t consumed = chunk - z_.avail_in;
                in_.erase(0, consumed);
                return true;
            }
            if (out.size() >= kMaxChunk)
                break; // 单块上限：中断解压，剩余输入留给下次 read()
            if (rc == Z_OK && z_.avail_in == 0 && z_.avail_out > 0)
                break; // 输入耗尽
        } while (z_.avail_out == 0);
        const size_t consumed = chunk - z_.avail_in;
        in_.erase(0, consumed);
        return true;
    }

    // brotli 一次推进
    bool pump_brotli(std::string& out)
    {
        if (stream_end_)
            return true;
        if (in_.empty())
            return true;
        const size_t chunk = std::min<size_t>(in_.size(), 64 * 1024);
        size_t avail_in = chunk;
        const uint8_t* next_in = reinterpret_cast<const uint8_t*>(in_.data());
        for (;;) {
            uint8_t buf[64 * 1024];
            size_t avail_out = sizeof(buf);
            uint8_t* next_out = buf;
            BrotliDecoderResult rc =
                BrotliDecoderDecompressStream(br_state_, &avail_in, &next_in, &avail_out,
                                              &next_out, nullptr);
            const size_t produced = sizeof(buf) - avail_out;
            if (produced > 0)
                out.append(reinterpret_cast<char*>(buf), produced);
            if (rc == BROTLI_DECODER_RESULT_ERROR)
                return false;
            if (rc == BROTLI_DECODER_RESULT_SUCCESS) {
                stream_end_ = true;
                break;
            }
            if (out.size() >= kMaxChunk)
                break; // 单块上限：中断解码，剩余输入留给下次 read()
            if (avail_in == 0 && avail_out > 0)
                break; // 输入耗尽
        }
        in_.erase(0, chunk - avail_in);
        return true;
    }

    // 上游 EOF：解码器收尾（产出尾部数据；截断 → 抛）
    void finish(std::string& out)
    {
        if (stream_end_)
            return;
        if (kind_ == Kind::Brotli) {
            // 流未到 end 但输入耗尽 → 截断
            throw std::runtime_error("brotli: 流截断（未到 stream end）");
        }
        // zlib：把剩余输入喂完，期望 Z_STREAM_END
        while (!in_.empty()) {
            if (!pump_zlib(out))
                throw std::runtime_error("zlib: 解码失败");
            if (stream_end_)
                return;
        }
        throw std::runtime_error("zlib: 流截断（未到 stream end）");
    }

    std::shared_ptr<BodySource> upstream_;
    Kind kind_;
    std::string in_;       // 上游未消费输入缓冲
    bool eof_upstream_ = false;
    bool finished_ = false;
    bool stream_end_ = false;
    size_t total_ = 0;       // 已解压总字节（超 total_limit_ 抛异常）
    size_t total_limit_ = 0; // 0 = 无限制
    bool fallback_ = false;       // deflate：允许裸 deflate 回退
    bool fallback_tried_ = false;
    z_stream z_{};
    BrotliDecoderState* br_state_ = nullptr;
};

// ---- AcceptEncodingMiddleware（fetch_design.md §5.6）----
class AcceptEncodingMiddleware : public Middleware {
public:
    std_exec::task<Response> intercept(const Request& req, std::stop_token st,
                                       Handler next) override
    {
        // ---- 前置相位 ----
        // 无 accept-encoding 且无 range（压缩使 Range 偏移失效）才自动加头；
        // 用户自己设了 accept-encoding → 原样透传不解压（undici 语义）
        const bool auto_added = !has_header(req.headers, "accept-encoding") &&
                                !has_header(req.headers, "range");
        Response resp;
        if (auto_added) {
            Request r = req; // req 只读：拷贝后改
            r.headers.push_back({"Accept-Encoding", "gzip, deflate, br"});
            resp = co_await next(r, st);
        } else {
            resp = co_await next(req, st);
        }

        // ---- 后置相位：局部变量 auto_added 即 per-request ctx ----
        if (auto_added && resp.body) {
            if (auto enc = single_content_encoding(resp.headers)) {
                DecompressSource::Kind kind;
                if (*enc == "gzip" || *enc == "x-gzip")
                    kind = DecompressSource::Kind::Gzip;
                else if (*enc == "deflate")
                    kind = DecompressSource::Kind::Deflate;
                else if (*enc == "br")
                    kind = DecompressSource::Kind::Brotli;
                else
                    co_return resp; // 未知编码：透传原始字节
                strip_headers(resp.headers, {"content-encoding", "content-length"});
                resp.body =
                    std::make_shared<DecompressSource>(std::move(resp.body), kind);
            }
        }
        co_return resp;
    }
};

// ---- IntegritySource：SRI 末端校验（fetch_design.md §4.5）----
// 包在解码层之外（摘要对解码后字节）；read 时增量算摘要，EOF 比对。
// 不匹配 → 抛 std::runtime_error → 消费 reject TypeError（getReader 路径流进 Errored）。
class IntegritySource : public BodySource {
public:
    IntegritySource(std::shared_ptr<BodySource> upstream, std::string expected)
        : upstream_(std::move(upstream)), expected_(std::move(expected))
    {
        // expected 形如 "sha256-BASE64"（可多个空白分隔；取第一个）
        const size_t dash = expected_.find('-');
        if (dash == std::string::npos)
            throw std::runtime_error("integrity: 格式非法");
        algo_ = expected_.substr(0, dash);
        digest_b64_ = expected_.substr(dash + 1);
        if (algo_ != "sha256" && algo_ != "sha384" && algo_ != "sha512")
            throw std::runtime_error("integrity: 不支持的算法 " + algo_);
        const EVP_MD* md = EVP_get_digestbyname(algo_.c_str());
        if (!md)
            throw std::runtime_error("integrity: 算法初始化失败");
        mdctx_ = EVP_MD_CTX_new();
        if (!mdctx_ || EVP_DigestInit_ex(mdctx_, md, nullptr) != 1)
            throw std::runtime_error("integrity: 摘要初始化失败");
    }

    ~IntegritySource() override
    {
        if (mdctx_)
            EVP_MD_CTX_free(mdctx_);
    }

    void cancel() override
    {
        if (upstream_)
            upstream_->cancel();
    }

    std_exec::task<std::optional<std::string>> read() override
    {
        auto block = co_await upstream_->read();
        if (!block) {
            // EOF：比对摘要（不匹配 → 抛，消费 reject TypeError）
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int len = 0;
            if (EVP_DigestFinal_ex(mdctx_, digest, &len) != 1)
                throw std::runtime_error("integrity: 摘要计算失败");
            std::string b64 = base64_encode_raw(digest, len);
            // url-safe 变体归一化（-_ → +/）+ 去 padding（与 digest_matches 一致）
            if (norm_b64(b64) != norm_b64(digest_b64_))
                throw std::runtime_error("integrity: 摘要不匹配（SRI 校验失败）");
            co_return std::nullopt;
        }
        if (EVP_DigestUpdate(mdctx_, block->data(), block->size()) != 1)
            throw std::runtime_error("integrity: 摘要更新失败");
        co_return block;
    }

private:
    // url-safe 变体归一化（-/_ → +//）+ 去 padding（SRI 允许两种编码，规范 §4.5）
    static std::string norm_b64(std::string s)
    {
        for (auto& c : s) {
            if (c == '-')
                c = '+';
            else if (c == '_')
                c = '/';
        }
        while (!s.empty() && s.back() == '=')
            s.pop_back();
        return s;
    }

    static std::string base64_encode_raw(const unsigned char* data, size_t len)
    {
        static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                                 "0123456789+/";
        std::string out;
        out.reserve((len + 2) / 3 * 4);
        for (size_t i = 0; i < len; i += 3) {
            const uint32_t v = (static_cast<uint32_t>(data[i]) << 16) |
                               (i + 1 < len ? static_cast<uint32_t>(data[i + 1]) << 8 : 0) |
                               (i + 2 < len ? static_cast<uint32_t>(data[i + 2]) : 0);
            out.push_back(tbl[(v >> 18) & 63]);
            out.push_back(tbl[(v >> 12) & 63]);
            out.push_back(i + 1 < len ? tbl[(v >> 6) & 63] : '=');
            out.push_back(i + 2 < len ? tbl[v & 63] : '=');
        }
        return out;
    }

    std::shared_ptr<BodySource> upstream_;
    std::string expected_;
    std::string algo_;
    std::string digest_b64_;
    EVP_MD_CTX* mdctx_ = nullptr;
};

// ---- Socks5ProxyMiddleware：SOCKS5 代理选路（fetch_design.md §5.7）----
// 机制（握手/隧道）在传输层（socks5_connect + BeastTransport 代理参数），
// 中间件只做策略（route_：url → optional<Socks5Proxy>，空 = 直连）与选路。
// 命中 → 调 transport_->request_via_socks5（不调 next，整条交换走隧道）；
// 未命中 → co_await next（直连）。面向 Transport 抽象 + Route 回调——
// 不依赖具体传输实现（解耦 SOCKS5 策略与 beast，见 fetch_cpp_decoupling.md §7-5）。
class Socks5ProxyMiddleware : public Middleware {
public:
    using Route =
        std::function<std::optional<Socks5Proxy>(const std::string& url)>;

    Socks5ProxyMiddleware(std::shared_ptr<Transport> transport, Route route)
        : transport_(std::move(transport)), route_(std::move(route)) {}

    std_exec::task<Response> intercept(const Request& req, std::stop_token st,
                                       Handler next) override
    {
        if (auto proxy = route_(req.url))
            co_return co_await transport_->request_via_socks5(req, *proxy, st);
        co_return co_await next(req, st);
    }

private:
    std::shared_ptr<Transport> transport_;
    Route route_;
};

} // namespace fetch
