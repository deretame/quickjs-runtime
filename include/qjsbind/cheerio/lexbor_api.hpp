// lexbor_api.hpp —— cheerio 兼容 API 的 C++ 实现（方案 A）
//
// 所有操作直接在 lexbor C 树上执行：序列化（lxb_html_serialize_tree_str）、
// 文本拼接、属性读写、class 操作、DOM 修改（fragment 解析 + 节点移动）、
// 克隆（lxb_dom_node_clone）。方法注册在 $ 原型上，this_val 直接解包
// CheerioSel，不经过 JS 树。
#pragma once

#include <qjsbind/cheerio/lexbor_match.hpp>

#include <cctype>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

namespace qjsbind::cheerio::lxb_handle {

// ---------------------------------------------------------------------------
// 序列化 / 文本
// ---------------------------------------------------------------------------
inline std::string c_serialize_tree(lxb_dom_node_t* n)
{
    lexbor_str_t str = {0};
    lxb_status_t st = lxb_html_serialize_tree_str(n, &str);
    if (st != LXB_STATUS_OK)
        return "";
    std::string out((const char*)str.data, str.length);
    lxb_dom_document_t* doc = n->owner_document;
    // serialize_tree_str 内部用 doc->text 分配 str（见 lexbor serialize.c）
    lexbor_str_destroy(&str, doc ? doc->text : nullptr, false);
    return out;
}

inline std::string c_text_content(lxb_dom_node_t* n)
{
    CTreeMatcher m{nullptr, nullptr};
    return m.text_content(n);
}

// ---------------------------------------------------------------------------
// 属性 / class
// ---------------------------------------------------------------------------
inline bool c_attr_set(lxb_dom_node_t* el, const std::string& name,
                       const std::string& value)
{
    if (!c_node_is_element(el))
        return false;
    lxb_dom_attr_t* a = lxb_dom_element_set_attribute(
        lxb_dom_interface_element(el), (const lxb_char_t*)name.data(),
        name.size(), (const lxb_char_t*)value.data(), value.size());
    return a != nullptr;
}

inline bool c_attr_remove(lxb_dom_node_t* el, const std::string& name)
{
    if (!c_node_is_element(el))
        return false;
    lxb_status_t st = lxb_dom_element_remove_attribute(
        lxb_dom_interface_element(el), (const lxb_char_t*)name.data(),
        name.size());
    return st == LXB_STATUS_OK;
}

inline std::string c_attr_get(lxb_dom_node_t* el, const std::string& name)
{
    size_t vl = 0;
    const lxb_char_t* v = c_attr_value(el, name.data(), name.size(), &vl);
    return v ? std::string((const char*)v, vl) : "";
}

// class 属性分词（保留原始顺序）
inline std::vector<std::string> c_class_list(lxb_dom_node_t* el)
{
    std::vector<std::string> out;
    std::string attr = c_attr_get(el, "class");
    size_t pos = 0;
    while (pos <= attr.size()) {
        size_t end = attr.find(' ', pos);
        if (end == std::string::npos)
            end = attr.size();
        std::string tok = attr.substr(pos, end - pos);
        if (!tok.empty())
            out.push_back(tok);
        if (end == attr.size())
            break;
        pos = end + 1;
    }
    return out;
}

inline void c_class_set(lxb_dom_node_t* el, const std::vector<std::string>& cls)
{
    std::string joined;
    for (size_t i = 0; i < cls.size(); ++i) {
        if (i)
            joined += ' ';
        joined += cls[i];
    }
    if (joined.empty())
        c_attr_remove(el, "class");
    else
        c_attr_set(el, "class", joined);
}

// ---------------------------------------------------------------------------
// 递归复制节点到目标文档（fragment 节点属于独立文档，跨文档 clone 会因
// tag_id 表不兼容崩溃，故自实现重建：元素/文本/注释 + 属性 + 子节点）
inline lxb_dom_node_t* c_copy_node(lxb_dom_document_t* target,
                                   lxb_dom_node_t* src)
{
    if (!src)
        return nullptr;
    switch (src->type) {
        case LXB_DOM_NODE_TYPE_ELEMENT: {
            lxb_dom_element_t* el = lxb_dom_interface_element(src);
            size_t nl = 0;
            const lxb_char_t* nm =
                lxb_tag_name_by_id(lxb_dom_element_tag_id(el), &nl);
            lxb_dom_element_t* ne = lxb_dom_document_create_element(
                target, nm, nl, nullptr);
            if (!ne)
                return nullptr;
            for (lxb_dom_attr_t* a = el->first_attr; a; a = a->next) {
                size_t anl = 0, avl = 0;
                const lxb_char_t* an = lxb_dom_attr_local_name(a, &anl);
                const lxb_char_t* av = lxb_dom_attr_value(a, &avl);
                if (an && av)
                    lxb_dom_element_set_attribute(ne, an, anl, av, avl);
            }
            for (lxb_dom_node_t* c = src->first_child; c; c = c->next) {
                lxb_dom_node_t* cp = c_copy_node(target, c);
                if (cp)
                    lxb_dom_node_insert_child(lxb_dom_interface_node(ne), cp);
            }
            return lxb_dom_interface_node(ne);
        }
        case LXB_DOM_NODE_TYPE_TEXT: {
            size_t dl = 0;
            const lxb_char_t* d = lxb_dom_node_text_content(src, &dl);
            lxb_dom_text_t* t = lxb_dom_document_create_text_node(target, d, dl);
            return t ? lxb_dom_interface_node(t) : nullptr;
        }
        case LXB_DOM_NODE_TYPE_COMMENT: {
            size_t dl = 0;
            const lxb_char_t* d = lxb_dom_node_text_content(src, &dl);
            lxb_dom_comment_t* cm = lxb_dom_document_create_comment(target, d, dl);
            return cm ? lxb_dom_interface_node(cm) : nullptr;
        }
        default:
            return nullptr;
    }
}

// fragment 解析：document=NULL 让 lexbor 创建独立 fragment 文档（节点不随
// parser 销毁），复制到目标文档后销毁 fragment 文档。上下文用目标元素的
// tag（默认 BODY）。
// ---------------------------------------------------------------------------
inline std::vector<lxb_dom_node_t*> c_parse_fragment(JSContext* jctx,
                                                     lxb_dom_node_t* parent,
                                                     const std::string& html)
{
    std::vector<lxb_dom_node_t*> out;
    lxb_html_parser_t* parser = lxb_html_parser_create();
    if (!parser)
        return out;
    lxb_status_t st = lxb_html_parser_init(parser);
    if (st != LXB_STATUS_OK) {
        lxb_html_parser_destroy(parser);
        return out;
    }
    lxb_tag_id_t ctx_tag = LXB_TAG_BODY;
    if (c_node_is_element(parent)) {
        lxb_tag_id_t t = lxb_dom_element_tag_id(lxb_dom_interface_element(parent));
        if (t != LXB_TAG__UNDEF)
            ctx_tag = t;
    }
    lxb_dom_node_t* root = lxb_html_parse_fragment_by_tag_id(
        parser, nullptr, ctx_tag, LXB_NS_HTML, (const lxb_char_t*)html.data(),
        html.size());
    if (root) {
        lxb_html_document_t* fdoc = lxb_html_interface_document(root->owner_document);
        // 复制顶层节点并链接兄弟指针（复制不保留兄弟关系）
        lxb_dom_node_t* prev_cp = nullptr;
        for (lxb_dom_node_t* c = root->first_child; c; c = c->next) {
            lxb_dom_node_t* cp = c_copy_node(parent->owner_document, c);
            if (cp) {
                cp->prev = prev_cp;
                if (prev_cp)
                    prev_cp->next = cp;
                prev_cp = cp;
                out.push_back(cp);
            }
        }
        lxb_html_document_destroy(fdoc); // 深销毁 fragment 树
    }
    lxb_html_parser_destroy(parser);
    return out;
}

// ---------------------------------------------------------------------------
// $ 方法（C 风格函数：this_val = CheerioSel）
// ---------------------------------------------------------------------------
using SelFn = JSValue (*)(JSContext*, JSValueConst, int, JSValueConst*);

inline CheerioSel* sel_of(JSContext* ctx, JSValueConst this_val)
{
    return unwrap_sel(ctx, this_val);
}

// length 语义：new_sel(空) 直接返回空 $；否则构建
// prev：派生选择集带 prevObject（end() 链式回退）
inline JSValue make_sel_or_empty(JSContext* ctx, CheerioSel* base,
                                 std::vector<lxb_dom_node_t*> nodes,
                                 JSValueConst prev = JS_UNDEFINED)
{
    return make_sel(ctx, base->ref, std::move(nodes), prev);
}

// ---------------------------------------------------------------------------
// 集合工具：文档序、去重、统一过滤器（对应 JS 版 uniqueSort / _removeDuplicates
// / filterArray / getFilterFn）
// ---------------------------------------------------------------------------
inline bool c_matches(JSContext* ctx, lxb_dom_node_t* el,
                      const std::string& selector,
                      bool* ok = nullptr); // 定义见 fn_filter 之前
// 文档序比较：a 在 b 前 → -1；a 在 b 后 → 1；相同 → 0
inline int c_doc_order_cmp(lxb_dom_node_t* a, lxb_dom_node_t* b)
{
    if (a == b)
        return 0;
    std::vector<lxb_dom_node_t*> pa, pb;
    for (lxb_dom_node_t* p = a; p; p = p->parent)
        pa.push_back(p);
    for (lxb_dom_node_t* p = b; p; p = p->parent)
        pb.push_back(p);
    size_t ia = pa.size(), ib = pb.size();
    while (ia > 0 && ib > 0 && pa[ia - 1] == pb[ib - 1]) {
        --ia;
        --ib;
    }
    if (ia == 0)
        return -1; // a 是 b 的祖先
    if (ib == 0)
        return 1; // b 是 a 的祖先
    // 无公共祖先（不同文档的节点，如 fragment 节点）：视为等价，保持插入序
    if (ia >= pa.size() || ib >= pb.size() || pa[ia] != pb[ib])
        return 0;
    // pa[ia-1] / pb[ib-1] 是最近公共祖先下的两个孩子：比较兄弟序
    lxb_dom_node_t* ca = pa[ia - 1];
    lxb_dom_node_t* cb = pb[ib - 1];
    for (lxb_dom_node_t* s = ca->prev; s; s = s->prev) {
        if (s == cb)
            return 1; // cb 在 ca 前 → a 在 b 后
    }
    return -1;
}

// uniqueSort：文档序排序 + 去重（domutils uniqueSort 等价；跨文档等价节点
// 保持插入序——stable_sort 保证）
inline std::vector<lxb_dom_node_t*> c_unique_sort(std::vector<lxb_dom_node_t*> nodes)
{
    std::stable_sort(nodes.begin(), nodes.end(), [](lxb_dom_node_t* a, lxb_dom_node_t* b) {
        return c_doc_order_cmp(a, b) < 0;
    });
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

// _removeDuplicates：保持顺序去重
inline std::vector<lxb_dom_node_t*> c_remove_dups(std::vector<lxb_dom_node_t*> nodes)
{
    std::unordered_set<lxb_dom_node_t*> seen;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* n : nodes) {
        if (seen.insert(n).second)
            out.push_back(n);
    }
    return out;
}

// getFilterFn 等价：match 为函数时 fn(el, i)（this=el）；$ 时包含检查；否则 ===
// 注意：$ 对象可调用（JS_IsFunction 为 true），必须先判 $ 再判函数
inline bool c_match_pred(JSContext* ctx, JSValueConst match, lxb_dom_node_t* el,
                         size_t idx, DomRef* ref, bool* threw)
{
    if (CheerioSel* os = unwrap_sel(ctx, match)) {
        for (lxb_dom_node_t* n : os->nodes) {
            if (n == el)
                return true;
        }
        return false;
    }
    if (JS_IsFunction(ctx, match)) {
        JSValue el_v = make_node(ctx, el, ref);
        JSValue args[2] = {JS_NewUint32(ctx, (uint32_t)idx), el_v};
        JSValue r = JS_Call(ctx, match, el_v, 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, el_v);
        if (JS_IsException(r)) {
            JS_FreeValue(ctx, r);
            *threw = true;
            return false;
        }
        bool keep = JS_ToBool(ctx, r);
        JS_FreeValue(ctx, r);
        return keep;
    }
    if (CheerioSel* os = unwrap_sel(ctx, match)) {
        for (lxb_dom_node_t* n : os->nodes) {
            if (n == el)
                return true;
        }
        return false;
    }
    if (NodeHandle* h = unwrap_node(ctx, match))
        return h->node == el;
    return false;
}

// filterArray 等价：字符串 → 选择器匹配；否则 getFilterFn 谓词
inline std::vector<lxb_dom_node_t*> c_filter_nodes(JSContext* ctx,
                                                   const std::vector<lxb_dom_node_t*>& nodes,
                                                   JSValueConst match, DomRef* ref,
                                                   bool* threw)
{
    std::vector<lxb_dom_node_t*> out;
    if (JS_IsString(match)) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, match);
        if (!sel) {
            *threw = true;
            return out;
        }
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        bool ok = true;
        for (lxb_dom_node_t* el : nodes) {
            if (c_node_is_element(el) && c_matches(ctx, el, selector, &ok))
                out.push_back(el);
            if (!ok)
                break;
        }
        if (!ok) {
            throw_invalid_selector(ctx, selector);
            *threw = true;
        }
        return out;
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (c_match_pred(ctx, match, nodes[i], i, ref, threw))
            out.push_back(nodes[i]);
        if (*threw)
            return out;
    }
    return out;
}

