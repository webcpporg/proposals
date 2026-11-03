// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <boost/core/lightweight_test.hpp>
#include <cstdlib>
#include <string>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/wasi/stream.hpp>

namespace {
struct memory_channel {
    std::string input =
        "POST /one HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc"
        "GET /two HTTP/1.1\r\n\r\n";
    std::string output;
    std::size_t offset = 0;
    std::size_t waits = 0;
    std::size_t read_limit = 4096;
    bool blocked = true;

    static std::size_t read(void* context, std::span<char> bytes,
                            boost::system::error_code& error) {
        auto& channel = *static_cast<memory_channel*>(context);
        if (channel.blocked) {
            channel.blocked = false;
            error =
                boost::system::errc::make_error_code(boost::system::errc::operation_would_block);
            return 0;
        }
        error.clear();
        const auto count =
            std::min({bytes.size(), channel.input.size() - channel.offset, channel.read_limit});
        std::copy_n(channel.input.data() + channel.offset, count, bytes.data());
        channel.offset += count;
        return count;
    }

    static std::size_t write(void* context, std::span<const char> bytes,
                             boost::system::error_code& error) {
        auto& channel = *static_cast<memory_channel*>(context);
        const auto count = std::min<std::size_t>(bytes.size(), 3);
        channel.output.append(bytes.data(), count);
        error.clear();
        return count;
    }

    static void wait(void* context, bool /*writing*/,
                     const webcpp::beast::wasi::stream_control& control,
                     boost::system::error_code& error) {
        ++static_cast<memory_channel*>(context)->waits;
        error = control.check();
    }
};

void bounded_reads() {
    namespace beast = webcpp::beast;
    namespace http = beast::http;
    namespace wasi = beast::wasi;
    for (const std::size_t fragment : {7U, 256U}) {
        memory_channel state;
        state.read_limit = fragment;
        wasi::supplied_stream channel(&state, memory_channel::read, memory_channel::write,
                                      memory_channel::wait);
        beast::flat_static_buffer<256> buffer;
        boost::system::error_code error;
        http::request_parser<http::string_body> first;
        wasi::read(channel, buffer, first, error);
        BOOST_TEST(!error);
        BOOST_TEST(first.is_done());
        BOOST_TEST_EQ(first.get().body(), "abc");
        if (fragment == 256) {
            BOOST_TEST_EQ(beast::buffers_to_string(buffer.data()), "GET /two HTTP/1.1\r\n\r\n");
        }
        http::request_parser<http::empty_body> second;
        wasi::read(channel, buffer, second, error);
        BOOST_TEST(!error);
        BOOST_TEST(second.is_done());
        BOOST_TEST_EQ(second.get().target(), "/two");
    }
    memory_channel state;
    wasi::supplied_stream channel(&state, memory_channel::read, memory_channel::write,
                                  memory_channel::wait);
    beast::flat_buffer limited(256);
    boost::system::error_code error;
    http::request_parser<http::string_body> fitting;
    wasi::read(channel, limited, fitting, error);
    BOOST_TEST(!error);
    BOOST_TEST_EQ(fitting.get().body(), "abc");

    state.offset = 0;
    state.input = "GET /" + std::string(512, 'a') + " HTTP/1.1\r\n\r\n";
    beast::flat_static_buffer<32> exhausted;
    http::request_parser<http::empty_body> oversized;
    wasi::read(channel, exhausted, oversized, error);
    BOOST_TEST(error == http::error::buffer_overflow);
    BOOST_TEST(!oversized.is_done());
    BOOST_TEST_EQ(state.offset, 32U);
}
}  // namespace

int main() {
    bounded_reads();
    namespace http = webcpp::beast::http;
    namespace wasi = webcpp::beast::wasi;
    memory_channel state;
    wasi::supplied_stream channel(&state, memory_channel::read, memory_channel::write,
                                  memory_channel::wait);
    webcpp::beast::flat_buffer buffer;
    boost::system::error_code error;
    http::request_parser<http::string_body> first;
    wasi::read(channel, buffer, first, error);
    if (error || first.get().target() != "/one" || first.get().body() != "abc" ||
        state.waits != 1 || buffer.size() == 0) {
        std::abort();
    }
    http::request_parser<http::empty_body> second;
    wasi::read(channel, buffer, second, error);
    if (error || second.get().target() != "/two") {
        std::abort();
    }
    http::response<http::string_body> response{http::status::ok, 11};
    response.body() = "partial writes";
    response.prepare_payload();
    const auto count = wasi::write(channel, response, error);
    if (error || count != state.output.size() || !state.output.ends_with("partial writes")) {
        std::abort();
    }
    wasi::stream_control control;
    control.cancelled = [](void*) { return true; };
    channel.control(control);
    channel.write_some(boost::asio::buffer("x", 1), error);
    BOOST_TEST(error == boost::system::errc::operation_canceled);
    return boost::report_errors();
}
