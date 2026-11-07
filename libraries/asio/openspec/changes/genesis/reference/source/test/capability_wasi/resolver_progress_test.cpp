// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <wasi/wasip3.h>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../wasip2/counting_allocator.hpp"

namespace {
struct test_state {
    int lookups = 0;
    wasip3_subtask_t pending_handle = 0;
    int pending_releases = 0;
};

test_state& state() {
    static test_state value;
    return value;
}
}  // namespace

// Delay the native host boundary deterministically. This retains real component
// subtask/waitable/timer scheduling while avoiding reliance on DNS latency.
// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" std::uint32_t __wrap_webcpp_asio_wasip3_lookup(const char* name,
                                                          [[maybe_unused]] std::size_t size,
                                                          void* result) {
    ++state().lookups;
    if (std::strncmp(name, "cancel.", 7) == 0) {
        auto const status = monotonic_clock_wait_for(1000000000);
        state().pending_handle = WASIP3_SUBTASK_HANDLE(status);
        return status;
    }
    auto* address = static_cast<ip_name_lookup_ip_address_t*>(
        std::calloc(1, sizeof(ip_name_lookup_ip_address_t)));
    address->tag = SOCKETS_IP_ADDRESS_IPV4;
    address->val.ipv4 = {.f0 = 127, .f1 = 0, .f2 = 0, .f3 = 1};
    const ip_name_lookup_list_ip_address_t addresses{.ptr = address, .len = 1};
    auto* bytes = static_cast<unsigned char*>(result);
    bytes[0] = 0;
    std::memcpy(bytes + sizeof(void*), &addresses, sizeof(addresses));
    return monotonic_clock_wait_for(50000000);
}

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" void __real_wasip3_subtask_drop(wasip3_subtask_t);

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" void __wrap_wasip3_subtask_drop(wasip3_subtask_t handle) {
    if (handle == state().pending_handle && handle != 0) {
        state().pending_handle = 0;
        ++state().pending_releases;
    }
    __real_wasip3_subtask_drop(handle);
}

namespace {
void pending_cancellation(bool per_operation) {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    webcpp::asio::io_context context;
    tcp::resolver resolver(context);
    counts allocations;
    asio::cancellation_signal cancellation;
    int completed = 0;
    const auto before = state().pending_releases;
    resolver.async_resolve(
        "cancel.test.invalid", "443",
        asio::bind_allocator(
            allocator<void>(allocations),
            asio::bind_cancellation_slot(
                cancellation.slot(),
                [&](boost::system::error_code error, const tcp::resolver::results_type& result) {
                    BOOST_TEST_EQ(error, asio::error::operation_aborted);
                    BOOST_TEST(result.empty());
                    BOOST_TEST_EQ(allocations.live, 0U);
                    BOOST_TEST_EQ(state().pending_releases, before + 1);
                    ++completed;
                })));
    BOOST_TEST_EQ(context.poll_one(), 1U);
    BOOST_TEST(state().pending_handle != 0);
    if (per_operation) {
        cancellation.emit(asio::cancellation_type::all);
    } else {
        resolver.cancel();
    }
    context
        .run();  // lint-run: release a genuinely pending host subtask before cancellation delivery
    BOOST_TEST_EQ(completed, 1);
    BOOST_TEST_EQ(state().pending_handle, 0U);
}

void pending_shutdown() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    counts allocations;
    const auto before = state().pending_releases;
    {
        webcpp::asio::io_context context;
        tcp::resolver resolver(context);
        resolver.async_resolve(
            "cancel.test.invalid", "443",
            asio::bind_allocator(allocator<void>(allocations),
                                 [](boost::system::error_code, const tcp::resolver::results_type&) {
                                     BOOST_ERROR("abandoned resolver completion");
                                 }));
        BOOST_TEST_EQ(context.poll_one(), 1U);
        BOOST_TEST(state().pending_handle != 0);
    }
    BOOST_TEST_EQ(allocations.live, 0U);
    BOOST_TEST_EQ(state().pending_releases, before + 1);
    BOOST_TEST_EQ(state().pending_handle, 0U);
}

}  // namespace

int main() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    webcpp::asio::io_context context;
    tcp::resolver resolver(context);
    counts allocations;
    bool resolved = false;
    bool timer_fired = false;
    auto lazy = resolver.async_resolve("delayed.test.invalid", "443", asio::deferred);
    BOOST_TEST_EQ(state().lookups, 0);
    BOOST_TEST_EQ(context.poll(), 0U);
    context.restart();
    std::move(lazy)(asio::bind_allocator(
        allocator<void>(allocations),
        [&](boost::system::error_code error, const tcp::resolver::results_type& result) {
            BOOST_TEST_EQ(error, boost::system::error_code{});
            BOOST_TEST_EQ(result.size(), 1U);
            BOOST_TEST(timer_fired);
            BOOST_TEST_EQ(allocations.live, 0U);
            if (!result.empty()) {
                BOOST_TEST_EQ(result.begin()->endpoint().port(), 443);
            }
            resolved = true;
        }));
    BOOST_TEST_EQ(state().lookups, 0);
    BOOST_TEST(!resolved);
    webcpp::asio::steady_timer timer(context, std::chrono::milliseconds(1));
    timer.async_wait([&](boost::system::error_code error) {
        BOOST_TEST(!error);
        BOOST_TEST(!resolved);
        BOOST_TEST_EQ(state().lookups, 1);
        timer_fired = true;
    });
    context.run();  // lint-run: prove the native pending lookup permits unrelated timer progress
    BOOST_TEST(resolved);
    BOOST_TEST(timer_fired);
    BOOST_TEST_EQ(allocations.live, 0U);
    pending_cancellation(false);
    pending_cancellation(true);
    pending_shutdown();
    return webcpp::test::exit_status(boost::report_errors());
}
