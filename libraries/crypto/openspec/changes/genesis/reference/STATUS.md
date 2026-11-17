# webcpp::crypto: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal was made from the owner-approved specification and the
plan; revised 2026-11-14, when the owner set version 0.1's scope and the plan was patched to the
superproject's `main` of that day; revised 2026-11-17, with version 0.1 merged and the owner's
decision on increments 3 to 5. `spec.md` is the specification (approved by the owner on
2026-10-11); `plan.md` is the implementation plan, whose sections "The revision of 2026-11-17"
and "The revision of 2026-11-14" rule over the rest of it, the first over the second;
`../tasks.md` is the checklist; `../design.md` D22 and D23 record the revisions. **The reference code is the code written in the plan's tasks**, and version 0.1's
code on `webcpporg/crypto`'s `main`.

## Where it stands (2026-11-17)

- **Version 0.1 is merged.** `webcpporg/crypto` `main` is at `d242be8` and the superproject's
  `main` (`3fa7ca5`) records it: increment 1 lean and increment 2 (Tasks 0 to 14, 17 to 19, 21,
  24 to 30a), with the renames into `ciphers::` and `curves::secp256k1::`, the x-only tweak and
  the tables of `doc/tables/crypto.json`. Gates G1 and G2 are passed. It is recorded as built
  (task 7.3) once the CI is green on both `main`s: on 2026-11-17 the library's last run on
  `main` was cancelled and the superproject's was queued.
- **The owner's decision of 2026-11-17** (`../design.md` D23): increments 3 to 5 without
  trystero's migration, in one order with the foundation change, whose
  `reference/roadmap.md` is the order of all the remaining work. Tasks 31 and 32 are dropped;
  Task 33 moves into crypto's own tests; Tasks 35, 38 and 39 are built by the foundation (its
  tasks 1.8b, 7.3a and 7.4) and Task 37 is superseded by its Botan submodule; Tasks 20, 22 and
  23 are built by the foundation (its tasks 1.14a, 1.14, 1.11 and 4.4), Task 22 re-cut to
  release builds that decide nothing.
- **What this change still builds itself:** stream M, the memory option (Tasks 15, 16, 33 and
  34); and, behind the owner's gates O2 and O3, Task 25b and NIST CAVP (stream N).
- **The upstream pins** of 2026-11-14 hold for the memory option: OpenSSL 3.5.9 at `45e844f`,
  the superproject's source-built provider.
- **Specification status:** accepted by the owner on 2026-10-11, amended by the provider
  amendment of 2026-11-03, D22 (2026-11-14) and D23 (2026-11-17); the foundation's MODIFIED and
  REMOVED requirements amend it further.

## What was measured, and when

Everything below is stated in `spec.md` or `plan.md`; none of it was re-run for this proposal.

- **2026-10-11, macOS arm64, Apple clang 21, OpenSSL 3.6.4 (spec, PROBE-1011 and PROBE-R3):**
  the first OpenSSL `make` of a process made 6,061 allocations (and 192 reallocations under the
  memory functions, 4,621 blocks live afterwards); a second `make` 2; seals and opens 0;
  OpenSSL's GCM accepts IVs of 1 to 128 bytes and refuses 129 and more; with the functions
  installed libcrypto called the C library's allocator 0 times and without them 7,584 times;
  `CRYPTO_set_mem_functions` returns 0 after any allocation by OpenSSL's own functions and 1
  after the first make through installed ones; an `ERR_set_mark` before the install makes it
  return 0. A failure in OpenSSL's first initialisation is permanent (n = 0, 5, 50).
