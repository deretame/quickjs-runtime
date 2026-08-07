"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const fixtures_js_1 = require("../__fixtures__/fixtures.js");
const index_js_1 = require("../index.js");
(0, vitest_1.describe)('$(...)', () => {
    (0, vitest_1.describe)('.css', () => {
        (0, vitest_1.it)('(prop): should return a css property value', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="hai: there">');
            (0, vitest_1.expect)(el.css('hai')).toBe('there');
        });
        (0, vitest_1.it)('([prop1, prop2]): should return the specified property values as an object', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="margin: 1px; padding: 2px; color: blue;">');
            (0, vitest_1.expect)(el.css(['margin', 'color'])).toStrictEqual({
                margin: '1px',
                color: 'blue',
            });
        });
        (0, vitest_1.it)('(prop, val): should set a css property', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="margin: 0;"></li><li></li>');
            el.css('color', 'red');
            (0, vitest_1.expect)(el.attr('style')).toBe('margin: 0; color: red;');
            (0, vitest_1.expect)(el.eq(1).attr('style')).toBe('color: red;');
        });
        (0, vitest_1.it)('(prop, val) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.css('test', 'value');
            (0, vitest_1.expect)($text('body').html()).toBe('<a style="test: value;">1</a>TEXT<b style="test: value;">2</b>');
        });
        (0, vitest_1.it)('(prop, ""): should unset a css property', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="padding: 1px; margin: 0;">');
            el.css('padding', '');
            (0, vitest_1.expect)(el.attr('style')).toBe('margin: 0;');
        });
        (0, vitest_1.it)('(any, val): should ignore unsupported prop types', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="padding: 1px;">');
            el.css(123, 'test');
            (0, vitest_1.expect)(el.attr('style')).toBe('padding: 1px;');
        });
        (0, vitest_1.it)('(prop): should not mangle embedded urls', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="background-image:url(http://example.com/img.png);">');
            (0, vitest_1.expect)(el.css('background-image')).toBe('url(http://example.com/img.png)');
        });
        (0, vitest_1.it)('(prop): should ignore blank properties', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style=":#ccc;color:#aaa;">');
            (0, vitest_1.expect)(el.css()).toStrictEqual({ color: '#aaa' });
        });
        (0, vitest_1.it)('(prop): should ignore blank values', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="color:;position:absolute;">');
            (0, vitest_1.expect)(el.css()).toStrictEqual({ position: 'absolute' });
        });
        (0, vitest_1.it)('(prop): should return undefined for unmatched elements', () => {
            const $ = (0, index_js_1.load)('<li style="color:;position:absolute;">');
            (0, vitest_1.expect)($('ul').css('background-image')).toBeUndefined();
        });
        (0, vitest_1.it)('(prop): should return undefined for unmatched styles', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="color:;position:absolute;">');
            (0, vitest_1.expect)(el.css('margin')).toBeUndefined();
        });
        (0, vitest_1.describe)('(prop, function):', () => {
            let $el;
            (0, vitest_1.beforeEach)(() => {
                const $ = (0, index_js_1.load)('<div style="margin: 0px;"></div><div style="margin: 1px;"></div><div style="margin: 2px;">');
                $el = $('div');
            });
            (0, vitest_1.it)('should iterate over the selection', () => {
                let count = 0;
                $el.css('margin', function (idx, value) {
                    (0, vitest_1.expect)(idx).toBe(count);
                    (0, vitest_1.expect)(value).toBe(`${count}px`);
                    (0, vitest_1.expect)(this).toBe($el[count]);
                    count++;
                    return;
                });
                (0, vitest_1.expect)(count).toBe(3);
            });
            (0, vitest_1.it)('should set each attribute independently', () => {
                const values = ['4px', '', undefined];
                $el.css('margin', (idx) => values[idx]);
                (0, vitest_1.expect)($el.eq(0).attr('style')).toBe('margin: 4px;');
                (0, vitest_1.expect)($el.eq(1).attr('style')).toBe('');
                (0, vitest_1.expect)($el.eq(2).attr('style')).toBe('margin: 2px;');
            });
        });
        (0, vitest_1.it)('(obj): should set each key and val', () => {
            const el = (0, fixtures_js_1.cheerio)('<li style="padding: 0;"></li><li></li>');
            el.css({ foo: 0 });
            (0, vitest_1.expect)(el.eq(0).attr('style')).toBe('padding: 0; foo: 0;');
            (0, vitest_1.expect)(el.eq(1).attr('style')).toBe('foo: 0;');
        });
        (0, vitest_1.describe)('parser', () => {
            (0, vitest_1.it)('should allow any whitespace between declarations', () => {
                const el = (0, fixtures_js_1.cheerio)('<li style="one \t:\n 0;\n two \f\r:\v 1">');
                (0, vitest_1.expect)(el.css(['one', 'two', 'five'])).toStrictEqual({
                    one: '0',
                    two: '1',
                });
            });
            (0, vitest_1.it)('should add malformed values to previous field (#1134)', () => {
                const el = (0, fixtures_js_1.cheerio)('<button style="background-image: url(data:image/png;base64,iVBORw0KGgo)"></button>');
                (0, vitest_1.expect)(el.css('background-image')).toStrictEqual('url(data:image/png;base64,iVBORw0KGgo)');
            });
        });
    });
});
