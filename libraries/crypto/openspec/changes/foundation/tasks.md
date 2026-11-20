# Tasks

Each task of increment 1 points to its section in `reference/plan.md`, which holds its files,
interfaces, steps, code and expected output, read through the plan's revision of 2026-11-17;
tasks marked from the genesis point to `../genesis/reference/plan.md`. Increment 2 is written
there in full (Tasks 2.1 to 2.7); increments 3 to 12 are outlined there and written in full,
task by task, when each starts; each one's documentation step follows design D19. **The order and the parallel streams are `reference/roadmap.md`**
(design D35): wave 1 runs groups 1 (stream A) and 2 (stream B) beside the genesis's memory
option (stream M, `../genesis/tasks.md` group 4); wave 2 runs groups 3 to 8, 10 and 11 after
group 1 merges; wave 3 runs 9 after 6 and 12 after 8. Group 1 (increment 1) is merged
(`webcpporg/crypto#4`, `dae9e84`; `webcpporg/webcpp#35`, `485c5e0`; 2026-11-19); group 2
(increment 2) is built on its branches and awaits its pull requests; `reference/STATUS.md`
says where each stands.

## 1. Increment 1: libsodium's core, ChaCha20-Poly1305 on three backends, the capability table, the defaults (version 0.2; stream A)

Group A1, in order:

- [x] 1.1 The worktree from `origin/main`, the ledger and the baseline (plan: Task 1)
- [x] 1.2 crypto's canary on `/third_party//sodium`, `backends/sodium.hpp` and its three layers (plan: Task 2)
- [x] 1.3 `secret<N>` and the errors 15 to 20 (plan: Task 3)
- [x] 1.4 The capability table grown in version 0.1's `doc/tables/crypto.json`, `generate.py`'s new outputs, `supports_v` and the generated refusals (plan: Task 4', design D33)
- [x] 1.5 One default per family and target with its reason, `default_backend_t`, the "needs" messages, version 0.1's two defines refused with the type to write (plan: Task 5', design D32)

Group A2, in parallel:

- [x] 1.6 Ed25519 and Ed25519ph on libsodium (plan: Task 6)
- [x] 1.6b Ed25519's strict policy, and the twelve "Taming the many EdDSAs" cases per backend, committed and published (plan: Task 6b)
- [x] 1.7 X25519 and the Ed25519 to X25519 conversion (plan: Task 7)
- [x] 1.8 `ciphers::aead<Family, Backend>` and the three ChaCha20-Poly1305 AEADs on libsodium (plan: Task 8)
- [x] 1.8b ChaCha20-Poly1305 on BearSSL and OpenSSL over `ciphers::aead`, the genesis's increment 4: RFC 8439, Wycheproof, three-way agreement, allocation, OpenSSL's error queue, ctaes's refusal, its constant-time cases (plan: Task 8b; genesis plan: Task 35)
- [x] 1.11 The own lane `sizes` through `webcpp.program-sizes` (release builds only) and the throughput page measured by hand from release builds, version 0.1's operations first; no ranking (plan: Task 11'; genesis plan: Task 22)
- [x] 1.12 Threads: `WEBCPP_CRYPTO_HAS_THREADS` and the refusal of a heavy variant without them (plan: Task 12)
- [x] 1.14a The constant-time harness over version 0.1's backends: BearSSL, OpenSSL with assembly, ctaes, libsecp256k1's `ctime_tests.c`, the planted leak (plan: Task 14; genesis plan: Task 20)

Group A3, in parallel:

- [x] 1.7b SPKI, PKCS#8 and PEM for Ed25519 and X25519, strict, with OpenSSL and WebCrypto as oracles (plan: Task 7b)
- [x] 1.9 `hybrid::box`, sealed boxes and `ciphers::secretbox`, in both forms (plan: Task 9)
- [x] 1.10 `kx` (plan: Task 10)
- [x] 1.13 Vectors, libsodium's own suite in `c-suites`, node's WebCrypto as the oracle for Ed25519 and X25519 (plan: Task 13)
- [x] 1.14 The constant-time lane over libsodium's paths, the ChaCha engines and the new guards (plan: Task 14)

Group A4:

- [x] 1.15 The documentation (design D19): `generate.py`'s comparison tables, backend lists with each default's reason, matrices from the release-build tables, decision table and `llms.txt`, the sections of increment 1, the teaching chapters' first sections (plan: Task 15)
- [x] 1.16 Increment 1 whole, with the genesis's increment 4 review items: the rulebook, green, review and the pull requests (plan: Task 16; genesis plan: Task 36)

