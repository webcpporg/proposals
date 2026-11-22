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

# Increment 2: libsecp256k1's remaining modules (version 0.2; stream B)

Written in full on 2026-11-17, before any code, as `roadmap.md` requires of an outlined stream,
and reviewed locally before stream B's code starts; by the controller's ruling of 2026-11-17
(the owner's rule "no CI until a stream is complete"), its proposals pull request opens with
stream B's code pull requests (Task 2.7, Step 7). It replaces the outline of the same name; tasks.md group 2 points here (Tasks 2.1 to 2.7).
The design is `../design.md` D9, D11, D24, D29, D30 and D32; the requirements are
`../specs/crypto/spec.md`'s "secp256k1: recovery, raw ECDH, tweaks and key forms",
"secp256k1: ElligatorSwift, MuSig2 and silent payments", "secp256k1: ECDH and the secret",
"Key formats: SPKI, PKCS#8 and PEM, strict", "Scope: primitives only", "Allocation", "Tests:
constant time for the new code" and the documentation requirements (D19).

**Branches.** The superproject's branch `crypto/secp256k1-modules` and the library's branch of
the same name start from stream A's branches (`crypto/foundation-1`) once A's Task 3
(`secret<N>`, the errors 15 to 20) is committed; they rebase onto A's branches when A's Task 4'
(the capability table) and Task 7b (`pem.hpp`) are committed, and onto `main` when stream A
merges; increment 2 merges after A (`roadmap.md`, "Merging"). Tasks 2.1 to 2.6 run in order
(each builds on the types of the one before); Task 2.7 closes the stream.

**What it starts from** (crypto `main` at `d242be8`, superproject `main`):

- `include/webcpp/crypto/curves/secp256k1.hpp` calls libsecp256k1 directly, with no engine
  trait: every function takes `class Backend = backends::secp256k1`, refused by
  `static_assert(webcpp::crypto::detail::secp256k1_backend<Backend>, WEBCPP_CRYPTO_DETAIL_SECP256K1_ONLY)`
  (the message test `test/messages/secp256k1_other_backend/` and its MSVC case); every
  function reads `WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY` with `read_canary`, and the free
  functions call `secp256k1_selftest()` and use `secp256k1_context_static`. `basic_keypair`
  holds a randomised signing context allocated once from the `std::pmr::memory_resource` that
  `make` is given, a `secp256k1_keypair`, and `keyed_`; it is move-only and wipes on move and
  destruction. `detail::secp256k1_parse_public_key` refuses every prefix but 02, 03 and 04,
  and `detail::secp256k1_secret` wipes a local secret on every path.
- `include/webcpp/crypto/backends/secp256k1.hpp` includes `<secp256k1.h>`,
  `<secp256k1_ecdh.h>`, `<secp256k1_extrakeys.h>`, `<secp256k1_preallocated.h>` and
  `<secp256k1_schnorrsig.h>`.
- Superproject `third_party/Jamfile`: `/third_party//secp256k1` compiles libsecp256k1 v0.8.0
  (`third_party/libs/secp256k1` at `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d`) with
  `ENABLE_MODULE_ECDH`, `_RECOVERY`, `_EXTRAKEYS`, `_SCHNORRSIG`, `_MUSIG`, `_ELLSWIFT` and
  `_SILENTPAYMENTS` all `=1` (`.secp256k1-config`), and `secp256k1-test-config` gives a test
  the same defines with `libs/secp256k1/include` and `libs/secp256k1/src` on its include path.
  `/third_party//bips` (bitcoin/bips at `927b6de9915c9262615a6399de51b200f81e5aa4`) defines
  `WEBCPP_TEST_BIPS` and depends on `bip-0340/test-vectors.csv` and
  `bip-0341/wallet-test-vectors.json` only; `/third_party//wycheproof` defines
  `WEBCPP_TEST_WYCHEPROOF`.
- `test/secp256k1_c_api_test.cpp` calls the recovery and ElligatorSwift modules through the C
  API, "which webcpp does not wrap"; libsecp256k1's own `src/tests.c`, with every module, runs
  in the own lane `c-suites` on the four targets (`test/c_suites/Jamfile`).
- `doc/tables/crypto.json`'s `operations` hold `keypair`, `verify`, `encodings` and `tweak`
  for `curves::secp256k1`; `doc/secp256k1.adoc` includes `generated/backends-<id>.adoc` per
  operation; the teaching chapters are `doc/wallets.adoc` and `doc/p2p.adoc`; the decision
  table is `doc/choose.adoc` over `use_cases`; `sources` already holds `sec2`, `bip32`,
  `bip340`, `bip341`, `nip01`, `nip44`, `ethereum`, `rfc6979` and `libsecp256k1`.

**Rules for every task of this increment** (beside the Global Constraints):

- **The shape is version 0.1's** (design D11, "thin wrappers"): each function calls its
  libsecp256k1 function directly, takes `class Backend = backends::secp256k1` last, starts with
  the `secp256k1_backend` static assertion and `read_canary`, and is `[[nodiscard]]` and
  `noexcept` (but `make`, as version 0.1's, which may throw from the resource). No engine trait
  is added: libsecp256k1 is the family's only backend (D32), and a second backend of a
  secp256k1 family would bring its engine in its own increment. A backend other than
  libsecp256k1 is refused, in every header, with version 0.1's
  `WEBCPP_CRYPTO_DETAIL_SECP256K1_ONLY` until the rebase onto A's Task 4'; Task 2.7, Step 1
  replaces it by Task 4''s generated refusal (the controller's ruling of 2026-11-17: one source
  of refusal text, design D33). Likewise the default `class Backend = backends::secp256k1` of
  Tasks 2.1 to 2.6 becomes `class Backend = default_backend_t<families::<family>>` there, as
  Task 7b's formats write it.
- **libsecp256k1's illegal callback is never reached.** Every precondition libsecp256k1 checks
  with `ARG_CHECK` (a count of zero, a static context where a signing one is needed, a nonce
  made for another key, a used nonce, an object whose magic is not set, a null pointer inside
  an array, a non-null array of zero elements) is checked first by webcpp and refused with a
  value of `error`; every opaque type that wraps a libsecp256k1 struct whose magic
  libsecp256k1 checks (`public_key`, `musig::aggregate_key`, `public_nonce`,
  `aggregate_nonce`, `session`, `partial_signature`, `silent_payments::prevouts_summary`) has
  a private default constructor and `friend struct webcpp::crypto::detail::secp256k1_access`,
  so that only the functions that return one make one, and its test asserts
  `!std::is_default_constructible_v<T>`; an array of zero elements is passed as `nullptr`; each such refusal has a case in the task's test, since the callback's default
  aborts the program.
- **Contexts.** A function that multiplies the generator (`ecmult_gen`: signing, key creation,
  `ellswift_create`, `musig_nonce_gen`, silent payments' sender and labels) takes a key pair
  and uses its randomised context; every other function uses `secp256k1_context_static` after
  `secp256k1_selftest()`, as version 0.1's free functions do. No function creates a context.
- **Nothing is allocated** (spec "Allocation": libsecp256k1 allocates nothing but the key
  pair's context). A function over many keys, nonces or outputs, for which libsecp256k1 takes
  arrays of pointers, takes a `workspace` (Task 2.3) the caller lends and a size function that
  says how many bytes it needs; the workspace is wiped before the function returns (the
  controller's ruling of 2026-11-17 keeps this span over a `std::pmr` resource, which would
  still allocate). The carver `detail::secp256k1_workspace` hands out its arrays in the order
  each function's size function lists them, each aligned to `alignof(std::max_align_t)`, so a
  size function is the sum of its arrays' bytes plus one `alignof(std::max_align_t)` of padding
  per array; an array of zero elements takes no bytes and is handed out as `nullptr`.
- **Errors.** A key that does not parse is `invalid_public_key`; a secret that is zero or not
  below the group order is `invalid_secret_key`; a tweak not below the group order, or one
  that gives no valid key (a zero secret, the point at infinity), is `invalid_secret_key`, as
  version 0.1's `tweak_xonly` reports it (not D10's `invalid_scalar`: libsecp256k1 returns the
  same 0 for a tweak out of range and for a result that is no key, and version 0.1 already
  answers both with `invalid_secret_key`; one header keeps one word); an encoded nonce or label that is not a point is
  `invalid_point` (16); a partial signature not below the group order, or one that does not
  verify, is `invalid_signature`; a span of the wrong length, or a workspace too short, is
  `invalid_input_size`; a moved-from or consumed object, or a null key pair pointer, is
  `invalid_state`; an SPKI, PKCS#8 or PEM that is not exactly the form webcpp writes, its
  point or its secret included, is `invalid_encoding` (19, the spec's "Key formats"); a libsecp256k1
  failure that a valid input cannot cause is `backend_failure`.
- **Secret outputs are `secret<N>`** (design D9): the ECDH outputs, a tweaked or negated
  secret key, ElligatorSwift's shared secret, and a silent payment's spending tweak.
- **Primitives only** (design D24): no BIP32 derivation, no `TapTweak`, no
  `taproot_output_key`, no `from_lax_der`, no address, no transaction, no PSBT, no bech32m,
  no NIP-44 payload; BIP vectors are test data, and where a vector needs a protocol step (a
  tagged hash, a script's key) the test computes it, or reads data libsecp256k1's own tests
  derived from the same file.
- **Tests** are `crypto-run` programs (exceptions on and off), written first and seen
  failing, on the lanes native, wasip2, wasip3, emscripten and gcc-15 (the genesis plan's
  "The lanes"; `$B2` is the Global Constraints' command), MSVC and Apple clang in the CI.
  **Run the lanes** means, from the worktree's root:
  `$B2 -a --build-dir=bin/lane-native libs/crypto/test libs/crypto/example`;
  `$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test libs/crypto/example`;
  the same with `clang-wasip3`, `tools/boost/asio-wasip3/run.sh` and `bin/lane-wasip3`;
  `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/crypto/test libs/crypto/example`;
  `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/crypto/test libs/crypto/example`;
  and `python3 libs/crypto/doc/tables/generate.py --check`. Expected: each b2 run ends with
  its `...updated N targets...` line and prints no `...failed` line and no warning, and
  `generate.py --check` prints nothing and exits 0.
  Each task's header joins the libsecp256k1 `webcpp.headers-alone` line of `test/Jamfile`,
  the convenience header `include/webcpp/crypto.hpp` under `WEBCPP_CRYPTO_HAS_SECP256K1`,
  `test/windows_h_before_test.cpp`, `test/windows_h_after_test.cpp` and
  `test/crypto_secp256k1_test.cpp` (crypto.hpp's libsecp256k1 branch), and its functions join
  `test/secp256k1_allocation_test.cpp` (nothing allocated) and, where they take a key pair,
  `test/support/secp256k1_checks.hpp`'s `expect_empty_keypair` (`invalid_state` moved from).
- **Capability rows.** Each task adds its operations to `doc/tables/crypto.json` in version
  0.1's `operations` form (`id`, `functions`, `namespace`, `api`, `header`, `what`,
  `section`, `backends.secp256k1.tests`), which `generate.py` reads today, and runs
  `python3 libs/crypto/doc/tables/generate.py`, committing `doc/generated/` and
  `doc/llms.txt` with it; `generate.py --check` passes after every commit. At the rebase onto
  A's Task 4' the rows join the `families` form (Task 2.7, Step 1).
- **Documentation per task** (design D19, the owner's `doc-requirements.md`): each task adds
  its section to `doc/secp256k1.adoc` (a lead paragraph in webcpp's words saying what the
  primitive is and what an ordinary application uses it for, how it works, then details; a
  compiled example included by tag with its `.expected` output; its pitfalls, framed as what
  goes wrong without it and what the mistake costs; every fact about the world citing a source
  of `crypto.json`), its `generated/backends-<id>.adoc` include, its `use_cases` rows, its new
  `sources`, and its changelog line in `doc/changelog.adoc`'s `unreleased` tag (`AGENTS.md`,
  chapter 5). Task 2.7 writes the teaching chapters' sections.

**Files of the increment** (library paths under `libs/crypto/`):

| File | Tasks |
| --- | --- |
| `include/webcpp/crypto/curves/secp256k1.hpp` | 2.1 (ECDH), 2.2 (recovery, hedged ECDSA, `public_key`, `point_form`, the uncompressed key, the access struct), 2.3 (`workspace` and its carver) |
| `include/webcpp/crypto/backends/secp256k1.hpp` | 2.2, 2.4, 2.5, 2.6 (each includes its module's C header) |
| `include/webcpp/crypto/curves/secp256k1/tweak.hpp` | 2.3 (the tweaks, negation, combination) |
| `include/webcpp/crypto/curves/secp256k1/formats.hpp` | 2.3 (SPKI, PKCS#8) |
| `include/webcpp/crypto/curves/secp256k1/ellswift.hpp` | 2.4 |
| `include/webcpp/crypto/curves/secp256k1/musig.hpp` | 2.5 |
| `include/webcpp/crypto/curves/secp256k1/silent_payments.hpp` | 2.6 |
| `test/secp256k1_ecdh_test.cpp`, `test/secp256k1_recovery_test.cpp`, `test/secp256k1_keys_test.cpp`, `test/secp256k1_tweak_test.cpp`, `test/secp256k1_formats_test.cpp`, `test/secp256k1_ellswift_test.cpp`, `test/secp256k1_musig_test.cpp`, `test/secp256k1_silent_payments_test.cpp` | 2.1 to 2.6 |
| `test/messages/musig_nonce_copied/`, `test/msvc/musig_nonce_copied/` | 2.5 |
| `test/secp256k1_musig_entropy_test.cpp` | 2.5 |
| `test/support/bip352_vectors.c`, `test/support/bip352_vectors.h` | 2.6 |
| `test/messages/secp256k1_other_backend/` (rewritten), `test/messages/ellswift_other_backend/`, `test/messages/musig_other_backend/`, `test/messages/silent_payments_other_backend/` and their `test/msvc/` cases | 2.7 |
| `example/ecdh_x_coordinate.cpp`, `example/recover_signer.cpp`, `example/key_tweak.cpp`, `example/key_formats.cpp`, `example/ellswift_handshake.cpp`, `example/musig_wallet.cpp`, `example/silent_payment.cpp`, each with its `.expected` | 2.1 to 2.6 |
| `doc/secp256k1.adoc`, `doc/wallets.adoc`, `doc/p2p.adoc`, `doc/changelog.adoc`, `doc/tables/crypto.json` | every task |
| superproject `third_party/Jamfile` (the `bips` target's files), `tools/test/third_party_test.py` | 2.4, 2.5, 2.6 |

The outline's `curves/secp256k1/recovery.hpp` is not created: design D11 makes
`sign_recoverable` a member of the key pair, so recovery lives in `curves/secp256k1.hpp` with
it (Task 2.2). Silent payments' header and namespace are `silent_payments` (design D29: a
header mirrors its namespace), not the outline's `silentpayments`.

**As built (2026-11-19).** The code is on the branches `crypto/secp256k1-modules` of the
library (head `e7c71db`) and the superproject (head `d25b22f`: the code's verified head
`221357e` and the rulebook's chapter 17 on it). Where it differs from the
tasks below, a review accepted the difference or stream B's controller ruled it; each task says
so under "As built" after its heading. Across the increment:

- **The base.** Both branches sit on stream A's final heads (library `1fc5140`, superproject
  `c00bf7a`), whose trees `main`'s merges of stream A (`dae9e84`, `485c5e0`, 2026-11-19)
  equal, so increment 2 was not rebased onto `main` and its pull requests hold its own commits
  alone.
- **The changelog.** `doc/changelog.adoc`'s `unreleased` tag holds at most ten lines of at most
  30 words (the lint), so the tasks' lines below are merged into two: the `curves::secp256k1`
  line ("key pairs, BIP340, hedged and recoverable ECDSA, raw and hashed ECDH, strict DER,
  low-S, tweaks, SPKI, PKCS#8, PEM, BIP 324's ElligatorSwift, BIP 327's MuSig2, BIP 352's
  silent payments") and the `supports_v` line ("secp256k1's modules among them"). No line says
  BREAKING: the changelog has no released section yet (version 0.1 is in the same `unreleased`
  tag).
- **The default aliases' fixtures.** Stream A's rule that every `<family>-default` alias has a
  user's Jamfile fixture gives the superproject
  `tools/test/fixtures/crypto_jamfiles/secp256k1_ellswift_default/`, `secp256k1_musig_default/`
  and `secp256k1_silent_payments_default/`, each run by `crypto_jamfiles_test.py`.
- **Green before the pull requests** is a full local run in place of Task 2.7's Step 5, which is
  the CI's and runs with Step 7: the full lint; the five lanes from scratch (native 344 passed,
  gcc-15 307, wasip2 303, wasip3 291, emscripten 230, none failed, no warning); the own lanes
  `c-suites`, `constant-time` (natively on macOS arm64, on wasip2, and in a Linux arm64
  container: memcheck, MemorySanitizer and dudect) and `sizes`; the docs and the site; the
  tools' tests; MSVC 14.3 and 14.5 in the local Wine lane (`msvc-lane`), with no failure
  outside its known list. The Linux x86-64 run of `constant-time` is the CI's.
- **The rulebook.** Task 2.7's superproject commit is `d25b22f`, "guidelines: chapter 17 states
  webcpp::crypto's increment 2 as built, and increment 1 merged", not Step 7's subject:
  chapter 17's crypto entry gains increment 2's block and records increment 1 as merged.

---

## Task 2.1: `ecdh` returns `secret<32>`; `ecdh_x` and `ecdh_point`

**As built.** As written, but the changelog: its two lines are merged into the
`curves::secp256k1` line ("raw and hashed ECDH"), without BREAKING (the increment's "As
built").

**Files:**
- Modify: `include/webcpp/crypto/curves/secp256k1.hpp` (`ecdh`'s return type, the two new
  members, the two copying hash functions), `test/secp256k1_test.cpp`,
  `test/secp256k1_wycheproof_test.cpp` (its ECDH half moves out), `test/secp256k1_allocation_test.cpp`,
  `test/support/secp256k1_checks.hpp`, `test/Jamfile`, `example/ecdh_session.cpp` and
  `example/Jamfile`, `doc/secp256k1.adoc`, `doc/p2p.adoc` (the session section's code lines
  that name `ecdh`'s type), `doc/tables/crypto.json`, `doc/changelog.adoc`.
- Create: `test/secp256k1_ecdh_test.cpp`, `example/ecdh_x_coordinate.cpp`,
  `example/ecdh_x_coordinate.expected`.

**Interfaces** (members of `curves::secp256k1::basic_keypair<Backend>`; `secret.hpp` is Task
3's):

```cpp
namespace webcpp::crypto::detail {

extern "C" {

/**
 * libsecp256k1's ECDH hash function that hashes nothing: the shared point's x coordinate,
 * 32 bytes, as Wycheproof's and NIP-44's ECDH give it. C linkage, as libsecp256k1's function
 * pointer type has.
 *
 * @return 1, always.
 */
inline int webcpp_crypto_detail_secp256k1_copy_x(unsigned char* output_bytes,
                                                 unsigned char const* x_coordinate,
                                                 unsigned char const* /*y_coordinate*/,
                                                 void* /*user_data*/) noexcept {
    std::memcpy(output_bytes, x_coordinate, 32);
    return 1;
}

/**
 * libsecp256k1's ECDH hash function that writes the shared point, SEC1 uncompressed:
 * 0x04, x, y, 65 bytes.
 *
 * @return 1, always.
 */
inline int webcpp_crypto_detail_secp256k1_copy_point(unsigned char* output_bytes,
                                                     unsigned char const* x_coordinate,
                                                     unsigned char const* y_coordinate,
                                                     void* /*user_data*/) noexcept {
    output_bytes[0] = 0x04;
    std::memcpy(output_bytes + 1, x_coordinate, 32);
    std::memcpy(output_bytes + 33, y_coordinate, 32);
    return 1;
}

}  // extern "C"

}  // namespace webcpp::crypto::detail

namespace webcpp::crypto::curves::secp256k1 {

template <class Backend>
class basic_keypair {
public:
    /**
     * The SHA-256 of the compressed shared point with a peer's public key, libsecp256k1's
     * default.
     *
     * @param public_key_bytes The peer's key, SEC1: 33 bytes, prefix 02 or 03, or 65, prefix 04.
     * @return 32 secret bytes; `invalid_state`; `invalid_public_key`; `backend_failure`.
     */
    [[nodiscard]] boost::system::result<secret<32>> ecdh(
        std::span<std::uint8_t const> public_key_bytes) const noexcept;

    /**
     * The shared point's x coordinate, unhashed: what Wycheproof's ECDH vectors, Nostr's NIP-44
     * and other protocols that hash it themselves take.
     *
     * Tip: the x coordinate is not a key; a protocol derives its keys from it (NIP-44 with
     * HKDF), and that derivation is the protocol's.
     *
     * @param public_key_bytes The peer's key, SEC1, as `ecdh` takes it.
     * @return 32 secret bytes; `invalid_state`; `invalid_public_key`; `backend_failure`.
     */
    [[nodiscard]] boost::system::result<secret<32>> ecdh_x(
        std::span<std::uint8_t const> public_key_bytes) const noexcept;

    /**
     * The shared point itself, SEC1 uncompressed (0x04, x, y).
     *
     * @param public_key_bytes The peer's key, SEC1, as `ecdh` takes it.
     * @return 65 secret bytes; `invalid_state`; `invalid_public_key`; `backend_failure`.
     */
    [[nodiscard]] boost::system::result<secret<65>> ecdh_point(
        std::span<std::uint8_t const> public_key_bytes) const noexcept;
};

}  // namespace webcpp::crypto::curves::secp256k1
```

  The three share one private member template, `template <std::size_t N> result<secret<N>>
  agree(std::span<std::uint8_t const> public_key_bytes, secp256k1_ecdh_hash_function
  hash_function) const noexcept`, which is version 0.1's `ecdh` body writing into
  `secret<N>::writable()`: the guard, the parse, `read_canary`, the secret taken out with
  `secp256k1_keypair_sec` into a `detail::secp256k1_secret` (wiped on every path), and
  `backend_failure` when `secp256k1_ecdh` returns 0. `ecdh` passes `nullptr` (libsecp256k1's
  SHA-256), `ecdh_x` and `ecdh_point` the two functions above. The header gains `#include
  <webcpp/crypto/secret.hpp>` and `<cstring>`.

- [ ] **Step 1: The failing test** `test/secp256k1_ecdh_test.cpp` (the Jamfile line
  `crypto-run secp256k1_ecdh : secp256k1_ecdh_test.cpp : <library>/third_party//secp256k1
  <library>/third_party//wycheproof <library>/webcpp//boost_json ;`):
  - agreement: 32 key pairs from a seeded `std::mt19937_64` (secrets refused by `make` drawn
    again), `a.ecdh_x(B)` and `b.ecdh_x(A)` hold the same bytes, and so do `a.ecdh_point(B)` and
    `b.ecdh_point(A)` (secrets are compared as `std::ranges::equal(left->bytes(),
    right->bytes())`, since `secret<N>` has no `operator==`), and `ecdh_point`'s bytes 1 to 32
    are `ecdh_x`'s; the point parses with
    `secp256k1_ec_pubkey_parse` and is on the curve;
  - a known answer: the secrets `0x11` repeated 32 times and `0x22` repeated 32 times give the
    x coordinate `77e0510d5042e2f5e9e59c977b81eeed590cf7d20c1c51da451a8eaa9fdc45ff`, and the
    first key's compressed public key is
    `034f355bdcb7cc0af728ef3cceb9615d90684bb5b2ca5f859ab0f0b704075871aa` (both computed
    independently of libsecp256k1, in affine arithmetic over the curve's field, when this
    plan was written);
  - `ecdh` keeps version 0.1's value: the SHA-256 (Boost.Hash2) of the compressed form of
    `ecdh_point`'s point equals `ecdh`;
  - Wycheproof's `testvectors_v1/ecdh_secp256k1_test.json` (ASN.1 public keys) and
    `ecdh_secp256k1_webcrypto_test.json` (JWK), read as `secp256k1_wycheproof_test.cpp` reads
    them today (the same two SPKI prefixes, `3056301006072a8648ce3d020106052b8104000a034200`
    and `3036301006072a8648ce3d020106052b8104000a032200`, the same skips): every valid case's
    `shared` equals `ecdh_x`, every invalid case is refused with `invalid_public_key` (or
    `invalid_secret_key` from `make` for its scalar), and the counts stay version 0.1's
    (ASN.1: 752 cases, 496 taken, 473 valid, 21 invalid, 2 acceptable of which 1 taken; the
    JWK file's counts as its test asserts them today). The ECDH functions of
    `secp256k1_wycheproof_test.cpp` move here; it keeps the ECDSA half;
  - the guards: a moved-from key pair gives `invalid_state` from all three; 32, 34 and 66
    bytes, an off-curve point and a hybrid prefix (06, 07) give `invalid_public_key`;
  - the type: `static_assert(std::is_same_v<decltype(key.ecdh(peer)), boost::system::result<webcpp::crypto::secret<32>>>)`
    and the same for `ecdh_x` (32) and `ecdh_point` (65).

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_ecdh`: expected a
  compile error (no `ecdh_x`).
- [ ] **Step 2: Implement** the interfaces above. `test/secp256k1_test.cpp`,
  `secp256k1_allocation_test.cpp`, `support/secp256k1_checks.hpp` and
  `example/ecdh_session.cpp` follow `ecdh`'s new type: `session_cipher` takes
  `crypto::secret<32> const&` and passes `shared.bytes()`; its two `wipe` calls on the
  shared secrets go (the secret wipes itself), and its comment says so. The allocation test
  adds `ecdh_x` and `ecdh_point`.
- [ ] **Step 3: The example** `example/ecdh_x_coordinate.cpp` (`webcpp.example
  ecdh_x_coordinate.cpp : <library>/third_party//secp256k1 ;`): two key pairs from the fixed
  secrets above (the comment says real keys come from `random_device`, as `wallet_key.cpp`
  draws them), each side's `ecdh_x` with the other's public key, `equal` on the two, then
  what a Nostr library (webcpp::nostr, design D24) would derive from it: NIP-44's conversation
  key, HKDF-extract with the salt `nip44-v2` (Boost.Hash2's `hmac_sha2_256`, written in the
  example as `ecdh_session.cpp` writes its HKDF). Its `.expected`:

```
shared x coordinates match
x: 77e0510d5042e2f5e9e59c977b81eeed590cf7d20c1c51da451a8eaa9fdc45ff
conversation key (what a NIP-44 library derives): 2cbdf074f601178c24da3f829b504507a1f550f97d472af0f3f2cc59ab7757d1
```

  (the conversation key is HMAC-SHA-256 keyed with `nip44-v2` over that x, computed with
  Python's `hmac` when this plan was written).
- [ ] **Step 4: The rows and the documentation.** `crypto.json`: the `keypair` operation's
  `functions` gain `ecdh_x` and `ecdh_point`, its `tests` gain `secp256k1_ecdh`, its `what`
  becomes "A secp256k1 key pair: BIP 340 Schnorr and ECDSA signatures, ECDH in three forms
  and the x-only tweak."; the `use_cases` row "Agree on a key with a peer through a relay
  that sees everything" keeps `ecdh` and its text loses "not NIP-44's raw x coordinate"; a
  new row `{"need": "Derive a Nostr direct message's conversation key (NIP-44; the format is a
  domain library's)", "namespace": "webcpp::crypto::curves::secp256k1", "api": "`keypair::ecdh_x`, then
  HKDF on Boost.Hash2", "backend": "libsecp256k1", "jamfile": "/webcpp/crypto//crypto
  /third_party//secp256k1", "example": "ecdh_x_coordinate", "section": "crypto-secp256k1-ecdh"}`;
  the `curves` namespace row's "Choose it for compatibility with" loses "ECDH in the raw
  x-coordinate form of Nostr's encrypted messages ({nip44})" from its "Not in this version"
  list. `doc/secp256k1.adoc` gains the section `[#crypto-secp256k1-ecdh]` "Three forms of
  ECDH": what a shared secret between two key pairs is for (a chat app's two users who have
  never met), the three outputs and who takes each (libsecp256k1's hashed default; the raw x
  of Wycheproof and NIP-44 ({nip44}); the whole point), that each is a `secret<N>` that wipes
  itself, the example, and the pitfalls (the raw x is not a key: a protocol's KDF makes keys
  of it; two programs must agree on the form, or they derive different keys and fail
  silently; never reuse one shared secret as two keys). The `ecdh` paragraph of the page's
  opening list says the hashed form is one of three. `changelog.adoc`:
  `* Changed curves::secp256k1::keypair::ecdh to return secret<32>, which wipes itself (BREAKING: a caller that kept std::array<std::uint8_t, 32> reads .bytes()).`
  and `* Added keypair::ecdh_x and ecdh_point: the shared point's x coordinate and the point itself, as secrets.`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output). Commit
  `crypto: secp256k1's ECDH returns secret<32>, and gives the raw x coordinate and the shared point`.

## Task 2.2: Recovery, hedged ECDSA, and the parsed public key in both forms

**As built.** As written, but the changelog: its lines are merged into the `curves::secp256k1`
line ("hedged and recoverable ECDSA").

**Files:**
- Modify: `include/webcpp/crypto/curves/secp256k1.hpp`, `include/webcpp/crypto/backends/secp256k1.hpp`
  (`#include <secp256k1_recovery.h>`), `test/secp256k1_c_api_test.cpp` (its comment: the C
  API stays reachable beside the wrappers; it keeps calling the C functions, and now also
  compares with the wrappers), `test/secp256k1_allocation_test.cpp`,
  `test/support/secp256k1_checks.hpp`, `test/Jamfile` (the new programs, and the comment on
  `secp256k1_c_api`, which no longer says "which webcpp does not wrap"), `example/Jamfile`,
  `doc/secp256k1.adoc`, `doc/tables/crypto.json`, `doc/changelog.adoc`.
- Create: `test/secp256k1_recovery_test.cpp`, `test/secp256k1_keys_test.cpp`,
  `example/recover_signer.cpp`, `example/recover_signer.expected`.

**Interfaces:**

```cpp
namespace webcpp::crypto::curves::secp256k1 {

/** The two SEC1 encodings of a point. */
enum class point_form {
    /** 33 bytes: 02 or 03 by the parity of y, then x. Bitcoin's form. */
    compressed,
    /** 65 bytes: 04, x, y. Ethereum's form, and OpenSSL's default. */
    uncompressed,
};

/** The size of a point in `Form`: 33 or 65. */
template <point_form Form>
inline constexpr std::size_t point_size = Form == point_form::compressed ? 33 : 65;

/** The size of a recoverable signature: r, s, then the recovery id. */
inline constexpr std::size_t recoverable_signature_size = 65;

/**
 * A secp256k1 public key, parsed: a point on the curve, never the identity
 * (libsecp256k1's `secp256k1_pubkey`). It holds nothing secret and is copied freely; it is
 * made only by the functions that return one, so that it is always valid.
 */
class public_key {
public:
    /** Equal when the points are equal. */
    [[nodiscard]] friend bool operator==(public_key const& left_key,
                                         public_key const& right_key) noexcept;
    /** Ordered as their compressed encodings, BIP 327's KeySort (`secp256k1_ec_pubkey_cmp`). */
    [[nodiscard]] friend std::strong_ordering operator<=>(public_key const& left_key,
                                                          public_key const& right_key) noexcept;

private:
    friend struct webcpp::crypto::detail::secp256k1_access;
    public_key() noexcept = default;
    secp256k1_pubkey point_value_{};
};

/**
 * Parses a public key: 33 bytes with prefix 02 or 03, or 65 with prefix 04 (X9.62's hybrid
 * prefixes 06 and 07 are refused, as everywhere in crypto).
 *
 * @return The key; `invalid_public_key` for any other length or prefix, or a point off the curve.
 */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> parse_public_key(
    std::span<std::uint8_t const> encoded_key) noexcept;

/**
 * Serialises a public key in `Form`.
 *
 * @return 33 bytes (`compressed`, the default) or 65 (`uncompressed`).
 */
template <point_form Form = point_form::compressed, class Backend = backends::secp256k1>
[[nodiscard]] std::array<std::uint8_t, point_size<Form>> serialize_public_key(
    public_key const& parsed_key) noexcept;

/**
 * The public key that made a recoverable signature of a 32-byte hash
 * (`secp256k1_ecdsa_recover`), as Ethereum identifies a transaction's signer.
 *
 * @param recoverable_signature 65 bytes: r, s, then the recovery id, 0 to 3.
 * @param message_hash The 32-byte hash signed.
 * @return The key; `invalid_signature` for a recovery id above 3, an r or an s not below the
 * group order, or a signature from which no key is recovered.
 */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> recover(
    std::span<std::uint8_t const, recoverable_signature_size> recoverable_signature,
    std::span<std::uint8_t const, 32> message_hash) noexcept;

template <class Backend>
class basic_keypair {
public:
    /**
     * The public key, SEC1 uncompressed.
     *
     * @return 65 bytes; `invalid_state` moved from.
     */
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, 65>> public_key_uncompressed()
        const noexcept;

    /**
     * An ECDSA signature of a 32-byte hash with its recovery id: RFC 6979's nonce, low-S, as
     * `sign_ecdsa` (the same r and s), then the id from which `recover` finds the key.
     *
     * @return 65 bytes, r, s and the id 0 to 3; `invalid_state`; `backend_failure`.
     */
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, recoverable_signature_size>>
    sign_recoverable(std::span<std::uint8_t const, 32> message_hash) const noexcept;

    /**
     * A hedged ECDSA signature: RFC 6979's nonce with 32 bytes of the caller's fresh
     * randomness mixed in (libsecp256k1's `ndata`), so that a fault or a side channel on one
     * signing does not repeat on the next.
     *
     * @param extra_entropy 32 bytes from `random_device`, new for each signature.
     * @return 64 bytes, r then s, low-S; `invalid_state`; `backend_failure`.
     */
    [[nodiscard]] boost::system::result<std::array<std::uint8_t, 64>> sign_ecdsa(
        std::span<std::uint8_t const, 32> message_hash,
        std::span<std::uint8_t const, 32> extra_entropy) const noexcept;

private:
    friend struct webcpp::crypto::detail::secp256k1_access;
};

}  // namespace webcpp::crypto::curves::secp256k1

namespace webcpp::crypto::detail {

/**
 * The one door of the module headers (`curves/secp256k1/*.hpp`) into a key pair and a public
 * key: the key pair's context and `secp256k1_keypair`, whether it is keyed, and a public key's
 * point. It hands out nothing a caller of crypto could not already do through the key pair.
 */
struct secp256k1_access {
    template <class Backend>
    [[nodiscard]] static bool keyed(curves::secp256k1::basic_keypair<Backend> const& key_pair) noexcept;
    template <class Backend>
    [[nodiscard]] static secp256k1_context const* context(
        curves::secp256k1::basic_keypair<Backend> const& key_pair) noexcept;
    template <class Backend>
    [[nodiscard]] static secp256k1_keypair const& pair(
        curves::secp256k1::basic_keypair<Backend> const& key_pair) noexcept;
    [[nodiscard]] static secp256k1_pubkey const& point(curves::secp256k1::public_key const& key) noexcept;
    [[nodiscard]] static secp256k1_pubkey& point(curves::secp256k1::public_key& key) noexcept;
    [[nodiscard]] static curves::secp256k1::public_key make_public_key() noexcept;
};

}  // namespace webcpp::crypto::detail
```

  `public_key`'s `==` and `<=>` call `secp256k1_ec_pubkey_cmp` with the static context;
  `recover` parses with `secp256k1_ecdsa_recoverable_signature_parse_compact` (id above 3
  refused before the call, since libsecp256k1 checks it with `ARG_CHECK`); `sign_recoverable`
  calls `secp256k1_ecdsa_sign_recoverable` with the key pair's context and the secret taken out
  as `sign_ecdsa` takes it; the hedged `sign_ecdsa` copies `extra_entropy` into a
  `detail::secp256k1_secret` and passes it as `ndata` with `nullptr` (RFC 6979) as the nonce
  function. `detail::secp256k1_access` is declared before `public_key`, which names it as a
  friend. Inside `basic_keypair` the member function `public_key()` hides the class of the same
  name, so a member signature that names the type writes `secp256k1::public_key`.

- [ ] **Step 1: The failing tests.** `test/secp256k1_keys_test.cpp` (`crypto-run secp256k1_keys
  : secp256k1_keys_test.cpp : <library>/third_party//secp256k1 ;`): the generator and its
  multiples parse and serialise, 1G =
  `0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798`, 2G =
  `02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5`, 3G =
  `02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9` (from the key pairs of
  the secrets 1, 2 and 3), and each compressed form re-parsed and serialised uncompressed then
  compressed gives its bytes back; a key pair's `public_key_uncompressed()` parses to the same
  `public_key` as its `public_key()`; refusals with `invalid_public_key`: lengths 0, 32, 34,
  64, 66, prefixes 00, 01, 05, 06, 07 (06 and 07 with a valid point, which libsecp256k1 alone
  would take), x not below the field's prime, x with no y; `<=>` orders 1G, 2G, 3G as their
  compressed bytes order them, and `==` holds between a key and its re-parsed copy;
  `static_assert(!std::is_default_constructible_v<secp256k1::public_key>)`.
  `test/secp256k1_c_api_test.cpp` gains, failing first, the comparison of the key its C calls
  recover with `recover`'s and of its ElligatorSwift decoding with `parse_public_key`'s.
  `test/secp256k1_recovery_test.cpp` (`crypto-run secp256k1_recovery :
  secp256k1_recovery_test.cpp : <library>/third_party//secp256k1 ;`):
  - 64 seeded key pairs and hashes: `recover(sign_recoverable(h), h)` is the key pair's key;
    the id is 0 to 3; `sign_recoverable`'s first 64 bytes equal `sign_ecdsa(h)`; recovering
    with another hash gives another key or `invalid_signature`; an id of 4 and of 255 is
    `invalid_signature` (and no abort);
  - libsecp256k1's own recovery cases (`src/modules/recovery/tests_impl.h`,
    `test_ecdsa_recovery_edge_cases`, MIT, its bytes written into the test with that file
    named): the 64-byte signature made with secret key 0 recovers a key with id 1 and fails
    with ids 0, 2 and 3; (r, s) = (4, 4) recovers a key with each of the ids 0 to 3, and
    `verify_ecdsa` accepts (4, 4) under the key recovered with each id; (1, 1) recovers with id
    0 and verifies; r = 0 or s = 0 is `invalid_signature`;
  - the hedged form: two signatures of one hash with different `extra_entropy` differ, both
    verify with `verify_ecdsa`, and each equals libsecp256k1's `secp256k1_ecdsa_sign` called
    with the same `ndata` through the C API (the hedging is deterministic given its input);
    with 32 zero bytes of entropy it differs from the unhedged signature (libsecp256k1 mixes
    `ndata` even when zero);
  - a moved-from key pair: `invalid_state` from `sign_recoverable`, the hedged `sign_ecdsa`
    and `public_key_uncompressed`.

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_keys
  libs/crypto/test//secp256k1_recovery`: expected compile errors.
- [ ] **Step 2: Implement** the interfaces; the allocation test and `expect_empty_keypair`
  gain the new members.
- [ ] **Step 3: The example** `example/recover_signer.cpp` (`webcpp.example recover_signer.cpp
  : <library>/third_party//secp256k1 ;`): a wallet signs the hash of a message (Boost.Hash2's
  SHA-256 of `"pay 10 to bob"`, the comment saying that Ethereum's own message hash is
  EIP-191's, a domain library's) with `sign_recoverable`; a reader that holds no key recovers
  the signer's key from the signature and the hash and compares it with the wallet's
  published key, then sees a changed message recover another key. Its `.expected`:

```
recovered the wallet's key
a changed message names another signer
```

- [ ] **Step 4: The rows and the documentation.** `crypto.json`: `keypair`'s `functions` gain
  `public_key_uncompressed` and `sign_recoverable`, its `tests` `secp256k1_recovery`; new
  operations `{"id": "recovery", "free": true, "functions": ["recover"], "namespace": "curves",
  "api": "curves::secp256k1::recover", "header": "<webcpp/crypto/curves/secp256k1.hpp>",
  "what": "The public key that made a recoverable ECDSA signature of a hash.", "section":
  "crypto-secp256k1-recovery", "backends": {"secp256k1": {"tests": ["secp256k1_recovery"]}}}`
  and `{"id": "public_key", "free": true, "functions": ["parse_public_key",
  "serialize_public_key"], "namespace": "curves", "api": "curves::secp256k1::public_key,
  parse_public_key, serialize_public_key", "header": "<webcpp/crypto/curves/secp256k1.hpp>",
  "what": "A parsed public key, read from and written to its compressed or uncompressed form.",
  "section": "crypto-secp256k1-keys", "backends": {"secp256k1": {"tests":
  ["secp256k1_keys"]}}}`; the `use_cases` row `{"need": "Find who signed a message or a
  transaction without being sent their key (Ethereum's signatures)", "api":
  "`keypair::sign_recoverable`, `recover`", "example": "recover_signer", "section":
  "crypto-secp256k1-recovery", ...}`; the `curves` row's compatibility text loses "Ethereum's
  recoverable signatures and its addresses from the uncompressed key" from "Not in this
  version" and gains "Ethereum's recoverable signatures and uncompressed keys ({ethereum}),
  the address hashing being a domain library's"; its "Key sizes" cell gains "public 65 bytes
  uncompressed" and its "Signature sizes" cell "65 bytes recoverable". New `sources`: `sec1` (SEC 1 version 2,
  section 2.3.3 and 2.3.4, `https://www.secg.org/sec1-v2.pdf`). `doc/secp256k1.adoc` gains
  `[#crypto-secp256k1-keys]` "Public keys and their two forms" (what a public key is and why a
  program parses it once: a key off the curve is how invalid-curve attacks start; the two
  forms and which ecosystem writes which ({sec1}); the hybrid prefixes refused and why) and
  `[#crypto-secp256k1-recovery]` "Recoverable signatures" (Ethereum names an account by its
  key's hash and sends no key with a transaction ({ethereum}): the signature carries the id
  that finds it; the example; pitfalls: recovery always yields some key, so compare it with
  the expected one, never trust it alone; Ethereum's `v` is the id plus 27 or a chain's offset,
  a domain library's encoding; the hedged form, when to use it). `changelog.adoc`:
  `* Added curves::secp256k1::public_key with parse_public_key and serialize_public_key in both forms, and keypair::public_key_uncompressed.`,
  `* Added keypair::sign_recoverable and recover, Ethereum's recoverable ECDSA signatures.` and
  `* Added the hedged keypair::sign_ecdsa, RFC 6979 with the caller's extra entropy.`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output). Commit
  `crypto: secp256k1's recoverable signatures, hedged ECDSA and the parsed public key in both forms`.

## Task 2.3: Tweaks, negation, combination; SPKI, PKCS#8 and PEM for secp256k1

**As built.** In two task commits: the tweaks with SPKI and PKCS#8 first, and PEM after stream A's
Task 7b brought `pem.hpp` (`crypto: secp256k1's keys in PEM`). The example is
`example/secp256k1_key_formats.cpp` with its `.expected`, since stream A's
`example/key_formats.cpp` is Ed25519's and X25519's; it links
`/webcpp/crypto//pem-default` for PEM, as stream A's example does, and the use case
"Store or exchange a secp256k1 key" names it. A compressed public key inside RFC 5915's `[1]`
makes a 102-byte PKCS#8, not 103 as Step 1 says (`secp256k1_formats_test.cpp` asserts 102 for
OpenSSL's encoding).

**Files:**
- Create: `include/webcpp/crypto/curves/secp256k1/tweak.hpp`,
  `include/webcpp/crypto/curves/secp256k1/formats.hpp`, `test/secp256k1_tweak_test.cpp`,
  `test/secp256k1_formats_test.cpp`, `example/key_tweak.cpp`, `example/key_tweak.expected`,
  `example/key_formats.cpp`, `example/key_formats.expected`.
- Modify: `include/webcpp/crypto/curves/secp256k1.hpp` (`workspace` and
  `detail::secp256k1_workspace`), `include/webcpp/crypto.hpp`, `test/Jamfile`,
  `test/secp256k1_allocation_test.cpp`, `test/windows_h_before_test.cpp`, `test/windows_h_after_test.cpp`, `example/Jamfile`,
  `doc/secp256k1.adoc`, `doc/tables/crypto.json`, `doc/changelog.adoc`.

**Interfaces** (`workspace` in `curves/secp256k1.hpp`, the rest in `curves/secp256k1/tweak.hpp`;
namespace `webcpp::crypto::curves::secp256k1`):

```cpp
/**
 * Memory a caller lends to a function over many keys, nonces or outputs: libsecp256k1 takes
 * arrays of pointers, which webcpp builds here and allocates nowhere. Each such function has a
 * `*_workspace_size` that says how many bytes it needs; the bytes are wiped before it returns.
 *
 * Tip: `alignas(std::max_align_t) std::array<std::byte, N>` on the stack, or a vector the
 * program owns; a short workspace is `invalid_input_size`, never an allocation.
 */
using workspace = std::span<std::byte>;

/** The workspace `pubkey_combine` needs for `key_count` keys. */
[[nodiscard]] constexpr std::size_t combine_workspace_size(std::size_t key_count) noexcept {
    return (key_count * sizeof(secp256k1_pubkey const*)) + alignof(std::max_align_t);
}

/** `secret_key` + `tweak_bytes` modulo the group order (`secp256k1_ec_seckey_tweak_add`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<secret<32>> seckey_tweak_add(
    std::span<std::uint8_t const, 32> secret_key, std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
/** `secret_key` * `tweak_bytes` modulo the group order (`secp256k1_ec_seckey_tweak_mul`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<secret<32>> seckey_tweak_mul(
    std::span<std::uint8_t const, 32> secret_key, std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
/** The group order minus `secret_key` (`secp256k1_ec_seckey_negate`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<secret<32>> seckey_negate(
    std::span<std::uint8_t const, 32> secret_key) noexcept;
/** P + t times G (`secp256k1_ec_pubkey_tweak_add`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> pubkey_tweak_add(
    public_key const& parsed_key, std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
/** t times P (`secp256k1_ec_pubkey_tweak_mul`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> pubkey_tweak_mul(
    public_key const& parsed_key, std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
/** -P (`secp256k1_ec_pubkey_negate`), which never fails. */
template <class Backend = backends::secp256k1>
[[nodiscard]] public_key pubkey_negate(public_key const& parsed_key) noexcept;
/** The sum of the keys (`secp256k1_ec_pubkey_combine`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> pubkey_combine(
    std::span<public_key const> parsed_keys, workspace pointer_space) noexcept;
```

  Errors: a secret key zero or not below the order is `invalid_secret_key`, checked with
  `secp256k1_ec_seckey_verify` before the call (libsecp256k1 would return 0 for both, and the
  caller should know which input was wrong); a tweak not below the order, a product or sum
  that is zero or the point at infinity, and a zero multiplier are `invalid_secret_key`;
  `pubkey_combine` refuses no keys and a short workspace with `invalid_input_size` (zero keys
  is an `ARG_CHECK`) and a sum at infinity with `invalid_public_key`. A secret result is
  computed in a `detail::secp256k1_secret` copy of the input and moved into the `secret<32>`;
  on a refusal the copy is wiped and nothing is returned. The workspace is carved by
  `detail::secp256k1_workspace` (in `curves/secp256k1.hpp`, beside version 0.1's
  `detail::secp256k1_secret`; never in a `detail/` header, which `meta/include-boundaries.json`
  keeps free of libsecp256k1), which `std::align`s each array it hands out,
  answers `nullptr` when the span is short, and wipes every byte it handed out in its
  destructor; Tasks 2.5 and 2.6 use the same carver.

**Interfaces** (`curves/secp256k1/formats.hpp`; design D30):

```cpp
namespace webcpp::crypto::detail {

/** RFC 5480's SPKI before an uncompressed secp256k1 point: SEQUENCE, AlgorithmIdentifier
 * (id-ecPublicKey, secp256k1), BIT STRING of 66 bytes. */
inline constexpr std::array<std::uint8_t, 23> secp256k1_spki_uncompressed_prefix{
    0x30, 0x56, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01,
    0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x0a, 0x03, 0x42, 0x00};
/** The same before a compressed point (BIT STRING of 34 bytes). */
inline constexpr std::array<std::uint8_t, 23> secp256k1_spki_compressed_prefix{
    0x30, 0x36, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01,
    0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x0a, 0x03, 0x22, 0x00};
/** PKCS#8 (version 0, OpenSSL's form) before the secret: OneAsymmetricKey, the
 * AlgorithmIdentifier, an OCTET STRING holding RFC 5915's ECPrivateKey (version 1, its
 * 32-byte OCTET STRING), with no [0] parameters. */
inline constexpr std::array<std::uint8_t, 33> secp256k1_pkcs8_prefix{
    0x30, 0x81, 0x84, 0x02, 0x01, 0x00, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x02, 0x01, 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x0a, 0x04, 0x6d, 0x30, 0x6b, 0x02, 0x01,
    0x01, 0x04, 0x20};
/** Between the secret and the uncompressed public point: ECPrivateKey's [1] publicKey BIT STRING. */
inline constexpr std::array<std::uint8_t, 5> secp256k1_pkcs8_infix{0xa1, 0x44, 0x03, 0x42, 0x00};

}  // namespace webcpp::crypto::detail

namespace webcpp::crypto::curves::secp256k1 {

/** The SPKI's size with a compressed point, 56 bytes, and with an uncompressed one, 88. */
inline constexpr std::size_t spki_compressed_size = 56;
inline constexpr std::size_t spki_uncompressed_size = 88;
/** The longest SPKI, 88 bytes. */
inline constexpr std::size_t max_spki_size = spki_uncompressed_size;
/** The PKCS#8's size, 135 bytes: the secret and the uncompressed public point. */
inline constexpr std::size_t max_pkcs8_size = 135;

/** Writes a public key's SPKI (RFC 5480, the secp256k1 OID) into `output_bytes`, at least
 * `max_spki_size` bytes; returns its length. Nothing is written on an error. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<std::size_t> encode_spki(
    public_key const& parsed_key, std::span<std::uint8_t> output_bytes,
    point_form encoded_form = point_form::compressed) noexcept;
/** The public key of an SPKI in exactly one of the two forms `encode_spki` writes;
 * `invalid_encoding` for anything else, a point off the curve included (the spec's "Key
 * formats": every refusal of a decoder is `invalid_encoding`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> decode_spki(
    std::span<std::uint8_t const> spki_bytes) noexcept;
/** Writes a key pair's PKCS#8 (OpenSSL's form: version 0, ECPrivateKey version 1 with the
 * uncompressed public key, no parameters inside) into `output_bytes`, at least
 * `max_pkcs8_size` bytes; returns its length. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<std::size_t> encode_pkcs8(
    basic_keypair<Backend> const& key_pair, std::span<std::uint8_t> output_bytes) noexcept;
/** The key pair of a PKCS#8 that is exactly what `encode_pkcs8` writes for its secret, the
 * public key included; made as `basic_keypair::make` makes it. `invalid_encoding` for any
 * other bytes, a secret that is zero or not below the order included; `entropy_unavailable`
 * and `backend_failure` as `make` reports them. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<basic_keypair<Backend>> decode_pkcs8(
    std::span<std::uint8_t const> pkcs8_bytes,
    std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

}  // namespace webcpp::crypto::curves::secp256k1
```

  The encoders copy the constants and the key bytes; the decoders compare the prefix with
  `equal`, check the length, parse the point, and `decode_pkcs8` makes the key pair from the
  secret, encodes it again and compares the whole input with `equal` (design D30's
  re-encoding rule, which refuses a PKCS#8 whose public key is not its secret's). The
  secret's 32 bytes are copied through a `detail::secp256k1_secret` on both sides. PEM is
  Task 7b's `pem::armour` and `pem::unarmour` (`pem.hpp`, on libsodium's constant-time base64)
  with the labels `PUBLIC KEY` and `PRIVATE KEY`: a program that writes PEM links
  `/third_party//sodium` beside `/third_party//secp256k1`, which the page says. WebCrypto has
  no secp256k1 (the `curves` row says so, citing {webcrypto}), so the outside oracles are
  OpenSSL natively and on WASI and node's own `crypto` module under Emscripten, as spec.md's
  "Key formats" and design D30 now say.

- [ ] **Step 1: The failing tests.** `test/secp256k1_tweak_test.cpp` (`crypto-run
  secp256k1_tweak : secp256k1_tweak_test.cpp : <library>/third_party//secp256k1 ;`), over 64
  seeded secrets d and tweaks t: `pubkey_tweak_add(P(d), t) == P(seckey_tweak_add(d, t))`,
  `pubkey_tweak_mul(P(d), t) == P(seckey_tweak_mul(d, t))`, `P(seckey_negate(d)) ==
  pubkey_negate(P(d))`, `seckey_negate(seckey_negate(d))` holds d's bytes (`std::ranges::equal`
  on `bytes()`),
  `pubkey_combine({P(d), P(t)}) == P(seckey_tweak_add(d, t))`, and each against the C API
  called directly; the known answer `seckey_tweak_add(1, 1)` gives a key whose public key is
  2G above; the refusals: a tweak equal to the order
  (`fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141`) is
  `invalid_secret_key`, so are `seckey_tweak_add(d, n - d)`, `seckey_tweak_mul(d, 0)`, a zero
  and an order-sized secret, `pubkey_combine({P, -P})` is `invalid_public_key`, no keys and a
  workspace one byte short are `invalid_input_size`; the workspace is zero after each call
  (read through the span). `test/secp256k1_formats_test.cpp`
  (`crypto-run secp256k1_formats : secp256k1_formats_test.cpp : <library>/third_party//secp256k1
  <library>/third_party//sodium <library>/third_party//wycheproof <library>/webcpp//boost_json ;`,
  and natively, on wasip2 and wasip3, `crypto-run secp256k1_formats_openssl :
  secp256k1_formats_test.cpp : <library>/third_party//secp256k1 <library>/third_party//sodium
  <library>/webcpp/crypto//openssl <define>WEBCPP_TEST_OPENSSL_ORACLE : native wasip2 wasip3 ;`):
  - round trips of SPKI in both forms and of PKCS#8 for 64 seeded key pairs, and PEM
    armoured and unarmoured around each;
  - Wycheproof's `ecdh_secp256k1_test.json`, all 752 `public` fields through `decode_spki`:
    the 496 that version 0.1's test takes decode exactly where version 0.1's
    `detail::secp256k1_parse_public_key` (which refuses the hybrid prefixes) parses their
    point (`invalid_encoding` otherwise), and the other 256 are refused with
    `invalid_encoding`, the counts printed and asserted;
  - malformed inputs, each `invalid_encoding` with the output untouched: P-256's OID
    (`06 08 2a 86 48 ce 3d 03 01 07`) in place of secp256k1's, a trailing byte, a length in
    long form (`30 81 56 ...`), a compressed point under the uncompressed prefix, the hybrid
    prefix 06, a PKCS#8 without the public key (OpenSSL's `-nopubkey` form), one with `[0]`
    parameters inside the ECPrivateKey, one of version 1 (OneAsymmetricKey v2), one whose
    public key is another key's, a PEM label other than the expected, a line of 63
    characters, a carriage return, a PKCS#8 whose secret is zero or the group order;
  - with `WEBCPP_TEST_OPENSSL_ORACLE`: 64 keys made by OpenSSL
    (`EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "secp256k1")`); for each, first its PKCS#8
    from `OSSL_ENCODER_CTX_new_for_pkey(key, EVP_PKEY_KEYPAIR, "DER", "PrivateKeyInfo",
    nullptr)` and its uncompressed SPKI from `i2d_PUBKEY`, both while the key keeps OpenSSL's
    default uncompressed form (OpenSSL writes the key's current form into ECPrivateKey's
    `[1]`, and a compressed one there is a 103-byte PKCS#8 webcpp rightly refuses), then the
    compressed SPKI after
    `EVP_PKEY_set_utf8_string_param(key, OSSL_PKEY_PARAM_EC_POINT_CONVERSION_FORMAT, "compressed")`: webcpp decodes each and encodes the same bytes; webcpp's
    encodings decode in OpenSSL (`d2i_PUBKEY`, `d2i_AutoPrivateKey`) to the same key; and
    `PEM_write_bio_PUBKEY`'s text equals `pem::armour("PUBLIC KEY", ...)`'s; OpenSSL's error
    queue is left empty;
  - on Emscripten under node: the same round trip through node's `crypto` module, reached in
    an `EM_JS` function with `process.getBuiltinModule('node:crypto')` (node 26.7.0):
    `createPrivateKey({key, format: 'der', type: 'pkcs8'}).export({format: 'der', type:
    'pkcs8'})` and `createPublicKey(...).export({format: 'der', type: 'spki'})` give webcpp's
    bytes back for 64 keys.

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_tweak
  libs/crypto/test//secp256k1_formats`: expected compile errors.
- [ ] **Step 2: Implement** both headers; `formats.hpp` waits for Task 7b's `pem.hpp` only in
  its PEM test cases: if stream A's Task 7b is not yet on the branch, the DER half is
  committed first and the PEM cases join at the rebase (Branches, above).
- [ ] **Step 3: The examples.** `example/key_tweak.cpp` (`<library>/third_party//secp256k1`):
  a watch-only wallet adds a tweak to a public key while the signing device adds the same
  tweak to the secret key; the device's tweaked key pair signs (BIP 340), the watcher's
  tweaked public key verifies, and the comment says that BIP 32 derives such tweaks with
  HMAC-SHA-512 and is a domain library's ({bip32}). `.expected`:

```
tweaked keys match
signature verifies under the tweaked public key
```

  `example/key_formats.cpp` (`<library>/third_party//secp256k1 <library>/third_party//sodium`):
  a server's key pair from the fixed secret `0x11` repeated, its public key written as PEM
  SPKI for a config file and read back. `.expected`:

```
-----BEGIN PUBLIC KEY-----
MDYwEAYHKoZIzj0CAQYFK4EEAAoDIgADTzVb3LfMCvco7zzOuWFdkGhLtbLKX4Wa
sPC3BAdYcao=
-----END PUBLIC KEY-----
read back: same key
```

  (the base64 of `secp256k1_spki_compressed_prefix` followed by the compressed key of Task
  2.1's known answer, 56 bytes, computed with Python's `base64` when this plan was written).
- [ ] **Step 4: The rows and the documentation.** `crypto.json` operations `key_tweaks`
  (`seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `pubkey_tweak_add`,
  `pubkey_tweak_mul`, `pubkey_negate`, `pubkey_combine`; header
  `<webcpp/crypto/curves/secp256k1/tweak.hpp>`; tests `secp256k1_tweak`; section
  `crypto-secp256k1-tweaks`) and `key_formats` (`encode_spki`, `decode_spki`, `encode_pkcs8`,
  `decode_pkcs8`; header `<webcpp/crypto/curves/secp256k1/formats.hpp>`; tests
  `secp256k1_formats`, `secp256k1_formats_openssl`; section `crypto-secp256k1-formats`);
  `use_cases` rows "Derive child keys for a watch-only wallet (BIP 32's derivation is a domain
  library's)" (`seckey_tweak_add`, `pubkey_tweak_add`, example `key_tweak`) and "Store or
  exchange a secp256k1 key with OpenSSL or a config file" (`encode_spki`, `encode_pkcs8`,
  `pem::armour`, backend "libsecp256k1, libsodium for PEM", example `key_formats`); new
  `sources` `rfc5480`, `rfc5915`, `rfc5208` and `rfc7468`. `doc/secp256k1.adoc` gains
  `[#crypto-secp256k1-tweaks]` "Tweaks, negation and combination" (what a tweak is for: a
  wallet's many addresses from one key, a watch-only server that never holds a secret;
  BIP 32 as the domain library's use ({bip32}); pitfalls: a tweak derived from public data
  alone lets anyone who sees two related keys relate them; a tweaked secret is a secret, which
  is why it is returned as one; combination of attacker-chosen keys is not a multisignature,
  MuSig2 is (Task 2.5)) and `[#crypto-secp256k1-formats]` "Keys in files: SPKI, PKCS#8 and
  PEM" (what each holds; strictness, which producer's forms are accepted (OpenSSL's), and why
  a lenient parser is a risk ({rfc5915}); PEM needs libsodium's target). `changelog.adoc`:
  `* Added curves::secp256k1's tweaks, negation and combination of secret and public keys (curves/secp256k1/tweak.hpp).` and
  `* Added curves::secp256k1's SPKI and PKCS#8, strict, in both point forms (curves/secp256k1/formats.hpp).`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output). Commit
  `crypto: secp256k1's tweaks, negation and combination, and its SPKI, PKCS#8 and PEM`.

## Task 2.4: ElligatorSwift, with BIP 324's vectors

**Files:**
- Create: `include/webcpp/crypto/curves/secp256k1/ellswift.hpp`,
  `test/secp256k1_ellswift_test.cpp`, `example/ellswift_handshake.cpp`,
  `example/ellswift_handshake.expected`.
- Modify: `include/webcpp/crypto/backends/secp256k1.hpp` (`#include <secp256k1_ellswift.h>`),
  `include/webcpp/crypto.hpp`, `test/Jamfile`, `test/secp256k1_allocation_test.cpp`,
  `test/support/secp256k1_checks.hpp`, `test/windows_h_*_test.cpp`, `example/Jamfile`,
  `doc/secp256k1.adoc`, `doc/tables/crypto.json`, `doc/changelog.adoc`; superproject
  `third_party/Jamfile` (the `bips` target's `.bips-files` gain
  `libs/bips/bip-0324/ellswift_decode_test_vectors.csv` and
  `libs/bips/bip-0324/packet_encoding_test_vectors.csv`, and its comment names them),
  `tools/test/third_party_test.py` (a new case, `test_bips_target_files`, in its `CASES`
  list).

**Interfaces** (`curves/secp256k1/ellswift.hpp`, namespace `curves::secp256k1`):

```cpp
/** The size of an ElligatorSwift encoding: 64 bytes, indistinguishable from random. */
inline constexpr std::size_t ellswift_size = 64;

/** The two sides of `ellswift_xdh` (BIP 324's initiator and responder). */
enum class ellswift_party {
    /** The side whose encoding comes first in the hash: BIP 324's initiator. */
    initiator = 0,
    /** The other side. */
    responder = 1,
};

/** A public key's ElligatorSwift encoding (`secp256k1_ellswift_encode`).
 * @param randomness 32 fresh bytes from `random_device`: the encoding is one of many. */
template <class Backend = backends::secp256k1>
[[nodiscard]] std::array<std::uint8_t, ellswift_size> ellswift_encode(
    public_key const& parsed_key, std::span<std::uint8_t const, 32> randomness) noexcept;

/** The public key of any 64 bytes (`secp256k1_ellswift_decode`): every input decodes. */
template <class Backend = backends::secp256k1>
[[nodiscard]] public_key ellswift_decode(
    std::span<std::uint8_t const, ellswift_size> encoded_key) noexcept;

/** The encoding of a key pair's public key, made from its secret with the key pair's
 * context (`secp256k1_ellswift_create`), so that the public key is never computed apart.
 * @return 64 bytes; `invalid_state`; `backend_failure`. */
template <class Backend>
[[nodiscard]] boost::system::result<std::array<std::uint8_t, ellswift_size>> ellswift_create(
    basic_keypair<Backend> const& key_pair, std::span<std::uint8_t const, 32> randomness) noexcept;

/** The shared secret of an ElligatorSwift key exchange, hashed as BIP 324 hashes it
 * (`secp256k1_ellswift_xdh` with `secp256k1_ellswift_xdh_hash_function_bip324`).
 * @param initiator_encoding The initiator's 64 bytes, as sent.
 * @param responder_encoding The responder's 64 bytes, as sent.
 * @param our_party Which of the two this key pair is.
 * @return 32 secret bytes; `invalid_state`; `backend_failure`. */
template <class Backend>
[[nodiscard]] boost::system::result<secret<32>> ellswift_xdh(
    basic_keypair<Backend> const& key_pair,
    std::span<std::uint8_t const, ellswift_size> initiator_encoding,
    std::span<std::uint8_t const, ellswift_size> responder_encoding,
    ellswift_party our_party) noexcept;
```

  The prefix hash function (`secp256k1_ellswift_xdh_hash_function_prefix`) is not wrapped:
  only BIP 324's is required (spec), and the C API stays reachable for the other. A key pair's
  secret is taken out into a `detail::secp256k1_secret` for `ellswift_create` and
  `ellswift_xdh`, which name it `seckey32`.

- [ ] **Step 1: The failing test** `test/secp256k1_ellswift_test.cpp` (`crypto-run
  secp256k1_ellswift : secp256k1_ellswift_test.cpp : <library>/third_party//secp256k1
  <library>/third_party//bips ;`):
  - BIP 324's `bip-0324/ellswift_decode_test_vectors.csv` (76 rows: `ellswift,x,comment`):
    each encoding decodes to a key whose compressed form's x is `x`;
  - BIP 324's `bip-0324/packet_encoding_test_vectors.csv` (7 rows): from `in_priv_ours`,
    `in_ellswift_ours`, `in_ellswift_theirs` and `in_initiating`: `ellswift_decode` of ours
    and theirs gives keys whose x are `mid_x_ours` and `mid_x_theirs`, the key pair of
    `in_priv_ours` has `mid_x_ours`'s key, and `ellswift_xdh` with the two encodings in
    initiator-then-responder order and our party gives `mid_shared_secret`; the other columns
    (the session's keys, garbage terminators, ciphertexts) are BIP 324's protocol and are not
    read;
  - 64 seeded key pairs: `ellswift_decode(ellswift_create(k, r)) == parse(k.public_key())`,
    `ellswift_decode(ellswift_encode(P, r)) == P`, two encodings of one key with different
    randomness differ, and the initiator's and the responder's `ellswift_xdh` agree while a
    swapped party does not;
  - `xswiftec_inv_test_vectors.csv` is not read: its inverse map is internal to
    libsecp256k1, and `c-suites` runs libsecp256k1's own ElligatorSwift tests;
  - a moved-from key pair: `invalid_state` from `ellswift_create` and `ellswift_xdh`.

  The superproject's case `test_bips_target_files` (added to `CASES` in
  `tools/test/third_party_test.py`, which has only `test_bips_is_pinned` for this target
  today) reads `third_party/Jamfile`, takes the `.bips-files` glob's arguments and asserts
  they are exactly `libs/bips/bip-0340/test-vectors.csv`,
  `libs/bips/bip-0341/wallet-test-vectors.json` and the two BIP 324 CSV files (Tasks 2.5 and
  2.6 extend the expected list).

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_ellswift`: expected a
  compile error; `python3 tools/test/third_party_test.py test_bips_target_files`: expected
  that case to fail, naming the two missing files.
- [ ] **Step 2: Implement**; the superproject's `bips` target names the two CSV files.
- [ ] **Step 3: The example** `example/ellswift_handshake.cpp` (`<library>/third_party//secp256k1`):
  two peers, from the fixed secrets `0x11` and `0x22` repeated 32 times and the fixed
  randomness `0x33` and `0x44` repeated 32 times (the comment says a real program draws all
  four from `random_device`), each send `ellswift_create`'s 64 bytes; each message decodes to
  its sender's public key, and the two `ellswift_xdh` agree. `.expected`:

```
each 64-byte message decodes to its sender's key
initiator and responder agree
```

  (nothing random is printed or asserted; that an encoding carries no recognisable prefix,
  unlike a compressed key's 02 or 03, is said in the prose, citing {bip324}, and that
  ElligatorSwift hides that a key exchange happens, not who takes part).
- [ ] **Step 4: The rows and the documentation.** `crypto.json` operation `ellswift`
  (`ellswift_encode`, `ellswift_decode`, `ellswift_create`, `ellswift_xdh`; header
  `<webcpp/crypto/curves/secp256k1/ellswift.hpp>`; tests `secp256k1_ellswift`; section
  `crypto-secp256k1-ellswift`); the `use_cases` row "Make a peer-to-peer connection's key
  exchange look like random bytes (BIP 324's transport; its packets are a domain library's)"
  (example `ellswift_handshake`); new `sources` `bip324` (BIP 324,
  `https://github.com/bitcoin/bips/blob/master/bip-0324.mediawiki`) and `elligatorswift`
  (Chavez-Saab, Rodríguez-Henríquez and Tibouchi, "SwiftEC: Shallue-van de Woestijne Indifferentiable Function to
  Elliptic Curves", 2022, `https://eprint.iacr.org/2022/759`). `doc/secp256k1.adoc` gains
  `[#crypto-secp256k1-ellswift]` "ElligatorSwift: keys that look like noise" (what it is for:
  a censor or a middlebox that blocks a protocol by recognising its handshake ({bip324}); how:
  every 64-byte string decodes to a key, so an encoding drawn at random is uniform; the
  example; pitfalls: fresh randomness per encoding, or two connections of one key are
  linkable; the hash binds both encodings, so each side passes them in the same order; the
  shared secret is input to the protocol's key schedule, never a key by itself).
  `changelog.adoc`: `* Added curves::secp256k1's ElligatorSwift: ellswift_encode, ellswift_decode, ellswift_create and ellswift_xdh with BIP 324's hash.`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output) and the superproject's
  `python3 tools/test/third_party_test.py` (expected: every case passed, the run's exit status
  0). Commit (library)
  `crypto: ElligatorSwift on libsecp256k1, against BIP 324's vectors`; (superproject)
  `third_party: BIP 324's ElligatorSwift and packet vectors among the bips target's files`.

## Task 2.5: MuSig2, with BIP 327's vectors and a nonce its signature consumes

**As built.** The message case `musig_nonce_copied` expects one error,
`{{call to deleted constructor of}}`, and one note on `musig.hpp`,
`{{'secret_nonce' has been explicitly marked deleted here}}`, not two substrings of one
error: `webcpp.compile-diagnostic` matches each directive with a diagnostic of its own. The
entropy test checks `entropy_unavailable` and that `nonce_gen` drew once and called nothing
after the failure; the zeroed buffer is `random_device::fill`'s contract, tested by
`random_device_failing`. The owner's two examples of Task 2.7d (below) show this task's
serialisation and tweak functions.

**Files:**
- Create: `include/webcpp/crypto/curves/secp256k1/musig.hpp`, `test/secp256k1_musig_test.cpp`,
  `test/secp256k1_musig_entropy_test.cpp`, `test/messages/musig_nonce_copied/Jamfile`, `test/messages/musig_nonce_copied/musig_nonce_copied.cpp`,
  `test/msvc/musig_nonce_copied/Jamfile`, `test/msvc/musig_nonce_copied/app.cpp`,
  `example/musig_wallet.cpp`, `example/musig_wallet.expected`.
- Modify: `include/webcpp/crypto/backends/secp256k1.hpp` (`#include <secp256k1_musig.h>`),
  `include/webcpp/crypto.hpp`, `test/Jamfile`, `test/messages/Jamfile`, `test/msvc/Jamfile`,
  `test/secp256k1_allocation_test.cpp`, `test/support/secp256k1_checks.hpp`,
  `test/wipe_objects_test.cpp` (the secret nonce), `test/windows_h_*_test.cpp`,
  `example/Jamfile`, `doc/secp256k1.adoc`, `doc/tables/crypto.json` (the operation, the
  message's `fixes` entry), `doc/changelog.adoc`; superproject `third_party/Jamfile` (the
  `bips` target's files gain `libs/bips/bip-0327/vectors/*.json`),
  `tools/test/third_party_test.py` (`test_bips_target_files`'s expected list gains the eight
  JSON files, failing first).

**Interfaces** (`curves/secp256k1/musig.hpp`, namespace `curves::secp256k1::musig`):

```cpp
/** The sizes of the encodings. */
inline constexpr std::size_t public_nonce_size = 66;
inline constexpr std::size_t aggregate_nonce_size = 66;
inline constexpr std::size_t partial_signature_size = 32;

/** The signers' aggregate key and what signing needs of it (libsecp256k1's keyagg cache,
 * public data, copyable), with the tweaks a protocol applies to it; made only by
 * `key_aggregate`, since a zeroed cache has no magic and libsecp256k1's `ARG_CHECK` on it
 * aborts. */
class aggregate_key {
public:
    /** The BIP 340 x-only key the final signature verifies under. */
    [[nodiscard]] std::array<std::uint8_t, 32> xonly_public_key() const noexcept;
    /** The aggregate as a full point (`secp256k1_musig_pubkey_get`), which a plain tweak acts on. */
    [[nodiscard]] public_key full_public_key() const noexcept;
    /** Adds t times G as a plain tweak (BIP 327's ApplyTweak, is_xonly false): `invalid_secret_key`
     * for a tweak not below the order or a result at infinity, after which the key is unchanged. */
    [[nodiscard]] boost::system::result<void> tweak_add(std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
    /** Adds t times G as an x-only tweak (is_xonly true), as `tweak_add` reports. */
    [[nodiscard]] boost::system::result<void> tweak_xonly_add(std::span<std::uint8_t const, 32> tweak_bytes) noexcept;
private:
    friend struct webcpp::crypto::detail::secp256k1_access;
    aggregate_key() noexcept = default;
    secp256k1_musig_keyagg_cache cache_value_{};
};

/** A signer's public nonce, an aggregate nonce, a session, a partial signature: parsed,
 * public, copyable; each made only by the functions below, with a private default
 * constructor and `detail::secp256k1_access` as its friend, as `aggregate_key`, since
 * libsecp256k1 checks each one's magic with `ARG_CHECK`. */
class public_nonce;     // secp256k1_musig_pubnonce
class aggregate_nonce;  // secp256k1_musig_aggnonce
class session;          // secp256k1_musig_session
class partial_signature;  // secp256k1_musig_partial_sig

/**
 * A signer's secret nonce: move-only, wiped on move and destruction, made only by
 * `nonce_gen` and consumed by `partial_sign`, which takes it by value, so that a nonce
 * cannot be passed to a second signature without an explicit `std::move` of an object already
 * moved from (which `partial_sign` refuses with `invalid_state`). Reusing a nonce in two
 * signatures reveals the secret key.
 */
class secret_nonce {
public:
    secret_nonce(secret_nonce&& moved_source) noexcept;
    secret_nonce& operator=(secret_nonce&& moved_source) noexcept;
    secret_nonce(secret_nonce const&) = delete;
    secret_nonce& operator=(secret_nonce const&) = delete;
    ~secret_nonce();
private:
    secp256k1_musig_secnonce nonce_value_{};
    std::array<std::uint8_t, 33> signer_key_{};  // the key pair it was made for, public
    bool fresh_ = false;
};

/** What `nonce_gen` returns: the secret nonce to keep, the public nonce to send. */
struct nonce_pair {
    secret_nonce secret_part;
    public_nonce public_part;
};

/** The workspace `key_aggregate`, `nonce_aggregate` and `partial_signature_aggregate` need for `count` items. */
[[nodiscard]] constexpr std::size_t workspace_size(std::size_t count) noexcept {
    // One array: `count` pointers to the parsed keys, nonces or partial signatures.
    return (count * sizeof(void const*)) + alignof(std::max_align_t);
}

/** Aggregates the signers' keys in the order given (`secp256k1_musig_pubkey_agg`); sort them
 * first (`std::ranges::sort`, `public_key`'s order is BIP 327's KeySort) where the protocol does. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<aggregate_key> key_aggregate(
    std::span<public_key const> signer_keys, workspace pointer_space) noexcept;

/** A fresh nonce for one signature (`secp256k1_musig_nonce_gen`), its session randomness
 * drawn from `random_device` inside, with the key pair's secret, its public key and the
 * aggregate key; `message_hash` and `extra_input`, 32 bytes each, are optional (empty spans).
 * @return The pair; `invalid_state`; `invalid_input_size`; `entropy_unavailable`; `backend_failure`. */
template <class Backend>
[[nodiscard]] boost::system::result<nonce_pair> nonce_gen(
    basic_keypair<Backend> const& signer, aggregate_key const& signers_key,
    std::span<std::uint8_t const> message_hash = {}, std::span<std::uint8_t const> extra_input = {}) noexcept;

/** Sums the signers' public nonces (`secp256k1_musig_nonce_agg`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<aggregate_nonce> nonce_aggregate(
    std::span<public_nonce const> signer_nonces, workspace pointer_space) noexcept;

/** The session of one message (`secp256k1_musig_nonce_process`): a 32-byte message, as
 * libsecp256k1 signs. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<session> nonce_process(
    aggregate_nonce const& nonce_sum, std::span<std::uint8_t const, 32> message_bytes,
    aggregate_key const& signers_key) noexcept;

/** This signer's partial signature (`secp256k1_musig_partial_sign`), consuming the nonce:
 * `invalid_state` for a moved-from key pair or nonce, or a nonce made for another key pair. */
template <class Backend>
[[nodiscard]] boost::system::result<partial_signature> partial_sign(
    basic_keypair<Backend> const& signer, secret_nonce signer_nonce,
    aggregate_key const& signers_key, session const& signing_session) noexcept;

/** Verifies one signer's partial signature (`secp256k1_musig_partial_sig_verify`):
 * `invalid_signature` when it does not verify. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<void> partial_signature_verify(
    partial_signature const& signature_part, public_nonce const& signer_nonce,
    public_key const& signer_key, aggregate_key const& signers_key,
    session const& signing_session) noexcept;

/** The BIP 340 signature of the partial signatures (`secp256k1_musig_partial_sig_agg`),
 * which `verify_schnorr` checks under `aggregate_key::xonly_public_key()`. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<std::array<std::uint8_t, 64>> partial_signature_aggregate(
    session const& signing_session, std::span<partial_signature const> signature_parts,
    workspace pointer_space) noexcept;

/** Parsing and serialising the public forms: `invalid_point` for a nonce whose points do not
 * parse (an aggregate nonce may hold the point at infinity, encoded as 33 zero bytes),
 * `invalid_signature` for a partial signature not below the order. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_nonce> parse_public_nonce(std::span<std::uint8_t const> encoded_nonce) noexcept;
template <class Backend = backends::secp256k1>
[[nodiscard]] std::array<std::uint8_t, public_nonce_size> serialize_public_nonce(public_nonce const& parsed_nonce) noexcept;
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<aggregate_nonce> parse_aggregate_nonce(std::span<std::uint8_t const> encoded_nonce) noexcept;
template <class Backend = backends::secp256k1>
[[nodiscard]] std::array<std::uint8_t, aggregate_nonce_size> serialize_aggregate_nonce(aggregate_nonce const& parsed_nonce) noexcept;
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<partial_signature> parse_partial_signature(std::span<std::uint8_t const> encoded_signature) noexcept;
template <class Backend = backends::secp256k1>
[[nodiscard]] std::array<std::uint8_t, partial_signature_size> serialize_partial_signature(partial_signature const& parsed_signature) noexcept;
```

  `nonce_gen` draws its 32 bytes of session randomness into a `detail::secp256k1_secret`
  (libsecp256k1 zeroes `session_secrand32` itself; webcpp wipes it again), passes the
  key pair's secret and public key and the cache, and records the signer's compressed key in
  the `secret_nonce`; `partial_sign` refuses a nonce whose recorded key is not the signer's
  before calling (libsecp256k1 checks it with `ARG_CHECK`) and marks the nonce used (it is a
  by-value parameter, destroyed and wiped at the call's end in any case). No `partial_sign`
  takes a nonce by reference.

- [ ] **Step 1: The failing tests.** `test/secp256k1_musig_test.cpp` (`crypto-run
  secp256k1_musig : secp256k1_musig_test.cpp : <library>/third_party//secp256k1
  <library>/third_party//bips <library>/webcpp//boost_json ;`), reading
  `WEBCPP_TEST_BIPS "/bip-0327/vectors/<file>.json"`:
  - `key_sort_vectors.json`: `std::ranges::sort` of the six parsed keys gives `sorted_pubkeys`;
  - `key_agg_vectors.json`: the 4 valid cases give `expected` as `xonly_public_key()`; the 5
    error cases: keys 3, 4 and 5 are refused by `parse_public_key` (`invalid_public_key`), the
    x-only tweak 0 is `invalid_secret_key` from `tweak_xonly_add`, and the plain tweak 1 on key
    6's aggregate is `invalid_secret_key` (the result at infinity);
  - `nonce_agg_vectors.json`: the 2 valid cases give `expected` (the second sums to the point
    at infinity, which `serialize_aggregate_nonce` writes as the vector does); the 3 error
    cases are `invalid_point` from `parse_public_nonce`;
  - `sign_verify_vectors.json`: the 4 valid cases whose message is 32 bytes verify their
    `expected` partial signature with `partial_signature_verify` under `pnonces[0]` and the
    signer's key (the 2 cases with the empty and the 38-byte message are counted as skipped:
    libsecp256k1 signs 32-byte messages); the 3 `verify_fail` cases, which carry no
    `aggnonce_index`, first aggregate their `pnonces` with `nonce_aggregate` into the session's
    nonce, and are `invalid_signature`
    (the third from `parse_partial_signature`); the 2 `verify_error` cases are `invalid_point`
    and `invalid_public_key`; the `sign_error` cases that need no secret nonce of the
    vector's (an invalid key, three invalid aggregate nonces) are refused at parsing;
  - `tweak_vectors.json`: the 5 valid cases' `expected` verify after the tweaks are applied
    in order; the error case is `invalid_secret_key`;
  - `sig_agg_vectors.json`: the 4 valid cases give `expected` from
    `partial_signature_aggregate` over a session built from the case's aggregate nonce, keys
    and tweaks, and each verifies with `verify_schnorr`; the error case is `invalid_signature`
    from `parse_partial_signature`;
  - signing with the vectors' own secret nonces (`secnonces`, the deterministic signing of
    `det_sign_vectors.json`) and `nonce_gen_vectors.json` are not reachable through the
    public interface by design (a `secret_nonce` exists only from `nonce_gen`, with fresh
    randomness, and libsecp256k1 implements no deterministic signing); libsecp256k1's own
    `src/tests.c` runs them from the same JSON files (`src/modules/musig/vectors.h`, generated
    by its `tools/test_vectors_musig2_generate.py`) in the own lane `c-suites`, which this task
    runs on its four targets;
  - round trips: 2-of-2 and 3-of-3 over seeded key pairs and messages, plain and with an
    x-only tweak: `nonce_gen`, `nonce_aggregate`, `nonce_process`, `partial_sign` by each,
    `partial_signature_verify` of each, `partial_signature_aggregate`, `verify_schnorr` under
    the (tweaked) aggregate key; a partial signature changed in one bit is
    `invalid_signature`;
  - the nonce: after `partial_sign(signer, std::move(n), ...)`, a second
    `partial_sign(signer, std::move(n), ...)` is `invalid_state` and the program does not
    abort (the test carries `// NOLINT(bugprone-use-after-move)` with its reason); a nonce made
    by one key pair and given to another is `invalid_state`; a moved-from key pair is
    `invalid_state` from `nonce_gen` and `partial_sign`; a 31-byte message hash or extra input
    is `invalid_input_size`; empty spans of keys, nonces and partial signatures, and a short
    workspace, are `invalid_input_size`;
  - `static_assert(!std::is_default_constructible_v<T>)` for `aggregate_key`, `public_nonce`,
    `aggregate_nonce`, `session`, `partial_signature` and `secret_nonce`;
  - BIP 327's KeySort ties the vectors to `public_key`'s `<=>` (above).

  `test/secp256k1_musig_entropy_test.cpp`, a program of its own because version 0.1's seam is
  a whole-program define (`crypto-run secp256k1_musig_entropy : secp256k1_musig_entropy_test.cpp
  : <library>/third_party//secp256k1 <define>WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY ;`, as
  `test/Jamfile` builds `random_device_failing`): its `webcpp_test_entropy` script answers
  the key pairs' context seeds (`keypair::make` draws from the same source) and fails the
  next call; `nonce_gen` then returns `entropy_unavailable`, and the session randomness's
  buffer is zero afterwards.

  The message test `test/messages/musig_nonce_copied/` (`webcpp.compile-diagnostic
  musig_nonce_copied : musig_nonce_copied.cpp : <library>/third_party//secp256k1 : native
  wasip2 ;`) passes a `secret_nonce` lvalue to `partial_sign` and expects the deleted copy
  constructor's diagnostic as two substrings, `{{call to deleted constructor of}}` and
  `{{secret_nonce}}` (clang qualifies the type as the source names it, so the test does not
  depend on that), which `webcpp.compile-diagnostic` checks (spec.md's scenario "A MuSig2
  nonce passed without a move", amended in this change: C++ cannot reject a second
  `std::move` of one object at compile time, so the second use is spec.md's run-time
  scenario "A MuSig2 nonce used twice", the `invalid_state` case above); `test/msvc/musig_nonce_copied/` is its MSVC case
  (`webcpp.compile-fail`). `wipe_objects_test.cpp` gains `WEBCPP_TEST_WIPE_SECP256K1`'s second
  object: a `secret_nonce`'s `nonce_value_` is zero after destruction and after a move.

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_musig
  libs/crypto/test//secp256k1_musig_entropy`: expected compile errors;
  `python3 tools/test/third_party_test.py test_bips_target_files`: expected to fail, naming
  the eight JSON files.
- [ ] **Step 2: Implement**; the superproject's `bips` target names the eight JSON files.
- [ ] **Step 3: The example** `example/musig_wallet.cpp` (`<library>/third_party//secp256k1`):
  a two-device wallet (a phone and a laptop) whose coins need both: the two public keys are
  sorted and aggregated; each device makes a nonce and sends its public nonce; each signs its
  part; the aggregate signature verifies under the one x-only key, which on the chain looks
  like any single signer's. `.expected`:

```
aggregate key made from 2 signers
both partial signatures verify
the aggregate signature verifies as one BIP 340 signature
```

- [ ] **Step 4: The rows and the documentation.** `crypto.json` operation `musig` (every
  function above; header `<webcpp/crypto/curves/secp256k1/musig.hpp>`; tests
  `secp256k1_musig`; section `crypto-secp256k1-musig`), the `fixes` entry
  `"musig_nonce_copied": "Pass the nonce with std::move, once: a secret nonce signs one message."`,
  the `use_cases` row "Require two devices to sign for a wallet, with one ordinary signature
  on the chain" (example `musig_wallet`), new `sources` `bip327` (BIP 327,
  `https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki`) and `musig2` (Nick,
  Ruffing and Seurin, "MuSig2: Simple Two-Round Schnorr Multi-Signatures", CRYPTO 2021,
  `https://eprint.iacr.org/2020/1261`). `doc/secp256k1.adoc` gains `[#crypto-secp256k1-musig]`
  "MuSig2: many signers, one signature" (what it is for: a shared account, a lightning
  channel, a wallet split across devices ({bip327}); how: two rounds, nonces then partial
  signatures; the example; pitfalls: a nonce used twice reveals the secret key, which is why
  `partial_sign` takes it by value and refuses a used one; never sign before every public
  nonce is in; a key aggregated with an attacker's key chosen after seeing yours is safe in
  MuSig2 but not in a naive sum (Task 2.3's `pubkey_combine`); the tweaks belong to a
  protocol (Taproot's tagged hash is a domain library's, design D24)). `changelog.adoc`:
  `* Added curves::secp256k1::musig, BIP 327's MuSig2, with a secret nonce consumed by its partial signature.`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output; the message test
  runs in them on native and wasip2), the own lane `c-suites` on its four targets (each
  target's lane command with `libs/crypto/test/c_suites//c-suites` in place of the two
  directories; expected: `secp256k1_tests` passes), and the superproject's
  `python3 tools/test/third_party_test.py` (every case passed, exit status 0). Commit (library)
  `crypto: MuSig2 on libsecp256k1, against BIP 327's vectors, with a nonce its signature consumes`;
  (superproject) `third_party: BIP 327's vectors among the bips target's files`.

## Task 2.6: Silent payments, with BIP 352's vectors

**As built.** `support/bip352_vectors.c` is an `obj` of its own in `test/Jamfile`, compiled with
`<warnings>off <warnings-as-errors>off`, so the C++ test keeps every warning. The concept
`label_lookup` holds when the lookup is `noexcept` and its result type is exactly
`std::optional<std::span<std::uint8_t const, 32>>` (`std::same_as` on
`std::invoke_result_t`), not any type convertible to it: a lookup returning a `std::array` or
an optional of one by value would leave the span over a destroyed temporary, which
libsecp256k1 reads after the lookup returns. The Doc Comment says the lookup owns the bytes
until it is called again, and the message case `label_lookup_by_value` (with its MSVC case)
shows the refusal. `scan_outputs` calls libsecp256k1 with the scan key pair's context, not
`secp256k1_context_static` as the increment's "Contexts" rule has it, so that libsecp256k1
marks its public values public under the constant-time checkers (Task 2.7, Step 2); outside
them every key pair's context has no flag (`SECP256K1_CONTEXT_NONE`), as before.

**Files:**
- Create: `include/webcpp/crypto/curves/secp256k1/silent_payments.hpp`,
  `test/secp256k1_silent_payments_test.cpp`, `test/support/bip352_vectors.c`,
  `test/support/bip352_vectors.h`, `example/silent_payment.cpp`,
  `example/silent_payment.expected`.
- Modify: `include/webcpp/crypto/backends/secp256k1.hpp` (`#include <secp256k1_silentpayments.h>`),
  `include/webcpp/crypto.hpp`, `test/Jamfile`, `test/secp256k1_allocation_test.cpp`,
  `test/support/secp256k1_checks.hpp`, `test/windows_h_*_test.cpp`, `example/Jamfile`,
  `doc/secp256k1.adoc`, `doc/tables/crypto.json`, `doc/changelog.adoc`; superproject
  `third_party/Jamfile` (the `bips` target's files gain
  `libs/bips/bip-0352/send_and_receive_test_vectors.json`), `tools/test/third_party_test.py`
  (`test_bips_target_files`'s list gains that file, and a new case in `CASES`,
  `test_bip352_vectors_match_libsecp256k1`, asserts that it and libsecp256k1's
  `src/modules/silentpayments/bip352_send_and_receive_test_vectors.json` are the same bytes,
  which they are at the two pins).

**Interfaces** (`curves/secp256k1/silent_payments.hpp`, namespace
`curves::secp256k1::silent_payments`):

```cpp
/** The size of a transaction's smallest outpoint, serialised: 36 bytes. */
inline constexpr std::size_t outpoint_size = 36;
/** The most outputs one scan key's group may hold (BIP 352's limit). */
inline constexpr std::size_t group_limit = SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT;

/** A recipient as the sender reads it from the address: its scan and spend keys (decoding
 * the address is a domain library's). */
struct recipient {
    public_key scan_key;
    public_key spend_key;
};

/** A label (`secp256k1_silentpayments_label`, serialised) and its tweak. */
struct label {
    std::array<std::uint8_t, 33> point;
    std::array<std::uint8_t, 32> tweak;
};

/** An output the recipient found: its x-only key, the tweak that spends it (added to the
 * spend secret key with `seckey_tweak_add`), and the label it was sent to, if any. */
struct found_output {
    std::array<std::uint8_t, 32> output_key;
    secret<32> spend_tweak;
    std::optional<std::array<std::uint8_t, 33>> label_point;
};

/** The prevouts' summary a recipient scans with (101 bytes, public, copyable). */
class prevouts_summary;

/** Workspaces, in bytes, of the sender, the summary and the scan. */
[[nodiscard]] constexpr std::size_t send_workspace_size(std::size_t recipient_count,
    std::size_t taproot_input_count, std::size_t plain_input_count) noexcept;
[[nodiscard]] constexpr std::size_t summary_workspace_size(std::size_t taproot_input_count,
    std::size_t plain_input_count) noexcept;
[[nodiscard]] constexpr std::size_t scan_workspace_size(std::size_t output_count) noexcept;

// Their bodies, each array in the order the carver hands them out (the Rules: one
// alignof(std::max_align_t) of padding per array; an array of zero elements takes none and is
// passed to libsecp256k1 as nullptr):
//   send: r recipients' secp256k1_silentpayments_recipient, their r pointers, r
//         secp256k1_xonly_pubkey outputs, their r pointers, t Taproot key pair pointers, p
//         plain secrets of 32 bytes, their p pointers:
//         r * (sizeof(secp256k1_silentpayments_recipient) + sizeof(void*)
//              + sizeof(secp256k1_xonly_pubkey) + sizeof(void*))
//         + t * sizeof(void*) + p * (32 + sizeof(void*)) + 7 * alignof(std::max_align_t)
//   summary: t parsed secp256k1_xonly_pubkey, their t pointers, p pointers to the public
//         keys' points: t * (sizeof(secp256k1_xonly_pubkey) + sizeof(void*))
//         + p * sizeof(void*) + 3 * alignof(std::max_align_t)
//   scan: n parsed secp256k1_xonly_pubkey outputs, their n pointers, n
//         secp256k1_silentpayments_found_output, their n pointers:
//         n * (sizeof(secp256k1_xonly_pubkey) + sizeof(void*)
//              + sizeof(secp256k1_silentpayments_found_output) + sizeof(void*))
//         + 4 * alignof(std::max_align_t)

/** The sender's outputs (`secp256k1_silentpayments_sender_create_outputs`): one x-only key
 * per recipient, in the recipients' order, from the secret keys of the transaction's eligible
 * inputs, Taproot inputs (their keys negated for an odd y, as BIP 352 says) apart from the
 * others, and the smallest outpoint, which the caller determines.
 * The context is the first Taproot input's key pair's, else the first plain input's.
 * @return Nothing; `invalid_input_size` (no recipient, no input at all, outputs not one per
 * recipient, a short workspace, more than `group_limit` recipients sharing a scan key, which
 * webcpp counts only when there are more recipients than that); `invalid_state` (a moved-from
 * key pair, or a null pointer among the inputs); `invalid_secret_key` when libsecp256k1 refuses: the inputs' keys sum to zero, or
 * a recipient's spend key gives no output, which only a key chosen against the sender can. */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<void> create_outputs(
    std::span<recipient const> recipients,
    std::span<std::uint8_t const, outpoint_size> smallest_outpoint,
    std::span<std::type_identity_t<basic_keypair<Backend>> const* const> taproot_inputs,
    std::span<std::type_identity_t<basic_keypair<Backend>> const* const> plain_inputs,
    std::span<std::array<std::uint8_t, 32>> output_keys, workspace pointer_space) noexcept;

/** The `m`-th label of a scan key (`secp256k1_silentpayments_recipient_label_create`); 0 is
 * BIP 352's change label. */
template <class Backend>
[[nodiscard]] boost::system::result<label> create_label(
    basic_keypair<Backend> const& scan_key, std::uint32_t label_number) noexcept;

/** The spend key of a labelled address (`..._create_labeled_spend_pubkey`). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<public_key> create_labeled_spend_key(
    public_key const& spend_key, label const& address_label) noexcept;

/** The prevouts' summary (`..._recipient_prevouts_summary_create`) from the eligible inputs'
 * public keys: `invalid_input_size` for no input key at all or a short workspace (an
 * `ARG_CHECK` in libsecp256k1); `invalid_public_key` when they sum to infinity or a Taproot
 * key does not parse (not a silent payment). */
template <class Backend = backends::secp256k1>
[[nodiscard]] boost::system::result<prevouts_summary> make_prevouts_summary(
    std::span<std::uint8_t const, outpoint_size> smallest_outpoint,
    std::span<std::array<std::uint8_t, 32> const> taproot_input_keys,
    std::span<public_key const> plain_input_keys, workspace pointer_space) noexcept;

/** Scans a transaction's x-only outputs (`..._recipient_scan_outputs`) with the scan key pair
 * and the unlabelled spend key; `label_lookup` is called with a candidate label's 33 bytes
 * and returns its tweak or `std::nullopt`, noexcept (no labels: the default).
 * @return The number of outputs found, written to the front of `found` (at least as many
 * entries as outputs); `invalid_input_size` (no output key, `found` shorter than the outputs,
 * a short workspace); `invalid_state`; `invalid_public_key` (an output that is not an x-only
 * key). */
template <class Backend, label_lookup LabelLookup = no_labels>
[[nodiscard]] boost::system::result<std::size_t> scan_outputs(
    basic_keypair<Backend> const& scan_key, public_key const& spend_key,
    prevouts_summary const& summary, std::span<std::array<std::uint8_t, 32> const> output_keys,
    std::span<found_output> found, workspace pointer_space,
    LabelLookup const& label_lookup = {}) noexcept;
```

  `no_labels` is a callable that returns `std::nullopt`; `scan_outputs` is constrained by
  the concept `label_lookup<LabelLookup>`, which holds when
  `std::is_nothrow_invocable_r_v<std::optional<std::span<std::uint8_t const, 32>>,
  LabelLookup const&, std::span<std::uint8_t const, 33>>` does (an exception must not cross
  libsecp256k1's C frames). libsecp256k1's callback is an `extern "C"` trampoline,
  `webcpp_crypto_detail_secp256k1_label_lookup`, which reaches the callable through
  `label_context`; the tweak span it returns stays valid until the next call, as libsecp256k1
  requires. The key pairs' spans name `std::type_identity_t` so that a vector of
  pointers converts without deducing the backend. The plain inputs' secrets are taken out of
  their key pairs into the workspace,
  which is wiped before `create_outputs` returns on every path; the recipients' C structs get
  their `index` from their position. The scan key's secret is taken out into a
  `detail::secp256k1_secret`. Each side's input array (Taproot, plain) is passed as `nullptr`
  when its count is zero, as libsecp256k1 requires (an empty `std::span` may hold a non-null
  pointer, and the carver hands out `nullptr` for zero elements); every pointer in the input
  spans is checked non-null before any is used.

- [ ] **Step 1: The failing test** `test/secp256k1_silent_payments_test.cpp`
  (`crypto-run secp256k1_silent_payments : secp256k1_silent_payments_test.cpp
  support/bip352_vectors.c : <library>/third_party//secp256k1
  <library>/third_party//secp256k1-test-config ;`). It reads BIP 352's vectors as
  libsecp256k1's own test does, from `src/modules/silentpayments/vectors.h`, compiled in C,
  not C++: `test/support/bip352_vectors.c` includes it (on the include path
  `secp256k1-test-config` gives; a 4747-line header of large static arrays initialised from
  string literals, which C++ and MSVC's warnings would reject) and exposes
  `struct bip352_test_vector const* webcpp_test_bip352_vector(size_t index)` and
  `size_t webcpp_test_bip352_vector_count(void)`, declared in `test/support/bip352_vectors.h`
  (`extern "C"` under C++); the `.c` file is compiled with `<warnings>off`, as `c_suites`
  compiles libsecp256k1's sources. libsecp256k1's
  `tools/tests_silentpayments_generate.py` derives the header from the JSON file that is byte for byte
  BIP 352's `send_and_receive_test_vectors.json` (the superproject's test checks the two are
  equal). That data holds each case's input keys, smallest outpoint and recipients' keys
  already taken out of its transactions and addresses, the protocol's part (design D24),
  which webcpp does not write, in the library or in its tests.
  - Sending, the 28 cases of `bip352_test_vectors`: `create_outputs` with key pairs of the
    plain and Taproot secret keys and the recipients' keys repeated `count` times; the outputs
    match one of the case's `num_output_sets` permutations; a case with no expected output is
    refused, as libsecp256k1's test expects its function to fail there: the three such cases
    of the JSON are "No valid inputs" (`invalid_input_size`, which libsecp256k1 would take as
    an `ARG_CHECK`), "Input keys sum up to zero" (`invalid_secret_key`) and "Maximum
    per-group recipient limit K_max is exceeded", 2324 recipients on one scan key
    (`invalid_input_size`, webcpp's own count).
  - Receiving, the 29 subtests: `make_prevouts_summary` (refused where the case expects no
    found output: `invalid_input_size` with no input key, `invalid_public_key` for keys that
    sum to infinity), labels from `label_integers` with `create_label`, `scan_outputs` with a
    lookup over those labels; where `full_check` is set, every found output's key and spend
    tweak are among the case's, and the BIP 340 signature by
    `keypair::make(seckey_tweak_add(spend_seckey, spend_tweak))` of SHA-256(`"message"`) with
    the auxiliary randomness SHA-256(`"random auxiliary data"`) (Boost.Hash2) equals the
    case's; the found count equals the case's; an output found with a label has a label the
    lookup knows, one found without has none.
  - Refusals: no recipient, no input, no output key to scan, no input key for the summary, a
    short workspace, `found` shorter than the outputs, an output key that does not parse, a
    moved-from key pair, a null pointer among the inputs; only Taproot inputs and only plain
    inputs, each from a `std::vector` emptied after `reserve` (a non-null `data()` with size
    zero), run without abort; `static_assert(!std::is_default_constructible_v<prevouts_summary>)`;
    a lookup that is not noexcept
    does not satisfy `label_lookup`, which the test checks with a `requires` expression.

  The superproject's `test_bips_target_files` expects the BIP 352 file too, and
  `test_bip352_vectors_match_libsecp256k1` compares the two JSON files byte for byte.

  Run `$B2 -a --build-dir=bin/lane-native libs/crypto/test//secp256k1_silent_payments`:
  expected a compile error; `python3 tools/test/third_party_test.py test_bips_target_files
  test_bip352_vectors_match_libsecp256k1`: expected the first to fail (the file is not listed)
  and the second, new, to pass.
- [ ] **Step 2: Implement**; the superproject's `bips` target names the JSON file.
- [ ] **Step 3: The example** `example/silent_payment.cpp` (`<library>/third_party//secp256k1`):
  a donation page publishes one reusable pair of keys (scan, spend); a donor's wallet with one
  input pays it; the recipient scans the transaction's outputs, finds the payment, and makes
  the spending key pair with `seckey_tweak_add`; a second donation to the same keys gives an
  unrelated output key. `.expected`:

```
the recipient found 1 output
the spending key matches the output
two payments to one address share no output key
```

- [ ] **Step 4: The rows and the documentation.** `crypto.json` operation `silent_payments`
  (every function above; header `<webcpp/crypto/curves/secp256k1/silent_payments.hpp>`; tests
  `secp256k1_silent_payments`; section `crypto-secp256k1-silent-payments`), the `use_cases`
  row "Publish one donation address that never shows two payments as related (BIP 352's
  addresses and transactions are a domain library's)" (example `silent_payment`), the new
  `sources` entry `bip352` (BIP 352,
  `https://github.com/bitcoin/bips/blob/master/bip-0352.mediawiki`). `doc/secp256k1.adoc`
  gains `[#crypto-secp256k1-silent-payments]` "Silent payments" (what it is for: a reusable
  address whose payments an onlooker cannot link ({bip352}); how: ECDH between the inputs'
  keys and the scan key tweaks the spend key per transaction; the example; pitfalls: the
  smallest outpoint must be the transaction's, or the recipient never finds the payment; every
  generated output must be in the transaction; scanning costs one ECDH per transaction, which
  a light client must plan for; labels beyond change hurt light clients; the workspace sizes).
  `changelog.adoc`: `* Added curves::secp256k1::silent_payments, BIP 352's sender outputs, labels and the recipient's scan.`
- [ ] **Step 5: Run the lanes** (the Rules' commands and expected output) and the superproject's
  `python3 tools/test/third_party_test.py` (every case passed, exit status 0). Commit (library)
  `crypto: silent payments on libsecp256k1, against BIP 352's vectors`; (superproject)
  `third_party: BIP 352's vectors among the bips target's files, the same as libsecp256k1's copy`.

## Task 2.7: The capability rows, constant time, sizes and the teaching chapters; increment 2 whole

**As built.**

- **Step 1.** Each function's assertion is
  `static_assert(webcpp::crypto::detail::secp256k1_refuses_others<families::<family>, Backend>)`:
  `secp256k1_refusal<Family, Backend>`, specialised per family for a backend other than
  libsecp256k1, holds the generated `WEBCPP_CRYPTO_DETAIL_REFUSE_<FAMILY>(Backend)` (the form
  Task 4' landed), which cannot stand bare in a function since for libsecp256k1 it asserts both
  "linked" and "not linked". The third family's title is "BIP 352 silent payments", not "Silent
  payments", so that the generated refusals and tables read as sentences ("has no BIP 352
  silent payments"). The three module families' backend cells take Task 14's per-checker form,
  `lane` and `lane_note`: memcheck and MemorySanitizer natively over `ctime_tests.c` and
  webcpp's code around each module; dudect is listed only on the `secp256k1` family, for
  `ecdh_x`.
- **Step 2.** The cases are in `test/constant_time/paths.hpp`, which `ctgrind_test.cpp` and
  `msan_test.cpp` share (stream A's form), and cover the key pair's `make`, `sign_schnorr`,
  `tweak_xonly` and `create_label` beside the listed functions. dudect times `ecdh_x` in a
  program of its own, `test/constant_time/dudect_secp256k1_test.cpp` (`dudect_secp256k1`), as
  stream A split `dudect_sodium_test.cpp` from `dudect_test.cpp`, keeping libsecp256k1 off
  BearSSL's program; it takes 500,000 measurements (wasmtime's 600 seconds) and, natively on
  arm64 (macOS and Linux), runs under DIT (data-independent timing) as stream A's dudect
  programs do, so that its |t| is judged against the threshold 10 there; under wasmtime no DIT
  is set and it is judged too. Measured on the final code: 1.33 on macOS arm64, 1.99 in the
  Linux arm64 container, 2.86 on wasip2. The lane ran locally (macOS arm64, wasip2, and a
  Linux arm64 container for memcheck and MemorySanitizer); its Linux x86-64 run is the CI's.
- **Step 3.** The programs follow `test/sizes`' contract, `<family>_<backend>` and
  `<family>_<backend>_<operation>`: `secp256k1_secp256k1_ecdh_x`, `secp256k1_secp256k1_recover`,
  `secp256k1_ellswift_secp256k1`, `secp256k1_musig_secp256k1` and
  `secp256k1_silent_payments_secp256k1`, declared with `webcpp.link` and tabled by the
  superproject's `webcpp.program-sizes` (release builds only), with the two operations named in
  the `secp256k1` family's `sized_operations`. The ElligatorSwift program makes the two
  encodings and their shared secret.
- **Step 4.** No section names an application (none was looked for, so none has a public source
  showing noble or scure for the primitive shown), and each section says, in one sentence, "This
  section names no application." The five use cases link the teaching sections, and a use case's
  `answers` key names the operation section it answers, so that `llms.txt` keeps each
  operation's "Use it to".
- **Steps 5 to 7.** Green before the pull requests is the local run of the increment's "As
  built"; the review is the whole-branch review with one fix wave (the label lookup's exact
  result type, Task 2.6); chapter 17 is written (`d25b22f`, the increment's "As built"); the
  pull requests remain (`STATUS.md`).
- **Task 2.7d, the owner's two MuSig2 examples (2026-11-19).** `example/musig_network.cpp`:
  two devices' MuSig2 signature in which every public part crosses a byte buffer (each public
  nonce, the aggregate nonce and each partial signature serialised and parsed), each partial
  signature verified before the aggregate, and a part with one byte flipped refused.
  `example/musig_taproot.cpp`: the aggregate key tweaked by BIP 341's TapTweak with
  `aggregate_key::tweak_xonly_add` before any nonce is made (the TapTweak hash, the protocol's,
  computed in the example with webcpp::hash's SHA-256, D24), and the signature
  verifying under the output key and refused under the internal one. Both run on every lane
  against their `.expected`, are included in `doc/secp256k1.adoc`'s MuSig2 section beside
  `musig_wallet`, and add no changelog line.

Runs once stream B is rebased onto stream A's branch with Tasks 4', 5', 7b, 11', 14 and 15
committed (or onto `main` after A merges).

**Files:**
- Modify: `include/webcpp/crypto/curves/secp256k1.hpp` and the five module headers (the
  refusal and the default), `test/messages/secp256k1_other_backend/` and
  `test/msvc/secp256k1_other_backend/` (the generated text), `test/messages/Jamfile`,
  `test/msvc/Jamfile`, `test/default_backend_test.cpp` (the four families' defaults),
  `doc/tables/crypto.json` (the families, the `fixes` entries), `test/constant_time/ctgrind_test.cpp`,
  `test/constant_time/msan_test.cpp`, `test/constant_time/dudect_test.cpp`,
  `test/sizes/Jamfile`, `doc/wallets.adoc`, `doc/p2p.adoc`, `doc/curves.adoc` (its text
  around the generated table), `doc/secp256k1.adoc` (its opening), `doc/changelog.adoc`;
  superproject `CODING-GUIDELINES.md` chapter 17 (crypto's entry: increment 2's state).
- Create: `test/messages/ellswift_other_backend/`, `test/messages/musig_other_backend/`,
  `test/messages/silent_payments_other_backend/` and their `test/msvc/` cases,
  `test/sizes/secp256k1_ecdh_x.cpp`, `test/sizes/secp256k1_recover.cpp`,
  `test/sizes/secp256k1_ellswift_xdh.cpp`, `test/sizes/secp256k1_musig_sign.cpp`,
  `test/sizes/secp256k1_silent_payments_send.cpp`.

- [ ] **Step 1: The families.** `crypto.json`'s `families` (Task 4' form) gain, beside version
  0.1's `secp256k1` row, which takes the operations `recovery`, `public_key`, `key_tweaks`
  and `key_formats`: `{"name": "secp256k1_ellswift", "title": "ElligatorSwift", "group":
  "curves", "since": "0.2"}`, `{"name": "secp256k1_musig", "title": "MuSig2", "group":
  "curves", "since": "0.2"}` and `{"name": "secp256k1_silent_payments", "title": "Silent
  payments", "group": "curves", "since": "0.2"}`, each with `default` libsecp256k1 on all four
  targets, `why` "the only backend" (design D32; roadmap's default table), the backend
  `secp256k1` with `targets` all four, `runtime` false, `constant_time` per target in Task
  4''s form (`{"native": ..., "wasip2": ..., "wasip3": ..., "emscripten": ...}`, each "by
  design", citing `libsecp256k1` and naming the Step 2 lane where it measures that target),
  and the operation's tests; the operations rows of Tasks 2.1 to 2.6 move under them as Task
  4' moved version 0.1's. `generate.py` then generates, in `detail/capabilities.hpp`, the
  family and name tags of the three rows (`families::secp256k1_ellswift`,
  `families::secp256k1_musig`, `families::secp256k1_silent_payments` and the `tags::` of the
  same names, beside those Task 4' generates for version 0.1's `secp256k1` row, called
  `families::secp256k1` here), `WEBCPP_CRYPTO_DETAIL_HAVE_<NAME>` and
  `WEBCPP_CRYPTO_DETAIL_TITLE_<NAME>` for each, `supports.hpp`'s table, a
  `default_backend_of<families::...>` with `backends::secp256k1` per family in
  `detail/defaults.hpp`, and `test/capabilities_test.cpp` (`supports_v` true on the four
  targets, false for every other backend). Each secp256k1 function's default becomes
  `class Backend = default_backend_t<families::<its family>>` (as Task 7b's formats write
  it), and `test/default_backend_test.cpp` asserts each of the four is `backends::secp256k1`
  on every target. The controller's ruling Q3 (one source of refusal text, D33): the
  assertion in `curves/secp256k1.hpp` and the module headers becomes
  `static_assert(webcpp::crypto::detail::secp256k1_backend<Backend>,
  WEBCPP_CRYPTO_DETAIL_REFUSAL("this backend", <NAME>))` with `<NAME>` the function's family
  (`SECP256K1`, `SECP256K1_ELLSWIFT`, `SECP256K1_MUSIG`, `SECP256K1_SILENT_PAYMENTS`), or the
  form Task 4' lands if it differs; `WEBCPP_CRYPTO_DETAIL_SECP256K1_ONLY` is deleted;
  `test/messages/secp256k1_other_backend/` and its MSVC case expect the generated text (the
  test written first, failing on the old text); one new message case per other family
  (`ellswift_other_backend`, `musig_other_backend`, `silent_payments_other_backend`, each in a
  directory of its own, with its MSVC case); `tweak.hpp` and `formats.hpp` belong to the
  `secp256k1` family and use its macro, which `secp256k1_other_backend` already tests, so they
  get no case of their own; the `fixes` entries name the type to write
  (`curves::secp256k1::musig::...` with the backend left out).
- [ ] **Step 2: Constant time** (spec "Tests: constant time for the new code"). Task 14's
  harness already runs libsecp256k1's `src/ctime_tests.c`, which covers the library's side of
  every module compiled (ECDH, recovery, the seckey tweaks, the key pair, Schnorr, MuSig2's
  nonce generation and partial signing, ElligatorSwift's create and xdh, silent payments'
  sender, label and scan). This step adds webcpp's own code around them to
  `ctgrind_test.cpp` and `msan_test.cpp`, each secret marked undefined
  (`detail::declassify.hpp`'s seam): `ecdh_x` and `ecdh_point` (the key pair's secret),
  `sign_recoverable` and the hedged `sign_ecdsa` (the secret, the extra entropy),
  `seckey_tweak_add`, `seckey_tweak_mul` and `seckey_negate` (the secret and the tweak),
  `encode_pkcs8` and `decode_pkcs8` (the secret; the re-encoding's `equal` on secret bytes),
  `ellswift_create` and `ellswift_xdh`, `nonce_gen` and `partial_sign` (the secret, the
  nonce), `create_outputs` and `scan_outputs` (the input secrets, the scan secret, the
  workspace's wipe); each verdict webcpp branches on (a refusal from libsecp256k1, the
  `equal` of a re-encoding) is declassified where it is read. `dudect_test.cpp` adds
  `ecdh_x` natively and under wasmtime, classes differing only in the secret. Run the lane
  `constant-time` on Linux x86-64; expected: no report but the planted leak's.
- [ ] **Step 3: Sizes** (Task 11''s lane): one program per new cell, each calling its
  family's fixed operation once (`ellswift_xdh`; a 2-of-2 MuSig2 signature; `create_outputs`
  for one recipient), and two per new operation of the existing `secp256k1` family (`ecdh_x`,
  `recover`), declared in `test/sizes/Jamfile` through `webcpp.size-table` (release builds
  only); `generate.py` reads the lane's table into the size matrix, whose rows are per
  operation (the spec's "binary size per target per operation"), so the two `secp256k1`
  programs are rows of their own under the family's cell. Run the lane `sizes` on its four targets.
- [ ] **Step 4: The teaching chapters** (design D19). `doc/wallets.adoc` gains, after
  "A tweaked key: Taproot": `[#crypto-wallets-recovery]` "Who signed this? Recoverable
  signatures" (the problem: a wallet that receives a signed request and no key; the idea;
  `recover_signer`'s code by tag; mistakes: trusting a recovered key without comparing it;
  the address hashing and EIP-191's message prefix belong to webcpp::ethereum, design D24),
  `[#crypto-wallets-musig]` "Two devices, one signature: MuSig2" (`musig_wallet`'s code;
  mistakes: reusing a nonce, signing before every nonce is in, a naive key sum) and
  `[#crypto-wallets-silent]` "A reusable address: silent payments" (`silent_payment`'s code;
  mistakes: the outpoint, a dropped output, scanning cost). `doc/p2p.adoc` gains
  `[#crypto-p2p-nostr-dm]` "A shared key in Nostr's direct messages" (`ecdh_x_coordinate`'s
  code; NIP-44's conversation key as what webcpp::nostr derives ({nip44}); mistakes: hashing
  the x coordinate differently on the two sides, reusing the conversation key as a message
  key) and `[#crypto-p2p-v2]` "A handshake an observer cannot recognise" (`ellswift_handshake`'s
  code; BIP 324 as the protocol that uses it ({bip324})). By the controller's ruling Q2, a section
  names an application only where that application's public source shows it uses noble or
  scure for the primitive shown, citing that source in `crypto.json`'s `sources`; where none
  is verifiable, the section names none and says so in one sentence; it never copies an
  application's code. `doc/curves.adoc` and
  `doc/secp256k1.adoc`'s opening list every function of the namespace by module, the
  `curves` namespace row's "What it does" becomes "BIP 340 Schnorr and ECDSA signatures,
  recoverable signatures, ECDH in three forms, tweaks, ElligatorSwift, MuSig2, silent
  payments, SPKI and PKCS#8", and its "Not in this version" keeps only BIP 32's derivation,
  as a domain library's ({bip32}). `llms.txt` is regenerated.
- [ ] **Step 5: Green** is the CI's verdict on both pull requests, every lane it plans: the
  ordinary lanes (native clang, gcc-15, MSVC 14.3 and 14.5, Apple clang, wasip2, wasip3,
  emscripten), the own lanes `c-suites`, `constant-time` and `sizes`, the docs
  (`generate.py --check`), the lint (`lint: clean`; `meta/include-boundaries.json` holds: no
  libsecp256k1 code enters a `detail/` header), and the tools job
  (`third_party_test.py`, `crypto_jamfiles_test.py`). A lane that fails, flakes or warns is
  fixed on the branch, whatever caused it.
- [ ] **Step 6: The review**, against this increment's rules above and the requirements it
  meets: every libsecp256k1 `ARG_CHECK` reachable from a public function has a refusal test;
  every secret output is a `secret<N>`; nothing allocates (the counting test over every new
  function); every key pair taken by a module function answers `invalid_state` when moved
  from; nothing of D24's protocol list entered (`grep -rn -i "taptweak\|taproot_output_key\|from_lax_der\|bech32\|bip32" include/`
  prints nothing); each message has a test of its own; the documentation's facts cite their
  sources, and every application a teaching section names has a cited public source showing
  its use of noble or scure (ruling Q2); no secp256k1 header still holds
  `WEBCPP_CRYPTO_DETAIL_SECP256K1_ONLY` (ruling Q3). Fixes come with a failing test first, on
  the same branches. Then
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing for each commit.
- [ ] **Step 7: The pull requests** (the controller opens them; the owner merges): the
  proposals branch `crypto/foundation-increment-2` (these tasks, reviewed locally before the
  code), the library's into `webcpporg/crypto`, then the superproject's (`third_party/Jamfile`,
  `tools/test/third_party_test.py`, the rulebook, the library's pin), rebased onto
  `origin/main`, all opened together when stream B's code is complete (the controller's
  ruling of 2026-11-17). Commit (library) `crypto: increment 2's families, constant-time cases, sizes and teaching sections`;
  (superproject) `guidelines: crypto's increment 2 in chapter 17`.

---

# Increments 3 to 12 (outlined; each is written in full task form when it starts)

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
   **Built ahead as part A (below, 2026-11-20):** the post over synchronous backends, the
   completion shape, the seam `detail::async_engine`, and the heavy machinery; part B adds
   `async_engine<backends::browser>` and the functions R-A1 leaves to it.
3. The errors of rejected promises, cancellation, run-time availability (node with an
   algorithm removed).
4. The lane `browser`: node and Chrome, every algorithm WebCrypto has against every synchronous
   backend on `main` (D13), and the audio worklet's `not_supported`.
5. Pages, green, review, pull requests.

## Increment 4, part A: the asynchronous family over the synchronous backends (written in full on 2026-11-20)

Written on 2026-11-20 on the library's tree with the changes `data-independent-timing` and
`api-gaps` (`../../api-gaps/`) merged, which it requires; the owner's queue of that day puts it
after `api-gaps` and after increment 3's P-256 (stream 3). Its rulings are recorded below and
in `../design.md` (D13 and D23, amended 2026-11-20), `../specs/crypto/spec.md` and
`roadmap.md`. Part B, the browser backend, is this increment's steps 1, 3 and 4 above, unchanged
but for the shape part A fixes.

**Goal:** `<webcpp/crypto/async.hpp>`, webcpp::hash's asynchronous family's twin, for every
algorithm WebCrypto offers that crypto has on `main` (AES-GCM; Ed25519; X25519; and P-256's
ECDSA and ECDH when increment 3 has merged), completing through a post on every target, and the
heavy operations' machinery rebuilt on hash's model, so that increments 3, 6, 7, 8, 9 and 10
declare their `async_` variants in one line each and part B plugs WebCrypto in behind the same
functions.

**Architecture:** two kinds, as hash's `async.hpp`. The cheap functions take the executor the
completion is posted to first, run the synchronous call at once (its guards and its work) and
post the completion, `void(boost::system::error_code, T)` or `void(boost::system::error_code)`,
never inside the call; a seam, `detail::asynchronous_backend_v<Backend>` and
`detail::async_engine<Backend>`, is where part B's `backends::browser` answers instead. The heavy
machinery, `detail::run_on_work`, takes the work executor first, checks the guards on the
caller's thread, runs the work on the work executor and completes on the handler's associated
executor, with hash's cancellation rules. Everything is on webcpp::asio (`<webcpp/asio/*.hpp>`,
`/webcpp/asio//asio`), never on Boost.Asio's headers directly.

**Tech stack:** as increment 1, with webcpp::asio and, in one test, webcpp::cobalt.

### Rulings of 2026-11-20 (the owner on autopilot)

- **R-A1. Part A now, part B later.** What needs no browser is built now: the family's
  functions over the synchronous backends, their completion-token shape, the seam part B fills,
  and the heavy machinery (natively and on Emscripten with `-pthread`, as hash's). Waiting for
  part B: `backends::browser`, its key table and handles, `async_engine<backends::browser>`, the
  DOMException mapping and the promise's cancellation (spec "Asynchronous family: the browser's
  errors"), `async_encrypt`, `async_decrypt`, `async_wrap`, `async_unwrap`, `async_export` and
  `async_make` from SPKI or PKCS#8 (their shape depends on the browser's non-extractable keys,
  and crypto has none of AES-CBC, AES-CTR, AES-KW or RSA on `main`), the lane `browser`, the
  audio worklet's `not_supported`, Chrome's throughput page, and the completion on
  `webcpp::asio::browser::main_thread_context`.
- **R-A2. Executor first, as hash's.** Every cheap function takes the executor its completion is
  posted to as its first parameter (`async_seal(executor, aead, iv, ad, data, token)`), every
  heavy one the work executor. Without it, Asio would post to the system executor, a thread of
  its own, and a handler would run off the program's loop; hash's family takes it for that
  reason. The spec's scenario "One coroutine, two targets" is amended to pass it.
- **R-A3. Results as `void(error_code, T)`.** As the spec says and as every Asio operation does;
  hash's functions write into the caller's spans and complete with `void(error_code)`, crypto's
  return values, so the value is the second argument. On an error `T` is the empty object a
  move leaves (a `gcm` or key pair without a key, a `secret<N>` or array of zeros), made through
  `detail::async_access`, a friend of each such type; a program that reads it gets
  `invalid_state` from every member. Operations without a value (`open`, `verify`) complete
  with `void(error_code)`.
- **R-A4. The default is the synchronous one, everywhere.** `default_async_backend_t<Family>` is
  `default_backend_t<Family>` on every target, Emscripten included; `backends::browser` is used
  only where a type names it, as hash's owner decided for `backends::webcrypto` (hash's ruling
  R76, decision O2): one program gives the same bytes on every target, and the browser's key
  model is chosen, not inherited. This amends the roadmap's last default row.
- **R-A5. The heavy machinery is hash's model, ported.** crypto's `detail::run_heavy` of Task 12
  completes with `void(R)` and has no cancellation; it is replaced by hash's `heavy_state`,
  `heavy_job` and `heavy_initiation` (`libs/hash/include/webcpp/hash/async.hpp`), carrying a
  value: a cancellation before the work starts completes at once with `operation_aborted`
  (every type); after it starts, `terminal` and `partial` let it end, destroy its value (which
  wipes a `secret<N>` or a key pair) and complete with `operation_aborted`; `total` is not
  honoured. The code is ported, not shared: crypto does not reach into hash's `detail`, and
  lifting the state into webcpp::asio is a later change of that library when a third one needs
  it (recorded here as a follow-up).
- **R-A6. No heavy operation exists yet.** The stated list of D32 holds none that `main` has;
  part A tests the machinery with a test-only heavy operation, and each later increment declares
  its own with `WEBCPP_CRYPTO_DETAIL_REQUIRE_THREADS` and `detail::run_on_work`.
- **R-A7. The timing guard.** Every `async_` function holds `detail::timing_guard` as its first
  statement (the change `data-independent-timing`'s rule R1, forwarders included), and the
  synchronous call it makes holds its own: for a cheap function both run on the caller's thread;
  for a heavy one the synchronous call's guard runs on the pool's thread, where DIT matters. The
  coverage tests call every `async_` function of their backend.
- **R-A8. A test that waits for a timer never races.** Each test that shows the loop going on
  while work runs keeps the work from completing until the timer's handler has run (a latch, as
  hash's `async_heavy_test.cpp` after its fix of 2026-11-20), never by timing; the examples
  likewise.

### Global Constraints

The foundation's (above), with: `async.hpp` and `detail/heavy.hpp` include only
`<webcpp/asio/*.hpp>` for Asio; every test of the family links `/webcpp/asio//asio`; the
convenience header `crypto.hpp` does not include `async.hpp`; webcpp's code calls neither
`operator new` nor the C allocator in the family (the handler's associated allocator holds every
state); every new public function holds the timing guard and is in its backend's coverage test.

### Review Focus

1. **A handler bound to another executor** (`bind_executor(loop, ...)` with a pool's executor
   given): the completion runs on the loop. Pinned by Task 4.A2's `post_bound_executor`.
2. **The object or the spans destroyed before completion.** On a synchronous backend the work
   runs before the call returns, so this is harmless today, and part B's browser reads them
   later: the documentation says they are the caller's until completion; Task 4.A6's page
   states it and Task 4.A2's `post_never_inline` proves the completion is later.
3. **A cancellation emitted from another thread** or a signal destroyed before its handler:
   Task 4.A1's `heavy_cancel_before_start`, `heavy_cancel_while_running` and
   `heavy_signal_outlives`.
4. **`use_future` in a build without exceptions:** the family compiles there with callbacks and
   `as_tuple`; Task 4.A2's test runs in both builds (`crypto-run`'s twins).
5. **An error completion's value read by mistake:** the empty object refuses every member with
   `invalid_state`. Task 4.A3's `async_make_refused` reads it.

### Task 4.A1: the heavy machinery on hash's model

**Files:**
- Modify: `include/webcpp/crypto/detail/heavy.hpp` (replace `run_heavy` and `carrying` by the
  port of hash's `heavy_phase`, `heavy_state`, `heavy_job`, `heavy_initiation`, `run_on_work`;
  `WEBCPP_CRYPTO_DETAIL_REQUIRE_THREADS` kept with its text)
- Create: `include/webcpp/crypto/detail/async_access.hpp`
- Modify: `test/heavy_test.cpp` (rewritten), `test/heavy_refusal_forgotten_test.cpp` (names
  `run_on_work`), `test/messages/heavy_without_threads*/`, `test/Jamfile` (`heavy` gains
  `<library>/webcpp/asio//asio` and the target `emscripten` with
  `<toolset>emscripten:<linkflags>-sPTHREAD_POOL_SIZE=3`, as hash's `async_heavy`)

**Interfaces:**
- Produces, in `webcpp::crypto::detail`:

```cpp
// Empty objects for error completions (R-A3); each returned type befriends it.
struct async_access {
    template <class T>
    [[nodiscard]] static T empty() noexcept;  // T's private default constructor, or T{}
};

// The completion signature of a synchronous result: void(error_code, T) or void(error_code).
template <class Result>
struct completion_of;  // completion_of<result<T>>::type, completion_of<result<void>>::type

#ifdef WEBCPP_CRYPTO_HAS_THREADS
// Checks guards() on the caller's thread (a refusal is posted to the handler's associated
// executor with its error and the empty value); then runs run() on `work`, completing on the
// handler's associated executor with run()'s result split into (error_code, value).
template <class CompletionToken, class WorkExecutor, class Guards, class Work>
auto run_on_work(WorkExecutor const& work, Guards guards, Work run, CompletionToken& token);
#endif
```

  `Guards` returns `boost::system::result<void>`; `Work` returns `boost::system::result<T>` or
  `result<void>`, is `noexcept`, and is called once.

- [ ] **Step 1: The failing tests,** `heavy_test.cpp`, on a test-only heavy operation
  `async_slow(work, value, token)` whose work returns `result<crypto::secret<32>>` (bytes all
  `value`) or `invalid_input_size` for `value == 0` as its guard:
  - `heavy_runs_on_pool`: the work's thread id differs from the caller's; the completion runs on
    the caller's `io_context` thread (bound with `bind_executor`).
  - `heavy_loop_goes_on`: a latch holds the work until a 1 ms timer on the loop fired (R-A8);
    the completion follows the timer.
  - `heavy_guard_refused`: `value == 0` completes with `invalid_input_size` and an all-zero
    secret, without the pool running anything (a pool of one thread held by a latch).
  - `heavy_cancel_before_start`: the pool's one thread held; a `terminal` cancellation emitted
    on the loop completes at once with `operation_aborted`; the work never runs.
  - `heavy_cancel_while_running`: the work started (latched); `partial` emitted; the work ends;
    the completion is `operation_aborted`, the value empty; `total` emitted the same way is
    ignored (the value arrives).
  - `heavy_signal_outlives`: the signal declared before the loop and the pool, as hash's page
    requires; destroying the pool with work queued destroys the handler unrun.
  - `heavy_tokens`: callback, `use_future` (exceptions build), `deferred`, `as_tuple`.
  - `heavy_allocator`: with a counting allocator bound by `bind_allocator`, every allocation of
    the operation goes through it and is freed before the handler runs; `operator new` is not
    called (a replaced global `operator new` counts).
  Natively and on Emscripten with `-pthread` under node.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//heavy`; expected: compile errors naming
  `run_on_work`.
- [ ] **Step 3: Implement** the port: hash's code with `webcpp::hash` names replaced, the
  output span replaced by the value carried in the state (`std::optional<T>` emplaced by the
  job, destroyed on an aborted completion), the completion through `completion_of`. Doc
  comments carry hash's text adapted, and cite hash's file as the model.
- [ ] **Step 4: Run** `heavy`, `heavy_refusal_forgotten`, `heavy_without_threads`,
  `heavy_without_threads_first` on native (clang, gcc-15), Emscripten with and without
  `-pthread`, wasip2 and wasip3 (the refusal only); expected: PASS.
- [ ] **Step 5: Commit** `crypto: the heavy operations' machinery on webcpp::hash's model, with
  cancellation before and during the work and values that wipe themselves`.

### Task 4.A2: `async.hpp`'s base, the post and the browser's seam

**Files:**
- Create: `include/webcpp/crypto/async.hpp`
- Create: `test/async_post_test.cpp`; modify `test/Jamfile` (`crypto-run async_post :
  async_post_test.cpp : <library>/webcpp/asio//asio <library>/webcpp/crypto//gcm-default
  <library>/third_party//sodium ;` on every target), the headers-alone block (`crypto/async.hpp`
  with `/webcpp/asio//asio`)
- Modify: `test/convenience_header_test.cpp` (asserts `WEBCPP_CRYPTO_ASYNC_HPP` undefined after
  `crypto.hpp`)

**Interfaces:**
- Produces, in `webcpp::crypto`:

```cpp
template <class Family>
using default_async_backend_t = default_backend_t<Family>;  // R-A4

namespace detail {
template <class Backend>
inline constexpr bool asynchronous_backend_v = false;  // part B: true for backends::browser
template <class Backend>
struct async_engine;  // part B specialises it for backends::browser

// Runs operation() at once and posts the completion, split by completion_of, to `executor`,
// whence it reaches the handler's associated executor, in its associated allocator.
template <class CompletionToken, class Executor, class Operation>
auto complete_by_post(Executor const& executor, CompletionToken& token, Operation operation);
}  // namespace detail
```

- [ ] **Step 1: The failing tests,** `async_post_test.cpp`, through Task 4.A3's `async_seal`
  and Task 4.A4's `async_verify` once they exist, and first through `complete_by_post` on a
  lambda: `post_never_inline` (a flag set by the callback is unset when the call returns, set
  after `run`), `post_bound_executor` (bound to a second `io_context`, the completion runs
  there), `post_allocator` (the post allocates through the bound allocator only),
  `post_tokens` (callback, `use_future` where exceptions are on, `deferred`, `as_tuple`,
  `use_awaitable` in a coroutine), and `post_error_value` (an error completes with the empty
  value).
- [ ] **Step 2: Run** `$B2 libs/crypto/test//async_post`; expected: FAIL (no header).
- [ ] **Step 3: Implement** `async.hpp`'s base: licence, the header comment (hash's, adapted:
  the two kinds, the default, the Jamfile line, an example), includes of `config.hpp`,
  `detail/heavy.hpp`, `detail/async_access.hpp`, the families' headers of Tasks 4.A3 and 4.A4,
  and the webcpp::asio headers hash's includes.
- [ ] **Step 4: Run** on all five lanes; expected: PASS.
- [ ] **Step 5: Commit** `crypto: async.hpp's base, completions posted on every target, and the
  seam the browser backend fills`.

### Task 4.A3: AES-GCM's asynchronous functions

**Files:**
- Modify: `include/webcpp/crypto/async.hpp`; `include/webcpp/crypto/ciphers/gcm.hpp`
  (`friend struct detail::async_access;`)
- Create: `test/async_gcm_test.cpp`; `test/messages/async_seal_xchacha/` and its MSVC fixture;
  modify `test/Jamfile`

**Interfaces:**
- Produces:

```cpp
// Algorithm: a gcm<...> (Task 4.A4 adds the Ed25519 key pair). void(error_code, Algorithm).
template <class Algorithm, class Executor, class CompletionToken>
auto async_make(Executor const& executor, std::span<std::uint8_t const> key_bytes,
                CompletionToken&& token);
// void(error_code, typename Gcm::result_type)
template <class Gcm, class Executor, class CompletionToken>
auto async_seal(Executor const& executor, Gcm& aead, std::span<std::uint8_t const> iv_bytes,
                std::span<std::uint8_t const> additional_data_bytes,
                std::span<std::uint8_t> data_bytes, CompletionToken&& token);
// void(error_code)
template <class Gcm, class Executor, class CompletionToken>
auto async_open(Executor const& executor, Gcm& aead, std::span<std::uint8_t const> iv_bytes,
                std::span<std::uint8_t const> additional_data_bytes,
                std::span<std::uint8_t> data_bytes, std::span<std::uint8_t const> tag_bytes,
                CompletionToken&& token);
```

  Each refuses at compile time a family WebCrypto lacks: "webcpp::crypto: async_seal takes
  AES-GCM, the AEAD WebCrypto has: call seal for the others." (and `async_open`'s likewise).

- [ ] **Step 1: The failing tests,** `async_gcm_test.cpp` on `gcm<aes256<B>>` for each AES-GCM
  backend the target links: the 18 specification cases through `async_make`, `async_seal` and
  `async_open` give the synchronous answers byte for byte; a wrong tag completes with
  `authentication_failed` and zeroed data; a 129-byte IV with `invalid_iv_size` and data
  untouched; `async_make_refused` (a 31-byte key) completes with `invalid_key_size` and an empty
  `gcm` whose `seal` returns `invalid_state`. The message test: `async_seal` on
  `ciphers::xchacha20_poly1305<>` stops with the text above, while its `seal` compiles.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//async_gcm`; expected: FAIL.
- [ ] **Step 3: Implement** the three, each `detail::timing_guard const timing;` first, then
  `if constexpr (detail::asynchronous_backend_v<backend>)` to `async_engine`, else
  `detail::complete_by_post` over the synchronous member.
- [ ] **Step 4: Coverage:** `test/support/timing_coverage.hpp` gains `cover_async_gcm<Gcm>(family,
  backend)` (an `io_context` run after each call; the first guard is `async_make`'s,
  `async_seal`'s, `async_open`'s), called by `timing_coverage_bearssl_test.cpp` and
  `timing_coverage_openssl_test.cpp`, whose Jamfile targets gain `<library>/webcpp/asio//asio`.
- [ ] **Step 5: Run** on all five lanes and the MSVC fixture; expected: PASS.
- [ ] **Step 6: Commit** `crypto: AES-GCM's asynchronous make, seal and open, posted on every
  target`.

### Task 4.A4: Ed25519's and X25519's asynchronous functions

**Files:**
- Modify: `include/webcpp/crypto/async.hpp`; `include/webcpp/crypto/curves/ed25519.hpp`
  (`friend struct detail::async_access;` in `basic_keypair`)
- Create: `test/async_25519_test.cpp`; `test/messages/async_sign_secp256k1/` and its MSVC
  fixture; modify `test/Jamfile` (`<library>/third_party//sodium`, every target)
- Modify: `test/support/timing_coverage.hpp` (`cover_async_ed25519`, `cover_async_x25519`),
  `test/timing_coverage_sodium_test.cpp`

**Interfaces:**
- Produces (`async_make<curves::ed25519::basic_keypair<B>>` through Task 4.A3's template):

```cpp
// void(error_code, Keypair)
template <class Keypair = curves::ed25519::keypair, class Executor, class CompletionToken>
auto async_generate(Executor const& executor, CompletionToken&& token);
// void(error_code, std::array<std::uint8_t, 64>)
template <class Keypair, class Executor, class CompletionToken>
auto async_sign(Executor const& executor, Keypair const& key_pair,
                std::span<std::uint8_t const> message, CompletionToken&& token);
// void(error_code)
template <class Policy = curves::ed25519::strict,
          class Backend = default_async_backend_t<families::ed25519>, class Executor,
          class CompletionToken>
auto async_verify(Executor const& executor, std::span<std::uint8_t const> public_key,
                  std::span<std::uint8_t const> message, std::span<std::uint8_t const> signature,
                  CompletionToken&& token);
// void(error_code, secret<32>)
template <class Backend = default_async_backend_t<families::x25519>, class Executor,
          class CompletionToken>
auto async_agree(Executor const& executor, std::span<std::uint8_t const> secret_key,
                 std::span<std::uint8_t const> peer_public_key, CompletionToken&& token);
```

  `async_generate`, `async_make` and `async_sign` take the Ed25519 key pair (and P-256's, Task
  4.A5); a secp256k1 key pair stops with "webcpp::crypto: async_sign takes a key pair of an
  algorithm WebCrypto has (Ed25519, P-256): call the key pair's sign for the others."

- [ ] **Step 1: The failing tests,** `async_25519_test.cpp`: RFC 8032's test 1, 2 and 3 through
  `async_make` and `async_sign` equal the synchronous signatures, and `async_verify` accepts
  them; a changed byte completes `async_verify` with `invalid_signature`; `async_generate` gives
  a key pair whose signature verifies; RFC 7748's section 6.1 through `async_agree`; a peer key
  of low order completes with `weak_public_key` and a zero secret. The message test on
  `curves::secp256k1::keypair`.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//async_25519`; expected: FAIL.
- [ ] **Step 3: Implement** as Task 4.A3, each with its guard first.
- [ ] **Step 4: Coverage** in `timing_coverage_sodium_test.cpp`, as Task 4.A3's.
- [ ] **Step 5: Run** on all five lanes and the MSVC fixture; expected: PASS.
- [ ] **Step 6: Commit** `crypto: Ed25519's and X25519's asynchronous functions, posted on every
  target`.

### Task 4.A5: P-256's asynchronous functions (when increment 3's P-256 is on `main`)

Increment 3 runs before this part (the owner's queue of 2026-11-20). With its P-256 key pair
and its functions on `main`, `async_generate`, `async_make` (from the raw 32-byte scalar),
`async_sign`, `async_verify` and `async_agree` (the raw x coordinate, WebCrypto's `deriveBits`)
take its types through the same templates, named as increment 3 built them (its "As built");
`test/async_p256_test.cpp` runs Wycheproof's ECDSA P-256 SHA-256 file's first 50 valid cases and
its ECDH P-256 file's first 50 through the async functions against the synchronous answers on
BearSSL, on every target; the coverage is BearSSL's. If increment 3 has not merged when this
part starts, this task moves to increment 3's stream, which then adds it.

- [ ] **Step 1:** the test, failing; **Step 2:** the functions accept P-256's types; **Step
  3:** the coverage; **Step 4:** five lanes; **Step 5:** commit `crypto: P-256's asynchronous
  functions, posted on every target`.

### Task 4.A6: the page, the examples, the table and part A whole

**Files:**
- Create: `doc/async.adoc` (`[#crypto-async]` "Asynchronous calls", hash's chapter's form: the
  two kinds, the executor, the default, the completion's value on an error, the spans and the
  object kept until completion, cancellation of the heavy ones, `use_future` without
  exceptions, the storage, and "the browser's WebCrypto" as part B's section to come);
  `example/async_seal.cpp` and `.expected` (a loop seals a record and opens it, the handler
  printing the tag), `example/async_sign.cpp` and `.expected` (a coroutine with
  `use_awaitable` signs and verifies); modify `example/Jamfile`, `doc/crypto.adoc` (the
  include), `doc/tables/crypto.json` (an operation `async` with the functions and the header,
  backends per family as the table already says), regenerate
- Modify: `../tasks.md` boxes 4.A1 to 4.A6; `STATUS.md`

- [ ] **Step 1:** the page and the examples, the generated tables, `--check`, the doc build.
- [ ] **Step 2:** every lane, the lints, the MSVC lane (the foundation's Task 16's rules); the
  review against R-A1 to R-A8 and the Review Focus.
- [ ] **Step 3:** commit `crypto: the asynchronous family's page and examples`; the pull
  requests, merged; the CI once at the end of the owner's queue.

### Increment 4, part A: as built (2026-11-22)

Part A was built on the branch `crypto/async` of the library and the superproject (the
library's commits on its `main` at `e999052`, listed below), by the tasks above, each reviewed,
with a final review of the branch and one fix wave, then the lint and GCC 15 release fixes and
Task 4.A5. The code is the truth; where it differs from the text above, this subsection says so.

**The commits** of the library, oldest first:

- `f4db5b3` crypto: the heavy operations' machinery on webcpp::hash's model, with cancellation before and during the work and values that wipe themselves
- `9da0387` crypto: the heavy job's allocator is chosen lazily, with a type-erased work executor tested, and a heavy operation's value must move without throwing
- `4a7450b` crypto: async.hpp's base, completions posted on every target, and the seam the browser backend fills
- `8240676` crypto: AES-GCM's asynchronous make, seal and open, posted on every target
- `4828b3d` crypto: Ed25519's and X25519's asynchronous functions, posted on every target
- `f7c314d` crypto: refusal texts name the synchronous function to call, the coverage counter is generic, and the review's minors of 4.A3 and 4.A4 are closed
- `27d53a5` crypto: the asynchronous family's page and examples
- `3d567f5` crypto: the asynchronous family's tables list each backend's own functions, with use cases, and the sign example reports a missing public key
- `68f2976` crypto: the asynchronous family's remaining wording matches its behavior, a const AES-GCM object gets the constness error, and crypto.hpp names the header it leaves out
- `0ba6463` crypto: the asynchronous family's lint findings are fixed at their cause, and the changelog's first line fits its word limit
- `ee0747e` crypto: the asynchronous tests and the sign example build warning-free on GCC 15 release, with the counting operators never inlined and each awaited result named before it is bound
- `a3710d3` crypto: merge P-256's branch into the asynchronous family, ahead of its landing on main
- `973913b` crypto: P-256's asynchronous functions, posted on every target
- `f89b0d9` crypto: llms.txt gives P-256's asynchronous functions their own briefs and use cases, and the P-256 chapter and the seam name the message the async functions hash

**The public API** (`include/webcpp/crypto/async.hpp`; every function takes the executor its
completion is posted to first and the completion token last; a failure is a value of the
completion, never a throw; the spans and the objects are the caller's until the completion):

| Function | Completion signature |
|---|---|
| `async_make<Algorithm>(executor, key_bytes, token)`, AES-GCM or an Ed25519 key pair | `void(error_code, Algorithm)`; on an error the empty object, every member of which answers `invalid_state` |
| `async_generate<Keypair = curves::ed25519::keypair>(executor, token)` | `void(error_code, Keypair)`; empty on an error |
| `async_seal(executor, aead, iv, additional_data, data, token)` | `void(error_code, Gcm::result_type)`, the tag; zeroed on an error |
| `async_open(executor, aead, iv, additional_data, data, tag, token)` | `void(error_code)`; the plaintext in `data` on success, zeroed on `authentication_failed` or a backend's failure, untouched on a guard's refusal |
| `async_sign(executor, key_pair, message, token)`, an Ed25519 key pair | `void(error_code, std::array<std::uint8_t, 64>)`; zeros on an error |
| `async_verify<Policy = curves::ed25519::strict, Backend = default_async_backend_t<families::ed25519>>(executor, public_key, message, signature, token)` | `void(error_code)` |
| `async_agree<Backend = default_async_backend_t<families::x25519>>(executor, secret_key, peer_public_key, token)` | `void(error_code, secret<32>)`; zeros on an error |

`default_async_backend_t<Family>` is `default_backend_t<Family>` on every target (R-A4). The seam
for part B is `detail::asynchronous_backend_v<Backend>` (false for every backend now) and the
declared-only `detail::async_engine<Backend>`, whose member functions and their argument order
`async.hpp` documents. The heavy machinery is `detail/heavy.hpp`'s `detail::run_on_work` with
`WEBCPP_CRYPTO_DETAIL_REQUIRE_THREADS`; no heavy operation exists yet, so only the test-only
operation in `test/heavy_test.cpp` uses it (R-A6). `detail/async_access.hpp` holds
`async_access`, the friend that builds the empty objects.

**Departures from the plan's text:**

- **The heavy job's allocator is the work executor's, else `std::allocator<void>`** (Task 4.A1),
  where webcpp::hash's `heavy_job` always uses `std::allocator<void>`. The choice is lazy
  (`heavy_job_allocator_of<WorkExecutor>`), so that an executor that cannot answer the allocator
  query, such as a type-erased `asio::any_io_executor`, names nothing ill-formed; the test
  `heavy_allocator` pins that the completed path calls no `operator new` (the cases with a
  connected cancellation slot do not, since Asio's own signal storage allocates).
- **A heavy operation's value must move without throwing** (a `static_assert`), so that the
  state moves it into the completion on every path; a `secret<N>` or a key pair does.
- **`async_gcm` is two targets, `async_gcm` (BearSSL) and `async_gcm_openssl`**, running the same
  test: one target cannot link OpenSSL on Emscripten, where only BearSSL is built.
- **The refusal text of `async_sign` says "call the key pair's synchronous signing function for
  the others"**, not the plan's "call the key pair's sign": secp256k1's key pair has no `sign`,
  only `sign_ecdsa` and `sign_schnorr`. `async_make` and `async_generate` have refusals of their
  own ("call the synchronous make" and "call the synchronous generate"), which the plan did not
  give; `async_seal` and `async_open` keep the plan's texts.
- **Two message tests more than planned**, `async_open_xchacha` and `async_make_xchacha`, each
  with its MSVC fixture, beside `async_seal_xchacha` and `async_sign_secp256k1`.
- **A const AES-GCM object** passed to `async_seal` or `async_open` reaches the non-const member's
  own constness error, not the refusal for a non-GCM type.
- **The capability table's asynchronous operations are split per family**: `async_gcm`,
  `async_ed25519` and `async_x25519` in `doc/tables/crypto.json`, not the one operation `async`
  of Task 4.A6's text, so that each generated backends page lists its own backends; the use cases
  `async_seal` and `async_sign` answer them.
- **The changelog line** for the family was folded into the first Unreleased line (version 0.1's
  entry is at the ten-line cap), as the earlier changes did.
- **The test's counter** `async_gcm_completion` became the generic `async_completion_counter`.
- **Emscripten builds run with `EM_CACHE` unset**: the read-only emsdk cache lacks the
  multi-threaded `libc++` variant and cannot build it, so the build uses the writable default
  cache.
- **The superproject's fixture** `heavy_without_threads_on_emscripten` names `run_on_work`,
  matching the library's message tests.

**What the tests pin beyond the plan's list:** `async_destroyed` in `async_gcm_test.cpp` and
`async_25519_test.cpp` (the object, the key pair and the spans destroyed after the initiating
call and before the completion: the results are right); the allocation counts of the cheap
functions' handlers; `use_future` where exceptions are on, `as_tuple` where they are off.

**Follow-ups (R-A5 and the final review):**

- Lift the heavy state, `heavy_state`, `heavy_job` and `heavy_initiation`, into webcpp::asio when
  a third library needs it; crypto and webcpp::hash each hold a port now.
- webcpp::hash's `heavy_job` could take the work executor's allocator, as crypto's does.
- When increment 3's P-256 lands on `main`, merge `main` into `crypto/async` once more and
  regenerate the generated files (never keep the auto-merge's). The branch carries P-256's
  library branch as merged before its landing (see Task 4.A5 below).

**The final review's fixes** (`68f2976`; seven Minor findings, six fixed and one left to Task
4.A5): the file comment of `async.hpp` and the `@tparam` of `async_sign` point to the key pair's
own signing function (`sign_ecdsa`, `sign_schnorr`); the page says the synchronous call runs when
the operation is initiated (the call, the `co_await` or the launch of a deferred operation); the
page no longer quotes a refusal text; `crypto.hpp` says it leaves out `async.hpp` and why
(webcpp::asio); `async_seal` and `async_open` test `is_gcm_v` of the type without its
cv-qualifiers, so a const object reaches the non-const member's own error; the allocation pitfall
adds the OpenSSL qualifier of `make` with a link to the memory guide.

**The lint fixes** (`0ba6463`, from the CI's Linux lint of `3d567f5`: 36 clang-tidy lines, a
fluent chain and a changelog line over its word limit), each at its cause: `post_initiation`'s
delivery is two `deliver` overloads in `post_initiation` (a `result<void>` by const reference, a
`result<T>` by rvalue), so no variable is left that could be const; the tests that covered every
token kind in one function are split per kind (`post_tokens` and `post_gcm_tokens` into future,
deferred, tuple and awaitable tests); `timing_coverage.hpp`'s `completions` has a default member
initializer and designated initializers; `example/async_sign.cpp` binds the public key's error to
a named variable; the sign example's hex digit is `std::uint32_t`; an unused alias is removed.
The changelog's first Unreleased line was shortened to 30 words or fewer, which dropped its
platform list and thread detail, the asynchronous functions joining it (ten lines is the cap);
Task 4.A5 restored the platform list within the limit. The clang-tidy run of the fixes used the
lint's own options on just the touched units, the next full Linux lint of the tree was clean,
and so were the macOS lint, MSVC 14.3 and 14.5 (no new failures) and every lane in debug and
release but one, the next item.

**The GCC 15 release fixes** (`ee0747e`; `-Wall -Wextra -Werror`, which no release GCC build had
run over the asynchronous tests):

- `-Werror=mismatched-new-delete` in `test/support/counting.hpp`: at `-O3` GCC 15 inlines the
  header's replaced `operator delete` (`free`) into a caller that has inlined a library's
  `operator new`, and pairs them wrongly. The replaced operators are now `BOOST_NOINLINE`, as
  `libs/pratt` and `libs/idna` mark theirs for the same warning; no pragma, the counts are
  unchanged.
- `-Werror=maybe-uninitialized` in `async_gcm_test`, `async_25519_test`, `async_post_test` and
  `example/async_sign`: a structured binding that initializes from a `co_await` of a
  value-carrying completion makes the hidden tuple a coroutine-frame temporary that GCC reports
  as possibly uninitialized, for the types whose move leaves a wiped object. This is a GCC
  coroutine false positive. Each awaited result is named first and bound by reference after
  (`auto made_result = co_await ...; auto& [code, key] = made_result;`), with no suppression.

#### Task 4.A5 as built: P-256's asynchronous functions

**The merge.** P-256's library branch was not on `main` when the task ran, and its pull request
was final-gated, so the branch was merged into `crypto/async` as it stood (`a3710d3`, the merge
of the P-256 pull request's branch after its own merge of `main`), and `main` is merged again
once P-256 lands. Two conflicts, both sides kept: `test/Jamfile`'s `headers-alone` lists
(`detail/heavy.hpp` moved to the Asio list with `async.hpp`, `detail/async_access.hpp` added,
and P-256's `detail/p_curves.hpp` in the first list) and `convenience_header_test.cpp` (the
header comment merged into one paragraph, both blocks kept). The generated files were
regenerated after the merge and equalled the auto-merged ones. P-256's superproject files and
its BearSSL pin are not in this stream: they arrive with `main`.

**The functions** (`973913b`, in `async.hpp`; P-256's key pair is `curves::p256::basic_keypair`):

| Function | Completion signature |
|---|---|
| `async_make<p256::basic_keypair<B>>(executor, secret_scalar, token)` and `async_generate<p256::basic_keypair<B>>(executor, token)`, through the existing templates | `void(error_code, p256::basic_keypair<B>)`; empty on an error (`invalid_key_size` for a scalar of another size, `invalid_secret_key` for zero or the group order) |
| `async_sign(executor, p256_key_pair, message, token)`, through the existing template | `void(error_code, std::array<std::uint8_t, 64>)`, the signature as `r \|\| s`; zeros on an error |
| `async_verify<Backend>(executor, p256::basic_public_key<Backend> const&, message, signature, token)`, an overload | `void(error_code)`; a signature of another size than 64 bytes completes with `invalid_signature` |
| `async_agree<Backend>(executor, p256::basic_keypair<Backend> const&, p256::basic_public_key<Backend> const&, token)`, an overload | `void(error_code, secret<32>)`, the raw x coordinate; zeros on an error |

`async_verify` and `async_agree` are overloads taking P-256's key types because those types
cannot pass through the span-based Ed25519 and X25519 templates; an explicit
`async_verify<Backend>` or `async_agree<Backend>` still works. Each function holds
`detail::timing_guard const timing;` first, then the seam's dispatch, else `complete_by_post`, as
the others do. `curves/p256.hpp`'s `basic_keypair` has `friend struct detail::async_access;`, so
an error completion's key pair is the empty one (every member answers `invalid_state`). The trait
`detail::is_p256_keypair_v` and `is_webcrypto_keypair_v` (Ed25519 or P-256) route the templates.
The refusal texts that name "(Ed25519, P-256)" are now true.

**The message, not the digest (the controller's ruling).** P-256's `async_sign` and
`async_verify` take the message and hash it with SHA-256, as WebCrypto's ECDSA with SHA-256
does, where the synchronous `sign_ecdsa` and `verify_ecdsa` take the digest. The reason: WebCrypto
can neither sign nor verify a prehashed digest, so a digest-taking asynchronous function could
never be served by part B's `backends::browser`; the asynchronous family's contract is
WebCrypto's shape. The hash is `detail::p256_digest`, webcpp::hash's `sha2_256` through
Boost.Hash2 (`<webcpp/hash/algorithms.hpp>` and `<boost/hash2/digest.hpp>`, header-only;
`<webcpp/hash/digest.hpp>` pulled backend headers into the doc build's aggregate translation
unit and was not used). `async_engine`'s documentation says that P-256's sign and verify hand the
engine the message, which it hashes with SHA-256, and lists the P-256 shapes of `verify` and
`agree`; the P-256 chapter says the same. If the owner prefers the digest, the change is local to
two lambdas and the documentation.

**The tests** (`test/async_p256_test.cpp`, target `async_p256`, every target; the helpers are
P-256's own): `async_rfc6979` (RFC 6979 A.2.5, "sample" and "test", through `async_make` and
`async_sign`, equal to the RFC and to `sign_ecdsa` of the digest; `async_verify` accepts);
`async_wycheproof_ecdsa` (the first 50 valid cases of `ecdsa_secp256r1_sha256_p1363_test`, the
`r || s` form that WebCrypto takes, `async_verify` against `verify_ecdsa`, a changed message
refused by both; the loop stops reading groups once 50 cases ran); `async_wycheproof_ecdh` (the
first 50 valid cases of `ecdh_secp256r1_ecpoint_test` through `async_make` and `async_agree`,
equal to the file's shared x and to `ecdh_x`); `async_verify_refused` (a changed byte, 63 bytes,
`r` = 0); `async_generate` (the default and a named backend); `async_make_refused` (31 bytes, the
order, the empty key pair's members and the functions on it); `async_destroyed`;
`async_coroutine`. The task started red (three `static_assert` refusals and no matching
function) and ended with 50 verified and 50 agreed.

**Coverage on BearSSL.** `cover_async_p256` in `timing_coverage_bearssl_test.cpp` runs
`async_make` (valid, a 31-byte scalar), `async_generate`, `async_sign` (valid, moved-from),
`async_verify` (valid, 63 bytes) and `async_agree` (valid, moved-from), nine completions; the
comment says `async_generate`, which has no input, is not refused. A mutation check removed the
guard of `async_agree`, and the test failed naming `ecdh_x`'s guard as the first entered.

**The page and the table.** `doc/async.adoc` has a P-256 row in the functions table, a paragraph
(the message hashed with SHA-256 as WebCrypto does, `r || s`, P-256's types, the raw x) and
`backends-async_p256.adoc`; `doc/p256.adoc`'s ECDSA and ECDH sections point to the asynchronous
functions. `doc/tables/crypto.json` has the operation `async_p256` (BearSSL, test `async_p256`),
beside `async_gcm`, `async_ed25519` and `async_x25519`; the message of `async_sign_secp256k1`
says only Ed25519's and P-256's key pairs have `async_sign`. Examples are unchanged.

**The generator change** (`f89b0d9`, `doc/tables/generate.py`, which the review found necessary:
`llms.txt` told a reader P-256's entry could "sign and verify with Ed25519"). Two optional
fields of `crypto.json`: a use case's `operations` lists the operation ids it answers, so the
asynchronous AES-GCM and Ed25519 use cases name `async_gcm` and `async_ed25519` and P-256's entry
no longer gets theirs (an unknown id is a fault); an operation's `overloads` maps a function to
the text its declaration must hold, so that the brief comes from the overload's own Doc Comment
(`async_p256` names `p256::basic_public_key` for `async_verify` and `p256::basic_keypair` for
`async_agree`; a selector no declaration matches is a fault naming it). `llms.txt` now shows
P-256's own briefs. No P-256 use case was added: the decision table requires every row a program
can follow to link an example, and there is no asynchronous P-256 example, so the P-256 entry has
no "Use it to" line rather than a false one. `generate_test.py` has three new cases for these.

**The review of the task** found one Important finding (the `llms.txt` one above) and four
Minor ones (the coverage comment, the pointers in `doc/p256.adoc`, the seam's documentation of
the message, the Wycheproof loop's bound), all fixed in `f89b0d9` and re-reviewed clean.

**What is not done.** The superproject's gitlink, with P-256's superproject files and the
BearSSL pin, arrives with `main`; the CI's first run of the family is at the end of the owner's
queue.

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
