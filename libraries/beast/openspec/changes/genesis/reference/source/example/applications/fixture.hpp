// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_APPLICATION_FIXTURE_HPP
#define WEBCPP_BEAST_APPLICATION_FIXTURE_HPP

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>

namespace webcpp_fixture {
[[nodiscard]] inline const char* host() {
    const auto* value = std::getenv("WEBCPP_BEAST_FIXTURE_HOST");
    return value != nullptr ? value : "localhost";
}

[[nodiscard]] inline const char* port() {
    const auto* value = std::getenv("WEBCPP_BEAST_FIXTURE_PORT");
    return value != nullptr ? value : "8080";
}

[[nodiscard]] inline int integer(const char* text) {
    int value = 0;
    const auto* end = text + std::strlen(text);
    const auto result = std::from_chars(text, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        static_cast<void>(std::fputs("application fixture: invalid integer argument\n", stderr));
        std::abort();
    }
    return value;
}

[[nodiscard]] inline unsigned short port_number() {
    const auto value = integer(port());
    if (value < 1 || std::cmp_greater(value, std::numeric_limits<unsigned short>::max())) {
        static_cast<void>(
            std::fputs("application fixture: port is outside the valid range\n", stderr));
        std::abort();
    }
    return static_cast<unsigned short>(value);
}
}  // namespace webcpp_fixture

#endif
