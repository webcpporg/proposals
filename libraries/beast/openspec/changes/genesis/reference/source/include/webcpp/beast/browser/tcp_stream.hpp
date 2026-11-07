// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_TCP_STREAM_HPP
#define WEBCPP_BEAST_BROWSER_TCP_STREAM_HPP

#include <webcpp/beast/config.hpp>

#include <arpa/inet.h>  // lint-world: allocation-free numeric address formatting, without I/O
#include <array>
#include <boost/beast/core/stream_traits.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <charconv>
#include <chrono>
#include <string>
#include <webcpp/asio.hpp>
#include <webcpp/beast/browser/detail/support.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
/** Finds the innermost layer of a composed stream. */
using boost::beast::get_lowest_layer;
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
/**
 A browser authority record, with Beast's connection and deadline interface.
 */
class tcp_stream {
public:
    /**
     Executor used to schedule stream operations and their completions.
     */
    using executor_type = boost::asio::any_io_executor;
    /**
     Address and service value recorded by connect.
     */
    using endpoint_type = webcpp::asio::ip::tcp::endpoint;
    /**
     The authority record at the bottom of a browser stream stack.
     */
    using lowest_layer_type = tcp_stream;

    /**
     Selects the stream type for a different executor.
     @tparam Executor Executor type to bind to the underlying layer.
     */
    template <class Executor>
    struct rebind_executor {
        /**
         Stream type with the requested executor binding.
         */
        using other = tcp_stream;
    };

    /**
     Constructs an authority record with no active connection.
     @param executor Executor used to post asynchronous completions.
     */
    explicit tcp_stream(executor_type executor)
        : tcp_stream(std::allocator_arg, std::allocator<std::byte>{}, std::move(executor)) {}

    /**
     Constructs an authority record with no active connection.
     @tparam Context Execution context providing get_executor().
     @param context Execution context owning the stream executor.
     */
    template <class Context>
        requires requires(Context& context) { context.get_executor(); }
    explicit tcp_stream(Context& context) : tcp_stream(context.get_executor()) {}

    /**
     Constructs an authority record with no active connection.
     @tparam Allocator Allocator used for owned state and variable-sized stream storage.
     @param allocator Allocator retained for stream-owned storage.
     @param executor Executor used to post asynchronous completions.
     */
    template <class Allocator>
    tcp_stream(std::allocator_arg_t /*unused*/, const Allocator& allocator, executor_type executor)
        : executor_(std::move(executor)),
          memory_(std::allocate_shared<detail::browser::allocator_resource<Allocator>>(allocator,
                                                                                       allocator)),
          host_(memory_.get()) {}

    /**
     Constructs an authority record with no active connection.
     @tparam Allocator Allocator used for owned state and variable-sized stream storage.
     @tparam Context Execution context providing get_executor().
     @param tag Selects the allocator-aware constructor.
     @param allocator Allocator retained for stream-owned storage.
     @param context Execution context owning the stream executor.
     */
    template <class Allocator, class Context>
        requires requires(Context& context) { context.get_executor(); }
    tcp_stream(std::allocator_arg_t tag, const Allocator& allocator, Context& context)
        : tcp_stream(tag, allocator, executor_type(context.get_executor())) {}

    /**
     Returns the allocation resource retained by this stream.
     @return The stream-owned resource; its lifetime is tied to retained resource ownership.
     */
    [[nodiscard]] std::pmr::memory_resource* resource() const noexcept { return memory_.get(); }

    /**
     Returns shared ownership of the stream allocation resource.
     @return Shared ownership of the stream allocation resource.
     */
    [[nodiscard]] const std::shared_ptr<std::pmr::memory_resource>& resource_owner()
        const noexcept {
        return memory_;
    }

