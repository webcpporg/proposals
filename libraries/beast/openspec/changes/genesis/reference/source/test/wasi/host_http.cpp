// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/wasi/http.hpp>

#include <string>
#include <variant>

namespace http = webcpp::beast::http;

namespace {
std::variant<http::response<http::string_body>, http::response<http::empty_body>> handle(
    http::request<http::string_body> request) {
    http::response<http::string_body> response{http::status::ok, 11};
    response.set(http::field::content_type, "text/plain");
    response.set("x-method", request.method_string());
    response.set("x-target", request.target());
    if (request.target().starts_with("/client")) {
        webcpp::beast::wasi::client peer{
            "http", std::string_view(request["x-peer"].data(), request["x-peer"].size())};
        http::request<http::string_body> outbound{
            http::verb::post, request.target() == "/client-timeout" ? "/slow" : "/echo", 11};
        outbound.set("x-test", "outgoing");
        outbound.body() = request.body();
        outbound.prepare_payload();
        boost::system::error_code error;
        webcpp::beast::wasi::transfer_options options;
        if (request.target() == "/client-timeout") {
            options.expires_after(50000000);
        }
        http::fields trailers;
        http::fields upload_trailers;
        if (request.target() == "/client-trailers") {
            outbound.erase(http::field::content_length);
            outbound.set(http::field::trailer, "x-upload");
            upload_trailers.set("x-upload", "uploaded");
        }
        peer.perform(outbound, response, error, options, &trailers,
                     request.target() == "/client-trailers" ? &upload_trailers : nullptr);
        response.erase(http::field::transfer_encoding);
        response.erase(http::field::trailer);
        if (!error) {
            response.set("x-trailer", trailers["x-reply"]);
        }
        if (error) {
            response.result(http::status::bad_gateway);
            response.body() = error.message();
        }
    } else {
        response.set("x-test", request["x-test"]);
        response.body() = request.body().empty() ? "host HTTP ready\n" : request.body();
    }
    response.prepare_payload();
    return response;
}

}  // namespace

WEBCPP_BEAST_WASI_HTTP_EXPORT(handle)
