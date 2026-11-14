// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// What an event becomes while no wait is pending (design.md, D5), without a platform: states
// coalesce, events queue up to a bound with a place kept for the end of gathering, one wait of a
// kind at a time, a cancelled wait consumes nothing, a satisfied wait is posted, and an
// operation's storage is its handler's (spec: "Events: ...", "Semantics: handlers run on the
// executor, never inside the call").

#include <webcpp/webrtc/detail/events.hpp>
#include <webcpp/webrtc/detail/operation.hpp>
#include <webcpp/webrtc/types.hpp>

#include <webcpp/asio/bind_allocator.hpp>          // lint-world: a handler's own allocator
#include <webcpp/asio/bind_cancellation_slot.hpp>  // lint-world: a handler's cancellation
#include <webcpp/asio/cancellation_signal.hpp>     // lint-world: a handler's cancellation
#include <webcpp/asio/io_context.hpp>              // lint-world: the tests' event loop

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace webrtc = webcpp::webrtc;
namespace asio = webcpp::asio;
using webrtc::detail::error_code;

namespace {

using state = webrtc::peer_connection_state;

// Starts a wait on `watch` whose result lands in `out`.
template <class Handler>
webrtc::detail::pending<state>& wait_op(asio::io_context& io, Handler handler) {
    return webrtc::detail::make_operation(std::move(handler), io.get_executor(),
                                          std::tuple<state>{state::new_},
                                          webrtc::detail::wait_cancellation);
}

void states_coalesce() {
    asio::io_context io;
    webrtc::detail::state_watch<state> watch;
    watch.update(state::connecting);
    watch.update(state::connected);
    std::optional<state> got;
    watch.wait(wait_op(io, [&](error_code ec, state s) {
        BOOST_TEST(!ec);
        got = s;
    }));
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(got == state::connected);
}

void no_change_keeps_waiting() {
    asio::io_context io;
    webrtc::detail::state_watch<state> watch;
    watch.update(state::connected);
    int completions = 0;
    watch.wait(wait_op(io, [&](error_code, state) { ++completions; }));
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST_EQ(completions, 1);
    std::optional<state> next;
    watch.wait(wait_op(io, [&](error_code ec, state s) {
        BOOST_TEST(!ec);
        next = s;
    }));
    io.restart();
    io.poll();
    BOOST_TEST(!next);
    watch.update(state::connected);
    io.poll();
    BOOST_TEST(!next);
    watch.update(state::disconnected);
    io.restart();
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(next == state::disconnected);
}

void one_wait_of_a_kind() {
    asio::io_context io;
    webrtc::detail::state_watch<state> watch;
    std::optional<error_code> first;
    std::optional<error_code> second;
    watch.wait(wait_op(io, [&](error_code ec, state) { first = ec; }));
    watch.wait(wait_op(io, [&](error_code ec, state) { second = ec; }));
    io.poll();
    BOOST_TEST(second && *second == asio::error::already_started);
    BOOST_TEST(!first);
    watch.update(state::connecting);
    io.restart();
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(first && !*first);
}

void a_cancelled_wait_consumes_nothing() {
    asio::io_context io;
    using candidate = webrtc::ice_candidate;
    webrtc::detail::event_queue<candidate, std::allocator<std::byte>> queue(4, {});
    asio::cancellation_signal signal;
    std::optional<error_code> cancelled;
    queue.wait(webrtc::detail::make_operation(
        asio::bind_cancellation_slot(signal.slot(),
                                     [&](error_code ec, const candidate&) { cancelled = ec; }),
        io.get_executor(), std::tuple<candidate>{}, webrtc::detail::wait_cancellation));
    signal.emit(asio::cancellation_type::terminal);
    BOOST_TEST(!queue.push(candidate{"candidate:1", "0", 0, ""}));
    std::optional<candidate> next;
    queue.wait(webrtc::detail::make_operation(
        [&](error_code ec, candidate c) {
            BOOST_TEST(!ec);
            next = std::move(c);
        },
        io.get_executor(), std::tuple<candidate>{}, webrtc::detail::wait_cancellation));
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(cancelled && *cancelled == asio::error::operation_aborted);
    BOOST_TEST(next && next->candidate == "candidate:1");
}

void the_bound_keeps_a_place_for_the_end() {
    asio::io_context io;
    using candidate = webrtc::ice_candidate;
    webrtc::detail::event_queue<candidate, std::allocator<std::byte>> queue(2, {});
    BOOST_TEST(!queue.push(candidate{"candidate:1", "0", 0, ""}));
    BOOST_TEST(!queue.push(candidate{"candidate:2", "0", 0, ""}));
    BOOST_TEST(queue.push(candidate{"candidate:3", "0", 0, ""}).has_value());
    BOOST_TEST(!queue.push(candidate{}, true));
    BOOST_TEST_EQ(queue.size(), 3u);
    std::vector<std::string> got;
    std::function<void()> next;
    next = [&] {
        queue.wait(webrtc::detail::make_operation(
            [&](error_code ec, candidate c) {
                if (ec) {
                    return;
                }
                got.emplace_back(c.candidate);
                if (!c.candidate.empty()) {
                    next();
                }
            },
            io.get_executor(), std::tuple<candidate>{}, webrtc::detail::wait_cancellation));
    };
    next();
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST((got == std::vector<std::string>{"candidate:1", "candidate:2", ""}));
}

void a_satisfied_wait_is_posted() {
    asio::io_context io;
    webrtc::detail::state_watch<state> watch;
    watch.update(state::connected);
    bool ran = false;
    watch.wait(wait_op(io, [&](error_code, state) { ran = true; }));
    BOOST_TEST(!ran);
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(ran);
}

void close_aborts_and_then_refuses() {
    asio::io_context io;
    webrtc::detail::state_watch<state> watch;
    std::optional<error_code> pending;
    watch.wait(wait_op(io, [&](error_code ec, state) { pending = ec; }));
    watch.close(state::closed, asio::error::operation_aborted);
    watch.update(state::failed);
    std::optional<error_code> later;
    watch.wait(wait_op(io, [&](error_code ec, state) { later = ec; }));
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(pending && *pending == asio::error::operation_aborted);
    BOOST_TEST(later && *later == webrtc::error::closed);
    BOOST_TEST(watch.current() == state::closed);
}

// An allocator that counts what it gives, as a handler's associated allocator.
template <class T>
struct counting_allocator {
    using value_type = T;
    std::size_t* count;

