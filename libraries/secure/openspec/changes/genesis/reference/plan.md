# secure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** webcpp::secure, a header-only library holding webcpp's DIT guard, `erase_secret`,
`secret<N>`, `constant_time_equal`, `declassify` and the constant-time building blocks, built,
tested on every lane and documented; then webcpp::hash and webcpp::crypto moved onto it with no copy
and no alias left.

**Architecture:** the prior code (`reference/prior/`) is ported by mechanical renames into
`libs/secure` (namespace `webcpp::secure`, headers `<webcpp/secure/...>`, macros
`WEBCPP_SECURE_*`); new code is the generic `constant_time_equal`, the function `declassify`, the
blocks of `constant_time.hpp`, the wipe's test seam (B9) and the constant-time lane. hash migrates
first (crypto depends on hash), then crypto, each keeping its own test support (O1). The
superproject registers the library and its lane; the controller verifies every lane and delivers.

**Tech Stack:** C++20 header-only, b2 5.5.3, Boost 1.92 (Core's LightweightTest), clang (Apple),
gcc 15, wasi-sdk 34 (wasip2, wasip3), Emscripten 6.0.11, MSVC 14.3 and 14.5 (x64 and arm64 on the
owner's Windows 11 ARM64 VM), Valgrind and clang 18's MemorySanitizer (Linux container), dudect
(crypto's harness, copied), Python tools of the superproject.

**Spec:** this change, `libraries/secure/openspec/changes/genesis/` of webcpporg/proposals:
`proposal.md`, `design.md` (D1 to D11, B1 to B9, owner decisions 1 to 9, O1 decided, all binding),
`specs/secure/spec.md` (binding), `tasks.md`. Read design.md and spec.md before any task.

**Paths:** `$W` is the owner's workspace directory, the folder that holds `heavy.sh`,
`msvc-lane/`, `linux-lint/` and this stream's folder `secure/` (a plan in the proposals repository
names no machine path). `$S` is `$W/secure/webcpp`, the superproject worktree on branch
`secure/genesis`; its `libs/secure`, `libs/hash` and `libs/crypto` are each on branch
`secure/genesis` of their repository (bases: secure `e5870a1`, hash `030e757`, crypto `3314a1d`,
superproject `dccffd7` plus `b1972f8`, the submodule). Every command runs from `$S`. Never create
another worktree or tree; never touch `libs/config` or hkdf-pieces' folders.

**The worked example:** the withdrawn change `timing-guard`, local branches `config/timing-guard`
in `$W/timing-guard/webcpp` (read with `git -C ... show/diff`): config `ba72ad4` (the guard and its
tests, the Jamfile lines), hash `445a403` and `1e0ddf5` (every guard site, its own seam support,
regenerated pages), crypto `c02020d` and `89c3d84` (every guard site, its own seam support,
`timing_nested_hash_test`, dudect, benches) and `4f62e73`'s `timing_nested_hash_test.cpp` (DIT read
inside hkdf through `dit_probe`), superproject `6b39403`, `ae4ef95`, `6e689cb`, `d4206ea` (tools and
guidelines). Its later commits that moved the support into `tools/node-test` (`baf4de3`,
`4f62e73`'s support part, `4803ecd`, `3fd46d6`) are NOT followed (O1), but their
include-anchored `path_in_library` is (below).

**Commands:** `B2` means
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`
(as `$W/hkdf-pieces/lanes/run.sh` runs it). Lanes in this plan:
- native: `B2 toolset=clang <targets>`; `B2 toolset=gcc-15 <targets>`
- wasip2: `B2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh <targets>`
- wasip3: `B2 toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh <targets>`
- Emscripten: `B2 toolset=emscripten <targets>`
- MSVC: `$W/heavy.sh secure $W/msvc-lane/run-lane.sh <14.3|14.5> --tree $W/secure/lane-tree [--arch arm64] -- <targets> [variant=release]`, read with `$W/msvc-lane/summarize.sh <14.3|14.5> <console>` (`new failures:` empty). Commit, then `$W/secure/refresh-tree.sh`, before a run: the lane tree is built from committed heads.
- A whole library from scratch (`b2 -a` over `libs/<l>/test libs/<l>/example`), the full lints and the MSVC lane are heavy: only through `$W/heavy.sh secure <command>`.

**The lint step of every task (owner decision):** before reporting DONE, commit, then run
`$W/secure/lint.sh $W/secure/lint-task<N>.log <every file the task touched, superproject-relative>`
(it refreshes the lane tree itself); fix every finding of one run in the task's own commit
(`<repo>: ... lint fix`), run once more; a task reaches review lint-clean. Format C++ first with
`.local/wasi-sdk/bin/clang-format -i <files>`. Deleted files are not passed.

## Global Constraints

- English in files; never the em dash character, use "-"; commit messages with no trailer at all
  (no Co-Authored-By); never hand-edit CHANGELOG.md or generated files (regenerate with their
  generators); "Copyright (c) 2026 WebCpp.org" BSL-1.0 headers; heavy work only through heavy.sh;
  implementers run the Linux lint on everything their task touched before DONE; flaky tests and lint
  warnings are defects to fix; measurements only on release builds on an idle machine; never contact
  upstream projects; merge origin/main into branches, never rebase; no GitHub CI runs (any push's
  runs get cancelled).
- Commit subjects in each repository's style: `secure: ...`, `hash: ...`, `crypto: ...`;
  superproject `build: ...`, `ci: ...`, `tools: ...`, `guidelines: ...`, and for gitlinks
  `libs/secure, libs/hash, libs/crypto: the gitlinks at <what> (<sha>, <sha>, <sha>)`. Never
  `git add -A` or `git add libs` in the superproject.
- Names, verbatim from the spec: namespace `webcpp::secure` (with `secure::constant_time`,
  `secure::detail`, and `secure::test` for test support and the wipe seam), headers
  `<webcpp/secure.hpp>` and `<webcpp/secure/{config,timing_guard,erase_secret,constant_time_equal,secret,declassify,constant_time}.hpp>`,
  `<webcpp/secure/detail/wipe.hpp>`; include guards `WEBCPP_SECURE_<PATH>_HPP`; macros
  `WEBCPP_SECURE_NO_EXCEPTIONS`, `WEBCPP_SECURE_TIMING_COVERED`, `WEBCPP_SECURE_TIMING_ENTERED(where)`,
  `WEBCPP_SECURE_CT_VALGRIND`, `WEBCPP_SECURE_CT_MSAN`, `WEBCPP_SECURE_TEST_WIPE_RECORD`; target
  `/webcpp/secure//secure`. No name outside `webcpp::secure`; no header of another webcpp library
  but config's; no algorithm; every function `noexcept`, none throws.
- The guard's behaviour is the prior header's, unchanged (D2, D3): engaged runs the seam (a test's
  build) then sets DIT when the CPU has it and it is clear, clears on destruction only if it set
  it; disengaged does nothing, seam included; `isb` after each write; not copyable, not movable;
  no DIT instruction without `cpu_has_dit()` but the Windows probe's one read; coverage
  `defined(__aarch64__) && (defined(__APPLE__) || defined(__linux__))` and
  `defined(_MSC_VER) && defined(_M_ARM64) && !defined(_M_ARM64EC)`; elsewhere an empty class and
  both functions `false`.
- Owner decisions: config stays macros only; crypto's `erase_secret`, `constant_time_equal`,
  `secret<N>`, wipe and declassify move with NO alias; crypto's and hash's `detail/timing.hpp` and
  `detail/wipe.hpp` are deleted; test helpers stay in each library's own `test/support/` (O1), never
  `tools/node-test`, no `webcpp::core`; hex helpers stay where they are.
- Generated files, each written by its generator and checked:
  - hash: `doc/generated/*`, `doc/backends.adoc`, `doc/measurements.adoc`, `doc/llms.txt` by
    `python3 tools/hash/doc_tables.py --write` (check `--check`); `doc/performance.adoc` by
    `python3 libs/hash/bench/collect.py --render-only --data libs/hash/doc/measurements.throughput.json --page libs/hash/doc/performance.adoc`.
  - crypto: `doc/generated/*`, `doc/llms.txt` and `build.jam`'s defaults block by
    `python3 libs/crypto/doc/tables/generate.py --root .` (check `--check`) from
    `doc/tables/crypto.json`; `doc/performance.adoc` by
    `python3 libs/crypto/bench/collect.py --render-only --data libs/crypto/doc/performance.json --page libs/crypto/doc/performance.adoc`.
  - secure: `doc/performance.adoc` by `python3 libs/secure/bench/collect.py` (Task 9).
  - Each library's `doc/changelog.adoc` is hand-written (`AGENTS.md` chapter 5: one line per change,
    past-tense verb, under "Unreleased").
- Every test runs with exceptions on and off (`secure-run`, `secure-compile`, `hash-run`,
  `crypto-run`). A test that defines a seam includes it before any webcpp header, in every
  translation unit of its program (the guard's constructor depends on the macro: ODR).
- The owner's VM: never shut it down, suspend or reconfigure it; `$W/msvc-lane/known-failures.txt`
  is never edited.

## Review Focus

1. **Integer promotion in the blocks on MSVC at `/W4 /WX` and on 8- and 16-bit types.** `~value`,
   `U(0) - bit` and shifts of `std::uint8_t` promote to `int`: a block that forgets a
   `static_cast<U>` returns a wrong mask for `uint8_t` or warns C4146/C4244 on MSVC. Task 6's
   exhaustive 8-bit test and its 16-bit edges catch the value; Task 6 builds `constant_time_test`
   on both MSVC versions, x64 and arm64.
2. **The optimiser turning a mask back into a branch.** clang and gcc at `-O2`/`-Os` may if-convert
   and then branch. Task 8's memcheck programs run at `<variant>release` and again at
   `<optimization>space` (`ctgrind_space`), natively on clang 18 (the lane) and gcc 15 (Task 8's
   extra run), each with the planted branching `select` that must be reported.
3. **`constant_time_equal` on a temporary, on `char` with the high bit set, and on unequal
   sizes.** A `std::string_view` of bytes 0x80 to 0xFF (signed `char`), a temporary `secret<32>`
   (bound to `const&`, viewed through `bytes()` for the call), a `std::byte` array, and a 31- versus
   32-byte pair (false, no byte read: checked with a sequence whose `data()` is null and size 31).
4. **`lookup` past the end and narrow `Element`s.** `is_equal_mask` over `std::size_t` narrowed to
   `std::uint8_t` must stay all ones; an index of `N`, `N + 1` and `SIZE_MAX` yields `Element{}`;
   a table of `std::dynamic_extent` and of one entry; Task 6 pins all of them.
5. **MSVC spells `file_name()` with backslashes.** Each library's seam recorder counts its own
   guards with `path_in_library`, anchored on `include/webcpp/<library>/`, either separator; Task 2
   pins it for secure on the spellings of clang, GCC and MSVC and on near misses
   (`include/webcpp/securex/`, `myinclude/webcpp/secure/`, `libs/secure/test/`); Tasks 10 and 11
   pin hash's and crypto's.

## The renames (Tasks 2 to 5 apply them to `reference/prior/`)

P1. Paths: `<webcpp/config/detail/timing_guard.hpp>` -> `<webcpp/secure/timing_guard.hpp>`;
`<webcpp/config.hpp>` (in the guard) -> `<webcpp/secure/config.hpp>`;
`<webcpp/crypto/config.hpp>` -> `<webcpp/secure/config.hpp>`; `<webcpp/crypto/X.hpp>` ->
`<webcpp/secure/X.hpp>` for `erase_secret`, `constant_time_equal`, `secret`; `detail/wipe.hpp` ->
`<webcpp/secure/detail/wipe.hpp>`; crypto's `<webcpp/crypto/detail/timing.hpp>` ->
`<webcpp/secure/timing_guard.hpp>`; tests' `<webcpp/test/timing_dit.hpp>` ->
`"support/timing_dit.hpp"` and `<webcpp/test/timing_seam.hpp>` -> `"support/timing_seam.hpp"`.
P2. Macros: `WEBCPP_CONFIG_DETAIL_TIMING_GUARD_HPP` -> `WEBCPP_SECURE_TIMING_GUARD_HPP`;
`WEBCPP_CONFIG_DETAIL_TIMING_COVERED` -> `WEBCPP_SECURE_TIMING_COVERED`;
`WEBCPP_CONFIG_DETAIL_TIMING_ENTERED` -> `WEBCPP_SECURE_TIMING_ENTERED`; `WEBCPP_CRYPTO_<X>_HPP` ->
`WEBCPP_SECURE_<X>_HPP`; test guards `WEBCPP_TEST_TIMING_{SEAM,DIT}_HPP` ->
`WEBCPP_TEST_SECURE_SUPPORT_TIMING_{SEAM,DIT}_HPP`; `WEBCPP_TEST_TIMING_LIBRARY` is dropped
(secure's recorder holds `timing_library = "secure"`).
P3. Namespaces: `webcpp::config::detail::{timing_guard, cpu_has_dit, dit_is_set}` ->
`webcpp::secure::{...}`; `webcpp::config::detail::{dit_bit, write_dit, dit_register,
isb_full_system, status_illegal_instruction, probe_dit}` -> `webcpp::secure::detail::{...}` (inside
`webcpp::secure` the guard calls them as `detail::`); `webcpp::crypto` -> `webcpp::secure`;
`webcpp::crypto::detail` -> `webcpp::secure::detail`; tests' `webcpp::test` (the recorder, the DIT
helper) -> `webcpp::secure::test`.
P4. Text: refusal messages `"webcpp::crypto: ..."` -> `"webcpp::secure: ..."`; comments "config's
guard", "config's internal header" -> "secure's guard", "the public header of webcpp::secure"; the
guard header's paragraph "It is config's internal header ... does not include it." becomes "It is
public: crypto, hash and codecs hold it, and a program may hold one around its own constant-time
code."; crypto's `detail::timing_guard const timing_scope;` -> `timing_guard const timing_scope;`;
`detail::wipe_object(bytes_)` -> `detail::wipe(bytes_.data(), bytes_.size())` (secure has no
`wipe_object`, B1); crypto's `detail::dependent_false` is defined in `erase_secret.hpp`'s
`secure::detail` (`template <class> inline constexpr bool dependent_false = false;`).
P5. Tests: `config-run`/`crypto-run` -> `secure-run`; `config-compile` -> `secure-compile`; crypto's
`$(OPTIMISED)` is copied as is.

---

### Task 1: The library's skeleton and configuration

Model tier: standard (multi-file, conventions). Covers tasks.md 1.1, 1.2.

**Files:**
- Create (libs/secure): `meta/libraries.json`, `build.jam`, `AGENTS.md`,
  `include/webcpp/secure.hpp`, `include/webcpp/secure/config.hpp`, `test/Jamfile`,
  `test/config_test.cpp`, `test/.clang-tidy`, `example/Jamfile`, `example/.clang-tidy`,
  `doc/Jamfile`, `doc/secure.adoc`, `doc/changelog.adoc`
- Modify (libs/secure): `README.md` (how to build and test inside the superproject, where its page
  is published; it keeps "being built" until Task 9)
- Copy: `test/.clang-tidy` and `example/.clang-tidy` from libs/hash (the same reasons apply)

**Interfaces:**
- Produces: target `/webcpp/secure//secure`; `<webcpp/secure/config.hpp>` (include guard
  `WEBCPP_SECURE_CONFIG_HPP`, includes `<webcpp/config.hpp>` first, defines
  `WEBCPP_SECURE_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is undefined or a developer defines it;
  under `__MRDOCS__` the documented define-then-undef of xactor's `config.hpp`); test rules
  `secure-run` and `secure-compile` (each also as `<name>_no_exceptions` with
  `<exception-handling>off`).

- [ ] **Step 1: Write the failing test.** `test/config_test.cpp` is hash's skeleton
  `config_test.cpp` (`git -C libs/hash show 59db86a:test/config_test.cpp`) with `HASH` -> `SECURE`,
  `hash` -> `secure`, and without the `WEBASSEMBLY` and tag checks (secure has neither); it also
  checks `#ifndef WEBCPP_VERSION #error ...`. `test/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/secure//secure ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# Every program that runs, runs with exceptions on and off, and every check
# that compiles, compiles both ways: the second has the suffix _no_exceptions
# and is built with exception-handling=off.
rule secure-run ( name : sources + : requirements * : targets * )
{
    webcpp.run $(name) : $(sources) : $(requirements) : $(targets) ;
    webcpp.run $(name)_no_exceptions : $(sources)
      : $(requirements) <exception-handling>off : $(targets) ;
}
rule secure-compile ( name : sources + : requirements * : targets * )
{
    webcpp.compile $(name) : $(sources) : $(requirements) : $(targets) ;
    webcpp.compile $(name)_no_exceptions : $(sources)
      : $(requirements) <exception-handling>off : $(targets) ;
}

webcpp.headers-alone secure : ../include ;

# config.hpp defines WEBCPP_SECURE_NO_EXCEPTIONS exactly when webcpp has no
# exceptions, keeps it when a developer defines it, and defines it when
# WEBCPP_NO_EXCEPTIONS is.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_no_exceptions : config_test.cpp : <exception-handling>off ;
webcpp.compile config_all_off : config_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_SECURE_ALL_OFF ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_SECURE_NO_EXCEPTIONS <define>WEBCPP_TEST_SECURE_DEVELOPER_DEFINED ;
```

- [ ] **Step 2: Run it, expect failure.** `B2 toolset=clang libs/secure/test//config` fails:
  `/webcpp/secure//secure` does not exist.

- [ ] **Step 3: Write the skeleton.** `build.jam`: hash's at `59db86a` with `hash` -> `secure`, its
  comment "secure: the side-channel and secret-in-memory primitives webcpp's libraries share; a
  program names /webcpp/secure//secure: `exe app : app.cpp /webcpp/secure//secure ;`", the alias
  `alias secure : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;`
  between `# tag::secure[]` and `# end::secure[]`. `meta/libraries.json`: key and name `secure`,
  authors and maintainers as hash's, `"description": "The side-channel and secret-in-memory primitives webcpp's libraries share: the DIT timing guard, erase_secret, secret<N>, constant_time_equal, declassify and constant-time building blocks, no algorithm."`,
  `"category": ["Cryptography"]`, `"cxxstd": "20"`, `"port-of": null`. `config.hpp`: xactor's
  (`libs/xactor/include/webcpp/xactor/config.hpp`) with `XACTOR` -> `SECURE`, its doc comment
  saying secure raises no exception and every function is `noexcept`. `secure.hpp`: the
  convenience header, includes `config.hpp` (each later task adds its header). `example/Jamfile`
  and `doc/Jamfile`: hash's skeleton's with `secure` (`webcpp.doc secure : secure.adoc ;`,
  `webcpp.reference secure ;`). `doc/secure.adoc`: hash's skeleton page's frame (title "secure:
  side-channel and secret-in-memory primitives for {cpp}", the reference section, the changelog
  include), one paragraph of what secure is and is not. `doc/changelog.adoc`: hash's skeleton's
  form, "Unreleased" with "Added webcpp::secure's skeleton and configuration." `AGENTS.md`: hash's
  skeleton's form: what secure is (own, no algorithm, the primitives crypto, hash and codecs
  share), cautions (tests through `secure-run`; a test that defines a seam includes it first;
  `detail` is not API: another library's header never includes it, test code may call
  `secure::detail::write_dit`, ruling R1).

- [ ] **Step 4: Run.** `B2 toolset=clang libs/secure/test libs/secure/example`,
  `B2 toolset=gcc-15 libs/secure/test`, `B2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/secure/test`,
  `B2 toolset=emscripten libs/secure/test`, `B2 libs/secure/doc doc` (secure's page and the index,
  which lists secure from `meta/libraries.json`). Expected: pass.

- [ ] **Step 5: Commit, lint.** secure: `secure: the library's skeleton, its configuration and the test rules secure-run and secure-compile`. Lint step with every file created.

---

### Task 2: The guard, its test support and its tests

Model tier: cheap (complete renames of prior code). Covers tasks.md 2.1 (the Windows branch is
ported here with the rest and verified in Task 3).

**Files:**
- Create: `include/webcpp/secure/timing_guard.hpp` from
  `reference/prior/config/include/webcpp/config/detail/timing_guard.hpp` by P1 to P4, Windows
  branch included
- Create: `test/support/timing_seam.hpp` from `reference/prior/node-test/include/webcpp/test/timing_seam.hpp`,
  `test/support/timing_dit.hpp` from `.../timing_dit.hpp`, by P1 to P3
- Create: `test/timing_seam_support_test.cpp` (Step 1)
- Create: `test/timing_guard_{,nesting_,seam_,threads_,windows_}test.cpp` from
  `reference/prior/config/test/` by P1 to P5; `test/timing_guard_header_test.cpp` from
  `git -C $W/timing-guard/webcpp/libs/config show ba72ad4:test/timing_guard_header_test.cpp` by the
  same renames (it checks `WEBCPP_SECURE_TIMING_GUARD_HPP`, `WEBCPP_VERSION`, and the coverage
  conditions above, Windows included)
- Modify: `test/Jamfile`, `include/webcpp/secure.hpp`

**Interfaces:**
- Produces (every later task uses exactly these):

```cpp
// <webcpp/secure/timing_guard.hpp>
// WEBCPP_SECURE_TIMING_COVERED          defined (to 1) where the guard acts
// WEBCPP_SECURE_TIMING_ENTERED(where)   the seam, defined by a test alone before its first include
namespace webcpp::secure {
[[nodiscard]] inline bool cpu_has_dit() noexcept;
[[nodiscard]] inline bool dit_is_set() noexcept;
class timing_guard {
public:
    explicit timing_guard(bool engaged = true) noexcept;                        // without the seam
    explicit timing_guard(bool engaged = true,
        std::source_location where = std::source_location::current()) noexcept;  // with it
    ~timing_guard();   // copy and move deleted
};
namespace detail {   // where WEBCPP_SECURE_TIMING_COVERED; test code may call them (R1)
inline constexpr std::uint64_t dit_bit = std::uint64_t{1} << 24U;
inline void write_dit(std::uint64_t dit_value) noexcept;
}
}
// test/support/timing_seam.hpp, namespace webcpp::secure::test: timing_all_entries,
// timing_entries (secure's own guards), timing_first_entry, timing_last_entry,
// timing_library = "secure", path_separator, path_in_library (anchored on include/webcpp/<lib>/),
// timing_entered(std::source_location const&), and
// #define WEBCPP_SECURE_TIMING_ENTERED(where) (::webcpp::secure::test::timing_entered(where))
// test/support/timing_dit.hpp: webcpp::secure::test::force_dit(bool) noexcept
```

- [ ] **Step 1: Write the failing tests.** Port the six tests and the two support headers. The
  recorder's `#error` checks `WEBCPP_SECURE_TIMING_GUARD_HPP` ("must be included before any webcpp
  header"); the `WEBCPP_TEST_TIMING_LIBRARY` check goes. `timing_guard_seam_test.cpp` keeps its own
  seam, as the prior's. A new `test/timing_seam_support_test.cpp` includes
  `"support/timing_seam.hpp"` first and checks the recorder itself: a guard declared in the test
  raises `timing_all_entries` by one and `timing_entries` by none (the test's file is not under
  `include/webcpp/secure/`), `timing_first_entry` names the test's function; and
  `path_in_library_spellings()`, `path_in_library(path, "secure")` true for
  `/w/libs/secure/include/webcpp/secure/timing_guard.hpp`, `../include/webcpp/secure/x.hpp`,
  `include/webcpp/secure/x.hpp`, `C:\t\a\libs\secure\include\webcpp\secure\x.hpp`,
  `C:\t\a\include\webcpp/secure\detail\wipe.hpp`; false for `/w/libs/secure/test/x_test.cpp`,
  `/w/include/webcpp/securex/x.hpp`, `/w/myinclude/webcpp/secure/x.hpp`,
  `/w/include/webcpp/crypto/x.hpp`, `/w/include/webcpp/secure`. `test/Jamfile` gains (texts of the
  prior Jamfile's comments with secure's names):

```jam
secure-compile timing_guard_header : timing_guard_header_test.cpp ;
secure-run timing_guard : timing_guard_test.cpp ;
secure-run timing_guard_nesting : timing_guard_nesting_test.cpp ;
secure-run timing_guard_seam : timing_guard_seam_test.cpp ;
secure-run timing_guard_threads : timing_guard_threads_test.cpp
  : <threading>multi : native ;
secure-run timing_guard_windows : timing_guard_windows_test.cpp ;
# The test support's recorder: what it counts, and which paths are secure's.
secure-run timing_seam_support : timing_seam_support_test.cpp ;
```

- [ ] **Step 2: Run, expect failure.** `B2 toolset=clang libs/secure/test//timing_guard_header`:
  fails, `<webcpp/secure/timing_guard.hpp>` not found.

- [ ] **Step 3: Port the header** by P1 to P4; `secure.hpp` includes it.

- [ ] **Step 4: Run.** `B2 toolset=clang libs/secure/test`, `B2 toolset=gcc-15 libs/secure/test`,
  wasip2, wasip3 and Emscripten lanes over `libs/secure/test`. Expected: pass; on this Mac
  (arm64, FEAT_DIT) the covered path runs; on WebAssembly `std::is_empty_v<secure::timing_guard>`.
  Then the grep `grep -rnE 'CONFIG_DETAIL|config::detail|webcpp/test/timing' libs/secure` prints
  nothing.

- [ ] **Step 5: Commit, lint.** `secure: timing_guard, cpu_has_dit and dit_is_set, the DIT guard moved from config's withdrawn change, with its seam, its test support and its tests`. Lint step.

---

### Task 3: Windows on ARM64

Model tier: standard (VM runs, judgment on failures). Covers tasks.md 2.2.

**Files:** none planned; a fix, if a run fails, goes to `timing_guard.hpp` or a test, with its
reason in the commit.

- [ ] **Step 1: The lanes, debug.** After `$W/secure/refresh-tree.sh`, each of:

```bash
H="$W/heavy.sh secure"; L=$W/msvc-lane/run-lane.sh; T=$W/secure/lane-tree
$H $L 14.3 --tree $T --arch arm64 -- libs/secure/test
$H $L 14.5 --tree $T --arch arm64 -- libs/secure/test
$H $L 14.3 --tree $T -- libs/secure/test
$H $L 14.5 --tree $T -- libs/secure/test
```
  Expected (`summarize.sh`): every secure test and its `_no_exceptions` twin passed, `new
  failures:` empty; on arm64 `timing_guard_windows` runs its Windows branch (`cpu_has_dit()` true,
  DIT kept across ten sleeps), `timing_guard_header` holds the Windows coverage; on x64 the guard is
  empty.
- [ ] **Step 2: The lanes, release** (the withdrawn change never ran one): the two arm64 lines
  again with `variant=release` after the targets. Expected: as Step 1.
- [ ] **Step 3: Record.** The four consoles' paths and counts go in the task report (Task 9's page
  cites the coverage as verified on MSVC 14.3 and 14.5, debug and release); `known-failures.txt` is
  not edited. A failure is fixed in secure, committed (`secure: ...`), and Steps 1-2 re-run.

---

### Task 4: The wipe, `erase_secret` and `secret<N>`

Model tier: cheap. Covers tasks.md 3.1, 3.2.

**Files:**
- Create: `include/webcpp/secure/detail/wipe.hpp` (from crypto's `detail/wipe.hpp` by P1 to P3,
  without `wipe_object`, with the seam below), `include/webcpp/secure/erase_secret.hpp`,
  `include/webcpp/secure/secret.hpp` (from `reference/prior/crypto/include/` by P1 to P4)
- Create: `test/erase_secret_test.cpp` (from `reference/prior/crypto/test/` by P1 to P5),
  `test/secret_test.cpp` (from `git -C libs/crypto show 3314a1d:test/secret_test.cpp` by P1 to P5),
  `test/wipe_record_test.cpp`, `test/messages/Jamfile`,
  `test/messages/{erase_secret_const_range,secret_of_no_bytes}/` (from crypto's
  `test/messages/` by P1 to P4, project requirement `<library>/webcpp/secure//secure`)
- Modify: `test/Jamfile`, `include/webcpp/secure.hpp`

**Interfaces:**
- Consumes: Task 1's config.
- Produces:

```cpp
namespace webcpp::secure {
namespace detail {
inline void wipe(void* wiped_data, std::size_t byte_count) noexcept;   // volatile stores, then
                                                                      // atomic_signal_fence
template <class> inline constexpr bool dependent_false = false;
template <class Range> concept erasable_range = /* crypto's, unchanged */;
}
template <class Range> requires detail::erasable_range<Range>
void erase_secret(Range&& erased_range) noexcept;          // + the refusing overload, P4 message
template <std::size_t N> class secret;                      // crypto's interface, unchanged (D6)
#ifdef WEBCPP_SECURE_TEST_WIPE_RECORD
namespace test { void wipe_record(void const* wiped_data, std::size_t byte_count) noexcept; }
#endif
}
```
  `detail::wipe` calls `test::wipe_record(wiped_data, byte_count)` after the fence, under the macro
  only (hash's `detail/wipe.hpp` is the model of the seam's text). `secret`'s three wipes and its
  destructor call `detail::wipe(bytes_.data(), N)`.

- [ ] **Step 1: Write the failing tests.** The ported two, and `test/wipe_record_test.cpp` (the
  spec's scenario "A wipe reported to a test"): it defines
  `void webcpp::secure::test::wipe_record(void const*, std::size_t) noexcept` recording each
  call's address, size and whether those bytes read zero at the call; it erases a
  `std::array<std::uint8_t, 32>` filled with 0xA5 (one record: its `data()`, 32, zero), destroys a
  `secret<16>` written with 0x5A in placement storage (one record: the storage's address, 16,
  zero), moves one `secret<16>` into another (records for the source after the move and for the
  target before it takes the bytes), and erases a `std::span(&context, 1)` of a
  `struct context { std::uint64_t key[4]; }` (one record, `sizeof(context)`). `test/Jamfile`:

```jam
OPTIMISED = <optimization>speed <toolset>emscripten:<debug-symbols>off ;
# secure::erase_secret zeroes the bytes of any contiguous range the program may
# write, and a buffer erased as its scope ends stays zero: optimised, with every
# call inlined, where a wipe the compiler could drop as a dead store would be.
secure-run erase_secret : erase_secret_test.cpp : $(OPTIMISED) <inlining>full ;
# secret<N>: not copyable, moves wipe the source, the destructor wipes the
# storage; optimised and inlined as erase_secret's.
secure-run secret : secret_test.cpp : $(OPTIMISED) <inlining>full ;
# The wipe's test seam (B9): every wipe reported once, its bytes already zero.
secure-run wipe_record : wipe_record_test.cpp
  : <define>WEBCPP_SECURE_TEST_WIPE_RECORD ;
build-project messages ;
```
  `test/messages/Jamfile` builds `erase_secret_const_range` (compile-diagnostic, native wasip2
  wasip3, `alone`) and `secret_of_no_bytes` (native wasip2 wasip3), their `expected-error` texts
  saying `webcpp/secure/...hpp` and `webcpp::secure:`.

- [ ] **Step 2: Run, expect failure.** `B2 toolset=clang libs/secure/test//erase_secret`: the
  header is not found.
- [ ] **Step 3: Port the headers** (P1 to P4, the seam above); `secure.hpp` includes
  `erase_secret.hpp` and `secret.hpp`.
- [ ] **Step 4: Run** `libs/secure/test` on native clang, gcc-15, wasip2, wasip3 and Emscripten.
  Expected: pass, the messages included (clang lanes).
- [ ] **Step 5: Commit, lint.** `secure: erase_secret and secret<N>, moved from crypto, over secure's wipe and its test seam`. Lint step.

---

### Task 5: `constant_time_equal` and `declassify`

Model tier: standard (new generic interface). Covers tasks.md 3.3, 3.4.

**Files:**
- Create: `include/webcpp/secure/constant_time_equal.hpp`, `include/webcpp/secure/declassify.hpp`
- Create: `test/constant_time_equal_test.cpp` (from `reference/prior/crypto/test/` by P1 to P5,
  extended below), `test/declassify_compile_test.cpp` (from prior, extended below),
  `test/support/constant_time_marks.hpp`,
  `test/support/valgrind_stub/valgrind/memcheck.h` (crypto's stub by P2, its comment naming
  secure's `declassify`; it adds `VALGRIND_MAKE_MEM_UNDEFINED`, `VALGRIND_GET_VBITS` (returns 0)
  and `VALGRIND_CHECK_VALUE_IS_DEFINED` reading their arguments, for the marks header)
- Modify: `test/Jamfile`, `include/webcpp/secure.hpp`

**Interfaces:**
- Consumes: Task 2's guard and recorder, Task 4's `secret`.
- Produces:

```cpp
namespace webcpp::secure {
namespace detail {
template <class T>   // std::uint8_t, unsigned char, char, std::byte (cv dropped)
concept byte_element = std::same_as<std::remove_cv_t<T>, std::uint8_t> ||
    std::same_as<std::remove_cv_t<T>, unsigned char> || std::same_as<std::remove_cv_t<T>, char> ||
    std::same_as<std::remove_cv_t<T>, std::byte>;
template <class T>   // a contiguous sized range of byte_element, or a type whose bytes() gives one
concept byte_sequence =
    (std::ranges::contiguous_range<T const&> && std::ranges::sized_range<T const&> &&
     byte_element<std::ranges::range_value_t<T const&>>) ||
    requires(T const& value) { { value.bytes() } -> std::convertible_to<std::span<std::uint8_t const>>; };
}
template <class First, class Second>
    requires detail::byte_sequence<First> && detail::byte_sequence<Second>
[[nodiscard]] bool constant_time_equal(First const& first, Second const& second) noexcept;

template <class T>
    requires std::is_trivially_copyable_v<T> && (!std::is_const_v<T>)
void declassify(T& value) noexcept;
}
// test/support/constant_time_marks.hpp, namespace webcpp::secure::test: under
// WEBCPP_SECURE_CT_VALGRIND `struct memcheck`, under WEBCPP_SECURE_CT_MSAN
// `struct memory_sanitizer`, each { exact_subtraction; reads_destroyed;
// mark_secret(std::span<std::uint8_t>); mark_public(std::span<std::uint8_t const>);
// std::optional<bool> traced(std::span<std::uint8_t const>); verdict(bool const&); }
// (crypto's ctgrind_test.cpp and msan_test.cpp structs, moved), plus
// template <class Checker> bool carries_mark(std::span<std::uint8_t const>) (crypto's paths.hpp).
```
  `constant_time_equal`: a guard first (`timing_guard const timing_scope;`); each operand viewed as
  its `bytes()` when it has one, else as its range; sizes differ -> `false` without reading; else
  every element read through `static_cast<std::uint8_t>(element)` (no `reinterpret_cast`), XOR
  accumulated with OR in `unsigned volatile`, reduced as crypto's `((total - 1U) >> 8U) & 1U`.
  Includes no Boost.Hash2 header. `declassify`: empty in a program's build and including nothing;
  under `WEBCPP_SECURE_CT_VALGRIND` `static_cast<void>(VALGRIND_MAKE_MEM_DEFINED(&value, sizeof value))`
  with `<valgrind/memcheck.h>`; under `WEBCPP_SECURE_CT_MSAN` `__msan_unpoison(&value, sizeof value)`
  with `<sanitizer/msan_interface.h>`; both defined -> `#error "define WEBCPP_SECURE_CT_VALGRIND or WEBCPP_SECURE_CT_MSAN, not both"`.

- [ ] **Step 1: Write the failing tests.** `constant_time_equal_test.cpp` includes
  `"support/timing_seam.hpp"` first and checks, besides the prior's cases: a 32-byte sequence
  against itself and against each of its 256 one-bit flips, as `std::span<std::uint8_t const>`,
  `std::array<std::uint8_t, 32>`, `std::array<std::byte, 32>`, `secret<32>` (lvalue and a temporary
  from a function), `std::string_view` holding bytes 0x80 to 0x9F, and `boost::hash2::digest<32>`
  (mixed pairs too: digest against array, secret against span); only the identical compare equal;
  31 against 32 bytes is `false`, and so is a null `data()` of size 31 against 32 (sizes compared
  first); two empty spans are equal; each call raises `timing_entries` by exactly one and its
  `timing_first_entry` names `constant_time_equal`; static_asserts that a `std::list<char>`, a
  `std::vector<std::uint16_t>` and an `int` are not `detail::byte_sequence`.
  `declassify_compile_test.cpp`: includes `<webcpp/secure.hpp>` and every header, defines nothing
  itself; with the stub it calls `declassify` on a `bool`, a `std::uint32_t` and a
  `std::array<std::uint8_t, 4>` and branches on each; static_asserts that `declassify` is not
  callable on a `const bool` or a `std::string`. `test/Jamfile`:

```jam
# constant_time_equal over any two byte sequences, its guard through the seam.
secure-run constant_time_equal : constant_time_equal_test.cpp ;
# declassify with the memcheck build's marks, on every lane, against a stand-in
# for Valgrind's header; and with MemorySanitizer's under clang natively.
secure-run declassify_compile : declassify_compile_test.cpp
  : <define>WEBCPP_SECURE_CT_VALGRIND <include>support/valgrind_stub ;
secure-compile declassify_compile_msan : declassify_compile_test.cpp
  : <define>WEBCPP_SECURE_CT_MSAN <toolset>gcc:<build>no <toolset>msvc:<build>no
  : native ;
```

- [ ] **Step 2: Run, expect failure.** `B2 toolset=clang libs/secure/test//constant_time_equal`.
- [ ] **Step 3: Write both headers** (interfaces above; doc comments as crypto's, P4); the marks
  header moves crypto's `memcheck` and `memory_sanitizer` structs and `carries_mark` verbatim but
  for P3; `secure.hpp` includes both headers.
- [ ] **Step 4: Run** `libs/secure/test` on all five local lanes. Expected: pass; `grep -n hash2
  libs/secure/include -r` prints nothing.
- [ ] **Step 5: Commit, lint.** `secure: constant_time_equal over any two byte sequences, and declassify with its checkers' marks as test support`. Lint step.

---

### Task 6: The constant-time building blocks

Model tier: most capable (barriers, promotions, MSVC, constexpr). Covers tasks.md 4.1.

**Files:**
- Create: `include/webcpp/secure/constant_time.hpp`, `test/constant_time_test.cpp`
- Modify: `test/Jamfile` (`secure-run constant_time : constant_time_test.cpp ;`),
  `include/webcpp/secure.hpp`

**Interfaces:**
- Produces, verbatim from D8, every function `constexpr` and `noexcept`:

```cpp
namespace webcpp::secure::constant_time {
template <std::unsigned_integral U> constexpr U value_barrier(U value) noexcept;
template <std::unsigned_integral U> constexpr U mask_from_bit(U bit) noexcept;
template <std::unsigned_integral U> constexpr U is_zero_mask(U value) noexcept;
template <std::unsigned_integral U> constexpr U is_equal_mask(U first, U second) noexcept;
template <std::unsigned_integral U> constexpr U is_less_mask(U first, U second) noexcept;
template <std::unsigned_integral U> constexpr U is_in_range_mask(U value, U low, U high) noexcept;
template <std::unsigned_integral U> constexpr U select(U mask, U when_set, U when_clear) noexcept;
template <std::unsigned_integral Element, std::size_t N>
constexpr Element lookup(std::span<Element const, N> table, std::size_t index) noexcept;
constexpr void select_bytes(std::uint8_t mask, std::span<std::uint8_t> destination,
                            std::span<std::uint8_t const> when_set,
                            std::span<std::uint8_t const> when_clear) noexcept;
constexpr std::uint8_t equal_mask(std::span<std::uint8_t const> first,
                                  std::span<std::uint8_t const> second) noexcept;
constexpr std::uint8_t is_zero_mask(std::span<std::uint8_t const> bytes) noexcept;
constexpr std::uint8_t less_mask(std::span<std::uint8_t const> first,
                                 std::span<std::uint8_t const> second) noexcept;  // big-endian
}
```
  The algorithms (BoringSSL's `constant_time_*`), each operation cast back to `U` (Review Focus 1),
  with `bits = std::numeric_limits<U>::digits` and `msb(x) = x >> (bits - 1)`:
  - `value_barrier`: `if (std::is_constant_evaluated()) return value;` then, on GCC and clang but
    not WebAssembly, `__asm__("" : "+r"(value));`; on MSVC and WebAssembly (`__wasm__`) a round trip
    through a `U volatile` local (ruling R6).
  - `mask_from_bit(bit) = value_barrier(U(U(0) - bit))`; `is_zero_mask(v) = mask_from_bit(msb(U(~v & U(v - 1))))`;
    `is_equal_mask(a, b) = is_zero_mask(U(a ^ b))`;
    `is_less_mask(a, b) = mask_from_bit(msb(U(a ^ U(U(a ^ b) | U(U(a - b) ^ a)))))`;
    `is_in_range_mask(v, lo, hi) = U(U(~is_less_mask(v, lo)) & U(~is_less_mask(hi, v)))`;
    `select(m, s, c) = U(U(value_barrier(m) & s) | U(U(~value_barrier(m)) & c))`.
  - `lookup`: `Element result{}`; for every `i` of the table, in order,
    `result |= table[i] & static_cast<Element>(is_equal_mask<std::size_t>(i, index))`.
  - `select_bytes`: `BOOST_ASSERT` on equal sizes; unequal -> return without writing; else each
    byte `select<std::uint8_t>(mask, s[i], c[i])`.
  - `equal_mask`, `is_zero_mask` (bytes): unequal sizes -> 0 without reading (`BOOST_ASSERT`
    first); OR of XORs (of bytes), then `is_zero_mask<std::uint8_t>`.
  - `less_mask`: unequal sizes -> 0 without reading; from the last byte to the first,
    `std::uint16_t difference = first[i] - second[i] - borrow; borrow = (difference >> 8) & 1`;
    the result `mask_from_bit<std::uint8_t>(borrow)`.

- [ ] **Step 1: Write the failing test** `constant_time_test.cpp`, with a plain reference
  `template <class U> constexpr U reference_mask(bool on) { return on ? U(~U(0)) : U(0); }`:
  - every pair `a, b` of `std::uint8_t`: `mask_from_bit(U(a & 1)) == reference_mask(a & 1)`,
    `is_zero_mask(a) == reference_mask(a == 0)`, `is_equal_mask(a, b) == reference_mask(a == b)`,
    `is_less_mask(a, b) == reference_mask(a < b)`, `select(reference_mask(b & 1), a, U(~a))`
    equals `(b & 1) ? a : U(~a)`, `value_barrier(a) == a`;
  - every triple of `std::uint8_t`: `is_in_range_mask(v, lo, hi) == reference_mask(lo <= v && v <= hi)`;
  - `std::uint16_t`, `std::uint32_t`, `std::uint64_t`: the edges {0, 1, 2, max/2, max/2 + 1,
    max - 1, max} in every pair (triple for the range), then 2^16 random values from
    `std::mt19937_64{20261124}`, same checks;
  - `lookup` over a 64-entry `std::uint8_t` table (`i * 7 + 3`) and a 32-entry `std::uint32_t`
    table at every index, at 64, 65 and `SIZE_MAX` (-> 0), over a `std::span<..., std::dynamic_extent>`,
    and over a one-entry table;
  - byte comparisons over random sequences of 0, 1, 31 and 32 bytes and their edges (all 0x00,
    all 0xFF, differing only in the first or the last byte): `equal_mask` vs `std::ranges::equal`,
    `is_zero_mask` vs `std::ranges::all_of(== 0)`, `less_mask` vs
    `std::ranges::lexicographical_compare` (big-endian of equal sizes); `select_bytes` with
    `mask` 0 and 0xFF writes `when_clear` or `when_set` exactly;
  - `static_assert`s evaluating `select`, `is_in_range_mask`, `lookup` and `equal_mask` in constant
    expressions; `bool` refused, through a concept probe
    `template <class U> concept maskable = requires(U v) { constant_time::is_equal_mask(v, v); };`
    with `static_assert(maskable<std::uint8_t> && !maskable<bool> && !maskable<int>)`.
- [ ] **Step 2: Run, expect failure.** `B2 toolset=clang libs/secure/test//constant_time`.
- [ ] **Step 3: Write `constant_time.hpp`** (above; Doc Comments say what each block covers and
  what the compiler may still do, D8); `secure.hpp` includes it.
- [ ] **Step 4: Run** `libs/secure/test` on all five local lanes, and the MSVC lanes of Task 3 on
  `libs/secure/test//constant_time` (14.3 and 14.5, x64 and arm64, debug). Expected: pass, no
  warning (`/W4 /WX`).
- [ ] **Step 5: Commit, lint.** `secure: the constant-time building blocks, masks, select, lookup and byte comparisons, checked against a plain reference`. Lint step.

---

### Task 7: No heap, the convenience header, a user's Jamfile

Model tier: cheap. Covers tasks.md 4.2 and the spec's scenario "A program that erases a buffer
builds nothing".

**Files:**
- Create: `test/allocation_test.cpp` (counting `operator new` from
  `libs/crypto/test/support/counting.hpp`'s form, copied into `test/support/counting.hpp` by P3),
  `test/convenience_header_test.cpp`
- Create (superproject): `tools/test/secure_jamfiles_test.py` (crypto_jamfiles_test.py's form,
  libraries `('config', 'secure')`), `tools/test/fixtures/secure_jamfiles/program/`
  (`Jamfile`: `exe app : app.cpp /webcpp/secure//secure ;`; `app.cpp` includes
  `<webcpp/secure.hpp>`, holds a `secure::timing_guard`, compares two buffers with
  `secure::constant_time_equal`, erases one with `secure::erase_secret`, prints `equal` and
  `erased`; outcome: targets native wasip2 wasip3 emscripten, build succeeds, run prints both
  lines), `program_no_exceptions/` (the same with `<exception-handling>off` in its `exe`)
- Modify: `test/Jamfile`

- [ ] **Step 1: Write the failing tests.** `allocation_test.cpp` calls every function of the
  library (each block for `std::uint8_t` and `std::uint64_t`, `lookup`, the byte comparisons,
  `select_bytes`, `constant_time_equal` on every operand kind of Task 5, `erase_secret`,
  `secret<32>`'s construction, move, `clone`, `bytes`, destruction, a guard engaged and
  disengaged, `cpu_has_dit`, `dit_is_set`, `declassify`) and checks the counter is zero after each.
  `convenience_header_test.cpp` includes `<webcpp/secure.hpp>` alone and names one function of
  each header. Jamfile: `secure-run allocation : allocation_test.cpp ;`,
  `secure-run convenience_header : convenience_header_test.cpp ;`.
- [ ] **Step 2: Run, expect failure** (missing files); then write the tests' bodies until they
  pass; no library change is expected (a failure here is a finding in the library).
- [ ] **Step 3: Run** `libs/secure/test` on all five local lanes and
  `python3 tools/test/secure_jamfiles_test.py`. Expected: pass.
- [ ] **Step 4: Commit, lint.** secure: `secure: no function allocates, and the convenience header compiles alone`; superproject: `tools: secure_jamfiles_test, a user's Jamfile of webcpp::secure on every target`. Lint step.

---

### Task 8: The constant-time lane

Model tier: most capable (checkers' semantics, planted leaks, CI wiring). Covers tasks.md 5.1.

**Files (libs/secure/test/constant_time/):**
- Create: `Jamfile`, `ctgrind_test.cpp`, `ctgrind_planted_test.cpp` (`#include "ctgrind_test.cpp"`
  with its NOLINT, as crypto's), `msan_test.cpp`, `msan_planted_test.cpp`, `paths.hpp`,
  `dudect.hpp` (crypto's `test/constant_time/dudect.hpp` copied, P3, its DIT report on
  `secure::cpu_has_dit()` and `secure::dit_is_set()` under `WEBCPP_SECURE_TIMING_COVERED`),
  `dudect_test.cpp`, `dudect_planted_test.cpp`, `dudect_harness_test.cpp` (crypto's, P3),
  `dudect_judge.py` (crypto's; only its docstring changes), `valgrind.mjs` (crypto's without the
  three suppression files), `msan_report.mjs` (crypto's), `package.json` and `package-lock.json`
  (name `webcpp-secure-constant-time`)
- Modify: `libs/secure/test/Jamfile` (`build-project constant_time ;`)
- Modify (superproject): `tools/ci/matrix.py` (`OWN_LANE_PACKAGES` gains
  `('secure', 'constant-time', 'native'): ('valgrind', 'llvm-18', 'libclang-rt-18-dev')` and its
  comment names secure), `tools/ci/matrix_test.py` (its assertion on `OWN_LANE_PACKAGES` and the
  lint-packages expectation), `.github/workflows/library.yml` (the "Judge dudect" step runs for
  `matrix.lane == 'constant-time' && (matrix.library == 'crypto' || matrix.library == 'secure')`
  and calls `libs/${{ matrix.library }}/test/constant_time/dudect_judge.py`; its comment says so),
  `tools/test/secure_dudect_judge_test.py` (new: imports `crypto_dudect_judge_test`, sets its
  `JUDGE` to secure's judge and runs its `main`, as `crypto_jamfiles_format_test.py` imports its
  sibling)
- Stream tool (not committed): `$W/secure/ct-run/linux.sh` and `inside.sh`, copied from
  `$W/crypto-sign/ct-run/` with the tree `$W/secure/lane-tree`, the container `ct-secure`, the
  volume `ct-secure-work`, and the library as their first argument (`--library "$1"`)

**Interfaces:**
- Consumes: `test/support/constant_time_marks.hpp` (Task 5), every block (Task 6).
- Produces: the own lane `constant-time` of secure (`webcpp.lane constant-time : ... : native ;`).

- [ ] **Step 1: Write the lane, planted leaks first.** `paths.hpp` holds
  `template <class Checker> int common_paths()`: for each block on `std::uint8_t`, `std::uint32_t`
  and `std::uint64_t`, its operands marked secret (`Checker::mark_secret` over their bytes), its
  result checked `carries_mark` where `Checker::exact_subtraction` (memcheck) and not checked
  where not (MemorySanitizer drops a subtraction's carry: crypto's msan_test.cpp says why);
  `lookup` with the index secret over a 64-entry table; `select_bytes` with the mask secret;
  `equal_mask`, `is_zero_mask`, `less_mask` with both operands secret; `constant_time_equal` with
  both secret, its `bool` passed to `Checker::verdict` only after `secure::declassify` (the mark's
  test); `secret<32>`'s move, move assignment, `clone` and destruction with its bytes secret.
  Planted, each a `[[gnu::noinline]]` function of its own so that a report's first frame names it:
  `planted_select` (`if (mask != 0) { planted_sink(when_set); }` with a `noinline` sink, so that no
  compiler if-converts it) and `planted_lookup` (`return table[index];`), each called with its
  mask or index itself marked secret, under `WEBCPP_TEST_SECURE_PLANTED_SELECT` or
  `WEBCPP_TEST_SECURE_PLANTED_LOOKUP`, run before the paths. `dudect_test.cpp`: cases
  `constant_time_equal` (class 0: equal 64-byte inputs; class 1: random second), `lookup` (index
  0 vs random over 64 entries), `select_bytes` (mask 0x00 vs 0xFF), `less_mask` (equal vs random
  32 bytes), each with dudect.hpp's A/A control; `dudect_planted_test.cpp`: an early-exit
  comparison of 4096 bytes, class 0 differing at byte 0, class 1 equal, which must exceed the
  threshold (judged everywhere). The Jamfile:

```jam
project : requirements <library>/webcpp/secure//secure ;
import webcpp os ;
webcpp.targets native ;
webcpp.original node ;
# linux-only and clang-on-linux: crypto's test/constant_time/Jamfile rules, copied.
CTGRIND = <variant>release <debug-symbols>on <conditional>@linux-only
    <define>WEBCPP_SECURE_CT_VALGRIND <webcpp-drive-timeout>300 ;
webcpp.drive ctgrind : ctgrind_test.cpp : $(CTGRIND) : valgrind.mjs : : native ;
webcpp.drive ctgrind_space : ctgrind_test.cpp : $(CTGRIND) <optimization>space
  : valgrind.mjs : : native ;
webcpp.drive ctgrind_planted_select : ctgrind_planted_test.cpp
  : $(CTGRIND) <define>WEBCPP_TEST_SECURE_PLANTED_SELECT
  : valgrind.mjs : --expect-leak --in planted_select : native ;
webcpp.drive ctgrind_planted_lookup : ctgrind_planted_test.cpp
  : $(CTGRIND) <define>WEBCPP_TEST_SECURE_PLANTED_LOOKUP <location-prefix>ctgrind_planted_lookup
  : valgrind.mjs : --expect-leak --in planted_lookup : native ;
MSAN_FLAGS = -fsanitize=memory -fno-sanitize-memory-param-retval ;
MSAN = <define>WEBCPP_SECURE_CT_MSAN <cxxflags>$(MSAN_FLAGS) <linkflags>-fsanitize=memory
    <variant>release <debug-symbols>on <conditional>@clang-on-linux ;
webcpp.run msan : msan_test.cpp : $(MSAN) : native ;
webcpp.drive msan_planted_select : msan_planted_test.cpp
  : $(MSAN) <define>WEBCPP_TEST_SECURE_PLANTED_SELECT
  : msan_report.mjs : --in planted_select : native ;
webcpp.drive msan_planted_lookup : msan_planted_test.cpp
  : $(MSAN) <define>WEBCPP_TEST_SECURE_PLANTED_LOOKUP <location-prefix>msan_planted_lookup
  : msan_report.mjs : --in planted_lookup : native ;
DUDECT = <variant>release ;
if [ os.environ RUNNER_ENVIRONMENT ] = github-hosted
{
    DUDECT += <define>WEBCPP_TEST_DUDECT_UNJUDGED ;
}
webcpp.run dudect : dudect_test.cpp : $(DUDECT) : native ;
webcpp.run dudect_planted : dudect_planted_test.cpp : <variant>release : native ;
webcpp.run dudect_harness : dudect_harness_test.cpp : : native ;
webcpp.lane constant-time : ctgrind ctgrind_space ctgrind_planted_select
    ctgrind_planted_lookup msan msan_planted_select msan_planted_lookup dudect
    dudect_planted dudect_harness
  : native ;
```
  (with crypto's comments' reasons, secure's words: what is checked, the planted leaks, dudect on
  hosted runners).
- [ ] **Step 2: See the planted leaks fail and the rest pass, natively on Linux.**
  `$W/heavy.sh secure $W/secure/ct-run/linux.sh secure` (commit and refresh first). Expected: the
  lane exits 0: memcheck and MemorySanitizer pass on the paths and report each planted function in
  its own frame (the drivers' `--in` passes only then), `dudect` judges every case below the
  threshold with its A/A control, `dudect_planted` exceeds it. Then once more with
  `toolset=gcc-15` for the memcheck programs (Review Focus 2): in the container,
  `b2 toolset=gcc-15 libs/secure/test/constant_time//ctgrind libs/secure/test/constant_time//ctgrind_planted_select`
  (if the container has no gcc-15, the run's report says so and the clang lane stands).
- [ ] **Step 3: On this Mac** (dudect only; the rest is `<build>no`):
  `B2 toolset=clang libs/secure/test/constant_time//dudect libs/secure/test/constant_time//dudect_planted libs/secure/test/constant_time//dudect_harness`.
  Expected: pass, DIT reported set inside the calls where the CPU has it, clear outside.
- [ ] **Step 4: The superproject's wiring.** Run `python3 tools/ci/matrix_test.py`,
  `python3 tools/test/crypto_dudect_judge_test.py`, `python3 tools/test/secure_dudect_judge_test.py`,
  `python3 tools/ci/matrix.py own-lanes --library secure --user-config .local/user-config.jam`
  (one entry, native, its apt packages). Expected: pass.
- [ ] **Step 5: Commit, lint.** secure: `secure: the constant-time lane, memcheck, MemorySanitizer and dudect over every block, each with a planted leak that must fail`; superproject: `ci: secure's constant-time lane, its packages and dudect's judge`. Lint step (the lint analyses the lane's programs natively with the lane's packages).

---

### Task 9: The documentation and the guard's measured cost

Model tier: standard. Covers tasks.md 6.1.

**Files:**
- Create: `doc/overview.adoc` (or sections in `secure.adoc`, as hash's page includes its sections):
  what secure is and is not (no algorithm; crypto, hash and codecs use it); the guard (D3's
  coverage table, verified as Task 3 recorded, nesting, `engaged`, the seam, what DIT covers and
  not, the probe's absent path untested); erasing (`erase_secret`, a span of one for a C
  library's object, the wipe seam for tests); `secret<N>` (crypto's warning kept);
  `constant_time_equal`; `declassify`; the blocks with their limits (what masks and
  `value_barrier` cover and what the compiler may still do, `lookup`'s cost); the measured cost;
  the changelog
- Create: `example/{guard,erase,secret,compare,hex_digit}.cpp` with `.expected` beside each
  (`hex_digit.cpp`: a 4-bit value to its hex character and back by `is_in_range_mask` and
  `select`, libsodium's method), each `webcpp.example` in `example/Jamfile`, quoted in the page by
  tags
- Create: `bench/Jamfile`, `bench/timing.cpp` (hash's `bench/timing.cpp` on
  `<webcpp/secure/timing_guard.hpp>`, P3; `timing_check` in every lane, `timing` at
  `<variant>release`, explicit), `bench/collect.py` (hash's `bench/collect.py` reduced to its
  `--timing` and `--render-only` paths: it writes `doc/measurements.json` and renders
  `doc/performance.adoc`, refusing a program not built at `variant=release`), and in
  `test/Jamfile` `build-project ../bench ;`
- Create (superproject): `tools/test/secure_collect_test.py` (hash_collect_test.py's timing
  cases on secure's collect.py)
- Modify: `doc/changelog.adoc` (Unreleased: one line per public header), `README.md` (no longer
  "being built"), `AGENTS.md` (the generated page and how to measure)

- [ ] **Step 1: Write the failing tests:** the examples' `.expected` files and
  `secure_collect_test.py`; run `B2 toolset=clang libs/secure/example` and
  `python3 tools/test/secure_collect_test.py`. Expected: FAIL (missing programs, missing script).
- [ ] **Step 2: Write the examples, the bench and collect.py; run** the examples on the five local
  lanes, `B2 toolset=clang libs/secure/bench//timing_check`, the collect test. Expected: pass.
- [ ] **Step 3: Measure on an idle machine.** Only when no `$W/HEAVY.lock*` slot is held and
  `uptime`'s 1-minute load is below 2: `B2 toolset=clang libs/secure/bench//timing`, then
  `python3 libs/secure/bench/collect.py --timing <the built timing> --data libs/secure/doc/measurements.json --page libs/secure/doc/performance.adoc`.
  Never `--provisional`; if the machine is not idle, wait (the controller may defer this step to
  the delivery, before the review).
- [ ] **Step 4: Build the page.** `B2 libs/secure/doc doc`. Expected: the page with every section
  of the spec's requirement "Documentation", the strict reference, the index entry.
- [ ] **Step 5: Commit, lint.** secure: `secure: the page, its examples, and the guard's cost measured from a release build`; superproject: `tools: secure_collect_test`. Lint step.

---

### Task 10: hash on secure

Model tier: standard (multi-file, worked example `445a403` + `1e0ddf5`). Covers tasks.md 7.

Before it the controller runs `git -C libs/hash fetch origin` and, if `origin/main` moved past
`030e757` (hkdf-pieces may have landed new guarded functions), `git -C libs/hash merge origin/main`
(never rebase), and `B2 toolset=clang libs/hash/test//timing`. The guards and wipes are found by
grep at execution time, never from a list.

**Files:**
- Delete: `libs/hash/include/webcpp/hash/detail/timing.hpp`, `.../detail/wipe.hpp`
- Modify: every file `grep -rlE 'timing_guard|TIMING_(ENTERED|COVERED)|detail/timing\.hpp|detail/wipe\.hpp|cpu_has_dit|dit_is_set|write_dit|dit_bit|(^|[^_[:alnum:]])wipe(_object)?\(|WIPE_RECORD|wipe_record|crypto::constant_time_equal|crypto/constant_time_equal' libs/hash`
  prints (at planning time: the guarded headers `argon2.hpp`, `bcrypt.hpp`, `blake2b_subkey.hpp`,
  `blake3.hpp`, `digest.hpp`, `hkdf.hpp`, `kdf_standards.hpp`, `mac.hpp`, `pbkdf2.hpp`,
  `scrypt.hpp`, the wiping headers, `async.hpp`, `backends/*.hpp`, `hmac.hpp`; `test/support/timing_seam.hpp`,
  `timing_dit.hpp`, `timing_coverage.hpp`; every `test/timing_*_test.cpp`; the five tests of the
  wipe seam; `test/Jamfile`; `bench/timing.cpp`, `bench/Jamfile`, `bench/collect.py`; the MAC
  examples and `example/Jamfile`; `AGENTS.md`; `meta/include-boundaries.json`; the pages' sources
  `doc/*.adoc`, `doc/guide.json`; `doc/changelog.adoc`)
- Modify: `libs/hash/build.jam` (`<library>/webcpp/secure//secure` in `hash`'s usage requirements)
- Modify (crypto, so that crypto still builds): delete `libs/crypto/test/timing_hash_interop_test.cpp`
  and its three Jamfile lines (worked example `c02020d`)
- Modify (superproject): `tools/hash/doc_tables.py` (the DIT section: secure's
  `webcpp::secure::timing_guard`, Windows on ARM64 covered; the guard "secure's, shared with crypto
  and codecs"), `tools/test/hash_doc_tables_test.py` if it pins that text,
  `tools/test/hash_jamfiles_test.py` and `tools/test/crypto_jamfiles_test.py` (libraries gain
  `'secure'`)
- Regenerate: hash's generated pages (Global Constraints)

**Interfaces:**
- Consumes: Tasks 2 and 4 (`secure::timing_guard`, `secure::erase_secret`, the wipe seam).
- Produces: hash's `test/support/timing_seam.hpp` in the form of secure's recorder (Task 2) with
  `namespace webcpp::test` kept, `timing_library = "hash"`, `path_in_library` anchored on
  `include/webcpp/<library>/`, and `#define WEBCPP_SECURE_TIMING_ENTERED(where) (::webcpp::test::timing_entered(where))`;
  its `#error` checks `WEBCPP_SECURE_TIMING_GUARD_HPP`; hash's `support/timing_dit.hpp` calls
  `secure::detail::write_dit` (R1).

**Substitution rules** (Task 11 applies them to crypto with `crypto` for `hash`):
- S1. `#include <webcpp/hash/detail/timing.hpp>` -> `#include <webcpp/secure/timing_guard.hpp>`;
  `<webcpp/hash/detail/wipe.hpp>` -> `<webcpp/secure/erase_secret.hpp>`; clang-format orders them.
- S2. Every use of `timing_guard`, `cpu_has_dit`, `dit_is_set`, whatever qualifies it, becomes
  `secure::timing_guard` (etc.; inside `webcpp::hash`, `secure::` finds `webcpp::secure`);
  `write_dit` and `dit_bit` (tests and benches only) become `secure::detail::...`. Guards keep their
  variable and argument: `detail::timing_guard const timing{keyed_};` ->
  `secure::timing_guard const timing{keyed_};`.
- S3. `WEBCPP_HASH_DETAIL_TIMING_COVERED` -> `WEBCPP_SECURE_TIMING_COVERED`;
  `WEBCPP_HASH_DETAIL_TIMING_ENTERED` -> `WEBCPP_SECURE_TIMING_ENTERED`;
  `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD` -> `WEBCPP_SECURE_TEST_WIPE_RECORD`; a test's definition of
  `webcpp::hash::detail::test::wipe_record` becomes `webcpp::secure::test::wipe_record` (same
  signature).
- S4. Wipes (B1): `detail::wipe(bytes)` of a byte span -> `secure::erase_secret(bytes)`;
  `wipe(x.data(), x.size())` -> `secure::erase_secret(x)`; `wipe(p, n)` of a prefix ->
  `secure::erase_secret(std::span(x).first(n))`; `wipe_object(object)` of an array ->
  `secure::erase_secret(object)`, of any other object -> `secure::erase_secret(std::span(&object, 1))`.
  The bytes zeroed (address and size) stay the same, which the wipe-seam tests confirm.
- S5. Prose: `detail::timing_guard` -> `webcpp::secure::timing_guard` (secure's guard);
  "hash's own copy", "a copy of crypto's", "a change to one is a change to the other" and
  `timing_hash_interop_test` go; "macOS, iOS, Linux and Android" gains Windows on ARM64; "depends on
  no other webcpp library" becomes "depends on config and secure";
  `webcpp::crypto::constant_time_equal` -> `webcpp::secure::constant_time_equal` (and its header).
- S6. Every count of a guard declared in the test file itself reads `timing_all_entries`; every
  count of hash's own guards reads `timing_entries` (worked example's S5).
- Completion grep: `grep -rnE 'WEBCPP_HASH_DETAIL_(TIMING|TEST_WIPE)|hash/detail/(timing|wipe)\.hpp|hash::detail::(timing_guard|cpu_has_dit|dit_is_set|write_dit|dit_bit|wipe)|crypto::constant_time_equal|timing_hash_interop' libs/hash tools CODING-GUIDELINES.md`
  prints only `libs/hash/doc/*.json` measurement strings (data collect.py wrote; kept) and
  `CODING-GUIDELINES.md` lines Task 12 rewrites.

- [ ] **Step 1: Point hash's seam support at secure's seam and test it.** Rewrite the recorder's
  top and `timing_dit.hpp` as Interfaces say; add `path_in_library_spellings()` to
  `test/timing_test.cpp` (Task 2's cases with `hash`); `seam_fires` reads `timing_all_entries`.
- [ ] **Step 2: Run, expect failure.**
  `B2 toolset=clang libs/hash/test//timing libs/hash/test//timing_coverage_hash2 libs/hash/test//wipe`:
  hash's headers still hold hash's own guard and wipe seam, which the support no longer defines.
- [ ] **Step 3: Migrate** by S1 to S6 every file the grep prints; delete the two headers, their
  entries in `meta/include-boundaries.json` and `test/Jamfile`'s headers-alone list; hash's MAC
  examples include `<webcpp/secure/constant_time_equal.hpp>`, and `example/Jamfile` keeps
  `<library>/webcpp/crypto//crypto` only where a crypto name remains (`random_device`); delete
  crypto's interop test; `bench/collect.py`'s paragraph names secure's guard ("Elsewhere (x86,
  WebAssembly) it executes no instruction"), then `--render-only` (no measurement: the guard's
  instructions on Apple and Linux are unchanged, R9); `tools/hash/doc_tables.py --write`;
  `AGENTS.md` and `doc/changelog.adoc` (Unreleased: "Changed every keyed function to hold
  webcpp::secure's timing guard, which also sets DIT on Windows on ARM64, and every wipe to
  secure's erase_secret; hash depends on secure.").
- [ ] **Step 4: Run.** `B2 toolset=clang libs/hash/test libs/hash/example libs/hash/bench//timing_check`,
  `B2 toolset=gcc-15 libs/hash/test//timing libs/hash/test//timing_coverage_hash2 libs/hash/test//timing_keyed libs/hash/test//timing_unseamed libs/hash/test//wipe`,
  `B2 toolset=clang libs/hash/test//timing_unseamed_release`,
  wasip2 `libs/hash/test//timing libs/hash/test//timing_coverage_hash2 libs/hash/test//wipe`,
  Emscripten `libs/hash/test//timing libs/hash/test//timing_async`,
  `B2 toolset=clang libs/crypto/test`, `python3 tools/hash/doc_tables.py --check`,
  `python3 tools/test/hash_doc_tables_test.py`, `python3 tools/test/hash_jamfiles_test.py`,
  `python3 tools/test/crypto_jamfiles_test.py`, `B2 libs/hash/doc`, the completion grep.
  Expected: pass; the grep prints only what it allows.
- [ ] **Step 5: Commit, lint.** hash: `hash: every keyed function holds webcpp::secure's timing guard and every wipe is secure's erase_secret; hash's copies are gone and hash depends on secure`;
  crypto: `crypto: timing_hash_interop goes with hash's copy of the guard`; superproject:
  `tools: hash's DIT section names secure's guard and Windows on ARM64, and the user Jamfile tests copy libs/secure`.
  Lint step.

---

### Task 11: crypto on secure

Model tier: most capable (largest change, judgment at each wipe and declassify). Covers tasks.md 8.
Before it the controller runs `git -C libs/crypto fetch origin` and merges `origin/main` if it
moved (never rebase).

**Files:**
- Delete: `libs/crypto/include/webcpp/crypto/{detail/timing,detail/wipe,detail/declassify,erase_secret,constant_time_equal,secret}.hpp`;
  `test/{erase_secret,constant_time_equal,secret,declassify_compile,wipe}_test.cpp` (moved to
  secure in Tasks 4 and 5; `wipe_test.cpp`'s `detail::exceeds` half stays as `test/limits_test.cpp`
  with its Jamfile line), `test/support/valgrind_stub/` (ruling R4),
  `test/messages/{erase_secret_const_range,secret_of_no_bytes}/` (moved),
  `tools/test/fixtures/crypto_jamfiles/secret_of_no_bytes_on_emscripten/` (superproject: moved to
  `tools/test/fixtures/secure_jamfiles/` with secure's message)
- Modify: every file `grep -rlE 'timing_guard|TIMING_(ENTERED|COVERED)|detail/timing\.hpp|cpu_has_dit|dit_is_set|write_dit|dit_bit|detail/wipe\.hpp|(^|[^_[:alnum:]])wipe(_object)?\(|DECLASSIFY|declassify\.hpp|CT_VALGRIND|CT_MSAN|erase_secret|constant_time_equal|(^|[^_[:alnum:]])secret<|crypto/secret\.hpp' libs/crypto tools/test/fixtures/crypto_jamfiles`
  prints (at planning time 49 files for the guard, 22 for the wipe, 16 for declassify, about 90
  for erase, comparison and secret: the headers, tests, `test/constant_time/*`, `test/Jamfile`'s
  headers-alone list, `bench/*`, `doc/*.adoc`, `doc/tables/crypto.json`, `AGENTS.md`,
  `doc/changelog.adoc`, the fixtures)
- Modify: `libs/crypto/build.jam` (`<library>/webcpp/secure//secure` in `crypto`'s usage
  requirements)
- Create: `test/support/timing_dit.hpp` (crypto's DIT helper, hash's form of Task 10 with
  `WEBCPP_TEST_CRYPTO_SUPPORT_TIMING_DIT_HPP`), `test/timing_nested_hash_test.cpp` (worked example
  `4f62e73`'s file, with its includes on crypto's own support: `"support/timing_seam.hpp"` first,
  `"support/timing_dit.hpp"`, and `<webcpp/secure/timing_guard.hpp>`), its Jamfile line where the
  interop test was; `test/messages/{constant_time_equal_moved,erase_secret_moved,secret_moved}/`
  (each a compile-diagnostic: `webcpp::crypto::constant_time_equal`, `erase_secret`, `secret<16>`
  named with `<webcpp/crypto.hpp>` included, `expected-error` "no member named ... in namespace
  'webcpp::crypto'" or clang's "no type named" form, as `equal_removed` writes it)
- Modify: `test/messages/{equal_removed,wipe_removed}/*.cpp` include `<webcpp/crypto/error.hpp>`
  in place of the deleted headers; `test/messages/Jamfile`
- Modify (superproject): `tools/hash/doc_tables.py` (the sentence naming crypto's
  `timing_nested_hash_test`, as `ae4ef95`), then `tools/hash/doc_tables.py --write`
- Regenerate: crypto's generated pages and `performance.adoc` (Global Constraints)

**Interfaces:**
- Consumes: every name of secure; hash on secure (Task 10).
- Produces: crypto's `test/support/timing_seam.hpp` as hash's of Task 10 with
  `timing_library = "crypto"` (crypto's own `named_function` machinery below the top kept).

**Substitution rules:** S1 to S6 of Task 10 with `crypto` for `hash`, and:
- S7. `crypto::erase_secret`, `crypto::constant_time_equal`, `crypto::secret<N>` (and unqualified
  uses inside `webcpp::crypto`) -> `secure::erase_secret`, `secure::constant_time_equal`,
  `secure::secret<N>`, always qualified (a class with a member `secret()` would otherwise find the
  function); includes of the deleted headers -> secure's.
- S8. `WEBCPP_CRYPTO_DETAIL_DECLASSIFY(&x, sizeof x);` -> `secure::declassify(x);` (every site is
  of this form; `x` is never const there); `WEBCPP_CRYPTO_DETAIL_CT_VALGRIND` ->
  `WEBCPP_SECURE_CT_VALGRIND`, `WEBCPP_CRYPTO_DETAIL_CT_MSAN` -> `WEBCPP_SECURE_CT_MSAN` (in
  `curves/secp256k1.hpp`, `test/constant_time/Jamfile` and its comments).
- S9. dudect (`test/constant_time/dudect.hpp`, `dudect_sodium_test.cpp`) and benches
  (`bench/timing.cpp`, `bench/Jamfile`, `bench/collect.py`) read `secure::cpu_has_dit()` and
  `secure::dit_is_set()` under `WEBCPP_SECURE_TIMING_COVERED`, "not applicable here (not arm64
  under macOS, Linux or Windows)".
- Completion grep: `grep -rnE 'WEBCPP_CRYPTO_DETAIL_(TIMING|DECLASSIFY|CT_)|crypto/detail/(timing|wipe|declassify)\.hpp|crypto/(erase_secret|constant_time_equal|secret)\.hpp|crypto::(erase_secret|constant_time_equal|secret<)|detail::wipe(_object)?|timing_hash_interop' libs tools CODING-GUIDELINES.md`
  prints only `libs/*/doc/*.json` measurement strings, crypto's own moved-name messages, and
  `CODING-GUIDELINES.md` lines Task 12 rewrites; the spec's scenario "No copy remains" holds:
  `grep -rnE 'class timing_guard|void erase_secret|bool constant_time_equal|void wipe|class secret[^_[:alnum:]]|define [A-Z_]*DECLASSIFY' libs/hash/include libs/crypto/include`
  prints nothing.

- [ ] **Step 1: Point crypto's seam support at secure's seam, add its DIT helper, write the nested
  test**; `timing_test.cpp` gains `path_in_library_spellings()` with crypto's paths, `seam_fires`
  reads `timing_all_entries`; write the three moved-name messages.
- [ ] **Step 2: Run, expect failure.**
  `B2 toolset=clang libs/crypto/test//timing_nested_hash libs/crypto/test//timing libs/crypto/test//timing_mock`:
  crypto's headers still hold crypto's own guard.
- [ ] **Step 3: Migrate** by S1 to S9; delete the files above; `doc/backends.adoc`,
  `doc/x25519.adoc` and `doc/tables/crypto.json` say Windows on ARM64 is covered (worked example
  `89c3d84`'s texts, secure's names) and name `secure::secret<N>` and `secure::constant_time_equal`;
  `generate.py --root .`; `bench/collect.py --render-only` (R9); `AGENTS.md`;
  `doc/changelog.adoc` (Unreleased: "Changed every public operation to hold webcpp::secure's timing
  guard, also on Windows on ARM64, and moved erase_secret, constant_time_equal and secret<N> to
  webcpp::secure, with no alias; crypto depends on secure."). The test/messages that include
  `<webcpp/crypto.hpp>` stay; any `expected-error` naming a moved header is updated.
- [ ] **Step 4: Run.** `B2 toolset=clang libs/crypto/test libs/crypto/example libs/crypto/bench//timing_check`,
  `B2 toolset=gcc-15 libs/crypto/test//timing libs/crypto/test//timing_nested_hash libs/crypto/test//timing_mock libs/crypto/test//timing_coverage_sodium libs/crypto/test//timing_unseamed`,
  `B2 toolset=clang libs/crypto/test//timing_unseamed_release`,
  wasip2 `libs/crypto/test//timing libs/crypto/test//timing_nested_hash libs/crypto/test//timing_coverage_sodium`,
  Emscripten `libs/crypto/test//timing libs/crypto/test//timing_nested_hash`,
  `B2 toolset=clang libs/crypto/test/constant_time//dudect_sodium libs/crypto/test/constant_time//dudect_harness`,
  `$W/heavy.sh secure $W/secure/ct-run/linux.sh crypto` (crypto's lane natively on Linux, after a
  commit and refresh), `B2 toolset=clang libs/hash/test//timing`,
  `python3 libs/crypto/doc/tables/generate.py --root . --check`,
  `python3 tools/test/crypto_jamfiles_test.py`, `python3 tools/test/secure_jamfiles_test.py`,
  `B2 libs/crypto/doc`, both completion greps. Expected: all pass.
- [ ] **Step 5: Commit, lint.** crypto: `crypto: every public operation holds webcpp::secure's timing guard, and erase_secret, constant_time_equal, secret<N>, the wipe and declassify are secure's, with no alias`;
  superproject: `tools: hash's DIT section names crypto's timing_nested_hash_test, and secret_of_no_bytes is secure's fixture`.
  Lint step.

---

### Task 12: The superproject

Model tier: standard. Covers tasks.md 9.

The registration mirrors hash's (`647813c`, `3724513`) and webrtc's (`aa0ea3a`). Every place the
superproject names a library, found at planning time:

| File | Change | Task |
| --- | --- | --- |
| `.gitmodules`, `libs/secure` | the submodule | done (`b1972f8`) |
| `AGENTS.md` | the libraries table gains secure's row ("the side-channel and secret-in-memory primitives crypto, hash and codecs share; no algorithm; webcpp's own" / native, wasip2, wasip3, emscripten; its own lane `constant-time` / a submodule at `libs/secure`) and hash's row, missing today (its targets, own lane `c-suites`, "depends on config and secure"); crypto's row says it depends on hash and secure; the layout's list of `libs/<name>/` gains hash and secure | 12 |
| `doc/releases/0.1.0.adoc` | New Libraries: `link:{library-pages}secure/{library-page}[secure]`, one sentence, "from R."; Tools and build: one line each for `secure_jamfiles_test.py`, `secure_collect_test.py`, `secure_dudect_judge_test.py`, secure's constant-time lane in `matrix.py` and `library.yml`, and the jamfiles tests copying libs/secure | 12 |
| `CODING-GUIDELINES.md` | chapter 17: "### secure (side-channel and secret-in-memory primitives; own; accepted 2026-11-24, built, not merged)" with its decisions (D1 to D11, B1 to B9, O1) each with its `Source:`; chapter 15 crypto: its DIT bullet, `constant_time_equal` bullet and every `crypto::erase_secret`/`secret`/wipe mention name secure's; chapter 17 hash: its DIT bullet and wipe mentions; chapter 1's list of tools tests gains the secure tests if that list names hash's | 12 |
| `tools/ci/matrix.py`, `tools/ci/matrix_test.py`, `.github/workflows/library.yml` | secure's lane, packages, judge | 8 |
| `tools/test/crypto_jamfiles_test.py`, `tools/test/hash_jamfiles_test.py` | `'secure'` copied | 10 |
| `tools/test/secure_jamfiles_test.py`, `secure_collect_test.py`, `secure_dudect_judge_test.py`, `tools/test/fixtures/secure_jamfiles/` | new | 7, 8, 9, 11 |
| `tools/hash/doc_tables.py` | DIT text | 10, 11 |
| the index page `doc/index.adoc` | nothing: it lists `libs/*/meta/libraries.json` (checked by `b2 doc`) | 1 |
| `README.md` | nothing: its table lists neither hash, config, cobalt nor webrtc today (reported, not changed) | - |
| `tools/lint/lint.sh` | nothing: its canary list (`crypto hash`) is of C libraries' canaries; secure has none | - |

- [ ] **Step 1: Write** the three files' changes above.
- [ ] **Step 2: The gitlinks.** `git add libs/secure libs/hash libs/crypto` (nothing else) and
  commit `libs/secure, libs/hash, libs/crypto: the gitlinks at secure built and hash and crypto on it (<sha>, <sha>, <sha>)`.
- [ ] **Step 3: Run.** `B2 doc`, `python3 tools/test/doc_test.py`, `python3 tools/release/release.py`'s
  check of the notes if it runs offline (`AGENTS.md` chapter 5 names it), and the completion greps
  of Tasks 10 and 11 over `CODING-GUIDELINES.md` (now nothing).
- [ ] **Step 4: Commit, lint.** `build: register secure, a library of the superproject`,
  `guidelines: secure is accepted and built, in chapter 17, and crypto's and hash's chapters name its guard, erase, comparison and secret`.
  Lint step.

---

### Task 13: The delivery (a checklist the controller runs)

Model tier: the controller; the final review by the most capable model.

- [ ] hash: `git -C libs/hash fetch origin`; if `origin/main` moved (hkdf-pieces landed), merge it
  (never rebase), migrate any new guard site and wipe by Task 10's rules, re-run Task 10's Step 4,
  commit, update the gitlink. crypto and the superproject: the same merge of `origin/main` if it
  moved.
- [ ] Commit everything; `$W/secure/refresh-tree.sh`.
- [ ] Lanes, each from scratch, one build directory each, through heavy.sh, for `L` in
  `secure hash crypto`:

```bash
B2="env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam"
H="$W/heavy.sh secure"
$H $B2 -a --dump-tests --build-dir=bin/lane-clang --out-xml=clang.$L.xml toolset=clang libs/$L/test libs/$L/example
$H $B2 -a --dump-tests --build-dir=bin/lane-gcc-15 --out-xml=gcc-15.$L.xml toolset=gcc-15 libs/$L/test libs/$L/example
$H $B2 -a --dump-tests --build-dir=bin/lane-wasip2 --out-xml=wasip2.$L.xml toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/$L/test libs/$L/example
$H $B2 -a --dump-tests --build-dir=bin/lane-wasip3 --out-xml=wasip3.$L.xml toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh libs/$L/test libs/$L/example
$H $B2 -a --dump-tests --build-dir=bin/lane-emscripten --out-xml=emscripten.$L.xml toolset=emscripten libs/$L/test libs/$L/example
```
  (wasip2 and wasip3 with `WEBCPP_WASIP2_TEST_TIMEOUT=1800 WEBCPP_WASIP3_TEST_TIMEOUT=1800`, as
  hkdf-pieces' run.sh), plus `toolset=clang variant=release libs/$L/test`; every test with its
  `_no_exceptions` twin.
- [ ] The constant-time lanes: `$H $W/secure/ct-run/linux.sh secure`, `$H $W/secure/ct-run/linux.sh crypto`,
  natively on this Mac the dudect programs of both, and crypto's wasip2 half
  (`$H $B2 -a --build-dir=bin/lane-ct-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test/constant_time//constant-time`).
- [ ] MSVC on the VM: `run-lane.sh 14.3` and `14.5`, x64 and `--arch arm64`, over
  `libs/secure/test libs/secure/example libs/hash/test libs/hash/example libs/crypto/test libs/crypto/example`;
  release on arm64 for `libs/secure/test`; on arm64 also
  `libs/crypto/test/constant_time//dudect_sodium` (dudect's X25519 on Windows ARM64 with DIT set by
  the library). Each `summarize.sh`: `new failures:` empty.
- [ ] Docs and tools: `$B2 doc`, the generators' `--check`s, every `tools/test/*_test.py` the
  stream touched, `python3 tools/ci/matrix_test.py`, `$B2 libs/{secure,hash,crypto}/bench//timing_check`.
- [ ] The report: `python3 tools/report/report.py` over the XML files, every cell green.
- [ ] Lints: macOS `$H tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`;
  Linux full `$H $W/linux-lint/run.sh --full $W/secure/lane-tree`. Clean.
- [ ] Task 9's measurement, if deferred, on an idle machine.
- [ ] The final whole-branch review (most capable model) over the four repositories; fixes in
  their own commits; lanes that a fix touches re-run.
- [ ] Pull requests, in merge order: secure, hash, crypto (each merged with a merge commit), then
  the superproject (gitlinks at the merged mains, `tools/`, guidelines, notes), then the proposals
  (this change; its status moves on with the archive when the owner says). Merged by the owner's
  standing instruction or by the owner; no CI gates them (pushes' runs are cancelled); one CI at
  the end on the superproject's main.

## Rulings (the planner's)

- **R1:** the guard's register helpers (`dit_bit`, `write_dit`, the Windows constants,
  `probe_dit`) are `webcpp::secure::detail`; the public names are `timing_guard`, `cpu_has_dit`,
  `dit_is_set`. Test code (secure's, hash's and crypto's `force_dit`, their benches) may call
  `secure::detail::write_dit`; no library header of another library names `secure::detail`.
- **R2 (B9, added to design D4 and the spec):** the wipe's test seam
  `WEBCPP_SECURE_TEST_WIPE_RECORD` / `secure::test::wipe_record`, hash's moved: deleting hash's
  `detail/wipe.hpp` would otherwise remove the evidence of five hash tests.
- **R3:** every `detail::wipe`/`wipe_object` of hash and crypto becomes `secure::erase_secret` over
  a span (B1); no library of webcpp calls `secure::detail::wipe`.
- **R4:** crypto's `declassify_compile_test` and `valgrind_stub` go: `secure::declassify(x)` is a
  call whose argument every build compiles, which was the test's reason; secure's
  `declassify_compile_test` covers the marks.
- **R5:** `constant_time_equal` accepts `secret<N>` through its `bytes()` (it is not a range), and
  exactly the four element types the spec names.
- **R6:** `value_barrier` is a volatile round trip on MSVC and on WebAssembly (where an
  `asm` register constraint is not relied on), an empty `asm` with `"+r"` elsewhere.
- **R7:** secure's constant-time lane runs natively only (memcheck and MemorySanitizer on Linux,
  dudect natively everywhere, D10); its own copies of dudect.hpp, the harness test and the judge
  (O1); the CI's judge step runs the library's own judge; locally the Linux half runs in the
  linux-lint image by the stream's `ct-run/linux.sh`, as crypto-sign's did.
- **R8:** the spec's scenario "A program that erases a buffer builds nothing" is a user Jamfile
  fixture (`secure_jamfiles_test.py`), and crypto's `secret_of_no_bytes_on_emscripten` fixture moves
  there with secure's message.
- **R9:** hash's and crypto's guard-cost benches stay, on secure's guard, rendered again
  (`--render-only`), not re-measured: the guard's instructions on Apple and Linux are unchanged.
  secure measures its own once, on an idle machine (D11).
- **R10:** crypto gains three moved-name messages, as it keeps `equal_removed`, so that a program
  still naming `crypto::constant_time_equal`, `erase_secret` or `secret` fails with that name.
- **R11:** tasks.md's chapter 17 entry moves from task 1.2 to the superproject's task (9.1), with
  every other superproject registration, so that it records the library as built.
- **R12:** AGENTS.md gains hash's missing row and layout name with secure's, since its
  dependencies change; README.md's libraries table, which lacks hash, config, cobalt and webrtc
  alike, is reported, not changed.
- **R13:** the Windows branch is ported with the rest in Task 2 (it is the prior's, built and once
  verified); Task 3 verifies it on the VM, release included, which the withdrawn change never ran.
