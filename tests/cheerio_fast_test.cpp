// cheerio_fast_test.cpp —— 方案 A：lexbor C 树句柄基础设施测试
#include <gtest/gtest.h>
#include <qjsbind/cheerio/lexbor_api.hpp>
#include <qjsbind/loop.hpp>
#include <qjsbind/qjsbind.hpp>

using namespace qjs;

namespace {

struct CheerioFastFixture : ::testing::Test {
    Runtime rt;
    Context ctx = rt.main_context();
    bool install_ok = false;

    CheerioFastFixture()
    {
        try {
            qjsbind::cheerio::lxb_handle::install_cheerio_fast(ctx);
            install_ok = true;
        } catch (...) {
            install_ok = false;
        }
    }
};

// 惰性属性：domhandler 兼容的 children/name/attribs/data 访问
TEST_F(CheerioFastFixture, HandleBasics)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        const $ = __lxb_load('<ul id="fruits"><li class="apple">Apple</li></ul>');
        const root = $[0];
        const body = root.children[0].children[1]; // html -> body
        const ul = body.children[0];
        const li = ul.children[0];
        root.type + '|' + root.children.length + '|' + root.children[0].name
        + '|' + body.name + '|' + ul.name + '|' + li.name
        + '|' + li.attribs['class'] + '|' + ul.attribs.id
        + '|' + li.children[0].data + '|' + li.children[0].type
        // 句柄模式：parent 返回新句柄对象（非同一引用）；语义比较用 name
        + '|' + (li.parent.name === 'ul') + '|' + (li.prev === null);
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(),
              "root|1|html|body|ul|li|apple|fruits|Apple|text|true|true");
}

// 生命周期：$ 被 GC 后，句柄仍持有文档可读；多次 load + GC 无崩溃
TEST_F(CheerioFastFixture, GcLifecycle)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        // 块内 $ 释放后 root 句柄仍可读
        let root;
        {
            const $ = __lxb_load('<p>keep</p>');
            root = $[0];
        }
        // 批量 load 制造垃圾文档
        for (let i = 0; i < 50; i++) __lxb_load('<div>g' + i + '</div>');
        const kept = root.children[0].children[1].children[0].name;
        kept;
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "p");
    // 强制 GC 后再操作：无悬垂、无崩溃
    JS_RunGC(JS_GetRuntime(ctx.raw()));
    r = ctx.eval("__lxb_load('<span>after</span>')[0].children[0].children[1]"
                 ".children[0].name");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "span");
}

// C 树选择器匹配：id/class/组合/伪类/属性
TEST_F(CheerioFastFixture, SelectorMatching)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        const $ = __lxb_load(
            '<ul id="fruits"><li class="apple">Apple</li>' +
            '<li class="orange">Orange</li><li class="pear">Pear</li></ul>' +
            '<table><tr><td class="num">1</td><td class="cell">a</td></tr>' +
            '<tr><td class="num">2</td><td class="cell">b</td></tr></table>');
        const names = (sel) => {
            const r = $(sel);
            let out = [];
            for (let i = 0; i < r.length; i++) out.push(r[i].name + '.' + r[i].attribs['class']);
            return out.join(',');
        };
        names('li') + '|' + names('li.apple') + '|' + names('ul > li')
        + '|' + names('td.cell') + '|' + names('li:first-child')
        + '|' + names('li:last-child') + '|' + names('td:nth-child(2)')
        + '|' + names('td[class]') + '|' + names('#fruits');
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(),
              "li.apple,li.orange,li.pear|li.apple|li.apple,li.orange,li.pear|"
              "td.cell,td.cell|li.apple|li.pear|td.cell,td.cell|"
              "td.num,td.cell,td.num,td.cell|ul.undefined");
}

// $(node) 包装 + 数组包装
TEST_F(CheerioFastFixture, WrapNodeAndArray)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        const $ = __lxb_load('<div><p>a</p><p>b</p></div>');
        const p0 = $('p')[0];
        const wrapped = $(p0);
        const arr = $([p0, $('p')[1]]);
        wrapped.length + '|' + wrapped[0].children[0].data
        + '|' + arr.length + '|' + arr[1].children[0].data;
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "1|a|2|b");
}

// cheerio.load + API：查询/属性/文本/序列化/class/修改
TEST_F(CheerioFastFixture, ApiBasics)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        const $ = cheerio.load(
            '<ul id="fruits"><li class="apple">Apple</li>' +
            '<li class="orange">Orange</li></ul>');
        const li = $('li');
        const apple = $('li.apple');
        const t1 = apple.attr('class');
        apple.attr('data-x', '42');
        const t2 = apple.attr('data-x');
        const t3 = apple.text();
        const t4 = li.length;
        apple.addClass('red');
        const t5 = apple.hasClass('red');
        apple.removeClass('apple');
        const t6 = apple.attr('class');
        const t7 = $('li').filter('.orange').text();
        const t8 = $('li').first().text();
        const t9 = $('li').last().text();
        const t10 = $('ul').find('li').length;
        const t11 = $('li').parent().attr('id');
        apple.append('<em>X</em>');
        const t12 = $('li.red em').length;
        const t14 = $('ul').html().indexOf('em') >= 0;
        $('li.red').remove();
        const t13 = $('li').length;
        t1 + '|' + t2 + '|' + t3 + '|' + t4 + '|' + t5 + '|' + t6
        + '|' + t7 + '|' + t8 + '|' + t9 + '|' + t10 + '|' + t11
        + '|' + t12 + '|' + t13 + '|' + t14;
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(),
              "apple|42|Apple|2|true|red|Orange|Apple|Orange|2|fruits|1|1|true");
}

} // namespace
