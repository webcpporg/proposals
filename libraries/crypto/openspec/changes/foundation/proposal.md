## Why

Version 0.1 of webcpp::crypto gives a program AES-GCM, the AES block cipher, a `random_device`,
a constant-time `equal` and secp256k1, on four C libraries. That is what webcpp::trystero
needed, and it is far from what the programs above webcpp need. The research of 2026-11-14 read
the noble and scure stacks (the cryptography most JavaScript applications of the decentralised
web run on), and found most of their needs missing: Ed25519 and X25519 for every identity,
certificate and signed request; raw ECDH, public key recovery, tweaks and uncompressed keys on secp256k1 for Nostr, Ethereum and stealth
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

Two decisions revised it. On 2026-11-16 the owner retired webcpp::hash's profiles and measured
defaults (hash's design D33). On 2026-11-17, with version 0.1 merged, he decided that the
genesis's increments 3 to 5 are built without trystero's migration, ChaCha20-Poly1305 and Botan
included, beside this change in one order (`reference/roadmap.md`), that measured tables come
only from release builds, and that crypto's defaults follow hash's simple rule: one default per
algorithm chosen by a stated reason, every other backend named in the type, no profile
(`design.md` D32 to D35; the owner confirmed the details the same day, the roadmap's answers
Q1 to Q7).

## What Changes

- **Twelve increments, ordered by value** (`design.md` D20, D21, D34, D35; the order and the
  parallel streams in `reference/roadmap.md`):
  1. the libsodium backend's core (Ed25519, X25519, the Ed25519 to X25519 conversion,
     ChaCha20-Poly1305 IETF and original, XChaCha20-Poly1305, `crypto_box`, sealed boxes,
     `crypto_secretbox` in both forms, `crypto_kx`), ChaCha20-Poly1305 on BearSSL and OpenSSL
     (the genesis's increment 4), the capability table grown from version 0.1's, one default per
     family by a stated reason, the constant-time lane (the genesis's Task 20), the sizes lane
     and a throughput page from release builds, and the documentation's new form: version 0.2;
  2. libsecp256k1's remaining modules: recovery, raw ECDH, tweaks, uncompressed keys,
     ElligatorSwift, MuSig2, silent payments (beside 1, also in version 0.2);
  3. BearSSL's remaining parts: P-256, P-384 and P-521 ECDSA and ECDH, X25519, AES-CTR,
     AES-CBC, AES-CCM, AES-EAX, RSA; P-256 first and on its own, with a key pair made from a
     raw scalar and its export, ECDH's raw x coordinate and low S as two primitives, for
     webcpp::trystero's port of Trystero 0.27.0 (the owner's approval of 2026-11-20, design D36);
  4. the browser backend on Emscripten, WebCrypto's SubtleCrypto, through the asynchronous
     family `async_*` that takes a webcpp::asio completion token;
  5. libsodium completed: ristretto255 and edwards25519 groups, ML-KEM-768 and X-Wing, and the
     rest of libsodium that belongs to cryptography (secretstream, AEGIS, AES256-GCM, MACs,
     stream ciphers, memory utilities, padding, constant-time encodings, IP address encryption,
     guarded memory);
  6. blst: BLS12-381 signatures, aggregation and hash-to-curve;
  7. Botan 3 as a pinned submodule (the genesis's increment 5, its AEAD and block cipher cells
     first): Ed448, X448, brainpool, AES-SIV, AES-GCM-SIV, AES-KW,
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
  memory utilities, the configuration and the defaults; `crypto::backends` stays outside. Version
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
- **A capability table** (algorithm, backend, target), version 0.1's `doc/tables/crypto.json`
  grown, from which version 0.1's `doc/tables/generate.py` generates the refusals, the
  `supports` trait, the defaults and the documentation's tables. A backend without a function
  refuses at compile time, naming the backends that have it (D4, D33).
- **One default per family and target, by a stated reason** (D32, which supersedes the
  profiles of D5 and the measured defaults of D6): AES-GCM OpenSSL natively and BearSSL on
  WebAssembly, as version 0.1; libsodium for the ChaCha AEADs, Ed25519, X25519 and its other
  families; BearSSL for the P-curves and RSA; the only backend elsewhere. No profile, no
  webcpp-wide umbrella, no per-family override, and the link set changes nothing; every other
  backend is named in the type. Sizes and throughput are measured from release builds only and
  published as references that decide nothing.
- **New backends:** `backends::sodium` (libsodium 1.0.22, the `/third_party//sodium` that
  webcpp::hash builds), `backends::blst` (blst v0.3.17), `backends::botan` (Botan 3.14.1, now a
  pinned submodule built by b2, replacing the genesis's "found on the machine"), and
  `backends::browser` (WebCrypto, Emscripten only, asynchronous only). Each C library keeps
  chapter 5's three layers.
- **MODIFIED from the genesis:** the library is no longer synchronous only (a parallel
  asynchronous family exists, which the synchronous API never uses); the default backend is
  one per family and target, never changed by the link set; Botan is a submodule and may serve
  WebAssembly; the genesis's increments 4 (ChaCha20-Poly1305 on BearSSL and OpenSSL) and 5
  (Botan) are built in this change's increments 1 and 7, and its Tasks 20, 22 and 23 in 1 and
  4; HKDF comes from webcpp::hash. **BREAKING (version 0)**: `curves::secp256k1::basic_keypair::ecdh` returns
  `secret<32>`, which wipes itself, in place of `std::array<std::uint8_t, 32>` (D9).
- **Heavy operations on another thread, only with threads** (D23, the owner's decision, the
  same rule as webcpp::hash's): an operation of D32's stated list (RSA key generation and large
  private operations, SLH-DSA, ML-DSA key generation and signing, BLS verification and
  aggregation, KZG over a blob, BN254's pairing check) gets an
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
  from `doc/tables/crypto.json` and the lanes' tables from release builds by
  `doc/tables/generate.py`; prose in webcpp's own words in the manner of Wikipedia's best articles
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
  capability table, one default per family by a stated reason, new backends (libsodium, blst, Botan as a
  submodule, the browser) and an asynchronous family for the browser.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis: webcpp's interface over libsodium,
  libsecp256k1, BearSSL, OpenSSL, ctaes, Botan, blst and WebCrypto, none of them ported.
- **Implementation:** not started; its base is ready (version 0.1 merged, webcpp::hash's
  `/third_party//sodium` on the superproject's `main`). **The reference code is the code
  written in the tasks of increment 1 in `reference/plan.md`, read through its revision of
  2026-11-17:** Task 2 (`backends/sodium.hpp`, `src/canary_sodium.c`), Task 3
  (`secret.hpp`, the errors 15 to 20), Task 4' (`doc/tables/crypto.json` and `generate.py`
  grown, `detail/capabilities.hpp`, `supports.hpp`), Task 5' (the defaults by reason), Task 6 (`curves/ed25519.hpp`),
  Task 7 (`curves/x25519.hpp`), Task 8 (`ciphers/aead.hpp` and the three ChaCha AEADs), Task 9 (`hybrid/box.hpp`,
  `ciphers/secretbox.hpp`), Task 10 (`curves/x25519/kx.hpp`) and, from the genesis's plan, Task 35
  (ChaCha20-Poly1305 on BearSSL and OpenSSL, re-cut as Task 8b), Task 20 (the constant-time
  lane) and Task 22 (sizes, re-cut as Task 11'), with the tests and Jamfiles the other tasks give.
  Increments 2 to 12 are outlined there and written in full when each starts.
  `reference/roadmap.md` gives the order and the parallel streams; `reference/STATUS.md` says
  where things stand and how to start.
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
- **Status:** accepted on 2026-11-14 (the owner decided its scope and shape that day), revised
  on 2026-11-17 (D32 to D35, the roadmap); not built, no release.
- **Other proposals:** `libraries/crypto/openspec/changes/genesis` is modified as the
  MODIFIED and REMOVED requirements of `specs/crypto/spec.md` say; its version 0.1 is merged,
  its increment 3 is reduced to the memory option (trystero's migration dropped), and its
  increments 4 and 5 are built here. `libraries/hash` builds the shared `/third_party//sodium`,
  to which this change adds crypto's canary; neither library reads a webcpp-wide profile, and
  hash's increment 6 (Botan) waits for this change's increment 7, which provisions Botan.
  `organization` carries the rulebook this change extends (chapter 5's rules for Botan and
  blst, chapter 10's rows for `blst`, `botan` and `acvp`, chapter 17's entry).
