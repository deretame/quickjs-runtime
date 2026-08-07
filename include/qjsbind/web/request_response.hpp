// qjsbind::web —— Request / Response（fetch 规范 v1 边界）
//
// body 支持：string / ArrayBuffer / TypedArray / URLSearchParams / undefined；
// 消费：text() / json() / arrayBuffer() / formData()；不做 blob() / ReadableStream。
// Request 相对 URL 以 globalThis.location.href 为 base（无 location 时仅绝对 URL）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/rt_value.hpp>
#include <qjsbind/value.hpp>
#include <qjsbind/web/abort.hpp>
#include <qjsbind/web/errors.hpp>
#include <qjsbind/web/headers.hpp>
#include <qjsbind/web/url.hpp>

#include <exec/task.hpp>

#include <optional>
#include <cstdlib>
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
        // 规范：string body 默认 Content-Type
        out.content_type = "text/plain;charset=UTF-8";
        JS_FreeCStringUTF16(ctx, units);
        return out;
    }
    if (is_url_params_instance(ctx, body)) {
        const auto* p = qjs::registry_of(ctx).opaque<UrlSearchParamsImpl>(ctx, body);
        out.bytes = p->to_query();
        out.content_type = "application/x-www-form-urlencoded;charset=UTF-8";
        return out;
    }
    if (JS_GetTypedArrayType(body) >= 0 || JS_IsArrayBuffer(body) || JS_IsDataView(body)) {
        out.bytes = js_bytes_from(ctx, body);
        return out;
    }
    if (try_blob_bytes(ctx, body, out.bytes)) {
        // Blob/File：Content-Type 取自 type（规范：body 是 Blob 时自动设置）
        if (is_blob_instance(ctx, body))
            out.content_type = qjs::registry_of(ctx).opaque<BlobImpl>(ctx, body)->type;
        else if (is_file_instance(ctx, body))
            out.content_type = qjs::registry_of(ctx).opaque<FileImpl>(ctx, body)->blob.type;
        return out;
    }
    if (is_form_data_instance(ctx, body)) {
        // FormData → multipart/form-data（随机 boundary；规范语义）
        const auto* fd = qjs::registry_of(ctx).opaque<FormDataImpl>(ctx, body);
        static const char* hexd = "0123456789abcdef";
        std::string boundary = "----qjsformdata";
        for (int i = 0; i < 16; ++i)
            boundary.push_back(hexd[(rand() >> 4) & 15]);
        out.bytes = encode_multipart(*fd, boundary);
        out.content_type = "multipart/form-data; boundary=" + boundary;
        return out;
    }
    // 其他值（对象/数字/布尔等）：ToString 后按字符串处理（fetch 规范 body 提取；wpt request-init-002）
    {
        size_t len = 0;
        const uint16_t* units = JS_ToCStringLenUTF16(ctx, &len, body);
        if (!units)
            throw qjs::js_error(ctx, JS_GetException(ctx)); // Symbol 等 → TypeError
        out.bytes = utf16_to_utf8(units, len);
        out.content_type = "text/plain;charset=UTF-8";
        JS_FreeCStringUTF16(ctx, units);
        return out;
    }
}

