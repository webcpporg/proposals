## Context

webcpp::hash is a header-only C++20 wrapper library: one entry point for hashing and key
derivation, on native (Linux, macOS, Windows with MSVC), wasip2, wasip3 and Emscripten. The
owner proposed it and decided its shape on 2026-11-14, while webcpp::crypto's version 0.1 was
being built and the research behind crypto's roadmap (the noble and scure
libraries) showed what programs above crypto need:
Keccak-256 for Ethereum, HKDF for every key hierarchy, PBKDF2 for BIP39 seeds, Argon2id and
scrypt for passwords, BLAKE3 on demand. Boost.Hash2 1.92 already has SHA-1/2/3, SHAKE, RIPEMD,
BLAKE2 and HMAC, and none of these. The specification is `reference/spec.md`, the plan
`reference/plan.md`. Increment 1 (the plan's Tasks 0 to 24) is built in the library's own
repository and green on every local lane, not yet published (the owner's gates); D25 to D32
record what building it decided, and the spec and the plan are amended to match. D33 records the
owner's decision of 2026-11-16 that every key derivation defaults to `backends::hash2`, which
retires the measured defaults, the profiles and the overrides of D8, D20, D28 and D32. D34 to D36
record the owner's decisions of 2026-11-17 before increment 2 (O3 answered), and D37 to D42 what
building increments 2 to 5 and 7 decided, on local integration branches, and why increment 6 waits.

Constraints every decision works within, from the superproject's `CODING-GUIDELINES.md`:

- header-only C++20, namespace `webcpp::hash`, headers `<webcpp/hash/...>`, macros
  `WEBCPP_HASH_*`, include guards `WEBCPP_HASH_<PATH>_HPP`, b2 target `/webcpp/hash//hash`,
  error category `webcpp.hash` (chapter 3);
- what a library builds on, in order: webcpp, Boost, the C++ Alliance, the standard library, an
  outside library that follows them, a port of a widely used C library, and only then its own
  code; an outside library is wrapped (chapter 4);
- a C library comes in as a pinned submodule, built file by file by `third_party/Jamfile`, named
  by the program, with three layers that stop a wrong build (chapter 5);
- errors as values, exceptions optional through `WEBCPP_HASH_NO_EXCEPTIONS` (chapters 4 and 6);
- no global or static mutable state, as little allocation as the job allows (chapter 4);
- every test on every target, with exceptions on and off (chapter 8); English, no em dash, BSL-1.0
  with "Copyright (c) 2026 WebCpp.org" on every source file (chapter 2).

## Goals / Non-Goals

**Goals:**

- Boost.Hash2's algorithms and HMAC under webcpp::hash's names, as the Hash2 types themselves.
- Keccak-256 (Ethereum's) and BLAKE3 (with keyed hashing, key derivation and extendable output)
  in Hash2's HashAlgorithm form, so that `hash_append` and the rest of Hash2 work with them.
- HKDF and PBKDF2 over any Hash2 hmac, each on a backend chosen by a template parameter, with
  `backends::hash2` as the default on every target (D33).
- Argon2id and scrypt from libsodium.
- Proof by the RFCs' vectors, the C libraries' own suites, agreement between backends, WebCrypto
  as an outside oracle, a counting allocation test and a test for every message; the size table
  and the PBKDF2 throughput measured, kept as dated reference that decides no default (D33).

**Non-Goals:**

- Any primitive written by webcpp: every permutation and compression function is Hash2's or a
  C library's (the two constructions webcpp writes are D2 and D7).
- Encryption, signatures, randomness, constant-time comparison: webcpp::crypto's.
- In increment 1: what increments 2 to 7 add (D19). In any increment: a password-hash string
  format or a stored-hash verifier; a backend's functions that are not hashing or key derivation;
  WebCrypto in a synchronous function (D23).

## Decisions

### D1. A wrapper, and Boost.Hash2 re-exported by alias

`webcpp::hash::sha2_256` is `using sha2_256 = boost::hash2::sha2_256;`, and so for every
algorithm Boost.Hash2 1.92 has (MD5, SHA-1, the six SHA-2, the four SHA-3, SHAKE128/256, RIPEMD-128
and -160, BLAKE2b-512, BLAKE2s-256, FNV-1a, SipHash, xxHash, XXH3, the legacy MurmurHash3 and
SpookyHash) and for `hmac<H>` and its sixteen aliases; `digest`, `hash_append` and its family
and `get_integral_result` are brought in by using-declarations.

- **Why:** the owner's decision; an alias is the Hash2 type, so a user's `hash_append`
  overloads, Hash2's tests and Hash2's later fixes apply unchanged.
- **Alternatives:** wrapper classes (a second type for the same algorithm, which breaks
  `hash_append`'s customisation and doubles the documentation); leaving users to name
  `boost::hash2` directly (no single entry point, which the owner asked for).

### D2. `keccak_256` drives XKCP's Keccak-p[1600] with Hash2's own sponge

XKCP, the Keccak team's code package (CC0), does the permutation:
`lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c`, the one file of its "generic64" profile
(all 24 rounds unrolled, no lane complementing) on every target. webcpp's `keccak_256` absorbs,
pads with `0x01 ... 0x80` and squeezes exactly as Boost.Hash2's
`detail::keccak_base<0x01, 512, 256>` does, so that every sequence of `update` and `result`
gives Hash2's results. XKCP is reached through four C functions of webcpp's adapter,
`libs/hash/src/xkcp_adapter.c`, compiled into `/third_party//xkcp`.

- **Why the permutation alone:** the owner's plan is that `keccak_256` becomes an alias of a
  public Boost.Hash2 `keccak_256` (D3); Hash2's HashAlgorithm contract allows `update` after
  `result` and repeated `result` calls, and only Hash2's own sponge discipline makes the alias
  change nothing a program sees. XKCP's FIPS 202 interface (`Keccak_HashInstance`) refuses
  `update` after squeezing and squeezes the next bytes of the same block where Hash2 permutes
  again.
- **Why an adapter:** XKCP's headers include a generated `config.h` by that generic name; on a
  user's include path it would shadow or be shadowed by the user's own. The adapter keeps every
  XKCP header inside the C library's build.
- **Why generic64 everywhere:** wasm32 has native 64-bit integers, MSVC compiles the file with
  `_rotl64`, and XKCP's faster profiles are GNU assembly (AVX2, AVX-512) that MSVC and
  WebAssembly cannot build. Constant time does not matter for a public hash; correctness is
  proved by the vectors and Hash2's oracle on every target.
- **Alternatives:** XKCP's `KeccakHash.c` and `KeccakSponge.c` (results diverge from Hash2's
  after the first `result`); naming Hash2's `detail::keccak_base` (not a public interface, can
  change in any Boost release); libsodium's Keccak-f1600 state functions (libsodium 1.0.21 and
  later expose the state, but tie Keccak-256 to libsodium); webcpp's own permutation (refused by
  chapter 4).

### D3. A public `keccak_256` in Boost.Hash2 is the long-term plan

`keccak_base<0x01, 512, 256>` is already Keccak-256 inside Hash2. The plan prepares a
contribution of public `keccak_224`, `keccak_256`, `keccak_384` and `keccak_512` to Boost.Hash2;
once a Boost release has them, `webcpp::hash::keccak_256` becomes their alias and stops needing
`/third_party//xkcp`. Opening the contribution is the owner's outward action.

### D4. `blake3` over the official C implementation, in Hash2's form

