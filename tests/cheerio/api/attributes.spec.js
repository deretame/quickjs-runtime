"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const fixtures_js_1 = require("../__fixtures__/fixtures.js");
const index_js_1 = require("../index.js");
function withClass(attr) {
    return (0, fixtures_js_1.cheerio)(`<div class="${attr}"></div>`);
}
(0, vitest_1.describe)('$(...)', () => {
    (0, vitest_1.describe)('.attr', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('() : should get all the attributes', () => {
            const attrs = $('ul').attr();
            (0, vitest_1.expect)(attrs).toHaveProperty('id', 'fruits');
        });
        (0, vitest_1.it)('(invalid key) : invalid attr should get undefined', () => {
            const attr = $('.apple').attr('lol');
            (0, vitest_1.expect)(attr).toBeUndefined();
        });
        (0, vitest_1.it)('(valid key) : valid attr should get value', () => {
            const cls = $('.apple').attr('class');
            (0, vitest_1.expect)(cls).toBe('apple');
        });
        (0, vitest_1.it)('(valid key) : valid attr should get name when boolean', () => {
            const attr = $('<input name=email autofocus>').attr('autofocus');
            (0, vitest_1.expect)(attr).toBe('autofocus');
        });
        (0, vitest_1.it)('(key, value) : should set one attr', () => {
            const $pear = $('.pear').attr('id', 'pear');
            (0, vitest_1.expect)($('#pear')).toHaveLength(1);
            (0, vitest_1.expect)($pear).toBeInstanceOf($);
        });
        (0, vitest_1.it)('(key, value) : should set multiple attr', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div></div> <div></div>').attr('class', 'pear');
            (0, vitest_1.expect)($el[0].attribs).toHaveProperty('class', 'pear');
            (0, vitest_1.expect)($el[1].attribs).toBeUndefined();
            (0, vitest_1.expect)($el[2].attribs).toHaveProperty('class', 'pear');
        });
        (0, vitest_1.it)('(key, value) : should return an empty object for an empty object', () => {
            const $src = $().attr('key', 'value');
            (0, vitest_1.expect)($src.length).toBe(0);
            (0, vitest_1.expect)($src[0]).toBeUndefined();
        });
        (0, vitest_1.it)('(map) : object map should set multiple attributes', () => {
            $('.apple').attr({
                id: 'apple',
                style: 'color:red;',
                'data-url': 'http://apple.com',
            });
            const attrs = $('.apple').attr();
            (0, vitest_1.expect)(attrs).toHaveProperty('id', 'apple');
            (0, vitest_1.expect)(attrs).toHaveProperty('style', 'color:red;');
            (0, vitest_1.expect)(attrs).toHaveProperty('data-url', 'http://apple.com');
        });
        (0, vitest_1.it)('(map, val) : should throw with wrong combination of arguments', () => {
            (0, vitest_1.expect)(() => $('.apple').attr({
                id: 'apple',
                style: 'color:red;',
                'data-url': 'http://apple.com',
            }, () => '')).toThrow('Bad combination of arguments.');
        });
        (0, vitest_1.it)('(key, function) : should call the function and update the attribute with the return value', () => {
            const $fruits = $('#fruits');
            $fruits.attr('id', (index, value) => {
                (0, vitest_1.expect)(index).toBe(0);
                (0, vitest_1.expect)(value).toBe('fruits');
                return 'ninja';
            });
            const attrs = $fruits.attr();
            (0, vitest_1.expect)(attrs).toHaveProperty('id', 'ninja');
        });
        (0, vitest_1.it)('(key, function) : should ignore text nodes', () => {
            const $text = $(fixtures_js_1.mixedText);
            $text.attr('class', () => 'ninja');
            const className = $text.attr('class');
            (0, vitest_1.expect)(className).toBe('ninja');
        });
        (0, vitest_1.it)('(key, value) : should correctly encode then decode unsafe values', () => {
            const $apple = $('.apple');
            $apple.attr('href', 'http://github.com/"><script>alert("XSS!")</script><br');
            (0, vitest_1.expect)($apple.attr('href')).toBe('http://github.com/"><script>alert("XSS!")</script><br');
            $apple.attr('href', 'http://github.com/"><script>alert("XSS!")</script><br');
            (0, vitest_1.expect)($apple.html()).not.toContain('<script>alert("XSS!")</script>');
        });
        (0, vitest_1.it)('(key, value) : should coerce values to a string', () => {
            const $apple = $('.apple');
            $apple.attr('data-test', 1);
            (0, vitest_1.expect)($apple[0].attribs['data-test']).toBe('1');
            (0, vitest_1.expect)($apple.attr('data-test')).toBe('1');
        });
        (0, vitest_1.it)('(key, value) : handle removed boolean attributes', () => {
            const $apple = $('.apple');
            $apple.attr('autofocus', 'autofocus');
            (0, vitest_1.expect)($apple.attr('autofocus')).toBe('autofocus');
            $apple.removeAttr('autofocus');
            (0, vitest_1.expect)($apple.attr('autofocus')).toBeUndefined();
        });
        (0, vitest_1.it)('(key, value) : should remove non-boolean attributes with names or values similar to boolean ones', () => {
            const $apple = $('.apple');
            $apple.attr('data-autofocus', 'autofocus');
            (0, vitest_1.expect)($apple.attr('data-autofocus')).toBe('autofocus');
            $apple.removeAttr('data-autofocus');
            (0, vitest_1.expect)($apple.attr('data-autofocus')).toBeUndefined();
        });
        (0, vitest_1.it)('(key, value) : should remove attributes when called with null value', () => {
            const $pear = $('.pear').attr('autofocus', 'autofocus');
            (0, vitest_1.expect)($pear.attr('autofocus')).toBe('autofocus');
            $pear.attr('autofocus', null);
            (0, vitest_1.expect)($pear.attr('autofocus')).toBeUndefined();
        });
        (0, vitest_1.it)('(map) : should remove attributes with null values', () => {
            const $pear = $('.pear').attr({
                autofocus: 'autofocus',
                style: 'color:red',
            });
            (0, vitest_1.expect)($pear.attr('autofocus')).toBe('autofocus');
            (0, vitest_1.expect)($pear.attr('style')).toBe('color:red');
            $pear.attr({ autofocus: null, style: 'color:blue' });
            (0, vitest_1.expect)($pear.attr('autofocus')).toBeUndefined();
            (0, vitest_1.expect)($pear.attr('style')).toBe('color:blue');
        });
        (0, vitest_1.it)('(chaining) setting value and calling attr returns result', () => {
            const pearAttr = $('.pear').attr('foo', 'bar').attr('foo');
            (0, vitest_1.expect)(pearAttr).toBe('bar');
        });
        (0, vitest_1.it)('(chaining) setting attr to null returns a $', () => {
            const $pear = $('.pear').attr('foo', null);
            (0, vitest_1.expect)($pear).toBeInstanceOf($);
        });
        (0, vitest_1.it)('(chaining) setting attr to undefined returns a $', () => {
            const $pear = $('.pear').attr('foo', undefined);
            (0, vitest_1.expect)($('.pear')).toHaveLength(1);
            (0, vitest_1.expect)($('.pear').attr('foo')).toBeUndefined();
            (0, vitest_1.expect)($pear).toBeInstanceOf($);
        });
        (0, vitest_1.it)("(bool) shouldn't treat boolean attributes differently in XML mode", () => {
            const $xml = $.load('<input checked=checked disabled=yes />', {
                xml: true,
            })('input');
            (0, vitest_1.expect)($xml.attr('checked')).toBe('checked');
            (0, vitest_1.expect)($xml.attr('disabled')).toBe('yes');
        });
    });
    (0, vitest_1.describe)('.prop', () => {
        let $;
        let checkbox;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.inputs);
            checkbox = $('input[name=checkbox_on]');
        });
        (0, vitest_1.it)('(valid key) : valid prop should get value', () => {
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(true);
            checkbox.css('display', 'none');
            (0, vitest_1.expect)(checkbox.prop('style')).toHaveProperty('display', 'none');
            (0, vitest_1.expect)(checkbox.prop('style')).toHaveLength(1);
            (0, vitest_1.expect)(checkbox.prop('style')).toContain('display');
            (0, vitest_1.expect)(checkbox.prop('tagName')).toBe('INPUT');
            (0, vitest_1.expect)(checkbox.prop('nodeName')).toBe('INPUT');
        });
        (0, vitest_1.it)('(valid key) : should return on empty collection', () => {
            (0, vitest_1.expect)($(undefined).prop('checked')).toBeUndefined();
            (0, vitest_1.expect)($(undefined).prop('style')).toBeUndefined();
            (0, vitest_1.expect)($(undefined).prop('tagName')).toBeUndefined();
            (0, vitest_1.expect)($(undefined).prop('nodeName')).toBeUndefined();
        });
        (0, vitest_1.it)('(invalid key) : invalid prop should get undefined', () => {
            (0, vitest_1.expect)(checkbox.prop('lol')).toBeUndefined();
            (0, vitest_1.expect)(checkbox.prop(4)).toBeUndefined();
            (0, vitest_1.expect)(checkbox.prop(true)).toBeUndefined();
        });
        (0, vitest_1.it)('(key, value) : should set prop', () => {
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(true);
            checkbox.prop('checked', false);
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(false);
            checkbox.prop('checked', true);
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(true);
        });
        (0, vitest_1.it)('(key, value) : should update attribute', () => {
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(true);
            (0, vitest_1.expect)(checkbox.attr('checked')).toBe('checked');
            checkbox.prop('checked', false);
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(false);
            (0, vitest_1.expect)(checkbox.attr('checked')).toBeUndefined();
            checkbox.prop('checked', true);
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(true);
            (0, vitest_1.expect)(checkbox.attr('checked')).toBe('checked');
        });
        (0, vitest_1.it)('(key, value) : should update namespace', () => {
            const imgs = $('<img>\n\n<img>\n\n<img>');
            const nsHtml = 'http://www.w3.org/1999/xhtml';
            imgs.prop('src', '#').prop('namespace', nsHtml);
            (0, vitest_1.expect)(imgs.prop('namespace')).toBe(nsHtml);
            imgs.prop('attribs', null);
            (0, vitest_1.expect)(imgs.prop('src')).toBeUndefined();
            (0, vitest_1.expect)(imgs.prop('data-foo')).toBeUndefined();
        });
        (0, vitest_1.it)('(key, value) : should ignore empty collection', () => {
            (0, vitest_1.expect)($(undefined).prop('checked')).toBeUndefined();
            $(undefined).prop('checked', true);
            (0, vitest_1.expect)($(undefined).prop('checked')).toBeUndefined();
        });
        (0, vitest_1.it)('(map) : object map should set multiple props', () => {
            checkbox.prop({
                id: 'check',
                checked: false,
            });
            (0, vitest_1.expect)(checkbox.prop('id')).toBe('check');
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(false);
        });
        (0, vitest_1.it)('(map, val) : should throw with wrong combination of arguments', () => {
            (0, vitest_1.expect)(() => $('.apple').prop({
                id: 'check',
                checked: false,
            }, () => '')).toThrow('Bad combination of arguments.');
        });
        (0, vitest_1.it)('(key, function) : should call the function and update the prop with the return value', () => {
            checkbox.prop('checked', (index, value) => {
                (0, vitest_1.expect)(index).toBe(0);
                (0, vitest_1.expect)(value).toBe(true);
                return false;
            });
            (0, vitest_1.expect)(checkbox.prop('checked')).toBe(false);
        });
        (0, vitest_1.it)('(key, value) : should support chaining after setting props', () => {
            (0, vitest_1.expect)(checkbox.prop('checked', false)).toBe(checkbox);
        });
        (0, vitest_1.it)('(invalid element/tag) : prop should return undefined', () => {
            (0, vitest_1.expect)($(undefined).prop('prop')).toBeUndefined();
            (0, vitest_1.expect)($(null).prop('prop')).toBeUndefined();
        });
        (0, vitest_1.it)('("href") : should resolve links with `baseURI`', () => {
            const $ = (0, index_js_1.load)(`
          <a id="1" href="http://example.org">example1</a>
          <a id="2" href="//example.org">example2</a>
          <a id="3" href="/example.org">example3</a>
          <a id="4" href="example.org">example4</a>
        `, { baseURI: 'http://example.com/page/1' });
            (0, vitest_1.expect)($('#1').prop('href')).toBe('http://example.org/');
            (0, vitest_1.expect)($('#2').prop('href')).toBe('http://example.org/');
            (0, vitest_1.expect)($('#3').prop('href')).toBe('http://example.com/example.org');
            (0, vitest_1.expect)($('#4').prop('href')).toBe('http://example.com/page/example.org');
            (0, vitest_1.expect)($(undefined).prop('href')).toBeUndefined();
        });
        (0, vitest_1.it)('("href") : should skip values without an href', () => {
            const $ = (0, index_js_1.load)('<a id="1">example1</a>');
            (0, vitest_1.expect)($('#1').prop('href')).toBeUndefined();
        });
        (0, vitest_1.it)('("src") : should resolve links with `baseURI`', () => {
            const $ = (0, index_js_1.load)(`
          <img id="1" src="http://example.org/image.png">
          <iframe id="2" src="//example.org/page.html"></iframe>
          <audio id="3" src="/example.org/song.mp3"></audio>
          <source id="4" src="example.org/image.png">
        `, { baseURI: 'http://example.com/page/1' });
            (0, vitest_1.expect)($('#1').prop('src')).toBe('http://example.org/image.png');
            (0, vitest_1.expect)($('#2').prop('src')).toBe('http://example.org/page.html');
            (0, vitest_1.expect)($('#3').prop('src')).toBe('http://example.com/example.org/song.mp3');
            (0, vitest_1.expect)($('#4').prop('src')).toBe('http://example.com/page/example.org/image.png');
            (0, vitest_1.expect)($(undefined).prop('src')).toBeUndefined();
        });
        (0, vitest_1.it)('("outerHTML") : should render properly', () => {
            const outerHtml = '<div><a></a></div>';
            const $a = $(outerHtml);
            (0, vitest_1.expect)($a.prop('outerHTML')).toBe(outerHtml);
            (0, vitest_1.expect)($(undefined).prop('outerHTML')).toBeUndefined();
        });
        (0, vitest_1.it)('("outerHTML") : should support root nodes', () => {
            const $ = (0, index_js_1.load)('<div></div>');
            (0, vitest_1.expect)($.root().prop('outerHTML')).toBe('<html><head></head><body><div></div></body></html>');
        });
        (0, vitest_1.it)('("innerHTML") : should render properly', () => {
            const $a = $('<div><a></a></div>');
            (0, vitest_1.expect)($a.prop('innerHTML')).toBe('<a></a>');
            (0, vitest_1.expect)($(undefined).prop('innerHTML')).toBeUndefined();
        });
        (0, vitest_1.it)('("textContent") : should render properly', () => {
            (0, vitest_1.expect)($('select').children().prop('textContent')).toBe('Option not selected');
            (0, vitest_1.expect)($(fixtures_js_1.script).prop('textContent')).toBe('A  var foo = "bar";B');
            (0, vitest_1.expect)($(undefined).prop('textContent')).toBeUndefined();
        });
        (0, vitest_1.it)('("textContent") : should include style and script tags', () => {
            const $ = (0, index_js_1.load)('<body>Welcome <div>Hello, testing text function,<script>console.log("hello")</script></div><style type="text/css">.cf-hidden { display: none; }</style>End of message</body>');
            (0, vitest_1.expect)($('body').prop('textContent')).toBe('Welcome Hello, testing text function,console.log("hello").cf-hidden { display: none; }End of message');
            (0, vitest_1.expect)($('style').prop('textContent')).toBe('.cf-hidden { display: none; }');
            (0, vitest_1.expect)($('script').prop('textContent')).toBe('console.log("hello")');
        });
        (0, vitest_1.it)('("innerText") : should render properly', () => {
            (0, vitest_1.expect)($('select').children().prop('innerText')).toBe('Option not selected');
            (0, vitest_1.expect)($(fixtures_js_1.script).prop('innerText')).toBe('AB');
            (0, vitest_1.expect)($(undefined).prop('innerText')).toBeUndefined();
        });
        (0, vitest_1.it)('("innerText") : should omit style and script tags', () => {
            const $ = (0, index_js_1.load)('<body>Welcome <div>Hello, testing text function,<script>console.log("hello")</script></div><style type="text/css">.cf-hidden { display: none; }</style>End of message</body>');
            (0, vitest_1.expect)($('body').prop('innerText')).toBe('Welcome Hello, testing text function,End of message');
            (0, vitest_1.expect)($('style').prop('innerText')).toBe('');
            (0, vitest_1.expect)($('script').prop('innerText')).toBe('');
        });
        (0, vitest_1.it)('(inherited properties) : prop should support inherited properties', () => {
            (0, vitest_1.expect)($('select').prop('childNodes')).toBe($('select')[0].childNodes);
        });
        (0, vitest_1.it)('(key) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            (0, vitest_1.expect)($text($body[1]).prop('tagName')).toBeUndefined();
            $body.prop('test-name', () => 'tester');
            (0, vitest_1.expect)($text('body').html()).toBe('<a test-name="tester">1</a>TEXT<b test-name="tester">2</b>');
        });
        (0, vitest_1.it)("(bool) shouldn't treat boolean attributes differently in XML mode", () => {
            const $xml = $.load('<input checked=checked disabled=yes />', {
                xml: true,
            })('input');
            (0, vitest_1.expect)($xml.prop('checked')).toBe('checked');
            (0, vitest_1.expect)($xml.prop('disabled')).toBe('yes');
        });
    });
    (0, vitest_1.describe)('.data', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.chocolates);
        });
        (0, vitest_1.it)('() : should get all data attributes initially declared in the markup', () => {
            const data = $('.linth').data();
            (0, vitest_1.expect)(data).toStrictEqual({
                highlight: 'Lindor',
                origin: 'swiss',
            });
        });
        (0, vitest_1.it)('() : should get all data set via `data`', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div>');
            $el.data('a', 1);
            $el.data('b', 2);
            (0, vitest_1.expect)($el.data()).toStrictEqual({
                a: 1,
                b: 2,
            });
        });
        (0, vitest_1.it)('() : should get all data attributes initially declared in the markup merged with all data additionally set via `data`', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data-a="a" data-b="b">');
            $el.data('b', 'b-modified');
            $el.data('c', 'c');
            (0, vitest_1.expect)($el.data()).toStrictEqual({
                a: 'a',
                b: 'b-modified',
                c: 'c',
            });
        });
        (0, vitest_1.it)('() : no data attribute should return an empty object', () => {
            const data = $('.cailler').data();
            (0, vitest_1.expect)(Object.keys(data)).toHaveLength(0);
            (0, vitest_1.expect)($('.free').data()).toBeUndefined();
        });
        (0, vitest_1.it)('(invalid key) : invalid data attribute should return `undefined`', () => {
            const data = $('.frey').data('lol');
            (0, vitest_1.expect)(data).toBeUndefined();
        });
        (0, vitest_1.it)('(valid key) : valid data attribute should get value', () => {
            const highlight = $('.linth').data('highlight');
            const origin = $('.linth').data('origin');
            (0, vitest_1.expect)(highlight).toBe('Lindor');
            (0, vitest_1.expect)(origin).toBe('swiss');
        });
        (0, vitest_1.it)('(key) : should translate camel-cased key values to hyphen-separated versions', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data--three-word-attribute="a" data-foo-Bar_BAZ-="b">');
            (0, vitest_1.expect)($el.data('ThreeWordAttribute')).toBe('a');
            (0, vitest_1.expect)($el.data('fooBar_baz-')).toBe('b');
        });
        (0, vitest_1.it)('(key) : should retrieve object values', () => {
            const data = {};
            const $el = (0, fixtures_js_1.cheerio)('<div>');
            $el.data('test', data);
            (0, vitest_1.expect)($el.data('test')).toBe(data);
        });
        (0, vitest_1.it)('(key) : should parse JSON data derived from the markup', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data-json="[1, 2, 3]">');
            (0, vitest_1.expect)($el.data('json')).toStrictEqual([1, 2, 3]);
        });
        (0, vitest_1.it)('(key) : should not parse JSON data set via the `data` API', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div>');
            $el.data('json', '[1, 2, 3]');
            (0, vitest_1.expect)($el.data('json')).toBe('[1, 2, 3]');
        });
        // See https://api.jquery.com/data/ and https://bugs.jquery.com/ticket/14523
        (0, vitest_1.it)('(key) : should ignore the markup value after the first access', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data-test="a">');
            (0, vitest_1.expect)($el.data('test')).toBe('a');
            $el.attr('data-test', 'b');
            (0, vitest_1.expect)($el.data('test')).toBe('a');
        });
        (0, vitest_1.it)('(key) : should recover from malformed JSON', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data-custom="{{templatevar}}">');
            (0, vitest_1.expect)($el.data('custom')).toBe('{{templatevar}}');
        });
        (0, vitest_1.it)('("") : should accept the empty string as a name', () => {
            const $el = (0, fixtures_js_1.cheerio)('<div data-="a">');
            (0, vitest_1.expect)($el.data('')).toBe('a');
        });
        (0, vitest_1.it)('(hyphen key) : data addribute with hyphen should be camelized ;-)', () => {
            const data = $('.frey').data();
            (0, vitest_1.expect)(data).toStrictEqual({
                taste: 'sweet',
                bestCollection: 'Mahony',
            });
        });
        (0, vitest_1.it)('(key, value) : should set data attribute', () => {
            // Adding as object.
            const a = $('.frey').data({
                balls: 'giandor',
            });
            // Adding as string.
            const b = $('.linth').data('snack', 'chocoletti');
            (0, vitest_1.expect)(() => {
                a.data(4, 'throw');
            }).not.toThrow();
            (0, vitest_1.expect)(a.data('balls')).toStrictEqual('giandor');
            (0, vitest_1.expect)(b.data('snack')).toStrictEqual('chocoletti');
        });
        (0, vitest_1.it)('(key, value) : should set data for all elements in the selection', () => {
            $('li').data('foo', 'bar');
            (0, vitest_1.expect)($('li').eq(0).data('foo')).toStrictEqual('bar');
            (0, vitest_1.expect)($('li').eq(1).data('foo')).toStrictEqual('bar');
            (0, vitest_1.expect)($('li').eq(2).data('foo')).toStrictEqual('bar');
        });
        (0, vitest_1.it)('(map) : object map should set multiple data attributes', () => {
            const { data } = $('.linth').data({
                id: 'Cailler',
                flop: 'Pippilotti Rist',
                top: 'Frigor',
                url: 'http://www.cailler.ch/',
            })[0];
            (0, vitest_1.expect)(data).toHaveProperty('id', 'Cailler');
            (0, vitest_1.expect)(data).toHaveProperty('flop', 'Pippilotti Rist');
            (0, vitest_1.expect)(data).toHaveProperty('top', 'Frigor');
            (0, vitest_1.expect)(data).toHaveProperty('url', 'http://www.cailler.ch/');
        });
        (0, vitest_1.describe)('(attr) : data-* attribute type coercion :', () => {
            (0, vitest_1.it)('boolean', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-bool="true">');
                (0, vitest_1.expect)($el.data('bool')).toBe(true);
            });
            (0, vitest_1.it)('number', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-number="23">');
                (0, vitest_1.expect)($el.data('number')).toBe(23);
            });
            (0, vitest_1.it)('number (scientific notation is not coerced)', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-sci="1E10">');
                (0, vitest_1.expect)($el.data('sci')).toBe('1E10');
            });
            (0, vitest_1.it)('null', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-null="null">');
                (0, vitest_1.expect)($el.data('null')).toBe(null);
            });
            (0, vitest_1.it)('object', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-obj=\'{ "a": 45 }\'>');
                (0, vitest_1.expect)($el.data('obj')).toStrictEqual({ a: 45 });
            });
            (0, vitest_1.it)('array', () => {
                const $el = (0, fixtures_js_1.cheerio)('<div data-array="[1, 2, 3]">');
                (0, vitest_1.expect)($el.data('array')).toStrictEqual([1, 2, 3]);
            });
        });
        (0, vitest_1.it)('(key, value) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.data('snack', 'chocoletti');
            (0, vitest_1.expect)($text('b').data('snack')).toBe('chocoletti');
        });
    });
    (0, vitest_1.describe)('.val', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.inputs);
        });
        (0, vitest_1.it)('(): on div should get undefined', () => {
            (0, vitest_1.expect)($('<div>').val()).toBeUndefined();
        });
        (0, vitest_1.it)('(): on button should get value', () => {
            const val = $('#btn-value').val();
            (0, vitest_1.expect)(val).toBe('button');
        });
        (0, vitest_1.it)('(): on button with no value should get undefined', () => {
            const val = $('#btn-valueless').val();
            (0, vitest_1.expect)(val).toBeUndefined();
        });
        (0, vitest_1.it)('(): on select should get value', () => {
            const val = $('select#one').val();
            (0, vitest_1.expect)(val).toBe('option_selected');
        });
        (0, vitest_1.it)('(): on select with no value should get text', () => {
            const val = $('select#one-valueless').val();
            (0, vitest_1.expect)(val).toBe('Option selected');
        });
        (0, vitest_1.it)('(): on select with no value should get converted HTML', () => {
            const val = $('select#one-html-entity').val();
            (0, vitest_1.expect)(val).toBe('Option <selected>');
        });
        (0, vitest_1.it)('(): on select with no value should get text content', () => {
            const val = $('select#one-nested').val();
            (0, vitest_1.expect)(val).toBe('Option selected');
        });
        (0, vitest_1.it)('(): on option should get value', () => {
            const val = $('select#one option').eq(0).val();
            (0, vitest_1.expect)(val).toBe('option_not_selected');
        });
        (0, vitest_1.it)('(): on text input should get value', () => {
            const val = $('input[type="text"]').val();
            (0, vitest_1.expect)(val).toBe('input_text');
        });
        (0, vitest_1.it)('(): on checked checkbox should get value', () => {
            const val = $('input[name="checkbox_on"]').val();
            (0, vitest_1.expect)(val).toBe('on');
        });
        (0, vitest_1.it)('(): on unchecked checkbox should get value', () => {
            const val = $('input[name="checkbox_off"]').val();
            (0, vitest_1.expect)(val).toBe('off');
        });
        (0, vitest_1.it)('(): on valueless checkbox should get value', () => {
            const val = $('input[name="checkbox_valueless"]').val();
            (0, vitest_1.expect)(val).toBe('on');
        });
        (0, vitest_1.it)('(): on radio should get value', () => {
            const val = $('input[type="radio"]').val();
            (0, vitest_1.expect)(val).toBe('off');
        });
        (0, vitest_1.it)('(): on valueless radio should get value', () => {
            const val = $('input[name="radio_valueless"]').val();
            (0, vitest_1.expect)(val).toBe('on');
        });
        (0, vitest_1.it)('(): on multiple select should get an array of values', () => {
            const val = $('select#multi').val();
            (0, vitest_1.expect)(val).toStrictEqual(['2', '3']);
        });
        (0, vitest_1.it)('(): on multiple select with no value attribute should get an array of text content', () => {
            const val = $('select#multi-valueless').val();
            (0, vitest_1.expect)(val).toStrictEqual(['2', '3']);
        });
        (0, vitest_1.it)('(): with no selector matches should return nothing', () => {
            const val = $('.nasty').val();
            (0, vitest_1.expect)(val).toBeUndefined();
        });
        (0, vitest_1.it)('(invalid value): should only handle arrays when it has the attribute multiple', () => {
            const val = $('select#one').val([]);
            (0, vitest_1.expect)(val).not.toBeUndefined();
        });
        (0, vitest_1.it)('(value): on empty set should get `this`', () => {
            const $empty = $([]);
            (0, vitest_1.expect)($empty.val('test')).toBe($empty);
        });
        (0, vitest_1.it)('(value): on input text should set value', () => {
            const element = $('input[type="text"]').val('test');
            (0, vitest_1.expect)(element.val()).toBe('test');
        });
        (0, vitest_1.it)('(value): on select should set value', () => {
            const element = $('select#one').val('option_not_selected');
            (0, vitest_1.expect)(element.val()).toBe('option_not_selected');
        });
        (0, vitest_1.it)('(value): on option should set value', () => {
            const element = $('select#one option').eq(0).val('option_changed');
            (0, vitest_1.expect)(element.val()).toBe('option_changed');
        });
        (0, vitest_1.it)('(value): on radio should set value', () => {
            const element = $('input[name="radio"]').val('off');
            (0, vitest_1.expect)(element.val()).toBe('off');
        });
        (0, vitest_1.it)('(value): on radio with special characters should set value', () => {
            const element = $('input[name="radio[brackets]"]').val('off');
            (0, vitest_1.expect)(element.val()).toBe('off');
        });
        (0, vitest_1.it)('(values): on multiple select should set multiple values', () => {
            const element = $('select#multi').val(['1', '3', '4']);
            (0, vitest_1.expect)(element.val()).toHaveLength(3);
        });
    });
    (0, vitest_1.describe)('.removeAttr', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('(key) : should remove a single attr', () => {
            const $fruits = $('#fruits');
            (0, vitest_1.expect)($fruits.attr('id')).not.toBeUndefined();
            $fruits.removeAttr('id');
            (0, vitest_1.expect)($fruits.attr('id')).toBeUndefined();
        });
        (0, vitest_1.it)('(key key) : should remove multiple attrs', () => {
            const $apple = $('.apple');
            $apple.attr('id', 'favorite');
            $apple.attr('size', 'small');
            (0, vitest_1.expect)($apple.attr('id')).toBe('favorite');
            (0, vitest_1.expect)($apple.attr('class')).toBe('apple');
            (0, vitest_1.expect)($apple.attr('size')).toBe('small');
            $apple.removeAttr('id class');
            (0, vitest_1.expect)($apple.attr('id')).toBeUndefined();
            (0, vitest_1.expect)($apple.attr('class')).toBeUndefined();
            (0, vitest_1.expect)($apple.attr('size')).toBe('small');
        });
        (0, vitest_1.it)('(key) : should return cheerio object', () => {
            const obj = $('ul').removeAttr('id');
            (0, vitest_1.expect)(obj).toBeInstanceOf($);
        });
        (0, vitest_1.it)('(key) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.addClass(() => 'test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a class="test">1</a>TEXT<b class="test">2</b>');
            $body.removeAttr('class');
            (0, vitest_1.expect)($text('body').html()).toBe(fixtures_js_1.mixedText);
        });
    });
    (0, vitest_1.describe)('.hasClass', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('(valid class) : should return true', () => {
            const cls = $('.apple').hasClass('apple');
            (0, vitest_1.expect)(cls).toBe(true);
            (0, vitest_1.expect)(withClass('foo').hasClass('foo')).toBe(true);
            (0, vitest_1.expect)(withClass('foo bar').hasClass('foo')).toBe(true);
            (0, vitest_1.expect)(withClass('bar foo').hasClass('foo')).toBe(true);
            (0, vitest_1.expect)(withClass('bar foo bar').hasClass('foo')).toBe(true);
        });
        (0, vitest_1.it)('(invalid class) : should return false', () => {
            const cls = $('#fruits').hasClass('fruits');
            (0, vitest_1.expect)(cls).toBe(false);
            (0, vitest_1.expect)(withClass('foo-bar').hasClass('foo')).toBe(false);
            (0, vitest_1.expect)(withClass('foo-bar').hasClass('foo')).toBe(false);
            (0, vitest_1.expect)(withClass('foo-bar').hasClass('foo-ba')).toBe(false);
        });
        (0, vitest_1.it)('should check multiple classes', () => {
            // Add a class
            $('.apple').addClass('red');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(true);
            // Remove one and test again
            $('.apple').removeClass('apple');
            (0, vitest_1.expect)($('li').eq(0).hasClass('apple')).toBe(false);
        });
        (0, vitest_1.it)('(empty string argument) : should return false', () => {
            (0, vitest_1.expect)(withClass('foo').hasClass('')).toBe(false);
            (0, vitest_1.expect)(withClass('foo bar').hasClass('')).toBe(false);
            (0, vitest_1.expect)(withClass('foo bar').removeClass('foo').hasClass('')).toBe(false);
        });
    });
    (0, vitest_1.describe)('.addClass', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('(first class) : should add the class to the element', () => {
            const $fruits = $('#fruits');
            $fruits.addClass('fruits');
            const cls = $fruits.hasClass('fruits');
            (0, vitest_1.expect)(cls).toBe(true);
        });
        (0, vitest_1.it)('(single class) : should add the class to the element', () => {
            $('.apple').addClass('fruit');
            const cls = $('.apple').hasClass('fruit');
            (0, vitest_1.expect)(cls).toBe(true);
        });
        (0, vitest_1.it)('(class): adds classes to many selected items', () => {
            $('li').addClass('fruit');
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.orange').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.pear').hasClass('fruit')).toBe(true);
            // Mixed with text nodes
            const $red = $('<html>\n<ul id=one>\n</ul>\t</html>').addClass('red');
            (0, vitest_1.expect)($red).toHaveLength(3);
            (0, vitest_1.expect)($red[0].type).toBe('text');
            (0, vitest_1.expect)($red[1].type).toBe('tag');
            (0, vitest_1.expect)($red[2].type).toBe('text');
            (0, vitest_1.expect)($red.hasClass('red')).toBe(true);
        });
        (0, vitest_1.it)('(class class class) : should add multiple classes to the element', () => {
            $('.apple').addClass('fruit red tasty');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('tasty')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should add classes returned from the function', () => {
            const $fruits = $('#fruits').children().add($('#fruits'));
            const args = [];
            const thisVals = [];
            const toAdd = ['main', 'apple red', '', undefined];
            $fruits.addClass(function (...myArgs) {
                args.push(myArgs);
                thisVals.push(this);
                return toAdd[myArgs[0]];
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, ''],
                [1, 'apple'],
                [2, 'orange'],
                [3, 'pear'],
            ]);
            (0, vitest_1.expect)(thisVals).toStrictEqual([
                $fruits[0],
                $fruits[1],
                $fruits[2],
                $fruits[3],
            ]);
            (0, vitest_1.expect)($fruits.eq(0).hasClass('main')).toBe(true);
            (0, vitest_1.expect)($fruits.eq(0).hasClass('apple')).toBe(false);
            (0, vitest_1.expect)($fruits.eq(1).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.eq(1).hasClass('red')).toBe(true);
            (0, vitest_1.expect)($fruits.eq(2).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($fruits.eq(3).hasClass('pear')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.removeClass', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('() : should remove all the classes', () => {
            $('.pear').addClass('fruit');
            $('.pear').removeClass();
            (0, vitest_1.expect)($('.pear').attr('class')).toBeUndefined();
        });
        (0, vitest_1.it)('("") : should not modify class list', () => {
            const $fruits = $('#fruits');
            $fruits.children().removeClass('');
            (0, vitest_1.expect)($('.apple')).toHaveLength(1);
        });
        (0, vitest_1.it)('(invalid class) : should not remove anything', () => {
            $('.pear').removeClass('fruit');
            (0, vitest_1.expect)($('.pear').hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(no class attribute) : should not throw an exception', () => {
            const $vegetables = (0, fixtures_js_1.cheerio)(fixtures_js_1.vegetables);
            (0, vitest_1.expect)(() => {
                $('li', $vegetables).removeClass('vegetable');
            }).not.toThrow();
        });
        (0, vitest_1.it)('(single class) : should remove a single class from the element', () => {
            $('.pear').addClass('fruit');
            (0, vitest_1.expect)($('.pear').hasClass('fruit')).toBe(true);
            $('.pear').removeClass('fruit');
            (0, vitest_1.expect)($('.pear').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.pear').hasClass('pear')).toBe(true);
            // Remove one class from set
            const $li = $('li').removeClass('orange');
            (0, vitest_1.expect)($li.eq(0).attr('class')).toBe('apple');
            (0, vitest_1.expect)($li.eq(1).attr('class')).toBe('');
            (0, vitest_1.expect)($li.eq(2).attr('class')).toBe('pear');
            // Mixed with text nodes
            const $red = $('<html>\n<ul class=one>\n</ul>\t</html>').removeClass('one');
            (0, vitest_1.expect)($red).toHaveLength(3);
            (0, vitest_1.expect)($red[0].type).toBe('text');
            (0, vitest_1.expect)($red[1].type).toBe('tag');
            (0, vitest_1.expect)($red[2].type).toBe('text');
            (0, vitest_1.expect)($red.eq(1).attr('class')).toBe('');
            (0, vitest_1.expect)($red.eq(1).prop('tagName')).toBe('UL');
        });
        (0, vitest_1.it)('(single class) : should remove a single class from multiple classes on the element', () => {
            $('.pear').addClass('fruit green tasty');
            (0, vitest_1.expect)($('.pear').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.pear').hasClass('green')).toBe(true);
            (0, vitest_1.expect)($('.pear').hasClass('tasty')).toBe(true);
            $('.pear').removeClass('green');
            (0, vitest_1.expect)($('.pear').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.pear').hasClass('green')).toBe(false);
            (0, vitest_1.expect)($('.pear').hasClass('tasty')).toBe(true);
        });
        (0, vitest_1.it)('(class class class) : should remove multiple classes from the element', () => {
            $('.apple').addClass('fruit red tasty');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('tasty')).toBe(true);
            $('.apple').removeClass('apple red tasty');
            (0, vitest_1.expect)($('.fruit').hasClass('apple')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('red')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('tasty')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('fruit')).toBe(true);
        });
        (0, vitest_1.it)('(class) : should remove all occurrences of a class name', () => {
            const $div = (0, fixtures_js_1.cheerio)('<div class="x x y x z"></div>');
            (0, vitest_1.expect)($div.removeClass('x').hasClass('x')).toBe(false);
        });
        (0, vitest_1.it)('(fn) : should remove classes returned from the function', () => {
            const $fruits = $('#fruits').children();
            const args = [];
            const thisVals = [];
            const toAdd = ['apple red', '', undefined];
            $fruits.removeClass(function (...myArgs) {
                args.push(myArgs);
                thisVals.push(this);
                return toAdd[myArgs[0]];
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, 'apple'],
                [1, 'orange'],
                [2, 'pear'],
            ]);
            (0, vitest_1.expect)(thisVals).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
            (0, vitest_1.expect)($fruits.eq(0).hasClass('apple')).toBe(false);
            (0, vitest_1.expect)($fruits.eq(0).hasClass('red')).toBe(false);
            (0, vitest_1.expect)($fruits.eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($fruits.eq(2).hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should no op elements without attributes', () => {
            const $inputs = $(fixtures_js_1.inputs);
            const val = $inputs.removeClass(() => 'tasty');
            (0, vitest_1.expect)(val).toHaveLength(17);
        });
        (0, vitest_1.it)('(fn) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.addClass(() => 'test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a class="test">1</a>TEXT<b class="test">2</b>');
            $body.removeClass(() => 'test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a class="">1</a>TEXT<b class="">2</b>');
        });
    });
    (0, vitest_1.describe)('.toggleClass', () => {
        let $;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('(class class) : should toggle multiple classes from the element', () => {
            $('.apple').addClass('fruit');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(false);
            $('.apple').toggleClass('apple red');
            (0, vitest_1.expect)($('.fruit').hasClass('apple')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('red')).toBe(true);
            (0, vitest_1.expect)($('.fruit').hasClass('fruit')).toBe(true);
            // Mixed with text nodes
            const $red = $('<html>\n<ul class=one>\n</ul>\t</html>').toggleClass('red');
            (0, vitest_1.expect)($red).toHaveLength(3);
            (0, vitest_1.expect)($red.hasClass('red')).toBe(true);
            (0, vitest_1.expect)($red.hasClass('one')).toBe(true);
            $red.toggleClass('one');
            (0, vitest_1.expect)($red.hasClass('red')).toBe(true);
            (0, vitest_1.expect)($red.hasClass('one')).toBe(false);
        });
        (0, vitest_1.it)('(class class, true) : should add multiple classes to the element', () => {
            $('.apple').addClass('fruit');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(false);
            $('.apple').toggleClass('apple red', true);
            (0, vitest_1.expect)($('.fruit').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.fruit').hasClass('red')).toBe(true);
            (0, vitest_1.expect)($('.fruit').hasClass('fruit')).toBe(true);
        });
        (0, vitest_1.it)('(class true) : should add only one instance of class', () => {
            $('.apple').toggleClass('tasty', true);
            $('.apple').toggleClass('tasty', true);
            (0, vitest_1.expect)($('.apple').attr('class')).toMatch(/tasty/g);
        });
        (0, vitest_1.it)('(class class, false) : should remove multiple classes from the element', () => {
            $('.apple').addClass('fruit');
            (0, vitest_1.expect)($('.apple').hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('red')).toBe(false);
            $('.apple').toggleClass('apple red', false);
            (0, vitest_1.expect)($('.fruit').hasClass('apple')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('red')).toBe(false);
            (0, vitest_1.expect)($('.fruit').hasClass('fruit')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should toggle classes returned from the function', () => {
            const $ = (0, index_js_1.load)(fixtures_js_1.food);
            $('.apple').addClass('fruit');
            $('.carrot').addClass('vegetable');
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.apple').hasClass('vegetable')).toBe(false);
            (0, vitest_1.expect)($('.orange').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.orange').hasClass('vegetable')).toBe(false);
            (0, vitest_1.expect)($('.carrot').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.carrot').hasClass('vegetable')).toBe(true);
            (0, vitest_1.expect)($('.sweetcorn').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.sweetcorn').hasClass('vegetable')).toBe(false);
            $('li').toggleClass(function () {
                return $(this).parent().is('#fruits') ? 'fruit' : 'vegetable';
            });
            (0, vitest_1.expect)($('.apple').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.apple').hasClass('vegetable')).toBe(false);
            (0, vitest_1.expect)($('.orange').hasClass('fruit')).toBe(true);
            (0, vitest_1.expect)($('.orange').hasClass('vegetable')).toBe(false);
            (0, vitest_1.expect)($('.carrot').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.carrot').hasClass('vegetable')).toBe(false);
            (0, vitest_1.expect)($('.sweetcorn').hasClass('fruit')).toBe(false);
            (0, vitest_1.expect)($('.sweetcorn').hasClass('vegetable')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should work with no initial class attribute', () => {
            const $inputs = (0, index_js_1.load)(fixtures_js_1.inputs);
            $inputs('input, select').toggleClass(function () {
                return $inputs(this).get(0).tagName === 'select'
                    ? 'selectable'
                    : 'inputable';
            });
            (0, vitest_1.expect)($inputs('.selectable')).toHaveLength(6);
            (0, vitest_1.expect)($inputs('.inputable')).toHaveLength(9);
        });
        (0, vitest_1.it)('(fn) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.toggleClass(() => 'test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a class="test">1</a>TEXT<b class="test">2</b>');
            $body.toggleClass(() => 'test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a class="">1</a>TEXT<b class="">2</b>');
        });
        (0, vitest_1.it)('(invalid) : should be a no-op for invalid inputs', () => {
            const original = $('.apple');
            const testAgainst = original.attr('class');
            (0, vitest_1.expect)(original.toggleClass().attr('class')).toStrictEqual(testAgainst);
            for (const value of [undefined, true, false, null, 0, 1, {}]) {
                (0, vitest_1.expect)(original.toggleClass(value).attr('class')).toStrictEqual(testAgainst);
            }
        });
    });
});
