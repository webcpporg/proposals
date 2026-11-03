// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_HTTP_HPP
#define WEBCPP_BEAST_BROWSER_HTTP_HPP

#include <webcpp/beast/config.hpp>

#include <emscripten/val.h>
#include <array>
#include <boost/asio/steady_timer.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/beast/core/buffers_range.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <boost/beast/core/error.hpp>  // lint-world: portable protocol data and explicit stream algorithms
#include <vector>
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/beast/browser/detail/imports.hpp>
#include <webcpp/beast/browser/ssl_stream.hpp>
#include <webcpp/beast/browser/tcp_stream.hpp>
#include <webcpp/beast/http.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::detail {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace browser {
/**
 One fetch exchange; browser response metadata maps directly into a Beast message.
 */
// Registry lookup and shared ownership have independent lifetimes.
// NOLINTNEXTLINE(misc-multiple-inheritance)
class http_state final : public webcpp::asio::browser::intrusive_registration,
                         public std::enable_shared_from_this<http_state> {
public:
    /**
     Retains the resource backing the fetch state and its queued callbacks.
     */
    std::shared_ptr<std::pmr::memory_resource> memory;
    /**
     Allocation resource kept alive while queued callbacks can access storage.
     */
    std::pmr::memory_resource* resource;
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
     Pending application read completion, or null when no read is outstanding.
     */
    pending* read = nullptr;
    /**
     JavaScript AbortController for the active fetch.
     */
    emscripten::val controller = emscripten::val::undefined();
    /**
     JavaScript response metadata and decoded body retained until consumption.
     */
    emscripten::val response = emscripten::val::undefined();
    /**
     Terminal error retained for the next pending completion.
     */
    error_code failure;
    /**
     Whether the owning stream still accepts browser events.
     */
    bool alive = true;
    /**
     Whether a fetch promise is outstanding.
     */
    bool active = false;
    /**
     Whether a completed fetch is waiting for a read.
     */
    bool ready = false;
    /** Whether fetch has exposed the response status and fields. */
    bool headers_ready = false;
    /** Whether the current pending read requests metadata without consuming the body. */
    bool header_only = false;
    /**
     Whether the request method forbids a response body.
     */
    bool head = false;

    /**
     Initializes retained state and its executor or allocation resource.
     */
    explicit http_state(executor_type ex, std::shared_ptr<std::pmr::memory_resource> storage)
        : memory(std::move(storage)),
          resource(memory.get()),
          executor(std::move(ex)),
          bridge(&boost::asio::use_service<webcpp::asio::browser::bridge>(
              boost::asio::query(executor, boost::asio::execution::context))),
          timer(executor) {}

    /**
     Releases registration and retained state during destruction.
     */
    ~http_state() { detach(); }

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
    }

    /**
     Asks the browser to abort the current fetch.
     */
    void abort() const {
        if (!controller.isUndefined()) {
            controller.call<void>("abort");
        }
    }

    /**
     Cancels the active host operation and posts its aborted completion.
     */
    void cancel() {
        abort();
        detach();
        cancel_timer();
        active = false;
        failure = boost::asio::error::operation_aborted;
        ready = true;
        if (read != nullptr) {
            read->complete(failure);
        }
    }

    /**
     Detaches the host resource and aborts all pending operations.
     */
    void stop() {
        alive = false;
        cancel();
    }

    /**
     Closes the browser operation and its authority record.
     */
    void shutdown() noexcept override {
        bridge = nullptr;
        key = 0;
        alive = false;
        abort();
        if (read != nullptr) {
            read->discard();
        }
    }

    /**
     Queues a browser event on the stream executor while retaining state ownership.
     */
    bool deliver(const emscripten::val& event) override {
        post(executor, boost::asio::bind_allocator(
                           owning_allocator<std::byte>(memory),
                           [self = shared_from_this(), event, generation = key] {
                               if (!self->alive || !self->active || self->key != generation) {
                                   return;
                               }
                               const auto type = js_text(event["type"], self->resource);
                               if (type == "fetch-header") {
                                   self->response = event;
                                   self->headers_ready = true;
                                   self->failure = unsupported(event["redirected"].as<bool>());
                                   if (self->read && self->header_only) {
                                       self->read->complete(self->failure);
                                   }
                                   return;
                               }
                               self->active = false;
                               self->ready = true;
                               self->cancel_timer();
                               self->detach();
                               if (type == "fetch-error") {
                                   self->failure = make_error_code(browser_errc::fetch_failed);
                               } else if (type == "fetch-limit") {
                                   self->failure = boost::beast::http::error::body_limit;
                               } else {
                                   self->response = event;
                                   self->failure = unsupported(event["redirected"].as<bool>());
                               }
                               if (self->read) {
                                   self->read->complete(self->failure);
                               }
                           }));
        return false;
    }

    /**
     Schedules the absolute deadline for the active host operation.
     */
    void arm(::webcpp::beast::detail::browser::clock_type::time_point deadline) {
        if (deadline == ::webcpp::beast::detail::browser::clock_type::time_point::max()) {
            return;
        }
        timer.expires_at(deadline);
        timer.async_wait(boost::asio::bind_allocator(
            owning_allocator<std::byte>(memory),
            [weak = weak_from_this(), generation = key](error_code ec) {
                if (auto const self = weak.lock();
                    self && !ec && self->alive && self->key == generation) {
                    self->abort();
                    self->detach();
                    self->active = false;
                    self->ready = true;
                    self->failure = boost::beast::error::timeout;
                    if (self->read) {
                        self->read->complete(self->failure);
                    }
                }
            }));
    }
};

