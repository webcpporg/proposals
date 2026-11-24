// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// webcpp::config::detail::timing_guard, config's data-independent-timing guard, without the seam:
// an engaged guard (timing_guard{} or timing_guard{true}) sets DIT for its scope where the CPU has
// it and clears it after when it found it clear; a guard that finds DIT set, as a program that set
// it itself, leaves it set; a disengaged guard (timing_guard{false}) does nothing; cpu_has_dit()
// answers the same at every call. Without the seam the constructor takes a bool and no
// std::source_location, so that no function's name or header's path reaches a program. On a
// target the guard does not cover, it is an empty class and both functions answer false.

#include <webcpp/config/detail/timing_guard.hpp>
#include <webcpp/test/exit_status.hpp>
#include <webcpp/test/timing_dit.hpp>

#include <boost/core/lightweight_test.hpp>

#include <source_location>
#include <type_traits>

namespace {

using webcpp::config::detail::cpu_has_dit;
using webcpp::config::detail::dit_is_set;
using webcpp::config::detail::timing_guard;
using webcpp::test::force_dit;

static_assert(!std::is_copy_constructible_v<timing_guard> &&
                  !std::is_copy_assignable_v<timing_guard>,
              "a guard is not copyable: the state it restores is its scope's");
static_assert(!std::is_move_constructible_v<timing_guard> &&
                  !std::is_move_assignable_v<timing_guard>,
              "a guard is not movable: the state it restores is its scope's");
static_assert(std::is_nothrow_default_constructible_v<timing_guard> &&
                  std::is_nothrow_constructible_v<timing_guard, bool>,
              "a guard is made from nothing or from engaged, and never throws");
static_assert(!std::is_convertible_v<bool, timing_guard>, "the constructor is explicit");
static_assert(!std::is_constructible_v<timing_guard, bool, std::source_location> &&
                  !std::is_constructible_v<timing_guard, std::source_location>,
              "without the seam, a guard takes no source_location");
#ifndef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
static_assert(std::is_empty_v<timing_guard>, "where the guard does not act, it is an empty class");
#endif

// Where the guard does not act, both functions answer false.
void uncovered_answers_false() {
#ifndef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
    BOOST_TEST(!cpu_has_dit());
    BOOST_TEST(!dit_is_set());
#endif
}

// cpu_has_dit() is read once per process: every call answers as the first.
void cpu_answer_is_stable() {
    bool const first = cpu_has_dit();
    for (int call = 0; call < 3; ++call) {
        BOOST_TEST_EQ(cpu_has_dit(), first);
    }
}

// From clear: set inside an engaged guard, made either way, and clear after it.
void engaged_guard_sets_and_clears() {
    force_dit(false);
    BOOST_TEST(!dit_is_set());
    {
        // tag::guard[]
        webcpp::config::detail::timing_guard const
            guard;  // DIT set from here to the end of the scope
        // end::guard[]
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    BOOST_TEST(!dit_is_set());
    {
        timing_guard const guard{true};
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    BOOST_TEST(!dit_is_set());
}

// From set, as a program sets it: still set inside and after a guard (spec: a program that set
// DIT keeps it).
void program_set_dit_is_kept() {
    force_dit(true);
    BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    {
        timing_guard const guard;
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    force_dit(false);
    BOOST_TEST(!dit_is_set());
}

// A disengaged guard leaves DIT as it found it, clear or set.
void disengaged_guard_does_nothing() {
    for (bool const initially_set : {false, true}) {
        force_dit(initially_set);
        bool const before = dit_is_set();
        BOOST_TEST_EQ(before, initially_set && cpu_has_dit());
        {
            timing_guard const guard{false};
            BOOST_TEST_EQ(dit_is_set(), before);
        }
        BOOST_TEST_EQ(dit_is_set(), before);
    }
    force_dit(false);
}

}  // namespace

int main() {
    uncovered_answers_false();
    cpu_answer_is_stable();
    engaged_guard_sets_and_clears();
    program_set_dit_is_kept();
    disengaged_guard_does_nothing();
    return webcpp::test::exit_status(boost::report_errors());
}
