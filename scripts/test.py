"""运行测试（ctest）。

用法: python scripts/test.py
"""
from __future__ import annotations

import sys
from pathlib import Path

import vs_env

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"


def main() -> int:
    cmd = f'ctest --test-dir "{BUILD_DIR}" --output-on-failure'
    print(f">> {cmd}", flush=True)
    return vs_env.run_in_vs_env(cmd, cwd=ROOT)


if __name__ == "__main__":
    sys.exit(main())
