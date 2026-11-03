// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_WEBSOCKET_HPP
#define WEBCPP_BEAST_BROWSER_WEBSOCKET_HPP

#include <webcpp/beast/config.hpp>

#include <boost/asio/buffer.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/steady_timer.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/beast/core/error.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/core/role.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/websocket/error.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/websocket/option.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/websocket/rfc6455.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/websocket/stream_base.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <limits>
#include <list>
#include <optional>
#include <vector>
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/beast/browser/detail/imports.hpp>
#include <webcpp/beast/browser/ssl_stream.hpp>
#include <webcpp/beast/browser/tcp_stream.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
/** Client or server role used by protocol options. */
using boost::beast::role_type;
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace websocket {
/** WebSocket close status codes. */
using boost::beast::websocket::close_code;
/** WebSocket close status and UTF-8 explanation. */
using boost::beast::websocket::close_reason;
/** Errors produced by WebSocket protocol operations. */
using boost::beast::websocket::error;
/** Tests whether a request asks to upgrade to WebSocket. */
using boost::beast::websocket::is_upgrade;
/** Compatibility record for browser-owned compression negotiation. */
using boost::beast::websocket::permessage_deflate;
/** Payload type for a WebSocket control frame. */
using boost::beast::websocket::ping_data;
/** HTTP request shape supplied to a WebSocket decorator. */
using request_type = boost::beast::http::request<
    boost::beast::http::empty_body,
    boost::beast::http::basic_fields<std::pmr::polymorphic_allocator<char>>>;
/** HTTP response shape used by the guest WebSocket engine. */
using boost::beast::websocket::response_type;
/**
 Kind of WebSocket control event.
 */
enum class frame_type {
    /** A closing handshake notification. */
    close,
    /** A browser-owned ping control frame. */
    ping,
    /** A browser-owned pong control frame. */
    pong,
};

/**
 Browser-compatible options, with a deferred allocator-aware decorator.
 */
struct stream_base {
    /**
     Handshake and idle timeout options shared with Beast.
     */
    using timeout = boost::beast::websocket::stream_base::timeout;

    /**
     Stores a callable that records the browser-compatible handshake subprotocols.
     @tparam Function Callable stored for request decoration or control notification.
     */
    template <class Function>
    struct decorator {
        /**
         Callable retained for later protocol notification.
         */
        Function function;

        /**
         Retains a handshake request decorator.
         @param fn Callable invoked with the handshake request before opening the browser socket.
         */
        explicit decorator(Function fn) : function(std::move(fn)) {}
    };
};

/**
 Bounds bytes buffered for sending and awaiting application reads.
 */
struct queue_limits {
    /**
     Maximum combined bufferedAmount and unsent fragment bytes, defaulting to 4 MiB.
     */
    std::size_t send = 4 * 1024 * 1024;
    /**
     Maximum retained payload plus message-record bytes in Wasm, defaulting to 4 MiB.

     Includes browser arrivals awaiting executor dispatch, even while the context is stopped.
     */
    std::size_t receive = 4 * 1024 * 1024;
};
}  // namespace websocket
}
}  // namespace webcpp::beast

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::detail {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace browser {
/**
 Type-erased decorator or control callback.
 */
struct callback_base {
    /**
     Invokes a request decorator when this object stores one.
     */
    virtual void decorate(websocket::request_type& /*unused*/) {};
    /**
     Invokes a control callback when this object stores one.
     */
    virtual void control(websocket::frame_type /*unused*/, boost::beast::string_view /*unused*/){};
    /**
     Releases registration and retained state during destruction.
     */
    virtual ~callback_base() = default;
};

/**
 Stores a callable using the stream allocator.
 */
template <class Function, bool Decorator>
struct callback final : callback_base {
    /**
     Callable retained for later protocol notification.
     */
    Function function;

    /**
     Initializes retained state and its executor or allocation resource.
     */
    explicit callback(Function value) : function(std::move(value)) {}

    /**
     Invokes a request decorator when this object stores one.
     */
    void decorate(websocket::request_type& request) override {
        if constexpr (Decorator) {
            function(request);
        }
    }

    /**
     Invokes a control callback when this object stores one.
     */
    void control(websocket::frame_type kind, boost::beast::string_view value) override {
        if constexpr (!Decorator) {
            function(kind, value);
        }
    }
};

/**
 Returns an English description of a browser error value.
 */
struct message {
    /**
     Decoded message bytes allocated from the stream resource.
     */
    std::pmr::vector<std::byte> bytes;
    /**
     Whether this complete message contains UTF-8 text.
     */
    bool text;
    /**
     Number of message bytes already delivered to the application.
     */
    std::size_t offset = 0;

    /**
     Initializes retained state and its executor or allocation resource.
     */
    message(std::pmr::memory_resource* resource, bool is_text) : bytes(resource), text(is_text) {}
};

/** Payload bookkeeping and the two queue links charged for every received message. */
inline constexpr std::size_t message_record_bytes = sizeof(message) + (2 * sizeof(void*));

/**
 Owns browser WebSocket state independently of the facade lifetime.
 */
// Registry lookup and shared ownership have independent lifetimes.
// NOLINTNEXTLINE(misc-multiple-inheritance)
class websocket_state final : public webcpp::asio::browser::intrusive_registration,
                              public std::enable_shared_from_this<websocket_state> {
public:
    /**
     Allocation resource kept alive while queued callbacks can access storage.
     */
    std::shared_ptr<std::pmr::memory_resource> resource;
    /**
     Executor that serializes state changes and completion posting.
     */
    executor_type executor;
    /**
     Browser event registry owned by the execution context.
     */
    webcpp::asio::browser::bridge* bridge;
    /**
     Timer enforcing the current operation deadline.
     */
    boost::asio::steady_timer timer;  // lint-world: browser operation deadline
    /**
     Registration key used to reject events from detached operations.
     */
    std::uint64_t key = 0;
    /**
     JavaScript WebSocket handle owned by this state.
     */
    emscripten::val socket = emscripten::val::undefined();
    /**
     Complete browser messages awaiting application reads.
     */
    std::pmr::list<message> messages;
    /** Browser arrivals charged to the receive budget before the executor runs. */
    std::pmr::list<message> ingress;
    /** One retained close event, whose callback runs only on the executor. */
    emscripten::val ingress_close = emscripten::val::undefined();
    /** Terminal ingress error awaiting executor-side completion. */
    error_code ingress_failure;
    /** Whether an opening notification awaits the executor. */
    bool ingress_open = false;
    /** Whether a single executor notification already owns the staged arrivals. */
    bool ingress_posted = false;
    /** Connection identity checked by posted arrivals and deadline callbacks. */
    std::uint64_t generation = 0;
    /**
     Outgoing fragments assembled before one browser send.
     */
    std::pmr::vector<std::byte> fragments;
    /**
     Allocator-owned request decorator, if installed.
     */
    std::shared_ptr<callback_base> decorator;
    /**
     Allocator-owned close notification callback, if installed.
     */
    std::shared_ptr<callback_base> control;
    /**
     Handshake and receive-idle timeout settings.
     */
    websocket::stream_base::timeout timeout =
        websocket::stream_base::timeout::suggested(boost::beast::role_type::client);
    /**
     Send and receive queue bounds enforced by this state.
     */
    websocket::queue_limits limits;
    /**
     Most recent browser-reported close status and reason.
     */
    websocket::close_reason reason;
    /**
     Terminal error retained for the next pending completion.
     */
    error_code failure;
    /**
     Total retained payload and message-record bytes in the queue.
     */
    std::size_t queued = 0;
    /**
     Maximum decoded message size accepted from the browser.
     */
    std::size_t maximum = 16 * 1024 * 1024;
    /**
     Whether the browser WebSocket has reported an open event.
     */
    bool opened = false;
    /**
     Whether the owning stream still accepts browser events.
     */
    bool alive = true;
    /**
     Whether the next outgoing message uses a text opcode.
     */
    bool send_text = false;
    /**
     Whether the last delivered message used a text opcode.
     */
    bool received_text = false;
    /**
     Whether the last read consumed the end of its message.
     */
    bool message_done = true;
    /**
     Pending opening-handshake completion, or null after completion.
     */
    pending* handshake = nullptr;
    /**
     Pending application read completion, or null when no read is outstanding.
     */
    pending* read = nullptr;
    /**
     Pending closing-handshake completion, or null after completion.
     */
    pending* close = nullptr;

    /**
     Initializes retained state and its executor or allocation resource.
     */
    websocket_state(executor_type ex, std::shared_ptr<std::pmr::memory_resource> storage)
        : resource(std::move(storage)),
          executor(std::move(ex)),
          bridge(&boost::asio::use_service<webcpp::asio::browser::bridge>(
              boost::asio::query(executor, boost::asio::execution::context))),
          timer(executor),
          messages(resource.get()),
          ingress(resource.get()),
          fragments(resource.get()) {}

    /**
     Releases registration and retained state during destruction.
     */
    ~websocket_state() { detach(); }

    /** Cancels the deadline without throwing during stream destruction. */
    void cancel_timer() noexcept {
#ifndef WEBCPP_BEAST_NO_EXCEPTIONS
        try {
            timer.cancel();
        } catch (...) {  // NOLINT(bugprone-empty-catch): cleanup must still detach and abort the
                         // host resource
            // Cleanup still detaches callbacks and aborts the host resource.
        }
#else
        timer.cancel();
#endif
    }

    /**
     Removes the registration so later browser events cannot reach this state.
     */
    void detach() {
        if ((bridge != nullptr) && (key != 0u)) {
            bridge->remove(std::exchange(key, 0));
        }
        if (!socket.isUndefined()) {
            for (const auto* name : {"onopen", "onmessage", "onclose", "onerror"}) {
                socket.set(name, emscripten::val::null());
            }
        }
    }

    /**
     Retains bounded arrivals and schedules at most one executor notification.
     Browser callbacks never invoke application handlers or manipulate their buffers.
     */
    bool deliver(const emscripten::val& event) override {
        if (!alive) {
            return false;
        }
        const auto type = event["type"].as<std::string>();
        if (type == "message") {
            if (failure || ingress_failure) {
                return false;
            }
            receive_message(event);
        } else if (type == "open") {
            ingress_open = true;
        } else if (type == "close") {
            ingress_close = event;
        } else {
            // Browser error events carry no cause; close owns the terminal result.
            return false;
        }
        if (!ingress_posted) {
            ingress_posted = true;
            const auto epoch = generation;
            auto const self = shared_from_this();
            post(executor,
                 boost::asio::bind_allocator(owning_allocator<std::byte>(resource), [self, epoch] {
                     if (self->alive && self->generation == epoch) {
                         self->drain_ingress();
                     }
                 }));
        }
        return false;
    }

    /** Retires connection-specific storage without changing the configured options. */
    void reset_arrivals() {
        ++generation;
        ingress.clear();
        messages.clear();
        fragments.clear();
        queued = 0;
        ingress_close = emscripten::val::undefined();
        ingress_failure.clear();
        ingress_open = false;
        ingress_posted = false;
        received_text = false;
        message_done = true;
    }

    /** Prepares a detached state for another browser connection. */
    void begin_connection() {
        detach();
        cancel_timer();
        abort_socket();
        reset_arrivals();
        socket = emscripten::val::undefined();
        failure.clear();
        reason = {};
        alive = true;
    }

    /**
     Closes the browser operation and its authority record.
     */
    void shutdown() noexcept override {
        bridge = nullptr;
        key = 0;
        alive = false;
        reset_arrivals();
        for (auto* const* item : {&handshake, &read, &close}) {
            if ((*item) != nullptr) {
                (*item)->discard();
            }
        }
        detach();
        abort_socket();
    }

    /**
     Requests browser closure and marks the WebSocket unavailable.
     */
    void abort_socket() {
        if (!socket.isUndefined()) {
            webcpp_beast_close(socket.as_handle(), 0, emscripten::val::undefined().as_handle());
        }
        opened = false;
    }

    /**
     Completes each outstanding operation with the supplied terminal error.
     */
    void finish(error_code ec) {
        cancel_timer();
        for (auto* const* item : {&handshake, &read, &close}) {
            if ((*item) != nullptr) {
                (*item)->complete(ec);
            }
        }
    }

    /** Retires the connection generation before completing a terminal operation. */
    void invalidate(error_code ec) {
        failure = ec;
        reset_arrivals();
        detach();
        abort_socket();
        cancel_timer();
    }

    /**
     Detaches the host resource and aborts all pending operations.
     */
    void stop() noexcept {
        alive = false;
        invalidate(boost::asio::error::operation_aborted);
        finish(boost::asio::error::operation_aborted);
    }

    /**
     Aborts the WebSocket and completes pending operations as cancelled.
     */
    void cancel_all() {
        invalidate(boost::asio::error::operation_aborted);
        finish(boost::asio::error::operation_aborted);
    }

    /**
     Schedules the absolute deadline for the active host operation.
     */
    void arm(::webcpp::beast::detail::browser::clock_type::time_point deadline) {
        cancel_timer();
        if (deadline == ::webcpp::beast::detail::browser::clock_type::time_point::max()) {
            return;
        }
        timer.expires_at(deadline);
        auto const weak = weak_from_this();
        const auto epoch = generation;
        timer.async_wait(boost::asio::bind_allocator(
            owning_allocator<std::byte>(resource), [weak, epoch](error_code ec) {
                if (auto const self = weak.lock();
                    self && !ec && self->alive && self->generation == epoch) {
                    self->invalidate(boost::beast::error::timeout);
                    self->finish(self->failure);
                }
            }));
    }

    /**
     Restarts the receive-idle timer after browser activity.
     */
    void arm_idle() {
        if (timeout.idle_timeout != boost::beast::websocket::stream_base::none()) {
            arm(::webcpp::beast::detail::browser::clock_type::now() + timeout.idle_timeout);
        } else {
            cancel_timer();
        }
    }

    /** Restores idle monitoring after cancellation without changing a handshake deadline. */
    void cancel_read() {
        if (handshake == nullptr && close == nullptr) {
            arm_idle();
        }
    }

    /** Applies staged arrivals and all operation completions on the stream executor. */
    void drain_ingress() {
        ingress_posted = false;
        if (std::exchange(ingress_open, false)) {
            opened = true;
            arm_idle();
            if (handshake != nullptr) {
                handshake->complete();
            }
        }
        if (ingress_failure) {
            failure = std::exchange(ingress_failure, error_code{});
            ingress.clear();
            messages.clear();
            queued = 0;
            abort_socket();
            if (read != nullptr) {
                read->complete(failure);
            }
        } else if (!ingress.empty()) {
            messages.splice(messages.end(), ingress);
            if (close == nullptr) {
                arm_idle();
            }
            if (read != nullptr) {
                read->complete();
            }
        }
        if (!ingress_close.isUndefined()) {
            const auto event = std::exchange(ingress_close, emscripten::val::undefined());
            receive_close(event);
        }
    }

    /** Charges and copies an arrival before the browser can deliver another message. */
    void receive_message(const emscripten::val& event) {
        const auto data = event["data"];
        const auto size = webcpp_beast_message_size(data.as_handle());
        const auto available = limits.receive - std::min(limits.receive, queued);
        if (size > maximum || size > available || message_record_bytes > available - size) {
            ingress_failure = size > maximum ? error_code(websocket::error::message_too_big)
                                             : make_error_code(browser_errc::receive_limit);
            // Stop host ingress immediately, even while the stream executor is paused.
            // The executor still owns completions, control callbacks and timers.
            webcpp_beast_close(socket.as_handle(), 0, emscripten::val::undefined().as_handle());
            return;
        }
        ingress.emplace_back(resource.get(), data.typeOf().as<std::string>() == "string");
        auto& message = ingress.back();
        message.bytes.resize(size);
        webcpp_beast_copy(data.as_handle(), message.bytes.data(), size);
        queued += size + message_record_bytes;
    }

    /** Records browser closure and completes pending operations with its status. */
    void receive_close(const emscripten::val& event) {
        detach();
        const auto epoch = generation;
        const bool was_open = opened;
        opened = false;
        cancel_timer();
        reason.code = static_cast<std::uint16_t>(event["code"].as<unsigned>());
        const auto text = js_text(event["reason"], resource.get());
        reason.reason = boost::beast::string_view(text.data(), text.size());
        if (control) {
            control->control(websocket::frame_type::close, reason.reason);
            if (generation != epoch) {
                return;
            }
        }
        if (!failure) {
            if (!was_open && handshake != nullptr) {
                failure = make_error_code(browser_errc::connection_failed);
            } else if (reason.code == 1006) {
                failure = boost::asio::error::connection_reset;
            } else {
                failure = websocket::error::closed;
            }
        }
        if (handshake != nullptr) {
            handshake->complete(failure);
        }
        if ((read != nullptr) && messages.empty()) {
            read->complete(failure);
        }
        if (close != nullptr) {
            close->complete(reason.code == 1006 ? failure : error_code{});
        }
    }

    /**
     Copies queued message bytes and updates partial-read bookkeeping.
     */
    template <class Buffers>
    std::size_t copy(const Buffers& buffers, std::size_t maximum_count) {
        auto& message = messages.front();
        auto size = std::min(maximum_count, message.bytes.size() - message.offset);
        size = boost::asio::buffer_copy(
            buffers, boost::asio::buffer(message.bytes.data() + message.offset, size));
        received_text = message.text;
        message.offset += size;
        message_done = message.offset == message.bytes.size();
        if (message_done) {
            queued -= message.bytes.size() + message_record_bytes;
            messages.pop_front();
        }
        return size;
    }
};

/**
 Allocates WebSocket state and its retained resource from the caller allocator.
 */
template <class Allocator>
std::shared_ptr<websocket_state> make_websocket_state(executor_type executor,
                                                      const Allocator& allocator) {
    auto resource = std::allocate_shared<allocator_resource<Allocator>>(allocator, allocator);
    return std::allocate_shared<websocket_state>(allocator, std::move(executor),
                                                 std::move(resource));
}
}  // namespace browser
}
}  // namespace webcpp::beast::detail

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif

namespace websocket {
/**
 Browser WebSocket with layered operations and Asio completion tokens.
 @tparam NextLayer Underlying browser authority or TLS-intent layer.
 @tparam DeflateSupported Compatibility parameter; the browser owns extension negotiation.
 */
template <class NextLayer, bool DeflateSupported = true>
class stream {
    NextLayer next_;
    std::shared_ptr<detail::browser::websocket_state> state_;
    using ec_type = boost::system::error_code;

    void bind() {
        get_lowest_layer(next_).bind(
            state_.get(),
            [](void* value) {
                static_cast<detail::browser::websocket_state*>(value)->cancel_all();
            },
            [](void* value) { static_cast<detail::browser::websocket_state*>(value)->stop(); });
    }

    void arm(bool handshake = false) {
        auto deadline = get_lowest_layer(next_).expiry();
        if (handshake &&
            state_->timeout.handshake_timeout != boost::beast::websocket::stream_base::none()) {
            deadline = std::min(deadline, ::webcpp::beast::detail::browser::clock_type::now() +
                                              state_->timeout.handshake_timeout);
        }
        if (deadline != ::webcpp::beast::detail::browser::clock_type::time_point::max()) {
            state_->arm(deadline);
        }
    }

    template <class Begin>
    void sync(Begin begin, ec_type& ec) {
        auto& context = static_cast<webcpp::asio::io_context&>(
            boost::asio::query(get_executor(), boost::asio::execution::context));
        if (context.get_executor().running_in_this_thread() || state_->bridge->delivering()) {
            ec = boost::asio::error::operation_not_supported;
            return;
        }
        bool done = false;
        begin([&](ec_type result) {
            ec = result;
            done = true;
        });
        context.restart();
        // NOLINTNEXTLINE(bugprone-infinite-loop): run_one invokes the callback that sets done
        while (!done) {
            context.run_one();
        }
    }

