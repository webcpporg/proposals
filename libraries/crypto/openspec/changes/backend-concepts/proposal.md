## Why

webcpp::crypto reaches its backends (BearSSL, OpenSSL, libsodium, ctaes, libsecp256k1, and the
browser's WebCrypto in part B) through engines: for each algorithm, a family header declares a
primary `detail::<family>_engine` whose body is the generated refusal, and each backend's header
specializes it (`detail::p_curve_engine<families::p256, backends::bearssl>`,
`detail::aead_engine<Family, backends::sodium>`, `detail::gcm_engine<backends::openssl, KeySize>`,
...). What an engine must provide is written only in comments, and what a backend has is written
only in the capability table, `doc/tables/crypto.json`, from which `detail::supported` and
`supports_v` are generated. Nothing checks that a specialization has the operations its cell
promises, an engine that lacks one fails deep inside a public function with the compiler's message,
and a key that never leaves its backend (WebCrypto's non-extractable `CryptoKey` today; an HSM, a
Secure Enclave or a TPM tomorrow) has no shape at all: the engines assume a key's bytes are in
memory (`p_curve_engine::scalar` copies the secret out).

On 2026-11-22 the owner decided to put the library's capabilities into concepts, one per
capability, so that a backend implements only the concepts it has, by the path "2 built to become
1": internal concepts now, shaped as they would be published, promoted to a public namespace when
the first external backend appears, with nothing redesigned then.

## What Changes

- **A catalogue of capability concepts** in `detail::backend` (headers
  `<webcpp/crypto/detail/backend/*.hpp>`, one per area), one per capability: the AES block
  cipher, GCM (one-shot and incremental), each AEAD family, the authenticated boxes (secretbox,
  crypto_box), the sealed box, crypto_kx, Ed25519 (import, generate, sign, verify, export, to
  X25519), Ed25519ph, X25519 (import, generate, agree, export), P-256 (import, generate, public
  point, ECDSA sign and verify, ECDH, export, DER), secp256k1's modules (core keys and signatures,
  recovery, ECDH, tweaks, ElligatorSwift, MuSig2, silent payments), constant-time base64, the
  entropy source, and the asynchronous engine's operations. Each names its operations, their
  results (`result<void>`, `result<T>` or `bool` for a public predicate, outputs into fixed spans),
  `noexcept`, and states the semantic contract the compiler cannot check: who holds the timing
  guard, what may allocate, thread safety, wiping.
- **The opaque key**: key capabilities are split into import, generate, use and export, and an
  engine state may be an opaque handle; a backend whose keys never leave it implements generate and
  use without import or export. A stand-in opaque backend in the tests, and part B's browser backend
  when it lands, prove the shape.
- **Every backend engine is checked**: each backend header asserts, for every engine it
  specializes, the concepts its cells promise; a cell present in the table without its concepts,
  or concepts satisfied without the cell, stops the build of `capabilities_test`.
- **One source of truth**: the concepts are the truth of what a backend implements; the table
  stays the header-free declaration `supports_v` reads (so a program can ask about a backend it
  does not link) and is checked against the concepts on every target and lane; run-time cells and
  targets without a backend's library keep their table annotations.
- **The public API is constrained by the capabilities**: `p256::basic_keypair<B>`, `gcm<B>`,
  `aead<F, B>` and the others check the capability each member needs, and an incomplete backend
  stops the build with a message naming the missing capability; the generated refusals for a
  backend without the algorithm, not linked, or whose header is missing stay as they are.
- **secp256k1 behind an engine**: its direct libsecp256k1 calls move behind
  `detail::secp256k1_engine<Module, backends::secp256k1>`, so that its modules have concepts too.
- **The promotion criterion**, written down: when the first real external backend appears, the same
  concepts move to a public namespace (`crypto::backend`) with a stability policy, documentation and
  an example backend; nothing is redesigned then.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: what a backend implements is a set of checked concepts, the capability table is
  checked against them, and an incomplete backend is refused by naming the missing capability.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** no public name changes; every backend header and every family header changes
  internally. Order: crypto/sign-unified, then crypto/uniform-types, then crypto/formats, then this
  change (it touches the engines both reshape). webcpp::trystero's phase 3 of the 0.27 port does
  not wait for it.
- **Implementation:** webcpporg/crypto (`include/webcpp/crypto/detail/backend/`, every
  `backends/*.hpp`, every family header, `curves/secp256k1*.hpp`, `async.hpp`,
  `doc/tables/generate.py` and its generated files, `test/capabilities_test.cpp`, message tests,
  the stand-in opaque backend), the superproject's pointer.
- **Status:** proposed.
