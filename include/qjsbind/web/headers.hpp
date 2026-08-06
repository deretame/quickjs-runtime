// qjsbind::web —— Headers（fetch 规范语义子集）
//
// v1 边界：
//   - 存储：lowercase name → 合并值（append 以 ", " 连接），保持规范语义；
//   - guard：none/request/request-no-cors/response；request 系做 forbidden 头检查；
//   - 构造：Headers 实例 / record / 序列 of pairs / undefined；
//   - name 非法字符、value 含 CR/LF → TypeError（headers 规范校验）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/value.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qjsbind::web {

// fetch 规范 forbidden request-header name（request / request-no-cors guard）
inline bool is_forbidden_request_header(const std::string& lower_name) {
    static const char* const kForbidden[] = {
        "accept-charset", "accept-encoding", "access-control-request-headers",
        "access-control-request-method", "connection", "content-length", "cookie",
        "cookie2", "date", "dnt", "expect", "host", "keep-alive", "origin", "referer",
        "te", "trailer", "transfer-encoding", "upgrade", "via",
    };
    for (const auto* f : kForbidden)
        if (lower_name == f)
            return true;
    return lower_name.starts_with("proxy-") || lower_name.starts_with("sec-");
}

inline bool is_forbidden_response_header(const std::string& lower_name) {
    return lower_name == "set-cookie" || lower_name == "set-cookie2";
}

// HTTP token 字符（name 校验）与 field-value 校验（禁 CR/LF/NUL）
inline bool is_http_token_char(char c) {
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;
    if (c >= '0' && c <= '9')
        return true;
    static const char* kExtra = "!#$%&'*+-.^_`|~";
    for (const char* p = kExtra; *p; ++p)
        if (c == *p)
            return true;
    return false;
}

inline std::string trim_http_ws(std::string_view s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t'))
        ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t'))
        --e;
    return std::string(s.substr(b, e - b));
}

struct HeadersImpl {
    enum class Guard { None, Request, RequestNoCors, Response };
    Guard guard = Guard::None;

    std::map<std::string, std::string> list; // lowercase name → 合并值

    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> init); // 定义见 headers_from 之后

    void set_guard(Guard g) { guard = g; }

    // 规范校验：name 必须全 token 字符；value 不能含 CR/LF/NUL（已 trim）
    static std::string normalize_name(JSContext* ctx, const std::string& raw) {
        const std::string name = trim_http_ws(raw);
        if (name.empty() || name.size() > 128)
            JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: name 非法"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        for (const char c : name)
            if (!is_http_token_char(c))
                JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: name 含非法字符"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lower;
    }
    static std::string normalize_value(JSContext* ctx, const std::string& raw) {
        const std::string value = trim_http_ws(raw);
        for (const char c : value)
            if (c == '\r' || c == '\n' || c == '\0')
                JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: value 含非法字符"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
        return value;
    }

    void check_forbidden(JSContext* ctx, const std::string& lower_name) const {
        if (guard == Guard::Request || guard == Guard::RequestNoCors) {
            if (guard == Guard::Request && is_forbidden_request_header(lower_name))
                throw qjs::js_error(ctx,
                                    JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: %s 是 forbidden 请求头",
                                                      lower_name.c_str())));
            if (guard == Guard::RequestNoCors && lower_name != "accept" &&
                lower_name != "accept-language" && lower_name != "content-language" &&
                lower_name != "content-type")
                throw qjs::js_error(ctx,
                                    JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: %s 在 no-cors 下 forbidden",
                                                      lower_name.c_str())));
        }
        if (guard == Guard::Response && is_forbidden_response_header(lower_name))
            throw qjs::js_error(ctx,
                                JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: %s 是 forbidden 响应头",
                                                  lower_name.c_str())));
    }

    void append(JSContext* ctx, const std::string& name_raw, const std::string& value_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        const std::string value = normalize_value(ctx, value_raw);
        check_forbidden(ctx, name);
        auto it = list.find(name);
        if (it != list.end())
            it->second += ", " + value;
        else
            list.emplace(name, value);
    }
    void set(JSContext* ctx, const std::string& name_raw, const std::string& value_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        const std::string value = normalize_value(ctx, value_raw);
        check_forbidden(ctx, name);
        list[name] = value;
    }
    void erase(JSContext* ctx, const std::string& name_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        check_forbidden(ctx, name);
        list.erase(name);
    }
    bool has(JSContext* ctx, const std::string& name_raw) const {
        const std::string name = normalize_name(ctx, name_raw);
        check_forbidden(ctx, name);
        return list.count(name) != 0;
    }
    std::optional<std::string> get(JSContext* ctx, const std::string& name_raw) const {
        const std::string name = normalize_name(ctx, name_raw);
        check_forbidden(ctx, name);
        const auto it = list.find(name);
        return it == list.end() ? std::nullopt : std::optional<std::string>(it->second);
    }
    // 有序键值对（迭代用）
    std::vector<std::pair<std::string, std::string>> sorted_entries() const {
        std::vector<std::pair<std::string, std::string>> out;
        out.reserve(list.size());
        for (const auto& [k, v] : list)
            out.push_back({k, v});
        return out;
    }
};

