// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/error.hpp>

#include <chrono>
#include <cstdio>

namespace asio = webcpp::asio;
using namespace std::chrono_literals;

int main() {
    asio::io_context io;
    const auto start = std::chrono::steady_clock::now();
    asio::steady_timer sixty(io, start + 60ms);
    asio::steady_timer twenty(io, start + 20ms);
    asio::steady_timer forty(io, start + 40ms);
    const auto report = [](const char* label) {
        return [label](boost::system::error_code ec) {
            if (ec) {
                std::printf("timer error: %d\n", ec.value());
            } else {
                std::puts(label);
            }
        };
    };
    sixty.async_wait(report("60 ms"));
    twenty.async_wait(report("20 ms"));
    forty.async_wait(report("40 ms"));
    io.run();  // lint-run: main waits for the three deadlines

    io.restart();
    asio::steady_timer cancelled(io, std::chrono::steady_clock::time_point::max());
    cancelled.async_wait([](boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) {
            std::puts("cancelled: operation_aborted");
        } else {
            std::printf("unexpected cancellation result: %d\n", ec.value());
        }
    });
    cancelled.cancel();
    io.run();  // lint-run: main delivers the cancelled wait's completion
    return webcpp::test::exit_status(0);
}

// end::program[]