// ---- 序列化输出（cheerio: 序列化第一个元素）----
inline JSValue fn_html_get(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        // html(htmlString)：设置第一个元素内容
        size_t hlen = 0;
        const char* h = JS_ToCStringLen(ctx, &hlen, argv[0]);
        if (!h)
            return JS_EXCEPTION;
        std::string html(h, hlen);
        JS_FreeCString(ctx, h);
        if (!s->nodes.empty()) {
            lxb_dom_node_t* el = s->nodes[0];
            // 清空子节点
            while (el->first_child)
                lxb_dom_node_remove(el->first_child);
            if (!html.empty()) {
                std::vector<lxb_dom_node_t*> frag =
                    c_parse_fragment(ctx, el, html);
                for (lxb_dom_node_t* n : frag)
                    lxb_dom_node_insert_child(el, n);
            }
        }
        return JS_UNDEFINED;
    }
    if (s->nodes.empty())
        return JS_NewString(ctx, "");
    return JS_NewStringLen(ctx, c_serialize_tree(s->nodes[0]).data(),
                           (int)c_serialize_tree(s->nodes[0]).size());
}

// ---- text()：拼接所有选中元素的文本 ----
inline JSValue fn_text(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        // text(textString)：设置所有元素文本
        size_t tlen = 0;
        const char* t = JS_ToCStringLen(ctx, &tlen, argv[0]);
        if (!t)
            return JS_EXCEPTION;
        std::string text(t, tlen);
        JS_FreeCString(ctx, t);
        for (lxb_dom_node_t* el : s->nodes) {
            while (el->first_child)
                lxb_dom_node_remove(el->first_child);
            // 创建文本节点
            lxb_dom_text_t* txt = lxb_dom_document_create_text_node(
                el->owner_document, (const lxb_char_t*)text.data(), text.size());
            if (txt)
                lxb_dom_node_insert_child(el, lxb_dom_interface_node(txt));
        }
        return JS_UNDEFINED;
    }
    std::string out;
    for (lxb_dom_node_t* n : s->nodes)
        out += c_text_content(n);
    return JS_NewStringLen(ctx, out.data(), (int)out.size());
}

