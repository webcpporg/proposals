# Tasks

Implemented after crypto/sign-unified merges and before crypto's formats. Each task writes its
tests first; the library builds at the end of every task, and every caller of a removed name moves
in the task that removes it. A plan with code goes in `reference/plan.md` when the proposal is
accepted.

Built (2026-11-23 and 2026-11-24) in the order X3, 2.5, 2.4, X2, 1.1 to 1.4, 2.1, X4, 2.2, 2.3, 3.1
to 3.3, X1, X1b, X5, 4.1, on the branches `crypto/uniform-types` of webcpporg/crypto (head
`e07d44a`), webcpporg/hash (`659e50d`, X2's callers, `main` merged into it) and the superproject
(`0ab0ace`), each task reviewed clean. Where the code differs from the design or the plan,
`design.md` says so under "As built".

## 1. Typed signatures

- [x] 1.1 The signature class template in `detail` and its five types (no distinct Ed25519ph type: owner decision 12), `from_bytes`, `bytes()`, `recovery_id()`; the scheme traits gain the signature type; message tests for a signature of another curve or scheme (design D1)
- [x] 1.2 Every `sign` returns its type and every `verify` and `recover` takes it, the four forms, the asynchronous completions; the span overloads removed; `normalize_low_s` and `is_low_s` on the ECDSA types; Ed25519ph's streaming signer retyped onto `ed25519::signature`; callers moved (D1, R1)
- [x] 1.3 `crypto::ecdsa_keccak256_recoverable` on secp256k1: Keccak-256 inside or `prehashed`, the typed recoverable signature, `recover`, no `verify`; go-ethereum's vectors, an EIP-191 example, the refusal without XKCP (owner decision 10, D8)
- [x] 1.4 Ed25519ph as the scheme `crypto::eddsa_prehashed` on Ed25519's key pair: `crypto::prehashed{digest}` of 64 bytes only, a message refused at compile time, `ed25519::signature`, `verify` member and free, asynchronous on native backends only; `curves::ed25519::ph` renamed `prehashed_signer`, no alias; RFC 8032's Ed25519ph vectors and the streaming signer equal to the scheme (owner decision 12, D9)

## 2. X25519 typed

- [x] 2.1 `x25519::public_key` and `x25519::basic_keypair<B>` with `from_secret`, `generate`, `public_key()`, `secret()`, `agree`, the asynchronous members and the free forms; the free `public_key(secret)` and `agree(secret, peer)` removed; RFC 7748's vectors (D2)
- [x] 2.2 `kx_keypair` on the typed key pair, `from_keypair`, `generate` and `derive_from_seed`, tested against `crypto_kx_seed_keypair` and against `from_keypair` on the same bytes; the boxes and the sealed box on typed keys (D3, R5, R6, owner decision 7)
- [x] 2.3 Ed25519's `to_x25519()` on the key pair and the public key, the free `crypto::to_x25519`; `x25519_secret()` and `to_x25519_public` removed (D3, R4)

- [x] 2.4 `crypto::public_key_encryption` and `crypto::key_exchange`: the boxes, the sealed boxes, `kx_keypair` and `session_keys` moved with their headers, `crypto::hybrid` removed with no alias; every caller moved (tests, examples, fixtures, the page, `crypto.json` and its generated files, the MSVC and message tests), the convenience header updated (D3b, owner decision 8)

- [x] 2.5 Factories named by their input: every `make` and `async_make` renamed (`from_secret`, `from_bytes`, `from_coordinates`, `from_key`, `from_keys`, `from_keypair`), `make.hpp` to `factories.hpp`, no alias; every caller moved; a message test that `make` no longer exists (D3c, owner decision 9)

## 3. Concepts

- [x] 3.1 `keypair`, `signing_keypair`, `agreement_keypair` and each curve's `static_assert`s; `crypto::sign`, `crypto::agree` and their asynchronous forms constrained; message tests (D4)
- [x] 3.2 `<webcpp/crypto/secret_key_encryption/aead.hpp>`: `secret_key_encryption::aead::cipher` and `streaming_cipher`, and the free `crypto::seal`/`open` moved there as `secret_key_encryption::aead::seal`/`open`, constrained; `<webcpp/crypto/secret_key_encryption/authenticated_box.hpp>`: `secret_key_encryption::authenticated_box` and the box forms `secret_key_encryption::seal`/`open`; `ciphers::secretbox` moved to `secret_key_encryption/secretbox.hpp` as `secret_key_encryption::basic_secretbox`, `secretbox`, `secretbox_xchacha20poly1305`, without associated data, no alias; each cipher's `static_assert`; every caller of the moved names migrated; message tests for the removed names (D5, R7, R21, R23, owner decision 13)
- [x] 3.3 `async_seal`, `async_open` and `async_from_key` for every AEAD and box on native backends: the free `crypto::async_seal`/`async_open` moved into `secret_key_encryption::aead` (callers migrated, a message test for the removed names), the box forms `secret_key_encryption`'s; the browser stand-in's refusal (D6, R21, R23)

## 4. The page, the generated files and the delivery

- [x] 4.1 The guide's chapters (signatures, X25519, AEADs and `secret_key_encryption`, with decision 14's names: `ciphers::basic_chacha20_poly1305`, secp256k1's spelled-out functions), the section about protecting secrets outside the primitives (`constant_time_equal`, `erase_secret`, `secret<N>`, `random_device`; owner decision 11), `crypto.json` and its generated files, the changelog line
- [ ] 4.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end (plan: Task 4.2)
  - [x] Native clang 836 passed, 0 failed; gcc-15 407/0; Emscripten 329/0 (both variants); wasip2 492/0; wasip3 474/0; `generate.py`, the doc tables and the doc build pass; X3's survey is empty (superproject `586b869`, crypto `e7dc82e`, hash `659e50d`)
  - [x] `crypto_jamfiles_test.py`: 168 passed and 1 failed (`heavy_without_threads_on_emscripten` still named `token`, X3's rename), the fixture fixed in fix round 1: 169 passed
  - [x] MSVC 14.3 x64, 14.5 x64 and 14.5 arm64: 461 passed each, `user_globals` passes; `async_aead` and `async_aead_no_exceptions` failed on C4459 (a test's global named after a library parameter), fixed in fix round 1 by renaming the test's globals; MSVC 14.5 x64 8/8 on the touched targets
  - [x] Linux lint `--full` CLEAN (1309 translation units), the macOS full lint clean; fix round 1's files CLEAN (1310)
  - [x] Constant-time lane 14/14 (memcheck, MemorySanitizer, dudect), `x25519_agree`'s |t| 1.74 to 2.49, within `crypto.json`'s range; fix round 1 adds the paths of `erase_secret`, X25519's key pair's wipe and kx's `from_keypair`: 14/14
  - [x] The whole-branch review's findings fixed (fix round 1: crypto `e7dc82e..e07d44a`, superproject `586b869..0ab0ace`): the browser's JavaScript array zeroed after a draw, the stack copies of libsodium's draws wiped, the guidelines' present-tense names, the removed names pinned, the changelog, the include boundary, the overview's namespaces
  - [ ] The fix round's scoped re-review
  - [ ] `origin/main` merged into the branches; the pull requests in webcpporg/crypto, webcpporg/hash, webcpporg/proposals and webcpporg/webcpp (the pointers of libs/crypto and libs/hash); merge commits; one CI at the end, green on GitHub

## 5. Small fixes found along the way

- [x] X1 In the browser a throwing `crypto.getRandomValues` gives `entropy_unavailable` to `random_device` and every caller of `detail::read_entropy`, never a terminated page; reproduced first under node (controller ruling)
- [x] X1b libsodium's own randomness on Emscripten (its stir in `sodium_init`, the sealed box's ephemeral key) goes through crypto's entropy, so that a throwing `getRandomValues` is `entropy_unavailable` there too; reproduced first (controller ruling)
- [x] X2 A public `crypto::erase_secret` in `<webcpp/crypto/erase_secret.hpp>` for any non-const contiguous sized range of trivially copyable elements, over `detail::wipe`; `crypto::equal` renamed `crypto::constant_time_equal` (`<webcpp/crypto/constant_time_equal.hpp>`), no alias, every caller moved, webcpp::hash's and the superproject's fixtures included; tests, message tests, the pages and the namespace table's rows (controller ruling, owner decision 11, D10)
- [x] X3 No one-word local or parameter in a public header (MSVC C4459 at /W4 /WX): renamed, and a test whose globals carry every such name compiles warning-free on MSVC (controller ruling)
- [x] X4 Public names spelled out, on the names that stay where they are (3.2 and 3.3 move the AEAD forms and secretbox with decision 13's names, R23): `iv`/`iv_bytes` -> `nonce`/`nonce_bytes` and `additional_data` -> `associated_data` on the existing members, free forms and engines; `nonce_size`, `max_nonce_size`, `max_associated_data_size`, `error::invalid_nonce_size` (value 2 kept); `aux_random` -> `auxiliary_random_bytes`, `ctx` -> `context` and any other abbreviation; every call site of those names, the superproject's fixtures included; the rule in CODING-GUIDELINES.md; webcpp::trystero and hash's HKDF names out of scope; runs after 2.1, before 2.2 (controller ruling, owner decision 13, D11, R22, R23)
- [x] X5 The remaining spelled-out names (owner decision 14, D12, R24, R25), after X1b and before 4.1: first commit, D6's refusal reworded to say no form of the constructions WebCrypto lacks runs on the browser's backend, its message tests and generated pages; then `ciphers::aead` -> `ciphers::basic_chacha20_poly1305` (aliases unchanged, header unchanged, the timing seam's member labels following); `musig::nonce_gen` -> `generate_nonce`; `pubkey_tweak_add`/`_mul`/`negate`/`combine` -> `public_key_*`; `seckey_tweak_add`/`_mul`/`negate` -> `secret_key_*`; silent payments' `prevouts_summary`/`make_prevouts_summary` -> `previous_outputs_summary`/`make_previous_outputs_summary`; no alias; every call site, doc, example, test, fixture and the superproject migrated; message tests for the removed names, `alone`; webcpp::trystero out of scope (controller ruling)

## Open

- [ ] webcpp::trystero's port of Trystero 0.27 moves to the new names by its own stream (decisions 11, 13 and 14): `crypto::equal` in `room_keys.hpp` (its include and line 231) and `doc/protocol.adoc`, and any use of `ed25519::ph`, `crypto::seal`/`open`, `ciphers::secretbox` or an `iv` parameter; none of X5's removed names on 2026-11-23.
- [ ] An Ed25519 backend other than libsodium (OpenSSL's `Ed25519ph` signs a finished digest through a public API): until one exists, `eddsa_prehashed` depends on libsodium's internal functions (design "As built", P27).
- [ ] If webcpp::hash, which shares libsodium, calls `sodium_init` before crypto's first call, libsodium's JavaScript stir runs first and can still throw on Emscripten; crypto cannot reach it.
- [ ] Natively, libsodium's `randombytes_sysrandom` still aborts on a failed generator; X1b changes Emscripten only.
- [ ] R24 (`prevouts_summary` read as an abbreviation of decision 14) waits for the owner's confirmation.
