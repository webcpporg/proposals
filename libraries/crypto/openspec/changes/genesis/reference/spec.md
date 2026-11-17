---
title: "webcpp::crypto: authenticated encryption, randomness and secp256k1 over C backends"
tags: [webcpp, crypto, bearssl, openssl, secp256k1, ctaes, botan, b2, wasm, emscripten, spec]
status: active
created: 2026-10-10
---

# webcpp::crypto: specification (approved by the owner on 2026-10-11)

Rewritten on 2026-10-11 from the owner's decisions of 2026-10-10 and 2026-10-11. It replaces
the draft of 2026-10-10, whose AES-GCM was webcpp's own port of BearSSL: the owner refused the
port (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries themselves"). Revised
the same day to its independent review (CR-REVIEW) and the controller's rulings on it
(LEDGER:22-29); section 10 lists every departure from the owner's decisions, two of them for
the owner. Revised again the same day for the owner's answer to those two (DESIGN, 2026-10-11
"webcpp::crypto's two departures accepted"; 2026-10-11 "OpenSSL's memory functions are a
customization of webcpp::crypto's config"): OpenSSL's memory functions become an option of the
config (section 3.7), revised once more to that option's independent review (crypto-r3-review)
and the controller's rulings on it: a resource that is never destroyed, OpenSSL's defaults
recognised on every link, `make`'s errors when the resource fails, and the owner's decisions
cited by date and title. Brought in line with the library's code on 2026-11-15: OpenSSL is the
superproject's, built from source, since the provider amendment of 2026-11-03 (sections 3.3
and 4.4), and the names are grouped in namespaces since the owner's reshape of 2026-11-15
(section 2.2).

Sources, cited by short name below:

| Short name | File |
| --- | --- |
| DESIGN | `~/.buzz/PLANS/WEBCPP_DESIGN.md`, the owner's decisions. An entry is cited by its date and its title, the bold words after "owner:", as in DESIGN, 2026-10-11 "the BearSSL mirror", never by line: the file grows, and entries are inserted before others |
| AGENTS | `~/dev/webcpp/AGENTS.md` (chapter 6 at commit 66c259c) |
| C-LIBS | `~/.buzz/RESEARCH/C_LIBRARIES_IN_WEBCPP.md` |
| AES-GCM | `~/.buzz/RESEARCH/AES_GCM_WITHOUT_WEBCRYPTO.md` |
| BOOST | `~/.buzz/RESEARCH/BOOST_CRYPTO_INTERFACES.md` |
| REVIEW | `~/dev/webcpp/.superpowers/sdd/plan-reviews/webrtc-crypto-review.md` |
| PROBES | `~/dev/webcpp/.superpowers/sdd/plan-reviews/c-backends/` (the `jam/` prototype, `canary/`, `msvc/`, their logs) |
| PROBE-1011 | `/tmp/crypto-spec-probe/`, run on 2026-10-11 for this spec (macOS arm64, Apple clang 21, OpenSSL 3.6.4); sources only, in `/tmp`, so not durable (see CR-PROBES for what was reproduced) |
| CR-REVIEW | `~/dev/webcpp/.superpowers/sdd/plan-reviews/crypto-review.md`, the independent review of this spec (findings I1 to I8, M1 to M14) |
| CR-PROBES | `~/dev/webcpp/.superpowers/sdd/plan-reviews/crypto-review/`: `ivlen.out` and `ossl_open.out` (OpenSSL 3.6.4), `star-messages.log` (the six starred messages of section 4.3, reproduced), `ctaes-pin.log` (the ctaes pin, `gh api`), `secp/` (Wycheproof's secp256k1 files through libsecp256k1 v0.8.0), `wp_*.json` (Wycheproof files at the pin) |
| LEDGER | `~/dev/webcpp/.superpowers/sdd/plan-reviews/README.ledger`, the controller's rulings (`Ruling: I1` to `I8`, lines 22-29) |
| OPENSSL-SRC | OpenSSL's sources at tag `openssl-3.6.4` (commit `d3c1b1169b35`), the version PROBE-1011 and CR-PROBES measured, and `crypto/mem.c` at tag `openssl-3.0.13`, the CI's Linux version; read on 2026-10-11 from `github.com/openssl/openssl` (cited as `crypto/mem.c:61-74`, `3.0.13 crypto/mem.c:177-196`) |
| BOTAN-SRC | Botan's sources at tag 3.13.0 (commit `2a81eef5`, the version REVIEW read), read on 2026-10-11 from `github.com/randombit/botan` (cited as `src/lib/utils/allocator.cpp:21-48`) |
| PROBE-R3 | `~/.claude/jobs/fcf3a902/tmp/crypto-r3/probe/`, run on 2026-10-11 for section 3.7 (macOS arm64, Apple clang 21, Homebrew OpenSSL 3.6.4 shared): `probe.cpp` (OpenSSL's allocations through a `std::pmr` resource), `interpose.c` (counts the C library's `malloc`, `calloc`, `realloc`, `posix_memalign`, `aligned_alloc` and `free`), `build-and-run.sh`, `probe.log`, `linkerr.cpp` and `linkerr.log`, `errmark.cpp` and `errmark.log` (OpenSSL's error marks on a new thread); a job's scratch directory, so not durable, as PROBE-1011 is not |
| crypto-r3-review | `~/dev/webcpp/.superpowers/sdd/plan-reviews/crypto-r3-review.md`, the independent review of revision 3 (findings C1, I1 to I5, M1 to M12), and `crypto-r3-review/`: `probe-r3-rerun.log` (PROBE-R3 reproduced) and `exit_and_failure.cpp`, `exit_and_failure.log` (exit order and allocation failure, macOS, OpenSSL 3.6.4 shared and static) |

## 1. Purpose and scope

### 1.1 What the library is

webcpp::crypto gives a C++ program the cryptography that Boost does not provide, on every
target webcpp has: native (Linux, macOS, Windows with MSVC), wasip2, wasip3 and Emscripten.

- **Authenticated encryption (AEAD).** AES-GCM first, with keys of 16, 24 and 32 bytes.
  ChaCha20-Poly1305 where a backend exports it.
- **Randomness.** A `random_device` that reads the operating system's or the browser's
  generator, and nothing else (section 6).
- **secp256k1.** BIP340 Schnorr signatures, ECDSA signatures, their verification, and ECDH,
  through the libsecp256k1 backend.
- **A constant-time `equal`** for tags and MACs, Boost.Hash2's included.

The algorithms are not webcpp's. Each runs on a backend, a C or C++ library that webcpp wraps
behind an interface shaped as Boost.Hash2's (section 2). A backend is named after its library:
`backends::bearssl`, `backends::openssl`, `backends::ctaes`, `backends::secp256k1`,
`backends::botan` (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries
themselves"; 2026-10-10 "webcpp::crypto's interface and backends").

Every call is synchronous. The first decision made the library asynchronous because WebCrypto
is promise-based (DESIGN, 2026-10-10 "webcpp::webrtc and webcpp::crypto"). Amendment C1 removed
WebCrypto (DESIGN, 2026-10-10 "webcpp::crypto, amendment C1"), and with C code on every target
nothing waits. This is a consequence of the owner's decisions, recorded as such (REVIEW, X-I6).

### 1.2 What stays with Boost.Hash2

Hashing and HMAC are Boost.Hash2's: SHA-1, SHA-2, SHA-3, RIPEMD, BLAKE2, and
`boost::hash2::hmac<H>` (amendment C1, DESIGN, 2026-10-10 "webcpp::crypto, amendment C1";
BOOST:10-28). webcpp::crypto offers no second interface for them. HKDF, when a library needs
it, is written on Hash2's `hmac`. What webcpp::crypto adds for Hash2's users is `equal`:
Hash2's `digest<N>::operator==` is `memcmp`, not constant time
(`hash2/detail/memcmp.hpp:21-23`, BOOST:28), so a MAC is never checked with it.

### 1.3 The owner's decisions this spec applies

| Date       | Decision                                                                                                                                                                                                                                                          | Where                      |
| ---------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------- |
| 2026-10-10 | Every library builds with exceptions on and off; `WEBCPP_<NAME>_NO_EXCEPTIONS`                                                                                                                                                                                    | DESIGN, 2026-10-10 "exceptions"                 |
| 2026-10-10 | Every variant tested; CI in three levels                                                                                                                                                                                                                          | DESIGN, 2026-10-10 "every variant tested"; 2026-10-10 "CI in three levels"           |
| 2026-10-10 | Allocators: run-time objects route allocation through the user's allocator                                                                                                                                                                                        | DESIGN, 2026-10-10 "allocators"; 2026-10-10 "allocators, scope"             |
| 2026-10-10 | Amendment C1: hashing and HMAC leave webcpp::crypto for Boost.Hash2; AES-GCM without WebCrypto                                                                                                                                                                    | DESIGN, 2026-10-10 "webcpp::crypto, amendment C1"                 |
| 2026-10-10 | What a library builds on, refined                                                                                                                                                                                                                                 | DESIGN, 2026-10-10 "what a library builds on, refined", AGENTS:602-613 |
| 2026-10-10 | Backends are the C libraries themselves, no port of BearSSL; named after the library; each exports what it has, and the documentation says what; the default is the smallest binary on the web and the fastest on servers; OpenSSL can be linked shared or static | DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries themselves"                 |
| 2026-10-10 | The interface shaped as Boost.Hash2's, the backend a template parameter whose default the config chooses; ctaes and BearSSL are backends with different functions; Botan becomes a backend                                                                        | DESIGN, 2026-10-10 "webcpp::crypto's interface and backends"                 |
| 2026-10-11 | C libraries built from their own sources, file by file, by webcpp's Jamfile; the program names `/third_party//<backend>` in its own Jamfile; the errors say exactly what to add                                                                                   | DESIGN, 2026-10-11 "C libraries built from their own sources, and the backend named in the user's Jamfile"                 |
| 2026-10-11 | BearSSL from the unmodified mirror `webcpporg/bearssl`, created from upstream master 7bea48e5                                                                                                                                                                     | DESIGN, 2026-10-11 "the BearSSL mirror"                 |
| 2026-10-11 | Entropy: always the safest path, in readable code                                                                                                                                                                                                                 | DESIGN, 2026-10-11 "entropy, safest and readable"                 |
| 2026-10-11 | An external library a program names and the machine lacks stops the build                                                                                                                                                                                         | DESIGN, 2026-10-11 "an external library a program names and the machine lacks stops the build", AGENTS:726-732 |
| 2026-10-11 | Bringing a C library into webcpp: twelve rules, three layers that stop a wrong build                                                                                                                                                                              | AGENTS:691-748             |
| 2026-10-11 | The two departures of section 10 accepted: OpenSSL's and Botan's `make` allocate outside the user's allocator; their shared or static link is a b2 feature                                                                                                        | DESIGN, 2026-10-11 "webcpp::crypto's two departures accepted"                 |
| 2026-10-11 | OpenSSL's memory functions are an option of webcpp::crypto's config; without it the departure stands; OpenSSL and Botan stay backends, both recommended where they apply                                                                                          | DESIGN, 2026-10-11 "OpenSSL's memory functions are a customization of webcpp::crypto's config"                 |
| 2026-11-03 | The provider amendment: OpenSSL 3.5.9 built from source by the superproject, static, on native, wasip2 and wasip3, never the machine's; a program names `/webcpp/crypto//openssl`                                                                                 | CODING-GUIDELINES.md chapter 5 rule 7; `design.md` |
| 2026-11-15 | webcpp::crypto holds primitives only, its names grouped by kind in namespaces that its headers mirror                                                                                                                                                             | `design.md`; `reference/plan.md` |

The decision of 2026-10-10 that a C library comes in through one header included once, as
Boost.JSON's `src.hpp` (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries
themselves"), is superseded by DESIGN, 2026-10-11 "C libraries built from their own sources,
and the backend named in the user's Jamfile": neither BearSSL nor libsecp256k1 compiles as C++
(C-LIBS:122-129, :232-236), and the library is built from its own files instead.

## 2. The interface

### 2.1 The shape

The interface follows Boost.Hash2 (BOOST:12-28, DESIGN, 2026-10-10 "webcpp::crypto's interface
and backends"):

- **Algorithm types with a small uniform set of members.** Every type states its sizes as
  `static constexpr` members (`key_size`, `block_size`, `iv_size`, `tag_size`) and names its
  output `result_type`, as Hash2's algorithms do (`hash2/sha2.hpp:508-580`).
- **Modes are templates over a cipher,** as `hmac<H>` is a template over a hash
  (`hash2/hmac.hpp:38-140`): `gcm<aes256<>>`.
- **Tags are Hash2's `digest<16>`** (`hash2/digest.hpp`), compared with `equal`.
- **The backend is a template parameter** whose default `config.hpp` chooses (section 5):
  `aes256<>` is `aes256<default_backend>`, and `aes256<backends::openssl>` names one.
- **Where Hash2 cannot be followed, the reason is an error.** Hash2 constructs an algorithm
  from a key and cannot fail. Here a key can be of the wrong size and a backend can fail to
  allocate, so a keyed object is made by `static make(...)`, which returns
  `boost::system::result<T>` (AGENTS:636-638).

Every operation that can fail returns `boost::system::result<T>`, or `result<void>`, whose
`operator bool` is true on success, with exceptions on and off. No function of the library
throws, except `random_device::operator()`, which exists only with exceptions, as
`std::random_device`'s does (section 2.7). Botan's backend catches every exception of
Botan's at its boundary (section 3.6).

Data and keys are `std::span<std::uint8_t const>` and `std::span<std::uint8_t>`; sizes are
`std::size_t`, and a limit wider than `size_t` on wasm32 is `std::uint64_t` (AGENTS:630-635).

### 2.2 The headers

The names are grouped by kind, and the headers mirror the namespaces as
`<webcpp/crypto/<group>/<name>.hpp>` (the owner's decision of 2026-11-15): `webcpp::crypto`
holds `error`, `random_device`, `equal`, the memory option and the configuration;
`webcpp::crypto::backends` the backends; `webcpp::crypto::ciphers` the ciphers and their modes
(`aes128`, `aes192`, `aes256`, `gcm`, `chacha20_poly1305`, `aes128_gcm`, `aes256_gcm`);
`webcpp::crypto::curves::secp256k1` the curve. webcpp::crypto holds primitives only: a
protocol's rules (a BIP's tweak, lax DER, transactions, sighashes, key derivation) belong to a
future domain library. Nothing is released, so no alias or forwarding header keeps an old name.
Where this specification writes `aes256`, `gcm` or `keypair` alone, it means the name in its
namespace.

| Header | What | Needs a backend linked |
| --- | --- | --- |
| `<webcpp/crypto/config.hpp>` | the target, the `HAS` defines, the default backend, `WEBCPP_CRYPTO_NO_EXCEPTIONS`, the option `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` (section 3.7) | no |
| `<webcpp/crypto/error.hpp>` | `error`, its category | no |
| `<webcpp/crypto/equal.hpp>` | `equal` | no |
| `<webcpp/crypto/random_device.hpp>` | `random_device` | no |
| `<webcpp/crypto/ciphers/aes.hpp>` | `ciphers::aes128`, `aes192`, `aes256` | the one it is used with |
| `<webcpp/crypto/ciphers/gcm.hpp>` | `ciphers::gcm<Cipher>` | the cipher's |
| `<webcpp/crypto/ciphers/chacha20_poly1305.hpp>` | `ciphers::chacha20_poly1305<Backend>` | the one it is used with |
| `<webcpp/crypto/ciphers/aes256_gcm.hpp>` | `ciphers::aes128_gcm`, `aes256_gcm`, and the default backend's header | the default |
| `<webcpp/crypto/backends/<name>.hpp>` | one backend: its tag type, its compile-time check, its canary | that one |
| `<webcpp/crypto/curves/secp256k1.hpp>` | `curves::secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`, `to_der`, `from_der`, `normalize_ecdsa`, `tweak_xonly` | libsecp256k1 |
| `<webcpp/crypto.hpp>` | everything that needs no backend, and the header of each backend whose `HAS` define is set | no |

A program that only uses `random_device` and `equal` names no backend library. A program that
only signs names `/third_party//secp256k1` and no encryption backend (C-LIBS:549-551).

**`<windows.h>` in public headers** (crypto-r3-review N3). Two headers reach Windows' API on
`_WIN32`: `random_device.hpp`, for `BCryptGenRandom` (section 6), and `backends/openssl.hpp`
under the memory option, for `GetModuleHandleExW` and `GetProcAddress` (section 3.7).
`<windows.h>` defines `min` and `max` as function-like macros unless `NOMINMAX` is defined,
which break any `max()` declared or called after it, `random_device`'s and
`std::numeric_limits<T>::max()` included, in webcpp's headers and in the user's, whichever
comes first; and without `WIN32_LEAN_AND_MEAN` it drags in most of the Windows SDK, `winsock.h`
included, which then clashes with a later `winsock2.h` (Boost.Asio's). So:

- both include it through one header, `<webcpp/crypto/detail/windows.hpp>`, which defines
  `NOMINMAX` and `WIN32_LEAN_AND_MEAN` only when they are not already defined, includes
  `<windows.h>` (then `<bcrypt.h>` where needed), and undefines again only the macros it
  defined itself, with `lint-world:` and its reason (AGENTS:641-649);
- webcpp's headers write `(max)()`, `(min)()` and `(std::numeric_limits<T>::max)()`, so that
  they compile whatever a user's `<windows.h>` included first defined;
- the documentation says that `<windows.h>`'s include guard makes a later `#include
  <windows.h>` of the user's add nothing: a program that needs the macros `min` and `max`, or
  the parts `WIN32_LEAN_AND_MEAN` leaves out, includes `<windows.h>` itself before any header
  of webcpp::crypto;
- an MSVC test includes every public header after a bare `<windows.h>` (no `NOMINMAX`, no
  `WIN32_LEAN_AND_MEAN`), and another before it, and compiles both with `max` used as a
  function after them (section 8.6).

### 2.3 Block ciphers

```cpp
namespace webcpp::crypto::ciphers {

// aes128 and aes192 are the same, with key_size 16 and 24.
template <class Backend = default_backend>
class aes256 {
public:
    using backend_type = Backend;
    static constexpr std::size_t key_size = 32;
    static constexpr std::size_t block_size = 16;

    // Declared for every backend; defined only where Backend exports the block cipher
    // (table of section 3.1), else a static_assert names the backends that do.
    static boost::system::result<aes256> make(std::span<std::uint8_t const> key) noexcept;

    aes256(aes256&&) noexcept;             // the moved-from object is wiped and empty
    aes256& operator=(aes256&&) noexcept;
    aes256(aes256 const&) = delete;
    aes256& operator=(aes256 const&) = delete;
    ~aes256();                             // wipes the key schedule

    // In place, on whole blocks: blocks.size() is a multiple of block_size.
    boost::system::result<void> encrypt(std::span<std::uint8_t> blocks) noexcept;
    boost::system::result<void> decrypt(std::span<std::uint8_t> blocks) noexcept;
};

}
```

- `aes256<B>` is a complete type for every backend, so that it can name GCM's cipher:
  `gcm<aes256<backends::bearssl>>`. Its `make`, `encrypt` and `decrypt` exist only where the
  backend exports the AES block cipher: ctaes, OpenSSL and Botan. BearSSL exports AES only
  inside its modes: its public AES classes are CBC, CTR and CTR+CBC, with no single-block
  class (`inc/bearssl_block.h:393`, `:449`, `:506`, `:573`).
- **`aes256<>` on WebAssembly.** `aes256<>` is `aes256<default_backend>`, and the default
  backend is the AEADs' (section 5): BearSSL on WebAssembly, which has no block cipher. So a
  program that wants the block cipher names its backend, `aes256<backends::ctaes>`, and
  includes that backend's header, `<webcpp/crypto/backends/ctaes.hpp>` (section 7.5). The
  block cipher gets no default of its own: `aes256<>` must stay the cipher of
  `aes256_gcm = gcm<aes256<>>`, which a second default would break (CR-REVIEW M3). The
  `static_assert` of `make` on a backend without the block cipher says (new text; its test
  fixes it, section 8.6):

      webcpp::crypto: ciphers::aes256<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in ciphers::aes256<backends::ctaes>, ciphers::aes256<backends::openssl> or ciphers::aes256<backends::botan>, and add its library to the program's Jamfile.

- Guards: `make` gives `invalid_key_size` for a key of another size than `key_size`, then, on
  OpenSSL with the memory option, `memory_functions_not_installed` (section 3.7);
  `encrypt` and `decrypt` give `invalid_state` on an object moved from (section 2.4), then
  `invalid_input_size` when `blocks` is not a multiple of 16.
- A raw block cipher is a building block. The documentation says that it is not encryption
  by itself, and points to `gcm`.

### 2.4 AES-GCM

```cpp
namespace webcpp::crypto::ciphers {

template <class Cipher>
class gcm {
public:
    using cipher_type = Cipher;
    using backend_type = typename Cipher::backend_type;
    using result_type = boost::hash2::digest<16>;  // the tag
    static constexpr std::size_t key_size = Cipher::key_size;
    static constexpr std::size_t tag_size = 16;
    static constexpr std::size_t iv_size = 12;      // recommended
    static constexpr std::size_t max_iv_size = 128; // IVs of 1 to 128 bytes are accepted
    static constexpr std::uint64_t max_message_size = (std::uint64_t{1} << 36) - 32;

    static boost::system::result<gcm> make(std::span<std::uint8_t const> key) noexcept;

    gcm(gcm&&) noexcept;                   // abandons a message in progress; the moved-from
    gcm& operator=(gcm&&) noexcept;        // object is wiped and empty
    gcm(gcm const&) = delete;
    gcm& operator=(gcm const&) = delete;
    ~gcm();                                // wipes the key and the state

    // One shot, in place: data holds the plaintext before seal and the ciphertext after it.
    boost::system::result<result_type> seal(std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data) noexcept;
    // data holds the ciphertext before and the plaintext after; zeroed when the tag does not
    // match or the backend fails, untouched when a guard refuses the call.
    boost::system::result<void> open(std::span<std::uint8_t const> iv,
                                     std::span<std::uint8_t const> additional_data,
                                     std::span<std::uint8_t> data,
                                     std::span<std::uint8_t const> tag) noexcept;

    // Incremental seal, one message at a time: begin with the IV and the whole additional
    // data, encrypt the data in pieces, then end with the tag.
    boost::system::result<void> begin_seal(std::span<std::uint8_t const> iv,
                                           std::span<std::uint8_t const> additional_data) noexcept;
    boost::system::result<void> update(std::span<std::uint8_t> data) noexcept;
    boost::system::result<result_type> result() noexcept;  // ends the message
};

using aes128_gcm = gcm<aes128<>>;
using aes256_gcm = gcm<aes256<>>;

}
```

**`gcm<aes256<backends::ctaes>>` does not compile.** `gcm<C>` requires that `C`'s backend
export GCM over its own AES. ctaes exports the block cipher alone, and webcpp builds no GCM of
its own on another library's AES: that would be the port the owner refused (DESIGN, 2026-10-10
"webcpp::crypto's backends are the C libraries themselves"). The `static_assert` says (new
text; its test fixes it, section 8.6):

    webcpp::crypto: ciphers::gcm<ciphers::aes256<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: ciphers::gcm<ciphers::aes256<backends::bearssl>>, ciphers::gcm<ciphers::aes256<backends::openssl>> or ciphers::gcm<ciphers::aes256<backends::botan>>.

**Guards.** Each function checks every condition first, in this order, and returns before it
reads a secret byte (AGENTS:682-683):

| Condition                                                                                                                                                                                                                                                                                      | Error                                    |
| ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| `make`: a key of another size than `key_size`                                                                                                                                                                                                                                                  | `invalid_key_size`                       |
| `make`, OpenSSL's backend in a program built with `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`: OpenSSL's memory functions are not webcpp's adapter (section 3.7)                                                                                                                                   | `memory_functions_not_installed`         |
| any other function: the object was moved from                                                                                                                                                                                                                                                  | `invalid_state`                          |
| `begin_seal` while a message is in progress; `update` or `result` with no message begun                                                                                                                                                                                                        | `invalid_state`                          |
| an empty IV, or one longer than `max_iv_size`, 128 bytes                                                                                                                                                                                                                                       | `invalid_iv_size`                        |
| `open`: a tag of another size than 16                                                                                                                                                                                                                                                          | `invalid_tag_size`                       |
| a message longer than `max_message_size`, 2^36 - 32 bytes, in one call or summed over `update`s (NIST SP 800-38D section 5.2.1.1, 2^39 - 256 bits); reachable natively, where `size_t` has 64 bits. Additional data longer than 2^61 - 1 bytes (2^64 - 1 bits), the same section's other limit | `message_too_long`                       |
| `open`: the tag does not match                                                                                                                                                                                                                                                                 | `authentication_failed`                  |
| the backend fails where it should not (an OpenSSL `EVP_*` call returning 0, an exception of Botan's other than those mapped in section 3.6)                                                                                                                                                    | `backend_failure`                        |
| `make`: the backend cannot allocate its context (OpenSSL's `EVP_CIPHER_CTX_new`, Botan's `std::bad_alloc`; OpenSSL's other failures in `make` are `backend_failure`, section 3.3) | `boost::system::errc::not_enough_memory` |

**The incremental seal, completely.** `begin_seal` begins a message; `update` encrypts the
next piece in place; `result` ends it with its tag. After `result` no message is in progress,
so a second `result` gives `invalid_state`. Any error from `begin_seal`, `update` or `result`
ends the message, which is abandoned: its state is wiped, the pieces already encrypted have
no tag and are not to be sent, and the object is ready for a new `begin_seal`. A move
abandons a message in progress, as above. Each case has a test (section 8.1).

**A moved-from object** is empty: the move wipes the source's key and state and leaves it
holding nothing (a null context for OpenSSL and Botan, a flag cleared for BearSSL, ctaes and
libsecp256k1's key pair, whose state lives inside the object). Every function of it but assignment and destruction checks
that first and returns `invalid_state` (LEDGER:24). Without that guard a moved-from BearSSL
`gcm`, whose vtable survives the move, would seal under an all-zero key schedule and return a
valid-looking tag (CR-REVIEW I3). The same holds for `aes128/192/256` (section 2.3),
`chacha20_poly1305` (section 2.5) and `curves::secp256k1::basic_keypair` (section 2.8); one test per
type and backend.

**Decisions on the behaviour, with their reasons.**

- **In place, the tag apart.** `data` is both input and output, so no buffer is copied and
  no overlap can be partial. The tag is separate, a `digest<16>` out of `seal` and a span
  into `open`, so that a wire format that appends it (trystero's, WebCrypto's) or sends it
  apart is a matter of the caller's spans. The tag parameter is a dynamic span, checked, so
  that both a `digest<16>` and a `std::array<std::uint8_t, 16>` pass without a cast.
- **A failed `open` zeroes `data`** (a ruling, LEDGER:27; section 10). Every backend
  decrypts and checks the tag in one pass: BearSSL runs `br_gcm_run` and then
  `br_gcm_check_tag` (`src/aead/gcm.c:278-296`), OpenSSL checks in `EVP_DecryptFinal_ex`
  after its `EVP_DecryptUpdate`, and Botan decrypts, clears and throws
  (`botan_gcm.cpp:177-193`, REVIEW sources). The backends do not all clear what they
  decrypted: Botan clears only the part of the buffer given to `finish`
  (`botan_gcm.cpp:191`), not what `process` already decrypted in the caller's span, and
  OpenSSL clears nothing (CR-REVIEW M1). So webcpp's backend code zeroes the whole of `data`
  itself, on every backend, after any failure that follows the start of decryption
  (`authentication_failed`, `backend_failure`), through the same `volatile` stores as the
  key wipe (section 2.11). No unauthenticated plaintext is left in `data`, and an in-place
  `open` that fails does not keep its ciphertext: a caller who needs it keeps a copy. A
  guard's refusal (`invalid_state`, `invalid_iv_size`, `invalid_tag_size`,
  `message_too_long`) comes before any work and leaves `data` untouched (AGENTS:682-683).
  The draft of 2026-10-10 verified first and left the ciphertext intact (README.ledger:14);
  that needed a GHASH pass of webcpp's own over the backend's AES, which is the GCM webcpp no
  longer writes.
- **Plaintext is present in `data` during an `open`.** While an in-place `open` runs, the
  decrypted bytes sit in `data` before the tag is checked, and are zeroed on failure. When
  `data` is memory another party can read while the call runs (a shared segment, a
  `SharedArrayBuffer` that JavaScript sees), the caller opens into private memory instead.
  The documentation says so (CR-REVIEW M1).
- **No incremental `open`** (LEDGER:23). An incremental decryption returns plaintext before
  the tag is known, and nothing prevents its use when the tag later fails (CR-REVIEW I2). No
  user in this spec needs streaming decryption: trystero opens whole messages. A later
  increment may add it only with the plaintext withheld until the tag verifies. The
  incremental seal stays, and what it produces is opened by the one-shot `open`.
- **The incremental seal takes the IV and the additional data at its start**
  (`begin_seal(iv, additional_data)`, LEDGER:25). Botan's public interface takes the
  associated data whole, through `set_associated_data`, before the nonce:
  `GCM_Mode::set_associated_data_n` (`botan_gcm.cpp:96-99`) calls
  `GHASH::set_associated_data`, which checks `BOTAN_STATE_CHECK(!m_nonce)` and replaces the
  stored value (`botan_ghash.cpp:178-186`); the incremental `GHASH::update_associated_data`
  (`:196-200`) is in the internal header `botan/internal/ghash.h`. Pieces of additional data
  could only be joined in a growing buffer, an allocation against section 2.11 (CR-REVIEW I4).
  BearSSL (`br_gcm_aad_inject`) and OpenSSL (`EVP_EncryptUpdate` with a null output) take it
  in one call as well.
- **IVs of 1 to 128 bytes** (LEDGER:22). BearSSL hashes an IV other than 12 bytes into the
  first counter block (`src/aead/gcm.c:87-101`), as OpenSSL (`EVP_CTRL_GCM_SET_IVLEN`) and
  Botan (`botan_gcm.cpp:110-115`) do. OpenSSL accepts no IV longer than 128 bytes: measured on
  3.6.4, 1 to 128 bytes accepted, 129, 256, 512 and 1,024 refused (CR-PROBES `ivlen.c`,
  `ivlen.out`), its GCM provider keeping the IV in a fixed buffer of `GCM_IV_MAX_SIZE`, 1,024
  bits (CR-REVIEW I1). BearSSL and Botan accept longer ones (`botan_gcm.cpp:71-74`, any length
  above 0), so without a cap of webcpp's the same call would succeed or fail by backend, which
  section 8.2's agreement rules out. Every vector of the GCM specification and of NIST CAVP
  (IVs of 8 to 1,024 bits) stays inside the cap, as do trystero's 16-byte IVs (trystero
  `protocol/signal.hpp:36-37`), for which the hashed path is the one trystero runs. The
  documentation recommends 12 (REVIEW, X-I2).
- **Keys of 24 bytes** (`aes192`) are accepted: AES-192 is part of the standard and every
  AEAD backend has it, and it lets every case of the test vectors run (section 8.1). Chrome's
  WebCrypto refuses it (`components/webcrypto/status.cc`, `ErrorAes192BitUnsupported`), so the
  documentation says that a key meant for a browser's WebCrypto is of 16 or 32 bytes.
- **Usage limit.** With random IVs, or IVs other than 96 bits, NIST SP 800-38D section 8.3
  allows at most 2^32 messages per key. trystero draws a random 16-byte IV per message. The
  documentation states the limit.
- **An IV is never reused under one key.** A repeated IV under one key exposes the XOR of
  the two plaintexts and lets an attacker recover GHASH's authentication key, and so forge
  tags for that key (NIST SP 800-38D section 8, the uniqueness requirement; A. Joux,
  "Authentication Failures in NIST version of GCM", 2006). The documentation says so beside
  the usage limit (CR-REVIEW M10).
- **One thread at a time.** A `gcm` object holds the state of a message, so its functions
  are not `const` and an object is used by one thread at a time, as a Hash2 algorithm is.
- **Not shareable.** A `gcm` holds a key, and OpenSSL's and Botan's hold pointers into their
  libraries' heaps. The documentation lists it as not shareable (AGENTS:651-667) and says why
  a key in a shared segment is a bad idea in any case.

### 2.5 ChaCha20-Poly1305

```cpp
namespace webcpp::crypto::ciphers {

template <class Backend = default_backend>
class chacha20_poly1305 {
public:
    using backend_type = Backend;
    using result_type = boost::hash2::digest<16>;
    static constexpr std::size_t key_size = 32;
    static constexpr std::size_t iv_size = 12;     // exactly, RFC 8439 section 2.8
    static constexpr std::size_t tag_size = 16;
    static constexpr std::uint64_t max_message_size = (std::uint64_t{1} << 38) - 64;

    static boost::system::result<chacha20_poly1305> make(std::span<std::uint8_t const> key) noexcept;
    // move-only, wiping, as gcm
    boost::system::result<result_type> seal(std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data) noexcept;
    boost::system::result<void> open(std::span<std::uint8_t const> iv,
                                     std::span<std::uint8_t const> additional_data,
                                     std::span<std::uint8_t> data,
                                     std::span<std::uint8_t const> tag) noexcept;
};

}
```

- One shot only. BearSSL's ChaCha20-Poly1305 is one function over the whole message,
  `br_poly1305_run` (`inc/bearssl_block.h:2513-2515`), so an incremental form would not be
  uniform across backends. It is added later if a library needs it.
- The guards and errors are GCM's, the moved-from object and the zeroing of `data` included,
  with an IV of exactly 12 bytes (`invalid_iv_size` otherwise) and RFC 8439's limit of
  2^38 - 64 bytes per message.
- **Nonces** (CR-REVIEW M10). A nonce repeated under one key gives both messages the same
  keystream and the same one-time Poly1305 key, which reveals the XOR of the plaintexts and
  allows forgeries (RFC 8439 section 4). With random 96-bit nonces the documentation gives
  GCM's limit of 2^32 messages per key: 2^32 random 96-bit values repeat with a probability of
  about 2^64 / 2^97 = 2^-33, inside the 2^-32 that NIST SP 800-38D section 8.3 sets for GCM's
  random IVs.
- BearSSL leaves the tag check to its caller ("After decryption, the caller is responsible
  for checking that the computed tag matches the expected value", `inc/bearssl_block.h:2500`).
  The backend compares with `equal` and zeroes `data` on a mismatch. That is BearSSL's
  documented contract, not a mode of webcpp's.
- Backends: BearSSL, OpenSSL (`EVP_chacha20_poly1305`), Botan. `chacha20_poly1305<backends::ctaes>`
  does not compile, with the message of section 2.4's kind.

### 2.6 Tags and `equal`

```cpp
namespace webcpp::crypto {

// Constant time in the contents: every byte is read, and no branch or index depends on one.
template <std::size_t N>
bool equal(boost::hash2::digest<N> const& a, boost::hash2::digest<N> const& b) noexcept;
// Spans of different sizes compare unequal without reading their contents: a size is public.
bool equal(std::span<std::uint8_t const> a, std::span<std::uint8_t const> b) noexcept;

}
```

- It accumulates the XOR of every pair of bytes with OR and reduces the result to one bit
  without a branch, as BearSSL's tag check does (`src/aead/gcm.c:278-291`, `EQ0`).
- `equal` compiles in the user's translation unit, where the checks of section 8.3 do not
  run, so a compiler could turn the reduction into an early exit. The accumulator is a
  `volatile` local, read once after the loop, so that every byte is read and combined before
  the result is known (CR-REVIEW M13); the deterministic check of section 8.3 runs on a test
  program that calls it, at -O2 and -Os.
- A `digest<N>` is a contiguous range of `unsigned char` (`hash2/digest.hpp:40-60`), so the
  span overload also takes a digest against a `std::array`.
- It serves Boost.Hash2's `hmac<H>::result()` as much as webcpp::crypto's tags.

### 2.7 `random_device`

```cpp
namespace webcpp::crypto {

class random_device {
public:
    // std::random_device's own type, which the standard fixes as unsigned int.
    using result_type = unsigned int;

    random_device() noexcept = default;
    random_device(random_device const&) = delete;
    random_device& operator=(random_device const&) = delete;

    // (min) and (max) in parentheses: <windows.h> may define min and max as macros (section 2.2).
    static constexpr result_type (min)() noexcept { return 0; }
    static constexpr result_type (max)() noexcept { return (std::numeric_limits<result_type>::max)(); }
    double entropy() const noexcept;  // std::numeric_limits<result_type>::digits: every bit is the system's

    // Fills out from the system's generator (section 6). On error out is zeroed.
    boost::system::result<void> fill(std::span<std::uint8_t> out) noexcept;

#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    // fill of sizeof(result_type) bytes; on error, boost::throw_exception of a
    // boost::system::system_error carrying fill's error.
    result_type operator()();
#endif
};

}
```

- Shaped as `std::random_device` (DESIGN, 2026-10-10 "webcpp::crypto's interface and
  backends"): the same `result_type`, `min`, `max`, `entropy` and `operator()`, so that it is a
  UniformRandomBitGenerator with exceptions on. Without exceptions `operator()` is not
  declared, since it has no way to report a failure, and `fill` is the interface. The
  documentation says so.
- It holds no state and needs no backend. Its errors are `entropy_unavailable` and, where
  the environment has no generator, `boost::system::errc::not_supported` (section 6).

### 2.8 secp256k1

```cpp
namespace webcpp::crypto::curves::secp256k1 {

template <class Backend = backends::secp256k1>
class basic_keypair {
public:
    using backend_type = Backend;

    // The context lives in memory, and is randomized with 32 bytes of random_device, as
    // libsecp256k1 recommends (include/secp256k1.h:849-879).
    static boost::system::result<basic_keypair> make(
        std::span<std::uint8_t const> secret,
        std::pmr::memory_resource* memory = std::pmr::get_default_resource());

    basic_keypair(basic_keypair&&) noexcept;      // the moved-from key pair is wiped and empty
    basic_keypair& operator=(basic_keypair&&) noexcept;
    basic_keypair(basic_keypair const&) = delete;
    basic_keypair& operator=(basic_keypair const&) = delete;
    ~basic_keypair();                             // wipes the key pair, gives the context back

    // A result, so that a moved-from key pair answers invalid_state.
    boost::system::result<std::array<std::uint8_t, 32>> xonly_public_key() const noexcept;  // BIP340
    boost::system::result<std::array<std::uint8_t, 33>> public_key() const noexcept;        // SEC1, compressed

    // BIP340, any message length; aux is 32 bytes of fresh randomness, the caller's.
    boost::system::result<std::array<std::uint8_t, 64>> sign_schnorr(
        std::span<std::uint8_t const> message, std::span<std::uint8_t const, 32> aux) const noexcept;
    // ECDSA over a 32-byte hash, RFC 6979 nonce, low-S, compact form.
    boost::system::result<std::array<std::uint8_t, 64>> sign_ecdsa(
        std::span<std::uint8_t const, 32> hash) const noexcept;
    // The SHA-256 of the compressed shared point, libsecp256k1's default (secp256k1_ecdh.h).
    boost::system::result<std::array<std::uint8_t, 32>> ecdh(
        std::span<std::uint8_t const> public_key) const noexcept;
    // In place by a 32-byte tweak t, the caller's: the x-only public key P becomes P + t*G
    // (secp256k1_keypair_xonly_tweak_add); a tweak refused wipes the pair.
    boost::system::result<void> tweak_xonly(std::span<std::uint8_t const, 32> tweak) noexcept;
};
using keypair = basic_keypair<>;

// With libsecp256k1's static context, which allocates nothing.
template <class Backend = backends::secp256k1>
boost::system::result<void> verify_schnorr(std::span<std::uint8_t const, 64> signature,
                                           std::span<std::uint8_t const> message,
                                           std::span<std::uint8_t const, 32> xonly_public_key) noexcept;
template <class Backend = backends::secp256k1>
boost::system::result<void> verify_ecdsa(std::span<std::uint8_t const, 64> signature,
                                         std::span<std::uint8_t const, 32> hash,
                                         std::span<std::uint8_t const> public_key) noexcept;

// ECDSA's encodings and the x-only tweak, with the static context too.
inline constexpr std::size_t max_der_size = 72;
template <class Backend = backends::secp256k1>
boost::system::result<std::size_t> to_der(std::span<std::uint8_t const, 64> compact,
                                          std::span<std::uint8_t> out) noexcept;  // strict DER
template <class Backend = backends::secp256k1>
boost::system::result<std::array<std::uint8_t, 64>> from_der(
    std::span<std::uint8_t const> der) noexcept;                      // strict DER, BIP66
template <class Backend = backends::secp256k1>
boost::system::result<std::array<std::uint8_t, 64>> normalize_ecdsa(
    std::span<std::uint8_t const, 64> compact) noexcept;              // the low-S form

struct tweaked_xonly_key {
    std::array<std::uint8_t, 32> key;
    int parity;                                   // 0 when the point's y is even, 1 when odd
};
template <class Backend = backends::secp256k1>
boost::system::result<tweaked_xonly_key> tweak_xonly(
    std::span<std::uint8_t const, 32> xonly_public_key,
    std::span<std::uint8_t const, 32> tweak) noexcept;  // P + t*G

}
```

- The shape is trystero's `signer` and `verify`, which already run on libsecp256k1 with a
  context preallocated in the user's resource and the static context for verification
  (trystero `protocol/nostr.hpp:118-221`). `memory` is used for the context alone
  (`secp256k1_context_preallocated_create`), so the counting test sees every allocation. The
  context is allocated with `alignof(std::max_align_t)`, since libsecp256k1 asks for memory
  "aligned to hold an object of any type" (`include/secp256k1_preallocated.h:43`, `:85`,
  CR-REVIEW M9).
- **The static context and its self-test.** `verify_schnorr` and `verify_ecdsa` use
  `secp256k1_context_static`, which libsecp256k1 recommends using only after
  `secp256k1_selftest()` (`include/secp256k1.h:235`, `:241-255`). Each verification calls
  `secp256k1_selftest()` first: it hashes one 63-byte string with SHA-256 and compares it
  (`src/selftest.h`), next to a verification of about 20 µs natively (50,289 verifications
  per second, C-LIBS:242), and it keeps no flag, which a run-once check would need and the
  rule against static mutable state forbids (AGENTS chapter 6). A failure is a library built
  wrongly (the wrong endianness), and libsecp256k1 then calls its default error callback,
  which aborts (`src/secp256k1.c:87-91`). `make` needs no call: the preallocated context runs
  the self-test itself (`src/secp256k1.c:123`).
- **`ecdh`'s secret.** libsecp256k1's `secp256k1_ecdh` takes the secret key, so `ecdh` takes
  it out of the key pair (`secp256k1_keypair_sec`) into a local array, which is wiped with
  the key wipe of section 2.11 before `ecdh` returns, on every path (CR-REVIEW M9).
- `make` is not `noexcept`: the resource reports a failed allocation as it does, by
  throwing `std::bad_alloc` with exceptions on.
- The backend is a template parameter for the uniform shape (DESIGN, 2026-10-10
  "webcpp::crypto's interface and backends"). libsecp256k1 is its only backend: OpenSSL and
  Botan have ECDSA on secp256k1 but no BIP340, and the owner named libsecp256k1 for this
  (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries themselves").
- Errors: `make` gives `invalid_key_size` (not 32 bytes), `invalid_secret_key` (zero, or not
  below the group order), `entropy_unavailable` (the randomization); every other function of
  a moved-from key pair gives `invalid_state` (section 2.4). A public key maps to
  `invalid_public_key` in every way it can be wrong: in `ecdh` and `verify_ecdsa`, a size
  other than 33 or 65 bytes, or bytes that `secp256k1_ec_pubkey_parse` refuses (a prefix
  other than 02, 03 or 04, a coordinate not below the field's prime, a point not on the
  curve, an x with no point); in `verify_schnorr`, an x-only key that
  `secp256k1_xonly_pubkey_parse` refuses. `ecdh` gives `backend_failure` if
  `secp256k1_ecdh` returns 0 on a parsed key, which a valid key pair and libsecp256k1's own
  hash function never cause (`include/secp256k1_ecdh.h`). A verification that fails
  gives `invalid_signature`; `verify_ecdsa` gives it too for a compact signature that
  `secp256k1_ecdsa_signature_parse_compact` refuses (r or s not below the group order), and
  for one in high-S form, as `secp256k1_ecdsa_verify` does; the documentation says so.
- **ECDSA's encodings and the x-only tweak** (Task 27b, the owner's decision of 2026-11-14,
  reshaped on 2026-11-15). `to_der` writes a compact signature as strict DER into a caller's
  buffer of at least `max_der_size` bytes, 72 (`invalid_input_size` when shorter, nothing
  written), and returns its length; `from_der` reads strict DER (BIP66) with nothing after it;
  `normalize_ecdsa` gives the low-S form `verify_ecdsa` requires. Each gives
  `invalid_signature` for an r or an s that is zero or not below the group order, and
  `from_der` for anything that is not strict DER. `tweak_xonly` tweaks an x-only key by any
  32-byte tweak t, P + t*G, and gives the tweaked x-only key and its parity
  (`secp256k1_xonly_pubkey_tweak_add`); `keypair::tweak_xonly` tweaks a key pair in place
  (`secp256k1_keypair_xonly_tweak_add`). A tweak not below the group order, or one that gives
  no valid key, gives `invalid_secret_key`, and the key pair is then wiped and empty, as moved
  from; `tweak_xonly` gives `invalid_public_key` for a key that is not a point's x. Each is a
  wrapper over libsecp256k1's own function, reads the canary and allocates nothing. The caller
  computes the tweak: BIP341's TapTweak is a protocol's rule, not a primitive, and BIP341's
  vectors are the tweak's test (section 8.1). Lax DER, the Taproot functions, transactions,
  sighashes, BIP32 and addresses belong to a future domain library.
- The whole C API of libsecp256k1 v0.8.0 stays reachable through `<secp256k1*.h>`, every
  module built (C-LIBS:249-262): what the C++ types do not wrap is used directly.

### 2.9 Errors

`webcpp::crypto::error`, a `boost::system` error category named `webcpp.crypto`, with fixed
values that are never reused (AGENTS:636-638). Nothing of the draft of 2026-10-10 was
released, so the values start afresh:

| Value | Enumerator | Raised by |
| --- | --- | --- |
| 1 | `invalid_key_size` | every `make` |
| 2 | `invalid_iv_size` | GCM (empty, or longer than 128 bytes), ChaCha20-Poly1305 (not 12) |
| 3 | `invalid_tag_size` | `open` |
| 4 | `invalid_input_size` | a block cipher on a partial block; `to_der` into a buffer shorter than `max_der_size` |
| 5 | `message_too_long` | the AEADs' limits |
| 6 | `authentication_failed` | `open` |
| 7 | `invalid_state` | every function but `make`, assignment and destruction of a moved-from `aes128/192/256`, `gcm`, `chacha20_poly1305` or `curves::secp256k1::basic_keypair`; the incremental seal out of order |
| 8 | `entropy_unavailable` | `random_device`, `keypair::make` |
| 9 | `invalid_secret_key` | `keypair::make`; `tweak_xonly` and `keypair::tweak_xonly` (a tweak not below the group order, or one that gives no key) |
| 10 | `invalid_public_key` | `ecdh`, `verify_schnorr`, `verify_ecdsa`, `tweak_xonly` |
| 11 | `invalid_signature` | `verify_schnorr`, `verify_ecdsa`, `to_der`, `from_der`, `normalize_ecdsa` |
| 12 | `backend_failure` | a backend failing where its contract says it does not |
| 13 | `memory_functions_refused` | `backends::openssl::install_memory_functions` (section 3.7) |
| 14 | `memory_functions_not_installed` | every `make` of the OpenSSL backend, with the memory option (section 3.7) |

Two conditions use `boost::system::errc`, because they are generic: `not_enough_memory` and
`not_supported`.

### 2.10 Exceptions

Every operation reports its errors as values. One function throws, `random_device::operator()`,
through `boost::throw_exception`, because `std::random_device`'s shape has no other way to
report a failure (section 2.7); `keypair::make` lets the user's resource report a failed
allocation as it does (section 2.8). The library works whole with exceptions off. It has
`WEBCPP_CRYPTO_NO_EXCEPTIONS` in `config.hpp`, defined under `BOOST_NO_EXCEPTIONS`, and its
Doc Comment says what depends on it (AGENTS:772-787, CR-REVIEW M2):

- `random_device::operator()`, declared only with exceptions (section 2.7);
- the Botan backend, which exists only with exceptions (section 3.6);
- the `try` of OpenSSL's memory adapter, which turns an exception of the user's resource into
  a null pointer for OpenSSL (section 3.7), and is absent without exceptions: there, a
  resource that throws ends the process at the adapter's `noexcept`, and only a resource that
  returns null reports a failure (section 3.7).

### 2.11 Allocation and keys

- **webcpp::crypto's own code allocates only `keypair::make`'s context, in the resource the
  user gives** (section 2.8). Every buffer is the caller's (CR-REVIEW M2). OpenSSL's memory
  adapter (section 3.7) allocates nothing of its own: it forwards OpenSSL's requests to the
  resource the program names.
- **No allocation in `seal`, `open`, `begin_seal`, `update`, `result`, `encrypt`, `decrypt`,
  `fill`, the signatures, `ecdh` or the verifications, on any backend.** A backend that needs
  memory takes it in `make`, and the counting tests prove the rest (section 8.5). What each
  backend allocates in `make`, and through what, is in section 3. OpenSSL and Botan allocate
  there outside the user's allocator, a departure the owner accepted on 2026-10-11 (DESIGN,
  2026-10-11 "webcpp::crypto's two departures accepted"; section 10). For OpenSSL a program can
  end it: with the option `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, every allocation of
  OpenSSL's goes to a `std::pmr::memory_resource` the program names (DESIGN, 2026-10-11
  "OpenSSL's memory functions are a customization of webcpp::crypto's config"; section 3.7).
  Botan has no hook that could do the same, and keeps the departure (section 3.6).
- **Keys are wiped.** The backends' key schedules that live inside webcpp's objects (BearSSL's,
  ctaes's, libsecp256k1's key pair) are overwritten on destruction and on move by stores
  through a `volatile` pointer followed by `std::atomic_signal_fence(std::memory_order_seq_cst)`,
  which the optimiser cannot drop as dead stores; a plain `memset` can be dropped. OpenSSL's
  `EVP_CIPHER_CTX_free` and Botan's `clear()` and `secure_vector` wipe their own.
- **Constant time** is the backends' property, and webcpp's `equal` keeps it; the checks of
  section 8.3 cover the code as webcpp compiles it.

## 3. Backends

### 3.1 What each exports

| Backend | Library | Brought by | Targets | webcpp::crypto types it serves | Its own API, also reachable |
| --- | --- | --- | --- | --- | --- |
| `backends::bearssl` | BearSSL, MIT, commit 7bea48e5 of the mirror `webcpporg/bearssl` (DESIGN, 2026-10-11 "the BearSSL mirror") | built from source | native, wasip2, wasip3, emscripten | `gcm<aes128/192/256<bearssl>>`, `chacha20_poly1305<bearssl>`; no block cipher | `<bearssl.h>`, all of it |
| `backends::openssl` | OpenSSL 3.5.9's libcrypto, the superproject's `/webcpp/third_party//openssl` | built from source by the superproject, static | native, wasip2, wasip3 | `aes128/192/256<openssl>`, `gcm<...<openssl>>`, `chacha20_poly1305<openssl>` | `<openssl/*.h>` |
| `backends::ctaes` | ctaes as Bitcoin Core ships it, MIT, `src/crypto/ctaes` of `bitcoin/bitcoin` at tag v31.1 | built from source | native, wasip2, wasip3, emscripten | `aes128/192/256<ctaes>`: the block cipher alone | `<ctaes.h>`: `AES128/192/256_init`, `_encrypt`, `_decrypt` on whole blocks, nothing else at that tag (CR-PROBES `ctaes-pin.log`) |
| `backends::secp256k1` | libsecp256k1 v0.8.0, MIT, commit 6e2c8bc4 | built from source, every module on | native, wasip2, wasip3, emscripten | `curves::secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`, DER, low-S and `tweak_xonly` | `<secp256k1*.h>`, 82 functions (C-LIBS:249-262) |
| `backends::botan` | Botan 3, BSD-2-Clause, the system's | found on the machine | native only, for now (LEDGER:29) | `aes128/192/256<botan>`, `gcm<...<botan>>`, `chacha20_poly1305<botan>`; only with exceptions | `<botan/*.h>` |

The documentation gives this table, and per backend its Jamfile line, its headers and
functions, and every message of section 4.3 with its fix (AGENTS:737-739).

A backend's tag type has `static constexpr std::string_view name` (`"bearssl"`,
`"openssl"`, ...), which the examples print.

### 3.2 BearSSL

- **The original C, unchanged.** BearSSL is used as its author wrote it, as OpenSSL is: no port
  (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries themselves"). It is
  published only on its author's host; webcpp's unmodified mirror,
  `github.com/webcpporg/bearssl`, created from upstream master
  `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (2026-04-06) with tags v0.4 to v0.6, is the
  submodule's URL, refreshed only when the pin moves (DESIGN, 2026-10-11 "the BearSSL mirror").
- **Which code runs.** The backend picks, for GCM, the CTR class `br_aes_x86ni_ctr_get_vtable()`,
  then `br_aes_pwr8_ctr_get_vtable()`, then `br_aes_ct64_ctr_vtable`, and for GHASH
  `br_ghash_pclmul_get()`, then `br_ghash_pwr8_get()`, then `br_ghash_ctmul64` (C-LIBS:133-146).
  It names the 64-bit constant-time code itself rather than take BearSSL's default: on wasm32
  `BR_64` is not set, and BearSSL's default there is the 32-bit code, half as fast (159 against
  79 MiB/s, C-LIBS:151-155, :205-209). ARM has no hardware path in BearSSL: no ARMv8 AES or
  PMULL code exists in `src/` (C-LIBS:139).
- **Moving a `gcm`.** BearSSL's GCM context points at the key schedule
  (`br_gcm_context::bctx`, `inc/bearssl_aead.h:360`). A move copies the key schedule and calls
  `br_gcm_init` again on the destination, through the public API, so a message in progress is
  abandoned (section 2.4).
- **ChaCha20-Poly1305:** `br_chacha20_sse2_get()` else `br_chacha20_ct_run`, and
  `br_poly1305_ctmulq_get()` else `br_poly1305_ctmul_run` (`inc/bearssl_block.h:2456-2612`).
- **Allocation:** none, anywhere. The key schedule and the GCM context live inside the
  webcpp object.
- **Constant time:** BearSSL's software AES and GHASH are bitsliced and constant time (its
  design, which BoringSSL's software fallbacks copy, REVIEW:37). `bmul64` assumes a
  constant-time 64-bit multiplication (`src/hash/ghash_ctmul64.c:31-55`), which holds on x86-64
  and AArch64. A WebAssembly engine on a 32-bit host lowers `i64.mul` to several instructions,
  which may not be constant time: the documentation says so, and the checks under wasmtime
  (section 8.3) watch for it.
- **Licence:** MIT. The library's README names BearSSL, its notice ships as
  `LICENSE-BEARSSL.txt`, and no BearSSL file is copied into webcpp.

### 3.3 OpenSSL

- **Built from source by the superproject, never the machine's** (the provider amendment of
  2026-11-03, CODING-GUIDELINES.md chapter 5 rule 7, which supersedes for OpenSSL the
  decision of 2026-10-11 that found it). The superproject's provider builds OpenSSL 3.5.9,
  static, as `/webcpp/third_party//openssl`, on native, wasip2 and wasip3, and a program names
  crypto's alias over it, `/webcpp/crypto//openssl` (section 4.4). Emscripten has no OpenSSL:
  the alias stops the compile there with the fix (section 4.4).
- **Without assembly on MSVC, wasip2 and wasip3.** The provider builds OpenSSL there with
  `no-asm`: its AES is table-based and its GHASH 4-bit, neither constant time, and there is no
  AES-NI. The documentation says so, and that a program for which timing attacks matter there
  defines `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` or names
  `ciphers::gcm<ciphers::aes256<backends::bearssl>>`. OpenSSL stays the native default when
  linked, MSVC included (section 5).
- **Version.** OpenSSL 3.0 or later, for `EVP_CIPHER_fetch`; the provider builds 3.5.9. The
  header checks `OPENSSL_VERSION_NUMBER >= 0x30000000L` and `#error`s otherwise, for a build
  made by other means with an older OpenSSL's headers first (its test fixes the text):

      webcpp::crypto: the backend OpenSSL needs OpenSSL 3.0 or later, and the headers found are of an older OpenSSL. In the program's Jamfile, name /webcpp/crypto//openssl, which builds OpenSSL 3.5.9. (without b2: put OpenSSL 3's include/ first)

- **Calls.** `EVP_CIPHER_fetch` and `EVP_CIPHER_CTX_new` in `make`; each message re-initialises
  the same context (`EVP_EncryptInit_ex` with the IV length set by `EVP_CTRL_GCM_SET_IVLEN`),
  `EVP_EncryptUpdate` or `EVP_DecryptUpdate`, then `EVP_*Final_ex` and the tag controls. EVP
  takes lengths as `int`, so the backend feeds a longer span in pieces of at most `INT_MAX`
  bytes rounded down to a multiple of 16.
- **`make`'s errors, by the call that failed** (crypto-r3-review I3), the same on every
  OpenSSL 3 and with or without the option of section 3.7: `EVP_CIPHER_CTX_new` returning
  null gives `errc::not_enough_memory`; `EVP_CIPHER_fetch` returning null, or
  `EVP_EncryptInit_ex` returning 0, gives `backend_failure`. The error queue cannot tell a
  memory failure apart across versions: 3.6.4 raises `malloc failure` when an allocation
  returns null (`crypto/mem.c:194-198`, `:217-218`), 3.0.13 returns the null and raises nothing
  (`3.0.13 crypto/mem.c:180-181`). So a null from an allocation inside `EVP_EncryptInit_ex`
  (the provider's context, the second of the two blocks a later `make` takes, PROBE-R3) is a
  `backend_failure` too, and the documentation says so. A null from `EVP_CIPHER_fetch` may
  mean that OpenSSL's first initialisation failed, which is permanent for the process
  (section 3.7).
- **The key** is given to EVP once, in `make`, and webcpp keeps no copy of it: every message
  re-initialises the context with a null key, which keeps the expanded key, and switching the
  reused context from encryption to decryption that way works (measured on 3.6.4: a seal,
  then an `open` on the same context re-initialised with a null key, verifies its tag,
  CR-PROBES `ossl_open.c`, `ossl_open.out` "good open: ok=1"; CR-REVIEW M7).
- **The IV** is at most 128 bytes, OpenSSL's own limit (section 2.4); webcpp's guard refuses a
  longer one before OpenSSL sees it.
- **OpenSSL's error queue.** Nothing webcpp's calls raise is left in the thread's queue for the
  program's other users of OpenSSL (Beast's TLS) to misread (CR-REVIEW M7), and nothing the
  caller had there is lost:
  - **`make`** sets `ERR_set_mark()` after its guards (the key size and, with the option of
    section 3.7, the memory functions), immediately before `EVP_CIPHER_fetch`, and ends with
    `ERR_pop_to_mark()` on every path after it. Not earlier: the mark initialises OpenSSL and
    allocates the thread's error state (`crypto/err/err.c:665-700`), and as a process's first
    OpenSSL call it leaves `CRYPTO_set_mem_functions` returning 0 (crypto-r3-review
    `mark_first.log`), so a mark before the guard would make the install impossible for the
    rest of the process. `ERR_pop_to_mark()` removes exactly
    the entries raised after the mark and keeps the caller's (measured: an entry of the
    caller's survived a marked call that failed and raised, PROBE-R3 `errmark.log`). A `make`
    can succeed and still leave entries: with a resource that fails inside OpenSSL's first
    initialisation, `make` succeeded and left `malloc failure`, `too many names` or
    `internal error` behind (crypto-r3-review `exit_and_failure.log`, failures at the 500th,
    1,500th, 4,500th and 6,000th allocation).
  - **The functions of a message** (`seal`, `open`, the incremental seal, `encrypt`,
    `decrypt`) set no mark: the first `ERR_*` call of a thread allocates that thread's error
    state (`crypto/err/err.c:665-700`), 6 blocks measured on a thread that had not used
    OpenSSL's queue (PROBE-R3 `errmark.log`), which would break section 2.11's "no allocation"
    on every new thread. They need none on their normal paths: a success and a tag that does
    not match raise nothing (measured, CR-PROBES `ossl_open.out`; PROBE-R3 `errmark.log`). On
    the one path that raises, a `backend_failure`, which OpenSSL's contract says does not
    happen, they call `ERR_clear_error()`, so that nothing of theirs is left; the caller's
    earlier entries go with it, which the documentation says.
- **Allocation:** in `make`, through OpenSSL's own `CRYPTO_malloc`. Measured (PROBE-1011,
  `ossl.c`, OpenSSL 3.6.4): the first `make` of a process made 6,061 allocations, OpenSSL's
  own library initialisation included; three seals on the reused context then made **0**,
  with IVs of 12 and of 16 bytes. A good `open` and three failed ones made 0 each
  (CR-PROBES `ossl_open.out`). PROBE-R3 measured the same 6,061 for the first `make`, 2 for a
  second one, and 0 for seals and opens (`probe.log`, mode `on`). The documentation states
  both. On the source-built 3.5.9 the same counts are measured by section 8.5's test in
  increment 1.
  - **By default, outside the user's allocator.** OpenSSL's allocator is replaced only for the
    whole process (`CRYPTO_set_mem_functions`, section 3.7), which webcpp never does on its
    own. This departure from the owner's allocators decision was accepted on 2026-10-11
    (DESIGN, 2026-10-11 "webcpp::crypto's two departures accepted"; section 10) and is stated
    on the backend's page.
  - **With the option `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, through the program's resource**
    (DESIGN, 2026-10-11 "OpenSSL's memory functions are a customization of webcpp::crypto's
    config"): the program names a `std::pmr::memory_resource` and installs webcpp's adapter
    first in `main`, and every allocation OpenSSL makes in the process goes to that resource.
    Section 3.7 gives the design, its errors and its limits.
- **Process-wide state.** OpenSSL keeps a library context and its providers for the process.
  That state is OpenSSL's; webcpp adds none (AGENTS:639-640).
- **Linked static,** libssl and libcrypto from the provider's one build directory per
  configuration, with the link flags the provider gives; the build's threading follows the
  program's (`no-threads` under b2's default `threading=single`), and the documentation says
  so. The feature `webcpp-crypto-openssl-link` the owner confirmed on 2026-10-11 left with the
  machine's OpenSSL (section 10).

### 3.4 ctaes

- Pieter Wuille's constant-time bitsliced AES (AES-GCM:50), the AES of Bitcoin Core, which
  has no GCM (AES-GCM:51). At the pin it exports AES-128, AES-192 and AES-256 on whole
  blocks, encryption and decryption (`ctaes.h:29-39`), and nothing else: `aes256<ctaes>`
  works and `gcm<aes256<ctaes>>` does not compile (section 2.4).
- **The pin: the copy Bitcoin Core ships** (LEDGER:26), the owner's words, "o ctaes do
  Bitcoin Core". It is `src/crypto/ctaes` of `github.com/bitcoin/bitcoin` at the release tag
  v31.1 (Bitcoin Core's latest release, published 2026-07-08), the annotated tag `bfa6a4b7`
  on commit `9be056a8a72b624dae9623b2f7bded92c2a21c91`, whose `src/crypto/ctaes` is the tree
  `1b6c31139a71f80245c09597c343936a8e41d021` (`COPYING`, `README.md`, `bench.c`, `ctaes.c`,
  `ctaes.h`, `test.c`). Checked with `gh api` on 2026-10-11, and the probes' copy
  (PROBES `bitcoin-v31.1/src/crypto/ctaes`, C-LIBS:445) has the same blobs (CR-PROBES
  `ctaes-pin.log`).
- **Not `bitcoin-core/ctaes`.** ctaes also has a repository of its own,
  `github.com/bitcoin-core/ctaes`, whose master `3b10b89` the first version of this spec
  pinned. It is not the copy Bitcoin Core ships: it adds CBC (`AES{128,192,256}_CBC_ctx` and
  `_CBC_init/_encrypt/_decrypt`), `#include <string.h>`, two `(uint16_t)` casts in the
  bitslicing code, and NIST SP 800-38A CBC vectors in `test.c` (CR-REVIEW I5). Whatever a
  later pin's copy holds, webcpp exposes its block cipher alone; at v31.1 the copy holds
  nothing more.
- **The submodule** `third_party/libs/ctaes` is `github.com/bitcoin/bitcoin` at v31.1,
  declared `shallow = true` in `.gitmodules`, since AGENTS rule 1 keeps a mirror for upstreams
  not on GitHub (AGENTS:697-700). Its checkout is the whole of Bitcoin Core's tree at the tag,
  2,923 files and 49,069,741 bytes, with no submodule of its own (CR-PROBES `ctaes-pin.log`);
  `third_party/Jamfile` compiles `src/crypto/ctaes/ctaes.c` alone, and ctaes's own `test.c`
  for section 8.1. The canary's commit is `9be056a` (section 4.3); a later Bitcoin Core
  release moves the pin as a change of its own (AGENTS:747-748).
- **Allocation:** none; the key schedule (`AES256_ctx`) lives inside the webcpp object.

### 3.5 libsecp256k1

- `github.com/bitcoin-core/secp256k1`, tag v0.8.0, commit `6e2c8bc4`, built as its CMake
  build does: `src/secp256k1.c`, `src/precomputed_ecmult.c`, `src/precomputed_ecmult_gen.c`,
  every module on, `recovery` included (C-LIBS:221-223, PROBES `jam/third_party/Jamfile`).
- **Its tables** are the size knob: the default ones (window 15, comb 11x6) make an Emscripten
  program of 1,211,251 bytes at -Os, the small ones (window 2, comb 2x5) one of 141,922, for
  verification about a third slower (C-LIBS:238-247, :481-490). The feature
  `webcpp-secp256k1-tables` takes `auto` (the default), `small` or `large`; `auto` is `small`
  on Emscripten and `large` natively and on wasip2 and wasip3 (a ruling, LEDGER:27; section
  10). The owner's rule is the smallest binary on the web and the fastest on servers (DESIGN,
  2026-10-10 "webcpp::crypto's backends are the C libraries themselves"), and his mission entry
  names Emscripten the browser's target and WASI the server's and the cloud's ("webcpp's
  mission", DESIGN): on wasip2 the large tables verify 2,937 signatures per second against
  1,744 with the small ones (C-LIBS:242-243), about 1.7 times, for about 1.07 MB more
  (CR-REVIEW I6). The configuration enters the canary (`w2c2x5`, `w15c11x6`).
- **MSVC:** a static libsecp256k1 needs `SECP256K1_STATIC` in the program, which the target's
  usage requirements give (C-LIBS:626, trystero `build.jam:29`).
- **Allocation:** the signing context, in the user's resource; none elsewhere.
- **One copy per program.** The library's tables are global symbols, so webcpp's build is
  never linked with another libsecp256k1, trystero's external one included (C-LIBS:264-267).

### 3.6 Botan

- **Found, not built,** as OpenSSL was until 2026-11-03 (section 4.4): Botan is a C++ library
  with its own Python build, not header-only and not built by b2 (`configure.py:517-621`,
  REVIEW:28). `webcpp.external` finds Botan 3 by `botan-3/botan/aead.h`; it is linked shared
  or static by `webcpp-crypto-botan-link`, as OpenSSL was. The header checks
  `BOTAN_VERSION_MAJOR == 3`.
- **Targets: native only, for now** (LEDGER:29). No system Botan exists for Emscripten, and
  `webcpp.external` keeps one directory per target (`tools/webcpp.jam:973-987`), so an
  Emscripten Botan would need a build of webcpp's own, with Botan's `configure.py
  --os=emscripten`, the program's exception model (`-fwasm-exceptions` or `-fexceptions`) and
  both suspension variants (CR-REVIEW I8); none is defined, so `/third_party//botan` stops a
  WebAssembly build, Emscripten included, as `/webcpp/crypto//openssl` stops an Emscripten one,
  and `config.hpp` refuses `WEBCPP_CRYPTO_HAS_BOTAN` on WebAssembly as it refuses OpenSSL's on
  Emscripten (section 4.3). Botan 3.13.0 has no OS
  definition for wasip2 or wasip3 in any case (`src/build-data/os/`, REVIEW:29). A Botan
  WebAssembly build is a later increment's, if a user needs it.
- **Only with exceptions on.** Botan reports every error by throwing, the tag check included
  (`botan_gcm.cpp:105`, `:192`). Under chapter 6's rule for a header that wraps a throwing
  dependency (AGENTS:805-821), `backends/botan.hpp` declares nothing under
  `WEBCPP_CRYPTO_NO_EXCEPTIONS`, every test and program of the backend declares
  `<exception-handling>on`, and the lint leaves the header out of its analysis without
  exceptions and names it. A program built without exceptions that names `/third_party//botan`
  and uses the default backend gets an `#error` from `config.hpp` (new text, with its test):
  "webcpp::crypto: the backend Botan needs exceptions, and this program is built without them.
  In the program's Jamfile, name /third_party//bearssl or /webcpp/crypto//openssl in its place."
- **Its exceptions are caught at the boundary.** Every call into Botan sits in one `try`:
  `Botan::Invalid_Authentication_Tag` becomes `authentication_failed`,
  `Botan::Invalid_IV_Length` `invalid_iv_size`, `Botan::Invalid_Key_Length`
  `invalid_key_size`, `std::bad_alloc` `errc::not_enough_memory`, and any other
  `std::exception` `backend_failure`. No exception leaves a webcpp function.
- **Allocation outside the user's allocator.** `make` creates the cipher objects
  (`Botan::AEAD_Mode::create`, a `std::unique_ptr`) and a scratch `secure_vector` reserved to
  one update granularity plus the tag, through `new` and Botan's `secure_allocator`, outside
  the user's allocator, and the documentation says so. Botan's `finish` takes a `secure_vector`
  and appends the tag to it (`botan_gcm.cpp:139-153`), so the backend runs the bulk of a
  message through `process` on the caller's span and finishes on the reserved scratch, which
  then never grows; the counting test proves that `seal` and `open` allocate nothing.
  Allocating outside the user's allocator departs from the owner's allocators decision; the
  owner accepted it on 2026-10-11 (DESIGN, 2026-10-11 "webcpp::crypto's two departures
  accepted"; section 10), and the backend's page states it.
- **No hook can route Botan's allocations, so the departure stays** (DESIGN, 2026-10-11
  "OpenSSL's memory functions are a customization of webcpp::crypto's config" gave OpenSSL an
  option; Botan 3.13.0 offers nothing that could carry one, BOTAN-SRC):
  - Its secure memory goes through `Botan::allocate_memory` and `deallocate_memory`
    (`src/lib/utils/allocator.h:33`, `:41`), public functions with fixed definitions and no
    setter. `allocate_memory` takes a block from the mlock pool when the
    `locking_allocator` module is built, else calls `std::calloc` (`calloc_conceal` where the
    OS has it), and `deallocate_memory` scrubs and gives back to the pool or to `std::free`
    (`src/lib/utils/allocator.cpp:21-48`, `:50-64`).
  - `secure_allocator<T>`, the allocator of every `secure_vector` (GHASH's tables
    `m_HM` and `m_H_pow`, `src/lib/utils/ghash/ghash.h:104-105`), is stateless: it holds
    nothing, calls those two functions (`src/lib/base/secmem.h:95`, `:102`), and every
    instance compares equal (`:111-114`). Botan's mode classes take no allocator parameter.
  - The mlock pool is one global object, `g_mlock_allocator`
    (`src/lib/utils/locking_allocator/locking_allocator.cpp:75-82`), sized once at static
    initialisation from `RLIMIT_MEMLOCK` and capped at 512 KiB, a cap the environment variable
    `BOTAN_MLOCK_POOL_SIZE` can only lower (`std::min`,
    `src/lib/utils/os_utils/os_utils.cpp:388-405`). Building Botan
    with `--disable-modules=locking_allocator` (`configure.py:616`) only moves secure memory
    from the pool to `calloc`.
  - The objects themselves are `std::make_unique`'d (`GCM_Mode`'s `CTR_BE` and `GHASH`,
    `src/lib/modes/aead/gcm/gcm.cpp:38-39`; `ChaCha20Poly1305_*`,
    `src/lib/modes/aead/aead.cpp:68-70`), through the global `operator new`, which a
    program replaces only for all of its C++ and which does not reach `allocate_memory`'s
    `calloc`.
  - Defining `allocate_memory` in the program is no interface: against a static Botan it is a
    duplicate symbol, against a shared one an interposition of Botan's internals.

  So the backend's page says that Botan's `make` allocates through `new` and Botan's own
  secure allocator, and that a program which needs every allocation in its allocator uses
  OpenSSL with the option of section 3.7, or BearSSL.