`blake3` holds a `blake3_hasher` and calls `c/blake3.c` of BLAKE3 1.8.7. Hash2's form:
`result_type` is `digest<32>`, `block_size` 64, the three constructors (plain, `uint64_t` seed,
bytes seed) and `update`; `result()` returns the digest first and then successive 32-byte blocks
of the extendable output (as Hash2's `shake_*` return successive blocks), reset when input is
added; `squeeze(output, position)` reads any part of the extendable output; `keyed(key)` and
`derive_key(context)` give BLAKE3's two other modes. A bytes seed keys the hash with its first
32 bytes, zero-padded, the rest absorbed and finalized, as Hash2's `blake2b_512` keys with its
first 64.

- **Why:** the owner's decision; BLAKE3's C code is the reference, its `finalize_seek` gives
  the extendable output without a second state, and its update after finalize gives Hash2's
  semantics directly.
- **Alternatives:** a separate `blake3_xof` type (a second type for one state); refusing the
  seeded constructors (Hash2's contract requires them).

### D5. Key derivations take the backend as a template parameter, backends named after the library

`hkdf_extract`, `hkdf_expand`, `hkdf` and `pbkdf2` are
`template <class Hmac, class Backend = default_kdf_backend<Hmac, Kdf>>` function templates, `Kdf`
the tag of the operation itself (D25); `argon2id` and `scrypt` are
`template <class Backend = default_backend<kdfs::argon2id>>` (and `kdfs::scrypt`), which is
`backends::sodium` (until 2026-11-16 it also honoured or refused their overrides, retired by D33;
the default of every other key derivation is `backends::hash2`, D33). The backends are
`backends::hash2`, `backends::sodium`, `backends::bearssl`, `backends::openssl` (and later
`backends::botan`). The public functions do every guard once; a backend's header specialises
`detail::hkdf_engine<Backend, Hmac>`, `detail::pbkdf2_engine<Backend, Hmac>`,
`detail::argon2id_engine<Backend>` and `detail::scrypt_engine<Backend>` with the work alone. A
primary template, or a specialisation for an hmac a backend lacks, holds a `static_assert` whose
text names the backends that have the function (spec 3.3).

- **Why:** the owner's decision, webcpp::crypto's shape (its D1, D3), and the guards in one place
  are what lets the agreement tests compare backends.
- **Alternatives:** a run-time backend choice (a vtable, and every backend linked); one function
  per backend (guards that diverge).

### D6. BLAKE3's SIMD files on their instruction sets alone

`/third_party//blake3` compiles `blake3.c`, `blake3_dispatch.c` and `blake3_portable.c` on every
target; the SSE2, SSE4.1, AVX2 and AVX-512 intrinsics files natively on x86-64, each with its
instruction set's flag, and `blake3_neon.c` natively on arm64; WebAssembly builds get
`BLAKE3_NO_SSE2`, `BLAKE3_NO_SSE41`, `BLAKE3_NO_AVX2` and `BLAKE3_NO_AVX512`. BLAKE3's own
dispatcher chooses at run time.

- **Why:** the owner asked for "SIMD dispatch natively where available, portable on wasm"; the
  intrinsics files include x86 or Arm headers that no WebAssembly sysroot has, so chapter 5 rule
  2's "every file on every target" cannot hold for them. This is a recorded exception, in the
  Jamfile beside the sources and in chapter 10's row.
- **Alternatives:** the `.S` assembly (GNU as only; MSVC needs the `.asm` files and an assembler
  b2 does not configure); the portable file alone everywhere (several times slower natively for
  no reason).

### D7. `backends::hash2`: webcpp's HKDF and PBKDF2 over Hash2's hmac (the recorded departure)

webcpp writes RFC 5869's two steps and RFC 8018's PBKDF2 over `boost::hash2::hmac<H>`, generic
over every Hash2 hmac, on every target. It is a departure from "webcpp writes no cryptography",
recorded by the owner on 2026-11-14 as a construction over an audited HMAC, never a primitive,
with conditions the plan turns into tests and review items: the code is the RFCs' steps, under
150 lines for both, with the RFCs' names in comments; every RFC vector passes; it agrees with
every other backend on random inputs on every target where one exists; every intermediate is
wiped; it compares no secret.

- **Why:** it is the only backend on every target over every hash (BearSSL and libsodium cover
  a few SHA-2 hmacs, OpenSSL is not built for Emscripten, and neither BearSSL nor libsodium has
  PBKDF2), and BIP39 needs PBKDF2-HMAC-SHA-512 in the browser.
- **Alternatives:** Botan for PBKDF2 (not yet a dependency, and its WebAssembly build is
  unproved); mbedTLS (a third C library for one function); libsodium's internal
  `escrypt_PBKDF2_SHA256` (private, SHA-256 alone).

### D8. The default backend: the fastest natively, the smallest on WebAssembly, per KDF

> **Superseded on 2026-11-16 by D33.** No measurement decides a default any more: every key
> derivation defaults to `backends::hash2`, and argon2id and scrypt to `backends::sodium`.
> `detail/ranking.hpp` and the lane `measure` are retired. What still holds: the second
> parameter of `default_kdf_backend<Hmac, Kdf = kdfs::hkdf>`, its name and its signature.

`default_kdf_backend<Hmac, Kdf = kdfs::hkdf>`: the program's override or profile when it gives one
(D20); natively the first linked backend having the KDF over the hmac in the measured throughput
order (the owner's "OpenSSL when linked, else hash2", which the measurement confirms or reports to
him; for HKDF it did not, D32); on WebAssembly the first linked backend having it in the measured
size order of `detail/ranking.hpp`, generated by `tools/hash/measure.py` from one program per
algorithm per backend per target and checked by the own lane `measure`, which fails when a pin move
changes the order without the file being regenerated. D20 generalises this rule into the profiles.

- **Why the second parameter:** the owner's rule is per program size, and the backends differ
  per KDF: libsodium and BearSSL have HKDF and no PBKDF2, so one default for both would either
  be wrong for PBKDF2 or never consider them for HKDF. `default_kdf_backend<Hmac>` stays the
  owner's spelling for HKDF.
- **Alternatives:** a ranking written by hand (the owner asked for a measured one); one default
  for both KDFs.

### D9. One contract on every backend: the PRK of the digest size, info up to 32,768 bytes

`hkdf_expand` takes a pseudorandom key of exactly the hmac's digest size, and `hkdf_expand` and
`hkdf` refuse info longer than 32,768 bytes, on every backend. RFC 5869 allows a longer PRK and
any info; libsodium takes a PRK of exactly `KEYBYTES`, and OpenSSL 3.5.9 refuses info past
`HKDF_MAXINFO` (32 KiB, `providers/implementations/kdfs/hkdf.c` line 37 at `45e844f`).

- **Why:** a call must never succeed on one backend and fail on another, webcpp::crypto's rule
  (its D7, the GCM IV cap).
- **Alternatives:** per-backend limits (the default backend's choice would change behaviour).
- **The one exception** is a length a backend's C interface cannot hold at all: OpenSSL's past
  `INT_MAX` bytes, which `backends::openssl` refuses where `backends::hash2` accepts (D26). It is
  a refusal at a size no protocol uses, never a different result.

### D10. PBKDF2 on OpenSSL with `pkcs5 = 1`

OpenSSL's PBKDF2 can apply SP 800-132's lower bounds (iterations 1,000, salt 16 bytes, key 112
bits); the default provider's default is off, and webcpp sets it off explicitly, so that RFC
6070's one-iteration cases and short salts run on every backend and a provider's default can
never change the result.

### D11. libsodium as a pinned submodule, built file by file, shared with webcpp::crypto

`third_party/libs/libsodium` at 1.0.22 (`77e1ce5`), every `.c` of `src/libsodium/` compiled one
object per file by `third_party/Jamfile` as `/third_party//sodium`, with the defines
libsodium's own `configure` finds, run once per target and recorded (each `HAVE_*` only where
found, `DEV_MODE` off; MSVC from `builds/msvc/properties/`), `CONFIGURED=1`, `SODIUM_STATIC=1`,
`NATIVE_LITTLE_ENDIAN=1`, and without `HAVE_AMD64_ASM` and `HAVE_AVX_ASM`, so that its seven
`.S` files are not needed. `version.h`, which `configure` generates, is upstream's own
`builds/msvc/version.h`, copied by b2 into the build directory. Every engine calls
`sodium_init()` first, as libsodium requires; its process-wide state is libsodium's. The same
target serves webcpp::crypto's later libsodium backend, which adds its canary to it.

- **Why:** chapter 5; the owner's decision that libsodium is "one /third_party//sodium shared by
  both"; `configure` is upstream's own configuration step, so its defines are upstream's for
  each target (`build.zig`'s lists give some defines on every target unconditionally). Its own
  suite runs on every target, each run in a writable directory of the build, since `cmptest.h`
  writes `<name>.res` there.
- **Alternatives:** libsodium's autotools or its prebuilt Emscripten and WASI packages (not
  file by file, not one copy per program); the assembly (GNU as only; it speeds Curve25519 and
  Salsa20, which hash does not use, and webcpp::crypto's change reconsiders it).

### D12. Argon2id and scrypt from libsodium, which allocates their memory

`argon2id` calls `crypto_pwhash_argon2id` with `crypto_pwhash_ALG_ARGON2ID13`, `scrypt` calls
`crypto_pwhash_scryptsalsa208sha256_ll`. libsodium allocates the memory the costs name (`mmap`,
`malloc`) outside any allocator of the program's: a departure from chapter 4's allocation rule,
which the owner's decision foresaw ("with the allocation departure recorded"). The owner named
the alternative, the Argon2 reference implementation's `allocate_cbk`/`free_cbk`, which would
route Argon2id's memory through a `std::pmr::memory_resource`.

- **Why libsodium, decided:** it is the owner's choice of library; it is maintained and constant
  time, already pinned for HKDF, and gives scrypt too (the reference implementation has no
  scrypt, so scrypt would allocate outside the resource anyway); the memory of a memory-hard
  function is its whole purpose, of a size the caller chose, freed before the call returns, so a
  resource buys little; and a second Argon2 implementation would be a second copy of the same
  algorithm per program. If a user's resource must hold the memory, the reference
  implementation becomes a later backend, `backends::argon2`, which the template parameter
  already admits.
- **Limits recorded:** libsodium's Argon2id has one lane and takes no secret and no associated
  data, so `argon2id_parameters` has `passes` and `memory` alone; the memory must be a multiple
  of 1,024 bytes, which libsodium would otherwise round down silently.

### D13. RFC 9106's vector through libsodium's own Argon2 core

RFC 9106's Argon2id vector (section 5.3) uses four lanes, a secret and associated data, none of
which `crypto_pwhash_argon2id` takes. libsodium's internal `argon2_ctx`
(`src/libsodium/crypto_pwhash/argon2/argon2.h`) takes all three; a test, and only a test,
includes that private header and runs the vector through it, which proves that the core under
`argon2id` is RFC 9106's. The public function is then checked against libsodium's own
`pwhash_argon2id` cases and against `argon2_ctx` with one lane on random inputs.

### D14. Canaries and three layers, as webcpp::crypto's

`detail/canary.h` and `src/canary_<name>.c` for XKCP, BLAKE3, libsodium and BearSSL, with the
commit and configuration of spec 5.4 in each symbol, read by every function that calls the
library; the header's `#error` when the `HAS` define is missing; MSVC's `detect_mismatch`.
`/third_party//bearssl` gains webcpp::hash's canary file and `WEBCPP_HASH_HAS_BEARSSL=1` beside
webcpp::crypto's.

- **Why one canary per webcpp library:** chapter 5 rule 6 names the canary
  `webcpp_<lib>_<name>_...`; hash cannot include crypto's headers (crypto will depend on hash,
  never the reverse), and each library's canary checks the configuration that library needs.

### D15. OpenSSL through `/webcpp/hash//openssl`, a hash-owned alias of the one provider

`/webcpp/hash//openssl` links `/webcpp/third_party//openssl` and defines
`WEBCPP_HASH_HAS_OPENSSL=1` on native, wasip2 and wasip3, and stops Emscripten with the fix, as
`/webcpp/crypto//openssl` does.

- **Why not crypto's alias:** hash cannot depend on crypto (D14), and crypto's alias defines
  crypto's macro. Both aliases link the same archives (chapter 5 rule 10).

### D16. Errors, spans and synchronous calls

Every function returns `boost::system::result<T>`; the values of `error` (1 to 7) are fixed; a
failed allocation in a C library is `errc::not_enough_memory`; inputs are
`std::span<std::uint8_t const>`, outputs `std::span<std::uint8_t>`; nothing throws, with or
without exceptions; a guard's refusal leaves the output untouched, a backend's failure zeroes it.

### D17. Tests: vectors, suites, agreement, an outside oracle, messages, allocation

RFC 5869, 6070, 7914 and 9106; Keccak-256's published digests, EIP-55's examples and Hash2's
`keccak_base` as an oracle; BLAKE3's `test_vectors.json`; libsodium's, XKCP's and BLAKE3's own
suites on webcpp's build on every target, all in the own lane `c-suites` (D29); every backend
against every other on random inputs natively, hash2 against sodium and bearssl (and OpenSSL on
WASI) on WebAssembly; WebCrypto under node on Emscripten; a test per message in a directory of its
own; a counting `operator new`; the user's Jamfiles as fixtures; `webcpp.same-program` per C
library; exceptions on and off; every lane.

### D18. The measurements the owner asked for are tasks of the plan

> **Superseded in part on 2026-11-16 by D33.** The measured table no longer orders the backends
> or decides a default: `doc/measurements.json` and `doc/measurements.adoc` stay as a dated
> reference measured on 2026-11-16, and no lane re-measures them. What still holds: the
> measurements were made, and the PBKDF2-HMAC-SHA-512 throughput of `doc/performance.adoc`.

The measured table (D8, D20) and PBKDF2-HMAC-SHA-512 at 1,000,000 iterations for `backends::hash2`,
OpenSSL and libsodium. libsodium has no PBKDF2, so its line measures RFC 8018's loop over
`crypto_auth_hmacsha512_*` in the benchmark program, never in the library: the measurement of
libsodium's HMAC-SHA-512 in PBKDF2's shape.

### D19. Every backend's whole hashing and KDF surface, in increments by value

The owner widened the scope the same day: every backend exposes everything its library offers
that belongs to hashing and key derivation, as far as it can. The work is cut into seven
increments, the highest value first (spec 1.5): 1, what Ethereum, Bitcoin, Nostr and key
hierarchies need now; 2, libsodium's whole surface (BLAKE2b with key, salt, personalisation and
length, SipHash-2-4 and -2-4x, `crypto_kdf` subkeys, Argon2i, SHAKE and TurboSHAKE, its SHA-2,
SHA-3 and HMACs for agreement); 3, XKCP's Keccak family (Keccak-224/384/512, cSHAKE, KMAC,
TupleHash, ParallelHash, KangarooTwelve, TurboSHAKE); 4, OpenSSL's (every EVP digest of the
default provider, KECCAK-224 to -512 included, KMAC, BLAKE2 MACs, scrypt, Argon2d/i/id with every
RFC 9106 parameter, KBKDF, SSKDF, X9.63); 5, BearSSL's hashes and HMAC; 6, Botan 3's (after
webcpp::crypto provisions it); 7, the browser backend (D23). A capability table (spec 3.5),
kept as `doc/capabilities.json` and compared by a test with `backend_has_v`, says which backend
provides each function; every refusal names the backends that have it.

