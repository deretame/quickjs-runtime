// vendor/domutils.js —— domutils 运行时部分（textContent / innerText / removeElement）
// 从 domutils 源码移植（MIT License, fb55）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const domhandler = require('./domhandler.js');

function getChildren(node) {
  return Object.prototype.hasOwnProperty.call(node, 'children')
    ? node.children
    : [];
}

function textContent(node) {
  if (domhandler.isText(node)) return node.data;
  if (domhandler.isCDATA(node)) return node.data;
  if (domhandler.isComment(node)) return '';
  if (domhandler.isDirective(node)) return '';
  const result = [];
  for (const child of getChildren(node)) {
    result.push(textContent(child));
  }
  return result.join('');
}

function innerText(node) {
  if (domhandler.isText(node) || domhandler.isCDATA(node)) return node.data;
  if (domhandler.isComment(node)) return '';
  if (domhandler.isDirective(node)) return '';
  let result = '';
  for (const child of getChildren(node)) {
    const text = innerText(child);
    if (text) {
      if (domhandler.isTag(node) && node.name === 'br' && result) {
        result += '\n';
      }
      result += text;
    }
  }
  return result;
}

function removeElement(elem) {
  const parent = elem.parent;
  if (parent) {
    const children = parent.children;
    const idx = children.indexOf(elem);
    if (idx >= 0) children.splice(idx, 1);
    const prev = elem.prev;
    const next = elem.next;
    if (prev) prev.next = next;
    if (next) next.prev = prev;
    if (children.length > 0) {
      if (prev === null) children[0].prev = null;
      if (next === null) children[children.length - 1].next = null;
    }
  }
  elem.parent = null;
  elem.prev = null;
  elem.next = null;
}

exports.textContent = textContent;
exports.innerText = innerText;
exports.removeElement = removeElement;
exports.getChildren = getChildren;

// ---- traversing 依赖（domutils 原版语义）----

function getChildrenWithChecks(node) {
  return getChildren(node);
}

// 所有兄弟（含自身），文档序
function getSiblings(node) {
  const parent = node.parent;
  if (parent) return getChildren(parent);
  // 无父：自身
  return [node];
}

function nextElementSibling(node) {
  let next = node.next;
  while (next && !domhandler.isTag(next)) next = next.next;
  return next;
}

function prevElementSibling(node) {
  let prev = node.prev;
  while (prev && !domhandler.isTag(prev)) prev = prev.prev;
  return prev;
}

// 稳定去重排序（文档序）：按节点在树中的前序遍历位置
function uniqueSort(nodes) {
  const seen = new Set();
  const out = [];
  for (const n of nodes) {
    if (!seen.has(n)) { seen.add(n); out.push(n); }
  }
  // 文档序：先序遍历索引（domutils.uniqueSort 语义）
  const order = new Map();
  let root = null;
  for (const n of nodes) {
    let cur = n;
    while (cur && cur.type !== 'root') cur = cur.parent;
    if (cur) { root = cur; break; }
  }
  if (root) {
    let idx = 0;
    const stack = [root];
    while (stack.length) {
      const node = stack.pop();
      order.set(node, idx++);
      const children = node.children || [];
      for (let i = children.length - 1; i >= 0; i--) stack.push(children[i]);
    }
  }
  out.sort((a, b) => {
    const ia = order.get(a);
    const ib = order.get(b);
    if (ia === undefined && ib === undefined) return 0;
    if (ia === undefined) return 1;
    if (ib === undefined) return -1;
    return ia - ib;
  });
  return out;
}

exports.getChildren = getChildrenWithChecks;
exports.getSiblings = getSiblings;
exports.nextElementSibling = nextElementSibling;
exports.prevElementSibling = prevElementSibling;
exports.uniqueSort = uniqueSort;
