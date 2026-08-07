// vendor/parse5.js —— parse5 替代：lexbor 解析 + parse5 风格序列化
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

// ---------------------------------------------------------------------------
// 序列化（对齐 parse5 的 serialize/serializeOuter 对 htmlparser2 树的输出）
// ---------------------------------------------------------------------------

const VOID_ELEMENTS = new Set([
  'area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
  'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source',
  'track', 'wbr',
]);

const RAW_TEXT_ELEMENTS = new Set([
  'script', 'style', 'xmp', 'iframe', 'noembed', 'noframes', 'plaintext',
  'noscript',
]);

// parse5 encodeText：& < > \u00A0（parse5 默认 encodeHtmlEntities=false，
// 命名实体不解码后重新编码）
function encodeText(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/\u00A0/g, '&nbsp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

// parse5 encodeAttr：& " \u00A0
function encodeAttr(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/\u00A0/g, '&nbsp;')
    .replace(/"/g, '&quot;');
}

function serializeChildren(children) {
  let out = '';
  for (const c of children) out += serializeNode(c);
  return out;
}

function serializeNode(node) {
  if (Array.isArray(node)) return serializeChildren(node);
  if (!node || typeof node !== 'object') return '';
  const t = node.type;
  switch (t) {
    case 'root':
      return serializeChildren(node.children || []);
    case 'text':
      return encodeText(node.data);
    case 'comment':
      return '<!--' + node.data + '-->';
    case 'directive':
      return '<' + node.data + '>';
    case 'cdata':
      return '<![CDATA[' + node.data + ']]>';
    case 'script':
    case 'style':
    case 'tag':
      return serializeElement(node);
    default:
      return '';
  }
}

function serializeElement(el) {
  const name = el.name;
  let out = '<' + name;
  const attrs = el.attribs || {};
  for (const k of Object.keys(attrs)) {
    out += ' ' + k + '="' + encodeAttr(attrs[k]) + '"';
  }
  if (VOID_ELEMENTS.has(name)) {
    return out + '>';
  }
  out += '>';
  const children = el.children || [];
  if (RAW_TEXT_ELEMENTS.has(name)) {
    // raw text 内容不转义（parse5 行为）
    for (const c of children) {
      if (c.type === 'text' || c.type === 'cdata') out += c.data;
      else out += serializeNode(c);
    }
  } else {
    out += serializeChildren(children);
  }
  return out + '</' + name + '>';
}

// 节点自身 + 后代序列化（serializeOuter 语义；root 展开 children）
function serializeOuter(node) {
  return serializeNode(node);
}

// ---------------------------------------------------------------------------
// 解析（lexbor）
// ---------------------------------------------------------------------------

// parse5-htmlparser2-tree-adapter 兼容：给节点加 parentNode/childNodes 等
// 别名（getter 实时反映 cheerio 的树操作；不可枚举避免干扰 deepEqual）
function addParse5Aliases(node) {
  if (node && typeof node === 'object') {
    if (!('parentNode' in node)) {
      Object.defineProperty(node, 'parentNode', {
        get() { return this.parent; },
        enumerable: false,
        configurable: true,
      });
    }
    if (!('nextSibling' in node)) {
      Object.defineProperty(node, 'nextSibling', {
        get() { return this.next; },
        enumerable: false,
        configurable: true,
      });
    }
    if (!('previousSibling' in node)) {
      Object.defineProperty(node, 'previousSibling', {
        get() { return this.prev; },
        enumerable: false,
        configurable: true,
      });
    }
    if (Array.isArray(node.children) && !('childNodes' in node)) {
      Object.defineProperty(node, 'childNodes', {
        get() { return this.children; },
        enumerable: false,
        configurable: true,
      });
    }
  }
  return node;
}

function addAliasesRecursive(root) {
  const stack = [root];
  while (stack.length) {
    const n = stack.pop();
    if (!n || typeof n !== 'object') continue;
    addParse5Aliases(n);
    if (Array.isArray(n.children)) {
      for (let i = 0; i < n.children.length; i++) stack.push(n.children[i]);
    }
  }
  return root;
}

// parse5 的 parse(content, options)：完整文档
function parse(content, options) {
  const root = globalThis.__lexbor_parse(String(content), true, null);
  return addAliasesRecursive(root);
}

// parse5 的 parseFragment(context, content, options)
function parseFragment(context, content, options) {
  let contextTag = null;
  if (context && typeof context === 'object') {
    if (context.name) contextTag = context.name;
    else if (context.tagName) contextTag = context.tagName;
  }
  const root = globalThis.__lexbor_parse(String(content), false, contextTag);
  return addAliasesRecursive(root);
}

exports.parse = parse;
exports.parseFragment = parseFragment;
exports.serializeOuter = serializeOuter;
exports.serialize = serializeNode;
