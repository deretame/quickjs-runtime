// lexbor_handle.hpp —— lexbor C 树句柄绑定（方案 A 核心）
//
// 目标：HTML 解析、DOM 树、选择器匹配、序列化全部在 C++/lexbor 侧执行，
// QuickJS 只持有轻量句柄。两个 JS class：
//   1. 节点句柄（node class）——opaque 持有 lxb_dom_node_t* + DomRef
//   2. cheerio 选择集（$ class）——可调用 + 类数组 + 原型方法
//
// 节点句柄通过 JSClassExoticMethods.get_property 提供 domhandler 兼容的
// 惰性属性（children/parent/attribs/data...），供直接访问节点结构的代码
// 使用；cheerio API 内部一律走 C++ 快速路径。
//
// 生命周期：DomRef 引用计数（文档 + 可选 fragment 解析器），最后一个
// 句柄/选择集释放时销毁文档。
#pragma once

#include <qjsbind/error.hpp>
#include <qjsbind/qjsbind.hpp>
#include <qjsbind/value.hpp>

#include <qjsbind/context.hpp>

#include <lexbor/dom/dom.h>
#include <lexbor/html/html.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace qjsbind::cheerio::lxb_handle {

// ---------------------------------------------------------------------------
// DomRef：文档生命周期
// ---------------------------------------------------------------------------
struct DomRef {
    lxb_html_document_t* doc = nullptr;
    // refs=0 起步：load 创建后由 make_sel 接管（retain 到 1），
    // 所有派生选择集/节点句柄各自 retain，最后一个释放时销毁文档
    uint32_t refs = 0;
    // 节点句柄缓存：node -> JSValue（弱引用语义，不增 refcount；NodeHandle
    // finalizer 删除条目）。保证 $[i] 与 get(i)/toArray() 返回同一 JS 对象
    // （spec 用 toBe 断言对象同一性）。
    std::unordered_map<lxb_dom_node_t*, JSValue> node_cache;

    void retain() { ++refs; }
    void release()
    {
        if (--refs == 0) {
            if (doc)
                lxb_html_document_destroy(doc);
            delete this;
        }
    }
};

// 节点句柄 opaque
struct NodeHandle {
    lxb_dom_node_t* node = nullptr;
    DomRef* ref = nullptr;
};

// cheerio 选择集 opaque（$ 对象）
struct CheerioSel {
    DomRef* ref = nullptr;
    std::vector<lxb_dom_node_t*> nodes;
    // prevObject（end() 链式回退）：C++ 字段持有引用，finalizer 释放。
    // 不用 JS 属性：JS 属性会引入 GC 跨轮回收延迟（被回收对象的引用
    // 对象错过当轮回收窗口），在 Runtime 销毁时触发 quickjs debug assert。
    JSValue prev = JS_UNDEFINED;
    ~CheerioSel()
    {
        if (ref)
            ref->release();
    }
};

// ---------------------------------------------------------------------------
// 全局 class 注册（惰性初始化）
// ---------------------------------------------------------------------------
struct ClassIds {
    JSClassID node = 0;
    JSClassID sel = 0;
    JSValue sel_proto = JS_UNDEFINED;
    // 惰性属性 atom（per-runtime：atom 值按 runtime 分配，不可全局共享）
    JSAtom type = 0, name = 0, attribs = 0, children = 0, childNodes = 0,
           parent = 0, prev = 0, next = 0, data = 0, length = 0, tagName = 0,
           nodeType = 0, nodeValue = 0;
};

inline ClassIds& class_ids(JSRuntime* rt)
{
    static std::unordered_map<JSRuntime*, ClassIds> m;
    return m[rt];
}

