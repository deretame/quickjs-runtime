// qjsbind::web —— TextEncoder / TextDecoder（UTF-8）
//
// 注意：不能直接用 JS_ToCString 做 encode —— quickjs-ng 对孤立代理不做替换
//（保留 CESU-8 风格编码），与 TextEncoder 规范（→ U+FFFD）不符。
// 正确路径：JS_ToCStringLenUTF16 取 UTF-16 单元 → utf16_to_utf8。
#pragma once

#include <qjsbind/class.hpp>
#include <qjsbind/context.hpp>
#include <qjsbind/web/errors.hpp>
#include <qjsbind/web/utf8.hpp>

#include <string>

namespace qjsbind::web {

struct TextEncoderImpl {
    std::string encoding() const { return "utf-8"; }
};

struct TextDecoderImpl {
    bool fatal = false;
    bool ignore_bom = false;

    std::string encoding() const { return "utf-8"; }

    // 构造 options：{fatal, ignoreBOM}（decode() 的 options 参数在方法内另读）
    void qjs_init(JSContext* ctx, qjs::Opt<qjs::Value> options) {
        if (options && options->is_object()) {
            qjs::Object obj(*options);
            qjs::Value fatal = obj.get("fatal");
            if (!fatal.is_undefined())
                this->fatal = fatal.as<bool>();
            qjs::Value ignore_bom = obj.get("ignoreBOM");
            if (!ignore_bom.is_undefined())
                this->ignore_bom = ignore_bom.as<bool>();
        }
    }
};

// 从 JS 值提取字节（TypedArray/ArrayBuffer → 拷贝；其他 → TypeError）
inline std::string js_bytes_from(JSContext* ctx, JSValueConst v) {
    if (JS_GetTypedArrayType(v) >= 0) {
        size_t byte_offset = 0, byte_length = 0, bytes_per_element = 0;
        JSValue buf =
            JS_GetTypedArrayBuffer(ctx, v, &byte_offset, &byte_length, &bytes_per_element);
        if (JS_IsException(buf))
            throw qjs::js_error(ctx, JS_GetException(ctx));
        size_t size = 0;
        uint8_t* data = JS_GetArrayBuffer(ctx, &size, buf);
        std::string out(reinterpret_cast<const char*>(data + byte_offset), byte_length);
        JS_FreeValue(ctx, buf);
        return out;
    }
    if (JS_IsArrayBuffer(v)) {
        size_t size = 0;
        uint8_t* data = JS_GetArrayBuffer(ctx, &size, v);
        return std::string(reinterpret_cast<const char*>(data), size);
    }
    throw_type_error(ctx, "TypeError: 参数不是字节数组");
}

inline void install_text_encoder(qjs::Context& ctx) {
    auto cls = qjs::class_<TextEncoderImpl>(ctx, "TextEncoder")
                   .constructor<>()
                   .getter("encoding", [](qjs::This<TextEncoderImpl> self) { return self->encoding(); })
                   .method("encode",
                [](qjs::Ctx ctx, qjs::This<TextEncoderImpl>, qjs::Value input) -> qjs::Value {
                    // JS_ToCStringLenUTF16 内部做 ToString 转换（含非字符串参数）
                    size_t len = 0;
                    const uint16_t* units = JS_ToCStringLenUTF16(ctx.ctx, &len, input.raw());
                    if (!units)
                        throw qjs::js_error(ctx.ctx, JS_GetException(ctx.ctx));
                    const std::string utf8 = utf16_to_utf8(units, len);
                    JS_FreeCStringUTF16(ctx.ctx, units);
                    return qjs::Value(
                        ctx.ctx,
                        JS_NewUint8ArrayCopy(ctx.ctx,
                                             reinterpret_cast<const uint8_t*>(utf8.data()),
                                             utf8.size()));
                });
    ctx.globals().set("TextEncoder", cls.constructor_function());
}

inline void install_text_decoder(qjs::Context& ctx) {
    auto cls = qjs::class_<TextDecoderImpl>(ctx, "TextDecoder")
                   .constructor<qjs::Opt<qjs::Value>>()
                   .getter("encoding", [](qjs::This<TextDecoderImpl> self) { return self->encoding(); })
                   .method("decode",
                [](qjs::Ctx ctx, qjs::This<TextDecoderImpl> self, qjs::Opt<qjs::Value> input,
                   qjs::Opt<qjs::Value> options) -> std::string {
                    if (options && options->is_object()) {
                        qjs::Object obj(*options);
                        qjs::Value fatal = obj.get("fatal");
                        if (!fatal.is_undefined())
                            self->fatal = fatal.as<bool>();
                        qjs::Value ignore_bom = obj.get("ignoreBOM");
                        if (!ignore_bom.is_undefined())
                            self->ignore_bom = ignore_bom.as<bool>();
                    }
                    if (!input || input->is_undefined() || input->is_null())
                        return {};
                    std::string bytes = js_bytes_from(ctx.ctx, input->raw());
                    // 规范：decode() 默认剥离 UTF-8 BOM；ignoreBOM: true 时保留
                    if (!self->ignore_bom && bytes.size() >= 3 &&
                        static_cast<uint8_t>(bytes[0]) == 0xEF &&
                        static_cast<uint8_t>(bytes[1]) == 0xBB &&
                        static_cast<uint8_t>(bytes[2]) == 0xBF)
                        bytes.erase(0, 3);
                    return bytes_to_valid_utf8(bytes);
                });
    ctx.globals().set("TextDecoder", cls.constructor_function());
}

} // namespace qjsbind::web
