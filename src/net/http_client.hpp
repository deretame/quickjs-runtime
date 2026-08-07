// qjsbind_net —— boost::beast + OpenSSL 的 HTTP/HTTPS 客户端（静态库，非 header-only）
//
// 设计：单次请求（不含重定向），协程化（exec::task）。v2 流式：
//   - request() 读出头即返回；body 由 BeastBodySource（web::BodySource）流式交出，
//     读取路径：read() → http::async_read_some（64 KiB 块），EOF 由
//     response_parser<buffer_body> 的 is_done() 判定（chunked / content-length /
//     need_eof 连接关闭终止统一由 beast 处理）。
//   - 取消语义：
//     * 头前阶段：stop_token 触发 → socket.cancel() → asio 异步操作以
//       operation_aborted 完成 → exec::asio::use_sender 转为 set_stopped
//       → 整个 task 以 stopped 完成 → promise_from_sender reject AbortError
//     * 读 body 阶段：stop_callback 挂在 BeastBodySource 上（构造时注册、
//       析构时注销），触发 cancel() → lowest_layer().close() → 挂起的 read()
//       以 stopped 完成 → 流读取 reject AbortError
//   - 网络/协议/TLS 错误（头前）→ 协程抛出 boost::system::system_error；
//     读 body 中途失败 → read() 抛异常（fetch 已 resolve，见设计文档 §3.3）
//
// 证书：默认信任嵌入的 Mozilla CA bundle（cacert_embedded.hpp，脚本生成）；
//       可通过 TlsOptions::extra_trust_pem 追加信任的 PEM（本地自签测试用）。
#pragma once

#include <qjsbind/web/net.hpp>

#include "socks5.hpp"

#include <exec/task.hpp>
#include <stop_token>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>

#include <optional>

namespace qjsbind::net {

// 与 web 层同名同构的请求描述（net 层独立类型，桥接见 http_backend.cpp）
struct HttpRequest {
    std::string method;               // "GET" / "POST" / ...
    std::string url;                  // http(s)://host[:port]/path?query
    std::vector<web::Header> headers; // 追加请求头（Host/Accept 等由实现自动补）
    std::string body;
};

struct HttpResponse {
    int status = 0;
    std::string reason;
    std::vector<web::Header> headers;
    std::shared_ptr<web::BodySource> body; // null = 无 body
};

struct TlsOptions {
    bool verify = true;                        // 默认校验证书（嵌入的 Mozilla CA bundle）
    std::vector<std::string> extra_trust_pem;  // 追加信任的 PEM（测试用）
};

// 执行一次 HTTP 请求：读出头即返回，body 由响应流提供。
// 头前失败抛 boost::system::system_error（含 TLS/解析/网络错误）。
// proxy 非空 → 经 SOCKS5 隧道交换（https 在隧道上照常 TLS handshake，§3.4）。
exec::task<HttpResponse> http_request(boost::asio::io_context& io, HttpRequest req,
                                      TlsOptions tls, std::stop_token st,
                                      std::optional<Socks5Proxy> proxy = std::nullopt);

} // namespace qjsbind::net
