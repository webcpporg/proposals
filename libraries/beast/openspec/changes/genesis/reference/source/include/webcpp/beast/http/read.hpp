// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_HTTP_READ_HPP
#define WEBCPP_BEAST_HTTP_READ_HPP

#include <webcpp/beast/config.hpp>

#include <boost/beast/http/read.hpp>  // lint-world: explicit HTTP byte-stream operations

namespace webcpp::beast::http {
/** Upstream HTTP read operation on a byte stream. */
using boost::beast::http::read;
/** Upstream HTTP read header operation on a byte stream. */
using boost::beast::http::read_header;
/** Upstream HTTP read some operation on a byte stream. */
using boost::beast::http::read_some;
/** Upstream HTTP async read operation on a byte stream. */
using boost::beast::http::async_read;
/** Upstream HTTP async read header operation on a byte stream. */
using boost::beast::http::async_read_header;
/** Upstream HTTP async read some operation on a byte stream. */
using boost::beast::http::async_read_some;
}  // namespace webcpp::beast::http

#endif
