# Tasks

Each task of increment 1 points to its section in `reference/plan.md`, which holds its files,
interfaces, steps, code and expected output. Increments 2 to 12 are outlined there and written
in full, task by task, when each starts; each one's documentation step follows design D19. Nothing is built. The work starts after version 0.1
has merged and webcpp::hash's `/third_party//sodium` is on the superproject's `main`
(`reference/STATUS.md`).

## 1. Increment 1: libsodium's core, profiles, the capability table (version 0.2)

- [ ] 1.1 The worktree from `origin/main`, the ledger and the baseline (plan: Task 1)
- [ ] 1.2 crypto's canary on `/third_party//sodium`, `backends/sodium.hpp` and its three layers (plan: Task 2)
- [ ] 1.3 `secret<N>` and the errors 15 to 20 (plan: Task 3)
- [ ] 1.4 The capability table, its generator, `supports_v` and the generated refusals (plan: Task 4)
- [ ] 1.5 The profiles, the overrides, `default_backend_t` and the seeded rankings (plan: Task 5)
- [ ] 1.6 Ed25519 and Ed25519ph on libsodium (plan: Task 6)
- [ ] 1.6b Ed25519's strict policy, and the twelve "Taming the many EdDSAs" cases per backend, committed and published (plan: Task 6b)
- [ ] 1.7 X25519 and the Ed25519 to X25519 conversion (plan: Task 7)
- [ ] 1.7b SPKI, PKCS#8 and PEM for Ed25519 and X25519, strict, with OpenSSL and WebCrypto as oracles (plan: Task 7b)
- [ ] 1.8 `ciphers::aead<Family, Backend>` and the three ChaCha20-Poly1305 AEADs on libsodium (plan: Task 8)
- [ ] 1.9 `hybrid::box`, sealed boxes and `ciphers::secretbox`, in both forms (plan: Task 9)
- [ ] 1.10 `kx` (plan: Task 10)
- [ ] 1.11 The measurement table, `measure.py`, the lanes `sizes` and `throughput`, the heavy column (plan: Task 11)
- [ ] 1.12 Threads: `WEBCPP_CRYPTO_HAS_THREADS` and the refusal of a heavy variant without them (plan: Task 12)
- [ ] 1.13 Vectors, libsodium's own suite in `c-suites`, node's WebCrypto as the oracle for Ed25519 and X25519 (plan: Task 13)
- [ ] 1.14 The constant-time lane over libsodium's paths and the new guards (plan: Task 14)
- [ ] 1.15 The documentation (design D19): `docs.py`, the generated comparison tables, backend lists, matrices, decision table and `llms.txt`, the sections of increment 1, the teaching chapters' first sections (plan: Task 15)
- [ ] 1.16 Increment 1 whole: the rulebook, green, review and the pull requests (plan: Task 16)

## 2. Increment 2: libsecp256k1's remaining modules (version 0.2, beside 1)

- [ ] 2.1 `ecdh` returns `secret<32>`; `ecdh_x` and `ecdh_point` (plan: Increment 2, step 1)
- [ ] 2.2 Recovery and hedged ECDSA, with libsecp256k1's own recovery tests (plan: Increment 2, step 2)
- [ ] 2.3 Tweaks, negation, combination, uncompressed keys; SPKI, PKCS#8 and PEM for secp256k1 (plan: Increment 2, step 3)
- [ ] 2.4 ElligatorSwift with BIP324's vectors (plan: Increment 2, step 4)
- [ ] 2.5 MuSig2 with BIP327's vectors, the consumed nonce (plan: Increment 2, step 5)
- [ ] 2.6 Silent payments with BIP352's vectors (plan: Increment 2, step 6)
- [ ] 2.7 The cryptocurrencies chapter's first sections; increment 2 whole (plan: Increment 2, step 7)

## 3. Increment 3: BearSSL's remaining parts (a parallel stream after 1)

- [ ] 3.1 P-256, P-384, P-521 ECDSA and ECDH, strict DER signatures and public key forms (uncompressed point, raw x and y); PKCS#8 and PEM (plan: Increment 3, step 1)
- [ ] 3.2 X25519 and ChaCha20-Poly1305 on BearSSL, joining libsodium in agreement (plan: Increment 3, step 2)
- [ ] 3.3 AES-CTR, AES-CBC, AES-CCM, AES-EAX (plan: Increment 3, step 3)
- [ ] 3.4 RSA: PKCS #1 v1.5, PSS, OAEP, key generation, PKCS#8; its heavy variants (plan: Increment 3, step 4)
- [ ] 3.5 Increment 3 whole (plan: Increment 3, step 5)

## 4. Increment 4: the browser backend and the asynchronous family (a parallel stream after 1)

