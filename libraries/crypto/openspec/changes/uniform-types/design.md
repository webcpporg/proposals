## Context

This change builds on crypto/sign-unified as merged: typed public keys (`ed25519::public_key`,
`p256::basic_public_key<B>`, `secp256k1::public_key`), the concept `crypto::keypair` in
`<webcpp/crypto/keypair.hpp>` (`make`, `generate`, `public_key()`, `secret()` and `sign` for every
scheme of `detail::all_schemes` it declares), `agree(peer)` on P-256 and secp256k1, scheme tags as
values, the backend first among template parameters, and four mirrored forms per operation (its
D9: member and free, synchronous and asynchronous, one implementation, the asynchronous members
defined in `async.hpp`). Its rule for a signature is a span: every `sign` returns
`std::array<std::uint8_t, N>` and every `verify` and `recover` takes `std::span<std::uint8_t const>`,
checking its size first.

What is not typed on that branch:

| Value | Today |
|---|---|
| Ed25519 signature | `std::array<std::uint8_t, 64>` |
| P-256 and secp256k1 ECDSA signatures | `std::array<std::uint8_t, 64>` |
| secp256k1 recoverable and BIP340 signatures | `std::array<std::uint8_t, 65>`, `std::array<std::uint8_t, 64>` |
| X25519 public key | `std::array<std::uint8_t, 32>` from the free `curves::x25519::public_key(secret)` |
| X25519 secret key | `secret<32>`; agreement by the free `curves::x25519::agree(secret, peer)` |
| crypto_kx | `kx_keypair<B>::make(seed)`, peers as 32-byte spans |
| Ed25519 to X25519 | `basic_keypair::x25519_secret()` (`secret<32>`), free `to_x25519_public(public_key)` (an array) |
| crypto_box, sealed box | made from secret and public key bytes |
| secretbox | `ciphers::aead<families::secretbox_*, B>`, whose `seal` takes additional data it refuses unless empty |

The AEADs are `ciphers::gcm<Cipher>` (AES-GCM, with an incremental seal and open: `begin_seal`,
`update`, `result`, `begin_open`, `authenticate`, `decrypt`, `verify`, `end_open`) and
`ciphers::aead<Family, B>` (ChaCha20-Poly1305 in three nonce sizes); the asynchronous family has
`async_seal` and `async_open` for AES-GCM only, by the foundation's rule that no `async_` variant
of a cheap algorithm WebCrypto lacks exists.

## Goals / Non-Goals

**Goals:** a type for every signature of every curve and scheme, returned by `sign` and taken by
`verify` and `recover`; X25519's keys typed as the other curves' are, with crypto_kx, the boxes and
Ed25519's conversion on them; three key pair concepts that every key pair meets according to what
it does; concepts for AEADs, AES-GCM's incremental form and the boxes, constraining the free and
asynchronous forms; asynchronous seal and open for every AEAD and box on native backends; every
caller moved, the old names removed with no alias.

**Non-goals:** key and signature formats (the formats change, which builds on this one); sealed
boxes in a concept (their shape, a recipient's key per call, is neither an AEAD's nor a box's);
new algorithms; the browser backend itself (part B of the asynchronous family).

## Owner decisions (2026-11-22, binding)

1. **Typed signatures per curve and scheme**, returned by `sign` on every curve and taken by
   `verify` and `recover`, replacing crypto/sign-unified's span rule, each with `bytes()` and
   `from_bytes(bytes)` (named `make` when decided, renamed by decision 9); whether `verify` keeps a bytes overload is ruled here (R1).
2. **X25519 typed like the others**: `x25519::public_key` (`from_bytes`, `bytes`),
   `x25519::basic_keypair<B>` (`from_secret`, `generate`, `public_key()`, `secret()`, `agree(peer)` with
   `async_agree` and the free `crypto::agree`), `kx_keypair` built on it; Ed25519's
   `x25519_secret()` and `to_x25519_public` become a typed conversion (`ed_key.to_x25519()` giving
   an `x25519::keypair`, `ed_pub.to_x25519()` an `x25519::public_key`); the old functions removed.
3. **Key pair concepts split**: `keypair` (`from_secret`, `generate`, `public_key()`, `secret()`),
   `signing_keypair` (also `sign` for each declared scheme, at least one), `agreement_keypair`
   (also `agree`). Ed25519 signs; P-256 and secp256k1 do both; X25519 agrees.
4. **`concept aead`** (`key_size`, `iv_size`, `tag_size`, `result_type`, `static from_key(key)`
   returning `result<C>`, `seal(iv, ad, data)` returning `result<result_type>`,
   `open(iv, ad, data, tag)` returning `result<void>`), with a `static_assert` per cipher; the
   refinement **`streaming_aead`** (AES-GCM's `begin_seal`, `update`, `result`, `begin_open`,
   `authenticate`, `decrypt`, `verify`, `end_open`); option (a) for the boxes: box and secretbox
   get the sibling concept **`authenticated_box`** (`seal(iv, data)`, `open(iv, data, tag)`), with
   no fake additional data. `crypto::seal`, `crypto::open` and the asynchronous forms are
   constrained by these concepts; `async_seal` and `async_open` extend to any AEAD on native
   backends, the browser staying WebCrypto-only and refusing with a clear message.
5. **Four mirrored forms apply to everything new.**

Second round, the same day, answering the first draft's rulings:

6. **`verify` takes only the typed signature**, with no bytes overload; a signature from the wire
   goes through `from_bytes` first (R1 confirmed).
7. **libsodium's seeded kx derivation keeps a distinct name**, option (b):
   `key_exchange::kx_keypair::derive_from_seed(seed)` (`crypto_kx_seed_keypair`: the secret is
   BLAKE2b-256 of the seed), beside `from_keypair(x25519_keypair)` and `generate()`; the factory
   named by its input says the bytes or the key are the secret (R5 replaced; names per decision 9).

Third round, the same day:

8. **`crypto::hybrid` is removed as a namespace; constructions are grouped by purpose**:
   `crypto::public_key_encryption::{basic_box, box, box_xchacha20poly1305, basic_sealed_box,
   sealed_box}` and `crypto::key_exchange::{kx_keypair, session_keys}` (out of `curves::x25519`),
   with no alias for the old names; kx's `derive_from_seed` is unchanged. The reason: "hybrid
   encryption" (a KEM with a DEM) does not describe crypto_kx, and "hybrid" now means post-quantum
   hybrids (X25519MLKEM768, X-Wing), so `crypto::hybrid` stays free for those. `curves` holds curve
   primitives only, `ciphers` symmetric ciphers, and the two new namespaces the composed
   constructions, named by the problem they solve; a future HPKE (RFC 9180) lives in
   `public_key_encryption`.

Fourth round, the same day:

9. **The fallible factory `make` is renamed by what it takes**, with no alias: a key pair's
   `K::from_secret(secret_bytes)` (and `K::generate()`, unchanged); a public key's and a
   signature's `T::from_bytes(bytes)`; a cipher's, an AEAD's and a secretbox's
   `C::from_key(key)`; and, by this change's rulings, P-256's public key from raw coordinates
   `from_coordinates(x, y)`, crypto_box's `from_keys(own_keypair, peer_public_key)`, and kx's
   `from_keypair(x25519_keypair)` beside `derive_from_seed(seed)` and `generate()`. The concept
   `keypair` requires `from_secret` and `generate`. The four forms follow: the free
   `crypto::from_secret<K>(bytes)`, `crypto::from_bytes<T>(bytes)`, `crypto::from_key<C>(key)` and
   `crypto::generate<K>()`; the asynchronous `async_from_secret`, `async_from_key` and
   `async_generate`, member and free. The reason: `make`, a fallible constructor in the style of
   `std::make_*`, reads as "make a key", and a caller who passes fixed bytes believing a key is
   drawn gets a predictable private key; the name of the input makes the call site say where the
   secret comes from. The backends' internal capabilities keep their names (`*_import`,
   `*_generate`).

