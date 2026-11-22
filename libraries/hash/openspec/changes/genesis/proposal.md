## Why

Programs above webcpp::crypto need hashing and key derivation that Boost.Hash2 does not
provide: Keccak-256 for Ethereum addresses and signatures, HKDF for every key hierarchy and
every protocol that turns a shared secret into keys, PBKDF2-HMAC-SHA-512 for BIP39 seeds,
Argon2id and scrypt for passwords, and BLAKE3 where a format asks for it. The research behind
webcpp::crypto's roadmap (the noble and scure libraries) found each of these on the first page
of their needs. Boost.Hash2 1.92 has
SHA-1/2/3, SHAKE, RIPEMD, BLAKE2 and HMAC, and none of them; webcpp::crypto's specification said
only that HKDF "is written on Hash2's hmac when needed".

The owner proposed a library of its own on 2026-11-14: one entry point for hashing and key
derivation, on every target webcpp has (native, wasip2, wasip3, Emscripten), whose primitives
are Boost.Hash2's and proven C libraries', whose key derivations have backends as webcpp::crypto's
algorithms do, and whose default backend was first to be the fastest natively and the smallest
on the web. He decided its shape the same day and accepted it, then widened it: every backend
exposes all its library offers for hashing and key derivation, in increments by value; the
browser's WebCrypto becomes a backend through a parallel asynchronous API. Increment 1 is built in
the library's own repository and green on every local lane; it is not yet published, and the spec
records what building it decided. Its measurements showed differences of nanoseconds natively and
a few KB on WebAssembly between the backends of HKDF, so on 2026-11-16 the owner replaced the
measured default, the profiles and the overrides by one default: `backends::hash2` for every key
derivation on every target (design D33). On 2026-11-17, before increment 2, he gave the families
the same kind of rule (the first of hash2, XKCP, BLAKE3 and libsodium that offers an algorithm,
design D34), made the asynchronous family's default the synchronous one (D35), and asked for
measured tables from release builds only (D36). Increments 2 to 5 and 7 are built on local
integration branches, and the spec records what building them decided (D37 to D41); increment 6
provisions Botan itself and is built on the stream `botan-6` (D43 to D45, superseding D42's
waiting).

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
  function templates `template <class Hmac, class Backend = default_kdf_backend<Hmac, Kdf>>`,
  the default named per operation through the public tags `kdfs::hkdf`, `kdfs::hkdf_extract`,
  `kdfs::hkdf_expand` and `kdfs::pbkdf2`, over
  backends named after the library that does the work: `backends::hash2` (webcpp's construction
  over any Hash2 hmac, every target, the recorded departure from "webcpp writes no
  cryptography"), `backends::sodium` (libsodium's HKDF over SHA-256 and SHA-512),
  `backends::bearssl` (BearSSL's HKDF), `backends::openssl` (both; native, wasip2, wasip3), and
  later `backends::botan`. A backend without the function refuses at compile time, naming those
  that have it; a backend whose C interface holds a length in an `int` refuses past `INT_MAX`
  (OpenSSL, `detail::backend_limits`). Every function that returns a result is `[[nodiscard]]`.
- **The default backend** (the owner's decision of 2026-11-16, design D33): `backends::hash2`
  for `hkdf`, `hkdf_extract`, `hkdf_expand` and `pbkdf2`, over every hmac, on every target and
  whatever the program links; `backends::sodium` for `argon2id` and `scrypt`, its only backend.
  BearSSL, OpenSSL and libsodium's HKDF stay, tested as before, reached only by naming them in the
  type (`hkdf<hmac_sha2_256, backends::openssl>`). No measurement and no define decides a default:
  the measured default of increment 1, its profiles, its overrides, `detail/ranking.hpp` and the
  lane `measure` are retired. The families' tags (the owner's decision of 2026-11-17, design D34)
  default to the first of `backends::hash2`, `backends::xkcp`, `backends::blake3` and
  `backends::sodium` that offers the tag (SHAKE on hash2, Keccak and the SP 800-185 functions on
  XKCP, BLAKE3 on BLAKE3, BLAKE2b with its options and Argon2i on libsodium), else none, and the
  call names its backend; BearSSL, OpenSSL, Botan and WebCrypto are never defaults.
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
  `tools/hash/doc_tables.py` (which renders `doc/backends.adoc` and the dated
  `doc/measurements.adoc`; `tools/hash/measure.py` and the lane `measure` are retired by design
  D33), `tools/test/hash_jamfiles_test.py` with its fixtures, chapter 10's
  three rows and chapter 17's entry; libsodium links with `-sALLOW_MEMORY_GROWTH=1` on Emscripten.
  BLAKE3's own test program runs in the library's own lane `c-suites`, beside libsodium's and XKCP's
  suites.
- **Tests:** RFC 5869, 6070, 7914 and 9106 vectors, Keccak-256's and BLAKE3's official vectors,
  cross-backend agreement natively (all backends, random inputs) and on WebAssembly (hash2 against
  sodium and bearssl), WebCrypto as an outside oracle on Emscripten under node, the C libraries'
  own suites, a test per message, a counting allocation test, every lane (native, wasip2,
  wasip3, emscripten, gcc-15; MSVC and Apple clang in the CI at the end), exceptions on and off.
- **Measurements:** the measured table (program size per target, throughput natively), kept as
  a reference dated 2026-11-16 that decides nothing, and PBKDF2-HMAC-SHA-512 at 1,000,000
  iterations for hash2, OpenSSL and libsodium; every measured table, the allocation counts
  included, built only from release builds (the owner's decision of 2026-11-17, design D36).
- **Seven increments, the highest value first** (spec 1.5): 1, the above; 2, libsodium's whole
  hashing surface (BLAKE2b with key, salt, personalisation and length; SipHash-2-4 and -2-4x;
  `crypto_kdf` subkeys; Argon2i; SHAKE and TurboSHAKE; its SHA-2, SHA-3 and HMACs); 3, XKCP's
  Keccak family (Keccak-224/384/512, cSHAKE, KMAC, TupleHash, ParallelHash, KangarooTwelve,
  TurboSHAKE); 4, OpenSSL's (every EVP digest including KECCAK-256, KMAC, BLAKE2 MACs, scrypt,
  Argon2d/i/id with every RFC 9106 parameter, KBKDF, SSKDF, X9.63); 5, BearSSL's hashes and HMAC;
  6, Botan 3's (PBKDF2, Argon2, bcrypt, BLAKE2b, Keccak, cSHAKE, KMAC, Skein, SM3, Streebog,
  Whirlpool), Botan provisioned by the increment itself, built (designs D43 to D45); 7, the browser
  backend. A capability table
  says which backend provides each function, and every refusal names the backends that have it.
  From increment 2, `digest_of`, `hasher`, `mac`, `argon2` and the KDF functions (`blake2b_subkey`,
  `kbkdf`, `sskdf`, `x963kdf`) reach every backend; Hash2's form stays for Hash2's types,
  `keccak_256` and `blake3`.
- **No profile and no override** (retired on 2026-11-16, design D33): increment 1 built one
  profile per program (`WEBCPP_HASH_PROFILE_*` and webcpp's umbrella `WEBCPP_PROFILE_*`) and
  per-algorithm overrides `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`; with the backend named
  in the type, a define that changed what `hkdf<Hmac>` means across a program would make BearSSL
  or OpenSSL a default again and hide the backend from the call's reader, so a former define has
  no effect.
- **The browser backend through a parallel asynchronous API** (the owner's answer to O1):
  `async_digest`, `async_hmac`, `async_hkdf` and `async_pbkdf2` take a webcpp::asio completion
  token (callback, coroutine in Asio or Cobalt, future); their default is the synchronous
  function's on every target (the owner's decision of 2026-11-17, design D35), the cheap ones
  completing through a post and the heavy ones (`async_argon2id`, `async_argon2`, `async_scrypt`,
  `async_pbkdf2`) on a thread pool where the build has threads; `backends::webcrypto` is reached on
  Emscripten by naming it; the synchronous functions never use WebCrypto. WebCrypto remains the
  outside oracle of every algorithm it has.
- **Documentation** in the manner of noble's README (one section per algorithm: what it is, an
  example, its pitfalls) and libsodium's documentation (ISC, adapted with attribution in
  `NOTICE.md`), with teaching chapters by use case: peer-to-peer networks, cryptocurrencies and
  privacy.