- **The additional data is set on every message, empty included.** Botan keeps it between
  messages: `GHASH::start` copies the stored associated data's hash into the message's state
  (`botan_ghash.cpp:173`), and only `set_associated_data` replaces it (`:178-186`). So the
  backend calls `set_associated_data` before every `start`, in `seal`, `open` and
  `begin_seal`, with an empty span when there is none; otherwise a message without
  additional data would be authenticated with the previous message's (CR-REVIEW I4). A test
  seals with additional data, then without, and compares with BearSSL.
- **Never the first choice by default** (section 5): it is the default only when it is the
  only AEAD backend linked.

### 3.7 OpenSSL's memory functions, an option of the config (the owner, 2026-10-11)

"`CRYPTO_set_mem_functions` becomes an option of the library's config, so a program that wants
OpenSSL's allocations to go through its own functions can say so; without the option, the
departure above stands" (DESIGN, 2026-10-11 "OpenSSL's memory functions are a customization of
webcpp::crypto's config"). This section gives the option's facts, its form, its errors and its
limits.

**What OpenSSL offers** (OPENSSL-SRC, 3.6.4 unless stated):

- **The call.** `int CRYPTO_set_mem_functions(CRYPTO_malloc_fn, CRYPTO_realloc_fn,
  CRYPTO_free_fn)`, whose types are `void *(*)(size_t num, const char *file, int line)`,
  `void *(*)(void *addr, size_t num, const char *file, int line)` and
  `void (*)(void *addr, const char *file, int line)` (`include/openssl/crypto.h.in:345-351`;
  the installed Homebrew header, `crypto.h:368-374`). A null argument leaves that function as
  it is (`crypto/mem.c:67-72`). It returns 1, or 0 when it refuses (`mem.c:65-66`, `:73`),
  "almost always because allocations have already happened"
  (`doc/man3/OPENSSL_malloc.pod:272-273`).
