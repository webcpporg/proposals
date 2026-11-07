// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_HTTP_BYTE_STREAM_HPP
#define WEBCPP_BEAST_BROWSER_HTTP_BYTE_STREAM_HPP

#include <webcpp/beast/config.hpp>

#include <array>
#include <boost/asio/compose.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/beast/core/flat_buffer.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/http/parser.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/http/serializer.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <charconv>
#include <optional>
#include <tuple>
#include <webcpp/beast/browser/http.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace browser {
/**
 Explicit synthetic HTTP/1.1 adapter for upstream parser and serializer algorithms.

  Fetch owns real framing. This adapter decodes an outgoing serialized request and
  exposes a synthetic chunked response. HEAD, 204 and 304 have no body framing.
  The adapter and its next layer must outlive every outstanding operation.
 @tparam NextLayer Underlying browser authority or TLS-intent layer.
 */
template <detail::browser::host_stream NextLayer>
class http_byte_stream {
    using allocator_type = std::pmr::polymorphic_allocator<char>;
    using body_type =
        boost::beast::http::basic_string_body<char, std::char_traits<char>, allocator_type>;
    using fields_type = boost::beast::http::basic_fields<allocator_type>;
    NextLayer& next_;
    std::optional<boost::beast::http::request_parser<body_type, allocator_type>> request_;
    boost::beast::http::response<body_type, fields_type> response_;
    boost::beast::basic_flat_buffer<allocator_type> unused_;
    std::pmr::string wire_;
    std::size_t offset_ = 0;
    bool head_ = false;
    bool response_pending_ = false;

    void frame(boost::system::error_code& ec) {
        namespace http = boost::beast::http;
        wire_.clear();
        offset_ = 0;
        const bool bodyless =
            head_ || response_.result_int() == 204 || response_.result_int() == 304;
        response_.erase(http::field::content_length);
        response_.erase(http::field::content_encoding);
        if (bodyless) {
            response_.body().clear();
            response_.erase(http::field::transfer_encoding);
        } else {
            response_.chunked(true);
        }
        http::response_serializer<body_type, fields_type> serializer(response_);
        serializer.split(true);
        while (!serializer.is_header_done()) {
            std::size_t consumed = 0;
            serializer.next(ec, [&](auto&, const auto& buffers) {
                for (const auto buffer : boost::beast::buffers_range_ref(buffers)) {
                    wire_.append(static_cast<const char*>(buffer.data()), buffer.size());
                    consumed += buffer.size();
                }
            });
            if (ec) {
                return;
            }
            serializer.consume(consumed);
        }
        if (!bodyless) {
            // Beast's chunk-size helper uses make_shared without an allocator hook.
            // Format this single buffered synthetic chunk in fixed storage instead.
            if (!response_.body().empty()) {
                std::array<char, 2 * sizeof(std::size_t)> digits{};
                const auto converted = std::to_chars(digits.data(), digits.data() + digits.size(),
                                                     response_.body().size(), 16);
                wire_.append(digits.data(), converted.ptr);
                wire_.append("\r\n");
                wire_.append(response_.body());
                wire_.append("\r\n");
            }
            wire_.append("0\r\n\r\n");
        }
        response_pending_ = false;
    }

    template <class Buffers>
    std::size_t copy(const Buffers& buffers) {
        auto const count = boost::asio::buffer_copy(
            buffers, boost::asio::buffer(wire_.data() + offset_, wire_.size() - offset_));
        offset_ += count;
        return count;
    }

public:
    /**
     Executor used to schedule stream operations and their completions.
     */
    using executor_type = NextLayer::executor_type;
    /**
     Type of the layer immediately below this stream.
     */
    using next_layer_type = NextLayer;

    /**
     Attaches a synthetic HTTP byte view to the supplied browser layer.
     @param next Underlying layer that must outlive this adapter and its operations.
     */
    explicit http_byte_stream(NextLayer& next)
        : next_(next),
          response_(std::piecewise_construct,
                    std::make_tuple(allocator_type(get_lowest_layer(next).resource())),
                    std::make_tuple(allocator_type(get_lowest_layer(next).resource()))),
          unused_(allocator_type(get_lowest_layer(next).resource())),
          wire_(get_lowest_layer(next).resource()) {}

    /**
     Returns the executor used for asynchronous completions.
     @return The executor associated with the underlying context.
     */
    [[nodiscard]] executor_type get_executor() const { return next_.get_executor(); }

