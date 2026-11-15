## Why

Programs above webcpp::crypto need hashing and key derivation that Boost.Hash2 does not
provide: Keccak-256 for Ethereum addresses and signatures, HKDF for every key hierarchy and
every protocol that turns a shared secret into keys, PBKDF2-HMAC-SHA-512 for BIP39 seeds,
Argon2id and scrypt for passwords, and BLAKE3 where a format asks for it. The research behind
webcpp::crypto's roadmap (the noble and scure stacks, and the TypeScript wallet and identity
stacks built on them) found each of these on the first page of their needs. Boost.Hash2 1.92 has
SHA-1/2/3, SHAKE, RIPEMD, BLAKE2 and HMAC, and none of them; webcpp::crypto's specification said
only that HKDF "is written on Hash2's hmac when needed".

The owner proposed a library of its own on 2026-11-14: one entry point for hashing and key
derivation, on every target webcpp has (native, wasip2, wasip3, Emscripten), whose primitives
are Boost.Hash2's and proven C libraries', whose key derivations have backends as webcpp::crypto's
algorithms do, and whose default backend is the fastest natively and the smallest on the web. He
decided its shape the same day and accepted it, then widened it: every backend exposes all its
library offers for hashing and key derivation, in increments by value; one profile in the
configuration changes every default at once; the browser's WebCrypto becomes a backend through a
parallel asynchronous API. Nothing is built.

## What Changes

- **New library `webcpp::hash`**, a wrapper, header-only C++20, `/webcpp/hash//hash`,
  repository `webcpporg/hash` (the owner creates it), submodule `libs/hash`.
- **Boost.Hash2 1.92 re-exported by alias:** every algorithm Hash2 has (`sha2_256`,
  `sha3_256`, `blake2b_512`, `ripemd_160`, ... and the non-cryptographic ones), `hmac<H>` and its
  sixteen aliases, `digest`, `hash_append` and its family. No wrapper: they are the Hash2 types.
