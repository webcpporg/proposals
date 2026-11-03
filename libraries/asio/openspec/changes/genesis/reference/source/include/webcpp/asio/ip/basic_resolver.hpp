// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_IP_BASIC_RESOLVER_HPP
#define WEBCPP_ASIO_IP_BASIC_RESOLVER_HPP

#include <webcpp/asio/config.hpp>

#ifndef WEBCPP_TARGET_EMSCRIPTEN
#include <boost/asio/ip/basic_resolver.hpp>  // lint-world: native resolver semantics

namespace webcpp::asio::ip {
/**
 Boost's native resolver.
 @tparam InternetProtocol The internet protocol.
 @tparam Executor The associated executor.
*/
template <class InternetProtocol, class Executor = boost::asio::any_io_executor>
using basic_resolver = boost::asio::ip::basic_resolver<InternetProtocol, Executor>;
}  // namespace webcpp::asio::ip
#else
#include <boost/asio/append.hpp>                // lint-world: preserve handler associations
#include <boost/asio/associated_allocator.hpp>  // lint-world: input storage uses the token allocator
#include <boost/asio/async_result.hpp>          // lint-world: completion token initiation
#include <boost/asio/error.hpp>                 // lint-world: resolver error values
#include <boost/asio/ip/address_v4.hpp>         // lint-world: an unspecified address without DNS
#include <boost/asio/ip/basic_resolver_results.hpp>  // lint-world: Boost resolver result entries
#include <boost/asio/ip/resolver_base.hpp>           // lint-world: Boost resolver flags
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <webcpp/asio/detail/post_completion.hpp>

namespace webcpp::asio::ip::detail {
/** Maps an empty, numeric or supported named service to its port. */
inline std::optional<std::uint16_t> port_of(std::string_view service,
                                            boost::asio::ip::resolver_base::flags flags) noexcept {
    if (service.empty()) {
        return 0;
    }
    if (service.find_first_not_of("0123456789") == std::string_view::npos) {
        std::uint32_t port = 0;
        const auto result = std::from_chars(service.data(), service.data() + service.size(), port);
        if (result.ec == std::errc{} && port <= 65535) {
            return static_cast<std::uint16_t>(port);
        }
        return std::nullopt;
    }
    if ((flags & boost::asio::ip::resolver_base::numeric_service) !=
        static_cast<boost::asio::ip::resolver_base::flags>(0)) {
        return std::nullopt;
    }
    if (service == "http" || service == "ws") {
        return 80;
    }
    if (service == "https" || service == "wss") {
        return 443;
    }
    return std::nullopt;
}
}  // namespace webcpp::asio::ip::detail

namespace webcpp::asio::ip {
/**
 Preserves a host and service for browser transports without querying DNS.
 @tparam InternetProtocol The internet protocol.
 @tparam Executor The associated executor.
 @see "Sockets and transports", in the guide.
*/
template <class InternetProtocol, class Executor = boost::asio::any_io_executor>
class basic_resolver : public boost::asio::ip::resolver_base {
public:
    /** The associated executor type. */
    using executor_type = Executor;
    /** The internet protocol type. */
    using protocol_type = InternetProtocol;
    /** The protocol endpoint type. */
    using endpoint_type = InternetProtocol::endpoint;
    /** Boost's sequence of resolver entries. */
    using results_type = boost::asio::ip::basic_resolver_results<InternetProtocol>;

    /**
     Constructs a resolver on executor.
     @param executor The completion executor.
    */
    explicit basic_resolver(executor_type executor) : executor_(std::move(executor)) {}

    /**
     Constructs a resolver on context.
     @tparam ExecutionContext An execution context type.
     @param context The context that runs completions.
    */
    template <class ExecutionContext>
        requires std::is_convertible_v<ExecutionContext&, boost::asio::execution_context&>
    explicit basic_resolver(ExecutionContext& context) : basic_resolver(context.get_executor()) {}

    /** Returns the associated executor.
     @return The executor.
    */
    [[nodiscard]] executor_type get_executor() const noexcept { return executor_; }

    /** Leaves already-computed, posted results unchanged. */
    // Boost's resolver exposes cancel as a non-static member.
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void cancel() {}

    /**
     Constructs an entry retaining host and service, with an unspecified address.
     @param host The host name retained verbatim.
     @param service An empty, numeric or supported named service.
     @param ec Receives success or service_not_found.
     @return One entry on success, otherwise an empty sequence.
    */
    results_type resolve(std::string_view host, std::string_view service,
                         boost::system::error_code& ec) {
        return resolve(host, service, static_cast<flags>(0), ec);
    }

