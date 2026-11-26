// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// crypto::erase_secret zeroes every byte of a contiguous range the program may write, and none
// around it; it takes no range that is not contiguous, and no pointer with a size; and a buffer
// erased just before its scope ends is zero in its storage afterwards, read at -O2 with every
// call inlined (the Jamfile's <optimization>speed <inlining>full), where a store the optimiser
// may drop is the case.

#include <webcpp/crypto/erase_secret.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace crypto = webcpp::crypto;

// Written through a concept, so that a range with no viable overload reads as false instead of
// stopping the build (a requires-expression outside a template is a hard error).
template <class Range>
concept erasable =
    requires(Range&& erased_range) { crypto::erase_secret(std::forward<Range>(erased_range)); };

static_assert(erasable<std::vector<std::uint8_t>&>);
static_assert(erasable<std::span<std::byte>>);
static_assert(!erasable<std::list<char>&>);
static_assert(!erasable<std::vector<bool>&>);

// Whether erase_secret takes a pointer and a size, the overload it has none of: a range carries
// its own size (design D10).
template <class Pointer>
concept erasable_by_pointer_and_size = requires(Pointer erased_data, std::size_t byte_count) {
    crypto::erase_secret(erased_data, byte_count);
};

static_assert(!erasable_by_pointer_and_size<std::uint8_t*>);
static_assert(!erasable_by_pointer_and_size<char*>);
static_assert(!erasable_by_pointer_and_size<void*>);
static_assert(noexcept(crypto::erase_secret(std::declval<std::vector<std::uint8_t>&>())));

// A function the optimiser cannot see into, so that what it is handed is read from memory.
#if defined(_MSC_VER) && !defined(__clang__)
#define WEBCPP_TEST_NOINLINE __declspec(noinline)
#else
#define WEBCPP_TEST_NOINLINE __attribute__((noinline))
#endif

// Whether every byte at `where` is zero, read through a volatile pointer in a function that is
// never inlined: a store that was dropped before the call is found here. It may be handed the
// address of an object that is gone (the point of the scope's check), so it is exempt from the
// sanitizers that would stop that read.
#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("address")))
#endif
WEBCPP_TEST_NOINLINE bool memory_is_zero(void const* where, std::size_t count) {
    auto const volatile* const bytes = static_cast<unsigned char const volatile*>(where);
    for (std::size_t i = 0; i < count; ++i) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

// A buffer in a scope of its own, which a call the optimiser cannot see into is handed, is
// erased as the scope's last statement; its storage, in this frame, is read after the scope. An
// erasure that is a dead store (a plain write to an object about to die) is dropped by the
// optimiser and the bytes remain.
WEBCPP_TEST_NOINLINE bool erasure_before_scope_end_stays() {
    std::uint8_t const* where = nullptr;
    constexpr std::size_t count = 32;
    {
        std::array<std::uint8_t, count> password{};
        for (std::size_t i = 0; i < count; ++i) {
            password[i] = static_cast<std::uint8_t>(i + 1);
        }
        where = password.data();
        if (memory_is_zero(where, count)) {
            return false;
        }
        crypto::erase_secret(password);
    }
    return memory_is_zero(where, count);
}

}  // namespace

int main() {
    // A vector of bytes.
    std::vector<std::uint8_t> key(32, 0xa5);
    crypto::erase_secret(key);
    BOOST_TEST_EQ(key.size(), 32U);
    BOOST_TEST(memory_is_zero(key.data(), key.size()));

    // A string: its size() bytes, its terminator kept, its size unchanged.
    // tag::erase[]
    std::string password(200, '\0');
    password = "correct horse battery staple";  // read into a buffer reserved beforehand
    // ... the password is checked or hashed ...
    webcpp::crypto::erase_secret(password);
    // end::erase[]
    BOOST_TEST_EQ(password.size(), 28U);
    BOOST_TEST(memory_is_zero(password.data(), password.size()));
    BOOST_TEST_EQ(password[password.size()], '\0');

    // An array of 33 bytes, a compressed public key's size.
    std::array<std::uint8_t, 33> point{};
    point.fill(0x5a);
    crypto::erase_secret(point);
    BOOST_TEST(memory_is_zero(point.data(), point.size()));

    // A span of std::byte over part of a stack buffer: the bytes around it are untouched.
    std::array<std::byte, 18> framed{};
    framed.fill(std::byte{0xc3});
    crypto::erase_secret(std::span<std::byte>(framed).subspan(1, 16));
    BOOST_TEST(framed.front() == std::byte{0xc3});
    BOOST_TEST(framed.back() == std::byte{0xc3});
    BOOST_TEST(memory_is_zero(framed.data() + 1, 16));

    // A C array, which erase_secret takes as a program may hold one.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays): the C array is what this case erases
    unsigned char raw[16];
    for (auto& byte : raw) {
        byte = 0x77;
    }
    crypto::erase_secret(raw);
    BOOST_TEST(memory_is_zero(raw, sizeof raw));

    // A vector of words: size() * sizeof(element) bytes.
    std::vector<std::uint32_t> words(8, 0xdeadbeefU);
    crypto::erase_secret(words);
    BOOST_TEST(memory_is_zero(words.data(), words.size() * sizeof(std::uint32_t)));

    // An empty vector: nothing to erase.
    std::vector<std::uint8_t> empty;
    crypto::erase_secret(empty);
    BOOST_TEST(empty.empty());

    // A buffer erased as its scope ends is zero afterwards, at -O2 with every call inlined.
    BOOST_TEST(erasure_before_scope_end_stays());

    return webcpp::test::exit_status(boost::report_errors());
}
