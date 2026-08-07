"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
const node_http_1 = require("node:http");
const node_stream_1 = require("node:stream");
const vitest_1 = require("vitest");
const cheerio = __importStar(require("./index.js"));
function noop() {
    // Ignore
}
// Returns a promise and a resolve function
function getPromise() {
    let cb;
    const promise = new Promise((resolve, reject) => {
        cb = (error, $) => (error ? reject(error) : resolve($));
    });
    return { promise, cb };
}
const TEST_HTML = '<h1>Hello World</h1><a href="link">Example</a>';
const TEST_HTML_UTF16 = Buffer.from(TEST_HTML, 'utf16le');
const TEST_HTML_UTF16_BOM = Buffer.from([
    // UTF16-LE BOM
    0xff,
    0xfe,
    ...TEST_HTML_UTF16,
]);
(0, vitest_1.describe)('loadBuffer', () => {
    (0, vitest_1.it)('should parse UTF-8 HTML', () => {
        const $ = cheerio.loadBuffer(Buffer.from(TEST_HTML));
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
    });
    (0, vitest_1.it)('should parse UTF-16 HTML', () => {
        const $ = cheerio.loadBuffer(TEST_HTML_UTF16_BOM);
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
    });
});
(0, vitest_1.describe)('stringStream', () => {
    (0, vitest_1.it)('should use parse5 by default', async () => {
        const { promise, cb } = getPromise();
        const stream = cheerio.stringStream({}, cb);
        (0, vitest_1.expect)(stream).toBeInstanceOf(node_stream_1.Writable);
        stream.end(TEST_HTML);
        const $ = await promise;
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
    });
    (0, vitest_1.it)('should error from parse5 on buffer', () => {
        const stream = cheerio.stringStream({}, noop);
        (0, vitest_1.expect)(stream).toBeInstanceOf(node_stream_1.Writable);
        (0, vitest_1.expect)(() => stream.write(Buffer.from(TEST_HTML))).toThrow('Parser can work only with string streams.');
    });
    (0, vitest_1.it)('should use htmlparser2 for XML', async () => {
        const { promise, cb } = getPromise();
        const stream = cheerio.stringStream({ xmlMode: true }, cb);
        (0, vitest_1.expect)(stream).toBeInstanceOf(node_stream_1.Writable);
        stream.end(TEST_HTML);
        const $ = await promise;
        (0, vitest_1.expect)($.html()).toBe(TEST_HTML);
    });
});
(0, vitest_1.describe)('decodeStream', () => {
    (0, vitest_1.it)('should use parse5 by default', async () => {
        const { promise, cb } = getPromise();
        const stream = cheerio.decodeStream({}, cb);
        (0, vitest_1.expect)(stream).toBeInstanceOf(node_stream_1.Writable);
        stream.end(TEST_HTML_UTF16_BOM);
        const $ = await promise;
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
        (0, vitest_1.expect)($('a').prop('href')).toBe('link');
    });
    (0, vitest_1.it)('should use htmlparser2 for XML', async () => {
        const { promise, cb } = getPromise();
        const stream = cheerio.decodeStream({ xmlMode: true }, cb);
        (0, vitest_1.expect)(stream).toBeInstanceOf(node_stream_1.Writable);
        stream.end(TEST_HTML_UTF16_BOM);
        const $ = await promise;
        (0, vitest_1.expect)($.html()).toBe(TEST_HTML);
    });
});
(0, vitest_1.describe)('fromURL', () => {
    let server;
    function createTestServer(contentType, body, handler = (_req, res) => {
        res.writeHead(200, { 'Content-Type': contentType });
        res.end(body);
    }) {
        return new Promise((resolve, reject) => {
            server = (0, node_http_1.createServer)(handler);
            server.listen(0, () => {
                const address = server?.address();
                if (typeof address === 'string' || address == null) {
                    reject(new Error('Failed to get port'));
                }
                else {
                    resolve(address.port);
                }
            });
        });
    }
    (0, vitest_1.afterEach)(async () => new Promise((resolve, reject) => {
        if (server) {
            server.close((err) => (err ? reject(err) : resolve()));
            server = undefined;
        }
        else {
            resolve();
        }
    }));
    (0, vitest_1.it)('should fetch UTF-8 HTML', async () => {
        const port = await createTestServer('text/html', TEST_HTML);
        const $ = await cheerio.fromURL(`http://localhost:${port}`);
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
    });
    (0, vitest_1.it)('should fetch UTF-16 HTML', async () => {
        const port = await createTestServer('text/html; charset=utf-16le', TEST_HTML_UTF16);
        const $ = await cheerio.fromURL(`http://localhost:${port}`);
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
    });
    (0, vitest_1.it)('should parse XML based on Content-Type', async () => {
        const port = await createTestServer('text/xml', TEST_HTML);
        const $ = await cheerio.fromURL(`http://localhost:${port}`);
        (0, vitest_1.expect)($.html()).toBe(TEST_HTML);
    });
    (0, vitest_1.it)('should throw on non-HTML/XML Content-Type', async () => {
        const port = await createTestServer('text/plain', TEST_HTML);
        await (0, vitest_1.expect)(cheerio.fromURL(`http://localhost:${port}`)).rejects.toThrow('The content-type "text/plain" is neither HTML nor XML.');
    });
    (0, vitest_1.it)('should throw on non-2xx responses', async () => {
        const port = await createTestServer('text/html', TEST_HTML, (_, res) => {
            res.writeHead(500);
            res.end();
        });
        await (0, vitest_1.expect)(cheerio.fromURL(`http://localhost:${port}`)).rejects.toThrow('Response Error');
    });
    (0, vitest_1.it)('should follow redirects', async () => {
        let firstRequestUrl;
        let secondRequestUrl;
        const port = await createTestServer('text/html', TEST_HTML, (req, res) => {
            if (firstRequestUrl === undefined) {
                firstRequestUrl = req.url;
                res.writeHead(302, { Location: `http://localhost:${port}/final/path` });
                res.end();
            }
            else {
                secondRequestUrl = req.url;
                res.writeHead(200, { 'Content-Type': 'text/html' });
                res.end(TEST_HTML);
            }
        });
        const $ = await cheerio.fromURL(`http://localhost:${port}/first`);
        (0, vitest_1.expect)(firstRequestUrl).toBe('/first');
        (0, vitest_1.expect)(secondRequestUrl).toBe('/final/path');
        (0, vitest_1.expect)($.html()).toBe(`<html><head></head><body>${TEST_HTML}</body></html>`);
        (0, vitest_1.expect)($('a').prop('href')).toBe(`http://localhost:${port}/final/link`);
    });
});