    /**
     Constructs a forward result under the service parsing flags.
     @param host The host name retained verbatim.
     @param service The service retained verbatim.
     @param resolve_flags Only numeric_service affects parsing.
     @param ec Receives success or service_not_found.
     @return One entry on success, otherwise an empty sequence.
    */
    // Preserve Boost's non-static member interface.
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    results_type resolve(std::string_view host, std::string_view service, flags resolve_flags,
                         boost::system::error_code& ec) {
        const auto port = detail::port_of(service, resolve_flags);
        if (!port) {
            ec = boost::asio::error::service_not_found;
            return {};
        }
        ec.clear();
        return results_type::create(endpoint_type(boost::asio::ip::address_v4::any(), *port),
                                    std::string(host), std::string(service));
    }

    /**
     Refuses reverse resolution without accessing DNS.
     @param ec Receives operation_not_supported.
     @return An empty sequence.
    */
    // Preserve Boost's non-static member interface.
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    results_type resolve(const endpoint_type& /*endpoint*/, boost::system::error_code& ec) {
        ec = boost::asio::error::operation_not_supported;
        return {};
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Constructs a forward result, throwing on an unsupported service.
     @param host The original host.
     @param service The original service.
     @return One resolver entry.
    */
    results_type resolve(std::string_view host, std::string_view service) {
        return resolve(host, service, static_cast<flags>(0));
    }

    /**
     Constructs a forward result with flags, throwing on an unsupported service.
     @param host The original host.
     @param service The original service.
     @param resolve_flags The service parsing flags.
     @return One resolver entry.
    */
    results_type resolve(std::string_view host, std::string_view service, flags resolve_flags) {
        boost::system::error_code ec;
        auto result = resolve(host, service, resolve_flags, ec);
        if (ec) {
            boost::throw_exception(boost::system::system_error(ec));
        }
        return result;
    }

    /**
     Refuses reverse resolution by throwing operation_not_supported.
     @param endpoint The endpoint.
     @return An empty sequence if the exception hook returns.
    */
    results_type resolve(const endpoint_type& endpoint) {
        boost::system::error_code ec;
        auto result = resolve(endpoint, ec);
        if (ec) {
            boost::throw_exception(boost::system::system_error(ec));
        }
        return result;
    }
#endif
    /**
     Posts a forward resolution result.
     @tparam ResolveToken The completion token type.
     @param host The original host, copied before returning.
     @param service The original service, copied before returning.
     @param token The completion token.
     @return The token result for void(error_code, results_type).
    */
    template <class ResolveToken>
    auto async_resolve(std::string_view host, std::string_view service, ResolveToken&& token) {
        return async_resolve(host, service, static_cast<flags>(0),
                             std::forward<ResolveToken>(token));
    }

    /**
     Posts a forward result with service parsing flags.

     Copies the inputs with the token's associated allocator before returning.
     Deferred tokens retain those copies and compute the result only when initiated.
     Bind an allocator to the deferred token to select its input storage allocator.
     @tparam ResolveToken The completion token type.
     @param host The original host, copied before returning.
     @param service The original service, copied before returning.
     @param resolve_flags The service parsing flags.
     @param token The completion token.
     @return The token result for void(error_code, results_type).
    */
    template <class ResolveToken>
    auto async_resolve(std::string_view host, std::string_view service, flags resolve_flags,
                       ResolveToken&& token) {
        using input_allocator =
            std::allocator_traits<boost::asio::associated_allocator_t<std::decay_t<ResolveToken>>>::
                template rebind_alloc<char>;
        using input_string = std::basic_string<char, std::char_traits<char>, input_allocator>;
        const input_allocator allocator(boost::asio::get_associated_allocator(token));
        return boost::asio::async_initiate<ResolveToken,
                                           void(boost::system::error_code, results_type)>(
            [this](auto handler, const input_string& name, const input_string& port, flags mode) {
                boost::system::error_code ec;
                auto result = resolve(name, port, mode, ec);
                webcpp::asio::detail::post_completion(
                    executor_, boost::asio::append(std::move(handler), ec, std::move(result)));
            },
            token, input_string(host, allocator), input_string(service, allocator), resolve_flags);
    }

    /**
     Posts operation_not_supported for reverse resolution.
     @tparam ResolveToken The completion token type.
     @param endpoint The endpoint.
     @param token The completion token.
     @return The token result for void(error_code, results_type).
    */
    template <class ResolveToken>
    auto async_resolve(const endpoint_type& endpoint, ResolveToken&& token) {
        return boost::asio::async_initiate<ResolveToken,
                                           void(boost::system::error_code, results_type)>(
            [this](auto handler, const endpoint_type& peer) {
                boost::system::error_code ec;
                auto result = resolve(peer, ec);
                webcpp::asio::detail::post_completion(
                    executor_, boost::asio::append(std::move(handler), ec, std::move(result)));
            },
            token, endpoint);
    }

private:
    executor_type executor_;
};
}  // namespace webcpp::asio::ip
#endif
#endif  // WEBCPP_ASIO_IP_BASIC_RESOLVER_HPP
