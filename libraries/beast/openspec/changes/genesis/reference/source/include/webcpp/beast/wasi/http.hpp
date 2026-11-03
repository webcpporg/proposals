// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_WASI_HTTP_HPP
#define WEBCPP_BEAST_WASI_HTTP_HPP

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_WASI_HTTP_P2) || defined(WEBCPP_WASI_HTTP_P3)
#include <webcpp/beast/http.hpp>
#include <webcpp/wasi/http/host.hpp>

#include <boost/asio/buffer.hpp>               // lint-world: caller-owned memory buffers only
#include <boost/beast/core/buffers_range.hpp>  // lint-world: memory buffer iteration only
#include <boost/system/error_code.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace webcpp::beast::wasi {

/** Transfer limits and cancellation shared with the host HTTP resources. */
using webcpp::wasi::http::transfer_options;
/** Explicit streaming host input, independent of a socket. */
using webcpp::wasi::http::body_reader;
/** Explicit streaming host output, independent of a socket. */
using webcpp::wasi::http::body_writer;

namespace detail {
/** Reusable host resource operations. */
namespace host = webcpp::wasi::http;

/** Error messages for host HTTP operations. */
class host_category final : public boost::system::error_category {
public:
    /** Returns the stable category name.
    @return The stable error category name. */
    const char* name() const noexcept override { return "webcpp.beast.wasi"; }

    /** Describes a portable or host error.
    @return A readable error description. */
    std::string message(int value) const override {
        if (value >= 100) {
            return "wasi:http error-code " + std::to_string(value - 100);
        }
        switch (static_cast<host::message_errc>(value)) {
            case host::message_errc::success: return "success";
            case host::message_errc::invalid_message: return "invalid host HTTP message";
            case host::message_errc::forbidden_header: return "host forbids this HTTP field";
            case host::message_errc::body_limit: return "host HTTP body limit exceeded";
            case host::message_errc::cancelled: return "host HTTP operation cancelled";
            case host::message_errc::timed_out: return "host HTTP operation timed out";
            case host::message_errc::closed: return "host HTTP stream closed";
            case host::message_errc::host_error: return "host HTTP operation failed";
            case host::message_errc::invalid_state: return "invalid host HTTP operation state";
        }
        return "unknown host HTTP error";
    }
};

/** Maps resource failures into Boost.System values.
    @return The corresponding Boost.System error value. */
inline boost::system::error_code error_code(host::message_error value) {
    static const host_category category;
    if (!value) {
        return {};
    }
    if (value.code == host::message_errc::body_limit) {
        return http::error::body_limit;
    }
    return {value.code == host::message_errc::host_error ? 100 + value.host_code
                                                         : static_cast<int>(value.code),
            category};
}

/** Borrows a standard view as a Beast view.
    @return A borrowed view of the same bytes. */
inline boost::beast::string_view view(std::string_view value) {
    return {value.data(), value.size()};
}

/** Borrows a Beast view as a standard view.
    @return A borrowed view of the same bytes. */
inline std::string_view view(boost::beast::string_view value) {
    return {value.data(), value.size()};
}

/** Copies application fields into the host resource. */
template <class Fields>
host::message_fields copy_fields(const Fields& fields, boost::system::error_code& error,
                                 std::string_view authority = {}) {
    host::message_fields result;
    for (const auto& field : fields) {
        if (!authority.empty() && field.name() == http::field::host) {
            if (view(field.value()) != authority) {
                error = error_code({.code = host::message_errc::invalid_message});
            }
            if (error) {
                return result;
            }
            continue;
        }
        error = error_code(result.append(view(field.name_string()), view(field.value())));
        if (error) {
            return result;
        }
    }
    return result;
}

/** Delivers bounded host chunks to the caller-selected Beast BodyReader.
    @return The number of bytes transferred before completion or failure. */
template <bool Request, class Body, class Fields, class Trailers>
std::size_t read_body(host::body_reader& source, http::message<Request, Body, Fields>& message,
                      Trailers* trailers, const transfer_options& options,
                      boost::system::error_code& error) {
    typename Body::reader reader(message.base(), message.body());
    // Do not reserve an untrusted content-length before applying the body limit.
    reader.init(boost::none, error);
    if (error) {
        return 0;
    }
    std::array<char, 4096> storage{};
    std::size_t total = 0;
    host::message_error failure;
    while (!source.eof()) {
        const auto size = source.read_some(storage, options, failure);
        if (failure) {
            error = error_code(failure);
            return total;
        }
        total += reader.put(boost::asio::buffer(storage.data(), size), error);
        if (error) {
            return total;
        }
    }
    failure = source.finish(
        [&](std::string_view name, std::string_view value) {
            if (trailers) {
                trailers->insert(view(name), view(value));
            }
        },
        options);
    error = error_code(failure);
    if (!error) {
        reader.finish(error);
    }
    return total;
}

/** Delivers Beast BodyWriter buffers to host partial writes.
    @return The number of bytes transferred before completion or failure. */
template <bool Request, class Body, class Fields>
std::size_t write_body(host::body_writer& target, http::message<Request, Body, Fields>& message,
                       const transfer_options& options, boost::system::error_code& error) {
    typename Body::writer writer(message.base(), message.body());
    writer.init(error);
    if (error) {
        return 0;
    }
    std::size_t total = 0;
    for (;;) {
        auto next = writer.get(error);
        if (error || !next) {
            break;
        }
        for (auto const buffer : boost::beast::buffers_range_ref(next->first)) {
            auto bytes = std::string_view(static_cast<const char*>(buffer.data()), buffer.size());
            while (!bytes.empty()) {
                host::message_error failure;
                const auto written = target.write_some(bytes, options, failure);
                total += written;
                error = error_code(failure);
                if (error) {
                    return total;
                }
                bytes.remove_prefix(written);
            }
        }
        if (!next->second) {
            break;
        }
    }
    return total;
}
}  // namespace detail