// JS init 参数 → HeadersImpl（undefined / Headers 实例 / record / 序列 of pairs）
inline HeadersImpl headers_from(JSContext* ctx, JSValueConst init) {
    HeadersImpl out;
    if (JS_IsUndefined(init) || JS_IsNull(init))
        return out;
    // 已注册的 HeadersImpl 实例 → 拷贝
    if (JS_IsObject(init)) {
        auto& reg = qjs::registry_of(ctx);
        if (reg.is_registered<HeadersImpl>() && reg.id_of<HeadersImpl>(ctx) == JS_GetClassID(init)) {
            return *reg.opaque<HeadersImpl>(ctx, init);
        }
    }
    if (JS_IsArray(init)) {
        qjs::Array arr(ctx, init);
        for (std::size_t i = 0; i < arr.length(); ++i) {
            qjs::Value item = arr.get(i);
            if (!JS_IsArray(item.raw())) {
                JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: 序列项不是 [name, value] 数组"));
                throw qjs::js_error(ctx, JS_GetException(ctx));
            }
            qjs::Array pair(ctx, item.raw());
            out.append(ctx, pair.get(0).as<std::string>(), pair.get(1).as<std::string>());
        }
        return out;
    }
    if (JS_IsObject(init)) {
        JSPropertyEnum* props = nullptr;
        uint32_t nprops = 0;
        if (JS_GetOwnPropertyNames(ctx, &props, &nprops, init,
                                   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
            throw qjs::js_error(ctx, JS_GetException(ctx));
        for (uint32_t i = 0; i < nprops; ++i) {
            qjs::Value key(ctx, JS_AtomToString(ctx, props[i].atom));
            qjs::Value val(ctx, JS_GetProperty(ctx, init, props[i].atom));
            out.append(ctx, key.as<std::string>(), val.as<std::string>());
        }
        js_free(ctx, props);
        return out;
    }
    JS_Throw(ctx, JS_NewTypeError(ctx, "Headers: 不支持的 init 参数"));
    throw qjs::js_error(ctx, JS_GetException(ctx));
}

// qjs_init 类外定义（依赖 headers_from）
inline void HeadersImpl::qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> init) {
    if (init)
        *this = headers_from(ctx, init->raw());
}

// 键值对数组 → JS 数组（entries/keys/values 的 v1 返回形态，同 URLSearchParams）
inline qjs::Value header_entries_to_js(JSContext* ctx,
                                       const std::vector<std::pair<std::string, std::string>>& list,
                                       bool keys_only) {
    qjs::Array arr(ctx, JS_NewArray(ctx));
    std::size_t idx = 0;
    for (const auto& [k, v] : list) {
        if (keys_only) {
            arr.set(idx++, qjs::Value(ctx, JS_NewString(ctx, k.c_str())));
        } else {
            qjs::Array pair(ctx, JS_NewArray(ctx));
            pair.set(0, qjs::Value(ctx, JS_NewString(ctx, k.c_str())));
            pair.set(1, qjs::Value(ctx, JS_NewString(ctx, v.c_str())));
            arr.set(idx++, qjs::Value(std::move(pair)));
        }
    }
    return qjs::Value(std::move(arr));
}

inline void install_headers(qjs::Context& ctx) {
    auto cls = qjs::class_<HeadersImpl>(ctx, "Headers")
                   .constructor<qjs::Opt<qjs::Value>>()
                   .method("append",
                           [](qjs::Ctx ctx, qjs::This<HeadersImpl> self, const std::string& name,
                              const std::string& value) { self->append(ctx.ctx, name, value); })
                   .method("set", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self, const std::string& name,
                                     const std::string& value) { self->set(ctx.ctx, name, value); })
                   .method("delete",
                           [](qjs::Ctx ctx, qjs::This<HeadersImpl> self, const std::string& name) {
                               self->erase(ctx.ctx, name);
                           })
                   .method("has", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self,
                                     const std::string& name) { return self->has(ctx.ctx, name); })
                   .method("get", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self,
                                     const std::string& name) -> qjs::Value {
                       const auto v = self->get(ctx.ctx, name);
                       return v ? qjs::Value(ctx.ctx, JS_NewString(ctx.ctx, v->c_str()))
                                : qjs::Value(ctx.ctx, JS_NULL);
                   })
                   .method("entries",
                           [](qjs::Ctx ctx, qjs::This<HeadersImpl> self) -> qjs::Value {
                               return header_entries_to_js(ctx.ctx, self->sorted_entries(), false);
                           })
                   .method("keys", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self) -> qjs::Value {
                       return header_entries_to_js(ctx.ctx, self->sorted_entries(), true);
                   })
                   .method("values", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self) -> qjs::Value {
                       std::vector<std::string> vals;
                       for (const auto& [k, v] : self->sorted_entries())
                           vals.push_back(v);
                       qjs::Array arr(ctx.ctx, JS_NewArray(ctx.ctx));
                       std::size_t i = 0;
                       for (const auto& v : vals)
                           arr.set(i++, qjs::Value(ctx.ctx, JS_NewString(ctx.ctx, v.c_str())));
                       return qjs::Value(std::move(arr));
                   })
                   .method("forEach",
                           [](qjs::Ctx ctx, qjs::This<HeadersImpl> self, qjs::Function cb,
                              qjs::Opt<qjs::Value> this_arg) {
                               for (const auto& [k, v] : self->sorted_entries()) {
                                   JSValue args[3] = {JS_NewString(ctx.ctx, v.c_str()),
                                                      JS_NewString(ctx.ctx, k.c_str()),
                                                      JS_DupValue(ctx.ctx, this_arg ? this_arg->raw()
                                                                                   : JS_UNDEFINED)};
                                   qjs::Value r = cb.call_raw(3, args);
                                   JS_FreeValue(ctx.ctx, args[0]);
                                   JS_FreeValue(ctx.ctx, args[1]);
                                   JS_FreeValue(ctx.ctx, args[2]);
                                   if (r.is_exception())
                                       throw qjs::js_error(ctx.ctx, JS_GetException(ctx.ctx));
                               }
                           });
    ctx.globals().set("Headers", cls.constructor_function());
}

} // namespace qjsbind::web
