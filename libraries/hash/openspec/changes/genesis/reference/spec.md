---
title: "webcpp::hash: Specification"
status: accepted
created: 2026-11-14
decided: 2026-11-14
---

# webcpp::hash: Specification

The owner decided this library's shape on 2026-11-14 ("The owner's decisions (2026-11-14)" of
the research behind webcpp::crypto's roadmap, quoted in section 1.4) and accepted this
specification the same day. `plan.md` beside it is the implementation plan; `../design.md`
records each decision with the alternatives rejected.

Section numbers are cited by the plan as "spec N.M".

## 1. Purpose and scope

### 1.1 What the library is

webcpp::hash is one entry point for hashing and key derivation in a C++ program on every target
webcpp has: native (Linux x86-64 and arm64, macOS arm64, Windows with MSVC), wasip2, wasip3 and
Emscripten. It is a **wrapper** (proposals' `CONTRIBUTING.md`, "Kinds of library"): webcpp's
interface over code other projects wrote, with webcpp's own code only where section 1.3 says.

| What | Where it comes from |
| --- | --- |
| MD5, SHA-1, SHA-2, SHA-3, SHAKE, RIPEMD, BLAKE2b/s, the non-cryptographic hashes, HMAC | Boost.Hash2 1.92, re-exported by alias (spec 2.4, 2.5) |
| Keccak-256 with the original padding (Ethereum's) | XKCP's Keccak-p[1600] permutation, in Hash2's HashAlgorithm form (spec 2.6) |
| BLAKE3, keyed BLAKE3, BLAKE3's key derivation and its extendable output | the official BLAKE3 C implementation, in Hash2's HashAlgorithm form (spec 2.7) |
| HKDF (RFC 5869: extract, expand, extract-and-expand) | a backend: `backends::hash2`, `backends::sodium`, `backends::bearssl`, `backends::openssl` (spec 2.8, 3) |
| PBKDF2 (RFC 8018) | a backend: `backends::hash2`, `backends::openssl` (spec 2.9, 3) |
| Argon2id (RFC 9106, parallelism 1) | libsodium's `crypto_pwhash_argon2id` (spec 2.10) |
| scrypt (RFC 7914) | libsodium's `crypto_pwhash_scryptsalsa208sha256_ll` (spec 2.11) |
| From increment 2 on: everything else libsodium, XKCP, OpenSSL, BearSSL and Botan 3 offer for hashing and key derivation | each library, through the families of spec 2.15, by the capability table of 3.5 |
| From increment 7: SHA-1/256/384/512, HMAC, HKDF and PBKDF2 in the browser | WebCrypto, through the parallel asynchronous family of section 11 |

Every function is synchronous, reports failure as a value (`boost::system::result`), takes its
data as `std::span`, and allocates nothing in webcpp's code (section 6 names the C libraries
that allocate inside the two memory-hard functions and inside OpenSSL).

### 1.2 What it is not

- Not a new implementation of a primitive. Every permutation, compression function and hash is
  Boost.Hash2's or a C library's. Section 1.3 lists the constructions webcpp writes over them.
- Not webcpp::crypto. Encryption, signatures, randomness and constant-time comparison stay
  there; `webcpp::crypto::equal` is what compares a MAC (`digest<N>::operator==` is `memcmp`).
- Not asynchronous in its synchronous functions: WebCrypto, which is promise-based, is reached
  only by the parallel asynchronous family of section 11; the synchronous functions run the same
  C and C++ on every target. WebCrypto is also an outside oracle in the tests (spec 7.4).
- Not, in any increment: a primitive written by webcpp (1.3 lists the two constructions it
  writes), a password-hash string format (`$argon2id$...`) or a helper that compares stored
  hashes, and anything of a backend that is not hashing or key derivation (its ciphers,
  signatures, key exchange, randomness).

### 1.3 The code webcpp writes, and its conditions

The superproject's rule is that webcpp writes no cryptography of its own (CODING-GUIDELINES.md
chapter 4, "What a library builds on", and chapter 5). The owner recorded two departures for
this library on 2026-11-14, both constructions over an audited primitive, never a primitive:

1. **`backends::hash2`: HKDF and PBKDF2 written by webcpp over Boost.Hash2's `hmac<H>`,** on
   every target and for every Hash2 hmac. Conditions, each a test or a review item of the plan:
   the code is the RFC's text, step by step, under 150 lines for both functions, with the RFC's
   step names in comments; RFC 5869's seven cases, RFC 6070's six and RFC 7914's two PBKDF2
   cases pass; it agrees with every other backend that has the same function on random inputs
   on every target where one exists (natively OpenSSL, libsodium and BearSSL; on WebAssembly
   libsodium and BearSSL, and WebCrypto on Emscripten); every intermediate value is wiped; it
   never compares a secret.
2. **`keccak_256`'s sponge over XKCP's permutation.** XKCP does the Keccak-p[1600] permutation;
   webcpp's code does the absorbing, the padding (`0x01 ... 0x80`) and the squeezing, line for
   line as Boost.Hash2's `detail::keccak_base<0x01, 512, 256>` does, so that every sequence of
   `update` and `result` gives Hash2's own results (spec 2.6). Conditions: Hash2's
   `detail::keccak_base<0x01, 512, 256>` is an oracle in a test on random call sequences; the
   type becomes an alias of Boost.Hash2's public `keccak_256` once Boost publishes one (spec 9.1).

### 1.4 The owner's decisions, as recorded on 2026-11-14 and 2026-11-16

- "A new library, webcpp::hash, proposed by the owner: one entry point for hashing and key
  derivation." Boost.Hash2's algorithms and hmac re-exported by alias, no wrapper; Keccak-256
  over XKCP while a public keccak_256 is proposed upstream; BLAKE3 over its official C
  implementation; HKDF and PBKDF2 generic over any Hash2 hmac (the departure of 1.3.1); Argon2id
  and scrypt from libsodium; webcpp::crypto depends on webcpp::hash for HKDF; libsodium is one
  `/third_party//sodium` shared by both.
- "webcpp::hash's KDFs have backends, like webcpp::crypto": `backends::hash2`,
  `backends::sodium` (HKDF SHA-256/512), `backends::bearssl` (HKDF), `backends::openssl` (both;
  native, wasip2, wasip3), later `backends::botan`; a backend without the function refuses at
  compile time. Cross-backend agreement natively on all of them, on WebAssembly hash2 against
  sodium and bearssl, WebCrypto on Emscripten as an outside oracle.
- "Default backend rule": natively the fastest linked (OpenSSL when linked, else hash2); on
  WebAssembly the one that gives the smallest binary, decided by a measured size table (one KDF
  program per backend per target), regenerated with every pin move; this proposal carries that
  measurement and the SHA-512 PBKDF2 throughput at 1,000,000 iterations of hash2, OpenSSL and
  libsodium.

- The widening of 2026-11-14 (the same day, accepted): every backend exposes everything its
  library offers that belongs to hashing and key derivation, as far as it can, in increments,
  the highest value first, with a capability table that says which backend provides each
  function and a compile-time refusal naming the backends that have it; one setting in
  `config.hpp` changes every default backend at once (a profile: fastest, smallest, portable, or
  a named backend), with per-algorithm overrides, and the defaults come from the build (target,
  linked `HAS` macros) and a committed table of webcpp's own measurements (size per target,
  throughput natively), regenerated at every pin move; a browser backend on Emscripten over
  WebCrypto, which the owner then decided (O1) to reach through a parallel asynchronous API
  (section 11), and WebCrypto as an oracle of every algorithm it has stays in any case; the documentation follows noble's README (one short
  section per algorithm: what it is, an example, its pitfalls) and libsodium's documentation
  (ISC, adapted with attribution), with teaching chapters by use case.
- The defaults of 2026-11-16 (design D33), which replace the "Default backend rule" and the
  profile of the widening above: the default backend of every key derivation (`hkdf`,
  `hkdf_extract`, `hkdf_expand`, `pbkdf2`), on every target and whatever the program links, is
  `backends::hash2`; `backends::sodium` is the default only for what hash2 does not offer,
  Argon2id and scrypt, of which it is the only backend. `backends::bearssl`, `backends::openssl`
  and libsodium's HKDF stay, tested as before, as explicit choices named in the type, never a
  default. Every profile, hash's reading of webcpp's umbrella profile, the per-algorithm
  overrides, `detail/ranking.hpp` and the lane `measure` are retired; the measured table stays as
  a reference dated 2026-11-16 that decides nothing. Why hash2: it is the only backend covering
  every hmac, operation and target; it is header-only, so a program carries only what it calls
  and shares the SHA code it already uses; and the measured differences for HKDF are nanoseconds
  natively and a few KB isolated on WebAssembly.

### 1.5 The increments

Each increment is usable alone, merged and recorded before the next; the plan's tasks are
grouped by them and, inside each, into streams by backend (plan, "The order of the work").

| Increment | What it adds | Backends | Why this rank |
| --- | --- | --- | --- |
| 1 | Hash2 by alias; `keccak_256`; `blake3`; HKDF, PBKDF2; Argon2id, scrypt; the default backend and the measured table, a dated reference (section 4); the oracle | hash2, xkcp, blake3, sodium, bearssl, openssl | what Ethereum, Bitcoin (BIP39), Nostr and every key hierarchy need now |
| 2 | libsodium's whole hashing surface: generic hashing (BLAKE2b with key, salt, personalisation and variable length), short hashing (SipHash-2-4, SipHash-2-4x), subkey derivation (`crypto_kdf`, BLAKE2b), Argon2i, the XOFs (SHAKE128/256, TurboSHAKE128/256), SHA-256, SHA-512, SHA3-256, SHA3-512 and HMAC-SHA-256/512/512-256 as digests | sodium | Zcash-family and WireGuard-style formats, hash tables under attack, libsodium interoperability; one library already linked |
| 3 | XKCP's Keccak family: Keccak-224/384/512, SHA-3 and SHAKE as XKCP computes them, cSHAKE, KMAC (and its XOF), TupleHash, ParallelHash (SP 800-185), KangarooTwelve (KT128, KT256), TurboSHAKE128/256 | xkcp | the Keccak team's reference for everything above Keccak-p; one library already linked |
| 4 | OpenSSL's whole hashing and KDF surface: every EVP digest of the default provider (SHA-1/2/3, SHAKE, KECCAK-224 to -512, SM3, BLAKE2b-512, BLAKE2s-256, RIPEMD-160, MD5), KMAC, BLAKE2 MACs, scrypt, Argon2d/i/id with every RFC 9106 parameter (lanes, secret, associated data), KBKDF (SP 800-108), SSKDF (SP 800-56C), X9.63 KDF | openssl | full Argon2 on servers; KDFs of standards (ECIES, TLS-adjacent formats); native, wasip2, wasip3 |
| 5 | BearSSL's hashes and HMAC: MD5, SHA-1, SHA-224/256/384/512, MD5-SHA1, SHAKE | bearssl | the smallest WebAssembly programs, where measured so |
| 6 | Botan 3's: PBKDF2, Argon2d/i/id with any parameters, bcrypt and bcrypt-pbkdf, scrypt, HKDF, BLAKE2b variable, Keccak, SHAKE, cSHAKE, KMAC, Skein-512, SM3, Streebog, Whirlpool | botan | the long tail; waits for webcpp::crypto's Botan provisioning (its increment 5: native, exceptions only) |
| 7 | the browser backend over WebCrypto (SHA-1/256/384/512, HMAC, HKDF, PBKDF2), reached through the parallel asynchronous family (`async_digest`, `async_hmac`, `async_hkdf`, `async_pbkdf2`) | webcrypto | the owner's answer to O1 (section 11): the smallest Emscripten programs and JavaScript's own behaviour |

## 2. Interface

### 2.1 Names

| What | Name |
| --- | --- |
| namespace | `webcpp::hash`, `webcpp::hash::backends`, `webcpp::hash::kdfs`; `webcpp::hash::detail` for what is not public |
| headers | `<webcpp/hash/...>`, one per responsibility; the convenience header `<webcpp/hash.hpp>` |
| configuration | `<webcpp/hash/config.hpp>`, first in every public header; it includes `<webcpp/config.hpp>` first |
| macros | `WEBCPP_HASH_*`; test macros `WEBCPP_TEST_*` |
| include guards | `WEBCPP_HASH_<PATH>_HPP`; `WEBCPP_HASH_DETAIL_CANARY_H` for the C header |
| b2 targets | `/webcpp/hash//hash` (the library), `/webcpp/hash//openssl` (OpenSSL, spec 5.6) |
| C libraries | `/third_party//xkcp`, `/third_party//blake3`, `/third_party//sodium`, `/third_party//bearssl` |
| error category | `webcpp.hash` |
| repository | `webcpporg/hash`, submodule `libs/hash` (the owner creates both) |

Every public header includes `<webcpp/hash/config.hpp>` first, then what it uses. A header whose
functions call a C library needs that library linked (spec 5.4); the others need nothing but
Boost.

Every function of `include/` that returns `boost::system::result` is `[[nodiscard]]`:
`boost::system::result` carries no `[[nodiscard]]` of its own, and a dropped result, a refusal
at run time, would pass as a success and leave the output unwritten. `libs/hash/meta/lint.json`
opts the library into the lint's rule "nodiscard results" with that reason (design D30).

### 2.2 `<webcpp/hash/config.hpp>`

