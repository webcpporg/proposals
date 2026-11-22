## Why

webcpp::crypto signs with three shapes today. P-256's synchronous key pair signs a digest
(`p256::basic_keypair::sign_ecdsa(std::span<std::uint8_t const, 32> digest)`, the free
`p256::verify_ecdsa(signature, digest, key)`), while `crypto::async_sign` and `async_verify` of
`<webcpp/crypto/async.hpp>` take the message and hash it with SHA-256 inside, as WebCrypto's
ECDSA does. Ed25519 signs the message (`sign(message)`, `verify(public_key, message,
signature)`). secp256k1 names each scheme in its functions (`sign_ecdsa`, `sign_schnorr`,
`sign_recoverable`, `verify_ecdsa`, `verify_schnorr`) and signs ECDSA over a 32-byte hash.

A program that moves a call from the synchronous form to the asynchronous one, or from one curve
to another, changes what it must pass: the same 32 bytes are a message in one call and a digest
in the next, and nothing in the types tells them apart. A 32-byte message given to a function
that expects a digest is signed without being hashed, and the signature verifies under the same
mistake: a silent protocol break.

On 2026-11-22 the owner chose option C (design.md, "Approaches"): one name per operation, `sign`
and `verify`, the same inputs synchronously and asynchronously, a digest accepted only through an
explicit marker; the same day the owner brought secp256k1 into the change and settled the final
form: the scheme is always named, by a tag that names its hash where it has one.

## What Changes

- **Scheme tags** in a new header `<webcpp/crypto/signing.hpp>`: `crypto::eddsa` (Ed25519,
  RFC 8032's pure form), `crypto::ecdsa_sha256` (P-256 and secp256k1),
  `crypto::ecdsa_sha256_recoverable` (secp256k1) and `crypto::schnorr` (BIP340, secp256k1),
  constexpr objects of empty types. The ECDSA tag names its hash, as WebCrypto's
  `{name: "ECDSA", hash: "SHA-256"}` does, so that a later `ecdsa_sha384` (P-384) or an
  Ed25519ph tag joins without changing a call.
- **Inputs**: a message, `std::span<std::uint8_t const>`, or a digest marked as one,
  `crypto::prehashed{digest}`, which wraps a `std::span<std::uint8_t const, N>` (deduced from a
  `std::array`, a fixed-size span or a Boost.Hash2 digest); the scheme fixes the hash, and so
  requires N to be its digest size.
- **One form per operation, the scheme always named**: `key.sign(scheme, input)` returns
  `result<std::array<std::uint8_t, N>>`; the free `crypto::verify(scheme, signature, input,
  public_key)` returns `result<void>`; `crypto::async_sign(executor, key, scheme, input, token)`
  and `crypto::async_verify(executor, scheme, signature, input, public_key, token)`.
- **The valid pairs**: `eddsa` with a message; `ecdsa_sha256` and `ecdsa_sha256_recoverable` with
  a message (SHA-256 inside) or a 32-byte `prehashed`; `schnorr` with a message. Every other pair
  (a scheme the key's curve does not have, `eddsa` or `schnorr` with `prehashed`, a digest of
  another size) stops the build, checked by the public concepts `crypto::can_sign` and
  `crypto::can_verify`, with a message that names the reason. A backend that cannot sign a digest
  (the browser's, which part B adds) refuses `prehashed` the same way.
- **Removed, with no shim** (crypto has no release): Ed25519's `sign(message)` and free
  `curves::ed25519::verify`; P-256's `sign_ecdsa` and `verify_ecdsa`; secp256k1's `sign_ecdsa`
  (both forms), `sign_schnorr`, `sign_recoverable`, `verify_ecdsa` and `verify_schnorr`. Every
  caller in libs/crypto (tests, examples, the page, the constant-time harnesses, the benches, the
  message and MSVC tests) moves to the new form.
- **ECDSA signs low S on both curves**: `sign(ecdsa_sha256, ...)` and
  `sign(ecdsa_sha256_recoverable, ...)` return s at most (n - 1) / 2 on P-256 too (P-256's
  RFC 6979 signature is normalized after signing); verification is unchanged: secp256k1 refuses
  a high S (libsecp256k1, Bitcoin's policy), P-256 accepts both (WebCrypto's signatures may be
  high).
- **The signature's format helpers keep the curve's namespace and get one set of names**:
  `to_der`, `from_der`, `normalize_low_s` (was `normalize_ecdsa`) and `is_low_s` (was P-256's
  `check_low_s`; new on secp256k1), the same on both curves. The 64-byte r || s (IEEE P1363,
  WebCrypto's form) stays the signature; the recoverable form keeps its id and `recover`.
- **A Bitcoin legacy or segwit v0 sighash (SHA-256d) stays out of crypto**: a program computes it
  with webcpp::hash and signs `prehashed{sighash}` under `ecdsa_sha256`; a new example shows it.
- **secp256k1's extras keep their reason in the same name**: hedged ECDSA as
  `sign(ecdsa_sha256, input, extra_entropy)`, BIP340 with the caller's auxiliary randomness as
  `sign(schnorr, message, aux_random)`; `recover(ecdsa_sha256_recoverable, signature, input)`.
  secp256k1 stays out of the asynchronous family, whose rule is WebCrypto's algorithms.

Not changed: DER, key formats, ECDH, MuSig2's session
functions and silent payments, Ed25519ph's incremental signer (except its `verify`'s argument
order), any backend's library.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: signing and verification have one name per operation, the scheme named at every
  call, and one set of inputs on every curve, synchronously and asynchronously; a digest is
  accepted only when marked.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** yes, before the first release; webcpp::trystero's port of Trystero 0.27
  (stream trystero-027) adapts after this change merges.
- **Implementation:** webcpporg/crypto (`include/webcpp/crypto/signing.hpp`, `curves/p256.hpp`,
  `curves/ed25519.hpp`, `curves/secp256k1.hpp`, `async.hpp`, tests, examples, benches, the
  constant-time lane, `doc/`, `doc/tables/crypto.json` and the generated files), the
  superproject's pointer.
