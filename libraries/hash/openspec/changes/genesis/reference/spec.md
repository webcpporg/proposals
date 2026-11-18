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
- The decisions of 2026-11-17 (design D34 to D36), before increment 2's first task: a tag of the
  families that Boost.Hash2 lacks defaults to the first of `backends::hash2`, `backends::xkcp`,
  `backends::blake3` and `backends::sodium` that offers it, and `backends::bearssl`,
  `backends::openssl`, `backends::botan` and `backends::webcrypto` are never defaults, only named
  in the type (one stated rule, no measurement: the answer to design O3; section 4.1);
  `default_async_backend<X>` is `default_backend<X>` on every target, and `backends::webcrypto`
  serves the asynchronous family on Emscripten only when the type names it (section 11.2); and a
  measured table, the allocation counts included, is built only from release builds (sections 4.2
  and 7.7).

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

**As built (2026-11-18).** Increments 2, 3, 4, 5 and 7 are built, each reviewed, on the
integration branches of the library and the superproject (`../tasks.md` groups 2 to 5 and 7).
Increment 2 also brought the families over increment 1's backends: Boost.Hash2's algorithms on
`backends::hash2` (SHAKE128 and SHAKE256 whole, design D37; BLAKE2b with its 64-byte output and no
option, keyed BLAKE2s with its 32-byte tag; SipHash-2-4), `digests::keccak_256` on `backends::xkcp`
and `digests::blake3` on `backends::blake3`. Increment 4 also brought OpenSSL's SipHash-2-4 and
-2-4x, keyed BLAKE2b and BLAKE2s, MD5-SHA1, HMAC over Keccak-224 to -512 and SM3, and Argon2id and
scrypt for the functions `argon2id` and `scrypt`. Increment 6 is not built: Botan is not
provisioned (design D42).

## 2. Interface

### 2.1 Names

| What | Name |
| --- | --- |
| namespace | `webcpp::hash`, `webcpp::hash::backends`, `webcpp::hash::kdfs`, `webcpp::hash::digests`, `webcpp::hash::macs` (the families' tags, 2.15); `webcpp::hash::detail` for what is not public |
| headers | `<webcpp/hash/...>`, one per responsibility; the convenience header `<webcpp/hash.hpp>` |
| configuration | `<webcpp/hash/config.hpp>`, first in every public header; it includes `<webcpp/config.hpp>` first |
| macros | `WEBCPP_HASH_*`; test macros `WEBCPP_TEST_*` |
| include guards | `WEBCPP_HASH_<PATH>_HPP`; `WEBCPP_HASH_DETAIL_CANARY_H` for the C header |
| b2 targets | `/webcpp/hash//hash` (the library), `/webcpp/hash//openssl` (OpenSSL, spec 5.6) |
| C libraries | `/third_party//xkcp`, `/third_party//blake3`, `/third_party//sodium`, `/third_party//bearssl` |
| test data | `/third_party//blake3_vectors`, `/third_party//blake2_vectors`, `/third_party//sodium_vectors`, `/third_party//xkcp_vectors`, `/third_party//openssl_evp_data` (7.1) |
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
// The definitions the reference lists (chapter 6): the first two undone at once; each
// WEBCPP_HASH_HAS_<NAME> left defined, since doc/Jamfile links every library; THREADS as a
// native build has it, NO_THREADS undone.
#define WEBCPP_HASH_NO_EXCEPTIONS
#undef WEBCPP_HASH_NO_EXCEPTIONS
#define WEBCPP_HASH_WEBASSEMBLY 1
#undef WEBCPP_HASH_WEBASSEMBLY
#define WEBCPP_HASH_HAS_XKCP 1
#define WEBCPP_HASH_HAS_BLAKE3 1
#define WEBCPP_HASH_HAS_SODIUM 1
#define WEBCPP_HASH_HAS_BEARSSL 1
#define WEBCPP_HASH_HAS_OPENSSL 1
#define WEBCPP_HASH_HAS_THREADS 1
#define WEBCPP_HASH_NO_THREADS
#undef WEBCPP_HASH_NO_THREADS
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_HASH_NO_EXCEPTIONS)
#define WEBCPP_HASH_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3) || \
    defined(WEBCPP_TARGET_EMSCRIPTEN)
#define WEBCPP_HASH_WEBASSEMBLY 1
#endif

// Threads that run in parallel (11.5): natively Boost.Config's BOOST_HAS_THREADS; on Emscripten
// -pthread; never WASI, whose wasip3 threads are cooperative (BOOST_HAS_THREADS is defined there
// too, so the target decides first).
#if !defined(__MRDOCS__) && !defined(WEBCPP_HASH_NO_THREADS) &&       \
    ((defined(WEBCPP_TARGET_NATIVE) && defined(BOOST_HAS_THREADS)) || \
     (defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(__EMSCRIPTEN_PTHREADS__)))
#define WEBCPP_HASH_HAS_THREADS 1
#endif

// The refusals of impossible combinations (spec 5.7).
// clang-format off
#if defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(WEBCPP_HASH_HAS_OPENSSL)
#error "webcpp::hash: OpenSSL is not built for Emscripten. In the program's Jamfile, remove /webcpp/hash//openssl: hkdf and pbkdf2 then run on backends::hash2."
#endif
// clang-format on
// No profile and no override define is read (spec 4.1, design D33).

namespace webcpp::hash::backends {

/** webcpp's own HKDF and PBKDF2 over Boost.Hash2's `hmac<H>`, on every target. */
struct hash2;
/** libsodium, built from its sources by `/third_party//sodium`. */
struct sodium;
/** BearSSL, built from its sources by `/third_party//bearssl`. */
struct bearssl;
/** OpenSSL 3.5, built from its pinned sources, named by `/webcpp/hash//openssl`. */
struct openssl;
/** XKCP, built from its sources by `/third_party//xkcp`: the Keccak functions of the families. */
struct xkcp;
/** BLAKE3's official C implementation, built by `/third_party//blake3`: `digests::blake3`. */
struct blake3;
/** WebCrypto (Emscripten only), a backend of the asynchronous family alone, never a default. */
struct webcrypto;

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
/** Argon2id, RFC 9106, with one lane: `argon2id`; and the variant of the family `argon2`. */
struct argon2id;
/** scrypt, RFC 7914: `scrypt`. */
struct scrypt;
// The families' tags (2.15): argon2d, argon2i, bcrypt, bcrypt_pbkdf, blake2b_subkey, kbkdf_hmac,
// sskdf, x963kdf; each declared and never defined.

}  // namespace webcpp::hash::kdfs

// namespace webcpp::hash::digests and namespace webcpp::hash::macs: the tags of 2.15, each declared
// and never defined, every tag whose default backend section 4.1 names, whether or not a backend
// serves it yet.

#endif  // WEBCPP_HASH_CONFIG_HPP
```

`WEBCPP_HASH_NO_EXCEPTIONS` exists as every library's does (chapter 6); webcpp::hash raises no
exception of its own, and its Doc Comment says so. `config_test.cpp` is built four times
(`config`, `config_no_exceptions`, `config_all_off`, `config_developer_defined`), and
`config_threads_test.cpp` four times for `WEBCPP_HASH_HAS_THREADS` (`config_single`;
`config_threads`, `<threading>multi` natively and on Emscripten; `config_threads_wasip3`, which
is multi-threaded and still has none; `config_no_threads`, with `WEBCPP_HASH_NO_THREADS`).
`backends::webcrypto` is declared on every target, so that a synchronous function that names it
refuses with 11.2's text natively too (design D39).

### 2.3 `<webcpp/hash/error.hpp>`

```cpp
namespace webcpp::hash {

/** Why an operation of webcpp::hash failed; the values are fixed and never reused. */
enum class error : int {
    /** An output that is not of a size the function produces: empty, too short or too long. */
    invalid_output_size = 1,
    /** A key that is not of a size the function accepts: a MAC's, a derivation's, HKDF's PRK. */
    invalid_key_size = 2,
    /** A salt that is not of a size the function accepts. */
    invalid_salt_size = 3,
    /** An iteration count of zero, or above the backend's limit (`backend_limits`). */
    invalid_iterations = 4,
    /** A parameter that is outside the function's limits: a cost, a lane count, an option. */
    invalid_parameters = 5,
    /** An input that is longer than the function accepts. */
    input_too_long = 6,
    /** A backend failed where its contract says it does not. */
    backend_failure = 7,
    /** A parameter the chosen backend does not take; name a backend that does. */
    unsupported_by_backend = 8,
    /** A `hasher` used after its `finish`, once moved from, or after a backend failure spent it. */
    invalid_state = 9,
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
| `invalid_output_size` | "the output is not of a size the function produces" |
| `invalid_key_size` | "the key is not of a size the function accepts" |
| `invalid_salt_size` | "the salt is not of a size the function accepts" |
| `invalid_iterations` | "the iteration count is zero or more than the backend takes" (increment 7: a count above `backend_limits<Backend>::pbkdf2_iterations`, 2^32 - 1 on `backends::webcrypto`, is `invalid_iterations` too, 2.9) |
| `invalid_parameters` | "a parameter is outside the function's limits" |
| `input_too_long` | "an input is longer than the function accepts" |
| `backend_failure` | "the backend failed where it should not" |
| `unsupported_by_backend` | "the backend does not take this parameter; name a backend that does" |
| `invalid_state` | "the hasher was finished or moved from" |

A value outside the enumeration reads "unknown webcpp.hash error". The families of increment 2
(2.15) reworded the messages of values 1, 2 and 5, whose meaning they widened (an output too short
for its options, a MAC's key, any option), and added value 9; no value changed (design D38).

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
`pbkdf2_output`, `pbkdf2_iterations` (from increment 7, `invalid_iterations` beyond it),
`argon2id_*`, `scrypt_*`); the primary template, every backend's unless its
header specialises it, has no limit (`detail::no_limit`). The public functions read it after the
spec's own guards and before any call of the backend. `backends::openssl` limits HKDF's salt and
PRK (HMAC's key, an `int` in `HMAC_Init_ex`) and PBKDF2's password, salt and output (`int`s in
`pbkdf2_derive`) to `INT_MAX`: OpenSSL 3.5.9 holds them in an `int`, and a PBKDF2 output of 2^31
bytes crashed it (SIGSEGV, a negative length reaching `memcpy`). `backends::hash2` accepts these
lengths: this is the one place where a call can succeed on one backend and be refused on another,
a refusal at a size no protocol uses, never a different result. `backends::webcrypto` (section 11)
declares WebCrypto's: an HKDF or PBKDF2 output of at most (2^32 - 1) / 8 bytes (`deriveBits`
takes its length in bits as an `unsigned long`), at most 2^32 - 1 PBKDF2 iterations, and an HKDF
info of at most 1,024 bytes, node's limit (`input_too_long`), documented as WebCrypto's limit as
D26 documents OpenSSL's (design D39).

The guards of each function, without its work, are a function of their own in `detail`
(`hkdf_guards`, `pbkdf2_guards`, `argon2id_guards`, `argon2_guards`, `scrypt_guards`,
`digest_guards`, `mac_guards`), which the synchronous function and its asynchronous variant
(section 11) both run, so that the two refuse alike.

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
`input_too_long`, and the output past `INT_MAX`, `invalid_output_size`; on WebCrypto more than
2^32 - 1 iterations, `invalid_iterations`); a backend's failure zeroes `output`. OpenSSL is called
with `pkcs5 = 1`, which turns off SP 800-132's lower bounds, so that every RFC 6070 case runs on
every backend (design D10).

### 2.10 `<webcpp/hash/argon2.hpp>`: Argon2id

```cpp
namespace webcpp::hash {

/**
 The costs of Argon2id with one lane, as libsodium's crypto_pwhash takes them, for `argon2id`. Its
 memory is in bytes; `argon2_parameters`, the family's (2.15), counts KiB as RFC 9106 does.
*/
struct argon2id_parameters {
    /** The number of passes over the memory, t: 1 to 4,294,967,295. */
    std::uint64_t passes;
    /** The memory in bytes, m times 1,024: at least 8,192, a multiple of 1,024, at most 4 TiB less 1 KiB (2 GiB on wasm32). */
    std::size_t memory;
};

/** libsodium's interactive costs: 2 passes over 64 MiB. */
inline constexpr argon2id_parameters argon2id_interactive{.passes = 2, .memory = 67108864};
/** libsodium's moderate costs: 3 passes over 256 MiB. */
inline constexpr argon2id_parameters argon2id_moderate{.passes = 3, .memory = 268435456};
/** libsodium's sensitive costs: 4 passes over 1 GiB. */
inline constexpr argon2id_parameters argon2id_sensitive{.passes = 4, .memory = 1073741824};

/** The salt's size, 16 bytes. */
inline constexpr std::size_t argon2id_salt_size = 16;

/**
 Argon2id version 0x13 (RFC 9106) with one lane, no secret and no associated data: fills
 `output` from `password` and `salt`.

 @tparam Backend The backend; by default `default_backend<kdfs::argon2id>`, `backends::sodium`;
 or `backends::openssl`, named (increment 4).
 @param output The tag, 16 to 4,294,967,295 bytes; it must not overlap the inputs.
 @param password The password, at most 4,294,967,295 bytes.
 @param salt The salt, of `argon2id_salt_size` bytes.
 @param parameters The costs.
 @return Success, or the error of the first guard that fails, `output` untouched; or
 `errc::not_enough_memory` when the memory cannot be allocated, or `backend_failure`, `output`
 zeroed.
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
8,192, not a multiple of 1,024 (libsodium would silently round it down) or above the engine's
`argon2id_limits::memory` (`invalid_parameters`): libsodium's `crypto_pwhash_argon2id_MEMLIMIT_MAX`;
on `backends::openssl`, m of at most 2^32 - 1 KiB within a `size_t` (4 TiB less 1 KiB natively,
4 GiB less 1 KiB on wasm32); then the backend's `detail::backend_limits` (2.8). The three presets
are checked against libsodium's `OPSLIMIT_*` and `MEMLIMIT_*` macros by `static_assert` in
`backends/sodium.hpp`. The backend allocates the memory (spec 6.2). On `backends::openssl`,
`argon2id` runs OpenSSL's "ARGON2ID" with one lane and no secret or associated data, the same
function as libsodium's (increment 4, ruling R43).

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

 @tparam Backend The backend; by default `default_backend<kdfs::scrypt>`, `backends::sodium`; or
 `backends::openssl`, named (increment 4).
 @param output The derived key, 1 to (2^32 - 1) times 32 bytes; it must not overlap the inputs.
 @param password The password. @param salt The salt.
 @param parameters The costs.
 @return Success, or the error of the first guard that fails, `output` untouched; or
 `errc::not_enough_memory` when the 128 r N bytes cannot be allocated, or `backend_failure`,
 `output` zeroed.
*/
template <class Backend = default_backend<kdfs::scrypt>>
[[nodiscard]] boost::system::result<void> scrypt(std::span<std::uint8_t> output,
                                   std::span<std::uint8_t const> password,
                                   std::span<std::uint8_t const> salt,
                                   scrypt_parameters const& parameters);

}  // namespace webcpp::hash
```

Guards: `output` empty or past the limit (`invalid_output_size`); RFC 7914's conditions on N, r
and p, and 128 r (N + p) bytes that do not fit `size_t` (`invalid_parameters`); then the engine's
`scrypt_limits` (`invalid_parameters`: on `backends::sodium` an N above 2^32 - 1, on
`backends::openssl` an r p of 2^24 or more) and the backend's `detail::backend_limits` (2.8: on
OpenSSL a password, salt or output of more than `INT_MAX` bytes). On libsodium the call is
`crypto_pwhash_scryptsalsa208sha256_ll(password, salt, N, r, p, output)`; on OpenSSL the
`EVP_KDF` "SCRYPT" with `maxmem_bytes` at its largest (increment 4, ruling R43).

