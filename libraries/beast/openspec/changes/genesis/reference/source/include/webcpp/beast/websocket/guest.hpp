// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_WEBSOCKET_GUEST_HPP
#define WEBCPP_BEAST_WEBSOCKET_GUEST_HPP

#include <webcpp/beast/config.hpp>

#include <webcpp/beast/websocket.hpp>
#ifdef WEBCPP_TARGET_EMSCRIPTEN
#include <boost/beast/websocket.hpp>  // lint-world: explicitly selected guest protocol engine
#include <functional>
#include <webcpp/asio/basic_stream_socket.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace websocket {
/**
 Selects upstream framing for an explicitly supplied byte stream.
 @tparam NextLayer User-supplied stream implementing the Beast byte-stream concepts.
 @tparam DeflateSupported Whether the guest engine supports permessage-deflate.
 */
template <class NextLayer, bool DeflateSupported>
    requires(!std::is_same_v<NextLayer, tcp_stream> &&
             !std::is_same_v<NextLayer, ssl_stream<tcp_stream>>)
class stream<NextLayer, DeflateSupported>
    : public boost::beast::websocket::stream<NextLayer, DeflateSupported> {
    using base = boost::beast::websocket::stream<NextLayer, DeflateSupported>;
    std::shared_ptr<std::pmr::memory_resource> memory_;

public:
    using base::base;
    using base::set_option;

    /** Adapts facade request and response decorators to the upstream handshake.
     @tparam Function Callable accepting the facade request or response type.
     @param option Decorator applied before the guest engine writes its handshake.
     */
    template <class Function>
    void set_option(stream_base::decorator<Function> option) {
        base::set_option(boost::beast::websocket::stream_base::decorator(
            [function = std::move(option.function), memory = memory_](auto& upstream) mutable {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(upstream)>,
                                             boost::beast::websocket::request_type>) {
                    if constexpr (std::is_invocable_v<Function&, request_type&>) {
                        request_type request(
                            std::piecewise_construct, std::tuple<>(),
                            std::make_tuple(memory ? memory.get()
                                                   : std::pmr::new_delete_resource()));
                        request.method(upstream.method());
                        request.target(upstream.target());
                        request.version(upstream.version());
                        for (const auto& field : upstream) {
                            request.insert(field.name_string(), field.value());
                        }
                        function(request);
                        upstream.clear();
                        upstream.method(request.method());
                        upstream.target(request.target());
                        upstream.version(request.version());
                        for (const auto& field : request) {
                            upstream.insert(field.name_string(), field.value());
                        }
                    }
                } else if constexpr (std::is_invocable_v<Function&, response_type&>) {
                    function(upstream);
                }
            }));
    }

    /** Registers a control notification using the facade's portable frame category.
     @param callback Callback invoked with the frame category and payload, or empty to clear.
     */
    void control_callback(std::function<void(frame_type, boost::beast::string_view)> callback) {
        if (!callback) {
            base::control_callback({});
            return;
        }
        base::control_callback(
            [callback = std::move(callback)](boost::beast::websocket::frame_type kind,
                                             boost::beast::string_view payload) {
                switch (kind) {
                    case boost::beast::websocket::frame_type::close:
                        callback(frame_type::close, payload);
                        break;
                    case boost::beast::websocket::frame_type::ping:
                        callback(frame_type::ping, payload);
                        break;
                    case boost::beast::websocket::frame_type::pong:
                        callback(frame_type::pong, payload);
                        break;
                }
            });
    }

    /** Constructs the guest engine with the upstream allocation behavior.

     The allocator argument preserves constructor compatibility; Boost owns its protocol state.
     @tparam Allocator Allocator used for the facade's request adaptation.
     @tparam Args Constructor argument types of the next layer.
     @param allocator Allocator retained for the facade's request adaptation.
     @param args Arguments forwarded to the upstream stream constructor.
     */
    template <class Allocator, class... Args>
    explicit stream(std::allocator_arg_t /*unused*/, const Allocator& allocator, Args&&... args)
        : base(std::forward<Args>(args)...),
          memory_(std::allocate_shared<detail::browser::allocator_resource<Allocator>>(
              allocator, allocator)) {}

    /** Selects the guest stream type for another executor.
     @tparam Executor Executor to bind to the next layer.
     */
    template <class Executor>
    struct rebind_executor {
        /** Stream type with the requested executor binding. */
        using other =
            stream<typename NextLayer::template rebind_executor<Executor>::other, DeflateSupported>;
    };
};
}  // namespace websocket
}
}  // namespace webcpp::beast

namespace webcpp::asio {
/** Closes a user-supplied browser byte transport after the WebSocket close exchange.
 @tparam Protocol Socket protocol of the supplied byte transport.
 @tparam Executor Executor associated with the socket.
 @param socket Byte transport to close.
 @param ec Receives any error from closing the socket.
 */
template <class Protocol, class Executor>
void teardown(boost::beast::role_type /*unused*/, basic_stream_socket<Protocol, Executor>& socket,
              boost::system::error_code& ec) {
    socket.close(ec);
}

/** Posts teardown completion using the handler's associations.
 @tparam Protocol Socket protocol of the supplied byte transport.
 @tparam Executor Executor associated with the socket.
 @tparam Token Completion token for void(error_code).
 @param role Protocol role passed through to teardown.
 @param socket Byte transport to close.
 @param token Completion token determining the result type.
 @return The result determined by the completion token.
 */
template <class Protocol, class Executor, class Token>
auto async_teardown(boost::beast::role_type role, basic_stream_socket<Protocol, Executor>& socket,
                    Token&& token) {
    return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
        [&socket, role](auto handler) {
            boost::system::error_code ec;
            teardown(role, socket, ec);
            webcpp::beast::detail::browser::post(socket.get_executor(), std::move(handler), ec);
        },
        token);
}
}  // namespace webcpp::asio
#endif
#endif