// ---- attr(name[, value]) ----
inline JSValue fn_attr(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    size_t nlen = 0;
    const char* n = JS_ToCStringLen(ctx, &nlen, argv[0]);
    if (!n)
        return JS_EXCEPTION;
    std::string name(n, nlen);
    JS_FreeCString(ctx, n);
    if (argc > 1 && !JS_IsUndefined(argv[1]) && !JS_IsNull(argv[1])) {
        // attr(name, value)：设置所有选中元素
        size_t vlen = 0;
        const char* v = JS_ToCStringLen(ctx, &vlen, argv[1]);
        if (!v)
            return JS_EXCEPTION;
        std::string value(v, vlen);
        JS_FreeCString(ctx, v);
        for (lxb_dom_node_t* el : s->nodes)
            c_attr_set(el, name, value);
        return JS_UNDEFINED;
    }
    if (s->nodes.empty())
        return JS_UNDEFINED;
    return JS_NewStringLen(ctx, c_attr_get(s->nodes[0], name).data(),
                           (int)c_attr_get(s->nodes[0], name).size());
}

// ---- removeAttr(name) ----
inline JSValue fn_remove_attr(JSContext* ctx, JSValueConst this_val, int argc,
                              JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    size_t nlen = 0;
    const char* n = JS_ToCStringLen(ctx, &nlen, argv[0]);
    if (!n)
        return JS_EXCEPTION;
    std::string name(n, nlen);
    JS_FreeCString(ctx, n);
    for (lxb_dom_node_t* el : s->nodes)
        c_attr_remove(el, name);
    return JS_DupValue(ctx, this_val);
}

// ---- hasClass(name) ----
inline JSValue fn_has_class(JSContext* ctx, JSValueConst this_val, int argc,
                            JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1 || s->nodes.empty())
        return JS_FALSE;
    size_t nlen = 0;
    const char* n = JS_ToCStringLen(ctx, &nlen, argv[0]);
    if (!n)
        return JS_EXCEPTION;
    std::string name(n, nlen);
    JS_FreeCString(ctx, n);
    for (lxb_dom_node_t* el : s->nodes) {
        for (auto& c : c_class_list(el)) {
            if (c == name)
                return JS_TRUE;
        }
    }
    return JS_FALSE;
}

// ---- addClass / removeClass / toggleClass ----
inline void c_add_class(lxb_dom_node_t* el, const std::string& name)
{
    auto cls = c_class_list(el);
    for (auto& c : cls) {
        if (c == name)
            return;
    }
    cls.push_back(name);
    c_class_set(el, cls);
}

inline void c_remove_class(lxb_dom_node_t* el, const std::string& name)
{
    auto cls = c_class_list(el);
    auto it = cls.begin();
    while (it != cls.end()) {
        if (*it == name)
            it = cls.erase(it);
        else
            ++it;
    }
    c_class_set(el, cls);
}

inline void c_toggle_class(lxb_dom_node_t* el, const std::string& name)
{
    auto cls = c_class_list(el);
    for (auto it = cls.begin(); it != cls.end(); ++it) {
        if (*it == name) {
            cls.erase(it);
            c_class_set(el, cls);
            return;
        }
    }
    cls.push_back(name);
    c_class_set(el, cls);
}

inline JSValue fn_class_op(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv, int magic)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    size_t nlen = 0;
    const char* n = JS_ToCStringLen(ctx, &nlen, argv[0]);
    if (!n)
        return JS_EXCEPTION;
    std::string name(n, nlen);
    JS_FreeCString(ctx, n);
    for (lxb_dom_node_t* el : s->nodes) {
        if (magic == 0)
            c_add_class(el, name);
        else if (magic == 1)
            c_remove_class(el, name);
        else
            c_toggle_class(el, name);
    }
    return JS_DupValue(ctx, this_val);
}

// ---- 遍历 ----
inline JSValue fn_each(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1 || !JS_IsFunction(ctx, argv[0]))
        return JS_DupValue(ctx, this_val);
    for (size_t i = 0; i < s->nodes.size(); ++i) {
        JSValue el = make_node(ctx, s->nodes[i], s->ref);
        JSValue args[2] = {JS_NewUint32(ctx, (uint32_t)i), el};
        JSValue r = JS_Call(ctx, argv[0], el, 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, el);
        if (JS_IsException(r)) {
            JS_FreeValue(ctx, r);
            return JS_EXCEPTION;
        }
        // 仅严格 === false 中断（jQuery/cheerio 语义：fn(...) !== false）
        bool strict_false = JS_VALUE_GET_TAG(r) == JS_TAG_BOOL &&
                            !JS_VALUE_GET_BOOL(r);
        JS_FreeValue(ctx, r);
        if (strict_false)
            break;
    }
    return JS_DupValue(ctx, this_val);
}

// ---- filter(selector|fn) ----
// 无效选择器 → JS Error（css-select 兼容消息），不抛 C++ 异常
inline JSValue throw_invalid_selector(JSContext* ctx, const std::string& selector)
{
    std::string msg = "Invalid selector: " + selector;
    // 提取第一个 :name 片段（Unknown pseudo-class :bah 风格）
    size_t pos = selector.find(':');
    if (pos != std::string::npos && pos + 1 < selector.size()) {
        size_t end = pos + 1;
        while (end < selector.size() &&
               (std::isalnum((unsigned char)selector[end]) ||
                selector[end] == '-' || selector[end] == '_'))
            ++end;
        if (end > pos + 1)
            msg = "Unknown pseudo-class " + selector.substr(pos, end - pos);
    }
    JS_ThrowTypeError(ctx, "%s", msg.c_str());
    return JS_EXCEPTION;
}

// el 自身是否匹配 selector（is 语义：不遍历子树，O(1) 匹配）
inline bool c_matches(JSContext* ctx, lxb_dom_node_t* el,
                      const std::string& selector, bool* ok)
{
    lxb_css_selector_list_t* list = nullptr;
    lxb_css_parser_t* parser = parse_selectors(ctx, selector, &list);
    if (!parser) {
        if (ok)
            *ok = false;
        return false;
    }
    CTreeMatcher m{ctx, el};
    bool match = false;
    for (lxb_css_selector_list_t* l = list; l; l = l->next) {
        if (l->first != nullptr && m.match_chain(l->last, el)) {
            match = true;
            break;
        }
    }
    lxb_css_parser_destroy(parser, true);
    if (ok)
        *ok = true;
    return match;
}