inline void fill_atoms(JSContext* ctx, ClassIds& ids)
{
    // 每次注册重新创建（旧 atom 随旧 runtime 销毁，无需释放）
    ids.type = JS_NewAtom(ctx, "type");
    ids.name = JS_NewAtom(ctx, "name");
    ids.attribs = JS_NewAtom(ctx, "attribs");
    ids.children = JS_NewAtom(ctx, "children");
    ids.childNodes = JS_NewAtom(ctx, "childNodes");
    ids.parent = JS_NewAtom(ctx, "parent");
    ids.prev = JS_NewAtom(ctx, "prev");
    ids.next = JS_NewAtom(ctx, "next");
    ids.data = JS_NewAtom(ctx, "data");
    ids.length = JS_NewAtom(ctx, "length");
    ids.tagName = JS_NewAtom(ctx, "tagName");
    ids.nodeType = JS_NewAtom(ctx, "nodeType");
    ids.nodeValue = JS_NewAtom(ctx, "nodeValue");
}

// ---------------------------------------------------------------------------
// 节点类型/名称/文本辅助
// ---------------------------------------------------------------------------
inline const char* node_type_str(lxb_dom_node_t* n)
{
    switch (n->type) {
        case LXB_DOM_NODE_TYPE_ELEMENT: {
            lxb_dom_element_t* el = lxb_dom_interface_element(n);
            if (lxb_dom_element_tag_id(el) == LXB_TAG_SCRIPT)
                return "script";
            if (lxb_dom_element_tag_id(el) == LXB_TAG_STYLE)
                return "style";
            return "tag";
        }
        case LXB_DOM_NODE_TYPE_TEXT:
            return "text";
        case LXB_DOM_NODE_TYPE_COMMENT:
            return "comment";
        case LXB_DOM_NODE_TYPE_DOCUMENT:
        case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE:
            return "root";
        default:
            return "tag";
    }
}

inline std::string node_name(JSContext* ctx, lxb_dom_node_t* n)
{
    if (n->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        lxb_dom_element_t* el = lxb_dom_interface_element(n);
        size_t len = 0;
        const lxb_char_t* nm = lxb_tag_name_by_id(lxb_dom_element_tag_id(el), &len);
        return std::string((const char*)nm, len);
    }
    if (n->type == LXB_DOM_NODE_TYPE_DOCUMENT)
        return "#document";
    if (n->type == LXB_DOM_NODE_TYPE_TEXT)
        return "#text";
    if (n->type == LXB_DOM_NODE_TYPE_COMMENT)
        return "#comment";
    return "#document";
}

inline std::string node_data(JSContext* ctx, lxb_dom_node_t* n)
{
    if (n->type == LXB_DOM_NODE_TYPE_TEXT || n->type == LXB_DOM_NODE_TYPE_COMMENT) {
        size_t len = 0;
        const lxb_char_t* d = lxb_dom_node_text_content(n, &len);
        return std::string((const char*)d, len);
    }
    return "";
}

inline bool node_is_element(lxb_dom_node_t* n)
{
    return n != nullptr && n->type == LXB_DOM_NODE_TYPE_ELEMENT;
}

// ---------------------------------------------------------------------------
// 句柄创建/解包
// ---------------------------------------------------------------------------
inline JSValue make_node(JSContext* ctx, lxb_dom_node_t* node, DomRef* ref)
{
    // 缓存命中：同一节点返回同一句柄（get(i) === $[i]）
    auto it = ref->node_cache.find(node);
    if (it != ref->node_cache.end())
        return JS_DupValue(ctx, it->second);
    JSValue obj = JS_NewObjectClass(ctx, class_ids(JS_GetRuntime(ctx)).node);
    if (JS_IsException(obj))
        return obj;
    auto* h = new NodeHandle{node, ref};
    ref->retain();
    JS_SetOpaque(obj, h);
    ref->node_cache.emplace(node, obj);
    return obj;
}

inline NodeHandle* unwrap_node(JSContext* ctx, JSValue v)
{
    if (!JS_IsObject(v))
        return nullptr;
    auto& ids = class_ids(JS_GetRuntime(ctx));
    if (!ids.node)
        return nullptr;
    return (NodeHandle*)JS_GetOpaque(v, ids.node);
}

// ---------------------------------------------------------------------------
// 节点句柄惰性属性（domhandler 兼容）
// ---------------------------------------------------------------------------
// domhandler 兼容惰性属性读取（返回 JS_UNDEFINED 表示未处理）
inline bool node_get_property(JSContext* ctx, JSValue obj, JSAtom atom,
                              JSValue* value); // 前置声明

