// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_ERROR_HPP
#define WEBCPP_WEBRTC_ERROR_HPP

#include <webcpp/webrtc/config.hpp>

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/is_error_code_enum.hpp>

#include <string>
#include <type_traits>

namespace webcpp::webrtc {

/**
 The failures of webcpp::webrtc that have no counterpart among Asio's codes.

 Their values are fixed and never reused. Asio's codes keep their own meaning beside them:
 `operation_aborted` for an operation cancelled or ended by `close()` or destruction,
 `already_started` for a second wait of one kind, `operation_not_supported` for what a target
 cannot honour, `no_buffer_space` for a message larger than the room left in the caller's buffer.
*/
enum class error {
    /**
     The W3C's `InvalidStateError`: a candidate before a remote description, a send on a channel
     that is not open.
    */
    invalid_state = 1,
    /**
     The W3C's `OperationError`: the platform refused an operation, as a description it cannot
     parse.
    */
    operation_error = 2,
    /** The W3C's `InvalidAccessError`. */
    invalid_access = 3,
    /**
     The W3C's `TypeError`: an argument out of its range, as both reliability limits of a channel.
    */
    type_error = 4,
    /** The W3C's `SyntaxError`: an ICE server URL that cannot be parsed. */
    syntax_error = 5,
    /** The channel or the connection closed under a pending operation. */
    closed = 6,
    /** A message past `options::max_message_size` or the peer's limit, sent or received. */
    message_too_large = 7,
    /** A send that would pass `options::max_buffered_amount`. */
    buffered_amount_exceeded = 8,
    /** The browser's unread messages past `options::max_received_bytes`. */
    receive_overflow = 9,
    /** `make` past `runtime_options::max_peer_connections`. */
    too_many_peer_connections = 10,
};

namespace detail {

/** The category of webcpp::webrtc's errors, named `webcpp.webrtc`. */
class error_category final : public boost::system::error_category {
public:
    /** The category's name. */
    [[nodiscard]] const char* name() const noexcept override { return "webcpp.webrtc"; }

    /** An English description of a value of `error`. */
    [[nodiscard]] std::string message(int value) const override {
        switch (static_cast<error>(value)) {
            case error::invalid_state: return "Invalid state";
            case error::operation_error: return "Operation refused by the platform";
            case error::invalid_access: return "Invalid access";
            case error::type_error: return "Argument out of its range";
            case error::syntax_error: return "Syntax error";
            case error::closed: return "Closed";
            case error::message_too_large: return "Message too large";
            case error::buffered_amount_exceeded: return "Buffered amount exceeded";
            case error::receive_overflow: return "Receive queue overflow";
            case error::too_many_peer_connections: return "Too many peer connections";
        }
        return "Unknown webcpp.webrtc error";
    }
};

}  // namespace detail

/**
 The category of webcpp::webrtc's errors, named `webcpp.webrtc`.

 @return The category, alive for the whole program.
*/
inline const boost::system::error_category& webrtc_category() noexcept {
    static const detail::error_category category;
    return category;
}

/**
 Makes the error code of a value of `error`.

 @param e The error.
 @return The code, of the category `webcpp.webrtc`.
*/
inline boost::system::error_code make_error_code(error e) noexcept {
    return {static_cast<int>(e), webrtc_category()};
}

}  // namespace webcpp::webrtc

namespace boost::system {

/** Makes `webcpp::webrtc::error` convert to `boost::system::error_code`. */
template <>
struct is_error_code_enum<webcpp::webrtc::error> : std::true_type {};

}  // namespace boost::system

#endif  // WEBCPP_WEBRTC_ERROR_HPP
