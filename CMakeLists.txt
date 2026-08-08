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
find_package(Boost REQUIRED COMPONENTS asio uuid url)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(unofficial-brotli CONFIG REQUIRED)
find_package(utf8cpp CONFIG REQUIRED)
find_package(stdexec CONFIG REQUIRED)
find_package(qjs CONFIG REQUIRED)
find_package(lexbor CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ---- dart_cpp_bridge：header-only 异步库（co::stream + co::channel）----
add_library(dart_cpp_bridge INTERFACE)
target_include_directories(dart_cpp_bridge INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dart_cpp_bridge INTERFACE STDEXEC::stdexec)

# ---- qjsbind：QuickJS-NG 的 C++ 自动绑定层（header-only）----
add_library(qjsbind INTERFACE)
target_include_directories(qjsbind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(qjsbind INTERFACE qjs Boost::uuid Boost::url utf8cpp::utf8cpp)

# ---- qjsbind_net：fetch 网络层（boost::beast + OpenSSL，静态库）----
add_library(qjsbind_net STATIC src/net/http_client.cpp src/net/socks5.cpp)
if(MSVC)
    target_compile_options(qjsbind_net PRIVATE /utf-8)
endif()
target_include_directories(qjsbind_net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(qjsbind_net PUBLIC
    Boost::asio
    Boost::url
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::brotli::brotlidec
    unofficial::brotli::brotlicommon
    STDEXEC::stdexec
    lexbor::lexbor
)

# ---- 主程序 ----
add_executable(quickjs_runtime src/main.cpp)
target_link_libraries(quickjs_runtime PRIVATE
    qjs
    qjsbind
    qjsbind_net
    Boost::asio
    STDEXEC::stdexec
    fmt::fmt
    spdlog::spdlog
)

# ---- 测试 ----
enable_testing()
add_executable(quickjs_runtime_tests
    tests/test_main.cpp
    tests/stream_test.cpp
    tests/channel_test.cpp
    tests/sleep_test.cpp
    tests/m1_binding_test.cpp
    tests/m2_binding_test.cpp
    tests/m3_binding_test.cpp
    tests/m4_binding_test.cpp
    tests/fetch_test.cpp
    tests/wpt_runner.cpp
    tests/socks5_test.cpp
    tests/cheerio_bench_test.cpp
    tests/cheerio_fast_test.cpp
    tests/mini_task_test.cpp
)
target_link_libraries(quickjs_runtime_tests PRIVATE
    qjs
    qjsbind
    qjsbind_net
    STDEXEC::stdexec
    fmt::fmt
    dart_cpp_bridge
    GTest::gtest_main
    unofficial::brotli::brotlienc
    ZLIB::ZLIB
)

# Debug 构建 JS_CallInternal 栈帧 ~21KB：默认 1MB 线程栈下 ~48 层 JS 递归
# 即崩（QuickJS 栈检查被 __chkstk 抢先）。给测试进程 16MB 栈。
set_target_properties(quickjs_runtime_tests PROPERTIES
    LINK_FLAGS "/STACK:16777216")

include(GoogleTest)
gtest_discover_tests(quickjs_runtime_tests)