inline JSValue node_get_property_ex(JSContext* ctx, JSValueConst obj,
                                    JSAtom atom, JSValueConst receiver)
{
    JSValue out = JS_UNDEFINED;
    if (node_get_property(ctx, obj, atom, &out))
        return out;
    // quickjs exotic get_property 返回 undefined 即终止查找（不断原型链），
    // 因此未处理时手动查原型
    JSValue proto = JS_GetPrototype(ctx, obj);
    JSValue r = JS_GetProperty(ctx, proto, atom);
    JS_FreeValue(ctx, proto);
    return r;
}

inline bool node_get_property(JSContext* ctx, JSValue obj, JSAtom atom,
                              JSValue* value)
{
    auto& ids = class_ids(JS_GetRuntime(ctx));
    auto* h = (NodeHandle*)JS_GetOpaque(obj, ids.node);
    if (!h || !h->node)
        return false;
    lxb_dom_node_t* n = h->node;

    if (atom == ids.type) {
        *value = JS_NewString(ctx, node_type_str(n));
        return true;
    }
    if (atom == ids.name || atom == ids.tagName) {
        *value = JS_NewStringLen(ctx, node_name(ctx, n).data(),
                                 (int)node_name(ctx, n).size());
        return true;
    }
    if (atom == ids.data || atom == ids.nodeValue) {
        std::string d = node_data(ctx, n);
        *value = JS_NewStringLen(ctx, d.data(), (int)d.size());
        return true;
    }
    if (atom == ids.nodeType) {
        int t = n->type == LXB_DOM_NODE_TYPE_ELEMENT ? 1
                : n->type == LXB_DOM_NODE_TYPE_TEXT  ? 3
                : n->type == LXB_DOM_NODE_TYPE_COMMENT ? 8
                                                      : 9;
        *value = JS_NewInt32(ctx, t);
        return true;
    }
    // children 与 childNodes 同义（domhandler 兼容）
    if (atom == ids.children || atom == ids.childNodes) {
        JSValue arr = JS_NewArray(ctx);
        uint32_t i = 0;
        for (lxb_dom_node_t* c = n->first_child; c; c = c->next) {
            JSValue hv = make_node(ctx, c, h->ref);
            JS_SetPropertyUint32(ctx, arr, i++, hv);
        }
        *value = arr;
        return true;
    }
    if (atom == ids.parent) {
        *value = n->parent ? make_node(ctx, n->parent, h->ref) : JS_NULL;
        return true;
    }
    if (atom == ids.prev) {
        *value = n->prev ? make_node(ctx, n->prev, h->ref) : JS_NULL;
        return true;
    }
    if (atom == ids.next) {
        *value = n->next ? make_node(ctx, n->next, h->ref) : JS_NULL;
        return true;
    }
    if (atom == ids.attribs) {
        JSValue o = JS_NewObject(ctx);
        if (n->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t* el = lxb_dom_interface_element(n);
            for (lxb_dom_attr_t* a = el->first_attr; a; a = a->next) {
                size_t nl = 0, vl = 0;
                const lxb_char_t* an = lxb_dom_attr_local_name(a, &nl);
                const lxb_char_t* av = lxb_dom_attr_value(a, &vl);
                if (an) {
                    JSValue sv = JS_NewStringLen(ctx, (const char*)av, (int)vl);
                    JS_SetPropertyStr(ctx, o, (const char*)an, sv);
                }
            }
        }
        *value = o;
        return true;
    }
    return false;
}

// 惰性 set：未处理属性走 JS_DefineProperty（quickjs-ng exotic set_property
// 返回值直接生效，不 fallthrough 到默认路径；返回 0 = 静默丢弃）
inline int node_set_property(JSContext* ctx, JSValue obj, JSAtom atom,
                             JSValue value, JSValue receiver, int flags)
{
    return JS_DefineProperty(ctx, obj, atom, JS_DupValue(ctx, value),
                             JS_UNDEFINED, JS_UNDEFINED, JS_PROP_HAS_VALUE)
               ? 1
               : -1;
}