/** Reads a complete host request into the caller's allocator.
    @tparam Body Beast body type with a BodyReader.
    @tparam Fields Beast fields type.
    @tparam Trailers Output fields type for trailers.
    @param source Owned host request.
    @param request Destination message, initially empty.
    @param error Receives any transfer or parsing failure.
    @param options Body limit, cancellation and deadline.
    @param trailers Optional destination for incoming trailers.
    @return The number of incoming body payload bytes. */
template <class Body, class Fields, class Trailers = http::fields>
std::size_t receive(webcpp::wasi::http::incoming_request& source,
                    http::request<Body, Fields>& request, boost::system::error_code& error,
                    const transfer_options& options = transfer_options(),
                    Trailers* trailers = nullptr) {
    error.clear();
    request.version(11);
    request.method_string(detail::view(source.method()));
    request.target(detail::view(source.target()));
    {
        auto const headers = source.headers();
        headers.for_each([&](std::string_view name, std::string_view value) {
            request.insert(detail::view(name), detail::view(value));
        });
    }
    if (!source.authority().empty() && request.find(http::field::host) == request.end()) {
        request.set(http::field::host, detail::view(source.authority()));
    }
    webcpp::wasi::http::message_error failure;
    auto body = source.consume(failure);
    if (failure) {
        error = detail::error_code(failure);
        return 0;
    }
    return detail::read_body(body, request, trailers, options, error);
}

/** Sends one response, returning logical body payload bytes.
    @tparam Body Beast body type with a BodyWriter.
    @tparam Fields Beast fields type.
    @tparam Trailers Input fields type for trailers.
    @param out Active response commitment resource.
    @param response Application response.
    @param error Receives any transfer failure.
    @param options Cancellation and deadline.
    @param trailers Optional outgoing trailers.
    @return The number of outgoing body payload bytes. */
template <class Body, class Fields, class Trailers = http::fields>
std::size_t send(webcpp::wasi::http::response_out out, http::response<Body, Fields>& response,
                 boost::system::error_code& error,
                 const transfer_options& options = transfer_options(),
                 const Trailers* trailers = nullptr) {
    error.clear();
    auto headers = detail::copy_fields(response.base(), error);
    if (error) {
        webcpp::wasi::http::refuse_response(out);
        return 0;
    }
    std::optional<webcpp::wasi::http::message_fields> final_fields;
    if (trailers) {
        final_fields.emplace(detail::copy_fields(*trailers, error));
        if (error) {
            webcpp::wasi::http::refuse_response(out);
            return 0;
        }
    }
    webcpp::wasi::http::message_error failure;
    auto body = webcpp::wasi::http::commit_response(out, response.result_int(), std::move(headers),
                                                    failure);
    if (failure) {
        error = detail::error_code(failure);
        return 0;
    }
    const auto total = detail::write_body(body, response, options, error);
    if (!error) {
        error = detail::error_code(body.finish(final_fields ? &*final_fields : nullptr));
    }
    return total;
}

/** A host HTTP client.

    The host owns DNS, connections and HTTPS trust. */
class client {
    std::string_view scheme_;
    std::string_view authority_;

public:
    /** Borrows connection metadata, which must outlive this client.
        @param scheme HTTP or HTTPS.
        @param authority Host and optional port. */
    client(std::string_view scheme, std::string_view authority)
        : scheme_(scheme), authority_(authority) {}

    /** Returns the borrowed request scheme.
        @return HTTP or HTTPS, as supplied at construction. */
    [[nodiscard]] std::string_view scheme() const noexcept { return scheme_; }

    /** Returns the borrowed request authority.
        @return Host and optional port, as supplied at construction. */
    [[nodiscard]] std::string_view authority() const noexcept { return authority_; }

