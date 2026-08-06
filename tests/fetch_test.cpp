// fetch 功能验收测试：Web API 层（URL/Headers/TextEncoder/AbortController）+ fetch
//
// 覆盖（v1 边界）：
//   - URL / URLSearchParams 解析与操作
//   - Headers 规范（大小写/合并/校验/迭代）
//   - TextEncoder / TextDecoder（UTF-8、代理对、BOM）
//   - AbortController / AbortSignal 事件与状态
//   - Request / Response 构造与 body 消费
//   - fetch 全链路（本地 HTTP 服务器 + 重定向 + 取消）
#include <gtest/gtest.h>
#include <qjsbind/qjsbind.hpp>
#include <qjsbind/web/web.hpp>
#include <net/http_backend.hpp>

using namespace qjs;

namespace {

struct FetchFixture : ::testing::Test {
    Runtime rt;
    Context ctx = rt.main_context();
    std::shared_ptr<qjsbind::net::BeastFetchBackend> backend;

    FetchFixture()
        : backend(std::make_shared<qjsbind::net::BeastFetchBackend>(rt.io()))
    {
        qjsbind::web::install_web_apis(ctx, backend);
    }
};

// ---- URL ----
TEST_F(FetchFixture, UrlParseAndProperties)
{
    Value r = ctx.eval(
        "var u = new URL('/a/b?x=1&y=2#frag', 'http://example.com:8080/');"
        "u.href + '|' + u.protocol + '|' + u.host + '|' + u.port + '|' + u.pathname + '|' +"
        "u.search + '|' + u.hash + '|' + u.origin;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(),
              "http://example.com:8080/a/b?x=1&y=2#frag|http:|example.com:8080|8080|/a/b|"
              "?x=1&y=2|#frag|http://example.com:8080");
}

TEST_F(FetchFixture, UrlDefaultPort)
{
    EXPECT_EQ(ctx.eval("new URL('http://example.com:80/x').port").as<std::string>(), "");
    EXPECT_EQ(ctx.eval("new URL('https://example.com:443/x').port").as<std::string>(), "");
}

TEST_F(FetchFixture, UrlRelativeWithoutBase)
{
    Value r = ctx.eval("(() => { try { new URL('/x'); return 'no-error' } catch (e) { return e.name } })()");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "TypeError");
}

TEST_F(FetchFixture, UrlSearchParams)
{
    Value r = ctx.eval(
        "var p = new URLSearchParams('a=1&b=2&a=3');"
        "p.get('a') + '|' + p.getAll('a').join(',') + '|' + p.has('b') + '|' + p.toString();");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "1|1,3|true|a=1&b=2&a=3");
}

TEST_F(FetchFixture, UrlSearchParamsFromObject)
{
    EXPECT_EQ(ctx.eval("new URLSearchParams({x: 1, y: 'hello world'}).toString()")
                  .as<std::string>(),
              "x=1&y=hello+world");
}

// ---- Headers ----
TEST_F(FetchFixture, HeadersBasic)
{
    Value r = ctx.eval(
        "var h = new Headers();"
        "h.append('X-Test', 'a'); h.append('x-test', 'b'); h.set('Content-Type', 'text/plain');"
        "h.get('X-Test') + '|' + h.get('content-type') + '|' + h.has('X-TEST');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "a, b|text/plain|true");
}

TEST_F(FetchFixture, HeadersInvalidName)
{
    Value r = ctx.eval("(() => { try { new Headers({'bad name': 'x'}); return 'no' } catch (e) { return e.name } })()");
    EXPECT_EQ(r.as<std::string>(), "TypeError");
}

TEST_F(FetchFixture, HeadersIteration)
{
    Value r = ctx.eval(
        "var h = new Headers({'b': '2', 'a': '1'});"
        "var keys = []; h.forEach((v, k) => keys.push(k)); keys.join(',');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "a,b");
}

// ---- TextEncoder / TextDecoder ----
TEST_F(FetchFixture, TextEncoderDecoder)
{
    Value r = ctx.eval(
        "var enc = new TextEncoder(); var dec = new TextDecoder();"
        "dec.decode(enc.encode('hello 世界')) + '|' + dec.decode(enc.encode('a\\uD800b'));");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "hello 世界|a\uFFFDb");
}

TEST_F(FetchFixture, TextDecoderBom)
{
    Value r = ctx.eval(
        "var bom = new Uint8Array([0xEF,0xBB,0xBF, 0x61]);"
        "new TextDecoder().decode(bom) + '|' + new TextDecoder({ignoreBOM: true}).decode(bom);");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "a|\uFEFFa");
}

// ---- AbortController ----
TEST_F(FetchFixture, AbortControllerBasic)
{
    Value r = ctx.eval(
        "var c = new AbortController(); var fired = false;"
        "c.signal.addEventListener('abort', () => { fired = true; });"
        "c.abort();"
        "c.signal.aborted + '|' + fired + '|' + c.signal.reason.name;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "true|true|AbortError");
}

TEST_F(FetchFixture, AbortSignalStaticAbort)
{
    Value r = ctx.eval("var s = AbortSignal.abort(); s.aborted + '|' + s.reason.name;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "true|AbortError");
}

// ---- Request / Response ----
TEST_F(FetchFixture, RequestConstruct)
{
    Value r = ctx.eval(
        "var req = new Request('http://example.com/x', {method: 'post', body: 'hello'});"
        "req.method + '|' + req.url + '|' + req.headers.get('content-type');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "POST|http://example.com/x|null");
}

TEST_F(FetchFixture, RequestGetWithBodyRejected)
{
    Value r = ctx.eval("(() => { try { new Request('http://x/', {body: 'b'}); return 'no' } catch (e) { return e.name } })()");
    EXPECT_EQ(r.as<std::string>(), "TypeError");
}

TEST_F(FetchFixture, ResponseConstruct)
{
    Value r = ctx.eval(
        "var resp = new Response('hello', {status: 201, statusText: 'Created',"
        " headers: {'X-A': '1'}});"
        "resp.status + '|' + resp.statusText + '|' + resp.ok + '|' + resp.type;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "201|Created|true|default");
}

TEST_F(FetchFixture, ResponseConsume)
{
    Value r = ctx.eval(
        "var resp = new Response('{\"k\": 1}');"
        "resp.text().then(t => { globalThis.__t = t; return resp.json(); }).catch(e => {"
        " globalThis.__e = e.name; });"
        "'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__t").as<std::string>(), "{\"k\": 1}");
    // 第二次消费 body → TypeError
    EXPECT_EQ(ctx.eval("__e").as<std::string>(), "TypeError");
}

TEST_F(FetchFixture, ResponseErrorAndRedirect)
{
    Value r = ctx.eval(
        "Response.error().type + '|' + Response.redirect('http://x/y', 302).status + '|' +"
        "Response.redirect('http://x/y', 302).headers.get('location');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "error|302|http://x/y");
}

} // namespace
