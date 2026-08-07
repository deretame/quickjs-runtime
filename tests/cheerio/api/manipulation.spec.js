"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vitest_1 = require("vitest");
const fixtures_js_1 = require("../__fixtures__/fixtures.js");
const index_js_1 = require("../index.js");
(0, vitest_1.describe)('$(...)', () => {
    let $;
    let $fruits;
    (0, vitest_1.beforeEach)(() => {
        $ = (0, index_js_1.load)(fixtures_js_1.fruits);
        $fruits = $('#fruits');
    });
    (0, vitest_1.describe)('.wrap', () => {
        (0, vitest_1.it)('(Cheerio object) : should insert the element and add selected element(s) as its child', () => {
            const $redFruits = $('<div class="red-fruits"></div>');
            $('.apple').wrap($redFruits);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('red-fruits')).toBe(true);
            (0, vitest_1.expect)($('.red-fruits').children().eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($redFruits.children()).toHaveLength(1);
        });
        (0, vitest_1.it)('(element) : should wrap the base element correctly', () => {
            $('ul').wrap('<a></a>');
            (0, vitest_1.expect)($('body').children()[0].tagName).toBe('a');
        });
        (0, vitest_1.it)('(document) : should ignore document node', () => {
            $.root().wrap('<a></a>');
            (0, vitest_1.expect)($.root()[0]).toHaveProperty('type', 'root');
        });
        (0, vitest_1.it)('(element) : should insert the element and add selected element(s) as its child', () => {
            const $redFruits = $('<div class="red-fruits"></div>');
            $('.apple').wrap($redFruits[0]);
            (0, vitest_1.expect)($fruits.children()[0]).toBe($redFruits[0]);
            (0, vitest_1.expect)($redFruits.children()).toHaveLength(1);
            (0, vitest_1.expect)($redFruits.children()[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($fruits.children()[1]).toBe($('.orange')[0]);
        });
        (0, vitest_1.it)('(html) : should insert the markup and add selected element(s) as its child', () => {
            $('.apple').wrap('<div class="red-fruits"> </div>');
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('red-fruits')).toBe(true);
            (0, vitest_1.expect)($('.red-fruits').children().eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($('.red-fruits').children()).toHaveLength(1);
        });
        (0, vitest_1.it)('(html) : discards extraneous markup', () => {
            $('.apple').wrap('<div></div><p></p>');
            (0, vitest_1.expect)($('div')).toHaveLength(1);
            (0, vitest_1.expect)($('p')).toHaveLength(0);
        });
        (0, vitest_1.it)('(html) : wraps with nested elements', () => {
            const $orangeFruits = $('<div class="orange-fruits"><div class="and-stuff"></div></div>');
            $('.orange').wrap($orangeFruits);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('orange-fruits')).toBe(true);
            (0, vitest_1.expect)($('.orange-fruits').children().eq(0).hasClass('and-stuff')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.orange-fruits').children()).toHaveLength(1);
        });
        (0, vitest_1.it)('(html) : should only worry about the first tag children', () => {
            const delicious = '<span> This guy is delicious: <b></b></span>';
            $('.apple').wrap(delicious);
            (0, vitest_1.expect)($('b>.apple')).toHaveLength(1);
        });
        (0, vitest_1.it)('(selector) : wraps the content with a copy of the first matched element', () => {
            $('.apple').wrap('.orange, .pear');
            const $oranges = $('.orange');
            (0, vitest_1.expect)($('.pear')).toHaveLength(1);
            (0, vitest_1.expect)($oranges).toHaveLength(2);
            (0, vitest_1.expect)($oranges.eq(0).parent()[0]).toBe($fruits[0]);
            (0, vitest_1.expect)($oranges.eq(0).children()).toHaveLength(1);
            (0, vitest_1.expect)($oranges.eq(0).children()[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($('.apple').parent()[0]).toBe($oranges[0]);
            (0, vitest_1.expect)($oranges.eq(1).children()).toHaveLength(0);
        });
        (0, vitest_1.it)('(fn) : should invoke the provided function with the correct arguments and context', () => {
            const $children = $fruits.children();
            const args = [];
            const thisValues = [];
            $children.wrap(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return '';
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, $children[0]],
                [1, $children[1]],
                [2, $children[2]],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([
                $children[0],
                $children[1],
                $children[2],
            ]);
        });
        (0, vitest_1.it)('(fn) : should use the returned HTML to wrap each element', () => {
            const $children = $fruits.children();
            const tagNames = ['div', 'span', 'p'];
            $children.wrap(() => `<${tagNames.shift()}>`);
            (0, vitest_1.expect)($fruits.find('div')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('div')[0]).toBe($fruits.children()[0]);
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.apple').parent()[0]).toBe($fruits.find('div')[0]);
            (0, vitest_1.expect)($fruits.find('span')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')[0]).toBe($fruits.children()[1]);
            (0, vitest_1.expect)($fruits.find('.orange')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.orange').parent()[0]).toBe($fruits.find('span')[0]);
            (0, vitest_1.expect)($fruits.find('p')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')[0]).toBe($fruits.children()[2]);
            (0, vitest_1.expect)($fruits.find('.pear')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.pear').parent()[0]).toBe($fruits.find('p')[0]);
        });
        (0, vitest_1.it)('(fn) : should use the returned Cheerio object to wrap each element', () => {
            const $children = $fruits.children();
            const tagNames = ['span', 'p', 'div'];
            $children.wrap(() => $(`<${tagNames.shift()}>`));
            (0, vitest_1.expect)($fruits.find('span')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')[0]).toBe($fruits.children()[0]);
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.apple').parent()[0]).toBe($fruits.find('span')[0]);
            (0, vitest_1.expect)($fruits.find('p')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')[0]).toBe($fruits.children()[1]);
            (0, vitest_1.expect)($fruits.find('.orange')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.orange').parent()[0]).toBe($fruits.find('p')[0]);
            (0, vitest_1.expect)($fruits.find('div')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('div')[0]).toBe($fruits.children()[2]);
            (0, vitest_1.expect)($fruits.find('.pear')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('.pear').parent()[0]).toBe($fruits.find('div')[0]);
        });
        (0, vitest_1.it)('($(...)) : for each element it should add a wrapper element and add the selected element as its child', () => {
            const $fruitDecorator = $('<div class="fruit-decorator"></div>');
            $('li').wrap($fruitDecorator);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(0).children().eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).children().eq(0).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).children().eq(0).hasClass('pear')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.wrapInner', () => {
        (0, vitest_1.it)('(Cheerio object) : should insert the element and add selected element(s) as its parent', () => {
            const $container = $('<div class="container"></div>');
            $fruits.wrapInner($container);
            (0, vitest_1.expect)($fruits.children()[0]).toBe($container[0]);
            (0, vitest_1.expect)($container[0].parent).toBe($fruits[0]);
            (0, vitest_1.expect)($container[0].children[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($container[0].children[1]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($('.apple')[0].parent).toBe($container[0]);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(1);
            (0, vitest_1.expect)($container.children()).toHaveLength(3);
        });
        (0, vitest_1.it)('(element) : should insert the element and add selected element(s) as its parent', () => {
            const $container = $('<div class="container"></div>');
            $fruits.wrapInner($container[0]);
            (0, vitest_1.expect)($fruits.children()[0]).toBe($container[0]);
            (0, vitest_1.expect)($container[0].parent).toBe($fruits[0]);
            (0, vitest_1.expect)($container[0].children[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($container[0].children[1]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($('.apple')[0].parent).toBe($container[0]);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(1);
            (0, vitest_1.expect)($container.children()).toHaveLength(3);
        });
        (0, vitest_1.it)('(html) : should ignore text nodes', () => {
            const $test = (0, index_js_1.load)(fixtures_js_1.mixedText);
            $test($test('body')[0].children).wrapInner('<test>');
            (0, vitest_1.expect)($test('body').html()).toBe('<a><test>1</test></a>TEXT<b><test>2</test></b>');
        });
        (0, vitest_1.it)('(html) : should insert the element and add selected element(s) as its parent', () => {
            $fruits.wrapInner('<div class="container"></div>');
            (0, vitest_1.expect)($fruits.children()[0]).toBe($('.container')[0]);
            (0, vitest_1.expect)($('.container')[0].parent).toBe($fruits[0]);
            (0, vitest_1.expect)($('.container')[0].children[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($('.container')[0].children[1]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($('.apple')[0].parent).toBe($('.container')[0]);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(1);
            (0, vitest_1.expect)($('.container').children()).toHaveLength(3);
        });
        (0, vitest_1.it)("(selector) : should wrap the html of the element with the selector's first match", () => {
            $('.apple').wrapInner('.orange, .pear');
            const $oranges = $('.orange');
            (0, vitest_1.expect)($('.pear')).toHaveLength(1);
            (0, vitest_1.expect)($oranges).toHaveLength(2);
            (0, vitest_1.expect)($oranges.eq(0).parent()[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($oranges.eq(0).text()).toBe('Apple');
            (0, vitest_1.expect)($('.apple').eq(0).children()[0]).toBe($oranges[0]);
            (0, vitest_1.expect)($oranges.eq(1).parent()[0]).toBe($fruits[0]);
            (0, vitest_1.expect)($oranges.eq(1).text()).toBe('Orange');
        });
        (0, vitest_1.it)('(fn) : should invoke the provided function with the correct arguments and context', () => {
            const $children = $fruits.children();
            const args = [];
            const thisValues = [];
            $children.wrapInner(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return this;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, $children[0]],
                [1, $children[1]],
                [2, $children[2]],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([
                $children[0],
                $children[1],
                $children[2],
            ]);
        });
        (0, vitest_1.it)("(fn) : should use the returned HTML to wrap each element's contents", () => {
            const $children = $fruits.children();
            const tagNames = ['div', 'span', 'p'];
            $children.wrapInner(() => `<${tagNames.shift()}>`);
            (0, vitest_1.expect)($fruits.find('div')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('div')[0]).toBe($('.apple').children()[0]);
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')[0]).toBe($('.orange').children()[0]);
            (0, vitest_1.expect)($fruits.find('.orange')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')[0]).toBe($('.pear').children()[0]);
            (0, vitest_1.expect)($fruits.find('.pear')).toHaveLength(1);
        });
        (0, vitest_1.it)("(fn) : should use the returned Cheerio object to wrap each element's contents", () => {
            const $children = $fruits.children();
            const tags = [$('<div></div>'), $('<span></span>'), $('<p></p>')];
            $children.wrapInner(() => tags.shift());
            (0, vitest_1.expect)($fruits.find('div')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('div')[0]).toBe($('.apple').children()[0]);
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('span')[0]).toBe($('.orange').children()[0]);
            (0, vitest_1.expect)($fruits.find('.orange')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')).toHaveLength(1);
            (0, vitest_1.expect)($fruits.find('p')[0]).toBe($('.pear').children()[0]);
            (0, vitest_1.expect)($fruits.find('.pear')).toHaveLength(1);
        });
        (0, vitest_1.it)('($(...)) : for each element it should add a wrapper element and add the selected element as its child', () => {
            const $fruitDecorator = $('<div class="fruit-decorator"></div>');
            const $children = $fruits.children();
            $('li').wrapInner($fruitDecorator);
            (0, vitest_1.expect)($('.fruit-decorator')).toHaveLength(3);
            (0, vitest_1.expect)($children.eq(0).children().eq(0).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($children.eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($children.eq(1).children().eq(0).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($children.eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($children.eq(2).children().eq(0).hasClass('fruit-decorator')).toBe(true);
            (0, vitest_1.expect)($children.eq(2).hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(html) : wraps with nested elements', () => {
            const $badOrangeJoke = $('<div class="orange-you-glad"><div class="i-didnt-say-apple"></div></div>');
            $('.orange').wrapInner($badOrangeJoke);
            (0, vitest_1.expect)($('.orange').children().eq(0).hasClass('orange-you-glad')).toBe(true);
            (0, vitest_1.expect)($('.orange-you-glad').children().eq(0).hasClass('i-didnt-say-apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.orange-you-glad').children()).toHaveLength(1);
        });
        (0, vitest_1.it)('(html) : should only worry about the first tag children', () => {
            const delicious = '<span> This guy is delicious: <b></b></span>';
            $('.apple').wrapInner(delicious);
            (0, vitest_1.expect)($('.apple>span>b')).toHaveLength(1);
            (0, vitest_1.expect)($('.apple>span>b').text()).toBe('Apple');
        });
    });
    (0, vitest_1.describe)('.unwrap', () => {
        let $elem;
        (0, vitest_1.beforeEach)(() => {
            $elem = (0, index_js_1.load)(fixtures_js_1.unwrapspans);
        });
        (0, vitest_1.it)('() : should be unwrap span elements', () => {
            const abcd = $elem('#unwrap1 > span, #unwrap2 > span').get();
            const abcdef = $elem('#unwrap span').get();
            // Make #unwrap1 and #unwrap2 go away
            (0, vitest_1.expect)($elem('#unwrap1 span').add('#unwrap2 span:first-child').unwrap()).toHaveLength(3);
            /*
             * .toEqual
             *  all four spans should still exist
             */
            (0, vitest_1.expect)($elem('#unwrap > span').get()).toEqual(abcd);
            // Make all b elements in #unwrap3 go away
            (0, vitest_1.expect)($elem('#unwrap3 span').unwrap().get()).toEqual($elem('#unwrap3 > span').get());
            // Make #unwrap3 go away
            (0, vitest_1.expect)($elem('#unwrap3 span').unwrap().get()).toEqual($elem('#unwrap > span.unwrap3').get());
            // #unwrap only contains 6 child spans
            (0, vitest_1.expect)($elem('#unwrap').children().get()).toEqual(abcdef);
            // Make the 6 spans become children of body
            (0, vitest_1.expect)($elem('#unwrap > span').unwrap().get()).toEqual($elem('body > span.unwrap').get());
            // Can't unwrap children of body
            (0, vitest_1.expect)($elem('body > span.unwrap').unwrap().get()).toEqual($elem('body > span.unwrap').get());
            // Can't unwrap children of body
            (0, vitest_1.expect)($elem('body > span.unwrap').unwrap().get()).toEqual(abcdef);
            // Can't unwrap children of body
            (0, vitest_1.expect)($elem('body > span.unwrap').get()).toEqual(abcdef);
        });
        (0, vitest_1.it)('(selector) : should only unwrap element parent what specified', () => {
            const abcd = $elem('#unwrap1 > span, #unwrap2 > span').get();
            // Shouldn't unwrap, no match
            $elem('#unwrap1 span').unwrap('#unwrap2');
            (0, vitest_1.expect)($elem('#unwrap1')).toHaveLength(1);
            // Shouldn't unwrap, no match
            $elem('#unwrap1 span').unwrap('span');
            (0, vitest_1.expect)($elem('#unwrap1')).toHaveLength(1);
            // Unwraps
            $elem('#unwrap1 span').unwrap('#unwrap1');
            (0, vitest_1.expect)($elem('#unwrap1')).toHaveLength(0);
            // Should not unwrap - unmatched unwrap
            $elem('#unwrap2 span').unwrap('quote');
            (0, vitest_1.expect)($elem('#unwrap > span')).toHaveLength(2);
            // Check return values - matched unwrap
            $elem('#unwrap2 span').unwrap('#unwrap2');
            (0, vitest_1.expect)($elem('#unwrap > span').get()).toEqual(abcd);
        });
    });
    (0, vitest_1.describe)('.wrapAll', () => {
        let doc;
        let $inner;
        (0, vitest_1.beforeEach)(() => {
            doc = (0, index_js_1.load)(fixtures_js_1.divcontainers);
            $inner = doc('.inner');
        });
        (0, vitest_1.it)('(Cheerio object) : should insert the element and wrap elements with it', () => {
            $inner.wrapAll(doc('#new'));
            const $container = doc('.container');
            const $wrap = doc('b');
            (0, vitest_1.expect)($container).toHaveLength(2);
            (0, vitest_1.expect)($container[0].children).toHaveLength(1);
            (0, vitest_1.expect)($container[1].children).toHaveLength(0);
            (0, vitest_1.expect)($container[0].children[0]).toBe(doc('#new')[0]);
            (0, vitest_1.expect)($inner).toHaveLength(4);
            (0, vitest_1.expect)($wrap[0].children).toHaveLength(4);
            (0, vitest_1.expect)($inner[0].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[1].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[2].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[3].parent).toBe($wrap[0]);
        });
        (0, vitest_1.it)('(html) : should wrap elements with it', () => {
            $inner.wrapAll('<div class="wrap"></div>');
            const $container = doc('.container');
            const $wrap = doc('.wrap');
            (0, vitest_1.expect)($inner).toHaveLength(4);
            (0, vitest_1.expect)($container).toHaveLength(2);
            (0, vitest_1.expect)($wrap).toHaveLength(1);
            (0, vitest_1.expect)($wrap[0].children).toHaveLength(4);
            (0, vitest_1.expect)($container[0].children).toHaveLength(1);
            (0, vitest_1.expect)($container[1].children).toHaveLength(0);
            (0, vitest_1.expect)($inner[0].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[1].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[2].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[3].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($wrap[0].parent).toBe($container[0]);
            (0, vitest_1.expect)($container[0].children[0]).toBe($wrap[0]);
        });
        (0, vitest_1.it)('(html) : should wrap single element with it', () => {
            const parent = doc('<p>').wrapAll('<div></div>').parent();
            (0, vitest_1.expect)(parent).toHaveLength(1);
            (0, vitest_1.expect)(parent.is('div')).toBe(true);
        });
        (0, vitest_1.it)('(selector) : should find element from dom, wrap elements with it', () => {
            $inner.wrapAll('#new');
            const $container = doc('.container');
            const $wrap = doc('b');
            const $new = doc('#new');
            (0, vitest_1.expect)($inner).toHaveLength(4);
            (0, vitest_1.expect)($container).toHaveLength(2);
            (0, vitest_1.expect)($container[0].children).toHaveLength(1);
            (0, vitest_1.expect)($container[1].children).toHaveLength(0);
            (0, vitest_1.expect)($wrap[0].children).toHaveLength(4);
            (0, vitest_1.expect)($inner[0].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[1].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[2].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($inner[3].parent).toBe($wrap[0]);
            (0, vitest_1.expect)($new[0].parent).toBe($container[0]);
            (0, vitest_1.expect)($container[0].children[0]).toBe($new[0]);
        });
        (0, vitest_1.it)('(function) : check execution', () => {
            const $container = doc('.container');
            const p = $container[0].parent;
            const result = $container.wrapAll(() => "<div class='red'><div class='tmp'></div></div>");
            (0, vitest_1.expect)(result.parent()).toHaveLength(1);
            (0, vitest_1.expect)($container.eq(0).parent().parent().is('.red')).toBe(true);
            (0, vitest_1.expect)($container.eq(1).parent().parent().is('.red')).toBe(true);
            (0, vitest_1.expect)($container.eq(0).parent().parent().parent().is(p)).toBe(true);
        });
        (0, vitest_1.it)('(function) : check execution characteristics', () => {
            const $new = doc('#new');
            let i = 0;
            doc('no-result').wrapAll(() => {
                i++;
                return '';
            });
            (0, vitest_1.expect)(i).toBeFalsy();
            $new.wrapAll(function (index) {
                (0, vitest_1.expect)(this).toBe($new[0]);
                (0, vitest_1.expect)(index).toBe(0);
                return this;
            });
        });
        (0, vitest_1.it)('(nodes) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.wrapAll($text('body')[0].children.slice(1));
            (0, vitest_1.expect)($text('body').html()).toBe('TEXT<b>2<a>1</a>TEXT<b>2</b></b>');
        });
    });
    (0, vitest_1.describe)('.append', () => {
        (0, vitest_1.it)('() : should do nothing', () => {
            (0, vitest_1.expect)($('#fruits').append()[0].tagName).toBe('ul');
        });
        (0, vitest_1.it)('(null) :  should do nothing', () => {
            $fruits.append(null);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(3);
        });
        (0, vitest_1.it)('(html) : should add element as last child', () => {
            $fruits.append('<li class="plum">Plum</li>');
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(html) : should not fail on text nodes', () => {
            (0, vitest_1.expect)($(fixtures_js_1.mixedText).append(' UP').html()).toBe('1 UP');
        });
        (0, vitest_1.it)('($(...)) : should add element as last child', () => {
            const $plum = $('<li class="plum">Plum</li>');
            $fruits.append($plum);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Node) : should add element as last child', () => {
            const plum = $('<li class="plum">Plum</li>')[0];
            $fruits.append(plum);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove node from previous location', () => {
            const apple = $fruits.children()[0];
            (0, vitest_1.expect)($fruits.children()).toHaveLength(3);
            $fruits.append(apple);
            const $children = $fruits.children();
            (0, vitest_1.expect)($children).toHaveLength(3);
            (0, vitest_1.expect)($children[0]).not.toBe(apple);
            (0, vitest_1.expect)($children[2]).toBe(apple);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing node from previous location', () => {
            const apple = $fruits.children()[0];
            const $dest = $('<div></div>');
            (0, vitest_1.expect)($fruits.children()).toHaveLength(3);
            $dest.append(apple);
            const $children = $fruits.children();
            (0, vitest_1.expect)($children).toHaveLength(2);
            (0, vitest_1.expect)($children[0]).not.toBe(apple);
            (0, vitest_1.expect)($dest.children()).toHaveLength(1);
            (0, vitest_1.expect)($dest.children()[0]).toBe(apple);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.pear').append($('.orange'));
            (0, vitest_1.expect)($('.apple').next()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.pear').prev()[0]).toBe($('.apple')[0]);
        });
        (0, vitest_1.it)('(existing Node) : should clone all but the last occurrence', () => {
            const $originalApple = $('.apple');
            $('.orange, .pear').append($originalApple);
            const $apples = $('.apple');
            (0, vitest_1.expect)($apples).toHaveLength(2);
            (0, vitest_1.expect)($apples.eq(0).parent()[0]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($apples.eq(1).parent()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($apples[1]).toBe($originalApple[0]);
        });
        (0, vitest_1.it)('(elem) : should NOP if removed', () => {
            const $apple = $('.apple');
            $apple.remove();
            $fruits.append($apple);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('apple')).toBe(true);
        });
        (0, vitest_1.it)('($(...), html) : should add multiple elements as last children', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const grape = '<li class="grape">Grape</li>';
            $fruits.append($plum, grape);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(4).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(Array) : should append all elements in the array', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>').get();
            $fruits.append(more);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(4).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should invoke the callback with the correct arguments and context', () => {
            const $fruits = $('#fruits').children();
            const args = [];
            const thisValues = [];
            $fruits.append(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return this;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, 'Apple'],
                [1, 'Orange'],
                [2, 'Pear'],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
        });
        (0, vitest_1.it)('(fn) : should add returned string as last child', () => {
            const $fruits = $('#fruits').children();
            $fruits.append(() => '<div class="first">');
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.first')[0]).toBe($apple.contents()[1]);
            (0, vitest_1.expect)($orange.find('.first')[0]).toBe($orange.contents()[1]);
            (0, vitest_1.expect)($pear.find('.first')[0]).toBe($pear.contents()[1]);
        });
        (0, vitest_1.it)('(fn) : should add returned Cheerio object as last child', () => {
            const $fruits = $('#fruits').children();
            $fruits.append(() => $('<div class="second">'));
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.second')[0]).toBe($apple.contents()[1]);
            (0, vitest_1.expect)($orange.find('.second')[0]).toBe($orange.contents()[1]);
            (0, vitest_1.expect)($pear.find('.second')[0]).toBe($pear.contents()[1]);
        });
        (0, vitest_1.it)('(fn) : should add returned Node as last child', () => {
            const $fruits = $('#fruits').children();
            $fruits.append(() => $('<div class="third">')[0]);
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.third')[0]).toBe($apple.contents()[1]);
            (0, vitest_1.expect)($orange.find('.third')[0]).toBe($orange.contents()[1]);
            (0, vitest_1.expect)($pear.find('.third')[0]).toBe($pear.contents()[1]);
        });
        (0, vitest_1.it)('should maintain correct object state (Issue: #10)', () => {
            const $obj = $('<div></div>')
                .append('<div><div></div></div>')
                .children()
                .children()
                .parent();
            (0, vitest_1.expect)($obj).toBeTruthy();
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const { parent } = $plum[0];
            (0, vitest_1.expect)(parent).toBeTruthy();
            $fruits.append($plum);
            (0, vitest_1.expect)($plum[0].parent?.type).not.toBe('root');
            (0, vitest_1.expect)(parent?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.prepend', () => {
        (0, vitest_1.it)('() : should do nothing', () => {
            (0, vitest_1.expect)($('#fruits').prepend()[0].tagName).toBe('ul');
        });
        (0, vitest_1.it)('(html) : should add element as first child', () => {
            $fruits.prepend('<li class="plum">Plum</li>');
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add element as first child', () => {
            const $plum = $('<li class="plum">Plum</li>');
            $fruits.prepend($plum);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add style element as first child', () => {
            const $style = $('<style>.foo {}</style>');
            $fruits.prepend($style);
            const styleTag = $fruits.children().get(0);
            (0, vitest_1.expect)(styleTag?.tagName).toBe('style');
            (0, vitest_1.expect)(styleTag?.children[0]).toHaveProperty('data', '.foo {}');
        });
        (0, vitest_1.it)('($(...)) : should add script element as first child', () => {
            const $script = $('<script>var foo;</script>');
            $fruits.prepend($script);
            const scriptTag = $fruits.children().get(0);
            (0, vitest_1.expect)(scriptTag?.tagName).toBe('script');
            (0, vitest_1.expect)(scriptTag?.children[0]).toHaveProperty('data', 'var foo;');
        });
        (0, vitest_1.it)('(Node) : should add node as first child', () => {
            const plum = $('<li class="plum">Plum</li>')[0];
            $fruits.prepend(plum);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing nodes from previous locations', () => {
            const pear = $fruits.children()[2];
            (0, vitest_1.expect)($fruits.children()).toHaveLength(3);
            $fruits.prepend(pear);
            const $children = $fruits.children();
            (0, vitest_1.expect)($children).toHaveLength(3);
            (0, vitest_1.expect)($children[2]).not.toBe(pear);
            (0, vitest_1.expect)($children[0]).toBe(pear);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.pear').prepend($('.orange'));
            (0, vitest_1.expect)($('.apple').next()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.pear').prev()[0]).toBe($('.apple')[0]);
        });
        (0, vitest_1.it)('(existing Node) : should clone all but the last occurrence', () => {
            const $originalApple = $('.apple');
            $('.orange, .pear').prepend($originalApple);
            const $apples = $('.apple');
            (0, vitest_1.expect)($apples).toHaveLength(2);
            (0, vitest_1.expect)($apples.eq(0).parent()[0]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($apples.eq(1).parent()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($apples[1]).toBe($originalApple[0]);
        });
        (0, vitest_1.it)('(elem) : should handle if removed', () => {
            const $apple = $('.apple');
            $apple.remove();
            $fruits.prepend($apple);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('apple')).toBe(true);
        });
        (0, vitest_1.it)('(Array) : should add all elements in the array as initial children', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>').get();
            $fruits.prepend(more);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(html, $(...), html) : should add multiple elements as first children', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const grape = '<li class="grape">Grape</li>';
            $fruits.prepend($plum, grape);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should invoke the callback with the correct arguments and context', () => {
            const args = [];
            const thisValues = [];
            const $fruits = $('#fruits').children();
            $fruits.prepend(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return this;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, 'Apple'],
                [1, 'Orange'],
                [2, 'Pear'],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
        });
        (0, vitest_1.it)('(fn) : should add returned string as first child', () => {
            const $fruits = $('#fruits').children();
            $fruits.prepend(() => '<div class="first">');
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.first')[0]).toBe($apple.contents()[0]);
            (0, vitest_1.expect)($orange.find('.first')[0]).toBe($orange.contents()[0]);
            (0, vitest_1.expect)($pear.find('.first')[0]).toBe($pear.contents()[0]);
        });
        (0, vitest_1.it)('(fn) : should add returned Cheerio object as first child', () => {
            const $fruits = $('#fruits').children();
            $fruits.prepend(() => $('<div class="second">'));
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.second')[0]).toBe($apple.contents()[0]);
            (0, vitest_1.expect)($orange.find('.second')[0]).toBe($orange.contents()[0]);
            (0, vitest_1.expect)($pear.find('.second')[0]).toBe($pear.contents()[0]);
        });
        (0, vitest_1.it)('(fn) : should add returned Node as first child', () => {
            const $fruits = $('#fruits').children();
            $fruits.prepend(() => $('<div class="third">')[0]);
            const $apple = $fruits.eq(0);
            const $orange = $fruits.eq(1);
            const $pear = $fruits.eq(2);
            (0, vitest_1.expect)($apple.find('.third')[0]).toBe($apple.contents()[0]);
            (0, vitest_1.expect)($orange.find('.third')[0]).toBe($orange.contents()[0]);
            (0, vitest_1.expect)($pear.find('.third')[0]).toBe($pear.contents()[0]);
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const root = $plum[0].parent;
            (0, vitest_1.expect)(root?.type).toBe('root');
            $fruits.prepend($plum);
            (0, vitest_1.expect)($plum[0].parent?.type).not.toBe('root');
            (0, vitest_1.expect)(root?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.appendTo', () => {
        (0, vitest_1.it)('(html) : should add element as last child', () => {
            const $plum = $('<li class="plum">Plum</li>').appendTo(fixtures_js_1.fruits);
            (0, vitest_1.expect)($plum.parent().children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add element as last child', () => {
            $('<li class="plum">Plum</li>').appendTo($fruits);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Node) : should add element as last child', () => {
            $('<li class="plum">Plum</li>').appendTo($fruits[0]);
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(selector) : should add element as last child', () => {
            $('<li class="plum">Plum</li>').appendTo('#fruits');
            (0, vitest_1.expect)($fruits.children().eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Array) : should add element as last child of all elements in the array', () => {
            const $multiple = $('<ul><li>Apple</li></ul><ul><li>Orange</li></ul>');
            $('<li class="plum">Plum</li>').appendTo($multiple.get());
            (0, vitest_1.expect)($multiple.first().children().eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($multiple.last().children().eq(1).hasClass('plum')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.prependTo', () => {
        (0, vitest_1.it)('(html) : should add element as first child', () => {
            const $plum = $('<li class="plum">Plum</li>').prependTo(fixtures_js_1.fruits);
            (0, vitest_1.expect)($plum.parent().children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add element as first child', () => {
            $('<li class="plum">Plum</li>').prependTo($fruits);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Node) : should add node as first child', () => {
            $('<li class="plum">Plum</li>').prependTo($fruits[0]);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(selector) : should add element as first child', () => {
            $('<li class="plum">Plum</li>').prependTo('#fruits');
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Array) : should add element as first child of all elements in the array', () => {
            const $multiple = $('<ul><li>Apple</li></ul><ul><li>Orange</li></ul>');
            $('<li class="plum">Plum</li>').prependTo($multiple.get());
            (0, vitest_1.expect)($multiple.first().children().eq(0).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($multiple.last().children().eq(0).hasClass('plum')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.after', () => {
        (0, vitest_1.it)('() : should do nothing', () => {
            (0, vitest_1.expect)($fruits.after()[0].tagName).toBe('ul');
        });
        (0, vitest_1.it)('(html) : should add element as next sibling', () => {
            const grape = '<li class="grape">Grape</li>';
            $('.apple').after(grape);
            (0, vitest_1.expect)($('.apple').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(Array) : should add all elements in the array as next sibling', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>').get();
            $('.apple').after(more);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add element as next sibling', () => {
            const $plum = $('<li class="plum">Plum</li>');
            $('.apple').after($plum);
            (0, vitest_1.expect)($('.apple').next().hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Node) : should add element as next sibling', () => {
            const plum = $('<li class="plum">Plum</li>')[0];
            $('.apple').after(plum);
            (0, vitest_1.expect)($('.apple').next().hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing nodes from previous locations', () => {
            const pear = $fruits.children()[2];
            $('.apple').after(pear);
            const $children = $fruits.children();
            (0, vitest_1.expect)($children).toHaveLength(3);
            (0, vitest_1.expect)($children[1]).toBe(pear);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.pear').after($('.orange'));
            (0, vitest_1.expect)($('.apple').next()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.pear').prev()[0]).toBe($('.apple')[0]);
        });
        (0, vitest_1.it)('(existing Node) : should clone all but the last occurrence', () => {
            const $originalApple = $('.apple');
            $('.orange, .pear').after($originalApple);
            (0, vitest_1.expect)($('.apple')).toHaveLength(2);
            (0, vitest_1.expect)($('.apple').eq(0).prev()[0]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($('.apple').eq(0).next()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.apple').eq(1).prev()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.apple').eq(1).next()).toHaveLength(0);
            (0, vitest_1.expect)($('.apple')[0]).not.toStrictEqual($originalApple[0]);
            (0, vitest_1.expect)($('.apple')[1]).toStrictEqual($originalApple[0]);
        });
        (0, vitest_1.it)('(elem) : should handle if removed', () => {
            const $apple = $('.apple');
            const $plum = $('<li class="plum">Plum</li>');
            $apple.remove();
            $apple.after($plum);
            (0, vitest_1.expect)($plum.prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('($(...), html) : should add multiple elements as next siblings', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const grape = '<li class="grape">Grape</li>';
            $('.apple').after($plum, grape);
            (0, vitest_1.expect)($('.apple').next().hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($('.plum').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should invoke the callback with the correct arguments and context', () => {
            const args = [];
            const thisValues = [];
            const $fruits = $('#fruits').children();
            $fruits.after(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return this;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, 'Apple'],
                [1, 'Orange'],
                [2, 'Pear'],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
        });
        (0, vitest_1.it)('(fn) : should add returned string as next sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.after(() => '<li class="first">');
            (0, vitest_1.expect)($('.first')[0]).toBe($('#fruits').contents()[1]);
            (0, vitest_1.expect)($('.first')[1]).toBe($('#fruits').contents()[3]);
            (0, vitest_1.expect)($('.first')[2]).toBe($('#fruits').contents()[5]);
        });
        (0, vitest_1.it)('(fn) : should add returned Cheerio object as next sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.after(() => $('<li class="second">'));
            (0, vitest_1.expect)($('.second')[0]).toBe($('#fruits').contents()[1]);
            (0, vitest_1.expect)($('.second')[1]).toBe($('#fruits').contents()[3]);
            (0, vitest_1.expect)($('.second')[2]).toBe($('#fruits').contents()[5]);
        });
        (0, vitest_1.it)('(fn) : should add returned element as next sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.after(() => $('<li class="third">')[0]);
            (0, vitest_1.expect)($('.third')[0]).toBe($('#fruits').contents()[1]);
            (0, vitest_1.expect)($('.third')[1]).toBe($('#fruits').contents()[3]);
            (0, vitest_1.expect)($('.third')[2]).toBe($('#fruits').contents()[5]);
        });
        (0, vitest_1.it)('(fn) : should support text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            $text($text('body')[0].children).after((_, content) => `<c>${content}added</c>`);
            (0, vitest_1.expect)($text('body').html()).toBe('<a>1</a><c>1added</c>TEXT<b>2</b><c>2added</c>');
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const root = $plum[0].parent;
            (0, vitest_1.expect)(root?.type).toBe('root');
            $fruits.after($plum);
            (0, vitest_1.expect)($plum[0].parent?.type).not.toBe('root');
            (0, vitest_1.expect)(root?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.insertAfter', () => {
        (0, vitest_1.it)('(selector) : should create element and add as next sibling', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertAfter('.apple');
            (0, vitest_1.expect)($('.apple').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(selector) : should create element and add as next sibling of multiple elements', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertAfter('.apple, .pear');
            (0, vitest_1.expect)($('.apple').next().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.pear').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create element and add as next sibling', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertAfter($('.apple'));
            (0, vitest_1.expect)($('.apple').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create element and add as next sibling of multiple elements', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertAfter($('.apple, .pear'));
            (0, vitest_1.expect)($('.apple').next().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.pear').next().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create all elements in the array and add as next siblings', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>');
            more.insertAfter($('.apple'));
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing nodes from previous locations', () => {
            $('.orange').insertAfter('.pear');
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('orange')).toBe(false);
            (0, vitest_1.expect)($fruits.children().length).toBe(3);
            (0, vitest_1.expect)($('.orange').length).toBe(1);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.orange').insertAfter('.pear');
            (0, vitest_1.expect)($('.apple').next().hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.pear').next().hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($('.orange').next()).toHaveLength(0);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings of multiple elements', () => {
            $('.apple').insertAfter('.orange, .pear');
            (0, vitest_1.expect)($('.orange').prev()).toHaveLength(0);
            (0, vitest_1.expect)($('.orange').next().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.pear').next().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($fruits.children().length).toBe(4);
            const apples = $('.apple');
            (0, vitest_1.expect)(apples.length).toBe(2);
            (0, vitest_1.expect)(apples.eq(0).prev().hasClass('orange')).toBe(true);
            (0, vitest_1.expect)(apples.eq(1).prev().hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(elem) : should handle if removed', () => {
            const $apple = $('.apple');
            const $plum = $('<li class="plum">Plum</li>');
            $apple.remove();
            $plum.insertAfter($apple);
            (0, vitest_1.expect)($plum.prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('(single) should return the new element for chaining', () => {
            const $grape = $('<li class="grape">Grape</li>').insertAfter('.apple');
            (0, vitest_1.expect)($grape.cheerio).toBeTruthy();
            (0, vitest_1.expect)($grape.each).toBeTruthy();
            (0, vitest_1.expect)($grape.length).toBe(1);
            (0, vitest_1.expect)($grape.hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the new elements for chaining', () => {
            const $purple = $('<li class="grape">Grape</li><li class="plum">Plum</li>').insertAfter('.apple');
            (0, vitest_1.expect)($purple.cheerio).toBeTruthy();
            (0, vitest_1.expect)($purple.each).toBeTruthy();
            (0, vitest_1.expect)($purple.length).toBe(2);
            (0, vitest_1.expect)($purple.eq(0).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(1).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(multiple) should return the new elements for chaining', () => {
            const $purple = $('<li class="grape">Grape</li><li class="plum">Plum</li>').insertAfter('.apple, .pear');
            (0, vitest_1.expect)($purple.cheerio).toBeTruthy();
            (0, vitest_1.expect)($purple.each).toBeTruthy();
            (0, vitest_1.expect)($purple.length).toBe(4);
            (0, vitest_1.expect)($purple.eq(0).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($purple.eq(2).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the existing element for chaining', () => {
            const $pear = $('.pear').insertAfter('.apple');
            (0, vitest_1.expect)($pear.cheerio).toBeTruthy();
            (0, vitest_1.expect)($pear.each).toBeTruthy();
            (0, vitest_1.expect)($pear.length).toBe(1);
            (0, vitest_1.expect)($pear.hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the existing elements for chaining', () => {
            const $things = $('.orange, .apple').insertAfter('.pear');
            (0, vitest_1.expect)($things.cheerio).toBeTruthy();
            (0, vitest_1.expect)($things.each).toBeTruthy();
            (0, vitest_1.expect)($things.length).toBe(2);
            (0, vitest_1.expect)($things.eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($things.eq(1).hasClass('orange')).toBe(true);
        });
        (0, vitest_1.it)('(multiple) should return the existing elements for chaining', () => {
            $('<li class="grape">Grape</li>').insertAfter('.apple');
            const $things = $('.orange, .apple').insertAfter('.pear, .grape');
            (0, vitest_1.expect)($things.cheerio).toBeTruthy();
            (0, vitest_1.expect)($things.each).toBeTruthy();
            (0, vitest_1.expect)($things.length).toBe(4);
            (0, vitest_1.expect)($things.eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($things.eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($things.eq(2).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($things.eq(3).hasClass('orange')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.before', () => {
        (0, vitest_1.it)('() : should do nothing', () => {
            (0, vitest_1.expect)($('#fruits').before()[0].tagName).toBe('ul');
        });
        (0, vitest_1.it)('(html) : should add element as previous sibling', () => {
            const grape = '<li class="grape">Grape</li>';
            $('.apple').before(grape);
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should add element as previous sibling', () => {
            const $plum = $('<li class="plum">Plum</li>');
            $('.apple').before($plum);
            (0, vitest_1.expect)($('.apple').prev().hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(Node) : should add element as previous sibling', () => {
            const plum = $('<li class="plum">Plum</li>')[0];
            $('.apple').before(plum);
            (0, vitest_1.expect)($('.apple').prev().hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing nodes from previous locations', () => {
            const pear = $fruits.children()[2];
            $('.apple').before(pear);
            const $children = $fruits.children();
            (0, vitest_1.expect)($children).toHaveLength(3);
            (0, vitest_1.expect)($children[0]).toBe(pear);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.apple').before($('.orange'));
            (0, vitest_1.expect)($('.apple').next()[0]).toBe($('.pear')[0]);
            (0, vitest_1.expect)($('.pear').prev()[0]).toBe($('.apple')[0]);
        });
        (0, vitest_1.it)('(existing Node) : should clone all but the last occurrence', () => {
            const $originalPear = $('.pear');
            $('.apple, .orange').before($originalPear);
            (0, vitest_1.expect)($('.pear')).toHaveLength(2);
            (0, vitest_1.expect)($('.pear').eq(0).prev()).toHaveLength(0);
            (0, vitest_1.expect)($('.pear').eq(0).next()[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($('.pear').eq(1).prev()[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($('.pear').eq(1).next()[0]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($('.pear')[0]).not.toStrictEqual($originalPear[0]);
            (0, vitest_1.expect)($('.pear')[1]).toStrictEqual($originalPear[0]);
        });
        (0, vitest_1.it)('(elem) : should handle if removed', () => {
            const $apple = $('.apple');
            const $plum = $('<li class="plum">Plum</li>');
            $apple.remove();
            $apple.before($plum);
            (0, vitest_1.expect)($plum.next()).toHaveLength(0);
        });
        (0, vitest_1.it)('(Array) : should add all elements in the array as previous sibling', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>').get();
            $('.apple').before(more);
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...), html) : should add multiple elements as previous siblings', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const grape = '<li class="grape">Grape</li>';
            $('.apple').before($plum, grape);
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.grape').prev().hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(fn) : should invoke the callback with the correct arguments and context', () => {
            const args = [];
            const thisValues = [];
            const $fruits = $('#fruits').children();
            $fruits.before(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return this;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, 'Apple'],
                [1, 'Orange'],
                [2, 'Pear'],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
        });
        (0, vitest_1.it)('(fn) : should add returned string as previous sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.before(() => '<li class="first">');
            (0, vitest_1.expect)($('.first')[0]).toBe($('#fruits').contents()[0]);
            (0, vitest_1.expect)($('.first')[1]).toBe($('#fruits').contents()[2]);
            (0, vitest_1.expect)($('.first')[2]).toBe($('#fruits').contents()[4]);
        });
        (0, vitest_1.it)('(fn) : should add returned Cheerio object as previous sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.before(() => $('<li class="second">'));
            (0, vitest_1.expect)($('.second')[0]).toBe($('#fruits').contents()[0]);
            (0, vitest_1.expect)($('.second')[1]).toBe($('#fruits').contents()[2]);
            (0, vitest_1.expect)($('.second')[2]).toBe($('#fruits').contents()[4]);
        });
        (0, vitest_1.it)('(fn) : should add returned Node as previous sibling', () => {
            const $fruits = $('#fruits').children();
            $fruits.before(() => $('<li class="third">')[0]);
            (0, vitest_1.expect)($('.third')[0]).toBe($('#fruits').contents()[0]);
            (0, vitest_1.expect)($('.third')[1]).toBe($('#fruits').contents()[2]);
            (0, vitest_1.expect)($('.third')[2]).toBe($('#fruits').contents()[4]);
        });
        (0, vitest_1.it)('(fn) : should support text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            $text($text('body')[0].children).before((_, content) => `<c>${content}added</c>`);
            (0, vitest_1.expect)($text('body').html()).toBe('<c>1added</c><a>1</a>TEXT<c>2added</c><b>2</b>');
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const root = $plum[0].parent;
            (0, vitest_1.expect)(root?.type).toBe('root');
            $fruits.before($plum);
            (0, vitest_1.expect)($plum[0].parent?.type).not.toBe('root');
            (0, vitest_1.expect)(root?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.insertBefore', () => {
        (0, vitest_1.it)('(selector) : should create element and add as prev sibling', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertBefore('.apple');
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(selector) : should create element and add as prev sibling of multiple elements', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertBefore('.apple, .pear');
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create element and add as prev sibling', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertBefore($('.apple'));
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create element and add as next sibling of multiple elements', () => {
            const grape = $('<li class="grape">Grape</li>');
            grape.insertBefore($('.apple, .pear'));
            (0, vitest_1.expect)($('.apple').prev().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('($(...)) : should create all elements in the array and add as prev siblings', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>');
            more.insertBefore($('.apple'));
            (0, vitest_1.expect)($fruits.children().eq(0).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('apple')).toBe(true);
        });
        (0, vitest_1.it)('(existing Node) : should remove existing nodes from previous locations', () => {
            $('.pear').insertBefore('.apple');
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('pear')).toBe(false);
            (0, vitest_1.expect)($fruits.children().length).toBe(3);
            (0, vitest_1.expect)($('.pear').length).toBe(1);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings', () => {
            $('.pear').insertBefore('.apple');
            (0, vitest_1.expect)($('.apple').prev().hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.apple').next().hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($('.pear').next().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('(existing Node) : should update original direct siblings of multiple elements', () => {
            $('.pear').insertBefore('.apple, .orange');
            (0, vitest_1.expect)($('.apple').prev().hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.apple').next().hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.orange').prev().hasClass('pear')).toBe(true);
            (0, vitest_1.expect)($('.orange').next()).toHaveLength(0);
            (0, vitest_1.expect)($fruits.children().length).toBe(4);
            const pears = $('.pear');
            (0, vitest_1.expect)(pears.length).toBe(2);
            (0, vitest_1.expect)(pears.eq(0).next().hasClass('apple')).toBe(true);
            (0, vitest_1.expect)(pears.eq(1).next().hasClass('orange')).toBe(true);
        });
        (0, vitest_1.it)('(elem) : should handle if removed', () => {
            const $apple = $('.apple');
            const $plum = $('<li class="plum">Plum</li>');
            $apple.remove();
            $plum.insertBefore($apple);
            (0, vitest_1.expect)($plum.next()).toHaveLength(0);
        });
        (0, vitest_1.it)('(single) should return the new element for chaining', () => {
            const $grape = $('<li class="grape">Grape</li>').insertBefore('.apple');
            (0, vitest_1.expect)($grape.cheerio).toBeTruthy();
            (0, vitest_1.expect)($grape.each).toBeTruthy();
            (0, vitest_1.expect)($grape.length).toBe(1);
            (0, vitest_1.expect)($grape.hasClass('grape')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the new elements for chaining', () => {
            const $purple = $('<li class="grape">Grape</li><li class="plum">Plum</li>').insertBefore('.apple');
            (0, vitest_1.expect)($purple.cheerio).toBeTruthy();
            (0, vitest_1.expect)($purple.each).toBeTruthy();
            (0, vitest_1.expect)($purple.length).toBe(2);
            (0, vitest_1.expect)($purple.eq(0).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(1).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(multiple) should return the new elements for chaining', () => {
            const $purple = $('<li class="grape">Grape</li><li class="plum">Plum</li>').insertBefore('.apple, .pear');
            (0, vitest_1.expect)($purple.cheerio).toBeTruthy();
            (0, vitest_1.expect)($purple.each).toBeTruthy();
            (0, vitest_1.expect)($purple.length).toBe(4);
            (0, vitest_1.expect)($purple.eq(0).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($purple.eq(2).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($purple.eq(3).hasClass('plum')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the existing element for chaining', () => {
            const $orange = $('.orange').insertBefore('.apple');
            (0, vitest_1.expect)($orange.cheerio).toBeTruthy();
            (0, vitest_1.expect)($orange.each).toBeTruthy();
            (0, vitest_1.expect)($orange.length).toBe(1);
            (0, vitest_1.expect)($orange.hasClass('orange')).toBe(true);
        });
        (0, vitest_1.it)('(single) should return the existing elements for chaining', () => {
            const $things = $('.orange, .pear').insertBefore('.apple');
            (0, vitest_1.expect)($things.cheerio).toBeTruthy();
            (0, vitest_1.expect)($things.each).toBeTruthy();
            (0, vitest_1.expect)($things.length).toBe(2);
            (0, vitest_1.expect)($things.eq(0).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($things.eq(1).hasClass('pear')).toBe(true);
        });
        (0, vitest_1.it)('(multiple) should return the existing elements for chaining', () => {
            $('<li class="grape">Grape</li>').insertBefore('.apple');
            const $things = $('.orange, .apple').insertBefore('.pear, .grape');
            (0, vitest_1.expect)($things.cheerio).toBeTruthy();
            (0, vitest_1.expect)($things.each).toBeTruthy();
            (0, vitest_1.expect)($things.length).toBe(4);
            (0, vitest_1.expect)($things.eq(0).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($things.eq(1).hasClass('orange')).toBe(true);
            (0, vitest_1.expect)($things.eq(2).hasClass('apple')).toBe(true);
            (0, vitest_1.expect)($things.eq(3).hasClass('orange')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.remove', () => {
        (0, vitest_1.it)('() : should remove selected elements', () => {
            $('.apple').remove();
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should be reentrant', () => {
            const $apple = $('.apple');
            $apple.remove();
            $apple.remove();
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(0);
        });
        (0, vitest_1.it)('(selector) : should remove matching selected elements', () => {
            $('li').remove('.apple');
            (0, vitest_1.expect)($fruits.find('.apple')).toHaveLength(0);
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const root = $plum[0].parent;
            (0, vitest_1.expect)(root?.type).toBe('root');
            $plum.remove();
            (0, vitest_1.expect)($plum[0].parent).toBe(null);
            (0, vitest_1.expect)(root?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.replaceWith', () => {
        (0, vitest_1.it)('(elem) : should replace one <li> tag with another', () => {
            const $plum = $('<li class="plum">Plum</li>');
            $('.orange').replaceWith($plum);
            (0, vitest_1.expect)($('.apple').next().hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($('.apple').next().html()).toBe('Plum');
            (0, vitest_1.expect)($('.pear').prev().hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().html()).toBe('Plum');
        });
        (0, vitest_1.it)('(Array) : should replace one <li> tag with the elements in the array', () => {
            const more = $('<li class="plum">Plum</li><li class="grape">Grape</li>').get();
            $('.orange').replaceWith(more);
            (0, vitest_1.expect)($fruits.children().eq(1).hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($fruits.children().eq(2).hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($('.apple').next().hasClass('plum')).toBe(true);
            (0, vitest_1.expect)($('.pear').prev().hasClass('grape')).toBe(true);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(4);
        });
        (0, vitest_1.it)('(Node) : should replace the selected element with given node', () => {
            const $src = $('<h2>hi <span>there</span></h2>');
            const $new = $('<ul></ul>');
            const $replaced = $src.find('span').replaceWith($new[0]);
            (0, vitest_1.expect)($new[0].parentNode).toBe($src[0]);
            (0, vitest_1.expect)($replaced[0].parentNode).toBe(null);
            (0, vitest_1.expect)($.html($src)).toBe('<h2>hi <ul></ul></h2>');
        });
        (0, vitest_1.it)('(existing element) : should remove element from its previous location', () => {
            $('.pear').replaceWith($('.apple'));
            (0, vitest_1.expect)($fruits.children()).toHaveLength(2);
            (0, vitest_1.expect)($fruits.children()[0]).toBe($('.orange')[0]);
            (0, vitest_1.expect)($fruits.children()[1]).toBe($('.apple')[0]);
        });
        (0, vitest_1.it)('(elem) : should NOP if removed', () => {
            const $pear = $('.pear');
            const $plum = $('<li class="plum">Plum</li>');
            $pear.remove();
            $pear.replaceWith($plum);
            (0, vitest_1.expect)($('.orange').next().hasClass('plum')).toBe(false);
        });
        (0, vitest_1.it)('(elem) : should replace the single selected element with given element', () => {
            const $src = $('<h2>hi <span>there</span></h2>');
            const $new = $('<div>here</div>');
            const $replaced = $src.find('span').replaceWith($new);
            (0, vitest_1.expect)($new[0].parentNode).toBe($src[0]);
            (0, vitest_1.expect)($replaced[0].parentNode).toBe(null);
            (0, vitest_1.expect)($.html($src)).toBe('<h2>hi <div>here</div></h2>');
        });
        (0, vitest_1.it)('(self) : should be replaced after replacing it with itself', () => {
            const $a = (0, index_js_1.load)('<a>foo</a>', null, false);
            const replacement = '<a>bar</a>';
            $a('a').replaceWith((_, el) => el);
            $a('a').replaceWith(replacement);
            (0, vitest_1.expect)($a.html()).toBe(replacement);
        });
        (0, vitest_1.it)('(str) : should accept strings', () => {
            const $src = $('<h2>hi <span>there</span></h2>');
            const newStr = '<div>here</div>';
            const $replaced = $src.find('span').replaceWith(newStr);
            (0, vitest_1.expect)($replaced[0].parentNode).toBe(null);
            (0, vitest_1.expect)($.html($src)).toBe('<h2>hi <div>here</div></h2>');
        });
        (0, vitest_1.it)('(str) : should replace all selected elements', () => {
            const $src = $('<b>a<br>b<br>c<br>d</b>');
            const $replaced = $src.find('br').replaceWith(' ');
            (0, vitest_1.expect)($replaced[0].parentNode).toBe(null);
            (0, vitest_1.expect)($.html($src)).toBe('<b>a b c d</b>');
        });
        (0, vitest_1.it)('(fn) : should invoke the callback with the correct argument and context', () => {
            const origChildren = $fruits.children().get();
            const args = [];
            const thisValues = [];
            $fruits.children().replaceWith(function (...myArgs) {
                args.push(myArgs);
                thisValues.push(this);
                return '<li class="first">';
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, origChildren[0]],
                [1, origChildren[1]],
                [2, origChildren[2]],
            ]);
            (0, vitest_1.expect)(thisValues).toStrictEqual([
                origChildren[0],
                origChildren[1],
                origChildren[2],
            ]);
        });
        (0, vitest_1.it)('(fn) : should replace the selected element with the returned string', () => {
            $fruits.children().replaceWith(() => '<li class="first">');
            (0, vitest_1.expect)($fruits.find('.first')).toHaveLength(3);
        });
        (0, vitest_1.it)('(fn) : should replace the selected element with the returned Cheerio object', () => {
            $fruits.children().replaceWith(() => $('<li class="second">'));
            (0, vitest_1.expect)($fruits.find('.second')).toHaveLength(3);
        });
        (0, vitest_1.it)('(fn) : should replace the selected element with the returned node', () => {
            $fruits.children().replaceWith(() => $('<li class="third">')[0]);
            (0, vitest_1.expect)($fruits.find('.third')).toHaveLength(3);
        });
        (0, vitest_1.it)('($(...)) : should remove from root element', () => {
            const $plum = $('<li class="plum">Plum</li>');
            const root = $plum[0].parent;
            (0, vitest_1.expect)(root?.type).toBe('root');
            $fruits.children().replaceWith($plum);
            (0, vitest_1.expect)($plum[0].parent?.type).not.toBe('root');
            (0, vitest_1.expect)(root?.childNodes).not.toContain($plum[0]);
        });
    });
    (0, vitest_1.describe)('.empty', () => {
        (0, vitest_1.it)('() : should remove all children from selected elements', () => {
            (0, vitest_1.expect)($fruits.children()).toHaveLength(3);
            $fruits.empty();
            (0, vitest_1.expect)($fruits.children()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should allow element reinsertion', () => {
            const $children = $fruits.children();
            $fruits.empty();
            (0, vitest_1.expect)($fruits.children()).toHaveLength(0);
            (0, vitest_1.expect)($children).toHaveLength(3);
            $fruits.append($('<div></div><div></div>'));
            const $remove = $fruits.children().eq(0);
            $remove.replaceWith($children);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(4);
        });
        (0, vitest_1.it)("() : should destroy children's references to the parent", () => {
            const $children = $fruits.children();
            $fruits.empty();
            (0, vitest_1.expect)($children.eq(0).parent()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(0).next()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(0).prev()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(1).parent()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(1).next()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(1).prev()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(2).parent()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(2).next()).toHaveLength(0);
            (0, vitest_1.expect)($children.eq(2).prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.empty();
            (0, vitest_1.expect)($text('body').html()).toBe('<a></a>TEXT<b></b>');
        });
        (0, vitest_1.it)('() : should skip comment nodes', () => {
            const $comment = (0, index_js_1.load)('<a>1</a><!--Comment-->TEXT<b>2</b>');
            const $body = $comment($comment('body')[0].children);
            $body.empty();
            (0, vitest_1.expect)($comment('body').html()).toBe('<a></a><!--Comment-->TEXT<b></b>');
        });
    });
    (0, vitest_1.describe)('.html', () => {
        (0, vitest_1.it)('() : should get the innerHTML for an element', () => {
            (0, vitest_1.expect)($fruits.html()).toBe([
                '<li class="apple">Apple</li>',
                '<li class="orange">Orange</li>',
                '<li class="pear">Pear</li>',
            ].join(''));
        });
        (0, vitest_1.it)('() : should get innerHTML even if its just text', () => {
            (0, vitest_1.expect)($('.pear', '<li class="pear">Pear</li>').html()).toBe('Pear');
        });
        (0, vitest_1.it)('() : should return empty string if nothing inside', () => {
            (0, vitest_1.expect)($('li', '<li></li>').html()).toBe('');
        });
        (0, vitest_1.it)('(html) : should set the html for its children', () => {
            $fruits.html('<li class="durian">Durian</li>');
            const html = $fruits.html();
            (0, vitest_1.expect)(html).toBe('<li class="durian">Durian</li>');
        });
        (0, vitest_1.it)('(html) : should add new elements for each element in selection', () => {
            const $fruits = $('li');
            $fruits.html('<li class="durian">Durian</li>');
            let tested = 0;
            $fruits.each(function () {
                (0, vitest_1.expect)($(this).children().parent().get(0)).toBe(this);
                tested++;
            });
            (0, vitest_1.expect)(tested).toBe(3);
        });
        (0, vitest_1.it)('(html) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.html('test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a>test</a>TEXT<b>test</b>');
        });
        (0, vitest_1.it)('(elem) : should set the html for its children with element', () => {
            $fruits.html($('<li class="durian">Durian</li>'));
            const html = $fruits.html();
            (0, vitest_1.expect)(html).toBe('<li class="durian">Durian</li>');
        });
        (0, vitest_1.it)('(elem) : should move the passed element (#940)', () => {
            $('.apple').html($('.orange'));
            (0, vitest_1.expect)($fruits.html()).toBe('<li class="apple"><li class="orange">Orange</li></li><li class="pear">Pear</li>');
        });
        (0, vitest_1.it)('() : should allow element reinsertion', () => {
            const $children = $fruits.children();
            $fruits.html('<div></div><div></div>');
            (0, vitest_1.expect)($fruits.children()).toHaveLength(2);
            const $remove = $fruits.children().eq(0);
            $remove.replaceWith($children);
            (0, vitest_1.expect)($fruits.children()).toHaveLength(4);
        });
        (0, vitest_1.it)('(script value) : should add content as text', () => {
            const $data = '<a><b>';
            const $script = $('<script>').html($data);
            (0, vitest_1.expect)($script).toHaveLength(1);
            (0, vitest_1.expect)($script[0].type).toBe('script');
            (0, vitest_1.expect)($script[0]).toHaveProperty('name', 'script');
            (0, vitest_1.expect)($script[0].children).toHaveLength(1);
            (0, vitest_1.expect)($script[0].children[0].type).toBe('text');
            (0, vitest_1.expect)($script[0].children[0]).toHaveProperty('data', $data);
        });
    });
    (0, vitest_1.describe)('.toString', () => {
        (0, vitest_1.it)('() : should get the outerHTML for an element', () => {
            (0, vitest_1.expect)($fruits.toString()).toBe(fixtures_js_1.fruits);
        });
        (0, vitest_1.it)('() : should return an html string for a set of elements', () => {
            (0, vitest_1.expect)($fruits.find('li').toString()).toBe('<li class="apple">Apple</li><li class="orange">Orange</li><li class="pear">Pear</li>');
        });
        (0, vitest_1.it)('() : should be called implicitly', () => {
            const string = [$('<foo>'), $('<bar>'), $('<baz>')].join('');
            (0, vitest_1.expect)(string).toBe('<foo></foo><bar></bar><baz></baz>');
        });
        (0, vitest_1.it)('() : should pass options', () => {
            const dom = (0, index_js_1.load)('&', { xml: { decodeEntities: false } });
            (0, vitest_1.expect)(dom.root().toString()).toBe('&');
        });
    });
    (0, vitest_1.describe)('.text', () => {
        (0, vitest_1.it)('() : gets the text for a single element', () => {
            (0, vitest_1.expect)($('.apple').text()).toBe('Apple');
        });
        (0, vitest_1.it)('() : combines all text from children text nodes', () => {
            (0, vitest_1.expect)($('#fruits').text()).toBe('AppleOrangePear');
        });
        (0, vitest_1.it)('(text) : sets the text for the child node', () => {
            $('.apple').text('Granny Smith Apple');
            (0, vitest_1.expect)($('.apple')[0].childNodes[0]).toHaveProperty('data', 'Granny Smith Apple');
        });
        (0, vitest_1.it)('(text) : inserts separate nodes for all children', () => {
            $('li').text('Fruits');
            let tested = 0;
            $('li').each(function () {
                (0, vitest_1.expect)(this.childNodes[0].parentNode).toBe(this);
                tested++;
            });
            (0, vitest_1.expect)(tested).toBe(3);
        });
        (0, vitest_1.it)('(text) : should create a Node with the DOM level 1 API', () => {
            const $apple = $('.apple');
            $apple.text('anything');
            const textNode = $apple[0].childNodes[0];
            (0, vitest_1.expect)(textNode.parentNode).toBe($apple[0]);
            (0, vitest_1.expect)(textNode.nodeType).toBe(3);
            (0, vitest_1.expect)(textNode).toHaveProperty('data', 'anything');
        });
        (0, vitest_1.it)('(html) : should skip text nodes', () => {
            const $text = (0, index_js_1.load)(fixtures_js_1.mixedText);
            const $body = $text($text('body')[0].children);
            $body.text('test');
            (0, vitest_1.expect)($text('body').html()).toBe('<a>test</a>TEXT<b>test</b>');
        });
        (0, vitest_1.it)('should allow functions as arguments', () => {
            $('.apple').text((idx, content) => {
                (0, vitest_1.expect)(idx).toBe(0);
                (0, vitest_1.expect)(content).toBe('Apple');
                return 'whatever mate';
            });
            (0, vitest_1.expect)($('.apple')[0].childNodes[0]).toHaveProperty('data', 'whatever mate');
        });
        (0, vitest_1.it)('should allow functions as arguments for multiple elements', () => {
            $('li').text((idx) => `text${idx}`);
            $('li').each(function (idx) {
                (0, vitest_1.expect)(this.childNodes[0]).toHaveProperty('data', `text${idx}`);
            });
        });
        (0, vitest_1.it)('should decode special chars', () => {
            const text = $('<p>M&amp;M</p>').text();
            (0, vitest_1.expect)(text).toBe('M&M');
        });
        (0, vitest_1.it)('should work with special chars added as strings', () => {
            const text = $('<p>M&M</p>').text();
            (0, vitest_1.expect)(text).toBe('M&M');
        });
        (0, vitest_1.it)('should turn passed values to strings', () => {
            $('.apple').text(1);
            (0, vitest_1.expect)($('.apple')[0].childNodes[0]).toHaveProperty('data', '1');
        });
        (0, vitest_1.it)('( undefined ) : should act as an accessor', () => {
            const $div = $('<div>test</div>');
            (0, vitest_1.expect)(typeof $div.text(undefined)).toBe('string');
            (0, vitest_1.expect)($div.text()).toBe('test');
        });
        (0, vitest_1.it)('( "" ) : should convert to string', () => {
            const $div = $('<div>test</div>');
            (0, vitest_1.expect)($div.text('').text()).toBe('');
        });
        (0, vitest_1.it)('( null ) : should convert to string', () => {
            (0, vitest_1.expect)($('<div>')
                .text(null)
                .text()).toBe('null');
        });
        (0, vitest_1.it)('( 0 ) : should convert to string', () => {
            (0, vitest_1.expect)($('<div>')
                .text(0)
                .text()).toBe('0');
        });
        (0, vitest_1.it)('(str) should encode then decode unsafe characters', () => {
            const $apple = $('.apple');
            $apple.text('blah <script>alert("XSS!")</script> blah');
            (0, vitest_1.expect)($apple[0].childNodes[0]).toHaveProperty('data', 'blah <script>alert("XSS!")</script> blah');
            (0, vitest_1.expect)($apple.text()).toBe('blah <script>alert("XSS!")</script> blah');
            $apple.text('blah <script>alert("XSS!")</script> blah');
            (0, vitest_1.expect)($apple.html()).not.toContain('<script>alert("XSS!")</script>');
        });
    });
    (0, vitest_1.describe)('.clone', () => {
        (0, vitest_1.it)('() : should return a copy', () => {
            const $src = $('<div><span>foo</span><span>bar</span><span>baz</span></div>').children();
            const $elem = $src.clone();
            (0, vitest_1.expect)($elem.length).toBe(3);
            (0, vitest_1.expect)($elem.parent()).toHaveLength(0);
            (0, vitest_1.expect)($elem.text()).toBe($src.text());
            $src.text('rofl');
            (0, vitest_1.expect)($elem.text()).not.toBe($src.text());
        });
        (0, vitest_1.it)('() : should return a copy of document', () => {
            const $src = (0, index_js_1.load)('<html><body><div>foo</div>bar</body></html>')
                .root()
                .children();
            const $elem = $src.clone();
            (0, vitest_1.expect)($elem.length).toBe(1);
            (0, vitest_1.expect)($elem.parent()).toHaveLength(0);
            (0, vitest_1.expect)($elem.text()).toBe($src.text());
            $src.text('rofl');
            (0, vitest_1.expect)($elem.text()).not.toBe($src.text());
        });
        (0, vitest_1.it)('() : should preserve parsing options', () => {
            const $ = (0, index_js_1.load)('<div>π</div>', { xml: { decodeEntities: false } });
            const $div = $('div');
            (0, vitest_1.expect)($div.text()).toBe($div.clone().text());
        });
    });
});
