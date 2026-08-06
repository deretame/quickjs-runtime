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
#include <qjsbind/web/errors.hpp>
#include <qjsbind/web/utf8.hpp>

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

// method-override 头（值含 forbidden method 时整体忽略，见 forbidden()）
inline bool is_method_override_header(const std::string& lower_name) {
    return lower_name == "x-http-method-override" || lower_name == "x-http-method" ||
           lower_name == "x-method-override";
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
    enum class Guard { None, Request, RequestNoCors, Response, Immutable };
    Guard guard = Guard::None;

    // list of (lowercase name, value)：保序 + 同名多值（set-cookie 语义）；
    // get 合并；迭代 sort+combine（stable：同名保插入序）。
    std::vector<std::pair<std::string, std::string>> list;

    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> init); // 定义见 headers_from 之后

    void set_guard(Guard g) { guard = g; }

    // 规范校验：name 必须全 token 字符；value 不能含 CR/LF/NUL（已 trim）
    static std::string normalize_name(JSContext* ctx, const std::string& raw) {
        const std::string name = trim_http_ws(raw);
        if (name.empty() || name.size() > 128)
            throw_type_error(ctx, "Headers: name 非法");
        for (const char c : name)
            if (!is_http_token_char(c))
                throw_type_error(ctx, "Headers: name 含非法字符");
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lower;
    }
    static std::string normalize_value(JSContext* ctx, const std::string& raw) {
        // 首尾 HTTP 空白（SP/TAB/CR/LF，obs-fold 语义）去掉；中间残留 CR/LF/NUL → TypeError
        size_t b = 0, e = raw.size();
        auto is_ws = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        while (b < e && is_ws(raw[b]))
            ++b;
        while (e > b && is_ws(raw[e - 1]))
            --e;
        const std::string value = raw.substr(b, e - b);
        for (const char c : value)
            if (c == '\r' || c == '\n' || c == '\0')
                throw_type_error(ctx, "Headers: value 含非法字符");
        // ByteString 转换检查：UTF-8 代码点 > U+00FF → TypeError（wpt headers-errors）
        for (auto it = value.begin(); it != value.end();) {
            uint32_t cp = 0;
            try {
                cp = utf8::next(it, value.end());
            } catch (...) {
                throw_type_error(ctx, "Headers: value 非 UTF-8");
            }
            if (cp > 0xFF)
                throw_type_error(ctx, "Headers: value 代码点超出 ByteString 范围");
        }
        return value;
    }

    // fetch 规范：guard=request/request-no-cors/response 下，append/set/delete
    // 对 forbidden 头静默忽略（不存储、不抛错）；get/has 不检查 guard。
    // method-override 头（x-http-method 等）的值按逗号分列、trim、小写后
    // 任一项是 forbidden method（trace/track/connect）→ 同样忽略。
    bool forbidden(JSContext* ctx, const std::string& lower_name, const std::string& value) const {
        if (guard == Guard::Request && is_forbidden_request_header(lower_name))
            return true;
        if (guard == Guard::Request && is_method_override_header(lower_name)) {
            std::string part;
            auto check_part = [&] {
                size_t b = 0, e = part.size();
                while (b < e && (part[b] == ' ' || part[b] == '\t'))
                    ++b;
                while (e > b && (part[e - 1] == ' ' || part[e - 1] == '\t'))
                    --e;
                std::string p = part.substr(b, e - b);
                std::transform(p.begin(), p.end(), p.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return p == "trace" || p == "track" || p == "connect";
            };
            for (char c : value) {
                if (c == ',') {
                    if (check_part())
                        return true;
                    part.clear();
                } else {
                    part.push_back(c);
                }
            }
            if (check_part())
                return true;
        }
        if (guard == Guard::RequestNoCors && lower_name != "accept" &&
            lower_name != "accept-language" && lower_name != "content-language" &&
            lower_name != "content-type")
            return true;
        if (guard == Guard::Response && is_forbidden_response_header(lower_name))
            return true;
        return false;
    }

    void append(JSContext* ctx, const std::string& name_raw, const std::string& value_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        const std::string value = normalize_value(ctx, value_raw);
        // 规范：仅 immutable guard 完全不可变（抛 TypeError）；response guard 下
        // forbidden 响应头（set-cookie）静默忽略（forbidden 检查），其他可改
        if (guard == Guard::Immutable)
            throw_type_error(ctx, "Headers: guard=immutable 不可修改");
        if (forbidden(ctx, name, value))
            return; // 静默忽略
        list.push_back({name, value});
    }
    void set(JSContext* ctx, const std::string& name_raw, const std::string& value_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        const std::string value = normalize_value(ctx, value_raw);
        if (guard == Guard::Immutable)
            throw_type_error(ctx, "Headers: guard=immutable 不可修改");
        if (forbidden(ctx, name, value))
            return; // 静默忽略
        erase_all(name);
        list.push_back({name, value});
    }
    void erase(JSContext* ctx, const std::string& name_raw) {
        const std::string name = normalize_name(ctx, name_raw);
        if (guard == Guard::Immutable)
            throw_type_error(ctx, "Headers: guard=immutable 不可修改");
        if (forbidden(ctx, name, ""))
            return; // 静默忽略
        erase_all(name);
    }
    bool has(JSContext* ctx, const std::string& name_raw) const {
        const std::string name = normalize_name(ctx, name_raw);
        // 规范：response guard 下 forbidden 响应头（set-cookie）→ false
        if (guard == Guard::Response && is_forbidden_response_header(name))
            return false;
        for (const auto& [k, v] : list)
            if (k == name)
                return true;
        return false;
    }
    std::optional<std::string> get(JSContext* ctx, const std::string& name_raw) const {
        const std::string name = normalize_name(ctx, name_raw);
        // 规范：response guard 下 forbidden 响应头（set-cookie）→ null
        if (guard == Guard::Response && is_forbidden_response_header(name))
            return std::nullopt;
        std::string out;
        bool first = true;
        for (const auto& [k, v] : list) {
            if (k == name) {
                if (!first)
                    out += ", ";
                out += v;
                first = false;
            }
        }
        return first ? std::nullopt : std::optional<std::string>(std::move(out));
    }
    // 内部直存（fetch 响应组装用，绕过 guard 检查；name 仍需小写化）
    void append_raw(const std::string& name, const std::string& value) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        list.push_back({std::move(lower), value});
    }
    // getSetCookie()：set-cookie 值数组（不合并）
    std::vector<std::string> get_set_cookie() const {
        std::vector<std::string> out;
        for (const auto& [k, v] : list)
            if (k == "set-cookie")
                out.push_back(v);
        return out;
    }
    // 有序键值对（迭代用）：按 name 排序 + 同名合并（", " 连接）；
    // set-cookie 例外：不合并、保持插入序（fetch 规范，wpt header-setcookie）
    std::vector<std::pair<std::string, std::string>> sorted_entries() const {
        std::vector<std::pair<std::string, std::string>> out = list;
        std::stable_sort(out.begin(), out.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
        std::vector<std::pair<std::string, std::string>> merged;
        for (const auto& p : out) {
            if (p.first == "set-cookie") {
                merged.push_back(p);
            } else if (merged.empty() || merged.back().first != p.first) {
                merged.push_back(p);
            } else {
                merged.back().second += ", " + p.second;
            }
        }
        return merged;
    }

private:
    void erase_all(const std::string& name)
    {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&](const auto& p) { return p.first == name; }),
                   list.end());
    }
};

