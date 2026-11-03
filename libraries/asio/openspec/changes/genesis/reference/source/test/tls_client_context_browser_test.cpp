// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <webcpp/asio/tls_client_context.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    const auto expected =
        webcpp::asio::make_error_code(webcpp::asio::tls_context_error::unsupported_target);
    BOOST_TEST_EQ(webcpp::asio::make_tls_client_context().error(), expected);
    BOOST_TEST_EQ(webcpp::asio::make_tls_client_context("unused").error(), expected);
    return webcpp::test::exit_status(boost::report_errors());
}