    /**
     Transfers the stream and its pending operation ownership.
     @param other Stream whose state and operation ownership are transferred.
     */
    tcp_stream(tcp_stream&& other) noexcept
        : http_state(std::move(other.http_state)),
          executor_(std::move(other.executor_)),
          memory_(std::move(other.memory_)),
          host_(std::move(other.host_)),
          port_(other.port_),
          response_body_limit_(other.response_body_limit_),
          open_(std::exchange(other.open_, false)),
          secure_(other.secure_),
          deadline_(other.deadline_),
          owner_(std::exchange(other.owner_, nullptr)),
          cancel_(std::exchange(other.cancel_, nullptr)),
          close_(std::exchange(other.close_, nullptr)) {}

    /**
     Disables assignment of stream ownership.
     */
    tcp_stream& operator=(tcp_stream&&) = delete;
    /**
     Disables copying of stream ownership.
     */
    tcp_stream(const tcp_stream&) = delete;
    /**
     Disables assignment of stream ownership.
     */
    tcp_stream& operator=(const tcp_stream&) = delete;

    /**
     Aborts pending host operations and releases browser registrations.
     */
    ~tcp_stream() { close(); }

    /**
     Returns the executor used for asynchronous completions.
     @return The executor associated with the underlying context.
     */
    [[nodiscard]] executor_type get_executor() const noexcept { return executor_; }

    /**
     Returns this authority record through the Beast lowest-layer interface.
     @return A reference to this authority record.
     */
    // lint-std: Beast basic_stream::socket returns its lowest transport layer
    tcp_stream& socket() noexcept { return *this; }  // lint-world: authority facade only

    /**
     Returns this authority record through the Beast lowest-layer interface.
     @return A reference to this authority record.
     */
    // lint-std: Beast basic_stream::socket returns its lowest transport layer
    [[nodiscard]] const tcp_stream& socket() const noexcept {  // lint-world: authority facade only
        return *this;
    }

    /**
     Reports whether a connection authority has been recorded.
     @return True when the layer is available for operations.
     */
    [[nodiscard]] bool is_open() const noexcept { return open_; }

    /**
     Sets an absolute deadline relative to the current steady-clock time.
     @tparam Rep Representation type of the duration count.
     @tparam Period Tick period of the duration.
     @param duration Time remaining until the deadline.
     */
    template <class Rep, class Period>
    void expires_after(std::chrono::duration<Rep, Period> duration) {
        deadline_ = ::webcpp::beast::detail::browser::clock_type::now() + duration;
    }

    /**
     Sets the deadline for subsequent browser operations.
     @param time Absolute steady-clock deadline.
     */
    void expires_at(::webcpp::beast::detail::browser::clock_type::time_point time) {
        deadline_ = time;
    }

    /**
     Disables the authority record deadline.
     */
    void expires_never() {
        deadline_ = ::webcpp::beast::detail::browser::clock_type::time_point::max();
    }

    /**
     Returns the deadline recorded for browser operations.
     @return The recorded deadline, or time_point::max() when disabled.
     */
    [[nodiscard]] auto expiry() const noexcept { return deadline_; }

    /**
     Cancels the active host operation and posts its aborted completion.
     */
    void cancel() {
        if (cancel_ != nullptr) {
            cancel_(owner_);
        }
    }

    /**
     Cancels the active host operation and posts its aborted completion.
     @param ec Receives the operation error, or is cleared on success.
     */
    void cancel(boost::system::error_code& ec) {
        cancel();
        ec.clear();
    }

    /**
     Closes the authority record and aborts its active host operation.
     */
    void close() {
        if (close_ != nullptr) {
            close_(owner_);
        }
        open_ = false;
    }

    /**
     Closes the authority record and aborts its active host operation.
     @param ec Receives the operation error, or is cleared on success.
     */
    void close(boost::system::error_code& ec) {
        close();
        ec.clear();
    }

    /**
     Closes the browser operation and its authority record.
     @param ec Receives the operation error, or is cleared on success.
     */
    void shutdown(boost::asio::socket_base::shutdown_type /*unused*/,
                  boost::system::error_code& ec) {
        close(ec);
    }

    /**
     Closes the browser operation and its authority record.
     @param what Shutdown direction; browser host operations close as a whole.
     */
    void shutdown(boost::asio::socket_base::shutdown_type what) {
        boost::system::error_code ec;
        shutdown(what, ec);
    }

