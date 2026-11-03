// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <wasi/wasip2.h>
#include <boost/core/lightweight_test.hpp>
#include <limits>
#include <optional>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/asio/wasi/stdio_stream.hpp>
#include <webcpp/test/exit_status.hpp>

#ifdef WEBCPP_TEST_PENDING_WRITE
namespace {
std::optional<int32_t>& tested_stream() {
    static std::optional<int32_t> handle;
    return handle;
}
}  // namespace

extern "C" streams_own_pollable_t real_subscribe(streams_borrow_output_stream_t stream) asm(
    "__real_streams_method_output_stream_subscribe");
#endif

// Linker wrappers model valid host quotas at the WASI C boundary. The large
// quota case still writes real bytes to Wasmtime stdout. The pending case
// supplies a native clock pollable that will not become ready during the test.
extern "C" bool real_check_write(
    streams_borrow_output_stream_t stream, uint64_t* capacity,
    streams_stream_error_t* error) asm("__real_streams_method_output_stream_check_write");
extern "C" bool wrapped_check_write(
    streams_borrow_output_stream_t stream, uint64_t* capacity,
    streams_stream_error_t* error) asm("__wrap_streams_method_output_stream_check_write");

extern "C" bool wrapped_check_write(streams_borrow_output_stream_t stream, uint64_t* capacity,
                                    streams_stream_error_t* error) {
#ifdef WEBCPP_TEST_PENDING_WRITE
    if (tested_stream() == stream.__handle) {
        *capacity = 0;
        return true;
    }
    return real_check_write(stream, capacity, error);
#else
    if (!real_check_write(stream, capacity, error)) {
        return false;
    }
    if (*capacity != 0) {
        *capacity = uint64_t{1} << 32u;
    }
    return true;
#endif
}

#ifdef WEBCPP_TEST_PENDING_WRITE
extern "C" streams_own_pollable_t wrapped_subscribe(streams_borrow_output_stream_t stream) asm(
    "__wrap_streams_method_output_stream_subscribe");

extern "C" streams_own_pollable_t wrapped_subscribe(streams_borrow_output_stream_t stream) {
    if (!tested_stream()) {
        tested_stream() = stream.__handle;
        return monotonic_clock_subscribe_instant(std::numeric_limits<uint64_t>::max());
    }
    return real_subscribe(stream);
}
#endif

int main() {
    webcpp::asio::io_context context;
    webcpp::asio::wasi::stdio_stream stream(context.get_executor());
    webcpp::asio::steady_timer timer(context);
    int calls = 0;
#ifdef WEBCPP_TEST_PENDING_WRITE
    timer.expires_after(std::chrono::milliseconds(5));
    timer.async_wait([&](boost::system::error_code error) {
        BOOST_TEST(!error);
        stream.cancel();
    });
#endif
    stream.async_write_some(boost::asio::buffer("ping", 4),
                            [&](boost::system::error_code error, std::size_t transferred) {
#ifdef WEBCPP_TEST_PENDING_WRITE
                                BOOST_TEST_EQ(error, boost::asio::error::operation_aborted);
                                BOOST_TEST_EQ(transferred, 0U);
#else
                                BOOST_TEST(!error);
                                BOOST_TEST_EQ(transferred, 4U);
#endif
                                ++calls;
                            });
    context.run();  // lint-run: exercise host quota conversion and pending-write cancellation
    BOOST_TEST_EQ(calls, 1);
    return webcpp::test::exit_status(boost::report_errors());
}