// 消费字节：text / json / arrayBuffer
inline qjs::Value consume_text(JSContext* ctx, const std::string& bytes) {
    // 规范：text() 走 UTF-8 decode（剥离 BOM，与 TextDecoder 默认一致；wpt 测试权威）
    std::string s = bytes;
    if (s.size() >= 3 && static_cast<uint8_t>(s[0]) == 0xEF &&
        static_cast<uint8_t>(s[1]) == 0xBB && static_cast<uint8_t>(s[2]) == 0xBF)
        s.erase(0, 3);
    return qjs::Value(ctx, JS_NewStringLen(ctx, s.data(), s.size()));
}
inline qjs::Value consume_json(JSContext* ctx, const std::string& bytes) {
    // 规范：JSON 解析前先做 UTF-8 decode（去 BOM）——wpt json.any.js
    std::string s = bytes;
    if (s.size() >= 3 && static_cast<uint8_t>(s[0]) == 0xEF &&
        static_cast<uint8_t>(s[1]) == 0xBB && static_cast<uint8_t>(s[2]) == 0xBF)
        s.erase(0, 3);
    JSValue v = JS_ParseJSON(ctx, s.data(), s.size(), "<json>");
    if (JS_IsException(v))
        throw qjs::js_error(ctx, JS_GetException(ctx));
    return qjs::Value(ctx, v);
}
inline qjs::Value consume_array_buffer(JSContext* ctx, const std::string& bytes) {
    return qjs::Value(ctx, JS_NewArrayBufferCopy(
                              ctx, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()));
}
// 规范：bytes() 返回 Uint8Array（Body mixin；wpt request/response-consume 的
// "Consume ... body as bytes"：instanceof Uint8Array + buffer 内容一致）
inline qjs::Value consume_bytes(JSContext* ctx, const std::string& bytes) {
    return qjs::Value(ctx, JS_NewUint8ArrayCopy(
                               ctx, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()));
}
// 规范：blob() 返回的 Blob.type = 响应 Content-Type（小写；保留参数——
// 浏览器实测含 boundary，`new Response(blob).formData()` 依赖它）
inline qjs::Value consume_blob(JSContext* ctx, const std::string& bytes, const std::string& type) {
    // 与 Blob 构造同一规范化（小写 + trim，保留 MIME 参数如 boundary）
    BlobImpl b;
    b.bytes = bytes;
    b.type = BlobImpl::normalize_type(type);
    return qjs::Value(ctx, qjs::js_convert<BlobImpl>::to_js(ctx, b));
}

// 消费入口（置 bodyUsed；重复消费 → TypeError）
template <class Self, class Fn>
qjs::Value consume_impl(JSContext* ctx, Self& self, const char* what, Fn&& fn) {
    if (self.body_used)
        throw_type_error(ctx, "%s: body 已被消费", what);
    // 规范：body 为 null（无 body）时消费直接返回空结果，不置 bodyUsed
    //（wpt request/response-consume-empty：text/json/blob/arrayBuffer 后
    // assert_false(bodyUsed)；有 body 的消费才置位）
    if (!self.has_body)
        return fn(ctx, self.body_bytes);
    self.body_used = true;
    return fn(ctx, self.body_bytes);
}

// 读取当前 content-type：优先从 headers JS 对象（用户 set/delete 后同步）；
// 未访问过 headers 时退回内部 list（headers getter 是独立拷贝，见 install 注释）
template <class T>
std::string content_type_of(JSContext* ctx, const T& self) {
    if (!self.headers_js.empty()) {
        auto* h = qjs::registry_of(ctx).opaque<HeadersImpl>(ctx, self.headers_js.raw());
        if (h) {
            auto v = h->get(ctx, "content-type");
            if (v)
                return *v;
            return "";
        }
    }
    return self.headers.get(ctx, "content-type").value_or("");
}

