// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Browser socket operations fail as values unless a transport supplies them (doc: #asio-sockets).
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/system/system_error.hpp>
#include <utility>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/test/exit_status.hpp>

namespace {
namespace net = webcpp::asio;
using boost::system::error_code;
const auto unsupported = boost::asio::error::operation_not_supported;

template <class Start>
void posted(net::io_context& io, Start start) {
    int called = 0;
    bool initiated = false;
    auto check_value = [](const auto& value) {
        if constexpr (requires { value.is_open(); }) {
            BOOST_TEST(!value.is_open());
        } else {
            BOOST_TEST_EQ(value, 0U);
        }
    };
    auto handler = [&](error_code ec, const auto&... values) {
        (check_value(values), ...);
        BOOST_TEST(initiated);
        BOOST_TEST(ec == unsupported);
        ++called;
    };
    start(handler);
    BOOST_TEST_EQ(called, 0);
    initiated = true;
    io.restart();
    io.run();  // lint-run: drain the callback completion
    BOOST_TEST_EQ(called, 1);
    initiated = false;
    auto pending = start(boost::asio::deferred);
    std::move(pending)(handler);
    BOOST_TEST_EQ(called, 1);
    initiated = true;
    io.restart();
    io.run();  // lint-run: drain the deferred completion
    BOOST_TEST_EQ(called, 2);
}
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
template <class Call>
void throws_unsupported(Call call) {
    bool caught = false;
    try {
        call();
    } catch (const boost::system::system_error& error) {
        caught = error.code() == unsupported;
    }
    BOOST_TEST(caught);
}
#endif

template <class Socket, class Protocol>
void common(Socket& socket, Protocol protocol) {
    error_code ec;
    BOOST_TEST(!socket.is_open());
    BOOST_TEST(socket.open(protocol, ec) == ec);
    BOOST_TEST(!ec);
    BOOST_TEST(socket.is_open());
    BOOST_TEST(socket.open(protocol, ec) == ec);
    BOOST_TEST(ec == boost::asio::error::already_open);
    boost::asio::socket_base::reuse_address option;
    BOOST_TEST(socket.cancel(ec) == ec);
    BOOST_TEST(ec == unsupported);
    BOOST_TEST(socket.set_option(option, ec) == ec);
    BOOST_TEST(ec == unsupported);
    BOOST_TEST(socket.get_option(option, ec) == ec);
    BOOST_TEST(ec == unsupported);
    socket.local_endpoint(ec);
    BOOST_TEST(ec == unsupported);
    BOOST_TEST(socket.non_blocking(true, ec) == ec);
    BOOST_TEST(ec == unsupported);
    BOOST_TEST(!socket.non_blocking());
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { socket.cancel(); });
    throws_unsupported([&] { socket.set_option(option); });
    throws_unsupported([&] { socket.get_option(option); });
    throws_unsupported([&] { (void)socket.local_endpoint(); });
    throws_unsupported([&] { socket.non_blocking(true); });
#endif
    BOOST_TEST(socket.close(ec) == ec);
    BOOST_TEST(!ec);
    BOOST_TEST(!socket.is_open());
    BOOST_TEST(socket.close(ec) == ec);
    BOOST_TEST(!ec);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    socket.open(protocol);
    BOOST_TEST(socket.is_open());
    socket.close();
    BOOST_TEST(!socket.is_open());
#endif
}

void all_socket_operations() {
    net::io_context io;
    net::ip::tcp::socket tcp(io);
    net::ip::udp::socket udp(io.get_executor());
    net::ip::tcp::acceptor acceptor(io);
    common(tcp, net::ip::tcp::v4());
    common(udp, net::ip::udp::v6());
    common(acceptor, net::ip::tcp::v4());
    std::array<char, 8> bytes{};
    const auto buffer = boost::asio::buffer(bytes);
    const net::ip::tcp::endpoint tcp_endpoint(boost::asio::ip::address_v4::loopback(), 80);
    error_code ec;
    BOOST_TEST(tcp.connect(tcp_endpoint, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { tcp.connect(tcp_endpoint); });
#endif
    BOOST_TEST(tcp.shutdown(boost::asio::socket_base::shutdown_both, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { tcp.shutdown(boost::asio::socket_base::shutdown_both); });
#endif
    tcp.read_some(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.read_some(buffer); });
#endif
    tcp.write_some(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.write_some(buffer); });
#endif
    tcp.send(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.send(buffer); });
