// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DATA_CHANNEL_HPP
#define WEBCPP_WEBRTC_DATA_CHANNEL_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/types.hpp>

#ifdef WEBCPP_TARGET_EMSCRIPTEN
#error "webcpp::webrtc: the browser implementation is increment 2 of reference/plan.md"
#endif

#include <webcpp/webrtc/detail/native/channel.hpp>
#include <webcpp/webrtc/detail/operation.hpp>

#include <webcpp/asio/any_io_executor.hpp>  // lint-world: the default executor
#include <webcpp/asio/async_result.hpp>     // lint-world: completion tokens
#include <webcpp/asio/buffer.hpp>           // lint-world: a send's buffer sequence

#include <boost/system/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace webcpp::webrtc {

template <class Executor, class Allocator>
class basic_peer_connection;

namespace detail {
struct channel_access;
}  // namespace detail

/**
 A data channel, the W3C's `RTCDataChannel`, whose events are operations the caller re-arms.

 Messages are kept from the channel's creation until `async_receive` reads them: natively in
 libdatachannel's own bounded queue, which slows the peer when it is full. A channel is made by
 its connection's `create_data_channel` or `async_accept_data_channel`; it is movable, not
 copyable, and its executor's context outlives it.

 @tparam Executor The executor its handlers run on.
 @tparam Allocator The allocator of its state, its queues and its label.
*/
template <class Executor = webcpp::asio::any_io_executor,
          class Allocator = std::allocator<std::byte>>
class basic_data_channel {
    using state_type = detail::native::channel_state<Executor, Allocator>;

public:
    /** The executor its handlers run on. */
    using executor_type = Executor;
    /** The allocator of its state. */
    using allocator_type = Allocator;

    /** Moves the channel, its pending operations included; the source holds none. */
    basic_data_channel(basic_data_channel&& other) noexcept = default;

    /**
     Moves the channel, closing the one this held.

     @param other The channel moved from.
     @return This channel.
    */
    basic_data_channel& operator=(basic_data_channel&& other) noexcept {
        if (this != &other) {
            reset();
            state_ = std::move(other.state_);
        }
        return *this;
    }

    basic_data_channel(const basic_data_channel&) = delete;
    basic_data_channel& operator=(const basic_data_channel&) = delete;

    /** Closes the channel and completes every pending operation with `operation_aborted`. */
    // Posting the aborted completions allocates, and a failure to allocate there ends the
    // program, as in any noexcept destructor that must complete operations.
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~basic_data_channel() { reset(); }

    /**
     The executor its handlers run on.

     @return The executor.
    */
    [[nodiscard]] executor_type get_executor() const noexcept { return state_->get_executor(); }

    /**
     The allocator of its state.

     @return The allocator.
    */
    [[nodiscard]] allocator_type get_allocator() const noexcept { return state_->get_allocator(); }

