// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <webcpp/asio/tls_client_context.hpp>
#include "tls_error_peer.hpp"

int main() {
    using webcpp::asio::tls_context_error;
    for (const auto value : {
             tls_context_error::invalid_ca_bundle,
             tls_context_error::initialization_failed,
             tls_context_error::unsupported_target,
         }) {
        const auto local = webcpp::asio::make_error_code(value);
        const auto imported = webcpp::test::tls_error_from_library(static_cast<int>(value));
        BOOST_TEST_NE(&local.category(), &imported.category());
        BOOST_TEST_EQ(local.value(), imported.value());
        BOOST_TEST_CSTR_EQ(local.category().name(), imported.category().name());
        BOOST_TEST(local == imported);
        BOOST_TEST(local.default_error_condition() == imported.default_error_condition());
        BOOST_TEST(local != boost::system::errc::make_error_code(
                                boost::system::errc::operation_not_permitted));
    }
    return boost::report_errors();
}
