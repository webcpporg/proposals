## Context

This change comes after crypto/sign-unified, crypto/uniform-types and crypto/formats, which reshape
the engines it describes (typed keys and signatures, X25519's key pair, the boxes and kx in
`public_key_encryption` and `key_exchange`, the format codecs). The engines as they are, read in
`include/webcpp/crypto/backends/*.hpp` and `detail/engines.hpp`:

- **The pattern** (`detail/engines.hpp`): per algorithm, a state an object stores, which never
  refuses, and an engine of static functions used only inside the algorithm's member functions.
  The family header defines both primaries: an empty state, and an engine whose body is the
  family's generated refusal (`detail/capabilities.hpp`), which fires where a backend has no
  specialization: a backend without the algorithm is refused naming the backends that have it, the
  default not linked names the Jamfile line, a backend whose header is not included is told to
  include it.
- **The engines**: `block_engine<B, KeySize>` (`init`, `encrypt`, `decrypt`, `move`, `reset`) on
  OpenSSL, BearSSL (a refusal of its own) and ctaes; `gcm_engine<B, KeySize>` (`init`, `start`,
  `run`, `tag`, `check`, `move`, `abandon`, `reset`) on BearSSL, OpenSSL and ctaes;
  `aead_engine<Family, B>` (`init`, `seal`, `open`, `move`, `reset`) for ChaCha20-Poly1305 on
  BearSSL, OpenSSL and libsodium and for libsodium's other AEAD families and secretbox;
  `p_curve_engine<families::p256, backends::bearssl>` (`make`, `valid_point`, `sign`, `ecdh_x`,
  `verify`, `to_der`, `from_der`, `public_point`, `scalar`, `reset`); on libsodium
  `ed25519_engine`, `ed25519ph_engine`, `x25519_engine` (`base`, `agree`), `box_engine`,
  `sealed_box_engine`, `kx_engine`, `pem_engine` and the formats engines; libsecp256k1 has no
  engine: `curves/secp256k1*.hpp` call it directly; the entropy source is `detail/entropy.hpp`;
  `async.hpp` declares `async_engine<Backend>` (`make`, `generate`, `seal`, `open`, `sign`,
  `verify`, `agree`, each with the executor first and the token last), which only part B's browser
  backend will specialize.
- **The contract** lives in comments: what each engine provides, that the public function holds
  `detail::timing_guard` and the engine none, that `reset` wipes, that `move` leaves the source
  empty.
- **The table**: `doc/tables/crypto.json` lists every (family, algorithm, backend) cell with its
  targets, whether it is decided at run time, its constant-time lanes; `generate.py` writes
  `detail/capabilities.hpp` (the families, `detail::supported`, the refusals),
  `test/capabilities_test.cpp` and the pages. `supports_v<Algorithm, Backend>` reads
  `detail::supported` and needs no backend header, so a program can ask about a backend it does not
  link. `capabilities_test` checks that a present cell has a specialization; nothing checks the
  specialization's operations.

## Intent (owner, 2026-11-22)

Put the library's capabilities into concepts; a backend implements only the concepts it has. The
path is "2 built to become 1": internal concepts now (2), written against the real engines and
shaped as they would be published (names, minimal operations, error contract, no exceptions,
timing-guard expectations, allocation rules, thread safety, the opaque key), so that when the first
external backend appears they become the public backend interface (1) without being redesigned.

## Goals / Non-Goals

**Goals:** one concept per capability, checked against every backend's engines; the opaque key as a
first-class shape; one source of truth for what a backend implements, with the table checked
against it; refusals that name the missing capability; secp256k1 behind the same engine seam; a
written promotion criterion.

**Non-goals:** publishing the concepts now; new backends (part B's browser backend is its own
change and the first real consumer of the opaque and asynchronous shapes); changing any public
name, result or error; a run-time plugin or provider mechanism.

**Anti-goals:** a concept that encodes one backend's accidents (BearSSL's lack of point
compression, OpenSSL's contexts) instead of the capability; a concept that requires secret bytes in
memory for an operation that does not need them.

