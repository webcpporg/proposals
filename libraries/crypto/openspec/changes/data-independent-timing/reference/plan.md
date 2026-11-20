---
title: "webcpp::crypto, data-independent timing: Implementation Plan"
tags: [webcpp, crypto, dit, constant-time, plan]
status: accepted
created: 2026-11-20
---

# webcpp::crypto, data-independent timing: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** every public function of webcpp::crypto that runs a backend's cryptographic code, and
`equal`, runs its whole body with PSTATE.DIT set on 64-bit ARM under macOS, iOS, Linux and
Android where the CPU has FEAT_DIT, and returns with the thread's DIT as it found it.

**Architecture:** one RAII type, `detail::timing_guard` in `detail/timing.hpp`, is the first
statement of every guarded public function (design D1, D2). A seam,
`WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED()`, fires in its constructor on every target, and a
coverage test per backend calls every guarded function and checks that the seam fired (D3). The
constant-time lane stops setting DIT itself (D5); the guard's cost is measured by a release
program that `bench/collect.py` runs and renders (D6).

**Tech stack:** C++20 header-only, b2, Boost.Core's lightweight_test, the superproject's lanes
(native clang and gcc-15, wasip2, wasip3, Emscripten, MSVC 14.3 and 14.5 under Wine), Python 3
for `bench/collect.py`.

**Spec:** `../proposal.md`, `../design.md` (D1 to D7, binding), `../specs/crypto/spec.md`,
`../tasks.md`. Executors read all four.

Paths below are relative to the library, `libs/crypto/` of the superproject (its own repository
`webcpporg/crypto`), unless they start with `webcpp/` (the superproject) or `proposals/`.

## Global Constraints

- Everything written to a file, a commit or a pull request is in English; no em dash (U+2014)
  anywhere; the banned word of `CODING-GUIDELINES.md` chapter 2 nowhere.
- Commits: `git -c user.name=... commit -- <paths>` with explicit paths, subject `crypto: ...`
  as a descriptive sentence like the history, no trailer of any kind (`CODING-GUIDELINES.md`
  chapter 2).
- Every new source file opens with the licence notice of chapter 2; every public or detail
  symbol carries its doc comment (chapter 7); `.clang-format` and `.clang-tidy` clean.
- Never edit a generated file (`doc/generated/*`, `doc/llms.txt`, `detail/capabilities.hpp`,
  `detail/defaults.hpp`, `doc/performance.adoc`, `doc/performance.json`,
  `test/capabilities_test.cpp`): change the source and run its generator.
- Sizes and speed come from release builds only (`<variant>release`), never typed by hand.
- webcpp::hash is out of scope: nothing under `libs/hash/` changes. The public interface and
  the errors do not change.
- The register is named by its encoding, `s3_3_c4_c2_5`, DIT is bit 24, each write is followed
  by `isb` (D2). Linux's `HWCAP_DIT` is `1UL << 24` of `AT_HWCAP`; Apple's sysctl is
  `hw.optional.arm.FEAT_DIT`, an unreadable value counted as absent.
- The guard applies where `__aarch64__` and (`__APPLE__` or `__linux__`) are defined; it is an
  empty class with no instruction everywhere else (x86, WebAssembly, MSVC and clang-cl on
  Windows ARM64, D7).

## Rulings made while writing this plan (controller, 2026-11-20, owner on autopilot)

