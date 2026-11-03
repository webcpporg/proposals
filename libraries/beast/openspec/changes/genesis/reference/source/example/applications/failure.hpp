// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_APPLICATION_FAILURE_HPP
#define WEBCPP_BEAST_APPLICATION_FAILURE_HPP
#include <boost/system/error_code.hpp>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace webcpp_fixture {
// Exception-off application counterparts stop at a formerly throwing failure site.
[[noreturn]] inline void fail_stop() {
    static_cast<void>(
        std::fputs("application failure: exception-off fail-stop counterpart\n", stderr));
    std::abort();
}

template <class Resolver>
[[nodiscard]] auto resolve(Resolver& resolver, std::string_view host, std::string_view port) {
    boost::system::error_code error;
    auto results = resolver.resolve(host, port, error);
    if (error) {
        fail_stop();
    }
    return results;
}
}  // namespace webcpp_fixture
#endif
