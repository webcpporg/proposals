// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BASIC_DATAGRAM_SOCKET_HPP
#define WEBCPP_ASIO_BASIC_DATAGRAM_SOCKET_HPP
#include <webcpp/asio/config.hpp>
#ifndef WEBCPP_TARGET_EMSCRIPTEN
#include <boost/asio/basic_datagram_socket.hpp>  // lint-world: native sockets keep Boost semantics

namespace webcpp::asio {
/**
 Boost's native datagram socket.
 @tparam Protocol The network protocol.
 @tparam Executor The associated executor.
*/
template <class Protocol, class Executor = boost::asio::any_io_executor>
using basic_datagram_socket = boost::asio::basic_datagram_socket<Protocol, Executor>;
}  // namespace webcpp::asio
#else
#include <webcpp/asio/detail/browser_socket.hpp>

namespace webcpp::asio {
/**
 A browser datagram socket whose operations use browser::transport.
 @tparam Protocol The network protocol.
 @tparam Executor The associated executor.
*/
template <class Protocol, class Executor = boost::asio::any_io_executor>
class basic_datagram_socket : public detail::browser_socket<Protocol, Executor> {
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

    /**
     Forwards connect to the transport.
     @param endpoint The endpoint.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code connect(const endpoint_type& endpoint,
                                      boost::system::error_code& ec) {
        ec = transport_state.connect(endpoint);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards connect, throwing on error.
     @param endpoint The endpoint.
    */
    void connect(const endpoint_type& endpoint) {
        boost::system::error_code ec;
        connect(endpoint, ec);
        detail::throw_socket_error(ec);
    }
#endif
    /**
     Initiates connect and posts its completion.
     @tparam Token The completion token type.
     @param endpoint The endpoint.
     @param token The completion token.
     @return The completion token result.
    */
    template <class Token>
    auto async_connect(const endpoint_type& endpoint, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
            [this](auto handler, const endpoint_type& endpoint) {
                transport_state.async_connect(endpoint, std::move(handler));
            },
            token, endpoint);
    }

    /**
     Forwards shutdown to the transport.
     @param what The what.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code shutdown(boost::asio::socket_base::shutdown_type what,
                                       boost::system::error_code& ec) {
        ec = transport_state.shutdown(what);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards shutdown, throwing on error.
     @param what The what.
    */
    void shutdown(boost::asio::socket_base::shutdown_type what) {
        boost::system::error_code ec;
        shutdown(what, ec);
        detail::throw_socket_error(ec);
    }
#endif
    /**
     Forwards send to the transport.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param flags The flags.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Buffers>
    std::size_t send(const Buffers& buffers, boost::asio::socket_base::message_flags flags,
                     boost::system::error_code& ec) {
        return transport_state.send(buffers, flags, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards send, throwing on error.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param flags The flags.
     @return The operation result.
    */
    template <class Buffers>
    std::size_t send(const Buffers& buffers, boost::asio::socket_base::message_flags flags) {
        boost::system::error_code ec;
        const auto result = send(buffers, flags, ec);
        detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Initiates send and posts its completion.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param flags The flags.
     @param token The completion token.
     @return The completion token result.
    */
    template <class Buffers, class Token>
    auto async_send(const Buffers& buffers, boost::asio::socket_base::message_flags flags,
                    Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
            [this](auto handler, const Buffers& buffers,
                   boost::asio::socket_base::message_flags flags) {
                transport_state.async_send(buffers, flags, std::move(handler));
            },
            token, buffers, flags);
    }

    /**
     Uses send with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param ec Receives the transport error, or success.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t send(const Buffers& buffers, boost::system::error_code& ec) {
        return send(buffers, 0, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Uses send with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t send(const Buffers& buffers) {
        return send(buffers, 0);
    }
#endif
    /**
     Initiates send with no message flags.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param token The completion token.
     @return The token result.
    */
    template <class Buffers, class Token>
    auto async_send(const Buffers& buffers, Token&& token) {
        return async_send(buffers, 0, std::forward<Token>(token));
    }

    /**
     Forwards receive to the transport.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param flags The flags.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Buffers>
    std::size_t receive(const Buffers& buffers, boost::asio::socket_base::message_flags flags,
                        boost::system::error_code& ec) {
        return transport_state.receive(buffers, flags, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards receive, throwing on error.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param flags The flags.
     @return The operation result.
    */
    template <class Buffers>
    std::size_t receive(const Buffers& buffers, boost::asio::socket_base::message_flags flags) {
        boost::system::error_code ec;
        const auto result = receive(buffers, flags, ec);
        detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Initiates receive and posts its completion.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param flags The flags.
     @param token The completion token.
     @return The completion token result.
    */
    template <class Buffers, class Token>
    auto async_receive(const Buffers& buffers, boost::asio::socket_base::message_flags flags,
                       Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
            [this](auto handler, const Buffers& buffers,
                   boost::asio::socket_base::message_flags flags) {
                transport_state.async_receive(buffers, flags, std::move(handler));
            },
            token, buffers, flags);
    }

