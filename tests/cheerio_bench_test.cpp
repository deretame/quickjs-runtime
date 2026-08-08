// cheerio_bench_test.cpp —— cheerio 解析/查询/序列化性能基准
//
// 用途：方案 A（lexbor C 树句柄）重构前后的对比基线。
// 运行：--gtest_filter="CheerioBench.*" 输出 [bench] 行。
// 数据：3000 行表格（6000 个 td）+ 3 个 li 的 HTML，
//       分别计时 cheerio.load（解析）、$('td.cell')（查询）、
//       $.html()（序列化）、$('tr').filter（回调遍历）。
#include <gtest/gtest.h>
#include <qjsbind/cheerio/cheerio.hpp>
#include <qjsbind/cheerio/lexbor_api.hpp>
#include <qjsbind/loop.hpp>
#include <qjsbind/qjsbind.hpp>

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace qjs;

class CheerioBench : public ::testing::Test {
protected:
    Runtime rt;
    Context ctx = rt.main_context();
    bool install_ok = false;

    CheerioBench() { install_ok = qjsbind::cheerio::install_cheerio(ctx); }
};

TEST_F(CheerioBench, ParseQuerySerialize)
{
    ASSERT_TRUE(install_ok);
    Value r = ctx.eval(R"JS(
        const rows = [];
        for (let i = 0; i < 3000; i++)
            rows.push('<tr id="r' + i + '"><td class="cell">cell ' + i + '</td><td class="num">' + i + '</td></tr>');
        const html = '<table><tbody>' + rows.join('') + '</tbody></table>'
            + '<ul><li class="item">a</li><li class="item">b</li><li class="item">c</li></ul>';
        const t0 = Date.now();
        const $ = cheerio.load(html);
        const t1 = Date.now();
        const n = $('td.cell').length;
        const t2 = Date.now();
        const s = $.html().length;
        const t3 = Date.now();
        const n2 = $('tr').filter(function () { return $(this).attr('id') === 'r1500'; }).length;
        const t4 = Date.now();
        (t1 - t0) + '|' + (t2 - t1) + '|' + (t3 - t2) + '|' + (t4 - t3) + '|' + n + '|' + s + '|' + n2;
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    std::string out = r.as<std::string>();
    std::vector<std::string> parts;
    std::stringstream ss(out);
    std::string item;
    while (std::getline(ss, item, '|'))
        parts.push_back(item);
    ASSERT_EQ(parts.size(), 7u);
    std::printf("[bench] load=%sms query=%sms serialize=%sms filter=%sms | tds=%s htmlLen=%s filterHits=%s\n",
                parts[0].c_str(), parts[1].c_str(), parts[2].c_str(),
                parts[3].c_str(), parts[4].c_str(), parts[5].c_str(),
                parts[6].c_str());
    std::fflush(stdout);
    // 结果正确性断言（基准也必须是正确性回归）
    EXPECT_EQ(parts[4], "3000");   // td.cell 数量（每 tr 一个）
    EXPECT_EQ(parts[6], "1");      // filter 命中 r1500
}

// 方案 A（C 树句柄）同负载基准——对比基线（旧 JS 实现）
TEST_F(CheerioBench, FastCppParseQuerySerialize)
{
    ASSERT_TRUE(install_ok);
    qjsbind::cheerio::lxb_handle::install_cheerio_fast(ctx);
    Value r = ctx.eval(R"JS(
        const rows = [];
        for (let i = 0; i < 3000; i++)
            rows.push('<tr id="r' + i + '"><td class="cell">cell ' + i + '</td><td class="num">' + i + '</td></tr>');
        const html = '<table><tbody>' + rows.join('') + '</tbody></table>'
            + '<ul><li class="item">a</li><li class="item">b</li><li class="item">c</li></ul>';
        const t0 = Date.now();
        const $ = cheerio.load(html);
        const t1 = Date.now();
        const n = $('td.cell').length;
        const t2 = Date.now();
        const s = $.html().length;
        const t3 = Date.now();
        const n2 = $('tr').filter(function () { return $(this).attr('id') === 'r1500'; }).length;
        const t4 = Date.now();
        (t1 - t0) + '|' + (t2 - t1) + '|' + (t3 - t2) + '|' + (t4 - t3) + '|' + n + '|' + s + '|' + n2;
    )JS");
    ASSERT_FALSE(r.is_exception()) << r.as<std::string>();
    std::string out = r.as<std::string>();
    std::vector<std::string> parts;
    std::stringstream ss(out);
    std::string item;
    while (std::getline(ss, item, '|'))
        parts.push_back(item);
    ASSERT_EQ(parts.size(), 7u);
    std::printf("[bench-fast] load=%sms query=%sms serialize=%sms filter=%sms | tds=%s htmlLen=%s filterHits=%s\n",
                parts[0].c_str(), parts[1].c_str(), parts[2].c_str(),
                parts[3].c_str(), parts[4].c_str(), parts[5].c_str(),
                parts[6].c_str());
    std::fflush(stdout);
    EXPECT_EQ(parts[4], "3000");
    EXPECT_EQ(parts[6], "1");
}
