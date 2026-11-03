// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A threaded build keeps the ordinary main-thread run and DOM-event contract.
#include <emscripten.h>
#include <emscripten/threading.h>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdio>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/test/exit_status.hpp>

namespace {
struct state {
    webcpp::asio::io_context io;
    boost::asio::executor_work_guard<webcpp::asio::io_context::executor_type> work{
        io.get_executor()};
    int completed = 0;
};
}  // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void complete_main(state* pending) {
    boost::asio::post(pending->io, [pending] {
        BOOST_TEST(emscripten_is_main_browser_thread());
        ++pending->completed;
        pending->work.reset();
    });
}

int main() {
    state pending;
    EM_ASM({ setTimeout(function() { _complete_main($0); }, 30); }, &pending);
    BOOST_TEST_EQ(pending.io.run(), 1U);  // lint-run: ordinary browser main entry stack
    BOOST_TEST_EQ(pending.completed, 1);
    std::puts("worker profile main complete");
    return webcpp::test::exit_status(boost::report_errors());
}