// Content-Type → MIME essence（';' 前部分 trim + ASCII 小写；
// formData() 的 multipart/urlencoded 分支判断用）
inline std::string mime_essence(const std::string& ct) {
    const size_t semi = ct.find(';');
    std::string s = ct.substr(0, semi);
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t'))
        ++b;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t'))
        --e;
    s = s.substr(b, e - b);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// 解析 URL 的端口并做 blocked 检查（定义见 RequestImpl 之后；类内方法先声明）
inline void check_url_ports(JSContext* ctx, const std::string& url);

// 前向声明（RequestImpl::qjs_init 的 init.body 需检查 Response 实例）
struct ResponseImpl;

// init.body 是 Request/Response 实例时复制其内部 body（bodyUsed → TypeError）。
// 定义见 ResponseImpl 之后（需完整类型）；RequestImpl::qjs_init 里调用。
inline bool try_extract_init_body(JSContext* ctx, JSValueConst v, ExtractedBody& out);

// ---------- Request ----------

struct RequestImpl {
    std::string method = "GET";
    std::string url;             // 绝对 URL
    HeadersImpl headers;         // guard=request
    std::string body_bytes;
    bool has_body = false;
    bool body_used = false;
    std::string redirect = "follow";
    std::string integrity;            // SRI 元数据（空 = 不校验）
    AbortSignalImpl* signal = nullptr; // 借用（signal JS 对象持有）
    qjs::RtValue signal_js;            // 持有 signal JS 引用（fetch 取消用）
    qjs::RtValue headers_js;           // 缓存的 Headers JS 对象（同一对象语义）

    RequestImpl() = default;
    // 拷贝：signal 不复制（fetch 规范：Request clone 不继承 signal）
    RequestImpl(const RequestImpl& o)
        : method(o.method), url(o.url), headers(o.headers), body_bytes(o.body_bytes),
          has_body(o.has_body), body_used(o.body_used), redirect(o.redirect),
          integrity(o.integrity), signal(nullptr) {}
    RequestImpl& operator=(const RequestImpl& o) {
        method = o.method;
        url = o.url;
        headers = o.headers;
        body_bytes = o.body_bytes;
        has_body = o.has_body;
        body_used = o.body_used;
        redirect = o.redirect;
        integrity = o.integrity;
        signal = nullptr;
        signal_js = qjs::RtValue(); // 释放旧引用（若有）
        headers_js = qjs::RtValue();
        return *this;
    }

    // 从缓存的 Headers JS 对象同步回数据（用户可能通过 r.headers 修改过）
    void sync_headers(JSContext* ctx) {
        if (!headers_js.empty()) {
            const auto* h = qjs::registry_of(ctx).opaque<HeadersImpl>(ctx, headers_js.dup(ctx));
            if (h)
                headers = *h;
        }
    }
    // Headers getter 缓存实现（同一对象语义，wpt 要求）
    qjs::Value headers_value(qjs::Ctx ctx, qjs::This<RequestImpl> self) {
        if (self->headers_js.empty())
            self->headers_js = qjs::RtValue(JS_GetRuntime(ctx.ctx),
                                            qjs::js_convert<HeadersImpl>::to_js(ctx.ctx, self->headers));
        return qjs::Value(ctx.ctx, self->headers_js.dup(ctx.ctx));
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
                // 走 url_string_of：UTF-16 单元转 UTF-8（孤立代理 → U+FFFD）
                url = url_string_of(ctx, input->raw());
            } else {
                throw_type_error(ctx, "Request: input 类型不支持");
            }
        } else {
            throw_type_error(ctx, "Request: 缺少 input");
        }
        check_url_ports(ctx, url); // fetch 规范 #port-blocking
        headers.set_guard(HeadersImpl::Guard::Request);
        if (signal)
            signal = nullptr; // 拷贝 input 时不继承 signal（规范：signal 不复制）

        // 2. init
        if (init && init->is_object()) {
            qjs::Object obj(*init);
            // init 本身是 Request/Response 实例（new Request(url, req)）：复制其内部 body
            ExtractedBody init_body;
            if (try_extract_init_body(ctx, init->raw(), init_body)) {
                body_bytes = std::move(init_body.bytes);
                has_body = init_body.has;
                body_used = false;
                if (!init_body.content_type.empty())
                    headers.append(ctx, "Content-Type", init_body.content_type);
            }
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
                ExtractedBody b;
                // init 是 Request/Response 实例时 body getter 返回 null（v1 无流），
                // 但规范要求复制其内部 body（bodyUsed → TypeError）
                if (!try_extract_init_body(ctx, body.raw(), b))
                    b = extract_body(ctx, body.raw());
                body_bytes = std::move(b.bytes);
                has_body = b.has;
                body_used = false; // 覆盖后重置（input 拷贝可能已消费）
                if (!b.content_type.empty() && !headers.has(ctx, "Content-Type"))
                    headers.append(ctx, "Content-Type", b.content_type);
            }
            qjs::Value redirect = obj.get("redirect");
            if (!redirect.is_undefined())
                this->redirect = normalize_redirect(ctx, redirect.as<std::string>());
            qjs::Value integrity = obj.get("integrity");
            if (!integrity.is_undefined() && !integrity.is_null()) {
                // SRI 元数据解析（fetch 规范 §4.7）：空格分隔项，每项 algo-base64；
                // 算法必须 sha256/sha384/sha512，digest 为 base64（标准或 url-safe，可去 padding）
                const std::string meta = integrity.as<std::string>();
                auto check_item = [](const std::string& item) -> bool {
                    const size_t dash = item.find('-');
                    if (dash == std::string::npos || dash + 1 >= item.size())
                        return false;
                    const std::string algo = item.substr(0, dash);
                    if (algo != "sha256" && algo != "sha384" && algo != "sha512")
                        return false;
                    for (size_t i = dash + 1; i < item.size(); ++i) {
                        const char c = item[i];
                        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '/' ||
                              c == '-' || c == '_' || c == '='))
                            return false;
                    }
                    return true;
                };
                bool ok = true;
                std::string cur;
                for (const char c : meta + " ") {
                    if (c == ' ') {
                        if (!cur.empty() && !check_item(cur)) {
                            ok = false;
                            break;
                        }
                        cur.clear();
                    } else {
                        cur.push_back(c);
                    }
                }
                if (!ok)
                    throw_type_error(ctx, "Request: integrity 元数据非法");
                this->integrity = meta;
            }
            qjs::Value signal_v = obj.get("signal");
            if (!signal_v.is_undefined() && !signal_v.is_null()) {
                auto& reg = qjs::registry_of(ctx);
                if (reg.is_registered<AbortSignalImpl>() &&
                    reg.id_of<AbortSignalImpl>(ctx) == JS_GetClassID(signal_v.raw())) {
                    signal = reg.opaque<AbortSignalImpl>(ctx, signal_v.raw());
                    signal_js = qjs::RtValue(JS_GetRuntime(ctx), JS_DupValue(ctx, signal_v.raw()));
                } else {
                    throw_type_error(ctx, "Request: signal 类型不支持");
                }
            }
        }
        // GET/HEAD 带 body → TypeError（fetch 规范）
        if (has_body && (method == "GET" || method == "HEAD"))
            throw_type_error(ctx, "Request: GET/HEAD 不能带 body");
    }

    static std::string normalize_method(JSContext* ctx, std::string m) {
        for (auto& c : m)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (m.empty())
            throw_type_error(ctx, "Request: method 为空");
        return m;
    }
    static std::string normalize_redirect(JSContext* ctx, const std::string& r) {
        if (r != "follow" && r != "error" && r != "manual")
            throw_type_error(ctx, "Request: redirect 非法");
        return r;
    }
    // GC 标记：signal 对象引用
    void qjs_mark(JSRuntime* rt, JS_MarkFunc* mark_func) {
        signal_js.mark(rt, mark_func);
        headers_js.mark(rt, mark_func);
    }

    static std::string url_string_of(JSContext* ctx, JSValueConst v);
    static std::string resolve_url(JSContext* ctx, const std::string& str);
};

