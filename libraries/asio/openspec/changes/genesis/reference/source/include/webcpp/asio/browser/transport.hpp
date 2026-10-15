// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_TRANSPORT_HPP
#define WEBCPP_ASIO_BROWSER_TRANSPORT_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/append.hpp>       // lint-world: preserve completion associations
#include <boost/asio/error.hpp>        // lint-world: unsupported browser operations
#include <boost/asio/socket_base.hpp>  // lint-world: socket flags
#include <cstddef>
#include <utility>
#include <webcpp/asio/detail/post_completion.hpp>

namespace webcpp::asio::browser {

/**
 How the sockets of Protocol reach the network in the browser, which gives them none: every
 operation fails with operation_not_supported.

 A later library (relays that carry TCP or UDP over a WebSocket or a WebRTC channel)
 specialises it for a protocol; the specialisation is visible in every translation unit that
 uses the socket type, before its first use.

 @tparam Protocol The protocol: ip::tcp or ip::udp.
*/
template <class Protocol>
class transport {
public:
    /** The protocol endpoint type. */
    using endpoint_type = Protocol::endpoint;

    /**
     Constructs an unsupported transport on executor.
     @param executor The executor.
    */
    explicit transport(boost::asio::any_io_executor executor) : executor_(std::move(executor)) {}

    /** Copies the executor so both transports remain usable after a move.
     @param other The source transport.
    */
    // NOLINTNEXTLINE(cert-oop11-cpp,performance-move-constructor-init)
    transport(transport&& other) noexcept : executor_(other.executor_) {}

    /** Copies the executor, preserving the source transport.
     @param other The source transport.
     @return This transport.
    */
    transport& operator=(transport&& other) noexcept {
        executor_ = other.executor_;
        return *this;
    }

    /** Not copyable: a specialized transport may own a connection. */
    transport(const transport&) = delete;
    /** Not copy-assignable: a specialized transport may own a connection. */
    transport& operator=(const transport&) = delete;
    /** Releases the transport state. */
    ~transport() = default;

    /** Closes the resource-free default transport successfully.
     @return Success.
    */
    boost::system::error_code close() noexcept { return {}; }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code connect(const endpoint_type& /*endpoint*/) { return not_supported(); }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class Handler>
    void async_connect(const endpoint_type& /*endpoint*/, Handler&& handler) {
        fail(std::forward<Handler>(handler));
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& /*buffers*/, boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class MutableBufferSequence, class Handler>
    void async_read_some(const MutableBufferSequence& /*buffers*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& /*buffers*/, boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class ConstBufferSequence, class Handler>
    void async_write_some(const ConstBufferSequence& /*buffers*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class ConstBufferSequence>
    std::size_t send(const ConstBufferSequence& /*buffers*/,
                     boost::asio::socket_base::message_flags /*flags*/,
                     boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class ConstBufferSequence, class Handler>
    void async_send(const ConstBufferSequence& /*buffers*/,
                    boost::asio::socket_base::message_flags /*flags*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class MutableBufferSequence>
    std::size_t receive(const MutableBufferSequence& /*buffers*/,
                        boost::asio::socket_base::message_flags /*flags*/,
                        boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class MutableBufferSequence, class Handler>
    void async_receive(const MutableBufferSequence& /*buffers*/,
                       boost::asio::socket_base::message_flags /*flags*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class ConstBufferSequence>
    std::size_t send_to(const ConstBufferSequence& /*buffers*/, const endpoint_type& /*endpoint*/,
                        boost::asio::socket_base::message_flags /*flags*/,
                        boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam ConstBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class ConstBufferSequence, class Handler>
    void async_send_to(const ConstBufferSequence& /*buffers*/, const endpoint_type& /*endpoint*/,
                       boost::asio::socket_base::message_flags /*flags*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    template <class MutableBufferSequence>
    std::size_t receive_from(const MutableBufferSequence& /*buffers*/, endpoint_type& /*endpoint*/,
                             boost::asio::socket_base::message_flags /*flags*/,
                             boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam MutableBufferSequence The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class MutableBufferSequence, class Handler>
    void async_receive_from(const MutableBufferSequence& /*buffers*/, endpoint_type& /*endpoint*/,
                            boost::asio::socket_base::message_flags /*flags*/, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code bind(const endpoint_type& /*endpoint*/) { return not_supported(); }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code listen(int /*backlog*/) { return not_supported(); }

    /**
     Reports the default transport result without accessing a network.
     @tparam Socket The operation argument type.
     @return operation_not_supported.
    */
    template <class Socket>
    boost::system::error_code accept(Socket& /*peer*/) {
        return not_supported();
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam Socket The operation argument type.
     @tparam Handler The completion handler type.
     @param handler The handler to post with the unsupported-operation result.
    */
    template <class Socket, class Handler>
    void async_accept(Socket& /*peer*/, Handler&& handler) {
        fail(std::forward<Handler>(handler));
    }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code shutdown(boost::asio::socket_base::shutdown_type /*what*/) {
        return not_supported();
    }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code cancel() { return not_supported(); }

    /**
     Reports the default transport result without accessing a network.
     @tparam SettableSocketOption The operation argument type.
     @return operation_not_supported.
    */
    template <class SettableSocketOption>
    boost::system::error_code set_option(const SettableSocketOption& /*option*/) {
        return not_supported();
    }

    /**
     Reports the default transport result without accessing a network.
     @tparam GettableSocketOption The operation argument type.
     @return operation_not_supported.
    */
    template <class GettableSocketOption>
    boost::system::error_code get_option(GettableSocketOption& /*option*/) const {
        return not_supported();
    }

    /**
     Reports the default transport result without accessing a network.
     @param ec Receives operation_not_supported.
     @return An unspecified endpoint.
    */
    endpoint_type local_endpoint(boost::system::error_code& ec) const {
        ec = not_supported();
        return endpoint_type();
    }

    /**
     Reports the default transport result without accessing a network.
     @param ec Receives operation_not_supported.
     @return An unspecified endpoint.
    */
    endpoint_type remote_endpoint(boost::system::error_code& ec) const {
        ec = not_supported();
        return endpoint_type();
    }

    /**
     Reports the default transport result without accessing a network.
     @param ec Receives operation_not_supported.
     @return Zero.
    */
    std::size_t available(boost::system::error_code& ec) const {
        ec = not_supported();
        return 0;
    }

    /**
     Reports the default transport result without accessing a network.
     @return operation_not_supported.
    */
    boost::system::error_code non_blocking(bool /*mode*/) { return not_supported(); }

    /**
     Reports the default transport result without accessing a network.
     @return False.
    */
    [[nodiscard]] bool non_blocking() const noexcept { return false; }

private:
    static boost::system::error_code not_supported() noexcept {
        return boost::asio::error::operation_not_supported;
    }

    /**
     Posts operation_not_supported without accessing a network.
     @tparam Handler The completion handler type.
     @tparam Values The operation argument type.
     @param handler The handler to post with the unsupported-operation result.
     @param values The completion values.
    */
    template <class Handler, class... Values>
    void fail(Handler&& handler, Values... values) {
        webcpp::asio::detail::post_completion(
            executor_, boost::asio::append(std::forward<Handler>(handler), not_supported(),
                                           std::move(values)...));
    }

    boost::asio::any_io_executor executor_;
};

}  // namespace webcpp::asio::browser

#endif  // WEBCPP_ASIO_BROWSER_TRANSPORT_HPP
