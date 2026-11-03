// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_CORE_HPP
#define WEBCPP_BEAST_CORE_HPP

#include <webcpp/beast/config.hpp>

#include <boost/beast/core/bind_handler.hpp>       // lint-world: binds handler arguments only
#include <boost/beast/core/buffers_to_string.hpp>  // lint-world: copies memory buffers only
#include <boost/beast/core/error.hpp>              // lint-world: error codes only
#include <boost/beast/core/flat_buffer.hpp>        // lint-world: memory buffers only
#include <boost/beast/core/multi_buffer.hpp>       // lint-world: memory buffers only
#include <boost/beast/core/static_buffer.hpp>      // lint-world: memory buffers only

namespace webcpp::beast {
/** Upstream contiguous dynamic buffer with an application-supplied allocator. */
using boost::beast::basic_flat_buffer;
/** Upstream segmented dynamic buffer with an application-supplied allocator. */
using boost::beast::basic_multi_buffer;
/** Binds leading arguments to a completion handler. */
using boost::beast::bind_front_handler;
/** Copies a buffer sequence to a string. */
using boost::beast::buffers_to_string;
/** Upstream common stream errors, including timeout. */
using boost::beast::error;
/** The error-code type used by Beast operations. */
using boost::beast::error_code;
/** Upstream contiguous dynamic buffer. */
using boost::beast::flat_buffer;
/** Upstream segmented dynamic buffer. */
using boost::beast::multi_buffer;
/** Upstream fixed-capacity buffer. */
using boost::beast::static_buffer;
}  // namespace webcpp::beast

#endif