    static void add_protocols(const emscripten::val& protocols, std::string_view value) {
        while (!value.empty()) {
            const auto comma = value.find(',');
            auto name = value.substr(0, comma);
            while (!name.empty() && name.front() == ' ') {
                name.remove_prefix(1);
            }
            while (!name.empty() && name.back() == ' ') {
                name.remove_suffix(1);
            }
            protocols.call<void>("push", detail::browser::js_string(name));
            if (comma == std::string_view::npos) {
                break;
            }
            value.remove_prefix(comma + 1);
        }
    }

    emscripten::val protocols(ec_type& ec) {
        request_type request(std::piecewise_construct, std::tuple<>(),
                             std::make_tuple(state_->resource.get()));
        if (state_->decorator) {
            state_->decorator->decorate(request);
        }
        auto const protocols = emscripten::val::array();
        for (const auto& field : request) {
            if (field.name() != boost::beast::http::field::sec_websocket_protocol) {
                if (auto const error = detail::browser::unsupported()) {
                    ec = error;
                }
            } else {
                add_protocols(protocols, {field.value().data(), field.value().size()});
            }
        }
        return protocols;
    }

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
        using other =
            stream<typename NextLayer::template rebind_executor<Executor>::other, DeflateSupported>;
    };

    /**
     Constructs the next layer and allocator-owned WebSocket state.
     @tparam First Type of the first argument forwarded to the next layer.
     @tparam Args Types of the remaining arguments forwarded to the next layer.
     @param first First argument forwarded to the next layer constructor.
     @param args Remaining arguments forwarded to the next layer constructor.
     */
    template <class First, class... Args>
        requires(!std::is_same_v<std::remove_cvref_t<First>, std::allocator_arg_t>)
    explicit stream(First&& first, Args&&... args)
        : next_(std::forward<First>(first), std::forward<Args>(args)...),
          state_(detail::browser::make_websocket_state(next_.get_executor(),
                                                       std::allocator<std::byte>{})) {
        bind();
    }

