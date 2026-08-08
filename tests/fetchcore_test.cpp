// fetchcore 纯 C++ 直连测试（fetch_cpp_decoupling.md §9 验收标尺）
//
// 不建 JSRuntime：直接 io_context + fetch::Client 打 WptTestServer / 自签
// TLS 服务器 / SOCKS5 测试服务器，验证核心库独立可用性与行为等价：
//   GET/POST、流式读、重定向（follow/error/manual/超限）、解压、SRI、
//   data: URL、abort（stop_token）、用户中间件、多实例共用 io、SOCKS5、
//   HTTPS（extra_trust_pem）。
//
// 驱动方式（§4.2 方案 B）：spawn 上 io 调度器，counting_scope close+join 后
// 用 poll 循环驱动 io（ScopeJoiner；io.run() 会因残留在飞 work 挂起，
// join 语义保证 scope 析构安全，见 stdexec [exec.simple.counting]）。
#include <gtest/gtest.h>
#include <fetch/client.hpp>
#include <fetch/scheduler.hpp>
#include "wpt_server.hpp"
#include "socks5_server.hpp"
#include "tls_echo_server.hpp"

#include <stdexec/execution.hpp>

#include <chrono>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

namespace {

namespace wpt = qjsbind::net::wpt;

// ---- 驱动辅助：counting_scope 的 join 驱动 ----------------
// counting_scope 析构要求 state 为 joined（stdexec [exec.simple.counting]：
// 仅 spawn 不 join 会 terminate）。close 后 connect 一个 join sender，
// 用 poll 循环驱动 io（不阻塞）直到 join 完成。
struct ScopeJoiner {
    struct JoinRcvr {
        bool* joined;
        boost::asio::io_context* io;
        using receiver_concept = stdexec::receiver_t;
        void set_value() noexcept { *joined = true; }
        void set_error(std::exception_ptr) noexcept { *joined = true; }
        void set_stopped() noexcept { *joined = true; }
        auto get_env() const noexcept
        {
            return stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{*io}};
        }
    };

    static bool run(stdexec::counting_scope& scope, boost::asio::io_context& io)
    {
        scope.close();
        bool joined = false;
        auto join_op = stdexec::connect(scope.join(), JoinRcvr{&joined, &io});
        stdexec::start(join_op);
        for (int i = 0; !joined && i < 20000; ++i) {
            io.poll();
            if (!joined)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (!joined) // 轮询耗尽：join 未完成（慢机/死锁）——调用方据返回值判失败
            std::fprintf(stderr, "[fetchcore] warning: scope join 超时（20000 轮 poll）\n");
        return joined;
    }
};

// ---- 驱动辅助：把协程任务 spawn 上 io 调度器并跑 io.run() ----
// 结果（done/stopped/error）存成员，主线程断言。
struct Probe {
    boost::asio::io_context io;
    fetch::Client client{io};
    bool done = false;
    bool stopped = false;
    std::exception_ptr error;

    void run(std_exec::task<void> work)
    {
        stdexec::counting_scope scope;
        stdexec::spawn(
            std::move(work)
                | stdexec::then([this]() noexcept { done = true; })
                | stdexec::upon_error([this](std::exception_ptr ep) noexcept {
                      error = std::move(ep);
                      done = true;
                  })
                | stdexec::upon_stopped([this]() noexcept {
                      stopped = true;
                      done = true;
                  }),
            scope.get_token(),
            stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{io}});
        // counting_scope 析构要求 state 为 joined——必须 close + join
        // （见 stdexec [exec.simple.counting]；仅 spawn 不 join 会 terminate）。
        // 超时由 ScopeJoiner 告警 + 调用方 done 断言（协程未完成）覆盖。
        (void)ScopeJoiner::run(scope, io);
    }

    std::string error_message() const
    {
        if (!error)
            return {};
        try {
            std::rethrow_exception(error);
        } catch (const std::exception& e) {
            return e.what();
        }
    }
};

std::string read_cert()
{
    for (const char* p : {"tests/certs/server.crt", "../tests/certs/server.crt",
                          "../../tests/certs/server.crt"}) {
        std::ifstream f(p);
        if (f)
            return std::string(std::istreambuf_iterator<char>(f),
                               std::istreambuf_iterator<char>());
    }
    return {};
}

