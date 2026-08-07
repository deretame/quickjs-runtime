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
#include "wpt_server.hpp"

#include <fstream>
#include <sstream>

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
    EXPECT_EQ(r.as<std::string>(), "POST|http://example.com/x|text/plain;charset=UTF-8");
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

TEST_F(FetchFixture, HeaderMethodOverride)
{
    Value r = ctx.eval(
        "var r = new Request('https://site.example/');"
        "r.headers.append('x-http-method-override', 'GETTRACE');"
        "r.headers.get('x-http-method-override') + '|' + r.headers.has('x-http-method-override');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "GETTRACE|true");
}

TEST_F(FetchFixture, HeadersLiveIteration)
{
    // 活迭代器：迭代期间 delete/append 实时反映（wpt headers-basic 语义）
    Value r = ctx.eval(
        "var h = new Headers({'foo': '2', 'baz': '1', 'BAR': '0'});"
        "var k = []; for (const [n, v] of h) { k.push(n); h.delete('foo'); }"
        "k.join(',');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "bar,baz");
    r = ctx.eval(
        "var h = new Headers({'foo': '2', 'baz': '1', 'BAR': '0', 'quux': '3'});"
        "var k = []; for (const [n, v] of h) { k.push(n); if (n === 'baz') h.delete('bar'); }"
        "k.join(',');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "bar,baz,quux");
    r = ctx.eval(
        "var h = new Headers({'foo': '2', 'baz': '1', 'BAR': '0', 'quux': '3'});"
        "var k = []; for (const [n, v] of h) { k.push(n); if (n === 'baz') h.append('abc', '-1'); }"
        "k.join(',');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "bar,baz,baz,foo,quux");
    // 迭代器原型链 + next 描述符（wpt checkIteratorProperties）
    r = ctx.eval(
        "var it = new Headers({'a': '1'}).entries();"
        "var p = Object.getPrototypeOf(it);"
        "var d = Object.getOwnPropertyDescriptor(p, 'next');"
        "(Object.getPrototypeOf(p) === Object.getPrototypeOf(Object.getPrototypeOf([].values())))"
        " + '|' + d.enumerable + '|' + d.configurable + '|' + d.writable;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "true|true|true|true");
}

TEST_F(FetchFixture, RequestNonAsciiUrl)
{
    Value r = ctx.eval(
        "var r = new Request('http://x/y?z=1|x', {headers: {'X-Test': 'before-ß-after'}});"
        "r.url + '|' + r.headers.get('X-Test');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "http://x/y?z=1%7Cx|before-ß-after");
}

// 参照 Node(undici)：referer/cookie/origin 等用户自定义头可正常存储与发送
//（不做浏览器式 forbidden 过滤）；host/content-length 由运行时管理（忽略）。
TEST_F(FetchFixture, RequestRefererCustomHeader)
{
    Value r = ctx.eval(
        "var req = new Request('http://x/', {headers: {"
        "  'Referer': 'http://example.com/ref', 'Cookie': 'a=b',"
        "  'Origin': 'http://custom-origin.com', 'Host': 'evil.com',"
        "  'Content-Length': '999'"
        "}});"
        "['referer','cookie','origin','host','content-length'].map("
        "  n => req.headers.get(n)).join('|');");
    ASSERT_FALSE(r.is_exception());
    // 存储层不检查（Node Headers 语义）；host/content-length 在发送层忽略
    EXPECT_EQ(r.as<std::string>(),
              "http://example.com/ref|a=b|http://custom-origin.com|evil.com|999");
}

TEST_F(FetchFixture, RequestSurrogateUrl)
{
    ctx.eval("location = {href: 'http://x/url-encoding.html'}");
    Value r = ctx.eval(
        "var u1 = new URL('?\\uD83D', location.href).href;"
        "var r1 = new Request('?\\uD83D').url;"
        "u1 + '|' + r1;");
    if (r.is_exception()) {
        qjs::Value exc(ctx.raw(), JS_GetException(ctx.raw()));
        const char* s = JS_ToCString(ctx.raw(), exc.raw());
        std::fprintf(stderr, "EXC: %s\n", s ? s : "(null)");
        if (s)
            JS_FreeCString(ctx.raw(), s);
    }
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "http://x/url-encoding.html?%EF%BF%BD|http://x/url-encoding.html?%EF%BF%BD");
}

TEST_F(FetchFixture, FetchIntegrity)
{
    qjsbind::net::wpt::WptTestServer server("third_party/wpt");
    const std::string base = server.base_url();
    ctx.eval("var base = '" + base + "';");
    // 正确摘要 → 成功（'hello world' 的 sha384 = /b2OdaZ/KfcBpOBAOF4uI5hjA+oQI5IRr5B/y7g1eLPkF8txzmRu/QgZ3YwIjeG9）
    Value r = ctx.eval(
        "fetch(base + '/echo-content.py', {method: 'POST', body: 'hello world',"
        " integrity: 'sha384-/b2OdaZ/KfcBpOBAOF4uI5hjA+oQI5IRr5B/y7g1eLPkF8txzmRu/QgZ3YwIjeG9'})"
        ".then(x => { globalThis.__ok = x.status; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__ok").as<int>(), 200);
    // 错误摘要 → reject TypeError
    r = ctx.eval(
        "fetch(base + '/echo-content.py', {method: 'POST', body: 'hello world',"
        " integrity: 'sha384-' + 'A'.repeat(64)}).then(() => { globalThis.__bad = 'no'; })"
        ".catch(e => { globalThis.__bad = e.name; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__bad").as<std::string>(), "TypeError");
    // 204 null body + integrity → reject TypeError（wpt response-null-body 语义）
    r = ctx.eval(
        "fetch(base + '/status?code=204', {integrity: 'sha384-UT6f7WCFp32YJnp1is4l/ZYnOeQKpE8xjmdkLOwZ3nIP+tmT2aMRFQGJomjVf5cE'})"
        ".then(() => { globalThis.__n = 'no'; })"
        ".catch(e => { globalThis.__n = e.name; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__n").as<std::string>(), "TypeError");
    // 构造时非法元数据 → TypeError
    r = ctx.eval("new Request(base + '/echo-content.py', {integrity: 'bogus'});");
    EXPECT_TRUE(r.is_exception());
    r = ctx.eval("new Request(base + '/echo-content.py', {integrity: 'md5-abc'});");
    EXPECT_TRUE(r.is_exception());
    // 空串合法（不校验）
    r = ctx.eval("new Request(base + '/echo-content.py', {integrity: ''}).integrity;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "");
}

TEST_F(FetchFixture, FetchDataUrl)
{
    Value r = ctx.eval(
        "var out = [];"
        "fetch('data:,response%27s%20body').then(x => {"
        " out.push(x.status + '|' + x.headers.get('content-type')); return x.text(); })"
        ".then(t => { out.push(t); });"
        "fetch('data:text/plain;base64,cmVzcG9uc2UncyBib2R5').then(x => {"
        " out.push(x.headers.get('content-type')); return x.text(); })"
        ".then(t => { out.push(t); });"
        "fetch('data:image/png;base64,AAAA').then(x => {"
        " out.push(x.headers.get('content-type')); return x.arrayBuffer(); })"
        ".then(b => { out.push(new Uint8Array(b).length); });"
        "fetch('data:notAdataUrl.com').then(() => { out.push('no'); })"
        ".catch(e => { out.push(e.name); });"
        "globalThis.__out = out; 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__out.join(',')").as<std::string>(),
              "200|text/plain;charset=US-ASCII,text/plain,image/png,TypeError,"
              "response's body,response's body,3");
}

TEST_F(FetchFixture, FetchDataUrlHead)
{
    Value r = ctx.eval(
        "fetch('data:,hello', {method: 'HEAD'}).then(x => {"
        " globalThis.__h = x.status + '|' + x.headers.get('content-type'); return x.text(); })"
        ".then(t => { globalThis.__h += '|' + t; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__h").as<std::string>(), "200|text/plain;charset=US-ASCII|");
}

TEST_F(FetchFixture, UrlSearchParamsLinkage)
{
    // SameObject：多次 getter 同一对象
    Value r = ctx.eval(
        "var u = new URL('http://x/a?b=1');"
        "u.searchParams === u.searchParams;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<bool>(), true);
    // params 修改 → URL.search 实时回写
    r = ctx.eval(
        "var u = new URL('http://x/a?b=1&c=2');"
        "var p = u.searchParams;"
        "p.append('d', 'x y'); p.set('b', '9'); p.delete('c'); p.sort();"
        "u.search + '|' + p.toString();");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "?b=9&d=x+y|b=9&d=x+y");
    // URL.search 修改 → 已缓存的 params 同步（live）
    r = ctx.eval(
        "var u = new URL('http://x/a?b=1');"
        "var p = u.searchParams;"
        "u.search = '?x=42&y=7';"
        "p.get('x') + '|' + p.get('b') + '|' + p.toString();");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "42|null|x=42&y=7");
    // href 整体重设 → 同步
    r = ctx.eval(
        "var u = new URL('http://x/a?b=1');"
        "var p = u.searchParams;"
        "u.href = 'http://x/z?q=9';"
        "p.get('q') + '|' + p.get('b');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "9|null");
    // 独立实例不联动（new URLSearchParams(url.searchParams) 是拷贝）
    r = ctx.eval(
        "var u = new URL('http://x/a?b=1');"
        "var p2 = new URLSearchParams(u.searchParams);"
        "p2.set('b', '2');"
        "u.search + '|' + p2.toString();");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "?b=1|b=2");
}

