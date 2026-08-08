// fetchcore —— BeastTransport 声明（boost::beast + OpenSSL 的 HTTP/HTTPS 传输）
//
// 设计：单次请求（不含重定向），协程化（std_exec::task）。v2 流式：
//   - request() 读出头即返回；body 由 BeastBodySource（fetch::BodySource）流式交出，
//     读取路径：read() → http::async_read_some（64 KiB 块），EOF 由
//     response_parser<buffer_body> 的 is_done() 判定（chunked / content-length /
//     need_eof 连接关闭终止统一由 beast 处理）。
//   - 取消语义：
//     * 头前阶段：stop_token 触发 → socket.cancel() → asio 异步操作以
//       operation_aborted 完成 → exec::asio::use_sender 转为 set_stopped
//       → 整个 task 以 stopped 完成 → 绑定层 reject AbortError
//     * 读 body 阶段：stop_callback 挂在 BeastBodySource 上（构造时注册、
//       析构时注销），触发 cancel() → lowest_layer().close() → 挂起的 read()
//       以 stopped 完成 → 流读取 reject AbortError
//   - 网络/协议/TLS 错误（头前）→ 协程抛出 boost::system::system_error；
//     读 body 中途失败 → read() 抛异常（fetch 已 resolve）
//
// 证书：默认信任嵌入的 Mozilla CA bundle（cacert_embedded.hpp，脚本生成）；
//       可通过 TlsOptions::extra_trust_pem 追加信任的 PEM（本地自签测试用）。
#pragma once

#include <fetch/task.hpp>
#include <fetch/transport.hpp>
#include <fetch/types.hpp>

#include <boost/asio/io_context.hpp>

namespace fetch {

class BeastTransport : public Transport {
public:
    explicit BeastTransport(boost::asio::io_context& io, TlsOptions tls = {})
        : io_(io), tls_(std::move(tls)) {}
    ~BeastTransport() override = default;

    std_exec::task<Response> request(const Request& req, std::stop_token st) override;

    // 经 SOCKS5 隧道交换（https 在隧道上照常 TLS handshake）
    std_exec::task<Response> request_via_socks5(const Request& req, const Socks5Proxy& proxy,
                                                std::stop_token st) override;

private:
    boost::asio::io_context& io_;
    TlsOptions tls_;
};

} // namespace fetch
