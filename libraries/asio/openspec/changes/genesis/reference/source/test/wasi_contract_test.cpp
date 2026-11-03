// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/test/exit_status.hpp>

#include <array>
#include <chrono>
#include <string>

namespace asio = boost::asio;
using tcp = webcpp::asio::ip::tcp;
using udp = webcpp::asio::ip::udp;

int main() {
    webcpp::asio::io_context context;
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, udp::v4());
    std::array<char, 8> data{};
    udp::endpoint peer;
    boost::system::error_code error;

    // Empty datagrams are messages and must reach the receiver.
    int empty_sent = 0;
    int empty_received = 0;
    receiver.async_receive_from(asio::buffer(data), peer,
                                [&](boost::system::error_code ec, std::size_t size) {
                                    BOOST_TEST_EQ(ec, boost::system::error_code{});
                                    BOOST_TEST_EQ(size, 0U);
                                    ++empty_received;
                                });
    sender.async_send_to(asio::const_buffer{}, receiver.local_endpoint(),
                         [&](boost::system::error_code ec, std::size_t size) {
                             BOOST_TEST_EQ(ec, boost::system::error_code{});
                             if (ec) {
                                 receiver.cancel();
                             }
                             BOOST_TEST_EQ(size, 0U);
                             ++empty_sent;
                         });
    context.run();  // lint-run: deliver a zero-length datagram
    BOOST_TEST_EQ(empty_sent, 1);
    BOOST_TEST_EQ(empty_received, 1);

    std::array<asio::const_buffer, 2> pieces{asio::buffer("a", 1), asio::buffer("b", 1)};
    const auto sent = sender.send_to(pieces, receiver.local_endpoint(), 0, error);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(sent, 2U);
#if defined(__wasip2__) || defined(__wasip3__)
    // A refused vectored receive must not consume even the first byte.
    const std::array<asio::mutable_buffer, 2> outputs{
        asio::buffer(data.data(), 1),
        asio::buffer(data.data() + 1, 1),
    };
    BOOST_TEST_EQ(receiver.receive_from(outputs, peer, 0, error), 0U);
    BOOST_TEST_EQ(error, asio::error::operation_not_supported);
#endif
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 2U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST(std::string(data.data(), 2) == "ab");

    // Container buffers containing only one nonempty buffer remain supported.
    pieces = {asio::const_buffer{}, asio::buffer("z", 1)};
    BOOST_TEST_EQ(sender.send_to(pieces, receiver.local_endpoint(), 0, error), 1U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 1U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(data[0], 'z');

#if defined(__wasip2__) || defined(__wasip3__)
    // Empty buffers must not hide payload beyond Asio's native iovec limit.
    std::array<asio::const_buffer, 65> distant{};
    distant.front() = asio::buffer("a", 1);
    distant.back() = asio::buffer("b", 1);
    BOOST_TEST_EQ(sender.send_to(distant, receiver.local_endpoint(), 0, error), 2U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 2U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST(std::string(data.data(), 2) == "ab");
    distant.front() = asio::const_buffer{};
    BOOST_TEST_EQ(sender.send_to(distant, receiver.local_endpoint(), 0, error), 1U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 1U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(data[0], 'b');

    BOOST_TEST_EQ(sender.send_to(asio::buffer("ab", 2), receiver.local_endpoint()), 2U);
    std::array<asio::mutable_buffer, 65> distant_outputs{};
    distant_outputs.front() = asio::buffer(data.data(), 1);
    distant_outputs.back() = asio::buffer(data.data() + 1, 1);
    BOOST_TEST_EQ(receiver.receive_from(distant_outputs, peer, 0, error), 0U);
    BOOST_TEST_EQ(error, asio::error::operation_not_supported);
    if (error == asio::error::operation_not_supported) {
        BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer), 2U);
        BOOST_TEST(std::string(data.data(), 2) == "ab");
    }
#endif

    // Both connected scalar and vectored empty operations keep UDP semantics.
    sender.connect(receiver.local_endpoint());
    BOOST_TEST_EQ(sender.send(asio::const_buffer{}, 0, error), 0U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
#if defined(__wasip2__) || defined(__wasip3__)
    BOOST_TEST_EQ(receiver.receive_from(asio::mutable_buffer{}, peer, 0, error), 0U);
#else
    // macOS may return from a zero-capacity receive before consuming a packet.
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 0U);
#endif
    BOOST_TEST_EQ(error, boost::system::error_code{});
    const std::array<asio::const_buffer, 2> empty_inputs{};
#if defined(__wasip2__) || defined(__wasip3__)
    const std::array<asio::mutable_buffer, 2> empty_outputs{};
#endif
    BOOST_TEST_EQ(sender.send(empty_inputs, 0, error), 0U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
#if defined(__wasip2__) || defined(__wasip3__)
    BOOST_TEST_EQ(receiver.receive_from(empty_outputs, peer, 0, error), 0U);
#else
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 0U);
#endif
    BOOST_TEST_EQ(error, boost::system::error_code{});

#if defined(__wasip2__) || defined(__wasip3__)
    receiver.non_blocking(true);
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 0U);
    BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
    receiver.non_blocking(false);