- **R1. Placement.** The guard is the first statement of every public function the inventory
  below lists, forwarders included (e.g. both `x25519::public_key` and
  `x25519::algorithm<B>::public_key`): one rule, checked mechanically, at the cost of one more
  `mrs` where a call nests. A public function that only forwards to a protected or private helper
  shared by several public functions (aes's `make_as`, kx's `session`, secp256k1's
  `encoded_public_key`, `signed_ecdsa`, `agree`, `public_key::compare`, musig's `tweaked`) holds
  its own guard too; the helper needs none.
- **R2. Nesting.** Guards nest by construction: a guard entered with DIT already set does
  nothing and leaves it set, so `generate` calling `make`, `decode_pkcs8` calling `make` and
  `encode_pkcs8`, `xonly_public_key` calling `full_public_key`, and backends calling `equal`
  are correct with no counter.
- **R3. GCM's moves are guarded.** The design excludes constructors that "only move or wipe
  bytes"; `gcm`'s move constructor and move assignment do more: on BearSSL `gcm_engine::move`
  runs `br_gcm_init` (an AES encryption under the key), and `abandon` runs `br_gcm_init` on
  BearSSL and an `EVP_CipherUpdate` on OpenSSL. They hold the guard and the coverage test calls
  them. Every other move, reset and destructor only copies, wipes or frees, and holds none.
- **R4. The boundary of "calls a backend".** Functions whose body makes no backend call are
  excluded even when they touch secret bytes: `ed25519::encode_spki`, `ed25519::encode_pkcs8`,
  `x25519::encode_spki`, `x25519::encode_pkcs8`, `kx_keypair::public_key` (copies). The
  `x25519` and `ed25519` `decode_*` functions are included (they call `equal`, constant-time
  code on the encoding of a secret). `ed25519::basic_keypair::public_key` is included (it calls
  libsodium, even though that call is a copy).
- **R5. secp256k1's tweak helpers.** The guard sits in each public function, before
  `secp256k1_selftest`, never only in `detail::secp256k1_tweaked_secret` or
  `secp256k1_tweaked_key`.
- **R6. User code inside a guard.** `silent_payments::scan_outputs` runs the program's
  `LabelLookup` inside the guard; this is accepted and documented on the page (the lookup runs
  with DIT set, which only makes it data-independent too). An exception thrown by
  `std::pmr::memory_resource::allocate` inside `secp256k1::basic_keypair::make` or
  `decode_pkcs8` unwinds through the guard, whose destructor restores the state: the guard is
  RAII and its destructor `noexcept`.
- **R7. The coverage test, one file per backend** (the library's rule that a backend's test is
  a `.cpp` of its own): `timing_coverage_{sodium,secp256k1,bearssl,openssl,ctaes}_test.cpp`,
  built where the backend is (OpenSSL: native, wasip2, wasip3), with exceptions on and off
  through `crypto-run`. `equal` is covered by the guard's own test, which links no backend.
- **R8. The guard's cost is measured by a program and rendered by `bench/collect.py`**, so
  that no number on a page is typed by hand: `bench/timing.cpp` (exe `timing`, release only,
  `explicit`) and its correctness twin `timing_check` run in every lane like
  `throughput_check`; `collect.py --timing <exe>` stores its results in `doc/performance.json`
  and renders them in `doc/performance.adoc`. `doc/backends.adoc` points there.
- **R9. The changelog holds ten lines already** (the limit of `AGENTS.md` chapter 5) and crypto
  is unreleased: the change is folded into the first line ("Added crypto, ...") as one clause,
  not an eleventh line.
- **R10. The constant-time lane's verdict.** The dudect programs print what the CPU offers and
  whether DIT is set on the measuring thread outside the library's calls (it must not be: the
  harness sets nothing); they no longer fail where the CPU has DIT and it is clear, since the
  library now sets it inside each measured call. `dudect_planted` keeps failing as before.

## Review Focus

1. **A thread whose DIT the program set itself** (Apple's `timingsafe_enable_if_supported`):
   every call must leave it set. Pinned by Task 1's `guard_leaves_set_bit` and Task 3's
   `sign_leaves_set_state`.
2. **An exception unwinding through a guard** (a `memory_resource` that throws in
   `secp256k1::basic_keypair::make`): DIT must be cleared on the way out. Pinned by Task 4's
   `throwing_resource_restores_state` (exceptions-on build; the `_no_exceptions` twin skips it).
3. **An error return before the backend is reached** (a wrong key size): the guard is still
   entered and still restores. Pinned by every coverage call that passes invalid input and by
   Task 1's `early_error_restores_state` on a mock engine.
4. **Many threads** (`gcm_openssl_threads`): DIT is per thread; a guard on one thread must not
   change another's. Pinned by Task 1's `other_thread_untouched` (native, `<threading>multi`).
5. **A CPU or system without DIT, and every non-arm64 target**: no trapping instruction, the
   seam still fires. Pinned by Task 1's tests running on every lane (wasip2, wasip3,
   Emscripten, MSVC, x86 Linux in CI) and Task 8's run in an arm64 Linux container.

---

## Inventory of guarded public functions (D1, R1 to R5)

