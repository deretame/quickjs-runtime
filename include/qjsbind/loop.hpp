// loop.hpp —— 事件循环：Runtime::spawn / run / stop / shutdown / pump_js_jobs
//
// 设计文档 §8：
//   - 退出判据 = pending_ 计数（Runtime::spawn +1、三路收尾 -1），不用 on_empty
//   - 检查总在 pump_js_jobs() 之后（pump 排干结算 job 才可能 spawn 新任务）
//   - stop() 任意线程可调（asio::post 置 done_），shutdown 单向（request_stop 不可逆）
#pragma once

#include <atomic>
#include <cstdio>

#include <quickjs.h>
#include <stdexec/execution.hpp>

#include <qjsbind/context.hpp>
#include <qjsbind/error.hpp>

namespace qjs {

// ---- 统一 spawn 入口（设计文档 §8.1 不变量 1）----
// 三路收尾消化 error/stopped（noexcept，否则 async_scope::spawn 会 terminate），
// 并维护 pending_ 计数；env 注入 get_start_scheduler（exec::task 要求）。
template <stdexec::sender S>
void Runtime::spawn(S&& sndr)
{
    pending_.fetch_add(1, std::memory_order_relaxed);
    scope_.spawn(
        std::forward<S>(sndr)
            | stdexec::then([this](auto&&...) noexcept { complete(); })
            | stdexec::upon_error([this](auto&&) noexcept { complete(); })
            | stdexec::upon_stopped([this]() noexcept { complete(); }),
        stdexec::prop{stdexec::get_start_scheduler, io_scheduler()});
}

// ---- JS job 泵：一次排干（与浏览器微任务语义一致）----
inline void Runtime::pump_js_jobs()
{
    JSContext* c;
    int r;
    while ((r = JS_ExecutePendingJob(rt_, &c)) == 1) {
    }
    if (r < 0) {
        // job 执行抛了未捕获异常：取走并打印（类似 js_std_dump_error）
        JSValue exc = JS_GetException(c ? c : ctx_);
        const char* msg = JS_ToCString(c ? c : ctx_, exc);
        std::fprintf(stderr, "[qjs] unhandled job exception: %s\n", msg ? msg : "(null)");
        if (msg)
            JS_FreeCString(c ? c : ctx_, msg);
        JS_FreeValue(c ? c : ctx_, exc);
    }
}

inline void Runtime::loop_body()
{
    pump_js_jobs();
    io_.run_one(); // 无 ready handler 时阻塞等未来工作（guard_ 保活）
}

// ---- 模式 A：服务模式（默认主模式，设计文档 §8.2）----
inline void Runtime::run()
{
    // 绑定当前 JS 线程（TLS）：协程异步函数经 current_io() 拿事件循环
    Runtime* prev = tls_current_runtime;
    tls_current_runtime = this;
    guard_.emplace(boost::asio::make_work_guard(io_));
    while (!done_.load(std::memory_order_acquire))
        loop_body();
    shutdown();
    tls_current_runtime = prev;
}

// ---- 模式 B：脚本模式：排干后无在飞异步即退出（设计文档 §8.2）----
inline void Runtime::run_to_completion()
{
    Runtime* prev = tls_current_runtime;
    tls_current_runtime = this;
    guard_.emplace(boost::asio::make_work_guard(io_));
    for (;;) {
        pump_js_jobs();
        if (done_.load(std::memory_order_acquire) ||
            pending_.load(std::memory_order_acquire) == 0)
            break;
        io_.run_one();
    }
    shutdown();
    tls_current_runtime = prev;
}

// ---- stop()：任意线程可调，只碰 io_（线程安全）、不碰 JS ----
inline void Runtime::stop()
{
    boost::asio::post(io_, [this] { done_.store(true, std::memory_order_release); });
}

// ---- shutdown()：单向关闭（设计文档 §8.3）----
inline void Runtime::shutdown()
{
    if (shutdown_done_.load(std::memory_order_acquire))
        return;
    shutdown_done_.store(true, std::memory_order_release);

    scope_.request_stop(); // 1. 通知所有在飞任务（协作式取消）
    while (pending_.load(std::memory_order_acquire) != 0) { // 2. 驱动到全部结算
        pump_js_jobs();
        io_.run_one(); // guard_ 仍在：取消结算必然 post 回 io_
    }
    pump_js_jobs(); // 3. 收尾 job（AbortError 的 then/catch 链）
    guard_.reset(); // 4. 解除保活
}

} // namespace qjs
