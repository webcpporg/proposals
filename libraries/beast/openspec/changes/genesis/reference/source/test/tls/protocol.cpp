// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/ssl.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <boost/asio/post.hpp>
#include <boost/asio/write.hpp>
#include <boost/core/lightweight_test.hpp>

#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>

namespace beast = webcpp::beast;
namespace http = beast::http;
namespace ws = beast::websocket;
namespace ssl = webcpp::asio::ssl;
using tcp = webcpp::asio::ip::tcp;
using boost::system::error_code;
using namespace std::chrono_literals;

namespace {

enum class scenario {
    http,
    websocket,
    wrong_host,
    untrusted,
    cancel_read,
    truncated_shutdown,
    cancel_shutdown,
    handshake_timeout,
};

ssl::context server_context(std::string_view certificate, std::string_view key, int version) {
    ssl::context result(ssl::context::tls_server);
    error_code ec;
    BOOST_TEST(!result.use_certificate_chain(boost::asio::buffer(certificate), ec));
    BOOST_TEST(!ec);
    BOOST_TEST(!result.use_private_key(boost::asio::buffer(key), ssl::context::pem, ec));
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(SSL_CTX_set_min_proto_version(result.native_handle(), version), 1);
    BOOST_TEST_EQ(SSL_CTX_set_max_proto_version(result.native_handle(), version), 1);
    return result;
}

ssl::context client_context(std::string_view authorities, bool trust) {
    ssl::context result(ssl::context::tls_client);
    error_code ec;
    BOOST_TEST(!result.set_verify_mode(ssl::verify_peer, ec));
    BOOST_TEST(!ec);
    if (trust) {
        BOOST_TEST(!result.add_certificate_authority(boost::asio::buffer(authorities), ec));
        BOOST_TEST(!ec);
    }
    return result;
}

template <class Socket>
void close_checked(Socket& socket) {
    if (socket.is_open()) {
        error_code ec;
        BOOST_TEST(!socket.close(ec));
    }
}

class exchange {
    scenario mode_;
    webcpp::asio::io_context context_;
    ssl::context server_context_;
    ssl::context client_context_;
    tcp::acceptor listener_{context_, {boost::asio::ip::address_v4::loopback(), 0}};
    ws::stream<beast::ssl_stream<beast::tcp_stream>> server_;
    ws::stream<beast::ssl_stream<beast::tcp_stream>> client_;
    webcpp::asio::steady_timer deadline_{context_, 5s};
    beast::flat_buffer server_buffer_;
    beast::flat_buffer client_buffer_;
    http::request<http::string_body> request_{http::verb::post, "/echo", 11};
    http::request<http::string_body> received_;
    http::response<http::string_body> response_{http::status::ok, 11};
    http::response_parser<http::string_body> parser_;
    std::array<char, 4096> hello_{};
    const std::string payload{"encrypted\0payload", 17};
    const std::string partial{"HTTP/1.1 200 OK\r\nContent-Length: 32\r\n\r\npart"};
    int shutdowns_ = 0;
    bool finished_ = false;

    void finish() {
        if (finished_) {
            return;
        }
        finished_ = true;
        deadline_.cancel();
        close_checked(listener_);
        close_checked(beast::get_lowest_layer(client_).socket());
        close_checked(beast::get_lowest_layer(server_).socket());
    }

    void expired(error_code ec) {
        if (!ec) {
            BOOST_ERROR("Beast TLS exchange exceeded its deadline");
            finish();
        }
    }

    void accepted(error_code ec) {
        if (finished_ || !BOOST_TEST(!ec)) {
            return;
        }
        if (mode_ == scenario::handshake_timeout) {
            beast::get_lowest_layer(server_).socket().async_read_some(
                boost::asio::buffer(hello_), [this](error_code read_error, std::size_t bytes) {
                    if (!finished_) {
                        BOOST_TEST(!read_error);
                        BOOST_TEST_GT(bytes, 0U);
                    }
                });
            return;
        }
        server_.next_layer().async_handshake(
            ssl::stream_base::server, beast::bind_front_handler(&exchange::server_ready, this));
    }

    void connected(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        if (mode_ == scenario::handshake_timeout) {
            beast::get_lowest_layer(client_).expires_after(100ms);
        }
        client_.next_layer().async_handshake(
            ssl::stream_base::client, beast::bind_front_handler(&exchange::client_ready, this));
    }

    void server_ready(error_code ec) {
        if (finished_ || mode_ == scenario::wrong_host || mode_ == scenario::untrusted) {
            return;
        }
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        const char* name =
            SSL_get_servername(server_.next_layer().native_handle(), TLSEXT_NAMETYPE_host_name);
        if (BOOST_TEST(name != nullptr)) {
            BOOST_TEST_EQ(std::string_view(name), "localhost");
        }
        if (mode_ == scenario::websocket) {
            server_.async_accept(beast::bind_front_handler(&exchange::ws_server_ready, this));
            return;
        }
        http::async_read(server_.next_layer(), server_buffer_, received_,
                         beast::bind_front_handler(&exchange::http_received, this));
    }