// ---------------------------------------------------------------------------
// $ class：可调用 + 类数组
// ---------------------------------------------------------------------------
inline JSValue g_sel_proto = JS_UNDEFINED; // $ 原型（方法注册处）

inline CheerioSel* unwrap_sel(JSContext* ctx, JSValue v)
{
    auto& ids = class_ids(JS_GetRuntime(ctx));
    if (!ids.sel || !JS_IsObject(v))
        return nullptr;
    return (CheerioSel*)JS_GetOpaque(v, ids.sel);
}

// 前置声明（定义在 lexbor_match.hpp / lexbor_api.hpp / 本文件下方）
// prev 为有效 JS 值时，结果集带 prevObject 属性（end() 语义：链式回退）
inline JSValue make_sel(JSContext* ctx, DomRef* ref,
                        std::vector<lxb_dom_node_t*> nodes,
                        JSValueConst prev = JS_UNDEFINED);
inline std::vector<lxb_dom_node_t*> c_query_selector(JSContext* jctx,
                                                     lxb_dom_node_t* root,
                                                     const std::string& selector,
                                                     bool include_self,
                                                     lxb_dom_node_t* scope = nullptr,
                                                     bool* ok = nullptr);
inline JSValue throw_invalid_selector(JSContext* ctx,
                                      const std::string& selector);
inline std::vector<lxb_dom_node_t*> c_parse_fragment(JSContext* jctx,
                                                     lxb_dom_node_t* parent,
                                                     const std::string& html);