// ---- 最小驱动自检：空协程经 spawn + io.run 应同步完成 ----
// ---- GET/POST 直连 ----
TEST(FetchcoreDirect, GetAndPost)
{
    wpt::WptTestServer server("third_party/wpt");
    const std::string base = server.base_url();
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        // GET：200 + reason + 无 body
        fetch::Request get;
        get.url = base + "/echo-content.py";
        fetch::Response r = co_await p.client.fetch(std::move(get));
        EXPECT_EQ(r.status, 200);
        EXPECT_EQ(r.reason, "OK");
        EXPECT_FALSE(r.redirected);
        EXPECT_EQ(r.url, base + "/echo-content.py");
        std::string got1 = co_await fetch::read_all(r); // 先读干再断言（co_await 不进断言宏）
        EXPECT_EQ(got1, "");
        // POST：body 回显
        fetch::Request post;
        post.method = "POST";
        post.url = base + "/echo-content.py";
        post.body = "hello from fetchcore";
        fetch::Response r2 = co_await p.client.fetch(std::move(post));
        EXPECT_EQ(r2.status, 200);
        std::string got2 = co_await fetch::read_all(r2);
        EXPECT_EQ(got2, "hello from fetchcore");
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
}

// ---- 流式读：慢响应分块读 ----
TEST(FetchcoreDirect, StreamingRead)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    std::string body;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.url = server.base_url() + "/slow-response.py?delay=150&content=streamed";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        EXPECT_EQ(resp.status, 200);
        // 手动分块读（拉模型：read 逐块）
        for (;;) {
            auto block = co_await resp.body->read();
            if (!block)
                break;
            body += *block;
        }
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
    EXPECT_EQ(body, "streamed");
}

// ---- 重定向 follow / error / manual / 超限 ----
TEST(FetchcoreDirect, RedirectFollow)
{
    wpt::WptTestServer server("third_party/wpt");
    const std::string base = server.base_url();
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        // 302 → /status.py?code=200（simple：Location 原样，不附加 query）
        fetch::Request req;
        req.url = base + "/redirect.py?location=/status.py?code=200&redirect_status=302&simple=1";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        EXPECT_EQ(resp.status, 200);
        EXPECT_TRUE(resp.redirected);
        EXPECT_EQ(resp.url, base + "/status.py?code=200");
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
}

// Location 带大写 scheme（HTTP://…）：WHATWG 解析器归一为小写后跟随
// （review should-fix 1——resolve_url 小写化 scheme，传输层大小写不敏感比较）
TEST(FetchcoreDirect, RedirectSchemeCase)
{
    wpt::WptTestServer server("third_party/wpt");
    const std::string base = server.base_url();
    const std::string port = base.substr(base.rfind(':') + 1);
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.url = base + "/redirect.py?location=HTTP://127.0.0.1:" + port +
                  "/status.py?code=200&redirect_status=302&simple=1";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        EXPECT_EQ(resp.status, 200);
        EXPECT_EQ(resp.url, base + "/status.py?code=200");
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
}

// resolve_url 归一单测（review sa_20260808_171142：encoded host 保留、
// 前导零端口剥离、非 http/https 不剥离、IPv6 不抛）
TEST(FetchcoreDirect, ResolveUrlNormalization)
{
    // encoded host 不二次编码（%41 = 'A' 保留；round-trip 无二次 escape）
    EXPECT_EQ(fetch::resolve_url("http://%41.example/x", "http://base/"),
              "http://%41.example/x");
    // 前导零默认端口剥离（080 == 80，WHATWG 数值语义）
    EXPECT_EQ(fetch::resolve_url("http://h:080/x", "http://base/"),
              "http://h/x");
    // 非默认端口保留；ws://h:80 是 ws 的默认端口（WHATWG 默认端口表）→ 剥离
    EXPECT_EQ(fetch::resolve_url("http://h:8080/x", "http://base/"),
              "http://h:8080/x");
    EXPECT_EQ(fetch::resolve_url("ws://h:80/x", "http://base/"),
              "ws://h/x");
    // IPv6 文字地址：host 跳过小写化、不抛异常
    EXPECT_EQ(fetch::resolve_url("http://[::1]:8080/x", "http://base/"),
              "http://[::1]:8080/x");
    // IPv6 + 默认端口剥离（[::1]:80 → 剥）
    EXPECT_EQ(fetch::resolve_url("http://[::1]:80/x", "http://base/"),
              "http://[::1]/x");
    // pct hex 大写 → 小写（%AF 保留 escape 但 hex 字母归一）
    EXPECT_EQ(fetch::resolve_url("http://%AF.example/x", "http://base/"),
              "http://%af.example/x");
    // host 小写化（普通域名）
    EXPECT_EQ(fetch::resolve_url("http://ExAmPle.COM/x", "http://base/"),
              "http://example.com/x");
}

