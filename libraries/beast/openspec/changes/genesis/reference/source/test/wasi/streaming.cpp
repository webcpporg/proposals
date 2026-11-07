// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/wasi/http.hpp>

namespace host = webcpp::wasi::http;
namespace wasi = webcpp::beast::wasi;
namespace http = webcpp::beast::http;

namespace {
void handle(host::request_handle raw, host::response_out out) {
    host::incoming_request incoming(raw);
    const auto target = incoming.target();
    if (target == "/fail-after-commit") {
        host::message_fields fields;
        static_cast<void>(fields.append("content-length", "100"));
        host::message_error error;
        auto writer = host::commit_response(out, 200, std::move(fields), error);
        if (error) {
            return;
        }
        static_cast<void>(writer.write("abc"));
        // No finish: the only committed response must be failed by its ABI owner.
        return;
    }
    http::request<http::string_body> request;
    http::fields trailers;
    boost::system::error_code error;
    wasi::transfer_options options;
    std::size_t checks = 0;
    if (target == "/cancel-read") {
        options.cancellation_context = &checks;
        options.cancelled = [](void* context) { return ++*static_cast<std::size_t*>(context) > 2; };
    }
    if (target == "/read-timeout") {
        options.expires_after(50000000);
    }
    wasi::receive(incoming, request, error, options, &trailers);
    http::response<http::string_body> response{
        error ? http::status::request_timeout : http::status::ok, 11};
    response.set("x-request-trailer", trailers["x-upload"]);
    response.body() = error ? error.message() : request.body();
    http::fields outgoing_trailers;
    outgoing_trailers.set("x-complete", "finished");
    if (target == "/trailers") {
        response.set(http::field::trailer, "x-complete");
        wasi::send(out, response, error, wasi::transfer_options(), &outgoing_trailers);
    } else {
        response.prepare_payload();
        wasi::send(out, response, error);
    }
}

}  // namespace

#ifdef WEBCPP_WASI_HTTP_P2
extern "C" void exports_wasi_http_incoming_handler_handle(
    exports_wasi_http_incoming_handler_own_incoming_request_t request,
    exports_wasi_http_incoming_handler_own_response_outparam_t out) {
    handle(request, out);
}
#else
extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle(
    exports_wasi_http_handler_own_request_t request) {
    handle(request, {});
    return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;
}

extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle_callback(
    webcpp_wasi_http_event_t* /*unused*/ /*unused*/) {
    return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;
}
#endif