```cpp
#ifndef WEBCPP_HASH_CONFIG_HPP
#define WEBCPP_HASH_CONFIG_HPP

#include <webcpp/config.hpp>

#ifdef __MRDOCS__
// One definition per macro the reference lists, each undone at once (chapter 6).
#define WEBCPP_HASH_NO_EXCEPTIONS
#undef WEBCPP_HASH_NO_EXCEPTIONS
#define WEBCPP_HASH_WEBASSEMBLY 1
#undef WEBCPP_HASH_WEBASSEMBLY
#define WEBCPP_HASH_HAS_XKCP 1
#undef WEBCPP_HASH_HAS_XKCP
#define WEBCPP_HASH_HAS_BLAKE3 1
#undef WEBCPP_HASH_HAS_BLAKE3
#define WEBCPP_HASH_HAS_SODIUM 1
#undef WEBCPP_HASH_HAS_SODIUM
#define WEBCPP_HASH_HAS_BEARSSL 1
#undef WEBCPP_HASH_HAS_BEARSSL
#define WEBCPP_HASH_HAS_OPENSSL 1
#undef WEBCPP_HASH_HAS_OPENSSL
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_HASH_NO_EXCEPTIONS)
#define WEBCPP_HASH_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3) || \
    defined(WEBCPP_TARGET_EMSCRIPTEN)
#define WEBCPP_HASH_WEBASSEMBLY 1
#endif

// The refusals of impossible combinations (spec 5.7).
// clang-format off
#if defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(WEBCPP_HASH_HAS_OPENSSL)
#error "webcpp::hash: OpenSSL is not built for Emscripten. In the program's Jamfile, remove /webcpp/hash//openssl: hkdf and pbkdf2 then run on backends::hash2."
#endif
// No profile and no override define is read (spec 4.1, design D33).
// clang-format on

namespace webcpp::hash::backends {

/** webcpp's own HKDF and PBKDF2 over Boost.Hash2's `hmac<H>`, on every target. */
struct hash2;
/** libsodium, built from its sources by `/third_party//sodium`. */
struct sodium;
/** BearSSL, built from its sources by `/third_party//bearssl`. */
struct bearssl;
/** OpenSSL 3.5, built from its pinned sources, named by `/webcpp/hash//openssl`. */
struct openssl;

}  // namespace webcpp::hash::backends

namespace webcpp::hash::kdfs {

/** HKDF, RFC 5869: the operation `hkdf`. */
struct hkdf;
/** HKDF-Extract alone, the operation `hkdf_extract` (RFC 5869, section 2.2). */
struct hkdf_extract;
/** HKDF-Expand alone, the operation `hkdf_expand` (RFC 5869, section 2.3). */
struct hkdf_expand;
/** PBKDF2, RFC 8018: `pbkdf2`. */
struct pbkdf2;
/** Argon2id, RFC 9106, with one lane: `argon2id`. */
struct argon2id;
/** scrypt, RFC 7914: `scrypt`. */
struct scrypt;

}  // namespace webcpp::hash::kdfs

#endif  // WEBCPP_HASH_CONFIG_HPP
```

`WEBCPP_HASH_NO_EXCEPTIONS` exists as every library's does (chapter 6); webcpp::hash raises no
exception of its own, and its Doc Comment says so. `config_test.cpp` is built three times
(`config`, `config_all_off`, `config_developer_defined`).

### 2.3 `<webcpp/hash/error.hpp>`

```cpp
namespace webcpp::hash {

/** Why an operation of webcpp::hash failed; the values are fixed and never reused. */
enum class error : int {
    /** An output of a length the function does not produce: empty, or longer than its limit. */
    invalid_output_size = 1,
    /** A pseudorandom key given to `hkdf_expand` that is not of the hmac's digest size. */
    invalid_key_size = 2,
    /** A salt of a size the function refuses: Argon2id's is of 16 bytes. */
    invalid_salt_size = 3,
    /** An iteration count of 0 given to `pbkdf2`. */
    invalid_iterations = 4,
    /** A cost of a memory-hard function outside its limits. */
    invalid_parameters = 5,
    /** An input longer than every backend accepts: HKDF's info past 32,768 bytes, a password past the function's limit. */
    input_too_long = 6,
    /** A backend failed where its contract says it does not. */
    backend_failure = 7,
    /** A parameter the chosen backend does not take (Argon2 lanes, a secret or associated data on libsodium, from increment 4 on). */
    unsupported_by_backend = 8,
};

/** The category of every `error`, named "webcpp.hash". */
const boost::system::error_category& category() noexcept;

/** Makes the `error_code` of `e`. */
boost::system::error_code make_error_code(error e) noexcept;

}  // namespace webcpp::hash

template <>
struct boost::system::is_error_code_enum<webcpp::hash::error> : std::true_type {};
```

The category has webcpp::crypto's (and pratt's) form: a `final` class with a constexpr
constructor giving a fixed identity (`0x6861736800000000`, the ASCII of "hash" and four zero
bytes), `name()`, both `message` overloads, and one `inline constexpr` instance in `detail`.
The messages:

| Value | Message |
| --- | --- |
| `invalid_output_size` | "the output is empty or longer than the function produces" |
| `invalid_key_size` | "the pseudorandom key is not of the hmac's digest size" |
| `invalid_salt_size` | "the salt is not of a size the function accepts" |
| `invalid_iterations` | "the iteration count is zero" |
| `invalid_parameters` | "a cost is outside the function's limits" |
| `input_too_long` | "an input is longer than the function accepts" |
| `backend_failure` | "the backend failed where it should not" |
| `unsupported_by_backend` | "the backend does not take this parameter; name a backend that does" |

A failed allocation inside a C library is `boost::system::errc::not_enough_memory` in the
generic category, as webcpp::crypto reports it.

### 2.4 `<webcpp/hash/algorithms.hpp>`: Boost.Hash2's algorithms

Every algorithm Boost.Hash2 1.92 has, by alias, so that a type of webcpp::hash is the Hash2 type
itself: `hash_append`, `get_integral_result` and every other part of Hash2 take it unchanged.

```cpp
#include <boost/hash2/blake2.hpp>
#include <boost/hash2/digest.hpp>
#include <boost/hash2/fnv1a.hpp>
#include <boost/hash2/hash_append.hpp>
#include <boost/hash2/legacy/murmur3.hpp>
#include <boost/hash2/legacy/spooky2.hpp>
#include <boost/hash2/md5.hpp>
#include <boost/hash2/ripemd.hpp>
#include <boost/hash2/sha1.hpp>
#include <boost/hash2/sha2.hpp>
#include <boost/hash2/sha3.hpp>
#include <boost/hash2/siphash.hpp>
#include <boost/hash2/xxh3.hpp>
#include <boost/hash2/xxhash.hpp>

namespace webcpp::hash {

/** A digest of `N` bytes, Boost.Hash2's. @tparam N The size in bytes. */
template <std::size_t N>
using digest = boost::hash2::digest<N>;

using md5_128 = boost::hash2::md5_128;
using sha1_160 = boost::hash2::sha1_160;
using sha2_224 = boost::hash2::sha2_224;
using sha2_256 = boost::hash2::sha2_256;
using sha2_384 = boost::hash2::sha2_384;
using sha2_512 = boost::hash2::sha2_512;
using sha2_512_224 = boost::hash2::sha2_512_224;
using sha2_512_256 = boost::hash2::sha2_512_256;
using sha3_224 = boost::hash2::sha3_224;
using sha3_256 = boost::hash2::sha3_256;
using sha3_384 = boost::hash2::sha3_384;
using sha3_512 = boost::hash2::sha3_512;
using shake_128 = boost::hash2::shake_128;
using shake_256 = boost::hash2::shake_256;
using ripemd_128 = boost::hash2::ripemd_128;
using ripemd_160 = boost::hash2::ripemd_160;
using blake2b_512 = boost::hash2::blake2b_512;
using blake2s_256 = boost::hash2::blake2s_256;

// Not cryptographic: for hash tables, checksums and identifiers.
using fnv1a_32 = boost::hash2::fnv1a_32;
using fnv1a_64 = boost::hash2::fnv1a_64;
using siphash_32 = boost::hash2::siphash_32;
using siphash_64 = boost::hash2::siphash_64;
using xxhash_32 = boost::hash2::xxhash_32;
using xxhash_64 = boost::hash2::xxhash_64;
using xxh3_128 = boost::hash2::xxh3_128;
using murmur3_32 = boost::hash2::murmur3_32;
using murmur3_128 = boost::hash2::murmur3_128;
using spooky2_128 = boost::hash2::spooky2_128;

using boost::hash2::hash_append;
using boost::hash2::hash_append_range;
using boost::hash2::hash_append_range_and_size;
using boost::hash2::hash_append_size;
using boost::hash2::hash_append_unordered_range;
using boost::hash2::get_integral_result;

}  // namespace webcpp::hash
```

Each alias has its Doc Comment (chapter 7), naming the algorithm, its standard and, for the
legacy and non-cryptographic ones, that they are not for security. The plan's first test proves
`std::is_same_v<webcpp::hash::sha2_256, boost::hash2::sha2_256>` for each, and that
`hash_append` of a `std::string` through `webcpp::hash::sha2_256` gives Hash2's digest.

### 2.5 `<webcpp/hash/hmac.hpp>`: Boost.Hash2's HMAC

```cpp
namespace webcpp::hash {

/** HMAC over `H`, RFC 2104, Boost.Hash2's. @tparam H A Hash2 HashAlgorithm. */
template <class H>
using hmac = boost::hash2::hmac<H>;

using hmac_md5_128 = boost::hash2::hmac_md5_128;
using hmac_sha1_160 = boost::hash2::hmac_sha1_160;
using hmac_sha2_224 = boost::hash2::hmac_sha2_224;
using hmac_sha2_256 = boost::hash2::hmac_sha2_256;
using hmac_sha2_384 = boost::hash2::hmac_sha2_384;
using hmac_sha2_512 = boost::hash2::hmac_sha2_512;
using hmac_sha2_512_224 = boost::hash2::hmac_sha2_512_224;
using hmac_sha2_512_256 = boost::hash2::hmac_sha2_512_256;
using hmac_sha3_224 = boost::hash2::hmac_sha3_224;
using hmac_sha3_256 = boost::hash2::hmac_sha3_256;
using hmac_sha3_384 = boost::hash2::hmac_sha3_384;
using hmac_sha3_512 = boost::hash2::hmac_sha3_512;
using hmac_ripemd_128 = boost::hash2::hmac_ripemd_128;
using hmac_ripemd_160 = boost::hash2::hmac_ripemd_160;
using hmac_blake2b_512 = boost::hash2::hmac_blake2b_512;
using hmac_blake2s_256 = boost::hash2::hmac_blake2s_256;

}  // namespace webcpp::hash
```

A program checks a MAC with `webcpp::crypto::equal`, never with `digest<N>::operator==`; the
page says so with an example.

### 2.6 `<webcpp/hash/keccak.hpp>`: Keccak-256

Keccak-256 as the Keccak team submitted it to the SHA-3 competition and as Ethereum uses it:
rate 1088 bits, capacity 512, the padding byte `0x01` (SHA3-256 differs only in its `0x06`).

```cpp
namespace webcpp::hash {

/**
 Keccak-256 with the original padding, Ethereum's hash, in Boost.Hash2's HashAlgorithm form.

 The permutation is XKCP's Keccak-p[1600]; the sponge follows Boost.Hash2's
 `detail::keccak_base<0x01, 512, 256>` exactly, so that every sequence of calls gives its
 results.
*/
class keccak_256 {
public:
    /** The digest, 32 bytes. */
    using result_type = boost::hash2::digest<32>;
    /** The rate, 136 bytes. */
    static constexpr std::size_t block_size = 136;

    /** Constructs the hash of nothing yet. */
    keccak_256() noexcept;
    /** Constructs a hash seeded by `seed`, as Hash2's sha3_256 is: nothing when it is zero. @param seed The seed. */
    explicit keccak_256(std::uint64_t seed) noexcept;
    /** Constructs a hash seeded by the `n` bytes at `p`, as Hash2's sha3_256 is. @param p The seed. @param n Its size. */
    keccak_256(void const* p, std::size_t n) noexcept;
    /** The same, from bytes. @param p The seed. @param n Its size. */
    keccak_256(unsigned char const* p, std::size_t n) noexcept;

    /** Absorbs `n` bytes. @param p The bytes. @param n Their number. */
    void update(void const* p, std::size_t n) noexcept;
    /** The same, from bytes. @param p The bytes. @param n Their number. */
    void update(unsigned char const* p, std::size_t n) noexcept;

    /**
     Pads, if nothing was padded since the last update, permutes and returns the first 32 bytes
     of the state; a further call returns the next permutation's.

     @return The digest.
    */
    result_type result() noexcept;

private:
    std::array<std::uint64_t, 25> state_;  // Keccak-p[1600]'s state, in XKCP's plain64 layout
    std::size_t m_ = 0;                    // bytes absorbed into the current block
    bool finalized_ = false;               // the padding is in the state
};

}  // namespace webcpp::hash
```

The functions are defined inline, and every one that calls XKCP reads the canary (spec 5.4).
XKCP is reached through four C functions of webcpp's own adapter, `libs/hash/src/xkcp_adapter.c`,
which `/third_party//xkcp` compiles beside XKCP's file, so that no XKCP header, and with it
XKCP's generic `config.h`, reaches a user's translation unit:

