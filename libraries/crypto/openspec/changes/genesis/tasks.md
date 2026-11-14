# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. Nothing is built: the library's repository has no commit, so
every task is open. The plan's tasks are numbered 0a, 0, 1 to 25, 25a, 25b, 26 to 30, 30a and
31 to 39; here they are grouped by version 0.1 (the plan's revision of 2026-11-14: increment 1
without its deferred parts, and increment 2), then by what comes after it. `reference/STATUS.md`
says how to start.

## 1. Version 0.1, increment 1: the foundation, and AES-GCM on BearSSL and OpenSSL

- [ ] 1.1 The worktree from `origin/main`, the ledger and the baseline (plan: Task 0; Task 0a is obsolete)
- [ ] 1.2 The library's skeleton, the second project id `/third_party` and the test Jamfile's rule `crypto-run` (plan: Task 1)
- [ ] 1.3 The errors: `error`, its category, values 1 to 12, 13 and 14 reserved (plan: Task 2)
- [ ] 1.4 `equal`, and the wipe (plan: Task 3)
- [ ] 1.5 `random_device` and the entropy of each target (plan: Task 4)
- [ ] 1.6 BearSSL from the mirror, file by file, with its canary and the own lane `c-suites` (plan: Task 5)
- [ ] 1.7 The configuration's refusals, the default backend, and BearSSL's three layers (plan: Task 6)
- [ ] 1.8 The block cipher's shape, and GCM one shot on BearSSL (plan: Task 7)
- [ ] 1.9 The incremental seal, and the limits (plan: Task 8)
- [ ] 1.10 Wycheproof, through BearSSL (plan: Task 9)
- [ ] 1.11 The user's Jamfiles as fixtures of a test of the tools (plan: Task 10)
- [ ] 1.12 crypto's OpenSSL alias over the source-built provider (plan: Task 11)
- [ ] 1.13 The OpenSSL backend: AES and AES-GCM, OpenSSL's error queue, two threads (plan: Task 12)
- [ ] 1.14 BearSSL and OpenSSL agree (plan: Task 13)
- [ ] 1.15 The default backend, the convenience headers, and their net on MSVC (plan: Task 14)
- [ ] 1.16 Allocation on every backend, and BearSSL's seeder (plan: Task 17)
- [ ] 1.17 Every message at link time and on MSVC, and the canary's pins (plan: Task 18)
- [ ] 1.18 `<windows.h>` before and after every header, on MSVC (plan: Task 19)
- [ ] 1.19 A library named and not called adds no byte: `webcpp.same-program` (plan: Task 21)
- [ ] 1.20 The documentation: the page, the user's Jamfiles, the examples (plan: Task 24)
- [ ] 1.21 Increment 1 whole: the rulebook, green, review and the pull requests (plan: Task 25)

## 2. Version 0.1, increment 2: libsecp256k1 and ctaes

- [ ] 2.1 libsecp256k1 built file by file, its tables feature, its canary and its own tests (plan: Task 26)
- [ ] 2.2 `secp256k1::keypair`, `verify_schnorr` and `verify_ecdsa` (plan: Task 27)
- [ ] 2.3 secp256k1's vectors: BIP340, Wycheproof's Bitcoin ECDSA, and ECDH (plan: Task 28)
- [ ] 2.4 ctaes, Bitcoin Core's copy: the block cipher alone (plan: Task 29)
- [ ] 2.5 Increment 2 whole: green, review, the pull requests (plan: Task 30)
- [ ] 2.6 trystero's build onto `/third_party//secp256k1`, the CI's external libsecp256k1 removed (plan: Task 30a)

## 3. After version 0.1: the deferred parts of increment 1

- [ ] 3.1 OpenSSL's memory functions, an option of the config: the adapter and the install (plan: Task 15)
- [ ] 3.2 The memory option, process by process (plan: Task 16)
- [ ] 3.3 Constant time: Valgrind, MemorySanitizer and dudect (plan: Task 20)
- [ ] 3.4 Sizes and throughput, and `runners.yml` (plan: Task 22)
- [ ] 3.5 The browser: WebCrypto as an outside oracle, the audio worklet, throughput in Chrome (plan: Task 23)
- [ ] 3.6 The nightly level: the heavy own lanes, the tools' macOS and Windows legs (plan: Task 25b; owner gate O2)
- [ ] 3.7 NIST CAVP's archives and their tests (plan: Task 9's deferred half; owner gate O3)

## 4. After version 0.1: increment 3, trystero's migration

- [ ] 4.1 trystero seals with `aes256_gcm` directly, and draws from `random_device` (plan: Task 31)
- [ ] 4.2 trystero signs with `secp256k1::keypair` (plan: Task 32; its build half is Task 30a)
- [ ] 4.3 The memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake (plan: Task 33)
- [ ] 4.4 Increment 3 whole: green, review, the pull requests (plan: Task 34)

## 5. After version 0.1: increment 4, ChaCha20-Poly1305

- [ ] 5.1 ChaCha20-Poly1305 on BearSSL and OpenSSL, with its vectors, its cross-backend lane and its part of the memory option's tests (plan: Task 35)
- [ ] 5.2 Increment 4 whole: green, review, the pull requests (plan: Task 36)

## 6. After version 0.1: increment 5, Botan, native, with exceptions

- [ ] 6.1 Botan found, or the build stops (plan: Task 37; owner item O5, `brew install botan`)
- [ ] 6.2 The Botan backend, with exceptions only (plan: Task 38)
- [ ] 6.3 Botan in every native lane, its page, and the last green (plan: Task 39)

## 7. Publication

- [ ] 7.1 The owner creates `webcpporg/crypto`, public, BSL-1.0 (plan: gate G1)
- [ ] 7.2 The owner adds it as the submodule `libs/crypto` and merges the superproject's pull request (plan: gate G2, Tasks 25 and 30)
- [ ] 7.3 The proposal records version 0.1 as built once the CI is green on the library's and the superproject's `main` (plan: Task 25)
