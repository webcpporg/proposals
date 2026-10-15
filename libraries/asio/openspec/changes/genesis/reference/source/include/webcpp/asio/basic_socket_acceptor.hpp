// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BASIC_SOCKET_ACCEPTOR_HPP
#define WEBCPP_ASIO_BASIC_SOCKET_ACCEPTOR_HPP
#include <webcpp/asio/config.hpp>
#ifndef __EMSCRIPTEN__
#include <boost/asio/basic_socket_acceptor.hpp>  // lint-world: native sockets keep Boost semantics

namespace webcpp::asio {
/**
 Boost's native socket acceptor.
 @tparam Protocol The network protocol.
 @tparam Executor The associated executor.
*/
template <class Protocol, class Executor = boost::asio::any_io_executor>
using basic_socket_acceptor = boost::asio::basic_socket_acceptor<Protocol, Executor>;
}  // namespace webcpp::asio
#else
#include <boost/asio/associated_allocator.hpp>  // lint-world: allocate a stable peer with the handler allocator
#include <boost/asio/compose.hpp>  // lint-world: preserve associations and peer lifetime across acceptance
#include <memory>
#include <webcpp/asio/basic_stream_socket.hpp>
#include <webcpp/asio/detail/browser_socket.hpp>

namespace webcpp::asio {
/**
 A browser socket acceptor whose operations use browser::transport.
 @tparam Protocol The network protocol.
 @tparam Executor The associated executor.
*/
template <class Protocol, class Executor = boost::asio::any_io_executor>
class basic_socket_acceptor : public detail::browser_socket<Protocol, Executor> {
    using base = detail::browser_socket<Protocol, Executor>;
    using base::transport_state;

public:
    /** Inherits executor, context, protocol and endpoint constructors. */
    using base::base;
    /** The executor type. */
    using executor_type = Executor;
    /** The protocol type. */
    using protocol_type = Protocol;
    /** The endpoint type. */
    using endpoint_type = Protocol::endpoint;
    /** The accepted socket type. */
    using socket_type = basic_stream_socket<Protocol, Executor>;

    /**
     Forwards listen to the transport.
     @param backlog The backlog.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code listen(int backlog, boost::system::error_code& ec) {
        ec = transport_state.listen(backlog);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards listen, throwing on error.
     @param backlog The backlog.
    */
    void listen(int backlog) {
        boost::system::error_code ec;
        listen(backlog, ec);
        detail::throw_socket_error(ec);
    }
#endif
    /**
     Listens with the default backlog.
     @param ec Receives the transport error, or success.
     @return The error code.
    */
    boost::system::error_code listen(boost::system::error_code& ec) {
        return listen(boost::asio::socket_base::max_listen_connections, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /** Listens with the default backlog, throwing on error. */
    void listen() { listen(boost::asio::socket_base::max_listen_connections); }
#endif
    /**
     Forwards accept to the transport.
     @param peer The peer socket or remote endpoint.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code accept(socket_type& peer, boost::system::error_code& ec) {
        ec = transport_state.accept(peer);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards accept, throwing on error.
     @param peer The peer socket or remote endpoint.
    */
    void accept(socket_type& peer) {
        boost::system::error_code ec;
        accept(peer, ec);
        detail::throw_socket_error(ec);
    }
#endif
    /**
     Accepts into peer asynchronously.
     @tparam Token The completion token type.
     @param peer The peer socket or remote endpoint.
     @param token The completion token.
     @return The token result.
    */
    template <class Token>
    auto async_accept(socket_type& peer, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
            [this](auto handler, socket_type* destination) {
                transport_state.async_accept(*destination, std::move(handler));
            },
            token, &peer);
    }

    /**
     Returns a socket with the acceptance result.
     @param ec Receives the transport error, or success.
     @return The accepted socket, or a closed socket on failure.
    */
    socket_type accept(boost::system::error_code& ec) {
        socket_type peer(this->get_executor());
        accept(peer, ec);
        return peer;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /** Returns an accepted socket, throwing on error. @return The accepted socket. */
    socket_type accept() {
        boost::system::error_code ec;
        auto peer = accept(ec);
        detail::throw_socket_error(ec);
        return peer;
    }
#endif
    /**
     Accepts a socket whose storage stays stable until completion.
     @tparam Token The completion token type.
     @param token The completion token.
     @return The token result.
    */
    template <class Token>
    auto async_accept(Token&& token) {
        return boost::asio::async_compose<Token, void(boost::system::error_code, socket_type)>(
            [this, peer = std::shared_ptr<socket_type>()](
                auto& self, boost::system::error_code ec = {}) mutable {
                if (!peer) {
                    peer = std::allocate_shared<socket_type>(
                        boost::asio::get_associated_allocator(self), this->get_executor());
                    transport_state.async_accept(*peer, std::move(self));
                } else {
                    auto result = std::move(*peer);
                    peer.reset();
                    self.complete(ec, std::move(result));
                }
            },
            token, this->get_executor());
    }
};
}  // namespace webcpp::asio
#endif
#endif  // WEBCPP_ASIO_BASIC_SOCKET_ACCEPTOR_HPP
