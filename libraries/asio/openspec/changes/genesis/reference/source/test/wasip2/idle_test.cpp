// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <boost/asio/executor_work_guard.hpp>
#include <cstdio>
#include <optional>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

int main(int argc, char** /*argv*/) {
    webcpp::asio::io_context context;
    auto const guard = boost::asio::make_work_guard(context);
    std::optional<webcpp::asio::steady_timer> timer;
    if (argc > 1) {
        timer.emplace(context, std::chrono::milliseconds(0));
        timer->async_wait([](boost::system::error_code) {});
    }
    if (std::puts("ready") < 0 || std::fflush(stdout) != 0) {
        return 2;
    }
    context.run();  // lint-run: a live guard must wait indefinitely without trapping or spinning
    return 3;
}