    void client_ready(error_code ec) {
        if (mode_ == scenario::handshake_timeout) {
            BOOST_TEST(ec == beast::error::timeout);
            finish();
            return;
        }
        if (mode_ == scenario::wrong_host || mode_ == scenario::untrusted) {
            BOOST_TEST(ec);
            BOOST_TEST_NE(SSL_get_verify_result(client_.next_layer().native_handle()), X509_V_OK);
            finish();
            return;
        }
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        BOOST_TEST_EQ(SSL_get_verify_result(client_.next_layer().native_handle()), X509_V_OK);
        if (mode_ == scenario::websocket) {
            client_.async_handshake("localhost", "/echo",
                                    beast::bind_front_handler(&exchange::ws_client_ready, this));
            return;
        }
        request_.set(http::field::host, "localhost");
        request_.body() = payload;
        request_.prepare_payload();
        http::async_write(client_.next_layer(), request_,
                          beast::bind_front_handler(&exchange::http_sent, this));
    }

    void http_received(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (finished_ || !BOOST_TEST(!ec)) {
            return;
        }
        BOOST_TEST_EQ(received_.body(), payload);
        BOOST_TEST_EQ(received_.target(), "/echo");
        if (mode_ == scenario::cancel_read) {
            boost::asio::async_write(server_.next_layer(), boost::asio::buffer(partial),
                                     [](error_code write_error, std::size_t count) {
                                         BOOST_TEST(!write_error);
                                         BOOST_TEST_GT(count, 0U);
                                     });
            return;
        }
        response_.body() = received_.body();
        response_.prepare_payload();
        http::async_write(server_.next_layer(), response_,
                          beast::bind_front_handler(&exchange::http_answered, this));
    }

    void http_sent(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        if (mode_ == scenario::cancel_read) {
            read_partial();
            return;
        }
        http::async_read(client_.next_layer(), client_buffer_, parser_,
                         beast::bind_front_handler(&exchange::http_complete, this));
    }

    void read_partial() {
        http::async_read_some(client_.next_layer(), client_buffer_, parser_,
                              beast::bind_front_handler(&exchange::partial_received, this));
    }

    void partial_received(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        auto& response = parser_.get();
        const auto& body = response.body();
        if (body.size() < 4) {
            read_partial();
            return;
        }
        BOOST_TEST_EQ(body, "part");
        BOOST_TEST(!parser_.is_done());
        http::async_read(client_.next_layer(), client_buffer_, parser_,
                         [this](error_code read_error, std::size_t) {
                             BOOST_TEST(read_error == boost::asio::error::operation_aborted);
                             BOOST_TEST(!parser_.is_done());
                             finish();
                         });
        boost::asio::post(context_, [this] { beast::get_lowest_layer(client_).cancel(); });
    }

    void http_answered(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (finished_ || !BOOST_TEST(!ec)) {
            return;
        }
        if (mode_ == scenario::truncated_shutdown || mode_ == scenario::cancel_shutdown) {
            // Drain the client's close_notify first. Closing TCP with that
            // record unread can produce a reset instead of a truncated stream.
            // Cancelling before its write finishes can miss the pending read.
            server_.next_layer().async_read_some(
                boost::asio::buffer(hello_), [this](error_code read_error, std::size_t bytes) {
                    BOOST_TEST(read_error == boost::asio::error::eof);
                    BOOST_TEST_EQ(bytes, 0U);
                    BOOST_TEST_NE(static_cast<unsigned>(
                                      SSL_get_shutdown(server_.next_layer().native_handle())) &
                                      SSL_RECEIVED_SHUTDOWN,
                                  0);
                    if (mode_ == scenario::truncated_shutdown) {
                        close_checked(beast::get_lowest_layer(server_).socket());
                    } else {
                        beast::get_lowest_layer(client_).cancel();
                    }
                });
        } else {
            server_.next_layer().async_shutdown(
                beast::bind_front_handler(&exchange::shutdown_complete, this));
        }
    }

    void http_complete(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        BOOST_TEST(parser_.get().result() == http::status::ok);
        BOOST_TEST_EQ(parser_.get().body(), payload);
        client_.next_layer().async_shutdown(
            beast::bind_front_handler(&exchange::shutdown_complete, this));
    }

