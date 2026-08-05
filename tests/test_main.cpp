#include <gtest/gtest.h>
#include <quickjs.h>

#include <stdexec/execution.hpp>

TEST(Quickjs, Eval) {
    JSRuntime* rt = JS_NewRuntime();
    ASSERT_NE(rt, nullptr);
    JSContext* ctx = JS_NewContext(rt);
    ASSERT_NE(ctx, nullptr);

    JSValue result = JS_Eval(ctx, "40 + 2", 6, "<test>", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(result));
    int32_t value = 0;
    ASSERT_EQ(JS_ToInt32(ctx, &value, result), 0);
    EXPECT_EQ(value, 42);

    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

TEST(Stdexec, Just) {
    auto result = stdexec::sync_wait(stdexec::just(7));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(*result), 7);
}