/**
 Obtains or creates the fetch state attached to an authority record.
 */
inline std::shared_ptr<http_state> http(tcp_stream& stream) {
    if (!stream.http_state) {
        auto const state =
            std::allocate_shared<http_state>(owning_allocator<std::byte>(stream.resource_owner()),
                                             stream.get_executor(), stream.resource_owner());
        stream.http_state = state;
        stream.bind(
            state.get(), [](void* value) { static_cast<http_state*>(value)->cancel(); },
            [](void* value) { static_cast<http_state*>(value)->stop(); });
    }
    return std::static_pointer_cast<http_state>(stream.http_state);
}
/**
 Identifies a browser authority record or its TLS-intent layer.
 */
template <class Stream>
concept host_stream = std::same_as<std::remove_cvref_t<Stream>, tcp_stream> ||
                      std::same_as<std::remove_cvref_t<Stream>, ssl_stream<tcp_stream>>;

/** Detects forbidden methods in a Fetch method-override field value. */
inline bool forbidden_method_override(boost::beast::string_view value) {
    // Fetch splits comma-separated values while preserving HTTP quoted strings.
    // https://fetch.spec.whatwg.org/#forbidden-request-header
    while (!value.empty()) {
        std::size_t length = 0;
        bool quoted = false;
        for (; length != value.size(); ++length) {
            const auto character = value[length];
            if (quoted && character == '\\' && length + 1 != value.size()) {
                ++length;
            } else if (character == '"') {
                quoted = !quoted;
            } else if (character == ',' && !quoted) {
                break;
            }
        }
        auto method = value.substr(0, length);
        while (!method.empty() && (method.front() == ' ' || method.front() == '\t')) {
            method.remove_prefix(1);
        }
        while (!method.empty() && (method.back() == ' ' || method.back() == '\t')) {
            method.remove_suffix(1);
        }
        if (boost::beast::iequals(method, "CONNECT") || boost::beast::iequals(method, "TRACE") ||
            boost::beast::iequals(method, "TRACK")) {
            return true;
        }
        if (length == value.size()) {
            break;
        }
        value.remove_prefix(length + 1);
    }
    return false;
}

/**
 Detects request headers that the browser owns or may overwrite.
 */
