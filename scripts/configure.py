"""配置 CMake 项目（MSVC x64 + Ninja + vcpkg toolchain）。

用法: python scripts/configure.py [--build-type Debug|Release]
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import vs_env

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
TOOLCHAIN = ROOT / "third_party" / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-type", default="Debug", choices=["Debug", "Release", "RelWithDebInfo"])
    args = parser.parse_args()

    if not TOOLCHAIN.exists():
        print(f"错误: 未找到 vcpkg toolchain: {TOOLCHAIN}\n请先运行: pixi run setup-vcpkg", file=sys.stderr)
        return 1

    cmd = (
        f'cmake -S "{ROOT}" -B "{BUILD_DIR}" -G Ninja '
        f'-DCMAKE_TOOLCHAIN_FILE="{TOOLCHAIN}" '
        f'-DCMAKE_BUILD_TYPE={args.build_type}'
    )
    print(f">> {cmd}", flush=True)
    return vs_env.run_in_vs_env(cmd, cwd=ROOT)


if __name__ == "__main__":
    sys.exit(main())
