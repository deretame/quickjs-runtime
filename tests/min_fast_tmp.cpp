// 临时诊断/泄漏测试：C 树版 cheerio（方案 A）
// 每轮 load + 常用操作，JS_RunGC 回收后 Runtime 销毁时不应有对象泄漏
// （quickjs-ng debug assert: gc_obj_list 为空）。
#include <gtest/gtest.h>
#include <qjsbind/cheerio/lexbor_api.hpp>
#include <qjsbind/loop.hpp>
#include <qjsbind/qjsbind.hpp>
using namespace qjs;
TEST(MinLeak, Combos)
{
    Runtime rt;
    Context ctx = rt.main_context();
    qjsbind::cheerio::lxb_handle::install_cheerio_fast(ctx);
    for (int round = 0; round < 50; ++round) {
        Value r = ctx.eval(R"JS(
            globalThis.$g = cheerio.load('<ul id="u"><li class="a">A</li><li>B</li></ul>');
            $g('ul').find('> li').length;
            $g('li').is('.a');
            $g('<em>X</em>').length;
            $g('li').filter(function (i, el) { return i === 0; }).length;
            $g('li').map(function (i, el) { return el; }).length;
            $g('li').each(function () {});
            $g('li').parent().attr('id');
            $g('ul').siblings().length;
            $g('li').next().length;
            $g('li').prev().length;
            $g('li').eq(1).length;
            $g('li').first().length;
            $g('li').last().length;
            (function () {
                var $f = $g('#u');
                var back = $f.eq(1).end();
                if (back !== $f) throw new Error('end() !== $f');
                return 'ok';
            })();
            'ok';
        )JS");
        if (r.is_exception()) {
            Value exc(ctx.raw(), JS_GetException(ctx.raw()));
            std::fprintf(stderr, "[exc] %s\n", exc.as<std::string>().c_str());
            FAIL();
        }
        JS_RunGC(rt.raw()); // 每轮回收旧 $g（含 prevObject 链，需跨轮 GC）
        JS_RunGC(rt.raw());
    }
    std::fprintf(stderr, "[done] 50 rounds ok\n");
}
