// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Asyncify's stack (doc: #asio-building-asyncify-stack): every suspension of the run family, the
// reactor's wait and the yield, under each member that makes one, is measured as the bytes its
// unwind wrote into the stack Asyncify saves it in, and leaves at least half of that stack to
// the program's own frames above the call. It is read from Asyncify's own data when the unwind
// stops, never inferred from whether a smaller stack would trap. Reactor cases must reach a
// blocking poll, not just suspend in the initial nonblocking poll. Each path prints its bytes.

#include <webcpp/asio/io_context.hpp>

#include "asyncify_watch.hpp"

#include <webcpp/test/exit_status.hpp>

#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <emscripten/eventloop.h>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

using namespace std::chrono_literals;
using webcpp::test::measure;

/** A context held by work that only a JavaScript macrotask can release, after run waits. */
struct reactor_wait {
    webcpp::asio::io_context io;
    boost::asio::executor_work_guard<webcpp::asio::io_context::executor_type> guard{
        io.get_executor()};
    bool fired = false;
    int timeout;

    reactor_wait()
        : timeout(emscripten_set_timeout(
              [](void* address) {
                  auto* state = static_cast<reactor_wait*>(address);
                  boost::asio::post(state->io, [state] {
                      state->fired = true;
                      state->guard.reset();
                  });
              },
              0, this)) {
        // Delay entry past the old timer's expiry to pin the measurement-path regression.
        EM_ASM({
            const until = performance.now() + 25;
            while (performance.now() < until) {
            }
        });
    }

    // Cancel a pending callback if a timed run returned before the macrotask could execute.
    ~reactor_wait() { emscripten_clear_timeout(timeout); }

    reactor_wait(const reactor_wait&) = delete;
    reactor_wait& operator=(const reactor_wait&) = delete;
    reactor_wait(reactor_wait&&) = delete;
    reactor_wait& operator=(reactor_wait&&) = delete;
};

/** A context with two handlers ready and a limit of one, so that the run family yields once. */
struct yield_once {
    webcpp::asio::io_context io{webcpp::asio::io_context_options{.max_handlers_per_turn = 1}};
    std::size_t ran = 0;

    yield_once() {
        boost::asio::post(io, [this] { ++ran; });
        boost::asio::post(io, [this] { ++ran; });
    }
};

void every_suspension_leaves_half_the_stack() {
    measure("run, the reactor's wait", [] {
        reactor_wait state;
        state.io.run();  // lint-run: the path measured
        BOOST_TEST(state.fired);
    });
    measure(
        "run, the yield",
        [] {
            yield_once state;
            state.io.run();  // lint-run: the path measured
            BOOST_TEST_EQ(state.ran, 2U);
        },
        false);
    measure("run_one, the reactor's wait", [] {
        reactor_wait state;
        state.io.run_one();  // lint-run: the path measured
        BOOST_TEST(state.fired);
    });
    measure("run_for, the reactor's wait", [] {
        reactor_wait state;
        state.io.run_for(10s);  // lint-run: the path measured
        BOOST_TEST(state.fired);
    });
    measure(
        "run_for, the yield",
        [] {
            yield_once state;
            state.io.run_for(10s);  // lint-run: the path measured
            BOOST_TEST_EQ(state.ran, 2U);
        },
        false);
    measure("run_until, the reactor's wait", [] {
        reactor_wait state;
        state.io.run_until(std::chrono::steady_clock::now() + 10s);
        BOOST_TEST(state.fired);
    });
    measure(
        "run_until, the yield",
        [] {
            yield_once state;
            state.io.run_until(std::chrono::steady_clock::now() + 10s);
            BOOST_TEST_EQ(state.ran, 2U);
        },
        false);
    measure("run_one_for, the reactor's wait", [] {
        reactor_wait state;
        state.io.run_one_for(10s);
        BOOST_TEST(state.fired);
    });
    measure("run_one_until, the reactor's wait", [] {
        reactor_wait state;
        state.io.run_one_until(std::chrono::steady_clock::now() + 10s);
        BOOST_TEST(state.fired);
    });
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    // JavaScript-based exceptions call what a try encloses through an import of their own.
    measure("run inside a try, the reactor's wait", [] {
        reactor_wait state;
        try {
            state.io.run();  // lint-run: the path measured
        } catch (const std::runtime_error&) {
            BOOST_ERROR("nothing throws");
        }
        BOOST_TEST(state.fired);
    });
    measure("run inside a catch block, the reactor's wait", [] {
        reactor_wait state;
        try {
            throw std::runtime_error("before run");
        } catch (const std::runtime_error&) {
            state.io.run();  // lint-run: the path measured
        }
        BOOST_TEST(state.fired);
    });
#endif
}

}  // namespace

int main() {
    test_watch_unwinds();
    every_suspension_leaves_half_the_stack();
    return webcpp::test::exit_status(boost::report_errors());
}
