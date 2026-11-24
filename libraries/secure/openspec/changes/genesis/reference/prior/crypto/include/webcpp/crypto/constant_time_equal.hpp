// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `constant_time_equal`, the constant-time comparison of tags and MACs, Boost.Hash2's included.
*/
#ifndef WEBCPP_CRYPTO_CONSTANT_TIME_EQUAL_HPP
#define WEBCPP_CRYPTO_CONSTANT_TIME_EQUAL_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/timing.hpp>

#include <boost/hash2/digest.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace webcpp::crypto {

/**
 * Whether two byte sequences are equal, in a time that does not depend on their contents.
 *
 * Every byte is read, and no branch or index depends on one: the XOR of each pair is
 * accumulated with OR in a volatile local, read once after the loop, and reduced to one bit
 * without a branch, as BearSSL's tag check does. Sizes are public: spans of different sizes
 * compare unequal without reading their contents. On 64-bit ARM the comparison runs with DIT
 * set where the CPU has it, and leaves the thread's DIT as it found it.
 *
 * @param first_operand The first sequence, a tag or a MAC.
 * @param second_operand The second.
 * @return Whether they have the same size and the same bytes.
 */
[[nodiscard]] inline bool constant_time_equal(
    std::span<std::uint8_t const> first_operand,
    std::span<std::uint8_t const> second_operand) noexcept {
    detail::timing_guard const timing_scope;
    if (first_operand.size() != second_operand.size()) {
        return false;
    }
    unsigned volatile accumulated_difference = 0;
    for (std::size_t byte_index = 0; byte_index < first_operand.size(); ++byte_index) {
        accumulated_difference =
            accumulated_difference |
            static_cast<unsigned>(first_operand[byte_index] ^ second_operand[byte_index]);
    }
    unsigned const total_difference = accumulated_difference;
    // all is at most 255: all - 1 wraps to the type's maximum only when all is 0.
    return (((total_difference - 1U) >> 8U) & 1U) != 0;
}

/**
 * Whether two digests are equal, in a time that does not depend on their contents.
 *
 * Boost.Hash2's own `operator==` is `memcmp`, which is not constant time: a MAC is checked
 * with this one.
 *
 * @tparam N The digests' size in bytes.
 * @param first_operand The first digest.
 * @param second_operand The second.
 * @return Whether every byte is equal.
 */
template <std::size_t N>
[[nodiscard]] bool constant_time_equal(boost::hash2::digest<N> const& first_operand,
                                       boost::hash2::digest<N> const& second_operand) noexcept {
    detail::timing_guard const timing_scope;
    return constant_time_equal(std::span<std::uint8_t const>(first_operand.data(), N),
                               std::span<std::uint8_t const>(second_operand.data(), N));
}

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_CONSTANT_TIME_EQUAL_HPP
