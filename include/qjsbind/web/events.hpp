// qjsbind::web —— Event / EventTarget（v1：同步派发、单 type 字符串事件）
//
// 注意：listeners 存 qjs::RtValue（JSRuntime 释放）而非 qjs::Function——
// EventTargetImpl 是 class opaque，析构发生在 JS_FreeRuntime 的最终 GC（ctx 已死），
// 只有 RtValue 能安全释放（JS_FreeValueRT，见 rt_value.hpp 设计注释）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/rt_value.hpp>
#include <qjsbind/value.hpp>

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace qjsbind::web {

// 事件对象：JS 侧为 { type: string, ... }（任何带 type 属性的对象均可派发）。
// v1 不校验事件类原型（wpt 相关测试只用 AbortSignal 的 abort 事件）。
struct Event {
    std::string type;

    void qjs_init(JSContext*, qjs::Opt<std::string> t) {
        if (t)
            type = *t;
    }
};

struct EventTargetImpl {
    // type → 监听器（插入序）
    std::vector<std::pair<std::string, std::vector<qjs::RtValue>>> listeners;

    void add_listener(JSContext* ctx, const std::string& type, qjs::Function cb) {
        qjs::RtValue fn(JS_GetRuntime(ctx), cb.take());
        for (auto& [t, fns] : listeners)
            if (t == type) {
                fns.push_back(std::move(fn));
                return;
            }
        // emplace_back：initializer_list 需要拷贝（RtValue 不可拷贝）
        listeners.emplace_back(type, std::vector<qjs::RtValue>{});
        listeners.back().second.push_back(std::move(fn));
    }

    bool remove_listener(JSContext* ctx, const std::string& type, const qjs::Function& cb) {
        for (auto& [t, fns] : listeners) {
            if (t != type)
                continue;
            for (auto it = fns.begin(); it != fns.end(); ++it)
                if (JS_IsStrictEqual(ctx, it->raw(), cb.raw())) { // 同一函数对象（严格相等）
                    fns.erase(it);
                    return true;
                }
        }
        return false;
    }

    bool has_listeners(const std::string& type) const {
        for (const auto& [t, fns] : listeners)
            if (t == type && !fns.empty())
                return true;
        return false;
    }

    // 同步派发；监听器抛出的异常吞掉（打印到 stderr 便于调试）
    void dispatch(JSContext* ctx, const std::string& type, JSValueConst event) {
        JSValue args[1] = {JS_DupValue(ctx, event)};
        for (auto& [t, fns] : listeners) {
            if (t != type)
                continue;
            for (auto& fn : fns) {
                qjs::Function fn_tmp(qjs::Value(ctx, fn.dup(ctx))); // 临时包装（dup，析构释放）
                qjs::Value r = fn_tmp.call_raw(1, args);
                if (r.is_exception()) {
                    JSValue exc = JS_GetException(ctx);
                    const char* str = JS_ToCString(ctx, exc);
                    std::fprintf(stderr, "[qjsbind::web] listener '%s' threw: %s\n", type.c_str(),
                                 str ? str : "?");
                    if (str)
                        JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, exc);
                }
            }
        }
        JS_FreeValue(ctx, args[0]);
    }

    // 派发 plain object 事件（{ type: "..." }）
    void dispatch_type(JSContext* ctx, const std::string& type) {
        JSValue ev = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, ev, "type", JS_NewString(ctx, type.c_str()));
        dispatch(ctx, type, ev);
        JS_FreeValue(ctx, ev);
    }

    // GC 标记：listeners 内的函数引用
    void qjs_mark(JSRuntime* rt, JS_MarkFunc* mark_func) {
        for (auto& [t, fns] : listeners)
            for (auto& fn : fns)
                fn.mark(rt, mark_func);
    }
};

// 给任意 class_ 注册 EventTarget 三件套（v1 无继承，组合式批量注册）
template <class Cls, class T>
Cls& install_event_target_methods(Cls& cls) {
    cls.method("addEventListener",
               [](qjs::Ctx ctx, qjs::This<T> self, const std::string& type, qjs::Function cb) {
                   self->add_listener(ctx.ctx, type, std::move(cb));
               });
    cls.method("removeEventListener",
               [](qjs::Ctx ctx, qjs::This<T> self, const std::string& type,
                  const qjs::Function& cb) { self->remove_listener(ctx.ctx, type, cb); });
    cls.method("dispatchEvent", [](qjs::Ctx ctx, qjs::This<T> self, qjs::Value ev) -> bool {
        // 从事件对象取 type（非字符串 → js_convert 抛 TypeError）
        const std::string t = qjs::Object(ev).get("type").as<std::string>();
        self->dispatch(ctx.ctx, t, ev.raw());
        return true;
    });
    return cls;
}

inline void install_event(qjs::Context& ctx) {
    auto cls = qjs::class_<Event>(ctx, "Event")
                   .constructor<qjs::Opt<std::string>>()
                   .getter("type", [](qjs::This<Event> self) { return self->type; });
    ctx.globals().set("Event", cls.constructor_function());
}

} // namespace qjsbind::web
