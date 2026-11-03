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

int main() {
    namespace beast = webcpp::beast;
    namespace websocket = beast::websocket;
    boost::asio::io_context context;
    websocket::stream<supplied_example::channel> stream(context.get_executor());
    stream.set_option(websocket::stream_base::decorator([](websocket::response_type& response) {
        response.set(beast::http::field::server,
                     std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-local");
    }));
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
}
