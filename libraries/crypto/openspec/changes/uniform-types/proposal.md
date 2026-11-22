## Why

crypto/sign-unified gave every curve typed public keys, one key pair contract and one name per
operation, but three kinds of value are still bare bytes or ad hoc shapes:

- **Signatures** are `std::array<std::uint8_t, 64>` (65 for a recoverable one) on every curve and
  scheme, and `verify` takes any `std::span<std::uint8_t const>`: a P-256 signature passed to
  secp256k1, or an Ed25519 signature given to a DER encoder, is a run-time failure at best, and the
  formats change (`libraries/crypto/openspec/changes/formats/`) cannot refuse `der` on Ed25519 or
  know an ECDSA signature's curve.
- **X25519** has no types at all: its public key is `std::array<std::uint8_t, 32>`, its secret a
  `secret<32>`, its operations free functions (`curves::x25519::public_key(secret)`,
  `curves::x25519::agree(secret, peer)`), while Ed25519, P-256 and secp256k1 have key pairs with
  `make`, `generate`, `public_key()`, `secret()` and `agree`. crypto_kx, the boxes and Ed25519's
  conversion (`x25519_secret()`, `to_x25519_public`) pass the same raw bytes.
- **Contracts**: the one concept `crypto::keypair` requires `sign`, which an X25519 key pair has
  not; the AEADs and boxes share a shape that no concept states, and secretbox carries additional
  data it refuses (a "fake" parameter that must be empty).

On 2026-11-22 the owner decided to type all three in one small change merged before formats.

## What Changes

- **Typed signatures**: `ed25519::signature`, `p256::ecdsa_signature`,
  `secp256k1::ecdsa_signature`, `secp256k1::recoverable_signature`,
  `secp256k1::schnorr_signature`, each made by `make(bytes)` (its size checked) and giving
  `bytes()`; every `sign` returns one, and every `verify` and `recover` takes one, synchronous and
  asynchronous, member and free. The bytes overload of crypto/sign-unified (`verify` taking any
  span and checking its size) is replaced: a signature from the wire is made first.
- **X25519 typed like the others**: `x25519::public_key` (`make`, `bytes()`),
  `x25519::basic_keypair<B>` (`make`, `generate`, `public_key()`, `secret()`, `agree(peer)`,
  `async_agree`, free `crypto::agree` and `crypto::async_agree`); `kx_keypair` built on it; the
  boxes take typed keys. The free `curves::x25519::public_key(secret)` and `agree(secret, peer)`
  are removed.
- **Ed25519 to X25519 as a typed conversion**: `ed_key.to_x25519()` gives an
  `x25519::basic_keypair<B>`, `ed_public_key.to_x25519()` an `x25519::public_key`, with the free
  `crypto::to_x25519`; `x25519_secret()` and `curves::ed25519::to_x25519_public` are removed.
- **Three key pair concepts**: `crypto::keypair` (`make`, `generate`, `public_key()`, `secret()`),
  `crypto::signing_keypair` (also `sign` for each scheme it declares, at least one) and
  `crypto::agreement_keypair` (also `agree`). Ed25519 signs; P-256 and secp256k1 sign and agree;
  X25519 agrees.
- **AEAD and box concepts**: `crypto::aead` (sizes, `result_type`, `make(key)`,
  `seal(iv, ad, data)`, `open(iv, ad, data, tag)`), its refinement `crypto::streaming_aead`
  (AES-GCM's incremental seal and open), and the sibling `crypto::authenticated_box`
  (`seal(iv, data)`, `open(iv, data, tag)`) for secretbox and crypto_box, which lose the additional
  data they refused. `crypto::seal`, `crypto::open` and the asynchronous forms are constrained by
  them, and `async_seal` and `async_open` extend to every AEAD and box on native backends; the
  browser backend keeps WebCrypto's AES-GCM only, refusing the others with a clear message.
- **Four mirrored forms** for every new operation that calls a backend.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: every signature and every X25519 key has a type of its own; key pairs, AEADs and boxes
  are checked by concepts; secretbox and crypto_box take no additional data; seal and open are
  asynchronous on every native backend.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** yes, before the first release. Order: crypto/sign-unified merges; then this change;
  then crypto's formats; then phase 3 of
  webcpp::trystero's port of Trystero 0.27, which adapts to both.
- **Implementation:** webcpporg/crypto (`signing.hpp`, `keypair.hpp`, `curves/*.hpp`,
  `curves/x25519/kx.hpp`, `ciphers/*.hpp`, `hybrid/*.hpp`, `async.hpp`, a new `aead.hpp` concept
  header, tests, examples, benches, the constant-time lane, `doc/`, `doc/tables/crypto.json` and
  the generated files), the superproject's pointer.
- **Status:** approved by the owner on 2026-11-22 (`decided`); it stays `proposed` in the
  metadata until `reference/plan.md` carries the plan's code, which `tools/proposals.py` requires
  of an `accepted` proposal, and then becomes `accepted`.