#endif
    tcp.send(buffer, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.send(buffer, 0); });
#endif
    tcp.receive(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.receive(buffer); });
#endif
    tcp.receive(buffer, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.receive(buffer, 0); });
#endif
    tcp.remote_endpoint(ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.remote_endpoint(); });
#endif
    tcp.available(ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)tcp.available(); });
#endif
}

void synchronous_datagram_operations() {
    net::io_context io;
    net::ip::udp::socket udp(io);
    std::array<char, 8> bytes{};
    const auto buffer = boost::asio::buffer(bytes);
    net::ip::udp::endpoint udp_endpoint(boost::asio::ip::address_v4::loopback(), 53);
    error_code ec;
    BOOST_TEST(udp.connect(udp_endpoint, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { udp.connect(udp_endpoint); });
#endif
    BOOST_TEST(udp.bind(udp_endpoint, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { udp.bind(udp_endpoint); });
#endif
    BOOST_TEST(udp.shutdown(boost::asio::socket_base::shutdown_both, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { udp.shutdown(boost::asio::socket_base::shutdown_both); });
#endif
    udp.send(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.send(buffer); });
#endif
    udp.send(buffer, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.send(buffer, 0); });
#endif
    udp.receive(buffer, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.receive(buffer); });
#endif
    udp.receive(buffer, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.receive(buffer, 0); });
#endif
    udp.send_to(buffer, udp_endpoint, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.send_to(buffer, udp_endpoint); });
#endif
    udp.send_to(buffer, udp_endpoint, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.send_to(buffer, udp_endpoint, 0); });
#endif
    udp.receive_from(buffer, udp_endpoint, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.receive_from(buffer, udp_endpoint); });
#endif
    udp.receive_from(buffer, udp_endpoint, 0, ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.receive_from(buffer, udp_endpoint, 0); });
#endif
    udp.remote_endpoint(ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.remote_endpoint(); });
#endif
    udp.available(ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)udp.available(); });
#endif
}