Fifth round, 2026-11-23:

10. **Ethereum's recoverable ECDSA is a scheme of this change**, `crypto::ecdsa_keccak256_recoverable`,
    on secp256k1 alone: the input a message, hashed inside with Keccak-256 (the pre-NIST padding
    Ethereum uses, never SHA3-256), or `crypto::prehashed{digest}` of 32 bytes for a digest the
    program builds (EIP-191's `personal_sign`, EIP-712, a transaction's sighash: protocol work,
    outside crypto); the output the typed `secp256k1::recoverable_signature` (r, s, the recovery id
    0 to 3, always low S as EIP-2 requires); `recover(ecdsa_keccak256_recoverable, signature, input)`
    gives the signer's `secp256k1::public_key`. Ethereum's `v` (27 or 28, or EIP-155's chain-id
    form) and the address are protocol encodings: the guide shows them, crypto computes neither (D8).

Sixth round, 2026-11-23 (the owner's reason for both: readability):

11. **The secret-hygiene utilities are named by what they do** (D10): the public wipe of the
    controller's Task X2 is `crypto::erase_secret` in `<webcpp/crypto/erase_secret.hpp>` (never
    `crypto::wipe`), with the plan's contract unchanged (a contiguous sized range of non-const
    trivially copyable elements, `noexcept`, no pointer and size overload, a `static_assert` whose
    message names the ranges it accepts), `detail::wipe` staying the internal primitive; and
    `crypto::equal`, both overloads (spans, and `boost::hash2::digest<N>`), is renamed
    `crypto::constant_time_equal` in `<webcpp/crypto/constant_time_equal.hpp>`, with no alias, every
    caller moved (tests, examples, the page, `crypto.json`, the constant-time lane's paths and
    suppressions, the message and MSVC tests, the superproject's fixtures, and other libraries'
    callers such as webcpp::hash's, through the superproject's pointers). webcpp::trystero's
    port of Trystero 0.27 is not touched; its call sites are listed in the stream's handback.
    The guide gains one section about protecting secrets outside the primitives (memory and
    timing hygiene) grouping `constant_time_equal`, `erase_secret`, `secret<N>` and
    `random_device`; no new namespace: they stay at the top of `webcpp::crypto`.
12. **Ed25519ph (RFC 8032) is a scheme, `crypto::eddsa_prehashed`**, in the one list of schemes,
    which Ed25519's signing key pair declares (D9): its input is `crypto::prehashed{digest}` of 64
    bytes only, the message's SHA-512 as RFC 8032's Ed25519ph defines it, a message being refused
    at compile time with a clear message (the opposite of `eddsa`, which refuses a digest); its
    result `ed25519::signature`, the same 64-byte type, an `eddsa` and an `eddsa_prehashed`
    signature not being interchangeable (the page says so); `verify(eddsa_prehashed, signature,
    prehashed, public_key)` and the member form; the asynchronous forms on native backends only,
    refused on the browser's backend (WebCrypto has no Ed25519ph). The streaming signer
    `curves::ed25519::ph<Hash, B>` is renamed `curves::ed25519::prehashed_signer<Hash, B>`
    (`begin`, `update`, `sign`, `verify` unchanged), with no alias, its callers moved. RFC 8032's
    Ed25519ph vectors stay, and a test checks that the streaming signer and
    `sign(eddsa_prehashed, prehashed{sha512(message)})` give the same signature. Ruling R14 (a
    distinct `ph_signature`) is superseded: no such type exists.

Seventh round, 2026-11-23 (the owner's reason: names that say what a thing is, read without a
glossary):

13. **The AEAD contract has a namespace of its own, secretbox joins it, and public names are spelled
    out** (D11):
    - the AEAD contract lives in namespace `webcpp::crypto::secret_key_encryption::aead`, in
      `<webcpp/crypto/secret_key_encryption/aead.hpp>`: the concept decision 4 called
      `crypto::aead` is `secret_key_encryption::aead::cipher`, its refinement `streaming_aead` is
      `secret_key_encryption::aead::streaming_cipher`, and the free `seal`, `open`, `async_seal`
      and `async_open` move there from `crypto::`, where no free AEAD `seal` or `open` remains; the
      members (`cipher.seal`, `open`, `async_seal`, `async_open`) are unchanged;
    - the algorithm types stay in `crypto::ciphers` with their names: `gcm<aes128>`,
      `gcm<aes256>`, `chacha20_poly1305`, `xchacha20_poly1305`, `aes128`, `aes256`, and later
      `chacha20`;
    - secretbox moves from `crypto::ciphers` to `crypto::secret_key_encryption`, parallel to
      `public_key_encryption::box`: `secret_key_encryption::secretbox` and
      `secretbox_xchacha20poly1305` in `<webcpp/crypto/secret_key_encryption/secretbox.hpp>`, with
      no alias, the library being before 1.0;
    - parameter names are spelled out everywhere in crypto's public API: `iv` and `iv_bytes`
      become `nonce` and `nonce_bytes`, `additional_data` becomes `associated_data`, `ctx` becomes
      `context`, and any other abbreviation in a public parameter name gets its full name; only
      algorithm proper names stay acronyms (`aes`, `gcm`, `hkdf`, `hmac`, `ecdsa`, `sha256`, ...).
      CODING-GUIDELINES.md's crypto and hash rules gain the rule: public names spelled out,
      algorithm proper names excepted;
    - every call site moves: crypto's code, pages, examples and tests, and the superproject's
      fixtures. Out of scope: webcpp::trystero's port of Trystero 0.27, not in the tree, which
      migrates later; webcpp::hash's HKDF names (`ikm`, `prk`, `info`), which a change of hash's
      renames.
    Decision 4's names (`crypto::aead`, `crypto::streaming_aead`, `crypto::seal`, `crypto::open`,
    `iv`, `ad`) are superseded by these; D5, D6, D11, R7, R21 and R22 give the result.
    The controller's ruling on how it is built (2026-11-23, R23): 3.2 and 3.3 are written with
    decision 13's names; X4 covers the renames they do not touch.

Eighth round, 2026-11-23 (the owner's reason: `ciphers::aead` reads like the namespace
`secret_key_encryption::aead`, and libsecp256k1's abbreviations are the glossary decision 13
removed):

