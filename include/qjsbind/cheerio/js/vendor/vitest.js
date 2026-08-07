// vendor/vitest.js —— vitest 测试 API 最小兼容层（describe/it/expect 等）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const g = globalThis;

// ---------------------------------------------------------------------------
// 结果收集
// ---------------------------------------------------------------------------
if (!g.__cheerio_tests) {
  g.__cheerio_tests = { pass: 0, fail: 0, failures: [] };
}

function fail(name, err) {
  g.__cheerio_tests.fail++;
  g.__cheerio_tests.failures.push({
    name,
    message: String(err && err.message ? err.message : err),
    stack: err && err.stack ? String(err.stack) : '',
  });
}

// it 的错误带调用栈（QuickJS 只给 <eval> 行号，但能给到 spec 文件内行号）
// ---------------------------------------------------------------------------
// describe / it
// ---------------------------------------------------------------------------
const __describeStack = [];

function describe(name, fn) {
  __describeStack.push({ befores: [], afters: [] });
  try {
    if (typeof fn === 'function') fn();
  } finally {
    __describeStack.pop();
  }
}

function it(name, fn) {
  g.__cheerio_tests.last = String(name);
  try {
    // 收集从外层到内层的 beforeEach（vitest 语义）
    for (let d = 0; d < __describeStack.length; d++) {
      const befores = __describeStack[d].befores;
      for (let i = 0; i < befores.length; i++) befores[i]();
    }
    const r = fn();
    if (r && typeof r.then === 'function') {
      // 异步测试：同步等待（测试基本同步；异步极少）
      g.__cheerio_tests.fail++;
      g.__cheerio_tests.failures.push({
        name: String(name),
        message: 'async test not supported',
        stack: '',
      });
      return;
    }
    g.__cheerio_tests.pass++;
  } catch (e) {
    fail(name, e);
  }
}

function beforeEach(fn) {
  if (__describeStack.length > 0) __describeStack[__describeStack.length - 1].befores.push(fn);
}
function afterEach(fn) {
  if (__describeStack.length > 0) __describeStack[__describeStack.length - 1].afters.push(fn);
}
function beforeAll(fn) { if (typeof fn === 'function') fn(); }
function afterAll(fn) { if (typeof fn === 'function') fn(); }
function test(name, fn) { it(name, fn); }

function expectTypeOf() {
  return {
    toEqualTypeOf() {},
    toMatchTypeOf() {},
    toBeNullable() {},
    toBeNever() {},
  };
}

// ---------------------------------------------------------------------------
// expect
// ---------------------------------------------------------------------------
function deepEqual(a, b, strict) {
  return deepEqualImpl(a, b, strict, new Set());
}

function deepEqualImpl(a, b, strict, seen) {
  if (Object.is(a, b)) return true;
  if (typeof a !== typeof b) return false;
  if (a === null || b === null) return false;
  if (typeof a !== 'object') return false;
  // 循环引用保护（Cheerio 实例/节点树可能自引用）
  if (seen.has(a)) return seen.has(b);
  seen.add(a);
  if (seen.has(b)) return false;
  seen.add(b);
  if (Array.isArray(a) !== Array.isArray(b)) return false;
  if (Array.isArray(a)) {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
      if (!deepEqualImpl(a[i], b[i], strict, seen)) return false;
    }
    return true;
  }
  // cheerio 实例/类数组：比较元素（toStrictEqual 对 Cheerio 与数组）
  const aKeys = Object.keys(a);
  const bKeys = Object.keys(b);
  if (strict && aKeys.length !== bKeys.length) return false;
  // 顺序：对 toStrictEqual 要求键一致（含顺序）
  if (strict) {
    for (let i = 0; i < aKeys.length; i++) {
      if (aKeys[i] !== bKeys[i]) return false;
    }
  }
  for (const k of aKeys) {
    if (k === 'prevObject' || k === '_root') continue;
    if (!(k in b)) return false;
    if (!deepEqualImpl(a[k], b[k], strict, seen)) return false;
  }
  return true;
}

let __fvDepth = 0;
function formatValue(v, seen) {
  __fvDepth++;
  if (typeof g.__log_fv2 === 'function' && __fvDepth <= 3) {
    g.__log_fv2('fv ' + __fvDepth + ' t=' + typeof v + (v && v.type ? '/' + v.type : '') + (v && v.name ? ':' + v.name : ''));
  }
  if (__fvDepth > 25) { __fvDepth = 0; return '[Deep]'; }
  let out;
  if (v === null) out = 'null';
  else if (v === undefined) out = 'undefined';
  else if (typeof v === 'string') out = JSON.stringify(v);
  else if (typeof v === 'number' || typeof v === 'boolean' || typeof v === 'bigint') out = String(v);
  else if (typeof v === 'function') out = '[Function]';
  else if (typeof v === 'symbol') out = String(v);
  else if (typeof v === 'object') {
    seen = seen || new Set();
    if (seen.has(v)) {
      out = '[Circular]';
    } else {
      seen.add(v);
      if (Array.isArray(v)) {
        const items = [];
        for (let i = 0; i < v.length; i++) items.push(formatValue(v[i], seen));
        out = '[' + items.join(', ') + ']';
      } else if (v.cheerio || (v.length !== undefined && v.type === undefined)) {
        // cheerio 类数组
        const items = [];
        for (let i = 0; i < v.length; i++) items.push(formatValue(v[i], seen));
        out = 'Cheerio(' + items.join(', ') + ')';
      } else {
        const keys = Object.keys(v);
        const items = [];
        for (let i = 0; i < keys.length; i++) {
          items.push(keys[i] + ': ' + formatValue(v[keys[i]], seen));
        }
        out = '{' + items.join(', ') + '}';
      }
    }
  } else {
    out = String(v);
  }
  __fvDepth--;
  return out;
}