inline bool forbidden_header(boost::beast::string_view name, boost::beast::string_view value) {
    // Fetch's forbidden request headers plus User-Agent, which browsers may overwrite.
    constexpr std::array<std::string_view, 18> names = {
        "accept-charset",
        "accept-encoding",
        "access-control-request-headers",
        "access-control-request-method",
        "cookie",
        "cookie2",
        "date",
        "dnt",
        "expect",
        "origin",
        "referer",
        "set-cookie",
        "te",
        "trailer",
        "upgrade",
        "via",
        "user-agent",
        "permissions-policy",
    };
    for (auto const item : names) {
        if (boost::beast::iequals(name, {item.data(), item.size()})) {
            return true;
        }
    }
    if (name.size() >= 4 &&
        (boost::beast::iequals(name.substr(0, 4), "sec-") ||
         (name.size() >= 6 && boost::beast::iequals(name.substr(0, 6), "proxy-")))) {
        return true;
    }
    if (!boost::beast::iequals(name, "x-http-method") &&
        !boost::beast::iequals(name, "x-http-method-override") &&
        !boost::beast::iequals(name, "x-method-override")) {
        return false;
    }
    return forbidden_method_override(value);
}

/** Builds fetch headers after applying authority and forbidden-field policy. */
template <class Body, class Fields>
emscripten::val fetch_headers(const tcp_stream& stream,
                              const boost::beast::http::request<Body, Fields>& request,
                              std::size_t& count, error_code& ec) {
    namespace http = boost::beast::http;
    auto const headers = emscripten::val::array();
    for (const auto& field : request) {
        count += field.name_string().size() + field.value().size() + 4;
        if (field.name() == http::field::host) {
            ec = unsupported(
                !stream.authority_matches({field.value().data(), field.value().size()}));
            if (ec) {
                return headers;
            }
            continue;
        }
        if (field.name() == http::field::connection || field.name() == http::field::keep_alive ||
            field.name() == http::field::content_length ||
            field.name() == http::field::transfer_encoding) {
            continue;
        }
        if (forbidden_header(field.name_string(), field.value())) {
            ec = unsupported();
            if (ec) {
                return headers;
            }
            continue;
        }
        auto pair = emscripten::val::array();
        pair.call<void>("push", js_string(field.name_string()));
        pair.call<void>("push", js_string(field.value()));
        headers.call<void>("push", pair);
    }
    return headers;
}

/**
 Validates and buffers a complete request before initiating fetch.
 */
template <class Body, class Fields>
std::size_t start_fetch(tcp_stream& stream, boost::beast::http::request<Body, Fields>& request,
                        error_code& ec) {
    namespace http = boost::beast::http;
    ec.clear();
    if (!stream.is_open()) {
        ec = boost::asio::error::not_connected;
        return 0;
    }
    auto const state = detail::browser::http(stream);
    if (state->active || state->ready || state->read) {
        ec = boost::asio::error::already_started;
        return 0;
    }
    const auto method = request.method();
    if (method == http::verb::connect || method == http::verb::trace ||
        request.method_string() == "TRACK") {
        ec = boost::asio::error::operation_not_supported;
        return 0;
    }
    auto init = emscripten::val::object();
    std::size_t count = request.method_string().size() + request.target().size() + 12;
    const auto headers = fetch_headers(stream, request, count, ec);
    if (ec) {
        return 0;
    }
    std::pmr::vector<std::byte> body(stream.resource());
    typename Body::writer writer(request.base(), request.body());
    writer.init(ec);
    if (ec) {
        return 0;
    }
    while (auto next = writer.get(ec)) {
        if (ec) {
            return 0;
        }
        const auto size = boost::asio::buffer_size(next->first);
        const auto old = body.size();
        body.resize(old + size);
        boost::asio::buffer_copy(boost::asio::buffer(body.data() + old, size), next->first);
        if (!next->second) {
            break;
        }
    }
    if (ec) {
        return 0;
    }
    if (!body.empty() && (method == http::verb::get || method == http::verb::head)) {
        ec = boost::asio::error::operation_not_supported;
        return 0;
    }
    if (request.has_content_length() && request.payload_size() &&
        request[http::field::content_length] != std::to_string(body.size())) {
        ec = http::error::bad_content_length;
        return 0;
    }
    init.set("webcppBodyLimit", static_cast<double>(stream.response_body_limit()));
    init.set("method", js_string(request.method_string()));
    init.set("headers", headers);
    init.set("mode", js_string("cors"));
    init.set("redirect", js_string("follow"));
    if (!body.empty()) {
        init.set("body",
                 emscripten::val(emscripten::typed_memory_view(
                                     body.size(), reinterpret_cast<unsigned char*>(body.data())))
                     .call<emscripten::val>("slice"));
    }
    state->head = method == http::verb::head;
    state->headers_ready = false;
    state->failure.clear();
    state->response = emscripten::val::undefined();
    state->active = true;
    state->key = state->bridge->add(*state);
    const auto target = request.target();
    auto const url = js_string(stream.url({target.data(), target.size()}));
    state->controller = emscripten::val::take_ownership(webcpp_beast_fetch(
        url.as_handle(), init.as_handle(), state->bridge->function(state->key).as_handle()));
    state->arm(stream.expiry());
    return count + 2 + body.size();
}