## Capabilities

### New Capabilities

- `hash`: webcpp::hash, Boost.Hash2's algorithms and HMAC by alias, Keccak-256 and BLAKE3 in
  Hash2's form, HKDF and PBKDF2 over backends with `backends::hash2` as the default, Argon2id and
  scrypt, every backend's further hashing and KDF functions in increments, the
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
- **Implementation:** increment 1 (the plan's Tasks 0 to 24) is built: the library's repository
  (branch `main`, not yet on GitHub) and the superproject's integration branch hold it, green on
  native, wasip2, wasip3, Emscripten and gcc-15 locally; MSVC and Apple clang wait for the CI after
  the owner's gates. Increments 2, 3, 4, 5 and 7 (Tasks 25 to 40 and 44 to 47) are built, each task
  reviewed, on the library's branch `increments-2-7` and the superproject's `hash/increments-2-7`
  (local), increment 4 merging into them when this was written; increment 6 (Tasks 40b to 43 and
  47) is built on the branches `hash/botan-6`, Botan provisioned by its first task (design D43).
  **The reference code is the code written in
  the tasks of `reference/plan.md`:** Task 1 (`build.jam`, `config.hpp`, the test Jamfile's
  `hash-run`), Task 2 (`error.hpp`), Task 3 (`algorithms.hpp`, `hmac.hpp`), Task 4 (the XKCP target,
  `xkcp_adapter.c`, `canary.h`, `canary_xkcp.c`), Task 5 (`keccak.hpp`), Tasks 6 and 7 (the BLAKE3
  target, `blake3.hpp`), Task 8 (the libsodium target), Task 9 (`kdf.hpp`, `hkdf.hpp`,
  `detail/engines.hpp`, `backends/hash2.hpp`), Task 10 (`pbkdf2.hpp`), Tasks 11 to 13
  (`backends/sodium.hpp`, `backends/openssl.hpp`, `backends/bearssl.hpp`), Task 15 (`argon2.hpp`),
  Task 16 (`scrypt.hpp`) and Task 21 (`tools/hash/measure.py`, deleted by design D33), and the tasks
  of increments 2 to 7 (Tasks 25 to 47), with the tests and Jamfiles the other tasks give. What
  Tasks 14 and 21 built for the profiles, the overrides, the ranking and the lane `measure` is
  retired by design D33 (task 1.26). `reference/spec.md` is the full specification and
  `reference/STATUS.md` says where things stand and how to start.
- **Dependencies:** Boost 1.92 (Hash2, System, Core's `lightweight_test`, JSON in tests); libsodium
  1.0.22 at `77e1ce5d6dee871c49ef211222ba18ef0c486bda` (ISC); XKCP at
  `d20c8fcaecc543f816a0721d0dfc13b4cf27c14d` (CC0); BLAKE3 1.8.7 at
  `f3149ec5bb5449af877ba20377a11008ff499fa2` (CC0-1.0 or Apache-2.0); BLAKE2's reference repository
  at `ed1974ea83433eba7b2d95c5dcd9ac33cb847913` (CC0-1.0), for its test vectors (increment 2);
  BearSSL at `7bea48e` and OpenSSL 3.5.9 at `45e844f`, as the superproject builds them; for tests,
  wasmtime, node's WebCrypto, the RFCs' and NIST's vectors, and the C libraries' own test files read
  at run time (OpenSSL's EVP test data, libsodium's `.exp` files, XKCP's test vectors). From
  increment 7, webcpp::asio (`async.hpp` alone). The superproject's `tools/webcpp.jam`,
  `third_party/Jamfile`, `webcpp.same-program` and asio's test support
  (`webcpp::test::exit_status`).
- **Status:** accepted on 2026-11-14 (the owner decided the shape and approved it); increments 1
  to 7 built locally, not published, no release. It becomes `implementing` at the owner's gates.
- **Other proposals:** `libraries/crypto` is changed later: it depends on webcpp::hash for HKDF
  (replacing its spec 1.2's "written on Hash2's hmac when needed"), its libsodium backend
  (version 0.2) uses `/third_party//sodium` as built here, and `/third_party//bearssl`, which its
  plan adds, gains webcpp::hash's canary here (the BearSSL task waits for that target on `main`).
  webcpp::hash reads no profile, webcpp's umbrella `WEBCPP_PROFILE_*` included (design D33);
  whether webcpp::crypto takes a profile for its own defaults is that library's own later change.
  Increment 6 provisions Botan for both libraries (`/third_party//botan`, one module list with
  webcpp::crypto's needs, design D43); crypto's increment 7 reuses it and adds its own canary.
  `libraries/asio` supplies the completion-token model and `browser::async_await` the
  asynchronous family uses. `organization` carries the rulebook this plan extends (chapter 10's
  rows, chapter 17's entry).
