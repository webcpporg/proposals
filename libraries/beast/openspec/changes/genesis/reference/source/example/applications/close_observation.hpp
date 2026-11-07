// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_APPLICATION_CLOSE_OBSERVATION_HPP
#define WEBCPP_BEAST_APPLICATION_CLOSE_OBSERVATION_HPP

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace webcpp_fixture {
// Publish only after the protocol engine has reported its normal closed state.
inline void observe_normal_close() {
    const auto* path = std::getenv("WEBCPP_BEAST_FIXTURE_CLOSED");
    if (path == nullptr) {
        return;
    }
    const auto temporary = std::string(path) + ".tmp";
    {
        std::ofstream output(temporary);
        output << "closed\n";
        output.flush();
        if (!output) {
            std::abort();
        }
    }
    if (std::rename(temporary.c_str(), path) != 0) {
        std::abort();
    }
}
}  // namespace webcpp_fixture

#endif
