"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const htmlparser2_1 = require("htmlparser2");
const vitest_1 = require("vitest");
const parse_js_1 = require("./parse.js");
const parse5_adapter_js_1 = require("./parsers/parse5-adapter.js");
const defaultOpts = { _useHtmlParser2: false };
const parse = (0, parse_js_1.getParse)((content, options, isDocument, context) => options._useHtmlParser2
    ? (0, htmlparser2_1.parseDocument)(content, options)
    : (0, parse5_adapter_js_1.parseWithParse5)(content, options, isDocument, context));
// Tags
const basic = '<html></html>';
const siblings = '<h2></h2><p></p>';
// Single Tags
const single = '<br/>';
const singleWrong = '<br>';
// Children
const children = '<html><br/></html>';
const li = '<li class="durian">Durian</li>';
// Attributes
const attributes = '<img src="hello.png" alt="man waving">';
const noValueAttribute = '<textarea disabled></textarea>';
// Comments
const comment = '<!-- sexy -->';
const conditional = '<!--[if IE 8]><html class="no-js ie8" lang="en"><![endif]-->';
// Text
const text = 'lorem ipsum';
// Script
const script = '<script type="text/javascript">alert("hi world!");</script>';
const scriptEmpty = '<script></script>';
// Style
const style = '<style type="text/css"> h2 { color:blue; } </style>';
const styleEmpty = '<style></style>';
// Directives
const directive = '<!doctype html>';
function rootTest(root) {
    (0, vitest_1.expect)(root).toHaveProperty('type', 'root');
    (0, vitest_1.expect)(root.nextSibling).toBe(null);
    (0, vitest_1.expect)(root.previousSibling).toBe(null);
    (0, vitest_1.expect)(root.parentNode).toBe(null);
    const child = root.childNodes[0];
    (0, vitest_1.expect)(child.parentNode).toBe(root);
}
(0, vitest_1.describe)('parse', () => {
    (0, vitest_1.describe)('evaluate', () => {
        (0, vitest_1.it)(`should parse basic empty tags: ${basic}`, () => {
            const [tag] = parse(basic, defaultOpts, true, null).children;
            (0, vitest_1.expect)(tag.type).toBe('tag');
            (0, vitest_1.expect)(tag.tagName).toBe('html');
            (0, vitest_1.expect)(tag.childNodes).toHaveLength(2);
        });
        (0, vitest_1.it)(`should handle sibling tags: ${siblings}`, () => {
            const dom = parse(siblings, defaultOpts, false, null)
                .children;
            const [h2, p] = dom;
            (0, vitest_1.expect)(dom).toHaveLength(2);
            (0, vitest_1.expect)(h2.tagName).toBe('h2');
            (0, vitest_1.expect)(p.tagName).toBe('p');
        });
        (0, vitest_1.it)(`should handle single tags: ${single}`, () => {
            const [tag] = parse(single, defaultOpts, false, null)
                .children;
            (0, vitest_1.expect)(tag.type).toBe('tag');
            (0, vitest_1.expect)(tag.tagName).toBe('br');
            (0, vitest_1.expect)(tag.childNodes).toHaveLength(0);
        });
        (0, vitest_1.it)(`should handle malformatted single tags: ${singleWrong}`, () => {
            const [tag] = parse(singleWrong, defaultOpts, false, null)
                .children;
            (0, vitest_1.expect)(tag.type).toBe('tag');
            (0, vitest_1.expect)(tag.tagName).toBe('br');
            (0, vitest_1.expect)(tag.childNodes).toHaveLength(0);
        });
        (0, vitest_1.it)(`should handle tags with children: ${children}`, () => {
            const [tag] = parse(children, defaultOpts, true, null)
                .children;
            (0, vitest_1.expect)(tag.type).toBe('tag');
            (0, vitest_1.expect)(tag.tagName).toBe('html');
            (0, vitest_1.expect)(tag.childNodes).toBeTruthy();
            (0, vitest_1.expect)(tag.childNodes[1]).toHaveProperty('tagName', 'body');
            (0, vitest_1.expect)(tag.childNodes[1].childNodes).toHaveLength(1);
        });
        (0, vitest_1.it)(`should handle tags with children: ${li}`, () => {
            const [tag] = parse(li, defaultOpts, false, null).children;
            (0, vitest_1.expect)(tag.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(tag.childNodes[0]).toHaveProperty('data', 'Durian');
        });
        (0, vitest_1.it)(`should handle tags with attributes: ${attributes}`, () => {
            const attrs = parse(attributes, defaultOpts, false, null)
                .children[0];
            (0, vitest_1.expect)(attrs.attribs).toBeTruthy();
            (0, vitest_1.expect)(attrs.attribs).toHaveProperty('src', 'hello.png');
            (0, vitest_1.expect)(attrs.attribs).toHaveProperty('alt', 'man waving');
        });
        (0, vitest_1.it)(`should handle value-less attributes: ${noValueAttribute}`, () => {
            const attrs = parse(noValueAttribute, defaultOpts, false, null)
                .children[0];
            (0, vitest_1.expect)(attrs.attribs).toBeTruthy();
            (0, vitest_1.expect)(attrs.attribs).toHaveProperty('disabled', '');
        });
        (0, vitest_1.it)(`should handle comments: ${comment}`, () => {
            const elem = parse(comment, defaultOpts, false, null).children[0];
            (0, vitest_1.expect)(elem.type).toBe('comment');
            (0, vitest_1.expect)(elem).toHaveProperty('data', ' sexy ');
        });
        (0, vitest_1.it)(`should handle conditional comments: ${conditional}`, () => {
            const elem = parse(conditional, defaultOpts, false, null).children[0];
            (0, vitest_1.expect)(elem.type).toBe('comment');
            (0, vitest_1.expect)(elem).toHaveProperty('data', conditional.replace('<!--', '').replace('-->', ''));
        });
        (0, vitest_1.it)(`should handle text: ${text}`, () => {
            const text_ = parse(text, defaultOpts, false, null).children[0];
            (0, vitest_1.expect)(text_.type).toBe('text');
            (0, vitest_1.expect)(text_).toHaveProperty('data', 'lorem ipsum');
        });
        (0, vitest_1.it)(`should handle script tags: ${script}`, () => {
            const script_ = parse(script, defaultOpts, false, null)
                .children[0];
            (0, vitest_1.expect)(script_.type).toBe('script');
            (0, vitest_1.expect)(script_.tagName).toBe('script');
            (0, vitest_1.expect)(script_.attribs).toHaveProperty('type', 'text/javascript');
            (0, vitest_1.expect)(script_.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(script_.childNodes[0].type).toBe('text');
            (0, vitest_1.expect)(script_.childNodes[0]).toHaveProperty('data', 'alert("hi world!");');
        });
        (0, vitest_1.it)(`should handle style tags: ${style}`, () => {
            const style_ = parse(style, defaultOpts, false, null)
                .children[0];
            (0, vitest_1.expect)(style_.type).toBe('style');
            (0, vitest_1.expect)(style_.tagName).toBe('style');
            (0, vitest_1.expect)(style_.attribs).toHaveProperty('type', 'text/css');
            (0, vitest_1.expect)(style_.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(style_.childNodes[0].type).toBe('text');
            (0, vitest_1.expect)(style_.childNodes[0]).toHaveProperty('data', ' h2 { color:blue; } ');
        });
        (0, vitest_1.it)(`should handle directives: ${directive}`, () => {
            const elem = parse(directive, defaultOpts, true, null).children[0];
            (0, vitest_1.expect)(elem.type).toBe('directive');
            (0, vitest_1.expect)(elem).toHaveProperty('data', '!DOCTYPE html');
            (0, vitest_1.expect)(elem).toHaveProperty('name', '!doctype');
        });
    });
    (0, vitest_1.describe)('.parse', () => {
        // Root test utility
        (0, vitest_1.it)(`should add root to: ${basic}`, () => {
            const root = parse(basic, defaultOpts, true, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0]).toHaveProperty('tagName', 'html');
        });
        (0, vitest_1.it)(`should add root to: ${siblings}`, () => {
            const root = parse(siblings, defaultOpts, false, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(2);
            (0, vitest_1.expect)(root.childNodes[0]).toHaveProperty('tagName', 'h2');
            (0, vitest_1.expect)(root.childNodes[1]).toHaveProperty('tagName', 'p');
            (0, vitest_1.expect)(root.childNodes[1].parent).toBe(root);
        });
        (0, vitest_1.it)(`should add root to: ${comment}`, () => {
            const root = parse(comment, defaultOpts, false, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0].type).toBe('comment');
        });
        (0, vitest_1.it)(`should add root to: ${text}`, () => {
            const root = parse(text, defaultOpts, false, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0].type).toBe('text');
        });
        (0, vitest_1.it)(`should add root to: ${scriptEmpty}`, () => {
            const root = parse(scriptEmpty, defaultOpts, false, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0].type).toBe('script');
        });
        (0, vitest_1.it)(`should add root to: ${styleEmpty}`, () => {
            const root = parse(styleEmpty, defaultOpts, false, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0].type).toBe('style');
        });
        (0, vitest_1.it)(`should add root to: ${directive}`, () => {
            const root = parse(directive, defaultOpts, true, null);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(2);
            (0, vitest_1.expect)(root.childNodes[0].type).toBe('directive');
        });
        (0, vitest_1.it)('should simply return root', () => {
            const oldroot = parse(basic, defaultOpts, true, null);
            const root = parse(oldroot, defaultOpts, true, null);
            (0, vitest_1.expect)(root).toBe(oldroot);
            rootTest(root);
            (0, vitest_1.expect)(root.childNodes).toHaveLength(1);
            (0, vitest_1.expect)(root.childNodes[0]).toHaveProperty('tagName', 'html');
        });
        (0, vitest_1.it)('should expose the DOM level 1 API', () => {
            const root = parse('<div><a></a><span></span><p></p></div>', defaultOpts, false, null).childNodes[0];
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes).toHaveLength(3);
            (0, vitest_1.expect)(root.tagName).toBe('div');
            (0, vitest_1.expect)(root.firstChild).toBe(childNodes[0]);
            (0, vitest_1.expect)(root.lastChild).toBe(childNodes[2]);
            (0, vitest_1.expect)(childNodes[0].tagName).toBe('a');
            (0, vitest_1.expect)(childNodes[0].previousSibling).toBe(null);
            (0, vitest_1.expect)(childNodes[0].nextSibling).toBe(childNodes[1]);
            (0, vitest_1.expect)(childNodes[0].parentNode).toBe(root);
            (0, vitest_1.expect)(childNodes[0].childNodes).toHaveLength(0);
            (0, vitest_1.expect)(childNodes[0].firstChild).toBe(null);
            (0, vitest_1.expect)(childNodes[0].lastChild).toBe(null);
            (0, vitest_1.expect)(childNodes[1].tagName).toBe('span');
            (0, vitest_1.expect)(childNodes[1].previousSibling).toBe(childNodes[0]);
            (0, vitest_1.expect)(childNodes[1].nextSibling).toBe(childNodes[2]);
            (0, vitest_1.expect)(childNodes[1].parentNode).toBe(root);
            (0, vitest_1.expect)(childNodes[1].childNodes).toHaveLength(0);
            (0, vitest_1.expect)(childNodes[1].firstChild).toBe(null);
            (0, vitest_1.expect)(childNodes[1].lastChild).toBe(null);
            (0, vitest_1.expect)(childNodes[2].tagName).toBe('p');
            (0, vitest_1.expect)(childNodes[2].previousSibling).toBe(childNodes[1]);
            (0, vitest_1.expect)(childNodes[2].nextSibling).toBe(null);
            (0, vitest_1.expect)(childNodes[2].parentNode).toBe(root);
            (0, vitest_1.expect)(childNodes[2].childNodes).toHaveLength(0);
            (0, vitest_1.expect)(childNodes[2].firstChild).toBe(null);
            (0, vitest_1.expect)(childNodes[2].lastChild).toBe(null);
        });
        (0, vitest_1.it)('Should parse less than or equal sign sign', () => {
            const root = parse('<i>A</i><=<i>B</i>', defaultOpts, false, null);
            const { childNodes } = root;
            (0, vitest_1.expect)(childNodes[0]).toHaveProperty('tagName', 'i');
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('data', 'A');
            (0, vitest_1.expect)(childNodes[1]).toHaveProperty('data', '<=');
            (0, vitest_1.expect)(childNodes[2]).toHaveProperty('tagName', 'i');
            (0, vitest_1.expect)(childNodes[2].childNodes[0]).toHaveProperty('data', 'B');
        });
        (0, vitest_1.it)('Should ignore unclosed CDATA', () => {
            const root = parse('<a></a><script>foo //<![CDATA[ bar</script><b></b>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes[0].tagName).toBe('a');
            (0, vitest_1.expect)(childNodes[1].tagName).toBe('script');
            (0, vitest_1.expect)(childNodes[1].childNodes[0]).toHaveProperty('data', 'foo //<![CDATA[ bar');
            (0, vitest_1.expect)(childNodes[2].tagName).toBe('b');
        });
        (0, vitest_1.it)('Should add <head> to documents', () => {
            const root = parse('<html></html>', defaultOpts, true, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes[0].tagName).toBe('html');
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('tagName', 'head');
        });
        (0, vitest_1.it)('Should implicitly create <tr> around <td>', () => {
            const root = parse('<table><td>bar</td></tr></table>', defaultOpts, false, null);
            const table = root.childNodes[0];
            (0, vitest_1.expect)(table.tagName).toBe('table');
            (0, vitest_1.expect)(table.childNodes.length).toBe(1);
            const tbody = table.childNodes[0];
            (0, vitest_1.expect)(table.childNodes[0]).toHaveProperty('tagName', 'tbody');
            const tr = tbody.childNodes[0];
            (0, vitest_1.expect)(tr).toHaveProperty('tagName', 'tr');
            const td = tr.childNodes[0];
            (0, vitest_1.expect)(td).toHaveProperty('tagName', 'td');
            (0, vitest_1.expect)(td.childNodes[0]).toHaveProperty('data', 'bar');
        });
        (0, vitest_1.it)('Should parse custom tag <line>', () => {
            const root = parse('<line>test</line>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes.length).toBe(1);
            (0, vitest_1.expect)(childNodes[0].tagName).toBe('line');
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('data', 'test');
        });
        (0, vitest_1.it)('Should properly parse misnested table tags', () => {
            const root = parse('<tr><td>i1</td></tr><tr><td>i2</td></td></tr><tr><td>i3</td></td></tr>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes.length).toBe(3);
            for (let i = 0; i < childNodes.length; i++) {
                const child = childNodes[i];
                (0, vitest_1.expect)(child.tagName).toBe('tr');
                (0, vitest_1.expect)(child.childNodes[0]).toHaveProperty('tagName', 'td');
                (0, vitest_1.expect)(child.childNodes[0].childNodes[0]).toHaveProperty('data', `i${i + 1}`);
            }
        });
        (0, vitest_1.it)('Should correctly parse data url attributes', () => {
            const html = '<div style=\'font-family:"butcherman-caps"; src:url(data:font/opentype;base64,AAEA...);\'></div>';
            const expectedAttr = 'font-family:"butcherman-caps"; src:url(data:font/opentype;base64,AAEA...);';
            const root = parse(html, defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes[0].attribs).toHaveProperty('style', expectedAttr);
        });
        (0, vitest_1.it)('Should treat <xmp> tag content as text', () => {
            const root = parse('<xmp><h2></xmp>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('data', '<h2>');
        });
        (0, vitest_1.it)('Should correctly parse malformed numbered entities', () => {
            const root = parse('<p>z&#</p>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('data', 'z&#');
        });
        (0, vitest_1.it)('Should correctly parse mismatched headings', () => {
            const root = parse('<h2>Test</h3><div></div>', defaultOpts, false, null);
            const { childNodes } = root;
            (0, vitest_1.expect)(childNodes.length).toBe(2);
            (0, vitest_1.expect)(childNodes[0]).toHaveProperty('tagName', 'h2');
            (0, vitest_1.expect)(childNodes[1]).toHaveProperty('tagName', 'div');
        });
        (0, vitest_1.it)('Should correctly parse tricky <pre> content', () => {
            const root = parse('<pre>\nA <- factor(A, levels = c("c","a","b"))\n</pre>', defaultOpts, false, null);
            const childNodes = root.childNodes;
            (0, vitest_1.expect)(childNodes.length).toBe(1);
            (0, vitest_1.expect)(childNodes[0].tagName).toBe('pre');
            (0, vitest_1.expect)(childNodes[0].childNodes[0]).toHaveProperty('data', 'A <- factor(A, levels = c("c","a","b"))\n');
        });
        (0, vitest_1.it)('should pass the options for including the location info to parse5', () => {
            const root = parse('<p>Hello</p>', { ...defaultOpts, sourceCodeLocationInfo: true }, false, null);
            const location = root.children[0].sourceCodeLocation;
            (0, vitest_1.expect)(typeof location).toBe('object');
            (0, vitest_1.expect)(location?.endOffset).toBe(12);
        });
    });
});