// JS init 参数 → HeadersImpl（undefined / Headers 实例 / record / 序列 of pairs）
inline HeadersImpl headers_from(JSContext* ctx, JSValueConst init) {
    HeadersImpl out;
    if (JS_IsUndefined(init))
        return out;
    // 规范：null 也抛 TypeError（仅 undefined 表示空）
    if (JS_IsNull(init))
        throw_type_error(ctx, "Headers: init 不能为 null");
    // 已注册的 HeadersImpl 实例 → 拷贝
    if (JS_IsObject(init)) {
        auto& reg = qjs::registry_of(ctx);
        if (reg.is_registered<HeadersImpl>() && reg.id_of<HeadersImpl>(ctx) == JS_GetClassID(init)) {
            return *reg.opaque<HeadersImpl>(ctx, init);
        }
    }
    if (JS_IsArray(init)) {
        // init 是借用值：dup 后交由 arr 接管（否则调用方析构 double-free）
        qjs::Array arr(ctx, JS_DupValue(ctx, init));
        for (std::size_t i = 0; i < arr.length(); ++i) {
            qjs::Value item = arr.get(i);
            if (!JS_IsArray(item.raw())) {
                throw_type_error(ctx, "Headers: 序列项不是 [name, value] 数组");
            }
            qjs::Array pair(std::move(item)); // 移动接管 item 的唯一引用
            // 规范：每项必须是 [name, value] 两元素数组；元素经 ToString 转 ByteString
            //（null → "null" 等；非 ASCII 代码点由 normalize_value 的 ByteString 检查拦截）
            if (pair.length() != 2)
                throw_type_error(ctx, "Headers: 序列项必须是 [name, value] 二元组");
            qjs::Value k = pair.get(0);
            qjs::Value v = pair.get(1);
            out.append(ctx, k.as<std::string>(), v.as<std::string>());
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
    throw_type_error(ctx, "Headers: 不支持的 init 参数");
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
                   .method("getSetCookie", [](qjs::Ctx ctx, qjs::This<HeadersImpl> self) -> qjs::Value {
                       const auto vals = self->get_set_cookie();
                       qjs::Array arr(ctx.ctx, JS_NewArray(ctx.ctx));
                       std::size_t i = 0;
                       for (const auto& v : vals)
                           arr.set(i++, qjs::Value(ctx.ctx, JS_NewString(ctx.ctx, v.c_str())));
                       return qjs::Value(std::move(arr));
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
    // Symbol.iterator（vcpkg quickjs.h 无公共 atom 常量，JS 侧补丁最稳）
    ctx.eval(
        "Headers.prototype[Symbol.iterator] = function* () { yield* this.entries(); };"
        "Headers.prototype[Symbol.toStringTag] = 'Headers';");
}

} // namespace qjsbind::web
