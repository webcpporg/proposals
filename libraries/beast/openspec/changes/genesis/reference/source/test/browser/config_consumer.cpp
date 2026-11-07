// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#undef WEBCPP_BEAST_UNSUPPORTED_IGNORE
#undef WEBCPP_BEAST_UNSUPPORTED_ERROR
#ifdef WEBCPP_BEAST_TEST_MISMATCH
#define WEBCPP_BEAST_UNSUPPORTED_IGNORE
#else
#define WEBCPP_BEAST_UNSUPPORTED_ERROR
#endif
#include <webcpp/beast/tcp_stream.hpp>

void configured_stream_boundary(webcpp::beast::tcp_stream*);

int main() {
    configured_stream_boundary(nullptr);
}
