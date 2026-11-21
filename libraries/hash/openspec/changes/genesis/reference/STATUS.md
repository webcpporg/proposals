# webcpp::hash: where the implementation stands, and how to start

Written 2026-11-14, when this proposal was made, brought up to date on 2026-11-15, when increment 1
was built, on 2026-11-16, when the owner fixed the default backends (design D33), and on 2026-11-18,
when increments 2, 3, 4, 5 and 7 were built under his decisions of 2026-11-17 (design D34 to D36;
what the build decided is design D37 to D42), merged, reviewed whole and fixed, and on 2026-11-21,
when increment 6 (Botan) was built on the stream `botan-6` (design D43 to D45). The owner decided
the library's shape on 2026-11-14, widened it the same day (every backend's whole hashing and KDF
surface, in increments; one profile for every default backend, retired on 2026-11-16; documentation
in noble's and libsodium's manner) and answered O1 (the browser backend through a parallel
asynchronous family), and accepted it. On 2026-11-16 he decided that every key derivation defaults
to `backends::hash2` on every target and link set, and `argon2id` and `scrypt` to
`backends::sodium`, which retires the profiles, the overrides, the measured ranking and the lane
`measure` (design D33). On 2026-11-17 he answered O3: a tag of the families defaults to the first of
`backends::hash2`, `backends::xkcp`, `backends::blake3` and `backends::sodium` that offers it, never
BearSSL, OpenSSL, Botan or WebCrypto (design D34); the asynchronous family's default is the
synchronous one (D35); and measured tables come from release builds only (D36). `spec.md` is the
specification, `plan.md` the implementation plan of 48 tasks (0 to 47) in seven increments,
`../tasks.md` the checklist. **The reference code is the code written in the plan's tasks** (listed
in `../proposal.md`, "Implementation"). There is no `source/` directory: the code is in the
library's own repository, which the owner publishes at gate G1.

## Where it stands

