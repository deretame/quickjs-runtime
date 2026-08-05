#include <boost/asio.hpp>
#include <exec/static_thread_pool.hpp>
#include <fmt/format.h>
#include <quickjs.h>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>

namespace asio = boost::asio;

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("quickjs-runtime 启动");

    // --- quickjs-ng: 内嵌 JS 引擎，执行一段脚本 ---
    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    JSValue result = JS_Eval(ctx, "1 + 2 * 3", 9, "<demo>", JS_EVAL_TYPE_GLOBAL);
    if (!JS_IsException(result)) {
        int32_t value = 0;
        JS_ToInt32(ctx, &value, result);
        spdlog::info("quickjs: 1 + 2 * 3 = {}", value);
    } else {
        spdlog::error("quickjs: 脚本执行失败");
        JS_FreeValue(ctx, result);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }
    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    // --- boost::asio: 事件循环 ---
    asio::io_context io;
    asio::post(io, [] { spdlog::info("boost::asio: post 回调执行"); });
    io.run();

    // --- stdexec: 线程池调度 ---
    exec::static_thread_pool pool{2};
    auto sched = pool.get_scheduler();
    stdexec::sync_wait(
        stdexec::schedule(sched) | stdexec::then([] {
            spdlog::info("stdexec: 线程池任务执行 (thread {})",
                         fmt::ptr(reinterpret_cast<void*>(std::hash<std::thread::id>{}(std::this_thread::get_id()))));
        }));

    spdlog::info("quickjs-runtime 正常退出");
    return 0;
}
