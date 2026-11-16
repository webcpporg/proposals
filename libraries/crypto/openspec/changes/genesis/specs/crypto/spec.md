## Purpose

webcpp::crypto is a header-only C++20 library that gives a program the cryptography Boost does
not provide, on native, wasip2, wasip3 and Emscripten: authenticated encryption (AES-GCM,
ChaCha20-Poly1305), the AES block cipher, a `random_device`, a constant-time `equal` and
secp256k1 (BIP340 Schnorr, ECDSA, ECDH). The algorithms are not webcpp's: each runs on a
backend, a C or C++ library (BearSSL, OpenSSL, ctaes, libsecp256k1, Botan) behind an interface
shaped as Boost.Hash2's. These requirements are the structured form of `reference/spec.md` and
of the build, tests and tools of `reference/plan.md`; the library is not built yet.

## ADDED Requirements

### Requirement: Library: header-only target and namespace
The library SHALL be header-only C++20 and hold cryptographic primitives only (a protocol's rules belong to a domain library), in namespace `webcpp::crypto` (with `webcpp::crypto::backends`, `webcpp::crypto::ciphers` and `webcpp::crypto::curves::secp256k1`), with public headers under `<webcpp/crypto/...>` that mirror the namespaces as `<webcpp/crypto/<group>/<name>.hpp>`, one per responsibility, internals in `webcpp::crypto::detail`, macros prefixed `WEBCPP_CRYPTO_` and include guards `WEBCPP_CRYPTO_<PATH>_HPP` (`WEBCPP_CRYPTO_DETAIL_CANARY_H` for the C header). A program MUST reach it through the b2 target `/webcpp/crypto//crypto`, an alias that gives `libs/crypto/include`, Boost's headers, and on Windows `bcrypt`.

#### Scenario: A program that only draws random bytes links no backend
- **WHEN** a Jamfile reads `exe app : app.cpp /webcpp/crypto//crypto ;` and `app.cpp` includes `<webcpp/crypto/random_device.hpp>` and `<webcpp/crypto/equal.hpp>`
- **THEN** it compiles and links without any library of `/third_party` being named or built

### Requirement: Library: the public headers
The library SHALL provide `<webcpp/crypto/config.hpp>`, `error.hpp`, `equal.hpp`, `random_device.hpp`, `ciphers/aes.hpp` (`ciphers::aes128`, `aes192`, `aes256`), `ciphers/gcm.hpp` (`ciphers::gcm<Cipher>`), `ciphers/chacha20_poly1305.hpp`, `ciphers/aes256_gcm.hpp` (`ciphers::aes128_gcm`, `aes256_gcm` and the default backend's header), `backends/<name>.hpp` (a backend's tag type, compile-time check and canary), `curves/secp256k1.hpp` (`curves::secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`) and the convenience header `<webcpp/crypto.hpp>`. Every public header MUST include `<webcpp/crypto/config.hpp>` first, and only the headers that call a C library need a backend linked.

#### Scenario: A program that only signs names only libsecp256k1
- **WHEN** a Jamfile reads `exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1 ;` and `sign.cpp` includes `<webcpp/crypto/curves/secp256k1.hpp>`
- **THEN** it builds and prints `schnorr ok`, with no encryption backend linked

### Requirement: Library: the convenience header
`<webcpp/crypto.hpp>` SHALL include everything that needs no backend (`config.hpp`, `error.hpp`, `equal.hpp`, `random_device.hpp`, `ciphers/aes.hpp`, `ciphers/gcm.hpp`) and the header of each backend whose `WEBCPP_CRYPTO_HAS_<NAME>` define is set. It MUST NOT make a program that links no backend fail to compile.

#### Scenario: The convenience header with no backend linked
- **WHEN** a program that links no backend includes only `<webcpp/crypto.hpp>` and uses `equal`
- **THEN** it compiles, links and runs

### Requirement: Library: nothing of hashing
The library SHALL NOT offer SHA, HMAC or any hash: hashing and HMAC are Boost.Hash2's (`boost::hash2::hmac<H>`), and HKDF, when a library needs it, is written on Hash2's `hmac`. What the library adds for Hash2's users is `equal`, because `digest<N>::operator==` is `memcmp` and so not constant time.

#### Scenario: A MAC is checked with equal
- **WHEN** a program computes `boost::hash2::hmac<sha2_256>` and compares its `result()` with the expected digest
- **THEN** it uses `webcpp::crypto::equal`, whose `digest<N>` overload accepts both

### Requirement: Library: every call is synchronous
Every operation SHALL return before it yields; the library MUST NOT offer a promise, a callback or an awaitable, because with C code on every target nothing waits (the first design was asynchronous for WebCrypto, which the owner removed).

#### Scenario: Browser and native run the same call
- **WHEN** `aes256_gcm::seal` is called on Emscripten and natively
- **THEN** it returns its `result` directly in both builds

### Requirement: Library: Windows API through one header
`random_device.hpp` and `backends/openssl.hpp` (under the memory option) SHALL include `<windows.h>` only through `<webcpp/crypto/detail/windows.hpp>`, which defines `NOMINMAX` and `WIN32_LEAN_AND_MEAN` only when they are not already defined, includes `<windows.h>` (then `<bcrypt.h>` where needed), undefines again only the macros it defined itself and carries `lint-world:` with its reason. Every header MUST write `(max)()`, `(min)()` and `(std::numeric_limits<T>::max)()` so that it compiles whatever a user's `<windows.h>` defined first.

#### Scenario: Every public header after a bare windows.h on MSVC
- **WHEN** an MSVC test includes `<windows.h>` without `NOMINMAX` or `WIN32_LEAN_AND_MEAN`, then every public header, and uses `std::max` and `random_device::max` as functions afterwards (with the memory option on)
- **THEN** it compiles and runs, and so does the same test with `<windows.h>` included after the headers

### Requirement: Interface: results are values
Every operation that can fail SHALL return `boost::system::result<T>` or `result<void>`, whose `operator bool` is true on success, with exceptions on and off, and no function of the library SHALL throw except `random_device::operator()` (which exists only with exceptions) and the resource a caller gives `keypair::make`. Data and keys MUST be `std::span<std::uint8_t const>` and `std::span<std::uint8_t>`; sizes are `std::size_t`, and a limit wider than `size_t` on wasm32 is `std::uint64_t`, compared through `detail::exceeds(std::uint64_t, std::uint64_t)` so that no comparison is tautological there.

#### Scenario: A wrong key size is a value
- **WHEN** `aes256_gcm::make` is called with a 31-byte key
- **THEN** the `result` holds `error::invalid_key_size` and nothing is thrown, with exceptions on or off

### Requirement: Interface: shaped as Boost.Hash2's
Every algorithm type SHALL state its sizes as `static constexpr` members (`key_size`, `block_size`, `iv_size`, `tag_size`) and name its output `result_type`; modes MUST be templates over a cipher (`gcm<aes256<>>`), tags MUST be `boost::hash2::digest<16>`, and the backend MUST be a template parameter whose default is `default_backend`, so that `aes256<>` is `aes256<default_backend>` and `aes256<backends::openssl>` names one.

#### Scenario: GCM sizes
- **WHEN** a program reads `gcm<aes256<>>::key_size`, `tag_size`, `iv_size` and `max_iv_size`
- **THEN** they are 32, 16, 12 and 128

### Requirement: Interface: keyed objects are made, move-only and wiped
A keyed object SHALL be built by `static make(...)` returning `result<T>`, MUST be move-only (copy deleted), and its destructor MUST wipe the key schedule. A move SHALL wipe the source's key and state and leave it empty: a null context for OpenSSL and Botan, a cleared flag for BearSSL, ctaes and libsecp256k1's key pair. Every function of an empty object but assignment and destruction MUST return `error::invalid_state` before reading a secret byte, and moving from an empty object, assigning from one, or swapping an empty with a live object MUST give an empty destination.

#### Scenario: A moved-from BearSSL gcm does not seal under a zero key
- **WHEN** `auto b = std::move(a);` and then `a.seal(iv, {}, data)` on `gcm<aes256<backends::bearssl>>`
- **THEN** the result holds `error::invalid_state` and `data` is untouched

#### Scenario: Moving an empty object
- **WHEN** `aead c = std::move(b);` with `b` already moved from
- **THEN** `c` is empty and every function of `b` and `c` returns `invalid_state`

### Requirement: Block cipher: aes128, aes192 and aes256
`aes128<Backend>`, `aes192<Backend>` and `aes256<Backend>` SHALL have `key_size` 16, 24 and 32, `block_size` 16, `backend_type`, `static make(key)`, and `encrypt(blocks)` and `decrypt(blocks)` that work in place on whole blocks and return `result<void>`. Each MUST be a complete type for every backend, so that it can name GCM's cipher (`gcm<aes256<backends::bearssl>>`), while `make`, `encrypt` and `decrypt` are defined only where the backend exports the block cipher: ctaes, OpenSSL and Botan.

#### Scenario: FIPS-197 appendix C.3 on ctaes
- **WHEN** `aes256<backends::ctaes>::make` is given the key `00 01 02 ... 1f` and `encrypt` the block `00 11 22 ... ff`
- **THEN** the block becomes `8e a2 b7 ca 51 67 45 bf ea fc 49 90 4b 49 60 89`, `decrypt` restores it, and the program prints `ctaes aes256 ok`

### Requirement: Block cipher: guards
`make` SHALL give `invalid_key_size` for a key of another size than `key_size` and then, on OpenSSL with the memory option, `memory_functions_not_installed`; `encrypt` and `decrypt` MUST give `invalid_state` on an object moved from, then `invalid_input_size` when `blocks.size()` is not a multiple of 16, and a zero-block call is a success.

#### Scenario: A partial block
- **WHEN** `encrypt` is called on a 17-byte span
- **THEN** it returns `error::invalid_input_size` and the span is untouched

### Requirement: Block cipher: BearSSL has none, and aes256 has no default of its own
`aes256<backends::bearssl>::make` SHALL fail to compile with the `static_assert` text "webcpp::crypto: ciphers::aes256<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in ciphers::aes256<backends::ctaes> or ciphers::aes256<backends::openssl>, and add its library to the program's Jamfile." (Botan joins the list with increment 5) The block cipher MUST get no default backend of its own, because `aes256<>` must stay the cipher of `aes256_gcm`; on WebAssembly a program names `aes256<backends::ctaes>` and includes `<webcpp/crypto/backends/ctaes.hpp>`.

#### Scenario: make on BearSSL
- **WHEN** a program calls `aes256<backends::bearssl>::make(key)`
- **THEN** compilation fails with exactly that text, from a test in a directory of its own

### Requirement: GCM: the interface
`gcm<Cipher>` SHALL declare `cipher_type`, `backend_type`, `result_type` (`boost::hash2::digest<16>`), `key_size` (the cipher's), `tag_size` 16, `iv_size` 12, `max_iv_size` 128 and `max_message_size` `(1 << 36) - 32`, `static make(key)`, a move constructor and assignment that abandon a message in progress, `seal(iv, additional_data, data)` returning `result<digest<16>>`, `open(iv, additional_data, data, tag)` returning `result<void>`, and the incremental `begin_seal(iv, additional_data)`, `update(data)` and `result()`. `aes128_gcm` and `aes256_gcm` MUST be `gcm<aes128<>>` and `gcm<aes256<>>`.

#### Scenario: NIST test case 14 (zero key, IV and block)
- **WHEN** `aes256_gcm::make` is given 32 zero bytes and `seal` a 12-byte zero IV, no additional data and a zeroed 16-byte `data`
- **THEN** `data` becomes `ce a7 40 3d 4d 60 6b 6e 07 4e c5 d3 ba f3 9d 18` and the tag is `d0 d1 c8 a7 99 99 6b f0 26 5b 98 b5 d4 8a b9 19`; `open` with that tag restores 16 zero bytes

### Requirement: GCM: in place, the tag apart
`seal` and `open` SHALL work in place: `data` holds the plaintext before `seal` and the ciphertext after it, and the ciphertext before `open` and the plaintext after it. The tag MUST be separate, a `digest<16>` out of `seal` and a dynamic span into `open`, so that a `digest<16>` and a `std::array<std::uint8_t, 16>` both pass without a cast. `gcm` and `chacha20_poly1305` MUST copy the tag into a local array and consume the IV and the additional data before the first byte of `data` is written, so that a tag that is a subspan right after `data` or overlapping its end, and an IV or additional data in the same buffer, give correct results.

#### Scenario: A tag appended to the ciphertext
- **WHEN** `open` is given `tag` as the 16 bytes that immediately follow `data` in one buffer
- **THEN** it verifies and decrypts correctly

#### Scenario: An overlapping tag fails cleanly
- **WHEN** `open` is given a tag that overlaps the end of `data` and does not match
- **THEN** it returns `authentication_failed`, `data` is zeroed and nothing else is corrupted

### Requirement: GCM: the guards and their order
Each function SHALL check every condition first, in this order, and return before it reads a secret byte: `make` a key of another size (`invalid_key_size`) then the memory option's `memory_functions_not_installed`; any other function an object moved from (`invalid_state`); `begin_seal` while a message is in progress, and `update` or `result` with none (`invalid_state`); an empty IV or one longer than 128 bytes (`invalid_iv_size`); `open` a tag of another size than 16 (`invalid_tag_size`); a message longer than 2^36 - 32 bytes, in one call or summed over `update`s, or additional data longer than 2^61 - 1 bytes (`message_too_long`); then `open` a tag that does not match (`authentication_failed`), a backend failing where it should not (`backend_failure`), and in `make` a context that cannot be allocated (`errc::not_enough_memory`). A guard's refusal MUST leave `data` untouched.

#### Scenario: A 129-byte IV
- **WHEN** `seal` is called with an IV of 129 bytes
- **THEN** it returns `error::invalid_iv_size` and `data` is untouched

#### Scenario: A tag of 15 bytes
- **WHEN** `open` is given a 15-byte tag
- **THEN** it returns `error::invalid_tag_size` before reading the key

### Requirement: GCM: a failed open zeroes data
After any failure that follows the start of decryption (`authentication_failed`, `backend_failure`) the backend code of webcpp SHALL zero the whole of `data` itself, on every backend, through `volatile` stores, so that no unauthenticated plaintext is left; an in-place `open` that fails does not keep its ciphertext, and a caller who needs it keeps a copy. The documentation MUST say that decrypted bytes sit in `data` before the tag is checked, so that a caller whose `data` another party can read opens into private memory.

#### Scenario: Every byte changed in turn
- **WHEN** each byte of the ciphertext, the tag and the additional data is changed in turn and `open` is called
- **THEN** every call returns `authentication_failed` and `data` is all zero

### Requirement: GCM: IVs of 1 to 128 bytes
`gcm` SHALL accept IVs of 1 to 128 bytes on every backend, the length at which OpenSSL's GCM provider keeps its IV in a fixed buffer, so that a call never succeeds on one backend and fails on another; the documentation MUST recommend 12 bytes. An IV of another length than 12 is hashed into the first counter block.

#### Scenario: Wycheproof's 257-byte IV
- **WHEN** the three valid Wycheproof cases with a 257-byte IV (tcId 268, 272, 276) are sealed
- **THEN** each returns `error::invalid_iv_size` with `data` untouched, and the test counts them as expected refusals

#### Scenario: trystero's 16-byte IV
- **WHEN** `seal` is given a random 16-byte IV
- **THEN** it succeeds and equals WebCrypto's result for the same key, IV and data

### Requirement: GCM: keys of 16, 24 and 32 bytes
`aes128_gcm`, `gcm<aes192<>>` and `aes256_gcm` SHALL accept keys of 16, 24 and 32 bytes; the documentation MUST say that Chrome's WebCrypto refuses 24-byte keys, so a key meant for a browser's WebCrypto is of 16 or 32 bytes.

#### Scenario: All 18 cases of the GCM specification
- **WHEN** the 18 test cases of the GCM specification (keys of 16, 24 and 32 bytes; IVs of 8, 12 and 60 bytes) are sealed and opened on each backend
- **THEN** every one matches

### Requirement: GCM: the incremental seal
`begin_seal(iv, additional_data)` SHALL begin a message, `update(data)` encrypt the next piece in place and `result()` end it with its tag; after `result` no message is in progress, so a second `result` gives `invalid_state`. Any error from `begin_seal`, `update` or `result` MUST end the message: its state is wiped, the pieces already encrypted have no tag, and the object is ready for a new `begin_seal`. A move abandons a message in progress. A one-shot `seal` or `open` while a message is in progress MUST give `invalid_state` and leave the message in progress, its `result()` still correct, and `data` untouched.

#### Scenario: Data in pieces of every size
- **WHEN** a message is sealed incrementally in pieces of 1 byte up to the whole, and the result opened one shot
- **THEN** the ciphertext and tag equal the one-shot `seal` and `open` succeeds

#### Scenario: An error in the middle ends the message
- **WHEN** an `update` returns `message_too_long` and a fresh `begin_seal` follows
- **THEN** the message is ended, `result()` before the fresh begin gives `invalid_state`, and the new message works

### Requirement: GCM: no incremental open
`gcm` SHALL NOT offer an incremental decryption, because it would return plaintext before the tag is known; a later increment may add one only with the plaintext withheld until the tag verifies. The incremental seal takes the IV and the additional data at its start, because Botan's public interface takes the associated data whole, before the nonce.

#### Scenario: What the incremental seal produced
- **WHEN** a message sealed with `begin_seal`, `update` and `result` is received
- **THEN** it is opened by the one-shot `open`

### Requirement: GCM: limits and thread use
`gcm` SHALL reject a message longer than 2^36 - 32 bytes with `message_too_long` (reachable natively, where `size_t` has 64 bits), and its functions MUST be non-`const` and used by one thread at a time. The documentation MUST list `gcm` as not shareable (it holds a key, and OpenSSL's and Botan's hold pointers into their libraries' heaps), state NIST SP 800-38D's limit of 2^32 messages per key with random IVs or IVs other than 96 bits, and state that an IV repeated under one key exposes the XOR of the plaintexts and lets an attacker forge tags.

#### Scenario: A message over the limit
- **WHEN** natively `seal` is given a span of `max_message_size + 1` bytes of reserved, untouched address space (`webcpp::test::reserved`)
- **THEN** it returns `error::message_too_long` and `data` is untouched

### Requirement: ChaCha20-Poly1305: the interface
`chacha20_poly1305<Backend>` SHALL have `key_size` 32, `iv_size` 12 (exactly, RFC 8439 section 2.8), `tag_size` 16 and `max_message_size` `(1 << 38) - 64`, `static make(key)`, and one-shot `seal(iv, additional_data, data)` and `open(iv, additional_data, data, tag)` as `gcm` has them, move-only and wiping. It MUST NOT offer an incremental form, because BearSSL's ChaCha20-Poly1305 is one function over the whole message.

#### Scenario: RFC 8439 section 2.8.2
- **WHEN** the 114-byte plaintext of section 2.8.2 is sealed
- **THEN** the ciphertext begins `d3 1a 8d 34 64 8e 60 db` and the tag is `1a e1 0b 59 4f 09 e2 6a 7e 90 2e cb d0 60 06 91`

### Requirement: ChaCha20-Poly1305: guards, nonces and backends
The guards and errors of `chacha20_poly1305` SHALL be GCM's, the moved-from object and the zeroing of `data` included, with an IV of exactly 12 bytes (`invalid_iv_size` otherwise) and RFC 8439's limit of 2^38 - 64 bytes; where a backend leaves the tag check to its caller (BearSSL), the backend MUST compare with `equal` and zero `data` on a mismatch. Its backends are BearSSL, OpenSSL (`EVP_chacha20_poly1305`) and Botan; `chacha20_poly1305<backends::ctaes>` MUST fail to compile with a message of the kind of GCM's. The documentation MUST state that a nonce repeated under one key reveals the XOR of the plaintexts and allows forgeries, and give GCM's limit of 2^32 messages per key for random 96-bit nonces.

#### Scenario: Wycheproof's ChaCha20-Poly1305 file
- **WHEN** all 325 tests of `chacha20_poly1305_test.json` at the pinned commit run
- **THEN** the 316 with a 12-byte nonce pass through `seal` and `open` (256 valid, 60 invalid failing with `authentication_failed`), and the 9 with a nonce of 0, 8, 11, 13, 14, 16, 20, 24 or 32 bytes fail with `invalid_iv_size`

### Requirement: Equal: constant time on tags
`equal(digest<N> const&, digest<N> const&)` SHALL be `noexcept`, read every byte, and neither branch nor index on one: it accumulates the XOR of every pair of bytes with OR in a `volatile` local read once after the loop, and reduces the result to one bit without a branch, as BearSSL's tag check does. It MUST serve Boost.Hash2's `hmac<H>::result()` as much as the library's tags.

#### Scenario: Two 32-byte digests differing in the last byte
- **WHEN** `equal` compares them
- **THEN** it returns false after reading all 32 bytes, and the deterministic check passes at -O2 and -Os

### Requirement: Equal: spans of different sizes
`equal(std::span<std::uint8_t const>, std::span<std::uint8_t const>)` SHALL compare spans of different sizes as unequal without reading their contents, because a size is public, and MUST take a `digest<N>` against a `std::array` (a digest is a contiguous range of `unsigned char`); two empty spans are equal.

#### Scenario: Empty spans
- **WHEN** `equal` compares two empty spans
- **THEN** it returns true

### Requirement: Random device: the interface
`random_device` SHALL be shaped as `std::random_device`: `result_type` is `unsigned int`, `(min)()` is 0, `(max)()` is the type's maximum (written in parentheses), `entropy()` is `std::numeric_limits<result_type>::digits`, and `fill(std::span<std::uint8_t>)` returns `result<void>`. It MUST be default constructible with no state, non-copyable, and need no backend.

#### Scenario: Buffers of every edge size
- **WHEN** `fill` is given buffers of 0, 1, 256, 257, 65,536 and 65,537 bytes
- **THEN** every call succeeds, and two calls give different bytes

### Requirement: Random device: errors and the throwing call
When the source fails, `fill` SHALL return `error::entropy_unavailable` and zero `out`; where the environment has no generator (an Emscripten audio worklet) it MUST return `errc::not_supported`. `operator()` SHALL exist only without `WEBCPP_CRYPTO_NO_EXCEPTIONS`, call `fill` for `sizeof(result_type)` bytes and on error `boost::throw_exception` a `boost::system::system_error` carrying fill's error, so that it is a UniformRandomBitGenerator with exceptions on.

#### Scenario: A planted failing source
- **WHEN** a test build with `WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY` makes the source fail
- **THEN** `fill` returns `entropy_unavailable` and the buffer is zero, and `operator()` throws with exceptions on

### Requirement: Entropy: one safe source per platform
The entropy code SHALL read one source per platform and nothing else: `getentropy` on Linux, macOS and WASI, `crypto.getRandomValues` through Emscripten's `getentropy`, and `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` on Windows. It MUST keep no cache and no generator of its own or a backend's, MUST never fall back to a weaker source (no clock, `rand`, `rdrand` alone or address), MUST ask in pieces of at most 256 bytes (the limit of `getentropy`) and check the result of every call, and the whole function MUST fit on one screen with each branch commented with the contract it relies on and every include and call marked `lint-world:`.

#### Scenario: The lint forbids weaker sources
- **WHEN** a header whose path ends `entropy.hpp` or `random_device.hpp` names `rand`, `time`, `clock` or `__rdrand`, even with `lint-world:`
- **THEN** the superproject's lint fails

### Requirement: Entropy: the backends use the same source
BearSSL SHALL be compiled with `BR_USE_GETENTROPY=1` (and `_DEFAULT_SOURCE`, and on macOS `-include sys/random.h`), `BR_RDRAND=0` and `BR_USE_URANDOM=0`; on Windows every BearSSL seeder MUST be compiled out (`BR_USE_GETENTROPY=0`, `BR_USE_WIN32_RAND=0`), so that no backend chooses its entropy and a program that uses BearSSL's PRNG seeds it from `random_device`. libsecp256k1's context MUST be randomized from `random_device`, and the library MUST never call OpenSSL's `RAND_bytes` or a Botan RNG.

#### Scenario: BearSSL's seeder
- **WHEN** a test asks `br_prng_seeder_system` for its name
- **THEN** it names `getentropy` on every target but Windows, and the seeder is absent on Windows

### Requirement: secp256k1: the key pair
`curves::secp256k1::basic_keypair<Backend = backends::secp256k1>` (aliased `keypair`) SHALL be built by `static make(secret, memory = std::pmr::get_default_resource())`, move-only with a wiping destructor that gives the context back, and its `make` MUST create a libsecp256k1 context in the given resource with `secp256k1_context_preallocated_create`, aligned to `alignof(std::max_align_t)`, and randomize it with 32 bytes of `random_device`. `make` is not `noexcept`: the resource reports a failed allocation as it does, by throwing `std::bad_alloc` with exceptions on.

#### Scenario: make allocates only in the given resource
- **WHEN** a counting `std::pmr::memory_resource` is passed to `keypair::make` with a valid 32-byte secret
- **THEN** every allocation of the call is seen by that resource and none by the global `operator new`

### Requirement: secp256k1: public keys and signatures
A key pair SHALL provide `xonly_public_key()` (32 bytes, BIP340) and `public_key()` (33 bytes, SEC1 compressed) as `result<std::array<...>>`, `sign_schnorr(message, aux)` (BIP340, any message length, `aux` being 32 bytes of the caller's fresh randomness, 64-byte signature), `sign_ecdsa(hash)` (over a 32-byte hash, RFC 6979 nonce, low-S, compact 64-byte form) and `ecdh(public_key)` (the SHA-256 of the compressed shared point, libsecp256k1's default). All of them MUST be `const noexcept` and return `invalid_state` on a moved-from key pair.

#### Scenario: Sign and verify a BIP340 message
- **WHEN** the secret is 32 bytes of `0x42`, the message 32 bytes of `0x17` and `aux` 32 zero bytes, and `sign_schnorr` then `verify_schnorr` run
- **THEN** the signature verifies, and changing the first message byte makes `verify_schnorr` fail

### Requirement: secp256k1: verification without a context
`verify_schnorr<Backend>(signature, message, xonly_public_key)` and `verify_ecdsa<Backend>(signature, hash, public_key)` SHALL use `secp256k1_context_static`, allocate nothing and be `noexcept`, and each MUST call `secp256k1_selftest()` first, keeping no flag, because libsecp256k1 recommends the static context only after the self-test and the library may hold no static mutable state. A failed self-test is a library built wrongly, and libsecp256k1's default error callback aborts.

#### Scenario: Verification allocates nothing
- **WHEN** the counting tests run `verify_schnorr` and `verify_ecdsa` under a counting `operator new`
- **THEN** the counts are zero

### Requirement: secp256k1: ECDH and the secret
`ecdh` SHALL take the secret out of the key pair (`secp256k1_keypair_sec`) into a local array and wipe it with the library's key wipe before returning on every path, and it MUST return `backend_failure` if `secp256k1_ecdh` returns 0 on a parsed key, which a valid key pair and libsecp256k1's own hash function never cause.

#### Scenario: Agreement
- **WHEN** two key pairs a and b run `a.ecdh(B)` and `b.ecdh(A)` on random secrets
- **THEN** the two 32-byte secrets are equal, and equal to the SHA-256 of the compressed shared point from the C API on Wycheproof's valid cases

### Requirement: secp256k1: its errors
`make` SHALL give `invalid_key_size` (not 32 bytes), `invalid_secret_key` (zero, or not below the group order) and `entropy_unavailable` (the randomization). A public key MUST map to `invalid_public_key` in every way it can be wrong: in `ecdh` and `verify_ecdsa`, a size other than 33 or 65 bytes or bytes that `secp256k1_ec_pubkey_parse` refuses (a prefix other than 02, 03 or 04, a coordinate not below the field's prime, a point not on the curve, an x with no point); in `verify_schnorr`, an x-only key that `secp256k1_xonly_pubkey_parse` refuses. A failed verification MUST give `invalid_signature`, as does `verify_ecdsa` for a compact signature that `secp256k1_ecdsa_signature_parse_compact` refuses and for one in high-S form.

#### Scenario: A zero secret
- **WHEN** `keypair::make` is given 32 zero bytes
- **THEN** it returns `error::invalid_secret_key`

#### Scenario: A 34-byte public key
- **WHEN** `ecdh` is given 34 bytes
- **THEN** it returns `error::invalid_public_key`

### Requirement: secp256k1: libsecp256k1 is the only backend and its C API stays reachable
libsecp256k1 SHALL be the only secp256k1 backend (OpenSSL and Botan have ECDSA on secp256k1 but no BIP340), and the whole C API of v0.8.0, 82 functions with every module built (ecdh, recovery, extrakeys, schnorrsig, musig, ellswift, silentpayments), MUST stay reachable through `<secp256k1*.h>`.

#### Scenario: A C module not wrapped
- **WHEN** a program includes `<secp256k1_recovery.h>` and calls a function of it
- **THEN** it links against webcpp's build of libsecp256k1

### Requirement: Errors: the category and its values
`webcpp::crypto::error` SHALL be a `boost::system` error category named `webcpp.crypto` with these fixed values, never reused: 1 `invalid_key_size`, 2 `invalid_iv_size`, 3 `invalid_tag_size`, 4 `invalid_input_size`, 5 `message_too_long`, 6 `authentication_failed`, 7 `invalid_state`, 8 `entropy_unavailable`, 9 `invalid_secret_key`, 10 `invalid_public_key`, 11 `invalid_signature`, 12 `backend_failure`, 13 `memory_functions_refused`, 14 `memory_functions_not_installed`. It MUST provide `error_category()` and `make_error_code(error)`, and the two generic conditions MUST use `boost::system::errc::not_enough_memory` and `not_supported`.

#### Scenario: The category's name and a value
- **WHEN** `make_error_code(error::authentication_failed)` is inspected
- **THEN** its category name is `webcpp.crypto` and its value is 6

### Requirement: Errors: the memory option's messages
`memory_functions_refused` SHALL carry the message "OpenSSL's memory functions cannot be installed: OpenSSL has already allocated, or another party installed its own. Call backends::openssl::install_memory_functions() as the first statement of main, before anything uses OpenSSL." and `memory_functions_not_installed` the message "OpenSSL's memory functions are not webcpp::crypto's. Call backends::openssl::install_memory_functions() as the first statement of main, and let nothing else call CRYPTO_set_mem_functions."

#### Scenario: A run test per error value
- **WHEN** the error category is asked for the message of value 13 and of value 14
- **THEN** each equals the text above

### Requirement: Exceptions: the configuration macro
`config.hpp` SHALL define `WEBCPP_CRYPTO_NO_EXCEPTIONS` when the build has no exceptions (`BOOST_NO_EXCEPTIONS`), keep it when a developer defines it, and have a Doc Comment that says what depends on it: `random_device::operator()`, the Botan backend (which declares nothing under it) and the `try` of OpenSSL's memory adapter. An API that throws MUST sit behind `#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS` and raise through `boost::throw_exception`, never a bare `throw`.

#### Scenario: The configuration test in its builds
- **WHEN** `test/config_test.cpp` is built as the build asks and with `<exception-handling>off`
- **THEN** the macro is defined exactly in the second, and the target is classified

### Requirement: Allocation: webcpp's code allocates only a key pair's context
webcpp::crypto's own code SHALL allocate only `keypair::make`'s context, in the resource the user gives; every buffer is the caller's. No `seal`, `open`, `begin_seal`, `update`, `result`, `encrypt`, `decrypt`, `fill`, signature, `ecdh` or verification MAY allocate, on any backend: a backend that needs memory takes it in `make`. A counting `std::pmr::memory_resource`, a counting global `operator new` (every form) and, for OpenSSL, counting functions given to `CRYPTO_set_mem_functions` in the test program alone MUST prove it.

#### Scenario: OpenSSL's measured counts
- **WHEN** the first `make` of a process runs on OpenSSL 3.6.4, then three seals, a good open and three failed opens
- **THEN** the first `make` makes 6,061 allocations (OpenSSL's own initialisation included), a second `make` 2, and the seals and opens 0

### Requirement: Allocation: what each backend does in make
BearSSL, ctaes and libsecp256k1 (but its context) SHALL allocate nothing anywhere: their key schedules and GCM contexts live inside the webcpp object. OpenSSL's and Botan's `make` allocate their contexts through their own allocators outside the user's, a departure from the owner's allocators decision that the owner accepted on 2026-10-11 and the backend's page MUST state; the counting test counts and reports it for Botan, and for OpenSSL without the memory option, instead of refusing it.

#### Scenario: Botan's make is counted
- **WHEN** the counting test runs on Botan
- **THEN** it reports what `make` allocates, and proves `seal` and `open` allocate nothing

### Requirement: Keys: wiped by volatile stores
The key schedules that live inside webcpp's objects (BearSSL's, ctaes's, libsecp256k1's key pair, and the local array of `ecdh`) SHALL be overwritten on destruction and on move by stores through a `volatile` pointer followed by `std::atomic_signal_fence(std::memory_order_seq_cst)`, which the optimiser cannot drop as dead stores, where a plain `memset` can be dropped. OpenSSL's `EVP_CIPHER_CTX_free` and Botan's `clear()` and `secure_vector` MUST be relied on to wipe their own.

#### Scenario: A destroyed key schedule
- **WHEN** the wipe test destroys a BearSSL `gcm` and inspects the storage it occupied at -O2
- **THEN** the key schedule bytes are zero

### Requirement: Backends: the five and what each exports
The backends SHALL be `backends::bearssl`, `backends::openssl`, `backends::ctaes`, `backends::secp256k1` and `backends::botan`, each a tag type with `static constexpr std::string_view name` (`"bearssl"`, `"openssl"`, ...), exporting: BearSSL `gcm<aes128/192/256<bearssl>>` and `chacha20_poly1305<bearssl>` (no block cipher; native, wasip2, wasip3, emscripten); OpenSSL `aes128/192/256`, `gcm` and `chacha20_poly1305` (native, wasip2, wasip3; built from source by the superproject); ctaes `aes128/192/256<ctaes>` as the block cipher alone (all targets); libsecp256k1 `curves::secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`, the DER encodings, low-S and `tweak_xonly` (all targets); Botan `aes128/192/256`, `gcm` and `chacha20_poly1305` (native only, with exceptions). The documentation MUST give this table and, per backend, its Jamfile line, its headers and functions and every message with its fix, and each backend's own C API MUST stay reachable (`<bearssl.h>`, `<openssl/*.h>`, `<ctaes.h>`, `<secp256k1*.h>`, `<botan/*.h>`).

#### Scenario: The backend names
- **WHEN** an example prints `aes256_gcm::backend_type::name`
- **THEN** it prints `bearssl` on WebAssembly and `openssl` natively when OpenSSL is linked

### Requirement: Backend BearSSL: the original C from the mirror
BearSSL SHALL be used as its author wrote it, from webcpp's unmodified mirror `github.com/webcpporg/bearssl`, created from upstream master `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (tags v0.4 to v0.6), as the submodule `third_party/libs/bearssl`; its notice MUST ship as `LICENSE-BEARSSL.txt` and the README MUST name BearSSL (MIT). No BearSSL file is copied into webcpp, and webcpp's canary lives in the library's `src/`, never in the mirror.

#### Scenario: The pin
- **WHEN** `git -C third_party/libs/bearssl rev-parse HEAD` runs
- **THEN** it prints `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f`, and the canary's commit is `7bea48e`

### Requirement: Backend BearSSL: which code runs
For GCM the BearSSL backend SHALL pick the CTR class `br_aes_x86ni_ctr_get_vtable()`, then `br_aes_pwr8_ctr_get_vtable()`, then `br_aes_ct64_ctr_vtable`, and for GHASH `br_ghash_pclmul_get()`, then `br_ghash_pwr8_get()`, then `br_ghash_ctmul64`; for ChaCha20-Poly1305 `br_chacha20_sse2_get()` else `br_chacha20_ct_run`, and `br_poly1305_ctmulq_get()` else `br_poly1305_ctmul_run`. It MUST name the 64-bit constant-time code itself rather than take BearSSL's default, because on wasm32 `BR_64` is not set and the default is the 32-bit code, half as fast (159 against 79 MiB/s).

#### Scenario: Both code paths compared on x86-64
- **WHEN** the agreement test runs once with AES-NI and PCLMUL and once with `WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE` forcing `aes_ct64` and `ghash_ctmul64`
- **THEN** both match OpenSSL's output byte for byte

### Requirement: Backend BearSSL: moving a gcm
BearSSL's GCM context points at the key schedule (`br_gcm_context::bctx`), so a move of `gcm<aes256<backends::bearssl>>` SHALL copy the key schedule and call `br_gcm_init` again on the destination through the public API, abandon a message in progress, and wipe the source. BearSSL's software AES and GHASH are bitsliced and constant time; the documentation MUST say that `bmul64` assumes a constant-time 64-bit multiplication, which a WebAssembly engine on a 32-bit host may not give.

#### Scenario: A message in progress is moved
- **WHEN** a `gcm` with a message in progress is moved
- **THEN** the destination has no message in progress and the source is empty

### Requirement: Backend OpenSSL: the superproject's source-built OpenSSL, through crypto's alias
The OpenSSL backend SHALL use the superproject's source-built OpenSSL (`/webcpp/third_party//openssl`, OpenSSL 3.5.9, static), never a copy of the machine's, through crypto's alias `/webcpp/crypto//openssl`, which on native, wasip2 and wasip3 links it and defines `WEBCPP_CRYPTO_HAS_OPENSSL=1`, and on Emscripten MUST stop the compile with "webcpp::crypto: /webcpp/crypto//openssl is named in a build for Emscripten, where webcpp does not build OpenSSL. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl ...". `backends/openssl.hpp` MUST check `OPENSSL_VERSION_NUMBER >= 0x30000000L` and `#error` otherwise. OpenSSL has no link-time canary, since the provider injects nothing into upstream.

#### Scenario: OpenSSL 1.1
- **WHEN** `backends/openssl.hpp` is included with `OPENSSL_VERSION_NUMBER` below 3.0
- **THEN** compilation stops with an `#error` whose text a test fixes

#### Scenario: Emscripten names crypto's OpenSSL
- **WHEN** an Emscripten program names `/webcpp/crypto//openssl`
- **THEN** its compile stops with the alias's message, which names `/third_party//bearssl`

### Requirement: Backend OpenSSL: the calls and the key
`make` SHALL call `EVP_CIPHER_fetch` and `EVP_CIPHER_CTX_new`; each message MUST re-initialise the same context (`EVP_EncryptInit_ex` with the IV length set by `EVP_CTRL_GCM_SET_IVLEN`), call `EVP_EncryptUpdate` or `EVP_DecryptUpdate`, then `EVP_*Final_ex` and the tag controls. The key MUST be given to EVP once, in `make`, with webcpp keeping no copy (each message re-initialises with a null key, which keeps the expanded key, and a seal then an `open` on the same context works), and lengths MUST be fed in pieces of at most `INT_MAX` rounded down to a multiple of 16.

#### Scenario: Seal then open on one context
- **WHEN** `seal` then `open` run on the same OpenSSL `gcm`
- **THEN** `open` verifies the tag (measured on 3.6.4: "good open: ok=1")

### Requirement: Backend OpenSSL: make's errors by the call that failed
`make` on OpenSSL SHALL map by the call that failed: `EVP_CIPHER_CTX_new` returning null gives `errc::not_enough_memory`; `EVP_CIPHER_fetch` returning null, or `EVP_EncryptInit_ex` returning 0, gives `backend_failure`, whatever the cause, because the error queue cannot tell a memory failure apart across versions (3.6.4 raises `malloc failure`, 3.0.13 raises nothing). The documentation MUST say so.

#### Scenario: The second allocation fails
- **WHEN** a failing resource makes the second allocation of a `make` fail after one successful `make`
- **THEN** `make` returns `backend_failure`

### Requirement: Backend OpenSSL: the error queue is left as found
`make` SHALL call `ERR_set_mark()` after its guards, immediately before `EVP_CIPHER_fetch`, and `ERR_pop_to_mark()` on every path after it, so that nothing it raises remains and the caller's entries survive; the mark MUST NOT come earlier, because it initialises OpenSSL and allocates the thread's error state, which would leave `CRYPTO_set_mem_functions` returning 0. The functions of a message MUST set no mark (the first `ERR_*` call of a thread allocates 6 blocks); on the one path that raises, a `backend_failure`, they call `ERR_clear_error()`, and the documentation says the caller's earlier entries go with it.

#### Scenario: The caller's entry survives
- **WHEN** a test raises an entry of its own before each `make`, including a `make` that fails
- **THEN** after it the queue holds that entry alone

### Requirement: Backend OpenSSL: static, as the provider builds it
The OpenSSL backend SHALL link OpenSSL statically, libssl and libcrypto from the provider's one build directory per configuration, with the link flags the provider gives; the threading of the build MUST follow the program's (`no-threads` under b2's default `threading=single`), and the documentation MUST say so, and that on MSVC, wasip2 and wasip3 the provider builds OpenSSL without assembly (table AES and GHASH, no AES-NI, not constant time).

#### Scenario: Two threads
- **WHEN** a native program built with `<threading>multi` seals and opens on two threads, each with its own object
- **THEN** both succeed and each thread's error queue is left as it was found

### Requirement: Backend ctaes: Bitcoin Core's copy, the block cipher alone
The ctaes backend SHALL be `src/crypto/ctaes` of `github.com/bitcoin/bitcoin` at tag v31.1 (commit `9be056a8a72b624dae9623b2f7bded92c2a21c91`, whose `src/crypto/ctaes` is tree `1b6c31139a71f80245c09597c343936a8e41d021`), as the shallow submodule `third_party/libs/bitcoin`, and `third_party/Jamfile` MUST compile its `src/crypto/ctaes/ctaes.c` alone, with only that directory on an include path, (and ctaes's `test.c` for its own suite). It exports AES-128, AES-192 and AES-256 on whole blocks and nothing else, so `aes256<backends::ctaes>` works and `gcm<aes256<backends::ctaes>>` MUST fail to compile with "webcpp::crypto: ciphers::gcm<ciphers::aes256<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: ciphers::gcm<ciphers::aes256<backends::bearssl>> or ciphers::gcm<ciphers::aes256<backends::openssl>>." (Botan joins the list with increment 5) It MUST NOT be `bitcoin-core/ctaes`, which is a different copy.

#### Scenario: GCM over ctaes
- **WHEN** a program names `gcm<aes256<backends::ctaes>>`
- **THEN** compilation fails with exactly that text, and ctaes's canary is `webcpp_crypto_ctaes_9be056a_default__add_third_party_ctaes_to_your_link`

### Requirement: Backend libsecp256k1: pin, modules and tables
The libsecp256k1 backend SHALL be `github.com/bitcoin-core/secp256k1` tag v0.8.0 (commit `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d`), built as its CMake build does (`src/secp256k1.c`, `src/precomputed_ecmult.c`, `src/precomputed_ecmult_gen.c`) with every module on, `recovery` included. The b2 feature `webcpp-secp256k1-tables` MUST take `auto` (the default), `small` or `large`, with `auto` small on Emscripten and large natively, on wasip2 and on wasip3; small is window 2, comb 2x5 (canary `w2c2x5`), large the defaults, window 15, comb 11x6 (canary `w15c11x6`), and the usage requirement `WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES` MUST be given where the tables are small. On MSVC a static libsecp256k1 gets `SECP256K1_STATIC` as a usage requirement.

#### Scenario: Table sizes on Emscripten at -Os
- **WHEN** the signing program is built on Emscripten with `small` and with `large`
- **THEN** it is 141,922 bytes and 1,211,251 bytes

### Requirement: Backend libsecp256k1: one copy per program
webcpp's build of libsecp256k1 SHALL never be linked with another libsecp256k1, because its tables are global symbols, and trystero MUST leave its external libsecp256k1 in the same change that moves it to `/third_party//secp256k1`.

#### Scenario: trystero after its migration
- **WHEN** a native program of trystero is linked
- **THEN** it names `/third_party//secp256k1` and no external libsecp256k1

### Requirement: Backend Botan: found, native only, with exceptions
The Botan backend SHALL find Botan 3 by `botan-3/botan/aead.h` through `webcpp.external`, check `BOTAN_VERSION_MAJOR == 3`, be native only for now (`/third_party//botan` stops every WebAssembly build, Emscripten included, and `config.hpp` refuses `WEBCPP_CRYPTO_HAS_BOTAN` there), and be available only with exceptions: `backends/botan.hpp` MUST declare nothing under `WEBCPP_CRYPTO_NO_EXCEPTIONS`, and a program built without exceptions that names it gets the `#error` "webcpp::crypto: the backend Botan needs exceptions, and this program is built without them. In the program's Jamfile, name /third_party//bearssl or /webcpp/crypto//openssl in its place." Its link is `webcpp-crypto-botan-link`, shared or static.

#### Scenario: Botan without exceptions
- **WHEN** a program built with `<exception-handling>off` names `/third_party//botan` and uses the default backend
- **THEN** compilation stops with that text, and a headers-alone test without exceptions passes because the header declares nothing

### Requirement: Backend Botan: exceptions caught at the boundary
Every call into Botan SHALL sit in one `try`: `Botan::Invalid_Authentication_Tag` becomes `authentication_failed`, `Botan::Invalid_IV_Length` `invalid_iv_size`, `Botan::Invalid_Key_Length` `invalid_key_size`, `std::bad_alloc` `errc::not_enough_memory` and any other `std::exception` `backend_failure`, so that no exception leaves a webcpp function.

#### Scenario: A tampered message on Botan
- **WHEN** a tag is changed and `open` is called
- **THEN** it returns `authentication_failed` and `data` is zeroed

### Requirement: Backend Botan: scratch, additional data and allocation
`make` SHALL create the cipher objects and a scratch `secure_vector` reserved to one update granularity plus the tag, so that the bulk of a message runs through `process` on the caller's span and finishes on the reserved scratch, which never grows; `seal` and `open` allocate nothing. The backend MUST call `set_associated_data` before every `start` in `seal`, `open` and `begin_seal`, with an empty span when there is none, because Botan keeps the additional data between messages. Botan's `make` allocates through `new` and its own secure allocator, with no hook that could route it (its secure memory goes through fixed functions with no setter, its `secure_allocator` is stateless, and its objects are `std::make_unique`'d), and the documentation MUST say that a program that needs every allocation in its allocator uses OpenSSL with the memory option, or BearSSL. Botan is never the first choice by default: it is the default only when it is the only AEAD backend linked.

#### Scenario: Additional data then none
- **WHEN** a message is sealed with additional data and the next with none, on the same object
- **THEN** the second equals BearSSL's seal of that message with none

### Requirement: Memory option: the define and where it exists
The option SHALL be `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, a macro of the config, for native builds only, given to every target of the program that includes a header of the library, its own libraries included, and every target that gets it MUST name `/webcpp/crypto//openssl`: the project's requirements carry `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE <library>/webcpp/crypto//openssl`, and a WebAssembly build is a project of its own, without the define, naming `/third_party//bearssl`. `config.hpp` MUST refuse it on WebAssembly with "webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE configures OpenSSL, a native backend, and this program is built for WebAssembly. Remove that define from the program's Jamfile." and natively without `WEBCPP_CRYPTO_HAS_OPENSSL` with "webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE routes OpenSSL's allocations, and OpenSSL is not linked. In the program's Jamfile, add /webcpp/crypto//openssl beside /webcpp/crypto//crypto, or remove that define. (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto)".

#### Scenario: The option on wasip2
- **WHEN** a wasip2 program is built with the define
- **THEN** compilation stops with the first text

### Requirement: Memory option: a resource the program defines
Under the option `<webcpp/crypto/backends/openssl.hpp>` SHALL declare `std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept`, which the program defines once, and the tag type `backends::openssl` SHALL gain `static boost::system::result<void> install_memory_functions() noexcept`. The program's function MUST return a resource that lives until the process ends and is never destroyed (made with `new` and never deleted, or placement-new'd in static storage; a function-local static object is not enough), thread-safe wherever OpenSSL may run on two threads, and reached without an exception. The state is kept in the program because OpenSSL's hook carries no user data and the library may hold no global mutable state, the form Boost uses for `boost::assertion_failed` and `boost::throw_exception`.

#### Scenario: The documentation's example
- **WHEN** the example defines `openssl_memory_resource()` as `static auto* const resource = new std::pmr::synchronized_pool_resource; return *resource;` and calls `install_memory_functions()` first in `main`
- **THEN** `aes256_gcm` seals and opens one block and the program prints `aes256_gcm ok backend=openssl`

### Requirement: Memory option: install_memory_functions, guards first
`install_memory_functions()` SHALL only read OpenSSL's state until its guards pass, in this order: when `CRYPTO_get_mem_functions` already gives webcpp's adapter it returns success and changes nothing; when it gives functions other than OpenSSL's own defaults it returns `memory_functions_refused`; it then calls `openssl_memory_resource()` once so that the resource is constructed outside OpenSSL's allocation path; then it calls `CRYPTO_set_mem_functions` with the adapter, and a 0 from it (OpenSSL has already allocated with its own functions) is `memory_functions_refused`. The program MUST call it as the first statement of `main`, before any thread starts; the library MUST NOT install from a static initialiser of its own.

#### Scenario: A late install
- **WHEN** one program calls `EVP_CIPHER_fetch`, frees the cipher, then `install_memory_functions()`
- **THEN** it returns `memory_functions_refused` and OpenSSL's own functions are still in place

#### Scenario: Another party's functions
- **WHEN** another party has installed its own functions with one allocation made through them
- **THEN** `install_memory_functions()` returns `memory_functions_refused` and leaves that party's functions in place

#### Scenario: A second install
- **WHEN** `install_memory_functions()` is called twice
- **THEN** the second returns success and changes nothing

### Requirement: Memory option: recognising OpenSSL's defaults on every link
A function `f` from `CRYPTO_get_mem_functions` SHALL be taken as OpenSSL's default `CRYPTO_malloc` when `f == &CRYPTO_malloc` (ELF and Mach-O, shared or static, and a static libcrypto on Windows) or, on `_WIN32` only, when the module that holds `f` (`GetModuleHandleExW` with `GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT`) exports `CRYPTO_malloc` at `f` (`GetProcAddress`); the same MUST hold for `CRYPTO_realloc` and `CRYPTO_free`. The Windows branch is needed because OpenSSL declares its functions without `__declspec(dllimport)`, so a program linked against `libcrypto-3-x64.dll` takes its own import thunk; it is unmeasured until the MSVC lane runs.

#### Scenario: A program that never used OpenSSL on the MSVC shared link
- **WHEN** a program that never used OpenSSL calls `install_memory_functions()` against the DLL
- **THEN** the install succeeds, which proves the defaults are recognised through the import

### Requirement: Memory option: never silent
Under the option every `make` of the OpenSSL backend SHALL check, after the key size, that OpenSSL's functions are webcpp's adapter (`CRYPTO_get_mem_functions`, three loads) and otherwise return `memory_functions_not_installed` before OpenSSL allocates anything, so that a forgotten install, or functions another party replaced after it, gives an error and not an allocation outside the resource. A program that sets the option, uses the backend and does not define `openssl_memory_resource()` MUST fail to link on that name.

#### Scenario: A forgotten install
- **WHEN** a program built with the option defines the function and calls `make` without the install
- **THEN** `make` returns `memory_functions_not_installed`, OpenSSL has allocated nothing, and a `CRYPTO_set_mem_functions` afterwards returns 1

#### Scenario: A missing definition
- **WHEN** a program sets the option and calls any `make` on OpenSSL without defining the function
- **THEN** it fails to link naming `webcpp::crypto::openssl_memory_resource()` (ld64: "Undefined symbols for architecture arm64")

### Requirement: Memory option: the adapter
The adapter SHALL be three functions, `extern "C"`, `inline`, `noexcept` and of external linkage (never `static` or in an unnamed namespace): `webcpp_crypto_openssl_malloc`, `webcpp_crypto_openssl_realloc` and `webcpp_crypto_openssl_free`. Each block MUST start with a header holding its size, the header's size and the blocks' alignment being the larger of `alignof(std::max_align_t)` and `__STDCPP_DEFAULT_NEW_ALIGNMENT__`. `malloc(n)` MUST return null for 0 bytes or a size whose header would overflow `size_t`, otherwise `allocate(header + n, alignment)` with the size stored and the address after the header returned; `free(p)` MUST do nothing for null, otherwise `deallocate(p - header, header + size, alignment)`; `realloc(p, n)` MUST be `malloc(n)` for a null `p`, free `p` and return null for `n == 0`, otherwise allocate a block of `n`, copy the smaller size and free `p`, and on failure return null leaving `p` valid. A second module that includes the header has its own copy of the adapter, and its `make` returns `memory_functions_not_installed`.

#### Scenario: The adapter's edges
- **WHEN** the adapter's C functions are called directly with an allocation of 0 bytes, one whose header overflows, a `realloc` that grows, one that shrinks, one to 0 bytes, and one that fails
- **THEN** the results are null, null, a copy, a copy, null with `p` freed, and null with the old block still valid

### Requirement: Memory option: exceptions and a failing resource
With exceptions each call into the resource SHALL sit in a `try` whose `catch (...)` returns null, because no exception may cross OpenSSL's C; without exceptions there MUST be no `try`, a resource that throws ends the process at the adapter's `noexcept`, and only a resource that returns null reports a failure. `make` on a failing resource MUST return `not_enough_memory` (from `EVP_CIPHER_CTX_new`) or `backend_failure` (from `EVP_EncryptInit_ex`) by the call that failed; a failure during OpenSSL's first initialisation is permanent for the process, and every later `make` returns `backend_failure`. `seal` and `open` MUST allocate nothing, so a failing resource cannot reach them.

#### Scenario: The first allocation after the install fails
- **WHEN** the first allocation after the install fails, inside OpenSSL's first initialisation
- **THEN** `make` returns `backend_failure`, and so does a `make` after the resource has stopped failing

### Requirement: Memory option: what it routes
Under the option every allocation of the libcrypto the program links, in the whole process (Beast's TLS and libdatachannel's DTLS included), SHALL go through the program's resource, `CRYPTO_zalloc`, `CRYPTO_aligned_alloc`, `CRYPTO_clear_realloc` and `CRYPTO_clear_free` included; it MUST NOT claim to route OpenSSL's secure heap (only if the program calls `CRYPTO_secure_malloc_init`) nor the C library's own allocations inside functions OpenSSL calls, such as the `FILE` and buffer of the configuration file (one or two blocks, by C library). Nothing else in the process MAY call `CRYPTO_set_mem_functions`, and the option does nothing for Botan, BearSSL, ctaes and libsecp256k1.

#### Scenario: Everything through the functions, measured
- **WHEN** the functions are installed first and the first `make`, three seals, a good open, three failed opens and the destructors run (macOS, OpenSSL 3.6.4)
- **THEN** the adapter is called 6,061 times to allocate and 192 to reallocate, and libcrypto makes 0 calls to the C library's allocator

### Requirement: Memory option: exit, threads and the resource's requirements
OpenSSL SHALL free through the installed free function at exit (`OPENSSL_cleanup` via `atexit` on ELF and Mach-O, via `_onexit` on Windows, where with a shared libcrypto it runs at the DLL's unload after the program's static destructors), and when a thread ends; the program's resource MUST therefore be never destroyed and thread-safe whenever webcpp objects are used on two threads or Beast's TLS runs beside them (`std::pmr::synchronized_pool_resource`, `std::pmr::new_delete_resource()`; an `unsynchronized_` resource only in a program with one thread). A bounded resource MUST be sized for the first `make`: 6,061 allocations, 192 reallocations and 4,621 blocks live after it (OpenSSL 3.6.4 on macOS; other versions and configurations differ).

#### Scenario: The exit check
- **WHEN** the counting resource's count of live blocks reaches 0 after `main` has returned
- **THEN** it writes one line with an unbuffered write, the program ends with `_exit(0)`, `main` having returned 3 so that a cleanup that frees nothing fails, and the resource's destructor, which would write its own line and end with `_exit(4)`, never runs

### Requirement: Memory option: a mismatch on MSVC
On MSVC `backends/openssl.hpp` SHALL carry `#pragma detect_mismatch("webcpp_crypto_openssl_memory", "resource")` with the option and `"openssl"` without it, so that two files of one program compiled one with and one without the option, whose `make` differ, fail with LNK2038.

#### Scenario: Two files, one with the option
- **WHEN** an MSVC program links a file compiled with the option and one without
- **THEN** the link fails with "mismatch detected for 'webcpp_crypto_openssl_memory'"

### Requirement: Build: C libraries from their own sources
BearSSL, libsecp256k1 and ctaes SHALL be submodules `third_party/libs/bearssl` (`webcpporg/bearssl`, `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f`), `third_party/libs/secp256k1` (`bitcoin-core/secp256k1`, v0.8.0, `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d`) and `third_party/libs/bitcoin` for ctaes (`bitcoin/bitcoin` shallow, v31.1, `9be056a8a72b624dae9623b2f7bded92c2a21c91`), and `third_party/Jamfile` (project `/third_party`) MUST build each from its own files, one object per file, as a static library on every target: all 294 `.c` files of BearSSL's `src/`, the POWER8 ones included (off POWER they compile to the stubs other files call), for ctaes `ctaes.c` alone. No `-std` is given, so each compiles in its compiler's GNU dialect; the project's requirements are `<link>static`, `<warnings>off`, `-ffunction-sections -fdata-sections` for clang, gcc and Emscripten, and `/Gy /Gw` for MSVC. webcpp's `libs/crypto/src/canary_<name>.c` MUST join each library's sources.

#### Scenario: A user names a backend by its project id
- **WHEN** a Jamfile names `/third_party//bearssl`, and another names `/webcpp/third_party//bearssl`
- **THEN** both resolve to the one project, and `/third_party//nothere` fails with "Unable to find file or target named"

### Requirement: Build: defines beside the sources
`third_party/Jamfile` SHALL set the defines a library cannot work out, each with its reason beside it: BearSSL `BR_RDRAND=0` and `BR_USE_URANDOM=0` on every target; `BR_USE_GETENTROPY=1` and `_DEFAULT_SOURCE` on every target but Windows (wasi-libc and Emscripten declare `getentropy` only under `_DEFAULT_SOURCE`); `-include sys/random.h` on macOS (otherwise `src/rand/sysrng.c` fails with "call to undeclared function 'getentropy'"); `BR_USE_GETENTROPY=0` and `BR_USE_WIN32_RAND=0` on Windows; libsecp256k1 `ENABLE_MODULE_*=1` and the table sizes on every target, `SECP256K1_STATIC` as a usage requirement on MSVC and `WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES` as a usage requirement where the tables are small.

#### Scenario: BearSSL on macOS
- **WHEN** BearSSL is built on macOS with the defines
- **THEN** its seeder reports `seeder=getentropy` and seeds

### Requirement: Build: usage requirements of each library
Each library's target SHALL give as usage requirements its public include directory, `WEBCPP_CRYPTO_HAS_<NAME>=1`, the defines marked usage requirements, and the link-time removal of unused sections (`-Wl,-dead_strip` on Darwin, `-Wl,--gc-sections` on Linux, `/OPT:REF` on MSVC, which `link.exe` leaves off under `/DEBUG`; wasm-ld and Emscripten do it by default).

#### Scenario: A program that calls one function
- **WHEN** a program calls only `gcm` on BearSSL
- **THEN** the unused sections of BearSSL are not in the executable

### Requirement: Build: a library named and not called adds no byte
A program built with `/third_party//bearssl`, `//secp256k1` or `//ctaes` and the same program without it SHALL be byte-identical on all four targets (natively the two linked under the same output name, because ld64's ad hoc signature embeds it); a shared OpenSSL or Botan named and not called still adds its load command (Mach-O) or `DT_NEEDED` entry (ELF), so they are left out. The superproject MUST gain `webcpp.same-program <name> : <sources> + : <library> : <targets> * : <requirements> *`, b2 alone, which builds `<sources>` with and without `<library>` under one output name in two directories of their own, with `/Brepro` on MSVC, and passes when the two files are byte-identical.

#### Scenario: BearSSL named and not called
- **WHEN** `webcpp.same-program` builds an empty program with and without `/third_party//bearssl` on wasip2
- **THEN** the two files are byte-identical

### Requirement: Build: sizes against a unity build
Built file by file, BearSSL's AES-GCM program SHALL never be larger in code than the unity build, and on WebAssembly 0.6 to 2.4 KB smaller (wasip2 -Os stripped 59,887 against 60,570 bytes).

#### Scenario: The size table
- **WHEN** the sizes lane builds the AES-GCM program on BearSSL for wasip2 at b2's `space`
- **THEN** its size is recorded in `sizes.json` and compared with `doc/sizes.adoc`

### Requirement: Build layer 1: compile-time messages
`config.hpp` SHALL classify the target (`WEBCPP_TARGET_*` from `<webcpp/config.hpp>`, the `HAS` defines), refuse impossible combinations and pick the default backend, and the check that a backend is linked MUST sit in the backend's own header. The messages SHALL be: no backend linked on WebAssembly ("webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"); none natively (naming OpenSSL or BearSSL); "the backend BearSSL is used and not linked", and the same for OpenSSL, ctaes and Botan; "<webcpp/crypto/curves/secp256k1.hpp> needs libsecp256k1, which is not linked"; "OpenSSL is not built for Emscripten. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl."; "WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL selects OpenSSL, which is not built for Emscripten. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL."; and "both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL are defined. Keep one." Each MUST come from a test of its own, in a directory of its own: a `webcpp.compile-diagnostic` that states the whole text on the header that raises it, or, for a message raised on Emscripten, a fixture of the tools' test of the user's Jamfiles.

#### Scenario: wasip2 with no backend
- **WHEN** a wasip2 program includes `<webcpp/crypto/backends/bearssl.hpp>` with no backend linked
- **THEN** the first error is the no-backend text, and the lines after it (`use of undeclared identifier 'webcpp'`) come from it

### Requirement: Build layer 2: the link-time canary
Every function of a backend's header that calls the C library SHALL read, with a volatile load, the symbol `webcpp_crypto_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link`, which `src/canary_<name>.c` defines from the library's own compile flags, so that a library missing, of another commit, of another configuration or built without the canary fails to link with that name. The pins MUST be in `<webcpp/crypto/detail/canary.h>`, readable as C and C++; the configurations are BearSSL's seeder (`getentropy`, or `noseeder` on Windows), libsecp256k1's tables (`w15c11x6`, `w2c2x5`) and ctaes `default`. The CI MUST compare each pin with `git -C third_party/libs/<name> rev-parse --short=7 HEAD`.

#### Scenario: BearSSL missing on wasm-ld
- **WHEN** a program that uses BearSSL's backend is linked without BearSSL by wasm-ld (wasi-sdk 34)
- **THEN** the error reads `undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link`, then `error: failed to invoke LLD: exit status: 1`

#### Scenario: A library built without the canary
- **WHEN** BearSSL is built by other means without `canary_bearssl.c`
- **THEN** the canary symbol is the only link error

### Requirement: Build layer 3: detect_mismatch on MSVC
Each backend header and each canary file SHALL carry `#pragma detect_mismatch("webcpp_crypto_backend_<name>", "<name>-<commit7>-<configuration>")` under `_MSC_VER`, one key per backend (one key for all would fail a program that uses BearSSL and libsecp256k1), and `<webcpp/crypto/ciphers/aes256_gcm.hpp>` MUST carry `detect_mismatch("webcpp_crypto_default_backend", "<name>")` so that two files that choose different default backends fail with LNK2038 instead of an ODR violation that links silently. Through a static library a mismatching canary member is not loaded, so another commit gives LNK2001; LNK2038 fires between two of the program's own files.

#### Scenario: Two files with different configurations
- **WHEN** two files of one MSVC program are compiled with different BearSSL configurations
- **THEN** the link fails with "error LNK2038: mismatch detected for 'webcpp_crypto_backend_bearssl'" then "fatal error LNK1319: 1 mismatches detected"

### Requirement: Build: each error case in a directory of its own
Two `exe` of one Jamfile built from the same source and differing only by a free define or a `/third_party` library share one object file in b2, so every error case SHALL have a directory of its own.

#### Scenario: Two cases of one source
- **WHEN** two message tests need the same `.cpp` with different defines
- **THEN** each has its own directory and Jamfile

### Requirement: Build: Botan found, or the build stops
Botan SHALL be found by `webcpp.external`: `-sBOTAN_ROOT` when given, else webcpp's `.local/botan`, else the system's directories, its declaration in `libs/crypto/build.jam`; a user's program that names `/third_party//botan` and does not find it MUST stop the build with an instruction that names where it looked, how to install Botan 3, and `/third_party//bearssl` in its place. OpenSSL is never found: it is the superproject's source-built provider.

#### Scenario: Botan not found for a user's program
- **WHEN** a Jamfile names `/third_party//botan` on a machine without Botan
- **THEN** the build stops with that instruction and exits non-zero

### Requirement: Build: Botan on WebAssembly stops the build
`/third_party//botan` on a WebAssembly build SHALL stop before compiling with "/third_party//botan: Botan is a native backend, and this build is for WebAssembly. In the program's Jamfile, name /third_party//bearssl in its place."; a webcpp test declared native that names it MUST be skipped on WebAssembly without evaluating the stop.

#### Scenario: Emscripten names Botan
- **WHEN** an Emscripten program names `/third_party//botan`
- **THEN** the build stops with that text

### Requirement: Build: one copy of a C library per program
webcpp's build of a C library SHALL never be linked together with another copy of it: asio's `/webcpp/asio//openssl` and crypto's `/webcpp/crypto//openssl` MUST link the one source-built OpenSSL, and trystero MUST link `/third_party//secp256k1`, not a libsecp256k1 of the machine's, once crypto brings it.

#### Scenario: asio and crypto in one program
- **WHEN** a native program names `/webcpp/asio//openssl` and `/webcpp/crypto//openssl`
- **THEN** its link line holds `libssl.a` and `libcrypto.a` once each, from one build directory

### Requirement: Default backend: per target
`config.hpp` SHALL define `default_backend` from `WEBCPP_TARGET_*` and the `HAS` defines for the AEADs: BearSSL on Emscripten, wasip2 and wasip3; natively OpenSSL when linked, else BearSSL, MSVC included. With more than one AEAD backend linked, the default MUST be the first linked of OpenSSL, BearSSL, Botan natively and BearSSL on WebAssembly; a program selects another with `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>` and names a backend at any use with the template argument. The architecture MUST NOT enter the choice.

#### Scenario: Both linked, BearSSL chosen
- **WHEN** a Jamfile links OpenSSL and BearSSL with `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`
- **THEN** `aes256_gcm::backend_type` is `backends::bearssl`

#### Scenario: Measured speed behind the rule
- **WHEN** the throughput of AES-GCM is compared on the CI's runners
- **THEN** OpenSSL is about 1.4 times BearSSL's AES-NI on x86-64, about 30 times on Linux aarch64 (2,821 against 92.4 MiB/s) and about 62 times on macOS-15 (5,571 to 5,908 against 90.1 to 94.1 MiB/s)

### Requirement: Default backend: the documentation warns about ARM
The documentation SHALL warn on the backends' page and on the page of `gcm` that natively without OpenSSL BearSSL is slow on ARM, because BearSSL has no ARMv8 AES or PMULL code, and that a server on ARM names `/webcpp/crypto//openssl`; it MUST NOT give a factor, since the slowdown is not measured on the source-built OpenSSL until the throughput lane, which is deferred after version 0.1.

#### Scenario: The backends page
- **WHEN** the documentation is built
- **THEN** the backends page and the `gcm` page both carry the warning

### Requirement: Tests: every program twice, on every target the backend has
The test Jamfile's rule `crypto-run <name> : <sources> + : <requirements> * : <targets> *` SHALL declare each program twice, the second with `<exception-handling>off` and the suffix `_no_exceptions`, on every target its backend has (Botan's with exceptions only, with a headers-alone test without them). Every test `main` MUST end `return webcpp::test::exit_status(boost::report_errors());` with `#include <webcpp/test/exit_status.hpp>`, and tests use `<boost/core/lightweight_test.hpp>`. A lane or check that builds nothing on a target it names MUST fail, and every own lane (`c-suites`, `constant-time`, `browser`, `sizes`, `throughput`) names its targets.

#### Scenario: A test without exceptions
- **WHEN** `b2 libs/crypto/test` runs
- **THEN** each program has a twin built with exceptions off, and both pass

### Requirement: Tests: AES-GCM vectors
Every case SHALL run on every backend that has the algorithm, sealed and opened one shot and sealed incrementally in pieces of every size from 1 byte up to the whole (then opened one shot): the 18 test cases of the GCM specification; all 316 of Wycheproof's AES-GCM tests (`C2SP/wycheproof` at `12fd3aaf33eb5fa1f52e026912ee00c054f9d984`, 108 with 128-bit keys, 103 with 192, 105 with 256; 229 valid, 87 invalid), of which the 307 with an IV of 1 to 128 bytes run through `seal` and `open` (the 226 valid matching, the 81 invalid failing with `authentication_failed` and a zeroed `data`), the 6 with an empty IV (all invalid) fail with `invalid_iv_size`, and the 3 with a 257-byte IV are expected refusals; and NIST CAVP's `gcmtestvectors`, the cases with 128-bit tags (6 files of 7,875 cases, 1,125 each with 128-bit tags), the shorter tags skipped because the interface offers only 16-byte tags.

#### Scenario: Wycheproof counts
- **WHEN** the Wycheproof AES-GCM file runs
- **THEN** 226 valid cases match, 81 invalid fail with `authentication_failed`, 6 give `invalid_iv_size` and 3 are the expected refusals (226 + 81 + 6 + 3 = 316)

### Requirement: Tests: block cipher and ChaCha vectors
The AES block cipher SHALL be tested with FIPS-197 appendix C and NIST's AESAVS known-answer tests (`KAT_AES.zip`: ECBGFSbox, ECBKeySbox, ECBVarKey, ECBVarTxt for 128, 192 and 256) on ctaes, OpenSSL and Botan, and ChaCha20-Poly1305 with RFC 8439 section 2.8.2 and appendix A.5 (265-byte plaintext, tag `eead9d67890cbb22392336fea1851f38`) and Wycheproof's `chacha20_poly1305_test.json`. NIST's two archives MUST be pinned by URL and SHA-256 (`gcmtestvectors.zip` `f9fc479e134cde2980b3bb7cddbcb567b2cd96fd753835243ed067699f26a023`, `KAT_AES.zip` `a203b16c9246b2ebae31dee5de21a606be80cf78ceabaca37150236fa098eb60`), fetched once by an action and a script into `.local/nist-cavp` (or `-sNIST_CAVP_ROOT`), never tracked.

#### Scenario: A machine without the archives
- **WHEN** webcpp's own CAVP test is built on a machine without `.local/nist-cavp`
- **THEN** it is left out with one line, and the CI, with `webcpp-require-external=on`, stops instead

### Requirement: Tests: secp256k1 vectors
secp256k1 SHALL be tested with BIP340's `test-vectors.csv` (a shallow submodule `third_party/libs/bips` at `927b6de9915c9262615a6399de51b200f81e5aa4`) through `sign_schnorr` and `verify_schnorr`; Wycheproof's `ecdsa_secp256k1_sha256_bitcoin_test.json` (464 tests, 162 valid, 302 invalid), hashing with Boost.Hash2's SHA-256 and converting DER signatures through libsecp256k1's parse and compact serialization (the 162 valid verify; of the 302 invalid, 190 are refused by the DER parser and skipped with that reason, and 112 fail verification as `invalid_signature`); ECDH through libsecp256k1's C API on `ecdh_secp256k1_test.json` (752 tests) and `ecdh_secp256k1_webcrypto_test.json` (496 tests), skipping the cases that test ASN.1 or a JWK label, and every key libsecp256k1 refuses MUST give `invalid_public_key` through `ecdh`; agreement of two parties on random key pairs; and libsecp256k1's own `tests` and `ctime_tests` on webcpp's build.

#### Scenario: The Bitcoin ECDSA file
- **WHEN** the Wycheproof Bitcoin ECDSA file runs through `verify_ecdsa`
- **THEN** 162 verify, 112 give `invalid_signature` and 190 are skipped as unparseable DER

### Requirement: Tests: the C libraries' own suites
The C libraries' own test programs SHALL run on webcpp's build on every target as the own lane `c-suites`, outside the ordinary lanes: BearSSL's `test/test_crypto.c` (88 tests, 28 to 46 s per target), libsecp256k1's `tests` and ctaes's `test.c` (from Bitcoin Core v31.1's `src/crypto/ctaes`).

#### Scenario: BearSSL's suite on wasip2
- **WHEN** the lane `c-suites` runs on wasip2
- **THEN** BearSSL's 88 tests pass

### Requirement: Tests: tampering, the machine and moved-from objects
Every byte of ciphertext, tag and additional data changed in turn SHALL fail with `authentication_failed` and leave `data` zeroed, every guard of GCM MUST have a test (one shot and incremental) that leaves `data` untouched, the incremental seal's machine MUST be tested (a second `begin_seal`, `update` and `result` with no message, a second `result`, an error in the middle, a move during a message, a one-shot call during a message, each followed by a fresh `begin_seal` that works), and every function of a moved-from `aes128/192/256`, `gcm`, `chacha20_poly1305` and `secp256k1::basic_keypair` MUST return `invalid_state`, one test per type and backend. Empty inputs (an empty plaintext with empty additional data, an empty `update`, `fill` of 0 bytes, `equal` of empty spans, `encrypt` of zero blocks, a span whose `data()` is null) MUST give the same tag on every backend.

#### Scenario: GCM case 1
- **WHEN** an empty plaintext with empty additional data is sealed on BearSSL, OpenSSL and Botan
- **THEN** the three tags are equal

### Requirement: Tests: WebCrypto as an outside oracle
In node and in Chrome, WebCrypto SHALL seal the same AES-GCM cases with keys of 16 and 32 bytes and random messages under 16-byte IVs (trystero's), and the results MUST match the library's byte for byte; the lane `browser` runs on Emscripten, and an Emscripten audio worklet gives `fill` `errc::not_supported`.

#### Scenario: Random messages under 16-byte IVs
- **WHEN** random messages are sealed by `aes256_gcm` and by node's WebCrypto under the same key and IV
- **THEN** ciphertext and tag are identical

### Requirement: Tests: cross-backend agreement
Every backend SHALL be compared against every other on the same inputs from a seeded generator so that a failure replays: random keys, IVs of 12 bytes and of random lengths 1 to 64, additional data of 0 to 64 bytes, messages of 0 to 4,096 bytes and across every multiple of 16 near those sizes. What one seals each other MUST open, with byte-identical ciphertexts and tags, for AES-GCM and ChaCha20-Poly1305 (BearSSL, OpenSSL, Botan) and the block cipher (ctaes, OpenSSL, Botan); on WebAssembly each algorithm is compared among the backends built there, which is one, and the vectors and WebCrypto stand in for a second.

#### Scenario: BearSSL against OpenSSL
- **WHEN** `agree_gcm<bearssl_gcm, openssl_gcm>(seed)` runs natively
- **THEN** every ciphertext and tag is identical and each opens the other's

### Requirement: Tests: constant time
Constant time SHALL be checked deterministically on Linux x86-64 and arm64 (Valgrind memcheck's client requests, MemorySanitizer's poisoning for the code webcpp compiles) with the key, plaintext and tag marked uninitialised, covering `equal` (compiled at -O2 and -Os in a test program), `seal` and `open` of every backend, and the secp256k1 signing, plus libsecp256k1's `src/ctime_tests.c`; and statistically with dudect's method on secret inputs natively and under wasmtime. A `WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address, size)` seam, empty unless a test defines it, MUST declassify a verdict where webcpp's code branches on it, each harness MUST fail on a planted leak (and MemorySanitizer's only on its own report of it), and dudect's classes differ only in the secret.

#### Scenario: A planted leak
- **WHEN** the same program with a planted branch on the key runs under memcheck
- **THEN** it fails

### Requirement: Tests: entropy
The entropy tests SHALL fill buffers of 0, 1, 256, 257, 65,536 and 65,537 bytes, check two calls differ, plant a failing source (the test build's seam, `WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY`, makes the source `webcpp_test_entropy`) so that `fill` returns `entropy_unavailable` and zeroes the buffer and `operator()` throws with exceptions on, check `fill` in an Emscripten audio worklet gives `errc::not_supported`, and check BearSSL's `br_prng_seeder_system` as above.

#### Scenario: 257 bytes
- **WHEN** `fill` is given 257 bytes
- **THEN** it succeeds in two pieces of at most 256 bytes

### Requirement: Tests: the memory option, process by process
Each test of the OpenSSL memory option SHALL be a program of its own that defines `openssl_memory_resource()` (without which none links) over a never-destroyed counting resource (a `std::pmr::synchronized_pool_resource` over a `monotonic_buffer_resource` on a fixed buffer whose upstream is `null_memory_resource()`), run natively on Linux, macOS and Windows with MSVC, OpenSSL shared and static, exceptions on and off, and MUST fail on: any call to the C library's `malloc`, `calloc`, `realloc`, `posix_memalign`, `aligned_alloc` or `free` from any caller (interposed on macOS by a library linked into the program, on Linux by the test's own definitions forwarding to `__libc_malloc` and kin, never `dlsym(RTLD_NEXT)`), any allocation in `seal`, `open` or the incremental seal, a block freed through the adapter that the resource did not give, and an exit without the resource's line. The test calls `OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr)` after the install, since reading the configuration file makes the C library allocate. The same program without the option MUST count and report; the tests of a failing resource, a late install, a foreign install, a forgotten install, a replaced install, a second install and the adapter's edges are as the requirements above state; a test on a thread that never touched OpenSSL's queue MUST show `make` may allocate through the resource, `seal` and `open` allocate nothing, and the exit check holds.

#### Scenario: The exit
- **WHEN** the everything-in-the-resource test returns from `main`
- **THEN** OpenSSL's cleanup frees every block through the adapter, the resource writes its line, and the test passes only with that line

### Requirement: Tests: the memory option beside its neighbours
With increment 3 one program SHALL link Boost.Asio's SSL headers and libdatachannel, install first and expect success so that neither uses OpenSSL during static initialisation, and another MUST run a TLS handshake through libssl over a loopback socket under the option, with the counting resource refusing a foreign free and checking the exit.

#### Scenario: A TLS handshake under the option
- **WHEN** a loopback TLS handshake runs with the memory functions installed
- **THEN** it completes and every allocation of libssl is in the counting resource

### Requirement: Tests: every message from a test of its own
Each message of the GCM and block-cipher `static_assert`s, OpenSSL's version check, Botan's, the memory option's two refusals and their run-time errors, the layer-1 `#error`s, the compile stop of `/webcpp/crypto//openssl` on Emscripten, the stops of b2 for Botan (Botan not found for a program, a wrong root, static without the static library, Botan on WebAssembly) and the left-out message SHALL come from a test in a directory of its own that checks the exact text. Two link tests natively MUST show that a program with the memory option and no `openssl_memory_resource()` definition fails to link, one calling `install_memory_functions()` and one only a `make`; a link test per canary case (library missing, of another commit, of another configuration, without the canary) MUST run on ld64, wasm-ld, Emscripten and MSVC; MSVC's `C1189`, `C2338`, `LNK2001` and `LNK2038` are checked by building the cases with MSVC in the tools test on its nightly Windows leg.

#### Scenario: The user's Jamfiles as fixtures
- **WHEN** `python3 tools/test/crypto_jamfiles_test.py` runs on a scratch superproject
- **THEN** it builds each fixture under `tools/test/fixtures/crypto_jamfiles/<case>/` and compares its outcome with `<case>.outcome`, giving a "not found" case only the roots it names

### Requirement: Tests: sizes and throughput
The CI SHALL build, through b2 on every target at `<variant>release` with `<optimization>speed` and `space`, the baseline program, the AES-GCM program on each backend, the signing program with small and large tables, and natively the OpenSSL program, static, writing the table raw and `gzip -9n` into the job's summary and an artifact; the documentation's `doc/sizes.adoc` MUST be regenerated from the sizes lane, its WebAssembly rows (pinned toolchains) checked byte for byte and its native rows reported. Throughput of `seal` and `open` of 100 B and 1 MiB per backend MUST be measured natively on ubuntu-24.04, ubuntu-24.04-arm and macos-15 (the library's `runners.yml`), under wasmtime, under node and in Chrome against WebCrypto, three runs each.

#### Scenario: BearSSL's code size on WebAssembly
- **WHEN** the sizes lane builds the AES-GCM program at -Os stripped
- **THEN** it adds about 15.6 KB raw on wasip2 (59,887 against 44,264 bytes) and about 17.6 KB on Emscripten (26,387 against 8,779)

### Requirement: Tests: lanes and CI levels
The ordinary lanes (native clang and gcc-15, wasip2, wasip3, Emscripten) SHALL run `libs/crypto/test` and the examples on every push; the heavy own lanes (`c-suites`, `constant-time`, `browser`, `throughput`) MUST run at the CI's nightly level built by this plan for own lanes (`matrix.py own-lanes --level push|nightly` and `.github/workflows/nightly.yml`), with the library's `runners.yml` running Linux arm64 and macOS throughput and the arm64 constant-time checks, and the tools job's macOS and Windows legs running `crypto_jamfiles_test.py --target native`.

#### Scenario: A push
- **WHEN** a commit is pushed
- **THEN** the ordinary lanes and the light own lane `sizes` run, and the heavy lanes wait for the nightly run

### Requirement: Documentation: the page and the user's Jamfiles
The documentation SHALL be built by the superproject's `doc` target and include `doc/jamfile.adoc` (the rule: a program names `/webcpp/crypto//crypto` and `/third_party//<backend>`), `aead.adoc`, `block.adoc`, `equal.adoc`, `entropy.adoc`, `backends.adoc`, `memory.adoc`, `errors.adoc` and `sizes.adoc`, with every Jamfile line and every printed result built and run as a fixture (web on Emscripten with BearSSL printing `aes256_gcm ok backend=bearssl`, server with the source-built OpenSSL, both BearSSL default, signing with `schnorr ok`, ctaes with `ctaes aes256 ok`, the memory option). Every public symbol MUST have a Doc Comment (a one-sentence brief, `@param`, `@tparam`, `@return`), and the page of every message of section 7.7 of the specification MUST say what to do.

#### Scenario: The WebAssembly Jamfile
- **WHEN** the fixture `web/` builds `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;` on Emscripten, wasip2, wasip3 and native
- **THEN** it prints `aes256_gcm ok backend=bearssl` on WebAssembly

### Requirement: Documentation: the repository's files
The repository SHALL hold `build.jam` (project `/webcpp/crypto`, alias `crypto`), `meta/libraries.json`, `meta/include-boundaries.json`, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`, the notices `LICENSE-BEARSSL.txt`, `LICENSE-CTAES.txt` (a copy of ctaes's `COPYING` at the pin) and `LICENSE-SECP256K1.txt`, `.gitattributes`, `.gitignore`, `.github/workflows/ci.yml` and `runners.yml`, `doc/` and `example/`; every source file MUST open with the notice "Copyright (c) 2026 WebCpp.org" and the BSL-1.0 lines, be English, without the em dash, and without the banned word the lint checks.

#### Scenario: A file without the notice
- **WHEN** a new `.hpp` is added without the licence notice
- **THEN** the lint fails naming the file

### Requirement: secp256k1: its encodings and the x-only tweak
The interface of `curves::secp256k1` SHALL offer `to_der` (a compact signature as strict DER, at most `max_der_size` = 72 bytes, into a caller's buffer), `from_der` (strict DER, BIP66), `normalize_ecdsa` (the low-S form `verify_ecdsa` requires), `tweak_xonly` (an x-only key tweaked by any 32-byte tweak: the tweaked x-only key and its parity, `secp256k1_xonly_pubkey_tweak_add`) and `basic_keypair::tweak_xonly` (the key pair tweaked in place by any 32-byte tweak, `secp256k1_keypair_xonly_tweak_add`), each a wrapper over libsecp256k1's own function, reading the canary and allocating nothing; a key pair whose tweak gives no valid key is wiped. The caller computes a tweak; crypto MUST NOT compute a protocol's tweak, decode lax DER, serialize transactions, compute sighashes or derive keys.

#### Scenario: BIP341's wallet vectors through the generic tweak
- **WHEN** every `scriptPubKey` and `keyPathSpending` case of BIP341's `wallet-test-vectors.json` is run, the test computing each TapTweak with libsecp256k1's tagged hash and passing it
- **THEN** `tweak_xonly` gives each `tweakedPubkey` (7 cases), and a key pair tweaked by `keypair::tweak_xonly` signs each sighash into the vector's witness (7 inputs)

#### Scenario: A tweak not below the group order
- **WHEN** an x-only key or a key pair is tweaked by 32 bytes of 0xff
- **THEN** both give `invalid_secret_key`, and the key pair is wiped and empty, as moved from

#### Scenario: A high-S signature
- **WHEN** a valid signature's `s` is replaced by `n - s`
- **THEN** `verify_ecdsa` refuses it and `normalize_ecdsa` returns the original, which verifies

### Requirement: Increments: five, in order
The work SHALL proceed in five increments: (1) the foundation and AES-GCM on BearSSL and OpenSSL (config, errors, `equal`, `random_device`, `aes128/192/256`, `gcm` one shot and incremental, both backends, the submodule `bearssl`, the canary, `detect_mismatch`, crypto's alias over the source-built OpenSSL, the tests, the unused-library rule, the documentation; then, after version 0.1, the memory option, constant time, sizes, throughput, the browser lane and NIST's vectors); (2) libsecp256k1 and ctaes (submodules, `secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`, ECDH, `aes128/192/256<ctaes>`, the diagnostic of `gcm<aes256<ctaes>>`, the tables feature, and trystero's build onto `/third_party//secp256k1`); version 0.1 is (1) without its deferred parts and (2); (3) trystero's migration; (4) ChaCha20-Poly1305 on BearSSL and OpenSSL with its part of the memory option's tests; (5) Botan, found, native only, with exceptions only, in every native cross-backend lane. Each increment ends with the definition of green, a review against the plan's Review Focus, and pull requests the owner merges.

#### Scenario: Increment 4 without Botan
- **WHEN** ChaCha20-Poly1305 lands in increment 4
- **THEN** its agreement lane compares BearSSL and OpenSSL, and Botan joins it in increment 5

### Requirement: Migration: trystero moves onto the library
In increment 3 webcpp::trystero (in its own repository, on a branch `crypto`, after its shared-connection track has integrated and with a re-read of the table of call sites) SHALL replace `native/sealer.hpp` and its browser twin with `aes256_gcm` using trystero's 16-byte IVs, its asynchronous `sealer` interface with a direct call, its `random` with `random_device::fill` reporting `entropy_unavailable` as a failure of the operation that needed the bytes (it aborts today natively when the system has no entropy), and its `signer` and `verify` with `secp256k1::keypair` and `verify_schnorr`, its build already naming `/third_party//secp256k1` since version 0.1. Its direct use of OpenSSL MUST go, natively OpenSSL staying a transitive dependency through libdatachannel's DTLS and Beast's TLS, and every test of trystero MUST pass before each commit on its four lanes.

#### Scenario: Entropy runs out natively
- **WHEN** the system has no entropy during a trystero join
- **THEN** the join fails with `entropy_unavailable` instead of aborting
