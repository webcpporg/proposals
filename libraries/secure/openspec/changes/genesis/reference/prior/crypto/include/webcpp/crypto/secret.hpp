// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/** A secret output of webcpp::crypto, which wipes itself (design D9). */
#ifndef WEBCPP_CRYPTO_SECRET_HPP
#define WEBCPP_CRYPTO_SECRET_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/wipe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace webcpp::crypto {

/**
 `N` secret bytes: a shared secret, a session key, a converted secret key.

 They are wiped by volatile stores when the object is destroyed or moved from; a copy is made
 only by `clone`.

 The wipe covers the object's own storage only: a move or a `clone` may leave copies of the bytes in
 registers or temporaries, which the library does not reach.

 @tparam N The number of bytes.
*/
template <std::size_t N>
class secret {
    static_assert(N > 0, "webcpp::crypto: a secret of no bytes holds nothing.");

public:
    /** The number of bytes. */
    static constexpr std::size_t size_bytes = N;

    /** Zero bytes. */
    secret() noexcept = default;

    /**
     Takes the bytes of `moved_secret`, which is wiped.

     @param moved_secret The secret to take the bytes of.
    */
    secret(secret&& moved_secret) noexcept : bytes_(moved_secret.bytes_) {
        detail::wipe_object(moved_secret.bytes_);
    }

    /**
     Wipes these bytes, then takes the bytes of `moved_secret`, which is wiped; a move to itself
     changes nothing.

     @param moved_secret The secret to take the bytes of.
     @return This secret.
    */
    secret& operator=(secret&& moved_secret) noexcept {
        if (this != &moved_secret) {
            detail::wipe_object(bytes_);
            bytes_ = moved_secret.bytes_;
            detail::wipe_object(moved_secret.bytes_);
        }
        return *this;
    }

    /** Not copyable: a copy is asked for by name, with `clone`. */
    secret(secret const&) = delete;
    /** Not copyable: a copy is asked for by name, with `clone`. */
    secret& operator=(secret const&) = delete;

    /** Wipes the bytes. */
    ~secret() { detail::wipe_object(bytes_); }

    /**
     A copy of the bytes, which the caller then owns.

     @return A secret of the same bytes.
    */
    [[nodiscard]] secret clone() const noexcept {
        secret secret_copy;
        secret_copy.bytes_ = bytes_;
        return secret_copy;
    }

    /**
     The bytes, to read.

     @return A view of the bytes.
    */
    [[nodiscard]] std::span<std::uint8_t const, N> bytes() const& noexcept { return bytes_; }

    /**
     Not available on a secret that is about to be destroyed: the view would dangle.

     @return Nothing.
    */
    [[nodiscard]] std::span<std::uint8_t const, N> bytes() const&& = delete;

    /**
     The bytes, for the library's engines to write.

     @return A writable view of the bytes.
    */
    [[nodiscard]] std::span<std::uint8_t, N> writable() & noexcept { return bytes_; }

    /**
     Not available on a secret that is about to be destroyed: the view would dangle.

     @return Nothing.
    */
    std::span<std::uint8_t, N> writable() && = delete;

    /**
     The bytes, as a span of fixed size.

     @return A view of the bytes.
    */
    // Implicit, as the spec asks, so that a secret is passed where a function takes the span of
    // its bytes: misc-explicit-constructor is silenced for it.
    // NOLINTNEXTLINE(misc-explicit-constructor)
    operator std::span<std::uint8_t const, N>() const& noexcept { return bytes_; }

    /**
     Not available on a secret that is about to be destroyed: the view would dangle.

     @return Nothing.
    */
    operator std::span<std::uint8_t const, N>() const&& = delete;

    /**
     The number of bytes.

     @return `N`.
    */
    static constexpr std::size_t size() noexcept { return N; }

private:
    std::array<std::uint8_t, N> bytes_{};
};

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_SECRET_HPP
