// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <boost/asio/bind_allocator.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cerrno>
#include <string_view>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/test/exit_status.hpp>
#include "../wasip2/counting_allocator.hpp"

namespace {
struct test_state {
    int sending_socket = -1;
    int attempts = 0;
};

test_state& state() {
    static test_state value;
    return value;
}
}  // namespace

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" ssize_t __real_send(int, const void*, size_t, int);

// The linker --wrap protocol requires this exact native boundary symbol.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" ssize_t __wrap_send(int socket, const void* data, size_t size, int flags) {
    if (socket == state().sending_socket && ++state().attempts == 1) {
        errno = EAGAIN;
        return -1;
    }
    return __real_send(socket, data, size, flags);
}

int main() {
    namespace asio = boost::asio;
    using udp = webcpp::asio::ip::udp;
    for (bool const gather : {false, true}) {
        webcpp::asio::io_context context;
        udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
        udp::socket sender(context, udp::v4());
        sender.connect(receiver.local_endpoint());
        state().sending_socket = sender.native_handle();
        state().attempts = 0;
        counts allocations;
        int completed = 0;
        int received = 0;
        std::array<char, 8> data{};
        udp::endpoint peer;
        receiver.async_receive_from(asio::buffer(data), peer,
                                    [&](boost::system::error_code error, std::size_t size) {
                                        BOOST_TEST(!error);
                                        BOOST_TEST_EQ(size, 2U);
                                        BOOST_TEST(std::string_view(data.data(), 2) == "ab");
                                        ++received;
                                    });
        auto handler = asio::bind_allocator(allocator<void>(allocations),
                                            [&](boost::system::error_code error, std::size_t size) {
                                                BOOST_TEST(!error);
                                                BOOST_TEST_EQ(size, 2U);
                                                BOOST_TEST_EQ(allocations.live, 0U);
                                                ++completed;
                                            });
        const std::array<asio::const_buffer, 2> pieces{asio::buffer("a", 1), asio::buffer("b", 1)};
        if (gather) {
            sender.async_send(pieces, handler);
        } else {
            sender.async_send(asio::buffer("ab", 2), handler);
        }
        BOOST_TEST_EQ(completed, 0);
        context.run();  // lint-run: retry the transient native send after real write readiness
        BOOST_TEST_EQ(completed, 1);
        BOOST_TEST(state().attempts >= 2);
        BOOST_TEST_EQ(received, 1);
        receiver.non_blocking(true);
        boost::system::error_code error;
        BOOST_TEST_EQ(receiver.receive_from(asio::buffer(data), peer, 0, error), 0U);
        BOOST_TEST(error == asio::error::would_block || error == asio::error::try_again);
        state().sending_socket = -1;
    }
    return webcpp::test::exit_status(boost::report_errors());
}
