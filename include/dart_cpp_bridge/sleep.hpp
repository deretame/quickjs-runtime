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
// - 两个对照后端的取消路径（协程挂起中销毁 opstate）是安全的：receiver 由共享
//   控制块持有，opstate 析构（abandon）置 done 并**等待进行中的完成回调结束**
//   （in_flight 计数 + 条件变量）后才销毁 receiver——完成回调要么因 done 短路，
//   要么在 opstate 析构完成前已经结束，不存在悬垂回调访问已销毁帧。

#pragma once

#include <exec/timed_thread_scheduler.hpp>
#include <stdexec/execution.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <condition_variable>
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

// 共享控制块：把 receiver 的完整生命周期与 opstate 解耦，并串行化
// "完成回调"与"opstate 析构"两个方向：
//   begin_completion(): 锁内检查 done、move 出 receiver，in_flight++；
//                       完成回调据此获得唯一的交付权。
//   end_completion():   完成回调结束后 in_flight--，notify_all。
//   abandon():          opstate 析构入口；置 done、销毁仍持有的 receiver，
//                       并等待 in_flight == 0（进行中的回调结束）后才返回。
template <typename R>
struct shared_control_block {
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  int in_flight = 0;
  std::optional<R> rcvr;
};

template <typename R>
std::optional<R> begin_completion(shared_control_block<R>& ctrl)
{
  std::lock_guard<std::mutex> lk(ctrl.mu);
  if (ctrl.done || !ctrl.rcvr) return std::nullopt;
  std::optional<R> out = std::move(ctrl.rcvr);
  ctrl.rcvr.reset();
  ++ctrl.in_flight;
  return out;
}

template <typename R>
void end_completion(shared_control_block<R>& ctrl) noexcept
{
  std::lock_guard<std::mutex> lk(ctrl.mu);
  --ctrl.in_flight;
  ctrl.cv.notify_all();
}

template <typename R>
void abandon(shared_control_block<R>& ctrl)
{
  std::unique_lock<std::mutex> lk(ctrl.mu);
  ctrl.done = true;
  ctrl.rcvr.reset();
  // 等待进行中的完成回调结束（end_completion 会在其调用栈上层释放）。
  // 若析构恰发生在完成回调的 set_* 调用栈内（协程被恢复后立即完成），
  // cv.wait 释放锁，上层 end_completion 能拿到锁并 notify，无死锁。
  ctrl.cv.wait(lk, [&] { return ctrl.in_flight == 0; });
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
      // 取消路径：置 done、销毁 receiver，并等待进行中的完成回调结束。
      abandon(*ctrl);
    }

    void start() & noexcept
    {
      std::thread([ctrl = ctrl, dur = dur] {
        std::this_thread::sleep_for(dur);
        auto rcvr = begin_completion(*ctrl);
        if (!rcvr) return;  // 已取消/销毁
        stdexec::set_value(std::move(*rcvr));
        end_completion(*ctrl);
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
      // 取消路径：置 done、销毁 receiver，等待进行中的 handler 结束；
      // timer 析构取消 async_wait，迟到的 handler 见 done 即返回。
      abandon(*ctrl);
    }

    void start() & noexcept
    {
      timer.expires_after(dur);
      timer.async_wait([ctrl = ctrl](const boost::system::error_code& ec) {
        auto rcvr = begin_completion(*ctrl);
        if (!rcvr) return;  // 已取消/销毁
        if (ec == boost::asio::error::operation_aborted) {
          stdexec::set_stopped(std::move(*rcvr));
        } else if (ec) {
          stdexec::set_error(std::move(*rcvr),
                             std::make_exception_ptr(boost::system::system_error(ec)));
        } else {
          stdexec::set_value(std::move(*rcvr));
        }
        end_completion(*ctrl);
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
