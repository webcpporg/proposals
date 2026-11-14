// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of webrtc, which each of its headers includes first: whether webrtc is built
 without exceptions, and the policy for what a target cannot honour.

 webcpp::webrtc works with exceptions on and off on every target it builds for: natively it calls
 libdatachannel's C API, which catches libdatachannel's exceptions and returns codes, and in the
 browser its JavaScript catches the browser's. It raises no exception of its own; the macro
 exists so that a program sets `WEBCPP_NO_EXCEPTIONS` for every library alike.

 @see "Exceptions", in the guide.
*/
#ifndef WEBCPP_WEBRTC_CONFIG_HPP
#define WEBCPP_WEBRTC_CONFIG_HPP

#include <webcpp/config.hpp>

#ifdef __MRDOCS__
/**
 Defined when webrtc is built without exceptions: when webcpp has none, or when the developer
 defines it.

 webrtc raises no exception of its own, so the macro changes nothing in its behaviour.
*/
#define WEBCPP_WEBRTC_NO_EXCEPTIONS
#undef WEBCPP_WEBRTC_NO_EXCEPTIONS
/** Makes what a target cannot honour fail with `operation_not_supported`, the default. */
#define WEBCPP_WEBRTC_UNSUPPORTED_ERROR
#undef WEBCPP_WEBRTC_UNSUPPORTED_ERROR
/** Makes what a target cannot honour, and where it may be ignored, ignored. */
#define WEBCPP_WEBRTC_UNSUPPORTED_IGNORE
#undef WEBCPP_WEBRTC_UNSUPPORTED_IGNORE
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_WEBRTC_NO_EXCEPTIONS)
#define WEBCPP_WEBRTC_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_WEBRTC_UNSUPPORTED_ERROR) && defined(WEBCPP_WEBRTC_UNSUPPORTED_IGNORE)
#error \
    "webcpp::webrtc: define one of WEBCPP_WEBRTC_UNSUPPORTED_ERROR and WEBCPP_WEBRTC_UNSUPPORTED_IGNORE, not both"
#elif !defined(WEBCPP_WEBRTC_UNSUPPORTED_ERROR) && !defined(WEBCPP_WEBRTC_UNSUPPORTED_IGNORE)
/** Makes what a target cannot honour fail with `operation_not_supported`, the default. */
#define WEBCPP_WEBRTC_UNSUPPORTED_ERROR
#endif

#if defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3) || defined(__wasi__)
#error \
    "webcpp::webrtc has no WASI target: there is no WebRTC stack for WASI. libdatachannel has no WASI port (its threads, and libjuice's and usrsctp's, need preemptive threads, and its ICE and SCTP were never ported to WASI sockets), and no WASI host interface offers WebRTC as a browser offers RTCPeerConnection. Build for native or emscripten."
#endif

#endif  // WEBCPP_WEBRTC_CONFIG_HPP
