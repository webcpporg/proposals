// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <type_traits>

namespace beast = webcpp::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = webcpp::asio::ip::tcp;
using boost::system::error_code;
using namespace std::chrono_literals;

static_assert(std::is_same_v<beast::tcp_stream, boost::beast::tcp_stream>);
static_assert(std::is_base_of_v<boost::beast::websocket::stream<beast::tcp_stream>,
                                websocket::stream<beast::tcp_stream>>);

namespace {

class default_socket : public tcp::socket {
    static webcpp::asio::io_context& context() {
        static webcpp::asio::io_context instance;
        return instance;
    }

public:
    default_socket() : tcp::socket(context()) {}
};

void test_constructors() {
    // Instantiate the constructor bodies: upstream's variadic constructor can make
    // is_default_constructible report true even for a non-defaultable next layer.
    websocket::stream<default_socket> original;
    const websocket::stream<default_socket> moved(std::move(original));
    BOOST_TEST(!moved.is_open());
    const std::allocator<std::byte> allocator;
    const websocket::stream<default_socket> allocated(std::allocator_arg, allocator);
    BOOST_TEST(!allocated.is_open());

    webcpp::asio::io_context context;
    using precise_socket =
        boost::asio::basic_stream_socket<tcp, boost::asio::io_context::executor_type>;
    using precise_stream = websocket::stream<precise_socket>;
    using rebound = precise_stream::rebind_executor<boost::asio::any_io_executor>::other;
    static_assert(std::is_same_v<rebound, websocket::stream<tcp::socket>>);
    precise_stream precise(context);
    const rebound converted(std::move(precise));
    BOOST_TEST(!converted.is_open());
}

class http_exchange {
    webcpp::asio::io_context context_;
    tcp::acceptor listener_{context_, {boost::asio::ip::address_v4::loopback(), 0}};
    beast::tcp_stream client_{context_};
    beast::tcp_stream server_{context_};
    beast::flat_buffer input_;
    beast::flat_buffer output_;
    const std::string payload{"hello\0webcpp", 12};
    http::request<http::string_body> request_{http::verb::post, "/echo", 11};
    http::request<http::string_body> received_;
    http::response<http::string_body> answer_{http::status::ok, 11};
    http::response<http::string_body> response_;
    bool completed_ = false;
    webcpp::asio::steady_timer deadline_{context_, 5s};

    void expired(error_code ec) {
        if (!ec) {
            BOOST_ERROR("HTTP exchange exceeded its deadline");
            client_.cancel();
            server_.cancel();
            BOOST_TEST(!listener_.close(ec));
        }
    }

    void accepted(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        http::async_read(server_, input_, received_,
                         beast::bind_front_handler(&http_exchange::received, this));
    }

    void received(error_code ec, [[maybe_unused]] std::size_t transferred) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        BOOST_TEST_EQ(received_.target(), "/echo");
        BOOST_TEST(received_.method() == http::verb::post);
        BOOST_TEST_EQ(received_.body(), payload);
        answer_.set(http::field::content_type, "application/octet-stream");
        answer_.body() = received_.body();
        answer_.prepare_payload();
        http::async_write(server_, answer_,
                          [](error_code write_error, std::size_t) { BOOST_TEST(!write_error); });
    }

    void connected(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        http::async_write(client_, request_,
                          beast::bind_front_handler(&http_exchange::written, this));
    }

    void written(error_code ec, [[maybe_unused]] std::size_t transferred) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        http::async_read(client_, output_, response_,
                         beast::bind_front_handler(&http_exchange::responded, this));
    }

    void responded(error_code ec, [[maybe_unused]] std::size_t transferred) {
        BOOST_TEST(!ec);
        BOOST_TEST(response_.result() == http::status::ok);
        BOOST_TEST_EQ(response_.body(), payload);
        completed_ = !ec;
        deadline_.cancel();
    }

public:
    void run() {
        request_.set(http::field::host, "localhost");
        request_.body() = payload;
        request_.prepare_payload();
        deadline_.async_wait(beast::bind_front_handler(&http_exchange::expired, this));
        listener_.async_accept(server_.socket(),
                               beast::bind_front_handler(&http_exchange::accepted, this));
        client_.async_connect(listener_.local_endpoint(),
                              beast::bind_front_handler(&http_exchange::connected, this));
        context_.run();  // lint-run: drive the real client and server exchange
        BOOST_TEST(completed_);
    }
};

class websocket_exchange {
    webcpp::asio::io_context context_;
    tcp::acceptor listener_{context_, {boost::asio::ip::address_v4::loopback(), 0}};
    websocket::stream<beast::tcp_stream> server_{context_};
    websocket::stream<beast::tcp_stream> client_{std::allocator_arg, std::allocator<std::byte>{},
                                                 context_};
    beast::flat_buffer server_data_;
    beast::flat_buffer client_data_;
    const std::string payload{"binary\0message", 14};
    bool echoed_ = false;
    bool client_closed_ = false;
    bool server_closed_ = false;
    webcpp::asio::steady_timer deadline_{context_, 5s};

