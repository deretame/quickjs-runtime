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
const vitest_1 = require("vitest");
const fixtures = __importStar(require("../__fixtures__/fixtures.js"));
const load_parse_js_1 = require("../load-parse.js");
(0, vitest_1.describe)('$.extract', () => {
    (0, vitest_1.it)('should return an empty object when no selectors are provided', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({})).toEqualTypeOf();
        const emptyExtract = $root.extract({});
        (0, vitest_1.expect)(emptyExtract).toStrictEqual({});
    });
    (0, vitest_1.it)('should return undefined for selectors that do not match any elements', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({ foo: 'bar' })).toEqualTypeOf();
        const simpleExtract = $root.extract({ foo: 'bar' });
        (0, vitest_1.expect)(simpleExtract).toStrictEqual({ foo: undefined });
    });
    (0, vitest_1.it)('should extract values for existing selectors', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({ red: '.red' })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({ red: '.red' })).toStrictEqual({ red: 'Four' });
        (0, vitest_1.expectTypeOf)($root.extract({ red: '.red', sel: '.sel' })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({ red: '.red', sel: '.sel' })).toStrictEqual({
            red: 'Four',
            sel: 'Three',
        });
    });
    (0, vitest_1.it)('should extract values using descriptor objects', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: { selector: '.red' },
            sel: { selector: '.sel' },
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: { selector: '.red' },
            sel: { selector: '.sel' },
        })).toStrictEqual({ red: 'Four', sel: 'Three' });
    });
    (0, vitest_1.it)('should extract multiple values for selectors', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: ['.red'],
            sel: ['.sel'],
        })).toEqualTypeOf();
        const multipleExtract = $root.extract({
            red: ['.red'],
            sel: ['.sel'],
        });
        (0, vitest_1.expectTypeOf)(multipleExtract).toEqualTypeOf();
        (0, vitest_1.expect)(multipleExtract).toStrictEqual({
            red: ['Four', 'Five', 'Nine'],
            sel: ['Three', 'Nine', 'Eleven'],
        });
    });
    (0, vitest_1.it)('should extract custom properties specified by the user', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: { selector: '.red', value: 'outerHTML' },
            sel: { selector: '.sel', value: 'tagName' },
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: { selector: '.red', value: 'outerHTML' },
            sel: { selector: '.sel', value: 'tagName' },
        })).toStrictEqual({ red: '<li class="red">Four</li>', sel: 'LI' });
    });
    (0, vitest_1.it)('should extract multiple custom properties for selectors', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: [{ selector: '.red', value: 'outerHTML' }],
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: [{ selector: '.red', value: 'outerHTML' }],
        })).toStrictEqual({
            red: [
                '<li class="red">Four</li>',
                '<li class="red">Five</li>',
                '<li class="red sel">Nine</li>',
            ],
        });
    });
    (0, vitest_1.it)('should extract values using custom extraction functions', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: {
                selector: '.red',
                value: (el, key) => `${key}=${$(el).text()}`,
            },
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: {
                selector: '.red',
                value: (el, key) => `${key}=${$(el).text()}`,
            },
        })).toStrictEqual({ red: 'red=Four' });
    });
    (0, vitest_1.it)('should correctly type check custom extraction functions returning non-string values', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: {
                selector: '.red',
                value: (el) => $(el).text().length,
            },
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: {
                selector: '.red',
                value: (el) => $(el).text().length,
            },
        })).toStrictEqual({ red: 4 });
    });
    (0, vitest_1.it)('should extract multiple values using custom extraction functions', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            red: [
                {
                    selector: '.red',
                    value: (el, key) => `${key}=${$(el).text()}`,
                },
            ],
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            red: [
                {
                    selector: '.red',
                    value: (el, key) => `${key}=${$(el).text()}`,
                },
            ],
        })).toStrictEqual({ red: ['red=Four', 'red=Five', 'red=Nine'] });
    });
    (0, vitest_1.it)('should extract nested objects based on selectors', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            section: {
                selector: 'ul:nth(1)',
                value: {
                    red: '.red',
                    sel: '.blue',
                },
            },
        })).toEqualTypeOf();
        const subExtractObject = $root.extract({
            section: {
                selector: 'ul:nth(1)',
                value: {
                    red: '.red',
                    sel: '.blue',
                },
            },
        });
        (0, vitest_1.expectTypeOf)(subExtractObject).toEqualTypeOf();
        (0, vitest_1.expect)(subExtractObject).toStrictEqual({
            section: {
                red: 'Five',
                sel: 'Seven',
            },
        });
    });
    (0, vitest_1.it)('should correctly type check nested objects returning non-string values', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        const $root = $.root();
        (0, vitest_1.expectTypeOf)($root.extract({
            section: {
                selector: 'ul:nth(1)',
                value: {
                    red: {
                        selector: '.red',
                        value: (el) => $(el).text().length,
                    },
                },
            },
        })).toEqualTypeOf();
        (0, vitest_1.expect)($root.extract({
            section: {
                selector: 'ul:nth(1)',
                value: {
                    red: {
                        selector: '.red',
                        value: (el) => $(el).text().length,
                    },
                },
            },
        })).toStrictEqual({
            section: {
                red: 4,
            },
        });
    });
    (0, vitest_1.it)('should handle missing href properties without errors (#4239)', () => {
        const $ = (0, load_parse_js_1.load)(fixtures.eleven);
        (0, vitest_1.expect)($.extract({ links: [{ selector: 'li', value: 'href' }] })).toStrictEqual({ links: [] });
    });
});
