// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The turn limit, pinned by counting the yields themselves (doc: #asio-invariant-3): a
// self-posting handler under run and run_for sees a yield after every max_handlers_per_turn
// handlers, never more handlers between two yields, and the page's setTimeout(0) runs at a
// yield, never inside a turn. Boost's own run, reached through a boost::asio::io_context&, never
// yields.

#include <webcpp/asio/io_context.hpp>

#include "page_timeout.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>

namespace {

using namespace std::chrono_literals;

/** A self-posting spin, the yields it sees, and the page's setTimeout(0) it watches for. */
struct turn {
    webcpp::asio::io_context io;
    webcpp::test::yield_counter yields;  // from here, before any handler runs
    webcpp::test::page_timeout timeout;  // armed here, before any handler runs
    std::size_t handlers = 0;
    std::size_t until = 0;       // 0: until the timeout has run
    int seen = 0;                // the yields so far, at the last handler
    std::size_t since = 0;       // handlers since the last yield, this one included
    std::size_t longest = 0;     // the most handlers between two yields
    std::size_t seen_at = 0;     // the handler that first saw the timeout run, from 1
    std::size_t seen_since = 0;  // since, at that handler
    int seen_yields = 0;         // the yields, at that handler
};

/**
 Runs one handler of the spin, and posts the next while the spin goes on.

 Tip: it posts itself through a function pointer, which the analysis does not follow: posted as
 a functor of its own, it would be a recursive call chain to it (misc-no-recursion), since
 Asio's post instantiates a call of its handler.
*/
void spin(turn* state) {
    ++state->handlers;
    const int now = state->yields.count();
    if (now != state->seen) {
        state->seen = now;
        state->since = 0;
    }
    ++state->since;
    state->longest = std::max(state->longest, state->since);
    if (state->seen_at == 0 && state->timeout.ran()) {
        state->seen_at = state->handlers;
        state->seen_since = state->since;
        state->seen_yields = now;
    }
    const bool more = state->until == 0 ? state->seen_at == 0 : state->handlers < state->until;
    if (more) {
        boost::asio::post(state->io, std::bind_front(&spin, state));
    }
}

// Every turn runs the limit exactly and then yields, under run: a yield between each two of the
// fifty turns, and none after the last, whose last handler leaves the context out of work.
void run_yields_every_turn() {
    turn state;
    state.until = 5000;
    boost::asio::post(state.io, std::bind_front(&spin, &state));
    BOOST_TEST_EQ(state.io.run(), 5000U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(state.longest, 100U);
    BOOST_TEST_EQ(state.yields.count(), 49);
}

// The same under run_for, whose deadline is never reached: twenty turns, nineteen yields.
void run_for_yields_every_turn() {
    turn state;
    state.io.max_handlers_per_turn(250);
    state.until = 5000;
    boost::asio::post(state.io, std::bind_front(&spin, &state));
    BOOST_TEST_EQ(state.io.run_for(10s), 5000U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(state.longest, 250U);
    BOOST_TEST_EQ(state.yields.count(), 19);
}

// The page's timer runs at a yield, and the first handler of a turn sees it: a turn suspends
// only to microtasks, which run no timer. The spin goes on until the timer has run, so no count
// of handlers is assumed. The case rests on one bound of the host's: a due setTimeout(0) runs
// within run_for's 10 s of yielding turns (node runs it within 1 ms, a browser's background tab
// within about 1 s); past it, the case fails at 10 s rather than hang.
void the_page_s_timer_runs_at_a_yield() {
    turn state;
    state.io.max_handlers_per_turn(250);
    boost::asio::post(state.io, std::bind_front(&spin, &state));
    state.io.run_for(10s);  // lint-run: the test's event loop
    BOOST_TEST(state.seen_at > 0);
    BOOST_TEST_EQ(state.seen_since, 1U);
    BOOST_TEST(state.seen_yields >= 1);
    BOOST_TEST(state.longest <= 250U);
}

void boost_s_own_run_never_yields() {
    turn state;
    state.until = 5000;
    boost::asio::post(state.io, std::bind_front(&spin, &state));
    boost::asio::io_context& boost_s = state.io;
    BOOST_TEST_EQ(boost_s.run(), 5000U);  // lint-run: Boost's own run, which has no limit
    BOOST_TEST_EQ(state.yields.count(), 0);
    BOOST_TEST_EQ(state.longest, 5000U);
    BOOST_TEST_EQ(state.seen_at, 0U);
}

}  // namespace

int main() {
    run_yields_every_turn();
    run_for_yields_every_turn();
    the_page_s_timer_runs_at_a_yield();
    boost_s_own_run_never_yields();
    return webcpp::test::exit_status(boost::report_errors());
}
