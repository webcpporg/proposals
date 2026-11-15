## Context

webcpp::crypto is a header-only C++20 wrapper library: webcpp's interface over cryptography
that other projects wrote. Its design is the owner's, decided on 2026-10-10 and 2026-10-11 in
his design notes and turned into a specification (`reference/spec.md`, approved on
2026-10-11 after an independent review, a second review of its OpenSSL memory option, and the
controller's rulings on both) and a plan of 43 tasks in five increments (`reference/plan.md`,
status draft, written 2026-10-11). Nothing is implemented. The first version of the design made
the library asynchronous because WebCrypto is promise-based, and made AES-GCM webcpp's own port
of BearSSL; the owner removed WebCrypto (amendment C1) and refused the port, so the final
design has C code on every target, every call synchronous, and the algorithms the C libraries'
own.

Constraints every decision below works within, from the superproject's rulebook (its
`AGENTS.md`, chapter 6 "Bringing a C library into webcpp" above all):

- header-only C++20, namespace `webcpp::crypto`, headers under `<webcpp/crypto/...>`, internals
  in `detail`, macros `WEBCPP_CRYPTO_*`, include guards `WEBCPP_CRYPTO_<PATH>_HPP`;
- targets native (Linux x86-64 and arm64, macOS arm64, Windows with MSVC), wasip2, wasip3 and
  Emscripten, every test on each target the backend has, with exceptions on and off;
- errors as values (`boost::system::result`); exceptions optional through
  `WEBCPP_CRYPTO_NO_EXCEPTIONS`, raised only through `boost::throw_exception`;
- no global or static mutable state, no clock, file, network or environment in a header
  except what is the purpose of the entropy header (marked `lint-world:`);
- allocation through the allocator the program gives, as little as the job allows;
- Boost before custom code (Hash2, System, Core), third-party C code only as a pinned
  submodule, built by the superproject's `third_party/Jamfile`;
- BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- AES-GCM (keys of 16, 24 and 32 bytes) and ChaCha20-Poly1305 with an interface shaped as
  Boost.Hash2's, the AES block cipher where a backend has it, a `random_device`, a constant-time
  `equal`, and secp256k1 (BIP340 Schnorr, ECDSA, ECDH), on every target webcpp has.
- Backends that are the C libraries themselves; one default per target chosen by the owner's
  rule: the smallest binary on the web, the fastest on servers.
- A build that cannot go wrong silently: three layers that say exactly what to add.
- No allocation in an operation on any backend; the context of a key pair in the user's
  resource; OpenSSL's allocations in the user's resource on request.
- Proof by official vectors (GCM specification, Wycheproof, NIST CAVP, RFC 8439, BIP340), the
  C libraries' own suites, cross-backend agreement, WebCrypto as an outside oracle, constant-time
  checks, counting allocation tests and a test for every message.
- trystero moved onto it, with its own sealer, entropy and signer gone.

**Non-Goals:**

- Hashing and HMAC: Boost.Hash2's. HKDF, when needed, is written on Hash2's `hmac`.
- Any asynchronous interface, and WebCrypto as a backend.
- A port of BearSSL's, OpenSSL's or any other library's algorithms into webcpp.
- An incremental `open` (a later increment may add it only with the plaintext withheld until the
  tag verifies), an incremental ChaCha20-Poly1305, tags shorter than 16 bytes.
- A Botan build for WebAssembly, which is a later increment's if a user needs it.
- OpenSSL on WebAssembly, or the memory option anywhere but native.
- The CI's three levels as a whole: this plan builds only the nightly level of the own lanes.
- Routing Botan's allocations through the user's allocator: Botan 3.13.0 offers no hook.

## Decisions

### D1. Backends are the C libraries themselves, named after the library

An algorithm runs on a backend, a C or C++ library that webcpp wraps: `backends::bearssl`,
`backends::openssl`, `backends::ctaes`, `backends::secp256k1`, `backends::botan`. BearSSL is
used as its author wrote it, from webcpp's unmodified mirror `webcpporg/bearssl`.

