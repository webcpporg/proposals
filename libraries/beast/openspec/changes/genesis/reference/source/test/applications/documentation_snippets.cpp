// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/config.hpp>

#ifdef WEBCPP_HAS_EXCEPTIONS
#include "../../example/applications/port/doc/http_examples.hpp"
#else
#include "../../example/applications/noexcept-native/doc/http_examples.hpp"
#endif

#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>

#include <boost/core/lightweight_test.hpp>

#include <sstream>

int main() {
    namespace http = webcpp::beast::http;
    http::request<http::string_body> request{http::verb::post, "/snippet", 11};
    request.body() = "documentation body";
    request.prepare_payload();
    boost::system::error_code error;
    std::ostringstream output;
    boost::beast::http::write_ostream(output, request, error);
    BOOST_TEST(!error);
    std::istringstream input(output.str());
    webcpp::beast::flat_buffer buffer;
    http::request<http::string_body> decoded;
    boost::beast::http::read_istream(input, buffer, decoded, error);
    BOOST_TEST(!error);
    BOOST_TEST(decoded.method() == request.method());
    BOOST_TEST(decoded.target() == request.target());
    BOOST_TEST(decoded.body() == request.body());
    return boost::report_errors();
}
