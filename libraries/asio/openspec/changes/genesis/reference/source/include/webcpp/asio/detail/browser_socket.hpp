// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_DETAIL_BROWSER_SOCKET_HPP
#define WEBCPP_ASIO_DETAIL_BROWSER_SOCKET_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/async_result.hpp>       // lint-world: socket completion token initiation
#include <boost/asio/execution_context.hpp>  // lint-world: constrain context constructors
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <type_traits>
#include <utility>
#include <webcpp/asio/browser/transport.hpp>

namespace webcpp::asio::detail {
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
/** Throws a socket error through Boost's configured exception hook. */
inline void throw_socket_error(boost::system::error_code ec) {
    if (ec) {
        boost::throw_exception(boost::system::system_error(ec));
    }
}
#endif
/** Shared browser socket state and forwarding, with no native file descriptor. */
template <class Protocol, class Executor>
class browser_socket : public boost::asio::socket_base {
public:
    /** The executor type. */
    using executor_type = Executor;
    /** The protocol type. */
    using protocol_type = Protocol;
    /** The endpoint type. */
    using endpoint_type = Protocol::endpoint;

    /** Constructs a closed socket on executor. */
    explicit browser_socket(const Executor& executor)
        : executor_(executor), transport_state(executor) {}

    /** Constructs a closed socket on context. */
    template <class Context>
        requires std::is_convertible_v<Context&, boost::asio::execution_context&>
    explicit browser_socket(Context& context) : browser_socket(context.get_executor()) {}
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /** Constructs and opens a socket for protocol. */
    browser_socket(const Executor& executor, const Protocol& protocol) : browser_socket(executor) {
        open(protocol);
    }

    /** Constructs and binds a socket to endpoint. */
    browser_socket(const Executor& executor, const endpoint_type& endpoint)
        : browser_socket(executor, endpoint.protocol()) {
        bind(endpoint);
    }

    /** Constructs and opens a socket on context. */
    template <class Context>
        requires std::is_convertible_v<Context&, boost::asio::execution_context&>
    browser_socket(Context& context, const Protocol& protocol)
        : browser_socket(context.get_executor(), protocol) {}

    /** Constructs and binds a socket on context. */
    template <class Context>
        requires std::is_convertible_v<Context&, boost::asio::execution_context&>
    browser_socket(Context& context, const endpoint_type& endpoint)
        : browser_socket(context.get_executor(), endpoint) {}
#endif
    /** Transfers state and leaves other closed. */
    // A moved-from socket keeps its executor, as Boost sockets do.
    // NOLINTNEXTLINE(cert-oop11-cpp,performance-move-constructor-init)
    browser_socket(browser_socket&& other) noexcept(
        std::is_nothrow_copy_constructible_v<Executor> &&
        std::is_nothrow_move_constructible_v<browser::transport<Protocol>> &&
        std::is_nothrow_copy_constructible_v<Protocol>)
        // Keep the moved-from executor usable, matching Boost socket semantics.
        // NOLINTNEXTLINE(cert-oop11-cpp,performance-move-constructor-init)
        : executor_(other.executor_),
          transport_state(std::move(other.transport_state)),
          protocol_(other.protocol_),
          open_(std::exchange(other.open_, false)) {}

    /** Transfers state and leaves other closed. */
    browser_socket& operator=(browser_socket&& other) noexcept(
        std::is_nothrow_copy_assignable_v<Executor> &&
        std::is_nothrow_move_assignable_v<browser::transport<Protocol>> &&
        std::is_nothrow_copy_assignable_v<Protocol>) {
        if (this != &other) {
            executor_ = other.executor_;
            transport_state = std::move(other.transport_state);
            protocol_ = other.protocol_;
            open_ = std::exchange(other.open_, false);
        }
        return *this;
    }

    /** Not copyable. */
    browser_socket(const browser_socket&) = delete;
    /** Not copy-assignable. */
    browser_socket& operator=(const browser_socket&) = delete;
    /** Destroys the transport state. */
    ~browser_socket() = default;

    /** Returns the associated executor. */
    [[nodiscard]] executor_type get_executor() const noexcept { return executor_; }

    /** Returns whether the local socket state is open. */
    [[nodiscard]] bool is_open() const noexcept { return open_; }

    /** Opens local state, or reports already_open. */
    boost::system::error_code open(const protocol_type& protocol, boost::system::error_code& ec) {
        if (open_) {
            ec = boost::asio::error::already_open;
        } else {
            protocol_ = protocol;
            open_ = true;
            ec.clear();
        }
        return ec;
    }

