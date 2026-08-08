// 垫片：parse() 返回 domhandler 风格树（经 C++ cheerio 解析）
module.exports = {
  parse: (content, options, isDocument, context) => {
    const $ = globalThis.cheerio.load(content);
    return $[0]; // document 节点（惰性 children/type 等）
  },
};
