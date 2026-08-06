// qjsbind::web —— Request / Response（fetch 规范 v1 边界）
//
// body 支持：string / ArrayBuffer / TypedArray / URLSearchParams / undefined；
// 消费：text() / json() / arrayBuffer()；不做 blob() / formData() / ReadableStream。
// Request 相对 URL 以 globalThis.location.href 为 base（无 location 时仅绝对 URL）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/rt_value.hpp>
#include <qjsbind/value.hpp>
#include <qjsbind/web/abort.hpp>
#include <qjsbind/web/headers.hpp>
#include <qjsbind/web/url.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qjsbind::web {

// ---------- body 提取 / 消费 ----------

struct ExtractedBody {
    std::string bytes;
    std::string content_type; // 可能为空
    bool has = false;
};

inline bool is_url_params_instance(JSContext* ctx, JSValueConst v) {
    if (!JS_IsObject(v))
        return false;
    auto& reg = qjs::registry_of(ctx);
    if (!reg.is_registered<UrlSearchParamsImpl>())
        return false;
    return reg.id_of<UrlSearchParamsImpl>(ctx) == JS_GetClassID(v);
}

// JS body 参数 → 字节（undefined/null → has=false；其他类型 → TypeError）
inline ExtractedBody extract_body(JSContext* ctx, JSValueConst body) {
    ExtractedBody out;
    if (JS_IsUndefined(body) || JS_IsNull(body))
        return out;
    out.has = true;
    if (JS_IsString(body)) {
        size_t len = 0;
        const uint16_t* units = JS_ToCStringLenUTF16(ctx, &len, body);
        if (!units)
            throw qjs::js_error(ctx, JS_GetException(ctx));
        out.bytes = utf16_to_utf8(units, len);
        JS_FreeCStringUTF16(ctx, units);
        return out;
    }
    if (is_url_params_instance(ctx, body)) {
        const auto* p = qjs::registry_of(ctx).opaque<UrlSearchParamsImpl>(ctx, body);
        out.bytes = p->to_query();
        out.content_type = "application/x-www-form-urlencoded;charset=UTF-8";
        return out;
    }
    if (JS_GetTypedArrayType(body) >= 0 || JS_IsArrayBuffer(body)) {
        out.bytes = js_bytes_from(ctx, body);
        return out;
    }
    JS_Throw(ctx, JS_NewTypeError(ctx, "body: 不支持的 body 类型"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
}

// 消费字节：text / json / arrayBuffer
inline qjs::Value consume_text(JSContext* ctx, const std::string& bytes) {
    return qjs::Value(ctx, JS_NewStringLen(ctx, bytes.data(), bytes.size()));
}
inline qjs::Value consume_json(JSContext* ctx, const std::string& bytes) {
    JSValue v = JS_ParseJSON(ctx, bytes.data(), bytes.size(), "<json>");
    if (JS_IsException(v))
        throw qjs::js_error(ctx, JS_GetException(ctx));
    return qjs::Value(ctx, v);
}
inline qjs::Value consume_array_buffer(JSContext* ctx, const std::string& bytes) {
    return qjs::Value(ctx, JS_NewArrayBufferCopy(
                              ctx, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()));
}

// 消费入口（置 bodyUsed；重复消费 → TypeError）
template <class Self, class Fn>
qjs::Value consume_impl(JSContext* ctx, Self& self, const char* what, Fn&& fn) {
    if (self.body_used)
        JS_Throw(ctx, JS_NewTypeError(ctx, "%s: body 已被消费", what));
    throw qjs::js_error(ctx, JS_GetException(ctx));
    self.body_used = true;
    return fn(ctx, self.body_bytes);
}

// ---------- Request ----------

struct RequestImpl {
    std::string method = "GET";
    std::string url;             // 绝对 URL
    HeadersImpl headers;         // guard=request
    std::string body_bytes;
    bool has_body = false;
    bool body_used = false;
    std::string redirect = "follow";
    AbortSignalImpl* signal = nullptr; // 借用（signal JS 对象持有）
    qjs::RtValue signal_js;            // 持有 signal JS 引用（fetch 取消用）

    RequestImpl() = default;
    // 拷贝：signal 不复制（fetch 规范：Request clone 不继承 signal）
    RequestImpl(const RequestImpl& o)
        : method(o.method), url(o.url), headers(o.headers), body_bytes(o.body_bytes),
          has_body(o.has_body), body_used(o.body_used), redirect(o.redirect), signal(nullptr) {}
    RequestImpl& operator=(const RequestImpl& o) {
        method = o.method;
        url = o.url;
        headers = o.headers;
        body_bytes = o.body_bytes;
        has_body = o.has_body;
        body_used = o.body_used;
        redirect = o.redirect;
        signal = nullptr;
        signal_js = qjs::RtValue(); // 释放旧引用（若有）
        return *this;
    }

    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> input, qjs::Opt<qjs::Value> init) {
        // 1. input：Request 实例 → 拷贝；string/URL → 解析绝对 URL
        if (input && !input->is_undefined() && !input->is_null()) {
            if (JS_IsObject(input->raw())) {
                auto& reg = qjs::registry_of(ctx);
                if (reg.is_registered<RequestImpl>() &&
                    reg.id_of<RequestImpl>(ctx) == JS_GetClassID(input->raw())) {
                    *this = *reg.opaque<RequestImpl>(ctx, input->raw());
                } else {
                    // URL 实例或字符串
                    url = url_string_of(ctx, input->raw());
                }
            } else if (input->is_string()) {
                url = resolve_url(ctx, input->as<std::string>());
            } else {
                JS_Throw(ctx, JS_NewTypeError(ctx, "Request: input 类型不支持"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
            }
        } else {
            JS_Throw(ctx, JS_NewTypeError(ctx, "Request: 缺少 input"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        }
        headers.set_guard(HeadersImpl::Guard::Request);
        if (signal)
            signal = nullptr; // 拷贝 input 时不继承 signal（规范：signal 不复制）

        // 2. init
        if (init && init->is_object()) {
            qjs::Object obj(*init);
            qjs::Value method = obj.get("method");
            if (!method.is_undefined())
                this->method = normalize_method(ctx, method.as<std::string>());
            qjs::Value hdrs = obj.get("headers");
            if (!hdrs.is_undefined() && !hdrs.is_null()) {
                headers = headers_from(ctx, hdrs.raw());
                headers.set_guard(HeadersImpl::Guard::Request);
            }
            qjs::Value body = obj.get("body");
            if (!body.is_undefined() && !body.is_null()) {
                ExtractedBody b = extract_body(ctx, body.raw());
                body_bytes = std::move(b.bytes);
                has_body = b.has;
                if (!b.content_type.empty())
                    headers.append(ctx, "Content-Type", b.content_type);
            }
            qjs::Value redirect = obj.get("redirect");
            if (!redirect.is_undefined())
                this->redirect = normalize_redirect(ctx, redirect.as<std::string>());
            qjs::Value signal_v = obj.get("signal");
            if (!signal_v.is_undefined() && !signal_v.is_null()) {
                auto& reg = qjs::registry_of(ctx);
                if (reg.is_registered<AbortSignalImpl>() &&
                    reg.id_of<AbortSignalImpl>(ctx) == JS_GetClassID(signal_v.raw())) {
                    signal = reg.opaque<AbortSignalImpl>(ctx, signal_v.raw());
                    signal_js = qjs::RtValue(JS_GetRuntime(ctx), JS_DupValue(ctx, signal_v.raw()));
                } else {
                    JS_Throw(ctx, JS_NewTypeError(ctx, "Request: signal 类型不支持"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
                }
            }
        }
        // GET/HEAD 带 body → TypeError（fetch 规范）
        if (has_body && (method == "GET" || method == "HEAD"))
            JS_Throw(ctx, JS_NewTypeError(ctx, "Request: GET/HEAD 不能带 body"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
    }

    static std::string normalize_method(JSContext* ctx, std::string m) {
        for (auto& c : m)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (m.empty())
            JS_Throw(ctx, JS_NewTypeError(ctx, "Request: method 为空"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        return m;
    }
    static std::string normalize_redirect(JSContext* ctx, const std::string& r) {
        if (r != "follow" && r != "error" && r != "manual")
            JS_Throw(ctx, JS_NewTypeError(ctx, "Request: redirect 非法"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        return r;
    }
    // GC 标记：signal 对象引用
    void qjs_mark(JSRuntime* rt, JS_MarkFunc* mark_func) { signal_js.mark(rt, mark_func); }

    static std::string url_string_of(JSContext* ctx, JSValueConst v);
    static std::string resolve_url(JSContext* ctx, const std::string& str);
};

// URL 实例 → 序列化；相对字符串 → location.href 为 base 解析
inline std::string RequestImpl::url_string_of(JSContext* ctx, JSValueConst v) {
    auto& reg = qjs::registry_of(ctx);
    if (reg.is_registered<UrlImpl>() && reg.id_of<UrlImpl>(ctx) == JS_GetClassID(v))
        return reg.opaque<UrlImpl>(ctx, v)->href();
    if (JS_IsString(v))
        return resolve_url(ctx, std::string(JS_ToCString(ctx, v)));
    JS_Throw(ctx, JS_NewTypeError(ctx, "Request: input 类型不支持"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
}

inline std::string RequestImpl::resolve_url(JSContext* ctx, const std::string& str) {
    // base = globalThis.location.href（wpt 运行器等环境注入）
    std::string base;
    JSValue g = JS_GetGlobalObject(ctx);
    JSValue loc = JS_GetPropertyStr(ctx, g, "location");
    if (JS_IsObject(loc)) {
        JSValue href = JS_GetPropertyStr(ctx, loc, "href");
        if (JS_IsString(href)) {
            size_t len = 0;
            const char* s = JS_ToCStringLen(ctx, &len, href);
            if (s) {
                base.assign(s, len);
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, href);
    }
    JS_FreeValue(ctx, loc);
    JS_FreeValue(ctx, g);
    return UrlImpl::parse(ctx, str, base).href();
}

inline void install_request(qjs::Context& ctx) {
    auto cls = qjs::class_<RequestImpl>(ctx, "Request")
                   .constructor<qjs::Opt<qjs::Value>, qjs::Opt<qjs::Value>>()
                   .getter("method", [](qjs::This<RequestImpl> self) { return self->method; })
                   .getter("url", [](qjs::This<RequestImpl> self) { return self->url; })
                   .getter("headers", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       return qjs::Value(ctx.ctx, qjs::js_convert<HeadersImpl>::to_js(ctx.ctx, self->headers));
                   })
                   .getter("bodyUsed", [](qjs::This<RequestImpl> self) { return self->body_used; })
                   .getter("redirect", [](qjs::This<RequestImpl> self) { return self->redirect; })
                   .getter("signal", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       if (self->signal_js.empty())
                           return qjs::Value(ctx.ctx, JS_UNDEFINED);
                       return qjs::Value(ctx.ctx, self->signal_js.dup(ctx.ctx));
                   })
                   .method("clone", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       if (self->body_used)
                           throw qjs::js_error(ctx.ctx,
                                               JS_Throw(ctx.ctx, JS_NewTypeError(ctx.ctx, "Request: body 已被消费")));
                       return qjs::Value(ctx.ctx, qjs::js_convert<RequestImpl>::to_js(ctx.ctx, *self));
                   })
                   .method("text", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Request", consume_text);
                   })
                   .method("json", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Request", consume_json);
                   })
                   .method("arrayBuffer", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Request", consume_array_buffer);
                   });
    ctx.globals().set("Request", cls.constructor_function());
}

// ---------- Response ----------

struct ResponseImpl {
    int status = 200;
    std::string status_text;
    HeadersImpl headers; // guard=response
    std::string body_bytes;
    bool has_body = false;
    bool body_used = false;
    std::string type = "default";
    std::string url;
    bool redirected = false;

    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> body, qjs::Opt<qjs::Value> init) {
        headers.set_guard(HeadersImpl::Guard::Response);
        if (init && init->is_object()) {
            qjs::Object obj(*init);
            qjs::Value status_v = obj.get("status");
            if (!status_v.is_undefined()) {
                const int s = status_v.as<int>();
                if (s < 200 || s > 599)
                    JS_Throw(ctx, JS_NewTypeError(ctx, "Response: status 必须在 200-599"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
                status = s;
            }
            qjs::Value st = obj.get("statusText");
            if (!st.is_undefined())
                status_text = st.as<std::string>();
            for (const char c : status_text)
                if (c == '\r' || c == '\n')
                    throw qjs::js_error(ctx,
                                        JS_Throw(ctx, JS_NewTypeError(ctx, "Response: statusText 含 CR/LF")));
            qjs::Value hdrs = obj.get("headers");
            if (!hdrs.is_undefined() && !hdrs.is_null()) {
                headers = headers_from(ctx, hdrs.raw());
                headers.set_guard(HeadersImpl::Guard::Response);
            }
        }
        if (body && !body->is_undefined() && !body->is_null()) {
            ExtractedBody b = extract_body(ctx, body->raw());
            body_bytes = std::move(b.bytes);
            has_body = b.has;
        }
        // 规范：204/205/304 无 body
        if (status == 204 || status == 205 || status == 304)
            has_body = false;
    }

    bool ok() const { return status >= 200 && status <= 299; }
};

inline void install_response(qjs::Context& ctx) {
    auto cls = qjs::class_<ResponseImpl>(ctx, "Response")
                   .constructor<qjs::Opt<qjs::Value>, qjs::Opt<qjs::Value>>()
                   .getter("status", [](qjs::This<ResponseImpl> self) { return self->status; })
                   .getter("statusText", [](qjs::This<ResponseImpl> self) { return self->status_text; })
                   .getter("ok", [](qjs::This<ResponseImpl> self) { return self->ok(); })
                   .getter("type", [](qjs::This<ResponseImpl> self) { return self->type; })
                   .getter("url", [](qjs::This<ResponseImpl> self) { return self->url; })
                   .getter("redirected", [](qjs::This<ResponseImpl> self) { return self->redirected; })
                   .getter("headers", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       return qjs::Value(ctx.ctx, qjs::js_convert<HeadersImpl>::to_js(ctx.ctx, self->headers));
                   })
                   .getter("bodyUsed", [](qjs::This<ResponseImpl> self) { return self->body_used; })
                   .method("clone", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       if (self->body_used)
                           throw qjs::js_error(ctx.ctx,
                                               JS_Throw(ctx.ctx, JS_NewTypeError(ctx.ctx, "Response: body 已被消费")));
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, *self));
                   })
                   .method("text", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Response", consume_text);
                   })
                   .method("json", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Response", consume_json);
                   })
                   .method("arrayBuffer", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       return consume_impl(ctx.ctx, *self, "Response", consume_array_buffer);
                   })
                   .static_method("error", [](qjs::Ctx ctx) -> qjs::Value {
                       ResponseImpl r;
                       r.status = 0;
                       r.type = "error";
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, r));
                   })
                   .static_method("redirect", [](qjs::Ctx ctx, const std::string& url,
                                                 qjs::Opt<int> status) -> qjs::Value {
                       const int s = status ? *status : 302;
                       if (s != 301 && s != 302 && s != 303 && s != 307 && s != 308)
                           throw qjs::js_error(ctx.ctx,
                                               JS_Throw(ctx.ctx, JS_NewTypeError(ctx.ctx, "Response: redirect 状态非法")));
                       ResponseImpl r;
                       r.status = s;
                       r.type = "default";
                       r.headers.append(ctx.ctx, "Location", RequestImpl::resolve_url(ctx.ctx, url));
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, r));
                   });
    ctx.globals().set("Response", cls.constructor_function());
}

} // namespace qjsbind::web
