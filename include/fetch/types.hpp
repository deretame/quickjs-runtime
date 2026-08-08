// fetchcore —— 核心值类型与纯 C++ 工具（header-only）
//
// 本头是 fetchcore 的类型底座：Header/Headers/Request/Response/Options 均为
// 纯值类型，不含 JS guard/forbidden 检查与规范化逻辑（那些是 JS 规范语义，
// 留在绑定层）。同时收纳原 qjsbind::web 层的头操作、重定向判定、data: URL
// 与 SRI 摘要工具（纯 C++，随策略下沉迁入）。
//
// 依赖方向：本头只依赖 fetch/task.hpp + 标准库 + OpenSSL EVP；禁止 include
// 任何 quickjs/qjsbind 头。
#pragma once

#include <fetch/task.hpp>
#include <fetch/error.hpp>

#include <openssl/evp.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fetch {

struct BodySource; // 前向声明（body.hpp 定义；shared_ptr 允许不完整类型）

struct Header {
    std::string name;
    std::string value;
};

// 保序、允许重复名；大小写不敏感查找工具随库提供（has_header 等）。
using Headers = std::vector<Header>;

// TLS 选项（原 src/net TlsOptions 迁入）
struct TlsOptions {
    bool verify = true;                       // 默认校验证书（嵌入的 Mozilla CA bundle）
    std::vector<std::string> extra_trust_pem; // 追加信任的 PEM（测试/自签用）
};

struct Request {
    std::string method = "GET";
    std::string url;            // 绝对 http/https/data URL（相对 URL 解析是绑定层职责）
    Headers headers;
    std::string body;           // 整收；流式上传不做（duplex 非目标）
    std::string integrity;      // SRI 表达式，空 = 不校验
    enum class Redirect { follow, error, manual } redirect = Redirect::follow;
};

struct Response {
    int status = 0;
    std::string reason;
    Headers headers;
    std::string url;            // 最终 URL（重定向后）
    bool redirected = false;
    std::shared_ptr<BodySource> body; // null = 无 body（HEAD/204/205/304）；拉模型，64 KiB 块
};

struct Options {
    TlsOptions tls{};            // 默认 BeastTransport 的 TLS 配置（注入自定义 Transport 时忽略）
    bool auto_decompress = true; // 内建 Accept-Encoding 中间件开关（固定最外层）
    int max_redirects = 20;
    // 自动解压的总字节上限（gzip bomb 防护；security review MEDIUM）。
    // 0 = 无限制；默认 256 MiB——单块上限 1 MiB 控峰值，此值控总量。
    size_t max_decompressed_bytes = 256 * 1024 * 1024;
};

// ---- 头操作工具（原 interceptor.hpp 迁入）----

// 大小写不敏感比较
inline bool header_name_eq(std::string_view a, std::string_view b)
{
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](char x, char y) { return (x | 32) == (y | 32); });
}

inline bool has_header(const Headers& headers, std::string_view name)
{
    for (const auto& h : headers)
        if (header_name_eq(h.name, name))
            return true;
    return false;
}

inline void strip_headers(Headers& headers, std::initializer_list<const char*> names)
{
    headers.erase(std::remove_if(headers.begin(), headers.end(), [&](const Header& h) {
                      for (const char* n : names)
                          if (header_name_eq(h.name, n))
                              return true;
                      return false;
                  }),
                  headers.end());
}

// 取单一 content-encoding（逗号多个 → nullopt；identity/无 → nullopt）
inline std::optional<std::string> single_content_encoding(const Headers& headers)
{
    for (const auto& h : headers) {
        if (header_name_eq(h.name, "content-encoding")) {
            const std::string& v = h.value;
            if (v.find(',') != std::string::npos)
                return std::nullopt; // 多编码：不处理透传（避免歧义）
            std::string enc = v;
            enc.erase(std::remove_if(enc.begin(), enc.end(), [](char c) { return c == ' '; }),
                      enc.end());
            std::transform(enc.begin(), enc.end(), enc.begin(),
                           [](char c) { return static_cast<char>(c | 32); });
            if (enc.empty() || enc == "identity")
                return std::nullopt;
            return enc;
        }
    }
    return std::nullopt;
}

// ---- 重定向判定工具（原 fetch_detail 迁入）----

inline bool is_redirect_status(int status)
{
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

inline bool status_requires_get(int status, const std::string& method)
{
    // 303：仅非 GET/HEAD 转 GET；301/302：仅 POST 转 GET
    if (status == 303)
        return method != "GET" && method != "HEAD";
    if ((status == 301 || status == 302) && method == "POST")
        return true;
    return false;
}

// 从响应头取 Location（大小写不敏感；无 → 空串）
inline std::string location_of(const Headers& headers)
{
    for (const auto& h : headers)
        if (header_name_eq(h.name, "location"))
            return h.value;
    return {};
}

inline Headers without_body_headers(const Headers& headers)
{
    Headers out;
    for (const auto& h : headers) {
        // 重定向转 GET/HEAD 后剥离全部 body 相关头（fetch 规范；wpt redirect-method）
        if (!header_name_eq(h.name, "content-length") &&
            !header_name_eq(h.name, "content-type") &&
            !header_name_eq(h.name, "content-encoding") &&
            !header_name_eq(h.name, "content-language") &&
            !header_name_eq(h.name, "content-location"))
            out.push_back(h);
    }
    return out;
}

// ---- data: URL（WHATWG data URL processor，fetch 规范 §data URL）----

inline std::string percent_decode(const std::string& in)
{
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
inline std::optional<std::string> base64_decode(const std::string& in)
{
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
                           std::string& body_out)
{
    const size_t comma = url.find(',');
    if (comma == std::string::npos)
        return false; // 无逗号 → 解析失败
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
            return false; // base64 非法 → 解析失败
        body_out = std::move(*decoded);
    } else {
        body_out = percent_decode(data);
    }
    return true;
}

// ---- SRI（Subresource Integrity）----
inline std::string sha_digest(const std::string& algo, const std::string& data)
{
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

inline std::string base64_encode(const std::string& in)
{
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
inline bool digest_matches(const std::string& expected, const std::string& actual)
{
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

// 校验响应体摘要（SRI）。不匹配/无法校验 → fetch::Error。
// 注：body 为空串调用仅用于"null body"立即校验（status/method 判定）。
inline void check_integrity(const std::string& integrity, int status,
                            const std::string& method, const std::string& body)
{
    if (integrity.empty())
        return;
    // 规范：null body（null body status 或 HEAD）无法校验 → 网络错误
    const bool null_body = status == 204 || status == 205 || status == 304 || method == "HEAD";
    if (null_body)
        throw Error("fetch: integrity 无法校验 null body 响应");
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
        throw Error("fetch: integrity 校验失败");
}

} // namespace fetch