// $(selector)：在 $ 的文档上查询；$(node)/(数组)：包装
// $(selector, context)：context 为 HTML 字符串 → fragment 解析后在其后代查；
// 非 HTML 字符串 → 组合查询（"ctx sel"）；$ / 节点 / 数组 → 在其后代查
inline JSValue sel_call_ex(JSContext* ctx, JSValueConst func_obj,
                           JSValueConst this_val, int argc, JSValueConst* argv,
                           int magic)
{
    CheerioSel* s = unwrap_sel(ctx, func_obj);
    if (!s)
        return JS_UNDEFINED;
    // $(htmlString)：'<' 开头 → fragment 解析（cheerio 语义）
    if (argc >= 1 && JS_IsString(argv[0])) {
        size_t slen = 0;
        const char* sel_str = JS_ToCStringLen(ctx, &slen, argv[0]);
        if (!sel_str)
            return JS_EXCEPTION;
        std::string selector(sel_str, slen);
        JS_FreeCString(ctx, sel_str);
        size_t b = selector.find_first_not_of(" \t\r\n");
        bool is_html = b != std::string::npos && selector[b] == '<';
        if (is_html) {
            lxb_dom_node_t* doc_node = &s->ref->doc->dom_document.node;
            std::vector<lxb_dom_node_t*> nodes =
                c_parse_fragment(ctx, doc_node, selector);
            return make_sel(ctx, s->ref, std::move(nodes));
        }
        // $(selector[, context])
        bool have_ctx = argc > 1 && !JS_IsUndefined(argv[1]);
        std::vector<lxb_dom_node_t*> ctx_nodes;
        if (have_ctx) {
            if (JS_IsString(argv[1])) {
                size_t clen = 0;
                const char* cstr = JS_ToCStringLen(ctx, &clen, argv[1]);
                if (!cstr)
                    return JS_EXCEPTION;
                std::string context_str(cstr, clen);
                JS_FreeCString(ctx, cstr);
                size_t cb = context_str.find_first_not_of(" \t\r\n");
                if (cb != std::string::npos && context_str[cb] == '<') {
                    // $('li', '<ul>...</ul>')：解析 context 为节点
                    lxb_dom_node_t* doc_node = &s->ref->doc->dom_document.node;
                    ctx_nodes = c_parse_fragment(ctx, doc_node, context_str);
                } else {
                    // $('li', 'ul')：组合查询
                    selector = context_str + " " + selector;
                }
            } else if (CheerioSel* cs = unwrap_sel(ctx, argv[1])) {
                ctx_nodes = cs->nodes;
            } else if (NodeHandle* h = unwrap_node(ctx, argv[1])) {
                ctx_nodes = {h->node};
            } else if (JS_IsArray(argv[1])) {
                JSValue arr = JS_GetPropertyStr(ctx, argv[1], "length");
                uint32_t len = 0;
                if (!JS_IsException(arr)) {
                    JS_ToUint32(ctx, &len, arr);
                    JS_FreeValue(ctx, arr);
                }
                for (uint32_t i = 0; i < len; ++i) {
                    JSValue v = JS_GetPropertyUint32(ctx, argv[1], i);
                    if (NodeHandle* h = unwrap_node(ctx, v))
                        ctx_nodes.push_back(h->node);
                    JS_FreeValue(ctx, v);
                }
            }
        }
        if (!ctx_nodes.empty()) {
            // 在 context 每个节点（含自身）的后代中查询——css-select
            // selectAll 语义：compiled(elem) 自身也参与匹配
            bool ok = true;
            std::vector<lxb_dom_node_t*> out;
            for (lxb_dom_node_t* n : ctx_nodes) {
                std::vector<lxb_dom_node_t*> r =
                    c_query_selector(ctx, n, selector, true, nullptr, &ok);
                if (!ok)
                    return throw_invalid_selector(ctx, selector);
                for (lxb_dom_node_t* m : r) {
                    bool dup = false;
                    for (lxb_dom_node_t* o : out) {
                        if (o == m) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup)
                        out.push_back(m);
                }
            }
            return make_sel(ctx, s->ref, std::move(out));
        }
        lxb_dom_node_t* root = &s->ref->doc->dom_document.node;
        std::vector<lxb_dom_node_t*> nodes =
            c_query_selector(ctx, root, selector, false);
        return make_sel(ctx, s->ref, std::move(nodes));
    }
    // $(nodeHandle)：包装单节点
    if (NodeHandle* h = unwrap_node(ctx, argv[0])) {
        return make_sel(ctx, h->ref, {h->node});
    }
    // $(array of handles)：包装
    if (JS_IsArray(argv[0])) {
        JSValue arr = JS_GetPropertyStr(ctx, argv[0], "length");
        uint32_t len = 0;
        if (!JS_IsException(arr)) {
            JS_ToUint32(ctx, &len, arr);
            JS_FreeValue(ctx, arr);
        }
        std::vector<lxb_dom_node_t*> nodes;
        for (uint32_t i = 0; i < len; ++i) {
            JSValue v = JS_GetPropertyUint32(ctx, argv[0], i);
            if (NodeHandle* h = unwrap_node(ctx, v))
                nodes.push_back(h->node);
            JS_FreeValue(ctx, v);
        }
        if (!nodes.empty())
            return make_sel(ctx, s->ref, std::move(nodes));
    }
    // $(null/undefined/空数组/其他)：空集合（cheerio 语义）
    return make_sel(ctx, s->ref, {});
}

// prev 为有效 JS 值时，结果集带 prevObject 属性（end() 语义：链式回退）
inline JSValue make_sel(JSContext* ctx, DomRef* ref,
                        std::vector<lxb_dom_node_t*> nodes,
                        JSValueConst prev)
{
    JSValue obj = JS_NewObjectClass(ctx, class_ids(JS_GetRuntime(ctx)).sel);
    if (JS_IsException(obj))
        return obj;
    auto* s = new CheerioSel;
    ref->retain();
    s->ref = ref;
    s->nodes = std::move(nodes);
    JS_SetOpaque(obj, s);
    if (!JS_IsUndefined(prev))
        s->prev = JS_DupValue(ctx, prev);
    return obj;
}