#endif

    // Use an independent socket so prior traffic cannot satisfy this receive.
    udp::socket cancel_receiver(context, {asio::ip::address_v4::loopback(), 0});
    // Preserve asynchronous cancellation with no incoming datagram.
    context.restart();
    int cancelled = 0;
    cancel_receiver.async_receive_from(asio::buffer(data), peer,
                                       [&](boost::system::error_code ec, std::size_t size) {
                                           BOOST_TEST_EQ(ec, asio::error::operation_aborted);
                                           BOOST_TEST_EQ(size, 0U);
                                           ++cancelled;
                                       });
    asio::post(context, [&] { cancel_receiver.cancel(); });
    context.run();  // lint-run: complete cancellation through the executor
    BOOST_TEST_EQ(cancelled, 1);

#if defined(__wasip2__) || defined(__wasip3__)
    tcp::socket socket(context, tcp::v4());
    BOOST_TEST_EQ(socket.set_option(asio::socket_base::linger(true, 1), error),
                  asio::error::operation_not_supported);
    BOOST_TEST_EQ(error, asio::error::operation_not_supported);
    BOOST_TEST(!socket.at_mark(error));
    BOOST_TEST_EQ(error, asio::error::operation_not_supported);
    const auto numeric = asio::ip::make_address_v6("fe80::1%12", error);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(numeric.scope_id(), 12U);
    BOOST_TEST(numeric.to_string() == "fe80::1%12");
    static_cast<void>(asio::ip::make_address_v6("fe80::1%eth0", error));
    BOOST_TEST_EQ(error, asio::error::invalid_argument);
#endif

    context.restart();
    tcp::resolver resolver(context);
    const auto synchronous = resolver.resolve(
        "127.0.0.1", "80",
        asio::ip::resolver_base::numeric_host | asio::ip::resolver_base::numeric_service, error);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST(!synchronous.empty());
    int resolved = 0;
    resolver.async_resolve(
        "localhost", "80",
        [&](boost::system::error_code ec, const tcp::resolver::results_type& addresses) {
#if defined(__wasip3__) && !defined(BOOST_ASIO_HAS_THREADS)
            BOOST_TEST_EQ(ec, asio::error::operation_not_supported);
            BOOST_TEST(addresses.empty());
#else
            BOOST_TEST_EQ(ec, boost::system::error_code{});
            BOOST_TEST(!addresses.empty());
#endif
            ++resolved;
        });
    BOOST_TEST_EQ(resolved, 0);
    context.run();  // lint-run: resolve through the configured executor
    BOOST_TEST_EQ(resolved, 1);
    return webcpp::test::exit_status(boost::report_errors());
}
