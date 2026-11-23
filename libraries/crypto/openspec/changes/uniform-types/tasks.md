# Tasks

Implemented after crypto/sign-unified merges and before crypto's formats. Each task writes its
tests first; the library builds at the end of every task, and every caller of a removed name moves
in the task that removes it. A plan with code goes in `reference/plan.md` when the proposal is
accepted.

## 1. Typed signatures

- [ ] 1.1 The signature class template in `detail` and its five types, `from_bytes`, `bytes()`, `recovery_id()`; the scheme traits gain the signature type; message tests for a signature of another curve or scheme (design D1)
- [ ] 1.2 Every `sign` returns its type and every `verify` and `recover` takes it, the four forms, the asynchronous completions; the span overloads removed; `normalize_low_s` and `is_low_s` on the ECDSA types; callers moved (D1, R1)

## 2. X25519 typed

- [ ] 2.1 `x25519::public_key` and `x25519::basic_keypair<B>` with `from_secret`, `generate`, `public_key()`, `secret()`, `agree`, the asynchronous members and the free forms; the free `public_key(secret)` and `agree(secret, peer)` removed; RFC 7748's vectors (D2)
- [ ] 2.2 `kx_keypair` on the typed key pair, `from_keypair`, `generate` and `derive_from_seed`, tested against `crypto_kx_seed_keypair` and against `from_keypair` on the same bytes; the boxes and the sealed box on typed keys (D3, R5, R6, owner decision 7)
- [ ] 2.3 Ed25519's `to_x25519()` on the key pair and the public key, the free `crypto::to_x25519`; `x25519_secret()` and `to_x25519_public` removed (D3, R4)

- [ ] 2.4 `crypto::public_key_encryption` and `crypto::key_exchange`: the boxes, the sealed boxes, `kx_keypair` and `session_keys` moved with their headers, `crypto::hybrid` removed with no alias; every caller moved (tests, examples, fixtures, the page, `crypto.json` and its generated files, the MSVC and message tests), the convenience header updated (D3b, owner decision 8)

- [ ] 2.5 Factories named by their input: every `make` and `async_make` renamed (`from_secret`, `from_bytes`, `from_coordinates`, `from_key`, `from_keys`, `from_keypair`), `make.hpp` to `factories.hpp`, no alias; every caller moved; a message test that `make` no longer exists (D3c, owner decision 9)

## 3. Concepts

- [ ] 3.1 `keypair`, `signing_keypair`, `agreement_keypair` and each curve's `static_assert`s; `crypto::sign`, `crypto::agree` and their asynchronous forms constrained; message tests (D4)
- [ ] 3.2 `<webcpp/crypto/aead.hpp>`: `aead`, `streaming_aead`, `authenticated_box`; secretbox out of the AEAD template without additional data; each cipher's `static_assert`; the free `seal` and `open` in both forms (D5, R7)
- [ ] 3.3 `async_seal`, `async_open` and `async_from_key` for every AEAD and box on native backends, the browser stand-in's refusal (D6)

## 4. The page, the generated files and the delivery

- [ ] 4.1 The guide's chapters (signatures, X25519, AEADs), `crypto.json` and its generated files, the changelog line
- [ ] 4.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end
