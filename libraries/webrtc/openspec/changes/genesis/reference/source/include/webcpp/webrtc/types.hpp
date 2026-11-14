// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_TYPES_HPP
#define WEBCPP_WEBRTC_TYPES_HPP

#include <webcpp/webrtc/config.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace webcpp::webrtc {

/** The type of a session description, the W3C's `RTCSdpType`. */
enum class sdp_type {
    /** "offer". */
    offer,
    /** "answer". */
    answer,
    /** "pranswer". */
    pranswer,
    /** "rollback". */
    rollback,
};

/** The state of a peer connection, the W3C's `RTCPeerConnectionState`. */
enum class peer_connection_state {
    /** "new". */
    new_,
    /** "connecting". */
    connecting,
    /** "connected". */
    connected,
    /** "disconnected". */
    disconnected,
    /** "failed". */
    failed,
    /** "closed". */
    closed,
};

/** The state of a peer connection's ICE agent, the W3C's `RTCIceConnectionState`. */
enum class ice_connection_state {
    /** "new". */
    new_,
    /** "checking". */
    checking,
    /** "connected". */
    connected,
    /** "completed". */
    completed,
    /** "disconnected". */
    disconnected,
    /** "failed". */
    failed,
    /** "closed". */
    closed,
};

/** The state of a data channel, the W3C's `RTCDataChannelState`. */
enum class data_channel_state {
    /** "connecting". */
    connecting,
    /** "open". */
    open,
    /** "closing". */
    closing,
    /** "closed". */
    closed,
};

/** Which candidates ICE may use, the W3C's `RTCIceTransportPolicy`. */
enum class ice_transport_policy {
    /** "all". */
    all,
    /** "relay": TURN only. */
    relay,
};

/** A STUN or TURN server, the W3C's `RTCIceServer`, read during the call that takes it. */
struct ice_server {
    /** Its URLs, `stun:host:port` or `turn:host:port?transport=udp`. */
    std::span<const std::string_view> urls{};
    /** The TURN user name, empty for STUN. */
    std::string_view username{};
    /** The TURN credential, empty for STUN. */
    std::string_view credential{};
};

/** How a peer connection gathers candidates, the W3C's `RTCConfiguration`, read during `make`. */
struct configuration {
    /** The servers ICE gathers through; empty for host candidates only. */
    std::span<const ice_server> ice_servers{};
    /** Which candidates ICE may use. */
    ice_transport_policy policy = ice_transport_policy::all;
    /**
     The local address ICE binds to, natively; empty for every interface but the loopback.

     A loopback candidate needs "127.0.0.1". The browser cannot honour it (the table of
     differences).
    */
    std::string_view bind_address{};
};

/** A session description given to a call, the W3C's `RTCSessionDescriptionInit`. */
struct session_description_view {
    /** Its type. */
    sdp_type type = sdp_type::offer;
    /** Its SDP. */
    std::string_view sdp{};
};

/**
 An ICE candidate given to a call, the W3C's `RTCIceCandidateInit`.

 An empty `candidate` ends the remote peer's candidates, and adds none.
*/
struct ice_candidate_view {
    /** The candidate's attribute line, `candidate:...`, or empty. */
    std::string_view candidate{};
    /** The media stream identification it belongs to, when known. */
    std::optional<std::string_view> sdp_mid{};
    /** The index of the m-line it belongs to, when known. */
    std::optional<std::uint16_t> sdp_m_line_index{};
    /** Its ICE user name fragment; natively ignored (the table of differences). */
    std::string_view username_fragment{};
};

/** How a data channel is created, the W3C's `RTCDataChannelInit`. */
struct data_channel_init {
    /** Whether messages arrive in order. */
    bool ordered = true;
    /**
     How long a message may be retransmitted, in milliseconds; exclusive with `max_retransmits`.
    */
    std::optional<std::uint16_t> max_packet_life_time{};
    /** How many times a message may be retransmitted; exclusive with `max_packet_life_time`. */
    std::optional<std::uint16_t> max_retransmits{};
    /** The subprotocol name. */
    std::string_view protocol{};
    /**
     Whether both peers create the channel themselves, with the same `id`, without announcing it.
    */
    bool negotiated = false;
    /** The SCTP stream id, required when `negotiated`; chosen by the platform otherwise. */
    std::optional<std::uint16_t> id{};
};

namespace detail {

/** The string type of an output, in the allocator rebound to `char`. */
template <class Allocator>
using string =
    std::basic_string<char, std::char_traits<char>,
                      typename std::allocator_traits<Allocator>::template rebind_alloc<char>>;

}  // namespace detail

/**
 A session description a peer connection produced, owned, its text in the connection's allocator.

 @tparam Allocator The allocator of its text.
*/
template <class Allocator>
struct basic_session_description {
    /** Its type. */
    sdp_type type = sdp_type::offer;
    /** Its SDP. */
    detail::string<Allocator> sdp{};

    /**
     The view of this description, to give to the other peer's `async_set_remote_description`.

     @return A view valid while this description is unchanged.
    */
    [[nodiscard]] session_description_view view() const noexcept { return {type, sdp}; }
};

/**
 An ICE candidate a peer connection gathered, owned, its text in the connection's allocator.

 An empty `candidate` is the end of gathering, the W3C's own form of it.

 @tparam Allocator The allocator of its text.
*/
template <class Allocator>
struct basic_ice_candidate {
    /** The candidate's attribute line, `candidate:...`, or empty at the end of gathering. */
    detail::string<Allocator> candidate{};
    /** The media stream identification it belongs to; empty when unknown. */
    detail::string<Allocator> sdp_mid{};
    /** The index of the m-line it belongs to; natively always 0 (the table of differences). */
    std::optional<std::uint16_t> sdp_m_line_index{};
    /** Its ICE user name fragment; natively empty (the table of differences). */
    detail::string<Allocator> username_fragment{};

    /**
     The view of this candidate, to give to the other peer's `async_add_ice_candidate`.

     @return A view valid while this candidate is unchanged; an empty `sdp_mid` is no mid.
    */
    [[nodiscard]] ice_candidate_view view() const noexcept {
        return {
            candidate,
            sdp_mid.empty() ? std::nullopt : std::optional<std::string_view>(sdp_mid),
            sdp_m_line_index,
            username_fragment,
        };
    }
};

/** A session description in the standard allocator. */
using session_description = basic_session_description<std::allocator<std::byte>>;

/** An ICE candidate in the standard allocator. */
using ice_candidate = basic_ice_candidate<std::allocator<std::byte>>;

}  // namespace webcpp::webrtc

#endif  // WEBCPP_WEBRTC_TYPES_HPP
