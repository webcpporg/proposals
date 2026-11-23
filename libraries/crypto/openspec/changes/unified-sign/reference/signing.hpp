// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What every curve's signing shares (design D1 to D3): the scheme tags, `eddsa`, `ecdsa_sha256`,
 `ecdsa_sha256_recoverable` and `schnorr`, and the library's one list of them,
 `detail::all_schemes`; the marker of a digest, `prehashed{digest}`; the
 concepts `can_sign` and `can_verify`, which say which key, scheme and input go together; the
 free synchronous `sign(key_pair, scheme, input)`, the key pair's `sign` (design D9); and the
 catch-alls of `sign` and `verify`, which stop the build with what goes together.

 A signature's input is a message, any bytes that convert to `std::span<std::uint8_t const>`, of
 any size, empty included, or a digest marked as one, `prehashed{digest}`: no call tells the two
 apart by a span's size. Each curve's header includes this one and declares, for its own key
 types, the schemes they have, through the traits of `detail`.

 @see "Signatures: the scheme and the input", in the guide.
*/
#ifndef WEBCPP_CRYPTO_SIGNING_HPP
#define WEBCPP_CRYPTO_SIGNING_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/asynchronous.hpp>
#include <webcpp/crypto/detail/timing.hpp>
#include <webcpp/hash/algorithms.hpp>  // webcpp::hash::sha2_256, ECDSA's hash of a message

#include <boost/hash2/digest.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

namespace webcpp::crypto {

/**
 * The type of @ref eddsa, Ed25519's signature scheme (RFC 8032's pure Ed25519).
 *
 * Its explicit default constructor keeps `{}` from standing for a scheme.
 */
struct eddsa_t {
    /** Makes the tag; explicit, so that only its name makes one. */
    explicit eddsa_t() = default;
};

/**
 * The type of @ref ecdsa_sha256, ECDSA over SHA-256, on P-256 and secp256k1.
 *
 * Its explicit default constructor keeps `{}` from standing for a scheme.
 */
struct ecdsa_sha256_t {
    /** Makes the tag; explicit, so that only its name makes one. */
    explicit ecdsa_sha256_t() = default;
};

/**
 * The type of @ref ecdsa_sha256_recoverable, ECDSA over SHA-256 with the recovery id, on
 * secp256k1.
 *
 * Its explicit default constructor keeps `{}` from standing for a scheme.
 */
struct ecdsa_sha256_recoverable_t {
    /** Makes the tag; explicit, so that only its name makes one. */
    explicit ecdsa_sha256_recoverable_t() = default;
};

/**
 * The type of @ref schnorr, BIP340's Schnorr signatures, on secp256k1.
 *
 * Its explicit default constructor keeps `{}` from standing for a scheme.
 */
struct schnorr_t {
    /** Makes the tag; explicit, so that only its name makes one. */
    explicit schnorr_t() = default;
};

namespace detail {

/**
 * A list of scheme tags, which the checks that hold for every scheme fold over.
 *
 * @tparam Schemes Scheme tags.
 */
template <class... Schemes>
struct scheme_list {};

/**
 * The library's one list of schemes, every scheme tag: a new scheme is one more tag here, and
 * `is_scheme_v` and the key pair contract, `crypto::keypair`, take it from this list.
 */
using all_schemes = scheme_list<eddsa_t, ecdsa_sha256_t, ecdsa_sha256_recoverable_t, schnorr_t>;

/**
 * Whether `Type` is a member of the list `List`: false for a type that is not a `scheme_list`.
 *
 * @tparam Type Any type.
 * @tparam List A `scheme_list`.
 */
template <class Type, class List>
inline constexpr bool in_scheme_list_v = false;

/**
 * Whether `Type` is one of `Schemes`.
 *
 * @tparam Type Any type.
 * @tparam Schemes The list's scheme tags.
 */
template <class Type, class... Schemes>
inline constexpr bool in_scheme_list_v<Type, scheme_list<Schemes...>> =
    (std::is_same_v<Type, Schemes> || ...);

/**
 * Whether `Type` is one of the scheme tags, a member of `all_schemes`.
 *
 * @tparam Type Any type.
 */
template <class Type>
inline constexpr bool is_scheme_v = in_scheme_list_v<Type, all_schemes>;

}  // namespace detail

/**
 * Ed25519's signature scheme, RFC 8032's pure Ed25519: it signs the message itself, of any
 * length, and takes no digest; its signature is 64 bytes.
 */
inline constexpr eddsa_t eddsa{};

/**
 * ECDSA over SHA-256: the message is hashed with SHA-256 inside the call, or a 32-byte digest is
 * given as `prehashed{digest}`; its signature is 64 bytes, r and s, with a low s.
 */
inline constexpr ecdsa_sha256_t ecdsa_sha256{};

/**
 * ECDSA over SHA-256 with the recovery id, secp256k1's: its signature is 65 bytes, r, s and the
 * id, from which `recover` gives the public key.
 */
inline constexpr ecdsa_sha256_recoverable_t ecdsa_sha256_recoverable{};

/**
 * BIP340's Schnorr signatures, secp256k1's: it signs the message itself, of any length, and has
 * no prehash mode; its signature is 64 bytes, against a 32-byte x-only key.
 */
inline constexpr schnorr_t schnorr{};

/**
 * A digest given to a scheme that signs one, in place of the message it hashes:
 * `key.sign(crypto::ecdsa_sha256, crypto::prehashed{digest})`.
 *
 * It refers to the digest, which stays the caller's: the marker must not outlive it, so a marker
 * of a temporary, such as `prehashed{hasher.result()}`, is valid only within the full expression
 * that makes it, as the argument of a call. Its size is fixed by its type, deduced from a
 * `std::array<std::uint8_t, N>`, a `std::span<std::uint8_t const, N>` (or of `std::uint8_t`) or a
 * `boost::hash2::digest<N>`; a span of a dynamic size stops the build. No conversion turns a span
 * or an array into a marker, or a marker into a span: a message is never read as a digest, nor a
 * digest as a message.
 *
 * @tparam N The digest's size in bytes, never `std::dynamic_extent`.
 */
template <std::size_t N>
struct prehashed {
    static_assert(N != std::dynamic_extent,
                  "webcpp::crypto: crypto::prehashed takes a digest of a fixed size: a "
                  "std::array<std::uint8_t, N> or a std::span<std::uint8_t const, N>.");