### 2.12 The backends' headers

| Header | Declares | Needs |
| --- | --- | --- |
| `<webcpp/hash/backends/hash2.hpp>` | `backends::hash2` and its HKDF and PBKDF2 engines; the families' engines over Boost.Hash2's algorithms: MD5, SHA-1, SHA-2, SHA-3, SHAKE128/256 (any output size), RIPEMD-160, BLAKE2s-256, BLAKE2b (64 bytes, no option), HMAC over each fixed-size digest and BLAKE2b-512, SipHash-2-4, keyed BLAKE2s (a 32-byte tag) | nothing |
| `<webcpp/hash/backends/xkcp.hpp>` | `backends::xkcp`: Keccak-224 to -512, SHA-3, SHAKE, cSHAKE, TupleHash, ParallelHash, KT128, KT256, TurboSHAKE and KMAC through webcpp's adapter (increments 2 and 3) | `/third_party//xkcp` |
| `<webcpp/hash/backends/blake3.hpp>` | `backends::blake3`: `digests::blake3` over webcpp's `blake3` | `/third_party//blake3` |
| `<webcpp/hash/backends/sodium.hpp>` | `backends::sodium`: HKDF engines for `hmac_sha2_256` and `hmac_sha2_512`, the Argon2id and scrypt engines, the presets' checks; BLAKE2b with its options, SipHash-2-4 and -2-4x, `blake2b_subkey`, SHA-256, SHA-512, SHA3-256, SHA3-512, SHAKE, TurboSHAKE, HMAC-SHA-256 and -512, Argon2i and Argon2id of the family `argon2` (increment 2) | `/third_party//sodium` |
| `<webcpp/hash/backends/bearssl.hpp>` | `backends::bearssl`: the `hkdf` engine for six hmacs; MD5, SHA-1, SHA-224 to SHA-512, MD5-SHA1, SHAKE and HMAC over the six (increment 5) | `/third_party//bearssl` |
| `<webcpp/hash/backends/openssl.hpp>` | `backends::openssl`: HKDF and PBKDF2 engines; every digest of the default provider that has a tag, HMAC over them, KMAC, SipHash-2-4 and -2-4x, keyed BLAKE2b and BLAKE2s, scrypt, Argon2id and the family `argon2`, `kbkdf`, `sskdf` and `x963kdf` (increment 4) | `/webcpp/hash//openssl` |
| `<webcpp/hash/backends/webcrypto.hpp>` | `backends::webcrypto`'s operations and `detail::backend_limits<backends::webcrypto>` (section 11; on Emscripten; elsewhere the limits alone) | webcpp::asio |

A backend's header that needs a library checks its `WEBCPP_HASH_HAS_<NAME>` first and stops with
the Jamfile line (spec 5.7). Each engine is a specialisation, in `webcpp::hash::detail`, of

```cpp
template <class Backend, class Hmac> struct hkdf_engine;      // extract, expand, extract_and_expand
template <class Backend, class Hmac> struct pbkdf2_engine;    // derive
template <class Backend> struct argon2id_engine;              // derive, limits
template <class Backend> struct scrypt_engine;                // derive, limits
template <class Backend> struct blake2b_subkey_engine;        // derive
template <class Backend, class Digest> struct digest_engine;  // the families' contract below
template <class Backend, class Mac> struct mac_engine;        // accepts, compute
template <class Backend, class Variant> struct argon2_engine; // limits, derive
template <class Backend, class Hmac> struct kbkdf_engine;     // limits, derive
template <class Backend, class Digest> struct sskdf_engine;   // limits, derive
template <class Backend, class Digest> struct x963kdf_engine; // limits, derive
```

whose primary templates hold the `static_assert` that refuses a backend without the function
(spec 3.3). The public functions do every guard of 2.8 to 2.11 and 2.15 once; an engine does only
the work, and is called only with valid arguments.

A `digest_engine` specialisation, written once per tag a backend has, gives, each `noexcept`:
`state` (default-constructible, nothrow movable; a moved-from state owns nothing), `min_output` and
`max_output` (the backend's narrower outputs, 0 and `no_limit` when they are the family's),
`accepts(options)`, `init(state, options)`, `update(state, input)`, `finish(state, output)`
(called once per `init`), `destroy(state)` (wipes the state and frees what it owns, called once
after every `init` and on each state moved from) and `one_shot(output, input, options)`. A
`mac_engine` gives `accepts(options, key_size, output_size)` and `compute(output, key, input,
options)`; an `argon2_engine` gives `limits` (`detail::argon2_limits`: the fewest passes, the most
memory in KiB and lanes, the shortest tag and salt, the longest salt, whether a secret and
associated data are taken) and `derive`. `detail::full_engine` gives the members of an engine
without narrower limits. What the family accepts and an engine does not is
`unsupported_by_backend`, before any call (ruling R41). A failure is `backend_failure`, or
`errc::not_enough_memory` when the library could not allocate; the public function then zeroes
the output, and a `hasher` is spent.

### 2.13 The convenience header

`<webcpp/hash.hpp>` includes `config.hpp`, `error.hpp`, `algorithms.hpp`, `hmac.hpp`, `kdf.hpp`,
`hkdf.hpp`, `pbkdf2.hpp`, `digest.hpp`, `mac.hpp`, `argon2.hpp`, `blake2b_subkey.hpp`,
`kdf_standards.hpp` and `backends/hash2.hpp`, which need no C library (a function of `argon2.hpp`,
`blake2b_subkey.hpp` or `kdf_standards.hpp` called without its library refuses at compile time with
the Jamfile line to add), and `keccak.hpp` with `backends/xkcp.hpp`, `blake3.hpp` with
`backends/blake3.hpp`, the other backends' headers, and `scrypt.hpp` (when libsodium or OpenSSL is
linked) only when their `WEBCPP_HASH_HAS_<NAME>` is defined. It leaves out `async.hpp` and
`backends/webcrypto.hpp`, which bring webcpp::asio (section 11). It names in its comment what it
leaves out and why (CODING-GUIDELINES.md chapter 3). A program that links no C library includes
it and compiles.

### 2.14 What is not public

`detail/canary.h` (spec 5.4), `detail/wipe.hpp` (stores of zero through `volatile` over a span
and over a trivially copyable object), `detail/limits.hpp` (`exceeds`, `sp800_185_max_bytes`),
`detail/engines.hpp` (the primary templates of 2.12, `linked_v`, `detail::no_default`,
`needs_library` and each tag's refusal of 3.3), `detail/xkcp.hpp` (the adapter's declarations),
the families' `detail::digest_traits` and `detail::mac_traits` (each tag's guards, in one place),
`detail::digest_tag_of<Hmac>` (the tag of `digests` of a Boost.Hash2 hmac) and the guards of each
function (2.8). Until 2026-11-16 it also held the generated `detail/ranking.hpp` and
`detail/overrides.hpp`, retired by design D33.

