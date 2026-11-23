# Tasks

Each task points to its section in `reference/plan.md`.

## 1. The shared vocabulary

- [x] 1.1 `<webcpp/crypto/signing.hpp>`: the tags, `prehashed`, the traits and the concepts, with their tests (plan: Task 1)

## 2. Each curve signs and verifies with the scheme named

- [x] 2.1 P-256: `sign(ecdsa_sha256, input)` with low S, `crypto::verify`, `normalize_low_s` and `is_low_s`, the old names removed, its callers moved (plan: Task 2)
- [x] 2.2 Ed25519: `sign(eddsa, message)`, `crypto::verify`, `ph::verify`'s order, its callers moved (plan: Task 3)
- [x] 2.3 secp256k1: the three schemes, `recover`, `normalize_low_s` and `is_low_s`, the Bitcoin sighash example, the old names removed, its callers moved (plan: Task 4)

## 3. The asynchronous family

- [x] 3.1 `async_sign` and `async_verify` with the scheme and the concepts, the message tests (plan: Task 5)

## 3b. The key pair contract and crypto's reference

- [x] 3.2 crypto's refusals never stop a dependent's reference (plan: Task 5a)
- [x] 3.3 `crypto::keypair`, typed public keys, `agree`, `secret()` on Ed25519 and P-256 (plan: Task 5b)
- [x] 3.4 The same on secp256k1 (plan: Task 5c)
- [x] 3.5 Backend first in every `crypto::verify`, Ed25519's policy as an argument (plan: Task 4b)
- [x] 3.6 Every typed public key verifies; the free synchronous forms (plan: Task 5d-1)
- [x] 3.7 The asynchronous members, defined in `async.hpp` (plan: Task 5d-2)

## 4. The page, the generated files and the delivery

- [x] 4.1 The page, `crypto.json` and its generated files, the changelog line (plan: Task 6)
- [x] 4.1b One list of schemes (plan: Task 6b)
- [x] 4.1c The page shows each operation's four forms (plan: Task 6c)
- [x] 4.2 Full local verification, pull requests, merge (plan: Task 7)

## Open

- [ ] A Keccak-256 tag for Ethereum's recoverable signatures (`ecdsa_keccak256_recoverable`), design.md's open question.
- [ ] One-word local names left in headers outside the curves (`timing`, `executor`, `token`, `state`): a program's global of the same name can see MSVC's C4459.
- [ ] The final quiet measurement pass: the P-256 and secp256k1 rows of the size and throughput tables move with this change.
