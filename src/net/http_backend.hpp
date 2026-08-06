// qjsbind_net —— beast + OpenSSL 的 FetchBackend 实现（静态库）
#pragma once

#include <qjsbind/web/net.hpp>

#include <boost/asio/io_context.hpp>

namespace qjsbind::net {

class BeastFetchBackend : public web::FetchBackend {
public:
    explicit BeastFetchBackend(boost::asio::io_context& io) : io_(io) {}
    ~BeastFetchBackend() override = default;

    exec::task<web::HttpResponse> request(web::HttpRequest req, std::stop_token st) override;

private:
    boost::asio::io_context& io_;
};

} // namespace qjsbind::net
