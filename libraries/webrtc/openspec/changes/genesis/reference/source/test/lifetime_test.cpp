// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Lifetime against libdatachannel's threads (design.md, D7 and D9): a connection destroyed while
// its peer floods it, again and again; and a channel flooded past libdatachannel's queue of 1,024
// messages, never read, whose destruction releases the thread that waits for room (spec:
// "Lifetime: native destruction is safe against foreign threads", "Testing: native lifetime").

#include <webcpp/webrtc.hpp>

#include "pair.hpp"
#include "require.hpp"

#include <webcpp/asio/buffer.hpp>        // lint-world: messages
#include <webcpp/asio/steady_timer.hpp>  // lint-world: lets a flood fill the queue

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace webrtc = webcpp::webrtc;
namespace asio = webcpp::asio;
using webrtc::detail::error_code;

#ifndef WEBCPP_TEST_WEBRTC_REPEAT
#define WEBCPP_TEST_WEBRTC_REPEAT 10
#endif

namespace {

// Sends small messages until the channel refuses one, then waits for room and goes on, until it
// fails otherwise.
void flood(webrtc::data_channel& channel, std::size_t& sent) {
    static constexpr std::array<char, 64> message{};
    for (;;) {
        const auto ec = channel.send(asio::buffer(message));
        if (ec == webrtc::error::buffered_amount_exceeded) {
            channel.async_wait_buffered_amount_low([&channel, &sent](error_code ec) {
                if (!ec) {
                    flood(channel, sent);
                }
            });
            return;
        }
        if (ec) {
            return;
        }
        ++sent;
    }
}

// The answering peer and its channel are destroyed while the offering peer floods them; then the
// offering peer closes. Repeated, under ThreadSanitizer in its lane.
void destroyed_while_messages_arrive() {
    for (int round = 0; round < WEBCPP_TEST_WEBRTC_REPEAT; ++round) {
        webrtc::options opts;
        opts.max_buffered_amount = std::size_t{64} * 1024;
        webcpp::test::pair p(opts);
        std::optional<webrtc::data_channel> ca;
        std::optional<webrtc::data_channel> cb;
        std::size_t sent = 0;
        std::size_t read = 0;
        std::string text;
        auto buffer = asio::dynamic_buffer(text);
        std::function<void()> receive = [&] {
            cb->async_receive(buffer, [&](error_code ec, std::size_t n, bool) {
                if (ec) {
                    return;
                }
                buffer.consume(n);
                if (++read == 50) {
                    // Mid-flood: the answering side goes, its channel first.
                    cb.reset();
                    p.b.reset();
                    asio::post(p.io, [&] { p.a->close(); });
                    return;
                }
                receive();
            });
        };
        webcpp::test::open_channel(p, "flood", [&](webrtc::data_channel a, webrtc::data_channel b) {
            ca.emplace(std::move(a));
            cb.emplace(std::move(b));
            ca->set_buffered_amount_low_threshold(std::size_t{16} * 1024);
            flood(*ca, sent);
            receive();
        });
        p.run();
        BOOST_TEST_GE(read, 50u);
        BOOST_TEST_GT(sent, 50u);
    }
}

// A channel flooded past libdatachannel's 1,024 unread messages is destroyed promptly: draining
// its queue releases the thread that waits for room.
void a_flooded_channel_is_destroyed_promptly() {
    webcpp::test::pair p;
    std::optional<webrtc::data_channel> ca;
    std::optional<webrtc::data_channel> cb;
    asio::steady_timer filled(p.io);  // lint-world: lets the flood fill the queue
    std::size_t sent = 0;
    std::chrono::steady_clock::duration took{};  // lint-world: measures the destruction
    webcpp::test::open_channel(
        p, "never-read", [&](webrtc::data_channel a, webrtc::data_channel b) {
            ca.emplace(std::move(a));
            cb.emplace(std::move(b));
            static constexpr std::array<char, 64> message{};
            for (int i = 0; i < 3000; ++i) {
                if (ca->send(asio::buffer(message))) {
                    break;
                }
                ++sent;
            }
            filled.expires_after(std::chrono::seconds(1));
            filled.async_wait([&](error_code) {
                const auto start = std::chrono::steady_clock::now();  // lint-world: measures it
                cb.reset();
                took = std::chrono::steady_clock::now() - start;  // lint-world: measures it
                p.a->close();
                p.b->close();
            });
        });
    p.run();
    BOOST_TEST_EQ(sent, 3000u);
    BOOST_TEST(took < std::chrono::seconds(2));
}

}  // namespace

int main() {
    destroyed_while_messages_arrive();
    a_flooded_channel_is_destroyed_promptly();
    return boost::report_errors();
}
