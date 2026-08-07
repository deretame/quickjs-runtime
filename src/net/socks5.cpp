// SOCKS5 握手实现（RFC 1928 + RFC 1929 子协商）
#include "socks5.hpp"
#include <qjsbind/std_exec.hpp>

#include <boost/asio.hpp>
#include <exec/asio/use_sender.hpp>

#include <functional>
#include <stdexcept>

namespace qjsbind::net {

namespace {

// REP → error_code（category=socks5，错误信息可读）
struct Socks5Category : boost::system::error_category {
    const char* name() const noexcept override { return "socks5"; }
    std::string message(int ev) const override
    {
        switch (ev) {
        case 0x00: return "succeeded";
        case 0x01: return "general failure";
        case 0x02: return "connection not allowed by ruleset";
        case 0x03: return "network unreachable";
        case 0x04: return "host unreachable";
        case 0x05: return "connection refused";
        case 0x06: return "TTL expired";
        case 0x07: return "command not supported";
        case 0x08: return "address type not supported";
        default: return "unknown SOCKS5 reply";
        }
    }
};
const Socks5Category& socks5_category()
{
    static const Socks5Category c;
    return c;
}

[[noreturn]] void throw_protocol(const char* what)
{
    throw boost::system::system_error(
        boost::system::error_code(boost::system::errc::protocol_error,
                                  boost::system::generic_category()),
        what);
}

// greeting：无认证 {05,01,00}；有 auth 只声明 0x02（不允许降级为无认证）
std::string build_greeting(const Socks5Proxy& p)
{
    return p.auth ? std::string("\x05\x01\x02", 3) : std::string("\x05\x01\x00", 3);
}

// RFC 1929：{01, ulen, user, plen, pass}
std::string build_userpass(const Socks5Proxy& p)
{
    const auto& [user, pass] = *p.auth;
    std::string s;
    s.push_back('\x01');
    s.push_back(static_cast<char>(std::min<size_t>(user.size(), 255)));
    s += user.substr(0, 255);
    s.push_back(static_cast<char>(std::min<size_t>(pass.size(), 255)));
    s += pass.substr(0, 255);
    return s;
}

// 目标地址 → {ATYP, ADDR}：IP 字面量按 0x01/0x04，其余按 0x03 域名（代理解析）
std::string build_target(std::string_view host)
{
    boost::system::error_code ec;
    if (auto v4 = boost::asio::ip::make_address_v4(host, ec); !ec) {
        auto b = v4.to_bytes();
        std::string s("\x01", 1);
        s.append(reinterpret_cast<const char*>(b.data()), b.size());
        return s;
    }
    if (auto v6 = boost::asio::ip::make_address_v6(host, ec); !ec) {
        auto b = v6.to_bytes();
        std::string s("\x04", 1);
        s.append(reinterpret_cast<const char*>(b.data()), b.size());
        return s;
    }
    std::string s("\x03", 1);
    s.push_back(static_cast<char>(std::min<size_t>(host.size(), 255)));
    s += host.substr(0, 255);
    return s;
}

} // namespace

std_exec::task<std::shared_ptr<boost::asio::ip::tcp::socket>>
socks5_connect(boost::asio::io_context& io, const Socks5Proxy& proxy,
               std::string_view target_host, uint16_t target_port, std::stop_token st)
{
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    auto use_sender = exec::asio::use_sender;

    // 1. 连接代理
    tcp::resolver resolver(io);
    const auto results = co_await resolver.async_resolve(proxy.host, std::to_string(proxy.port),
                                                         use_sender);
    auto sock = std::make_shared<tcp::socket>(io);
    std::optional<std::stop_callback<std::function<void()>>> stop_cb;
    if (st.stop_possible())
        stop_cb.emplace(st, [sock] {
            boost::system::error_code ec;
            sock->cancel(ec); // operation_aborted → use_sender 转 stopped
        });
    co_await sock->async_connect(*results.begin(), use_sender);

    // 2. greeting：方法协商
    const std::string greeting = build_greeting(proxy);
    co_await net::async_write(*sock, net::buffer(greeting), use_sender);
    char gr[2];
    co_await net::async_read(*sock, net::buffer(gr, 2), use_sender);
    if (gr[0] != 0x05)
        throw_protocol("socks5: bad version from server");
    const uint8_t method = static_cast<uint8_t>(gr[1]);
    if (method == 0xFF)
        throw boost::system::system_error(
            boost::system::error_code(boost::system::errc::permission_denied,
                                      boost::system::generic_category()),
            "socks5: no acceptable authentication method");
    if (proxy.auth) {
        if (method != 0x02)
            throw boost::system::system_error(
                boost::system::error_code(boost::system::errc::permission_denied,
                                          boost::system::generic_category()),
                "socks5: server rejected user-pass auth");
        // 3. RFC 1929 子协商
        const std::string up = build_userpass(proxy);
        co_await net::async_write(*sock, net::buffer(up), use_sender);
        char ur[2];
        co_await net::async_read(*sock, net::buffer(ur, 2), use_sender);
        if (ur[0] != 0x01 || ur[1] != 0x00)
            throw boost::system::system_error(
                boost::system::error_code(boost::system::errc::permission_denied,
                                          boost::system::generic_category()),
                "socks5: user-pass authentication failed");
    } else if (method != 0x00) {
        throw_protocol("socks5: server chose an undeclared method");
    }

    // 4. CONNECT（注意：不能用含 \x00 的 C 字符串字面量 += —— strlen 会截断）
    std::string conn;
    conn.reserve(7 + target_host.size());
    conn.push_back(0x05);
    conn.push_back(0x01);
    conn.push_back(0x00);
    conn += build_target(target_host);
    conn.push_back(static_cast<char>(target_port >> 8));
    conn.push_back(static_cast<char>(target_port & 0xFF));
    co_await net::async_write(*sock, net::buffer(conn), use_sender);
    char rh[4];
    co_await net::async_read(*sock, net::buffer(rh, 4), use_sender);
    if (rh[0] != 0x05 || rh[2] != 0x00)
        throw_protocol("socks5: malformed CONNECT reply");
    if (rh[1] != 0x00)
        throw boost::system::system_error(
            boost::system::error_code(rh[1], socks5_category()),
            "socks5: CONNECT rejected");
    // BND.ADDR（按 ATYP 读变长）
    size_t bnd_len = 0;
    switch (rh[3]) {
    case 0x01: bnd_len = 4; break;
    case 0x04: bnd_len = 16; break;
    case 0x03: {
        char len = 0;
        co_await net::async_read(*sock, net::buffer(&len, 1), use_sender);
        bnd_len = static_cast<uint8_t>(len);
        break;
    }
    default:
        throw boost::system::system_error(
            boost::system::error_code(0x08, socks5_category()),
            "socks5: unsupported BND.ADDR type");
    }
    std::string bnd(bnd_len + 2, 0);
    co_await net::async_read(*sock, net::buffer(bnd.data(), bnd.size()), use_sender);

    co_return sock;
}

} // namespace qjsbind::net
