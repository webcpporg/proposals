// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// webcpp::config::detail::timing_guard on Windows on ARM64 (_M_ARM64, not ARM64EC), where FEAT_DIT
// is read by one read of the register under structured exception handling: on the Windows 11 ARM64
// machine of the Windows ARM64 lane, whose CPU has FEAT_DIT (every Armv8.4 CPU does), cpu_has_dit()
// is true; DIT is set inside a guard, still set after the thread sleeps inside it (Windows keeps
// PSTATE across a context switch), and clear after it; a guard that finds DIT set leaves it set.
// Built on every target config runs: on one the guard does not cover (Windows x64 among them) the
// guard is an empty class and both functions answer false, even after a program's attempt to set
// DIT; on arm64 under an Apple system or Linux it checks only that a guard restores a clear DIT,
// and timing_guard_test checks the rest there.

#include <webcpp/config/detail/timing_guard.hpp>
#include <webcpp/test/exit_status.hpp>
#include <webcpp/test/timing_dit.hpp>

#include <boost/core/lightweight_test.hpp>

#include <type_traits>

#if defined(_MSC_VER) && defined(_M_ARM64) && !defined(_M_ARM64EC)
#define WEBCPP_TEST_TIMING_WINDOWS_ARM64 1
#include <chrono>
#include <thread>
#endif

namespace {

using webcpp::config::detail::cpu_has_dit;
using webcpp::config::detail::dit_is_set;
using webcpp::config::detail::timing_guard;
using webcpp::test::force_dit;

#ifdef WEBCPP_TEST_TIMING_WINDOWS_ARM64
// The CPU has FEAT_DIT, and the probe answers the same at every call.
bool cpu_has_dit_here() {
    bool const offered = cpu_has_dit();
    BOOST_TEST(offered);
    BOOST_TEST_EQ(cpu_has_dit(), offered);
    return offered;
}

// From clear: set inside a guard, still set after each of ten sleeps inside it, clear after it.
void guard_survives_sleeps() {
    force_dit(false);
    BOOST_TEST(!dit_is_set());
    {
        timing_guard const guard;
        BOOST_TEST(dit_is_set());
        for (int round = 0; round < 10; ++round) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            BOOST_TEST(dit_is_set());
        }
    }
    BOOST_TEST(!dit_is_set());
}

// From set, as a program sets it: still set after a guard; a disengaged guard changes nothing.
void program_set_dit_is_kept() {
    force_dit(true);
    {
        timing_guard const guard;
        BOOST_TEST(dit_is_set());
    }
    BOOST_TEST(dit_is_set());
    force_dit(false);
    {
        timing_guard const guard{false};
        BOOST_TEST(!dit_is_set());
    }
    BOOST_TEST(!dit_is_set());
}
#elif defined(WEBCPP_CONFIG_DETAIL_TIMING_COVERED)
// arm64 under an Apple system or Linux: a guard entered from clear leaves DIT clear (the full
// checks are timing_guard_test's; this program's subject is Windows).
void guard_restores_clear_dit() {
    force_dit(false);
    {
        timing_guard const guard;
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    BOOST_TEST(!dit_is_set());
}
#else
static_assert(std::is_empty_v<timing_guard>, "where the guard does not act, it is empty");

// Where the guard does not act, both functions answer false, also after force_dit(true), which
// does nothing there.
void uncovered_answers_false() {
    force_dit(true);
    BOOST_TEST(!cpu_has_dit());
    BOOST_TEST(!dit_is_set());
}
#endif

}  // namespace

int main() {
#ifdef WEBCPP_TEST_TIMING_WINDOWS_ARM64
    if (cpu_has_dit_here()) {
        guard_survives_sleeps();
        program_set_dit_is_kept();
    }
#elif defined(WEBCPP_CONFIG_DETAIL_TIMING_COVERED)
    guard_restores_clear_dit();
#else
    uncovered_answers_false();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
