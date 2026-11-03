// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <wasi/wasip3.h>
#include <boost/asio/bind_allocator.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../wasip2/counting_allocator.hpp"

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" std::uint32_t __wrap_webcpp_asio_wasip3_lookup([[maybe_unused]] const char* name,
                                                          [[maybe_unused]] std::size_t size,
                                                          void* result) {
    auto* bytes = static_cast<unsigned char*>(result);
#ifdef WEBCPP_TEST_EMPTY_ERROR_MESSAGE
    bytes[0] = 1;
    bytes[sizeof(void*)] = IP_NAME_LOOKUP_ERROR_CODE_OTHER;
    bytes[2 * sizeof(void*)] = 1;
    const wasip3_string_t message{.ptr = reinterpret_cast<std::uint8_t*>(1), .len = 0};
    std::memcpy(bytes + (3 * sizeof(void*)), &message, sizeof(message));
#else
    bytes[0] = 0;
    const ip_name_lookup_list_ip_address_t addresses{
        // Empty canonical allocations encode their alignment in the pointer.
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        .ptr = reinterpret_cast<ip_name_lookup_ip_address_t*>(alignof(ip_name_lookup_ip_address_t)),
        .len = 0,
    };
    std::memcpy(bytes + sizeof(void*), &addresses, sizeof(addresses));
#endif
    return WASIP3_SUBTASK_RETURNED;
}

int main() {
    namespace asio = boost::asio;
    using tcp = webcpp::asio::ip::tcp;
    webcpp::asio::io_context context;
    tcp::resolver resolver(context);
    counts allocations;
    int completed = 0;
    resolver.async_resolve("empty.test.invalid", "443",
                           asio::bind_allocator(allocator<void>(allocations),
                                                [&](boost::system::error_code error,
                                                    const tcp::resolver::results_type& results) {
#ifdef WEBCPP_TEST_EMPTY_ERROR_MESSAGE
                                                    BOOST_TEST_EQ(error, asio::error::no_recovery);
#else
                BOOST_TEST_EQ(error, asio::error::host_not_found);
#endif
                                                    BOOST_TEST(results.empty());
                                                    BOOST_TEST_EQ(allocations.live, 0U);
                                                    ++completed;
                                                }));
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: canonical zero-byte allocations are sentinels, not freeable storage
    BOOST_TEST_EQ(completed, 1);
    return webcpp::test::exit_status(boost::report_errors());
}