## Approaches

**A. Public concepts now (`crypto::backend`).** The interface would be right only by luck: no
external backend exists to test it against, every later correction becomes a breaking change, and
the documentation and stability policy would be written for nobody. OpenSSL 3's providers show the
cost of a public backend interface: the dispatch tables and parameter passing are a large stable
surface the project must carry for years. Rejected for now.

**B. Tables only (today).** The table and the generated refusals say what exists; nothing says what
an engine must provide, an incomplete engine fails inside a public function, and the opaque key has
no shape. Rejected.

**C. Internal concepts shaped for publication (chosen by the owner).** The concepts are real, checked
against every engine on every lane, and used by the public API's constraints, but they stay in
`detail`, free to change, until an external backend gives them a second implementor outside the
library. RustCrypto's public traits (`aead::Aead`, `signature::Signer`, `digest::Digest`) show the
benefit of a published capability interface with many implementors, and the cost of changing it
once published (a trait change ripples through every implementor crate): publication waits for the
second implementor, and the concepts are shaped for it from the start.

## Decisions

### D1. Where the concepts live and how they are named

`<webcpp/crypto/detail/backend/*.hpp>`, namespace `webcpp::crypto::detail::backend`, one header
per area (`symmetric.hpp`, `boxes.hpp`, `ed25519.hpp`, `x25519.hpp`, `p256.hpp`, `secp256k1.hpp`,
`encoding.hpp`, `entropy.hpp`, `asynchronous.hpp`) and `concepts.hpp` including them all. The
directory and namespace are the ones promotion turns into `<webcpp/crypto/backend/*.hpp>` and
`crypto::backend` (D9). Each concept takes the engine type it checks and the parameters that select
it, and is named after the capability, never after a backend: `backend::aead<Engine, Family>`,
`backend::p256_ecdsa_signing<Engine>`. Each family header includes its area's header, and no
concept header includes a backend.

### D2. The contract every capability shares

What a concept checks (C++ requirements):

- static member functions of the engine, on a state the engine names (`typename Engine::state`), as
  today;
- every function `noexcept`;
- results: `boost::system::result<void>` for an operation that can fail, `result<T>` where it
  returns a value, `bool` only for a predicate on public data (`valid_point`, `verify` keeps
  `bool` where today's engine has it, D7); every output written into a caller's fixed-size span;
- error codes from `crypto::error` only (`backend_failure` for a backend's unexplained failure,
  `invalid_*` for a refused input, `weak_public_key`, `authentication_failed`, `not_supported`
  for a run-time absence), never an exception;
- `move(target, source)` and `reset(state)` for every state type that holds anything.

What a concept states and the tests check (semantic requirements, in each concept's documentation
and in `capabilities_test`, the coverage tests and the lanes):

- **Timing guard**: the public function holds `detail::timing_guard`; an engine function never
  holds one and never calls a public function (the coverage tests of the data-independent-timing
  change already prove the first guard a call enters is its own).
- **Allocation**: an engine never allocates through the C++ global heap or any allocator of the
  program; a backend library's own internal allocations (OpenSSL's contexts) are the backend's,
  named in its table row; a failed allocation inside the library is `backend_failure` or
  `errc::not_enough_memory`, never an exception or a termination.