    explicit counting_allocator(std::size_t* c) : count(c) {}

    template <class U>
    explicit counting_allocator(const counting_allocator<U>& other) : count(other.count) {}

    T* allocate(std::size_t n) {
        ++*count;
        return std::allocator<T>().allocate(n);
    }

    void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }

    template <class U>
    bool operator==(const counting_allocator<U>& other) const {
        return count == other.count;
    }
};

void an_operation_is_in_its_handlers_allocator() {
    asio::io_context io;
    std::size_t count = 0;
    webrtc::detail::state_watch<state> watch;
    bool ran = false;
    watch.wait(webrtc::detail::make_operation(
        asio::bind_allocator(counting_allocator<void>(&count),
                             [&](error_code, state) { ran = true; }),
        io.get_executor(), std::tuple<state>{state::new_}, webrtc::detail::wait_cancellation));
    BOOST_TEST_GE(count, 1u);
    watch.update(state::connecting);
    io.run();  // lint-run: drains the posted completions of this test
    BOOST_TEST(ran);
}

}  // namespace

int main() {
    states_coalesce();
    no_change_keeps_waiting();
    one_wait_of_a_kind();
    a_cancelled_wait_consumes_nothing();
    the_bound_keeps_a_place_for_the_end();
    a_satisfied_wait_is_posted();
    close_aborts_and_then_refuses();
    an_operation_is_in_its_handlers_allocator();
    return boost::report_errors();
}
