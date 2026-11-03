// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/udp.hpp>

#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/test/exit_status.hpp>

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace {
namespace asio = boost::asio;
using udp = webcpp::asio::ip::udp;

struct allocation_counts {
    std::size_t calls = 0;
    std::size_t live = 0;
};

template <class T>
struct counting_allocator {
    using value_type = T;
    allocation_counts* counts;

    explicit counting_allocator(allocation_counts& state) : counts(&state) {}

    template <class U>
    explicit counting_allocator(const counting_allocator<U>& other) noexcept
        : counts(other.counts) {}

    T* allocate(std::size_t n) {
        T* result = std::allocator<T>{}.allocate(n);
        ++counts->calls;
        ++counts->live;
        return result;
    }

    void deallocate(T* pointer, std::size_t n) noexcept {
        --counts->live;
        std::allocator<T>{}.deallocate(pointer, n);
    }

    template <class U>
    bool operator==(const counting_allocator<U>& other) const noexcept {
        return counts == other.counts;
    }
};

// Wait for delivery after a successful send; the WASI launcher bounds the test.
void expect_packet(udp::socket& receiver, std::string_view expected) {
    std::array<char, 1024> bytes{};
    udp::endpoint peer;
    boost::system::error_code error;
    receiver.non_blocking(false);
    const auto received = receiver.receive_from(asio::buffer(bytes), peer, 0, error);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(received, expected.size());
    BOOST_TEST(std::string_view(bytes.data(), received) == expected);
    receiver.non_blocking(true);
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(bytes), peer, 0, error), 0U);
    BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
}

template <class Buffers>
void check_sends(webcpp::asio::io_context& context, udp::socket& sender, udp::socket& receiver,
                 const Buffers& buffers, std::string_view expected) {
    boost::system::error_code error;
    BOOST_TEST_EQ(sender.send_to(buffers, receiver.local_endpoint(), 0, error), expected.size());
    BOOST_TEST_EQ(error, boost::system::error_code{});
    if (!error) {
        expect_packet(receiver, expected);
    }

    allocation_counts handler_allocations;
    int completions = 0;
    context.restart();
    sender.async_send_to(buffers, receiver.local_endpoint(), 0,
                         asio::bind_allocator(counting_allocator<char>(handler_allocations),
                                              [&](boost::system::error_code ec, std::size_t size) {
                                                  BOOST_TEST_EQ(ec, boost::system::error_code{});
                                                  BOOST_TEST_EQ(size, expected.size());
                                                  BOOST_TEST_EQ(handler_allocations.live, 0U);
                                                  error = ec;
                                                  ++completions;
                                              }));
    BOOST_TEST_EQ(completions, 0);
    context.run();  // lint-run: complete the UDP send and release its owned storage
    BOOST_TEST_EQ(completions, 1);
    BOOST_TEST_EQ(handler_allocations.live, 0U);
    if (!error) {
        expect_packet(receiver, expected);
    }
}

void test_gather() {
    allocation_counts context_allocations;
    webcpp::asio::io_context context(std::allocator_arg,
                                     counting_allocator<char>(context_allocations));
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, udp::v4());
    receiver.non_blocking(true);

    const std::string header = "HEADER:";
    const std::string body = "Hello";
    const std::array<asio::const_buffer, 4> pieces{
        asio::const_buffer{},
        asio::buffer(header),
        asio::const_buffer{},
        asio::buffer(body),
    };
    check_sends(context, sender, receiver, pieces, "HEADER:Hello");

#ifdef __wasip3__
    // Gather the original sequence, before Asio's 64-entry iovec adapter.
    std::array<asio::const_buffer, 80> many{};
    many.fill(asio::buffer("x", 1));
    check_sends(context, sender, receiver, many, std::string(80, 'x'));
    many.fill(asio::const_buffer{});
    many.front() = asio::buffer(header);
    many.back() = asio::buffer(body);
    check_sends(context, sender, receiver, many, "HEADER:Hello");
#endif

    const std::array<asio::const_buffer, 3> single{
        asio::const_buffer{},
        asio::buffer(body),
        asio::const_buffer{},
    };
    check_sends(context, sender, receiver, single, "Hello");
    const std::array<asio::const_buffer, 3> empty{};
    check_sends(context, sender, receiver, empty, "");