- **2026-10-11, the plan's own measurements M1 to M7** (macOS arm64, b2 5.5.3): `/third_party`
  and `/webcpp/third_party` both resolve from one Jamroot; a skipped program does not evaluate a
  library's stop; NIST's archives' sizes and SHA-256; RFC 8439's two vectors through OpenSSL
  3.6.4; an interposer linked at launch counts the same as `DYLD_INSERT_LIBRARIES`; upstream
  commits (Wycheproof `12fd3aaf33eb5fa1f52e026912ee00c054f9d984`, `bitcoin/bips`
  `927b6de9915c9262615a6399de51b200f81e5aa4`, BearSSL's mirror master); and the tree at the time.
- **Throughput and sizes (spec section 5 and 8.8, from the owner's research):** AES-GCM on
  x86-64 (AMD EPYC 7763) 3,333 to 3,509 MiB/s on OpenSSL against 2,373 to 2,496 on BearSSL;
  Linux aarch64 2,821 to 2,825 against 92.4 to 92.5; macOS-15 (Apple M1) 5,571 to 5,908 against
  90.1 to 94.1; BearSSL 146 to 160 MiB/s on WebAssembly. libsecp256k1 verification 2,937
  against 1,744 per second on wasip2 with large against small tables; an Emscripten program of
  1,211,251 bytes with the large tables and 141,922 with the small ones.
- **Not measured yet (the spec says so):** the Windows branch of the memory option (recognising
  OpenSSL's defaults through `GetProcAddress`, `OPENSSL_cleanup` at a DLL's unload), the counts on
  OpenSSL 3.0.13 (the CI's Linux version), and MSVC's messages.

## How to resume

Prerequisites are the superproject's (AGENTS.md chapter 1, "Prerequisites"): Boost 1.92, b2
5.5.3, a C++20 compiler, wasi-sdk 34, wasmtime 50.0.1, Emscripten 6.0.11 with node 26.7.0,
Python 3.9 or newer, and a `.local/user-config.jam`.

1. Read the foundation's `reference/roadmap.md`, then `plan.md`'s revision of 2026-11-17.
2. **Stream M** (branch `crypto/openssl-memory` of the library and of the superproject, from
   `origin/main`): Tasks 15, 16, 33 (in `libs/crypto/test/openssl_memory/`) and 34, each re-read
   against the source-built provider as their notes say, each test written first and seen
   failing, the lanes, the lint, the commit (the contributor's own identity, no trailer, staged
   by path). It runs beside the foundation's streams A and B.
3. **Stream R:** once the CI is green on both `main`s at commits that hold version 0.1, a
   proposals pull request checks task 7.3 and records the release and the implementation
   commit in `.openspec.yaml` as CONTRIBUTING.md says.

### The commands

From the superproject worktree's root. The plan writes `$B2` for
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.

| What | Command |
| --- | --- |
| one test, while working on it | `$B2 libs/crypto/test//<name>` |
| native, from scratch | `$B2 -a --build-dir=bin/lane-native libs/crypto/test libs/crypto/example` |
| native with gcc 15 | `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/crypto/test libs/crypto/example` |
| wasip2 | `$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test libs/crypto/example` |
| wasip3 | the same with `clang-wasip3`, `tools/boost/asio-wasip3/run.sh` and `bin/lane-wasip3` |
| Emscripten | `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/crypto/test libs/crypto/example` |
| the own lane `c-suites` on a target | the target's command with `libs/crypto/test/c_suites//c-suites` in place of the two directories |
| the user's Jamfiles | `python3 tools/test/crypto_jamfiles_test.py` |
| the lint | `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`, last line `lint: clean` |

Only a build from scratch (`-a`) counts as evidence. MSVC and Apple clang run in the CI only.

## What to verify first

1. Stream M's base: `libs/crypto/include/webcpp/crypto/backends/openssl.hpp` on the
   superproject's `origin/main`, with the error values 13 and 14 reserved and undeclared in
   `error.hpp`, and `/webcpp/third_party//openssl` and `/third_party//libdatachannel` building
   natively.
2. After Task 15: the adapter's tests pass on native clang and gcc-15, with and without
   exceptions.
3. After Task 33: the TLS handshake under the option passes natively with no foreign block
   freed through the adapter.

## Known issues

- **The plan was written on one machine** (macOS arm64); the Linux, Windows and arm64 facts are
  the CI's to establish. MSVC's and Apple clang's evidence comes from the CI only.
- **The source-built OpenSSL is compiled without assembly on MSVC, wasip2 and wasip3,** so its
  AES there is table code; the owner kept the spec's default (OpenSSL natively when linked) and
  the documentation explains it.
- **Each exception twin of a test may build OpenSSL again,** since the provider fingerprints
  `<exception-handling>`; Task 12 measures it once.
- **Recorded limits:** MSVC's message text is not checked (its cases only fail), and crypto has
  no size table until Task 22.
- **Not carried:** the controller's ledger and the independent reviews of the specification
  (findings I1 to I8, M1 to M14, and the second review's C1, I1 to I5, M1 to M12), private notes
  of the original machine; the spec cites their rulings and the plan records each in its
  "Conflicts with the spec" table and "Review Focus".

## Decisions pending

- After version 0.1: O2 (nightly level), O3 (NIST's archives), O5 (C17; C16 falls away with the
  foundation's D32).
- The roadmap's owner questions of 2026-11-17 that touch this change: Q2 and Q3 (D9's default
  and version 0.1's two defines), Q6 (Task 33 kept in crypto's tests), Q7 (this change's status).
- Whether `webcpp::webrtc` and `webcpp::beast` adopt the library, and whether a change of
  trystero's own adopts it.
