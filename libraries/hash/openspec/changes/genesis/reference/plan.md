---
title: "webcpp::hash, genesis: Implementation Plan"
tags: [webcpp, hash, hash2, keccak, xkcp, blake3, hkdf, pbkdf2, argon2, scrypt, libsodium, bearssl, openssl, b2, wasm, emscripten, plan]
status: accepted
created: 2026-11-14
---

# webcpp::hash, genesis: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** a new library, webcpp::hash, at `libs/hash`, in seven increments (spec 1.5). Increment 1: Boost.Hash2 1.92's algorithms and HMAC by
alias; `keccak_256` over XKCP's Keccak-p[1600] and `blake3` over BLAKE3's C implementation, both
in Hash2's HashAlgorithm form; HKDF and PBKDF2 over four backends (`backends::hash2`, `sodium`,
`bearssl`, `openssl`) with the owner's default rule and its measured size table; Argon2id and
scrypt from libsodium; on native, wasip2, wasip3 and Emscripten, every test with exceptions on
and off; and the PBKDF2-HMAC-SHA-512 throughput measured. Increments 2 to 6: every further hashing and
KDF function of libsodium, XKCP, OpenSSL, BearSSL and Botan 3, through the families of spec 2.15.
Increment 7: the browser backend through the parallel asynchronous family (spec 11).

**Architecture:**
- **Header-only** (`/webcpp/hash//hash`, an alias). The public functions (`hkdf_extract`,
  `hkdf_expand`, `hkdf`, `pbkdf2`, `argon2id`, `scrypt`) do every guard once; a backend's header
  specialises only the engines of `webcpp::hash::detail` (`hkdf_engine<Backend, Hmac>`,
  `pbkdf2_engine<Backend, Hmac>`, `argon2id_engine<Backend>`, `scrypt_engine<Backend>`), whose
  primary templates and refusing specialisations hold the `static_assert`s of spec 3.3. The
  default backend is `default_kdf_backend<Hmac, Kdf>` (spec 4), from the `HAS` defines and the
  generated `detail/ranking.hpp`, under the profile and the overrides of spec 4.1.