inline JSValue fn_filter(JSContext* ctx, JSValueConst this_val, int argc,
                         JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    if (argc > 0 && JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel)
            return JS_EXCEPTION;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        for (lxb_dom_node_t* el : s->nodes) {
            if (c_matches(ctx, el, selector))
                out.push_back(el);
        }
    } else if (argc > 0 && !JS_IsUndefined(argv[0])) {
        // 函数 / $ / 节点（$ 可调用，不能只用 JS_IsFunction 判断）
        bool threw = false;
        out = c_filter_nodes(ctx, s->nodes, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- map(fn) ----
inline JSValue fn_map(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1 || !JS_IsFunction(ctx, argv[0]))
        return make_sel_or_empty(ctx, s, {});
    std::vector<lxb_dom_node_t*> out;
    for (size_t i = 0; i < s->nodes.size(); ++i) {
        JSValue el = make_node(ctx, s->nodes[i], s->ref);
        JSValue args[2] = {JS_NewUint32(ctx, (uint32_t)i), el};
        JSValue r = JS_Call(ctx, argv[0], el, 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, el);
        if (JS_IsException(r)) {
            JS_FreeValue(ctx, r);
            return JS_EXCEPTION;
        }
        // 回调返回 null/undefined 跳过；返回节点句柄则收集
        if (!JS_IsNull(r) && !JS_IsUndefined(r)) {
            if (NodeHandle* h = unwrap_node(ctx, r))
                out.push_back(h->node);
        }
        JS_FreeValue(ctx, r);
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- is(selector|fn)：任一元素匹配 ----
inline JSValue fn_is(JSContext* ctx, JSValueConst this_val, int argc,
                     JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_FALSE;
    if (JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel)
            return JS_EXCEPTION;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        for (lxb_dom_node_t* el : s->nodes) {
            if (c_matches(ctx, el, selector))
                return JS_TRUE;
        }
        return JS_FALSE;
    }
    // 函数 / $ / 节点（$ 可调用，不能只用 JS_IsFunction 判断）
    if (!JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        bool threw = false;
        for (size_t i = 0; i < s->nodes.size(); ++i) {
            if (c_match_pred(ctx, argv[0], s->nodes[i], i, s->ref, &threw))
                return JS_TRUE;
            if (threw)
                return JS_EXCEPTION;
        }
    }
    return JS_FALSE;
}

// ---- find(selector|$|node|array)：后代匹配（组合子相对选择器在文档根遍历，
// :scope 绑定每个选中元素——css-select 语义；非字符串参数 = 过滤出属于
// 本集合后代的部分）----
inline JSValue fn_find(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
        return make_sel_or_empty(ctx, s, {}, this_val);
    if (!JS_IsString(argv[0])) {
        // $ / 节点 / 数组：取候选节点，过滤出属于 this 后代的
        std::vector<lxb_dom_node_t*> candidates;
        if (CheerioSel* os = unwrap_sel(ctx, argv[0])) {
            candidates = os->nodes;
        } else if (NodeHandle* h = unwrap_node(ctx, argv[0])) {
            candidates = {h->node};
        } else if (JS_IsArray(argv[0])) {
            JSValue arr = JS_GetPropertyStr(ctx, argv[0], "length");
            uint32_t len = 0;
            if (!JS_IsException(arr)) {
                JS_ToUint32(ctx, &len, arr);
                JS_FreeValue(ctx, arr);
            }
            for (uint32_t i = 0; i < len; ++i) {
                JSValue v = JS_GetPropertyUint32(ctx, argv[0], i);
                if (NodeHandle* h = unwrap_node(ctx, v))
                    candidates.push_back(h->node);
                JS_FreeValue(ctx, v);
            }
        }
        std::vector<lxb_dom_node_t*> out;
        for (lxb_dom_node_t* c : candidates) {
            bool contained = false;
            for (lxb_dom_node_t* base : s->nodes) {
                for (lxb_dom_node_t* p = c; p; p = p->parent) {
                    if (p == base) {
                        contained = true;
                        break;
                    }
                }
                if (contained)
                    break;
            }
            if (contained)
                out.push_back(c);
        }
        return make_sel_or_empty(ctx, s, std::move(out), this_val);
    }
    size_t slen = 0;
    const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
    if (!sel)
        return JS_EXCEPTION;
    std::string selector(sel, slen);
    JS_FreeCString(ctx, sel);
    std::vector<lxb_dom_node_t*> out;
    bool ok = true;
    lxb_dom_node_t* doc_root = &s->ref->doc->dom_document.node;
    // 相对选择器（> + ~ 开头）需要兄弟上下文 → 文档根遍历 + :scope；
    // 普通选择器只查每个元素的子树（find 的 descendants-only 语义）
    size_t sb = selector.find_first_not_of(" \t\r\n");
    bool relative = sb != std::string::npos && sb < selector.size() &&
                    (selector[sb] == '>' || selector[sb] == '+' || selector[sb] == '~');
    // :scope 匹配元素自身 → 查询需包含自身
    bool scoped = selector.find(":scope") != std::string::npos;
    for (lxb_dom_node_t* el : s->nodes) {
        std::vector<lxb_dom_node_t*> r = c_query_selector(
            ctx, relative ? doc_root : el, selector, scoped,
            relative ? el : nullptr, &ok);
        if (!ok)
            return throw_invalid_selector(ctx, selector);
        for (lxb_dom_node_t* n : r) {
            bool dup = false;
            for (lxb_dom_node_t* o : out) {
                if (o == n) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                out.push_back(n);
        }
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- children / parent / parents / siblings / next / prev（均支持 selector
// 过滤，对应 JS 版 _matcher/_singleMatcher 的 filterArray） ----
inline JSValue fn_children(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* c = el->first_child; c; c = c->next) {
            if (c_node_is_element(c))
                out.push_back(c);
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

inline JSValue fn_parent(JSContext* ctx, JSValueConst this_val, int argc,
                         JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        if (el->parent && c_node_is_element(el->parent))
            out.push_back(el->parent);
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

inline JSValue fn_parents(JSContext* ctx, JSValueConst this_val, int argc,
                          JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* p = el->parent; p; p = p->parent) {
            if (c_node_is_element(p))
                out.push_back(p);
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    // parents：uniqueSort（文档序）+ reverse（近→远，domutils 语义）
    std::vector<lxb_dom_node_t*> sorted = c_unique_sort(std::move(out));
    std::reverse(sorted.begin(), sorted.end());
    return make_sel_or_empty(ctx, s, std::move(sorted), this_val);
}

// parent：_singleMatcher + _removeDuplicates（保持顺序去重）
inline JSValue fn_parent_dedup(JSContext* ctx, JSValueConst this_val, int argc,
                               JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        if (el->parent && c_node_is_element(el->parent))
            out.push_back(el->parent);
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, c_remove_dups(std::move(out)), this_val);
}

inline JSValue fn_siblings(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        if (!el->parent)
            continue;
        for (lxb_dom_node_t* c = el->parent->first_child; c; c = c->next) {
            if (c_node_is_element(c) && c != el)
                out.push_back(c);
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, c_unique_sort(std::move(out)), this_val);
}

inline JSValue fn_next(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* nx = el->next; nx; nx = nx->next) {
            if (c_node_is_element(nx)) {
                out.push_back(nx);
                break;
            }
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

inline JSValue fn_prev(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* p = el->prev; p; p = p->prev) {
            if (c_node_is_element(p)) {
                out.push_back(p);
                break;
            }
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- first / last / eq / slice ----
inline JSValue fn_first(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    if (!s->nodes.empty())
        out.push_back(s->nodes[0]);
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

inline JSValue fn_last(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    if (!s->nodes.empty())
        out.push_back(s->nodes.back());
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

inline JSValue fn_eq(JSContext* ctx, JSValueConst this_val, int argc,
                     JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    int32_t idx = 0;
    JS_ToInt32(ctx, &idx, argv[0]);
    std::vector<lxb_dom_node_t*> out;
    if (idx < 0)
        idx = (int32_t)s->nodes.size() + idx;
    if (idx >= 0 && (size_t)idx < s->nodes.size())
        out.push_back(s->nodes[(size_t)idx]);
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- get([i]) / toArray()：节点句柄访问（$[i] 同一对象缓存）----
inline JSValue fn_get(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    if (argc < 1 || JS_IsUndefined(argv[0])) {
        JSValue arr = JS_NewArray(ctx);
        uint32_t i = 0;
        for (lxb_dom_node_t* n : s->nodes)
            JS_SetPropertyUint32(ctx, arr, i++, make_node(ctx, n, s->ref));
        return arr;
    }
    int32_t idx = 0;
    JS_ToInt32(ctx, &idx, argv[0]);
    if (idx < 0)
        idx = (int32_t)s->nodes.size() + idx;
    if (idx >= 0 && (size_t)idx < s->nodes.size())
        return make_node(ctx, s->nodes[(size_t)idx], s->ref);
    return JS_UNDEFINED;
}

inline JSValue fn_to_array(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    return fn_get(ctx, this_val, 0, nullptr);
}

// ---- index([selectorOrNeedle]) ----
inline JSValue fn_index(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> haystack;
    lxb_dom_node_t* needle = nullptr;
    if (argc < 1 || JS_IsUndefined(argv[0])) {
        // 无参：this[0] 在父元素 children 中的索引
        if (s->nodes.empty())
            return JS_NewInt32(ctx, -1);
        needle = s->nodes[0];
        if (!needle->parent)
            return JS_NewInt32(ctx, -1);
        int32_t idx = 0;
        for (lxb_dom_node_t* c = needle->parent->first_child; c; c = c->next) {
            if (c == needle)
                return JS_NewInt32(ctx, idx);
            if (c_node_is_element(c))
                ++idx;
        }
        return JS_NewInt32(ctx, -1);
    }
    if (JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel)
            return JS_EXCEPTION;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        lxb_dom_node_t* root = &s->ref->doc->dom_document.node;
        haystack = c_query_selector(ctx, root, selector, false);
        if (!s->nodes.empty())
            needle = s->nodes[0];
    } else if (CheerioSel* os = unwrap_sel(ctx, argv[0])) {
        haystack = s->nodes;
        if (!os->nodes.empty())
            needle = os->nodes[0];
    } else if (NodeHandle* h = unwrap_node(ctx, argv[0])) {
        haystack = s->nodes;
        needle = h->node;
    }
    for (size_t i = 0; i < haystack.size(); ++i) {
        if (haystack[i] == needle)
            return JS_NewInt32(ctx, (int32_t)i);
    }
    return JS_NewInt32(ctx, -1);
}

// ---- slice(start[, end]) ----
inline JSValue fn_slice(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    int32_t start = 0, end = (int32_t)s->nodes.size();
    if (argc > 0 && !JS_IsUndefined(argv[0]))
        JS_ToInt32(ctx, &start, argv[0]);
    if (argc > 1 && !JS_IsUndefined(argv[1]))
        JS_ToInt32(ctx, &end, argv[1]);
    if (start < 0)
        start = (int32_t)s->nodes.size() + start;
    if (end < 0)
        end = (int32_t)s->nodes.size() + end;
    if (start < 0)
        start = 0;
    if (end > (int32_t)s->nodes.size())
        end = (int32_t)s->nodes.size();
    std::vector<lxb_dom_node_t*> out;
    if (start < end) {
        for (int32_t i = start; i < end; ++i)
            out.push_back(s->nodes[(size_t)i]);
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- end()：回退到 prevObject（C++ 字段，见 CheerioSel::prev）----
inline JSValue fn_end(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    if (!JS_IsUndefined(s->prev))
        return JS_DupValue(ctx, s->prev);
    return make_sel_or_empty(ctx, s, {});
}

// ---- add(other[, context])：合并去重（文档序）----
inline std::vector<lxb_dom_node_t*> c_sel_nodes_of(JSContext* ctx, CheerioSel* base,
                                                   JSValueConst v)
{
    std::vector<lxb_dom_node_t*> out;
    if (JS_IsString(v)) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, v);
        if (!sel)
            return out;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        size_t b = selector.find_first_not_of(" \t\r\n");
        if (b != std::string::npos && selector[b] == '<') {
            // add('<li>...')：fragment 解析
            lxb_dom_node_t* doc_node = &base->ref->doc->dom_document.node;
            return c_parse_fragment(ctx, doc_node, selector);
        }
        lxb_dom_node_t* root = &base->ref->doc->dom_document.node;
        return c_query_selector(ctx, root, selector, false);
    }
    if (CheerioSel* os = unwrap_sel(ctx, v))
        return os->nodes;
    if (NodeHandle* h = unwrap_node(ctx, v))
        return {h->node};
    if (JS_IsArray(v)) {
        JSValue arr = JS_GetPropertyStr(ctx, v, "length");
        uint32_t len = 0;
        if (!JS_IsException(arr)) {
            JS_ToUint32(ctx, &len, arr);
            JS_FreeValue(ctx, arr);
        }
        for (uint32_t i = 0; i < len; ++i) {
            JSValue item = JS_GetPropertyUint32(ctx, v, i);
            if (NodeHandle* h = unwrap_node(ctx, item))
                out.push_back(h->node);
            JS_FreeValue(ctx, item);
        }
        return out;
    }
    return out;
}

inline JSValue fn_add(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out = s->nodes;
    // add(selector, context)：argv[1] 是查询上下文，不作为独立元素加入
    bool has_ctx = argc > 1 && JS_IsString(argv[0]) && !JS_IsUndefined(argv[1]) &&
                   !JS_IsNull(argv[1]);
    for (int i = 0; i < argc; ++i) {
        if (i == 1 && has_ctx)
            continue;
        std::vector<lxb_dom_node_t*> add;
        // add(selector, context)：查询限定在 context 内
        if (i == 0 && has_ctx) {
            if (JS_IsString(argv[1])) {
                size_t clen = 0;
                const char* cstr = JS_ToCStringLen(ctx, &clen, argv[1]);
                if (!cstr)
                    return JS_EXCEPTION;
                std::string context_str(cstr, clen);
                JS_FreeCString(ctx, cstr);
                size_t cb = context_str.find_first_not_of(" \t\r\n");
                if (cb != std::string::npos && context_str[cb] == '<') {
                    // context 是 HTML：fragment 节点（含自身）内查询
                    lxb_dom_node_t* doc_node = &s->ref->doc->dom_document.node;
                    std::vector<lxb_dom_node_t*> ctx_nodes =
                        c_parse_fragment(ctx, doc_node, context_str);
                    size_t slen = 0;
                    const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
                    if (!sel)
                        return JS_EXCEPTION;
                    std::string selector(sel, slen);
                    JS_FreeCString(ctx, sel);
                    bool ok = true;
                    for (lxb_dom_node_t* n : ctx_nodes) {
                        std::vector<lxb_dom_node_t*> r = c_query_selector(
                            ctx, n, selector, true, nullptr, &ok);
                        if (!ok)
                            return throw_invalid_selector(ctx, selector);
                        add.insert(add.end(), r.begin(), r.end());
                    }
                } else {
                    // context 是选择器：组合查询 "ctx sel"
                    size_t slen = 0;
                    const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
                    if (!sel)
                        return JS_EXCEPTION;
                    std::string selector(sel, slen);
                    JS_FreeCString(ctx, sel);
                    lxb_dom_node_t* root = &s->ref->doc->dom_document.node;
                    add = c_query_selector(ctx, root,
                                           context_str + " " + selector, false);
                }
            } else {
                // context 是 $ / 节点：其节点（含自身）内查询
                std::vector<lxb_dom_node_t*> ctx_nodes;
                if (CheerioSel* cs = unwrap_sel(ctx, argv[1]))
                    ctx_nodes = cs->nodes;
                else if (NodeHandle* h = unwrap_node(ctx, argv[1]))
                    ctx_nodes = {h->node};
                size_t slen = 0;
                const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
                if (!sel)
                    return JS_EXCEPTION;
                std::string selector(sel, slen);
                JS_FreeCString(ctx, sel);
                bool ok = true;
                for (lxb_dom_node_t* n : ctx_nodes) {
                    std::vector<lxb_dom_node_t*> r = c_query_selector(
                        ctx, n, selector, true, nullptr, &ok);
                    if (!ok)
                        return throw_invalid_selector(ctx, selector);
                    add.insert(add.end(), r.begin(), r.end());
                }
            }
        } else {
            add = c_sel_nodes_of(ctx, s, argv[i]);
        }
        out.insert(out.end(), add.begin(), add.end());
    }
    return make_sel_or_empty(ctx, s, c_unique_sort(std::move(out)), this_val);
}

// ---- addBack([selector]) ----
inline JSValue fn_add_back(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    // 无 prevObject：返回自身（JS 版 this.prevObject ? ... : this）
    if (JS_IsUndefined(s->prev))
        return JS_DupValue(ctx, this_val);
    JSValue prev = JS_DupValue(ctx, s->prev);
    std::vector<lxb_dom_node_t*> out = s->nodes;
    CheerioSel* ps = unwrap_sel(ctx, prev);
    if (ps) {
        if (argc > 0 && !JS_IsUndefined(argv[0])) {
            bool threw = false;
            std::vector<lxb_dom_node_t*> filtered =
                c_filter_nodes(ctx, ps->nodes, argv[0], s->ref, &threw);
            if (threw) {
                JS_FreeValue(ctx, prev);
                return JS_EXCEPTION;
            }
            out.insert(out.end(), filtered.begin(), filtered.end());
        } else {
            out.insert(out.end(), ps->nodes.begin(), ps->nodes.end());
        }
    }
    JS_FreeValue(ctx, prev);
    return make_sel_or_empty(ctx, s, c_unique_sort(std::move(out)), this_val);
}

// ---- not(match)：排除匹配元素 ----
inline JSValue fn_not(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    if (argc < 1 || JS_IsUndefined(argv[0])) {
        out = s->nodes;
    } else if (JS_IsString(argv[0])) {
        bool threw = false;
        std::vector<lxb_dom_node_t*> matches =
            c_filter_nodes(ctx, s->nodes, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
        std::unordered_set<lxb_dom_node_t*> excl(matches.begin(), matches.end());
        for (lxb_dom_node_t* n : s->nodes) {
            if (!excl.count(n))
                out.push_back(n);
        }
    } else {
        bool threw = false;
        for (size_t i = 0; i < s->nodes.size(); ++i) {
            if (!c_match_pred(ctx, argv[0], s->nodes[i], i, s->ref, &threw))
                out.push_back(s->nodes[i]);
            if (threw)
                return JS_EXCEPTION;
        }
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- has(selectorOrNode)：保留含匹配后代的元素 ----
inline JSValue fn_has(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    if (argc > 0 && JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel)
            return JS_EXCEPTION;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        for (lxb_dom_node_t* n : s->nodes) {
            std::vector<lxb_dom_node_t*> r =
                c_query_selector(ctx, n, selector, false);
            if (!r.empty())
                out.push_back(n);
        }
    } else {
        lxb_dom_node_t* needle = nullptr;
        if (argc > 0) {
            if (CheerioSel* os = unwrap_sel(ctx, argv[0])) {
                if (!os->nodes.empty())
                    needle = os->nodes[0];
            } else if (NodeHandle* h = unwrap_node(ctx, argv[0])) {
                needle = h->node;
            }
        }
        if (needle) {
            for (lxb_dom_node_t* n : s->nodes) {
                bool found = false;
                std::vector<lxb_dom_node_t*> stack;
                for (lxb_dom_node_t* c = n->first_child; c; c = c->next)
                    stack.push_back(c);
                while (!stack.empty()) {
                    lxb_dom_node_t* cur = stack.back();
                    stack.pop_back();
                    if (cur == needle) {
                        found = true;
                        break;
                    }
                    for (lxb_dom_node_t* c = cur->first_child; c; c = c->next)
                        stack.push_back(c);
                }
                if (found)
                    out.push_back(n);
            }
        }
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- contents()：所有子节点（含文本/注释）----
inline JSValue fn_contents(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* c = el->first_child; c; c = c->next)
            out.push_back(c);
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- closest(selector|fn)：自身起向上第一个匹配 ----
inline JSValue fn_closest(JSContext* ctx, JSValueConst this_val, int argc,
                          JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
        return make_sel_or_empty(ctx, s, {}, this_val);
    std::vector<lxb_dom_node_t*> out;
    if (JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel)
            return JS_EXCEPTION;
        std::string selector(sel, slen);
        JS_FreeCString(ctx, sel);
        for (lxb_dom_node_t* el : s->nodes) {
            for (lxb_dom_node_t* p = el; p; p = p->parent) {
                if (c_node_is_element(p) && c_matches(ctx, p, selector)) {
                    out.push_back(p);
                    break;
                }
            }
        }
    } else if (JS_IsFunction(ctx, argv[0])) {
        for (lxb_dom_node_t* el : s->nodes) {
            for (lxb_dom_node_t* p = el; p; p = p->parent) {
                if (!c_node_is_element(p))
                    continue;
                JSValue node_v = make_node(ctx, p, s->ref);
                JSValue args[2] = {JS_NewInt32(ctx, 0), node_v};
                JSValue r = JS_Call(ctx, argv[0], node_v, 2, args);
                JS_FreeValue(ctx, args[0]);
                JS_FreeValue(ctx, node_v);
                if (JS_IsException(r)) {
                    JS_FreeValue(ctx, r);
                    return JS_EXCEPTION;
                }
                bool keep = JS_ToBool(ctx, r);
                JS_FreeValue(ctx, r);
                if (keep) {
                    out.push_back(p);
                    break;
                }
            }
        }
    }
    return make_sel_or_empty(ctx, s, c_remove_dups(std::move(out)), this_val);
}

// ---- clone()：深复制节点（lxb_dom_node_clone 跨文档有 tag 表问题，
// 用自实现 c_copy_node 复制到同一文档）----
inline JSValue fn_clone(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        if (!el || el->type == LXB_DOM_NODE_TYPE_DOCUMENT)
            continue;
        lxb_dom_document_t* doc = lxb_dom_interface_document(el->owner_document);
        lxb_dom_node_t* cp = c_copy_node(doc, el);
        if (cp)
            out.push_back(cp);
    }
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- nextAll / prevAll（近→远，支持 selector 过滤）----
inline JSValue fn_next_all(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* nx = el->next; nx; nx = nx->next) {
            if (c_node_is_element(nx))
                out.push_back(nx);
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, c_remove_dups(std::move(out)), this_val);
}

inline JSValue fn_prev_all(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        for (lxb_dom_node_t* p = el->prev; p; p = p->prev) {
            if (c_node_is_element(p))
                out.push_back(p);
        }
    }
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[0], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    return make_sel_or_empty(ctx, s, c_remove_dups(std::move(out)), this_val);
}

// ---- *Until(selector[, filter])：遍历到匹配 selector 处停止（不含）----
inline JSValue fn_until_impl(JSContext* ctx, JSValueConst this_val, int argc,
                             JSValueConst* argv, int mode)
{
    // mode: 0=nextUntil 1=prevUntil 2=parentsUntil
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    // 停止条件：selector 字符串 → 元素匹配；函数 → 谓词；null/undefined → 无
    bool has_stop = argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]);
    std::vector<lxb_dom_node_t*> out;
    for (lxb_dom_node_t* el : s->nodes) {
        lxb_dom_node_t* cur = el;
        while (true) {
            lxb_dom_node_t* next = nullptr;
            if (mode == 0) {
                for (lxb_dom_node_t* nx = cur->next; nx; nx = nx->next) {
                    if (c_node_is_element(nx)) {
                        next = nx;
                        break;
                    }
                }
            } else if (mode == 1) {
                for (lxb_dom_node_t* p = cur->prev; p; p = p->prev) {
                    if (c_node_is_element(p)) {
                        next = p;
                        break;
                    }
                }
            } else {
                next = cur->parent;
                if (next && !c_node_is_element(next))
                    next = nullptr; // 文档节点不算
            }
            if (!next)
                break;
            // 停止检查
            bool stop = false;
            if (has_stop) {
                if (JS_IsString(argv[0])) {
                    size_t slen = 0;
                    const char* sel = JS_ToCStringLen(ctx, &slen, argv[0]);
                    if (!sel)
                        return JS_EXCEPTION;
                    std::string selector(sel, slen);
                    JS_FreeCString(ctx, sel);
                    stop = c_matches(ctx, next, selector);
                } else {
                    bool threw = false;
                    stop = c_match_pred(ctx, argv[0], next, out.size(), s->ref, &threw);
                    if (threw)
                        return JS_EXCEPTION;
                }
            }
            if (stop)
                break;
            out.push_back(next);
            cur = next;
        }
    }
    if (argc > 1 && !JS_IsUndefined(argv[1])) {
        bool threw = false;
        out = c_filter_nodes(ctx, out, argv[1], s->ref, &threw);
        if (threw)
            return JS_EXCEPTION;
    }
    if (mode == 2) {
        // parentsUntil：uniqueSort（文档序）+ reverse（近→远，domutils 语义）
        std::vector<lxb_dom_node_t*> sorted = c_unique_sort(std::move(out));
        std::reverse(sorted.begin(), sorted.end());
        return make_sel_or_empty(ctx, s, std::move(sorted), this_val);
    }
    return make_sel_or_empty(ctx, s, c_remove_dups(std::move(out)), this_val);
}

inline JSValue fn_next_until(JSContext* ctx, JSValueConst this_val, int argc,
                             JSValueConst* argv)
{
    return fn_until_impl(ctx, this_val, argc, argv, 0);
}

inline JSValue fn_prev_until(JSContext* ctx, JSValueConst this_val, int argc,
                             JSValueConst* argv)
{
    return fn_until_impl(ctx, this_val, argc, argv, 1);
}

inline JSValue fn_parents_until(JSContext* ctx, JSValueConst this_val, int argc,
                                JSValueConst* argv)
{
    return fn_until_impl(ctx, this_val, argc, argv, 2);
}

// ---- root()：文档根元素 ----
inline JSValue fn_root(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    lxb_dom_node_t* doc = &s->ref->doc->dom_document.node;
    lxb_dom_node_t* root_el = nullptr;
    for (lxb_dom_node_t* c = doc->first_child; c; c = c->next) {
        if (c_node_is_element(c)) {
            root_el = c;
            break;
        }
    }
    std::vector<lxb_dom_node_t*> out;
    if (root_el)
        out.push_back(root_el);
    return make_sel_or_empty(ctx, s, std::move(out), this_val);
}

// ---- prop(name)：属性/标签名读取（tagName 大写，其余同 attr）----
inline JSValue fn_prop(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1 || !JS_IsString(argv[0]))
        return JS_UNDEFINED;
    size_t slen = 0;
    const char* name = JS_ToCStringLen(ctx, &slen, argv[0]);
    if (!name)
        return JS_EXCEPTION;
    std::string prop_name(name, slen);
    JS_FreeCString(ctx, name);
    if (prop_name == "tagName") {
        if (s->nodes.empty())
            return JS_UNDEFINED;
        std::string tag = c_node_name(s->nodes[0]);
        for (auto& ch : tag)
            ch = (char)std::toupper((unsigned char)ch);
        return JS_NewString(ctx, tag.c_str());
    }
    // 其余按属性读取（与 attr 一致；prop 对无属性返回 undefined）
    if (s->nodes.empty())
        return JS_UNDEFINED;
    return fn_attr(ctx, this_val, argc, argv);
}

// ---- remove / empty ----
inline JSValue fn_remove(JSContext* ctx, JSValueConst this_val, int argc,
                         JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    for (lxb_dom_node_t* el : s->nodes)
        lxb_dom_node_remove(el);
    return JS_DupValue(ctx, this_val);
}

inline JSValue fn_empty(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s)
        return JS_UNDEFINED;
    for (lxb_dom_node_t* el : s->nodes) {
        while (el->first_child)
            lxb_dom_node_remove(el->first_child);
    }
    return JS_DupValue(ctx, this_val);
}

// ---- append(content) / prepend(content) ----
inline void c_append_content(JSContext* ctx, lxb_dom_node_t* parent,
                             JSValueConst content)
{
    if (JS_IsString(content)) {
        size_t hlen = 0;
        const char* h = JS_ToCStringLen(ctx, &hlen, content);
        if (!h)
            return;
        std::string html(h, hlen);
        JS_FreeCString(ctx, h);
        std::vector<lxb_dom_node_t*> frag =
            c_parse_fragment(ctx, parent, html);
        for (lxb_dom_node_t* n : frag)
            lxb_dom_node_insert_child(parent, n);
    } else if (NodeHandle* h = unwrap_node(ctx, content)) {
        // 移动节点（或克隆？cheerio 是移动）
        if (h->node != parent && h->node->parent)
            lxb_dom_node_remove(h->node);
        lxb_dom_node_insert_child(parent, h->node);
    } else if (CheerioSel* cs = unwrap_sel(ctx, content)) {
        for (lxb_dom_node_t* n : cs->nodes) {
            if (n->parent)
                lxb_dom_node_remove(n);
            lxb_dom_node_insert_child(parent, n);
        }
    }
}

inline JSValue fn_append(JSContext* ctx, JSValueConst this_val, int argc,
                         JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    for (lxb_dom_node_t* el : s->nodes)
        c_append_content(ctx, el, argv[0]);
    return JS_DupValue(ctx, this_val);
}

inline JSValue fn_prepend(JSContext* ctx, JSValueConst this_val, int argc,
                          JSValueConst* argv)
{
    CheerioSel* s = sel_of(ctx, this_val);
    if (!s || argc < 1)
        return JS_UNDEFINED;
    for (lxb_dom_node_t* el : s->nodes) {
        if (JS_IsString(argv[0])) {
            size_t hlen = 0;
            const char* h = JS_ToCStringLen(ctx, &hlen, argv[0]);
            if (!h)
                return JS_EXCEPTION;
            std::string html(h, hlen);
            JS_FreeCString(ctx, h);
            std::vector<lxb_dom_node_t*> frag =
                c_parse_fragment(ctx, el, html);
            for (lxb_dom_node_t* n : frag)
                lxb_dom_node_insert_before(el, n);
        }
    }
    return JS_DupValue(ctx, this_val);
}

// ---------------------------------------------------------------------------
// 方法注册
// ---------------------------------------------------------------------------
inline void reg_sel_method(JSContext* ctx, const char* name, SelFn fn,
                           int nargs = 1)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, nargs);
    JS_SetPropertyStr(ctx, class_ids(JS_GetRuntime(ctx)).sel_proto, name, f);
    // 注意：这里不 FreeValue(f)——JS_SetPropertyStr 内部会 dup，f 的原始
    // 引用如果释放，后续属性查找会 double-free（QuickJS 的 CFunction 对象
    // 经 SetPropertyStr 后 refcount 归零即被回收）。由 runtime 结束时统一清理。
    (void)f;
}

inline void reg_sel_method_magic(JSContext* ctx, const char* name,
                                 JSValue (*fn)(JSContext*, JSValueConst, int,
                                               JSValueConst*, int),
                                 int magic)
{
    JSValue f = JS_NewCFunctionMagic(ctx, fn, name, 1, JS_CFUNC_generic_magic,
                                     magic);
    JS_SetPropertyStr(ctx, class_ids(JS_GetRuntime(ctx)).sel_proto, name,
                      JS_DupValue(ctx, f));
    JS_FreeValue(ctx, f);
}


// ---------------------------------------------------------------------------
// 安装入口：cheerio.load + $ 原型方法 + 测试入口
// ---------------------------------------------------------------------------
inline void install_cheerio_fast(qjs::Context& ctx)
{
    JSContext* jctx = ctx.raw();
    register_classes(JS_GetRuntime(jctx), jctx);

    // $ 原型方法
    reg_sel_method(jctx, "html", fn_html_get, 1);
    reg_sel_method(jctx, "text", fn_text, 1);
    reg_sel_method(jctx, "attr", fn_attr, 1);
    reg_sel_method(jctx, "removeAttr", fn_remove_attr, 1);
    reg_sel_method(jctx, "hasClass", fn_has_class, 1);
    reg_sel_method_magic(jctx, "addClass", fn_class_op, 0);
    reg_sel_method_magic(jctx, "removeClass", fn_class_op, 1);
    reg_sel_method_magic(jctx, "toggleClass", fn_class_op, 2);
    reg_sel_method(jctx, "each", fn_each, 1);
    reg_sel_method(jctx, "filter", fn_filter, 1);
    reg_sel_method(jctx, "map", fn_map, 1);
    reg_sel_method(jctx, "is", fn_is, 1);
    reg_sel_method(jctx, "find", fn_find, 1);
    reg_sel_method(jctx, "children", fn_children, 0);
    reg_sel_method(jctx, "parent", fn_parent_dedup, 0);
    reg_sel_method(jctx, "parents", fn_parents, 0);
    reg_sel_method(jctx, "siblings", fn_siblings, 0);
    reg_sel_method(jctx, "next", fn_next, 0);
    reg_sel_method(jctx, "prev", fn_prev, 0);
    reg_sel_method(jctx, "first", fn_first, 0);
    reg_sel_method(jctx, "last", fn_last, 0);
    reg_sel_method(jctx, "eq", fn_eq, 1);
    reg_sel_method(jctx, "get", fn_get, 1);
    reg_sel_method(jctx, "toArray", fn_to_array, 0);
    reg_sel_method(jctx, "index", fn_index, 1);
    reg_sel_method(jctx, "slice", fn_slice, 2);
    reg_sel_method(jctx, "end", fn_end, 0);
    reg_sel_method(jctx, "add", fn_add, 1);
    reg_sel_method(jctx, "addBack", fn_add_back, 1);
    reg_sel_method(jctx, "not", fn_not, 1);
    reg_sel_method(jctx, "has", fn_has, 1);
    reg_sel_method(jctx, "contents", fn_contents, 0);
    reg_sel_method(jctx, "closest", fn_closest, 1);
    reg_sel_method(jctx, "nextAll", fn_next_all, 1);
    reg_sel_method(jctx, "prevAll", fn_prev_all, 1);
    reg_sel_method(jctx, "nextUntil", fn_next_until, 2);
    reg_sel_method(jctx, "prevUntil", fn_prev_until, 2);
    reg_sel_method(jctx, "parentsUntil", fn_parents_until, 2);
    reg_sel_method(jctx, "root", fn_root, 0);
    reg_sel_method(jctx, "clone", fn_clone, 0);
    reg_sel_method(jctx, "prop", fn_prop, 1);
    reg_sel_method(jctx, "remove", fn_remove, 0);
    reg_sel_method(jctx, "empty", fn_empty, 0);
    reg_sel_method(jctx, "append", fn_append, 1);
    reg_sel_method(jctx, "prepend", fn_prepend, 1);

    // cheerio = { load(html) -> $ }
    // 用裸 C 函数注册：无参 load() 须抛 "cheerio.load() expects a string"
    // （qjsbind 的参数校验消息不匹配 spec）
    qjs::Object cheerio_obj(jctx, JS_NewObject(jctx));
    JSValue load_cfn = JS_NewCFunction(
        jctx,
        [](JSContext* jctx, JSValueConst this_val, int argc,
           JSValueConst* argv) -> JSValue {
            if (argc < 1 || !JS_IsString(argv[0])) {
                // load([])（fixtures: cheerio = load([])）→ 空文档 $
                if (argc >= 1 && JS_IsArray(argv[0])) {
                    JSValue arr = JS_GetPropertyStr(jctx, argv[0], "length");
                    uint32_t len = 0;
                    if (!JS_IsException(arr)) {
                        JS_ToUint32(jctx, &len, arr);
                        JS_FreeValue(jctx, arr);
                    }
                    DomRef* ref = parse_document(jctx, "");
                    if (!ref)
                        return JS_EXCEPTION;
                    std::vector<lxb_dom_node_t*> nodes;
                    for (uint32_t i = 0; i < len; ++i) {
                        JSValue v = JS_GetPropertyUint32(jctx, argv[0], i);
                        if (NodeHandle* h = unwrap_node(jctx, v))
                            nodes.push_back(h->node);
                        JS_FreeValue(jctx, v);
                    }
                    return make_sel(jctx, ref, std::move(nodes));
                }
                JS_ThrowTypeError(jctx, "cheerio.load() expects a string");
                return JS_EXCEPTION;
            }
            size_t len = 0;
            const char* s = JS_ToCStringLen(jctx, &len, argv[0]);
            std::string html(s ? s : "", s ? len : 0);
            if (s)
                JS_FreeCString(jctx, s);
            DomRef* ref = parse_document(jctx, html);
            if (!ref)
                return JS_EXCEPTION;
            std::vector<lxb_dom_node_t*> nodes;
            nodes.push_back(&ref->doc->dom_document.node);
            return make_sel(jctx, ref, std::move(nodes));
        },
        "load", 1);
    cheerio_obj.set("load", qjs::Value(jctx, load_cfn));
    ctx.globals().set("cheerio", cheerio_obj);

    // Symbol.iterator：$ 可迭代（for...of / 展开，yield 每个元素句柄）
    ctx.eval(R"JS(
        const __proto = Object.getPrototypeOf(cheerio.load('<i></i>'));
        __proto[Symbol.iterator] = function* () {
            for (let i = 0; i < this.length; i++) yield this[i];
        };
        // Cheerio class：$ 实例 instanceof Cheerio（spec 断言）
        function Cheerio() {}
        Cheerio.prototype = __proto;
        cheerio.Cheerio = Cheerio;
        1;
    )JS");

    // 测试入口
    ctx.globals().set("__lxb_load", qjs::func(jctx, [](qjs::Ctx c, std::string html) -> qjs::Value {
        JSContext* jctx = c.ctx;
        DomRef* ref = parse_document(jctx, html);
        if (!ref)
            return qjs::Value(jctx, JS_EXCEPTION);
        std::vector<lxb_dom_node_t*> nodes;
        nodes.push_back(&ref->doc->dom_document.node);
        return qjs::Value(jctx, make_sel(jctx, ref, std::move(nodes)));
    }));
    ctx.globals().set("__lxb_queryAll", qjs::func(jctx, [](qjs::Ctx c, qjs::Value sel_v, std::string selector) -> qjs::Value {
        JSContext* jctx = c.ctx;
        CheerioSel* s = unwrap_sel(jctx, sel_v.raw());
        if (!s)
            qjs::throw_type_error(jctx, "__lxb_queryAll: expected cheerio selection");
        lxb_dom_node_t* root = &s->ref->doc->dom_document.node;
        std::vector<lxb_dom_node_t*> nodes =
            c_query_selector(jctx, root, selector, false);
        return qjs::Value(jctx, make_sel(jctx, s->ref, std::move(nodes)));
    }));
}

} // namespace qjsbind::cheerio::lxb_handle
