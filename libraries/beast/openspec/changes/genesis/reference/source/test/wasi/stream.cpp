// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/wasi/stream.hpp>

#ifndef __wasi__
namespace {
int verify_signal_state(bool pending_signal) {
    sigset_t selected{};
    sigset_t previous{};
    sigset_t pending{};
    sigset_t current{};
    static_cast<void>(sigemptyset(&selected));
    static_cast<void>(sigaddset(&selected, SIGPIPE));
    if (::pthread_sigmask(SIG_BLOCK, &selected, &previous) != 0) {
        return 4;
    }
    if (pending_signal) {
        static_cast<void>(::raise(SIGPIPE));
    }
    webcpp::beast::wasi::stdio_stream stream;
    boost::system::error_code error;
    stream.write_some(boost::asio::buffer("abc", 3), error);
    static_cast<void>(::pthread_sigmask(SIG_BLOCK, nullptr, &current));
    static_cast<void>(::sigpending(&pending));
    const bool preserved = sigismember(&current, SIGPIPE) == 1 &&
                           (sigismember(&pending, SIGPIPE) == 1) == pending_signal;
    if (sigismember(&pending, SIGPIPE) == 1) {
        int caught = 0;
        static_cast<void>(::sigwait(&selected, &caught));
    }
    static_cast<void>(::pthread_sigmask(SIG_SETMASK, &previous, nullptr));
    if (!preserved || error != boost::system::errc::broken_pipe) {
        return 4;
    }
    static_cast<void>(std::fprintf(stderr, "write: signal state preserved\n"));
    return 2;
}
}  // namespace
#endif

int main(int argc, const char* const* argv) {
#ifndef __wasi__
    if (argc > 1 && std::string_view(argv[1]) == "--preserve-signal") {
        return verify_signal_state(true);
    }
    if (argc > 1 && std::string_view(argv[1]) == "--preserve-mask") {
        return verify_signal_state(false);
    }
#else
    static_cast<void>(argv);
#endif
    webcpp::beast::wasi::stdio_stream stream;
    if (argc > 1) {
        const auto stop = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        webcpp::beast::wasi::stream_control control;
        control.cancellation_context = const_cast<std::chrono::steady_clock::time_point*>(&stop);
        control.cancelled = [](void* context) {
            return std::chrono::steady_clock::now() >=
                   *static_cast<std::chrono::steady_clock::time_point*>(context);
        };
        stream.control(control);
        std::array<char, 4096> bytes{};
        boost::system::error_code error;
        for (;;) {
            stream.write_some(boost::asio::buffer(bytes), error);
            if (error) {
                break;
            }
        }
        if (error != boost::system::errc::operation_canceled) {
            return 3;
        }
        static_cast<void>(std::fprintf(stderr, "cancelled blocked output\n"));
        return 0;
    }
    webcpp::beast::flat_buffer buffer;
    webcpp::beast::http::request_parser<webcpp::beast::http::string_body> parser;
    boost::system::error_code error;
    webcpp::beast::wasi::read(stream, buffer, parser, error);
    if (error) {
        static_cast<void>(std::fprintf(stderr, "read: %s\n", error.message().c_str()));
        return 1;
    }
    auto request = parser.release();
    webcpp::beast::http::response<webcpp::beast::http::string_body> response{
        webcpp::beast::http::status::ok,
        11,
    };
    response.set(webcpp::beast::http::field::content_type, "text/plain");
    response.body() = request.body();
    response.prepare_payload();
    webcpp::beast::wasi::write(stream, response, error);
    if (error) {
        static_cast<void>(std::fprintf(stderr, "write: %s\n", error.message().c_str()));
    }
    return error ? 2 : 0;
}