- **Thread safety**: engine functions may run concurrently on distinct states; one state is never
  shared between threads without the caller's synchronization; any global initialization a backend
  needs (`sodium_init`, libsecp256k1's self test) is idempotent and thread-safe, done by the engine.
- **Wiping**: `reset` wipes every secret the state holds and releases any handle; `move` leaves the
  source as `reset` does; a function that writes a secret into a caller's span writes it only on
  success.
- **Canary**: an engine reads its backend's canary before its first library call, so a program
  without the backend's library fails to link naming it.
- **Constant time** where the table marks the cell constant time on a target: the lanes trace it.

### D3. The opaque key

A key capability is split by what it does to the key:

| Capability | Meaning | Needs the secret in memory |
|---|---|---|
| `*_import` | make a state from secret bytes the program gives | yes, briefly |
| `*_generate` | make a state whose secret the backend draws | no |
| `*_public` | give the public key of a state | no |
| use (`*_signing`, `*_agreement`, ...) | operate with the state | no |
| `*_export` | give the secret bytes back | yes |

A state may be an opaque handle (a WebCrypto `CryptoKey` reference, an HSM object id, a Secure
Enclave or TPM handle); every use capability is written so that it reads the state only through
the engine. Today's `p_curve_engine::scalar` and the Ed25519 seed copy become `*_export`, and
today's `generate()`, which draws bytes with `random_device` then imports them, becomes: the
engine's `*_generate` when it has one, else `random_device` and `*_import` (R3). A backend whose
keys never leave it implements `*_generate`, `*_public` and the uses, and neither `*_import` nor
`*_export`; then `make(bytes)` and `secret()` are refused at compile time naming the missing
capability, and the formats change's private formats (`pkcs8`, a key pair's `raw`) are refused the
same way (its D7, now expressed by `*_export`). A non-extractable WebCrypto key is such a key; an
extractable one adds `*_export` through `exportKey`, which is asynchronous, hence D6.

A test-only stand-in opaque backend (`test/backends/opaque.hpp`) implements P-256 and X25519 with
handles into a table of keys it owns, `*_generate`, `*_public` and the uses only, so that the opaque
shape is proven before part B exists; part B's browser backend is its first real consumer.

### D4. The catalogue

| Area | Concept (`detail::backend::`) | Operations | Engines checked |
|---|---|---|---|
| Symmetric | `block_cipher<E, KeySize>` | `init`, `encrypt`, `decrypt`, `move`, `reset` | `block_engine` on OpenSSL, ctaes |
| | `gcm<E, KeySize>` | `init`, `start`, `run`, `tag`, `check`, `move`, `abandon`, `reset` | `gcm_engine` on BearSSL, OpenSSL, ctaes |
| | `gcm_incremental_open<E, KeySize>` | the incremental open's `start`, `authenticate`, `decrypt`, `verify`, `finish` | as GCM where offered |
| | `aead<E, Family>` | `init`, `seal(iv, ad, data, tag)`, `open(iv, ad, data, tag)`, `move`, `reset` | `aead_engine` for each family on BearSSL, OpenSSL, libsodium |
| | `authenticated_box<E, Family>` | `init`, `seal(iv, data, tag)`, `open(iv, data, tag)`, `move`, `reset` | secretbox on libsodium |
| Public-key encryption | `box<E, Family>` | `init(own, peer)`, `seal`, `open`, `move`, `reset` | `box_engine` on libsodium |
| | `sealed_box<E>` | `seal(recipient, data, out)`, `open(recipient, sealed, out)` | `sealed_box_engine` on libsodium |
| Key exchange | `kx<E>` | `derive_from_seed`, `client_session_keys`, `server_session_keys` over X25519 states | `kx_engine` on libsodium |
| Ed25519 | `ed25519_import`, `ed25519_generate`, `ed25519_public`, `ed25519_signing`, `ed25519_verifying`, `ed25519_export`, `ed25519_to_x25519` | per D3 | `ed25519_engine` on libsodium |
| | `ed25519ph<E>` | `begin`, `update`, `sign`, `verify` | `ed25519ph_engine` on libsodium |
| X25519 | `x25519_import`, `x25519_generate`, `x25519_public`, `x25519_agreement`, `x25519_export` | per D3; `agree` refuses an all-zero result | `x25519_engine` on libsodium |
| P-256 | `p256_import`, `p256_generate`, `p256_public`, `p256_point` (validate, compress, decompress), `p256_ecdsa_signing`, `p256_ecdsa_verifying`, `p256_ecdh`, `p256_export`, `ecdsa_der` | per D3 | `p_curve_engine<families::p256, B>` on BearSSL (and OpenSSL where the table has it) |
| secp256k1 | `secp256k1_core` (import, public, export, ECDSA, Schnorr), `secp256k1_recovery`, `secp256k1_ecdh`, `secp256k1_tweak`, `secp256k1_ellswift`, `secp256k1_musig`, `secp256k1_silent_payments` | one per libsecp256k1 module | `secp256k1_engine<Module, backends::secp256k1>` (D8) |
| Encoding | `base64<E>` | constant-time `encode` and `decode`, standard with padding and URL-safe without, into fixed spans | `pem_engine` on libsodium (renamed `base64_engine`) |
| Entropy | `entropy_source<E>` | `fill(span)` returning `result<void>` (`entropy_unavailable`) | the platform sources of `detail/entropy.hpp` |
| Asynchronous | `async_make<E, A>`, `async_generate<E, K>`, `async_seal<E, A>`, `async_open<E, A>`, `async_sign<E, K, S>`, `async_verify<E, S, P>`, `async_agree<E, K>`, `async_export<E, K>` | the shapes `async.hpp` documents: executor first, token last, completion signatures as the public functions' | `async_engine<backends::browser>` (part B), the stand-in |

