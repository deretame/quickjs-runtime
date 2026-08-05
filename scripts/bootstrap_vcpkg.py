"""克隆 vcpkg 到 third_party/vcpkg 并执行 bootstrap（固定 release tag）。

用法: python scripts/bootstrap_vcpkg.py
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VCPKG_DIR = ROOT / "third_party" / "vcpkg"
VCPKG_URL = "https://github.com/microsoft/vcpkg.git"
# 固定的 vcpkg release tag（与 vcpkg.json 中 builtin-baseline 的 commit 对应）
VCPKG_TAG = "2026.07.29"
VCPKG_EXE = VCPKG_DIR / "vcpkg.exe"


def run(cmd: list[str]) -> None:
    print(f">> {' '.join(cmd)}", flush=True)
    subprocess.run(cmd, check=True)


def ensure_tag() -> None:
    """确保 vcpkg 仓库 checkout 在固定 tag 上（防止上游 master 漂移）。"""
    if not VCPKG_DIR.exists():
        return
    head = subprocess.run(
        ["git", "-C", str(VCPKG_DIR), "rev-parse", "HEAD"],
        capture_output=True, text=True,
    ).stdout.strip()
    tag_commit = subprocess.run(
        ["git", "-C", str(VCPKG_DIR), "rev-parse", f"{VCPKG_TAG}^{{commit}}"],
        capture_output=True, text=True,
    ).stdout.strip()
    if head and tag_commit and head != tag_commit:
        print(f"[checkout] 切换到固定 tag {VCPKG_TAG} ...")
        run(["git", "-C", str(VCPKG_DIR), "fetch", "--depth", "1", "origin", "tag", VCPKG_TAG])
        run(["git", "-C", str(VCPKG_DIR), "checkout", VCPKG_TAG])


def main() -> None:
    if VCPKG_EXE.exists():
        print(f"[skip] vcpkg 已就绪: {VCPKG_EXE}")
        ensure_tag()
    else:
        if VCPKG_DIR.exists():
            print(f"[info] 目录已存在但缺少 vcpkg.exe，尝试复用: {VCPKG_DIR}")
        else:
            VCPKG_DIR.parent.mkdir(parents=True, exist_ok=True)
            print(f"[clone] 克隆 vcpkg (tag {VCPKG_TAG}) -> {VCPKG_DIR}")
            run(["git", "clone", "--depth", "1", "--branch", VCPKG_TAG, VCPKG_URL, str(VCPKG_DIR)])
        ensure_tag()
        print("[bootstrap] 运行 bootstrap-vcpkg.bat ...")
        run([str(VCPKG_DIR / "bootstrap-vcpkg.bat")])

    print("[verify] vcpkg 版本:")
    run([str(VCPKG_EXE), "version"])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"失败: {e}", file=sys.stderr)
        sys.exit(e.returncode)