    /** Performs a suspending exchange, preserving the caller's Beast body and field allocators.

        The request and response may use any synchronous Beast BodyReader/BodyWriter. Returned
        bytes count decoded response payload only. Transfer encoding and connection ownership
        remain host capabilities; forbidden fields produce a visible error.
        @tparam RequestBody Outgoing Beast body type.
        @tparam RequestFields Outgoing Beast fields type.
        @tparam ResponseBody Incoming Beast body type.
        @tparam ResponseFields Incoming Beast fields type.
        @tparam Trailers Trailer fields type.
        @param request Outgoing application message.
        @param response Initially empty incoming message.
        @param error Receives any transfer failure.
        @param options Body limit, cancellation and deadline.
        @param response_trailers Optional received trailers.
        @param request_trailers Optional outgoing trailers.
        @return The number of received response payload bytes. */
    template <class RequestBody, class RequestFields, class ResponseBody, class ResponseFields,
              class Trailers = http::fields>
    std::size_t perform(http::request<RequestBody, RequestFields>& request,
                        http::response<ResponseBody, ResponseFields>& response,
                        boost::system::error_code& error,
                        const transfer_options& options = transfer_options(),
                        Trailers* response_trailers = nullptr,
                        const Trailers* request_trailers = nullptr) {
        error = detail::error_code(options.check());
        if (error) {
            return 0;
        }
        auto headers = detail::copy_fields(request.base(), error, authority_);
        if (error) {
            return 0;
        }
        std::optional<webcpp::wasi::http::message_fields> final_fields;
        if (request_trailers) {
            final_fields.emplace(detail::copy_fields(*request_trailers, error));
            if (error) {
                return 0;
            }
        }
        webcpp::wasi::http::outgoing_exchange exchange;
        webcpp::wasi::http::message_error failure;
        {
            auto body = exchange.start(scheme_, authority_, detail::view(request.method_string()),
                                       detail::view(request.target()), std::move(headers), failure);
            error = detail::error_code(failure);
            if (error) {
                return 0;
            }
            detail::write_body(body, request, options, error);
            if (error) {
                return 0;
            }
            error = detail::error_code(body.finish(final_fields ? &*final_fields : nullptr));
            if (error) {
                return 0;
            }
        }
        auto incoming = exchange.receive(options, failure);
        if (failure) {
            error = detail::error_code(failure);
            return 0;
        }
        response.version(11);
        response.result(incoming.status());
        {
            auto const received = incoming.headers();
            received.for_each([&](std::string_view name, std::string_view value) {
                response.insert(detail::view(name), detail::view(value));
            });
        }
        auto body = incoming.consume(failure);
        if (failure) {
            error = detail::error_code(failure);
            return 0;
        }
        return detail::read_body(body, response, response_trailers, options, error);
    }
};

namespace detail {
/** Adapts only bootstrap and transport; the application's routing and response builder remain
 * callable. */
template <class Handler>
void handle(host::request_handle raw, host::response_out out, Handler&& handler,
            const transfer_options& options = {}) {
    host::incoming_request incoming(raw);
    http::request<http::string_body> request;
    boost::system::error_code error;
    receive(incoming, request, error, options);
    if (error) {
        http::response<http::empty_body> response{error == http::error::body_limit
                                                      ? http::status::payload_too_large
                                                      : http::status::bad_request,
                                                  11};
        response.prepare_payload();
        send(out, response, error, options);
        return;
    }
    auto response = handler(std::move(request));
    if constexpr (requires {
                      typename std::variant_size<std::remove_cvref_t<decltype(response)>>::type;
                  }) {
        std::visit([&](auto& value) { send(out, value, error, options); }, response);
    } else {
        send(out, response, error, options);
    }
}
}  // namespace detail
}  // namespace webcpp::beast::wasi

#ifdef WEBCPP_WASI_HTTP_P2
/** Exports the P2 host server.
    @param handler Callable building a response or variant for one request. */
#define WEBCPP_BEAST_WASI_HTTP_EXPORT(handler)                             \
    extern "C" void exports_wasi_http_incoming_handler_handle(             \
        exports_wasi_http_incoming_handler_own_incoming_request_t request, \
        exports_wasi_http_incoming_handler_own_response_outparam_t out) {  \
        webcpp::beast::wasi::detail::handle(request, out, handler);        \
    }
#else
/** Exports a P3 task with body resources scoped to its lifetime.
    @param handler Callable building a response or variant for one request. */
#define WEBCPP_BEAST_WASI_HTTP_EXPORT(handler)                                             \
    extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle(          \
        exports_wasi_http_handler_own_request_t request) {                                 \
        webcpp::beast::wasi::detail::handle(request, {}, handler);                         \
        return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;                                        \
    }                                                                                      \
    extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle_callback( \
        webcpp_wasi_http_event_t*) {                                                       \
        return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;                                        \
    }
#endif

#endif  // Host HTTP bindings selected.

#endif