/**
 Copies decoded fetch metadata and body into a Beast response.
 */
template <class Body, class Fields>
void map_response(http_state& state, boost::beast::http::response<Body, Fields>& response,
                  error_code& ec, std::size_t& count, bool header_only = false) {
    if (ec) {
        return;
    }
    namespace http = boost::beast::http;
    response.clear();
    response.version(11);
    response.result(state.response["status"].as<unsigned>());
    response.reason({});
    auto const reason = js_text(state.response["statusText"], state.resource);
    if (!reason.empty()) {
        response.reason(reason);
    }
    const auto headers = state.response["headers"];
    for (std::size_t i = 0; i < headers["length"].as<std::size_t>(); ++i) {
        const auto pair = headers[i];
        const auto name = js_text(pair[0], state.resource);
        const auto value = js_text(pair[1], state.resource);
        if (boost::beast::iequals(name, "content-length") ||
            boost::beast::iequals(name, "content-encoding") ||
            boost::beast::iequals(name, "transfer-encoding")) {
            continue;
        }
        response.insert(name, value);
    }
    const bool bodyless =
        state.head || response.result_int() == 204 || response.result_int() == 304;
    count = 0;
    if (header_only) {
        count = 13 + response.reason().size();
        for (const auto& field : response) {
            count += field.name_string().size() + field.value().size() + 4;
        }
        return;
    }
    const auto body = state.response["body"];
    const auto size = bodyless ? 0 : body["byteLength"].as<std::size_t>();
    // Preserve the body's allocator while replacing the previous exchange's bytes.
    if constexpr (requires { response.body().clear(); }) {
        response.body().clear();
    } else if constexpr (requires { response.body().consume(response.body().size()); }) {
        response.body().consume(response.body().size());
    }
    typename Body::reader reader(response.base(), response.body());
    reader.init(size, ec);
    if (!ec && size) {
        std::pmr::vector<std::byte> bytes(size, state.resource);
        webcpp_beast_copy(body.as_handle(), bytes.data(), size);
        count = reader.put(boost::asio::buffer(bytes), ec);
    }
    if (!ec) {
        reader.finish(ec);
    }
    if (!bodyless) {
        response.content_length(size);
    }
    state.ready = false;
    state.response = emscripten::val::undefined();
}

/**
 Pumps a suspendable context until a fetch read completes.
 */
template <class Begin>
void sync_http(tcp_stream& stream, Begin begin, error_code& ec, std::size_t& size) {
    auto& context = static_cast<webcpp::asio::io_context&>(
        boost::asio::query(stream.get_executor(), boost::asio::execution::context));
    if (context.get_executor().running_in_this_thread() ||
        boost::asio::use_service<webcpp::asio::browser::bridge>(context).delivering()) {
        ec = boost::asio::error::operation_not_supported;
        return;
    }
    bool done = false;
    begin([&](error_code error, std::size_t count) {
        ec = error;
        size = count;
        done = true;
    });
    context.restart();
    // NOLINTNEXTLINE(bugprone-infinite-loop): run_one invokes the callback that sets done
    while (!done) {
        context.run_one();
    }
}

