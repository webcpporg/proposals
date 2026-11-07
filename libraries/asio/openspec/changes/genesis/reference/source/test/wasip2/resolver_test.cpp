// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>
#include "counting_allocator.hpp"

namespace {
void initiation_and_cancellation() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    webcpp::asio::io_context context;
    webcpp::asio::io_context completions;
    tcp::resolver resolver(context);
    int completed = 0;
    auto lazy = resolver.async_resolve("localhost", "80", asio::deferred);
    BOOST_TEST_EQ(context.poll(), 0U);
    context.restart();
    std::move(lazy)(asio::bind_executor(
        completions.get_executor(),
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST(!ec);
            BOOST_TEST(!result.empty());
            for (auto const& entry : result) {
                BOOST_TEST_EQ(entry.endpoint().port(), 80);
            }
            ++completed;
        }));
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: the lookup must not invoke another executor's handler
    BOOST_TEST_EQ(completed, 0);
    completions.run();  // lint-run: deliver on the associated executor
    BOOST_TEST_EQ(completed, 1);

    context.restart();
    resolver.async_resolve(
        "localhost", "80",
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST_EQ(ec, asio::error::operation_aborted);
            BOOST_TEST(result.empty());
            ++completed;
        });
    resolver.cancel();
    context.run();  // lint-run: cancellation before the lookup starts
    BOOST_TEST_EQ(completed, 2);

    context.restart();
    asio::cancellation_signal cancel;
    resolver.async_resolve(
        "localhost", "80",
        asio::bind_cancellation_slot(cancel.slot(), [&](boost::system::error_code ec,
                                                        const tcp::resolver::results_type& result) {
            BOOST_TEST_EQ(ec, asio::error::operation_aborted);
            BOOST_TEST(result.empty());
            ++completed;
        }));
    cancel.emit(asio::cancellation_type::all);
    context.run();  // lint-run: per-operation cancellation
    BOOST_TEST_EQ(completed, 3);

    context.restart();
    resolver.async_resolve(
        "127.0.0.1", "443", tcp::resolver::numeric_host,
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST(!ec);
            BOOST_TEST_EQ(result.size(), 1U);
            if (!result.empty()) {
                BOOST_TEST_EQ(result.begin()->endpoint().port(), 443);
            }
            ++completed;
        });
    context.run();  // lint-run: numeric addresses never require DNS
    BOOST_TEST_EQ(completed, 4);
}

void resources_and_errors() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    webcpp::asio::io_context context;
    tcp::resolver resolver(context);
    int completed = 0;
    resolver.async_resolve(
        "localhost", "80",
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST_EQ(ec, asio::error::operation_aborted);
            BOOST_TEST(result.empty());
            ++completed;
        });
    BOOST_TEST_EQ(context.poll_one(), 1U);  // Start the native lookup, leave its completion queued.
    resolver.cancel();
    context.run();  // lint-run: cancel after registration with the pollable reactor
    BOOST_TEST_EQ(completed, 1);

    context.restart();
    resolver.async_resolve(
        "localhost", "80", tcp::resolver::canonical_name,
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST_EQ(ec, asio::error::operation_not_supported);
            BOOST_TEST(result.empty());
            ++completed;
        });
    context.run();  // lint-run: unsupported flags fail deliberately and asynchronously
    BOOST_TEST_EQ(completed, 2);
    context.restart();
    counts allocations;
    resolver.async_resolve(
        "localhost", "80",
        asio::bind_allocator(
            allocator<void>(allocations),
            [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
                BOOST_TEST(!ec);
                BOOST_TEST(!result.empty());
                BOOST_TEST(allocations.allocations >=
                           2);                        // Operation and endpoint scratch storage.
                BOOST_TEST_EQ(allocations.live, 0U);  // Scratch storage is freed before the upcall.
                ++completed;
            }));
    context.run();  // lint-run: verify associated allocation and deallocation before completion
    BOOST_TEST_EQ(completed, 3);

    context.restart();
    resolver.async_resolve(
        "invalid host name", "80",
        [&](boost::system::error_code ec, const tcp::resolver::results_type& result) {
            BOOST_TEST(ec == asio::error::invalid_argument || ec == asio::error::host_not_found);
            BOOST_TEST(result.empty());
            ++completed;
        });
    context.run();  // lint-run: malformed names report a value without an initiation-time failure
    BOOST_TEST_EQ(completed, 4);
    {
        webcpp::asio::io_context abandoned;
        tcp::resolver pending(abandoned);
        pending.async_resolve("localhost", "80",
                              [&](auto, const auto&) { BOOST_ERROR("abandoned callback"); });
        BOOST_TEST_EQ(abandoned.poll_one(), 1U);
    }
}

}  // namespace

int main() {
    initiation_and_cancellation();
    resources_and_errors();
    return webcpp::test::exit_status(boost::report_errors());
}