// 数字索引（经 get_property 惰性分发；length 已在上面处理）
inline bool sel_get_property(JSContext* ctx, JSValue obj, JSAtom atom,
                             JSValue* value)
{
    auto& ids = class_ids(JS_GetRuntime(ctx));
    auto* s = (CheerioSel*)JS_GetOpaque(obj, ids.sel);
    if (!s)
        return false;
    if (atom == ids.length) {
        *value = JS_NewUint32(ctx, (uint32_t)s->nodes.size());
        return true;
    }
    return false;
}

inline JSValue sel_get_property_ex(JSContext* ctx, JSValueConst obj,
                                   JSAtom atom, JSValueConst receiver)
{
    JSValue out = JS_UNDEFINED;
    if (sel_get_property(ctx, obj, atom, &out))
        return out;
    // 数字索引（必须是纯数字 atom，否则 'attr' 之类会被 strtol 误解析）
    {
        size_t nlen = 0;
        const char* name = JS_AtomToCStringLen(ctx, &nlen, atom);
        if (name) {
            bool numeric = nlen > 0;
            for (size_t i = 0; i < nlen; ++i) {
                if (name[i] < '0' || name[i] > '9') {
                    numeric = false;
                    break;
                }
            }
            if (numeric) {
                auto* s = (CheerioSel*)JS_GetOpaque(
                    obj, class_ids(JS_GetRuntime(ctx)).sel);
                if (s) {
                    long idx = strtol(name, nullptr, 10);
                    if (idx >= 0 && (size_t)idx < s->nodes.size())
                        return make_node(ctx, s->nodes[(size_t)idx], s->ref);
                }
            }
            JS_FreeCString(ctx, name);
        }
    }
    // own property（prevObject 等经 set_property/DefineProperty 写入的
    // 普通属性——exotic get_property 拦截一切查找，必须手动兜底）
    {
        JSPropertyDescriptor desc;
        if (JS_GetOwnProperty(ctx, &desc, obj, atom)) {
            JSValue v = JS_DupValue(ctx, desc.value);
            JS_FreeValue(ctx, desc.value);
            JS_FreeValue(ctx, desc.getter);
            JS_FreeValue(ctx, desc.setter);
            return v;
        }
    }
    // 未处理属性：查原型链（方法等）
    JSValue proto = JS_GetPrototype(ctx, obj);
    JSValue r = JS_GetProperty(ctx, proto, atom);
    JS_FreeValue(ctx, proto);
    return r;
}

// $ 索引赋值（sel[2] = node 语义，Cheerio 类数组支持）
inline int sel_set_property(JSContext* ctx, JSValue obj, JSAtom atom,
                            JSValue value, JSValue receiver, int flags)
{
    auto& ids = class_ids(JS_GetRuntime(ctx));
    auto* s = (CheerioSel*)JS_GetOpaque(obj, ids.sel);
    if (!s)
        return 0;
    size_t nlen = 0;
    const char* name = JS_AtomToCStringLen(ctx, &nlen, atom);
    if (name) {
        bool numeric = nlen > 0;
        for (size_t i = 0; i < nlen; ++i) {
            if (name[i] < '0' || name[i] > '9') {
                numeric = false;
                break;
            }
        }
        if (numeric) {
            long idx = strtol(name, nullptr, 10);
            NodeHandle* h = unwrap_node(ctx, value);
            if (h && idx >= 0 && (size_t)idx < s->nodes.size())
                s->nodes[(size_t)idx] = h->node;
            JS_FreeCString(ctx, name);
            return 1; // 数字索引：已处理（无效值静默忽略，与数组语义一致）
        }
        JS_FreeCString(ctx, name);
    }
    return 0;
}

inline int sel_set_property_ex(JSContext* ctx, JSValue obj, JSAtom atom,
                               JSValue value, JSValue receiver, int flags)
{
    if (sel_set_property(ctx, obj, atom, value, receiver, flags))
        return 1;
    // 非数字属性：必须自行完成设置（quickjs-ng exotic set_property 返回
    // 0 = 静默丢弃，不 fallthrough 到默认路径）
    return JS_DefineProperty(ctx, obj, atom, JS_DupValue(ctx, value),
                             JS_UNDEFINED, JS_UNDEFINED, JS_PROP_HAS_VALUE)
               ? 1
               : -1;
}