TEST_F(FetchFixture, BlobBasic)
{
    // 构造（string/ArrayBuffer/TypedArray/Blob parts）+ size/type + slice + text/arrayBuffer
    Value r = ctx.eval(
        "var b = new Blob(['hello ', new Uint8Array([0xE4, 0xB8, 0x96]), new ArrayBuffer(0)],"
        " {type: 'text/PLAIN;charset=utf-8'});"
        "b.size + '|' + b.type;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "9|text/plain");
    r = ctx.eval(
        "var b = new Blob(['abcdef']);"
        "b.slice(1, 4).text().then(t => { globalThis.__sl = t; });"
        "b.slice(-3).text().then(t => { globalThis.__sl += '|' + t; });"
        "'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__sl").as<std::string>(), "bcd|def");
    r = ctx.eval(
        "var b = new Blob([new Uint8Array([1, 2, 3])]);"
        "b.arrayBuffer().then(ab => { globalThis.__ab = new Uint8Array(ab).length; });"
        "'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__ab").as<int>(), 3);
    // BOM：text() 去 BOM
    r = ctx.eval(
        "var b = new Blob([new Uint8Array([0xEF, 0xBB, 0xBF, 0x61])]);"
        "b.text().then(t => { globalThis.__bom = t; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__bom").as<std::string>(), "a");
}