- **Why:** the owner refused a webcpp port of BearSSL's AES-GCM on 2026-10-10; a proven
  library is audited and measured, a port is a new thing to audit.
- **Alternatives:** WebCrypto (asynchronous, removed by amendment C1); a unity-build single
  header per C library (superseded: neither BearSSL nor libsecp256k1 compiles as C++).

### D2. An interface shaped as Boost.Hash2's, with `make` where Hash2 cannot be followed

Algorithm types state `key_size`, `iv_size`, `tag_size`, `block_size` as `static constexpr`
members and name their output `result_type`; modes are templates over a cipher
(`gcm<aes256<>>`); tags are `boost::hash2::digest<16>`; the backend is a template parameter
whose default is `default_backend`. A keyed object is built by `static make(key)`, returning
`boost::system::result<T>`, because a key can have the wrong size and a backend can fail to
allocate.

- **Why:** Boost's own shape is what a Boost user expects, and it keeps `gcm` parallel to
  `hmac<H>`.
- **Alternatives:** free functions per backend (the prototype's ctaes was one); a constructor
  that throws (the library works without exceptions).

### D3. The algorithm types do the guards once; a backend supplies only the work

`aes128/192/256<Backend>`, `gcm<Cipher>`, `chacha20_poly1305<Backend>` and
`secp256k1::basic_keypair<Backend>` do every guard, the moved-from state, the incremental seal's
state machine, the length accounting and the zeroing of a failed `open`, once, for every
backend. A backend's header supplies specialisations of six traits in `webcpp::crypto::detail`:
`block_state`, `block_engine`, `gcm_state`, `gcm_engine`, `chacha_state`, `chacha_engine`. A
state trait is what an object stores and never refuses, so `aes256<backends::bearssl>` is a
complete type that `gcm` can name; an engine trait is used only inside member functions, so a
backend without the algorithm refuses with its `static_assert` only where the algorithm is
called.

- **Why:** the same guards on every backend are what lets the agreement tests compare them; a
  moved-from BearSSL `gcm` would otherwise seal under an all-zero key schedule.
- **Alternatives:** each backend implements the whole class (guards diverge by backend).

### D4. Errors are values; one function throws

Every operation returns `result<T>` or `result<void>`. The one throwing function is
`random_device::operator()`, which exists only with exceptions, as `std::random_device`'s does;
`keypair::make` lets the user's resource report a failed allocation as it does. Botan's backend
catches every Botan exception at its boundary and maps it.

- **Why:** the owner's rule that every library builds and works with exceptions on and off.

### D5. A failed `open` zeroes `data`

Every backend decrypts and checks the tag in one pass, and not all clear what they decrypted,
so webcpp's code zeroes the whole of `data` after any failure that follows the start of
decryption (`authentication_failed`, `backend_failure`), by `volatile` stores. A guard's refusal
comes before any work and leaves `data` untouched.

- **Why:** verifying before decrypting needs a GHASH of webcpp's own over the backend's AES,
  the port the owner refused. A ruling of the controller, listed among the departures.
- **Alternatives:** leave the ciphertext intact and verify first (the 2026-10-10 draft).

### D6. No incremental `open`; the incremental seal takes the IV and the additional data first

`begin_seal(iv, additional_data)`, `update(data)`, `result()`. Botan's public interface takes
the associated data whole before the nonce, so pieces of additional data could only be joined
in a growing buffer, an allocation. An incremental decryption would return plaintext before the
tag is known, so none is offered; what the incremental seal produces is opened by the one-shot
`open`.

### D7. IVs of 1 to 128 bytes on every backend

OpenSSL accepts no GCM IV longer than 128 bytes (measured on 3.6.4), BearSSL and Botan accept
longer ones. Without a cap the same call would succeed or fail by backend. 12 bytes is
recommended, 16 bytes (trystero's) takes the hashed path. Every vector of the GCM specification
and NIST CAVP stays inside the cap; three Wycheproof cases with a 257-byte IV are expected
refusals.

### D8. AES-192 is accepted

The key sizes are 16, 24 and 32 bytes: AES-192 is part of the standard, every AEAD backend has
it, and it lets every test vector run. Chrome's WebCrypto refuses it, so the documentation says
a key meant for a browser's WebCrypto is of 16 or 32 bytes.

### D9. The default backend follows the owner's rule per target

Emscripten, wasip2 and wasip3: BearSSL. Native with OpenSSL linked: OpenSSL (1.4 times faster
than BearSSL's AES-NI on x86-64, 30 to 62 times on ARM). Native otherwise: BearSSL. With more
than one linked, the first of OpenSSL, BearSSL, Botan; a program picks another with
`<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>`. The default is the AEADs'; the block cipher has
none of its own, so on WebAssembly a program names `aes256<backends::ctaes>`.

- **Alternatives:** choose by architecture (BearSSL picks AES-NI and PCLMUL by CPUID itself).

### D10. Entropy: one source per platform, nothing in between

`getentropy` on Linux, macOS and WASI (and, through musl, `crypto.getRandomValues` on
Emscripten), `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` on Windows. No cache, no
DRBG, no weaker fallback; asked in pieces of at most 256 bytes; every call's result checked;
the whole function fits on one screen. The backends use the same source: BearSSL is compiled
with `BR_USE_GETENTROPY=1`, `BR_RDRAND=0`, `BR_USE_URANDOM=0` and its seeder is absent on
Windows; webcpp never calls `RAND_bytes` or a Botan RNG.

- **Why:** the owner's decision of 2026-10-11, "entropy, safest and readable".

### D11. libsecp256k1's tables: small on Emscripten, large elsewhere

The feature `webcpp-secp256k1-tables` takes `auto` (the default), `small` or `large`. The small
tables make an Emscripten program 141,922 bytes against 1,211,251, for verification about a
third slower; the large tables verify 1.7 times faster on wasip2. The owner's mission makes
Emscripten the browser's target and WASI the server's and the cloud's. A ruling of the
controller.

### D12. ctaes is the copy Bitcoin Core ships, at tag v31.1, as a block cipher alone

The owner named "the ctaes of Bitcoin Core". It is `src/crypto/ctaes` of `bitcoin/bitcoin`
(commit `9be056a8a72b624dae9623b2f7bded92c2a21c91`), a shallow submodule. It exports AES-128,
AES-192 and AES-256 on whole blocks and nothing else; `gcm<aes256<backends::ctaes>>` does not
compile, because webcpp builds no GCM on another library's AES.

### D13. C libraries are built from source, file by file, and the program names them

BearSSL, libsecp256k1 and ctaes are submodules under `third_party/libs/`, built one object per
file as static libraries by `third_party/Jamfile` (project `/third_party`, also reachable as
`/webcpp/third_party`), in the library's own C dialect. A program names
`/webcpp/crypto//crypto` and `/third_party//<backend>`. A library named and not called adds no
byte (byte-identical programs on all four targets). Three layers stop a wrong build (D14).

- **Alternatives:** one header included once, as Boost.JSON's `src.hpp` (superseded); building
  inside the header-only alias (a program would link what it does not name).

### D14. Three layers stop a wrong build, each saying what to add

Layer 1, compile time: `config.hpp` classifies the target, refuses impossible combinations and
picks the default; the check that a backend is linked sits in the backend's own header, so a
program that only signs needs no encryption backend. Layer 2, link time: every function of a
backend's header that calls the C library reads a volatile symbol
`webcpp_crypto_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link` that
`src/canary_<name>.c` defines; a library missing, of another commit or configuration, or built
without the canary fails to link with that name. Layer 3, MSVC: `#pragma detect_mismatch`, one
key per backend.

### D15. OpenSSL and Botan are found, not built; a program that names a missing one stops

`webcpp.external` finds them (`-sOPENSSL_ROOT`, `.local/openssl`, the system). A webcpp test or
example whose library is missing is left out with one line; a user's program that names it
stops the build with three ways out. They are native only; `/third_party//openssl` and
`/third_party//botan` stop a WebAssembly build. Shared or static is the propagated feature
`webcpp-crypto-openssl-link` (and `webcpp-crypto-botan-link`), shared by default; a header
cannot change how a program is linked, so the feature carries the owner's choice.

### D16. OpenSSL's memory functions are an option the program turns on with a resource it defines

`<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` makes `backends/openssl.hpp` declare
`std::pmr::memory_resource& openssl_memory_resource() noexcept`, which the program defines once,
and `backends::openssl::install_memory_functions()`, which the program calls first in `main`.
Every allocation of OpenSSL in the process then goes to the resource through three `extern "C"`
adapter functions that store each block's size in a header. The resource is never destroyed,
because OpenSSL frees through it after `main`, and on Windows with a shared libcrypto after every
static destructor. Every `make` of the backend checks that the functions are webcpp's and
otherwise returns `memory_functions_not_installed`; a missing definition is a link error.

- **Why a function the program defines:** OpenSSL's hook carries no user data, so any other form
  would keep the resource in a global variable of webcpp's, which the rulebook forbids; Boost
  already uses this form for `boost::assertion_failed` and `boost::throw_exception`.
- **Alternatives:** three C functions of the program's given to OpenSSL as they are (the
  spec reads "its own functions" as a `std::pmr::memory_resource` instead); installing from a
  static initialiser (initialisation order is unspecified and cannot return an error).

### D17. Two departures from the owner's allocator decision are accepted

OpenSSL's and Botan's `make` allocate their contexts through their own allocators, outside the
user's, and every later call allocates nothing. The owner accepted this on 2026-10-11 and gave
OpenSSL the option of D16; Botan 3.13.0 has no hook that could carry one (its secure memory
goes through fixed functions with no setter, its allocator is stateless, its objects come from
the global `operator new`), so Botan keeps the departure and the documentation says a program
that needs every allocation in its allocator uses OpenSSL with the option, or BearSSL.

### D18. `<windows.h>` is included through one header, and `max` is written `(max)()`

`random_device.hpp` and `backends/openssl.hpp` reach Windows' API through
`<webcpp/crypto/detail/windows.hpp>`, which defines `NOMINMAX` and `WIN32_LEAN_AND_MEAN` only
when they are not defined and undefines only what it defined. Every header writes `(max)()`,
`(min)()` and `(std::numeric_limits<T>::max)()`.

### D19. Botan is native only and only with exceptions, for now

No Botan for Emscripten exists that webcpp's build can find, and Botan 3.13.0 has no OS
definition for wasip2 or wasip3. Botan reports every error by throwing, the tag check included,
so its header declares nothing under `WEBCPP_CRYPTO_NO_EXCEPTIONS` and a program built without
exceptions that names it gets an `#error`.

### D20. Tests: official vectors, outside oracles and counting resources, on every target

Vectors: the GCM specification's 18 cases, Wycheproof's 316 AES-GCM tests and 325
ChaCha20-Poly1305 tests, NIST CAVP, RFC 8439, BIP340's CSV, Wycheproof's Bitcoin ECDSA and ECDH
files. Outside oracle: node's and Chrome's WebCrypto. Agreement: every backend against every
other, from a seeded generator. Constant time: Valgrind and MemorySanitizer with a planted leak
that must fail, dudect natively and under wasmtime. Allocation: a counting resource, a counting
`operator new` and, for OpenSSL, counting hooks and an interposer on the C library's allocator.
Every message has a test of its own in a directory of its own.

### D21. The plan's rulings on the tree (conflicts C1 to C17)

Where the superproject's tree disagrees with the spec's prose the plan rules: the Jamroot
registers `third_party` under both `/third_party` and `/webcpp/third_party`; `infra/shared` is
integrated into `main` first; the `webcpp.external` declaration lives in `libs/crypto/build.jam`
and `third_party/Jamfile` reads the root through `webcpp.external-root`; the user's Jamfiles
and every build stop are fixtures of a Python test of the tools (`crypto_jamfiles_test.py`),
since the lint refuses b2's raw `exe` in a library's Jamfiles; every test program is declared
twice by `crypto-run`, the second with `<exception-handling>off`; sizes are measured at b2's
`speed` and `space`. Three of the rulings are departures for the owner (O5): C17 (sizes at
`-O3` and `-Os` in place of `-O2` and `-Os`), C16 (webcpp's own throughput program in place of a
workflow no tree holds) and C14 (MSVC's messages checked by building the cases with MSVC in the
tools test).

### D22. Version 0.1, and the tree of 2026-11-14 (the revision of the plan)

On 2026-11-14 the owner set the first version's scope and the plan was revised against the
superproject's `main` of that day (`reference/plan.md`, "The revision of 2026-11-14"):

- **Version 0.1 is increment 1 without OpenSSL's memory option (D16, D17's OpenSSL half), the
  constant-time lane, sizes and throughput, the browser lane, the nightly level and NIST CAVP,
  plus increment 2.** Those, and increments 3 to 5, come after it. The error values 13 and 14
  stay reserved for the memory option.
- **OpenSSL is built from source, which supersedes D15 for OpenSSL** (the provider amendment of
  2026-11-03, CODING-GUIDELINES.md chapter 5 rule 7): OpenSSL 3.5.9, static, on native, wasip2
  and wasip3, never a system copy. A program names `/webcpp/crypto//openssl`, crypto's alias over
  `/webcpp/third_party//openssl`, which defines `WEBCPP_CRYPTO_HAS_OPENSSL`; on Emscripten the
  alias stops the compile with the fix. OpenSSL has no canary, since the provider injects
  nothing into upstream; its layers are the compile-time messages and MSVC's
  `detect_mismatch`. Botan keeps D15.
- **D9 holds, MSVC included:** OpenSSL is the native default when linked. On wasip2 and wasip3
  OpenSSL may be linked and chosen with `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`; the default there
  stays BearSSL. The source-built OpenSSL is compiled without assembly on MSVC, wasip2 and wasip3
  (table AES, no AES-NI, not constant time); the documentation says so and how to choose BearSSL.
- **secp256k1 gains what a Bitcoin wallet needs (Task 27b, the owner's decision of
  2026-11-14):** DER encoding and strict and lax decoding of ECDSA signatures, low-S
  normalization, and the BIP341 Taproot tweak of a key pair and of an x-only key, thin wrappers
  over libsecp256k1's own functions (its `contrib/lax_der_parsing.c` included); transactions,
  sighashes, BIP32 and addresses stay outside the library.
- **trystero's build moves onto `/third_party//secp256k1` in version 0.1**, its code unchanged,
  which removes the CI's external libsecp256k1 (Migration Plan, step 3's build half).
