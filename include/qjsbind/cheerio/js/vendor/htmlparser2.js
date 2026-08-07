// vendor/htmlparser2.js —— cheerio 的 xml/htmlparser2 路径退化为 lexbor 解析
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const domhandler = require('./domhandler.js');

// 与 htmlparser2 相同的 ElementType 常量
const ElementType = domhandler.ElementType;

// 完整文档解析（xml 模式也走 lexbor HTML 解析）
function parseDocument(content, options) {
  const root = globalThis.__lexbor_parse(String(content), true, null);
  return root;
}

// parseDOM: 片段解析
function parseDOM(content, options) {
  const root = globalThis.__lexbor_parse(String(content), false, null);
  return root.children;
}

function isTag(node) {
  return domhandler.isTag(node);
}

exports.ElementType = ElementType;
exports.parseDocument = parseDocument;
exports.parseDOM = parseDOM;
exports.isTag = isTag;
exports.DomHandler = domhandler.Document;
