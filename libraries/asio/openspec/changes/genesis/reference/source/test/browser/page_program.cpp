// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A page program loaded by the browser lane (doc: #asio-building-loader).
#include <webcpp/asio.hpp>

#include "../browser_values.hpp"

#include <boost/asio/post.hpp>

#include <cstdio>

// tag::page-program[]
int main() {
    webcpp::asio::io_context io;
    int ran = 0;
    for (int i = 0; i < 3; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    io.run();  // lint-run: the page's event loop, suspended into the browser's
    if (ran != 3) {
        std::puts("handler count mismatch");
        return 1;
    }
    std::puts("ran 3 handlers");
    return 0;
}

// end::page-program[]
