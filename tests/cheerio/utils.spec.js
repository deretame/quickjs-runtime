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
const utils = __importStar(require("./utils.js"));
(0, vitest_1.describe)('util functions', () => {
    (0, vitest_1.it)('camelCase function test', () => {
        (0, vitest_1.expect)(utils.camelCase('cheerio.js')).toBe('cheerioJs');
        (0, vitest_1.expect)(utils.camelCase('camel-case-')).toBe('camelCase');
        (0, vitest_1.expect)(utils.camelCase('__directory__')).toBe('_directory_');
        (0, vitest_1.expect)(utils.camelCase('_one-two.three')).toBe('OneTwoThree');
    });
    (0, vitest_1.it)('cssCase function test', () => {
        (0, vitest_1.expect)(utils.cssCase('camelCase')).toBe('camel-case');
        (0, vitest_1.expect)(utils.cssCase('jQuery')).toBe('j-query');
        (0, vitest_1.expect)(utils.cssCase('neverSayNever')).toBe('never-say-never');
        (0, vitest_1.expect)(utils.cssCase('CSSCase')).toBe('-c-s-s-case');
    });
    (0, vitest_1.it)('isHtml function test', () => {
        (0, vitest_1.expect)(utils.isHtml('<html>')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('\n<html>\n')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('#main')).toBe(false);
        (0, vitest_1.expect)(utils.isHtml('\n<p>foo<p>bar\n')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('dog<p>fox<p>cat')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('<p>fox<p>cat')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('\n<p>fox<p>cat\n')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('#<p>fox<p>cat#')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('<!-- comment -->')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('<!doctype html>')).toBe(true);
        (0, vitest_1.expect)(utils.isHtml('<123>')).toBe(false);
    });
});
