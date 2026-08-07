// std_exec.hpp —— 标准 P2300 task（std::exec::task 参考实现）入口
//
// 现状：vcpkg stdexec 2026-05-25（commit fee4d651）里有两套 task 实现：
//   1. stdexec::task（__detail/__task.hpp 的旧实现）——在 MSVC 19.51 下
//      completion signatures 推导失败（C2938，__partitions_of_t 无法特化）。
//   2. experimental::execution::task（exec/task.hpp 的 basic_task 新实现，
//      P2300 最终版协程，带 start scheduler / 调度亲和）——编译正常。
//
// 标准 C++26 的 <execution> 提供 std::exec::task；本项目的 stdexec 实现
// 对应的就是 experimental::execution。这里定义别名 std_exec，
// 代码统一写 std_exec::task<T>（即 std::exec::task 的参考实现）。
#pragma once

#include <exec/task.hpp>

namespace std_exec = ::experimental::execution;
