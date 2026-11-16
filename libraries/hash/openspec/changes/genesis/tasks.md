# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. Increment 1 is built and committed in the library's own
repository (branch `main`, not yet on GitHub) and the superproject's integration branch, reviewed
and green on every local lane; each of its tasks says what was built, with the design decision
where the build changed the plan; a note dated 2026-11-16 marks the work design D33 retires.
Increments 2 to 7 and the publication are open. The groups are
the spec's increments (spec 1.5); inside each, the plan's "The order of the work, and the
parallel streams" says which tasks run in parallel, by backend.
`reference/STATUS.md` says how to start.

## 1. Increment 1: Hash2, Keccak-256, BLAKE3, HKDF, PBKDF2, Argon2id, scrypt, the default backend

- [x] 1.1 The worktree, the ledger and the baseline (plan: Task 0). Built: the superproject's integration branch on webcpp::crypto's version 0.1, the ledger, and the baseline lanes and lint recorded
- [x] 1.2 The library's skeleton, the test rule `hash-run` and chapter 17's entry (plan: Task 1). Built: `build.jam`, `config.hpp`, the test Jamfile's `hash-run` (each test with its `_no_exceptions` twin), third_party's shared hash block and `detail/canary.h`, chapter 17's entry
- [x] 1.3 The errors, values 1 to 8 (9 comes with the families) (plan: Task 2; stream `errors`). Built: `error.hpp`, the category `webcpp.hash`, each message tested
- [x] 1.4 Boost.Hash2's algorithms and HMAC by alias (plan: Task 3; stream `hash2`). Built: `algorithms.hpp` and `hmac.hpp`, every alias checked with `std::is_same_v`
- [x] 1.5 XKCP's Keccak-p[1600], its adapter, canary and own tests (plan: Task 4; stream `keccak`). Built: `/third_party//xkcp` at `d20c8fc`, `src/xkcp_adapter.c`, its `config.h` and canary; XKCP's `UnitTests --SnP` in the lane `c-suites`
- [x] 1.6 `keccak_256` with Hash2's sponge, Hash2 as the oracle (plan: Task 5; stream `keccak`). Built: `keccak.hpp`, the published digests and `keccak_base<0x01, 512, 256>` as the oracle on every target
- [x] 1.7 BLAKE3's C implementation file by file, its canary and own suite (plan: Task 6; stream `blake3`). Built: `/third_party//blake3` at 1.8.7, portable on WebAssembly and dispatched natively; BLAKE3's `c/main.c` in the lane `c-suites` with a corrupted twin, replacing the plan's `tools/test/blake3_c_suite.py` (design D29)
- [x] 1.8 `blake3` in Hash2's form, with its vectors (plan: Task 7; stream `blake3`). Built: `blake3.hpp`, every case of `test_vectors.json` through `squeeze` and repeated `result()`
- [x] 1.9 libsodium file by file, its canary and its own tests (plan: Task 8; stream `sodium`). Built: `/third_party//sodium` at 1.0.22 with `configure`'s defines per target and `-sALLOW_MEMORY_GROWTH=1` on Emscripten (design D27); libsodium's 101 tests and one more in the lane `c-suites`
- [x] 1.10 The key derivations' layer, and HKDF on `backends::hash2` (plan: Task 9; stream `kdf`). Built: `kdf.hpp`, `hkdf.hpp`, `detail/engines.hpp`, `detail/wipe.hpp`, `backends/hash2.hpp`; RFC 5869's seven cases and the wipe record
- [x] 1.11 PBKDF2 on `backends::hash2` (plan: Task 10; stream `kdf`). Built: `pbkdf2.hpp`, RFC 6070's and RFC 7914's cases, the 16,777,216-iteration case in its own program
- [x] 1.12 HKDF on `backends::sodium` (plan: Task 11; stream `kdf-sodium`). Built: `backends/sodium.hpp` over `crypto_kdf_hkdf_sha256_*` and `_sha512_*`
- [x] 1.13 `/webcpp/hash//openssl` and `backends::openssl` (plan: Task 12; stream `kdf-openssl`). Built: the alias, `backends/openssl.hpp` (HKDF and PBKDF2 with `pkcs5 = 1`), its refusal on Emscripten, and `detail::backend_limits` with OpenSSL's `INT_MAX` (design D26)
- [x] 1.14 HKDF on `backends::bearssl`, after webcpp::crypto's BearSSL is on `main` (plan: Task 13; stream `kdf-bearssl`). Built: `backends/bearssl.hpp` and hash's canary in `/third_party//bearssl`, on webcpp::crypto's version 0.1 integration branch (its `main` waits for crypto's gates)
- [x] 1.15 The profile, the overrides, the default backend, the capability table (plan: Task 14; stream `default`). Built: the profiles and the umbrella in `config.hpp`, the public tags `kdfs::hkdf_extract`, `kdfs::hkdf_expand`, `kdfs::argon2id` and `kdfs::scrypt` with a default per operation (design D25), the generated `detail/overrides.hpp` included by `kdf.hpp` (design D28), `doc/capabilities.json` and `capabilities_test`. Retired on 2026-11-16 by design D33: the profiles, the umbrella's reading, the overrides with `detail/overrides.hpp` and `overrides_list_test`, the profiles' tests and messages and their `detect_mismatch`; the public tags, `default_backend`, `default_kdf_backend`, `doc/capabilities.json` and `capabilities_test` stay (task 1.26)
- [x] 1.16 Argon2id on libsodium, RFC 9106 through its core (plan: Task 15; stream `pwhash`). Built: `argon2.hpp`, RFC 9106's case through `argon2_ctx`, the presets and the guards
- [x] 1.17 scrypt on libsodium (plan: Task 16; stream `pwhash`). Built: `scrypt.hpp`, RFC 7914's cases, the 1 GiB case in its own program
- [x] 1.18 Agreement between backends on every target (plan: Task 17; stream `agreement`). Built: `agreement_hkdf_test` and `agreement_pbkdf2_test`, every linked backend that has the function against every other on each target, 1,000 seeded random cases per hmac, the empty inputs first
- [x] 1.19 WebCrypto under node as an outside oracle (plan: Task 18; stream `webcrypto`). Built: `webcrypto_oracle_test` on Emscripten, HKDF and PBKDF2 over SHA-1, SHA-256, SHA-384 and SHA-512
- [x] 1.20 The user's Jamfiles, the canaries at link time, one program (plan: Task 19; stream `build`). Built: `tools/test/hash_jamfiles_test.py` and its fixtures, the link-fail cases, `webcpp.same-program` per C library with libsodium's flag as its one difference on Emscripten (design D27)
- [x] 1.21 Allocation (plan: Task 20; stream `allocation`). Built: `allocation_test` (exact counts), OpenSSL's allocations counted apart, `doc/measurements.allocation.json`
- [x] 1.22 The measured table and `detail/ranking.hpp` (plan: Task 21; stream `measure`). Built: `tools/hash/measure.py`, the own lane `measure` (284 cells on four targets), `doc/measurements.json` and `.adoc`, rankings per operation with the native toolchain's identity; provisional, measured on one loaded machine, to be re-measured on an idle one before a release (design D32). Retired on 2026-11-16 by design D33: `detail/ranking.hpp` and the code reading it, the own lane `measure` (`tools/hash/measure.py --check`, the CI matrix entry "Own lane (hash, measure)", `libs/hash/test/measure`) and the re-measurement before a release; `doc/measurements.json` and `.adoc` stay as a reference dated 2026-11-16 that decides nothing (task 1.26)
- [x] 1.23 PBKDF2-HMAC-SHA-512 at 1,000,000 iterations (plan: Task 22; stream `throughput`). Built: `bench/pbkdf2_sha512.cpp` in the lane `measure`, `doc/performance.adoc` (natively OpenSSL about 2.2 times faster than hash2). On 2026-11-16 the lane `measure` is retired by design D33; the correctness program `pbkdf2_sha512_key` runs in every lane, the times are measured by hand with `bench/collect.py`, and the table stays
- [x] 1.24 The documentation, the examples and the Boost.Hash2 proposal's text (plan: Task 23). Built: the guide, generated by `tools/hash/doc_tables.py` with two comparison tables a chapter, each under eight columns (design D31); the examples on every target; `doc/upstream.adoc`
- [x] 1.25 Increment 1 whole: the rulebook, green, review, the gates (plan: Task 24). Built: chapter 5's exceptions and chapter 17 as built, the lint's canary pins over hash, `[[nodiscard]]` on every result under the lint's rule (design D30); native, wasip2, wasip3, Emscripten, gcc-15, `c-suites`, `measure` and the lint green from scratch; the whole review. The gates are 8.1 and 8.2. The lane `measure` is retired on 2026-11-16 by design D33
- [x] 1.26 Every key derivation defaults to `backends::hash2` (design D33, the owner's decision of 2026-11-16; plan: the notes of 2026-11-16 at Tasks 14 and 21). Built on the branches `hash/defaults-hash2` of `webcpporg/hash` and of the superproject: `default_backend` and `default_kdf_backend` answer `backends::hash2` for `hkdf`, `hkdf_extract`, `hkdf_expand` and `pbkdf2` on every target and link set, `backends::sodium` for `argon2id` and `scrypt`, whose engine's primary template refuses an unlinked libsodium with its "needs" text (messages `argon2id_default_not_linked` and `scrypt_default_not_linked`, their Emscripten fixtures and MSVC cases); `default_backend_former_defines` shows that former profile and override defines change nothing; the profiles, the umbrella's reading, the overrides, `detail/ranking.hpp`, `detail/overrides.hpp`, their `detect_mismatch`, tests and messages removed; the lane `measure`, `tools/hash/measure.py` whole with `hash_measure_test.py`, `tools/ci/matrix.py`'s `EVERY_TARGET_LANES` and `tools/lint/compile_commands.py`'s native WebAssembly commands removed with their tests; `doc/capabilities.json` at layout version 3 with a `"default"` per function row (no `"portable"`, no `"overridable"`); `tools/hash/doc_tables.py` renders `doc/backends.adoc` and `doc/measurements.adoc`, the reference of 2026-11-16, and its `--check` guards `measurements.json`'s digest; `pbkdf2_sha512_key` in every lane, the times by hand with `bench/collect.py`; the chapter `choosing_backend.adoc` and `guide.json`'s use-case field `backend` (layout version 2)

## 2. Increment 2: libsodium's whole hashing surface (stream `sodium-2`)

- [ ] 2.1 The families `digest`, `hasher`, `mac`, `argon2` (plan: Task 25; first, every later increment needs it)
- [ ] 2.2 BLAKE2b with key, salt, personalisation and length (plan: Task 26)
- [ ] 2.3 SipHash-2-4, SipHash-2-4x, BLAKE2b subkeys (plan: Task 27)
- [ ] 2.4 SHAKE, TurboSHAKE, SHA-2, SHA-3 and HMACs as digests and MACs (plan: Task 28)
- [ ] 2.5 Argon2i, `argon2` on libsodium, increment 2 whole (plan: Task 29, Task 47)

## 3. Increment 3: XKCP's Keccak family (stream `xkcp-3`)

- [ ] 3.1 XKCP's high-level files (plan: Task 30)
- [ ] 3.2 Keccak-224 to -512, SHA-3 and SHAKE on XKCP (plan: Task 31)
- [ ] 3.3 cSHAKE, KMAC, TupleHash, ParallelHash (plan: Task 32)
- [ ] 3.4 KangarooTwelve and TurboSHAKE, increment 3 whole (plan: Task 33, Task 47)

## 4. Increment 4: OpenSSL's whole surface (stream `openssl-4`)

- [ ] 4.1 OpenSSL's digests, KECCAK-256 and SM3 included (plan: Task 34)
- [ ] 4.2 KMAC, BLAKE2 MACs, HMAC (plan: Task 35)
- [ ] 4.3 scrypt and Argon2 with every RFC 9106 parameter (plan: Task 36)
- [ ] 4.4 KBKDF, SSKDF, X9.63 KDF (plan: Task 37)
- [ ] 4.5 Increment 4 whole (plan: Task 38, Task 47)

## 5. Increment 5: BearSSL's hashes and HMAC (stream `bearssl-5`)

- [ ] 5.1 BearSSL's hashes, HMAC and SHAKE (plan: Task 39)
- [ ] 5.2 Increment 5 whole (plan: Task 40, Task 47)

## 6. Increment 6: Botan 3 (stream `botan-6`; after webcpp::crypto provisions Botan)

- [ ] 6.1 Botan's column, checked at its pin (plan: Task 41)
- [ ] 6.2 Botan's hashes and MACs (plan: Task 42)
- [ ] 6.3 Botan's KDFs and password hashes, increment 6 whole (plan: Task 43, Task 47)

## 7. Increment 7: the browser backend, through the asynchronous family (stream `async-7`)

- [ ] 7.1 The cheap asynchronous functions by post, and the heavy ones on another thread where the build has threads (plan: Task 44)
- [ ] 7.2 `backends::webcrypto` on Emscripten through `browser::async_await` (plan: Task 45)
- [ ] 7.3 Cancellation, allocation and tokens; increment 7 whole (plan: Task 46, Task 47)

## 8. Publication

- [ ] 8.1 The owner creates `webcpporg/hash` and adds the submodule `libs/hash` (plan: Owner gates G1, G2)
- [ ] 8.2 Every pull request merged by the owner, the CI's MSVC and Apple clang lanes green (plan: G4)
- [ ] 8.3 The Boost.Hash2 contribution opened by the owner (plan: G5)
