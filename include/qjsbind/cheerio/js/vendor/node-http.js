// vendor/node-http.js —— node:http 退化实现（index.spec 的 fromURL 用不到时占位）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

exports.request = function () {
  throw new Error('node:http request is not available in this environment');
};
