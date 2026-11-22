# Tasks

Each task points to its section in `reference/plan.md`.

## 1. The shared vocabulary

- [ ] 1.1 `<webcpp/crypto/signing.hpp>`: the tags, `prehashed`, the traits and the concepts, with their tests (plan: Task 1)

## 2. Each curve signs and verifies with the scheme named

- [ ] 2.1 P-256: `sign(ecdsa_sha256, input)` with low S, `crypto::verify`, `normalize_low_s` and `is_low_s`, the old names removed, its callers moved (plan: Task 2)
- [ ] 2.2 Ed25519: `sign(eddsa, message)`, `crypto::verify`, `ph::verify`'s order, its callers moved (plan: Task 3)
- [ ] 2.3 secp256k1: the three schemes, `recover`, `normalize_low_s` and `is_low_s`, the Bitcoin sighash example, the old names removed, its callers moved (plan: Task 4)

## 3. The asynchronous family

- [ ] 3.1 `async_sign` and `async_verify` with the scheme and the concepts, the message tests (plan: Task 5)

## 3b. The key pair contract and crypto's reference

- [ ] 3.2 crypto's refusals never stop a dependent's reference (plan: Task 5a)
- [ ] 3.3 `crypto::keypair`, typed public keys, `agree`, `secret()` on Ed25519 and P-256 (plan: Task 5b)
- [ ] 3.4 The same on secp256k1 (plan: Task 5c)

## 4. The page, the generated files and the delivery

- [ ] 4.1 The page, `crypto.json` and its generated files, the changelog line (plan: Task 6)
- [ ] 4.2 Full local verification, pull requests, merge (plan: Task 7)