- **Increment 1 is built** (the plan's Tasks 0 to 24, `../tasks.md` group 1): the library's own
  repository, branch `main` (local), and the superproject's integration branch `hash/v0.1`
  (local), rebased onto webcpp::crypto's final version 0.1. Every task was reviewed, every
  Important finding fixed and re-reviewed. Nothing is pushed: `webcpporg/hash` has not been
  created (gate G1), the submodule not added (G2).
- **The default backends are fixed (design D33, 2026-11-16), and built** (`../tasks.md` 1.26) on
  the branches `hash/defaults-hash2` of the library and of the superproject.
- **Increments 2, 3, 4, 5 and 7 are built** (the plan's Tasks 25 to 40 and 44 to 47,
  `../tasks.md` groups 2 to 5 and 7), each task reviewed by someone who did not write it, every
  Important finding fixed and re-reviewed, each increment whole reviewed. They were built in
  parallel streams by backend (`sodium-2`, `xkcp-3`, `openssl-4`, `bearssl-5`, `async-7`, and
  `alloc` for design D36) from the families' task (Task 25), and merged into the library's branch
  `increments-2-7` and the superproject's `hash/increments-2-7` (local), all five merged. Three
  extra tasks ran on the integration branches: "Allocation from release builds" (`../tasks.md`
  1.27, design D36), "BLAKE2b MAC on libsodium" (2.6, ruling R95: `macs::blake2b_mac`'s default
  has its engine, so every default has one) and "Agreement across increments" (7.4, rulings R6
  and R94).
- **The whole branch was reviewed** by three seats (the core families and the documentation; the
  C backends and `third_party`; the asynchronous family, the tools and this proposal), and one
  fix wave answered every finding: the error reference, the overview and the README, the keyed
  BLAKE2 MACs listed as MACs, XKCP's storage sizes in one header read by C and C++, libsodium's
  refusal naming the backend alone, HMAC's refusal over a digest no backend will have, WebCrypto's
  partial cells, and `tools/hash/doc_tables.py`'s checks and llms.txt's defaults derived from the
  table.
- **Green locally, from scratch**, on the integration branches after the fix wave: native 229
  programs, wasip2 222, wasip3 223, Emscripten 161, gcc-15 184 (each test with its
  `_no_exceptions` twin), the own lane `c-suites` (105 programs on each of the four targets), the
  tools' tests, the documentation's build and the full lint (`lint: clean`). MSVC and Apple
  clang run only in the CI, after the gates (their cases are written, not run); the controller
  runs MSVC's check of XKCP's `config.h` locally.
- **Increment 6 is built** (the plan's Tasks 40b to 43 and 47, `../tasks.md` group 6) on the
  branches `hash/botan-6` of the library, the superproject and this repository, each task reviewed
  by someone who did not write it. The stream provisioned Botan itself, as webcpp::crypto's
  specification says (design D43, which supersedes D42's waiting): the submodule
  `third_party/libs/botan` at 3.14.1 (`01af88c`), configured by upstream's `configure.py` per
  target and compiled by b2 through `tools/webcpp-botan.jam` as `/third_party//botan`, one module
  list for hash and webcpp::crypto, natively and on Emscripten, with exceptions only, no RNG
  module. Its column was read at 3.14.1 (D44); `backends::botan` has its digests (Streebog,
  Whirlpool and Skein-512 among them, which no other backend has), its MACs, HKDF, PBKDF2,
  Argon2, scrypt, the key derivations of standards, and `bcrypt.hpp`'s `bcrypt`, `bcrypt_verify`
  and `bcrypt_pbkdf` with their heavy forms; no default changed (D34). Every call into Botan runs
  inside one boundary, so that no exception leaves hash, and a program without exceptions has no
  `backends::botan` (D45). The guide has a section per Botan algorithm with an example, its
  allocations from release builds (to be re-measured in the final pass), and `NOTICE.md` keeps
  Botan's licence for the documentation it adapts.
- **The spec and the design are amended** to what increments 2 to 5 and 7 built (design D37 to
  D41): Boost.Hash2's SHAKE is a whole extendable output and SHAKE's default; `digest_of`, the
  families' guards in one place, BLAKE2b's output size as an option, SP 800-185's L at `finish`,
  the floors of HMAC (10 bytes) and KMAC (4 bytes), error value 9 and the reworded messages of
  values 1, 2 and 5; `backends::webcrypto` declared on every target, asynchronous only, its
  executor checked and its limits stated; the heavy functions natively in every build unless
  `WEBCPP_HASH_NO_THREADS`, with their new refusal text, their storage the handler's; no HMAC over
  MD5-SHA1, and `sskdf` and `x963kdf` over their standards' hashes alone; `doc/capabilities.json`
  at layout version 5.
- **The measurements are a dated reference.** `doc/measurements.*` stay, labelled as measured on
  2026-11-16, and decide nothing; the allocation table is regenerated from release builds (design
  D36), its counts unchanged.
- **The pins** are those read on 2026-11-14: libsodium 1.0.22
  (`77e1ce5d6dee871c49ef211222ba18ef0c486bda`, tag `1.0.22-RELEASE`), XKCP `master`
  (`d20c8fcaecc543f816a0721d0dfc13b4cf27c14d`, 2026-11-10; XKCP has no release tags), BLAKE3
  1.8.7 (`f3149ec5bb5449af877ba20377a11008ff499fa2`), BLAKE2's test vectors
  (`ed1974ea83433eba7b2d95c5dcd9ac33cb847913`, increment 2, added as a submodule), libsodium-doc
  (`6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`, ISC, for the documentation), Botan's latest tag
  3.14.1 (`01af88c`, for increment 6's reading). BearSSL and OpenSSL are the superproject's
  (`7bea48e`, `45e844f`).
- Every step of `plan.md` keeps its box as written; `../tasks.md` records what is done.

## What it waits on

- **The owner's gates** G1, G2 and G4 (`plan.md`, "Owner gates"): creating `webcpporg/hash`,
  adding `libs/hash`, merging the pull requests; MSVC and Apple clang then run in the CI. G5 opens
  the Boost.Hash2 contribution, whose text is ready (`doc/upstream.adoc`). No push, no pull
  request and no CI run by an agent; all evidence is local until the owner asks.
- **webcpp::crypto's version 0.1 on `main`**, which increment 1 is built on
  (`/third_party//bearssl`, `webcpp.same-program`).
- **The owner's merge of the branches `hash/defaults-hash2`** (task 1.26, design D33), then of
  `increments-2-7` and `hash/increments-2-7`.
- **The lanes and review of increment 6** (plan Task 47, Step 6.3: every lane from scratch, MSVC
  14.3 and 14.5 on the owner's lane, both lints, a review of the whole increment), then its pull
  requests (Step 6.6), the library's, the superproject's and this proposal's.
- **webcpp::crypto's increment 7** reuses `/third_party//botan` (design D43): it adds its canary
  beside hash's, and decides Botan's wasip2 and wasip3 cells, which then become hash's too.
- webcpp::asio's `browser::async_await`, which increment 7 needed, is on `main`.

## Flagged to the owner

- The BLAKE2 test-vector submodule (`third_party/libs/blake2` at `ed1974e`, CC0-1.0) is added to
  the superproject as the accepted proposal names it (ruling R20).
- webcpp::asio's `browser::async_await` makes one allocation of the C allocator per awaited
  promise: webcpp::asio's, not hash's (spec 6.1).

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1): Boost 1.92, b2 5.5.3, a C++20
compiler, wasi-sdk 34, wasmtime, Emscripten 6.0.11 with node 26.7.0, Python 3.9 or newer,
`.local/user-config.jam`.

1. **Increments 1 to 7** are built; what remains of increments 1 to 5 and 7 is the owner's: the
   gates G1, G2 and G4, then the CI's MSVC and Apple clang lanes.
2. **Increment 6** resumes at plan Task 47's Step 6.3 on the branches `hash/botan-6`: run every
   lane from scratch (its "Global Constraints" list, `c-suites` with Botan's own suite, MSVC on the
   owner's lane through `run-lane.sh 14.3` and `14.5`), the review of the whole increment, then
   Step 6.6's pull requests. Read designs D43 to D45 first.

Every b2 command runs as `$B2` (`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
--user-config=.local/user-config.jam`); only a build from scratch (`-a`) counts as evidence. The
lanes and their commands are in `plan.md`'s "Global Constraints".

## What to verify first

1. The integration branch builds and passes from scratch on the native lane:
   `$B2 -a libs/hash/test libs/hash/example`, every test and its `_no_exceptions` twin.
2. `default_backend_test`, `default_backend_former_defines` and `capabilities_test` show the
   defaults of spec 4.1 on the four targets with every library linked (`backends::hash2` for every
   key derivation, `backends::sodium` for `argon2id`, `scrypt` and `blake2b_subkey`, each family
   tag's default by design D34), `default_async_backend` equal to `default_backend`, and no
   `WEBCPP_HASH_PROFILE_*`, `WEBCPP_PROFILE_*` or `WEBCPP_HASH_DEFAULT_BACKEND_*` read by
   `include/` (design D33).
3. The rulebook and the lint: `lint: clean`, with `libs/hash/meta/lint.json`'s
   "nodiscard results" rule (design D30).

## Decisions pending

None. O3 is answered (design D34, 2026-11-17); O1 is decided (design D23); O2 is closed by design
D33. The owner's gates are actions, not decisions.
