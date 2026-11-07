// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <cstdio>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    webcpp::asio::io_context io;
    webcpp::asio::ip::tcp::socket socket(io);
    boost::system::error_code ec;
    int failures = 0;
    const auto open_result = socket.open(webcpp::asio::ip::tcp::v4(), ec);
    const bool opened = !open_result && !ec && socket.is_open();
    std::printf("open: %s\n", opened ? "true" : "false");
    failures += static_cast<int>(!opened);
    const auto connect_result = socket.connect({boost::asio::ip::address_v4::loopback(), 80}, ec);
    const bool unsupported =
        connect_result == ec && ec == boost::asio::error::operation_not_supported;
    std::printf("connect: %s\n", unsupported ? "operation_not_supported" : "unexpected");
    failures += static_cast<int>(!unsupported);
    const auto close_result = socket.close(ec);
    const bool closed = !close_result && !ec && !socket.is_open();
    std::printf("close: %s\n", closed ? "ok" : "unexpected");
    failures += static_cast<int>(!closed);
    return webcpp::test::exit_status(failures);
}

// end::program[]
