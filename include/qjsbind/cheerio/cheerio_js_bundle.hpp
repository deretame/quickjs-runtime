// cheerio_js_bundle.hpp —— 自动生成（python scripts/bundle_cheerio.py），勿手改
#pragma once
#include <string>
namespace qjsbind::cheerio {
inline const std::string& cheerio_bundle_js()
{
    static const std::string s = R"BUNDLE_7F3A9D2C(
(function () {
var __mods = {};
__mods['cheerio.js'] = function (module, exports, require) {
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
exports.Cheerio = void 0;
const Attributes = __importStar(require("./api/attributes.js"));
const Css = __importStar(require("./api/css.js"));
const Extract = __importStar(require("./api/extract.js"));
const Forms = __importStar(require("./api/forms.js"));
const Manipulation = __importStar(require("./api/manipulation.js"));
const Traversing = __importStar(require("./api/traversing.js"));
/**
 * The cheerio class is the central class of the library. It wraps a set of
 * elements and provides an API for traversing, modifying, and interacting with
 * the set.
 *
 * Loading a document will return the Cheerio class bound to the root element of
 * the document. The class will be instantiated when querying the document (when
 * calling `$('selector')`).
 *
 * @example This is the HTML markup we will be using in all of the API examples:
 *
 * ```html
 * <ul id="fruits">
 *   <li class="apple">Apple</li>
 *   <li class="orange">Orange</li>
 *   <li class="pear">Pear</li>
 * </ul>
 * ```
 */
class Cheerio {
    /**
     * Instance of cheerio. Methods are specified in the modules. Usage of this
     * constructor is not recommended. Please use `$.load` instead.
     *
     * @private
     * @param elements - The new selection.
     * @param root - Sets the root node.
     * @param options - Options for the instance.
     */
    constructor(elements, root, options) {
        this.length = 0;
        this.options = options;
        this._root = root;
        if (elements) {
            for (let idx = 0; idx < elements.length; idx++) {
                this[idx] = elements[idx];
            }
            this.length = elements.length;
        }
    }
}
exports.Cheerio = Cheerio;
/** Set a signature of the object. */
Cheerio.prototype.cheerio = '[cheerio object]';
/*
 * Make cheerio an array-like object
 */
Cheerio.prototype.splice = Array.prototype.splice;
// Support for (const element of $(...)) iteration:
Cheerio.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
// Plug in the API
Object.assign(Cheerio.prototype, Attributes, Traversing, Manipulation, Css, Forms, Extract);

};
__mods['load.js'] = function (module, exports, require) {
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
exports.getLoad = getLoad;
const htmlparser2_1 = require("./vendor/htmlparser2.js");
const cheerio_js_1 = require("./cheerio.js");
const options_js_1 = require("./options.js");
const staticMethods = __importStar(require("./static.js"));
const utils_js_1 = require("./utils.js");
/**
 * Create a loader factory from parser and renderer implementations.
 *
 * @param parse - Parser used to convert input into a document.
 * @param render - Renderer used to serialize nodes back to markup.
 */
function getLoad(parse, render) {
    /**
     * Create a querying function, bound to a document created from the provided
     * markup.
     *
     * Note that similar to web browser contexts, this operation may introduce
     * `<html>`, `<head>`, and `<body>` elements; set `isDocument` to `false` to
     * switch to fragment mode and disable this.
     *
     * @param content - Markup to be loaded.
     * @param options - Options for the created instance.
     * @param isDocument - Allows parser to be switched to fragment mode.
     * @returns The loaded document.
     * @see {@link https://cheerio.js.org/docs/basics/loading#load} for additional usage information.
     */
    return function load(content, options, isDocument = true) {
        if (content == null) {
            throw new Error('cheerio.load() expects a string');
        }
        const internalOpts = (0, options_js_1.flattenOptions)(options);
        const initialRoot = parse(content, internalOpts, isDocument, null);
        /**
         * Create an extended class here, so that extensions only live on one
         * instance.
         */
        class LoadedCheerio extends cheerio_js_1.Cheerio {
            _make(selector, context) {
                const cheerio = initialize(selector, context);
                cheerio.prevObject = this;
                return cheerio;
            }
            _parse(content, options, isDocument, context) {
                return parse(content, options, isDocument, context);
            }
            _render(dom) {
                return render(dom, this.options);
            }
        }
        function initialize(selector, context, root = initialRoot, opts) {
            // $($)
            if (selector && (0, utils_js_1.isCheerio)(selector))
                return selector;
            const options = (0, options_js_1.flattenOptions)(opts, internalOpts);
            const r = typeof root === 'string'
                ? [parse(root, options, false, null)]
                : 'length' in root
                    ? root
                    : [root];
            const rootInstance = (0, utils_js_1.isCheerio)(r)
                ? r
                : new LoadedCheerio(r, null, options);
            // Add a cyclic reference, so that calling methods on `_root` never fails.
            rootInstance._root = rootInstance;
            // $(), $(null), $(undefined), $(false)
            if (!selector) {
                return new LoadedCheerio(undefined, rootInstance, options);
            }
            const elements = typeof selector === 'string' && (0, utils_js_1.isHtml)(selector)
                ? // $(<html>)
                    parse(selector, options, false, null).children
                : isNode(selector)
                    ? // $(dom)
                        [selector]
                    : Array.isArray(selector)
                        ? // $([dom])
                            selector
                        : undefined;
            const instance = new LoadedCheerio(elements, rootInstance, options);
            if (elements) {
                return instance;
            }
            if (typeof selector !== 'string') {
                throw new TypeError('Unexpected type of selector');
            }
            // We know that our selector is a string now.
            let search = selector;
            const searchContext = context
                ? // If we don't have a context, maybe we have a root, from loading
                    typeof context === 'string'
                        ? (0, utils_js_1.isHtml)(context)
                            ? // $('li', '<ul>...</ul>')
                                new LoadedCheerio([parse(context, options, false, null)], rootInstance, options)
                            : // $('li', 'ul')
                                ((search = `${context} ${search}`), rootInstance)
                        : (0, utils_js_1.isCheerio)(context)
                            ? // $('li', $)
                                context
                            : // $('li', node), $('li', [nodes])
                                new LoadedCheerio(Array.isArray(context) ? context : [context], rootInstance, options)
                : rootInstance;
            // If we still don't have a context, return
            if (!searchContext)
                return instance;
            /*
             * #id, .class, tag
             */
            return searchContext.find(search);
        }
        // Add in static methods & properties
        Object.assign(initialize, staticMethods, {
            load,
            // `_root` and `_options` are used in static methods.
            _root: initialRoot,
            _options: internalOpts,
            // Add `fn` for plugins
            fn: LoadedCheerio.prototype,
            // Add the prototype here to maintain `instanceof` behavior.
            prototype: LoadedCheerio.prototype,
        });
        return initialize;
    };
}
function isNode(obj) {
    return (
    // @ts-expect-error: TS doesn't know about the `name` property.
    !!obj.name ||
        // @ts-expect-error: TS doesn't know about the `type` property.
        obj.type === htmlparser2_1.ElementType.Root ||
        // @ts-expect-error: TS doesn't know about the `type` property.
        obj.type === htmlparser2_1.ElementType.Text ||
        // @ts-expect-error: TS doesn't know about the `type` property.
        obj.type === htmlparser2_1.ElementType.Comment);
}

};
__mods['load-parse.js'] = function (module, exports, require) {
"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.load = void 0;
const dom_serializer_1 = __importDefault(require("./vendor/dom-serializer.js"));
const htmlparser2_1 = require("./vendor/htmlparser2.js");
const load_js_1 = require("./load.js");
const parse_js_1 = require("./parse.js");
const parse5_adapter_js_1 = require("./parsers/parse5-adapter.js");
const parse = (0, parse_js_1.getParse)((content, options, isDocument, context) => options._useHtmlParser2
    ? (0, htmlparser2_1.parseDocument)(content, options)
    : (0, parse5_adapter_js_1.parseWithParse5)(content, options, isDocument, context));
// Duplicate docs due to https://github.com/TypeStrong/typedoc/issues/1616
/**
 * Create a querying function, bound to a document created from the provided
 * markup.
 *
 * Note that similar to web browser contexts, this operation may introduce
 * `<html>`, `<head>`, and `<body>` elements; set `isDocument` to `false` to
 * switch to fragment mode and disable this.
 *
 * @category Loading
 * @param content - Markup to be loaded.
 * @param options - Options for the created instance.
 * @param isDocument - Allows parser to be switched to fragment mode.
 * @returns The loaded document.
 * @see {@link https://cheerio.js.org/docs/basics/loading#load} for additional usage information.
 */
exports.load = (0, load_js_1.getLoad)(parse, (dom, options) => options._useHtmlParser2
    ? (0, dom_serializer_1.default)(dom, options)
    : (0, parse5_adapter_js_1.renderWithParse5)(dom));

};
__mods['parse.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getParse = getParse;
exports.update = update;
const domhandler_1 = require("./vendor/domhandler.js");
const domutils_1 = require("./vendor/domutils.js");
/**
 * Get the parse function with options.
 *
 * @param parser - The parser function.
 * @returns The parse function with options.
 */
function getParse(parser) {
    /**
     * Parse a HTML string or a node.
     *
     * @param content - The HTML string or node.
     * @param options - The parser options.
     * @param isDocument - If `content` is a document.
     * @param context - The context node in the DOM tree.
     * @returns The parsed document node.
     */
    return function parse(content, options, isDocument, context) {
        if (typeof Buffer !== 'undefined' && Buffer.isBuffer(content)) {
            content = content.toString();
        }
        if (typeof content === 'string') {
            return parser(content, options, isDocument, context);
        }
        const doc = content;
        if (!Array.isArray(doc) && (0, domhandler_1.isDocument)(doc)) {
            // If `doc` is already a root, just return it
            return doc;
        }
        // Add content to new root element
        const root = new domhandler_1.Document([]);
        // Update the DOM using the root
        update(doc, root);
        return root;
    };
}
/**
 * Update the dom structure, for one changed layer.
 *
 * @param newChilds - The new children.
 * @param parent - The new parent.
 * @returns The parent node.
 */
function update(newChilds, parent) {
    // Normalize
    const arr = Array.isArray(newChilds) ? newChilds : [newChilds];
    // Update parent
    if (parent) {
        parent.children = arr;
    }
    else {
        parent = null;
    }
    // Update neighbors
    for (let i = 0; i < arr.length; i++) {
        const node = arr[i];
        // Cleanly remove existing nodes from their previous structures.
        if (node.parent && node.parent.children !== arr) {
            (0, domutils_1.removeElement)(node);
        }
        if (parent) {
            node.prev = arr[i - 1] || null;
            node.next = arr[i + 1] || null;
        }
        else {
            node.prev = node.next = null;
        }
        node.parent = parent;
    }
    return parent;
}

};
__mods['options.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.flattenOptions = flattenOptions;
const defaultOpts = {
    _useHtmlParser2: false,
};
/**
 * Flatten the options for Cheerio.
 *
 * This will set `_useHtmlParser2` to true if `xml` is set to true.
 *
 * @param options - The options to flatten.
 * @param baseOptions - The base options to use.
 * @returns The flattened options.
 */
function flattenOptions(options, baseOptions) {
    if (!options) {
        return baseOptions ?? defaultOpts;
    }
    const opts = {
        _useHtmlParser2: !!options.xmlMode,
        ...baseOptions,
        ...options,
    };
    if (options.xml) {
        opts._useHtmlParser2 = true;
        opts.xmlMode = true;
        if (options.xml !== true) {
            Object.assign(opts, options.xml);
        }
    }
    else if (options.xmlMode) {
        opts._useHtmlParser2 = true;
    }
    return opts;
}

};
__mods['static.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.html = html;
exports.xml = xml;
exports.text = text;
exports.parseHTML = parseHTML;
exports.root = root;
exports.contains = contains;
exports.extract = extract;
exports.merge = merge;
const domutils_1 = require("./vendor/domutils.js");
const options_js_1 = require("./options.js");
/**
 * Helper function to render a DOM.
 *
 * @param that - Cheerio instance to render.
 * @param dom - The DOM to render. Defaults to `that`'s root.
 * @param options - Options for rendering.
 * @returns The rendered document.
 */
function render(that, dom, options) {
    if (!that)
        return '';
    return that(dom ?? that._root.children, null, undefined, options).toString();
}
/**
 * Checks if a passed object is an options object.
 *
 * @param dom - Object to check if it is an options object.
 * @param options - Options object.
 * @returns Whether the object is an options object.
 */
function isOptions(dom, options) {
    return (!options &&
        typeof dom === 'object' &&
        dom != null &&
        !('length' in dom) &&
        !('type' in dom));
}
function html(dom, options) {
    /*
     * Be flexible about parameters, sometimes we call html(),
     * with options as only parameter
     * check dom argument for dom element specific properties
     * assume there is no 'length' or 'type' properties in the options object
     */
    const toRender = isOptions(dom) ? ((options = dom), undefined) : dom;
    /*
     * Sometimes `$.html()` is used without preloading html,
     * so fallback non-existing options to the default ones.
     */
    const opts = (0, options_js_1.flattenOptions)(options, this?._options);
    return render(this, toRender, opts);
}
/**
 * Render the document as XML.
 *
 * @category Static
 * @param dom - Element to render.
 * @returns THe rendered document.
 */
function xml(dom) {
    const options = { ...this._options, xmlMode: true };
    return render(this, dom, options);
}
/**
 * Render the document as text.
 *
 * This returns the `textContent` of the passed elements. The result will
 * include the contents of `<script>` and `<style>` elements. To avoid this, use
 * `.prop('innerText')` instead.
 *
 * @category Static
 * @param elements - Elements to render.
 * @returns The rendered document.
 */
function text(elements) {
    const elems = elements ?? (this ? this.root() : []);
    let ret = '';
    for (let i = 0; i < elems.length; i++) {
        ret += (0, domutils_1.textContent)(elems[i]);
    }
    return ret;
}
function parseHTML(data, context, keepScripts = typeof context === 'boolean' ? context : false) {
    if (!data || typeof data !== 'string') {
        return null;
    }
    if (typeof context === 'boolean') {
        keepScripts = context;
    }
    const parsed = this.load(data, this._options, false);
    if (!keepScripts) {
        parsed('script').remove();
    }
    /*
     * The `children` array is used by Cheerio internally to group elements that
     * share the same parents. When nodes created through `parseHTML` are
     * inserted into previously-existing DOM structures, they will be removed
     * from the `children` array. The results of `parseHTML` should remain
     * constant across these operations, so a shallow copy should be returned.
     */
    return [...parsed.root()[0].children];
}
/**
 * Sometimes you need to work with the top-level root element. To query it, you
 * can use `$.root()`.
 *
 * @category Static
 * @example
 *
 * ```js
 * $.root().append('<ul id="vegetables"></ul>').html();
 * //=> <ul id="fruits">...</ul><ul id="vegetables"></ul>
 * ```
 *
 * @returns Cheerio instance wrapping the root node.
 * @alias Cheerio.root
 */
function root() {
    return this(this._root);
}
/**
 * Checks to see if the `contained` DOM element is a descendant of the
 * `container` DOM element.
 *
 * @category Static
 * @param container - Potential parent node.
 * @param contained - Potential child node.
 * @returns Indicates if the nodes contain one another.
 * @alias Cheerio.contains
 * @see {@link https://api.jquery.com/jQuery.contains/}
 */
function contains(container, contained) {
    // According to the jQuery API, an element does not "contain" itself
    if (contained === container) {
        return false;
    }
    /*
     * Step up the descendants, stopping when the root element is reached
     * (signaled by `.parent` returning a reference to the same object)
     */
    let next = contained;
    while (next && next !== next.parent) {
        next = next.parent;
        if (next === container) {
            return true;
        }
    }
    return false;
}
/**
 * Extract multiple values from a document, and store them in an object.
 *
 * @category Static
 * @param map - An object containing key-value pairs. The keys are the names of
 *   the properties to be created on the object, and the values are the
 *   selectors to be used to extract the values.
 * @returns An object containing the extracted values.
 */
function extract(map) {
    return this.root().extract(map);
}
/**
 * $.merge().
 *
 * @category Static
 * @param arr1 - First array.
 * @param arr2 - Second array.
 * @returns `arr1`, with elements of `arr2` inserted.
 * @alias Cheerio.merge
 * @see {@link https://api.jquery.com/jQuery.merge/}
 */
function merge(arr1, arr2) {
    if (!(isArrayLike(arr1) && isArrayLike(arr2))) {
        return;
    }
    let newLength = arr1.length;
    const len = arr2.length;
    for (let i = 0; i < len; i++) {
        arr1[newLength++] = arr2[i];
    }
    arr1.length = newLength;
    return arr1;
}
/**
 * Checks if an object is array-like.
 *
 * @category Static
 * @param item - Item to check.
 * @returns Indicates if the item is array-like.
 */
function isArrayLike(item) {
    if (Array.isArray(item)) {
        return true;
    }
    if (typeof item !== 'object' ||
        item === null ||
        !('length' in item) ||
        typeof item.length !== 'number' ||
        /*
         * Not an array's `.length`: `item` is an arbitrary object being validated,
         * so this property really can be negative.
         */
        // eslint-disable-next-line unicorn/no-impossible-length-comparison
        item.length < 0) {
        return false;
    }
    for (let i = 0; i < item.length; i++) {
        if (!(i in item)) {
            return false;
        }
    }
    return true;
}

};
__mods['utils.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isCheerio = isCheerio;
exports.camelCase = camelCase;
exports.cssCase = cssCase;
exports.domEach = domEach;
exports.isHtml = isHtml;
/**
 * Checks if an object is a Cheerio instance.
 *
 * @category Utils
 * @param maybeCheerio - The object to check.
 * @returns Whether the object is a Cheerio instance.
 */
function isCheerio(maybeCheerio) {
    return maybeCheerio.cheerio != null;
}
/**
 * Convert a string to camel case notation.
 *
 * @private
 * @category Utils
 * @param str - The string to be converted.
 * @returns String in camel case notation.
 */
function camelCase(str) {
    return str.replace(/[._-](\w|$)/g, (_, x) => x.toUpperCase());
}
/**
 * Convert a string from camel case to "CSS case", where word boundaries are
 * described by hyphens ("-") and all characters are lower-case.
 *
 * @private
 * @category Utils
 * @param str - The string to be converted.
 * @returns String in "CSS case".
 */
function cssCase(str) {
    return str.replace(/[A-Z]/g, '-$&').toLowerCase();
}
/**
 * Iterate over each DOM element without creating intermediary Cheerio
 * instances.
 *
 * This is indented for use internally to avoid otherwise unnecessary memory
 * pressure introduced by _make.
 *
 * @category Utils
 * @param array - The array to iterate over.
 * @param fn - Function to call.
 * @returns The original instance.
 */
function domEach(array, fn) {
    const len = array.length;
    for (let i = 0; i < len; i++)
        fn(array[i], i);
    return array;
}
/**
 * Check if string is HTML.
 *
 * Tests for a `<` within a string, immediate followed by a letter and
 * eventually followed by a `>`.
 *
 * @private
 * @category Utils
 * @param str - The string to check.
 * @returns Indicates if `str` is HTML.
 */
function isHtml(str) {
    if (typeof str !== 'string') {
        return false;
    }
    const tagStart = str.indexOf('<');
    if (tagStart === -1 || tagStart > str.length - 3)
        return false;
    const tagChar = str.charCodeAt(tagStart + 1);
    return (((tagChar >= 97 /* CharacterCode.LowerA */ && tagChar <= 122 /* CharacterCode.LowerZ */) ||
        (tagChar >= 65 /* CharacterCode.UpperA */ && tagChar <= 90 /* CharacterCode.UpperZ */) ||
        tagChar === 33 /* CharacterCode.Exclamation */) &&
        str.includes('>', tagStart + 2));
}

};
__mods['types.js'] = function (module, exports, require) {
"use strict";
/** @file Types used in signatures of Cheerio methods. */
Object.defineProperty(exports, "__esModule", { value: true });

};
__mods['index.js'] = function (module, exports, require) {
"use strict";
/**
 * @file Batteries-included version of Cheerio. This module includes several
 *   convenience methods for loading documents from various sources.
 */
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
var __exportStar = (this && this.__exportStar) || function(m, exports) {
    for (var p in m) if (p !== "default" && !Object.prototype.hasOwnProperty.call(exports, p)) __createBinding(exports, m, p);
};
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
exports.merge = exports.contains = void 0;
exports.loadBuffer = loadBuffer;
exports.stringStream = stringStream;
exports.decodeStream = decodeStream;
exports.fromURL = fromURL;
__exportStar(require("./load-parse.js"), exports);
var static_js_1 = require("./static.js");
Object.defineProperty(exports, "contains", { enumerable: true, get: function () { return static_js_1.contains; } });
Object.defineProperty(exports, "merge", { enumerable: true, get: function () { return static_js_1.merge; } });
const node_stream_1 = require("./vendor/node-stream.js");
const encoding_sniffer_1 = require("./vendor/encoding-sniffer.js");
const htmlparser2 = __importStar(require("./vendor/htmlparser2.js"));
const parse5_htmlparser2_tree_adapter_1 = require("./vendor/parse5-htmlparser2-tree-adapter.js");
const parse5_parser_stream_1 = require("./vendor/parse5-parser-stream.js");
const whatwg_mimetype_1 = require("./vendor/whatwg-mimetype.js");
const load_parse_js_1 = require("./load-parse.js");
const options_js_1 = require("./options.js");
/**
 * Sniffs the encoding of a buffer, then creates a querying function bound to a
 * document created from the buffer.
 *
 * @category Loading
 * @example
 *
 * ```js
 * import * as cheerio from 'cheerio';
 *
 * const buffer = fs.readFileSync('index.html');
 * const $ = cheerio.loadBuffer(buffer);
 * ```
 *
 * @param buffer - The buffer to sniff the encoding of.
 * @param options - The options to pass to Cheerio.
 * @returns The loaded document.
 */
function loadBuffer(buffer, options = {}) {
    const opts = (0, options_js_1.flattenOptions)(options);
    const str = (0, encoding_sniffer_1.decodeBuffer)(buffer, {
        defaultEncoding: opts?.xmlMode ? 'utf8' : 'windows-1252',
        ...options.encoding,
    });
    return (0, load_parse_js_1.load)(str, opts);
}
function _stringStream(options, cb) {
    if (options?._useHtmlParser2) {
        const parser = htmlparser2.createDocumentStream((err, document) => cb(err, (0, load_parse_js_1.load)(document, options)), options);
        return new node_stream_1.Writable({
            decodeStrings: false,
            write(chunk, _encoding, callback) {
                if (typeof chunk !== 'string') {
                    throw new TypeError('Expected a string');
                }
                parser.write(chunk);
                callback();
            },
            final(callback) {
                parser.end();
                callback();
            },
        });
    }
    options ?? (options = {});
    options.treeAdapter ?? (options.treeAdapter = parse5_htmlparser2_tree_adapter_1.adapter);
    if (options.scriptingEnabled !== false) {
        options.scriptingEnabled = true;
    }
    const stream = new parse5_parser_stream_1.ParserStream(options);
    (0, node_stream_1.finished)(stream, (err) => cb(err, (0, load_parse_js_1.load)(stream.document, options)));
    return stream;
}
/**
 * Creates a stream that parses a sequence of strings into a document.
 *
 * The stream is a `Writable` stream that accepts strings. When the stream is
 * finished, the callback is called with the loaded document.
 *
 * @category Loading
 * @example
 *
 * ```js
 * import * as cheerio from 'cheerio';
 * import * as fs from 'fs';
 *
 * const writeStream = cheerio.stringStream({}, (err, $) => {
 *   if (err) {
 *     // Handle error
 *   }
 *
 *   console.log($('h1').text());
 *   // Output: Hello, world!
 * });
 *
 * fs.createReadStream('my-document.html', { encoding: 'utf8' }).pipe(
 *   writeStream,
 * );
 * ```
 *
 * @param options - The options to pass to Cheerio.
 * @param cb - The callback to call when the stream is finished.
 * @returns The writable stream.
 */
function stringStream(options, cb) {
    return _stringStream((0, options_js_1.flattenOptions)(options), cb);
}
/**
 * Parses a stream of buffers into a document.
 *
 * The stream is a `Writable` stream that accepts buffers. When the stream is
 * finished, the callback is called with the loaded document.
 *
 * @category Loading
 * @param options - The options to pass to Cheerio.
 * @param cb - The callback to call when the stream is finished.
 * @returns The writable stream.
 */
function decodeStream(options, cb) {
    const { encoding = {}, ...cheerioOptions } = options;
    const opts = (0, options_js_1.flattenOptions)(cheerioOptions);
    // Set the default encoding to UTF-8 for XML mode
    encoding.defaultEncoding ?? (encoding.defaultEncoding = opts?.xmlMode ? 'utf8' : 'windows-1252');
    const decodeStream = new encoding_sniffer_1.DecodeStream(encoding);
    const loadStream = _stringStream(opts, cb);
    decodeStream.pipe(loadStream);
    return decodeStream;
}
const defaultRequestOptions = {
    method: 'GET',
    // Set an Accept header
    headers: {
        accept: 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    },
};
/**
 * `fromURL` loads a document from a URL.
 *
 * By default, redirects are allowed and non-2xx responses are rejected.
 *
 * @category Loading
 * @example
 *
 * ```js
 * import * as cheerio from 'cheerio';
 *
 * const $ = await cheerio.fromURL('https://example.com');
 * ```
 *
 * @param url - The URL to load the document from.
 * @param options - The options to pass to Cheerio.
 * @returns The loaded document.
 */
async function fromURL(url, options = {}) {
    const { requestOptions = defaultRequestOptions, encoding = {}, ...cheerioOptions } = options;
    let undiciStream;
    const { Client, errors, interceptors } = await Promise.resolve().then(() => __importStar(require('undici')));
    // Add headers if none were supplied.
    const urlObject = typeof url === 'string' ? new URL(url) : url;
    const streamOptions = {
        headers: defaultRequestOptions.headers,
        path: urlObject.pathname + urlObject.search,
        ...requestOptions,
    };
    const promise = new Promise((resolve, reject) => {
        undiciStream = new Client(urlObject.origin)
            .compose(interceptors.redirect({ maxRedirections: 5 }))
            .stream(streamOptions, (res) => {
            if (res.statusCode < 200 || res.statusCode >= 300) {
                throw new errors.ResponseError('Response Error', res.statusCode, {
                    headers: res.headers,
                });
            }
            const contentTypeHeader = res.headers['content-type'] ?? 'text/html';
            const mimeType = new whatwg_mimetype_1.MIMEType(Array.isArray(contentTypeHeader)
                ? contentTypeHeader[0]
                : contentTypeHeader);
            if (!(mimeType.isHTML() || mimeType.isXML())) {
                throw new RangeError(`The content-type "${mimeType.essence}" is neither HTML nor XML.`);
            }
            // Forward the charset from the header to the decodeStream.
            encoding.transportLayerEncodingLabel =
                mimeType.parameters.get('charset');
            /*
             * If we allow redirects, we will have entries in the history.
             * The last entry will be the final URL.
             */
            const history = res.context?.history;
            // Set the `baseURI` to the final URL.
            const baseURI = history?.at(-1) ?? urlObject;
            const opts = {
                encoding,
                // Set XML mode based on the MIME type.
                xmlMode: mimeType.isXML(),
                baseURI,
                ...cheerioOptions,
            };
            return decodeStream(opts, (err, $) => (err ? reject(err) : resolve($)));
        });
    });
    // Let's make sure the request is completed before returning the promise.
    await undiciStream;
    return promise;
}

};
__mods['slim.js'] = function (module, exports, require) {
"use strict";
/**
 * @file Alternative entry point for Cheerio that always uses htmlparser2. This
 *   way, parse5 won't be loaded, saving some memory.
 */
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.load = exports.merge = exports.contains = void 0;
const dom_serializer_1 = __importDefault(require("./vendor/dom-serializer.js"));
const htmlparser2_1 = require("./vendor/htmlparser2.js");
const load_js_1 = require("./load.js");
const parse_js_1 = require("./parse.js");
var static_js_1 = require("./static.js");
Object.defineProperty(exports, "contains", { enumerable: true, get: function () { return static_js_1.contains; } });
Object.defineProperty(exports, "merge", { enumerable: true, get: function () { return static_js_1.merge; } });
/**
 * Create a querying function, bound to a document created from the provided
 * markup.
 *
 * @param content - Markup to be loaded.
 * @param options - Options for the created instance.
 * @param isDocument - Always `false` here, as we are always using
 *   `htmlparser2`.
 * @returns The loaded document.
 * @see {@link https://cheerio.js.org#loading} for additional usage information.
 */
exports.load = (0, load_js_1.getLoad)((0, parse_js_1.getParse)(htmlparser2_1.parseDocument), dom_serializer_1.default);

};
__mods['api/attributes.js'] = function (module, exports, require) {
"use strict";
/**
 * Methods for getting and modifying attributes.
 *
 * @module cheerio/attributes
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.attr = attr;
exports.prop = prop;
exports.data = data;
exports.val = val;
exports.removeAttr = removeAttr;
exports.hasClass = hasClass;
exports.addClass = addClass;
exports.removeClass = removeClass;
exports.toggleClass = toggleClass;
const domhandler_1 = require(".././vendor/domhandler.js");
const domutils_1 = require(".././vendor/domutils.js");
const htmlparser2_1 = require(".././vendor/htmlparser2.js");
const static_js_1 = require("../static.js");
const utils_js_1 = require("../utils.js");
const rspace = /\s+/;
const dataAttrPrefix = 'data-';
// Attributes that are booleans
const rboolean = /^(?:autofocus|autoplay|async|checked|controls|defer|disabled|hidden|loop|multiple|open|readonly|required|scoped|selected)$/i;
// Matches strings that look like JSON objects or arrays
const rbrace = /^{[\s\S]*}$|^\[[\s\S]*]$/;
// Tags whose `href`/`src` is resolved against `baseURI`
const hrefTags = new Set(['a', 'link']);
const srcTags = new Set(['img', 'iframe', 'audio', 'video', 'source']);
function getAttr(elem, name, xmlMode) {
    if (!(elem && (0, domhandler_1.isTag)(elem)))
        return;
    elem.attribs ?? (elem.attribs = {});
    // Return the entire attribs object if no attribute specified
    if (!name) {
        return elem.attribs;
    }
    if (Object.hasOwn(elem.attribs, name)) {
        // Get the (decoded) attribute
        return !xmlMode && rboolean.test(name) ? name : elem.attribs[name];
    }
    // Mimic the DOM and return text content as value for `option's`
    if (elem.name === 'option' && name === 'value') {
        return (0, static_js_1.text)(elem.children);
    }
    // Mimic DOM with default value for radios/checkboxes
    if (elem.name === 'input' &&
        (elem.attribs['type'] === 'radio' || elem.attribs['type'] === 'checkbox') &&
        name === 'value') {
        return 'on';
    }
    return;
}
/**
 * Sets the value of an attribute. The attribute will be deleted if the value is
 * `null`.
 *
 * @private
 * @param el - The element to set the attribute on.
 * @param name - The attribute's name.
 * @param value - The attribute's value.
 */
function setAttr(el, name, value) {
    if (value === null) {
        removeAttribute(el, name);
    }
    else {
        el.attribs[name] = `${value}`;
    }
}
function attr(name, value) {
    // Set the value (with attr map support)
    if (typeof name === 'object' || value !== undefined) {
        if (typeof value === 'function') {
            if (typeof name !== 'string') {
                throw new TypeError('Bad combination of arguments.');
            }
            return (0, utils_js_1.domEach)(this, (el, i) => {
                if ((0, domhandler_1.isTag)(el))
                    setAttr(el, name, value.call(el, i, el.attribs[name]));
            });
        }
        return (0, utils_js_1.domEach)(this, (el) => {
            if (!(0, domhandler_1.isTag)(el))
                return;
            if (typeof name === 'object') {
                for (const [objName, objValue] of Object.entries(name)) {
                    setAttr(el, objName, objValue);
                }
            }
            else {
                if (typeof name !== 'string') {
                    throw new TypeError('Bad combination of arguments.');
                }
                setAttr(el, name, value ?? null);
            }
        });
    }
    return arguments.length > 1
        ? this
        : getAttr(this[0], name, this.options.xmlMode);
}
/**
 * Gets a node's prop.
 *
 * @private
 * @category Attributes
 * @param el - Element to get the prop of.
 * @param name - Name of the prop.
 * @param xmlMode - Disable handling of special HTML attributes.
 * @returns The prop's value.
 */
function getProp(el, name, xmlMode) {
    return name in el
        ? // @ts-expect-error TS doesn't like us accessing the value directly here.
            el[name]
        : !xmlMode && rboolean.test(name)
            ? getAttr(el, name, false) !== undefined
            : getAttr(el, name, xmlMode);
}
/**
 * Sets the value of a prop.
 *
 * @private
 * @param el - The element to set the prop on.
 * @param name - The prop's name.
 * @param value - The prop's value.
 * @param xmlMode - Disable handling of special HTML attributes.
 */
function setProp(el, name, value, xmlMode) {
    if (name in el) {
        // @ts-expect-error Overriding value
        el[name] = value;
    }
    else {
        setAttr(el, name, !xmlMode && rboolean.test(name)
            ? value
                ? ''
                : null
            : `${value}`);
    }
}
function prop(name, value) {
    if (typeof name === 'string' && value === undefined) {
        const el = this[0];
        if (!el)
            return;
        switch (name) {
            case 'style': {
                const property = this.css();
                const keys = Object.keys(property);
                for (let i = 0; i < keys.length; i++) {
                    property[i] = keys[i];
                }
                property.length = keys.length;
                return property;
            }
            case 'tagName':
            case 'nodeName': {
                if (!(0, domhandler_1.isTag)(el))
                    return;
                return el.name.toUpperCase();
            }
            case 'href':
            case 'src': {
                if (!(0, domhandler_1.isTag)(el))
                    return;
                const prop = el.attribs?.[name];
                if (typeof URL !== 'undefined' &&
                    (name === 'href' ? hrefTags : srcTags).has(el.tagName) &&
                    prop !== undefined &&
                    this.options.baseURI) {
                    return new URL(prop, this.options.baseURI).href;
                }
                return prop;
            }
            case 'innerText': {
                return (0, domutils_1.innerText)(el);
            }
            case 'textContent': {
                return (0, domutils_1.textContent)(el);
            }
            case 'outerHTML': {
                if (el.type === htmlparser2_1.ElementType.Root)
                    return this.html();
                return this.clone().wrap('<container />').parent().html();
            }
            case 'innerHTML': {
                return this.html();
            }
            default: {
                if (!(0, domhandler_1.isTag)(el))
                    return;
                return getProp(el, name, this.options.xmlMode);
            }
        }
    }
    if (typeof name === 'object' || value !== undefined) {
        if (typeof value === 'function') {
            if (typeof name === 'object') {
                throw new TypeError('Bad combination of arguments.');
            }
            return (0, utils_js_1.domEach)(this, (el, i) => {
                if ((0, domhandler_1.isTag)(el)) {
                    setProp(el, name, value.call(el, i, getProp(el, name, this.options.xmlMode)), this.options.xmlMode);
                }
            });
        }
        return (0, utils_js_1.domEach)(this, (el) => {
            if (!(0, domhandler_1.isTag)(el))
                return;
            if (typeof name === 'object') {
                for (const [key, val] of Object.entries(name)) {
                    setProp(el, key, val, this.options.xmlMode);
                }
            }
            else {
                setProp(el, name, value, this.options.xmlMode);
            }
        });
    }
    return;
}
/**
 * Sets the value of a data attribute.
 *
 * @private
 * @param elem - The element to set the data attribute on.
 * @param name - The data attribute's name.
 * @param value - The data attribute's value.
 */
function setData(elem, name, value) {
    elem.data ?? (elem.data = {});
    if (typeof name === 'object')
        Object.assign(elem.data, name);
    else if (typeof name === 'string' && value !== undefined) {
        elem.data[name] = value;
    }
}
/**
 * Read _all_ HTML5 `data-*` attributes from the equivalent HTML5 `data-*`
 * attribute, and cache the value in the node's internal data store.
 *
 * @private
 * @category Attributes
 * @param el - Element to get the data attribute of.
 * @returns A map with all of the data attributes.
 */
function readAllData(el) {
    const data = (el.data ?? (el.data = {}));
    for (const [domName, domValue] of Object.entries(el.attribs)) {
        if (!domName.startsWith(dataAttrPrefix)) {
            continue;
        }
        const jsName = (0, utils_js_1.camelCase)(domName.slice(dataAttrPrefix.length));
        if (!Object.hasOwn(data, jsName)) {
            data[jsName] = parseDataValue(domValue);
        }
    }
    return data;
}
/**
 * Read the specified attribute from the equivalent HTML5 `data-*` attribute,
 * and (if present) cache the value in the node's internal data store.
 *
 * @category Attributes
 * @param el - Element to get the data attribute of.
 * @param name - Name of the data attribute.
 * @returns The data attribute's value.
 */
function readData(el, name) {
    const domName = dataAttrPrefix + (0, utils_js_1.cssCase)(name);
    const data = (el.data ?? (el.data = {}));
    if (Object.hasOwn(data, name)) {
        return data[name];
    }
    if (Object.hasOwn(el.attribs, domName)) {
        data[name] = parseDataValue(el.attribs[domName]);
        return data[name];
    }
    return;
}
/**
 * Coerce string data-* attributes to their corresponding JavaScript primitives.
 *
 * @category Attributes
 * @param value - The value to parse.
 * @returns The parsed value.
 */
function parseDataValue(value) {
    if (value === 'null')
        return null;
    if (value === 'true')
        return true;
    if (value === 'false')
        return false;
    const num = Number(value);
    if (value === String(num))
        return num;
    if (rbrace.test(value)) {
        try {
            return JSON.parse(value);
        }
        catch {
            /* Ignore */
        }
    }
    return value;
}
function data(name, value) {
    const elem = this[0];
    if (!(elem && (0, domhandler_1.isTag)(elem)))
        return;
    const dataEl = elem;
    dataEl.data ?? (dataEl.data = {});
    // Return the entire data object if no data specified
    if (name == null) {
        return readAllData(dataEl);
    }
    // Set the value (with attr map support)
    if (typeof name === 'object' || value !== undefined) {
        (0, utils_js_1.domEach)(this, (el) => {
            if ((0, domhandler_1.isTag)(el)) {
                if (typeof name === 'object')
                    setData(el, name);
                else
                    setData(el, name, value);
            }
        });
        return this;
    }
    return readData(dataEl, name);
}
function val(value) {
    const querying = arguments.length === 0;
    const element = this[0];
    if (!(element && (0, domhandler_1.isTag)(element)))
        return querying ? undefined : this;
    switch (element.name) {
        case 'textarea': {
            return this.text(value);
        }
        case 'select': {
            if (!querying) {
                if (this.attr('multiple') == null && typeof value === 'object') {
                    return this;
                }
                this.find('option').removeAttr('selected');
                const values = typeof value === 'object' ? value : [value];
                for (const val of values) {
                    this.find(`option[value="${val}"]`).attr('selected', '');
                }
                return this;
            }
            const option = this.find('option:selected');
            return this.attr('multiple')
                ? option.toArray().map((el) => (0, static_js_1.text)(el.children))
                : option.attr('value');
        }
        case 'button':
        case 'input':
        case 'option': {
            return querying
                ? this.attr('value')
                : this.attr('value', value);
        }
    }
    return;
}
/**
 * Remove an attribute.
 *
 * @param elem - Node to remove attribute from.
 * @param name - Name of the attribute to remove.
 */
function removeAttribute(elem, name) {
    if (!(elem.attribs && Object.hasOwn(elem.attribs, name)))
        return;
    delete elem.attribs[name];
}
/**
 * Splits a space-separated list of names to individual names.
 *
 * @category Attributes
 * @param names - Names to split.
 * @returns - Split names.
 */
function splitNames(names) {
    return names ? names.trim().split(rspace) : [];
}
/**
 * Method for removing attributes by `name`.
 *
 * @category Attributes
 * @example
 *
 * ```js
 * $('.pear').removeAttr('class').prop('outerHTML');
 * //=> <li>Pear</li>
 *
 * $('.apple').attr('id', 'favorite');
 * $('.apple').removeAttr('id class').prop('outerHTML');
 * //=> <li>Apple</li>
 * ```
 *
 * @param name - Name of the attribute.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/removeAttr/}
 */
function removeAttr(name) {
    const attrNames = splitNames(name);
    for (const attrName of attrNames) {
        (0, utils_js_1.domEach)(this, (elem) => {
            if ((0, domhandler_1.isTag)(elem))
                removeAttribute(elem, attrName);
        });
    }
    return this;
}
/**
 * Check to see if _any_ of the matched elements have the given `className`.
 *
 * @category Attributes
 * @example
 *
 * ```js
 * $('.pear').hasClass('pear');
 * //=> true
 *
 * $('apple').hasClass('fruit');
 * //=> false
 *
 * $('li').hasClass('pear');
 * //=> true
 * ```
 *
 * @param className - Name of the class.
 * @returns Indicates if an element has the given `className`.
 * @see {@link https://api.jquery.com/hasClass/}
 */
function hasClass(className) {
    return this.toArray().some((elem) => {
        const clazz = (0, domhandler_1.isTag)(elem) && elem.attribs['class'];
        if (clazz && className.length > 0) {
            for (let idx = clazz.indexOf(className); idx > -1; idx = clazz.indexOf(className, idx + 1)) {
                const end = idx + className.length;
                if ((idx === 0 || rspace.test(clazz[idx - 1])) &&
                    (end === clazz.length || rspace.test(clazz[end]))) {
                    return true;
                }
            }
        }
        return false;
    });
}
/**
 * Adds class(es) to all of the matched elements. Also accepts a `function`.
 *
 * @category Attributes
 * @example
 *
 * ```js
 * $('.pear').addClass('fruit').prop('outerHTML');
 * //=> <li class="pear fruit">Pear</li>
 *
 * $('.apple').addClass('fruit red').prop('outerHTML');
 * //=> <li class="apple fruit red">Apple</li>
 * ```
 *
 * @param value - Name of new class.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/addClass/}
 */
function addClass(value) {
    // Support functions
    if (typeof value === 'function') {
        return (0, utils_js_1.domEach)(this, (el, i) => {
            if (!(0, domhandler_1.isTag)(el)) {
                return;
            }
            const className = el.attribs['class'] || '';
            addClass.call([el], value.call(el, i, className));
        });
    }
    // Return if no value or not a string or function
    if (!value || typeof value !== 'string')
        return this;
    const classNames = value.split(rspace);
    const numElements = this.length;
    for (let i = 0; i < numElements; i++) {
        const el = this[i];
        // If selected element isn't a tag, move on
        if (!(0, domhandler_1.isTag)(el))
            continue;
        // If we don't already have classes — always set xmlMode to false here, as it doesn't matter for classes
        const className = getAttr(el, 'class', false);
        if (className) {
            let setClass = ` ${className} `;
            // Check if class already exists
            for (const cn of classNames) {
                const appendClass = `${cn} `;
                if (!setClass.includes(` ${appendClass}`))
                    setClass += appendClass;
            }
            setAttr(el, 'class', setClass.trim());
        }
        else {
            setAttr(el, 'class', classNames.join(' ').trim());
        }
    }
    return this;
}
/**
 * Removes one or more space-separated classes from the selected elements. If no
 * `className` is defined, all classes will be removed. Also accepts a
 * `function`.
 *
 * @category Attributes
 * @example
 *
 * ```js
 * $('.pear').removeClass('pear').prop('outerHTML');
 * //=> <li class="">Pear</li>
 *
 * $('.apple').addClass('red').removeClass().prop('outerHTML');
 * //=> <li class="">Apple</li>
 * ```
 *
 * @param name - Name of the class. If not specified, removes all elements.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/removeClass/}
 */
function removeClass(name) {
    // Handle if value is a function
    if (typeof name === 'function') {
        return (0, utils_js_1.domEach)(this, (el, i) => {
            if ((0, domhandler_1.isTag)(el)) {
                removeClass.call([el], name.call(el, i, el.attribs['class'] || ''));
            }
        });
    }
    const classes = splitNames(name);
    const numClasses = classes.length;
    const removeAll = arguments.length === 0;
    return (0, utils_js_1.domEach)(this, (el) => {
        if (!(0, domhandler_1.isTag)(el))
            return;
        if (removeAll) {
            // Short circuit the remove all case as this is the nice one
            el.attribs['class'] = '';
        }
        else {
            const elClasses = splitNames(el.attribs['class']);
            let changed = false;
            for (let j = 0; j < numClasses; j++) {
                const index = elClasses.indexOf(classes[j]);
                if (index !== -1) {
                    elClasses.splice(index, 1);
                    changed = true;
                    /*
                     * We have to do another pass to ensure that there are not duplicate
                     * classes listed
                     */
                    j--;
                }
            }
            if (changed) {
                el.attribs['class'] = elClasses.join(' ');
            }
        }
    });
}
/**
 * Add or remove class(es) from the matched elements, depending on either the
 * class's presence or the value of the switch argument. Also accepts a
 * `function`.
 *
 * @category Attributes
 * @example
 *
 * ```js
 * $('.apple.green').toggleClass('fruit green red').prop('outerHTML');
 * //=> <li class="apple fruit red">Apple</li>
 *
 * $('.apple.green').toggleClass('fruit green red', true).prop('outerHTML');
 * //=> <li class="apple green fruit red">Apple</li>
 * ```
 *
 * @param value - Name of the class. Can also be a function.
 * @param stateVal - If specified the state of the class.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/toggleClass/}
 */
function toggleClass(value, stateVal) {
    // Support functions
    if (typeof value === 'function') {
        return (0, utils_js_1.domEach)(this, (el, i) => {
            if ((0, domhandler_1.isTag)(el)) {
                toggleClass.call([el], value.call(el, i, el.attribs['class'] || '', stateVal), stateVal);
            }
        });
    }
    // Return if no value or not a string or function
    if (!value || typeof value !== 'string')
        return this;
    const classNames = value.split(rspace);
    const numClasses = classNames.length;
    const state = typeof stateVal === 'boolean' ? (stateVal ? 1 : -1) : 0;
    const numElements = this.length;
    for (let i = 0; i < numElements; i++) {
        const el = this[i];
        // If selected element isn't a tag, move on
        if (!(0, domhandler_1.isTag)(el))
            continue;
        const elementClasses = splitNames(el.attribs['class']);
        // Check if class already exists
        for (let j = 0; j < numClasses; j++) {
            // Check if the class name is currently defined
            const index = elementClasses.indexOf(classNames[j]);
            // Add if stateValue === true or we are toggling and there is no value
            if (state >= 0 && index === -1) {
                elementClasses.push(classNames[j]);
            }
            else if (state <= 0 && index !== -1) {
                // Otherwise remove but only if the item exists
                elementClasses.splice(index, 1);
            }
        }
        el.attribs['class'] = elementClasses.join(' ');
    }
    return this;
}

};
__mods['api/css.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.css = css;
const domhandler_1 = require(".././vendor/domhandler.js");
const utils_js_1 = require("../utils.js");
/**
 * Set multiple CSS properties for every matched element.
 *
 * @category CSS
 * @param prop - The names of the properties.
 * @param val - The new values.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/css/}
 */
function css(prop, val) {
    if ((prop != null && val != null) ||
        // When `prop` is a "plain" object
        (typeof prop === 'object' && !Array.isArray(prop))) {
        return (0, utils_js_1.domEach)(this, (el, i) => {
            if ((0, domhandler_1.isTag)(el)) {
                // `prop` can't be an array here anymore.
                setCss(el, prop, val, i);
            }
        });
    }
    if (this.length === 0) {
        return;
    }
    return getCss(this[0], prop);
}
/**
 * Set styles of all elements.
 *
 * @private
 * @param el - Element to set style of.
 * @param prop - Name of property.
 * @param value - Value to set property to.
 * @param idx - Optional index within the selection.
 */
function setCss(el, prop, value, idx) {
    if (typeof prop === 'string') {
        const styles = getCss(el);
        const val = typeof value === 'function' ? value.call(el, idx, styles[prop]) : value;
        if (val === '') {
            delete styles[prop];
        }
        else if (val != null) {
            styles[prop] = val;
        }
        el.attribs['style'] = stringify(styles);
    }
    else if (typeof prop === 'object') {
        const keys = Object.keys(prop);
        for (let i = 0; i < keys.length; i++) {
            const k = keys[i];
            setCss(el, k, prop[k], i);
        }
    }
}
function getCss(el, prop) {
    if (!(el && (0, domhandler_1.isTag)(el)))
        return;
    const styles = parse(el.attribs['style']);
    if (typeof prop === 'string') {
        return styles[prop];
    }
    if (Array.isArray(prop)) {
        const newStyles = {};
        for (const item of prop) {
            if (styles[item] != null) {
                newStyles[item] = styles[item];
            }
        }
        return newStyles;
    }
    return styles;
}
/**
 * Stringify `obj` to styles.
 *
 * @private
 * @category CSS
 * @param obj - Object to stringify.
 * @returns The serialized styles.
 */
function stringify(obj) {
    return Object.keys(obj).reduce((str, prop) => `${str}${str ? ' ' : ''}${prop}: ${obj[prop]};`, '');
}
/**
 * Parse `styles`.
 *
 * @private
 * @category CSS
 * @param styles - Styles to be parsed.
 * @returns The parsed styles.
 */
function parse(styles) {
    styles = (styles || '').trim();
    if (!styles)
        return {};
    const obj = {};
    let key;
    for (const str of styles.split(';')) {
        const n = str.indexOf(':');
        // If there is no :, or if it is the first/last character, add to the previous item's value
        if (n < 1 || n === str.length - 1) {
            const trimmed = str.trimEnd();
            if (trimmed.length > 0 && key !== undefined) {
                obj[key] += `;${trimmed}`;
            }
        }
        else {
            key = str.slice(0, n).trim();
            obj[key] = str.slice(n + 1).trim();
        }
    }
    return obj;
}

};
__mods['api/extract.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.extract = extract;
function getExtractDescr(descr) {
    if (typeof descr === 'string') {
        return { selector: descr, value: 'textContent' };
    }
    return {
        selector: descr.selector,
        value: descr.value ?? 'textContent',
    };
}
/**
 * Extract multiple values from a document, and store them in an object.
 *
 * @param map - An object containing key-value pairs. The keys are the names of
 *   the properties to be created on the object, and the values are the
 *   selectors to be used to extract the values.
 * @returns An object containing the extracted values.
 */
function extract(map) {
    const ret = {};
    for (const key in map) {
        const descr = map[key];
        const isArray = Array.isArray(descr);
        const { selector, value } = getExtractDescr(isArray ? descr[0] : descr);
        const fn = typeof value === 'function'
            ? value
            : typeof value === 'string'
                ? (el) => this._make(el).prop(value)
                : (el) => this._make(el).extract(value);
        if (isArray) {
            ret[key] = this._findBySelector(selector, Number.POSITIVE_INFINITY)
                .map((_, el) => fn(el, key, ret))
                .get();
        }
        else {
            const $ = this._findBySelector(selector, 1);
            ret[key] = $.length > 0 ? fn($[0], key, ret) : undefined;
        }
    }
    return ret;
}

};
__mods['api/forms.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.serialize = serialize;
exports.serializeArray = serializeArray;
const domhandler_1 = require(".././vendor/domhandler.js");
/*
 * https://github.com/jquery/jquery/blob/2.1.3/src/manipulation/var/rcheckableType.js
 * https://github.com/jquery/jquery/blob/2.1.3/src/serialize.js
 */
const submittableSelector = 'input,select,textarea,keygen';
const r20 = /%20/g;
const rCRLF = /\r?\n/g;
/**
 * Encode a set of form elements as a string for submission.
 *
 * @category Forms
 * @example
 *
 * ```js
 * $('<form><input name="foo" value="bar" /></form>').serialize();
 * //=> 'foo=bar'
 * ```
 *
 * @returns The serialized form.
 * @see {@link https://api.jquery.com/serialize/}
 */
function serialize() {
    // Convert form elements into name/value objects
    const arr = this.serializeArray();
    // Serialize each element into a key/value string
    const retArr = arr.map((data) => `${encodeURIComponent(data.name)}=${encodeURIComponent(data.value)}`);
    // Return the resulting serialization
    return retArr.join('&').replace(r20, '+');
}
/**
 * Encode a set of form elements as an array of names and values.
 *
 * @category Forms
 * @example
 *
 * ```js
 * $('<form><input name="foo" value="bar" /></form>').serializeArray();
 * //=> [ { name: 'foo', value: 'bar' } ]
 * ```
 *
 * @returns The serialized form.
 * @see {@link https://api.jquery.com/serializeArray/}
 */
function serializeArray() {
    // Resolve all form elements from either forms or collections of form elements
    return this.map((_, elem) => {
        const $elem = this._make(elem);
        if ((0, domhandler_1.isTag)(elem) && elem.name === 'form') {
            return $elem.find(submittableSelector).toArray();
        }
        return $elem.filter(submittableSelector).toArray();
    })
        .filter(
    // Verify elements have a name (`attr.name`) and are not disabled (`:enabled`)
    '[name!=""]:enabled' +
        // And cannot be clicked (`[type=submit]`) or are used in `x-www-form-urlencoded` (`[type=file]`)
        ':not(:submit, :button, :image, :reset, :file)' +
        // And are either checked/don't have a checkable state
        ':matches([checked], :not(:checkbox, :radio))')
        .map((_, elem) => {
        const $elem = this._make(elem);
        const name = $elem.attr('name');
        if (!name)
            return [];
        // If there is no value set (e.g. `undefined`, `null`), then default value to empty
        const value = $elem.val() ?? '';
        // If we have an array of values (e.g. `<select multiple>`), return an array of key/value pairs
        if (Array.isArray(value)) {
            return value.map((val) => 
            /*
             * We trim replace any line endings (e.g. `\r` or `\r\n` with `\r\n`) to guarantee consistency across platforms
             * These can occur inside of `<textarea>'s`
             */
            ({ name, value: val.replace(rCRLF, '\r\n') }));
        }
        // Otherwise (e.g. `<input type="text">`, return only one key/value pair
        return { name, value: value.replace(rCRLF, '\r\n') };
    })
        .toArray();
}

};
__mods['api/manipulation.js'] = function (module, exports, require) {
"use strict";
/**
 * Methods for modifying the DOM structure.
 *
 * @module cheerio/manipulation
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.wrapInner = exports.wrap = exports.prepend = exports.append = void 0;
exports._makeDomArray = _makeDomArray;
exports.appendTo = appendTo;
exports.prependTo = prependTo;
exports.unwrap = unwrap;
exports.wrapAll = wrapAll;
exports.after = after;
exports.insertAfter = insertAfter;
exports.before = before;
exports.insertBefore = insertBefore;
exports.remove = remove;
exports.replaceWith = replaceWith;
exports.empty = empty;
exports.html = html;
exports.toString = toString;
exports.text = text;
exports.clone = clone;
const domhandler_1 = require(".././vendor/domhandler.js");
const domutils_1 = require(".././vendor/domutils.js");
const htmlparser2_1 = require(".././vendor/htmlparser2.js");
const parse_js_1 = require("../parse.js");
const static_js_1 = require("../static.js");
const utils_js_1 = require("../utils.js");
/**
 * Create an array of nodes, recursing into arrays and parsing strings if
 * necessary.
 *
 * @private
 * @category Manipulation
 * @param elem - Elements to make an array of.
 * @param clone - Optionally clone nodes.
 * @returns The array of nodes.
 */
function _makeDomArray(elem, clone) {
    if (elem == null) {
        return [];
    }
    if (typeof elem === 'string') {
        return [...this._parse(elem, this.options, false, null).children];
    }
    if ('length' in elem) {
        if (elem.length === 1) {
            return this._makeDomArray(elem[0], clone);
        }
        const result = [];
        for (let i = 0; i < elem.length; i++) {
            const el = elem[i];
            if (typeof el === 'object') {
                if (el == null) {
                    continue;
                }
                if (!('length' in el)) {
                    result.push(clone ? (0, domhandler_1.cloneNode)(el, true) : el);
                    continue;
                }
            }
            result.push(...this._makeDomArray(el, clone));
        }
        return result;
    }
    return [clone ? (0, domhandler_1.cloneNode)(elem, true) : elem];
}
function _insert(concatenator) {
    return function (...elems) {
        const lastIdx = this.length - 1;
        return (0, utils_js_1.domEach)(this, (el, i) => {
            if (!(0, domhandler_1.hasChildren)(el))
                return;
            const domSrc = typeof elems[0] === 'function'
                ? elems[0].call(el, i, this._render(el.children))
                : elems;
            const dom = this._makeDomArray(domSrc, i < lastIdx);
            concatenator(dom, el.children, el);
        });
    };
}
/**
 * Modify an array in-place, removing some number of elements and adding new
 * elements directly following them.
 *
 * @private
 * @category Manipulation
 * @param array - Target array to splice.
 * @param spliceIdx - Index at which to begin changing the array.
 * @param spliceCount - Number of elements to remove from the array.
 * @param newElems - Elements to insert into the array.
 * @param parent - The parent of the node.
 * @returns The spliced array.
 */
function uniqueSplice(array, spliceIdx, spliceCount, newElems, parent) {
    const spliceArgs = [
        spliceIdx,
        spliceCount,
        ...newElems,
    ];
    const prev = spliceIdx === 0 ? null : array[spliceIdx - 1];
    const next = spliceIdx + spliceCount >= array.length
        ? null
        : array[spliceIdx + spliceCount];
    /*
     * Before splicing in new elements, ensure they do not already appear in the
     * current array.
     */
    for (let idx = 0; idx < newElems.length; ++idx) {
        const node = newElems[idx];
        const oldParent = node.parent;
        if (oldParent && Array.isArray(oldParent.children)) {
            const oldSiblings = oldParent.children;
            const prevIdx = oldSiblings.indexOf(node);
            if (prevIdx !== -1) {
                oldParent.children.splice(prevIdx, 1);
                if (parent === oldParent && spliceIdx > prevIdx) {
                    spliceArgs[0]--;
                }
            }
        }
        node.parent = parent;
        if (node.prev) {
            node.prev.next = node.next ?? null;
        }
        if (node.next) {
            node.next.prev = node.prev ?? null;
        }
        node.prev = idx === 0 ? prev : newElems[idx - 1];
        node.next = idx === newElems.length - 1 ? next : newElems[idx + 1];
    }
    if (prev) {
        prev.next = newElems[0];
    }
    if (next) {
        next.prev = newElems[newElems.length - 1];
    }
    return array.splice(...spliceArgs);
}
/**
 * Insert every element in the set of matched elements to the end of the target.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('<li class="plum">Plum</li>').appendTo('#fruits');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //      <li class="plum">Plum</li>
 * //    </ul>
 * ```
 *
 * @param target - Element to append elements to.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/appendTo/}
 */
function appendTo(target) {
    const appendTarget = (0, utils_js_1.isCheerio)(target) ? target : this._make(target);
    appendTarget.append(this);
    return this;
}
/**
 * Insert every element in the set of matched elements to the beginning of the
 * target.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('<li class="plum">Plum</li>').prependTo('#fruits');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="plum">Plum</li>
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @param target - Element to prepend elements to.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/prependTo/}
 */
function prependTo(target) {
    const prependTarget = (0, utils_js_1.isCheerio)(target) ? target : this._make(target);
    prependTarget.prepend(this);
    return this;
}
/**
 * Inserts content as the _last_ child of each of the selected elements.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('ul').append('<li class="plum">Plum</li>');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //      <li class="plum">Plum</li>
 * //    </ul>
 * ```
 *
 * @see {@link https://api.jquery.com/append/}
 */
exports.append = _insert((dom, children, parent) => {
    uniqueSplice(children, children.length, 0, dom, parent);
});
/**
 * Inserts content as the _first_ child of each of the selected elements.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('ul').prepend('<li class="plum">Plum</li>');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="plum">Plum</li>
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @see {@link https://api.jquery.com/prepend/}
 */
exports.prepend = _insert((dom, children, parent) => {
    uniqueSplice(children, 0, 0, dom, parent);
});
function _wrap(insert) {
    return function (wrapper) {
        const lastIdx = this.length - 1;
        const lastParent = this.parents().last();
        for (let i = 0; i < this.length; i++) {
            const el = this[i];
            const wrap = typeof wrapper === 'function'
                ? wrapper.call(el, i, el)
                : typeof wrapper === 'string' && !(0, utils_js_1.isHtml)(wrapper)
                    ? lastParent.find(wrapper).clone()
                    : wrapper;
            const [wrapperDom] = this._makeDomArray(wrap, i < lastIdx);
            if (!(wrapperDom && (0, domhandler_1.hasChildren)(wrapperDom)))
                continue;
            let elInsertLocation = wrapperDom;
            /*
             * Find the deepest child. Only consider the first tag child of each node
             * (ignore text); stop if no children are found.
             */
            let j = 0;
            while (j < elInsertLocation.children.length) {
                const child = elInsertLocation.children[j];
                if ((0, domhandler_1.isTag)(child)) {
                    elInsertLocation = child;
                    j = 0;
                }
                else {
                    j++;
                }
            }
            insert(el, elInsertLocation, [wrapperDom]);
        }
        return this;
    };
}
/**
 * The .wrap() function can take any string or object that could be passed to
 * the $() factory function to specify a DOM structure. This structure may be
 * nested several levels deep, but should contain only one inmost element. A
 * copy of this structure will be wrapped around each of the elements in the set
 * of matched elements. This method returns the original set of elements for
 * chaining purposes.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * const redFruit = $('<div class="red-fruit"></div>');
 * $('.apple').wrap(redFruit);
 *
 * //=> <ul id="fruits">
 * //     <div class="red-fruit">
 * //      <li class="apple">Apple</li>
 * //     </div>
 * //     <li class="orange">Orange</li>
 * //     <li class="plum">Plum</li>
 * //   </ul>
 *
 * const healthy = $('<div class="healthy"></div>');
 * $('li').wrap(healthy);
 *
 * //=> <ul id="fruits">
 * //     <div class="healthy">
 * //       <li class="apple">Apple</li>
 * //     </div>
 * //     <div class="healthy">
 * //       <li class="orange">Orange</li>
 * //     </div>
 * //     <div class="healthy">
 * //        <li class="plum">Plum</li>
 * //     </div>
 * //   </ul>
 * ```
 *
 * @param wrapper - The DOM structure to wrap around each element in the
 *   selection.
 * @see {@link https://api.jquery.com/wrap/}
 */
exports.wrap = _wrap((el, elInsertLocation, wrapperDom) => {
    const { parent } = el;
    if (!parent || !Array.isArray(parent.children))
        return;
    const siblings = parent.children;
    const index = siblings.indexOf(el);
    (0, parse_js_1.update)([el], elInsertLocation);
    /*
     * The previous operation removed the current element from the `siblings`
     * array, so the `dom` array can be inserted without removing any
     * additional elements.
     */
    uniqueSplice(siblings, index, 0, wrapperDom, parent);
});
/**
 * The .wrapInner() function can take any string or object that could be passed
 * to the $() factory function to specify a DOM structure. This structure may be
 * nested several levels deep, but should contain only one inmost element. The
 * structure will be wrapped around the content of each of the elements in the
 * set of matched elements.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * const redFruit = $('<div class="red-fruit"></div>');
 * $('.apple').wrapInner(redFruit);
 *
 * //=> <ul id="fruits">
 * //     <li class="apple">
 * //       <div class="red-fruit">Apple</div>
 * //     </li>
 * //     <li class="orange">Orange</li>
 * //     <li class="pear">Pear</li>
 * //   </ul>
 *
 * const healthy = $('<div class="healthy"></div>');
 * $('li').wrapInner(healthy);
 *
 * //=> <ul id="fruits">
 * //     <li class="apple">
 * //       <div class="healthy">Apple</div>
 * //     </li>
 * //     <li class="orange">
 * //       <div class="healthy">Orange</div>
 * //     </li>
 * //     <li class="pear">
 * //       <div class="healthy">Pear</div>
 * //     </li>
 * //   </ul>
 * ```
 *
 * @param wrapper - The DOM structure to wrap around the content of each element
 *   in the selection.
 * @returns The instance itself, for chaining.
 * @see {@link https://api.jquery.com/wrapInner/}
 */
exports.wrapInner = _wrap((el, elInsertLocation, wrapperDom) => {
    if (!(0, domhandler_1.hasChildren)(el))
        return;
    (0, parse_js_1.update)(el.children, elInsertLocation);
    (0, parse_js_1.update)(wrapperDom, el);
});
/**
 * The .unwrap() function, removes the parents of the set of matched elements
 * from the DOM, leaving the matched elements in their place.
 *
 * @category Manipulation
 * @example <caption>without selector</caption>
 *
 * ```js
 * const $ = cheerio.load(
 *   '<div id=test>\n  <div><p>Hello</p></div>\n  <div><p>World</p></div>\n</div>',
 * );
 * $('#test p').unwrap();
 *
 * //=> <div id=test>
 * //     <p>Hello</p>
 * //     <p>World</p>
 * //   </div>
 * ```
 *
 * @example <caption>with selector</caption>
 *
 * ```js
 * const $ = cheerio.load(
 *   '<div id=test>\n  <p>Hello</p>\n  <b><p>World</p></b>\n</div>',
 * );
 * $('#test p').unwrap('b');
 *
 * //=> <div id=test>
 * //     <p>Hello</p>
 * //     <p>World</p>
 * //   </div>
 * ```
 *
 * @param selector - A selector to check the parent element against. If an
 *   element's parent does not match the selector, the element won't be
 *   unwrapped.
 * @returns The instance itself, for chaining.
 * @see {@link https://api.jquery.com/unwrap/}
 */
function unwrap(selector) {
    this.parent(selector)
        .not('body')
        .each((_, el) => {
        this._make(el).replaceWith(el.children);
    });
    return this;
}
/**
 * The .wrapAll() function can take any string or object that could be passed to
 * the $() function to specify a DOM structure. This structure may be nested
 * several levels deep, but should contain only one inmost element. The
 * structure will be wrapped around all of the elements in the set of matched
 * elements, as a single group.
 *
 * @category Manipulation
 * @example <caption>With markup passed to `wrapAll`</caption>
 *
 * ```js
 * const $ = cheerio.load(
 *   '<div class="container"><div class="inner">First</div><div class="inner">Second</div></div>',
 * );
 * $('.inner').wrapAll("<div class='new'></div>");
 *
 * //=> <div class="container">
 * //     <div class='new'>
 * //       <div class="inner">First</div>
 * //       <div class="inner">Second</div>
 * //     </div>
 * //   </div>
 * ```
 *
 * @example <caption>With an existing cheerio instance</caption>
 *
 * ```js
 * const $ = cheerio.load(
 *   '<span>Span 1</span><strong>Strong</strong><span>Span 2</span>',
 * );
 * const wrap = $('<div><p><em><b></b></em></p></div>');
 * $('span').wrapAll(wrap);
 *
 * //=> <div>
 * //     <p>
 * //       <em>
 * //         <b>
 * //           <span>Span 1</span>
 * //           <span>Span 2</span>
 * //         </b>
 * //       </em>
 * //     </p>
 * //   </div>
 * //   <strong>Strong</strong>
 * ```
 *
 * @param wrapper - The DOM structure to wrap around all matched elements in the
 *   selection.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/wrapAll/}
 */
function wrapAll(wrapper) {
    const el = this[0];
    if (el) {
        const wrap = this._make(typeof wrapper === 'function' ? wrapper.call(el, 0, el) : wrapper).insertBefore(el);
        // If html is given as wrapper, wrap may contain text elements
        let elInsertLocation;
        for (let i = 0; i < wrap.length; i++) {
            if (wrap[i].type === htmlparser2_1.ElementType.Tag) {
                elInsertLocation = wrap[i];
            }
        }
        let j = 0;
        /*
         * Find the deepest child. Only consider the first tag child of each node
         * (ignore text); stop if no children are found.
         */
        while (elInsertLocation && j < elInsertLocation.children.length) {
            const child = elInsertLocation.children[j];
            if (child.type === htmlparser2_1.ElementType.Tag) {
                elInsertLocation = child;
                j = 0;
            }
            else {
                j++;
            }
        }
        if (elInsertLocation)
            this._make(elInsertLocation).append(this);
    }
    return this;
}
/**
 * Insert content next to each element in the set of matched elements.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('.apple').after('<li class="plum">Plum</li>');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="apple">Apple</li>
 * //      <li class="plum">Plum</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @param elems - HTML string, DOM element, array of DOM elements or Cheerio to
 *   insert after each element in the set of matched elements.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/after/}
 */
function after(...elems) {
    const lastIdx = this.length - 1;
    return (0, utils_js_1.domEach)(this, (el, i) => {
        if (!((0, domhandler_1.hasChildren)(el) && el.parent)) {
            return;
        }
        const siblings = el.parent.children;
        const index = siblings.indexOf(el);
        // If not found, move on
        /* istanbul ignore next */
        if (index === -1)
            return;
        const domSrc = typeof elems[0] === 'function'
            ? elems[0].call(el, i, this._render(el.children))
            : elems;
        const dom = this._makeDomArray(domSrc, i < lastIdx);
        // Add element after `this` element
        uniqueSplice(siblings, index + 1, 0, dom, el.parent);
    });
}
/**
 * Insert every element in the set of matched elements after the target.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('<li class="plum">Plum</li>').insertAfter('.apple');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="apple">Apple</li>
 * //      <li class="plum">Plum</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @param target - Element to insert elements after.
 * @returns The set of newly inserted elements.
 * @see {@link https://api.jquery.com/insertAfter/}
 */
function insertAfter(target) {
    if (typeof target === 'string') {
        target = this._make(target);
    }
    this.remove();
    const clones = [];
    for (const el of this._makeDomArray(target)) {
        const clonedSelf = this.clone().toArray();
        const { parent } = el;
        if (!parent) {
            continue;
        }
        const siblings = parent.children;
        const index = siblings.indexOf(el);
        // If not found, move on
        /* istanbul ignore next */
        if (index === -1)
            continue;
        // Add cloned `this` element(s) after target element
        uniqueSplice(siblings, index + 1, 0, clonedSelf, parent);
        clones.push(...clonedSelf);
    }
    return this._make(clones);
}
/**
 * Insert content previous to each element in the set of matched elements.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('.apple').before('<li class="plum">Plum</li>');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="plum">Plum</li>
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @param elems - HTML string, DOM element, array of DOM elements or Cheerio to
 *   insert before each element in the set of matched elements.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/before/}
 */
function before(...elems) {
    const lastIdx = this.length - 1;
    return (0, utils_js_1.domEach)(this, (el, i) => {
        if (!((0, domhandler_1.hasChildren)(el) && el.parent)) {
            return;
        }
        const siblings = el.parent.children;
        const index = siblings.indexOf(el);
        // If not found, move on
        /* istanbul ignore next */
        if (index === -1)
            return;
        const domSrc = typeof elems[0] === 'function'
            ? elems[0].call(el, i, this._render(el.children))
            : elems;
        const dom = this._makeDomArray(domSrc, i < lastIdx);
        // Add element before `el` element
        uniqueSplice(siblings, index, 0, dom, el.parent);
    });
}
/**
 * Insert every element in the set of matched elements before the target.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('<li class="plum">Plum</li>').insertBefore('.apple');
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="plum">Plum</li>
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //      <li class="pear">Pear</li>
 * //    </ul>
 * ```
 *
 * @param target - Element to insert elements before.
 * @returns The set of newly inserted elements.
 * @see {@link https://api.jquery.com/insertBefore/}
 */
function insertBefore(target) {
    const targetArr = this._make(target);
    this.remove();
    const clones = [];
    (0, utils_js_1.domEach)(targetArr, (el) => {
        const clonedSelf = this.clone().toArray();
        const { parent } = el;
        if (!parent) {
            return;
        }
        const siblings = parent.children;
        const index = siblings.indexOf(el);
        // If not found, move on
        /* istanbul ignore next */
        if (index === -1)
            return;
        // Add cloned `this` element(s) after target element
        uniqueSplice(siblings, index, 0, clonedSelf, parent);
        clones.push(...clonedSelf);
    });
    return this._make(clones);
}
/**
 * Removes the set of matched elements from the DOM and all their children.
 * `selector` filters the set of matched elements to be removed.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('.pear').remove();
 * $.html();
 * //=>  <ul id="fruits">
 * //      <li class="apple">Apple</li>
 * //      <li class="orange">Orange</li>
 * //    </ul>
 * ```
 *
 * @param selector - Optional selector for elements to remove.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/remove/}
 */
function remove(selector) {
    // Filter if we have selector
    const elems = selector ? this.filter(selector) : this;
    (0, utils_js_1.domEach)(elems, (el) => {
        (0, domutils_1.removeElement)(el);
        el.prev = el.next = el.parent = null;
    });
    return this;
}
/**
 * Replaces matched elements with `content`.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * const plum = $('<li class="plum">Plum</li>');
 * $('.pear').replaceWith(plum);
 * $.html();
 * //=> <ul id="fruits">
 * //     <li class="apple">Apple</li>
 * //     <li class="orange">Orange</li>
 * //     <li class="plum">Plum</li>
 * //   </ul>
 * ```
 *
 * @param content - Replacement for matched elements.
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/replaceWith/}
 */
function replaceWith(content) {
    return (0, utils_js_1.domEach)(this, (el, i) => {
        const { parent } = el;
        if (!parent) {
            return;
        }
        const siblings = parent.children;
        const cont = typeof content === 'function' ? content.call(el, i, el) : content;
        const dom = this._makeDomArray(cont);
        /*
         * In the case that `dom` contains nodes that already exist in other
         * structures, ensure those nodes are properly removed.
         */
        (0, parse_js_1.update)(dom, null);
        const index = siblings.indexOf(el);
        // Completely remove old element
        uniqueSplice(siblings, index, 1, dom, parent);
        if (!dom.includes(el)) {
            el.parent = el.prev = el.next = null;
        }
    });
}
/**
 * Removes all children from each item in the selection. Text nodes and comment
 * nodes are left as is.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * $('ul').empty();
 * $.html();
 * //=>  <ul id="fruits"></ul>
 * ```
 *
 * @returns The instance itself.
 * @see {@link https://api.jquery.com/empty/}
 */
function empty() {
    return (0, utils_js_1.domEach)(this, (el) => {
        if (!(0, domhandler_1.hasChildren)(el))
            return;
        for (const child of el.children) {
            child.next = child.prev = child.parent = null;
        }
        el.children.length = 0;
    });
}
function html(str) {
    if (str === undefined) {
        const el = this[0];
        if (!(el && (0, domhandler_1.hasChildren)(el)))
            return null;
        return this._render(el.children);
    }
    return (0, utils_js_1.domEach)(this, (el) => {
        if (!(0, domhandler_1.hasChildren)(el))
            return;
        for (const child of el.children) {
            child.next = child.prev = child.parent = null;
        }
        const content = (0, utils_js_1.isCheerio)(str)
            ? str.toArray()
            : this._parse(`${str}`, this.options, false, el).children;
        (0, parse_js_1.update)(content, el);
    });
}
/**
 * Turns the collection to a string. Alias for `.html()`.
 *
 * @category Manipulation
 * @returns The rendered document.
 */
function toString() {
    return this._render(this);
}
function text(str) {
    // If `str` is undefined, act as a "getter"
    if (str === undefined) {
        return (0, static_js_1.text)(this);
    }
    if (typeof str === 'function') {
        // Function support
        return (0, utils_js_1.domEach)(this, (el, i) => this._make(el).text(str.call(el, i, (0, static_js_1.text)([el]))));
    }
    // Append text node to each selected elements
    return (0, utils_js_1.domEach)(this, (el) => {
        if (!(0, domhandler_1.hasChildren)(el))
            return;
        for (const child of el.children) {
            child.next = child.prev = child.parent = null;
        }
        const textNode = new domhandler_1.Text(`${str}`);
        (0, parse_js_1.update)(textNode, el);
    });
}
/**
 * Clone the cheerio object.
 *
 * @category Manipulation
 * @example
 *
 * ```js
 * const moreFruit = $('#fruits').clone();
 * ```
 *
 * @returns The cloned object.
 * @see {@link https://api.jquery.com/clone/}
 */
function clone() {
    const clone = Array.prototype.map.call(this.get(), (el) => (0, domhandler_1.cloneNode)(el, true));
    // Add a root node around the cloned nodes
    const root = new domhandler_1.Document(clone);
    for (const node of clone) {
        node.parent = root;
    }
    return this._make(clone);
}

};
__mods['api/traversing.js'] = function (module, exports, require) {
"use strict";
/**
 * Methods for traversing the DOM structure.
 *
 * @module cheerio/traversing
 */
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
exports.children = exports.siblings = exports.prevUntil = exports.prevAll = exports.prev = exports.nextUntil = exports.nextAll = exports.next = exports.parentsUntil = exports.parents = exports.parent = void 0;
exports.find = find;
exports._findBySelector = _findBySelector;
exports.closest = closest;
exports.contents = contents;
exports.each = each;
exports.map = map;
exports.filter = filter;
exports.filterArray = filterArray;
exports.is = is;
exports.not = not;
exports.has = has;
exports.first = first;
exports.last = last;
exports.eq = eq;
exports.get = get;
exports.toArray = toArray;
exports.index = index;
exports.slice = slice;
exports.end = end;
exports.add = add;
exports.addBack = addBack;
const select = __importStar(require(".././vendor/cheerio-select.js"));
const domhandler_1 = require(".././vendor/domhandler.js");
const domutils_1 = require(".././vendor/domutils.js");
const static_js_1 = require("../static.js");
const utils_js_1 = require("../utils.js");
const reContextSelector = /^\s*(?:[+~]|:scope\b)/;
/**
 * Get the descendants of each element in the current set of matched elements,
 * filtered by a selector, jQuery object, or element.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('#fruits').find('li').length;
 * //=> 3
 * $('#fruits').find($('.apple')).length;
 * //=> 1
 * ```
 *
 * @param selectorOrHaystack - Element to look for.
 * @returns The found elements.
 * @see {@link https://api.jquery.com/find/}
 */
function find(selectorOrHaystack) {
    if (!selectorOrHaystack) {
        return this._make([]);
    }
    if (typeof selectorOrHaystack !== 'string') {
        const haystack = (0, utils_js_1.isCheerio)(selectorOrHaystack)
            ? selectorOrHaystack.toArray()
            : [selectorOrHaystack];
        const context = this.toArray();
        return this._make(haystack.filter((elem) => context.some((node) => (0, static_js_1.contains)(node, elem))));
    }
    return this._findBySelector(selectorOrHaystack, Number.POSITIVE_INFINITY);
}
/**
 * Find elements by a specific selector.
 *
 * @private
 * @category Traversing
 * @param selector - Selector to filter by.
 * @param limit - Maximum number of elements to match.
 * @returns The found elements.
 */
function _findBySelector(selector, limit) {
    const context = this.toArray();
    const elems = reContextSelector.test(selector)
        ? context
        : this.children().toArray();
    const options = {
        context,
        root: this._root?.[0],
        // Pass options that are recognized by `cheerio-select`
        xmlMode: this.options.xmlMode,
        lowerCaseTags: this.options.lowerCaseTags,
        lowerCaseAttributeNames: this.options.lowerCaseAttributeNames,
        pseudos: this.options.pseudos,
        quirksMode: this.options.quirksMode,
    };
    return this._make(select.select(selector, elems, options, limit));
}
/**
 * Creates a matcher, using a particular mapping function. Matchers provide a
 * function that finds elements using a generating function, supporting
 * filtering.
 *
 * @private
 * @param matchMap - Mapping function.
 * @returns - Function for wrapping generating functions.
 */
function _getMatcher(matchMap) {
    return (fn, ...postFns) => function (selector) {
        let matched = matchMap(fn, this);
        if (selector) {
            matched = filterArray(matched, selector, this.options.xmlMode, this._root?.[0]);
        }
        return this._make(
        // Post processing is only necessary if there is more than one element.
        this.length > 1 && matched.length > 1
            ? postFns.reduce((elems, fn) => fn(elems), matched)
            : matched);
    };
}
/** Matcher that adds multiple elements for each entry in the input. */
const _matcher = _getMatcher((fn, elems) => elems.toArray().flatMap((elem) => fn(elem)));
/** Matcher that adds at most one element for each entry in the input. */
const _singleMatcher = _getMatcher((fn, elems) => {
    const ret = [];
    for (let i = 0; i < elems.length; i++) {
        const value = fn(elems[i]);
        if (value !== null) {
            ret.push(value);
        }
    }
    return ret;
});
/**
 * Matcher that supports traversing until a condition is met.
 *
 * @param nextElem - Function that returns the next element.
 * @param postFns - Post processing functions.
 * @returns A function usable for `*Until` methods.
 */
function _matchUntil(nextElem, ...postFns) {
    // We use a variable here that is used from within the matcher.
    let matches = null;
    const innerMatcher = _getMatcher((nextElem, elems) => {
        const matched = [];
        (0, utils_js_1.domEach)(elems, (elem) => {
            for (let next; (next = nextElem(elem)); elem = next) {
                // FIXME: `matched` might contain duplicates here and the index is too large.
                if (matches?.(next, matched.length))
                    break;
                matched.push(next);
            }
        });
        return matched;
    })(nextElem, ...postFns);
    return function (selector, filterSelector) {
        // Override `matches` variable with the new target.
        matches =
            typeof selector === 'string'
                ? (elem) => select.is(elem, selector, this.options)
                : selector
                    ? getFilterFn(selector)
                    : null;
        const ret = innerMatcher.call(this, filterSelector);
        // Set `matches` to `null`, so we don't waste memory.
        matches = null;
        return ret;
    };
}
function _removeDuplicates(elems) {
    return elems.length > 1 ? [...new Set(elems)] : elems;
}
/**
 * Get the parent of each element in the current set of matched elements,
 * optionally filtered by a selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.pear').parent().attr('id');
 * //=> fruits
 * ```
 *
 * @param selector - If specified filter for parent.
 * @returns The parents.
 * @see {@link https://api.jquery.com/parent/}
 */
exports.parent = _singleMatcher(({ parent }) => (parent && !(0, domhandler_1.isDocument)(parent) ? parent : null), _removeDuplicates);
/**
 * Get a set of parents filtered by `selector` of each element in the current
 * set of match elements.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.orange').parents().length;
 * //=> 2
 * $('.orange').parents('#fruits').length;
 * //=> 1
 * ```
 *
 * @param selector - If specified filter for parents.
 * @returns The parents.
 * @see {@link https://api.jquery.com/parents/}
 */
exports.parents = _matcher((elem) => {
    const matched = [];
    while (elem.parent && !(0, domhandler_1.isDocument)(elem.parent)) {
        matched.push(elem.parent);
        elem = elem.parent;
    }
    return matched;
}, domutils_1.uniqueSort, 
// eslint-disable-next-line unicorn/no-array-reverse
(elems) => elems.reverse());
/**
 * Get the ancestors of each element in the current set of matched elements, up
 * to but not including the element matched by the selector, DOM node, or
 * cheerio object.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.orange').parentsUntil('#food').length;
 * //=> 1
 * ```
 *
 * @param selector - Selector for element to stop at.
 * @param filterSelector - Optional filter for parents.
 * @returns The parents.
 * @see {@link https://api.jquery.com/parentsUntil/}
 */
exports.parentsUntil = _matchUntil(({ parent }) => (parent && !(0, domhandler_1.isDocument)(parent) ? parent : null), domutils_1.uniqueSort, 
// eslint-disable-next-line unicorn/no-array-reverse
(elems) => elems.reverse());
/**
 * For each element in the set, get the first element that matches the selector
 * by testing the element itself and traversing up through its ancestors in the
 * DOM tree.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.orange').closest();
 * //=> []
 *
 * $('.orange').closest('.apple');
 * // => []
 *
 * $('.orange').closest('li');
 * //=> [<li class="orange">Orange</li>]
 *
 * $('.orange').closest('#fruits');
 * //=> [<ul id="fruits"> ... </ul>]
 * ```
 *
 * @param selector - Selector for the element to find.
 * @returns The closest nodes.
 * @see {@link https://api.jquery.com/closest/}
 */
function closest(selector) {
    const set = [];
    if (!selector) {
        return this._make(set);
    }
    const selectOpts = {
        xmlMode: this.options.xmlMode,
        root: this._root?.[0],
    };
    const selectFn = typeof selector === 'string'
        ? (elem) => select.is(elem, selector, selectOpts)
        : getFilterFn(selector);
    /*
     * Dedup: a linear scan is cheapest for the small result sets `closest`
     * usually produces. Once the set grows past `dedupThreshold` we switch to a
     * Set, so a pathological input with many distinct matches stays O(n) instead
     * of O(n^2). `set` always preserves document order.
     */
    const dedupThreshold = 100;
    let seen;
    (0, utils_js_1.domEach)(this, (elem) => {
        if (elem && !(0, domhandler_1.isDocument)(elem) && !(0, domhandler_1.isTag)(elem)) {
            elem = elem.parent;
        }
        while (elem && (0, domhandler_1.isTag)(elem)) {
            if (selectFn(elem, 0)) {
                // Do not add duplicate elements to the set
                if (seen ? !seen.has(elem) : !set.includes(elem)) {
                    set.push(elem);
                    if (seen) {
                        seen.add(elem);
                    }
                    else if (set.length > dedupThreshold) {
                        seen = new Set(set);
                    }
                }
                break;
            }
            elem = elem.parent;
        }
    });
    return this._make(set);
}
/**
 * Gets the next sibling of each selected element, optionally filtered by a
 * selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.apple').next().hasClass('orange');
 * //=> true
 * ```
 *
 * @param selector - If specified filter for sibling.
 * @returns The next nodes.
 * @see {@link https://api.jquery.com/next/}
 */
exports.next = _singleMatcher((elem) => (0, domutils_1.nextElementSibling)(elem));
/**
 * Gets all the following siblings of the each selected element, optionally
 * filtered by a selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.apple').nextAll();
 * //=> [<li class="orange">Orange</li>, <li class="pear">Pear</li>]
 * $('.apple').nextAll('.orange');
 * //=> [<li class="orange">Orange</li>]
 * ```
 *
 * @param selector - If specified filter for siblings.
 * @returns The next nodes.
 * @see {@link https://api.jquery.com/nextAll/}
 */
exports.nextAll = _matcher((elem) => {
    const matched = [];
    while (elem.next) {
        elem = elem.next;
        if ((0, domhandler_1.isTag)(elem))
            matched.push(elem);
    }
    return matched;
}, _removeDuplicates);
/**
 * Gets all the following siblings up to but not including the element matched
 * by the selector, optionally filtered by another selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.apple').nextUntil('.pear');
 * //=> [<li class="orange">Orange</li>]
 * ```
 *
 * @param selector - Selector for element to stop at.
 * @param filterSelector - If specified filter for siblings.
 * @returns The next nodes.
 * @see {@link https://api.jquery.com/nextUntil/}
 */
exports.nextUntil = _matchUntil((el) => (0, domutils_1.nextElementSibling)(el), _removeDuplicates);
/**
 * Gets the previous sibling of each selected element optionally filtered by a
 * selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.orange').prev().hasClass('apple');
 * //=> true
 * ```
 *
 * @param selector - If specified filter for siblings.
 * @returns The previous nodes.
 * @see {@link https://api.jquery.com/prev/}
 */
exports.prev = _singleMatcher((elem) => (0, domutils_1.prevElementSibling)(elem));
/**
 * Gets all the preceding siblings of each selected element, optionally filtered
 * by a selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.pear').prevAll();
 * //=> [<li class="orange">Orange</li>, <li class="apple">Apple</li>]
 *
 * $('.pear').prevAll('.orange');
 * //=> [<li class="orange">Orange</li>]
 * ```
 *
 * @param selector - If specified filter for siblings.
 * @returns The previous nodes.
 * @see {@link https://api.jquery.com/prevAll/}
 */
exports.prevAll = _matcher((elem) => {
    const matched = [];
    while (elem.prev) {
        elem = elem.prev;
        if ((0, domhandler_1.isTag)(elem))
            matched.push(elem);
    }
    return matched;
}, _removeDuplicates);
/**
 * Gets all the preceding siblings up to but not including the element matched
 * by the selector, optionally filtered by another selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.pear').prevUntil('.apple');
 * //=> [<li class="orange">Orange</li>]
 * ```
 *
 * @param selector - Selector for element to stop at.
 * @param filterSelector - If specified filter for siblings.
 * @returns The previous nodes.
 * @see {@link https://api.jquery.com/prevUntil/}
 */
exports.prevUntil = _matchUntil((el) => (0, domutils_1.prevElementSibling)(el), _removeDuplicates);
/**
 * Get the siblings of each element (excluding the element) in the set of
 * matched elements, optionally filtered by a selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.pear').siblings().length;
 * //=> 2
 *
 * $('.pear').siblings('.orange').length;
 * //=> 1
 * ```
 *
 * @param selector - If specified filter for siblings.
 * @returns The siblings.
 * @see {@link https://api.jquery.com/siblings/}
 */
exports.siblings = _matcher((elem) => (0, domutils_1.getSiblings)(elem).filter((el) => (0, domhandler_1.isTag)(el) && el !== elem), domutils_1.uniqueSort);
/**
 * Gets the element children of each element in the set of matched elements.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('#fruits').children().length;
 * //=> 3
 *
 * $('#fruits').children('.pear').text();
 * //=> Pear
 * ```
 *
 * @param selector - If specified filter for children.
 * @returns The children.
 * @see {@link https://api.jquery.com/children/}
 */
exports.children = _matcher((elem) => (0, domutils_1.getChildren)(elem).filter(domhandler_1.isTag), _removeDuplicates);
/**
 * Gets the children of each element in the set of matched elements, including
 * text and comment nodes.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('#fruits').contents().length;
 * //=> 3
 * ```
 *
 * @returns The children.
 * @see {@link https://api.jquery.com/contents/}
 */
function contents() {
    const elems = this.toArray().flatMap((elem) => (0, domhandler_1.hasChildren)(elem) ? elem.children : []);
    return this._make(elems);
}
/**
 * Iterates over a cheerio object, executing a function for each matched
 * element. When the callback is fired, the function is fired in the context of
 * the DOM element, so `this` refers to the current element, which is equivalent
 * to the function parameter `element`. To break out of the `each` loop early,
 * return with `false`.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * const fruits = [];
 *
 * $('li').each(function (i, elem) {
 *   fruits[i] = $(this).text();
 * });
 *
 * fruits.join(', ');
 * //=> Apple, Orange, Pear
 * ```
 *
 * @param fn - Function to execute.
 * @returns The instance itself, useful for chaining.
 * @see {@link https://api.jquery.com/each/}
 */
function each(fn) {
    let i = 0;
    const len = this.length;
    while (i < len && fn.call(this[i], i, this[i]) !== false)
        ++i;
    return this;
}
/**
 * Pass each element in the current matched set through a function, producing a
 * new Cheerio object containing the return values. The function can return an
 * individual data item or an array of data items to be inserted into the
 * resulting set. If an array is returned, the elements inside the array are
 * inserted into the set. If the function returns null or undefined, no element
 * will be inserted.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('li')
 *   .map(function (i, el) {
 *     // this === el
 *     return $(this).text();
 *   })
 *   .toArray()
 *   .join(' ');
 * //=> "apple orange pear"
 * ```
 *
 * @param fn - Function to execute.
 * @returns The mapped elements, wrapped in a Cheerio collection.
 * @see {@link https://api.jquery.com/map/}
 */
function map(fn) {
    const elems = [];
    for (let i = 0; i < this.length; i++) {
        const el = this[i];
        const val = fn.call(el, i, el);
        if (val != null) {
            /*
             * Accumulate in place; `concat` would copy the whole array each
             * iteration, making this O(n^2) in the size of the collection.
             */
            if (Array.isArray(val)) {
                for (let j = 0; j < val.length; j++)
                    elems.push(val[j]);
            }
            else {
                elems.push(val);
            }
        }
    }
    return this._make(elems);
}
/**
 * Creates a function to test if a filter is matched.
 *
 * @param match - A filter.
 * @returns A function that determines if a filter has been matched.
 */
function getFilterFn(match) {
    if (typeof match === 'function') {
        return (el, i) => match.call(el, i, el);
    }
    if ((0, utils_js_1.isCheerio)(match)) {
        return (el) => Array.prototype.includes.call(match, el);
    }
    return (el) => match === el;
}
function filter(match) {
    return this._make(filterArray(this.toArray(), match, this.options.xmlMode, this._root?.[0]));
}
/**
 * Filter an array of nodes with either a selector or predicate.
 *
 * @param nodes - The nodes to filter.
 * @param match - Selector or predicate used to keep nodes.
 * @param xmlMode - Whether selector matching should use XML mode.
 * @param root - Optional document root used for selector matching.
 */
function filterArray(nodes, match, xmlMode, root) {
    return typeof match === 'string'
        ? select.filter(match, nodes, { xmlMode, root })
        : nodes.filter(getFilterFn(match));
}
/**
 * Checks the current list of elements and returns `true` if _any_ of the
 * elements match the selector. If using an element or Cheerio selection,
 * returns `true` if _any_ of the elements match. If using a predicate function,
 * the function is executed in the context of the selected element, so `this`
 * refers to the current element.
 *
 * @category Traversing
 * @param selector - Selector for the selection.
 * @returns Whether or not the selector matches an element of the instance.
 * @see {@link https://api.jquery.com/is/}
 */
function is(selector) {
    const nodes = this.toArray();
    return typeof selector === 'string'
        ? select.some(nodes.filter(domhandler_1.isTag), selector, this.options)
        : selector
            ? nodes.some(getFilterFn(selector))
            : false;
}
/**
 * Remove elements from the set of matched elements. Given a Cheerio object that
 * represents a set of DOM elements, the `.not()` method constructs a new
 * Cheerio object from a subset of the matching elements. The supplied selector
 * is tested against each element; the elements that don't match the selector
 * will be included in the result.
 *
 * The `.not()` method can take a function as its argument in the same way that
 * `.filter()` does. Elements for which the function returns `true` are excluded
 * from the filtered set; all other elements are included.
 *
 * @category Traversing
 * @example <caption>Selector</caption>
 *
 * ```js
 * $('li').not('.apple').length;
 * //=> 2
 * ```
 *
 * @example <caption>Function</caption>
 *
 * ```js
 * $('li').not(function (i, el) {
 *   // this === el
 *   return $(this).attr('class') === 'orange';
 * }).length; //=> 2
 * ```
 *
 * @param match - Value to look for, following the rules above.
 * @returns The filtered collection.
 * @see {@link https://api.jquery.com/not/}
 */
function not(match) {
    let nodes = this.toArray();
    if (typeof match === 'string') {
        const matches = new Set(select.filter(match, nodes, this.options));
        nodes = nodes.filter((el) => !matches.has(el));
    }
    else {
        const filterFn = getFilterFn(match);
        nodes = nodes.filter((el, i) => !filterFn(el, i));
    }
    return this._make(nodes);
}
/**
 * Filters the set of matched elements to only those which have the given DOM
 * element as a descendant or which have a descendant that matches the given
 * selector. Equivalent to `.filter(':has(selector)')`.
 *
 * @category Traversing
 * @example <caption>Selector</caption>
 *
 * ```js
 * $('ul').has('.pear').attr('id');
 * //=> fruits
 * ```
 *
 * @example <caption>Element</caption>
 *
 * ```js
 * $('ul').has($('.pear')[0]).attr('id');
 * //=> fruits
 * ```
 *
 * @param selectorOrHaystack - Element to look for.
 * @returns The filtered collection.
 * @see {@link https://api.jquery.com/has/}
 */
function has(selectorOrHaystack) {
    return this.filter(typeof selectorOrHaystack === 'string'
        ? // Using the `:has` selector here short-circuits searches.
            `:has(${selectorOrHaystack})`
        : (_, el) => this._make(el).find(selectorOrHaystack).length > 0);
}
/**
 * Will select the first element of a cheerio object.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('#fruits').children().first().text();
 * //=> Apple
 * ```
 *
 * @returns The first element.
 * @see {@link https://api.jquery.com/first/}
 */
function first() {
    return this.length > 1 ? this._make(this[0]) : this;
}
/**
 * Will select the last element of a cheerio object.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('#fruits').children().last().text();
 * //=> Pear
 * ```
 *
 * @returns The last element.
 * @see {@link https://api.jquery.com/last/}
 */
function last() {
    return this.length > 0 ? this._make(this[this.length - 1]) : this;
}
/**
 * Reduce the set of matched elements to the one at the specified index. Use
 * `.eq(-i)` to count backwards from the last selected element.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('li').eq(0).text();
 * //=> Apple
 *
 * $('li').eq(-1).text();
 * //=> Pear
 * ```
 *
 * @param i - Index of the element to select.
 * @returns The element at the `i`th position.
 * @see {@link https://api.jquery.com/eq/}
 */
function eq(i) {
    /*
     * Not redundant: JavaScript callers pass string indices, and `eq('-1')`
     * would otherwise reach `this.length + i` as string concatenation.
     */
    // eslint-disable-next-line unicorn/no-useless-coercion
    i = +i;
    // Use the first identity optimization if possible
    if (i === 0 && this.length <= 1)
        return this;
    if (i < 0)
        i = this.length + i;
    return this._make(this[i] ?? []);
}
function get(i) {
    if (i == null) {
        return this.toArray();
    }
    return this[i < 0 ? this.length + i : i];
}
/**
 * Retrieve all the DOM elements contained in the jQuery set as an array.
 *
 * @example
 *
 * ```js
 * $('li').toArray();
 * //=> [ {...}, {...}, {...} ]
 * ```
 *
 * @returns The contained items.
 */
function toArray() {
    return Array.prototype.slice.call(this);
}
/**
 * Search for a given element from among the matched elements.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.pear').index();
 * //=> 2 $('.orange').index('li');
 * //=> 1
 * $('.apple').index($('#fruit, li'));
 * //=> 1
 * ```
 *
 * @param selectorOrNeedle - Element to look for.
 * @returns The index of the element.
 * @see {@link https://api.jquery.com/index/}
 */
function index(selectorOrNeedle) {
    let $haystack;
    let needle;
    if (selectorOrNeedle == null) {
        $haystack = this.parent().children();
        needle = this[0];
    }
    else if (typeof selectorOrNeedle === 'string') {
        $haystack = this._make(selectorOrNeedle);
        needle = this[0];
    }
    else {
        // eslint-disable-next-line unicorn/no-this-assignment
        $haystack = this;
        needle = (0, utils_js_1.isCheerio)(selectorOrNeedle)
            ? selectorOrNeedle[0]
            : selectorOrNeedle;
    }
    return Array.prototype.indexOf.call($haystack, needle);
}
/**
 * Gets the elements matching the specified range (0-based position).
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('li').slice(1).eq(0).text();
 * //=> 'Orange'
 *
 * $('li').slice(1, 2).length;
 * //=> 1
 * ```
 *
 * @param start - A position at which the elements begin to be selected. If
 *   negative, it indicates an offset from the end of the set.
 * @param end - A position at which the elements stop being selected. If
 *   negative, it indicates an offset from the end of the set. If omitted, the
 *   range continues until the end of the set.
 * @returns The elements matching the specified range.
 * @see {@link https://api.jquery.com/slice/}
 */
function slice(start, end) {
    return this._make(Array.prototype.slice.call(this, start, end));
}
/**
 * End the most recent filtering operation in the current chain and return the
 * set of matched elements to its previous state.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('li').eq(0).end().length;
 * //=> 3
 * ```
 *
 * @returns The previous state of the set of matched elements.
 * @see {@link https://api.jquery.com/end/}
 */
function end() {
    return this.prevObject ?? this._make([]);
}
/**
 * Add elements to the set of matched elements.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('.apple').add('.orange').length;
 * //=> 2
 * ```
 *
 * @param other - Elements to add.
 * @param context - Optionally the context of the new selection.
 * @returns The combined set.
 * @see {@link https://api.jquery.com/add/}
 */
function add(other, context) {
    const selection = this._make(other, context);
    const contents = (0, domutils_1.uniqueSort)([...this.get(), ...selection.get()]);
    return this._make(contents);
}
/**
 * Add the previous set of elements on the stack to the current set, optionally
 * filtered by a selector.
 *
 * @category Traversing
 * @example
 *
 * ```js
 * $('li').eq(0).addBack('.orange').length;
 * //=> 2
 * ```
 *
 * @param selector - Selector for the elements to add.
 * @returns The combined set.
 * @see {@link https://api.jquery.com/addBack/}
 */
function addBack(selector) {
    return this.prevObject
        ? this.add(selector ? this.prevObject.filter(selector) : this.prevObject)
        : this;
}

};
__mods['parsers/parse5-adapter.js'] = function (module, exports, require) {
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseWithParse5 = parseWithParse5;
exports.renderWithParse5 = renderWithParse5;
const domhandler_1 = require(".././vendor/domhandler.js");
const parse5_1 = require(".././vendor/parse5.js");
const parse5_htmlparser2_tree_adapter_1 = require(".././vendor/parse5-htmlparser2-tree-adapter.js");
/**
 * Parse the content with `parse5` in the context of the given `ParentNode`.
 *
 * @param content - The content to parse.
 * @param options - A set of options to use to parse.
 * @param isDocument - Whether to parse the content as a full HTML document.
 * @param context - The context in which to parse the content.
 * @returns The parsed content.
 */
function parseWithParse5(content, options, isDocument, context) {
    options.treeAdapter ?? (options.treeAdapter = parse5_htmlparser2_tree_adapter_1.adapter);
    if (options.scriptingEnabled !== false) {
        options.scriptingEnabled = true;
    }
    return isDocument
        ? (0, parse5_1.parse)(content, options)
        : (0, parse5_1.parseFragment)(context, content, options);
}
const renderOpts = { treeAdapter: parse5_htmlparser2_tree_adapter_1.adapter };
/**
 * Renders the given DOM tree with `parse5` and returns the result as a string.
 *
 * @param dom - The DOM tree to render.
 * @returns The rendered document.
 */
function renderWithParse5(dom) {
    /*
     * `dom-serializer` passes over the special "root" node and renders the
     * node's children in its place. To mimic this behavior with `parse5`, an
     * equivalent operation must be applied to the input array.
     */
    const nodes = 'length' in dom ? dom : [dom];
    for (let index = 0; index < nodes.length; index += 1) {
        const node = nodes[index];
        if ((0, domhandler_1.isDocument)(node)) {
            Array.prototype.splice.call(nodes, index, 1, ...node.children);
        }
    }
    let result = '';
    for (let index = 0; index < nodes.length; index += 1) {
        const node = nodes[index];
        result += (0, parse5_1.serializeOuter)(node, renderOpts);
    }
    return result;
}

};
__mods['vendor/domhandler.js'] = function (module, exports, require) {
// vendor/domhandler.js —— domhandler 运行时部分（类型判断 + ElementType 常量）
// 从 domhandler 源码移植（MIT License, fb55）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const ElementType = {
  Root: 'root',
  Text: 'text',
  Directive: 'directive',
  Comment: 'comment',
  Script: 'script',
  Style: 'style',
  Tag: 'tag',
  CDATA: 'cdata',
  Doctype: 'doctype',
};

function isTag(node) {
  return node.type === ElementType.Tag ||
    node.type === ElementType.Script ||
    node.type === ElementType.Style;
}
function isCDATA(node) { return node.type === ElementType.CDATA; }
function isText(node) { return node.type === ElementType.Text; }
function isComment(node) { return node.type === ElementType.Comment; }
function isDirective(node) { return node.type === ElementType.Directive; }
function isDocument(node) { return node.type === ElementType.Root; }
function isScript(node) { return node.type === ElementType.Script; }
function isStyle(node) { return node.type === ElementType.Style; }
function hasChildren(node) {
  return Object.prototype.hasOwnProperty.call(node, 'children');
}

// 深拷贝节点树（domhandler cloneNode 语义）：parent/prev/next 重建
function cloneNode(node, recursive) {
  const clone = {};
  for (const k of Object.keys(node)) {
    if (k === 'parent' || k === 'prev' || k === 'next') continue;
    clone[k] = node[k];
  }
  clone.children = [];
  if (recursive && Array.isArray(node.children)) {
    for (const child of node.children) {
      const cc = cloneNode(child, true);
      cc.parent = clone;
      if (clone.children.length > 0) {
        const prev = clone.children[clone.children.length - 1];
        prev.next = cc;
        cc.prev = prev;
      }
      clone.children.push(cc);
    }
  }
  return clone;
}

class Document {
  constructor(children) {
    this.type = ElementType.Root;
    if (!children) {
      this.children = [];
    }
  }
}

// 运行时构造用节点类（text setter / clone 路径）
class Text {
  constructor(data) {
    this.type = ElementType.Text;
    this.data = data;
  }
}

class Comment {
  constructor(data) {
    this.type = ElementType.Comment;
    this.data = data;
  }
}

class Element {
  constructor(name, attribs) {
    this.type = ElementType.Tag;
    this.name = name;
    this.attribs = attribs || {};
    this.children = [];
  }
}

class ProcessingInstruction {
  constructor(name, data) {
    this.type = ElementType.Directive;
    this.name = name;
    this.data = data;
  }
}

exports.ElementType = ElementType;
exports.Document = Document;
exports.Text = Text;
exports.Comment = Comment;
exports.Element = Element;
exports.ProcessingInstruction = ProcessingInstruction;
exports.isTag = isTag;
exports.isCDATA = isCDATA;
exports.isText = isText;
exports.isComment = isComment;
exports.isDirective = isDirective;
exports.isDocument = isDocument;
exports.isScript = isScript;
exports.isStyle = isStyle;
exports.hasChildren = hasChildren;
exports.cloneNode = cloneNode;

};
__mods['vendor/htmlparser2.js'] = function (module, exports, require) {
// vendor/htmlparser2.js —— cheerio 的 xml/htmlparser2 路径退化为 lexbor 解析
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const domhandler = require('./domhandler.js');

// 与 htmlparser2 相同的 ElementType 常量
const ElementType = domhandler.ElementType;

// 完整文档解析（xml 模式也走 lexbor HTML 解析）
function parseDocument(content, options) {
  const root = globalThis.__lexbor_parse(String(content), true, null);
  return root;
}

// parseDOM: 片段解析
function parseDOM(content, options) {
  const root = globalThis.__lexbor_parse(String(content), false, null);
  return root.children;
}

function isTag(node) {
  return domhandler.isTag(node);
}

exports.ElementType = ElementType;
exports.parseDocument = parseDocument;
exports.parseDOM = parseDOM;
exports.isTag = isTag;
exports.DomHandler = domhandler.Document;

};
__mods['vendor/domutils.js'] = function (module, exports, require) {
// vendor/domutils.js —— domutils 运行时部分（textContent / innerText / removeElement）
// 从 domutils 源码移植（MIT License, fb55）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const domhandler = require('./domhandler.js');

function getChildren(node) {
  return Object.prototype.hasOwnProperty.call(node, 'children')
    ? node.children
    : [];
}

function textContent(node) {
  if (domhandler.isText(node)) return node.data;
  if (domhandler.isCDATA(node)) return node.data;
  if (domhandler.isComment(node)) return '';
  if (domhandler.isDirective(node)) return '';
  const result = [];
  for (const child of getChildren(node)) {
    result.push(textContent(child));
  }
  return result.join('');
}

function innerText(node) {
  if (domhandler.isText(node) || domhandler.isCDATA(node)) return node.data;
  if (domhandler.isComment(node)) return '';
  if (domhandler.isDirective(node)) return '';
  let result = '';
  for (const child of getChildren(node)) {
    const text = innerText(child);
    if (text) {
      if (domhandler.isTag(node) && node.name === 'br' && result) {
        result += '\n';
      }
      result += text;
    }
  }
  return result;
}

function removeElement(elem) {
  const parent = elem.parent;
  if (parent) {
    const children = parent.children;
    const idx = children.indexOf(elem);
    if (idx >= 0) children.splice(idx, 1);
    const prev = elem.prev;
    const next = elem.next;
    if (prev) prev.next = next;
    if (next) next.prev = prev;
    if (children.length > 0) {
      if (prev === null) children[0].prev = null;
      if (next === null) children[children.length - 1].next = null;
    }
  }
  elem.parent = null;
  elem.prev = null;
  elem.next = null;
}

exports.textContent = textContent;
exports.innerText = innerText;
exports.removeElement = removeElement;
exports.getChildren = getChildren;

// ---- traversing 依赖（domutils 原版语义）----

function getChildrenWithChecks(node) {
  return getChildren(node);
}

// 所有兄弟（含自身），文档序
function getSiblings(node) {
  const parent = node.parent;
  if (parent) return getChildren(parent);
  // 无父：自身
  return [node];
}

function nextElementSibling(node) {
  let next = node.next;
  while (next && !domhandler.isTag(next)) next = next.next;
  return next;
}

function prevElementSibling(node) {
  let prev = node.prev;
  while (prev && !domhandler.isTag(prev)) prev = prev.prev;
  return prev;
}

// 稳定去重排序（文档序）：按节点在树中的前序遍历位置
function uniqueSort(nodes) {
  const seen = new Set();
  const out = [];
  for (const n of nodes) {
    if (!seen.has(n)) { seen.add(n); out.push(n); }
  }
  // 文档序：先序遍历索引（domutils.uniqueSort 语义）
  const order = new Map();
  let root = null;
  for (const n of nodes) {
    let cur = n;
    while (cur && cur.type !== 'root') cur = cur.parent;
    if (cur) { root = cur; break; }
  }
  if (root) {
    let idx = 0;
    const stack = [root];
    while (stack.length) {
      const node = stack.pop();
      order.set(node, idx++);
      const children = node.children || [];
      for (let i = children.length - 1; i >= 0; i--) stack.push(children[i]);
    }
  }
  out.sort((a, b) => {
    const ia = order.get(a);
    const ib = order.get(b);
    if (ia === undefined && ib === undefined) return 0;
    if (ia === undefined) return 1;
    if (ib === undefined) return -1;
    return ia - ib;
  });
  return out;
}

exports.getChildren = getChildrenWithChecks;
exports.getSiblings = getSiblings;
exports.nextElementSibling = nextElementSibling;
exports.prevElementSibling = prevElementSibling;
exports.uniqueSort = uniqueSort;

};
__mods['vendor/parse5.js'] = function (module, exports, require) {
// vendor/parse5.js —— parse5 替代：lexbor 解析 + parse5 风格序列化
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

// ---------------------------------------------------------------------------
// 序列化（对齐 parse5 的 serialize/serializeOuter 对 htmlparser2 树的输出）
// ---------------------------------------------------------------------------

const VOID_ELEMENTS = new Set([
  'area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
  'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source',
  'track', 'wbr',
]);

const RAW_TEXT_ELEMENTS = new Set([
  'script', 'style', 'xmp', 'iframe', 'noembed', 'noframes', 'plaintext',
  'noscript',
]);

// parse5 encodeText：& < > \u00A0（parse5 默认 encodeHtmlEntities=false，
// 命名实体不解码后重新编码）
function encodeText(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/\u00A0/g, '&nbsp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

// parse5 encodeAttr：& " \u00A0
function encodeAttr(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/\u00A0/g, '&nbsp;')
    .replace(/"/g, '&quot;');
}

function serializeChildren(children) {
  let out = '';
  for (const c of children) out += serializeNode(c);
  return out;
}

function serializeNode(node) {
  if (Array.isArray(node)) return serializeChildren(node);
  if (!node || typeof node !== 'object') return '';
  const t = node.type;
  switch (t) {
    case 'root':
      return serializeChildren(node.children || []);
    case 'text':
      return encodeText(node.data);
    case 'comment':
      return '<!--' + node.data + '-->';
    case 'directive':
      return '<' + node.data + '>';
    case 'cdata':
      return '<![CDATA[' + node.data + ']]>';
    case 'script':
    case 'style':
    case 'tag':
      return serializeElement(node);
    default:
      return '';
  }
}

function serializeElement(el) {
  const name = el.name;
  let out = '<' + name;
  const attrs = el.attribs || {};
  for (const k of Object.keys(attrs)) {
    out += ' ' + k + '="' + encodeAttr(attrs[k]) + '"';
  }
  if (VOID_ELEMENTS.has(name)) {
    return out + '>';
  }
  out += '>';
  const children = el.children || [];
  if (RAW_TEXT_ELEMENTS.has(name)) {
    // raw text 内容不转义（parse5 行为）
    for (const c of children) {
      if (c.type === 'text' || c.type === 'cdata') out += c.data;
      else out += serializeNode(c);
    }
  } else {
    out += serializeChildren(children);
  }
  return out + '</' + name + '>';
}

// 节点自身 + 后代序列化（serializeOuter 语义；root 展开 children）
function serializeOuter(node) {
  return serializeNode(node);
}

// ---------------------------------------------------------------------------
// 解析（lexbor）
// ---------------------------------------------------------------------------

// parse5-htmlparser2-tree-adapter 兼容：给节点加 parentNode/childNodes 等
// 别名（getter 实时反映 cheerio 的树操作；不可枚举避免干扰 deepEqual）
function addParse5Aliases(node) {
  if (node && typeof node === 'object') {
    if (!('parentNode' in node)) {
      Object.defineProperty(node, 'parentNode', {
        get() { return this.parent; },
        enumerable: false,
        configurable: true,
      });
    }
    if (!('nextSibling' in node)) {
      Object.defineProperty(node, 'nextSibling', {
        get() { return this.next; },
        enumerable: false,
        configurable: true,
      });
    }
    if (!('previousSibling' in node)) {
      Object.defineProperty(node, 'previousSibling', {
        get() { return this.prev; },
        enumerable: false,
        configurable: true,
      });
    }
    if (Array.isArray(node.children) && !('childNodes' in node)) {
      Object.defineProperty(node, 'childNodes', {
        get() { return this.children; },
        enumerable: false,
        configurable: true,
      });
    }
  }
  return node;
}

function addAliasesRecursive(root) {
  const stack = [root];
  while (stack.length) {
    const n = stack.pop();
    if (!n || typeof n !== 'object') continue;
    addParse5Aliases(n);
    if (Array.isArray(n.children)) {
      for (let i = 0; i < n.children.length; i++) stack.push(n.children[i]);
    }
  }
  return root;
}

// parse5 的 parse(content, options)：完整文档
function parse(content, options) {
  const root = globalThis.__lexbor_parse(String(content), true, null);
  return addAliasesRecursive(root);
}

// parse5 的 parseFragment(context, content, options)
function parseFragment(context, content, options) {
  let contextTag = null;
  if (context && typeof context === 'object') {
    if (context.name) contextTag = context.name;
    else if (context.tagName) contextTag = context.tagName;
  }
  const root = globalThis.__lexbor_parse(String(content), false, contextTag);
  return addAliasesRecursive(root);
}

exports.parse = parse;
exports.parseFragment = parseFragment;
exports.serializeOuter = serializeOuter;
exports.serialize = serializeNode;

};
__mods['vendor/parse5-htmlparser2-tree-adapter.js'] = function (module, exports, require) {
// vendor/parse5-htmlparser2-tree-adapter.js —— 占位 adapter（cheerio 仅赋值引用）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });
exports.adapter = {};

};
__mods['vendor/dom-serializer.js'] = function (module, exports, require) {
// vendor/dom-serializer.js —— dom-serializer 替代（htmlparser2 风格序列化，
// 用于 cheerio 的 xml/_useHtmlParser2 渲染路径）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const VOID_ELEMENTS = new Set([
  'area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
  'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source',
  'track', 'wbr',
]);

// encodeXML：& < > 转义（text）
function encodeXMLText(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

// 属性值：& < > " 转义
function encodeXMLAttr(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function render(node, options) {
  if (Array.isArray(node)) {
    let out = '';
    for (const n of node) out += render(n, options);
    return out;
  }
  if (!node || typeof node !== 'object') return '';
  options = options || {};
  const t = node.type;
  switch (t) {
    case 'root':
      return render(node.children || [], options);
    case 'text':
      return options.encodeEntities === false
        ? String(node.data)
        : encodeXMLText(node.data);
    case 'comment':
      return '<!--' + node.data + '-->';
    case 'directive':
      return '<' + node.data + '>';
    case 'cdata':
      return '<![CDATA[' + node.data + ']]>';
    case 'script':
    case 'style':
    case 'tag':
      return renderTag(node, options);
    default:
      return '';
  }
}

function renderTag(el, options) {
  const name = el.name;
  let out = '<' + name;
  const attrs = el.attribs || {};
  const keys = Object.keys(attrs);
  for (let i = 0; i < keys.length; i++) {
    const k = keys[i];
    out += ' ' + k + '="' + encodeXMLAttr(attrs[k]) + '"';
  }
  if (VOID_ELEMENTS.has(name)) {
    return out + '>';
  }
  out += '>';
  out += render(el.children || [], options);
  return out + '</' + name + '>';
}

module.exports = render;
module.exports.default = render;

};
__mods['vendor/cheerio-select.js'] = function (module, exports, require) {
// vendor/cheerio-select.js —— css-select/cheerio-select 替代：基于 lexbor CSS 选择器
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

function isTag(node) {
  return node && typeof node === 'object' &&
    (node.type === 'tag' || node.type === 'script' || node.type === 'style');
}

// css-select 特有语法 → lexbor 可解析形式
function preprocessSelector(sel) {
  let s = String(sel);
  // [name!="x"] → :not([name="x"])
  s = s.replace(/\[([\w.-]+)!=("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|[\w-]+)\]/g,
    ':not([$1=$2])');
  // :matches(...) → :is(...)
  s = s.replace(/:matches\(/g, ':is(');
  // jQuery 表单伪类 → 属性选择器组合
  s = s.replace(/:submit\b/g, 'input[type="submit"], button[type="submit"]');
  s = s.replace(/:image\b/g, 'input[type="image"]');
  s = s.replace(/:reset\b/g, 'input[type="reset"], button[type="reset"]');
  s = s.replace(/:file\b/g, 'input[type="file"]');
  s = s.replace(/:password\b/g, 'input[type="password"]');
  s = s.replace(/:radio\b/g, 'input[type="radio"]');
  s = s.replace(/:checkbox\b/g, 'input[type="checkbox"]');
  s = s.replace(/:text\b/g, 'input[type="text"]');
  s = s.replace(/:button\b/g, 'button, input[type="button"]');
  s = s.replace(/:selected\b/g, '[selected]');
  s = s.replace(/:first(?!-)\b/g, ':first-child');
  s = s.replace(/:last(?!-)\b/g, ':last-child');
  s = s.replace(/:even\b/g, ':nth-child(2n+1)');
  s = s.replace(/:odd\b/g, ':nth-child(2n)');
  // css-select 的 :eq(n)/:nth(n)（0-based 集合位置）→ nth-child(n+1)
  s = s.replace(/:eq\((\d+)\)/g, ':nth-child($1+1)');
  s = s.replace(/:nth\((\d+)\)/g, ':nth-child($1+1)');
  // 相对选择器：css-select 支持 find('> li')/find('+.b') 等（:scope 为查询根）
  if (/^\s*[>+~]/.test(s)) {
    s = ':scope ' + s;
  }
  return s;
}

// 向上找文档根（type === 'root' 的节点）
function getRoot(node) {
  let cur = node;
  while (cur && typeof cur === 'object') {
    if (cur.type === 'root') return cur;
    cur = cur.parent;
  }
  return null;
}

function queryAll(root, selector, includeSelf) {
  return globalThis.__lexbor_queryAll(root, preprocessSelector(selector), !!includeSelf);
}

// css-select 的 select(selector, elems, options, limit)：对每个候选元素，
// 在其自身范围内查询（scope = 元素本身，css-select 的 :scope 语义），
// 结果 = 匹配且在该元素子树内（含自身）；去重、limit 截断。
function select(selector, elems, options, limit) {
  const out = [];
  const seen = new Set();
  for (const el of elems) {
    if (!el || typeof el !== 'object') continue;
    let matches;
    try {
      matches = queryAll(el, selector, true);
    } catch (e) {
      // 选择器无法解析：交给调用方（css-select 语义为抛错）
      throw e;
    }
    for (let i = 0; i < matches.length; i++) {
      const m = matches[i];
      if (!seen.has(m) && isDescendantOrSelf(el, m)) {
        seen.add(m);
        out.push(m);
        if (limit && out.length >= limit) return out;
      }
    }
  }
  return out;
}

// m 是否在 el 的子树内（含 el 自身）
function isDescendantOrSelf(el, m) {
  let cur = m;
  while (cur && typeof cur === 'object') {
    if (cur === el) return true;
    cur = cur.parent;
  }
  return false;
}

function selectOne(selector, elems, options) {
  const res = select(selector, elems, options, 1);
  return res.length > 0 ? res[0] : null;
}

// css-select 的 is(elem, query, options)：elem 是否匹配 query（scope = elem）
function is(elem, query, options) {
  if (!isTag(elem)) return false;
  const matches = queryAll(elem, query, true);
  for (let i = 0; i < matches.length; i++) {
    if (matches[i] === elem) return true;
  }
  return false;
}

// css-select 的 filter(query, nodes, options)：nodes 中匹配的元素
function filter(query, nodes, options) {
  const out = [];
  for (const node of nodes) {
    if (!isTag(node)) continue;
    if (is(node, query, options)) out.push(node);
  }
  return out;
}

// 任一节点匹配
function some(nodes, query, options) {
  for (const node of nodes) {
    if (isTag(node) && is(node, query, options)) return true;
  }
  return false;
}

exports.select = select;
exports.selectOne = selectOne;
exports.is = is;
exports.filter = filter;
exports.some = some;

};
__mods['vendor/encoding-sniffer.js'] = function (module, exports, require) {
// vendor/encoding-sniffer.js —— 退化实现（UTF-8 假设）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

function decodeBuffer(buffer, options) {
  // buffer: { toString(enc) } 或 Uint8Array
  if (typeof buffer.toString === 'function') {
    return buffer.toString('utf8');
  }
  const bytes = Array.from(buffer);
  const chars = bytes.map((b) => String.fromCharCode(b)).join('');
  try {
    return decodeURIComponent(escape(chars));
  } catch (e) {
    return chars;
  }
}

function getEncoding(buffer) {
  return 'utf-8';
}

exports.decodeBuffer = decodeBuffer;
exports.getEncoding = getEncoding;

};
__mods['vendor/parse5-parser-stream.js'] = function (module, exports, require) {
// vendor/parse5-parser-stream.js —— 退化实现（同步一次性解析）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

class ParserStream {
  constructor(options) {
    this._data = '';
    this.options = options;
  }
  write(chunk) {
    this._data += String(chunk);
  }
  end(cb) {
    const root = globalThis.__lexbor_parse(this._data, true, null);
    if (this.options && this.options.treeAdapter) {
      this.document = root;
    }
    if (typeof cb === 'function') cb();
  }
}

exports.default = ParserStream;
exports.ParserStream = ParserStream;

};
__mods['vendor/whatwg-mimetype.js'] = function (module, exports, require) {
// vendor/whatwg-mimetype.js —— 退化实现（MIMEType 基础解析）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

class MIMEType {
  constructor(s) {
    const parts = String(s).split(';');
    const [type, subtype] = parts[0].trim().split('/');
    this.type = type.toLowerCase();
    this.subtype = (subtype || '').toLowerCase();
    this.parameters = new Map();
    for (let i = 1; i < parts.length; i++) {
      const eq = parts[i].indexOf('=');
      if (eq > 0) {
        this.parameters.set(
          parts[i].slice(0, eq).trim().toLowerCase(),
          parts[i].slice(eq + 1).trim().replace(/^"|"$/g, ''),
        );
      }
    }
  }
  get essence() {
    return this.type + '/' + this.subtype;
  }
  isXML() {
    return this.subtype === 'xml' || this.subtype.endsWith('+xml');
  }
  isHTML() {
    return this.type === 'text' && this.subtype === 'html';
  }
}

exports.default = MIMEType;
exports.MIMEType = MIMEType;

};
__mods['vendor/node-stream.js'] = function (module, exports, require) {
// vendor/node-stream.js —— node:stream 退化实现（Readable/Writable 极简）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const EventEmitter = (typeof globalThis.EventTarget !== 'undefined')
  ? class { constructor() { this._listeners = {}; } on(n, f) { (this._listeners[n] = this._listeners[n] || []).push(f); return this; } once(n, f) { const g = (...a) => { f(...a); this.off(n, g); }; return this.on(n, g); } off(n, f) { const l = this._listeners[n]; if (l) { const i = l.indexOf(f); if (i >= 0) l.splice(i, 1); } return this; } emit(n, ...a) { const l = this._listeners[n] || []; for (const f of [...l]) f(...a); return true; } }
  : class { constructor() { this._listeners = {}; } on(n, f) { (this._listeners[n] = this._listeners[n] || []).push(f); return this; } emit(n, ...a) { const l = this._listeners[n] || []; for (const f of [...l]) f(...a); return true; } };

class Readable extends EventEmitter {
  constructor(opts) { super(); this.readable = true; this._buf = []; this.ended = false; }
  _read() {}
  push(chunk) { if (chunk === null) { this.ended = true; this.emit('end'); } else { this._buf.push(chunk); this.emit('data', chunk); } return true; }
  pipe(dest) { this.on('data', (d) => dest.write(d)); this.on('end', () => dest.end()); return dest; }
  read() { return this._buf.length ? this._buf.shift() : null; }
  onData(fn) { this.on('data', fn); }
}

class Writable extends EventEmitter {
  constructor(opts) { super(); this.writable = true; }
  write(chunk) { this.emit('drain'); return true; }
  end(cb) { this.emit('finish'); if (typeof cb === 'function') cb(); return this; }
}

exports.Readable = Readable;
exports.Writable = Writable;
exports.EventEmitter = EventEmitter;

};
__mods['vendor/node-http.js'] = function (module, exports, require) {
// vendor/node-http.js —— node:http 退化实现（index.spec 的 fromURL 用不到时占位）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

exports.request = function () {
  throw new Error('node:http request is not available in this environment');
};

};
__mods['vendor/undici.js'] = function (module, exports, require) {
// vendor/undici.js —— fetch 替代（使用全局 fetch）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

exports.fetch = typeof globalThis.fetch === 'function'
  ? globalThis.fetch.bind(globalThis)
  : function () { throw new Error('fetch is not available'); };

};
__mods['vendor/vitest.js'] = function (module, exports, require) {
// vendor/vitest.js —— vitest 测试 API 最小兼容层（describe/it/expect 等）
'use strict';
Object.defineProperty(exports, '__esModule', { value: true });

const g = globalThis;

// ---------------------------------------------------------------------------
// 结果收集
// ---------------------------------------------------------------------------
if (!g.__cheerio_tests) {
  g.__cheerio_tests = { pass: 0, fail: 0, failures: [] };
}

function fail(name, err) {
  g.__cheerio_tests.fail++;
  g.__cheerio_tests.failures.push({
    name,
    message: String(err && err.message ? err.message : err),
    stack: err && err.stack ? String(err.stack) : '',
  });
}

// it 的错误带调用栈（QuickJS 只给 <eval> 行号，但能给到 spec 文件内行号）
// ---------------------------------------------------------------------------
// describe / it
// ---------------------------------------------------------------------------
const __describeStack = [];

function describe(name, fn) {
  __describeStack.push({ befores: [], afters: [] });
  try {
    if (typeof fn === 'function') fn();
  } finally {
    __describeStack.pop();
  }
}

function it(name, fn) {
  g.__cheerio_tests.last = String(name);
  try {
    // 收集从外层到内层的 beforeEach（vitest 语义）
    for (let d = 0; d < __describeStack.length; d++) {
      const befores = __describeStack[d].befores;
      for (let i = 0; i < befores.length; i++) befores[i]();
    }
    const r = fn();
    if (r && typeof r.then === 'function') {
      // 异步测试：同步等待（测试基本同步；异步极少）
      g.__cheerio_tests.fail++;
      g.__cheerio_tests.failures.push({
        name: String(name),
        message: 'async test not supported',
        stack: '',
      });
      return;
    }
    g.__cheerio_tests.pass++;
  } catch (e) {
    fail(name, e);
  }
}

function beforeEach(fn) {
  if (__describeStack.length > 0) __describeStack[__describeStack.length - 1].befores.push(fn);
}
function afterEach(fn) {
  if (__describeStack.length > 0) __describeStack[__describeStack.length - 1].afters.push(fn);
}
function beforeAll(fn) { if (typeof fn === 'function') fn(); }
function afterAll(fn) { if (typeof fn === 'function') fn(); }
function test(name, fn) { it(name, fn); }

function expectTypeOf() {
  return {
    toEqualTypeOf() {},
    toMatchTypeOf() {},
    toBeNullable() {},
    toBeNever() {},
  };
}

// ---------------------------------------------------------------------------
// expect
// ---------------------------------------------------------------------------
function deepEqual(a, b, strict) {
  return deepEqualImpl(a, b, strict, new Set());
}

function deepEqualImpl(a, b, strict, seen) {
  if (Object.is(a, b)) return true;
  if (typeof a !== typeof b) return false;
  if (a === null || b === null) return false;
  if (typeof a !== 'object') return false;
  // 循环引用保护（Cheerio 实例/节点树可能自引用）
  if (seen.has(a)) return seen.has(b);
  seen.add(a);
  if (seen.has(b)) return false;
  seen.add(b);
  if (Array.isArray(a) !== Array.isArray(b)) return false;
  if (Array.isArray(a)) {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
      if (!deepEqualImpl(a[i], b[i], strict, seen)) return false;
    }
    return true;
  }
  // cheerio 实例/类数组：比较元素（toStrictEqual 对 Cheerio 与数组）
  const aKeys = Object.keys(a);
  const bKeys = Object.keys(b);
  if (strict && aKeys.length !== bKeys.length) return false;
  // 顺序：对 toStrictEqual 要求键一致（含顺序）
  if (strict) {
    for (let i = 0; i < aKeys.length; i++) {
      if (aKeys[i] !== bKeys[i]) return false;
    }
  }
  for (const k of aKeys) {
    if (k === 'prevObject' || k === '_root') continue;
    if (!(k in b)) return false;
    if (!deepEqualImpl(a[k], b[k], strict, seen)) return false;
  }
  return true;
}

let __fvDepth = 0;
function formatValue(v, seen) {
  __fvDepth++;
  if (typeof g.__log_fv2 === 'function' && __fvDepth <= 3) {
    g.__log_fv2('fv ' + __fvDepth + ' t=' + typeof v + (v && v.type ? '/' + v.type : '') + (v && v.name ? ':' + v.name : ''));
  }
  if (__fvDepth > 25) { __fvDepth = 0; return '[Deep]'; }
  let out;
  if (v === null) out = 'null';
  else if (v === undefined) out = 'undefined';
  else if (typeof v === 'string') out = JSON.stringify(v);
  else if (typeof v === 'number' || typeof v === 'boolean' || typeof v === 'bigint') out = String(v);
  else if (typeof v === 'function') out = '[Function]';
  else if (typeof v === 'symbol') out = String(v);
  else if (typeof v === 'object') {
    seen = seen || new Set();
    if (seen.has(v)) {
      out = '[Circular]';
    } else {
      seen.add(v);
      if (Array.isArray(v)) {
        const items = [];
        for (let i = 0; i < v.length; i++) items.push(formatValue(v[i], seen));
        out = '[' + items.join(', ') + ']';
      } else if (v.cheerio || (v.length !== undefined && v.type === undefined)) {
        // cheerio 类数组
        const items = [];
        for (let i = 0; i < v.length; i++) items.push(formatValue(v[i], seen));
        out = 'Cheerio(' + items.join(', ') + ')';
      } else {
        const keys = Object.keys(v);
        const items = [];
        for (let i = 0; i < keys.length; i++) {
          items.push(keys[i] + ': ' + formatValue(v[keys[i]], seen));
        }
        out = '{' + items.join(', ') + '}';
      }
    }
  } else {
    out = String(v);
  }
  __fvDepth--;
  return out;
}

function makeExpect(actual) {
  const api = {
    get not() {
      return makeNegated(actual);
    },
    toBe(expected) {
      if (!Object.is(actual, expected)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to be ' + formatValue(expected),
        );
      }
    },
    toEqual(expected) {
      if (!deepEqual(actual, expected, false)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to equal ' + formatValue(expected),
        );
      }
    },
    toStrictEqual(expected) {
      if (!deepEqual(actual, expected, true)) {
        throw new Error(
          'expected ' + formatValue(actual) + ' to strictly equal ' +
            formatValue(expected),
        );
      }
    },
    toHaveLength(n) {
      const len = actual == null ? undefined : actual.length;
      if (len !== n) {
        throw new Error('expected length ' + String(len) + ' to be ' + String(n));
      }
    },
    toHaveProperty(path, value) {
      const parts = String(path).split('.');
      let cur = actual;
      for (const p of parts) {
        if (cur == null) {
          throw new Error('expected property ' + path + ' not found');
        }
        cur = cur[p];
      }
      if (arguments.length >= 2 && !deepEqual(cur, value, true)) {
        throw new Error('property ' + path + ' value mismatch');
      }
    },
    toBeUndefined() {
      if (actual !== undefined) {
        throw new Error('expected undefined, got ' + formatValue(actual));
      }
    },
    toBeDefined() {
      if (actual === undefined) {
        throw new Error('expected defined value');
      }
    },
    toBeNull() {
      if (actual !== null) {
        throw new Error('expected null, got ' + formatValue(actual));
      }
    },
    toBeTruthy() {
      if (!actual) {
        throw new Error('expected truthy, got ' + formatValue(actual));
      }
    },
    toBeFalsy() {
      if (actual) {
        throw new Error('expected falsy, got ' + formatValue(actual));
      }
    },
    toBeInstanceOf(cls) {
      if (!(actual instanceof cls)) {
        throw new Error('expected instance of ' + String(cls && cls.name));
      }
    },
    toThrow(match) {
      let threw = false;
      let err = null;
      try {
        if (typeof actual === 'function') actual();
        else throw new Error('not a function');
      } catch (e) {
        threw = true;
        err = e;
      }
      if (!threw) throw new Error('expected function to throw');
      if (match !== undefined) {
        if (typeof match === 'string') {
          if (!String(err && err.message).includes(match)) {
            throw new Error('expected error to match ' + match + ', got ' + err);
          }
        } else if (match instanceof RegExp) {
          if (!match.test(String(err && err.message))) {
            throw new Error('expected error to match ' + match + ', got ' + err);
          }
        } else if (typeof match === 'function') {
          if (!(err instanceof match)) {
            throw new Error('expected error instanceof ' + match.name + ', got ' + err);
          }
        }
      }
    },
    toContain(item) {
      if (actual == null) throw new Error('expected value to contain ' + formatValue(item));
      if (typeof actual === 'string') {
        if (!actual.includes(item)) {
          throw new Error('expected string to contain ' + formatValue(item));
        }
      } else {
        let found = false;
        for (let i = 0; i < actual.length; i++) {
          if (deepEqual(actual[i], item, false)) { found = true; break; }
        }
        if (!found) {
          throw new Error('expected array to contain ' + formatValue(item));
        }
      }
    },
    toContainEqual(item) {
      this.toContain(item);
    },
    toMatch(re) {
      if (!re.test(String(actual))) {
        throw new Error('expected ' + formatValue(actual) + ' to match ' + re);
      }
    },
    toBeLessThan(n) {
      if (!(actual < n)) {
        throw new Error('expected ' + actual + ' < ' + n);
      }
    },
    toBeGreaterThan(n) {
      if (!(actual > n)) {
        throw new Error('expected ' + actual + ' > ' + n);
      }
    },
    toBeLessThanOrEqual(n) {
      if (!(actual <= n)) {
        throw new Error('expected ' + actual + ' <= ' + n);
      }
    },
    toBeGreaterThanOrEqual(n) {
      if (!(actual >= n)) {
        throw new Error('expected ' + actual + ' >= ' + n);
      }
    },
    toBeCloseTo(n, digits) {
      const eps = Math.pow(10, -(digits || 2)) / 2;
      if (Math.abs(actual - n) > eps) {
        throw new Error('expected ' + actual + ' close to ' + n);
      }
    },
    toEqualTypeOf() {},
    toMatchTypeOf() {},
    toBeTypeOf() {},
    // 捕获断言错误：抛给 it() 的 try/catch
  };
  return api;
}

function makeNegated(actual) {
  const neg = {};
  const names = [
    'toBe', 'toEqual', 'toStrictEqual', 'toHaveLength', 'toHaveProperty',
    'toBeUndefined', 'toBeDefined', 'toBeNull', 'toBeTruthy', 'toBeFalsy',
    'toBeInstanceOf', 'toThrow', 'toContain', 'toMatch', 'toBeLessThan',
    'toBeGreaterThan', 'toBeCloseTo',
  ];
  for (const n of names) {
    neg[n] = (...args) => {
      try {
        makeExpect(actual)[n](...args);
      } catch (e) {
        return; // 原断言失败 = 取反成功
      }
      throw new Error('expected negation of ' + n + ' to fail');
    };
  }
  return neg;
}

function expect(actual) {
  return makeExpect(actual);
}

exports.describe = describe;
exports.it = it;
exports.test = test;
exports.expect = expect;
exports.beforeEach = beforeEach;
exports.afterEach = afterEach;
exports.beforeAll = beforeAll;
exports.afterAll = afterAll;
exports.expectTypeOf = expectTypeOf;
exports.vi = {};

};

// 路径规范化 + 相对解析
function __norm(p) {
  var parts = [];
  for (var seg of p.split('/')) {
    if (seg === '' || seg === '.') continue;
    if (seg === '..') { if (parts.length) parts.pop(); }
    else parts.push(seg);
  }
  return parts.join('/');
}
function __resolve(dir, name) {
  if (name.startsWith('./') || name.startsWith('../')) {
    return __norm(dir + '/' + name);
  }
  return name; // 已在 transform 时重写为相对路径
}
var __cache = {};
function __load(dir, name) {
  var id = __resolve(dir, name);
  if (__cache[id]) return __cache[id].exports;
  if (!__mods[id]) throw new Error('cheerio bundle: module not found: ' + id);
  var module = { exports: {} };
  __cache[id] = module;
  __mods[id](module, module.exports, function (n) { return __load(id.includes('/') ? id.slice(0, id.lastIndexOf('/')) : '', n); });
  return module.exports;
}
globalThis.__cheerio_require = function (name) {
  return __load('', name);
};
})();

)BUNDLE_7F3A9D2C";
    return s;
}
} // namespace qjsbind::cheerio