// fetch 规范 #port-blocking：Request URL 的端口在列表内 → 构造时 TypeError
inline bool is_blocked_port(int port) {
    static const int kBad[] = {
        0, 1, 7, 9, 11, 13, 15, 17, 19, 20, 21, 22, 23, 25, 37, 42, 43, 53, 69,
        77, 79, 87, 95, 101, 102, 103, 104, 109, 110, 111, 113, 115, 117, 119,
        123, 135, 137, 139, 143, 161, 179, 389, 427, 465, 512, 513, 514, 515,
        526, 530, 531, 532, 540, 548, 554, 556, 563, 587, 601, 636, 989, 990,
        993, 995, 1719, 1720, 1723, 2049, 3659, 4045, 4190, 5060, 5061, 6000,
        6566, 6665, 6666, 6667, 6668, 6669, 6679, 6697, 10080,
    };
    for (int b : kBad)
        if (port == b)
            return true;
    return false;
}

// 解析 URL 的端口并做 blocked 检查（绝对 URL 字符串）
inline void check_url_ports(JSContext* ctx, const std::string& url) {
    auto r = boost::urls::parse_uri_reference(url);
    if (r.has_error())
        return; // 解析失败由 URL 层抛
    if (r->has_port()) {
        try {
            const int p = std::stoi(std::string(r->port()));
            if (is_blocked_port(p))
                throw_type_error(ctx, "Request: URL 端口 %d 被禁止", p);
        } catch (const std::invalid_argument&) {
            return;
        }
    }
}

