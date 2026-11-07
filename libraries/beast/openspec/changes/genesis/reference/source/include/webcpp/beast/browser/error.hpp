// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_ERROR_HPP
#define WEBCPP_BEAST_BROWSER_ERROR_HPP

#include <webcpp/beast/config.hpp>

#include <boost/system/error_code.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast {
/**
 Browser failures without a corresponding Beast or Asio error.
 */
enum class browser_errc {
    /**
     The browser rejected a URL or WebSocket subprotocol syntax.
     */
    invalid_url = 1,
    /**
     Browser security policy blocked the operation.
     */
    blocked = 2,
    /**
     The browser could not establish the WebSocket connection.
     */
    connection_failed = 3,
    /**
     The outgoing WebSocket queue exceeded its configured bound.
     */
    send_limit = 4,
    /**
     Unread WebSocket messages exceeded their configured queue bound.
     */
    receive_limit = 5,
    /**
     Fetch failed without an exposed cause, including network and CORS failures.
     */
    fetch_failed = 6,
    /**
     The operation has no usable recorded destination authority.
     */
    no_host = 7,
};

namespace detail::browser_errors {
/**
 Stable error category for failures reported by browser host APIs.
 */
class error_category final : public boost::system::error_category {
public:
    /**
     Returns the stable browser error category name.
     */
    const char* name() const noexcept override { return "webcpp.beast"; }

    /**
     Returns an English description of a browser error value.
     */
    std::string message(int value) const override {
        switch (static_cast<browser_errc>(value)) {
            case browser_errc::invalid_url: return "Invalid browser URL or subprotocol";
            case browser_errc::blocked: return "Operation blocked by browser security";
            case browser_errc::connection_failed: return "Browser connection failed";
            case browser_errc::send_limit: return "Browser send queue limit exceeded";
            case browser_errc::receive_limit: return "Browser receive queue limit exceeded";
            case browser_errc::fetch_failed: return "Browser fetch failed";
            case browser_errc::no_host: return "No host was supplied";
        }
        return "Unknown browser error";
    }
};
}  // namespace detail::browser_errors

/**
 Returns the stable browser error category.
 @return The process-lifetime browser error category.
 */
inline const boost::system::error_category& browser_category() noexcept {
    static const detail::browser_errors::error_category category;
    return category;
}

/**
 Converts a browser failure into its stable error-code representation.
 @param code Browser error enumerator.
 @return The code associated with browser_category().
 */
inline boost::system::error_code make_error_code(browser_errc code) noexcept {
    return {static_cast<int>(code), browser_category()};
}
}  // namespace webcpp::beast

namespace boost::system {
template <>
struct is_error_code_enum<webcpp::beast::browser_errc> : std::true_type {};
}  // namespace boost::system
#endif
