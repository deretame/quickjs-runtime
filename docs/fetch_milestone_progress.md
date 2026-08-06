# fetch 里程碑进度记录（beast + OpenSSL + Web API + wpt）

> 状态：✅ 已完成 · 收尾时间：2026-08-07
> 说明：本文件是工作进度台账；设计文档见 `docs/fetch_design.md`。

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
  - 默认 Accept/Accept-Language 仅在用户未设置时生效（wpt accept-header 测试）；不设 Connection 头（wpt inspect-headers 语义）
- `http_backend.hpp`：`BeastFetchBackend` 实现 `web::FetchBackend`（类型桥接）

### 4. Web API 层（`include/qjsbind/web/`，header-only，命名空间 `qjsbind::web`）
- `net.hpp`：`Header/HttpRequest/HttpResponse` + `FetchBackend` 抽象接口（调用方注入）
- `errors.hpp`（新）：`throw_type_error` / `throw_range_error`——**唯一异常出口**：JS 全局 TypeError/RangeError 构造器 + message，保证 `instanceof` 与原型链正确（`JS_NewError` 只是普通 Error）
- `utf8.hpp`：基于 utf8cpp 的 `utf16_to_utf8`（代理对/孤立高/低代理 → U+FFFD）、`bytes_to_valid_utf8`、`percent_encode`
- `dom_exception.hpp`：`DOMException`（message/name/code 映射表）
- `events.hpp`：`Event/EventTargetImpl`（listeners 用 `qjs::RtValue` 持有）、`install_event_target_methods` 模板批量注册
- `encoding.hpp`：`TextEncoder`（JS_ToCStringLenUTF16 → utf16_to_utf8）、`TextDecoder`（构造 options：fatal/ignoreBOM；decode 默认剥离 UTF-8 BOM）
- `url.hpp`：`URL/URLSearchParams`（boost::urls；相对解析带 base；**宽松解析**：boost 严格语法拒绝的非 ASCII/`|`/裸 `%` 等按 WHATWG 语义 percent-encode 后重试；**字符串转换走 UTF-16**（孤立代理 → U+FFFD）；searchParams 双向联动未实现）
- `abort.hpp`：`AbortController/AbortSignal`——`AbortSignalImpl` 持 `std::stop_source`；`signal_js` 缓存用 RtValue
- `headers.hpp`：`Headers`（guard：none/request/request-no-cors/response/immutable；forbidden 头静默忽略、immutable 抛 TypeError；**存储为 list of pairs**（同名多值，set-cookie 语义）；迭代 sort+combine（set-cookie 不合并）；getSetCookie()；ByteString 检查（代码点 > U+00FF → TypeError）；method-override 头值含 TRACE/TRACK/CONNECT → 忽略）
- `request_response.hpp`：`Request/Response`（body：string/ArrayBuffer/TypedArray/URLSearchParams/Request/Response 实例复制；消费 text/json/arrayBuffer 为 **Promise**（exec::task）；text() 剥离 BOM；204/205/304 无 body 且带 body → TypeError；status 非法 → RangeError；statusText ByteString 检查；blocked port 检查；headers getter **缓存同一对象**（SameObject 语义）+ fetch 组装前同步；Response.error/redirect；Request.signal 持 RtValue）
- `timers.hpp`：`setTimeout/setInterval/clearTimeout/clearInterval`（asio steady_timer + 全局注册表 + RtValue 回调）
- `fetch.hpp`：`fetch()` 绑定为 `exec::task` 协程（input/init → RequestImpl → FetchBackend → redirect follow≤20/error/manual → Response，type="basic"）；AbortSignal → stop_token
- `web.hpp`：`install_web_apis(ctx, backend)` 安装入口（Headers/URLSearchParams 的 Symbol.iterator 用 JS 侧补丁）

### 5. 绑定层增强（qjsbind 核心）
- `class.hpp`：ctor 支持 `Opt<T>` 可选参数；`qjs_init(JSContext*, Args&...)` 构造后初始化扩展点
- `function.hpp`：`js_convert<Opt<T>>` 特化（undefined/null → 空）；**`Opt<Value>` 全特化**（null 保留——`new Headers(null)` 需抛 TypeError）
- `error.hpp`：`js_error` 加拷贝构造（`JS_DupValue` 深拷贝）——**MSVC 协程异常传播路径对 move-only 异常类型处理损坏**（崩溃根因）
- `promise.hpp`：`exception_to_js` 的 js_error 分支去掉多余 `JS_DupValue`（引用泄漏）
- `context.hpp`：gc_mark 支持；`~Runtime` 在 `JS_FreeContext` 前加 `JS_RunGC`
- `rt_value.hpp`（新）：`qjs::RtValue`

### 6. 测试基线
- `tests/fetch_test.cpp`：17 个 `FetchFixture` 用例 + 4 个回归用例（method-override/非 ASCII URL/孤立代理/迭代器原型链）
- `tests/wpt_runner.cpp` + `tests/wpt_server.hpp` + `tests/wpt_shim.js`（新）：wpt 精选子集运行器
- `scripts/analyze_wpt.py`（新）：扫描 `third_party/wpt/fetch/api`，静态检查依赖（未实现 API/端点/.sub/.https → skip + 原因），生成 `build/wpt_manifest.json` + `build/wpt_tests.txt`
- 全量 ctest：**89 个测试全绿**；wpt：**30/30 文件全过、510 pass / 0 fail、9 expected**（已知 v1 限制，见设计文档）

