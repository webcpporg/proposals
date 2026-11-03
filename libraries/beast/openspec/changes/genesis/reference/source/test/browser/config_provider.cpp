// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Deliberately model an externally built translation unit with strict policy.
#undef WEBCPP_BEAST_UNSUPPORTED_IGNORE
#undef WEBCPP_BEAST_UNSUPPORTED_ERROR
#define WEBCPP_BEAST_UNSUPPORTED_ERROR
#include <webcpp/beast/tcp_stream.hpp>

// NOLINTNEXTLINE(misc-use-internal-linkage): external symbol is the cross-TU policy contract
void configured_stream_boundary(webcpp::beast::tcp_stream* /*stream*/) {}
