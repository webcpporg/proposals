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
// Explicit supplied-channel counterpart of websocket/server/async-local.
// Retains server upgrade decoration, text/binary echo and normal peer close.

#include <boost/beast/version.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/websocket.hpp>
#include "websocket-channel.hpp"
#ifdef __wasip2__
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif

int main() {
    namespace beast = webcpp::beast;
    namespace websocket = beast::websocket;
    boost::asio::io_context context;
    websocket::stream<supplied_example::channel> stream(context.get_executor());
    stream.set_option(websocket::stream_base::decorator([](websocket::response_type& response) {
        response.set(beast::http::field::server,
                     std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-local");
    }));
#ifdef __wasip2__
    int result = 1;
    boost::asio::co_spawn(
        context,
        [&]() -> boost::asio::awaitable<void> {
            auto token = boost::asio::as_tuple(boost::asio::use_awaitable);
            auto [accept_error] = co_await stream.async_accept(token);
            if (accept_error) {
                co_return;
            }
            for (;;) {
                beast::flat_buffer input;
                auto [read_error, received] = co_await stream.async_read(input, token);
                if (read_error == websocket::error::closed) {
                    result = 0;
                    co_return;
                }
                if (read_error) {
                    result = 2;
                    co_return;
                }
                stream.text(stream.got_text());
                auto [write_error, written] = co_await stream.async_write(input.data(), token);
                if (write_error) {
                    result = 3;
                    co_return;
                }
            }
        },
        [&](const std::exception_ptr& error) {
            if (error) {
                result = 4;
            }
        });
    context.run();  // lint-run: drive supplied-channel readiness and protocol completions
    return result;
#else
    boost::system::error_code error;
    stream.accept(error);
    if (error) {
        return 1;
    }
    for (;;) {
        beast::flat_buffer input;
        stream.read(input, error);
        if (error == websocket::error::closed) {
            return 0;
        }
        if (error) {
            return 2;
        }
        stream.text(stream.got_text());
        stream.write(input.data(), error);
        if (error) {
            return 3;
        }
    }
#endif
}
