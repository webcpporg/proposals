## Context

webcpp::crypto's genesis (`../genesis/`) was accepted on 2026-10-11 and revised on 2026-11-14,
when the owner set version 0.1 to its increment 1 without the heaviest parts, and its increment
2. Version 0.1 is being built (the superproject's work tree `webcpp-crypto`, its streams merged
task by task): AES-GCM on BearSSL and OpenSSL, the AES block cipher on OpenSSL and ctaes,
`random_device`, `equal`, and secp256k1 on libsecp256k1 with DER, low-S and a key tweak (the
Taproot tweak of version 0.1, which a change of version 0.1 turns into a generic x-only tweak,
D24). Its headers are `config.hpp`, `error.hpp` (values 1 to 12, 13 and
14 reserved), `equal.hpp`, `random_device.hpp`, `aes.hpp`, `gcm.hpp`, `aes256_gcm.hpp`,
`secp256k1.hpp` and `backends/{bearssl,openssl,ctaes,secp256k1}.hpp`, over the traits
`detail::{block,gcm}_{state,engine}`.

The same day two studies were read: one of the noble and scure stacks (@noble/curves, hashes
and ciphers 2.4.0, @scure/bip32, bip39 and base 2.4.0), one of a TypeScript wallet and identity
stack built on them. They list what programs above webcpp need and where each primitive is best
taken from (`research-roadmap.md`, `research-noble.md` and the second study, in the
superproject's work tree). The owner then decided, the same day, the scope of this change:

- libsodium is webcpp::crypto's next backend;
- webcpp::hash, a new library, owns hashing and key derivation, with backends as crypto's;
- **scope widened:** webcpp::crypto and webcpp::hash become the complete cryptographic
  foundation; every backend exposes everything its library offers, marked in a capability
  table; one setting switches every default backend at once, with per-algorithm overrides;
  defaults come from the build and a committed table of webcpp's own measurements;
- a browser backend on Emscripten, WebCrypto through a parallel asynchronous interface, never
  used by the synchronous one; the browser stays the outside oracle;
- documentation in noble's and libsodium's manner, teaching by use case;
- crypto version 0.1 finishes as planned, and this is version 0.2 onward.

The constraints of the genesis stand (its design's Context): header-only C++20, namespace
`webcpp::crypto`, errors as values, exceptions optional, no global mutable state in a header,
allocation through the program's allocator, every C library a pinned submodule built by b2
(CODING-GUIDELINES.md chapter 5), BSL-1.0, English, no em dash.

## Goals / Non-Goals

**Goals:**

- Every primitive of every backend's library that belongs to cryptography, behind one
  interface shaped as the genesis's, on every target the library builds for.
- A capability table that is the single source of what exists where, from which the refusals,
  a trait and the documentation are generated.
- One setting that chooses every default, and defaults that follow the owner's rule from
  measurements webcpp takes itself.
- WebCrypto as a backend on Emscripten, through an asynchronous family that also runs every
  synchronous backend, so that one program's asynchronous code runs on every target.
- Proof by official vectors, the C libraries' own suites, agreement between backends, the
  browser as an outside oracle, and constant-time checks of the new code.
- Documentation a programmer learns cryptography from, safely.

**Non-Goals:**

- Hashing, XOFs, HMAC, HKDF, PBKDF2, scrypt, Argon2, Keccak and BLAKE3: webcpp::hash's.
- Anything a protocol specification defines (BIPs, EIPs, SLIPs, NIPs and their like): D24.
  Also JOSE, canonical JSON, base58, bech32, OPRF and FROST, which belong to libraries above
  this one.
- Generic curve construction, field and point arithmetic as an API, multi-scalar
  multiplication as an API, Poseidon, FFTs, jubjub, ZIP215 verification, ChaCha8 and ChaCha12,
  FF1, and any deterministic random bit generator (D12).
- A primitive written by webcpp. The constructions webcpp writes are the guards, the adapters,
  the canaries, `equal` and the wipe, as in version 0.1.
- An incremental `open` that releases plaintext before its tag verifies (the genesis's D6
  stands; `secretstream` verifies each chunk before releasing it, D7).

## Decisions

### D1. The boundary with webcpp::hash, and the dependency

A function whose output is a digest, an extendable output, an HMAC as a hash algorithm, a key
derived from a secret and a label, or a password hash belongs to webcpp::hash. A function that
encrypts, authenticates a message under a key object with a constant-time verification, signs,
agrees a key, encapsulates a key, or computes in a group belongs to webcpp::crypto. So
libsodium's `crypto_hash`, `crypto_generichash`, `crypto_shorthash`, `crypto_kdf`,
`crypto_kdf_hkdf_*`, `crypto_pwhash`, `crypto_xof_*`, `crypto_core_keccak1600` and
`crypto_auth_hmacsha256/512` are hash's; `crypto_auth` (HMAC-SHA-512-256 under a key, verified
in constant time) and `crypto_onetimeauth` (Poly1305) are crypto's MACs.

`/webcpp/crypto//crypto` adds `<library>/webcpp/hash//hash`, a header-only alias: signatures
over a prehash (Ed25519ph, ECDSA over a message) take hash's algorithms as template arguments,
the examples hash with them, and HKDF is hash's, which replaces the genesis's "written on
Hash2's hmac when needed". hash never includes crypto's headers.

- **Alternatives:** hash's KDFs inside crypto (refused by the owner, who proposed hash); crypto
  owning `crypto_auth_hmacsha*` (two interfaces for HMAC, Hash2's and crypto's).

### D2. Every backend in full: what each brings

| Backend | Library and pin | What it exposes in crypto | Targets |
| --- | --- | --- | --- |
| `sodium` | libsodium 1.0.22 (`77e1ce5`) | Ed25519, Ed25519ph, X25519, the Ed25519 to X25519 conversion, `crypto_box` (XSalsa20 and XChaCha20 forms) with precomputation, sealed boxes, `crypto_secretbox` (both forms), ChaCha20-Poly1305 IETF and original, XChaCha20-Poly1305, AEGIS-128L, AEGIS-256, AES256-GCM (where the CPU has it), `secretstream`, `crypto_kx`, `crypto_auth`, `crypto_onetimeauth`, the stream ciphers (ChaCha20 IETF and original, XChaCha20, Salsa20, XSalsa20, Salsa20/12, Salsa20/8) and the cores HChaCha20 and HSalsa20, ristretto255 and edwards25519 groups and scalars with their maps from hashes, ML-KEM-768, X-Wing, `randombytes_uniform` and `randombytes_buf_deterministic`, the memory utilities (`is_zero`, `compare`, `increment`, `add`, `sub`, `mlock`), ISO/IEC 7816-4 padding, constant-time hexadecimal and base64, IP address encryption (`crypto_ipcrypt`), guarded memory (`sodium_malloc`) | all four; AES256-GCM native only, guarded memory and `mlock` native only |
| `secp256k1` | libsecp256k1 v0.8.0 (`6e2c8bc`) | the genesis's (with version 0.1's generic x-only tweak, D24), and recovery, raw ECDH, seckey and pubkey tweaks, negation, combination, uncompressed keys, ElligatorSwift (BIP324), MuSig2 (BIP327), silent payments (BIP352), hedged ECDSA | all four |
| `bearssl` | BearSSL (`7bea48e`) | the genesis's, and P-256, P-384, P-521 ECDSA (RFC 6979, raw and DER) and ECDH, X25519, ChaCha20-Poly1305, AES-CTR, AES-CBC, AES-CCM, AES-EAX, RSA (PKCS #1 v1.5 and PSS signatures, OAEP, key generation) | all four |
| `openssl` | OpenSSL 3.5.9 (`45e844f`), the superproject's provider | the genesis's, and P-256, P-384, P-521, Ed25519, Ed448, X25519, X448, ML-KEM, ML-DSA, SLH-DSA, RSA, AES-CTR, CBC, CCM, OCB, KW, KWP, SIV, GCM-SIV, CMAC, ChaCha20-Poly1305 | native, wasip2, wasip3 |
| `ctaes` | ctaes of Bitcoin Core v31.1 (`9be056a`) | the genesis's block cipher; nothing else exists in it | all four |
| `botan` | Botan 3.14.1 (`01af88c`) | Ed25519, Ed448, X25519, X448, P-curves, brainpool, ECDSA, ECDH, RSA, ML-KEM, ML-DSA, SLH-DSA, AES and its modes (GCM, SIV, GCM-SIV, CCM, OCB, EAX, KW, KWP, CMAC), ChaCha20-Poly1305, XChaCha20-Poly1305, the other block ciphers it ships (Camellia, ARIA, SM4, Serpent, Twofish), hash-to-curve on the P-curves | native, Emscripten; wasip2 and wasip3 as increment 7's spike finds (D15) |
| `blst` | blst v0.3.17 (`54e6e55`) | BLS12-381 signatures (minimal public key and minimal signature), proofs of possession, aggregation, aggregate and fast aggregate verification, hash-to-curve to G1 and G2, key generation (EIP-2333's) | all four |
| `browser` | WebCrypto (SubtleCrypto) | AES-GCM, AES-CBC, AES-CTR, AES-KW, ECDSA and ECDH on P-256, P-384, P-521, Ed25519 and X25519 where the browser has them, RSA-PSS, RSA-OAEP, RSASSA-PKCS1-v1_5; asynchronous only | Emscripten |

"Everything its library offers" is read through D1 and the non-goals: a backend's hash
functions go to webcpp::hash's backend of the same library, and what the non-goals exclude is
not exposed. Reduced-round Salsa20 is exposed because libsodium ships it as a public interface;
its page says it is not for new designs.

- **Alternatives:** exposing only what the studies asked for (refused by the owner: "every
  backend exposes everything its library offers"); a second library for the long tail.

### D3. One interface shape for every family

The genesis's shape holds and is generalised. Each algorithm is a type with `static constexpr`
sizes; a family's keyed object is made by `static make(...)` returning
`boost::system::result<T>`, is move-only, wipes its secrets on destruction and on move, and
answers `invalid_state` when empty; a keyed object that can be drawn at random also has
`static generate()`, which draws from `random_device` and reports `entropy_unavailable`. The
backend is the last template parameter, whose default is `default_backend_t<Family>` (D5). The
families and their headers:

| Family | Types (default backend aliases) | Header |
| --- | --- | --- |
| signatures | `ed25519`, `ed25519ph<Hash>`, `ed448`, `ecdsa<Curve>`, `rsa_pss<Hash>`, `rsa_pkcs1<Hash>`, `ml_dsa_44/65/87`, `slh_dsa<Set>` | `ed25519.hpp`, `ed448.hpp`, `ecdsa.hpp`, `rsa.hpp`, `ml_dsa.hpp`, `slh_dsa.hpp` |
| key agreement | `x25519`, `x448`, `ecdh<Curve>` | `x25519.hpp`, `x448.hpp`, `ecdh.hpp` |
| key encapsulation | `ml_kem_512/768/1024`, `x_wing` | `ml_kem.hpp`, `x_wing.hpp` |
| AEADs | `aead<Family, Backend>`: `chacha20_poly1305`, `chacha20_poly1305_original`, `xchacha20_poly1305`, `aegis128l`, `aegis256`, `aes256_gcm_sodium`, `ccm<Cipher, TagSize>`, `eax<Cipher>`, `ocb<Cipher>`, `siv<Cipher>`, `gcm_siv<Cipher>`; the genesis's `gcm<Cipher>` keeps its own class | `aead.hpp` and one header per family |
| boxes | `box`, `box_xchacha20poly1305`, `sealed_box`, `secretbox`, `secretbox_xchacha20poly1305`, `secretstream` | `box.hpp`, `secretbox.hpp`, `secretstream.hpp` |
| key exchange | `kx` | `kx.hpp` |
| ciphers | `ctr<Cipher>`, `cbc<Cipher>`, `chacha20`, `chacha20_original`, `xchacha20`, `salsa20`, `xsalsa20`, `salsa20_12`, `salsa20_8`, `hchacha20`, `hsalsa20`, `key_wrap<Cipher>`, `key_wrap_padded<Cipher>`, the genesis's block ciphers and Botan's | `ctr.hpp`, `cbc.hpp`, `stream.hpp`, `key_wrap.hpp`, `aes.hpp`, `block.hpp` |
| MACs | `auth`, `poly1305`, `cmac<Cipher>` | `mac.hpp` |
| groups | `ristretto255::point`, `ristretto255::scalar`, `edwards25519::point`, `edwards25519::scalar`, `bls12_381::min_pk`, `bls12_381::min_sig` | `ristretto255.hpp`, `edwards25519.hpp`, `bls12_381.hpp` |
| secp256k1 | the genesis's, and D11's | `secp256k1.hpp`, `secp256k1/*.hpp` |
| utilities | `random_uniform`, `random_bytes_deterministic`, `memory.hpp`'s functions, `pad`, `unpad`, `hex`, `base64`, `ipcrypt`, `guarded_memory_resource` | `random.hpp`, `memory.hpp`, `padding.hpp`, `encoding.hpp`, `ipcrypt.hpp`, `guarded_memory.hpp` |

Each family's guards are written once, in its type; a backend's header specialises
`detail::<family>_state<Algorithm, Backend>` and `detail::<family>_engine<Algorithm,
Backend>` (D3 of the genesis). The genesis's `chacha_state` and `chacha_engine` become
`aead_state<families::chacha20_poly1305, Backend>` and its engine: one AEAD template serves
every one-shot AEAD, with a family tag carrying the sizes and the message limit.

- **Alternatives:** libsodium's names (`crypto_box_easy`) as the interface (backend-shaped,
  not algorithm-shaped); free functions per backend (the genesis's rejected alternative);
  `std::array` for secret outputs (D9).

### D4. The capability table is the single source of what exists where

`libs/crypto/meta/capabilities.json` lists every algorithm (by family and name), and for each
backend the targets where it has it, whether its availability is decided at run time (libsodium's
AES256-GCM by the CPU, the browser's Ed25519 by the browser), whether it is constant time on
each target, and the increment that adds it. `tools/crypto/capabilities.py` (superproject)
generates from it:

- `include/webcpp/crypto/detail/capabilities.hpp`: per algorithm, a string literal naming the
  backends that have it and their targets, and per (algorithm, backend) the primary engine's
  refusal, `static_assert(detail::dependent_false<Backend>, "webcpp::crypto: <backend> has no
  <algorithm>. The backends that have it: <list>.")`, the text concatenated from those
  literals;
- `supports_v<Algorithm, Backend>` (public, `supports.hpp`): `true` exactly where the table says
  the backend has the algorithm on the current target, so that generic code can choose;
- `doc/capabilities.adoc`: the table a user reads, and the documentation's per-backend lists.

`capabilities.py --check` fails when a generated file differs from what the table gives; the
library's test `capabilities_test.cpp`, generated too, asserts `supports_v` for every cell on
the target it runs on and instantiates every cell marked present, so that a specialisation
missing or present against the table fails the build. A run-time-available cell is present in
the table, `supports_v` is true, and `make` returns `boost::system::errc::not_supported` when
the machine lacks it.

- **Why:** one fact written once cannot disagree with itself; the messages name what exists
  because the table does.
- **Alternatives:** messages written by hand in each backend header (the genesis's way for two
  backends, which does not scale to eight); a table in the documentation only.

### D5. One define chooses every default; a webcpp-wide umbrella; per-family overrides

Every setting is a macro tested with `#ifdef`, never a macro with a value (the coordinator's
ruling of 2026-11-14, which webcpp::hash applies alike):

- **crypto's profile**, at most one of `WEBCPP_CRYPTO_PROFILE_FASTEST` (for each algorithm, the
  linked backend with the highest measured throughput on this target and architecture),
  `WEBCPP_CRYPTO_PROFILE_SMALLEST` (the linked backend that adds the fewest bytes on this
  target), `WEBCPP_CRYPTO_PROFILE_PORTABLE` (the backend the table marks portable, one that
  exists on all four targets, so that a program links the same backends and behaves the same
  everywhere; an algorithm no backend has on all four targets is refused under it, naming its
  backends) and `WEBCPP_CRYPTO_PROFILE_BACKEND_<NAME>` (`SODIUM`, `BEARSSL`, `OPENSSL`, `BOTAN`,
  `BLST`, `SECP256K1`, `CTAES`: that backend for every algorithm it has, the next level for
  the others);
- **the webcpp-wide umbrella**, at most one of `WEBCPP_PROFILE_FASTEST`,
  `WEBCPP_PROFILE_SMALLEST` and `WEBCPP_PROFILE_PORTABLE`, read by crypto's and hash's
  `config.hpp` alike (and by every later library with backends), so that one define changes
  every library's defaults;
- **per-family overrides**, at most one per family, `WEBCPP_CRYPTO_DEFAULT_BACKEND_<FAMILY>_<NAME>`
  (`WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_OPENSSL`), the families being the capability
  table's; the genesis's `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and `_OPENSSL` keep their
  meaning as spellings of `WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_BEARSSL` and `_GCM_OPENSSL`;
- **with none of them, the build's measured default** (`automatic`): natively `fastest`, on
  WebAssembly `smallest`, the owner's rule.

**Precedence:** a per-family override, then crypto's profile, then the webcpp-wide profile,
then the measured default. A named-backend profile that lacks an algorithm passes it to the
next level. Two of crypto's profiles, two webcpp-wide profiles, or two overrides of one family
stop compilation with an `#error` naming both macros; crypto's profile beside a webcpp-wide one
is allowed, and crypto's wins.

Mechanism: `config.hpp` counts the defined macros of each level with `#if` and resolves the
level into `WEBCPP_CRYPTO_DETAIL_PROFILE_*`, then declares `using profile = profiles::<one>;`
(`automatic`, `fastest`, `smallest`, `portable`, or the backend's alias). The generated
`detail/capabilities.hpp` declares, per family, one `#ifdef` per backend that has it, the
two-at-once `#error`, and the override's type. `default_backend_t<Family>` walks the generated
ranking of D6 for the profile, the target and the architecture, and takes the first backend
that is linked (its `HAS` macro), has the algorithm on this target, and is not
run-time-available; when none is linked, it is `detail::none_linked<Family>`, whose engine's
refusal names the Jamfile lines that would give one. On MSVC, `config.hpp` carries
`#pragma detect_mismatch("webcpp_crypto_profile", "<resolved profile>")` and each family's
resolved override its own key, extending the genesis's net for two translation units that
choose differently.

- **Why one macro per value:** a setting is then testable with `#ifdef` by any header, by
  b2's `<define>` and by a program's own preprocessor code, and two at once is detectable,
  which a token value is not; webcpp::hash uses the same form, so the umbrella reads the same
  way in both.
- **Why an umbrella:** the owner's "one setting switches every default backend at once" is about
  the program, not one library; `WEBCPP_PROFILE_SMALLEST` makes a whole web program small.
- **Why never a run-time-available backend as a default:** a default must work on every machine
  of its target; a program names libsodium's AES256-GCM itself, and handles `not_supported`.
- **Limit:** a misspelt macro (`WEBCPP_CRYPTO_PROFILE_FASTES`) is not seen by the
  preprocessor; the profile page lists every macro, and `crypto::profile_name`, a
  `std::string_view` of the resolved profile, lets a program print or assert what it got.
- **Alternatives:** one token-valued macro (`WEBCPP_CRYPTO_PROFILE=fastest`, the first draft:
  not testable with `#ifdef`, and hash's form differed); a run-time choice (the backend is a
  type, and a program must not carry what it does not call); a ranking written by hand (the
  owner asked for a measured one).

### D6. The measurement table: webcpp's own, regenerated at every pin move

`libs/crypto/meta/measurements.json` holds, for each (algorithm, backend): the bytes it adds to
a program over the baseline program on each target (native at `<optimization>space` stripped,
wasip2, wasip3 and Emscripten at `-Os` stripped, raw and `gzip -9n`), and its throughput
natively on the CI's runners (Linux x86-64, Linux arm64, macOS arm64), and under wasmtime and
node for `fastest` on WebAssembly, each the median of five runs of a fixed operation per
family (seal and open of 16 KiB, sign and verify, agree, encapsulate and decapsulate).
`tools/crypto/measure.py` measures and writes it, and generates
`include/webcpp/crypto/detail/rankings.hpp` and `doc/measurements.adoc`.

- The size ranking is checked byte for byte by the own lane `sizes` on every push (pinned
  toolchains give the same bytes); a pin move regenerates the file in the same change, as
  chapter 5 rule 12 asks of the size table.
- The throughput ranking is measured by `runners.yml` at every pin move and on demand; two
  backends whose medians differ by less than 10 % keep their committed order, so that noise
  never flips a default. A native architecture absent from the table takes the `portable`
  choice.
- This supersedes the genesis's Task 22 (sizes, throughput and `runners.yml`), which this
  change's increment 1 builds in the table's form.

- **Why per architecture:** the genesis measured BearSSL 30 to 62 times slower than OpenSSL on
  ARM and 1.4 times on x86-64; a measured rule that ignored the architecture would be wrong on
  one of them. The genesis's requirement that the architecture never enters the choice is
  modified accordingly.
- **Alternatives:** throughput on WebAssembly left out (then `fastest` would mean nothing
  there); a ranking per target only.

### D7. The libsodium backend

`backends::sodium` uses `/third_party//sodium` as webcpp::hash builds it (hash's D11: every
`.c` file one object, upstream's `build.zig` defines, no assembly); this change adds
`libs/crypto/src/canary_sodium.c` and `WEBCPP_CRYPTO_HAS_SODIUM=1` to that target, as hash adds
its canary to BearSSL's. Every engine calls `sodium_init()` first; its state is libsodium's own,
initialised once under libsodium's lock.

- **No assembly.** libsodium's x86-64 assembly (`HAVE_AMD64_ASM`, `HAVE_AVX_ASM`) speeds
  Curve25519 and Salsa20 and is GNU as only; one configuration per target, MSVC alike, keeps
  the canary's configuration simple. Increment 1 measures X25519 and Ed25519 with and without it
  on Linux x86-64 and records the numbers in `doc/measurements.adoc`; enabling it later is a
  change of its own, with its own canary configuration.
- **Entropy.** libsodium 1.0.22's default generator reads the system natively and on WASI
  (`randombytes_sysrandom`) and `crypto.getRandomValues` on Emscripten, the sources
  `random_device` reads (the genesis's D10): the build defines nothing that selects
  `randombytes_internal`, and webcpp never calls `randombytes_set_implementation`. webcpp's own
  key generation draws seeds from `random_device` and calls libsodium's `seed_keypair`
  functions; libsodium's generator is reached only where libsodium draws itself (a sealed box's
  ephemeral key, a secretstream header).
- **Ed25519 is RFC 8032's with libsodium's strict verification** (canonical encodings, no small
  order key, no non-canonical `S`); the page says that other implementations accept some
  signatures libsodium refuses, and that honest signatures agree everywhere.
- **AES256-GCM** exists only where the CPU has AES and carry-less multiplication: natively, with
  `make` returning `not_supported` otherwise; it is never a default (D5).
- **Ed25519ph** is an incremental signer over libsodium's `crypto_sign_ed25519ph_*`, typed
  `ed25519ph<sha2_512>` because RFC 8032 fixes SHA-512.
- **Allocation:** libsodium's crypto functions allocate nothing; guarded memory is the one
  allocator, and it is the program's choice.

- **Alternatives:** a second copy of libsodium for crypto (chapter 5 rule 10 forbids it);
  libsodium's prebuilt Emscripten and WASI packages (hash's D11 refused them).

### D8. X25519 and the conversion refuse weak keys

`x25519::agree` returns `weak_public_key` when the shared secret is all zero (libsodium's
`crypto_scalarmult` returns -1 then; the other backends are checked by webcpp's code after the
call, the one comparison `equal` makes). `ed25519::keypair::x25519_secret()` and
`ed25519_to_x25519_public` are libsodium's conversion functions; a public key of small order or
not canonical is `invalid_public_key`. `crypto_box` and `crypto_kx` refuse what libsodium
refuses. Every backend of X25519 gives the same answer on Wycheproof's X25519 file, and where a
backend accepts what another refuses, the test records the difference as an expected one.

### D9. Secret outputs are `secret<N>`, which wipes itself (BREAKING for `ecdh`)

`secret<N>` is an array of `N` bytes that wipes itself by volatile stores on destruction and on
move, copyable only explicitly (`clone()`), convertible to `std::span<std::uint8_t const, N>`.
Every new function whose output is secret (a shared secret, session keys, a derived secret
scalar, a KEM's shared secret, a converted secret key) returns it. The genesis's
`secp256k1::basic_keypair::ecdh` changes to return `secret<32>` in increment 2.

- **Why:** a shared secret returned in a `std::array` stays on the stack of every caller that
  copies it; the library wipes its keys (the genesis's rule) and should not leave the secrets
  it derives behind. Version 0 allows the change; the changelog says it.
- **Alternatives:** `std::array` everywhere (consistent with version 0.1, and leaves secrets);
  `secret<N>` only for new functions (two conventions in one header).

### D10. The errors 15 to 20

`weak_public_key` (15: a public key of small order, or one that gives an all-zero shared
secret), `invalid_point` (16: an encoding that is not a canonical element of the group),
`invalid_scalar` (17: a scalar not reduced, or zero where zero is refused), `invalid_padding`
(18: padding that `unpad` or a CBC decryption cannot remove; one value for every cause, so that
no padding oracle is built from it), `invalid_encoding` (19: hexadecimal or base64 that does
not decode), `invalid_ciphertext_size` (20: a KEM or RSA ciphertext of another size than the
key's). Values are fixed and never reused; 13 and 14 stay reserved for the memory option.
Asynchronous operations report `boost::asio::error::operation_aborted` when cancelled, and
`boost::system::errc::not_supported` where a browser lacks an algorithm.

### D11. libsecp256k1's remaining modules (increment 2)

All modules are already compiled (chapter 10's `secp256k1` row); increment 2 wraps them:

- `recover` and `basic_keypair::sign_recoverable` (65-byte signatures, `r || s || v` with the
  recovery id), for Ethereum;
- `ecdh_x` (the 32-byte x coordinate) and `ecdh_point` (the 65-byte uncompressed point), through
  `secp256k1_ecdh` with a hash function of webcpp's that copies, for Nostr's NIP-04 and NIP-44
  and the wallet stack;
- `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `pubkey_tweak_add`,
  `pubkey_tweak_mul`, `pubkey_negate`, `pubkey_combine` (on `secret<32>` and on public keys);
- `parse_public_key` and `serialize_public_key` in compressed and uncompressed forms;
- hedged ECDSA (`sign_ecdsa` with 32 bytes of extra entropy, libsecp256k1's `ndata`);
- ElligatorSwift (`ellswift_encode`, `ellswift_create`, `ellswift_xdh` with BIP324's hash);
- MuSig2 (BIP327): key aggregation (`musig::key_aggregate`, with its tweaks), nonce generation
  into a `musig::secret_nonce` that is move-only and consumed by `partial_sign`, so that a nonce
  is never used twice, partial signatures, their verification and aggregation;
- silent payments (BIP352): the sender's outputs, the recipient's scan with labels, through the
  module's own functions.

Each is a thin wrapper over libsecp256k1's function, allocating nothing beyond the key pair's
context; protocols built on them (transactions, PSBT, BIP32 derivation, Taproot's `TapTweak`,
addresses) are the domain libraries' (D24), and increment 2 brings back neither the Taproot
tweak nor `from_lax_der`, which version 0.1 moves out.

### D12. BearSSL's remaining parts (increment 3), and no DRBG anywhere

BearSSL brings P-256, P-384 and P-521 through `br_ec_all_m31` (`br_ec_p256_m62` natively where
64-bit multiplication is constant time, chosen as the genesis chose its AES code), ECDSA with
RFC 6979 nonces, raw and ASN.1, ECDH, X25519 (`br_ec_c25519_m62`/`m31`), ChaCha20-Poly1305
(the genesis's planned engine, its Task 35), AES-CTR, AES-CBC (PKCS #7 padding by webcpp's
guard, `invalid_padding` for every cause), AES-CCM, AES-EAX and RSA (`br_rsa_i62`/`i31`:
PKCS #1 v1.5 and PSS signatures, OAEP, key generation drawing from `random_device`). RSA is in
scope because BearSSL, OpenSSL, Botan and the browser all have it and WebAuthn and JWT use it.
BearSSL's HMAC-DRBG and AES-CTR-DRBG are not exposed, nor any backend's DRBG: `random_device`
is the only generator (the genesis's D10), and a deterministic stream for tests is
`random_bytes_deterministic` (libsodium's, a keyed ChaCha20 stream).

### D13. The browser backend and the asynchronous family (increment 4)

`backends::browser` exists on Emscripten only; its header refuses another target with the fix.
Its keys are WebCrypto `CryptoKey` objects, held in a JavaScript table and named in C++ by a
handle, never by bytes once imported (an imported key may be non-extractable). Its operations
are WebCrypto's promises, so it has no synchronous member: `seal` on a browser `gcm` is
refused at compile time with "backends::browser is asynchronous: call async_seal".

The asynchronous family, `<webcpp/crypto/async.hpp>`, is free functions `async_make`,
`async_generate`, `async_seal`, `async_open`, `async_encrypt`, `async_decrypt`, `async_sign`,
`async_verify`, `async_agree`, `async_wrap`, `async_unwrap` and `async_export`, each an Asio
asynchronous operation in webcpp::asio's model (`async_initiate`, any completion token: a
callback, `use_awaitable`, Cobalt's, `use_future`) completing with
`void(boost::system::error_code, T)`, as webcpp::hash's `async.hpp` is (hash's D23). It exists
for the algorithms WebCrypto offers, and for the heavy operations of D23:

- on `backends::browser` it calls WebCrypto, with or without pthreads (SubtleCrypto exists in
  workers too), and awaits the promise through `webcpp::asio::browser::async_await`, completing
  on the handler's associated executor; the caller's spans must live until completion, as
  Boost.Asio's buffers do;
- on a synchronous backend, for an algorithm the browser offers, it runs the synchronous call
  and completes through `asio::post`, never inline, so that asynchronous code written once runs
  on every target;
- a cheap algorithm the browser lacks has no `async_` variant (as hash's);
- the synchronous API never calls WebCrypto, and its results on Emscripten come from C code.

DOMExceptions map to the library's errors: `OperationError` on a decryption to
`authentication_failed`, `NotSupportedError` to `not_supported`, `DataError` to the size or key
error of the call, anything else to `backend_failure`. Algorithms a browser may lack (Ed25519,
X25519) are run-time-available cells (D4); Chrome's refusal of AES-192 is `not_supported`.

The browser is also the outside oracle: the lane `browser` runs, in node and in Chrome, every
algorithm WebCrypto has against every synchronous backend that has it, byte for byte where the
algorithm is deterministic and by cross-verification where it is not (ECDSA, RSA-PSS).

- **Why a parallel family, not WebCrypto under the synchronous API:** WebCrypto cannot be waited
  on synchronously without Asyncify or JSPI, which would make every synchronous call suspend
  (the owner's amendment C1 of the genesis removed exactly that); a parallel family keeps the
  synchronous API pure and gives the browser's non-extractable keys to programs that want them.
- **Alternatives:** the family on the browser only, never posting on other backends
  (asynchronous code would differ by target);
  `emscripten::val` and embind (a dependency the rest of webcpp does not take; `EM_JS` and a
  handle table suffice).
- The header and `backends/browser.hpp` include webcpp::asio, a heavy dependency, so the
  convenience header leaves them out and says so (chapter 3).

### D14. blst (increment 6): upstream's translation units, assembly where upstream has it

blst is the submodule `third_party/libs/blst` at v0.3.17. Its sources are written to be compiled
as two translation units, `src/server.c` (which includes every other C file) and
`build/assembly.S`; webcpp builds those two objects, one per file as upstream does, which is
how chapter 5 rule 2's "file by file, never a unity source of webcpp's" applies to it, and
`-ffunction-sections` still drops what a program does not call. Natively on Linux and macOS,
x86-64 and arm64, the assembly is used with `__BLST_PORTABLE__` (the ADX instructions chosen at
run time, so one binary runs on every x86-64); on MSVC and WebAssembly `__BLST_NO_ASM__` (blst's
constant-time C). The configuration is in the canary's name. The interface:
`bls12_381::min_pk` and `min_sig` (public keys in G1 and signatures in G2, or the reverse),
each with `secret_key::make(ikm)` (the IETF BLS draft's `KeyGen`, `blst_keygen`), `sign`, `verify`, `aggregate`,
`aggregate_verify`, `fast_aggregate_verify`, proofs of possession, and `hash_to_g1`,
`hash_to_g2` (RFC 9380 suites with a caller's domain separation tag). Pairing and field
arithmetic are not exposed, nor blst's EIP-2333 key tree (`blst_derive_master_eip2333`,
`blst_derive_child_eip2333`), which is a protocol's (D24) and stays reachable through blst's
C API for webcpp::ethereum.

### D15. Botan as a pinned submodule built by b2 (increment 7)

Botan becomes `third_party/libs/botan` at 3.14.1, which supersedes the genesis's D15 for Botan
(found on the machine) and its Task 37. Botan's module selection is upstream's `configure.py`,
run once per target and configuration into the b2 variant directory with
`--minimized-build --enable-modules=<the modules webcpp exposes> --os=<target>`, which writes
`build.h` and the list of sources; `tools/webcpp-botan.jam` reads that list and compiles each
source as an object of its own, never patching upstream. This is chapter 5 rule 7's kind of
exception (upstream's configuration, b2's compilation), written as a new rule of chapter 5 by
the increment.

- **Targets.** Natively and on Emscripten (`--os=emscripten`, which Botan supports). The
  increment opens with a spike on wasip2 and wasip3: `configure.py` with `--os=none` or the
  nearest generic OS and the operating-system features turned off; the cells of the table are
  `yes` where the spike's build passes Botan's own tests under wasmtime, and `no` otherwise,
  the spike's report recording which.
- **Exceptions.** Botan reports errors by throwing; the genesis's D19 stands: its header declares
  nothing without exceptions, its tests declare `<exception-handling>on`, and every exception
  is caught at the boundary and mapped.
- **Randomness.** Botan's operations that need randomness get webcpp's
  `detail::botan_random`, a `Botan::RandomNumberGenerator` whose `fill_bytes_with_input` draws
  from `random_device`; Botan's own RNGs are never instantiated (the genesis's D10).
- **Allocation.** The genesis's D17 departure for Botan stands, re-measured on 3.14.1.

- **Alternatives:** the amalgamation (a unity source, which chapter 5 refuses); a list of
  sources written by hand (it would drift from upstream's modules at every pin move).

### D16. OpenSSL's remaining algorithms (increment 8)

Through the existing provider (`/webcpp/crypto//openssl`), on native, wasip2 and wasip3. A
keyed object holds its `EVP_PKEY` and a context initialised once in `make`; operations reuse it
where OpenSSL allows. OpenSSL's public-key operations may still allocate inside OpenSSL per
call: increment 8 measures each with the counting tests and records the departure per
algorithm in the documentation, and the memory option (genesis Tasks 15 and 16), when built,
routes those allocations. The genesis's note on `no-asm` builds (MSVC, wasip2, wasip3) extends
to every algorithm: the measurement table shows the cost, and `automatic` follows it.

### D17. Hash-to-curve: where a released library has it

RFC 9380's hash-to-curve comes from blst (BLS12-381 G1 and G2) and Botan (the P-curves).
libsodium 1.0.22 has the maps (`crypto_core_ed25519_from_uniform`, `_from_hash`,
`crypto_core_ristretto255_from_hash`), which increment 5 exposes, and not the full
`hash_to_curve` with `expand_message`: its `crypto_core_*_from_string` functions are on
libsodium's `master` since February 2026 and in no release. They join when a libsodium release
holds them, which moves the shared pin with webcpp::hash as a change of its own. webcpp does
not write `expand_message_xmd` over libsodium's map in the meantime, and secp256k1 has no
mature C hash-to-curve, so it has none.

- **Alternatives:** pinning libsodium's `master` (a pin is a release); webcpp's own
  `expand_message_xmd` (a construction webcpp would write, refused while a release is coming).

### D18. Tests

- **Vectors per backend:** Wycheproof wherever it has a file for the algorithm (EdDSA, X25519,
  X448, ECDSA and ECDH per curve, ChaCha20-Poly1305, XChaCha20-Poly1305, AEGIS, AES-GCM,
  AES-CCM, AES-EAX, AES-SIV, AES-GCM-SIV, AES-KW, CMAC, RSA, ML-KEM, ML-DSA); the RFCs (8032,
  7748, 8439, 9380, 9496, 5297, 8452, 3394, 5649, 8017); the drafts' vectors for XChaCha20 and
  AEGIS; NIST ACVP's JSON vectors (ML-KEM, ML-DSA, SLH-DSA, AES modes, ECDSA), a shallow
  submodule `third_party/libs/acvp` pinned by the increment that first reads it; BIP vectors
  (BIP327, BIP324, BIP352 from the `bips` submodule) for the libsecp256k1 modules that
  implement them, the consensus specifications' BLS vectors for blst, and c-kzg-4844's own
  vectors for KZG; a vector is test data, and testing a primitive with it brings no protocol
  into the library (D24).
- **The C libraries' own suites** on webcpp's build, in the own lane `c-suites`: libsodium's
  `test/default` (with its `.exp` files), Botan's `botan-test` for the modules built, blst's own
  tests where it ships them.
- **Agreement:** every pair of backends that has an algorithm, natively and on each WebAssembly
  target where two have it, on inputs from a seeded generator, with the recorded expected
  differences of D8.
- **The browser** as the outside oracle (D13).
- **Constant time:** the genesis's harness (its Task 20: memcheck's client requests,
  MemorySanitizer, dudect, the planted leak) built in increment 1 over libsodium's code and the
  code webcpp adds, then extended by each increment to its backend's secret-dependent paths.
- **Messages and the table:** every refusal from a test of its own (the genesis's rule), and the
  generated table test of D4.

### D19. Documentation

- **A page per algorithm** in noble's form: what it is and when to use it, a complete example
  that compiles and runs as a fixture, its pitfalls (nonce reuse, small-order keys, malleability,
  message limits, what is and is not constant time on each target), and its backends from the
  capability table.
- **libsodium's documentation** (`jedisct1/libsodium-doc` at `6d1ea27`, ISC) is adapted for the
  libsodium families: each adapted page carries an attribution line, and the library ships the
  notice as `LICENSE-LIBSODIUM-DOC.txt`.
- **Wikipedia** (CC BY-SA) is cited by link where a page needs background, and never copied.
- **Teaching chapters by use case:** peer-to-peer networks (identities, signed and encrypted
  messages, key exchange), cryptocurrencies (secp256k1, recovery, MuSig2, silent payments,
  BLS, KZG), privacy (boxes, sealed boxes, secretstream, post-quantum hybrids, IP address
  encryption). Each is framed as raising a program's security: what goes wrong without the
  primitive, what it guarantees, how to use it right. Each example is webcpp's own code,
  written after what a real public application built on noble does with the primitives (the
  ECDH and XChaCha20 a Nostr client composes, the recoverable signatures an Ethereum library
  makes, the Schnorr signatures a Bitcoin signer makes, a passkey verifier's ECDSA), naming and
  linking that application, never copying its code; the protocol itself (NIP-44's format, a
  transaction's sighash) is the domain library's, and the chapter says which.

### D20. Increments and versions

1. libsodium core, profiles, capability table, measurements, constant-time lane, documentation
   form: **version 0.2** with increment 2.
2. libsecp256k1's remaining modules: version 0.2.
3. BearSSL's remaining parts.
4. The browser backend and the asynchronous family.
5. libsodium completed (D2's remainder, D17's maps).
6. blst.
7. Botan as a submodule.
8. OpenSSL's remaining algorithms.
9. KZG polynomial commitments over c-kzg-4844 (D25).
10. The BN254 pairing over mcl (D26).
11. Shamir secret sharing over dsprenkels/sss (D27).
12. HPKE (RFC 9180) over OpenSSL and Botan (D28).

After 0.2, each increment that completes is the next minor version, the owner naming each
release. The genesis's deferred work (the memory option, NIST CAVP, the nightly level) and its
increment 3 (trystero's migration) keep their own schedule; its Tasks 20, 22, 23, 35 and 37 to
39 are absorbed here (D6, D13, D15, D18).

### D21. Parallel streams

- **Increments 1 and 2 run beside each other** from the start: 2 touches only `secp256k1.hpp`,
  its tests and its documentation; its rows join the capability table once 1's table lands, by a
  rebase.
- **After increment 1 merges, increments 3 to 8 run as six parallel streams.** Each touches its
  own backend header, its own `third_party` target and its own tests; they share the generated
  files (`capabilities.json`, `measurements.json` and what they generate), whose conflicts are
  resolved by regenerating after the rebase, never by hand. Increment 4's oracle covers the
  algorithms on `main` when it merges, and each later increment adds its own oracle cases.
- **Increments 9 to 12 join the streams as their dependencies land:** 10 (mcl) and 11 (sss)
  bring backends of their own and run beside 3 to 8 from the merge of 1; 9 builds on blst and
  starts when 6 merges; 12 needs OpenSSL's remaining algorithms natively and Botan for
  WebAssembly, and starts when 8 merges, its WebAssembly cells joining when 7 merges.
- **Merges follow the value order** (3 before 4 before 5, ...), a stream that finishes early
  waiting for its turn only if its rebase conflicts.

### D22. What changes in the genesis's requirements

MODIFIED (whole text in `specs/crypto/spec.md`): the target and namespaces, the public headers,
the convenience header, nothing of hashing, every call synchronous, the backends, the default
backend per target, the trystero-era Botan requirement, the increments, ECDH's return type.
REMOVED: Botan found on the machine and its build stop, replaced by D15. Everything else of
the genesis stands.

### D23. Heavy operations on another thread, only in a build with threads

The owner's decision of 2026-11-14, the same rule as webcpp::hash's D23. `config.hpp` defines
`WEBCPP_CRYPTO_HAS_THREADS` natively when `BOOST_HAS_THREADS` is defined (b2's
`threading=multi`) and on Emscripten when `__EMSCRIPTEN_PTHREADS__` is (`-pthread`), never on
wasip2 or wasip3, unless the program defines `WEBCPP_CRYPTO_NO_THREADS`. Only then do the
heavy operations get `async_` variants that truly run elsewhere: each takes an executor first
(natively a `webcpp::asio::thread_pool`'s, on Emscripten a pool whose threads are Web Workers),
runs the synchronous work there, and completes on the handler's associated executor. Without
threads the variant is declared as a template whose use fails with "webcpp::crypto: <name> runs
on another thread and needs a build with threads (natively b2 threading=multi; on Emscripten
-pthread). Without threads, call <synchronous name>." The browser's own asynchronous
operations need no thread and exist with or without them.

**Which operations are heavy is measured, not chosen:** an operation is heavy when the median
of its fixed operation in `meta/measurements.json` exceeds 1 ms natively on the reference
runner (ubuntu-24.04, x86-64) or under node on that runner, which the generator writes as the
capability table's `heavy` column; the 10 % band of D6 keeps a flag from flipping on noise.
Operations whose cost the caller's data size sets (AEADs, boxes, secretstream, stream ciphers,
MACs) are never heavy: a program that seals a gigabyte chunks it, or posts it itself, and a
per-call thread hop would tax every small message. Everything else stays synchronous only.
From the measurements of the studies and upstream's own benchmarks the expected heavy set is:
RSA key generation (every size) and RSA private operations at 3072 bits and above; SLH-DSA key
generation, signing and verification; BLS12-381 verification, aggregate verification and fast
aggregate verification (pairings), and aggregation over many signatures; ML-DSA key generation
and signing and ML-KEM key generation where node exceeds 1 ms. Ed25519, X25519, secp256k1,
P-curve ECDSA and ECDH, X-Wing and the group operations are expected cheap. Each increment's
measurement task confirms or corrects its rows. Password-based key protection (Argon2, scrypt,
PBKDF2) is webcpp::hash's and has its variants there.

- **Why measured:** the owner's criterion ("above about 1 ms on the reference machine");
  a hand-written list would drift with every pin move and every backend.
- **Alternatives:** `async_` for every operation on a thread (a thread hop costs more than an
  Ed25519 signature); threads on WASI (wasip2 has none, wasip3's cooperative threads do not run
  in parallel); a thread pool owned by the library (global state, and the program's executor is
  the program's).

### D24. Primitives only: protocols go to domain libraries

The owner's rule of 2026-11-14: webcpp::crypto and webcpp::hash hold primitives. Whatever a
protocol specification defines (BIPs, EIPs, SLIPs, NIPs, and their like for other systems) goes
to future domain libraries built on crypto and hash: webcpp::bitcoin, webcpp::ethereum,
webcpp::nostr and others. The test: an operation belongs here when it is a cryptographic
algorithm with its own specification (an RFC, a NIST standard, an IETF or CFRG draft, a
paper) or the public interface of a backend's library that is such an algorithm; it belongs to
a domain library when it fixes a format, a derivation path, a tag, a domain separation string
or a message layout for one protocol.

| Primitive, here | Protocol, in a domain library |
| --- | --- |
| secp256k1 ECDSA, BIP340 Schnorr, ECDH, recovery, raw ECDH, tweaks by any 32-byte value, x-only tweaks, ElligatorSwift, MuSig2, silent payments: libsecp256k1's own modules and APIs | BIP32 and BIP39 derivation, BIP341's `TapTweak` tagged hash and output keys, sighashes, PSBT, transactions, addresses, `from_lax_der`'s historical-transaction parsing |
| Ed25519, X25519, AEADs, boxes, KEMs | NIP-04 and NIP-44 message formats, keystores (EIP-2335, Web3 Secret Storage) |
| BLS signatures, aggregation, hash-to-curve with a caller's tag, KZG commitments over a caller's trusted setup | EIP-2333 key trees, EIP-4844 versioned hashes, consensus signing domains |
| HMAC, HKDF, Keccak-256 (webcpp::hash) | EIP-712 typed-data hashing, EIP-191 messages, EIP-55 checksums |
| Shamir secret sharing over GF(2^8) | SLIP-39 mnemonics and share formats |

libsecp256k1's modules stay because they are that library's own interfaces, specified as
algorithms (MuSig2 and silent payments are signature and key-derivation schemes with their own
proofs), and wrapping them adds no protocol layout of webcpp's. Each backend's C API stays
reachable, so a domain library may call what this library does not expose (blst's EIP-2333
functions, for instance).

**Version 0.1 changes accordingly** (a change of the genesis, not of this one): Task 27b's
`basic_keypair::tweak_taproot` and `taproot_output_key` become a generic x-only tweak of a key
pair and of an x-only key by any 32-byte tweak (libsecp256k1's
`secp256k1_keypair_xonly_tweak_add` and `secp256k1_xonly_pubkey_tweak_add`), and
`from_lax_der` and the `bitcoin_sign` example leave crypto for webcpp::bitcoin. Increment 2 of
this change MUST NOT bring any of them back.

- **Why:** a primitive is reviewed once against its specification and serves every protocol;
  a protocol's rules change with its community's process and belong to the library that follows
  that community.
- **Alternatives:** protocol helpers in crypto "because they are small" (the drift the owner
  stopped); a single `webcpp::web3` library (the protocols do not share their rules).

### D25. KZG polynomial commitments over c-kzg-4844 (increment 9)

`backends::ckzg` over c-kzg-4844 v2.1.8 (`e125905e5e01186e6ccb7a0ced4845bf7eddbcfe`,
Apache-2.0), the submodule `third_party/libs/c-kzg-4844`, built file by file (`src/` without
its bindings) against this change's blst, never c-kzg's own blst submodule (one copy per
program, chapter 5 rule 10). c-kzg 2.1.8 pins blst v0.3.16 (`e7f90de`) and webcpp pins v0.3.17:
the increment runs c-kzg's own tests on v0.3.17 first, and if they fail, the blst pin moves to
the one c-kzg tests with, as a change of its own. The interface, `kzg::settings` and free
functions: `settings::make` from a trusted setup the caller gives (G1 monomial and Lagrange
points, G2 points; the Ethereum ceremony's file is test data, never embedded),
`blob_to_commitment`, `compute_proof`, `verify_proof`, `compute_blob_proof`,
`verify_blob_proof`, `verify_blob_proof_batch`, and the cell functions of c-kzg 2.x
(`compute_cells_and_proofs`, `recover_cells_and_proofs`, `verify_cell_proof_batch`). The blob
and cell sizes are c-kzg's compile-time constants. Versioned hashes and the blob transaction
format are webcpp::ethereum's (D24). c-kzg allocates the settings' tables with `malloc`, a
departure recorded as OpenSSL's and Botan's are; commitments and proofs over a blob are heavy
(D23).

### D26. The BN254 pairing over mcl (increment 10)

`backends::mcl` over mcl v4.43 (`50e4c6f2826de6b041f857caf9f253cc736ea926`, BSD-3-Clause), the
submodule `third_party/libs/mcl`, for pairing-based verification: `bn254::g1`, `bn254::g2`
(decoding with subgroup checks, addition, scalar multiplication on public data, the
serialisations mcl documents) and `bn254::pairing_check` (a product of pairings equal to one),
which zk-SNARK verifiers and the EVM's precompiles use. mcl is built without its JIT
(`MCL_DONT_USE_XBYAK`: a library must not write executable memory at run time), file by file
from its C++ sources; the increment opens with a spike that measures mcl's portable
configurations per target (generic C++, its LLVM-generated code where upstream ships it) and
records the choice in the canary's configuration. mcl is not constant time on every path, so
its table cells say `constant_time: false` and its pages say it is for public data; no secret
operation is exposed on it. A pairing check with many pairs is heavy (D23). The EVM's
precompile encodings are webcpp::ethereum's (D24).

### D27. Shamir secret sharing over dsprenkels/sss (increment 11)

`backends::sss` over dsprenkels/sss (MIT) at `9f2a10b1c0d391bed485c4c7e93788ce37c360f1`
(2025-08-18, its latest commit; its only tag, v0.1.0, is older and the increment reads the
diff since it), the submodule `third_party/libs/sss`, compiling `hazmat.c` and `sss.c` and its
`tweetnacl.c`, never its `randombytes` submodule. It offers `shamir::split_key` and
`shamir::combine_key` (`sss_create_keyshares`, `sss_combine_keyshares`: a 32-byte key into `n`
shares with threshold `k`, GF(2^8) arithmetic bitsliced and constant time) and `shamir::split`
and `shamir::combine` (`sss_create_shares`, `sss_combine_shares`: a 64-byte message, encrypted
under a key that is then split, with each share authenticated).

- **Entropy:** sss draws its polynomial through a function `randombytes` and ignores its result.
  webcpp's `src/sss_randombytes.c` defines it to hand over bytes the C++ wrapper drew from
  `random_device` into a thread-local buffer just before the call, so that the wrapper reports
  `entropy_unavailable` before sss runs and nothing reaches sss but `random_device`'s bytes; the
  buffer is wiped after.
- **Guards:** `2 <= k <= n <= 255` before any call (`k = 1` would make sss declare a
  zero-length array, undefined behaviour). sss uses C99 variable-length arrays, which MSVC does
  not compile: the increment's spike decides whether MSVC builds it with clang-cl or the table
  marks it absent there.
- **Why sss:** it is the one maintained C library whose arithmetic is constant time (bitsliced
  GF(2^8), no tables indexed by secrets), small, MIT, with a security review in its history.
  libgfshare uses log and exponent tables indexed by secret bytes; trezor-crypto's shamir lives
  inside a firmware repository without releases and serves SLIP-39; OpenSSL, Botan and
  libsodium have no secret sharing; Go's and Rust's implementations are not C. SLIP-39's
  mnemonics and share formats are webcpp::bitcoin's (D24).

### D28. HPKE (RFC 9180) over OpenSSL and Botan (increment 12)

HPKE is a primitive with its own RFC, offered where a library implements it, never composed by
webcpp from libsodium's parts (the rule against webcpp's own cryptography). OpenSSL 3.5.9's
`OSSL_HPKE_*` (in OpenSSL since 3.2) serves native, wasip2 and wasip3; Botan 3.14.1's module
`hpke` (`src/lib/pubkey/hpke/`, present at the 3.14.1 tag) serves Emscripten, and WASI where
increment 7's spike builds Botan. So every target has HPKE, and the WebAssembly backend is
Botan. libsodium has no HPKE; BoringSSL has one but is not pinned by webcpp and would be a
second copy of a libcrypto beside OpenSSL. The interface, `hpke<Kem, Kdf, Aead, Backend>` over
RFC 9180's suites both backends have (DHKEM(X25519, HKDF-SHA256), DHKEM(P-256, HKDF-SHA256),
HKDF-SHA256 and SHA512, AES-128-GCM, AES-256-GCM, ChaCha20-Poly1305): `setup_base_sender`,
`setup_base_receiver`, the PSK, auth and auth-PSK modes, a context with `seal`, `open` and
`export`, its sequence number guarded against overflow, and the single-shot `seal` and `open`.
RFC 9180's test vectors, and agreement between OpenSSL and Botan natively.

### Capability rows of increments 9 to 12

| Algorithm | Backends | Targets |
| --- | --- | --- |
| `kzg` (commitments, proofs, cells) | `ckzg` | all four (blst underneath) |
| `bn254` (decode, add, multiply, pairing check; public data) | `mcl` | all four, as the spike configures each |
| `shamir` (key shares, message shares) | `sss` | all four; MSVC as the spike finds |
| `hpke` (RFC 9180 suites above) | `openssl`, `botan` | OpenSSL native, wasip2, wasip3; Botan native, Emscripten, WASI as increment 7 finds |

## Risks / Trade-offs

- [The capability table grows to hundreds of cells] -> it is generated into the code and the
  documentation and checked by a generated test; a cell cannot be claimed without a
  specialisation that compiles and a vector that passes.
- [A profile changes a program's backends when a pin moves] -> rankings change only through a
  committed regeneration in the pin move's change, with the 10 % hysteresis for throughput;
  `portable` and named-backend profiles never move.
- [A misspelt profile or override macro is silently ignored] -> the profile page lists every
  macro, and `crypto::profile_name` shows the resolved profile (D5).
- [Two translation units with different profiles] -> MSVC's `detect_mismatch` per profile and
  override; elsewhere the documentation says to set the profile in the project's requirements.
- [Ed25519 verification differs between libsodium, OpenSSL, Botan and browsers on adversarial
  inputs] -> recorded per backend from Wycheproof's EdDSA file, documented; the agreement test
  states the expected differences.
- [libsodium's portable code is slower than its assembly on x86-64] -> measured in increment 1,
  recorded; a later change may add an assembly configuration.
- [libsodium's generator and `random_device` call the system through two entry points on
  Windows] -> both are the system's generator with nothing in user space; the entropy page
  says so.
- [Botan's WASI build may not exist] -> the spike decides the cells; native and Emscripten do
  not wait on it.
- [blst's assembly on MSVC and WebAssembly is not used] -> blst's C is constant time by its
  authors' design; the measurement table shows the cost.
- [WebCrypto keys and promises outlive a C++ object] -> the handle table frees a key when its
  last C++ owner is destroyed; an operation in flight holds its key's handle until it completes.
- [A browser lacks an algorithm the table marks run-time-available] -> `not_supported`, tested
  in node with the algorithm removed from `crypto.subtle`.
- [hash-to-curve on ristretto255 waits for libsodium] -> D17; programs that need it today use
  blst or Botan's suites.
- [c-kzg-4844 tests with blst v0.3.16 and webcpp pins v0.3.17] -> c-kzg's own tests run on
  webcpp's blst first; a failure moves the blst pin as its own change (D25).
- [sss is quiet upstream] -> pinned at its latest commit with the diff from v0.1.0 read, its
  own tests and the constant-time lane run on webcpp's build; a fork is never taken without the
  owner (D27).
- [mcl is not constant time everywhere] -> nothing secret is exposed on it (D26).
- [Six streams at once] -> the generated files are regenerated, never merged by hand; each
  stream rebases when another merges.
- [A heavy flag changes with a pin move, adding or removing an `async_` variant] -> the flag
  moves only past the 10 % band, in the pin move's own change, whose changelog names it; a
  program that used the variant keeps compiling only if the operation stays heavy, which the
  changelog says.
- [The new code's constant time] -> the harness runs on every increment's secret paths, with
  the planted leak that must fail.

## Migration Plan

Nothing to migrate until increment 2: version 0.1's API is unchanged by increment 1 apart from
additions (`default_backend` remains the AES-GCM family's default). Increment 2 changes
`secp256k1::basic_keypair::ecdh` to return `secret<32>`: callers that copied the array read it
through the span it converts to, and the changelog says so. Programs that defined
`WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` or `_OPENSSL` keep their meaning.

To start: `reference/STATUS.md`, "How to resume". Rollback: the change is a proposal;
withdrawing it deletes `openspec/changes/foundation/`.

## Open Questions

None. The owner decided the scope, the backends, the profiles, the browser's family, the
documentation and the order on 2026-11-14; the upstream facts this change waits on (Botan's WASI build,
libsodium's next release for hash-to-curve, c-kzg's tests on webcpp's blst, sss on MSVC) have
decided rules for either outcome (D15, D17, D25, D27).
