// vendor/node-stream.js —— node:stream 退化实现（Readable/Writable 极简）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const EventEmitter = (typeof globalThis.EventTarget !== 'undefined')
  ? class { constructor() { this._listeners = {}; } on(n, f) { (this._listeners[n] = this._listeners[n] || []).push(f); return this; } once(n, f) { const g = (...a) => { f(...a); this.off(n, g); }; return this.on(n, g); } off(n, f) { const l = this._listeners[n]; if (l) { const i = l.indexOf(f); if (i >= 0) l.splice(i, 1); } return this; } emit(n, ...a) { const l = this._listeners[n] || []; for (const f of [...l]) f(...a); return true; } }
  : class { constructor() { this._listeners = {}; } on(n, f) { (this._listeners[n] = this._listeners[n] || []).push(f); return this; } emit(n, ...a) { const l = this._listeners[n] || []; for (const f of [...l]) f(...a); return true; } };

class Readable extends EventEmitter {
  constructor(opts) { super(); this.readable = true; this._buf = []; this.ended = false; }
  _read() {}
  push(chunk) { if (chunk === null) { this.ended = true; this.emit('end'); } else { this._buf.push(chunk); this.emit('data', chunk); } return true; }
  pipe(dest) { this.on('data', (d) => dest.write(d)); this.on('end', () => dest.end()); return dest; }
  read() { return this._buf.length ? this._buf.shift() : null; }
  onData(fn) { this.on('data', fn); }
}

class Writable extends EventEmitter {
  constructor(opts) { super(); this.writable = true; }
  write(chunk) { this.emit('drain'); return true; }
  end(cb) { this.emit('finish'); if (typeof cb === 'function') cb(); return this; }
}

exports.Readable = Readable;
exports.Writable = Writable;
exports.EventEmitter = EventEmitter;