    /**
     Constructs the next layer and allocator-owned WebSocket state.
     @tparam Allocator Allocator used for owned state and variable-sized stream storage.
     @tparam Args Types of the remaining arguments forwarded to the next layer.
     @param allocator Allocator retained for stream-owned storage.
     @param args Remaining arguments forwarded to the next layer constructor.
     */
    template <class Allocator, class... Args>
    explicit stream(std::allocator_arg_t /*unused*/, const Allocator& allocator, Args&&... args)
        : next_(std::allocator_arg, allocator, std::forward<Args>(args)...),
          state_(detail::browser::make_websocket_state(next_.get_executor(), allocator)) {
        bind();
    }

    /**
     Disables copying of stream ownership.
     */
    stream(const stream&) = delete;
    /**
     Disables assignment of stream ownership.
     */
    stream& operator=(const stream&) = delete;

    /**
     Transfers the stream and its pending operation ownership.
     @param other Stream whose state and operation ownership are transferred.
     */
    stream(stream&& other) noexcept
        : next_(std::move(other.next_)), state_(std::move(other.state_)) {
        bind();
    }

    /**
     Disables assignment of stream ownership.
     */
    stream& operator=(stream&&) = delete;

    /**
     Aborts pending host operations and releases browser registrations.
     */
    ~stream() {
        if (state_) {
            get_lowest_layer(next_).bind(nullptr, nullptr, nullptr);
            state_->stop();
        }
    }

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
     Returns the executor used for asynchronous completions.
     @return The executor associated with the underlying context.
     */
    [[nodiscard]] executor_type get_executor() const { return next_.get_executor(); }