Line numbers are those of `origin/main` at the change's start (crypto `fec4047`); the
implementer finds each by name. Every row gets `detail::timing_guard const guard;` (or the name
the implementer chooses, the same everywhere) as its first statement.

**`equal.hpp`:** `equal(span, span)`; `equal(digest<N>, digest<N>)`.

**`ciphers/aes.hpp`:** `basic_aes::encrypt`, `basic_aes::decrypt`, `aes128::make`,
`aes192::make`, `aes256::make`.

**`ciphers/gcm.hpp` (`gcm<Cipher>`):** `make`, the move constructor and move assignment (R3),
`seal`, `open`, `begin_seal`, `update`, `result`.

**`ciphers/aead.hpp` (`aead<Family, Backend>`, which serves chacha20_poly1305, its original
form, xchacha20_poly1305 and both secretboxes):** `make`, `seal`, `open`.

**`hybrid/box.hpp` (`basic_box`):** `make`, `seal`, `open`.
**`hybrid/sealed_box.hpp` (`basic_sealed_box`):** `seal`, `open`.
**`pem.hpp`:** `algorithm<B>::armour`, `algorithm<B>::unarmour`, free `armour`, free
`unarmour`.

**`curves/ed25519.hpp`:** `basic_keypair::make`, `generate`, `public_key`, `sign`,
`x25519_secret`; free `verify`, `to_x25519_public`; `ph::begin`, `ph::update`, `ph::sign`,
`ph::verify`.
**`curves/ed25519/formats.hpp`:** `decode_spki`, `decode_pkcs8`.
**`curves/x25519.hpp`:** `algorithm<B>::public_key`, `algorithm<B>::agree`, free `public_key`,
free `agree`.
**`curves/x25519/formats.hpp`:** `decode_spki`, `decode_pkcs8`.
**`curves/x25519/kx.hpp` (`kx_keypair`):** `make`, `generate`, `client_session_keys`,
`server_session_keys`.

**`curves/secp256k1.hpp`:** `operator==` and `operator<=>` of `public_key`;
`basic_keypair::make`, `xonly_public_key`, `public_key`, `public_key_uncompressed`,
`sign_schnorr`, both `sign_ecdsa`, `sign_recoverable`, `ecdh`, `ecdh_x`, `ecdh_point`,
`tweak_xonly`; free `parse_public_key`, `serialize_public_key`, `recover`, `verify_schnorr`,
`verify_ecdsa`, `to_der`, `from_der`, `normalize_ecdsa`, `tweak_xonly`.
**`curves/secp256k1/ellswift.hpp`:** `ellswift_encode`, `ellswift_decode`, `ellswift_create`,
`ellswift_xdh`.
**`curves/secp256k1/formats.hpp`:** `encode_spki`, `decode_spki`, `encode_pkcs8`,
`decode_pkcs8`.
**`curves/secp256k1/musig.hpp`:** `aggregate_key::xonly_public_key`, `full_public_key`,
`tweak_add`, `tweak_xonly_add`; free `key_aggregate`, `nonce_gen`, `nonce_aggregate`,
`nonce_process`, `partial_sign`, `partial_signature_verify`, `partial_signature_aggregate`,
`parse_public_nonce`, `serialize_public_nonce`, `parse_aggregate_nonce`,
`serialize_aggregate_nonce`, `parse_partial_signature`, `serialize_partial_signature`.
**`curves/secp256k1/silent_payments.hpp`:** `create_outputs`, `create_label`,
`create_labeled_spend_key`, `make_prevouts_summary`, `scan_outputs`.
**`curves/secp256k1/tweak.hpp`:** `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`,
`pubkey_tweak_add`, `pubkey_tweak_mul`, `pubkey_negate`, `pubkey_combine`.

