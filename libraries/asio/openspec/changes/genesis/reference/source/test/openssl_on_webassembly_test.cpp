// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// expected-error@*:* {{is named in a build for Emscripten, whose browser profile does not link
// OpenSSL}}
#include "../src/openssl_on_webassembly.hpp"
