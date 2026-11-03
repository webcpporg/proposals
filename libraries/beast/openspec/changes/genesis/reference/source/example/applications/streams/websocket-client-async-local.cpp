// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//
// Explicit supplied-channel counterpart of websocket/client/async-local.
// Retains the upgrade, request decorator, text echo and normal close handshake.

#include <boost/beast/version.hpp>
#include <cstdio>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/websocket.hpp>
#include "websocket-channel.hpp"
#ifdef __wasip2__
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif

int main(int argc, const char* const* argv) {
    namespace beast = webcpp::beast;
    namespace websocket = beast::websocket;
    if (argc != 3) {
        return 64;
    }
    boost::asio::io_context context;
    websocket::stream<supplied_example::channel> stream(context.get_executor());
    stream.set_option(websocket::stream_base::decorator([](websocket::request_type& request) {
        request.set(beast::http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-local");
    }));
#ifdef __wasip2__
    int result = 1;
    boost::asio::co_spawn(
        context,
        [&]() -> boost::asio::awaitable<void> {
            auto token = boost::asio::as_tuple(boost::asio::use_awaitable);
            auto [handshake_error] = co_await stream.async_handshake(argv[1], "/", token);
            if (handshake_error) {
                co_return;
            }
            stream.text(true);
            auto [write_error, written] = co_await stream.async_write(
                boost::asio::buffer(argv[2], std::strlen(argv[2])), token);
            if (write_error) {
                result = 2;
                co_return;
            }
            beast::flat_buffer reply;
            auto [read_error, received] = co_await stream.async_read(reply, token);
            if (read_error) {
                result = 3;
                co_return;
            }
            const auto output = boost::beast::buffers_to_string(reply.data());
            static_cast<void>(std::fprintf(stderr, "%s\n", output.c_str()));
            auto [close_error] = co_await stream.async_close(websocket::close_code::normal, token);
            result = close_error ? 4 : 0;
        },
        [&](const std::exception_ptr& error) {
            if (error) {
                result = 5;
            }
        });
    context.run();  // lint-run: drive supplied-channel readiness and protocol completions
    return result;
#else
    boost::system::error_code error;
    stream.handshake(argv[1], "/", error);
    if (error) {
        return 1;
    }
    stream.text(true);
    stream.write(boost::asio::buffer(argv[2], std::strlen(argv[2])), error);
    if (error) {
        return 2;
    }
    beast::flat_buffer reply;
    stream.read(reply, error);
    if (error) {
        return 3;
    }
    const auto output = boost::beast::buffers_to_string(reply.data());
    static_cast<void>(std::fprintf(stderr, "%s\n", output.c_str()));
    stream.close(websocket::close_code::normal, error);
    return error ? 4 : 0;
#endif
}
