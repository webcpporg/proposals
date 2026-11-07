// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of asio, which each of its headers includes first: whether
 asio is built without exceptions, target capabilities, and the default number
 of handlers per turn.

 @see "Testing", in the guide.
*/
#ifndef WEBCPP_ASIO_CONFIG_HPP
#define WEBCPP_ASIO_CONFIG_HPP

#include <boost/asio/detail/config.hpp>  // lint-world: inherit Boost's configured facilities
#include <webcpp/config.hpp>

#ifndef WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN
/**
 The default number of ready handlers processed in one turn on Emscripten.

 Define this nonnegative integer on the compiler command line to replace 100.
 0 is taken as 1 by io_context. Explicit context options override this default.
 Use the same definition in every translation unit, including Cobalt's compiled
 runner, and rebuild when changing it. Natively it does not change scheduling.
 This is a handler count, not a time budget or a browser responsiveness guarantee.

 @see "The run family", in the guide.
*/
#define WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN 100
#endif

#if WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN < 0
#error "WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN must be a nonnegative integer"
#endif

// MrDocs parses asio as a build with exceptions does, where the macro is not
// defined: this definition, undone at once, is the one the reference lists, and
// the parse goes on with every API that throws in it.
#ifdef __MRDOCS__
/**
 Defined when asio is built without exceptions: automatically when the
 collection has none (`WEBCPP_HAS_EXCEPTIONS` is not defined), or by the
 developer to disable them in this library alone. `WEBCPP_NO_EXCEPTIONS`
 disables them throughout the collection.

 asio raises an exception only where Boost.Asio's own interface throws: the
 throwing overloads of its sockets, resolver and `ssl::context` (an
 `error_code&` overload stands beside each), which are absent while this is
 defined.

 @see "Testing", in the guide.
*/
#define WEBCPP_ASIO_NO_EXCEPTIONS
#undef WEBCPP_ASIO_NO_EXCEPTIONS
#endif

// asio's build defines it when a program asks for less Asyncify stack than asio's measured
// suspensions need (doc: #asio-building-asyncify-stack).
#ifdef WEBCPP_ASIO_ASYNCIFY_STACK_SIZE_REFUSED
#error \
    "asyncify-stack-size is below 8192 bytes, the least that asio's measured suspensions need, half of it left to the program: ask for 8192 or more"
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_ASIO_NO_EXCEPTIONS)
#define WEBCPP_ASIO_NO_EXCEPTIONS
#endif

// These are observations, never user switches. In particular, selecting the
// optional OpenSSL library is separate from having an OpenSSL-capable target.
#if defined(WEBCPP_ASIO_HAS_SOCKET_BACKEND) || defined(WEBCPP_ASIO_HAS_OPENSSL_BACKEND) || \
    defined(WEBCPP_ASIO_HAS_BROWSER_BRIDGE) || defined(WEBCPP_ASIO_HAS_BROWSER_WORKERS) || \
    defined(WEBCPP_ASIO_HAS_HOST_DNS) || defined(WEBCPP_ASIO_HAS_REVERSE_DNS) ||           \
    defined(WEBCPP_ASIO_HAS_WASI_STDIO) || defined(WEBCPP_ASIO_HAS_THREADS) ||             \
    defined(WEBCPP_ASIO_HAS_LOCAL_SOCKETS) || defined(WEBCPP_ASIO_HAS_SIGNALS) ||          \
    defined(WEBCPP_ASIO_HAS_SERIAL_PORT)
#error \
    "webcpp::asio: capability macros are derived by config.hpp; do not define WEBCPP_ASIO_HAS_* capabilities on the command line"
#else

#ifdef WEBCPP_TARGET_EMSCRIPTEN
/** 1 when Emscripten supplies the JavaScript event and Promise bridge; otherwise 0. */
#define WEBCPP_ASIO_HAS_BROWSER_BRIDGE 1
/** 1 when native or WASI socket APIs back TCP/UDP; grants and options remain runtime facts. */
#define WEBCPP_ASIO_HAS_SOCKET_BACKEND 0
/** 1 when this target can link the source-built OpenSSL provider; it does not select it. */
#define WEBCPP_ASIO_HAS_OPENSSL_BACKEND 0
/** 1 when the host supplies forward address lookup; it does not imply reverse DNS. */
#define WEBCPP_ASIO_HAS_HOST_DNS 0
#else
/** 1 when Emscripten supplies the JavaScript event and Promise bridge; otherwise 0. */
#define WEBCPP_ASIO_HAS_BROWSER_BRIDGE 0
/** 1 when native or WASI socket APIs back TCP/UDP; grants and options remain runtime facts. */
#define WEBCPP_ASIO_HAS_SOCKET_BACKEND 1
/** 1 when this target can link the source-built OpenSSL provider; it does not select it. */
#define WEBCPP_ASIO_HAS_OPENSSL_BACKEND 1
/** 1 when the host supplies forward address lookup; it does not imply reverse DNS. */
#define WEBCPP_ASIO_HAS_HOST_DNS 1
#endif

#ifdef WEBCPP_TARGET_NATIVE
/** 1 for native reverse lookup; WASI's address lookup interface is forward-only. */
#define WEBCPP_ASIO_HAS_REVERSE_DNS 1
#else
/** 1 for native reverse lookup; WASI's address lookup interface is forward-only. */
#define WEBCPP_ASIO_HAS_REVERSE_DNS 0
#endif

#ifdef WEBCPP_TARGET_WASIP2
/** 1 when the pollable WASIp2 CLI stream adapter is available; otherwise 0. */
#define WEBCPP_ASIO_HAS_WASI_STDIO 1
#else
/** 1 when the pollable WASIp2 CLI stream adapter is available; otherwise 0. */
#define WEBCPP_ASIO_HAS_WASI_STDIO 0
#endif

