## Why

Version 0.1 of webcpp::crypto gives a program AES-GCM, the AES block cipher, a `random_device`,
a constant-time `equal` and secp256k1, on four C libraries. That is what webcpp::trystero
needed, and it is far from what the programs above webcpp need. The research of 2026-11-14 read
the noble and scure stacks (the cryptography most JavaScript applications of the decentralised
web run on) and a TypeScript wallet and identity stack built on them, and found most of their
needs missing: Ed25519 and X25519 for every identity, certificate and signed request; raw ECDH,
public key recovery, tweaks and uncompressed keys on secp256k1 for Nostr, Ethereum and stealth
addresses; P-256 for passkeys and WebCrypto; ChaCha20-Poly1305 and XChaCha20-Poly1305 for
modern protocols; AES Key Wrap; BLS12-381 for Ethereum's consensus; ML-KEM and X-Wing for the
move to post-quantum keys.

On 2026-11-14 the owner decided that webcpp::crypto and webcpp::hash (proposed the same day, in
`libraries/hash/openspec/changes/genesis/`) become the complete cryptographic foundation of
webcpp: every backend exposes everything its library offers that belongs to cryptography,
libsodium becomes the next backend, a browser backend on Emscripten speaks to WebCrypto through
a parallel asynchronous interface, one setting chooses every default backend at once from a
table of webcpp's own measurements, and the documentation teaches as noble's does. He accepted
this change the same day. It is version 0.2 onward, a series of increments after version 0.1.

## What Changes

- **Eight increments, ordered by value** (`design.md` D20, D21):
  1. the libsodium backend's core (Ed25519, X25519, the Ed25519 to X25519 conversion,
     ChaCha20-Poly1305 IETF and original, XChaCha20-Poly1305, `crypto_box`, sealed boxes,
     `crypto_secretbox` in both forms, `crypto_kx`), the profile configuration, the capability
     table, the measurement table, the constant-time lane for the new code, and the
     documentation's new form: version 0.2;
  2. libsecp256k1's remaining modules: recovery, raw ECDH, tweaks, uncompressed keys,
     ElligatorSwift, MuSig2, silent payments (beside 1, also in version 0.2);
  3. BearSSL's remaining parts: P-256, P-384 and P-521 ECDSA and ECDH, X25519,
     ChaCha20-Poly1305, AES-CTR, AES-CBC, AES-CCM, AES-EAX, RSA;
  4. the browser backend on Emscripten, WebCrypto's SubtleCrypto, through the asynchronous
     family `async_*` that takes a webcpp::asio completion token;
  5. libsodium completed: ristretto255 and edwards25519 groups, ML-KEM-768 and X-Wing, and the
     rest of libsodium that belongs to cryptography (secretstream, AEGIS, AES256-GCM, MACs,
     stream ciphers, memory utilities, padding, constant-time encodings, IP address encryption,
     guarded memory);
  6. blst: BLS12-381 signatures, aggregation and hash-to-curve;
  7. Botan 3 as a pinned submodule: Ed448, X448, brainpool, AES-SIV, AES-GCM-SIV, AES-KW,
     CMAC, ML-KEM, ML-DSA, SLH-DSA and the long tail;
  8. OpenSSL's remaining algorithms on native, wasip2 and wasip3;
  9. KZG polynomial commitments over c-kzg-4844, on blst;
  10. the BN254 pairing over mcl, for pairing-based verification;
  11. Shamir secret sharing over dsprenkels/sss, constant time;
  12. HPKE (RFC 9180) over OpenSSL natively and on WASI, and Botan on Emscripten.
  Increment 2 runs beside 1; after 1, increments 3 to 8, 10 and 11 run as parallel streams; 9
  follows 6, and 12 follows 8 (its WebAssembly cells 7).