void synchronous_acceptor_operations() {
    net::io_context io;
    net::ip::tcp::acceptor acceptor(io);
    net::ip::tcp::socket peer(io);
    const net::ip::tcp::endpoint tcp_endpoint(boost::asio::ip::address_v4::loopback(), 80);
    error_code ec;
    BOOST_TEST(acceptor.bind(tcp_endpoint, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { acceptor.bind(tcp_endpoint); });
#endif
    BOOST_TEST(acceptor.listen(ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { acceptor.listen(); });
#endif
    BOOST_TEST(acceptor.listen(4, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { acceptor.listen(4); });
#endif
    BOOST_TEST(acceptor.accept(peer, ec) == ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { acceptor.accept(peer); });
#endif
    acceptor.accept(ec);
    BOOST_TEST(ec == unsupported);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    throws_unsupported([&] { (void)acceptor.accept(); });
#endif
}

void asynchronous_socket_operations() {
    net::io_context io;
    net::ip::tcp::socket tcp(io);
    net::ip::udp::socket udp(io);
    net::ip::tcp::acceptor acceptor(io);
    net::ip::tcp::socket peer(io);
    std::array<char, 8> bytes{};
    const auto buffer = boost::asio::buffer(bytes);
    const net::ip::tcp::endpoint tcp_endpoint(boost::asio::ip::address_v4::loopback(), 80);
    net::ip::udp::endpoint udp_endpoint(boost::asio::ip::address_v4::loopback(), 53);
    posted(io, [&](auto token) { return tcp.async_connect(tcp_endpoint, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_read_some(buffer, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_write_some(buffer, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_send(buffer, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_send(buffer, 0, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_receive(buffer, std::move(token)); });
    posted(io, [&](auto token) { return tcp.async_receive(buffer, 0, std::move(token)); });
    posted(io, [&](auto token) { return udp.async_connect(udp_endpoint, std::move(token)); });
    posted(io, [&](auto token) { return udp.async_send(buffer, std::move(token)); });
    posted(io, [&](auto token) { return udp.async_send(buffer, 0, std::move(token)); });
    posted(io, [&](auto token) { return udp.async_receive(buffer, std::move(token)); });
    posted(io, [&](auto token) { return udp.async_receive(buffer, 0, std::move(token)); });
    posted(io,
           [&](auto token) { return udp.async_send_to(buffer, udp_endpoint, std::move(token)); });
    posted(io, [&](auto token) {
        return udp.async_send_to(buffer, udp_endpoint, 0, std::move(token));
    });
    posted(io, [&](auto token) {
        return udp.async_receive_from(buffer, udp_endpoint, std::move(token));
    });
    posted(io, [&](auto token) {
        return udp.async_receive_from(buffer, udp_endpoint, 0, std::move(token));
    });
    posted(io, [&](auto token) { return acceptor.async_accept(peer, std::move(token)); });
    posted(io, [&](auto token) { return acceptor.async_accept(std::move(token)); });
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    net::ip::tcp::socket opened(io, net::ip::tcp::v4());
    BOOST_TEST(opened.is_open());
    const net::ip::tcp::socket moved(std::move(opened));
    BOOST_TEST(moved.is_open());
    // The moved-from state is part of the socket contract.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    BOOST_TEST(!opened.is_open());
    throws_unsupported([&] { const net::ip::tcp::socket bound(io, tcp_endpoint); });
    throws_unsupported([&] { const net::ip::udp::socket bound(io, udp_endpoint); });
    throws_unsupported([&] { const net::ip::tcp::acceptor bound(io, tcp_endpoint); });
#endif
}

// Reusing the moved-from object is part of the socket contract.
// NOLINTBEGIN(bugprone-use-after-move)
template <class Socket, class Protocol, class Start>
void moved_sources_remain_usable(Protocol protocol, Start start) {
    net::io_context io;
    Socket source(io);
    error_code ec;
    source.open(protocol, ec);
    BOOST_TEST(!ec);
    Socket destination(std::move(source));
    BOOST_TEST(!source.is_open());
    source.open(protocol, ec);
    BOOST_TEST(!ec);
    posted(io, [&](auto token) { return start(source, std::move(token)); });
    destination = std::move(source);
    BOOST_TEST(!source.is_open());
    source.open(protocol, ec);
    BOOST_TEST(!ec);
    posted(io, [&](auto token) { return start(source, std::move(token)); });
}

// NOLINTEND(bugprone-use-after-move)

void moved_sockets_keep_their_transport() {
    moved_sources_remain_usable<net::ip::tcp::socket>(
        net::ip::tcp::v4(),
        [](auto& socket, auto token) { return socket.async_connect({}, std::move(token)); });
    moved_sources_remain_usable<net::ip::udp::socket>(
        net::ip::udp::v4(),
        [](auto& socket, auto token) { return socket.async_connect({}, std::move(token)); });
    moved_sources_remain_usable<net::ip::tcp::acceptor>(
        net::ip::tcp::v4(),
        [](auto& socket, auto token) { return socket.async_accept(std::move(token)); });
}

}  // namespace

int main() {
    moved_sockets_keep_their_transport();
    all_socket_operations();
    synchronous_datagram_operations();
    synchronous_acceptor_operations();
    asynchronous_socket_operations();
    return webcpp::test::exit_status(boost::report_errors());
}
