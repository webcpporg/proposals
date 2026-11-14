// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_OPTIONS_HPP
#define WEBCPP_WEBRTC_OPTIONS_HPP

#include <webcpp/webrtc/config.hpp>

#include <cstddef>
#include <cstdint>

namespace webcpp::webrtc {

/**
 webcpp's own bounds on a peer connection and its channels, past which an object fails as a value.

 None of them throws or blocks: each is a code of `error`.
*/
struct options {
    /**
     Local candidates kept while no `async_wait_ice_candidate` is pending; the end of gathering has
     a place of its own.
    */
    std::size_t max_queued_candidates = 64;
    /**
     Incoming channels kept while no `async_accept_data_channel` is pending; one past it is closed
     at once.
    */
    std::size_t max_queued_channels = 16;
    /** The largest message sent or received, announced natively in the SDP. */
    std::size_t max_message_size = std::size_t{256} * 1024;
    /**
     The most bytes a channel may hold to send; a send past it fails with
     `buffered_amount_exceeded`.
    */
    std::size_t max_buffered_amount = std::size_t{16} * 1024 * 1024;
    /**
     In the browser, the most bytes of unread messages and their records; past it the channel
     closes.
    */
    std::size_t max_received_bytes = std::size_t{16} * 1024 * 1024;
};

/** What the platform holds for the whole process or page, given to `runtime`. */
struct runtime_options {
    /** The most peer connections alive at once; 256 is half of Chromium's 500 per page. */
    std::size_t max_peer_connections = 256;
    /** Natively, libdatachannel's threads; 0 for its own default. */
    std::uint32_t thread_pool_size = 0;
};

}  // namespace webcpp::webrtc

#endif  // WEBCPP_WEBRTC_OPTIONS_HPP
