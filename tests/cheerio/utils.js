// 垫片：utils（isHtml 等）
module.exports = {
  isHtml: (s) => typeof s === 'string' && s.trimStart().startsWith('<'),
};
