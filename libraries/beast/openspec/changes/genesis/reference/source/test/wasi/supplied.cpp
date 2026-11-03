// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
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
        const auto count = std::min(bytes.size(), channel.input.size() - channel.offset);
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
}  // namespace

int main() {
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
    return error == boost::system::errc::operation_canceled ? 0 : 1;
}