- **`keccak_256`** (Ethereum's Keccak-256, padding `0x01`) in Hash2's HashAlgorithm form, over
  XKCP's Keccak-p[1600] permutation (CC0), with Hash2's own sponge so that its results are
  `detail::keccak_base<0x01, 512, 256>`'s; a public `keccak_256` contributed to Boost.Hash2 is the
  long-term plan, after which ours becomes an alias.
- **`blake3`** in Hash2's HashAlgorithm form over the official BLAKE3 C implementation 1.8.7,
  with keyed hashing, key derivation and the extendable output; portable on WebAssembly, SIMD
  dispatch natively.
- **HKDF (RFC 5869: `hkdf_extract`, `hkdf_expand`, `hkdf`) and PBKDF2 (RFC 8018: `pbkdf2`)** as
  function templates `template <class Hmac, class Backend = default_kdf_backend<Hmac>>`, over
  backends named after the library that does the work: `backends::hash2` (webcpp's construction
  over any Hash2 hmac, every target, the recorded departure from "webcpp writes no
  cryptography"), `backends::sodium` (libsodium's HKDF over SHA-256 and SHA-512),
  `backends::bearssl` (BearSSL's HKDF), `backends::openssl` (both; native, wasip2, wasip3), and
  later `backends::botan`. A backend without the function refuses at compile time, naming those
  that have it.
- **The default backend:** natively the fastest linked (OpenSSL when linked, else hash2); on
  WebAssembly the one giving the smallest binary, from a measured size table (one KDF program per
  backend per target), regenerated at every pin move and checked by an own lane.
- **`argon2id` and `scrypt`** from libsodium (`crypto_pwhash_argon2id`,
  `crypto_pwhash_scryptsalsa208sha256_ll`), with libsodium's allocation of their memory recorded
  as a departure; the Argon2 reference implementation's allocation callbacks stay the
  alternative, a later backend if a user's `memory_resource` must hold the memory.
- **Three new third-party submodules,** built file by file by `third_party/Jamfile` with
  webcpp::hash's canary and three layers: `third_party/libs/libsodium` (1.0.22,
  `/third_party//sodium`, later shared by webcpp::crypto's libsodium backend, one copy per
  program), `third_party/libs/xkcp` (`/third_party//xkcp`) and `third_party/libs/blake3`
  (`/third_party//blake3`). `/third_party//bearssl` gains webcpp::hash's canary.
- **The superproject gains** `/webcpp/hash//openssl` (the one OpenSSL provider),
  `tools/hash/measure.py` and the own lane `measure`, `tools/test/hash_jamfiles_test.py` with its
  fixtures, `tools/test/blake3_c_suite.py`, chapter 10's three rows and chapter 17's entry.
- **Tests:** RFC 5869, 6070, 7914 and 9106 vectors, Keccak-256's and BLAKE3's official vectors,
  cross-backend agreement natively (all backends, random inputs) and on WebAssembly (hash2 against
  sodium and bearssl), WebCrypto as an outside oracle on Emscripten under node, the C libraries'
  own suites, a test per message, a counting allocation test, every lane (native, wasip2,
  wasip3, emscripten, gcc-15; MSVC and Apple clang in the CI at the end), exceptions on and off.
- **Measurements:** the measured table (program size per target, throughput natively), and
  PBKDF2-HMAC-SHA-512 at 1,000,000 iterations for hash2, OpenSSL and libsodium.
- **Seven increments, the highest value first** (spec 1.5): 1, the above; 2, libsodium's whole
  hashing surface (BLAKE2b with key, salt, personalisation and length; SipHash-2-4 and -2-4x;
  `crypto_kdf` subkeys; Argon2i; SHAKE and TurboSHAKE; its SHA-2, SHA-3 and HMACs); 3, XKCP's
  Keccak family (Keccak-224/384/512, cSHAKE, KMAC, TupleHash, ParallelHash, KangarooTwelve,
  TurboSHAKE); 4, OpenSSL's (every EVP digest including KECCAK-256, KMAC, BLAKE2 MACs, scrypt,
  Argon2d/i/id with every RFC 9106 parameter, KBKDF, SSKDF, X9.63); 5, BearSSL's hashes and HMAC;
  6, Botan 3's (PBKDF2, Argon2, bcrypt, BLAKE2b, Keccak, cSHAKE, KMAC, Skein, SM3, Streebog,
  Whirlpool), after webcpp::crypto provisions Botan; 7, the browser backend. A capability table
  says which backend provides each function, and every refusal names the backends that have it.
  From increment 2, `digest`, `hasher`, `mac` and the KDF functions reach every backend; Hash2's
  form stays for Hash2's types, `keccak_256` and `blake3`.
- **One profile changes every default backend:** none (`auto`: fastest natively, smallest on
  WebAssembly), `WEBCPP_HASH_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE` or `_BACKEND_<NAME>`,
  plus per-algorithm overrides `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`; the defaults come from
  the build and a committed table of webcpp's own measurements, regenerated at every pin move.
- **The browser backend through a parallel asynchronous API** (the owner's answer to O1):
  `async_digest`, `async_hmac`, `async_hkdf` and `async_pbkdf2` take a webcpp::asio completion
  token (callback, coroutine in Asio or Cobalt, future); on Emscripten they default to
  `backends::webcrypto`; elsewhere they run the synchronous backend and complete through a post;
  the synchronous functions never use WebCrypto. WebCrypto remains the outside oracle of every
  algorithm it has.
- **Documentation** in the manner of noble's README (one section per algorithm: what it is, an
  example, its pitfalls) and libsodium's documentation (ISC, adapted with attribution in
  `NOTICE.md`), with teaching chapters by use case: peer-to-peer networks, cryptocurrencies and
  privacy.

## Capabilities

### New Capabilities

