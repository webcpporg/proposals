// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/wasi/http.hpp>

namespace http = webcpp::beast::http;

namespace {
/** Application routing and response construction, independent of listener ownership. */
http::response<http::string_body> handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response{http::status::ok, request.version()};
    response.set(http::field::content_type, "text/plain");
    if (request.target() == "/health") {
        response.body() = "ready\n";
    } else {
        response.body() = request.body();
    }
    response.prepare_payload();
    return response;
}
}  // namespace

// The host owns the listener and any HTTPS trust policy.
WEBCPP_BEAST_WASI_HTTP_EXPORT(handle_request)