/** Starts a metadata-only or complete response read with the handler associations. */
template <bool HeaderOnly, host_stream Stream, class Body, class Allocator, class Token>
auto receive_response(
    Stream& stream,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response,
    Token&& token) {
    return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
        [&stream, &response](auto handler) {
            auto const state = detail::browser::http(get_lowest_layer(stream));
            if (state->read || (!state->active && !state->ready)) {
                detail::browser::post(
                    stream.get_executor(), std::move(handler),
                    boost::system::error_code(boost::asio::error::already_started), std::size_t{});
                return;
            }
            state->header_only = HeaderOnly;
            detail::browser::start<true>(
                state->read, stream.get_executor(), std::move(handler),
                [state, &response](auto& ec, auto& count) {
                    if (ec) {
                        state->ready = false;
                        return;
                    }
                    detail::browser::map_response(*state, response, ec, count, HeaderOnly);
                },
                boost::asio::cancellation_type::all,
                [state] {
                    state->abort();
                    state->detach();
                    state->cancel_timer();
                    state->active = false;
                    state->ready = false;
                });
            if (state->ready || (HeaderOnly && state->headers_ready)) {
                state->read->complete(state->failure);
            }
        },
        token);
}

}  // namespace browser
}
}  // namespace webcpp::beast::detail

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::http {
/**
 Submits one complete request to fetch after validating browser restrictions.

 The completion is posted through the associated executor and allocator.
 The completion signature is `void(error_code, std::size_t)`.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @tparam Token Asio completion token selecting the asynchronous result type.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param request Complete request, kept alive until asynchronous completion when applicable.
 @param token Completion token controlling handler construction and the return type.
 @return The result determined by the completion token.
 */
template <detail::browser::host_stream Stream, class Body, class Allocator, class Token>
auto async_write(
    Stream& stream,
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& request,
    Token&& token) {
    return boost::asio::async_initiate<Token, void(boost::system::error_code, std::size_t)>(
        [&stream, &request](auto handler) {
            boost::system::error_code ec;
            const auto size = detail::browser::start_fetch(get_lowest_layer(stream), request, ec);
            detail::browser::post(stream.get_executor(), std::move(handler), ec, size);
        },
        token);
}

/**
 Submits one complete request to fetch after validating browser restrictions.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param request Complete request, kept alive until asynchronous completion when applicable.
 @param ec Receives the operation error, or is cleared on success.
 @return The accepted body and synthetic header byte count.
 */
template <detail::browser::host_stream Stream, class Body, class Allocator>
std::size_t write(
    Stream& stream,
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& request,
    boost::system::error_code& ec) {
    return detail::browser::start_fetch(get_lowest_layer(stream), request, ec);
}

/**
 Submits one complete request to fetch after validating browser restrictions.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param request Complete request, kept alive until asynchronous completion when applicable.
 @return The accepted body and synthetic header byte count.
 */
template <detail::browser::host_stream Stream, class Body, class Allocator>
std::size_t write(
    Stream& stream,
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& request) {
    boost::system::error_code ec;
    const auto n = write(stream, request, ec);
    detail::browser::throw_error(ec);
    return n;
}

/**
 Receives the complete decoded fetch response into a Beast message.

 The completion is posted through the associated executor and allocator.
 The completion signature is `void(error_code, std::size_t)`.
 Fetch decoding precedes message delivery, and wire framing headers are normalized.
 The authority record response_body_limit bounds the decoded body before allocation.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Buffer Dynamic buffer type retained for Beast call compatibility.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @tparam Token Asio completion token selecting the asynchronous result type.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param response Destination message, kept alive until asynchronous completion when applicable.
 @param token Completion token controlling handler construction and the return type.
 @return The result determined by the completion token.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator,
          class Token>
auto async_read(
    Stream& stream, Buffer& /*unused*/,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response,
    Token&& token) {
    return detail::browser::receive_response<false>(stream, response, std::forward<Token>(token));
}

