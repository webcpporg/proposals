// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// C++ exceptions across a suspension, in each variant with its own exception model (doc:
// #asio-invariant-7): a try that encloses a run which suspends in the reactor's wait catches
// nothing when no handler throws; a run called inside a catch block suspends and resumes there;
// a handler's exception thrown after run suspended reaches main's catch, and the next run after
// it is no nested call; and so does one thrown after the run family yielded to the browser,
// 2,000 handlers having filled twenty turns of the default limit first.

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include "page_timeout.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace {

using namespace std::chrono_literals;

void a_try_encloses_a_suspended_run() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code) { fired = true; });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (...) {
        caught = true;
    }
    BOOST_TEST(fired);
    BOOST_TEST(!caught);
}

void a_catch_block_runs_and_suspends() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code) { fired = true; });
    try {
        throw std::runtime_error("before run");
    } catch (const std::runtime_error&) {
        io.run();  // lint-run: the test's event loop, suspended inside a catch block
    }
    BOOST_TEST(fired);
}

void a_handler_s_exception_reaches_main_after_a_suspension() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    timer.async_wait([](boost::system::error_code) { throw std::runtime_error("handler"); });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "handler";
    }
    BOOST_TEST(caught);
    // The exception cleared the run family's flag on its way: the next run is no nested call.
    bool again = false;
    boost::asio::post(io, [&again] { again = true; });
    io.restart();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop, after the catch
    BOOST_TEST(again);
}

void a_throw_after_a_yield_reaches_main() {
    webcpp::asio::io_context io;
    const webcpp::test::yield_counter yields;
    std::size_t ran = 0;
    for (std::size_t i = 0; i < 2000; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    boost::asio::post(io, [] { throw std::runtime_error("after a yield"); });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_TEST(caught);
    BOOST_TEST_EQ(ran, 2000U);
#ifdef __EMSCRIPTEN__
    BOOST_TEST_EQ(yields.count(), 20);
#else
    BOOST_TEST_EQ(yields.count(), 0);
#endif
}

}  // namespace

int main() {
    a_try_encloses_a_suspended_run();
    a_catch_block_runs_and_suspends();
    a_handler_s_exception_reaches_main_after_a_suspension();
    a_throw_after_a_yield_reaches_main();
    return webcpp::test::exit_status(boost::report_errors());
}
