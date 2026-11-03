// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <array>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <memory>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/asio/wasi/stdio_stream.hpp>
#include <webcpp/test/exit_status.hpp>
#include "counting_allocator.hpp"

int main() {
    namespace asio = boost::asio;
    using stream_type = webcpp::asio::wasi::stdio_stream;
    webcpp::asio::io_context context;
    stream_type stream(context.get_executor());
    std::array<char, 16> data{};
    counts counter;
    int completed = 0;
    webcpp::asio::steady_timer timer(context, std::chrono::milliseconds(5));
    timer.async_wait([&](auto error) {
        BOOST_TEST(!error);
        stream.cancel();
    });
    stream.async_read_some(
        asio::buffer(data),
        asio::bind_allocator(allocator<void>(counter),
                             [&](boost::system::error_code error, std::size_t count) {
                                 BOOST_TEST_EQ(error, asio::error::operation_aborted);
                                 BOOST_TEST_EQ(count, 0U);
                                 BOOST_TEST(counter.allocations > 0);
                                 BOOST_TEST_EQ(counter.live, 0U);
                                 ++completed;
                             }));
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: timer must progress while the host keeps input open and empty
    BOOST_TEST_EQ(completed, 1);

    context.restart();
    asio::cancellation_signal signal;
    stream.async_read_some(
        asio::buffer(data),
        asio::bind_cancellation_slot(signal.slot(),
                                     [&](boost::system::error_code error, std::size_t count) {
                                         BOOST_TEST_EQ(error, asio::error::operation_aborted);
                                         BOOST_TEST_EQ(count, 0U);
                                         ++completed;
                                     }));
    signal.emit(asio::cancellation_type::all);
    context.run();  // lint-run: cancellation before initial posted work
    BOOST_TEST_EQ(completed, 2);

    context.restart();
    {
        stream_type temporary(context.get_executor());
        temporary.async_read_some(asio::buffer(data),
                                  [&](boost::system::error_code error, std::size_t count) {
                                      BOOST_TEST_EQ(error, asio::error::operation_aborted);
                                      BOOST_TEST_EQ(count, 0U);
                                      ++completed;
                                  });
        BOOST_TEST_EQ(context.poll_one(), 1U);
    }
    context.run();  // lint-run: destroying the stream cancels its outstanding read safely
    BOOST_TEST_EQ(completed, 3);

    context.restart();
    webcpp::asio::io_context target;
    stream.async_read_some(
        asio::mutable_buffer{},
        asio::bind_executor(target.get_executor(),
                            [&](boost::system::error_code error, std::size_t count) {
                                BOOST_TEST(!error);
                                BOOST_TEST_EQ(count, 0U);
                                ++completed;
                            }));
    context.poll();  // Both contexts are single-threaded: poll one, then drive the other.
    BOOST_TEST_EQ(completed, 3);
    target.run();  // lint-run: deliver the zero-length read without consuming input
    BOOST_TEST_EQ(completed, 4);
    context.restart();
    auto deferred_read = stream.async_read_some(asio::buffer(data), asio::deferred);
    stream.cancel();  // No read was initiated yet; this must not cancel the deferred operation.
    bool cancelled_by_timer = false;
    timer.expires_after(std::chrono::milliseconds(5));
    timer.async_wait([&](auto error) {
        BOOST_TEST(!error);
        cancelled_by_timer = true;
        stream.cancel();
    });
    std::move(deferred_read)([&](boost::system::error_code error, std::size_t count) {
        BOOST_TEST(cancelled_by_timer);
        BOOST_TEST_EQ(error, asio::error::operation_aborted);
        BOOST_TEST_EQ(count, 0U);
        ++completed;
    });
    context.run();  // lint-run: cancellation is scoped to initiated operations
    BOOST_TEST_EQ(completed, 5);
    {
        webcpp::asio::io_context abandoned;
        stream_type pending(abandoned.get_executor());
        pending.async_read_some(asio::buffer(data),
                                [&](auto, auto) { BOOST_ERROR("abandoned callback"); });
        BOOST_TEST_EQ(abandoned.poll_one(), 1U);
    }
    return webcpp::test::exit_status(boost::report_errors());
}