// $(arg)：字符串（HTML fragment / 选择器）、节点句柄、数组/类数组
inline JSValue sel_call(JSContext* ctx, JSValue func_obj, JSValue this_val,
                        int argc, JSValue* argv, int magic)
{
    return JS_UNDEFINED; // 10.4 实现（load 内部注册）
}

// ---------------------------------------------------------------------------
// 注册（安装入口）：node class + $ class 原型
// ---------------------------------------------------------------------------
inline void register_classes(JSRuntime* rt, JSContext* ctx)
{
    // 注意：class_ids 以裸 rt 指针为键，Runtime 销毁后条目可能被新
    // Runtime 复用（同地址），因此这里不做"已注册跳过"判断——每次
    // install 都全新注册（重复 install 罕见；旧对象 finalizer 因 class_id
    // 不匹配会跳过释放，仅泄漏，不崩溃）
    auto& ids = class_ids(rt);
    ids = ClassIds{};

    JS_NewClassID(rt, &ids.node);
    JS_NewClassID(rt, &ids.sel);

    JSClassDef node_def{};
    node_def.class_name = "LxbNode";
    node_def.finalizer = [](JSRuntime* rt, JSValue val) {
        auto& ids = class_ids(rt);
        auto* h = (NodeHandle*)JS_GetOpaque(val, ids.node);
        if (h) {
            if (h->ref) {
                h->ref->node_cache.erase(h->node);
                h->ref->release();
            }
            delete h;
        }
    };
    // 注意：JS_NewClass 保存 exotic 指针（不复制），必须静态存储
    static JSClassExoticMethods node_exo{};
    node_exo.get_property = node_get_property_ex;
    node_exo.set_property = node_set_property;
    node_def.exotic = &node_exo;
    JS_NewClass(rt, ids.node, &node_def);

    JSClassDef sel_def{};
    sel_def.class_name = "Cheerio";
    sel_def.finalizer = [](JSRuntime* rt, JSValue val) {
        auto& ids = class_ids(rt);
        auto* s = (CheerioSel*)JS_GetOpaque(val, ids.sel);
        if (s) {
            if (!JS_IsUndefined(s->prev)) {
                JS_FreeValueRT(rt, s->prev);
                s->prev = JS_UNDEFINED;
            }
            delete s;
        }
    };
    // end() 语义：prevObject 从 C++ 字段读取（见 fn_end）
    sel_def.call = sel_call_ex;
    static JSClassExoticMethods sel_exo{};
    sel_exo.get_property = sel_get_property_ex;
    sel_exo.set_property = sel_set_property_ex;
    sel_def.exotic = &sel_exo;
    JS_NewClass(rt, ids.sel, &sel_def);

    // $ 原型
    ids.sel_proto = JS_NewObject(ctx);
    JS_SetClassProto(ctx, ids.sel, ids.sel_proto);
    // 节点原型（必须设置：class 对象 prototype 为 null 时，惰性属性的
    // "查原型链"回退会读到 null 而抛 TypeError）。注意 JS_SetClassProto
    // 转移引用（不 dup），调用后不得 FreeValue。
    JSValue node_proto = JS_NewObject(ctx);
    JS_SetClassProto(ctx, ids.node, node_proto);

    fill_atoms(ctx, ids);
}

// ---------------------------------------------------------------------------
// 文档解析入口
// ---------------------------------------------------------------------------
inline DomRef* parse_document(JSContext* ctx, const std::string& html)
{
    lxb_html_document_t* doc = lxb_html_document_create();
    if (!doc)
        qjs::throw_type_error(ctx, "lexbor: document create failed");
    lxb_status_t st =
        lxb_html_document_parse(doc, (const lxb_char_t*)html.data(), html.size());
    if (st != LXB_STATUS_OK) {
        lxb_html_document_destroy(doc);
        qjs::throw_type_error(ctx, "lexbor: html parse failed");
    }
    return new DomRef{doc, 0};
}

} // namespace qjsbind::cheerio::lxb_handle