```c
void webcpp_hash_xkcp_initialize(uint64_t *state);
void webcpp_hash_xkcp_add_bytes(uint64_t *state, const unsigned char *data, size_t offset, size_t length);
void webcpp_hash_xkcp_permute(uint64_t *state);
void webcpp_hash_xkcp_extract_bytes(const uint64_t *state, unsigned char *data, size_t offset, size_t length);
```

They call `KeccakP1600_Initialize`, `KeccakP1600_AddBytes`, `KeccakP1600_Permute_24rounds` and
`KeccakP1600_ExtractBytes` of `KeccakP-1600-SnP.h`, the state cast to XKCP's
`KeccakP1600_plain64_state`, whose layout is `uint64_t A[25]`. The four definitions of
`keccak_256`'s members mirror `boost/hash2/sha3.hpp`'s `keccak_base` (absorb by `add_bytes`,
permute when a block of 136 bytes is full, on `result` add `0x01` at `m_` and `0x80` at byte 135
once, permute, extract 32 bytes), and the seeded constructors mirror `sha3_256`'s (absorb, call
`result()`, then a zero-length update clears `finalized_`).

### 2.7 `<webcpp/hash/blake3.hpp>`: BLAKE3

```cpp
#include <blake3.h>  // third_party/libs/blake3/c, through /third_party//blake3

namespace webcpp::hash {

/**
 BLAKE3, its keyed mode, its key derivation and its extendable output, over the official C
 implementation, in Boost.Hash2's HashAlgorithm form.
*/
class blake3 {
public:
    /** The digest, 32 bytes. */
    using result_type = boost::hash2::digest<32>;
    /** BLAKE3's block, 64 bytes. */
    static constexpr std::size_t block_size = 64;
    /** The key of the keyed mode, 32 bytes. */
    static constexpr std::size_t key_size = 32;

    /** Constructs the plain hash. */
    blake3() noexcept;
    /** Constructs a hash seeded by `seed`: plain when it is zero, else as from its 8 little-endian bytes. @param seed The seed. */
    explicit blake3(std::uint64_t seed) noexcept;
    /**
     Constructs a hash seeded by `n` bytes, as Hash2's blake2b_512 is: plain when `n` is zero;
     else keyed by the first 32 bytes, zero-padded when there are fewer, the bytes past them
     absorbed and finalized.

     @param p The seed. @param n Its size.
    */
    blake3(void const* p, std::size_t n) noexcept;
    /** The same, from bytes. @param p The seed. @param n Its size. */
    blake3(unsigned char const* p, std::size_t n) noexcept;

    /** The keyed hash, BLAKE3's `keyed_hash`. @param key The key. @return The hash. */
    static blake3 keyed(std::span<std::uint8_t const, 32> key) noexcept;
    /** The key derivation, BLAKE3's `derive_key`, under `context`. @param context The context string, any bytes. @return The hash. */
    static blake3 derive_key(std::string_view context) noexcept;

    /** Absorbs `n` bytes. @param p The bytes. @param n Their number. */
    void update(void const* p, std::size_t n) noexcept;
    /** The same, from bytes. @param p The bytes. @param n Their number. */
    void update(unsigned char const* p, std::size_t n) noexcept;

    /**
     Returns the next 32 bytes of the output: the digest the first time after input was added,
     then the following 32 bytes of the extendable output at each call.

     @return 32 bytes of output.
    */
    result_type result() noexcept;

    /**
     Writes `output.size()` bytes of the extendable output, from byte `position` on, without
     changing the hash.

     @param output Where to write. @param position The first byte's position in the output.
    */
    void squeeze(std::span<std::uint8_t> output, std::uint64_t position = 0) const noexcept;

    /** Forgets the input, keeping the mode and its key or context. */
    void reset() noexcept;

private:
    blake3_hasher hasher_;        // the C implementation's state, 1,912 bytes on 64-bit targets
    std::uint64_t squeezed_ = 0;  // bytes result() returned since input was last added
};

}  // namespace webcpp::hash
```