The formats change's per-curve formats engines dissolve into these: a format codec needs a key's
`*_public`, `*_import`, `*_export`, `p256_point`, `ecdsa_der` and `base64`, and holds no engine of
its own.

### D5. Every engine is checked

Each backend header, after each specialization, asserts the concepts its table cells promise:
`static_assert(backend::aead<aead_engine<families::chacha20_poly1305, backends::bearssl>,
families::chacha20_poly1305>)`, and so on. `generate.py` writes, beside each cell, the list of
concepts the cell requires (from a map in `crypto.json`'s family rows: `"capabilities": ["aead"]`),
and `capabilities_test` checks both directions on every target where the backend's library
builds: a present cell's engine satisfies its concepts, and an engine that satisfies a family's
concepts has its cell present. A missing operation is a diagnostic naming the concept and the
operation (each concept is a conjunction of named sub-requirements, so compilers print which one
failed).

### D6. Asynchronous and opaque together

A backend that completes by itself (`asynchronous_backend_v`) implements the `async_*` concepts and
none of the synchronous ones; the public synchronous forms keep refusing with the name of their
`async_` form (crypto/sign-unified's D9). Export of an extractable opaque key is `async_export`,
the operation the formats change left to part B; a non-extractable key has none, and the private
formats are refused by type.

### D7. One source of truth, and the table (a ruling, R1)

The concepts are the truth of what a backend implements: a backend implements a capability exactly
when its engine satisfies the concept. The table stays, as the header-free declaration:

- `detail::supported` and `supports_v` stay generated from `crypto.json`, so that `supports_v` needs
  no backend header and can be asked of a backend the program does not link (a property the
  foundation requires and programs use to choose a backend). Deriving them from the concepts would
  need every backend's header, which a target without that library cannot include (OpenSSL on
  Emscripten, whose header refuses inclusion).
- The table's presence cells are **checked against** the concepts, not generated from them: D5's
  two-way check runs in `capabilities_test` on every target and lane where the backend builds, so
  the table cannot drift from the engines. Generating the cells would mean compiling every backend
  on every target at generation time, which the generator (a Python script run on one machine)
  cannot do.
- **Run-time cells** (availability decided by the CPU or the browser at run time: the foundation's
  `"runtime": true`) are present in the concepts and in `supports_v` (the code exists), and the
  table's `runtime` annotation says that `make` returns `not_supported` where the machine lacks it,
  as the foundation requires; the concept cannot express it, and a run-time test does.
- **Targets without a backend's library**: the cell is absent for that target; the backend header's
  inclusion refusal is the check (a message test), since no engine exists there to test.

### D8. secp256k1 behind an engine

The curve's headers call libsecp256k1 directly today. Their calls move, unchanged, behind
`detail::secp256k1_engine<Module, backends::secp256k1>`, one specialization per module (`core`,
`recovery`, `ecdh`, `extrakeys`, `ellswift`, `musig`, `silentpayments`), in
`backends/secp256k1.hpp`, so that the curve uses the seam every other curve uses, its modules have
concepts, and an alternative secp256k1 implementation (a hardware wallet's) is a backend like any
other. The static context, the self test, the canary and the no-allocation verification stay as they
are; the memory option's workspace for MuSig2 and silent payments is part of those concepts.

### D9. The public API's constraints

Each public type checks, in the member that needs it, the capability it uses: `basic_keypair<B>::make`
checks `*_import`, `generate` checks `*_generate` or `*_import` with entropy, `secret()` checks
`*_export`, `sign` checks `*_signing`, `agree` checks `*_agreement`, `ciphers::gcm<C>::seal` checks
`gcm`, and so on. The check is a `static_assert` whose message names the missing capability:
"webcpp::crypto: <backend> implements P-256 but not p256_export: its keys never leave it; secret()
is not available". Order of diagnostics: where the engine is not specialized at all (a backend
without the algorithm, not linked, header missing), the generated refusal of the primary fires
first, with today's messages (which name the backends that have it, the Jamfile line or the
header); the capability assertion speaks only to a specialized engine that lacks a capability. The
uniform-types concepts (`crypto::keypair`, `crypto::aead`, ...) are the public algorithm concepts;
the backend concepts are what their members require of a backend, and the names never collide
(`crypto::aead` versus `detail::backend::aead`).

