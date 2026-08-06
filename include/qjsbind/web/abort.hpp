// qjsbind::web —— AbortController / AbortSignal
//
// 取消链路：AbortSignalImpl 持有 std::stop_source；
//   abort() → stop.request_stop() → 网络层 stop_callback → socket.cancel()
//   → asio operation_aborted → use_sender set_stopped → Promise reject AbortError。
// 生命周期：AbortSignalImpl 由 signal JS 对象 opaque 持有；AbortControllerImpl
//   缓存 signal 的 JSValue（RtValue，JSRuntime 释放——见 rt_value.hpp）。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/rt_value.hpp>
#include <qjsbind/web/dom_exception.hpp>
#include <qjsbind/web/events.hpp>

#include <stop_token>

namespace qjsbind::web {

// 创建 DOMException("The operation was aborted", "AbortError") 实例
inline qjs::Value make_abort_error(JSContext* ctx) {
    JSClassID id = qjs::registry_of(ctx).id_of<DomException>(ctx);
    JSValue proto = JS_GetClassProto(ctx, id);
    JSValue obj = JS_NewObjectProtoClass(ctx, proto, id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj))
        throw qjs::js_error(ctx, JS_GetException(ctx));
    JS_SetOpaque(obj, new DomException("The operation was aborted", "AbortError"));
    return qjs::Value(ctx, obj);
}

struct AbortSignalImpl : EventTargetImpl {
    std::stop_source stop; // 桥接 C++ 网络层取消
    bool aborted = false;

    void abort(JSContext* ctx) {
        if (aborted)
            return;
        aborted = true;
        stop.request_stop();
        dispatch_type(ctx, "abort");
    }

    void throw_if_aborted(JSContext* ctx) const {
        if (aborted)
            throw qjs::js_error(ctx, make_abort_error(ctx).take());
    }
};

// 创建 signal JS 对象（opaque = 传入的 impl，所有权转移给 JS）
inline qjs::Value make_signal_object(JSContext* ctx, AbortSignalImpl* impl) {
    JSClassID id = qjs::registry_of(ctx).id_of<AbortSignalImpl>(ctx);
    JSValue proto = JS_GetClassProto(ctx, id);
    JSValue obj = JS_NewObjectProtoClass(ctx, proto, id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj))
        throw qjs::js_error(ctx, JS_GetException(ctx));
    JS_SetOpaque(obj, impl);
    return qjs::Value(ctx, obj);
}

struct AbortControllerImpl {
    AbortSignalImpl* signal = nullptr; // 借用（由 signal JS 对象 opaque 持有）
    qjs::RtValue signal_js;            // 缓存（首次访问创建；RtValue 释放安全）

    void qjs_init(JSContext* ctx) {
        signal = new AbortSignalImpl();
        signal_js = qjs::RtValue(JS_GetRuntime(ctx), make_signal_object(ctx, signal).take());
    }

    // GC 标记：缓存 signal 对象
    void qjs_mark(JSRuntime* rt, JS_MarkFunc* mark_func) { signal_js.mark(rt, mark_func); }
};

inline void install_abort(qjs::Context& ctx) {
    // ---- AbortSignal ----
    auto sig = qjs::class_<AbortSignalImpl>(ctx, "AbortSignal").constructor<>();
    install_event_target_methods<decltype(sig), AbortSignalImpl>(sig);
    sig.getter("aborted", [](qjs::This<AbortSignalImpl> self) { return self->aborted; })
        .getter("reason", [](qjs::Ctx ctx, qjs::This<AbortSignalImpl> self) -> qjs::Value {
            // v1：abort 后返回 AbortError DOMException；否则 undefined
            return self->aborted ? make_abort_error(ctx.ctx) : qjs::Value(ctx.ctx, JS_UNDEFINED);
        })
        .method("throwIfAborted", [](qjs::Ctx ctx, qjs::This<AbortSignalImpl> self) {
            self->throw_if_aborted(ctx.ctx);
        })
        .static_method("abort", [](qjs::Ctx ctx) -> qjs::Value {
            auto* impl = new AbortSignalImpl();
            impl->aborted = true;
            impl->stop.request_stop();
            return make_signal_object(ctx.ctx, impl);
        });
    ctx.globals().set("AbortSignal", sig.constructor_function());

    // ---- AbortController ----
    auto ctl = qjs::class_<AbortControllerImpl>(ctx, "AbortController")
                   .constructor<>()
                   .getter("signal", [](qjs::Ctx ctx, qjs::This<AbortControllerImpl> self) -> qjs::Value {
                       // 首次访问已由 qjs_init 创建；返回 dup
                       return qjs::Value(ctx.ctx, self->signal_js.dup(ctx.ctx));
                   })
                   .method("abort", [](qjs::Ctx ctx, qjs::This<AbortControllerImpl> self) {
                       self->signal->abort(ctx.ctx);
                   });
    ctx.globals().set("AbortController", ctl.constructor_function());
}

} // namespace qjsbind::web