- **The C libraries** are submodules under `third_party/libs/` (`xkcp`, `blake3`, `libsodium`;
  BearSSL is webcpp::crypto's), built file by file by `third_party/Jamfile` into static
  libraries whose usage requirements give `WEBCPP_HASH_HAS_<NAME>=1`; webcpp::hash's
  `src/canary_<name>.c` joins each, and XKCP also gets webcpp's `src/xkcp_adapter.c` and
  `src/xkcp/config.h`. OpenSSL is the superproject's provider, through hash's alias
  `/webcpp/hash//openssl`.
- **The superproject** gains: three submodules and their targets; the test-data target
  `/third_party//blake3_vectors`; `tools/hash/measure.py` and the own lane `measure`;
  `tools/test/hash_jamfiles_test.py` and its fixtures; `tools/test/blake3_c_suite.py`;
  CODING-GUIDELINES.md chapter 10's three rows and chapter 17's entry.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Hash2, System, Core's lightweight_test, JSON in tests), wasi-sdk
  34 with wasmtime 50.0.1 (47.0.3 accepted locally, as webcpp::crypto's ledger rules), Emscripten
  6.0.11 with node 26.7.0, Python 3.9+ (AGENTS.md chapter 1, "Prerequisites").
- libsodium 1.0.22, tag `1.0.22-RELEASE` (and `1.0.22`), commit
  `77e1ce5d6dee871c49ef211222ba18ef0c486bda`, published 2026-04-09, read with
  `gh api repos/jedisct1/libsodium/releases/latest` on 2026-11-14.
- XKCP, `master` at `d20c8fcaecc543f816a0721d0dfc13b4cf27c14d` (2026-11-10, "Add FlightDeck and
  related improvements"), read with `gh api repos/XKCP/XKCP/commits/master` on 2026-11-14. XKCP
  publishes no release tags.
- BLAKE3 1.8.7, tag `1.8.7`, commit `f3149ec5bb5449af877ba20377a11008ff499fa2`, published
  2026-08-20, read with `gh api repos/BLAKE3-team/BLAKE3/releases/latest` on 2026-11-14.
- BearSSL at `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (webcpp::crypto's `/third_party//bearssl`)
  and OpenSSL 3.5.9 at `45e844fa2a14ec92d146bd8f5778ac130b6625fb` (the superproject's provider).

**Spec:** `reference/spec.md` beside this plan, "the spec" below, cited by section, with the
proposal's `design.md` (decisions D1 to D18). What was read upstream for it, on 2026-11-14:
libsodium's tree at `77e1ce5` holds 141 `.c` files under `src/libsodium/`, seven `.S` files,
`builds/msvc/version.h` (`SODIUM_VERSION_STRING "1.0.22"`, library version 26.4), `build.zig`
(its define sets per target, and its copy of `builds/msvc/version.h` into the tree), 101 `.c`
tests under `test/default/` with `cmptest.h`; its private `argon2.h` declares `argon2_ctx` with
`secret`, `ad`, `lanes` and `threads`; `crypto_pwhash_argon2id` passes parallelism 1. XKCP's
`KeccakP-1600-opt64.c` (CC0) includes `brg_endian.h`, `load-store.h`, `KeccakP-1600-SnP.h`
(`plain-64bits/SnP/`), `KeccakP-1600-64.macros`, `KeccakP-1600-unrolling.macros` and
`SnP-Relaned.h`, and `KeccakP-1600-plain64.h` includes a generated `config.h`; its
`tests/UnitTests/main.c` takes `--SnP`. BLAKE3's `c/` holds `blake3.c`, `blake3_dispatch.c`,
`blake3_portable.c`, the four x86 intrinsics files, `blake3_neon.c`, `.S` and `.asm` twins,
`main.c` and `test.py` (which runs `c/blake3`, a binary beside it); `test_vectors/test_vectors.json`.
OpenSSL 3.5.9's `providers/implementations/kdfs/hkdf.c` defines `HKDF_MAXINFO (32 * 1024)`, and
`pbkdf2.c` turns its lower bounds off with `pkcs5 = 1`.

## The order of the work, and the parallel streams

The base is the superproject's branch `crypto/v0.1` (webcpp::crypto's version 0.1, at `278fb3c`
when this plan was checked), not `origin/main`: only it holds what the plan builds on,
`third_party/Jamfile`'s `.c-library`, `.gc` and `CRYPTO`, `/third_party//bearssl` and its
submodule, `tools/same_program/same_program.jam` and `webcpp.same-program`,
`tools/test/crypto_jamfiles_test.py`, `libs/crypto` (pre-flight, 2026-11-15). When `crypto/v0.1`
has merged into `main`, the work rebases onto `main`. No task waits for BearSSL.

The tasks form streams that run in parallel, each in a worktree of its own of the superproject
(branch `hash/<stream>` from the integration branch `hash/genesis`) with a worktree of the
library at `libs/hash` (branch `<stream>` from the library's `main`), as webcpp::crypto's
ledger did. Streams touch the same list files (`test/Jamfile`'s headers-alone list,
`meta/include-boundaries.json`, `doc/changelog.adoc`, `third_party/Jamfile`, `.gitmodules`,
CODING-GUIDELINES.md's rows, the release notes); the controller merges each reviewed stream into
the integration branches and resolves those files.

| Wave | Streams, in parallel | Tasks | Starts after |
| --- | --- | --- | --- |
| 0 | foundation | 0, 1 | - |
| 1 | `errors` | 2 | 1 |
| 1 | `hash2` | 3 | 1 |
| 1 | `keccak` | 4, then 5 | 1 |
| 1 | `blake3` | 6, then 7 | 1 |
| 1 | `sodium` | 8 | 1 |
| 2 | `kdf` | 9, then 10 | 2 and 3 |
| 3 | `kdf-sodium` | 11 | 8 and 9 |
| 3 | `kdf-openssl` | 12 | 10 |
| 3 | `kdf-bearssl` | 13 | 9 |
| 4 | `pwhash` | 15, then 16 | 8, 9 and 11 (`backends/sodium.hpp` is Task 11's) |
| 4 | `default` | 14 | 11, 12 and 13 |
| 4 | `agreement` | 17 | 11, 12 and 13 |
| 4 | `webcrypto` | 18 | 11 and 13 |
| 5 | `build` | 19 | 5, 7, 11, 12, 13, 14 and 15 |
| 5 | `allocation` | 20 | 5, 7, 11, 12, 13 and 16 |
| 4 | `throughput` | 22 | 8, 10 and 12 |
| 5 | `measure` | 21 | 5, 7, 14, 15 and 16 |
| 6 | `docs` | 23 | every task but 24 |
| 6 | `whole` | 24 | 23 |
| 7 | `sodium-2` | 25 to 29 (increment 2) | 24 |
| 7 | `xkcp-3` | 30 to 33 (increment 3) | 24, and Task 25 |
| 7 | `openssl-4` | 34 to 38 (increment 4) | 24, and Task 25 |
| 7 | `bearssl-5` | 39, 40 (increment 5) | 24, and Task 25 |
| 7 | `async-7` | 44 to 46 (increment 7; the heavy variants on threads in Task 44) | 24, and webcpp::asio's `browser::async_await` on `main` |
| 8 | `botan-6` | 41 to 43 (increment 6) | Task 25, and webcpp::crypto's Botan provisioning on `main` |
| 9 | `whole-n` | 47 | each increment's stream: run once per increment as it merges |

Increments 2 to 7 are independent of one another: each brings one backend's functions into the
families that Task 25 creates first (in the stream `sodium-2`); once Task 25 has merged, their
streams run in parallel, and each increment is merged and recorded (Task 47) on its own.

Inside a stream the tasks run in order. A stream that needs a definition another stream of the
same wave makes (a test helper, a list entry) carries a temporary copy, dropped at integration in
favour of the real one.

## Owner gates

- **G1:** the owner creates `webcpporg/hash`, public, BSL-1.0.
- **G2:** the owner adds the submodule (`git submodule add ../hash.git libs/hash`).
- **G3:** the three third-party submodules are the owner's decision (CODING-GUIDELINES.md chapter
  10, "Third-party code"), taken with this proposal's acceptance on 2026-11-14: libsodium
  (`https://github.com/jedisct1/libsodium.git`), XKCP (`https://github.com/XKCP/XKCP.git`) and
  BLAKE3 (`https://github.com/BLAKE3-team/BLAKE3.git`), at the pins above.
- **G4:** the owner merges every pull request; an agent never pushes, never opens a pull request,
  never runs a CI workflow, never creates a repository or changes a setting. Every piece of
  evidence is local until the owner asks for the CI (MSVC and Apple clang run there, at the end).
- **G5:** opening the Boost.Hash2 contribution (spec 9.1), prepared in Task 23.

## Global Constraints

**Repositories and paths:**
- The superproject is `webcpporg/webcpp`, checked out with its submodules; the work happens in
  a worktree of it on the branch `hash/genesis` from `crypto/v0.1` (Task 0), its `.local` a link
  to the main checkout's. Paths below are relative to that worktree.
- `libs/hash` is the library's own git repository, created in Task 1, branch `main`, whose
  remote is `webcpporg/hash` once the owner creates it (G1). Until the submodule is added (G2),
  the superproject ignores it through the common git directory's `info/exclude` (`/libs/hash/`),
  never through a tracked line.
- Every b2 command runs from the worktree's root as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`,
  written `$B2` below. A verdict is `-a`. Each lane has its own `--build-dir`.
- The lanes, as the CI runs them (`tools/ci/matrix.py`):
  - native: `$B2 -a --build-dir=bin/lane-native libs/hash/test libs/hash/example`;
  - wasip2: `$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/hash/test libs/hash/example`;
  - wasip3: the same with `clang-wasip3`, `tools/boost/asio-wasip3/run.sh` and `bin/lane-wasip3`;
  - emscripten: `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/hash/test libs/hash/example`;
  - gcc: `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/hash/test libs/hash/example`;
  - MSVC and Apple clang in the CI only, at the end (G4);
  - the own lane `c-suites` on target `<t>`: the target's command with
    `libs/hash/test/c_suites//c-suites` in place of the two directories; the own lane `measure`:
    `$B2 -a --build-dir=bin/lane-measure libs/hash/test/measure//measure` (it builds for the four
    targets itself, Task 21);
  - the user's Jamfiles: `python3 tools/test/hash_jamfiles_test.py`.
- The lint: `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`,
  ending `lint: clean`.

**The library's rules (CODING-GUIDELINES.md chapters 2 to 9; the spec):**
- Names: spec 2.1. `<webcpp/hash/config.hpp>` first in every public header; it includes
  `<webcpp/config.hpp>` first and reads the target from `WEBCPP_TARGET_*`; no header under
  `include/` reads `__wasi__` or `__EMSCRIPTEN__`.
- Every operation that can fail returns `boost::system::result<T>`; the values of `error` are
  spec 2.3's, fixed, never reused.
- Inputs `std::span<std::uint8_t const>`, outputs `std::span<std::uint8_t>`, sizes
  `std::size_t`; a limit wider than `size_t` on wasm32 is `std::uint64_t`, compared through
  `detail::exceeds`.
- No allocation in webcpp's code; no global or static mutable state in a header. A test's
  counters are global, each line with
  `// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)` and the reason.
- No C array in C++ code (`std::array`). A function starts with its guards. A comment says what
  the code does not. Doc Comments on every public symbol (chapter 7).
- Every header under `include/webcpp/hash/` is in the test Jamfile's headers-alone list and in
  `meta/include-boundaries.json`, added by the task that creates it.
- Tests use `<boost/core/lightweight_test.hpp>` and `test/require.hpp` (xactor's), end
  `return webcpp::test::exit_status(boost::report_errors());` with
  `#include <webcpp/test/exit_status.hpp>`, and are declared through `hash-run` (Task 1), on every
  target their libraries have.
- A message's test is a `webcpp.compile-diagnostic` in a directory of its own under
  `test/messages/`, whose source states the whole text:
  `// expected-error@webcpp/hash/<header>:* {{<the text>}}`; on Emscripten the message is a
  fixture of `tools/test/hash_jamfiles_test.py`; on MSVC a `webcpp.compile-fail` or
  `webcpp.link-fail` under `test/msvc/<case>/`.

**Commits and text:**
- Every commit is made with the contributor's own git identity, with no trailer of any kind and
  never an agent co-author: `git commit -m "<message>" -- <paths>` (in the library
  `git -C libs/hash commit ...`). After each,
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage by path: a new file is added first, `git add -- <new paths>`. Never `git add -A`,
  `git add .` or `git add libs`. A submodule's commit is recorded, never its files.
- Subjects: `hash: <what>` in the library; `third_party: <what>`, `tools: <what>`,
  `guidelines: <what>` in the superproject.
- A change a user can see adds its changelog line in the same commit: the library's
  `doc/changelog.adoc` (from Task 1); the superproject's release notes of the version in
  preparation, "Tools and build", for a tool or a b2 target (AGENTS.md chapter 5).
- English everywhere; no em dash (U+2014); the word the lint's banned-word rule matches appears
  nowhere; no home directory path in a tracked file.
- Every source file opens with the licence notice (`//` for C++ and AsciiDoc, `/* */` for C,
  `#` for Jam, Python and YAML).
- No environment variable is ever printed.

**Names fixed by this plan:**

| Name | What it is |
| --- | --- |
| `/webcpp/hash//hash` | the library: `include`, Boost's headers, `/webcpp/config//config` |
| `/webcpp/hash//openssl` | hash's alias over `/webcpp/third_party//openssl`, with `WEBCPP_HASH_HAS_OPENSSL=1`; refused on Emscripten (Task 12) |
| `/third_party//xkcp`, `//blake3`, `//sodium` | the C libraries of spec 5.2 |
| `/third_party//blake3_vectors` | test data: `WEBCPP_TEST_BLAKE3_VECTORS`, the directory of `test_vectors.json` |
| `/third_party//sodium-test-config` | libsodium's sources' include directories and defines, for the C suite and the RFC 9106 test (Task 8) |
| `hash-run <name> : <sources> + : <requirements> * : <targets> *` | the test Jamfile's rule: the test twice, with exceptions and without |
| `webcpp::hash::detail::{hkdf,pbkdf2}_engine<Backend, Hmac>`, `{argon2id,scrypt}_engine<Backend>` | the engines a backend specialises (Task 9) |
| `WEBCPP_HASH_<NAME>_CANARY_COMMIT` | the canaries' test seams (`detail/canary.h`) |
| `WEBCPP_HASH_DETAIL_TEST_OPENSSL_DIGEST` | the seam that replaces OpenSSL's digest name, to provoke a backend failure (Task 12) |
| `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD` | the seam through which `detail::wipe` reports each wipe to a test (Task 9) |
| `tools/test/hash_jamfiles_test.py`, `tools/test/fixtures/hash_jamfiles/<case>/` | the user's Jamfiles and the build's stops (Task 19) |
| `tools/hash/measure.py` | the measured table and `detail/ranking.hpp` (Task 21) |
| own lanes `c-suites`, `measure` | `libs/hash/test/c_suites`, `libs/hash/test/measure` |
| `.superpowers/sdd/WEBCPP_HASH_PLAN/progress.md` | the ledger, in the worktree, untracked |

## Review Focus

1. **Hash2's contract for `result()` and `update` after it.** A reasonable person expects
   `keccak_256` and `blake3` to behave as Hash2's own types in every sequence of calls: repeated
   `result()` gives new values, `update` after `result` is allowed and deterministic, the seeded
   constructors absorb and finalize as Hash2's do. Task 5 compares `keccak_256` with Hash2's
   `detail::keccak_base<0x01, 512, 256>` on random call sequences; Task 7 checks `blake3`'s
   repeated results against `squeeze` and its seeds against BLAKE3's keyed mode.
2. **One contract on every backend.** Empty salt, empty IKM, empty info, empty password, a PRK of
   one byte more or less, info of 32,768 and 32,769 bytes, the longest output and one byte more:
   every backend must answer the same. Tasks 9 to 13 test each on each backend; Task 17 on
   random inputs.
3. **A refusal that names the right backends.** Each `static_assert` of spec 3.3 fires on the
   call it is about, and only there: including a backend's header or naming
   `backends::bearssl` as a type is not an error; calling `hkdf_expand` on it is. Tasks 9, 13 and
   14 test it.
4. **The default backend on WebAssembly is per KDF.** `default_kdf_backend<hmac_sha2_256,
   kdfs::pbkdf2>` must never be `backends::sodium` or `backends::bearssl`, whatever the measured
   order or the profile says. Task 14 tests it on each target with every library linked.
5. **Secrets left behind.** HKDF's T blocks, PBKDF2's U values and accumulator, the keyed hmac
   copies and BearSSL's context are wiped on success and on failure; a backend's failure zeroes
   the output; a guard's refusal leaves it untouched. Tasks 9, 10 and 13 test it.
6. **libsodium initialised.** `sodium_init()` before every engine call, its return 1 not taken
   as an error, and on Emscripten its random generator's stirring under node. Tasks 8 and 11.
7. **One copy per program.** A program naming `/webcpp/hash//openssl` and `/webcpp/crypto//openssl`
   links each OpenSSL archive once; `/third_party//bearssl` with both canaries is one archive.
   Tasks 12 and 13.
8. **A lane or a check that passes having built nothing.** Every own lane names its targets; a
   measure program that a target skips is a failure of the `measure` lane, not a missing row. Tasks 4,
   6, 8 and 21.
9. **Generated files.** `detail/ranking.hpp` and `doc/measurements.*` are written only by
   `tools/hash/measure.py`; the `measure` lane fails on a hand edit. Task 21.
10. **The asynchronous family completes, never inline.** A completion handler must not run
    inside the initiating call on any target, and an operation keeps the caller's spans until it
    completes or is cancelled (then `output` is zeroed). Task 46 tests it.
11. **The capability table is one fact in two places.** `doc/capabilities.json` and
    `backend_has_v` must agree cell by cell; a refusal text names exactly the backends of its
    row. Tasks 14 and 25 test it.

---

## Task 0: The worktree, the ledger and the baseline

**Files:**
- Create: `.superpowers/sdd/WEBCPP_HASH_PLAN/progress.md` in the worktree (untracked).

- [ ] **Step 1: Make the worktree from `crypto/v0.1`.**

```bash
git -C <superproject> fetch origin
git -C <superproject> worktree add <worktree> -b hash/genesis crypto/v0.1
cd <worktree>
ls tools/node-test/include/webcpp/test/exit_status.hpp third_party/Jamfile tools/webcpp-openssl.jam tools/same_program/same_program.jam
git submodule update --init
git submodule status
ln -s <superproject>/.local .local
```

  Expected: the four files listed; one line per submodule without a leading `-` or `+`,
  `third_party/libs/bearssl` at `7bea48e` among them; `ls -l .local` shows the link.
- [ ] **Step 2: Create the ledger** with the first line
  `# SDD ledger - plan: libraries/hash/openspec/changes/genesis/reference/plan.md (proposals)`.
- [ ] **Step 3: Record the baseline** in the ledger, each with its output: `git rev-parse HEAD`;
  `$B2 -d0 declared-targets`; the toolchain versions (`.local/wasi-sdk/bin/clang --version | head -1`,
  `wasmtime --version`, `.local/emsdk/upstream/emscripten/em++ --version | head -1`,
  `.local/emscripten/node --version`); the provider
  (`$B2 --build-dir=bin/lane-native libs/asio/test//tls_client_context`, which builds OpenSSL and
  passes); the lint (`lint: clean`). A failure found here is fixed first, in its own change, as
  CODING-GUIDELINES.md chapter 1 asks.

## Task 1: The library's skeleton, the test Jamfile's rule and chapter 17's entry

**Files:**
- Create (in `libs/hash`, a new git repository): `LICENSE_1_0.txt` (the superproject's),
  `README.md`, `AGENTS.md`, `.gitattributes`, `.gitignore` (`doc/html/`),
  `.github/workflows/ci.yml`, `meta/libraries.json`, `meta/include-boundaries.json`, `build.jam`,
  `include/webcpp/hash.hpp`, `include/webcpp/hash/config.hpp`, `test/Jamfile`,
  `test/config_test.cpp`, `test/require.hpp`, `test/.clang-tidy`, `example/Jamfile`,
  `example/.clang-tidy`, `doc/Jamfile`, `doc/hash.adoc`, `doc/overview.adoc`,
  `doc/changelog.adoc`, `doc/mrdocs.yml`.
- Modify (superproject): `CODING-GUIDELINES.md` (chapter 17: the entry "hash (hashing and key
  derivation over Boost.Hash2 and C backends; wrapper; accepted 2026-11-14, not built)", a
  summary of the spec with a link to this proposal), the release notes (New Libraries: a line),
  the common git directory's `info/exclude` (`/libs/hash/`).

**Interfaces:**
- Produces: the project `/webcpp/hash` and its alias `hash`; `WEBCPP_HASH_NO_EXCEPTIONS`,
  `WEBCPP_HASH_WEBASSEMBLY`; the tags `backends::{hash2, sodium, bearssl, openssl}` and
  `kdfs::{hkdf, pbkdf2}` (spec 2.2); the rule `hash-run`.

- [ ] **Step 1: Create the repository** and its files, from webcpp::crypto's as the model
  (`libs/crypto`), every name changed to hash's:

```bash
mkdir -p libs/hash && git -C libs/hash init -b main
cp LICENSE_1_0.txt libs/hash/
echo '/libs/hash/' >> "$(git rev-parse --git-common-dir)/info/exclude"
```

  `meta/libraries.json`: `"key": "hash"`, `"name": "hash"`, `"description": "Hashing and key
  derivation: Boost.Hash2's algorithms, Keccak-256, BLAKE3, HKDF, PBKDF2, Argon2id and scrypt
  over C backends."`, `"category": ["Cryptography"]`, `"cxxstd": "20"`, `"port-of": null`, the
  authors and maintainers as webcpp::crypto's file has them. `ci.yml` calls
  `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: hash`.
- [ ] **Step 2: Write the failing configuration test** `test/config_test.cpp`, webcpp::crypto's
  `config_test.cpp` with `HASH` for `CRYPTO`, and the checks of spec 2.2's tags:

```cpp
#include <webcpp/hash/config.hpp>

#ifndef WEBCPP_CONFIG_HPP
#error "config.hpp does not include <webcpp/config.hpp>"
#endif

#ifdef WEBCPP_TEST_HASH_ALL_OFF
#ifndef WEBCPP_HASH_NO_EXCEPTIONS
#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_HASH_NO_EXCEPTIONS"
#endif
#elif defined(WEBCPP_TEST_HASH_DEVELOPER_DEFINED)
#ifndef WEBCPP_HASH_NO_EXCEPTIONS
#error "config.hpp drops WEBCPP_HASH_NO_EXCEPTIONS, which the developer defined"
#endif
#elif defined(WEBCPP_HAS_EXCEPTIONS) == defined(WEBCPP_HASH_NO_EXCEPTIONS)
#error "WEBCPP_HASH_NO_EXCEPTIONS is not defined exactly when webcpp has no exceptions"
#endif

#if defined(WEBCPP_TARGET_NATIVE) == defined(WEBCPP_HASH_WEBASSEMBLY)
#error "WEBCPP_HASH_WEBASSEMBLY is not defined exactly on WebAssembly"
#endif

// The tags are declared, incomplete until their headers define them.
template <class T>
constexpr bool declared = sizeof(T*) != 0;
static_assert(declared<webcpp::hash::backends::hash2> && declared<webcpp::hash::backends::sodium> &&
              declared<webcpp::hash::backends::bearssl> && declared<webcpp::hash::backends::openssl> &&
              declared<webcpp::hash::kdfs::hkdf> && declared<webcpp::hash::kdfs::pbkdf2>);

int main() {
    return 0;
}
```

  and `test/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/hash//hash ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# Every test runs with exceptions on and off: the second program has the suffix
# _no_exceptions and is built with exception-handling=off, which links
# tools/throw_exception.cpp.
rule hash-run ( name : sources + : requirements * : targets * )
{
    webcpp.run $(name) : $(sources) : $(requirements) : $(targets) ;
    webcpp.run $(name)_no_exceptions : $(sources)
      : $(requirements) <exception-handling>off : $(targets) ;
}

webcpp.headers-alone hash : ../include : hash.hpp hash/config.hpp ;

webcpp.compile config : config_test.cpp ;
webcpp.compile config_no_exceptions : config_test.cpp : <exception-handling>off ;
webcpp.compile config_all_off : config_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_HASH_ALL_OFF ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_HASH_NO_EXCEPTIONS <define>WEBCPP_TEST_HASH_DEVELOPER_DEFINED ;
```

  Run `$B2 libs/hash/test//config`. Expected: FAIL, "no project /webcpp/hash" (no `build.jam`).
- [ ] **Step 3: Write `build.jam`** (spec 5.1) and `include/webcpp/hash/config.hpp` (spec 2.2,
  whole, with a Doc Comment on every macro and tag), and `include/webcpp/hash.hpp` including
  `config.hpp` alone for now:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# hash: hashing and key derivation over Boost.Hash2 and C backends. A program
# names /webcpp/hash//hash and the C library of each part it uses,
# /third_party//<name> (third_party/Jamfile), or, for OpenSSL,
# /webcpp/hash//openssl:
#
#     exe app : app.cpp /webcpp/hash//hash /third_party//sodium ;

import webcpp-target ;

project /webcpp/hash ;

# tag::hash[]
alias hash : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;
# end::hash[]
```

  Run `$B2 libs/hash/test//config libs/hash/test//config_no_exceptions libs/hash/test//config_all_off libs/hash/test//config_developer_defined`,
  then the same on wasip2, wasip3 and Emscripten (each lane's toolset and build directory).
  Expected: PASS on the four targets.
- [ ] **Step 4: The documentation's skeleton and chapter 17.** `doc/hash.adoc` with the
  overview (spec 1.1's table), `doc/changelog.adoc` with "* Added the library." under the version
  in preparation; CODING-GUIDELINES.md chapter 17 gains hash's entry; the release notes gain the
  New Libraries line. Run the lint. Expected: `lint: clean`.
- [ ] **Step 5: Commit.** Library: `git -C libs/hash add -- <every file of Step 1 to 4>` and
  `git -C libs/hash commit -m "hash: the library's skeleton, its configuration and the test rule hash-run" -- <paths>`.
  Superproject: `git commit -m "guidelines: hash is accepted, in chapter 17" -- CODING-GUIDELINES.md <release notes>`.

## Task 2: The errors

**Files:**
- Create (library): `include/webcpp/hash/error.hpp`, `test/error_test.cpp`.
- Modify (library): `test/Jamfile` (headers-alone; `hash-run error`), `meta/include-boundaries.json`.

**Interfaces:**
- Produces: `webcpp::hash::error` (values 1 to 8; 9 comes with Task 25), `category()`, `make_error_code(error)`,
  `boost::system::is_error_code_enum<webcpp::hash::error>` (spec 2.3).

- [ ] **Step 1: Write the failing test** `test/error_test.cpp`:

```cpp
#include <webcpp/hash/error.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/result.hpp>

#include <webcpp/test/exit_status.hpp>

#include <array>
#include <cstring>
#include <string_view>

namespace hash = webcpp::hash;

namespace {

struct expected_message {
    hash::error value;
    int number;
    std::string_view text;
};

constexpr std::array<expected_message, 8> messages{{
    {hash::error::invalid_output_size, 1, "the output is empty or longer than the function produces"},
    {hash::error::invalid_key_size, 2, "the pseudorandom key is not of the hmac's digest size"},
    {hash::error::invalid_salt_size, 3, "the salt is not of a size the function accepts"},
    {hash::error::invalid_iterations, 4, "the iteration count is zero"},
    {hash::error::invalid_parameters, 5, "a cost is outside the function's limits"},
    {hash::error::input_too_long, 6, "an input is longer than the function accepts"},
    {hash::error::backend_failure, 7, "the backend failed where it should not"},
    {hash::error::unsupported_by_backend, 8, "the backend does not take this parameter; name a backend that does"},
}};

void check_values_and_messages() {
    for (const auto& expected : messages) {
        const boost::system::error_code code = expected.value;
        BOOST_TEST_EQ(code.value(), expected.number);
        BOOST_TEST_EQ(std::string_view(code.category().name()), "webcpp.hash");
        BOOST_TEST_EQ(code.message(), std::string(expected.text));
        std::array<char, 8> small{};
        const char* whole = code.category().message(code.value(), small.data(), small.size());
        BOOST_TEST_EQ(std::string_view(whole), expected.text);
        BOOST_TEST_EQ(std::strlen(small.data()), small.size() - 1);
    }
    BOOST_TEST_EQ(hash::category().message(0), std::string("unknown webcpp.hash error"));
}

void check_result() {
    const boost::system::result<int> failed = boost::system::error_code(hash::error::backend_failure);
    BOOST_TEST(!failed);
    BOOST_TEST(failed.error() == hash::error::backend_failure);
}

}  // namespace

int main() {
    check_values_and_messages();
    check_result();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  with `hash-run error : error_test.cpp ;`. Run `$B2 libs/hash/test//error`. Expected: FAIL, no
  `error.hpp`.
- [ ] **Step 2: Write `error.hpp`**, webcpp::crypto's `error.hpp` in form (its `message_of`,
  `category` with the identity `0x6861736800000000ULL`, both `message` overloads, the instance
  `detail::the_category`, `category()`, `make_error_code`), with spec 2.3's values and texts.
  Run the test on the four targets. Expected: PASS, and `error_no_exceptions` too.
- [ ] **Step 3: Commit** (library) `"hash: the errors, in the category webcpp.hash"`.

## Task 3: Boost.Hash2's algorithms and HMAC, by alias

**Files:**
- Create (library): `include/webcpp/hash/algorithms.hpp`, `include/webcpp/hash/hmac.hpp`,
  `test/algorithms_test.cpp`.
- Modify (library): `include/webcpp/hash.hpp`, `test/Jamfile`, `meta/include-boundaries.json`,
  `doc/changelog.adoc`.

**Interfaces:**
- Produces: spec 2.4 and 2.5, every alias with its Doc Comment.

- [ ] **Step 1: Write the failing test** `test/algorithms_test.cpp`: a `static_assert` of
  `std::is_same_v` for each of spec 2.4's 28 algorithm aliases and spec 2.5's 16 hmac aliases and
  `hmac<sha2_256>`, written out one line each; then:

```cpp
void check_hash_append() {
    // Hash2's own test value: SHA-256 of "abc" (FIPS 180-4's example).
    webcpp::hash::sha2_256 hasher;
    hasher.update("abc", 3);
    BOOST_TEST_EQ(boost::hash2::to_string(hasher.result()),
                  "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // hash_append through webcpp's name is Hash2's: the same digest as Hash2's own call.
    webcpp::hash::sha2_256 ours;
    boost::hash2::sha2_256 theirs;
    const std::string text = "webcpp";
    webcpp::hash::hash_append(ours, {}, text);
    boost::hash2::hash_append(theirs, {}, text);
    BOOST_TEST(ours.result() == theirs.result());
}

void check_hmac() {
    // RFC 4231, test case 1.
    const std::array<unsigned char, 20> key = make_filled<20>(0x0b);
    webcpp::hash::hmac_sha2_256 mac(key.data(), key.size());
    mac.update("Hi There", 8);
    BOOST_TEST_EQ(boost::hash2::to_string(mac.result()),
                  "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}
```

  with `hash-run algorithms : algorithms_test.cpp ;`. Run it. Expected: FAIL, no `algorithms.hpp`.
- [ ] **Step 2: Write the two headers** (spec 2.4, 2.5). Run the test on the four targets.
  Expected: PASS. Add both to the headers-alone call and the boundaries file, to `hash.hpp`, and
  the changelog line "Added Boost.Hash2's algorithms and HMAC under webcpp::hash's names."
- [ ] **Step 3: Commit** (library) `"hash: Boost.Hash2's algorithms and HMAC by alias"`.

## Task 4: XKCP, its adapter and canary, file by file

**Files:**
- Create (superproject): the submodule `third_party/libs/xkcp`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile`, `tools/test/third_party_test.py`
  (XKCP's cases), `CODING-GUIDELINES.md` (chapter 10's row `xkcp`), the release notes.
- Create (library): `src/xkcp/config.h`, `src/xkcp_adapter.c`, `src/canary_xkcp.c`,
  `include/webcpp/hash/detail/canary.h`, `include/webcpp/hash/detail/xkcp.hpp`,
  `test/c_suites/Jamfile`.
- Modify (library): `test/Jamfile` (`build-project c_suites ;`).

**Interfaces:**
- Produces: `/third_party//xkcp` (usage requirements `WEBCPP_HASH_HAS_XKCP=1` and the removal of
  unused sections); the adapter's four functions (spec 2.6); the symbol
  `webcpp_hash_xkcp_d20c8fc_generic64__add_third_party_xkcp_to_your_link`; the macros of
  `detail/canary.h`.

- [ ] **Step 1: Add the submodule,** pinned:

```bash
git submodule add https://github.com/XKCP/XKCP.git third_party/libs/xkcp
git -C third_party/libs/xkcp checkout d20c8fcaecc543f816a0721d0dfc13b4cf27c14d
ls third_party/libs/xkcp/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c third_party/libs/xkcp/lib/low/KeccakP-1600/plain-64bits/SnP/KeccakP-1600-SnP.h third_party/libs/xkcp/lib/common/brg_endian.h third_party/libs/xkcp/lib/low/common/SnP-Relaned.h
```

  Expected: the four files listed. (Upstream is on GitHub; no mirror is needed, chapter 5 rule 1.)
- [ ] **Step 2: Write the failing case** in `tools/test/third_party_test.py`, the form of
  BearSSL's case (`harness.copy_integration_libraries(root, 'hash')` first): build
  `/third_party//xkcp` natively with `-a -d+2`; assert that b2 compiled exactly three C files
  (`KeccakP-1600-opt64.c`, `xkcp_adapter.c`, `canary_xkcp.c`), each with
  `-ffunction-sections -fdata-sections` and none with `-std=`; then `nm` of the archive lists
  `webcpp_hash_xkcp_d20c8fc_generic64__add_third_party_xkcp_to_your_link` (with the leading `_`
  on Mach-O) and the four `webcpp_hash_xkcp_*` functions. Run `python3 tools/test/third_party_test.py`.
  Expected: FAIL, no target `xkcp`.
- [ ] **Step 3: Write the library's C files.** `src/xkcp/config.h`, the configuration XKCP's own
  build would generate for one component:

```c
/* Copyright (c) 2026 WebCpp.org
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * https://www.boost.org/LICENSE_1_0.txt)
 *
 * The configuration XKCP's build generates as config.h, for the one component webcpp::hash
 * compiles: Keccak-p[1600] in the plain-64bits implementation (XKCP's "generic64" profile).
 * It is on the include path of /third_party//xkcp alone, never of a program. */
#ifndef WEBCPP_HASH_XKCP_CONFIG_H
#define WEBCPP_HASH_XKCP_CONFIG_H
#define XKCP_has_KeccakP1600
#endif
```

  `include/webcpp/hash/detail/canary.h`: webcpp::crypto's `detail/canary.h` with `HASH` for
  `CRYPTO`, the guard `WEBCPP_HASH_DETAIL_CANARY_H`, and the four commits of spec 5.4 each behind
  its `WEBCPP_HASH_<NAME>_CANARY_COMMIT` seam (this task writes XKCP's; Tasks 6, 8 and 13 add
  theirs). `src/canary_xkcp.c`:

```c
/* (licence notice)
 *
 * The canary of XKCP, compiled into /third_party//xkcp with its defines. Its configuration is
 * the profile of KeccakP-1600-opt64.c those defines make: generic64, every round unrolled and
 * no lane complementing (no KeccakP1600_plain64_unrolling, no
 * KeccakP1600_plain64_useLaneComplementing); unconfigured for any other build. */
#include <webcpp/hash/detail/canary.h>

#if !defined(KeccakP1600_plain64_unrolling) && !defined(KeccakP1600_plain64_useLaneComplementing)
#define WEBCPP_HASH_XKCP_BUILT generic64
#else
#define WEBCPP_HASH_XKCP_BUILT unconfigured
#endif

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_hash_backend_xkcp", \
    WEBCPP_HASH_DETAIL_MISMATCH_VALUE(xkcp, WEBCPP_HASH_XKCP_COMMIT, WEBCPP_HASH_XKCP_BUILT))
#endif

extern const char WEBCPP_HASH_DETAIL_CANARY(xkcp, WEBCPP_HASH_XKCP_COMMIT, WEBCPP_HASH_XKCP_BUILT);
const char WEBCPP_HASH_DETAIL_CANARY(xkcp, WEBCPP_HASH_XKCP_COMMIT, WEBCPP_HASH_XKCP_BUILT) = 0;
```

  `src/xkcp_adapter.c`:

```c
/* (licence notice)
 *
 * webcpp::hash's adapter over XKCP's Keccak-p[1600]: four functions of webcpp's own names, so
 * that no XKCP header, and with them XKCP's config.h, reaches a program's translation unit.
 * The state is the caller's 25 lanes of 64 bits, the layout of KeccakP1600_plain64_state. */
#include <stddef.h>
#include <stdint.h>

#include "KeccakP-1600-SnP.h"

void webcpp_hash_xkcp_initialize(uint64_t *state) {
    KeccakP1600_Initialize((KeccakP1600_plain64_state *)state);
}

void webcpp_hash_xkcp_add_bytes(uint64_t *state, const unsigned char *data, size_t offset,
                                size_t length) {
    KeccakP1600_AddBytes((KeccakP1600_plain64_state *)state, data, (unsigned int)offset,
                         (unsigned int)length);
}

void webcpp_hash_xkcp_permute(uint64_t *state) {
    KeccakP1600_Permute_24rounds((KeccakP1600_plain64_state *)state);
}

void webcpp_hash_xkcp_extract_bytes(const uint64_t *state, unsigned char *data, size_t offset,
                                    size_t length) {
    KeccakP1600_ExtractBytes((const KeccakP1600_plain64_state *)state, data, (unsigned int)offset,
                             (unsigned int)length);
}
```

  (`offset + length` never exceeds 200: the callers are `keccak_256`'s members, which the
  `unsigned int` casts rely on.) `include/webcpp/hash/detail/xkcp.hpp` declares the four
  functions in `extern "C"` with `std::uint64_t*` and `std::size_t`.
- [ ] **Step 4: Write the target** in `third_party/Jamfile`, after webcpp::crypto's C libraries
  (or, if they are not on `main` yet, with the shared `.c-library` and `.gc` property sets moved
  here as crypto's Task 5 wrote them):

```jam
# webcpp::hash's C libraries: its canaries (libs/hash/src/canary_<name>.c) and
# XKCP's adapter are webcpp::hash's, so a checkout without libs/hash stops
# these targets, naming the command, and leaves the others as they are.
path-constant HASH : ../libs/hash ;

# The property set of webcpp::hash's C libraries: crypto's .c-library less
# crypto's conditional and include directory, so that no target of hash's needs
# libs/crypto. hash-checked-out, below, is declared before the targets use it.
.hash-c-library = <link>static <warnings>off <warnings-as-errors>off
    <toolset>clang:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>gcc:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>emscripten:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>msvc:<cflags>"/Gy /Gw"
    <conditional>@hash-checked-out <include>$(HASH)/include ;

rule hash-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(HASH)/src/canary_xkcp.c ] ]
    {
        errors.user-error "libs/hash is not checked out: it holds no"
            "src/canary_xkcp.c, which the C libraries of webcpp::hash are built with."
            "Check it out with `git submodule update --init libs/hash`." ;
    }
}

# XKCP (CC0; brg_endian.h under Brian Gladman's terms), the Keccak team's code
# package: its Keccak-p[1600] alone, in the plain-64bits implementation with all
# rounds unrolled ("generic64"), on every target: wasm32 has 64-bit integers,
# MSVC compiles it with _rotl64, and XKCP's faster profiles are GNU assembly.
# Its config.h, which XKCP's build generates, is webcpp's src/xkcp/config.h.
path-constant XKCP : libs/xkcp ;

rule xkcp-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(XKCP)/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c ] ]
    {
        errors.user-error "third_party/libs/xkcp is not checked out. Check it out with"
            "`git submodule update --init third_party/libs/xkcp`." ;
    }
}

lib xkcp
  : libs/xkcp/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c
    $(HASH)/src/xkcp_adapter.c $(HASH)/src/canary_xkcp.c
  : $(.hash-c-library) <conditional>@xkcp-checked-out <include>$(HASH)/src/xkcp
    <include>libs/xkcp/lib/common <include>libs/xkcp/lib/low/common
    <include>libs/xkcp/lib/low/KeccakP-1600/common
    <include>libs/xkcp/lib/low/KeccakP-1600/plain-64bits
    <include>libs/xkcp/lib/low/KeccakP-1600/plain-64bits/SnP
  : : <define>WEBCPP_HASH_HAS_XKCP=1 $(.gc) ;
explicit xkcp ;
```

  (`.gc` is crypto's, already in the file.) Add `include/webcpp/hash/detail/canary.h` and
  `detail/xkcp.hpp` to the headers-alone list and the boundaries file. Run the case.
  Expected: PASS.
- [ ] **Step 5: XKCP's own suite** in `test/c_suites/Jamfile` (the form of webcpp::crypto's):

```jam
# (licence notice)

# The C libraries' own test programs, on webcpp's build of each, on every target
# (CODING-GUIDELINES.md chapter 5, rule 11). One lane line lists every program;
# each task that adds a suite adds its programs to it.
import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

path-constant XKCP : ../../../../third_party/libs/xkcp ;
webcpp.run xkcp_unit_tests : $(XKCP)/tests/UnitTests/main.c $(XKCP)/tests/UnitTests/UT.c
    $(XKCP)/tests/UnitTests/testPermutations.c
  : <testing.arg>--SnP <library>/third_party//xkcp <include>../../src/xkcp
    <include>$(XKCP)/tests/UnitTests <include>$(XKCP)/lib/common <include>$(XKCP)/lib/low/common
    <include>$(XKCP)/lib/low/KeccakP-1600/common
    <include>$(XKCP)/lib/low/KeccakP-1600/plain-64bits
    <include>$(XKCP)/lib/low/KeccakP-1600/plain-64bits/SnP
    <warnings>off <warnings-as-errors>off ;

webcpp.lane c-suites : xkcp_unit_tests : native wasip2 wasip3 emscripten ;
```

  This is the Jamfile's one `webcpp.lane c-suites` line; Task 8 adds libsodium's programs to it,
  never a second line (two would declare the alias `c-suites` twice).

  Build it natively first: `main.c` references the functions of every component its
  `XKCP_has_*` guards enable; with webcpp's `config.h` only Keccak-p[1600]'s. If the link names
  a further function of XKCP's test framework (`XKCP_ProcessCpuFeatureCommandLineOption`, for
  one), add the `tests/` or `lib/` source that defines it to this program, never to
  `/third_party//xkcp`, and record it in the Jamfile's comment. Run the lane on the four targets.
  Expected: PASS, the SnP tests of Keccak-p[1600] reporting success (their own output, recorded
  in the ledger). A failure on one target is investigated as a build difference first; XKCP's
  code is never changed.
- [ ] **Step 6: Document and commit.** Chapter 10's row `xkcp`: "XKCP (the Keccak team's code
  package) at `d20c8fc` (2026-11-10, no release tags); `third_party/Jamfile` compiles its
  `KeccakP-1600-opt64.c` alone (Keccak-p[1600], the generic64 profile) with webcpp::hash's
  adapter, `config.h` and canary, on every target; `WEBCPP_HASH_HAS_XKCP=1` | CC0;
  `brg_endian.h` under Brian Gladman's terms". The release notes, "Tools and build": XKCP's
  Keccak-p[1600] as `/third_party//xkcp`. Commit (superproject)
  `"third_party: XKCP's Keccak-p[1600], built with webcpp::hash's adapter and canary"`; (library)
  `"hash: XKCP's adapter and canary, and XKCP's own permutation tests"`.

## Task 5: `keccak_256`

**Files:**
- Create (library): `include/webcpp/hash/keccak.hpp`, `test/keccak_test.cpp`,
  `test/messages/xkcp_not_linked/{Jamfile,xkcp_not_linked.cpp}`, `test/messages/Jamfile`.
- Modify (library): `test/Jamfile`, `include/webcpp/hash.hpp`, `meta/include-boundaries.json`,
  `doc/changelog.adoc`.

**Interfaces:**
- Consumes: Task 4's adapter and canary. Produces: spec 2.6's `keccak_256`.

- [ ] **Step 1: Write the failing test** `test/keccak_test.cpp`:

```cpp
#include <webcpp/hash/keccak.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/hash2/sha3.hpp>

#include <webcpp/test/exit_status.hpp>

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace hash = webcpp::hash;

namespace {

// Boost.Hash2's own Keccak-256, under detail: the oracle of every call sequence (spec 1.3).
using oracle = boost::hash2::detail::keccak_base<0x01, 512, 256>;

std::string hex(const hash::keccak_256::result_type& digest) {
    return boost::hash2::to_string(digest);
}

void check_published() {
    hash::keccak_256 empty;
    BOOST_TEST_EQ(hex(empty.result()),
                  "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
    hash::keccak_256 abc;
    abc.update("abc", 3);
    BOOST_TEST_EQ(hex(abc.result()),
                  "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45");
}

// EIP-55: a hex digit of an address is upper case where the same position of Keccak-256 of the
// lowercase hex address is 8 or more. The four examples of EIP-55 ("All caps", "All lower",
// "Normal").
void check_eip55() {
    const std::array<std::string, 4> addresses{
        "52908400098527886E0F7030069857D2E4169EE7", "8617E340B3D01FA5F11F306F4090FD50E238070D",
        "de709f2102306220921060314715629080e2fb77", "5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed"};
    for (const auto& address : addresses) {
        std::string lower = address;
        for (auto& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        hash::keccak_256 hasher;
        hasher.update(lower.data(), lower.size());
        const std::string digest = hex(hasher.result());
        std::string checksummed = lower;
        for (std::size_t i = 0; i < checksummed.size(); ++i) {
            const int nibble = std::stoi(digest.substr(i, 1), nullptr, 16);
            if (std::isalpha(static_cast<unsigned char>(checksummed[i])) != 0 && nibble >= 8) {
                checksummed[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(checksummed[i])));
            }
        }
        BOOST_TEST_EQ(checksummed, address);
    }
}

// Every length 0 to 1,000 in one update, then random sequences of updates and results.
void check_oracle() {
    std::vector<unsigned char> data(1000);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<unsigned char>(i * 7 + 3);
    }
    for (std::size_t n = 0; n <= data.size(); ++n) {
        hash::keccak_256 ours;
        oracle theirs;
        ours.update(data.data(), n);
        theirs.update(data.data(), n);
        BOOST_TEST(ours.result() == theirs.result());
    }
    constexpr std::uint32_t seed = 20261114;
    std::mt19937 random(seed);
    for (int sequence = 0; sequence < 200; ++sequence) {
        hash::keccak_256 ours(static_cast<std::uint64_t>(sequence));
        oracle theirs = seeded_oracle(static_cast<std::uint64_t>(sequence));
        for (int call = 0; call < 20; ++call) {
            if (random() % 3 == 0) {
                BOOST_TEST(ours.result() == theirs.result());
            } else {
                const std::size_t n = random() % 300;
                ours.update(data.data(), n);
                theirs.update(data.data(), n);
            }
        }
        if (!BOOST_TEST(ours.result() == theirs.result())) {
            std::printf("keccak oracle: seed %u, sequence %d\n", seed, sequence);
        }
    }
}

}  // namespace

int main() {
    check_published();
    check_eip55();
    check_oracle();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  where `seeded_oracle(seed)` does what Hash2's `sha3_256(std::uint64_t)` does to its base
  (`write64le`, `update(tmp, 8)`, `result()`, `update(tmp, 0)` when the seed is not zero). Declare
  `hash-run keccak : keccak_test.cpp : <library>/third_party//xkcp ;`. Run it. Expected: FAIL,
  no `keccak.hpp`.
- [ ] **Step 2: Write `keccak.hpp`** (spec 2.6). The XKCP check first:

```cpp
#ifndef WEBCPP_HASH_HAS_XKCP
// clang-format off
#error "webcpp::hash: keccak_256 needs XKCP. Add /third_party//xkcp to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//xkcp ;"
// clang-format on
#endif
```

  then the canary's declaration and `detect_mismatch` as webcpp::crypto's backend headers write
  them, and the members, each reading the canary through
  `static_cast<void>(*static_cast<volatile const char*>(&WEBCPP_HASH_DETAIL_CANARY(...)))`:

```cpp
inline keccak_256::keccak_256() noexcept : state_{} {
    detail::touch_xkcp_canary();
    webcpp_hash_xkcp_initialize(state_.data());
}

inline void keccak_256::update(unsigned char const* p, std::size_t n) noexcept {
    detail::touch_xkcp_canary();
    finalized_ = false;
    if (n == 0) {
        return;
    }
    if (m_ > 0) {
        std::size_t k = block_size - m_;
        if (n < k) {
            k = n;
        }
        webcpp_hash_xkcp_add_bytes(state_.data(), p, m_, k);
        p += k;
        n -= k;
        m_ += k;
        if (m_ < block_size) {
            return;
        }
        webcpp_hash_xkcp_permute(state_.data());
        m_ = 0;
    }
    while (n >= block_size) {
        webcpp_hash_xkcp_add_bytes(state_.data(), p, 0, block_size);
        webcpp_hash_xkcp_permute(state_.data());
        p += block_size;
        n -= block_size;
    }
    if (n > 0) {
        webcpp_hash_xkcp_add_bytes(state_.data(), p, 0, n);
        m_ = n;
    }
}

inline keccak_256::result_type keccak_256::result() noexcept {
    detail::touch_xkcp_canary();
    if (!finalized_) {
        constexpr unsigned char first = 0x01;  // Keccak's original padding delimiter
        constexpr unsigned char last = 0x80;
        webcpp_hash_xkcp_add_bytes(state_.data(), &first, m_, 1);
        webcpp_hash_xkcp_add_bytes(state_.data(), &last, block_size - 1, 1);
        m_ = 0;
        finalized_ = true;
    }
    webcpp_hash_xkcp_permute(state_.data());
    result_type digest;
    webcpp_hash_xkcp_extract_bytes(state_.data(), digest.data(), 0, digest.size());
    return digest;
}
```

  (`update`'s early return keeps `finalized_ = false` before it, as Hash2's does.) The seeded
  constructors follow `sha3_256`'s. Run the test on the four targets. Expected: PASS.
- [ ] **Step 3: The message.** `test/messages/xkcp_not_linked/xkcp_not_linked.cpp` includes
  `keccak.hpp` and states spec 5.7's first text as an `expected-error`; its Jamfile declares
  `webcpp.compile-diagnostic xkcp_not_linked : xkcp_not_linked.cpp : : native wasip2 wasip3 ;`
  without `/third_party//xkcp`. Run it. Expected: PASS. Add `keccak.hpp` to a headers-alone call
  with `<library>/third_party//xkcp`, to the boundaries file, and to `hash.hpp` under
  `#ifdef WEBCPP_HASH_HAS_XKCP`; the changelog: "Added keccak_256, Ethereum's Keccak-256, over
  XKCP's permutation."
- [ ] **Step 4: Commit** (library) `"hash: keccak_256 over XKCP's Keccak-p[1600], with Hash2's sponge"`.

## Task 6: BLAKE3's C implementation, file by file

**Files:**
- Create (superproject): the submodule `third_party/libs/blake3`; `tools/test/blake3_c_suite.py`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (`blake3`, `blake3_vectors`),
  `tools/test/third_party_test.py`, `CODING-GUIDELINES.md` (chapter 10's rows `blake3`), the
  release notes.
- Create (library): `src/canary_blake3.c`. Modify: `include/webcpp/hash/detail/canary.h`.

**Interfaces:**
- Produces: `/third_party//blake3` (usage requirements `<include>third_party/libs/blake3/c`,
  `WEBCPP_HASH_HAS_BLAKE3=1`, the gc flags); `/third_party//blake3_vectors`
  (`WEBCPP_TEST_BLAKE3_VECTORS`, `webcpp.data`, a dependency on the file); the symbols
  `webcpp_hash_blake3_f3149ec_{x86,neon,portable}__add_third_party_blake3_to_your_link`.

- [ ] **Step 1: Add the submodule,** pinned:

```bash
git submodule add https://github.com/BLAKE3-team/BLAKE3.git third_party/libs/blake3
git -C third_party/libs/blake3 checkout f3149ec5bb5449af877ba20377a11008ff499fa2
grep -n 'BLAKE3_VERSION_STRING' third_party/libs/blake3/c/blake3.h
```

  Expected: `#define BLAKE3_VERSION_STRING "1.8.7"`.
- [ ] **Step 2: Write the failing cases** in `tools/test/third_party_test.py`: natively on this
  machine, `/third_party//blake3` compiles `blake3.c`, `blake3_dispatch.c`, `blake3_portable.c`,
  the canary, and on x86-64 the four intrinsics files each with its flag (`-msse2`, `-msse4.1`,
  `-mavx2`, `-mavx512f -mavx512vl`), on arm64 `blake3_neon.c` with `-DBLAKE3_USE_NEON=1`; with
  `toolset=clang-wasip2` the three portable files and the canary alone, each with the four
  `-DBLAKE3_NO_*`; `nm` lists the configuration's canary. Expected: FAIL, no target `blake3`.
- [ ] **Step 3: Write `src/canary_blake3.c`** (the configurations of spec 5.4: `portable` when the
  four `BLAKE3_NO_*` are defined; `neon` when `BLAKE3_USE_NEON == 1`; `x86` when none of those and
  the target is x86-64; `unconfigured` otherwise) and BLAKE3's commit in `detail/canary.h`
  (`f3149ec`).
- [ ] **Step 4: Write the targets** in `third_party/Jamfile`:

```jam
# BLAKE3's C implementation (CC0-1.0, or Apache-2.0, or Apache-2.0 with LLVM
# exception), c/ of BLAKE3-team/BLAKE3. blake3.c, blake3_dispatch.c and
# blake3_portable.c on every target; the intrinsics files only on the targets
# of their instruction sets, each with its flag, as BLAKE3's own CMake builds
# them where it has no assembler: they include x86 or Arm headers that no
# WebAssembly sysroot has (a recorded exception to CODING-GUIDELINES.md
# chapter 5 rule 2). BLAKE3's dispatcher chooses at run time. On WebAssembly
# the four BLAKE3_NO_* leave the portable code alone. Never BLAKE3_USE_TBB.
path-constant BLAKE3 : libs/blake3 ;

rule blake3-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(BLAKE3)/c/blake3.h ] ]
    {
        errors.user-error "third_party/libs/blake3 is not checked out. Check it out with"
            "`git submodule update --init third_party/libs/blake3`." ;
    }
}

# Each intrinsics file is an object of its own, so that its flag reaches it
# alone; on a target without its instruction set the object is empty of
# sources (b2's <build>no), which the third_party test checks.
rule blake3-x86 ( properties * )
{
    if [ webcpp-target.target-of $(properties) ] = native && <architecture>x86 in $(properties)
    {
        return ;
    }
    return <build>no ;
}

obj blake3_sse2 : libs/blake3/c/blake3_sse2.c
  : $(.hash-c-library) <conditional>@blake3-x86 <toolset>clang:<cflags>-msse2 <toolset>gcc:<cflags>-msse2 ;
obj blake3_sse41 : libs/blake3/c/blake3_sse41.c
  : $(.hash-c-library) <conditional>@blake3-x86 <toolset>clang:<cflags>-msse4.1 <toolset>gcc:<cflags>-msse4.1 ;
obj blake3_avx2 : libs/blake3/c/blake3_avx2.c
  : $(.hash-c-library) <conditional>@blake3-x86 <toolset>clang:<cflags>-mavx2 <toolset>gcc:<cflags>-mavx2
    <toolset>msvc:<cflags>/arch:AVX2 ;
obj blake3_avx512 : libs/blake3/c/blake3_avx512.c
  : $(.hash-c-library) <conditional>@blake3-x86
    <toolset>clang:<cflags>"-mavx512f -mavx512vl" <toolset>gcc:<cflags>"-mavx512f -mavx512vl"
    <toolset>msvc:<cflags>/arch:AVX512 ;
explicit blake3_sse2 blake3_sse41 blake3_avx2 blake3_avx512 ;

lib blake3
  : libs/blake3/c/blake3.c libs/blake3/c/blake3_dispatch.c libs/blake3/c/blake3_portable.c
    $(HASH)/src/canary_blake3.c
  : $(.hash-c-library) <conditional>@hash-checked-out <conditional>@blake3-checked-out
    <include>$(HASH)/include <include>libs/blake3/c <conditional>@blake3-profile
  : : <include>libs/blake3/c <define>WEBCPP_HASH_HAS_BLAKE3=1 $(.gc) ;
explicit blake3 ;
```

  `third_party/Jamfile` gains `import webcpp-target ;` beside its imports (the rules read
  `webcpp-target.target-of`). Where `blake3-profile` returns, by target: on WebAssembly the four `<define>BLAKE3_NO_*`; on
  native x86-64 `<source>blake3_sse2 <source>blake3_sse41 <source>blake3_avx2
  <source>blake3_avx512`; on native arm64 `<source>libs/blake3/c/blake3_neon.c
  <define>BLAKE3_USE_NEON=1`. Read the architecture from b2's `<architecture>` and
  `<address-model>` as `tools/webcpp-openssl.jam` does; if b2 does not set `<architecture>` on
  the native toolset of a machine, take it from `webcpp-target`'s host detection and record the
  rule in the Jamfile's comment. And the test data, the form of `wycheproof`:

```jam
# BLAKE3's official vectors, test_vectors/test_vectors.json at the pin, read at
# run time from the directory WEBCPP_TEST_BLAKE3_VECTORS gives, on every target.
# Quoted as wycheproof's define is, with forward slashes, so that a path with a
# space or a backslash survives.
.blake3-vectors = [ regex.replace $(BLAKE3)/test_vectors "\\\\" "/" ] ;
alias blake3_vectors : : <conditional>@blake3-checked-out : :
    <define>"\"WEBCPP_TEST_BLAKE3_VECTORS=\\\"$(.blake3-vectors)\\\"\""
    [ webcpp.data $(BLAKE3)/test_vectors ]
    <dependency>libs/blake3/test_vectors/test_vectors.json ;
explicit blake3_vectors ;
```

  Run the cases. Expected: PASS.
- [ ] **Step 5: BLAKE3's own suite.** BLAKE3's C test is `c/test.py`, which runs `c/blake3`, a
  binary built inside the submodule's `c/` directory, where webcpp builds nothing; and a library's
  test Jamfile may not declare b2's raw `exe` (the lint's raw-rules rule). So it is a test of the
  tools, run by the superproject's tools job, as the user's Jamfiles are (CODING-GUIDELINES.md
  chapter 8): `tools/test/blake3_c_suite.py` builds the fixture
  `tools/test/fixtures/blake3_c_suite/Jamfile` on a scratch superproject for each target,

```jam
# BLAKE3's own test program, c/main.c with BLAKE3_TESTING, on webcpp's build of
# BLAKE3; tools/test/blake3_c_suite.py drives it as c/test.py drives c/blake3.
path-constant BLAKE3 : ../../../../third_party/libs/blake3 ;
exe blake3_main : $(BLAKE3)/c/main.c
  : <library>/third_party//blake3 <define>BLAKE3_TESTING <warnings>off <warnings-as-errors>off ;
```

  and runs it over every case of `test_vectors.json` as `c/test.py` does: the input of a case is
  `i % 251` for each `i` below `input_len`, given on standard input; the program runs with no
  argument, with `--length <n>`, with `--keyed <hex of the key>` and with `--derive-key
  <context>` (the arguments `c/main.c` parses at the pin), and each output line is compared with
  the JSON's; natively directly, on wasip2 and wasip3 through `tools/boost/asio-wasip2/run.sh` and
  `.../asio-wasip3/run.sh`, on Emscripten through node. It prints
  `blake3 c suite: <target>: 35 cases ok` per target (the file at the pin holds 35 cases, the key
  "whats the Elvish word for friend" and outputs of 131 bytes) or the first difference, and exits
  non-zero on a difference. Run `python3 tools/test/blake3_c_suite.py`. Expected: four lines,
  `native`, `wasip2`, `wasip3` and `emscripten`, each `35 cases ok`.
- [ ] **Step 6: Document and commit.** Chapter 10's rows `blake3` ("BLAKE3 1.8.7 at `f3149ec`;
  `third_party/Jamfile` compiles `c/blake3.c`, `blake3_dispatch.c` and `blake3_portable.c` on
  every target, the intrinsics files on their instruction sets alone (an exception to chapter 5
  rule 2), with webcpp::hash's canary; `WEBCPP_HASH_HAS_BLAKE3=1` | CC0-1.0, Apache-2.0, or
  Apache-2.0 with LLVM exception") and `blake3_vectors`. Commit (superproject)
  `"third_party: BLAKE3's C implementation, portable on WebAssembly and dispatched natively"`;
  (library) `"hash: BLAKE3's canary, and BLAKE3's own test program on webcpp's build"`.

## Task 7: `blake3`

**Files:**
- Create (library): `include/webcpp/hash/blake3.hpp`, `test/blake3_test.cpp`,
  `test/blake3_vectors_test.cpp`, `test/messages/blake3_not_linked/{Jamfile,blake3_not_linked.cpp}`.
- Modify (library): `test/Jamfile`, `test/messages/Jamfile`, `include/webcpp/hash.hpp`,
  `meta/include-boundaries.json`, `doc/changelog.adoc`.

**Interfaces:**
- Produces: spec 2.7's `blake3`.

- [ ] **Step 1: Write the failing tests.** `blake3_vectors_test.cpp` reads
  `WEBCPP_TEST_BLAKE3_VECTORS "/test_vectors.json"` with Boost.JSON (header-only parse, as
  webcpp::crypto's Wycheproof tests do), and for each case: the input `i % 251`; `blake3` plain,
  `blake3::keyed` with the file's `key` (32 ASCII bytes), `blake3::derive_key` with its
  `context_string`; for each, `squeeze` of the expected length (131 bytes) equals the hex, and
  repeated `result()` concatenated to that length equals it too. `blake3_test.cpp`:

```cpp
void check_empty() {
    webcpp::hash::blake3 hasher;
    BOOST_TEST_EQ(boost::hash2::to_string(hasher.result()),
                  "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262");
}

// A bytes seed of exactly 32 bytes is BLAKE3's keyed mode with that key.
void check_seed_is_key() {
    std::array<std::uint8_t, 32> key{};
    for (std::size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<std::uint8_t>(i);
    }
    webcpp::hash::blake3 seeded(key.data(), key.size());
    auto keyed = webcpp::hash::blake3::keyed(key);
    seeded.update("x", 1);
    keyed.update("x", 1);
    BOOST_TEST(seeded.result() == keyed.result());
}

// Hash2's contract: results never repeat for one history; update after result continues.
void check_result_sequence() {
    webcpp::hash::blake3 a;
    a.update("abc", 3);
    const auto first = a.result();
    const auto second = a.result();
    BOOST_TEST(first != second);
    std::array<std::uint8_t, 64> stream{};
    a.squeeze(stream);
    BOOST_TEST(std::equal(first.begin(), first.end(), stream.begin()));
    BOOST_TEST(std::equal(second.begin(), second.end(), stream.begin() + 32));
    a.update("d", 1);
    webcpp::hash::blake3 b;
    b.update("abcd", 4);
    BOOST_TEST(a.result() == b.result());
}
```

  plus `uint64_t` seeds (0 is plain; 1 equals the bytes seed of `01 00 00 00 00 00 00 00`), a
  41-byte seed (key of the first 32, the rest absorbed and finalized), `derive_key` with a
  context holding a zero byte, `reset()`. Declare both with
  `<library>/third_party//blake3` (and `/third_party//blake3_vectors` for the vectors). Run them.
  Expected: FAIL, no `blake3.hpp`.
- [ ] **Step 2: Write `blake3.hpp`** (spec 2.7), the check of `WEBCPP_HASH_HAS_BLAKE3` with spec
  5.7's text, the canary, `detect_mismatch`; `result()`:

```cpp
inline blake3::result_type blake3::result() noexcept {
    detail::touch_blake3_canary();
    result_type output;
    blake3_hasher_finalize_seek(&hasher_, squeezed_, output.data(), output.size());
    squeezed_ += output.size();
    return output;
}

inline void blake3::update(unsigned char const* p, std::size_t n) noexcept {
    detail::touch_blake3_canary();
    if (n == 0) {
        return;
    }
    blake3_hasher_update(&hasher_, p, n);
    squeezed_ = 0;
}
```

  The bytes seed: `n == 0` plain; else a 32-byte key of `min(n, 32)` bytes zero-padded,
  `blake3_hasher_init_keyed`, then for `n > 32` `update(p + 32, n - 32)` and `result()` (whose
  advance of `squeezed_` the next `update` resets, as Hash2's seeded blake2b does). Run both
  tests on the four targets. Expected: PASS.
- [ ] **Step 3: The message** `blake3_not_linked` (spec 5.7), the headers-alone call with
  `<library>/third_party//blake3`, the boundaries file, `hash.hpp` under
  `#ifdef WEBCPP_HASH_HAS_BLAKE3`, the changelog ("Added blake3, BLAKE3 with keyed hashing, key
  derivation and its extendable output.").
- [ ] **Step 4: Commit** (library) `"hash: blake3 over BLAKE3's C implementation, in Hash2's form"`.

## Task 8: libsodium, file by file

**Files:**
- Create (superproject): the submodule `third_party/libs/libsodium`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (`sodium`, `sodium-version`,
  `sodium-test-config`), `tools/test/third_party_test.py`, `CODING-GUIDELINES.md` (chapter 10's
  row `libsodium`), the release notes.
- Create (library): `src/canary_sodium.c`, `test/c_suites/sodium_bound.cpp`. Modify:
  `include/webcpp/hash/detail/canary.h`, `test/c_suites/Jamfile`.

**Interfaces:**
- Produces: `/third_party//sodium` (usage requirements
  `<include>third_party/libs/libsodium/src/libsodium/include`, `<define>SODIUM_STATIC=1`,
  `<define>WEBCPP_HASH_HAS_SODIUM=1`, the gc flags); `/third_party//sodium-test-config` (the
  library's sources' include directories, `version.h`'s directory and the same defines, for
  programs that compile against libsodium's private headers); the symbol
  `webcpp_hash_sodium_77e1ce5_configured__add_third_party_sodium_to_your_link`.

- [ ] **Step 1: Add the submodule,** pinned:

```bash
git submodule add https://github.com/jedisct1/libsodium.git third_party/libs/libsodium
git -C third_party/libs/libsodium checkout 77e1ce5d6dee871c49ef211222ba18ef0c486bda
find third_party/libs/libsodium/src/libsodium -name '*.c' | wc -l
find third_party/libs/libsodium/src/libsodium -name '*.S' | wc -l
grep -n 'SODIUM_VERSION_STRING' third_party/libs/libsodium/builds/msvc/version.h
```

  Expected: 141, 7, `#define SODIUM_VERSION_STRING "1.0.22"`.
- [ ] **Step 2: Derive the configuration defines from libsodium's own `configure`, once per
  target** (the ruling of 2026-11-15, which drops `build.zig`'s lists). In a scratch copy of the
  submodule (never the submodule itself), `./autogen.sh -s` once, then for each target:

```bash
# native, on each native machine of the matrix (macOS arm64, Linux x86-64, Linux arm64)
./configure --disable-shared --enable-static --disable-asm --disable-dependency-tracking
# wasip2 and wasip3 (wasi-sdk 34's clang, as libsodium's dist-build/wasm32-wasi.sh at the pin)
CC=<wasi-sdk>/bin/clang CFLAGS="--target=wasm32-wasip2" ./configure --host=wasm32-wasi \
  --disable-shared --enable-static --disable-asm --without-pthreads --disable-ssp
# Emscripten (as dist-build/emscripten.sh at the pin, with its flags)
emconfigure ./configure --disable-shared --enable-static --disable-asm --without-pthreads --disable-ssp
```

  and record, for each, the `DEFS` that `./config.status --config` and the generated Makefile give
  (every `-DHAVE_*` and the others): in this plan, by an amendment of the proposal (a table, one
  column per target), and beside the sources in `third_party/Jamfile`. MSVC has no `configure`:
  its defines are those of `builds/msvc/properties/` at the pin, recorded the same way. Rules: each
  `HAVE_*` only on the targets whose `configure` found it; `DEV_MODE` off everywhere;
  `CONFIGURED=1`, `SODIUM_STATIC=1` and `NATIVE_LITTLE_ENDIAN=1` everywhere; `HAVE_AMD64_ASM` and
  `HAVE_AVX_ASM` never (`--disable-asm`, design D11); a define the C sources do not read is
  recorded and omitted. The table replaces spec 5.3's prose when recorded.
- [ ] **Step 3: Write the failing cases** in `tools/test/third_party_test.py`: natively,
  `/third_party//sodium` with `-a -d+2` compiles 142 C files (the 141 and the canary), none with
  `-std=`, each with `-DCONFIGURED=1`, `-DSODIUM_STATIC=1`, `-DNATIVE_LITTLE_ENDIAN=1`, none with
  `-DHAVE_AMD64_ASM`; the build directory holds `sodium/version.h` byte-identical to
  `builds/msvc/version.h`; `git -C third_party/libs/libsodium status --porcelain` is empty after
  the build; `nm` lists the canary. Expected: FAIL, no target `sodium`.
- [ ] **Step 4: Write `src/canary_sodium.c`** (configuration `configured` exactly when
  `CONFIGURED == 1`, `SODIUM_STATIC == 1`, `NATIVE_LITTLE_ENDIAN == 1` and `HAVE_AMD64_ASM` is not
  defined; else `unconfigured`), libsodium's commit in `detail/canary.h` (`77e1ce5`), and the
  targets:

```jam
# libsodium 1.0.22 (ISC), jedisct1/libsodium at 77e1ce5: every .c of
# src/libsodium/, one object per file, with the defines of libsodium's own
# build.zig for each target (read at the pin; each group below cites it),
# without its assembly: HAVE_AMD64_ASM and HAVE_AVX_ASM stay off, so that the
# seven .S files are not needed and the C code is used (design D11). Shared by
# webcpp::hash and, later, webcpp::crypto: one copy per program.
path-constant SODIUM : libs/libsodium ;

rule sodium-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(SODIUM)/src/libsodium/include/sodium/core.h ] ]
    {
        errors.user-error "third_party/libs/libsodium is not checked out. Check it out with"
            "`git submodule update --init third_party/libs/libsodium`." ;
    }
}

# version.h is generated by libsodium's configure; upstream ships a generated
# copy, builds/msvc/version.h, which its build.zig copies into the tree. b2
# copies it, unchanged, into the build directory instead: the submodule is
# never written.
make sodium/version.h : libs/libsodium/builds/msvc/version.h : common.copy
  : <conditional>@sodium-checked-out ;
explicit sodium/version.h ;

.sodium-defines = <define>CONFIGURED=1 <define>SODIUM_STATIC=1 <define>NATIVE_LITTLE_ENDIAN=1
    <define>HAVE_INTTYPES_H=1 <define>HAVE_STDINT_H=1
    <conditional>@sodium-target-defines ;   # spec 5.3's groups, by target

lib sodium
  : [ glob-tree-ex libs/libsodium/src/libsodium : *.c ] $(HASH)/src/canary_sodium.c
  : $(.hash-c-library) <conditional>@hash-checked-out <conditional>@sodium-checked-out
    $(.sodium-defines) <implicit-dependency>sodium/version.h
    <include>$(HASH)/include <include>libs/libsodium/src/libsodium/include
    <include>libs/libsodium/src/libsodium/include/sodium <conditional>@sodium-version-include
  : : <include>libs/libsodium/src/libsodium/include <define>SODIUM_STATIC=1
    <define>WEBCPP_HASH_HAS_SODIUM=1 $(.gc) ;
explicit sodium ;
```

  where `sodium-version-include` gives the directory b2 wrote `sodium/version.h` into, and that
  directory's `sodium/` (the form `third_party/Jamfile` already uses for a generated header's
  directory; read it before writing; a target id with `/` may be read as a project path, so name
  the `make` target `sodium_version_h` and place its output under `sodium/` if b2 refuses the
  slash). `sodium-target-defines` returns Step 2's table by `webcpp-target.target-of`,
  `<target-os>` and the architecture, each group under a comment citing its `configure` run. `sodium-test-config` is an alias with the same include directories
  (plus `src/libsodium/include/sodium/private`'s parent and `crypto_pwhash/argon2`) and defines as
  usage requirements, for programs that include libsodium's private headers. Run the cases.
  Expected: PASS.
- [ ] **Step 5: libsodium's own suite.** In `test/c_suites/Jamfile`, one `webcpp.run` per
  `test/default/<name>.c` at the pin (101), generated by a Jam loop over a list written out in
  the Jamfile (so that a pin move that adds a test changes the list visibly):

```jam
path-constant SODIUM_TESTS : ../../../../third_party/libs/libsodium/test/default ;
.sodium-tests-dir = [ regex.replace $(SODIUM_TESTS) "\\\\" "/" ] ;
# cmptest.h writes <name>.res in the working directory: a writable directory of
# the build, given to wasmtime on WASI (webcpp.data) and made the working
# directory of every run, so that the suite runs on every target (chapter 5,
# rule 11; the ruling of 2026-11-15).
path-constant SODIUM_RESULTS : ../../../../bin/sodium-results ;
# libsodium's own tests (101 at 77e1ce5), each with cmptest.h, which writes
# <name>.res in the working directory and compares it with <name>.exp under
# TEST_SRCDIR (Emscripten: /test-data). Each runs on every target; a test a
# target cannot run is declared without that target, with its reason.
.sodium-tests =
    aead_aegis128l aead_aegis256 aead_aes256gcm aead_aes256gcm2 aead_chacha20poly1305
    aead_chacha20poly13052 aead_xchacha20poly1305 auth auth2 auth3 auth5 auth6 auth7 box
    box2 box7 box8 box_easy box_easy2 box_seal box_seed chacha20 codecs codecs2 codecs3
    core1 core2 core3 core4 core5 core6 core_ed25519 core_keccak1600 core_ristretto255
    ed25519_convert generichash generichash2 generichash3 hash hash3 hash_sha3256
    hash_sha3512 ipcrypt kdf kdf_hkdf kdf_hkdf2 kdf_hkdf3 kdf_hkdf4 kem kem_mlkem768
    kem_xwing keygen kx metamorphic misuse onetimeauth onetimeauth2 onetimeauth7
    pwhash_argon2i pwhash_argon2id pwhash_argon2id2 pwhash_scrypt pwhash_scrypt_ll
    randombytes scalarmult scalarmult2 scalarmult5 scalarmult6 scalarmult7 scalarmult8
    scalarmult_ed25519 scalarmult_ristretto255 secretbox secretbox2 secretbox7 secretbox8
    secretbox_easy secretbox_easy2 secretstream_xchacha20poly1305 shorthash sign siphashx24
    sodium_core sodium_utils sodium_utils2 sodium_utils3 sodium_utils4 sodium_version
    stream stream2 stream3 stream4 stream5 stream6 verify1 xchacha20 xof2 xof_shake128
    xof_shake256 xof_turboshake128 xof_turboshake256
    ;
for local name in $(.sodium-tests)
{
    webcpp.run sodium_$(name) : $(SODIUM_TESTS)/$(name).c
      : <library>/third_party//sodium-test-config <library>/third_party//sodium
        <define>"\"TEST_NAME=\\\"$(name)\\\"\""
        <define>"\"TEST_SRCDIR=\\\"$(.sodium-tests-dir)\\\"\""
        <include>$(SODIUM_TESTS) <include>$(SODIUM_TESTS)/../quirks
        [ webcpp.data $(SODIUM_TESTS) ] [ webcpp.data $(SODIUM_RESULTS) ]
        <warnings>off <warnings-as-errors>off
        <toolset>emscripten:<source>sodium_bound.cpp ;
}
# Added to Task 4's lane line, the Jamfile's only one:
# webcpp.lane c-suites : xkcp_unit_tests sodium_$(.sodium-tests) : native wasip2 wasip3 emscripten ;
```

  Run natively first; then wasip2, wasip3 and Emscripten. Record each test's time per target in
  the ledger; a test past the node bound (120 s) gets `sodium_bound.cpp`'s
  `webcpp::test::bound` with the measured time, and one past the WASI launcher's 600 s bound gets the bound
  raised for it alone, with its measured time and reason; no target is dropped. On WASI, the run's
  working directory must be `SODIUM_RESULTS`, preopened writable: if
  `tools/boost/asio-wasip2/run.sh` (and `asio-wasip3`'s) maps `webcpp.data` directories read-only
  or does not change into one, extend the launcher in this task with a directory it maps writable
  and runs in, with a case in `tools/test/`; natively the run changes into it the same way (the
  `.res` files never land in b2's working directory). On Emscripten, `cmptest.h` forces
  `TEST_SRCDIR "/test-data"`: the node runner mounts the test directory there and a writable
  working directory (extended the same way if it has no mount), so the suite runs on all four
  targets. A run that writes no `.res` fails, never passes. Expected: every declared test passes; `sodium_init` succeeds under
  node (Review Focus 6). A failing test is investigated as a build difference (a define of spec
  5.3) before anything else; libsodium's code is never changed.
- [ ] **Step 6: Document and commit.** Chapter 10's row `libsodium`: "libsodium 1.0.22 at
  `77e1ce5`; a static library built file by file by `third_party/Jamfile` (every `.c` of
  `src/libsodium/`, no assembly, `build.zig`'s defines per target, upstream's
  `builds/msvc/version.h` copied into the build directory) with webcpp::hash's canary, on every
  target; `WEBCPP_HASH_HAS_SODIUM=1`; shared with webcpp::crypto | ISC". Commit (superproject)
  `"third_party: libsodium 1.0.22, built file by file with webcpp::hash's canary"`; (library)
  `"hash: libsodium's canary, and libsodium's own tests on webcpp's build"`.

## Task 9: The key derivations' layer, and HKDF on `backends::hash2`

**Files:**
- Create (library): `test/messages/Jamfile` (a temporary copy until Task 5's merges),
  `test/messages/kdf_not_digest/`, `include/webcpp/hash/kdf.hpp`, `include/webcpp/hash/hkdf.hpp`,
  `include/webcpp/hash/backends/hash2.hpp`, `include/webcpp/hash/detail/engines.hpp`,
  `include/webcpp/hash/detail/limits.hpp`, `include/webcpp/hash/detail/wipe.hpp`,
  `include/webcpp/hash/detail/ranking.hpp` (the provisional order, spec 4.2),
  `test/support/rfc5869.hpp`, `test/hkdf_hash2_test.cpp`, `test/hkdf_guards_test.cpp`,
  `test/wipe_test.cpp`.
- Modify (library): `test/Jamfile`, `include/webcpp/hash.hpp`, `meta/include-boundaries.json`,
  `doc/changelog.adoc`.

**Interfaces:**
- Consumes: Task 2's `error`, Task 3's hmac aliases.
- Produces: `backend_has_v`, `default_kdf_backend` (spec 2.8; increment 1's rule, the profile of
  Task 14 joins it), `hkdf_extract`, `hkdf_expand`, `hkdf`, `hkdf_max_info_size`; the engines'
  primary templates (all four, so that later tasks only specialise); `detail::wipe`,
  `detail::exceeds`; the seam `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD`.

- [ ] **Step 1: Write the RFC 5869 data** `test/support/rfc5869.hpp`: the seven cases of the RFC's
  appendix A (hash, IKM, salt, info, L, PRK, OKM, each as a hex string copied from the RFC's text
  and checked against it twice); A.1 is

```cpp
// RFC 5869, A.1: basic test case with SHA-256.
inline constexpr rfc5869_case a1{
    "sha256",
    "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
    "000102030405060708090a0b0c",
    "f0f1f2f3f4f5f6f7f8f9",
    42,
    "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
    "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865"};
```

- [ ] **Step 2: Write the failing tests.** `hkdf_hash2_test.cpp` runs the seven cases through
  `hkdf_extract`, `hkdf_expand` and `hkdf` on `backends::hash2` (A.1 to A.3 with
  `hmac_sha2_256`, A.4 to A.7 with `hmac_sha1_160`), and one case each over `hmac_sha3_256`,
  `hmac_blake2b_512` and `hmac<boost::hash2::ripemd_160>`, whose expected outputs are computed in
  this task by an independent implementation (Python's `hmac` over `hashlib.sha3_256`,
  `hashlib.blake2b` and `hashlib.new('ripemd160')`, or, where the machine's `hashlib` lacks
  RIPEMD-160, a pure-Python RIPEMD-160 cited by its source), the script and its output recorded
  in the ledger; Task 12 then checks the same three on OpenSSL.
  `hkdf_guards_test.cpp`: for `hmac_sha2_256`, outputs of 0, 8,160 and 8,161 bytes; PRKs of 31,
  32 and 33 bytes; info of 32,768 and 32,769 bytes; each refusal leaves a span of `0xaa` bytes
  untouched. `backends/hash2.hpp` defines `struct backends::hash2 {};` (Tasks 12 and 13 define
  their backends' structs likewise). `wipe_test.cpp`: `detail::wipe` zeroes a span and a trivially copyable object, and,
  built with `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD`, `hkdf` reports the wipes the engine does: one
  per block's `mac` copy, one of `t` after the loop, one of `keyed`. Declare the three with `hash-run`. Run them. Expected: FAIL, no `hkdf.hpp`.
- [ ] **Step 3: Write the layer.** `detail/engines.hpp`:

```cpp
namespace webcpp::hash::detail {

template <class T>
inline constexpr bool always_false = false;

// The engines a backend specialises (spec 2.12). A primary template is reached only by a
// backend that has no specialisation for the call, and refuses with spec 3.3's text.
template <class Backend, class Hmac>
struct hkdf_engine {
    static_assert(always_false<Backend>,
                  "webcpp::hash: this backend has no HKDF. The backends that have HKDF: "
                  "backends::hash2 (every hmac), backends::openssl (every hmac but "
                  "hmac_ripemd_128), backends::sodium (hmac_sha2_256, hmac_sha2_512) and "
                  "backends::bearssl (MD5, SHA-1, SHA-224, SHA-256, SHA-384, SHA-512; hkdf alone).");
};
template <class Backend, class Hmac>
struct pbkdf2_engine {
    static_assert(always_false<Backend>,
                  "webcpp::hash: this backend has no PBKDF2. The backends that have pbkdf2: "
                  "backends::hash2 (every hmac) and backends::openssl (every hmac but "
                  "hmac_ripemd_128).");
};
template <class Backend>
struct argon2id_engine {
    static_assert(always_false<Backend>,
                  "webcpp::hash: argon2id is in backends::sodium alone. Name backends::sodium "
                  "and add /third_party//sodium to the program's Jamfile.");
};
template <class Backend>
struct scrypt_engine {
    static_assert(always_false<Backend>,
                  "webcpp::hash: scrypt is in backends::sodium alone. Name backends::sodium and "
                  "add /third_party//sodium to the program's Jamfile.");
};

// Whether Hmac is a Boost.Hash2 hmac over a hash whose result is a digest.
template <class Hmac>
struct is_digest_hmac : std::false_type {};
template <class H>
struct is_digest_hmac<boost::hash2::hmac<H>>
    : std::bool_constant<boost::hash2::has_constant_size<typename H::result_type>::value &&
                         !std::is_integral_v<typename H::result_type>> {};

}  // namespace webcpp::hash::detail
```

  `hkdf.hpp`'s functions (the guards of spec 2.8, then the engine):

```cpp
template <class Hmac, class Backend>
boost::system::result<void> hkdf_expand(std::span<std::uint8_t> output,
                                        std::span<std::uint8_t const> prk,
                                        std::span<std::uint8_t const> info) {
    static_assert(detail::is_digest_hmac<Hmac>::value,
                  "webcpp::hash: the key derivations take a Boost.Hash2 hmac whose result is a "
                  "digest, as hmac_sha2_256.");
    if constexpr (!detail::is_digest_hmac<Hmac>::value) {
        return error::backend_failure;  // unreachable: the static_assert above stops the build
    } else {
    // Hash2 1.92 declares no std::tuple_size for digest<N>; its size() is constexpr.
    constexpr std::size_t digest_size = typename Hmac::result_type().size();
    if (output.empty() || output.size() > 255 * digest_size) {
        return error::invalid_output_size;
    }
    if (prk.size() != digest_size) {
        return error::invalid_key_size;
    }
    if (info.size() > hkdf_max_info_size) {
        return error::input_too_long;
    }
    auto done = detail::hkdf_engine<Backend, Hmac>::expand(output, prk, info);
    if (!done) {
        detail::wipe(output);
    }
    return done;
    }
}
```

  and `backends/hash2.hpp`'s engine, RFC 5869's steps:

```cpp
template <class Hmac>
struct hkdf_engine<backends::hash2, Hmac> {
    using digest_type = typename Hmac::result_type;

    // RFC 5869, 2.2: PRK = HMAC-Hash(salt, IKM); an empty salt is HashLen zeros, which Hash2's
    // hmac keys as zeros of the block, the same key.
    static boost::system::result<digest_type> extract(std::span<std::uint8_t const> salt,
                                                      std::span<std::uint8_t const> ikm) noexcept {
        Hmac mac(salt.data(), salt.size());
        mac.update(ikm.data(), ikm.size());
        digest_type prk = mac.result();
        wipe_object(mac);
        return prk;
    }

    // RFC 5869, 2.3: T(0) = empty; T(i) = HMAC-Hash(PRK, T(i-1) | info | i); OKM = the first L
    // octets of T(1) | T(2) | ...
    static boost::system::result<void> expand(std::span<std::uint8_t> output,
                                              std::span<std::uint8_t const> prk,
                                              std::span<std::uint8_t const> info) noexcept {
        Hmac keyed(prk.data(), prk.size());  // not const: it is wiped at the end
        digest_type t{};
        std::size_t written = 0;
        for (unsigned counter = 1; written < output.size(); ++counter) {
            Hmac mac = keyed;
            if (counter > 1) {
                mac.update(t.data(), t.size());
            }
            mac.update(info.data(), info.size());
            const auto octet = static_cast<unsigned char>(counter);
            mac.update(&octet, 1);
            t = mac.result();
            wipe_object(mac);
            const std::size_t n = std::min(t.size(), output.size() - written);
            std::memcpy(output.data() + written, t.data(), n);
            written += n;
        }
        wipe_object(t);
        wipe_object(keyed);
        return {};
    }

    static boost::system::result<void> extract_and_expand(std::span<std::uint8_t> output,
                                                          std::span<std::uint8_t const> salt,
                                                          std::span<std::uint8_t const> ikm,
                                                          std::span<std::uint8_t const> info) noexcept {
        auto prk = extract(salt, ikm);
        auto done = expand(output, std::span<std::uint8_t const>(prk->data(), prk->size()), info);
        wipe_object(*prk);
        return done;
    }
};
```

  (`detail::wipe`'s object form takes the bytes of a trivially copyable object; a `static_assert`
  there checks that Hash2's hmac is one, which the pre-flight compiled for all sixteen aliases.)
  `default_kdf_backend` stays well-formed for any `Hmac` (it picks `backends::hash2` when the hmac
  is not a digest hmac), so that the `static_assert` is the only error a non-digest hmac gives.
  `kdf.hpp`'s `backend_has_v` is spec 3.2's table as variable template specialisations, and
  `default_kdf_backend` increment 1's rule (natively OpenSSL when linked and having it, else
  hash2; on WebAssembly the first linked of `detail::ranking`'s size order), which Task 14 turns
  into the profile. Task 9 fixes `detail/ranking.hpp`'s interface, which Task 14 reads and Task
  21's generator writes: `template <class... B> struct backend_list {};`; per target,
  `template <class Algorithm, class Hmac> struct ranking { using by_size = backend_list<...>;
  using by_throughput = backend_list<...>; };`; and `using portable_order = backend_list<hash2,
  xkcp, blake3, sodium, bearssl>;`. The provisional file encodes Task 9's rule: natively
  `by_throughput` is `openssl, hash2, sodium, bearssl` and `by_size` `hash2, sodium, bearssl,
  openssl`; on WebAssembly both are `hash2, sodium, bearssl, openssl`; so native defaults stay
  OpenSSL's, when linked, until Task 21 measures. Run the three tests on the four targets. Expected: PASS (the three
  OpenSSL-recorded cases pending).
- [ ] **Step 4: The refusal on a non-digest hmac.** `test/messages/kdf_not_digest/` calls
  `hkdf<boost::hash2::hmac<boost::hash2::fnv1a_64>>` and states spec 3.3's text. Run it.
  Expected: PASS. Headers-alone, boundaries, `hash.hpp`, changelog ("Added HKDF (RFC 5869) over
  every Boost.Hash2 hmac, on backends::hash2.").
- [ ] **Step 5: Review HKDF's engine against the departure's conditions** (spec 1.3.1): the RFC's
  names in comments, every intermediate wiped (the 150-line check of both engines is Task 10's,
  which has both). Commit (library)
  `"hash: the key derivations' layer, and HKDF on backends::hash2"`.

## Task 10: PBKDF2 on `backends::hash2`

**Files:**
- Create (library): `include/webcpp/hash/pbkdf2.hpp`, `test/support/rfc6070.hpp`,
  `test/pbkdf2_hash2_test.cpp`, `test/pbkdf2_rfc6070_long_test.cpp`, `test/pbkdf2_guards_test.cpp`.
- Modify (library): `include/webcpp/hash/backends/hash2.hpp`, `test/Jamfile`, `hash.hpp`,
  `meta/include-boundaries.json`, `doc/changelog.adoc`.

- [ ] **Step 1: Write the failing tests.** RFC 6070's cases 1, 2, 3, 5, 6 (SHA-1; case 1 is
  `"password"`, `"salt"`, 1 iteration, 20 bytes, `0c60c80f961f0e71f3a9b524af6012062fe037a6`; case 2
  the same at 2 iterations, `ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957`; case 3 at 4,096,
  `4b007901b765489abead49d926f721d065a429c1`) and RFC 7914 section 11's two SHA-256 cases (`"passwd"`,
  `"salt"`, 1 iteration, 64 bytes; `"Password"`, `"NaCl"`, 80,000 iterations, 64 bytes), copied
  from the RFCs' text; `pbkdf2_rfc6070_long_test.cpp` is case 4 (16,777,216 iterations), on every
  target, its time recorded in the ledger (under node with `webcpp::test::bound` at the measured
  time if past 120 s). Guards: output 0, iterations 0, and the output limit for `hmac_sha2_256`
  ((2^32 - 1) times 32): `detail::exceeds` tested directly, and natively only `pbkdf2` with
  `std::span<std::uint8_t>(ptr, limit + 1)` over a one-byte buffer, never dereferenced since the
  guard returns first (on wasm32 the limit exceeds `SIZE_MAX`, so the guard is unreachable there,
  which the test states). Run them. Expected: FAIL, no `pbkdf2.hpp`.
- [ ] **Step 2: Write `pbkdf2.hpp`** (guards of spec 2.9) and the engine:

```cpp
template <class Hmac>
struct pbkdf2_engine<backends::hash2, Hmac> {
    // RFC 8018, 5.2: T_i = F(P, S, c, i) = U_1 ^ U_2 ^ ... ^ U_c, with U_1 = PRF(P, S || INT(i))
    // and U_j = PRF(P, U_{j-1}); DK = the first dkLen octets of T_1 || T_2 || ...
    static boost::system::result<void> derive(std::span<std::uint8_t> output,
                                              std::span<std::uint8_t const> password,
                                              std::span<std::uint8_t const> salt,
                                              std::uint64_t iterations) noexcept {
        using digest_type = typename Hmac::result_type;
        Hmac keyed(password.data(), password.size());
        digest_type u{};
        digest_type t{};
        std::size_t written = 0;
        for (std::uint32_t block = 1; written < output.size(); ++block) {
            const std::array<unsigned char, 4> index{
                static_cast<unsigned char>(block >> 24), static_cast<unsigned char>(block >> 16),
                static_cast<unsigned char>(block >> 8), static_cast<unsigned char>(block)};
            Hmac mac = keyed;
            mac.update(salt.data(), salt.size());
            mac.update(index.data(), index.size());
            u = mac.result();
            t = u;
            for (std::uint64_t j = 1; j < iterations; ++j) {
                mac = keyed;
                mac.update(u.data(), u.size());
                u = mac.result();
                for (std::size_t k = 0; k < t.size(); ++k) {
                    t[k] ^= u[k];
                }
            }
            wipe_object(mac);
            const std::size_t n = std::min(t.size(), output.size() - written);
            std::memcpy(output.data() + written, t.data(), n);
            written += n;
        }
        wipe_object(u);
        wipe_object(t);
        wipe_object(keyed);
        return {};
    }
};
```

  Run on the four targets. Expected: PASS.
- [ ] **Step 3: The departure's size** (spec 1.3.1): HKDF's engine and PBKDF2's together under
  150 lines (`wc -l` of the two engines' bodies, recorded in the ledger).
- [ ] **Step 4: Commit** (library) `"hash: PBKDF2 (RFC 8018) on backends::hash2"`, with the
  changelog line and the lists.

## Task 11: HKDF on `backends::sodium`

**Files:**
- Create (library): `include/webcpp/hash/backends/sodium.hpp`, `test/hkdf_sodium_test.cpp`,
  `test/messages/sodium_not_linked/`, `test/messages/sodium_hmac/`, `test/messages/pbkdf2_on_sodium/`.
- Modify (library): `test/Jamfile`, `test/messages/Jamfile`, `hash.hpp`, the boundaries file,
  the changelog.

- [ ] **Step 1: Write the failing tests:** RFC 5869 A.1 to A.3 (SHA-256) and a SHA-512 case whose
  output is `backends::hash2`'s (agreement, recorded); the guards of Task 9 on this backend; the
  three messages (spec 5.7's libsodium text; spec 3.3's text for `hkdf<hmac_sha1_160,
  backends::sodium>`; `pbkdf2<hmac_sha2_512, backends::sodium>`). Declare the test with
  `<library>/third_party//sodium`. Expected: FAIL.
- [ ] **Step 2: Write `backends/sodium.hpp`:** the `WEBCPP_HASH_HAS_SODIUM` check, the canary and
  `detect_mismatch`, `struct backends::sodium {};`, the refusing specialisations for every hmac
  but `hmac_sha2_256` and `hmac_sha2_512` (a partial specialisation on `Hmac` with the text of
  spec 3.3), `pbkdf2_engine<backends::sodium, Hmac>` with its text, and:

```cpp
template <>
struct hkdf_engine<backends::sodium, hmac_sha2_256> {
    static boost::system::result<hmac_sha2_256::result_type> extract(
        std::span<std::uint8_t const> salt, std::span<std::uint8_t const> ikm) noexcept {
        touch_sodium_canary();
        if (sodium_init() < 0) {
            return error::backend_failure;
        }
        hmac_sha2_256::result_type prk;
        // libsodium declares the IKM nonnull; an empty span's data() may be null.
        if (crypto_kdf_hkdf_sha256_extract(prk.data(), salt.data(), salt.size(),
                                           detail::nonnull(ikm), ikm.size()) != 0) {
            return error::backend_failure;
        }
        return prk;
    }
    static boost::system::result<void> expand(std::span<std::uint8_t> output,
                                              std::span<std::uint8_t const> prk,
                                              std::span<std::uint8_t const> info) noexcept {
        touch_sodium_canary();
        if (sodium_init() < 0) {
            return error::backend_failure;
        }
        if (crypto_kdf_hkdf_sha256_expand(output.data(), output.size(),
                                          reinterpret_cast<const char*>(info.data()), info.size(),
                                          prk.data()) != 0) {
            return error::backend_failure;
        }
        return {};
    }
    // extract_and_expand: extract into a local PRK, expand, wipe the PRK.
};
```

  and the same for SHA-512. `detail::nonnull(span)` (in `detail/limits.hpp`) returns `span.data()`,
  or the address of a static constant byte when the span is empty, for every libsodium parameter
  declared `nonnull` (salt and info pass through it too); Tasks 15 and 16 use it. An empty IKM,
  salt and info are among the test's cases. Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: HKDF on libsodium (backends::sodium)"`.

## Task 12: OpenSSL: `/webcpp/hash//openssl` and `backends::openssl`

**Files:**
- Modify (library): `build.jam` (`openssl`, spec 5.6). Create: `src/openssl_on_emscripten.hpp`,
  `include/webcpp/hash/backends/openssl.hpp`, `test/hkdf_openssl_test.cpp`,
  `test/pbkdf2_openssl_test.cpp`, `test/openssl_error_queue_test.cpp`,
  `test/openssl_one_copy_test.cpp`, `test/openssl_failure_test.cpp`,
  `test/messages/openssl_not_linked/`, `test/messages/ripemd128_on_openssl/`; the fixture
  `tools/test/fixtures/hash_jamfiles/openssl_on_emscripten/` (superproject; Task 19's test
  builds it: this task writes the fixture and cannot run it yet, which the ledger says); and it
  modifies `test/Jamfile`, `hash.hpp`, the boundaries file, the changelog and
  `test/hkdf_hash2_test.cpp` (Task 9's three independent cases, checked on OpenSSL too).

- [ ] **Step 1: Write the failing tests:** RFC 5869 and RFC 6070/7914 through OpenSSL; every hmac
  of spec 3.2's OpenSSL column against `backends::hash2` (which also fills Task 9's three pending
  cases, natively, recorded into `hkdf_hash2_test.cpp`); info of 32,768 bytes succeeds and 32,769
  is refused before OpenSSL is called; a failure provoked with
  `WEBCPP_HASH_DETAIL_TEST_OPENSSL_DIGEST="NO-SUCH-DIGEST"` returns `backend_failure`, zeroes the
  output and leaves the thread's error queue as it found it; a program naming
  `/webcpp/hash//openssl` and `/webcpp/crypto//openssl` (when crypto is on `main`) or
  `/webcpp/asio//openssl` links one `libcrypto.a` (the link line printed at `-d+2`, counted).
  Targets: native, wasip2, wasip3. Expected: FAIL.
- [ ] **Step 2: Write the alias and the header.** The engines fetch, set and derive (spec 3.4):

```cpp
template <class Hmac>
boost::system::result<void> openssl_kdf(const char* kdf, std::span<std::uint8_t> output,
                                        std::span<OSSL_PARAM> params) noexcept {
    ERR_set_mark();
    EVP_KDF* algorithm = EVP_KDF_fetch(nullptr, kdf, nullptr);
    EVP_KDF_CTX* context = algorithm != nullptr ? EVP_KDF_CTX_new(algorithm) : nullptr;
    const bool derived = context != nullptr &&
                         EVP_KDF_derive(context, output.data(), output.size(), params.data()) == 1;
    const bool out_of_memory = !derived && ERR_GET_REASON(ERR_peek_last_error()) == ERR_R_MALLOC_FAILURE;
    EVP_KDF_CTX_free(context);
    EVP_KDF_free(algorithm);
    ERR_pop_to_mark();
    if (derived) {
        return {};
    }
    return out_of_memory ? boost::system::result<void>(boost::system::errc::make_error_code(
                               boost::system::errc::not_enough_memory))
                         : boost::system::result<void>(error::backend_failure);
}
```

  with the parameters `OSSL_PARAM_construct_utf8_string("digest", name)`,
  `..._octet_string("key"|"pass"|"salt"|"info", ...)`, `OSSL_PARAM_construct_int("mode", &mode)`
  (0, 1, 2), `OSSL_PARAM_construct_uint64("iter", &iterations)`,
  `OSSL_PARAM_construct_int("pkcs5", &one)`, `OSSL_PARAM_construct_end()`. Extract-only mode
  writes HashLen bytes into the digest. The names of spec 3.2 come from a trait
  `openssl_digest_name<Hmac>`, whose primary template refuses (`hmac_ripemd_128`'s text, and the
  general one for a hash OpenSSL lacks). Run on native, wasip2 and wasip3. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: OpenSSL's HKDF and PBKDF2 (backends::openssl), through /webcpp/hash//openssl"`.

## Task 13: HKDF on `backends::bearssl`

Builds on `/third_party//bearssl` of the base (`crypto/v0.1`).

**Files:**
- Modify (superproject): `third_party/Jamfile` (`bearssl` gains webcpp::hash's canary, when
  `libs/hash` is checked out, and `WEBCPP_HASH_HAS_BEARSSL=1`), `tools/test/third_party_test.py`
  (`test_bearssl_is_built_file_by_file`: 296 compiles, its scratch copy copies `hash` beside
  `config` and `crypto`; both canaries in the archive; a checkout without `libs/hash` still builds
  crypto's BearSSL with 295), `CODING-GUIDELINES.md` (chapter 10's `bearssl` row: "with
  webcpp::crypto's and webcpp::hash's canaries").
- Modify (library): `test/Jamfile`, `test/messages/Jamfile`, `include/webcpp/hash.hpp`, the
  boundaries file, `doc/changelog.adoc`.
- Create (library): `src/canary_bearssl.c`, `include/webcpp/hash/backends/bearssl.hpp`,
  `test/hkdf_bearssl_test.cpp`, `test/messages/{bearssl_not_linked,expand_on_bearssl,bearssl_hmac,pbkdf2_on_bearssl}/`.

- [ ] **Step 1: Write the failing tests:** RFC 5869's seven cases through `hkdf` (A.4 to A.7 over
  `br_sha1_vtable`); `hmac_md5_128`, `hmac_sha2_224`, `hmac_sha2_384`, `hmac_sha2_512` against
  `backends::hash2`; the guards; the four messages; `nm` of `/third_party//bearssl` lists both
  `webcpp_crypto_bearssl_7bea48e_*` and `webcpp_hash_bearssl_7bea48e_*`. Expected: FAIL.
- [ ] **Step 2: Write the canary** (webcpp::crypto's configuration logic, `HASH` names) and the
  target's additions: hash's canary is an `obj` of its own, `bearssl_hash_canary`, compiled with
  BearSSL's defines and `<include>$(HASH)/include`, added to `bearssl`'s sources by a
  `<conditional>` only when `$(HASH)/src/canary_bearssl.c` exists, so that a checkout without
  `libs/hash` builds crypto's BearSSL as before; `WEBCPP_HASH_HAS_BEARSSL=1` joins the usage
  requirements only then. Then `struct backends::bearssl {};`, the refusing
  `pbkdf2_engine<backends::bearssl, Hmac>` with spec 3.3's BearSSL text (so the
  `pbkdf2_on_bearssl` message is that text, not the primary's), and the engine:

```cpp
template <class Hmac>
struct hkdf_engine<backends::bearssl, Hmac> {
    static boost::system::result<void> extract_and_expand(std::span<std::uint8_t> output,
                                                          std::span<std::uint8_t const> salt,
                                                          std::span<std::uint8_t const> ikm,
                                                          std::span<std::uint8_t const> info) noexcept {
        touch_bearssl_canary();
        br_hkdf_context context;
        br_hkdf_init(&context, bearssl_hash_class<Hmac>::vtable(), salt.data(), salt.size());
        br_hkdf_inject(&context, ikm.data(), ikm.size());
        br_hkdf_flip(&context);
        const std::size_t produced =
            br_hkdf_produce(&context, info.data(), info.size(), output.data(), output.size());
        wipe_object(context);
        return produced == output.size() ? boost::system::result<void>()
                                         : boost::system::result<void>(error::backend_failure);
    }
    // extract and expand: a static_assert with spec 3.3's text, reached only when called.
};
```

  `bearssl_hash_class<Hmac>` maps the six hmacs to `&br_md5_vtable` ... `&br_sha512_vtable`; its
  primary template refuses with spec 3.3's BearSSL text. Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (superproject) `"third_party: BearSSL carries webcpp::hash's canary"`;
  (library) `"hash: BearSSL's HKDF (backends::bearssl)"`.

## Task 14: The profile, the overrides and the default backend

**Files:**
- Modify (library): `include/webcpp/hash/config.hpp` (the profile and override defines, their
  refusals, `detect_mismatch`), `include/webcpp/hash/kdf.hpp` (`default_backend`,
  `default_kdf_backend`). Create: `test/default_backend_test.cpp` (and one test per profile:
  `default_fastest_test.cpp`, `default_smallest_test.cpp`, `default_portable_test.cpp`,
  `default_named_test.cpp`, `default_override_test.cpp`, `default_umbrella_test.cpp`),
  `test/capabilities_test.cpp`, `test/overrides_list_test.cpp`, `doc/capabilities.json`,
  `test/messages/{two_profiles,two_umbrella_profiles,two_overrides,override_cannot_serve}/`
  (OpenSSL's profile on Emscripten is a fixture of Task 19's test, not a message test here).

**Interfaces:**
- Produces: spec 4.1, whole; `doc/capabilities.json` (spec 3.5, increment 1's rows).

- [ ] **Step 1: Write the failing tests.** `default_backend_test.cpp`, built once per target with
  every library linked: `auto` gives natively the throughput order's first linked backend for
  each KDF and hmac, and on WebAssembly the size order's; `default_kdf_backend<hmac_sha2_256,
  kdfs::pbkdf2>` is never `sodium` or `bearssl` on any target or profile (Review Focus 4);
  `hmac_ripemd_128` always `hash2`. Each profile's test, and the override test
  (`WEBCPP_HASH_DEFAULT_BACKEND_HKDF_SODIUM` makes `hkdf<hmac_sha2_256>` libsodium's while PBKDF2 keeps
  the profile's). `default_umbrella_test.cpp`: webcpp's umbrella `WEBCPP_PROFILE_SMALLEST` alone
  follows the size order; with `WEBCPP_HASH_PROFILE_PORTABLE` too the library's profile wins; with
  `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_OPENSSL` as well the override wins for HKDF (spec 4.1's
  precedence: override, library profile, umbrella profile, measured default).
  `capabilities_test.cpp` reads `doc/capabilities.json` (Boost.JSON) and compares
  each cell with `backend_has_v`. The messages: two library profiles and two umbrella profiles (each text naming both defines,
  as "webcpp::hash: WEBCPP_HASH_PROFILE_FASTEST and WEBCPP_HASH_PROFILE_SMALLEST are both defined.
  Keep one."), two overrides of HKDF, an override that cannot serve. Expected: FAIL.
- [ ] **Step 2: Write the selection** as a type computation over `detail::ranking` (Task 9's
  provisional file, Task 21's measured one):

```cpp
namespace webcpp::hash::detail {

template <class Backend> inline constexpr bool linked_v = false;
template <> inline constexpr bool linked_v<backends::hash2> = true;
#ifdef WEBCPP_HASH_HAS_SODIUM
template <> inline constexpr bool linked_v<backends::sodium> = true;
#endif
// ... bearssl, openssl, and from their increments xkcp, blake3, botan

// The first backend of List that is linked and has Algorithm over Hmac.
template <class Algorithm, class Hmac, class List> struct first_serving;
template <class Algorithm, class Hmac, class Head, class... Tail>
struct first_serving<Algorithm, Hmac, backend_list<Head, Tail...>>
    : std::conditional_t<linked_v<Head> && backend_has_v<Head, Algorithm, Hmac>,
                         std::type_identity<Head>,
                         first_serving<Algorithm, Hmac, backend_list<Tail...>>> {};

template <class Algorithm, class Hmac> struct profile_choice;  // by the profile define
template <class Algorithm, class Hmac> struct override_choice;  // void without an override

}  // namespace webcpp::hash::detail
```

  `profile_choice` takes the library's profile when one is defined, else webcpp's umbrella
  (`WEBCPP_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE`), else `auto`, and picks
  `ranking<Algorithm, Hmac>::by_throughput` (fastest),
  `::by_size` (smallest), `portable_order` (hash2, xkcp, blake3, sodium, bearssl), or the named
  backend when it serves and `auto`'s choice otherwise; `auto` is fastest natively and smallest on
  WebAssembly. An override macro `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>` specialises
  `override_choice` in `config.hpp`'s expansion: a list of the algorithms of the capability
  table, one `#ifdef` each, written by hand in this task (`tools/hash/measure.py` comes with Task
  21), with `overrides_list_test.cpp` checking that every algorithm of `doc/capabilities.json` has
  its lines and no other line exists; Task 21 adds `measure.py --overrides`, which regenerates the
  list, and the test keeps it honest either way. Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: one profile for every default backend, and per-algorithm overrides"`.

## Task 15: Argon2id on libsodium

**Files:**
- Create (library): `include/webcpp/hash/argon2.hpp`, `test/argon2id_test.cpp`,
  `test/argon2_rfc9106_test.cpp`, `test/messages/argon2id_on_hash2/`.
- Modify (library): `include/webcpp/hash/backends/sodium.hpp` (the engine and the presets'
  `static_assert`s), `test/Jamfile`, `hash.hpp`, the boundaries file, the changelog.

- [ ] **Step 1: Write the failing tests.** `argon2_rfc9106_test.cpp`, with
  `<library>/third_party//sodium-test-config`, includes libsodium's private `argon2.h` inside
  `extern "C" { }` (the header has no guard of its own, so `argon2_ctx` would otherwise be looked
  up with C++ linkage), and runs RFC
  9106 section 5.3's Argon2id case through `argon2_ctx(&context, Argon2_id)`: t 3, m 32 KiB, p 4
  (lanes and threads), password 32 bytes of `0x01`, salt 16 of `0x02`, secret 8 of `0x03`,
  associated data 12 of `0x04`, tag 32 bytes,
  `0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659` (copied from the RFC's text
  and compared with it before the commit). `argon2id_test.cpp`: three cases of libsodium's
  `test/default/pwhash_argon2id.c` table at the pin that its `.exp` shows succeeding, with their
  outputs (copied); the table's memory limits are not multiples of 1,024 and its salts are 32
  bytes, so each case passes `memory` rounded down to a multiple of 1,024 (libsodium itself uses
  `memlimit / 1024`, the same result) and the salt's first 16 bytes (the function reads 16), as
  the test's comment says; `argon2id` against `argon2_ctx` with one lane on 20 random cases
  (memory 8 to 256 KiB, passes 1 to 3); every guard of spec 2.10; the presets equal libsodium's
  macros. Expected: FAIL.
- [ ] **Step 2: Write `argon2.hpp`** (guards) and the engine (`errno = 0;` then
  `crypto_pwhash_argon2id(output, output.size(), detail::nonnull(password), password.size(), salt,
  passes, memory, crypto_pwhash_ALG_ARGON2ID13)`; `-1` with `errno == ENOMEM` is
  `errc::not_enough_memory`, any other `-1` `backend_failure`: libsodium returns -1 without
  setting `errno` when its core fails, so a stale value must not be read).
  Run on the four targets; the interactive preset (64 MiB) runs in its own program, with its
  time per target recorded. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: Argon2id over libsodium"`.

## Task 16: scrypt on libsodium

**Files:**
- Create (library): `include/webcpp/hash/scrypt.hpp`, `test/scrypt_test.cpp`,
  `test/scrypt_rfc7914_large_test.cpp`. Modify: `backends/sodium.hpp`, lists, changelog.

- [ ] **Step 1: Write the failing tests:** RFC 7914 section 12's first three cases (case 1:
  empty password and salt, N 16, r 1, p 1, 64 bytes,
  `77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906`;
  cases 2 and 3 copied from the RFC); the fourth (N 1,048,576, r 8, p 1, 1 GiB) in
  `scrypt_rfc7914_large_test.cpp`; the guards of spec 2.11 (N 0, 1, 3, 2^(16 r); r 0; p 0; r p =
  2^30; 128 r (N + p) past `size_t` on wasm32). Expected: FAIL.
- [ ] **Step 2: Write `scrypt.hpp` and the engine** (`errno = 0;` then
  `crypto_pwhash_scryptsalsa208sha256_ll(detail::nonnull(password), password.size(),
  detail::nonnull(salt), salt.size(), n, r, p, output.data(), output.size())`: declared `nonnull`
  on every pointer and `warn_unused_result`, and RFC 7914's first case has an empty password and
  salt). Run on the four targets. Run the large case
  natively and on each WebAssembly target; declare its program on the targets where it passed
  (each WebAssembly target's result, pass or the error and memory, recorded in the ledger and in
  the Jamfile's comment). Expected: PASS where declared.
- [ ] **Step 3: Commit** (library) `"hash: scrypt over libsodium"`.

## Task 17: Agreement between backends

**Files:**
- Create (library): `test/support/random_cases.hpp`, `test/agreement_hkdf_test.cpp`,
  `test/agreement_pbkdf2_test.cpp`.

- [ ] **Step 1: Write the tests** (spec 7.2): a seeded `std::mt19937` (seed printed on failure),
  1,000 cases each, the empty inputs always among them; for each hmac every pair of linked
  backends that has it. Declarations: native with every library; wasip2 and wasip3 with sodium,
  bearssl and openssl; Emscripten with sodium and bearssl. Run on each target. Expected: PASS. A
  difference is a bug of webcpp's code until shown otherwise; never a tolerance.
- [ ] **Step 2: Commit** (library) `"hash: every backend agrees with every other, on every target"`.

## Task 18: WebCrypto as an outside oracle

**Files:**
- Create (library): `test/webcrypto_oracle_test.cpp`.

- [ ] **Step 1: Write the test** (spec 7.4): `EM_ASYNC_JS` functions calling
  `crypto.subtle.digest`, `importKey` and `sign` (HMAC), `deriveBits` (HKDF, PBKDF2) on node's
  `globalThis.crypto.subtle`, each returning its bytes through a heap buffer; 200 random cases per
  function and hash (SHA-1, SHA-256, SHA-384, SHA-512), compared with every backend linked on
  Emscripten (`hash2`, `sodium` for its two hmacs, `bearssl`) and Hash2's digests. Declared
  `emscripten` only, the default JSPI variant. Run the Emscripten lane. Expected: PASS.
- [ ] **Step 2: Commit** (library) `"hash: WebCrypto under node as an outside oracle"`.

## Task 19: The user's Jamfiles, the canaries at link time, one program

**Files:**
- Create (superproject): `tools/test/hash_jamfiles_test.py`,
  `tools/test/fixtures/hash_jamfiles/<case>/` (one per line of spec 5.1 and one per build stop of
  spec 5.7 that b2 raises).
- Create (library): `test/unused/` (`webcpp.same-program` per C library), `test/link/` (a
  `webcpp.link-fail` per canary: the `HAS` define given by hand, no library), `test/msvc/<case>/`.

- [ ] **Step 1: Write `hash_jamfiles_test.py`** as webcpp::crypto's `crypto_jamfiles_test.py` (the
  outcome format of CODING-GUIDELINES.md chapter 8), and its fixtures; each outcome states the
  program's output or b2's message exactly. Run it. Expected: FAIL until the fixtures' libraries
  exist, then PASS; every case on every target it names.
- [ ] **Step 2: The canaries.** For XKCP, BLAKE3, libsodium and BearSSL: a program that defines
  the `HAS` macro, calls a function and links no library fails to link naming the canary (a
  `webcpp.link-fail` whose source states the symbol); a canary of another commit (the
  `_CANARY_COMMIT` seam) fails the same way. On MSVC the same cases under `test/msvc/`.
- [ ] **Step 3: One program.** `webcpp.same-program` for each of `xkcp`, `blake3`, `sodium`,
  `bearssl`, on every target (spec 7.8). Run. Expected: PASS.
- [ ] **Step 4: Commit** (superproject) `"tools: the user's Jamfiles of webcpp::hash as fixtures"`;
  (library) `"hash: the canaries at link time, and a library named and not called adds no byte"`.

## Task 20: Allocation

**Files:**
- Create (library): `test/support/counting.hpp` (webcpp::crypto's), `test/allocation_test.cpp`,
  `test/allocation_openssl_test.cpp`.

- [ ] **Step 1: Write the tests** (spec 7.7): the counter is zero after every call of `keccak_256`,
  `blake3`, every KDF on hash2, sodium (HKDF) and bearssl, on every target, with a control that
  allocates once and moves the counter; Argon2id and scrypt are called and documented as
  allocating inside libsodium (the test asserts only that `operator new` is not called);
  `allocation_openssl_test.cpp` counts OpenSSL's allocations per call through
  `CRYPTO_set_mem_functions` natively and prints them (reported in `doc/allocation.adoc`, not
  bounded). Run. Expected: PASS.
- [ ] **Step 2: Commit** (library) `"hash: no allocation in webcpp's code, and the C libraries' measured"`.

## Task 21: The measured table

**Files:**
- Create (superproject): `tools/hash/measure.py`, `tools/test/hash_measure_test.py`.
- Create (library): `test/measure/Jamfile`, `test/measure/<algorithm>_<backend>.cpp` (one per
  cell of `doc/capabilities.json` and target), `doc/measurements.json`, `doc/measurements.adoc`.
- Modify (library): `include/webcpp/hash/detail/ranking.hpp` (generated).

- [ ] **Step 1: Write the failing tool test** `hash_measure_test.py`: given a fixture of
  measurements, `measure.py --write` renders the JSON, the AsciiDoc table and `ranking.hpp`
  deterministically; `--check` fails naming `python3 tools/hash/measure.py --write` when the size
  order of one row changes, and passes when only a throughput median moves by under 10 percent.
  Expected: FAIL.
- [ ] **Step 2: Write `measure.py`** and the programs (spec 4.2): sizes on the four targets at
  `variant=release optimization=space` (the native executable stripped with the toolchain's
  `strip`; the `.wasm` file), throughput natively and on each WebAssembly target where the program
  ends within the lane's bound, at `optimization=speed`, median of five. The own lane `measure`
  in `test/measure/Jamfile` names its four targets, builds every program and runs
  `measure.py --check`. Run `python3 tools/hash/measure.py --write`, then the lane. Expected: the
  lane passes; `ranking.hpp` replaces the provisional order; Task 14's tests pass with it on
  every target.
- [ ] **Step 3: Commit** (superproject) `"tools: webcpp::hash's measured table"`; (library)
  `"hash: the measured table, and the defaults it chooses"`.

## Task 22: PBKDF2-HMAC-SHA-512 at 1,000,000 iterations

**Files:**
- Create (library): `bench/Jamfile`, `bench/pbkdf2_sha512.cpp`, `doc/performance.adoc`.

- [ ] **Step 1: Write the benchmark** (spec 4.3): three variants in one program, chosen by
  argument (`hash2`, `openssl`, `sodium-hmac`), each printing the median of three runs in
  milliseconds; its output checked against `backends::hash2`'s at 1,000 iterations first, so that a
  wrong loop cannot be fast. Build at `variant=release optimization=speed` on the four targets.
- [ ] **Step 2: Run** natively and on wasip2, wasip3 and Emscripten (OpenSSL's variant on its three
  targets), and write `doc/performance.adoc` with the machine, compiler, target, date and the
  three times. Expected: a table; a case where hash2 beats OpenSSL natively is reported to the
  owner in the ledger.
- [ ] **Step 3: Commit** (library) `"hash: PBKDF2-HMAC-SHA-512's throughput on every backend"`.

## Task 23: The documentation, the examples and the upstream text

**Files:**
- Create (library): `doc/` parts (one per header and per algorithm, in noble's README's manner:
  what it is, an example, its pitfalls), `doc/backends.adoc` (the capability table rendered from
  `doc/capabilities.json` by `tools/hash/measure.py --capabilities`), `doc/profile.adoc`,
  `doc/departures.adoc`, `doc/use-cases/{p2p,cryptocurrencies,privacy}.adoc`,
  `doc/upstream.adoc`, `NOTICE.md`, `example/*.cpp` (each section's example) and its Jamfile.

- [ ] **Step 1: The examples first:** each section's example is a program of `example/`, built and
  run on every target by the lanes, printing a known value (the page includes it by tag).
- [ ] **Step 2: The pages** (spec 8). Where a section adapts libsodium's documentation
  (`jedisct1/libsodium-doc` at `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`), it says so, and
  `NOTICE.md` keeps libsodium-doc's copyright notice and the ISC licence text verbatim. No text
  from Wikipedia or noble's README is copied; their facts are cited. The use-case chapters use
  webcpp's own code (Nostr event ids and NIP-44's HKDF, BIP39 seeds, hash160, Ethereum addresses
  and EIP-55, Argon2id password storage, BLAKE2b subkeys and personalisation, SipHash for tables
  keyed by peers' input), framed as raising the security of what a reader builds; a public
  application built on noble is named only where it is public and only as context.
- [ ] **Step 3: The upstream text** `doc/upstream.adoc`: the proposed public `keccak_224`,
  `keccak_256`, `keccak_384`, `keccak_512` of Boost.Hash2 (typedefs of
  `detail::keccak_base<0x01, 2 * D, D>`, their tests with the published digests), ready for the
  owner to open (G5).
- [ ] **Step 4: Build the documentation** (`$B2 libs/hash/doc`), the doc-comment check and the
  lint. Expected: clean. Commit (library) `"hash: the documentation, its examples and the Boost.Hash2 proposal's text"`.

## Task 24: Increment 1 whole

- [ ] **Step 1: The rulebook and the lint.** CODING-GUIDELINES.md chapter 10's rows (Tasks 4, 6, 8,
  13) are in; chapter 17's entry describes increment 1 as built; chapter 5's rule 2 records its
  three exceptions: BLAKE3's intrinsics files on their instruction sets alone, XKCP built from the
  one file of Keccak-p[1600] (and the high-level files increment 3 lists), libsodium without its
  `.S` files. The lint's rule 19, "canary pins" (`tools/lint/lint.sh`), reads hash's
  `detail/canary.h` beside crypto's, mapping `sodium` to `libsodium` as it maps `ctaes` to
  `bitcoin`, and its case in `tools/test/` gains a wrong hash pin that must fail.
- [ ] **Step 2: Green, from scratch,** every lane of the Global Constraints: native, wasip2, wasip3,
  emscripten, gcc-15, the own lanes `c-suites` and `measure` on their targets,
  `python3 tools/test/hash_jamfiles_test.py`, `python3 tools/test/blake3_c_suite.py`,
  `python3 tools/test/third_party_test.py`, the lint (`lint: clean`). Expected: every one passes;
  the ledger records each command and its last line.
- [ ] **Step 2b: Review** of the whole increment against the spec and the Review Focus, by a
  reviewer who did not write it; the departures' conditions (spec 1.3) checked line by line.
- [ ] **Step 3: The gates.** The owner creates `webcpporg/hash` (G1) and adds `libs/hash` (G2);
  the library's pull request, then the superproject's, are opened and merged by the owner (G4);
  MSVC and Apple clang run in the CI then. The proposal's status becomes `implementing`, and
  `implemented` with its release and commit when CONTRIBUTING.md's conditions hold.

---

# Increments 2 to 7: every backend's whole surface, and the browser

Each increment's first task reads its backend at the pin and corrects its column of
`doc/capabilities.json` (and spec 3.5 by a change of this proposal) before any code: a cell is
never assumed. Each function follows increment 1's pattern: the official vectors first and seen
failing, the engine, every target the backend has, exceptions on and off, agreement with every
other backend that has the function (Task 17's tests gain each pair), the WebCrypto oracle where
WebCrypto has it (Task 18's test gains it), the measured table regenerated (Task 21's lane), the
capability table and the documentation's section (noble's manner: what it is, an example, its
pitfalls), the changelog line.

## Task 25: The families: tags, `digest`, `hasher`, `mac`, `argon2`

Stream `sodium-2`, first; every later increment builds on it.

**Files:**
- Create (library): `include/webcpp/hash/digest.hpp` (`digests::` tags, `digest_options`,
  `digest`, `hasher`), `include/webcpp/hash/mac.hpp` (`macs::` tags, `mac_options`, `mac`),
  `include/webcpp/hash/argon2.hpp` gains `argon2_parameters` and `argon2<Variant, Backend>`;
  `include/webcpp/hash/detail/engines.hpp` gains `digest_engine<Backend, Digest>`,
  `mac_engine<Backend, Mac>`, `argon2_engine<Backend, Variant>`, each primary template refusing
  with the text generated from `doc/capabilities.json` (the backends of its row);
  `test/families_test.cpp`, `test/hasher_state_test.cpp`.
- Modify (library): `error.hpp` (value 9, `invalid_state`, "the hasher was finished or moved
  from"), `kdf.hpp` (`default_backend` over every tag), `doc/capabilities.json`,
  `tools/hash/measure.py --refusals` (superproject) to generate the refusal texts.

**Interfaces:**
- Produces: spec 2.15; `hasher` is move-only, wipes its context in its destructor and on move,
  and answers `invalid_state` after `finish` or once moved from.

- [ ] **Step 1: Write the failing tests** with a mock backend (`test/support/mock_backend.hpp`,
  webcpp::crypto's pattern): each family's guards (an output of the wrong size for a fixed-size
  digest is `invalid_output_size`; an XOF takes any size from 1), `hasher`'s state machine, the
  refusal text of a tag no backend serves, `unsupported_by_backend` from an engine that refuses a
  parameter. Expected: FAIL.
- [ ] **Step 2: Write the families** and wire `hash2`'s existing Hash2 algorithms as
  `digest_engine<backends::hash2, digests::sha2_256>` (and the other Hash2 algorithms), so that the
  families serve increment 1's algorithms too; `keccak_256` as
  `digest_engine<backends::xkcp, digests::keccak_256>`; `blake3` as
  `digest_engine<backends::blake3, digests::blake3>` (an XOF). Run on the four targets.
  Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: the families digest, hasher, mac and argon2 over every backend"`.

## Task 26: libsodium's BLAKE2b, with key, salt, personalisation and length

**Files:**
- Create (superproject): the shallow submodule `third_party/libs/blake2` (`BLAKE2/BLAKE2` at
  `ed1974ea83433eba7b2d95c5dcd9ac33cb847913`, 2023-02-12, CC0, test data:
  `testvectors/blake2b-kat.txt`), its target `/third_party//blake2_vectors`, chapter 10's row.
- Create (library): `test/blake2b_sodium_test.cpp`; modify `backends/sodium.hpp`.

- [ ] **Step 1: Write the failing test:** every keyed case of `blake2b-kat.txt` (64-byte outputs,
  key `00 01 ... 3f`, inputs of 0 to 255 bytes) through `digest<digests::blake2b,
  backends::sodium>` with `digest_options<digests::blake2b>{.key = ...}`; unkeyed outputs of 16,
  32 and 64 bytes against Hash2's `blake2b_512` where the length is 64; salt and personalisation
  against libsodium's `generichash3` test's values. Expected: FAIL.
- [ ] **Step 2: Write the engine** over `crypto_generichash_blake2b_salt_personal` (one shot) and
  `crypto_generichash_blake2b_init_salt_personal`, `_update`, `_final` (the hasher); the guards:
  key 0 or 16 to 64 bytes, salt and personalisation 0 or 16 bytes, output 16 to 64 bytes
  (`invalid_output_size`, `invalid_key_size`, `invalid_parameters`). Run on the four targets.
  Expected: PASS.
- [ ] **Step 3: Commit** (superproject) `"third_party: BLAKE2's test vectors"`; (library)
  `"hash: BLAKE2b with key, salt and personalisation on libsodium"`.

## Task 27: libsodium's SipHash and BLAKE2b subkeys

**Files:**
- Create (library): `test/siphash_sodium_test.cpp`, `test/kdf_blake2b_sodium_test.cpp`,
  `include/webcpp/hash/subkey.hpp` (`derive_subkey<Backend>(output, master_key, subkey_id,
  context)`, libsodium's `crypto_kdf`: an 8-byte context, a 32-byte master key, outputs of 16 to
  64 bytes).

- [ ] **Step 1: Write the failing tests:** SipHash-2-4 (64 bits) against the SipHash paper's
  reference vectors (the 64 outputs of key `00..0f` over inputs `00`, `00 01`, ...), as libsodium's
  `shorthash` test and Hash2's `siphash_64` both reproduce them (agreement with Hash2); SipHash-2-4x
  (128 bits) against libsodium's `siphashx24.exp`; `derive_subkey` against libsodium's `kdf.exp`.
  Expected: FAIL.
- [ ] **Step 2: Write the engines** (`crypto_shorthash_siphash24`, `crypto_shorthash_siphashx24`,
  `crypto_kdf_blake2b_derive_from_key`). Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: SipHash-2-4, SipHash-2-4x and BLAKE2b subkeys on libsodium"`.

## Task 28: libsodium's XOFs, SHA-2, SHA-3 and HMACs

**Files:**
- Create (library): `test/xof_sodium_test.cpp`, `test/digests_sodium_test.cpp`.

- [ ] **Step 1: Write the failing tests:** SHAKE128/256 and TurboSHAKE128/256 (`crypto_xof_*`)
  against RFC 9861's TurboSHAKE vectors and FIPS 202's SHAKE examples, and against Hash2's
  `shake_128`/`shake_256` and XKCP (when increment 3 merges, the agreement test gains the pair);
  SHA-256, SHA-512, SHA3-256, SHA3-512 and HMAC-SHA-256/512/512-256 against Hash2 on random inputs
  and RFC 4231. Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: libsodium's XOFs, SHA-2, SHA-3 and HMACs as digests and MACs"`.

## Task 29: Argon2i, and `argon2<Variant, backends::sodium>`

- [ ] **Step 1: Write the failing tests:** libsodium's `pwhash_argon2i` cases (three, copied with
  their outputs); `argon2<kdfs::argon2i>` and `<kdfs::argon2id>` on `backends::sodium` with `lanes
  = 1` and no secret or associated data equal `argon2_ctx`'s; `lanes = 4`, a secret or associated
  data return `unsupported_by_backend`; `kdfs::argon2d` on sodium refuses at compile time with the
  text naming OpenSSL and Botan. Expected: FAIL.
- [ ] **Step 2: Write the engines** (`crypto_pwhash_argon2i`, `crypto_pwhash` with
  `crypto_pwhash_ALG_ARGON2I13`). Run on the four targets. Expected: PASS.
- [ ] **Step 3: Increment 2 whole:** Task 47 for increment 2. Commit (library)
  `"hash: Argon2i on libsodium, and increment 2 whole"`.

## Task 30: XKCP's high-level files

Stream `xkcp-3`.

**Files:**
- Modify (superproject): `third_party/Jamfile` (`xkcp` gains `lib/high/Keccak/KeccakSponge.c`,
  `lib/high/Keccak/SP800-185/SP800-185.c`, `lib/high/KangarooTwelve/KangarooTwelve.c`,
  `lib/high/TurboSHAKE/TurboSHAKE.c` and what they include at the pin), `tools/test/third_party_test.py`.
- Modify (library): `src/xkcp/config.h` (the `XKCP_has_*` these files need, and nothing of the
  parallel permutations: KangarooTwelve and ParallelHash then use XKCP's own fallback,
  `lib/low/common/PlSnP-Fallback.inc`), `src/xkcp_adapter.c` (functions for each high-level
  interface, webcpp's names, so that no XKCP header reaches a program), `src/canary_xkcp.c`
  (configuration `generic64high`).

- [ ] **Step 1: Read the files at the pin** and list in the ledger what each includes and which
  `XKCP_has_*` it reads. Write the failing case: the archive holds the new objects and the canary
  `webcpp_hash_xkcp_d20c8fc_generic64high__...`. Expected: FAIL.
- [ ] **Step 2: Write the additions;** extend the c-suite's `xkcp_unit_tests` with
  `--KeccakSponge --SP800-185 --KangarooTwelve` (and TurboSHAKE's option at the pin) and their test
  files. Run the case and the lane on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (superproject) `"third_party: XKCP's sponge, SP 800-185, KangarooTwelve and TurboSHAKE"`;
  (library) `"hash: XKCP's adapter for its high-level functions"`.

## Task 31: Keccak-224 to -512, SHA-3 and SHAKE on XKCP

- [ ] **Step 1: Write the failing tests:** Keccak-224, -256, -384, -512 of the empty message and
  "abc" (the 256 values of Task 5; the others agreeing with Hash2's `keccak_base<0x01, ...>` on
  random inputs) through `digest<digests::keccak_*, backends::xkcp>`; SHA3 and SHAKE against
  XKCP's `ShortMsgKAT_SHA3-*.txt` and `ShortMsgKAT_SHAKE*.txt` (test data through a
  `/third_party//xkcp_vectors` target, the form of `blake3_vectors`) and against Hash2.
  Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: the Keccak family's hashes on XKCP"`.

## Task 32: cSHAKE, KMAC, TupleHash and ParallelHash on XKCP

- [ ] **Step 1: Write the failing tests:** NIST's SP 800-185 samples (cSHAKE, KMAC, KMACXOF,
  TupleHash, ParallelHash: each sample's inputs and outputs copied from NIST's published example
  files, cited by URL in the test, checked twice); `digest_options` of cSHAKE (function name N,
  customisation S), `mac_options` of KMAC (customisation, XOF form). Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run on the four targets. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: SP 800-185 on XKCP: cSHAKE, KMAC, TupleHash, ParallelHash"`.

## Task 33: KangarooTwelve and TurboSHAKE on XKCP

- [ ] **Step 1: Write the failing tests:** RFC 9861's vectors for KT128, KT256 (with
  customisation strings) and TurboSHAKE128/256 (with domain bytes), and agreement with libsodium's
  TurboSHAKE. Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run on the four targets. Expected: PASS.
- [ ] **Step 3: Increment 3 whole:** Task 47. Commit (library)
  `"hash: KangarooTwelve and TurboSHAKE on XKCP, and increment 3 whole"`.

## Task 34: OpenSSL's digests

Stream `openssl-4`; native, wasip2 and wasip3.

- [ ] **Step 1: List the default provider's digests** at OpenSSL 3.5.9
  (`providers/implementations/include/prov/names.h`, `PROV_NAMES_*`: MD5, SHA1, the SHA-2 six,
  SHA3-224 to -512, SHAKE-128/256, KECCAK-224 to -512, KECCAK-KMAC-128/256, SM3, BLAKE2S-256,
  BLAKE2B-512, RIPEMD-160; WHIRLPOOL only in the legacy provider, which webcpp does not load:
  recorded as absent) and correct `doc/capabilities.json`'s column.
- [ ] **Step 2: Write the failing tests:** each digest through `digest` and `hasher` against
  Hash2, XKCP and libsodium where they have it; KECCAK-256 against Task 5's values; SM3 against
  GB/T 32905's examples ("abc" and the 64-byte message). Expected: FAIL.
- [ ] **Step 3: Write the engines** over `EVP_MD_fetch`, `EVP_DigestInit_ex2`, `EVP_DigestUpdate`,
  `EVP_DigestFinal_ex` (`EVP_DigestFinalXOF` for SHAKE); the hasher owns an `EVP_MD_CTX`, freed in
  its destructor. Run. Expected: PASS.
- [ ] **Step 4: Commit** (library) `"hash: OpenSSL's digests, KECCAK-256 and SM3 included"`.

## Task 35: OpenSSL's MACs

- [ ] **Step 1: Write the failing tests:** KMAC128/256 (NIST's samples, and agreement with XKCP),
  BLAKE2BMAC and BLAKE2SMAC against the BLAKE2 keyed KAT, HMAC over every digest against Hash2.
  Expected: FAIL.
- [ ] **Step 2: Write the engines** over `EVP_MAC_fetch` and `EVP_MAC_init`/`update`/`final`. Run.
  Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: OpenSSL's MACs: KMAC, BLAKE2, HMAC"`.

## Task 36: OpenSSL's scrypt and Argon2 with every parameter

- [ ] **Step 1: Write the failing tests:** RFC 7914 section 12 through `scrypt<backends::openssl>`;
  RFC 9106 section 5's three cases (Argon2d, Argon2i, Argon2id: t 3, m 32 KiB, p 4, with secret
  and associated data) through `argon2<kdfs::argon2*, backends::openssl>`, and Argon2id's against
  Task 15's `argon2_ctx` result; agreement with libsodium at one lane. Expected: FAIL.
- [ ] **Step 2: Write the engines** over `EVP_KDF_fetch("SCRYPT"|"ARGON2D"|"ARGON2I"|"ARGON2ID")`,
  with `threads = 1` (the lanes are computed in turn; OpenSSL is built `no-threads` on the ordinary
  lanes) and the memory in KiB. Run. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: scrypt and Argon2 with every RFC 9106 parameter on OpenSSL"`.

## Task 37: OpenSSL's KBKDF, SSKDF and X9.63 KDF

- [ ] **Step 1: Read OpenSSL's own vectors** of the three at the pin
  (`test/recipes/30-test_evp_data/evpkdf_kbkdf_counter.txt`, `evpkdf_ss.txt`, `evpkdf_x963.txt`,
  or their names at 3.5.9) and copy five cases of each, with their source line, into the tests.
  Specify the interface in `include/webcpp/hash/kdf_standards.hpp` (`kbkdf<Hmac, Backend>` in
  counter mode with label and context, `sskdf<Digest, Backend>` hash form, `x963kdf<Digest,
  Backend>`), with the guards each standard states. Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: SP 800-108, SP 800-56C and X9.63 key derivation on OpenSSL"`.

## Task 38: Increment 4 whole

- [ ] Task 47 for increment 4: the agreement tests gain every OpenSSL pair; the measured table
  gains its rows. Commit (library) `"hash: increment 4 whole"`.

## Task 39: BearSSL's hashes, HMAC and SHAKE

Stream `bearssl-5`.

- [ ] **Step 1: Write the failing tests:** `br_md5`, `br_sha1`, `br_sha224` to `br_sha512`,
  `br_md5sha1` (TLS 1.0's, 36 bytes: MD5 followed by SHA-1), `br_shake` (128 and 256) through
  `digest` and `hasher`, and HMAC (`br_hmac_*`) through `mac`, against Hash2 on random inputs and
  RFC 4231. Expected: FAIL.
- [ ] **Step 2: Write the engines** in `backends/bearssl.hpp`. Run on the four targets.
  Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: BearSSL's hashes, HMAC and SHAKE"`.

## Task 40: Increment 5 whole

- [ ] Task 47 for increment 5: the `smallest` profile's choices on WebAssembly re-read from the
  regenerated table and reported in the ledger. Commit (library) `"hash: increment 5 whole"`.

## Task 41: Botan's column, at its pin

Stream `botan-6`; waits for webcpp::crypto's Botan provisioning (its increment 5) on `main`.

- [ ] **Step 1: Read Botan at the pin** crypto provisions (3.13.0 measured by crypto; 3.14.1 at
  `01af88c` the latest tag on 2026-11-14): its hash, MAC, KDF and password-hash modules
  (`src/lib/hash/`, `src/lib/mac/`, `src/lib/kdf/`, `src/lib/pbkdf/`), and whether the build crypto
  provisions enables each. Correct `doc/capabilities.json`'s Botan column and record the
  difference from spec 3.5 in the ledger; a change of the spec's table is a change of this
  proposal.

## Task 42: Botan's hashes and MACs

- [ ] **Step 1: Write `backends/botan.hpp`** (exceptions only, as webcpp::crypto's D19: the header
  declares nothing under `WEBCPP_HASH_NO_EXCEPTIONS`; every Botan exception is caught at the
  boundary and mapped), and the failing tests: every digest of the column (BLAKE2b variable,
  Keccak, SHA-3, SHAKE, cSHAKE, Skein-512, SM3, Streebog-256/512, Whirlpool, ...) against its
  official vectors or another backend; KMAC and HMAC. Expected: FAIL.
- [ ] **Step 2: Write the engines** over `Botan::HashFunction::create_or_throw` and
  `Botan::MessageAuthenticationCode`. Run natively. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: Botan's hashes and MACs (native, with exceptions)"`.

## Task 43: Botan's KDFs and password hashes

- [ ] **Step 1: Write the failing tests:** PBKDF2 (RFC 6070), HKDF (RFC 5869), scrypt (RFC 7914),
  Argon2d/i/id (RFC 9106, every parameter), bcrypt (the OpenBSD reference's test strings) and
  bcrypt-pbkdf (OpenSSH's vectors), through `Botan::PasswordHashFamily` and `Botan::KDF`.
  Expected: FAIL.
- [ ] **Step 2: Write the engines.** Run natively. Task 47 for increment 6. Commit (library)
  `"hash: Botan's KDFs and password hashes, and increment 6 whole"`.

## Task 44: The asynchronous family, everywhere

Stream `async-7`.

**Files:**
- Create (library): `include/webcpp/hash/async.hpp`, `test/async_post_test.cpp`.
- Modify (library): `test/Jamfile` (the async tests with `<library>/webcpp/asio//asio`),
  `hash.hpp`'s comment (it leaves `async.hpp` out: webcpp::asio, chapter 3).

**Interfaces:**
- Produces: spec 11's `async_digest`, `async_hmac`, `async_hkdf`, `async_pbkdf2`,
  `default_async_backend`.

- [ ] **Step 1: Write the failing test** `async_post_test.cpp` (native, wasip2, wasip3): each
  function on its synchronous default with a callback that records whether the initiating call
  has returned (it must have), `use_future`, and a guard's refusal (an output of 0 bytes)
  completing with `invalid_output_size` through the executor; a counting allocator associated
  with the handler sees the operation's storage. Expected: FAIL.
- [ ] **Step 2: Write `async.hpp`:** each function initiates through
  `webcpp::asio_backend::async_initiate<CompletionToken, void(boost::system::error_code)>`, runs
  the guards, and for a synchronous backend runs it at once and posts the handler, bound to its
  associated executor, with the result's error code. Run. Expected: PASS.
- [ ] **Step 3: The heavy variants on another thread** (spec 11.5). `config.hpp` gains
  `WEBCPP_HASH_HAS_THREADS` (from `BOOST_HAS_THREADS` natively, `__EMSCRIPTEN_PTHREADS__` on
  Emscripten, never WASI; `WEBCPP_HASH_NO_THREADS` turns it off) with a three-way test
  (`config_threads`, `<threading>multi`; `config_single`; `config_no_threads`). Write the failing
  tests first: `async_heavy_test.cpp`, declared natively with `<threading>multi` and on Emscripten
  with `<threading>multi` (b2 gives `-pthread`): `async_argon2id`, `async_scrypt` and
  `async_pbkdf2<hmac_sha2_512, backends::hash2>` on a `webcpp::asio::thread_pool` of two threads,
  the work's thread id differing from the caller's, results equal to the synchronous functions',
  and a 10 ms timer on the caller's `io_context` firing during an Argon2id of 64 MiB; and a
  message test per name (`test/messages/async_<name>_without_threads/`, built single-threaded and
  on wasip2) stating spec 11.5's text. Then write the variants: guards, `asio::post(work, ...)`
  running the synchronous function, completion dispatched to the handler's associated executor;
  without threads each name is a template whose `static_assert` holds the text. Run natively
  (`threading=multi` and single), on wasip2, and on Emscripten with and without pthreads.
  Expected: PASS.
- [ ] **Step 4: Commit** (library) `"hash: the asynchronous family by post, and the heavy functions on another thread where the build has threads"`.

## Task 45: `backends::webcrypto` on Emscripten

- [ ] **Step 1: Write the failing test** `async_webcrypto_test.cpp` (Emscripten, under node): RFC
  4231's case 1 through `async_hmac`, RFC 5869 A.1 through `async_hkdf`, RFC 6070 case 1 and RFC
  7914 section 11's first case through `async_pbkdf2`, SHA-256 of "abc" through `async_digest`,
  each with a callback, `use_awaitable` and `use_future`; 100 random cases against
  `backends::hash2`; the synchronous `hkdf<hmac_sha2_256, backends::webcrypto>` refuses with spec
  11.2's text (a message test). Expected: FAIL.
- [ ] **Step 2: Write the backend:** each operation builds the JavaScript call
  (`crypto.subtle.digest`, `importKey` then `sign`, `importKey` then `deriveBits` with the
  algorithm's parameters) with `emscripten::val`, hands the promise to
  `webcpp::asio::browser::async_await(executor, promise, ...)`, and in its completion copies the
  resulting `ArrayBuffer` into `output` (a size other than `output.size()` is `backend_failure`),
  or zeroes `output` on rejection (`backend_failure`) or cancellation
  (`asio::error::operation_aborted`). Run the Emscripten lane. Expected: PASS.
- [ ] **Step 3: Commit** (library) `"hash: WebCrypto as backends::webcrypto, through the asynchronous family"`.

## Task 46: The asynchronous family's behaviour

- [ ] **Step 1: Write the tests:** the heavy variants' cancellation (before the work starts:
  `operation_aborted` at once; after: the work ends, then `operation_aborted`, `output` zeroed);
  on Emscripten with pthreads, an `async_argon2id` on a pool of Web Workers under node while the
  main thread's timer keeps firing, the completion bound to
  `webcpp::asio::browser::main_thread_context`'s executor; cancellation through a cancellation
  slot before the promise settles (completion with `operation_aborted`, `output` zeroed, no later write); the handler's
  associated allocator holds the operation's storage (a counting allocator); co_await in Asio
  (`use_awaitable`) and in Cobalt (`cobalt::use_op`, where webcpp::cobalt is on `main`); the spans
  untouched by the caller until completion. Run on every target. Expected: PASS.
- [ ] **Step 2: Task 47 for increment 7.** Commit (library) `"hash: the asynchronous family's cancellation, allocation and tokens, and increment 7 whole"`.

## Task 47: An increment whole (run once per increment 2 to 7)

- [ ] **Step 1:** `doc/capabilities.json` and `capabilities_test` agree; the refusal texts are
  regenerated (`python3 tools/hash/measure.py --refusals --check`).
- [ ] **Step 2:** the measured table regenerated (`python3 tools/hash/measure.py --write`), the
  throughput re-measured natively, the `measure` lane green.
- [ ] **Step 3:** every lane from scratch, the own lanes, the tools' tests, the lint, as Task 24's
  Step 2; a review by someone who did not write the increment.
- [ ] **Step 4:** the documentation's sections of the increment (noble's manner; libsodium's
  adapted with attribution where used, `NOTICE.md` unchanged or extended), the changelog, chapter
  17's entry updated; the owner's gates (G4) for the increment's pull requests.
