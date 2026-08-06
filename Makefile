# 构建/测试入口（与 pixi.toml tasks 等价）
.PHONY: build test

build:
	python scripts/build.py

test: build
	python scripts/test.py