### 7. 环境坑（重要）
- **MSVC GBK 代码页**：UTF-8 无 BOM 文件的中文注释里若含 `0x5C` 字节，会被当作行继续符吞掉下一行 → 语法错乱。已用 `scripts/add_bom.py` 给 web 层全部文件加 UTF-8 BOM
- vcpkg 安装的 quickjs.h **无公共 atom/类常量** → Symbol.iterator 等用 JS 侧补丁（注意：**拼接的 JS 补丁字符串里不能写 `//` 行注释**——无换行会吞掉后续代码）
- **std::ifstream 打不开混合分隔符路径**（`D:\...\runtime/third_party/...`）→ 文件读取用 `std::fopen`
- **Windows 上被拒端口静默丢包**（connect 挂 2s+）→ 必须实现 blocked-port 构造检查
- **parse_uri_reference 返回的 url_view 借用输入字符串**——relax 临时字符串必须先提升到函数作用域再使用（悬垂 → 断言/崩溃）

## 二、本次会话修复清单（原 9 个 fetch 测试失败 → 全绿）

1. **真实根因**：`JS_ThrowTypeError` 批量替换时 `throw qjs::js_error(...)` 语句**脱离 if 控制流**（无条件执行，包装 `JS_UNINITIALIZED` 伪异常 → `HeadersInvalidName` 返回 "undefined"）。修复：新建 `errors.hpp::throw_type_error`（完全可控组合），替换 5 文件 33 处
2. **js_error move-only + MSVC 协程异常传播 → SEH 崩溃**：加拷贝构造（`JS_DupValue` 深拷贝）
3. **exception_to_js 引用泄漏**：去掉多余 `JS_DupValue`
4. **TextDecoder 构造 options**：`qjs_init(JSContext*, qjs::Opt<qjs::Value>)`
5. **Response.text()/json()/arrayBuffer() 应为 Promise**：方法改返回 `exec::task<qjs::Value>`
6. **`qjs::Array pair(ctx, item.raw())` 双重释放**（借用值被接管）：改 `std::move(item)` / `JS_DupValue`
7. **Response/Request 的 `body` getter**（恒 null，v1 无流）
8. **fetch 响应 type="basic"**；**headers 缓存 SameObject** + fetch 组装前同步
9. **Headers 存储改 list of pairs**（set-cookie 多值语义）、guard 语义修正（response/immutable）、getSetCookie、迭代器 JS 补丁（%IteratorPrototype% 链）、forEach 第三参数 SameObject
10. **wpt 跑通期间的实现补全**：blocked port、forbidden method-override 头值、URL 宽松解析 + UTF-16 转换、ByteString 检查、integrity/204-205-304/statusText 校验、默认 Accept/Accept-Language、服务器 .asis/HEAD/多同名头、`promise_rejects_js(test, ctor, promise)` 签名等

## 三、wpt 精选子集（30/30 文件全过，510 pass / 9 expected）

运行方式：
```bash
python scripts/analyze_wpt.py      # 重新生成清单（third_party/wpt 变更后）
cd build && ./quickjs_runtime_tests.exe --gtest_filter="WptRunner.*"
```

清单：`build/wpt_tests.txt`（file/mode/meta_scripts）；运行器对每个文件建独立 Runtime，
shim 提供 testharness 兼容层（test/promise_test/assert_*/promise_rejects_js/add_cleanup），
服务器提供 status/redirect/inspect-headers/method/echo 端点 + `.asis` 原样响应 + 静态文件。

**9 个 expected（v1 已知限制，shim 内登记）**：
- 活迭代器语义 6 个（快照迭代器：迭代中删除/追加元素不反映）
- `Escaping produces double-percent`（裸 `%` 在 query 中的 WHATWG 保留语义，boost 无法表示）
- `Ensure the correct JSON parser is used`（`data:` URL fetch 未实现）
- `Create headers with existing headers with custom iterator`（Headers 实例构造走内部拷贝，不走自定义迭代器）

## 四、关键文件清单

新增：
- `include/qjsbind/web/`（13 个头文件：net/utf8/errors/dom_exception/events/encoding/url/abort/headers/request_response/timers/fetch/web）
- `include/qjsbind/rt_value.hpp`
- `src/net/http_client.hpp`、`src/net/http_client.cpp`、`src/net/http_backend.hpp`、`src/net/cacert_embedded.hpp`（脚本生成，gitignore）
- `scripts/bootstrap_cacert.py`、`scripts/add_bom.py`、`scripts/analyze_wpt.py`
- `tests/fetch_test.cpp`、`tests/wpt_runner.cpp`、`tests/wpt_server.hpp`、`tests/wpt_shim.js`

修改：
- `vcpkg.json`、`CMakeLists.txt`、`pixi.toml`、`.gitignore`
- `include/qjsbind/class.hpp`、`function.hpp`、`convert.hpp`、`context.hpp`、`error.hpp`、`promise.hpp`
- `src/net/http_client.cpp`

数据（gitignore）：`third_party/wpt`（sparse clone：fetch/api + xhr/resources + resources/testharness.js + common）、`third_party/vcpkg`、`build/`

## 五、会话教训

- 别在 `JS_ThrowTypeError` 实现考古上绕圈——直接换可验证的 API 组合（且真实根因是 throw 脱离 if 控制流）
- 失败症状"异常损坏/undefined"≠ 实现语义问题，先怀疑**控制流被脚本批量替换破坏**
- MSVC 协程 + move-only 异常类型是雷区；JSValue 跨异常传播必须深拷贝
- `url_view` 借用字符串：relax 临时量提升作用域
- JS 侧补丁字符串禁止 `//` 注释（无换行拼接会吞代码）
- Windows：ifstream 混合路径、被拒端口 connect 超时、CRLF 残留（`getline` 的 `\r`）都是隐形坑
- 每次改完先跑 `cd build && ./quickjs_runtime_tests.exe --gtest_filter="FetchFixture.*:WptRunner.*"` 快速迭代，最后 `pixi run test` 全量
