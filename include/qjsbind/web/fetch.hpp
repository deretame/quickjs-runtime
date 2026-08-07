// qjsbind::web —— fetch()（v1 边界）
//
// 流程：input/init → RequestImpl → HttpRequest → FetchBackend::request（Task）
//   → redirect 处理（follow ≤20 跳 / error / manual）→ ResponseImpl。
// 取消：AbortSignal.stop_source 的 token 传入后端；abort() → 后端 socket.cancel()
//   → operation_aborted → set_stopped → 整个 task 链 stopped → reject AbortError。
// 网络错误（DNS/连接/TLS/协议）→ reject TypeError("fetch failed: ...")。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/value.hpp>
#include <qjsbind/web/errors.hpp>
#include <qjsbind/web/interceptor.hpp>
#include <qjsbind/web/net.hpp>
#include <qjsbind/web/request_response.hpp>

// SRI（integrity）摘要计算：web 层直接使用 OpenSSL EVP。
// 注：本头文件仅被链入 qjsbind_net（PUBLIC 传播 OpenSSL）的 target 使用。
#include <openssl/evp.h>

#include <exec/task.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qjsbind::web {

namespace fetch_detail {

inline bool is_redirect_status(int status) {
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

inline bool status_requires_get(int status, const std::string& method) {
    // 303：仅非 GET/HEAD 转 GET；301/302：仅 POST 转 GET
    if (status == 303)
        return method != "GET" && method != "HEAD";
    if ((status == 301 || status == 302) && method == "POST")
        return true;
    return false;
}

// 从响应头取 Location（大小写不敏感；无 → 空串）
inline std::string location_of(const std::vector<Header>& headers) {
    for (const auto& h : headers) {
        std::string lower = h.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower == "location")
            return h.value;
    }
    return {};
}

inline std::vector<Header> without_body_headers(const std::vector<Header>& headers) {
    std::vector<Header> out;
    for (const auto& h : headers) {
        std::string lower = h.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        // 重定向转 GET/HEAD 后剥离全部 body 相关头（fetch 规范；wpt redirect-method）
        if (lower != "content-length" && lower != "content-type" &&
            lower != "content-encoding" && lower != "content-language" &&
            lower != "content-location")
            out.push_back(h);
    }
    return out;
}

// ---- data: URL（WHATWG data URL processor，fetch 规范 §data URL）----

inline std::string percent_decode(const std::string& in) {
    std::string out;
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            const int h = hex(in[i + 1]), l = hex(in[i + 2]);
            if (h >= 0 && l >= 0) {
                out.push_back(static_cast<char>((h << 4) | l));
                i += 2;
                continue;
            }
        }
        out.push_back(in[i]);
    }
    return out;
}

