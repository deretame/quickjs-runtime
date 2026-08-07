// SOCKS5 代理：握手 / 隧道建立（设计文档 §3.4）
#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <qjsbind/std_exec.hpp>
#include <stdexec/execution.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace qjsbind::net {

struct Socks5Proxy {
    std::string host;                    // 代理地址（主机名或 IP）
    uint16_t port = 1080;                // 代理端口
    std::optional<std::pair<std::string, std::string>> auth; // RFC 1929 user-pass
};

// 建立经 SOCKS5 到目标的 TCP 隧道：greeting（方法协商）→（可选）RFC 1929 子协商
// → CONNECT（ATYP = 域名 / IPv4 / IPv6）→ 校验 REP=0x00。
// 失败抛 boost::system::system_error（含 REP 错误码，category=socks5）。
// 目标地址：IP 字面量 → 对应 ATYP；域名 → ATYP=0x03 交给代理解析。
// 取消：stop_token 注册期间 socket cancel()（握手阶段也可被取消 → stopped）。
// 注：返回 shared_ptr（而非设计文档草稿的 tcp::socket 值）——与 BeastBodySource /
//    stop_callback 的 shared_ptr 生命周期模型一致（隧道 socket 移交 body 阶段继续用）。
std_exec::task<std::shared_ptr<boost::asio::ip::tcp::socket>>
socks5_connect(boost::asio::io_context& io, const Socks5Proxy& proxy,
               std::string_view target_host, uint16_t target_port, std::stop_token st);

} // namespace qjsbind::net
