// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <openssl/ssl.h>
#include <boost/core/lightweight_test.hpp>
#include <boost/system/result.hpp>
#include <limits>
#include <string>
#include <string_view>
#include <webcpp/asio/ssl.hpp>
#include <webcpp/test/exit_status.hpp>

// Windows callers can define this before including the public TLS factory.
#ifdef max
#undef max
#endif
// The lowercase name reproduces the Windows SDK macro.
// NOLINTNEXTLINE(readability-identifier-naming)
#define max(left, right) ((left) > (right) ? (left) : (right))
#include <webcpp/asio/tls_client_context.hpp>
#undef max

int main() {
    auto defaults = webcpp::asio::make_tls_client_context();
    BOOST_TEST(defaults.has_value());
    if (!defaults) {
        return webcpp::test::exit_status(boost::report_errors());
    }
    auto* handle = defaults->native_handle();
    BOOST_TEST_EQ(SSL_CTX_get_verify_mode(handle), SSL_VERIFY_PEER);
    BOOST_TEST_EQ(SSL_CTX_get_min_proto_version(handle), TLS1_2_VERSION);
    const auto* roots = X509_STORE_get0_objects(SSL_CTX_get_cert_store(handle));
    BOOST_TEST_GT(sk_X509_OBJECT_num(roots), 1);
    BOOST_TEST_EQ(static_cast<std::size_t>(sk_X509_OBJECT_num(roots)),
                  webcpp::asio::detail::ca_certificates::count);

    // Supplying one authority must replace, not append to, public trust.
    const std::string_view pem = webcpp::asio::detail::ca_certificates::pem;
    constexpr std::string_view end = "-----END CERTIFICATE-----";
    const auto first = pem.substr(0, pem.find(end) + end.size());
    auto replacement = webcpp::asio::make_tls_client_context(first);
    BOOST_TEST(replacement.has_value());
    if (replacement) {
        const auto* store = SSL_CTX_get_cert_store(replacement->native_handle());
        BOOST_TEST_EQ(sk_X509_OBJECT_num(X509_STORE_get0_objects(store)), 1);
        BOOST_TEST_EQ(SSL_CTX_get_verify_mode(replacement->native_handle()), SSL_VERIFY_PEER);
    }
    for (const std::string& invalid : {
             std::string{},
             std::string("garbage"),
             std::string(first) + "\n-----BEGIN CERTIFICATE-----\ntruncated",
             std::string(first) + "\ntrailing garbage",
             std::string("-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----"),
         }) {
        BOOST_TEST(!webcpp::asio::make_tls_client_context(invalid));
    }
    // Failure cannot contaminate subsequent independent contexts.
    BOOST_TEST(webcpp::asio::make_tls_client_context().has_value());
    return webcpp::test::exit_status(boost::report_errors());
}
