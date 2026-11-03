// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
// State requirements before the first WebCPP include, or use a compiler define.
#define WEBCPP_ASIO_REQUIRE_SOCKET_BACKEND 1
#include <cstdio>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    webcpp::asio::io_context context;
    webcpp::asio::ip::tcp::socket socket(context);
    boost::system::error_code error;
    if (socket.open(webcpp::asio::ip::tcp::v4(), error)) {
        // A compiled backend does not imply that the host granted socket access.
        const auto message = error.message();
        (void)std::fprintf(stderr, "socket open: %s\n", message.c_str());
        return webcpp::test::exit_status(1);
    }
    std::puts("TCP socket opened");
    return webcpp::test::exit_status(0);
}

// end::program[]
