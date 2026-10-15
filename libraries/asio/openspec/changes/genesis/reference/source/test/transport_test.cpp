// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/asio/deferred.hpp>
#include <boost/core/lightweight_test.hpp>
#include <chrono>
#include <memory>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/steady_timer.hpp>
#include <webcpp/test/exit_status.hpp>

namespace {
struct test_protocol {
    using endpoint = boost::asio::ip::basic_endpoint<test_protocol>;

    static test_protocol v4() { return {}; }

    static test_protocol v6() { return {true}; }

    [[nodiscard]] int family() const { return ipv6 ? AF_INET6 : AF_INET; }

    static int type() { return SOCK_STREAM; }

    static int protocol() { return IPPROTO_TCP; }

    bool ipv6 = false;
};

// The observation belongs to this context, not to process-wide mutable state.
struct observations : boost::asio::execution_context::service {
    // Asio identifies each service type through its static id.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,bugprone-throwing-static-initialization,cert-err58-cpp)
    static inline boost::asio::execution_context::id id;

    explicit observations(boost::asio::execution_context& context) : service(context) {}

    test_protocol::endpoint peer;
    unsigned calls = 0;
    unsigned closes = 0;
    bool delay = false;

private:
    void shutdown() override {}
};

}  // namespace

namespace webcpp::asio::browser {
template <>
class transport<test_protocol> {
public:
    explicit transport(const boost::asio::any_io_executor& executor)
        : executor_(executor),
          seen_(boost::asio::use_service<observations>(
              boost::asio::query(executor, boost::asio::execution::context))) {}

    boost::system::error_code connect(const test_protocol::endpoint& peer) {
        seen_.peer = peer;
        ++seen_.calls;
        return {};
    }

    template <class Handler>
    void async_connect(const test_protocol::endpoint& peer, Handler&& handler) {
        if (seen_.delay) {
            timer_ = std::make_unique<webcpp::asio::steady_timer>(executor_, std::chrono::hours(1));
            timer_->async_wait(std::forward<Handler>(handler));
            return;
        }
        const auto ec = connect(peer);
        webcpp::asio::detail::post_completion(
            executor_, boost::asio::append(std::forward<Handler>(handler), ec));
    }

    boost::system::error_code close() {
        ++seen_.closes;
        timer_.reset();
        return {};
    }

private:
    std::unique_ptr<webcpp::asio::steady_timer> timer_;
    boost::asio::any_io_executor executor_;
    observations& seen_;
};
}  // namespace webcpp::asio::browser

int main() {
    webcpp::asio::io_context io;
    webcpp::asio::basic_stream_socket<test_protocol> socket(io);
    auto& seen = boost::asio::use_service<observations>(io);
    const test_protocol::endpoint peer(boost::asio::ip::address_v4::loopback(), 8080);
    boost::system::error_code ec;
    BOOST_TEST(socket.connect(peer, ec) == ec);
    BOOST_TEST(!ec);
    BOOST_TEST(seen.peer == peer);
    BOOST_TEST_EQ(seen.calls, 1U);
    bool returned = false;
    unsigned completed = 0;
    auto handler = [&](boost::system::error_code result) {
        BOOST_TEST(returned);
        BOOST_TEST(!result);
        ++completed;
    };
    socket.async_connect(peer, handler);
    auto operation = socket.async_connect(peer, boost::asio::deferred);
    std::move(operation)(handler);
    BOOST_TEST_EQ(completed, 0U);
    returned = true;
    io.run();  // lint-run: the test drives its queued completions
    BOOST_TEST_EQ(completed, 2U);
    BOOST_TEST_EQ(seen.calls, 3U);
    BOOST_TEST(seen.peer == peer);
    io.restart();
    seen.delay = true;
    BOOST_TEST(socket.open(test_protocol::v4(), ec) == ec);
    BOOST_TEST(!ec);
    socket.async_connect(peer, [&](boost::system::error_code result) {
        BOOST_TEST(result == boost::asio::error::operation_aborted);
        ++completed;
    });
    BOOST_TEST(socket.close(ec) == ec);
    BOOST_TEST(!ec);
    BOOST_TEST(!socket.is_open());
    BOOST_TEST_EQ(seen.closes, 1U);
    io.poll();
    BOOST_TEST_EQ(completed, 3U);
    return webcpp::test::exit_status(boost::report_errors());
}