TEST(FetchcoreDirect, RedirectErrorMode)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.url = server.base_url() + "/redirect.py?location=/x&redirect_status=302&simple=1";
        req.redirect = fetch::Request::Redirect::error;
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        (void)resp;
    }());
    ASSERT_TRUE(p.done);
    ASSERT_TRUE(p.error) << "redirect=error 应抛 fetch::Error";
    EXPECT_EQ(p.error_message(), "fetch: redirect mode 为 error");
}

TEST(FetchcoreDirect, RedirectManualMode)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.url = server.base_url() + "/redirect.py?location=/x&redirect_status=301&simple=1";
        req.redirect = fetch::Request::Redirect::manual;
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        // opaqueredirect 哨兵：status==0 且 url 空（绑定层据此构造 opaqueredirect）
        EXPECT_EQ(resp.status, 0);
        EXPECT_TRUE(resp.url.empty());
        EXPECT_FALSE(resp.body);
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
}

TEST(FetchcoreDirect, RedirectLoopExceeded)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        // 无 simple → 服务器把 count 追加进 Location，无限重定向
        fetch::Request req;
        req.url = server.base_url() + "/redirect.py?location=/redirect.py&redirect_status=302";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        (void)resp;
    }());
    ASSERT_TRUE(p.done);
    ASSERT_TRUE(p.error);
    EXPECT_EQ(p.error_message(), "fetch: 重定向次数超过 20");
}

// ---- 解压（Accept-Encoding 内建中间件）----
TEST(FetchcoreDirect, Decompress)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    std::string out;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.method = "POST";
        req.url = server.base_url() + "/compress.py?code=gzip";
        req.body = "hello world";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        out += "gzip:" + co_await fetch::read_all(resp) + ":";
        fetch::Request req2;
        req2.method = "POST";
        req2.url = server.base_url() + "/compress.py?code=br";
        req2.body = "hello brotli";
        fetch::Response resp2 = co_await p.client.fetch(std::move(req2));
        out += "br:" + co_await fetch::read_all(resp2);
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
    EXPECT_EQ(out, "gzip:hello world:br:hello brotli");
}

// ---- SRI：正确摘要通过；错误摘要 read 阶段抛异常 ----
TEST(FetchcoreDirect, Integrity)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    std::string out;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request ok;
        ok.method = "POST";
        ok.url = server.base_url() + "/echo-content.py";
        ok.body = "hello world";
        // 'hello world' 的 sha384
        ok.integrity = "sha384-/b2OdaZ/KfcBpOBAOF4uI5hjA+oQI5IRr5B/y7g1eLPkF8txzmRu/QgZ3YwIjeG9";
        fetch::Response resp = co_await p.client.fetch(std::move(ok));
        out += co_await fetch::read_all(resp);
        // 错误摘要：fetch 正常返回，读干时抛异常（SRI 消费末端校验）
        fetch::Request bad;
        bad.method = "POST";
        bad.url = server.base_url() + "/echo-content.py";
        bad.body = "hello world";
        bad.integrity = "sha384-" + std::string(64, 'A');
        fetch::Response resp2 = co_await p.client.fetch(std::move(bad));
        out += "|" + co_await fetch::read_all(resp2); // 应抛
    }());
    ASSERT_TRUE(p.done);
    // 错误摘要的 read_all 抛 runtime_error（SRI 不匹配）——通过 error 通道捕获
    EXPECT_TRUE(p.error) << "错误摘要应抛异常";
    // 注意：`out += "|" + co_await read_all(resp2)` 中 co_await 先求值（抛出），
    // "|" 未拼接——out 停在第一个成功读取的 "hello world"
    EXPECT_EQ(out, "hello world");
    EXPECT_EQ(p.error_message(), "integrity: 摘要不匹配（SRI 校验失败）");
}