    /** The digest's bytes, the caller's. */
    std::span<std::uint8_t const, N> digest;

    /**
     * Marks the bytes `digest_bytes` views as a digest.
     *
     * @param digest_bytes The digest, which must outlive the marker's use.
     */
    constexpr explicit prehashed(std::span<std::uint8_t const, N> digest_bytes) noexcept
        : digest(digest_bytes) {}

    /**
     * Marks a Boost.Hash2 digest, a hasher's result, as a digest.
     *
     * @param hashed_digest The digest, which must outlive the marker's use.
     */
    constexpr explicit prehashed(boost::hash2::digest<N> const& hashed_digest) noexcept
        requires(N != std::dynamic_extent)
        : digest(hashed_digest.data(), N) {}
};

/**
 * Deduces the size of a digest held in an array.
 *
 * @tparam N The digest's size in bytes.
 */
template <std::size_t N>
prehashed(std::array<std::uint8_t, N> const&) -> prehashed<N>;

/**
 * Deduces the size of a digest viewed by a span of constant bytes; a dynamic size stops the
 * build.
 *
 * @tparam N The digest's size in bytes.
 */
template <std::size_t N>
prehashed(std::span<std::uint8_t const, N>) -> prehashed<N>;

/**
 * Deduces the size of a digest viewed by a span of bytes; a dynamic size stops the build.
 *
 * @tparam N The digest's size in bytes.
 */
template <std::size_t N>
prehashed(std::span<std::uint8_t, N>) -> prehashed<N>;

/**
 * Deduces the size of a Boost.Hash2 digest, a hasher's result.
 *
 * @tparam N The digest's size in bytes.
 */
template <std::size_t N>
prehashed(boost::hash2::digest<N> const&) -> prehashed<N>;

namespace detail {

/** False for every type, so that a refusal's `static_assert` fires only when it is reached. */
template <class Type>
inline constexpr bool refused_v = false;

/**
 * The sizes of a scheme's signature and of the digest it takes, specialised for each tag.
 *
 * @tparam Scheme A scheme tag.
 */
template <class Scheme>
struct scheme_traits;

/** Ed25519's: a 64-byte signature, and no digest. */
template <>
struct scheme_traits<eddsa_t> {
    /** The signature's size in bytes. */
    static constexpr std::size_t signature_size = 64;
    /** The digest's size in bytes, 0: the scheme takes none. */
    static constexpr std::size_t digest_size = 0;
};

/** ECDSA over SHA-256's: a 64-byte signature, and SHA-256's 32-byte digest. */
template <>
struct scheme_traits<ecdsa_sha256_t> {
    /** The signature's size in bytes. */
    static constexpr std::size_t signature_size = 64;
    /** The digest's size in bytes, SHA-256's. */
    static constexpr std::size_t digest_size = 32;
};

/** Recoverable ECDSA over SHA-256's: a 65-byte signature, and SHA-256's 32-byte digest. */
template <>
struct scheme_traits<ecdsa_sha256_recoverable_t> {
    /** The signature's size in bytes, with the recovery id. */
    static constexpr std::size_t signature_size = 65;
    /** The digest's size in bytes, SHA-256's. */
    static constexpr std::size_t digest_size = 32;
};

/** BIP340's: a 64-byte signature, and no digest. */
template <>
struct scheme_traits<schnorr_t> {
    /** The signature's size in bytes. */
    static constexpr std::size_t signature_size = 64;
    /** The digest's size in bytes, 0: the scheme takes none. */
    static constexpr std::size_t digest_size = 0;
};

/**
 * Whether `Type` is a marker, `prehashed<N>`.
 *
 * @tparam Type The input's type, without its reference and qualifiers.
 */
template <class Type>
inline constexpr bool is_prehashed_v = false;

/**
 * Whether a marker is a marker: it is.
 *
 * @tparam N The digest's size in bytes.
 */
template <std::size_t N>
inline constexpr bool is_prehashed_v<prehashed<N>> = true;

/**
 * Whether `Type` is a message: bytes that convert to `std::span<std::uint8_t const>`, of any
 * size, and not a marker.
 *
 * @tparam Type The input's type, without its reference and qualifiers.
 */
template <class Type>
inline constexpr bool is_message_v =
    !is_prehashed_v<Type> && std::is_convertible_v<Type const&, std::span<std::uint8_t const>>;

/**
 * Whether the key pair `Keypair` signs with the scheme `Scheme`: false, unless the key pair's
 * curve says so, for its key pair types alone (a public key or bytes never sign).
 *
 * A curve's header specialises it: the header is included before `can_sign` or this trait is
 * used on its key pairs, or the specialisation comes after the first use (ill-formed).
 *
 * @tparam Keypair A key pair's type, without its reference and qualifiers.
 * @tparam Scheme A scheme tag.
 */
template <class Keypair, class Scheme>
inline constexpr bool signs_with_v = false;

/**
 * Whether the public key `PublicKey` verifies the scheme `Scheme`: false, unless the key's curve
 * says so, for its public key types and for the byte keys of its schemes (a key pair never
 * verifies: its public key does).
 *
 * A curve's header specialises it: the header is included before `can_verify`, `crypto::verify`
 * or this trait is used on its public keys, or the specialisation comes after the first use
 * (ill-formed).
 *
 * @tparam PublicKey A public key's type, a curve's type or bytes, without its reference and
 * qualifiers.
 * @tparam Scheme A scheme tag.
 */
template <class PublicKey, class Scheme>
inline constexpr bool verifies_with_v = false;

/**
 * The backend of a key type, `void` for a byte key; each curve specialises it for its own, in
 * its header, which is included before the trait is used on its keys.
 *
 * @tparam Key A key pair's or a public key's type.
 */
template <class Key>
struct key_backend {
    /** The backend, `void`: none. */
    using type = void;
};

/**
 * Whether a key whose backend is `Backend` signs and verifies a digest: every backend that does
 * not complete by itself (WebCrypto has no prehash mode); a byte key, whose backend is `void`,
 * does.
 *
 * @tparam Backend The key's backend, or `void`.
 */
template <class Backend>
inline constexpr bool signs_prehashed_v = !asynchronous_backend_v<Backend>;

/**
 * Whether `Input` is a valid input of `Scheme` on `Backend`: a message, on every scheme and
 * backend.
 *
 * @tparam Scheme A scheme tag.
 * @tparam Input The input's type, without its reference and qualifiers.
 * @tparam Backend The key's backend, or `void`.
 */
template <class Scheme, class Input, class Backend>
inline constexpr bool input_valid_v = is_message_v<Input>;

/**
 * Whether a marker is a valid input of `Scheme` on `Backend`: when the scheme takes a digest of
 * its size and the backend signs one.
 *
 * @tparam Scheme A scheme tag.
 * @tparam N The digest's size in bytes.
 * @tparam Backend The key's backend, or `void`.
 */
template <class Scheme, std::size_t N, class Backend>
inline constexpr bool input_valid_v<Scheme, prehashed<N>, Backend> =
    scheme_traits<Scheme>::digest_size != 0 && N == scheme_traits<Scheme>::digest_size &&
    signs_prehashed_v<Backend>;

/**
 * Stops the build with the first reason `Input` is not a valid input of `Scheme` on `Backend`,
 * one message alone, and does nothing for a valid one.
 *
 * @tparam Scheme A scheme tag.
 * @tparam Input The input's type, without its reference and qualifiers.
 * @tparam Backend The key's backend, or `void`.
 */
template <class Scheme, class Input, class Backend>
constexpr void check_input() noexcept {
    if constexpr (!is_prehashed_v<Input>) {
        static_assert(is_message_v<Input>,
                      "webcpp::crypto: a signature's input is a message (bytes, as "
                      "std::span<std::uint8_t const>) or crypto::prehashed{digest}.");
    } else if constexpr (std::is_same_v<Scheme, eddsa_t>) {
        static_assert(refused_v<Input>,
                      "webcpp::crypto: crypto::eddsa signs the message itself (RFC 8032's pure "
                      "Ed25519) and takes no digest; Ed25519ph, which hashes the message, is "
                      "curves::ed25519::ph.");
    } else if constexpr (std::is_same_v<Scheme, schnorr_t>) {
        static_assert(refused_v<Input>,
                      "webcpp::crypto: crypto::schnorr (BIP340) signs a message of any length and "
                      "has no prehash mode: pass the message.");
    } else if constexpr (decltype(Input::digest)::extent != scheme_traits<Scheme>::digest_size &&
                         std::is_same_v<Scheme, ecdsa_sha256_recoverable_t>) {
        static_assert(refused_v<Input>,
                      "webcpp::crypto: crypto::ecdsa_sha256_recoverable takes a 32-byte digest, "
                      "SHA-256's: crypto::prehashed{digest} of a std::array<std::uint8_t, 32>.");
    } else if constexpr (decltype(Input::digest)::extent != scheme_traits<Scheme>::digest_size) {
        static_assert(refused_v<Input>,
                      "webcpp::crypto: crypto::ecdsa_sha256 takes a 32-byte digest, SHA-256's: "
                      "crypto::prehashed{digest} of a std::array<std::uint8_t, 32>.");
    } else if constexpr (!signs_prehashed_v<Backend>) {
        static_assert(refused_v<Input>,
                      "webcpp::crypto: this backend cannot sign or verify a digest (WebCrypto has "
                      "no prehash mode): pass the message.");
    }
}

/**
 * The bytes of a message, viewed where they are.
 *
 * @tparam Input A message's type: bytes that convert to `std::span<std::uint8_t const>`.
 * @param message_input A message: bytes that convert to `std::span<std::uint8_t const>`.
 * @return The view of its bytes.
 */
template <class Input>
[[nodiscard]] constexpr std::span<std::uint8_t const> message_bytes(
    Input const& message_input) noexcept {
    return std::span<std::uint8_t const>(message_input);
}

/**
 * SHA-256 of a message (webcpp::hash's `sha2_256`, Boost.Hash2's): the digest ECDSA over SHA-256
 * signs and verifies, as WebCrypto's ECDSA with SHA-256 hashes it.
 *
 * @param message_bytes Public bytes, of any size.
 * @return The 32-byte digest.
 */
[[nodiscard]] inline std::array<std::uint8_t, 32> sha256_of(
    std::span<std::uint8_t const> message_bytes) noexcept {
    ::webcpp::hash::sha2_256 sha256_hasher;
    sha256_hasher.update(message_bytes.data(), message_bytes.size());
    static_assert(std::is_same_v<::webcpp::hash::sha2_256::result_type, boost::hash2::digest<32>>);
    auto const hash_result = sha256_hasher.result();
    std::array<std::uint8_t, 32> digest_bytes{};
    std::copy_n(hash_result.data(), digest_bytes.size(), digest_bytes.begin());
    return digest_bytes;
}

/**
 * Whether a curve gives `crypto::verify` of `Scheme` for the public key `Key`: false here; each
 * curve specialises it for its own keys, and the catch-all `verify` steps aside where it is true.
 * The curve's header is included before a call or the trait names its keys.
 *
 * @tparam Scheme A scheme tag.
 * @tparam Key The public key's type, without its reference and qualifiers.
 */
template <class Scheme, class Key>
inline constexpr bool has_verifier_v = false;

}  // namespace detail

/**
 * Whether the key pair `Keypair` signs `Input` with `Scheme`: when its curve has the scheme and
 * the input is a message, or a marker of the scheme's digest size on a backend that signs one.
 *
 * @tparam Keypair A key pair's type.
 * @tparam Scheme A scheme tag.
 * @tparam Input The input's type: a message, or `prehashed<N>`.
 */
template <class Keypair, class Scheme, class Input>
concept can_sign =
    detail::signs_with_v<std::remove_cvref_t<Keypair>, Scheme> &&
    detail::input_valid_v<Scheme, std::remove_cvref_t<Input>,
                          typename detail::key_backend<std::remove_cvref_t<Keypair>>::type>;

/**
 * Whether `crypto::verify` checks a signature of `Scheme` over `Input` against `PublicKey`: when
 * the key's curve has the scheme and the input is a message, or a marker of the scheme's digest
 * size on a backend that verifies one.
 *
 * @tparam Scheme A scheme tag.
 * @tparam Input The input's type: a message, or `prehashed<N>`.
 * @tparam PublicKey The public key's type, a curve's type or bytes.
 */
template <class Scheme, class Input, class PublicKey>
concept can_verify =
    detail::verifies_with_v<std::remove_cvref_t<PublicKey>, Scheme> &&
    detail::input_valid_v<Scheme, std::remove_cvref_t<Input>,
                          typename detail::key_backend<std::remove_cvref_t<PublicKey>>::type>;

namespace detail {

/**
 * Whether `key.sign(scheme, input, extra...)` names a member `sign` of `Keypair` that takes these
 * arguments: the key pair's own signing function, which `crypto::sign` calls.
 *
 * @tparam Keypair A key pair's type, or any type.
 * @tparam Scheme A scheme tag, or any type.
 * @tparam Input The input's type.
 * @tparam Extra The types of the arguments after the input, secp256k1's 32 bytes.
 */
template <class Keypair, class Scheme, class Input, class... Extra>
concept has_sign_member = requires {
    std::declval<Keypair const&>().sign(std::declval<Scheme const&>(), std::declval<Input const&>(),
                                        std::declval<Extra const&>()...);
};

}  // namespace detail

/**
 * Signs with a key pair, `key_pair.sign(signing_scheme, signing_input, extra_arguments...)`: the
 * free form of the key pair's own `sign`, which it calls, with its results, its errors and its
 * refusals.
 *
 * A scheme the key pair does not have, or an input the scheme does not take, stops the build with
 * the key pair's own message.
 *
 * @tparam Keypair A key pair's type, whose `sign` takes these arguments: Ed25519's, P-256's or
 * secp256k1's.
 * @tparam Scheme A scheme tag.
 * @tparam Input A message, bytes that convert to `std::span<std::uint8_t const>`, or
 * `crypto::prehashed<N>`.
 * @tparam Extra Nothing, or secp256k1's 32 bytes: the hedged ECDSA's extra entropy or BIP340's
 * auxiliary randomness.
 * @param key_pair The key pair.
 * @param signing_scheme The scheme.
 * @param signing_input The message or the marker.
 * @param extra_arguments Nothing, or secp256k1's 32 bytes.
 * @return What `key_pair.sign(signing_scheme, signing_input, extra_arguments...)` returns: the
 * signature, or its error.
 */
template <class Keypair, class Scheme, class Input, class... Extra>
    requires detail::has_sign_member<Keypair, Scheme, Input, Extra...>
[[nodiscard]] auto sign(
    Keypair const& key_pair, Scheme signing_scheme, Input const& signing_input,
    Extra const&... extra_arguments) noexcept(noexcept(key_pair.sign(signing_scheme, signing_input,
                                                                     extra_arguments...)))
    -> decltype(key_pair.sign(signing_scheme, signing_input, extra_arguments...)) {
    detail::timing_guard const timing;
    return key_pair.sign(signing_scheme, signing_input, extra_arguments...);
}

/**
 * Stops the build: the first argument has no `sign` that takes these arguments, a public key or a
 * key pair given arguments its `sign` does not take.
 *
 * @tparam Keypair The first argument's type.
 * @tparam Scheme A scheme tag.
 * @tparam Input The input's type.
 * @tparam Extra The types of the arguments after the input.
 * @param key_pair The first argument.
 * @param signing_scheme The scheme.
 * @param signing_input The input.
 * @param extra_arguments The arguments after the input.
 * @return Never: the call does not compile.
 */
template <class Keypair, class Scheme, class Input, class... Extra>
    requires(detail::is_scheme_v<Scheme> &&
             !detail::has_sign_member<Keypair, Scheme, Input, Extra...>)
[[nodiscard]] boost::system::result<void> sign(
    [[maybe_unused]] Keypair const& key_pair, [[maybe_unused]] Scheme signing_scheme,
    [[maybe_unused]] Input const& signing_input,
    [[maybe_unused]] Extra const&... extra_arguments) noexcept {
    static_assert(detail::refused_v<Keypair>,
                  "webcpp::crypto: sign: the first argument is a key pair, and the others are "
                  "its sign's: crypto::sign(key_pair, scheme, input), and on secp256k1 its 32 "
                  "bytes of extra entropy or auxiliary randomness after the input; a public key "
                  "verifies, with crypto::verify.");
    return {};
}

/**
 * Stops the build: no curve verifies a signature of `Scheme` against this public key.
 *
 * Each curve declares `crypto::verify` for its own keys and schemes; this one is chosen only when
 * none of them is, and says which keys go with which scheme.
 *
 * @tparam Scheme A scheme tag.
 * @tparam Signature The signature's type.
 * @tparam Input The input's type.
 * @tparam PublicKey The public key's type.
 * @param signing_scheme The scheme.
 * @param signature_bytes The signature.
 * @param signing_input The message or the marker.
 * @param public_key The public key.
 * @return Never: the call does not compile.
 */
template <class Scheme, class Signature, class Input, class PublicKey>
    requires(detail::is_scheme_v<Scheme> &&
             !detail::has_verifier_v<Scheme, std::remove_cvref_t<PublicKey>>)
[[nodiscard]] boost::system::result<void> verify(
    [[maybe_unused]] Scheme signing_scheme, [[maybe_unused]] Signature const& signature_bytes,
    [[maybe_unused]] Input const& signing_input,
    [[maybe_unused]] PublicKey const& public_key) noexcept {
    static_assert(detail::refused_v<Scheme>,
                  "webcpp::crypto: verify: this public key does not go with this scheme: "
                  "crypto::eddsa takes a curves::ed25519::public_key, crypto::ecdsa_sha256 a "
                  "curves::p256::basic_public_key or a curves::secp256k1::public_key, "
                  "crypto::schnorr a curves::secp256k1::public_key or a 32-byte x-only key; a "
                  "crypto::ecdsa_sha256_recoverable signature gives its key to "
                  "curves::secp256k1::recover.");
    return {};
}

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_SIGNING_HPP
