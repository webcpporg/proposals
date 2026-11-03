// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Native Beast peers shared by the browser and Node contracts. The driver owns
// this process and bounds its lifetime, including deliberately stalled sessions.
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/ssl.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

namespace {
namespace beast = webcpp::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
using net::ip::tcp;

struct barrier {
    std::mutex mutex;
    std::condition_variable changed;
    bool released = false;
};

template <class Stream>
void websocket(Stream stream, const beast::http::request<beast::http::string_body>& request,
               barrier& released) {
    beast::websocket::stream<Stream> socket(std::move(stream));
    boost::system::error_code ec;
    const auto protocol = request[beast::http::field::sec_websocket_protocol];
    socket.set_option(beast::websocket::stream_base::decorator(
        [protocol = std::string(protocol)](auto& response) {
            if (!protocol.empty()) {
                response.set(beast::http::field::sec_websocket_protocol,
                             protocol.substr(0, protocol.find(',')));
            }
        }));
    socket.accept(request, ec);
    if (ec) {
        return;
    }
    socket.text(true);
    if (request.target() == "/close") {
        socket.close(beast::websocket::close_reason{static_cast<beast::websocket::close_code>(4001),
                                                    "peer close"},
                     ec);
        return;
    }
    if (request.target() == "/empty-flood") {
        for (int n = 0; n != 64 && !ec; ++n) {
            socket.write(net::buffer("", 0), ec);
        }
    } else if (request.target() == "/retained-flood") {
        socket.write(net::buffer(std::string(128, 'A')), ec);
    }
    while (!ec) {
        beast::flat_buffer buffer;
        socket.read(buffer, ec);
        if (ec) {
            return;
        }
        if (request.target() == "/retained-flood") {
            socket.write(net::buffer(std::string(128, 'B')), ec);
        } else {
            if (request.target() == "/delayed-echo") {
                std::unique_lock lock(released.mutex);
                released.changed.wait_for(lock, std::chrono::seconds(5),
                                          [&] { return released.released; });
                released.released = false;
            }
            socket.text(socket.got_text());
            socket.write(buffer.data(), ec);
        }
    }
}

std::string policy_headers(const beast::http::request<beast::http::string_body>& request) {
    std::string result;
    for (const auto* name :
         {"set-cookie", "x-http-method", "x-http-method-override", "x-method-override"}) {
        if (request.contains(name)) {
            const auto value = request[name];
            result += std::string(name) + "=" + std::string(value.data(), value.size()) + "\n";
        }
    }
    return result;
}

template <class Stream>
void session(Stream stream, barrier& headers) {
    boost::system::error_code ec;
    beast::flat_buffer buffer;
    for (;;) {
        beast::http::request<beast::http::string_body> request;
        beast::http::read(stream, buffer, request, ec);
        if (ec) {
            return;
        }
        if (beast::websocket::is_upgrade(request) && request.target() != "/refuse") {
            websocket(std::move(stream), request, headers);
            return;
        }
        if (request.target() == "/empty-reason") {
            const std::string wire =
                "HTTP/1.1 200 \r\nContent-Length: 12\r\nConnection: close\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\nempty reason";
            net::write(stream, net::buffer(wire), ec);
            return;
        }
        beast::http::response<beast::http::string_body> response{beast::http::status::ok, 11};
        response.keep_alive(request.keep_alive());
        response.set(beast::http::field::content_type, "text/plain");
        response.set(beast::http::field::access_control_allow_origin, "*");
        response.set(beast::http::field::access_control_allow_methods, "GET,POST,OPTIONS");
        response.set(beast::http::field::access_control_allow_headers,
                     "x-http-method,x-http-method-override,x-method-override,x-allocator-field");
        if (request.target() == "/refuse") {
            response.result(beast::http::status::forbidden);
        } else if (request.target() == "/redirect") {
            response.result(beast::http::status::found);
            response.set(beast::http::field::location, "/http");
        } else if (request.target() == "/release") {
            {
                const std::scoped_lock lock(headers.mutex);
                headers.released = true;
            }
            headers.changed.notify_all();
            response.result(beast::http::status::no_content);
        } else if (request.target() == "/request-headers") {
            response.body() = policy_headers(request);
        } else if (request.target() == "/custom-reason") {
            response.reason("Fixture custom");
            response.body() = "custom reason";
        } else {
            response.body() =
                request.method() == beast::http::verb::get ? "fetch body" : request.body();
        }
        if (request.target() == "/headers") {
            response.body() = "released body";
            response.prepare_payload();
            beast::http::response_serializer<beast::http::string_body> serializer(response);
            beast::http::write_header(stream, serializer, ec);
            {
                std::unique_lock lock(headers.mutex);
                headers.changed.wait_for(lock, std::chrono::seconds(5),
                                         [&] { return headers.released; });
                headers.released = false;
            }
            net::write(stream, net::buffer(response.body()), ec);
        } else if (request.method() == beast::http::verb::head) {
            // A HEAD response without Content-Length exercises metadata-only parsers.
            beast::http::response_serializer<beast::http::string_body> serializer(response);
            beast::http::write_header(stream, serializer, ec);
        } else {
            response.prepare_payload();
            beast::http::write(stream, response, ec);
        }
        if (ec || !response.keep_alive()) {
            return;
        }
    }
}

class listener {
    tcp::acceptor acceptor_;
    ssl::context* tls_;
    barrier& headers_;

public:
    listener(net::io_context& io, ssl::context* tls, barrier& headers)
        : acceptor_(io, {net::ip::make_address("127.0.0.1"), 0}), tls_(tls), headers_(headers) {}

    void run() { accept(); }

    [[nodiscard]] unsigned short port() const { return acceptor_.local_endpoint().port(); }

private:
    void accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::thread([this, socket = std::move(socket)]() mutable {
                    if (tls_) {
                        ssl::stream<tcp::socket> stream(std::move(socket), *tls_);
                        boost::system::error_code handshake;
                        if (!stream.handshake(ssl::stream_base::server, handshake)) {
                            session(std::move(stream), headers_);
                        }
                    } else {
                        session(std::move(socket), headers_);
                    }
                }).detach();
            }
            accept();
        });
    }
};
}  // namespace

int main(int argc, const char* const* argv) {
    if (argc != 3) {
        return 2;
    }
    net::io_context io;
    ssl::context tls(ssl::context::tls_server);
    boost::system::error_code ec;
    if (tls.use_certificate_chain_file(argv[1], ec)) {
        return 3;
    }
    if (tls.use_private_key_file(argv[2], ssl::context::pem, ec)) {
        return 4;
    }
    barrier headers;
    listener plain(io, nullptr, headers);
    listener secure(io, &tls, headers);
    plain.run();
    secure.run();
    std::printf("{\"plain\":%u,\"secure\":%u}\n", plain.port(), secure.port());
    if (std::fflush(stdout) != 0) {
        return 5;
    }
    io.run();  // lint-run: fixture process is bounded and reaped by its driver
}
