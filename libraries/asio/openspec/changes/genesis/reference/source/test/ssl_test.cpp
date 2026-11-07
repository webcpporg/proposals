// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <type_traits>
#include <webcpp/asio/ssl.hpp>
#include <webcpp/test/exit_status.hpp>
#ifdef __EMSCRIPTEN__
#include <string>

namespace {
void recorded_settings() {
    namespace ssl = webcpp::asio::ssl;
    ssl::context context(ssl::context::tlsv12_client);
    BOOST_TEST(context.recorded_method() == ssl::context::tlsv12_client);
    BOOST_TEST_EQ(context.recorded_verify_mode(), ssl::verify_none);
    BOOST_TEST(!context.default_verify_paths());
    BOOST_TEST(context.recorded_callback() == ssl::verify_callback_kind::none);
    BOOST_TEST(!context.roots_added());
    BOOST_TEST(!context.client_credentials());
    boost::system::error_code ec = boost::asio::error::operation_aborted;
    context.set_verify_mode(ssl::verify_peer, ec);
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(context.recorded_verify_mode(), ssl::verify_peer);
    context.set_default_verify_paths(ec);
    BOOST_TEST(!ec);
    BOOST_TEST(context.default_verify_paths());
    context.set_verify_callback(ssl::host_name_verification("example.com"), ec);
    BOOST_TEST(!ec);
    BOOST_TEST(context.recorded_callback() == ssl::verify_callback_kind::host_name_verification);
    BOOST_TEST(context.verification_host() == "example.com");
    context.set_verify_callback([](bool, auto&) { return true; }, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(context.recorded_callback() == ssl::verify_callback_kind::other);
    BOOST_TEST(context.verification_host().empty());
    context.add_certificate_authority(boost::asio::buffer("root"), ec);
    BOOST_TEST(!ec);
    BOOST_TEST(context.roots_added());
    context.use_private_key(boost::asio::buffer("key"), ssl::context::pem, ec);
    BOOST_TEST(!ec);
    BOOST_TEST(context.client_credentials());
    context.set_options(ssl::context::default_workarounds | ssl::context::no_sslv3, ec);
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(context.recorded_options(),
                  ssl::context::default_workarounds | ssl::context::no_sslv3);
    // Every input-setting form records intent without reading a file or parsing a certificate.
    auto succeeds = [&](auto setting) {
        ssl::context current(ssl::context::tls_client);
        ec = boost::asio::error::operation_aborted;
        setting(current, ec);
        BOOST_TEST(!ec);
        return current;
    };
    BOOST_TEST(
        succeeds([](auto& c, auto& e) { c.load_verify_file("missing.pem", e); }).roots_added());
    BOOST_TEST(succeeds([](auto& c, auto& e) { c.add_verify_path("missing", e); }).roots_added());
    BOOST_TEST(succeeds([](auto& c, auto& e) {
                   c.use_certificate(boost::asio::buffer("cert"), ssl::context::pem, e);
               }).client_credentials());
    BOOST_TEST(succeeds([](auto& c, auto& e) {
                   c.use_certificate_file("missing", ssl::context::pem, e);
               }).client_credentials());
    BOOST_TEST(succeeds([](auto& c, auto& e) {
                   c.use_certificate_chain(boost::asio::buffer("cert"), e);
               }).client_credentials());
    BOOST_TEST(succeeds([](auto& c, auto& e) {
                   c.use_certificate_chain_file("missing", e);
               }).client_credentials());
    BOOST_TEST(succeeds([](auto& c, auto& e) {
                   c.use_private_key_file("missing", ssl::context::pem, e);
               }).client_credentials());
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    context.set_verify_mode(ssl::verify_peer);
    context.set_default_verify_paths();
    context.set_verify_callback(ssl::host_name_verification("other.example"));
    context.add_certificate_authority(boost::asio::buffer("root"));
    context.load_verify_file("missing");
    context.add_verify_path("missing");
    context.use_certificate(boost::asio::buffer("cert"), ssl::context::pem);
    context.use_certificate_file("missing", ssl::context::pem);
    context.use_certificate_chain(boost::asio::buffer("cert"));
    context.use_certificate_chain_file("missing");
    context.use_private_key(boost::asio::buffer("key"), ssl::context::pem);
    context.use_private_key_file("missing", ssl::context::pem);
    context.set_options(ssl::context::no_tlsv1);
#endif
    // A verification object owns its name, including when constructed from a temporary.
    const ssl::host_name_verification host(std::string(100, 'x'));
    BOOST_TEST_EQ(host.host().size(), 100U);
    context.set_verify_callback(host, ec);
    BOOST_TEST_EQ(context.verification_host().size(), 100U);
}
}  // namespace
#else
static_assert(std::is_same_v<webcpp::asio::ssl::context, boost::asio::ssl::context>);
static_assert(std::is_same_v<webcpp::asio::ssl::stream_base, boost::asio::ssl::stream_base>);
#endif
int main() {
#ifdef __EMSCRIPTEN__
    recorded_settings();
#else
    const webcpp::asio::ssl::context context(webcpp::asio::ssl::context::tlsv12_client);
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