- **Why this order:** each increment reuses a library already linked before it brings a new one
  (libsodium and XKCP are in increment 1), and the earlier ones serve the protocols the owner's
  research found first.
- **Alternatives:** one increment with everything (no usable stop on the way); only what a known
  user needs (the owner asked for as much as each library has).

### D20. One profile changes every default; overrides per algorithm; a measured table

> **Superseded on 2026-11-16 by D33.** Every profile (`WEBCPP_HASH_PROFILE_FASTEST`, `_SMALLEST`,
> `_PORTABLE`, `_BACKEND_<NAME>`), hash's reading of webcpp's umbrella `WEBCPP_PROFILE_*`, the
> per-algorithm overrides `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`, MSVC's
> `detect_mismatch` of the profile and of the overrides, `detail/ranking.hpp` and the lane
> `measure` are retired. Nothing of this decision still holds but its history; the measurements
> it named are a dated reference (D18).

`config.hpp` takes exactly one profile define: none (`auto`, the owner's rule: fastest natively,
smallest on WebAssembly), `WEBCPP_HASH_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE` (the same code
on every target: hash2, xkcp, blake3, sodium, bearssl, in that order) or `_BACKEND_<NAME>`; and
per-algorithm overrides `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`. The defaults are computed from
the build (target, `HAS` macros) and `detail/ranking.hpp`, generated by `tools/hash/measure.py`
from webcpp's measurements (program size per target, throughput natively and where it runs on
WebAssembly), committed, checked by the own lane `measure` for sizes and re-measured by hand for
throughput at every pin move, a committed order changing only past a 10 percent difference in the
median (a first measurement orders by the medians alone, D32).
MSVC's `detect_mismatch` guards the profile and each override. One macro per value; webcpp's
umbrella `WEBCPP_PROFILE_FASTEST`, `_SMALLEST` and `_PORTABLE` sets the profile of every
library that reads it (hash now, crypto by its later change), and the overrides are spelled
`WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`, as crypto's
`WEBCPP_CRYPTO_DEFAULT_BACKEND_<FAMILY>_<NAME>`. Precedence: per-algorithm override, library
profile, umbrella profile, the build's measured default; two profiles of one level are an
`#error` naming both (the coordinator's ruling of 2026-11-15).

- **Why:** the owner's decision; one switch answers "make this program small" or "make it fast"
  without knowing every algorithm's backend, and an override handles the exception.
- **The same idea in webcpp::crypto** is a later change of crypto's, recorded as an impact.
- **Alternatives:** a run-time choice (state, and every backend linked); a constant-expression
  configuration object (a program would have to name it in every call); a hand-written order (the
  owner asked for measurements).

### D21. Two shapes: Hash2's form for the canonical types, a family for every backend

Hash2's HashAlgorithm contract (repeated `result`, `update` after `result`, seeded constructors)
fits Hash2's own algorithms, `keccak_256` and `blake3`, which stay the types to use with
`hash_append`. The other backends' hashes, MACs and KDFs, and the algorithms with parameters
(BLAKE2b's key, salt and personalisation, cSHAKE's names, KMAC's customisation), are reached
through four families, `digest`, `hasher` (one finalisation, then `invalid_state`), `mac` and the
KDF functions, each `template <class Algorithm, class Backend = default_backend<Algorithm>>`
over the tags of `digests`, `macs` and `kdfs` (spec 2.15). The agreement tests compare the first
result of each.

- **Alternatives:** Hash2's form for every backend (OpenSSL's and libsodium's contexts cannot
  absorb after finalising, so repeated results would need a construction of webcpp's); a
  separate type per backend and algorithm (hundreds of types).

### D22. Documentation in noble's and libsodium's manner, licences respected

One short section per algorithm (what it is, an example that is a built program, its pitfalls),
in the manner of noble's README (MIT; its style, not its text); libsodium's documentation (ISC,
`jedisct1/libsodium-doc` at `6d1ea27`) adapted with attribution, its copyright notice and
licence kept in the library's `NOTICE.md`; Wikipedia (CC BY-SA, incompatible with BSL-1.0) cited,
never copied. Teaching chapters by use case, preferring peer-to-peer networks, cryptocurrencies
and privacy, framed as raising the security of what a reader builds; examples are webcpp's own
code, and an application built on noble is named only when it is public.

### D23. WebCrypto through a parallel asynchronous family (the owner's answer to O1)

> **Amended on 2026-11-17 by D35.** `default_async_backend` is `default_backend` on every target:
> WebCrypto serves the family on Emscripten only when the type names it. D39 records how
> `backends::webcrypto` was built.

`backends::webcrypto` (Emscripten) serves `async_digest`, `async_hmac`, `async_hkdf` and
`async_pbkdf2` in `<webcpp/hash/async.hpp>`, Asio asynchronous operations initiated through
`async_initiate`, so that a callback, `use_awaitable`, Cobalt's tokens and `use_future` all work;
the promise is awaited through `webcpp::asio::browser::async_await`, whose bridge keeps the
operation alive, honours the handler's allocator, executor and cancellation slot, and posts
completions. On Emscripten `default_async_backend` is the browser for every algorithm it serves.
The synchronous functions never use it (a compile-time refusal says to call the async function).
The family is offered on every target for those algorithms: off Emscripten, or with a
synchronous backend named, it runs that backend at once and completes through `asio::post`.

- **Why offered everywhere:** code written once against the asynchronous family builds and runs on
  every target, and the cost is a post.
- **Alternatives rejected by the owner:** JSPI or Asyncify suspension under the synchronous calls
  (a suspension per call, a larger program under Asyncify, impossible where no promise may be
  awaited, as in an audio worklet); WebCrypto as an oracle only (it stays as the oracle of every
  algorithm, spec 7.4, but not as the only use); not offering the family off Emscripten (code
  that uses it would not port).
- **Dependency:** `async.hpp` needs webcpp::asio; the convenience header leaves it out and says
  so (chapter 3).
- **Cheap functions only where the browser serves them:** `async_digest`, `async_hmac` and
  `async_hkdf` exist for WebCrypto's algorithms alone, completing by post off Emscripten for
  uniformity; any other cheap function would gain only a post.

### D24. The heavy functions run on another thread, only in a build with threads

> **Amended by D40.** `BOOST_HAS_THREADS` is defined in every native build, b2's `threading=single`
> included, so natively the heavy functions always exist unless `WEBCPP_HASH_NO_THREADS`; their
> refusal names that define in place of "natively b2 threading=multi".

The owner decided on 2026-11-14 that Argon2, scrypt, PBKDF2 with a synchronous backend and every
later deliberately slow function get `async_` variants that truly run on another thread: natively
with `threading=multi` on an executor the caller gives (a `webcpp::asio::thread_pool`'s),
completing through the completion token; on Emscripten with pthreads on a Web Worker, so the page
never blocks. `config.hpp` defines `WEBCPP_HASH_HAS_THREADS` from Boost.Config's
`BOOST_HAS_THREADS` natively and `__EMSCRIPTEN_PTHREADS__` on Emscripten (webcpp's own config
defines no threading macro), never on WASI (wasip3's cooperative threads are not parallel).
Without threads the names are templates whose `static_assert` says the build needs threads and
which synchronous function to call. This is separate from D23's browser family, which exists on
Emscripten with or without threads; `async_pbkdf2` belongs to both: WebCrypto by default on
Emscripten, a pool thread with a synchronous backend.

- **Alternatives:** heavy functions completing by post without threads (rejected: an
  asynchronous name that blocks the event loop for seconds); a thread per call (unbounded, and
  not the program's executor); detecting threads at run time (a program without a thread library
  cannot link the pool at all).

### D25. HKDF's two steps are public tags, and the default backend is chosen per operation

> **Superseded in part on 2026-11-16 by D33.** The default is no longer chosen per operation from
> a profile's order or a measured row: it is `backends::hash2` for `hkdf`, `hkdf_extract`,
> `hkdf_expand` and `pbkdf2` alike, and there is no override. What still holds: the public tags
> `kdfs::hkdf`, `kdfs::hkdf_extract`, `kdfs::hkdf_expand`, `kdfs::pbkdf2`, `kdfs::argon2id` and
> `kdfs::scrypt`; each function's default named through its own tag; `backend_has_v` answering
> for the two steps (false for `backends::bearssl`); and BearSSL's HKDF refusing the steps at
> compile time when named.

`kdfs` declares a tag per operation of increment 1: `hkdf` (both steps at once, the function
`hkdf`), `hkdf_extract`, `hkdf_expand`, `pbkdf2`, `argon2id` and `scrypt`. `default_backend<
Algorithm, Hmac>` (and `default_kdf_backend<Hmac, Kdf>`, the same) is chosen for the operation
the call makes: the override's backend, else the first backend of the profile's order that the
program links and that has that operation over that hmac (`backend_has_v`). `hkdf_extract`
defaults on `default_kdf_backend<Hmac, kdfs::hkdf_extract>`, `hkdf_expand` on
`kdfs::hkdf_expand`, `hkdf` on `kdfs::hkdf`. Each step has its own row in the measured table and
in `detail/ranking.hpp`. An override names an algorithm, not a step:
`WEBCPP_HASH_DEFAULT_BACKEND_HKDF_<NAME>` applies to all three HKDF operations, and with `BEARSSL`
a call of `hkdf_extract` or `hkdf_expand` refuses at compile time with spec 3.3's text, as any
override whose backend lacks the function does.

- **Why:** BearSSL has HKDF only whole (spec 3.2), and on every WebAssembly target its `hkdf` is the
  smallest program, so `auto` puts it first there; one default per algorithm would make
  `hkdf_extract` and `hkdf_expand` refuse wherever the order puts `backends::bearssl` first. Chosen
  per operation, a default never names a backend that lacks the call.
- **Alternatives:** one default per algorithm (the steps refuse under the smallest order); never
  ranking `backends::bearssl` for HKDF (its smallest `hkdf` lost); private tags (a program could
  not ask `default_backend` or `backend_has_v` about a step it calls).

### D26. A backend's narrower lengths are its limits: OpenSSL refuses past `INT_MAX`

OpenSSL 3.5.9 holds some lengths in an `int`: HMAC's key (HKDF's salt and PRK, PBKDF2's password),
PBKDF2's salt and PBKDF2's output (`tkeylen` in `pbkdf2_derive`). A PBKDF2 output of 2^31 bytes
crashed it in review (a negative length reached `memcpy`, SIGSEGV), natively and on wasm32. So a
backend whose C interface holds a length in a type narrower than `std::size_t` declares its limits
in `detail::backend_limits<Backend>` (`detail/engines.hpp`), one member per input and output of
each operation; the primary template, every other backend's, has no limit beyond the spec's. The
public functions read it after the spec's own guards and before any call of the backend, and
refuse a longer output with `invalid_output_size` and a longer input with `input_too_long`, the
output untouched. `backends::openssl` specialises it with `INT_MAX` for HKDF's salt and PRK and
PBKDF2's password, salt and output; its documentation states them. `backends::hash2` accepts
these lengths.

- **Why:** a crash is never an answer to a valid call; a refusal at the boundary is the guards'
  contract (D16), and the limit belongs to the one backend whose C interface has it.
- **Alternatives:** OpenSSL's limit on every backend (every backend's contract narrowed to one C
  library's types); feeding OpenSSL in pieces (a key, a salt and an output are each one
  parameter, which cannot be split); leaving it to OpenSSL (it crashes rather than refuse).
- **Tests:** natively, spans of the limit plus one, never dereferenced, for each guard, and a
  derivation within the limit; the C library is never run past its limit.

### D27. libsodium carries `-sALLOW_MEMORY_GROWTH=1` on Emscripten

`/third_party//sodium` gives a program that links it `-sALLOW_MEMORY_GROWTH=1` on Emscripten, as
a usage requirement. Argon2id and scrypt allocate the memory their costs name (64 MiB for
libsodium's interactive Argon2id costs), past Emscripten's fixed 16 MiB, and libsodium's own
tests link with the flag.

- **Why:** a user who names libsodium for Argon2id or scrypt gets a program that works without
  knowing the flag.
- **Consequence:** naming `/third_party//sodium` changes an Emscripten program by that flag
  alone. The "named and not called adds no byte" proof on Emscripten
  (`unused_sodium_emscripten`) gives the program without libsodium the flag explicitly and
  requires the two to be identical, so the flag is shown to be the only difference.
- **Alternatives:** the flag left to the user (a program that names libsodium and calls Argon2id
  fails at run time); a fixed larger memory (a size for every program that names libsodium,
  never enough for the sensitive costs).

### D28. The overrides' refusals live behind `kdf.hpp`, not `config.hpp`

> **Superseded on 2026-11-16 by D33.** The overrides are retired, and with them the generated
> `detail/overrides.hpp`, `tools/hash/measure.py` (deleted whole), `overrides_list_test` and their
> `#error`s and `detect_mismatch`. Nothing of this decision still holds but its history.

`config.hpp` keeps the profile's refusals (two library profiles, two umbrella profiles,
`WEBCPP_HASH_PROFILE_BACKEND_OPENSSL` and `/webcpp/hash//openssl` on Emscripten) and the
profile's `detect_mismatch`. The per-algorithm overrides, one block per algorithm of
`doc/capabilities.json` and one define per backend that has it, are in
`detail/overrides.hpp`, generated by `tools/hash/measure.py --overrides` from that file and
checked by `overrides_list_test`: two overrides of one algorithm and an OpenSSL override on
Emscripten are its `#error`s, and on MSVC each algorithm carries
`#pragma detect_mismatch("webcpp_hash_default_backend_<algorithm>", ...)`, `"none"` without an
override. `kdf.hpp`, the one reader of the overrides, includes it, so every key derivation header
brings the refusals.

- **Why:** the list grows with the capability table in every increment; generated from it, it
  cannot drift from the backends that exist, and it stays out of the header every program
  includes first.
- **Consequence:** two overrides of one algorithm stop a translation unit that includes a key
  derivation header, the only place a default backend is read, not one that includes only
  `config.hpp` or `algorithms.hpp`.
- **Alternatives:** the refusals written by hand in `config.hpp` (a list kept beside a table by
  hand, in the header every translation unit reads).

### D29. BLAKE3's own test program runs in the lane `c-suites`

BLAKE3's C test is `c/test.py`, which runs `c/blake3`, a binary built inside the submodule. The
plan's `tools/test/blake3_c_suite.py`, a tools test that built a fixture on a scratch superproject
per target, was replaced in review by programs of the own lane `c-suites` in
`libs/hash/test/c_suites/`: `blake3_main.c` compiles BLAKE3's `c/main.c`, unmodified, with its
`main` and its standard input and output renamed; `blake3_c_suite.cpp` drives it over every case
of `test_vectors.json` as `c/test.py` drives `c/blake3` (no argument, `--keyed`, `--derive-key`,
each with `--length` of the whole output) and compares every line; `cpu_features.c` defines, off
x86, the two names `main.c` reads that BLAKE3's dispatcher defines on x86 alone. The library is
built with `BLAKE3_TESTING` (`<webcpp-blake3-testing>on`, a build of its own), and
`blake3_c_suite_corrupted`, the same program with one expected line changed, passes only when
exactly that line is reported.

- **Why:** a script run by no lane was run by no CI job either; in the lane, the program runs on
  every target through the lane's launcher, as libsodium's and XKCP's suites do, and a library's
  test Jamfile needs no raw `exe`.
- **Alternatives:** the plan's script (a second runner per target outside every lane).

### D30. Every result-returning function is `[[nodiscard]]`

`boost::system::result` carries no `[[nodiscard]]` of its own, so a dropped result, a guard's or a
backend's refusal at run time, would pass as a success and leave the output unwritten. Every
function of `include/` that returns `boost::system::result` is `[[nodiscard]]`, and
`libs/hash/meta/lint.json` opts the library into the lint's rule "nodiscard results" with that
reason, so that a function without it fails the lint at its line.

- **Alternatives:** the attribute by convention only (the next function forgets it); a wrapper
  result type (not the error model of chapters 4 and 6).

### D31. Each chapter opens with two comparison tables, each under eight columns

The guide's chapter of each namespace (digests, MACs, key derivations, password hashing, the
non-cryptographic hashes) opens with two tables generated by `tools/hash/doc_tables.py` from
`doc/capabilities.json` and the measurements: "What each of X is" (Name, Output, Security,
Standard, Status, In the browser) and "How to choose among X" (Name, the two measured columns
when there are measurements, Where it is used, Choose it when). Each is led by the name and has
fewer than eight columns; `hash_doc_tables_test` checks every generated table, with and without
measurements.

- **Why:** the site's style reads a table of eight columns or more as a matrix, which scrolls in
  its box instead of stacking; the first tables, of ten columns, scrolled at 1,199 pixels, two of
  them at 1,440 too.
- **Alternatives:** one wide table (a matrix, scrolled sideways on every width); fewer facts per
  function (the owner's documentation requirements ask for them).

### D32. The measured native HKDF order is provisional

> **Superseded on 2026-11-16 by D33.** No order is committed or compared any more, so none is
> provisional, and no re-measurement is due before a release; O2 is closed. What still holds: the
> facts it records about increment 1's measurement (one loaded macOS arm64 machine, HKDF's native
> medians under 1 percent apart), which D33 cites among its reasons.

The committed measurements were made on one machine, macOS on arm64 (Apple clang 21), shared and
loaded (`doc/measurements.json` records `loaded: true`). The order follows spec 4.2's rule alone: by
the median of the invocations' medians, a first measurement by the medians alone, and a committed
order changing later only past 10 percent. For HKDF over `hmac_sha2_256` the native medians are
libsodium 1,540 ns, hash2 1,554 ns and OpenSSL 1,564 ns, each under 1 percent from the next, so the
native order (libsodium, hash2, OpenSSL) rests on differences the load can reverse, and differs from
the expectation recorded in D8, "OpenSSL when linked" (PBKDF2's native order, OpenSSL first, rests
on OpenSSL being 1.7 to 3.6 times faster). The committed native orders carry the native toolchain's
identity (compiler, operating system, processor, strip); a lane on another native toolchain builds
and checks every program and skips the native order comparison with one printed line. WebAssembly
orders are always compared.

- **Before a release:** the throughput is re-measured on an idle machine and the orders
  regenerated (`python3 tools/hash/measure.py --write`).
- **Open question:** whether one native order, measured on macOS arm64, serves Linux and Windows
  (and x86-64), or each platform gets its own order (see Open Questions).
- **Alternatives:** a tie clause of the implementation's own (rejected: the spec's rule alone
  decides, and a noise rule is the spec's to give); the recorded expectation kept against the
  measurement (the owner asked for a measured order).

### D33. Every key derivation defaults to `backends::hash2`; no measurement decides a default

> **Extended on 2026-11-17 by D34 and D35.** D34 gives the families' tags their defaults by the
> same reasoning; D35 replaces the last bullet's `default_async_backend` (WebCrypto on Emscripten)
> by `default_backend` on every target. Increment 4 gave `argon2id` and `scrypt` OpenSSL's engines,
> reached by naming them: `backends::sodium` stays their default.

The owner decided on 2026-11-16 to replace the measured default (D8, D20, D25's orders, D32) by a
fixed one. Increment 1 had built the rule as specified: a profile per program, an override per
algorithm, and orders of backends by size and throughput generated into `detail/ranking.hpp` from
`doc/measurements.json` and checked by the own lane `measure`. Its measurements showed that the
rule bought little: for HKDF the native medians of libsodium, hash2 and OpenSSL were under 1
percent apart on a loaded machine (D32), and on WebAssembly the backends' isolated programs
differed by a few KB; a default that followed them could change with a machine's load, a pin move
or the set of libraries a program happened to link.

- **The default.** The default backend of every key derivation, `hkdf`, `hkdf_extract`,
  `hkdf_expand` and `pbkdf2`, over every hmac, on every target and whatever the program links, is
  `backends::hash2`. `backends::sodium` is the default only for what hash2 does not offer,
  `argon2id` and `scrypt`, of which it is the only backend; a program that calls them without
  linking libsodium stops with libsodium's "needs" message (spec 5.7), which the primary template
  of the default backend's engine in `detail/engines.hpp` raises for `backends::sodium`, tested by
  the messages `argon2id_default_not_linked` and `scrypt_default_not_linked`.
- **The capability table names each default.** `doc/capabilities.json` (layout version 3) gives
  each function row a `"default"`: `hash2` for `hkdf_extract`, `hkdf_expand`, `hkdf` and `pbkdf2`,
  `sodium` for `argon2id` and `scrypt`, `null` for a type; `capabilities_test` compares it with
  `default_backend`, and `tools/hash/doc_tables.py` renders it. The backends' `"portable"` field
  and the algorithms' `"overridable"` field are removed.
- **The other backends are explicit choices.** `backends::bearssl` and `backends::openssl`, and
  `backends::sodium`'s HKDF, stay and are tested as before (their vectors, the agreement tests,
  their limits and refusals), but a program reaches them only by naming them in the type, as
  `hkdf<hmac_sha2_256, backends::openssl>`; none is ever a default.
- **The public names stay.** `default_backend<Algorithm, Hmac = void>` and
  `default_kdf_backend<Hmac, Kdf = kdfs::hkdf>` keep their names and signatures, and the tags
  `kdfs::hkdf_extract` and `kdfs::hkdf_expand` stay, so that `backend_has_v` answers for the steps;
  the aliases' answer no longer depends on the hmac, the libraries linked or the target.
- **No define changes a default.** A former profile or override define has no effect;
  `default_backend_former_defines`, `default_backend_test` built with
  `WEBCPP_HASH_PROFILE_FASTEST`, `WEBCPP_HASH_PROFILE_SMALLEST`,
  `WEBCPP_HASH_PROFILE_BACKEND_BEARSSL`, `WEBCPP_PROFILE_FASTEST`,
  `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_SODIUM` and `WEBCPP_HASH_DEFAULT_BACKEND_PBKDF2_BEARSSL` with
  every library linked, proves it on the four targets.
- **Why hash2:** it is the only backend that covers every hmac, every operation and every target
  (D7); it is header-only, so a program carries only the code it calls and shares the SHA code of
  Boost.Hash2 it already uses; and the measured differences that would have chosen another
  backend are nanoseconds natively for HKDF and a few KB isolated on WebAssembly. A default that
  is the same everywhere is one a reader of the call can name without knowing the build.
- **Why the overrides go too** (the implementer's decision, recorded): with the backends named in
  the type, an override that changes what `hkdf<Hmac>` means across a whole program would make
  `backends::bearssl` or `backends::openssl` a default again, against this decision, and would hide
  the backend from the reader of the call; and `argon2id` and `scrypt` have one backend, so their
  overrides choose nothing.
- **Retired:** every profile, `WEBCPP_HASH_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE` (which now
  equals the default, since hash2 exists on every target) and `_BACKEND_<NAME>` (which would make
  another backend a default); hash's reading of webcpp's umbrella `WEBCPP_PROFILE_*`; MSVC's
  `detect_mismatch` of the profile (`webcpp_hash_profile`) and of the overrides
  (`webcpp_hash_default_backend_<algorithm>`), since a fixed default cannot differ between two
  translation units; the per-algorithm overrides
  `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>` with the generated `detail/overrides.hpp` and
  `overrides_list_test`; the generated `include/webcpp/hash/detail/ranking.hpp` and the code that
  reads it; the profiles' tests and messages; the CI's measure lane, the matrix entry "Own lane
  (hash, measure)" and `libs/hash/test/measure`; `tools/hash/measure.py` whole (`--write`,
  `--check`, `--capabilities`, `--overrides`, `--refusals`) with `tools/test/hash_measure_test.py`;
  and, used by the measure lane alone, `tools/ci/matrix.py`'s `EVERY_TARGET_LANES` (an own lane
  naming no target that builds every target) and `tools/lint/compile_commands.py`'s handling of a
  native lane's WebAssembly commands, each with its test.
- **Kept:** `doc/measurements.json`, its records and orders unchanged, its about and layout text
  relabelled as a reference measured on 2026-11-16 that decides nothing and its digest recomputed;
  `doc/measurements.adoc`, now rendered from it by `tools/hash/doc_tables.py` without the orders and
  the default of each key derivation, with that label and why hash2 is the default; `doc_tables.py`
  also renders `doc/backends.adoc` from `doc/capabilities.json`, and its `--check`, run at every
  documentation build, fails when `measurements.json`'s digest does not match its content (a hand
  edit of the reference); the guide's comparison tables still read the measurements (D31); the
  PBKDF2-HMAC-SHA-512 throughput of `doc/performance.adoc` (D18), its times measured by hand with
  `bench/collect.py`, while the benchmark's correctness program `pbkdf2_sha512_key` (its derived
  keys checked, never its time) runs in every lane of `libs/hash/test`; the guide's chapter on the
  profile, now `choosing_backend.adoc` (anchors `hash-choosing-backend` and
  `hash-default-backend`), with `guide.json`'s use-case field `profile` renamed `backend` (layout
  version 2); `doc/capabilities.json` and
  `capabilities_test`; every backend with its tests; and the asynchronous family's
  `default_async_backend` (D23, increment 7), which this decision does not change: WebCrypto on
  Emscripten for the algorithms it serves, else `default_backend`.
