// cheerio_spec_runner.cpp —— 跑 cheerio 官方测试套件（转译后的 .spec.js）
//
// 每个 spec 文件在独立 Context 中加载执行（共享 Runtime），vitest API 由
// vendor/vitest.js shim 提供（describe/it/expect 等），结果收集到
// globalThis.__cheerio_tests。本测试不要求全绿：统计通过/失败并打印失败
// 明细，用于迭代提升兼容性。
#include <gtest/gtest.h>
#include <qjsbind/cheerio/cheerio.hpp>
#include <qjsbind/qjsbind.hpp>

#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

using namespace qjs;

namespace {

std::string read_file_or_null(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// 注册 __read_text_file（相对 tests/cheerio/ 根）与 CommonJS 加载器
void install_loader(Context& ctx)
{
    ctx.globals().set(
        "__read_text_file",
        qjs::func(ctx.raw(), [](qjs::Ctx c, std::string path) -> qjs::Value {
            // 安全：规范化后必须仍位于 CHEERIO_TEST_DIR 内（防 .. 逃逸，
            // 兼容 / 与 \ 分隔符——JS 侧 __norm 只处理正斜杠）
            namespace fs = std::filesystem;
            std::error_code ec1, ec2;
            fs::path base = fs::weakly_canonical(fs::path(CHEERIO_TEST_DIR), ec1);
            fs::path full = fs::weakly_canonical(fs::path(CHEERIO_TEST_DIR) / path, ec2);
            // 前缀带分隔符边界：full 必须等于 base，或以 base + '/' + '\\' 开头
            // （防 ../cheerio-evil/x 逃逸到 tests/ 下其它目录）
            const std::string bs = base.string();
            const std::string fs2 = full.string();
            bool inside = !ec1 && !ec2 && (fs2 == bs ||
                                  (fs2.size() > bs.size() &&
                                   fs2.compare(0, bs.size(), bs) == 0 &&
                                   (fs2[bs.size()] == '/' || fs2[bs.size()] == '\\')));
            if (!inside)
                return qjs::Value(c.ctx, JS_NULL);
            std::string content = read_file_or_null(full.string());
            if (content.empty())
                return qjs::Value(c.ctx, JS_NULL);
            return qjs::Value(c.ctx,
                              JS_NewStringLen(c.ctx, content.data(),
                                              (int)content.size()));
        }));
    qjs::Value r = ctx.eval(R"JS(
const __cache = {};
const __vendor = {
  'domhandler': 'vendor/domhandler.js',
  'htmlparser2': 'vendor/htmlparser2.js',
  'domutils': 'vendor/domutils.js',
  'parse5': 'vendor/parse5.js',
  'parse5-htmlparser2-tree-adapter': 'vendor/parse5-htmlparser2-tree-adapter.js',
  'dom-serializer': 'vendor/dom-serializer.js',
  'cheerio-select': 'vendor/cheerio-select.js',
  'encoding-sniffer': 'vendor/encoding-sniffer.js',
  'parse5-parser-stream': 'vendor/parse5-parser-stream.js',
  'whatwg-mimetype': 'vendor/whatwg-mimetype.js',
  'node:stream': 'vendor/node-stream.js',
  'node:http': 'vendor/node-http.js',
  'undici': 'vendor/undici.js',
  'vitest': 'vendor/vitest.js',
};
function __norm(p) {
  const parts = [];
  for (const seg of p.split('/')) {
    if (seg === '' || seg === '.') continue;
    if (seg === '..') { if (parts.length) parts.pop(); }
    else parts.push(seg);
  }
  return parts.join('/');
}
function __dirOf(p) {
  const i = p.lastIndexOf('/');
  return i < 0 ? '' : p.slice(0, i);
}
function __resolve(dir, name) {
  if (name.startsWith('./') || name.startsWith('../')) {
    return __norm(dir + '/' + name);
  }
  if (__vendor[name]) return __vendor[name];
  return name;
}
function __load(dir, name) {
  const id = __resolve(dir, name);
  try { return __cheerio_require(id); } catch (e) { /* bundle miss: fallthrough */ }
  const content = __read_text_file(id);
  if (content === null) throw new Error('cheerio spec: module not found: ' + id);
  if (__cache[id]) return __cache[id].exports;
  const module = { exports: {} };
  __cache[id] = module;
  const fn = new Function('module', 'exports', 'require', content);
  fn(module, module.exports, (n) => __load(__dirOf(id), n));
  return module.exports;
}
globalThis.__cjs_load_spec = (name) => __load('', name);
)JS");
    (void)r;
}

struct SpecResult {
    int pass = 0;
    int fail = 0;
    std::vector<std::string> failures;
};

SpecResult run_spec(Context& ctx, const std::string& spec)
{
    SpecResult out;
    ctx.eval("globalThis.__cheerio_tests = { pass: 0, fail: 0, failures: [] }; 1");
    qjs::Value r = ctx.eval("globalThis.__cjs_load_spec('" + spec + "')");
    if (r.is_exception()) {
        qjs::Value exc(ctx.raw(), JS_GetException(ctx.raw()));
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx.raw(), &len, exc.raw());
        out.fail++;
        out.failures.push_back(std::string("[spec load failed] ") +
                               (s ? std::string(s, len) : "(null)"));
        if (s)
            JS_FreeCString(ctx.raw(), s);
        return out;
    }
    qjs::Value t = ctx.eval("globalThis.__cheerio_tests");
    qjs::Value pass = qjs::Value(ctx.raw(),
                                 JS_GetPropertyStr(ctx.raw(), t.raw(), "pass"));
    qjs::Value fail = qjs::Value(ctx.raw(),
                                 JS_GetPropertyStr(ctx.raw(), t.raw(), "fail"));
    int32_t p = 0, f = 0;
    JS_ToInt32(ctx.raw(), &p, pass.raw());
    JS_ToInt32(ctx.raw(), &f, fail.raw());
    out.pass = p;
    out.fail = f;
    qjs::Value list = ctx.eval(
        "globalThis.__cheerio_tests.failures.map("
        "f => 'FAIL: ' + f.name + '  --  ' + f.message).join('\\n')");
    if (list.is_string()) {
        std::string all = list.as<std::string>();
        size_t pos = 0;
        while ((pos = all.find("FAIL: ", pos)) != std::string::npos) {
            size_t end = all.find('\n', pos);
            if (end == std::string::npos)
                end = all.size();
            out.failures.push_back(all.substr(pos, end - pos));
            pos = end + 1;
        }
    }
    return out;
}

} // namespace

