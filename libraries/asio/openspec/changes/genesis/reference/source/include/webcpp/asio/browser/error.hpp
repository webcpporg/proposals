// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_ERROR_HPP
#define WEBCPP_ASIO_BROWSER_ERROR_HPP

#include <boost/system/error_code.hpp>
#include <string>
#include <type_traits>
#include <webcpp/asio/config.hpp>

namespace webcpp::asio::browser::error {
/** Errors reported by browser operations; values are never reused. */
enum class errors : int {
    /** The JavaScript Promise rejected; its reason is the completion value. */
    promise_rejected = 1,
};
/** A JavaScript Promise rejection. */
inline constexpr errors promise_rejected = errors::promise_rejected;

namespace detail {
/** The category for browser operation errors. */
class browser_category final : public boost::system::error_category {
public:
    /** Returns the stable category name, webcpp.asio. */
    const char* name() const noexcept override { return "webcpp.asio"; }

    /** Returns a description of value, or the unknown-error description. */
    std::string message(int value) const override {
        return value == static_cast<int>(errors::promise_rejected) ? "the promise was rejected"
                                                                   : "unknown webcpp.asio error";
    }
};
}  // namespace detail

/**
 Returns the category shared by all browser operation errors.

 @return The category named webcpp.asio.
*/
inline const boost::system::error_category& category() noexcept {
    static const detail::browser_category instance;
    return instance;
}

/**
 Creates the error code corresponding to value.

 @param value The browser error.
 @return Its code in category().
*/
inline boost::system::error_code make_error_code(errors value) noexcept {
    return {static_cast<int>(value), category()};
}
}  // namespace webcpp::asio::browser::error

namespace boost::system {
/** Enables implicit conversion of browser errors to error_code. */
template <>
struct is_error_code_enum<webcpp::asio::browser::error::errors> : std::true_type {};
}  // namespace boost::system

#endif  // WEBCPP_ASIO_BROWSER_ERROR_HPP