- [ ] 4.1 `backends::browser`: the key table, `EM_JS` calls, the handle's lifetime (plan: Increment 4, step 1)
- [ ] 4.2 `async.hpp` over the browser and posting over synchronous backends (plan: Increment 4, step 2)
- [ ] 4.3 The errors of a rejected promise, cancellation, run-time availability (plan: Increment 4, step 3)
- [ ] 4.4 The lane `browser`: the oracle in node and Chrome for every algorithm on `main` (plan: Increment 4, step 4)
- [ ] 4.5 Increment 4 whole (plan: Increment 4, step 5)

## 5. Increment 5: libsodium completed (a parallel stream after 1)

- [ ] 5.1 ristretto255 and edwards25519 (plan: Increment 5, step 1)
- [ ] 5.2 ML-KEM-768 and X-Wing (plan: Increment 5, step 2)
- [ ] 5.3 secretstream, AEGIS, AES256-GCM, `auth`, `poly1305`, the stream ciphers and cores (plan: Increment 5, step 3)
- [ ] 5.4 Utilities: random, memory, padding, encodings, ipcrypt, guarded memory (plan: Increment 5, step 4)
- [ ] 5.5 Increment 5 whole (plan: Increment 5, step 5)

## 6. Increment 6: blst (a parallel stream after 1)

- [ ] 6.1 The submodule, its two translation units, assembly per target, its canary (plan: Increment 6, step 1)
- [ ] 6.2 `curves::bls12_381::min_pk` and `min_sig`, proofs of possession, hash-to-curve; the heavy variants (plan: Increment 6, step 2)
- [ ] 6.3 The consensus specifications' vectors; increment 6 whole (plan: Increment 6, step 3)

## 7. Increment 7: Botan as a submodule (a parallel stream after 1)

- [ ] 7.1 The spike: Botan 3.14.1 on wasip2 and wasip3 (plan: Increment 7, step 1)
- [ ] 7.2 `tools/webcpp-botan.jam`, the submodule, its canary, chapter 5's new rule (plan: Increment 7, step 2)
- [ ] 7.3 The Botan backend: every family the table lists for it, `detail::botan_random`, SPKI, PKCS#8 and PEM, Ed25519's twelve cases (plan: Increment 7, step 3)
- [ ] 7.4 Increment 7 whole (plan: Increment 7, step 4)

## 8. Increment 8: OpenSSL's remaining algorithms (a parallel stream after 1)

- [ ] 8.1 Curves and signatures: P-curves, Ed25519, Ed448, X25519, X448, RSA, with SPKI, PKCS#8 and PEM; Ed25519's twelve cases (plan: Increment 8, step 1)
- [ ] 8.2 ML-KEM, ML-DSA, SLH-DSA with ACVP's vectors; the heavy variants (plan: Increment 8, step 2)
- [ ] 8.3 AES modes and CMAC, ChaCha20-Poly1305; the allocation per operation recorded (plan: Increment 8, step 3)
- [ ] 8.4 Increment 8 whole (plan: Increment 8, step 4)

## 9. Increment 9: KZG over c-kzg-4844 (after 6)

- [ ] 9.1 The submodule built against webcpp's blst, c-kzg's own tests on blst v0.3.17 (plan: Increment 9, step 1)
- [ ] 9.2 `commitments::kzg::settings` and the commitment, proof and cell functions; their heavy variants (plan: Increment 9, step 2)
- [ ] 9.3 Increment 9 whole (plan: Increment 9, step 3)

## 10. Increment 10: BN254 over mcl (a parallel stream after 1)

- [ ] 10.1 The spike: mcl's portable configurations per target, without its JIT (plan: Increment 10, step 1)
- [ ] 10.2 The submodule, its canary, `curves::bn254::g1`, `g2`, `pairing_check` (plan: Increment 10, step 2)
- [ ] 10.3 Increment 10 whole (plan: Increment 10, step 3)

## 11. Increment 11: Shamir secret sharing over sss (a parallel stream after 1)

- [ ] 11.1 The submodule without `randombytes`, webcpp's `sss_randombytes.c`, MSVC's VLAs (plan: Increment 11, step 1)
- [ ] 11.2 `secret_sharing::shamir::split_key`, `combine_key`, `split`, `combine`, the constant-time lane (plan: Increment 11, step 2)
- [ ] 11.3 Increment 11 whole (plan: Increment 11, step 3)

## 12. Increment 12: HPKE (after 8; WebAssembly after 7)

- [ ] 12.1 `hpke` on OpenSSL with RFC 9180's vectors (plan: Increment 12, step 1)
- [ ] 12.2 `hpke` on Botan, agreement with OpenSSL (plan: Increment 12, step 2)
- [ ] 12.3 Increment 12 whole (plan: Increment 12, step 3)

## 13. Records

- [ ] 13.1 The proposal records version 0.2 as built once the CI is green on the library's and the superproject's `main`
- [ ] 13.2 Each later increment's version recorded the same way
