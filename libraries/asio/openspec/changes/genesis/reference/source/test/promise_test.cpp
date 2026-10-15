// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Promise values, cancellation and lifetime (doc: #asio-promises).
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/test/exit_status.hpp>
#include "browser_values.hpp"
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif
#include <chrono>
#include <memory>
#include <string_view>
#include <utility>

namespace {
using namespace std::chrono_literals;
using webcpp::asio::browser::async_await;
namespace error = webcpp::asio::browser::error;
using emscripten::val;

void resolved_and_rejected_promises() {
    webcpp::asio::io_context io;
    int called = 0;
    async_await(io.get_executor(), val::take_ownership(test_promise_after(20, 42, false)),
                [&](boost::system::error_code ec, const val& value) {
                    BOOST_TEST(!ec);
                    BOOST_TEST_EQ(value.as<int>(), 42);
                    ++called;
                });
    async_await(io.get_executor(), val::take_ownership(test_promise_after(20, 9, true)),
                [&](boost::system::error_code ec, const val& value) {
                    BOOST_TEST(ec == error::promise_rejected);
                    BOOST_TEST_EQ(std::string_view(ec.category().name()), "webcpp.asio");
                    BOOST_TEST_EQ(ec.value(), 1);
                    BOOST_TEST_EQ(ec.message(), "the promise was rejected");
                    BOOST_TEST_EQ(value.as<int>(), 9);
                    ++called;
                });
    io.run();  // lint-run: the test's event loop
    BOOST_TEST_EQ(called, 2);
    BOOST_TEST_EQ(error::category().message(99), "unknown webcpp.asio error");
}

void completion_tokens_work() {
    webcpp::asio::io_context io;
    int called = 0;
    auto pending = async_await(io.get_executor(), val(7), boost::asio::deferred);
    std::move(pending)([&](boost::system::error_code ec, const val& value) {
        BOOST_TEST(!ec);
        BOOST_TEST_EQ(value.as<int>(), 7);
        ++called;
    });
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    boost::asio::co_spawn(
        io,
        [&]() -> boost::asio::awaitable<void> {
            const auto value =
                co_await async_await(io.get_executor(), val(8), boost::asio::use_awaitable);
            BOOST_TEST_EQ(value.as<int>(), 8);
        },
        [&](const std::exception_ptr& exception) {
            BOOST_TEST(!exception);
            ++called;
        });
#endif
    io.run();  // lint-run: the test's event loop
#ifdef WEBCPP_ASIO_NO_EXCEPTIONS
    BOOST_TEST_EQ(called, 1);
#else
    BOOST_TEST_EQ(called, 2);
#endif
}

void cancellation_completes_before_settlement(boost::asio::cancellation_type kind) {
    webcpp::asio::io_context io;
    boost::asio::cancellation_signal signal;
    int called = 0;
    async_await(io.get_executor(), val::take_ownership(test_promise_after(1000, 42, false)),
                boost::asio::bind_cancellation_slot(
                    signal.slot(), [&](boost::system::error_code ec, const val& value) {
                        BOOST_TEST(ec == boost::asio::error::operation_aborted);
                        BOOST_TEST(value.isUndefined());
                        ++called;
                    }));
    webcpp::asio::steady_timer timer(io, 20ms);
    timer.async_wait([&](boost::system::error_code ec) {
        BOOST_TEST(!ec);
        signal.emit(kind);
        signal.emit(kind);
    });
    const auto before = std::chrono::steady_clock::now();
    io.run();  // lint-run: the test's event loop
    BOOST_TEST(std::chrono::steady_clock::now() - before < 500ms);
    BOOST_TEST_EQ(called, 1);
    BOOST_TEST(!signal.slot().has_handler());
    // Keep the context alive through the late settlement, which must not complete again.
    io.restart();
    timer.expires_after(1100ms);
    timer.async_wait([](boost::system::error_code) {});
    io.run();  // lint-run: observe the discarded late settlement
    BOOST_TEST_EQ(called, 1);
}

void never_settling_promises_keep_work() {
    webcpp::asio::io_context io;
    boost::asio::cancellation_signal signal;
    int called = 0;
    async_await(io.get_executor(), val::take_ownership(test_never_settles()),
                boost::asio::bind_cancellation_slot(
                    signal.slot(), [&](boost::system::error_code ec, const val&) {
                        BOOST_TEST(ec == boost::asio::error::operation_aborted);
                        ++called;
                    }));
    signal.emit(boost::asio::cancellation_type::none);
    BOOST_TEST_EQ(io.run_for(200ms), 0U);  // lint-run: a pending promise owns work
    BOOST_TEST(!io.stopped());
    BOOST_TEST_EQ(called, 0);
    signal.emit(boost::asio::cancellation_type::total);
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: drain the posted cancellation
    BOOST_TEST_EQ(called, 1);
}

void destruction_releases_the_handler() {
    const auto held = std::make_shared<int>(0);
    boost::asio::cancellation_signal signal;
    {
        webcpp::asio::io_context io;
        async_await(io.get_executor(), val::take_ownership(test_never_settles()),
                    boost::asio::bind_cancellation_slot(
                        signal.slot(), [held](boost::system::error_code, const val&) { ++*held; }));
        BOOST_TEST_GT(held.use_count(), 1);
    }
    BOOST_TEST_EQ(held.use_count(), 1);
    BOOST_TEST_EQ(*held, 0);
    BOOST_TEST(!signal.slot().has_handler());
    signal.emit(boost::asio::cancellation_type::total);
}

void settlement_after_destruction_finds_nothing() {
    int called = 0;
    {
        webcpp::asio::io_context io;
        async_await(io.get_executor(), val::take_ownership(test_promise_after(30, 1, false)),
                    [&](boost::system::error_code, const val&) { ++called; });
    }
    webcpp::asio::io_context observer;
    webcpp::asio::steady_timer timer(observer, 100ms);
    timer.async_wait([](boost::system::error_code) {});
    observer.run();  // lint-run: let the abandoned promise settle
    BOOST_TEST_EQ(called, 0);
}

void boundary_values_are_posted() {
    webcpp::asio::io_context io;
    bool initiated = false;
    int called = 0;
    auto promise = val::global("Promise").call<val>("resolve", 5);
    async_await(io.get_executor(), promise, [&](boost::system::error_code ec, const val& value) {
        BOOST_TEST(initiated);
        BOOST_TEST(!ec);
        BOOST_TEST_EQ(value.as<int>(), 5);
        ++called;
    });
    async_await(io.get_executor(), val(6), [&](boost::system::error_code ec, const val& value) {
        BOOST_TEST(initiated);
        BOOST_TEST(!ec);
        BOOST_TEST_EQ(value.as<int>(), 6);
        ++called;
    });
    auto rejected = val::global("Promise").call<val>("reject", val::undefined());
    async_await(io.get_executor(), rejected, [&](boost::system::error_code ec, const val& reason) {
        BOOST_TEST(initiated);
        BOOST_TEST(ec == error::promise_rejected);
        BOOST_TEST(reason.isUndefined());
        ++called;
    });
    BOOST_TEST_EQ(called, 0);
    initiated = true;
    io.run();  // lint-run: the test's event loop
    BOOST_TEST_EQ(called, 3);
}

void associated_executor_receives_success() {
    webcpp::asio::io_context source;
    webcpp::asio::io_context completion;
    int called = 0;
    const auto input = val(7);
    async_await(source.get_executor(), input,
                boost::asio::bind_executor(completion.get_executor(),
                                           [&](boost::system::error_code ec, const val& value) {
                                               BOOST_TEST(!ec);
                                               BOOST_TEST_EQ(value.as<int>(), 7);
                                               ++called;
                                           }));
    BOOST_TEST_EQ(completion.poll(), 0U);
    BOOST_TEST(!completion.stopped());

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
        const auto input = val::take_ownership(test_never_settles());
        async_await(source.get_executor(), input,
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
        const auto input = val::take_ownership(test_never_settles());
        async_await(
            source.get_executor(), input,
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
    resolved_and_rejected_promises();
    completion_tokens_work();
    for (const auto kind : {
             boost::asio::cancellation_type::terminal,
             boost::asio::cancellation_type::partial,
             boost::asio::cancellation_type::total,
         }) {
        cancellation_completes_before_settlement(kind);
    }
    never_settling_promises_keep_work();
    destruction_releases_the_handler();
    settlement_after_destruction_finds_nothing();
    boundary_values_are_posted();
    return webcpp::test::exit_status(boost::report_errors());
}