    /**
     Reports whether the browser WebSocket is currently open.
     @return True when the layer is available for operations.
     */
    [[nodiscard]] bool is_open() const noexcept { return state_->opened; }

    /**
     Reports whether the last read delivered a text message.
     @return True if the last delivered message was text.
     */
    [[nodiscard]] bool got_text() const noexcept { return state_->received_text; }

    /**
     Reports whether the last read delivered a binary message.
     @return True if the last delivered message was binary.
     */
    [[nodiscard]] bool got_binary() const noexcept { return !got_text(); }

    /**
     Reports whether the last read reached the end of its message.
     @return True when the last read consumed the message boundary.
     */
    [[nodiscard]] bool is_message_done() const noexcept { return state_->message_done; }

    /**
     Reports whether subsequent messages use text mode.
     @return True if outgoing messages use text mode.
     */
    [[nodiscard]] bool text() const noexcept { return state_->send_text; }

    /**
     Selects text mode for subsequent messages.
     @param value Whether subsequent messages are text.
     */
    void text(bool value) noexcept { state_->send_text = value; }

    /**
     Reports whether subsequent messages use binary mode.
     @return True if outgoing messages use binary mode.
     */
    [[nodiscard]] bool binary() const noexcept { return !text(); }

    /**
     Selects binary mode for subsequent messages.
     @param value Whether subsequent messages are binary.
     */
    void binary(bool value) noexcept { text(!value); }

    /**
     Returns the last close status and reason exposed by the browser.
     @return The most recently observed close reason.
     */
    [[nodiscard]] const close_reason& reason() const noexcept { return state_->reason; }

    /**
     Sets the largest decoded message accepted from the browser.
     @param value Maximum decoded message bytes; the default is 16 MiB.
     */
    void read_message_max(std::size_t value) noexcept { state_->maximum = value; }

    /**
     Returns the maximum accepted decoded message size.
     @return The maximum accepted message bytes.
     */
    [[nodiscard]] std::size_t read_message_max() const noexcept { return state_->maximum; }