## 2. Increment 2: libsecp256k1's remaining modules (version 0.2, beside 1; stream B)

It branches from stream A's branch once 1.3 (`secret<N>`) is committed, adds its capability
rows after 1.4, rebases onto `main` when group 1 merges, and merges after it. Built
(2026-11-19) on the branches `crypto/secp256k1-modules` of the library and the superproject,
on stream A's final heads, whose trees `main`'s merges of group 1 equal; where the code
differs from the plan's text, the plan says so under "As built" in each task. The pull
requests, the CI's run (Task 2.7, Steps 5 and 7) and chapter 17 of the rulebook remain
(`reference/STATUS.md`).

- [x] 2.1 `ecdh` returns `secret<32>` (BREAKING); `ecdh_x` and `ecdh_point`, against Wycheproof's ECDH files (plan: Task 2.1)
- [x] 2.2 Recovery and hedged ECDSA, with libsecp256k1's own recovery cases; `public_key`, `parse_public_key` and `serialize_public_key` in both forms (plan: Task 2.2)
- [x] 2.3 Tweaks, negation and combination, the caller's workspace; SPKI, PKCS#8 and PEM for secp256k1, with OpenSSL and node as oracles (plan: Task 2.3)
- [x] 2.4 ElligatorSwift with BIP 324's vectors (plan: Task 2.4)
- [x] 2.5 MuSig2 with BIP 327's vectors, the consumed nonce (plan: Task 2.5)
- [x] 2.6 Silent payments with BIP 352's vectors (plan: Task 2.6)
- [x] 2.7 The families in the capability table, the constant-time cases, the size cells, the teaching chapters' secp256k1 sections; increment 2 whole (plan: Task 2.7; its review and a full local verification in place of Step 5's CI, which runs with the pull requests of Step 7)
- [x] 2.7d The owner's two MuSig2 examples, `musig_network` (every public part through a byte buffer) and `musig_taproot` (a Taproot key-path output), run on every lane against their `.expected` and shown in the MuSig2 section (plan: Task 2.7, "As built")

## 3. Increment 3: BearSSL's remaining parts (wave 2, stream 3)

- [ ] 3.1 P-256, P-384, P-521 ECDSA and ECDH, strict DER signatures and public key forms (uncompressed point, raw x and y); PKCS#8 and PEM (plan: Increment 3, step 1)
- [ ] 3.2 X25519 on BearSSL, joining libsodium in agreement; its ChaCha20-Poly1305 is task 1.8b (plan: Increment 3, step 2)
- [ ] 3.3 AES-CTR, AES-CBC, AES-CCM, AES-EAX (plan: Increment 3, step 3)
- [ ] 3.4 RSA: PKCS #1 v1.5, PSS, OAEP, key generation, PKCS#8; its heavy variants (plan: Increment 3, step 4)
- [ ] 3.5 Increment 3 whole (plan: Increment 3, step 5)

## 4. Increment 4: the browser backend and the asynchronous family (wave 2, stream 4; the genesis's Task 23)

- [ ] 4.1 `backends::browser`: the key table, `EM_JS` calls, the handle's lifetime (plan: Increment 4, step 1)
- [ ] 4.2 `async.hpp` over the browser and posting over synchronous backends (plan: Increment 4, step 2)
- [ ] 4.3 The errors of a rejected promise, cancellation, run-time availability (plan: Increment 4, step 3)
- [ ] 4.4 The lane `browser`: the oracle in node and Chrome for every algorithm on `main`, the audio worklet's `not_supported`, Chrome's throughput page from release builds (plan: Increment 4, step 4; genesis plan: Task 23)
- [ ] 4.5 Increment 4 whole (plan: Increment 4, step 5)

## 5. Increment 5: libsodium completed (wave 2, stream 5)

- [ ] 5.1 ristretto255 and edwards25519 (plan: Increment 5, step 1)
- [ ] 5.2 ML-KEM-768 and X-Wing (plan: Increment 5, step 2)
- [ ] 5.3 secretstream, AEGIS, AES256-GCM, `auth`, `poly1305`, the stream ciphers and cores (plan: Increment 5, step 3)
- [ ] 5.4 Utilities: random, memory, padding, encodings, ipcrypt, guarded memory (plan: Increment 5, step 4)
- [ ] 5.5 Increment 5 whole (plan: Increment 5, step 5)