TEST_F(FetchFixture, BlobFile)
{
    Value r = ctx.eval(
        "var f = new File(['content'], 'a.txt', {type: 'text/plain', lastModified: 123});"
        "f.name + '|' + f.size + '|' + f.type + '|' + f.lastModified;");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "a.txt|7|text/plain|123");
    // File 作为 fetch body（Content-Type 自动设置）
    r = ctx.eval(
        "var f = new File(['x'], 'b.bin', {type: 'application/octet-stream'});"
        "new Request('http://x/', {method: 'POST', body: f}).headers.get('content-type');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "application/octet-stream");
    // Blob 作为 Response 构造 body（Content-Type 自动）
    r = ctx.eval(
        "var b = new Blob(['{\"k\":1}'], {type: 'application/json'});"
        "new Response(b).headers.get('content-type');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "application/json");
}

TEST_F(FetchFixture, FormDataBasic)
{
    Value r = ctx.eval(
        "var fd = new FormData();"
        "fd.append('a', '1'); fd.append('a', '2'); fd.append('b', new Blob(['x'], {type: 'text/x'}));"
        "fd.set('a', '9');"
        "fd.has('a') + '|' + fd.get('a') + '|' + fd.getAll('a').join(',') + '|' + fd.has('b');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "true|9|9|true");
    r = ctx.eval(
        "var fd = new FormData();"
        "fd.append('k1', 'v1'); fd.append('k2', new File(['b2'], 'f2.txt', {type: 'text/plain'}));"
        "var out = []; for (const [k, v] of fd) { out.push(k + '=' + (typeof v === 'string' ? v : v.name)); }"
        "fd.delete('k1'); out.join(',') + '|' + fd.has('k1');");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "k1=v1,k2=f2.txt|false");
    // multipart 编码：boundary + Content-Disposition + Content-Type
    r = ctx.eval(
        "var fd = new FormData();"
        "fd.append('n', 'v a'); fd.append('f', new File(['xy'], 'x.txt', {type: 'text/plain'}));"
        "var req = new Request('http://x/', {method: 'POST', body: fd});"
        "String(req.headers.get('content-type'));");
    ASSERT_FALSE(r.is_exception());
    std::fprintf(stderr, "CT=%s\n", r.as<std::string>().c_str());
    EXPECT_TRUE(r.as<std::string>().rfind("multipart/form-data; boundary=", 0) == 0);
}

TEST_F(FetchFixture, FormDataRoundTrip)
{
    // 空 FormData 往返
    Value r = ctx.eval(
        "new Response(new FormData()).formData().then(fd => {"
        " globalThis.__e = fd instanceof FormData; })"
        ".catch(e => { globalThis.__e = 'ERR:' + e.name + ':' + e.message; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("String(__e)").as<std::string>(), "true");
    // multipart 往返：string + File（type 保留）
    r = ctx.eval(
        "var fd = new FormData();"
        "fd.append('foo', 'bar');"
        "fd.append('file', new File(['{\"a\":1}'], 'j.json', {type: 'application/json'}));"
        "new Response(fd).formData().then(fd2 => {"
        " globalThis.__n = fd2.get('foo');"
        " var f = fd2.get('file');"
        " globalThis.__t = f.name + '|' + f.type + '|' + f.size;"
        " return f.text(); }).then(t => { globalThis.__b = t; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__n").as<std::string>(), "bar");
    EXPECT_EQ(ctx.eval("__t").as<std::string>(), "j.json|application/json|7");
    EXPECT_EQ(ctx.eval("__b").as<std::string>(), "{\"a\":1}");
    // 头名大小写不敏感（wpt formdata.any.js：小写 content-disposition 也能解析）
    r = ctx.eval(
        "var fd = new FormData();"
        "fd.append('foo', new Blob(['x'], {type: 'application/json'}));"
        "var r1 = new Response(fd);"
        "r1.formData().then(fd2 => {"
        " globalThis.__m = fd2.has('foo') + '|' + fd2.get('foo').type; }); 'ok'");
    ASSERT_FALSE(r.is_exception());
    rt.run_to_completion();
    EXPECT_EQ(ctx.eval("__m").as<std::string>(), "true|application/json");
}

} // namespace
