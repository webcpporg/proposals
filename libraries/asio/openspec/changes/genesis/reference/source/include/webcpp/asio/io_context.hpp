// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_IO_CONTEXT_HPP
#define WEBCPP_ASIO_IO_CONTEXT_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/io_context.hpp>  // lint-world: the program's event loop, asio's purpose
#ifdef __EMSCRIPTEN__
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/detail/browser_imports.hpp>
#include <webcpp/asio/detail/flag_scope.hpp>

#include <boost/asio/steady_timer.hpp>  // lint-world: the unarmed timer that makes the reactor
#include <boost/assert.hpp>
#endif

#include <chrono>
#include <cstddef>
#include <memory>

namespace webcpp::asio {

/** The options of an io_context. */
struct io_context_options {
    /**
     At most this many handlers run in one turn of the run family, which yields to the browser
     after a turn that reaches it.

     Defaults to WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN. 0 is taken as 1.
    */
    std::size_t max_handlers_per_turn = WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN;
};

/**
 Boost.Asio's io_context, whose run family on emscripten suspends into the browser's event loop
 and yields to it after every turn of max_handlers_per_turn handlers.

 Natively it is Boost's, and its options change nothing.

 @see "The run family", in the guide.
*/
class io_context : public boost::asio::io_context {
public:
    /** Constructs an io_context with the default options. */
    io_context() : io_context(io_context_options{}) {}

    /**
     Constructs an io_context with options.

     @param options Its options.
    */
    explicit io_context(io_context_options options) : options_(normal(options)) {}

    /**
     Constructs an io_context that allocates its services and its objects' state with allocator.

     @tparam Allocator A standard allocator.
     @param allocator What the context allocates with.
     @param options Its options.
    */
    template <class Allocator>
    io_context(std::allocator_arg_t /*tag*/, const Allocator& allocator,
               io_context_options options = {})
        : boost::asio::io_context(std::allocator_arg, allocator), options_(normal(options)) {}

    /**
     Constructs an io_context with Boost's hint of the concurrency it will see.

     @param concurrency_hint Boost's hint: 1, or BOOST_ASIO_CONCURRENCY_HINT_1, for one thread.
     @param options Its options.
    */
    explicit io_context(int concurrency_hint, io_context_options options = {})
        : boost::asio::io_context(concurrency_hint), options_(normal(options)) {}

    /**
     Constructs an io_context with an allocator and Boost's hint of its concurrency.

     @tparam Allocator A standard allocator.
     @param allocator What the context allocates with.
     @param concurrency_hint Boost's hint.
     @param options Its options.
    */
    template <class Allocator>
    io_context(std::allocator_arg_t /*tag*/, const Allocator& allocator, int concurrency_hint,
               io_context_options options = {})
        : boost::asio::io_context(std::allocator_arg, allocator, concurrency_hint),
          options_(normal(options)) {}

    /**
     The most handlers one turn runs.

     @return The option's value, at least 1.
    */
    [[nodiscard]] std::size_t max_handlers_per_turn() const noexcept {
        return options_.max_handlers_per_turn;
    }

    /**
     Sets the most handlers one turn runs.

     @param n The new value; 0 is taken as 1.
    */
    void max_handlers_per_turn(std::size_t n) noexcept {
        options_.max_handlers_per_turn = n == 0 ? 1 : n;
    }

#ifdef __EMSCRIPTEN__
    // run and run_one hide Boost's, which are not virtual: run to compose them with the turn
    // limit, and run_one so that the whole run family is this class's; hence the exemption from
    // bugprone-derived-method-shadowing-base-method.
    //
    // Each of the six is called on main's stack alone: one called while another runs, from a
    // handler or from a JavaScript event while run waits, or while the bridge delivers an event,
    // asserts and returns 0 having run nothing. Each admits once, first, and then calls only
    // private code that admits nothing (run_until_unchecked, compose, drain) or Boost's own,
    // never another public member, whose admission would refuse the call its caller flagged.

    /**
     Runs handlers until the io_context is out of work or stopped, yielding to the browser
     after every turn of max_handlers_per_turn handlers.

     @return The handlers run.
    */
    count_type run() {  // NOLINT(bugprone-derived-method-shadowing-base-method)
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        return compose(
            [this] {
                return boost::asio::io_context::run_one();  // lint-run: step 4, the one wait
            },
            [] { return false; });
    }

    /**
     Runs one handler, waiting in the browser's event loop for one to be ready.

     @return The handlers run: 1, or 0 when stopped or out of work.
    */
    count_type run_one() {  // NOLINT(bugprone-derived-method-shadowing-base-method)
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        return boost::asio::io_context::run_one();  // lint-run: step 4 alone
    }

    /**
     Runs handlers as run does, until abs_time at most.

     @tparam Clock The deadline's clock.
     @tparam Duration The deadline's duration.
     @param abs_time The deadline.
     @return The handlers run.
    */
    template <class Clock, class Duration>
    std::size_t run_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        return run_until_unchecked(abs_time);
    }

