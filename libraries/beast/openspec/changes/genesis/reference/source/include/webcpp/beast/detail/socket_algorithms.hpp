// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_DETAIL_SOCKET_ALGORITHMS_HPP
#define WEBCPP_BEAST_DETAIL_SOCKET_ALGORITHMS_HPP

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP3)
#include <boost/beast/core/async_base.hpp>         // lint-world: composed operation support
#include <boost/beast/core/basic_stream.hpp>       // lint-world: explicitly selected socket backend
#include <boost/beast/core/buffers_generator.hpp>  // lint-world: explicitly selected stream writes
#include <boost/beast/core/detect_ssl.hpp>         // lint-world: TLS detection over a byte stream
#include <boost/beast/core/rate_policy.hpp>        // lint-world: stream rate policies
#include <boost/beast/core/role.hpp>               // lint-world: protocol role enumeration
#include <boost/beast/core/stream_traits.hpp>      // lint-world: stream concepts
#include <boost/beast/http/message_generator.hpp>  // lint-world: explicit socket message writing

namespace webcpp::beast {
/** Base for an allocator-aware composed asynchronous operation. */
using boost::beast::async_base;
/** Detects a TLS ClientHello without consuming it. */
using boost::beast::async_detect_ssl;
/** Writes a buffer generator to an asynchronous byte stream. */
using boost::beast::async_write;
/** Upstream socket stream with deadlines and rate policies. */
using boost::beast::basic_stream;
/** Upstream executor type customization. */
using boost::beast::executor_type;
/** Tests whether a type satisfies Beast's asynchronous stream requirements. */
using boost::beast::is_async_stream;
/** Identifies a protocol's client or server role. */
using boost::beast::role_type;
/** Socket rate policy that does not limit throughput. */
using boost::beast::unlimited_rate_policy;
/** Writes a buffer generator to a synchronous byte stream. */
using boost::beast::write;
}  // namespace webcpp::beast

namespace webcpp::beast::http {
/** Type-erased message serializer for the explicitly selected socket backend. */
using boost::beast::http::message_generator;
}  // namespace webcpp::beast::http
#endif

#endif
