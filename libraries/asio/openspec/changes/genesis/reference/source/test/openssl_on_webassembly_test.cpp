// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// expected-error@*:* {{webcpp::asio: /webcpp/asio//openssl is named in a build for WebAssembly, and
// OpenSSL is native only}}
#include "../src/openssl_on_webassembly.hpp"