- **Alternatives:** keeping the measured rule (a default that follows a loaded machine's
  medians, a pin move or the link set, guarded by a lane that re-measures on four targets and an
  open question per platform, O2, for differences of nanoseconds and a few KB); keeping the
  overrides without the profiles (one define would still turn `hkdf<Hmac>` into BearSSL's or
  OpenSSL's across a program, unseen at the call, and would choose nothing for argon2id and
  scrypt).

### D34. A family's default: the first of hash2, xkcp, blake3 and sodium that offers the tag

The owner decided on 2026-11-17, answering O3 as it proposed, that the default backend of an
algorithm of the families (spec 2.15) that Boost.Hash2 lacks is fixed by one stated rule, in D33's
spirit, and never measured.

- **The rule.** The default of a tag is the first of `backends::hash2`, `backends::xkcp`,
  `backends::blake3` and `backends::sodium`, in that order, that offers it; `backends::bearssl`,
  `backends::openssl`, `backends::botan` and `backends::webcrypto` are never defaults, only named
  in the type. A backend offers a tag when its engine serves the family's whole contract for it:
  every option and every output size the family's guards accept. If none of the four offers it
  whole, the default is the first of them with a partial cell (`kdfs::argon2i` and
  `kdfs::argon2id` on libsodium, one lane; ParallelHash on XKCP, a block size that is a power of
  two of at least 8 bytes); if none of the four has a cell, the tag has no default (the
  controller's ruling R2).
- **No default is a type.** `detail::no_default`, whose engines refuse at compile time with
  "webcpp::hash: <qualified tag> has no default backend. Name one of: <the backends of its row>."
  (one message test per family), and `"default": null` in `doc/capabilities.json`. `kbkdf`,
  `sskdf`, `x963kdf`, Argon2d, SM3 and HMAC over Keccak have OpenSSL's cell alone, and MD5-SHA1
  BearSSL's and OpenSSL's: none has a default, and a call names its backend.
- **Written once.** The families' task (plan Task 25) wrote every default from the table as it
  stands once increments 2, 3, 4, 5 and 7 have landed, and no default changes between increments:
  a default whose backend's engine lands in a later stream refuses at compile time until it lands,
  first with the library's "needs" text and then with the backends that have the tag (ruling
  R84). The table is spec 4.1's.
