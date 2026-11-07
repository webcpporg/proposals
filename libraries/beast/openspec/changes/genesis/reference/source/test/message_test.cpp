// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/core/lightweight_test.hpp>

#include <string>
#include <type_traits>

namespace beast = webcpp::beast;
namespace http = beast::http;

static_assert(std::is_same_v<beast::flat_buffer, boost::beast::flat_buffer>);
static_assert(std::is_same_v<http::request<http::string_body>,
                             boost::beast::http::request<boost::beast::http::string_body>>);

int main() {
    const std::string payload("hello\0wasm", 10);
    http::request<http::string_body> request{http::verb::post, "/echo", 11};
    request.set(http::field::host, "localhost");
    request.body() = payload;
    request.prepare_payload();
    BOOST_TEST_EQ(request.payload_size().value_or(0), payload.size());
    const std::string wire =
        "POST /echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\n" + payload;
    http::request_parser<http::string_body> parser;
    parser.eager(true);
    boost::system::error_code error;
    const auto consumed = parser.put(boost::asio::buffer(wire), error);
    BOOST_TEST(!error);
    BOOST_TEST_EQ(consumed, wire.size());
    BOOST_TEST(parser.is_done());
    BOOST_TEST_EQ(parser.get().body(), payload);
    BOOST_TEST_EQ(parser.get().target(), "/echo");

    http::request_parser<http::string_body> limited;
    limited.body_limit(4);
    limited.eager(true);
    limited.put(boost::asio::buffer(wire), error);
    BOOST_TEST(error == http::error::body_limit);

    http::request_parser<http::string_body> malformed;
    const std::string invalid = "GET / HTTP/1.1\r\nBad Header: value\r\n\r\n";
    malformed.put(boost::asio::buffer(invalid), error);
    BOOST_TEST(error == http::error::bad_field);
    return boost::report_errors();
}
