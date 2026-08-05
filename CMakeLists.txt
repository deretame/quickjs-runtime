cmake_minimum_required(VERSION 3.25)

project(quickjs_runtime VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 目标平台：Windows 10（boost::asio 等库要求显式定义）
add_compile_definitions(_WIN32_WINNT=0x0A00)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# ---- 第三方库（均由 vcpkg 提供）----
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(stdexec CONFIG REQUIRED)
find_package(qjs CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ---- 主程序 ----
add_executable(quickjs_runtime src/main.cpp)
target_link_libraries(quickjs_runtime PRIVATE
    qjs
    Boost::asio
    STDEXEC::stdexec
    fmt::fmt
    spdlog::spdlog
)

# ---- 测试 ----
enable_testing()
add_executable(quickjs_runtime_tests tests/test_main.cpp)
target_link_libraries(quickjs_runtime_tests PRIVATE
    qjs
    STDEXEC::stdexec
    fmt::fmt
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(quickjs_runtime_tests)