- **One default whose engine is not built.** `macs::blake2b_mac` defaults to `backends::sodium`
  (libsodium's keyed BLAKE2b), but no increment built that engine; until one does, a call that
  names no backend refuses at compile time and names `backends::openssl`, its one cell.
- **Why:** D33's reasons hold for the families. One rule that a reader of the call can apply
  without knowing the build; hash2 first, header-only and on every target, so that every algorithm
  Boost.Hash2 has keeps it; XKCP and BLAKE3 next, the reference code of the Keccak team and of
  BLAKE3's authors, small and on every target; libsodium last of the four, already linked by a
  program that hashes passwords. BearSSL, OpenSSL, Botan and WebCrypto are reached for a reason
  the call states (a smaller program, OpenSSL's speed or its standards' KDFs, Botan's long tail,
  the browser), never by default.
- **Alternatives:** a measured order per family (D33's reasons against it); no default for a tag
  Boost.Hash2 lacks (every call of Keccak-256 or BLAKE2b with a salt would name its backend).

### D35. The asynchronous family's default is the synchronous one

The owner decided on 2026-11-17 (O2 of that day) that `default_async_backend<X>` is
`default_backend<X>` on every target, which replaces D23's "On Emscripten `default_async_backend` is
the browser for every algorithm it serves" and the sentence of D33's last bullet that kept it.

- **What runs.** The cheap functions (`async_digest`, `async_hmac`, `async_hkdf`) run the
  synchronous default backend at once and complete through `post` (spec 11.2); the heavy ones run
  it on the thread pool where the build has threads (spec 11.5), `async_pbkdf2` among them, so that
  on Emscripten without pthreads `async_pbkdf2<Hmac>` refuses at compile time and a program names
  `backends::webcrypto` for the browser.
- **WebCrypto by name.** `backends::webcrypto` is used on Emscripten only when the type names it,
  as `async_pbkdf2<hmac_sha2_256, backends::webcrypto>`; it stays the outside oracle of every
  algorithm it has (spec 7.4).
- **Checked.** `doc/capabilities.json` holds no separate asynchronous default, and
  `capabilities_test` checks that `default_async_backend<X>` is `default_backend<X>` (ruling R76).
- **Why:** one default per function, the same everywhere (D33): an asynchronous call computes on
  the backend its synchronous function would, so a program's bytes and its code do not change
  with the target.
- **Alternatives:** WebCrypto as the asynchronous default on Emscripten (D23's first form: the
  same call would run another backend on one target alone).

### D36. Measured tables are built from release builds only

The owner decided on 2026-11-17 (decision 3) that any measured table is built only from release
builds. The throughput was already so (`bench/collect.py`, `variant=release optimization=speed`);
the allocation counts, `doc/measurements.allocation.json` and `doc/allocation.adoc`, now come from
`bench/Jamfile`'s `allocation_release` and `allocation_openssl_release`, the sources of
`test/allocation_test.cpp` and `test/allocation_openssl_test.cpp` at `variant=release`, collected
by `bench/allocation_collect.py`, which reads the variant from b2's build directory
(`bench/<name>.test/<toolset>/<variant>/`), records `"build": "variant=release"` and refuses any
other output before it writes, its `--check` and `--render-only` included; the lanes' allocation
tests keep their variant and are never collected. Regenerated on 2026-11-17 on native, wasip2,
wasip3 and Emscripten, every count was identical to the earlier one. The dated measurements of
D33 stay as they are.

- **Why:** a debug build's counts and times measure the build, not the library.

### D37. Boost.Hash2's SHAKE is a whole extendable output, and SHAKE's default

Boost.Hash2 1.92's `shake_128::result()` and `shake_256::result()` permute and return the next
rate block on every call (`keccak_base::result` in `sha3.hpp`), so that repeated results continue
SHAKE's stream: Hash2's SHAKE is a whole extendable output, not one result of 32 or 64 bytes, as
the pre-flight ruling R14 had read it. The BearSSL task found it (Task 39, agreeing with BearSSL to
700 bytes), and the controller's ruling R89 overturned R14: `backends::hash2`'s SHAKE cells are
whole (any output length, the stream continued block after block), and by D34 the default of
`digests::shake_128` and `shake_256` is `backends::hash2`, not `backends::xkcp`. It is tested
against FIPS 202's examples (OpenSSL's `evpmd_sha.txt`, read at run time) and by agreement past one
block.

### D38. The families as built

What building the families (spec 2.15) decided, beside the plan:

- **`digest_of`.** The one-shot hash is `digest_of<Digest, Backend>(output, input, options)`, since
  `digest<N>` is Boost.Hash2's result type, which `algorithms.hpp` re-exports (ruling R10).
- **The guards in one place.** `detail::digest_traits` and `detail::mac_traits` hold each tag's
  guards; the output's size comes first (`invalid_output_size`), then the options', then the
  engine's narrower limits and options (`unsupported_by_backend`, ruling R41), so that a guard's
  error is the same on every backend. An engine's `min_output` exists because BLAKE2b's and
  BLAKE2s's output size is a parameter of the hash, which a partial cell cannot cut; a partial
  cell refuses every option it cannot serve, so that none is silently ignored (ruling R86).
- **BLAKE2b's output size is an option** (`output_size`, 64 by default): a `hasher` must know it at
  `make` (ruling R88); its key, salt and personalisation take RFC 7693's sizes (ruling R21).
- **SP 800-185's L at `finish`.** The fixed-length forms of cSHAKE, KMAC, TupleHash and
  ParallelHash encode the output's length after the input; XKCP takes it at `_Initialize`, so the
  adapter's `_final_length` writes it at `finish`, and there is no output-size option (ruling
  R91). KangarooTwelve's customisation is read at `finish` from the span `make` took (ruling R33).