- `hash`: webcpp::hash, Boost.Hash2's algorithms and HMAC by alias, Keccak-256 and BLAKE3 in
  Hash2's form, HKDF and PBKDF2 over backends with a measured default, Argon2id and scrypt, every
  backend's further hashing and KDF functions in increments, the configuration profile, the
  browser backend's asynchronous family, with their interface, error values, backends, build,
  messages, allocation rules, tests, measurements and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a wrapper (`"port-of": null`): webcpp's interface over Boost.Hash2, XKCP, BLAKE3's C
  implementation, libsodium, BearSSL and OpenSSL, none of them ported. webcpp's own code is the
  two constructions the owner recorded (HKDF and PBKDF2 over Hash2's hmac, Keccak-256's sponge
  over XKCP's permutation), the guards, the adapters and the canaries. Repository
  `webcpporg/hash` (not created; the owner creates it, plan gate G1), checked out as the
  submodule `libs/hash` (gate G2), BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** not started; the library's repository has no commit. **The reference code
  is the code written in the tasks of `reference/plan.md`:** Task 1 (`build.jam`, `config.hpp`,
  the test Jamfile's `hash-run`), Task 2 (`error.hpp`), Task 3 (`algorithms.hpp`, `hmac.hpp`),
  Task 4 (the XKCP target, `xkcp_adapter.c`, `canary.h`, `canary_xkcp.c`), Task 5
  (`keccak.hpp`), Tasks 6 and 7 (the BLAKE3 target, `blake3.hpp`), Task 8 (the libsodium target),
  Task 9 (`kdf.hpp`, `hkdf.hpp`, `detail/engines.hpp`, `backends/hash2.hpp`), Task 10
  (`pbkdf2.hpp`), Tasks 11 to 13 (`backends/sodium.hpp`, `backends/openssl.hpp`,
  `backends/bearssl.hpp`), Task 15 (`argon2.hpp`), Task 16 (`scrypt.hpp`) and Task 21
  (`tools/hash/measure.py`), and the tasks of increments 2 to 7 (Tasks 25 to 47), with the tests
  and Jamfiles the other tasks give.
  `reference/spec.md` is the full specification and `reference/STATUS.md` says where things
  stand and how to start.
- **Dependencies:** Boost 1.92 (Hash2, System, Core's `lightweight_test`, JSON in tests);
  libsodium 1.0.22 at `77e1ce5d6dee871c49ef211222ba18ef0c486bda` (ISC); XKCP at
  `d20c8fcaecc543f816a0721d0dfc13b4cf27c14d` (CC0); BLAKE3 1.8.7 at
  `f3149ec5bb5449af877ba20377a11008ff499fa2` (CC0-1.0 or Apache-2.0); BearSSL at `7bea48e` and
  OpenSSL 3.5.9 at `45e844f`, as the superproject builds them; for tests, wasmtime, node's
  WebCrypto, and the RFCs' vectors. The superproject's `tools/webcpp.jam`, `third_party/Jamfile`,
  `webcpp.same-program` and asio's test support (`webcpp::test::exit_status`).
- **Status:** accepted on 2026-11-14 (the owner decided the shape and approved it); not built,
  no release.
- **Other proposals:** `libraries/crypto` is changed later: it depends on webcpp::hash for HKDF
  (replacing its spec 1.2's "written on Hash2's hmac when needed"), its libsodium backend
  (version 0.2) uses `/third_party//sodium` as built here, and `/third_party//bearssl`, which its
  plan adds, gains webcpp::hash's canary here (the BearSSL task waits for that target on `main`).
  A later change of webcpp::crypto takes the same profile idea for its own defaults, and
  webcpp::crypto's Botan provisioning (its increment 5) is what increment 6 waits for.
  `libraries/asio` supplies the completion-token model and `browser::async_await` the
  asynchronous family uses. `organization` carries the rulebook this plan extends (chapter 10's
  rows, chapter 17's entry).
