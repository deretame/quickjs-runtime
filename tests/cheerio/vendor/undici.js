// vendor/undici.js —— fetch 替代（使用全局 fetch）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

exports.fetch = typeof globalThis.fetch === 'function'
  ? globalThis.fetch.bind(globalThis)
  : function () { throw new Error('fetch is not available'); };
