---
title: "webcpp::crypto, the foundation (version 0.2 onward): Implementation Plan"
tags: [webcpp, crypto, libsodium, secp256k1, bearssl, openssl, botan, blst, webcrypto, plan]
status: accepted
created: 2026-11-14
revised: 2026-11-17
---

# webcpp::crypto, the foundation: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal of increment 1 (version 0.2 with increment 2):** libsodium as webcpp::crypto's backend
`backends::sodium` (Ed25519, Ed25519ph, X25519, the Ed25519 to X25519 conversion, the three
ChaCha20-Poly1305 AEADs, `hybrid::box`, sealed boxes, `ciphers::secretbox` in both forms, `kx`),
ChaCha20-Poly1305 on BearSSL and OpenSSL (the genesis's increment 4), the capability table and
its generated refusals, one default per family by a stated reason, the threads detection of the
heavy variants, the constant-time lane over version 0.1's backends and the new code, sizes and
throughput from release builds, and the documentation's new form (the revision of 2026-11-17
below). The design is `../design.md` (D1 to D35); the requirements are
`../specs/crypto/spec.md`; the order is `roadmap.md`.

**Architecture:** every family is a type that does its guards once and calls
`detail::<family>_engine<Algorithm, Backend>`, which a backend's header specialises (the
genesis's D3). `meta/capabilities.json` is the single source of what exists where;
`tools/crypto/capabilities.py` generates the refusals, `supports_v`, the documentation's table
and a test. `meta/measurements.json` holds webcpp's own sizes and throughput;
`tools/crypto/measure.py` generates `detail/rankings.hpp`, which `default_backend_t<Family>`
walks for the profile the program's `#ifdef` macros resolve to (D5).

**Tech stack:** C++20, b2 5.5.3, Boost 1.92 (System, Hash2 through webcpp::hash, Mp11, Core's
lightweight_test, JSON in tests), wasi-sdk 34 with wasmtime 50.0.1, Emscripten 6.0.11 with node
26.7.0, Python 3.9+. libsodium 1.0.22 at `77e1ce5d6dee871c49ef211222ba18ef0c486bda`, as
webcpp::hash's `/third_party//sodium` builds it. Test data: Wycheproof at `12fd3aaf`, the RFCs'
vectors written into the tests, libsodium's own `test/default`.

## The revision of 2026-11-17: one order, the genesis's increments 3 to 5, and defaults by reason

This section rules over everything below it. The owner decided on 2026-11-17 that the
genesis's increments 3 to 5 are built without trystero's migration and beside this plan's
twelve increments in one order, that measured tables come only from release builds, and that
crypto's defaults follow webcpp::hash's rule of 2026-11-16 (hash's design D33). `../design.md`
D32 to D35 record the decisions and `roadmap.md` (this directory) the order, the waves, the
streams and the owner's answers Q1 to Q7. The tasks below keep their text; where this section
re-cuts one, its heading carries a note, and a task named with a prime (Task 4', 5', 11') is
the re-cut one.

**What the tree holds now.** Version 0.1 is merged (`webcpporg/crypto` `main` at `d242be8`),
with `ciphers::` and `curves::secp256k1::` in place, the generator `doc/tables/generate.py` and
its data `doc/tables/crypto.json` (the operations, the tests that back each backend, the
defaults with their reasons, the use cases, the fixtures and the fixes of every message), and
the default tests `default_backend`, `default_bearssl`, `default_both`, `default_both_wasm`,
`default_openssl_wasm`, `default_selected` and `default_selection`. The superproject's `main`
holds `/third_party//sodium` (libsodium 1.0.22) and `webcpp.size-table`, which requires
`<variant>release`. Task 1's check of the base passes on that tree.

**Task 4' (replaces Task 4's files; its interfaces and tests stand).** The table is
`libs/crypto/doc/tables/crypto.json`, grown, and the generator `libs/crypto/doc/tables/generate.py`
with its test `generate_test.py`, both version 0.1's. `crypto.json` gains a `families` list,
each with `name`, `title`, `group`, `since`, `default` (per target, with `why` and its cited
sources, D32) and per backend its `targets`, `runtime`, `constant_time` per target and the
tests that back it; the existing `operations`, `defaults` and `use_cases` move under it or
point to it, and a cell is claimed only where a named test runs on its targets (version 0.1's
rule). `generate.py` gains the outputs Task 4 gives `capabilities.py`:
`include/webcpp/crypto/detail/capabilities.hpp` (the refusal text per absent pair, the same
text as Task 4's Step 2), `supports.hpp`'s table, `include/webcpp/crypto/detail/defaults.hpp`
(Task 5'), and `test/capabilities_test.cpp`, each opening with version 0.1's generated-file
notice; `--check` covers them, and the documentation's build runs it as today. Not created:
`meta/capabilities.json`, `tools/crypto/capabilities.py`, `tools/test/crypto_capabilities_test.py`,
`doc/capabilities.adoc` (the table is one of `doc/generated/`), and no `portable` field.

**Task 5' (replaces Task 5).** One default per family and target, by a stated reason (D32).

- Files: generated `include/webcpp/crypto/detail/defaults.hpp`; modify `config.hpp`,
  `ciphers/aes256_gcm.hpp` and every family header that names a default, `test/Jamfile`,
  version 0.1's default tests, `test/messages/` (below), `doc/tables/crypto.json`.
- Interface: `template <class Family> struct default_backend_of;` with `using type`, and
  `template <class Family> using default_backend_t = typename default_backend_of<Family>::type;`
  in `detail/defaults.hpp`, one specialisation per family that has a default, chosen by
  `WEBCPP_TARGET_*` alone; `default_backend` stays `default_backend_t<families::gcm>`. A
  backend's tag is declared without its library (as version 0.1's `backends::*` tags are), and
  a family's primary engine template refuses, where the family is called, with the default's
  "needs" text: the Jamfile line that links it and the type that names another backend, as
  hash's `detail/engines.hpp` does for `argon2id`.
- Version 0.1 changes: `config.hpp` no longer reads the `HAS` defines to choose AES-GCM's
  default; natively it is OpenSSL whatever is linked (the owner's answer Q2). `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and `_OPENSSL`
  stop compilation with `webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME> no longer chooses
  a default. Write the backend in the type where the program wants it, as in
  ciphers::gcm<ciphers::aes256<backends::<name>>>.` (the owner's answer Q3). Version 0.1's
  `default_openssl_wasm` and `default_selected` become message tests of that text;
  `default_bearssl` natively becomes the message test `gcm_default_openssl_not_linked`;
  `default_both`, `default_both_wasm` and `default_selection` assert the fixed defaults.
- Tests, each first and seen failing: `default_backend_test` asserts every family's
  `default_backend_t` per target, built with every backend linked and with each linked alone;
  `default_backend_former_defines` builds it with `WEBCPP_CRYPTO_PROFILE_FASTEST`,
  `WEBCPP_PROFILE_SMALLEST` and `WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_BEARSSL` and asserts that
  nothing changed; one message test per family whose default may be unlinked
  (`ed25519_default_not_linked`, `chacha_default_not_linked`, `gcm_default_openssl_not_linked`),
  with its Emscripten fixture and MSVC case as version 0.1's messages have.
- Not created: `detail/select.hpp`, `detail/rankings.hpp`, `detail/profile_checks.hpp`,
  `profiles`, `profile`, `profile_name`, `none_linked`, the profile messages and their
  `detect_mismatch` keys.
- Commit `crypto: one default per family and target, by a stated reason (design D32)`.

**Task 8b (new, after Task 8): ChaCha20-Poly1305 on BearSSL and OpenSSL** (the genesis's
increment 4, its Task 35 re-cut onto `ciphers::aead`).

- Files: modify `backends/bearssl.hpp` and `backends/openssl.hpp` (the
  `aead_state<families::chacha20_poly1305, B>` and `aead_engine<...>` specialisations, the
  engine contract of Task 8), `backends/ctaes.hpp` (nothing: the generated refusal covers it),
  `test/Jamfile`, `test/support/agreement.hpp` (three-way), `test/allocation_bearssl_test.cpp`,
  `test/allocation_openssl_test.cpp`, `test/openssl_error_queue_test.cpp`,
  `doc/tables/crypto.json`; create `test/chacha_bearssl_test.cpp`,
  `test/chacha_openssl_test.cpp`, `test/chacha_wycheproof_bearssl_test.cpp`,
  `test/chacha_wycheproof_openssl_test.cpp`, `test/agreement_chacha_test.cpp`,
  `test/messages/chacha_on_ctaes/` and its MSVC fixture.
- The engines are the genesis plan's Task 35 Step 3 (BearSSL's `br_chacha20_ct_run` and
  `br_poly1305_ctmul_run` with the tag compared by `equal`; OpenSSL's `EVP_chacha20_poly1305`
  with the error queue left as found), written against `aead_state` and `aead_engine` in place
  of `chacha_state` and `chacha_engine`; the checks are Task 8's `aead_checks.hpp` with the
  genesis's RFC 8439 cases (section 2.8.2 and appendix A.5, M4) and Wycheproof's 325 cases per
  backend; the agreement runs libsodium, BearSSL and OpenSSL from one seed natively, and
  libsodium against BearSSL (and OpenSSL on WASI) on each WebAssembly target.
- Its constant-time cases join Task 14, its size programs Task 11', its part of the memory
  option's `everything_test.cpp` whichever of stream A and stream M merges second, and its
  documentation Task 15 (the genesis's Task 35 Step 6 text, in D19's form).
- Commit `crypto: ChaCha20-Poly1305 on BearSSL and OpenSSL over ciphers::aead, against RFC
  8439, Wycheproof and libsodium`.

**Task 11' (replaces Task 11): sizes from release builds, throughput by hand, nothing ranked.**

- The own lane `sizes`: `libs/crypto/test/sizes/Jamfile` declares the baseline and one program
  per (family, backend) cell, each calling the family's fixed operation once, through the
  superproject's `webcpp.size-table` (which requires `<variant>release`) on native, wasip2,
  wasip3 and Emscripten; `tools/ci/matrix.py` gains the own lane; `generate.py` reads the lane's
  table into the size matrix, writing "not measured yet" for a cell the lane has not built.
  Version 0.1's operations (AES-GCM on BearSSL and OpenSSL, the block cipher on OpenSSL and
  ctaes, secp256k1's signing, with Task 29's signing program) come first; each later task adds
  its cells.
- The throughput page: `libs/crypto/bench/throughput.cpp` (each family's fixed operation per
  backend, five runs, the median) and `bench/collect.py` after webcpp::hash's, refusing a
  program or data not built at `variant=release`, with its test in the superproject beside
  `tools/test/hash_collect_test.py`; measured by hand on the machines the page names (the
  developer's, and the CI's runners when the owner runs them), written to `doc/performance.adoc`
  as a dated reference. libsodium's X25519 and Ed25519 with `HAVE_AMD64_ASM` in a scratch build
  are recorded there too (D7).
- Not created: `meta/measurements.json`, `tools/crypto/measure.py`, its test,
  `detail/rankings.hpp`, `runners.yml`, the `heavy` column (D32 states the heavy list).
- Commit (superproject) `tools: crypto's size lane and its collect script, from release builds`;
  (library) `crypto: sizes and throughput from release builds, as references`.

**Task 14, whole.** Its Step 1 builds the genesis's Task 20 entire first: the harnesses, the
planted leak, and version 0.1's paths (BearSSL's AES-GCM seal and open, OpenSSL's where it is
built with assembly, with its open left out as the genesis says, ctaes's block, libsecp256k1's
own `ctime_tests.c` from Task 26), then Steps 2 and 3 over libsodium's paths and the ChaCha
engines of Task 8b. `runners.yml` is not built; the arm64 run is by hand.

**Task 15.** `generate.py` writes what `docs.py` was to write; no `doc/profiles.adoc` (the
chapter is `doc/choosing_backend.adoc`, each family's default with its reason, as hash's); the
use cases' field is `backend`, never `profile`; the matrices read the size lane's table and
`doc/performance.adoc`'s data.

**Task 16.** Green adds `generate.py --check` in place of `capabilities.py --check`, and the
review checks each family's fixed default on every lane in place of the profile's; the
genesis's Task 36 review items (spec 2.5, 8.1's ChaCha part, 8.2) join it.

**The outlines below:** increment 3 has no ChaCha20-Poly1305 (Task 8b); increment 4 carries the
genesis's Task 23 (the audio worklet's `not_supported`, Chrome's throughput page from release
builds); increment 7 opens, after its spike and its submodule, with the genesis's Task 38 cells
and ends with its Task 39 agreement and constant-time cases; increment 8 has no
ChaCha20-Poly1305 and moves the AES modes' native default to OpenSSL (D32). Every outlined
increment's "measured", "ranked" or "heavy column" wording is read through D32.

## The base and the order of the work

1. **The base** is the superproject's `origin/main` once (a) crypto's version 0.1 has merged
   (genesis Tasks 25 and 30) and (b) webcpp::hash's Task 8 (`/third_party//sodium`) and its
   Tasks 1 to 3 (`/webcpp/hash//hash`) have merged. Task 1 checks both and stops if either is
   missing.
   Both have merged (2026-11-17); `roadmap.md` gives the order of every stream.
2. **Increment 2 runs beside increment 1** on its own branch `crypto/secp256k1-modules`; its
   capability rows join after Task 4 merges, by a rebase.