    /**
     Returns the immediately underlying layer.
     @return A reference to the underlying stream.
     */
    NextLayer& next_layer() noexcept { return next_; }

    /**
     Consumes serialized request bytes and starts fetch when the request is complete.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param ec Receives the operation error, or is cleared on success.
     @return The number of input bytes accepted, or zero on failure.
     */
    template <class Buffers>
    std::size_t write_some(const Buffers& buffers, boost::system::error_code& ec) {
        namespace http = boost::beast::http;
        ec.clear();
        if (response_pending_ || offset_ < wire_.size()) {
            ec = boost::asio::error::already_started;
            return 0;
        }
        if (!request_) {
            const auto allocator = allocator_type(get_lowest_layer(next_).resource());
            request_.emplace(std::piecewise_construct, std::make_tuple(allocator),
                             std::make_tuple(allocator));
            request_->eager(true);
        }
        auto const count = request_->put(buffers, ec);
        if (ec == http::error::need_more) {
            ec.clear();
        }
        if (ec) {
            return count;
        }
        if (request_->is_done()) {
            auto request = request_->release();
            head_ = request.method() == http::verb::head;
            detail::browser::start_fetch(get_lowest_layer(next_), request, ec);
            request_.reset();
            if (!ec) {
                response_pending_ = true;
            }
        }
        return count;
    }

    /**
     Consumes serialized request bytes and starts fetch when the request is complete.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @return The number of input bytes accepted, or zero on failure.
     */
    template <class Buffers>
    std::size_t write_some(const Buffers& buffers) {
        boost::system::error_code ec;
        auto n = write_some(buffers, ec);
        detail::browser::throw_error(ec);
        return n;
    }

    /**
     Consumes serialized request bytes and starts fetch when the request is complete.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Buffers, class Token>
    auto async_write_some(const Buffers& buffers, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
            [this, buffers](auto handler) {
                boost::system::error_code ec;
                const auto n = write_some(buffers, ec);
                detail::browser::post(get_executor(), std::move(handler), ec, n);
            },
            token);
    }

    /**
     Reads bytes from the synthetic HTTP response after fetch completes.
     @tparam Buffers MutableBufferSequence receiving response or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param ec Receives the operation error, or is cleared on success.
     @return The number of bytes delivered to the destination.
     */
    template <class Buffers>
    std::size_t read_some(const Buffers& buffers, boost::system::error_code& ec) {
        ec.clear();
        if (offset_ == wire_.size()) {
            if (!response_pending_) {
                ec = boost::asio::error::eof;
                return 0;
            }
            webcpp::beast::http::read(next_, unused_, response_, ec);
            if (ec) {
                return 0;
            }
            frame(ec);
            if (ec) {
                return 0;
            }
        }
        return copy(buffers);
    }

    /**
     Reads bytes from the synthetic HTTP response after fetch completes.
     @tparam Buffers MutableBufferSequence receiving response or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @return The number of bytes delivered to the destination.
     */
    template <class Buffers>
    std::size_t read_some(const Buffers& buffers) {
        boost::system::error_code ec;
        auto n = read_some(buffers, ec);
        detail::browser::throw_error(ec);
        return n;
    }

    /**
     Reads bytes from the synthetic HTTP response after fetch completes.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     @tparam Buffers MutableBufferSequence receiving response or message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Buffers, class Token>
    auto async_read_some(const Buffers& buffers, Token&& token) {
        return boost::asio::async_compose<Token, void(boost::system::error_code, std::size_t)>(
            [this, buffers, started = false, posted = false](
                auto& self, boost::system::error_code ec = {}, std::size_t = 0) mutable {
                if (!posted) {
                    posted = true;
                    detail::browser::post(get_executor(), std::move(self),
                                          boost::system::error_code{}, std::size_t{});
                    return;
                }
                if (ec) {
                    self.complete(ec, 0);
                    return;
                }
                if (started) {
                    frame(ec);
                }
                if (ec) {
                    self.complete(ec, 0);
                    return;
                }
                if (offset_ < wire_.size()) {
                    self.complete({}, copy(buffers));
                    return;
                }
                if (!response_pending_) {
                    self.complete(boost::asio::error::eof, 0);
                    return;
                }
                started = true;
                webcpp::beast::http::async_read(next_, unused_, response_, std::move(self));
            },
            token, next_);
    }
};
}  // namespace browser
}
}  // namespace webcpp::beast
#endif
