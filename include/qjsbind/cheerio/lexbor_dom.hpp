// lexbor_dom.hpp —— lexbor HTML 解析器 + CSS 选择器绑定（qjsbind::cheerio 内部）
//
// 为 cheerio 兼容层提供两个内部 JS 函数：
//   __lexbor_parse(html, isDocument, contextTag) -> 根节点对象
//       解析 HTML 为 domhandler 兼容的纯 JS 对象树：
//       root:  { type:'root', children:[...] }
//       tag:   { type:'tag'|'script'|'style', name, attribs:{...}, children:[...],
//                parent, prev, next }
//       text:  { type:'text', data }
//       comment:{ type:'comment', data }
//       doctype:{ type:'directive', data:'!DOCTYPE html', name:'html' }
//   __lexbor_queryAll(root, selector) -> 元素节点数组（文档序，含 root 后代）
//       选择器由 lexbor CSS selectors 解析，匹配在 JS 树上进行（实时反映
//       cheerio 的 append/remove 等修改）。
//
// lexbor 仅作为一次性解析器：JS 树是独立副本，解析完即销毁 lxb 文档。
#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <lexbor/css/css.h>
#include <lexbor/css/selectors/selectors.h>
#include <lexbor/dom/dom.h>
#include <lexbor/html/parser.h>
#include <lexbor/ns/ns.h>
#include <lexbor/tag/tag.h>

#include <quickjs.h>

#include <qjsbind/convert.hpp>
#include <qjsbind/error.hpp>
#include <qjsbind/function.hpp>
#include <qjsbind/value.hpp>