#ifdef __wasip3__
    boost::system::error_code error;
    const auto before = context_allocations.calls;
    BOOST_TEST_EQ(sender.send_to(pieces, receiver.local_endpoint(), 0, error), 12U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(context_allocations.calls, before + 1);
    if (!error) {
        expect_packet(receiver, "HEADER:Hello");
    }
    const auto after = context_allocations.calls;
    BOOST_TEST_EQ(sender.send_to(single, receiver.local_endpoint(), 0, error), body.size());
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(sender.send_to(empty, receiver.local_endpoint(), 0, error), 0U);
    BOOST_TEST_EQ(error, boost::system::error_code{});
    BOOST_TEST_EQ(context_allocations.calls, after);
    // Drain these two messages separately; expect_packet checks for an empty queue.
    std::array<char, 16> bytes{};
    udp::endpoint peer;
    receiver.non_blocking(false);
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(bytes), peer, 0, error), body.size());
    BOOST_TEST_EQ(error, boost::system::error_code{});
    expect_packet(receiver, "");

    allocation_counts handler_allocations;
    context.restart();
    int completed = 0;
    const auto context_before_async = context_allocations.calls;
    sender.async_send_to(pieces, receiver.local_endpoint(),
                         asio::bind_allocator(counting_allocator<char>(handler_allocations),
                                              [&](boost::system::error_code ec, std::size_t size) {
                                                  BOOST_TEST_EQ(ec, boost::system::error_code{});
                                                  BOOST_TEST_EQ(size, 12U);
                                                  BOOST_TEST_EQ(handler_allocations.live, 0U);
                                                  error = ec;
                                                  ++completed;
                                              }));
    // One allocation owns the operation; a second owns the gathered bytes.
    BOOST_TEST_EQ(handler_allocations.calls, 2U);
    BOOST_TEST_EQ(context_allocations.calls, context_before_async);
    BOOST_TEST_EQ(completed, 0);
    context.run();  // lint-run: check associated-allocator ownership through completion
    BOOST_TEST_EQ(completed, 1);
    if (!error) {
        expect_packet(receiver, "HEADER:Hello");
    }

    // Reject an oversized datagram before allocating or reading its buffers.
    const std::string large(65536, 'x');
    const std::array<asio::const_buffer, 2> oversized{
        asio::buffer(large.data(), 65535),
        asio::buffer(large.data() + 65535, 1),
    };
    const auto before_oversized = context_allocations.calls;
    BOOST_TEST_EQ(sender.send_to(oversized, receiver.local_endpoint(), 0, error), 0U);
    BOOST_TEST_EQ(error, asio::error::message_size);
    BOOST_TEST_EQ(context_allocations.calls, before_oversized);
    context.restart();
    int rejected = 0;
    sender.async_send_to(oversized, receiver.local_endpoint(),
                         [&](boost::system::error_code ec, std::size_t size) {
                             BOOST_TEST_EQ(ec, asio::error::message_size);
                             BOOST_TEST_EQ(size, 0U);
                             ++rejected;
                         });
    BOOST_TEST_EQ(rejected, 0);
    context.run();  // lint-run: report invalid input through normal asynchronous completion
    BOOST_TEST_EQ(rejected, 1);
    receiver.non_blocking(true);
    BOOST_TEST_EQ(receiver.receive_from(asio::buffer(bytes), peer, 0, error), 0U);
    BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
#endif
}

#ifdef __wasip3__
void test_shutdown() {
    allocation_counts allocations;
    int completions = 0;
    {
        webcpp::asio::io_context context;
        const udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
        udp::socket sender(context, udp::v4());
        const std::array<asio::const_buffer, 2> pieces{asio::buffer("a", 1), asio::buffer("b", 1)};
        sender.async_send_to(
            pieces, receiver.local_endpoint(),
            asio::bind_allocator(counting_allocator<char>(allocations),
                                 [&](boost::system::error_code, std::size_t) { ++completions; }));
        BOOST_TEST_EQ(allocations.live, 2U);
        // Destroy the context without running the queued completion handler.
    }
    BOOST_TEST_EQ(completions, 0);
    BOOST_TEST_EQ(allocations.live, 0U);
}
#endif
}  // namespace

int main() {
    test_gather();
#ifdef __wasip3__
    test_shutdown();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
