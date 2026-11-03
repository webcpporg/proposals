// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_EXAMPLE_SUPPLIED_WEBSOCKET_CHANNEL_HPP
#define WEBCPP_BEAST_EXAMPLE_SUPPLIED_WEBSOCKET_CHANNEL_HPP

#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <webcpp/beast/wasi/stream.hpp>

#ifdef __wasip2__
#include <boost/asio/append.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/post.hpp>
#include <webcpp/asio/wasi/stdio_stream.hpp>

namespace webcpp::asio::wasi {
inline void teardown(boost::beast::role_type /*unused*/, stdio_stream& /*unused*/,
                     boost::system::error_code& error) {
    error.clear();
}

struct teardown_operation {
    template <class Self>
    void operator()(Self& self) {
        boost::asio::post(boost::asio::append(std::move(self), boost::system::error_code{}));
    }

    template <class Self>
    void operator()(Self& self, boost::system::error_code error) {
        self.complete(error);
    }
};

template <class Handler>
void async_teardown(boost::beast::role_type /*role*/, stdio_stream& stream, Handler&& handler) {
    boost::asio::async_compose<Handler, void(boost::system::error_code)>(
        teardown_operation{}, handler, stream.get_executor());
}

}  // namespace webcpp::asio::wasi

namespace supplied_example {
using channel = webcpp::asio::wasi::stdio_stream;
}
#else
namespace supplied_example {
// The executor exists for the upstream WebSocket object's associated state.
// This example performs explicit synchronous I/O on caller-provided CLI streams.
class channel {
    boost::asio::io_context::executor_type executor_;
    webcpp::beast::wasi::stdio_stream stream_;

public:
    using executor_type = boost::asio::io_context::executor_type;

    explicit channel(executor_type executor) : executor_(std::move(executor)) {}

    executor_type get_executor() noexcept { return executor_; }

    template <class Buffers>
    std::size_t read_some(const Buffers& buffers, boost::system::error_code& error) {
        const auto count = stream_.read_some(buffers, error);
        if (error == webcpp::beast::http::error::end_of_stream) {
            error = boost::asio::error::eof;
        }
        return count;
    }

    template <class Buffers>
    std::size_t write_some(const Buffers& buffers, boost::system::error_code& error) {
        return stream_.write_some(buffers, error);
    }

    template <class Buffers>
    std::size_t read_some(const Buffers& buffers) {
        boost::system::error_code error;
        const auto count = read_some(buffers, error);
        if (error) {
            boost::throw_exception(boost::system::system_error(error));
        }
        return count;
    }

    template <class Buffers>
    std::size_t write_some(const Buffers& buffers) {
        boost::system::error_code error;
        const auto count = write_some(buffers, error);
        if (error) {
            boost::throw_exception(boost::system::system_error(error));
        }
        return count;
    }
};

// The channel is borrowed: WebSocket close completes its handshake without
// inventing socket shutdown semantics or closing the caller's descriptors.
inline void teardown(boost::beast::role_type /*role*/, channel& /*stream*/,
                     boost::system::error_code& error) {
    error.clear();
}
}  // namespace supplied_example
#endif  // __wasip2__
#endif
