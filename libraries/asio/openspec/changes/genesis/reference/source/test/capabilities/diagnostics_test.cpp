// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#if defined(WEBCPP_TEST_FORGED)
// expected-error@webcpp/asio/config.hpp:* {{capability macros are derived by config.hpp}}
#include <webcpp/asio/config.hpp>
#elif defined(WEBCPP_TEST_REQUIRE_TLS)
// expected-error@webcpp/asio/config.hpp:* {{requires /webcpp/asio//openssl or /webcpp/asio//tls}}
#include <webcpp/asio/config.hpp>
#elif defined(WEBCPP_TEST_DISABLED_TLS)
// expected-error@webcpp/asio/ssl.hpp:* {{needs OpenSSL on native and WASI targets, and it is not
// linked}}
#include <webcpp/asio/ssl.hpp>
#elif defined(WEBCPP_TEST_BROWSER_HEADER)
// expected-error@webcpp/asio/browser/bridge.hpp:* {{requires the Emscripten browser bridge}}
#include <webcpp/asio/browser/bridge.hpp>
#elif defined(WEBCPP_TEST_WORKER_HEADER)
// expected-error@webcpp/asio/browser/main_thread_context.hpp:* {{requires Emscripten pthreads}}
#include <webcpp/asio/browser/main_thread_context.hpp>
#elif defined(WEBCPP_TEST_STDIO_HEADER)
// expected-error@webcpp/asio/wasi/stdio_stream.hpp:* {{requires WASIp2 CLI streams}}
#include <webcpp/asio/wasi/stdio_stream.hpp>
#endif
