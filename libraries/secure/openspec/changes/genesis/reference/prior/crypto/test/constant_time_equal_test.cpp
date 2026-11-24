// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// constant_time_equal compares two digests, a digest and an array, or two spans, and tells them
// apart by any byte; spans of different sizes are unequal, and two empty spans are equal.

#include <webcpp/crypto/constant_time_equal.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/hash2/digest.hpp>
#include <boost/hash2/hmac.hpp>
#include <boost/hash2/sha2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace crypto = webcpp::crypto;

int main() {
    boost::hash2::digest<16> a{};
    boost::hash2::digest<16> b{};
    BOOST_TEST(crypto::constant_time_equal(a, b));
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            b = a;
            b.data()[i] ^= static_cast<unsigned char>(1U << bit);
            BOOST_TEST(!crypto::constant_time_equal(a, b));
        }
    }
    std::array<std::uint8_t, 16> const array{};
    BOOST_TEST(crypto::constant_time_equal(a, array));
    std::array<std::uint8_t, 15> const shorter{};
    BOOST_TEST(!crypto::constant_time_equal(a, shorter));
    BOOST_TEST(crypto::constant_time_equal(std::span<std::uint8_t const>(),
                                           std::span<std::uint8_t const>()));

    // Spans of one size: equal ones, and ones that differ in any byte (the first, the last).
    std::array<std::uint8_t, 8> const eight{1, 2, 3, 4, 5, 6, 7, 8};
    auto different_first = eight;
    different_first.front() ^= 0x80U;
    auto different_last = eight;
    different_last.back() ^= 0x01U;
    BOOST_TEST(crypto::constant_time_equal(std::span<std::uint8_t const>(eight), std::span(eight)));
    BOOST_TEST(!crypto::constant_time_equal(std::span<std::uint8_t const>(eight),
                                            std::span(different_first)));
    BOOST_TEST(!crypto::constant_time_equal(std::span<std::uint8_t const>(eight),
                                            std::span(different_last)));
    // Two 32-byte digests (a SHA-256 tag's size) that differ in the last byte only.
    boost::hash2::digest<32> const first_digest{};
    boost::hash2::digest<32> second_digest{};
    BOOST_TEST(crypto::constant_time_equal(first_digest, second_digest));
    second_digest.data()[31] = 1;
    BOOST_TEST(!crypto::constant_time_equal(first_digest, second_digest));

    // Hash2's MAC, which constant_time_equal serves too: two results of one key and message agree.
    boost::hash2::hmac_sha2_256 first(reinterpret_cast<unsigned char const*>("key"), 3);
    boost::hash2::hmac_sha2_256 second(reinterpret_cast<unsigned char const*>("key"), 3);
    first.update("message", 7);
    second.update("message", 7);
    BOOST_TEST(crypto::constant_time_equal(first.result(), second.result()));
    return webcpp::test::exit_status(boost::report_errors());
}
