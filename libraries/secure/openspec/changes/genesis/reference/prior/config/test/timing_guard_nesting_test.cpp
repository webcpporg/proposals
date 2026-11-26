// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Guards nest, as one library's guarded function calls another's (crypto inside hash, hash inside
// crypto, codecs inside either): the guard is one class, so the nesting is the class with itself.
// Three levels in every order of engaged and disengaged guards, from DIT clear and from DIT set:
// inside each level DIT is set when it was set before or when any guard at or above that level is
// engaged (on a CPU with DIT); after each level it is as inside the level above; after the
// outermost, as before. Then the spec's scenario: three engaged guards with a disengaged one
// between them.

#include <webcpp/config/detail/timing_guard.hpp>
#include <webcpp/test/exit_status.hpp>
#include <webcpp/test/timing_dit.hpp>

#include <boost/core/lightweight_test.hpp>

namespace {

using webcpp::config::detail::cpu_has_dit;
using webcpp::config::detail::dit_is_set;
using webcpp::config::detail::timing_guard;
using webcpp::test::force_dit;

/**
 * Three guards nested, engaged as `pattern`'s bits say (bit 0 the outermost, bit 2 the innermost),
 * from DIT set or clear.
 */
void nest(unsigned pattern, bool initially_set) {
    force_dit(initially_set);
    bool const offered = cpu_has_dit();
    bool const before = initially_set && offered;
    bool const outer_engaged = (pattern & 1U) != 0;
    bool const middle_engaged = (pattern & 2U) != 0;
    bool const inner_engaged = (pattern & 4U) != 0;
    auto const expected = [before, offered](bool any_engaged) {
        return before || (offered && any_engaged);
    };
    BOOST_TEST_EQ(dit_is_set(), before);
    {
        timing_guard const outer{outer_engaged};
        BOOST_TEST_EQ(dit_is_set(), expected(outer_engaged));
        {
            timing_guard const middle{middle_engaged};
            BOOST_TEST_EQ(dit_is_set(), expected(outer_engaged || middle_engaged));
            {
                timing_guard const inner{inner_engaged};
                BOOST_TEST_EQ(dit_is_set(),
                              expected(outer_engaged || middle_engaged || inner_engaged));
            }
            BOOST_TEST_EQ(dit_is_set(), expected(outer_engaged || middle_engaged));
        }
        BOOST_TEST_EQ(dit_is_set(), expected(outer_engaged));
    }
    BOOST_TEST_EQ(dit_is_set(), before);
    force_dit(false);
}

// Every order of engaged and disengaged guards over three levels, from clear and from set.
void every_order() {
    for (unsigned pattern = 0; pattern < 8; ++pattern) {
        nest(pattern, false);
        nest(pattern, true);
    }
}

// The spec's scenario: codecs' guard, a disengaged one (an unkeyed hash call), hash's, crypto's,
// from clear on a CPU with DIT: set inside every level, still set after each inner guard ends,
// clear after the outermost.
void three_libraries_with_a_disengaged_guard_between() {
    force_dit(false);
    bool const offered = cpu_has_dit();
    {
        timing_guard const codecs;
        BOOST_TEST_EQ(dit_is_set(), offered);
        {
            timing_guard const unkeyed{false};
            BOOST_TEST_EQ(dit_is_set(), offered);
            {
                timing_guard const hash;
                BOOST_TEST_EQ(dit_is_set(), offered);
                {
                    timing_guard const crypto;
                    BOOST_TEST_EQ(dit_is_set(), offered);
                }
                BOOST_TEST_EQ(dit_is_set(), offered);
            }
            BOOST_TEST_EQ(dit_is_set(), offered);
        }
        BOOST_TEST_EQ(dit_is_set(), offered);
    }
    BOOST_TEST(!dit_is_set());
}

}  // namespace

int main() {
    every_order();
    three_libraries_with_a_disengaged_guard_between();
    return webcpp::test::exit_status(boost::report_errors());
}
