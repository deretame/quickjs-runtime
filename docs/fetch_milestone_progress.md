# fetch 里程碑进度记录（beast + OpenSSL + Web API + wpt）

> 状态：进行中 · 记录时间：2026-08-06 深夜暂停
> 说明：本文件是工作进度台账，供下次会话直接续接；设计文档另行撰写（见待办 5）。

## 一、已完成并验证

### 1. 依赖与构建链
- `vcpkg.json` 新增：`boost-beast`、`boost-url`、`openssl`、`utfcpp`
  （注意：vcpkg **没有** `beast` 和 `boost-ssl` port；`boost::asio::ssl` 是 header-only，直接链 `OpenSSL::SSL/OpenSSL::Crypto` 即可）
- `CMakeLists.txt`：`find_package(Boost COMPONENTS asio uuid url)`（**不能写 ssl**，会报找不到 boost_ssl）；新增 `qjsbind_net` 静态库 target（`src/net/http_client.cpp`，PUBLIC include 含 `src/` 与 `include/`）；tests 链接 `qjsbind_net`
- `pixi.toml`：新增 `fetch-cacert` 任务（configure 前置）
- `pixi run configure` / `pixi run build` 通过（Boost 1.91.0 + openssl + utfcpp）

### 2. 证书（嵌入）
- `scripts/bootstrap_cacert.py`：下载 Mozilla CA bundle（curl.se/ca/cacert.pem，备源 raw.githubusercontent），生成 `src/net/cacert_embedded.hpp`（182 KiB，`inline constexpr std::string_view embedded_cacert_pem`；已 gitignore）
- `http_client.cpp`：BIO 内存加载 PEM → `X509_STORE`；`shared_ca_store()` 用 `X509_STORE_up_ref` 做进程级共享，每个 `ssl::context` 再 up_ref 后 `SSL_CTX_set_cert_store`（boost 1.91 的 `ssl::context` 是 move-only，不能拷贝共享）

### 3. 网络层（`src/net/`，静态库 qjsbind_net）
- `http_client.hpp`：`HttpRequest/HttpResponse/TlsOptions` + `exec::task<HttpResponse> http_request(io, req, tls, st)`
- `http_client.cpp`：
  - URL 解析用 `boost::urls::parse_uri_reference`（仅 http/https）
  - beast `http::request<string_body>` + `async_write` + `async_read`（string_body 自动解 chunked）
  - TLS：`ssl::stream<tcp::socket>` + `host_name_verification`
  - **取消链路**：`stop_callback` → `socket.cancel()` → asio op 以 `operation_aborted` 完成 → `exec::asio::use_sender` 转 `set_stopped` → 协程 stopped → Promise reject AbortError
- `http_backend.hpp`：`BeastFetchBackend` 实现 `web::FetchBackend`（类型桥接）

### 4. Web API 层（`include/qjsbind/web/`，header-only，命名空间 `qjsbind::web`）
- `net.hpp`：`Header/HttpRequest/HttpResponse` + `FetchBackend` 抽象接口（调用方注入）
- `utf8.hpp`：基于 utf8cpp 的 `utf16_to_utf8`（代理对/孤立替换）、`bytes_to_valid_utf8`、`percent_encode`
- `dom_exception.hpp`：`DOMException`（message/name/code 映射表）
- `events.hpp`：`Event/EventTargetImpl`（listeners 用 `qjs::RtValue` 持有）、`install_event_target_methods` 模板批量注册
- `encoding.hpp`：`TextEncoder`（JS_ToCStringLenUTF16 → utf16_to_utf8，**不能用 JS_ToCString**——quickjs-ng 对孤立代理不替换）、`TextDecoder`（**decode 默认剥离 UTF-8 BOM，`ignoreBOM:true` 才保留**——语义已修）
- `url.hpp`：`URL/URLSearchParams`（boost::urls；相对解析带 base；v1 未做 searchParams 双向联动）
- `abort.hpp`：`AbortController/AbortSignal`——`AbortSignalImpl` 持 `std::stop_source`；`signal_js` 缓存用 RtValue
- `headers.hpp`：`Headers`（guard：none/request/request-no-cors/response；forbidden 头检查；大小写不敏感 + 合并值）
- `request_response.hpp`：`Request/Response`（body：string/ArrayBuffer/TypedArray/URLSearchParams；消费：text/json/arrayBuffer；clone；Response.error/redirect；Request.signal 持 RtValue）
- `timers.hpp`：`setTimeout/setInterval/clearTimeout/clearInterval`（asio steady_timer + 全局注册表 + RtValue 回调）
- `fetch.hpp`：`fetch()` 绑定为 `exec::task` 协程（input/init → RequestImpl → FetchBackend → redirect follow≤20/error/manual → Response）；AbortSignal → stop_token
- `web.hpp`：`install_web_apis(ctx, backend)` 安装入口

