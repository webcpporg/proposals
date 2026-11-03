// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_TCP_STREAM_HPP
#define WEBCPP_BEAST_TCP_STREAM_HPP

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP3)
#include <boost/beast/core/stream_traits.hpp>  // lint-world: stream-layer access
#include <boost/beast/core/tcp_stream.hpp>     // lint-world: explicit guest TCP backend

namespace webcpp::beast {
/** Finds the bottom layer of a stream stack. */
using boost::beast::get_lowest_layer;
/**
 Upstream TCP stream with operation deadlines on the selected Asio socket backend.

 Link `/webcpp/beast//sockets` for WASIp3's Asio overlay. Host HTTP does not use this type.
*/
using boost::beast::tcp_stream;
}  // namespace webcpp::beast
#else
#error "The Beast socket backend currently requires native or WASIp3; host transports are separate"
#endif

#endif
