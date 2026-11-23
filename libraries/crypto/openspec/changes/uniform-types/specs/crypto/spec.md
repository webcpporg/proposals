## ADDED Requirements

### Requirement: Signatures: a type per curve and scheme
The library SHALL offer `curves::ed25519::signature` (64 bytes), `curves::p256::ecdsa_signature` (64 bytes, r then s), `curves::secp256k1::ecdsa_signature` (64 bytes, r then s), `curves::secp256k1::recoverable_signature` (65 bytes, r, s and the recovery id, with `recovery_id()`) and `curves::secp256k1::schnorr_signature` (64 bytes), distinct regular value types, each with `static from_bytes(std::span<std::uint8_t const>)` returning `result<T>` (`invalid_signature` for another size, nothing else checked) and `bytes()`, and none converting implicitly to another type or to bytes. Every `sign` MUST return the type of its curve and scheme, and every `verify` and `recover`, member and free, synchronous and asynchronous, MUST take it; no overload taking a signature as bytes MUST exist, and a signature of another curve or scheme MUST stop the build with a message naming the type expected.

#### Scenario: A signature from the wire
- **WHEN** 64 bytes received from a peer are made into a `p256::ecdsa_signature` with `from_bytes` and given to `crypto::verify(crypto::ecdsa_sha256, signature, message, public_key)`, and 63 bytes are given to `from_bytes`
- **THEN** the first verifies and the second returns `invalid_signature` from `from_bytes`

#### Scenario: The wrong curve
- **WHEN** a program passes a `secp256k1::ecdsa_signature` to `crypto::verify(crypto::ecdsa_sha256, signature, message, p256_public_key)`
- **THEN** compilation stops with a message naming `p256::ecdsa_signature`

### Requirement: X25519: typed keys
`curves::x25519::public_key` SHALL be made by `from_bytes` from 32 bytes (`invalid_public_key` for another size, nothing else checked) and give `bytes()`; `curves::x25519::basic_keypair<Backend>` (and `x25519::keypair`) SHALL be made by `from_secret` from a 32-byte secret or by `generate()`, give `public_key()`, `secret()` and `agree(peer)` (`peer` an `x25519::public_key`, the result `secret<32>`, `weak_public_key` when it is all zero, on every backend), with `async_from_secret`, `async_generate` and `async_agree` members and the free `crypto::from_secret`, `crypto::generate`, `crypto::agree` and their asynchronous forms. The free functions `curves::x25519::public_key(secret)` and `curves::x25519::agree(secret, peer)` MUST NOT exist.

#### Scenario: RFC 7748's agreement
- **WHEN** Alice's and Bob's key pairs of RFC 7748 section 6.1 are made from their secrets and agree with each other's `public_key()`
- **THEN** both give the RFC's shared secret, synchronously and through `async_agree`

