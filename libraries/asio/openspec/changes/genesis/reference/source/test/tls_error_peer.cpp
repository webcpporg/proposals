// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include "tls_error_peer.hpp"
#include <webcpp/asio/tls_client_context.hpp>

boost::system::error_code webcpp::test::tls_error_from_library(int value) {
    return webcpp::asio::make_error_code(static_cast<webcpp::asio::tls_context_error>(value));
}
