## ADDED Requirements

### Requirement: Foundation: every backend exposes what its library offers to cryptography
Each backend SHALL expose every primitive of its library that belongs to cryptography as `design.md` D1 and D2 define it: libsodium (`backends::sodium`), libsecp256k1, BearSSL, OpenSSL, ctaes, Botan, blst and WebCrypto (`backends::browser`). Hash functions, XOFs, HMAC as a hash algorithm, key derivation and password hashing MUST NOT be exposed by webcpp::crypto: they are webcpp::hash's. Generic curve construction, field arithmetic, multi-scalar multiplication, reduced-round ChaCha, ZIP215 verification and any deterministic random bit generator MUST NOT be exposed.

#### Scenario: libsodium's hashing is not in crypto
- **WHEN** a program looks for `crypto_generichash` or `crypto_pwhash` in webcpp::crypto's headers
- **THEN** there is none, and the capability table points to webcpp::hash for them

#### Scenario: crypto_auth is in crypto
- **WHEN** a program makes `ciphers::auth` (libsodium's `crypto_auth`) under a 32-byte key and verifies a tag
- **THEN** the verification is constant time and a changed tag returns `authentication_failed`

### Requirement: Capability table: one committed source
The library SHALL hold its capability table in version 0.1's `doc/tables/crypto.json`, listing for every algorithm, by family and name, each backend's targets (native, wasip2, wasip3, emscripten), whether its availability is decided at run time, whether it is constant time on each target, the family's default backend per target with its reason, and the increment that adds it. Version 0.1's generator `doc/tables/generate.py` MUST generate from it, beside version 0.1's pages, `include/webcpp/crypto/detail/capabilities.hpp`, `include/webcpp/crypto/supports.hpp`'s table and `test/capabilities_test.cpp`; a cell MUST be backed by a test that runs on its targets, and `--check` MUST fail when a generated file differs from what the table gives. No `meta/capabilities.json` and no superproject tool of crypto's MAY hold a second copy of these facts.

#### Scenario: A hand edit of the generated header
- **WHEN** a line of `detail/capabilities.hpp` is changed by hand and `python3 libs/crypto/doc/tables/generate.py --check` runs
- **THEN** it fails, naming the file

#### Scenario: A cell without its specialisation
- **WHEN** the table marks `ed25519` present on `backends::botan` and no Botan engine for it exists
- **THEN** `capabilities_test` fails to compile on the targets where the cell is present

### Requirement: Capability table: a backend without a function refuses, naming the backends that have it
The primary engine of every (algorithm, backend) pair that the table marks absent SHALL fail at compile time, only where the algorithm is called, with `webcpp::crypto: <backend> has no <algorithm>. The backends that have it: <backend> (<targets>), ...`, the list generated from the table. Each such message MUST come from a test in a directory of its own for at least one absent pair per family.

#### Scenario: Ed25519 on BearSSL
- **WHEN** a program calls `curves::ed25519::basic_keypair<backends::bearssl>::make(seed)`
- **THEN** compilation stops with `webcpp::crypto: bearssl has no Ed25519. The backends that have it: sodium (native, wasip2, wasip3, emscripten), openssl (native, wasip2, wasip3), botan (native, emscripten), browser (emscripten, asynchronous).` as the table of that day gives it

### Requirement: Capability table: the supports trait and run-time availability
`webcpp::crypto::supports_v<Algorithm, Backend>` SHALL be `true` exactly where the table marks the backend as having the algorithm on the current target. A cell whose availability is decided at run time MUST be present in the trait, and its `make` or `async_make` MUST return `boost::system::errc::not_supported` on a machine or browser that lacks it, before reading a key byte.

#### Scenario: AES256-GCM on libsodium without AES instructions
- **WHEN** `ciphers::aes256_gcm_sodium::make(key)` runs where `crypto_aead_aes256gcm_is_available()` returns 0
- **THEN** it returns `not_supported`, and `supports_v<aes256_gcm_sodium, backends::sodium>` is `true` natively and `false` on WebAssembly

### Requirement: Default backend: one per family and target, by a stated reason
Each family SHALL have one default backend per target, `default_backend_t<Family>`, written with its reason in the family's row of `doc/tables/crypto.json` and never changed by the backends a program links: AES-GCM OpenSSL natively and BearSSL on wasip2, wasip3 and Emscripten; the AES block cipher none; the ChaCha20-Poly1305 AEADs, Ed25519, X25519, the boxes, secretbox, secretstream and the rest of libsodium's families libsodium; secp256k1 libsecp256k1; the P-curves and RSA BearSSL; AES-CTR, CBC, CCM and EAX BearSSL until OpenSSL has them, then AES-GCM's rule; the families only OpenSSL and Botan have OpenSSL on native, wasip2 and wasip3 and Botan on Emscripten; BLS12-381, KZG, BN254 and Shamir their only backend (`design.md` D32). A program that uses a default it has not linked MUST stop at compile time with a message that names the Jamfile line linking it and the type that names another backend, from a test of its own. The documentation MUST give each default with its reason.

#### Scenario: Ed25519 without libsodium
- **WHEN** a program links only BearSSL and uses `curves::ed25519::keypair`
- **THEN** compilation stops naming `/third_party//sodium` and `curves::ed25519::basic_keypair<Backend>` with the backends that have Ed25519

#### Scenario: A native program links both AES-GCM backends
- **WHEN** a native program links OpenSSL and BearSSL and uses `ciphers::aes256_gcm`
- **THEN** its backend is OpenSSL, and `ciphers::gcm<ciphers::aes256<backends::bearssl>>` is BearSSL

### Requirement: Default backend: no profile, no override, no measured default
No macro SHALL change a default: the library MUST NOT read `WEBCPP_CRYPTO_PROFILE_*`, `WEBCPP_PROFILE_*` or a per-family `WEBCPP_CRYPTO_DEFAULT_BACKEND_<FAMILY>_<NAME>`, and MUST NOT declare `profile`, `profiles` or `profile_name`. Version 0.1's `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` MUST stop compilation with an `#error` naming the type to write in their place. A measurement MUST NOT decide a default, a backend's order or whether an operation has an asynchronous variant. Every backend other than the default MUST stay reachable by naming it in the type.

#### Scenario: A version 0.1 define
- **WHEN** a program defines `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`
- **THEN** compilation stops with a message that says to write `ciphers::gcm<ciphers::aes256<backends::bearssl>>` where the program wants BearSSL

#### Scenario: A former profile define
- **WHEN** a program defines `WEBCPP_PROFILE_SMALLEST` and uses `ciphers::chacha20_poly1305`
- **THEN** its backend is libsodium, as without the define

### Requirement: Measurements: references from release builds that decide nothing
The own lane `sizes` SHALL build the baseline program and one program per (family, backend) through the superproject's `webcpp.program-sizes`, which requires `<variant>release` and, by the owner's decision of 2026-11-17 (C36), sizes plain linked programs on every target beside `webcpp.size-table`'s Emscripten page programs (foundation Task 11'), on native, wasip2, wasip3 and Emscripten, and the documentation's size matrix MUST be generated from its table. Throughput MUST be measured only from programs built at `variant=release`, by a collect script that refuses any other program or data, and published as a dated page that names the machines. No size or throughput MAY decide a default, an order of backends or an API.

#### Scenario: A debug build is offered to the throughput page
- **WHEN** the collect script is given a program built at `variant=debug`
- **THEN** it refuses it, saying that tables are built only from release builds, and writes nothing

#### Scenario: A pin moves
- **WHEN** libsodium's pin moves
- **THEN** the size matrix is regenerated from the lane's new table, and no default changes

### Requirement: Interface: every family keeps the genesis's shape
Every algorithm type SHALL state its sizes as `static constexpr` members, take its backend as its last template parameter with `default_backend_t<Family>` as default, and every keyed object MUST be made by `static make(...)` returning `boost::system::result<T>`, be move-only, wipe its secrets on destruction and on move, and answer `invalid_state` from every function when empty. A keyed object that can be drawn at random MUST also offer `static generate()`, which draws its seed from `random_device` and returns `entropy_unavailable` when the source fails. A backend's header MUST specialise only the family's `detail::<family>_state` and `detail::<family>_engine` traits; the guards MUST be written once, in the family's type.

#### Scenario: A moved-from Ed25519 key pair
- **WHEN** `auto b = std::move(a);` and then `a.sign(message)` on an `curves::ed25519::keypair`
- **THEN** the result holds `invalid_state`, and `b` signs as `a` would have

