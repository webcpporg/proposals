// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/context/fiber.hpp>
#include <boost/core/lightweight_test.hpp>

#include <utility>

int main() {
    int destroyed = 0;

    struct guard {
        int& count;

        ~guard() { ++count; }
    };

    {
        boost::context::fiber fiber([&](boost::context::fiber caller) {
            const guard lifetime{destroyed};
            caller = std::move(caller).resume();
            BOOST_ERROR("destroying the suspended fiber must unwind without resuming its body");
            return caller;
        });
        fiber = std::move(fiber).resume();
        BOOST_TEST_EQ(destroyed, 0);
    }
    BOOST_TEST_EQ(destroyed, 1);
    return boost::report_errors();
}
