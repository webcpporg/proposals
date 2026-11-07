// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_HTTP_WRITE_HPP
#define WEBCPP_BEAST_HTTP_WRITE_HPP

#include <webcpp/beast/config.hpp>

#include <boost/beast/http/write.hpp>  // lint-world: explicit HTTP byte-stream operations

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::http {
/** Upstream HTTP write operation on a byte stream. */
using boost::beast::http::write;
/** Upstream HTTP write header operation on a byte stream. */
using boost::beast::http::write_header;
/** Upstream HTTP write some operation on a byte stream. */
using boost::beast::http::write_some;
/** Upstream HTTP async write operation on a byte stream. */
using boost::beast::http::async_write;
/** Upstream HTTP async write header operation on a byte stream. */
using boost::beast::http::async_write_header;
/** Upstream HTTP async write some operation on a byte stream. */
using boost::beast::http::async_write_some;
}  // namespace webcpp::beast::http

#ifdef WEBCPP_TARGET_EMSCRIPTEN
#include <webcpp/beast/browser/http.hpp>
#endif

#endif
