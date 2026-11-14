// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_PEER_CONNECTION_HPP
#define WEBCPP_WEBRTC_PEER_CONNECTION_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/data_channel.hpp>
#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/runtime.hpp>
#include <webcpp/webrtc/types.hpp>

#ifdef WEBCPP_TARGET_EMSCRIPTEN
#error "webcpp::webrtc: the browser implementation is increment 2 of reference/plan.md"
#endif

#include <webcpp/webrtc/detail/native/connection.hpp>
#include <webcpp/webrtc/detail/operation.hpp>

#include <webcpp/asio/any_io_executor.hpp>  // lint-world: the default executor
#include <webcpp/asio/async_result.hpp>     // lint-world: completion tokens

#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

namespace webcpp::webrtc {

/**
 A peer connection, the W3C's `RTCPeerConnection`, whose operations follow Asio's model and whose
 events are operations the caller re-arms, as an acceptor's `async_accept`.

 Its operations run on its executor, which is not thread safe: a program calls them, and emits
 their cancellations, on that executor. Its executor's context, its `runtime` and its allocator
 outlive it.

 @tparam Executor The executor its handlers run on.
 @tparam Allocator The allocator of its state, its queues and its outputs.
*/
template <class Executor = webcpp::asio::any_io_executor,
          class Allocator = std::allocator<std::byte>>
class basic_peer_connection {
    using state_type = detail::native::connection_state<Executor, Allocator>;
    using string_type = detail::string<Allocator>;

public:
    /** The executor its handlers run on. */
    using executor_type = Executor;
    /** The allocator of its state and outputs. */
    using allocator_type = Allocator;
    /** A description it produced. */
    using session_description = basic_session_description<Allocator>;
    /** A candidate it gathered. */
    using ice_candidate = basic_ice_candidate<Allocator>;
    /** One of its channels. */
    using data_channel = basic_data_channel<Executor, Allocator>;

    /**
     Makes a peer connection.

     @param rt The runtime, which counts it and outlives it.
     @param executor The executor its handlers run on.
     @param config How it gathers candidates, read during the call.
     @param opts Its bounds.
     @param allocator The allocator of its state and outputs.
     @return The connection; or `error::too_many_peer_connections` past the runtime's cap,
     `error::syntax_error` for an ICE server URL that cannot be parsed, `error::type_error` for a
     bound out of the platform's range, `error::operation_error` for another refusal.
    */
    static boost::system::result<basic_peer_connection> make(
        runtime& rt, const Executor& executor, const configuration& config,
        const options& opts = options(), const Allocator& allocator = Allocator()) {
        detail::runtime_slot slot(rt);
        if (!slot) {
            return make_error_code(error::too_many_peer_connections);
        }
        auto state =
            std::allocate_shared<state_type>(allocator, std::move(slot), executor, opts, allocator);
        if (const auto ec = state->open(config)) {
            return ec;
        }
        return basic_peer_connection(std::move(state));
    }

    /** Moves the connection, its pending operations included; the source holds none. */
    basic_peer_connection(basic_peer_connection&& other) noexcept = default;

    /**
     Moves the connection, closing the one this held.

     @param other The connection moved from.
     @return This connection.
    */
    basic_peer_connection& operator=(basic_peer_connection&& other) noexcept {
        if (this != &other) {
            reset();
            state_ = std::move(other.state_);
        }
        return *this;
    }

    basic_peer_connection(const basic_peer_connection&) = delete;
    basic_peer_connection& operator=(const basic_peer_connection&) = delete;

    /** Closes the connection and completes every pending operation with `operation_aborted`. */
    // Posting the aborted completions allocates, and a failure to allocate there ends the
    // program, as in any noexcept destructor that must complete operations.
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~basic_peer_connection() { reset(); }

    /**
     The executor its handlers run on.

     @return The executor.
    */
    [[nodiscard]] executor_type get_executor() const noexcept { return state_->get_executor(); }

    /**
     The allocator of its state and outputs.

     @return The allocator.
    */
    [[nodiscard]] allocator_type get_allocator() const noexcept { return state_->get_allocator(); }

