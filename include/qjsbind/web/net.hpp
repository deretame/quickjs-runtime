// qjsbind::web —— fetch 网络层的数据类型与后端抽象（header-only）
//
// 约定：网络后端（默认是 src/net/ 的 boost::beast + OpenSSL 实现）实现 FetchBackend，
//       由调用方（main/tests）在 install_web_apis 时注入，绑定层不直接依赖网络库。
#pragma once

#include <exec/task.hpp>
#include <stop_token>
#include <string>
#include <vector>

namespace qjsbind::web {

struct Header {
    std::string name;
    std::string value;
};

struct HttpRequest {
    std::string method;           // "GET" / "POST" / ...
    std::string url;              // http(s)://host[:port]/path?query
    std::vector<Header> headers;  // 追加请求头（Host/Accept 由后端自动补）
    std::string body;
};

struct HttpResponse {
    int status = 0;
    std::string reason;
    std::vector<Header> headers;
    std::string body;
};

// 网络后端抽象。实现必须可跨线程安全调用（stop_token 可能在其他线程触发）。
struct FetchBackend {
    virtual ~FetchBackend() = default;
    // 单次请求（不含重定向）。失败抛 std::exception（网络/TLS/协议错误）。
    virtual exec::task<HttpResponse> request(HttpRequest req, std::stop_token st) = 0;
};

} // namespace qjsbind::web
