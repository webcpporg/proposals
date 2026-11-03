// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_WEBSOCKET_HPP
#define WEBCPP_BEAST_WEBSOCKET_HPP

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP3)
#include <boost/beast/websocket.hpp>  // lint-world: explicit guest WebSocket protocol engine

#include <memory>
#include <type_traits>
#include <utility>

namespace webcpp::beast::websocket {
/** Reports whether a request asks for a WebSocket upgrade. */
using boost::beast::websocket::is_upgrade;
/** Upstream WebSocket closing status codes. */
using boost::beast::websocket::close_code;
/** Upstream WebSocket close code and UTF-8 reason. */
using boost::beast::websocket::close_reason;
/** Upstream WebSocket operation errors. */
using boost::beast::websocket::error;
/** WebSocket control-frame category. */
using boost::beast::websocket::frame_type;
/** Bounded WebSocket ping or pong payload. */
using boost::beast::websocket::ping_data;
/** Upstream per-message compression settings. */
using boost::beast::websocket::permessage_deflate;
/** Upstream handshake request type. */
using boost::beast::websocket::request_type;
/** Upstream handshake response type. */
using boost::beast::websocket::response_type;
/** Upstream WebSocket timeout and decorator settings. */
using boost::beast::websocket::stream_base;

/**
 Runs the upstream WebSocket protocol over an explicitly selected byte stream.

 @tparam NextLayer The underlying AsyncStream or SyncStream type.
 @tparam DeflateSupported Whether upstream per-message deflate is compiled in.
 @note Native and WASIp3 state allocation remains upstream Beast's; associated
 handler allocators are preserved by upstream operations.
*/
template <class NextLayer, bool DeflateSupported = true>
class stream : public boost::beast::websocket::stream<NextLayer, DeflateSupported> {
    using base = boost::beast::websocket::stream<NextLayer, DeflateSupported>;

public:
    /**
     Rebinds this facade to another executor.
     @tparam Executor The executor type for the underlying stream.
    */
    template <class Executor>
    struct rebind_executor {
        /** The facade stream using the rebound underlying stream. */
        using other =
            stream<typename base::next_layer_type::template rebind_executor<Executor>::other,
                   DeflateSupported>;
    };

    /** Constructs a stream when the underlying stream is default constructible. */
    stream()
        requires std::is_default_constructible_v<NextLayer>
        : base() {}

    /**
     Constructs the underlying stream without consuming an allocator tag.
     @tparam First The first underlying-stream argument type.
     @tparam Rest The remaining argument types.
     @param first The first underlying-stream argument.
     @param rest The remaining underlying-stream arguments.
    */
    template <class First, class... Rest>
        requires(!std::is_same_v<std::remove_cvref_t<First>, std::allocator_arg_t>)
    explicit stream(First&& first, Rest&&... rest)
        : base(std::forward<First>(first), std::forward<Rest>(rest)...) {}

    /**
     Converts an upstream-compatible stream with another underlying executor.
     @tparam Other The source underlying stream type.
     @param other The source stream, forwarded to upstream's converting constructor.
     @note As upstream does, this moves the next layer, not established protocol state.
    */
    template <class Other>
    explicit stream(stream<Other, DeflateSupported>&& other)
        : base(std::move(other.next_layer())) {}

    /**
     Constructs the socket backend with the facade's portable allocator syntax.

     @tparam Allocator The allocator type accepted for source compatibility.
     @tparam Args The underlying stream's constructor argument types.
     @param tag Selects the allocator-aware facade constructor.
     @param allocator Accepted without changing upstream stream-state allocation.
     @param args Arguments forwarded to the upstream stream.
     @note This backend does not claim that the supplied allocator controls upstream state.
    */
    template <class Allocator, class... Args>
    explicit stream([[maybe_unused]] std::allocator_arg_t tag,
                    [[maybe_unused]] const Allocator& allocator, Args&&... args)
        : base(std::forward<Args>(args)...) {}
};
}  // namespace webcpp::beast::websocket
#elif defined(WEBCPP_TARGET_EMSCRIPTEN)
#include <webcpp/beast/browser/websocket.hpp>
#else
#error "The Beast WebSocket socket backend currently requires native or WASIp3"
#endif

#endif