// 204 null body + integrity：fetch 阶段立即抛 fetch::Error（null body 无法校验）
// ——与错误摘要用例拆开（协程异常后后续不可达，review nit 1）
TEST(FetchcoreDirect, IntegrityNullBody)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request nullb;
        nullb.url = server.base_url() + "/status.py?code=204";
        nullb.integrity = "sha384-UT6f7WCFp32YJnp1is4l/ZYnOeQKpE8xjmdkLOwZ3nIP+tmT2aMRFQGJomjVf5cE";
        fetch::Response resp3 = co_await p.client.fetch(std::move(nullb));
        (void)resp3;
    }());
    ASSERT_TRUE(p.done);
    ASSERT_TRUE(p.error) << "204 + integrity 应抛 fetch::Error";
    EXPECT_EQ(p.error_message(), "fetch: integrity 无法校验 null body 响应");
}

// ---- data: URL 本地构造 ----
TEST(FetchcoreDirect, DataUrl)
{
    Probe p;
    std::string out;
    p.run([&]() -> std_exec::task<void> {
        fetch::Request text;
        text.url = "data:text/plain,hello%20data";
        fetch::Response r1 = co_await p.client.fetch(std::move(text));
        EXPECT_EQ(r1.status, 200);
        EXPECT_EQ(r1.url, "data:text/plain,hello%20data");
        EXPECT_FALSE(r1.redirected);
        std::string t1 = co_await fetch::read_all(r1); // 先读干再断言
        EXPECT_EQ(t1, "hello data");
        // base64
        fetch::Request b64;
        b64.url = "data:application/json;base64,eyJrIjogMX0=";
        fetch::Response r2 = co_await p.client.fetch(std::move(b64));
        out = co_await fetch::read_all(r2);
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
    EXPECT_EQ(out, "{\"k\": 1}");
}

// ---- abort：fetch resolve 后挂起的 body 读被取消（stopped）----
// 对齐绑定层语义（fetch_test M1FetchAbortDuringBody）：慢响应头先到，
// fetch resolve；body 挂起期间 request_stop → socket.cancel → 读以 stopped 完成。
TEST(FetchcoreDirect, Abort)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    p.run([&]() -> std_exec::task<void> {
        std::stop_source ss;
        fetch::Request req;
        req.url = server.base_url() + "/slow-response.py?delay=400&content=hello";
        fetch::Response resp = co_await p.client.fetch(std::move(req), ss.get_token());
        // fetch 已 resolve（头到）；30ms 后 abort 挂起的 body 读
        boost::asio::steady_timer t(p.io, std::chrono::milliseconds(30));
        t.async_wait([&](const boost::system::error_code&) { ss.request_stop(); });
        co_await resp.body->read(); // 以 stopped 中断（不走这里）
        EXPECT_TRUE(false) << "读不应正常返回";
    }());
    ASSERT_TRUE(p.done);
    EXPECT_TRUE(p.stopped) << "abort 应使挂起的读以 stopped 完成";
    EXPECT_FALSE(p.error);
}

// ---- 用户中间件：鉴权头注入（C++ 插件定位示例）----
namespace {
struct AuthMiddleware : fetch::Middleware {
    std_exec::task<fetch::Response> intercept(const fetch::Request& req, std::stop_token st,
                                              fetch::Handler next) override
    {
        fetch::Request r = req; // req 只读：修改先拷贝
        r.headers.push_back({"Authorization", "Bearer token-123"});
        co_return co_await next(r, st);
    }
};
}

TEST(FetchcoreDirect, UserMiddleware)
{
    wpt::WptTestServer server("third_party/wpt");
    Probe p;
    std::string out;
    p.client.use(std::make_shared<AuthMiddleware>()); // 注入鉴权中间件
    p.run([&]() -> std_exec::task<void> {
        fetch::Request req;
        req.url = server.base_url() + "/echo-content.py";
        fetch::Response resp = co_await p.client.fetch(std::move(req));
        // 服务器把 X-Request-* 头回显在响应体外的响应头中
        for (const auto& h : resp.headers)
            if (fetch::header_name_eq(h.name, "X-Request-Authorization"))
                out = h.value;
    }());
    ASSERT_TRUE(p.done);
    ASSERT_FALSE(p.error) << p.error_message();
    EXPECT_EQ(out, "Bearer token-123");
}

// ---- 多实例共用同一 io：各自独立配置互不干扰 ----
TEST(FetchcoreDirect, MultiInstanceSharedIo)
{
    wpt::WptTestServer server("third_party/wpt");
    boost::asio::io_context io;
    fetch::Client c1{io};
    fetch::Client c2{io}; // 同一 io，独立实例
    std::string r1, r2;
    bool done1 = false, done2 = false;
    stdexec::counting_scope scope;
    auto work1 = [&]() -> std_exec::task<void> {
        fetch::Request req;
        req.method = "POST";
        req.url = server.base_url() + "/echo-content.py";
        req.body = "c1";
        fetch::Response resp = co_await c1.fetch(std::move(req));
        r1 = co_await fetch::read_all(resp);
    }();
    stdexec::spawn(
        std::move(work1)
            | stdexec::then([&]() noexcept { done1 = true; })
            | stdexec::upon_error([&](std::exception_ptr) noexcept { done1 = false; })
            | stdexec::upon_stopped([&]() noexcept { done1 = false; }),
        scope.get_token(),
        stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{io}});
    auto work2 = [&]() -> std_exec::task<void> {
        fetch::Request req;
        req.method = "POST";
        req.url = server.base_url() + "/echo-content.py";
        req.body = "c2";
        fetch::Response resp = co_await c2.fetch(std::move(req));
        r2 = co_await fetch::read_all(resp);
    }();
    stdexec::spawn(
        std::move(work2)
            | stdexec::then([&]() noexcept { done2 = true; })
            | stdexec::upon_error([&](std::exception_ptr) noexcept { done2 = false; })
            | stdexec::upon_stopped([&]() noexcept { done2 = false; }),
        scope.get_token(),
        stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{io}});
    EXPECT_TRUE(ScopeJoiner::run(scope, io)) << "scope join 超时";
    EXPECT_TRUE(done1);
    EXPECT_TRUE(done2);
    EXPECT_EQ(r1, "c1");
    EXPECT_EQ(r2, "c2");
}
// ---- SOCKS5：选路命中走隧道 ----
TEST(FetchcoreDirect, Socks5)
{
    wpt::WptTestServer wpt_server("third_party/wpt");
    wpt::Socks5TestServer proxy;
    proxy.start();
    boost::asio::io_context io;
    auto transport = std::make_shared<fetch::BeastTransport>(io);
    fetch::Client client{io, transport};
    const fetch::Socks5Proxy p{proxy.host(), proxy.port(), std::nullopt};
    client.use(std::make_shared<fetch::Socks5ProxyMiddleware>(
        transport, [p](const std::string&) { return p; }));
    bool ok = false;
    stdexec::counting_scope scope;
    auto work = [&]() -> std_exec::task<void> {
        fetch::Request req;
        req.method = "POST";
        req.url = wpt_server.base_url() + "/echo-content.py";
        req.body = "via socks5";
        fetch::Response resp = co_await client.fetch(std::move(req));
        ok = co_await fetch::read_all(resp) == "via socks5";
    }();
    stdexec::spawn(
        std::move(work)
            | stdexec::then([&]() noexcept { ok = ok && true; })
            | stdexec::upon_error([&](std::exception_ptr) noexcept { ok = false; })
            | stdexec::upon_stopped([&]() noexcept { ok = false; }),
        scope.get_token(),
        stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{io}});
    EXPECT_TRUE(ScopeJoiner::run(scope, io)) << "scope join 超时";
    EXPECT_TRUE(ok) << "经 SOCKS5 隧道请求失败";
}