`result()` calls `blake3_hasher_finalize_seek(&hasher_, squeezed_, ...)` and adds 32;
`update` with `n > 0` calls `blake3_hasher_update` and sets `squeezed_` to 0, so that a result
never repeats for one input history (Hash2's requirement) and the first result is always the
digest of everything absorbed. `derive_key` calls `blake3_hasher_init_derive_key_raw`, so a
context needs no terminating zero. The type is copyable, as Hash2 requires.

### 2.8 `<webcpp/hash/kdf.hpp>`, `<webcpp/hash/hkdf.hpp>`: HKDF

`kdf.hpp` holds what HKDF and PBKDF2 share:

```cpp
namespace webcpp::hash {

/**
 Whether `Backend` has the key derivation `Kdf` over `Hmac`: a property of the library behind
 it (spec 3), whether or not the program links it.

 @tparam Backend A backend of `backends`. @tparam Kdf A tag of `kdfs`: `hkdf`, `hkdf_extract`,
 `hkdf_expand`, `pbkdf2`, `argon2id` or `scrypt`. @tparam Hmac A Boost.Hash2 hmac, or `void`.
*/
template <class Backend, class Kdf, class Hmac>
inline constexpr bool backend_has_v = /* the table of spec 3.2 */;

/**
 The backend `hkdf_extract`, `hkdf_expand`, `hkdf` and `pbkdf2` use when the program names none:
 `backends::hash2`, for every `Kdf` and `Hmac`, on every target and whatever the program links
 (spec 4.1, design D33); `default_backend<Kdf, Hmac>`. Another backend is named in the call's
 type.

 @tparam Hmac A Boost.Hash2 hmac. @tparam Kdf `kdfs::hkdf` (the default), `kdfs::hkdf_extract`,
 `kdfs::hkdf_expand` or `kdfs::pbkdf2`.
*/
template <class Hmac, class Kdf = kdfs::hkdf>
using default_kdf_backend = /* spec 4 */;

}  // namespace webcpp::hash
```

`hkdf.hpp`:

```cpp
namespace webcpp::hash {

/** The longest info every backend accepts, 32,768 bytes (OpenSSL 3.5's limit). */
inline constexpr std::size_t hkdf_max_info_size = 32768;

/**
 HKDF-Extract (RFC 5869, section 2.2): the pseudorandom key of `ikm` under `salt`.

 @tparam Hmac The hmac, as `hmac_sha2_256`. @tparam Backend The backend.
 @param salt The salt, any length; empty means `Hmac`'s digest size of zeros.
 @param ikm The input keying material, any length.
 @return The pseudorandom key, of the hmac's digest size.
*/
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_extract>>
[[nodiscard]] boost::system::result<typename Hmac::result_type> hkdf_extract(
    std::span<std::uint8_t const> salt, std::span<std::uint8_t const> ikm);

/**
 HKDF-Expand (RFC 5869, section 2.3): fills `output` from `prk` and `info`.

 @tparam Hmac The hmac. @tparam Backend The backend.
 @param output The output keying material, 1 to 255 times the digest size.
 @param prk The pseudorandom key, of exactly the digest size.
 @param info The context, at most `hkdf_max_info_size` bytes.
 @return Success, or the error of the first guard that fails; on a backend's failure `output`
 is zeroed.
*/
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_expand>>
[[nodiscard]] boost::system::result<void> hkdf_expand(std::span<std::uint8_t> output,
                                        std::span<std::uint8_t const> prk,
                                        std::span<std::uint8_t const> info);

/**
 HKDF (RFC 5869, section 2): extract, then expand.

 @tparam Hmac The hmac. @tparam Backend The backend.
 @param output The output keying material, 1 to 255 times the digest size.
 @param salt The salt. @param ikm The input keying material. @param info The context.
 @return Success, or the error of the first guard that fails.
*/
template <class Hmac, class Backend = default_kdf_backend<Hmac>>
[[nodiscard]] boost::system::result<void> hkdf(std::span<std::uint8_t> output,
                                 std::span<std::uint8_t const> salt,
                                 std::span<std::uint8_t const> ikm,
                                 std::span<std::uint8_t const> info);

}  // namespace webcpp::hash
```

The guards, in order, before any work and before the backend is called:

| Function | Guard | Error |
| --- | --- | --- |
| `hkdf_expand`, `hkdf` | `output` empty, or longer than 255 times the digest size | `invalid_output_size` |
| `hkdf_expand` | `prk.size()` other than the digest size | `invalid_key_size` |
| `hkdf_expand`, `hkdf` | `info.size() > hkdf_max_info_size` | `input_too_long` |
| all | an input or the output past the backend's `detail::backend_limits` (OpenSSL: the salt and the PRK past `INT_MAX`) | `input_too_long` for an input, `invalid_output_size` for the output |
| all | the backend fails | `backend_failure`, or `errc::not_enough_memory` when OpenSSL cannot allocate; `output` zeroed |

The PRK is of exactly the digest size on every backend, where RFC 5869 allows "at least
HashLen octets", because libsodium takes exactly that and a call must never succeed on one
backend and fail on another (design D9). The salt and the IKM have no limit but `size_t`'s and
the backend's own.

**A backend's narrower lengths (design D26).** A backend whose C interface holds a length in a
type narrower than `std::size_t` declares its limits in `detail::backend_limits<Backend>`
(`detail/engines.hpp`), one member per input and output of each operation (`hkdf_salt`,
`hkdf_ikm`, `hkdf_prk`, `hkdf_info`, `hkdf_output`, `pbkdf2_password`, `pbkdf2_salt`,
`pbkdf2_output`, `argon2id_*`, `scrypt_*`); the primary template, every backend's unless its
header specialises it, has no limit (`detail::no_limit`). The public functions read it after the
spec's own guards and before any call of the backend. `backends::openssl` limits HKDF's salt and
PRK (HMAC's key, an `int` in `HMAC_Init_ex`) and PBKDF2's password, salt and output (`int`s in
`pbkdf2_derive`) to `INT_MAX`: OpenSSL 3.5.9 holds them in an `int`, and a PBKDF2 output of 2^31
bytes crashed it (SIGSEGV, a negative length reaching `memcpy`). `backends::hash2` accepts these
lengths: this is the one place where a call can succeed on one backend and be refused on another,
a refusal at a size no protocol uses, never a different result.

### 2.9 `<webcpp/hash/pbkdf2.hpp>`: PBKDF2

```cpp
namespace webcpp::hash {

/**
 PBKDF2 (RFC 8018, section 5.2): fills `output` from `password` and `salt` by `iterations`
 rounds of `Hmac`.

 @tparam Hmac The pseudorandom function, as `hmac_sha2_512`. @tparam Backend The backend.
 @param output The derived key, 1 to (2^32 - 1) times the digest size.
 @param password The password, any bytes. @param salt The salt, any bytes.
 @param iterations The iteration count, at least 1.
 @return Success, or the error of the first guard that fails.
*/
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::pbkdf2>>
[[nodiscard]] boost::system::result<void> pbkdf2(std::span<std::uint8_t> output,
                                   std::span<std::uint8_t const> password,
                                   std::span<std::uint8_t const> salt,
                                   std::uint64_t iterations);

}  // namespace webcpp::hash
```

Guards: `output` empty or longer than (2^32 - 1) times the digest size, compared through
`detail::exceeds(std::uint64_t, std::uint64_t)` so that no comparison is tautological on wasm32
(`invalid_output_size`); `iterations == 0` (`invalid_iterations`); then the backend's
`detail::backend_limits` (2.8: on OpenSSL the password and the salt past `INT_MAX`,
`input_too_long`, and the output past `INT_MAX`, `invalid_output_size`); a backend's failure zeroes
`output`. OpenSSL is called with `pkcs5 = 1`, which turns off SP 800-132's lower bounds, so that
every RFC 6070 case runs on every backend (design D10).

### 2.10 `<webcpp/hash/argon2.hpp>`: Argon2id

```cpp
namespace webcpp::hash {

/** The costs of Argon2id with one lane, as libsodium's crypto_pwhash takes them. */
struct argon2id_parameters {
    /** The number of passes over the memory, t: 1 to 4,294,967,295. */
    std::uint64_t passes;
    /** The memory in bytes, m times 1,024: at least 8,192, a multiple of 1,024, at most 4 TiB (2 GiB on wasm32). */
    std::size_t memory;
};

/** libsodium's interactive costs: 2 passes over 64 MiB. */
inline constexpr argon2id_parameters argon2id_interactive{2, 67108864};
/** libsodium's moderate costs: 3 passes over 256 MiB. */
inline constexpr argon2id_parameters argon2id_moderate{3, 268435456};
/** libsodium's sensitive costs: 4 passes over 1 GiB. */
inline constexpr argon2id_parameters argon2id_sensitive{4, 1073741824};

/** The salt's size, 16 bytes. */
inline constexpr std::size_t argon2id_salt_size = 16;

/**
 Argon2id version 0x13 (RFC 9106) with one lane, no secret and no associated data: fills
 `output` from `password` and `salt`.

 @tparam Backend The backend; `backends::sodium`, the only one.
 @param output The tag, 16 to 4,294,967,295 bytes.
 @param password The password, at most 4,294,967,295 bytes.
 @param salt The salt, of `argon2id_salt_size` bytes.
 @param parameters The costs.
 @return Success, or the error of the first guard that fails, or
 `errc::not_enough_memory` when the memory cannot be allocated.
*/
template <class Backend = default_backend<kdfs::argon2id>>
[[nodiscard]] boost::system::result<void> argon2id(std::span<std::uint8_t> output,
                                     std::span<std::uint8_t const> password,
                                     std::span<std::uint8_t const> salt,
                                     argon2id_parameters const& parameters);

}  // namespace webcpp::hash
```

Guards in order: `output` shorter than 16 or longer than 4,294,967,295 bytes
(`invalid_output_size`); `salt.size() != 16` (`invalid_salt_size`); a password longer than
4,294,967,295 bytes (`input_too_long`); `passes` outside 1 to 4,294,967,295, `memory` below
8,192, not a multiple of 1,024 (libsodium would silently round it down) or above
`crypto_pwhash_argon2id_MEMLIMIT_MAX` (`invalid_parameters`). The three presets are checked
against libsodium's `OPSLIMIT_*` and `MEMLIMIT_*` macros by `static_assert` in
`backends/sodium.hpp`. libsodium allocates the memory (spec 6.2).

### 2.11 `<webcpp/hash/scrypt.hpp>`: scrypt

```cpp
namespace webcpp::hash {

/** The costs of scrypt, RFC 7914's N, r and p. */
struct scrypt_parameters {
    /** The CPU and memory cost N: a power of two, greater than 1, less than 2^(16 r). */
    std::uint64_t n;
    /** The block size r, at least 1. */
    std::uint32_t r;
    /** The parallelisation p, at least 1, with r times p below 2^30. */
    std::uint32_t p;
};

/**
 scrypt (RFC 7914): fills `output` from `password` and `salt`.

 @tparam Backend The backend; `backends::sodium`, the only one.
 @param output The derived key, 1 to (2^32 - 1) times 32 bytes.
 @param password The password. @param salt The salt.
 @param parameters The costs.
 @return Success, or the error of the first guard that fails, or
 `errc::not_enough_memory` when the 128 r N bytes cannot be allocated.
*/
template <class Backend = default_backend<kdfs::scrypt>>
[[nodiscard]] boost::system::result<void> scrypt(std::span<std::uint8_t> output,
                                   std::span<std::uint8_t const> password,
                                   std::span<std::uint8_t const> salt,
                                   scrypt_parameters const& parameters);

}  // namespace webcpp::hash
```

Guards: `output` empty or past the limit (`invalid_output_size`); RFC 7914's conditions on N, r
and p, and 128 r (N + p) bytes that do not fit `size_t` (`invalid_parameters`). The call is
`crypto_pwhash_scryptsalsa208sha256_ll(password, salt, N, r, p, output)`.

### 2.12 The backends' headers

| Header | Declares | Needs |
| --- | --- | --- |
| `<webcpp/hash/backends/hash2.hpp>` | `backends::hash2` and its HKDF and PBKDF2 engines | nothing |
| `<webcpp/hash/backends/sodium.hpp>` | `backends::sodium`: HKDF engines for `hmac_sha2_256` and `hmac_sha2_512`, the Argon2id and scrypt engines, the presets' checks | `/third_party//sodium` |
| `<webcpp/hash/backends/bearssl.hpp>` | `backends::bearssl`: the `hkdf` engine for six hmacs | `/third_party//bearssl` |
| `<webcpp/hash/backends/openssl.hpp>` | `backends::openssl`: HKDF and PBKDF2 engines | `/webcpp/hash//openssl` |

A backend's header that needs a library checks its `WEBCPP_HASH_HAS_<NAME>` first and stops with
the Jamfile line (spec 5.7). Each engine is a specialisation, in `webcpp::hash::detail`, of

```cpp
template <class Backend, class Hmac> struct hkdf_engine;    // extract, expand, extract_and_expand
template <class Backend, class Hmac> struct pbkdf2_engine;  // derive
template <class Backend> struct argon2id_engine;            // derive, limits
template <class Backend> struct scrypt_engine;              // derive
```

whose primary templates hold the `static_assert` that refuses a backend without the function
(spec 3.3). The public functions do every guard of 2.8 to 2.11 once; an engine does only the
work, and is called only with valid arguments.

### 2.13 The convenience header

`<webcpp/hash.hpp>` includes `config.hpp`, `error.hpp`, `algorithms.hpp`, `hmac.hpp`, `kdf.hpp`,
`hkdf.hpp`, `pbkdf2.hpp` and `backends/hash2.hpp`, which need no C library, and `keccak.hpp`,
`blake3.hpp`, `argon2.hpp`, `scrypt.hpp` and the backends' headers only when their
`WEBCPP_HASH_HAS_<NAME>` is defined. It names in its comment what it leaves out and why
(CODING-GUIDELINES.md chapter 3). A program that links no C library includes it and compiles.

### 2.14 What is not public

`detail/canary.h` (spec 5.4), `detail/wipe.hpp` (stores of zero through `volatile` over a span
and over a trivially copyable object), `detail/limits.hpp` (`exceeds`), `detail/engines.hpp`
(the primary templates of 2.12), `detail/xkcp.hpp` (the adapter's declarations). Until
2026-11-16 it also held the generated `detail/ranking.hpp` and `detail/overrides.hpp`, retired by
design D33.

### 2.15 The families of increments 2 to 7

Increment 1's functions keep their form. From increment 2 on, an algorithm whose result does not
fit Hash2's HashAlgorithm contract (a single finalisation, parameters, a key) is reached through
four families, each a template over the algorithm's tag and the backend, with the same guards
in one place and the same engines-per-backend design (2.12):

```cpp
namespace webcpp::hash {

namespace digests {  // the tag of each hash and XOF: sha2_256, sha3_256, keccak_224 .. keccak_512,
                     // shake_128, shake_256, turboshake_128, turboshake_256, cshake_128, cshake_256,
                     // kt128, kt256, tuplehash_128, tuplehash_256, parallelhash_128, parallelhash_256,
                     // blake2b, blake2s_256, md5, md5_sha1, sha1, sha2_224, sha2_384, sha2_512,
                     // sha2_512_224, sha2_512_256, sha3_224, sha3_384, sha3_512, ripemd_160, sm3,
                     // streebog_256, streebog_512, whirlpool, skein_512
}
namespace macs {     // hmac<Digest>, kmac_128, kmac_256, siphash_2_4, siphash_2_4x, blake2b_mac, blake2s_mac
}
namespace kdfs {     // hkdf, pbkdf2 (increment 1), scrypt, argon2d, argon2i, argon2id, bcrypt_pbkdf,
                     // bcrypt, blake2b_subkey, kbkdf_hmac, sskdf, x963kdf
}

/** The parameters of a digest: empty for most; key, salt, personalisation for blake2b; function name and customisation for cshake and the SP 800-185 functions; customisation for kt128 and kt256; the domain byte for turboshake. @tparam Digest A tag of `digests`. */
template <class Digest> struct digest_options;

/** Hashes `input` into `output`, whose size is the digest's (or any size, for an XOF). */
template <class Digest, class Backend = default_backend<Digest>>
[[nodiscard]] boost::system::result<void> digest(std::span<std::uint8_t> output, std::span<std::uint8_t const> input,
                                   digest_options<Digest> const& options = {});

/** An incremental hash with one finalisation: `make`, `update`, `finish`. Move-only; wiped. */
template <class Digest, class Backend = default_backend<Digest>>
class hasher {
public:
    [[nodiscard]] static boost::system::result<hasher> make(digest_options<Digest> const& options = {});
    [[nodiscard]] boost::system::result<void> update(std::span<std::uint8_t const> input);
    [[nodiscard]] boost::system::result<void> finish(std::span<std::uint8_t> output);
};

/** The MAC of `input` under `key`; for KMAC, its customisation and its XOF form in the options. */
template <class Mac, class Backend = default_backend<Mac>>
[[nodiscard]] boost::system::result<void> mac(std::span<std::uint8_t> output, std::span<std::uint8_t const> key,
                                std::span<std::uint8_t const> input, mac_options<Mac> const& options = {});

/** Argon2d, Argon2i or Argon2id with every RFC 9106 parameter (increment 4 on). */
struct argon2_parameters {
    std::uint32_t passes;
    std::uint32_t memory_kib;
    std::uint32_t lanes = 1;
    std::span<std::uint8_t const> secret = {};
    std::span<std::uint8_t const> associated_data = {};
};
template <class Variant, class Backend = default_backend<Variant>>
[[nodiscard]] boost::system::result<void> argon2(std::span<std::uint8_t> output, std::span<std::uint8_t const> password,
                                   std::span<std::uint8_t const> salt, argon2_parameters const& parameters);

}  // namespace webcpp::hash
```

Increment 2 adds to `config.hpp` the tags `backends::xkcp` and `backends::blake3` (the libraries
behind `keccak_256` and `blake3`, now backends of the families), and increments 6 and 7 add
`backends::botan` and `backends::webcrypto`.

`hasher` and `digest` are not Hash2's form: a result is final, and a second `finish`, or an
`update` after it, returns `error::invalid_state`, the value 9, which increment 2 adds with the
family. The Hash2-form types (Hash2's own, `keccak_256`, `blake3`) stay the
types to use with `hash_append`; the families reach every backend's implementation of the same
algorithm, which the agreement tests compare on the first result. Each tag's options, limits
and per-backend engines are specified by the increment's task in the plan, from the backend's
own documentation at its pin; the capability table of 3.5 says which backend has each.

## 3. Backends

### 3.1 The libraries

| Backend | Library | Version and commit | Licence | Targets | Has |
| --- | --- | --- | --- | --- | --- |
| `backends::hash2` | Boost.Hash2's `hmac<H>`, webcpp's construction (spec 1.3) | Boost 1.92 | BSL-1.0 | all | HKDF (three functions), PBKDF2; every Hash2 hmac whose result is a `digest<N>` |
| `backends::sodium` | libsodium | 1.0.22, tag `1.0.22-RELEASE`, commit `77e1ce5d6dee871c49ef211222ba18ef0c486bda` | ISC | all | HKDF (three functions) over SHA-256 and SHA-512; Argon2id; scrypt |
| `backends::bearssl` | BearSSL | webcpporg/bearssl at `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (webcpp::crypto's pin) | MIT | all | `hkdf` alone, over MD5, SHA-1, SHA-224, SHA-256, SHA-384, SHA-512 |
| `backends::openssl` | OpenSSL | 3.5.9 at `45e844fa2a14ec92d146bd8f5778ac130b6625fb` (the superproject's provider) | Apache-2.0 | native, wasip2, wasip3 | HKDF (three functions), PBKDF2; every Hash2 hmac but `hmac_ripemd_128` |
| (none: `keccak_256`) | XKCP, `lib/low/KeccakP-1600/plain-64bits` | master at `d20c8fcaecc543f816a0721d0dfc13b4cf27c14d` (2026-11-10) | CC0 (`brg_endian.h`: Brian Gladman's BSD-style terms) | all | Keccak-p[1600] |
| (none: `blake3`) | BLAKE3's C implementation, `c/` | 1.8.7 at `f3149ec5bb5449af877ba20377a11008ff499fa2` | CC0-1.0, Apache-2.0, or Apache-2.0 with LLVM exception | all | BLAKE3 |
| `backends::xkcp` (increment 3) | XKCP, more of it: `lib/high/Keccak/` (sponge, SP 800-185), `lib/high/KangarooTwelve/`, `lib/high/TurboSHAKE/` (the files the task lists at the pin) | as above | as above | all | 3.5's XKCP column |
| `backends::botan` (increment 6) | Botan 3, as webcpp::crypto's increment 5 provisions it | 3.13.0 measured by crypto; 3.14.1 the latest tag on 2026-11-14 (`01af88c`) | BSD-2-Clause | native, with exceptions only (crypto's D19) | 3.5's Botan column |
| `backends::webcrypto` (increment 7, section 11) | the browser's or node's WebCrypto | the engine's | - | Emscripten | SHA-1/256/384/512, HMAC, HKDF, PBKDF2 |

### 3.2 What each backend has

`backend_has_v<Backend, Kdf, Hmac>` is this table:

| Hmac | hash2 HKDF | hash2 PBKDF2 | sodium HKDF | bearssl `hkdf` | openssl HKDF | openssl PBKDF2 |
| --- | --- | --- | --- | --- | --- | --- |
| `hmac_md5_128` | yes | yes | no | yes | yes (`MD5`) | yes |
| `hmac_sha1_160` | yes | yes | no | yes | yes (`SHA1`) | yes |
| `hmac_sha2_224` | yes | yes | no | yes | yes (`SHA2-224`) | yes |
| `hmac_sha2_256` | yes | yes | yes | yes | yes (`SHA2-256`) | yes |
| `hmac_sha2_384` | yes | yes | no | yes | yes (`SHA2-384`) | yes |
| `hmac_sha2_512` | yes | yes | yes | yes | yes (`SHA2-512`) | yes |
| `hmac_sha2_512_224` | yes | yes | no | no | yes (`SHA2-512/224`) | yes |
| `hmac_sha2_512_256` | yes | yes | no | no | yes (`SHA2-512/256`) | yes |
| `hmac_sha3_224` .. `hmac_sha3_512` | yes | yes | no | no | yes (`SHA3-224` .. `SHA3-512`) | yes |
| `hmac_ripemd_128` | yes | yes | no | no | no | no |
| `hmac_ripemd_160` | yes | yes | no | no | yes (`RIPEMD-160`) | yes |
| `hmac_blake2b_512` | yes | yes | no | no | yes (`BLAKE2B-512`) | yes |
| `hmac_blake2s_256` | yes | yes | no | no | yes (`BLAKE2S-256`) | yes |
| `hmac<H>`, any other Hash2 `H` with a `digest<N>` result | yes | yes | no | no | no | no |

BearSSL has `hkdf` and not `hkdf_extract` or `hkdf_expand`: `br_hkdf_*` keeps the pseudorandom key
inside its context and offers no way to give one in, so the two steps are reached only together.

### 3.3 A backend without the function refuses at compile time

The primary templates' `static_assert`s, and the specialisations for a backend that lacks a
function or an hmac, state exactly one of these texts (each checked by a test of its own in a
directory of its own, spec 7.6):

- `pbkdf2` on `backends::sodium`: "webcpp::hash: pbkdf2 is not in backends::sodium: libsodium
  has no PBKDF2. The backends that have pbkdf2: backends::hash2 (every hmac) and
  backends::openssl (every hmac but hmac_ripemd_128)."
- `pbkdf2` on `backends::bearssl`: the same with "backends::bearssl: BearSSL has no PBKDF2".
- `hkdf_extract` or `hkdf_expand` on `backends::bearssl`: "webcpp::hash: hkdf_extract and
  hkdf_expand are not in backends::bearssl, whose HKDF does both steps together: call hkdf, or
  name backends::hash2, backends::sodium or backends::openssl."
- an hmac `backends::sodium` lacks: "webcpp::hash: backends::sodium has HKDF over hmac_sha2_256
  and hmac_sha2_512 alone. Over this hmac, name backends::hash2 (every hmac), backends::openssl
  (every hmac but hmac_ripemd_128) or backends::bearssl (MD5, SHA-1, SHA-224, SHA-256, SHA-384,
  SHA-512; hkdf alone)."
- an hmac `backends::bearssl` lacks: "webcpp::hash: backends::bearssl has HKDF over hmac_md5_128,
  hmac_sha1_160, hmac_sha2_224, hmac_sha2_256, hmac_sha2_384 and hmac_sha2_512 alone. Over this
  hmac, name backends::hash2 (every hmac) or backends::openssl (every hmac but hmac_ripemd_128)."
- an hmac `backends::openssl` lacks: "webcpp::hash: backends::openssl has no hmac_ripemd_128
  (OpenSSL 3.5 has no RIPEMD-128). Name backends::hash2."
- an `Hmac` that is not a Hash2 hmac over a `digest<N>` result, on any backend: "webcpp::hash:
  the key derivations take a Boost.Hash2 hmac whose result is a digest, as hmac_sha2_256."
- `argon2id` or `scrypt` on another backend than `backends::sodium`: "webcpp::hash: argon2id is
  in backends::sodium alone. Name backends::sodium and add /third_party//sodium to the
  program's Jamfile." (and the same for scrypt).

### 3.4 Each backend's work

- **`backends::hash2`.** `extract`: `Hmac h(salt)` (Hash2 keys an empty salt as zeros of the
  block size, which is HMAC's key of HashLen zeros, RFC 5869 2.2), `update(ikm)`, `result()`.
  `expand`: one keyed `Hmac` built from the PRK, copied for each block T(i) =
  HMAC(PRK, T(i-1) | info | i), the last block cut to fit. `pbkdf2`: one keyed `Hmac` built
  from the password, copied for U_1 = PRF(P, S | INT(i)) and each U_j, the XOR accumulated in
  place, the last block cut. Every block, copy and U is wiped by `detail::wipe` at return.
- **`backends::sodium`.** `sodium_init()` is called first in every engine function (it returns
  0 the first time and 1 after, both success; -1 is `backend_failure`).
  `crypto_kdf_hkdf_sha256_extract`/`_sha512_extract`, `_expand` (whose `ctx` is the info);
  `crypto_pwhash_argon2id(..., crypto_pwhash_ALG_ARGON2ID13)`;
  `crypto_pwhash_scryptsalsa208sha256_ll`. A return of -1 with `errno == ENOMEM` is
  `errc::not_enough_memory`, any other -1 `backend_failure`.
- **`backends::bearssl`.** `br_hkdf_init` with the hash's `br_*_vtable` and the salt (an empty
  salt is passed with length 0: HMAC pads a key with zeros to the block, so it is RFC 5869's
  HashLen zeros, and the agreement test covers it), `br_hkdf_inject`,
  `br_hkdf_flip`, `br_hkdf_produce` into `output`; a produced count other than `output.size()` is
  `backend_failure`. The context is wiped at return.
- **`backends::openssl`.** `EVP_KDF_fetch(nullptr, "HKDF"|"PBKDF2", nullptr)`, a context, the
  parameters (`digest` the name of 3.2, `key` or `pass`, `salt`, `info`, `mode` 0, 1 or 2 for
  HKDF, `iter` and `pkcs5 = 1` for PBKDF2), `EVP_KDF_derive`, then the context and the KDF
  freed. A failure clears the thread's OpenSSL error queue it caused (`ERR_set_mark` and
  `ERR_pop_to_mark`), and is `errc::not_enough_memory` when the last error's reason is
  `ERR_R_MALLOC_FAILURE`, `backend_failure` otherwise. The lengths OpenSSL holds in an `int`
  are refused past `INT_MAX` before it is called (2.8, `detail::backend_limits`, design D26).

### 3.5 The capability table

Which backend provides each function, and from which increment. `hash2` is webcpp's code over
Boost.Hash2 (Hash2's own algorithms, HKDF and PBKDF2); a cell names the function of the
library. This table is `backend_has_v` (2.8) and the source of every refusal text of 3.3; the
documentation renders it from the same data file, `doc/capabilities.json`, which a test compares
with `backend_has_v` for every cell.

| Function | hash2 | xkcp | blake3 | sodium | bearssl | openssl | botan | webcrypto (async, 11) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MD5, SHA-1, SHA-2 (six) | 1 (Hash2) | - | - | SHA-256, SHA-512: 2 | 5 (no SHA-512/t) | 4 | 6 | SHA-1/256/384/512: 7 |
| SHA-3 (four), SHAKE128/256 | 1 (Hash2) | 3 | - | SHA3-256/512, SHAKE: 2 | SHAKE: 5 | 4 | 6 | - |
| Keccak-256 | - | 1 (`keccak_256`) | - | - | - | 4 (`KECCAK-256`) | 6 | - |
| Keccak-224, -384, -512 | - | 3 | - | - | - | 4 | 6 | - |
| cSHAKE, KMAC, TupleHash, ParallelHash | - | 3 | - | - | - | KMAC: 4 | cSHAKE, KMAC: 6 | - |
| KangarooTwelve (KT128, KT256) | - | 3 | - | - | - | - | - | - |
| TurboSHAKE128/256 | - | 3 | - | 2 | - | - | - | - |
| BLAKE2b (key, salt, personalisation, length) | 512 fixed: 1 (Hash2) | - | - | 2 | - | 512 and MAC: 4 | 6 | - |
| BLAKE2s-256 | 1 (Hash2) | - | - | - | - | 4 | 6 | - |
| BLAKE3 (hash, keyed, derive_key, XOF) | - | - | 1 | - | - | - | - | - |
| RIPEMD-128, RIPEMD-160 | 1 (Hash2) | - | - | - | - | RIPEMD-160: 4 | RIPEMD-160: 6 | - |
| SM3, Streebog, Whirlpool, Skein-512 | - | - | - | - | - | SM3: 4 | 6 | - |
| HMAC | 1 (Hash2, every hash) | - | - | SHA-256/512/512-256: 2 | 5 | 4 | 6 | 7 |
| SipHash-2-4, SipHash-2-4x | SipHash-2-4 64: 1 (Hash2) | - | - | 2 | - | - | 6 | - |
| HKDF | 1 | - | - | 1 (SHA-256/512) | 1 | 1 | 6 | 7 |
| `async_argon2id`, `async_argon2`, `async_scrypt`, `async_pbkdf2` on another thread | every backend that has the function, only with threads (11.5): native `threading=multi`, Emscripten `-pthread`; never WASI | | | | | | | `async_pbkdf2` without threads too |
| PBKDF2 | 1 | - | - | - | - | 1 | 6 | 7 |
| scrypt | - | - | - | 1 | - | 4 | 6 | - |
| Argon2id, one lane | - | - | - | 1 | - | 4 | 6 | - |
| Argon2i | - | - | - | 2 | - | 4 | 6 | - |
| Argon2d; Argon2 with lanes, secret, associated data | - | - | - | - | - | 4 | 6 | - |
| BLAKE2b subkeys (`crypto_kdf`) | - | - | - | 2 | - | - | - | - |
| bcrypt, bcrypt-pbkdf | - | - | - | - | - | - | 6 | - |
| KBKDF (SP 800-108), SSKDF (SP 800-56C), X9.63 KDF | - | - | - | - | - | 4 | 6 (where it has them) | - |

A cell of 6 is checked against Botan's documentation at its pin by increment 6's first task,
which corrects the table where Botan lacks a function; no cell is assumed.

## 4. The default backend

### 4.1 The default: `backends::hash2` (the owner's decision of 2026-11-16)

The default backend of every key derivation, `hkdf`, `hkdf_extract`, `hkdf_expand` and `pbkdf2`,
over every hmac, on native, wasip2, wasip3 and Emscripten and whatever the program links, is
`backends::hash2`. `backends::sodium` is the default only for what hash2 does not offer:
`argon2id` and `scrypt`, of which it is the only backend; a program that calls them without
`/third_party//sodium` stops with libsodium's "needs" message (5.7), raised for `backends::sodium`
by the primary template of the engine in `detail/engines.hpp` and tested by the messages
`argon2id_default_not_linked` and `scrypt_default_not_linked`.
`doc/capabilities.json` (layout version 3) names each function row's `"default"`, as the table
below (`null` for a type), and `capabilities_test` compares it with `default_backend`.

| Operation (tag of `kdfs`) | Default backend |
| --- | --- |
| `hkdf`, `hkdf_extract`, `hkdf_expand`, `pbkdf2` | `backends::hash2` |
| `argon2id`, `scrypt` | `backends::sodium` |

`default_backend<Algorithm, Hmac = void>` and `default_kdf_backend<Hmac, Kdf = kdfs::hkdf>`
(which is `default_backend<Kdf, Hmac>`) keep their names and signatures; their answer is the
table's and depends neither on the hmac, nor on the libraries linked, nor on the target. Each
function names its default through its own tag (`hkdf_extract` through `kdfs::hkdf_extract`, ...),
and `backend_has_v` answers for the two steps through `kdfs::hkdf_extract` and `kdfs::hkdf_expand`.

**The other backends are explicit choices.** `backends::bearssl`, `backends::openssl` and the HKDF
of `backends::sodium` stay, with their vectors, agreement tests, limits and refusals, and are
reached only by naming them in the type, as `hkdf<hmac_sha2_256, backends::openssl>`; none is
ever a default.

**No define changes a default.** webcpp::hash reads no profile (`WEBCPP_HASH_PROFILE_FASTEST`,
`_SMALLEST`, `_PORTABLE`, `_BACKEND_<NAME>`), not webcpp's umbrella `WEBCPP_PROFILE_FASTEST`,
`_SMALLEST` and `_PORTABLE`, and no override `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`; a
former define has no effect, which `default_backend_former_defines` proves on the four targets
with every library linked. No `#pragma detect_mismatch` guards a profile or an override, since a
fixed default cannot differ between two translation units.

**What it replaces.** Until 2026-11-16 this section held one profile per program (`auto`:
natively the fastest, on WebAssembly the smallest; `fastest`; `smallest`; `portable`, the first
that has it of `hash2`, `xkcp`, `blake3`, `sodium`, `bearssl`; a named backend), webcpp's umbrella
profile, per-algorithm overrides in the generated `detail/overrides.hpp`, and a choice per
operation from the measured orders of `detail/ranking.hpp` (design D8, D20, D25, D28). Increment 1
built them; the owner retired them all (design D33). `portable` equals the new default, since
hash2 exists on every target; a named backend would make another backend a default; and with the
backend named in the type an override would make BearSSL or OpenSSL a default again and hide the
backend from the reader of the call, while Argon2id and scrypt have one backend, which an
override cannot change.

The families of increments 2 to 7 (2.15) name their default through `default_backend<Algorithm>`
too; for an algorithm Boost.Hash2 lacks it is the owner's to decide before increment 2 (design O3).

### 4.2 The measured table, a dated reference

`doc/measurements.json` and `doc/measurements.adoc` hold webcpp's own measurements of increment
1: the size of one program per algorithm, backend and target (native, wasip2, wasip3,
Emscripten), built at `variant=release optimization=space` and linked with only that backend's
library, and the throughput natively (and on each WebAssembly target where it ran), at
`optimization=speed`, with the machine, compiler, target, date and each pin. They are kept as a
reference measured on 2026-11-16, labelled so, that decides nothing: no default and no order of
backends comes from them, and the guide's comparison tables (section 8) read them as reference.

`doc/measurements.json` keeps its records and orders; its about and layout text carry the label
and its digest is recomputed. `tools/hash/doc_tables.py` renders `doc/measurements.adoc` from it,
without the orders and without a default per key derivation, with the label and why hash2 is the
default, and its `--check`, run at every documentation build, fails when the digest does not match
the content (a hand edit of the reference).

No lane re-measures or compares them. The own lane `measure` (its programs under
`libs/hash/test/measure/`, the CI matrix entry "Own lane (hash, measure)"), `tools/hash/measure.py`
whole with its test, and the generated `include/webcpp/hash/detail/ranking.hpp` are retired (design
D33), and
with them the rule of orders that changed only past 10 percent, the native toolchain's
comparison and the re-measurement on an idle machine before a release (design D32). What the
measurements showed is recorded in design D32 and D33: for HKDF over `hmac_sha2_256` the native
medians of libsodium, hash2 and OpenSSL were under 1 percent apart on one loaded macOS arm64
machine, and the isolated programs on WebAssembly differed by a few KB.

### 4.3 The throughput measurement the owner named

Beside the table, `libs/hash/bench/pbkdf2_sha512.cpp` derives 64 bytes by PBKDF2-HMAC-SHA-512
with 1,000,000 iterations, three runs, the median reported, for: `backends::hash2`;
`backends::openssl` (native, wasip2, wasip3); libsodium, which has no PBKDF2, through RFC 8018's
loop written in the benchmark over `crypto_auth_hmacsha512_init/update/final` (a measurement of
libsodium's HMAC-SHA-512 in PBKDF2's shape, never part of the library). The results go to
`doc/performance.adoc` with the machine, compiler, target and date, its times measured by hand
with `bench/collect.py`; the benchmark's correctness program `pbkdf2_sha512_key` (its derived keys
checked, never its time) runs in every lane of `libs/hash/test`. Its table, like 4.2's, decides no
default (design D33).

## 5. Build

### 5.1 The library's target

```jam
project /webcpp/hash ;

alias hash : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;
```

Header-only. A program names `/webcpp/hash//hash` and the C library of each part it uses:

```jam
exe app : app.cpp /webcpp/hash//hash ;                              # Hash2's algorithms, HMAC, hkdf and pbkdf2 on backends::hash2
exe eth : eth.cpp /webcpp/hash//hash /third_party//xkcp ;           # keccak_256
exe b3 : b3.cpp /webcpp/hash//hash /third_party//blake3 ;           # blake3
exe pw : pw.cpp /webcpp/hash//hash /third_party//sodium ;           # argon2id, scrypt, backends::sodium
exe kdf : kdf.cpp /webcpp/hash//hash /webcpp/hash//openssl ;        # backends::openssl (native, wasip2, wasip3)
exe kdf2 : kdf.cpp /webcpp/hash//hash /third_party//bearssl ;       # backends::bearssl
```

### 5.2 The C libraries, file by file

Each is a submodule under `third_party/libs/<name>`, pinned, never edited, built by
`third_party/Jamfile` as a static library from its own sources in its own C dialect, with
`-ffunction-sections -fdata-sections` (`/Gy /Gw` on MSVC), `<warnings>off`, the defines each
target needs written beside the sources with their reasons (CODING-GUIDELINES.md chapter 5),
webcpp::hash's canary `libs/hash/src/canary_<name>.c` added, and usage requirements giving the
include directory, `WEBCPP_HASH_HAS_<NAME>=1` and the linker's removal of unused sections. A
checkout without `libs/hash` stops the build of these targets, naming
`git submodule update --init libs/hash`; a submodule not checked out stops its own target,
naming its command.

| Target | Submodule | Sources | Defines | Usage requirements |
| --- | --- | --- | --- | --- |
| `xkcp` | `third_party/libs/xkcp` (`XKCP/XKCP`) | `lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c`, webcpp's `src/xkcp_adapter.c` and `src/canary_xkcp.c` | none: XKCP's "generic64" profile (all rounds unrolled, no lane complementing) on every target; its `config.h`, generated by XKCP's own build, is webcpp's `libs/hash/src/xkcp/config.h` with `#define XKCP_has_KeccakP1600` alone | `WEBCPP_HASH_HAS_XKCP=1` (no include directory: the public header declares only the adapter) |
| `blake3` | `third_party/libs/blake3` (`BLAKE3-team/BLAKE3`) | `c/blake3.c`, `c/blake3_dispatch.c`, `c/blake3_portable.c` on every target; on native x86-64 also `c/blake3_sse2.c` (`-msse2`), `c/blake3_sse41.c` (`-msse4.1`), `c/blake3_avx2.c` (`-mavx2`), `c/blake3_avx512.c` (`-mavx512f -mavx512vl`), with MSVC `/arch:AVX2` and `/arch:AVX512` for the last two; on native arm64 `c/blake3_neon.c`; `src/canary_blake3.c` | WebAssembly: `BLAKE3_NO_SSE2`, `BLAKE3_NO_SSE41`, `BLAKE3_NO_AVX2`, `BLAKE3_NO_AVX512` (the portable code alone); arm64: `BLAKE3_USE_NEON=1`; never `BLAKE3_USE_TBB`; the `.S` and `.asm` files are not compiled (the intrinsics files are their equivalents, which BLAKE3's own CMake chooses where it has no assembler) | `<include>third_party/libs/blake3/c`, `WEBCPP_HASH_HAS_BLAKE3=1` |
| `sodium` | `third_party/libs/libsodium` (`jedisct1/libsodium`) | every `.c` of `src/libsodium/` (141), and `src/canary_sodium.c`; none of the seven `.S` files | the defines libsodium's own `configure` finds for each target (spec 5.3), with `CONFIGURED=1`, `SODIUM_STATIC=1`, `NATIVE_LITTLE_ENDIAN=1`, `DEV_MODE` off, and neither `HAVE_AMD64_ASM` nor `HAVE_AVX_ASM` (`--disable-asm`), so that the C code is used | `<include>third_party/libs/libsodium/src/libsodium/include`, `SODIUM_STATIC=1`, `WEBCPP_HASH_HAS_SODIUM=1`; on Emscripten the link flag `-sALLOW_MEMORY_GROWTH=1` |
| `bearssl` | `third_party/libs/bearssl` (webcpp::crypto's, Task 5 of its plan) | as webcpp::crypto builds it, plus `src/canary_bearssl.c` of webcpp::hash | webcpp::crypto's | webcpp::crypto's, plus `WEBCPP_HASH_HAS_BEARSSL=1` |

**libsodium's memory on Emscripten (design D27).** Argon2id and scrypt allocate the memory their
costs name (64 MiB for libsodium's interactive Argon2id costs), past Emscripten's fixed 16 MiB, so
`/third_party//sodium` gives a program that links it `-sALLOW_MEMORY_GROWTH=1` on Emscripten as a
usage requirement, as libsodium's own tests link: a user who names libsodium gets a program that
works without knowing the flag. Naming it therefore changes an Emscripten program by that flag
alone (7.8).

libsodium's `src/libsodium/include/sodium/version.h` is generated by its `configure`; upstream
ships a generated copy, `builds/msvc/version.h`, which its `build.zig` copies into place. b2
copies that file, unchanged, into the build directory as `sodium/version.h` and puts that
directory and its `sodium/` subdirectory on the compile's include path; the submodule is never
written. libsodium's headers that a webcpp header includes (`sodium/core.h`,
`sodium/crypto_kdf_hkdf_sha256.h`, `sodium/crypto_kdf_hkdf_sha512.h`,
`sodium/crypto_pwhash_argon2id.h`, `sodium/crypto_pwhash_scryptsalsa208sha256.h`) do not include
`version.h`.

### 5.3 libsodium's defines per target

Derived from libsodium's own `configure`, run once per target at the pin (`--disable-asm
--disable-shared --enable-static`, and for WebAssembly `--without-pthreads --disable-ssp` with
wasi-sdk's clang or `emconfigure`, as libsodium's `dist-build/` scripts do) by the plan's Task 8,
which records each command and its `DEFS` in a table of this proposal and beside the sources in
`third_party/Jamfile`; MSVC, which has no `configure`, takes the defines of
`builds/msvc/properties/` at the pin. Each `HAVE_*` only on the targets whose `configure` found
it; `DEV_MODE` off everywhere; `CONFIGURED=1`, `SODIUM_STATIC=1` and `NATIVE_LITTLE_ENDIAN=1`
everywhere; `HAVE_AMD64_ASM` and `HAVE_AVX_ASM` never. `build.zig`'s lists, which give several
defines on every target unconditionally (`HAVE_ATOMIC_OPS`, `HAVE_TI_MODE`, `HAVE_CET_H`,
`DEV_MODE`, ...), are not used (the ruling of 2026-11-15).

### 5.4 The canaries and the three layers

`libs/hash/include/webcpp/hash/detail/canary.h` has webcpp::crypto's form with `WEBCPP_HASH_`
names: the commits (`WEBCPP_HASH_XKCP_COMMIT d20c8fc`, `WEBCPP_HASH_BLAKE3_COMMIT f3149ec`,
`WEBCPP_HASH_SODIUM_COMMIT 77e1ce5`, `WEBCPP_HASH_BEARSSL_COMMIT 7bea48e`), each overridable by a
`_CANARY_COMMIT` test seam, and the macros `WEBCPP_HASH_DETAIL_CANARY(name, commit,
configuration)` and `WEBCPP_HASH_DETAIL_MISMATCH_VALUE(...)`. Each `src/canary_<name>.c` defines,
from the defines it is compiled with:

| Library | Configurations | Symbol |
| --- | --- | --- |
| XKCP | `generic64` (no `KeccakP1600_plain64_useLaneComplementing`, no `KeccakP1600_plain64_unrolling`), else `unconfigured` | `webcpp_hash_xkcp_d20c8fc_generic64__add_third_party_xkcp_to_your_link` |
| BLAKE3 | `portable` (all four `BLAKE3_NO_*`), `x86` (none of them, no `BLAKE3_USE_NEON`), `neon` (`BLAKE3_USE_NEON == 1`), else `unconfigured` | `webcpp_hash_blake3_f3149ec_<configuration>__add_third_party_blake3_to_your_link` |
| libsodium | `configured` (`CONFIGURED == 1`, `SODIUM_STATIC == 1`, `NATIVE_LITTLE_ENDIAN == 1`, no `HAVE_AMD64_ASM`), else `unconfigured` | `webcpp_hash_sodium_77e1ce5_configured__add_third_party_sodium_to_your_link` |
| BearSSL | `getentropy`, `noseeder`, as webcpp::crypto's canary decides them | `webcpp_hash_bearssl_7bea48e_<configuration>__add_third_party_bearssl_to_your_link` |

The three layers (CODING-GUIDELINES.md chapter 5 rule 6): the header's `#error` when
`WEBCPP_HASH_HAS_<NAME>` is missing; the canary read by every function that calls the library
(a `volatile` read of the `extern "C" const char`), so that a missing library or one of another
commit or configuration fails to link with the symbol's name; and on MSVC
`#pragma detect_mismatch("webcpp_hash_backend_<name>", "<name>-<commit7>-<configuration>")` in
the header and the canary. OpenSSL has no canary (the provider injects nothing into upstream,
chapter 5 rule 7); its layer is the compile-time message. It is never a default (4.1), so no
`detect_mismatch` guards a default backend.

### 5.5 One copy per program

`/third_party//bearssl` is webcpp::crypto's target, one archive whatever library names it: its
sources gain hash's canary file (an object of its own, whose unused section the linker drops).
`/third_party//sodium` is introduced here and shared by webcpp::crypto's later libsodium
backend, which adds its own canary to the same target. `/webcpp/hash//openssl` links the
superproject's one provider, `/webcpp/third_party//openssl`, the same archives as
`/webcpp/crypto//openssl` and `/webcpp/asio//openssl`.

### 5.6 `/webcpp/hash//openssl`

```jam
rule openssl-linked ( properties * )
{
    if [ webcpp-target.target-of $(properties) ] in native wasip2 wasip3
    {
        return <library>/webcpp/third_party//openssl <define>WEBCPP_HASH_HAS_OPENSSL=1 ;
    }
}

path-constant WEBCPP_HASH_ROOT : . ;
alias openssl : : : : <conditional>@openssl-linked
    <toolset>emscripten:<cflags>"-include \"$(WEBCPP_HASH_ROOT)/src/openssl_on_emscripten.hpp\""
    <toolset>emscripten:<cxxflags>"-include \"$(WEBCPP_HASH_ROOT)/src/openssl_on_emscripten.hpp\"" ;
```

webcpp::crypto's form: on Emscripten every source of the program stops at the forced include,
whose `#error` is the first message of spec 2.2.

### 5.7 The messages

| When | Text |
| --- | --- |
| `keccak.hpp` without `WEBCPP_HASH_HAS_XKCP` | "webcpp::hash: keccak_256 needs XKCP. Add /third_party//xkcp to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//xkcp ;" |
| `blake3.hpp` without `WEBCPP_HASH_HAS_BLAKE3` | "webcpp::hash: blake3 needs BLAKE3's C implementation. Add /third_party//blake3 to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//blake3 ;" |
| `backends/sodium.hpp`, `argon2.hpp` or `scrypt.hpp` without `WEBCPP_HASH_HAS_SODIUM` | "webcpp::hash: backends::sodium, argon2id and scrypt need libsodium. Add /third_party//sodium to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//sodium ;" |
| `backends/bearssl.hpp` without `WEBCPP_HASH_HAS_BEARSSL` | "webcpp::hash: backends::bearssl needs BearSSL. Add /third_party//bearssl to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//bearssl ;" |
| `backends/openssl.hpp` without `WEBCPP_HASH_HAS_OPENSSL` | "webcpp::hash: backends::openssl needs OpenSSL. Add /webcpp/hash//openssl to the program's Jamfile (native, wasip2 and wasip3): exe app : app.cpp /webcpp/hash//hash /webcpp/hash//openssl ;" |
| OpenSSL on Emscripten | spec 2.2's first `#error` |
| a link without the library | the canary's symbol, as undefined |
| two TUs, two configurations, MSVC | LNK2038 on `webcpp_hash_backend_<name>` |

The messages of the profiles and the overrides (two profiles of one level, two overrides of one
algorithm, OpenSSL selected on Emscripten by a profile or an override, LNK2038 on
`webcpp_hash_profile` or `webcpp_hash_default_backend_<algorithm>`) are retired with them on
2026-11-16 (design D33): a former define has no effect.

## 6. Allocation and wiping

### 6.1 webcpp's code

No function of webcpp::hash allocates: the public functions, `backends::hash2`, the engines and
the two hash types use only the caller's spans and objects on the stack. Every secret
intermediate (HKDF's T blocks, PBKDF2's U and its accumulator, keyed hmac copies, BearSSL's
context) is wiped through `volatile` stores before the function returns, on success and on
failure. `blake3` and `keccak_256` hold no secret beyond what their user gives; their
destructors do not wipe (Hash2's types do not), which the page states.

### 6.2 The C libraries that allocate (the recorded departures)

- **libsodium's Argon2id and scrypt** allocate the memory their cost names (`mmap` where
  `HAVE_MMAP`, else `malloc` and, for Argon2's 64-byte alignment, `posix_memalign` or an
  aligned `malloc`), outside any allocator of the program's. The owner's alternative, the
  Argon2 reference implementation's `allocate_cbk`/`free_cbk` callbacks, would route Argon2id's
  memory through a `std::pmr::memory_resource`; it is not built (design D12), and becomes a
  later backend `backends::argon2` if a user's resource must hold the memory.
- **OpenSSL** allocates its KDF and context in every call (`EVP_KDF_fetch`,
  `EVP_KDF_CTX_new`), through its own allocator, as webcpp::crypto's D17 records for its
  contexts; webcpp keeps no OpenSSL object between calls, since that would be state.
- **libsodium's `sodium_init`** allocates nothing; it keeps libsodium's own process-wide state
  (its CPU features and chosen implementations), which is the C library's, not webcpp's.

A counting test (spec 7.7) proves 6.1 on every backend and target, and measures 6.2 where the
C library's allocator can be counted.

## 7. Tests

Every test uses Boost.Core's lightweight_test, ends
`return webcpp::test::exit_status(boost::report_errors());`, and is declared by the test
Jamfile's rule `hash-run`, which builds it twice, the second `<exception-handling>off` with the
suffix `_no_exceptions`, on every target its libraries have.

### 7.1 Vectors

| Function | Vectors |
| --- | --- |
| Hash2's aliases | identity with Hash2's types; one digest each from Hash2's own tests (`sha2_256("abc")`), through `hash_append` |
| `keccak_256` | the Keccak team's Keccak-256 of the empty message, `c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470`, and of "abc", `4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45`; EIP-55's checksum examples (each address's case pattern is a function of Keccak-256 of its lowercase hex); Hash2's `detail::keccak_base<0x01, 512, 256>` as an oracle on every length 0 to 1,000 and on random sequences of `update` and `result` (the seed fixed and printed) |
| `blake3` | BLAKE3's `test_vectors/test_vectors.json` at the pin, every case (hash, keyed_hash, derive_key, each at its full extended length through `squeeze` and through repeated `result()`), read at run time through `/third_party//blake3_vectors` |
| `hkdf_*` | RFC 5869's seven cases (A.1 to A.3 SHA-256, A.4 to A.7 SHA-1), PRK and OKM, on every backend that has the hmac |
| `pbkdf2` | RFC 6070's six cases (SHA-1; the 16,777,216-iteration case in a program of its own, `pbkdf2_rfc6070_long`, on every target, its time recorded), RFC 7914 section 11's two (SHA-256), on every backend that has the hmac |
| `argon2id` | RFC 9106 section 5.3's Argon2id case through libsodium's internal `argon2_ctx` (parallelism 4, secret and associated data, which the public function does not take), which proves libsodium's core is RFC 9106's; three cases of libsodium's `test/default/pwhash_argon2id.c` through `webcpp::hash::argon2id`; the public function against `argon2_ctx` with one lane on random inputs |
| `scrypt` | RFC 7914 section 12's four cases; the fourth (N = 1,048,576, r = 8, 1 GiB) in a program of its own, native, and on each WebAssembly target where its run succeeds, measured by the plan |

Test data from a submodule is read at run time through a `third_party/Jamfile` target that gives
its directory (`webcpp.data`), as `/third_party//wycheproof` does: `/third_party//blake3_vectors`.

### 7.2 Agreement between backends

`agreement_hkdf_test` and `agreement_pbkdf2_test`, with a fixed, printed seed and 1,000 random
cases each (lengths 0 to 300 for salt, IKM, password and info, outputs of 1 to 4 digest sizes,
iterations 1 to 64): natively every backend with every other on every hmac both have; on
wasip2 and wasip3 `backends::hash2`, `backends::sodium`, `backends::bearssl` and
`backends::openssl`; on Emscripten `backends::hash2`, `backends::sodium` and
`backends::bearssl`. Empty salt, empty IKM, empty info and empty password are always among the
cases.

### 7.3 The C libraries' own suites

The own lane `c-suites` of `libs/hash/test/c_suites`, on every target: libsodium's
`test/default/*.c` with its `cmptest.h` and their `.exp` files, every test on every target, each
run in a writable directory of the build (`cmptest.h` writes `<name>.res` there); XKCP's `tests/UnitTests`
built with webcpp's `config.h` and run with `--SnP` (the Keccak-p[1600] tests); BLAKE3's
`c/main.c`, unmodified, over every case of `test_vectors.json`, as the lane's programs
`blake3_c_suite` and `blake3_c_suite_corrupted` (design D29). `blake3_main.c` compiles `main.c`
with its `main` and its standard input and output renamed; `blake3_c_suite.cpp` does what BLAKE3's
`c/test.py` does with `c/blake3` (a binary built inside the submodule, where webcpp builds
nothing): for each case the input `i % 251` for each `i` below `input_len`, the program run with
no argument, with `--keyed <hex of the key>` and with `--derive-key <context>`, each with
`--length` of the whole expected output, and every line it prints compared with the file's;
`cpu_features.c` defines, off x86, the two names `main.c` reads that BLAKE3's dispatcher defines
on x86 alone. The library is built with `BLAKE3_TESTING` where `blake3_dispatch.c` reads it
(`<webcpp-blake3-testing>on`, a build of its own). `blake3_c_suite_corrupted` changes one expected
line and passes only when exactly that line is reported. These programs replaced the plan's
`tools/test/blake3_c_suite.py`, a tools test that no lane and no CI job ran.

### 7.4 WebCrypto as an outside oracle

On Emscripten under node, in every increment: `webcrypto_oracle_test` computes every algorithm
WebCrypto has (SHA-1, SHA-256, SHA-384, SHA-512 through `crypto.subtle.digest`; HMAC through
`sign`; HKDF and PBKDF2 over each through `deriveBits`; an `EM_ASYNC_JS`, under the default JSPI
suspension) and compares with every backend that has the same function on Emscripten, on 200
random cases; each increment adds its functions to it where WebCrypto has them.

### 7.5 Guards and limits

Each guard of spec 2.8 to 2.11 has its case on each backend: the output limits at the limit and
one past it (255 times the digest size and one more; PBKDF2's limit computed without
allocating it), a 31-byte and a 33-byte PRK for a 32-byte digest, info of 32,768 and 32,769
bytes, iterations 0, Argon2id's salts of 15 and 17 bytes and memory of 8,191, 8,193 and 9,216
bytes, scrypt's N of 0, 1, 3 and 2^(16r), r times p of 2^30; natively, each of OpenSSL's
`backend_limits` with a span of `INT_MAX + 1` bytes, never dereferenced, and a derivation within
them (`openssl_limits_test`), OpenSSL never run past its limit. A guard's refusal leaves `output`
untouched; a backend's failure zeroes it (provoked with OpenSSL's digest name replaced through a
test seam, `WEBCPP_HASH_DETAIL_TEST_OPENSSL_DIGEST`).

### 7.6 The messages

Every text of spec 3.3 and 5.7 is a `webcpp.compile-diagnostic` in a directory of its own
(`// expected-error@webcpp/hash/<header>:* {{<the text>}}`), on clang; a message raised on
Emscripten is a fixture of `tools/test/hash_jamfiles_test.py`; each canary's absence is a
`webcpp.link-fail` per library; on MSVC the same cases are `webcpp.compile-fail` and
`webcpp.link-fail`, which fail on the MSVC lanes every push and are checked on clang and the
GNU and wasm linkers, as webcpp::crypto's decision 5 records.

### 7.7 Allocation

`allocation_test` replaces every form of `operator new` with a counter and calls every function
on every backend and target: zero for webcpp's code and for `backends::hash2`, `sodium`'s HKDF,
`bearssl`, `keccak_256` and `blake3`; Argon2id and scrypt allocate through libsodium's C
allocator, which the test does not see and the page names; OpenSSL's allocations are counted
natively through `CRYPTO_set_mem_functions` in a separate program, reported, not bounded.

### 7.8 The user's Jamfiles and one program

`tools/test/hash_jamfiles_test.py`, with fixtures under `tools/test/fixtures/hash_jamfiles/`,
builds every Jamfile line of spec 5.1 and every build stop on every target, as webcpp::crypto's
`crypto_jamfiles_test.py` does. `webcpp.same-program` proves on every target that a program
naming `/third_party//xkcp`, `//blake3`, `//sodium` or `//bearssl` without calling it is
byte-identical to one without it. On Emscripten `/third_party//sodium` adds its link flag
`-sALLOW_MEMORY_GROWTH=1` (5.2): `unused_sodium_emscripten` gives the program without libsodium
that flag explicitly and requires the two to be identical, so the flag is shown to be the only
difference.

### 7.9 The lanes

Native, wasip2, wasip3, Emscripten and gcc-15 locally, from scratch (`-a`); MSVC and Apple clang
in the CI at the end. Own lane: `c-suites` (the lane `measure` is retired, design D33); the
throughput benchmark's times of 4.3 are measured by hand, and its correctness program runs in every
lane.

## 8. Documentation

The library's page (`doc/hash.adoc` and its parts) is written in two models:

- **noble's README** (paulmillr's `noble-hashes`, MIT; its style, not its text): one short
  section per algorithm, each with what it is, a complete C++ example that compiles (every
  example is a program of `libs/hash/example/` built on every target), and its pitfalls (Keccak-256
  is not SHA3-256; a MAC is compared with `webcpp::crypto::equal`; PBKDF2's iterations and
  Argon2's costs; BLAKE2b's personalisation length; an XOF's output length is the caller's
  security choice);
- **libsodium's documentation** (`jedisct1/libsodium-doc`, ISC, `master` at
  `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`, 2026-11-11): its explanations may be adapted for the
  functions libsodium provides, with attribution: the library's `NOTICE.md` keeps libsodium-doc's
  copyright notice and the ISC licence text, and each adapted section says it adapts
  libsodium's documentation. Wikipedia (CC BY-SA, incompatible with BSL-1.0) is only cited,
  never copied; the RFCs and NIST publications are cited by number and section.

**Each chapter opens with two comparison tables (design D31).** The chapter of each namespace
(digests, MACs, key derivations, password hashing, the non-cryptographic hashes) opens with two
tables that `tools/hash/doc_tables.py` generates from `doc/capabilities.json` and the
measurements: "What each of X is" (Name, Output, Security, Standard, Status, In the browser) and
"How to choose among X" (Name, the two measured columns when there are measurements, Where it is
used, Choose it when). Each is led by the name and has fewer than eight columns, since the site
reads a table of eight columns or more as a matrix that scrolls in its box rather than stack;
`hash_doc_tables_test` checks every generated table, with and without measurements.

Per backend: its Jamfile line, its column of the capability table (rendered from
`doc/capabilities.json`), what it allocates, and every message with its fix (CODING-GUIDELINES.md
chapter 5 rule 9, written so that an AI agent writes a correct Jamfile at once). The default
backend (4.1) and how to name another one in the type, in the chapter `choosing_backend.adoc`
(anchors `hash-choosing-backend` and `hash-default-backend`), and `doc/backends.adoc` rendered
from `doc/capabilities.json` by `tools/hash/doc_tables.py`; the measured table as a reference dated
2026-11-16 that decides nothing (4.2). The departures of 1.3 and 6.2 with their conditions.

**Teaching chapters by use case**, framed as raising the security of what a reader builds, with
examples that are webcpp's own code: peer-to-peer networks (content addressing with BLAKE3 and
BLAKE2b, Nostr event ids with SHA-256 and NIP-44's HKDF, SipHash for tables keyed by peers'
input); cryptocurrencies (Bitcoin's BIP39 seeds with PBKDF2-HMAC-SHA-512, hash160 with SHA-256
and RIPEMD-160, Ethereum's Keccak-256 addresses and EIP-55); privacy (password storage with
Argon2id, key separation with HKDF and BLAKE2b subkeys, domain separation with personalisation
and cSHAKE). Where an example mirrors what a public application built on noble does, the page
names the application only when it is public, and the example is webcpp's own code; nothing is
copied from it. The upstream note of 9.1 and the changelog close the page.

## 9. Upstream and impact

### 9.1 Keccak-256 in Boost.Hash2

Boost.Hash2 1.92 has `detail::keccak_base<PaddingDelim, C, D>`, of which
`keccak_base<0x01, 512, 256>` is Keccak-256. The long-term plan is a public `keccak_256` (and
224, 384, 512) contributed to Boost.Hash2; when a Boost release has it, `webcpp::hash::keccak_256`
becomes `using keccak_256 = boost::hash2::keccak_256;`, `/third_party//xkcp` is no longer needed
by it, and, because spec 2.6's sponge is Hash2's, no result changes. The plan prepares the
contribution's text; opening it is the owner's outward action.

### 9.2 webcpp::crypto

A later change of webcpp::crypto replaces its spec 1.2's "HKDF ... written on Hash2's hmac when
needed" by a dependency on webcpp::hash (`<library>/webcpp/hash//hash`), and its libsodium
backend (version 0.2) uses `/third_party//sodium` as built here, adding its canary to it.
`/third_party//bearssl` gains webcpp::hash's canary and `WEBCPP_HASH_HAS_BEARSSL=1` in this
library's plan, which changes nothing a crypto program links.

## 10. Rulings recorded

- PRK of exactly the digest size on every backend (2.8; design D9).
- HKDF's info capped at 32,768 bytes on every backend, OpenSSL 3.5.9's `HKDF_MAXINFO`
  (`providers/implementations/kdfs/hkdf.c` at `45e844f`, line 37), so that no call succeeds on one
  backend and fails on another.
- PBKDF2 on OpenSSL with `pkcs5 = 1` (2.9; design D10).
- `default_kdf_backend` takes the KDF as a second, defaulted parameter, because libsodium and
  BearSSL have HKDF and no PBKDF2 (design D8); the parameter and its name stay, though every
  key derivation's default is now `backends::hash2` (design D33).
- Argon2id's memory must be a multiple of 1,024 bytes (2.10).
- BLAKE3's SIMD files are compiled on the targets of their instruction sets alone, a recorded
  exception to chapter 5 rule 2's "every file on every target": they include x86 or Arm
  intrinsics headers that no WebAssembly sysroot has (design D6).
- From increment 2 on, the families of 2.15 are not Hash2's form: one finalisation, and
  `invalid_state` (value 9) after it.
- Every key derivation defaults to `backends::hash2`, and Argon2id and scrypt to
  `backends::sodium`, on every target and link set; BearSSL, OpenSSL and libsodium's HKDF only
  when named in the type; no profile, no override, no measured order (4.1, 4.2; design D33, the
  owner's decision of 2026-11-16, which retires the profile of five defines of design D20).
  WebCrypto serves the asynchronous family alone (section 11).
- The cheap asynchronous functions exist only for the algorithms WebCrypto serves, on every
  target, completing through `post` on a synchronous backend (11.2); the heavy ones run on
  another thread and exist only with threads (11.5).
- Increment 1's amendments (design D25 to D32): the public tags `kdfs::hkdf_extract` and
  `kdfs::hkdf_expand` and a default backend per operation (4.1); `detail::backend_limits`, with
  OpenSSL refusing past `INT_MAX` (2.8); libsodium's `-sALLOW_MEMORY_GROWTH=1` on Emscripten (5.2,
  7.8); the overrides' refusals in `detail/overrides.hpp` behind `kdf.hpp` (retired with the
  overrides by design D33); BLAKE3's own
  program in the lane `c-suites` (7.3); `[[nodiscard]]` on every result (2.1); two comparison
  tables a chapter, each under eight columns (8); the native orders provisional until re-measured
  on an idle machine (retired with the orders by design D33).
- No `LICENSE-<ORIGIN>.txt` in the library's repository: webcpp::hash's code derives from no
  C library's (chapter 10, "The licence"); the README and the third-party table name each
  library and its licence. `NOTICE.md` holds libsodium-doc's copyright notice and ISC licence,
  for the documentation adapted from it (section 8).

## 11. The browser backend and the asynchronous family (the owner's answer to O1)

The owner decided on 2026-11-14 that the browser backend, `backends::webcrypto` (WebCrypto's
`SubtleCrypto`, Emscripten only), is reached through a **parallel asynchronous API** beside the
synchronous one, in `<webcpp/hash/async.hpp>` (increment 7):

```cpp
namespace webcpp::hash {

/** The backend of the asynchronous family: WebCrypto on Emscripten where it has the algorithm, else `default_backend<Algorithm, Hmac>`. */
template <class Algorithm, class Hmac = void>
using default_async_backend = /* section 11.2 */;

/** Hashes `input` into `output` (SHA-1, SHA-256, SHA-384, SHA-512), completing with `void(boost::system::error_code)`. */
template <class Digest, class Backend = default_async_backend<Digest>, class Executor, class CompletionToken>
auto async_digest(Executor const& executor, std::span<std::uint8_t> output,
                  std::span<std::uint8_t const> input, CompletionToken&& token);

/** HMAC of `input` under `key` over `Hmac` (SHA-1, SHA-256, SHA-384, SHA-512). */
template <class Hmac, class Backend = default_async_backend<macs::hmac<Hmac>>, class Executor, class CompletionToken>
auto async_hmac(Executor const& executor, std::span<std::uint8_t> output, std::span<std::uint8_t const> key,
                std::span<std::uint8_t const> input, CompletionToken&& token);

/** HKDF (RFC 5869), extract and expand. */
template <class Hmac, class Backend = default_async_backend<kdfs::hkdf, Hmac>, class Executor, class CompletionToken>
auto async_hkdf(Executor const& executor, std::span<std::uint8_t> output, std::span<std::uint8_t const> salt,
                std::span<std::uint8_t const> ikm, std::span<std::uint8_t const> info, CompletionToken&& token);

/** PBKDF2 (RFC 8018). */
template <class Hmac, class Backend = default_async_backend<kdfs::pbkdf2, Hmac>, class Executor, class CompletionToken>
auto async_pbkdf2(Executor const& executor, std::span<std::uint8_t> output, std::span<std::uint8_t const> password,
                  std::span<std::uint8_t const> salt, std::uint64_t iterations, CompletionToken&& token);

}  // namespace webcpp::hash
```

### 11.1 The model

Each function is an Asio asynchronous operation in webcpp::asio's model: it initiates through
`async_initiate`, so that one call works with a callback, `use_awaitable` (`co_await` in Asio),
Cobalt's `use_op` or `use_task` (`co_await` in Cobalt), `use_future` and every other completion
token; on Emscripten its WebCrypto call is a JavaScript promise awaited through
`webcpp::asio::browser::async_await(executor, promise, token)` (libs/asio's
`<webcpp/asio/browser/async_await.hpp>`, whose completion is `void(error_code, emscripten::val)`),
whose bridge keeps the operation alive until the promise settles, honours the handler's
associated allocator, executor and cancellation slot, and only posts completions to the event
loop. The spans are the caller's until completion, as Asio's buffers are. The same guards as the
synchronous function (2.8, 2.9) run before initiation and complete with their error, posted,
never inline. A rejected promise is `error::backend_failure`; a cancelled operation
`asio::error::operation_aborted`, with `output` zeroed.

### 11.2 Where the family runs, and its default

- **The cheap functions** (`async_digest`, `async_hmac`, `async_hkdf`) exist on every target,
  and only for the algorithms WebCrypto serves (SHA-1, SHA-256, SHA-384, SHA-512, HMAC and HKDF
  over them). Off Emscripten, and on Emscripten with a synchronous backend named, the operation
  runs the synchronous backend at once and completes through `asio::post` on the handler's
  associated executor (never inside the initiating call). Decided so that code written once
  against the browser's family builds and runs everywhere; they cost microseconds, so a post
  blocks nothing. No other cheap function gets an `async_` variant: off the browser it would add
  a post and nothing else.
- **`async_pbkdf2` is heavy** (11.5): on Emscripten its default is WebCrypto, with or without
  threads; with a synchronous backend it runs on another thread, and exists so only where the
  build has threads.
- **`default_async_backend`** is `backends::webcrypto` on Emscripten for every algorithm it
  serves (design D33 does not change it): the asynchronous family exists for the browser; elsewhere,
  and for an algorithm it does not serve, it is `default_backend<Algorithm, Hmac>`.
- **The synchronous functions never use `backends::webcrypto`:** naming it in `hkdf`, `pbkdf2`,
  `digest` or `mac` fails to compile with "webcpp::hash: backends::webcrypto is asynchronous: call
  async_hkdf (or async_pbkdf2, async_digest, async_hmac) with a completion token, or name a
  synchronous backend." It is never a synchronous function's default (4.1).
- `<webcpp/hash/async.hpp>` needs webcpp::asio (`<library>/webcpp/asio//asio`), which the
  convenience header does not bring (chapter 3: an optional heavy dependency, named in its
  comment); `backends::webcrypto` needs nothing else on Emscripten and names no C library.

### 11.5 The heavy functions on another thread (the owner's decision, 2026-11-14)

The deliberately slow functions get asynchronous variants that truly run on another thread, and
only when the build has threads: `async_argon2id`, `async_argon2` (increment 2 on),
`async_scrypt` and `async_pbkdf2` (with a synchronous backend), and every later memory-hard or
deliberately slow function (Botan's bcrypt and bcrypt-pbkdf). Separate from the browser's family,
which exists on Emscripten with or without threads.

```cpp
/** Argon2id on a thread of `work`'s context; completes with `void(boost::system::error_code)`. */
template <class Backend = backends::sodium, class WorkExecutor, class CompletionToken>
auto async_argon2id(WorkExecutor const& work, std::span<std::uint8_t> output,
                    std::span<std::uint8_t const> password, std::span<std::uint8_t const> salt,
                    argon2id_parameters const& parameters, CompletionToken&& token);
// async_scrypt, async_argon2<Variant, Backend>, async_pbkdf2<Hmac, Backend> alike.
```

- **Threads detected by `config.hpp`.** webcpp's `<webcpp/config.hpp>` defines no threading macro
  (its macros are `WEBCPP_TARGET_*`, `WEBCPP_HAS_EXCEPTIONS`, `WEBCPP_NO_EXCEPTIONS`), so
  `config.hpp` defines `WEBCPP_HASH_HAS_THREADS` natively when Boost.Config's `BOOST_HAS_THREADS`
  is defined (b2's `threading=multi` compiles with `-pthread`, which defines `_REENTRANT`; MSVC's
  `/MT` and `/MD` define `_MT`), and on Emscripten when `__EMSCRIPTEN_PTHREADS__` is defined
  (`-pthread`). Never on wasip2 (no threads) or wasip3 (its cooperative threads do not run in
  parallel, so a heavy function there would still block its caller). A developer may define
  `WEBCPP_HASH_NO_THREADS` to turn the heavy variants off in a build that has threads.
- **Where the work runs.** The function guards first (errors complete through a post, as 11.1),
  then posts the work to `work`, an executor whose context runs on other threads (a
  `webcpp::asio::thread_pool`'s, typically), and completes on the handler's associated executor
  (Asio's default: `work` itself; a program binds its own event loop's with `bind_executor`). On
  Emscripten with pthreads each thread of the pool is a Web Worker, so the page never blocks, and
  the completion reaches the page's thread through the handler's executor
  (`webcpp::asio::browser::main_thread_context`'s, when bound). The spans and the parameters are
  the caller's until completion; cancellation before the work starts completes with
  `operation_aborted`, after it starts the work runs to its end (no C library here can be
  interrupted) and then completes with `operation_aborted`, `output` zeroed.
- **Without threads they do not exist:** each name is then a template whose `static_assert`
  says "webcpp::hash: async_argon2id runs on another thread and needs a build with threads
  (natively b2 threading=multi; on Emscripten -pthread). Without threads, call argon2id." (and
  the same for each), so a call fails to compile with the instruction; `async_pbkdf2` with
  `backends::webcrypto` on Emscripten is not refused.
- **Alternatives:** completing heavy functions through a post without threads (rejected by the
  owner: it blocks the event loop for seconds under an asynchronous name); running them on a
  thread created per call (a thread per call, no bound, and no executor of the program's).

### 11.3 The alternatives the owner rejected

- **JSPI or Asyncify suspension under the synchronous call:** a suspension per call, a larger
  program under Asyncify (against the backend's purpose, the smallest binary), and calls that
  cannot run where no promise may be awaited (an audio worklet, a build without a suspension
  variant).
- **WebCrypto as an oracle only:** keeps no browser backend; rejected as the only use, and kept as
  the test of 7.4, which compares every algorithm with the browser's whatever the backend.

### 11.4 Tests

The heavy variants: natively with `threading=multi` and on Emscripten with `-pthread` (under
node, whose workers are Web Workers' equivalent), `async_argon2id`, `async_scrypt` and
`async_pbkdf2` with a synchronous backend run on a thread of the pool other than the caller's
(the thread id recorded in the work and compared), give RFC 7914's and libsodium's results, and
keep the caller's event loop responsive (a timer of 10 ms on the caller's loop fires while an
Argon2id of 64 MiB runs); without threads, a message test per name states 11.5's text.

The async family's tests run on every target: on Emscripten under node, every function on
`backends::webcrypto` against RFC 4231 (HMAC), RFC 5869, RFC 6070 and RFC 7914 section 11, and
against `backends::hash2` on random cases, with a callback, `use_awaitable` and `use_future`,
cancellation through a cancellation slot, and a counting allocator associated with the handler
(the operation's storage is the handler's); natively and on WASI the same calls complete through
`post`, checked to never complete inside the initiating call.
