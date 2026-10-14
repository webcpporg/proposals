// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>

#include <chrono>
#include <cstdio>

namespace asio = webcpp::asio;
using namespace std::chrono_literals;

int main() {
    asio::io_context io;
    asio::steady_timer timer(io);

    boost::asio::post(io, [&timer] {
        std::puts("posted");
        timer.expires_after(50ms);
        timer.async_wait([](boost::system::error_code ec) {
            if (ec) {
                std::printf("timer error: %d\n", ec.value());
            } else {
                std::puts("timer fired");
            }
        });
    });

    const auto handlers = io.run();  // lint-run: main owns the program's event loop
    std::printf("run returned %zu handlers\n", handlers);
    return webcpp::test::exit_status(0);
}

// end::program[]
