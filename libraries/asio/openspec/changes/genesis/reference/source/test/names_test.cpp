// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The names of asio are Boost.Asio's: its io_context derives from Boost's and shares its
// executor, its timers are Boost's own (doc: #asio-names). The options are accepted on
// every target.

#include <webcpp/asio.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <type_traits>

// The build supplies an independent expectation for configuration regressions.
#ifndef WEBCPP_TEST_TURN_LIMIT
#define WEBCPP_TEST_TURN_LIMIT 100
#endif

static_assert(std::is_base_of_v<boost::asio::io_context, webcpp::asio::io_context>);
static_assert(std::is_convertible_v<webcpp::asio::io_context&, boost::asio::io_context&>);
static_assert(std::is_same_v<webcpp::asio::io_context::executor_type,
                             boost::asio::io_context::executor_type>);
static_assert(std::is_same_v<webcpp::asio::steady_timer, boost::asio::steady_timer>);
static_assert(std::is_same_v<webcpp::asio::system_timer, boost::asio::system_timer>);
static_assert(std::is_same_v<webcpp::asio::basic_waitable_timer<std::chrono::steady_clock>,
                             boost::asio::basic_waitable_timer<std::chrono::steady_clock>>);

#ifndef __EMSCRIPTEN__
static_assert(std::is_same_v<webcpp::asio::basic_stream_socket<webcpp::asio::ip::tcp>,
                             boost::asio::basic_stream_socket<boost::asio::ip::tcp>>);
static_assert(std::is_same_v<webcpp::asio::basic_datagram_socket<webcpp::asio::ip::udp>,
                             boost::asio::basic_datagram_socket<boost::asio::ip::udp>>);
static_assert(std::is_same_v<webcpp::asio::basic_socket_acceptor<webcpp::asio::ip::tcp>,
                             boost::asio::basic_socket_acceptor<boost::asio::ip::tcp>>);
static_assert(std::is_same_v<webcpp::asio::ip::tcp::resolver, boost::asio::ip::tcp::resolver>);
static_assert(std::is_same_v<webcpp::asio::ip::tcp, boost::asio::ip::tcp>);
static_assert(std::is_same_v<webcpp::asio::ip::udp, boost::asio::ip::udp>);
static_assert(std::is_same_v<webcpp::asio::ip::tcp::socket, boost::asio::ip::tcp::socket>);
static_assert(std::is_same_v<webcpp::asio::ip::udp::socket, boost::asio::ip::udp::socket>);
static_assert(std::is_same_v<webcpp::asio::ip::tcp::acceptor, boost::asio::ip::tcp::acceptor>);
#endif

namespace {

void options_are_kept() {
    webcpp::asio::io_context io;
    BOOST_TEST_EQ(io.max_handlers_per_turn(), WEBCPP_TEST_TURN_LIMIT);
    io.max_handlers_per_turn(250);
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 250U);
    io.max_handlers_per_turn(0);
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 1U);
    const webcpp::asio::io_context configured(
        webcpp::asio::io_context_options{.max_handlers_per_turn = 64});
    BOOST_TEST_EQ(configured.max_handlers_per_turn(), 64U);
    const webcpp::asio::io_context zero(
        webcpp::asio::io_context_options{.max_handlers_per_turn = 0});
    BOOST_TEST_EQ(zero.max_handlers_per_turn(), 1U);
}

void a_posted_handler_runs() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
}

// Boost's idiom, a concurrency hint of 1, with the options or without.
void a_concurrency_hint_is_boost_s() {
    webcpp::asio::io_context io{1};
    BOOST_TEST_EQ(io.max_handlers_per_turn(), WEBCPP_TEST_TURN_LIMIT);
    const webcpp::asio::io_context hinted(
        BOOST_ASIO_CONCURRENCY_HINT_1,
        webcpp::asio::io_context_options{.max_handlers_per_turn = 8});
    BOOST_TEST_EQ(hinted.max_handlers_per_turn(), 8U);
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
}

}  // namespace

int main() {
    options_are_kept();
    a_posted_handler_runs();
    a_concurrency_hint_is_boost_s();
    return webcpp::test::exit_status(boost::report_errors());
}
