# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. The plan's tasks are numbered 0a, 0, 1 to 25, 25a, 25b, 26 to
30, 30a and 31 to 39; here they are grouped by version 0.1 (the plan's revision of 2026-11-14:
increment 1 without its deferred parts, and increment 2), which is merged, then by what comes
after it, as the owner's decision of 2026-11-17 and the plan's revision of that day place it
(`design.md` D23). **The order of all the remaining work, and its parallel streams, is the
foundation change's `reference/roadmap.md`.** `reference/STATUS.md` says how to resume.

## 1. Version 0.1, increment 1: the foundation, and AES-GCM on BearSSL and OpenSSL

- [x] 1.1 The worktree from `origin/main`, the ledger and the baseline (plan: Task 0; Task 0a is obsolete)
- [x] 1.2 The library's skeleton, the second project id `/third_party` and the test Jamfile's rule `crypto-run` (plan: Task 1)
- [x] 1.3 The errors: `error`, its category, values 1 to 12, 13 and 14 reserved (plan: Task 2)
- [x] 1.4 `equal`, and the wipe (plan: Task 3)
- [x] 1.5 `random_device` and the entropy of each target (plan: Task 4)
- [x] 1.6 BearSSL from the mirror, file by file, with its canary and the own lane `c-suites` (plan: Task 5)
- [x] 1.7 The configuration's refusals, the default backend, and BearSSL's three layers (plan: Task 6)
- [x] 1.8 The block cipher's shape, and GCM one shot on BearSSL (plan: Task 7)
- [x] 1.9 The incremental seal, and the limits (plan: Task 8)
- [x] 1.10 Wycheproof, through BearSSL (plan: Task 9)
- [x] 1.11 The user's Jamfiles as fixtures of a test of the tools (plan: Task 10)
- [x] 1.12 crypto's OpenSSL alias over the source-built provider (plan: Task 11)
- [x] 1.13 The OpenSSL backend: AES and AES-GCM, OpenSSL's error queue, two threads (plan: Task 12)
- [x] 1.14 BearSSL and OpenSSL agree (plan: Task 13)
- [x] 1.15 The default backend, the convenience headers, and their net on MSVC (plan: Task 14)
- [x] 1.16 Allocation on every backend, and BearSSL's seeder (plan: Task 17)
- [x] 1.17 Every message at link time and on MSVC, and the canary's pins (plan: Task 18)
- [x] 1.18 `<windows.h>` before and after every header, on MSVC (plan: Task 19)
- [x] 1.19 A library named and not called adds no byte: `webcpp.same-program` (plan: Task 21)
- [x] 1.20 The documentation: the page, the user's Jamfiles, the examples (plan: Task 24)
- [x] 1.21 Increment 1 whole: the rulebook, green, review and the pull requests (plan: Task 25)

## 2. Version 0.1, increment 2: libsecp256k1 and ctaes

- [x] 2.1 libsecp256k1 built file by file, its tables feature, its canary and its own tests (plan: Task 26)
- [x] 2.2 `secp256k1::keypair`, `verify_schnorr` and `verify_ecdsa` (plan: Task 27)
- [x] 2.3 secp256k1's vectors: BIP340, Wycheproof's Bitcoin ECDSA, and ECDH (plan: Task 28)
- [x] 2.3b secp256k1's encodings and tweak, primitives only: strict DER encoding and decoding, low-S normalization, the generic x-only tweak of a key and of a key pair; BIP341's vectors as its test (plan: Task 27b, reshaped on 2026-11-15)
- [x] 2.4 ctaes, Bitcoin Core's copy: the block cipher alone (plan: Task 29)
- [x] 2.5 Increment 2 whole: green, review, the pull requests (plan: Task 30)
- [x] 2.6 trystero's build onto `/third_party//secp256k1`, the CI's external libsecp256k1 removed (plan: Task 30a)

## 3. After version 0.1: the deferred parts of increment 1

Tasks 3.1 and 3.2 are stream M with group 4. Tasks 3.3 to 3.5 are built by the foundation
change (its tasks 1.14a and 1.14, 1.11, and 4.4); they are checked here when those are.

- [ ] 3.1 OpenSSL's memory functions, an option of the config: the adapter and the install (plan: Task 15; stream M; native only, refused on every WebAssembly target, WASI included)
- [ ] 3.2 The memory option, process by process (plan: Task 16; stream M; native only, refused on every WebAssembly target, WASI included)
- [ ] 3.3 Constant time: Valgrind, MemorySanitizer and dudect, over version 0.1's backends (plan: Task 20; built as the foundation's tasks 1.14a and 1.14)
- [ ] 3.4 Sizes from release builds through `webcpp.program-sizes`, throughput measured by hand from release builds, deciding nothing; no `runners.yml` (plan: Task 22, re-cut; built as the foundation's task 1.11)
- [ ] 3.5 The browser: WebCrypto as an outside oracle, the audio worklet, throughput in Chrome from release builds (plan: Task 23; built as the foundation's task 4.4)
- [ ] 3.6 The nightly level: the heavy own lanes, the tools' macOS and Windows legs (plan: Task 25b; owner gate O2; stream N)
- [ ] 3.7 NIST CAVP's archives and their tests (plan: Task 9's deferred half; owner gate O3; stream N)

## 4. After version 0.1: increment 3, the memory option without trystero's migration (stream M)

Tasks 31 and 32 (trystero's sealer, `random` and signer; the plan's "Increment 3: trystero's
migration") are dropped by the owner, 2026-11-17 (`design.md` D23); they stay in the plan as the reference of a future change of
trystero's own.

- [ ] 4.3 The memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake, in crypto's own `test/openssl_memory/` (plan: Task 33, moved by the revision of 2026-11-17; the owner's answer Q6; native only, refused on every WebAssembly target, WASI included)
- [ ] 4.4 Increment 3 whole: green, review, the pull requests (plan: Task 34)

## 5. After version 0.1: increment 4, ChaCha20-Poly1305 (built by the foundation, version 0.2)

- [ ] 5.1 ChaCha20-Poly1305 on BearSSL and OpenSSL over `ciphers::aead`, with its vectors, its three-way cross-backend lane and its part of the memory option's tests (plan: Task 35; built as the foundation's task 1.8b)
- [ ] 5.2 Increment 4 whole: green, review, the pull requests (plan: Task 36; built as the foundation's task 1.16)

## 6. After version 0.1: increment 5, Botan with exceptions (built by the foundation's increment 7)

Task 37 (Botan found on the machine) is superseded by the foundation's D15: Botan is a pinned
submodule built by b2, natively and on Emscripten.

- [ ] 6.2 The Botan backend's genesis cells, with exceptions only: the block cipher, AES-GCM and ChaCha20-Poly1305 (plan: Task 38; built as the foundation's task 7.3a)
- [ ] 6.3 Botan in every lane it builds for, the three-way agreement, its constant-time cases, its page (plan: Task 39; built as the foundation's task 7.4)

## 7. Publication

- [x] 7.1 The owner creates `webcpporg/crypto`, public, BSL-1.0 (plan: gate G1)
- [x] 7.2 The owner adds it as the submodule `libs/crypto` and merges the superproject's pull request (plan: gate G2, Tasks 25 and 30)
- [ ] 7.3 The proposal records version 0.1 as built once the CI is green on the library's and the superproject's `main` (plan: Task 25; on 2026-11-17 the library's last run on `main` was cancelled and the superproject's queued)