    void shutdown_complete(error_code ec) {
        if (finished_) {
            return;
        }
        if (mode_ == scenario::truncated_shutdown) {
            BOOST_TEST(ec == ssl::error::stream_truncated);
            finish();
        } else if (mode_ == scenario::cancel_shutdown) {
            BOOST_TEST(ec == boost::asio::error::operation_aborted);
            finish();
        } else {
            BOOST_TEST(!ec);
            if (++shutdowns_ == 2) {
                finish();
            }
        }
    }

    void ws_server_ready(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        server_.async_read(server_buffer_, beast::bind_front_handler(&exchange::ws_received, this));
    }

    void ws_client_ready(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        client_.binary(true);
        client_.async_write(boost::asio::buffer(payload),
                            beast::bind_front_handler(&exchange::ws_sent, this));
    }

    void ws_received(error_code ec, [[maybe_unused]] std::size_t bytes) {
        if (!BOOST_TEST(!ec)) {
            finish();
            return;
        }
        BOOST_TEST(server_.got_binary());
        BOOST_TEST_EQ(beast::buffers_to_string(server_buffer_.data()), payload);
        server_.binary(true);
        server_.async_write(server_buffer_.data(),
                            beast::bind_front_handler(&exchange::ws_echoed, this));
    }

    void ws_echoed(error_code ec, [[maybe_unused]] std::size_t bytes) {
        BOOST_TEST(!ec);
        server_buffer_.consume(server_buffer_.size());
        server_.async_read(server_buffer_, [this](error_code close_error, std::size_t) {
            BOOST_TEST(close_error == ws::error::closed);
            ws_closed(server_);
        });
    }

    void ws_sent(error_code ec, [[maybe_unused]] std::size_t bytes) {
        BOOST_TEST(!ec);
        client_.async_read(client_buffer_, beast::bind_front_handler(&exchange::ws_complete, this));
    }

    void ws_complete(error_code ec, [[maybe_unused]] std::size_t bytes) {
        BOOST_TEST(!ec);
        BOOST_TEST(client_.got_binary());
        BOOST_TEST_EQ(beast::buffers_to_string(client_buffer_.data()), payload);
        client_.async_close(ws::close_code::normal, [this](error_code close_error) {
            BOOST_TEST(!close_error);
            ws_closed(client_);
        });
    }

    void ws_closed(ws::stream<beast::ssl_stream<beast::tcp_stream>>& stream) {
        // WebSocket teardown already shuts down TLS and closes the transport.
        const auto state =
            static_cast<unsigned>(SSL_get_shutdown(stream.next_layer().native_handle()));
        BOOST_TEST_NE(state & SSL_SENT_SHUTDOWN, 0);
        BOOST_TEST_NE(state & SSL_RECEIVED_SHUTDOWN, 0);
        BOOST_TEST(!beast::get_lowest_layer(stream).socket().is_open());
        if (++shutdowns_ == 2) {
            finish();
        }
    }

public:
    exchange(scenario mode, std::string_view certificate, std::string_view key,
             std::string_view authorities, int version)
        : mode_(mode),
          server_context_(server_context(certificate, key, version)),
          client_context_(client_context(authorities, mode != scenario::untrusted)),
          server_(context_, server_context_),
          client_(context_, client_context_) {
        const char* host = mode_ == scenario::wrong_host ? "wrong.invalid" : "localhost";
        error_code ec;
        BOOST_TEST(
            !client_.next_layer().set_verify_callback(ssl::host_name_verification(host), ec));
        BOOST_TEST(!ec);
        BOOST_TEST(beast::set_server_name(client_.next_layer(), host).has_value());
    }

    void run() {
        deadline_.async_wait(beast::bind_front_handler(&exchange::expired, this));
        listener_.async_accept(beast::get_lowest_layer(server_).socket(),
                               beast::bind_front_handler(&exchange::accepted, this));
        beast::get_lowest_layer(client_).async_connect(
            listener_.local_endpoint(), beast::bind_front_handler(&exchange::connected, this));
        context_.run();  // lint-run: bounded real TLS peers share this test context
        BOOST_TEST(finished_);
    }
};
}  // namespace

int main(int argc, char* const* argv) {
    if (argc != 5) {
        (void)std::fputs("expected server certificate, private key, CA and TLS version\n", stderr);
        return 2;
    }
    const int version = std::string_view(argv[4]) == "1.2" ? TLS1_2_VERSION : TLS1_3_VERSION;
    for (const auto mode : {
             scenario::http,
             scenario::websocket,
             scenario::wrong_host,
             scenario::untrusted,
             scenario::cancel_read,
             scenario::truncated_shutdown,
             scenario::cancel_shutdown,
             scenario::handshake_timeout,
         }) {
        std::printf("Beast TLS %s scenario %d\n", argv[4], static_cast<int>(mode));
        exchange current(mode, argv[1], argv[2], argv[3], version);
        current.run();
    }
    return boost::report_errors();
}