    /**
     Accepts the browser-owned fragmentation setting.
     */
    void auto_fragment(bool /*unused*/) noexcept {}

    /**
     Reports that fragmentation is owned by the browser.
     @return True, because browser framing is not application-controlled.
     */
    [[nodiscard]] bool auto_fragment() const noexcept { return true; }

    /**
     Accepts a compatibility buffer hint without changing browser buffering.
     */
    void write_buffer_bytes(std::size_t /*unused*/) noexcept {}

    /**
     Returns the compatibility write-buffer hint.
     @return The compatibility value 4096.
     */
    [[nodiscard]] std::size_t write_buffer_bytes() const noexcept { return 4096; }

    /**
     Accepts browser-owned compression negotiation options.
     */
    void set_option(const permessage_deflate& /*unused*/) noexcept {}

    /**
     Records timeout settings for subsequent host operations.
     @param value Options to retain for subsequent operations.
     */
    void set_option(const stream_base::timeout& value) { state_->timeout = value; }

    /**
     Sets the byte limits enforced for outgoing and unread messages.
     @param value Options to retain for subsequent operations.
     */
    void set_option(queue_limits value) { state_->limits = value; }

    /**
     Stores a request decorator using the stream allocator.
     @tparam Function Callable stored for request decoration or control notification.
     @param value Options to retain for subsequent operations.
     */
    template <class Function>
    void set_option(stream_base::decorator<Function> value) {
        state_->decorator = std::allocate_shared<detail::browser::callback<Function, true>>(
            detail::browser::owning_allocator<std::byte>(state_->resource),
            std::move(value.function));
    }

    /**
     Installs the callback notified of browser-visible close events.
     @tparam Function Callable stored for request decoration or control notification.
     @param function Callable receiving the control frame kind and payload.
     */
    template <class Function>
    void control_callback(Function function) {
        state_->control = std::allocate_shared<detail::browser::callback<Function, false>>(
            detail::browser::owning_allocator<std::byte>(state_->resource), std::move(function));
    }

