# webcpp::crypto's roadmap: the genesis's increments 3 to 5 and the foundation, in one order

Written 2026-11-17, from the owner's decision of that day. It rules over the order of work in
`plan.md` (this directory), `../tasks.md`, `../../genesis/reference/plan.md` and
`../../genesis/tasks.md`; each of those carries a note that points here. Where a design
decision is cited, `D<n>` is the foundation's `design.md` and `genesis D<n>` the genesis's.

## The owner's decision of 2026-11-17

1. The genesis's increments 3 to 5 are built **without trystero's migration**: its Tasks 31 and
   32, and anything that exists only for trystero, are dropped. Increment 4
   (ChaCha20-Poly1305) and increment 5 (Botan) are built.
2. The foundation, version 0.2 onward, is built: its twelve increments.
3. The rules that bind every task: measured tables only from release builds; primitives only
   (D24); the documentation as the owner's requirements of 2026-11-15 say (D19); no external
   JavaScript or TypeScript SDK is named in the proposals, the code or the documentation beyond
   the noble and scure libraries the foundation's research already cites.
4. webcpp::hash's decision of 2026-11-16 (hash's design D33: one default per algorithm, no
   measured default, no profile) is re-applied to crypto: this roadmap proposes the same rule
   (D32); the owner answered the questions it raised the same day (below, "The owner's
   answers").

## Where the code stands (2026-11-17)

- **crypto version 0.1 is merged:** `webcpporg/crypto` `main` at `d242be8`, recorded by the
  superproject's `main` (`3fa7ca5`). It holds `ciphers::{aes128, aes192, aes256, gcm,
  aes128_gcm, aes256_gcm}`, `curves::secp256k1` (key pairs, BIP340, ECDSA, ECDH, strict DER,
  low-S, the x-only tweak), `random_device`, `equal`, the backends BearSSL, OpenSSL (the
  source-built 3.5.9), ctaes and libsecp256k1 with their three layers (OpenSSL has no canary),
  and a page whose tables `doc/tables/generate.py` writes from `doc/tables/crypto.json`, the
  test Jamfiles, the message tests and the superproject's fixtures. The defaults are v0.1's
  rule with a stated reason per target (`doc/generated/defaults.adoc`), and the two defines
  `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and `_OPENSSL`. Sizes, throughput and measured
  constant time read "not measured yet".
- **Not yet recorded as built:** the CI of the library's `main` (its last run was cancelled)
  and of the superproject's `main` (queued) must be green on those commits first (genesis task
  7.3, proposals' CONTRIBUTING.md "Implemented").
- **The foundation's base is ready:** the superproject's `main` holds `third_party/libs/libsodium`
  at `77e1ce5` and `/third_party//sodium` (webcpp::hash), and webcpp::hash with
  `backends::hash2` as every key derivation's default.
