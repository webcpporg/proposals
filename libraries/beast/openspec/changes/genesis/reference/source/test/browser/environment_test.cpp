// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/asio.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/ssl.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

#include <emscripten/val.h>
#include <boost/asio/post.hpp>
#define BOOST_LIGHTWEIGHT_TEST_OSTREAM std::cout
#include <boost/core/lightweight_test.hpp>
#include <cstdio>
#include <string>

namespace {
namespace beast = webcpp::beast;
namespace net = webcpp::asio;
using secure_stream = beast::ssl_stream<beast::tcp_stream>;

void begin_heartbeat(const char* label) {
    std::printf("heartbeat %s: start\n", label);
    // The native peer withholds its response bytes until this browser timer releases it.
    // clang-format off
    EM_ASM({
        globalThis.beastHeartbeatCount = 0;
        globalThis.beastHeartbeatTimer = setInterval(() => ++globalThis.beastHeartbeatCount, 10);
        setTimeout(() => fetch('https://127.0.0.1:' + globalThis.beastSecurePort + '/release'), 350);
    });
    // clang-format on
}

void finish_heartbeat(const char* label) {
    EM_ASM({ clearInterval(globalThis.beastHeartbeatTimer); });
    std::printf("heartbeat %s: finish\n", label);
    BOOST_TEST_GE(emscripten::val::global("beastHeartbeatCount").as<unsigned>(), 3U);
}

void delayed_synchronous_read(const std::string& port) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    net::ssl::context tls(net::ssl::context::tls_client);
    tls.set_verify_mode(net::ssl::verify_peer, ec);
    secure_stream stream(io, tls);
    beast::get_lowest_layer(stream).connect(resolver.resolve("127.0.0.1", port, ec), ec);
    stream.handshake(net::ssl::stream_base::client, ec);
    beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/headers", 11};
    beast::http::write(stream, request, ec);
    BOOST_TEST(!ec);
    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> response;
    beast::http::read_header(stream, buffer, response, ec);
    if (!BOOST_TEST(!ec)) {
        return;
    }
    BOOST_TEST(response.body().empty());
    begin_heartbeat("http");
    beast::http::read(stream, buffer, response, ec);
    finish_heartbeat("http");
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(response.body(), "released body");
}

void delayed_websocket_read(const std::string& port) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    net::ssl::context tls(net::ssl::context::tls_client);
    tls.set_verify_mode(net::ssl::verify_peer, ec);
    beast::websocket::stream<secure_stream> socket(io, tls);
    beast::get_lowest_layer(socket).connect(resolver.resolve("127.0.0.1", port, ec), ec);
    socket.next_layer().handshake(net::ssl::stream_base::client, ec);
    socket.handshake("127.0.0.1:" + port, "/delayed-echo", ec);
    if (!BOOST_TEST(!ec)) {
        return;
    }
    socket.write(boost::asio::buffer("delayed echo", 12), ec);
    BOOST_TEST(!ec);
    begin_heartbeat("websocket");
    beast::flat_buffer buffer;
    socket.read(buffer, ec);
    finish_heartbeat("websocket");
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(beast::buffers_to_string(buffer.data()), "delayed echo");
    socket.close(beast::websocket::close_code::normal, ec);
    BOOST_TEST(!ec);
}

void nested_entry(const std::string& port) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    const auto endpoints = resolver.resolve("127.0.0.1", port, ec);
    net::ssl::context tls(net::ssl::context::tls_client);
    tls.set_verify_mode(net::ssl::verify_peer, ec);
    secure_stream stream(io, tls);
    beast::get_lowest_layer(stream).connect(endpoints, ec);
    stream.handshake(net::ssl::stream_base::client, ec);
    beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
    beast::http::write(stream, request, ec);
    BOOST_TEST(!ec);
    beast::websocket::stream<secure_stream> socket(io, tls);
    beast::get_lowest_layer(socket).connect(endpoints, ec);
    socket.next_layer().handshake(net::ssl::stream_base::client, ec);
    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> response;
    bool checked = false;
    boost::asio::post(io, [&] {
        const auto size = beast::http::read(stream, buffer, response, ec);
        BOOST_TEST(ec == boost::asio::error::operation_not_supported);
        BOOST_TEST_EQ(size, 0U);
        BOOST_TEST(response.body().empty());
        socket.handshake("127.0.0.1:" + port, "/echo", ec);
        BOOST_TEST(ec == boost::asio::error::operation_not_supported);
        BOOST_TEST(!socket.is_open());
        checked = true;
    });
    io.run();  // lint-run: execute the handler that must reject nested synchronous facade entry
    BOOST_TEST(checked);
    // Rejection must leave both facades usable from the outer entry point.
    beast::http::read(stream, buffer, response, ec);
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(response.body(), "fetch body");
    socket.handshake("127.0.0.1:" + port, "/echo", ec);
    BOOST_TEST(!ec);
    socket.close(beast::websocket::close_code::normal, ec);
    BOOST_TEST(!ec);
    std::puts("nested HTTP and WebSocket entry: rejected");
}

void mixed_content(const std::string& port) {
    BOOST_TEST_EQ(emscripten::val::global("location")["protocol"].as<std::string>(), "https:");
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    beast::tcp_stream stream(io);
    // This fixture hostname maps to the local peer, but is not a trustworthy loopback origin.
    stream.connect(resolver.resolve("mixed.beast.test", port, ec), ec);
    beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
    request.set(beast::http::field::host, "mixed.beast.test:" + port);
    beast::http::write(stream, request, ec);
    BOOST_TEST(!ec);
    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> response;
    const auto size = beast::http::read(stream, buffer, response, ec);
    BOOST_TEST(ec == beast::browser_errc::fetch_failed);
    BOOST_TEST_EQ(size, 0U);
    std::puts("mixed content: fetch_failed");
}
}  // namespace

int main() {
    const auto secure_port = emscripten::val::global("beastSecurePort").as<std::string>();
    delayed_synchronous_read(secure_port);
    delayed_websocket_read(secure_port);
    nested_entry(secure_port);
    mixed_content(emscripten::val::global("beastPeerPort").as<std::string>());
    if (boost::report_errors() != 0) {
        return 1;
    }
    std::puts("browser environment: ok");
}
