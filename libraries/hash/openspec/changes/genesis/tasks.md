# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. Nothing is built: the library's repository has no commit, so
every task is open. The groups are the spec's increments (spec 1.5); inside each, the plan's
"The order of the work, and the parallel streams" says which tasks run in parallel, by backend.
`reference/STATUS.md` says how to start.

## 1. Increment 1: Hash2, Keccak-256, BLAKE3, HKDF, PBKDF2, Argon2id, scrypt, the profile

- [ ] 1.1 The worktree, the ledger and the baseline (plan: Task 0)
- [ ] 1.2 The library's skeleton, the test rule `hash-run` and chapter 17's entry (plan: Task 1)
- [ ] 1.3 The errors, values 1 to 8 (9 comes with the families) (plan: Task 2; stream `errors`)
- [ ] 1.4 Boost.Hash2's algorithms and HMAC by alias (plan: Task 3; stream `hash2`)
- [ ] 1.5 XKCP's Keccak-p[1600], its adapter, canary and own tests (plan: Task 4; stream `keccak`)
- [ ] 1.6 `keccak_256` with Hash2's sponge, Hash2 as the oracle (plan: Task 5; stream `keccak`)
- [ ] 1.7 BLAKE3's C implementation file by file, its canary and own suite (plan: Task 6; stream `blake3`)
- [ ] 1.8 `blake3` in Hash2's form, with its vectors (plan: Task 7; stream `blake3`)
- [ ] 1.9 libsodium file by file, its canary and its own tests (plan: Task 8; stream `sodium`)
- [ ] 1.10 The key derivations' layer, and HKDF on `backends::hash2` (plan: Task 9; stream `kdf`)
- [ ] 1.11 PBKDF2 on `backends::hash2` (plan: Task 10; stream `kdf`)
- [ ] 1.12 HKDF on `backends::sodium` (plan: Task 11; stream `kdf-sodium`)
- [ ] 1.13 `/webcpp/hash//openssl` and `backends::openssl` (plan: Task 12; stream `kdf-openssl`)
- [ ] 1.14 HKDF on `backends::bearssl`, after webcpp::crypto's BearSSL is on `main` (plan: Task 13; stream `kdf-bearssl`)
- [ ] 1.15 The profile, the overrides, the default backend, the capability table (plan: Task 14; stream `default`)
- [ ] 1.16 Argon2id on libsodium, RFC 9106 through its core (plan: Task 15; stream `pwhash`)
- [ ] 1.17 scrypt on libsodium (plan: Task 16; stream `pwhash`)
- [ ] 1.18 Agreement between backends on every target (plan: Task 17; stream `agreement`)
- [ ] 1.19 WebCrypto under node as an outside oracle (plan: Task 18; stream `webcrypto`)
- [ ] 1.20 The user's Jamfiles, the canaries at link time, one program (plan: Task 19; stream `build`)
- [ ] 1.21 Allocation (plan: Task 20; stream `allocation`)
- [ ] 1.22 The measured table and `detail/ranking.hpp` (plan: Task 21; stream `measure`)
- [ ] 1.23 PBKDF2-HMAC-SHA-512 at 1,000,000 iterations (plan: Task 22; stream `throughput`)
- [ ] 1.24 The documentation, the examples and the Boost.Hash2 proposal's text (plan: Task 23)
- [ ] 1.25 Increment 1 whole: the rulebook, green, review, the gates (plan: Task 24)

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

- [ ] 7.1 The asynchronous family, completing through post everywhere (plan: Task 44)
- [ ] 7.2 `backends::webcrypto` on Emscripten through `browser::async_await` (plan: Task 45)
- [ ] 7.3 Cancellation, allocation and tokens; increment 7 whole (plan: Task 46, Task 47)

## 8. Publication

- [ ] 8.1 The owner creates `webcpporg/hash` and adds the submodule `libs/hash` (plan: Owner gates G1, G2)
- [ ] 8.2 Every pull request merged by the owner, the CI's MSVC and Apple clang lanes green (plan: G4)
- [ ] 8.3 The Boost.Hash2 contribution opened by the owner (plan: G5)