- **Groups, and headers that mirror them** (D29, the owner's decision of 2026-11-15):
  `crypto::ciphers`, `crypto::curves::<curve>`, `crypto::post_quantum`, `crypto::rsa`,
  `crypto::hybrid`, `crypto::commitments::kzg`, `crypto::secret_sharing::shamir`, headers
  `<webcpp/crypto/<group>/<name>.hpp>`; the root keeps `error`, `random_device`, `equal`, the
  memory utilities, the configuration and the profiles; `crypto::backends` stays outside. Version
  0.1 is renamed to this layout before it closes, and the foundation starts from it.
- **Standard key formats** (D30, the owner's decision of 2026-11-15 from the research on libp2p
  and iroh): each curve's increment carries SPKI (public) and PKCS#8 (private) in DER and their
  PEM armour, import and export, decoded strictly (an encoding that does not re-encode to the
  same bytes is refused): Ed25519 and X25519 (RFC 8410) in increment 1, secp256k1 (RFC 5480)
  in increment 2, P-256 (RFC 5480, RFC 5915) in increment 3, RSA (PKCS #1 inside SPKI and
  PKCS#8) with RSA. libp2p's PeerIds for ECDSA and RSA keys are SPKI DER, WebCrypto imports and
  exports `spki` and `pkcs8`, and OpenSSL and SSH tooling exchange PEM; JWK stays above crypto.
- **Ed25519's verification policy** (D31): strict by default (libsodium's rules), named
  `curves::ed25519::strict`; the twelve edge cases of "Taming the many EdDSAs" run against every
  Ed25519 backend, their verdicts committed and published as a table; `zip215` is declared only
  when a mature C or C++ implementation of ZIP-215 is found, an open research item, since Rust
  peers verifying with ed25519-dalek's default rules accept signatures libsodium refuses.
- **Primitives only** (D24, the owner's rule): whatever BIPs, EIPs, SLIPs and NIPs define goes to
  future domain libraries (webcpp::bitcoin, webcpp::ethereum, webcpp::nostr) built on crypto and
  hash; libsecp256k1's own modules stay as the primitives they are. Version 0.1's Taproot tweak
  becomes a generic x-only tweak and its `from_lax_der` and `bitcoin_sign` example move to
  webcpp::bitcoin, a change of the genesis that increment 2 must not undo.
- **Every backend exposes everything its library offers that belongs to crypto.** Hashing,
  XOFs, HMAC, key derivation and password hashing are webcpp::hash's (D1); webcpp::crypto
  depends on webcpp::hash.
- **A capability table** (algorithm, backend, target), committed as `meta/capabilities.json`,
  from which the refusals, the `supports` trait and the documentation's table are generated. A
  backend without a function refuses at compile time, naming the backends that have it (D4).
- **One define chooses every default backend:** macros tested with `#ifdef`, at most one per
  level: crypto's `WEBCPP_CRYPTO_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE` or
  `_BACKEND_<NAME>`; the webcpp-wide umbrella `WEBCPP_PROFILE_FASTEST`, `_SMALLEST`,
  `_PORTABLE`, which crypto and hash both read; per-family overrides
  `WEBCPP_CRYPTO_DEFAULT_BACKEND_<FAMILY>_<NAME>`. The precedence is override, crypto's profile,
  the umbrella, then the build's measured default (the owner's rule: natively the fastest, on
  WebAssembly the smallest), from the target, the linked `HAS` macros and
  `meta/measurements.json`, webcpp's own measurements of size per target and throughput
  natively, regenerated at every pin move (D5, D6).
- **New backends:** `backends::sodium` (libsodium 1.0.22, the `/third_party//sodium` that
  webcpp::hash builds), `backends::blst` (blst v0.3.17), `backends::botan` (Botan 3.14.1, now a
  pinned submodule built by b2, replacing the genesis's "found on the machine"), and
  `backends::browser` (WebCrypto, Emscripten only, asynchronous only). Each C library keeps
  chapter 5's three layers.
- **MODIFIED from the genesis:** the library is no longer synchronous only (a parallel
  asynchronous family exists, which the synchronous API never uses); the default backend is
  chosen per algorithm by the profile and the measurements, the architecture included where the
  measurements differ; Botan is a submodule and may serve WebAssembly; the genesis's increments
  4 (ChaCha20-Poly1305) and 5 (Botan) are absorbed by this change's 1, 3, 7 and 8; HKDF comes
  from webcpp::hash. **BREAKING (version 0)**: `curves::secp256k1::basic_keypair::ecdh` returns
  `secret<32>`, which wipes itself, in place of `std::array<std::uint8_t, 32>` (D9).
- **Heavy operations on another thread, only with threads** (D23, the owner's decision, the
  same rule as webcpp::hash's): an operation measured above 1 ms on the reference runner (RSA
  key generation, SLH-DSA, BLS verification and aggregation, ML-DSA where it is slow) gets an
  `async_` variant that runs on a `webcpp::asio::thread_pool` natively with `threading=multi`,
  or on Web Workers on Emscripten with pthreads; without threads it does not compile, and cheap
  operations stay synchronous.
- **Tests per backend** with official vectors (Wycheproof where it covers, RFCs, NIST ACVP,
  BIP vectors for libsecp256k1's modules), the C libraries' own suites, cross-backend agreement natively and on
  WebAssembly, the browser as the outside oracle for everything it offers, and constant-time
  lanes for the new code.
- **Documentation** (D19, the owner's requirements of 2026-11-15): written for a newcomer and
  for an LLM choosing from it alone; a chapter per namespace opening with a comparison table
  (characteristics, use cases, where used with sources, compatibility, "choose it when"); every
  API page's backends, a backends chapter's matrices (operations, targets, sizes, throughput,
  constant time), a "choose by use case" decision table and an `llms.txt` index, all generated
  from `meta/capabilities.json`, `meta/use_cases.json` and `meta/measurements.json` by
  `tools/crypto/docs.py`; prose in webcpp's own words in the manner of Wikipedia's best articles
  (cited, never copied), libsodium's documentation adapted under ISC, noble's per-algorithm
  form; teaching chapters on crypto wallets, peer-to-peer protocols, passwords and everyday
  apps.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: the library grows from authenticated encryption, randomness and secp256k1 into the
  complete set of cryptographic primitives of its backends: signatures, key agreement, key
  encapsulation, AEADs, boxes, stream ciphers, MACs, groups and memory utilities, with a
  capability table, profiles, measured defaults, new backends (libsodium, blst, Botan as a
  submodule, the browser) and an asynchronous family for the browser.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis: webcpp's interface over libsodium,
  libsecp256k1, BearSSL, OpenSSL, ctaes, Botan, blst and WebCrypto, none of them ported.
- **Implementation:** not started. It begins after version 0.1 has merged (the genesis's
  groups 1 and 2) and after webcpp::hash's `/third_party//sodium` is on the superproject's
  `main`. **The reference code is the code written in the tasks of increment 1 in
  `reference/plan.md`:** Task 2 (`backends/sodium.hpp`, `src/canary_sodium.c`), Task 3
  (`secret.hpp`, the errors 15 to 20), Task 4 (`meta/capabilities.json`,
  `tools/crypto/capabilities.py`, `detail/capabilities.hpp`, `supports.hpp`), Task 5 (the
  profiles in `config.hpp`, `detail/select.hpp`, `detail/rankings.hpp`), Task 6 (`curves/ed25519.hpp`),
  Task 7 (`curves/x25519.hpp`), Task 8 (`ciphers/aead.hpp` and the three ChaCha AEADs), Task 9 (`hybrid/box.hpp`,
  `ciphers/secretbox.hpp`), Task 10 (`curves/x25519/kx.hpp`) and Task 11 (`tools/crypto/measure.py`), with the tests
  and Jamfiles the other tasks give. Increments 2 to 8 are outlined there and written in full
  when each starts. `reference/STATUS.md` says where things stand and how to start.
- **Dependencies:** webcpp::hash (`/webcpp/hash//hash`, its `/third_party//sodium`); libsodium
  1.0.22 at `77e1ce5d6dee871c49ef211222ba18ef0c486bda` (ISC); libsodium's documentation at
  `jedisct1/libsodium-doc` `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56` (ISC); blst v0.3.17 at
  `54e6e55674722fc2797ebb4bbb71b26d881eb4b8` (Apache-2.0); Botan 3.14.1 at
  `01af88c039fcfaedc418b1af4d6d3a139674e591` (BSD-2-Clause); c-kzg-4844 v2.1.8 at
  `e125905e5e01186e6ccb7a0ced4845bf7eddbcfe` (Apache-2.0); mcl v4.43 at
  `50e4c6f2826de6b041f857caf9f253cc736ea926` (BSD-3-Clause); dsprenkels/sss at
  `9f2a10b1c0d391bed485c4c7e93788ce37c360f1` (MIT); the genesis's BearSSL, OpenSSL
  3.5.9, libsecp256k1 v0.8.0 and ctaes pins; webcpp::asio on Emscripten for the asynchronous
  family; for tests, Wycheproof, NIST ACVP's JSON vectors, BIP vectors, the consensus specifications' BLS vectors, node and
  Chrome.
- **Status:** accepted on 2026-11-14 (the owner decided its scope and shape that day); not
  built, no release.
- **Other proposals:** `libraries/crypto/openspec/changes/genesis` is modified as the
  MODIFIED and REMOVED requirements of `specs/crypto/spec.md` say, its version 0.1 and
  increment 3 (trystero's migration) unchanged. `libraries/hash` builds the shared
  `/third_party//sodium`, to which this change adds crypto's canary, and reads the same
  webcpp-wide umbrella `WEBCPP_PROFILE_*`, so that one define covers both libraries.
  `organization` carries the rulebook this change extends (chapter 5's rules for Botan and
  blst, chapter 10's rows for `blst`, `botan` and `acvp`, chapter 17's entry).
