// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 webcpp::webrtc, the W3C WebRTC data-channel API in Asio's model: peer connections and data
 channels whose operations take completion tokens and run on the user's executor, natively over
 libdatachannel and, from increment 2, in the browser over its own objects.

 Its targets are native and emscripten; under WASI its configuration stops with an `#error` that
 names the reason.

 @see The library's page.
*/
#ifndef WEBCPP_WEBRTC_HPP
#define WEBCPP_WEBRTC_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/data_channel.hpp>
#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/peer_connection.hpp>
#include <webcpp/webrtc/runtime.hpp>
#include <webcpp/webrtc/types.hpp>

#endif  // WEBCPP_WEBRTC_HPP