// URL 实例 → 序列化；相对字符串 → location.href 为 base 解析
inline std::string RequestImpl::url_string_of(JSContext* ctx, JSValueConst v) {
    auto& reg = qjs::registry_of(ctx);
    if (reg.is_registered<UrlImpl>() && reg.id_of<UrlImpl>(ctx) == JS_GetClassID(v))
        return reg.opaque<UrlImpl>(ctx, v)->href();
    if (JS_IsString(v)) {
        // UTF-16 单元转 UTF-8（孤立代理 → U+FFFD，与 WHATWG URL 编码一致）
        size_t len = 0;
        const uint16_t* units = JS_ToCStringLenUTF16(ctx, &len, v);
        std::string s = units ? utf16_to_utf8(units, len) : std::string{};
        if (units)
            JS_FreeCStringUTF16(ctx, units);
        return resolve_url(ctx, std::move(s));
    }
    throw_type_error(ctx, "Request: input 类型不支持");
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
                   .getter("headers", &RequestImpl::headers_value)
                   .getter("bodyUsed", [](qjs::This<RequestImpl> self) { return self->body_used; })
                   .getter("body", [](qjs::Ctx ctx, qjs::This<RequestImpl>) -> qjs::Value {
                       // v1 无 ReadableStream：body 恒为 null（规范允许 null 或流）
                       return qjs::Value(ctx.ctx, JS_NULL);
                   })
                   .getter("redirect", [](qjs::This<RequestImpl> self) { return self->redirect; })
                   .getter("integrity", [](qjs::This<RequestImpl> self) { return self->integrity; })
                   .getter("signal", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       if (self->signal_js.empty())
                           return qjs::Value(ctx.ctx, JS_UNDEFINED);
                       return qjs::Value(ctx.ctx, self->signal_js.dup(ctx.ctx));
                   })
                   .method("clone", [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> qjs::Value {
                       if (self->body_used)
                           throw_type_error(ctx.ctx, "Request: body 已被消费");
                       return qjs::Value(ctx.ctx, qjs::js_convert<RequestImpl>::to_js(ctx.ctx, *self));
                   })
                   .method("text", [](qjs::Ctx ctx, qjs::This<RequestImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Request", consume_text);
                   })
                   .method("json", [](qjs::Ctx ctx, qjs::This<RequestImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Request", consume_json);
                   })
                   .method("arrayBuffer", [](qjs::Ctx ctx, qjs::This<RequestImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Request", consume_array_buffer);
                   })
                   .method("bytes", [](qjs::Ctx ctx, qjs::This<RequestImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Request", consume_bytes);
                   })
                   .method("formData",
                           [](qjs::Ctx ctx, qjs::This<RequestImpl> self) -> exec::task<qjs::Value> {
                               const std::string ct = content_type_of(ctx.ctx, *self);
                               const std::string essence = mime_essence(ct);
                               // 规范：essence 不是 multipart/urlencoded → TypeError
                               if (essence != "multipart/form-data" &&
                                   essence != "application/x-www-form-urlencoded")
                                   throw_type_error(
                                       ctx.ctx,
                                       "Request: formData() 需要 multipart/form-data 或 "
                                       "application/x-www-form-urlencoded Content-Type");
                               const bool has_body = self->has_body;
                               co_return consume_impl(
                                   ctx.ctx, *self, "Request",
                                   [ct, essence, has_body](JSContext* ctx,
                                                           const std::string& bytes)
                                       -> qjs::Value {
                                       if (essence == "application/x-www-form-urlencoded") {
                                           // urlencoded → FormData（字符串值；无 body 空串 → 空 FormData）
                                           FormDataImpl fd;
                                           for (auto& kv : UrlSearchParamsImpl::from_query(bytes).list)
                                               fd.append_entry(std::move(kv.first),
                                                               std::move(kv.second), "", "", false);
                                           return qjs::Value(
                                               ctx, qjs::js_convert<FormDataImpl>::to_js(ctx, fd));
                                       }
                                       // multipart：body 为 null（无 body）→ TypeError
                                       //（fetch 规范 formData() 步骤：bodyBytes null → reject）
                                       if (!has_body)
                                           throw_type_error(ctx,
                                                            "Request: multipart/form-data 需要 body");
                                       auto fd = parse_multipart(bytes, extract_boundary(ct));
                                       if (!fd) // 解析失败 → TypeError（wpt invalidCases）
                                           throw_type_error(ctx,
                                                            "Request: multipart/form-data 解析失败");
                                       return qjs::Value(
                                           ctx, qjs::js_convert<FormDataImpl>::to_js(ctx, *fd));
                                   });
                           })
                   .method("blob", [](qjs::Ctx ctx, qjs::This<RequestImpl> self)
                               -> exec::task<qjs::Value> {
                       const std::string ct =
                           content_type_of(ctx.ctx, *self);
                       co_return consume_impl(
                           ctx.ctx, *self, "Request",
                           [ct](JSContext* c, const std::string& bytes) {
                               return consume_blob(c, bytes, ct);
                           });
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
    qjs::RtValue headers_js; // 缓存的 Headers JS 对象（同一对象语义）

    ResponseImpl() = default;
    // 拷贝：headers_js 缓存不复制（to_js/clone 场景）
    ResponseImpl(const ResponseImpl& o)
        : status(o.status), status_text(o.status_text), headers(o.headers),
          body_bytes(o.body_bytes), has_body(o.has_body), body_used(o.body_used),
          type(o.type), url(o.url), redirected(o.redirected)
    {
    }
    ResponseImpl& operator=(const ResponseImpl& o)
    {
        status = o.status;
        status_text = o.status_text;
        headers = o.headers;
        body_bytes = o.body_bytes;
        has_body = o.has_body;
        body_used = o.body_used;
        type = o.type;
        url = o.url;
        redirected = o.redirected;
        headers_js = qjs::RtValue();
        return *this;
    }

    void qjs_mark(JSRuntime* rt, JS_MarkFunc* mark_func) { headers_js.mark(rt, mark_func); }

    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> body, qjs::Opt<qjs::Value> init) {
        headers.set_guard(HeadersImpl::Guard::Response);
        if (init && init->is_object()) {
            qjs::Object obj(*init);
            qjs::Value status_v = obj.get("status");
            if (!status_v.is_undefined()) {
                const int s = status_v.as<int>();
                // 规范：状态码范围违规 → RangeError（wpt 要求 instanceof RangeError）
                if (s < 200 || s > 599)
                    throw_range_error(ctx, "Response: status 必须在 200-599");
                status = s;
            }
            qjs::Value st = obj.get("statusText");
            if (!st.is_undefined())
                status_text = st.as<std::string>();
            // 规范：statusText 必须 HTTP reason-phrase（ByteString：代码点 ≤ U+00FF；
            // CR/LF 禁；0x80-0xFF obs-text 允许）
            for (const char c : status_text)
                if (c == '\r' || c == '\n')
                    throw_type_error(ctx, "Response: statusText 含 CR/LF");
            {
                const std::string& s = status_text;
                for (auto it = s.begin(); it != s.end();) {
                    uint32_t cp = 0;
                    try {
                        cp = utf8::next(it, s.end());
                    } catch (...) {
                        throw_type_error(ctx, "Response: statusText 非 UTF-8");
                    }
                    if (cp > 0xFF)
                        throw_type_error(ctx, "Response: statusText 代码点超出 ByteString 范围");
                }
            }
            qjs::Value hdrs = obj.get("headers");
            if (!hdrs.is_undefined() && !hdrs.is_null()) {
                headers = headers_from(ctx, hdrs.raw());
                headers.set_guard(HeadersImpl::Guard::Response);
            }
        }
        if (body && !body->is_undefined() && !body->is_null()) {
            // 规范：204/205/304 带 body → TypeError
            if (status == 204 || status == 205 || status == 304)
                throw_type_error(ctx, "Response: 204/205/304 不能带 body");
            ExtractedBody b = extract_body(ctx, body->raw());
            body_bytes = std::move(b.bytes);
            has_body = b.has;
            if (!b.content_type.empty() && !headers.has(ctx, "Content-Type"))
                headers.append(ctx, "Content-Type", b.content_type); // 规范：Blob/URLSearchParams 自动设置
        }
        // 规范：204/205/304 无 body
        if (status == 204 || status == 205 || status == 304)
            has_body = false;
    }

    bool ok() const { return status >= 200 && status <= 299; }
};

// init.body 是 Request/Response 实例时复制其内部 body（bodyUsed → TypeError）
inline bool try_extract_init_body(JSContext* ctx, JSValueConst v, ExtractedBody& out) {
    auto& reg = qjs::registry_of(ctx);
    if (reg.is_registered<RequestImpl>() && reg.id_of<RequestImpl>(ctx) == JS_GetClassID(v)) {
        auto* r = reg.opaque<RequestImpl>(ctx, v);
        if (r->body_used)
            throw_type_error(ctx, "Request: body 已被消费");
        out.bytes = r->body_bytes;
        out.has = r->has_body;
        return true;
    }
    if (reg.is_registered<ResponseImpl>() && reg.id_of<ResponseImpl>(ctx) == JS_GetClassID(v)) {
        auto* r = reg.opaque<ResponseImpl>(ctx, v);
        if (r->body_used)
            throw_type_error(ctx, "Response: body 已被消费");
        out.bytes = r->body_bytes;
        out.has = r->has_body;
        return true;
    }
    if (try_blob_bytes(ctx, v, out.bytes)) { // Blob/File 作 init
        out.has = true;
        if (is_blob_instance(ctx, v))
            out.content_type = reg.opaque<BlobImpl>(ctx, v)->type;
        else if (is_file_instance(ctx, v))
            out.content_type = reg.opaque<FileImpl>(ctx, v)->blob.type;
        return true;
    }
    return false;
}

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
                       if (self->headers_js.empty())
                           self->headers_js = qjs::RtValue(
                               JS_GetRuntime(ctx.ctx),
                               qjs::js_convert<HeadersImpl>::to_js(ctx.ctx, self->headers));
                       return qjs::Value(ctx.ctx, self->headers_js.dup(ctx.ctx));
                   })
                   .getter("bodyUsed", [](qjs::This<ResponseImpl> self) { return self->body_used; })
                   .getter("body", [](qjs::Ctx ctx, qjs::This<ResponseImpl>) -> qjs::Value {
                       // v1 无 ReadableStream：body 恒为 null（204/205/304/HEAD 也是 null）
                       return qjs::Value(ctx.ctx, JS_NULL);
                   })
                   .method("clone", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> qjs::Value {
                       if (self->body_used)
                           throw_type_error(ctx.ctx, "Response: body 已被消费");
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, *self));
                   })
                   .method("text", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Response", consume_text);
                   })
                   .method("json", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Response", consume_json);
                   })
                   .method("arrayBuffer", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Response", consume_array_buffer);
                   })
                   .method("bytes", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self)
                               -> exec::task<qjs::Value> {
                       co_return consume_impl(ctx.ctx, *self, "Response", consume_bytes);
                   })
                   .method("formData",
                           [](qjs::Ctx ctx, qjs::This<ResponseImpl> self) -> exec::task<qjs::Value> {
                               const std::string ct = content_type_of(ctx.ctx, *self);
                               const std::string essence = mime_essence(ct);
                               if (essence != "multipart/form-data" &&
                                   essence != "application/x-www-form-urlencoded")
                                   throw_type_error(
                                       ctx.ctx,
                                       "Response: formData() 需要 multipart/form-data 或 "
                                       "application/x-www-form-urlencoded Content-Type");
                               const bool has_body = self->has_body;
                               co_return consume_impl(
                                   ctx.ctx, *self, "Response",
                                   [ct, essence, has_body](JSContext* ctx,
                                                           const std::string& bytes)
                                       -> qjs::Value {
                                       if (essence == "application/x-www-form-urlencoded") {
                                           FormDataImpl fd;
                                           for (auto& kv : UrlSearchParamsImpl::from_query(bytes).list)
                                               fd.append_entry(std::move(kv.first),
                                                               std::move(kv.second), "", "", false);
                                           return qjs::Value(
                                               ctx, qjs::js_convert<FormDataImpl>::to_js(ctx, fd));
                                       }
                                       if (!has_body)
                                           throw_type_error(
                                               ctx, "Response: multipart/form-data 需要 body");
                                       auto fd = parse_multipart(bytes, extract_boundary(ct));
                                       if (!fd) // 解析失败 → TypeError（wpt invalidCases）
                                           throw_type_error(
                                               ctx, "Response: multipart/form-data 解析失败");
                                       return qjs::Value(
                                           ctx, qjs::js_convert<FormDataImpl>::to_js(ctx, *fd));
                                   });
                           })
                   .method("blob", [](qjs::Ctx ctx, qjs::This<ResponseImpl> self)
                               -> exec::task<qjs::Value> {
                       const std::string ct =
                           content_type_of(ctx.ctx, *self);
                       co_return consume_impl(
                           ctx.ctx, *self, "Response",
                           [ct](JSContext* c, const std::string& bytes) {
                               return consume_blob(c, bytes, ct);
                           });
                   })
                   .static_method("error", [](qjs::Ctx ctx) -> qjs::Value {
                       ResponseImpl r;
                       r.status = 0;
                       r.type = "error";
                       r.headers.set_guard(HeadersImpl::Guard::Immutable); // 规范：error 响应 headers 不可变
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, r));
                   })
                   .static_method("redirect", [](qjs::Ctx ctx, const std::string& url,
                                                 qjs::Opt<int> status) -> qjs::Value {
                       const int s = status ? *status : 302;
                       if (s != 301 && s != 302 && s != 303 && s != 307 && s != 308)
                           throw_range_error(ctx.ctx, "Response: redirect 状态非法");
                       ResponseImpl r;
                       r.status = s;
                       r.type = "default";
                       r.headers.append_raw("location", RequestImpl::resolve_url(ctx.ctx, url));
                       return qjs::Value(ctx.ctx, qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, r));
                   })
                   .static_method("json", [](qjs::Ctx ctx, qjs::Value data,
                                             qjs::Opt<qjs::Value> init) -> qjs::Value {
                       // 规范：bytes = JSON serialize(data)（BigInt/循环引用 → TypeError）
                       JSValue s = JS_JSONStringify(ctx.ctx, data.raw(), JS_UNDEFINED, JS_UNDEFINED);
                       if (JS_IsException(s))
                           throw qjs::js_error(ctx.ctx, JS_GetException(ctx.ctx));
                       std::string json;
                       {
                           size_t len = 0;
                           const char* str = JS_ToCStringLen(ctx.ctx, &len, s);
                           json.assign(str, len);
                           JS_FreeCString(ctx.ctx, str);
                       }
                       JS_FreeValue(ctx.ctx, s);
                       // 规范：仅当 init.headers 未显式给 content-type 时补 application/json
                       //（string body 的默认 text/plain 需被替换）
                       bool user_ct = false;
                       if (init && init->is_object()) {
                           qjs::Object obj(*init);
                           qjs::Value hdrs = obj.get("headers");
                           if (!hdrs.is_undefined() && !hdrs.is_null()) {
                               HeadersImpl h = headers_from(ctx.ctx, hdrs.raw());
                               user_ct = h.has(ctx.ctx, "content-type");
                           }
                       }
                       qjs::Opt<qjs::Value> body;
                       body.value.emplace(ctx.ctx,
                                          JS_NewStringLen(ctx.ctx, json.data(), json.size()));
                       ResponseImpl r;
                       r.qjs_init(ctx.ctx, body, init);
                       if (!user_ct)
                           r.headers.set(ctx.ctx, "content-type", "application/json");
                       return qjs::Value(ctx.ctx,
                                        qjs::js_convert<ResponseImpl>::to_js(ctx.ctx, r));
                   });
    ctx.globals().set("Response", cls.constructor_function());
    // forEach 的 container 参数包装（headers 实例；活迭代器补丁见 install_headers）
    ctx.eval(
        "var __f0 = Headers.prototype.forEach;"
        "Headers.prototype.forEach = function (cb, thisArg) {"
        "  var self = this;"
        "  __f0.call(this, function (value, key) { cb.call(thisArg, value, key, self); }, thisArg);"
        "};");
}

} // namespace qjsbind::web