### Requirement: Key pairs: three concepts
`<webcpp/crypto/keypair.hpp>` SHALL declare `crypto::keypair<K>` (`K::from_secret(secret_bytes)` and `K::generate()` returning `result<K>`, `public_key()` returning `result<typename K::public_key_type>`, `secret()` returning `result<secret<K::secret_key_size>>`), `crypto::signing_keypair<K>` (`keypair<K>` and `sign(scheme, input)` for every scheme `K` declares, at least one, returning the scheme's signature type) and `crypto::agreement_keypair<K>` (`keypair<K>` and `agree(peer)` returning `result<secret<N>>`). Ed25519's key pair MUST be a `signing_keypair` and not an `agreement_keypair`; X25519's an `agreement_keypair` and not a `signing_keypair`; P-256's and secp256k1's both; each checked by `static_assert`s in its curve's header. `crypto::sign` and `async_sign` MUST be constrained by `signing_keypair`, `crypto::agree` and `async_agree` by `agreement_keypair`.

#### Scenario: An X25519 key pair does not sign
- **WHEN** a program calls `crypto::sign(x25519_key, crypto::eddsa, message)`
- **THEN** compilation stops with a message saying that X25519 key pairs agree and do not sign

#### Scenario: A key pair made again from its secret
- **WHEN** `K::from_secret(k.secret()->bytes())` is called for a key pair `k` of each of the four curves
- **THEN** the new key pair has the same public key

### Requirement: Ed25519: a typed conversion to X25519
`curves::ed25519::basic_keypair<Backend>::to_x25519()` SHALL return `result<x25519::basic_keypair<B>>` and `curves::ed25519::public_key::to_x25519()` SHALL return `result<x25519::public_key>` (`invalid_public_key` for a key that is not canonical or of small order), with the free `crypto::to_x25519(value)`; `x25519_secret()` and `curves::ed25519::to_x25519_public` MUST NOT exist.

#### Scenario: Conversion agrees with signing keys
- **WHEN** an Ed25519 key pair is converted with `to_x25519()` and its public key with `public_key()->to_x25519()`
- **THEN** the converted key pair's `public_key()` equals the converted public key, and libsodium's `crypto_sign_ed25519_sk_to_curve25519` and `pk_to_curve25519` give the same bytes

### Requirement: AEADs and boxes: concepts
`<webcpp/crypto/aead.hpp>` SHALL declare `crypto::aead<C>` (`key_size`, `iv_size`, `tag_size`, `result_type`, `static from_key(key)` returning `result<C>`, `seal(iv, additional_data, data)` returning `result<result_type>`, `open(iv, additional_data, data, tag)` returning `result<void>`), its refinement `crypto::streaming_aead<C>` (`begin_seal`, `update`, `result`, `begin_open`, `authenticate`, `decrypt`, `verify`, `end_open`) and the sibling `crypto::authenticated_box<C>` (`key_size`, `iv_size`, `tag_size`, `result_type`, `seal(iv, data)`, `open(iv, data, tag)`). `ciphers::gcm<Cipher>` MUST be a `streaming_aead`; `ciphers::chacha20_poly1305`, `chacha20_poly1305_original` and `xchacha20_poly1305` `aead`s; `ciphers::secretbox`, `secretbox_xchacha20poly1305`, `public_key_encryption::box` and `box_xchacha20poly1305` `authenticated_box`es taking no additional data; each checked by a `static_assert` in its header. `crypto::seal` and `crypto::open` MUST exist in an `aead` form (with additional data) and an `authenticated_box` form (without), each constrained by its concept.

#### Scenario: secretbox has no additional data
- **WHEN** a program calls `secretbox.seal(nonce, additional_data, data)`
- **THEN** it does not compile, and `secretbox.seal(nonce, data)` and `crypto::seal(secretbox, nonce, data)` seal as libsodium's `crypto_secretbox_detached` does

### Requirement: AEADs and boxes: asynchronous on every native backend
`async_seal` and `async_open`, member and free, SHALL exist for every `aead` and every `authenticated_box`, completing through `asio::post` on a synchronous backend; on a backend whose `detail::asynchronous_backend_v` is true, only AES-GCM SHALL be offered, and any other type MUST stop the build with a message saying WebCrypto has no such algorithm and that its asynchronous forms run on a native backend.

#### Scenario: XChaCha20-Poly1305 asynchronously
- **WHEN** a coroutine `co_await crypto::async_seal(executor, aead, nonce, ad, data, use_awaitable)` runs with `ciphers::xchacha20_poly1305` on libsodium natively
- **THEN** it completes with the tag the synchronous `seal` gives, and the same call on a stand-in browser backend does not compile

### Requirement: Constructions: grouped by purpose
The library SHALL offer crypto_box and sealed boxes in namespace `webcpp::crypto::public_key_encryption` (`basic_box`, `box`, `box_xchacha20poly1305` in `<webcpp/crypto/public_key_encryption/box.hpp>`; `basic_sealed_box`, `sealed_box` in `<webcpp/crypto/public_key_encryption/sealed_box.hpp>`) and crypto_kx in namespace `webcpp::crypto::key_exchange` (`kx_keypair`, `session_keys` in `<webcpp/crypto/key_exchange/kx.hpp>`). The namespace `webcpp::crypto::hybrid`, the headers `hybrid/box.hpp`, `hybrid/sealed_box.hpp` and `curves/x25519/kx.hpp`, and `curves::x25519::kx_keypair` and `curves::x25519::session_keys` MUST NOT exist, with no alias; `curves` MUST hold curve primitives only.

#### Scenario: The old names are gone
- **WHEN** a program includes `<webcpp/crypto/hybrid/box.hpp>` or names `crypto::hybrid::box` or `crypto::curves::x25519::kx_keypair`
- **THEN** it does not compile, and the same program with `<webcpp/crypto/public_key_encryption/box.hpp>`, `crypto::public_key_encryption::box` and `crypto::key_exchange::kx_keypair` builds and runs

### Requirement: Factories: named by their input
The library SHALL name every fallible factory by what it takes: `K::from_secret(secret_bytes)` and `K::generate()` for every key pair, `T::from_bytes(bytes)` for every public key and signature, `p256::basic_public_key::from_coordinates(x, y)`, `C::from_key(key)` for every cipher, AEAD and secretbox, `public_key_encryption::basic_box::from_keys(own_keypair, peer_public_key)`, and `key_exchange::kx_keypair::from_keypair(x25519_keypair)` beside `derive_from_seed(seed)` and `generate()`; with the free `crypto::from_secret<K>`, `crypto::from_bytes<T>`, `crypto::from_key<C>` and `crypto::generate<K>` in `<webcpp/crypto/factories.hpp>`, and `async_from_secret`, `async_from_key` and `async_generate`, member and free. No function, member or free, synchronous or asynchronous, MUST be named `make` or `async_make`.

#### Scenario: The call site says where the secret comes from
- **WHEN** a program calls `p256::keypair::from_secret(bytes)` and `p256::keypair::generate()`, and then `p256::keypair::make(bytes)`
- **THEN** the first two compile, the first giving the key of those bytes and the second a drawn key, and the third does not compile

## MODIFIED Requirements

### Requirement: Signatures: one name per operation, the scheme always named
`<webcpp/crypto/signing.hpp>` SHALL declare, in namespace `webcpp::crypto`, the scheme tags `eddsa` (Ed25519, RFC 8032's pure form), `ecdsa_sha256` (ECDSA with SHA-256 on P-256 and secp256k1), `ecdsa_sha256_recoverable` (secp256k1, the signature with its recovery id) and `schnorr` (BIP340, secp256k1), constexpr objects of empty types whose default constructors are explicit; the marker `prehashed<N>`, holding `std::span<std::uint8_t const, N> digest` and made by explicit constructors only, deduced from a `std::array<std::uint8_t, N>`, a fixed-size span or a `boost::hash2::digest<N>`, and refused for `std::dynamic_extent`; and the concepts `can_sign<Keypair, Scheme, Input>` and `can_verify<Scheme, Input, PublicKey>`. Every key pair SHALL sign with `key.sign(scheme, input)`, returning `boost::system::result<T>`, `T` the signature type of the key's curve and scheme, and every signature SHALL be verified with the free `crypto::verify(scheme, signature, input, public_key)`, returning `boost::system::result<void>`, the input being a message (anything that converts to `std::span<std::uint8_t const>`) or a `prehashed` digest. Every `verify`, member and free, synchronous and asynchronous, and `curves::secp256k1::recover` SHALL take the signature as its typed value, made from bytes by the type's `from_bytes`, which returns `invalid_signature` for another size. No function MUST choose between a message and a digest by a span's extent or size, and no form without a scheme MUST exist.

#### Scenario: A 32-byte message is a message
- **WHEN** a P-256 key pair signs `std::array<std::uint8_t, 32>` with `sign(crypto::ecdsa_sha256, message)`
- **THEN** the signature is that of the message's SHA-256, equal to `sign(crypto::ecdsa_sha256, crypto::prehashed{sha256_of_message})`, and `crypto::verify(crypto::ecdsa_sha256, signature, message, public_key)` accepts it

#### Scenario: The backend named the same way on every curve
- **WHEN** a program calls `crypto::verify<B>(scheme, signature, input, key)` with `B` the key's backend on P-256, `backends::sodium` on Ed25519 and `backends::secp256k1` on secp256k1
- **THEN** each compiles and verifies, and on P-256 a `B` other than the key's backend does not compile

#### Scenario: A signature received as bytes
- **WHEN** a signature arrives as a `std::vector<std::uint8_t>`, is made into its curve's signature type and given to `key.verify`, `crypto::verify`, `key.async_verify` and `crypto::async_verify`, and the same with 63 or 65 bytes
- **THEN** every form compiles, the asynchronous ones on Ed25519 and P-256; the 64 bytes verify and the others give `invalid_signature` from `from_bytes`, on every curve

#### Scenario: A call without a scheme
- **WHEN** a program calls `key.sign(message)` on any key pair
- **THEN** it does not compile

### Requirement: Key pairs: one contract
`<webcpp/crypto/keypair.hpp>` SHALL declare the concept `crypto::keypair<K>`: `K::from_secret(secret_bytes)` and `K::generate()` returning `result<K>`, `public_key()` returning `result<typename K::public_key_type>` and `secret()` returning `result<secret<K::secret_key_size>>`, refined by `crypto::signing_keypair<K>` and `crypto::agreement_keypair<K>`. Every `basic_keypair<Backend>` of Ed25519, X25519, P-256 and secp256k1 MUST satisfy `keypair`, checked by a `static_assert` in its curve's header; the public key a signing key pair gives MUST be accepted by `crypto::verify` for each of the curve's verifying schemes; ECDH MUST be named `agree(peer)` on every curve that has it, the peer the curve's typed public key, giving the shared secret (the shared point's raw x coordinate on P-256 and secp256k1, RFC 7748's output on X25519).

#### Scenario: A key pair's public key verifies its signature
- **WHEN** any signing key pair `k` of the three signing curves signs `m` with one of its schemes `s`
- **THEN** `crypto::verify(s, signature, m, *k.public_key())` succeeds, with no conversion at the call

#### Scenario: A key pair made again from its secret
- **WHEN** `K::from_secret(k.secret()->bytes())` is called for a key pair `k`
- **THEN** the new key pair has the same public key

### Requirement: Ed25519: key pairs, signatures and strict verification
`curves::ed25519::basic_keypair<Backend>` SHALL be made from a 32-byte seed (RFC 8032) or by `generate()`, give its public key as a `curves::ed25519::public_key` (`from_bytes` from 32 bytes, `bytes()` back), give its seed back with `secret()`, sign a message into a `curves::ed25519::signature` with `sign(crypto::eddsa, message)`, and convert itself to an X25519 key pair with `to_x25519()`; `crypto::verify<Backend>(crypto::eddsa, signature, message, public_key[, policy])`, `public_key` a `curves::ed25519::public_key`, the policy a trailing tag object, `curves::ed25519::strict` by default, MUST return nothing on success and `invalid_signature` otherwise; a key of another size is refused earlier, by `curves::ed25519::public_key::from_bytes` with `invalid_public_key`. On libsodium verification is strict (canonical encodings, no small-order key, `S` reduced); the documentation MUST say which inputs other backends accept that libsodium refuses, as Wycheproof's EdDSA file shows.

#### Scenario: RFC 8032's test 1
- **WHEN** the key pair is made from RFC 8032 section 7.1's first secret key and signs the empty message with `sign(crypto::eddsa, message)`
- **THEN** the public key and the signature are the RFC's, and `crypto::verify(crypto::eddsa, signature, message, public_key)` accepts them

#### Scenario: A small-order public key
- **WHEN** `crypto::verify<backends::sodium>(crypto::eddsa, signature, message, public_key, curves::ed25519::strict)` is given a small-order public key (ed25519-speccheck's case 0)
- **THEN** it returns `invalid_signature`

### Requirement: X25519: agreement and the conversion
`curves::x25519::basic_keypair<Backend>` SHALL give its `x25519::public_key` from its 32-byte secret, agree a `secret<32>` with a peer's `x25519::public_key` by `agree(peer)`, and return `weak_public_key` when the result is all zero on every backend. `curves::ed25519::public_key::to_x25519()` MUST return the birational map's result as an `x25519::public_key`, or `invalid_public_key` for a key that is not canonical or of small order.

#### Scenario: RFC 7748's agreement
- **WHEN** Alice's and Bob's key pairs of RFC 7748 section 6.1 agree in both directions
- **THEN** both give the RFC's shared secret

#### Scenario: Conversion agrees with signing keys
- **WHEN** an Ed25519 key pair's `to_x25519()` gives an X25519 key pair
- **THEN** its `public_key()` equals the Ed25519 key pair's public key's `to_x25519()`

### Requirement: Boxes: crypto_box, sealed boxes and secretbox
The library SHALL offer `public_key_encryption::box` (X25519, XSalsa20-Poly1305) and `public_key_encryption::box_xchacha20poly1305`, made from an `x25519::basic_keypair` and a peer's `x25519::public_key` so that the shared key is computed once (libsodium's `beforenm`), with `seal(nonce, data)` and `open(nonce, data, tag)` in place and the 16-byte tag apart; `public_key_encryption::sealed_box::seal(recipient, data, out)`, `recipient` an `x25519::public_key`, and `open(recipient, sealed, out)`, `recipient` an `x25519::basic_keypair`; and `ciphers::secretbox` and `ciphers::secretbox_xchacha20poly1305`, keyed objects over 32-byte keys with 24-byte nonces, `seal(nonce, data)` and `open(nonce, data, tag)`, taking no additional data. A failed `open` MUST zero `data`, as the genesis's AEADs do.

#### Scenario: A sealed box opened by its recipient
- **WHEN** a message is sealed to a recipient's `x25519::public_key` and opened with the recipient's key pair
- **THEN** it opens, and a sealed box changed in one byte returns `authentication_failed` with `data` zeroed

### Requirement: Key exchange: crypto_kx
`key_exchange::kx_keypair<Backend>` SHALL be made from an `x25519::basic_keypair<Backend>` by `from_keypair`, by `generate()`, or from a 32-byte seed by `derive_from_seed(seed)`, libsodium's `crypto_kx_seed_keypair` (the secret is BLAKE2b-256 of the seed), no other factory taking a seed, give its `x25519::public_key`, and compute `client_session_keys(server_public_key)` and `server_session_keys(client_public_key)`, each peer an `x25519::public_key`, each returning a receive key and a transmit key as `secret<32>`, through libsodium's `crypto_kx`, and return `weak_public_key` where libsodium refuses the peer's key.

#### Scenario: Client and server agree
- **WHEN** a client and a server compute their session keys from each other's public keys
- **THEN** the client's transmit key is the server's receive key and the reverse

#### Scenario: A seed is not a secret
- **WHEN** the same 32 bytes are given to `kx_keypair::derive_from_seed` and, as an X25519 secret, to `kx_keypair::from_keypair(*x25519::keypair::from_secret(bytes))`
- **THEN** `derive_from_seed` gives the keys of libsodium's `crypto_kx_seed_keypair` for that seed, and its public key differs from `from_keypair`'s

### Requirement: Interface: one AEAD template for the one-shot AEADs
`ciphers::aead<Family, Backend>` SHALL implement the guards, the in-place `seal` and `open`, the tag apart, the zeroing of `data` after a failed `open` and the move rules of the genesis's `ciphers::chacha20_poly1305` for every one-shot AEAD with additional data, its family tag giving `key_size`, `iv_size`, `tag_size` and `max_message_size`, and every instance MUST satisfy `crypto::aead`. `ciphers::chacha20_poly1305` (12-byte nonce), `ciphers::chacha20_poly1305_original` (8-byte nonce) and `ciphers::xchacha20_poly1305` (24-byte nonce) MUST be its aliases; secretbox is not, being an `authenticated_box`; `ciphers::gcm<Cipher>` keeps its own class and its incremental seal and open, a `streaming_aead`.

#### Scenario: XChaCha20-Poly1305's nonce
- **WHEN** `ciphers::xchacha20_poly1305::seal` is called with a 12-byte nonce
- **THEN** it returns `invalid_iv_size` and `data` is untouched

### Requirement: P-256: a key pair from a raw scalar, raw ECDH and low S
Increment 3 SHALL add, on BearSSL on all four targets and first of the P-curves (`design.md` D36), `curves::p256::basic_keypair<Backend>::from_secret` from a raw 32-byte big-endian secret scalar d, refusing another size with `invalid_key_size` and d = 0 or d >= n with `invalid_secret_key` by a constant-time comparison before any backend call, and computing the public key; `generate()`, drawing from `random_device` until the scalar is valid; `secret()`, the scalar as `secret<32>`; `sign(crypto::ecdsa_sha256, input)` over the message's SHA-256 or a 32-byte `prehashed` digest, with RFC 6979's nonce, returning a `p256::ecdsa_signature`, `r || s` (IEEE P1363, 64 bytes) with low S: RFC 6979's signature, its s replaced by n - s when above (n - 1) / 2; `agree` with a validated peer key, returning the shared point's x coordinate as `secret<32>` (WebCrypto's `deriveBits` of 256 bits); `crypto::verify(crypto::ecdsa_sha256, signature, input, public_key)` accepting a valid signature whatever its S, refusing with `invalid_signature` a signature whose r or s is 0 or not below n (checked by webcpp before any backend call) and one that does not verify, and refusing a signature over a digest whose value modulo n is 0 (32 zero bytes, or n itself) as `invalid_signature` even when `sign` made it, BearSSL's verification refusing a zero multiplier, a limit its documentation states (no SHA-256 of a message gives that digest in practice); and `normalize_low_s` (the low-S form) and `is_low_s` (true exactly when 0 < r < n and 0 < s <= (n - 1) / 2), which call no backend. Verification MUST NOT impose low S, since WebCrypto's signatures may be high; a protocol that requires it of its peers (Trystero) checks `is_low_s` before `crypto::verify`. Every public function of the family that calls a backend MUST hold `detail::timing_guard` and be called by `test/timing_coverage_bearssl_test.cpp`; those that call none MUST be checked to enter none. Tests MUST run RFC 6979 section A.2.5's P-256 and SHA-256 cases, OpenSSL natively with deterministic nonces as a byte-for-byte oracle, and node's WebCrypto on Emscripten both ways.

#### Scenario: A Trystero peer's stored scalar
- **WHEN** `basic_keypair::from_secret` is given the 32-byte scalar of RFC 6979 section A.2.5's P-256 key
- **THEN** `public_key().uncompressed()` is `0x04` followed by that section's Ux and Uy, `secret()` gives the 32 bytes back, and `sign(crypto::ecdsa_sha256, "sample")` is the section's r and n - s, the section's s being high

#### Scenario: A scalar out of range
- **WHEN** `from_secret` is given n, n + 1 or 32 zero bytes
- **THEN** it returns `invalid_secret_key` without calling BearSSL, and 31 or 33 bytes give `invalid_key_size`

#### Scenario: ECDH with WebCrypto
- **WHEN** node's WebCrypto imports the same scalar (`pkcs8` from `encode_pkcs8`) and a peer's raw public key and derives 256 bits
- **THEN** the bits equal `agree` of the key pair with the peer's `basic_public_key`

#### Scenario: A high-S signature from a peer
- **WHEN** `crypto::verify` is given RFC 6979's signature of "sample" as the section prints it, whose S is high, as WebCrypto may sign
- **THEN** it accepts it, `is_low_s` is false, and `normalize_low_s` returns `r || n - s`, which is what `sign` gives, `is_low_s` accepts, and WebCrypto verifies

#### Scenario: A digest that is 0 modulo n
- **WHEN** `sign(crypto::ecdsa_sha256, crypto::prehashed{digest})` signs 32 zero bytes, or the 32 bytes of n, and `crypto::verify` is given that signature, the same marker and the key pair's public key, on BearSSL
- **THEN** `sign` succeeds and `crypto::verify` returns `invalid_signature`

### Requirement: Asynchronous family: completion tokens, for what the browser offers
`<webcpp/crypto/async.hpp>` SHALL offer `async_from_secret`, `async_from_key`, `async_generate`, `async_seal`, `async_open`, `async_encrypt`, `async_decrypt`, `async_sign`, `async_verify`, `async_agree`, `async_wrap`, `async_unwrap` and `async_export` for every algorithm WebCrypto offers, each an Asio asynchronous operation in webcpp::asio's model (initiated through `async_initiate`, any completion token: a callback, `use_awaitable`, Cobalt's, `use_future`) taking first the executor its completion is posted to, and completing with `void(boost::system::error_code, T)`, `T` the empty object a move leaves when the code is an error, or with `void(boost::system::error_code)` where the synchronous call returns no value. Each family's asynchronous default backend MUST be its synchronous default on every target, Emscripten included; `backends::browser` is used only where a type names it. On `backends::browser` each MUST call WebCrypto, with or without pthreads, and await its promise through `webcpp::asio::browser::async_await`, completing on the handler's associated executor; on a synchronous backend each MUST run the synchronous call and complete through `asio::post`, never inside the initiating call. An `async_` variant of a cheap algorithm WebCrypto lacks MUST NOT exist, except `async_from_key`, `async_seal` and `async_open` of every `crypto::aead` and `crypto::authenticated_box`, which exist on every native backend and stop the build on `backends::browser` for any algorithm but AES-GCM, with a message saying WebCrypto has no such algorithm. The synchronous API MUST NOT call WebCrypto. The convenience header MUST leave out `async.hpp` and `backends/browser.hpp`, naming webcpp::asio as the reason.

#### Scenario: One coroutine, two targets
- **WHEN** a coroutine `co_await async_seal(executor, aead, iv, ad, data, use_awaitable)` runs natively on BearSSL and on Emscripten on the browser
- **THEN** both complete with the same ciphertext and tag for the same key, IV and message

#### Scenario: A callback is never called inline
- **WHEN** `async_seal` on a synchronous backend is given an `io_context`'s executor and a callback that sets a flag
- **THEN** the flag is unset when `async_seal` returns, and set after the executor runs

#### Scenario: No asynchronous variant of a cheap algorithm the browser lacks
- **WHEN** a program calls `async_seal` on `ciphers::xchacha20_poly1305` on libsodium, and on a backend whose `asynchronous_backend_v` is true
- **THEN** the first compiles and seals as `ciphers::xchacha20_poly1305::seal` does, and the second does not compile
