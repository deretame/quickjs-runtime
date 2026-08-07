// vendor/domhandler.js —— domhandler 运行时部分（类型判断 + ElementType 常量）
// 从 domhandler 源码移植（MIT License, fb55）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const ElementType = {
  Root: 'root',
  Text: 'text',
  Directive: 'directive',
  Comment: 'comment',
  Script: 'script',
  Style: 'style',
  Tag: 'tag',
  CDATA: 'cdata',
  Doctype: 'doctype',
};

function isTag(node) {
  return node.type === ElementType.Tag ||
    node.type === ElementType.Script ||
    node.type === ElementType.Style;
}
function isCDATA(node) { return node.type === ElementType.CDATA; }
function isText(node) { return node.type === ElementType.Text; }
function isComment(node) { return node.type === ElementType.Comment; }
function isDirective(node) { return node.type === ElementType.Directive; }
function isDocument(node) { return node.type === ElementType.Root; }
function isScript(node) { return node.type === ElementType.Script; }
function isStyle(node) { return node.type === ElementType.Style; }
function hasChildren(node) {
  return Object.prototype.hasOwnProperty.call(node, 'children');
}

// 深拷贝节点树（domhandler cloneNode 语义）：parent/prev/next 重建
function cloneNode(node, recursive) {
  const clone = {};
  for (const k of Object.keys(node)) {
    if (k === 'parent' || k === 'prev' || k === 'next') continue;
    clone[k] = node[k];
  }
  clone.children = [];
  if (recursive && Array.isArray(node.children)) {
    for (const child of node.children) {
      const cc = cloneNode(child, true);
      cc.parent = clone;
      if (clone.children.length > 0) {
        const prev = clone.children[clone.children.length - 1];
        prev.next = cc;
        cc.prev = prev;
      }
      clone.children.push(cc);
    }
  }
  return clone;
}

class Document {
  constructor(children) {
    this.type = ElementType.Root;
    if (!children) {
      this.children = [];
    }
  }
}

// 运行时构造用节点类（text setter / clone 路径）
class Text {
  constructor(data) {
    this.type = ElementType.Text;
    this.data = data;
  }
}

class Comment {
  constructor(data) {
    this.type = ElementType.Comment;
    this.data = data;
  }
}

class Element {
  constructor(name, attribs) {
    this.type = ElementType.Tag;
    this.name = name;
    this.attribs = attribs || {};
    this.children = [];
  }
}

class ProcessingInstruction {
  constructor(name, data) {
    this.type = ElementType.Directive;
    this.name = name;
    this.data = data;
  }
}

exports.ElementType = ElementType;
exports.Document = Document;
exports.Text = Text;
exports.Comment = Comment;
exports.Element = Element;
exports.ProcessingInstruction = ProcessingInstruction;
exports.isTag = isTag;
exports.isCDATA = isCDATA;
exports.isText = isText;
exports.isComment = isComment;
exports.isDirective = isDirective;
exports.isDocument = isDocument;
exports.isScript = isScript;
exports.isStyle = isStyle;
exports.hasChildren = hasChildren;
exports.cloneNode = cloneNode;