### D10. The promotion criterion (part 1)

The concepts move from `detail::backend` to a public namespace `crypto::backend` (headers
`<webcpp/crypto/backend/*.hpp>`) when the first real external backend appears: an HSM, a Secure
Enclave or TPM binding, or a company's own library implemented outside webcpporg/crypto. At that
point, and not before:

- the concepts, the engine-and-state seam and the families' tags become public, with their
  documentation (each concept's semantic contract of D2 as text), unchanged in shape;
- a stability policy: the backend interface is versioned on its own (a
  `crypto::backend::interface_version`), additions are new concepts or optional operations, a
  removal or a changed signature is a major version of the interface, announced a release ahead;
- an example backend in the repository (`example/backend/`), built and tested on every lane,
  implementing a subset (the opaque P-256 of D3) as an external backend would;
- the table learns to take rows for backends it does not ship.

Nothing is redesigned at promotion: if the external backend needs a change, that change is made
first, in `detail`, as an ordinary change.

## Rulings

- **R1: the table is checked against the concepts, `supports_v` stays header-free** (D7). The owner
  asked that `supports_v` and `detail::supported` derive from the concepts; deriving them literally
  would require the backend's header to ask, which breaks asking about a backend the program does
  not link and cannot work on targets where the backend's header refuses inclusion. The two-way
  check makes the concepts the single source of truth while keeping the header-free answer.
- **R2: secp256k1 moves behind an engine seam** (D8), the only structural change outside the
  concepts themselves, so that "a backend implements only the concepts it has" holds for every
  curve.
- **R3: `generate()` uses the engine's `*_generate` when the backend has one**, else
  `random_device` and `*_import` as today; on today's backends (none has `*_generate` but WebCrypto
  and the stand-in) nothing changes.
- **R4: the formats engines dissolve** (D4) into key and encoding capabilities; the PEM family's
  engine becomes `base64_engine` under the family `families::pem`, which keeps its table row.
- **R5: the concepts' headers live in `detail/backend/`**, not `detail/capabilities/`, which would
  sit beside the generated `detail/capabilities.hpp` and read as its part.

## Before and after