    /**
     Opens a browser WebSocket using the recorded authority and request target.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param host Hostname to match against the recorded destination authority.
     @param target Origin-form path and query for the browser URL.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_handshake(boost::beast::string_view host, boost::beast::string_view target,
                         Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type)>(
            [this, host = std::pmr::string(host, state_->resource.get()),
             target = std::pmr::string(target, state_->resource.get())](auto handler) {
                auto& state = *state_;
                auto const& record = get_lowest_layer(next_);
                ec_type ec = detail::browser::unsupported(!record.authority_matches(host) ||
                                                          state.timeout.keep_alive_pings);
                if (!record.is_open()) {
                    ec = make_error_code(browser_errc::no_host);
                }
                if (state.handshake || state.opened || state.read || state.close) {
                    ec = boost::asio::error::already_started;
                }
                const emscripten::val protocols = this->protocols(ec);
                if (ec) {
                    detail::browser::post(get_executor(), std::move(handler), ec);
                    return;
                }
                state.begin_connection();
                bind();
                detail::browser::start<false>(
                    state.handshake, get_executor(), std::move(handler),
                    [](ec_type&, std::size_t&) {},
                    boost::asio::cancellation_type::terminal |
                        boost::asio::cancellation_type::partial,
                    [state = state_] { state->invalidate(boost::asio::error::operation_aborted); });
                state.key = state.bridge->add(state);
                auto const url = detail::browser::js_string(record.url(target, true));
                const emscripten::val result = emscripten::val::take_ownership(
                    webcpp_beast_open(url.as_handle(), protocols.as_handle(),
                                      state.bridge->function(state.key).as_handle()));
                const auto failure = result["error"].as<int>();
                if (failure) {
                    state.handshake->complete(make_error_code(static_cast<browser_errc>(failure)));
                    state.detach();
                    return;
                }
                state.socket = result["socket"];
                arm(true);
            },
            token);
    }

    /**
     Receives one complete WebSocket message into a dynamic buffer.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     Cancellation leaves queued messages available for a subsequent read.
     Only one read may be outstanding, and the destination must outlive completion.
     @tparam DynamicBuffer DynamicBuffer type receiving message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffer Dynamic buffer receiving message bytes and retained until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class DynamicBuffer, class Token>
    auto async_read(DynamicBuffer& buffer, Token&& token) {
        return async_read_some(buffer, std::numeric_limits<std::size_t>::max(),
                               std::forward<Token>(token));
    }

    /**
     Receives part of one queued WebSocket message.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     Cancellation leaves queued messages available for a subsequent read.
     Only one read may be outstanding, and the destination must outlive completion.
     @tparam DynamicBuffer DynamicBuffer type receiving message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffer Dynamic buffer receiving message bytes and retained until completion.
     @param maximum Maximum message bytes to append in this read.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class DynamicBuffer, class Token>
    auto async_read_some(DynamicBuffer& buffer, std::size_t maximum, Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type, std::size_t)>(
            [this, &buffer, maximum](auto handler) {
                if (state_->read) {
                    detail::browser::post(get_executor(), std::move(handler),
                                          ec_type(boost::asio::error::already_started),
                                          std::size_t{});
                    return;
                }
                detail::browser::start<true>(
                    state_->read, get_executor(), std::move(handler),
                    [state = state_, &buffer, maximum](ec_type& ec, std::size_t& count) {
                        if (ec) {
                            return;
                        }
                        auto const& message = state->messages.front();
                        const auto size = std::min(maximum, message.bytes.size() - message.offset);
                        if (size > buffer.max_size() - buffer.size()) {
                            ec = boost::asio::error::no_buffer_space;
                            return;
                        }
                        count = state->copy(buffer.prepare(size), size);
                        buffer.commit(count);
                    },
                    boost::asio::cancellation_type::all,
                    [state = state_] { state->cancel_read(); });
                if (!state_->messages.empty()) {
                    state_->read->complete();
                } else if (state_->failure) {
                    state_->read->complete(state_->failure);
                } else if (!state_->opened) {
                    state_->read->complete(boost::asio::error::not_connected);
                } else {
                    arm();
                }
            },
            token);
    }

    /**
     Receives part of one queued WebSocket message.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     Cancellation leaves queued messages available for a subsequent read.
     Only one read may be outstanding, and the destination must outlive completion.
     @tparam Buffers MutableBufferSequence receiving response or message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Buffers, class Token>
    auto async_read_some(const Buffers& buffers, Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type, std::size_t)>(
            [this, buffers](auto handler) {
                if (state_->read) {
                    detail::browser::post(get_executor(), std::move(handler),
                                          ec_type(boost::asio::error::already_started),
                                          std::size_t{});
                    return;
                }
                detail::browser::start<true>(
                    state_->read, get_executor(), std::move(handler),
                    [state = state_, buffers](ec_type& ec, std::size_t& count) {
                        if (!ec) {
                            count = state->copy(buffers, boost::asio::buffer_size(buffers));
                        }
                    },
                    boost::asio::cancellation_type::all,
                    [state = state_] { state->cancel_read(); });
                if (!state_->messages.empty()) {
                    state_->read->complete();
                } else if (state_->failure) {
                    state_->read->complete(state_->failure);
                } else if (!state_->opened) {
                    state_->read->complete(boost::asio::error::not_connected);
                } else {
                    arm();
                }
            },
            token);
    }

    /**
     Buffers a message fragment and submits the assembled final message.

     Success means the browser accepted the bytes into its queue, not that the peer read them.
     Exceeding queue_limits::send closes the connection with browser_errc::send_limit.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @param fin Whether this fragment finishes the message and submits it to the browser.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param ec Receives the operation error, or is cleared on success.
     @return The number of input bytes accepted, or zero on failure.
     */
    template <class Buffers>
    std::size_t write_some(bool fin, const Buffers& buffers, ec_type& ec) {
        const auto count = boost::asio::buffer_size(buffers);
        auto& state = *state_;
        ec.clear();
        if (!state.opened) {
            ec = boost::asio::error::not_connected;
            return 0;
        }
        const auto queued = static_cast<std::size_t>(state.socket["bufferedAmount"].as<double>());
        if (queued > state.limits.send || state.fragments.size() > state.limits.send - queued ||
            count > state.limits.send - queued - state.fragments.size()) {
            ec = make_error_code(browser_errc::send_limit);
            state.failure = ec;
            state.abort_socket();
            return 0;
        }
        const auto old = state.fragments.size();
        state.fragments.resize(old + count);
        boost::asio::buffer_copy(boost::asio::buffer(state.fragments.data() + old, count), buffers);
        if (fin) {
            if (webcpp_beast_send(state.socket.as_handle(), state.fragments.data(),
                                  state.fragments.size(), state.send_text)) {
                ec = boost::asio::error::connection_reset;
            }
            state.fragments.clear();
        }
        return ec ? 0 : count;
    }

    /**
     Submits a complete message to the browser send queue.

     Success means the browser accepted the bytes into its queue, not that the peer read them.
     Exceeding queue_limits::send closes the connection with browser_errc::send_limit.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param ec Receives the operation error, or is cleared on success.
     @return The number of input bytes accepted, or zero on failure.
     */
    template <class Buffers>
    std::size_t write(const Buffers& buffers, ec_type& ec) {
        return write_some(true, buffers, ec);
    }

    /**
     Submits a complete message to the browser send queue.

     Success means the browser accepted the bytes into its queue, not that the peer read them.
     Exceeding queue_limits::send closes the connection with browser_errc::send_limit.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @return The number of input bytes accepted, or zero on failure.
     */
    template <class Buffers>
    std::size_t write(const Buffers& buffers) {
        ec_type ec;
        const auto n = write(buffers, ec);
        detail::browser::throw_error(ec);
        return n;
    }

