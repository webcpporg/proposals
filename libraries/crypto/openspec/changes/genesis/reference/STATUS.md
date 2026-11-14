# webcpp::crypto: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal was made from the owner-approved specification and the
plan; revised 2026-11-14, when the owner set version 0.1's scope and the plan was patched to the
superproject's `main` of that day. `spec.md` is the specification (approved by the owner on
2026-10-11); `plan.md` is the implementation plan, whose section "The revision of 2026-11-14"
rules over the rest of it; `../tasks.md` is the checklist; `../design.md` D22 records the
revision. **The reference code is the code written in the plan's tasks.** There is no `source/`
directory: nothing is implemented.

## Where it stands

- **The library's repository has no commit,** and `libs/crypto` does not exist anywhere.
  `webcpporg/crypto` has not been created (gate G1) and the submodule has not been added (G2).
- **No task is done.**
- **Version 0.1 is increment 1 lean and increment 2** (Tasks 0 to 14, 17 to 19, 21, 24 to 30a).
  Deferred: OpenSSL's memory option, constant time, sizes and throughput, the browser lane, the
  nightly level, NIST CAVP, and increments 3 to 5.
- **The base exists:** the superproject's `main` (`f085d79` on 2026-11-14) holds `third_party/`,
  asio with `webcpp::test::exit_status`, the source-built OpenSSL 3.5.9 and trystero's shared
  connection. The gates the plan once waited on (the rewrite of `main`'s history, Task 0a,
  asio's Task 2) are past.
- **The upstream pins were re-read on 2026-11-14 and hold:** BearSSL's mirror at `7bea48e`,
  libsecp256k1 v0.8.0 (still the latest release), ctaes in Bitcoin Core v31.1 (unchanged in
  v31.2 and v32.1), Wycheproof `12fd3aaf`, BIP340's vectors at `927b6de9`.
- **Specification status:** accepted by the owner on 2026-10-11. The source-built OpenSSL
  (provider amendment of 2026-11-03) and the revision's decisions amend it; `design.md` D22
  lists them.

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

1. **Gate G1:** the owner creates `webcpporg/crypto`.
2. **Task 0:** a worktree of the superproject from `origin/main`, the ledger, the baseline.
3. **Tasks 1 to 14, 17 to 19, 21, 24, 25** (increment 1), each: the failing test first and seen
   failing, the implementation, the lanes, the lint, the commit (the contributor's own identity,
   no trailer, staged by path).
4. **Tasks 26 to 30, then 30a** (increment 2, then trystero's build).
5. **Pull requests** in the plan's order ("The base and the order of the work", point 4): the
   library's, one superproject pull request with the submodule `libs/crypto` (G2), trystero's,
   then this repository's, which records version 0.1.

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

1. `tools/node-test/include/webcpp/test/exit_status.hpp`, `third_party/Jamfile` and
   `tools/webcpp-openssl.jam` exist on the base.
2. After Task 1: `python3 tools/test/jamroot_test.py` passes with its case for both third-party
   ids, and `libs/crypto/test//config` builds with and without exceptions.
3. After Task 7: the 18 cases of the GCM specification pass on BearSSL on native and wasip2.
4. After Task 11: the fixture that names `/webcpp/asio//openssl` and `/webcpp/crypto//openssl`
   links each OpenSSL archive once.

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

- G1 and G2: the repository `webcpporg/crypto` and its submodule.
- After version 0.1: O2 (nightly level), O3 (NIST's archives), O5 (C16, C17, Botan on the
  development machine).
- Whether `webcpp::webrtc` and `webcpp::beast` adopt the library.
