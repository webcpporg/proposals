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
   `make(bytes)`; whether `verify` keeps a bytes overload is ruled here (R1).
2. **X25519 typed like the others**: `x25519::public_key` (`make`, `bytes`),
   `x25519::basic_keypair<B>` (`make`, `generate`, `public_key()`, `secret()`, `agree(peer)` with
   `async_agree` and the free `crypto::agree`), `kx_keypair` built on it; Ed25519's
   `x25519_secret()` and `to_x25519_public` become a typed conversion (`ed_key.to_x25519()` giving
   an `x25519::keypair`, `ed_pub.to_x25519()` an `x25519::public_key`); the old functions removed.
3. **Key pair concepts split**: `keypair` (`make`, `generate`, `public_key()`, `secret()`),
   `signing_keypair` (also `sign` for each declared scheme, at least one), `agreement_keypair`
   (also `agree`). Ed25519 signs; P-256 and secp256k1 do both; X25519 agrees.
4. **`concept aead`** (`key_size`, `iv_size`, `tag_size`, `result_type`, `static make(key)`
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
   goes through `make` first (R1 confirmed).
7. **libsodium's seeded kx derivation keeps a distinct name**, option (b):
   `x25519::kx_keypair::derive_from_seed(seed)` (`crypto_kx_seed_keypair`: the secret is
   BLAKE2b-256 of the seed), beside `make(x25519_keypair)` and `generate()`; `make` keeps its
   library-wide meaning, "these bytes, or this key, are the secret" (R5 replaced).

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
| `curves::ed25519::signature` | 64 | `ed25519::basic_keypair<B>::sign(eddsa, ...)` | `eddsa` |
| `curves::p256::ecdsa_signature` | 64, r then s | `p256::basic_keypair<B>::sign(ecdsa_sha256, ...)` | `ecdsa_sha256` |
| `curves::secp256k1::ecdsa_signature` | 64, r then s | `secp256k1::basic_keypair<B>::sign(ecdsa_sha256, ...)` | `ecdsa_sha256` |
| `curves::secp256k1::recoverable_signature` | 65, r, s, the id | `sign(ecdsa_sha256_recoverable, ...)` | `recover(ecdsa_sha256_recoverable, ...)` |
| `curves::secp256k1::schnorr_signature` | 64 | `sign(schnorr, ...)` | `schnorr` |

Each is a regular value type (copyable, comparable with `==`; a signature is public), implemented
by one class template in `detail` with a distinct tag per type, so that no two convert into each
other. Each has `static constexpr std::size_t size`; `static make(std::span<std::uint8_t const>)`
returning `result<T>`, `invalid_signature` for another size and nothing else checked (whether r and
s are below n, or R canonical, is verification's judgment, as crypto/sign-unified's R16 rules for
Ed25519's public key); `bytes()` returning `std::array<std::uint8_t, size> const&`; and nothing
that converts implicitly. `recoverable_signature` adds `recovery_id()` (0 to 3). The free
`crypto::make<T>(bytes)` of crypto/sign-unified serves them unchanged.

`sign` returns `result<T>` on every key pair and scheme, the asynchronous `sign` completes with
`void(error_code, T)`; `crypto::verify(scheme, signature, input, public_key)`, `pub.verify`,
their asynchronous forms and `recover` take `T`. The trait that pairs a scheme with its key's types
(`detail::signs_with_v`, crypto/sign-unified's D5) gains the signature type, so `can_verify` is
false for a signature of another curve or scheme, with a message naming the type it expects.
`normalize_low_s` and `is_low_s` take and return the curve's `ecdsa_signature`.

### D2. X25519's types

- `curves::x25519::public_key`: `static make(std::span<std::uint8_t const>)` returning
  `result<public_key>`, `invalid_public_key` for another size than 32 and nothing else checked
  (a small-order key is refused where it matters, by `agree`, as `weak_public_key`); `bytes()`.
- `curves::x25519::basic_keypair<Backend>` (and `x25519::keypair`, its default): `public_key_type`,
  `secret_key_size` 32; `static make(secret_bytes)` (RFC 7748's 32-byte scalar, clamped by the
  backend as today) and `static generate()`, returning `result<basic_keypair>`; `public_key()`
  returning `result<public_key>`; `secret()` returning `result<secret<32>>`; `agree(peer)`, `peer`
  a `public_key`, returning `result<secret<32>>` and `weak_public_key` when the shared secret is
  all zero, on every backend, as the foundation requires; moved from, every call returns
  `invalid_state`; its state wiped on destruction and move, as the other key pairs are.
- The four forms: `make`, `generate`, `agree` as members; `crypto::make<K>`, `crypto::generate<K>`,
  `crypto::agree(key, peer)` free (crypto/sign-unified's generic ones, now constrained by D3);
  `K::async_make`, `K::async_generate`, `key.async_agree(executor, peer, token)` and the free
  `async_make`, `async_generate`, `async_agree`, defined in `async.hpp`. X25519 is in WebCrypto, so
  the asynchronous forms exist on every backend, the browser's included once part B lands.
- Removed: the free `curves::x25519::public_key(secret)` (its name is the type's now) and
  `curves::x25519::agree(secret, peer)`; `curves::x25519::algorithm<B>` moves into `detail`, its
  family `families::x25519` unchanged.

### D3. crypto_kx, the boxes and Ed25519's conversion on typed keys

- `curves::x25519::kx_keypair<Backend>` holds an `x25519::basic_keypair<Backend>`: made from one
  (`make(x25519_keypair)`), by `generate()`, or by `derive_from_seed(seed)`, libsodium's seeded
  derivation (`crypto_kx_seed_keypair`: the secret is BLAKE2b-256 of the 32-byte seed), kept under
  its own name because programs store kx seeds (owner decision 7). The two differ on the same 32
  bytes: `make(x25519::keypair::make(bytes))` takes them as the secret, `derive_from_seed(bytes)`
  hashes them first; the guide says so, and a test checks that `derive_from_seed` gives
  `crypto_kx_seed_keypair`'s keys and differs from `make` on the same bytes; `public_key()` returns
  `x25519::public_key`; `client_session_keys(server)` and `server_session_keys(client)` take an
  `x25519::public_key` and return `session_keys` as today. Its seed and secret are the inner key
  pair's (`keypair()` gives it by reference).
- `hybrid::basic_box<Family, B>::make(own, peer)`, `own` an `x25519::basic_keypair<B>`, `peer` an
  `x25519::public_key`, computing the shared key once (libsodium's `beforenm`) as today.
- `hybrid::basic_sealed_box<B>::seal(recipient, message, out)`, `recipient` an
  `x25519::public_key`; `open(recipient, sealed, out)`, `recipient` an `x25519::basic_keypair<B>`.
- `ed25519::basic_keypair<B>::to_x25519()` returns `result<x25519::basic_keypair<B'>>`, `B'` the
  X25519 family's backend that matches (libsodium for libsodium), and
  `ed25519::public_key::to_x25519()` returns `result<x25519::public_key>`, `invalid_public_key` for
  a key that is not canonical or of small order; the free `crypto::to_x25519(value)` calls either.
  They are conversions of bytes the program holds and WebCrypto has none: synchronous only, member
  and free (R4). `x25519_secret()` and `curves::ed25519::to_x25519_public` are removed.

### D4. The key pair concepts

`<webcpp/crypto/keypair.hpp>` declares, folding over `detail::all_schemes` as crypto/sign-unified
does:

- `crypto::keypair<K>`: `K::make(secret_bytes)` and `K::generate()` returning `result<K>`,
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

A new header `<webcpp/crypto/aead.hpp>` (the concepts only; `ciphers/aead.hpp` keeps the
`ciphers::aead` template) declares:

- `crypto::aead<C>`: `C::key_size`, `C::iv_size`, `C::tag_size` (`std::size_t` constants),
  `typename C::result_type`; `C::make(std::span<std::uint8_t const>)` returning `result<C>`;
  `c.seal(iv, additional_data, data)` returning `result<typename C::result_type>`, `data` sealed in
  place; `c.open(iv, additional_data, data, tag)` returning `result<void>`, `data` opened in place
  and zeroed on failure.
- `crypto::streaming_aead<C>`: `aead<C>` and `begin_seal(iv, additional_data)`, `update(data)`,
  `result()`, `begin_open(iv, additional_data)`, `authenticate(data)`, `decrypt(data)`,
  `verify(tag)` and `end_open()`, with AES-GCM's signatures.
- `crypto::authenticated_box<C>`: `C::key_size`, `C::iv_size`, `C::tag_size`,
  `typename C::result_type`; `c.seal(iv, data)` returning `result<result_type>`;
  `c.open(iv, data, tag)` returning `result<void>`, `data` zeroed on failure. `make` is not part
  of it: a secretbox is made from a key, a crypto_box from two X25519 keys.

| Type | Concept |
|---|---|
| `ciphers::gcm<Cipher>` (`aes128_gcm`, `aes256_gcm`) | `streaming_aead` |
| `ciphers::chacha20_poly1305`, `chacha20_poly1305_original`, `xchacha20_poly1305` | `aead` |
| `ciphers::secretbox`, `secretbox_xchacha20poly1305` | `authenticated_box` |
| `hybrid::box`, `box_xchacha20poly1305` | `authenticated_box` |

Each header checks its row with a `static_assert`. secretbox leaves the `ciphers::aead` template:
it becomes `ciphers::basic_secretbox<Family, B>` with `make(key)`, `seal(iv, data)` and
`open(iv, data, tag)`, the guards and the zeroing of the AEAD template, and no additional data;
`detail::aead_family<secretbox...>` and its `max_additional_data_size` 0 are removed.

The free forms: `crypto::seal(c, iv, additional_data, data)` and
`crypto::open(c, iv, additional_data, data, tag)` constrained by `aead`, and `crypto::seal(c, iv,
data)` and `crypto::open(c, iv, data, tag)` constrained by `authenticated_box`; the arities differ,
so no call is ambiguous.

### D6. Asynchronous seal and open everywhere native

`async_seal` and `async_open`, member (`c.async_seal(executor, ...)`) and free
(`crypto::async_seal(executor, c, ...)`), with the synchronous arguments between the executor and
the token, exist for every `aead` and every `authenticated_box`. On a synchronous backend they run
the synchronous member and complete through `asio::post`, never inline, as the family does today.
On `backends::browser` (a backend whose `detail::asynchronous_backend_v` is true), only AES-GCM is
offered, through WebCrypto; any other stops the build with "webcpp::crypto: WebCrypto has no
<algorithm>; async_seal and async_open of <algorithm> run on a native backend". This supersedes the
foundation's sentence that no `async_` variant of a cheap algorithm WebCrypto lacks may exist, for
seal and open only, by the owner's decision; `async_make` follows the same rule for the types
these concepts cover.

### D7. Removed, and callers

Removed with no alias: the span `verify` and `recover` overloads of crypto/sign-unified; `sign`'s
array results; `curves::x25519::public_key(secret)`, `curves::x25519::agree(secret, peer)`;
`ed25519::basic_keypair::x25519_secret()`, `curves::ed25519::to_x25519_public`;
`kx_keypair::make(seed)` (now `derive_from_seed`); the boxes'
byte-key `make`, `seal` and `open`; secretbox's additional data. Every caller in libs/crypto moves
in the same change: tests, examples, the page, the constant-time harnesses and suppressions, the
benches, the message tests, the MSVC tests, `doc/tables/crypto.json` and its generated files
(rerun, never edited). The X25519 format functions keep their names here and leave with the
formats change, which gives them the typed keys' formats.

## Rulings

- **R1 (owner decision 6): `verify` takes the typed signature only; no bytes overload.** A signature from the wire is
  made first (`p256::ecdsa_signature::make(bytes)` or `crypto::make<T>(bytes)`), where its size is
  checked; then the type says its curve and scheme, and a mismatch is a compile-time error rather
  than an `invalid_signature` at run time. One form keeps the four mirrored forms one, and matches
  crypto/sign-unified's R17 for public keys. BIP340's x-only key bytes keep their `verify` overload:
  that is a key, not a signature.
- **R2: a signature's `make` checks the size only** (D1), so that a high-S or out-of-range
  signature from a peer can still be made, normalized and refused by `verify` with the policy of
  its curve.
- **R3: an X25519 public key's `make` checks the size only** (D2), as Ed25519's does; small order
  is `agree`'s refusal, `weak_public_key`, where RFC 7748 puts it.
- **R4: conversions and value construction are synchronous only, member and free**: `make` of a
  signature or a public key and `to_x25519` call no backend that WebCrypto could replace (WebCrypto
  converts nothing between Ed25519 and X25519), so "four mirrored forms" applies to the operations
  that call a backend (`make` and `generate` of a key pair, `sign`, `verify`, `agree`, `seal`,
  `open`), as crypto/sign-unified's D9 table lists them.
- **R5 (owner decision 7): the seeded derivation is `kx_keypair::derive_from_seed(seed)`**, never a
  `make`, so that `make` means "the secret" everywhere in the library (D3).
- **R6: sealed boxes stay out of the concepts** (Non-goals) but take typed keys (D3).
- **R7: the concepts live in `<webcpp/crypto/aead.hpp>`**, beside `keypair.hpp` and `signing.hpp`,
  which every cipher header includes; it includes no backend.

## Risks / Trade-offs

- Every signing caller changes its result type; the migration is mechanical (`.bytes()` where bytes
  were used) and the build finds every site.
- Programs that verify wire signatures gain a `make` call (R1).
- Async seal and open on native backends add instantiations only where called.

## Migration Plan

crypto/sign-unified merges; then this change; then crypto's formats; then phase 3 of webcpp::trystero's port of
Trystero 0.27. Branches `crypto/uniform-types` of webcpporg/crypto, webcpporg/proposals and the
superproject; local verification on every lane; merged by the controller; one CI at the end.

## Open Questions

None: the owner's decisions 1 to 7 settle the change.
