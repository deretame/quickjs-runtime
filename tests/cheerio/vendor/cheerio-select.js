// vendor/cheerio-select.js —— css-select/cheerio-select 替代：基于 lexbor CSS 选择器
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

function isTag(node) {
  return node && typeof node === 'object' &&
    (node.type === 'tag' || node.type === 'script' || node.type === 'style');
}

// css-select 特有语法 → lexbor 可解析形式
function preprocessSelector(sel) {
  let s = String(sel);
  // [name!="x"] → :not([name="x"])
  s = s.replace(/\[([\w.-]+)!=("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|[\w-]+)\]/g,
    ':not([$1=$2])');
  // :matches(...) → :is(...)
  s = s.replace(/:matches\(/g, ':is(');
  // jQuery 表单伪类 → 属性选择器组合
  s = s.replace(/:submit\b/g, 'input[type="submit"], button[type="submit"]');
  s = s.replace(/:image\b/g, 'input[type="image"]');
  s = s.replace(/:reset\b/g, 'input[type="reset"], button[type="reset"]');
  s = s.replace(/:file\b/g, 'input[type="file"]');
  s = s.replace(/:password\b/g, 'input[type="password"]');
  s = s.replace(/:radio\b/g, 'input[type="radio"]');
  s = s.replace(/:checkbox\b/g, 'input[type="checkbox"]');
  s = s.replace(/:text\b/g, 'input[type="text"]');
  s = s.replace(/:button\b/g, 'button, input[type="button"]');
  s = s.replace(/:selected\b/g, '[selected]');
  s = s.replace(/:first(?!-)\b/g, ':first-child');
  s = s.replace(/:last(?!-)\b/g, ':last-child');
  s = s.replace(/:even\b/g, ':nth-child(2n+1)');
  s = s.replace(/:odd\b/g, ':nth-child(2n)');
  // css-select 的 :eq(n)/:nth(n)（0-based 集合位置）→ nth-child(n+1)
  s = s.replace(/:eq\((\d+)\)/g, ':nth-child($1+1)');
  s = s.replace(/:nth\((\d+)\)/g, ':nth-child($1+1)');
  // 相对选择器：css-select 支持 find('> li')/find('+.b') 等（:scope 为查询根）
  if (/^\s*[>+~]/.test(s)) {
    s = ':scope ' + s;
  }
  return s;
}

// 向上找文档根（type === 'root' 的节点）
function getRoot(node) {
  let cur = node;
  while (cur && typeof cur === 'object') {
    if (cur.type === 'root') return cur;
    cur = cur.parent;
  }
  return null;
}

function queryAll(root, selector, includeSelf) {
  return globalThis.__lexbor_queryAll(root, preprocessSelector(selector), !!includeSelf);
}

// css-select 的 select(selector, elems, options, limit)：对每个候选元素，
// 在其自身范围内查询（scope = 元素本身，css-select 的 :scope 语义），
// 结果 = 匹配且在该元素子树内（含自身）；去重、limit 截断。
function select(selector, elems, options, limit) {
  const out = [];
  const seen = new Set();
  for (const el of elems) {
    if (!el || typeof el !== 'object') continue;
    let matches;
    try {
      matches = queryAll(el, selector, true);
    } catch (e) {
      // 选择器无法解析：交给调用方（css-select 语义为抛错）
      throw e;
    }
    for (let i = 0; i < matches.length; i++) {
      const m = matches[i];
      if (!seen.has(m) && isDescendantOrSelf(el, m)) {
        seen.add(m);
        out.push(m);
        if (limit && out.length >= limit) return out;
      }
    }
  }
  return out;
}

// m 是否在 el 的子树内（含 el 自身）
function isDescendantOrSelf(el, m) {
  let cur = m;
  while (cur && typeof cur === 'object') {
    if (cur === el) return true;
    cur = cur.parent;
  }
  return false;
}

function selectOne(selector, elems, options) {
  const res = select(selector, elems, options, 1);
  return res.length > 0 ? res[0] : null;
}

// css-select 的 is(elem, query, options)：elem 是否匹配 query（scope = elem）
function is(elem, query, options) {
  if (!isTag(elem)) return false;
  const matches = queryAll(elem, query, true);
  for (let i = 0; i < matches.length; i++) {
    if (matches[i] === elem) return true;
  }
  return false;
}

// css-select 的 filter(query, nodes, options)：nodes 中匹配的元素
function filter(query, nodes, options) {
  const out = [];
  for (const node of nodes) {
    if (!isTag(node)) continue;
    if (is(node, query, options)) out.push(node);
  }
  return out;
}

// 任一节点匹配
function some(nodes, query, options) {
  for (const node of nodes) {
    if (isTag(node) && is(node, query, options)) return true;
  }
  return false;
}

exports.select = select;
exports.selectOne = selectOne;
exports.is = is;
exports.filter = filter;
exports.some = some;
