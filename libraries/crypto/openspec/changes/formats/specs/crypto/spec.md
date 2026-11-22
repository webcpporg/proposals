## ADDED Requirements

### Requirement: Formats: tags as values and one list
`<webcpp/crypto/formats.hpp>` SHALL declare, in namespace `webcpp::crypto`, the format tags `raw`, `spki`, `pkcs8`, `der`, `jwk` and `pem`, constexpr objects of empty types whose default constructors are explicit; the point form modifiers `compressed`, `uncompressed` and `xonly`, taken by `raw(form)` and `spki(form)`; `pem(inner)` for an `inner` that is `spki` (with any form) or `pkcs8`, whose label is `PUBLIC KEY` for `spki` and `PRIVATE KEY` for `pkcs8`, taken from the inner format and never from the caller; and one list of formats, `detail::all_formats`, from which `detail::is_format_v`, the concepts, the tests and the guide's table of valid pairs derive. `pem` of any other format MUST stop the build, and no function MUST take a PEM label from its caller.

#### Scenario: The label follows the inner format
- **WHEN** a P-256 public key is encoded with `crypto::pem(crypto::spki)` and a P-256 key pair with `crypto::pem(crypto::pkcs8)`
- **THEN** the texts begin with `-----BEGIN PUBLIC KEY-----` and `-----BEGIN PRIVATE KEY-----`, and each is byte for byte what OpenSSL's `PEM_write_PUBKEY` and `PEM_write_PrivateKey` write for the same key

#### Scenario: PEM of a format that is not DER
- **WHEN** a program writes `crypto::pem(crypto::jwk)` or `crypto::pem(crypto::der)`
- **THEN** compilation stops with a message saying that PEM carries `pem(crypto::spki)` or `pem(crypto::pkcs8)`

