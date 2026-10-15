// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every public header of asio but <webcpp/asio/ssl.hpp>, which natively brings OpenSSL.
*/
#ifndef WEBCPP_ASIO_HPP
#define WEBCPP_ASIO_HPP

#include <webcpp/asio/config.hpp>

#include <webcpp/asio/basic_datagram_socket.hpp>
#include <webcpp/asio/basic_socket_acceptor.hpp>
#include <webcpp/asio/basic_stream_socket.hpp>
#include <webcpp/asio/basic_waitable_timer.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/basic_resolver.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/ip/udp.hpp>
#include <webcpp/asio/steady_timer.hpp>  // lint-world: Boost's timers are asio's
#include <webcpp/asio/system_timer.hpp>  // lint-world: Boost's timers are asio's

#ifdef __EMSCRIPTEN__
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/asio/browser/async_wait_event.hpp>
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/browser/error.hpp>
#endif

#endif  // WEBCPP_ASIO_HPP