    /**
     Uses receive with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param ec Receives the transport error, or success.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t receive(const Buffers& buffers, boost::system::error_code& ec) {
        return receive(buffers, 0, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Uses receive with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t receive(const Buffers& buffers) {
        return receive(buffers, 0);
    }
#endif
    /**
     Initiates receive with no message flags.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param token The completion token.
     @return The token result.
    */
    template <class Buffers, class Token>
    auto async_receive(const Buffers& buffers, Token&& token) {
        return async_receive(buffers, 0, std::forward<Token>(token));
    }

    /**
     Forwards send_to to the transport.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Buffers>
    std::size_t send_to(const Buffers& buffers, const endpoint_type& endpoint,
                        boost::asio::socket_base::message_flags flags,
                        boost::system::error_code& ec) {
        return transport_state.send_to(buffers, endpoint, flags, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards send_to, throwing on error.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @return The operation result.
    */
    template <class Buffers>
    std::size_t send_to(const Buffers& buffers, const endpoint_type& endpoint,
                        boost::asio::socket_base::message_flags flags) {
        boost::system::error_code ec;
        const auto result = send_to(buffers, endpoint, flags, ec);
        detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Initiates send_to and posts its completion.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @param token The completion token.
     @return The completion token result.
    */
    template <class Buffers, class Token>
    auto async_send_to(const Buffers& buffers, const endpoint_type& endpoint,
                       boost::asio::socket_base::message_flags flags, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
            [this](auto handler, const Buffers& buffers, const endpoint_type& endpoint,
                   boost::asio::socket_base::message_flags flags) {
                transport_state.async_send_to(buffers, endpoint, flags, std::move(handler));
            },
            token, buffers, endpoint, flags);
    }

    /**
     Uses send_to with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param ec Receives the transport error, or success.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t send_to(const Buffers& buffers, const endpoint_type& endpoint,
                        boost::system::error_code& ec) {
        return send_to(buffers, endpoint, 0, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Uses send_to with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t send_to(const Buffers& buffers, const endpoint_type& endpoint) {
        return send_to(buffers, endpoint, 0);
    }
#endif
    /**
     Initiates send_to with no message flags.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param token The completion token.
     @return The token result.
    */
    template <class Buffers, class Token>
    auto async_send_to(const Buffers& buffers, const endpoint_type& endpoint, Token&& token) {
        return async_send_to(buffers, endpoint, 0, std::forward<Token>(token));
    }

    /**
     Forwards receive_from to the transport.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Buffers>
    std::size_t receive_from(const Buffers& buffers, endpoint_type& endpoint,
                             boost::asio::socket_base::message_flags flags,
                             boost::system::error_code& ec) {
        return transport_state.receive_from(buffers, endpoint, flags, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards receive_from, throwing on error.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @return The operation result.
    */
    template <class Buffers>
    std::size_t receive_from(const Buffers& buffers, endpoint_type& endpoint,
                             boost::asio::socket_base::message_flags flags) {
        boost::system::error_code ec;
        const auto result = receive_from(buffers, endpoint, flags, ec);
        detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Initiates receive_from and posts its completion.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param flags The flags.
     @param token The completion token.
     @return The completion token result.
    */
    template <class Buffers, class Token>
    auto async_receive_from(const Buffers& buffers, endpoint_type& endpoint,
                            boost::asio::socket_base::message_flags flags, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
            [this](auto handler, const Buffers& buffers, endpoint_type* endpoint,
                   boost::asio::socket_base::message_flags flags) {
                transport_state.async_receive_from(buffers, *endpoint, flags, std::move(handler));
            },
            token, buffers, &endpoint, flags);
    }

    /**
     Uses receive_from with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param ec Receives the transport error, or success.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t receive_from(const Buffers& buffers, endpoint_type& endpoint,
                             boost::system::error_code& ec) {
        return receive_from(buffers, endpoint, 0, ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Uses receive_from with no message flags.
     @tparam Buffers The operation type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @return The byte count.
    */
    template <class Buffers>
    std::size_t receive_from(const Buffers& buffers, endpoint_type& endpoint) {
        return receive_from(buffers, endpoint, 0);
    }
#endif
    /**
     Initiates receive_from with no message flags.
     @tparam Buffers The operation type.
     @tparam Token The completion token type.
     @param buffers The buffers.
     @param endpoint The endpoint.
     @param token The completion token.
     @return The token result.
    */
    template <class Buffers, class Token>
    auto async_receive_from(const Buffers& buffers, endpoint_type& endpoint, Token&& token) {
        return async_receive_from(buffers, endpoint, 0, std::forward<Token>(token));
    }
};
}  // namespace webcpp::asio
#endif
#endif  // WEBCPP_ASIO_BASIC_DATAGRAM_SOCKET_HPP