14. **The one-shot AEAD template is named for its algorithm, and the names crypto took from
    libsecp256k1 are spelled out** (D12):
    - the class template `ciphers::aead<Family, Backend>` is
      `ciphers::basic_chacha20_poly1305<Family, Backend>`, the pattern of
      `public_key_encryption::basic_box` and `secret_key_encryption::basic_secretbox`; the aliases
      `chacha20_poly1305`, `chacha20_poly1305_original` and `xchacha20_poly1305` keep their names;
    - secp256k1's MuSig2 `musig::nonce_gen` is `musig::generate_nonce`; the key tweaks
      `pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate` and `pubkey_combine` are
      `public_key_tweak_add`, `public_key_tweak_mul`, `public_key_negate` and
      `public_key_combine`; `seckey_tweak_add`, `seckey_tweak_mul` and `seckey_negate` are
      `secret_key_tweak_add`, `secret_key_tweak_mul` and `secret_key_negate`;
    - any other public abbreviation of the same kind in crypto's public headers is spelled out
      (R24 lists the one found); algorithm proper names stay (`musig`, `ellswift`, `ecdh`,
      `xonly`, ...), and libsecp256k1's own C names (`secp256k1_ec_pubkey_*`), which crypto calls,
      are not crypto's public names;
    - secp256k1's `agree`, `agree_hashed` and `agree_point` stay as they are;
    - no compatibility alias; every call site moves (crypto's code, pages, examples, tests,
      fixtures, the constant-time lane, the benches and the superproject's fixtures);
      webcpp::trystero's port of Trystero 0.27 is out of scope, listed by the HANDBACK.
    The controller's ruling on how it is built (2026-11-23): Task X5 "remaining spelled-out
    names" runs after X1b and before 4.1, and its first commit rewords D6's refusal (D6), which
    3.3's fix round made fire from the synchronous forms too.

## Approaches

**Signatures.** (a) Keep bytes and add a scheme check at run time: a P-256 signature given to
secp256k1's `verify` still compiles. (b) One generic `crypto::signature<Scheme, N>`: `ecdsa_sha256`
is the scheme of two curves, so the type would not know its curve, which DER needs (P-256's
encoder refuses r or s not below its n). (c) One type per curve and scheme (chosen): the type says
the curve and the scheme, so `crypto::verify` refuses a mismatch at compile time and the formats
change can gate `der` by type.

**Boxes.** (a) A sibling concept `authenticated_box` without additional data (chosen by the
owner); (b) keep secretbox an `aead` whose additional data must be empty: a parameter that exists
only to be refused; (c) give boxes additional data by hashing it into the nonce: an invented
construction, not libsodium's.

## Decisions

### D1. Typed signatures

| Type | Bytes | `sign` of | `verify` / `recover` under |
|---|---|---|---|
| `curves::ed25519::signature` | 64 | `ed25519::basic_keypair<B>::sign(eddsa, ...)`, `sign(eddsa_prehashed, ...)` and `prehashed_signer::sign` (D9) | `eddsa`, `eddsa_prehashed` (not interchangeable: each verifies its own scheme's) |
| `curves::p256::ecdsa_signature` | 64, r then s | `p256::basic_keypair<B>::sign(ecdsa_sha256, ...)` | `ecdsa_sha256` |
| `curves::secp256k1::ecdsa_signature` | 64, r then s | `secp256k1::basic_keypair<B>::sign(ecdsa_sha256, ...)` | `ecdsa_sha256` |
| `curves::secp256k1::recoverable_signature` | 65, r, s, the id | `sign(ecdsa_sha256_recoverable, ...)`, `sign(ecdsa_keccak256_recoverable, ...)` (D8) | `recover(...)` under the same scheme |
| `curves::secp256k1::schnorr_signature` | 64 | `sign(schnorr, ...)` | `schnorr` |

Each is a regular value type (copyable, comparable with `==`; a signature is public), implemented
by one class template in `detail` with a distinct tag per type, so that no two convert into each
other. Each has `static constexpr std::size_t size`; `static from_bytes(std::span<std::uint8_t const>)`
returning `result<T>`, `invalid_signature` for another size and nothing else checked (whether r and
s are below n, or R canonical, is verification's judgment, as crypto/sign-unified's R16 rules for
Ed25519's public key); `bytes()` returning `std::array<std::uint8_t, size> const&`; and nothing
that converts implicitly. `recoverable_signature` adds `recovery_id()` (0 to 3). The free
`crypto::from_bytes<T>(bytes)` (D3c) serves them.

`sign` returns `result<T>` on every key pair and scheme, the asynchronous `sign` completes with
`void(error_code, T)`; `crypto::verify(scheme, signature, input, public_key)`, `pub.verify`,
their asynchronous forms and `recover` take `T`. The trait that pairs a scheme with its key's types
(`detail::signs_with_v`, crypto/sign-unified's D5) gains the signature type, so `can_verify` is
false for a signature of another curve or scheme, with a message naming the type it expects.
`normalize_low_s` and `is_low_s` take and return the curve's `ecdsa_signature`.

### D2. X25519's types

- `curves::x25519::public_key`: `static from_bytes(std::span<std::uint8_t const>)` returning
  `result<public_key>`, `invalid_public_key` for another size than 32 and nothing else checked
  (a small-order key is refused where it matters, by `agree`, as `weak_public_key`); `bytes()`.
- `curves::x25519::basic_keypair<Backend>` (and `x25519::keypair`, its default): `public_key_type`,
  `secret_key_size` 32; `static from_secret(secret_bytes)` (RFC 7748's 32-byte scalar, clamped by the
  backend as today) and `static generate()`, returning `result<basic_keypair>`; `public_key()`
  returning `result<public_key>`; `secret()` returning `result<secret<32>>`; `agree(peer)`, `peer`
  a `public_key`, returning `result<secret<32>>` and `weak_public_key` when the shared secret is
  all zero, on every backend, as the foundation requires; moved from, every call returns
  `invalid_state`; its state wiped on destruction and move, as the other key pairs are.
- The four forms: `from_secret`, `generate`, `agree` as members; `crypto::from_secret<K>`, `crypto::generate<K>`,
  `crypto::agree(key, peer)` free (crypto/sign-unified's generic ones, now constrained by D3);
  `K::async_from_secret`, `K::async_generate`, `key.async_agree(executor, peer, token)` and the free
  `async_from_secret`, `async_generate`, `async_agree`, defined in `async.hpp`. X25519 is in WebCrypto, so
  the asynchronous forms exist on every backend, the browser's included once part B lands.
- Removed: the free `curves::x25519::public_key(secret)` (its name is the type's now) and
  `curves::x25519::agree(secret, peer)`; `curves::x25519::algorithm<B>` moves into `detail`, its
  family `families::x25519` unchanged.

### D3. crypto_kx, the boxes and Ed25519's conversion on typed keys

- `key_exchange::kx_keypair<Backend>` holds an `x25519::basic_keypair<Backend>`: made from one
  (`from_keypair(x25519_keypair)`), by `generate()`, or by `derive_from_seed(seed)`, libsodium's seeded
  derivation (`crypto_kx_seed_keypair`: the secret is BLAKE2b-256 of the 32-byte seed), kept under
  its own name because programs store kx seeds (owner decision 7). The two differ on the same 32
  bytes: `from_keypair(*x25519::keypair::from_secret(bytes))` takes them as the secret, `derive_from_seed(bytes)`
  hashes them first; the guide says so, and a test checks that `derive_from_seed` gives
  `crypto_kx_seed_keypair`'s keys and differs from `from_keypair` on the same bytes; `public_key()` returns
  `x25519::public_key`; `client_session_keys(server)` and `server_session_keys(client)` take an
  `x25519::public_key` and return `session_keys` as today. Its seed and secret are the inner key
  pair's (`keypair()` gives it by reference).
- `public_key_encryption::basic_box<Family, B>::from_keys(own, peer)`, `own` an `x25519::basic_keypair<B>`, `peer` an
  `x25519::public_key`, computing the shared key once (libsodium's `beforenm`) as today.
- `public_key_encryption::basic_sealed_box<B>::seal(recipient, message, out)`, `recipient` an
  `x25519::public_key`; `open(recipient, sealed, out)`, `recipient` an `x25519::basic_keypair<B>`.
- `ed25519::basic_keypair<B>::to_x25519()` returns `result<x25519::basic_keypair<B'>>`, `B'` the
  X25519 family's backend that matches (libsodium for libsodium), and
  `ed25519::public_key::to_x25519()` returns `result<x25519::public_key>`, `invalid_public_key` for
  a key that is not canonical or of small order; the free `crypto::to_x25519(value)` calls either.
  They are conversions of bytes the program holds and WebCrypto has none: synchronous only, member
  and free (R4). `x25519_secret()` and `curves::ed25519::to_x25519_public` are removed.

### D3b. Namespaces by purpose (owner decision 8)

| Before | After | Header |
|---|---|---|
| `hybrid::basic_box`, `hybrid::box`, `hybrid::box_xchacha20poly1305` | `public_key_encryption::basic_box`, `box`, `box_xchacha20poly1305` | `<webcpp/crypto/public_key_encryption/box.hpp>` (was `hybrid/box.hpp`) |
| `hybrid::basic_sealed_box`, `hybrid::sealed_box` | `public_key_encryption::basic_sealed_box`, `sealed_box` | `<webcpp/crypto/public_key_encryption/sealed_box.hpp>` (was `hybrid/sealed_box.hpp`) |
| `curves::x25519::kx_keypair`, `curves::x25519::session_keys` | `key_exchange::kx_keypair`, `key_exchange::session_keys` | `<webcpp/crypto/key_exchange/kx.hpp>` (was `curves/x25519/kx.hpp`) |

The namespaces follow the directories, as `ciphers` and `curves` do; `detail`'s engines and states
of these types move with them (`detail::box_engine` and the others keep their names). The families
(`families::box_xsalsa20poly1305`, `families::box_xchacha20poly1305`, `families::sealed_box`,
`families::kx`), their backends and their capability rows are unchanged: only the namespaces and
headers of the public types move, and `crypto.json`'s entries that name a header or a type are
updated and its generated files rerun. The convenience header includes the new headers in place of
the old. The directory `hybrid/` and the namespace `crypto::hybrid` stay empty and unused in this
change, free for a later post-quantum hybrid (X25519MLKEM768, X-Wing).

### D3c. Factories named by their input (owner decision 9)

| Type | Before | After |
|---|---|---|
| every key pair (`ed25519`, `x25519`, `p256`, `secp256k1`) | `K::make(secret_bytes)`, `generate()` | `K::from_secret(secret_bytes)`, `K::generate()` |
| every public key | `T::make(bytes)` | `T::from_bytes(bytes)` |
| P-256's public key from raw coordinates | `make(x, y)` | `from_coordinates(x, y)` (R10) |
| every signature type | `T::make(bytes)` | `T::from_bytes(bytes)` |
| `ciphers::gcm`, `ciphers::basic_chacha20_poly1305` (was `ciphers::aead`, decision 14), `secret_key_encryption::basic_secretbox` (decision 13), the block cipher | `C::make(key)` | `C::from_key(key)` |
| `public_key_encryption::basic_box` | `make(own, peer)` | `from_keys(own, peer)` (R11) |
| `key_exchange::kx_keypair` | `make(x25519_keypair)`, `make(seed)` | `from_keypair(x25519_keypair)` (R12), `derive_from_seed(seed)`, `generate()` |
| free forms | `crypto::make<A>(...)` | `crypto::from_secret<K>`, `crypto::from_bytes<T>`, `crypto::from_key<C>`; `crypto::generate<K>` unchanged |
| asynchronous | `async_make` (member and free) | `async_from_secret`, `async_from_key`; `async_generate` unchanged |

The header `<webcpp/crypto/make.hpp>` of crypto/sign-unified, which holds the free factories,
becomes `<webcpp/crypto/factories.hpp>` (R13). No alias of `make` or `async_make` remains. A key
pair's `from_secret` and its `secret()` are inverses by name, and `from_bytes` and `bytes()` for
public values.

### D4. The key pair concepts

`<webcpp/crypto/keypair.hpp>` declares, folding over `detail::all_schemes` as crypto/sign-unified
does:

- `crypto::keypair<K>`: `K::from_secret(secret_bytes)` and `K::generate()` returning `result<K>`,
  `public_key()` returning `result<typename K::public_key_type>`, `secret()` returning
  `result<secret<K::secret_key_size>>`;
- `crypto::signing_keypair<K>`: `keypair<K>` and `sign(scheme, input)` for every scheme `K`
  declares, at least one, its result the scheme's signature type for `K`'s curve (D1);
- `crypto::agreement_keypair<K>`: `keypair<K>` and `agree(peer)`, `peer` a
  `typename K::public_key_type`, returning `result<secret<N>>`.

| Key pair | `keypair` | `signing_keypair` | `agreement_keypair` |
|---|---|---|---|
| `ed25519::basic_keypair<B>` | yes | yes | no (`to_x25519()` converts) |
| `x25519::basic_keypair<B>` | yes | no | yes |
| `p256::basic_keypair<B>` | yes | yes | yes |
| `secp256k1::basic_keypair<B>` | yes | yes | yes |

Each curve's header checks its row with `static_assert`s. `crypto::sign` and `async_sign` are
constrained by `signing_keypair`, `crypto::agree` and `async_agree` by `agreement_keypair`, and a
call with a key pair that does not do the operation stops the build with a message naming what the
key pair does ("X25519 key pairs agree; they do not sign").

### D5. The AEAD and box concepts

The AEAD contract is `secret_key_encryption::aead`'s (owner decision 13, D11). A new header
`<webcpp/crypto/secret_key_encryption/aead.hpp>` (no backend; `ciphers/aead.hpp` keeps the
one-shot template, `ciphers::basic_chacha20_poly1305` by decision 14, and includes it) declares,
in namespace
`webcpp::crypto::secret_key_encryption::aead`:

- `cipher<C>`: `C::key_size`, `C::nonce_size`, `C::tag_size` (`std::size_t` constants),
  `typename C::result_type`; `C::from_key(std::span<std::uint8_t const>)` returning `result<C>`;
  `c.seal(nonce, associated_data, data)` returning `result<typename C::result_type>`, `data`
  sealed in place; `c.open(nonce, associated_data, data, tag)` returning `result<void>`, `data`
  opened in place and zeroed on failure.
- `streaming_cipher<C>`: `cipher<C>` and `begin_seal(nonce, associated_data)`, `update(data)`,
  `result()`, `begin_open(nonce, associated_data)`, `authenticate(data)`, `decrypt(data)`,
  `verify(tag)` and `end_open()`, with AES-GCM's signatures.

`<webcpp/crypto/secret_key_encryption/authenticated_box.hpp>` (no backend) declares
`secret_key_encryption::authenticated_box<C>`:
`C::key_size`, `C::nonce_size`, `C::tag_size`, `typename C::result_type`; `c.seal(nonce, data)`
returning `result<result_type>`; `c.open(nonce, data, tag)` returning `result<void>`, `data`
zeroed on failure. The factory is not part of it: a secretbox is made by `from_key`, a crypto_box
by `from_keys` from two X25519 keys. The concept lives beside the secret box, in
`secret_key_encryption`; crypto_box, in `public_key_encryption`, satisfies it too (R21).

| Type | Concept |
|---|---|
| `ciphers::gcm<Cipher>` (`aes128_gcm`, `aes256_gcm`) | `secret_key_encryption::aead::streaming_cipher` |
| `ciphers::chacha20_poly1305`, `chacha20_poly1305_original`, `xchacha20_poly1305` | `secret_key_encryption::aead::cipher` |
| `secret_key_encryption::secretbox`, `secretbox_xchacha20poly1305` | `authenticated_box` |
| `public_key_encryption::box`, `box_xchacha20poly1305` | `authenticated_box` |

Each header checks its row with a `static_assert`. secretbox leaves the one-shot AEAD template
(`ciphers::aead` when built, `ciphers::basic_chacha20_poly1305` by decision 14):
it becomes `secret_key_encryption::basic_secretbox<Family, B>` with `from_key(key)`,
`seal(nonce, data)` and `open(nonce, data, tag)`, the guards and the zeroing of the AEAD template,
and no associated data; `detail::aead_family<secretbox...>` and its `max_associated_data_size` 0
are removed.

The free forms: `secret_key_encryption::aead::seal(c, nonce, associated_data, data)` and
`secret_key_encryption::aead::open(c, nonce, associated_data, data, tag)` constrained by `cipher`;
`secret_key_encryption::seal(c, nonce, data)` and `secret_key_encryption::open(c, nonce, data, tag)`
constrained by `authenticated_box`, for a secret box and a crypto_box alike (R21). No free `seal`
or `open` exists in `crypto::` nor in `public_key_encryption`. A box given to `secret_key_encryption::aead::seal` or `open` stops the build with a
message naming the box forms; the arities differ, so no call is ambiguous.

### D6. Asynchronous seal and open everywhere native

`async_seal` and `async_open`, member (`c.async_seal(executor, ...)`) and free
(`secret_key_encryption::aead::async_seal(executor, c, ...)` for a `cipher`;
`secret_key_encryption::async_seal` for an `authenticated_box`, R21), with the synchronous arguments between the executor and the token,
exist for every `secret_key_encryption::aead::cipher` and every
`secret_key_encryption::authenticated_box`. On a synchronous backend they run
the synchronous member and complete through `asio::post`, never inline, as the family does today.
On `backends::browser` (a backend whose `detail::asynchronous_backend_v` is true), only AES-GCM is
offered, through WebCrypto; any other stops the build, from every form, synchronous or
asynchronous, member or free (3.3's fix round, the controller's ruling), with one text that says no
form of them runs there: "webcpp::crypto: WebCrypto has no ChaCha20-Poly1305, XChaCha20-Poly1305,
secret box or crypto_box: no form of them, synchronous or asynchronous, runs on the browser's
backend; name a native backend for them. On the browser's backend only AES-GCM seals and opens."
(X5 rewords it so; 3.3 wrote "their async_seal, async_open and async_from_key run on a native
backend", which named only the asynchronous forms.) This supersedes the foundation's sentence that
no `async_` variant of a cheap algorithm WebCrypto lacks may exist, for seal and open only, by the
owner's decision; `async_from_key` follows the same rule for the types these concepts cover.

### D8. Ethereum's recoverable ECDSA (owner decision 10)

- The tag `ecdsa_keccak256_recoverable_t` (explicit default constructor) and the value
  `crypto::ecdsa_keccak256_recoverable` join `detail::all_schemes`; `scheme_traits` gives a 65-byte
  signature and a 32-byte digest; `signs_with_v<secp256k1::basic_keypair<B>, ...>` is true, and
  `signature_of` of it is `secp256k1::recoverable_signature`, the type `ecdsa_sha256_recoverable`
  gives (the bytes are the same; the scheme is the input's hash).
- `key.sign(ecdsa_keccak256_recoverable, input)` and `crypto::sign(key, ...)`: a message is hashed
  with `webcpp::hash::keccak_256` (`<webcpp/hash/keccak.hpp>`, over XKCP), a marker's 32 bytes are
  signed as they are; libsecp256k1's `secp256k1_ecdsa_sign_recoverable` with RFC 6979's nonce, low S.
- `curves::secp256k1::recover(ecdsa_keccak256_recoverable, signature, input)`, member-free as
  `ecdsa_sha256_recoverable`'s, with its checks (a recovery id above 3, r or s not below n, no key:
  `invalid_signature`). No `crypto::verify` overload: `ecdsa_sha256_recoverable` has none (its
  signature gives its key to `recover`), and Ethereum compares recovered keys or addresses (R18).
- Forms: the synchronous member and free forms; no asynchronous form, as no secp256k1 operation has
  one (crypto/sign-unified's R8).
- Keccak-256 needs `/third_party//xkcp`, which builds on every target and toolset; secp256k1's
  header includes `keccak.hpp` only where `WEBCPP_HASH_HAS_XKCP` is defined, and a message input
  without it stops the build with the Jamfile line (R19). A marker needs no XKCP.
- The guide: the digests a program builds (EIP-191 with an example program over `webcpp::hash`),
  `recovery_id()` to `v` (27 + id for `personal_sign`, 35 + 2 x chain id + id for EIP-155, the
  id itself as `yParity` for typed transactions), and the address (the last 20 bytes of the
  Keccak-256 of the uncompressed key without its 0x04), in the example, not in crypto.

### D9. Ed25519ph as the scheme `eddsa_prehashed` (owner decision 12)

- The tag `eddsa_prehashed_t` (explicit default constructor) and the value
  `crypto::eddsa_prehashed` join `detail::all_schemes` (after `eddsa_t`); `scheme_traits` gives a
  64-byte signature and a 64-byte digest, SHA-512's; `signs_with_v<ed25519::basic_keypair<B>, ...>`
  and `verifies_with_v<ed25519::public_key, ...>` are true, and `signature_of` of either is
  `ed25519::signature`.
- The input is `crypto::prehashed{digest}` of 64 bytes alone: a message stops the build with a
  message naming the marker, `eddsa` and the streaming signer; a digest of another size with one
  naming SHA-512's 64 bytes; `eddsa` keeps refusing a marker, its text now naming
  `eddsa_prehashed`.
- `key.sign(eddsa_prehashed, prehashed{digest})`, `crypto::sign`, `crypto::verify(eddsa_prehashed,
  signature, prehashed{digest}, public_key[, policy])` and `public_key.verify(...)`, strict as
  `eddsa` is; their asynchronous forms run on native backends and stop the build on a backend
  whose `asynchronous_backend_v` is true, WebCrypto having no Ed25519ph.
- `curves::ed25519::prehashed_signer<Hash, B>` (was `ph`) streams the message, SHA-512 inside, and
  its `sign` and `verify` give and take `ed25519::signature`; over the same message it gives the
  signature `sign(eddsa_prehashed, prehashed{sha512(message)})` gives.
- An `eddsa` and an `eddsa_prehashed` signature have one type: one given to the other scheme's
  `verify` is `invalid_signature` at run time, as RFC 8032's domain separation makes it; the page
  says which a protocol uses is the protocol's choice.

### D10. Protecting secrets: `constant_time_equal` and `erase_secret` (owner decision 11)

| Before | After | Header |
|---|---|---|
| `crypto::equal(span, span)`, `crypto::equal(digest<N>, digest<N>)` | `crypto::constant_time_equal`, both overloads, the same work | `<webcpp/crypto/constant_time_equal.hpp>` (was `equal.hpp`) |
| (none; the plan's `crypto::wipe`, never released) | `crypto::erase_secret(range)` over `detail::wipe` | `<webcpp/crypto/erase_secret.hpp>` |

Both stay at the top of `webcpp::crypto`, beside `secret<N>` and `random_device`; the guide's
section "Protecting secrets outside the primitives" groups the four (memory hygiene: `secret<N>`,
`erase_secret`; timing hygiene: `constant_time_equal`; drawing: `random_device`).

### D11. Secret-key encryption and spelled-out names (owner decision 13)

| Before | After | Header |
|---|---|---|
| `crypto::seal`, `crypto::open` (the AEAD forms) | `secret_key_encryption::aead::seal`, `open` | `<webcpp/crypto/secret_key_encryption/aead.hpp>` (were in `ciphers/aead.hpp`), which every AEAD's header includes |
| `crypto::async_seal`, `crypto::async_open` | `secret_key_encryption::aead::async_seal`, `async_open` | `<webcpp/crypto/async.hpp>`, which defines every asynchronous form |
| `crypto::aead`, `crypto::streaming_aead` (planned, never built) | `secret_key_encryption::aead::cipher`, `streaming_cipher` | `<webcpp/crypto/secret_key_encryption/aead.hpp>` |
| `ciphers::secretbox`, `ciphers::secretbox_xchacha20poly1305` (and the planned `ciphers::basic_secretbox`) | `secret_key_encryption::secretbox`, `secretbox_xchacha20poly1305`, `basic_secretbox` | `<webcpp/crypto/secret_key_encryption/secretbox.hpp>` (was `ciphers/secretbox.hpp`) |
| `ciphers::gcm`, `aes128_gcm`, `aes256_gcm`, `chacha20_poly1305`, `chacha20_poly1305_original`, `xchacha20_poly1305`, `aes128`, `aes192`, `aes256` | unchanged | unchanged |
| the template `ciphers::aead<Family, B>` | `ciphers::basic_chacha20_poly1305<Family, B>` (decision 14, D12) | `<webcpp/crypto/ciphers/aead.hpp>`, unchanged (R25) |

The namespaces by purpose, with D3b's:

| Namespace | Holds |
|---|---|
| `crypto` | the root: `random_device`, `constant_time_equal`, `erase_secret`, `secret<N>`, `error`, the signing vocabulary, the key pair concepts, the free factories |
| `crypto::ciphers` | symmetric algorithms: AES, AES-GCM, ChaCha20-Poly1305 in three nonce sizes |
| `crypto::secret_key_encryption` | constructions under one secret key: `secretbox`, the concept `authenticated_box` and the box forms `seal`/`open`; `aead`, the AEAD contract over `ciphers`' AEADs |
| `crypto::public_key_encryption` | constructions between key pairs: `box`, `sealed_box` |
| `crypto::key_exchange` | crypto_kx: `kx_keypair`, `session_keys` |
| `crypto::curves` | curve primitives |

The naming rule: every public name of crypto is spelled out; algorithm and format proper names
stay acronyms (`aes`, `gcm`, `chacha20`, `hmac`, `hkdf`, `ecdsa`, `eddsa`, `sha256`, `kx`, `der`,
`pem`, `spki`, `pkcs8`, BIP340's `xonly`), and so do a standard's own variable names (ECDSA's `r` and
`s`). The pages and doc comments say "nonce" and "associated data" (RFC 5116's term); a comment
that quotes a backend or a standard in its own words (OpenSSL's IV, NIST SP 800-38D's IV and
additional authenticated data) keeps them.

| Before | After |
|---|---|
| parameters `iv`, `iv_bytes` | `nonce`, `nonce_bytes` |
| parameters `additional_data`, `additional_data_bytes`, `ad` | `associated_data`, `associated_data_bytes` |
| parameter `ctx` | `context` (none in crypto when decided) |
| secp256k1's `sign(schnorr, message, aux_random)` | `auxiliary_random_bytes` |
| member constants `iv_size`, `max_iv_size`, `max_additional_data_size` | `nonce_size`, `max_nonce_size`, `max_associated_data_size` (R22) |
| `error::invalid_iv_size` (value 2) | `error::invalid_nonce_size` (value 2; "the nonce is not of a size the algorithm accepts") (R22) |

### D12. The ChaCha20-Poly1305 template and secp256k1's names spelled out (owner decision 14)

| Before | After | Header |
|---|---|---|
| `ciphers::aead<Family, Backend>` | `ciphers::basic_chacha20_poly1305<Family, Backend>`; its refusal of a family that is no AEAD's names it ("webcpp::crypto: ciphers::basic_chacha20_poly1305 takes an AEAD family: ...") | `<webcpp/crypto/ciphers/aead.hpp>` (R25) |
| `curves::secp256k1::musig::nonce_gen` | `musig::generate_nonce` | `curves/secp256k1/musig.hpp` |
| `curves::secp256k1::pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate`, `pubkey_combine` | `public_key_tweak_add`, `public_key_tweak_mul`, `public_key_negate`, `public_key_combine` | `curves/secp256k1/tweak.hpp` |
| `curves::secp256k1::seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate` | `secret_key_tweak_add`, `secret_key_tweak_mul`, `secret_key_negate` | `curves/secp256k1/tweak.hpp` |
| `curves::secp256k1::silent_payments::prevouts_summary`, `make_prevouts_summary` | `previous_outputs_summary`, `make_previous_outputs_summary` (R24) | `curves/secp256k1/silent_payments.hpp` |
| `chacha20_poly1305`, `chacha20_poly1305_original`, `xchacha20_poly1305`; secp256k1's `agree`, `agree_hashed`, `agree_point`; `combine_workspace_size`, `workspace_size`, `scan_workspace_size`, `send_workspace_size`, `summary_workspace_size` (no abbreviation, R25); `ellswift_*`, `tweak_xonly`, `tweak_xonly_add`, MuSig2's `key_aggregate`, `nonce_aggregate`, `nonce_process`, `partial_sign` and the other spelled-out names | unchanged | unchanged |

The signatures, the behaviour, the guards, the timing coverage and the capability rows
(`key_tweaks`, `musig`, `silent_payments`) are unchanged: only the names move. The timing seam's
member labels follow the class (`ciphers::basic_chacha20_poly1305::seal` for
`ciphers::aead::seal`). A doc comment that quotes libsecp256k1's own names (`secp256k1_ec_pubkey_tweak_add`,
the keyagg cache, `pubnonce`, `aggnonce`, `partial_sig`, `prevouts_summary` as libsecp256k1's type)
or a test vector's field names (BIP352's `scan_pubkey`, BIP327's `nonce_gen_vectors`) keeps them.

### D7. Removed, and callers

Removed with no alias: the span `verify` and `recover` overloads of crypto/sign-unified; `sign`'s
array results; `curves::x25519::public_key(secret)`, `curves::x25519::agree(secret, peer)`;
`ed25519::basic_keypair::x25519_secret()`, `curves::ed25519::to_x25519_public`;
`kx_keypair::make(seed)` (now `derive_from_seed`); every `make` and `async_make` (D3c); the namespace `crypto::hybrid` and the headers
`hybrid/box.hpp`, `hybrid/sealed_box.hpp` and `curves/x25519/kx.hpp` (D3b); the boxes'
byte-key factories, `seal` and `open`; secretbox's additional data; `crypto::equal` and
`<webcpp/crypto/equal.hpp>` (D10); `curves::ed25519::ph` (now `prehashed_signer`, D9); the
free `crypto::seal`, `crypto::open`, `crypto::async_seal`, `crypto::async_open`,
`ciphers::secretbox`, `ciphers::secretbox_xchacha20poly1305`, `<webcpp/crypto/ciphers/secretbox.hpp>`,
`iv_size`, `max_iv_size`, `max_additional_data_size` and `error::invalid_iv_size` (D11);
`ciphers::aead`, `musig::nonce_gen`, `pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate`,
`pubkey_combine`, `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `prevouts_summary` and
`make_prevouts_summary` (D12). No public
`crypto::wipe` ever exists (D10). Every caller in libs/crypto moves
in the same change: tests, examples, fixtures, the page, the constant-time harnesses and suppressions, the
benches, the message tests, the MSVC tests, `doc/tables/crypto.json` and its generated files
(rerun, never edited); `crypto::equal`'s callers in other libraries (webcpp::hash) move in the
same change, through the superproject's pointers. webcpp::trystero's port of Trystero 0.27 moves
later, by its own stream (decisions 11 and 13). The X25519 format functions keep their names here and leave with the
formats change, which gives them the typed keys' formats.

## Rulings

- **R1 (owner decision 6): `verify` takes the typed signature only; no bytes overload.** A signature from the wire is
  made first (`p256::ecdsa_signature::from_bytes(bytes)` or `crypto::from_bytes<T>(bytes)`), where its size is
  checked; then the type says its curve and scheme, and a mismatch is a compile-time error rather
  than an `invalid_signature` at run time. One form keeps the four mirrored forms one, and matches
  crypto/sign-unified's R17 for public keys. BIP340's x-only key bytes keep their `verify` overload:
  that is a key, not a signature.
- **R2: a signature's `from_bytes` checks the size only** (D1), so that a high-S or out-of-range
  signature from a peer can still be made, normalized and refused by `verify` with the policy of
  its curve.
- **R3: an X25519 public key's `from_bytes` checks the size only** (D2), as Ed25519's does; small order
  is `agree`'s refusal, `weak_public_key`, where RFC 7748 puts it.
- **R4: conversions and value construction are synchronous only, member and free**: `from_bytes` of a
  signature or a public key and `to_x25519` call no backend that WebCrypto could replace (WebCrypto
  converts nothing between Ed25519 and X25519), so "four mirrored forms" applies to the operations
  that call a backend (`from_secret` and `generate` of a key pair, `from_key` of a cipher, `sign`, `verify`, `agree`, `seal`,
  `open`), as crypto/sign-unified's D9 table lists them.
- **R5 (owner decision 7): the seeded derivation is `kx_keypair::derive_from_seed(seed)`**, never a
  factory named after another input, so that the call site says a seed is hashed (D3).
- **R6: sealed boxes stay out of the concepts** (Non-goals) but take typed keys (D3).
- **R10: P-256's public key from coordinates is `from_coordinates(x, y)`**, not a second
  `from_bytes` overload, so that the name says the input is two coordinates (a COSE key's), not an
  encoded point.
- **R11: crypto_box is made by `from_keys(own_keypair, peer_public_key)`**: two keys, the plural
  name says so, and neither is "the" secret alone.
- **R12: kx wraps a key pair by `from_keypair(x25519_keypair)`**: its input is a key pair, not
  secret bytes, so `from_secret` would misname it; a kx key pair from raw secret bytes is
  `from_keypair(*x25519::keypair::from_secret(bytes))`.
- **R13: the free factories' header is `<webcpp/crypto/factories.hpp>`** (was `make.hpp`).
- R8 and R9 were not assigned; the numbers are kept so that R10 to R13 keep the names other
  documents use.
- **R14 (superseded by owner decision 12): Ed25519ph's signature is `curves::ed25519::signature`**.
  The first draft ruled a sixth type, `ph_signature`; Ed25519ph is now the scheme
  `eddsa_prehashed` (D9), whose signature is Ed25519's 64-byte type, so no `ph_signature` exists,
  and an Ed25519ph signature given to the pure `verify` fails at run time with
  `invalid_signature` (D9's last point).
- **R15: `to_der`, `from_der`, `normalize_low_s` and `is_low_s` take and give the curve's
  `ecdsa_signature`** on P-256 and secp256k1, so that no function takes a signature as bytes; the
  formats change builds on typed values.
- **R16: X25519's `algorithm<B>` becomes `detail::x25519_operations<B>`**, the unguarded workers
  (`base`, `agree`) the key pair and the asynchronous family share (D2).
- **R17: no function is named `make`, a detail engine's included** (`import_secret`,
  `seed_keypair`, `allocate`); decision 9's "internal capabilities keep their names" means the
  capability table's rows (`*_import`, `*_generate`), which are not functions.
- **R18: `ecdsa_keccak256_recoverable` has `recover` and no `verify`**, mirroring
  `ecdsa_sha256_recoverable` (D8).
- **R19: Keccak-256's message form needs XKCP linked**; without it, "webcpp::crypto:
  crypto::ecdsa_keccak256_recoverable hashes a message with webcpp::hash's keccak_256, which needs
  XKCP: add /third_party//xkcp to the program's Jamfile, or sign crypto::prehashed{digest}." (D8).
- **R20: both recoverable schemes give `secp256k1::recoverable_signature`** (the owner's
  ruling): a signature recovered under the other scheme gives another key, which the program's
  comparison of keys or addresses refuses.
- **R7 (amended by owner decision 13): the AEAD concepts live in
  `<webcpp/crypto/secret_key_encryption/aead.hpp>`** (was `<webcpp/crypto/aead.hpp>`), beside the
  free AEAD forms, which `ciphers/aead.hpp` and `ciphers/gcm.hpp` include; it includes no backend.
- **R21 (the coordinator's refinement of decision 13, 2026-11-23): `authenticated_box` is
  `crypto::secret_key_encryption::authenticated_box`**, beside the secret box, in
  `<webcpp/crypto/secret_key_encryption/authenticated_box.hpp>` (no backend), which
  `secret_key_encryption/secretbox.hpp` and `public_key_encryption/box.hpp` include; its box forms
  follow it: `secret_key_encryption::seal(box, nonce, data)`, `open(box, nonce, data, tag)`,
  `async_seal` and `async_open`, for a secret box and a crypto_box alike (argument-dependent lookup
  finds them for a secret box; a crypto_box is sealed by `secret_key_encryption::seal` named in
  full, or by its member). No free `seal` or `open` exists in `crypto::` nor in
  `public_key_encryption`.
- **R22: decision 13's rule covers every public name, not only parameters**: the members
  `iv_size`, `max_iv_size` and `max_additional_data_size`, which the concepts require, become
  `nonce_size`, `max_nonce_size` and `max_associated_data_size`, and the error
  `invalid_iv_size` becomes `invalid_nonce_size` (its value 2 kept), so that a nonce is never
  called an IV in one name and a nonce in the next; `max_` stays, the standard library's own word
  (`std::numeric_limits<T>::max`, `max_size`). Accepted by the coordinator on 2026-11-23; X4 does it.
- **R23 (controller ruling, 2026-11-23): 3.2 and 3.3 are written with decision 13's names; X4
  covers the renames they do not touch.** 3.2 creates `secret_key_encryption/aead.hpp` and
  `secret_key_encryption/authenticated_box.hpp`, moves the free `crypto::seal`/`open` into
  `secret_key_encryption::aead` and `ciphers::secretbox` into `secret_key_encryption` (header
  `secret_key_encryption/secretbox.hpp`), and migrates their callers; 3.3 moves the free
  `crypto::async_seal`/`async_open` into `secret_key_encryption::aead` and migrates theirs. X4,
  which runs before 2.2, renames what stays where it is: `iv` and `additional_data` on the existing
  members, free forms and engines, `nonce_size`, `max_nonce_size`, `max_associated_data_size`,
  `error::invalid_nonce_size`, `aux_random`, any other abbreviation, every call site of those names,
  and the CODING-GUIDELINES rule; so the names 3.2 and 3.3 move already carry the renames.
- **R24: decision 14's "any other public abbreviation of the same kind" is silent payments'
  `prevouts`** (the amendment's reading, for the owner to confirm): libsecp256k1 names BIP352's
  summary of the previous outputs `prevouts_summary`, and crypto took the name for its type and its
  factory, so they become `previous_outputs_summary` and `make_previous_outputs_summary` (the verb
  kept: R17 forbids a function named `make` alone, and this one says what it makes). A survey of
  every public name in `include/webcpp/crypto` (outside `detail` and the backends) found no other:
  `outpoint` is Bitcoin's own term, `xdh` (`ellswift_xdh`) RFC 7748's and BIP324's, `xonly`
  BIP340's, and MuSig2's other names were spelled out when built (`key_aggregate`,
  `nonce_aggregate`, `partial_signature`).
- **R25: decision 14 renames names, not headers or helpers without an abbreviation**: the template
  stays in `<webcpp/crypto/ciphers/aead.hpp>`, which `ciphers/chacha20_poly1305.hpp` includes and
  which also holds the AEAD families (a program includes the aliases' header, so no include
  moves); the workspace helpers (`combine_workspace_size` for `public_key_combine`, MuSig2's
  `workspace_size`, silent payments' three) carry no abbreviation and keep their names; the message
  tests and fixture named `aead_not_a_family*` keep their directory names (as P29), their code and
  texts following the class.

## As built

Built on the branches `crypto/uniform-types` (crypto `e07d44a`, hash `659e50d`, superproject
`0ab0ace`). The rulings made while building that change or refine the design above:

- **P27, Ed25519ph on libsodium's internals (D9):** `sign(eddsa_prehashed, ...)` and its `verify`
  call libsodium's `_crypto_sign_ed25519_detached` and `_crypto_sign_ed25519_verify_detached` with
  `prehashed` 1, since its public API cannot sign a finished digest; they link because libsodium is
  built from source. A rename upstream or a prebuilt libsodium breaks it, and RFC 8032's Ed25519ph
  vectors then fail at once. No other shipped backend implements Ed25519 (`backends/openssl.hpp`
  has none; the family's default is libsodium on every target), so no public path exists yet.
- **`check_signature` takes three arguments (D1, R1):** `detail::check_signature<Expected,
  Signature, Verifier>`, `Verifier` the scheme tag or `ed25519ph_verifier_t` (the streaming
  signer's `verify`), beside `checked_signature_bytes`: a signature of another type stops the
  build with one message, and Ed25519ph's shared `ed25519::signature` has its explicit branch.
- **`can_verify` takes the signature (D1, P7, F8):** `can_verify<Scheme, Signature, Input,
  PublicKey>`, true only for the type of the key's curve and scheme (the spec amended); internal
  input checks call `detail::input_valid_v`, never `!can_verify`.
- **Signatures (D1, P6):** the default constructor stays non-explicit (all zeros, `std::regular`),
  so `verify(scheme, {}, input, key)` compiles and fails; `recovery_id()` gives the stored byte and
  `recover` refuses an id above 3.
- **Guards of `from_bytes` (D3c, P19, F12):** the free `crypto::from_bytes<T>` holds no guard
  (`from_secret` and `from_key` keep theirs); P-256's and secp256k1's public keys' `from_bytes` and
  `from_coordinates` validate the point through the backend and hold it; the signatures', Ed25519's
  and X25519's public keys' `from_bytes` enter none. R4's "call no backend" is read accordingly.
- **`detail::x25519_access::keyed` (D2, D3, P13):** reads, without a guard, whether an X25519 key
  pair was moved from, so kx, the boxes and the sealed box refuse one with `invalid_state` before
  reading its zeroed bytes.
- **`detail::kx_engine_defined` (D3):** derived from the capability table (supported and linked),
  not written by hand; it relies on `kx.hpp`'s tail include and one kx backend, which its comment
  says. The boxes and kx are refused by `supports_v<keypair_type, Backend>`, and libsodium's sections
  for them key on `WEBCPP_CRYPTO_DETAIL_{BOX,SEALED_BOX,KX}_DECLARED`, breaking the include cycle
  through `curves/x25519.hpp` as the formats headers do.
- **Key pair concepts (D4):** the free `crypto::sign` lives in `keypair.hpp`; `async_agree` is one
  template over the key pair (`detail::agrees_with_peer`); Ed25519's unreachable `agree` clause is
  gone, `agreement_keypair` refusing first.
- **D6's text covers the synchronous forms:** on an asynchronous backend, ChaCha20-Poly1305,
  XChaCha20-Poly1305, secretbox and crypto_box refuse their synchronous `seal`, `open` and
  `from_key(s)` too, with D6's text (no form of the constructions WebCrypto lacks runs on the
  browser's backend), through `detail::refuse_aead_on_asynchronous_backend`; AES-GCM's synchronous
  forms keep "call async_seal".
- **The timing seam matches every qualifier:** the test seam that ties a timing guard to the
  function a coverage check names compares every component of the label, not only the last, so the
  member `ciphers::basic_chacha20_poly1305::seal` and the free `secret_key_encryption::aead::seal`
  are told apart; it also reads GCC's spelling of a trailing requires-clause.
- **The browser's draw (X1, F3, P15):** `EM_ASM_INT` inside the inline `read_entropy`, never an
  `EM_JS` in a header; a throwing `getRandomValues` is `entropy_unavailable`, and its JavaScript
  array is zeroed after the copy and after a throw.
- **node 18's WebCrypto (X1):** without `globalThis.crypto`, the draw falls back to
  `require('node:crypto').webcrypto` inside the same `try` (Emscripten's minimum is node 18.3),
  superseding P15's cost; a bundler that cannot resolve it fails closed as `entropy_unavailable`.
- **The probe before `sodium_init` (X1b), superseding F4's consequence:** on Emscripten,
  `sodium_ready` draws a small probe through crypto's entropy before `sodium_init`; a failed probe
  returns its error and leaves libsodium uninitialised, so the next call retries and no zero
  `sodium_malloc` canary is kept. The watch around `sodium_init` stays for a good probe followed by
  a failing canary draw (a zero canary then, documented). The stack copies of each draw are wiped.
- **The native draw watch is a stateless stub (X1b):** a `thread_local` state allocated on macOS
  at first access, which `allocation_sodium` counts; the watched state exists on Emscripten only.
- **`generate.py`'s message rows:** the messages page lists every text a message test pins, each
  once, in the source's order, and leaves out a directive marked `(knock-on)`, so that each row
  leads with the refusal, not with an error that follows from it.
- **Removed-name message tests (3.2, X5):** a using-declaration and a call per group, since
  clang's typo correction differs by version; `ciphers::aead`'s removal is pinned through an alias
  (`aead_template_removed_call`).
- **The C4459 test rename (4.2):** MSVC flagged two tests' globals, `associated_data` and
  `secret_key`, which a library template's parameter names hide; the tests' names changed
  (`test_associated_data`, `secret_key_hex`), not the parameter, which decision 13 fixes.
- **X3's survey** renamed one-word locals and parameters only; static data members (`name`,
  `size`, `order`, `overhead`) keep their names, which C4459 does not concern.

## Risks / Trade-offs

- Every signing caller changes its result type; the migration is mechanical (`.bytes()` where bytes
  were used) and the build finds every site.
- Programs that verify wire signatures gain a `from_bytes` call (R1).
- Every `make` in libs/crypto and its users is renamed (D3c); the build finds each site.
- Async seal and open on native backends add instantiations only where called.

## Migration Plan

crypto/sign-unified merges; then this change; then crypto's formats; then phase 3 of webcpp::trystero's port of
Trystero 0.27. Branches `crypto/uniform-types` of webcpporg/crypto, webcpporg/proposals and the
superproject; local verification on every lane; merged by the controller; one CI at the end.

## Open Questions

None: the owner's decisions 1 to 14 settle the change (R21 to R23 settle how decision 13 is
built, R24 and R25 read decision 14 where it is silent).
