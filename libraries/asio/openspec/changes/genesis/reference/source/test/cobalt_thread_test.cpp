// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/this_coro.hpp>
#include <boost/cobalt/thread.hpp>
#include <boost/core/lightweight_test.hpp>

#include <atomic>
#include <cstdio>

namespace {

boost::cobalt::thread worker(std::atomic<int>& completed) {
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    const auto exec = co_await boost::cobalt::this_coro::executor;
    co_await boost::asio::post(exec, boost::cobalt::use_op);
    completed.store(42);
}

}  // namespace

boost::cobalt::main co_main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::atomic<int> completed{0};
    auto operation = worker(completed);
    co_await operation;
    BOOST_TEST_EQ(completed.load(), 42);
    std::puts("cobalt: thread coroutine completed and joined");
    co_return webcpp::test::exit_status(boost::report_errors());
}
