# quickjs-runtime

C++20 项目骨架：内嵌 **quickjs-ng** JavaScript 引擎，集成 **boost::asio** 与 **stdexec** 执行模型，
辅以 **fmt** / **spdlog** 日志与 **gtest** 测试。

## 工具链

| 工具 | 说明 |
| --- | --- |
| pixi | Python 环境管理器，驱动所有脚本（python 3.13.14） |
| cmake + ninja | 由 pixi 环境提供（`pixi run` 时自动进入 PATH） |
| MSVC | Visual Studio 18 Community 的 cl.exe（脚本通过 vswhere 自动定位 vcvars64） |
| vcpkg | 固定 release tag `2026.07.29`，克隆到 `third_party/vcpkg`（由脚本管理，不纳入版本控制） |

## 版本固定

- **vcpkg 本体**：`scripts/bootstrap_vcpkg.py` 中 `VCPKG_TAG` 固定克隆/校验 release tag `2026.07.29`，防止 master 漂移。
- **依赖版本**：`vcpkg.json` 的 `builtin-baseline` 记录该 tag 对应的 commit（`9e593bb1`），所有依赖按 baseline 锁定解析；升级需显式修改两者。

## 快速开始

```bash
# 1. 安装 pixi 环境（python 3.13.14 / cmake / ninja）
pixi install

# 2. 克隆并 bootstrap vcpkg（自动执行）
pixi run setup-vcpkg

# 3. 配置 CMake（MSVC x64 + Ninja + vcpkg toolchain，Debug）
pixi run configure

# 4. 构建
pixi run build

# 5. 运行测试
pixi run test
```

`configure` / `build` / `test` 之间有依赖串联：`pixi run test` 会依次执行 setup-vcpkg → configure → build → test。

构建产物位于 `build/`。Release 配置：

```bash
pixi run python scripts/configure.py --build-type Release
pixi run build
```

## 目录结构

```
├── CMakeLists.txt          # CMake 工程（C++20）
├── pixi.toml               # pixi 环境与任务定义
├── vcpkg.json              # vcpkg 依赖清单
├── scripts/                # Python 脚本（pixi run 调用）
│   ├── bootstrap_vcpkg.py  # 克隆 + bootstrap vcpkg
│   ├── vs_env.py           # 定位 MSVC 环境（vswhere + vcvars64）
│   ├── configure.py        # CMake 配置
│   ├── build.py            # 构建
│   └── test.py             # ctest
├── src/main.cpp            # 主程序（quickjs-ng + asio + stdexec demo）
└── tests/test_main.cpp     # gtest 测试
```

## 依赖

- [quickjs-ng](https://github.com/quickjs-ng/quickjs-ng) — 内嵌 JS 引擎
- [boost-asio](https://www.boost.org/doc/libs/release/libs/asio/) — 异步 I/O
- [stdexec](https://github.com/NVIDIA/stdexec) — C++26 执行模型（P2300）参考实现
- fmt / spdlog / gtest — 格式化、日志、测试