- **When it refuses.** A flag, `allow_customize`, starts at 1 (`mem.c:22`), and the first
  allocation through OpenSSL's own `CRYPTO_malloc` clears it (`mem.c:205-212`); from then on
  the call returns 0 (`mem.c:65-66`). 3.0.13, the CI's Linux version, has the same rule
  (`3.0.13 crypto/mem.c:57-61`, `:180-193`). Measured: one `EVP_CIPHER_fetch`, then the call,
  which returned 0 and left OpenSSL's own functions in place (PROBE-R3, mode `late`).
- **And when it does not.** Once other functions are installed, `CRYPTO_malloc` calls them
  and returns before the line that clears the flag (`mem.c:194-198`), so a later
  `CRYPTO_set_mem_functions` is accepted after any number of allocations. Measured: after an
  AEAD's `make`, `seal` and `open` through installed functions, a second party's functions
  were accepted (the call returned 1) and became OpenSSL's (PROBE-R3, mode `on`). OpenSSL
  would then free the first functions' blocks with the second's.
- **Process-global, and not synchronised.** The three pointers and the flag are plain
  `static` variables of `mem.c` (`:22-25`), written by `CRYPTO_set_mem_functions` without a
  lock (`:61-74`) and read on every allocation, reallocation and free (`:194`, `:321`, `:378`).
  The functions are the process's, every user of OpenSSL in it included (libssl, Beast's TLS,
  libdatachannel's DTLS), and an install must happen before any other thread can call OpenSSL.
- **No size on free.** `CRYPTO_free_fn` receives the address, a file and a line
  (`crypto.h.in:348`), and `OPENSSL_free(addr)` is `CRYPTO_free(addr, OPENSSL_FILE,
  OPENSSL_LINE)` (`crypto.h.in:131-132`). `std::pmr::memory_resource::deallocate` needs the
  size and the alignment, so the adapter below stores the size.
- **Every allocation of OpenSSL's goes through them.** `CRYPTO_zalloc` calls `CRYPTO_malloc`
  (`mem.c:222-231`); `CRYPTO_aligned_alloc` calls `posix_memalign` only while OpenSSL's own
  `malloc` is in place, and otherwise over-allocates through `CRYPTO_malloc` and aligns inside
  the block (`mem.c:251-252`, `:286-313`); `CRYPTO_clear_realloc` and `CRYPTO_clear_free`
  cleanse, then go through `CRYPTO_malloc` and `CRYPTO_free` (`mem.c:348-393`). The one
  exception is the secure heap: it exists only when a program calls
  `CRYPTO_secure_malloc_init`, and then serves `OPENSSL_secure_malloc` from an arena OpenSSL
  maps itself (`crypto/mem_sec.c:107`, `:153-179`); without it the secure calls go to
  `CRYPTO_malloc` (`:160-161`). webcpp never calls it.
- **OpenSSL frees at exit, and when is platform-dependent.** Its first initialisation
  registers `OPENSSL_cleanup`, which frees what the library still holds through the installed
  free function:
  - **ELF and Mach-O** register it with `atexit` (`crypto/init.c:100-112`, the branch after
    `#else`). Measured on macOS, shared and static: 4,621 blocks were live when `main`
    returned, and all of them came back through the adapter after it, 0 left (PROBE-R3, mode
    `on`; crypto-r3-review `exit_and_failure.log`).
  - **Windows** registers `win32atexit`, which calls `OPENSSL_cleanup`, with `_onexit`, "in
    preference because it gets called on DLL unload" (`crypto/init.c:92-97`, `:107-110`).
    With a shared libcrypto that runs when the DLL unloads, after `exit` has run the
    program's own exit table and destroyed its static objects (crypto-r3-review C1, from
    Microsoft's documentation of `_onexit`; not measured: no Windows lane has run yet).
  - **Threads.** OpenSSL also frees a thread's state when the thread ends: from `DllMain`'s
    `DLL_THREAD_DETACH` on Windows (`crypto/dllmain.c:37-39`), from a thread-local key's
    destructor elsewhere, so for a thread that outlives `main` that free also comes late.

  So the program's resource is freed through after every destructor of the program has run,
  on the default Windows link, and it must never be destroyed (below).
- **Measured, everything through the functions** (PROBE-R3, `probe.log`; the C library's
  `malloc`, `calloc`, `realloc`, `posix_memalign`, `aligned_alloc` and `free` interposed and
  counted, AES-256-GCM driven as section 3.3 drives it). With the functions installed first:
  the first `make` called the adapter 6,061 times to allocate and 192 to reallocate, and
  libcrypto called the C library's allocator **0** times; a second `make` allocated 2 blocks
  through the adapter; three seals, a good open, three failed opens and the frees of the
  contexts made 0 calls to the C library's allocator from any caller. Without them, the same
  first `make` made 7,584 calls from libcrypto. The first `make` made 2 calls from elsewhere
  in both runs. The probe's first frame for them is the C library's (`libsystem_malloc`,
  `fclose` in `libsystem_c`), and they disappear when no configuration file is read
  (`OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr)` before the first `make`, mode
  `noconf`, or a configuration path that names no file). So they are the C library's own
  allocations for the configuration file OpenSSL opens at initialisation
  (`crypto/conf/conf_def.c:175`, through `BIO_new_file`, `crypto/bio/bss_file.c:57-60`): its
  `FILE` and its buffer, one or two blocks by C library (macOS: one, measured; glibc
  allocates the `FILE` at `fopen` and the buffer at the first read), freed when the file
  closes. No hook of OpenSSL's reaches an allocation the C library makes inside a function
  OpenSSL calls.
- **A failing allocation.** With functions of the program's installed (crypto-r3-review
  `exit_and_failure.log`, shared and static alike, the n-th allocation after the install
  failing):
  - **Early in OpenSSL's first initialisation, the failure is permanent.** At n = 0, 5 and
    50, `EVP_CIPHER_fetch` returned null with `03080106` ("passed invalid argument"), and it
    kept returning null after the resource had stopped failing: OpenSSL runs its
    initialisation once and keeps its failure.
  - **Later in the initialisation, it is often swallowed.** At n = 500, 1,500, 4,500 and
    6,000, `make` succeeded and left an entry in the thread's queue (`malloc failure`, `too
    many names`, `internal error`).
  - **After a good first `make`,** a failure of the next allocation made `EVP_CIPHER_CTX_new`
    return null with `malloc failure`, and the `make` after it succeeded.
  - **3.6.4 and 3.0.13 differ.** 3.6.4 raises `malloc failure` when the installed function
    returns null (`mem.c:194-198`, `:217-218`); 3.0.13 returns the null and raises nothing
    (`3.0.13 crypto/mem.c:180-181`).

**The option.**

- **In the program's Jamfile,** `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, a macro of
  the config as `WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>` is (section 5), for native builds
  only, refused on every WebAssembly target, WASI included. It is given to every target of the program that includes a header of
  webcpp::crypto, its own libraries included, and every target that gets it names
  `/webcpp/crypto//openssl`: the project's requirements carry both,
  `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE <library>/webcpp/crypto//openssl`, and a
  WebAssembly build of the program is a project of its own (section 7.3).
- **With it,** `<webcpp/crypto/backends/openssl.hpp>` declares a function that the program
  defines once, and the backend's tag type gets the install:

  ```cpp
  namespace webcpp::crypto {

  // Defined by the program, only with WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE: the resource
  // every allocation of OpenSSL's in the process goes to, from any thread. Never destroyed.
  std::pmr::memory_resource& openssl_memory_resource() noexcept;

  namespace backends {
  struct openssl {
      static constexpr std::string_view name = "openssl";
      // Only with WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE. Called first in main.
      static boost::system::result<void> install_memory_functions() noexcept;
  };
  }

  }
  ```

- **Why a function the program defines.** OpenSSL's hook carries no user data
  (`crypto.h.in:345-348`): an install that took the resource as an argument would have to
  keep the pointer in a variable of webcpp's for the adapter to find, the global mutable state
  that chapter 6 forbids (AGENTS:639-640), and a template parameter reaches a C callback only
  through the same variable. A function the program defines keeps that state in the program,
  the form Boost already uses for a process-wide customisation: a program that defines
  `BOOST_ENABLE_ASSERT_HANDLER` defines `boost::assertion_failed` (`boost/assert.hpp:38`,
  `:61-67`, Boost 1.92), and one without exceptions defines `boost::throw_exception`
  (`boost/throw_exception.hpp:39-42`), as webcpp's `tools/throw_exception.cpp` does
  (AGENTS:763-767). A `std::pmr::memory_resource` is the allocator chapter 6 names for
  run-time objects (AGENTS:662-665), and the one `keypair::make` takes (section 2.8). This is
  the spec's reading of the owner's "its own functions", recorded as a ruling in section 10.
- **The resource is never destroyed** (crypto-r3-review C1). OpenSSL frees through it after
  `main` on every platform, and on Windows with a shared libcrypto after every static
  destructor of the program (above), so no destruction order can be relied on. The program's
  function returns a resource that lives until the process ends and is never destroyed: one
  made with `new` and never deleted, or one constructed with placement `new` in static
  storage. A function-local static object is not enough: its destructor runs before
  `OPENSSL_cleanup` on the Windows shared link. A tool that reports leaks does not report the
  resource, which a static pointer still reaches.
- **`install_memory_functions()`, guards first** (AGENTS:682-683). It only reads OpenSSL's
  state until the guards have passed:
  1. When `CRYPTO_get_mem_functions` gives webcpp's adapter already, it returns success and
     changes nothing: a second call, from another part of the program, is harmless.
  2. When it gives functions other than OpenSSL's own defaults (`CRYPTO_malloc`,
     `CRYPTO_realloc`, `CRYPTO_free`, `mem.c:23-25`), another party installed its own:
     `memory_functions_refused`. Replacing them would make OpenSSL free that party's blocks
     through webcpp's adapter, and OpenSSL would accept the replacement (above). How a default
     is recognised on every link is below.
  3. It calls `openssl_memory_resource()` once, so that the resource is constructed here,
     outside OpenSSL's allocation path, and a resource whose construction fails does so in
     the install.
  4. It calls `CRYPTO_set_mem_functions` with the adapter. A 0 means that OpenSSL has already
     allocated with its own functions, so something in the process used OpenSSL first (another
     library's static constructor, or code that ran before the call):
     `memory_functions_refused`.

  It reads and writes OpenSSL's state alone; webcpp keeps none (AGENTS:639-640).
- **Recognising OpenSSL's defaults on every link** (crypto-r3-review I1). Inside libcrypto
  the defaults are the functions' own addresses (`mem.c:23-25`). A function `f` that
  `CRYPTO_get_mem_functions` gives is OpenSSL's default `CRYPTO_malloc` when either holds:
  - `f == &CRYPTO_malloc`. On ELF and Mach-O, shared or static, the program reaches the
    function's own address (measured on macOS, shared and static: "defaults identical to
    &CRYPTO_*: 1", crypto-r3-review `exit_and_failure.log`; PROBE-R3 "defaults in place at
    start: 1"), and with a static libcrypto on Windows there is no import either.
  - On `_WIN32` only: the module that holds `f` exports `CRYPTO_malloc` at `f`
    (`GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, f, &module)`, then
    `GetProcAddress(module, "CRYPTO_malloc") == f`). It is needed because OpenSSL declares its
    functions without `__declspec(dllimport)` (`include/openssl/crypto.h.in:356`, `:371`,
    `:373`, whose `OSSL_CRYPTO_ALLOC` is `__declspec(restrict)` under MSVC,
    `include/openssl/macros.h:353-357`;
    `include/openssl/e_os2.h:179-185` gives `dllimport` to `OPENSSL_EXTERN` variables alone),
    so a program linked against `libcrypto-3-x64.dll` that takes `&CRYPTO_malloc` gets its own
    import thunk, never the address OpenSSL holds.

  The same for `CRYPTO_realloc` and `CRYPTO_free`. `GetProcAddress` was chosen over the
  import table's `__imp_CRYPTO_malloc`, which exists only with the shared link, is decorated
  differently on x86, and is a reserved name; it answers for any DLL name and either link.
  OpenSSL names its defaults through no API of its own, and calling the functions to see
  would allocate, which clears the flag. The Windows branch is unmeasured: section 8.5's MSVC
  lane runs the `late` and `foreign` tests on the shared and the static link. The header
  includes `<windows.h>` for it through `<webcpp/crypto/detail/windows.hpp>`, under
  `NOMINMAX` and `WIN32_LEAN_AND_MEAN` (section 2.2), with `lint-world:` and its reason
  (AGENTS:641-649).
  webcpp's own adapter needs no such care: its address is the program's, taken in the
  program.
- **Who installs, and when: the program, as the first statement of `main`,** before it
  starts a thread or calls anything that may use OpenSSL. webcpp does not install from a
  static initialiser of its own: the order of static initialisation across translation units
  and libraries is unspecified, so another library's constructor could use OpenSSL first, and
  a constructor could not return the error.
- **Never silence.** Under the option, every `make` of the backend checks, after the key size
  (section 2.4), that OpenSSL's functions are webcpp's adapter (`CRYPTO_get_mem_functions`,
  three loads) and otherwise returns `memory_functions_not_installed` before OpenSSL
  allocates anything. A program that defines `openssl_memory_resource()` and forgot the
  install, or whose functions another party replaced after it, gets an error, not an
  allocation outside its resource. A replacement after the install cannot be prevented, and
  the next `make` sees it only when the replacement happens before that `make`: one made on
  another thread while OpenSSL runs is a data race on `mem.c`'s plain statics (`:22-25`,
  `:61-74`), and nothing is then guaranteed. The documentation says that nothing else in the
  process may call `CRYPTO_set_mem_functions`.
