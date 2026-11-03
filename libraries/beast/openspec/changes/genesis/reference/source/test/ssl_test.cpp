// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/ssl.hpp>
#include <webcpp/beast/tcp_stream.hpp>

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ssl.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

namespace beast = webcpp::beast;

int main() {
    webcpp::asio::io_context context;
    webcpp::asio::ssl::context tls(webcpp::asio::ssl::context::tls_client);
    beast::ssl_stream<beast::tcp_stream> stream(context, tls);
    static_assert(std::is_same_v<decltype(stream), boost::beast::ssl_stream<beast::tcp_stream>>);
    const std::array source{'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', 'x', 'x', 'x', 'x', 'x'};
    const std::string_view host(source.data(), 9);
    BOOST_TEST(beast::set_server_name(stream, host).has_value());
    const char* recorded = SSL_get_servername(stream.native_handle(), TLSEXT_NAMETYPE_host_name);
    if (BOOST_TEST(recorded != nullptr)) {
        BOOST_TEST_EQ(std::string_view(recorded), "localhost");
    }
    BOOST_TEST(beast::set_server_name(stream, "").has_error());
    const std::string too_long(256, 'x');
    BOOST_TEST(beast::set_server_name(stream, too_long).has_error());
    const std::string embedded_null("good\0bad", 8);
    BOOST_TEST(beast::set_server_name(stream, embedded_null).has_error());
    recorded = SSL_get_servername(stream.native_handle(), TLSEXT_NAMETYPE_host_name);
    if (BOOST_TEST(recorded != nullptr)) {
        BOOST_TEST_EQ(std::string_view(recorded), "localhost");
    }
    return boost::report_errors();
}