- **The release-build rule exists in the superproject** (`69bf8ab`): `webcpp.size-table`
  requires `<variant>release` (and, by C36 of 2026-11-17, serves plain linked programs on every target through foundation Task 11', not only Emscripten page programs), and hash's `bench/collect.py` refuses a program or data not built
  at `variant=release`. crypto's size and throughput tables use the same tools.

## Every overlap, resolved

| Item | Where it goes | Why |
| --- | --- | --- |
| Genesis Task 31 (trystero's sealer and `random`) | dropped | the owner's decision; trystero keeps its own sealer until a change of trystero's own adopts crypto |
| Genesis Task 32 (trystero's signer) | dropped | the same; its build half (Task 30a) is in version 0.1 |
| Genesis Task 33 (the memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake) | stream M, task M3, **moved into crypto's own `test/openssl_memory/`** | it proves the option in any webcpp process that also holds Beast's TLS or libdatachannel's DTLS (webcpp::beast, webcpp::webrtc), not only trystero's; the owner's answer Q6 |
| Genesis Task 34 (increment 3 whole) | stream M, task M4 | what is left of increment 3 is the memory option |
| Genesis Tasks 15 and 16 (the memory option) | stream M, tasks M1 and M2 | Task 33 needs them, and D16 routes OpenSSL's per-call allocations through them |
| Genesis Task 35 (ChaCha20-Poly1305 on BearSSL and OpenSSL) | stream A, foundation **Task 8b**, re-cut onto `ciphers::aead<Family, Backend>` | the foundation's Task 8 replaces the genesis's `chacha_state` and `chacha_engine` by `aead_state` and `aead_engine`; building Task 35 first would write traits Task 8 deletes. Version 0.2 then has ChaCha20-Poly1305 on libsodium, BearSSL and OpenSSL, agreeing |
| The foundation's ChaCha20-Poly1305 cells of increments 3 (BearSSL) and 8 (OpenSSL) | moved into Task 8b | the same work, done once |
| Genesis Task 36 (increment 4 whole) | foundation Task 16 | increment 4 closes with version 0.2 |
| Genesis Task 37 (Botan found on the machine) | **superseded**, not built | D15: Botan is a pinned submodule built by b2 |
| Genesis Task 38 (the Botan backend: AES block, AES-GCM, ChaCha20-Poly1305, exceptions only) | stream 7 (foundation increment 7), step 3, first cells | genesis D19's exceptions rule stands (D15); Botan serves native and Emscripten, and WASI as the spike finds |
| Genesis Task 39 (Botan in every native lane, three-way agreement, constant time) | stream 7, step 4 | the same work over the submodule |
| Genesis Task 20 (constant time) | stream A, foundation Task 14, **whole**: the harness, v0.1's backends (BearSSL, OpenSSL with assembly, ctaes, libsecp256k1's `ctime_tests.c`) and the new code | D18 already absorbs it; v0.1's paths are measured in the same lane |
| Genesis Task 22 (sizes, throughput, `runners.yml`) | stream A, foundation Task 11 **re-cut**: the own lane `sizes` through `webcpp.size-table` (release builds only), a throughput page measured by hand from release builds; no `runners.yml`, no ranking | D32: no measurement decides anything |
| Genesis Task 23 (the browser oracle, the audio worklet, throughput in Chrome) | stream 4 (foundation increment 4) | D13 |
| Genesis Task 25b (the nightly level) | stream N, unchanged, owner gate O2 | independent of the code |
| Genesis Task 9's NIST CAVP half | stream N, owner gate O3 | independent of the code |
| Foundation Task 4 (`meta/capabilities.json`, `tools/crypto/capabilities.py`) | **re-cut onto version 0.1's `doc/tables/crypto.json` and `doc/tables/generate.py`** | D33: one source, the one v0.1 built |
| Foundation Task 5 (profiles, overrides, rankings) | **replaced** by Task 5' (one default per family and target, by a stated reason) | D32 |
| Foundation Task 11 (`measure.py`, `measurements.json`, `rankings.hpp`) | replaced as above | D32 |
| Foundation Task 15 (`tools/crypto/docs.py`) | `doc/tables/generate.py` grows | D33 |
| The heavy column measured above 1 ms (D23) | a stated list (D32) | D32; the owner's answer Q5 |
| The webcpp-wide umbrella `WEBCPP_PROFILE_*` | removed | hash no longer reads it (hash's D33); nothing would |

## The defaults (D32), one per family and target

No define, no profile and no measurement chooses a default; the link set does not change it.
A family whose default backend is not linked stops at compile time with the backend's "needs"
message (the Jamfile line, and the type to write to name another backend). Every other backend
is reached by naming it in the type.

| Family | Default | Reason |
| --- | --- | --- |
| AES-GCM (`gcm`, `aes128_gcm`, `aes256_gcm`) | native: OpenSSL; wasip2, wasip3, Emscripten: BearSSL | v0.1's reasons: OpenSSL's assembly uses the CPU's AES and carry-less multiplication on x86-64 and ARMv8, where BearSSL has them on x86 only; on WebAssembly BearSSL is constant time by design where OpenSSL is built `no-asm`, and OpenSSL is not built for Emscripten. Natively OpenSSL whatever is linked (the owner's answer Q2) |
| AES block cipher | none (as v0.1) | a raw block cipher is chosen deliberately |
| ChaCha20-Poly1305 (IETF), its original form, XChaCha20-Poly1305 | libsodium on every target | the one backend that has all three on all four targets, constant time by design, and the cipher under `hybrid::box`, `secretbox` and `secretstream` |
| Ed25519, Ed25519ph, X25519, `kx`, boxes, sealed boxes, secretbox, secretstream, AEGIS, MACs, stream ciphers, ristretto255, edwards25519, ML-KEM-768, X-Wing, the root's utilities | libsodium on every target | the only backend of most of them; for Ed25519 and X25519, the strict verification policy (D31) is libsodium's rules and it builds on all four targets |
| secp256k1 and its modules | libsecp256k1 | the only backend |
| P-256, P-384, P-521 ECDSA and ECDH, their key formats; RSA | BearSSL on every target | the one backend on all four targets, constant time by design (`i31`, `m31`, `m62`), allocating nothing |
| AES-CTR, AES-CBC, AES-CCM, AES-EAX | BearSSL until increment 8 adds OpenSSL's cells, then AES-GCM's rule | the same reasons as AES-GCM; the change at increment 8 is a line of its changelog |
| AES-OCB, AES-KW, AES-KWP, AES-SIV, AES-GCM-SIV, CMAC, Ed448, X448, brainpool, ML-KEM-512 and 1024, ML-DSA, SLH-DSA, HPKE | native, wasip2, wasip3: OpenSSL; Emscripten: Botan | OpenSSL is the source-built provider on those targets; Botan is the one that builds for Emscripten |
| BLS12-381 | blst | the only backend |
| KZG | c-kzg-4844 | the only backend |
| BN254 | mcl | the only backend |
| Shamir | sss | the only backend |
| The asynchronous family | on Emscripten, the browser for what WebCrypto serves; elsewhere the family's default, posted | D13, as hash's `default_async_backend` |

The owner approved this table as proposed (Q4).

## The order, as streams a controller can run in parallel

Every stream is a branch of the superproject from `origin/main` with the library's branch of
the same name; each opens a pull request into `webcpporg/crypto` and one into the superproject,
which the owner merges. A stream whose tasks are outlined (foundation increments 2 to 12)
writes them in full task form in `plan.md` first, in a proposals pull request, before code.

### Wave 1: start now, three streams in parallel

| Stream | Branch | Work | Depends on | Version |
| --- | --- | --- | --- | --- |
| **A** | `crypto/foundation-1` | foundation increment 1 re-cut (`../tasks.md` group 1): libsodium's core, `secret<N>`, errors 15 to 20, the capability table on `crypto.json`, the defaults by reason, Ed25519 with its policy and twelve cases, X25519, SPKI, PKCS#8 and PEM, `ciphers::aead`, the three ChaCha AEADs on libsodium and **ChaCha20-Poly1305 on BearSSL and OpenSSL (genesis increment 4)**, boxes, `kx`, threads, vectors, the constant-time lane whole (genesis Task 20), the sizes lane and the throughput page from release builds (genesis Task 22's remainder), the documentation | `origin/main` (ready) | 0.2 |
| **B** | `crypto/secp256k1-modules` | foundation increment 2 (`../tasks.md` group 2) | stream A's branch once A's Task 3 (`secret<N>`, which `ecdh`, the tweaks and ElligatorSwift return) is committed; its capability rows after A's Task 4'; onto `main` when A merges, and it merges after A | 0.2 |
| **M** | `crypto/openssl-memory` | genesis Tasks 15, 16, 33 (moved into crypto's tests) and 34 (`../../genesis/tasks.md` group 4) | `origin/main` (ready) | the version open when it merges |

Inside stream A, the controller runs the tasks in these groups; a group starts when the one
before it is committed, and the tasks of a group run in parallel (each a subagent, each
committing only its own files, the controller merging the shared ones):

1. A1, in order: Task 1 (baseline), Task 2 (`backends::sodium`), Task 3 (`secret<N>`, errors),
   Task 4' (the capability table on `crypto.json`), Task 5' (the defaults, by the owner's
   answers Q1 to Q4).
2. A2, in parallel: Task 6 with 6b (Ed25519); Task 7 (X25519); Task 8 then 8b (the AEAD
   template, libsodium's ChaCha AEADs, then BearSSL's and OpenSSL's ChaCha20-Poly1305);
   Task 12 (threads); Task 11' (the sizes lane and the throughput page, starting with v0.1's
   operations); Task 14's harness over v0.1's backends.
3. A3, in parallel: Task 7b (formats, after 6 and 7); Task 9 (boxes, after 7 and 8); Task 10
   (`kx`, after 7); Task 13 (vectors, libsodium's suite, node's oracle); Task 14 over the new
   code.
4. A4: Task 15 (the documentation), then Task 16 (increment 1 whole, with genesis increment
   4's review items).

Shared files in stream A (`config.hpp`, `crypto.hpp`, `test/Jamfile`, `doc/tables/crypto.json`
and what it generates) are written by one task at a time; the controller merges them and runs
`generate.py` after every merge, never merging a generated file by hand.

Stream M touches `backends/openssl.hpp`, `error.hpp` (values 13 and 14) and
`test/openssl_memory/`; it conflicts with stream A only in those two headers and in
`everything_test.cpp`: whichever merges second rebases, and if A merges second its Task 8b adds
ChaCha20-Poly1305's case to `everything_test.cpp`, if M merges second its M2 adds it.

### Wave 2: after stream A merges, eight streams in parallel

| Stream | Branch | Foundation increment | Notes |
| --- | --- | --- | --- |
| 3 | `crypto/bearssl-rest` | 3, without ChaCha20-Poly1305 (moved to 8b) | P-curves, DER, key forms, AES modes, RSA |
| 4 | `crypto/browser` | 4 | absorbs genesis Task 23 (the oracle for every algorithm on `main`, the audio worklet, Chrome's throughput page from release builds) |
| 5 | `crypto/sodium-rest` | 5 | |
| 6 | `crypto/blst` | 6 | unblocks stream 9 |
| 7 | `crypto/botan` | 7 = **genesis increment 5** | starts with the WASI spike; its first cells are the genesis's (AES block, AES-GCM, ChaCha20-Poly1305, three-way agreement, constant time), then the rest; unblocks webcpp::hash's increment 6 and stream 12's WebAssembly cells |
| 8 | `crypto/openssl-rest` | 8, without ChaCha20-Poly1305 (moved to 8b) | changes the AES modes' native default to OpenSSL (D32); unblocks stream 12 |
| 10 | `crypto/mcl` | 10 | starts with the spike |
| 11 | `crypto/sss` | 11 | starts with the MSVC spike |

### Wave 3: as dependencies merge

| Stream | Branch | Foundation increment | Starts when |
| --- | --- | --- | --- |
| 9 | `crypto/kzg` | 9 | stream 6 merges |
| 12 | `crypto/hpke` | 12 | stream 8 merges; its WebAssembly cells when stream 7 merges |

### Independent of the code, owner-gated

| Stream | Work | Gate |
| --- | --- | --- |
| N | genesis Task 25b (the CI's nightly level) and Task 9's NIST CAVP half | O2, O3 |
| R | the proposals' records: genesis task 7.3 (version 0.1 built), then each version | the CI green on both `main`s |

### Merging

A stream merges when it is green and reviewed. When two are ready at once, the value order
decides (A, B, then 3, 4, 5, 6, 7, 8, 10, 11, then 9, 12); a ready stream never waits for an
earlier one unless its rebase conflicts. After every merge each open stream rebases, merges
`crypto.json`'s rows as JSON, regenerates with `generate.py`, and runs its lanes again.
Version 0.2 is A and B (and M if merged by then); after it, each merged increment is the next
minor version, the owner naming each release.

## Rules every stream keeps

- Each task's test is written first and seen failing; lanes native, gcc-15, wasip2, wasip3 and
  Emscripten from scratch, exceptions on and off; MSVC and Apple clang in the CI.
- Size and throughput tables are built only from release builds (`webcpp.size-table`, and a
  collect script that refuses a program or data not built at `variant=release`, as hash's
  `bench/collect.py`); they are dated references and decide nothing.
- A protocol's rules stay out (D24). Documentation per D19, its tables generated from
  `crypto.json` and the lanes' data, with the default per target (never per profile).
- Commits under the contributor's own identity, no trailer, English, no em dash, staged by path.

## The owner's answers (2026-11-17)

The owner answered every question this roadmap raised on the day it was written; each answer
is the recommendation, and `../design.md` D32 to D34 and the genesis's design record them.

- **Q1. The rule: adopted.** crypto takes webcpp::hash's D33 rule (D32): no profile, no
  umbrella, no shared define, no per-family override and no measured default; one default per
  family and target, chosen for a stated reason; every other backend named in the type.
- **Q2. AES-GCM's native default: OpenSSL whatever is linked.** A reader of `aes256_gcm` knows
  its backend from the target alone. A native program that links only BearSSL stops at compile
  time with the fix: link `/webcpp/crypto//openssl`, or write
  `gcm<aes256<backends::bearssl>>`.
- **Q3. Version 0.1's two defines stop the build.** `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and
  `_OPENSSL` stop compilation with an `#error` that names the type to write, since a silently
  ignored define would change which code a program runs.
- **Q4. The default table: approved as proposed.** libsodium for the ChaCha AEADs on every
  target; BearSSL for the P-curves and RSA on every target; the AES modes on BearSSL until
  increment 8, then AES-GCM's rule; OpenSSL on native and WASI, and Botan on Emscripten, for the
  families only those two have.
- **Q5. Heavy operations: a written list.** The stated list of D32 decides which operations get
  an `async_` variant on a thread; the measured "over 1 ms" column is not built, so that a pin
  move never adds or removes an API.
- **Q6. Genesis Task 33: kept,** in crypto's own tests (`test/openssl_memory/`), since Beast's
  TLS and webcpp::webrtc's DTLS share a process with crypto as trystero's did.
- **Q7. The genesis's status:** `implementing` now (this pull request sets it), and
  `implemented` once version 0.1 and stream M are merged with the CI green on both `main`s; the
  nightly level and NIST's archives (O2, O3) go with it if decided by then, or move to a change
  of their own.
- **Q8. The memory option is native only (2026-11-17).** OpenSSL's memory option (genesis Tasks
  15, 16, 33 and 34) is refused on every WebAssembly target, WASI included (wasip2, wasip3 and
  Emscripten). `/webcpp/crypto//openssl` is a separate matter: OpenSSL builds on WASI, so the
  alias is refused on Emscripten alone.
- **Q9. Size tooling reaches every target (2026-11-17, C36).** The superproject's release-only
  `webcpp.size-table` extends from Emscripten page programs to plain linked programs on every
  target; crypto's own lane `sizes` (foundation Task 11') builds on that.
- Still open from before: O1 (a ZIP-215 verifier), O2 (the nightly level), O3 (NIST's
  archives), and the genesis's O5 departure C17 (C16, a throughput workflow, falls away with
  D32).