- **Floors from the standards.** An HMAC tag is 10 bytes or more, the digest cut (RFC 2104 section
  5, ruling R23); a KMAC tag 4 bytes or more (SP 800-185 section 8.4.2, ruling R91); TurboSHAKE's
  domain byte 0x01 to 0x7F, which neither libsodium nor XKCP checks (RFC 9861, ruling R25).
- **The errors.** Value 9, `invalid_state`, is a hasher used after its `finish`, once moved from,
  or after a backend failure spent it; the messages of values 1, 2 and 5 are reworded for what
  the families widened ("the output is not of a size the function produces", "the key is not of a
  size the function accepts", "a parameter is outside the function's limits"), the values unchanged
  (ruling R8).
- **HMAC over a tag.** `macs::hmac<Digest>` is over a tag of `digests`, and
  `detail::digest_tag_of<Hmac>` maps each Boost.Hash2 hmac alias to it (ruling R15).
- **Argon2's units.** `argon2_parameters::memory_kib` counts KiB as RFC 9106's m does;
  `argon2id_parameters::memory` keeps bytes; each Doc Comment names its unit (ruling R16).

### D39. `backends::webcrypto`: declared everywhere, asynchronous only, its executor and limits

- **Declared by increment 7, on every target.** `struct webcrypto;` is in `config.hpp` on every
  target, declared by the asynchronous family's task, not the families' (ruling R83), so that a
  synchronous function that names it refuses natively too and its message test runs natively; an
  asynchronous function that names it off Emscripten refuses with "webcpp::hash:
  backends::webcrypto exists only on Emscripten: name a synchronous backend, or leave the
  default." (ruling R77).
- **Its executor (ruling R73).** WebCrypto's promise settles on the JavaScript event loop, so the
  executor must belong to an `io_context` run on the calling thread: a concrete executor of
  another context fails to compile, a type-erased one is checked at run time (strands unwrapped;
  `invalid_parameters` otherwise), and the calling thread, which cannot be checked, is the Doc
  Comment's.
- **Its limits (ruling R74).** An empty HMAC key is imported as one zero byte, which HMAC pads to
  the same key and WebCrypto takes; `detail::backend_limits` gains `pbkdf2_iterations`, and
  WebCrypto's are 2^32 - 1 iterations, an output of (2^32 - 1) / 8 bytes and an HKDF info of
  1,024 bytes (node's), documented as WebCrypto's limits as D26 documents OpenSSL's.
- **In the capability table (ruling R90).** `doc/capabilities.json` lists `backends::webcrypto`
  as a backend with `asynchronous_only`, its cells (the digests SHA-1, SHA-256, SHA-384 and
  SHA-512, HMAC over them, HKDF, PBKDF2) the asynchronous functions', never a default, rendered by
  `tools/hash/doc_tables.py` with the note "asynchronous family only".

### D40. The heavy functions: every native build has threads, and the handler holds the storage

- **Natively always.** `WEBCPP_HASH_HAS_THREADS` follows Boost.Config's `BOOST_HAS_THREADS`, which
  every native build defines, b2's default `threading=single` included (the pre-flight finding
  C-B1): natively the heavy functions always exist, and only `WEBCPP_HASH_NO_THREADS` takes them
  away. The guard tests the target first (`WEBCPP_TARGET_NATIVE`, `WEBCPP_TARGET_EMSCRIPTEN`),
  since wasip3 defines `BOOST_HAS_THREADS` for its cooperative threads (ruling R70). The refusal
  reads "... needs a build with threads (natively, remove WEBCPP_HASH_NO_THREADS; on Emscripten,
  link with -pthread). Without threads, call <name>." in place of D24's "natively b2
  threading=multi".
