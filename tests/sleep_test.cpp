#include <dart_cpp_bridge/sleep.hpp>
#include <qjsbind/std_exec.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <exec/windows/windows_thread_pool.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include <chrono>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

namespace {

// sync_wait 运行 sender 并断言：正常完成 + 耗时 ≥ min_wait（容忍调度抖动）。
template <typename Sender>
void expect_sleep_ok(Sender&& s, std::chrono::milliseconds min_wait)
{
  const auto t0 = std::chrono::steady_clock::now();
  auto res = stdexec::sync_wait(std::forward<Sender>(s));
  const auto elapsed = std::chrono::steady_clock::now() - t0;
  ASSERT_TRUE(res.has_value());
  EXPECT_GE(elapsed, min_wait);
}

// io_context + 后台 run 线程的 RAII：断言失败提前 return 时也会 stop + join。
struct io_thread_guard {
  boost::asio::io_context& io;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
  std::thread th;

  explicit io_thread_guard(boost::asio::io_context& i)
    : io(i)
    , work(boost::asio::make_work_guard(io))
    , th([&io = i] { io.run(); })
  {}

  ~io_thread_guard()
  {
    work.reset();
    io.stop();
    if (th.joinable()) th.join();
  }
};

// 最小 receiver：统计到达的完成通道（用于取消路径测试）。
struct counting_receiver {
  using receiver_concept = stdexec::receiver_tag;
  int* value_calls;
  int* stop_calls;

  void set_value() && noexcept { ++*value_calls; }
  template <class E>
  void set_error(E&&) && noexcept { ++*stop_calls; }
  void set_stopped() && noexcept { ++*stop_calls; }
  auto get_env() const noexcept { return stdexec::empty_env(); }
};

}  // namespace

// 默认后端：exec::schedule_after + 全局 timed_thread_context
TEST(Sleep, ExecTimedThreadDefault)
{
  expect_sleep_ok(dcb::sleep(20ms), 15ms);
}

// Windows 线程池后端：exec::windows_thread_pool（CreateThreadpoolTimer）
TEST(Sleep, WindowsThreadPool)
{
  exec::windows_thread_pool pool;
  expect_sleep_ok(exec::schedule_after(pool.get_scheduler(), 20ms), 15ms);
}

// 对照后端：std::thread（每 sleep 一个线程）
TEST(Sleep, StdThreadBackend)
{
  expect_sleep_ok(dcb::thread_sleep(20ms), 15ms);
}

// 对照后端：boost::asio::steady_timer（io_context 在后台线程 run）
TEST(Sleep, AsioTimerBackend)
{
  boost::asio::io_context io;
  // RAII：work_guard 防 run() 空转返回；断言失败时析构也会 stop + join
  io_thread_guard io_guard(io);

  expect_sleep_ok(dcb::asio_sleep(io, 20ms), 15ms);
}

// 并行 3 个 sleep：总耗时应接近单个周期（真定时，不串行）
TEST(Sleep, ParallelWaits)
{
  const auto t0 = std::chrono::steady_clock::now();
  auto res = stdexec::sync_wait(stdexec::when_all(dcb::sleep(30ms), dcb::sleep(30ms),
                                                  dcb::sleep(30ms)));
  const auto elapsed = std::chrono::steady_clock::now() - t0;
  ASSERT_TRUE(res.has_value());
  EXPECT_GE(elapsed, 25ms);
  EXPECT_LT(elapsed, 80ms);  // 若串行实现会是 ~90ms
}

// 连续多次 sleep 稳定性
TEST(Sleep, RepeatedWaits)
{
  for (int i = 0; i < 5; ++i) {
    expect_sleep_ok(dcb::sleep(10ms), 5ms);
  }
}

// ---------------------------------------------------------------------------
// 取消路径（协程挂起中销毁 opstate）：回归测试，验证 begin_completion /
// abandon 串行化后不崩溃、不悬垂、不泄漏完成。
// ---------------------------------------------------------------------------

TEST(Sleep, CancelThreadBackendBeforeCompletion)
{
  int value_calls = 0;
  int stop_calls = 0;
  for (int i = 0; i < 100; ++i) {
    auto op = stdexec::connect(dcb::thread_sleep(5ms),
                               counting_receiver{&value_calls, &stop_calls});
    stdexec::start(op);
    // op 在此析构 = 取消：后台线程 5ms 后醒来应见 done 短路
  }
  // 全部被取消：set_value / set_stopped / set_error 均不应到达
  EXPECT_EQ(value_calls, 0);
  EXPECT_EQ(stop_calls, 0);
}

TEST(Sleep, CancelAsioBackendBeforeCompletion)
{
  boost::asio::io_context io;
  io_thread_guard guard(io);

  int value_calls = 0;
  int stop_calls = 0;
  for (int i = 0; i < 100; ++i) {
    auto op = stdexec::connect(dcb::asio_sleep(io, 5ms),
                               counting_receiver{&value_calls, &stop_calls});
    stdexec::start(op);
    // op 在此析构 = 取消：timer 析构取消 async_wait，迟到的 handler 见 done 短路
  }
  // 等 io 线程处理完所有取消的 handler
  std::this_thread::sleep_for(20ms);

  EXPECT_EQ(value_calls, 0);
  EXPECT_EQ(stop_calls, 0);
}

// ---------------------------------------------------------------------------
// 协程销毁发生在回调线程自身（真正触发 completing_thread 分支的形态）：
// 用 std_exec::task（标准 P2300 协程，co_await sender 走 as_awaitable 直接
// 同步 resume，不经过 continues_on/run_loop）——完成后台线程 set_value 在
// 完成线程原地恢复协程，co_return 时帧销毁连带 opstate 析构，析构线程 ==
// 回调线程。abandon 必须走 completing_thread 检测路径（不等待），否则死锁。
// 注：std_exec::task + sync_wait 下完成会 post 回 sync_wait 线程恢复协程，不会
// 走到该分支（修复前后都通过），故此处必须用 std_exec::task。
// ---------------------------------------------------------------------------

TEST(Sleep, CoroutineDestroysOpOnCallbackThread)
{
  auto task = []() -> std_exec::task<int> {
    co_await dcb::thread_sleep(5ms);
    co_return 42;
  }();
  auto res = stdexec::sync_wait(std::move(task));
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(std::get<0>(*res), 42);
}

TEST(Sleep, CoroutineDestroysOpOnAsioCallbackThread)
{
  boost::asio::io_context io;
  io_thread_guard guard(io);

  auto task = [&io]() -> std_exec::task<int> {
    co_await dcb::asio_sleep(io, 5ms);
    co_return 42;
  }();
  auto res = stdexec::sync_wait(std::move(task));
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(std::get<0>(*res), 42);
}
