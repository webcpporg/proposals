// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <array>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../wasip2/counting_allocator.hpp"

namespace {
namespace asio = boost::asio;
using udp = webcpp::asio::ip::udp;
#if defined(__wasip2__) || defined(__wasip3__)
constexpr std::size_t buffer_count = 80;
#else
constexpr std::size_t buffer_count = 16;
#endif

void connected_send() {
    webcpp::asio::io_context context;
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, udp::v4());
    boost::system::error_code connection_error;
    BOOST_TEST_EQ(sender.connect(receiver.local_endpoint(), connection_error),
                  boost::system::error_code{});
    BOOST_TEST_EQ(connection_error, boost::system::error_code{});
    if (connection_error) {
        return;
    }
    std::array<asio::const_buffer, buffer_count> pieces{};
    pieces.fill(asio::buffer("x", 1));
    boost::system::error_code error;
    BOOST_TEST_EQ(sender.send(pieces, 0, error), buffer_count);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    if (!error) {
        std::array<char, 100> received{};
        BOOST_TEST_EQ(receiver.receive(asio::buffer(received)), buffer_count);
        BOOST_TEST(std::string(received.data(), buffer_count) == std::string(buffer_count, 'x'));
    }
    counts allocations;
    int completed = 0;
    sender.async_send(pieces,
                      asio::bind_allocator(allocator<void>(allocations),
                                           [&](boost::system::error_code ec, std::size_t size) {
                                               BOOST_TEST_EQ(allocations.live, 0U);
                                               BOOST_TEST_EQ(ec, boost::system::error_code{});
                                               BOOST_TEST_EQ(size, buffer_count);
                                               error = ec;
                                               ++completed;
                                           }));
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: finish the public connected datagram operation
    BOOST_TEST_EQ(completed, 1);
    if (!error) {
        std::array<char, 100> received{};
        BOOST_TEST_EQ(receiver.receive(asio::buffer(received)), buffer_count);
        BOOST_TEST(std::string(received.data(), buffer_count) == std::string(buffer_count, 'x'));
    }
}

void scatter_receive(bool connected, bool asynchronous) {
    counts context_allocations;
    webcpp::asio::io_context context(std::allocator_arg, allocator<void>(context_allocations));
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, {asio::ip::address_v4::loopback(), 0});
    if (connected) {
        boost::system::error_code connection_error;
        BOOST_TEST_EQ(receiver.connect(sender.local_endpoint(), connection_error),
                      boost::system::error_code{});
        BOOST_TEST_EQ(connection_error, boost::system::error_code{});
        if (connection_error) {
            return;
        }
    }
    std::array<char, buffer_count> bytes{};
    std::array<asio::mutable_buffer, buffer_count> pieces{};
    for (std::size_t index = 0; index != pieces.size(); ++index) {
        pieces[index] = asio::buffer(&bytes[index], 1);
    }
    const std::string payload(100, 's');
    sender.send_to(asio::buffer(payload), receiver.local_endpoint());
    udp::endpoint peer;
    boost::system::error_code error;
    if (asynchronous) {
        counts allocations;
        int completed = 0;
        auto handler = asio::bind_allocator(allocator<void>(allocations),
                                            [&](boost::system::error_code ec, std::size_t size) {
                                                BOOST_TEST_EQ(allocations.live, 0U);
                                                BOOST_TEST_EQ(ec, boost::system::error_code{});
                                                BOOST_TEST_EQ(size, buffer_count);
                                                error = ec;
                                                ++completed;
                                            });
        if (connected) {
            receiver.async_receive(pieces, handler);
        } else {
            receiver.async_receive_from(pieces, peer, handler);
        }
        BOOST_TEST_EQ(completed, 0);
        context.run();  // lint-run: finish the public scattered datagram receive
        BOOST_TEST_EQ(completed, 1);
    } else {
        [[maybe_unused]] const auto before = context_allocations.allocations;
        auto const size = connected ? receiver.receive(pieces, 0, error)
                                    : receiver.receive_from(pieces, peer, 0, error);
        BOOST_TEST_EQ(error, boost::system::error_code{});
        BOOST_TEST_EQ(size, buffer_count);
#if defined(__wasip2__) || defined(__wasip3__)
        BOOST_TEST_EQ(context_allocations.allocations, before + 1);
#endif
    }
    if (!error) {
        BOOST_TEST(std::string(bytes.data(), bytes.size()) == std::string(buffer_count, 's'));
        if (!connected) {
            BOOST_TEST(peer == sender.local_endpoint());
        }
        receiver.non_blocking(true);
        BOOST_TEST_EQ(receiver.receive(asio::buffer(bytes), 0, error), 0U);
        BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
    }
}

void cancel_scatter() {
    webcpp::asio::io_context context;
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    std::array<char, 2> bytes{};
    const std::array<asio::mutable_buffer, 2> pieces{
        asio::buffer(bytes.data(), 1),
        asio::buffer(&bytes[1], 1),
    };
    asio::cancellation_signal cancellation;
    counts allocations;
    int completed = 0;
    receiver.async_receive(
        pieces, asio::bind_allocator(allocator<void>(allocations),
                                     asio::bind_cancellation_slot(
                                         cancellation.slot(),
                                         [&](boost::system::error_code error, std::size_t size) {
                                             BOOST_TEST_EQ(allocations.live, 0U);
                                             BOOST_TEST_EQ(error, asio::error::operation_aborted);
                                             BOOST_TEST_EQ(size, 0U);
                                             ++completed;
                                         })));
    cancellation.emit(asio::cancellation_type::all);
    context.run();  // lint-run: deliver cancellation of the pending scattered receive
    BOOST_TEST_EQ(completed, 1);
}

