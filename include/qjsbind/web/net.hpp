// qjsbind::web —— fetch 网络层的数据类型与后端抽象（header-only）
//
// 约定：网络后端（默认是 src/net/ 的 boost::beast + OpenSSL 实现）实现 FetchBackend，
//       由调用方（main/tests）在 install_web_apis 时注入，绑定层不直接依赖网络库。
//
// v2 流式化：request() 读出头即返回，body 以 BodySource 流交出（拉模型，
// 见 docs/fetch_streaming_design.md §3）。读取路径上的取消由 BodySource::cancel()
// 承担（命令式、可能跨线程）；请求头之前的失败仍由 request() 抛异常。
#pragma once

#include <stdexec/execution.hpp>
#include <qjsbind/std_exec.hpp>
#include <stop_token>
#include <memory>
#include <optional>
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

// 响应体字节源（取代 v1 的整收 std::string body）。
// read()：返回一块字节（可任意大小）；nullopt = EOF。失败抛 std::exception
// （网络/协议/解压错误，读取路径统一转 TypeError）。
// cancel()：尽力取消（关闭 socket、释放资源），幂等，可能在其他线程触发。
struct BodySource {
    virtual ~BodySource() = default;
    virtual std_exec::task<std::optional<std::string>> read() = 0;
    virtual void cancel() = 0;
};

struct HttpResponse {
    int status = 0;
    std::string reason;
    std::vector<Header> headers;
    std::shared_ptr<BodySource> body; // null = 无 body（HEAD/204/205/304）
};

// 网络后端抽象。实现必须可跨线程安全调用（stop_token 可能在其他线程触发）。
struct FetchBackend {
    virtual ~FetchBackend() = default;
    // 单次请求（不含重定向）。读出头即返回，body 尚未读完（由 resp.body 流提供）。
    // 头前失败（DNS/连接/TLS/写请求/读头）抛 std::exception。
    // req 为 const 引用：调用方（拦截器链）需要保留本跳最终请求供后置相位读取。
    virtual std_exec::task<HttpResponse> request(const HttpRequest& req, std::stop_token st) = 0;
};

} // namespace qjsbind::web
