#include <dart_cpp_bridge/sleep.hpp>

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
  // work_guard 防止 run() 在没有 pending 操作时立即返回（经典 asio 竞态）
  auto work = boost::asio::make_work_guard(io);
  std::thread io_thread([&io] { io.run(); });

  expect_sleep_ok(dcb::asio_sleep(io, 20ms), 15ms);

  work.reset();
  io.stop();
  io_thread.join();
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
