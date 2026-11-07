// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <webcpp/asio/config.hpp>

// The same public consumer exposes target capabilities before any feature header.
#ifdef WEBCPP_TARGET_EMSCRIPTEN
static_assert(WEBCPP_ASIO_HAS_BROWSER_BRIDGE == 1);
static_assert(WEBCPP_ASIO_HAS_SOCKET_BACKEND == 0);
static_assert(WEBCPP_ASIO_HAS_OPENSSL_BACKEND == 0);
static_assert(WEBCPP_ASIO_HAS_HOST_DNS == 0);
#else
static_assert(WEBCPP_ASIO_HAS_BROWSER_BRIDGE == 0);
static_assert(WEBCPP_ASIO_HAS_SOCKET_BACKEND == 1);
static_assert(WEBCPP_ASIO_HAS_OPENSSL_BACKEND == 1);
static_assert(WEBCPP_ASIO_HAS_HOST_DNS == 1);
#endif

#ifdef WEBCPP_TARGET_WASIP2
static_assert(WEBCPP_ASIO_HAS_WASI_STDIO == 1);
#else
static_assert(WEBCPP_ASIO_HAS_WASI_STDIO == 0);
#endif

#ifndef WEBCPP_TARGET_NATIVE
static_assert(WEBCPP_ASIO_HAS_REVERSE_DNS == 0);
static_assert(WEBCPP_ASIO_HAS_LOCAL_SOCKETS == 0);
static_assert(WEBCPP_ASIO_HAS_SIGNALS == 0);
static_assert(WEBCPP_ASIO_HAS_SERIAL_PORT == 0);
#endif

#ifdef BOOST_ASIO_HAS_THREADS
static_assert(WEBCPP_ASIO_HAS_THREADS == 1);
#else
static_assert(WEBCPP_ASIO_HAS_THREADS == 0);
#endif

#ifdef __EMSCRIPTEN_PTHREADS__
static_assert(WEBCPP_ASIO_HAS_BROWSER_WORKERS == 1);
#else
static_assert(WEBCPP_ASIO_HAS_BROWSER_WORKERS == 0);
#endif
