// fetchcore —— Client：注入 io_context 的 fetch 核心入口
//
// 管线（每跳重走全链，跳间不共享状态）：
//   data: URL 本地构造 → [Accept-Encoding(最外层) + 用户 use() 中间件 + 传输]
//   → redirect 循环（follow/error/manual）→ SRI 包装 → Response
//
// 生命周期与线程契约（fetch_cpp_decoupling.md §4.2，成文化）：
//   1. Client 不拥有 io；io 必须比 Client 及其在飞请求活得久。
//   2. io 为单线程驱动、无 strand：Client 只能由跑 io.run() 的那根线程使用；
//      唯一跨线程入口是 std::stop_token 触发的 cancel()（只碰 socket）。
//   3. Client 无全局状态、无 TLS 依赖；可在任意作用域构造，多实例共存、
//      多实例可共用一个 io（各自独立的中间件/TLS/代理配置互不干扰）。
//   4. 唯一进程级共享是内嵌 CA X509_STORE（实现细节，不构成实例间耦合）。
#pragma once

#include <fetch/task.hpp>
#include <fetch/body.hpp>
#include <fetch/types.hpp>
#include <fetch/transport.hpp>
#include <fetch/middleware.hpp>
#include <fetch/error.hpp>
#include <fetch/url_check.hpp>
#include <fetch/beast_transport.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cctype>

