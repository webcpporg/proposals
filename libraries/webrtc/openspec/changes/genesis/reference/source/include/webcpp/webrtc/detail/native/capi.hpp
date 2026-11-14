// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DETAIL_NATIVE_CAPI_HPP
#define WEBCPP_WEBRTC_DETAIL_NATIVE_CAPI_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/types.hpp>

// libdatachannel's C API, which catches every exception libdatachannel throws and returns a
// negative code instead, so that this backend works with exceptions on and off (design.md, D4).
#include <rtc/rtc.h>  // lint-world: libdatachannel's WebRTC, the library's purpose

#include <boost/system/error_code.hpp>

#include <climits>
#include <cstddef>
#include <optional>
#include <string_view>

namespace webcpp::webrtc::detail::native {

/** The error type of every completion. */
using boost::system::error_code;

/**
 The code of a negative result of the C API, for one call.

 @param rc The result, negative.
 @param invalid What `RTC_ERR_INVALID` (a `std::invalid_argument` inside) means for this call.
 @param failure What `RTC_ERR_FAILURE` (any other exception inside) means for this call.
*/
inline error_code code_of(int rc, error invalid, error failure) {
    switch (rc) {
        case RTC_ERR_INVALID: return make_error_code(invalid);
        case RTC_ERR_FAILURE: return make_error_code(failure);
        default: return make_error_code(error::operation_error);
    }
}

/** The state libdatachannel reports, as the W3C names it. */
inline peer_connection_state state_of(rtcState state) {
    switch (state) {
        case RTC_NEW: return peer_connection_state::new_;
        case RTC_CONNECTING: return peer_connection_state::connecting;
        case RTC_CONNECTED: return peer_connection_state::connected;
        case RTC_DISCONNECTED: return peer_connection_state::disconnected;
        case RTC_FAILED: return peer_connection_state::failed;
        case RTC_CLOSED: return peer_connection_state::closed;
    }
    return peer_connection_state::failed;
}

/** The ICE state libdatachannel reports, as the W3C names it. */
inline ice_connection_state ice_state_of(rtcIceState state) {
    switch (state) {
        case RTC_ICE_NEW: return ice_connection_state::new_;
        case RTC_ICE_CHECKING: return ice_connection_state::checking;
        case RTC_ICE_CONNECTED: return ice_connection_state::connected;
        case RTC_ICE_COMPLETED: return ice_connection_state::completed;
        case RTC_ICE_FAILED: return ice_connection_state::failed;
        case RTC_ICE_DISCONNECTED: return ice_connection_state::disconnected;
        case RTC_ICE_CLOSED: return ice_connection_state::closed;
    }
    return ice_connection_state::failed;
}

/** The type libdatachannel names a description by. */
inline const char* type_name(sdp_type type) {
    switch (type) {
        case sdp_type::offer: return "offer";
        case sdp_type::answer: return "answer";
        case sdp_type::pranswer: return "pranswer";
        case sdp_type::rollback: return "rollback";
    }
    return "offer";
}

/** The type a description's name says, or nothing for a name that is none. */
inline std::optional<sdp_type> type_of(std::string_view name) {
    if (name == "offer") {
        return sdp_type::offer;
    }
    if (name == "answer") {
        return sdp_type::answer;
    }
    if (name == "pranswer") {
        return sdp_type::pranswer;
    }
    if (name == "rollback") {
        return sdp_type::rollback;
    }
    return std::nullopt;
}

/** Whether `size` fits the C API's `int`. */
inline bool fits_int(std::size_t size) noexcept {
    return size <= static_cast<std::size_t>(INT_MAX);
}

/**
 Reads a text of the C API, whose getter writes it with its terminating zero, into `out`.

 The getter is called once for the length, then until the text fits: a local description grows as
 candidates are gathered between the two calls.

 @param get The getter, `rtcGetLocalDescription` or another of its form.
 @param id The object it reads.
 @param out The string it writes, in the caller's allocator.
 @return The C API's code: 0, or negative.
*/
template <class String>
int read_text(int (*get)(int, char*, int), int id, String& out) {
    for (;;) {
        const int length = get(id, nullptr, 0);
        if (length < 0) {
            return length;
        }
        out.resize(static_cast<std::size_t>(length));
        const int written = get(id, out.data(), length);
        if (written == RTC_ERR_TOO_SMALL) {
            continue;
        }
        if (written < 0) {
            return written;
        }
        // Without its terminating zero.
        out.resize(written > 0 ? static_cast<std::size_t>(written) - 1 : 0);
        return 0;
    }
}

}  // namespace webcpp::webrtc::detail::native

#endif  // WEBCPP_WEBRTC_DETAIL_NATIVE_CAPI_HPP
