// 端到端演示（设计文档 §11）：同步函数 + 异步协程函数 + 类 + 事件循环
#include <chrono>
#include <qjsbind/std_exec.hpp>
#include <memory>
#include <string>

#include <boost/asio/steady_timer.hpp>
#include <exec/asio/use_sender.hpp>
#include <stdexec/execution.hpp>
#include <qjsbind/qjsbind.hpp>
#include <spdlog/spdlog.h>

using qjs::This;

// ---- 同步自由函数：签名无 qjs 类型 ----
double add(double a, double b) { return a + b; }

// ---- 异步自由函数（★ 自由函数，不是 lambda —— 见 known_issues KI-001）----
// 无 Ctx 参数：经 qjs::current_io() 拿当前 JS 线程的事件循环
std_exec::task<std::string> greet_after(std::string name, double ms)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(
        qjs::current_io(), boost::asio::chrono::milliseconds(static_cast<long long>(ms)));
    co_await timer->async_wait(exec::asio::use_sender);
    co_return "hello, " + name;
}

// ---- 类 ----
struct Counter {
    int value = 0;
    int add(int d) { return value += d; }
};

// 异步方法糖：This<Counter> + std_exec::task 自由函数，经 method() 注册
std_exec::task<int> counter_add_later(This<Counter> self, int d, double ms)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(
        qjs::current_io(), boost::asio::chrono::milliseconds(static_cast<long long>(ms)));
    co_await timer->async_wait(exec::asio::use_sender);
    co_return self->add(d);
}

int main()
{
    qjs::Runtime rt;
    qjs::Context ctx = rt.main_context();

    auto globals = ctx.globals();
    globals.set("add", add);
    globals.set("greetAfter", greet_after);
    globals.set("log", qjs::func(ctx.raw(), [](const std::string& s) { spdlog::info("{}", s); }));

    qjs::class_<Counter> counter_cls(ctx, "Counter");
    counter_cls.constructor<>()
        .method("add", &Counter::add)
        .method("addLater", counter_add_later) // 异步方法（自由函数糖）
        .field("value", &Counter::value);
    globals.set("Counter", counter_cls.constructor_function());

    qjs::Value r = ctx.eval(R"(
        const c = new Counter();
        c.add(5);
        Promise.all([greetAfter("qjs", 30), c.addLater(2, 30)])
          .then(([g, total]) => {
              log(g + " total=" + total + " value=" + c.value);
              globalThis.__done = true;
          });
        'ok';
    )");
    if (r.is_exception())
        spdlog::error("脚本执行失败");

    rt.run_to_completion(); // 脚本模式：Promise.all 完成后 pending==0 退出（§8 模式 B）
    spdlog::info("qjsbind 端到端演示完成");
    return 0;
}