// 标准 base64 解码；失败（非法字符/长度）返回空 optional
inline std::optional<std::string> base64_decode(const std::string& in) {
    static const char* t =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int rev[256];
    for (int i = 0; i < 256; ++i)
        rev[i] = -1;
    for (int i = 0; i < 64; ++i)
        rev[static_cast<unsigned char>(t[i])] = i;
    std::string out;
    int buf = 0, bits = 0;
    for (const char c : in) {
        if (c == '=')
            break; // padding 结束
        const int v = rev[static_cast<unsigned char>(c)];
        if (v < 0)
            return std::nullopt; // 非法字符（含空白）
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

// 解析 data URL（入参为 URL 层已编码的字符串）。成功返回 true 并填充
// mime（Content-Type 头）与 body（解码后的原始字节）。
inline bool parse_data_url(const std::string& url, std::string& mime_out,
                           std::string& body_out) {
    const size_t comma = url.find(',');
    if (comma == std::string::npos)
        return false; // 无逗号 → 解析失败（TypeError）
    std::string meta = url.substr(5, comma - 5); // "data:" 之后、逗号之前
    std::string data = url.substr(comma + 1);
    bool is_base64 = false;
    const size_t semi = meta.rfind(';');
    if (semi != std::string::npos) {
        std::string last = meta.substr(semi + 1);
        for (auto& c : last)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (last == "base64") {
            is_base64 = true;
            meta = meta.substr(0, semi);
        }
    }
    // MIME 规范化：无 type/subtype → 默认；type/subtype 与参数名小写，参数值保留
    if (meta.find('/') == std::string::npos) {
        mime_out = "text/plain;charset=US-ASCII";
    } else {
        std::string out;
        bool in_param_value = false;
        for (size_t i = 0; i < meta.size(); ++i) {
            const char c = meta[i];
            if (c == ';') {
                in_param_value = false;
                out.push_back(c);
            } else if (c == '=') {
                in_param_value = true;
                out.push_back(c);
            } else if (!in_param_value && (c == ' ' || c == '\t')) {
                continue; // 参数名/类型段去空白
            } else if (!in_param_value) {
                out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            } else {
                out.push_back(c);
            }
        }
        mime_out = out;
    }
    if (is_base64) {
        auto decoded = base64_decode(data);
        if (!decoded)
            return false; // base64 非法 → 解析失败（TypeError）
        body_out = std::move(*decoded);
    } else {
        body_out = percent_decode(data);
    }
    return true;
}

// ---- SRI（Subresource Integrity）----
inline std::string sha_digest(const std::string& algo, const std::string& data) {
    const EVP_MD* md = nullptr;
    if (algo == "sha256")
        md = EVP_sha256();
    else if (algo == "sha384")
        md = EVP_sha384();
    else if (algo == "sha512")
        md = EVP_sha512();
    if (!md)
        return {};
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    EVP_Digest(data.data(), data.size(), buf, &len, md, nullptr);
    return std::string(reinterpret_cast<char*>(buf), len);
}

inline std::string base64_encode(const std::string& in) {
    static const char* t =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    for (size_t i = 0; i < in.size(); i += 3) {
        const uint32_t n = (static_cast<uint32_t>(static_cast<uint8_t>(in[i])) << 16) |
                           (i + 1 < in.size() ? static_cast<uint32_t>(static_cast<uint8_t>(in[i + 1])) << 8 : 0) |
                           (i + 2 < in.size() ? static_cast<uint32_t>(static_cast<uint8_t>(in[i + 2])) : 0);
        out += t[(n >> 18) & 63];
        out += t[(n >> 12) & 63];
        out += i + 1 < in.size() ? t[(n >> 6) & 63] : '=';
        out += i + 2 < in.size() ? t[n & 63] : '=';
    }
    return out;
}

// 比对：url-safe 变体归一化（-/_ → +//）+ 去 padding
inline bool digest_matches(const std::string& expected, const std::string& actual) {
    auto norm = [](std::string s) {
        for (auto& c : s) {
            if (c == '-')
                c = '+';
            else if (c == '_')
                c = '/';
        }
        while (!s.empty() && s.back() == '=')
            s.pop_back();
        return s;
    };
    return norm(expected) == norm(actual);
}

// 校验响应体摘要（SRI）。不匹配/无法校验 → TypeError。
inline void check_integrity(JSContext* ctx, const std::string& integrity, int status,
                            const std::string& method, const std::string& body) {
    if (integrity.empty())
        return;
    // 规范：null body（null body status 或 HEAD）无法校验 → 网络错误
    const bool null_body = status == 204 || status == 205 || status == 304 || method == "HEAD";
    if (null_body)
        throw_type_error(ctx, "fetch: integrity 无法校验 null body 响应");
    std::string cur;
    auto verify_item = [&](const std::string& item) {
        const size_t dash = item.find('-');
        if (dash == std::string::npos)
            return false;
        const std::string actual = base64_encode(sha_digest(item.substr(0, dash), body));
        return digest_matches(item.substr(dash + 1), actual);
    };
    bool matched = false;
    for (const char c : integrity + " ") {
        if (c == ' ') {
            if (!cur.empty() && verify_item(cur))
                matched = true;
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!matched)
        throw_type_error(ctx, "fetch: integrity 校验失败");
}

inline exec::task<qjs::Value> fetch_impl(JSContext* ctx, std::shared_ptr<FetchBackend> backend,
                                         FetchHandler chain, std::string method,
                                         std::string url, std::vector<Header> headers,
                                         std::string body, const std::string& redirect_mode,
                                         const std::string& integrity, std::stop_token st) {
    // data: URL：本地构造响应（Node 行为：data URL 可 fetch，type=basic）
    if (url.rfind("data:", 0) == 0) {
        std::string mime, data;
        if (!parse_data_url(url, mime, data))
            throw_type_error(ctx, "fetch: data URL 解析失败");
        if (method == "HEAD")
            data.clear(); // HEAD 响应无 body（wpt scheme-data）
        ResponseImpl r;
        r.status = 200;
        r.status_text = "OK";
        if (!data.empty() || method != "HEAD")
            r.body_stream = bytes_to_stream(ctx, std::move(data));
        r.type = "basic";
        r.url = url;
        r.headers.set_guard(HeadersImpl::Guard::Immutable);
        r.headers.append_raw("Content-Type", mime);
        co_return qjs::Value(ctx, qjs::js_convert<ResponseImpl>::to_js(ctx, r));
    }

    constexpr int kMaxRedirects = 20;
    for (int hop = 0; hop <= kMaxRedirects; ++hop) {
        HttpRequest req;
        req.method = method;
        req.url = url;
        req.headers = headers;
        req.body = body;

        HttpResponse resp;
        try {
            resp = co_await chain(req, st); // 拦截器链（每跳重新走全链，§5.4）
        } catch (const qjs::js_error&) {
            throw; // JS 异常原样透传
        } catch (const std::exception& e) {
            throw_type_error(ctx, "fetch failed: %s", e.what());
        }

        const std::string loc = location_of(resp.headers);
        if (redirect_mode == "error" && is_redirect_status(resp.status)) {
            throw_type_error(ctx, "fetch: redirect mode 为 error");
        }
        if (redirect_mode == "manual" && is_redirect_status(resp.status)) {
            ResponseImpl r;
            r.status = 0;
            r.type = "opaqueredirect";
            r.url = "";
            co_return qjs::Value(ctx, qjs::js_convert<ResponseImpl>::to_js(ctx, r));
        }
        if (redirect_mode == "follow" && is_redirect_status(resp.status) && !loc.empty()) {
            if (hop == kMaxRedirects)
                throw_type_error(ctx, "fetch: 重定向次数超过 20");
            // 相对 Location 以当前 URL 为 base 解析
            url = UrlImpl::parse(ctx, loc, url).href();
            // 303 一律转 GET；301/302 仅 POST 转 GET
            if (status_requires_get(resp.status, method)) {
                method = "GET";
                body.clear();
                headers = without_body_headers(headers);
            }
            continue;
        }

        // 构建 Response（headers guard=response；name 规范化由 append 完成）
        // SRI（M3）：消费末端增量校验——integrity 非空 → 包 IntegritySource
        //（read 时算摘要，EOF 比对；不匹配 → 消费 reject TypeError，设计文档 §4.5）。
        // 空 body（204/205/304/HEAD）仍走立即校验（v1 语义：null body + integrity → TypeError）。
        std::shared_ptr<BodySource> final_body;
        if (!integrity.empty() && resp.body) {
            final_body = std::make_shared<IntegritySource>(std::move(resp.body), integrity);
        } else {
            if (!integrity.empty())
                check_integrity(ctx, integrity, resp.status, method, ""); // null body 检查
            final_body = std::move(resp.body);
        }
        ResponseImpl r;
        r.status = resp.status;
        r.status_text = resp.reason;
        // 流式 body：204/205/304/HEAD → final_body 为 null → body_stream 为 null
        r.body_stream = final_body ? make_stream(io_of(ctx), std::move(final_body), st) : nullptr;
        r.type = "basic"; // 同源 fetch 响应（v1 无跨域，恒 basic）
        r.url = url;
        r.redirected = hop > 0; // 规范：经重定向的响应 redirected=true
        r.headers.set_guard(HeadersImpl::Guard::Immutable); // 规范：fetch 响应 headers 不可变
        for (const auto& h : resp.headers)
            r.headers.append_raw(h.name, h.value); // 绕过 guard 检查（响应组装）
        co_return qjs::Value(ctx, qjs::js_convert<ResponseImpl>::to_js(ctx, r));
    }
    throw_type_error(ctx, "fetch: 重定向次数超过 20");
}

} // namespace fetch_detail

// 安装 fetch 全局函数。backend 由调用方注入（默认实现见 src/net/http_backend）。
inline void install_fetch(qjs::Context& ctx, std::shared_ptr<FetchBackend> backend,
                           std::vector<std::shared_ptr<FetchInterceptor>> interceptors) {
    using namespace fetch_detail;
    // 拦截器链：每跳重走全链（handler 无状态可重用，§5.4）
    const FetchHandler chain = make_chain(interceptors, backend);
    ctx.globals().set(
        "fetch",
        qjs::func(ctx.raw(),
                  [backend, chain](qjs::Ctx ctx, qjs::Value input, qjs::Opt<qjs::Value> init)
                      -> exec::task<qjs::Value> {
                      // 同步部分：解析 input/init → RequestImpl
                      RequestImpl req;
                      qjs::Opt<qjs::Value> input_opt;
                      input_opt.value.emplace(ctx.ctx, JS_DupValue(ctx.ctx, input.raw()));
                      // 规范：fetch(Request) 消费 input 的 body（bodyUsed=true；已消费 → TypeError）。
                      // 注意：init 提供 body 时 input 不被消费（qjs_init 覆盖语义）。
                      bool input_consumed = false;
                      if (JS_IsObject(input.raw())) {
                          auto& reg = qjs::registry_of(ctx.ctx);
                          if (reg.is_registered<RequestImpl>() &&
                              reg.id_of<RequestImpl>(ctx.ctx) == JS_GetClassID(input.raw())) {
                              auto* src = reg.opaque<RequestImpl>(ctx.ctx, input.raw());
                              const bool init_has_body =
                                  init && init->is_object() &&
                                  !qjs::Value(ctx.ctx, JS_GetPropertyStr(ctx.ctx, init->raw(), "body"))
                                       .is_undefined();
                              if (!init_has_body) {
                                  // 已消费 → TypeError；未消费由 qjs_init 的
                                  // try_extract_init_body tee 提取（置 input disturbed）
                                  if (src->body_stream && src->body_stream->disturbed)
                                      throw_type_error(
                                          ctx.ctx, "fetch: Request body 已被消费");
                                  input_consumed = true;
                              }
                          }
                      }
                      req.qjs_init(ctx.ctx, input_opt, init);
                      (void)input_consumed; // req 是拷贝，body_used 语义属于 input 对象本身

                      // 同步 headers（用户可能通过 req.headers 修改过）
                      req.sync_headers(ctx.ctx);
                      // 组装请求头（guard=request 已做 forbidden 检查；再过滤一遍——
                      // headers_from 在 guard 置位前可能已存入了 forbidden 头/值）
                      std::vector<Header> hdrs;
                      for (const auto& [k, v] : req.headers.list) {
                          // Node(undici) 行为：referer/cookie/origin 等用户自定义头
                          // 正常发送（不做 forbidden 过滤）；host/content-length 由
                          // 运行时管理（用户设置被忽略，避免与连接/长度语义冲突）
                          if (k == "host" || k == "content-length")
                              continue;
                          if (qjsbind::web::is_method_override_header(k)) {
                              // 值含 forbidden method（逗号分列 + trim + 小写匹配）→ 不发
                              bool bad = false;
                              std::string part;
                              auto check = [&] {
                                  size_t b = 0, e = part.size();
                                  while (b < e && (part[b] == ' ' || part[b] == '\t')) ++b;
                                  while (e > b && (part[e - 1] == ' ' || part[e - 1] == '\t')) --e;
                                  std::string p = part.substr(b, e - b);
                                  std::transform(p.begin(), p.end(), p.begin(),
                                                 [](unsigned char c) {
                                                     return static_cast<char>(std::tolower(c));
                                                 });
                                  return p == "trace" || p == "track" || p == "connect";
                              };
                              for (char c : v) {
                                  if (c == ',') {
                                      if (check()) { bad = true; break; }
                                      part.clear();
                                  } else {
                                      part.push_back(c);
                                  }
                              }
                              if (!bad && check())
                                  bad = true;
                              if (bad)
                                  continue;
                          }
                          hdrs.push_back({k, v});
                      }
                      // 规范：非 GET/HEAD 请求带 Origin 头（值 = URL origin；Node/浏览器一致）
                      if (req.method != "GET" && req.method != "HEAD" &&
                          req.url.rfind("http", 0) == 0) {
                          UrlImpl u = UrlImpl::parse(ctx.ctx, req.url, "");
                          hdrs.push_back({"Origin", u.origin()});
                      }
                      // 规范：无 body 的 POST/PUT/PATCH 请求带 Content-Length: 0（undici/浏览器一致）
                      if ((req.method == "POST" || req.method == "PUT" || req.method == "PATCH") &&
                          !req.body_stream)
                          hdrs.push_back({"Content-Length", "0"});

                      const std::stop_token st =
                          req.signal ? req.signal->stop.get_token() : std::stop_token{};
                      // signal 已 abort → 立即 reject AbortError
                      if (req.signal && req.signal->aborted)
                          throw qjs::js_error(ctx.ctx, make_abort_error(ctx.ctx).take());

                      // 请求 body 读干（fetch 语义：消费 input 的 body；disturbed 已由
                      // 上面的 input 处理置位，这里实际读取 req 的拷贝流）
                      std::string body;
                      if (req.body_stream) {
                          std::shared_ptr<ReadableStreamImpl> bs = req.body_stream;
                          try {
                              for (;;) {
                                  auto block = co_await bs->read();
                                  if (!block)
                                      break;
                                  body += *block;
                              }
                          } catch (const qjs::js_error&) {
                              throw;
                          } catch (const std::exception& e) {
                              throw_type_error(ctx.ctx, "fetch failed: %s", e.what());
                          }
                      }

                      co_return co_await fetch_impl(ctx.ctx, backend, chain, req.method,
                                                    req.url, std::move(hdrs), std::move(body),
                                                    req.redirect, req.integrity, st);
                  },
                  "fetch"));
}

} // namespace qjsbind::web
