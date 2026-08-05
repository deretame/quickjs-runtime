// dcb::sleep —— 基于 stdexec exec 扩展命名空间的定时实现。
//
// 设计见 docs/exec_timer_sleep.md。
//
// 提供：
//   dcb::sleep(dur)          默认后端：exec::schedule_after + 全局 timed_thread_context
//   dcb::thread_sleep(dur)   对照后端：std::thread（每 sleep 一个线程）
//   dcb::asio_sleep(io, dur) 对照后端：boost::asio::steady_timer（自包 sender）
//
// Windows 线程池后端（exec::windows_thread_pool）无需包装，直接：
//   exec::schedule_after(pool.get_scheduler(), dur)
//
// 注意：
// - 所有后端要求其执行上下文（timed_thread_context / io_context / pool）活得比
//   sleep 操作久。
// - thread_sleep / asio_sleep 在"协程挂起中销毁 opstate"的取消路径上行为未定义
//   （std::thread 版本会 join 等待；asio 版本 cancel 后 handler 可能异步触发）。

#pragma once

#include <exec/timed_thread_scheduler.hpp>
#include <stdexec/execution.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace dcb {

namespace detail {

// 默认后端的全局上下文（函数级静态，C++11 线程安全初始化）。
inline exec::timed_thread_context& default_timer_context()
{
  static exec::timed_thread_context ctx;
  return ctx;
}

// ---------------------------------------------------------------------------
// std::thread 后端（每 sleep 一个线程）
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
struct thread_sleep_sender {
  using sender_concept = stdexec::sender_tag;
  using completion_signatures = stdexec::completion_signatures<stdexec::set_value_t()>;

  std::chrono::duration<Rep, Period> dur;

  template <typename R>
  struct Op {
    using operation_state_concept = stdexec::operation_state_tag;
    R rcvr;
    std::chrono::duration<Rep, Period> dur;
    std::thread th;

    Op(R r, std::chrono::duration<Rep, Period> d) : rcvr(std::move(r)), dur(d) {}
    Op(Op&&) = delete;
    ~Op()
    {
      if (th.joinable()) th.join();
    }

    void start() & noexcept
    {
      th = std::thread([this] {
        std::this_thread::sleep_for(dur);
        stdexec::set_value(std::move(rcvr));
      });
    }
  };

  template <typename R>
  Op<R> connect(R&& r) &&
  {
    return Op<R>{std::forward<R>(r), dur};
  }
};

// ---------------------------------------------------------------------------
// boost::asio steady_timer 后端（自包 sender，见 docs §5）
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
struct asio_timer_sender {
  using sender_concept = stdexec::sender_tag;
  using completion_signatures = stdexec::completion_signatures<
    stdexec::set_value_t(),
    stdexec::set_error_t(std::exception_ptr),
    stdexec::set_stopped_t()>;

  boost::asio::io_context& io;
  std::chrono::duration<Rep, Period> dur;

  template <typename R>
  struct Op {
    using operation_state_concept = stdexec::operation_state_tag;
    R rcvr;
    boost::asio::steady_timer timer;
    std::chrono::duration<Rep, Period> dur;
    std::atomic<bool> done{false};

    Op(R r, boost::asio::io_context& io, std::chrono::duration<Rep, Period> d)
      : rcvr(std::move(r)), timer(io), dur(d)
    {}

    void complete(const boost::system::error_code& ec) noexcept
    {
      if (done.exchange(true)) return;
      if (ec == boost::asio::error::operation_aborted) {
        stdexec::set_stopped(std::move(rcvr));
      } else if (ec) {
        stdexec::set_error(std::move(rcvr),
                           std::make_exception_ptr(boost::system::system_error(ec)));
      } else {
        stdexec::set_value(std::move(rcvr));
      }
    }

    void start() & noexcept
    {
      timer.expires_after(dur);
      timer.async_wait([this](const boost::system::error_code& ec) { complete(ec); });
    }
  };

  template <typename R>
  Op<R> connect(R&& r) &&
  {
    return Op<R>{std::forward<R>(r), io, dur};
  }
};

}  // namespace detail

// ---------------------------------------------------------------------------
// 默认后端：exec::schedule_after + 全局 timed_thread_context
// 与 stream.hpp 中的前向声明一致。
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
stdexec::sender auto sleep(std::chrono::duration<Rep, Period> dur)
{
  return exec::schedule_after(detail::default_timer_context().get_scheduler(), dur);
}

// ---------------------------------------------------------------------------
// 对照后端：std::thread（每 sleep 一个线程）
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
stdexec::sender auto thread_sleep(std::chrono::duration<Rep, Period> dur)
{
  return detail::thread_sleep_sender<Rep, Period>{dur};
}

// ---------------------------------------------------------------------------
// 对照后端：boost::asio::steady_timer
// 调用方需保证 io_context 正在被某个线程 run()。
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
stdexec::sender auto asio_sleep(boost::asio::io_context& io,
                                std::chrono::duration<Rep, Period> dur)
{
  return detail::asio_timer_sender<Rep, Period>{io, dur};
}

}  // namespace dcb
