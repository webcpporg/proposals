// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <cstdio>
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    webcpp::asio::io_context io;
    auto resolved = emscripten::val::global("Promise").call<emscripten::val>("resolve", 42);
    auto rejected = emscripten::val::global("Promise").call<emscripten::val>("reject", 9);
    int failures = 0;
    webcpp::asio::browser::async_await(
        io.get_executor(), resolved,
        [&](boost::system::error_code ec, const emscripten::val& value) {
            if (ec) {
                ++failures;
                return;
            }
            std::printf("resolved: %d\n", value.as<int>());
        });
    webcpp::asio::browser::async_await(
        io.get_executor(), rejected,
        [&](boost::system::error_code ec, const emscripten::val& reason) {
            if (ec != webcpp::asio::browser::error::promise_rejected) {
                ++failures;
                return;
            }
            std::printf("rejected: promise_rejected %d\n", reason.as<int>());
        });
    io.run();  // lint-run: main waits for both Promise completions
    return webcpp::test::exit_status(failures);
}

// end::program[]