namespace fetch {

// 相对 Location 以当前 URL 为 base 解析（boost::urls；失败抛 fetch::Error）。
// 与绑定层 UrlImpl::parse 对齐：boost 严格语法拒绝的字符（非 ASCII、
// 裸 % 等 WHATWG 允许的）→ 宽松 percent-encode 重试（wpt
// redirect-location-escape 覆盖原始 UTF-8 字节的 Location）；
// scheme/host 小写化 + 默认端口剥离（WHATWG 归一；否则大写 host 或
// 显式 :80/:443 的 Location 会破坏 resp.url 一致性——review should-fix）。
inline std::string resolve_url(const std::string& loc, const std::string& base)
{
    auto normalize_url = [](boost::urls::url& u) {
        // WHATWG 归一（v1 UrlImpl::parse 对齐）：scheme/host 小写化 +
        // 默认端口剥离。host 用 set_encoded_host（已编码语义）——实测
        // set_host 会把 %41 二次编码为 %2541（未编码语义），encoded 版保留。
        if (u.has_scheme()) {
            std::string s(u.scheme());
            for (auto& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            u.set_scheme(s);
        }
        std::string host(u.encoded_host());
        if (!host.empty() && host.find(':') == std::string::npos) {
            for (auto& c : host)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            u.set_encoded_host(host); // %XX 保留；hex 字母小写化语义不变
        }
        // 默认端口剥离（WHATWG 默认端口表 http:80/https:443/ws:80/wss:443），
        // 数值比较（前导零 080 == 80）
        if (u.has_port() && u.has_scheme()) {
            const std::string s(u.scheme());
            bool known = s == "http" || s == "https" || s == "ws" || s == "wss";
            if (known) {
                std::string p(u.port());
                while (p.size() > 1 && p[0] == '0')
                    p.erase(p.begin());
                const std::string def = (s == "https" || s == "wss") ? "443" : "80";
                if (p == def)
                    u.remove_port();
            }
        }
    };

    auto relax = [](std::string_view url) {
        size_t auth_end = 0;
        const size_t scheme_end = url.find("://");
        if (scheme_end != std::string_view::npos) {
            const size_t path_start = url.find_first_of("/?#", scheme_end + 3);
            auth_end = path_start == std::string_view::npos ? url.size() : path_start;
        }
        const char* hexd = "0123456789ABCDEF";
        auto is_safe = [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                   c == '-' || c == '.' || c == '_' || c == '~' || c == '!' || c == '$' ||
                   c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' || c == '+' ||
                   c == ',' || c == ';' || c == '=' || c == ':' || c == '@' || c == '/' ||
                   c == '?' || c == '#' || c == '[' || c == ']';
        };
        auto is_hex = [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        };
        std::string out;
        for (size_t i = 0; i < url.size(); ++i) {
            const unsigned char c = url[i];
            const bool pct_ok = c == '%' && i + 2 < url.size() && is_hex(url[i + 1]) &&
                                is_hex(url[i + 2]);
            if (i < auth_end || is_safe(c) || pct_ok) {
                out.push_back(static_cast<char>(c));
            } else {
                out.push_back('%');
                out.push_back(hexd[c >> 4]);
                out.push_back(hexd[c & 0xF]);
            }
        }
        return out;
    };
    // relaxed 存储提升到函数级：parse_uri_reference 返回的 url_view 借用
    // 输入字符串，必须存活到 resolve 完成（否则悬垂——boost assert 崩）。
    std::string relaxed, base_relaxed;
    auto r = boost::urls::parse_uri_reference(loc);
    if (r.has_error()) {
        relaxed = relax(loc);
        r = boost::urls::parse_uri_reference(relaxed);
        if (r.has_error())
            throw Error("fetch: Location 无法解析");
    }
    if ((*r).has_scheme()) {
        boost::urls::url u(*r);
        normalize_url(u);
        return std::string(u.buffer());
    }
    auto rb = boost::urls::parse_uri_reference(base);
    if (rb.has_error()) {
        base_relaxed = relax(base);
        rb = boost::urls::parse_uri_reference(base_relaxed);
        if (rb.has_error())
            throw Error("fetch: base URL 无法解析");
    }
    boost::urls::url u(*rb);
    auto res = u.resolve(*r); // 原地解析（boost 1.91：返回 result<void> 仅作错误检查）
    if (res.has_error())
        throw Error("fetch: 相对解析失败");
    normalize_url(u);
    return std::string(u.buffer());
}

class Client {
public:
    // 默认 BeastTransport（TLS 配置取 Options::tls）
    explicit Client(boost::asio::io_context& io, Options opt = {})
        : io_(io), opt_(std::move(opt)),
          transport_(std::make_shared<BeastTransport>(io_, opt_.tls))
    {
    }

    // 注入自定义 Transport（TLS/SOCKS5 等由调用方在 transport 上配置；
    // 此时 Options::tls 被忽略）
    Client(boost::asio::io_context& io, std::shared_ptr<Transport> transport,
           Options opt = {})
        : io_(io), opt_(std::move(opt)), transport_(std::move(transport))
    {
    }

    // 注册 C++ 中间件（先注册者在最外层；内建 Accept-Encoding 固定最外层，
    // SOCKS5 选路中间件命中时直接走传输隧道、与位置无关）。
    Client& use(std::shared_ptr<Middleware> mw)
    {
        mws_.push_back(std::move(mw));
        return *this;
    }

    // 主入口：redirect 循环 + SRI + data: + 中间件链 + 传输。
    std_exec::task<Response> fetch(Request req, std::stop_token st = {});

private:
    // 单跳：组装一次链（Accept-Encoding 最外层 + 用户中间件 + 传输）并走链。
    std_exec::task<Response> fetch_once(const Request& req, std::stop_token st);

    boost::asio::io_context& io_;
    Options opt_;
    std::shared_ptr<Transport> transport_;
    std::vector<std::shared_ptr<Middleware>> mws_;
};

inline std_exec::task<Response> Client::fetch_once(const Request& req, std::stop_token st)
{
    check_url_ports(req.url); // 公开 API 也检查（security review LOW：防 C++ 调用方绕过）
    Handler h = make_chain(mws_, transport_);
    if (opt_.auto_decompress) {
        auto ae =
            std::make_shared<AcceptEncodingMiddleware>(opt_.max_decompressed_bytes);
        h = wrap_middleware(ae, std::move(h));
    }
    co_return co_await h(req, std::move(st));
}

inline std_exec::task<Response> Client::fetch(Request req, std::stop_token st)
{
    // ---- data: URL：本地构造响应（Node 行为：data URL 可 fetch，type=basic）----
    if (req.url.rfind("data:", 0) == 0) {
        std::string mime, data;
        if (!parse_data_url(req.url, mime, data))
            throw Error("fetch: data URL 解析失败");
        if (req.method == "HEAD")
            data.clear(); // HEAD 响应无 body（wpt scheme-data）
        Response r;
        r.status = 200;
        r.reason = "OK";
        if (!data.empty() || req.method != "HEAD")
            r.body = std::make_shared<BytesBodySource>(std::move(data));
        r.url = req.url;
        r.headers.push_back({"Content-Type", mime});
        co_return r;
    }

    std::string method = req.method;
    std::string url = req.url;
    Headers headers = req.headers;
    std::string body = req.body;
    const int kMaxRedirects = opt_.max_redirects;
    for (int hop = 0; hop <= kMaxRedirects; ++hop) {
        check_url_ports(url); // 每跳检查（含初始 URL）：blocked 端口 → 抛 Error
        Request rq;
        rq.method = method;
        rq.url = url;
        rq.headers = headers;
        rq.body = body;

        Response resp = co_await fetch_once(rq, st); // 每跳重走全链

        const std::string loc = location_of(resp.headers);
        if (req.redirect == Request::Redirect::error && is_redirect_status(resp.status))
            throw Error("fetch: redirect mode 为 error");
        if (req.redirect == Request::Redirect::manual && is_redirect_status(resp.status)) {
            // opaqueredirect 哨兵：status==0 且 url 空（绑定层据此构造 opaqueredirect）
            Response r;
            co_return r;
        }
        if (req.redirect == Request::Redirect::follow && is_redirect_status(resp.status) &&
            !loc.empty()) {
            if (hop == kMaxRedirects)
                throw Error("fetch: 重定向次数超过 " + std::to_string(kMaxRedirects));
            url = resolve_url(loc, url); // 相对 Location 以当前 URL 为 base 解析
            // 303 一律转 GET；301/302 仅 POST 转 GET
            if (status_requires_get(resp.status, method)) {
                method = "GET";
                body.clear();
                headers = without_body_headers(headers);
            }
            continue;
        }

        // SRI（M3）：消费末端增量校验——integrity 非空 → 包 IntegritySource
        //（read 时算摘要，EOF 比对；不匹配 → 消费抛异常 → 绑定层 reject TypeError）。
        // 空 body（204/205/304/HEAD）仍走立即校验（v1 语义：null body + integrity → 错误）。
        if (!req.integrity.empty() && resp.body) {
            resp.body = std::make_shared<IntegritySource>(std::move(resp.body), req.integrity);
        } else if (!req.integrity.empty()) {
            check_integrity(req.integrity, resp.status, method, ""); // null body 检查
        }
        resp.url = url;             // 最终 URL（重定向后）
        resp.redirected = hop > 0;  // 规范：经重定向的响应 redirected=true
        co_return resp;
    }
    throw Error("fetch: 重定向次数超过 " + std::to_string(kMaxRedirects));
}

} // namespace fetch