    /**
     Creates and applies the implicit local description: an offer, or an answer to the remote
     offer, the W3C's `setLocalDescription()` without an argument.

     A cancellation is terminal only: the description may still be applied after the handler ran
     with `operation_aborted`.

     @param token The completion token, of signature `void(error_code, session_description)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_set_local_description(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, session_description)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(
                    std::move(handler), state->get_executor(),
                    std::tuple<session_description>{
                        session_description{sdp_type::offer, string_type(text_allocator(*state))},
                    },
                    webcpp::asio::cancellation_type::terminal);
                state->set_local_description(op);
            },
            token, state_);
    }

    /**
     Applies the remote peer's description.

     @param description The description, read during the call.
     @param token The completion token, of signature `void(error_code)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_set_remote_description(session_description_view description,
                                      CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state, sdp_type type,
               const string_type& sdp) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{},
                                                  webcpp::asio::cancellation_type::terminal);
                state->set_remote_description(type, sdp, op);
            },
            token, state_, description.type, string_type(description.sdp, text_allocator(*state_)));
    }

    /**
     Adds a remote candidate; an empty `candidate` ends the remote gathering and adds none.

     @param candidate The candidate, read during the call.
     @param token The completion token, of signature `void(error_code)`;
     `error::invalid_state` before a remote description.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_add_ice_candidate(ice_candidate_view candidate, CompletionToken&& token) {
        std::optional<string_type> mid;
        if (candidate.sdp_mid) {
            mid.emplace(*candidate.sdp_mid, text_allocator(*state_));
        }
        return webcpp::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
            [](auto handler, const std::shared_ptr<state_type>& state, const string_type& text,
               const std::optional<string_type>& media) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<>{}, detail::wait_cancellation);
                state->add_ice_candidate(text, media, op);
            },
            token, state_, string_type(candidate.candidate, text_allocator(*state_)),
            std::move(mid));
    }

    /**
     Waits for the next local candidate; one whose `candidate` is empty is the end of gathering.

     While no wait is pending, candidates are kept up to `options::max_queued_candidates`; one
     past it is dropped here and stays in `local_description()`.

     @param token The completion token, of signature `void(error_code, ice_candidate)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_ice_candidate(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, ice_candidate)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                const auto text = text_allocator(*state);
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<ice_candidate>{
                                                      ice_candidate{
                                                          string_type(text),
                                                          string_type(text),
                                                          std::nullopt,
                                                          string_type(text),
                                                      },
                                                  },
                                                  detail::wait_cancellation);
                state->wait_ice_candidate(op);
            },
            token, state_);
    }

    /**
     Waits for the connection's state to differ from the one the previous wait completed with.

     @param token The completion token, of signature `void(error_code, peer_connection_state)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_state_change(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, peer_connection_state)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(std::move(handler), state->get_executor(),
                                                  std::tuple<peer_connection_state>{state->state()},
                                                  detail::wait_cancellation);
                state->wait_state_change(op);
            },
            token, state_);
    }

    /**
     Waits for the ICE state to differ from the one the previous wait completed with.

     @param token The completion token, of signature `void(error_code, ice_connection_state)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_wait_ice_connection_state_change(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, ice_connection_state)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op =
                    detail::make_operation(std::move(handler), state->get_executor(),
                                           std::tuple<ice_connection_state>{state->ice_state()},
                                           detail::wait_cancellation);
                state->wait_ice_state_change(op);
            },
            token, state_);
    }

    /**
     Waits for the next channel the peer opened.

     While none is pending, channels are kept up to `options::max_queued_channels`; one past it
     is closed at once, as a connection past a listen backlog is refused.

     @param token The completion token, of signature `void(error_code, data_channel)`.
     @return What the token makes of the operation.
    */
    template <class CompletionToken>
    auto async_accept_data_channel(CompletionToken&& token) {
        return webcpp::asio::async_initiate<CompletionToken,
                                            void(boost::system::error_code, data_channel)>(
            [](auto handler, const std::shared_ptr<state_type>& state) {
                auto& op = detail::make_operation(
                    std::move(handler), state->get_executor(),
                    std::tuple<data_channel>{
                        detail::channel_access::make<data_channel>(
                            std::shared_ptr<typename state_type::channel_type>{}),
                    },
                    detail::wait_cancellation);
                state->accept_data_channel(op);
            },
            token, state_);
    }

    /**
     Creates a data channel, the W3C's `createDataChannel`; adding one does not negotiate.

     @param label The channel's label, read during the call.
     @param init How it is created.
     @return The channel; or `error::type_error` for both reliability limits, `negotiated`
     without an `id`, an id of 65535 or a label or protocol past 65,535 bytes;
     `error::invalid_state` on a closed connection.
    */
    boost::system::result<data_channel> create_data_channel(std::string_view label,
                                                            const data_channel_init& init = {}) {
        return state_->create_data_channel(label, init);
    }

    /**
     The local description, the W3C's `localDescription`, with the candidates gathered so far.

     @return The description, or nothing before one was set.
    */
    [[nodiscard]] std::optional<session_description> local_description() const {
        return state_->local_description();
    }

    /**
     Whether a remote description was applied, which a candidate needs.

     @return Whether there is one.
    */
    [[nodiscard]] bool has_remote_description() const { return state_->has_remote_description(); }

    /**
     The connection's state, the W3C's `connectionState`.

     @return The state.
    */
    [[nodiscard]] peer_connection_state state() const noexcept { return state_->state(); }

    /**
     The ICE state, the W3C's `iceConnectionState`.

     @return The state.
    */
    [[nodiscard]] ice_connection_state ice_state() const noexcept { return state_->ice_state(); }

    /**
     Closes the connection, as the W3C's `close()`: no state change is delivered, its pending
     operations complete with `operation_aborted`, its channels close with `error::closed`, and a
     wait started afterwards completes with `error::closed`.
    */
    void close() { state_->close(); }

private:
    explicit basic_peer_connection(std::shared_ptr<state_type> state) noexcept
        : state_(std::move(state)) {}

    static string_type::allocator_type text_allocator(const state_type& state) {
        return typename string_type::allocator_type(state.get_allocator());
    }

    void reset() {
        if (state_) {
            state_->destroy();
            state_.reset();
        }
    }

    std::shared_ptr<state_type> state_;
};

/** A peer connection on Asio's type-erased executor, in the standard allocator. */
using peer_connection = basic_peer_connection<>;

}  // namespace webcpp::webrtc

#endif  // WEBCPP_WEBRTC_PEER_CONNECTION_HPP
