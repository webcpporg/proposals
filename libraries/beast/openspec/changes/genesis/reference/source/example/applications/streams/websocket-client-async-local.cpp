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
}
