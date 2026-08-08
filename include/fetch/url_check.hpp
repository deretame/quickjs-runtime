// fetchcore —— URL 端口检查（fetch 规范 #port-blocking）
//
// 安全职责（security review sa_20260808_173002 MEDIUM 1）：blocked-port
// 清单与检查从绑定层下沉到核心层，fetch() 入口与 redirect 每跳都执行，
// 防止恶意 Location 把请求带向 127.0.0.1:22 等内网敏感端口（SSRF 端口绕过）。
#pragma once

#include <fetch/error.hpp>

#include <boost/url/parse.hpp>

#include <string>

namespace fetch {

// fetch 规范 #port-blocking 清单（与浏览器一致）
inline bool is_blocked_port(int port)
{
    static const int kBad[] = {
        0,   1,   7,   9,   11,  13,  15,  17,  19,  20,  21,  22,  23,  25,  37,  42, 43,
        53,  69,  77,  79,  87,  95,  101, 102, 103, 104, 109, 110, 111, 113, 115, 117, 119,
        123, 135, 137, 139, 143, 161, 179, 389, 427, 465, 512, 513, 514, 515, 526, 530, 531,
        532, 540, 548, 554, 556, 563, 587, 601, 636, 989, 990, 993, 995, 1719, 1720, 1723,
        2049, 3659, 4045, 4190, 5060, 5061, 6000, 6566, 6665, 6666, 6667, 6668, 6669, 6679,
        6697, 10080,
    };
    for (int b : kBad)
        if (port == b)
            return true;
    return false;
}

// 解析 URL 端口；被禁止 → 返回端口号，否则 -1（解析失败/无端口/非法端口 → -1）
inline int blocked_port_of(const std::string& url)
{
    auto r = boost::urls::parse_uri_reference(url);
    if (r.has_error() || !r->has_port())
        return -1;
    try {
        const int p = std::stoi(std::string(r->port()));
        return is_blocked_port(p) ? p : -1;
    } catch (const std::invalid_argument&) {
        return -1;
    }
}

// 检查并抛 fetch::Error（供核心层 fetch() 入口与 redirect 每跳调用）
inline void check_url_ports(const std::string& url)
{
    const int p = blocked_port_of(url);
    if (p > 0)
        throw Error("fetch: URL 端口 " + std::to_string(p) + " 被禁止");
}

} // namespace fetch
