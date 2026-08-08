// fetchcore —— asio io_context 的 stdexec scheduler 适配
//
// 从 qjsbind/context.hpp 的 io_context_scheduler 下沉（fetch_cpp_decoupling.md
// §4.5/§7-8：核心库自带一份，绑定层复用本版本，避免两份实现漂移）。
// schedule() = post(ioc, exec::asio::use_sender)。
#pragma once

#include <boost/asio/io_context.hpp>
#include <exec/asio/use_sender.hpp>
#include <stdexec/execution.hpp>

namespace fetch {

class io_scheduler {
public:
    using scheduler_concept = stdexec::scheduler_tag;
    explicit io_scheduler(boost::asio::io_context& ioc) noexcept : ioc_(&ioc) {}

    stdexec::sender auto schedule() const noexcept
    {
        return exec::asio::asio_impl::post(*ioc_, exec::asio::use_sender);
    }

    bool operator==(const io_scheduler&) const noexcept = default;

private:
    boost::asio::io_context* ioc_;
};

} // namespace fetch
