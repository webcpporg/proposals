// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The run family keeps Boost.Asio's meaning on every target (doc: #asio-run): run
// returns out of work with the count of the handlers it ran, run_one runs one, run waits for a
// timer, the four timed forms return at their deadline, and stop and restart are Boost's. At
// the inputs' edges: a deadline ends a run whose handlers are always ready, though every turn
// reaches the limit and yields; a stop made by the handler that reaches the limit returns at
// once, without a yield; and a limit of 0 is taken as 1, which still runs every handler.

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include "page_timeout.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <cstddef>
#include <functional>

namespace {

using namespace std::chrono_literals;

// The two handlers below post themselves through a function pointer, which the analysis does not
// follow: posted as a functor of its own, each would be a recursive call chain to it
// (misc-no-recursion), since Asio's post instantiates a call of its handler.

/** A stop_at that never stops the context. */
constexpr std::size_t never = 0;

/** Counts its runs, stops the context at stop_at, and posts itself again even then. */
void stop_then_post(webcpp::asio::io_context* io, std::size_t* count, std::size_t stop_at) {
    if (++*count == stop_at) {
        io->stop();
    }
    boost::asio::post(*io, std::bind_front(&stop_then_post, io, count, stop_at));
}

/** Posts itself until it stops the context at stop_at, counting its runs. */
void spin(webcpp::asio::io_context* io, std::size_t* count, std::size_t stop_at) {
    ++*count;
    if (stop_at != never && *count == stop_at) {
        io->stop();
        return;
    }
    boost::asio::post(*io, std::bind_front(&spin, io, count, stop_at));
}

void run_returns_out_of_work_with_the_handlers_run() {
    webcpp::asio::io_context io;
    int ran = 0;
    for (int i = 0; i < 3; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    BOOST_TEST_EQ(io.run(), 3U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 3);
    BOOST_TEST(io.stopped());
}

void run_one_runs_one() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run_one(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
}

void run_waits_for_a_timer() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 30ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code ec) { fired = !ec; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(fired);
}

void the_timed_forms_return_at_their_deadline() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer later(io, 10s);
    bool cancelled = false;
    later.async_wait([&cancelled](boost::system::error_code ec) {
        cancelled = ec == boost::asio::error::operation_aborted;
    });
    BOOST_TEST_EQ(io.run_for(50ms), 0U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() + 50ms), 0U);
    BOOST_TEST_EQ(io.run_one_for(50ms), 0U);
    BOOST_TEST_EQ(io.run_one_until(std::chrono::steady_clock::now() + 50ms), 0U);
    BOOST_TEST(!io.stopped());
    later.cancel();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(cancelled);
}

void stop_and_restart() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&] {
        ++ran;
        io.stop();
    });
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(io.stopped());
    io.restart();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 2);
}

// A deadline already passed runs nothing, as Boost's: the handlers wait for the next run.
void a_deadline_already_passed_runs_nothing() {
    webcpp::asio::io_context io;
    int ran = 0;
    for (int i = 0; i < 3; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    BOOST_TEST_EQ(io.run_for(0ms), 0U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() - 1s), 0U);
    BOOST_TEST_EQ(ran, 0);
    BOOST_TEST_EQ(io.run(), 3U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 3);
}

// Every turn reaches the limit and yields, and the deadline still ends the run.
void a_deadline_ends_a_run_whose_handlers_are_always_ready() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 100});
    std::size_t count = 0;
    boost::asio::post(io, std::bind_front(&spin, &io, &count, never));
    const auto start = std::chrono::steady_clock::now();
    const std::size_t ran = io.run_for(100ms);  // lint-run: the test's event loop
    const auto took = std::chrono::steady_clock::now() - start;
    BOOST_TEST(ran > 0);
    BOOST_TEST_EQ(ran, count);
    // The upper bound rests only on one turn of 100 trivial handlers and one MessageChannel
    // yield taking less than 1.9 s past the deadline.
    BOOST_TEST(took >= 100ms && took < 2s);
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() + 50ms) > 0, true);
}

// A stop by the handler that reaches the limit returns at once, without a yield (a
// MessageChannel message, which the yield counter sees). The handler posts itself again after
// its stop, so the context still has work: only the stopped() check ends the run.
void a_stop_at_the_limit_returns_at_once() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 10});
    std::size_t count = 0;
    const webcpp::test::yield_counter yields;
    boost::asio::post(io, std::bind_front(&stop_then_post, &io, &count, std::size_t{10}));
    BOOST_TEST_EQ(io.run(), 10U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(count, 10U);
    BOOST_TEST_EQ(yields.count(), 0);
}

// A limit of 0 is taken as 1, which still runs every handler.
void a_limit_of_zero_still_runs() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 0});
    std::size_t count = 0;
    boost::asio::post(io, std::bind_front(&spin, &io, &count, std::size_t{50}));
    BOOST_TEST_EQ(io.run(), 50U);  // lint-run: the test's event loop
}

}  // namespace

int main() {
    run_returns_out_of_work_with_the_handlers_run();
    run_one_runs_one();
    run_waits_for_a_timer();
    the_timed_forms_return_at_their_deadline();
    stop_and_restart();
    a_deadline_already_passed_runs_nothing();
    a_deadline_ends_a_run_whose_handlers_are_always_ready();
    a_stop_at_the_limit_returns_at_once();
    a_limit_of_zero_still_runs();
    return webcpp::test::exit_status(boost::report_errors());
}