- **Two limits are recorded instead of built:** MSVC's message cases fail on the MSVC lanes and
  their text is checked on clang and the GNU and wasm linkers; crypto has no size table until
  the sizes lane.
- **The superproject's newer rules apply:** `config.hpp` includes `<webcpp/config.hpp>` and reads
  `WEBCPP_TARGET_*`; `webcpp-own`, `webcpp.external-root` and `webcpp.external-looked` (D21) are
  not built, since every backend of version 0.1 is built by b2; every lane runs on every push;
  commits use the contributor's own identity.
- **Alternatives:** the whole of increment 1 first (rejected by the owner for a first version);
  OpenSSL's HAS define in the shared provider's usage requirements (rejected: it ties the
  provider to one library and leaves the Emscripten stop without an instruction); BearSSL as the
  default on MSVC because of `no-asm` (rejected by the owner: the spec's rule holds and the page
  explains the choice).

## Risks / Trade-offs

- [The Windows branch of the memory option is unmeasured] -> recognising OpenSSL's defaults
  through `GetModuleHandleExW` and `GetProcAddress`, and `OPENSSL_cleanup` at a DLL's unload,
  rest on documentation; the MSVC lane runs the `late` and `foreign` tests on the shared and
  the static link before anything is claimed.
- [A failure in OpenSSL's first initialisation is permanent for the process] -> stated in the
  documentation; a bounded resource is sized for the first `make` (6,061 allocations, 4,621
  blocks live afterwards on OpenSSL 3.6.4 on macOS).
- [BearSSL is slow on ARM] -> 30 to 62 times slower than OpenSSL on the CI's ARM runners, no
  ARMv8 AES or PMULL code; the documentation warns, and a server on ARM names OpenSSL.
- [`bmul64` assumes a constant-time 64-bit multiplication] -> true on x86-64 and AArch64; a
  WebAssembly engine on a 32-bit host may lower `i64.mul` to variable-time code; the
  documentation says so and the dudect lane runs under wasmtime.
- [A repeated IV under one key breaks GCM] -> the documentation states the uniqueness
  requirement and the limit of 2^32 messages per key with random IVs, for ChaCha20-Poly1305 too.
- [Plaintext sits in `data` during an in-place `open`] -> zeroed on failure; a caller whose
  `data` another party can read opens into private memory.
- [OpenSSL's process-global state] -> the option writes OpenSSL's process-wide functions, only
  when the program calls the install; nothing else in the process may call
  `CRYPTO_set_mem_functions`, and a replacement after the install is detected only when it
  finishes before the next `make`.
- [Two copies of libcrypto in one process] -> asio and crypto both link the one source-built
  provider, whose archives come from one build directory per configuration (D22).
- [OpenSSL without assembly on MSVC and WebAssembly] -> table AES, not constant time, is the
  native default on MSVC when OpenSSL is linked; the documentation says so and how to choose
  BearSSL (D22).
- [Two files of one program choose different default backends] -> `aes256_gcm.hpp` carries
  `detect_mismatch("webcpp_crypto_default_backend", ...)` on MSVC.
- [Large heavy lanes] -> BearSSL's own 88 tests take 28 to 46 s per target; the heavy lanes run
  at the nightly level, which this plan builds for own lanes.
- [b2 shares an object file between two `exe` of one source that differ only by a free define]
  -> every error case has a directory of its own.
- [The upstream pins move] -> a later Bitcoin Core release moves the ctaes pin as a change of its
  own; the CI compares each canary commit with its submodule's HEAD.
- [The superproject moves under the work] -> the work branch rebases onto `origin/main` when a
  pull request touching the same files merges, and at the end of each increment.

## Migration Plan

There is nothing to migrate in webcpp until increment 3: no program depends on this library.
Increment 3 changes webcpp::trystero in its own repository, on a branch `crypto`, after its
shared-connection change has integrated:

1. `native/sealer.hpp` and its browser twin are replaced by `aes256_gcm` with trystero's 16-byte
   IVs, the asynchronous `sealer` interface by a direct call.
2. Its `random` becomes `random_device::fill`; where it aborts today when the system has no
   entropy, it reports `entropy_unavailable` as a failure of the operation that needed the bytes.
3. Its `signer` and `verify` move to `secp256k1::keypair` and `verify_schnorr`, and its Jamfiles name `/third_party//secp256k1` in place of its external
   libsecp256k1, in the same change (one copy per program).
4. Its direct use of OpenSSL goes; natively OpenSSL stays a transitive dependency through
   libdatachannel's DTLS and Beast's TLS.

To start the implementation (details in `reference/STATUS.md`): the base is the superproject's
`origin/main`; follow the plan from Task 0, after the owner has created `webcpporg/crypto` (G1). Rollback: the
change is a proposal; withdrawing it deletes `openspec/changes/genesis/`.

## Open Questions

- Gate G1 and G2: the owner creates `webcpporg/crypto` and adds it as the submodule `libs/crypto`.
- After version 0.1: owner gate O2 (the heavy own lanes at the CI's nightly level, Task 25b),
  O3 (NIST CAVP's archives pinned by URL and SHA-256, or a mirror), O5 (the departures C16 and
  C17, and `brew install botan` on the development machine).
- Whether `webcpp::webrtc` and `webcpp::beast` use this library, and how.
- A later increment: an incremental `open` with the plaintext withheld until the tag verifies,
  an incremental ChaCha20-Poly1305, and a Botan WebAssembly build.