#ifdef BOOST_ASIO_HAS_THREADS
/** 1 when this Boost.Asio configuration enables threads, including cooperative threads. */
#define WEBCPP_ASIO_HAS_THREADS 1
#else
/** 1 when this Boost.Asio configuration enables threads, including cooperative threads. */
#define WEBCPP_ASIO_HAS_THREADS 0
#endif

#if defined(__EMSCRIPTEN_PTHREADS__) && WEBCPP_ASIO_HAS_THREADS
/** 1 for a pthread browser profile; isolation and worker availability are runtime facts. */
#define WEBCPP_ASIO_HAS_BROWSER_WORKERS 1
#else
/** 1 for a pthread browser profile; isolation and worker availability are runtime facts. */
#define WEBCPP_ASIO_HAS_BROWSER_WORKERS 0
#endif

#if defined(WEBCPP_TARGET_NATIVE) && defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
/** 1 when native Boost.Asio enables local sockets; otherwise 0. */
#define WEBCPP_ASIO_HAS_LOCAL_SOCKETS 1
#else
/** 1 when native Boost.Asio enables local sockets; otherwise 0. */
#define WEBCPP_ASIO_HAS_LOCAL_SOCKETS 0
#endif
#if defined(WEBCPP_TARGET_NATIVE) && defined(BOOST_ASIO_HAS_SIGNAL)
/** 1 when native Boost.Asio enables process signals; WASI and browser events are not signals. */
#define WEBCPP_ASIO_HAS_SIGNALS 1
#else
/** 1 when native Boost.Asio enables process signals; WASI and browser events are not signals. */
#define WEBCPP_ASIO_HAS_SIGNALS 0
#endif
#if defined(WEBCPP_TARGET_NATIVE) && defined(BOOST_ASIO_HAS_SERIAL_PORT)
/** 1 when native Boost.Asio enables serial ports; otherwise 0. */
#define WEBCPP_ASIO_HAS_SERIAL_PORT 1
#else
/** 1 when native Boost.Asio enables serial ports; otherwise 0. */
#define WEBCPP_ASIO_HAS_SERIAL_PORT 0
#endif

// A consumer can state requirements before including any library header. A
// zero or absent requirement leaves selection to the application at runtime.
#if defined(WEBCPP_ASIO_REQUIRE_SOCKET_BACKEND) && WEBCPP_ASIO_REQUIRE_SOCKET_BACKEND && \
    !WEBCPP_ASIO_HAS_SOCKET_BACKEND
#error \
    "webcpp::asio: this Emscripten target has no raw TCP/UDP socket backend; use a browser transport or a native/WASI target"
#endif
#if defined(WEBCPP_ASIO_REQUIRE_OPENSSL) && WEBCPP_ASIO_REQUIRE_OPENSSL
#if !WEBCPP_ASIO_HAS_OPENSSL_BACKEND
#error \
    "webcpp::asio: OpenSSL is unavailable in the browser profile; use browser-managed HTTPS/WSS or a native/WASI target"
#elif !defined(WEBCPP_ASIO_HAS_OPENSSL) || !WEBCPP_ASIO_HAS_OPENSSL
#error \
    "webcpp::asio: TLS requires /webcpp/asio//openssl or /webcpp/asio//tls; without B2, define WEBCPP_ASIO_HAS_OPENSSL=1 and link the matching libssl/libcrypto"
#endif
#endif
#if defined(WEBCPP_ASIO_REQUIRE_THREADS) && WEBCPP_ASIO_REQUIRE_THREADS && !WEBCPP_ASIO_HAS_THREADS
#error \
    "webcpp::asio: threads are disabled in this configuration; select a supported threading=multi profile (WASIp2 supports single only)"
#endif
#if defined(WEBCPP_ASIO_REQUIRE_LOCAL_SOCKETS) && WEBCPP_ASIO_REQUIRE_LOCAL_SOCKETS && \
    !WEBCPP_ASIO_HAS_LOCAL_SOCKETS
#error \
    "webcpp::asio: local sockets require native Boost.Asio support; WASI/browser targets must use TCP or a supplied byte stream"
#endif
#if defined(WEBCPP_ASIO_REQUIRE_SIGNALS) && WEBCPP_ASIO_REQUIRE_SIGNALS && !WEBCPP_ASIO_HAS_SIGNALS
#error \
    "webcpp::asio: process signals require a native target; use explicit application cancellation on WASI or in the browser"
#endif
#if defined(WEBCPP_ASIO_REQUIRE_SERIAL_PORT) && WEBCPP_ASIO_REQUIRE_SERIAL_PORT && \
    !WEBCPP_ASIO_HAS_SERIAL_PORT
#error \
    "webcpp::asio: serial ports require native Boost.Asio support; WASI/browser need a separate host adapter"
#endif
#if defined(WEBCPP_ASIO_REQUIRE_REVERSE_DNS) && WEBCPP_ASIO_REQUIRE_REVERSE_DNS && \
    !WEBCPP_ASIO_HAS_REVERSE_DNS
#error \
    "webcpp::asio: reverse DNS is unavailable on WASI/browser targets; use numeric endpoints or forward name lookup"
#endif

#endif  // derived capability macros

#if defined(WEBCPP_TARGET_WASIP2) && defined(_REENTRANT)
#error \
    "webcpp::asio: WASIp2 has no supported pthread profile in the pinned SDK; select threading=single or WASIp3 cooperative threads"
#endif

#endif  // WEBCPP_ASIO_CONFIG_HPP
