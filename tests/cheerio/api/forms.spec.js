"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const fixtures_js_1 = require("../__fixtures__/fixtures.js");
(0, vitest_1.describe)('$(...)', () => {
    let $;
    (0, vitest_1.beforeEach)(() => {
        $ = fixtures_js_1.cheerio.load(fixtures_js_1.forms);
    });
    (0, vitest_1.describe)('.serializeArray', () => {
        (0, vitest_1.it)('() : should get form controls', () => {
            (0, vitest_1.expect)($('form#simple').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
            ]);
        });
        (0, vitest_1.it)('() : should get nested form controls', () => {
            (0, vitest_1.expect)($('form#nested').serializeArray()).toHaveLength(2);
            const data = $('form#nested').serializeArray();
            data.sort((a, b) => a.value.localeCompare(b.value));
            (0, vitest_1.expect)(data).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
                {
                    name: 'vegetable',
                    value: 'Carrot',
                },
            ]);
        });
        (0, vitest_1.it)('() : should not get disabled form controls', () => {
            (0, vitest_1.expect)($('form#disabled').serializeArray()).toStrictEqual([]);
        });
        (0, vitest_1.it)('() : should not get form controls with the wrong type', () => {
            (0, vitest_1.expect)($('form#submit').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
            ]);
        });
        (0, vitest_1.it)('() : should get selected options', () => {
            (0, vitest_1.expect)($('form#select').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Orange',
                },
            ]);
        });
        (0, vitest_1.it)('() : should not get unnamed form controls', () => {
            (0, vitest_1.expect)($('form#unnamed').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
            ]);
        });
        (0, vitest_1.it)('() : should get multiple selected options', () => {
            (0, vitest_1.expect)($('form#multiple').serializeArray()).toHaveLength(2);
            const data = $('form#multiple').serializeArray();
            data.sort((a, b) => a.value.localeCompare(b.value));
            (0, vitest_1.expect)(data).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
                {
                    name: 'fruit',
                    value: 'Orange',
                },
            ]);
        });
        (0, vitest_1.it)('() : should get individually selected elements', () => {
            const data = $('form#nested input').serializeArray();
            data.sort((a, b) => a.value.localeCompare(b.value));
            (0, vitest_1.expect)(data).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'Apple',
                },
                {
                    name: 'vegetable',
                    value: 'Carrot',
                },
            ]);
        });
        (0, vitest_1.it)('() : should standardize line breaks', () => {
            (0, vitest_1.expect)($('form#textarea').serializeArray()).toStrictEqual([
                {
                    name: 'fruits',
                    value: 'Apple\r\nOrange',
                },
            ]);
        });
        (0, vitest_1.it)("() : shouldn't serialize the empty string", () => {
            (0, vitest_1.expect)($('<input value=pineapple>').serializeArray()).toStrictEqual([]);
            (0, vitest_1.expect)($('<input name="" value=pineapple>').serializeArray()).toStrictEqual([]);
            (0, vitest_1.expect)($('<input name="fruit" value=pineapple>').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: 'pineapple',
                },
            ]);
        });
        (0, vitest_1.it)('() : should serialize inputs without value attributes', () => {
            (0, vitest_1.expect)($('<input name="fruit">').serializeArray()).toStrictEqual([
                {
                    name: 'fruit',
                    value: '',
                },
            ]);
        });
    });
    (0, vitest_1.describe)('.serialize', () => {
        (0, vitest_1.it)('() : should get form controls', () => {
            (0, vitest_1.expect)($('form#simple').serialize()).toBe('fruit=Apple');
        });
        (0, vitest_1.it)('() : should get nested form controls', () => {
            (0, vitest_1.expect)($('form#nested').serialize()).toBe('fruit=Apple&vegetable=Carrot');
        });
        (0, vitest_1.it)('() : should not get disabled form controls', () => {
            (0, vitest_1.expect)($('form#disabled').serialize()).toBe('');
        });
        (0, vitest_1.it)('() : should get multiple selected options', () => {
            (0, vitest_1.expect)($('form#multiple').serialize()).toBe('fruit=Apple&fruit=Orange');
        });
        (0, vitest_1.it)("() : should encode spaces as +'s", () => {
            (0, vitest_1.expect)($('form#spaces').serialize()).toBe('fruit=Blood+orange');
        });
    });
});
