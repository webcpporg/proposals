// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The measurement of tasks.md 1.10: a native channel that is never read, flooded past
// libdatachannel's queue of 1,024 messages, stalls the connection's other channels, since the
// thread that waits for room in its queue is the one that delivers the association's messages;
// destroying it drains its queue and releases them (design.md, D9).

#include <webcpp/webrtc.hpp>

#include "pair.hpp"
#include "require.hpp"

#include <webcpp/asio/buffer.hpp>        // lint-world: messages
#include <webcpp/asio/steady_timer.hpp>  // lint-world: bounds each wait of the measurement

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace webrtc = webcpp::webrtc;
namespace asio = webcpp::asio;
using webrtc::detail::error_code;

namespace {

// Opens "x" and "y", floods "x" without reading it, then sends one message on "y" and waits for
// it, three seconds; if it does not arrive, destroys the far end of "x" and waits again.
class stall {
public:
    void run() {
        webcpp::test::open_channel(p_, "x", [this](webrtc::data_channel a, webrtc::data_channel b) {
            x_local_.emplace(std::move(a));
            x_remote_.emplace(std::move(b));
            open_y();
        });
        p_.run();
    }

    void check() const {
        // Measured on 2026-11-14 with libdatachannel 0.24.6: the connection's other channel
        // stalls while the never-read one is full, and destroying that one releases it. A change
        // of this finding changes the documentation's (design.md, D9).
        BOOST_TEST_EQ(sent_, 3000u);
        BOOST_TEST(stalled_ == std::optional<bool>(true));
        BOOST_TEST(released_ == std::optional<bool>(true));
    }

private:
    void open_y() {
        auto y = p_.a->create_data_channel("y");
        webcpp::test::require(BOOST_TEST(y.has_value()));
        y_local_.emplace(std::move(*y));
        p_.b->async_accept_data_channel([this](error_code ec, webrtc::data_channel c) {
            webcpp::test::require(BOOST_TEST(!ec));
            y_remote_.emplace(std::move(c));
            y_local_.value().async_wait_open([this](error_code ec) {
                webcpp::test::require(BOOST_TEST(!ec));
                flood_x();
            });
        });
    }

    void flood_x() {
        static constexpr std::array<char, 64> message{};
        for (int i = 0; i < 3000 && !x_local_.value().send(asio::buffer(message)); ++i) {
            ++sent_;
        }
        timer_.expires_after(std::chrono::seconds(1));
        timer_.async_wait([this](error_code) { send_on_y(); });
    }

    void send_on_y() {
        BOOST_TEST(!y_local_.value().send_text("y"));
        y_remote_.value().async_receive(buffer_, [this](error_code ec, std::size_t, bool) {
            if (stalled_) {
                released_ = !ec;
            } else {
                stalled_ = false;
                timer_.cancel();
            }
            p_.a->close();
            p_.b->close();
        });
        timer_.expires_after(std::chrono::seconds(3));
        timer_.async_wait([this](error_code ec) {
            if (ec) {
                return;
            }
            stalled_ = true;
            x_remote_.reset();
        });
    }

    webcpp::test::pair p_;
    std::optional<webrtc::data_channel> x_local_;
    std::optional<webrtc::data_channel> x_remote_;
    std::optional<webrtc::data_channel> y_local_;
    std::optional<webrtc::data_channel> y_remote_;
    asio::steady_timer timer_{p_.io};  // lint-world: bounds each wait of the measurement
    std::string text_;
    decltype(asio::dynamic_buffer(text_)) buffer_ = asio::dynamic_buffer(text_);
    std::optional<bool> stalled_;
    std::optional<bool> released_;
    std::size_t sent_ = 0;
};

}  // namespace

int main() {
    stall measurement;
    measurement.run();
    measurement.check();
    return boost::report_errors();
}