- **The functions.** `async_argon2id`, `async_argon2<Variant, Backend>` (ruling R75),
  `async_scrypt`, and `async_pbkdf2` with a synchronous backend (ruling R72), each defaulting to
  `default_backend` of its tag (the pre-flight finding C-m8).
- **The storage is the handler's.** The operation's state and its completion are allocated with
  the handler's associated allocator and given back before the handler runs, a cancellation's
  completion included; a completion destroyed unrun gives the state back too; webcpp's own code
  calls neither `operator new` nor the C allocator (the review of Task 46). A cancellation before
  the work starts completes at once with `operation_aborted` (the review of Task 44); after it
  starts, the work runs to its end and the output is zeroed.
- **Without exceptions.** The `_no_exceptions` twins check an error completion with a callback or
  `as_tuple`, and `use_future` on success paths alone (ruling R71).

### D41. No construction without a standard

The library exposes primitives a standard defines, never a protocol's construction (ruling R4:
BIP, EIP, NIP or SLIP constructions appear only in tests and in documentation examples marked as
what a domain library would build).

- **No HMAC over MD5-SHA1.** OpenSSL computes one, but no standard defines it and no vector checks
  it: no backend has `macs::hmac<digests::md5_sha1>` (ruling R92).
- **The standards' hashes.** `sskdf` takes SP 800-56C's approved hashes (SHA-1, SHA-2, SHA-3);
  `x963kdf` takes the hash functions SEC 1 v2 section 3.5 lists and NIST SP 800-135r1 section 5.1
  approves for it (SHA-1, SHA-224, SHA-256, SHA-384, SHA-512: not SHA-512/t, not SHA-3); another
  refuses at compile time. `kbkdf` takes every hmac its backend has, MD5's, RIPEMD-160's and
  BLAKE2's included, since protocols outside NIST's approval use them; its Doc Comment says to
  take SHA-2 or SHA-3 where approval matters.