TEST(CheerioSpec, OfficialSuite)
{
    setvbuf(stdout, nullptr, _IONBF, 0); // 崩溃时统计不丢（stdout 无缓冲）
    Runtime rt;
    Context ctx = rt.main_context();
    ASSERT_TRUE(qjsbind::cheerio::install_cheerio(ctx));
    install_loader(ctx);

    const std::vector<std::string> specs = {
        "cheerio.spec.js",
        "load.spec.js",
        "parse.spec.js",
        "static.spec.js",
        "utils.spec.js",
        "index.spec.js",
        "api/attributes.spec.js",
        "api/css.spec.js",
        "api/forms.spec.js",
        "api/extract.spec.js",
        "api/manipulation.spec.js",
        "api/traversing.spec.js",
    };
    // 全部 spec

    int total_pass = 0, total_fail = 0;
    for (const auto& spec : specs) {
        // 每个 spec 独立 Runtime：避免跨 spec 的 JS 对象积累（GC 压力）
        // 引发偶发崩溃，也隔离 spec 间的全局污染。
        Runtime spec_rt;
        Context spec_ctx = spec_rt.main_context();
        if (!qjsbind::cheerio::install_cheerio(spec_ctx)) {
            std::printf("[cheerio-spec] %-28s install failed\n", spec.c_str());
            total_fail += 1000;
            continue;
        }
        install_loader(spec_ctx);
        SpecResult res = run_spec(spec_ctx, spec);
        total_pass += res.pass;
        total_fail += res.fail;
        qjs::Value lastv = spec_ctx.eval("globalThis.__cheerio_tests.last || ''");
        std::printf("[cheerio-spec] %-28s pass=%4d fail=%4d last=%s\n",
                    spec.c_str(), res.pass, res.fail,
                    lastv.is_string() ? lastv.as<std::string>().c_str() : "");
        for (const auto& f : res.failures)
            std::printf("  FAIL: %s\n", f.c_str());
    }
    std::printf("[cheerio-spec] TOTAL pass=%d fail=%d\n", total_pass, total_fail);

    // 兼容性迭代目标：记录进度，不全绿也报（失败明细已打印）
    EXPECT_GT(total_pass, 0);
    EXPECT_LT(total_fail, 400);
}