- **The adapter,** three functions of webcpp's given to OpenSSL (PROBE-R3's `adapter_malloc`,
  `adapter_realloc` and `adapter_free` are a sketch of them):
  - **C linkage, one per program.** OpenSSL calls them as C functions, so they are declared
    `extern "C"`, with prefixed names, `webcpp_crypto_openssl_malloc`,
    `webcpp_crypto_openssl_realloc` and `webcpp_crypto_openssl_free`, `inline` with external
    linkage and `noexcept`; never `static` or in an unnamed namespace, since the install and
    `make` recognise them by address and every file of the program must see one function.
    A second module that includes the header has its own copy (a Windows DLL of the
    program's, or a shared object that hides its symbols), and its `make` returns
    `memory_functions_not_installed`: the option is for programs whose uses of webcpp's
    OpenSSL backend live in one module, which the documentation says.
  - Each block starts with a header that holds its size. The header's size and the blocks'
    alignment are the larger of `alignof(std::max_align_t)` and
    `__STDCPP_DEFAULT_NEW_ALIGNMENT__`, the alignment the system's `malloc` gives OpenSSL
    without the option (16 on x86-64 and arm64; on macOS arm64 `alignof(std::max_align_t)` is
    only 8, measured, PROBE-R3), so OpenSSL never gets less. Its aligned allocations need
    nothing more, since it aligns inside the block (`mem.c:286-313`).
  - `malloc(n)`: 0 bytes, or a size whose header would overflow `size_t`, give a null
    pointer, which OpenSSL takes as no block for 0 and as a failure otherwise (`mem.c:196`);
    otherwise `allocate(header + n, alignment)` on the resource. A null from `allocate` is a
    failure and is returned as such; otherwise the size is stored and the address after the
    header returned.
  - `free(p)`: a null `p` does nothing; otherwise the size is read and
    `deallocate(p - header, header + size, alignment)` called. OpenSSL wipes key material
    before it frees it (`OPENSSL_clear_free`, `mem.c:386-393`); the adapter adds no wipe.
  - `realloc(p, n)`: a null `p` is `malloc(n)`; `n == 0` frees `p` and returns null, as
    OpenSSL's own `CRYPTO_realloc` does (`mem.c:333-336`), and OpenSSL takes that null as
    success (`mem.c:324-325`); otherwise a new block of `n`, the smaller of the two sizes
    copied, `p` freed. On failure it returns null and leaves `p` valid, as C's `realloc` does.
  - **With exceptions,** each call into the resource sits in a `try` whose `catch (...)`
    returns null: OpenSSL is C, and no exception may cross it.
  - **Without exceptions** there is no `try` (section 2.10). A `std::pmr::memory_resource`
    reports a failure only by throwing (C++20 [mem.res.private], the Throws clause of
    `do_allocate`: an appropriate exception when it cannot allocate), and a resource built
    on `operator new` throws `std::bad_alloc` from code compiled with exceptions; the
    exception then meets the adapter's `noexcept` and the process ends (`std::terminate`). So without exceptions an
    allocation failure in the program's resource ends the process, unless the resource
    returns null instead, which the adapter checks and reports to OpenSSL as a failure. The
    documentation says so.
- **What `make` returns when the resource fails** (crypto-r3-review I3). By the call that
  failed, as section 3.3 maps it: `EVP_CIPHER_CTX_new` null gives `errc::not_enough_memory`;
  `EVP_CIPHER_fetch` null or `EVP_EncryptInit_ex` 0 gives `backend_failure`, the second
  whatever the cause, since the queue cannot tell a memory failure apart on 3.0.13 (above).
  `make` leaves the thread's queue as it found it (`ERR_set_mark` and `ERR_pop_to_mark`,
  section 3.3), the entries a swallowed failure leaves included. The mark is set after the
  guards, `memory_functions_not_installed`'s included, just before `EVP_CIPHER_fetch`: set
  first, it would initialise OpenSSL through its own `malloc` and make the install
  impossible (crypto-r3-review `mark_first.log`). `seal`, `open` and the other
  functions of a message allocate nothing (above), so a failing resource cannot reach them.
  **A failure during OpenSSL's first initialisation is permanent:** OpenSSL then stays
  unusable for the rest of the process, and every later `make` gives `backend_failure`
  (above). A bounded resource is sized for the first `make`: 6,061 allocations, 192
  reallocations and 4,621 blocks still live after it, with OpenSSL's configuration file read
  (PROBE-R3, OpenSSL 3.6.4 on macOS; other versions and configurations differ).
- **What the program's resource must be:**
  - never destroyed (above);
  - thread-safe whenever OpenSSL may run on two threads at once (webcpp objects used on two
    threads, or Beast's TLS beside them): `std::pmr::synchronized_pool_resource`,
    `std::pmr::new_delete_resource()`, or one of the program's. An `unsynchronized_` resource
    only in a program with one thread;
  - reached without an exception: `openssl_memory_resource()` is `noexcept`.
- **What it routes, and what it does not.** Every allocation of the libcrypto the program
  links, in the whole process, the TLS of Beast and the DTLS of libdatachannel included, not
  only webcpp's (section 8.5 tests both in the process, with increment 3). Not the secure
  heap, if the program initialises it, nor the C library's own allocations inside functions
  OpenSSL calls, such as the `FILE` of the configuration file and its buffer (above). A
  process with two copies of libcrypto (a static one in the program and a shared one under
  another library) has two hooks, and the install sets the one webcpp's backend calls.
- **Where it exists, and the layers that stop a wrong build.** The option exists where the
  OpenSSL backend does, natively (section 3.3). `config.hpp` refuses it (layer 1, section
  4.3; new text, with its tests, section 8.6):
  - on WebAssembly: `webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE configures OpenSSL, a native backend, and this program is built for WebAssembly. Remove that define from the program's Jamfile.`
  - natively, without `WEBCPP_CRYPTO_HAS_OPENSSL`: `webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE routes OpenSSL's allocations, and OpenSSL is not linked. In the program's Jamfile, add /webcpp/crypto//openssl beside /webcpp/crypto//crypto, or remove that define. (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto)`

  **A missing definition is a link error** (crypto-r3-review I2). Under the option every
  `make` of the backend compares OpenSSL's functions with the adapter's address, which
  odr-uses the adapter, and the adapter calls `openssl_memory_resource()`. So a program that
  sets the option and uses the backend, through `install_memory_functions()` or through any
  `make`, and does not define the function, fails to link on its name; ld64 prints
  `Undefined symbols for architecture arm64:` / `"webcpp::crypto::openssl_memory_resource()",
  referenced from:` (PROBE-R3, `linkerr.log`). `memory_functions_not_installed` is a run-time
  error of a program that does define it. On MSVC, `backends/openssl.hpp` carries
  `#pragma detect_mismatch("webcpp_crypto_openssl_memory", "resource")` with the option and
  `"openssl"` without it, so that two files of one program compiled one with and one without
  the option, whose `make` differ, fail with LNK2038 (layer 3).
- **The errors' messages**, from the category of section 2.9 (new text, with its tests):
  - `memory_functions_refused`: "OpenSSL's memory functions cannot be installed: OpenSSL has
    already allocated, or another party installed its own. Call
    backends::openssl::install_memory_functions() as the first statement of main, before
    anything uses OpenSSL."
  - `memory_functions_not_installed`: "OpenSSL's memory functions are not webcpp::crypto's.
    Call backends::openssl::install_memory_functions() as the first statement of main, and
    let nothing else call CRYPTO_set_mem_functions."
- **Only OpenSSL.** The option does nothing for Botan, which has no hook (section 3.6), nor
  for BearSSL, ctaes and libsecp256k1, which need none.

## 4. The build

### 4.1 Where the C libraries live and how they are built

By AGENTS:697-711 and DESIGN, 2026-10-11 "C libraries built from their own sources, and the
backend named in the user's Jamfile":

| Library | Submodule | Upstream | Pin |
| --- | --- | --- | --- |
| BearSSL | `third_party/libs/bearssl` | `webcpporg/bearssl`, the mirror | `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` |
| libsecp256k1 | `third_party/libs/secp256k1` | `bitcoin-core/secp256k1` | v0.8.0, `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d` |
| ctaes | `third_party/libs/ctaes` | `bitcoin/bitcoin`, shallow; its `src/crypto/ctaes` (section 3.4) | tag v31.1, `9be056a8a72b624dae9623b2f7bded92c2a21c91` |

`third_party/Jamfile` (project `/third_party`) builds each one from its own files, one object
per file, as a static library, on every target and with every file (for ctaes, the one file
of `src/crypto/ctaes/` that is the library, `ctaes.c`): BearSSL's 294 files of
`src/`, the POWER8 ones included, since off POWER they compile to the stubs that other files
call, and an archive without them fails to link (`Undefined symbols: "_br_aes_pwr8_ctr_get_vtable",
"_br_ghash_pwr8_get"`, C-LIBS:503-508). No `-std` is given, so each library compiles in its
compiler's GNU dialect, which BearSSL's POWER8 files need (C-LIBS:109-111). The project's
requirements are `<link>static`, `<warnings>off` (upstream's warnings are not webcpp's),
`-ffunction-sections -fdata-sections` for clang, gcc and Emscripten, `/Gy /Gw` for MSVC.
webcpp's `libs/crypto/src/canary_<name>.c` joins each library's sources (section 4.3); it
lives in webcpp's repository, never in the mirror (DESIGN, 2026-10-11 "the BearSSL mirror").

The defines a library cannot work out for itself sit beside its sources, each with its reason:

| Library | Define | Where | Why |
| --- | --- | --- | --- |
| BearSSL | `BR_RDRAND=0`, `BR_USE_URANDOM=0` | every target | one source of entropy, no RDRAND (section 6) |
| BearSSL | `BR_USE_GETENTROPY=1`, `_DEFAULT_SOURCE` | every target but Windows | `getentropy` is the source; wasi-libc and Emscripten declare it only under `_DEFAULT_SOURCE`, and without the define BearSSL has no seeder on WASI or Emscripten (C-LIBS:112-118, :156-160) |
| BearSSL | `-include sys/random.h` | macOS | `src/rand/sysrng.c` includes only `<unistd.h>`, and macOS declares `getentropy` in `<sys/random.h>`: without it the file fails, "call to undeclared function 'getentropy'"; with it the seeder reports `seeder=getentropy` and seeds (PROBE-1011) |
| BearSSL | `BR_USE_GETENTROPY=0`, `BR_USE_WIN32_RAND=0` | Windows | BearSSL's Windows seeder is `CryptGenRandom` (`src/rand/sysrng.c:39-43`), not the platform's one source, `BCryptGenRandom`, which webcpp's `random_device` uses; BearSSL's own seeder is then absent, webcpp never calls it, and no backend chooses its entropy (section 6; a ruling, LEDGER:27, listed in section 10) |
| libsecp256k1 | `ENABLE_MODULE_*=1`, the table sizes | every target | every module; the tables of section 3.5 |
| libsecp256k1 | `SECP256K1_STATIC` (usage requirement) | MSVC | the static library's declarations |
| libsecp256k1 | `WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES` (usage requirement) | where the tables are small: `small`, or `auto` on Emscripten | the header expects the canary of the configuration built, `w2c2x5` with it and `w15c11x6` without (section 4.3, CR-REVIEW M12) |

Usage requirements of each library: its public include directory,
`WEBCPP_CRYPTO_HAS_<NAME>=1`, the defines of the table above marked as usage requirements,
and the link-time removal of unused sections
(`-Wl,-dead_strip` on Darwin, `-Wl,--gc-sections` on Linux, `/OPT:REF` with MSVC, which
`link.exe` leaves off under `/DEBUG`; wasm-ld and Emscripten do it by default) (AGENTS:709-711,
C-LIBS:515-523, :626-628).

`/webcpp/crypto//crypto` is header-only: an alias that gives `libs/crypto/include`, and on
Windows `bcrypt` for `random_device` (section 6).

### 4.2 Built file by file, and a library named but not called

- **Sizes against the unity build** (C-LIBS:459-470): built file by file, BearSSL's AES-GCM
  program is never larger in code than the unity build, and 0.6 to 2.4 KB smaller on
  WebAssembly (wasip2 -Os stripped 59,887 against 60,570 bytes). libsecp256k1 is already its
  own unity build, and the two agree to 120 bytes (C-LIBS:481-493).
- **A library built from source, named and not called, adds no byte.** A program built with
  `/third_party//bearssl` and the same program without it are byte-identical on all four
  targets; natively the two must be linked under the same output name, because ld64's ad hoc
  signature embeds it (C-LIBS:495-501). This holds for BearSSL, libsecp256k1 and ctaes, the
  static archives webcpp builds. OpenSSL is the provider's static archive (section 4.4) and is
  not part of this check. A shared Botan named and not called still adds its load command
  (Mach-O) or `DT_NEEDED` entry (ELF) and so a dependency at run time (CR-REVIEW M5).

### 4.3 Three layers stop a wrong build

Each layer says exactly what to add (AGENTS:716-725). The messages below are the code's, which
their tests fix; the probes produced their first form, and a file path before a message is the
prototype's.

**Layer 1: compile time, the `HAS` define.** `config.hpp` classifies the target, refuses
impossible combinations and picks the default backend. The check that a backend is linked
sits in the backend's own header, which the algorithms include, so that a program that only
signs needs no encryption backend (C-LIBS:546-551). Clang and gcc continue after `#error`, so
the first error is the instruction and each following `use of undeclared identifier 'webcpp'`
comes from it; MSVC stops there, `fatal error C1189: #error: "webcpp::crypto: ..."`
(C-LIBS:562-564).

| Case | Message (clang; first produced by the probes, PROBES `jam/out/*.log`, and since then reworded with the provider amendment of 2026-11-03 where they name OpenSSL; the six marked * are in the prototype's headers, `jam/crypto/include/webcpp/crypto/config.hpp:21,24,27`, `backends/bearssl.hpp:19`, `openssl.hpp:14`, `ctaes.hpp:10`, and were reproduced by the review, CR-PROBES `star-messages.log`) |
| --- | --- |
| no backend, WebAssembly (wasip2, wasip3, emscripten) | `error: "webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"` |
| no backend, native | `error: "webcpp::crypto: no backend is linked. In the program's Jamfile, add /webcpp/crypto//openssl (OpenSSL built from source) or /third_party//bearssl (built from source) beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ; (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto, or define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"` |
| BearSSL selected, not linked * | `error: "webcpp::crypto: the backend BearSSL is used and not linked. In the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, or, with /webcpp/crypto//openssl linked, select OpenSSL with <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL. (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"` |
| OpenSSL used, not linked * | `error: "webcpp::crypto: the backend OpenSSL is used and not linked. In the program's Jamfile, add /webcpp/crypto//openssl beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto)"` |
| ctaes used, not linked * | `error: "webcpp::crypto: the backend ctaes is used and not linked. In the program's Jamfile, add /third_party//ctaes beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_CTAES=1 and link ctaes.c with webcpp's src/canary_ctaes.c)"` |
| libsecp256k1 not linked | `error: "webcpp::crypto: <webcpp/crypto/curves/secp256k1.hpp> needs libsecp256k1, which is not linked. In the program's Jamfile, add /third_party//secp256k1 beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_SECP256K1=1 and link libsecp256k1 built with webcpp's src/canary_secp256k1.c)"` |
| OpenSSL's `HAS` on Emscripten, without b2 * | `error: "webcpp::crypto: OpenSSL is not built for Emscripten. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl."` |
| `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` on Emscripten * | `error: "webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL selects OpenSSL, which is not built for Emscripten. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL."` |
| two defaults selected * | `error: "webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL are defined. Keep one."` |

New messages, not yet produced by a probe, whose tests fix their text: `gcm` and
`chacha20_poly1305` over a backend without the mode (section 2.4), a block cipher's `make` on
BearSSL (section 2.3), OpenSSL older than 3.0 (section 3.3), Botan without exceptions and
Botan not linked (section 3.6), the three-way selection with Botan
(`WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN`), Botan's `HAS` on WebAssembly without b2, the
text of OpenSSL's with Botan's name (section 3.6), and `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`
on WebAssembly or without OpenSSL linked (section 3.7).

**Layer 2: link time, the canary.** Every function of a backend's header that calls the C
library reads, with a volatile load, the symbol

    webcpp_crypto_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link

which `libs/crypto/src/canary_<name>.c` defines from the library's own compile flags; no
linker can drop the reference, and its cost is one byte and one load. A library missing, of
another commit, of another configuration, or built without the canary, fails to link with
that name (C-LIBS:571-583). The pins are in `<webcpp/crypto/detail/canary.h>`, readable as C
and C++. The configurations:

| Library | Configurations |
| --- | --- |
| BearSSL | its seeder: `getentropy` on every target but Windows, `noseeder` on Windows (section 4.1) |
| libsecp256k1 | its tables: `w15c11x6` (large), `w2c2x5` (small) |
| ctaes | `default` |

| Linker | Message (`canary/canary.log`, `msvc/ce*.out.json`) |
| --- | --- |
| ld64 (Apple clang 21) | `Undefined symbols for architecture arm64:` / `"_webcpp_crypto_bearssl_7bea48e_sysrng__add_third_party_bearssl_to_your_link", referenced from: webcpp::crypto::backends::bearssl::gcm::gcm(...) in app.o`; libsecp256k1: `"_webcpp_crypto_secp256k1_6e2c8bc_w15c11x6__add_third_party_secp256k1_to_your_link", referenced from: webcpp::crypto::secp256k1::schnorr_key::schnorr_key() in sign.o` |
| wasm-ld (wasi-sdk 34) | `wasm-ld: error: .../app.o: undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link`, then `error: failed to invoke LLD: exit status: 1` |
| Emscripten 6.0.11 | the same `wasm-ld: error:` line, then `em++: error: '<the whole wasm-ld command>' failed (returned 1)` |
| MSVC `link.exe` (Compiler Explorer, x64 msvc v19.latest) | `error LNK2001: unresolved external symbol webcpp_crypto_bearssl_7bea48e_sysrng__add_third_party_bearssl_to_your_link`, then `fatal error LNK1120: 1 unresolved externals` |

The probes ran before the entropy rule of section 6, with BearSSL's own seeder choice, which
the canary named `sysrng` natively. Under section 4.1's defines the native name is
`getentropy` (Windows: `noseeder`), so a native build reads
`webcpp_crypto_bearssl_7bea48e_getentropy__...`. The ctaes pin is the probes' own, Bitcoin
Core v31.1's `9be056a` (section 3.4), so ctaes's canary is
`webcpp_crypto_ctaes_9be056a_default__add_third_party_ctaes_to_your_link`.

With the library absent, the canary is one undefined symbol among the library's own (ld64
lists it last, wasm-ld fifth); with a library of another commit or configuration, or built
without the canary, it is the only error (C-LIBS:591-593).

**Layer 3: MSVC, `#pragma detect_mismatch`.** Each backend header and each canary file carry
`#pragma detect_mismatch("webcpp_crypto_backend_<name>", "<name>-<commit7>-<configuration>")`
under `_MSC_VER`, one key per backend, since one key for all would fail a program that uses
BearSSL and libsecp256k1 with a false mismatch (C-LIBS:596-599). Measured with `link.exe`:
`error LNK2038: mismatch detected for 'webcpp_crypto_backend_bearssl': value 'bearssl-8f795e5-sysrng' doesn't match value 'bearssl-7bea48e-sysrng' in output.s.obj`,
then `fatal error LNK1319: 1 mismatches detected` (C-LIBS:607-612). Through a static library a
mismatching canary member is not loaded, so another commit gives LNK2001; LNK2038 fires
between two of the program's own files compiled with different configurations. It is a second
net, chiefly between a program's own files (C-LIBS:616-621). OpenSSL's backend header carries
one more key, `webcpp_crypto_openssl_memory`, whose value says whether the file was compiled
with `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` (section 3.7).

**A b2 pitfall the tests must avoid.** Two `exe` of one Jamfile built from the same `.cpp` and
differing only by a `/third_party` library share one object file, since defines are free
features and do not change its path: b2 reused the object compiled with
`WEBCPP_CRYPTO_HAS_BEARSSL` for the program without it, which then failed at link instead. Each
error case has a directory of its own (C-LIBS:565-569).

### 4.4 OpenSSL built from source; Botan found, or the build stops

**OpenSSL is the superproject's, built from source** (the provider amendment of 2026-11-03,
CODING-GUIDELINES.md chapter 5 rule 7). It supersedes for OpenSSL the decisions of 2026-10-11
that found OpenSSL on the machine and linked it shared or static: the superproject's provider
builds OpenSSL 3.5.9 from its own submodule as `/webcpp/third_party//openssl`, static, on
native, wasip2 and wasip3, and webcpp never links a copy of the machine's. A program names
crypto's alias over it, `/webcpp/crypto//openssl` (`libs/crypto/build.jam`):

    rule openssl-linked ( properties * )
    {
        if [ webcpp-target.target-of $(properties) ] in native wasip2 wasip3
        {
            return <library>/webcpp/third_party//openssl
                <define>WEBCPP_CRYPTO_HAS_OPENSSL=1 ;
        }
    }

    path-constant WEBCPP_CRYPTO_ROOT : . ;
    alias openssl : : : : <conditional>@openssl-linked
        <toolset>emscripten:<cflags>"-include \"$(WEBCPP_CRYPTO_ROOT)/src/openssl_on_emscripten.hpp\""
        <toolset>emscripten:<cxxflags>"-include \"$(WEBCPP_CRYPTO_ROOT)/src/openssl_on_emscripten.hpp\"" ;