- **What a library has and hash does not expose** is listed with its reason in
  `doc/capabilities.json` (`omitted[]`, layout version 5): OpenSSL's SHA-256/192, its KMAC hashes,
  `NULL`, CMAC, GMAC and Poly1305, and Whirlpool, MD4 and MDC2 in its legacy provider, which
  webcpp never loads since loading a provider changes the process's default library context
  (ruling R42).

### D42. Increment 6 waits for Botan

On 2026-11-17 Botan is not provisioned on webcpp's `main`: there is no `third_party/libs/botan` and
no crypto target, since webcpp::crypto's increment 5 (Botan), built by its foundation change's
increment 7, is not built (ruling R60). Increment 6 (plan Tasks 41 to 43) is not started, and
`backends::botan` is not declared; the tags only it would serve (`digests::streebog_256`,
`streebog_512`, `whirlpool`, `skein_512`, `kdfs::bcrypt`, `bcrypt_pbkdf`) are declared and refuse
with "... is in no backend yet.". The pre-flight scan's findings stand for when it runs:

- **One module list a program (C-M8 a).** One copy of Botan per program (spec 5.5's rule), so
  hash's modules (`sha3 shake keccak blake2 blake2s skein sm3 streebog whirlpool rmd160 kmac
  siphash blake2mac hmac hkdf sp800_108 sp800_56a sp800_56c kdf2 pbkdf2 scrypt argon2 bcrypt
  bcrypt_pbkdf cshake_xof`) must be in webcpp::crypto's list: a request to crypto's owner before
  its foundation's increment 7, step 2.
- **Botan's targets (C-M8 b).** They follow crypto's foundation design D15 (native and Emscripten,
  wasip2 and wasip3 as its spike finds), not spec 3.1's "native" and Tasks 42 and 43's "Run
  natively".
- **Exceptions on (C-M8 c).** Botan's tests are declared `<exception-handling>on` (crypto's D19),
  never through `hash-run`.
- **The column overstates (C-m7).** At Botan 3.14.1: no SipHash-2-4x (an 8-byte output), BLAKE2b
  without salt or personalisation, no TupleHash or ParallelHash; X9.63 is Botan's `kdf2` and the
  one-step SSKDF its `sp800_56a`. Task 41 corrects spec 3.5's cells by a change of this proposal.
- **Botan's heavy variants (C-M9).** `async_bcrypt`, `async_bcrypt_pbkdf`, and `async_argon2` and
  `async_scrypt` on `backends::botan` belong to Task 43, whose Step 2 adds them with their message
  tests.

## Risks / Trade-offs

- [libsodium's `sodium_init` on Emscripten stirs its random generator through JavaScript that
  looks for `crypto.getRandomValues` or node's `crypto` module] -> hash never draws random bytes,
  but `sodium_init` stirs at its first call; the libsodium task runs the C suite under node first,
  and a failure there is investigated as a build difference before anything else.
- [The 1 GiB scrypt vector on wasm32] -> its own program; the plan measures on which WebAssembly
  targets it runs, and the page states the memory a WebAssembly program needs.
- [OpenSSL `no-asm` on MSVC, wasip2 and wasip3] -> OpenSSL is never a default (D33); the
  throughput table shows the difference, and a program that wants OpenSSL's speed natively names
  `backends::openssl` in the type.
- [hash2 is slower than OpenSSL for PBKDF2 natively (OpenSSL 1.7 to 3.6 times faster in the
  dated measurement)] -> accepted by the owner for a default that is the same on every target
  (D33); the guide's comparison table and `doc/performance.adoc` show it, and the page says how to
  name `backends::openssl`.
- [The dated measurements age as the pins move] -> they decide nothing (D33) and are labelled
  with their date.
- [The scope of increments 2 to 6 is wide] -> each increment is usable alone and merged before the
  next; a cell of the capability table is checked against the library at its pin by the
  increment's first task, never assumed.
- [Botan's provisioning belongs to webcpp::crypto] -> increment 6 waits for it (D42), with the
  pre-flight's findings kept for when it runs; nothing earlier depends on Botan.
- [OpenSSL 3.5.9's Argon2 leaves some of its allocations unchecked and returns success over a wrong
  tag when one fails] -> every OpenSSL call that raised an error is a failure whatever its
  functions returned, and an allocation's error is `errc::not_enough_memory` (spec 3.4); the
  out-of-memory test requires it exactly.
- [A default whose engine is not built: `macs::blake2b_mac` on `backends::sodium` (D34)] -> a call
  that names no backend refuses at compile time, naming `backends::openssl`; the engine, or a change
  of the default, is a later change of this proposal.
- [The asynchronous family adds webcpp::asio as a dependency] -> only through `async.hpp`, which
  nothing else includes.
- [XKCP has no tagged release] -> the pin is a master commit, `d20c8fc` (2026-11-10); moving it is
  a change of its own with the vectors and XKCP's SnP suite on every target.
- [libsodium's internal `argon2_ctx` is not a public interface] -> used by one test alone; a pin
  move that changes it breaks only that test, which then reads the RFC vector some other way.
- [The departures of D2 and D7 are webcpp's code] -> each has its oracle (Hash2's `keccak_base`,
  the other backends and WebCrypto) and its RFC vectors on every target.
- [BearSSL comes from webcpp::crypto's plan] -> the BearSSL task waits until
  `/third_party//bearssl` is on the superproject's `main`; nothing else does.

## Migration Plan

Nothing depends on webcpp::hash yet: it adds a library and three submodules. webcpp::crypto's
later change moves its HKDF need onto it (spec 9.2) and its libsodium backend onto
`/third_party//sodium`. To start, follow `reference/STATUS.md` ("How to resume"). Rollback: the
change is a proposal; withdrawing it deletes `openspec/changes/genesis/`.

D33 changes increment 1's built code before anything is published: the profiles, the overrides,
`detail/ranking.hpp`, `detail/overrides.hpp`, their tests and messages and the lane `measure` are
removed in the library's repository and the superproject's integration branch, and
`default_backend` answers `backends::hash2` (`backends::sodium` for `argon2id` and `scrypt`). A
program that named a backend in the type is unchanged; one that gave a former profile or override
define still compiles, the define having no effect, and runs on `backends::hash2`; one that relied
on a define to reach BearSSL or OpenSSL names that backend in the type instead.

D34 to D41 change little of increment 1's code, before anything is published: the messages of
error values 1, 2 and 5 are reworded (the values unchanged); `argon2id` and `scrypt` gain OpenSSL's
engines, and their refusal on another backend names both libraries; libsodium's "needs" text names
`blake2b_subkey` too; the allocation table is regenerated from release builds, its counts
unchanged. A program written against increment 1 compiles and computes the same bytes.

## Open Questions

None is open. O3 (the default of the families for an algorithm Boost.Hash2 lacks) was answered by
the owner on 2026-11-17 as it proposed: D34. O1 (a browser backend over WebCrypto) was answered by
the owner on 2026-11-14: D23. O2 (one native throughput order, or one per platform) was closed by
the owner's decision of 2026-11-16, D33: no measured order decides a default any more. The owner's
gates (creating `webcpporg/hash`, adding `libs/hash`, every push and
merge, and opening the Boost.Hash2 contribution) are in the plan's "Owner gates".
