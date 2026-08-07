"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const domhandler_1 = require("domhandler");
const vitest_1 = require("vitest");
const fixtures_js_1 = require("../__fixtures__/fixtures.js");
const cheerio_js_1 = require("../cheerio.js");
const index_js_1 = require("../index.js");
function getText(el) {
    if (el.length === 0)
        return;
    const [firstChild] = el[0].childNodes;
    return (0, domhandler_1.isText)(firstChild) ? firstChild.data : undefined;
}
(0, vitest_1.describe)('$(...)', () => {
    let $;
    (0, vitest_1.beforeEach)(() => {
        $ = (0, index_js_1.load)(fixtures_js_1.fruits);
    });
    (0, vitest_1.describe)('.load', () => {
        (0, vitest_1.it)('should throw a TypeError if given invalid input', () => {
            (0, vitest_1.expect)(() => {
                // @ts-expect-error Testing invalid input
                (0, index_js_1.load)();
            }).toThrow('cheerio.load() expects a string');
        });
    });
    (0, vitest_1.describe)('.find', () => {
        (0, vitest_1.it)('() : should find nothing', () => {
            (0, vitest_1.expect)($('ul').find()).toHaveLength(0);
        });
        (0, vitest_1.it)('(single) : should find one descendant', () => {
            (0, vitest_1.expect)($('#fruits').find('.apple')[0].attribs).toHaveProperty('class', 'apple');
        });
        // #1679 - text tags not filtered
        (0, vitest_1.it)('(single) : should filter out text nodes', () => {
            const $root = $(`<html>\n${fixtures_js_1.fruits.replace(/></g, '>\n<')}\n</html>`);
            (0, vitest_1.expect)($root.find('.apple')[0].attribs).toHaveProperty('class', 'apple');
        });
        (0, vitest_1.it)('(many) : should find all matching descendant', () => {
            (0, vitest_1.expect)($('#fruits').find('li')).toHaveLength(3);
        });
        (0, vitest_1.it)('(many) : should merge all selected elems with matching descendants', () => {
            (0, vitest_1.expect)($('#fruits, #food', fixtures_js_1.food).find('.apple')).toHaveLength(1);
        });
        (0, vitest_1.it)('(invalid single) : should return empty if cant find', () => {
            (0, vitest_1.expect)($('ul').find('blah')).toHaveLength(0);
        });
        (0, vitest_1.it)('(invalid single) : should query descendants only', () => {
            (0, vitest_1.expect)($('#fruits').find('ul')).toHaveLength(0);
        });
        (0, vitest_1.it)('should return empty if search already empty result', () => {
            (0, vitest_1.expect)($('#not-fruits').find('li')).toHaveLength(0);
        });
        (0, vitest_1.it)('should lowercase selectors', () => {
            (0, vitest_1.expect)($('#fruits').find('LI')).toHaveLength(3);
        });
        (0, vitest_1.it)('should query immediate descendant only', () => {
            const q = (0, index_js_1.load)('<foo><bar><bar></bar><bar></bar></bar></foo>');
            (0, vitest_1.expect)(q('foo').find('> bar')).toHaveLength(1);
        });
        (0, vitest_1.it)('should find siblings', () => {
            const q = (0, index_js_1.load)('<p class=a><p class=b></p>');
            (0, vitest_1.expect)(q('.a').find('+.b')).toHaveLength(1);
            (0, vitest_1.expect)(q('.a').find('~.b')).toHaveLength(1);
            (0, vitest_1.expect)(q('.a').find('+.a')).toHaveLength(0);
            (0, vitest_1.expect)(q('.a').find('~.a')).toHaveLength(0);
        });
        (0, vitest_1.it)('should find self', () => {
            const q = (0, index_js_1.load)('<p class=a></p>');
            (0, vitest_1.expect)(q('.a').find(':scope')).toHaveLength(1);
        });
        (0, vitest_1.it)('should query case-sensitively when in xml mode', () => {
            const q = (0, index_js_1.load)('<caseSenSitive allTheWay>', { xml: true });
            (0, vitest_1.expect)(q('caseSenSitive')).toHaveLength(1);
            (0, vitest_1.expect)(q('[allTheWay]')).toHaveLength(1);
            (0, vitest_1.expect)(q('casesensitive')).toHaveLength(0);
            (0, vitest_1.expect)(q('[alltheway]')).toHaveLength(0);
        });
        (0, vitest_1.it)('should throw an Error if given an invalid selector', () => {
            (0, vitest_1.expect)(() => {
                $('#fruits').find(':bah');
            }).toThrow('Unknown pseudo-class :bah');
        });
        (0, vitest_1.it)('should respect the `lowerCaseTags` option (#3495)', () => {
            const q = (0, index_js_1.load)(`<parentTag class="myClass">
          <firstTag> <child> blah </child> </firstTag>
          <secondTag> <child> blah </child> </secondTag>
        </parentTag> `, {
                xml: {
                    xmlMode: true,
                    decodeEntities: false,
                    lowerCaseTags: true,
                    lowerCaseAttributeNames: false,
                    recognizeSelfClosing: true,
                },
            });
            (0, vitest_1.expect)(q('.myClass').find('firstTag > child')).toHaveLength(1);
        });
        (0, vitest_1.describe)('(cheerio object) :', () => {
            (0, vitest_1.it)('returns only those nodes contained within the current selection', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('#fruits').find(q('li'));
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe(q('.apple')[0]);
                (0, vitest_1.expect)($selection[1]).toBe(q('.orange')[0]);
                (0, vitest_1.expect)($selection[2]).toBe(q('.pear')[0]);
            });
            (0, vitest_1.it)('returns only those nodes contained within any element in the current selection', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('.apple, #vegetables').find(q('li'));
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe(q('.carrot')[0]);
                (0, vitest_1.expect)($selection[1]).toBe(q('.sweetcorn')[0]);
            });
        });
        (0, vitest_1.describe)('(node) :', () => {
            (0, vitest_1.it)('returns node when contained within the current selection', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('#fruits').find(q('.apple')[0]);
                (0, vitest_1.expect)($selection).toHaveLength(1);
                (0, vitest_1.expect)($selection[0]).toBe(q('.apple')[0]);
            });
            (0, vitest_1.it)('returns node when contained within any element the current selection', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('#fruits, #vegetables').find(q('.carrot')[0]);
                (0, vitest_1.expect)($selection).toHaveLength(1);
                (0, vitest_1.expect)($selection[0]).toBe(q('.carrot')[0]);
            });
            (0, vitest_1.it)('does not return node that is not contained within the current selection', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('#fruits').find(q('.carrot')[0]);
                (0, vitest_1.expect)($selection).toHaveLength(0);
            });
        });
    });
    (0, vitest_1.describe)('.children', () => {
        (0, vitest_1.it)('() : should get all children', () => {
            (0, vitest_1.expect)($('ul').children()).toHaveLength(3);
        });
        (0, vitest_1.it)('() : should skip text nodes', () => {
            (0, vitest_1.expect)($(fixtures_js_1.mixedText).children()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return children of all matched elements', () => {
            (0, vitest_1.expect)($('ul ul', fixtures_js_1.food).children()).toHaveLength(5);
        });
        (0, vitest_1.it)('(selector) : should return children matching selector', () => {
            const { attribs } = $('ul').children('.orange')[0];
            (0, vitest_1.expect)(attribs).toHaveProperty('class', 'orange');
        });
        (0, vitest_1.it)('(invalid selector) : should return empty', () => {
            (0, vitest_1.expect)($('ul').children('.lulz')).toHaveLength(0);
        });
        (0, vitest_1.it)('should only match immediate children, not ancestors', () => {
            (0, vitest_1.expect)($(fixtures_js_1.food).children('li')).toHaveLength(0);
        });
    });
    (0, vitest_1.describe)('.contents', () => {
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.text);
        });
        (0, vitest_1.it)('() : should get all contents', () => {
            (0, vitest_1.expect)($('p').contents()).toHaveLength(5);
        });
        (0, vitest_1.it)('() : should skip text nodes', () => {
            (0, vitest_1.expect)($(fixtures_js_1.mixedText).contents()).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should include text nodes', () => {
            (0, vitest_1.expect)($('p').contents().first()[0].type).toBe('text');
        });
        (0, vitest_1.it)('() : should include comment nodes', () => {
            (0, vitest_1.expect)($('p').contents().last()[0].type).toBe('comment');
        });
    });
    (0, vitest_1.describe)('.next', () => {
        (0, vitest_1.it)('() : should return next element', () => {
            const { attribs } = $('.orange').next()[0];
            (0, vitest_1.expect)(attribs).toHaveProperty('class', 'pear');
        });
        (0, vitest_1.it)('() : should skip text nodes', () => {
            (0, vitest_1.expect)($(fixtures_js_1.mixedText).next()[0]).toHaveProperty('name', 'b');
        });
        (0, vitest_1.it)('(no next) : should return empty for last child', () => {
            (0, vitest_1.expect)($('.pear').next()).toHaveLength(0);
        });
        (0, vitest_1.it)('(next on empty object) : should return empty', () => {
            (0, vitest_1.expect)($('.banana').next()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            (0, vitest_1.expect)($('.apple, .orange', fixtures_js_1.food).next()).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should return elements in order', () => {
            const result = (0, index_js_1.load)(fixtures_js_1.eleven)('.red').next();
            (0, vitest_1.expect)(result).toHaveLength(2);
            (0, vitest_1.expect)(result.eq(0).text()).toBe('Six');
            (0, vitest_1.expect)(result.eq(1).text()).toBe('Ten');
        });
        (0, vitest_1.it)('should reject elements that violate the filter', () => {
            (0, vitest_1.expect)($('.apple').next('.non-existent')).toHaveLength(0);
        });
        (0, vitest_1.it)('should accept elements that satisfy the filter', () => {
            (0, vitest_1.expect)($('.apple').next('.orange')).toHaveLength(1);
        });
        (0, vitest_1.describe)('(selector) :', () => {
            (0, vitest_1.it)('should reject elements that violate the filter', () => {
                (0, vitest_1.expect)($('.apple').next('.non-existent')).toHaveLength(0);
            });
            (0, vitest_1.it)('should accept elements that satisfy the filter', () => {
                (0, vitest_1.expect)($('.apple').next('.orange')).toHaveLength(1);
            });
        });
    });
    (0, vitest_1.describe)('.nextAll', () => {
        (0, vitest_1.it)('() : should return all following siblings', () => {
            const elems = $('.apple').nextAll();
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('class', 'pear');
        });
        (0, vitest_1.it)('(no next) : should return empty for last child', () => {
            (0, vitest_1.expect)($('.pear').nextAll()).toHaveLength(0);
        });
        (0, vitest_1.it)('(nextAll on empty object) : should return empty', () => {
            (0, vitest_1.expect)($('.banana').nextAll()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            (0, vitest_1.expect)($('.apple, .carrot', fixtures_js_1.food).nextAll()).toHaveLength(3);
        });
        (0, vitest_1.it)('() : should not contain duplicate elements', () => {
            const elems = $('.apple, .orange', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.nextAll()).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should not contain text elements', () => {
            const elems = $('.apple', fixtures_js_1.fruits.replace(/></g, '>\n<'));
            (0, vitest_1.expect)(elems.nextAll()).toHaveLength(2);
        });
        (0, vitest_1.describe)('(selector) :', () => {
            (0, vitest_1.it)('should filter according to the provided selector', () => {
                (0, vitest_1.expect)($('.apple').nextAll('.pear')).toHaveLength(1);
            });
            (0, vitest_1.it)("should not consider siblings' contents when filtering", () => {
                (0, vitest_1.expect)($('#fruits', fixtures_js_1.food).nextAll('li')).toHaveLength(0);
            });
        });
    });
    (0, vitest_1.describe)('.nextUntil', () => {
        (0, vitest_1.it)('() : should return all following siblings if no selector specified', () => {
            const elems = $('.apple', fixtures_js_1.food).nextUntil();
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('class', 'pear');
        });
        (0, vitest_1.it)('() : should filter out non-element nodes', () => {
            const elems = $('<div><div></div><!-- comment -->text<div></div></div>');
            const div = elems.children().eq(0);
            (0, vitest_1.expect)(div.nextUntil()).toHaveLength(1);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            const elems = $('.apple, .carrot', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.nextUntil()).toHaveLength(3);
        });
        (0, vitest_1.it)('() : should not contain duplicate elements', () => {
            const elems = $('.apple, .orange', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.nextUntil()).toHaveLength(2);
        });
        (0, vitest_1.it)('(selector) : should return all following siblings until selector', () => {
            const elems = $('.apple', fixtures_js_1.food).nextUntil('.pear');
            (0, vitest_1.expect)(elems).toHaveLength(1);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
        });
        (0, vitest_1.it)('(selector) : should support selector matching multiple elements', () => {
            const elems = $('#disabled', fixtures_js_1.forms).nextUntil('option, #unnamed');
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('id', 'submit');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('id', 'select');
        });
        (0, vitest_1.it)('(selector not sibling) : should return all following siblings', () => {
            const elems = $('.apple').nextUntil('#vegetables');
            (0, vitest_1.expect)(elems).toHaveLength(2);
        });
        (0, vitest_1.it)('(selector, filterString) : should return all following siblings until selector, filtered by filter', () => {
            const elems = $('.beer', fixtures_js_1.drinks).nextUntil('.water', '.milk');
            (0, vitest_1.expect)(elems).toHaveLength(1);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'milk');
        });
        (0, vitest_1.it)('(null, filterString) : should return all following siblings until selector, filtered by filter', () => {
            const elems = $('<ul><li></li><li><p></p></li></ul>');
            const empty = elems.find('li').eq(0).nextUntil(null, 'p');
            (0, vitest_1.expect)(empty).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty object for last child', () => {
            (0, vitest_1.expect)($('.pear').nextUntil()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty object when called on an empty object', () => {
            (0, vitest_1.expect)($('.banana').nextUntil()).toHaveLength(0);
        });
        (0, vitest_1.it)('(node) : should return all following siblings until the node', () => {
            const $fruits = $('#fruits').children();
            const elems = $fruits.eq(0).nextUntil($fruits[2]);
            (0, vitest_1.expect)(elems).toHaveLength(1);
        });
        (0, vitest_1.it)('(cheerio object) : should return all following siblings until any member of the cheerio object', () => {
            const $drinks = $(fixtures_js_1.drinks).children();
            const $until = $([$drinks[4], $drinks[3]]);
            const elems = $drinks.eq(0).nextUntil($until);
            (0, vitest_1.expect)(elems).toHaveLength(2);
        });
    });
    (0, vitest_1.describe)('.prev', () => {
        (0, vitest_1.it)('() : should return previous element', () => {
            const { attribs } = $('.orange').prev()[0];
            (0, vitest_1.expect)(attribs).toHaveProperty('class', 'apple');
        });
        (0, vitest_1.it)('() : should skip text nodes', () => {
            (0, vitest_1.expect)($($(fixtures_js_1.mixedText)[2]).prev()[0]).toHaveProperty('name', 'a');
        });
        (0, vitest_1.it)('(no prev) : should return empty for first child', () => {
            (0, vitest_1.expect)($('.apple').prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('(prev on empty object) : should return empty', () => {
            (0, vitest_1.expect)($('.banana').prev()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            (0, vitest_1.expect)($('.orange, .pear', fixtures_js_1.food).prev()).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should maintain elements order', () => {
            const sel = (0, index_js_1.load)(fixtures_js_1.eleven)('.sel');
            (0, vitest_1.expect)(sel).toHaveLength(3);
            (0, vitest_1.expect)(sel.eq(0).text()).toBe('Three');
            (0, vitest_1.expect)(sel.eq(1).text()).toBe('Nine');
            (0, vitest_1.expect)(sel.eq(2).text()).toBe('Eleven');
            // Swap last elements
            const el = sel[2];
            sel[2] = sel[1];
            sel[1] = el;
            const result = sel.prev();
            (0, vitest_1.expect)(result).toHaveLength(3);
            (0, vitest_1.expect)(result.eq(0).text()).toBe('Two');
            (0, vitest_1.expect)(result.eq(1).text()).toBe('Ten');
            (0, vitest_1.expect)(result.eq(2).text()).toBe('Eight');
        });
        (0, vitest_1.describe)('(selector) :', () => {
            (0, vitest_1.it)('should reject elements that violate the filter', () => {
                (0, vitest_1.expect)($('.orange').prev('.non-existent')).toHaveLength(0);
            });
            (0, vitest_1.it)('should accept elements that satisfy the filter', () => {
                (0, vitest_1.expect)($('.orange').prev('.apple')).toHaveLength(1);
            });
            (0, vitest_1.it)('(selector) : should reject elements that violate the filter', () => {
                (0, vitest_1.expect)($('.orange').prev('.non-existent')).toHaveLength(0);
            });
            (0, vitest_1.it)('(selector) : should accept elements that satisfy the filter', () => {
                (0, vitest_1.expect)($('.orange').prev('.apple')).toHaveLength(1);
            });
        });
    });
    (0, vitest_1.describe)('.prevAll', () => {
        (0, vitest_1.it)('() : should return all preceding siblings', () => {
            const elems = $('.pear').prevAll();
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('class', 'apple');
        });
        (0, vitest_1.it)('() : should not contain text elements', () => {
            const elems = $('.pear', fixtures_js_1.fruits.replace(/></g, '>\n<'));
            (0, vitest_1.expect)(elems.prevAll()).toHaveLength(2);
        });
        (0, vitest_1.it)('(no prev) : should return empty for first child', () => {
            (0, vitest_1.expect)($('.apple').prevAll()).toHaveLength(0);
        });
        (0, vitest_1.it)('(prevAll on empty object) : should return empty', () => {
            (0, vitest_1.expect)($('.banana').prevAll()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            (0, vitest_1.expect)($('.orange, .sweetcorn', fixtures_js_1.food).prevAll()).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should not contain duplicate elements', () => {
            const elems = $('.orange, .pear', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.prevAll()).toHaveLength(2);
        });
        (0, vitest_1.describe)('(selector) :', () => {
            (0, vitest_1.it)('should filter returned elements', () => {
                const elems = $('.pear').prevAll('.apple');
                (0, vitest_1.expect)(elems).toHaveLength(1);
            });
            (0, vitest_1.it)("should not consider siblings's descendents", () => {
                const elems = $('#vegetables', fixtures_js_1.food).prevAll('li');
                (0, vitest_1.expect)(elems).toHaveLength(0);
            });
        });
    });
    (0, vitest_1.describe)('.prevUntil', () => {
        (0, vitest_1.it)('() : should return all preceding siblings if no selector specified', () => {
            const elems = $('.pear').prevUntil();
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('class', 'apple');
        });
        (0, vitest_1.it)('() : should filter out non-element nodes', () => {
            const elems = $('<div class="1"><div class="2"></div><!-- comment -->text<div class="3"></div></div>');
            const div = elems.children().last();
            (0, vitest_1.expect)(div.prevUntil()).toHaveLength(1);
        });
        (0, vitest_1.it)('() : should operate over all elements in the selection', () => {
            const elems = $('.pear, .sweetcorn', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.prevUntil()).toHaveLength(3);
        });
        (0, vitest_1.it)('() : should not contain duplicate elements', () => {
            const elems = $('.orange, .pear', fixtures_js_1.food);
            (0, vitest_1.expect)(elems.prevUntil()).toHaveLength(2);
        });
        (0, vitest_1.it)('(selector) : should return all preceding siblings until selector', () => {
            const elems = $('.pear').prevUntil('.apple');
            (0, vitest_1.expect)(elems).toHaveLength(1);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'orange');
        });
        (0, vitest_1.it)('(selector) : should support selector matching multiple elements', () => {
            const elems = $('#unnamed', fixtures_js_1.forms).prevUntil('option, #disabled');
            (0, vitest_1.expect)(elems).toHaveLength(2);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('id', 'select');
            (0, vitest_1.expect)(elems[1].attribs).toHaveProperty('id', 'submit');
        });
        (0, vitest_1.it)('(selector not sibling) : should return all preceding siblings', () => {
            const elems = $('.sweetcorn', fixtures_js_1.food).prevUntil('#fruits');
            (0, vitest_1.expect)(elems).toHaveLength(1);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'carrot');
        });
        (0, vitest_1.it)('(selector, filterString) : should return all preceding siblings until selector, filtered by filter', () => {
            const elems = $('.cider', fixtures_js_1.drinks).prevUntil('.juice', '.water');
            (0, vitest_1.expect)(elems).toHaveLength(1);
            (0, vitest_1.expect)(elems[0].attribs).toHaveProperty('class', 'water');
        });
        (0, vitest_1.it)('(selector, filterString) : should return all preceding siblings until selector', () => {
            const elems = $('<ul><li><p></p></li><li></li></ul>');
            const empty = elems.find('li').eq(1).prevUntil(null, 'p');
            (0, vitest_1.expect)(empty).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty object for first child', () => {
            (0, vitest_1.expect)($('.apple').prevUntil()).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty object when called on an empty object', () => {
            (0, vitest_1.expect)($('.banana').prevUntil()).toHaveLength(0);
        });
        (0, vitest_1.it)('(node) : should return all previous siblings until the node', () => {
            const $fruits = $('#fruits').children();
            const elems = $fruits.eq(2).prevUntil($fruits[0]);
            (0, vitest_1.expect)(elems).toHaveLength(1);
        });
        (0, vitest_1.it)('(cheerio object) : should return all previous siblings until any member of the cheerio object', () => {
            const $drinks = $(fixtures_js_1.drinks).children();
            const $until = $([$drinks[0], $drinks[1]]);
            const elems = $drinks.eq(4).prevUntil($until);
            (0, vitest_1.expect)(elems).toHaveLength(2);
        });
    });
    (0, vitest_1.describe)('.siblings', () => {
        (0, vitest_1.it)('() : should get all the siblings', () => {
            (0, vitest_1.expect)($('.orange').siblings()).toHaveLength(2);
            (0, vitest_1.expect)($('#fruits').siblings()).toHaveLength(0);
            (0, vitest_1.expect)($('.apple, .carrot', fixtures_js_1.food).siblings()).toHaveLength(3);
        });
        (0, vitest_1.it)('(selector) : should get all siblings that match the selector', () => {
            (0, vitest_1.expect)($('.orange').siblings('.apple')).toHaveLength(1);
            (0, vitest_1.expect)($('.orange').siblings('.peach')).toHaveLength(0);
        });
        (0, vitest_1.it)('(selector) : should throw an Error if given an invalid selector', () => {
            (0, vitest_1.expect)(() => {
                $('.orange').siblings(':bah');
            }).toThrow('Unknown pseudo-class :bah');
        });
        (0, vitest_1.it)('(selector) : does not consider the contents of siblings when filtering (GH-374)', () => {
            (0, vitest_1.expect)($('#fruits', fixtures_js_1.food).siblings('li')).toHaveLength(0);
        });
        (0, vitest_1.it)('() : when two elements are siblings to each other they have to be included', () => {
            const result = (0, index_js_1.load)(fixtures_js_1.eleven)('.sel').siblings();
            (0, vitest_1.expect)(result).toHaveLength(7);
            (0, vitest_1.expect)(result.eq(0).text()).toBe('One');
            (0, vitest_1.expect)(result.eq(1).text()).toBe('Two');
            (0, vitest_1.expect)(result.eq(2).text()).toBe('Four');
            (0, vitest_1.expect)(result.eq(3).text()).toBe('Eight');
            (0, vitest_1.expect)(result.eq(4).text()).toBe('Nine');
            (0, vitest_1.expect)(result.eq(5).text()).toBe('Ten');
            (0, vitest_1.expect)(result.eq(6).text()).toBe('Eleven');
        });
        (0, vitest_1.it)('(selector) : when two elements are siblings to each other they have to be included', () => {
            const result = (0, index_js_1.load)(fixtures_js_1.eleven)('.sel').siblings('.red');
            (0, vitest_1.expect)(result).toHaveLength(2);
            (0, vitest_1.expect)(result.eq(0).text()).toBe('Four');
            (0, vitest_1.expect)(result.eq(1).text()).toBe('Nine');
        });
        (0, vitest_1.it)('(cheerio) : test filtering with cheerio object', () => {
            const doc = (0, index_js_1.load)(fixtures_js_1.eleven);
            const result = doc('.sel').siblings(doc(':not([class])'));
            (0, vitest_1.expect)(result).toHaveLength(4);
            (0, vitest_1.expect)(result.eq(0).text()).toBe('One');
            (0, vitest_1.expect)(result.eq(1).text()).toBe('Two');
            (0, vitest_1.expect)(result.eq(2).text()).toBe('Eight');
            (0, vitest_1.expect)(result.eq(3).text()).toBe('Ten');
        });
    });
    (0, vitest_1.describe)('.parents', () => {
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.food);
        });
        (0, vitest_1.it)('() : should get all of the parents in logical order', () => {
            const orange = $('.orange').parents();
            (0, vitest_1.expect)(orange).toHaveLength(4);
            (0, vitest_1.expect)(orange[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)(orange[1].attribs).toHaveProperty('id', 'food');
            (0, vitest_1.expect)(orange[2].tagName).toBe('body');
            (0, vitest_1.expect)(orange[3].tagName).toBe('html');
            const fruits = $('#fruits').parents();
            (0, vitest_1.expect)(fruits).toHaveLength(3);
            (0, vitest_1.expect)(fruits[0].attribs).toHaveProperty('id', 'food');
            (0, vitest_1.expect)(fruits[1].tagName).toBe('body');
            (0, vitest_1.expect)(fruits[2].tagName).toBe('html');
        });
        (0, vitest_1.it)('(selector) : should get all of the parents that match the selector in logical order', () => {
            const fruits = $('.orange').parents('#fruits');
            (0, vitest_1.expect)(fruits).toHaveLength(1);
            (0, vitest_1.expect)(fruits[0].attribs).toHaveProperty('id', 'fruits');
            const uls = $('.orange').parents('ul');
            (0, vitest_1.expect)(uls).toHaveLength(2);
            (0, vitest_1.expect)(uls[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)(uls[1].attribs).toHaveProperty('id', 'food');
        });
        (0, vitest_1.it)('() : should not break if the selector does not have any results', () => {
            const result = $('.saladbar').parents();
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty set for top-level elements', () => {
            const result = $('html').parents();
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return the parents of every element in the *reversed* collection, omitting duplicates', () => {
            const $parents = $('li').parents();
            (0, vitest_1.expect)($parents).toHaveLength(5);
            (0, vitest_1.expect)($parents[0]).toBe($('#vegetables')[0]);
            (0, vitest_1.expect)($parents[1]).toBe($('#fruits')[0]);
            (0, vitest_1.expect)($parents[2]).toBe($('#food')[0]);
            (0, vitest_1.expect)($parents[3]).toBe($('body')[0]);
            (0, vitest_1.expect)($parents[4]).toBe($('html')[0]);
        });
    });
    (0, vitest_1.describe)('.parentsUntil', () => {
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.food);
        });
        (0, vitest_1.it)('() : should get all of the parents in logical order', () => {
            const result = $('.orange').parentsUntil();
            (0, vitest_1.expect)(result).toHaveLength(4);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)(result[1].attribs).toHaveProperty('id', 'food');
            (0, vitest_1.expect)(result[2].tagName).toBe('body');
            (0, vitest_1.expect)(result[3].tagName).toBe('html');
        });
        (0, vitest_1.it)('() : should get all of the parents in reversed order, omitting duplicates', () => {
            const result = $('.apple, .sweetcorn').parentsUntil();
            (0, vitest_1.expect)(result).toHaveLength(5);
            (0, vitest_1.expect)(result[0]).toBe($('#vegetables')[0]);
            (0, vitest_1.expect)(result[1]).toBe($('#fruits')[0]);
            (0, vitest_1.expect)(result[2]).toBe($('#food')[0]);
            (0, vitest_1.expect)(result[3]).toBe($('body')[0]);
            (0, vitest_1.expect)(result[4]).toBe($('html')[0]);
        });
        (0, vitest_1.it)('(selector) : should get all of the parents until selector', () => {
            const food = $('.orange').parentsUntil('#food');
            (0, vitest_1.expect)(food).toHaveLength(1);
            (0, vitest_1.expect)(food[0].attribs).toHaveProperty('id', 'fruits');
            const fruits = $('.orange').parentsUntil('#fruits');
            (0, vitest_1.expect)(fruits).toHaveLength(0);
        });
        (0, vitest_1.it)('(selector) : Less simple parentsUntil check with selector', () => {
            const result = $('#fruits').parentsUntil('html, body');
            (0, vitest_1.expect)(result.eq(0).attr('id')).toBe('food');
        });
        (0, vitest_1.it)('(selector not parent) : should return all parents', () => {
            const result = $('.orange').parentsUntil('.apple');
            (0, vitest_1.expect)(result).toHaveLength(4);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)(result[1].attribs).toHaveProperty('id', 'food');
            (0, vitest_1.expect)(result[2].tagName).toBe('body');
            (0, vitest_1.expect)(result[3].tagName).toBe('html');
        });
        (0, vitest_1.it)('(selector, filter) : should get all of the parents that match the filter', () => {
            const result = $('.apple, .sweetcorn').parentsUntil('.saladbar', '#vegetables');
            (0, vitest_1.expect)(result).toHaveLength(1);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'vegetables');
        });
        (0, vitest_1.it)('(selector, filter) : Multiple-filtered parentsUntil check', () => {
            const result = $('.orange').parentsUntil('html', 'ul,body');
            (0, vitest_1.expect)(result).toHaveLength(3);
            (0, vitest_1.expect)(result.eq(0).attr('id')).toBe('fruits');
            (0, vitest_1.expect)(result.eq(1).attr('id')).toBe('food');
            (0, vitest_1.expect)(result.eq(2).prop('tagName')).toBe('BODY');
        });
        (0, vitest_1.it)('() : should return empty object when called on an empty object', () => {
            const result = $('.saladbar').parentsUntil();
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should return an empty set for top-level elements', () => {
            const result = $('html').parentsUntil();
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('(cheerio object) : should return all parents until any member of the cheerio object', () => {
            const $fruits = $('#fruits');
            const $until = $('#food');
            const result = $fruits.children().eq(1).parentsUntil($until);
            (0, vitest_1.expect)(result).toHaveLength(1);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'fruits');
        });
        (0, vitest_1.it)('(cheerio object) : should return all parents until body element', () => {
            const body = $('body')[0];
            const result = $('.carrot').parentsUntil(body);
            (0, vitest_1.expect)(result).toHaveLength(2);
            (0, vitest_1.expect)(result.eq(0).is('ul#vegetables')).toBe(true);
        });
    });
    (0, vitest_1.describe)('.parent', () => {
        (0, vitest_1.it)('() : should return the parent of each matched element', () => {
            let result = $('.orange').parent();
            (0, vitest_1.expect)(result).toHaveLength(1);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'fruits');
            result = $('li', fixtures_js_1.food).parent();
            (0, vitest_1.expect)(result).toHaveLength(2);
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)(result[1].attribs).toHaveProperty('id', 'vegetables');
        });
        (0, vitest_1.it)('(undefined) : should not throw an exception', () => {
            (0, vitest_1.expect)(() => {
                $('li').parent(undefined);
            }).not.toThrow();
        });
        (0, vitest_1.it)('() : should return an empty object for top-level elements', () => {
            const result = $('html').parent();
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('() : should not contain duplicate elements', () => {
            const result = $('li').parent();
            (0, vitest_1.expect)(result).toHaveLength(1);
        });
        (0, vitest_1.it)('(selector) : should filter the matched parent elements by the selector', () => {
            const parents = $('.orange').parent();
            (0, vitest_1.expect)(parents).toHaveLength(1);
            (0, vitest_1.expect)(parents[0].attribs).toHaveProperty('id', 'fruits');
            const fruits = $('li', fixtures_js_1.food).parent('#fruits');
            (0, vitest_1.expect)(fruits).toHaveLength(1);
            (0, vitest_1.expect)(fruits[0].attribs).toHaveProperty('id', 'fruits');
        });
    });
    (0, vitest_1.describe)('.closest', () => {
        (0, vitest_1.it)('() : should return an empty array', () => {
            const result = $('.orange').closest();
            (0, vitest_1.expect)(result).toHaveLength(0);
            (0, vitest_1.expect)(result).toBeInstanceOf(cheerio_js_1.Cheerio);
        });
        (0, vitest_1.it)('(selector) : should find the closest element that matches the selector, searching through its ancestors and itself', () => {
            (0, vitest_1.expect)($('.orange').closest('.apple')).toHaveLength(0);
            (0, vitest_1.expect)($('.orange', fixtures_js_1.food).closest('#food')[0].attribs).toHaveProperty('id', 'food');
            (0, vitest_1.expect)($('.orange', fixtures_js_1.food).closest('ul')[0].attribs).toHaveProperty('id', 'fruits');
            (0, vitest_1.expect)($('.orange', fixtures_js_1.food).closest('li')[0].attribs).toHaveProperty('class', 'orange');
        });
        (0, vitest_1.it)('(selector) : should find the closest element of each item, removing duplicates', () => {
            const result = $('li', fixtures_js_1.food).closest('ul');
            (0, vitest_1.expect)(result).toHaveLength(2);
        });
        (0, vitest_1.it)('() : should not break if the selector does not have any results', () => {
            const result = $('.saladbar', fixtures_js_1.food).closest('ul');
            (0, vitest_1.expect)(result).toHaveLength(0);
        });
        (0, vitest_1.it)('(selector) : should find closest element for text nodes', () => {
            const textNode = $('.apple', fixtures_js_1.food).contents().first();
            const result = textNode.closest('#food');
            (0, vitest_1.expect)(result[0].attribs).toHaveProperty('id', 'food');
        });
        (0, vitest_1.it)('(fn) : should dedupe in linear time (no O(n^2) membership scan)', () => {
            /*
             * N distinct matches: pre-fix `set.includes` scans a growing array,
             * giving sum(0..N-1) = O(N^2) comparisons. The fix upgrades to a Set.
             */
            const N = 2000;
            const $big = (0, index_js_1.load)(`<div>${'<div class="t"><span></span></div>'.repeat(N)}</div>`);
            const spans = $big('span');
            (0, vitest_1.expect)(spans).toHaveLength(N);
            const origIncludes = Array.prototype.includes;
            let includesWork = 0;
            Array.prototype.includes = function (...args) {
                includesWork += this.length;
                return origIncludes.apply(this, args);
            };
            let result;
            try {
                /*
                 * A predicate selector avoids css-select internals, so the only
                 * Array#includes in play is the dedup scan under test.
                 */
                result = spans.closest((_i, el) => el.name === 'div');
            }
            finally {
                Array.prototype.includes = origIncludes;
            }
            (0, vitest_1.expect)(result).toHaveLength(N);
            /*
             * Fixed: 5050, the scans that happen before the Set upgrade at 100
             * entries. Pre-fix: N*(N-1)/2, about 2M for N=2000.
             */
            (0, vitest_1.expect)(includesWork).toBeLessThan(N * 10);
        });
    });
    (0, vitest_1.describe)('.each', () => {
        (0, vitest_1.it)('( (i, elem) -> ) : should loop selected returning fn with (i, elem)', () => {
            const items = [];
            const classes = ['apple', 'orange', 'pear'];
            $('li').each(function (idx, elem) {
                items[idx] = elem;
                (0, vitest_1.expect)(this.attribs).toHaveProperty('class', classes[idx]);
            });
            (0, vitest_1.expect)(items[0].attribs).toHaveProperty('class', 'apple');
            (0, vitest_1.expect)(items[1].attribs).toHaveProperty('class', 'orange');
            (0, vitest_1.expect)(items[2].attribs).toHaveProperty('class', 'pear');
        });
        (0, vitest_1.it)('( (i, elem) -> ) : should break iteration when the iterator function returns false', () => {
            let iterationCount = 0;
            $('li').each((idx) => {
                iterationCount++;
                return idx < 1;
            });
            (0, vitest_1.expect)(iterationCount).toBe(2);
        });
    });
    if (typeof Symbol !== 'undefined') {
        (0, vitest_1.describe)('[Symbol.iterator]', () => {
            (0, vitest_1.it)('should yield each element', () => {
                // The equivalent of: for (const element of $('li')) ...
                const $li = $('li');
                const iterator = $li[Symbol.iterator]();
                (0, vitest_1.expect)(iterator.next().value.attribs).toHaveProperty('class', 'apple');
                (0, vitest_1.expect)(iterator.next().value.attribs).toHaveProperty('class', 'orange');
                (0, vitest_1.expect)(iterator.next().value.attribs).toHaveProperty('class', 'pear');
                (0, vitest_1.expect)(iterator.next().done).toBe(true);
            });
        });
    }
    (0, vitest_1.describe)('.map', () => {
        (0, vitest_1.it)('(fn) : should be invoked with the correct arguments and context', () => {
            const $fruits = $('li');
            const args = [];
            const thisVals = [];
            $fruits.map(function (...myArgs) {
                args.push(myArgs);
                thisVals.push(this);
                return null;
            });
            (0, vitest_1.expect)(args).toStrictEqual([
                [0, $fruits[0]],
                [1, $fruits[1]],
                [2, $fruits[2]],
            ]);
            (0, vitest_1.expect)(thisVals).toStrictEqual([$fruits[0], $fruits[1], $fruits[2]]);
        });
        (0, vitest_1.it)('(fn) : should return an Cheerio object wrapping the returned items', () => {
            const $fruits = $('li');
            const $mapped = $fruits.map((i) => $fruits[2 - i]);
            (0, vitest_1.expect)($mapped).toHaveLength(3);
            (0, vitest_1.expect)($mapped[0]).toBe($fruits[2]);
            (0, vitest_1.expect)($mapped[1]).toBe($fruits[1]);
            (0, vitest_1.expect)($mapped[2]).toBe($fruits[0]);
        });
        (0, vitest_1.it)('(fn) : should ignore `null` and `undefined` returned by iterator', () => {
            const $fruits = $('li');
            const retVals = [null, undefined, $fruits[1]];
            const $mapped = $fruits.map((i) => retVals[i]);
            (0, vitest_1.expect)($mapped).toHaveLength(1);
            (0, vitest_1.expect)($mapped[0]).toBe($fruits[1]);
        });
        (0, vitest_1.it)('(fn) : should perform a shallow merge on arrays returned by iterator', () => {
            const $fruits = $('li');
            const $mapped = $fruits.map(() => [1, [3, 4]]);
            (0, vitest_1.expect)($mapped.get()).toStrictEqual([1, [3, 4], 1, [3, 4], 1, [3, 4]]);
        });
        (0, vitest_1.it)('(fn) : should tolerate `null` and `undefined` when flattening arrays returned by iterator', () => {
            const $fruits = $('li');
            const $mapped = $fruits.map(() => [null, undefined]);
            (0, vitest_1.expect)($mapped.get()).toStrictEqual([
                null,
                undefined,
                null,
                undefined,
                null,
                undefined,
            ]);
        });
        (0, vitest_1.it)('(fn) : should accumulate in linear time (no O(n^2) concat copies)', () => {
            /*
             * Pre-fix `elems = elems.concat(val)` copies the whole accumulator each
             * iteration, giving sum(0..N-1) = O(N^2) work. The fix uses `push`.
             */
            const N = 4000;
            const $big = (0, index_js_1.load)('<div></div>'.repeat(N));
            const sel = $big('div');
            (0, vitest_1.expect)(sel).toHaveLength(N);
            const origConcat = Array.prototype.concat;
            let concatWork = 0;
            Array.prototype.concat = function (...args) {
                concatWork += this.length;
                return origConcat.apply(this, args);
            };
            let mapped;
            try {
                mapped = sel.map((_i, el) => el);
            }
            finally {
                Array.prototype.concat = origConcat;
            }
            (0, vitest_1.expect)(mapped).toHaveLength(N);
            // Fixed: 0, as `push` replaces `concat`. Pre-fix: about 8M for N=4000.
            (0, vitest_1.expect)(concatWork).toBeLessThan(N * 10);
        });
    });
    (0, vitest_1.describe)('.filter', () => {
        (0, vitest_1.it)('(selector) : should reduce the set of matched elements to those that match the selector', () => {
            const pear = $('li').filter('.pear').text();
            (0, vitest_1.expect)(pear).toBe('Pear');
        });
        (0, vitest_1.it)('(selector) : should not consider nested elements', () => {
            const lis = $('#fruits').filter('li');
            (0, vitest_1.expect)(lis).toHaveLength(0);
        });
        (0, vitest_1.it)('(selection) : should reduce the set of matched elements to those that are contained in the provided selection', () => {
            const $fruits = $('li');
            const $pear = $fruits.filter('.pear, .apple');
            (0, vitest_1.expect)($fruits.filter($pear)).toHaveLength(2);
        });
        (0, vitest_1.it)('(element) : should reduce the set of matched elements to those that specified directly', () => {
            const $fruits = $('li');
            const pear = $fruits.filter('.pear')[0];
            (0, vitest_1.expect)($fruits.filter(pear)).toHaveLength(1);
        });
        (0, vitest_1.it)("(fn) : should reduce the set of matched elements to those that pass the function's test", () => {
            const orange = $('li')
                .filter(function (i, el) {
                (0, vitest_1.expect)(this).toBe(el);
                (0, vitest_1.expect)(el.tagName).toBe('li');
                (0, vitest_1.expect)(typeof i).toBe('number');
                return $(this).attr('class') === 'orange';
            })
                .text();
            (0, vitest_1.expect)(orange).toBe('Orange');
        });
        (0, vitest_1.it)('should also iterate over text nodes (#1867)', () => {
            const text = $('<a>a</a>b<c></c>').filter((_, el) => (0, domhandler_1.isText)(el));
            (0, vitest_1.expect)(text[0].data).toBe('b');
        });
    });
    (0, vitest_1.describe)('.not', () => {
        (0, vitest_1.it)('(selector) : should reduce the set of matched elements to those that do not match the selector', () => {
            const $fruits = $('li');
            const $notPear = $fruits.not('.pear');
            (0, vitest_1.expect)($notPear).toHaveLength(2);
            (0, vitest_1.expect)($notPear[0]).toBe($fruits[0]);
            (0, vitest_1.expect)($notPear[1]).toBe($fruits[1]);
        });
        (0, vitest_1.it)('(selector) : should not consider nested elements', () => {
            const lis = $('#fruits').not('li');
            (0, vitest_1.expect)(lis).toHaveLength(1);
        });
        (0, vitest_1.it)('(selection) : should reduce the set of matched elements to those that are not contained in the provided selection', () => {
            const $fruits = $('li');
            const $orange = $('.orange');
            const $notOrange = $fruits.not($orange);
            (0, vitest_1.expect)($notOrange).toHaveLength(2);
            (0, vitest_1.expect)($notOrange[0]).toBe($fruits[0]);
            (0, vitest_1.expect)($notOrange[1]).toBe($fruits[2]);
        });
        (0, vitest_1.it)('(element) : should reduce the set of matched elements to those that specified directly', () => {
            const $fruits = $('li');
            const apple = $('.apple')[0];
            const $notApple = $fruits.not(apple);
            (0, vitest_1.expect)($notApple).toHaveLength(2);
            (0, vitest_1.expect)($notApple[0]).toBe($fruits[1]);
            (0, vitest_1.expect)($notApple[1]).toBe($fruits[2]);
        });
        (0, vitest_1.it)("(fn) : should reduce the set of matched elements to those that do not pass the function's test", () => {
            const $fruits = $('li');
            const $notOrange = $fruits.not(function (i, el) {
                (0, vitest_1.expect)(this).toBe(el);
                (0, vitest_1.expect)(el).toHaveProperty('name', 'li');
                (0, vitest_1.expect)(typeof i).toBe('number');
                return $(this).attr('class') === 'orange';
            });
            (0, vitest_1.expect)($notOrange).toHaveLength(2);
            (0, vitest_1.expect)($notOrange[0]).toBe($fruits[0]);
            (0, vitest_1.expect)($notOrange[1]).toBe($fruits[2]);
        });
    });
    (0, vitest_1.describe)('.has', () => {
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.food);
        });
        (0, vitest_1.it)('(selector) : should reduce the set of matched elements to those with descendants that match the selector', () => {
            const $fruits = $('#fruits,#vegetables').has('.pear');
            (0, vitest_1.expect)($fruits).toHaveLength(1);
            (0, vitest_1.expect)($fruits[0]).toBe($('#fruits')[0]);
        });
        (0, vitest_1.it)('(selector) : should only consider nested elements', () => {
            const $empty = $('#fruits').has('#fruits');
            (0, vitest_1.expect)($empty).toHaveLength(0);
        });
        (0, vitest_1.it)('(element) : should reduce the set of matched elements to those that are ancestors of the provided element', () => {
            const $fruits = $('#fruits,#vegetables').has($('.pear')[0]);
            (0, vitest_1.expect)($fruits).toHaveLength(1);
            (0, vitest_1.expect)($fruits[0]).toBe($('#fruits')[0]);
        });
        (0, vitest_1.it)('(element) : should only consider nested elements', () => {
            const $fruits = $('#fruits');
            const fruitsEl = $fruits[0];
            const $empty = $fruits.has(fruitsEl);
            (0, vitest_1.expect)($empty).toHaveLength(0);
        });
    });
    (0, vitest_1.describe)('.first', () => {
        (0, vitest_1.it)('() : should return the first item', () => {
            const $src = $('<span>foo</span><span>bar</span><span>baz</span>');
            const $elem = $src.first();
            (0, vitest_1.expect)($elem.length).toBe(1);
            (0, vitest_1.expect)($elem[0].childNodes[0]).toHaveProperty('data', 'foo');
        });
        (0, vitest_1.it)('() : should return an empty object for an empty object', () => {
            const $src = $();
            const $first = $src.first();
            (0, vitest_1.expect)($first.length).toBe(0);
            (0, vitest_1.expect)($first[0]).toBeUndefined();
        });
    });
    (0, vitest_1.describe)('.last', () => {
        (0, vitest_1.it)('() : should return the last element', () => {
            const $src = $('<span>foo</span><span>bar</span><span>baz</span>');
            const $elem = $src.last();
            (0, vitest_1.expect)($elem.length).toBe(1);
            (0, vitest_1.expect)($elem[0].childNodes[0]).toHaveProperty('data', 'baz');
        });
        (0, vitest_1.it)('() : should return an empty object for an empty object', () => {
            const $src = $();
            const $last = $src.last();
            (0, vitest_1.expect)($last.length).toBe(0);
            (0, vitest_1.expect)($last[0]).toBeUndefined();
        });
    });
    (0, vitest_1.describe)('.first & .last', () => {
        (0, vitest_1.it)('() : should return equivalent collections if only one element', () => {
            const $src = $('<span>bar</span>');
            const $first = $src.first();
            const $last = $src.last();
            (0, vitest_1.expect)($first.length).toBe(1);
            (0, vitest_1.expect)($first[0].childNodes[0]).toHaveProperty('data', 'bar');
            (0, vitest_1.expect)($last.length).toBe(1);
            (0, vitest_1.expect)($last[0].childNodes[0]).toHaveProperty('data', 'bar');
            (0, vitest_1.expect)($first[0]).toBe($last[0]);
        });
    });
    (0, vitest_1.describe)('.eq', () => {
        (0, vitest_1.it)('(i) : should return the element at the specified index', () => {
            (0, vitest_1.expect)(getText($('li').eq(0))).toBe('Apple');
            (0, vitest_1.expect)(getText($('li').eq(1))).toBe('Orange');
            (0, vitest_1.expect)(getText($('li').eq(2))).toBe('Pear');
            (0, vitest_1.expect)(getText($('li').eq(3))).toBeUndefined();
            (0, vitest_1.expect)(getText($('li').eq(-1))).toBe('Pear');
        });
    });
    (0, vitest_1.describe)('.get', () => {
        (0, vitest_1.it)('(i) : should return the element at the specified index', () => {
            const children = $('#fruits').children();
            (0, vitest_1.expect)(children.get(0)).toBe(children[0]);
            (0, vitest_1.expect)(children.get(1)).toBe(children[1]);
            (0, vitest_1.expect)(children.get(2)).toBe(children[2]);
        });
        (0, vitest_1.it)('(-1) : should return the element indexed from the end of the collection', () => {
            const children = $('#fruits').children();
            (0, vitest_1.expect)(children.get(-1)).toBe(children[2]);
            (0, vitest_1.expect)(children.get(-2)).toBe(children[1]);
            (0, vitest_1.expect)(children.get(-3)).toBe(children[0]);
        });
        (0, vitest_1.it)('() : should return an array containing all of the collection', () => {
            const children = $('#fruits').children();
            const all = children.get();
            (0, vitest_1.expect)(Array.isArray(all)).toBe(true);
            (0, vitest_1.expect)(all).toStrictEqual([children[0], children[1], children[2]]);
        });
    });
    (0, vitest_1.describe)('.index', () => {
        (0, vitest_1.describe)('() :', () => {
            (0, vitest_1.it)('returns the index of a child amongst its siblings', () => {
                (0, vitest_1.expect)($('.orange').index()).toBe(1);
            });
            (0, vitest_1.it)('returns -1 when the selection has no parent', () => {
                (0, vitest_1.expect)($('<div/>').index()).toBe(-1);
            });
        });
        (0, vitest_1.describe)('(selector) :', () => {
            (0, vitest_1.it)('returns the index of the first element in the set matched by `selector`', () => {
                (0, vitest_1.expect)($('.apple').index('#fruits, li')).toBe(1);
            });
            (0, vitest_1.it)('returns -1 when the item is not present in the set matched by `selector`', () => {
                (0, vitest_1.expect)($('.apple').index('#fuits')).toBe(-1);
            });
            (0, vitest_1.it)('returns -1 when the first element in the set has no parent', () => {
                (0, vitest_1.expect)($('<div/>').index('*')).toBe(-1);
            });
        });
        (0, vitest_1.describe)('(node) :', () => {
            (0, vitest_1.it)('returns the index of the given node within the current selection', () => {
                const $lis = $('li');
                (0, vitest_1.expect)($lis.index($lis.get(1))).toBe(1);
            });
            (0, vitest_1.it)('returns the index of the given node within the current selection when the current selection has no parent', () => {
                const $apple = $('.apple').remove();
                (0, vitest_1.expect)($apple.index($apple.get(0))).toBe(0);
            });
            (0, vitest_1.it)('returns -1 when the given node is not present in the current selection', () => {
                (0, vitest_1.expect)($('li').index($('#fruits').get(0))).toBe(-1);
            });
            (0, vitest_1.it)('returns -1 when the current selection is empty', () => {
                (0, vitest_1.expect)($('.not-fruit').index($('#fruits').get(0))).toBe(-1);
            });
        });
        (0, vitest_1.describe)('(selection) :', () => {
            (0, vitest_1.it)('returns the index of the first node in the provided selection within the current selection', () => {
                const $lis = $('li');
                (0, vitest_1.expect)($lis.index($('.orange, .pear'))).toBe(1);
            });
            (0, vitest_1.it)('returns -1 when the given node is not present in the current selection', () => {
                (0, vitest_1.expect)($('li').index($('#fruits'))).toBe(-1);
            });
            (0, vitest_1.it)('returns -1 when the current selection is empty', () => {
                (0, vitest_1.expect)($('.not-fruit').index($('#fruits'))).toBe(-1);
            });
        });
    });
    (0, vitest_1.describe)('.slice', () => {
        (0, vitest_1.it)('(start) : should return all elements after the given index', () => {
            const sliced = $('li').slice(1);
            (0, vitest_1.expect)(sliced).toHaveLength(2);
            (0, vitest_1.expect)(getText(sliced.eq(0))).toBe('Orange');
            (0, vitest_1.expect)(getText(sliced.eq(1))).toBe('Pear');
        });
        (0, vitest_1.it)('(start, end) : should return all elements matching the given range', () => {
            const sliced = $('li').slice(1, 2);
            (0, vitest_1.expect)(sliced).toHaveLength(1);
            (0, vitest_1.expect)(getText(sliced.eq(0))).toBe('Orange');
        });
        (0, vitest_1.it)('(-start) : should return element matching the offset from the end', () => {
            const sliced = $('li').slice(-1);
            (0, vitest_1.expect)(sliced).toHaveLength(1);
            (0, vitest_1.expect)(getText(sliced.eq(0))).toBe('Pear');
        });
    });
    (0, vitest_1.describe)('.end() :', () => {
        let $fruits;
        (0, vitest_1.beforeEach)(() => {
            $fruits = $('#fruits').children();
        });
        (0, vitest_1.it)('returns an empty object at the end of the chain', () => {
            (0, vitest_1.expect)($fruits.end().end().end()).toBeTruthy();
            (0, vitest_1.expect)($fruits.end().end().end()).toHaveLength(0);
        });
        (0, vitest_1.it)('find', () => {
            (0, vitest_1.expect)($fruits.find('.apple').end()).toBe($fruits);
        });
        (0, vitest_1.it)('filter', () => {
            (0, vitest_1.expect)($fruits.filter('.apple').end()).toBe($fruits);
        });
        (0, vitest_1.it)('map', () => {
            (0, vitest_1.expect)($fruits
                .map(function () {
                return this;
            })
                .end()).toBe($fruits);
        });
        (0, vitest_1.it)('contents', () => {
            (0, vitest_1.expect)($fruits.contents().end()).toBe($fruits);
        });
        (0, vitest_1.it)('eq', () => {
            (0, vitest_1.expect)($fruits.eq(1).end()).toBe($fruits);
        });
        (0, vitest_1.it)('first', () => {
            (0, vitest_1.expect)($fruits.first().end()).toBe($fruits);
        });
        (0, vitest_1.it)('last', () => {
            (0, vitest_1.expect)($fruits.last().end()).toBe($fruits);
        });
        (0, vitest_1.it)('slice', () => {
            (0, vitest_1.expect)($fruits.slice(1).end()).toBe($fruits);
        });
        (0, vitest_1.it)('children', () => {
            (0, vitest_1.expect)($fruits.children().end()).toBe($fruits);
        });
        (0, vitest_1.it)('parent', () => {
            (0, vitest_1.expect)($fruits.parent().end()).toBe($fruits);
        });
        (0, vitest_1.it)('parents', () => {
            (0, vitest_1.expect)($fruits.parents().end()).toBe($fruits);
        });
        (0, vitest_1.it)('closest', () => {
            (0, vitest_1.expect)($fruits.closest('ul').end()).toBe($fruits);
        });
        (0, vitest_1.it)('siblings', () => {
            (0, vitest_1.expect)($fruits.siblings().end()).toBe($fruits);
        });
        (0, vitest_1.it)('next', () => {
            (0, vitest_1.expect)($fruits.next().end()).toBe($fruits);
        });
        (0, vitest_1.it)('nextAll', () => {
            (0, vitest_1.expect)($fruits.nextAll().end()).toBe($fruits);
        });
        (0, vitest_1.it)('prev', () => {
            (0, vitest_1.expect)($fruits.prev().end()).toBe($fruits);
        });
        (0, vitest_1.it)('prevAll', () => {
            (0, vitest_1.expect)($fruits.prevAll().end()).toBe($fruits);
        });
        (0, vitest_1.it)('clone', () => {
            (0, vitest_1.expect)($fruits.clone().end()).toBe($fruits);
        });
    });
    (0, vitest_1.describe)('.add()', () => {
        let $fruits;
        let $apple;
        let $orange;
        let $pear;
        (0, vitest_1.beforeEach)(() => {
            $ = (0, index_js_1.load)(fixtures_js_1.food);
            $fruits = $('#fruits');
            $apple = $('.apple');
            $orange = $('.orange');
            $pear = $('.pear');
        });
        (0, vitest_1.describe)('(selector) matched element :', () => {
            (0, vitest_1.it)('occurs before current selection', () => {
                const $selection = $orange.add('.apple');
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('is identical to the current selection', () => {
                const $selection = $orange.add('.orange');
                (0, vitest_1.expect)($selection).toHaveLength(1);
                (0, vitest_1.expect)($selection[0]).toBe($orange[0]);
            });
            (0, vitest_1.it)('occurs after current selection', () => {
                const $selection = $orange.add('.pear');
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[1]).toBe($pear[0]);
            });
            (0, vitest_1.it)('contains the current selection', () => {
                const $selection = $orange.add('#fruits');
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('is a child of the current selection', () => {
                const $selection = $fruits.add('.orange');
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('is root object preserved', () => {
                const $selection = $('<div></div>').add('#fruits');
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection.eq(0).is('div')).toBe(true);
                (0, vitest_1.expect)($selection.eq(1).is($fruits.eq(0))).toBe(true);
            });
        });
        (0, vitest_1.describe)('(selector) matched elements :', () => {
            (0, vitest_1.it)('occur before the current selection', () => {
                const $selection = $pear.add('.apple, .orange');
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('include the current selection', () => {
                const $selection = $pear.add('#fruits li');
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur after the current selection', () => {
                const $selection = $apple.add('.orange, .pear');
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur within the current selection', () => {
                const $selection = $fruits.add('#fruits li');
                (0, vitest_1.expect)($selection).toHaveLength(4);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[2]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[3]).toBe($pear[0]);
            });
        });
        (0, vitest_1.describe)('(selector, context) :', () => {
            (0, vitest_1.it)(', context)', () => {
                const $selection = $fruits.add('li', '#vegetables');
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($('.carrot')[0]);
                (0, vitest_1.expect)($selection[2]).toBe($('.sweetcorn')[0]);
            });
        });
        (0, vitest_1.describe)('(element) honors document order when element occurs :', () => {
            (0, vitest_1.it)('before the current selection', () => {
                const $selection = $orange.add($apple[0]);
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('after the current selection', () => {
                const $selection = $orange.add($pear[0]);
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[1]).toBe($pear[0]);
            });
            (0, vitest_1.it)('within the current selection', () => {
                const $selection = $fruits.add($orange[0]);
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('as an ancestor of the current selection', () => {
                const $selection = $orange.add($fruits[0]);
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('does not insert an element already contained within the current selection', () => {
                const $selection = $apple.add($apple[0]);
                (0, vitest_1.expect)($selection).toHaveLength(1);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
            });
        });
        (0, vitest_1.describe)('([elements]) : elements', () => {
            (0, vitest_1.it)('occur before the current selection', () => {
                const $selection = $pear.add($('.apple, .orange').get());
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('include the current selection', () => {
                const $selection = $pear.add($('#fruits li').get());
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur after the current selection', () => {
                const $selection = $apple.add($('.orange, .pear').get());
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur within the current selection', () => {
                const $selection = $fruits.add($('#fruits li').get());
                (0, vitest_1.expect)($selection).toHaveLength(4);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[2]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[3]).toBe($pear[0]);
            });
        });
        /**
         * Element order is undefined in this case, so it should not be asserted
         * here.
         *
         * If the collection consists of elements from different documents or ones
         * not in any document, the sort order is undefined.
         *
         * @see {@link https://api.jquery.com/add/}
         */
        (0, vitest_1.it)('(html) : correctly parses and adds the new elements', () => {
            const $selection = $apple.add('<li class="banana">banana</li>');
            (0, vitest_1.expect)($selection).toHaveLength(2);
            (0, vitest_1.expect)($selection.is('.apple')).toBe(true);
            (0, vitest_1.expect)($selection.is('.banana')).toBe(true);
        });
        (0, vitest_1.describe)('(selection) element in selection :', () => {
            (0, vitest_1.it)('occurs before current selection', () => {
                const $selection = $orange.add($('.apple'));
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('is identical to the current selection', () => {
                const $selection = $orange.add($('.orange'));
                (0, vitest_1.expect)($selection).toHaveLength(1);
                (0, vitest_1.expect)($selection[0]).toBe($orange[0]);
            });
            (0, vitest_1.it)('occurs after current selection', () => {
                const $selection = $orange.add($('.pear'));
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[1]).toBe($pear[0]);
            });
            (0, vitest_1.it)('contains the current selection', () => {
                const $selection = $orange.add($('#fruits'));
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
            (0, vitest_1.it)('is a child of the current selection', () => {
                const $selection = $fruits.add($('.orange'));
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
            });
        });
        (0, vitest_1.describe)('(selection) elements in the selection :', () => {
            (0, vitest_1.it)('occur before the current selection', () => {
                const $selection = $pear.add($('.apple, .orange'));
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('include the current selection', () => {
                const $selection = $pear.add($('#fruits li'));
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur after the current selection', () => {
                const $selection = $apple.add($('.orange, .pear'));
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[1]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[2]).toBe($pear[0]);
            });
            (0, vitest_1.it)('occur within the current selection', () => {
                const $selection = $fruits.add($('#fruits li'));
                (0, vitest_1.expect)($selection).toHaveLength(4);
                (0, vitest_1.expect)($selection[0]).toBe($fruits[0]);
                (0, vitest_1.expect)($selection[1]).toBe($apple[0]);
                (0, vitest_1.expect)($selection[2]).toBe($orange[0]);
                (0, vitest_1.expect)($selection[3]).toBe($pear[0]);
            });
        });
        (0, vitest_1.describe)('(selection) :', () => {
            (0, vitest_1.it)('modifying nested selections should not impact the parent [#834]', () => {
                const apple_pear = $apple.add($pear);
                // Applies red to apple and pear
                apple_pear.addClass('red');
                (0, vitest_1.expect)($apple.hasClass('red')).toBe(true); // This is true
                (0, vitest_1.expect)($pear.hasClass('red')).toBe(true); // This is true
                // Applies green to pear... AND should not affect apple
                $pear.addClass('green');
                (0, vitest_1.expect)($pear.hasClass('green')).toBe(true); // Currently this is true
                (0, vitest_1.expect)($apple.hasClass('green')).toBe(false); // And this should be false!
            });
        });
    });
    (0, vitest_1.describe)('.addBack', () => {
        (0, vitest_1.describe)('() :', () => {
            (0, vitest_1.it)('includes siblings and self', () => {
                const $selection = $('.orange').siblings().addBack();
                (0, vitest_1.expect)($selection).toHaveLength(3);
                (0, vitest_1.expect)($selection[0]).toBe($('.apple')[0]);
                (0, vitest_1.expect)($selection[1]).toBe($('.orange')[0]);
                (0, vitest_1.expect)($selection[2]).toBe($('.pear')[0]);
            });
            (0, vitest_1.it)('includes children and self', () => {
                const $selection = $('#fruits').children().addBack();
                (0, vitest_1.expect)($selection).toHaveLength(4);
                (0, vitest_1.expect)($selection[0]).toBe($('#fruits')[0]);
                (0, vitest_1.expect)($selection[1]).toBe($('.apple')[0]);
                (0, vitest_1.expect)($selection[2]).toBe($('.orange')[0]);
                (0, vitest_1.expect)($selection[3]).toBe($('.pear')[0]);
            });
            (0, vitest_1.it)('includes parent and self', () => {
                const $selection = $('.apple').parent().addBack();
                (0, vitest_1.expect)($selection).toHaveLength(2);
                (0, vitest_1.expect)($selection[0]).toBe($('#fruits')[0]);
                (0, vitest_1.expect)($selection[1]).toBe($('.apple')[0]);
            });
            (0, vitest_1.it)('includes parents and self', () => {
                const q = (0, index_js_1.load)(fixtures_js_1.food);
                const $selection = q('.apple').parents().addBack();
                (0, vitest_1.expect)($selection).toHaveLength(5);
                (0, vitest_1.expect)($selection[0]).toBe(q('html')[0]);
                (0, vitest_1.expect)($selection[1]).toBe(q('body')[0]);
                (0, vitest_1.expect)($selection[2]).toBe(q('#food')[0]);
                (0, vitest_1.expect)($selection[3]).toBe(q('#fruits')[0]);
                (0, vitest_1.expect)($selection[4]).toBe(q('.apple')[0]);
            });
        });
        (0, vitest_1.it)('(filter) : filters the previous selection', () => {
            const $selection = $('li').eq(1).addBack('.apple');
            (0, vitest_1.expect)($selection).toHaveLength(2);
            (0, vitest_1.expect)($selection[0]).toBe($('.apple')[0]);
            (0, vitest_1.expect)($selection[1]).toBe($('.orange')[0]);
        });
        (0, vitest_1.it)('() : fails gracefully when no args are passed', () => {
            const $div = (0, fixtures_js_1.cheerio)('<div>');
            (0, vitest_1.expect)($div.addBack()).toBe($div);
        });
    });
    (0, vitest_1.describe)('.is', () => {
        (0, vitest_1.it)('() : should return false', () => {
            (0, vitest_1.expect)($('li.apple').is()).toBe(false);
        });
        (0, vitest_1.it)('(true selector) : should return true', () => {
            (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('#vegetables', fixtures_js_1.vegetables).is('ul')).toBe(true);
        });
        (0, vitest_1.it)('(false selector) : should return false', () => {
            (0, vitest_1.expect)((0, fixtures_js_1.cheerio)('#vegetables', fixtures_js_1.vegetables).is('div')).toBe(false);
        });
        (0, vitest_1.it)('(true selection) : should return true', () => {
            const $vegetables = (0, fixtures_js_1.cheerio)('li', fixtures_js_1.vegetables);
            (0, vitest_1.expect)($vegetables.is($vegetables.eq(1))).toBe(true);
        });
        (0, vitest_1.it)('(false selection) : should return false', () => {
            const $vegetableList = (0, fixtures_js_1.cheerio)(fixtures_js_1.vegetables);
            const $vegetables = $vegetableList.find('li');
            (0, vitest_1.expect)($vegetables.is($vegetableList)).toBe(false);
        });
        (0, vitest_1.it)('(true element) : should return true', () => {
            const $vegetables = (0, fixtures_js_1.cheerio)('li', fixtures_js_1.vegetables);
            (0, vitest_1.expect)($vegetables.is($vegetables[0])).toBe(true);
        });
        (0, vitest_1.it)('(false element) : should return false', () => {
            const $vegetableList = (0, fixtures_js_1.cheerio)(fixtures_js_1.vegetables);
            const $vegetables = $vegetableList.find('li');
            (0, vitest_1.expect)($vegetables.is($vegetableList[0])).toBe(false);
        });
        (0, vitest_1.it)('(true predicate) : should return true', () => {
            const result = $('li').is(function () {
                return this.tagName === 'li' && $(this).hasClass('pear');
            });
            (0, vitest_1.expect)(result).toBe(true);
        });
        (0, vitest_1.it)('(false predicate) : should return false', () => {
            const result = $('li')
                .last()
                .is(function () {
                return this.tagName === 'ul';
            });
            (0, vitest_1.expect)(result).toBe(false);
        });
    });
});
