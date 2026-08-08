// 垫片：load 入口
module.exports = {
  load: (content, options) => globalThis.cheerio.load(content),
};
