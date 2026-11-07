// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/system/system_error.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/basic_resolver.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/test/exit_status.hpp>

namespace {
using resolver = webcpp::asio::ip::tcp::resolver;

void check_entry(const resolver::results_type& results, std::string_view host,
                 std::string_view service, std::uint16_t port) {
    BOOST_TEST_EQ(results.size(), 1U);
    if (results.empty()) {
        return;
    }
    BOOST_TEST(results.begin()->host_name() == host);
    BOOST_TEST(results.begin()->service_name() == service);
    BOOST_TEST(results.begin()->endpoint().address().is_unspecified());
    BOOST_TEST_EQ(results.begin()->endpoint().port(), port);
}

void services_and_errors() {
    webcpp::asio::io_context io;
    resolver r(io);

    struct expected_port {
        std::string_view service;
        bool found;
        std::uint16_t port;
    };

    constexpr auto services = std::to_array<expected_port>({
        {.service = "80", .found = true, .port = 80},
        {.service = "0", .found = true, .port = 0},
        {.service = "65535", .found = true, .port = 65535},
        {.service = "65536", .found = false, .port = 0},
        {.service = "http", .found = true, .port = 80},
        {.service = "ws", .found = true, .port = 80},
        {.service = "https", .found = true, .port = 443},
        {.service = "wss", .found = true, .port = 443},
        {.service = "WS", .found = false, .port = 0},
        {.service = "80x", .found = false, .port = 0},
        {.service = "", .found = true, .port = 0},
        {.service = "-1", .found = false, .port = 0},
        {.service = "+80", .found = false, .port = 0},
        {.service = " 80", .found = false, .port = 0},
        {.service = "99999999999999999999999", .found = false, .port = 0},
    });
    boost::system::error_code ec;
    for (const auto& entry : services) {
        const auto results = r.resolve("example.com", entry.service, ec);
        if (entry.found) {
            BOOST_TEST(!ec);
            check_entry(results, "example.com", entry.service, entry.port);
        } else {
            BOOST_TEST(ec == boost::asio::error::service_not_found);
            BOOST_TEST(results.empty());
        }
    }
    BOOST_TEST(r.resolve("h", "https", resolver::numeric_service, ec).empty());
    BOOST_TEST(ec == boost::asio::error::service_not_found);
    check_entry(r.resolve("h", "443", resolver::numeric_service, ec), "h", "443", 443);
    BOOST_TEST(!ec);
    const auto entry = r.resolve("example.com", "https", ec);
    webcpp::asio::ip::tcp::socket socket(io);
    BOOST_TEST(socket.connect(*entry.begin(), ec) == boost::asio::error::operation_not_supported);
    const webcpp::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 80);
    BOOST_TEST(r.resolve(endpoint, ec).empty());
    BOOST_TEST(ec == boost::asio::error::operation_not_supported);
    webcpp::asio::ip::udp::resolver udp(io);
    const auto datagram = udp.resolve("example.com", "53", ec);
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(datagram.begin()->endpoint().port(), 53);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    bool caught = false;
    try {
        (void)r.resolve("h", "nope");
    } catch (const boost::system::system_error& error) {
        caught = error.code() == boost::asio::error::service_not_found;
    }
    BOOST_TEST(caught);
#endif
}

void asynchronous_results() {
    webcpp::asio::io_context io;
    resolver r(io.get_executor());
    BOOST_TEST(r.get_executor() == io.get_executor());
    bool returned = false;
    unsigned calls = 0;
    auto handler = [&](boost::system::error_code ec, const resolver::results_type& results) {
        BOOST_TEST(returned);
        BOOST_TEST(!ec);
        check_entry(results, "example.com", "https", 443);
        ++calls;
    };
    r.async_resolve("example.com", "https", handler);
    std::string host = "example.com";
    std::string service = "https";
    auto deferred = r.async_resolve(host, service, boost::asio::deferred);
    host = "changed";
    service = "nope";
    std::move(deferred)(handler);
    r.async_resolve("h", "https", resolver::numeric_service,
                    [&](boost::system::error_code ec, const resolver::results_type& results) {
                        BOOST_TEST(returned);
                        BOOST_TEST(ec == boost::asio::error::service_not_found);
                        BOOST_TEST(results.empty());
                        ++calls;
                    });
    const webcpp::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 80);
    auto reverse = r.async_resolve(endpoint, boost::asio::deferred);
    std::move(reverse)([&](boost::system::error_code ec, const resolver::results_type& results) {
        BOOST_TEST(returned);
        BOOST_TEST(ec == boost::asio::error::operation_not_supported);
        BOOST_TEST(results.empty());
        ++calls;
    });
    r.cancel();  // Results already computed remain queued.
    BOOST_TEST_EQ(calls, 0U);
    returned = true;
    io.run();  // lint-run: the resolver test drains posted completions
    BOOST_TEST_EQ(calls, 4U);
}
}  // namespace

int main() {
    services_and_errors();
    asynchronous_results();
    return webcpp::test::exit_status(boost::report_errors());
}