namespace qjsbind::cheerio {

namespace lxb_detail {

// ---------------------------------------------------------------------------
// JS 树小工具（全部在 JS 对象树上操作）
// ---------------------------------------------------------------------------

inline qjs::Value js_get(JSContext* ctx, JSValue obj, const char* prop)
{
    return qjs::Value(ctx, JS_GetPropertyStr(ctx, obj, prop));
}

inline std::string js_str(JSContext* ctx, JSValue v)
{
    // 异常值不转为字符串（pending exception 由调用方/引擎统一处理）
    if (JS_IsException(v))
        return {};
    size_t len = 0;
    const char* s = JS_ToCStringLen(ctx, &len, v);
    if (!s)
        return {};
    std::string out(s, len);
    JS_FreeCString(ctx, s);
    return out;
}

// 数组长度（quickjs-ng 无 JS_GetArrayLength）
inline bool js_array_length(JSContext* ctx, JSValue arr, uint32_t* out)
{
    JSValue len = JS_GetPropertyStr(ctx, arr, "length");
    if (JS_IsException(len))
        return false;
    JS_ToUint32(ctx, out, len);
    JS_FreeValue(ctx, len);
    return true;
}

inline bool js_is_element(JSContext* ctx, JSValue node)
{
    qjs::Value t = js_get(ctx, node, "type");
    if (!t.is_string())
        return false;
    std::string type = js_str(ctx, t.raw());
    return type == "tag" || type == "script" || type == "style";
}

inline bool js_has_class(JSContext* ctx, JSValue el, const std::string& cls)
{
    qjs::Value attribs = js_get(ctx, el, "attribs");
    if (!attribs.is_object())
        return false;
    qjs::Value v = qjs::Value(ctx, JS_GetPropertyStr(ctx, attribs.raw(), "class"));
    if (!v.is_string())
        return false;
    const std::string val = js_str(ctx, v.raw());
    // CSS class 匹配：空格分隔词列表（大小写敏感）
    size_t pos = 0;
    while (pos <= val.size()) {
        size_t end = val.find(' ', pos);
        if (end == std::string::npos)
            end = val.size();
        if (end - pos == cls.size() && val.compare(pos, cls.size(), cls) == 0)
            return true;
        if (end == val.size())
            break;
        pos = end + 1;
    }
    return false;
}

inline bool js_attr_exists(JSContext* ctx, JSValue el, const std::string& name)
{
    qjs::Value attribs = js_get(ctx, el, "attribs");
    if (!attribs.is_object())
        return false;
    qjs::Value v = qjs::Value(ctx, JS_GetPropertyStr(ctx, attribs.raw(), name.c_str()));
    return !v.is_undefined();
}

inline std::optional<std::string> js_attr_get(JSContext* ctx, JSValue el,
                                              const std::string& name)
{
    qjs::Value attribs = js_get(ctx, el, "attribs");
    if (!attribs.is_object())
        return std::nullopt;
    qjs::Value v = qjs::Value(ctx, JS_GetPropertyStr(ctx, attribs.raw(), name.c_str()));
    if (!v.is_string())
        return std::nullopt;
    return js_str(ctx, v.raw());
}

// ---------------------------------------------------------------------------
// 解析：lexbor HTML -> JS 对象树
// ---------------------------------------------------------------------------

inline lxb_dom_element_t* make_context_element(lxb_html_document_t* doc,
                                               const std::string& tag)
{
    // 必须走 lxb_dom_interface_create（document 的接口注册表）：
    // lxb_dom_element_create 创建的裸元素与 fragment 解析内部按 tag_id
    // 创建 html 接口的路径不兼容（parse_fragment 内崩溃）。
    lxb_tag_id_t tag_id = lxb_tag_id_by_name(
        doc->dom_document.tags, reinterpret_cast<const lxb_char_t*>(tag.data()),
        tag.size());
    if (tag_id == LXB_TAG__UNDEF)
        tag_id = LXB_TAG_TEMPLATE; // 未知标签：template 内容模型最宽松
    return (lxb_dom_element_t*)lxb_dom_interface_create(
        &doc->dom_document, tag_id, LXB_NS_HTML);
}

// 递归构建节点。node 的 parent 由调用方维护。
inline qjs::Value build_node(JSContext* ctx, lxb_dom_node_t* node,
                             lxb_dom_node_t* parent_lxb)
{
    switch (node->type) {
        case LXB_DOM_NODE_TYPE_DOCUMENT: {
            qjs::Value obj(ctx, JS_NewObject(ctx));
            JS_SetPropertyStr(ctx, obj.raw(), "type",
                              JS_NewString(ctx, "root"));
            qjs::Value arr(ctx, JS_NewArray(ctx));
            // parent/prev/next 为 null
            JS_SetPropertyStr(ctx, obj.raw(), "parent", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "prev", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "next", JS_NULL);
            uint32_t idx = 0;
            for (lxb_dom_node_t* c = node->first_child; c; c = c->next) {
                qjs::Value child = build_node(ctx, c, node);
                JS_SetPropertyUint32(ctx, arr.raw(), idx++, child.take());
            }
            JS_SetPropertyStr(ctx, obj.raw(), "children", JS_DupValue(ctx, arr.raw()));
            // 设置 children 的 parent/prev/next
            uint32_t n = 0;
            js_array_length(ctx, arr.raw(), &n);
            for (uint32_t i = 0; i < n; ++i) {
                qjs::Value c(ctx, JS_GetPropertyUint32(ctx, arr.raw(), i));
                JS_SetPropertyStr(ctx, c.raw(), "parent", JS_DupValue(ctx, obj.raw()));
                JS_SetPropertyStr(ctx, c.raw(), "prev",
                                  i > 0 ? JS_GetPropertyUint32(ctx, arr.raw(), i - 1)
                                        : JS_NULL);
                JS_SetPropertyStr(ctx, c.raw(), "next",
                                  i + 1 < n ? JS_GetPropertyUint32(ctx, arr.raw(), i + 1)
                                            : JS_NULL);
            }
            return obj;
        }

        case LXB_DOM_NODE_TYPE_ELEMENT: {
            lxb_dom_element_t* el = reinterpret_cast<lxb_dom_element_t*>(node);
            // local_name：小写（domhandler 语义）；node_name 会保留源码大小写
            size_t name_len = 0;
            const lxb_char_t* name = lxb_dom_element_local_name(el, &name_len);
            std::string tag((const char*)name, name_len);

            qjs::Value obj(ctx, JS_NewObject(ctx));
            const char* type = (tag == "script")  ? "script"
                               : (tag == "style") ? "style"
                                                  : "tag";
            JS_SetPropertyStr(ctx, obj.raw(), "type", JS_NewString(ctx, type));
            JS_SetPropertyStr(ctx, obj.raw(), "name",
                              JS_NewStringLen(ctx, (const char*)name, name_len));
            // parse5-htmlparser2-tree-adapter 兼容：元素同时暴露 tagName
            JS_SetPropertyStr(ctx, obj.raw(), "tagName",
                              JS_NewStringLen(ctx, (const char*)name, name_len));

            // attribs
            qjs::Value attribs(ctx, JS_NewObject(ctx));
            for (lxb_dom_attr_t* a = el->first_attr; a; a = a->next) {
                size_t an_len = 0, av_len = 0;
                const lxb_char_t* an = lxb_dom_attr_qualified_name(a, &an_len);
                const lxb_char_t* av = lxb_dom_attr_value(a, &av_len);
                JS_SetPropertyStr(ctx, attribs.raw(),
                                  std::string((const char*)an, an_len).c_str(),
                                  JS_NewStringLen(ctx, (const char*)av, av_len));
            }
            JS_SetPropertyStr(ctx, obj.raw(), "attribs", attribs.take());

            qjs::Value arr(ctx, JS_NewArray(ctx));
            uint32_t idx = 0;
            for (lxb_dom_node_t* c = node->first_child; c; c = c->next) {
                qjs::Value child = build_node(ctx, c, node);
                JS_SetPropertyUint32(ctx, arr.raw(), idx++, child.take());
            }
            JS_SetPropertyStr(ctx, obj.raw(), "children", JS_DupValue(ctx, arr.raw()));
            uint32_t n = 0;
            js_array_length(ctx, arr.raw(), &n);
            for (uint32_t i = 0; i < n; ++i) {
                qjs::Value c(ctx, JS_GetPropertyUint32(ctx, arr.raw(), i));
                JS_SetPropertyStr(ctx, c.raw(), "parent", JS_DupValue(ctx, obj.raw()));
                JS_SetPropertyStr(ctx, c.raw(), "prev",
                                  i > 0 ? JS_GetPropertyUint32(ctx, arr.raw(), i - 1)
                                        : JS_NULL);
                JS_SetPropertyStr(ctx, c.raw(), "next",
                                  i + 1 < n ? JS_GetPropertyUint32(ctx, arr.raw(), i + 1)
                                            : JS_NULL);
            }
            (void)parent_lxb;
            return obj;
        }

        case LXB_DOM_NODE_TYPE_TEXT: {
            lxb_dom_character_data_t* cd = &reinterpret_cast<lxb_dom_text_t*>(node)->char_data;
            qjs::Value obj(ctx, JS_NewObject(ctx));
            JS_SetPropertyStr(ctx, obj.raw(), "type", JS_NewString(ctx, "text"));
            JS_SetPropertyStr(ctx, obj.raw(), "data",
                              JS_NewStringLen(ctx, (const char*)cd->data.data, cd->data.length));
            JS_SetPropertyStr(ctx, obj.raw(), "parent", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "prev", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "next", JS_NULL);
            return obj;
        }

        case LXB_DOM_NODE_TYPE_COMMENT: {
            lxb_dom_character_data_t* cd = &reinterpret_cast<lxb_dom_comment_t*>(node)->char_data;
            qjs::Value obj(ctx, JS_NewObject(ctx));
            JS_SetPropertyStr(ctx, obj.raw(), "type", JS_NewString(ctx, "comment"));
            JS_SetPropertyStr(ctx, obj.raw(), "data",
                              JS_NewStringLen(ctx, (const char*)cd->data.data, cd->data.length));
            JS_SetPropertyStr(ctx, obj.raw(), "parent", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "prev", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "next", JS_NULL);
            return obj;
        }

        case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: {
            lxb_dom_document_type_t* dt = reinterpret_cast<lxb_dom_document_type_t*>(node);
            size_t name_len = 0;
            const lxb_char_t* name = lxb_dom_node_name(node, &name_len);
            qjs::Value obj(ctx, JS_NewObject(ctx));
            JS_SetPropertyStr(ctx, obj.raw(), "type", JS_NewString(ctx, "directive"));
            std::string data = "!DOCTYPE ";
            data.append((const char*)name, name_len);
            JS_SetPropertyStr(ctx, obj.raw(), "data", JS_NewString(ctx, data.c_str()));
            // domhandler 语义：directive 的 name 为 '!doctype'
            JS_SetPropertyStr(ctx, obj.raw(), "name", JS_NewString(ctx, "!doctype"));
            JS_SetPropertyStr(ctx, obj.raw(), "parent", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "prev", JS_NULL);
            JS_SetPropertyStr(ctx, obj.raw(), "next", JS_NULL);
            (void)dt;
            return obj;
        }

        default:
            // CDATA / PI / 其它：CDATA 转 text；其余跳过
            if (node->type == LXB_DOM_NODE_TYPE_CDATA_SECTION) {
                lxb_dom_character_data_t* cd =
                    &reinterpret_cast<lxb_dom_cdata_section_t*>(node)->text.char_data;
                qjs::Value obj(ctx, JS_NewObject(ctx));
                JS_SetPropertyStr(ctx, obj.raw(), "type", JS_NewString(ctx, "text"));
                JS_SetPropertyStr(ctx, obj.raw(), "data",
                                  JS_NewStringLen(ctx, (const char*)cd->data.data, cd->data.length));
                JS_SetPropertyStr(ctx, obj.raw(), "parent", JS_NULL);
                JS_SetPropertyStr(ctx, obj.raw(), "prev", JS_NULL);
                JS_SetPropertyStr(ctx, obj.raw(), "next", JS_NULL);
                return obj;
            }
            return qjs::Value(ctx, JS_NULL);
    }
}

// 解析入口（JS 可调用）
inline qjs::Value lexbor_parse(qjs::Ctx ctx, std::string html, bool is_document,
                               qjs::Value context_tag)
{
    JSContext* jctx = ctx.ctx;
    lxb_html_document_t* doc = lxb_html_document_create();
    if (!doc)
        qjs::throw_type_error(jctx, "lexbor: document create failed");

    lxb_status_t status = LXB_STATUS_OK;
    lxb_dom_node_t* fragment_first = nullptr;
    std::string ctag;
    if (context_tag.is_string())
        ctag = js_str(jctx, context_tag.raw());

    if (is_document) {
        status = lxb_html_document_parse(doc, (const lxb_char_t*)html.data(),
                                         html.size());
    } else {
        // fragment 模式：context 默认 template（parse5 行为）；cheerio 传入
        // 显式 context 时用对应标签
        lxb_dom_element_t* ctx_el =
            make_context_element(doc, ctag.empty() ? "template" : ctag);
        if (!ctx_el) {
            lxb_html_document_destroy(doc);
            qjs::throw_type_error(jctx, "lexbor: context element create failed");
        }
        fragment_first = lxb_html_document_parse_fragment(
            doc, ctx_el, (const lxb_char_t*)html.data(), html.size());
        status = fragment_first ? LXB_STATUS_OK : LXB_STATUS_ERROR;
    }

    if (status != LXB_STATUS_OK) {
        lxb_html_document_destroy(doc);
        qjs::throw_type_error(jctx, "lexbor: html parse failed");
    }

    qjs::Value root;
    if (is_document) {
        root = build_node(jctx, &doc->dom_document.node, nullptr);
    } else {
        // fragment：返回节点链（含 text/comment），包装成 root
        root = qjs::Value(jctx, JS_NewObject(jctx));
        JS_SetPropertyStr(jctx, root.raw(), "type", JS_NewString(jctx, "root"));
        JS_SetPropertyStr(jctx, root.raw(), "parent", JS_NULL);
        JS_SetPropertyStr(jctx, root.raw(), "prev", JS_NULL);
        JS_SetPropertyStr(jctx, root.raw(), "next", JS_NULL);
        qjs::Value arr(jctx, JS_NewArray(jctx));
        uint32_t idx = 0;
        // fragment_first 是 lexbor 内部创建的 html 元素，fragment 内容在其
        // children（text/comment/元素平铺）
        if (fragment_first) {
            for (lxb_dom_node_t* c = fragment_first->first_child; c; c = c->next) {
                qjs::Value child = build_node(jctx, c, nullptr);
                if (!child.is_null()) {
                    JS_SetPropertyUint32(jctx, arr.raw(), idx++, child.take());
                }
            }
        }
        JS_SetPropertyStr(jctx, root.raw(), "children",
                          JS_DupValue(jctx, arr.raw()));
        uint32_t n = 0;
        js_array_length(jctx, arr.raw(), &n);
        for (uint32_t i = 0; i < n; ++i) {
            qjs::Value c(jctx, JS_GetPropertyUint32(jctx, arr.raw(), i));
            JS_SetPropertyStr(jctx, c.raw(), "parent", JS_DupValue(jctx, root.raw()));
            JS_SetPropertyStr(jctx, c.raw(), "prev",
                              i > 0 ? JS_GetPropertyUint32(jctx, arr.raw(), i - 1)
                                    : JS_NULL);
            JS_SetPropertyStr(jctx, c.raw(), "next",
                              i + 1 < n ? JS_GetPropertyUint32(jctx, arr.raw(), i + 1)
                                        : JS_NULL);
        }
    }

    lxb_html_document_destroy(doc);
    return root;
}

// ---------------------------------------------------------------------------
// 选择器匹配（在 JS 树上，lexbor 解析的 selector 结构）
// ---------------------------------------------------------------------------

// 元素在父元素子节点中的 1-based index（CSS :nth-child 只统计元素兄弟，
// text/comment 不计）
inline int64_t child_index(JSContext* ctx, JSValue el)
{
    qjs::Value parent = js_get(ctx, el, "parent");
    if (!parent.is_object())
        return -1;
    qjs::Value children = js_get(ctx, parent.raw(), "children");
    if (!children.is_array())
        return -1;
    uint32_t len = 0;
    js_array_length(ctx, children.raw(), &len);
    int64_t idx = 0;
    for (uint32_t i = 0; i < len; ++i) {
        qjs::Value c(ctx, JS_GetPropertyUint32(ctx, children.raw(), i));
        if (!js_is_element(ctx, c.raw()))
            continue;
        ++idx;
        if (JS_IsStrictEqual(ctx, c.raw(), el))
            return idx;
    }
    return -1;
}

// 同 type 兄弟中的序号（1-based）
inline int64_t type_index(JSContext* ctx, JSValue el)
{
    qjs::Value name_v = js_get(ctx, el, "name");
    if (!name_v.is_string())
        return -1;
    std::string name = js_str(ctx, name_v.raw());
    int64_t idx = 0;
    qjs::Value prev = js_get(ctx, el, "prev");
    for (int _g = 0; prev.is_object() && _g < 64; ++_g) {
        if (js_is_element(ctx, prev.raw())) {
            qjs::Value pn = js_get(ctx, prev.raw(), "name");
            if (pn.is_string() && js_str(ctx, pn.raw()) == name)
                ++idx;
        }
        prev = js_get(ctx, prev.raw(), "prev");
    }
    return idx + 1;
}

inline bool anb_match(long a, long b, int64_t n)
{
    // 匹配位置 n（1-based）是否满足 a*m + b == n (m >= 0 整数)
    if (a == 0)
        return n == b;
    int64_t m = (n - b) / a;
    return m >= 0 && a * m + b == n;
}

struct SelectorMatcher {
    JSContext* ctx;
    JSValue scope; // :scope 匹配对象（借用）

