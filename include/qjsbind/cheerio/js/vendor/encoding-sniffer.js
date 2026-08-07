// vendor/encoding-sniffer.js —— 退化实现（UTF-8 假设）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

function decodeBuffer(buffer, options) {
  // buffer: { toString(enc) } 或 Uint8Array
  if (typeof buffer.toString === 'function') {
    return buffer.toString('utf8');
  }
  const bytes = Array.from(buffer);
  const chars = bytes.map((b) => String.fromCharCode(b)).join('');
  try {
    return decodeURIComponent(escape(chars));
  } catch (e) {
    return chars;
  }
}

function getEncoding(buffer) {
  return 'utf-8';
}

exports.decodeBuffer = decodeBuffer;
exports.getEncoding = getEncoding;
