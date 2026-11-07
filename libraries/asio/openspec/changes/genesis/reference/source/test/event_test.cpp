// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Event waits receive once and release listeners (doc: #asio-events).
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <webcpp/asio/browser/async_wait_event.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/test/exit_status.hpp>
#include "browser_values.hpp"

namespace {
using namespace std::chrono_literals;
using emscripten::val;
using webcpp::asio::browser::async_wait_event;

void one_event_per_wait() {
    webcpp::asio::io_context io;
    const auto target = val::take_ownership(test_event_target());
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
    int called = 0;
    async_wait_event(
        io.get_executor(), target, "message", [&](boost::system::error_code ec, const val& event) {
            BOOST_TEST(!ec);
            BOOST_TEST_EQ(event["detail"].as<int>(), 5);
            BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
            ++called;
            async_wait_event(io.get_executor(), target, "message",
                             [&](boost::system::error_code second_ec, const val& second) {
                                 BOOST_TEST(!second_ec);
                                 BOOST_TEST_EQ(second["detail"].as<int>(), 6);
                                 ++called;
                             });
            test_dispatch_after(target.as_handle(), 20, 6);
        });
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 1);
    BOOST_TEST_EQ(called, 0);
    test_dispatch_after(target.as_handle(), 20, 5);
    io.run();  // lint-run: deliver two distinct events to two waits
    BOOST_TEST_EQ(called, 2);
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
}

void cancellation_removes_the_listener(boost::asio::cancellation_type kind) {
    webcpp::asio::io_context io;
    const auto target = val::take_ownership(test_event_target());
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
    boost::asio::cancellation_signal signal;
    int called = 0;
    async_wait_event(io.get_executor(), target, "message",
                     boost::asio::bind_cancellation_slot(
                         signal.slot(), [&](boost::system::error_code ec, const val& event) {
                             BOOST_TEST(ec == boost::asio::error::operation_aborted);
                             BOOST_TEST(event.isUndefined());
                             ++called;
                         }));
    signal.emit(boost::asio::cancellation_type::none);
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 1);
    signal.emit(kind);
    signal.emit(kind);
    BOOST_TEST_EQ(called, 0);
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
    test_dispatch_after(target.as_handle(), 20, 5);
    webcpp::asio::steady_timer observer(io, 50ms);
    observer.async_wait([](boost::system::error_code) {});
    io.run();  // lint-run: cancellation completes once, before the ignored late event
    BOOST_TEST_EQ(called, 1);
    BOOST_TEST(!signal.slot().has_handler());
}

void a_deferred_type_owns_its_string() {
    webcpp::asio::io_context io;
    const auto target = val::take_ownership(test_event_target());
    std::string text = "messages and more";
    auto pending = async_wait_event(io.get_executor(), target, std::string_view(text.data(), 7),
                                    boost::asio::deferred);
    text.assign(text.size(), 'x');
    int called = 0;
    std::move(pending)([&](boost::system::error_code ec, const val& event) {
        BOOST_TEST(!ec);
        BOOST_TEST_EQ(event["detail"].as<int>(), 5);
        ++called;
    });
    test_dispatch_after(target.as_handle(), 20, 5);
    io.run();  // lint-run: the event type owns its bytes across deferred initiation
    BOOST_TEST_EQ(called, 1);
}

void destruction_removes_the_listener() {
    const auto target = val::take_ownership(test_event_target());
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
    const auto held = std::make_shared<int>(0);
    boost::asio::cancellation_signal signal;
    {
        webcpp::asio::io_context io;
        async_wait_event(
            io.get_executor(), target, "message",
            boost::asio::bind_cancellation_slot(
                signal.slot(), [held](boost::system::error_code, const val&) { ++*held; }));
        BOOST_TEST_EQ(test_listener_count(target.as_handle()), 1);
        BOOST_TEST_GT(held.use_count(), 1);
        BOOST_TEST_EQ(io.run_for(20ms), 0U);  // lint-run: an event wait owns outstanding work
        BOOST_TEST(!io.stopped());
    }
    BOOST_TEST_EQ(held.use_count(), 1);
    BOOST_TEST_EQ(test_listener_count(target.as_handle()), 0);
    BOOST_TEST(!signal.slot().has_handler());
    signal.emit(boost::asio::cancellation_type::total);
    webcpp::asio::io_context observer;
    webcpp::asio::steady_timer timer(observer, 50ms);
    timer.async_wait([](boost::system::error_code) {});
    test_dispatch_after(target.as_handle(), 20, 5);
    observer.run();  // lint-run: a destroyed context receives nothing
    BOOST_TEST_EQ(*held, 0);
}

void associated_executor_receives_success() {
    webcpp::asio::io_context source;
    webcpp::asio::io_context completion;
    int called = 0;
    const auto input = val::take_ownership(test_event_target());
    async_wait_event(
        source.get_executor(), input, "message",
        boost::asio::bind_executor(completion.get_executor(),
                                   [&](boost::system::error_code ec, const val& value) {
                                       BOOST_TEST(!ec);
                                       BOOST_TEST_EQ(value["detail"].as<int>(), 7);
                                       ++called;
                                   }));
    BOOST_TEST_EQ(completion.poll(), 0U);
    BOOST_TEST(!completion.stopped());
    test_dispatch_after(input.as_handle(), 0, 7);
    source.run();      // lint-run: settle the operation and queue its completion
    completion.run();  // lint-run: deliver without restarting the associated context
    BOOST_TEST_EQ(called, 1);
    BOOST_TEST(completion.stopped());
}

void associated_executor_keeps_work() {
    webcpp::asio::io_context completion;
    int called = 0;
    {
        webcpp::asio::io_context source;
        boost::asio::cancellation_signal signal;
        const auto input = val::take_ownership(test_event_target());
        async_wait_event(source.get_executor(), input, "message",
                         boost::asio::bind_executor(
                             completion.get_executor(),
                             boost::asio::bind_cancellation_slot(
                                 signal.slot(), [&](boost::system::error_code ec, const val&) {
                                     BOOST_TEST(ec == boost::asio::error::operation_aborted);
                                     ++called;
                                 })));
        BOOST_TEST_EQ(completion.poll(), 0U);
        BOOST_TEST(!completion.stopped());
        signal.emit(boost::asio::cancellation_type::total);
        source.run();      // lint-run: transfer cancellation to the associated executor
        completion.run();  // lint-run: deliver without restarting the associated context
        BOOST_TEST_EQ(called, 1);
    }
    completion.restart();
    {
        webcpp::asio::io_context source;
        const auto input = val::take_ownership(test_event_target());
        async_wait_event(
            source.get_executor(), input, "message",
            boost::asio::bind_executor(completion.get_executor(),
                                       [&](boost::system::error_code, const val&) { ++called; }));
        BOOST_TEST_EQ(completion.poll(), 0U);
        BOOST_TEST(!completion.stopped());
    }
    BOOST_TEST_EQ(completion.poll(), 0U);
    BOOST_TEST(completion.stopped());
    BOOST_TEST_EQ(called, 1);
}

}  // namespace

int main() {
    associated_executor_keeps_work();
    associated_executor_receives_success();
    one_event_per_wait();
    for (const auto kind : {
             boost::asio::cancellation_type::terminal,
             boost::asio::cancellation_type::partial,
             boost::asio::cancellation_type::total,
         }) {
        cancellation_removes_the_listener(kind);
    }
    a_deferred_type_owns_its_string();
    destruction_removes_the_listener();
    return webcpp::test::exit_status(boost::report_errors());
}
