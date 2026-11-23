## Why

webcpp::crypto writes and reads its keys and signatures through one function per format and per
curve: `curves::p256::encode_spki`, `decode_spki`, `encode_pkcs8`, `decode_pkcs8`, the same four
on Ed25519, X25519 and secp256k1, `to_der` and `from_der` on both ECDSA curves, and the root's
`pem::armour` and `pem::unarmour`, which a caller combines by hand with a label it must spell
right (`pem::public_key_label` around an SPKI, `pem::private_key_label` around a PKCS#8) and a
buffer it must size from a constant of the right curve (`max_spki_size`, `max_pkcs8_size`,
`max_der_size`). Every format is a name to learn per curve, every pair of format and value that
does not exist is a missing name rather than a diagnostic, and the PEM label is a string the
compiler cannot check.

The crypto/sign-unified change gave signing one name per operation with the scheme as a value
(`key.sign(crypto::ecdsa_sha256, message)`) and typed public keys; the uniform-types change gives
every signature and X25519's keys a type of their own. On 2026-11-22 the owner decided that
formats follow the same shape: the format is a value passed first, `crypto::encode(format, value)`
and `crypto::decode<T>(format, input)` are the only names, and every invalid pair stops the build
with the formats the type does take. JWK, which WebCrypto, JOSE and Trystero 0.27 use, was in the
first drafts; the owner deferred it the same day, after webcpp::json_schema gains document support.

## What Changes

- **Format tags as values** in a new header `<webcpp/crypto/formats.hpp>`: `crypto::raw`,
  `crypto::spki`, `crypto::pkcs8`, `crypto::der` (an ECDSA signature), and `crypto::pem(F)`, which composes over `spki` or `pkcs8` and takes its label from the inner
  format; point forms as modifiers, `crypto::raw(crypto::compressed)`,
  `crypto::spki(crypto::compressed)`, `crypto::raw(crypto::xonly)`. One list of formats,
  `detail::all_formats`, from which the concepts, the tests and the guide's table derive, built so
  that `crypto::jwk` joins later as one more tag with no interface change.
- **Four mirrored forms, one implementation**: `crypto::encode(format, value)`,
  `crypto::decode<T>(format, input)`, `value.encode(format)` and `T::decode(format, input)`;
  plus `crypto::encode(format, value, output)` into a caller's span. The generic header includes no
  backend; each curve's `curves/<curve>/formats.hpp` holds its codecs and defines its members.
- **No heap**: `encode` returns `result<crypto::encoded<N, Form>>`, a fixed buffer whose `N` is
  the format's maximum for that type at compile time, with its length, `bytes()` and, for PEM,
  `text()`; a private key's encoding wipes itself as `secret<N>` does.
  `crypto::max_encoded_size<Format, T>` replaces every `max_*_size` constant.
- **Concepts** `crypto::encodes<Format, T>` and `crypto::decodes<Format, T>` gate every pair;
  a refused pair stops the build with a message and the type's list of formats.
- **The valid pairs**, on Ed25519, X25519, P-256 and secp256k1: public keys take `raw`, `spki`,
  `pem(spki)`; key pairs take `pkcs8`, `pem(pkcs8)` and `raw`, which is `secret()` and `from_secret` under the format's name; ECDSA signatures take `raw` and
  `der`; Ed25519, Schnorr and recoverable signatures take `raw` only. The uncompressed point is
  the default of `raw` and `spki` on every EC curve (secp256k1's SPKI default changes), the
  compressed point always available by name, on every backend.
- **JWK is deferred** ("Deferred: JWK" in `design.md`): it comes after webcpp::json_schema's
  genesis and its move onto webcpp::document, so that a JWK's shape and limits are a JSON Schema
  that document applies while parsing; what the first drafts designed for it is recorded there.
- **Removed, with no alias** (crypto has no release): `encode_spki`, `decode_spki`,
  `encode_pkcs8`, `decode_pkcs8` on every curve, X25519's included, `to_der`, `from_der`,
  `max_spki_size`, `max_pkcs8_size`, `max_der_size`, `spki_compressed_size`,
  `spki_uncompressed_size`, and the namespace `crypto::pem` (`armour`, `unarmour`, its labels
  and `armoured_size`), whose name the PEM format takes. Every caller in libs/crypto moves.

Not changed: what each format's bytes are (every SPKI, PKCS#8, DER and PEM of an uncompressed key
is byte for byte the foundation's), strict decoding (only what encoding gives again is accepted),
the backends, the timing guard rule.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: keys and signatures are written and read by one pair of names with the format as a
  value, every invalid pair refused at compile time.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** yes, before the first release. Order: crypto/sign-unified merges; then
  crypto's uniform-types; then this change; then phase 3
  of webcpp::trystero's port of Trystero 0.27 (stream trystero-027), which adapts to it.
- **Implementation:** webcpporg/crypto (`include/webcpp/crypto/formats.hpp`, each curve's
  `formats.hpp`, tests, fuzz targets, examples,
  benches, the constant-time lane, `doc/`, `doc/tables/crypto.json` and the generated files), the
  superproject's pointer.
- **Status:** approved by the owner on 2026-11-22 (`decided`); it stays `proposed` in the
  metadata until `reference/plan.md` carries the plan's code, which `tools/proposals.py` requires
  of an `accepted` proposal, and then becomes `accepted`.
