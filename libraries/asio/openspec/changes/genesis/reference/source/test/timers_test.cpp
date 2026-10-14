// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Boost's timers and Beast's timeouts keep their completion and cancellation semantics
// through webcpp's run family on both targets (doc: #asio-invariant-8). A lost timer,
// registration-order scheduling, dropped cancellation or a stalled Beast timeout fails here.

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/asio/system_timer.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/lightweight_test.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {

using namespace std::chrono_literals;

void steady_timers_complete_in_deadline_order() {
    std::cout << "case: steady timers complete in deadline order\n";
    webcpp::asio::io_context io;
    const auto start = std::chrono::steady_clock::now();
    webcpp::asio::steady_timer sixty(io, start + 60ms);
    webcpp::asio::steady_timer twenty(io, start + 20ms);
    webcpp::asio::steady_timer forty(io, start + 40ms);
    std::vector<std::chrono::milliseconds::rep> order;
    const auto record = [&order](auto delay) {
        return [&order, delay](boost::system::error_code ec) {
            BOOST_TEST(!ec);
            order.push_back(delay.count());
            std::cout << "completed steady timer: " << delay.count() << " ms\n";
        };
    };
    sixty.async_wait(record(60ms));
    twenty.async_wait(record(20ms));
    forty.async_wait(record(40ms));
    BOOST_TEST_EQ(io.run(), 3U);  // lint-run: the test's event loop
    const std::array<std::chrono::milliseconds::rep, 3> expected{20, 40, 60};
    BOOST_TEST_ALL_EQ(order.begin(), order.end(), expected.begin(), expected.end());
}

void a_system_timer_completes() {
    std::cout << "case: a system timer completes\n";
    webcpp::asio::io_context io;
    webcpp::asio::system_timer timer(io, 30ms);
    std::size_t completions = 0;
    timer.async_wait([&completions](boost::system::error_code ec) {
        ++completions;
        BOOST_TEST(!ec);
        std::cout << "system timer error code: " << ec.value() << "\n";
    });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(completions, 1U);
}

void cancelling_a_timer_aborts_its_wait() {
    std::cout << "case: cancelling a timer aborts its wait\n";
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 10s);
    std::size_t completions = 0;
    timer.async_wait([&completions](boost::system::error_code ec) {
        ++completions;
        BOOST_TEST(ec == boost::asio::error::operation_aborted);
        std::cout << "cancelled timer: " << ec.message() << "\n";
    });
    BOOST_TEST_EQ(timer.cancel(), 1U);
    BOOST_TEST_EQ(completions, 0U);
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(completions, 1U);
}

// tag::slot-cancellation[]
void a_cancellation_slot_aborts_only_its_wait() {
    std::cout << "case: a cancellation slot aborts only its wait\n";
    webcpp::asio::io_context io;
    boost::asio::cancellation_signal signal;
    const auto start = std::chrono::steady_clock::now();
    webcpp::asio::steady_timer timer(io, start + 10s);
    webcpp::asio::steady_timer trigger(io, start + 10ms);
    std::size_t cancelled = 0;
    std::size_t other = 0;
    std::size_t emitted = 0;
    timer.async_wait(
        boost::asio::bind_cancellation_slot(signal.slot(), [&](boost::system::error_code ec) {
            ++cancelled;
            BOOST_TEST(ec == boost::asio::error::operation_aborted);
            BOOST_TEST_EQ(emitted, 1U);
            BOOST_TEST_EQ(other, 0U);
            // The unbound wait is still pending. Cancel it explicitly to finish the case.
            BOOST_TEST_EQ(timer.cancel(), 1U);
            std::cout << "slot cancellation: " << ec.message() << "\n";
        }));
    timer.async_wait([&](boost::system::error_code ec) {
        ++other;
        BOOST_TEST(ec == boost::asio::error::operation_aborted);
        BOOST_TEST_EQ(cancelled, 1U);
    });
    trigger.async_wait([&](boost::system::error_code ec) {
        BOOST_TEST(!ec);
        ++emitted;
        signal.emit(boost::asio::cancellation_type::terminal);
    });
    io.run();  // lint-run: the test's event loop
    BOOST_TEST_EQ(emitted, 1U);
    BOOST_TEST_EQ(cancelled, 1U);
    BOOST_TEST_EQ(other, 1U);
}

// end::slot-cancellation[]

// tag::beast-timeout[]
void beast_s_handshake_timeout_fires() {
    std::cout << "case: Beast's handshake timeout fires\n";
    webcpp::asio::io_context io;
    boost::beast::test::stream peer(io);
    boost::beast::websocket::stream<boost::beast::test::stream> ws(io);
    boost::beast::websocket::stream_base::timeout timeout{};
    timeout.handshake_timeout = 100ms;
    timeout.idle_timeout = boost::beast::websocket::stream_base::none();
    timeout.keep_alive_pings = false;
    ws.set_option(timeout);
    ws.next_layer().connect(peer);
    std::size_t completions = 0;
    ws.async_accept([&completions](boost::system::error_code ec) {
        ++completions;
        BOOST_TEST(ec == boost::beast::error::timeout);
        std::cout << "handshake: " << ec.message() << "\n";
    });
    io.run();  // lint-run: the test's event loop
    BOOST_TEST_EQ(completions, 1U);
}

// end::beast-timeout[]

}  // namespace

int main() {
    steady_timers_complete_in_deadline_order();
    a_system_timer_completes();
    cancelling_a_timer_aborts_its_wait();
    a_cancellation_slot_aborts_only_its_wait();
    beast_s_handshake_timeout_fires();
    return webcpp::test::exit_status(boost::report_errors());
}