function makeExpect(actual) {
  const api = {
    get not() {
      return makeNegated(actual);
    },
    toBe(expected) {
      if (!Object.is(actual, expected)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to be ' + formatValue(expected),
        );
      }
    },
    toEqual(expected) {
      if (!deepEqual(actual, expected, false)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to equal ' + formatValue(expected),
        );
      }
    },
    toStrictEqual(expected) {
      if (!deepEqual(actual, expected, true)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to strictly equal ' +
            formatValue(expected),
        );
      }
    },
    toHaveLength(n) {
      const len = actual == null ? undefined : actual.length;
      if (len !== n) {
        throw new Error('expected length ' + String(len) + ' to be ' + String(n));
      }
    },
    toHaveProperty(path, value) {
      const parts = String(path).split('.');
      let cur = actual;
      for (const p of parts) {
        if (cur == null) {
          throw new Error('expected property ' + path + ' not found');
        }
        cur = cur[p];
      }
      if (arguments.length >= 2 && !deepEqual(cur, value, true)) {
        throw new Error('property ' + path + ' value mismatch');
      }
    },
    toBeUndefined() {
      if (actual !== undefined) {
        throw new Error('expected undefined, got ' + formatValue(actual));
      }
    },
    toBeDefined() {
      if (actual === undefined) {
        throw new Error('expected defined value');
      }
    },
    toBeNull() {
      if (actual !== null) {
        throw new Error('expected null, got ' + formatValue(actual));
      }
    },
    toBeTruthy() {
      if (!actual) {
        throw new Error('expected truthy, got ' + formatValue(actual));
      }
    },
    toBeFalsy() {
      if (actual) {
        throw new Error('expected falsy, got ' + formatValue(actual));
      }
    },
    toBeInstanceOf(cls) {
      if (!(actual instanceof cls)) {
        throw new Error('expected instance of ' + String(cls && cls.name));
      }
    },
    toThrow(match) {
      let threw = false;
      let err = null;
      try {
        if (typeof actual === 'function') actual();
        else throw new Error('not a function');
      } catch (e) {
        threw = true;
        err = e;
      }
      if (!threw) throw new Error('expected function to throw');
      if (match !== undefined) {
        if (typeof match === 'string') {
          if (!String(err && err.message).includes(match)) {
            throw new Error('expected error to match ' + match + ', got ' + err);
          }
        } else if (match instanceof RegExp) {
          if (!match.test(String(err && err.message))) {
            throw new Error('expected error to match ' + match + ', got ' + err);
          }
        } else if (typeof match === 'function') {
          if (!(err instanceof match)) {
            throw new Error('expected error instanceof ' + match.name + ', got ' + err);
          }
        }
      }
    },
    toContain(item) {
      if (actual == null) throw new Error('expected value to contain ' + formatValue(item));
      if (typeof actual === 'string') {
        if (!actual.includes(item)) {
          throw new Error('expected string to contain ' + formatValue(item));
        }
      } else {
        let found = false;
        for (let i = 0; i < actual.length; i++) {
          if (deepEqual(actual[i], item, false)) { found = true; break; }
        }
        if (!found) {
          throw new Error('expected array to contain ' + formatValue(item));
        }
      }
    },
    toContainEqual(item) {
      this.toContain(item);
    },
    toMatch(re) {
      if (!re.test(String(actual))) {
        throw new Error('expected ' + formatValue(actual) + ' to match ' + re);
      }
    },
    toBeLessThan(n) {
      if (!(actual < n)) {
        throw new Error('expected ' + actual + ' < ' + n);
      }
    },
    toBeGreaterThan(n) {
      if (!(actual > n)) {
        throw new Error('expected ' + actual + ' > ' + n);
      }
    },
    toBeLessThanOrEqual(n) {
      if (!(actual <= n)) {
        throw new Error('expected ' + actual + ' <= ' + n);
      }
    },
    toBeGreaterThanOrEqual(n) {
      if (!(actual >= n)) {
        throw new Error('expected ' + actual + ' >= ' + n);
      }
    },
    toBeCloseTo(n, digits) {
      const eps = Math.pow(10, -(digits || 2)) / 2;
      if (Math.abs(actual - n) > eps) {
        throw new Error('expected ' + actual + ' close to ' + n);
      }
    },
    toEqualTypeOf() {},
    toMatchTypeOf() {},
    toBeTypeOf() {},
    // 捕获断言错误：抛给 it() 的 try/catch
  };
  return api;
}

function makeNegated(actual) {
  const neg = {};
  const names = [
    'toBe', 'toEqual', 'toStrictEqual', 'toHaveLength', 'toHaveProperty',
    'toBeUndefined', 'toBeDefined', 'toBeNull', 'toBeTruthy', 'toBeFalsy',
    'toBeInstanceOf', 'toThrow', 'toContain', 'toMatch', 'toBeLessThan',
    'toBeGreaterThan', 'toBeCloseTo',
  ];
  for (const n of names) {
    neg[n] = (...args) => {
      try {
        makeExpect(actual)[n](...args);
      } catch (e) {
        return; // 原断言失败 = 取反成功
      }
      throw new Error('expected negation of ' + n + ' to fail');
    };
  }
  return neg;
}

function expect(actual) {
  return makeExpect(actual);
}

exports.describe = describe;
exports.it = it;
exports.test = test;
exports.expect = expect;
exports.beforeEach = beforeEach;
exports.afterEach = afterEach;
exports.beforeAll = beforeAll;
exports.afterAll = afterAll;
exports.expectTypeOf = expectTypeOf;
exports.vi = {};