    /** Closes the transport and marks local state closed on success. */
    boost::system::error_code close(boost::system::error_code& ec) {
        ec = transport_state.close();
        if (!ec) {
            open_ = false;
        }
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /** Opens local state, throwing on failure. */
    void open(const protocol_type& protocol = protocol_type::v4()) {
        boost::system::error_code ec;
        open(protocol, ec);
        throw_socket_error(ec);
    }

    /** Closes local state. */
    void close() {
        boost::system::error_code ec;
        close(ec);
        throw_socket_error(ec);
    }
#endif
    /** Returns the transport's recorded non-blocking mode. */
    [[nodiscard]] bool non_blocking() const noexcept { return transport_state.non_blocking(); }

    /**
     Forwards cancel to the transport.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code cancel(boost::system::error_code& ec) {
        ec = transport_state.cancel();
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards cancel, throwing on error.
    */
    void cancel() {
        boost::system::error_code ec;
        cancel(ec);
        webcpp::asio::detail::throw_socket_error(ec);
    }
#endif
    /**
     Forwards bind to the transport.
     @param endpoint The endpoint.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code bind(const endpoint_type& endpoint, boost::system::error_code& ec) {
        ec = transport_state.bind(endpoint);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards bind, throwing on error.
     @param endpoint The endpoint.
    */
    void bind(const endpoint_type& endpoint) {
        boost::system::error_code ec;
        bind(endpoint, ec);
        webcpp::asio::detail::throw_socket_error(ec);
    }
#endif
    /**
     Forwards set_option to the transport.
     @tparam Option The operation type.
     @param option The option.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Option>
    boost::system::error_code set_option(const Option& option, boost::system::error_code& ec) {
        ec = transport_state.set_option(option);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards set_option, throwing on error.
     @tparam Option The operation type.
     @param option The option.
    */
    template <class Option>
    void set_option(const Option& option) {
        boost::system::error_code ec;
        set_option(option, ec);
        webcpp::asio::detail::throw_socket_error(ec);
    }
#endif
    /**
     Forwards get_option to the transport.
     @tparam Option The operation type.
     @param option The option.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    template <class Option>
    boost::system::error_code get_option(Option& option, boost::system::error_code& ec) const {
        ec = transport_state.get_option(option);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards get_option, throwing on error.
     @tparam Option The operation type.
     @param option The option.
    */
    template <class Option>
    void get_option(Option& option) const {
        boost::system::error_code ec;
        get_option(option, ec);
        webcpp::asio::detail::throw_socket_error(ec);
    }
#endif
    /**
     Forwards local_endpoint to the transport.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    endpoint_type local_endpoint(boost::system::error_code& ec) const {
        return transport_state.local_endpoint(ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards local_endpoint, throwing on error.
     @return The operation result.
    */
    [[nodiscard]] endpoint_type local_endpoint() const {
        boost::system::error_code ec;
        auto result = local_endpoint(ec);
        webcpp::asio::detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Forwards remote_endpoint to the transport.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    endpoint_type remote_endpoint(boost::system::error_code& ec) const {
        return transport_state.remote_endpoint(ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards remote_endpoint, throwing on error.
     @return The operation result.
    */
    [[nodiscard]] endpoint_type remote_endpoint() const {
        boost::system::error_code ec;
        auto result = remote_endpoint(ec);
        webcpp::asio::detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Forwards available to the transport.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    std::size_t available(boost::system::error_code& ec) const {
        return transport_state.available(ec);
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards available, throwing on error.
     @return The operation result.
    */
    [[nodiscard]] std::size_t available() const {
        boost::system::error_code ec;
        const auto result = available(ec);
        webcpp::asio::detail::throw_socket_error(ec);
        return result;
    }
#endif
    /**
     Forwards non_blocking to the transport.
     @param mode The mode.
     @param ec Receives the transport error, or success.
     @return The result; ec receives any error.
    */
    boost::system::error_code non_blocking(bool mode, boost::system::error_code& ec) {
        ec = transport_state.non_blocking(mode);
        return ec;
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Forwards non_blocking, throwing on error.
     @param mode The mode.
    */
    void non_blocking(bool mode) {
        boost::system::error_code ec;
        non_blocking(mode, ec);
        webcpp::asio::detail::throw_socket_error(ec);
    }
#endif
private:
    executor_type executor_;

protected:
    browser::transport<Protocol> transport_state;

private:
    protocol_type protocol_ = protocol_type::v4();
    bool open_ = false;
};
}  // namespace webcpp::asio::detail
#endif  // WEBCPP_ASIO_DETAIL_BROWSER_SOCKET_HPP
