// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_CORE_HPP
#define WEBCPP_BEAST_CORE_HPP

#include <webcpp/beast/config.hpp>

#include <webcpp/beast/browser/error.hpp>

#include <boost/beast/core/bind_handler.hpp>        // lint-world: binds handler arguments only
#include <boost/beast/core/buffers_prefix.hpp>      // lint-world: buffer-sequence views only
#include <boost/beast/core/buffers_to_string.hpp>   // lint-world: copies memory buffers only
#include <boost/beast/core/error.hpp>               // lint-world: error codes only
#include <boost/beast/core/file_base.hpp>           // lint-world: file access mode enumeration
#include <boost/beast/core/flat_buffer.hpp>         // lint-world: memory buffers only
#include <boost/beast/core/flat_static_buffer.hpp>  // lint-world: fixed-capacity memory buffers
#include <boost/beast/core/make_printable.hpp>      // lint-world: buffer output formatting only
#include <boost/beast/core/multi_buffer.hpp>        // lint-world: memory buffers only
#include <boost/beast/core/ostream.hpp>             // lint-world: writes into memory buffers
#include <boost/beast/core/static_buffer.hpp>       // lint-world: memory buffers only
#include <boost/beast/core/string.hpp>              // lint-world: string comparison only

namespace webcpp::beast {
/** Upstream contiguous dynamic buffer with an application-supplied allocator. */
using boost::beast::basic_flat_buffer;
/** Upstream segmented dynamic buffer with an application-supplied allocator. */
using boost::beast::basic_multi_buffer;
/** Binds leading arguments to a completion handler. */
using boost::beast::bind_front_handler;
/** Returns a view of the first bytes of a buffer sequence. */
using boost::beast::buffers_prefix;
/** Copies a buffer sequence to a string. */
using boost::beast::buffers_to_string;
/** Upstream common stream errors, including timeout. */
using boost::beast::error;
/** The error-code type used by Beast operations. */
using boost::beast::error_code;
/** Upstream generic error conditions. */
namespace errc = boost::beast::errc;
/** Upstream file-opening modes for explicit file bodies. */
using boost::beast::file_mode;
/** Upstream contiguous dynamic buffer. */
using boost::beast::flat_buffer;
/** Upstream contiguous buffer with fixed capacity. */
using boost::beast::flat_static_buffer;
/** Compares strings with HTTP's ASCII case-insensitive rules. */
using boost::beast::iequals;
/** Formats a buffer sequence for diagnostic output. */
using boost::beast::make_printable;
/** Upstream segmented dynamic buffer. */
using boost::beast::multi_buffer;
/** Creates an output stream that appends to a dynamic buffer. */
using boost::beast::ostream;
/** Upstream fixed-capacity buffer. */
using boost::beast::static_buffer;
/** Upstream non-owning string view. */
using boost::beast::string_view;
/** Upstream exception type for explicitly throwing application code. */
using boost::beast::system_error;
}  // namespace webcpp::beast

#endif