**Not guarded (checked boundary):** `random_device` and `detail/entropy.hpp`; `secret<N>`;
`supports_v`, `default_backend_t` and everything `constexpr` (`pem::armoured_size`,
`musig::workspace_size`, the silent payments' and `combine_workspace_size`); every move,
reset and destructor except `gcm`'s moves (R3); the encoders of R4; `silent_payments::no_labels`;
`backends::openssl::install_memory_functions` (configuration); `make_error_code` and the
category; `detail::run_heavy`. Backends' own headers hold no guard (design approach 2 rejected).

---

### Task 1: The guard, its seam and its unit tests

**Files:**
- Create: `include/webcpp/crypto/detail/timing.hpp`
- Create: `test/timing_test.cpp` (no backend), `test/timing_mock_test.cpp` (a stand-in AEAD
  engine), `test/support/timing_seam.hpp`
- Modify: `test/Jamfile` (the two tests through `crypto-run`; `crypto/detail/timing.hpp` in the
  first `webcpp.headers-alone` list; `timing_threads` native only, `<threading>multi`)
- Modify: `include/webcpp/crypto/equal.hpp` (both overloads guarded)

**Interfaces:**
- Produces, in `namespace webcpp::crypto::detail`:
  - `[[nodiscard]] bool cpu_has_dit() noexcept`: read once per process (function-local
    static); `false` on every target the guard does not cover.
  - `[[nodiscard]] bool dit_is_set() noexcept`: reads PSTATE.DIT (`mrs`) where the CPU has DIT
    on a covered target; `false` elsewhere (never executes `mrs` on a CPU without DIT).
  - `class timing_guard`: `timing_guard() noexcept`, `~timing_guard()` (noexcept),
    copy and move deleted. Covered targets: holds `bool armed_`. Elsewhere: no data member.
  - The macro `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED()`, defined as `static_cast<void>(0)` unless
    defined before the first include (the pattern of `detail/declassify.hpp`), expanded as the
    constructor's first statement.
- Produces, in `test/support/timing_seam.hpp` (included before any crypto header):
  `namespace webcpp::test { inline thread_local std::size_t timing_entries = 0; }`, the macro
  defined as `(static_cast<void>(++::webcpp::test::timing_entries))`, and
  `template <class Call> void require_enters(char const* name, Call&& call)`, which records
  `timing_entries`, runs `call()`, and on no increase reports
  `"<name> did not enter detail::timing_guard"` through `BOOST_ERROR` with the name.

- [ ] **Step 1: Write the failing tests** in `test/timing_test.cpp` (includes
  `support/timing_seam.hpp` first, then `<webcpp/crypto/equal.hpp>` and
  `<webcpp/crypto/detail/timing.hpp>`), each a function called from `main`:
  - `seam_fires`: constructing a guard raises `timing_entries` by exactly 1, on every target.
  - `guard_sets_and_clears`: where `cpu_has_dit()`, clear DIT through a test-local `msr`
    helper first, then `BOOST_TEST(dit_is_set())` inside a guard and `BOOST_TEST(!dit_is_set())`
    after it; elsewhere `BOOST_TEST(!dit_is_set())` throughout.
  - `guard_leaves_set_bit`: where `cpu_has_dit()`, set DIT through the helper, enter and leave a
    guard, `BOOST_TEST(dit_is_set())`; clear it at the end.
  - `nested_guards`: from clear, two nested guards; set inside both and after the inner one,
    clear after the outer one.
  - `equal_enters`: `require_enters("equal(span, span)", ...)` and the digest overload.
  - `other_thread_untouched` (in `timing_threads`, the same source with
    `WEBCPP_TEST_TIMING_THREADS`): while a guard holds DIT on the main thread, a `std::thread`
    reads `dit_is_set() == false`.
  In `test/timing_mock_test.cpp`: a stand-in `aead_engine<families::chacha20_poly1305,
  test::timing_stand_in>` (as `aead_box_mock_test.cpp` declares one) whose `seal` records
  `dit_is_set()` into a global; tests `backend_runs_with_dit` (from clear, after `seal`, the
  recorded value equals `cpu_has_dit()`, and DIT is clear again), `early_error_restores_state`
  (`make` with a 31-byte key fails with `invalid_key_size`, DIT clear after, seam fired).
- [ ] **Step 2: Run them to see them fail:** `b2 libs/crypto/test//timing libs/crypto/test//timing_mock`
  (the target names `AGENTS.md` chapter 4 gives); expected: a compile error, `detail/timing.hpp`
  missing.
- [ ] **Step 3: Implement `detail/timing.hpp`** as the interfaces say: `<sys/sysctl.h>` on
  Apple arm64, `<sys/auxv.h>` on Linux arm64, nothing else included on other targets; the
  `mrs`/`msr` as in today's `test/constant_time/dudect.hpp` (`"msr s3_3_c4_c2_5, %0\n\tisb"`,
  `"mrs %0, s3_3_c4_c2_5"`, clobber `"memory"`); the destructor writes 0 and `isb` only when
  `armed_`. Its doc comments state D2's reasons (Apple's requirement, the encoding, the `isb`,
  why it does nothing when already set). Guard both `equal` overloads.
