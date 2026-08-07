// vendor/dom-serializer.js —— dom-serializer 替代（htmlparser2 风格序列化，
// 用于 cheerio 的 xml/_useHtmlParser2 渲染路径）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const VOID_ELEMENTS = new Set([
  'area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
  'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source',
  'track', 'wbr',
]);

// encodeXML：& < > 转义（text）
function encodeXMLText(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

// 属性值：& < > " 转义
function encodeXMLAttr(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function render(node, options) {
  if (Array.isArray(node)) {
    let out = '';
    for (const n of node) out += render(n, options);
    return out;
  }
  if (!node || typeof node !== 'object') return '';
  options = options || {};
  const t = node.type;
  switch (t) {
    case 'root':
      return render(node.children || [], options);
    case 'text':
      return options.encodeEntities === false
        ? String(node.data)
        : encodeXMLText(node.data);
    case 'comment':
      return '<!--' + node.data + '-->';
    case 'directive':
      return '<' + node.data + '>';
    case 'cdata':
      return '<![CDATA[' + node.data + ']]>';
    case 'script':
    case 'style':
    case 'tag':
      return renderTag(node, options);
    default:
      return '';
  }
}

function renderTag(el, options) {
  const name = el.name;
  let out = '<' + name;
  const attrs = el.attribs || {};
  const keys = Object.keys(attrs);
  for (let i = 0; i < keys.length; i++) {
    const k = keys[i];
    out += ' ' + k + '="' + encodeXMLAttr(attrs[k]) + '"';
  }
  if (VOID_ELEMENTS.has(name)) {
    return out + '>';
  }
  out += '>';
  out += render(el.children || [], options);
  return out + '</' + name + '>';
}

module.exports = render;
module.exports.default = render;
