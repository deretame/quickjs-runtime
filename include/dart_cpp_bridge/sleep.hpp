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
// - thread_sleep / asio_sleep 两个对照后端在"协程挂起中销毁 opstate"（取消/提前
//   销毁）时是安全降级：receiver 的完成回调不会再被调用（取消即放弃交付），
//   不存在悬垂回调访问已销毁帧的问题（receiver 由共享控制块持有，析构互斥）。

#pragma once

#include <exec/timed_thread_scheduler.hpp>
#include <stdexec/execution.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
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

// 共享控制块：把 receiver 的完整生命周期与 opstate 解耦。
// opstate 提前析构（取消路径）时置 done 并销毁 receiver；后台完成回调
// （线程/io 线程）持 shared_ptr 到达时发现 done 即静默返回，绝不触碰
// 已销毁的帧或 receiver。
template <typename R>
struct shared_control_block {
  std::mutex mu;
  bool done = false;
  std::optional<R> rcvr;
};

// 取出 receiver（若仍有效）。调用方持有控制块。
template <typename R>
std::optional<R> take_receiver(shared_control_block<R>& ctrl)
{
  std::lock_guard<std::mutex> lk(ctrl.mu);
  if (ctrl.done || !ctrl.rcvr) return std::nullopt;
  std::optional<R> out = std::move(ctrl.rcvr);
  ctrl.rcvr.reset();
  return out;
}

// 标记取消：opstate 析构入口。
template <typename R>
void abandon(shared_control_block<R>& ctrl)
{
  std::lock_guard<std::mutex> lk(ctrl.mu);
  ctrl.done = true;
  ctrl.rcvr.reset();
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
    std::shared_ptr<shared_control_block<R>> ctrl;
    std::chrono::duration<Rep, Period> dur;

    Op(R r, std::chrono::duration<Rep, Period> d)
      : ctrl(std::make_shared<shared_control_block<R>>()), dur(d)
    {
      ctrl->rcvr.emplace(std::move(r));
    }
    Op(Op&&) = delete;
    ~Op()
    {
      // 取消路径：置 done 并销毁 receiver；后台线程到达时静默返回。
      abandon(*ctrl);
    }

    void start() & noexcept
    {
      std::thread([ctrl = ctrl, dur = dur] {
        std::this_thread::sleep_for(dur);
        auto rcvr = take_receiver(*ctrl);
        if (!rcvr) return;  // 已取消/销毁
        stdexec::set_value(std::move(*rcvr));
      }).detach();
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
    std::shared_ptr<shared_control_block<R>> ctrl;
    boost::asio::steady_timer timer;
    std::chrono::duration<Rep, Period> dur;

    Op(R r, boost::asio::io_context& io, std::chrono::duration<Rep, Period> d)
      : ctrl(std::make_shared<shared_control_block<R>>()), timer(io), dur(d)
    {
      ctrl->rcvr.emplace(std::move(r));
    }

    ~Op()
    {
      // 取消路径：置 done 并销毁 receiver；timer 析构取消 async_wait，
      // 其 handler（持 ctrl）稍后触发时发现 done 即返回。
      abandon(*ctrl);
    }

    void start() & noexcept
    {
      timer.expires_after(dur);
      timer.async_wait([ctrl = ctrl](const boost::system::error_code& ec) {
        auto rcvr = take_receiver(*ctrl);
        if (!rcvr) return;  // 已取消/销毁
        if (ec == boost::asio::error::operation_aborted) {
          stdexec::set_stopped(std::move(*rcvr));
        } else if (ec) {
          stdexec::set_error(std::move(*rcvr),
                             std::make_exception_ptr(boost::system::system_error(ec)));
        } else {
          stdexec::set_value(std::move(*rcvr));
        }
      });
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
