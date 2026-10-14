// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Continuous ready work exposes whether the page gets animation opportunities.
#include <webcpp/asio.hpp>

#include <boost/asio/post.hpp>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <functional>

namespace {

struct spinner {
    webcpp::asio::io_context& io;
    std::chrono::steady_clock::time_point deadline;
    std::size_t& handlers;
};

// As in turn_limit_test, a bound function pointer avoids a false recursive-call
// diagnostic: post queues this handler; it never calls spin on this stack.
void spin(spinner const* state) {
    ++state->handlers;
    if (std::chrono::steady_clock::now() < state->deadline) {
        boost::asio::post(state->io, std::bind_front(&spin, state));
    }
}

}  // namespace

int main() {
    webcpp::asio::io_context io;
    std::size_t handlers = 0;
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    spinner const state{.io = io, .deadline = deadline, .handlers = handlers};
    boost::asio::post(io, std::bind_front(&spin, &state));
    std::puts("spinning");
    static_cast<boost::asio::io_context&>(io).run();  // lint-run: Boost's own run, the control
    std::printf("done %zu\n", handlers);
    return 0;
}
