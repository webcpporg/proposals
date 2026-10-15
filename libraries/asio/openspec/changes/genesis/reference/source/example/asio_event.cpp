// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <emscripten/em_js.h>
#include <cstdio>
#include <webcpp/asio/browser/async_wait_event.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/test/exit_status.hpp>

// clang-format off
EM_JS(void, send_event, (emscripten::EM_VAL handle), {
    const target = Emval.toValue(handle);
    setTimeout(() => target.dispatchEvent(
        new CustomEvent('message', {detail: 5})), 20);
});
// clang-format on

int main() {
    webcpp::asio::io_context io;
    const auto target = emscripten::val::global("EventTarget").new_();
    int failures = 0;
    webcpp::asio::browser::async_wait_event(
        io.get_executor(), target, "message",
        [&](boost::system::error_code ec, const emscripten::val& event) {
            if (ec) {
                ++failures;
                return;
            }
            std::printf("event: message %d\n", event["detail"].as<int>());
        });
    send_event(target.as_handle());
    io.run();  // lint-run: main waits for the browser event
    return webcpp::test::exit_status(failures);
}

// end::program[]
