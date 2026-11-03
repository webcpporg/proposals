// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_SSL_STREAM_HPP
#define WEBCPP_BEAST_BROWSER_SSL_STREAM_HPP

#include <webcpp/beast/config.hpp>

#include <boost/system/result.hpp>
#include <optional>
#include <webcpp/asio/ssl.hpp>
#include <webcpp/beast/browser/tcp_stream.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
/**
 TLS intent validated before the browser owns the secure connection.
 @tparam NextLayer Underlying browser authority or TLS-intent layer.
 */
template <class NextLayer>
class ssl_stream {
    NextLayer next_;

    struct tls_record {
        webcpp::asio::ssl::context::method method{};
        int mode{};
        webcpp::asio::ssl::verify_callback_kind callback{};
        std::pmr::string host;
        bool roots{};
        bool credentials{};
        webcpp::asio::ssl::context::options options{};
    } context_;

    std::pmr::string server_name_;
    std::optional<int> verify_mode_;
    webcpp::asio::ssl::verify_callback_kind callback_ =
        webcpp::asio::ssl::verify_callback_kind::none;
    std::pmr::string verification_host_;

public:
    /**
     Type of the layer immediately below this stream.
     */
    using next_layer_type = NextLayer;
    /**
     Executor used to schedule stream operations and their completions.
     */
    using executor_type = NextLayer::executor_type;

    /**
     Selects the stream type for a different executor.
     @tparam Executor Executor type to bind to the underlying layer.
     */
    template <class Executor>
    struct rebind_executor {
        /**
         Stream type with the requested executor binding.
         */
        using other = ssl_stream<typename NextLayer::template rebind_executor<Executor>::other>;
    };

    /**
     Constructs the next layer and snapshots browser TLS policy.
     @tparam Arg Argument forwarded to construct the next layer.
     @param arg Argument forwarded to the underlying layer constructor.
     @param context TLS context whose configuration is copied at construction.
     */
    template <class Arg>
    ssl_stream(Arg&& arg, webcpp::asio::ssl::context& context)
        : next_(std::forward<Arg>(arg)),
          context_{
              context.recorded_method(),
              context.recorded_verify_mode(),
              context.recorded_callback(),
              std::pmr::string(context.verification_host(), get_lowest_layer(next_).resource()),
              context.roots_added(),
              context.client_credentials(),
              context.recorded_options(),
          },
          server_name_(get_lowest_layer(next_).resource()),
          verification_host_(get_lowest_layer(next_).resource()) {}

    /** Constructs every layer and copied TLS name with the supplied allocator.
     @tparam Allocator Allocator retained for owned stream state.
     @tparam Arg Argument forwarded to the next layer.
     @param allocator Allocator used by the layer and its TLS metadata.
     @param arg Argument forwarded to the next layer constructor.
     @param context TLS configuration copied before opening the host connection.
     */
    template <class Allocator, class Arg>
    ssl_stream(std::allocator_arg_t /*unused*/, const Allocator& allocator, Arg&& arg,
               webcpp::asio::ssl::context& context)
        : next_(std::allocator_arg, allocator, std::forward<Arg>(arg)),
          context_{
              context.recorded_method(),
              context.recorded_verify_mode(),
              context.recorded_callback(),
              std::pmr::string(context.verification_host(), get_lowest_layer(next_).resource()),
              context.roots_added(),
              context.client_credentials(),
              context.recorded_options(),
          },
          server_name_(get_lowest_layer(next_).resource()),
          verification_host_(get_lowest_layer(next_).resource()) {}

    /**
     Returns the executor used for asynchronous completions.
     @return The executor associated with the underlying context.
     */
    [[nodiscard]] executor_type get_executor() const { return next_.get_executor(); }

    /**
     Returns the immediately underlying layer.
     @return A reference to the underlying stream.
     */
    NextLayer& next_layer() noexcept { return next_; }

    /**
     Returns the immediately underlying layer.
     @return A reference to the underlying stream.
     */
    [[nodiscard]] const NextLayer& next_layer() const noexcept { return next_; }

    /**
     Records the verification policy for the next TLS-intent validation.
     @param mode Requested verification flags; strict browser TLS requires verify_peer.
     */
    void set_verify_mode(int mode) { verify_mode_ = mode; }

    /**
     Records the verification policy for the next TLS-intent validation.
     @param mode Requested verification flags; strict browser TLS requires verify_peer.
     @param ec Receives the operation error, or is cleared on success.
     */
    void set_verify_mode(int mode, boost::system::error_code& ec) {
        set_verify_mode(mode);
        ec.clear();
    }

    /**
     Records hostname verification or marks a custom callback unsupported.
     @tparam Callback Verification callable type; only host_name_verification is representable.
     @param callback Hostname-verification record or unsupported custom callable.
     */
    template <class Callback>
    void set_verify_callback(const Callback& callback) {
        if constexpr (std::is_same_v<Callback, webcpp::asio::ssl::host_name_verification>) {
            callback_ = webcpp::asio::ssl::verify_callback_kind::host_name_verification;
            verification_host_ = callback.host();
        } else {
            callback_ = webcpp::asio::ssl::verify_callback_kind::other;
        }
    }

