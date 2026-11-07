// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_TEST_ALL_OFF) || defined(WEBCPP_TEST_DEVELOPER_OFF)
#ifndef WEBCPP_BEAST_NO_EXCEPTIONS
#error "The explicit exception policy was lost"
#endif
#elif defined(WEBCPP_HAS_EXCEPTIONS) == defined(WEBCPP_BEAST_NO_EXCEPTIONS)
#error "The exception policy must follow webcpp config"
#endif

#if defined(WEBCPP_BEAST_UNSUPPORTED_ERROR) == defined(WEBCPP_BEAST_UNSUPPORTED_IGNORE)
#error "Exactly one unsupported-operation policy must be selected"
#endif

int main() {}
