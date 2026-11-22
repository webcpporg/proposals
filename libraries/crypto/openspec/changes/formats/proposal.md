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
(`key.sign(crypto::ecdsa_sha256, message)`), typed public keys and the `crypto::keypair`
contract. On 2026-11-22 the owner decided that formats follow the same shape: the format is a
value passed first, `crypto::encode(format, value)` and `crypto::decode<T>(format, input)` are
the only names, every invalid pair stops the build with the formats the type does take, and
JWK, which WebCrypto, JOSE and Trystero 0.27 use, joins the family.

## What Changes

- **Format tags as values** in a new header `<webcpp/crypto/formats.hpp>`: `crypto::raw`,
  `crypto::spki`, `crypto::pkcs8`, `crypto::der` (an ECDSA signature), `crypto::jwk`, and
  `crypto::pem(F)`, which composes over `spki` or `pkcs8` and takes its label from the inner
  format; point forms as modifiers, `crypto::raw(crypto::compressed)`,
  `crypto::spki(crypto::compressed)`, `crypto::raw(crypto::xonly)`. One list of formats,
  `detail::all_formats`, from which the concepts, the tests and the guide's table derive.
- **Four mirrored forms, one implementation**: `crypto::encode(format, value)`,
  `crypto::decode<T>(format, input)`, `value.encode(format)` and `T::decode(format, input)`;
  plus `crypto::encode(format, value, output)` into a caller's span.
- **No heap**: `encode` returns `result<crypto::encoded<N, Form>>`, a fixed buffer whose `N` is
  the format's maximum for that type at compile time, with its length, `bytes()` and, for PEM
  and JWK, `text()`; a private key's encoding wipes itself as `secret<N>` does.
  `crypto::max_encoded_size<Format, T>` replaces every `max_*_size` constant.
- **Concepts** `crypto::encodes<Format, T>` and `crypto::decodes<Format, T>` gate every pair;
  a refused pair stops the build with a message and the type's list of formats.
- **The valid pairs**: public keys take `raw`, `spki`, `pem(spki)` and `jwk`; key pairs take
  `pkcs8`, `pem(pkcs8)` and `jwk` (private, with `d`), and `raw`, which is `secret()` and
  `make` under the format's name; ECDSA signatures take `raw` and `der`; Ed25519, Schnorr and
  recoverable signatures take `raw` only.
- **Typed signatures**: `sign` returns `p256::ecdsa_signature`, `secp256k1::ecdsa_signature`,
  `secp256k1::recoverable_signature`, `secp256k1::schnorr_signature` or `ed25519::signature`,
  each holding its bytes and converting to the fixed-size span `crypto::verify` takes, so that
  `der` knows the curve and Ed25519 refuses it by type.
- **JWK** (RFC 7517, 7518, 8037, 8812): EC P-256 and secp256k1 (`x`, `y`, `d`), OKP Ed25519
  (`x`, `d`), base64url without padding, in its own header `<webcpp/crypto/jwk.hpp>` and Jamfile
  target `/webcpp/crypto//jwk`, which brings webcpp::document; the core of crypto never depends on
  document. Encoding writes RFC 7638's canonical member order with no white space into a fixed
  buffer (P-256's private JWK is 176 bytes); `crypto::encode<Document>(crypto::jwk, value)`
  builds a document in the storage the program picks. Decoding a text runs document's streaming
  parser over a fixed working buffer, never the heap; decoding a document's value (a JWK Set's
  element, a JWT header's member) allocates nothing. `crypto::thumbprint(key)` is RFC 7638's
  SHA-256 thumbprint, on webcpp::hash.
- **Removed, with no alias** (crypto has no release): `encode_spki`, `decode_spki`,
  `encode_pkcs8`, `decode_pkcs8` on every curve that joins, `to_der`, `from_der`,
  `max_spki_size`, `max_pkcs8_size`, `max_der_size`, `spki_compressed_size`,
  `spki_uncompressed_size`, and the namespace `crypto::pem` (`armour`, `unarmour`, its labels
  and `armoured_size`), whose name the PEM format takes. Every caller in libs/crypto moves.
- **The foundation's "JWK MUST NOT be offered" is withdrawn** by the owner's decision.
- **X25519 stays out** until its keys are typed (an open owner question); its four format
  functions keep their names until then, the one exception to the removal.

Not changed: what each format's bytes are (every SPKI, PKCS#8, DER and PEM is byte for byte the
foundation's), strict decoding (only what encoding gives again is accepted), the backends, the
timing guard rule, `normalize_low_s` and `is_low_s` (which take the typed ECDSA signature).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: keys and signatures are written and read by one pair of names with the format as a
  value, every invalid pair refused at compile time; JWK is offered, in a target of its own.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** yes, before the first release; it builds on crypto/sign-unified and is
  implemented after that change merges and before phase 3 of webcpp::trystero's port of
  Trystero 0.27 (stream trystero-027), which adapts to it.
- **Implementation:** webcpporg/crypto (`include/webcpp/crypto/formats.hpp`, `jwk.hpp`, each
  curve's `formats.hpp`, `curves/*.hpp` for the typed signatures and the members,
  `build.jam` for the target `jwk`, tests, fuzz targets, examples, benches, the constant-time
  lane, `doc/`, `doc/tables/crypto.json` and the generated files), the superproject's pointer.
- **Status:** proposed: the owner's decisions are recorded in `design.md`; its rulings and open
  questions await the owner's review before the change is accepted.