The define is the alias's, not a usage requirement of the provider, which stays free of any
one library. The provider links libssl and libcrypto from one build directory per
configuration, with the link flags it gives, and the build's threading follows the program's
(`no-threads` under b2's default `threading=single`). On MSVC, wasip2 and wasip3 it builds
OpenSSL without assembly (section 3.3). Nothing is looked for on the machine, so OpenSSL has
no root to give, no stop for a missing OpenSSL and no shared link. The provider injects
nothing into upstream, so OpenSSL has no link-time canary: its layers are the compile-time
messages of section 4.3 and MSVC's `detect_mismatch`.

**Emscripten stops the compile.** webcpp builds no OpenSSL for Emscripten. There the alias
forces `src/openssl_on_emscripten.hpp` into every file the program compiles, C and C++, and its
`#error` says (a fixture of the superproject's Jamfile test fixes it, section 8.6):

    webcpp::crypto: /webcpp/crypto//openssl is named in a build for Emscripten, where webcpp does not build OpenSSL. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl, or name OpenSSL for native and WASI builds alone, by a condition in the project's requirements: project : requirements <conditional>@native-openssl ; rule native-openssl ( properties * ) { if ! <toolset>emscripten in $(properties) { return <library>/webcpp/crypto//openssl ; } } (without b2: link OpenSSL into native and WASI programs only)

**Botan is found** by `webcpp.external`, where `-sBOTAN_ROOT=<dir>` says, else in webcpp's
`.local/botan`, else in the system's directories (`tools/webcpp.jam:967-986`). OpenSSL was
found the same way until 2026-11-03, and the prototype measured the mechanism on it
(C-LIBS:273-304, :539-540, :630-639). Botan is linked shared, the default, or static by the
feature `webcpp-crypto-botan-link` (`shared static`, propagated, so that a program's request
reaches the library, and not free or incidental, so that the two builds get directories of
their own). A header cannot change how a program is linked, so the feature carries the choice
the decision of 2026-10-10 put in the config, and `config.hpp` only reads what the build says;
the owner confirmed the feature on 2026-10-11 (section 10). A static link on a root without the
static library stops the build, naming the file, and a wrong root stops it too.

**Botan not found stops a program's build** (DESIGN, 2026-10-11 "an external library a program
names and the machine lacks stops the build", AGENTS:726-732). A user's program that names
`/third_party//botan` has asked for it, so the build stops with the three ways out the owner
named: how to install it, `-sBOTAN_ROOT`, or another backend. The text is written with the
backend (increment 5) and its test fixes it; it reuses the "where it looked" part that
`webcpp.external-found` already writes (`tools/webcpp.jam:996-1032`), and holds no `$(`, which
b2 would expand. Only webcpp's own tests and examples are left out, with a message, as
chapter 9 says, and the CI requires them anyway with `webcpp-require-external=on`
(`tools/webcpp.jam:1016-1023`). The two cases are told apart by an incidental propagated
feature that webcpp's test rules (`webcpp.run` and its kin) and `webcpp.example` set on their
programs; any other program gets the stop. `/third_party//botan` also stops every WebAssembly
build (section 3.6).

### 4.5 One copy per program

webcpp's build of a C library is never linked together with another copy of it
(AGENTS:740-741). For libsecp256k1 that means trystero leaves its external libsecp256k1 in the
same change that moves it to `/third_party//secp256k1` (increment 3).

## 5. The default backend per target

The rule is the owner's: the smallest binary on the web, the fastest on servers (DESIGN,
2026-10-10 "webcpp::crypto's backends are the C libraries themselves"). `config.hpp` applies it
to the AEADs from `__EMSCRIPTEN__`, `__wasi__` and the `HAS` defines; the architecture does not
enter the choice, since BearSSL picks AES-NI and PCLMUL by CPUID inside its own code
(C-LIBS:338-340).

| Target | Default | The measured reason |
| --- | --- | --- |
| Emscripten | `backends::bearssl` | OpenSSL and Botan do not exist there. BearSSL's AES-GCM adds about 17.6 KB raw, 7.8 KB gzip, to a program at -Os (26,387 bytes against a baseline of 8,779; C-LIBS:177, :462) and runs at 146 to 160 MiB/s (C-LIBS:207-208) |
| wasip2, wasip3 | `backends::bearssl` | the default even with OpenSSL linked, which a program selects with `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` or names in the type; about 15.6 KB raw, 6.2 KB gzip, at -Os stripped (wasip2 59,887 against 44,264; wasip3 67,871 against 52,188; C-LIBS:180-182, :464-466), 147 to 160 MiB/s |
| native, OpenSSL linked | `backends::openssl` | the fastest on every architecture measured: x86-64 (AMD EPYC 7763) 3,333 to 3,509 MiB/s against BearSSL's AES-NI 2,373 to 2,496, about 1.4 times; Linux aarch64 2,821 to 2,825 against 92.4 to 92.5, about 30 times; macOS-15 (Apple M1) 5,571 to 5,908 against 90.1 to 94.1, about 62 times (CI run 38113681987, C-LIBS:416-426); Apple M5 Max 7,501 to 7,687 against 159 to 175, about 46 times (C-LIBS:199-211). Shared, it adds nothing to the executable beyond the system's libcrypto (4.87 MB, already on a server) |
| native, OpenSSL not linked | `backends::bearssl` | it builds everywhere from source; with AES-NI it is within 1.4 times of OpenSSL on x86-64, but it has no ARM hardware path and runs at 90 to 175 MiB/s there |

- With more than one AEAD backend linked, the default is the first linked in this order:
  natively `openssl`, `bearssl`, `botan`; Botan comes last because it needs exceptions. On
  wasip2 and wasip3 the default is BearSSL, OpenSSL linked or not; on Emscripten BearSSL is the
  only AEAD backend (Botan is native only for now, section 3.6).
- The table's OpenSSL figures were measured on the runners' own OpenSSL, with its assembly
  (C-LIBS:416-426). The source-built 3.5.9 is not measured until the throughput lane, which
  comes after version 0.1; on MSVC, wasip2 and wasip3 it has no assembly (section 3.3).
- A program selects another with `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>`, and names a
  backend at any use with the template argument (`gcm<aes256<backends::bearssl>>`).
- The default backend is the AEADs'. The block cipher has none of its own: on WebAssembly a
  program names `aes256<backends::ctaes>` (section 2.3).
- The tables of libsecp256k1 follow the same rule, read with the owner's mission entry: small
  on Emscripten, the web; large on wasip2 and wasip3, the server and cloud targets, and
  natively (section 3.5).
- **The documentation warns** on the backends' page and on the page of `gcm`: natively without
  OpenSSL, BearSSL is slow on ARM, because BearSSL has no ARMv8 AES or PMULL code; a server on
  ARM names `/webcpp/crypto//openssl`. The page gives no factor: the slowdown is not measured
  on the source-built OpenSSL until the throughput lane (deferred).

## 6. Entropy (the owner, 2026-10-11)

The function that gets entropy always takes the safest path, and its code is simple enough to
be read and checked by anyone, and documented (DESIGN, 2026-10-11 "entropy, safest and readable").

- **One source per platform, the operating system's or the browser's:**
  - `getentropy` on Linux, macOS and WASI (wasmtime's `random_get`, the host's generator);
  - on Emscripten, `getentropy` becomes `crypto.getRandomValues` in the browser and in node
    (musl's `getentropy.c`, then `__wasi_random_get`, then `libwasi.js`'s `randomFill`,
    `src/lib/libwasi.js:626-629`);
  - `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` on Windows (learn.microsoft.com,
    `bcrypt.h`); MSVC has no `getentropy`, and `/webcpp/crypto//crypto` adds `bcrypt` to its
    usage requirements there.
- **No user-space generator in between.** Every call reads the source. There is no cache and
  no DRBG of webcpp's or of a backend's.
- **No weaker fallback, ever.** No clock, no `rand`, no `rdrand` alone, no address.
  - When the source fails, `fill` returns `entropy_unavailable` and zeroes `out`, so it fills
    nothing the caller may use.
  - An audio worklet, which has no `crypto`, gets `errc::not_supported`.
- **The whole function fits on one screen.**
  - It asks in pieces of at most 256 bytes, the limit of `getentropy`: POSIX.1-2024's
    `GETENTROPY_MAX`, wasi-libc's `EIO` past 256 (`libc-bottom-half/sources/getentropy.c:7-10`),
    Emscripten's musl (`system/lib/libc/musl/src/misc/getentropy.c:15-22`). Windows keeps the
    same loop, although one call could take up to a `ULONG`.
  - It checks the result of every call; trystero's browser backend loops today but ignores
    the result (trystero `browser/backend.hpp:994-999`), which is not repeated.
  - Each platform's branch is a few lines, with a comment naming the documented contract it
    relies on (the man page, Microsoft's page, the WHATWG Web Crypto section, the Emscripten
    source above).
  - Its includes and calls say why after `lint-world:`, since entropy is its purpose
    (AGENTS:641-649, :851).
- **The backends use the same source.**
  - BearSSL is compiled with `BR_USE_GETENTROPY=1` (and `_DEFAULT_SOURCE`, and on macOS
    `-include sys/random.h`), its `rdrand` seeder disabled (`BR_RDRAND=0`) and its
    `/dev/urandom` fallback too (`BR_USE_URANDOM=0`, `src/rand/sysrng.c:148-162`). On the
    x86-64 runner of 2026-10-11 it had picked `rdrand` by itself (C-LIBS:428-429).
  - On Windows BearSSL's only seeder would be `CryptGenRandom`; it is compiled out, and
    BearSSL's own seeder there is absent (section 4.1), since `random_device` uses
    `BCryptGenRandom` and no backend chooses its entropy (a ruling, LEDGER:27; section 10). A
    program that uses BearSSL's C PRNG directly seeds it from `random_device`, as the
    documentation shows.
  - libsecp256k1's context is randomized from `random_device` (section 2.8); its ECDSA nonce is
    RFC 6979's, and BIP340's auxiliary randomness is the caller's.
  - webcpp never calls OpenSSL's `RAND_bytes` or a Botan RNG. A backend never chooses its own
    entropy source.
- **Tested** (section 8.4).

## 7. Using webcpp::crypto in your Jamfile

This is the section of the library's documentation for its users. Every Jamfile line and every
printed result in it was built and run with b2 on native (clang-darwin), clang-wasip2,
clang-wasip3 and Emscripten (C-LIBS:527-542, PROBES `jam/app/<case>/Jamfile`), except the new
error texts of sections 3.7, 4.3 and 4.4, and the memory option of section 7.3, whose OpenSSL
calls PROBE-R3 ran natively without b2 and whose interface is not built yet. The C++ of the
programs follows section 2's interface; the probes ran the same programs on the prototype's
sketch (`jam/crypto/include`), whose `seal` wrote the tag into a span, whose key type was `schnorr_key`, and whose ctaes
backend was a free function, `backends::ctaes::aes256_encrypt` (`jam/app/ctaes/block.cpp`).

### 7.1 The rule

webcpp::crypto is header-only, and every algorithm runs on a backend: a C library that webcpp
builds from its original sources (BearSSL, libsecp256k1, ctaes), that the superproject builds
from source (OpenSSL, static), or that webcpp finds on the system (Botan). A program names two
things in its Jamfile: `/webcpp/crypto//crypto`, and the library of each backend it uses,
`/third_party//<backend>`, or for OpenSSL crypto's alias `/webcpp/crypto//openssl`. Nothing
is linked that the program does not name, and a backend built from source (BearSSL,
libsecp256k1, ctaes) named but not called adds no byte to the program; a shared Botan named
still adds its dependency at run time (section 4.2).

A program names a backend in C++ by including its header,
`<webcpp/crypto/backends/<name>.hpp>` (`backends/ctaes.hpp`, `backends/openssl.hpp`), and
using its tag type, `backends::<name>`, as the template argument. The default backend needs
no include of its own: `<webcpp/crypto/ciphers/aes256_gcm.hpp>` brings it.

    exe <program> : <sources> /webcpp/crypto//crypto /third_party//<backend> ;

A program that only draws random bytes (`random_device`) or compares tags (`equal`) names
`/webcpp/crypto//crypto` alone.

### 7.2 A web application, on Emscripten, with BearSSL

`Jamfile`:

    exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;

Build and run:

    b2 toolset=emscripten
    node app.js            # prints: aes256_gcm ok backend=bearssl

The same Jamfile builds for WASI (`b2 toolset=clang-wasip2`, run with `wasmtime app.wasm`;
`b2 toolset=clang-wasip3`, run with `wasmtime -W component-model-async=y -S p3=y app.wasm`)
and natively (`b2`). On WebAssembly BearSSL is the backend of encryption, and the default.

`app.cpp`:

```cpp
// Seals one zero block under AES-256-GCM with the default backend and checks it against the
// NIST test vector (key, IV and plaintext all zero: GCM test case 14), then opens it again.
#include <webcpp/crypto/ciphers/aes256_gcm.hpp>
#include <webcpp/crypto/equal.hpp>

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 16> data{};
    std::array<std::uint8_t, 16> const cipher{0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
                                              0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18};
    std::array<std::uint8_t, 16> const expected_tag{0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
                                                    0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19};
    auto aead = webcpp::crypto::ciphers::aes256_gcm::make(key);
    if (!aead) {
        return 1;
    }
    auto const tag = aead->seal(iv, {}, data);
    bool const sealed = tag && data == cipher && webcpp::crypto::equal(*tag, expected_tag);
    bool const opened = aead->open(iv, {}, data, expected_tag) && data == std::array<std::uint8_t, 16>{};
    auto const name = webcpp::crypto::ciphers::aes256_gcm::backend_type::name;
    std::printf("aes256_gcm %s backend=%.*s\n", sealed && opened ? "ok" : "FAILED",
                static_cast<int>(name.size()), name.data());
    return sealed && opened ? 0 : 1;
}
```

### 7.3 A server, natively, with OpenSSL

`Jamfile`:

    exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ;

Build and run (the same `app.cpp`):

    b2
    ./app                  # prints: aes256_gcm ok backend=openssl

`/webcpp/crypto//openssl` links the superproject's OpenSSL 3.5.9, built from source and
static; nothing is looked for on the machine (section 4.4).

Natively, OpenSSL is the default backend whenever it is linked, even when BearSSL is linked too.
To make BearSSL the default with both linked:

    exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl /third_party//bearssl
      : <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL ;

OpenSSL builds natively, on wasip2 and on wasip3, and never for Emscripten, where the build
stops and says to name BearSSL (section 4.4). On wasip2 and wasip3 the default stays BearSSL
even with OpenSSL linked: a WASI program that wants OpenSSL names it in the type,
`ciphers::gcm<ciphers::aes256<backends::openssl>>`, or defines
`WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`. On MSVC, wasip2 and wasip3 the source-built OpenSSL has
no assembly: its AES is table-based and its GHASH 4-bit, neither constant time, and there is
no AES-NI; a program for which timing attacks matter there defines
`WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`, or names
`ciphers::gcm<ciphers::aes256<backends::bearssl>>`. On an ARM server without OpenSSL, BearSSL
is slow, since it has no ARMv8 AES or PMULL code (section 5): name OpenSSL there.

**OpenSSL's allocations in your own memory resource.** By default OpenSSL allocates with its
own `malloc`, outside any allocator of the program's (section 3.3). To send every allocation
OpenSSL makes in the process to a `std::pmr::memory_resource` of the program's, do the three
things below; section 3.7 says why each is needed.

1. Give the option, and OpenSSL with it, to every target of the program, in the requirements
   of the program's project, so that its own libraries get both too. The option is for native
   builds only: `config.hpp` stops a target that has the define without OpenSSL, or that is
   built for WebAssembly, with an `#error` (section 7.7). Carrying `/webcpp/crypto//openssl` in
   the same requirements is what keeps every target that gets the define linked with OpenSSL,
   a tool or a library that only uses `random_device` or `equal` included (this form is not
   yet built with b2; section 7's introduction):

       project app
         : requirements <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE
                        <library>/webcpp/crypto//openssl ;
       exe app : app.cpp /webcpp/crypto//crypto ;

   A WebAssembly build of the same program is a project of its own, in a directory of its own
   with its own Jamfile, which has neither the define nor OpenSSL and names
   `/third_party//bearssl` (section 7.2): the requirements above cannot be built for
   WebAssembly, since `/webcpp/crypto//openssl` stops an Emscripten build (section 4.4).

2. Define `webcpp::crypto::openssl_memory_resource()` once in the program. Without the
   definition the program does not link. It returns a resource that is thread-safe and is
   **never destroyed**: create it with `new` and never delete it, as below. OpenSSL frees
   through it after `main` returns, and on Windows with a shared OpenSSL after every static
   object of the program has been destroyed, so a static resource object, or a local variable
   of `main`, would be used after its destruction.
3. Call `webcpp::crypto::backends::openssl::install_memory_functions()` as the first statement
   of `main`, before any thread starts and before anything uses OpenSSL, and stop when it
   returns an error.

`app.cpp`:

```cpp
// Sends every allocation of OpenSSL's to the program's resource, then seals and opens one
// block with AES-256-GCM on OpenSSL.
#include <webcpp/crypto/ciphers/aes256_gcm.hpp>
#include <webcpp/crypto/backends/openssl.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory_resource>

// Declared by webcpp::crypto under WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE, defined once here.
// Never destroyed: OpenSSL frees through it after main returns, and on Windows after every
// static destructor of the program. The synchronized pool serves OpenSSL on any thread.
std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept {
    static auto* const resource = new std::pmr::synchronized_pool_resource;
    return *resource;
}

int main() {
    auto const installed = webcpp::crypto::backends::openssl::install_memory_functions();
    if (!installed) {
        std::fprintf(stderr, "%s\n", installed.error().message().c_str());
        return 1;
    }
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 16> data{};
    auto aead = webcpp::crypto::ciphers::aes256_gcm::make(key);
    if (!aead) {
        std::fprintf(stderr, "%s\n", aead.error().message().c_str());
        return 1;
    }
    auto const tag = aead->seal(iv, {}, data);
    bool const ok = tag && aead->open(iv, {}, data, *tag) && data == std::array<std::uint8_t, 16>{};
    std::printf("aes256_gcm %s backend=openssl\n", ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
```

Build and run: `b2`, then `./app`, which is to print `aes256_gcm ok backend=openssl`.

What to know:

- The resource receives every allocation of the OpenSSL the program links, in the whole
  process: Beast's TLS and libdatachannel's DTLS too, not only webcpp::crypto's.
- Nothing else in the process may call `CRYPTO_set_mem_functions`. If another library has
  called it before, `install_memory_functions()` returns `memory_functions_refused`; if it
  calls it after the install, the next `make` returns `memory_functions_not_installed`, and
  only when that call has finished before the `make`.
- If `install_memory_functions()` returns `memory_functions_refused` although it is the first
  statement of `main`, something used OpenSSL before `main`: a static object of another
  library. OpenSSL then keeps its own `malloc` for the rest of the process.
- A `make` that returns `memory_functions_not_installed` means that the install was not
  called, or that something replaced OpenSSL's functions after it.
- The program's code that uses webcpp's OpenSSL backend lives in one module: a DLL or a shared
  library of the program's that includes the backend's header has its own copy of webcpp's
  functions, and its `make` returns `memory_functions_not_installed`.
- When the resource cannot allocate: during the first `make` of the process, OpenSSL's own
  initialisation fails, and OpenSSL stays unusable for the rest of the process (`make` gives
  `backend_failure` from then on); later, `make` gives `not_enough_memory` or
  `backend_failure`. `seal` and `open` allocate nothing. A resource with a fixed size needs
  room for the first `make`: about 6,100 allocations, 4,621 of them still live afterwards,
  with OpenSSL 3.6.4 on macOS (section 3.7). Without exceptions, a resource that throws ends
  the process; one that returns null reports the failure.
- Two allocations stay outside the resource, because no hook of OpenSSL's reaches them: the C
  library's `FILE` and its buffer while OpenSSL reads its configuration file at
  initialisation (one or two blocks, by C library, freed when the file closes), and OpenSSL's
  secure heap, if the program turns it on with `CRYPTO_secure_malloc_init`.
- Botan has no such hook: with Botan, allocations stay outside the program's allocator
  (section 3.6).

### 7.4 A program that signs with libsecp256k1

`Jamfile`:

    exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1 ;

On Emscripten the small precomputed tables are the default; on wasip2, wasip3 and natively
the large ones (section 3.5). They can be chosen: the small ones make the program about 1 MB
smaller (122 KB instead of 1.2 MB natively), for verifications about a third slower:

    exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1
      : <webcpp-secp256k1-tables>small ;

Build and run: `b2`, then `./sign`, which prints `schnorr ok` (on every target).

`sign.cpp`:

```cpp
// Signs a message with BIP340 through libsecp256k1, verifies it, and refuses a changed one.
#include <webcpp/crypto/curves/secp256k1.hpp>

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    namespace secp256k1 = webcpp::crypto::curves::secp256k1;
    std::array<std::uint8_t, 32> secret{};
    secret.fill(0x42);
    std::array<std::uint8_t, 32> message{};
    message.fill(0x17);
    std::array<std::uint8_t, 32> const aux{};
    auto key = secp256k1::keypair::make(secret);
    bool ok = false;
    if (key) {
        auto const signature = key->sign_schnorr(message, aux);
        auto const public_key = key->xonly_public_key();
        ok = signature && public_key && secp256k1::verify_schnorr(*signature, message, *public_key);
        message[0] ^= 1;
        ok = ok && !secp256k1::verify_schnorr(*signature, message, *public_key);
    }
    std::printf("schnorr %s\n", ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
```

A program that both encrypts and signs names both libraries:

    exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl /third_party//secp256k1 ;

### 7.5 A block cipher, with ctaes

`Jamfile`:

    exe block : block.cpp /webcpp/crypto//crypto /third_party//ctaes ;

Build and run: `b2`, then `./block`, which prints `ctaes aes256 ok` (on every target,
C-LIBS:535). The default backend is the AEADs' and has no block cipher on WebAssembly, so the
program names ctaes and includes its header (section 2.3). A raw block cipher is not
encryption by itself; a program that encrypts messages uses `gcm` (section 2.4).

`block.cpp`:

```cpp
// Encrypts the FIPS-197 appendix C.3 block with AES-256 on ctaes, checks it, and decrypts it again.
#include <webcpp/crypto/ciphers/aes.hpp>
#include <webcpp/crypto/backends/ctaes.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

int main() {
    std::array<std::uint8_t, 32> key{};
    for (std::size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<std::uint8_t>(i);
    }
    std::array<std::uint8_t, 16> const plain{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    std::array<std::uint8_t, 16> const expected{0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
                                                0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89};
    std::array<std::uint8_t, 16> block = plain;
    auto aes = webcpp::crypto::ciphers::aes256<webcpp::crypto::backends::ctaes>::make(key);
    bool const ok = aes && aes->encrypt(block) && block == expected && aes->decrypt(block) &&
                    block == plain;
    std::printf("ctaes aes256 %s\n", ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
```

### 7.6 The backends

| Name in the Jamfile       | Library                                                 | Targets                            | Options                                                                                                                                                                                            | What it exports                                                                                                                                                                                                                                                      |
| ------------------------- | ------------------------------------------------------- | ---------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `/third_party//bearssl`   | BearSSL, built from source                              | native, emscripten, wasip2, wasip3 | none                                                                                                                                                                                               | `gcm<aes128/192/256<backends::bearssl>>` (`aes256_gcm` by default on WebAssembly, and natively without OpenSSL), `chacha20_poly1305<backends::bearssl>`; BearSSL's whole C API through `<bearssl.h>`                                                                 |
| `/webcpp/crypto//openssl` | OpenSSL 3.5.9's libcrypto, built from source, static    | native, wasip2, wasip3             | none; `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` with `<library>/webcpp/crypto//openssl` in the project's requirements, natively only (refused on wasip2, wasip3 and Emscripten), OpenSSL's allocations in the program's resource (section 7.3)                                                                             | `aes128/192/256<backends::openssl>`, `gcm<...>` (`aes256_gcm` by default natively when linked; on MSVC, wasip2 and wasip3 without assembly), `chacha20_poly1305<backends::openssl>`; OpenSSL's C API through `<openssl/*.h>`                                                                                      |
| `/third_party//secp256k1` | libsecp256k1 v0.8.0, built from source, every module on | native, emscripten, wasip2, wasip3 | `<webcpp-secp256k1-tables>auto` (default: small on Emscripten, large elsewhere), `small` or `large`                                                                                                | `webcpp::crypto::curves::secp256k1::keypair` (BIP340, ECDSA, ECDH), `verify_schnorr`, `verify_ecdsa` in `<webcpp/crypto/curves/secp256k1.hpp>`; the C API of `<secp256k1.h>` and its modules: ecdh, recovery, extrakeys, schnorrsig, musig, ellswift, silentpayments |
| `/third_party//ctaes`     | ctaes as Bitcoin Core v31.1 ships it, built from source | native, emscripten, wasip2, wasip3 | none                                                                                                                                                                                               | `aes128/192/256<backends::ctaes>` in `<webcpp/crypto/backends/ctaes.hpp>`: the AES block cipher alone, constant time, no mode; the C API of `<ctaes.h>`, `AES128/192/256_init`, `_encrypt`, `_decrypt`                                                               |
| `/third_party//botan`     | Botan 3, found on the system                            | native only, for now               | `<webcpp-crypto-botan-link>shared` (default) or `static`; `-sBOTAN_ROOT=<dir>`                                                                                                                     | `aes128/192/256<backends::botan>`, `gcm<...>`, `chacha20_poly1305<backends::botan>`, only in a build with exceptions; Botan's C++ API                                                                                                                                |

### 7.7 Every message, and what to do

At compile time, the first error is the one to read: the lines after it
(`use of undeclared identifier 'webcpp'`) come from it and go away with it.

| Message | Fix |
| --- | --- |
| `webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; ...` | Add `/third_party//bearssl` to the program's sources. |
| `webcpp::crypto: no backend is linked. In the program's Jamfile, add /webcpp/crypto//openssl (OpenSSL built from source) or /third_party//bearssl (built from source) beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ; ...` | Natively, on wasip2 or on wasip3: add `/webcpp/crypto//openssl`, or `/third_party//bearssl`. |
| `webcpp::crypto: the backend BearSSL is used and not linked. In the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, or, with /webcpp/crypto//openssl linked, select OpenSSL with <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL. ...` | Add `/third_party//bearssl`, or, with OpenSSL linked, select OpenSSL. The same message names ctaes or Botan for those; OpenSSL's says to add `/webcpp/crypto//openssl`. |
| `webcpp::crypto: <webcpp/crypto/curves/secp256k1.hpp> needs libsecp256k1, which is not linked. In the program's Jamfile, add /third_party//secp256k1 beside /webcpp/crypto//crypto. ...` | Add `/third_party//secp256k1`. |
| `webcpp::crypto: ciphers::gcm<ciphers::aes256<backends::ctaes>>: ctaes exports the AES block cipher alone, ...` | Use `gcm<aes256<>>` with BearSSL, OpenSSL or Botan linked. |
| `webcpp::crypto: ciphers::aes256<backends::bearssl>: BearSSL exports AES only inside its modes, ...` | Name `aes256<backends::ctaes>` (or OpenSSL's or Botan's), include its header, and add its library, as in section 7.5. |
| `webcpp::crypto: /webcpp/crypto//openssl is named in a build for Emscripten, where webcpp does not build OpenSSL. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl, or name OpenSSL for native and WASI builds alone, ...` | Replace `/webcpp/crypto//openssl` with `/third_party//bearssl`, or name OpenSSL only for native and WASI builds, by the condition the message gives (section 4.4). |
| `webcpp::crypto: OpenSSL is not built for Emscripten. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl.` (without b2) | The same. |
| `webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL selects OpenSSL, which is not built for Emscripten. ...` | Remove that define. |
| `webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL are defined. Keep one.` | Keep one. |
| `webcpp::crypto: the backend Botan needs exceptions, and this program is built without them. ...` | Build with exceptions, or name BearSSL or OpenSSL. |
| `/third_party//botan: ...` on WebAssembly (from b2), or `webcpp::crypto: Botan is a native backend ...` (without b2) | Name `/third_party//bearssl`: Botan is native only for now. |
| Botan not found for a program, or a wrong `-sBOTAN_ROOT=<dir>` (from b2; the texts come with increment 5, section 4.4) | Install Botan, or give `-sBOTAN_ROOT=<dir>`, the directory of its `include/` and `lib/`, or use BearSSL or OpenSSL (section 4.4). |
| `webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE configures OpenSSL, a native backend, and this program is built for WebAssembly. ...` | Remove `<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` from the WebAssembly build: give that build a project of its own, without the define and OpenSSL, naming `/third_party//bearssl` (section 7.3). On WebAssembly the backend is BearSSL, which allocates nothing. |
| `webcpp::crypto: WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE routes OpenSSL's allocations, and OpenSSL is not linked. ...` | A target has the define and not OpenSSL: put `<library>/webcpp/crypto//openssl` in the same project requirements as the define (section 7.3), or remove the define. |
| `Undefined symbols ...: "webcpp::crypto::openssl_memory_resource()"`, at link time (ld64, PROBE-R3; GNU ld and MSVC's `link.exe` name the same function in their own forms, which the link test of section 8.6 fixes) | The program sets `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, uses the OpenSSL backend (any `make`, or the install) and does not define the function: define `std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept` once, returning a resource that is never destroyed, as in section 7.3. |
| `LNK2038: mismatch detected for 'webcpp_crypto_openssl_memory'` (MSVC) | Some files of the program were compiled with `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` and some without: give the define in the requirements of the program's project, with OpenSSL beside it, `project app : requirements <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE <library>/webcpp/crypto//openssl ;`, so that every target of the program gets both (natively only; section 7.3). |

At run time, two errors of `webcpp.crypto` concern the memory option, and their messages say
what to do (section 3.7):

| Error and message | Fix |
| --- | --- |
| `memory_functions_refused`, from `install_memory_functions()`: "OpenSSL's memory functions cannot be installed: OpenSSL has already allocated, or another party installed its own. ..." | Make the call the first statement of `main`. If it is already, a static object of another library used OpenSSL, or called `CRYPTO_set_mem_functions`, before `main`: remove that use, or drop the option. |
| `memory_functions_not_installed`, from `make` on OpenSSL: "OpenSSL's memory functions are not webcpp::crypto's. ..." | Call `install_memory_functions()` first in `main`, and check that nothing else in the process calls `CRYPTO_set_mem_functions`. If the `make` is in a DLL or a shared library of the program's, move the code that uses the OpenSSL backend into the module that calls the install (section 3.7). |
| `backend_failure` from every `make` on OpenSSL, under the option, after the resource failed once early in the process | The resource could not allocate during OpenSSL's first initialisation, which OpenSSL does not retry: the process cannot use OpenSSL any more. Give the resource room for the first `make` (section 7.3) and restart. |

At link time, a symbol whose name is the instruction reports a backend library that is
missing, or that is of another commit or configuration than the headers. With b2 this cannot
happen; it guards builds made by other means. Its name is
`webcpp_crypto_<name>_<commit>_<configuration>__add_third_party_<name>_to_your_link`, as the
linkers of section 4.3 print it.

Fix: link the library through b2 (`/third_party//<name>` in the Jamfile). Built by other
means, the library must be the pinned commit (the canary's `<commit>`), built with webcpp's
`src/canary_<name>.c`, in the configuration the canary names: for BearSSL `getentropy`
(compiled with `BR_USE_GETENTROPY=1`, `_DEFAULT_SOURCE`, `BR_RDRAND=0`, `BR_USE_URANDOM=0`,
and on macOS `-include sys/random.h`), on Windows `noseeder`; for libsecp256k1 `w15c11x6`
(the large tables) or `w2c2x5` (`ECMULT_WINDOW_SIZE=2`, `COMB_BLOCKS=2`, `COMB_TEETH=5`, with
`WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES` in the program). LNK2038 means two files of the program,
or the program and the library, were compiled with different settings.

## 8. Testing

Across the levels, every lane runs on every target: native (Linux x86-64 with gcc and clang,
Linux arm64, macOS arm64, Windows with MSVC 14.3 and 14.5), wasip2 and wasip3 under wasmtime,
and Emscripten in both suspension variants (`jspi` and `asyncify`) under node and in Chrome; a
backend's lanes run on the targets it has (section 3.1). Every suite runs with exceptions on
and off, except Botan's, which runs with them on and has a headers-alone test without them
(DESIGN, 2026-10-10 "every variant tested", AGENTS:796-821).

The levels are the owner's three (DESIGN, 2026-10-10 "CI in three levels"): fast on every push
and pull request (one native compiler and wasip2 in the default variant, one native lane
without exceptions, the lint, the docs), nightly on main (every target and compiler in the
default variant, whole suites without exceptions natively, sanitizers, the heavy lanes),
release when webcpp is released (every target times every variant). Each lane of this section
has its level (CR-REVIEW M11):

| Lane | Fast | Nightly | Release |
| --- | --- | --- | --- |
| Test vectors and guards (8.1), BearSSL against OpenSSL (8.2), entropy (8.4), allocation (8.5), every message (8.6) | on the fast set | every target and compiler | every variant |
| The C libraries' own suites (8.1): BearSSL's 88 tests take 28 to 46 s per target (C-LIBS:98-101); libsecp256k1's `tests`; ctaes's `test.c` | ctaes's `test.c` only | every target | every variant |
| Cross-backend agreement with Botan (8.2) | | native | native, every variant with exceptions |
| Constant time, deterministic (8.3, Valgrind, MemorySanitizer, `ctime_tests`) | | Linux x86-64 and arm64 | the same |
| Constant time, statistical (8.3, dudect's method) | | native and wasmtime | the same |
| Chrome (8.1's WebCrypto oracle, 8.4's audio worklet, 8.8's throughput in Chrome) | | yes | yes |
| The unused library (8.7) | | every target | every target |
| OpenSSL's memory option (8.5): Linux, macOS and Windows with MSVC 14.3 and 14.5, OpenSSL linked shared and static, exceptions on and off; the MSVC shared lane is the only one where `OPENSSL_cleanup` runs at the DLL's unload | Linux, shared | every platform and link | every variant |
| Sizes and throughput (8.8) | | yes, into the report | yes, into the release's documentation |

### 8.1 Test vectors

Every case runs on every backend that has the algorithm, sealed and opened one shot, and,
for GCM, sealed incrementally as well, the data in pieces of every size from 1 byte up to the
whole, and the result opened one shot (there is no incremental `open`, section 2.4):

- **AES-GCM:**
  - the 18 test cases of the GCM specification (McGrew and Viega), all of them, since keys of
    24 bytes and IVs of 8 and 60 bytes are accepted (REVIEW, X-I2);
  - all 316 of Wycheproof's AES-GCM tests (`C2SP/wycheproof` main 12fd3aaf,
    `testvectors_v1/aes_gcm_test.json`; 108 with 128-bit keys, 103 with 192, 105 with 256; 229
    valid, 87 invalid; every tag 128 bits; PROBE-1011 count, confirmed by CR-REVIEW I1 on
    `webrtc-crypto/wp_aes_gcm.json`): the 307 with an IV of 1 to 128 bytes run through `seal`
    and `open`, the 226 valid ones matching and the 81 invalid ones failing with
    `authentication_failed` and a zeroed `data`; the 6 with an empty IV (all invalid) must fail
    with `invalid_iv_size`; and the 3 with a 257-byte IV, tcId 268, 272 and 276, valid in
    Wycheproof, are expected refusals, `invalid_iv_size` with `data` untouched, by the cap of
    section 2.4 (LEDGER:22);
  - NIST CAVP's `gcmtestvectors`, the cases with 128-bit tags (their IVs, 8 to 1,024 bits,
    all inside the cap); the cases with shorter tags are skipped, because the interface offers
    only 16-byte tags, as trystero and WebCrypto's default use.
- **The AES block cipher:** FIPS-197's appendix C examples (the probe ran C.3 on ctaes,
  C-LIBS:535) and NIST's AESAVS known-answer tests, on ctaes, OpenSSL and Botan.
- **ChaCha20-Poly1305:** RFC 8439's section 2.8.2 and appendix A.5 vectors, and Wycheproof's
  `chacha20_poly1305_test.json` at the same pin, all 325 of its tests (CR-PROBES
  `wp_chacha20_poly1305_test.json`): the 316 with a 12-byte nonce run through `seal` and
  `open` (256 valid, 60 invalid that fail with `authentication_failed`), and the 9 with a
  nonce of 0, 8, 11, 13, 14, 16, 20, 24 or 32 bytes, all invalid, must fail with
  `invalid_iv_size` (CR-REVIEW M10).
- **secp256k1** (LEDGER:28; CR-REVIEW I7). Wycheproof's general ECDSA file gives DER
  signatures and counts high-S signatures valid, and its ECDH file gives the raw x-coordinate
  as the shared secret, neither of which fits section 2.8's interface. So:
  - BIP340's `test-vectors.csv`, through `sign_schnorr` and `verify_schnorr`;
  - ECDSA: Wycheproof's Bitcoin variant, `testvectors_v1/ecdsa_secp256k1_sha256_bitcoin_test.json`
    at the same pin, which applies Bitcoin's low-S rule (464 tests, 162 valid, 302 invalid).
    The test hashes each message with Boost.Hash2's SHA-256, converts the DER signature
    through libsecp256k1's `secp256k1_ecdsa_signature_parse_der` and
    `secp256k1_ecdsa_signature_serialize_compact`, and calls `verify_ecdsa` on the compact
    form. Measured through the C API on libsecp256k1 v0.8.0 (CR-PROBES `secp/wp_secp.out`):
    the 162 valid verify; of the 302 invalid, 190 are refused by the DER parser, so they test
    an encoding webcpp's compact interface does not take and are skipped with that reason,
    and the other 112 fail verification, which `verify_ecdsa` must report as
    `invalid_signature`;
  - ECDH: through libsecp256k1's C API, `secp256k1_ecdh` with a hash function that copies x,
    on Wycheproof's `ecdh_secp256k1_test.json` (752 tests, ASN.1 public keys) and
    `ecdh_secp256k1_webcrypto_test.json` (496 tests, JWK public keys). No
    `ecdh_secp256k1_ecpoint_test.json` exists at the pin. The test takes the SEC1 point out of
    a key in the standard secp256k1 encoding and skips the others with the reason that they
    test ASN.1 or a JWK curve label, which are not webcpp's input. Measured (CR-PROBES
    `secp/wp_secp.out`): ASN.1, 474 agree with Wycheproof's shared secret (the 473 valid and 1
    acceptable), 22 are refused by libsecp256k1 (21 invalid, 1 acceptable), 256
    are ASN.1 cases skipped (228 acceptable, 28 invalid); JWK, 473 agree, 20 invalid refused, 2
    skipped, and tcId 490, a valid secp256k1 point labelled `P-256`, skipped for its label.
    Every key libsecp256k1 refuses there must give `invalid_public_key` through `ecdh`
    (section 2.8);
  - webcpp's `ecdh` itself: agreement between the two parties, `a.ecdh(B) == b.ecdh(A)` on
    random key pairs, and equality with the SHA-256 of the compressed shared point that the C
    API computes with its default hash function, on Wycheproof's valid ECDH cases;
  - ECDSA's encodings and the x-only tweak (section 2.8): every signature of `sign_ecdsa`
    through `to_der` and back through `from_der`, every refusal of `to_der`, the strict decoder
    on Wycheproof's Bitcoin ECDSA file (counted per flag), and `normalize_ecdsa` on a high-S
    signature that `verify_ecdsa` refuses; BIP341's `wallet-test-vectors.json`, the test
    computing each TapTweak with libsecp256k1's tagged hash and passing it: `tweak_xonly` gives
    each `scriptPubKey` case's `tweakedPubkey` (7 cases), a key pair tweaked by
    `keypair::tweak_xonly` signs each `keyPathSpending` sighash into the vector's witness (7
    inputs), and a tweak of 32 bytes of 0xff gives `invalid_secret_key` to both, the key pair
    wiped and empty;
  - libsecp256k1's own test program on webcpp's build, on every target (AGENTS:742-743).
    `src/tests.c` includes `secp256k1.c` itself (`src/tests.c:21`), so it recompiles the
    library with webcpp's flags and defines rather than testing the archive webcpp links; the
    archive is tested by the vectors above (CR-REVIEW M9).
