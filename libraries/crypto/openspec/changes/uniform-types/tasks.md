# Tasks

Implemented after crypto/sign-unified merges and before crypto's formats. Each task writes its
tests first; the library builds at the end of every task, and every caller of a removed name moves
in the task that removes it. A plan with code goes in `reference/plan.md` when the proposal is
accepted.

## 1. Typed signatures

- [ ] 1.1 The signature class template in `detail` and its five types (no distinct Ed25519ph type: owner decision 12), `from_bytes`, `bytes()`, `recovery_id()`; the scheme traits gain the signature type; message tests for a signature of another curve or scheme (design D1)
- [ ] 1.2 Every `sign` returns its type and every `verify` and `recover` takes it, the four forms, the asynchronous completions; the span overloads removed; `normalize_low_s` and `is_low_s` on the ECDSA types; Ed25519ph's streaming signer retyped onto `ed25519::signature`; callers moved (D1, R1)
- [ ] 1.3 `crypto::ecdsa_keccak256_recoverable` on secp256k1: Keccak-256 inside or `prehashed`, the typed recoverable signature, `recover`, no `verify`; go-ethereum's vectors, an EIP-191 example, the refusal without XKCP (owner decision 10, D8)
- [ ] 1.4 Ed25519ph as the scheme `crypto::eddsa_prehashed` on Ed25519's key pair: `crypto::prehashed{digest}` of 64 bytes only, a message refused at compile time, `ed25519::signature`, `verify` member and free, asynchronous on native backends only; `curves::ed25519::ph` renamed `prehashed_signer`, no alias; RFC 8032's Ed25519ph vectors and the streaming signer equal to the scheme (owner decision 12, D9)

## 2. X25519 typed

- [ ] 2.1 `x25519::public_key` and `x25519::basic_keypair<B>` with `from_secret`, `generate`, `public_key()`, `secret()`, `agree`, the asynchronous members and the free forms; the free `public_key(secret)` and `agree(secret, peer)` removed; RFC 7748's vectors (D2)
- [ ] 2.2 `kx_keypair` on the typed key pair, `from_keypair`, `generate` and `derive_from_seed`, tested against `crypto_kx_seed_keypair` and against `from_keypair` on the same bytes; the boxes and the sealed box on typed keys (D3, R5, R6, owner decision 7)
- [ ] 2.3 Ed25519's `to_x25519()` on the key pair and the public key, the free `crypto::to_x25519`; `x25519_secret()` and `to_x25519_public` removed (D3, R4)

- [ ] 2.4 `crypto::public_key_encryption` and `crypto::key_exchange`: the boxes, the sealed boxes, `kx_keypair` and `session_keys` moved with their headers, `crypto::hybrid` removed with no alias; every caller moved (tests, examples, fixtures, the page, `crypto.json` and its generated files, the MSVC and message tests), the convenience header updated (D3b, owner decision 8)

- [ ] 2.5 Factories named by their input: every `make` and `async_make` renamed (`from_secret`, `from_bytes`, `from_coordinates`, `from_key`, `from_keys`, `from_keypair`), `make.hpp` to `factories.hpp`, no alias; every caller moved; a message test that `make` no longer exists (D3c, owner decision 9)

## 3. Concepts

- [ ] 3.1 `keypair`, `signing_keypair`, `agreement_keypair` and each curve's `static_assert`s; `crypto::sign`, `crypto::agree` and their asynchronous forms constrained; message tests (D4)
- [ ] 3.2 `<webcpp/crypto/secret_key_encryption/aead.hpp>`: `secret_key_encryption::aead::cipher` and `streaming_cipher`; `<webcpp/crypto/authenticated_box.hpp>`: `authenticated_box`; secretbox out of the AEAD template as `secret_key_encryption::basic_secretbox`, without associated data; each cipher's `static_assert`; the free `secret_key_encryption::aead::seal` and `open` constrained, and the box forms in `secret_key_encryption` and `public_key_encryption` (D5, R7, R21, owner decision 13)
- [ ] 3.3 `async_seal`, `async_open` and `async_from_key` for every AEAD and box on native backends (the free forms `secret_key_encryption::aead`'s, and the box forms `secret_key_encryption`'s and `public_key_encryption`'s), the browser stand-in's refusal (D6, R21)

## 4. The page, the generated files and the delivery

- [ ] 4.1 The guide's chapters (signatures, X25519, AEADs and `secret_key_encryption`), the section about protecting secrets outside the primitives (`constant_time_equal`, `erase_secret`, `secret<N>`, `random_device`; owner decision 11), `crypto.json` and its generated files, the changelog line
- [ ] 4.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end

## 5. Small fixes found along the way

- [ ] X1 In the browser a throwing `crypto.getRandomValues` gives `entropy_unavailable` to `random_device` and every caller of `detail::read_entropy`, never a terminated page; reproduced first under node (controller ruling)
- [ ] X1b libsodium's own randomness on Emscripten (its stir in `sodium_init`, the sealed box's ephemeral key) goes through crypto's entropy, so that a throwing `getRandomValues` is `entropy_unavailable` there too; reproduced first (controller ruling)
- [ ] X2 A public `crypto::erase_secret` in `<webcpp/crypto/erase_secret.hpp>` for any non-const contiguous sized range of trivially copyable elements, over `detail::wipe`; `crypto::equal` renamed `crypto::constant_time_equal` (`<webcpp/crypto/constant_time_equal.hpp>`), no alias, every caller moved, webcpp::hash's and the superproject's fixtures included; tests, message tests, the pages and the namespace table's rows (controller ruling, owner decision 11, D10)
- [ ] X3 No one-word local or parameter in a public header (MSVC C4459 at /W4 /WX): renamed, and a test whose globals carry every such name compiles warning-free on MSVC (controller ruling)
- [ ] X4 The AEAD contract in `crypto::secret_key_encryption::aead`: the free `seal`, `open`, `async_seal` and `async_open` moved there from `crypto::` (`<webcpp/crypto/secret_key_encryption/aead.hpp>`, included by every AEAD's header); `ciphers::secretbox` and `secretbox_xchacha20poly1305` moved to `crypto::secret_key_encryption` (`secret_key_encryption/secretbox.hpp`), no alias; public names spelled out (`nonce`, `nonce_bytes`, `associated_data_bytes`, `auxiliary_random_bytes`, `nonce_size`, `max_nonce_size`, `max_associated_data_size`, `error::invalid_nonce_size`) and the rule in CODING-GUIDELINES.md; every caller moved, the superproject's fixtures included; message tests for the removed names; webcpp::trystero and hash's HKDF names out of scope; runs after 2.1, before 2.2 (controller ruling, owner decision 13, D11, R22)
