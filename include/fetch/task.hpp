// fetchcore —— 标准 P2300 task（std::exec::task 参考实现）别名头
//
// 等价于 qjsbind/std_exec.hpp（后者仅 include 本头）：纯 stdexec 别名，
// 无任何 quickjs/qjsbind 依赖。vcpkg stdexec 里两套 task 实现的取舍
// 说明见 qjsbind/std_exec.hpp 原注释（本核心库一律用
// experimental::execution::task，即 std::exec::task 的参考实现）。
#pragma once

#include <exec/task.hpp>

namespace std_exec = ::experimental::execution;