    /**
     Records the destination authority without opening a raw TCP connection.
     @param results Resolver results supplying the original hostname and service.
     @param ec Receives the operation error, or is cleared on success.
     @return The endpoint corresponding to the recorded authority, or an empty endpoint on failure.
     */
    endpoint_type connect(const webcpp::asio::ip::tcp::resolver::results_type& results,
                          boost::system::error_code& ec) {
        if (results.empty()) {
            ec = boost::asio::error::host_not_found;
            return {};
        }
        reset_operation();
        const auto& entry = *results.begin();
        host_ = entry.host_name(std::pmr::polymorphic_allocator<char>(memory_.get()));
        port_ = entry.endpoint().port();
        if (host_.empty()) {
            return connect(entry.endpoint(), ec);
        }
        if (host_.find(':') != std::string::npos && host_.front() != '[') {
            host_.insert(host_.begin(), '[');
            host_.push_back(']');
        }
        open_ = true;
        ec.clear();
        return entry.endpoint();
    }

    /**
     Records the destination authority without opening a raw TCP connection.
     @param results Resolver results supplying the original hostname and service.
     @return The endpoint corresponding to the recorded authority, or an empty endpoint on failure.
     */
    endpoint_type connect(const webcpp::asio::ip::tcp::resolver::results_type& results) {
        boost::system::error_code ec;
        auto result = connect(results, ec);
        detail::browser::throw_error(ec);
        return result;
    }

    /**
     Records the destination authority without opening a raw TCP connection.
     @param endpoint Address and port to record as the destination.
     @param ec Receives the operation error, or is cleared on success.
     @return The endpoint corresponding to the recorded authority, or an empty endpoint on failure.
     */
    endpoint_type connect(const endpoint_type& endpoint, boost::system::error_code& ec) {
        if (endpoint.address().is_unspecified()) {
            ec = make_error_code(browser_errc::no_host);
            return {};
        }
        reset_operation();
        std::array<char, INET6_ADDRSTRLEN> text{};
        const auto address = endpoint.address();
        if (address.is_v6()) {
            const auto bytes = address.to_v6().to_bytes();
            if (inet_ntop(AF_INET6, bytes.data(), text.data(), text.size()) == nullptr) {
                ec = boost::asio::error::invalid_argument;
                return {};
            }
            host_.assign(1, '[');
            host_.append(text.data());
            if (const auto scope = address.to_v6().scope_id(); scope != 0) {
                host_.push_back('%');
                const auto converted = std::to_chars(text.data(), text.data() + text.size(), scope);
                host_.append(text.data(), converted.ptr);
            }
            host_.push_back(']');
        } else {
            const auto bytes = address.to_v4().to_bytes();
            if (inet_ntop(AF_INET, bytes.data(), text.data(), text.size()) == nullptr) {
                ec = boost::asio::error::invalid_argument;
                return {};
            }
            host_.assign(text.data());
        }
        port_ = endpoint.port();
        open_ = true;
        ec.clear();
        return endpoint;
    }

    /**
     Records the destination authority without opening a raw TCP connection.
     @param endpoint Address and port to record as the destination.
     */
    void connect(const endpoint_type& endpoint) {
        boost::system::error_code ec;
        connect(endpoint, ec);
        detail::browser::throw_error(ec);
    }

