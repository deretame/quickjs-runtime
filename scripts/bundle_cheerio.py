"""打包 cheerio JS 库为 C++ 嵌入头（include/qjsbind/cheerio/cheerio_js_bundle.hpp）。

用法: python scripts/bundle_cheerio.py
输入: include/qjsbind/cheerio/js/**/*.js
输出: include/qjsbind/cheerio/cheerio_js_bundle.hpp

每个 JS 文件被包装为 CommonJS 模块函数并注册到 __cheerio_modules；
外部依赖（domhandler 等）重定向到 ./vendor/*.js。
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JS_DIR = ROOT / "include" / "qjsbind" / "cheerio" / "js"
OUT = ROOT / "include" / "qjsbind" / "cheerio" / "cheerio_js_bundle.hpp"

# 外部依赖 -> vendor 相对路径（相对 JS_DIR，模块里再按需算相对深度）
VENDOR_MAP = {
    "domhandler": "domhandler.js",
    "htmlparser2": "htmlparser2.js",
    "domutils": "domutils.js",
    "parse5": "parse5.js",
    "parse5-htmlparser2-tree-adapter": "parse5-htmlparser2-tree-adapter.js",
    "dom-serializer": "dom-serializer.js",
    "cheerio-select": "cheerio-select.js",
    "encoding-sniffer": "encoding-sniffer.js",
    "parse5-parser-stream": "parse5-parser-stream.js",
    "whatwg-mimetype": "whatwg-mimetype.js",
    "node:stream": "node-stream.js",
    "undici": "undici.js",
    "vitest": "vitest.js",
}

MODULE_NAMES = [
    "cheerio.js", "load.js", "load-parse.js", "parse.js", "options.js",
    "static.js", "utils.js", "types.js", "index.js", "slim.js",
    "api/attributes.js", "api/css.js", "api/extract.js", "api/forms.js",
    "api/manipulation.js", "api/traversing.js",
    "parsers/parse5-adapter.js",
]

# vendor 模块（全部打包）
VENDOR_FILES = [
    "vendor/domhandler.js",
    "vendor/htmlparser2.js",
    "vendor/domutils.js",
    "vendor/parse5.js",
    "vendor/parse5-htmlparser2-tree-adapter.js",
    "vendor/dom-serializer.js",
    "vendor/cheerio-select.js",
    "vendor/encoding-sniffer.js",
    "vendor/parse5-parser-stream.js",
    "vendor/whatwg-mimetype.js",
    "vendor/node-stream.js",
    "vendor/node-http.js",
    "vendor/undici.js",
    "vendor/vitest.js",
]

MODULE_NAMES += VENDOR_FILES


def rel_vendor_path(mod_name: str) -> str:
    """从模块文件出发到 vendor 目录的相对路径（模块内 require 用）。"""
    depth = mod_name.count("/")
    return "../" * depth + "./vendor/"


def transform_source(mod_name: str, src: str) -> str:
    """把 require('外部包') 重写为 require('<相对>/vendor/x.js')。"""
    prefix = rel_vendor_path(mod_name)
    for pkg, vfile in VENDOR_MAP.items():
        src = src.replace(f'require("{pkg}")', f'require("{prefix}{vfile}")')
    return src


def main() -> int:
    modules = []
    for name in MODULE_NAMES:
        p = JS_DIR / name
        if not p.exists():
            print(f"missing: {p}", file=sys.stderr)
            return 1
        src = p.read_text(encoding="utf-8")
        src = transform_source(name, src)
        modules.append((name, src))

    # 生成 bundle JS（嵌入 C++ raw string）
    parts = []
    parts.append("(function () {\n")
    parts.append("var __mods = {};\n")
    for name, src in modules:
        parts.append(f'__mods[{name!r}] = function (module, exports, require) {{\n{src}\n}};\n')
    parts.append(
        """
// 路径规范化 + 相对解析
function __norm(p) {
  var parts = [];
  for (var seg of p.split('/')) {
    if (seg === '' || seg === '.') continue;
    if (seg === '..') { if (parts.length) parts.pop(); }
    else parts.push(seg);
  }
  return parts.join('/');
}
function __resolve(dir, name) {
  if (name.startsWith('./') || name.startsWith('../')) {
    return __norm(dir + '/' + name);
  }
  return name; // 已在 transform 时重写为相对路径
}
var __cache = {};
function __load(dir, name) {
  var id = __resolve(dir, name);
  if (__cache[id]) return __cache[id].exports;
  if (!__mods[id]) throw new Error('cheerio bundle: module not found: ' + id);
  var module = { exports: {} };
  __cache[id] = module;
  __mods[id](module, module.exports, function (n) { return __load(id.includes('/') ? id.slice(0, id.lastIndexOf('/')) : '', n); });
  return module.exports;
}
globalThis.__cheerio_require = function (name) {
  return __load('', name);
};
"""
    )
    parts.append("})();\n")
    bundle_js = "".join(parts)

    marker = "BUNDLE_7F3A9D2C"  # <=16 字符（C++ raw string 分隔符上限）
    header = (
        "// cheerio_js_bundle.hpp —— 自动生成（python scripts/bundle_cheerio.py），勿手改\n"
        "#pragma once\n"
        "#include <string>\n"
        "namespace qjsbind::cheerio {\n"
        "inline const std::string& cheerio_bundle_js()\n"
        "{\n"
        f"    static const std::string s = R\"{marker}(\n{bundle_js}\n){marker}\";\n"
        "    return s;\n"
        "}\n"
        "} // namespace qjsbind::cheerio\n"
    )
    OUT.write_text(header, encoding="utf-8")
    print(f"wrote {OUT} ({len(header)} bytes, {len(bundle_js)} JS bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