### 5. 绑定层增强（qjsbind 核心）
- `class.hpp`：ctor 支持 `Opt<T>` 可选参数（min_args = 非 Opt 数，缺参补 `JS_UNDEFINED`）；新增 **`qjs_init(JSContext*, Args&...)` 构造后初始化扩展点**（有则默认构造 + qjs_init，否则 `new T(args...)`）
- `function.hpp`：`js_convert<Opt<T>>` 特化（undefined/null → 空）
- `convert.hpp`：`js_convert<uint64_t>`（MSVC 下 `uint64_t` 是 `unsigned long long`，与 `int64_t` 特化不匹配）
- `context.hpp`：**gc_mark 支持**（`JSClassDef` 第 3 字段 `class_mark<T>` 模板；T 可定义 `void qjs_mark(JSRuntime*, JS_MarkFunc*)` 标记 opaque 内 JSValue 引用——否则 GC 误判不可达提前回收）；**`~Runtime` 在 `JS_FreeContext` 前加 `JS_RunGC`**（finalizer 在 ctx 存活期执行，opaque 内 RtValue 才能安全释放）
- `rt_value.hpp`（新）：`qjs::RtValue`——存 `JSRuntime* + JSValue`，析构用 `JS_FreeValueRT`（**opaque 内 JSValue 成员必须用它**：finalizer 无 ctx，ctx 在最终 GC 前已释放）；含 `mark()` 供 gc_mark

### 6. 测试基线
- `tests/fetch_test.cpp`：14 个 `FetchFixture` 用例（URL/Headers/TextEncoder/Abort/Request/Response）
- `tests/probe_fetch.cpp`：临时调试探针（**待删除**）
- ctest 共 84 个测试；AbortController 崩溃已修复（gc_mark + JS_RunGC）

### 7. 环境坑（重要）
- **MSVC GBK 代码页**：UTF-8 无 BOM 文件的中文注释里若含 `0x5C` 字节（某些汉字 UTF-8 连续字节），会被当作行继续符吞掉下一行 → 语法错乱。已用 `scripts/add_bom.py` 给 web 层全部文件加 UTF-8 BOM；`bootstrap_cacert.py` 模板已 ASCII 化
- vcpkg 安装的 quickjs.h **无公共 atom 常量**（`JS_ATOM_length` 等都没有）→ 不能直接做 Symbol.iterator 补丁

## 二、当前失败（9 个 fetch 测试）与根因

```
FetchFixture.UrlParseAndProperties / HeadersBasic / HeadersInvalidName / HeadersIteration /
TextDecoderBom / RequestConstruct / ResponseConstruct / ResponseConsume / ResponseErrorAndRedirect
```

1. **JS_ThrowTypeError / JS_NewTypeError 语义不明**（实现不在源码树，`quickjs.c` 里只有使用）：
   当前代码形态 `JS_Throw(ctx, JS_NewTypeError(ctx, ...)); throw qjs::js_error(ctx, JS_GetException(ctx));`
   若 `JS_NewTypeError` 内部已设置 current_exception 并返回 `JS_EXCEPTION` tag，则 `JS_Throw` 会把 tag 覆盖进 current_exception → 异常对象损坏（`HeadersInvalidName` 返回 `"undefined"` 吻合）。
   **修复方向：改用完全可控的组合**：
   ```cpp
   JSValue err = JS_NewError(ctx);
   JS_SetPropertyStr(ctx, err, "name", JS_NewString(ctx, "TypeError"));
   JS_SetPropertyStr(ctx, err, "message", JS_NewString(ctx, msg));
   JS_Throw(ctx, err);
   throw qjs::js_error(ctx, JS_GetException(ctx));
   ```
   涉及 `web/{encoding,fetch,headers,request_response,url}.hpp` 共 33 处；`scripts/fix_throw_usage.py` 可参考（平衡括号替换脚本）。