    /**
     Runs handlers as run does, for rel_time at most.

     @tparam Rep The duration's representation.
     @tparam Period The duration's period.
     @param rel_time How long.
     @return The handlers run.
    */
    template <class Rep, class Period>
    std::size_t run_for(const std::chrono::duration<Rep, Period>& rel_time) {
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        using clock = std::chrono::steady_clock;  // lint-world: the deadline's clock
        return run_until_unchecked(clock::now() + rel_time);
    }

    /**
     Runs one handler, waiting until abs_time at most.

     @tparam Clock The deadline's clock.
     @tparam Duration The deadline's duration.
     @param abs_time The deadline.
     @return The handlers run: 1 or 0.
    */
    template <class Clock, class Duration>
    std::size_t run_one_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        return boost::asio::io_context::run_one_until(abs_time);
    }

    /**
     Runs one handler, waiting for rel_time at most.

     @tparam Rep The duration's representation.
     @tparam Period The duration's period.
     @param rel_time How long.
     @return The handlers run: 1 or 0.
    */
    template <class Rep, class Period>
    std::size_t run_one_for(const std::chrono::duration<Rep, Period>& rel_time) {
        if (!admitted()) {
            return 0;
        }
        const detail::flag_scope running(running_);
        return boost::asio::io_context::run_one_for(rel_time);
    }

private:
    /**
     Whether a run family call may start: refused while another runs or the bridge delivers.

     Tip: not noexcept, since whether the bridge delivers is read from JavaScript.
    */
    [[nodiscard]] bool admitted() const {
        const bool allowed = !running_ && !bridge_.delivering();
        BOOST_ASSERT_MSG(allowed,
                         "webcpp::asio: the run family is called on main's stack alone, "
                         "never from a handler or a browser event");
        return allowed;
    }

    /** What run_until and run_for run: the composition until abs_time. */
    template <class Clock, class Duration>
    std::size_t run_until_unchecked(const std::chrono::time_point<Clock, Duration>& abs_time) {
        return compose(
            [this, &abs_time] { return boost::asio::io_context::run_one_until(abs_time); },
            [&abs_time] { return Clock::now() >= abs_time; });
    }

    /**
     Runs the ready handlers, max_handlers_per_turn at most, without waiting, and none once
     expired.

     Tip: the deadline is checked before each handler, as Boost's run_until does, so that a
     deadline already passed runs nothing.
    */
    template <class Expired>
    std::size_t drain(const Expired& expired) {
        std::size_t ran = 0;
        while (ran < options_.max_handlers_per_turn && !expired() && poll_one() == 1) {
            ++ran;
        }
        return ran;
    }

    /**
     The run family's four steps: a turn of ready handlers; return when stopped or expired;
     yield through a macrotask when the turn reached the limit; else wait for one handler.
    */
    template <class Wait, class Expired>
    count_type compose(Wait wait, Expired expired) {
        count_type total = 0;
        for (;;) {
            const std::size_t ran = drain(expired);
            total += ran;
            if (stopped() || expired()) {
                return total;
            }
            if (ran == options_.max_handlers_per_turn) {
                webcpp_asio_yield();
                continue;
            }
            const std::size_t one = wait();
            if (one == 0) {
                return total;
            }
            total += one;
        }
    }

public:
#else
    /** Boost's own, natively. */
    using boost::asio::io_context::run;  // lint-run: Boost's own, natively
    /** Boost's own, natively. */
    using boost::asio::io_context::run_for;  // lint-run: Boost's own, natively
    /** Boost's own, natively. */
    using boost::asio::io_context::run_one;  // lint-run: Boost's own, natively
    /** Boost's own, natively. */
    using boost::asio::io_context::run_one_for;
    /** Boost's own, natively. */
    using boost::asio::io_context::run_one_until;
    /** Boost's own, natively. */
    using boost::asio::io_context::run_until;
#endif

private:
    static io_context_options normal(io_context_options options) noexcept {
        if (options.max_handlers_per_turn == 0) {
            options.max_handlers_per_turn = 1;
        }
        return options;
    }

    io_context_options options_;
#ifdef __EMSCRIPTEN__
    // Never armed: its construction makes the reactor, which the scheduler then waits on, so a
    // run with work and no timer of the program's own suspends in select, which a post wakes,
    // rather than in ::pause(), which nothing wakes (measured with Emscripten 6.0.11;
    // test/reactor_test.cpp fails without it).
    boost::asio::steady_timer reactor_{*this};  // lint-world: never armed
    // The bridge whose deliveries the run family refuses to run inside, looked up once.
    browser::bridge& bridge_ = boost::asio::use_service<browser::bridge>(*this);
    // Whether a call of the run family is active on this context.
    bool running_ = false;
#endif
};

}  // namespace webcpp::asio

#endif  // WEBCPP_ASIO_IO_CONTEXT_HPP
