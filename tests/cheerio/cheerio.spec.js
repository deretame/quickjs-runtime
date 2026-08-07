"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const htmlparser2_1 = require("htmlparser2");
const vitest_1 = require("vitest");
const fixtures_js_1 = require("./__fixtures__/fixtures.js");
function testAppleSelect($apple) {
    (0, vitest_1.expect)($apple).toHaveLength(1);
    const apple = $apple[0];
    (0, vitest_1.expect)(apple.parentNode).toHaveProperty('tagName', 'ul');
    (0, vitest_1.expect)(apple.prev).toBe(null);
    (0, vitest_1.expect)(apple.next.attribs).toHaveProperty('class', 'orange');
    (0, vitest_1.expect)(apple.childNodes).toHaveLength(1);
    (0, vitest_1.expect)(apple.childNodes[0]).toHaveProperty('data', 'Apple');
}
(0, vitest_1.describe)('cheerio', () => {
    (0, vitest_1.it)('cheerio(null) should be empty', () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)(null)).toHaveLength(0);
    });
    (0, vitest_1.it)('cheerio(undefined) should be empty', () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)(undefined)).toHaveLength(0);
    });
    (0, vitest_1.it)("cheerio('') should be empty", () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('')).toHaveLength(0);
    });
    (0, vitest_1.it)('cheerio(selector) with no context or root should be empty', () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('.h2')).toHaveLength(0);
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('#fruits')).toHaveLength(0);
    });
    (0, vitest_1.it)('cheerio(node) : should override previously-loaded nodes', () => {
        const $ = fixtures_js_1.cheerio.load('<div><span></span></div>');
        const spanNode = $('span')[0];
        const $span = $(spanNode);
        (0, vitest_1.expect)($span[0]).toBe(spanNode);
    });
    (0, vitest_1.it)('should be able to create html without a root or context', () => {
        const $h2 = (0, fixtures_js_1.cheerio)('<h2>');
        (0, vitest_1.expect)($h2).not.toHaveLength(0);
        (0, vitest_1.expect)($h2).toHaveLength(1);
        (0, vitest_1.expect)($h2[0]).toHaveProperty('tagName', 'h2');
    });
    (0, vitest_1.it)('should be able to create complicated html', () => {
        const $script = (0, fixtures_js_1.cheerio)('<script src="script.js" type="text/javascript"></script>');
        (0, vitest_1.expect)($script).not.toHaveLength(0);
        (0, vitest_1.expect)($script).toHaveLength(1);
        (0, vitest_1.expect)($script[0].attribs).toHaveProperty('src', 'script.js');
        (0, vitest_1.expect)($script[0].attribs).toHaveProperty('type', 'text/javascript');
        (0, vitest_1.expect)($script[0].childNodes).toHaveLength(0);
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to select .apple with only a context', () => {
        const $apple = (0, fixtures_js_1.cheerio)('.apple', fixtures_js_1.fruits);
        testAppleSelect($apple);
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to select .apple with a node as context', () => {
        const $apple = (0, fixtures_js_1.cheerio)('.apple', (0, fixtures_js_1.cheerio)(fixtures_js_1.fruits)[0]);
        testAppleSelect($apple);
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to select .apple with only a root', () => {
        const $apple = (0, fixtures_js_1.cheerio)('.apple', null, fixtures_js_1.fruits);
        testAppleSelect($apple);
    });
    (0, vitest_1.it)('should be able to select an id', () => {
        const $fruits = (0, fixtures_js_1.cheerio)('#fruits', null, fixtures_js_1.fruits);
        (0, vitest_1.expect)($fruits).toHaveLength(1);
        (0, vitest_1.expect)($fruits[0].attribs).toHaveProperty('id', 'fruits');
    });
    (0, vitest_1.it)('should be able to select a tag', () => {
        const $ul = (0, fixtures_js_1.cheerio)('ul', fixtures_js_1.fruits);
        (0, vitest_1.expect)($ul).toHaveLength(1);
        (0, vitest_1.expect)($ul[0].tagName).toBe('ul');
    });
    (0, vitest_1.it)('should accept a node reference as a context', () => {
        const $elems = (0, fixtures_js_1.cheerio)('<div><span></span></div>');
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('span', $elems[0])).toHaveLength(1);
    });
    (0, vitest_1.it)('should accept an array of node references as a context', () => {
        const $elems = (0, fixtures_js_1.cheerio)('<div><span></span></div>');
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('span', $elems.toArray())).toHaveLength(1);
    });
    (0, vitest_1.it)('should select only elements inside given context (Issue #193)', () => {
        const $ = fixtures_js_1.cheerio.load(fixtures_js_1.food);
        const $fruits = $('#fruits');
        const fruitElements = $('li', $fruits);
        (0, vitest_1.expect)(fruitElements).toHaveLength(3);
    });
    (0, vitest_1.it)('should be able to select multiple tags', () => {
        const $fruits = (0, fixtures_js_1.cheerio)('li', null, fixtures_js_1.fruits);
        (0, vitest_1.expect)($fruits).toHaveLength(3);
        const classes = ['apple', 'orange', 'pear'];
        $fruits.each((idx, $fruit) => {
            (0, vitest_1.expect)($fruit.attribs).toHaveProperty('class', classes[idx]);
        });
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to do: cheerio("#fruits .apple")', () => {
        const $apple = (0, fixtures_js_1.cheerio)('#fruits .apple', fixtures_js_1.fruits);
        testAppleSelect($apple);
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to do: cheerio("li.apple")', () => {
        const $apple = (0, fixtures_js_1.cheerio)('li.apple', fixtures_js_1.fruits);
        testAppleSelect($apple);
    });
    // eslint-disable-next-line vitest/expect-expect
    (0, vitest_1.it)('should be able to select by attributes', () => {
        const $apple = (0, fixtures_js_1.cheerio)('li[class=apple]', fixtures_js_1.fruits);
        testAppleSelect($apple);
    });
    (0, vitest_1.it)('should be able to select multiple classes: cheerio(".btn.primary")', () => {
        const $a = (0, fixtures_js_1.cheerio)('.btn.primary', '<p><a class="btn primary" href="#">Save</a></p>');
        (0, vitest_1.expect)($a).toHaveLength(1);
        (0, vitest_1.expect)($a[0].childNodes[0]).toHaveProperty('data', 'Save');
    });
    (0, vitest_1.it)('should not create a top-level node', () => {
        const $elem = (0, fixtures_js_1.cheerio)('* div', '<div>');
        (0, vitest_1.expect)($elem).toHaveLength(0);
    });
    (0, vitest_1.it)('should be able to select multiple elements: cheerio(".apple, #fruits")', () => {
        const $elems = (0, fixtures_js_1.cheerio)('.apple, #fruits', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elems).toHaveLength(2);
        const $apple = $elems
            .toArray()
            .filter((elem) => elem.attribs['class'] === 'apple');
        const $fruit = $elems
            .toArray()
            .find((elem) => elem.attribs['id'] === 'fruits');
        testAppleSelect($apple);
        (0, vitest_1.expect)($fruit?.attribs).toHaveProperty('id', 'fruits');
    });
    (0, vitest_1.it)('should select first element cheerio(:first)', () => {
        const $elem = (0, fixtures_js_1.cheerio)('li:first', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elem.attr('class')).toBe('apple');
        const $filtered = (0, fixtures_js_1.cheerio)('li', fixtures_js_1.fruits).filter(':even');
        (0, vitest_1.expect)($filtered).toHaveLength(2);
    });
    (0, vitest_1.it)('should be able to select immediate children: cheerio("#fruits > .pear")', () => {
        const $food = (0, fixtures_js_1.cheerio)(fixtures_js_1.food);
        (0, fixtures_js_1.cheerio)('.pear', $food).append('<li class="pear">Another Pear!</li>');
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('#fruits .pear', $food)).toHaveLength(2);
        const $elem = (0, fixtures_js_1.cheerio)('#fruits > .pear', $food);
        (0, vitest_1.expect)($elem).toHaveLength(1);
        (0, vitest_1.expect)($elem.attr('class')).toBe('pear');
    });
    (0, vitest_1.it)('should be able to select immediate children: cheerio(".apple + .pear")', () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('.apple + li', fixtures_js_1.fruits)).toHaveLength(1);
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('.apple + .pear', fixtures_js_1.fruits)).toHaveLength(0);
        const $elem = (0, fixtures_js_1.cheerio)('.apple + .orange', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elem).toHaveLength(1);
        (0, vitest_1.expect)($elem.attr('class')).toBe('orange');
    });
    (0, vitest_1.it)('should be able to select immediate children: cheerio(".apple ~ .pear")', () => {
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('.apple ~ li', fixtures_js_1.fruits)).toHaveLength(2);
        (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('.apple ~ .pear', fixtures_js_1.fruits).attr('class')).toBe('pear');
    });
    (0, vitest_1.it)('should handle wildcards on attributes: cheerio("li[class*=r]")', () => {
        const $elem = (0, fixtures_js_1.cheerio)('li[class*=r]', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elem).toHaveLength(2);
        (0, vitest_1.expect)($elem.eq(0).attr('class')).toBe('orange');
        (0, vitest_1.expect)($elem.eq(1).attr('class')).toBe('pear');
    });
    (0, vitest_1.it)('should handle beginning of attr selectors: cheerio("li[class^=o]")', () => {
        const $elem = (0, fixtures_js_1.cheerio)('li[class^=o]', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elem).toHaveLength(1);
        (0, vitest_1.expect)($elem.eq(0).attr('class')).toBe('orange');
    });
    (0, vitest_1.it)('should handle beginning of attr selectors: cheerio("li[class$=e]")', () => {
        const $elem = (0, fixtures_js_1.cheerio)('li[class$=e]', fixtures_js_1.fruits);
        (0, vitest_1.expect)($elem).toHaveLength(2);
        (0, vitest_1.expect)($elem.eq(0).attr('class')).toBe('apple');
        (0, vitest_1.expect)($elem.eq(1).attr('class')).toBe('orange');
    });
    (0, vitest_1.it)('(extended Array) should not interfere with prototype methods (issue #119)', () => {
        const extended = [];
        // @ts-expect-error - Ignore for testing
        extended.find =
            // @ts-expect-error - Ignore for testing
            extended.children =
                // @ts-expect-error - Ignore for testing
                extended.each =
                    () => {
                        /* Ignore */
                    };
        const $empty = (0, fixtures_js_1.cheerio)(extended);
        // eslint-disable-next-line @typescript-eslint/no-unsafe-member-access
        (0, vitest_1.expect)($empty.find).toBe(fixtures_js_1.cheerio.prototype.find);
        // eslint-disable-next-line @typescript-eslint/no-unsafe-member-access
        (0, vitest_1.expect)($empty.children).toBe(fixtures_js_1.cheerio.prototype.children);
        // eslint-disable-next-line @typescript-eslint/no-unsafe-member-access
        (0, vitest_1.expect)($empty.each).toBe(fixtures_js_1.cheerio.prototype.each);
    });
    (0, vitest_1.it)('cheerio.html(null) should return a "" string', () => {
        (0, vitest_1.expect)(fixtures_js_1.cheerio.html(null)).toBe('');
    });
    (0, vitest_1.it)('should set html(number) as a string', () => {
        const $elem = (0, fixtures_js_1.cheerio)('<div>');
        $elem.html(123);
        (0, vitest_1.expect)(typeof $elem.text()).toBe('string');
    });
    (0, vitest_1.it)('should set text(number) as a string', () => {
        const $elem = (0, fixtures_js_1.cheerio)('<div>');
        $elem.text(123);
        (0, vitest_1.expect)(typeof $elem.text()).toBe('string');
    });
    (0, vitest_1.describe)('.load', () => {
        (0, vitest_1.it)('should generate selections as proper instances', () => {
            const $ = fixtures_js_1.cheerio.load(fixtures_js_1.fruits);
            (0, vitest_1.expect)($('.apple')).toBeInstanceOf($);
        });
        // Issue #1092
        (0, vitest_1.it)('should handle a character `)` in `:contains` selector', () => {
            const result = fixtures_js_1.cheerio.load('<p>)aaa</p>')(String.raw `:contains('\)aaa')`);
            (0, vitest_1.expect)(result).toHaveLength(3);
            (0, vitest_1.expect)(result.first().prop('tagName')).toBe('HTML');
            (0, vitest_1.expect)(result.eq(1).prop('tagName')).toBe('BODY');
            (0, vitest_1.expect)(result.last().prop('tagName')).toBe('P');
        });
        (0, vitest_1.it)('should be able to filter down using the context', () => {
            const $ = fixtures_js_1.cheerio.load(fixtures_js_1.fruits);
            const apple = $('.apple', 'ul');
            const lis = $('li', 'ul');
            (0, vitest_1.expect)(apple).toHaveLength(1);
            (0, vitest_1.expect)(lis).toHaveLength(3);
        });
        (0, vitest_1.it)('should preserve root content', () => {
            const $ = fixtures_js_1.cheerio.load(fixtures_js_1.fruits);
            // Root should not be overwritten
            const el = $('<div></div>');
            (0, vitest_1.expect)(Object.is(el, el._root)).toBe(false);
            // Query has to have results
            (0, vitest_1.expect)($('li', 'ul')).toHaveLength(3);
        });
        (0, vitest_1.it)('should allow loading a pre-parsed DOM', () => {
            const dom = (0, htmlparser2_1.parseDOM)(fixtures_js_1.food);
            const $ = fixtures_js_1.cheerio.load(dom);
            (0, vitest_1.expect)($('ul')).toHaveLength(3);
        });
        (0, vitest_1.it)('should allow loading a single element', () => {
            const el = (0, htmlparser2_1.parseDOM)(fixtures_js_1.food)[0];
            const $ = fixtures_js_1.cheerio.load(el);
            (0, vitest_1.expect)($('ul')).toHaveLength(3);
        });
        (0, vitest_1.it)('should render xml in html() when options.xml = true', () => {
            const str = '<MixedCaseTag UPPERCASEATTRIBUTE=""></MixedCaseTag>';
            const expected = '<MixedCaseTag UPPERCASEATTRIBUTE=""/>';
            const $ = fixtures_js_1.cheerio.load(str, { xml: true });
            (0, vitest_1.expect)($('MixedCaseTag').get(0)).toHaveProperty('tagName', 'MixedCaseTag');
            (0, vitest_1.expect)($.html()).toBe(expected);
        });
        (0, vitest_1.it)('should render xml in html() when options.xml = true passed to html()', () => {
            const str = '<MixedCaseTag UPPERCASEATTRIBUTE=""></MixedCaseTag>';
            // Since parsing done without xml flag, all tags converted to lowercase
            const expectedXml = '<html><head/><body><mixedcasetag uppercaseattribute=""/></body></html>';
            const expectedNoXml = '<html><head></head><body><mixedcasetag uppercaseattribute=""></mixedcasetag></body></html>';
            const $ = fixtures_js_1.cheerio.load(str);
            (0, vitest_1.expect)($('MixedCaseTag').get(0)).toHaveProperty('tagName', 'mixedcasetag');
            (0, vitest_1.expect)($.html()).toBe(expectedNoXml);
            (0, vitest_1.expect)($.html({ xml: true })).toBe(expectedXml);
        });
        (0, vitest_1.it)('should respect options on the element level', () => {
            const str = '<!doctype html><html><head><title>Some test</title></head><body><footer><p>Copyright &copy; 2003-2014</p></footer></body></html>';
            const expectedHtml = '<p>Copyright &copy; 2003-2014</p>';
            const expectedXml = '<p>Copyright © 2003-2014</p>';
            const domNotEncoded = fixtures_js_1.cheerio.load(str, {
                xml: { decodeEntities: false },
            });
            const domEncoded = fixtures_js_1.cheerio.load(str);
            (0, vitest_1.expect)(domNotEncoded('footer').html()).toBe(expectedHtml);
            (0, vitest_1.expect)(domEncoded('footer').html()).toBe(expectedXml);
        });
        (0, vitest_1.it)('should use htmlparser2 if xml option is used', () => {
            const str = '<div></div>';
            const dom = fixtures_js_1.cheerio.load(str, null, false);
            (0, vitest_1.expect)(dom.html()).toBe(str);
        });
        (0, vitest_1.it)('should return a fully-qualified Function', () => {
            const $ = fixtures_js_1.cheerio.load('<div>');
            (0, vitest_1.expect)($).toBeInstanceOf(Function);
        });
        (0, vitest_1.describe)('prototype extensions', () => {
            (0, vitest_1.it)('should honor extensions defined on `prototype` property', () => {
                const $ = fixtures_js_1.cheerio.load('<div>');
                $.prototype.myPlugin = function (...args) {
                    return {
                        context: this,
                        args,
                    };
                };
                const $div = $('div');
                (0, vitest_1.expect)(typeof $div.myPlugin).toBe('function');
                (0, vitest_1.expect)($div.myPlugin().context).toBe($div);
                (0, vitest_1.expect)($div.myPlugin(1, 2, 3).args).toStrictEqual([1, 2, 3]);
            });
            (0, vitest_1.it)('should honor extensions defined on `fn` property', () => {
                const $ = fixtures_js_1.cheerio.load('<div>');
                $.fn.myPlugin = function (...args) {
                    return {
                        context: this,
                        args,
                    };
                };
                const $div = $('div');
                (0, vitest_1.expect)(typeof $div.myPlugin).toBe('function');
                (0, vitest_1.expect)($div.myPlugin().context).toBe($div);
                (0, vitest_1.expect)($div.myPlugin(1, 2, 3).args).toStrictEqual([1, 2, 3]);
            });
            (0, vitest_1.it)('should isolate extensions between loaded functions', () => {
                const $a = fixtures_js_1.cheerio.load('<div>');
                const $b = fixtures_js_1.cheerio.load('<div>');
                $a.prototype.foo = () => {
                    /* Ignore */
                };
                (0, vitest_1.expect)($b('div').foo).toBeUndefined();
            });
        });
    });
    (0, vitest_1.describe)('parse5 options', () => {
        // Should parse noscript tags only with false option value
        (0, vitest_1.it)('{scriptingEnabled: ???}', () => {
            // [default] `scriptingEnabled: true` - tag contains one text element
            const withScripts = fixtures_js_1.cheerio.load(fixtures_js_1.noscript)('noscript');
            (0, vitest_1.expect)(withScripts).toHaveLength(1);
            (0, vitest_1.expect)(withScripts[0].children).toHaveLength(1);
            (0, vitest_1.expect)(withScripts[0].children[0].type).toBe('text');
            // `scriptingEnabled: false` - content of noscript will parsed
            const noScripts = fixtures_js_1.cheerio.load(fixtures_js_1.noscript, { scriptingEnabled: false })('noscript');
            (0, vitest_1.expect)(noScripts).toHaveLength(1);
            (0, vitest_1.expect)(noScripts[0].children).toHaveLength(2);
            (0, vitest_1.expect)(noScripts[0].children[0].type).toBe('comment');
            (0, vitest_1.expect)(noScripts[0].children[1].type).toBe('tag');
            (0, vitest_1.expect)(noScripts[0].children[1]).toHaveProperty('name', 'a');
            // `scriptingEnabled: ???` - should acts as true
            for (const val of [undefined, null, 0, '']) {
                const options = { scriptingEnabled: val };
                const result = fixtures_js_1.cheerio.load(fixtures_js_1.noscript, options)('noscript');
                (0, vitest_1.expect)(result).toHaveLength(1);
                (0, vitest_1.expect)(result[0].children).toHaveLength(1);
                (0, vitest_1.expect)(result[0].children[0].type).toBe('text');
            }
        });
        // Should contain location data only with truthful option value
        (0, vitest_1.it)('{sourceCodeLocationInfo: ???}', () => {
            // Location data should not be present
            for (const val of [undefined, null, 0, false, '']) {
                const options = { sourceCodeLocationInfo: val };
                const result = fixtures_js_1.cheerio.load(fixtures_js_1.noscript, options)('noscript');
                (0, vitest_1.expect)(result).toHaveLength(1);
                (0, vitest_1.expect)(result[0]).not.toHaveProperty('sourceCodeLocation');
            }
            // Location data should be present
            for (const val of [true, 1, 'test']) {
                const options = { sourceCodeLocationInfo: val };
                const result = fixtures_js_1.cheerio.load(fixtures_js_1.noscript, options)('noscript');
                (0, vitest_1.expect)(result).toHaveLength(1);
                (0, vitest_1.expect)(result[0]).toHaveProperty('sourceCodeLocation');
                (0, vitest_1.expect)(typeof result[0].sourceCodeLocation).toBe('object');
            }
        });
    });
});