    // 从右向左匹配一条 selector 链；el 为当前候选元素。
    // lexbor 语义：selector->combinator = 该 selector 与 prev selector 的关系
    // （CLOSE = 复合段内；CHILD/SIBLING/FOLLOWING/DESCENDANT = 与左边段的关系）。
    // depth 防 JS 对象环：祖先/兄弟链递归超过上限即失败。
    bool match_chain(lxb_css_selector_t* sel, JSValue el, int depth = 0)
    {
        if (depth > 64)
            return false;
        // 复合段 [head..sel]：段内元素（非 head）的 combinator == CLOSE
        lxb_css_selector_t* head = sel;
        while (head->prev != nullptr && head->combinator == LXB_CSS_SELECTOR_COMBINATOR_CLOSE)
            head = head->prev;
        for (lxb_css_selector_t* s = head;; s = s->next) {
            if (!match_simple(s, el))
                return false;
            if (s == sel)
                break;
        }
        // 左边还有段？
        lxb_css_selector_t* left = head->prev;
        if (left == nullptr)
            return true;

        switch (head->combinator) {
            case LXB_CSS_SELECTOR_COMBINATOR_DESCENDANT: {
                // el 的某个祖先匹配左边链（left 作为新的链尾）
                qjs::Value parent = js_get(ctx, el, "parent");
                for (int _g = 0; parent.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, parent.raw()) &&
                        match_chain(left, parent.raw(), depth + 1))
                        return true;
                    parent = js_get(ctx, parent.raw(), "parent");
                }
                return false;
            }
            case LXB_CSS_SELECTOR_COMBINATOR_CHILD: {
                qjs::Value parent = js_get(ctx, el, "parent");
                if (!parent.is_object() || !js_is_element(ctx, parent.raw()))
                    return false;
                return match_chain(left, parent.raw(), depth + 1);
            }
            case LXB_CSS_SELECTOR_COMBINATOR_SIBLING: {
                qjs::Value prev = js_get(ctx, el, "prev");
                for (int _g = 0; prev.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, prev.raw()))
                        return match_chain(left, prev.raw(), depth + 1);
                    prev = js_get(ctx, prev.raw(), "prev");
                }
                return false;
            }
            case LXB_CSS_SELECTOR_COMBINATOR_FOLLOWING: {
                qjs::Value prev = js_get(ctx, el, "prev");
                for (int _g = 0; prev.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, prev.raw()) &&
                        match_chain(left, prev.raw(), depth + 1))
                        return true;
                    prev = js_get(ctx, prev.raw(), "prev");
                }
                return false;
            }
            default:
                return false;
        }
    }

    // 简单选择器匹配（含伪类）
    bool match_simple(lxb_css_selector_t* sel, JSValue el)
    {
        switch (sel->type) {
            case LXB_CSS_SELECTOR_TYPE_ANY:
                return true;

            case LXB_CSS_SELECTOR_TYPE_ELEMENT: {
                qjs::Value name_v = js_get(ctx, el, "name");
                if (!name_v.is_string())
                    return false;
                std::string_view want((const char*)sel->name.data, sel->name.length);
                return js_str(ctx, name_v.raw()) == want;
            }

            case LXB_CSS_SELECTOR_TYPE_ID: {
                auto v = js_attr_get(ctx, el, "id");
                if (!v)
                    return false;
                std::string_view want((const char*)sel->name.data, sel->name.length);
                return *v == want;
            }

            case LXB_CSS_SELECTOR_TYPE_CLASS: {
                std::string_view want((const char*)sel->name.data, sel->name.length);
                if (want == "__lexbor_scope__")
                    return JS_IsStrictEqual(ctx, el, scope); // :scope 映射
                return js_has_class(ctx, el, std::string(want));
            }

            case LXB_CSS_SELECTOR_TYPE_ATTRIBUTE:
                return match_attribute(sel, el);

            case LXB_CSS_SELECTOR_TYPE_PSEUDO_CLASS:
                return match_pseudo(sel->u.pseudo.type, sel, el);

            case LXB_CSS_SELECTOR_TYPE_PSEUDO_CLASS_FUNCTION:
                return match_pseudo_function(sel->u.pseudo.type, sel, el);

            case LXB_CSS_SELECTOR_TYPE_PSEUDO_ELEMENT:
            case LXB_CSS_SELECTOR_TYPE_PSEUDO_ELEMENT_FUNCTION:
                // css-select 不匹配伪元素
                return false;

            default:
                return false;
        }
    }

    bool match_attribute(lxb_css_selector_t* sel, JSValue el)
    {
        std::string name((const char*)sel->name.data, sel->name.length);
        auto v = js_attr_get(ctx, el, name);
        if (!v)
            return false;
        const lxb_css_selector_attribute_t& a = sel->u.attribute;
        if (a.match == LXB_CSS_SELECTOR_MATCH_EQUAL) {
            if (a.value.data == nullptr)
                return true; // [attr] 无值 = 存在性检查
            return *v == std::string((const char*)a.value.data, a.value.length);
        }
        // 值比较（区分大小写，除非 modifier I）
        std::string value((const char*)a.value.data, a.value.length);
        std::string attr = *v;
        bool insensitive = a.modifier == LXB_CSS_SELECTOR_MODIFIER_I;
        auto eq = [&](const std::string& x, const std::string& y) {
            if (!insensitive)
                return x == y;
            if (x.size() != y.size())
                return false;
            for (size_t i = 0; i < x.size(); ++i)
                if (std::tolower((unsigned char)x[i]) != std::tolower((unsigned char)y[i]))
                    return false;
            return true;
        };
        switch (a.match) {
            case LXB_CSS_SELECTOR_MATCH_INCLUDE: {
                // ~= 空格分词
                size_t pos = 0;
                while (pos <= attr.size()) {
                    size_t end = attr.find(' ', pos);
                    if (end == std::string::npos)
                        end = attr.size();
                    if (eq(attr.substr(pos, end - pos), value))
                        return true;
                    if (end == attr.size())
                        break;
                    pos = end + 1;
                }
                return false;
            }
            case LXB_CSS_SELECTOR_MATCH_DASH: {
                if (eq(attr, value))
                    return true;
                std::string prefix = value + "-";
                return attr.size() > prefix.size() &&
                       eq(attr.substr(0, prefix.size()), prefix);
            }
            case LXB_CSS_SELECTOR_MATCH_PREFIX: {
                return attr.size() >= value.size() &&
                       eq(attr.substr(0, value.size()), value);
            }
            case LXB_CSS_SELECTOR_MATCH_SUFFIX: {
                return attr.size() >= value.size() &&
                       eq(attr.substr(attr.size() - value.size()), value);
            }
            case LXB_CSS_SELECTOR_MATCH_SUBSTRING: {
                if (insensitive) {
                    std::string lo_attr = attr, lo_val = value;
                    for (auto& c : lo_attr) c = (char)std::tolower((unsigned char)c);
                    for (auto& c : lo_val) c = (char)std::tolower((unsigned char)c);
                    return lo_attr.find(lo_val) != std::string::npos;
                }
                return attr.find(value) != std::string::npos;
            }
            default:
                return false;
        }
    }

    // 元素文本内容（text/script/style 后代拼接）；depth 防环
    std::string text_content(JSValue el)
    {
        std::string out;
        collect_text(el, out, 0);
        return out;
    }

    void collect_text(JSValue node, std::string& out, int depth)
    {
        if (depth > 64)
            return;
        qjs::Value type = js_get(ctx, node, "type");
        if (!type.is_string())
            return;
        std::string t = js_str(ctx, type.raw());
        if (t == "text" || t == "cdata") {
            qjs::Value data = js_get(ctx, node, "data");
            if (data.is_string())
                out += js_str(ctx, data.raw());
            return;
        }
        if (t == "tag" || t == "script" || t == "style" || t == "root") {
            qjs::Value children = js_get(ctx, node, "children");
            if (children.is_array()) {
                uint32_t len = 0;
                js_array_length(ctx, children.raw(), &len);
                for (uint32_t i = 0; i < len; ++i) {
                    qjs::Value c(ctx, JS_GetPropertyUint32(ctx, children.raw(), i));
                    collect_text(c.raw(), out, depth + 1);
                }
            }
        }
    }

    bool match_pseudo(unsigned type, lxb_css_selector_t* sel, JSValue el)
    {
        switch (type) {
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FIRST_CHILD: {
                return child_index(ctx, el) == 1;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_LAST_CHILD: {
                // el 之后没有元素兄弟
                qjs::Value next = js_get(ctx, el, "next");
                for (int _g = 0; next.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, next.raw()))
                        return false;
                    next = js_get(ctx, next.raw(), "next");
                }
                return true;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_ONLY_CHILD: {
                // 前后都没有元素兄弟
                qjs::Value prev = js_get(ctx, el, "prev");
                for (int _g = 0; prev.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, prev.raw()))
                        return false;
                    prev = js_get(ctx, prev.raw(), "prev");
                }
                qjs::Value next = js_get(ctx, el, "next");
                for (int _g = 0; next.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, next.raw()))
                        return false;
                    next = js_get(ctx, next.raw(), "next");
                }
                return true;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FIRST_OF_TYPE: {
                qjs::Value name_v = js_get(ctx, el, "name");
                if (!name_v.is_string())
                    return false;
                std::string name = js_str(ctx, name_v.raw());
                qjs::Value prev = js_get(ctx, el, "prev");
                for (int _g = 0; prev.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, prev.raw())) {
                        qjs::Value pn = js_get(ctx, prev.raw(), "name");
                        if (pn.is_string() && js_str(ctx, pn.raw()) == name)
                            return false;
                    }
                    prev = js_get(ctx, prev.raw(), "prev");
                }
                return true;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_LAST_OF_TYPE: {
                qjs::Value name_v = js_get(ctx, el, "name");
                if (!name_v.is_string())
                    return false;
                std::string name = js_str(ctx, name_v.raw());
                qjs::Value next = js_get(ctx, el, "next");
                for (int _g = 0; next.is_object() && _g < 64; ++_g) {
                    if (js_is_element(ctx, next.raw())) {
                        qjs::Value nn = js_get(ctx, next.raw(), "name");
                        if (nn.is_string() && js_str(ctx, nn.raw()) == name)
                            return false;
                    }
                    next = js_get(ctx, next.raw(), "next");
                }
                return true;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_ONLY_OF_TYPE: {
                return match_pseudo(LXB_CSS_SELECTOR_PSEUDO_CLASS_FIRST_OF_TYPE, sel, el) &&
                       match_pseudo(LXB_CSS_SELECTOR_PSEUDO_CLASS_LAST_OF_TYPE, sel, el);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_EMPTY: {
                qjs::Value children = js_get(ctx, el, "children");
                if (!children.is_array())
                    return false;
                uint32_t len = 0;
                js_array_length(ctx, children.raw(), &len);
                return len == 0;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_ROOT: {
                // 文档根元素：parent 是 root 节点
                qjs::Value parent = js_get(ctx, el, "parent");
                if (!parent.is_object())
                    return false;
                qjs::Value ptype = js_get(ctx, parent.raw(), "type");
                if (!ptype.is_string())
                    return false;
                if (js_str(ctx, ptype.raw()) != "root")
                    return false;
                qjs::Value pp = js_get(ctx, parent.raw(), "parent");
                return pp.is_null();
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_SCOPE: {
                return JS_IsStrictEqual(ctx, el, scope);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_LINK: {
                qjs::Value name_v = js_get(ctx, el, "name");
                if (!name_v.is_string())
                    return false;
                std::string name = js_str(ctx, name_v.raw());
                return (name == "a" || name == "area" || name == "link") &&
                       js_attr_exists(ctx, el, "href");
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_CHECKED: {
                if (js_attr_exists(ctx, el, "checked"))
                    return true;
                if (js_attr_exists(ctx, el, "selected"))
                    return true;
                return false;
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_DISABLED:
                return js_attr_exists(ctx, el, "disabled");
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_ENABLED:
                return !js_attr_exists(ctx, el, "disabled");
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_REQUIRED:
                return js_attr_exists(ctx, el, "required");
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_OPTIONAL:
                return !js_attr_exists(ctx, el, "required");
            default:
                return false;
        }
    }

    bool match_pseudo_function(unsigned type, lxb_css_selector_t* sel, JSValue el)
    {
        switch (type) {
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_CHILD:
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_LAST_CHILD: {
                auto* anb = (lxb_css_selector_anb_of_t*)sel->u.pseudo.data;
                if (!anb)
                    return false;
                int64_t idx = child_index(ctx, el);
                if (idx < 0)
                    return false;
                int64_t pos = idx;
                if (type == LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_LAST_CHILD) {
                    // 从后数：沿 next 链数元素兄弟（len 含 text/comment，不可用）
                    pos = 1;
                    qjs::Value next = js_get(ctx, el, "next");
                    for (int _g = 0; next.is_object() && _g < 64; ++_g) {
                        if (js_is_element(ctx, next.raw()))
                            ++pos;
                        next = js_get(ctx, next.raw(), "next");
                    }
                }
                return anb_match(anb->anb.a, anb->anb.b, pos);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_OF_TYPE:
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_LAST_OF_TYPE: {
                auto* anb = (lxb_css_selector_anb_of_t*)sel->u.pseudo.data;
                if (!anb)
                    return false;
                int64_t idx = type_index(ctx, el);
                if (idx < 0)
                    return false;
                if (type == LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NTH_LAST_OF_TYPE) {
                    qjs::Value name_v = js_get(ctx, el, "name");
                    std::string name = js_str(ctx, name_v.raw());
                    int64_t total = 0;
                    qjs::Value next = js_get(ctx, el, "next");
                    for (int _g = 0; next.is_object() && _g < 64; ++_g) {
                        if (js_is_element(ctx, next.raw())) {
                            qjs::Value nn = js_get(ctx, next.raw(), "name");
                            if (nn.is_string() && js_str(ctx, nn.raw()) == name)
                                ++total;
                        }
                        next = js_get(ctx, next.raw(), "next");
                    }
                    idx = total + 1; // 从后数位置 = 后续同类型兄弟数 + 1
                }
                return anb_match(anb->anb.a, anb->anb.b, idx);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_NOT: {
                auto* list = (lxb_css_selector_list_t*)sel->u.pseudo.data;
                if (!list)
                    return true;
                return !match_any_list(list, el);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_IS:
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_WHERE: {
                auto* list = (lxb_css_selector_list_t*)sel->u.pseudo.data;
                if (!list)
                    return false;
                return match_any_list(list, el);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_HAS: {
                // :has(sel)：el 的后代（含自身？标准为后代）中任一匹配
                auto* list = (lxb_css_selector_list_t*)sel->u.pseudo.data;
                if (!list)
                    return false;
                return has_descendant_match(list, el);
            }
            case LXB_CSS_SELECTOR_PSEUDO_CLASS_FUNCTION_LEXBOR_CONTAINS: {
                auto* c = (lxb_css_selector_contains_t*)sel->u.pseudo.data;
                if (!c)
                    return false;
                std::string needle((const char*)c->str.data, c->str.length);
                std::string hay = text_content(el);
                if (c->insensitive) {
                    for (auto& ch : hay) ch = (char)std::tolower((unsigned char)ch);
                    for (auto& ch : needle) ch = (char)std::tolower((unsigned char)ch);
                }
                return hay.find(needle) != std::string::npos;
            }
            default:
                return false;
        }
    }

    // selector list（逗号分隔）任一链匹配 el
    bool match_any_list(lxb_css_selector_list_t* list, JSValue el)
    {
        for (lxb_css_selector_list_t* l = list; l; l = l->next) {
            if (l->first != nullptr && match_chain(l->last, el))
                return true;
        }
        return false;
    }

    // :has：el 后代中任一节点匹配（含 el 自身？CSS 规范 :has() 匹配后代；
    // css-select 的 :has 匹配"后代"（不含自身））；depth 防环
    bool has_descendant_match(lxb_css_selector_list_t* list, JSValue el, int depth = 0)
    {
        if (depth > 64)
            return false;
        qjs::Value children = js_get(ctx, el, "children");
        if (!children.is_array())
            return false;
        uint32_t len = 0;
        js_array_length(ctx, children.raw(), &len);
        for (uint32_t i = 0; i < len; ++i) {
            qjs::Value c(ctx, JS_GetPropertyUint32(ctx, children.raw(), i));
            if (js_is_element(ctx, c.raw()) &&
                (match_any_list(list, c.raw()) || has_descendant_match(list, c.raw(), depth + 1)))
                return true;
        }
        return false;
    }
};

// 查询入口（JS 可调用）：root 后代中匹配 selector 的元素（文档序）。
// include_self 为 true 时 root 本身也参与匹配（is()/filter() 用）。
inline qjs::Value lexbor_query_all(qjs::Ctx ctx, qjs::Value root, std::string selector,
                                   qjs::Opt<bool> include_self)
{
    JSContext* jctx = ctx.ctx;

    lxb_css_parser_t* parser = lxb_css_parser_create();
    if (!parser)
        qjs::throw_type_error(jctx, "lexbor: css parser create failed");
    lxb_status_t st = lxb_css_parser_init(parser, nullptr);
    if (st != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        qjs::throw_type_error(jctx, "lexbor: css parser init failed");
    }
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        parser, (const lxb_char_t*)selector.data(), selector.size());
    if (list == nullptr) {
        // 兼容 css-select 伪类名/语法：
        //   :contains → :lexbor-contains；:scope → .__lexbor_scope__（匹配器映射）
        std::string mapped = selector;
        size_t pos = 0;
        while ((pos = mapped.find(":contains(", pos)) != std::string::npos) {
            mapped.replace(pos, 10, ":lexbor-contains(");
            pos += 17;
        }
        pos = 0;
        while ((pos = mapped.find(":scope", pos)) != std::string::npos) {
            mapped.replace(pos, 6, ".__lexbor_scope__");
            pos += 16;
        }
        if (mapped != selector) {
            list = lxb_css_selectors_parse(
                parser, (const lxb_char_t*)mapped.data(), mapped.size());
        }
        if (list == nullptr) {
            lxb_css_parser_destroy(parser, true);
            qjs::throw_type_error(jctx, "lexbor: invalid selector: " + selector);
        }
    }

    qjs::Value out(jctx, JS_NewArray(jctx));
    SelectorMatcher m{jctx, root.raw()};

    auto matches_any = [&](JSValue el) {
        for (lxb_css_selector_list_t* l = list; l; l = l->next) {
            if (l->first != nullptr && m.match_chain(l->last, el))
                return true;
        }
        return false;
    };

    uint32_t out_idx = 0;
    // DFS 收集（可选包含 root 本身）
    std::vector<qjs::Value> stack;
    if (include_self.value.has_value() && *include_self &&
        js_is_element(jctx, root.raw())) {
        if (matches_any(root.raw()))
            JS_SetPropertyUint32(jctx, out.raw(), out_idx++,
                                 JS_DupValue(jctx, root.raw()));
    }
    qjs::Value children = js_get(jctx, root.raw(), "children");
    if (children.is_array()) {
        uint32_t len = 0;
        js_array_length(jctx, children.raw(), &len);
        for (uint32_t i = 0; i < len; ++i)
            stack.push_back(qjs::Value(jctx, JS_GetPropertyUint32(jctx, children.raw(), i)));
    }
    // 节点计数上限：防 JS 对象环导致 DFS 死循环（正常文档树远小于此值）
    constexpr uint32_t kMaxNodes = 100000;
    uint32_t visited = 0;
    while (!stack.empty() && visited++ < kMaxNodes) {
        qjs::Value node = std::move(stack.back());
        stack.pop_back();
        if (js_is_element(jctx, node.raw())) {
            if (matches_any(node.raw()))
                JS_SetPropertyUint32(jctx, out.raw(), out_idx++, JS_DupValue(jctx, node.raw()));
        }
        // 子节点压栈（保持文档序：先压最后一个）
        qjs::Value kids = js_get(jctx, node.raw(), "children");
        if (kids.is_array()) {
            uint32_t len = 0;
            js_array_length(jctx, kids.raw(), &len);
            for (uint32_t i = len; i > 0; --i)
                stack.push_back(qjs::Value(jctx, JS_GetPropertyUint32(jctx, kids.raw(), i - 1)));
        }
    }

    lxb_css_parser_destroy(parser, true);
    return out;
}

} // namespace lxb_detail

// 注册内部函数：__lexbor_parse / __lexbor_queryAll
inline void install_lexbor_dom(qjs::Context& ctx)
{
    ctx.globals().set("__lexbor_parse", qjs::func(ctx.raw(), lxb_detail::lexbor_parse));
    ctx.globals().set("__lexbor_queryAll", qjs::func(ctx.raw(), lxb_detail::lexbor_query_all));
}

} // namespace qjsbind::cheerio
