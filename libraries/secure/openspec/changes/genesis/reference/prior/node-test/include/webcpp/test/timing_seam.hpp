// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The seam of webcpp::config::detail::timing_guard, config's guard, recorded for a library's tests:
// a library's test/support/timing_seam.hpp defines WEBCPP_TEST_TIMING_LIBRARY, the library's
// directory name ("crypto", "hash"), then includes this header, and a test includes that support
// header before any webcpp header, so that the seam is defined before
// <webcpp/config/detail/timing_guard.hpp> is first read and every engaged guard a call enters is
// counted, on every target, also where DIT does not apply. A guard is the library's own when one of
// the library's headers declares it: its source_location's file_name() holds
// include/webcpp/<library>/, with `/` or `\` around each word (MSVC writes backslashes, and joins
// an include directory to a header's path with either); the include root keeps a checkout whose
// own folders read webcpp/<library>/ from counting a test file's guard. timing_entries counts the
// library's own guards and timing_all_entries every guard: another library's, which a call may
// enter inside its own, and a test's own. timing_first_entry and timing_last_entry keep the
// function of the first guard since a reset and of the last, which a library's checks read.

#ifndef WEBCPP_TEST_TIMING_SEAM_HPP
#define WEBCPP_TEST_TIMING_SEAM_HPP

#ifdef WEBCPP_CONFIG_DETAIL_TIMING_GUARD_HPP
#error "<webcpp/test/timing_seam.hpp> must be included before any webcpp header"
#endif

#ifndef WEBCPP_TEST_TIMING_LIBRARY
#error "define WEBCPP_TEST_TIMING_LIBRARY, the library whose guards timing_entries counts"
#endif

#include <cstddef>
#include <source_location>
#include <string_view>

namespace webcpp::test {

/** Every timing guard this thread entered: the library's, another's, a test's. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's counter
inline thread_local std::size_t timing_all_entries = 0;

/** The timing guards of the library's headers this thread has entered. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's counter
inline thread_local std::size_t timing_entries = 0;

/**
 * The `function_name()` of the first guard, of any file, this thread entered since it was last
 * reset to null: the function that holds that guard, as the compiler spells it.
 */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's record
inline thread_local char const* timing_first_entry = nullptr;

/** The `function_name()` of the last guard, of any file, this thread entered. */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the seam's record
inline thread_local char const* timing_last_entry = nullptr;

/** The library whose guards timing_entries counts, WEBCPP_TEST_TIMING_LIBRARY. */
inline constexpr std::string_view timing_library = WEBCPP_TEST_TIMING_LIBRARY;

/** Whether `character` separates a path's components, on any system webcpp builds on. */
constexpr bool path_separator(char character) noexcept {
    return character == '/' || character == '\\';
}

/**
 * Whether a path names a file under a library's include directory: the components `include`,
 * `webcpp` and `library`, in that order, each followed by a separator, `/` or `\` in any mix, and
 * `include` the path's first component or after a separator.
 *
 * @param path A `file_name()`.
 * @param library The library's directory name.
 * @return Whether the path is under `include/webcpp/<library>/`.
 */
constexpr bool path_in_library(std::string_view path, std::string_view library) noexcept {
    constexpr std::string_view include_root = "include";
    constexpr std::string_view root = "webcpp";
    for (std::size_t at = path.find(include_root); at != std::string_view::npos;
         at = path.find(include_root, at + 1)) {
        std::size_t const root_at = at + include_root.size() + 1;
        std::size_t const library_at = root_at + root.size() + 1;
        std::size_t const after = library_at + library.size();
        if ((at == 0 || path_separator(path[at - 1])) && after < path.size() &&
            path_separator(path[root_at - 1]) && path.substr(root_at, root.size()) == root &&
            path_separator(path[library_at - 1]) &&
            path.substr(library_at, library.size()) == library && path_separator(path[after])) {
            return true;
        }
    }
    return false;
}

/**
 * Records the entry into a webcpp::config::detail::timing_guard: counts it, as the library's when
 * its file is under the library's include directory, and keeps its function as the first since
 * timing_first_entry was reset and as the last.
 *
 * @param where Where the guard is declared.
 */
inline void timing_entered(std::source_location const& where) noexcept {
    ++timing_all_entries;
    if (path_in_library(where.file_name(), timing_library)) {
        ++timing_entries;
    }
    if (timing_first_entry == nullptr) {
        timing_first_entry = where.function_name();
    }
    timing_last_entry = where.function_name();
}

}  // namespace webcpp::test

/** Records the entry into a webcpp::config::detail::timing_guard declared at `where`. */
#define WEBCPP_CONFIG_DETAIL_TIMING_ENTERED(where) (::webcpp::test::timing_entered(where))

#endif  // WEBCPP_TEST_TIMING_SEAM_HPP