void datagram_boundaries() {
    counts context_counts;
    webcpp::asio::io_context context(std::allocator_arg, allocator<void>(context_counts));
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, udp::v4());
    sender.connect(receiver.local_endpoint());
    const std::array<asio::const_buffer, 3> empty{};
    std::array<char, 8> received{};
    boost::system::error_code error;
    BOOST_TEST_EQ(sender.send(empty, 0, error), 0U);
    BOOST_TEST(!error);
    udp::endpoint peer;
#if defined(__wasip2__) || defined(__wasip3__)
    std::array<asio::mutable_buffer, 3> const no_capacity{};
    BOOST_TEST_EQ(receiver.receive_from(no_capacity, peer, 0, error), 0U);
#else
    // macOS needs positive capacity to consume an empty datagram.
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(received), peer, 0, error), 0U);
#endif
    BOOST_TEST(!error);
    sender.send(asio::buffer("discard", 7));
#if defined(__wasip2__) || defined(__wasip3__)
    BOOST_TEST_EQ(receiver.receive_from(no_capacity, peer, 0, error), 0U);
#else
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(received), peer, 0, error), 7U);
#endif
    BOOST_TEST(!error);
    sender.send(asio::buffer("next", 4));
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(received), peer), 4U);
    BOOST_TEST(std::string(received.data(), 4) == "next");

    const std::string too_large(65536, 'x');
    const std::array<asio::const_buffer, 2> oversized{
        asio::buffer(too_large.data(), 65535),
        asio::buffer(too_large.data() + 65535, 1),
    };
    const auto before = context_counts.allocations;
    BOOST_TEST_EQ(sender.send(oversized, 0, error), 0U);
    BOOST_TEST_EQ(error, asio::error::message_size);
    BOOST_TEST_EQ(context_counts.allocations, before);
    int rejected = 0;
    sender.async_send(oversized, [&](boost::system::error_code ec, std::size_t size) {
#ifdef BOOST_ASIO_HAS_IOCP
        // Upstream forwards the IOCP error, observed as ERROR_INVALID_USER_BUFFER for this send.
        const boost::system::error_code invalid_buffer(ERROR_INVALID_USER_BUFFER,
                                                       boost::system::system_category());
        BOOST_TEST(ec == asio::error::message_size || ec == invalid_buffer);
#else
        BOOST_TEST_EQ(ec, asio::error::message_size);
#endif
        BOOST_TEST_EQ(size, 0U);
        ++rejected;
    });
    BOOST_TEST_EQ(rejected, 0);
    context.run();  // lint-run: oversized connected sends fail through deferred completion
    BOOST_TEST_EQ(rejected, 1);
    receiver.non_blocking(true);
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(received), peer, 0, error), 0U);
    BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
}

[[maybe_unused]] void asynchronous_empty_buffers() {
    webcpp::asio::io_context context;
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, {asio::ip::address_v4::loopback(), 0});
    sender.connect(receiver.local_endpoint());
    receiver.connect(sender.local_endpoint());
    const std::array<asio::const_buffer, 3> empty{};
    std::array<asio::mutable_buffer, 3> const no_capacity{};
    int completed = 0;
    sender.async_send(empty, [&](boost::system::error_code error, std::size_t size) {
        BOOST_TEST(!error);
        BOOST_TEST_EQ(size, 0U);
        ++completed;
    });
    receiver.async_receive(no_capacity, [&](boost::system::error_code error, std::size_t size) {
        BOOST_TEST(!error);
        BOOST_TEST_EQ(size, 0U);
        ++completed;
    });
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: empty buffers still send and consume a complete datagram
    BOOST_TEST_EQ(completed, 2);
    context.restart();
    sender.send(asio::buffer("discard", 7));
    udp::endpoint peer;
    receiver.async_receive_from(no_capacity, peer,
                                [&](boost::system::error_code error, std::size_t size) {
                                    BOOST_TEST(!error);
                                    BOOST_TEST_EQ(size, 0U);
                                    BOOST_TEST(peer == sender.local_endpoint());
                                    ++completed;
                                });
    context.run();  // lint-run: a zero-capacity scattered receive discards the datagram remainder
    BOOST_TEST_EQ(completed, 3);
    sender.send(asio::buffer("next", 4));
    std::array<char, 8> bytes{};
    BOOST_TEST_EQ(receiver.receive(asio::buffer(bytes)), 4U);
    BOOST_TEST(std::string(bytes.data(), 4) == "next");
}

void abandoned_receive() {
    counts allocations;
    bool completed = false;
    std::array<char, 2> bytes{};
    const std::array<asio::mutable_buffer, 2> pieces{
        asio::buffer(bytes.data(), 1),
        asio::buffer(&bytes[1], 1),
    };
    {
        webcpp::asio::io_context context;
        udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
        receiver.async_receive(
            pieces, asio::bind_allocator(
                        allocator<void>(allocations),
                        [&](boost::system::error_code, std::size_t) { completed = true; }));
        BOOST_TEST(allocations.live != 0U);
    }
    BOOST_TEST(!completed);
    BOOST_TEST_EQ(allocations.live, 0U);
}

}  // namespace

int main() {
    connected_send();
    for (bool const connected : {false, true}) {
        for (bool const asynchronous : {false, true}) {
            scatter_receive(connected, asynchronous);
        }
    }
    cancel_scatter();
    datagram_boundaries();
    abandoned_receive();
#if defined(__wasip2__) || defined(__wasip3__)
    asynchronous_empty_buffers();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