    void expired(error_code ec) {
        if (!ec) {
            BOOST_ERROR("WebSocket exchange exceeded its deadline");
            beast::get_lowest_layer(client_).cancel();
            beast::get_lowest_layer(server_).cancel();
            BOOST_TEST(!listener_.close(ec));
        }
    }

    void accepted(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        server_.async_accept(beast::bind_front_handler(&websocket_exchange::server_ready, this));
    }

    void server_ready(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        server_.async_read(server_data_,
                           beast::bind_front_handler(&websocket_exchange::server_received, this));
    }

    void server_received(error_code ec, [[maybe_unused]] std::size_t transferred) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        BOOST_TEST(server_.got_binary());
        server_.binary(true);
        server_.async_write(server_data_.data(),
                            beast::bind_front_handler(&websocket_exchange::server_written, this));
    }

    void server_written(error_code ec, [[maybe_unused]] std::size_t transferred) {
        BOOST_TEST(!ec);
        server_data_.consume(server_data_.size());
        server_.async_read(server_data_,
                           beast::bind_front_handler(&websocket_exchange::server_closed, this));
    }

    void server_closed(error_code ec, [[maybe_unused]] std::size_t transferred) {
        BOOST_TEST(ec == websocket::error::closed);
        server_closed_ = ec == websocket::error::closed;
        if (client_closed_) {
            deadline_.cancel();
        }
    }

    void connected(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        client_.async_handshake("localhost", "/echo",
                                beast::bind_front_handler(&websocket_exchange::client_ready, this));
    }

    void client_ready(error_code ec) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        client_.binary(true);
        client_.async_write(boost::asio::buffer(payload),
                            beast::bind_front_handler(&websocket_exchange::client_written, this));
    }

    void client_written(error_code ec, [[maybe_unused]] std::size_t transferred) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        client_.async_read(client_data_,
                           beast::bind_front_handler(&websocket_exchange::client_received, this));
    }

    void client_received(error_code ec, [[maybe_unused]] std::size_t transferred) {
        BOOST_TEST(!ec);
        BOOST_TEST_EQ(beast::buffers_to_string(client_data_.data()), payload);
        BOOST_TEST(client_.got_binary());
        echoed_ = !ec;
        client_.async_close(websocket::close_code::normal,
                            beast::bind_front_handler(&websocket_exchange::client_closed, this));
    }

    void client_closed(error_code ec) {
        BOOST_TEST(!ec);
        client_closed_ = !ec;
        if (server_closed_) {
            deadline_.cancel();
        }
    }

public:
    void run() {
        deadline_.async_wait(beast::bind_front_handler(&websocket_exchange::expired, this));
        listener_.async_accept(beast::get_lowest_layer(server_).socket(),
                               beast::bind_front_handler(&websocket_exchange::accepted, this));
        beast::get_lowest_layer(client_).async_connect(
            listener_.local_endpoint(),
            beast::bind_front_handler(&websocket_exchange::connected, this));
        context_.run();  // lint-run: drive handshake, echo and the closing handshake
        BOOST_TEST(echoed_);
        BOOST_TEST(client_closed_);
        BOOST_TEST(server_closed_);
    }
};

void test_http() {
    http_exchange exchange;
    exchange.run();
}

void test_websocket() {
    websocket_exchange exchange;
    exchange.run();
}

void test_timeout() {
    webcpp::asio::io_context context;
    tcp::acceptor listener(context, {boost::asio::ip::address_v4::loopback(), 0});
    tcp::socket peer(context);
    beast::tcp_stream client(context);
    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    bool timed_out = false;
    webcpp::asio::steady_timer deadline(context, 5s);
    deadline.async_wait([&](error_code ec) {
        if (!ec) {
            BOOST_ERROR("Stream timeout did not terminate the pending read");
            client.cancel();
            BOOST_TEST(!listener.close(ec));
        }
    });
    listener.async_accept(peer, [](error_code ec) { BOOST_TEST(!ec); });
    client.async_connect(listener.local_endpoint(), [&](error_code ec) {
        BOOST_TEST(!ec);
        if (ec) {
            return;
        }
        client.expires_after(20ms);
        http::async_read(client, buffer, response, [&](error_code read_error, std::size_t) {
            BOOST_TEST(read_error == beast::error::timeout);
            timed_out = read_error == beast::error::timeout;
            deadline.cancel();
        });
    });
    context.run();  // lint-run: prove the configured I/O deadline terminates a stalled peer
    BOOST_TEST(timed_out);
}

}  // namespace

int main() {
    test_constructors();
    test_http();
    test_websocket();
    test_timeout();
    return boost::report_errors();
}
