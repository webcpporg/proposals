// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_SRC_OPENSSL_ON_WEBASSEMBLY_HPP
#define WEBCPP_ASIO_SRC_OPENSSL_ON_WEBASSEMBLY_HPP

// Forced into every source when a WebAssembly program names the OpenSSL target.
#if defined(__EMSCRIPTEN__) || defined(WEBCPP_TEST_ASIO_AS_WEBASSEMBLY)
#error \
    "webcpp::asio: /webcpp/asio//openssl is named in a build for WebAssembly, and OpenSSL is native only: there <webcpp/asio/ssl.hpp> is a record of the TLS settings and links no library, and nothing of OpenSSL would be in the .wasm. In the program's Jamfile, name OpenSSL for native builds alone, by a condition in the project's requirements: project : requirements <conditional>@native-openssl ; rule native-openssl ( properties * ) { if ! <toolset>emscripten in $(properties) { return <library>/webcpp/asio//openssl ; } } (without b2: link OpenSSL into the native program only)"
#endif
#endif  // WEBCPP_ASIO_SRC_OPENSSL_ON_WEBASSEMBLY_HPP
