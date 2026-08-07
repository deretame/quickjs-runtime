// vendor/parse5-parser-stream.js —— 退化实现（同步一次性解析）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

class ParserStream {
  constructor(options) {
    this._data = '';
    this.options = options;
  }
  write(chunk) {
    this._data += String(chunk);
  }
  end(cb) {
    const root = globalThis.__lexbor_parse(this._data, true, null);
    if (this.options && this.options.treeAdapter) {
      this.document = root;
    }
    if (typeof cb === 'function') cb();
  }
}

exports.default = ParserStream;
exports.ParserStream = ParserStream;
