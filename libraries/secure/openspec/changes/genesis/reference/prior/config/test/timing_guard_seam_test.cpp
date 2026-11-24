// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The seam of webcpp::config::detail::timing_guard, WEBCPP_CONFIG_DETAIL_TIMING_ENTERED, defined
// here before the first include as a library's test defines it: every engaged guard runs it once,
// with the std::source_location of the guard's declaration, before DIT is touched, on every target;
// a disengaged guard never runs it. The location's function_name() names the function that holds
// the guard and its file_name() the file that declares it, which is how a library's coverage test
// tells its own guards from another library's. Without the seam the constructor takes no
// location: timing_guard_test checks that.

#include <cstddef>
#include <source_location>

namespace webcpp::test {
namespace {

/** Records one engaged guard's entry; defined below, once the guard's header is included. */
void timing_entered(std::source_location const& where) noexcept;

}  // namespace
}  // namespace webcpp::test

/** The seam: records the entry into a guard declared at `where`. */
#define WEBCPP_CONFIG_DETAIL_TIMING_ENTERED(where) (::webcpp::test::timing_entered(where))

#include <webcpp/config/detail/timing_guard.hpp>
#include <webcpp/test/exit_status.hpp>
#include <webcpp/test/timing_dit.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>
#include <type_traits>

namespace webcpp::test {
namespace {

/** The seam's runs on this thread. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's record
thread_local std::size_t entries = 0;

/** Where the last guard the seam saw is declared. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's record
thread_local std::source_location last_entry{};

/** Whether DIT was set when the seam last ran. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's record
thread_local bool dit_at_seam = false;

void timing_entered(std::source_location const& where) noexcept {
    ++entries;
    last_entry = where;
    dit_at_seam = webcpp::config::detail::dit_is_set();
}

}  // namespace
}  // namespace webcpp::test

namespace {

using webcpp::config::detail::cpu_has_dit;
using webcpp::config::detail::dit_is_set;
using webcpp::config::detail::timing_guard;
using webcpp::test::force_dit;

static_assert(std::is_nothrow_constructible_v<timing_guard, bool, std::source_location>,
              "with the seam, the constructor takes the guard's location after engaged");

namespace seam_probe {

/** Holds an engaged guard, as a library's guarded function does. */
void guarded_function() {
    timing_guard const guard;
}

/** Holds a guard engaged explicitly, as a function keyed by its arguments does for a keyed call. */
void engaged_function() {
    timing_guard const guard{true};
}

/** Holds a disengaged guard, as a function keyed by its arguments does for an unkeyed call. */
void disengaged_function() {
    timing_guard const guard{false};
}

}  // namespace seam_probe

// An engaged guard, made either way, runs the seam exactly once, at its construction.
void engaged_guard_runs_the_seam_once() {
    std::size_t const before = webcpp::test::entries;
    {
        timing_guard const guard;
        BOOST_TEST_EQ(webcpp::test::entries, before + 1);
    }
    BOOST_TEST_EQ(webcpp::test::entries, before + 1);
    {
        timing_guard const guard{true};
    }
    BOOST_TEST_EQ(webcpp::test::entries, before + 2);
}

// A disengaged guard runs no seam.
void disengaged_guard_runs_no_seam() {
    std::size_t const before = webcpp::test::entries;
    seam_probe::disengaged_function();
    BOOST_TEST_EQ(webcpp::test::entries, before);
}

// The seam's location names the function that holds the guard, and the file that declares it,
// for a guard made either way: timing_guard{} and timing_guard{true}.
void seam_names_function_and_file() {
    seam_probe::guarded_function();
    std::string_view function = webcpp::test::last_entry.function_name();
    std::string_view file = webcpp::test::last_entry.file_name();
    BOOST_TEST(function.find("seam_probe::guarded_function") != std::string_view::npos);
    BOOST_TEST(file.ends_with("timing_guard_seam_test.cpp"));
    seam_probe::engaged_function();
    function = webcpp::test::last_entry.function_name();
    file = webcpp::test::last_entry.file_name();
    BOOST_TEST(function.find("seam_probe::engaged_function") != std::string_view::npos);
    BOOST_TEST(file.ends_with("timing_guard_seam_test.cpp"));
}

// The seam runs before DIT is touched: from clear, it finds DIT clear; then the guard sets it.
void seam_runs_before_dit() {
    force_dit(false);
    {
        timing_guard const guard;
        BOOST_TEST(!webcpp::test::dit_at_seam);
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    BOOST_TEST(!dit_is_set());
}

}  // namespace

int main() {
    engaged_guard_runs_the_seam_once();
    disengaged_guard_runs_no_seam();
    seam_names_function_and_file();
    seam_runs_before_dit();
    return webcpp::test::exit_status(boost::report_errors());
}