```cpp
// Before: a backend engine, its contract in a comment.
template <>
struct aead_engine<families::chacha20_poly1305, backends::bearssl> {
    using state = aead_state<families::chacha20_poly1305, backends::bearssl>;
    [[nodiscard]] static boost::system::result<void> init(state&, std::span<std::uint8_t const, 32>) noexcept;
    [[nodiscard]] static boost::system::result<void> seal(state&, /* iv, ad, data, tag */) noexcept;
    [[nodiscard]] static boost::system::result<void> open(state&, /* iv, ad, data, tag */) noexcept;
    static void move(state&, state&) noexcept;
    static void reset(state&) noexcept;
};

// After: the same engine, checked against the capability its cell promises.
template <>
struct aead_engine<families::chacha20_poly1305, backends::bearssl> { /* unchanged */ };
static_assert(detail::backend::aead<aead_engine<families::chacha20_poly1305, backends::bearssl>,
                                    families::chacha20_poly1305>);

// The concept, shaped for publication.
template <class Engine, class Family>
concept aead = requires(typename Engine::state& s, typename Engine::state& other,
                        std::span<std::uint8_t const, Family::key_size> key,
                        std::span<std::uint8_t const> iv, std::span<std::uint8_t const> ad,
                        std::span<std::uint8_t> data, std::span<std::uint8_t, Family::tag_size> tag,
                        std::span<std::uint8_t const, Family::tag_size> expected) {
    { Engine::init(s, key) } noexcept -> std::same_as<boost::system::result<void>>;
    { Engine::seal(s, iv, ad, data, tag) } noexcept -> std::same_as<boost::system::result<void>>;
    { Engine::open(s, iv, ad, data, expected) } noexcept -> std::same_as<boost::system::result<void>>;
    { Engine::move(s, other) } noexcept;
    { Engine::reset(s) } noexcept;
};

// Before: supports_v, generated from the table, checked by nothing but a specialization's existence.
template <class Algorithm, class Backend>
inline constexpr bool supports_v =
    detail::supported<typename Algorithm::family_type, typename Algorithm::name_type, Backend>;

// After: the same header-free trait, and the generated test proves it equals the concepts.
static_assert(detail::supported<families::p256, families::p256, backends::bearssl> ==
              (detail::backend::p256_import<p256_engine_bearssl> &&
               detail::backend::p256_ecdsa_signing<p256_engine_bearssl> && /* the row's list */ true));

// Before: an opaque backend could not be written (sign reads the scalar from the state).
// After: an opaque P-256 engine, its state a handle.
struct opaque_p256_engine {
    struct state { std::uint32_t handle = 0; };
    static boost::system::result<void> generate(state&) noexcept;                      // p256_generate
    static void public_point(state const&, std::span<std::uint8_t, 65>) noexcept;      // p256_public
    static boost::system::result<void> sign(state const&, std::span<std::uint8_t const, 32>,
                                            std::span<std::uint8_t, 64>) noexcept;     // p256_ecdsa_signing
    static void move(state&, state&) noexcept;
    static void reset(state&) noexcept;                                                // releases the handle
};  // no import, no export: make(bytes) and secret() refuse, naming p256_import and p256_export
```

## Risks / Trade-offs

- **Concepts too narrow or too wide** for an external backend: mitigated by the opaque stand-in now
  and part B next, two shapes unlike today's engines, before anything is published.
- **Diagnostics**: a failed concept's message can be long; each concept is a conjunction of named
  sub-concepts so the first failing operation is named, and the public assertion's text names the
  capability in plain words.
- **Churn**: every backend header changes, and the formats engines dissolve; the change is internal
  and covered by the existing tests, the coverage tests and the lanes, plus the two-way check.
- **secp256k1's seam** is a mechanical move of calls; the no-allocation and constant-time tests of
  its modules guard it.

## Migration Plan

crypto/sign-unified, then crypto/uniform-types, then crypto/formats, then this change, on branches
`crypto/backend-concepts` of webcpporg/crypto, webcpporg/proposals and the superproject. Part B's
browser backend implements the asynchronous and opaque concepts when it is written. Local
verification on every lane, merged by the controller, one CI at the end. webcpp::trystero's phase 3
does not depend on it: no public name changes.

## Open Questions

- **O1** (R1): keep `supports_v` header-free from the table, checked two ways against the concepts,
  or make it concept-derived and require the backend's header to ask?
- **O2** (R2): move secp256k1's direct calls behind `secp256k1_engine` in this change, or keep
  secp256k1 outside the seam as a single-backend curve?
- **O3** (R3): when a backend can both import and generate (OpenSSL), should `generate()` prefer the
  backend's own generation, or keep crypto's `random_device` path for one behaviour on every
  backend?
