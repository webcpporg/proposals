// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifdef WEBCPP_TEST_UPSTREAM_BEAST
#include <boost/beast/http.hpp>
namespace http = boost::beast::http;
#else
#include <webcpp/beast/http.hpp>
namespace http = webcpp::beast::http;
#endif

#include <iostream>

namespace {
// tag::handler[]
http::response<http::string_body> handle_request(const http::request<http::string_body>& request) {
    http::response<http::string_body> response{http::status::ok, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.body() = "Hello from WebCPP\n";
    response.prepare_payload();
    return response;
}

// end::handler[]
}  // namespace

// tag::program[]
int main() {
    const http::request<http::string_body> request{http::verb::get, "/hello", 11};
    const auto response = handle_request(request);
    std::cout << response.result_int() << ' ' << response.reason() << '\n';
    std::cout << response[http::field::content_type] << '\n';
    std::cout << response.body();
}

// end::program[]