### 2.15 The families of increments 2 to 7

Increment 1's functions keep their form. From increment 2 on, an algorithm whose result does not
fit Hash2's HashAlgorithm contract (a single finalisation, parameters, a key) is reached through
four families, each a template over the algorithm's tag and the backend, with the same guards
in one place and the same engines-per-backend design (2.12), and, beside them, the key
derivations of one backend each (`blake2b_subkey`, `kbkdf`, `sskdf`, `x963kdf`). The headers are
`digest.hpp` (`digest_options`, `digest_of`, `hasher`), `mac.hpp` (`mac_options`, `mac`),
`argon2.hpp` (`argon2_parameters`, `argon2`, beside `argon2id`), `blake2b_subkey.hpp` and
`kdf_standards.hpp`; the tags are declared in `config.hpp`.

```cpp
namespace webcpp::hash {

namespace digests {  // the tag of each hash and XOF: md5, sha1, sha2_224, sha2_256, sha2_384, sha2_512,
                     // sha2_512_224, sha2_512_256, sha3_224, sha3_256, sha3_384, sha3_512, shake_128,
                     // shake_256, keccak_224 .. keccak_512, cshake_128, cshake_256, kt128, kt256,
                     // turboshake_128, turboshake_256, tuplehash_128, tuplehash_256, parallelhash_128,
                     // parallelhash_256, blake2b, blake2s_256, blake3, ripemd_160, md5_sha1, sm3,
                     // streebog_256, streebog_512, whirlpool, skein_512
}
namespace macs {     // hmac<Digest> (over a tag of digests), kmac_128, kmac_256, siphash_2_4, siphash_2_4x,
                     // blake2b_mac, blake2s_mac
}
namespace kdfs {     // hkdf, hkdf_extract, hkdf_expand, pbkdf2, argon2id, scrypt (increment 1), argon2d,
                     // argon2i, bcrypt, bcrypt_pbkdf, blake2b_subkey, kbkdf_hmac, sskdf, x963kdf
}

/** The parameters of a digest: none for most tags, which the primary template serves. */
template <class Digest> struct digest_options {};
template <> struct digest_options<digests::blake2b> {
    std::span<std::uint8_t const> key = {};              // 0 to 64 bytes (invalid_key_size)
    std::span<std::uint8_t const> salt = {};             // empty or 16 bytes (invalid_salt_size)
    std::span<std::uint8_t const> personalisation = {};  // empty or 16 bytes (invalid_parameters)
    std::size_t output_size = 64;                        // 1 to 64 (invalid_output_size)
};
template <> struct digest_options<digests::cshake_128> {  // and cshake_256
    std::span<std::uint8_t const> function_name = {};    // N, empty unless NIST defines the function
    std::span<std::uint8_t const> customisation = {};    // S
};
template <> struct digest_options<digests::tuplehash_128> {  // and tuplehash_256
    std::span<std::uint8_t const> customisation = {};    // S
    bool xof = false;                                    // TupleHashXOF128 in place of TupleHash128
};
template <> struct digest_options<digests::parallelhash_128> {  // and parallelhash_256
    std::size_t block_size = 0;                          // B: no default (0 is invalid_parameters)
    std::span<std::uint8_t const> customisation = {};    // S
    bool xof = false;                                    // ParallelHashXOF128
};
template <> struct digest_options<digests::kt128> {  // and kt256
    std::span<std::uint8_t const> customisation = {};    // C, read at finish by a hasher
};
template <> struct digest_options<digests::turboshake_128> {  // and turboshake_256
    std::uint8_t domain = 0x1F;                          // D, 0x01 to 0x7F (invalid_parameters)
};

/** Hashes `input` into `output`, whose size is the digest's (or any size from 1 byte, for an XOF). */
template <class Digest, class Backend = default_backend<Digest>>
[[nodiscard]] boost::system::result<void> digest_of(std::span<std::uint8_t> output,
                                                    std::span<std::uint8_t const> input,
                                                    digest_options<Digest> const& options = {});

/** An incremental hash with one finalisation: `make`, `update`, `finish`. Move-only; wiped. */
template <class Digest, class Backend = default_backend<Digest>>
class hasher {
public:
    [[nodiscard]] static boost::system::result<hasher> make(
        digest_options<Digest> const& options = {}) noexcept;
    [[nodiscard]] boost::system::result<void> update(std::span<std::uint8_t const> input) noexcept;
    [[nodiscard]] boost::system::result<void> finish(std::span<std::uint8_t> output) noexcept;
};

/** The parameters of a MAC: none for most tags, which the primary template serves. */
template <class Mac> struct mac_options {};
template <> struct mac_options<macs::kmac_128> {  // and kmac_256
    std::span<std::uint8_t const> customisation = {};    // S
    bool xof = false;                                    // KMACXOF128 in place of KMAC128
};

/** The tag of `input` under `key`. */
template <class Mac, class Backend = default_backend<Mac>>
[[nodiscard]] boost::system::result<void> mac(std::span<std::uint8_t> output,
                                              std::span<std::uint8_t const> key,
                                              std::span<std::uint8_t const> input,
                                              mac_options<Mac> const& options = {});

/** Every parameter of Argon2, RFC 9106 (increment 2 on libsodium, one lane; every one from 4). */
struct argon2_parameters {
    std::uint32_t passes;                                // t: at least 1
    std::uint32_t memory_kib;                            // m, in KiB: at least 8 times the lanes
    std::uint32_t lanes = 1;                             // p: 1 to 16,777,215
    std::span<std::uint8_t const> secret = {};           // K: at most 4,294,967,295 bytes
    std::span<std::uint8_t const> associated_data = {};  // X: at most 4,294,967,295 bytes
};
template <class Variant, class Backend = default_backend<Variant>>
[[nodiscard]] boost::system::result<void> argon2(std::span<std::uint8_t> output,
                                                 std::span<std::uint8_t const> password,
                                                 std::span<std::uint8_t const> salt,
                                                 argon2_parameters const& parameters);

/** libsodium's crypto_kdf: the subkey `subkey_id` of `master_key` in `context` (ruling R22). */
inline constexpr std::size_t blake2b_subkey_min_size = 16;
inline constexpr std::size_t blake2b_subkey_max_size = 64;
inline constexpr std::size_t blake2b_subkey_key_size = 32;
inline constexpr std::size_t blake2b_subkey_context_size = 8;
template <class Backend = default_backend<kdfs::blake2b_subkey>>
[[nodiscard]] boost::system::result<void> blake2b_subkey(std::span<std::uint8_t> output,
                                                         std::span<std::uint8_t const> master_key,
                                                         std::uint64_t subkey_id,
                                                         std::span<std::uint8_t const> context);

/** SP 800-108's fixed input, by default Label || 0x00 || Context || [L]_32 (ruling R40). */
struct kbkdf_options {
    std::span<std::uint8_t const> label = {};
    std::span<std::uint8_t const> context = {};
    std::uint32_t counter_bits = 32;                     // r: 8, 16, 24 or 32
    bool separator = true;                               // the byte 0x00 between label and context
    bool length = true;                                  // L in bits, 4 bytes big-endian, at the end
};
template <class Hmac, class Backend = default_backend<kdfs::kbkdf_hmac, Hmac>>
[[nodiscard]] boost::system::result<void> kbkdf(std::span<std::uint8_t> output,
                                                std::span<std::uint8_t const> key,
                                                kbkdf_options const& options = {});
template <class Digest, class Backend = default_backend<kdfs::sskdf>>
[[nodiscard]] boost::system::result<void> sskdf(std::span<std::uint8_t> output,
                                                std::span<std::uint8_t const> secret,
                                                std::span<std::uint8_t const> fixed_info = {});
template <class Digest, class Backend = default_backend<kdfs::x963kdf>>
[[nodiscard]] boost::system::result<void> x963kdf(std::span<std::uint8_t> output,
                                                  std::span<std::uint8_t const> secret,
                                                  std::span<std::uint8_t const> shared_info = {});

}  // namespace webcpp::hash
```

The one-shot hash is `digest_of`, not `digest`: `digest<N>` is Boost.Hash2's result type, which
2.4 re-exports by alias (ruling R10). Increment 2 adds to `config.hpp` the tags `backends::xkcp`
and `backends::blake3` (the libraries behind `keccak_256` and `blake3`, now backends of the
families) and every tag whose default 4.1 names; increment 7 adds `backends::webcrypto` (design
D39). `backends::botan` is increment 6's, not built (design D42).

**The guards, in one place.** `detail::digest_traits<Digest>` and `detail::mac_traits<Mac>` hold
each tag's guards; `digest_of` runs, in order: the output's size for the options
(`invalid_output_size`), the options' own guards, the engine's `min_output` and `max_output`, then
its `accepts(options)` (`unsupported_by_backend`), and only then the work. `mac` runs the tag's
size (`invalid_output_size`), the key's (`invalid_key_size`), the options', then the engine's
`accepts(options, key_size, output_size)`. A guard's refusal leaves `output` untouched; a
backend's failure zeroes it.

