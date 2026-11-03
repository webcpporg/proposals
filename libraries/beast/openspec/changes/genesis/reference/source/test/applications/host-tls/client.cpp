// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/wasi/async.hpp>

#include <cstdlib>
#include <string_view>

namespace http = webcpp::beast::http;
namespace wasi = webcpp::beast::wasi;

namespace {
http::response<http::string_body> handle(http::request<http::string_body> incoming) {
    const auto authority = incoming["x-peer"];
    wasi::client client{"https", std::string_view(authority.data(), authority.size())};
    http::request<http::string_body> request{http::verb::post, "/tls-echo", 11};
    request.set(http::field::content_type, "application/octet-stream");
    request.body() = incoming.body();
    request.prepare_payload();
    http::response<http::string_body> response;
    http::fields trailers;
    boost::system::error_code error;
    wasi::transfer_options options;
    options.expires_after(10000000000);
    if (incoming.target() == "/async") {
        wasi::http_context context;
        bool done = false;
        wasi::async_perform(
            context, client, request, response,
            [&](boost::system::error_code failure, std::size_t) {
                error = failure;
                done = true;
            },
            options, &trailers);
        context.run();
        if (!done) {
            std::abort();
        }
    } else {
        client.perform(request, response, error, options, &trailers);
    }
    if (error) {
        response = {http::status::bad_gateway, 11};
        response.body() = error.message();
        response.set("x-host-error", std::to_string(error.value()));
    } else {
        response.erase(http::field::transfer_encoding);
        response.erase(http::field::trailer);
        response.set("x-tls-trailer", trailers["x-tls-finished"]);
    }
    response.prepare_payload();
    return response;
}
}  // namespace

WEBCPP_BEAST_WASI_HTTP_EXPORT(handle)