- **The C libraries' own suites on webcpp's build,** on every target: BearSSL's
  `test/test_crypto.c` (its 88 tests passed natively, on wasip2 and on Emscripten against the
  unity build, C-LIBS:98-101), libsecp256k1's `tests`, ctaes's `test.c` (from Bitcoin Core
  v31.1's `src/crypto/ctaes`, section 3.4).
- **Tampering:** every byte of ciphertext, tag and additional data changed in turn fails with
  `authentication_failed` and leaves `data` zeroed; every guard of section 2.4 has a test, one
  shot and incremental, and leaves `data` untouched.
- **The incremental seal's machine** (section 2.4): a second `begin_seal` during a message,
  `update` and `result` with no message, a second `result`, an error in the middle of a
  message (`message_too_long` from an `update`) that ends it, a move during a message, each
  with its `invalid_state` or its error, and a fresh `begin_seal` that works after each.
- **Moved-from objects:** every function of a moved-from `aes128/192/256`, `gcm`,
  `chacha20_poly1305` and `curves::secp256k1::basic_keypair` returns `invalid_state`, one test per
  type and backend (section 2.4).
- **WebCrypto as an outside oracle:** in node and in Chrome, WebCrypto seals the same AES-GCM
  cases with keys of 16 and 32 bytes, and also random messages under 16-byte IVs, the ones
  trystero exchanges with Trystero's JavaScript peers (CR-REVIEW M14), and the results match
  byte for byte.

### 8.2 Cross-backend agreement

Every backend against every other, on the same inputs: random keys, IVs of 12 bytes and of
random lengths from 1 to 64, additional data of 0 to 64 bytes, messages of 0 to 4,096 bytes
and across every multiple of 16 near those sizes, from a seeded generator so that a failure
replays. What one seals each other opens, and the ciphertexts and tags are byte-identical.

| Algorithm | Backends compared | Where |
| --- | --- | --- |
| AES-GCM | BearSSL, OpenSSL, Botan | natively all three; on wasip2 and wasip3 BearSSL and OpenSSL; on Emscripten BearSSL alone, against the vectors |
| the AES block cipher | ctaes, OpenSSL, Botan | natively all three; on wasip2 and wasip3 ctaes and OpenSSL; on Emscripten ctaes alone, against the vectors |
| ChaCha20-Poly1305 | BearSSL, OpenSSL, Botan | as AES-GCM |

Botan is native only for now (LEDGER:29), and OpenSSL is not built for Emscripten, so on
Emscripten each algorithm is compared among the backends built there, which is one; the
vectors and WebCrypto's oracle (section 8.1) stand in for a second backend. The IV lengths of
1 to 64 bytes are inside the cap of section 2.4.

On x86-64, BearSSL runs once with its AES-NI and PCLMUL path and once forced onto
`aes_ct64` and `ghash_ctmul64` (a test seam of the backend), so that both of its code paths
are compared.

### 8.3 Constant time

- **Deterministic, natively:** the key, the plaintext and the tag are marked uninitialised
  (Valgrind memcheck's client requests; MemorySanitizer's poisoning for the code webcpp
  compiles), and any branch or memory index on them fails the run, in the style of ctgrind,
  as Botan checks its GHASH (`ghash.cpp:74`, REVIEW:52). It covers `equal`, `seal` and `open`
  of every backend, and the secp256k1 signing, on Linux x86-64 and arm64 at the nightly
  level. `equal` is checked in a test program compiled at -O2 and -Os, since it compiles in
  the user's translation unit (section 2.6). For libsecp256k1 it runs the library's own
  `src/ctime_tests.c`, which marks secrets through Valgrind's or MemorySanitizer's interface
  and refuses to build without one (`src/ctime_tests.c:16`), on webcpp's build (CR-REVIEW M9).
- **Statistical (dudect's method):** on secret inputs, natively and under wasmtime, where the
  engine's compiler is what matters for BearSSL's `bmul64` (section 3.2).

### 8.4 Entropy

- buffers of 0, 1, 256, 257, 65,536 and 65,537 bytes are filled;
- two calls differ;
- a planted failing source (a fault-injection seam in the test build) makes `fill` return
  `entropy_unavailable` and zero the buffer, and makes `operator()` throw with exceptions on;
- in an Emscripten audio worklet, `fill` returns `errc::not_supported`;
- BearSSL's `br_prng_seeder_system` names `getentropy` on every target but Windows, and is
  absent on Windows;
- a lint rule forbids `rand`, `time`, `clock` and `__rdrand` in the entropy header.

### 8.5 Allocation

A counting `std::pmr::memory_resource`, a counting global `operator new`, and, for OpenSSL,
counting functions given to `CRYPTO_set_mem_functions` in the test program alone (as
PROBE-1011's `ossl.c` and CR-PROBES `ossl_open.c` do) prove that `seal`, `open`, the
incremental seal, `encrypt`, `decrypt`, `fill`, the signatures, `ecdh` and the verifications
allocate nothing on any backend, and that `keypair::make` allocates only in the resource it is
given. What Botan's `make` allocates, and OpenSSL's without the memory option, is counted and
reported, not refused (DESIGN, 2026-10-11 "webcpp::crypto's two departures accepted"; section 10).

**OpenSSL with the memory option** (section 3.7) has tests of its own, each a program of its
own, since OpenSSL's functions are the process's and cannot be put back once allocated
through. Every one of them defines `openssl_memory_resource()`, without which none links
(section 3.7), and returns a resource that is never destroyed. Each runs natively on Linux,
macOS and Windows with MSVC, with OpenSSL linked shared and static (the MSVC shared lane is
the one where `OPENSSL_cleanup` runs at the DLL's unload and where the defaults are told apart
through `GetProcAddress`, both unmeasured until it runs), with exceptions on and off. The
parts on `chacha20_poly1305` land with it, in increment 4 (section 9).

- **Everything in the resource, refused otherwise.** The test is built with
  `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`. Its `openssl_memory_resource()` returns, never
  destroyed, a counting resource that records the blocks it gives: a
  `std::pmr::synchronized_pool_resource` over a `monotonic_buffer_resource` on a fixed buffer
  whose upstream is `null_memory_resource()`, so that the resource itself never calls the C
  library (PROBE-R3's). The test calls `install_memory_functions()` first, then
  `OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr)`, since reading the
  configuration file makes the C library allocate its `FILE` and buffer, which no hook reaches
  (section 3.7). It then runs `make`, the one-shot and incremental seals, good and failed
  opens, and the destructors, for `gcm`, `aes256` and `chacha20_poly1305` on OpenSSL, and
  **fails** on:
  - any call to the C library's `malloc`, `calloc`, `realloc`, `posix_memalign`,
    `aligned_alloc` or `free` while they run, from any caller, counted by interposition:
    on macOS, a library given to `DYLD_INSERT_LIBRARIES` whose `__interpose` section replaces
    them, as PROBE-R3's `interpose.c` does; on Linux, definitions of those functions in the
    test program, which replace glibc's for the whole process and so forward to glibc's own
    `__libc_malloc`, `__libc_calloc`, `__libc_realloc`, `__libc_free` and `__libc_memalign`,
    never through `dlsym(RTLD_NEXT, ...)`, which itself allocates. Windows has no such
    interposition, and its lane runs the other checks. The counts on 3.0.13, the Linux CI's
    version, are measured by this lane first;
  - any allocation in `seal`, `open` or the incremental seal, in the resource or elsewhere;
  - a block freed through the adapter that the resource did not give;
  - **the exit:** the resource writes one line to standard output, with an unbuffered write
    (`write`, or `WriteFile` on Windows, since the C library's streams may be gone), when its
    count of live blocks reaches 0 after `main` has returned, which `OPENSSL_cleanup` causes
    whenever it runs, at `exit` or at the DLL's unload. The test passes only with that line,
    and with no line from the resource's destructor, which it also writes from and which
    must never run. This holds on every platform and link, the Windows shared one included,
    where a handler of the program's `atexit` would run before OpenSSL's cleanup
    (crypto-r3-review C1).
- **Off, reported.** The same program without the option counts what OpenSSL's `make`
  allocates through test-installed functions and reports it, as above.
- **A late install is an error value.** One program calls `EVP_CIPHER_fetch` and frees the
  cipher, then `install_memory_functions()`, which must return `memory_functions_refused`, and
  OpenSSL's own functions must still be in place (`CRYPTO_get_mem_functions`, PROBE-R3 mode
  `late`). A second has another party's functions installed with `CRYPTO_set_mem_functions`
  and one allocation made through them, then calls `install_memory_functions()`, which must
  return `memory_functions_refused` and leave that party's functions in place (mode
  `foreign`). On the MSVC shared lane the first is also what proves that OpenSSL's defaults
  are recognised through the import (section 3.7): a wrong check would refuse a program that
  never used OpenSSL, which a third program checks, the install succeeding there.
- **A forgotten install is an error value.** A program that defines the function, is built
  with the option, and calls `make` without the install gets
  `memory_functions_not_installed`, and OpenSSL has allocated nothing (its defaults are still
  replaceable: a `CRYPTO_set_mem_functions` afterwards returns 1). Another installs, makes,
  then lets a second party replace the functions on the same thread (which OpenSSL accepts,
  mode `on`), and its next `make` gets `memory_functions_not_installed`; it puts webcpp's
  adapter back before it exits, so that `OPENSSL_cleanup` frees through it.
- **A second install** returns success and changes nothing.
- **The adapter's edges:** an allocation of 0 bytes, one whose header overflows `size_t`, a
  `realloc` that grows, one that shrinks, one to 0 bytes, and one that fails (the old block
  still valid), called through the adapter's C functions directly.
- **A failing resource,** armed by the test. With exceptions on it throws `std::bad_alloc`;
  with exceptions off it returns null, the only failure a program without exceptions can
  report (section 3.7):
  - after one successful `make`, the next `make`'s first allocation fails: `make` gives
    `not_enough_memory` (from `EVP_CIPHER_CTX_new`), and a `make` after it succeeds
    (crypto-r3-review mode `failsecond`); its second allocation fails: `make` gives
    `backend_failure` (from `EVP_EncryptInit_ex`);
  - in a program of its own, the first allocation after the install fails, inside OpenSSL's
    first initialisation: `make` gives `backend_failure`, and so does a `make` after the
    resource has stopped failing, which is the permanence section 3.7 documents
    (crypto-r3-review mode `failinit`);
  - before each `make`, the test raises an entry of its own in the thread's queue, and after
    it the queue holds that entry alone, whatever `make` returned (`ERR_set_mark`, section
    3.3);
  - the mark comes after the guards: in the forgotten-install program above, the `make` that
    returns `memory_functions_not_installed` has made no OpenSSL call at all, which the
    `CRYPTO_set_mem_functions` returning 1 after it proves (a mark set first would make it
    return 0, crypto-r3-review `mark_first.log`).
- **Its neighbours in the process** (crypto-r3-review M11), with increment 3, where trystero
  links both natively: one program links Boost.Asio's SSL headers and libdatachannel,
  installs first, and expects success, so that neither uses OpenSSL during static
  initialisation; another runs a TLS handshake through libssl over a loopback socket under
  the option, with the counting resource refusing a foreign free and checking the exit as
  above.

### 8.6 Every message

Each message of sections 2.4, 3.3, 3.6, 3.7, 4.3 and 4.4 comes from a test of its own, in a
directory of its own (section 4.3's pitfall), which checks its exact text:

- `webcpp.compile-diagnostic` for every `#error` and `static_assert`, on every target where it
  can fire (the rule runs on b2's `clang` toolsets; MSVC's `C1189` is checked by the
  Compiler Explorer probe's method, PROBES `msvc/`), the two of the memory option (section
  3.7) among them;
- two link tests, natively, of programs that set the memory option and do not define
  `openssl_memory_resource()`: one calls `install_memory_functions()`, the other only a
  `make` on OpenSSL, and both must fail naming the function (ld64, GNU ld and lld, MSVC's
  `link.exe`), since any use of the backend under the option needs it (section 3.7); and
  LNK2038 on MSVC between two files compiled one with and one without the option;
- a run test per error value of the memory option, whose message is compared with section
  3.7's text;
- on MSVC, two compile tests of every public header with `<windows.h>`, included before them
  without `NOMINMAX` or `WIN32_LEAN_AND_MEAN`, and after them, each using `std::max` and
  `random_device::max` afterwards (section 2.2), with the memory option on so that
  `backends/openssl.hpp`'s Windows branch is compiled;
- a link test per canary case (library missing, of another commit, of another configuration,
  without the canary) on ld64, wasm-ld, Emscripten and MSVC, and LNK2038 on MSVC between two
  files compiled with different configurations;
- a fixture of the superproject's Jamfile test (`tools/test/crypto_jamfiles_test.py`) per
  message raised on Emscripten, where the compiler's line starts with a path of the machine:
  `/webcpp/crypto//openssl` named there, OpenSSL's `HAS` and
  `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` there; with Botan (increment 5), a b2 test per stop
  of `webcpp.external` (Botan not found for a program, a wrong root, static without the static
  library, Botan on WebAssembly), run on a fixture project, and the left-out message for
  webcpp's own test.

The CI also compares each pin in `<webcpp/crypto/detail/canary.h>` with
`git -C third_party/libs/<name> rev-parse --short=7 HEAD` (AGENTS:745-746).

### 8.7 The unused library

A program with `/third_party//<name>` against the same program without it, for BearSSL,
libsecp256k1 and ctaes, byte-identical on all four targets (natively under the same output
name) (C-LIBS:495-501). A library's test Jamfile may not use b2's raw `exe` (AGENTS:852, the
lint's raw b2 rules), and no rule of `tools/webcpp.jam` builds a pair and compares it, so
increment 1 adds one, `webcpp.same-program <name> : <sources> : <library> : <targets>`: it
builds `<sources>` with and without `<library>` under the same output name in two directories
of its own, and passes when the two files are byte-identical (CR-REVIEW M6). OpenSSL and
Botan are left out: OpenSSL is the provider's archive, not one webcpp builds, and a shared
Botan named adds its dependency (section 4.2).

### 8.8 Sizes and throughput

- **Sizes:** the CI builds, through b2, on every target at -O2 and -Os: the baseline program,
  the AES-GCM program on each backend, the signing program with the small and the large
  tables, and natively the OpenSSL program, static. It writes the table, raw and
  `gzip -9n` (C-LIBS:455-457), into the report, and the documentation's table is regenerated
  from it; moving a pin regenerates it (AGENTS:747-748).
- **Throughput:** seal and open of 100 B and of 1 MiB per backend, natively on the CI's
  ubuntu-24.04 (x86-64), ubuntu-24.04-arm and macos-15 runners, under wasmtime, under node and
  in Chrome against WebCrypto, three runs each, into the report and the documentation's table
  of section 5. The x86-64 job is the one of run 38113681987 (C-LIBS:416-426), made part of
  the library's CI.

## 9. Increments

1. **The foundation, and AES-GCM on BearSSL and OpenSSL.**
   - `config.hpp`, `error`, `equal`, `random_device` (section 6), `aes128/192/256`,
     `gcm<Cipher>` one shot and the incremental seal; the backends BearSSL and OpenSSL; the
     convenience header; OpenSSL's memory option with its adapter and its errors (section
     3.7).
   - The submodule `third_party/libs/bearssl` on the mirror; `third_party/Jamfile` with
     `bearssl`; crypto's alias `/webcpp/crypto//openssl` over the superproject's source-built
     OpenSSL, and its stop on Emscripten (section 4.4); the canary; `detect_mismatch`.
   - The tests of section 8 for these: vectors, BearSSL against OpenSSL, constant time,
     entropy, allocation (the memory option's on Linux, macOS and MSVC, shared and static,
     without its parts on ChaCha20-Poly1305 and its neighbours), every message, the unused
     library with the new rule `webcpp.same-program` (section 8.7), sizes.
   - **Its CI carries the x86-64 throughput job**, with the ARM and macOS runners beside it,
     so that section 5's table is measured by the library's own CI from the start.
   - The documentation, with section 7 as its page on Jamfiles and the warning on ARM.
2. **libsecp256k1 and ctaes.** The submodules (ctaes as Bitcoin Core v31.1's
   `src/crypto/ctaes`, section 3.4), their targets, `curves::secp256k1::keypair`,
   `verify_schnorr`, `verify_ecdsa`, ECDH, the DER encodings, low-S and the x-only tweak
   (Task 27b); `aes128/192/256<ctaes>` and the diagnostic of
   `gcm<aes256<ctaes>>`; the tables feature with `auto`; their tests, the block cipher compared
   across ctaes and OpenSSL.
3. **trystero's migration.**
   - `native/sealer.hpp` and its browser twin are replaced by `aes256_gcm` with trystero's
     16-byte IVs; its `sealer` interface, asynchronous today (trystero `backend.hpp:290-315`),
     becomes a direct call.
   - Its `random` becomes `random_device::fill`. Natively it aborts today when the system has
     no entropy (`native/backend.hpp:240-245`); it then reports `entropy_unavailable` as a
     failure of the operation that needed the bytes (a join, a seal), through its errors, as
     errors are values (AGENTS:636-638). Its documentation records the change.
   - Its `signer` and `verify` (`protocol/nostr.hpp:118-221`) move to
     `curves::secp256k1::keypair` and `verify_schnorr`, and its Jamfiles name
     `/third_party//secp256k1` in place of its external libsecp256k1 (`build.jam:32-33`), in
     the same change (section 4.5).
   - Its direct use of OpenSSL is gone. Natively OpenSSL stays a transitive dependency,
     through libdatachannel's DTLS and Beast's TLS.
   - The memory option's tests with its neighbours in the process, Boost.Asio's SSL and
     libdatachannel linked, and a TLS handshake under the option (section 8.5).
4. **ChaCha20-Poly1305** on BearSSL and OpenSSL, with its vectors and cross-backend lane, and
   its part of the memory option's tests (section 8.5).
5. **Botan**, found, native only (LEDGER:29), only with exceptions: AES, GCM and
   ChaCha20-Poly1305, its exception mapping, the additional data set on every message, its
   allocation statement, its place in every native cross-backend lane, and the
   `webcpp.external` change that stops a program's build (section 4.4).

## 10. Every departure, and the owner's answers

Every departure of this spec from the owner's decisions of section 1.3 is listed here
(LEDGER:27). Two needed the owner, who answered both on 2026-10-11 (DESIGN, 2026-10-11
"webcpp::crypto's two departures accepted"; 2026-10-11 "OpenSSL's memory functions are a
customization of webcpp::crypto's config"); the others are rulings of the controller, recorded
with their reasons, which the owner may reverse. Nothing is open for the owner now but his
approval of the whole spec.

**Answered by the owner on 2026-10-11.**

1. **OpenSSL and Botan allocate outside the user's allocator.** The decision of 2026-10-10 says
   that run-time objects route every allocation through an allocator the user gives, and that a
   counting test proves none goes elsewhere (DESIGN, 2026-10-10 "allocators"; 2026-10-10
   "allocators, scope", AGENTS chapter 6 "Allocation"). OpenSSL's and Botan's `make` allocate
   their contexts through their own allocators: OpenSSL through `CRYPTO_malloc`, whose only
   hook, `CRYPTO_set_mem_functions`, is process-global and so not a library's to set on its own
   (section 3.3); Botan through `new` and its `secure_allocator` (section 3.6). Every later
   call allocates nothing, measured on OpenSSL 3.6.4 (CR-PROBES `ossl_open.out`) and proved by
   section 8.5's counting test, which, for Botan and for OpenSSL without the option, counts and
   reports what `make` allocates instead of refusing it. BearSSL, ctaes and libsecp256k1 follow
   the decision whole. The spec proposed to accept the departure for these two backends and to
   state it on their pages; the alternative was to drop them as backends. **Accepted on
   2026-10-11, with the option of section 3.7 for OpenSSL** (DESIGN, 2026-10-11
   "webcpp::crypto's two departures accepted"; 2026-10-11 "OpenSSL's memory functions are a
   customization of webcpp::crypto's config"): both backends stay, recommended where they
   apply, and the departure is stated on their pages. A program that defines
   `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, names a `std::pmr::memory_resource` in
   `openssl_memory_resource()` and calls `backends::openssl::install_memory_functions()` first
   in `main`, with a resource that is never destroyed, has every allocation of OpenSSL's in
   that resource, proved by section 8.5's test, which then refuses rather than reports; without
   the option the departure stands as stated above. **Botan keeps the departure:** at 3.13.0
   its secure memory goes through `allocate_memory`, fixed functions with no setter over a
   global mlock pool or `calloc`, its `secure_allocator` is stateless, and its objects come
   from the global `operator new`, so no hook can route its allocations (section 3.6,
   BOTAN-SRC).
2. **OpenSSL's shared or static link is a b2 feature, not a setting of the config.** The
   decision of 2026-10-10 puts the choice in the library's config (DESIGN, 2026-10-10
   "webcpp::crypto's backends are the C libraries themselves"). A header cannot change how a
   program is linked, so the feature `webcpp-crypto-openssl-link` (`shared`, the default, or
   `static`) carries it, and `config.hpp` only reads what the build says (section 4.4,
   C-LIBS:296-304); Botan's `webcpp-crypto-botan-link` is the same. The spec proposed to
   confirm the feature as the decision's form. **Confirmed on 2026-10-11** (DESIGN, 2026-10-11
   "webcpp::crypto's two departures accepted"; 2026-10-11 "OpenSSL's memory functions are a
   customization of webcpp::crypto's config"). Since the provider amendment of 2026-11-03
   OpenSSL is built from source and linked static (section 4.4), so the feature is Botan's
   alone.

**Rulings** (LEDGER:22-29), each in the section that applies it. The first three depart from
a decision; the others narrow one, or restore it where the first version of this spec had
left it:

- **A failed `open` zeroes `data`** (section 2.4), replacing the reviewed behaviour of
  2026-10-10, which verified first and left the ciphertext untouched (README.ledger:14): no
  backend can verify before it decrypts without a GHASH of webcpp's own, the port the owner
  refused (DESIGN, 2026-10-10 "webcpp::crypto's backends are the C libraries themselves").
- **BearSSL's own seeder is off on Windows** (sections 4.1 and 6): its only seeder there is
  `CryptGenRandom`, while webcpp's `random_device` uses `BCryptGenRandom` and no backend
  chooses its entropy (DESIGN, 2026-10-11 "entropy, safest and readable").
- **libsecp256k1's small tables are the default on Emscripten only;** wasip2, wasip3 and native
  builds get the default tables (section 3.5), since the owner's mission entry makes WASI a
  server target, where his rule asks for the fastest (DESIGN, 2026-10-10 "webcpp::crypto's
  backends are the C libraries themselves", "webcpp's mission").
- **Botan is a native backend only, for now** (sections 3.6, 8.2, 9): the owner made Botan a
  backend (DESIGN, 2026-10-10 "webcpp::crypto's interface and backends") without naming its
  targets, and no Botan for Emscripten exists that webcpp's build could find.
- **GCM's IV is at most 128 bytes on every backend** (section 2.4), OpenSSL's measured limit,
  so that a call never succeeds on one backend and fails on another.
- **No incremental `open` for now; the incremental seal takes the IV and the additional data at
  its start** (section 2.4). The interface the owner approved is shaped as Hash2's (DESIGN,
  2026-10-10 "webcpp::crypto's interface and backends") and names no incremental form; this
  keeps unauthenticated plaintext from being released and fits Botan's interface.
- **ctaes is the copy Bitcoin Core ships, at tag v31.1** (section 3.4), as the owner named it
  ("o ctaes do Bitcoin Core"), and webcpp exposes its block cipher alone. The first version
  of this spec pinned `bitcoin-core/ctaes`, which is another copy (CR-REVIEW I5).
- **OpenSSL's memory option, as the spec reads the owner's words** (section 3.7;
  crypto-r3-review M12). The owner asked that "a program that wants OpenSSL's allocations to
  go through its own functions can say so" (DESIGN, 2026-10-11 "OpenSSL's memory functions are
  a customization of webcpp::crypto's config"). The spec reads "its own functions" as a
  `std::pmr::memory_resource` that the program returns from a function it defines, not three
  C functions of the program's given to OpenSSL as they are: a `std::pmr::memory_resource` is
  the allocator chapter 6 names for run-time objects (AGENTS:662-665), and the function keeps
  the resource in the program, since OpenSSL's hook passes no user data and webcpp may hold no
  global state (AGENTS:639-640). The install writes OpenSSL's process-global functions, which
  is the hook's nature, and only when the program calls it; and the resource is never
  destroyed, which the Windows shared link requires (crypto-r3-review C1). Neither departs
  from a decision; both are recorded so that the owner approves them knowingly.
