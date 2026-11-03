// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The browser contract driver serves this page beside its controlled peers.
#include <webcpp/asio.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

#include <emscripten/val.h>
#include <boost/asio/buffer.hpp>
#include <cstdio>
#include <string>

int main() {
    namespace beast = webcpp::beast;
    namespace net = webcpp::asio;
    net::io_context io;
    net::ip::tcp::resolver resolver(io);
    boost::system::error_code ec;
    const auto port = (emscripten::val::global("beastPeerPort").isUndefined()
                           ? emscripten::val::global("location")["port"]
                           : emscripten::val::global("beastPeerPort"))
                          .as<std::string>();
    const std::string host = "127.0.0.1:" + port;
    const auto addresses = resolver.resolve("127.0.0.1", port, ec);
    if (ec) {
        return 1;
    }

    // tag::browser-websocket-client[]
    beast::websocket::stream<beast::tcp_stream> socket(io);
    beast::get_lowest_layer(socket).connect(addresses, ec);
    if (ec) {
        return 1;
    }
    // These synchronous operations suspend into the browser event loop.
    socket.handshake(host, "/echo", ec);
    if (ec) {
        return 1;
    }
    socket.text(true);
    socket.write(boost::asio::buffer("hello", 5), ec);
    if (ec) {
        return 1;
    }
    beast::flat_buffer incoming;
    socket.read(incoming, ec);
    if (ec || beast::buffers_to_string(incoming.data()) != "hello") {
        return 1;
    }
    socket.close(beast::websocket::close_code::normal, ec);
    if (ec) {
        return 1;
    }
    // end::browser-websocket-client[]

    // tag::browser-fetch-client[]
    beast::tcp_stream stream(io);
    stream.connect(addresses, ec);
    if (ec) {
        return 1;
    }
    beast::http::request<beast::http::string_body> request{beast::http::verb::post, "/http", 11};
    request.set(beast::http::field::host, host);
    request.body() = "hello";
    request.prepare_payload();
    beast::http::write(stream, request, ec);
    if (ec) {
        return 1;
    }
    beast::http::response<beast::http::string_body> response;
    incoming.consume(incoming.size());
    beast::http::read(stream, incoming, response, ec);
    if (ec || response.result_int() != 200 || response.body() != "hello") {
        return 1;
    }
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);
    if (ec) {
        return 1;
    }
    // end::browser-fetch-client[]
    std::puts("browser tutorial: ok");
}
