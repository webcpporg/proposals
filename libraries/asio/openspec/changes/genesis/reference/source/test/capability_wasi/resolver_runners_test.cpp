// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <wasi/wasip3.h>
#include <atomic>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <thread>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../wasip2/counting_allocator.hpp"

namespace {
struct test_state {
    std::atomic<wasip3_subtask_t> pending_handle{0};
    std::atomic<bool> poll_owns_waitable{false};
};

test_state& state() {
    static test_state value;
    return value;
}
}  // namespace

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" std::uint32_t __wrap_webcpp_asio_wasip3_lookup([[maybe_unused]] const char* name,
                                                          [[maybe_unused]] std::size_t size,
                                                          [[maybe_unused]] void const* result) {
    auto const status = monotonic_clock_wait_for(1000000000);
    state().pending_handle = WASIP3_SUBTASK_HANDLE(status);
    return status;
}

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" void __real_wasip3_waitable_join(std::uint32_t, wasip3_waitable_set_t);

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" void __wrap_wasip3_waitable_join(std::uint32_t waitable, wasip3_waitable_set_t set) {
    __real_wasip3_waitable_join(waitable, set);
    if (waitable != 0 && waitable == state().pending_handle.load()) {
        state().poll_owns_waitable = set != 0;
    }
}

int main() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    for (bool const per_operation : {false, true}) {
        webcpp::asio::io_context context;
        tcp::resolver resolver(context);
        asio::cancellation_signal cancellation;
        counts allocations;
        std::atomic<int> completed{0};
        resolver.async_resolve(
            "pending.test.invalid", "443",
            asio::bind_allocator(
                allocator<void>(allocations),
                asio::bind_cancellation_slot(
                    cancellation.slot(), [&](boost::system::error_code error,
                                             const tcp::resolver::results_type& results) {
                        BOOST_TEST_EQ(error, asio::error::operation_aborted);
                        BOOST_TEST(results.empty());
                        BOOST_TEST_EQ(allocations.live, 0U);
                        ++completed;
                    })));
        BOOST_TEST_EQ(context.poll_one(), 1U);
        std::thread runner([&] {
            context.run();  // lint-run: one runner owns the libc waitable poll during cancellation
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (!state().poll_owns_waitable && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        BOOST_TEST(state().poll_owns_waitable.load());
        if (per_operation) {
            cancellation.emit(asio::cancellation_type::all);
        } else {
            resolver.cancel();
        }
        context.run();  // lint-run: a second runner must not destroy storage still borrowed by poll
        runner.join();
        BOOST_TEST_EQ(completed.load(), 1);
        BOOST_TEST_EQ(allocations.live, 0U);
        BOOST_TEST(!state().poll_owns_waitable.load());
    }
    return webcpp::test::exit_status(boost::report_errors());
}
