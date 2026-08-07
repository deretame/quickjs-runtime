// qjsbind_net —— beast + OpenSSL 的 FetchBackend 实现（静态库）
#pragma once

#include "http_client.hpp"
#include <qjsbind/std_exec.hpp>

#include <boost/asio/io_context.hpp>

namespace qjsbind::net {

class BeastFetchBackend : public web::FetchBackend {
public:
    explicit BeastFetchBackend(boost::asio::io_context& io, TlsOptions tls = {})
        : io_(io), tls_(std::move(tls)) {}
    ~BeastFetchBackend() override = default;

    std_exec::task<web::HttpResponse> request(const web::HttpRequest& req,
                                          std::stop_token st) override;

    // 经 SOCKS5 隧道交换（设计文档 §5.7：代理拦截器作为"另一条 handler"调用；
    // 与 request() 共用类型桥接，https 在隧道上照常 TLS handshake）
    std_exec::task<web::HttpResponse> request_via_socks5(const web::HttpRequest& req,
                                                     const Socks5Proxy& proxy,
                                                     std::stop_token st);

private:
    boost::asio::io_context& io_;
    TlsOptions tls_;
};

} // namespace qjsbind::net