## 6. Increment 6: blst (wave 2, stream 6)

- [ ] 6.1 The submodule, its two translation units, assembly per target, its canary (plan: Increment 6, step 1)
- [ ] 6.2 `curves::bls12_381::min_pk` and `min_sig`, proofs of possession, hash-to-curve; the heavy variants (plan: Increment 6, step 2)
- [ ] 6.3 The consensus specifications' vectors; increment 6 whole (plan: Increment 6, step 3)

## 7. Increment 7: Botan as a submodule, the genesis's increment 5 (wave 2, stream 7)

- [ ] 7.1 The spike: Botan 3.14.1 on wasip2 and wasip3 (plan: Increment 7, step 1)
- [ ] 7.2 `tools/webcpp-botan.jam`, the submodule, its canary, chapter 5's new rule; the genesis's Task 37 is superseded (plan: Increment 7, step 2)
- [ ] 7.3a The genesis's cells on Botan: the AES block cipher, AES-GCM and ChaCha20-Poly1305, exceptions only, the mapping, the additional data in one piece, the counted `make` (plan: Increment 7, step 3; genesis plan: Task 38)
- [ ] 7.3 The Botan backend: every other family the table lists for it, `detail::botan_random`, SPKI, PKCS#8 and PEM, Ed25519's twelve cases (plan: Increment 7, step 3)
- [ ] 7.4 Increment 7 whole: the three-way agreement and Botan's constant-time cases of the genesis's Task 39, every lane Botan builds for (plan: Increment 7, step 4; genesis plan: Task 39)

## 8. Increment 8: OpenSSL's remaining algorithms (wave 2, stream 8)

- [ ] 8.1 Curves and signatures: P-curves, Ed25519, Ed448, X25519, X448, RSA, with SPKI, PKCS#8 and PEM; Ed25519's twelve cases (plan: Increment 8, step 1)
- [ ] 8.2 ML-KEM, ML-DSA, SLH-DSA with ACVP's vectors; the heavy variants (plan: Increment 8, step 2)
- [ ] 8.3 AES modes and CMAC (its ChaCha20-Poly1305 is task 1.8b); the AES modes' native default becomes OpenSSL (design D32); the allocation per operation recorded (plan: Increment 8, step 3)
- [ ] 8.4 Increment 8 whole (plan: Increment 8, step 4)

## 9. Increment 9: KZG over c-kzg-4844 (wave 3, after 6)

- [ ] 9.1 The submodule built against webcpp's blst, c-kzg's own tests on blst v0.3.17 (plan: Increment 9, step 1)
- [ ] 9.2 `commitments::kzg::settings` and the commitment, proof and cell functions; their heavy variants (plan: Increment 9, step 2)
- [ ] 9.3 Increment 9 whole (plan: Increment 9, step 3)

## 10. Increment 10: BN254 over mcl (wave 2, stream 10)

- [ ] 10.1 The spike: mcl's portable configurations per target, without its JIT (plan: Increment 10, step 1)
- [ ] 10.2 The submodule, its canary, `curves::bn254::g1`, `g2`, `pairing_check` (plan: Increment 10, step 2)
- [ ] 10.3 Increment 10 whole (plan: Increment 10, step 3)

## 11. Increment 11: Shamir secret sharing over sss (wave 2, stream 11)

- [ ] 11.1 The submodule without `randombytes`, webcpp's `sss_randombytes.c`, MSVC's VLAs (plan: Increment 11, step 1)
- [ ] 11.2 `secret_sharing::shamir::split_key`, `combine_key`, `split`, `combine`, the constant-time lane (plan: Increment 11, step 2)
- [ ] 11.3 Increment 11 whole (plan: Increment 11, step 3)

## 12. Increment 12: HPKE (wave 3, after 8; WebAssembly after 7)

- [ ] 12.1 `hpke` on OpenSSL with RFC 9180's vectors (plan: Increment 12, step 1)
- [ ] 12.2 `hpke` on Botan, agreement with OpenSSL (plan: Increment 12, step 2)
- [ ] 12.3 Increment 12 whole (plan: Increment 12, step 3)

## 13. Records

- [ ] 13.1 The proposal records version 0.2 as built once the CI is green on the library's and the superproject's `main`
- [ ] 13.2 Each later increment's version recorded the same way