| Tag | Output | Options and key |
| --- | --- | --- |
| a fixed-size digest | exactly its size | none |
| `shake_*`, `blake3`, `kt128`, `kt256` | any size from 1 byte | none (KT's C: any size) |
| `blake2b` | exactly `output_size`, 1 to 64 bytes | key at most 64 bytes (`invalid_key_size`); salt empty or 16 bytes (`invalid_salt_size`); personalisation empty or 16 bytes (`invalid_parameters`), RFC 7693 (ruling R21) |
| `turboshake_*` | any size from 1 byte | D within 0x01 to 0x7F (`invalid_parameters`; RFC 9861, which neither libsodium nor XKCP checks; ruling R25) |
| `cshake_*` | any size from 1 byte | N and S each at most `SIZE_MAX / 8` bytes (`input_too_long`) |
| `tuplehash_*`, `parallelhash_*` | from 1 byte; at most `SIZE_MAX / 8` bytes in the fixed-length form | S at most `SIZE_MAX / 8` bytes (`input_too_long`); ParallelHash's B other than 0 (`invalid_parameters`), its whole input at most `SIZE_MAX / 8` bytes (`input_too_long`) |
| `macs::hmac<D>` | 10 bytes up to D's size, the digest cut (RFC 2104 section 5; ruling R23); none over an extendable-output D | key of any size |
| `kmac_*` | 4 bytes (SP 800-185 section 8.4.2) up to `SIZE_MAX / 8` | key at most `SIZE_MAX / 8` bytes (`invalid_key_size`); S at most `SIZE_MAX / 8` bytes (`input_too_long`) |
| `siphash_2_4`, `siphash_2_4x` | 8 bytes, 16 bytes | key of 16 bytes |
| `blake2b_mac`, `blake2s_mac` | 1 to 64 bytes, 1 to 32 bytes | key of 1 to 64 bytes, 1 to 32 bytes |

`SIZE_MAX / 8` (`detail::sp800_185_max_bytes`) is the most whose length in bits a `size_t` holds,
which matters on wasm32 (512 MiB).

**What the build decided for the options (design D38).** BLAKE2b's output size is an option, 64 by
default (ruling R88): BLAKE2b-256 is not BLAKE2b-512 cut to 32 bytes, and a `hasher` must know it at
`make`. SP 800-185's fixed-length forms encode the output's length L after the input, so a hasher
learns it at `finish`, where XKCP takes it at `_Initialize`: the adapter's `_final_length` writes L
at `finish`, and cSHAKE, KMAC, TupleHash and ParallelHash have no output-size option (ruling R91);
their XOF forms encode none, and a shorter output is the start of a longer. KangarooTwelve absorbs C
after the message, so a `hasher` keeps the span `make` took and reads C at `finish` (no copy, no
allocation; the caller keeps it alive and unchanged; ruling R33); on `backends::xkcp` where a
`size_t` has 32 bits (wasm32), the message and C together are at most a little under 32 TiB
(`input_too_long`, which spends the hasher), since XKCP counts the chunks in a `size_t`.
ParallelHash on `backends::xkcp` takes a block size that is a power of two of at least 8 bytes
(`unsupported_by_backend` otherwise; SP 800-185 takes any from 1). An engine's `min_output` exists
because BLAKE2b's and BLAKE2s's output size is a parameter of the hash: a partial cell offers no
cut. A partial cell refuses every option it cannot serve, so that no option is silently ignored
(ruling R86): `digest_engine<hash2, digests::blake2b>` answers `unsupported_by_backend` for a key, a
salt, a personalisation or an output under 64 bytes, and `mac_engine<hash2, macs::blake2s_mac>` for
a tag under 32 bytes.

**`hasher`.** `make` checks the options and the engine's `accepts`, then `init`; `update` absorbs;
`finish` checks the output's size for the options taken at `make` and the engine's, writes the
result once and wipes the state. An `update` or a `finish` after `finish`, on a hasher moved from,
or after a backend's failure spent it, is `error::invalid_state`. A guard's error at `finish`
leaves `output` untouched and the hasher hashing; a backend's failure zeroes `output` and spends
the hasher, as does an `input_too_long` of `update` (ParallelHash's whole input, a TupleHash
element, KangarooTwelve's input on 32-bit XKCP).

**`macs::hmac<Digest>`** is over a tag of `digests`; `detail::digest_tag_of<Hmac>` maps each
Boost.Hash2 hmac alias to its tag (ruling R15), which the asynchronous family's `async_hmac` uses.
No backend has HMAC over `digests::md5_sha1`, which no standard defines and no vector checks
(ruling R92, design D41).

**`argon2`.** Guards in order: the tag of 4 to 4,294,967,295 bytes (`invalid_output_size`); the
salt of 8 to 4,294,967,295 bytes (`invalid_salt_size`); the password, the secret and the associated
data of at most 4,294,967,295 bytes each (`input_too_long`); at least 1 pass, 1 to 16,777,215
lanes and at least 8 KiB of memory per lane (`invalid_parameters`); then the engine's
`argon2_limits` (`unsupported_by_backend`): libsodium takes one lane, no secret and no associated
data, a salt of 16 bytes, a tag of 16 bytes or more, and for Argon2i 3 passes or more (ruling
R24); OpenSSL takes every value, but m of at most 4,194,303 KiB on wasm32, where it sizes the
memory in a `size_t`. `argon2_parameters::memory_kib` counts KiB, RFC 9106's m;
`argon2id_parameters::memory` (2.10) counts bytes (ruling R16).

**`blake2b_subkey`** (ruling R22) is libsodium's `crypto_kdf_blake2b_derive_from_key`: BLAKE2b of
the empty message keyed with the master key, of the subkey's length, with `subkey_id` in 8
little-endian bytes then 8 zero bytes as the salt and the context then 8 zero bytes as the
personalisation. Guards: a subkey of 16 to 64 bytes (`invalid_output_size`), a master key of 32
bytes (`invalid_key_size`), a context of 8 bytes (`invalid_parameters`); the output must not
overlap the inputs. Its one backend, and default, is `backends::sodium`.

**`kbkdf`, `sskdf`, `x963kdf`** (`kdf_standards.hpp`; increment 4) have `backends::openssl` alone
and no default (design D34): a call names it. `kbkdf` is SP 800-108r1's counter mode over a
Boost.Hash2 hmac, which takes every hmac its backend has (MD5's, RIPEMD-160's and BLAKE2's
included, for the protocols outside NIST's approval that use them); its guards: a counter width
other than 8, 16, 24 or 32 bits (`invalid_parameters`), an output empty, longer than 2^r - 1
digests, or, with L, than 536,870,911 bytes (`invalid_output_size`), an empty key
(`invalid_key_size`), then the engine's limits (`unsupported_by_backend`). `sskdf` is SP 800-56C
revision 2's one-step KDF over a digest, each block `Digest(counter || secret || fixed_info)`, and
takes SP 800-56C's approved hashes, SHA-1, the six SHA-2 and the four SHA-3; `x963kdf` is ANSI
X9.63's (SEC 1 v2 section 3.6.1), each block `Digest(secret || counter || shared_info)`, and takes
the hash functions SEC 1 v2 section 3.5 lists and NIST SP 800-135r1 section 5.1 approves for it,
SHA-1, SHA-224, SHA-256, SHA-384 and SHA-512; another digest refuses at compile time with 3.3's
text. Their guards: an output empty or of 2^32 - 1 digests or more (`sskdf`: more than 2^32 - 1;
`invalid_output_size`), an empty secret (`invalid_key_size`), a counter, secret and info longer
than the digest hashes (`input_too_long`), then the engine's limits (`unsupported_by_backend`;
OpenSSL's `SSKDF_MAX_INLEN`, 2^30 bytes).

`hasher` and `digest_of` are not Hash2's form: a result is final, and a second `finish`, or an
`update` after it, returns `error::invalid_state`, the value 9, which increment 2 adds with the
family. The Hash2-form types (Hash2's own, `keccak_256`, `blake3`) stay the
types to use with `hash_append`; the families reach every backend's implementation of the same
algorithm, which the agreement tests compare on the first result. Each tag's per-backend engines
are specified by the increment's task in the plan, from the backend's own documentation at its pin;
the capability table of 3.5 says which backend has each. The tags of increment 6's functions
(`digests::streebog_256`, `streebog_512`, `whirlpool`, `skein_512`, `kdfs::bcrypt`,
`bcrypt_pbkdf`) are declared with no backend: a call refuses with "... is in no backend yet."
(3.3).

## 3. Backends

### 3.1 The libraries

| Backend | Library | Version and commit | Licence | Targets | Has |
| --- | --- | --- | --- | --- | --- |
| `backends::hash2` | Boost.Hash2's `hmac<H>`, webcpp's construction (spec 1.3) | Boost 1.92 | BSL-1.0 | all | HKDF (three functions), PBKDF2; every Hash2 hmac whose result is a `digest<N>`; from increment 2, the families over Hash2's algorithms (2.12) |
| `backends::sodium` | libsodium | 1.0.22, tag `1.0.22-RELEASE`, commit `77e1ce5d6dee871c49ef211222ba18ef0c486bda` | ISC | all | HKDF (three functions) over SHA-256 and SHA-512; Argon2id; scrypt; increment 2's surface (2.12) |
| `backends::bearssl` | BearSSL | webcpporg/bearssl at `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (webcpp::crypto's pin) | MIT | all | `hkdf` alone, over MD5, SHA-1, SHA-224, SHA-256, SHA-384, SHA-512; increment 5's digests and HMAC |
| `backends::openssl` | OpenSSL | 3.5.9 at `45e844fa2a14ec92d146bd8f5778ac130b6625fb` (the superproject's provider) | Apache-2.0 | native, wasip2, wasip3 | HKDF (three functions), PBKDF2; every Hash2 hmac but `hmac_ripemd_128`; increment 4's surface (2.12) |
| (none: `keccak_256`) | XKCP, `lib/low/KeccakP-1600/plain-64bits` | master at `d20c8fcaecc543f816a0721d0dfc13b4cf27c14d` (2026-11-10) | CC0 (`brg_endian.h`: Brian Gladman's BSD-style terms) | all | Keccak-p[1600] |
| (none: `blake3`) | BLAKE3's C implementation, `c/` | 1.8.7 at `f3149ec5bb5449af877ba20377a11008ff499fa2` | CC0-1.0, Apache-2.0, or Apache-2.0 with LLVM exception | all | BLAKE3 |
| `backends::xkcp` (`keccak_256` in increment 2, the rest in increment 3) | XKCP, more of it: `lib/high/Keccak/KeccakSponge.c`, `lib/high/Keccak/FIPS202/KeccakHash.c`, `lib/high/Keccak/SP800-185/SP800-185.c`, `lib/high/TurboSHAKE/TurboSHAKE.c`, `lib/high/KangarooTwelve/KangarooTwelve.c` (5.2) | as above | as above | all | 3.5's XKCP column |
| `backends::botan` (increment 6, not built: design D42) | Botan 3, as webcpp::crypto's increment 5 provisions it | 3.13.0 measured by crypto; 3.14.1 the latest tag on 2026-11-14 (`01af88c`) | BSD-2-Clause | native, with exceptions only (crypto's D19) | 3.5's Botan column |
| `backends::webcrypto` (increment 7, section 11) | the browser's or node's WebCrypto | the engine's | - | Emscripten | SHA-1/256/384/512, HMAC, HKDF, PBKDF2 over them, through the asynchronous family alone |

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
  (OpenSSL 3.5 has no RIPEMD-128). Name backends::hash2."; any other hmac outside `hmac.hpp`'s
  aliases: "webcpp::hash: backends::openssl has HKDF and PBKDF2 over the hmacs of hmac.hpp alone,
  hmac_ripemd_128 excepted. Over this hmac, name backends::hash2."
- `hkdf` or `pbkdf2` on a backend without the function, through the primary templates:
  "webcpp::hash: this backend has no HKDF. The backends that have HKDF: backends::hash2 (every
  hmac), backends::openssl (every hmac but hmac_ripemd_128), backends::sodium (hmac_sha2_256,
  hmac_sha2_512) and backends::bearssl (MD5, SHA-1, SHA-224, SHA-256, SHA-384, SHA-512; hkdf
  alone)." and "webcpp::hash: this backend has no PBKDF2. The backends that have pbkdf2:
  backends::hash2 (every hmac) and backends::openssl (every hmac but hmac_ripemd_128)."
- an `Hmac` that is not a Hash2 hmac over a `digest<N>` result, on any backend: "webcpp::hash:
  the key derivations take a Boost.Hash2 hmac whose result is a digest, as hmac_sha2_256."
- `argon2id` or `scrypt` on another backend than `backends::sodium` and `backends::openssl`
  (increment 4, ruling R43): "webcpp::hash: argon2id is in backends::sodium and
  backends::openssl alone. Name one, and add /third_party//sodium or /webcpp/hash//openssl to the
  program's Jamfile." (and the same for scrypt). `blake2b_subkey` on another backend than
  `backends::sodium`: "webcpp::hash: blake2b_subkey is in backends::sodium alone. Name
  backends::sodium and add /third_party//sodium to the program's Jamfile."

**The families' refusals (increments 2 to 7).** Each tag has a partial specialisation of its
engine that refuses a backend without the tag; its text names the backends of the tag's row of
`doc/capabilities.json`, which `capabilities_test` checks, so that each text below changes with the
row:

- a type that is not a tag: "webcpp::hash: digest_of and hasher take a tag of
  webcpp::hash::digests, as digests::sha2_256.", "webcpp::hash: mac takes a tag of
  webcpp::hash::macs, as macs::hmac<digests::sha2_256>.", "webcpp::hash: argon2 takes
  kdfs::argon2d, kdfs::argon2i or kdfs::argon2id.";
- a tag on a backend without it: "webcpp::hash: <qualified tag> is not in this backend. Name one
  of: <the backends of its row>." (as "webcpp::hash: digests::sha2_256 is not in this backend.
  Name one of: backends::hash2, backends::sodium, backends::bearssl, backends::openssl."); the
  backends are listed in the order hash2, xkcp, blake3, sodium, bearssl, openssl, and a partial
  cell may carry its limit in brackets ("backends::xkcp (a block size that is a power of two of
  at least 8 bytes)" for ParallelHash);
- a tag no backend has yet (increment 6's: Streebog, Whirlpool, Skein-512, bcrypt): "webcpp::hash:
  <qualified tag> is in no backend yet."; HMAC over a digest no backend has it over
  (`digests::md5_sha1` included, ruling R92, and every extendable-output digest, over which HMAC
  is not defined): "webcpp::hash: macs::hmac over this digest is in no backend.", without "yet",
  since no backend will have it;
- a tag whose default is `detail::no_default` (design D34), called without a backend:
  "webcpp::hash: <qualified tag> has no default backend. Name one of: <the backends of its row>."
  (as "webcpp::hash: kdfs::argon2d has no default backend. Name one of: backends::openssl."), or
  "webcpp::hash: <qualified tag> has no default backend, and no backend has it yet." for a tag no
  backend has (and "webcpp::hash: macs::hmac over this digest has no default backend: no backend
  has it.");
- `kbkdf` over an hmac outside `hmac.hpp`'s aliases on `backends::openssl`: "webcpp::hash:
  backends::openssl has kbkdf over the hmacs of hmac.hpp alone, hmac_ripemd_128 excepted, and no
  other backend has kbkdf."; `sskdf` and `x963kdf` over a digest their standard does not take
  (design D41): "webcpp::hash: sskdf takes a digest of SP 800-56C: digests::sha1,
  digests::sha2_224, sha2_256, sha2_384, sha2_512, sha2_512_224, sha2_512_256, sha3_224,
  sha3_256, sha3_384 or sha3_512." and "webcpp::hash: x963kdf takes a hash function of SEC 1 v2,
  section 3.5: digests::sha1, digests::sha2_224, sha2_256, sha2_384 or sha2_512.";
- a family's call on a C library's backend that the program does not link, named or by default:
  that library's text of 5.7 (`detail::needs_library`), so that a refused call shows one message;
  and a synchronous function on `backends::webcrypto`: section 11.2's text.

Between increments a default whose library is not linked may first say "add the library" and then
name the backends that have the tag; once increments 2 to 5 and 7 have landed every default has its
engine (ruling R84), `macs::blake2b_mac`'s on `backends::sodium` included (ruling R95), so that a
call that names no backend refuses only with its library's text of 5.7 when the program does not
link it.

### 3.4 Each backend's work

- **`backends::hash2`.** `extract`: `Hmac h(salt)` (Hash2 keys an empty salt as zeros of the
  block size, which is HMAC's key of HashLen zeros, RFC 5869 2.2), `update(ikm)`, `result()`.
  `expand`: one keyed `Hmac` built from the PRK, copied for each block T(i) =
  HMAC(PRK, T(i-1) | info | i), the last block cut to fit. `pbkdf2`: one keyed `Hmac` built
  from the password, copied for U_1 = PRF(P, S | INT(i)) and each U_j, the XOR accumulated in
  place, the last block cut. Every block, copy and U is wiped by `detail::wipe` at return. The
  families (increment 2): a Hash2 algorithm per tag, its state wiped by `destroy`; SHAKE128 and
  SHAKE256 of any length from repeated `result()` calls, each of which permutes and gives the
  next rate block (design D37); SipHash-2-4 as Hash2's `siphash_64` keyed by the 16 bytes, its
  result written little-endian; keyed BLAKE2s as Hash2's keyed `blake2s_256`.
- **`backends::xkcp`.** `keccak_256` through webcpp's sponge over Keccak-p[1600] (2.6); from
  increment 3, FIPS 202's `Keccak_Hash`, SP 800-185's functions, `KangarooTwelve` and
  `TurboSHAKE`, each through webcpp's adapter (`src/xkcp_adapter.c`, `detail/xkcp.hpp`) with
  opaque storage per instance kind, its size and alignment checked by `_Static_assert` in the
  adapter (ruling R32), the number of 64-bit words of each kind written once, in the C and C++
  header `detail/xkcp_storage.h` that the adapter and `detail/xkcp.hpp` both read; the adapter's
  `_final_length` writes SP 800-185's L at `finish` (2.15).
- **`backends::blake3`.** `digests::blake3` over webcpp's `blake3` (2.7), its extendable output.
- **`backends::sodium`.** `sodium_init()` is called first in every engine function (it returns 0 the
  first time and 1 after, both success; -1 is `backend_failure`).
  `crypto_kdf_hkdf_sha256_extract`/`_sha512_extract`, `_expand` (whose `ctx` is the info);
  `crypto_pwhash_argon2id(..., crypto_pwhash_ALG_ARGON2ID13)`;
  `crypto_pwhash_scryptsalsa208sha256_ll`. A return of -1 with `errno == ENOMEM` is
  `errc::not_enough_memory`, any other -1 `backend_failure`. Increment 2:
  `crypto_generichash_blake2b_salt_personal` and its incremental form, `crypto_shorthash_siphash24`
  and `_siphashx24`, `crypto_kdf_blake2b_derive_from_key`, `crypto_hash_sha256_*`,
  `crypto_hash_sha512_*`, `crypto_hash_sha3256_*`, `crypto_hash_sha3512_*`, `crypto_xof_shake*` and
  `crypto_xof_turboshake*`, `crypto_auth_hmacsha256_*`/`_hmacsha512_*` (one-shot HMAC engines
  through `_init(state, key, keylen)`; ruling R23), `crypto_pwhash_argon2i` and
  `crypto_pwhash_argon2id`.
- **`backends::bearssl`.** `br_hkdf_init` with the hash's `br_*_vtable` and the salt (an empty
  salt is passed with length 0: HMAC pads a key with zeros to the block, so it is RFC 5869's
  HashLen zeros, and the agreement test covers it), `br_hkdf_inject`,
  `br_hkdf_flip`, `br_hkdf_produce` into `output`; a produced count other than `output.size()` is
  `backend_failure`. The context is wiped at return. Increment 5: `br_<h>_init`, `_update` and
  `_out` for MD5, SHA-1 and SHA-224 to SHA-512, `br_md5sha1_*`, `br_shake_*` (any output size),
  and `br_hmac_key_init`, `br_hmac_init`, `br_hmac_update`, `br_hmac_out` (the tag cut by BearSSL);
  each context wiped at return, a hasher's when it ends.
- **`backends::openssl`.** `EVP_KDF_fetch(nullptr, "HKDF"|"PBKDF2", nullptr)`, a context, the
  parameters (`digest` the name of 3.2, `key` or `pass`, `salt`, `info`, `mode` 0, 1 or 2 for
  HKDF, `iter` and `pkcs5 = 1` for PBKDF2), `EVP_KDF_derive`, then the context and the KDF
  freed. Every call clears the thread's OpenSSL errors it caused (`ERR_set_mark` and
  `ERR_pop_to_mark`); a call during which OpenSSL raised an error failed, whatever its functions
  returned (OpenSSL 3.5.9's Argon2 leaves some of its allocations unchecked and returns success
  over a wrong tag when one fails; increment 4), and is `errc::not_enough_memory` when an error it
  raised is an allocation's (`ERR_R_MALLOC_FAILURE`, or the function's own memory error),
  `backend_failure` otherwise. The lengths OpenSSL holds in an `int` are refused past `INT_MAX`
  before it is called (2.8, `detail::backend_limits`, design D26). Increment 4: the digests
  through `EVP_MD_fetch` and an `EVP_MD_CTX` (a hasher owns its context until it ends), the MACs
  through `EVP_MAC_fetch` and an `EVP_MAC_CTX` ("HMAC", "KMAC-128", "KMAC-256", "SIPHASH",
  "BLAKE2BMAC", "BLAKE2SMAC"), and the `EVP_KDF`s "SCRYPT" (`maxmem_bytes` at its largest),
  "ARGON2D", "ARGON2I", "ARGON2ID" (the lanes computed one after the other in the calling
  thread), "KBKDF", "SSKDF" and "X963KDF". What the default provider has and hash does not expose
  (SHA-256/192, the KMAC hashes, `NULL`, CMAC, GMAC, Poly1305), and Whirlpool, MD4 and MDC2, in
  the legacy provider that webcpp never loads, are listed with their reasons in
  `doc/capabilities.json` (ruling R42).

### 3.5 The capability table

Which backend provides each function, and from which increment. `hash2` is webcpp's code over
Boost.Hash2 (Hash2's own algorithms, HKDF and PBKDF2); a cell names the function of the
library. This table is `backend_has_v` (2.8) and the source of every refusal text of 3.3; the
documentation renders it from the same data file, `doc/capabilities.json`, which a test compares
with `backend_has_v` for every cell.

**`doc/capabilities.json`'s layout.** Increment 1 left it at layout version 3 (a `"default"` per
function row, design D33). The families' task raised it to 4 (ruling R13): a family's row is named
by its qualified tag (`digests::sha2_256`, `macs::hmac<digests::sha2_256>`) and has a `kind`
(`type`, `function`, `digest`, `mac`, `kdf`); a cell has `since` (its increment, 1 to 7), `hmacs`,
`partial` (on a cell that refuses some values the family accepts: what, with
`unsupported_by_backend`) and `constant_time` (`value` "yes", "no" or "not stated", with its
upstream `source`); xkcp and blake3 are backends of their own. Increment 4 raised it to 5: a
backend may list `omitted[]` (`names` and `reason`: what its library has and hash does not expose
or load, as OpenSSL's legacy provider; ruling R42). Increment 7 gave a backend the field
`asynchronous_only` (true for `backends::webcrypto`, whose cells are the asynchronous functions'
and which links no library of its own, `macro` null; ruling R90), which `tools/hash/doc_tables.py`
renders with the note "asynchronous family only"; it raised no version. WebCrypto's HKDF and
PBKDF2 cells are `partial`: their narrower limits (11.2) answer `input_too_long`,
`invalid_iterations` and `invalid_output_size`, the errors of `backend_limits`, not
`unsupported_by_backend`. A row sits in an algorithm group of its namespace's chapter (a tag of
`macs` in a group of chapter `macs`: keyed BLAKE2b and BLAKE2s in `blake2_mac`), which
`tools/hash/doc_tables.py` checks. `capabilities_test` compares every cell with `backend_has_v`,
every default with `default_backend`, and every refusal text of 3.3 with its row.

| Function | hash2 | xkcp | blake3 | sodium | bearssl | openssl | botan | webcrypto (async, 11) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MD5, SHA-1, SHA-2 (six) | 1 (Hash2); family: 2 | - | - | SHA-256, SHA-512: 2 | 5 (no SHA-512/t) | 4 | 6 | SHA-1/256/384/512: 7 |
| MD5-SHA1 (TLS 1.0's 36 bytes) | - | - | - | - | 5 | 4 | - | - |
| SHA-3 (four), SHAKE128/256 | 1 (Hash2); family: 2, SHAKE of any length (design D37) | 3 | - | SHA3-256/512, SHAKE: 2 | SHAKE: 5 | 4 | 6 | - |
| Keccak-256 | - | 1 (`keccak_256`); family: 2 | - | - | - | 4 (`KECCAK-256`) | 6 | - |
| Keccak-224, -384, -512 | - | 3 | - | - | - | 4 | 6 | - |
| cSHAKE, KMAC, TupleHash, ParallelHash | - | 3 (ParallelHash: a block size that is a power of two of at least 8 bytes) | - | - | - | KMAC: 4 (OpenSSL's limits: a key of 4 to 512 bytes, a customisation of at most 512, a tag of at most 2,097,151) | cSHAKE, KMAC: 6 | - |
| KangarooTwelve (KT128, KT256) | - | 3 | - | - | - | - | - | - |
| TurboSHAKE128/256 | - | 3 | - | 2 | - | - | - | - |
| BLAKE2b (key, salt, personalisation, length) | 512 fixed: 1 (Hash2); family: 2 (64 bytes, no option) | - | - | 2; keyed (`macs::blake2b_mac`), a key and a tag of 1 to 64 bytes: 2 | - | unkeyed, 1 to 64 bytes: 4; keyed (`macs::blake2b_mac`): 4 | 6 | - |
| BLAKE2s-256 | 1 (Hash2); family: 2; keyed (`macs::blake2s_mac`), a 32-byte tag: 2 | - | - | - | - | 4; keyed: 4 | 6 | - |
| BLAKE3 (hash, keyed, derive_key, XOF) | - | - | 1; family: 2 | - | - | - | - | - |
| RIPEMD-128, RIPEMD-160 | 1 (Hash2); family, RIPEMD-160: 2 | - | - | - | - | RIPEMD-160: 4 | RIPEMD-160: 6 | - |
| SM3, Streebog, Whirlpool, Skein-512 | - | - | - | - | - | SM3: 4 | 6 | - |
| HMAC | 1 (Hash2, every hash); family: 2 | - | - | SHA-256/512: 2 | 5 (MD5, SHA-1, SHA-224 to SHA-512) | 4 (each fixed-size digest of its own but MD5-SHA1; a key of at most `INT_MAX` bytes) | 6 | 7 |
| SipHash-2-4, SipHash-2-4x | SipHash-2-4 64: 1 (Hash2); family, SipHash-2-4: 2 | - | - | 2 | - | 4 | 6 | - |
| HKDF | 1 | - | - | 1 (SHA-256/512) | 1 | 1 | 6 | 7 |
| `async_argon2id`, `async_argon2`, `async_scrypt`, `async_pbkdf2` on another thread | every backend that has the function, only with threads (11.5): natively always (unless `WEBCPP_HASH_NO_THREADS`), Emscripten `-pthread`; never WASI | | | | | | | `async_pbkdf2` without threads too |
| PBKDF2 | 1 | - | - | - | - | 1 | 6 | 7 |
| scrypt | - | - | - | 1 | - | 4 (r p below 2^24) | 6 | - |
| Argon2id, one lane | - | - | - | 1 | - | 4 | 6 | - |
| Argon2i | - | - | - | 2 (one lane, 3 passes or more) | - | 4 | 6 | - |
| Argon2d; Argon2 with lanes, secret, associated data | - | - | - | - | - | 4 (m of at most 4,194,303 KiB on wasm32) | 6 | - |
| BLAKE2b subkeys (`crypto_kdf`) | - | - | - | 2 | - | - | - | - |
| bcrypt, bcrypt-pbkdf | - | - | - | - | - | - | 6 | - |
| KBKDF (SP 800-108), SSKDF (SP 800-56C), X9.63 KDF | - | - | - | - | - | 4 (SSKDF and X9.63: at most 2^30 bytes each) | 6 (where it has them) | - |

A cell with a limit in brackets is partial in `doc/capabilities.json`: the backend answers
`unsupported_by_backend` (for the families) or the function's guard error (for increment 1's
functions) where it refuses a value the family accepts, which the row's `partial` text states.
"family: 2" is the cell of the families (2.15) over increment 1's library, from increment 2. The
webcrypto column is the asynchronous family's alone (section 11): no synchronous function takes
`backends::webcrypto`, which `doc/capabilities.json` marks `asynchronous_only` (design D39).
Increment 6 is not built (design D42): the botan column is the plan's, not a cell of
`doc/capabilities.json`, and at Botan 3.14.1 it overstates (no SipHash-2-4x, BLAKE2b without salt
or personalisation, no TupleHash or ParallelHash; X9.63 is Botan's `kdf2` and the one-step SSKDF
its `sp800_56a`).

A cell of 6 is checked against Botan's documentation at its pin by increment 6's first task,
which corrects the table where Botan lacks a function; no cell is assumed.

## 4. The default backend

### 4.1 The default: `backends::hash2` (the owner's decision of 2026-11-16)

The default backend of every key derivation, `hkdf`, `hkdf_extract`, `hkdf_expand` and `pbkdf2`,
over every hmac, on native, wasip2, wasip3 and Emscripten and whatever the program links, is
`backends::hash2`. `backends::sodium` is the default only for what hash2 does not offer:
`argon2id` and `scrypt`, of which it was the only backend in increment 1 (increment 4 brought
OpenSSL's, reached by naming it), and `blake2b_subkey` (increment 2); a program that calls them
without `/third_party//sodium` stops with libsodium's "needs" message (5.7), raised for
`backends::sodium` by the primary template of the engine in `detail/engines.hpp` and tested by the
messages `argon2id_default_not_linked`, `scrypt_default_not_linked` and
`blake2b_subkey_default_not_linked`.
`doc/capabilities.json` (layout version 3 in increment 1, 5 from increment 4; 3.5) names each
function row's `"default"`, as the tables below (`null` for a type), and `capabilities_test`
compares it with `default_backend`.

| Operation (tag of `kdfs`) | Default backend |
| --- | --- |
| `hkdf`, `hkdf_extract`, `hkdf_expand`, `pbkdf2` | `backends::hash2` |
| `argon2id`, `scrypt`, `blake2b_subkey` | `backends::sodium` |

`default_backend<Algorithm, Hmac = void>` and `default_kdf_backend<Hmac, Kdf = kdfs::hkdf>`
(which is `default_backend<Kdf, Hmac>`) keep their names and signatures; their answer is the
table's and depends neither on the hmac, nor on the libraries linked, nor on the target. Each
function names its default through its own tag (`hkdf_extract` through `kdfs::hkdf_extract`, ...),
and `backend_has_v` answers for the two steps through `kdfs::hkdf_extract` and `kdfs::hkdf_expand`.

**The other backends are explicit choices.** `backends::bearssl`, `backends::openssl` (its
Argon2id and scrypt included) and the HKDF of `backends::sodium` stay, with their vectors,
agreement tests, limits and refusals, and are reached only by naming them in the type, as
`hkdf<hmac_sha2_256, backends::openssl>`; none is ever a default.

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

**The families' defaults (the owner's decision of 2026-11-17, design D34).** A tag of the families
(2.15) names its default through `default_backend<Tag>` too: the first of `backends::hash2`,
`backends::xkcp`, `backends::blake3` and `backends::sodium`, in that order, that offers the tag, its
engine serving the family's whole contract (every option and every output size the family's guards
accept); if none of the four offers it whole, the first of them with a partial cell; if none has a
cell, `detail::no_default`, and the call names its backend (3.3's "has no default backend" text;
`"default": null` in `doc/capabilities.json`). `backends::bearssl`, `backends::openssl`,
`backends::botan` and `backends::webcrypto` are never defaults. One stated rule, no measurement
(D33's reason). The defaults were written once, by the families' task, from the table as it stands
once increments 2 to 5 and 7 have landed, and they never change between increments (ruling R2); a
default whose engine landed in a later stream refused at compile time until it landed (ruling R84),
and once the batch has landed every default has its engine, which `tools/hash/doc_tables.py` checks
(a family's default without its cell fails).

| Tags | Default backend |
| --- | --- |
| `digests::md5`, `sha1`, the six SHA-2, the four SHA-3, `shake_128`, `shake_256` (design D37), `ripemd_160`, `blake2s_256`; `macs::hmac<D>` over each of these and over `digests::blake2b`; `macs::siphash_2_4`, `macs::blake2s_mac` (partial: a 32-byte tag) | `backends::hash2` |
| `digests::keccak_224` to `keccak_512`, `cshake_128`, `cshake_256`, `kt128`, `kt256`, `turboshake_128`, `turboshake_256`, `tuplehash_128`, `tuplehash_256`, `parallelhash_128`, `parallelhash_256` (partial); `macs::kmac_128`, `macs::kmac_256` | `backends::xkcp` |
| `digests::blake3` | `backends::blake3` |
| `digests::blake2b`; `macs::siphash_2_4x`, `macs::blake2b_mac`; `kdfs::argon2i`, `kdfs::argon2id` (partial: one lane); `kdfs::blake2b_subkey` | `backends::sodium` |
| `digests::md5_sha1`, `sm3`, `streebog_256`, `streebog_512`, `whirlpool`, `skein_512`; `macs::hmac<D>` over any other digest; `kdfs::argon2d`, `bcrypt`, `bcrypt_pbkdf`, `kbkdf_hmac`, `sskdf`, `x963kdf` | none (`detail::no_default`) |

`capabilities_test` checks each row's default against `default_backend` and that no backend
before it in the order has the tag whole.

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

**Release builds only (the owner's decision of 2026-11-17, design D36).** Every measured table is
built only from release builds: the sizes above (`variant=release optimization=space`), the
throughput of 4.3 (`bench/collect.py` reads the variant from b2's build directory and refuses
another), and the allocation counts of 7.7 (`bench/allocation_collect.py`). The measurements of
2026-11-16 stay as they are and gain no rows (ruling R3).

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
| `xkcp` | `third_party/libs/xkcp` (`XKCP/XKCP`) | `lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c`; from increment 3 `lib/high/Keccak/KeccakSponge.c`, `lib/high/Keccak/FIPS202/KeccakHash.c`, `lib/high/Keccak/SP800-185/SP800-185.c`, `lib/high/TurboSHAKE/TurboSHAKE.c`, `lib/high/KangarooTwelve/KangarooTwelve.c` (no times-N file); webcpp's `src/xkcp_adapter.c` and `src/canary_xkcp.c` | none: XKCP's "generic64" profile (all rounds unrolled, no lane complementing) on every target; its `config.h`, generated by XKCP's own build, is webcpp's `libs/hash/src/xkcp/config.h` with `XKCP_has_KeccakP1600`, `XKCP_has_Sponge_Keccak`, `XKCP_has_TurboSHAKE`, `XKCP_has_FIPS202`, `XKCP_has_SP800_185` and `XKCP_has_KangarooTwelve` (the defines of the fragments XKCP's `lib/HighLevel.build` names), no `XKCP_has_KeccakP1600times2`, 4 or 8 (generic64 has no parallel permutation, so `KangarooTwelve.c` and `SP800-185.inc` take their serial loops), and `__attribute__(x)` defined empty on MSVC's own compiler, for `KangarooTwelve.c` | `WEBCPP_HASH_HAS_XKCP=1` (no include directory: the public header declares only the adapter) |
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
| `backends/xkcp.hpp` without `WEBCPP_HASH_HAS_XKCP` (increment 2) | "webcpp::hash: backends::xkcp needs XKCP. Add /third_party//xkcp to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//xkcp ;" |
| `backends/blake3.hpp` without `WEBCPP_HASH_HAS_BLAKE3` (increment 2) | "webcpp::hash: backends::blake3 needs BLAKE3's C implementation. Add /third_party//blake3 to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//blake3 ;" |
| `backends/sodium.hpp` without `WEBCPP_HASH_HAS_SODIUM` | "webcpp::hash: backends::sodium needs libsodium. Add /third_party//sodium to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//sodium ;" (increment 1's text named argon2id and scrypt; since `backends::openssl` has both and libsodium is the default of several families' tags, the text names the backend alone, as the other libraries' do) |
| `backends/bearssl.hpp` without `WEBCPP_HASH_HAS_BEARSSL` | "webcpp::hash: backends::bearssl needs BearSSL. Add /third_party//bearssl to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//bearssl ;" |
| `backends/openssl.hpp` without `WEBCPP_HASH_HAS_OPENSSL` | "webcpp::hash: backends::openssl needs OpenSSL. Add /webcpp/hash//openssl to the program's Jamfile (native, wasip2 and wasip3): exe app : app.cpp /webcpp/hash//hash /webcpp/hash//openssl ;" |
| OpenSSL on Emscripten | spec 2.2's first `#error` |
| a function of the families, `argon2id`, `scrypt`, `blake2b_subkey`, `kbkdf`, `sskdf` or `x963kdf` called on a C library's backend the program does not link, named or by default | that library's text above, raised by the engine's primary template or `detail::needs_library` (the header that holds the engines is included only when the library is linked), so that `argon2.hpp`, `blake2b_subkey.hpp` and `kdf_standards.hpp` need no library to be included; one message test per library and kind (`<library>_not_linked`, `<library>_default_not_linked`, `openssl_kdf_not_linked`) |
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
destructors do not wipe (Hash2's types do not), which the page states. A `hasher` (2.15) wipes its
state, and frees what the backend allocated for it, at `finish`, in its destructor and in the
hasher moved from.

The asynchronous family (section 11) allocates as every Asio operation does: the operation's
state and its completion are allocated with the handler's associated allocator
(`bind_allocator`), and all of it is given back before the handler runs, a cancellation's
completion included; a completion destroyed without running gives the state back too. webcpp's own
code calls neither `operator new` nor the C allocator; the thread pool allocates the job it queues
as Asio does, and webcpp::asio's `browser::async_await` makes one allocation of the C allocator
per awaited promise, which is webcpp::asio's, not hash's (flagged to the owner).

### 6.2 The C libraries that allocate (the recorded departures)

- **libsodium's Argon2id and scrypt** allocate the memory their cost names (`mmap` where
  `HAVE_MMAP`, else `malloc` and, for Argon2's 64-byte alignment, `posix_memalign` or an
  aligned `malloc`), outside any allocator of the program's. The owner's alternative, the
  Argon2 reference implementation's `allocate_cbk`/`free_cbk` callbacks, would route Argon2id's
  memory through a `std::pmr::memory_resource`; it is not built (design D12), and becomes a
  later backend `backends::argon2` if a user's resource must hold the memory.
- **OpenSSL's Argon2id and scrypt** (increment 4) allocate the memory their cost names inside
  OpenSSL, through its allocator, as libsodium's do.
- **OpenSSL** allocates its KDF and context in every call (`EVP_KDF_fetch`,
  `EVP_KDF_CTX_new`), through its own allocator, as webcpp::crypto's D17 records for its
  contexts; webcpp keeps no OpenSSL object between calls, since that would be state. From
  increment 4 a digest allocates an `EVP_MD_CTX` (`EVP_MD_CTX_new`) and the digest's context,
  which `digest_of` frees before it returns and a `hasher` owns, its own and no other's, until it
  finishes or is destroyed (the one OpenSSL object that outlives a call); a MAC fetches the MAC and
  allocates an `EVP_MAC_CTX` (`EVP_MAC_CTX_new`) and the MAC's context, which `mac` frees before it
  returns (ruling R44).
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

| the families (increments 2 to 7) | each tag on each backend against its standard's or upstream's vectors where they exist: FIPS 202's (OpenSSL's `evpmd_sha.txt`) for Hash2's SHAKE past one block; Wycheproof's `siphash_2_4_test.json` and OpenSSL's `evpmac_blake.txt` for Hash2's SipHash and keyed BLAKE2s; BLAKE2's reference KAT (keyed and unkeyed, the submodule `third_party/libs/blake2` at `ed1974e`) and libsodium's `.exp` files (`generichash3`, `shorthash`, `siphashx24`, `kdf`, `pwhash_argon2i`) for libsodium; RFC 9861's TurboSHAKE and KangarooTwelve examples (the RFC's printed values, copied with the section cited); XKCP's `tests/TestVectors` (the byte-aligned entries, counted) and NIST's SP 800-185 samples for XKCP; OpenSSL's own `test/recipes/30-test_evp_data` files (`evpmac_common.txt`, `evpmac_siphash.txt`, `evpkdf_kbkdf_counter.txt`, `evpkdf_ss.txt`, ...) and RFC 9106 section 5's three cases through `argon2`; RFC 2202 for BearSSL's HMAC-MD5 and HMAC-SHA-1; RFC 4231, 5869, 6070 and 7914 section 11 for WebCrypto (section 11.4) |

Test data from a submodule is read at run time through a `third_party/Jamfile` target that gives its
directory (`webcpp.data`, in wycheproof's quoting form, `[ webcpp.data <dir> ]` for WASI's preopen),
as `/third_party//wycheproof` does, never copied (ruling R5): `/third_party//blake3_vectors`,
`/third_party//blake2_vectors`, `/third_party//sodium_vectors` (libsodium's `.exp` files),
`/third_party//xkcp_vectors` and `/third_party//openssl_evp_data` (OpenSSL's EVP test files at its
pin). An RFC's or a NIST publication's printed example may be copied with its section cited.

### 7.2 Agreement between backends

`agreement_hkdf_test` and `agreement_pbkdf2_test`, with a fixed, printed seed and 1,000 random
cases each (lengths 0 to 300 for salt, IKM, password and info, outputs of 1 to 4 digest sizes,
iterations 1 to 64): natively every backend with every other on every hmac both have; on
wasip2 and wasip3 `backends::hash2`, `backends::sodium`, `backends::bearssl` and
`backends::openssl`; on Emscripten `backends::hash2`, `backends::sodium` and
`backends::bearssl`. Empty salt, empty IKM, empty info and empty password are always among the
cases.

The families' agreement (increments 2 to 7): each increment's tests compare its backend's cells with
every backend that has the same tag and exists when its stream starts (Hash2, `keccak_256`,
`blake3`, increment 1's backends, the families, its own stream; ruling R6);
`agreement_families_test` compares every pair of backends a tag's row names, and the pairs across
streams (libsodium's, XKCP's, BearSSL's and OpenSSL's cells of the same tag; the `argon2` family on
libsodium and on OpenSSL) are added once every stream has merged, by the task "Agreement across
increments" (plan Task 47).

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
GNU and wasm linkers, as webcpp::crypto's decision 5 records. The families' texts of 3.3 are
checked against `doc/capabilities.json` by `capabilities_test`, tag by tag; their message tests are
one per family's primary template, one per kind of default not linked, one per family's
`no_default`, one per backend that lacks a whole function (`*_on_hash2`), and, for section 11, one
per heavy name without threads, the synchronous refusal of `backends::webcrypto`, its refusal off
Emscripten and the cheap functions' refusal of an algorithm WebCrypto lacks.

### 7.7 Allocation

`allocation_test` replaces every form of `operator new` with a counter and calls every function
on every backend and target: zero for webcpp's code and for `backends::hash2`, `sodium`'s HKDF,
`bearssl`, `keccak_256` and `blake3`; Argon2id and scrypt allocate through libsodium's C
allocator, which the test does not see and the page names; OpenSSL's allocations are counted
natively through `CRYPTO_set_mem_functions` in a separate program, reported, not bounded. Each
increment whole adds its entry points (ruling R94): the families on every backend that has a tag
and is linked, and on each tag's default, with the count of calls asserted per backend from
`doc/capabilities.json`; `blake2b_subkey`; `argon2` on libsodium (its C allocator's count
printed, `operator new` zero); OpenSSL's digests, MACs and KDFs in `allocation_openssl_test`; the
asynchronous family (its storage the handler's, 6.1) and `backends::webcrypto`.

The published counts (`doc/measurements.allocation.json`, `doc/allocation.adoc`) are built only
from release builds (the owner's decision of 2026-11-17, design D36): `bench/Jamfile`'s
`allocation_release` and `allocation_openssl_release`, the sources of the two tests at
`variant=release`, collected by `bench/allocation_collect.py`, which reads the variant from b2's
build directory (`bench/<name>.test/<toolset>/<variant>/`), records `"build": "variant=release"`
and refuses data from another build, its `--check` and `--render-only` included. The lanes'
allocation tests keep their variant and are never collected.

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
- A tag of the families defaults to the first of `backends::hash2`, `backends::xkcp`,
  `backends::blake3` and `backends::sodium` that offers it whole, else the first with a partial
  cell, else none; BearSSL, OpenSSL, Botan and WebCrypto are never defaults (4.1; design D34, the
  owner's decision of 2026-11-17, which answers O3). `default_async_backend` is `default_backend`
  on every target (11.2; design D35). Every measured table, the allocation counts included, is
  built only from release builds (4.2, 7.7; design D36).
- What building increments 2 to 5 and 7 amended (design D37 to D41, the controller's rulings R1 to
  R94 of the plan's execution): Boost.Hash2's SHAKE is a whole extendable output and SHAKE's
  default (D37, ruling R89, which overturns R14); the one-shot hash is `digest_of`, BLAKE2b's
  output size is an option, SP 800-185's L is written at `finish`, KMAC's tag is at least 4 bytes
  and HMAC's at least 10, TurboSHAKE's domain byte is 0x01 to 0x7F, the messages of values 1, 2
  and 5 are reworded and value 9 added (2.3, 2.15; D38); `backends::webcrypto` is declared on every
  target by increment 7, appears in `doc/capabilities.json` as an asynchronous-only backend, checks
  its executor and states WebCrypto's limits (11.2; D39); the heavy functions exist natively in
  every build unless `WEBCPP_HASH_NO_THREADS`, with 11.5's new text, and allocate with the
  handler's allocator (6.1, 11.5; D40); no HMAC over MD5-SHA1, and `sskdf` and `x963kdf` take
  their standard's hashes alone (2.15, 3.3; D41); `argon2id` and `scrypt` gain OpenSSL with 3.3's
  new texts (ruling R43); `doc/capabilities.json` is at layout version 5 (3.5).
- Increment 6 (Botan) is not built: Botan is not provisioned on webcpp's `main` (design D42,
  ruling R60).

## 11. The browser backend and the asynchronous family (the owner's answer to O1)

The owner decided on 2026-11-14 that the browser backend, `backends::webcrypto` (WebCrypto's
`SubtleCrypto`, Emscripten only), is reached through a **parallel asynchronous API** beside the
synchronous one, in `<webcpp/hash/async.hpp>` (increment 7):

```cpp
namespace webcpp::hash {

/** The backend of the asynchronous family: the synchronous function's, on every target (11.2, design D35). */
template <class Algorithm, class Hmac = void>
using default_async_backend = default_backend<Algorithm, Hmac>;

/** Hashes `input` into `output` (SHA-1, SHA-256, SHA-384, SHA-512), completing with `void(boost::system::error_code)`. */
template <class Digest, class Backend = default_async_backend<Digest>, class Executor, class CompletionToken>
auto async_digest(Executor const& executor, std::span<std::uint8_t> output,
                  std::span<std::uint8_t const> input, CompletionToken&& token);

/** HMAC of `input` under `key` over `Hmac` (SHA-1, SHA-256, SHA-384, SHA-512). */
template <class Hmac, class Backend = default_async_backend<macs::hmac<typename detail::digest_tag_of<Hmac>::type>>,
          class Executor, class CompletionToken>
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
  over them). On a synchronous backend, their default, the operation runs the synchronous
  backend at once and completes through `asio::post` on the handler's associated executor (never
  inside the initiating call). Decided so that code written once against the browser's family
  builds and runs everywhere; they cost microseconds, so a post blocks nothing. No other cheap
  function gets an `async_` variant: off the browser it would add a post and nothing else. An
  algorithm WebCrypto lacks fails to compile with "webcpp::hash: async_digest takes digests::sha1,
  digests::sha2_256, digests::sha2_384 or digests::sha2_512, the digests WebCrypto has: call
  digest_of for the others.", "webcpp::hash: async_hmac takes hmac_sha1_160, hmac_sha2_256,
  hmac_sha2_384 or hmac_sha2_512, the hmacs WebCrypto has: call mac for the others." and
  "webcpp::hash: async_hkdf takes hmac_sha1_160, hmac_sha2_256, hmac_sha2_384 or hmac_sha2_512,
  the hmacs WebCrypto has HKDF over: call hkdf for the others."
- **`async_pbkdf2` is heavy** (11.5): with a synchronous backend, its default, it runs on another
  thread, and exists so only where the build has threads; with `backends::webcrypto` named, on
  Emscripten, it awaits WebCrypto, with or without threads.
- **`default_async_backend<Algorithm, Hmac>` is `default_backend<Algorithm, Hmac>`** on every
  target (the owner's decision O2 of 2026-11-17, design D35, which replaces "WebCrypto on
  Emscripten for every algorithm it serves"): an asynchronous call computes on the backend its
  synchronous function would, and completes through a post (the cheap functions) or on the
  thread pool (the heavy ones). `backends::webcrypto` is used on Emscripten only when the type
  names it, as `async_pbkdf2<hmac_sha2_256, backends::webcrypto>`; it stays the outside oracle of
  every algorithm it has (7.4). `doc/capabilities.json` has no separate asynchronous default, and
  `capabilities_test` checks that `default_async_backend<X>` is `default_backend<X>` (ruling R76).
- **The synchronous functions never use `backends::webcrypto`:** naming it in `hkdf`, `pbkdf2`,
  `digest_of`, `hasher`, `mac` or `argon2` fails to compile, on every target, with "webcpp::hash:
  backends::webcrypto is asynchronous: call async_hkdf (or async_pbkdf2, async_digest, async_hmac)
  with a completion token, or name a synchronous backend." It is never a default (4.1). An
  asynchronous function that names it off Emscripten fails to compile with "webcpp::hash:
  backends::webcrypto exists only on Emscripten: name a synchronous backend, or leave the
  default." (`struct webcrypto;` is declared in `config.hpp` on every target, design D39.)
- **WebCrypto's executor (ruling R73).** With `backends::webcrypto` named, the executor must
  belong to an `io_context` run on the calling thread, whose `run` returns to the JavaScript event
  loop where WebCrypto's promise settles: a concrete executor whose context is not an `io_context`
  fails to compile with "webcpp::hash: backends::webcrypto completes through an io_context run on
  the calling thread, whose run returns to the JavaScript event loop where WebCrypto's promise
  settles: pass that io_context's executor, not a thread_pool's."; a type-erased one
  (`any_io_executor`) is checked at run time: an `io_context`'s executor with any of its
  properties (blocking, relationship, outstanding work: the eight combinations), and a strand of
  one, a type-erased strand checked through to its inner executor, are accepted; an `io_context`'s
  executor rebound to another allocator, and a concrete strand of a strand behind
  `any_io_executor`, are refused, since the erasure hides their type (pass the inner strand, or the
  `io_context`'s executor); another completes with `invalid_parameters`, posted to the handler's
  associated executor, `output` untouched. That the `io_context` runs on the calling thread
  cannot be checked: it is the program's duty, stated in the Doc Comment.
- **WebCrypto's limits (ruling R74).** An empty HMAC key is imported as one zero byte, which HMAC
  pads to the same key and WebCrypto takes where it refuses an empty one; `detail::backend_limits`
  gains `pbkdf2_iterations`, and `backend_limits<backends::webcrypto>` is at most 2^32 - 1
  iterations (`invalid_iterations`), an HKDF or PBKDF2 output of at most (2^32 - 1) / 8 bytes
  (`invalid_output_size`) and an HKDF info of at most 1,024 bytes, node's limit (`input_too_long`),
  documented as WebCrypto's limit (D26's precedent). PBKDF2 over another hmac than the four:
  "webcpp::hash: backends::webcrypto has PBKDF2 over hmac_sha1_160, hmac_sha2_256, hmac_sha2_384
  and hmac_sha2_512 alone: name a synchronous backend for the others."
- `<webcpp/hash/async.hpp>` needs webcpp::asio (`<library>/webcpp/asio//asio`), which the
  convenience header does not bring (chapter 3: an optional heavy dependency, named in its
  comment); `backends::webcrypto` needs nothing else on Emscripten and names no C library.

### 11.5 The heavy functions on another thread (the owner's decision, 2026-11-14)

The deliberately slow functions get asynchronous variants that truly run on another thread, and only
when the build has threads: `async_argon2id`, `async_argon2` (increment 2 on), `async_scrypt` and
`async_pbkdf2` (with a synchronous backend), and every later memory-hard or deliberately slow
function (Botan's bcrypt and bcrypt-pbkdf, increment 6, not built: their `async_bcrypt` and
`async_bcrypt_pbkdf` come with it, design D42). Separate from the browser's family, which exists on
Emscripten with or without threads.

```cpp
/** Argon2id on a thread of `work`'s context; completes with `void(boost::system::error_code)`. */
template <class Backend = default_backend<kdfs::argon2id>, class WorkExecutor, class CompletionToken>
auto async_argon2id(WorkExecutor const& work, std::span<std::uint8_t> output,
                    std::span<std::uint8_t const> password, std::span<std::uint8_t const> salt,
                    argon2id_parameters const& parameters, CompletionToken&& token);
/** Argon2 of `Variant` with every parameter (2.15), on a thread of `work`'s context. */
template <class Variant, class Backend = default_backend<Variant>, class WorkExecutor,
          class CompletionToken>
auto async_argon2(WorkExecutor const& work, std::span<std::uint8_t> output,
                  std::span<std::uint8_t const> password, std::span<std::uint8_t const> salt,
                  argon2_parameters const& parameters, CompletionToken&& token);
/** scrypt on a thread of `work`'s context. */
template <class Backend = default_backend<kdfs::scrypt>, class WorkExecutor, class CompletionToken>
auto async_scrypt(WorkExecutor const& work, std::span<std::uint8_t> output,
                  std::span<std::uint8_t const> password, std::span<std::uint8_t const> salt,
                  scrypt_parameters const& parameters, CompletionToken&& token);
// async_pbkdf2<Hmac, Backend> (above) with a synchronous backend runs on `executor`'s threads alike.
```

- **Threads detected by `config.hpp`.** webcpp's `<webcpp/config.hpp>` defines no threading macro
  (its macros are `WEBCPP_TARGET_*`, `WEBCPP_HAS_EXCEPTIONS`, `WEBCPP_NO_EXCEPTIONS`), so
  `config.hpp` defines `WEBCPP_HASH_HAS_THREADS` natively when Boost.Config's `BOOST_HAS_THREADS`
  is defined, which every native build defines, b2's default `threading=single` included, so that
  natively the heavy variants always exist (ruling R70); and on Emscripten when
  `__EMSCRIPTEN_PTHREADS__` is defined (`-pthread`, b2's `threading=multi`). Never on wasip2 (no
  threads) or wasip3 (its cooperative threads do not run in parallel, so a heavy function there
  would still block its caller; `BOOST_HAS_THREADS` is defined there too, so the target is tested
  first, `WEBCPP_TARGET_NATIVE` or `WEBCPP_TARGET_EMSCRIPTEN`). A developer may define
  `WEBCPP_HASH_NO_THREADS` to turn the heavy variants off in a build that has threads, which is
  how a native build shows the refusal.
- **Where the work runs.** The function guards first (errors complete through a post, as 11.1),
  then posts the work to `work`, an executor whose context runs on other threads (a
  `webcpp::asio::thread_pool`'s, typically), and completes on the handler's associated executor
  (Asio's default: `work` itself; a program binds its own event loop's with `bind_executor`). On
  Emscripten with pthreads each thread of the pool is a Web Worker, so the page never blocks, and
  the completion reaches the page's thread through the handler's executor
  (`webcpp::asio::browser::main_thread_context`'s, when bound). The spans and the parameters are
  the caller's until completion. Cancellation through the handler's slot: before the work starts,
  any type completes the operation at once (posted to the handler's associated executor) with
  `operation_aborted`, and the work never runs; after it starts, a `terminal` or `partial` one lets
  the work run to its end (no C library here can be interrupted), then `output` is zeroed and the
  completion is `operation_aborted`; a `total` one is then ignored. The guards run before the post
  and their refusal completes through a post to the handler's associated executor, `output`
  untouched. The operation's state is the handler's (6.1). `async_pbkdf2` with a synchronous
  backend is governed by this section (ruling R72): another thread, only with threads.
- **Without threads they do not exist:** each name is then a template whose `static_assert`
  says "webcpp::hash: async_argon2id runs on another thread and needs a build with threads
  (natively, remove WEBCPP_HASH_NO_THREADS; on Emscripten, link with -pthread). Without threads,
  call argon2id." (and the same for `async_argon2`, `async_scrypt` and `async_pbkdf2`), so a call
  fails to compile with the instruction; `async_pbkdf2` with `backends::webcrypto` on Emscripten is
  not refused. The text replaced "natively b2 threading=multi; on Emscripten -pthread", since
  natively every build has threads and only `WEBCPP_HASH_NO_THREADS` takes them away (ruling R70,
  design D40).
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
`post`, checked to never complete inside the initiating call, and on Emscripten too with
`backends::hash2` named (`async_post_test`). As built (increment 7): the `_no_exceptions` twins
check an error completion with a callback or `as_tuple(...)`, and use `use_future` on success paths
alone, since a future reports an error as an exception (ruling R71); Cobalt's `use_op` where
webcpp::cobalt is linked; a cancellation before the work starts, during it, racing its end (300
rounds) and of each type; a completion destroyed unrun, which gives the state back
(`check_never_completed`); the handler's allocator released before the handler runs
(`check_allocator_released`); WebCrypto's refusals through stubbed promises (a rejected promise,
a wrong result, no `crypto.subtle`, a cancellation between two promises); and
`config_threads_test` (2.2). `allocation_test` counts the family and `backends::webcrypto` (7.7).