    /**
     Buffers a message fragment and submits the assembled final message.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     Success means the browser accepted the bytes into its queue, not that the peer read them.
     Exceeding queue_limits::send closes the connection with browser_errc::send_limit.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param fin Whether this fragment finishes the message and submits it to the browser.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Buffers, class Token>
    auto async_write_some(bool fin, const Buffers& buffers, Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type, std::size_t)>(
            [this, fin, buffers](auto handler) {
                ec_type ec;
                auto count = write_some(fin, buffers, ec);
                detail::browser::post(get_executor(), std::move(handler), ec, count);
            },
            token);
    }

    /**
     Submits a complete message to the browser send queue.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, std::size_t)`.
     Success means the browser accepted the bytes into its queue, not that the peer read them.
     Exceeding queue_limits::send closes the connection with browser_errc::send_limit.
     @tparam Buffers ConstBufferSequence providing request or message bytes.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param buffers Buffer sequence whose storage remains valid until completion.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Buffers, class Token>
    auto async_write(const Buffers& buffers, Token&& token) {
        return async_write_some(true, buffers, std::forward<Token>(token));
    }

    /**
     Performs the browser closing handshake and waits for its close event.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param reason Close code and UTF-8 reason passed to the browser.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_close(close_reason reason, Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type)>(
            [this, reason](auto handler) mutable {
                auto ec = detail::browser::unsupported(reason.code != 1000 &&
                                                       (reason.code < 3000 || reason.code > 4999));
                if (state_->close) {
                    ec = boost::asio::error::already_started;
                }
                if (!state_->opened) {
                    ec = boost::asio::error::not_connected;
                }
                if (ec) {
                    detail::browser::post(get_executor(), std::move(handler), ec);
                    return;
                }
                if (reason.code != 1000 && (reason.code < 3000 || reason.code > 4999)) {
                    reason.code = 1000;
                }
                detail::browser::start<false>(
                    state_->close, get_executor(), std::move(handler),
                    [](ec_type&, std::size_t&) {},
                    boost::asio::cancellation_type::terminal |
                        boost::asio::cancellation_type::partial,
                    [] {});
                webcpp_beast_close(state_->socket.as_handle(), reason.code,
                                   detail::browser::js_string(reason.reason).as_handle());
                arm();
            },
            token);
    }

    /**
     Completes according to the unsupported-operation policy for browser-owned control frames.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_ping(const ping_data& /*unused*/, Token&& token) {
        return boost::asio::async_initiate<Token, void(ec_type)>(
            [this](auto handler) {
                detail::browser::post(get_executor(), std::move(handler),
                                      detail::browser::unsupported());
            },
            token);
    }

    /**
     Completes according to the unsupported-operation policy for browser-owned control frames.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param data Control payload, which the browser cannot transmit explicitly.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_pong(const ping_data& data, Token&& token) {
        return async_ping(data, std::forward<Token>(token));
    }

    /**
     Opens a browser WebSocket using the recorded authority and request target.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @param host Hostname to match against the recorded destination authority.
     @param target Origin-form path and query for the browser URL.
     @param ec Receives the operation error, or is cleared on success.
     */
    void handshake(boost::beast::string_view host, boost::beast::string_view target, ec_type& ec) {
        sync([&](auto handler) { async_handshake(host, target, std::move(handler)); }, ec);
    }

    /**
     Opens a browser WebSocket using the recorded authority and request target.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @param host Hostname to match against the recorded destination authority.
     @param target Origin-form path and query for the browser URL.
     */
    void handshake(boost::beast::string_view host, boost::beast::string_view target) {
        ec_type ec;
        handshake(host, target, ec);
        detail::browser::throw_error(ec);
    }

    /**
     Performs the browser closing handshake and waits for its close event.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @param reason Close code and UTF-8 reason passed to the browser.
     @param ec Receives the operation error, or is cleared on success.
     */
    void close(close_reason reason, ec_type& ec) {
        sync([&](auto handler) { async_close(reason, std::move(handler)); }, ec);
    }

    /**
     Performs the browser closing handshake and waits for its close event.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @param reason Close code and UTF-8 reason passed to the browser.
     */
    void close(close_reason reason) {
        ec_type ec;
        close(reason, ec);
        detail::browser::throw_error(ec);
    }

    /**
     Receives one complete WebSocket message into a dynamic buffer.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @tparam Buffer Dynamic buffer type retained for Beast call compatibility.
     @param buffer Dynamic buffer receiving message bytes and retained until completion.
     @param ec Receives the operation error, or is cleared on success.
     @return The number of bytes delivered to the destination.
     */
    template <class Buffer>
    std::size_t read(Buffer& buffer, ec_type& ec) {
        std::size_t n = 0;
        sync(
            [&](auto handler) {
                async_read(buffer, [&, handler = std::move(handler)](ec_type error,
                                                                     std::size_t count) mutable {
                    n = count;
                    handler(error);
                });
            },
            ec);
        return n;
    }

    /**
     Receives one complete WebSocket message into a dynamic buffer.

     This synchronous facade suspends the main stack through JSPI or Asyncify.
     Calling it from an executing handler returns operation_not_supported.
     @tparam Buffer Dynamic buffer type retained for Beast call compatibility.
     @param buffer Dynamic buffer receiving message bytes and retained until completion.
     @return The number of bytes delivered to the destination.
     */
    template <class Buffer>
    std::size_t read(Buffer& buffer) {
        ec_type ec;
        const auto n = read(buffer, ec);
        detail::browser::throw_error(ec);
        return n;
    }
};
}  // namespace websocket
}
}  // namespace webcpp::beast
#endif
