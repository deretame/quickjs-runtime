// qjsbind::web —— Web API 层总入口（header-only）
//
// install_web_apis(ctx, backend) 注册全部全局：
//   DOMException / Event / TextEncoder / TextDecoder / URL / URLSearchParams /
//   AbortController / AbortSignal / Headers / Request / Response / Blob / File /
//   FormData / fetch /
//   setTimeout / setInterval / clearTimeout / clearInterval
// backend 由调用方注入（默认 beast+OpenSSL 实现见 src/net/http_backend）。
#pragma once

#include <qjsbind/web/abort.hpp>
#include <qjsbind/web/blob.hpp>
#include <qjsbind/web/dom_exception.hpp>
#include <qjsbind/web/encoding.hpp>
#include <qjsbind/web/events.hpp>
#include <qjsbind/web/blob.hpp>
#include <qjsbind/web/formdata.hpp> // 先于 fetch/request_response（extract_body 用 FormData 编码）
#include <qjsbind/web/fetch.hpp>
#include <qjsbind/web/headers.hpp>
#include <qjsbind/web/net.hpp>
#include <qjsbind/web/request_response.hpp>
#include <qjsbind/web/stream.hpp>
#include <qjsbind/web/timers.hpp>
#include <qjsbind/web/url.hpp>

#include <memory>

namespace qjsbind::web {

inline void install_web_apis(
    qjs::Context& ctx, std::shared_ptr<FetchBackend> backend,
    std::vector<std::shared_ptr<FetchInterceptor>> interceptors = {
        std::make_shared<AcceptEncodingInterceptor>() }) {
    install_dom_exception(ctx);
    install_event(ctx);
    install_text_encoder(ctx);
    install_text_decoder(ctx);
    install_url(ctx);
    install_abort(ctx);
    install_headers(ctx);
    install_readable_stream(ctx); // 先于 Request/Response（body getter 返回流）
    install_request(ctx);
    install_response(ctx);
    install_blob(ctx);
    install_form_data(ctx);
    install_timers(ctx);
    install_fetch(ctx, std::move(backend), std::move(interceptors));
}

} // namespace qjsbind::web