/**
 Receives the complete decoded fetch response into a Beast message.

 Fetch decoding precedes message delivery, and wire framing headers are normalized.
 The authority record response_body_limit bounds the decoded body before allocation.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Buffer Dynamic buffer type retained for Beast call compatibility.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param buffer Compatibility buffer; direct fetch reads do not consume or append wire bytes.
 @param response Destination message, kept alive until asynchronous completion when applicable.
 @param ec Receives the operation error, or is cleared on success.
 @return The number of decoded body bytes delivered.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator>
std::size_t read(
    Stream& stream, Buffer& buffer,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response,
    boost::system::error_code& ec) {
    std::size_t size = 0;
    detail::browser::sync_http(
        get_lowest_layer(stream),
        [&](auto handler) { async_read(stream, buffer, response, std::move(handler)); }, ec, size);
    return size;
}

/**
 Receives the complete decoded fetch response into a Beast message.

 Fetch decoding precedes message delivery, and wire framing headers are normalized.
 The authority record response_body_limit bounds the decoded body before allocation.
 @tparam Stream Browser authority record or TLS-intent stream.
 @tparam Buffer Dynamic buffer type retained for Beast call compatibility.
 @tparam Body Beast body model used to encode or receive the message body.
 @tparam Allocator Allocator used by the HTTP message fields.
 @param stream Browser stream whose recorded authority selects the host operation.
 @param buffer Compatibility buffer; direct fetch reads do not consume or append wire bytes.
 @param response Destination message, kept alive until asynchronous completion when applicable.
 @return The number of decoded body bytes delivered.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator>
std::size_t read(
    Stream& stream, Buffer& buffer,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response) {
    boost::system::error_code ec;
    const auto n = read(stream, buffer, response, ec);
    detail::browser::throw_error(ec);
    return n;
}

/** Receives response metadata before fetch has consumed its body.

 The next complete read consumes the retained response body; a metadata read leaves it untouched.
 The buffer parameter is retained for Beast call compatibility and receives no wire bytes.
 @tparam Stream Browser authority or TLS-intent stream.
 @tparam Buffer Dynamic buffer type accepted for call compatibility.
 @tparam Body Beast body model of the destination response.
 @tparam Allocator Allocator of the destination fields.
 @tparam Token Completion token for void(error_code, std::size_t).
 @param stream Stream with an outstanding fetch request.
 @param response Destination for status and exposed fields; kept alive until completion.
 @param token Completion token determining the operation result type.
 @return The result determined by the completion token.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator,
          class Token>
auto async_read_header(
    Stream& stream, Buffer& /*unused*/,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response,
    Token&& token) {
    return detail::browser::receive_response<true>(stream, response, std::forward<Token>(token));
}

/** Suspends the main stack until fetch exposes response metadata.

 The complete body remains available to a later read, and executing handlers cannot reenter
 this synchronous facade.
 @tparam Stream Browser authority or TLS-intent stream.
 @tparam Buffer Dynamic buffer type accepted for call compatibility.
 @tparam Body Beast body model of the destination response.
 @tparam Allocator Allocator of the destination fields.
 @param stream Stream with an outstanding fetch request.
 @param buffer Compatibility buffer, whose contents are unchanged.
 @param response Destination for response status and exposed fields.
 @param ec Receives the operation error, or is cleared on success.
 @return The logical HTTP status-line and field byte count, excluding the body.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator>
std::size_t read_header(
    Stream& stream, Buffer& buffer,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response,
    boost::system::error_code& ec) {
    std::size_t count = 0;
    detail::browser::sync_http(
        get_lowest_layer(stream),
        [&](auto handler) { async_read_header(stream, buffer, response, std::move(handler)); }, ec,
        count);
    return count;
}

/** Suspends until response metadata is available and throws on failure.
 @tparam Stream Browser authority or TLS-intent stream.
 @tparam Buffer Dynamic buffer type accepted for call compatibility.
 @tparam Body Beast body model of the destination response.
 @tparam Allocator Allocator of the destination fields.
 @param stream Stream with an outstanding fetch request.
 @param buffer Compatibility buffer, whose contents are unchanged.
 @param response Destination for response status and exposed fields.
 @return The logical HTTP status-line and field byte count, excluding the body.
 */
template <detail::browser::host_stream Stream, class Buffer, class Body, class Allocator>
std::size_t read_header(
    Stream& stream, Buffer& buffer,
    boost::beast::http::response<Body, boost::beast::http::basic_fields<Allocator>>& response) {
    boost::system::error_code ec;
    auto count = read_header(stream, buffer, response, ec);
    detail::browser::throw_error(ec);
    return count;
}
}  // namespace webcpp::beast::http
#endif
