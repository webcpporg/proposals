// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#define WEBCPP_ASIO_REQUIRE_SOCKET_BACKEND 0
#define WEBCPP_ASIO_REQUIRE_OPENSSL 0
#include <webcpp/asio.hpp>
#include <webcpp/asio/browser/transport.hpp>
#include <webcpp/asio/ssl.hpp>
#include <webcpp/asio/tls_client_context.hpp>

// These settings are also consumed by browser-native WSS or custom transports.
static_assert(requires(webcpp::asio::ssl::context& context, boost::system::error_code& error) {
    context.set_verify_mode(webcpp::asio::ssl::verify_peer, error);
});
