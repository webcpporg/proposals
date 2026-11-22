## ADDED Requirements

### Requirement: Signatures: one name per operation, the scheme always named
`<webcpp/crypto/signing.hpp>` SHALL declare, in namespace `webcpp::crypto`, the scheme tags `eddsa` (Ed25519, RFC 8032's pure form), `ecdsa_sha256` (ECDSA with SHA-256 on P-256 and secp256k1), `ecdsa_sha256_recoverable` (secp256k1, the signature with its recovery id) and `schnorr` (BIP340, secp256k1), constexpr objects of empty types whose default constructors are explicit; the marker `prehashed<N>`, an aggregate holding `std::span<std::uint8_t const, N> digest`, deduced from a `std::array<std::uint8_t, N>`, a fixed-size span or a `boost::hash2::digest<N>`, and refused for `std::dynamic_extent`; and the concepts `can_sign<Keypair, Scheme, Input>` and `can_verify<Scheme, Input, PublicKey>`. Every key pair SHALL sign with `key.sign(scheme, input)`, returning `boost::system::result<std::array<std::uint8_t, N>>`, and every signature SHALL be verified with the free `crypto::verify(scheme, signature, input, public_key)`, returning `boost::system::result<void>`, the input being a message (anything that converts to `std::span<std::uint8_t const>`) or a `prehashed` digest. No function MUST choose between a message and a digest by a span's extent or size, and no form without a scheme MUST exist.

#### Scenario: A 32-byte message is a message
- **WHEN** a P-256 key pair signs `std::array<std::uint8_t, 32>` with `sign(crypto::ecdsa_sha256, message)`
- **THEN** the signature is that of the message's SHA-256, equal to `sign(crypto::ecdsa_sha256, crypto::prehashed{sha256_of_message})`, and `crypto::verify(crypto::ecdsa_sha256, signature, message, public_key)` accepts it

#### Scenario: A call without a scheme
- **WHEN** a program calls `key.sign(message)` on any key pair
- **THEN** it does not compile

### Requirement: Signatures: the valid pairs of scheme and input
The library SHALL accept exactly these pairs, synchronously and asynchronously, and every other pair MUST stop the build in a `static_assert` whose message names the reason (the key's curve and the schemes it has, the scheme that takes no digest, the digest size the scheme needs, the backend that cannot sign a digest):

| Scheme | Curve and public key | Message | `prehashed{digest}` | Signature |
|---|---|---|---|---|
| `eddsa` | Ed25519, its 32 bytes | signed as given | refused | 64 bytes |
| `ecdsa_sha256` | P-256, `basic_public_key<B>` | SHA-256 inside | N = 32 | 64 bytes, r then s; signs low S, verifies high and low S |
| `ecdsa_sha256` | secp256k1, `public_key` | SHA-256 inside | N = 32 | 64 bytes, r then s; signs low S, refuses high S |
| `ecdsa_sha256_recoverable` | secp256k1, the key `recover` gives | SHA-256 inside | N = 32 | 65 bytes, r, s (low), the id |
| `schnorr` | secp256k1, its 32-byte x-only key | signed as given | refused | 64 bytes |

`can_sign` and `can_verify` MUST be true exactly for these pairs, and false for a marker on a backend whose `detail::asynchronous_backend_v` is true.

#### Scenario: Ed25519 and a digest
- **WHEN** a program calls `key.sign(crypto::eddsa, crypto::prehashed{digest})` on an Ed25519 key pair
- **THEN** compilation stops with a message saying that Ed25519 signs the message itself and that Ed25519ph is `curves::ed25519::ph`

#### Scenario: Schnorr and a digest
- **WHEN** a program calls `key.sign(crypto::schnorr, crypto::prehashed{digest})` on a secp256k1 key pair
- **THEN** compilation stops with a message saying that BIP340 signs a message of any length and has no prehash mode

#### Scenario: A scheme the curve does not have
- **WHEN** a program calls `key.sign(crypto::schnorr, message)` on a P-256 key pair
- **THEN** compilation stops with a message naming P-256's scheme, `ecdsa_sha256`

#### Scenario: A digest of another size
- **WHEN** a program passes `crypto::prehashed{std::array<std::uint8_t, 48>{}}` with `ecdsa_sha256`
- **THEN** compilation stops with a message saying that `ecdsa_sha256` takes a 32-byte digest

#### Scenario: Bitcoin's sighash
- **WHEN** a secp256k1 key pair signs a transaction's SHA-256d sighash, computed by the program with webcpp::hash, with `sign(crypto::ecdsa_sha256, crypto::prehashed{sighash})`
- **THEN** the signature is libsecp256k1's `secp256k1_ecdsa_sign` of those 32 bytes, and `crypto::verify` with the same marker accepts it; crypto offers no SHA-256d scheme

### Requirement: Signatures: the format helpers have one set of names
`curves::p256` and `curves::secp256k1` SHALL each offer `to_der(signature, output)` (strict DER into a caller's buffer of at least `max_der_size` = 72 bytes, returning the length), `from_der(der)` (strict DER only, returning the 64-byte r || s), `normalize_low_s(signature)` (r || n - s when s is above (n - 1) / 2, the signature unchanged otherwise; `invalid_signature` when r or s is 0 or not below n) and `is_low_s(signature)` (`bool`: true exactly when 0 < r < n and 0 < s <= (n - 1) / 2). The names `normalize_ecdsa` and `check_low_s` MUST NOT exist. The 64-byte r || s (IEEE P1363, WebCrypto's form) is the signature on both curves.

#### Scenario: The same helper on both curves
- **WHEN** a high-S signature of each curve is given to its curve's `normalize_low_s`
- **THEN** the result is r || n - s for that curve's n, `is_low_s` is false before and true after, and `to_der` then `from_der` gives the result back

### Requirement: Signatures: the asynchronous family takes the same forms
`async_sign(executor, key, scheme, input, token)` and `async_verify(executor, scheme, signature, input, public_key, token)` SHALL take the inputs of the synchronous calls for Ed25519 (`eddsa`, `Policy` and `Backend` template parameters) and P-256 (`ecdsa_sha256`), complete with the synchronous result as before, and check `can_sign` or `can_verify` in a `static_assert`. secp256k1 MUST stay out of the asynchronous family. A backend that cannot sign a digest (WebCrypto's has no prehash mode) MUST refuse `prehashed` at compile time.

#### Scenario: The same bytes, synchronously and asynchronously
- **WHEN** a P-256 key pair signs a message with `sign(crypto::ecdsa_sha256, message)` and with `async_sign(executor, key, crypto::ecdsa_sha256, message, token)`
- **THEN** both give the same signature, and `async_verify(executor, crypto::ecdsa_sha256, signature, crypto::prehashed{sha256_of_message}, public_key, token)` accepts it

#### Scenario: A digest on a backend that cannot sign one
- **WHEN** `can_sign` is evaluated for a P-256 key pair on a stand-in backend whose `asynchronous_backend_v` is true, `ecdsa_sha256` and a `prehashed<32>`
- **THEN** it is false, and with a message it is true

## MODIFIED Requirements

### Requirement: Ed25519: key pairs, signatures and strict verification
`curves::ed25519::basic_keypair<Backend>` SHALL be made from a 32-byte seed (RFC 8032) or by `generate()`, give its 32-byte public key, sign a message into 64 bytes with `sign(crypto::eddsa, message)`, and give its X25519 secret key as `secret<32>`; `crypto::verify<Policy, Backend>(crypto::eddsa, signature, message, public_key)` MUST return nothing on success and `invalid_signature` otherwise, `invalid_public_key` for a key of another size. On libsodium verification is strict (canonical encodings, no small-order key, `S` reduced); the documentation MUST say which inputs other backends accept that libsodium refuses, as Wycheproof's EdDSA file shows.

#### Scenario: RFC 8032's test 1
- **WHEN** the key pair is made from RFC 8032 section 7.1's first secret key and signs the empty message with `sign(crypto::eddsa, message)`
- **THEN** the public key and the signature are the RFC's, and `crypto::verify(crypto::eddsa, signature, message, public_key)` accepts them

#### Scenario: A small-order public key
- **WHEN** `crypto::verify<curves::ed25519::strict, backends::sodium>(crypto::eddsa, ...)` is given a small-order public key from Wycheproof
- **THEN** it returns `invalid_signature`

### Requirement: Ed25519ph: an incremental signer
`curves::ed25519::ph<Hash, Backend>`, with `Hash` webcpp::hash's `sha2_512` as RFC 8032 requires, SHALL sign and verify a message given in pieces (`begin`, `update`, `sign(key)` or `verify(signature, public_key)`) through libsodium's `crypto_sign_ed25519ph_*`, and refuse another hash at compile time.

#### Scenario: RFC 8032's Ed25519ph vector
- **WHEN** RFC 8032 section 7.3's message is given in two pieces
- **THEN** the signature is the RFC's

### Requirement: secp256k1: public keys and signatures
A key pair SHALL provide `xonly_public_key()` (32 bytes, BIP340) and `public_key()` (33 bytes, SEC1 compressed) as `result<std::array<...>>`; `sign(crypto::schnorr, message)` (BIP340, any message length, its 32 auxiliary random bytes drawn from `random_device`, `entropy_unavailable` when none can be drawn) and `sign(crypto::schnorr, message, aux_random)` (the caller's 32 fresh bytes); `sign(crypto::ecdsa_sha256, input)` (RFC 6979 nonce, low-S, compact 64-byte form, over the message's SHA-256 or a 32-byte `prehashed` digest) and `sign(crypto::ecdsa_sha256, input, extra_entropy)` (hedged); `sign(crypto::ecdsa_sha256_recoverable, input)` (65 bytes); and `ecdh(public_key)` (the SHA-256 of the compressed shared point, libsecp256k1's default). All of them MUST be `const noexcept` and return `invalid_state` on a moved-from key pair.

#### Scenario: Sign and verify a BIP340 message
- **WHEN** the secret is 32 bytes of `0x42`, the message 32 bytes of `0x17` and `aux` 32 zero bytes, and `sign(crypto::schnorr, message, aux)` then `crypto::verify(crypto::schnorr, signature, message, xonly_public_key)` run
- **THEN** the signature verifies, and changing the first message byte makes `crypto::verify` fail

### Requirement: secp256k1: verification without a context
`crypto::verify(crypto::schnorr, signature, message, xonly_public_key)` and `crypto::verify(crypto::ecdsa_sha256, signature, input, public_key)`, `public_key` a `curves::secp256k1::public_key` made by `parse_public_key`, SHALL use `secp256k1_context_static`, allocate nothing and be `noexcept`, and each MUST call `secp256k1_selftest()` first, keeping no flag, because libsecp256k1 recommends the static context only after the self-test and the library may hold no static mutable state. A failed self-test is a library built wrongly, and libsecp256k1's default error callback aborts. ECDSA's verification MUST refuse a high-S signature, as libsecp256k1 does.

#### Scenario: Verification allocates nothing
- **WHEN** the counting tests run both verifications under a counting `operator new`
- **THEN** the counts are zero

### Requirement: secp256k1: recovery, raw ECDH, tweaks and key forms
Increment 2 SHALL add, as thin wrappers over libsecp256k1 v0.8.0: `sign(crypto::ecdsa_sha256_recoverable, input)` and `recover(crypto::ecdsa_sha256_recoverable, signature, input)` (65-byte signatures with the recovery id, the input a message hashed with SHA-256 or a 32-byte `prehashed` digest); `ecdh_x` (the shared point's 32-byte x coordinate) and `ecdh_point` (its 65-byte uncompressed form) as `secret<32>` and `secret<65>`; `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate` and `pubkey_combine`; `parse_public_key` of 33 and 65 bytes and `serialize_public_key` in both forms; and `sign(crypto::ecdsa_sha256, input, extra_entropy)` (hedged signing).

#### Scenario: Ethereum's recovery
- **WHEN** a recoverable signature of a digest is made with `prehashed` and `recover` is given the scheme, the signature and the same marker
- **THEN** it returns the signer's public key, for random keys and for libsecp256k1's own recovery test cases

#### Scenario: Nostr's shared x coordinate
- **WHEN** two key pairs compute `ecdh_x` with each other's public keys
- **THEN** both give the same 32 bytes, the x coordinate of the shared point, unhashed

### Requirement: P-256: a key pair from a raw scalar, raw ECDH and low S
Increment 3 SHALL add, on BearSSL on all four targets and first of the P-curves (`design.md` D36), `curves::p256::basic_keypair<Backend>::make` from a raw 32-byte big-endian secret scalar d, refusing another size with `invalid_key_size` and d = 0 or d >= n with `invalid_secret_key` by a constant-time comparison before any backend call, and computing the public key; `generate()`, drawing from `random_device` until the scalar is valid; `secret_scalar()`, the scalar as `secret<32>`; `sign(crypto::ecdsa_sha256, input)` over the message's SHA-256 or a 32-byte `prehashed` digest, with RFC 6979's nonce, returning `r || s` (IEEE P1363, 64 bytes) with low S: RFC 6979's signature, its s replaced by n - s when above (n - 1) / 2; `ecdh_x` with a validated peer key, returning the shared point's x coordinate as `secret<32>` (WebCrypto's `deriveBits` of 256 bits); `crypto::verify(crypto::ecdsa_sha256, signature, input, public_key)` accepting a valid signature whatever its S, refusing with `invalid_signature` a signature whose r or s is 0 or not below n (checked by webcpp before any backend call) and one that does not verify, and refusing a signature over a digest whose value modulo n is 0 (32 zero bytes, or n itself) as `invalid_signature` even when `sign` made it, BearSSL's verification refusing a zero multiplier, a limit its documentation states (no SHA-256 of a message gives that digest in practice); and `normalize_low_s` (the low-S form) and `is_low_s` (true exactly when 0 < r < n and 0 < s <= (n - 1) / 2), which call no backend. Verification MUST NOT impose low S, since WebCrypto's signatures may be high; a protocol that requires it of its peers (Trystero) checks `is_low_s` before `crypto::verify`. Every public function of the family that calls a backend MUST hold `detail::timing_guard` and be called by `test/timing_coverage_bearssl_test.cpp`; those that call none MUST be checked to enter none. Tests MUST run RFC 6979 section A.2.5's P-256 and SHA-256 cases, OpenSSL natively with deterministic nonces as a byte-for-byte oracle, and node's WebCrypto on Emscripten both ways.

#### Scenario: A Trystero peer's stored scalar
- **WHEN** `basic_keypair::make` is given the 32-byte scalar of RFC 6979 section A.2.5's P-256 key
- **THEN** `public_key().uncompressed()` is `0x04` followed by that section's Ux and Uy, `secret_scalar()` gives the 32 bytes back, and `sign(crypto::ecdsa_sha256, "sample")` is the section's r and n - s, the section's s being high

#### Scenario: A scalar out of range
- **WHEN** `make` is given n, n + 1 or 32 zero bytes
- **THEN** it returns `invalid_secret_key` without calling BearSSL, and 31 or 33 bytes give `invalid_key_size`

#### Scenario: ECDH with WebCrypto
- **WHEN** node's WebCrypto imports the same scalar (`pkcs8` from `encode_pkcs8`) and a peer's raw public key and derives 256 bits
- **THEN** the bits equal `ecdh_x` of the key pair with the peer's `basic_public_key`

#### Scenario: A high-S signature from a peer
- **WHEN** `crypto::verify` is given RFC 6979's signature of "sample" as the section prints it, whose S is high, as WebCrypto may sign
- **THEN** it accepts it, `is_low_s` is false, and `normalize_low_s` returns `r || n - s`, which is what `sign` gives, `is_low_s` accepts, and WebCrypto verifies

#### Scenario: A digest that is 0 modulo n
- **WHEN** `sign(crypto::ecdsa_sha256, crypto::prehashed{digest})` signs 32 zero bytes, or the 32 bytes of n, and `crypto::verify` is given that signature, the same marker and the key pair's public key, on BearSSL
- **THEN** `sign` succeeds and `crypto::verify` returns `invalid_signature`

### Requirement: secp256k1: its encodings and the x-only tweak
The interface of `curves::secp256k1` SHALL offer `to_der` (a compact signature as strict DER, at most `max_der_size` = 72 bytes, into a caller's buffer), `from_der` (strict DER, BIP66), `normalize_low_s` (the low-S form `crypto::verify` requires under `ecdsa_sha256`), `is_low_s`, `tweak_xonly` (an x-only key tweaked by any 32-byte tweak: the tweaked x-only key and its parity, `secp256k1_xonly_pubkey_tweak_add`) and `basic_keypair::tweak_xonly` (the key pair tweaked in place by any 32-byte tweak, `secp256k1_keypair_xonly_tweak_add`), each a wrapper over libsecp256k1's own function or arithmetic on public bytes, reading the canary and allocating nothing; a key pair whose tweak gives no valid key is wiped. The caller computes a tweak; crypto MUST NOT compute a protocol's tweak, decode lax DER, serialize transactions, compute sighashes or derive keys.

#### Scenario: BIP341's wallet vectors through the generic tweak
- **WHEN** every `scriptPubKey` and `keyPathSpending` case of BIP341's `wallet-test-vectors.json` is run, the test computing each TapTweak with libsecp256k1's tagged hash and passing it
- **THEN** `tweak_xonly` gives each `tweakedPubkey` (7 cases), and a key pair tweaked by `keypair::tweak_xonly` signs each sighash, passed as the message to `sign(crypto::schnorr, ...)`, into the vector's witness (7 inputs)

#### Scenario: A tweak not below the group order
- **WHEN** an x-only key or a key pair is tweaked by 32 bytes of 0xff
- **THEN** both give `invalid_secret_key`, and the key pair is wiped and empty, as moved from

#### Scenario: A high-S signature
- **WHEN** a valid signature's s is replaced by n - s
- **THEN** `crypto::verify(crypto::ecdsa_sha256, ...)` refuses it, `is_low_s` is false, and `normalize_low_s` returns the original, which verifies
