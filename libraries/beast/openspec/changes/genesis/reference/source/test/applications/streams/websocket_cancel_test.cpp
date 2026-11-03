// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/websocket.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../../../example/applications/streams/websocket-channel.hpp"

int main() {
    namespace asio = boost::asio;
    asio::io_context context;
    webcpp::beast::websocket::stream<supplied_example::channel> stream(context.get_executor());
    webcpp::beast::flat_buffer buffer;
    asio::cancellation_signal cancellation;
    asio::steady_timer timer(context);
    int accepted = 0;
    int cancelled = 0;
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            auto token = asio::as_tuple(asio::use_awaitable);
            auto [error] = co_await stream.async_accept(token);
            BOOST_TEST(!error);
            if (error) {
                co_return;
            }
            ++accepted;
            timer.expires_after(std::chrono::milliseconds(20));
            timer.async_wait([&](boost::system::error_code timer_error) {
                BOOST_TEST(!timer_error);
                cancellation.emit(asio::cancellation_type::terminal);
            });
            auto [read_error, transferred] = co_await stream.async_read(
                buffer, asio::bind_cancellation_slot(cancellation.slot(),
                                                     asio::as_tuple(asio::use_awaitable)));
            BOOST_TEST_EQ(read_error, asio::error::operation_aborted);
            BOOST_TEST_EQ(transferred, 0U);
            ++cancelled;
        },
        [](std::exception_ptr const& error) { BOOST_TEST(!error); });
    context.run();  // lint-run: timer cancellation must wake the pending WebSocket read
    BOOST_TEST_EQ(accepted, 1);
    BOOST_TEST_EQ(cancelled, 1);
    return webcpp::test::exit_status(boost::report_errors());
}