// ---- HTTPS：自签证书 + extra_trust_pem ----
TEST(FetchcoreDirect, Https)
{
    const std::string cert = read_cert();
    ASSERT_FALSE(cert.empty());
    wpt_test::TlsEchoServer tls;
    boost::asio::io_context io;
    auto transport = std::make_shared<fetch::BeastTransport>(
        io, fetch::TlsOptions{true, {cert}});
    fetch::Client client{io, transport};
    std::string out;
    bool ok = false;
    stdexec::counting_scope scope;
    auto work = [&]() -> std_exec::task<void> {
        fetch::Request req;
        req.method = "POST";
        req.url = tls.base_url() + "/echo";
        req.body = "tls direct";
        fetch::Response resp = co_await client.fetch(std::move(req));
        out = co_await fetch::read_all(resp);
    }();
    stdexec::spawn(
        std::move(work)
            | stdexec::then([&]() noexcept { ok = true; })
            | stdexec::upon_error([&](std::exception_ptr) noexcept { ok = false; })
            | stdexec::upon_stopped([&]() noexcept { ok = false; }),
        scope.get_token(),
        stdexec::prop{stdexec::get_start_scheduler, fetch::io_scheduler{io}});
    EXPECT_TRUE(ScopeJoiner::run(scope, io)) << "scope join 超时";
    ASSERT_TRUE(ok);
    EXPECT_EQ(out, "tls direct");
}

} // namespace