    /**
     Records the destination authority and posts the result.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code, endpoint_type)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param results Resolver results supplying the original hostname and service.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_connect(const webcpp::asio::ip::tcp::resolver::results_type& results,
                       Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, endpoint_type)>(
            [this, results](auto handler) {
                boost::system::error_code ec;
                auto endpoint = connect(results, ec);
                detail::browser::post(executor_, std::move(handler), ec, endpoint);
            },
            token);
    }

    /**
     Records the destination authority and posts the result.

     The completion is posted through the associated executor and allocator.
     The completion signature is `void(error_code)`.
     @tparam Token Asio completion token selecting the asynchronous result type.
     @param endpoint Address and port to record as the destination.
     @param token Completion token controlling handler construction and the return type.
     @return The result determined by the completion token.
     */
    template <class Token>
    auto async_connect(const endpoint_type& endpoint, Token&& token) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code)>(
            [this, endpoint](auto handler) {
                boost::system::error_code ec;
                connect(endpoint, ec);
                detail::browser::post(executor_, std::move(handler), ec);
            },
            token);
    }

    /**
     Returns the recorded hostname or bracketed IPv6 address.
     @return The hostname or bracketed IPv6 address.
     */
    [[nodiscard]] std::string_view host() const noexcept { return host_; }

    /**
     Returns the recorded service port.
     @return The recorded service port.
     */
    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

    /**
     Sets the maximum decoded fetch body size in bytes.
     @param value Maximum decoded response bytes; the default is 8 MiB.
     */
    void response_body_limit(std::size_t value) noexcept { response_body_limit_ = value; }

    /**
     Returns the maximum decoded fetch body size in bytes.
     @return The maximum decoded body size in bytes.
     */
    [[nodiscard]] std::size_t response_body_limit() const noexcept { return response_body_limit_; }

    /**
     Reports whether browser URLs use TLS.
     @return True when secure URL schemes are selected.
     */
    [[nodiscard]] bool secure() const noexcept { return secure_; }

    /**
     Records whether browser URLs use TLS.
     @param value Whether subsequent URLs use https or wss.
     */
    void secure(bool value) noexcept { secure_ = value; }

    /**
     Checks a hostname or host-and-port against the recorded authority.
     @param host Hostname to match against the recorded destination authority.
     @return True if the value names the recorded destination.
     */
    [[nodiscard]] bool authority_matches(std::string_view host) const {
        if (host == std::string_view(host_)) {
            return true;
        }
        if (!host.starts_with(host_) || host.size() <= host_.size() || host[host_.size()] != ':') {
            return false;
        }
        const auto service = host.substr(host_.size() + 1);
        std::array<char, 5> text{};
        const auto converted = std::to_chars(text.data(), text.data() + text.size(), port_);
        return service == std::string_view(text.data(), converted.ptr - text.data());
    }

    /**
     Builds a browser URL from the recorded authority and request target.
     @param target Origin-form path and query for the browser URL.
     @param websocket Selects ws or wss instead of http or https.
     @return The fully qualified browser URL.
     */
    [[nodiscard]] std::pmr::string url(std::string_view target, bool websocket = false) const {
        const char* const scheme = websocket ? "ws" : "http";
        const char* const security = secure_ ? "s://" : "://";
        std::pmr::string result(scheme, memory_.get());
        result += security;
        result += host_;
        result += ':';
        std::array<char, 5> service{};
        const auto converted =
            std::to_chars(service.data(), service.data() + service.size(), port_);
        result.append(service.data(), converted.ptr);
        result += target;
        return result;
    }

    /**
     Connects host cancellation and closure to the authority record.
     @param owner Object retained by the host operation owner.
     @param cancel Callback invoked to cancel the active host operation.
     @param close Callback invoked to close the active host operation.
     */
    void bind(void* owner, void (*cancel)(void*), void (*close)(void*)) {
        owner_ = owner;
        cancel_ = cancel;
        close_ = close;
    }

    /**
     Shared fetch transaction storage retained by the authority record.
     */
    std::shared_ptr<void> http_state;

private:
    void reset_operation() {
        if (!open_ && !http_state) {
            secure_ = false;
            return;
        }
        close();
        bind(nullptr, nullptr, nullptr);
        http_state.reset();
        secure_ = false;
    }

    executor_type executor_;
    std::shared_ptr<std::pmr::memory_resource> memory_;
    std::pmr::string host_;
    std::uint16_t port_ = 0;
    std::size_t response_body_limit_ = 8 * 1024 * 1024;
    bool open_ = false;
    bool secure_ = false;
    ::webcpp::beast::detail::browser::clock_type::time_point deadline_ =
        ::webcpp::beast::detail::browser::clock_type::time_point::max();
    void* owner_ = nullptr;
    void (*cancel_)(void*) = nullptr;
    void (*close_)(void*) = nullptr;
};
}
}  // namespace webcpp::beast
#endif