- [ ] **Step 4: Run them and see them pass** on native clang, then `b2 ... toolset=gcc-15`,
  wasip2, wasip3 and Emscripten (`AGENTS.md` chapter 4's commands for each target).
- [ ] **Step 5: Commit** `crypto: detail::timing_guard sets arm64's DIT for its scope and restores the thread's state, with a seam a test counts`.

### Task 2: The ciphers and the hybrid boxes hold the guard

**Files:** Modify `ciphers/aes.hpp`, `ciphers/gcm.hpp`, `ciphers/aead.hpp`, `hybrid/box.hpp`,
`hybrid/sealed_box.hpp`, `pem.hpp` (each includes `<webcpp/crypto/detail/timing.hpp>` after
`config.hpp`). Test: Task 5's coverage files for bearssl, openssl, ctaes and sodium (their
cipher, box and PEM parts are written in this task, first).

**Interfaces:** Consumes Task 1's `detail::timing_guard`, `support/timing_seam.hpp`'s
`require_enters`. Produces the cipher sections of `timing_coverage_{bearssl,openssl,ctaes,sodium}_test.cpp`.

- [ ] **Step 1: Write the failing coverage calls** for every row of the inventory under
  `ciphers/`, `hybrid/` and `pem.hpp`, one `require_enters("<family>::<function> on <backend>",
  ...)` per function and per backend that builds it (gcm: bearssl and openssl, each of
  aes128_gcm and aes256_gcm; aes block: openssl and ctaes, each of aes128/192/256; aead
  chacha20_poly1305: sodium, bearssl, openssl; the original form, xchacha, both secretboxes,
  both boxes, sealed_box and pem: sodium). Include the move constructor and move assignment of
  a keyed `gcm` and of a `gcm` in the middle of `begin_seal` (R3), and one call with invalid
  input per function family.
- [ ] **Step 2: Run** `b2 libs/crypto/test//timing_coverage_sodium` (and the other three):
  expected FAIL naming each unguarded function.
- [ ] **Step 3: Add the guard** as the first statement of each inventoried function in these
  headers.
- [ ] **Step 4: Run** the four coverage tests and the existing tests of these families
  (`gcm_*`, `aes_*`, `chacha_*`, `xchacha_sodium`, `secretbox_sodium`, `box_sodium`,
  `aead_box_mock`, `pem_mock`, the `agreement_*`): PASS.
- [ ] **Step 5: Commit** `crypto: the ciphers, the boxes and PEM set DIT around each operation on every backend`.

### Task 3: Ed25519, X25519, kx, their formats

**Files:** Modify `curves/ed25519.hpp`, `curves/ed25519/formats.hpp`, `curves/x25519.hpp`,
`curves/x25519/formats.hpp`, `curves/x25519/kx.hpp`. Test: the curve section of
`test/timing_coverage_sodium_test.cpp`; `test/timing_sodium_test.cpp` (new, `crypto-run` with
`<library>/third_party//sodium`).

**Interfaces:** Consumes Tasks 1 and 2 (the sodium coverage file exists).

- [ ] **Step 1: Write the failing tests:** `require_enters` for every inventoried function of
  these headers (R4: not the encoders), including `ph::begin/update/sign/verify` and
  `generate` (built with `WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY` only if `ed25519_sodium` needs it;
  otherwise the system entropy). In `timing_sodium_test.cpp`, the spec's two scenarios:
  `sign_clears_state` (DIT clear before `curves::ed25519::keypair<>::make(seed)->sign(msg)`,
  clear after, signature equal to RFC 8032's test 1) and `sign_leaves_set_state` (set before,
  set after); where the CPU has no DIT both reduce to `!dit_is_set()`.
- [ ] **Step 2: Run** them: FAIL naming the functions.
- [ ] **Step 3: Add the guards.**
- [ ] **Step 4: Run** the coverage and the existing `ed25519_*`, `x25519_*`, `kx_*`,
  `formats_25519_*`, `oracle_25519` (Emscripten) tests: PASS.
