"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const index_js_1 = require("./index.js");
(0, vitest_1.describe)('.load', () => {
    (0, vitest_1.it)('(html) : should retain original root after creating a new node', () => {
        const $ = (0, index_js_1.load)('<body><ul id="fruits"></ul></body>');
        (0, vitest_1.expect)($('body')).toHaveLength(1);
        $('<script>');
        (0, vitest_1.expect)($('body')).toHaveLength(1);
    });
    (0, vitest_1.it)('(html) : should handle lowercase tag options', () => {
        const $ = (0, index_js_1.load)('<BODY><ul id="fruits"></ul></BODY>', {
            xml: { lowerCaseTags: true },
        });
        (0, vitest_1.expect)($.html()).toBe('<body><ul id="fruits"/></body>');
    });
    (0, vitest_1.it)('(html) : should handle xml tag option', () => {
        const $ = (0, index_js_1.load)('<body><script><foo></script></body>', {
            xml: true,
        });
        (0, vitest_1.expect)($('script')[0].children[0].type).toBe('tag');
    });
    (0, vitest_1.it)('(buffer) : should accept a buffer', () => {
        const html = '<html><head></head><body>foo</body></html>';
        const $html = (0, index_js_1.load)(Buffer.from(html));
        (0, vitest_1.expect)($html.html()).toBe(html);
    });
});
