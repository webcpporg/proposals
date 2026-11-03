// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <wasi/wasip2.h>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <webcpp/asio/detail/wasi_pollable_descriptor.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/test/exit_status.hpp>

namespace {
void check_readiness(std::uint64_t delay) {
    webcpp::asio::io_context context;
    auto const ready = monotonic_clock_subscribe_duration(delay);
    webcpp::asio::detail::wasi_pollable_descriptor storage{};
    storage.handle = ready.__handle;
    auto const fd = webcpp_asio_wasi_pollable_descriptor(&storage);
    BOOST_TEST(fd >= 0);
    if (fd < 0) {
        poll_pollable_drop_own(ready);
        return;
    }
    boost::asio::posix::stream_descriptor descriptor(context, fd);
    webcpp::asio::steady_timer timer(context, std::chrono::milliseconds(1));
    int callbacks = 0;
    bool timer_fired = false;
    timer.async_wait([&](boost::system::error_code error) {
        BOOST_TEST(!error);
        timer_fired = true;
    });
    descriptor.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                          [&](boost::system::error_code error) {
                              BOOST_TEST(!error);
                              ++callbacks;
                          });
    BOOST_TEST_EQ(callbacks, 0);
    context.run();  // lint-run: wait for a native pollable alongside an Asio timer
    BOOST_TEST_EQ(callbacks, 1);
    BOOST_TEST(timer_fired);
    descriptor.close();
    BOOST_TEST(!storage.active);
    poll_pollable_drop_own(ready);
}
}  // namespace

int main() {
    check_readiness(0);  // Ready before registration must not lose a wakeup.
    check_readiness(20000000);
    return webcpp::test::exit_status(boost::report_errors());
}
