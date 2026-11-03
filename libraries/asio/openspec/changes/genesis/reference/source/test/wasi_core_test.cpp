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
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/core/lightweight_test.hpp>
#include <webcpp/test/exit_status.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace asio = boost::asio;
using tcp = webcpp::asio::ip::tcp;
using udp = webcpp::asio::ip::udp;
using namespace std::chrono_literals;

namespace {

void test_timers() {
    webcpp::asio::io_context context;
    bool posted = false;
    int expired = 0;
    int cancelled = 0;
    webcpp::asio::steady_timer timer(context, 20ms);
    webcpp::asio::steady_timer cancelled_timer(context, 1h);
    timer.async_wait([&](boost::system::error_code error) {
        BOOST_TEST_EQ(error, boost::system::error_code{});
        BOOST_TEST(posted);
        ++expired;
    });
    cancelled_timer.async_wait([&](boost::system::error_code error) {
        BOOST_TEST(error == asio::error::operation_aborted);
        ++cancelled;
    });
    asio::post(context, [&] {
        posted = true;
        BOOST_TEST_EQ(cancelled_timer.cancel(), 1U);
    });
    context.run();  // lint-run: test the application's event loop
    BOOST_TEST_EQ(expired, 1);
    BOOST_TEST_EQ(cancelled, 1);
}

void test_tcp() {
    webcpp::asio::io_context context;
    tcp::acceptor acceptor(context, {asio::ip::address_v4::loopback(), 0});
    tcp::socket server(context);
    tcp::socket client(context);
    const std::string message = "hello WASIp3";
    std::array<char, 12> server_data{};
    std::array<char, 12> client_data{};
    int exchanged = 0;
    acceptor.async_accept(server, [&](boost::system::error_code error) {
        BOOST_TEST_EQ(error, boost::system::error_code{});
        if (error) {
            return;
        }
        const std::array<asio::mutable_buffer, 3> input = {
            asio::mutable_buffer{},
            asio::buffer(server_data.data(), 6),
            asio::buffer(server_data.data() + 6, 6),
        };
        asio::async_read(
            server, input, [&](boost::system::error_code read_error, std::size_t size) {
                BOOST_TEST_EQ(read_error, boost::system::error_code{});
                BOOST_TEST_EQ(size, message.size());
                asio::async_write(server, asio::buffer(server_data),
                                  [&](boost::system::error_code write_error, std::size_t written) {
                                      BOOST_TEST_EQ(write_error, boost::system::error_code{});
                                      BOOST_TEST_EQ(written, message.size());
                                      ++exchanged;
                                  });
            });
    });
    const std::array<asio::const_buffer, 3> pieces = {
        asio::const_buffer{},
        asio::buffer(message.data(), 6),
        asio::buffer(message.data() + 6, 6),
    };
    client.async_connect(acceptor.local_endpoint(), [&](boost::system::error_code error) {
        BOOST_TEST_EQ(error, boost::system::error_code{});
        if (error) {
            return;
        }
        asio::async_write(
            client, pieces, [&](boost::system::error_code write_error, std::size_t size) {
                BOOST_TEST_EQ(write_error, boost::system::error_code{});
                BOOST_TEST_EQ(size, message.size());
                asio::async_read(
                    client, asio::buffer(client_data),
                    [&](boost::system::error_code read_error, std::size_t received) {
                        BOOST_TEST_EQ(read_error, boost::system::error_code{});
                        BOOST_TEST_EQ(received, message.size());
                        BOOST_TEST(std::string(client_data.data(), received) == message);
                        ++exchanged;
                    });
            });
    });
    context.run();  // lint-run: drive both peers without an external server
    BOOST_TEST_EQ(exchanged, 2);

#if defined(__wasip2__) || defined(__wasip3__)
    // A long empty prefix must not turn a stream operation into a no-op.
    context.restart();
    std::array<asio::const_buffer, 65> late_input{};
    std::array<asio::mutable_buffer, 65> late_output{};
    late_input.back() = asio::buffer("x", 1);
    late_output.back() = asio::buffer(server_data.data(), 1);
    int late_completions = 0;
    server.async_read_some(late_output, [&](boost::system::error_code error, std::size_t size) {
        BOOST_TEST_EQ(error, boost::system::error_code{});
        BOOST_TEST_EQ(size, 1U);
        BOOST_TEST_EQ(server_data[0], 'x');
        ++late_completions;
    });
    client.async_write_some(late_input, [&](boost::system::error_code error, std::size_t size) {
        BOOST_TEST_EQ(error, boost::system::error_code{});
        BOOST_TEST_EQ(size, 1U);
        ++late_completions;
    });
    context.run();  // lint-run: transfer data beyond an empty iovec prefix
    BOOST_TEST_EQ(late_completions, 2);
#endif

    context.restart();
    int eof = 0;
    server.shutdown(tcp::socket::shutdown_send);
    client.async_read_some(asio::buffer(client_data),
                           [&](boost::system::error_code error, std::size_t size) {
                               BOOST_TEST(error == asio::error::eof);
                               BOOST_TEST_EQ(size, 0U);
                               ++eof;
                           });
    context.run();  // lint-run: observe the peer's write shutdown
    BOOST_TEST_EQ(eof, 1);

    context.restart();
    int read_cancelled = 0;
    server.async_read_some(asio::buffer(server_data),
                           [&](boost::system::error_code error, std::size_t size) {
                               BOOST_TEST(error == asio::error::operation_aborted);
                               BOOST_TEST_EQ(size, 0U);
                               ++read_cancelled;
                           });
    asio::post(context, [&] { server.cancel(); });
    context.run();  // lint-run: complete TCP read cancellation
    BOOST_TEST_EQ(read_cancelled, 1);
}

void test_udp() {
    webcpp::asio::io_context context;
    const std::string message = "hello WASIp3";
    udp::socket receiver(context, {asio::ip::address_v4::loopback(), 0});
    udp::socket sender(context, udp::v4());
    udp::endpoint peer;
    std::array<char, 32> datagram{};
    int received_datagrams = 0;
    receiver.async_receive_from(asio::buffer(datagram), peer,
                                [&](boost::system::error_code error, std::size_t size) {
                                    BOOST_TEST_EQ(error, boost::system::error_code{});
                                    BOOST_TEST_EQ(size, message.size());
                                    BOOST_TEST(std::string(datagram.data(), size) == message);
                                    ++received_datagrams;
                                });
    sender.async_send_to(asio::buffer(message), receiver.local_endpoint(),
                         [&](boost::system::error_code error, std::size_t size) {
                             BOOST_TEST_EQ(error, boost::system::error_code{});
                             BOOST_TEST_EQ(size, message.size());
                         });
    context.run();  // lint-run: drive a local datagram exchange
    BOOST_TEST_EQ(received_datagrams, 1);
}

#ifdef WEBCPP_TEST_WASI_THREADS
void test_threads() {
#ifndef BOOST_ASIO_HAS_THREADS
#error "The threaded test must compile Asio with thread support"
#endif
    webcpp::asio::io_context context;
    auto work = asio::make_work_guard(context);
    int awakened = 0;
    int wake_cancelled = 0;
    webcpp::asio::steady_timer pending(context, 1h);
    pending.async_wait([&](boost::system::error_code error) {
        BOOST_TEST(error == asio::error::operation_aborted);
        ++wake_cancelled;
    });
    std::thread worker([&] {
        std::this_thread::sleep_for(20ms);
        asio::post(context, [&] {
            ++awakened;
            pending.cancel();
            work.reset();
        });
    });
    context.run();  // lint-run: wait for work posted by another thread
    worker.join();
    BOOST_TEST_EQ(awakened, 1);
    BOOST_TEST_EQ(wake_cancelled, 1);

    context.restart();
    std::atomic<int> completions{0};
    for (int i = 0; i < 32; ++i) {
        asio::post(context, [&] { ++completions; });
    }
    std::thread runner([&] {
        context.run();  // lint-run: share a context between two runners
    });
    context.run();  // lint-run: the application's second runner
    runner.join();
    BOOST_TEST_EQ(completions.load(), 32);
}
#endif

}  // namespace

int main() {
    test_timers();
    test_tcp();
    test_udp();
#ifdef WEBCPP_TEST_WASI_THREADS
    test_threads();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
