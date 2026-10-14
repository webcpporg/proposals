// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// An idle page must suspend without polling (specification section 9).
#include <webcpp/asio.hpp>

#include <boost/asio/executor_work_guard.hpp>

#include <cstdio>

int main() {
    webcpp::asio::io_context io;
    auto const guard = boost::asio::make_work_guard(io);
    std::puts("idle");
    io.run();  // lint-run: the page waits for work that the guard keeps outstanding
    return 0;
}
