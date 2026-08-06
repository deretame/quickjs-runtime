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
#include <qjsbind/web/net.hpp>
#include <qjsbind/web/request_response.hpp>

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
    if (status == 303)
        return true;
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
        if (lower != "content-length" && lower != "content-type")
            out.push_back(h);
    }
    return out;
}

inline exec::task<qjs::Value> fetch_impl(JSContext* ctx, std::shared_ptr<FetchBackend> backend,
                                         std::string method, std::string url,
                                         std::vector<Header> headers, std::string body,
                                         const std::string& redirect_mode, std::stop_token st) {
    constexpr int kMaxRedirects = 20;
    for (int hop = 0; hop <= kMaxRedirects; ++hop) {
        HttpRequest req;
        req.method = method;
        req.url = url;
        req.headers = headers;
        req.body = body;

        HttpResponse resp;
        try {
            resp = co_await backend->request(std::move(req), st);
        } catch (const qjs::js_error&) {
            throw; // JS 异常原样透传
        } catch (const std::exception& e) {
            JS_Throw(ctx, JS_NewTypeError(ctx, "fetch failed: %s", e.what()));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        }

        const std::string loc = location_of(resp.headers);
        if (redirect_mode == "error" && is_redirect_status(resp.status)) {
            JS_Throw(ctx, JS_NewTypeError(ctx, "fetch: redirect mode 为 error"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
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
                JS_Throw(ctx, JS_NewTypeError(ctx, "fetch: 重定向次数超过 20"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
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
        ResponseImpl r;
        r.status = resp.status;
        r.status_text = resp.reason;
        r.body_bytes = std::move(resp.body);
        r.has_body = true;
        r.type = "default";
        r.url = url;
        r.headers.set_guard(HeadersImpl::Guard::Response);
        for (const auto& h : resp.headers)
            r.headers.append(ctx, h.name, h.value);
        co_return qjs::Value(ctx, qjs::js_convert<ResponseImpl>::to_js(ctx, r));
    }
    JS_Throw(ctx, JS_NewTypeError(ctx, "fetch: 重定向次数超过 20"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
}

} // namespace fetch_detail

// 安装 fetch 全局函数。backend 由调用方注入（默认实现见 src/net/http_backend）。
inline void install_fetch(qjs::Context& ctx, std::shared_ptr<FetchBackend> backend) {
    using namespace fetch_detail;
    ctx.globals().set(
        "fetch",
        qjs::func(ctx.raw(),
                  [backend](qjs::Ctx ctx, qjs::Value input, qjs::Opt<qjs::Value> init)
                      -> exec::task<qjs::Value> {
                      // 同步部分：解析 input/init → RequestImpl
                      RequestImpl req;
                      qjs::Opt<qjs::Value> input_opt;
                      input_opt.value.emplace(ctx.ctx, JS_DupValue(ctx.ctx, input.raw()));
                      req.qjs_init(ctx.ctx, input_opt, init);

                      // 组装请求头（guard=request 已做 forbidden 检查）
                      std::vector<Header> hdrs;
                      for (const auto& [k, v] : req.headers.list)
                          hdrs.push_back({k, v});

                      const std::stop_token st =
                          req.signal ? req.signal->stop.get_token() : std::stop_token{};
                      // signal 已 abort → 立即 reject AbortError
                      if (req.signal && req.signal->aborted)
                          throw qjs::js_error(ctx.ctx, make_abort_error(ctx.ctx).take());

                      co_return co_await fetch_impl(ctx.ctx, backend, req.method, req.url,
                                                    std::move(hdrs), req.body_bytes,
                                                    req.redirect, st);
                  },
                  "fetch"));
}

} // namespace qjsbind::web