3. **After increment 1 merges, increments 3 to 8 are six parallel streams**, each on its own
   branch (`crypto/bearssl-rest`, `crypto/browser`, `crypto/sodium-rest`, `crypto/blst`,
   `crypto/botan`, `crypto/openssl-rest`). The generated files (`meta/capabilities.json`'s
   derived files, `meta/measurements.json`'s) are regenerated after each rebase, never merged by
   hand. Merges follow the value order.
4. **Pull requests:** the library's into `webcpporg/crypto`'s `main`; the superproject's
   (`third_party/Jamfile`, `tools/crypto/`, the rulebook) beside it. The owner merges both.

## Global Constraints

The genesis's Global Constraints (`../../genesis/reference/plan.md`) hold, with these
additions:

- Every b2 command runs from the worktree's root as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`
  (`$B2`). The lanes are the genesis's: native, wasip2, wasip3, emscripten, gcc-15; MSVC and
  Apple clang in the CI.
- libsodium's engines call `detail::sodium_ready()` first, which calls `sodium_init()` and
  maps -1 to `backend_failure`.
- No file under `include/webcpp/crypto/detail/capabilities.hpp`, `detail/rankings.hpp`,
  `supports.hpp`'s generated block, `doc/capabilities.adoc`, `doc/measurements.adoc` or
  `test/capabilities_test.cpp` is edited by hand; each opens with
  `// Generated by tools/crypto/<tool>.py from meta/<file>.json. Do not edit.` after the
  licence notice.
- Secret outputs are `secret<N>`; seeds are drawn from `random_device`; libsodium's
  `randombytes_*` is reached only where libsodium draws itself.
- Commits use the contributor's own identity, no trailer; subjects `crypto: ...` in the
  library, `third_party: ...`, `tools/crypto: ...`, `guidelines: ...` in the superproject.
- English, no em dash, never the banned word, the licence notice on every source file.

**Names fixed by this plan:**

| Name | What it is |
| --- | --- |
| `/third_party//sodium` | webcpp::hash's target; this plan adds `libs/crypto/src/canary_sodium.c` and `WEBCPP_CRYPTO_HAS_SODIUM=1` to its usage requirements |
| `backends::sodium` | the libsodium backend, `backends/sodium.hpp` |
| `families::<name>` | the family tags of the capability table (`ed25519`, `x25519`, `ciphers::chacha20_poly1305`, `gcm`, ...) |
| `default_backend_t<Family>` | the default backend of a family (`detail/select.hpp`) |
| `WEBCPP_CRYPTO_PROFILE_{FASTEST,SMALLEST,PORTABLE}`, `WEBCPP_CRYPTO_PROFILE_BACKEND_<NAME>` | crypto's profile, at most one |
| `WEBCPP_PROFILE_{FASTEST,SMALLEST,PORTABLE}` | the webcpp-wide umbrella, read by crypto and hash, at most one |
| `WEBCPP_CRYPTO_DEFAULT_BACKEND_<FAMILY>_<NAME>` | a per-family override, at most one per family |
| `profiles::{automatic,fastest,smallest,portable,<backend>}`, `profile_name` | the resolved profile |
| `supports_v<Algorithm, Backend>` | the capability table on the current target (`supports.hpp`) |
| `secret<N>` | a self-wiping array (`secret.hpp`) |
| `ciphers::aead<Family, Backend>` | the one-shot AEAD template (`ciphers/aead.hpp`) |
| groups `ciphers`, `curves::<curve>`, `post_quantum`, `rsa`, `hybrid`, `commitments::kzg`, `secret_sharing::shamir` | the namespaces, headers `<webcpp/crypto/<group>/<name>.hpp>` (design D29); the root keeps `error`, `random_device`, `equal`, the memory utilities, `secret<N>`, the profiles; `backends` stays outside |
| `WEBCPP_CRYPTO_HAS_THREADS`, `WEBCPP_CRYPTO_NO_THREADS` | threads detection (D23) |
| `tools/crypto/capabilities.py`, `tools/crypto/measure.py` | the generators (superproject) |
| own lanes `sizes`, `throughput`, `constant-time`, `c-suites` | as the genesis named them; `throughput` runs in `runners.yml` |

---

# Increment 1: libsodium's core, profiles and the capability table

## Task 1: The worktree, the ledger and the baseline

**Files:** create `.superpowers/sdd/WEBCPP_CRYPTO_FOUNDATION/progress.md` (untracked).

- [ ] **Step 1: Make the worktree.**

```bash
git -C <superproject> fetch origin
git -C <superproject> worktree add <worktree> -b crypto/foundation-1 origin/main
cd <worktree> && git submodule update --init
ls libs/crypto/include/webcpp/crypto/curves/secp256k1.hpp libs/hash/include/webcpp/hash.hpp third_party/libs/libsodium/src/libsodium/include/sodium.h
grep -n 'lib sodium\|alias sodium' third_party/Jamfile
ln -s <superproject>/.local .local
```

  Expected: the three files exist, and `third_party/Jamfile` declares `sodium`. A missing one
  stops the task: the base is not ready (Base 1).
- [ ] **Step 2: The ledger** opens with
  `# SDD ledger - plan: libraries/crypto/openspec/changes/foundation/reference/plan.md (proposals)`
  and records `git rev-parse HEAD`, `git -C third_party/libs/libsodium rev-parse HEAD`
  (expected `77e1ce5d6dee871c49ef211222ba18ef0c486bda`), and the four lanes of
  `libs/crypto/test` and `libs/hash/test`, all passing. A failure found here is fixed first,
  in its own pull request.

## Task 2: crypto's canary on `/third_party//sodium`, and `backends/sodium.hpp`

**Files:**
- Create: `libs/crypto/src/canary_sodium.c`, `include/webcpp/crypto/backends/sodium.hpp`,
  `test/sodium_backend_test.cpp`, `test/messages/sodium_not_linked/`,
  `test/messages/sodium_canary_commit/`.
- Modify: `include/webcpp/crypto/detail/canary.h`, `include/webcpp/crypto/config.hpp` (the tag
  `backends::sodium`), `test/Jamfile`, superproject `third_party/Jamfile` (the canary source
  and the define on `sodium`), `tools/test/fixtures/crypto_jamfiles/` (the Emscripten case).

- [ ] **Step 1: The failing test** `test/sodium_backend_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/crypto/backends/sodium.hpp>

#include <boost/core/lightweight_test.hpp>
#include <webcpp/test/exit_status.hpp>

namespace crypto = webcpp::crypto;

int main() {
    BOOST_TEST_EQ(crypto::backends::sodium::name, "sodium");
    BOOST_TEST(crypto::detail::sodium_ready().has_value());
    // A second call is libsodium's no-op (sodium_init returns 1), still a success.
    BOOST_TEST(crypto::detail::sodium_ready().has_value());
    return webcpp::test::exit_status(boost::report_errors());
}
```

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//sodium_backend_test`: expected a
  compile error, no `backends/sodium.hpp`.
- [ ] **Step 2: The canary.** In `detail/canary.h`, beside BearSSL's:

```c
#ifdef WEBCPP_CRYPTO_SODIUM_CANARY_COMMIT
#define WEBCPP_CRYPTO_SODIUM_COMMIT WEBCPP_CRYPTO_SODIUM_CANARY_COMMIT
#else
#define WEBCPP_CRYPTO_SODIUM_COMMIT 77e1ce5
#endif
```

  `src/canary_sodium.c`:

```c
/* Copyright (c) 2026 WebCpp.org
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * https://www.boost.org/LICENSE_1_0.txt) */

/* The canary of libsodium (CODING-GUIDELINES.md chapter 5, rule 6), compiled into
 * /third_party//sodium with libsodium's own flags. The configuration is "portable": no
 * assembly on any target (design D7). */
#include <webcpp/crypto/detail/canary.h>

const char WEBCPP_CRYPTO_DETAIL_CANARY(sodium, WEBCPP_CRYPTO_SODIUM_COMMIT, portable) = 0;

#ifdef _MSC_VER
#pragma detect_mismatch("webcpp_crypto_backend_sodium", \
                        WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(sodium, WEBCPP_CRYPTO_SODIUM_COMMIT, portable))
#endif
```

  In `third_party/Jamfile`, the `sodium` target gains the source
  `../libs/crypto/src/canary_sodium.c` (with `<include>../libs/crypto/include`) and the usage
  requirement `<define>WEBCPP_CRYPTO_HAS_SODIUM=1`, beside webcpp::hash's canary and define.
- [ ] **Step 3: `backends/sodium.hpp`**, the three layers and the initialisation:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The backend libsodium: libsodium 1.0.22's own C code, built from its sources by
 `/third_party//sodium`, the one copy webcpp::hash and webcpp::crypto share.

 @see "The backends", in the guide.
*/
#ifndef WEBCPP_CRYPTO_BACKENDS_SODIUM_HPP
#define WEBCPP_CRYPTO_BACKENDS_SODIUM_HPP

#include <webcpp/crypto/config.hpp>

#ifndef WEBCPP_CRYPTO_HAS_SODIUM
// clang-format off
#error "webcpp::crypto: the backend libsodium is used and not linked. In the program's Jamfile, add /third_party//sodium beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//sodium ; (without b2: define WEBCPP_CRYPTO_HAS_SODIUM=1 and link libsodium built with webcpp's src/canary_sodium.c)"
// clang-format on
#else

#include <webcpp/crypto/detail/canary.h>
#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/system/result.hpp>
#include <sodium.h>

#include <string_view>

#define WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY \
    WEBCPP_CRYPTO_DETAIL_CANARY(sodium, WEBCPP_CRYPTO_SODIUM_COMMIT, portable)

#ifdef _MSC_VER
#pragma detect_mismatch("webcpp_crypto_backend_sodium", \
                        WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(sodium, WEBCPP_CRYPTO_SODIUM_COMMIT, portable))
#endif

extern "C" const char WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY;

namespace webcpp::crypto::backends {

/** The backend libsodium (design D7). */
struct sodium {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "sodium";
};

}  // namespace webcpp::crypto::backends

