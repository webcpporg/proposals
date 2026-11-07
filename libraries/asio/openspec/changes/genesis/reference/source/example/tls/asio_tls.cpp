// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <cstdio>
#include <webcpp/asio/ssl.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    webcpp::asio::ssl::context context(webcpp::asio::ssl::context::tlsv12_client);
    context.set_verify_mode(webcpp::asio::ssl::verify_peer);
    std::puts("context: tlsv12_client, verify_peer");
    return webcpp::test::exit_status(0);
}

// end::program[]
