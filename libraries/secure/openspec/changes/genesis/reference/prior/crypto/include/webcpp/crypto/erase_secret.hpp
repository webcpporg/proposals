// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `erase_secret`, which zeroes a buffer the program owns, a password, a key or a plaintext, with
 stores the optimiser keeps.

 A `memset` of a buffer that is not read again is a dead store, which a compiler may remove:
 the secret then stays in memory after the program is done with it. `erase_secret` writes
 through a volatile pointer, then a signal fence, as `secret<N>` does when it dies.

 @see "Erasing a secret from a buffer", in the guide.
*/
#ifndef WEBCPP_CRYPTO_ERASE_SECRET_HPP
#define WEBCPP_CRYPTO_ERASE_SECRET_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/detail/wipe.hpp>

#include <ranges>
#include <type_traits>

namespace webcpp::crypto {

namespace detail {

/** A contiguous sized range whose elements the program may write and copy as bytes. */
template <class Range>
concept erasable_range =
    std::ranges::contiguous_range<Range> && std::ranges::sized_range<Range> &&
    (!std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<Range>>>) &&
    std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>;

}  // namespace detail

/**
 * Zeroes every byte of a buffer the program owns, with stores the optimiser keeps.
 *
 * It erases `size() * sizeof(element)` bytes from the range's first element: a `std::string`'s
 * size, not its capacity, and nothing a reallocation left behind. It calls no backend.
 *
 * @tparam Range A contiguous sized range of non-const, trivially copyable elements: a
 *   `std::vector<std::uint8_t>`, a `std::string`, a `std::array`, a `std::span<std::byte>`, a
 *   C array.
 * @param erased_range The buffer.
 */
template <class Range>
    requires detail::erasable_range<Range>
void erase_secret(Range&& erased_range) noexcept {
    detail::wipe(std::ranges::data(erased_range),
                 std::ranges::size(erased_range) * sizeof(std::ranges::range_value_t<Range>));
}

/**
 * A contiguous sized range of const or non-trivially-copyable elements: it stops the build with
 * a message naming the ranges `erase_secret` takes.
 *
 * @tparam Range The range refused.
 */
template <class Range>
    requires std::ranges::contiguous_range<Range> && std::ranges::sized_range<Range> &&
             (!detail::erasable_range<Range>)
void erase_secret(Range&& /*refused_range*/) noexcept {
    static_assert(detail::dependent_false<Range>,
                  "webcpp::crypto: erase_secret zeroes a buffer the program may write, of "
                  "trivially copyable elements: a std::vector<std::uint8_t>, a std::string, a "
                  "std::array, a std::span<std::byte>; this range's elements are const or not "
                  "trivially copyable.");
}

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_ERASE_SECRET_HPP
