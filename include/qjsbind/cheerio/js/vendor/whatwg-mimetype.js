// vendor/whatwg-mimetype.js —— 退化实现（MIMEType 基础解析）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

class MIMEType {
  constructor(s) {
    const parts = String(s).split(';');
    const [type, subtype] = parts[0].trim().split('/');
    this.type = type.toLowerCase();
    this.subtype = (subtype || '').toLowerCase();
    this.parameters = new Map();
    for (let i = 1; i < parts.length; i++) {
      const eq = parts[i].indexOf('=');
      if (eq > 0) {
        this.parameters.set(
          parts[i].slice(0, eq).trim().toLowerCase(),
          parts[i].slice(eq + 1).trim().replace(/^"|"$/g, ''),
        );
      }
    }
  }
  get essence() {
    return this.type + '/' + this.subtype;
  }
  isXML() {
    return this.subtype === 'xml' || this.subtype.endsWith('+xml');
  }
  isHTML() {
    return this.type === 'text' && this.subtype === 'html';
  }
}

exports.default = MIMEType;
exports.MIMEType = MIMEType;
