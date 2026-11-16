# webcpp::hash: where the implementation stands, and how to start

Written 2026-11-14, when this proposal was made, brought up to date on 2026-11-15, when increment 1
was built, and on 2026-11-16, when the owner fixed the default backends (design D33). The owner
decided the library's shape on 2026-11-14, widened it the same day (every backend's whole hashing
and KDF surface, in increments; one profile for every default backend, retired on 2026-11-16;
documentation in noble's and libsodium's manner) and answered O1 (the browser backend through a
parallel asynchronous family), and accepted it. On 2026-11-16 he decided that every key derivation
defaults to `backends::hash2` on every target and link set, and `argon2id` and `scrypt` to
`backends::sodium`, which retires the profiles, the overrides, the measured ranking and the lane
`measure` (design D33). `spec.md` is the specification, `plan.md` the implementation plan of 48
tasks (0 to 47) in seven increments, `../tasks.md` the checklist. **The reference code is the code
written in the plan's tasks** (listed in `../proposal.md`, "Implementation"). There is no `source/`
directory: increment 1's code is in the library's own repository, which the owner publishes at gate
G1.

## Where it stands

- **Increment 1 is built** (the plan's Tasks 0 to 24, `../tasks.md` group 1): the library's own
  repository, branch `main` (local), and the superproject's integration branch `hash/v0.1`
  (local), rebased onto webcpp::crypto's final version 0.1. Every task was reviewed, every
  Important finding fixed and re-reviewed. Nothing is pushed: `webcpporg/hash` has not been
  created (gate G1), the submodule not added (G2).
- **Green locally, from scratch:** native, wasip2, wasip3, Emscripten and gcc-15 (the library's
  tests and examples, each with its `_no_exceptions` twin), the own lanes `c-suites` (XKCP's,
  BLAKE3's and libsodium's own suites, 105 programs on every target) and `measure` (284 cells on
  four targets, retired since by design D33), the tools' tests, the documentation's build and the
  lint (`lint: clean`); an amd64 lane in the CI's image under emulation. MSVC and Apple clang run
  only in the CI, after the gates (their cases are written).
- **The spec and the design are amended** to what increment 1 built (design D25 to D32): the
  public tags `kdfs::hkdf_extract` and `kdfs::hkdf_expand` and a default backend per operation;
  `detail::backend_limits`, with OpenSSL refusing past `INT_MAX`; libsodium's
  `-sALLOW_MEMORY_GROWTH=1` on Emscripten; the overrides' refusals in `detail/overrides.hpp`
  behind `kdf.hpp`; BLAKE3's own program in the lane `c-suites` instead of
  `tools/test/blake3_c_suite.py`; `[[nodiscard]]` on every result; two comparison tables a
  chapter; the native orders provisional.
- **The default backends are fixed (design D33, 2026-11-16), and built** (`../tasks.md` 1.26) on
  the branches `hash/defaults-hash2` of the library and of the superproject: `default_backend`
  answers `backends::hash2` (`backends::sodium` for `argon2id` and `scrypt`); the profiles, the
  umbrella's reading, the overrides with `detail/overrides.hpp`, `detail/ranking.hpp`, their
  `detect_mismatch`, tests and messages, the lane `measure` and `tools/hash/measure.py` are
  removed; `tools/hash/doc_tables.py` renders `doc/backends.adoc` and the dated
  `doc/measurements.adoc`.
- **The measurements are a dated reference.** `doc/measurements.*` stay, labelled as measured on
  2026-11-16, and decide nothing; no re-measurement is due before a release (design D32 is
  superseded).
- **The pins** are those read on 2026-11-14: libsodium 1.0.22
  (`77e1ce5d6dee871c49ef211222ba18ef0c486bda`, tag `1.0.22-RELEASE`), XKCP `master`
  (`d20c8fcaecc543f816a0721d0dfc13b4cf27c14d`, 2026-11-10; XKCP has no release tags), BLAKE3
  1.8.7 (`f3149ec5bb5449af877ba20377a11008ff499fa2`), BLAKE2's test vectors
  (`ed1974ea83433eba7b2d95c5dcd9ac33cb847913`, increment 2), libsodium-doc
  (`6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`, ISC, for the documentation), Botan's latest tag
  3.14.1 (`01af88c`, for increment 6's reading). BearSSL and OpenSSL are the superproject's
  (`7bea48e`, `45e844f`).
- **Increments 2 to 7 are not started.** Every step of `plan.md` keeps its box as written;
  `../tasks.md` records what is done.

## What it waits on

- **The owner's gates** G1, G2 and G4 for increment 1 (`plan.md`, "Owner gates"): creating
  `webcpporg/hash`, adding `libs/hash`, merging the pull requests; MSVC and Apple clang then run in
  the CI. G5 opens the Boost.Hash2 contribution, whose text is ready (`doc/upstream.adoc`). No
  push, no pull request and no CI run by an agent; all evidence is local until the owner asks.
- **webcpp::crypto's version 0.1 on `main`**, which increment 1 is built on
  (`/third_party//bearssl`, `webcpp.same-program`).
- **The owner's merge of the branches `hash/defaults-hash2`** (task 1.26, design D33).
- **webcpp::crypto's Botan provisioning** (its increment 5) for increment 6.
- **webcpp::asio's `browser::async_await`** on `main` for increment 7.

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1): Boost 1.92, b2 5.5.3, a C++20
compiler, wasi-sdk 34, wasmtime, Emscripten 6.0.11 with node 26.7.0, Python 3.9 or newer,
`.local/user-config.jam`.

1. **Increment 1** is done (Tasks 0 to 24, and task 1.26 of design D33 on the branches
   `hash/defaults-hash2`); what remains of it is the owner's: the gates G1, G2 and G4, then the
   CI's MSVC and Apple clang lanes.
2. **Increment 2 starts with Task 25** (the families), from the library's `main` and the
   superproject's integration branch; it waits for nothing else.
3. **Increments 3 to 7** after Task 25, in parallel streams by backend, as `plan.md`'s table of
   streams says; Task 47 closes each.

Every b2 command runs as `$B2` (`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
--user-config=.local/user-config.jam`); only a build from scratch (`-a`) counts as evidence. The
lanes and their commands are in `plan.md`'s "Global Constraints".

## What to verify first

1. The library's `main` builds and passes from scratch on the native lane:
   `$B2 -a libs/hash/test libs/hash/example`, every test and its `_no_exceptions` twin.
2. `default_backend_test` and `default_backend_former_defines` show `backends::hash2` for every
   key derivation (`backends::sodium` for `argon2id` and `scrypt`) on the four targets with every
   library linked, and no `WEBCPP_HASH_PROFILE_*`, `WEBCPP_PROFILE_*` or
   `WEBCPP_HASH_DEFAULT_BACKEND_*` is read by `include/` (design D33).
3. The rulebook and the lint: `lint: clean`, with `libs/hash/meta/lint.json`'s
   "nodiscard results" rule (design D30).

## Decisions pending

- **O3** (design, Open Questions): the default of the families of increments 2 to 7 for an
  algorithm Boost.Hash2 lacks, before increment 2's first task.

O1 is decided (design D23). O2 is closed by design D33: no measured order decides a default.
The owner's gates are actions, not decisions.
