// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <webcpp/asio/io_context.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>

#include <cstddef>
#include <cstdio>
#include <functional>

namespace asio = webcpp::asio;

namespace {

/** The context and remaining work of a finite chain of posted handlers. */
struct work {
    asio::io_context& io;
    std::size_t remaining;
};

/** Runs one piece of work and schedules the next without calling it inline. */
void next(work* state) {
    if (--state->remaining != 0) {
        boost::asio::post(state->io, std::bind_front(&next, state));
    }
}

}  // namespace

int main() {
    asio::io_context io(asio::io_context_options{.max_handlers_per_turn = 250});
    work state{.io = io, .remaining = 5000};
    boost::asio::post(io, std::bind_front(&next, &state));
    std::printf("max_handlers_per_turn: %zu\n", io.max_handlers_per_turn());
    const auto handlers = io.run();  // lint-run: main owns the program's event loop
    std::printf("handlers run: %zu\n", handlers);
    return webcpp::test::exit_status(0);
}

// end::program[]
