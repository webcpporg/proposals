---
title: "webcpp::hash, data-independent timing: Implementation Plan"
tags: [webcpp, hash, dit, constant-time, plan]
status: accepted
created: 2026-11-20
---

# webcpp::hash, data-independent timing: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** every public function of webcpp::hash one of whose inputs is a key, a password or a
key derivation's secret input runs its whole body with PSTATE.DIT set on 64-bit ARM under macOS,
iOS, Linux and Android where the CPU has FEAT_DIT, on the thread that computes it, and returns
with the thread's DIT as it found it; unkeyed hashing and Boost.Hash2's aliases are left as they
are, and a test proves both sides of that line.

**Architecture:** hash's own RAII type, `webcpp::hash::detail::timing_guard` in
`detail/timing.hpp`, crypto's guard with an `engaged` flag (design D4), is the first statement
of every keyed public function (D1). A seam, `WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)`, fires in
an engaged guard's constructor on every target; a coverage test per backend reads
`doc/capabilities.json`, holds one call per keyed cell of its backend, and checks each call's
first guard (D5); a test runs the asynchronous forms on a pool and reads each thread's entries
(D6); a test in crypto checks that the two libraries' guards agree (D4).

**Tech stack:** C++20 header-only, b2, Boost.Core's lightweight_test, Boost.JSON in the tests, the
superproject's lanes (native clang and gcc-15, wasip2, wasip3, Emscripten, MSVC 14.3 and 14.5
under Wine), Python 3 for `bench/collect.py`.

**Spec:** `../proposal.md`, `../design.md` (D1 to D9, binding), `../specs/hash/spec.md`,
`../tasks.md`. Executors read all four, and crypto's change
`libraries/crypto/openspec/changes/data-independent-timing/` (its design and its plan's rulings
R1 to R10, which this plan follows where it does not say otherwise).

Paths below are relative to the library, `libs/hash/` of the superproject (its own repository
`webcpporg/hash`), unless they start with `libs/crypto/` (webcpp::crypto, `webcpporg/crypto`) or
`webcpp/` (the superproject).

## Global Constraints

- Everything written to a file, a commit or a pull request is in English; no em dash (U+2014);
  the banned word of `CODING-GUIDELINES.md` chapter 2 nowhere.
- Commits: `git -c user.name=... commit -- <paths>` with explicit paths, subjects `hash: ...`
  (`crypto: ...` for Task 6, `guidelines: ...` in the superproject) as descriptive sentences, no
  trailer of any kind.
- Every new source file opens with the licence notice; every public or detail symbol carries its
  doc comment (chapter 7); `.clang-format` and `.clang-tidy` clean; a test is declared through
  `hash-run` (exceptions on and off) unless its backend allows only one (Botan: on).
