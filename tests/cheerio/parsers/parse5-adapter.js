// 垫片：parse5 adapter（经 C++ cheerio）
module.exports = {
  parseWithParse5: (content, options, isDocument, context) => {
    const $ = globalThis.cheerio.load(content);
    return $[0];
  },
  renderWithParse5: (dom, options) => {
    const $ = globalThis.cheerio.load('');
    return $(dom[0]).html();
  },
};