namespace webcpp::crypto::detail {

/**
 Reads libsodium's canary and initialises libsodium, which every engine calls first.
 `sodium_init` is idempotent and takes libsodium's own lock; its state is libsodium's.

 @return Nothing; `backend_failure` when `sodium_init` returns -1.
*/
inline boost::system::result<void> sodium_ready() noexcept {
    read_canary(WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY);
    if (sodium_init() < 0) {
        return error::backend_failure;
    }
    return {};
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_HAS_SODIUM
#endif  // WEBCPP_CRYPTO_BACKENDS_SODIUM_HPP
```

- [ ] **Step 4: The messages.** `test/messages/sodium_not_linked/` (a
  `webcpp.compile-diagnostic` that includes the header without the define, its source stating
  the whole `#error` text) and `test/messages/sodium_canary_commit/` (a link test with
  `<define>WEBCPP_CRYPTO_SODIUM_CANARY_COMMIT=0000000`, expecting the canary's name), each in a
  directory of its own; on Emscripten, the fixture `sodium_not_linked_emscripten`.
- [ ] **Step 5: Run** the native, wasip2, wasip3 and emscripten lanes on `libs/crypto/test`;
  expected all passed, the two message tests included. Commit (library):
  `crypto: the backend libsodium, its canary and its three layers`; (superproject)
  `third_party: crypto's canary on /third_party//sodium`.

## Task 3: `secret<N>` and the errors 15 to 20

**Files:**
- Create: `include/webcpp/crypto/secret.hpp`, `test/secret_test.cpp`.
- Modify: `include/webcpp/crypto/error.hpp`, `test/error_test.cpp`, `test/Jamfile`.

- [ ] **Step 1: The failing tests.** `test/secret_test.cpp` checks that `secret<32>` is not
  copyable (`static_assert(!std::is_copy_constructible_v<secret<32>>)`), that `clone()` copies,
  that a move leaves the source zero, that `std::span<std::uint8_t const, 32>` converts, and
  that the storage is zero after destruction (placement-new into an `alignas` buffer, destroy,
  inspect, as the genesis's wipe test does). `test/error_test.cpp` gains the values 15 to 20
  and their messages.
- [ ] **Step 2: `secret.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/** A secret output of webcpp::crypto, which wipes itself (design D9). */
#ifndef WEBCPP_CRYPTO_SECRET_HPP
#define WEBCPP_CRYPTO_SECRET_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/wipe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace webcpp::crypto {

/**
 `N` secret bytes: a shared secret, a session key, a converted secret key. They are wiped by
 volatile stores when the object is destroyed or moved from; a copy is made only by `clone`.

 @tparam N The number of bytes.
*/
template <std::size_t N>
class secret {
public:
    /** The number of bytes. */
    static constexpr std::size_t size_bytes = N;

    /** Zero bytes. */
    secret() noexcept = default;

    /** Takes the bytes of `other`, which is wiped. */
    secret(secret&& other) noexcept : bytes_(other.bytes_) { detail::wipe_object(other.bytes_); }

    /** Takes the bytes of `other` over these, which are wiped first; `other` is wiped. */
    secret& operator=(secret&& other) noexcept {
        if (this != &other) {
            bytes_ = other.bytes_;
            detail::wipe_object(other.bytes_);
        }
        return *this;
    }

    /** Not copyable: a copy is asked for by name, with `clone`. */
    secret(secret const&) = delete;
    /** Not copyable: a copy is asked for by name, with `clone`. */
    secret& operator=(secret const&) = delete;

    /** Wipes the bytes. */
    ~secret() { detail::wipe_object(bytes_); }

    /** A copy of the bytes, which the caller then owns. */
    [[nodiscard]] secret clone() const noexcept {
        secret copy;
        copy.bytes_ = bytes_;
        return copy;
    }

    /** The bytes, to read. */
    [[nodiscard]] std::span<std::uint8_t const, N> bytes() const noexcept { return bytes_; }

    /** The bytes, for the library's engines to write. */
    [[nodiscard]] std::span<std::uint8_t, N> writable() noexcept { return bytes_; }

    /** The bytes, as a span of fixed size. */
    operator std::span<std::uint8_t const, N>() const noexcept { return bytes_; }

    /** The number of bytes. */
    static constexpr std::size_t size() noexcept { return N; }

private:
    std::array<std::uint8_t, N> bytes_{};
};

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_SECRET_HPP
```

- [ ] **Step 3: The errors.** In `error.hpp`, after the comment on 13 and 14:

```cpp
    /** A public key of small order, or one that gives an all-zero shared secret. */
    weak_public_key = 15,
    /** An encoding that is not a canonical element of the group. */
    invalid_point = 16,
    /** A scalar that is not reduced, or zero where zero is refused. */
    invalid_scalar = 17,
    /** Padding that cannot be removed, whatever the cause. */
    invalid_padding = 18,
    /** Hexadecimal or base64 that does not decode. */
    invalid_encoding = 19,
    /** A KEM or RSA ciphertext of another size than the key's. */
    invalid_ciphertext_size = 20,
```

  and in `message_of`: "the public key is weak: of small order, or the shared secret is zero",
  "the encoding is not a canonical element of the group", "the scalar is not reduced, or is
  zero", "the padding is not valid", "the encoding does not decode", "the ciphertext is not of
  the key's size".
- [ ] **Step 4: Run** the lanes; commit `crypto: secret<N>, and the errors 15 to 20`.

## Task 4: The capability table, its generator, `supports_v` and the refusals

Re-cut by the revision of 2026-11-17 as Task 4': the table and generator are version 0.1's `doc/tables/crypto.json` and `generate.py`.

**Files:**
- Create: `libs/crypto/meta/capabilities.json`, superproject `tools/crypto/capabilities.py`,
  `tools/test/crypto_capabilities_test.py`, generated
  `include/webcpp/crypto/detail/capabilities.hpp`, `include/webcpp/crypto/supports.hpp`,
  `doc/capabilities.adoc`, `test/capabilities_test.cpp`.
- Modify: `test/Jamfile`, `include/webcpp/crypto/detail/engines.hpp` (`dependent_false`).

- [ ] **Step 1: The table's form**, with the genesis's rows and increment 1's:

```json
{
  "targets": ["native", "wasip2", "wasip3", "emscripten"],
  "backends": {
    "bearssl":   {"jam": "/third_party//bearssl",   "targets": ["native", "wasip2", "wasip3", "emscripten"]},
    "openssl":   {"jam": "/webcpp/crypto//openssl", "targets": ["native", "wasip2", "wasip3"]},
    "ctaes":     {"jam": "/third_party//ctaes",     "targets": ["native", "wasip2", "wasip3", "emscripten"]},
    "secp256k1": {"jam": "/third_party//secp256k1", "targets": ["native", "wasip2", "wasip3", "emscripten"]},
    "sodium":    {"jam": "/third_party//sodium",    "targets": ["native", "wasip2", "wasip3", "emscripten"]}
  },
  "algorithms": [
    {"family": "gcm", "name": "aes256_gcm", "title": "AES-256-GCM", "since": "0.1",
     "backends": {"bearssl": {"targets": "all"}, "openssl": {"targets": "all"},
                  "sodium": {"targets": ["native"], "runtime": true, "since": "foundation-5"}},
     "portable": "bearssl"},
    {"family": "ed25519", "name": "ed25519", "title": "Ed25519", "since": "foundation-1",
     "backends": {"sodium": {"targets": "all"}}, "portable": "sodium"},
    {"family": "x25519", "name": "x25519", "title": "X25519", "since": "foundation-1",
     "backends": {"sodium": {"targets": "all"}}, "portable": "sodium"},
    {"family": "chacha20_poly1305", "name": "chacha20_poly1305", "title": "ChaCha20-Poly1305",
     "since": "foundation-1", "backends": {"sodium": {"targets": "all"}}, "portable": "sodium"}
  ]
}
```

  `"all"` means every target of the backend. Every algorithm row also takes `"constant_time"`
  per backend and target where it is not everywhere (OpenSSL's `no-asm` AES on MSVC and
  WebAssembly), and `"heavy"` is never written by hand: `measure.py` (Task 11) writes it.
- [ ] **Step 2: The failing tools test** `tools/test/crypto_capabilities_test.py`: a table with
  one algorithm and two backends generates a header whose refusal text is
  `webcpp::crypto: bearssl has no Ed25519. The backends that have it: sodium (native, wasip2, wasip3, emscripten).`;
  `--check` passes on fresh output and fails, naming the file, after one byte changes; an
  unknown backend in a row and a `portable` backend that lacks a target are refused with their
  row named.
- [ ] **Step 3: `tools/crypto/capabilities.py`:**

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Generates webcpp::crypto's refusals, supports_v, the documentation's table and the table's
test from libs/crypto/meta/capabilities.json (design D4). --check compares instead of writing."""

import argparse
import json
import sys
from pathlib import Path

NOTICE = """// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Generated by tools/crypto/capabilities.py from meta/capabilities.json. Do not edit.
"""
MACRO_OF_TARGET = {'native': 'WEBCPP_CRYPTO_DETAIL_TARGET_NATIVE', 'wasip2': 'WEBCPP_TARGET_WASIP2',
                   'wasip3': 'WEBCPP_TARGET_WASIP3', 'emscripten': 'WEBCPP_TARGET_EMSCRIPTEN'}


def targets_of(table, backend, cell):
    wanted = cell['targets']
    every = table['backends'][backend]['targets']
    return list(every) if wanted == 'all' else [t for t in every if t in wanted]


def validate(table):
    faults = []
    for row in table['algorithms']:
        for backend, cell in row['backends'].items():
            if backend not in table['backends']:
                faults.append(f"{row['name']}: unknown backend {backend}")
                continue
            if cell['targets'] != 'all' and not set(cell['targets']) <= set(table['backends'][backend]['targets']):
                faults.append(f"{row['name']}: {backend} is not built for {cell['targets']}")
        portable = row.get('portable')
        if portable and set(targets_of(table, portable, row['backends'][portable])) != set(table['targets']):
            faults.append(f"{row['name']}: the portable backend {portable} lacks a target")
    return faults


def have_list(table, row):
    parts = []
    for backend, cell in row['backends'].items():
        where = ', '.join(targets_of(table, backend, cell))
        extra = ', at run time' if cell.get('runtime') else ''
        parts.append(f'{backend} ({where}{extra})')
    return ', '.join(parts)


def header(table):
    lines = [NOTICE, '#ifndef WEBCPP_CRYPTO_DETAIL_CAPABILITIES_HPP',
             '#define WEBCPP_CRYPTO_DETAIL_CAPABILITIES_HPP', '',
             '#include <webcpp/crypto/config.hpp>', '', '#include <webcpp/crypto/detail/engines.hpp>', '']
    for row in table['algorithms']:
        name = row['name'].upper()
        lines.append(f'#define WEBCPP_CRYPTO_DETAIL_HAVE_{name} "{have_list(table, row)}"')
        lines.append(f'#define WEBCPP_CRYPTO_DETAIL_TITLE_{name} "{row["title"]}"')
    lines += ['', '// A backend without the algorithm: the primary engine of each family refuses with',
              '//   static_assert(dependent_false<Backend>, WEBCPP_CRYPTO_DETAIL_REFUSAL(<backend>, <NAME>));',
              '#define WEBCPP_CRYPTO_DETAIL_REFUSAL(backend, NAME) \\',
              '    "webcpp::crypto: " backend " has no " WEBCPP_CRYPTO_DETAIL_TITLE_##NAME \\',
              '    ". The backends that have it: " WEBCPP_CRYPTO_DETAIL_HAVE_##NAME "."', '',
              '#endif  // WEBCPP_CRYPTO_DETAIL_CAPABILITIES_HPP', '']
    return '\n'.join(lines)


def supports(table):
    lines = [NOTICE, '// The block of supports.hpp that the table gives: one specialisation per cell.']
    for row in table['algorithms']:
        for backend, cell in row['backends'].items():
            condition = ' || '.join(f'defined({MACRO_OF_TARGET[t]})' for t in targets_of(table, backend, cell))
            lines += [f'#if {condition}',
                      f'template <> inline constexpr bool supported<families::{row["family"]}, '
                      f'tags::{row["name"]}, backends::{backend}> = true;', '#endif']
    return '\n'.join(lines) + '\n'


def document(table):
    columns = list(table['backends'])
    lines = ['// Copyright (c) 2026 WebCpp.org', '//', '// Distributed under the Boost Software License, Version 1.0. (See',
             '// accompanying file LICENSE_1_0.txt or copy at', '// https://www.boost.org/LICENSE_1_0.txt)', '',
             '// Generated by tools/crypto/capabilities.py from meta/capabilities.json. Do not edit.', '',
             '= What each backend has', '', f'[cols="2,{",".join("1" * len(columns))}",options="header"]', '|===',
             '| Algorithm | ' + ' | '.join(columns)]
    for row in table['algorithms']:
        cells = []
        for backend in columns:
            cell = row['backends'].get(backend)
            cells.append(', '.join(targets_of(table, backend, cell)) if cell else '-')
        lines.append(f'| {row["title"]} | ' + ' | '.join(cells))
    return '\n'.join(lines + ['|===', ''])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--library', default='libs/crypto')
    parser.add_argument('--check', action='store_true')
    arguments = parser.parse_args()
    library = Path(arguments.library)
    table = json.loads((library / 'meta/capabilities.json').read_text())
    faults = validate(table)
    if faults:
        print('\n'.join(f'capabilities.json: {fault}' for fault in faults))
        return 1
    outputs = {library / 'include/webcpp/crypto/detail/capabilities.hpp': header(table),
               library / 'include/webcpp/crypto/detail/supports_table.hpp': supports(table),
               library / 'doc/capabilities.adoc': document(table)}
    stale = [path for path, text in outputs.items() if not path.exists() or path.read_text() != text]
    if arguments.check:
        for path in stale:
            print(f'{path}: differs from what meta/capabilities.json gives; run tools/crypto/capabilities.py')
        return 1 if stale else 0
    for path in stale:
        path.write_text(outputs[path])
    return 0


if __name__ == '__main__':
    sys.exit(main())
```

  (The generated test `test/capabilities_test.cpp` is written by the same tool in Step 5 of
  this task, once `supports.hpp` exists; the code above shows the three outputs whose form is
  fixed.)
- [ ] **Step 4: `supports.hpp`**, hand-written around the generated block:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/** Whether a backend has an algorithm on the target being compiled (design D4). */
#ifndef WEBCPP_CRYPTO_SUPPORTS_HPP
#define WEBCPP_CRYPTO_SUPPORTS_HPP

#include <webcpp/crypto/config.hpp>

namespace webcpp::crypto {

namespace detail {

/** One cell of the capability table: false unless the generated block says otherwise. */
template <class Family, class Name, class Backend>
inline constexpr bool supported = false;

#include <webcpp/crypto/detail/supports_table.hpp>

}  // namespace detail

/**
 `true` exactly where the capability table says `Backend` has `Algorithm` on this target, a
 cell decided at run time included (its `make` then reports `not_supported` on a machine that
 lacks it).

 @tparam Algorithm An algorithm type, which names its `family_type` and `name_type`.
 @tparam Backend A backend tag.
*/
template <class Algorithm, class Backend>
inline constexpr bool supports_v =
    detail::supported<typename Algorithm::family_type, typename Algorithm::name_type, Backend>;

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_SUPPORTS_HPP
```

  `config.hpp` defines `WEBCPP_CRYPTO_DETAIL_TARGET_NATIVE` when no `WEBCPP_TARGET_*` of
  WebAssembly is defined, and declares `namespace families` and `namespace tags` with an empty
  struct per row (generated into `detail/capabilities.hpp` too, so that a family exists once).
- [ ] **Step 5: The generated test** `test/capabilities_test.cpp`: for every cell,
  `static_assert(supports_v<A, B> == <table>)` under the target's condition, and, for every
  present cell whose backend is linked in the test's build, a call of the family's smallest
  operation so that a missing specialisation fails to compile; `test/Jamfile` builds it on
  every lane with every backend of the target linked.
- [ ] **Step 6: Run** `python3 tools/test/crypto_capabilities_test.py`,
  `python3 tools/crypto/capabilities.py --check`, the lanes; commit (superproject)
  `tools/crypto: the capability table's generator and its test`; (library)
  `crypto: the capability table, supports_v and the generated refusals`.

## Task 5: The profiles, the overrides and `default_backend_t`

Superseded by the revision of 2026-11-17, Task 5' (design D32): no profile, no override, no ranking. Kept as the record of what D32 replaces.

**Files:**
- Create: `include/webcpp/crypto/detail/select.hpp`, `include/webcpp/crypto/detail/rankings.hpp`
  (generated; seeded by hand in this task from the genesis's measurements, then regenerated by
  Task 11), `test/profile_test.cpp`, `test/messages/profile_crypto_twice/`, `test/messages/profile_webcpp_twice/`, `test/messages/override_ed25519_twice/`,
  `test/messages/profile_gcm_twice/`, `test/messages/none_linked_x25519/`.
- Modify: `include/webcpp/crypto/config.hpp`, `test/Jamfile`,
  `tools/test/fixtures/crypto_jamfiles/` (an MSVC `detect_mismatch` case per profile).

- [ ] **Step 1: The failing tests.** `test/profile_test.cpp` is built by `crypto-run` once
  with no define and once with each of `WEBCPP_CRYPTO_PROFILE_FASTEST`, `_SMALLEST`,
  `_PORTABLE`, `_BACKEND_SODIUM`, `WEBCPP_PROFILE_FASTEST`, `WEBCPP_PROFILE_SMALLEST` and
  `WEBCPP_PROFILE_PORTABLE`, with libsodium, BearSSL and (natively) OpenSSL linked, and asserts
  `default_backend_t<families::gcm>`, `default_backend_t<families::chacha20_poly1305>` and
  `profile_name` per build and target from the seeded rankings. Three more builds check the
  precedence: `WEBCPP_PROFILE_PORTABLE` with `WEBCPP_CRYPTO_PROFILE_FASTEST` (crypto's wins),
  then with `WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_BEARSSL` added (the override wins for `gcm`
  only), and `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` alone (the genesis's meaning).
- [ ] **Step 2: `config.hpp`**, after the genesis's refusals:

```cpp
// The profile (design D5): macros tested with #ifdef, at most one per level. The precedence is
// a family's override, then crypto's profile, then the webcpp-wide profile, then the build's
// measured default (automatic).
// Two profiles of one level: one #error per pair, each naming both macros. The pairs are
// generated by tools/crypto/capabilities.py into detail/profile_checks.hpp (crypto's seven
// profile macros, the umbrella's three); two of them:
#if defined(WEBCPP_CRYPTO_PROFILE_FASTEST) && defined(WEBCPP_CRYPTO_PROFILE_PORTABLE)
#error "webcpp::crypto: WEBCPP_CRYPTO_PROFILE_FASTEST and WEBCPP_CRYPTO_PROFILE_PORTABLE are both defined. Keep one."
#endif
#if defined(WEBCPP_PROFILE_FASTEST) && defined(WEBCPP_PROFILE_SMALLEST)
#error "webcpp: WEBCPP_PROFILE_FASTEST and WEBCPP_PROFILE_SMALLEST are both defined. Keep one."
#endif
#include <webcpp/crypto/detail/profile_checks.hpp>

// crypto's profile first, then the umbrella, then the measured default.
#if defined(WEBCPP_CRYPTO_PROFILE_FASTEST)
#define WEBCPP_CRYPTO_DETAIL_PROFILE fastest
#elif defined(WEBCPP_CRYPTO_PROFILE_SMALLEST)
#define WEBCPP_CRYPTO_DETAIL_PROFILE smallest
#elif defined(WEBCPP_CRYPTO_PROFILE_PORTABLE)
#define WEBCPP_CRYPTO_DETAIL_PROFILE portable
#elif defined(WEBCPP_CRYPTO_PROFILE_BACKEND_SODIUM)
#define WEBCPP_CRYPTO_DETAIL_PROFILE sodium
#elif defined(WEBCPP_CRYPTO_PROFILE_BACKEND_BEARSSL)
#define WEBCPP_CRYPTO_DETAIL_PROFILE bearssl
#elif defined(WEBCPP_CRYPTO_PROFILE_BACKEND_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_PROFILE openssl
#elif defined(WEBCPP_CRYPTO_PROFILE_BACKEND_CTAES)
#define WEBCPP_CRYPTO_DETAIL_PROFILE ctaes
#elif defined(WEBCPP_CRYPTO_PROFILE_BACKEND_SECP256K1)
#define WEBCPP_CRYPTO_DETAIL_PROFILE secp256k1
#endif  // BOTAN and BLST join with their increments
#ifndef WEBCPP_CRYPTO_DETAIL_PROFILE
#if defined(WEBCPP_PROFILE_FASTEST)
#define WEBCPP_CRYPTO_DETAIL_PROFILE fastest
#elif defined(WEBCPP_PROFILE_SMALLEST)
#define WEBCPP_CRYPTO_DETAIL_PROFILE smallest
#elif defined(WEBCPP_PROFILE_PORTABLE)
#define WEBCPP_CRYPTO_DETAIL_PROFILE portable
#else
#define WEBCPP_CRYPTO_DETAIL_PROFILE automatic
#endif
#endif

#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_BEARSSL) + defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_OPENSSL) + \
        defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL) + defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL) > 1
// clang-format off
#error "webcpp::crypto: two overrides of AES-GCM's backend are defined (WEBCPP_CRYPTO_DEFAULT_BACKEND_GCM_<NAME>, or the genesis's WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL or _OPENSSL). Keep one."
// clang-format on
#endif

// Threads (design D23): natively when Boost.Config sees threads (b2 threading=multi), on
// Emscripten with -pthread, never on WASI.
#if !defined(WEBCPP_CRYPTO_NO_THREADS) && !defined(WEBCPP_TARGET_WASIP2) && \
    !defined(WEBCPP_TARGET_WASIP3) &&                                       \
    ((defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(__EMSCRIPTEN_PTHREADS__)) || \
     (!defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(BOOST_HAS_THREADS)))
#define WEBCPP_CRYPTO_HAS_THREADS 1
#endif

namespace webcpp::crypto::profiles {

/** Natively `fastest`, on WebAssembly `smallest`: the owner's rule, and the default. */
struct automatic;
/** For each algorithm, the linked backend measured fastest on this target and architecture. */
struct fastest;
/** For each algorithm, the linked backend that adds the fewest bytes on this target. */
struct smallest;
/** For each algorithm, the capability table's backend that exists on every target. */
struct portable;
/** libsodium for every algorithm it has. */
using sodium = backends::sodium;
/** BearSSL for every algorithm it has. */
using bearssl = backends::bearssl;
/** OpenSSL for every algorithm it has. */
using openssl = backends::openssl;
/** ctaes for every algorithm it has. */
using ctaes = backends::ctaes;
/** libsecp256k1 for every algorithm it has. */
using secp256k1 = backends::secp256k1;

}  // namespace webcpp::crypto::profiles

namespace webcpp::crypto {

/** The profile the program's macros resolve to (design D5). */
using profile = profiles::WEBCPP_CRYPTO_DETAIL_PROFILE;

/** The resolved profile's name, for a program to print or assert. */
inline constexpr std::string_view profile_name = WEBCPP_CRYPTO_DETAIL_STRING(WEBCPP_CRYPTO_DETAIL_PROFILE);

}  // namespace webcpp::crypto

#ifdef _MSC_VER
#pragma detect_mismatch("webcpp_crypto_profile", WEBCPP_CRYPTO_DETAIL_STRING(WEBCPP_CRYPTO_DETAIL_PROFILE))
#endif
```

  (`WEBCPP_CRYPTO_DETAIL_STRING` moves from `detail/canary.h` into `config.hpp`, which
  `canary.h` then includes when compiled as C++.) The genesis's `default_backend` becomes
  `using default_backend = default_backend_t<families::gcm>;`, declared in `detail/select.hpp`,
  which every algorithm header includes.
- [ ] **Step 3: `detail/select.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_SELECT_HPP
#define WEBCPP_CRYPTO_DETAIL_SELECT_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/capabilities.hpp>
#include <webcpp/crypto/detail/rankings.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <type_traits>

namespace webcpp::crypto::detail {

/** Whether a backend's library is linked: its WEBCPP_CRYPTO_HAS_<NAME> define. */
template <class Backend>
inline constexpr bool linked = false;
#ifdef WEBCPP_CRYPTO_HAS_BEARSSL
template <> inline constexpr bool linked<backends::bearssl> = true;
#endif
#ifdef WEBCPP_CRYPTO_HAS_OPENSSL
template <> inline constexpr bool linked<backends::openssl> = true;
#endif
#ifdef WEBCPP_CRYPTO_HAS_CTAES
template <> inline constexpr bool linked<backends::ctaes> = true;
#endif
#ifdef WEBCPP_CRYPTO_HAS_SECP256K1
template <> inline constexpr bool linked<backends::secp256k1> = true;
#endif
#ifdef WEBCPP_CRYPTO_HAS_SODIUM
template <> inline constexpr bool linked<backends::sodium> = true;
#endif

/** The stand-in default of a family with no linked backend; its engines refuse with the lines. */
template <class Family>
struct none_linked {};

/** A backend a default may take: linked, having the family here, not decided at run time. */
template <class Family>
struct usable {
    template <class Backend>
    using fn = std::bool_constant<linked<Backend> && family_has<Family, Backend> &&
                                  !family_runtime<Family, Backend>>;
};

/** The first usable backend of a ranking, else none_linked. */
template <class Family, class Ranking>
using first_usable = boost::mp11::mp_eval_if_c<
    boost::mp11::mp_find_if_q<Ranking, usable<Family>>::value == boost::mp11::mp_size<Ranking>::value,
    none_linked<Family>, boost::mp11::mp_at, Ranking,
    boost::mp11::mp_find_if_q<Ranking, usable<Family>>>;

/** A profile's choice for a family; a backend profile falls back to automatic. */
template <class Family, class Profile>
struct choose {
    using type = std::conditional_t<family_has<Family, Profile>, Profile,
                                    typename choose<Family, profiles::automatic>::type>;
};
template <class Family>
struct choose<Family, profiles::fastest> {
    using type = first_usable<Family, ranking_t<Family, orders::throughput>>;
};
template <class Family>
struct choose<Family, profiles::smallest> {
    using type = first_usable<Family, ranking_t<Family, orders::size>>;
};
template <class Family>
struct choose<Family, profiles::portable> {
    using type = portable_t<Family>;
};
template <class Family>
struct choose<Family, profiles::automatic> {
#ifdef WEBCPP_CRYPTO_WEBASSEMBLY
    using type = typename choose<Family, profiles::smallest>::type;
#else
    using type = typename choose<Family, profiles::fastest>::type;
#endif
};

/** A family's override, when the program gives one (generated: one specialisation per family). */
template <class Family>
struct override_of {
    using type = void;
};

}  // namespace webcpp::crypto::detail

namespace webcpp::crypto {

/**
 The default backend of a family: its override when the program gives one, else the profile's
 choice among the linked backends (design D5).

 @tparam Family A tag of `webcpp::crypto::families`.
*/
template <class Family>
using default_backend_t = std::conditional_t<
    std::is_void_v<typename detail::override_of<Family>::type>,
    typename detail::choose<Family, profile>::type, typename detail::override_of<Family>::type>;

/** The default backend of the AES block cipher and AES-GCM, the genesis's name. */
using default_backend = default_backend_t<families::gcm>;

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_DETAIL_SELECT_HPP
```

  The generator writes, per family, `family_has<Family, Backend>`, `family_runtime<Family,
  Backend>`, `portable_t<Family>`, and the overrides, one `#ifdef` per backend the family has,
  with the two-at-once refusal naming both:

```cpp
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_SODIUM) + defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_OPENSSL) > 1
#error "webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_SODIUM and WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_OPENSSL are both defined. Keep one."
#endif
#ifdef WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_SODIUM
template <>
struct override_of<families::ed25519> {
    using type = backends::sodium;
};
#ifdef _MSC_VER
#pragma detect_mismatch("webcpp_crypto_default_ed25519", "sodium")
#endif
#endif
#ifdef WEBCPP_CRYPTO_DEFAULT_BACKEND_ED25519_OPENSSL
template <>
struct override_of<families::ed25519> {
    using type = backends::openssl;
};
#ifdef _MSC_VER
#pragma detect_mismatch("webcpp_crypto_default_ed25519", "openssl")
#endif
#endif
```

  (with three or more backends, the generator writes one `#error` per pair, so that the
  message names exactly the two defined). For `gcm` the genesis's `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`
  and `_OPENSSL` select the same specialisations as `_GCM_BEARSSL` and `_GCM_OPENSSL`.
- [ ] **Step 4: The seeded `detail/rankings.hpp`** (Task 11 regenerates it from measurements):
  for `gcm`, throughput natively `openssl, bearssl, sodium` and size on WebAssembly `bearssl,
  openssl`, the genesis's measured order, which keeps version 0.1's defaults; for the
  libsodium families, `sodium` alone. Its form:

```cpp
template <>
struct ranking<families::gcm, orders::throughput, targets::native_x86_64> {
    using type = boost::mp11::mp_list<backends::openssl, backends::bearssl>;
};
template <class Family, class Order>
using ranking_t = typename ranking<Family, Order, targets::current>::type;
```

  with `targets::current` chosen from `WEBCPP_TARGET_*` and `__x86_64__`/`_M_X64` or
  `__aarch64__`/`_M_ARM64`, and `targets::native_other` falling back to the `portable` choice.
- [ ] **Step 5: The messages.** `profile_crypto_twice` (`WEBCPP_CRYPTO_PROFILE_FASTEST` and
  `_PORTABLE`: the `#error` naming both), `profile_webcpp_twice`
  (`WEBCPP_PROFILE_FASTEST` and `_SMALLEST`), `override_ed25519_twice`, `profile_gcm_twice`
  (the genesis's define beside `_GCM_BEARSSL`),
  `none_linked_x25519` (the generated lines), each in its own directory.
- [ ] **Step 6: Run** every lane; `ciphers::aes256_gcm::backend_type` is unchanged on every lane against
  version 0.1 (the genesis's scenario "The backend names" passes). Commit
  `crypto: one profile chooses every default, with per-family overrides`.

## Task 6: Ed25519 and Ed25519ph on libsodium

**Files:**
- Create: `include/webcpp/crypto/curves/ed25519.hpp`, `test/ed25519_sodium_test.cpp`,
  `test/ed25519_wycheproof_sodium_test.cpp`, `test/ed25519ph_sodium_test.cpp`,
  `test/messages/ed25519_on_bearssl/`, `test/messages/ed25519ph_sha256/`.
- Modify: `include/webcpp/crypto/backends/sodium.hpp` (the engine), `meta/capabilities.json`,
  `test/Jamfile`.

**Interfaces:** the engine a backend specialises:

```cpp
template <> struct ed25519_engine<tags::ed25519, B> {
    static boost::system::result<void> make(ed25519_state<tags::ed25519, B>&,
        std::span<std::uint8_t const, 32> seed) noexcept;
    static void public_key(ed25519_state<tags::ed25519, B> const&, std::span<std::uint8_t, 32>) noexcept;
    static boost::system::result<void> sign(ed25519_state<tags::ed25519, B> const&,
        std::span<std::uint8_t const> message, std::span<std::uint8_t, 64> signature) noexcept;
    static boost::system::result<void> verify(std::span<std::uint8_t const, 32> public_key,
        std::span<std::uint8_t const> message, std::span<std::uint8_t const, 64> signature) noexcept;
    static boost::system::result<void> x25519_secret(ed25519_state<tags::ed25519, B> const&,
        std::span<std::uint8_t, 32>) noexcept;
    static void reset(ed25519_state<tags::ed25519, B>&) noexcept;
};
```

- [ ] **Step 1: The failing tests.** `ed25519_sodium_test.cpp` runs RFC 8032 section 7.1's
  tests 1, 2, 3 and SHA(abc) (secret, public key, message, signature written into the test),
  the guards (a 31-byte seed is `invalid_key_size`; a moved-from key pair is `invalid_state`;
  a signature changed in one bit is `invalid_signature`), `generate()` under the failing
  entropy seam (`entropy_unavailable`), and the conversion's agreement with
  `crypto_sign_ed25519_pk_to_curve25519`. `ed25519_wycheproof_sodium_test.cpp` reads
  `testvectors_v1/ed25519_test.json` and expects every `valid` case accepted and every
  `invalid` case refused (libsodium is strict, so no case is listed as an exception here).
- [ ] **Step 2: `curves/ed25519.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Ed25519 signatures (RFC 8032): a key pair made from a 32-byte seed, signatures of 64 bytes,
 and verification, strict on libsodium.

 @see "Ed25519", in the guide.
*/
#ifndef WEBCPP_CRYPTO_CURVES_ED25519_HPP
#define WEBCPP_CRYPTO_CURVES_ED25519_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/select.hpp>
#include <webcpp/crypto/error.hpp>
#include <webcpp/crypto/random_device.hpp>  // lint-world: random_device draws generate()'s seed
#include <webcpp/crypto/secret.hpp>

#include <boost/system/result.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace webcpp::crypto::detail {

/** What an Ed25519 key pair stores on a backend; never refuses (the genesis's D3). */
template <class Name, class Backend>
struct ed25519_state;

/** The work of Ed25519 on a backend; the primary refuses, naming the backends that have it. */
template <class Name, class Backend>
struct ed25519_engine {
    static_assert(dependent_false<Backend>, WEBCPP_CRYPTO_DETAIL_REFUSAL("this backend", ED25519));
};

}  // namespace webcpp::crypto::detail

namespace webcpp::crypto::curves::ed25519 {

/**
 An Ed25519 key pair.

 @tparam Backend The backend; by default the family's (`default_backend_t<families::ed25519>`).
*/
template <class Backend = default_backend_t<families::ed25519>>
class basic_keypair {
    using state_type = detail::ed25519_state<tags::ed25519, Backend>;
    using engine = detail::ed25519_engine<tags::ed25519, Backend>;

public:
    /** The family, for `supports_v`. */
    using family_type = families::ed25519;
    /** The algorithm, for `supports_v`. */
    using name_type = tags::ed25519;
    /** The backend. */
    using backend_type = Backend;
    /** The seed's size. */
    static constexpr std::size_t seed_size = 32;
    /** The public key's size. */
    static constexpr std::size_t public_key_size = 32;
    /** The signature's size. */
    static constexpr std::size_t signature_size = 64;

    /**
     Makes a key pair from a seed, RFC 8032's secret key.

     @param seed 32 bytes.
     @return The key pair; `invalid_key_size`; `backend_failure`.
    */
    static boost::system::result<basic_keypair> make(std::span<std::uint8_t const> seed) noexcept {
        if (seed.size() != seed_size) {
            return error::invalid_key_size;
        }
        basic_keypair made;
        auto const keyed = engine::make(made.state_, seed.template first<seed_size>());
        if (!keyed) {
            return keyed.error();
        }
        made.keyed_ = true;
        return made;
    }

    /**
     Makes a key pair from a seed drawn from `random_device`.

     @return The key pair; `entropy_unavailable`; `backend_failure`.
    */
    static boost::system::result<basic_keypair> generate() noexcept {
        secret<seed_size> seed;
        auto const drawn = random_device().fill(seed.writable());  // lint-world: the seed
        if (!drawn) {
            return drawn.error();
        }
        return make(seed.bytes());
    }

    /** Moves the key pair; the source is wiped and empty. */
    basic_keypair(basic_keypair&& other) noexcept : keyed_(std::exchange(other.keyed_, false)) {
        state_ = other.state_;
        engine::reset(other.state_);
    }

    /** Moves over this key pair, which is wiped first. */
    basic_keypair& operator=(basic_keypair&& other) noexcept {
        if (this != &other) {
            engine::reset(state_);
            keyed_ = std::exchange(other.keyed_, false);
            state_ = other.state_;
            engine::reset(other.state_);
        }
        return *this;
    }

    /** Not copyable: a secret key has one owner. */
    basic_keypair(basic_keypair const&) = delete;
    /** Not copyable: a secret key has one owner. */
    basic_keypair& operator=(basic_keypair const&) = delete;

    /** Wipes the secret key. */
    ~basic_keypair() { engine::reset(state_); }

    /**
     The public key.

     @return 32 bytes; `invalid_state` moved from.
    */
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, 32>> public_key() const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        std::array<std::uint8_t, 32> key{};
        engine::public_key(state_, key);
        return key;
    }

    /**
     Signs a message.

     @param message Any bytes.
     @return The 64-byte signature; `invalid_state`; `backend_failure`.
    */
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, 64>> sign(
        std::span<std::uint8_t const> message) const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        std::array<std::uint8_t, 64> signature{};
        auto const signed_ = engine::sign(state_, message, signature);
        if (!signed_) {
            return signed_.error();
        }
        return signature;
    }

    /**
     The X25519 secret key of this key pair, RFC 7748's form of the clamped first half of the
     seed's SHA-512.

     @return 32 secret bytes; `invalid_state`; `backend_failure`.
    */
    [[nodiscard]] boost::system::result<secret<32>> x25519_secret() const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        secret<32> converted;
        auto const done = engine::x25519_secret(state_, converted.writable());
        if (!done) {
            return done.error();
        }
        return converted;
    }

private:
    basic_keypair() noexcept = default;

    state_type state_{};
    bool keyed_ = false;
};

/** The key pair on the default backend. */
using keypair = basic_keypair<>;

/**
 Ed25519 verification as libsodium does it: `S` below the group order, `A` and `R` canonical,
 neither of small order (design D31).
*/
struct strict {};

/**
 Verifies an Ed25519 signature under a verification policy (design D31).

 @tparam Policy `strict` (libsodium's rules), the only policy declared.
 @tparam Backend The backend; by default the family's.
 @param public_key 32 bytes.
 @param message The signed bytes.
 @param signature 64 bytes.
 @return Nothing; `invalid_public_key` for a key of another size; `invalid_signature`.
*/
template <class Policy = strict, class Backend = default_backend_t<families::ed25519>>
boost::system::result<void> verify(std::span<std::uint8_t const> public_key,
                                   std::span<std::uint8_t const> message,
                                   std::span<std::uint8_t const> signature) noexcept {
    static_assert(std::is_same_v<Policy, strict>,
                  "webcpp::crypto: Ed25519's only verification policy is curves::ed25519::strict.");
    if (public_key.size() != 32) {
        return error::invalid_public_key;
    }
    if (signature.size() != 64) {
        return error::invalid_signature;
    }
    return detail::ed25519_engine<tags::ed25519, Backend>::verify(
        public_key.first<32>(), message, signature.first<64>());
}

}  // namespace webcpp::crypto::curves::ed25519

#endif  // WEBCPP_CRYPTO_CURVES_ED25519_HPP
```

- [ ] **Step 3: The libsodium engine**, in `backends/sodium.hpp` under
  `#ifdef WEBCPP_CRYPTO_CURVES_ED25519_HPP` (so that a program that does not include `curves/ed25519.hpp`
  pays nothing), and `curves/ed25519.hpp` includes the default backend's header as `ciphers/aes256_gcm.hpp`
  does:

```cpp
namespace webcpp::crypto::detail {

template <>
struct ed25519_state<tags::ed25519, backends::sodium> {
    // libsodium's secret key: the seed, then the public key.
    std::array<unsigned char, crypto_sign_ed25519_SECRETKEYBYTES> secret_key{};
};

template <>
struct ed25519_engine<tags::ed25519, backends::sodium> {
    using state = ed25519_state<tags::ed25519, backends::sodium>;

    static boost::system::result<void> make(state& s, std::span<std::uint8_t const, 32> seed) noexcept {
        auto const ready = sodium_ready();
        if (!ready) {
            return ready;
        }
        std::array<unsigned char, crypto_sign_ed25519_PUBLICKEYBYTES> public_key{};
        if (crypto_sign_ed25519_seed_keypair(public_key.data(), s.secret_key.data(), seed.data()) != 0) {
            return error::backend_failure;
        }
        return {};
    }
    static void public_key(state const& s, std::span<std::uint8_t, 32> out) noexcept {
        static_cast<void>(crypto_sign_ed25519_sk_to_pk(out.data(), s.secret_key.data()));
    }
    static boost::system::result<void> sign(state const& s, std::span<std::uint8_t const> message,
                                            std::span<std::uint8_t, 64> out) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY);
        if (crypto_sign_ed25519_detached(out.data(), nullptr, message.data(), message.size(),
                                         s.secret_key.data()) != 0) {
            return error::backend_failure;
        }
        return {};
    }
    static boost::system::result<void> verify(std::span<std::uint8_t const, 32> public_key,
                                              std::span<std::uint8_t const> message,
                                              std::span<std::uint8_t const, 64> signature) noexcept {
        auto const ready = sodium_ready();
        if (!ready) {
            return ready;
        }
        if (crypto_sign_ed25519_verify_detached(signature.data(), message.data(), message.size(),
                                                public_key.data()) != 0) {
            return error::invalid_signature;
        }
        return {};
    }
    static boost::system::result<void> x25519_secret(state const& s, std::span<std::uint8_t, 32> out) noexcept {
        if (crypto_sign_ed25519_sk_to_curve25519(out.data(), s.secret_key.data()) != 0) {
            return error::backend_failure;
        }
        return {};
    }
    static void reset(state& s) noexcept { wipe_object(s.secret_key); }
};

}  // namespace webcpp::crypto::detail
```

- [ ] **Step 4: Ed25519ph.** `curves::ed25519::ph<Hash, Backend>` with `begin()`, `update(span)`,
  `sign(keypair)` and `verify(public_key, signature)`, a state of
  `crypto_sign_ed25519ph_state` wiped on destruction and move; `Hash` other than
  `webcpp::hash::sha2_512` is refused by
  `static_assert(std::is_same_v<Hash, hash::sha2_512>, "webcpp::crypto: Ed25519ph is defined over SHA-512 (RFC 8032 section 5.1); name curves::ed25519::ph<webcpp::hash::sha2_512>.")`.
  Its test runs RFC 8032 section 7.3 with the message in one piece and in two.
- [ ] **Step 5: The table and the messages.** `capabilities.json` gains `ed25519ph`;
  `ed25519_on_bearssl` checks the generated refusal; `ed25519ph_sha256` the static assertion.
- [ ] **Step 6: Run** every lane; commit `crypto: Ed25519 and Ed25519ph on libsodium`.

## Task 6b: Ed25519's verification policy and the twelve edge cases

**Files:** create `meta/ed25519_cases.json`, `test/ed25519_speccheck_test.cpp`; superproject
`third_party/Jamfile` (`/third_party//ed25519-speccheck`, test data, its define
`WEBCPP_TEST_ED25519_SPECCHECK`) and the shallow submodule `third_party/libs/ed25519-speccheck`
at `65519336fda78a3d016e947df6d82848aca0c9da` with its chapter 10 row; modify `tools/crypto/docs.py`
(the table "which backend accepts which case").

- [ ] **Step 1:** `ed25519_speccheck_test` reads `cases.json` (twelve cases: message, public
  key, signature) and, for each Ed25519 backend linked, records the raw verdict and the verdict
  of `verify<strict, Backend>`, comparing them with `meta/ed25519_cases.json`; under `strict`
  every backend must give libsodium's verdicts. The file starts with libsodium's row:

```json
{"source": {"repository": "novifinancial/ed25519-speccheck", "commit": "65519336", "file": "cases.json"},
 "backends": {"sodium": {"raw": ["?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?"],
                         "strict": ["?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?"]}}}
```

  where each `?` becomes `accept` or `reject` from the first run, reviewed against the paper's
  table before it is committed (a disagreement with the paper is reported, never smoothed).
- [ ] **Step 2:** under node on Emscripten, the same twelve cases through WebCrypto's
  `verify` (Ed25519), recorded as `webcrypto-node`, an outside view, not asserted against
  `strict`.
- [ ] **Step 3:** `docs.py` generates the table into the curves chapter, with the paragraph on
  ed25519-dalek's default `verify` and ZIP-215 (design D31, O1). Commit
  `crypto: Ed25519's strict policy, and the twelve edge cases per backend`.

## Task 7: X25519 and the conversion

**Files:** create `include/webcpp/crypto/curves/x25519.hpp`, `test/x25519_sodium_test.cpp`,
`test/x25519_wycheproof_sodium_test.cpp`; modify `backends/sodium.hpp`,
`meta/capabilities.json`, `test/Jamfile`.

- [ ] **Step 1: The failing tests.** RFC 7748 section 5.2's two scalar multiplications and
  section 6.1's Alice and Bob; a peer key of order 8 from Wycheproof gives `weak_public_key`;
  `x25519_secret()` of a key pair, through `curves::x25519::public_key`, equals
  `curves::ed25519::to_x25519_public` of its public key; a small-order Ed25519 key gives
  `invalid_public_key` in the conversion; Wycheproof's `x25519_test.json`, every case, with the
  cases libsodium refuses for an all-zero result expected as `weak_public_key`.
- [ ] **Step 2: `curves/x25519.hpp`:**

```cpp
namespace webcpp::crypto::curves::x25519 {

/**
 X25519 (RFC 7748) on a backend: the type `supports_v` names, whose functions the free
 functions below call.

 @tparam Backend The backend; by default the family's.
*/
template <class Backend = default_backend_t<families::x25519>>
struct algorithm {
    /** The family, for `supports_v`. */
    using family_type = families::x25519;
    /** The algorithm, for `supports_v`. */
    using name_type = tags::x25519;
    /** The backend. */
    using backend_type = Backend;
    /** The secret key's, the public key's and the shared secret's size. */
    static constexpr std::size_t key_size = 32;

    /**
     The public key of a secret key.

     @param secret_key 32 bytes.
     @return 32 bytes; `invalid_key_size`; `backend_failure`.
    */
    static boost::system::result<std::array<std::uint8_t, 32>> public_key(
        std::span<std::uint8_t const> secret_key) noexcept {
        if (secret_key.size() != key_size) {
            return error::invalid_key_size;
        }
        std::array<std::uint8_t, 32> out{};
        auto const done = engine::base(secret_key.first<32>(), out);
        if (!done) {
            return done.error();
        }
        return out;
    }

    /**
     The shared secret of a secret key and a peer's public key, refused when it is all zero.

     @param secret_key 32 bytes.
     @param peer_public_key 32 bytes.
     @return 32 secret bytes; `invalid_key_size`; `invalid_public_key`; `weak_public_key`.
    */
    static boost::system::result<secret<32>> agree(std::span<std::uint8_t const> secret_key,
                                                   std::span<std::uint8_t const> peer_public_key) noexcept {
        if (secret_key.size() != key_size) {
            return error::invalid_key_size;
        }
        if (peer_public_key.size() != key_size) {
            return error::invalid_public_key;
        }
        secret<32> shared;
        auto const done = engine::agree(secret_key.first<32>(), peer_public_key.first<32>(), shared.writable());
        if (!done) {
            return done.error();
        }
        // Every backend gives the same answer: libsodium refuses an all-zero result itself,
        // and the others are checked here, in constant time (design D8).
        if (is_all_zero(shared.bytes())) {
            return error::weak_public_key;
        }
        return shared;
    }

private:
    using engine = detail::x25519_engine<tags::x25519, Backend>;
};

/** The public key of a secret key: `algorithm<Backend>::public_key`. */
template <class Backend = default_backend_t<families::x25519>>
boost::system::result<std::array<std::uint8_t, 32>> public_key(
    std::span<std::uint8_t const> secret_key) noexcept {
    return algorithm<Backend>::public_key(secret_key);
}

/** The shared secret, refused when all zero: `algorithm<Backend>::agree`. */
template <class Backend = default_backend_t<families::x25519>>
boost::system::result<secret<32>> agree(std::span<std::uint8_t const> secret_key,
                                        std::span<std::uint8_t const> peer_public_key) noexcept {
    return algorithm<Backend>::agree(secret_key, peer_public_key);
}

}  // namespace webcpp::crypto::curves::x25519

namespace webcpp::crypto::curves::ed25519 {

/**
 The X25519 public key of an Ed25519 public key (the birational map), declared in
 `curves/ed25519.hpp`.

 @return 32 bytes; `invalid_public_key` for a key that is not canonical or of small order.
*/
template <class Backend = default_backend_t<families::ed25519>>
boost::system::result<std::array<std::uint8_t, 32>> to_x25519_public(
    std::span<std::uint8_t const> ed25519_public_key) noexcept;

}  // namespace webcpp::crypto::curves::ed25519
```

  `is_all_zero` is `detail::is_all_zero`, an OR-reduction over the bytes with no early exit,
  added to `detail/wipe.hpp`'s neighbour `detail/bytes.hpp`.
- [ ] **Step 3: The engine** on libsodium: `base` is `crypto_scalarmult_curve25519_base`,
  `agree` is `crypto_scalarmult_curve25519` (whose -1 is `weak_public_key`), the conversion is
  `crypto_sign_ed25519_pk_to_curve25519` (whose -1 is `invalid_public_key`).
- [ ] **Step 4: Run** every lane; commit `crypto: X25519 and the Ed25519 to X25519 conversion`.

## Task 7b: SPKI, PKCS#8 and PEM for Ed25519 and X25519

**Files:** create `include/webcpp/crypto/pem.hpp`, `include/webcpp/crypto/curves/ed25519/formats.hpp`,
`include/webcpp/crypto/curves/x25519/formats.hpp`, `test/formats_25519_sodium_test.cpp`,
`test/oracle/webcrypto_formats.mjs`; modify `backends/sodium.hpp`, `meta/capabilities.json`.

- [ ] **Step 1: The failing tests:** round trips of SPKI and PKCS#8 for random key pairs;
  OpenSSL's `i2d_PUBKEY` and `i2d_PrivateKey` (the source-built provider, linked in the test
  only) produce the same bytes and decode ours; malformed inputs refused with
  `invalid_encoding` and nothing written (Ed25519's OID under X25519, a trailing byte, a length
  in long form, PKCS#8 version 2 with the public key, a PEM label other than the expected, a
  line of 63 characters, a carriage return); under node, WebCrypto's `importKey` and
  `exportKey` with `spki` and `pkcs8` agree byte for byte.
- [ ] **Step 2: The constants and the functions:**

```cpp
namespace webcpp::crypto::detail {

/** RFC 8410's SPKI before an Ed25519 public key: SEQUENCE, AlgorithmIdentifier 1.3.101.112, BIT STRING. */
inline constexpr std::array<std::uint8_t, 12> ed25519_spki_prefix{
    0x30, 0x2a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x21, 0x00};
/** RFC 8410's PKCS#8 (version 1) before an Ed25519 seed: OneAsymmetricKey, OCTET STRING of an OCTET STRING. */
inline constexpr std::array<std::uint8_t, 16> ed25519_pkcs8_prefix{
    0x30, 0x2e, 0x02, 0x01, 0x00, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x04, 0x22, 0x04, 0x20};
// X25519: the same with 0x6e in place of 0x70 (1.3.101.110).

}  // namespace webcpp::crypto::detail

namespace webcpp::crypto::curves::ed25519 {

/** The SPKI's size, 44 bytes. */
inline constexpr std::size_t spki_size = 44;
/** The PKCS#8's size, 48 bytes. */
inline constexpr std::size_t pkcs8_size = 48;

/** Writes a public key's SPKI into `out` (at least `spki_size` bytes); returns its length. */
template <class Backend = default_backend_t<families::ed25519_formats>>
boost::system::result<std::size_t> encode_spki(std::span<std::uint8_t const> public_key,
                                               std::span<std::uint8_t> out) noexcept;
/** The public key of an SPKI, refused unless it is exactly RFC 8410's form. */
template <class Backend = default_backend_t<families::ed25519_formats>>
boost::system::result<std::array<std::uint8_t, 32>> decode_spki(std::span<std::uint8_t const> spki) noexcept;
/** Writes a key pair's PKCS#8 (version 1, the seed) into `out`; returns its length. */
template <class Backend = default_backend_t<families::ed25519_formats>>
boost::system::result<std::size_t> encode_pkcs8(keypair const& key, std::span<std::uint8_t> out) noexcept;
/** The key pair of a PKCS#8, refused unless it is exactly RFC 8410's version 1 form. */
template <class Backend = default_backend_t<families::ed25519>>
boost::system::result<basic_keypair<Backend>> decode_pkcs8(std::span<std::uint8_t const> pkcs8) noexcept;

}  // namespace webcpp::crypto::curves::ed25519
```

  On libsodium the encoders copy the prefix and the key bytes, and the decoders compare the
  prefix with `equal` and check the length, which is the re-encoding test of design D30 for a
  fixed layout. `pem.hpp`'s `pem::armour` and `pem::unarmour` use libsodium's
  `sodium_bin2base64` and `sodium_base642bin` (original variant), lines of 64 characters, LF
  only, `-----BEGIN <label>-----`; `unarmour` re-armours and compares.
- [ ] **Step 3: Run** every lane; commit `crypto: SPKI, PKCS#8 and PEM for Ed25519 and X25519`.

## Task 8: `ciphers::aead<Family, Backend>` and the three ChaCha20-Poly1305 AEADs

**Files:** create `include/webcpp/crypto/ciphers/aead.hpp`, `ciphers/chacha20_poly1305.hpp` (the genesis's
planned header, now an alias), `test/support/aead_checks.hpp`,
`test/chacha_sodium_test.cpp`, `test/chacha_wycheproof_sodium_test.cpp`,
`test/xchacha_sodium_test.cpp`, `test/chacha_original_sodium_test.cpp`; modify
`backends/sodium.hpp`, `detail/engines.hpp` (the genesis's `chacha_state`/`chacha_engine`
replaced by `aead_state`/`aead_engine`), `meta/capabilities.json`, `test/Jamfile`.

**Interfaces:**

```cpp
namespace webcpp::crypto::families {
/** RFC 8439's ChaCha20-Poly1305: 32-byte key, 12-byte nonce, 16-byte tag. */
struct chacha20_poly1305 {
    static constexpr std::size_t key_size = 32, iv_size = 12, tag_size = 16;
    static constexpr std::uint64_t max_message_size = (std::uint64_t{1} << 38) - 64;
};
/** The original ChaCha20-Poly1305, an 8-byte nonce. */
struct chacha20_poly1305_original {
    static constexpr std::size_t key_size = 32, iv_size = 8, tag_size = 16;
    static constexpr std::uint64_t max_message_size = ~std::uint64_t{0} - 16;
};
/** XChaCha20-Poly1305, a 24-byte nonce safe to draw at random. */
struct xchacha20_poly1305 {
    static constexpr std::size_t key_size = 32, iv_size = 24, tag_size = 16;
    static constexpr std::uint64_t max_message_size = ~std::uint64_t{0} - 16;
};
}  // namespace webcpp::crypto::families

namespace webcpp::crypto::detail {
template <class Family, class Backend> struct aead_state;
template <class Family, class Backend> struct aead_engine;  // primary: the generated refusal
// A specialisation provides: init(state&, span<const uint8_t> key), seal(state&, iv, ad, data,
// digest<16>& tag), open(state&, iv, ad, data, array<uint8_t,16> const& tag), reset(state&).
}
```

- [ ] **Step 1: The checks** `test/support/aead_checks.hpp`, generic over an AEAD type: the
  guards in order (state, IV size, tag size, length), in-place seal and open, a tampered byte of
  the ciphertext, the tag, the additional data and the IV each giving `authentication_failed`
  with `data` zeroed, a guard's refusal leaving `data` untouched, adjacent and overlapping
  spans (the genesis's Review Focus 2), empty inputs (Review Focus 3), moves of empty objects
  (Review Focus 6). RFC 8439 section 2.8.2 and appendix A.5 for `ciphers::chacha20_poly1305`; the
  XChaCha draft's appendix A.3.1 for `ciphers::xchacha20_poly1305`; libsodium's own
  `test/default/aead_chacha20poly1305.exp` cases for the original; Wycheproof's
  `chacha20_poly1305_test.json` and `xchacha20_poly1305_test.json`.
- [ ] **Step 2: `ciphers/aead.hpp`**: the genesis's `ciphers::chacha20_poly1305` class body (its Task 35),
  with `Family` in place of the fixed sizes, `family_type` and `name_type` exposed for
  `supports_v`, and `aead_engine<Family, Backend>` in place of `chacha_engine<Backend>`;
  `ciphers/chacha20_poly1305.hpp` reads
  `template <class Backend = default_backend_t<families::chacha20_poly1305>> using chacha20_poly1305 = ciphers::aead<families::chacha20_poly1305, Backend>;`
  and the two others alike.
- [ ] **Step 3: The libsodium engine**, one template for the three, since libsodium's detached
  functions differ only by name:

```cpp
template <class Family>
struct sodium_aead_calls;
template <>
struct sodium_aead_calls<families::chacha20_poly1305> {
    static constexpr auto encrypt = &crypto_aead_chacha20poly1305_ietf_encrypt_detached;
    static constexpr auto decrypt = &crypto_aead_chacha20poly1305_ietf_decrypt_detached;
};
template <>
struct sodium_aead_calls<families::chacha20_poly1305_original> {
    static constexpr auto encrypt = &crypto_aead_chacha20poly1305_encrypt_detached;
    static constexpr auto decrypt = &crypto_aead_chacha20poly1305_decrypt_detached;
};
template <>
struct sodium_aead_calls<families::xchacha20_poly1305> {
    static constexpr auto encrypt = &crypto_aead_xchacha20poly1305_ietf_encrypt_detached;
    static constexpr auto decrypt = &crypto_aead_xchacha20poly1305_ietf_decrypt_detached;
};

template <class Family>
    requires requires { sodium_aead_calls<Family>::encrypt; }
struct aead_state<Family, backends::sodium> {
    std::array<unsigned char, 32> key{};
};

template <class Family>
    requires requires { sodium_aead_calls<Family>::encrypt; }
struct aead_engine<Family, backends::sodium> {
    using state = aead_state<Family, backends::sodium>;
    using calls = sodium_aead_calls<Family>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        auto const ready = sodium_ready();
        if (!ready) {
            return ready;
        }
        std::copy(key.begin(), key.end(), s.key.begin());
        return {};
    }
    static boost::system::result<void> seal(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            boost::hash2::digest<16>& tag) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY);
        unsigned long long tag_length = 0;
        // libsodium encrypts in place when the output is the input (its documented contract).
        if (calls::encrypt(data.data(), tag.data(), &tag_length, data.data(), data.size(),
                           additional_data.data(), additional_data.size(), nullptr, iv.data(),
                           s.key.data()) != 0) {
            return error::backend_failure;
        }
        return {};
    }
    static boost::system::result<void> open(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            std::array<std::uint8_t, 16> const& tag) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_SODIUM_CANARY);
        if (calls::decrypt(data.data(), nullptr, data.data(), data.size(), tag.data(),
                           additional_data.data(), additional_data.size(), iv.data(), s.key.data()) != 0) {
            return error::authentication_failed;
        }
        return {};
    }
    static void move(state& to, state& from) noexcept {
        to.key = from.key;
        wipe_object(from.key);
    }
    static void reset(state& s) noexcept { wipe_object(s.key); }
};
```

  libsodium's detached decryption verifies the tag before it writes plaintext, so `data` is
  untouched on `authentication_failed`; `aead` zeroes it anyway (the genesis's D5), which the
  test checks.
- [ ] **Step 4: Run** every lane; commit
  `crypto: one AEAD template, and ChaCha20-Poly1305, its original form and XChaCha20-Poly1305 on libsodium`.

## Task 9: `hybrid::box`, sealed boxes and `ciphers::secretbox`, in both forms

**Files:** create `include/webcpp/crypto/hybrid/box.hpp`, `ciphers/secretbox.hpp`, `test/box_sodium_test.cpp`,
`test/secretbox_sodium_test.cpp`; modify `backends/sodium.hpp`, `meta/capabilities.json`,
`test/Jamfile`.

- [ ] **Step 1: The failing tests:** libsodium's `test/default/box.exp`, `box2.exp`,
  `secretbox.exp` and `box_seal.exp` cases (the key pairs and nonces of NaCl's tests), the
  generic AEAD checks of Task 8 applied to `ciphers::secretbox` and to a `hybrid::box` made from a key pair and
  a peer key, a sealed box opened by its recipient and refused (`authentication_failed`, `data`
  zeroed) by another key pair or after a change, and a `hybrid::box` with a small-order peer key
  (`weak_public_key`, libsodium's `beforenm` refusing it).
- [ ] **Step 2: The interface.**

```cpp
// <webcpp/crypto/hybrid/box.hpp> and <webcpp/crypto/hybrid/sealed_box.hpp>
namespace webcpp::crypto::hybrid {

/**
 crypto_box: X25519 agreement and an AEAD, computed once per pair of keys (libsodium's
 beforenm), then `seal` and `open` in place with the tag apart.
*/
template <class Family = families::box_xsalsa20poly1305,
          class Backend = default_backend_t<families::box_xsalsa20poly1305>>
class basic_box {
public:
    static constexpr std::size_t key_size = 32, iv_size = 24, tag_size = 16;
    static boost::system::result<basic_box> make(std::span<std::uint8_t const> secret_key,
                                                 std::span<std::uint8_t const> peer_public_key) noexcept;
    boost::system::result<boost::hash2::digest<16>> seal(std::span<std::uint8_t const> iv,
                                                         std::span<std::uint8_t> data) noexcept;
    boost::system::result<void> open(std::span<std::uint8_t const> iv, std::span<std::uint8_t> data,
                                     std::span<std::uint8_t const> tag) noexcept;
    // move-only, wiped: the precomputed key is the state.
};
using box = basic_box<>;
using box_xchacha20poly1305 = basic_box<families::box_xchacha20poly1305>;

/** Sealed boxes: anonymous encryption to a public key (an ephemeral key pair libsodium draws). */
template <class Backend = default_backend_t<families::sealed_box>>
struct basic_sealed_box {
    static constexpr std::size_t overhead = 48;  // the ephemeral public key and the tag
    /** Seals `message` into `out`, of `message.size() + overhead` bytes. */
    static boost::system::result<void> seal(std::span<std::uint8_t const> recipient_public_key,
                                            std::span<std::uint8_t const> message,
                                            std::span<std::uint8_t> out) noexcept;
    /** Opens `sealed` into `out`, of `sealed.size() - overhead` bytes, with the recipient's keys. */
    static boost::system::result<void> open(std::span<std::uint8_t const> recipient_public_key,
                                            std::span<std::uint8_t const> recipient_secret_key,
                                            std::span<std::uint8_t const> sealed,
                                            std::span<std::uint8_t> out) noexcept;
};
using sealed_box = basic_sealed_box<>;

}  // namespace webcpp::crypto::hybrid

// <webcpp/crypto/ciphers/secretbox.hpp>
namespace webcpp::crypto::ciphers {

/** crypto_secretbox: XSalsa20-Poly1305 (or XChaCha20-Poly1305) under a 32-byte key. */
template <class Backend = default_backend_t<families::secretbox_xsalsa20poly1305>>
using secretbox = aead<families::secretbox_xsalsa20poly1305, Backend>;
template <class Backend = default_backend_t<families::secretbox_xchacha20poly1305>>
using secretbox_xchacha20poly1305 = aead<families::secretbox_xchacha20poly1305, Backend>;

}  // namespace webcpp::crypto::ciphers
```

  `ciphers::secretbox` has no additional data: its family's `max_additional_data_size` is 0, and `aead`
  refuses a non-empty one with `invalid_input_size`. The sealed box's sizes are checked first
  (`invalid_input_size` for a wrong `out`), and a failed `open` zeroes `out`.
- [ ] **Step 3: The libsodium engines:** `crypto_box_beforenm` and
  `crypto_box_detached_afternm`/`crypto_box_open_detached_afternm`, their
  `curve25519xchacha20poly1305` twins, `crypto_box_seal`/`crypto_box_seal_open`,
  `crypto_secretbox_detached`/`crypto_secretbox_open_detached` and their XChaCha twins.
- [ ] **Step 4: Run** every lane; commit `crypto: box, sealed boxes and secretbox on libsodium`.

## Task 10: `kx`

**Files:** create `include/webcpp/crypto/curves/x25519/kx.hpp`, `test/kx_sodium_test.cpp`; modify
`backends/sodium.hpp`, `meta/capabilities.json`, `test/Jamfile`.

- [ ] **Step 1: The failing test:** libsodium's `test/default/kx.exp` (its seeded key pairs),
  the client's transmit key equal to the server's receive key and the reverse, a small-order
  peer key giving `weak_public_key`, the session keys wiped (`secret<32>`).
- [ ] **Step 2: The interface:**

```cpp
// <webcpp/crypto/curves/x25519/kx.hpp>
namespace webcpp::crypto::curves::x25519 {

/** The session keys of one side: what it receives with and what it transmits with. */
struct session_keys {
    secret<32> receive;
    secret<32> transmit;
};

/** A crypto_kx key pair, made from a 32-byte seed or drawn. */
template <class Backend = default_backend_t<families::kx>>
class kx_keypair {
public:
    static boost::system::result<kx_keypair> make(std::span<std::uint8_t const> seed) noexcept;
    static boost::system::result<kx_keypair> generate() noexcept;
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, 32>> public_key() const noexcept;
    /** The client's keys, from the server's public key. */
    [[nodiscard]] boost::system::result<session_keys> client_session_keys(
        std::span<std::uint8_t const> server_public_key) const noexcept;
    /** The server's keys, from the client's public key. */
    [[nodiscard]] boost::system::result<session_keys> server_session_keys(
        std::span<std::uint8_t const> client_public_key) const noexcept;
    // move-only, wiped
};

}  // namespace webcpp::crypto::curves::x25519
```

  over `crypto_kx_seed_keypair`, `crypto_kx_client_session_keys` and
  `crypto_kx_server_session_keys`, whose -1 is `weak_public_key`.
- [ ] **Step 3: Run** every lane; commit `crypto: crypto_kx on libsodium`.

## Task 11: The measurement table, `measure.py`, the lanes `sizes` and `throughput`

Superseded by the revision of 2026-11-17, Task 11' (design D32): sizes and throughput from release builds, as references; nothing ranked.

**Files:**
- Create: `libs/crypto/meta/measurements.json`, superproject `tools/crypto/measure.py`,
  `tools/test/crypto_measure_test.py`, `libs/crypto/test/sizes/<family>_<backend>.cpp` (one
  program per cell, each calling the family's fixed operation once), `test/sizes/Jamfile`,
  `test/throughput/throughput.cpp`, `libs/crypto/.github/workflows/runners.yml`, generated
  `doc/measurements.adoc`.
- Modify: `detail/rankings.hpp` (now generated), `meta/capabilities.json` (the `heavy` column,
  written by the tool), `tools/ci/matrix.py` (the own lane `sizes` on every push, naming its
  targets).

- [ ] **Step 1: The file's form:**

```json
{
  "reference": {"runner": "ubuntu-24.04", "architecture": "x86_64"},
  "toolchains": {"wasi-sdk": "34", "emscripten": "6.0.11", "wasmtime": "50.0.1", "node": "26.7.0"},
  "sizes": {
    "chacha20_poly1305": {
      "sodium": {"native": {"raw": 0, "gzip": 0}, "wasip2": {"raw": 0, "gzip": 0},
                 "wasip3": {"raw": 0, "gzip": 0}, "emscripten": {"raw": 0, "gzip": 0}}
    }
  },
  "throughput": {
    "chacha20_poly1305": {
      "operation": "seal and open of 16 KiB",
      "sodium": {"linux-x86_64": {"median_us": 0.0, "runs": 5}, "linux-arm64": {}, "macos-arm64": {},
                 "wasmtime": {}, "node": {}}
    }
  }
}
```

  (zeros stand for the first measurement's numbers, which Step 4 writes).
- [ ] **Step 2: The failing tools test** `tools/test/crypto_measure_test.py`: from a table with
  two backends, the size ranking orders by raw bytes then gzip; two throughput medians within
  10 % keep the committed order and outside it reorder; an operation whose median exceeds
  1,000 microseconds on the reference runner or under node there is written `"heavy": true`
  in `capabilities.json`, families whose cost the caller's data sets (`aead`, `hybrid::box`,
  `ciphers::secretbox`, `ciphers::secretstream`, `stream`, `mac`) never are; `--check-sizes` fails naming an
  entry whose rebuilt size differs.
- [ ] **Step 3: `tools/crypto/measure.py`** builds `libs/crypto/test/sizes` for each target at
  `<variant>release <optimization>space <debug-symbols>off` with `--strip`, reads each
  program's size against the baseline, runs `throughput` five times per runner, and writes the
  JSON and its derived files; `--check-sizes` rebuilds the WebAssembly programs and compares,
  for the own lane `sizes`; `--throughput-only` is what `runners.yml` runs on the three native
  runners, uploading its partial file as an artifact that a maintainer merges with
  `--merge <files>`. The script opens with the licence notice and follows the genesis's
  Task 22 for its b2 invocations (its departures C16 and C17 apply: webcpp's own throughput
  program, and b2's `space` in place of `-O2`).
- [ ] **Step 4: Measure** increment 1's cells and the genesis's (`gcm` on BearSSL and OpenSSL),
  libsodium's X25519 and Ed25519 also with `HAVE_AMD64_ASM` on Linux x86-64 in a scratch build
  (recorded in `doc/measurements.adoc` as design D7 asks, not used), regenerate, and check that
  `ciphers::aes256_gcm::backend_type` keeps version 0.1's value on every lane.
- [ ] **Step 5: Run** the tools test, the lane `sizes` on its four targets and `runners.yml` by
  hand (`gh workflow run` is the owner's; locally, `measure.py --throughput-only` on the
  developer's machine is reported and not committed). Commit (superproject)
  `tools/crypto: measure.py, the size lane and the heavy column`; (library)
  `crypto: webcpp's own measurements behind the defaults`.

## Task 12: Threads, and the refusal of a heavy variant without them

**Files:** create `include/webcpp/crypto/detail/heavy.hpp`, `test/threads_config_test.cpp`,
`test/messages/heavy_without_threads/`; modify `test/Jamfile`.

Increment 1 has no heavy operation (Ed25519, X25519, kx and the AEADs are expected cheap; Task
11's measurement confirms it). This task builds the detection and the refusal every later
heavy operation uses, so that increments 3, 6, 7 and 8 only declare theirs.

- [ ] **Step 1: The failing tests.** `threads_config_test.cpp` built four times: natively
  with `<threading>single` (no `WEBCPP_CRYPTO_HAS_THREADS`), natively with `<threading>multi`
  (defined), with `<threading>multi` and `<define>WEBCPP_CRYPTO_NO_THREADS` (not defined), and
  on wasip2 (never defined); on Emscripten, without and with `-pthread` (the second a fixture of
  `crypto_jamfiles_test.py`, as Emscripten's messages are). `heavy_without_threads` instantiates
  a test-only heavy operation in a build without threads and checks the whole text.
- [ ] **Step 2: `detail/heavy.hpp`:**

```cpp
namespace webcpp::crypto::detail {

/**
 The refusal of a heavy operation's `async_` variant in a build without threads (design D23).
 Each such variant's body starts with it, so that a program fails only where it calls one.
*/
#ifdef WEBCPP_CRYPTO_HAS_THREADS
#define WEBCPP_CRYPTO_DETAIL_REQUIRE_THREADS(async_name, sync_name)
#else
#define WEBCPP_CRYPTO_DETAIL_REQUIRE_THREADS(async_name, sync_name)                              \
    static_assert(dependent_false<decltype(token)>, "webcpp::crypto: " async_name                \
                  " runs on another thread and needs a build with threads (natively b2 "         \
                  "threading=multi; on Emscripten -pthread). Without threads, call " sync_name ".")
#endif

}  // namespace webcpp::crypto::detail
```

  With threads, a heavy variant is
  `async_<name>(Executor executor, <arguments>, CompletionToken&& token)`, initiated by
  `boost::asio::async_initiate`, which posts the synchronous call to `executor` and dispatches
  the completion to the handler's associated executor; Task 12 writes the generic initiator
  `detail::run_heavy(executor, function, token)` and its test with a test-only heavy function
  under `<threading>multi`, a `thread_pool` and a 10 ms timer on the caller's `io_context`.
- [ ] **Step 3: Run** the lanes and the fixture; commit
  `crypto: threads detection, and heavy operations' variants refused without threads`.

## Task 13: Vectors, libsodium's own suite, node's WebCrypto as the oracle

**Files:** create `test/c_suites/sodium/Jamfile` (libsodium's `test/default/*.c` each a program
compared with its `.exp`, on every target; webcpp::hash may already run it, in which case this
task adds crypto's families to the same lane), `test/oracle/webcrypto_25519.mjs`,
`test/oracle_25519_test.cpp`; modify `test/Jamfile`.

- [ ] **Step 1:** the own lane `c-suites` runs libsodium's `sign`, `hybrid::box`, `box2`, `box7`,
  `box8`, `box_seal`, `ciphers::secretbox`, `secretbox2`, `secretbox7`, `secretbox8`,
  `aead_chacha20poly1305`, `aead_chacha20poly1305_ietf` (where present in 1.0.22),
  `aead_xchacha20poly1305`, `kx`, `scalarmult`, `ed25519_convert` programs on every target,
  each bounded as the genesis's C suites are.
- [ ] **Step 2:** on Emscripten under node, `oracle_25519_test` writes 200 seeded Ed25519 key
  pairs, messages and signatures and 200 X25519 agreements; `webcrypto_25519.mjs` imports the
  keys into node's `crypto.subtle` (`Ed25519`, `X25519`), verifies every signature and derives
  every shared secret, and prints `oracle ok 400`; the test compares the counts. Node 26.7.0
  has both algorithms; the browser lane of increment 4 extends this to Chrome.
- [ ] **Step 3: Run**; commit `crypto: libsodium's own suite and WebCrypto's oracle for Ed25519 and X25519`.

## Task 14: The constant-time lane over the new code

Extended by the revision of 2026-11-17: it builds the genesis's Task 20 whole, version 0.1's backends first, and the ChaCha engines of Task 8b.

**Files:** the genesis's Task 20 files (`test/constant_time/ctgrind_test.cpp`,
`msan_test.cpp`, `dudect_test.cpp`, `detail/declassify.hpp` already empty in version 0.1), now
built, with libsodium's paths: Ed25519 signing (the seed secret), X25519 agreement (the secret
key), the three AEADs' seal and open (key, plaintext, tag), `hybrid::box` and `ciphers::secretbox`, `kx`, and
webcpp's `equal`, `is_all_zero` and `secret<N>`'s wipe.

- [ ] **Step 1:** write the harnesses as the genesis's Task 20 says, the planted leak first
  (a branch on a key byte), seen failing under memcheck and MemorySanitizer.
- [ ] **Step 2:** add each libsodium path; mark the verdict of `verify` and `open` declassified
  where webcpp branches on it.
- [ ] **Step 3:** dudect natively and under wasmtime on X25519 agreement and Ed25519 signing,
  classes differing only in the secret.
- [ ] **Step 4: Run** the lane on Linux x86-64 (and arm64 in `runners.yml`); commit
  `crypto: the constant-time lane, over libsodium's code and the new guards`.

## Task 15: The documentation's form, and its generator

Amended by the revision of 2026-11-17: `generate.py` in place of `docs.py`, no profile page, the default per target with its reason.

The owner's requirements (design D19) bind this task and every later increment's
documentation step.

**Files:** create superproject `tools/crypto/docs.py`, `tools/test/crypto_docs_test.py`;
library `meta/use_cases.json`, `doc/ciphers.adoc`, `doc/curves.adoc`, `doc/hybrid.adoc`, `doc/root.adoc` (each a
chapter opening with its generated comparison table), the sections of increment 1's algorithms
in them, `doc/backends.adoc` (generated matrices), `doc/choose.adoc` (the generated decision
table), `doc/profiles.adoc`, `doc/llms.txt` (generated), `doc/guide/{wallets,p2p,passwords,everyday}.adoc`,
`LICENSE-LIBSODIUM-DOC.txt`, the fixtures of every example; modify `meta/capabilities.json`
(the documentation fields), `doc/jamfile.adoc`, `doc/errors.adoc`, `README.md` (libsodium named,
ISC), `doc/changelog.adoc`.

- [ ] **Step 1: The failing tools test** `crypto_docs_test.py`: from a two-row table, the
  ciphers comparison table has the columns of D19; a `used_in` entry without `source` is
  refused naming the row; the decision table's row links its example; `llms.txt` lists the
  function with its use case and backends; `--check` fails after a measurement changes.
- [ ] **Step 2: The documentation fields** of each increment-1 row, for example:

```json
{"family": "xchacha20_poly1305", "name": "xchacha20_poly1305", "title": "XChaCha20-Poly1305",
 "group": "ciphers",
 "doc": {"aead": true, "key_bytes": 32, "nonce_bytes": 24, "misuse_resistant": false,
         "random_nonce_safe": true, "standards": [{"name": "draft-irtf-cfrg-xchacha", "url": "https://datatracker.ietf.org/doc/draft-irtf-cfrg-xchacha/"}],
         "used_in": [{"what": "libsodium's secretstream", "source": "https://doc.libsodium.org/secret-key_cryptography/secretstream"}],
         "webcrypto": false,
         "choose_when": "you encrypt many messages under one key and want to draw each nonce at random",
         "example": "xchacha_seal"}}
```

  and `meta/use_cases.json` rows such as
  `{"need": "encrypt messages in a chat app, on the web and natively", "algorithm": "ciphers::xchacha20_poly1305", "profile": "automatic", "example": "xchacha_seal"}`.
- [ ] **Step 3: `tools/crypto/docs.py`** reads `capabilities.json`, `measurements.json` and
  `use_cases.json` and writes the comparison table at the head of each group chapter, the
  backend list of every API section (with the default per target and profile from
  `rankings`), the backends matrices, the decision table and `llms.txt`, each opening with the
  generated-file notice; `--check` compares.
- [ ] **Step 4: The prose:** each section of increment 1's algorithms in D19's style (lead
  paragraph in webcpp's own words, the ordinary use case first, what goes wrong without it, how
  it works, a compiled example included by tag with its expected output, pitfalls); the
  libsodium families adapted from `jedisct1/libsodium-doc` at `6d1ea27` with the attribution
  line, `LICENSE-LIBSODIUM-DOC.txt` its `LICENSE` verbatim; Wikipedia only as links. The X25519
  section warns that a shared secret is not a key (derive one with webcpp::hash's HKDF) and
  about the all-zero secret; the AEAD sections about nonce reuse; Ed25519 about libsodium's
  strictness.
- [ ] **Step 5: The teaching chapters' first sections:** everyday apps (sealed boxes and
  `ciphers::secretbox` for a file-sharing app), peer-to-peer protocols (Ed25519 identities and
  `curves::x25519::kx_keypair`), each after a named, linked application built on noble, its
  protocol steps shown as a domain library's.
- [ ] **Step 6:** `b2 doc` builds the pages; every example fixture runs; `docs.py --check` and
  the tools test pass; the changelog gains the lines of increment 1. Commit (superproject)
  `tools/crypto: docs.py, the generated tables and llms.txt`; (library)
  `crypto: the documentation of increment 1, its tables generated from the data`.

## Task 16: Increment 1 whole

Amended by the revision of 2026-11-17: `generate.py --check`, the fixed defaults reviewed, the genesis's Task 36 items.

- [ ] **Step 1:** the rulebook: CODING-GUIDELINES.md chapter 17's crypto entry records the
  foundation's acceptance and increment 1's state; chapter 10's `sodium` row names crypto's
  canary beside hash's; the library's `AGENTS.md` names the generators.
- [ ] **Step 2:** green: the lint clean; every lane (native clang, gcc-15, wasip2, wasip3,
  emscripten; MSVC and Apple clang in the CI); the own lanes `c-suites`, `sizes`,
  `constant-time`; `capabilities.py --check`; the tools tests.
- [ ] **Step 3:** a review against the genesis's Review Focus and this plan's: the profile's
  default of every family on every lane; empty inputs on each new family; every message from a
  test of its own; nothing allocated (the counting test over the new families).
- [ ] **Step 4:** the pull requests (library, superproject), which the owner merges; then the
  proposal's status is updated as CONTRIBUTING.md says.

---

# Increments 2 to 8 (outlined; each is written in full task form when it starts)

## Increment 2: libsecp256k1's remaining modules (beside increment 1)

Files: `include/webcpp/crypto/curves/secp256k1.hpp` and `curves/secp256k1/{recovery,tweak,ellswift,musig,silentpayments}.hpp`,
their tests, `doc/secp256k1*.adoc`, `guide/cryptocurrencies.adoc`. Steps:
1. `ecdh` returns `secret<32>` (BREAKING, its changelog line); `ecdh_x` and `ecdh_point` through
   `secp256k1_ecdh` with webcpp's copying hash functions; Wycheproof's ECDH file and NIP-44's
   vectors.
2. `sign_recoverable`, `recover`, hedged `sign_ecdsa`; libsecp256k1's own recovery tests.
3. Tweaks, negation, combination, `parse_public_key` and `serialize_public_key` in both forms;
   SPKI (RFC 5480 with the secp256k1 OID, compressed and uncompressed) and PKCS#8 (RFC 5915's
   `ECPrivateKey` with its public key, as OpenSSL writes it) as fixed layouts around the key
   bytes, PEM through `pem.hpp`, strict decoding, OpenSSL's encoders as the oracle (design D30).
4. ElligatorSwift; BIP324's `packet_encoding_test_vectors.csv` and its ellswift vectors.
5. MuSig2 with `musig::secret_nonce` consumed by move; BIP327's vectors.
6. Silent payments; BIP352's `send_and_receive_test_vectors.json`.
7. No Taproot tweak, `taproot_output_key` or `from_lax_der` (version 0.1 moves them out, D24).
   Capability rows, constant-time paths (signing, tweaks, nonce generation), the
   cryptocurrencies chapter's secp256k1 sections, green, review, pull requests.

## Increment 3: BearSSL's remaining parts (a stream after 1)

1. `ecdsa` and `ecdh` on P-256, P-384 and P-521 (`br_ec_all_m31`, `br_ec_p256_m62` natively,
   `br_ecdsa_i31_*`), with Wycheproof's ECDSA and ECDH files per curve; then their key and
   signature forms (design D12): `to_der` and strict `from_der` (BearSSL's decoder, re-encoded
   and compared), `public_key` from the uncompressed point and from raw `x` and `y` with
   validation, and its serialisation; Wycheproof's DER files per curve, NIST's ECDSA signature
   and key validation vectors; the compressed form's absent BearSSL cells; a passkey's ES256
   assertion verified from its COSE key's coordinates, with the curves chapter's note that
   WebAuthn's protocol stays outside crypto; PKCS#8 for the P-curves on BearSSL
   (`br_skey_decoder`, `br_encode_ec_pkcs8_der`) and PEM (`br_pem_decoder`, `br_pem_encode`),
   strict by re-encoding; SPKI's BearSSL cells absent (OpenSSL and Botan later); OpenSSL and
   WebCrypto's `spki` and `pkcs8` as oracles (design D30).
2. X25519 (`br_ec_c25519_m62`/`m31`) and ChaCha20-Poly1305 (the genesis's Task 35 engine),
   joining libsodium in agreement natively and on WebAssembly.
3. `ctr`, `cbc` (PKCS #7, `invalid_padding`), `ccm`, `eax`; NIST ACVP's AES vectors (the
   `acvp` submodule, pinned here by `gh api`, its chapter 10 row).
4. RSA: PKCS #1 v1.5 and PSS, OAEP, key generation; PKCS#8 with PKCS #1 inside on BearSSL
   (`br_skey_decoder`, `br_encode_rsa_pkcs8_der`), strict; Wycheproof's RSA files; the heavy
   variants (key generation, private operations at 3072 bits and above as measured).
5. Measurements, constant-time paths, the documentation step of D19 (table rows, sections,
   generated pages), green, review, pull requests.

## Increment 4: the browser backend and the asynchronous family (a stream after 1)

1. `backends/browser.hpp`: the JavaScript key table (`EM_JS`), handles, lifetime; refused off
   Emscripten.
2. `async.hpp`: `async_initiate` over webcpp::asio, `webcpp::asio::browser::async_await` for
   promises, `asio::post` over synchronous backends; only the algorithms WebCrypto offers.
3. The errors of rejected promises, cancellation, run-time availability (node with an
   algorithm removed).
4. The lane `browser`: node and Chrome, every algorithm WebCrypto has against every synchronous
   backend on `main` (D13), and the audio worklet's `not_supported`.
5. Pages, green, review, pull requests.

## Increment 5: libsodium completed (a stream after 1)

1. `ristretto255` and `edwards25519` points and scalars; RFC 9496's vectors.
2. `post_quantum::ml_kem_768` and `post_quantum::x_wing`; the X-Wing draft's vectors, ACVP's ML-KEM-768; their heavy rows
   as measured.
3. `ciphers::secretstream`, `ciphers::aegis128l`, `ciphers::aegis256`, `ciphers::aes256_gcm_sodium` (run-time), `auth`,
   `poly1305`, the stream ciphers and the cores; Wycheproof's AEGIS files, the drafts' vectors,
   libsodium's own.
4. Utilities: `random_uniform`, `random_bytes_deterministic`, `memory.hpp`, `padding.hpp`,
   `encoding.hpp`, `ciphers/ipcrypt.hpp`, `guarded_memory.hpp` (native only).
5. Measurements, constant-time paths, pages (the privacy chapter's post-quantum and
   secretstream sections), green, review, pull requests.

## Increment 6: blst (a stream after 1)

1. The submodule `third_party/libs/blst` at v0.3.17 (`54e6e55`), its two translation units,
   assembly with `__BLST_PORTABLE__` natively on Linux and macOS, `__BLST_NO_ASM__` on MSVC and
   WebAssembly, its canary naming the configuration, chapter 10's row.
2. `curves::bls12_381::min_pk` and `min_sig`, proofs of possession, `hash_to_g1`, `hash_to_g2`; the
   heavy variants (verification, aggregate and fast aggregate verification, aggregation over
   many signatures).
3. The consensus specifications' BLS vectors (a test-data submodule pinned here), RFC 9380's
   suites; blst's EIP-2333 key tree is not wrapped (D24); measurements, pages, green, review,
   pull requests.

## Increment 7: Botan as a submodule (a stream after 1)

1. The spike: Botan 3.14.1's `configure.py` for wasip2 and wasip3 with the nearest generic OS
   and its OS features off; its tests under wasmtime; the report fixes the table's cells.
2. `tools/webcpp-botan.jam`: `configure.py` per target into the variant directory, each listed
   source an object; the submodule at `01af88c`; the canary; chapter 5's new rule and chapter
   10's row; the genesis's Task 37 dropped.
3. The backend: every family the table lists for Botan, `detail::botan_random`, the boundary's
   `try`; SPKI, PKCS#8 and PEM for every key type Botan has, strict by re-encoding (D30), and
   Ed25519's twelve cases recorded (D31); Wycheproof and ACVP per family; the heavy variants (SLH-DSA, RSA, ML-DSA as measured).
4. Measurements, the allocation departure re-measured, pages, green, review, pull requests.

## Increment 8: OpenSSL's remaining algorithms (a stream after 1)

1. P-curves, Ed25519, Ed448, X25519, X448, RSA through `EVP_PKEY`, a context made in `make`;
   SPKI, PKCS#8 and PEM for each (`i2d_PUBKEY`, `d2i_PUBKEY`, `i2d_PrivateKey`, the PEM
   functions), strict by re-encoding (D30); Ed25519's twelve cases recorded (D31).
2. ML-KEM, ML-DSA, SLH-DSA (OpenSSL 3.5); ACVP's vectors; the heavy variants as measured.
3. AES-CTR, CBC, CCM, OCB, KW, KWP, SIV, GCM-SIV, CMAC, ChaCha20-Poly1305; the allocation per
   operation measured and recorded per algorithm.
4. Measurements (the `no-asm` builds on MSVC, wasip2 and wasip3 included), pages, green,
   review, pull requests.

## Increment 9: KZG over c-kzg-4844 (after increment 6)

1. The submodule `third_party/libs/c-kzg-4844` at v2.1.8 (`e125905`), `src/` compiled file by
   file against webcpp's blst (c-kzg's `blst` submodule never checked out); c-kzg's own tests on
   blst v0.3.17, and the pin rule of D25 if they fail; chapter 10's row; the canary.
2. `commitments::kzg::settings::make` from a caller's setup (the ceremony's file as test data), the
   commitment, proof and cell functions; the settings' allocation recorded; the heavy variants
   as measured; c-kzg's reference vectors.
3. Measurements, pages (the cryptocurrencies chapter's KZG section, versioned hashes named as
   webcpp::ethereum's), green, review, pull requests.

## Increment 10: BN254 over mcl (a stream after increment 1)

1. The spike: mcl v4.43 (`50e4c6f`) with `MCL_DONT_USE_XBYAK` on each target, generic C++ and
   upstream's generated code where it exists, measured; the configuration per target fixed in
   the canary's name.
2. The submodule, its sources file by file, `curves::bn254::g1`, `g2`, `pairing_check`; vectors from
   mcl's own tests and the pairing's bilinearity checks on random public points; the heavy
   variant of a many-pair check.
3. Measurements, pages ("public data only"), green, review, pull requests.

## Increment 11: Shamir secret sharing over sss (a stream after increment 1)

1. The submodule `third_party/libs/sss` at `9f2a10b` (its `randombytes` submodule never checked
   out), `hazmat.c`, `sss.c`, `tweetnacl.c`, and webcpp's `src/sss_randombytes.c` handing over
   the thread-local bytes the wrapper drew; the spike on MSVC's missing VLAs (clang-cl, or the
   cell absent); the diff since v0.1.0 read and recorded.
2. `secret_sharing::shamir::split_key`, `combine_key`, `split`, `combine`, the guards `2 <= k <= n <= 255`,
   sss's own `test_hazmat.c` and `test_sss.c` in `c-suites`, every subset recovery test, the
   constant-time lane over the split and the combination.
3. Measurements, pages (SLIP-39 named as webcpp::bitcoin's), green, review, pull requests.

## Increment 12: HPKE (after increment 8; WebAssembly after increment 7)

1. `hybrid::hpke<Kem, Kdf, Aead, Backend>` on OpenSSL's `OSSL_HPKE_*` on native, wasip2 and wasip3;
   RFC 9180's test vectors for every suite both backends have; the sequence number's guard.
2. Botan's `hpke` module enabled in the Botan configuration of increment 7; the same vectors;
   agreement between OpenSSL and Botan natively; Emscripten served by Botan.
3. Capability rows, measurements, pages, green, review, pull requests.
