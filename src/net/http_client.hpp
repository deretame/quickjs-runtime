// qjsbind_net —— boost::beast + OpenSSL 的 HTTP/HTTPS 客户端（静态库，非 header-only）
//
// 设计：单次请求（不含重定向），协程化（exec::task）。取消语义：
//   - stop_token 触发 → socket.cancel() → asio 异步操作以 operation_aborted 完成
//     → exec::asio::use_sender 转为 set_stopped → 整个 task 以 stopped 完成
//     → promise_from_sender reject AbortError（fetch 层已接好）
//   - 网络/协议/TLS 错误 → 协程抛出 boost::system::system_error
//
// 证书：默认信任嵌入的 Mozilla CA bundle（cacert_embedded.hpp，脚本生成）；
//       可通过 TlsOptions::extra_trust_pem 追加信任的 PEM（本地自签测试用）。
#pragma once

#include <exec/task.hpp>
#include <stop_token>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>

namespace qjsbind::net {

struct Header {
    std::string name;
    std::string value;
};

struct HttpRequest {
    std::string method;               // "GET" / "POST" / ...
    std::string url;                  // http(s)://host[:port]/path?query
    std::vector<Header> headers;      // 追加请求头（Host/Accept 等由实现自动补）
    std::string body;
};

struct HttpResponse {
    int status = 0;
    std::string reason;
    std::vector<Header> headers;
    std::string body;
};

struct TlsOptions {
    bool verify = true;                        // 默认校验证书（嵌入的 Mozilla CA bundle）
    std::vector<std::string> extra_trust_pem;  // 追加信任的 PEM（测试用）
};

// 执行一次 HTTP 请求。失败时抛 boost::system::system_error（含 TLS/解析/网络错误）。
exec::task<HttpResponse> http_request(boost::asio::io_context& io, HttpRequest req,
                                      TlsOptions tls, std::stop_token st);

} // namespace qjsbind::net