- [ ] **Step 5: Commit** `crypto: Ed25519, X25519, kx and their formats set DIT around each operation`.

### Task 4: secp256k1 and its groups

**Files:** Modify `curves/secp256k1.hpp` and `curves/secp256k1/{ellswift,formats,musig,silent_payments,tweak}.hpp`.
Test: `test/timing_coverage_secp256k1_test.cpp`.

- [ ] **Step 1: Write the failing tests:** `require_enters` for every inventoried secp256k1
  function, both `public_key` comparison operators, both `sign_ecdsa` overloads, all three
  ECDH forms, a whole MuSig2 round of two signers and a silent payments create and scan (as
  `secp256k1_musig_test.cpp` and `secp256k1_silent_payments_test.cpp` build them), each
  serialization and parse; and `throwing_resource_restores_state` (exceptions on only, under
  `#ifndef BOOST_NO_EXCEPTIONS` or the library's own macro): a `memory_resource` whose
  `do_allocate` throws, `basic_keypair::make` propagates the exception, DIT is as before.
- [ ] **Step 2: Run** it: FAIL naming the functions.
- [ ] **Step 3: Add the guards** (R5: before `secp256k1_selftest`).
- [ ] **Step 4: Run** it and every `secp256k1_*` test: PASS.
- [ ] **Step 5: Commit** `crypto: secp256k1 and its groups set DIT around each operation`.

### Task 5: The coverage test complete on every lane

**Files:** Modify `test/Jamfile` (the five `timing_coverage_*` through `crypto-run`, OpenSSL's
on `native wasip2 wasip3`); `doc/tables/crypto.json` only if `generate.py --check` asks for it.

- [ ] **Step 1: Check completeness:** every row of the inventory appears in exactly the
  coverage files of the backends that build it; write the list in a comment at the top of each
  file (the family and functions it covers), so that a later family extends it.
- [ ] **Step 2: Plant a miss:** remove one guard (e.g. `gcm::result`) locally, run
  `timing_coverage_bearssl`: FAIL naming `gcm::result on bearssl`; restore.
- [ ] **Step 3: Run** the five tests on native clang, gcc-15, wasip2, wasip3, Emscripten (no
  OpenSSL), release and debug: PASS; `python3 libs/crypto/doc/tables/generate.py --check`:
  clean.
- [ ] **Step 4: Commit** `crypto: a coverage test per backend proves every public operation enters the timing guard`.

### Task 6: The constant-time lane measures what a program gets

**Files:** Modify `test/constant_time/dudect.hpp` (remove `timing_mode`, `dit_support`,
`cpu_dit`, `data_independent_timing`, `enter_data_independent_timing`; add
`void report_data_independent_timing() noexcept` built on `detail::cpu_has_dit()` and
`detail::dit_is_set()`), `dudect_test.cpp`, `dudect_sodium_test.cpp`,
`dudect_secp256k1_test.cpp`, `dudect_planted_test.cpp` if it calls it, the lane's Jamfile
comment.

- [ ] **Step 1:** `report_data_independent_timing` prints one line: `DIT (data-independent
  timing): the CPU has it; the library sets it in each call, the harness does not` with
  `(clear on this thread outside the calls)`, or `not on this CPU`, or `not applicable here (not
  macOS or Linux on arm64)`; where the CPU has DIT and it is set on the thread outside the calls,
  the program fails (the harness must not measure a DIT it set itself, R10).
- [ ] **Step 2:** the programs call it instead of `enter_data_independent_timing`, and judge as
  before (`twice`, `verdict`).
- [ ] **Step 3: Run the lane natively on this Mac in release** (`AGENTS.md` chapter 4's
  command for the own lane `constant-time`): every dudect case passes, X25519's `agree` on
  libsodium included; `dudect_planted` still fails (its expected failure is the pass). Record
  each |t| printed. Then, for the report only and never committed: in a scratch copy of the
  library at `fec4047` (before the guard), delete the call to `enter_data_independent_timing`
  from `dudect_sodium_test.cpp`, run `dudect_sodium` natively in release, and record X25519's
  |t| without DIT.
- [ ] **Step 4: Commit** `crypto: the constant-time lane no longer sets DIT and measures what the library's own guard gives a program`.

### Task 7: The guard's cost, the throughput tables, the pages and the changelog

**Files:**
- Create: `bench/timing.cpp`; Modify: `bench/Jamfile` (`timing_check` with `webcpp.run`, exe
  `timing` at `<variant>release`, `explicit`), `bench/collect.py` (`--timing <exe>`, the
  `timing_guard` object in the data and its paragraph in the page), its tests if it has any.
- Regenerate: `doc/performance.json`, `doc/performance.adoc` (with `collect.py`, release builds
  of every target the data already holds).
- Modify: `doc/backends.adoc`, `doc/tables/crypto.json` (the constant-time findings' text) and
  regenerate `doc/generated/*` and `doc/llms.txt`, `doc/changelog.adoc` (R9),
  `AGENTS.md` of the library (the caution: a new public function holds the guard and joins the
  coverage test), and in the superproject `webcpp/CODING-GUIDELINES.md` chapter 15's crypto
  section (the guard as built, the rule for later increments) and chapter 17's paragraph on
  wasip2's finding (the native programs no longer set DIT).

- [ ] **Step 1: `bench/timing.cpp`:** times, in one process, 10^7 iterations each of an empty
  loop body, a guard entered from a clear thread, and a guard entered with DIT already set
  (the program sets it for that part through the same `msr` the test helper uses, then clears
  it), five runs each, and prints the medians in nanoseconds per guard minus the empty loop as
  JSON `{"dit": true|false, "clear": <ns>, "set": <ns>}`; on targets the guard does not cover it
  prints `"dit": false` and both costs. `timing_check` runs it with 10^3 iterations and checks
  the JSON's form only.
- [ ] **Step 2: `collect.py --timing`** refuses a non-release build like the throughput
  program, stores the native result with the date and the CPU, renders one paragraph and a
  two-row table in the page; run its existing tests (and add one for the refusal).
- [ ] **Step 3: Measure** on this Mac: build `throughput` and `timing` at release for each
  target, run `collect.py` as its docstring says; the tables and the guard's cost are
  regenerated, nothing typed.
- [ ] **Step 4: Rewrite `doc/backends.adoc`'s DIT paragraphs:** webcpp sets DIT around every
  operation on 64-bit ARM under macOS, iOS, Linux and Android where the CPU has it, and
  restores the thread's state; a program does nothing; what a program on x86 (DOITM is the
  kernel's), WebAssembly (the engine decides; the wasip2 finding stays open) and Windows on
  ARM64 (D7: no documented feature query, untested) gets; the cost, linking to the performance
  page; `scan_outputs`' lookup runs inside the guard (R6); the lane no longer sets DIT and what
  it measured, with the dates.
- [ ] **Step 5: Regenerate and check:** `generate.py`, then `generate.py --check`, the doc build
  of `AGENTS.md` chapter 3 (`doc-check`) clean.
- [ ] **Step 6: Commit** in crypto `crypto: the guard's cost measured in release and the pages say that webcpp sets DIT itself`;
  in the superproject the guidelines change goes with the submodule bump (Task 8).

### Task 8: Verification and delivery

- [ ] **Step 1:** the crypto tests and examples on native clang and gcc-15, wasip2, wasip3,
  Emscripten, in the variants each lane builds (`AGENTS.md` chapter 4); the constant-time lane
  natively in release (Task 6's results); the timing and coverage tests in an arm64 Linux
  container (the linux-lint image, where the Linux path of `cpu_has_dit` runs on real DIT
  hardware).
- [ ] **Step 2:** the full macOS lint (`AGENTS.md`); the Linux lint,
  `/Users/rodrigo/dev/webcpporg/linux-lint/run.sh <superproject>` (exit 0); the MSVC lane
  (`/Users/rodrigo/dev/webcpporg/msvc-lane`, 14.3 and 14.5, `summarize.sh`, no failure beyond
  `known-failures.txt`); the superproject's tools tests if a tool changed. Any failure or flake,
  related or not, is fixed at its cause.
- [ ] **Step 3:** tick `../tasks.md`; pull requests in crypto, proposals and the superproject
  (the gitlink bump and the guidelines), merged with merge commits, branches deleted, the
  intermediate CI runs cancelled, one CI run per touched repository on main watched to green.