### Requirement: Interface: secret outputs wipe themselves
`webcpp::crypto::secret<N>` SHALL hold `N` bytes, wipe them by volatile stores on destruction and on move, be copyable only through `clone()`, and convert to `std::span<std::uint8_t const, N>`. Every function whose output is secret (a shared secret, session keys, a converted secret key, a KEM's shared secret, a tweaked secret key) MUST return it.

#### Scenario: A shared secret goes out of scope
- **WHEN** the wipe test destroys a `secret<32>` from `curves::x25519::agree` and inspects its storage at -O2
- **THEN** the 32 bytes are zero

### Requirement: Interface: one AEAD template for the one-shot AEADs
`ciphers::aead<Family, Backend>` SHALL implement the guards, the in-place `seal` and `open`, the tag apart, the zeroing of `data` after a failed `open` and the move rules of the genesis's `ciphers::chacha20_poly1305` for every one-shot AEAD, its family tag giving `key_size`, `iv_size`, `tag_size` and `max_message_size`. `ciphers::chacha20_poly1305` (12-byte nonce), `ciphers::chacha20_poly1305_original` (8-byte nonce), `ciphers::xchacha20_poly1305` (24-byte nonce), `ciphers::aegis128l`, `ciphers::aegis256` and `ciphers::aes256_gcm_sodium` MUST be its aliases; `ciphers::gcm<Cipher>` keeps its own class and its incremental seal.

#### Scenario: XChaCha20-Poly1305's nonce
- **WHEN** `ciphers::xchacha20_poly1305::seal` is called with a 12-byte nonce
- **THEN** it returns `invalid_iv_size` and `data` is untouched

### Requirement: Errors: the values of the foundation
The category SHALL add these fixed values, never reused: 15 `weak_public_key` (a public key of small order, or one giving an all-zero shared secret), 16 `invalid_point` (an encoding that is not a canonical group element), 17 `invalid_scalar` (a scalar not reduced, or zero where zero is refused), 18 `invalid_padding` (one value for every cause of a padding that cannot be removed), 19 `invalid_encoding` (hexadecimal or base64 that does not decode), 20 `invalid_ciphertext_size` (a KEM or RSA ciphertext of another size than the key's), each with its message.

#### Scenario: A small-order X25519 key
- **WHEN** `curves::x25519::agree` is given a public key of order 8
- **THEN** it returns `weak_public_key`

### Requirement: Backend libsodium: the shared target with crypto's canary
`backends::sodium` SHALL use `/third_party//sodium` as webcpp::hash builds it (libsodium 1.0.22 at `77e1ce5d6dee871c49ef211222ba18ef0c486bda`, every `.c` file one object, no assembly), to which this change adds `libs/crypto/src/canary_sodium.c` and the usage requirement `WEBCPP_CRYPTO_HAS_SODIUM=1`, with the three layers of chapter 5: the header's `#error` with the Jamfile line, the canary `webcpp_crypto_sodium_77e1ce5_<configuration>__add_third_party_sodium_to_your_link` read by every function that calls libsodium, and MSVC's `detect_mismatch`. A program MUST link one copy of libsodium whether it uses crypto, hash or both.

#### Scenario: libsodium named by crypto and hash
- **WHEN** a program names `/webcpp/crypto//crypto`, `/webcpp/hash//hash` and `/third_party//sodium` and uses Ed25519 and Argon2id
- **THEN** its link line holds one libsodium archive, and both canaries resolve

#### Scenario: libsodium not linked
- **WHEN** a program includes `<webcpp/crypto/backends/sodium.hpp>` without linking libsodium
- **THEN** compilation stops with the line `exe app : app.cpp /webcpp/crypto//crypto /third_party//sodium ;`

### Requirement: Backend libsodium: initialisation and entropy
Every engine of `backends::sodium` SHALL call `sodium_init()` before its first libsodium call and treat a return of -1 as `backend_failure`. libsodium's generator MUST be its default, which reads the system natively and on WASI and `crypto.getRandomValues` on Emscripten: the build MUST NOT select `randombytes_internal`, and webcpp MUST NOT call `randombytes_set_implementation`. webcpp's own key generation MUST draw seeds from `random_device` and call libsodium's `seed_keypair` functions.

#### Scenario: A key pair from generate
- **WHEN** `curves::ed25519::keypair::generate()` runs with the test's failing entropy source
- **THEN** it returns `entropy_unavailable` and libsodium's generator is never called

### Requirement: Ed25519: key pairs, signatures and strict verification
`curves::ed25519::basic_keypair<Backend>` SHALL be made from a 32-byte seed (RFC 8032) or by `generate()`, give its 32-byte public key, sign a message into 64 bytes, and give its X25519 secret key as `secret<32>`; `curves::ed25519::verify<Policy, Backend>(public_key, message, signature)` MUST return nothing on success and `invalid_signature` otherwise. On libsodium verification is strict (canonical encodings, no small-order key, `S` reduced); the documentation MUST say which inputs other backends accept that libsodium refuses, as Wycheproof's EdDSA file shows.

#### Scenario: RFC 8032's test 1
- **WHEN** the key pair is made from RFC 8032 section 7.1's first secret key and signs the empty message
- **THEN** the public key and the signature are the RFC's, and `curves::ed25519::verify` accepts them

#### Scenario: A small-order public key
- **WHEN** `curves::ed25519::verify<curves::ed25519::strict, backends::sodium>` is given a small-order public key from Wycheproof
- **THEN** it returns `invalid_signature`

### Requirement: Ed25519: a named verification policy and the twelve edge cases
`curves::ed25519::verify<Policy, Backend>` SHALL take a verification policy, `curves::ed25519::strict` by default and the only one declared: `S` below the group order, `A` and `R` canonical encodings, neither of small order, as libsodium's `crypto_sign_verify_detached` decides. Every Ed25519 backend MUST give libsodium's verdicts under `strict`. The twelve cases of "Taming the many EdDSAs" (Chalkias, Garillot, Nikolaenko, 2020; `novifinancial/ed25519-speccheck` at `65519336fda78a3d016e947df6d82848aca0c9da`) MUST run against every Ed25519 backend, raw and under `strict`, and against WebCrypto in node as an outside view; the verdicts MUST be committed in `meta/ed25519_cases.json`, asserted by the test, and published as a generated table, with the documentation explaining the interop risk with peers using ed25519-dalek's default verification. `curves::ed25519::zip215` MUST NOT be declared until a mature C or C++ implementation of ZIP-215 passes every ZIP-215 vector with the same verdicts as `ed25519-zebra` (open question O1).

#### Scenario: A non-canonical R
- **WHEN** the speccheck case with a non-canonical `R` is verified with `verify<strict, backends::sodium>`
- **THEN** it returns `invalid_signature`, as `meta/ed25519_cases.json` records

#### Scenario: A verdict changes with a pin move
- **WHEN** a backend's pin moves and its raw verdict on one of the twelve cases changes
- **THEN** the test fails until `meta/ed25519_cases.json` and its table are regenerated in that change

### Requirement: Ed25519ph: an incremental signer
`curves::ed25519::ph<Hash, Backend>`, with `Hash` webcpp::hash's `sha2_512` as RFC 8032 requires, SHALL sign and verify a message given in pieces (`begin`, `update`, `sign` or `verify`) through libsodium's `crypto_sign_ed25519ph_*`, and refuse another hash at compile time.

#### Scenario: RFC 8032's Ed25519ph vector
- **WHEN** RFC 8032 section 7.3's message is given in two pieces
- **THEN** the signature is the RFC's

### Requirement: X25519: agreement and the conversion
`curves::x25519` SHALL give a public key from a 32-byte secret, agree a `secret<32>` from a secret and a peer's public key, and return `weak_public_key` when the result is all zero on every backend. `curves::ed25519::to_x25519_public(public_key)` MUST return the birational map's result or `invalid_public_key` for a key that is not canonical or of small order.

#### Scenario: RFC 7748's agreement
- **WHEN** Alice's and Bob's keys of RFC 7748 section 6.1 agree in both directions
- **THEN** both give the RFC's shared secret

#### Scenario: Conversion agrees with signing keys
- **WHEN** a key pair's `x25519_secret()` gives a public key through `curves::x25519::public_key`
- **THEN** it equals `curves::ed25519::to_x25519_public` of the key pair's Ed25519 public key

### Requirement: Boxes: crypto_box, sealed boxes and secretbox
The library SHALL offer `hybrid::box` (X25519, XSalsa20-Poly1305) and `hybrid::box_xchacha20poly1305`, made from a secret key and a peer's public key so that the shared key is computed once (libsodium's `beforenm`), with `seal` and `open` in place and the 16-byte tag apart; `hybrid::sealed_box::seal(recipient_public_key, data, out)` and `sealed_box_keypair::open`; and `ciphers::secretbox` and `ciphers::secretbox_xchacha20poly1305`, keyed objects over 32-byte keys with 24-byte nonces. A failed `open` MUST zero `data`, as the genesis's AEADs do.

#### Scenario: A sealed box opened by its recipient
- **WHEN** a message is sealed to a recipient's public key and opened with the recipient's key pair
- **THEN** it opens, and a sealed box changed in one byte returns `authentication_failed` with `data` zeroed

### Requirement: Key formats: SPKI, PKCS#8 and PEM, strict
Each curve's increment SHALL add, in its namespace, `encode_spki`, `decode_spki`, `encode_pkcs8` and `decode_pkcs8` (into the caller's buffer, with `max_spki_size` and `max_pkcs8_size`, nothing written on refusal), and the root `pem::armour` and `pem::unarmour` (labels `PUBLIC KEY` and `PRIVATE KEY`, lines of 64 characters, LF): Ed25519 and X25519 per RFC 8410 in increment 1 (on libsodium, RFC 8410's fixed DER prefix and the 32 key bytes, a recorded constant), secp256k1 per RFC 5480 with its OID in increment 2, P-256, P-384 and P-521 per RFC 5480 and RFC 5915 in increment 3 (PKCS#8 and PEM on BearSSL, SPKI on OpenSSL and Botan), RSA with PKCS #1 inside SPKI and PKCS#8 with RSA, and on OpenSSL and Botan for every key type they have. Decoding MUST accept an encoding only when re-encoding what was decoded gives the same bytes, on every backend, and refuse anything else with `invalid_encoding`. JWK MUST NOT be offered. Tests MUST run round trips, malformed-input refusals, and OpenSSL natively and WebCrypto's `importKey` and `exportKey` (`spki`, `pkcs8`) under node as outside oracles; for secp256k1, which WebCrypto lacks, node's `crypto` module (`createPrivateKey`, `createPublicKey`, `export`) is the oracle under node.

#### Scenario: An Ed25519 key from WebCrypto
- **WHEN** node's WebCrypto exports an Ed25519 key pair as `pkcs8` and `spki` and the library decodes both on libsodium
- **THEN** the key pair signs as WebCrypto's does, and encoding it again gives the same bytes

#### Scenario: PKCS#8 version 2 is refused
- **WHEN** `decode_pkcs8` is given an Ed25519 PKCS#8 that carries the public key (version 2)
- **THEN** it returns `invalid_encoding` and writes nothing, on libsodium and on OpenSSL alike

#### Scenario: A libp2p PeerId's key
- **WHEN** a P-256 public key's SPKI DER from OpenSSL is decoded by `curves::p256::decode_spki` on OpenSSL and re-encoded
- **THEN** the bytes are identical, as a libp2p PeerId of an ECDSA key requires

### Requirement: Key exchange: crypto_kx
`curves::x25519::kx_keypair<Backend>` SHALL compute `client_session_keys(server_public_key)` and `server_session_keys(client_public_key)`, each returning a receive key and a transmit key as `secret<32>`, through libsodium's `crypto_kx`, and return `weak_public_key` where libsodium refuses the peer's key.

#### Scenario: Client and server agree
- **WHEN** a client and a server compute their session keys from each other's public keys
- **THEN** the client's transmit key is the server's receive key and the reverse

### Requirement: libsodium completed: secretstream, AEGIS, AES256-GCM, MACs and stream ciphers
Increment 5 SHALL add `ciphers::secretstream` (XChaCha20-Poly1305 secretstream: a header, `push` and `pull` with the tags message, push, rekey and final, each chunk verified before its plaintext is released, `invalid_state` after the final tag), `ciphers::aegis128l`, `ciphers::aegis256`, `ciphers::aes256_gcm_sodium` (run-time available, native only), `ciphers::auth` and `ciphers::poly1305` MACs, and in `ciphers` the stream ciphers `chacha20`, `chacha20_original`, `xchacha20`, `salsa20`, `xsalsa20`, `salsa20_12` and `salsa20_8`, and the cores `hchacha20` and `hsalsa20`, all on libsodium.

#### Scenario: A secretstream chunk tampered with
- **WHEN** the second of three chunks is changed in one byte before `pull`
- **THEN** `pull` returns `authentication_failed`, its output is zeroed, and the stream answers `invalid_state` afterwards

### Requirement: libsodium completed: groups, ML-KEM-768 and X-Wing
Increment 5 SHALL add `curves::ristretto255::point` and `curves::ristretto255::scalar`, `curves::edwards25519::point` and `curves::edwards25519::scalar` (validation, addition, subtraction, `from_hash`, `from_uniform` for edwards25519, scalar multiplication with and without the base, scalar invert, negate, complement, add, sub, mul, reduce, random), returning `invalid_point` and `invalid_scalar` where libsodium refuses; and `post_quantum::ml_kem_768` and `post_quantum::x_wing` key pairs from a seed or `generate()`, `encapsulate` returning a ciphertext and a `secret<32>`, a deterministic `encapsulate` for tests, and `decapsulate`, which returns `invalid_ciphertext_size` for a ciphertext of the wrong size.

#### Scenario: X-Wing round trip
- **WHEN** a key pair from a seed encapsulates to its public key and decapsulates the ciphertext
- **THEN** both shared secrets are equal, and equal the X-Wing draft's vector for that seed

### Requirement: libsodium completed: utilities
Increment 5 SHALL add `random_uniform(upper_bound)` (unbiased, libsodium's `randombytes_uniform`), `random_bytes_deterministic(seed, out)`, the constant-time memory functions `is_zero`, `compare`, `increment`, `add` and `sub`, `pad` and `unpad` (ISO/IEC 7816-4, `invalid_padding`), constant-time hexadecimal and base64 in libsodium's four base64 variants (`invalid_encoding`), `ipcrypt` (libsodium's IP address encryption), and natively `lock`, `unlock` and `guarded_memory_resource<backends::sodium>`, a `std::pmr::memory_resource` over `sodium_malloc` with guard pages, refused at compile time on WebAssembly.

#### Scenario: An unbiased shuffle
- **WHEN** a Fisher-Yates shuffle draws its indices with `random_uniform(i + 1)`
- **THEN** every index is below its bound, and a chi-squared test over a million shuffles of four elements passes

### Requirement: secp256k1: recovery, raw ECDH, tweaks and key forms
Increment 2 SHALL add, as thin wrappers over libsecp256k1 v0.8.0: `sign_recoverable` and `recover` (65-byte signatures with the recovery id); `ecdh_x` (the shared point's 32-byte x coordinate) and `ecdh_point` (its 65-byte uncompressed form) as `secret<32>` and `secret<65>`; `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate` and `pubkey_combine`; `parse_public_key` of 33 and 65 bytes and `serialize_public_key` in both forms; and `sign_ecdsa` with 32 bytes of extra entropy (hedged signing).

#### Scenario: Ethereum's recovery
- **WHEN** a recoverable signature of a digest is made and `recover` is given the signature and the digest
- **THEN** it returns the signer's public key, for random keys and for libsecp256k1's own recovery test cases

#### Scenario: Nostr's shared x coordinate
- **WHEN** two key pairs compute `ecdh_x` with each other's public keys
- **THEN** both give the same 32 bytes, the x coordinate of the shared point, unhashed

### Requirement: secp256k1: ElligatorSwift, MuSig2 and silent payments
Increment 2 SHALL add ElligatorSwift (`ellswift_encode`, `ellswift_create`, `ellswift_xdh` with BIP324's hash function), MuSig2 per BIP327 (key aggregation with its tweaks, `musig::secret_nonce`, a move-only object consumed by `partial_sign` so that a nonce is never used twice, partial signature verification and aggregation) and silent payments per BIP352 (the sender's outputs and the recipient's scan with labels), checked against the BIP vectors in the `bips` submodule.

#### Scenario: A MuSig2 nonce passed without a move
- **WHEN** a program passes a `secret_nonce` lvalue to `partial_sign`
- **THEN** it does not compile, because `partial_sign` takes the nonce by value and the nonce cannot be copied

#### Scenario: A MuSig2 nonce used twice
- **WHEN** a program passes to `partial_sign` a `secret_nonce` already moved into an earlier call
- **THEN** `partial_sign` returns `invalid_state` without calling libsecp256k1, since C++ cannot reject a second `std::move` of one object at compile time

#### Scenario: BIP352's vectors
- **WHEN** every sending and receiving case of BIP352's test vectors runs
- **THEN** the outputs and the found outputs are the vectors'

### Requirement: ChaCha20-Poly1305: on libsodium, BearSSL and OpenSSL in version 0.2
Increment 1 SHALL add `ciphers::chacha20_poly1305` (RFC 8439, a 32-byte key, a 12-byte nonce, a 16-byte tag) over `ciphers::aead<Family, Backend>` on libsodium (every target), BearSSL (every target) and OpenSSL (native, wasip2, wasip3), which is the genesis's increment 4 built on the foundation's AEAD template. Its tests MUST be the genesis's: RFC 8439's vectors, Wycheproof's ChaCha20-Poly1305 file on each backend, the cross-backend agreement of the three, the counting allocation tests, OpenSSL's error queue left as found, ctaes's refusal from a message test of its own, the constant-time cases, and, once the memory option has merged, its part of the memory option's tests.

#### Scenario: Three backends agree
- **WHEN** `agree_aead<chacha20_poly1305>` runs natively on libsodium, BearSSL and OpenSSL from one seed
- **THEN** every ciphertext and tag is identical and each backend opens the others'

#### Scenario: ctaes has no ChaCha
- **WHEN** a program names `ciphers::aead<families::chacha20_poly1305, backends::ctaes>`
- **THEN** compilation stops naming the backends that have ChaCha20-Poly1305

### Requirement: Backend BearSSL: the remaining parts
Increment 3 SHALL add on BearSSL (whose ChaCha20-Poly1305 comes in increment 1), on every target: `curves::p256::ecdsa`, `curves::p384::ecdsa` and `curves::p521::ecdsa` (RFC 6979 nonces, raw and DER signatures, over a digest the caller gives), `curves::p256::ecdh`, `curves::p384::ecdh` and `curves::p521::ecdh`, `curves::x25519`, `ciphers::ctr<ciphers::aesN>`, `ciphers::cbc<ciphers::aesN>` with PKCS #7 padding (`invalid_padding` for every cause), `ciphers::ccm<ciphers::aesN, TagSize>`, `ciphers::eax<ciphers::aesN>`, and RSA (`rsa::pkcs1<Hash>` and `rsa::pss<Hash>` signatures, `rsa::oaep<Hash>` encryption, key generation from `random_device`). BearSSL's DRBGs MUST NOT be exposed.

#### Scenario: A passkey's ES256 signature
- **WHEN** `verify` of `curves::p256::ecdsa<backends::bearssl>` is given a WebAuthn assertion's DER signature, the digest and the credential's public key
- **THEN** it accepts it, and refuses it with one bit of the digest changed

### Requirement: P-curves: DER signatures and public key forms
Increment 3 SHALL add, as primitives on BearSSL for P-256, P-384 and P-521 by the same code: `curves::p256::to_der` and `from_der` shaped as `curves::secp256k1`'s (`to_der` into the caller's buffer of at least `max_der_size` bytes, 72 for P-256, returning the length; `from_der` returning the raw `r || s`; a refusal writing nothing), decoding strict DER only (BearSSL's `br_ecdsa_asn1_to_raw`, then re-encoded by `br_ecdsa_raw_to_asn1` and required to give the same bytes); and `curves::p256::public_key` parsed from the uncompressed point (`0x04 || x || y`, 65 bytes) or from raw 32-byte `x` and `y` coordinates, refusing a point off the curve or the identity with `invalid_public_key`, and serialised to the uncompressed form. The compressed form MUST be present only on backends that have it (OpenSSL, Botan), its BearSSL cells absent. The documentation MUST say these serve WebAuthn's ES256 verification and that COSE parsing and WebAuthn's protocol stay outside crypto. Tests MUST run Wycheproof's `ecdsa_secp256r1_sha256_test.json` (DER), its P-384 and P-521 DER files, and NIST's ECDSA signature and public-key validation vectors (ACVP's JSON, or CAVP's archives once gate O3 pins them).

#### Scenario: A non-canonical DER signature
- **WHEN** `from_der` is given a Wycheproof case whose DER has a leading zero byte it does not need
- **THEN** it returns `invalid_signature` and writes nothing

#### Scenario: A COSE key's coordinates
- **WHEN** `public_key` is made from the raw `x` and `y` of a passkey's COSE key and verifies an assertion's signature after `from_der`
- **THEN** the verification succeeds, and the same `x` with `y` changed by one is refused as `invalid_public_key`

#### Scenario: A short buffer
- **WHEN** `to_der` is given a 70-byte buffer for a signature whose DER takes 72 bytes
- **THEN** it returns `invalid_input_size` and the buffer is untouched

### Requirement: Backend browser: WebCrypto on Emscripten
`backends::browser` SHALL exist on Emscripten only, its header stopping another target with the fix; its keys MUST be WebCrypto `CryptoKey` objects held in a JavaScript table and named in C++ by a handle, freed when their last owner is destroyed and kept alive while an operation uses them. It MUST offer AES-GCM, AES-CBC, AES-CTR, AES-KW, ECDSA and ECDH on P-256, P-384 and P-521, RSA-PSS, RSA-OAEP and RSASSA-PKCS1-v1_5, and Ed25519 and X25519 as run-time-available algorithms; it MUST have no synchronous member, and calling one MUST fail to compile with "backends::browser is asynchronous: call async_<name>".

#### Scenario: A synchronous call on the browser
- **WHEN** a program calls `seal` on `ciphers::gcm<aes256<backends::browser>>`
- **THEN** compilation stops with `backends::browser is asynchronous: call async_seal`

### Requirement: Asynchronous family: completion tokens, for what the browser offers
`<webcpp/crypto/async.hpp>` SHALL offer `async_make`, `async_generate`, `async_seal`, `async_open`, `async_encrypt`, `async_decrypt`, `async_sign`, `async_verify`, `async_agree`, `async_wrap`, `async_unwrap` and `async_export` for every algorithm WebCrypto offers, each an Asio asynchronous operation in webcpp::asio's model (initiated through `async_initiate`, any completion token: a callback, `use_awaitable`, Cobalt's, `use_future`) completing with `void(boost::system::error_code, T)`. On `backends::browser` each MUST call WebCrypto, with or without pthreads, and await its promise through `webcpp::asio::browser::async_await`, completing on the handler's associated executor; on a synchronous backend each MUST run the synchronous call and complete through `asio::post`, never inside the initiating call. An `async_` variant of a cheap algorithm WebCrypto lacks MUST NOT exist. The synchronous API MUST NOT call WebCrypto. The convenience header MUST leave out `async.hpp` and `backends/browser.hpp`, naming webcpp::asio as the reason.

#### Scenario: One coroutine, two targets
- **WHEN** a coroutine `co_await async_seal(aead, iv, ad, data, use_awaitable)` runs natively on BearSSL and on Emscripten on the browser
- **THEN** both complete with the same ciphertext and tag for the same key, IV and message

#### Scenario: A callback is never called inline
- **WHEN** `async_seal` on a synchronous backend is given a callback that sets a flag
- **THEN** the flag is unset when `async_seal` returns, and set after the executor runs

#### Scenario: No asynchronous variant of a cheap algorithm the browser lacks
- **WHEN** a program calls `async_seal` on `ciphers::xchacha20_poly1305`
- **THEN** it does not compile, and `ciphers::xchacha20_poly1305::seal` does

### Requirement: Heavy operations run on another thread, only in a build with threads
`config.hpp` SHALL define `WEBCPP_CRYPTO_HAS_THREADS` natively when `BOOST_HAS_THREADS` is defined and on Emscripten when `__EMSCRIPTEN_PTHREADS__` is defined, never on wasip2 or wasip3, unless `WEBCPP_CRYPTO_NO_THREADS` is defined, as webcpp::hash's `WEBCPP_HASH_HAS_THREADS` does. The heavy operations SHALL be the ones `design.md` D32 lists, never a set generated from measurements: RSA key generation at every size and RSA private operations at 3072 bits and above; SLH-DSA key generation, signing and verification; ML-DSA key generation and signing; BLS12-381 verification, aggregate verification, fast aggregate verification and aggregation; KZG's commitments and proofs over a blob and their batch verification; BN254's `pairing_check`. Operations whose cost the caller's data size sets (AEADs, boxes, secretstream, stream ciphers, MACs) MUST NOT be heavy. With threads, every heavy operation MUST have an `async_` variant that takes an executor first (natively a `webcpp::asio::thread_pool`'s, on Emscripten a pool whose threads are Web Workers), runs the work there, and completes on the handler's associated executor; without threads, calling it MUST fail to compile with "webcpp::crypto: <name> runs on another thread and needs a build with threads (natively b2 threading=multi; on Emscripten -pthread). Without threads, call <synchronous name>.", except on `backends::browser`, which needs no thread. Cheap operations MUST stay synchronous only, unless the browser offers them.

#### Scenario: RSA key generation off the caller's thread
- **WHEN** a native test built with `threading=multi` calls `async_generate` for `rsa::pss<sha2_256>` at 3072 bits on BearSSL with a `thread_pool`'s executor and a callback bound to its own `io_context`, and runs a 10 ms timer on that `io_context`
- **THEN** the key is generated on a pool thread, the timer fires before it completes, and the key signs and verifies

#### Scenario: Without threads
- **WHEN** a program built with b2's default `threading=single` calls `async_aggregate_verify` of `curves::bls12_381::min_pk`
- **THEN** compilation fails with "webcpp::crypto: async_aggregate_verify runs on another thread and needs a build with threads (natively b2 threading=multi; on Emscripten -pthread). Without threads, call aggregate_verify."

#### Scenario: A Web Worker on Emscripten
- **WHEN** an Emscripten program built with `-pthread` calls `async_sign` of `slh_dsa` with a pool under node
- **THEN** the signature is computed on a worker, the main thread's timer keeps firing, and the completion reaches the main thread

### Requirement: Asynchronous family: the browser's errors
A rejected WebCrypto promise SHALL map to the library's errors: `OperationError` on a decryption or an unwrap to `authentication_failed`, `NotSupportedError` to `boost::system::errc::not_supported`, `DataError` to the size or key error of the call, and anything else to `backend_failure`; a cancelled operation completes with `boost::asio::error::operation_aborted` and its output zeroed.

#### Scenario: A browser without Ed25519
- **WHEN** node runs `async_generate` for Ed25519 with `Ed25519` removed from `crypto.subtle`
- **THEN** it completes with `not_supported`

### Requirement: Backend blst: BLS12-381
Increment 6 SHALL add `backends::blst` over blst v0.3.17 (`54e6e55674722fc2797ebb4bbb71b26d881eb4b8`), the submodule `third_party/libs/blst`, built as upstream's two translation units (`src/server.c`, `build/assembly.S`) with the assembly and `__BLST_PORTABLE__` natively on Linux and macOS and `__BLST_NO_ASM__` on MSVC and WebAssembly, the configuration in the canary's name. It MUST offer `curves::bls12_381::min_pk` and `curves::bls12_381::min_sig`, each with `secret_key::make(ikm)` (the IETF BLS draft's `KeyGen`), `sign`, `verify`, `aggregate`, `aggregate_verify`, `fast_aggregate_verify`, proofs of possession, and `hash_to_g1` and `hash_to_g2` with a caller's domain separation tag; pairing, field arithmetic and blst's EIP-2333 key tree MUST NOT be exposed.

#### Scenario: The consensus vectors
- **WHEN** the BLS vectors of Ethereum's consensus specifications run on `min_pk`
- **THEN** every sign, verify, aggregate and fast aggregate verify case gives the expected result

### Requirement: Backend Botan: a pinned submodule built by b2, with exceptions
Increment 7 SHALL make Botan the submodule `third_party/libs/botan` at 3.14.1 (`01af88c039fcfaedc418b1af4d6d3a139674e591`), configured by upstream's `configure.py` per target into the b2 variant directory with the modules webcpp exposes, and compiled by b2 one object per source from the list it writes, never patched, with crypto's canary and three layers. Botan MUST be available natively and on Emscripten; on wasip2 and wasip3 the table's cells MUST be what increment 7's spike finds (present where Botan's build passes its own tests under wasmtime). Its header MUST declare nothing without exceptions (the genesis's rule stands), every Botan call MUST sit in one `try` at the boundary, and Botan's randomness MUST come from `detail::botan_random`, which draws from `random_device`.

#### Scenario: Botan on Emscripten
- **WHEN** an Emscripten program names `/third_party//botan` and signs with `curves::ed448`
- **THEN** it builds and the signature verifies against RFC 8032's Ed448 vector

#### Scenario: Botan's own RNGs
- **WHEN** a Botan engine needs randomness
- **THEN** it receives `detail::botan_random`, and no `Botan::System_RNG` or `AutoSeeded_RNG` is constructed anywhere in the library

### Requirement: Backend OpenSSL: the remaining algorithms
Increment 8 SHALL add on OpenSSL, through `/webcpp/crypto//openssl` on native, wasip2 and wasip3: the P-curves' ECDSA and ECDH, Ed25519, Ed448, X25519, X448, ML-KEM, ML-DSA, SLH-DSA, RSA, AES-CTR, CBC, CCM, OCB, KW, KWP, SIV, GCM-SIV and CMAC (its ChaCha20-Poly1305 comes in increment 1). A keyed object MUST hold its `EVP_PKEY` and a context initialised in `make`, and the documentation MUST record, per algorithm, what OpenSSL allocates per operation as the counting tests measure it.

#### Scenario: ML-DSA's ACVP vectors
- **WHEN** NIST ACVP's ML-DSA-65 signature generation and verification cases run on OpenSSL
- **THEN** every case gives the expected result

### Requirement: Hash-to-curve: from released libraries only
Hash-to-curve per RFC 9380 SHALL come from blst (BLS12-381 G1 and G2) and Botan (the P-curves); ristretto255's and edwards25519's maps from a hash (`from_hash`, `from_uniform`) come from libsodium 1.0.22. The full `hash_to_curve` for ristretto255 and edwards25519 MUST join only with a libsodium release that holds `crypto_core_*_from_string`, moving the pin shared with webcpp::hash; webcpp MUST NOT write `expand_message_xmd` itself.

#### Scenario: RFC 9496's one-way map
- **WHEN** RFC 9496's `from_hash` vectors run on `curves::ristretto255::point::from_hash`
- **THEN** every output encoding is the RFC's

### Requirement: Tests: vectors and suites per backend
Every algorithm SHALL be tested on every backend that has it, on every target that backend builds for, with exceptions on and off where the backend allows, against Wycheproof wherever it has a file for the algorithm, the RFCs and drafts that define it, NIST ACVP's JSON vectors where they exist (a shallow submodule `third_party/libs/acvp`, pinned by the increment that first reads it), and the BIP and EIP vectors of its increment; the own lane `c-suites` MUST run libsodium's `test/default`, Botan's tests of the modules built and blst's own tests on webcpp's build, on each target.

#### Scenario: Wycheproof's X25519 on two backends
- **WHEN** Wycheproof's `x25519_test.json` runs on libsodium and on BearSSL
- **THEN** every valid case gives its shared secret, and every case a backend refuses is one the test lists as that backend's expected refusal

### Requirement: Tests: constant time for the new code
The genesis's constant-time harness (memcheck's client requests, MemorySanitizer, dudect natively and under wasmtime, the planted leak that must fail) SHALL be built in increment 1 (the genesis's Task 20, whole) over version 0.1's backends (BearSSL's AES-GCM, OpenSSL's AES-GCM where it is built with assembly, ctaes, and libsecp256k1's own `ctime_tests.c`), over libsodium's signing, agreement, AEAD and box code, BearSSL's and OpenSSL's ChaCha20-Poly1305, and webcpp's guards, and each later increment MUST extend it to its backend's secret-dependent paths before its last green.

#### Scenario: Ed25519 signing under memcheck
- **WHEN** the seed and the message are marked undefined and `curves::ed25519::keypair::sign` runs under memcheck
- **THEN** no conditional jump depends on them, and the planted branch on the seed fails the same test

### Requirement: Documentation: two readers, and pages in the owner's style
The documentation SHALL let a developer who knows little cryptography and an LLM that collects a person's use case choose correctly from it alone. Every page MUST open with a lead paragraph in webcpp's own words saying what the thing is and what it is for, then how it works, then details; MUST put use cases from ordinary applications first; MUST say what goes wrong without the primitive and what the mistake costs; and every algorithm's section MUST give what it is, a compiled example included by tag with its expected output, and its pitfalls. Wikipedia (CC BY-SA) MUST only be cited by link, never copied; the libsodium families' pages adapt libsodium's documentation (`jedisct1/libsodium-doc` at `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`, ISC) with an attribution line each, and the library MUST ship its notice as `LICENSE-LIBSODIUM-DOC.txt`. Every increment's documentation task MUST follow these rules and the requirements below.

#### Scenario: The X25519 section
- **WHEN** the documentation is built
- **THEN** the X25519 section opens with what X25519 is for in a chat app, holds a running example, and warns about the all-zero secret and about using the shared secret directly as a key

#### Scenario: An adapted page
- **WHEN** the secretbox page is read
- **THEN** it ends with the attribution to libsodium's documentation and its ISC licence

### Requirement: Documentation: a chapter per namespace, opening with a comparison table
Each group of the library (`ciphers`, `curves`, `post_quantum`, `rsa`, `hybrid`, `commitments`, `secret_sharing`, the root utilities) SHALL have a chapter that opens with a comparison table of everything inside it, generated from `doc/tables/crypto.json` and the lanes' tables from release builds: for `ciphers` whether each is an AEAD, key and nonce sizes, nonce-misuse resistance, speed with and without hardware support, constant time per backend, standards, where it is used in the world, WebCrypto availability and "choose it when"; for `curves` what each does, its security level, key and signature sizes, speed, standards, the ecosystems that require it, and "choose it for compatibility with"; the other groups with the columns their algorithms need. Every statement of where an algorithm is used MUST cite a source, and the generator MUST refuse one without.

#### Scenario: The curves table
- **WHEN** the curves chapter is built
- **THEN** its table lists secp256k1 with Bitcoin, Ethereum and Nostr, P-256 with passkeys and WebCrypto, and Ed25519 with SSH and Signal, each with its cited source

### Requirement: Documentation: generated backend lists and matrices
Every API page SHALL list the backends that implement it, with the default per target and its reason, generated from the capability table. A backends chapter MUST hold generated matrices: operations by backends by targets, binary size per target per operation, throughput natively and under node per operation, and constant time per backend and target. No table that states support, size, speed or a property MAY be written by hand: `doc/tables/generate.py` generates them, and its `--check` MUST fail on a stale page.

#### Scenario: A pin move
- **WHEN** a pin moves and the `sizes` lane's table changes without rerunning `generate.py`
- **THEN** `generate.py --check` fails, naming the stale pages

### Requirement: Documentation: choosing by use case, and an index for LLMs
The documentation SHALL hold a "choose by use case" chapter, a decision table generated from `crypto.json`'s `use_cases` whose rows read "I need to ..." and give a namespace, an algorithm, a backend with a link to the example, and an `llms.txt` at its root listing each namespace, each function, its use cases and its backends, generated with the tables.

#### Scenario: An LLM picks a cipher
- **WHEN** the row "encrypt messages in a chat app, on the web and natively" is read
- **THEN** it names `ciphers::xchacha20_poly1305`, its default backend per target, and its example, and `llms.txt` lists the same function with that use case

### Requirement: Documentation: teaching chapters by use case
The documentation SHALL hold teaching chapters on crypto wallets, peer-to-peer protocols, passwords and everyday apps, each giving the problem, the idea, code from a compiled example and the mistakes, framed as raising a program's security; their examples MUST be webcpp's own code written after what a named, linked public application built on noble does, never copying its code, and protocol-level steps (BIP, EIP, NIP) MUST be shown as what a domain library builds from these primitives, not as this library's API.

#### Scenario: The everyday apps chapter
- **WHEN** its sealed-box example is built and run
- **THEN** it prints its expected output, and the chapter names the application whose practice it follows

### Requirement: Scope: primitives only, protocols in domain libraries
webcpp::crypto SHALL hold cryptographic primitives only: algorithms with a specification of their own (an RFC, a NIST standard, an IETF or CFRG draft, a paper) and the public interfaces of its backends' libraries that are such algorithms, libsecp256k1's modules included (recovery, raw ECDH, tweaks by any 32-byte value, x-only tweaks, ElligatorSwift, MuSig2, silent payments). Whatever a protocol specification defines (BIPs, EIPs, SLIPs, NIPs and their like: BIP32 and BIP39 derivation, BIP341's `TapTweak`, sighashes, PSBT, addresses, EIP-712 hashing, EIP-2333 key trees, EIP-4844 versioned hashes, keystores, NIP-44's format, SLIP-39 shares) MUST NOT be in the library: it belongs to domain libraries built on crypto and hash (webcpp::bitcoin, webcpp::ethereum, webcpp::nostr, ...). Increment 2 MUST NOT bring back the Taproot tweak, `taproot_output_key` or `from_lax_der`, which version 0.1 replaces by a generic x-only tweak or moves to webcpp::bitcoin; a protocol's vectors MAY serve as test data for a primitive.

#### Scenario: A tagged tweak is a domain library's
- **WHEN** a program needs BIP341's output key from an internal key and a merkle root
- **THEN** webcpp::crypto offers the x-only tweak by a 32-byte value, and the `TapTweak` tagged hash that computes that value is webcpp::bitcoin's

#### Scenario: A backend function that is a protocol
- **WHEN** a program needs EIP-2333's child key derivation, which blst implements
- **THEN** webcpp::crypto does not expose it, and blst's C API stays reachable for webcpp::ethereum

### Requirement: KZG commitments over c-kzg-4844
Increment 9 SHALL add `backends::ckzg` over c-kzg-4844 v2.1.8 (`e125905e5e01186e6ccb7a0ced4845bf7eddbcfe`), the submodule `third_party/libs/c-kzg-4844`, built file by file against the library's blst submodule, never c-kzg's own. It MUST offer `commitments::kzg::settings::make` from a trusted setup the caller gives, `blob_to_commitment`, `compute_proof`, `verify_proof`, `compute_blob_proof`, `verify_blob_proof`, `verify_blob_proof_batch` and c-kzg's cell functions, with c-kzg's own vectors; the trusted setup MUST NOT be embedded, and versioned hashes MUST NOT be offered.

#### Scenario: c-kzg's vectors
- **WHEN** c-kzg-4844's reference tests run through `kzg` with the ceremony's setup loaded as test data
- **THEN** every commitment, proof and verification gives the expected result

### Requirement: The BN254 pairing over mcl
Increment 10 SHALL add `backends::mcl` over mcl v4.43 (`50e4c6f2826de6b041f857caf9f253cc736ea926`), the submodule `third_party/libs/mcl`, built without its JIT (`MCL_DONT_USE_XBYAK`), offering `curves::bn254::g1` and `curves::bn254::g2` (decoding with subgroup checks, addition, scalar multiplication) and `curves::bn254::pairing_check` for public data only, its capability cells marked not constant time.

#### Scenario: A pairing check
- **WHEN** `pairing_check` is given pairs whose product of pairings is one, and then the same with one point negated
- **THEN** the first succeeds and the second returns `invalid_signature`

### Requirement: Shamir secret sharing over sss
Increment 11 SHALL add `backends::sss` over dsprenkels/sss at `9f2a10b1c0d391bed485c4c7e93788ce37c360f1`, the submodule `third_party/libs/sss` without its `randombytes` submodule, offering `secret_sharing::shamir::split_key`, `secret_sharing::shamir::combine_key`, `secret_sharing::shamir::split` and `secret_sharing::shamir::combine`, refusing `k < 2`, `k > n` and `n > 255` before any call, and feeding sss's `randombytes` only bytes the wrapper drew from `random_device`, reporting `entropy_unavailable` before sss runs.

#### Scenario: Any k shares recover the key
- **WHEN** a 32-byte key is split into 5 shares with threshold 3 and every 3-subset is combined
- **THEN** each gives the key, and a 2-subset gives a different value

#### Scenario: No entropy
- **WHEN** `split_key` runs with the test's failing entropy source
- **THEN** it returns `entropy_unavailable` and sss is not called

### Requirement: HPKE over OpenSSL and Botan
Increment 12 SHALL add `hybrid::hpke<Kem, Kdf, Aead, Backend>` (RFC 9180: the base, PSK, auth and auth-PSK modes, a context with `seal`, `open` and `export` whose sequence number is guarded against overflow, and the single-shot calls) on OpenSSL (`OSSL_HPKE_*`, native, wasip2, wasip3) and on Botan (its `hpke` module, natively, on Emscripten, and on WASI as increment 7 finds), over the suites both have, and MUST NOT compose HPKE from another library's parts.

#### Scenario: RFC 9180's vectors on two backends
- **WHEN** RFC 9180's test vectors for DHKEM(X25519, HKDF-SHA256) with ChaCha20-Poly1305 run on OpenSSL and on Botan natively
- **THEN** both give every ciphertext and exported secret, and each opens the other's

### Requirement: Increments: the foundation's twelve, and its parallel streams
The foundation SHALL proceed in twelve increments ordered by value, in the order of `reference/roadmap.md`: (1) libsodium's core, the capability table, the defaults by reason, ChaCha20-Poly1305 on libsodium, BearSSL and OpenSSL (the genesis's increment 4), the constant-time lane over version 0.1's backends and the new code, the sizes lane and the throughput page from release builds, and the documentation's form; (2) libsecp256k1's remaining modules; (3) BearSSL's remaining parts; (4) the browser backend and the asynchronous family; (5) libsodium completed; (6) blst; (7) Botan as a submodule, which is the genesis's increment 5; (8) OpenSSL's remaining algorithms; (9) KZG over c-kzg-4844; (10) BN254 over mcl; (11) Shamir secret sharing over sss; (12) HPKE. Increments 1 and 2 MUST be able to run beside each other and beside the genesis's memory option (stream M), and form version 0.2; after increment 1 merges, increments 3 to 8, 10 and 11 MUST be able to run as parallel streams; 9 starts when 6 merges, and 12 when 8 merges, its WebAssembly cells joining when 7 merges. A stream merges when green and reviewed, two ready at once in the value order; after each merge the others rebase, merge `doc/tables/crypto.json` by rows and regenerate, never merging a generated file by hand. Each increment ends with the definition of green, a review, and pull requests the owner merges.

#### Scenario: Two streams touch the capability table
- **WHEN** increments 3 and 6 both add rows to `doc/tables/crypto.json` and 3 merges first
- **THEN** 6 rebases, merges the JSON rows, regenerates with `generate.py`, and `--check` passes

#### Scenario: KZG waits for blst
- **WHEN** increment 9 is about to start and increment 6 has not merged
- **THEN** it waits, because `backends::ckzg` builds on the library's blst

## MODIFIED Requirements

### Requirement: Library: header-only target and namespace
The library SHALL be header-only C++20, in namespace `webcpp::crypto`, organised in groups whose headers mirror them as `<webcpp/crypto/<group>/<name>.hpp>`: `ciphers` (block ciphers and their modes, AEADs, stream ciphers, MACs, `secretbox`, `secretstream`, `ipcrypt`), `curves` (one namespace per curve: `secp256k1` with its modules, `ed25519`, `x25519`, `ed448`, `x448`, `p256`, `p384`, `p521`, `brainpool`, `ristretto255`, `edwards25519`, `bls12_381`, `bn254`), `post_quantum` (`ml_kem`, `ml_dsa`, `slh_dsa`, `x_wing`), `rsa`, `hybrid` (`box`, `sealed_box`, `hpke`), `commitments::kzg` and `secret_sharing::shamir`; the root holds `error`, `random_device`, `equal`, the memory utilities, `secret<N>`, the configuration and the defaults (`families`, `tags`, `default_backend_t`, `supports_v`) and the asynchronous family; `webcpp::crypto::backends` holds one tag per backend, outside the groups, with `<webcpp/crypto/backends/<name>.hpp>`. Internals MUST be in `webcpp::crypto::detail` alone (no group has a `detail` of its own), macros prefixed `WEBCPP_CRYPTO_` and include guards `WEBCPP_CRYPTO_<PATH>_HPP` (`WEBCPP_CRYPTO_DETAIL_CANARY_H` for the C header). Version 0.1's names are renamed to this layout before it closes, and no alias of the old names exists. A program MUST reach the library through the b2 target `/webcpp/crypto//crypto`, an alias that gives `libs/crypto/include`, Boost's headers, `/webcpp/hash//hash`, and on Windows `bcrypt`.

#### Scenario: A program that only draws random bytes links no backend
- **WHEN** a Jamfile reads `exe app : app.cpp /webcpp/crypto//crypto ;` and `app.cpp` includes `<webcpp/crypto/random_device.hpp>` and `<webcpp/crypto/equal.hpp>`
- **THEN** it compiles and links without any library of `/third_party` being named or built

#### Scenario: A header and its namespace
- **WHEN** a program includes `<webcpp/crypto/curves/ed25519.hpp>` and `<webcpp/crypto/ciphers/aes256_gcm.hpp>`
- **THEN** it names `webcpp::crypto::curves::ed25519::keypair` and `webcpp::crypto::ciphers::aes256_gcm`, and `webcpp::crypto::aes256_gcm` does not exist

### Requirement: Library: the public headers
The library SHALL provide version 0.1's headers in their groups (`config.hpp`, `error.hpp`, `equal.hpp`, `random_device.hpp`, `ciphers/aes.hpp`, `ciphers/gcm.hpp`, `ciphers/aes256_gcm.hpp`, `curves/secp256k1.hpp`, `backends/<name>.hpp`) and, as their increments add them, `ciphers/chacha20_poly1305.hpp`, `ciphers/aegis.hpp`, `ciphers/ccm.hpp`, `ciphers/eax.hpp`, `ciphers/ocb.hpp`, `ciphers/siv.hpp`, `ciphers/gcm_siv.hpp`, `curves/brainpool.hpp`, `curves/bn254.hpp`, `hybrid/sealed_box.hpp`, `hybrid/hpke.hpp`, `commitments/kzg.hpp`, `secret_sharing/shamir.hpp`, `supports.hpp`, `secret.hpp`, `ciphers/aead.hpp`, `curves/ed25519.hpp`, `curves/ed448.hpp`, `curves/x25519.hpp`, `curves/x448.hpp`, `curves/p256.hpp`, `curves/p384.hpp`, `curves/p521.hpp`, `rsa/keypair.hpp`, `rsa/pss.hpp`, `rsa/pkcs1.hpp`, `rsa/oaep.hpp`, `post_quantum/ml_kem.hpp`, `post_quantum/ml_dsa.hpp`, `post_quantum/slh_dsa.hpp`, `post_quantum/x_wing.hpp`, `hybrid/box.hpp`, `ciphers/secretbox.hpp`, `ciphers/secretstream.hpp`, `curves/x25519/kx.hpp`, `ciphers/ctr.hpp`, `ciphers/cbc.hpp`, `ciphers/stream.hpp`, `ciphers/keywrap.hpp`, `ciphers/block.hpp`, `ciphers/mac.hpp`, `curves/ristretto255.hpp`, `curves/edwards25519.hpp`, `curves/bls12_381.hpp`, `random.hpp`, `memory.hpp`, `padding.hpp`, `encoding.hpp`, `ciphers/ipcrypt.hpp`, `guarded_memory.hpp`, `async.hpp` and `curves/secp256k1/*.hpp`, and the convenience header `<webcpp/crypto.hpp>`. Every public header MUST include `<webcpp/crypto/config.hpp>` first, and only the headers that call a C library need a backend linked.

#### Scenario: A program that only signs names only libsecp256k1
- **WHEN** a Jamfile reads `exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1 ;` and `sign.cpp` includes `<webcpp/crypto/curves/secp256k1.hpp>`
- **THEN** it builds and prints `schnorr ok`, with no encryption backend linked

#### Scenario: A program that only signs with Ed25519 names only libsodium
- **WHEN** a Jamfile reads `exe sign : sign.cpp /webcpp/crypto//crypto /third_party//sodium ;` and `sign.cpp` includes `<webcpp/crypto/curves/ed25519.hpp>`
- **THEN** it builds and prints `ed25519 ok backend=sodium`

### Requirement: Library: the convenience header
`<webcpp/crypto.hpp>` SHALL include everything that needs no backend (`config.hpp`, `error.hpp`, `equal.hpp`, `random_device.hpp`, `supports.hpp`, `secret.hpp`, and every algorithm header whose use refuses without a backend) and the header of each backend whose `WEBCPP_CRYPTO_HAS_<NAME>` define is set, and MUST leave out `async.hpp` and `backends/browser.hpp`, saying in its comment that they bring webcpp::asio. It MUST NOT make a program that links no backend fail to compile.

#### Scenario: The convenience header with no backend linked
- **WHEN** a program that links no backend includes only `<webcpp/crypto.hpp>` and uses `equal`
- **THEN** it compiles, links and runs

### Requirement: Library: nothing of hashing
The library SHALL NOT offer a hash, an XOF, HMAC as a hash algorithm, a key derivation or a password hash: they are webcpp::hash's, which re-exports Boost.Hash2's algorithms and HMAC and owns HKDF, PBKDF2, scrypt, Argon2, Keccak-256 and BLAKE3. `/webcpp/crypto//crypto` MUST depend on `/webcpp/hash//hash`, and a signature over a prehash MUST take webcpp::hash's algorithm as a template argument. What the library adds for hash's users is `equal`, because `digest<N>::operator==` is `memcmp` and so not constant time.

#### Scenario: A MAC is checked with equal
- **WHEN** a program computes `webcpp::hash::hmac<sha2_256>` and compares its `result()` with the expected digest
- **THEN** it uses `webcpp::crypto::equal`, whose `digest<N>` overload accepts both

### Requirement: Library: every call is synchronous
Every operation of the synchronous API SHALL return before it yields, and the synchronous API MUST NOT call WebCrypto or offer a promise, a callback or an awaitable: with C code on every target nothing waits. A parallel asynchronous family, `<webcpp/crypto/async.hpp>`, exists beside it for two purposes only: the algorithms WebCrypto offers (the only interface of `backends::browser`; on another backend the synchronous call, posted) and, in a build with threads, the heavy operations, which run on another thread.

#### Scenario: Browser and native run the same call
- **WHEN** `ciphers::aes256_gcm::seal` is called on Emscripten and natively
- **THEN** it returns its `result` directly in both builds, from C code

#### Scenario: The browser only through the asynchronous family
- **WHEN** a program seals with `backends::browser`
- **THEN** it does so through `async_seal`, and the synchronous `seal` on that backend does not compile

### Requirement: Backends: the five and what each exports
The backends SHALL be `backends::bearssl`, `backends::openssl`, `backends::ctaes`, `backends::secp256k1`, `backends::botan`, `backends::sodium`, `backends::blst` and `backends::browser`, each a tag type with `static constexpr std::string_view name`, exporting what `doc/tables/crypto.json` lists for it, on the targets it lists: BearSSL, ctaes, libsecp256k1, libsodium and blst on all four targets; OpenSSL on native, wasip2 and wasip3; Botan natively, on Emscripten, and on wasip2 and wasip3 as increment 7's spike finds; the browser on Emscripten, asynchronously. The documentation MUST give the generated table and, per backend, its Jamfile line, its headers and functions and every message with its fix, and each backend's own C API MUST stay reachable (`<bearssl.h>`, `<openssl/*.h>`, `<ctaes.h>`, `<secp256k1*.h>`, `<botan/*.h>`, `<sodium.h>`, `<blst.h>`).

#### Scenario: The backend names
- **WHEN** an example prints `ciphers::aes256_gcm::backend_type::name`
- **THEN** it prints `bearssl` on WebAssembly and `openssl` natively, the defaults' stated reasons

### Requirement: Default backend: per target
`config.hpp` and the generated defaults SHALL define every family's default, `default_backend_t<Family>`, from `WEBCPP_TARGET_*` alone, as "Default backend: one per family and target, by a stated reason" gives them; `default_backend`, the genesis's name, MUST stay the default of the AES-GCM family: OpenSSL natively, MSVC included, and BearSSL on Emscripten, wasip2 and wasip3, whatever the program links. A program names another backend at any use with the template argument. The documentation MUST say that the source-built OpenSSL is compiled `no-asm` on MSVC, wasip2 and wasip3, and that BearSSL is 30 to 62 times slower than OpenSSL on ARM.

#### Scenario: BearSSL natively
- **WHEN** a native program wants AES-GCM on BearSSL
- **THEN** it names `ciphers::gcm<ciphers::aes256<backends::bearssl>>`, and its Jamfile links `/third_party//bearssl`

#### Scenario: OpenSSL not linked natively
- **WHEN** a native program links only BearSSL and uses `ciphers::aes256_gcm`
- **THEN** compilation stops naming `/webcpp/crypto//openssl` and `ciphers::gcm<ciphers::aes256<backends::bearssl>>`

### Requirement: secp256k1: ECDH and the secret
`ecdh` SHALL take the secret out of the key pair (`secp256k1_keypair_sec`) into a local array and wipe it with the library's key wipe before returning on every path, return the 32-byte result as `secret<32>`, and return `backend_failure` if `secp256k1_ecdh` returns 0 on a parsed key, which a valid key pair and libsecp256k1's own hash function never cause. `ecdh_x` and `ecdh_point` follow the same rules with their own hash functions.

#### Scenario: Agreement
- **WHEN** two key pairs a and b run `a.ecdh(B)` and `b.ecdh(A)` on random secrets
- **THEN** the two 32-byte secrets are equal, and equal to the SHA-256 of the compressed shared point from the C API on Wycheproof's valid cases

### Requirement: Errors: the category and its values
`webcpp::crypto::error` SHALL be a `boost::system` error category named `webcpp.crypto` with these fixed values, never reused: 1 `invalid_key_size`, 2 `invalid_iv_size`, 3 `invalid_tag_size`, 4 `invalid_input_size`, 5 `message_too_long`, 6 `authentication_failed`, 7 `invalid_state`, 8 `entropy_unavailable`, 9 `invalid_secret_key`, 10 `invalid_public_key`, 11 `invalid_signature`, 12 `backend_failure`, 13 `memory_functions_refused`, 14 `memory_functions_not_installed`, 15 `weak_public_key`, 16 `invalid_point`, 17 `invalid_scalar`, 18 `invalid_padding`, 19 `invalid_encoding`, 20 `invalid_ciphertext_size`. It MUST provide `error_category()` and `make_error_code(error)`, and the generic conditions MUST use `boost::system::errc::not_enough_memory` and `not_supported`, and `boost::asio::error::operation_aborted` for a cancelled asynchronous operation.

#### Scenario: The category's name and a value
- **WHEN** `make_error_code(error::authentication_failed)` is inspected
- **THEN** its category name is `webcpp.crypto` and its value is 6

#### Scenario: A value of the foundation
- **WHEN** `make_error_code(error::weak_public_key)` is inspected
- **THEN** its value is 15 and its message says the public key is weak

### Requirement: Allocation: what each backend does in make
BearSSL, ctaes, libsodium, blst and libsecp256k1 (but its context) SHALL allocate nothing anywhere: their keys and states live inside the webcpp object, and libsodium's guarded memory resource is an allocator the program chooses. OpenSSL's and Botan's `make` allocate their contexts through their own allocators outside the user's, a departure from the owner's allocators decision that the owner accepted on 2026-10-11 and the backend's page MUST state; OpenSSL's public-key operations may also allocate inside OpenSSL per call, which the page MUST state per algorithm. The counting test counts and reports these for Botan, and for OpenSSL without the memory option, instead of refusing them, and proves every other backend allocates nothing.

#### Scenario: Botan's make is counted
- **WHEN** the counting test runs on Botan
- **THEN** it reports what `make` allocates, and proves `seal` and `open` allocate nothing

#### Scenario: libsodium allocates nothing
- **WHEN** the counting test signs, verifies, agrees and seals on libsodium
- **THEN** no allocation is counted

### Requirement: Tests: WebCrypto as an outside oracle
In node and in Chrome, WebCrypto SHALL run every algorithm it offers against every synchronous backend that has it, on the vectors and on random inputs: byte for byte where the algorithm is deterministic (AES-GCM with keys of 16 and 32 bytes and trystero's 16-byte IVs, AES-CBC, AES-CTR, AES-KW, ECDH, X25519, Ed25519, RSA-OAEP decryption, RSASSA-PKCS1-v1_5), and by each side verifying the other's output where it is not (ECDSA, RSA-PSS, RSA-OAEP encryption). The lane `browser` runs on Emscripten, and an Emscripten audio worklet gives `fill` `errc::not_supported`.

#### Scenario: Random messages under 16-byte IVs
- **WHEN** random messages are sealed by `ciphers::aes256_gcm` and by node's WebCrypto under the same key and IV
- **THEN** ciphertext and tag are identical

#### Scenario: ECDSA verified both ways
- **WHEN** BearSSL signs a digest on P-256 and WebCrypto verifies it, and WebCrypto signs and BearSSL verifies
- **THEN** both verifications succeed

### Requirement: Tests: cross-backend agreement
Every backend SHALL be compared against every other that has the same algorithm, on the same inputs from a seeded generator so that a failure replays, natively and on each WebAssembly target where two backends have it: for AEADs random keys, nonces, additional data of 0 to 64 bytes and messages of 0 to 4,096 bytes and across every multiple of 16 near those sizes, with byte-identical ciphertexts and tags and each opening the other's; for signatures each verifying the other's; for agreements and KEMs equal shared secrets; for deterministic signatures equal signatures. Where a backend refuses an input another accepts, the difference MUST be listed in the test as expected, with its reason.

#### Scenario: BearSSL against OpenSSL
- **WHEN** `agree_gcm<bearssl_gcm, openssl_gcm>(seed)` runs natively
- **THEN** every ciphertext and tag is identical and each opens the other's

#### Scenario: Agreement on WebAssembly
- **WHEN** the X25519 agreement test runs on wasip2 with libsodium and BearSSL linked
- **THEN** both give the same shared secrets for the same keys

### Requirement: Tests: sizes and throughput
The own lane `sizes` SHALL build, through `webcpp.program-sizes` at `<variant>release` on every target, the baseline program and one program per (algorithm, backend) of the capability table, stripped, and its table MUST feed the documentation's size matrix; throughput MUST be measured by hand from release builds alone, by a collect script that refuses a program or data not built at `variant=release`, and published as a dated page. Neither MAY decide a default; no `runners.yml` and no ranking exist.

#### Scenario: BearSSL's code size on WebAssembly
- **WHEN** the sizes lane builds the AES-GCM program on wasip2
- **THEN** it builds it at `variant=release`, and the documentation's matrix shows the size the lane's table records

### Requirement: Increments: five, in order
The genesis's work SHALL proceed as version 0.1 (its increment 1 without its deferred parts, and its increment 2, merged), then, by the owner's decision of 2026-11-17, without trystero's migration: its increment 3 reduced to the memory option (its Tasks 15 and 16, Task 33 moved into crypto's own tests, and Task 34), which runs beside the foundation's increments 1 and 2; its increment 4 (ChaCha20-Poly1305 on BearSSL and OpenSSL) built in the foundation's increment 1 over `ciphers::aead`; and its increment 5 (Botan) built as the foundation's increment 7, Botan a submodule. Its Tasks 31 and 32 (trystero's sealer, entropy and signer) MUST NOT be built by this library's work, and its Task 37 (Botan found on the machine) MUST NOT be built. Its Tasks 20 (constant time) and 22 (sizes and throughput, re-cut to release builds that decide nothing) are built in the foundation's increment 1, Task 23 (the browser lane) in its increment 4, and Task 25b and NIST's vectors stay behind the owner's gates O2 and O3.

#### Scenario: ChaCha20-Poly1305 in version 0.2
- **WHEN** the foundation's increment 1 lands
- **THEN** `ciphers::chacha20_poly1305` exists on libsodium, BearSSL and OpenSSL, and the three agree

#### Scenario: trystero keeps its own sealer
- **WHEN** the foundation's increments and the memory option have merged
- **THEN** webcpp::trystero's code is unchanged by them, its build still naming `/third_party//secp256k1` since version 0.1

## REMOVED Requirements

### Requirement: Backend Botan: found, native only, with exceptions
**Reason**: Botan becomes a pinned submodule built by b2, available natively, on Emscripten and on WASI as its spike finds (`design.md` D15), as chapter 5 asks of every C or C++ library a backend wraps.
**Migration**: "Backend Botan: a pinned submodule built by b2, with exceptions" replaces it; the rule that Botan's header declares nothing without exceptions is kept there.

### Requirement: Build: Botan found, or the build stops
**Reason**: Botan is no longer found on the machine; `webcpp.external` is not used for it (D15).
**Migration**: a program names `/third_party//botan`, which the superproject builds from the submodule; a missing checkout stops with `git submodule update --init third_party/libs/botan`, as every third-party target does.

### Requirement: Build: Botan on WebAssembly stops the build
**Reason**: Botan builds for Emscripten, and for wasip2 and wasip3 where the spike shows it does (D15).
**Migration**: on a target the capability table marks absent for Botan, `/third_party//botan` stops with the message the table generates, naming the backends that serve that target.
