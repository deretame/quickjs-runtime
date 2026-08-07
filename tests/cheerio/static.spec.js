"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const fixtures_js_1 = require("./__fixtures__/fixtures.js");
(0, vitest_1.describe)('cheerio', () => {
    (0, vitest_1.describe)('.html', () => {
        (0, vitest_1.it)('() : should return innerHTML; $.html(obj) should return outerHTML', () => {
            const $div = (0, fixtures_js_1.cheerio)('div', '<div><span>foo</span><span>bar</span></div>');
            const span = $div.children()[1];
            (0, vitest_1.expect)((0, fixtures_js_1.cheerio)(span).html()).toBe('bar');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html(span)).toBe('<span>bar</span>');
        });
        (0, vitest_1.it)('(<obj>) : should accept an object, an array, or a cheerio object', () => {
            const $span = (0, fixtures_js_1.cheerio)('<span>foo</span>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html($span[0])).toBe('<span>foo</span>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html($span)).toBe('<span>foo</span>');
        });
        (0, vitest_1.it)('(<value>) : should be able to set to an empty string', () => {
            const $elem = (0, fixtures_js_1.cheerio)('<span>foo</span>').html('');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html($elem)).toBe('<span></span>');
        });
        (0, vitest_1.it)('(<root>) : does not render the root element', () => {
            const $ = fixtures_js_1.cheerio.load('');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html($.root())).toBe('<html><head></head><body></body></html>');
        });
        (0, vitest_1.it)('(<elem>, <root>, <elem>) : does not render the root element', () => {
            const $ = fixtures_js_1.cheerio.load('<div>a div</div><span>a span</span>');
            const $collection = $('div').add($.root()).add('span');
            const expected = '<html><head></head><body><div>a div</div><span>a span</span></body></html><div>a div</div><span>a span</span>';
            (0, vitest_1.expect)(fixtures_js_1.cheerio.html($collection)).toBe(expected);
        });
        (0, vitest_1.it)('(<opts>) : keeps using the instance parser when serializing', () => {
            // An htmlparser2-backed instance, rendered as HTML rather than XML.
            const $ = fixtures_js_1.cheerio.load('<style>a < b && c > d</style><p>x<br>y</p>', {
                xmlMode: true,
            });
            (0, vitest_1.expect)($.html(undefined, { xmlMode: false })).toBe('<style>a < b && c > d</style><p>x<br>y</p>');
        });
        (0, vitest_1.it)('() : does not crash with `null` as `this` value', () => {
            const { html } = fixtures_js_1.cheerio;
            (0, vitest_1.expect)(html.call(null)).toBe('');
            (0, vitest_1.expect)(html.call(null, '#nothing')).toBe('');
        });
    });
    (0, vitest_1.describe)('.text', () => {
        (0, vitest_1.it)('(cheerio object) : should return the text contents of the specified elements', () => {
            const $ = fixtures_js_1.cheerio.load('<a>This is <em>content</em>.</a>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.text($('a'))).toBe('This is content.');
        });
        (0, vitest_1.it)('(cheerio object) : should omit comment nodes', () => {
            const $ = fixtures_js_1.cheerio.load('<a>This is <!-- a comment --> not a comment.</a>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.text($('a'))).toBe('This is  not a comment.');
        });
        (0, vitest_1.it)('(cheerio object) : should include text contents of children recursively', () => {
            const $ = fixtures_js_1.cheerio.load('<a>This is <div>a child with <span>another child and <!-- a comment --> not a comment</span> followed by <em>one last child</em> and some final</div> text.</a>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.text($('a'))).toBe('This is a child with another child and  not a comment followed by one last child and some final text.');
        });
        (0, vitest_1.it)('() : should return the rendered text content of the root', () => {
            const $ = fixtures_js_1.cheerio.load('<a>This is <div>a child with <span>another child and <!-- a comment --> not a comment</span> followed by <em>one last child</em> and some final</div> text.</a>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.text($.root())).toBe('This is a child with another child and  not a comment followed by one last child and some final text.');
        });
        (0, vitest_1.it)('(cheerio object) : should not omit script tags', () => {
            const $ = fixtures_js_1.cheerio.load('<script>console.log("test")</script>');
            (0, vitest_1.expect)(fixtures_js_1.cheerio.text($.root())).toBe('console.log("test")');
        });
        (0, vitest_1.it)('(cheerio object) : should omit style tags', () => {
            const $ = fixtures_js_1.cheerio.load('<style type="text/css">.cf-hidden { display: none; }</style>');
            (0, vitest_1.expect)($.text()).toBe('.cf-hidden { display: none; }');
        });
        (0, vitest_1.it)('() : does not crash with `null` as `this` value', () => {
            const { text } = fixtures_js_1.cheerio;
            (0, vitest_1.expect)(text.call(null)).toBe('');
        });
    });
    (0, vitest_1.describe)('.parseHTML', () => {
        const $ = fixtures_js_1.cheerio.load('');
        (0, vitest_1.it)('() : returns null', () => {
            (0, vitest_1.expect)($.parseHTML()).toBe(null);
        });
        (0, vitest_1.it)('(null) : returns null', () => {
            (0, vitest_1.expect)($.parseHTML(null)).toBe(null);
        });
        (0, vitest_1.it)('("") : returns null', () => {
            (0, vitest_1.expect)($.parseHTML('')).toBe(null);
        });
        (0, vitest_1.it)('(largeHtmlString) : parses large HTML strings', () => {
            const html = '<div></div>'.repeat(10);
            const nodes = $.parseHTML(html);
            (0, vitest_1.expect)(nodes.length).toBe(10);
            (0, vitest_1.expect)(nodes).toBeInstanceOf(Array);
        });
        (0, vitest_1.it)('("<script>") : ignores scripts by default', () => {
            const html = '<script>undefined()</script>';
            (0, vitest_1.expect)($.parseHTML(html)).toHaveLength(0);
        });
        (0, vitest_1.it)('("<script>", true) : preserves scripts when requested', () => {
            const html = '<script>undefined()</script>';
            (0, vitest_1.expect)($.parseHTML(html, true)[0]).toHaveProperty('tagName', 'script');
        });
        (0, vitest_1.it)('("scriptAndNonScript) : preserves non-script nodes', () => {
            const html = '<script>undefined()</script><div></div>';
            (0, vitest_1.expect)($.parseHTML(html)[0]).toHaveProperty('tagName', 'div');
        });
        (0, vitest_1.it)('(scriptAndNonScript, true) : Preserves script position', () => {
            const html = '<script>undefined()</script><div></div>';
            (0, vitest_1.expect)($.parseHTML(html, true)[0]).toHaveProperty('tagName', 'script');
        });
        (0, vitest_1.it)('(text) : returns a text node', () => {
            (0, vitest_1.expect)($.parseHTML('text')[0].type).toBe('text');
        });
        (0, vitest_1.it)('(<tab>>text) : preserves leading whitespace', () => {
            (0, vitest_1.expect)($.parseHTML('\t<div></div>')[0]).toHaveProperty('data', '\t');
        });
        (0, vitest_1.it)('( text) : Leading spaces are treated as text nodes', () => {
            (0, vitest_1.expect)($.parseHTML(' <div/> ')[0].type).toBe('text');
        });
        (0, vitest_1.it)('(html) : should preserve content', () => {
            const html = '<div>test div</div>';
            (0, vitest_1.expect)((0, fixtures_js_1.cheerio)($.parseHTML(html)[0]).html()).toBe('test div');
        });
        (0, vitest_1.it)('(malformedHtml) : should not break', () => {
            (0, vitest_1.expect)($.parseHTML('<span><span>')).toHaveLength(1);
        });
        (0, vitest_1.it)('(garbageInput) : should not cause an error', () => {
            (0, vitest_1.expect)($.parseHTML('<#if><tr><p>This is a test.</p></tr><#/if>')).toBeTruthy();
        });
        (0, vitest_1.it)('(text) : should return an array that is not effected by DOM manipulation methods', () => {
            const $div = fixtures_js_1.cheerio.load('<div>');
            const elems = $div.parseHTML('<b></b><i></i>');
            $div('div').append(elems);
            (0, vitest_1.expect)(elems).toHaveLength(2);
        });
        (0, vitest_1.it)('(html, context) : should ignore context argument', () => {
            const $div = fixtures_js_1.cheerio.load('<div>');
            const elems = $div.parseHTML('<script>foo</script><a>', { foo: 123 });
            $div('div').append(elems);
            (0, vitest_1.expect)(elems).toHaveLength(1);
        });
        (0, vitest_1.it)('(html, context, keepScripts) : should ignore context argument', () => {
            const $div = fixtures_js_1.cheerio.load('<div>');
            const elems = $div.parseHTML('<script>foo</script><a>', { foo: 123 }, true);
            $div('div').append(elems);
            (0, vitest_1.expect)(elems).toHaveLength(2);
        });
    });
    (0, vitest_1.describe)('.merge', () => {
        const $ = fixtures_js_1.cheerio.load('');
        (0, vitest_1.it)('should be a function', () => {
            (0, vitest_1.expect)(typeof $.merge).toBe('function');
        });
        (0, vitest_1.it)('(arraylike, arraylike) : should modify the first array, but not the second', () => {
            const arr1 = [1, 2, 3];
            const arr2 = [4, 5, 6];
            const ret = $.merge(arr1, arr2);
            (0, vitest_1.expect)(typeof ret).toBe('object');
            (0, vitest_1.expect)(Array.isArray(ret)).toBe(true);
            (0, vitest_1.expect)(ret).toBe(arr1);
            (0, vitest_1.expect)(arr1).toHaveLength(6);
            (0, vitest_1.expect)(arr2).toHaveLength(3);
        });
        (0, vitest_1.it)('(arraylike, arraylike) : should handle objects that arent arrays, but are arraylike', () => {
            const arr1 = {
                length: 3,
                0: 'a',
                1: 'b',
                2: 'c',
            };
            const arr2 = {
                length: 3,
                0: 'd',
                1: 'e',
                2: 'f',
            };
            $.merge(arr1, arr2);
            (0, vitest_1.expect)(arr1).toHaveLength(6);
            (0, vitest_1.expect)(arr1[3]).toBe('d');
            (0, vitest_1.expect)(arr1[4]).toBe('e');
            (0, vitest_1.expect)(arr1[5]).toBe('f');
            (0, vitest_1.expect)(arr2).toHaveLength(3);
        });
        (0, vitest_1.it)('(?, ?) : should gracefully reject invalid inputs', () => {
            (0, vitest_1.expect)($.merge([4], 3)).toBeFalsy();
            (0, vitest_1.expect)($.merge({}, {})).toBeFalsy();
            (0, vitest_1.expect)($.merge([], {})).toBeFalsy();
            (0, vitest_1.expect)($.merge({}, [])).toBeFalsy();
            const fakeArray1 = { length: 3, 0: 'a', 1: 'b', 3: 'd' };
            (0, vitest_1.expect)($.merge(fakeArray1, [])).toBeFalsy();
            (0, vitest_1.expect)($.merge([], fakeArray1)).toBeFalsy();
            (0, vitest_1.expect)($.merge({ length: '7' }, [])).toBeFalsy();
            (0, vitest_1.expect)($.merge({ length: -1 }, [])).toBeFalsy();
        });
        (0, vitest_1.it)('(?, ?) : should no-op on invalid inputs', () => {
            const fakeArray1 = { length: 3, 0: 'a', 1: 'b', 3: 'd' };
            $.merge(fakeArray1, []);
            (0, vitest_1.expect)(fakeArray1).toHaveLength(3);
            (0, vitest_1.expect)(fakeArray1[0]).toBe('a');
            (0, vitest_1.expect)(fakeArray1[1]).toBe('b');
            (0, vitest_1.expect)(fakeArray1[3]).toBe('d');
            $.merge([], fakeArray1);
            (0, vitest_1.expect)(fakeArray1).toHaveLength(3);
            (0, vitest_1.expect)(fakeArray1[0]).toBe('a');
            (0, vitest_1.expect)(fakeArray1[1]).toBe('b');
            (0, vitest_1.expect)(fakeArray1[3]).toBe('d');
        });
    });
    (0, vitest_1.describe)('.contains', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = fixtures_js_1.cheerio.load(fixtures_js_1.food);
        });
        (0, vitest_1.it)('(container, contained) : should correctly detect the provided element', () => {
            const $food = $('#food');
            const $fruits = $('#fruits');
            const $apple = $('.apple');
            (0, vitest_1.expect)($.contains($food[0], $fruits[0])).toBe(true);
            (0, vitest_1.expect)($.contains($food[0], $apple[0])).toBe(true);
        });
        (0, vitest_1.it)('(container, other) : should not detect elements that are not contained', () => {
            const $fruits = $('#fruits');
            const $vegetables = $('#vegetables');
            const $apple = $('.apple');
            (0, vitest_1.expect)($.contains($vegetables[0], $apple[0])).toBe(false);
            (0, vitest_1.expect)($.contains($fruits[0], $vegetables[0])).toBe(false);
            (0, vitest_1.expect)($.contains($vegetables[0], $fruits[0])).toBe(false);
            (0, vitest_1.expect)($.contains($fruits[0], $fruits[0])).toBe(false);
            (0, vitest_1.expect)($.contains($vegetables[0], $vegetables[0])).toBe(false);
        });
    });
    (0, vitest_1.describe)('.root', () => {
        (0, vitest_1.it)('() : should return a cheerio-wrapped root object', () => {
            const $ = fixtures_js_1.cheerio.load('<html><head></head><body>foo</body></html>');
            $.root().append('<div id="test"></div>');
            (0, vitest_1.expect)($.html()).toBe('<html><head></head><body>foo</body></html><div id="test"></div>');
        });
    });
    (0, vitest_1.describe)('.extract', () => {
        (0, vitest_1.it)('() : should extract values for selectors', () => {
            const $ = fixtures_js_1.cheerio.load(fixtures_js_1.eleven);
            (0, vitest_1.expect)($.extract({
                red: [{ selector: '.red', value: 'outerHTML' }],
            })).toStrictEqual({
                red: [
                    '<li class="red">Four</li>',
                    '<li class="red">Five</li>',
                    '<li class="red sel">Nine</li>',
                ],
            });
        });
    });
});