    /**
     Waits for the channel to open; completes at once, posted, when it is open.

     @param token The completion token, of signature `void(error_code)`; `error::closed` when the
     channel closed first.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_open(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                state->wait_open(op);
            },
            token, state_);
    }

    /**
     Waits for the channel to close; completes at once, posted, when it is closed.

     @param token The completion token, of signature `void(error_code)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_close(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                state->wait_close(op);
            },
            token, state_);
    }

    /**
     Appends the next message, whole, to `buffer`.

     A message larger than the room left in `buffer` stays queued, and the operation completes
     with `no_buffer_space`. Once the peer closed the channel, the messages it sent before are
     still delivered, then the operation completes with `error::closed`.

     @param buffer A dynamic buffer (Asio's first version, as `beast::flat_buffer`), which
     outlives the operation.
     @param token The completion token, of signature `void(error_code, std::size_t, bool)`: the
     message's size and whether it was text.
     @return What the token makes of the operation.
    */
    template <class DynamicBuffer, class CompletionToken>
    auto async_receive(DynamicBuffer& buffer, CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, std::size_t, bool)>(
            [](auto handler, const std::shared_ptr<state_type>& state, DynamicBuffer* target) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<std::size_t, bool>{0, false},
                                                  detail::wait_cancellation);
                state->receive(*target, op);
            },
            token, state_, &buffer);
    }

    /**
     Sends a binary message, completing once it is queued, at once, posted.

     @param buffers The message, read during the call.
     @param token The completion token, of signature `void(error_code)`.
     @return What the token makes of the operation.
    */
    template <class ConstBufferSequence, class CompletionToken>
    auto async_send(const ConstBufferSequence& buffers, CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state,
               const ConstBufferSequence& data) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                op.fail(send_buffers(*state, data));
            },
            token, state_, buffers);
    }

    /**
     Sends a text message, completing once it is queued, at once, posted.

     @param text The message, UTF-8, read during the call.
     @param token The completion token, of signature `void(error_code)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_send_text(std::string_view text, CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state, std::string_view data) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                op.fail(state->send(data.data(), data.size(), true));
            },
            token, state_, text);
    }

    /**
     Waits for `buffered_amount()` to be at or below the threshold; at once, posted, when it is.

     @param token The completion token, of signature `void(error_code)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_buffered_amount_low(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                state->wait_buffered_amount_low(op);
            },
            token, state_);
    }

    /**
     Sends a binary message: queues it, or fails.

     @param buffers The message, read during the call.
     @return `error::invalid_state` when the channel is not open, `error::message_too_large` past
     the smaller of `options::max_message_size` and the peer's limit,
     `error::buffered_amount_exceeded` past `options::max_buffered_amount`; nothing once queued.
    */
    template <class ConstBufferSequence>
    boost::system::error_code send(const ConstBufferSequence& buffers) {
        return send_buffers(*state_, buffers);
    }

    /**
     Sends a text message: queues it, or fails as `send` does.

     @param text The message, UTF-8, read during the call. Natively a text holding a zero fails
     with `operation_not_supported` (the table of differences).
     @return The failure, or nothing once queued.
    */
    boost::system::error_code send_text(std::string_view text) {
        return state_->send(text.data(), text.size(), true);
    }

    /**
     Sets the threshold `async_wait_buffered_amount_low` waits for.

     @param threshold The threshold, in bytes; 0 at first.
    */
    void set_buffered_amount_low_threshold(std::size_t threshold) {
        state_->set_buffered_amount_low_threshold(threshold);
    }

    /**
     The bytes queued to send, the W3C's `bufferedAmount`.

     @return The bytes.
    */
    [[nodiscard]] std::size_t buffered_amount() const { return state_->buffered_amount(); }

    /**
     The channel's state, the W3C's `readyState`.

     @return The state.
    */
    [[nodiscard]] data_channel_state ready_state() const noexcept { return state_->ready_state(); }

    /**
     The channel's label.

     @return A view of the label, in the object's allocator, valid while the channel lives.
    */
    [[nodiscard]] std::string_view label() const noexcept { return state_->label(); }

    /**
     The channel's SCTP stream id, the W3C's `id`.

     @return The id, or nothing before the transport gave it one.
    */
    [[nodiscard]] std::optional<std::uint16_t> id() const { return state_->id(); }

    /**
     Closes the channel: its pending operations complete with `operation_aborted`, but
     `async_wait_close`, which completes once it is closed; unread messages are discarded.
    */
    void close() { state_->close(); }

private:
    template <class, class>
    friend class basic_peer_connection;
    friend struct detail::channel_access;

    explicit basic_data_channel(std::shared_ptr<state_type> state) noexcept
        : state_(std::move(state)) {}

    // Sends the message `buffers` holds, gathered once in the channel's allocator when it is in
    // several buffers.
    template <class ConstBufferSequence>
    static boost::system::error_code send_buffers(state_type& state,
                                                  const ConstBufferSequence& buffers) {
        const auto begin = webcpp::asio::buffer_sequence_begin(buffers);
        const auto end = webcpp::asio::buffer_sequence_end(buffers);
        if (begin == end) {
            return state.send(nullptr, 0, false);
        }
        if (std::next(begin) == end) {
            const webcpp::asio::const_buffer one(*begin);
            return state.send(static_cast<const char*>(one.data()), one.size(), false);
        }
        using storage_allocator = std::allocator_traits<Allocator>::template rebind_alloc<char>;
        std::vector<char, storage_allocator> storage(webcpp::asio::buffer_size(buffers),
                                                     storage_allocator(state.get_allocator()));
        webcpp::asio::buffer_copy(webcpp::asio::buffer(storage), buffers);
        return state.send(storage.data(), storage.size(), false);
    }

    void reset() {
        if (state_) {
            state_->destroy();
            state_.reset();
        }
    }

    std::shared_ptr<state_type> state_;
};

namespace detail {

/** The library's access to a channel's construction. */
struct channel_access {
    /** A channel over `state`, null for the value a failed accept completes with. */
    template <class Channel, class State>
    static Channel make(std::shared_ptr<State> state) noexcept {
        return Channel(std::move(state));
    }
};

}  // namespace detail

/** A data channel on Asio's type-erased executor, in the standard allocator. */
using data_channel = basic_data_channel<>;

}  // namespace webcpp::webrtc

#endif  // WEBCPP_WEBRTC_DATA_CHANNEL_HPP
