// qjsbind::web —— setTimeout / setInterval / clearTimeout / clearInterval
//
// 实现：asio::steady_timer + 全局注册表。回调在 io 线程执行（= JS 线程，
// Runtime::run 单线程驱动 io_context）。Runtime 析构时 io_context 销毁，
// 未完成定时器随之取消，回调不会在 ctx 死后执行。
// 回调函数经 qjs::RtValue 持有（JSRuntime 释放，见 rt_value.hpp）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/function.hpp>
#include <qjsbind/rt_value.hpp>

#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>

namespace qjsbind::web {

namespace timers_detail {

struct timer_entry {
    std::shared_ptr<boost::asio::steady_timer> timer;
    JSContext* ctx = nullptr;
    qjs::RtValue fn;                 // 回调函数
    std::vector<qjs::RtValue> args;  // 附加参数
    bool repeat = false;
};

inline std::unordered_map<uint64_t, std::shared_ptr<timer_entry>>& timer_map() {
    static std::unordered_map<uint64_t, std::shared_ptr<timer_entry>> m;
    return m;
}
inline std::atomic<uint64_t> next_id{1};

inline void invoke_callback(JSContext* ctx, const timer_entry& e) {
    // 清理引擎可能挂起的异常（不应有，防御性）
    if (JS_HasException(ctx)) {
        JSValue exc = JS_GetException(ctx);
        JS_FreeValue(ctx, exc);
    }
    std::vector<JSValue> argv;
    argv.reserve(e.args.size());
    for (const auto& a : e.args)
        argv.push_back(a.dup(ctx));
    JSValue r = JS_Call(ctx, e.fn.raw(), JS_UNDEFINED, static_cast<int>(argv.size()),
                        argv.data());
    for (const auto& a : argv)
        JS_FreeValue(ctx, a);
    if (JS_IsException(r)) {
        JSValue exc = JS_GetException(ctx);
        const char* str = JS_ToCString(ctx, exc);
        std::fprintf(stderr, "[qjsbind::web] timer callback threw: %s\n", str ? str : "?");
        if (str)
            JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, r);
}

inline void run_timer(uint64_t id, std::shared_ptr<timer_entry> e) {
    if (e->repeat) {
        e->timer->async_wait([id, e](const boost::system::error_code& ec) {
            if (!ec)
                run_timer(id, e); // 先安排下一次，再回调（回调异常不影响周期）
        });
        invoke_callback(e->ctx, *e);
    } else {
        timer_map().erase(id); // 一次性：先摘除再回调（回调里 clearTimeout(id) 无副作用）
        invoke_callback(e->ctx, *e);
    }
}

inline uint64_t schedule(JSContext* ctx, qjs::Function fn, double ms,
                         std::vector<qjs::RtValue> args, bool repeat) {
    const uint64_t id = next_id.fetch_add(1);
    auto e = std::make_shared<timer_entry>();
    e->ctx = ctx;
    e->fn = qjs::RtValue(JS_GetRuntime(ctx), fn.take());
    e->args = std::move(args);
    e->repeat = repeat;
    e->timer = std::make_shared<boost::asio::steady_timer>(
        qjs::current_io(),
        std::chrono::milliseconds(ms > 0 ? static_cast<long long>(ms) : 0));
    timer_map()[id] = e;
    e->timer->async_wait([id, e](const boost::system::error_code& ec) {
        if (!ec)
            run_timer(id, e);
    });
    return id;
}

inline void clear(uint64_t id) {
    auto it = timer_map().find(id);
    if (it != timer_map().end()) {
        it->second->timer->cancel(); // 新版 asio：cancel() 无 error_code 重载
        timer_map().erase(it);
    }
}

} // namespace timers_detail

inline void install_timers(qjs::Context& ctx) {
    using namespace timers_detail;
    ctx.globals().set("setTimeout",
                      qjs::func(ctx.raw(),
                                [](qjs::Ctx c, qjs::Function fn, double ms,
                                   qjs::Rest<qjs::Value> args) -> uint64_t {
                                    std::vector<qjs::RtValue> rt_args;
                                    for (auto& a : args.items)
                                        rt_args.emplace_back(JS_GetRuntime(c.ctx), a.take());
                                    return schedule(c.ctx, std::move(fn), ms, std::move(rt_args),
                                                    false);
                                },
                                "setTimeout"));
    ctx.globals().set("setInterval",
                      qjs::func(ctx.raw(),
                                [](qjs::Ctx c, qjs::Function fn, double ms,
                                   qjs::Rest<qjs::Value> args) -> uint64_t {
                                    std::vector<qjs::RtValue> rt_args;
                                    for (auto& a : args.items)
                                        rt_args.emplace_back(JS_GetRuntime(c.ctx), a.take());
                                    return schedule(c.ctx, std::move(fn), ms, std::move(rt_args),
                                                    true);
                                },
                                "setInterval"));
    ctx.globals().set("clearTimeout",
                      qjs::func(ctx.raw(),
                                [](uint64_t id) { timers_detail::clear(id); }, "clearTimeout"));
    ctx.globals().set("clearInterval",
                      qjs::func(ctx.raw(),
                                [](uint64_t id) { timers_detail::clear(id); }, "clearInterval"));
}

} // namespace qjsbind::web