- Never edit a generated file (`doc/generated/*`, `doc/llms.txt`, `doc/performance.adoc` and its
  data, the superproject's generated tables): change the source and run its generator.
- Sizes and speed come from release builds only, never typed by hand.
- hash gains no dependency: nothing under `libs/hash/` includes a header of `libs/crypto/` or
  names a crypto target. The public interface and the errors do not change.
- The guard's code is crypto's (`libs/crypto/include/webcpp/crypto/detail/timing.hpp` on crypto's
  `main`, after its change data-independent-timing): the register by its encoding,
  `s3_3_c4_c2_5`, DIT bit 24, `isb` after each write, `hw.optional.arm.FEAT_DIT` on Apple (an
  unreadable value counted as absent), `getauxval(AT_HWCAP) & (1UL << 24)` on Linux; applied
  where `__aarch64__` and (`__APPLE__` or `__linux__`); an empty class with no instruction
  elsewhere.

## Rulings made while writing this plan (controller, 2026-11-20, owner on autopilot)

- **H1. One guard spelling.** `detail::timing_guard const timing;` where always engaged,
  `detail::timing_guard const timing{engaged};` where the arguments decide (D4), first statement,
  before the guards' checks (`mac_guards`, `hkdf_guards`, ...) and any `static_assert`.
- **H2. Templates guard once.** `mac`, `hkdf`, `pbkdf2`, `argon2` and the others are templates
  over the backend: the guard is in the template, and every backend's engine runs inside it. No
  backend header holds a guard (crypto's design, approach 2 rejected).
- **H3. Forwarders hold their own** (crypto's R1): `hkdf` and the `hkdf_extract` and
  `hkdf_expand` it calls; `blake3::update(void const*, ...)` and the overload it calls; any
  public function that calls another (`argon2id` and `argon2`, if one calls the other on a
  backend). Guards nest. A delegating constructor (`blake3(void const*, std::size_t)`) holds
  none: the constructor it delegates to runs first and holds its own, so it is the first guard
  the seam sees.
- **H4. What is keyed, at run time.** `hasher<Digest>` keeps `bool keyed_`, set in `make` from
  `detail::options_keyed(options)`: `!options.key.empty()` where `digest_options<Digest>` has a
  `key` member (BLAKE2b's today, found by a `requires` expression), `false` otherwise; `update`
  and `finish` engage their guard with it. `digest_of` engages its guard with the same function
  of its options. The `blake3` class keeps `bool keyed_`, set by `keyed`, `derive_key`, a
  non-empty byte seed and a non-zero integer seed; every member function engages its guard with
  it, and each of those factories and constructors engages its own with its argument. Moves copy
  the flag, as they copy the state. The flags are private; the classes' sizes grow by at most
  their alignment, which the documentation of `blake3` (1,912 bytes of state) says.
- **H5. Moves, resets, destructors** hold no guard but `blake3::reset` of a keyed object
  (it re-initialises BLAKE3's state from the key: a computation on the key) and `hasher`'s
  destructor and `end` only wipe; consistent with crypto's R3.
- **H6. What the coverage tests call "keyed cells"**: in `doc/capabilities.json`, every function
  of the chapters `macs`, `kdfs` and `password_hashing` whose `kind` is not `type` (the Hash2
  aliases, D3), each backend's cell of it, one per HMAC where the cell lists `hmacs` (`other`, any
  Hash2 hmac the table allows, is one call over a Hash2 hmac the list does not name, the test's
  choice, named `<function><other>`); plus two keyed digests named by the test support with
  design D1 as the reason: `digests::blake2b` (each backend's cell, called with a key) and the
  `blake3` type (on `blake3`, its keyed modes). `backends::webcrypto`'s cells are not keyed cells
  of any coverage test (D6).
- **H7. The seam's support is a copy of crypto's** (`test/support/timing_seam.hpp`: the counter,
  the first entry, `function_named`, `normalized_function_name`, `names_function`,
  `require_enters`), with hash's macro; `test/support/timing_coverage.hpp` holds `covered`,
  `uncovered` and the table check. The duplication is two test files and is accepted for the
  reason of D4; each names crypto's as its origin.
- **H8. The changelog** holds at most ten lines (the lint); if hash's `unreleased` tag is full,
  the change is a clause folded into the line of the keyed families, as crypto's R9 folded its
  own.
- **H9. The order** (D8): the branches start from `main` after hash's increment 6 has merged. If
  they start before, every mention of Botan and bcrypt below is skipped and the increment's
  stream adds them, its coverage file included.

## Patch against main (controller, 2026-11-22, owner on autopilot)

The plan was written on 2026-11-20, before two streams merged into hash's `main`: increment 6,
Botan (`backends/botan.hpp`, `bcrypt.hpp`, the Botan forms of `async.hpp`), and P-256's BearSSL pin
move (hash's BearSSL canary at the pin `32381d6`). The branches start from hash's `main` `24f2d63`
and the superproject's `83755de`, both after increment 6, so H9's first case holds: every mention
of Botan and bcrypt below is in scope. The pin move changes no keyed function and no task. The
rulings H10 to H17 complete or override the plan where they say so; crypto's change is read as
built (its design's section "As built").

- **H10. Botan adds no public function but `bcrypt.hpp`'s.** Its hashes, MACs and KDFs are engines
  under the templates `digest_of`, `hasher`, `mac`, `hkdf_extract`, `hkdf_expand`, `hkdf`,
  `pbkdf2`, `kbkdf`, `sskdf`, `x963kdf`, `argon2id`, `argon2` and `scrypt` (H2): the template's
  guard covers each. `detail::botan_boundary` (the backend's one seam where Botan's exceptions
  become `error_code`s, `noexcept`) runs inside the public function's guard and holds none; since it
  catches everything, the guard's scope always ends by a return, and the destructor restores the
  thread's DIT on every path. No Botan header holds a guard.
- **H11. Botan's keyed cells** (`doc/capabilities.json` at `24f2d63`, H6's rule), 41: `mac` over
  `macs::siphash_2_4`, `macs::kmac_128`, `macs::kmac_256`, `macs::blake2b_mac` and
  `macs::hmac<D>` for the 23 digests of Botan's cells (MD5, SHA-1, SHA-2 but the truncated
  SHA-512/224, SHA-3, RIPEMD-160, BLAKE2b, BLAKE2s-256, Keccak-224 to 512, SM3, Streebog-256 and
  512, Whirlpool, Skein-512); `hkdf_extract`, `hkdf_expand`, `hkdf`, `pbkdf2`, `kbkdf` over each
  of the 14 hmacs the cells list; `sskdf`, `x963kdf`; `argon2id`, `argon2<kdfs::argon2d>`,
  `argon2<kdfs::argon2i>`, `argon2<kdfs::argon2id>`; `scrypt`; `bcrypt`, `bcrypt_pbkdf`. Plus the
  keyed digest `digests::blake2b` (Botan's `BLAKE2bMAC` when the options carry a key). The table
  is the list: `timing_coverage_botan_test.cpp` reads it and fails on a cell it does not call.
- **H12. `bcrypt_verify` is guarded and covered though it is no cell** (the table's `bcrypt` row
  holds it): `timing_coverage_botan_test.cpp` names it as a keyed function of the test support
  with D1 as the reason, as H6 names the keyed digests, so that `check_and_run` takes it as a
  cell. The same support list names `blake3` (keyed modes) and `digests::blake2b` (keyed).
- **H13. The asynchronous forms, Botan's included.** Every form of `async.hpp` computes by calling
  its synchronous function (`async_bcrypt` calls `bcrypt<Backend>`, `async_bcrypt_verify`
  `bcrypt_verify<Backend>`, `async_bcrypt_pbkdf` `bcrypt_pbkdf<Backend>`; the heavy
  `async_pbkdf2`, `async_argon2id`, `async_argon2`, `async_scrypt` on `backends::botan` the same):
  no initiating function holds a guard (D6), and Task 5's `timing_async_botan` proves each Botan
  heavy form on the pool's thread. A form found computing outside its synchronous function is
  fixed to call it, with Task 5's test failing first.
- **H14. The guard follows crypto's as built, not crypto's plan.** crypto's constructor takes a
  `std::source_location` only where a test defines the seam (clang on arm64 kept every guarded
  function's name and header path in release otherwise). hash's: where
  `WEBCPP_HASH_DETAIL_TIMING_ENTERED` is defined, `explicit timing_guard(bool engaged = true,
  std::source_location where = std::source_location::current()) noexcept`; otherwise `explicit
  timing_guard(bool engaged = true) noexcept`, and `<source_location>` is included only with the
  seam. Task 1 also copies crypto's `timing_unseamed_test.cpp` (`hash-run timing_unseamed` and
  `webcpp.run timing_unseamed_release : ... : <variant>release
  <define>WEBCPP_TEST_TIMING_READ_SELF : native`), over `mac<macs::hmac<digests::sha2_256>>` on
  `hash2`: without the seam the guard is not constructible from a `source_location`, and the
  release program holds neither `webcpp::hash::mac` nor `mac.hpp`. crypto's header is not edited
  (this stream owns one file of crypto, below): its comment names no other copy; hash's header,
  the interop test and the guidelines name both. crypto's comment is a parked item for crypto's
  next change of that header.
- **H15. crypto: one file and its Jamfile line.** Task 6 adds
  `libs/crypto/test/timing_hash_interop_test.cpp` and its line in `libs/crypto/test/Jamfile`,
  nothing else of crypto (`libs/crypto/AGENTS.md` is not edited; hash's `AGENTS.md` and the
  guidelines carry the caution). Another stream (crypto/async) edits crypto: whichever merges
  second merges `origin/main` first.
- **H16. Measurements: the guard's cost alone, provisional.** Task 7 measures `bench/timing` from
  a release build on this Mac and renders it with `collect.py --timing`; the page marks the
  figures provisional (measured on a loaded machine) for the coordinator's final quiet pass. The
  throughput tables are not regenerated by this change: the guard adds a per-call constant that the
  quiet pass's regeneration captures; `../tasks.md` lists the pass as open, as crypto's does.
- **H17. Delivery** (owner, 2026-11-22): no CI until the coordinator's end; the pull requests are
  merged with merge commits without waiting for CI, every run they start is cancelled, none is
  watched. Task 8 Step 3's "one CI run ... watched to green" is replaced by this. Heavy lanes go
  through `/Users/rodrigo/dev/webcpporg/heavy.sh hash-dit ...`; the Linux lint and the MSVC lane
  run on a full-clone copy of the superproject (they cannot read a worktree's `.git` files).

## Review Focus

1. **A keyed call on a thread whose program set DIT itself**: it stays set. Pinned by Task 1's
   `guard_leaves_set_bit` and Task 2's `mac_leaves_set_state`.
2. **An unkeyed call**: no guard, no seam. Pinned by every `uncovered` call (Task 4) and Task 2's
   `unkeyed_blake2b_enters_none`.
3. **A keyed hasher moved, then updated**: the moved-to object keeps `keyed_`. Pinned by Task 2's
   `moved_keyed_hasher`.
4. **A pool's thread after an asynchronous derivation**: its DIT as before. Pinned by Task 5.
5. **The two guards together** in one program (crypto's Ed25519ph over hash's SHA-512 beside an
   HKDF): pinned by Task 6.
6. **Every non-arm64 target**: the seam fires, nothing traps. Task 1's tests on every lane, Task
   8's arm64 Linux container.

---

## Inventory of guarded public functions (D1, H1 to H6)

On hash's `main` after increment 6; the implementer finds each by name.

**Always engaged:**
- `mac.hpp`: `mac`.
- `hkdf.hpp`: `hkdf_extract`, `hkdf_expand`, `hkdf`.
- `pbkdf2.hpp`: `pbkdf2`.
- `kdf_standards.hpp`: `kbkdf`, `sskdf`, `x963kdf`.
- `blake2b_subkey.hpp`: `blake2b_subkey`.
- `argon2.hpp`: `argon2id`, `argon2`.
- `scrypt.hpp`: `scrypt`.
- `bcrypt.hpp` (increment 6): `bcrypt`, `bcrypt_verify`, `bcrypt_pbkdf`.
- Every backend's engine of these runs inside the template's guard, Botan's included (H10, H11).

**Engaged by the arguments (H4):**
- `digest.hpp`: `digest_of` (by its options), `hasher::make` (by its options), `hasher::update`,
  `hasher::finish` (by `keyed_`).
- `blake3.hpp`: `blake3::keyed`, `blake3::derive_key` (always engaged), the constructors
  `blake3(std::uint64_t)` (engaged when not zero), `blake3(unsigned char const*, std::size_t)`
  (engaged when the size is not zero; `blake3(void const*, std::size_t)` delegates to it, H3), `update` (both),
  `result`, `squeeze`, `reset` (by `keyed_`).

**Not guarded (checked boundary):** `digest_of` and `hasher` of every other tag, and of BLAKE2b
without a key; `keccak_256` (all of it); `blake3()` and `blake3` in its plain mode; every Hash2
alias (`algorithms.hpp`, `hmac.hpp`); everything `constexpr` (the options' checks, the traits);
the guards' helpers (`mac_guards`, `hkdf_guards`, `bcrypt_guards`, ... : they run inside the
public function's guard); `detail::botan_boundary` (H10); every move and destructor; `async.hpp`'s
initiating functions (D6), Botan's `async_bcrypt`, `async_bcrypt_verify` and `async_bcrypt_pbkdf`
included (H13); `backends::webcrypto`.

---

### Task 1: hash's guard, its seam and its unit tests

**Files:**
- Create: `include/webcpp/hash/detail/timing.hpp`, `test/timing_test.cpp`,
  `test/support/timing_seam.hpp`.
- Modify: `test/Jamfile` (`hash-run timing : timing_test.cpp ;`; `timing_threads`, the same
  source with `WEBCPP_TEST_TIMING_THREADS`, native only, `<threading>multi`; `hash/detail/timing.hpp`
  in the `webcpp.headers-alone hash` list), `meta/include-boundaries.json` (the new header, as
  the other `detail/` headers are listed).

**Interfaces:**
- Also (H14): Create `test/timing_unseamed_test.cpp`; `test/Jamfile` gains `timing_unseamed` and
  `timing_unseamed_release`.
- Produces, in `namespace webcpp::hash::detail`: `[[nodiscard]] bool cpu_has_dit() noexcept`,
  `[[nodiscard]] bool dit_is_set() noexcept`, `class timing_guard` (with the seam `explicit
  timing_guard(bool engaged = true, std::source_location where = std::source_location::current())
  noexcept`, without it `explicit timing_guard(bool engaged = true) noexcept` (H14), a `noexcept`
  destructor, copy and move deleted), the macros
  `WEBCPP_HASH_DETAIL_TIMING_COVERED` and `WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)` (defined as
  `static_cast<void>(where)` unless defined before the first include).
- Produces, in `test/support/timing_seam.hpp` (included before any hash header; it `#error`s if
  `WEBCPP_HASH_DETAIL_TIMING_HPP` is already defined): `webcpp::test::timing_entries`,
  `timing_first_entry`, `timing_entered`, the matcher and `require_enters`, crypto's
  `test/support/timing_seam.hpp` with `CRYPTO` read as `HASH`.

- [ ] **Step 1: Write the failing tests** in `test/timing_test.cpp` (crypto's `timing_test.cpp`
  with hash's names), each a function called from `main`: `seam_fires` (an engaged guard raises
  `timing_entries` by 1), `disengaged_does_nothing` (a guard constructed with `false` raises
  nothing and leaves DIT as it was in both states), `guard_sets_and_clears`,
  `guard_leaves_set_bit`, `nested_guards` (an engaged guard inside an engaged one; a disengaged
  one inside an engaged one leaves DIT set), and in `timing_threads`
  `other_thread_untouched`. Where `cpu_has_dit()` is false, each reduces to `!dit_is_set()`
  throughout and the seam's counts. The test-local `msr` helper that sets and clears DIT is
  crypto's test's.
- [ ] **Step 2: Run them to see them fail:** `$B2 libs/hash/test//timing
  libs/hash/test//timing_threads`; expected: `detail/timing.hpp` missing.
- [ ] **Step 3: Implement `detail/timing.hpp`.** Copy crypto's header as built (H14), then: the namespace,
  include guard and macros renamed (`WEBCPP_HASH_DETAIL_TIMING_HPP`,
  `WEBCPP_HASH_DETAIL_TIMING_COVERED`, `WEBCPP_HASH_DETAIL_TIMING_ENTERED`); `#include
  <webcpp/hash/config.hpp>` first; the constructor:

  ```cpp
  explicit timing_guard(bool engaged = true,
                        std::source_location where = std::source_location::current()) noexcept {
      if (!engaged) {
          return;
      }
      WEBCPP_HASH_DETAIL_TIMING_ENTERED(where);
  #ifdef WEBCPP_HASH_DETAIL_TIMING_COVERED
      if (cpu_has_dit() && !dit_is_set()) {
          write_dit(dit_bit);
          armed_ = true;
      }
  #endif
  }
  ```

  The header's opening comment states D4: why hash has its own copy (the dependency's direction,
  crypto's `detail`, config's rule), that crypto's
  `libs/crypto/include/webcpp/crypto/detail/timing.hpp` is the other copy and
  `libs/crypto/test/timing_hash_interop_test.cpp` keeps them in step, and what the `engaged` flag
  is for. On a target the guard does not cover, `engaged` is read only to fire the seam.
- [ ] **Step 4: Run them and see them pass** on native clang, gcc-15, wasip2, wasip3 and
  Emscripten (AGENTS.md chapter 4's commands for each).
- [ ] **Step 5: Commit** `hash: detail::timing_guard sets arm64's DIT for its scope and restores the thread's state, hash's own copy of crypto's with an engaged flag`.

### Task 2: `mac` and the keyed modes of BLAKE2b and BLAKE3 hold the guard

**Files:** Modify `include/webcpp/hash/mac.hpp`, `include/webcpp/hash/digest.hpp`,
`include/webcpp/hash/blake3.hpp` (each includes `<webcpp/hash/detail/timing.hpp>` after
`config.hpp`). Create `test/timing_keyed_test.cpp` (`hash-run` with `<library>/third_party//blake3
<library>/third_party//sodium`).

**Interfaces:** Consumes Task 1. Produces `detail::options_keyed(options) -> bool` (in
`digest.hpp`'s `detail`), the private `keyed_` members of `hasher` and `blake3` (H4).

- [ ] **Step 1: Write the failing tests** in `test/timing_keyed_test.cpp` (seam first):
  - `mac_enters`: `require_enters("hash::mac", ...)` for `mac<macs::hmac<digests::sha2_256>>` on
    `backends::hash2`, and for `mac<macs::blake2b_mac, backends::sodium>`; an output of the
    wrong size still enters (the guard precedes the checks).
  - `mac_leaves_set_state`: where the CPU has DIT, set it through the test helper, call `mac`,
    `BOOST_TEST(dit_is_set())`; clear it.
  - `keyed_blake2b`: `digest_of<digests::blake2b>` with `{.key = key}` enters `digest_of`'s
    guard; `hasher<digests::blake2b>::make({.key = key})`, its `update` and `finish` each enter
    their own.
  - `unkeyed_blake2b_enters_none`: the same three without a key, and `digest_of<digests::sha2_256>`,
    enter none (`uncovered`, which Task 4 moves into the support header; this task defines it
    locally first).
  - `moved_keyed_hasher`: a keyed hasher moved into another, then updated: the update enters
    its guard; an unkeyed one moved, then updated: none.
  - `blake3_modes`: `blake3::keyed(key)` and `blake3::derive_key("ctx")`, their `update`,
    `result`, `squeeze` and `reset`, and the constructors `blake3(7)` and `blake3(bytes, 16)`
    each enter their own guard; `blake3()`, `blake3(0)`, `blake3(bytes, 0)` and their `update`
    and `result` enter none; outputs equal BLAKE3's official vectors' keyed and derive-key
    entries as `blake3_vectors_test.cpp` reads them (the guard changes no byte).
- [ ] **Step 2: Run** `$B2 libs/hash/test//timing_keyed`: FAIL naming each function.
- [ ] **Step 3: Add the guards** (H1, H4). `mac`:

  ```cpp
  template <class Mac, class Backend = default_backend<Mac>>
  [[nodiscard]] boost::system::result<void> mac(std::span<std::uint8_t> output,
                                                std::span<std::uint8_t const> key,
                                                std::span<std::uint8_t const> input,
                                                mac_options<Mac> const& options = {}) {
      detail::timing_guard const timing;
      using engine = detail::mac_engine<Backend, Mac>;
      // ... unchanged
  }
  ```

  `digest.hpp`:

  ```cpp
  namespace detail {
  /** Whether a digest's options carry a key: a keyed BLAKE2b is a MAC (design D1). */
  template <class Options>
  [[nodiscard]] constexpr bool options_keyed(Options const& options) noexcept {
      if constexpr (requires { options.key.empty(); }) {
          return !options.key.empty();
      } else {
          return false;
      }
  }
  }  // namespace detail
  ```

  `digest_of`'s first statement `detail::timing_guard const timing{detail::options_keyed(options)};`;
  `hasher::make` the same and `made.keyed_ = detail::options_keyed(options);`; `update` and
  `finish` `detail::timing_guard const timing{keyed_};`; the moves copy `keyed_`. `blake3`: the
  member, set as H4 says; each listed member's first statement engages its guard (the
  constructors' by their argument: `timing_guard const timing{seed_value != 0};`,
  `{input_size != 0}`); `keyed` and `derive_key` always. The class comment gains one paragraph:
  the keyed modes set DIT for each call where the CPU has it (D1), the plain mode does not (D2).
- [ ] **Step 4: Run** it and the existing tests of these headers (`families`, `hasher_state`,
  `blake3`, `blake3_vectors`, `blake2b_sodium`, `blake2b_mac_sodium`, `macs_openssl`,
  `hash2_mac_vectors`, the `agreement_*`): PASS.
- [ ] **Step 5: Commit** `hash: mac and the keyed modes of BLAKE2b and BLAKE3 set DIT around each call on every backend`.

### Task 3: The key derivations and the password hashes hold the guard

**Files:** Modify `hkdf.hpp`, `pbkdf2.hpp`, `kdf_standards.hpp`, `blake2b_subkey.hpp`,
`argon2.hpp`, `scrypt.hpp`, `bcrypt.hpp`. Modify `test/timing_keyed_test.cpp`.

- [ ] **Step 1: Write the failing tests:** `require_enters` for each function of the inventory in
  these headers on its default backend (`hkdf<hmac_sha2_256>` on `hash2`, its extract and expand;
  `pbkdf2<hmac_sha2_256>` with 1,000 iterations; `argon2id` on `sodium` with the minimum
  parameters; `scrypt` on `sodium` with N = 16; `blake2b_subkey` on `sodium`; `kbkdf`, `sskdf`,
  `x963kdf` where OpenSSL is (native, wasip2, wasip3: a second program `timing_keyed_openssl`
  with `<library>/webcpp/hash//openssl`); `bcrypt`, `bcrypt_verify`, `bcrypt_pbkdf` on Botan (a
  third, `timing_keyed_botan`, exceptions on, native and Emscripten)); each also with a parameter
  refused before the backend (an iteration count of 0, a salt too short); `hkdf`'s first entry is
  `hkdf`'s own though it calls the other two (H3).
- [ ] **Step 2: Run** them: FAIL naming the functions.
- [ ] **Step 3: Add the guards,** first statement of each (H1).
- [ ] **Step 4: Run** them and every test of these headers (`hkdf_*`, `pbkdf2_*`, `argon2*`,
  `scrypt*`, `kdf_*`, `kdf_standards_openssl`, the Botan KDF tests, `agreement_hkdf`,
  `agreement_pbkdf2`, `async_*`): PASS.
- [ ] **Step 5: Commit** `hash: the key derivations and the password hashes set DIT around each call on every backend`.

### Task 4: The coverage test per backend, driven by the capability table

**Files:**
- Create: `test/support/timing_coverage.hpp`,
  `test/timing_coverage_{hash2,sodium,bearssl,openssl,xkcp,blake3,botan}_test.cpp`.
- Modify: `test/Jamfile` (each through `hash-run` with its backend's library and
  `$(.hash-tables)`, on its backend's targets: OpenSSL `native wasip2 wasip3`, Botan through
  `botan-run` natively and on Emscripten), `test/timing_keyed_test.cpp` (its local `uncovered`
  replaced by the support's).

**Interfaces:**
- Produces, in `test/support/timing_coverage.hpp` (`namespace webcpp::test`):
  - `covered(name, call)` and `covered(name, function, call)`, crypto's: `require_enters`, then
    DIT as before.
  - `uncovered(name, call)`: fails if the call enters a guard.
  - `using coverage_calls = std::map<std::string, std::function<void()>, std::less<>>;`
  - `std::vector<std::string> keyed_cells(std::string_view backend)`: reads
    `WEBCPP_TEST_HASH_DOC "/capabilities.json"` with Boost.JSON, returns the names of H6's keyed
    cells of `backend`, in the table's order.
  - `void check_and_run(std::string_view backend, coverage_calls const& calls)`: reports
    `"<cell> on <backend> is a keyed cell with no coverage call"` for each cell missing from
    `calls`, `"<name> on <backend> names no keyed cell"` for each call not in the cells, then
    runs every call.

- Botan (H11, H12): `timing_coverage_botan` through `botan-run` (exceptions on, native and
  Emscripten); its registry covers the 41 cells of H11 and `bcrypt_verify` and the keyed
  `digests::blake2b`; its unkeyed boundary is every digest cell of `botan` in the table.
- [ ] **Step 1: Write each backend's file:** its comment lists, as crypto's do, what it covers
  by family and states that the list is the table's (a new cell fails until its call is added);
  `main` builds the backend's `coverage_calls` from helpers (`cover_mac<Mac, Backend>(name)`,
  `cover_hkdf<Hmac, Backend>`, `cover_pbkdf2<Hmac, Backend>`, `cover_argon2<Variant, Backend>`,
  ..., each a `covered` call with valid input and one refused before the backend, and for
  BLAKE2b's cell a keyed `digest_of` and a keyed `hasher` round), calls `check_and_run`, then
  the backend's unkeyed boundary through `uncovered`: every digest cell of the backend in the
  table, called unkeyed through `digest_of` and through a `hasher` round (the table gives the
  list: each file iterates the backend's digest cells the same way, with a small dispatch from
  the tag's name to the call, and fails on a digest cell it cannot dispatch).
  `timing_coverage_hash2_test.cpp` also calls the Hash2 aliases of `hmac.hpp` and
  `algorithms.hpp` through `uncovered` (D3), and `timing_coverage_xkcp_test.cpp` and
  `timing_coverage_blake3_test.cpp` `keccak_256` and plain `blake3`.
- [ ] **Step 2: Run** the seven on native clang: FAIL only where a call is missing or a guard is
  (none should be, after Tasks 2 and 3; a failure here is a gap in the inventory, fixed in the
  header with its own test first).
- [ ] **Step 3: Plant two misses,** locally and never committed: remove `pbkdf2`'s guard and
  see `timing_coverage_hash2` and `timing_coverage_openssl` fail naming `pbkdf2`; add a fake
  keyed cell to a scratch copy of `capabilities.json` (`macs::fake` on `sodium`) pointed to by
  the define and see `timing_coverage_sodium` fail naming it; restore.
- [ ] **Step 4: Run** the seven on every lane each builds on (native clang, gcc-15, wasip2,
  wasip3, Emscripten), release and debug: PASS; `capabilities_test` and the superproject's
  `tools/test/hash_doc_tables_test.py`: unchanged and PASS.
- [ ] **Step 5: Commit** `hash: a coverage test per backend, driven by the capability table, proves every keyed cell enters its own timing guard and every unkeyed one none`.

### Task 5: The asynchronous forms set DIT on the thread that computes

**Files:** Create `test/timing_async_test.cpp`. Modify `test/Jamfile` (native and Emscripten
with threads, where `WEBCPP_HASH_HAS_THREADS` is defined, as `async_heavy` is declared; with
`<library>/third_party//sodium`; a `timing_async_botan` twin for `async_bcrypt*` where Botan
builds).

- [ ] **Step 1: Write the test** (seam first). A `webcpp::asio::thread_pool pool(1)` and an
  `io_context` on the calling thread. For each heavy function (`async_pbkdf2`, `async_argon2id`,
  `async_argon2`, `async_scrypt`, and with Botan `async_bcrypt`, `async_bcrypt_verify`,
  `async_bcrypt_pbkdf`): post to the pool a probe that resets the worker's `timing_entries` and
  `timing_first_entry`; reset the calling thread's; initiate on the pool's executor; run to
  completion; post a probe that reads the worker's two values; check the worker entered the
  synchronous function's guard first (`names_function` with `hash::pbkdf2`, ...) and the
  calling thread entered none; where the CPU has DIT, the probe and the calling thread each read
  `dit_is_set() == false` after the completion. For the cheap ones on a synchronous backend
  (`async_hmac`, `async_hkdf`): the calling thread entered `mac`'s and `hkdf`'s guard during the
  initiation, the pool none. `async_digest` (unkeyed): no thread entered one. A cancellation
  before the heavy work starts (the existing `async_heavy_test` pattern): no thread entered one.
- [ ] **Step 2: Run** it natively and on Emscripten with threads: PASS (Tasks 2 and 3 already
  put the guards in the synchronous functions; if a form computes outside them, it fails here
  and the form is fixed to call its synchronous function).
- [ ] **Step 3: Commit** `hash: a test proves the asynchronous forms set DIT on the thread that computes, the pool's for a heavy function and the caller's for a cheap one`.

### Task 6: crypto's test of the two guards together

**Files (webcpp::crypto):** Create `libs/crypto/test/timing_hash_interop_test.cpp`. Modify
`libs/crypto/test/Jamfile` (`crypto-run timing_hash_interop : timing_hash_interop_test.cpp :
<library>/third_party//sodium ;`). Nothing else of crypto (H15).

- [ ] **Step 1: Write the test.** It includes crypto's seam support first, then defines hash's
  seam macro to a second counter, then both `detail/timing.hpp` headers (crypto's
  `<webcpp/crypto/detail/timing.hpp>` and hash's `<webcpp/hash/detail/timing.hpp>`, which
  crypto's target reaches through `/webcpp/hash//hash`). Cases: `same_cpu`
  (`crypto::detail::cpu_has_dit() == hash::detail::cpu_has_dit()` and the same for
  `dit_is_set()`, inside and outside guards); `hash_inside_crypto` and `crypto_inside_hash` from
  a clear state and from a set state (DIT inside both, after the inner as inside, after the
  outer as before); `ed25519ph_uses_hash_unkeyed`: `curves::ed25519::ph<webcpp::hash::sha2_512>`
  signs; crypto's counter rose, hash's did not (Ed25519ph's prehash is unkeyed, D2);
  `hkdf_beside_crypto`: an `hkdf` then an Ed25519 signature on the same thread, each entering its
  library's guard once, DIT as before after both.
- [ ] **Step 2: Run** it on every lane crypto runs: PASS (with hash's Task 1 merged first; until
  then it does not build, and this task waits for hash's pull request).
- [ ] **Step 3: Commit** (crypto) `crypto: a test proves crypto's and hash's timing guards nest either way and agree on what the CPU offers`.

### Task 7: The guard's cost, the throughput tables, the pages, the changelog and the guidelines

**Files:**
- Create: `bench/timing.cpp` (crypto's, with hash's guard: the empty loop, a guard entered from a
  clear thread, a guard entered with DIT set, a disengaged guard; medians of five runs of 10^7,
  JSON); modify `bench/Jamfile` (`timing_check` with `webcpp.run` and 10^3 iterations, exe
  `timing` at `<variant>release`, `explicit`), `bench/collect.py` (`--timing <exe>`, refusing a
  non-release build as for throughput, the `timing_guard` object in the data and its paragraph
  and table in the page) and its tests.
- Regenerate: `doc/performance.adoc` and its data with the guard's cost (with `collect.py
  --timing`, a release build on this Mac), marked provisional; the throughput tables are left to
  the coordinator's quiet pass (H16).
- Modify: `doc/backends.adoc` (a section "Data-independent timing"), `doc/macs.adoc`,
  `doc/kdfs.adoc`, `doc/password_hashing.adoc` (one paragraph each: the family sets DIT around
  each call), `doc/digests.adoc` (unkeyed digests do not, and why; the keyed BLAKE2b and BLAKE3
  modes do), the HMAC page's alias section (D3: `hmac<H>` holds none; call `mac<macs::hmac<D>>`
  or set DIT), `doc/async.adoc` (D6), `doc/changelog.adoc` (H8), `AGENTS.md` of the library (a
  caution: a new keyed function or keyed cell holds the guard and gets its coverage call; the
  guard is a copy of crypto's, changed in both), and regenerate `doc/generated/*` and
  `doc/llms.txt`.
- Superproject: `CODING-GUIDELINES.md`, hash's design section (chapter 15 if hash is recorded
  there when this runs, else its chapter 17 entry): the rule as built (keyed functions hold
  `detail::timing_guard`, unkeyed and the aliases do not, with D2's and D3's reasons; the
  coverage tests from the table; the copy of crypto's guard and the interop test); crypto's
  section's sentence "webcpp::hash is out of scope" replaced by a pointer to hash's.

- [ ] **Step 1: `bench/timing.cpp` and `timing_check`,** as crypto's Task 7 Step 1, with the
  disengaged case added.
- [ ] **Step 2: `collect.py --timing`** and its test for the refusal of a debug build.
- [ ] **Step 3: Measure** on this Mac (release, each target), run `collect.py` as its docstring
  says; nothing typed.
- [ ] **Step 4: The pages** (above), each fact cited; the cost from the generated table.
- [ ] **Step 5: Regenerate and check:** the superproject's `tools/hash/doc_tables.py`, the doc
  build (`b2 -a doc`, `doc-check`): clean.
- [ ] **Step 6: Commit** (hash) `hash: the guard's cost measured in release and the pages say which functions set DIT and which do not`;
  the guidelines go with the superproject's gitlink bump (Task 8).

### Task 8: Verification and delivery

- [ ] **Step 1:** hash's tests and examples on native clang and gcc-15, wasip2, wasip3 and
  Emscripten, exceptions on and off, release and debug as each lane builds (AGENTS.md chapter 4);
  crypto's tests on the same lanes (Task 6's test, and nothing else of crypto changed); the
  timing, coverage and asynchronous tests in the arm64 Linux container (the linux-lint image),
  where `cpu_has_dit` reads the real `HWCAP_DIT`.
- [ ] **Step 2:** the full macOS lint; the Linux lint
  (`/Users/rodrigo/dev/webcpporg/linux-lint/run.sh <superproject>`, exit 0); the MSVC lane
  (`/Users/rodrigo/dev/webcpporg/msvc-lane`, 14.3 and 14.5, `summarize.sh`, no failure outside
  `known-failures.txt`); the superproject's tools tests that changed or whose inputs changed
  (`tools/test/hash_doc_tables_test.py`, `tools/test/hash_jamfiles_test.py`). Any failure or
  flake, ours or not, is fixed at its cause.
- [ ] **Step 3:** tick `../tasks.md`; pull requests in hash, crypto (Task 6), proposals and the
  superproject (the gitlinks and the guidelines), hash's merged before crypto's; merge commits,
  branches deleted, every CI run the merges start cancelled, none watched (H17).
