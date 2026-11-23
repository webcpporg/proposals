# Tasks

Implemented after crypto/formats merges. Each task writes its tests first; the library builds at
the end of every task. A plan with code goes in `reference/plan.md` when the proposal is accepted.

## 1. The concepts

- [ ] 1.1 `detail/backend/*.hpp`: the shared requirements and the symmetric, box and kx concepts, each with its documented contract; message tests for a missing operation (design D1, D2, D4)
- [ ] 1.2 The key concepts of Ed25519, X25519 and P-256 split by import, generate, public, use and export; P-256's point and DER; base64; entropy; the asynchronous operations (D3, D4, D6)

## 2. The engines checked

- [ ] 2.1 Every backend header asserts its engines' concepts; the formats engines dissolve into key and encoding capabilities (D5, R4)
- [ ] 2.2 secp256k1's calls behind `secp256k1_engine<Capability, backends::secp256k1>`, split into its capabilities, each concept asserted; its no-allocation and constant-time tests unchanged (D8, R2)
- [ ] 2.3 The test-only opaque backend for P-256 and X25519, synchronous and asynchronous, outside the table; the `async_*` concepts tested on it; `generate()` through a backend's own generation (D3, R3, R6, R9)

## 3. The table and the public API

- [ ] 3.1 `crypto.json`'s family rows list their capabilities, one cell per capability; `generate.py` writes `supports_capability_v`, `detail::shipped_backends` and the specialization marker's check; `capabilities_test` checks the cells against the concepts both ways on every target and lane; `supports_v` header-free, meaning the use operations (D5, D7, R1, R7, R8)
- [ ] 3.2 Every public member asserts the capability it uses, after the primaries' generated refusals; message tests for each area (D9)

## 4. The page and the delivery

- [ ] 4.1 The guide's chapter on backends: the catalogue, the contract, the opaque key as a program sees it, no public backend API (D10); `crypto.json`'s generated files; the changelog line
- [ ] 4.2 Full local verification on every lane, pull requests, merge, one CI at the end