2. **TextDecoder 构造忽略 options**：`constructor<>()` 无参，`new TextDecoder({ignoreBOM:true})` 的 options 丢失 → BOM 测试 `"a|a"`（应 `"a|\uFEFFa"`）。
   **修复方向**：`TextDecoderImpl` 加 `qjs_init(JSContext*, qjs::Opt<qjs::Value>)` 读构造 options（fatal/ignoreBOM），decode 的 options 参数保留现状。

3. **其余 7 个 eval `is_exception=true`**：疑与问题 1 的异常对象损坏相关，修完 1 后用 probe 确认。

## 三、待办（顺序执行）

1. **修测试至 84/84**：JS_NewError 方案替换 33 处 + TextDecoder 构造 options → `cd build && ./quickjs_runtime_tests.exe --gtest_filter="FetchFixture.*"` 快速迭代 → `pixi run test` 全量
2. **清理**：删 `tests/probe_fetch.cpp` + CMakeLists 引用；删 `scripts/opt_probe.cpp`（若在）
3. **wpt 基础设施**：
   - `scripts/analyze_wpt.py`：扫描 `third_party/wpt/fetch/api` 候选目录，静态检查依赖（Blob/FormData/ReadableStream/Worker/ServiceWorker/.sub 模板/.https → skip + 原因），生成清单 JSON（file、mode(html/anyjs)、meta_scripts、skip、reason）
   - beast mini 测试服务器（tests 内）：echo/status/redirect 端点 + 静态文件（`/resources/testharness.js`、wpt 测试文件）
   - testharness.js 加载器：HTML 提取内嵌 script 执行 / `.any.js` 直接执行 / 处理 `META: script=` 依赖
   - shim：`self`/`window`/`location`/`setTimeout`（已实现）/`EventTarget`（已实现）/`TextEncoder`（已实现）/`URL`（已实现）
4. **wpt 精选子集跑通**（`third_party/wpt` 已 sparse clone：fetch/api + resources/testharness.js + testharnessreport.js + common，30MB）
5. **文档收尾**：`docs/fetch_design.md`（设计文档）、README 更新（含 pixi ≥0.76 与用例数偏差，**需用户确认**）、known_issues 台账、git 提交

## 四、关键文件清单

新增：
- `include/qjsbind/web/`（12 个头文件：net/utf8/dom_exception/events/encoding/url/abort/headers/request_response/timers/fetch/web）
- `include/qjsbind/rt_value.hpp`
- `src/net/http_client.hpp`、`src/net/http_client.cpp`、`src/net/http_backend.hpp`、`src/net/cacert_embedded.hpp`（脚本生成，gitignore）
- `scripts/bootstrap_cacert.py`、`scripts/add_bom.py`、`scripts/fix_throw_usage.py`
- `tests/fetch_test.cpp`、`tests/probe_fetch.cpp`（临时）

修改：
- `vcpkg.json`、`CMakeLists.txt`、`pixi.toml`、`.gitignore`
- `include/qjsbind/class.hpp`、`function.hpp`、`convert.hpp`、`context.hpp`

数据（gitignore）：`third_party/wpt`（sparse clone）、`third_party/vcpkg`（已有）、`build/`

## 五、会话教训

- 别在 `JS_ThrowTypeError` 实现考古上绕圈（曾卡半小时）——直接换可验证的 API 组合
- 每次改完先跑 `cd build && ./quickjs_runtime_tests.exe --gtest_filter="FetchFixture.*"` 快速迭代，最后 `pixi run test` 全量
- 命名空间 `qjsbind::web` 里 `Opt/This/Ctx/Rest` 必须写 `qjs::` 前缀（兄弟命名空间不做隐式查找）
- `try/catch` 是 JS 语句，eval 拿不到值——测试里用 IIFE `(() => { try {...} catch(e) { return e.name } })()`
