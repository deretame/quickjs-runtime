// cheerio.hpp —— cheerio 兼容层安装入口（qjsbind::cheerio）
//
// install_cheerio(ctx)：
//   1. 注册 lexbor 内部函数（__lexbor_parse / __lexbor_queryAll，见 lexbor_dom.hpp）
//   2. eval cheerio JS bundle（cheerio_js_bundle.hpp 内嵌的 CommonJS 模块集）
//   3. 加载 index.js，导出全局 `cheerio` 对象
//
// JS API 与 cheeriojs/cheerio 保持一致：
//   cheerio.load(html, options?) -> $（可调用 + 静态方法 + 选择器查询）
//   $('selector') / $(htmlString) / $(element) ...
//   $.html() / $.text() / $.root() / $.parseHTML() / $.merge() / $.contains() ...
//   实例方法：attr/prop/class/data/val、find/parents/children/...、
//   append/prepend/after/before/remove/empty/html/text/wrap/...
#pragma once

#include <qjsbind/cheerio/cheerio_js_bundle.hpp>
#include <qjsbind/cheerio/lexbor_dom.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/error.hpp>
#include <qjsbind/value.hpp>

namespace qjsbind::cheerio {

// 安装 cheerio 全局对象。返回值为是否成功（false = bundle 执行异常）。
inline bool install_cheerio(qjs::Context& ctx)
{
    install_lexbor_dom(ctx);

    // 1. eval bundle（注册 __cheerio_require）
    qjs::Value r = ctx.eval(cheerio_bundle_js());
    if (r.is_exception()) {
        qjs::Value exc(ctx.raw(), JS_GetException(ctx.raw()));
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx.raw(), &len, exc.raw());
        std::fprintf(stderr, "[cheerio] bundle eval failed: %.*s\n", (int)len,
                     s ? s : "(null)");
        if (s)
            JS_FreeCString(ctx.raw(), s);
        return false;
    }

    // 2. 加载 index.js 模块
    qjs::Value load_result = ctx.eval("globalThis.__cheerio_require('index.js');");
    if (load_result.is_exception()) {
        qjs::Value exc(ctx.raw(), JS_GetException(ctx.raw()));
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx.raw(), &len, exc.raw());
        std::fprintf(stderr, "[cheerio] load index.js failed: %.*s\n", (int)len,
                     s ? s : "(null)");
        if (s)
            JS_FreeCString(ctx.raw(), s);
        return false;
    }

    ctx.globals().set("cheerio", std::move(load_result));
    return true;
}

} // namespace qjsbind::cheerio
