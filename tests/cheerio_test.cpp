// cheerio_test.cpp —— cheerio 兼容层冒烟测试（lexbor 解析 + 选择器 + 核心 API）
#include <gtest/gtest.h>
#include <qjsbind/cheerio/cheerio.hpp>
#include <qjsbind/qjsbind.hpp>

using namespace qjs;

namespace {

struct CheerioFixture : ::testing::Test {
    Runtime rt;
    Context ctx = rt.main_context();

    CheerioFixture()
    {
        install_ok = qjsbind::cheerio::install_cheerio(ctx);
    }
    bool install_ok = false;
};

// 冒烟：load + 基础选择器 + attr/text/html
TEST_F(CheerioFixture, LoadAndSelectBasics)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(
        "const root = __lexbor_parse('<ul id=\"fruits\"><li class=\"apple\">Apple</li></ul>', true, null);"
        "root.type + '|' + root.children.length + '|' + JSON.stringify(root.children.map(c => c.type + ':' + c.name));");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "root|1|[\"tag:html\"]");
    r = ctx.eval(
        "const $ = cheerio.load('<ul id=\"fruits\"><li class=\"apple\">Apple</li></ul>');"
        "$('li').length + '|' + $('.apple').text() + '|' + $('#fruits').attr('id')"
        "+ '|' + $('li').eq(0).attr('class') + '|' + $('li:first-child').length"
        "+ '|' + $('li:last-child').text();");
    if (r.is_exception()) {
        Value exc(ctx.raw(), JS_GetException(ctx.raw()));
        size_t elen = 0;
        const char* es = JS_ToCStringLen(ctx.raw(), &elen, exc.raw());
        FAIL() << "JS exception: " << (es ? std::string(es, elen) : "(null)");
    }
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "1|Apple|fruits|apple|1|Apple");
}

TEST_F(CheerioFixture, HtmlSerialization)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<div class=\"a\">hello &amp; bye<br><p>x</p></div>');"
        "$.html();");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(),
              "<html><head></head><body><div class=\"a\">hello &amp; bye<br><p>x</p></div></body></html>");
}

TEST_F(CheerioFixture, FragmentLoad)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<li>a</li><li>b</li>', null, false);"
        "$('li').length + '|' + $.html();");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "2|<li>a</li><li>b</li>");
}

TEST_F(CheerioFixture, Manipulation)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<ul><li>a</li></ul>');"
        "$('ul').append('<li>b</li>');"
        "$('li').length + '|' + $.html() + '|' + $('li').last().text();");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(),
              "2|<html><head></head><body><ul><li>a</li><li>b</li></ul></body></html>|b");
}

TEST_F(CheerioFixture, Traversing)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<div><ul id=\"u\"><li class=\"a\">1</li>"
        "<li class=\"b\">2</li></ul></div>');"
        "$('.a').parent().attr('id') + '|' + $('li').first().text()"
        "+ '|' + $('li').last().text() + '|' + $('li').siblings().length"
        "+ '|' + $('ul').children().length + '|' + $('.b').prev().text();");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "u|1|2|2|2|1");
}

TEST_F(CheerioFixture, AttributeOps)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<p class=\"a b\">x</p>');"
        "$('p').addClass('c').hasClass('b') + '|' + $('p').attr('class')"
        "+ '|' + $('p').removeClass('a').attr('class')"
        "+ '|' + $('p').toggleClass('b').attr('class');");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "true|a b c|b c|c");
}

TEST_F(CheerioFixture, ComplexSelectors)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<ul><li data-x=\"1\">a</li><li>"
        "<span>s</span></li></ul>');"
        "$('li[data-x]').length + '|' + $('li:has(span)').length"
        "+ '|' + $('ul > li').length + '|' + $('li + li').length"
        "+ '|' + $('li:contains(s)').length + '|' + $('li:not([data-x])').length;");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "1|1|2|1|1|1");
}

TEST_F(CheerioFixture, StaticHelpers)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<div></div>');"
        "($.html('<b>x</b>') === '<b>x</b>') + '|' + $.parseHTML('<i>y</i>').length"
        "+ '|' + typeof $.merge + '|' + $.root().length;");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "true|1|function|1");
}

TEST_F(CheerioFixture, DocumentStructure)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<!DOCTYPE html><html><head><title>t</title></head>"
        "<body><p>hi</p></body></html>');"
        "$('html').length + '|' + $('title').text() + '|' + $('p').text()"
        "+ '|' + $('body').children().length + '|' + $.html().startsWith('<!DOCTYPE html>');");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "1|t|hi|1|true");
}

} // namespace

TEST_F(CheerioFixture, PreprocessRegex)
{
    Value r = ctx.eval(
        "'li:first-child'.replace(/:first(?!-)\b/g, ':first-child') + '|' + "
        "'li:last-child'.replace(/:last(?!-)\b/g, ':last-child')");
    ASSERT_FALSE(r.is_exception());
    EXPECT_EQ(r.as<std::string>(), "li:first-child|li:last-child");
}

// nth-* 只统计元素子节点（text/comment 不计）——回归：text 位于两个元素
// 之间时（旧公式 len - idx + 1 混合基数会错位），span#s 应匹配
// :nth-last-child(1)/:last-child，且不匹配 :first-child/:nth-child(1)/:only-child
TEST_F(CheerioFixture, NthChildCountsElementsOnly)
{
    Value r = ctx.eval(
        "const $ = cheerio.load('<div><span id=\"a\"></span>text<span id=\"s\"></span></div>');"
        "$('#s').is(':nth-last-child(1)') + '|' + $('#s').is(':first-child')"
        "+ '|' + $('#s').is(':nth-child(1)') + '|' + $('#s').is(':only-child')"
        "+ '|' + $('#s').is(':last-child')");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "true|false|false|false|true");
}

// 安全回归：JS 构造自环对象（children=[自身]）配合 :has/:not/:is 选择器，
// 匹配器的 depth 传递/链步数上限必须快速终止而非栈溢出或死循环。
// 注：kMaxNodes=100000（lexbor_dom.hpp）限制 DFS；自环节点 a 的后代含 a
// 自身（div），故 :has(div)/:is(:has(div)) 全部匹配、:not(:has(div)) 不匹配。
TEST_F(CheerioFixture, SelfCyclePseudoNoCrash)
{
    Value r = ctx.eval(
        "const a = {type:'tag', name:'div', attribs:{}, children:null};"
        "a.children = [a];" // 自环
        "const t0 = Date.now();"
        "const n = __lexbor_queryAll(a, 'div:has(div)', false).length;"
        "const n2 = __lexbor_queryAll(a, 'div:not(:has(div))', false).length;"
        "const n3 = __lexbor_queryAll(a, 'div:is(:has(div))', false).length;"
        // 深层嵌套 :has(:has(...)) 100 层：普通 1 节点树，深度截断快速返回
        "const b = {type:'tag', name:'div', attribs:{}, children:[]};"
        "const root2 = {type:'root', children:[b]};"
        "let sel = 'div';"
        "for (let i = 0; i < 100; i++) sel = ':has(' + sel + ')';"
        "let n4 = -1;"
        "try { n4 = __lexbor_queryAll(root2, sel, false).length; } catch (e) { n4 = -2; }"
        "n + '|' + n2 + '|' + n3 + '|' + n4"
        "+ '|' + (Date.now() - t0 < 30000);");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    EXPECT_EQ(r.as<std::string>(), "100000|0|100000|0|true");
}