### Requirement: Formats: four mirrored forms and a fixed result
The library SHALL offer `crypto::encode(format, value)`, returning `boost::system::result<crypto::encoded<N, Form>>`; `crypto::encode(format, value, output)`, writing into a caller's `std::span<std::uint8_t>` (binary formats) or `std::span<char>` (text formats) and returning `result<std::size_t>`; `crypto::decode<T>(format, input)`, returning `result<T>`, `input` a `std::span<std::uint8_t const>` for `raw`, `spki`, `pkcs8` and `der`, a `std::string_view` for `pem(F)` and `jwk`; and the members `value.encode(format)` and `T::decode(format, input)`, with one implementation behind all four. `encoded<N, Form>` MUST hold its bytes in a fixed array of `N = crypto::max_encoded_size<Format, T>` elements, the format's maximum for the type computed at compile time, with its length, `bytes()` on every form and `text()` on text forms only; the encodings of a private key (`pkcs8`, `pem(pkcs8)`, a key pair's `jwk` and `raw`) MUST be move-only and wiped on destruction and on move, as `secret<N>` is. No encode or decode MUST allocate from the heap, except that encoding a JWK into a document allocates in that document's storage. The span form MUST refuse an output shorter than `max_encoded_size` with `invalid_input_size` and write nothing on any error.

#### Scenario: The same bytes from every form
- **WHEN** a secp256k1 public key is encoded with `crypto::encode(crypto::spki, key)`, `key.encode(crypto::spki)` and `crypto::encode(crypto::spki, key, output)`
- **THEN** the three give the same 88 bytes, and `crypto::decode<secp256k1::public_key>(crypto::spki, bytes)` and `secp256k1::public_key::decode(crypto::spki, bytes)` give the key back

#### Scenario: No heap
- **WHEN** every valid pair of the format table is encoded and decoded under a counting `operator new`, JWK text included
- **THEN** the count is zero

#### Scenario: A private key's encoding is wiped
- **WHEN** the `encoded` of a P-256 key pair's `pkcs8` is moved from and destroyed
- **THEN** a test's inspection of its storage finds every byte zero

### Requirement: Formats: the valid pairs, checked by concepts
The concepts `crypto::encodes<Format, T>` and `crypto::decodes<Format, T>` SHALL be true exactly for these pairs, and every other pair MUST stop the build in a `static_assert` whose message says the type has no such format and whose failed requirement names the type's list of formats:

| Type | Formats | `max_encoded_size` |
|---|---|---|
| `ed25519::public_key` | `raw`, `spki`, `pem(spki)`, `jwk` | 32, 44, 113, 79 |
| `p256::basic_public_key<B>` | `raw`, `spki`, `pem(spki)`, `jwk` | 65, 91, 178, 126 |
| `secp256k1::public_key` | `raw` (also `compressed`, `xonly`), `spki` (also `compressed`), `pem(spki)` (also `compressed`), `jwk` | 65 (33, 32), 88 (56), 174 (130), 130 |
| `ed25519::basic_keypair<B>` | `raw` (its `secret()`), `pkcs8`, `pem(pkcs8)`, `jwk` with `d` | 32, 48, 119, 129 |
| `p256::basic_keypair<B>` | `raw` (its `secret()`), `pkcs8`, `pem(pkcs8)`, `jwk` with `d` | 32, 138, 241, 176 |
| `secp256k1::basic_keypair<B>` | `raw` (its `secret()`), `pkcs8`, `pem(pkcs8)`, `jwk` with `d` | 32, 135, 237, 180 |
| `p256::ecdsa_signature`, `secp256k1::ecdsa_signature` | `raw`, `der` | 64, 72 |
| `secp256k1::recoverable_signature` | `raw` | 65 |
| `secp256k1::schnorr_signature`, `ed25519::signature` | `raw` | 64 |

A key pair's `raw` MUST be its `secret()` on encoding and its `make` on decoding, with no second implementation. The default point form of `raw` and `spki` MUST be the uncompressed point on every EC curve; `compressed` MUST be refused at compile time on a backend that has no compressed form. On a backend whose keys live in WebCrypto (`detail::asynchronous_backend_v`), the private formats MUST be refused at compile time.

#### Scenario: Ed25519 and DER
- **WHEN** a program calls `crypto::encode(crypto::der, signature)` with an Ed25519 signature
- **THEN** compilation stops with a message saying that Ed25519 and BIP340 signatures have no DER form, and the failed requirement names the signature's list, which holds `raw` alone

#### Scenario: A key pair's raw is its secret
- **WHEN** a P-256 key pair `k` is encoded with `crypto::raw`
- **THEN** the bytes equal `k.secret()->bytes()`, and `crypto::decode<p256::keypair>(crypto::raw, bytes)` has `k`'s public key

#### Scenario: A list in one place
- **WHEN** the test of the format table iterates `detail::all_formats` over every type
- **THEN** `encodes` is true exactly for the table's pairs, and the guide's table is generated from the same list

### Requirement: Formats: typed signatures
Every `sign` SHALL return a typed signature: `p256::ecdsa_signature` and `secp256k1::ecdsa_signature` (64 bytes, r then s), `secp256k1::recoverable_signature` (65 bytes), `secp256k1::schnorr_signature` and `ed25519::signature` (64 bytes), each made by `make(bytes)` (its size checked, nothing else), giving `bytes()`, and converting implicitly to the `std::span<std::uint8_t const, N>` that `crypto::verify` and `recover` take. `normalize_low_s` and `is_low_s` MUST take the curve's `ecdsa_signature`. `crypto::encode(crypto::der, signature)` MUST write strict DER of 8 to 72 bytes, refusing r or s 0 or not below n with `invalid_signature`, and `crypto::decode<ecdsa_signature>(crypto::der, bytes)` MUST accept only the DER that encoding gives again.

#### Scenario: Verification unchanged
- **WHEN** a P-256 key pair signs a message and the typed signature is passed to `crypto::verify(crypto::ecdsa_sha256, signature, message, public_key)`
- **THEN** it verifies with no conversion at the call, and so do the signature's bytes from the wire

#### Scenario: A non-canonical DER signature
- **WHEN** `crypto::decode<p256::ecdsa_signature>(crypto::der, bytes)` is given a Wycheproof case whose DER has a leading zero byte it does not need
- **THEN** it returns `invalid_signature`

### Requirement: JWK: its header, its target and its text
`<webcpp/crypto/jwk.hpp>` and the Jamfile target `/webcpp/crypto//jwk` SHALL offer JWK (RFC 7517, RFC 7518 section 6.2, RFC 8037, RFC 8812) for Ed25519 (`kty` `"OKP"`, `crv` `"Ed25519"`, `x`, and `d` for a key pair), P-256 (`kty` `"EC"`, `crv` `"P-256"`, `x`, `y`, and `d`) and secp256k1 (`kty` `"EC"`, `crv` `"secp256k1"`, `x`, `y`, and `d`), each value base64url without padding of 32 bytes. The target MUST bring webcpp::document, and `/webcpp/crypto//crypto` and every header but `jwk.hpp` MUST build without document. Encoding a JWK MUST write one JSON object with no white space, its members in lexicographic order of their names (`crv`, `d`, `kty`, `x`, `y`) and no other member, into the fixed result, with no heap; the public JWK's text MUST be RFC 7638's thumbprint input. A JWK pair called without `jwk.hpp` MUST stop the build with a message naming the header and the target. JWK's base64url MUST run on the PEM family's constant-time backend.

#### Scenario: WebCrypto reads the text
- **WHEN** node's WebCrypto imports the JWK text of a P-256 key pair (176 bytes) and of its public key (126 bytes) with `importKey("jwk", ...)`, signs and verifies
- **THEN** it accepts both, its signature verifies under `crypto::verify`, and its `exportKey("jwk")` of the public key, with `key_ops` and `ext`, decodes to the same public key

#### Scenario: RFC 8037's example
- **WHEN** the Ed25519 key pair of RFC 8037 appendix A.1 is encoded with `crypto::jwk`
- **THEN** the members are the appendix's, and `crypto::thumbprint` of its public key is appendix A.3's

### Requirement: JWK: decoding a text in bounded working memory
`crypto::decode<T>(crypto::jwk, text)` SHALL parse with webcpp::document's streaming parser over a working buffer of `crypto::jwk_working_size` bytes on the stack, and `crypto::decode<T>(crypto::jwk, text, working)` over the caller's span: the document's image in one region through external storage, the parser's working state in a `std::pmr::monotonic_buffer_resource` over the other with `std::pmr::null_memory_resource()` upstream, the whole text given at once so that nothing is allocated in the global heap, and `parse_options::max_bytes` set so that no accepted text exhausts the working state. One selector MUST receive every member of the root and take it out of the document; the values of `kty`, `crv`, `x`, `y` and `d` MUST be copied into fixed buffers, and every other member ignored. The decoder MUST refuse with `invalid_encoding` a root that is not an object, any of the five members present twice or not a string, a `kty` or `crv` that is not the type's, a coordinate that is not strict base64url of 32 bytes, a key pair without `d`, a public key with `d`, a key pair whose `x` and `y` are not `d`'s public key, and a point or scalar its curve refuses; and with `jwk_too_large` a text the working buffer or `max_bytes` cannot hold. Every byte that held `d` MUST be wiped before the call returns.

#### Scenario: A duplicate member
- **WHEN** a P-256 public JWK has `"x"` twice
- **THEN** the decode returns `invalid_encoding`, and with a second `"kid"` instead it decodes

#### Scenario: A large unknown member
- **WHEN** a JWK carries a `"kid"` of 10 KB and is decoded with the default working buffer
- **THEN** it returns `jwk_too_large` without ending the program, in a build with exceptions and in one without, and with a working span large enough it decodes

#### Scenario: The wrong kind of key
- **WHEN** a public JWK is decoded as a key pair, and a private JWK as a public key
- **THEN** both return `invalid_encoding`

### Requirement: JWK: documents and the thumbprint
`crypto::decode<T>(crypto::jwk, value)` SHALL read a `document::basic_const_value<Traits, Storage>` (a JWK Set's element, a JWT header's member) with the rules of the text's decode except duplicates, which a document has already resolved, allocating nothing; `crypto::encode<Document>(crypto::jwk, value, arguments...)` SHALL build a `document::basic_document<Traits, Storage>` in the storage its arguments give, and `crypto::encode(crypto::jwk, value, target)` SHALL set a `document::basic_value` to the JWK object, the members in the text's order so that `stringify()` gives the text. `crypto::thumbprint(key)` SHALL return RFC 7638's SHA-256 thumbprint, `result<std::array<std::uint8_t, 32>>`, of a public key or of a key pair's public key, computed with webcpp::hash's `sha2_256` over the public JWK's text.

#### Scenario: A JWK Set's element
- **WHEN** a document holding a JWK Set is parsed and its `keys[1]`, a P-256 public JWK, is decoded under a counting `operator new` and a counting resource of the document
- **THEN** it gives the key, and both counts are zero

#### Scenario: A document's text
- **WHEN** a secp256k1 key pair is encoded into a `document::pmr::document` and stringified
- **THEN** the text equals `crypto::encode(crypto::jwk, key)->text()`

### Requirement: Formats: every decoder is fuzzed
Every decoder of this change SHALL have a fuzz target from the commit that adds it, one per format and type family (`raw`, `spki`, `pkcs8`, `der`, `pem(spki)`, `pem(pkcs8)`, JWK text, JWK document value), each asserting that nothing crashes, nothing is allocated from the heap, what is accepted encodes back to the same input, and the working buffer is wiped, seeded with Wycheproof's, the RFCs' and WebCrypto's vectors, and run for a bounded time in CI.

#### Scenario: A fuzzed JWK
- **WHEN** the JWK text target runs its corpus and a bounded run in CI
- **THEN** it finds no crash, no allocation and no accepted text whose re-encoding differs from the canonical form of the same key

## MODIFIED Requirements

### Requirement: Key formats: SPKI, PKCS#8 and PEM, strict
Each curve's increment SHALL offer its keys through `crypto::encode` and `crypto::decode` with the formats `spki`, `pkcs8`, `pem(spki)` and `pem(pkcs8)` (PEM per RFC 7468: labels `PUBLIC KEY` and `PRIVATE KEY` taken from the inner format, lines of 64 characters, LF), and JWK through `<webcpp/crypto/jwk.hpp>`: Ed25519 per RFC 8410 in increment 1 (on libsodium, RFC 8410's fixed DER prefix and the 32 key bytes, a recorded constant), secp256k1 per RFC 5480 with its OID in increment 2, P-256, P-384 and P-521 per RFC 5480 and RFC 5915 in increment 3 (SPKI with the uncompressed point, 91 bytes for P-256, and PKCS#8 version 0 whose `ECPrivateKey` is version 1 with the `publicKey` and without `parameters`, 138 bytes for P-256, by fixed layouts on every backend of the curve, BearSSL included, `design.md` D36; a PKCS#8 without `publicKey` or with `[0] parameters` refused; the compressed point's SPKI on OpenSSL and Botan), RSA with PKCS #1 inside SPKI and PKCS#8 with RSA, and on OpenSSL and Botan for every key type they have. X25519 keeps `curves::x25519::encode_spki`, `decode_spki`, `encode_pkcs8` and `decode_pkcs8` until its keys are typed (the formats change's R11). The names `encode_spki`, `decode_spki`, `encode_pkcs8`, `decode_pkcs8`, `max_spki_size` and `max_pkcs8_size` of every other curve, and the namespace `crypto::pem`, MUST NOT exist. Decoding MUST accept an encoding only when re-encoding what was decoded gives the same bytes, on every backend, and refuse anything else with `invalid_encoding`. Tests MUST run round trips, malformed-input refusals, and OpenSSL natively and WebCrypto's `importKey` and `exportKey` (`spki`, `pkcs8`, `jwk`) under node as outside oracles; for secp256k1, which WebCrypto lacks, node's `crypto` module (`createPrivateKey`, `createPublicKey`, `export`, its `jwk` format included) is the oracle under node.

#### Scenario: An Ed25519 key from WebCrypto
- **WHEN** node's WebCrypto exports an Ed25519 key pair as `pkcs8` and `spki` and the library decodes both on libsodium with `crypto::decode`
- **THEN** the key pair signs as WebCrypto's does, and encoding it again gives the same bytes

#### Scenario: PKCS#8 version 2 is refused
- **WHEN** `crypto::decode<ed25519::keypair>(crypto::pkcs8, bytes)` is given an Ed25519 PKCS#8 that carries the public key (version 2)
- **THEN** it returns `invalid_encoding`, on libsodium and on OpenSSL alike

#### Scenario: A P-256 key from WebCrypto
- **WHEN** node's WebCrypto exports a P-256 key pair imported from `crypto::encode(crypto::pkcs8, key)`'s bytes as `pkcs8` and its public key as `spki`, and the library decodes both on BearSSL
- **THEN** the exports are exactly the 138 and 91 bytes `crypto::encode` writes, and a PKCS#8 of the same key carrying `[0] parameters` (150 bytes) or without `publicKey` (67 bytes) is refused with `invalid_encoding`

#### Scenario: A libp2p PeerId's key
- **WHEN** a P-256 public key's SPKI DER from OpenSSL is decoded by `crypto::decode<p256::basic_public_key<backends::openssl>>(crypto::spki, bytes)` and re-encoded
- **THEN** the bytes are identical, as a libp2p PeerId of an ECDSA key requires

### Requirement: P-curves: DER signatures and public key forms
Increment 3 SHALL add, as primitives on BearSSL for P-256, P-384 and P-521 by the same code: the typed ECDSA signature of each curve, encoded and decoded with `crypto::der` (encoding into the fixed result of `max_encoded_size` bytes, 72 for P-256, or the caller's span; decoding returning the typed `r || s`; a refusal writing nothing), decoding strict DER only (BearSSL's `br_ecdsa_asn1_to_raw`, then re-encoded by `br_ecdsa_raw_to_asn1` and required to give the same bytes); and the public key type `curves::p256::basic_public_key`, made by `make` from the uncompressed point (`0x04 || x || y`, 65 bytes) or from raw 32-byte `x` and `y` coordinates, refusing a point off the curve or the identity with `invalid_public_key`, and serialised to the uncompressed form by `uncompressed()` and by `crypto::raw`. The compressed form MUST be present only on backends that have it (OpenSSL, Botan), `crypto::raw(crypto::compressed)` refused at compile time on BearSSL. The names `to_der`, `from_der` and `max_der_size` MUST NOT exist. The documentation MUST say these serve WebAuthn's ES256 verification and that COSE parsing and WebAuthn's protocol stay outside crypto. Tests MUST run Wycheproof's `ecdsa_secp256r1_sha256_test.json` (DER) and `ecdsa_secp256r1_sha256_p1363_test.json` (raw), its P-384 and P-521 files of both forms, and NIST's ECDSA signature and public-key validation vectors (ACVP's JSON, from the `acvp` submodule that increment 3's AES step pins, or CAVP's archives once gate O3 pins them); P-256 merges before that submodule and gains them with it (`design.md` D36).

#### Scenario: A non-canonical DER signature
- **WHEN** `crypto::decode<p256::ecdsa_signature>(crypto::der, bytes)` is given a Wycheproof case whose DER has a leading zero byte it does not need
- **THEN** it returns `invalid_signature`

#### Scenario: A COSE key's coordinates
- **WHEN** `public_key` is made from the raw `x` and `y` of a passkey's COSE key and verifies an assertion's signature after `crypto::decode<p256::ecdsa_signature>(crypto::der, ...)`
- **THEN** the verification succeeds, and the same `x` with `y` changed by one is refused as `invalid_public_key`

#### Scenario: A short buffer
- **WHEN** `crypto::encode(crypto::der, signature, output)` is given a 70-byte buffer
- **THEN** it returns `invalid_input_size` and the buffer is untouched, whatever the signature's DER length

### Requirement: Signatures: the format helpers have one set of names
`curves::p256` and `curves::secp256k1` SHALL each offer `normalize_low_s(signature)` (r || n - s when s is above (n - 1) / 2, the signature unchanged otherwise, as the curve's `ecdsa_signature`; `invalid_signature` when r or s is 0 or not below n) and `is_low_s(signature)` (`bool`: true exactly when 0 < r < n and 0 < s <= (n - 1) / 2), each taking the curve's `ecdsa_signature`; DER is the format `crypto::der`. The names `normalize_ecdsa`, `check_low_s`, `to_der` and `from_der` MUST NOT exist. The 64-byte r || s (IEEE P1363, WebCrypto's form) is the signature's `raw` form on both curves.

#### Scenario: The same helper on both curves
- **WHEN** a high-S signature of each curve is given to its curve's `normalize_low_s`
- **THEN** the result is r || n - s for that curve's n, `is_low_s` is false before and true after, and `crypto::encode(crypto::der, ...)` then `crypto::decode` gives the result back

### Requirement: P-256: a key pair from a raw scalar, raw ECDH and low S
Increment 3 SHALL add, on BearSSL on all four targets and first of the P-curves (`design.md` D36), `curves::p256::basic_keypair<Backend>::make` from a raw 32-byte big-endian secret scalar d, refusing another size with `invalid_key_size` and d = 0 or d >= n with `invalid_secret_key` by a constant-time comparison before any backend call, and computing the public key; `generate()`, drawing from `random_device` until the scalar is valid; `secret()`, the scalar as `secret<32>`; `sign(crypto::ecdsa_sha256, input)` over the message's SHA-256 or a 32-byte `prehashed` digest, with RFC 6979's nonce, returning a `p256::ecdsa_signature`, `r || s` (IEEE P1363, 64 bytes) with low S: RFC 6979's signature, its s replaced by n - s when above (n - 1) / 2; `agree` with a validated peer key, returning the shared point's x coordinate as `secret<32>` (WebCrypto's `deriveBits` of 256 bits); `crypto::verify(crypto::ecdsa_sha256, signature, input, public_key)` accepting a valid signature whatever its S, refusing with `invalid_signature` a signature whose r or s is 0 or not below n (checked by webcpp before any backend call) and one that does not verify, and refusing a signature over a digest whose value modulo n is 0 (32 zero bytes, or n itself) as `invalid_signature` even when `sign` made it, BearSSL's verification refusing a zero multiplier, a limit its documentation states (no SHA-256 of a message gives that digest in practice); and `normalize_low_s` (the low-S form) and `is_low_s` (true exactly when 0 < r < n and 0 < s <= (n - 1) / 2), which call no backend. Verification MUST NOT impose low S, since WebCrypto's signatures may be high; a protocol that requires it of its peers (Trystero) checks `is_low_s` before `crypto::verify`. Every public function of the family that calls a backend MUST hold `detail::timing_guard` and be called by `test/timing_coverage_bearssl_test.cpp`; those that call none MUST be checked to enter none. Tests MUST run RFC 6979 section A.2.5's P-256 and SHA-256 cases, OpenSSL natively with deterministic nonces as a byte-for-byte oracle, and node's WebCrypto on Emscripten both ways.

#### Scenario: A Trystero peer's stored scalar
- **WHEN** `basic_keypair::make` is given the 32-byte scalar of RFC 6979 section A.2.5's P-256 key
- **THEN** `public_key().uncompressed()` is `0x04` followed by that section's Ux and Uy, `secret()` gives the 32 bytes back, and `sign(crypto::ecdsa_sha256, "sample")` is the section's r and n - s, the section's s being high

#### Scenario: A scalar out of range
- **WHEN** `make` is given n, n + 1 or 32 zero bytes
- **THEN** it returns `invalid_secret_key` without calling BearSSL, and 31 or 33 bytes give `invalid_key_size`

#### Scenario: ECDH with WebCrypto
- **WHEN** node's WebCrypto imports the same scalar (`pkcs8` from `crypto::encode(crypto::pkcs8, key)`, and `jwk` from `crypto::encode(crypto::jwk, key)`) and a peer's raw public key and derives 256 bits
- **THEN** the bits equal `agree` of the key pair with the peer's `basic_public_key`, through either import

#### Scenario: A high-S signature from a peer
- **WHEN** `crypto::verify` is given RFC 6979's signature of "sample" as the section prints it, whose S is high, as WebCrypto may sign
- **THEN** it accepts it, `is_low_s` is false, and `normalize_low_s` returns `r || n - s`, which is what `sign` gives, `is_low_s` accepts, and WebCrypto verifies

#### Scenario: A digest that is 0 modulo n
- **WHEN** `sign(crypto::ecdsa_sha256, crypto::prehashed{digest})` signs 32 zero bytes, or the 32 bytes of n, and `crypto::verify` is given that signature, the same marker and the key pair's public key, on BearSSL
- **THEN** `sign` succeeds and `crypto::verify` returns `invalid_signature`

### Requirement: secp256k1: its encodings and the x-only tweak
The interface of `curves::secp256k1` SHALL offer its keys and signatures through `crypto::encode` and `crypto::decode` (the public key's `raw` uncompressed by default, `raw(crypto::compressed)` and `raw(crypto::xonly)`, `spki` uncompressed by default and `spki(crypto::compressed)`, decoding either form; the ECDSA signature's `der`, strict DER, BIP66, at most 72 bytes), `normalize_low_s` (the low-S form `crypto::verify` requires under `ecdsa_sha256`), `is_low_s`, `tweak_xonly` (an x-only key tweaked by any 32-byte tweak: the tweaked x-only key and its parity, `secp256k1_xonly_pubkey_tweak_add`) and `basic_keypair::tweak_xonly` (the key pair tweaked in place by any 32-byte tweak, `secp256k1_keypair_xonly_tweak_add`), each a wrapper over libsecp256k1's own function or arithmetic on public bytes, reading the canary and allocating nothing; a key pair whose tweak gives no valid key is wiped. The caller computes a tweak; crypto MUST NOT compute a protocol's tweak, decode lax DER, serialize transactions, compute sighashes or derive keys.

#### Scenario: BIP341's wallet vectors through the generic tweak
- **WHEN** every `scriptPubKey` and `keyPathSpending` case of BIP341's `wallet-test-vectors.json` is run, the test computing each TapTweak with libsecp256k1's tagged hash and passing it
- **THEN** `tweak_xonly` gives each `tweakedPubkey` (7 cases), and a key pair tweaked by `keypair::tweak_xonly` signs each sighash, passed as the message to `sign(crypto::schnorr, ...)`, into the vector's witness (7 inputs)

#### Scenario: A tweak not below the group order
- **WHEN** an x-only key or a key pair is tweaked by 32 bytes of 0xff
- **THEN** both give `invalid_secret_key`, and the key pair is wiped and empty, as moved from

#### Scenario: A high-S signature
- **WHEN** a valid signature's s is replaced by n - s
- **THEN** `crypto::verify(crypto::ecdsa_sha256, ...)` refuses it, `is_low_s` is false, and `normalize_low_s` returns the original, which verifies

#### Scenario: A Nostr key in 32 bytes
- **WHEN** a BIP340 x-only public key is decoded with `crypto::decode<secp256k1::public_key>(crypto::raw(crypto::xonly), bytes)`
- **THEN** the key is the one of even y, and `crypto::encode(crypto::raw(crypto::xonly), key)` gives the 32 bytes back
