## ADDED Requirements

### Requirement: Backend capabilities: one concept per capability
`<webcpp/crypto/detail/backend/*.hpp>` SHALL declare, in namespace `webcpp::crypto::detail::backend` and including no backend, one concept per capability of the catalogue in `design.md` D4 (block cipher, GCM and its incremental open, each AEAD family, authenticated box, box, sealed box, kx, Ed25519 and X25519 and P-256 split into import, generate, public, use and export, Ed25519ph, P-256's point and DER, secp256k1 split as the other curves are, with its protocol capabilities, constant-time base64, entropy, and the asynchronous operations), each requiring static `noexcept` functions of the engine over its `state`, results of `boost::system::result<void>` or `result<T>` (or `bool` for a predicate on public data), outputs into fixed-size spans, and `move` and `reset` for a state that holds anything. Each concept's documentation MUST state its semantic contract: the public function holds the timing guard and the engine none, the engine allocates nothing through the C++ heap or the program's allocators, engine functions are safe on distinct states concurrently, `reset` wipes secrets and releases handles, the backend's canary is read before its first call. Each backend header MUST `static_assert` every concept its table cells promise, after each specialization.

#### Scenario: Every backend checked
- **WHEN** each backend's header is compiled on every target where its library builds
- **THEN** every engine it specializes satisfies the concepts its cells promise, and the build fails naming the concept and the operation otherwise

### Requirement: Backend capabilities: the opaque key
Key capabilities SHALL be split into import (a state from secret bytes), generate (a state whose secret the backend draws), public (the public key of a state), use (sign, verify, agree) and export (the secret bytes back), and no use capability MUST require the secret in memory. A backend that implements generate, public and the uses without import or export MUST be usable through the public key pair types, whose `make(bytes)`, `secret()` and private formats then stop the build with a message naming the missing capability. A test-only opaque backend whose states are handles MUST implement P-256 and X25519 this way, in a synchronous and an asynchronous form, so that the asynchronous concepts are tested; it MUST stay outside the capability table and its checks. `generate()` MUST use a backend's own generation when it has one, and `random_device` with import only for a backend that can only import.

#### Scenario: A key that never leaves its backend
- **WHEN** a P-256 key pair on the test-only opaque backend is generated, signs a message and agrees with a peer
- **THEN** its signature verifies on BearSSL, its agreement equals the peer's, and a call of `secret()` or `make(bytes)` on it does not compile, naming `p256_export` and `p256_import`

### Requirement: Backend capabilities: refusals name the missing capability
Each public member that calls an engine SHALL check, in a `static_assert`, the capability it uses, with a message naming the backend, the algorithm and the missing capability; where the engine is not specialized at all, the generated refusal of the primary engine MUST fire first with its existing message.

#### Scenario: Two diagnostics, in order
- **WHEN** a program calls `curves::ed25519::basic_keypair<backends::bearssl>::make(seed)`, and then `secret()` on a key pair of the opaque test backend
- **THEN** the first stops with the generated message naming the backends that have Ed25519, and the second with the message naming the missing export capability

### Requirement: Backend capabilities: secp256k1 behind engines, split into capabilities
The calls of `curves::secp256k1` and its modules into libsecp256k1 SHALL go through `detail::secp256k1_engine<Capability, backends::secp256k1>`, one specialization per capability (import, generate, public, export, ECDSA signing and verifying, Schnorr signing and verifying, recoverable, ECDH, tweaks, ElligatorSwift, MuSig2, silent payments), each checked against its concept, keeping the static context, the self test, the canary and no allocation; no single concept MUST cover the curve's whole interface.

#### Scenario: Nothing allocates
- **WHEN** the counting tests run secp256k1's verifications through the engine
- **THEN** the counts are zero, as before

### Requirement: Backend capabilities: the promotion criterion
The documentation of `detail::backend` SHALL state that its concepts move unchanged to a public namespace `crypto::backend`, with a stability policy, documentation and an example backend, when the first real external backend appears, and not before.

#### Scenario: The criterion written down
- **WHEN** the guide's chapter on backends is read
- **THEN** it states the criterion, the namespace the concepts would take, and that nothing is redesigned at promotion

### Requirement: Backend capabilities: partial support
`supports_v<Algorithm, Backend>` SHALL be true when the backend has the algorithm's use operations (sign and verify, agree, seal and open, or the algorithm's operation), whatever it has of import, generate and export, and `supports_capability_v<Capability, Backend>` SHALL answer each capability of the catalogue from the table's one cell per capability, needing no backend's header. The check of the table against the concepts MUST, in its reverse direction, count only real specializations of the shipped backends, marked as such, never a refusing primary engine.

#### Scenario: P-256 without export
- **WHEN** a backend's P-256 engines implement generate, public, signing, verifying and agreement but not export
- **THEN** `supports_v` of P-256 is true for it and `supports_capability_v<capabilities::p256_export, B>` is false

## MODIFIED Requirements

### Requirement: Capability table: one committed source
The library SHALL hold its capability table in version 0.1's `doc/tables/crypto.json`, listing for every algorithm, by family and name, each backend's targets (native, wasip2, wasip3, emscripten), whether its availability is decided at run time, whether it is constant time on each target, the family's default backend per target with its reason, and the increment that adds it. Version 0.1's generator `doc/tables/generate.py` MUST generate from it, beside version 0.1's pages, `include/webcpp/crypto/detail/capabilities.hpp`, `include/webcpp/crypto/supports.hpp`'s table and `test/capabilities_test.cpp`; a cell MUST be backed by a test that runs on its targets, each family row MUST list the backend capability concepts its cells require, `capabilities_test` MUST check on every target and lane where a backend builds that a present cell's engines satisfy those concepts and that engines satisfying them have their cell present, and `--check` MUST fail when a generated file differs from what the table gives. No `meta/capabilities.json` and no superproject tool of crypto's MAY hold a second copy of these facts.

#### Scenario: A hand edit of the generated header
- **WHEN** a line of `detail/capabilities.hpp` is changed by hand and `python3 libs/crypto/doc/tables/generate.py --check` runs
- **THEN** it fails, naming the file

#### Scenario: A cell without its specialisation
- **WHEN** the table marks `ed25519` present on `backends::botan` and no Botan engine for it exists
- **THEN** `capabilities_test` fails to compile on the targets where the cell is present

#### Scenario: An engine missing an operation
- **WHEN** BearSSL's ChaCha20-Poly1305 engine loses its `open`
- **THEN** `capabilities_test` and BearSSL's header fail to compile, naming `detail::backend::aead` and `open`

### Requirement: Capability table: the supports trait and run-time availability
`webcpp::crypto::supports_v<Algorithm, Backend>` SHALL be `true` exactly where the table marks the backend as having the algorithm on the current target, needing no backend's header, and the table's presence cells MUST equal, on every target where the backend builds, whether the backend's engines satisfy the algorithm's capability concepts. A cell whose availability is decided at run time MUST be present in the trait, and its `make` or `async_make` MUST return `boost::system::errc::not_supported` on a machine or browser that lacks it, before reading a key byte.

#### Scenario: AES256-GCM on libsodium without AES instructions
- **WHEN** `ciphers::aes256_gcm_sodium::make(key)` runs where `crypto_aead_aes256gcm_is_available()` returns 0
- **THEN** it returns `not_supported`, and `supports_v<aes256_gcm_sodium, backends::sodium>` is `true` natively and `false` on WebAssembly