    /**
     Records hostname verification or marks a custom callback unsupported.
     @tparam Callback Verification callable type; only host_name_verification is representable.
     @param callback Hostname-verification record or unsupported custom callable.
     @param ec Receives the operation error, or is cleared on success.
     */
    template <class Callback>
    void set_verify_callback(const Callback& callback, boost::system::error_code& ec) {
        set_verify_callback(callback);
        ec.clear();
    }

    /**
     Validates TLS intent and marks the authority secure without opening a TLS socket.

     The browser performs certificate validation when fetch or WebSocket opens the URL.
     Under ERROR, custom roots, credentials, version pins, callbacks, or mismatched names
     produce operation_not_supported; IGNORE leaves those choices to the browser.
     @param type Requested TLS role; only client is representable by browser host APIs.
     @param ec Receives the operation error, or is cleared on success.
     */
    void handshake(webcpp::asio::ssl::stream_base::handshake_type type,
                   boost::system::error_code& ec) {
        namespace ssl = webcpp::asio::ssl;
        auto& record = get_lowest_layer(next_);
        const auto mode = verify_mode_.value_or(context_.mode);
        const auto callback =
            callback_ == ssl::verify_callback_kind::none ? context_.callback : callback_;
        const auto host = callback_ == ssl::verify_callback_kind::none
                              ? std::string_view(context_.host)
                              : std::string_view(verification_host_);
        ec = detail::browser::unsupported(
            type != ssl::stream_base::client ||
            (context_.method != ssl::context::tls_client &&
             context_.method != ssl::context::sslv23_client) ||
            mode != ssl::verify_peer || context_.roots || context_.credentials ||
            context_.options != 0 || callback == ssl::verify_callback_kind::other ||
            (callback == ssl::verify_callback_kind::host_name_verification &&
             host != record.host()) ||
            (!server_name_.empty() && server_name_ != record.host()));
        if (!ec) {
            record.secure(true);
        }
    }

    /**
     Validates TLS intent and marks the authority secure without opening a TLS socket.

     The browser performs certificate validation when fetch or WebSocket opens the URL.
     Under ERROR, custom roots, credentials, version pins, callbacks, or mismatched names
     produce operation_not_supported; IGNORE leaves those choices to the browser.
     @param type Requested TLS role; only client is representable by browser host APIs.
     */
    void handshake(webcpp::asio::ssl::stream_base::handshake_type type) {
        boost::system::error_code ec;
        handshake(type, ec);
        detail::browser::throw_error(ec);
    }

    /**
     Validates TLS intent and marks the authority secure without opening a TLS socket.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     The browser performs certificate validation when fetch or WebSocket opens the URL.
     Under ERROR, custom roots, credentials, version pins, callbacks, or mismatched names
     produce operation_not_supported; IGNORE leaves those choices to the browser.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param type Requested TLS role; only client is representable by browser host APIs.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_handshake(webcpp::asio::ssl::stream_base::handshake_type type, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
            [this, type](auto handler) {
                boost::system::error_code ec;
                handshake(type, ec);
                detail::browser::post(get_executor(), std::move(handler), ec);
            },
            token);
    }

    /**
     Closes the browser operation associated with the underlying authority.
     @param ec Receives the operation error, or is cleared on success.
     */
    void shutdown(boost::system::error_code& ec) { get_lowest_layer(next_).close(ec); }

    /**
     Closes the browser operation associated with the underlying authority.
     */
    void shutdown() {
        boost::system::error_code ec;
        shutdown(ec);
    }

    /**
     Closes the browser operation associated with the underlying authority.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_shutdown(Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
            [this](auto handler) {
                boost::system::error_code ec;
                shutdown(ec);
                detail::browser::post(get_executor(), std::move(handler), ec);
            },
            token);
    }

    /**
     Records the requested TLS server name for authority validation.
     @param host Hostname to match against the recorded destination authority.
     @return Success, or invalid_argument for an invalid server-name record.
     */
    boost::system::result<void> set_server_name(std::string_view host) {
        if (host.empty() || host.size() > 255 || host.find('\0') != std::string_view::npos) {
            return boost::asio::error::invalid_argument;
        }
        server_name_ = host;
        return {};
    }
};

/**
 Records the requested TLS server name for authority validation.
 @tparam NextLayer Underlying browser authority or TLS-intent layer.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param host Hostname to match against the recorded destination authority.
 @return Success, or invalid_argument for an invalid server-name record.
 */
template <class NextLayer>
boost::system::result<void> set_server_name(ssl_stream<NextLayer>& stream, std::string_view host) {
    return stream.set_server_name(host);
}
}
}  // namespace webcpp::beast
#endif
