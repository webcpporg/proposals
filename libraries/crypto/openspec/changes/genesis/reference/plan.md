---
title: "webcpp::crypto, increments 1 to 5: Implementation Plan"
tags: [webcpp, crypto, bearssl, openssl, secp256k1, ctaes, botan, b2, wasm, emscripten, plan]
status: draft
created: 2026-10-11
revised: 2026-11-14
---

# webcpp::crypto, increments 1 to 5: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal of version 0.1:** a new library, webcpp::crypto, at `libs/crypto`: AES-GCM and the AES
block cipher, a `random_device` on the system's generator, a constant-time `equal`, and
secp256k1 (BIP340, ECDSA, ECDH), each running on a backend that is a C library itself (BearSSL,
OpenSSL, libsecp256k1, ctaes), every one built from its own sources by b2, on native, wasip2,
wasip3 and Emscripten (OpenSSL on native, wasip2 and wasip3); and trystero's build moved onto
the libsecp256k1 this library brings. Increments 3 to 5 (trystero's code moved onto the
library, ChaCha20-Poly1305, Botan) and the deferred parts of increment 1 come after 0.1.

## The revision of 2026-11-14: version 0.1

The plan was written on 2026-10-11 against a tree that has since changed, and the owner set the
scope of the first version on 2026-11-14. This section rules over everything below it; the tasks
of version 0.1 were patched to it, and the deferred tasks carry a note under their heading.

**What changed in the tree** (superproject `main` at `f085d79`, 2026-11-14):
- The base the plan waited on exists: `third_party/` (project `/webcpp/third_party`), asio
  integrated with `webcpp::test::exit_status`, the shared infrastructure merged. The identity
  incident's commits are not in the history. Work reaches `main` by pull request. Tasks 0a and
  25a are obsolete; Task 0 is rewritten.
- OpenSSL is no longer found on the machine: since 2026-11-03 it is built from the submodule
  `third_party/libs/openssl` (OpenSSL 3.5.9 at `45e844f`) as `/webcpp/third_party//openssl`,
  static, for native Clang, GCC and MSVC, wasip2 single and wasip3; Emscripten stops; never a
  system OpenSSL (CODING-GUIDELINES.md chapter 5 rule 7; chapter 17, crypto's "Provider
  amendment, 2026-11-03"). Task 11 is rewritten around it.
- The rules of the old AGENTS.md are split between AGENTS.md and CODING-GUIDELINES.md, renumbered
  on 2026-10-26; the citations below are the current ones.
- A library's `config.hpp` includes `<webcpp/config.hpp>` first and reads the target from
  `WEBCPP_TARGET_*` (CODING-GUIDELINES.md chapter 6, and config's section of chapter 15).
- The WASI lanes run through `tools/boost/asio-wasip2/run.sh` and `tools/boost/asio-wasip3/run.sh`
  (wasmtime 50.0.1), as `tools/ci/matrix.py` `wasm_lane` does.
- The CI has no levels: every lane and every own lane runs on every push.

**The owner's decisions of 2026-11-14:**
1. **Version 0.1 is increment 1 lean and increment 2.** Deferred, kept as written and re-read
   before they run: OpenSSL's memory option (Tasks 15, 16, 33), constant time (Task 20), sizes,
   throughput and `runners.yml` (Task 22), the browser lane (Task 23), the nightly level (Task
   25b), NIST CAVP (gate O3), increments 3, 4 and 5 (Tasks 31 to 39).
2. **The default backend is the spec's, MSVC included:** BearSSL on WebAssembly; natively OpenSSL
   when it is linked, else BearSSL. The documentation says that the source-built OpenSSL is
   compiled `no-asm` on MSVC, wasip2 and wasip3 (T-table AES and 4-bit GHASH, no AES-NI, not
   constant time), and how a program chooses BearSSL there.
3. **New submodules,** each with its row in CODING-GUIDELINES.md chapter 10 in the task that
   adds it: `third_party/libs/bearssl` (Task 5), `wycheproof` (Task 9, shallow), `secp256k1`
   (Task 26), `bips` (Task 28, shallow), `bitcoin` for ctaes (Task 29, shallow; only
   `src/crypto/ctaes` is used, and its `src/secp256k1/` subtree is never on an include path).
4. **trystero's build moves onto `/third_party//secp256k1` in version 0.1** (Task 30a), its code
   unchanged; the CI's external libsecp256k1 goes with it.
5. **Limits recorded instead of deferred machinery:** MSVC's message cases fail on the MSVC lanes
   every push, and their exact text is checked on clang, GNU ld and wasm-ld; ld64's on a
   developer's macOS. crypto has no size table until Task 22. Task 25 records both in
   CODING-GUIDELINES.md chapter 19.

**The controller's rulings of 2026-11-14:**
- **A program names `/webcpp/crypto//openssl`,** crypto's alias over the provider, as asio's
  `/webcpp/asio//openssl` does: on native, wasip2 and wasip3 it links
  `/webcpp/third_party//openssl` and defines `WEBCPP_CRYPTO_HAS_OPENSSL=1`; on Emscripten it
  stops the compile with the fix (Task 11).
- **OpenSSL has no canary:** the provider injects nothing into upstream (CODING-GUIDELINES.md
  chapter 5 rule 7, and chapter 19's "Asio/Beast closure"). Its layers are the compile-time
  `#error`s and MSVC's `detect_mismatch` on the default backend. BearSSL, libsecp256k1 and ctaes
  keep the three layers.
- **Both project ids:** Task 1 registers `/third_party` beside `/webcpp/third_party`.
- **No `webcpp-own`, `webcpp.external-root` or `webcpp.external-looked`:** every backend of
  version 0.1 is built by b2. Botan brings them back if it needs them.
- **The error values 13 and 14** (`memory_functions_*`) are reserved for the memory option and
  not declared in version 0.1 (spec 2.9: values are never reused).
- **The error category has pratt's form;** the test Jamfile's `crypto-run` stays; the empty seam
  `detail/declassify.hpp` stays; ctaes's refusal names no Botan.
- **Commits use the contributor's own identity**, with no trailer of any kind (proposals'
  CONTRIBUTING.md, "Commit identity in older plans").

**The tasks of version 0.1:** 0, 1 to 14, 17 to 19, 21, 24, 25 (increment 1); 26 to 27b, 28 to 30 and 30a
(increment 2). Every other task is deferred or obsolete, as its heading's note says.

**Architecture:**
- **The library is header-only** (`/webcpp/crypto//crypto`, an alias). Its algorithm types
  (`aes128/192/256<Backend>`, `gcm<Cipher>`, `secp256k1::basic_keypair<Backend>`, and later
  `chacha20_poly1305<Backend>`) do every guard, the moved-from state, the incremental seal's
  state machine, the length accounting and the zeroing of a failed `open` themselves, once, for
  every backend; a backend's header supplies only the work, as specialisations of the traits in
  `webcpp::crypto::detail` (`block_state`, `block_engine`, `gcm_state`, `gcm_engine`; later
  `chacha_state`, `chacha_engine`). A state trait is what an object stores and never refuses,
  so `aes256<backends::bearssl>` is a complete type that `gcm` can name; an engine trait is used
  only inside member functions, so a backend without the algorithm refuses with its
  `static_assert` only where the algorithm is called.
- **The C libraries** are submodules under `third_party/libs/`, built file by file by
  `third_party/Jamfile` into static libraries whose usage requirements give
  `WEBCPP_CRYPTO_HAS_<NAME>=1`; webcpp's `libs/crypto/src/canary_<name>.c` joins each. OpenSSL is
  the superproject's source-built provider, which crypto's alias `/webcpp/crypto//openssl` links.
- **The superproject** gains: the project id `/third_party` beside `/webcpp/third_party`;
  `webcpp.same-program`, two `exe`s of one program with and without a library under one output
  name, compared byte for byte, in b2 itself (spec 8.7); a Python test of the tools,
  `tools/test/crypto_jamfiles_test.py`, which builds the user's Jamfiles of the documentation and
  every build stop as fixtures, the pattern of `tools/test/fixtures/` and
  `tools/test/emscripten_test.py`; a lint rule on the entropy header and one on the canary's
  pins; the submodules of decision 3.
- **The lanes:** the ordinary ones (every native compiler of the matrix, MSVC and Apple clang
  included, wasip2, wasip3, emscripten) run the tests of `libs/crypto/test` and the examples on
  every push; the own lane `c-suites` (the C libraries' own test programs) runs on its targets on
  every push. The user's Jamfiles run in the tools job of the superproject's CI
  (`tools/test/crypto_jamfiles_test.py`).

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (System, Hash2, Core's lightweight_test, JSON in tests),
  wasi-sdk 34 with wasmtime 50.0.1, Emscripten 6.0.11 with node 26.7.0, Python 3.9+ for the
  tools (the versions of AGENTS.md chapter 1, "Prerequisites").
- BearSSL at `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (the mirror `webcpporg/bearssl`),
  libsecp256k1 v0.8.0 at `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d`, ctaes as Bitcoin Core
  v31.1 ships it (`9be056a8a72b624dae9623b2f7bded92c2a21c91`), OpenSSL 3.5.9 at `45e844f`,
  source-built by the superproject (`tools/openssl/pin.json`). Later: Botan 3.13.0.
- Test data: Wycheproof at `12fd3aaf33eb5fa1f52e026912ee00c054f9d984`, BIP340's
  `test-vectors.csv` from `bitcoin/bips` at `927b6de9915c9262615a6399de51b200f81e5aa4`. Later:
  NIST CAVP's `gcmtestvectors.zip` and `KAT_AES.zip` (gate O3).

**Spec:** `reference/spec.md` beside this plan (approved by the owner on 2026-10-11; "the spec"
below, cited by section), with the proposal's `design.md` and the provider amendment of
CODING-GUIDELINES.md chapter 17. The owner's decisions it cites by date and title, and the
research it cites by short name (C-LIBS is `reference/research/C_LIBRARIES_IN_WEBCPP.md`), were
private notes of the original machine; the spec quotes what they ruled.

This plan's own measurements, taken on 2026-10-11 for it (macOS arm64, b2 5.5.3, Apple clang
21, Homebrew OpenSSL 3.6.4), recorded here as they were; M5 and M7's OpenSSL lines describe the
old provisioning and belong to the deferred memory option:
- **M1, two project ids.** A Jamroot with `use-project /webcpp/third_party : third_party ;`
  and `use-project /third_party : third_party ;`, the Jamfile declaring
  `project /webcpp/third_party ;`: `/third_party//bearssl` and `/webcpp/third_party//bearssl`
  both resolve, and `/third_party//nothere` fails with "Unable to find file or target named
  '/third_party//nothere'" (`pid/`).
- **M2, a skipped program does not evaluate a library's stop.** A `webcpp.run` declared
  `native` that names an alias whose `<conditional>` stops the build on emscripten, built
  with `toolset=emscripten`: the program is skipped and the stop never fires; a program of
  every target beside it builds (`p2/`).
- **M3, NIST CAVP's archives.** `gcmtestvectors.zip`, 9,154,174 bytes, SHA-256
  `f9fc479e134cde2980b3bb7cddbcb567b2cd96fd753835243ed067699f26a023`, from
  `https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/mac/gcmtestvectors.zip`:
  six files, each 7,875 cases, 1,125 of them with 128-bit tags (IVs of 8, 96 and 1,024
  bits); with 128-bit tags, `gcmEncryptExtIV{128,192,256}.rsp` 1,125 each, and
  `gcmDecrypt128.rsp` 535 valid and 590 `FAIL`, `gcmDecrypt192.rsp` 557 and 568,
  `gcmDecrypt256.rsp` 559 and 566. `KAT_AES.zip`, 726,299 bytes, SHA-256
  `a203b16c9246b2ebae31dee5de21a606be80cf78ceabaca37150236fa098eb60`, from
  `.../documents/aes/KAT_AES.zip`: the ECB files `ECBGFSbox{128,192,256}.rsp` (14, 12, 10
  cases), `ECBKeySbox` (42, 48, 32), `ECBVarKey` (256, 384, 512), `ECBVarTxt` (256, 256, 256).
- **M4, RFC 8439's two vectors through OpenSSL 3.6.4** (`a5/a5.c`): section 2.8.2's 114-byte
  plaintext gives the ciphertext
  `d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc3ff4def08e4b7a9de576d26586cec64b6116`
  and the tag `1ae10b594f09e26a7e902ecbd0600691`; appendix A.5's 265-byte plaintext (key
  `1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0`, nonce
  `000000000102030405060708`, AAD `f33388860000000000004e91`) gives the tag
  `eead9d67890cbb22392336fea1851f38` and the ciphertext written in Task 35.
- **M5, an interposer linked at launch is enough on macOS.** PROBE-R3's `probe on`, run with
  and without `DYLD_INSERT_LIBRARIES`, its `libinterpose.dylib` linked into the program:
  identical counts (first `make`: adapter 6,061 allocations, 192 reallocations; C library from
  libcrypto 0, all callers 2).
- **M6, upstream facts read for this plan:** `gh api` gives Wycheproof's `12fd3aaf` as
  `12fd3aaf33eb5fa1f52e026912ee00c054f9d984` (2026-10-06), `bitcoin/bips` master as
  `927b6de9915c9262615a6399de51b200f81e5aa4` (2026-10-02), the last change of
  `bip-0340/test-vectors.csv` as `200f9b26` (2023-04-20); `webcpporg/bearssl` exists, master
  `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f`, tags v0.4 to v0.6, and `webcpporg/crypto` does
  not. Botan 3.13.0's release archive `https://botan.randombit.net/releases/Botan-3.13.0.tar.xz`
  is 9,598,736 bytes, SHA-256
  `12f5a8358890bbee82edfe9d2e7769b0a610b6dd0e0698aea13d20a675d84620`, and Homebrew's `botan`
  is 3.13.0; in it `GCM_Mode::update_granularity()` and
  `ChaCha20Poly1305_Mode::update_granularity()` return 1 (`src/lib/modes/aead/gcm/gcm.cpp:63-65`,
  `chacha20poly1305/chacha20poly1305.cpp:30-32`).
- **M7, the tree.** `~/dev/webcpp` `main` (73aa1ea) has no `third_party/`; `infra/shared`
  (13e6e22, worktree `~/dev/webcpp-infra`) has `third_party/Jamfile` (project
  `/webcpp/third_party`, targets `json-schema-test-suite` and `srell`), `webcpp.data` and
  `webcpp.generated`. `tools/ci/matrix.py` has no CI level, no Linux arm64 lane, and marks a
  library external with one boolean, which installs OpenSSL, libsecp256k1 and libdatachannel
  together. `.local/openssl/lib` holds `libcrypto.dylib` and `libssl.dylib` alone, links the
  OpenSSL action makes; Homebrew's `/opt/homebrew/opt/openssl@3/lib` holds `libcrypto.a`. The
  lint's world rule matches `random_device`, `getentropy`, `RAND_bytes`, `<sys/random.h>` and
  `<unistd.h>` on any line of a library header without `lint-world:`.


**Not in version 0.1:**
- **Everything decision 1 defers:** OpenSSL's memory option, constant time, sizes and
  throughput, the browser lane, the nightly level, NIST CAVP, increments 3 to 5.
- **The CI's levels, whole** (spec section 8's table): the CI runs every lane on every push.
- **The `asyncify` suspension variant for crypto's emscripten tests** (spec 8: "Emscripten in
  both suspension variants"): crypto's tests are synchronous and never suspend, so the variant
  changes nothing they do.
- **A Botan WebAssembly build** (spec section 3.6, "a later increment's, if a user needs it").
- **An incremental `open`** (spec section 2.4: "a later increment may add it").
- **webcpp::webrtc** and **webcpp::beast**, which may use this library later.

## The base and the order of the work

1. **The base is the superproject's `origin/main`** of the day Task 0 runs (at the revision,
   `f085d79`). It already holds `third_party/`, `libs/asio` with
   `tools/node-test/include/webcpp/test/exit_status.hpp`, the source-built OpenSSL and trystero's
   shared connection.
2. **Every test of crypto uses asio's test support:** every `main` of a test ends
   `return webcpp::test::exit_status(boost::report_errors());` with
   `#include <webcpp/test/exit_status.hpp>`, which `/webcpp//node-test` puts on every program;
   under node the pre-js bounds each test (120 s by default; a test that needs longer, the C
   suites, calls `webcpp::test::bound` with its own and says why) and turns a `main` that never
   finished into a failure. Natively `exit_status` returns its argument.
3. **Rebases.** The work branch rebases onto `origin/main` whenever a pull request that touches
   the same superproject files (`Jamroot`, `tools/webcpp.jam`, `third_party/Jamfile`,
   `tools/test/harness.py`, `tools/ci/matrix.py`, `tools/lint/`, AGENTS.md, CODING-GUIDELINES.md)
   merges, and at the end of each increment. After a rebase the lint and the lanes the branch
   touches run again before anything else.
4. **The pull requests, in order** (Task 25 for increment 1, Task 30 for increment 2):
   1. the owner creates `webcpporg/crypto` (G1); the library's pull request into its `main`;
   2. one superproject pull request with the third-party submodules and `third_party/Jamfile`,
      `/third_party`, `webcpp.same-program`, `crypto_jamfiles_test.py`, the lint rules, the
      rulebook, and the submodule `libs/crypto` (G2): the third-party targets compile
      `libs/crypto/src/canary_<name>.c`, so they cannot land before the submodule;
   3. after increment 2, trystero's pull request and the superproject's that records it
      (Task 30a);
   4. the proposals' pull request that records version 0.1 (proposals' CONTRIBUTING.md,
      "Implemented": the code in the library's `main`, recorded by the superproject's `main`, the
      CI green on both).
   The library's CI reads the superproject's `main`, so the library's pull request is green
   only once the superproject's has merged; until then the evidence is the local lanes, and the
   library's CI is re-run after the merge.

## Owner gates

- **G1:** the owner creates `webcpporg/crypto`, public, BSL-1.0.
- **G2:** the owner adds the submodule (`git submodule add ../crypto.git libs/crypto`).
- **G4:** the owner merges every pull request; an agent never pushes to `main`, creates a
  repository or changes a setting.
- Later, with the deferred tasks: O2 (the CI's nightly level, Task 25b), O3 (NIST's archives,
  Task 9's deferred half), O5 (the departures below, and `brew install botan` for Task 37).

## Departures for the owner (O5, with the deferred tasks)

1. **C17.** Sizes at b2's `speed` (`-O3`) and `space` (`-Os`) where spec 8.8 says -O2 and -Os:
   b2 has no -O2 that reaches the libraries a program names.
2. **C16.** webcpp's own throughput program in `runners.yml` in place of "the job of run
   38113681987", whose workflow no tree holds.
3. **C14** is settled for version 0.1 by decision 5 (MSVC's cases fail on its lanes; their text
   is checked on clang and the GNU and wasm linkers).
4. **`brew install botan`** on the development machine, for Botan's local runs (Task 37).

## Conflicts with the spec

Each is a place where the tree, a measurement or a rule disagrees with the spec's prose, with
this plan's ruling. Rows marked "later" belong to deferred tasks.

| # | The spec says | The tree or a measurement says | Ruling |
| --- | --- | --- | --- |
| C1 | A program names `/third_party//<backend>` (sections 4, 7; CODING-GUIDELINES.md chapter 5 rule 5) | `third_party/Jamfile` declares `project /webcpp/third_party`, and the Jamroot registers that id alone | The Jamroot registers the one project under both ids (M1): `use-project /third_party : third_party ;` beside the existing line. The user's form is `/third_party//<name>`; `/webcpp/third_party//<name>` keeps working for the existing consumers (Task 1) |
| C2 | OpenSSL is found on the machine, linked shared or static, native only (sections 3.3, 4.4, 5) | The superproject builds OpenSSL 3.5.9 from source, static, on native, wasip2 and wasip3, and never uses a system copy (CODING-GUIDELINES.md chapter 5 rule 7; the provider amendment of 2026-11-03) | crypto's alias `/webcpp/crypto//openssl` over the provider, refused on Emscripten; no canary; the default backend as the spec says (Task 11) |
| C3 | Each lane has a level: fast, nightly or release (section 8's table) | `matrix.py` has no level | Every lane runs on every push; the nightly level is Task 25b's, later |
| C4 | Constant time and throughput on Linux arm64 (sections 8.3, 8.8) | The matrix has no arm64 lane | Later: `runners.yml` (Task 22) |
| C5 | The macOS counting test interposes through `DYLD_INSERT_LIBRARIES` (section 8.5) | Linking the interposer counts the same (M5) | Later: Task 16 |
| C7 | The exit test "passes only with that line" (section 8.5) | `webcpp.run` judges by the exit status alone | Later: Task 16 |
| C8 | Sizes and throughput go "into the report" (section 8.8) | `tools/report/report.py` has no place for a table | Later: Task 22. Version 0.1 has no size table, a limit recorded in chapter 19 (decision 5) |
| C9 | `webcpp.same-program <name> : <sources> : <library> : <targets>` (section 8.7) | A program of crypto also needs `/webcpp/crypto//crypto`, which is neither its source nor the library compared | One more, last parameter, `: <requirements> *`, the program's other libraries; the rule is b2 alone, two `exe`s and a comparison (Task 21) |
| C10 | Section 7's Jamfiles were "built and run with b2" and section 8.6's stops are "run on a fixture project" | The lint refuses b2's raw `exe` in a library's `test/` and `example/` Jamfiles (CODING-GUIDELINES.md chapter 9) | Fixtures `tools/test/fixtures/crypto_jamfiles/<case>/`, a user's Jamfile each, built by `tools/test/crypto_jamfiles_test.py` on a scratch superproject as `emscripten_test.py` builds its fixtures (Task 10; the cases in Tasks 11, 18, 24, 29) |
| C11 | NIST CAVP's `gcmtestvectors` and AESAVS (section 8.1) | Published as zip archives on csrc.nist.gov only (M3) | Later: gate O3. Version 0.1 checks AES-GCM against the spec's GCM cases and Wycheproof (Task 9) |
| C12 | BIP340's `test-vectors.csv` (section 8.1) | It lives only in `bitcoin/bips` (M6) | A shallow submodule `third_party/libs/bips` at `927b6de9` (Task 28) |
| C14 | MSVC's `C1189` text is checked "by the Compiler Explorer probe's method" (section 8.6) | That method calls a public service from the CI, unpinned; the tools job runs on Linux only | Decision 5: the cases fail on the MSVC lanes; the text is checked on clang, GNU ld and wasm-ld (Task 18) |
| C15 | Every suite runs with exceptions on and off (section 8) | No lane of the matrix builds without exceptions | The test Jamfile's rule `crypto-run` declares each program twice, the second `<exception-handling>off` with the suffix `_no_exceptions` (Task 1 onwards) |
| C16 | The throughput job is "the one of run 38113681987" (section 8.8) | No tree holds that job's workflow | Later: Task 22 |
| C17 | Sizes at -O2 and -Os (section 8.8) | b2 has no -O2 that reaches the libraries a program names | Later: Task 22 |
| C18 | `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` and the errors 13 and 14 (sections 2.9, 3.7) | Deferred by decision 1 | Values 13 and 14 are reserved, not declared; the option and its errors come with Task 15 |
| C19 | OpenSSL is the native default (section 5) | The provider builds OpenSSL `no-asm` on MSVC, wasip2 and wasip3 | Decision 2: the default stays; the backend's page says what `no-asm` means and how to choose BearSSL (Tasks 14, 24) |

C6 and C13 (OpenSSL's static library on the machine, and where `webcpp.external` declares it)
are gone with the source-built provider.

## Global Constraints

**Repositories and paths:**
- The superproject is `webcpporg/webcpp`, checked out with its submodules; the work happens in
  a worktree of it on a branch `crypto/v0.1` from `origin/main` (Task 0), its `.local` a link
  to the main checkout's. Paths below are relative to that worktree.
- `libs/crypto` is the library's own git repository, created in Task 1, branch `main`, whose
  remote is `webcpporg/crypto` once the owner creates it (G1). Until the submodule is added
  (G2), the superproject ignores it through the common git directory's `info/exclude`
  (`/libs/crypto/`), never through a tracked line.
- trystero (Task 30a) is changed in `libs/trystero`, its own repository, on a branch
  `crypto/secp256k1`.
- Every b2 command runs from the worktree's root, as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`
  (written `$B2` below). A verdict is `-a`. Each lane has its own `--build-dir`. No cap on
  concurrent builds: a test that fails under load is flaky and is fixed, never protected by
  running less at once.
- The lanes, as the CI runs them (`tools/ci/matrix.py`):
  - native: `$B2 -a --build-dir=bin/lane-native libs/crypto/test libs/crypto/example`;
  - wasip2: `$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test libs/crypto/example`;
  - wasip3: the same with `clang-wasip3`, `tools/boost/asio-wasip3/run.sh` and `bin/lane-wasip3`;
  - emscripten: `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/crypto/test libs/crypto/example`;
  - gcc: `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/crypto/test libs/crypto/example`;
  - MSVC and Apple clang run in the CI only (`msvc-14.3`, `msvc-14.5`, `apple-clang` lanes of
    the matrix); their evidence is the CI's;
  - the own lane `c-suites` on target `<t>`: the target's lane command with
    `libs/crypto/test/c_suites//c-suites` in place of the two directories; it names its targets,
    so that the CI runs it on each;
  - the user's Jamfiles: `python3 tools/test/crypto_jamfiles_test.py` (Task 10), which the
    superproject's tools job runs.
- The lint: `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`,
  ending `lint: clean`.

**The library's rules (CODING-GUIDELINES.md chapters 2, 3, 5 to 9; the spec):**
- Names: namespace `webcpp::crypto`, `webcpp::crypto::backends`, `webcpp::crypto::secp256k1`;
  `detail` for what is not public; macros `WEBCPP_CRYPTO_*`, test macros `WEBCPP_TEST_*`;
  include guards `WEBCPP_CRYPTO_<PATH>_HPP` (`WEBCPP_CRYPTO_DETAIL_CANARY_H` for the C
  header); b2 target `/webcpp/crypto//crypto`; error category `webcpp.crypto`; `snake_case`.
- `<webcpp/crypto/config.hpp>` first in every public header; it includes `<webcpp/config.hpp>`
  first and derives `WEBCPP_CRYPTO_NO_EXCEPTIONS` from `WEBCPP_HAS_EXCEPTIONS` and
  `WEBCPP_CRYPTO_WEBASSEMBLY` from `WEBCPP_TARGET_*`; no header under `include/` reads
  `__wasi__` or `__EMSCRIPTEN__` (tests and `src/` may). An API that throws sits behind `#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS` and
  raises through `boost::throw_exception`, never a bare `throw`.
- Every line of a library header that names `random_device`, `getentropy`, `RAND_bytes`,
  `<sys/random.h>` or `<unistd.h>` says why after `lint-world:`, the class's own name
  included: `// lint-world: random_device reads the system's entropy, its purpose`.
- Every operation that can fail returns `boost::system::result<T>`; the values of `error` are
  those of spec section 2.9, fixed, never reused (13 and 14 reserved, C18).
- Data, keys and outputs are `std::span<std::uint8_t const>` and `std::span<std::uint8_t>`;
  sizes `std::size_t`; a limit wider than `size_t` on wasm32 is `std::uint64_t`, compared
  through `detail::exceeds(std::uint64_t, std::uint64_t)` so that no comparison is
  tautological on wasm32 (clang's `-Wtautological-constant-out-of-range-compare`, an error
  under `<warnings-as-errors>on`).
- No allocation in webcpp's code but `keypair::make`'s context, in the resource it is given.
  No global or static mutable state in a header. A test's counters and replaced allocation
  functions are global, each line with
  `// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)` and the reason.
- No C array in C++ code (`modernize-avoid-c-arrays`): `std::array`.
- A function starts with its guards. A comment says what the code does not. Doc Comments: a
  one-sentence brief, `@param`, `@tparam`, `@return`, only the allowed commands; every
  public symbol documented, deleted members included (CODING-GUIDELINES.md chapter 7).
- `(max)()`, `(min)()` and `(std::numeric_limits<T>::max)()` in every header (spec 2.2).
- Every header under `include/webcpp/crypto/`, `detail/` included, is in the test Jamfile's
  headers-alone list and in `meta/include-boundaries.json` once it exists, added by the task
  that creates it.
- Tests use `<boost/core/lightweight_test.hpp>` and `test/require.hpp` (xactor's), and end
  `return webcpp::test::exit_status(boost::report_errors());` with
  `#include <webcpp/test/exit_status.hpp>`. Every test program is declared through the test
  Jamfile's `crypto-run` (C15), on every target its backend has.
- A message's test is a `webcpp.compile-diagnostic` in a directory of its own (spec 4.3's
  pitfall), whose source states the whole text on the header that raises it, as wasi's tests
  do: `// expected-error@webcpp/crypto/<header>:* {{<the text>}}`. `compile-diagnostic` is
  clang's, and skips Emscripten: a message raised on Emscripten is a fixture of
  `tools/test/crypto_jamfiles_test.py`. On MSVC the same case is a `webcpp.compile-fail` or
  `webcpp.link-fail` (decision 5).

**Commits and text:**
- Every commit is made with the contributor's own git identity, with no trailer of any kind and
  never an agent co-author: `git commit -m "<message>" -- <paths>` (in the library
  `git -C libs/crypto commit ...`, in trystero `git -C libs/trystero ...`). After each,
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage by path: a new file is added first, `git add -- <new paths>`, since
  `commit -- <paths>` refuses a path git does not track. Never `git add -A`, `git add .`
  or `git add libs`.
- Subjects: `crypto: <what>` in the library; `third_party: <what>`, `tools: <what>`,
  `Jamroot: <what>`, `tools/ci: <what>`, `guidelines: <what>` in the superproject;
  `trystero: <what>` in trystero.
- A change a user can see adds its changelog line in the same commit: the library's
  `doc/changelog.adoc` (from Task 1), the superproject's `doc/releases/0.1.0.adoc`, "Tools and
  build", for a tool or a b2 target (AGENTS.md chapter 5).
- Everything written to a file is English. No em dash (U+2014) anywhere. The word the lint's
  banned-word rule matches (`tools/lint/rules.py`) appears nowhere, in files or commits. No
  home directory path in a tracked file.
- Every source file opens with the licence notice (`//` for C++ and AsciiDoc, `/* */` for C,
  `#` for Jam, Python, shell and YAML, `<!-- -->` for HTML, `//` for JavaScript).
- No environment variable is ever printed (no bare `env`, `export`, `printenv`, `set`).

**Names fixed by this plan:**

| Name | What it is |
| --- | --- |
| `/webcpp/crypto//crypto` | the library: `include`, Boost's headers, `/webcpp/config//config`, and on Windows `bcrypt` |
| `/webcpp/crypto//openssl` | crypto's alias over `/webcpp/third_party//openssl`, with `WEBCPP_CRYPTO_HAS_OPENSSL=1`; refused on Emscripten (Task 11) |
| `/third_party//bearssl`, `//ctaes`, `//secp256k1` | the backends' libraries (spec 4.1); later `//botan` |
| `/third_party//wycheproof`, `//bips` | test data: a define naming the directory, and the directory under wasmtime |
| `webcpp-secp256k1-tables` | `auto` (default), `small`, `large`, propagated |
| `webcpp.same-program <name> : <sources> + : <library> : <targets> * : <requirements> *` | a test: `<sources>` built with and without `<library>` under one output name, byte-identical, in b2 (`tools/same_program/same_program.jam`) |
| `tools/test/crypto_jamfiles_test.py`, `tools/test/fixtures/crypto_jamfiles/<case>/` | the user's Jamfiles and the build's stops, each a fixture with its `<case>.outcome`, built on a scratch superproject |
| `WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address, size)` | the constant-time checks' seam: empty unless a test defines it (Task 20, later) |
| `test/support/counting.hpp` | every `operator new` form and the C library's allocator, counted (Task 17) |
| `webcpp::crypto::detail::{block,gcm}_{state,engine}<Backend, ...>` | the traits a backend specialises; later `chacha_{state,engine}` |
| `WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY` | the test seam of `random_device`: the source becomes `webcpp_test_entropy` |
| `WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE` | the test seam that keeps BearSSL on `aes_ct64` and `ghash_ctmul64` |
| `WEBCPP_CRYPTO_<NAME>_CANARY_COMMIT`, `_CANARY_CONFIGURATION` | the canary's test seams (`detail/canary.h`) |
| own lane `c-suites` | `libs/crypto/test/c_suites`, naming its targets; later `constant-time`, `browser`, `sizes`, `throughput` |
| `.superpowers/sdd/WEBCPP_CRYPTO_PLAN/progress.md` | the ledger, in the worktree, untracked |

## Review Focus

1. **A one-shot `seal` or `open` while an incremental message is in progress.** The spec lists
   `invalid_state` for `begin_seal` during a message and for `update` or `result` without one,
   and is silent on the one-shot calls. Both re-initialise the backend's context, so letting
   them run would silently abandon the message, whose encrypted pieces then get a tag of
   another message. A reasonable person expects a refusal that leaves both the call's `data`
   and the message as they were: `invalid_state`, the message still in progress, its
   `result()` still correct. Task 8 tests it on BearSSL, Task 12 on OpenSSL.
2. **Spans that alias.** A wire format that appends the tag (trystero's, WebCrypto's) invites
   an `open` whose `tag` is a subspan right after `data`, or overlapping its end, and a `seal`
   whose IV or additional data sits in the same buffer. A failed `open` zeroes `data`, which
   would wipe an overlapping tag while the backend reads it, and in-place decryption would
   overwrite an overlapping IV. Expected: correct results when the spans touch, and an
   overlapping tag is an `authentication_failed` with `data` zeroed and nothing else
   corrupted. `gcm` copies the tag into a local array and consumes the IV and the additional
   data before the first byte of `data` is written; Task 7 tests adjacent and overlapping
   spans.
3. **Empty inputs.** An empty plaintext with empty additional data (GCM case 1), an `update`
   with an empty span, `fill` of 0 bytes, `equal` of two empty spans, `encrypt` of zero
   blocks, a span whose `data()` is null. OpenSSL reads `EVP_EncryptUpdate` with a null output
   as additional data, and BearSSL takes a null pointer with length 0 differently; every
   backend must give the same tag. Tasks 3, 4, 7, 8 and 12 test them on each backend.
4. **Two translation units that choose different default backends.** One file compiled with
   `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` and one without, with OpenSSL linked, makes
   `aes256_gcm` two types under one name; a function of the program's that names it in a
   header is then an ODR violation that links silently. Expected: layer 3's kind of net:
   `<webcpp/crypto/aes256_gcm.hpp>` carries
   `#pragma detect_mismatch("webcpp_crypto_default_backend", "<name>")` on MSVC, which fails
   such a program with LNK2038, and the documentation says to give the define in the project's
   requirements. Task 14 adds it and Task 18 tests it on the MSVC lanes.
5. **OpenSSL on two threads.** The ordinary lanes build OpenSSL `no-threads` (b2's default
   `threading=single`), where OpenSSL's error queue is the process's; a program that builds
   `<threading>multi` gets the threaded build. Expected: two threads, each with its own `aead`,
   seal and open correctly and each leaves the error queue as it found it. Task 12 tests it
   natively with `<threading>multi`.
6. **Moving an object that was moved from.** `aead c = std::move(b)` with `b` already empty, a
   move-assignment from it, `std::swap` of an empty and a live object: BearSSL's move would
   run `br_gcm_init` through a wiped, null vtable. Expected: the destination is empty too, and
   every function of both answers `invalid_state`. Tasks 7, 12 and 29 test it on every type and
   backend.
7. **A lane or a check that passes having built nothing.** An own lane that names no target
   runs natively only; a `make` whose sources a target skips is skipped whole and the lane is
   green; `webcpp.compile-diagnostic` skips Emscripten. Expected: every own lane names its
   targets, a lane that builds nothing on a target it names fails, and every Emscripten message
   is a fixture. Tasks 5, 10 and 25 test it.
8. **One copy per program.** asio and crypto both link the source-built OpenSSL, and trystero
   and crypto both reach libsecp256k1. Expected: a program that names `/webcpp/asio//openssl`
   and `/webcpp/crypto//openssl` links `libssl.a` and `libcrypto.a` once each, from one build
   directory, and after Task 30a no program sees a second `secp256k1.h`. Tasks 11 and 30a test
   it.
9. **Allocation outside webcpp's sight.** A `malloc` in C code never reaches `operator new`.
   Expected: the allocation tests count the C library's allocator where a program can see it,
   with a control that moves the counter. Task 17 tests it.

---

# Increment 1: the foundation, and AES-GCM on BearSSL and OpenSSL

## Task 0a: `infra/shared` integrated into `main` first (obsolete)

Obsolete since the revision of 2026-11-14: `infra/shared` has integrated into `main`, and the
identity incident's commits are not in its history. Nothing of this task runs.

## Task 0: The worktree, the ledger and the baseline

**Files:**
- Create: `.superpowers/sdd/WEBCPP_CRYPTO_PLAN/progress.md` in the worktree (untracked).

- [ ] **Step 1: Make the worktree from `origin/main`.**

```bash
git -C <superproject> fetch origin
git -C <superproject> worktree add <worktree> -b crypto/v0.1 origin/main
cd <worktree>
ls tools/node-test/include/webcpp/test/exit_status.hpp third_party/Jamfile tools/webcpp-openssl.jam
git submodule update --init
git submodule status
ln -s <superproject>/.local .local
```

  Expected: the three files listed (a missing one stops the task and is reported), one line
  per submodule without a leading `-` or `+`, `libs/asio`, `libs/trystero` and
  `third_party/libs/openssl` among them; `ls -l .local` shows the link. The base's commit goes
  into the ledger.
- [ ] **Step 2: Create the ledger** with the first line
  `# SDD ledger - plan: libraries/crypto/openspec/changes/genesis/reference/plan.md (proposals)`.
- [ ] **Step 3: Record the baseline** in the ledger, each with its output:
  - `git rev-parse HEAD` (expected: Step 1's base);
  - `grep -n '^project' third_party/Jamfile` (expected: `project /webcpp/third_party ;`) and
    `grep -n 'use-project /webcpp/third_party' Jamroot` (one line);
  - `$B2 -d0 declared-targets` and `$B2 -d0 declared-lanes`;
  - `.local/wasi-sdk/bin/clang --version | head -1`, `wasmtime --version` (50.0.1),
    `.local/emsdk/upstream/emscripten/em++ --version | head -1` (6.0.11),
    `.local/emscripten/node --version` (v26.7.0);
  - the provider: `$B2 --build-dir=bin/lane-native libs/asio/test//tls_client_context` builds
    OpenSSL and passes (its first build takes minutes);
  - the lint: expected `lint: clean`; the native lane of trystero
    (`$B2 -a --build-dir=bin/lane-native libs/trystero/test`): expected all passed.
  A failure found here is fixed first, in its own pull request, as CODING-GUIDELINES.md chapter 1
  asks.

## Task 1: The library's skeleton, the second project id and the test Jamfile's rule

**Files:**
- Create (in `libs/crypto`, a new git repository): `LICENSE_1_0.txt` (the superproject's),
  `README.md`, `AGENTS.md`, `.gitattributes`, `.gitignore`, `.github/workflows/ci.yml`,
  `meta/libraries.json`, `meta/include-boundaries.json`, `build.jam`,
  `include/webcpp/crypto.hpp`, `include/webcpp/crypto/config.hpp`, `test/Jamfile`,
  `test/config_test.cpp`, `test/require.hpp`, `test/.clang-tidy`, `example/Jamfile`,
  `example/.clang-tidy`, `doc/Jamfile`, `doc/crypto.adoc`, `doc/overview.adoc`,
  `doc/testing.adoc`, `doc/changelog.adoc`, `doc/mrdocs.yml`.
- Modify (superproject): `Jamroot` (one `use-project` line), `tools/test/jamroot_test.py`
  (one case and its docstring), `CODING-GUIDELINES.md` (chapter 10's "Third-party code" and a
  bullet of chapter 19), `doc/releases/0.1.0.adoc` (its line), the common git directory's
  `info/exclude` (`/libs/crypto/`).

**Interfaces:**
- Produces: the project `/webcpp/crypto` and its alias `crypto`; the macros
  `WEBCPP_CRYPTO_NO_EXCEPTIONS` and `WEBCPP_CRYPTO_WEBASSEMBLY`; the forward declarations
  `webcpp::crypto::backends::{bearssl, openssl, ctaes, secp256k1}`; the project id
  `/third_party`; the test Jamfile's rule `crypto-run <name> : <sources> + : <requirements> * : <targets> *`.

- [ ] **Step 1: Write the failing tools test.** In `tools/test/jamroot_test.py`, a case that
  builds a scratch alias through each id:

```python
def test_third_party_has_both_ids(root):
    # A user's Jamfile names /third_party//<name> (CODING-GUIDELINES.md chapter 5, rule 5);
    # the existing consumers name /webcpp/third_party//<name>: both reach one project. The probe
    # is appended to the real third_party/Jamfile, whose targets the Jamroot names.
    with (root / 'third_party/Jamfile').open('a') as jamfile:
        jamfile.write('\nalias probe : : : : <define>WEBCPP_TEST_PROBE=1 ;\nexplicit probe ;\n')
    harness.add_library(root, 'ids', 'alias one : /third_party//probe ;\n'
                        'alias two : /webcpp/third_party//probe ;\n', {})
    result = harness.run_b2(root, '-n', 'libs/ids/test//one', 'libs/ids/test//two')
    harness.expect(result, True)
```

  and add it to `CASES`; the module's docstring gains "reaches third_party/ through both of
  its project ids". Run `python3 tools/test/jamroot_test.py test_third_party_has_both_ids`.
  Expected: FAIL, "Unable to find file or target named '/third_party//probe'".
- [ ] **Step 2: Register the id.** In the Jamroot, after
  `use-project /webcpp/third_party : third_party ;`:

```jam
# The same project under the id a user's Jamfile writes, /third_party//<name>
# (CODING-GUIDELINES.md chapter 5, "Bringing a C library into webcpp"): b2
# resolves both ids to the one Jamfile.
use-project /third_party : third_party ;
```

  Run the case again. Expected: PASS. Then, in `CODING-GUIDELINES.md` chapter 10, "Third-party
  code", the sentence "`third_party/Jamfile` declares it as `/webcpp/third_party//<target>`"
  says that the Jamroot registers the project under both ids, `/webcpp/third_party//<target>`
  and `/third_party//<target>`, the latter the form a user's Jamfile writes (chapter 5, rule 5);
  and chapter 19's bullet "Chapter 10 registers the project `/webcpp/third_party` (`Jamroot`),
  while chapter 5's rule 5 writes `/third_party//bearssl` and crypto's plan
  `/third_party//<name>`." is removed. `doc/releases/0.1.0.adoc`, under "Tools and build",
  gains `* Added the project id /third_party, the form a user's Jamfile writes for third-party
  code.`
- [ ] **Step 3: Create the repository.**

```bash
mkdir -p libs/crypto && git -C libs/crypto init -b main
cp LICENSE_1_0.txt libs/crypto/
echo /libs/crypto/ >> "$(git rev-parse --git-common-dir)/info/exclude"
```

- [ ] **Step 4: Write the failing test** `test/config_test.cpp`, document's
  (`libs/document/test/config_test.cpp`) with the names changed, plus the classification of
  the target:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/crypto/config.hpp> defines WEBCPP_CRYPTO_NO_EXCEPTIONS exactly when
// webcpp has no exceptions, keeps the developer's own definition in a build that
// has them, and defines WEBCPP_CRYPTO_WEBASSEMBLY exactly on wasip2, wasip3 and
// Emscripten. test/Jamfile builds this file as the build asks; without exceptions;
// with WEBCPP_NO_EXCEPTIONS defined as a developer defines it for every webcpp
// library, which WEBCPP_TEST_CRYPTO_ALL_OFF tells this file; and with the macro
// itself defined on the command line, which WEBCPP_TEST_CRYPTO_DEVELOPER_DEFINED
// tells this file.

#include <webcpp/crypto/config.hpp>

// webcpp's configuration, which config.hpp includes, has said whether webcpp
// has exceptions and which target the program is for.
#ifndef WEBCPP_CONFIG_HPP
#error "config.hpp does not include <webcpp/config.hpp>"
#endif

#ifdef WEBCPP_TEST_CRYPTO_ALL_OFF
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_CRYPTO_NO_EXCEPTIONS"
#endif
#elif defined(WEBCPP_TEST_CRYPTO_DEVELOPER_DEFINED)
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
#error "config.hpp drops WEBCPP_CRYPTO_NO_EXCEPTIONS, which the developer defined"
#endif
#elif defined(WEBCPP_HAS_EXCEPTIONS) == defined(WEBCPP_CRYPTO_NO_EXCEPTIONS)
#error "WEBCPP_CRYPTO_NO_EXCEPTIONS is not defined exactly when webcpp has no exceptions"
#endif

#if defined(WEBCPP_TARGET_NATIVE) == defined(WEBCPP_CRYPTO_WEBASSEMBLY)
#error "WEBCPP_CRYPTO_WEBASSEMBLY is not defined exactly on WebAssembly"
#endif
```

- [ ] **Step 5: Write `build.jam`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# crypto: authenticated encryption, randomness and secp256k1 over C backends.
# A program names /webcpp/crypto//crypto and the library of each backend it
# uses, /third_party//<backend> (third_party/Jamfile), or, for OpenSSL,
# /webcpp/crypto//openssl (Task 11):
#
#     exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;

project /webcpp/crypto ;

# Header-only, on webcpp's configuration. random_device reads Windows'
# generator through bcrypt (BCryptGenRandom), a system library.
# tag::crypto[]
alias crypto : : : : <include>include <library>/boost//headers
  <library>/webcpp/config//config <target-os>windows:<find-shared-library>bcrypt ;
# end::crypto[]
```

- [ ] **Step 6: Write `include/webcpp/crypto/config.hpp`.** This step writes the part every
  later task builds on; Task 6 adds the backends' refusals and the default.

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of webcpp::crypto, which each of its headers includes first:
 whether it is built without exceptions, whether the target is WebAssembly, the
 backends the build links and the default backend.

 @see "Using webcpp::crypto in your Jamfile", in the guide.
*/
#ifndef WEBCPP_CRYPTO_CONFIG_HPP
#define WEBCPP_CRYPTO_CONFIG_HPP

#include <webcpp/config.hpp>

// MrDocs parses crypto natively, as a build with exceptions does, where neither
// macro is defined: these definitions, each undone at once, are the ones the
// reference lists.
#ifdef __MRDOCS__
/**
 Defined when webcpp::crypto is built without exceptions: when webcpp has none (the
 compiler has none, or the developer defined `WEBCPP_NO_EXCEPTIONS` for every webcpp
 library), or when the developer defines it alone.

 Every operation reports its errors as values whether or not it is defined. The random
 device's `operator()`, whose failure could not be reported, is declared only when it is
 not defined.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_CRYPTO_NO_EXCEPTIONS
#undef WEBCPP_CRYPTO_NO_EXCEPTIONS
/** Defined on the WebAssembly targets, wasip2, wasip3 and Emscripten. */
#define WEBCPP_CRYPTO_WEBASSEMBLY 1
#undef WEBCPP_CRYPTO_WEBASSEMBLY
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_CRYPTO_NO_EXCEPTIONS)
#define WEBCPP_CRYPTO_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3) || \
    defined(WEBCPP_TARGET_EMSCRIPTEN)
#define WEBCPP_CRYPTO_WEBASSEMBLY 1
#endif

namespace webcpp::crypto::backends {

/** BearSSL, built from its sources by `/third_party//bearssl`. */
struct bearssl;
/** OpenSSL 3.5, built from its pinned sources, named by `/webcpp/crypto//openssl`. */
struct openssl;
/** ctaes as Bitcoin Core ships it, built from its source by `/third_party//ctaes`. */
struct ctaes;
/** libsecp256k1, built from its sources by `/third_party//secp256k1`. */
struct secp256k1;

}  // namespace webcpp::crypto::backends

#endif  // WEBCPP_CRYPTO_CONFIG_HPP
```

  `include/webcpp/crypto.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every header of webcpp::crypto that needs no backend, and the header of each
 backend the build links.
*/
#ifndef WEBCPP_CRYPTO_HPP
#define WEBCPP_CRYPTO_HPP

#include <webcpp/crypto/config.hpp>

#endif  // WEBCPP_CRYPTO_HPP
```

- [ ] **Step 7: Write `test/Jamfile`** with the rule every later task uses (C15):

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/crypto//crypto ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# Every test runs with exceptions on and off (DESIGN, 2026-10-10 "every
# variant tested"): the second program has the suffix _no_exceptions and is
# built with exception-handling=off, which links tools/throw_exception.cpp.
rule crypto-run ( name : sources + : requirements * : targets * )
{
    webcpp.run $(name) : $(sources) : $(requirements) : $(targets) ;
    webcpp.run $(name)_no_exceptions : $(sources)
      : $(requirements) <exception-handling>off : $(targets) ;
}

webcpp.headers-alone crypto : ../include : crypto.hpp crypto/config.hpp ;

# config.hpp defines WEBCPP_CRYPTO_NO_EXCEPTIONS exactly when webcpp has no
# exceptions, keeps it when a developer defines it, defines it when
# WEBCPP_NO_EXCEPTIONS is, and classifies the target.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_no_exceptions : config_test.cpp : <exception-handling>off ;
webcpp.compile config_all_off : config_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_CRYPTO_ALL_OFF ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_CRYPTO_NO_EXCEPTIONS <define>WEBCPP_TEST_CRYPTO_DEVELOPER_DEFINED ;
```

  `test/require.hpp` is xactor's (`libs/xactor/test/require.hpp`) with the guard
  `WEBCPP_TEST_CRYPTO_REQUIRE_HPP`. `test/.clang-tidy` and `example/.clang-tidy` are
  xactor's, with their comments. `example/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/crypto//crypto ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;
```

- [ ] **Step 8: Run** `$B2 -a --build-dir=bin/lane-native libs/crypto/test`. Expected: PASS,
  `alone-crypto`, `alone-crypto-config`, `config`, `config_no_exceptions`, `config_all_off`,
  `config_developer_defined`. Then the wasip2 and emscripten lanes. Expected: PASS, the
  same six.
- [ ] **Step 9: Write the repository's files:**
  - `meta/libraries.json`:

```json
{
    "key": "crypto",
    "name": "crypto",
    "authors": ["R"],
    "description": "Authenticated encryption, randomness and secp256k1 for C++20 on every target, over BearSSL, OpenSSL, ctaes and libsecp256k1.",
    "category": ["Cryptography"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

  - `meta/include-boundaries.json`, which keeps the backend-free headers free of every C
    library. Its `headers` list names only headers that exist, since the lint fails a glob
    that matches no file: `config.hpp` now, and each later task adds its own (`error.hpp` in
    Task 2, `equal.hpp` in Task 3, `random_device.hpp` in Task 4, `aes.hpp` and `gcm.hpp`
    with theirs):

```json
{
    "boundaries": [
        {
            "headers": ["include/webcpp/crypto/config.hpp"],
            "must-not-include": ["bearssl", "openssl/", "ctaes", "secp256k1", "webcpp/crypto/backends/"],
            "why": "a program that only draws random bytes or compares tags names no backend library (spec section 2.2)"
        }
    ]
}
```

  - `.gitignore`: `doc/html/` (crypto has no oracle, so no `node_modules` or `.node-modules/`);
  - `.gitattributes`: pratt's two lines (`* text=auto eol=lf`, `*.bat text eol=crlf`);
  - `.github/workflows/ci.yml`: pratt's, with `library: crypto`;
  - `README.md`: what crypto is (spec 1.1 in four sentences), the backends' table (spec 3.1's
    first four columns), how to build and test it in the superproject (the lane commands of
    Global Constraints), where its page is published
    (`https://webcpporg.github.io/libs/crypto/`), its licence (BSL-1.0) and the backends'
    (BearSSL MIT, ctaes MIT, libsecp256k1 MIT, OpenSSL Apache-2.0), and where OpenSSL comes
    from: the superproject builds it from `third_party/libs/openssl` (OpenSSL 3.5.9 at
    `45e844f`), static, natively and on wasip2 and wasip3, never a system copy, and refuses it
    on Emscripten (CODING-GUIDELINES.md chapter 5, rule 7);
  - `AGENTS.md`: document's opening ("# Working on crypto", with the links to
    `../../CODING-GUIDELINES.md` and `../../AGENTS.md`); "What crypto is" (webcpp's own library over C
    backends, `"port-of": null`, no oracle); "Cautions": a backend's header supplies only the
    six traits, and every guard lives in the algorithm types; a test of a backend is a `.cpp`
    of its own per backend, never a define that changes the backend of one source (spec 4.3's
    pitfall); every C library is unchanged upstream code; a moved pin is a change of its own
    (CODING-GUIDELINES.md chapter 5, rule 12).
- [ ] **Step 10: Write the page.** `doc/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

import webcpp ;

webcpp.doc crypto : crypto.adoc ;
webcpp.reference crypto ;
```

  `doc/mrdocs.yml` is pratt's (`show-enum-constants: true`, the error's values being fixed).
  `doc/crypto.adoc` opens with the licence notice and the attributes of AGENTS.md chapter 3
  (`= crypto: authenticated encryption, randomness and secp256k1 for {cpp}`, `:toc: left`,
  `:toclevels: 1`, `:idprefix:`, `:sectanchors:`, `:example-caption!:`,
  `:attribute-missing: warn`), one paragraph (what crypto is, one of the libraries of webcpp),
  `include::overview.adoc[]`, `include::testing.adoc[]`, the reference section of
  AGENTS.md chapter 3 ("The page") and, last, `include::changelog.adoc[]`. `overview.adoc`
  (`[#crypto-overview]`, "Why crypto") holds spec 1.1 and 1.2 as prose. `testing.adoc` (`[#crypto-testing]`) says how crypto is tested: the lanes and
  their commands, `{n-tests}` tests, the report's address. `changelog.adoc` is the form of
  AGENTS.md chapter 5: `[appendix]`, `[#crypto-changelog]`, `== Changelog`, a paragraph, then
  `[#crypto-changelog-unreleased]`, `=== Unreleased` and, between `// tag::unreleased[]` and
  `// end::unreleased[]`, the line `* Added crypto: authenticated encryption, randomness and
  secp256k1 over C backends.` From Task 2 on, each change a user sees adds its line there in
  its own commit.
- [ ] **Step 11: Run** `$B2 -a --build-dir=bin/b-doc libs/crypto/doc`. Expected: PASS, the
  page written. Then the lint. Expected: `lint: clean`.
- [ ] **Step 12: Commit** (library), then (superproject):

```bash
git -C libs/crypto add -- LICENSE_1_0.txt README.md AGENTS.md .gitattributes .gitignore \
    .github meta build.jam include test example doc
git -C libs/crypto commit \
    -m "crypto: the library's skeleton: its configuration, its page and its CI"
git commit \
    -m "Jamroot: third_party also as /third_party, the id a user's Jamfile writes" \
    -- Jamroot tools/test/jamroot_test.py CODING-GUIDELINES.md doc/releases/0.1.0.adoc
```

## Task 2: The errors

**Files:**
- Create: `include/webcpp/crypto/error.hpp`, `test/error_test.cpp`.
- Modify: `test/Jamfile`, `include/webcpp/crypto.hpp` (the include).

**Interfaces:**
- Produces: `enum class webcpp::crypto::error`, values 1 to 12 of spec 2.9 (13 and 14,
  `memory_functions_refused` and `memory_functions_not_installed`, are reserved for OpenSSL's
  memory option, deferred after v0.1, and not declared);
  `webcpp::crypto::error_category()`; `webcpp::crypto::make_error_code(error)`;
  `boost::system::is_error_code_enum<webcpp::crypto::error>`.

- [ ] **Step 1: Write the failing test** `test/error_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The values of webcpp::crypto::error are fixed (spec section 2.9), its category is
// webcpp.crypto, and the two messages the spec writes out are exact.

#include <webcpp/crypto/error.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/result.hpp>

#include <array>
#include <string_view>

namespace crypto = webcpp::crypto;

int main() {
    using crypto::error;
    BOOST_TEST_EQ(static_cast<int>(error::invalid_key_size), 1);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_iv_size), 2);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_tag_size), 3);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_input_size), 4);
    BOOST_TEST_EQ(static_cast<int>(error::message_too_long), 5);
    BOOST_TEST_EQ(static_cast<int>(error::authentication_failed), 6);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_state), 7);
    BOOST_TEST_EQ(static_cast<int>(error::entropy_unavailable), 8);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_secret_key), 9);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_public_key), 10);
    BOOST_TEST_EQ(static_cast<int>(error::invalid_signature), 11);
    BOOST_TEST_EQ(static_cast<int>(error::backend_failure), 12);

    boost::system::error_code const code = error::authentication_failed;
    BOOST_TEST_EQ(std::string_view(code.category().name()), "webcpp.crypto");
    BOOST_TEST(code.category() == crypto::error_category());

    boost::system::result<void> const failed = error::invalid_state;
    BOOST_TEST(!failed);
    BOOST_TEST(failed.error() == error::invalid_state);

    BOOST_TEST_EQ(crypto::make_error_code(error::authentication_failed).message(),
                  "the authentication tag does not match");
    // 13 and 14 are reserved, not declared: their messages are the unknown one.
    BOOST_TEST_EQ(crypto::make_error_code(static_cast<error>(13)).message(),
                  "unknown webcpp.crypto error");
    BOOST_TEST_EQ(crypto::make_error_code(static_cast<error>(99)).message(),
                  "unknown webcpp.crypto error");
    // The form that allocates nothing returns the whole message, and copies into the buffer
    // what fits, with its terminating zero.
    std::array<char, 8> buffer{};
    BOOST_TEST_EQ(
        std::string_view(crypto::error_category().message(6, buffer.data(), buffer.size())),
        "the authentication tag does not match");
    BOOST_TEST_EQ(std::string_view(buffer.data()), "the aut");
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile`: `crypto-run error : error_test.cpp ;`. Run the native lane. Expected:
  FAIL, `webcpp/crypto/error.hpp` not found.
- [ ] **Step 2: Write `include/webcpp/crypto/error.hpp`**, pratt's form
  (`libs/pratt/include/webcpp/pratt/errors.hpp`: a `constexpr` category with a fixed identity,
  which holds no state, CODING-GUIDELINES.md chapter 15, pratt), each enumerator with a Doc
  Comment that says who raises it (spec 2.9's third column):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The errors of webcpp::crypto, in the category `webcpp.crypto`.

 @see "Errors", in the guide.
*/
#ifndef WEBCPP_CRYPTO_ERROR_HPP
#define WEBCPP_CRYPTO_ERROR_HPP

#include <webcpp/crypto/config.hpp>

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace webcpp::crypto {

/**
 Why an operation of webcpp::crypto failed.

 The values are fixed and never reused.

 @see "Errors", in the guide.
*/
enum class error : int {
    /** A key of another size than the algorithm's `key_size`, or a secret key not of 32 bytes. */
    invalid_key_size = 1,
    /** A GCM IV that is empty or longer than 128 bytes. */
    invalid_iv_size = 2,
    /** A tag given to `open` that is not of 16 bytes. */
    invalid_tag_size = 3,
    /** Data given to a block cipher that is not a whole number of blocks. */
    invalid_input_size = 4,
    /** A message or additional data longer than the algorithm allows. */
    message_too_long = 5,
    /** The tag does not match: the message or its additional data was changed, or the key differs. */
    authentication_failed = 6,
    /** An object moved from, or the incremental seal called out of order. */
    invalid_state = 7,
    /** The system's or the browser's generator failed. */
    entropy_unavailable = 8,
    /** A secret key that is zero or not below the group order. */
    invalid_secret_key = 9,
    /** A public key of the wrong size, or not a point of the curve. */
    invalid_public_key = 10,
    /** A signature that does not verify, or cannot be parsed. */
    invalid_signature = 11,
    /** A backend failed where its contract says it does not. */
    backend_failure = 12,
    // 13 and 14 are reserved for OpenSSL's memory option (spec 2.9: memory_functions_refused
    // and memory_functions_not_installed), which declares them; never reused.
};

namespace detail {

/** The message of a value of @ref error, or the unknown message for a value it does not define. */
inline const char* message_of(int value) noexcept {
    switch (static_cast<error>(value)) {
        case error::invalid_key_size: return "the key is not of the algorithm's size";
        case error::invalid_iv_size: return "the IV is not of a size the algorithm accepts";
        case error::invalid_tag_size: return "the tag is not of 16 bytes";
        case error::invalid_input_size: return "the data is not a whole number of blocks";
        case error::message_too_long: return "the message is longer than the algorithm allows";
        case error::authentication_failed: return "the authentication tag does not match";
        case error::invalid_state: return "the object was moved from, or the call is out of order";
        case error::entropy_unavailable: return "the system's generator of entropy failed";
        case error::invalid_secret_key:
            return "the secret key is zero or not below the group order";
        case error::invalid_public_key: return "the public key is not a point of the curve";
        case error::invalid_signature: return "the signature does not verify";
        case error::backend_failure: return "the backend failed where it should not";
    }
    return "unknown webcpp.crypto error";
}

/**
 A constant category with a fixed identity, so it holds no mutable state.

 @note The identity is the ASCII of "crypto" followed by two zero bytes.
*/
class category final : public boost::system::error_category {
public:
    /** Constructs the category with its fixed identity. */
    constexpr category() noexcept : boost::system::error_category(0x63727970746f0000ULL) {}

    /** The category's name, "webcpp.crypto". */
    const char* name() const noexcept override { return "webcpp.crypto"; }

    /** The message of the error `value`, as @ref message_of spells it. */
    std::string message(int value) const override { return message_of(value); }

    /**
     Copies the message of the error `value` into `buffer`, cut to fit `size` bytes with its
     terminating zero, and returns the whole message, a static text.
    */
    const char* message(int value, char* buffer, std::size_t size) const noexcept override {
        const char* text = message_of(value);
        if (buffer != nullptr && size != 0) {
            const std::string_view spelling(text);
            const std::size_t limit = size - 1;
            const std::size_t count = spelling.size() < limit ? spelling.size() : limit;
            std::memcpy(buffer, spelling.data(), count);
            buffer[count] = '\0';
        }
        return text;
    }
};

/** The one category of crypto's errors, which @ref error_category returns. */
inline constexpr category the_category{};

}  // namespace detail

/**
 The category of every @ref error.

 Its `name()` is "webcpp.crypto", and its `message(value)` the message of the error, or
 "unknown webcpp.crypto error" for a value @ref error does not define.

 @return The category, a constant with a fixed identity.
 @see "Errors", in the guide.
*/
inline const boost::system::error_category& error_category() noexcept {
    return detail::the_category;
}

/**
 Makes the `error_code` of an @ref error, as its implicit conversion does.

 @param e The error.
 @return `e` as an `error_code` of the category @ref error_category.
*/
inline boost::system::error_code make_error_code(error e) noexcept {
    return {static_cast<int>(e), error_category()};
}

}  // namespace webcpp::crypto

namespace boost::system {
template <>
struct is_error_code_enum<webcpp::crypto::error> : std::true_type {};
}  // namespace boost::system

#endif  // WEBCPP_CRYPTO_ERROR_HPP
```

  Add `#include <webcpp/crypto/error.hpp>` to `crypto.hpp`, `crypto/error.hpp` to the
  headers-alone call's list and `include/webcpp/crypto/error.hpp` to the `headers` of
  `meta/include-boundaries.json`; `doc/changelog.adoc` gains, under "Unreleased", `* Added
  the errors of crypto, with fixed values, in the category webcpp.crypto.`
- [ ] **Step 3: Run** the native and wasip2 lanes. Expected: PASS, `error` and
  `error_no_exceptions` among them.
- [ ] **Step 4: Commit:** `"crypto: the errors, their fixed values and their messages"`.

## Task 3: `equal`, and the wipe

**Files:**
- Create: `include/webcpp/crypto/equal.hpp`, `include/webcpp/crypto/detail/wipe.hpp`,
  `include/webcpp/crypto/detail/limits.hpp`, `test/equal_test.cpp`, `test/wipe_test.cpp`.
- Modify: `test/Jamfile`, `include/webcpp/crypto.hpp`.

**Interfaces:**
- Produces: `template <std::size_t N> bool webcpp::crypto::equal(boost::hash2::digest<N> const&, boost::hash2::digest<N> const&) noexcept`;
  `bool webcpp::crypto::equal(std::span<std::uint8_t const>, std::span<std::uint8_t const>) noexcept`;
  `void webcpp::crypto::detail::wipe(void* data, std::size_t size) noexcept`;
  `template <class T> void webcpp::crypto::detail::wipe_object(T& object) noexcept`;
  `bool webcpp::crypto::detail::exceeds(std::uint64_t value, std::uint64_t limit) noexcept`.

- [ ] **Step 1: Write the failing tests.** `test/equal_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// equal compares two digests, a digest and an array, or two spans, and tells them apart
// by any byte; spans of different sizes are unequal, and two empty spans are equal.

#include <webcpp/crypto/equal.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/hash2/digest.hpp>
#include <boost/hash2/hmac.hpp>
#include <boost/hash2/sha2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace crypto = webcpp::crypto;

int main() {
    boost::hash2::digest<16> a{};
    boost::hash2::digest<16> b{};
    BOOST_TEST(crypto::equal(a, b));
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            b = a;
            b.data()[i] ^= static_cast<unsigned char>(1U << bit);
            BOOST_TEST(!crypto::equal(a, b));
        }
    }
    std::array<std::uint8_t, 16> const array{};
    BOOST_TEST(crypto::equal(a, array));
    std::array<std::uint8_t, 15> const shorter{};
    BOOST_TEST(!crypto::equal(a, shorter));
    BOOST_TEST(crypto::equal(std::span<std::uint8_t const>(), std::span<std::uint8_t const>()));

    // Hash2's MAC, which equal serves too: two results of the same key and message agree.
    boost::hash2::hmac_sha2_256 first(reinterpret_cast<unsigned char const*>("key"), 3);
    boost::hash2::hmac_sha2_256 second(reinterpret_cast<unsigned char const*>("key"), 3);
    first.update("message", 7);
    second.update("message", 7);
    BOOST_TEST(crypto::equal(first.result(), second.result()));
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/wipe_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// detail::wipe zeroes every byte it is given and none around it; detail::exceeds compares
// in 64 bits on every target.

#include <webcpp/crypto/detail/limits.hpp>
#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace detail = webcpp::crypto::detail;

int main() {
    std::array<std::uint8_t, 34> bytes{};
    bytes.fill(0xa5);
    detail::wipe(bytes.data() + 1, 32);
    BOOST_TEST_EQ(bytes.front(), 0xa5);
    BOOST_TEST_EQ(bytes.back(), 0xa5);
    for (std::size_t i = 1; i < 33; ++i) {
        BOOST_TEST_EQ(bytes[i], 0);
    }
    struct schedule {
        std::array<std::uint32_t, 4> words;
    } object{{{1, 2, 3, 4}}};
    detail::wipe_object(object);
    BOOST_TEST_EQ(object.words[0] | object.words[1] | object.words[2] | object.words[3], 0U);
    detail::wipe(nullptr, 0);

    BOOST_TEST(detail::exceeds((std::uint64_t{1} << 36) - 31, (std::uint64_t{1} << 36) - 32));
    BOOST_TEST(!detail::exceeds((std::uint64_t{1} << 36) - 32, (std::uint64_t{1} << 36) - 32));
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile`: `crypto-run equal : equal_test.cpp ;` and `crypto-run wipe : wipe_test.cpp ;`.
  Run the native lane. Expected: FAIL, the headers not found.
- [ ] **Step 2: Write `include/webcpp/crypto/detail/wipe.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_WIPE_HPP
#define WEBCPP_CRYPTO_DETAIL_WIPE_HPP

#include <webcpp/crypto/config.hpp>

#include <atomic>
#include <cstddef>
#include <type_traits>

namespace webcpp::crypto::detail {

/**
 * Zeroes `size` bytes at `data` with stores the optimiser cannot drop.
 *
 * Tip: a `memset` before an object dies is a dead store a compiler may remove; stores through
 * a volatile pointer, then a signal fence, are kept (spec section 2.11).
 *
 * @param data The first byte, or null when `size` is 0.
 * @param size How many bytes.
 */
inline void wipe(void* data, std::size_t size) noexcept {
    auto* const bytes = static_cast<unsigned char volatile*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        bytes[i] = 0;
    }
    std::atomic_signal_fence(std::memory_order_seq_cst);
}

/**
 * Zeroes an object's bytes, as @ref wipe does.
 *
 * @tparam T A trivially copyable type, a C library's key schedule or context.
 * @param object The object.
 */
template <class T>
void wipe_object(T& object) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "only a C library's plain state is wiped");
    wipe(&object, sizeof object);
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_DETAIL_WIPE_HPP
```

  `include/webcpp/crypto/detail/limits.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_LIMITS_HPP
#define WEBCPP_CRYPTO_DETAIL_LIMITS_HPP

#include <webcpp/crypto/config.hpp>

#include <cstdint>

namespace webcpp::crypto::detail {

/**
 * Whether `value` is above `limit`, both in 64 bits.
 *
 * Tip: a `size_t` compared with a 64-bit limit is a tautology on wasm32, which clang reports
 * as an error under `<warnings-as-errors>on`; a function of two `uint64_t` is not.
 *
 * @param value The size, widened.
 * @param limit The limit.
 * @return `value > limit`.
 */
inline bool exceeds(std::uint64_t value, std::uint64_t limit) noexcept {
    return value > limit;
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_DETAIL_LIMITS_HPP
```

- [ ] **Step 3: Write `include/webcpp/crypto/equal.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `equal`, the constant-time comparison of tags and MACs, Boost.Hash2's included.
*/
#ifndef WEBCPP_CRYPTO_EQUAL_HPP
#define WEBCPP_CRYPTO_EQUAL_HPP

#include <webcpp/crypto/config.hpp>

#include <boost/hash2/digest.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace webcpp::crypto {

/**
 * Whether two byte sequences are equal, in a time that does not depend on their contents.
 *
 * Every byte is read, and no branch or index depends on one: the XOR of each pair is
 * accumulated with OR in a volatile local, read once after the loop, and reduced to one bit
 * without a branch, as BearSSL's tag check does. Sizes are public: spans of different sizes
 * compare unequal without reading their contents.
 *
 * @param a The first sequence, a tag or a MAC.
 * @param b The second.
 * @return Whether they have the same size and the same bytes.
 */
inline bool equal(std::span<std::uint8_t const> a, std::span<std::uint8_t const> b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    unsigned volatile accumulated = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        accumulated = accumulated | static_cast<unsigned>(a[i] ^ b[i]);
    }
    unsigned const all = accumulated;
    // all is at most 255: all - 1 wraps to the type's maximum only when all is 0.
    return (((all - 1U) >> 8U) & 1U) != 0;
}

/**
 * Whether two digests are equal, in a time that does not depend on their contents.
 *
 * Boost.Hash2's own `operator==` is `memcmp`, which is not constant time: a MAC is checked
 * with this one.
 *
 * @tparam N The digests' size in bytes.
 * @param a The first digest.
 * @param b The second.
 * @return Whether every byte is equal.
 */
template <std::size_t N>
bool equal(boost::hash2::digest<N> const& a, boost::hash2::digest<N> const& b) noexcept {
    return equal(std::span<std::uint8_t const>(a.data(), N),
                 std::span<std::uint8_t const>(b.data(), N));
}

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_EQUAL_HPP
```

  Add `equal.hpp` to `crypto.hpp`; the headers-alone list gains `crypto/equal.hpp`,
  `crypto/detail/wipe.hpp` and `crypto/detail/limits.hpp` (every `.hpp` under
  `webcpp/crypto/`, `detail/` included, needs a headers-alone unit, or the lint's compile
  database fails, `tools/lint/compile_commands.py`); `meta/include-boundaries.json`'s
  `headers` gains `include/webcpp/crypto/equal.hpp`; `doc/changelog.adoc` gains `* Added
  equal, a constant-time comparison of tags and Boost.Hash2 digests.`
- [ ] **Step 4: Run** the native, wasip2, wasip3 and emscripten lanes. Expected: PASS,
  `equal`, `wipe` and their `_no_exceptions` twins on each. If the digest-against-array call
  does not compile (a `digest<16>` not converting to `std::span<std::uint8_t const>`), the
  failure is the measurement against spec 2.6's reading of `hash2/digest.hpp`: add the
  overload `template <std::size_t N> bool equal(boost::hash2::digest<N> const&, std::span<std::uint8_t const>) noexcept`
  that forwards with `a.data()`, and record it in the ledger.
- [ ] **Step 5: Commit:** `"crypto: equal in constant time, and the wipe the optimiser keeps"`.

## Task 4: `random_device` and the entropy of each target

**Files:**
- Create: `include/webcpp/crypto/random_device.hpp`, `include/webcpp/crypto/detail/entropy.hpp`,
  `include/webcpp/crypto/detail/windows.hpp`, `test/random_device_test.cpp`,
  `test/random_device_failing_test.cpp`, `test/random_device_no_call_test.cpp`.
- Modify: `test/Jamfile`, `include/webcpp/crypto.hpp`, `meta/include-boundaries.json`,
  `doc/changelog.adoc`; (superproject) `tools/lint/lint.sh` (the rule `entropy sources`),
  `tools/lint/lint_test.py` (its case), `CODING-GUIDELINES.md` (chapter 9's table of rules),
  `doc/releases/0.1.0.adoc` (its line).

**Interfaces:**
- Consumes: `error::entropy_unavailable`, `detail::wipe`.
- Produces: `class webcpp::crypto::random_device` (spec 2.7);
  `boost::system::result<void> webcpp::crypto::detail::fill_from_system(std::span<std::uint8_t>) noexcept`;
  the test seam `WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY`: defined, the source is
  `extern "C" int webcpp_test_entropy(void* out, std::size_t size)`, which returns 0 or an
  `errno` value.

- [ ] **Step 1: Write the failing tests.** `test/random_device_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// random_device fills buffers of 0, 1, 256, 257, 65,536 and 65,537 bytes from the
// system's generator, two fills differ, and with exceptions it is a
// UniformRandomBitGenerator (spec sections 2.7 and 8.4).

#include <webcpp/crypto/random_device.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

namespace crypto = webcpp::crypto;

namespace {

void fills(std::size_t size) {
    crypto::random_device device;
    std::vector<std::uint8_t> first(size);
    std::vector<std::uint8_t> second(size);
    BOOST_TEST(device.fill(first));
    BOOST_TEST(device.fill(second));
    if (size >= 16) {
        BOOST_TEST(first != second);
        BOOST_TEST(std::any_of(first.begin(), first.end(), [](std::uint8_t b) { return b != 0; }));
    }
}

}  // namespace

int main() {
    for (std::size_t size : {0, 1, 256, 257, 65'536, 65'537}) {
        fills(size);
    }
    BOOST_TEST_EQ((crypto::random_device::min)(), 0U);
    BOOST_TEST_EQ((crypto::random_device::max)(), (std::numeric_limits<unsigned int>::max)());
    BOOST_TEST_EQ(crypto::random_device().entropy(),
                  static_cast<double>(std::numeric_limits<unsigned int>::digits));
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    static_assert(std::uniform_random_bit_generator<crypto::random_device>);
    crypto::random_device device;
    std::uniform_int_distribution<int> die(1, 6);
    int const roll = die(device);
    BOOST_TEST(roll >= 1 && roll <= 6);
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/random_device_failing_test.cpp`, the planted failing source of spec 8.4:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// With the source failing (the test seam WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY), fill returns
// entropy_unavailable and zeroes the whole buffer, a source that fails on the second
// piece of 256 included; ENOTSUP is not_supported, Emscripten's answer in an audio
// worklet; operator() throws the error with exceptions on.

#include <webcpp/crypto/random_device.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/system_error.hpp>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the planted source's script
int answer_after = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the planted source's script
int failure = EIO;

}  // namespace

extern "C" int webcpp_test_entropy(void* out, std::size_t size) {
    if (answer_after == 0) {
        return failure;
    }
    --answer_after;
    std::memset(out, 0x5a, size);
    return 0;
}

int main() {
    namespace crypto = webcpp::crypto;
    crypto::random_device device;
    std::vector<std::uint8_t> buffer(600, 0xff);

    answer_after = 0;
    auto filled = device.fill(buffer);
    BOOST_TEST(!filled && filled.error() == crypto::error::entropy_unavailable);
    BOOST_TEST(std::all_of(buffer.begin(), buffer.end(), [](std::uint8_t b) { return b == 0; }));

    std::fill(buffer.begin(), buffer.end(), 0xff);
    answer_after = 1;  // the first piece of 256 succeeds, the second fails
    filled = device.fill(buffer);
    BOOST_TEST(!filled && filled.error() == crypto::error::entropy_unavailable);
    BOOST_TEST(std::all_of(buffer.begin(), buffer.end(), [](std::uint8_t b) { return b == 0; }));

    answer_after = 0;
    failure = ENOTSUP;
    filled = device.fill(buffer);
    BOOST_TEST(!filled && filled.error() == boost::system::errc::not_supported);
    failure = EIO;

#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    answer_after = 0;
    bool threw = false;
    try {
        static_cast<void>(device());
    } catch (boost::system::system_error const& e) {
        threw = e.code() == crypto::error::entropy_unavailable;
    }
    BOOST_TEST(threw);
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/random_device_no_call_test.cpp`, which proves that without exceptions `operator()` is
  not declared:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Built with WEBCPP_CRYPTO_NO_EXCEPTIONS defined: random_device has no operator(), whose
// failure could not be reported, and fill is its interface.

#include <webcpp/crypto/random_device.hpp>

int main() {
    webcpp::crypto::random_device device;
    // expected-error@+1 {{type 'webcpp::crypto::random_device' does not provide a call operator}}
    return static_cast<int>(device());
}
```

  In `test/Jamfile`:

```jam
# random_device: the system's generator, and a planted failing source.
crypto-run random_device : random_device_test.cpp ;
crypto-run random_device_failing : random_device_failing_test.cpp
  : <define>WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY ;
webcpp.compile-diagnostic random_device_no_call : random_device_no_call_test.cpp
  : <define>WEBCPP_CRYPTO_NO_EXCEPTIONS : native wasip2 ;
```

  Run the native lane. Expected: FAIL, `random_device.hpp` not found.
- [ ] **Step 2: Write `include/webcpp/crypto/detail/windows.hpp`** (spec 2.2):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_WINDOWS_HPP
#define WEBCPP_CRYPTO_DETAIL_WINDOWS_HPP

#include <webcpp/crypto/config.hpp>

#ifdef _WIN32

// <windows.h> without NOMINMAX defines min and max as macros, which break every max()
// after it, and without WIN32_LEAN_AND_MEAN brings winsock.h, which clashes with a later
// winsock2.h (Boost.Asio's). Each is defined here only when the program has not, and
// undefined again only when defined here: a program that wants the macros includes
// <windows.h> itself first, and its include guard makes this one add nothing.
#ifndef NOMINMAX
#define NOMINMAX
#define WEBCPP_CRYPTO_DETAIL_DEFINED_NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define WEBCPP_CRYPTO_DETAIL_DEFINED_WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>  // lint-world: BCryptGenRandom and GetProcAddress, this header's purpose

#include <bcrypt.h>  // lint-world: BCryptGenRandom, Windows' one generator

#ifdef WEBCPP_CRYPTO_DETAIL_DEFINED_NOMINMAX
#undef NOMINMAX
#undef WEBCPP_CRYPTO_DETAIL_DEFINED_NOMINMAX
#endif
#ifdef WEBCPP_CRYPTO_DETAIL_DEFINED_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef WEBCPP_CRYPTO_DETAIL_DEFINED_WIN32_LEAN_AND_MEAN
#endif

#endif  // _WIN32

#endif  // WEBCPP_CRYPTO_DETAIL_WINDOWS_HPP
```

- [ ] **Step 3: Write `include/webcpp/crypto/detail/entropy.hpp`**, the whole function on one
  screen (DESIGN, 2026-10-11 "entropy, safest and readable"):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_ENTROPY_HPP
#define WEBCPP_CRYPTO_DETAIL_ENTROPY_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/crypto/detail/windows.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/system/errc.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <span>

#if defined(WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY)
// The test seam of spec section 8.4: a test defines the source, which returns 0 or an errno.
extern "C" int webcpp_test_entropy(void* out, std::size_t size);
#elif !defined(_WIN32)
#include <sys/random.h>  // lint-world: getentropy, the system's entropy, this header's purpose
#endif

namespace webcpp::crypto::detail {

/** The most one call of the system's source gives (POSIX.1-2024's `GETENTROPY_MAX`). */
inline constexpr std::size_t entropy_piece = 256;

/**
 * Asks the platform's one source for `size` bytes, at most @ref entropy_piece.
 *
 * Linux, macOS and WASI: `getentropy`, which fills the buffer or fails (lint-world: the source)
 * with `errno` set (POSIX.1-2024; wasmtime serves it with the host's generator). Emscripten:
 * the same call (lint-world: the source) becomes `crypto.getRandomValues` in the browser and
 * in node, and fails with `ENOTSUP` in
 * an audio worklet, which has no `crypto` (Emscripten's `src/lib/libwasi.js`). Windows:
 * `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG`, the system's generator
 * (learn.microsoft.com, `BCryptGenRandom`).
 *
 * @param out Where to write.
 * @param size How many bytes, 1 to @ref entropy_piece.
 * @return Nothing, `errc::not_supported` where there is no generator, or
 * `error::entropy_unavailable`.
 */
inline boost::system::result<void> read_entropy(std::uint8_t* out, std::size_t size) noexcept {
#if defined(WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY)
    int const failed = webcpp_test_entropy(out, size);
#elif defined(_WIN32)
    NTSTATUS const status = ::BCryptGenRandom(nullptr, out, static_cast<ULONG>(size),
                                              BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    int const failed = BCRYPT_SUCCESS(status) ? 0 : EIO;
#else
    int const failed = ::getentropy(out, size) == 0 ? 0 : errno;  // lint-world: the entropy
#endif
    if (failed == ENOTSUP) {
        return boost::system::errc::make_error_code(boost::system::errc::not_supported);
    }
    if (failed != 0) {
        return error::entropy_unavailable;
    }
    return {};
}

/**
 * Fills `out` from the platform's source, in pieces of at most @ref entropy_piece bytes.
 *
 * No generator of webcpp's or of a backend's stands between the source and `out`, and no
 * weaker source is ever tried: when the source fails, `out` is zeroed whole, so that nothing
 * the caller may use is left in it.
 *
 * @param out The bytes to fill.
 * @return Nothing, or the source's error.
 */
inline boost::system::result<void> fill_from_system(std::span<std::uint8_t> out) noexcept {
    for (std::size_t done = 0; done < out.size(); done += entropy_piece) {
        std::size_t const piece = (std::min)(out.size() - done, entropy_piece);
        auto const read = read_entropy(out.data() + done, piece);
        if (!read) {
            wipe(out.data(), out.size());
            return read.error();
        }
    }
    return {};
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_DETAIL_ENTROPY_HPP
```

- [ ] **Step 4: Write `include/webcpp/crypto/random_device.hpp`** with spec 2.7's
  declarations and these definitions. The world rule matches the class's name on any line
  (M7), so every line that names it ends with `lint-world:` and its reason, and the Doc
  Comments say "the random device" in words:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The random device: the system's or the browser's generator, shaped as the standard
 library's.
*/
#ifndef WEBCPP_CRYPTO_RANDOM_DEVICE_HPP
#define WEBCPP_CRYPTO_RANDOM_DEVICE_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/entropy.hpp>

#include <boost/system/result.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace webcpp::crypto {

/**
 * The operating system's or the browser's generator, and nothing else.
 *
 * Every call reads the source: there is no cache and no generator in between. It holds no
 * state and needs no backend. With exceptions it is a UniformRandomBitGenerator, as the
 * standard library's random device is; without them its interface is `fill`.
 *
 * @see "Entropy", in the guide.
 */
class random_device {  // lint-world: the system's entropy, this class's purpose
public:
    /** The standard library's random device's own type. */
    using result_type = unsigned int;

    /** Makes the device, which holds nothing. */
    random_device() noexcept = default;  // lint-world: reads the system's entropy
    /** Not copyable: the device holds nothing to copy. */
    random_device(random_device const&) = delete;  // lint-world: not copyable
    /** Not copyable: the device holds nothing to copy. */
    random_device& operator=(random_device const&) = delete;  // lint-world: not copyable

    /**
     * The smallest value `operator()` gives.
     *
     * @return 0.
     */
    static constexpr result_type(min)() noexcept { return 0; }

    /**
     * The largest value `operator()` gives.
     *
     * @return The largest `unsigned int`.
     */
    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

    /**
     * The entropy of a value, in bits: every bit is the system's.
     *
     * @return The digits of `result_type`.
     */
    double entropy() const noexcept {
        return static_cast<double>(std::numeric_limits<result_type>::digits);
    }

    /**
     * Fills `out` from the system's generator.
     *
     * @param out The bytes to fill; zeroed on error.
     * @return Nothing; `error::entropy_unavailable` when the source fails;
     * `errc::not_supported` where there is none (an Emscripten audio worklet).
     */
    boost::system::result<void> fill(std::span<std::uint8_t> out) noexcept {
        return detail::fill_from_system(out);
    }

#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    /**
     * A value from the system's generator, as the standard library's random device gives one.
     *
     * @return The value.
     * @throws boost::system::system_error carrying @ref fill's error.
     */
    result_type operator()() {
        std::array<std::uint8_t, sizeof(result_type)> bytes{};
        auto const filled = fill(bytes);
        if (!filled) {
            boost::throw_exception(boost::system::system_error(filled.error()));
        }
        result_type value = 0;
        std::memcpy(&value, bytes.data(), bytes.size());
        return value;
    }
#endif
};

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_RANDOM_DEVICE_HPP
```

  `crypto.hpp` includes it on the line
  `#include <webcpp/crypto/random_device.hpp>  // lint-world: the system's entropy, a purpose of crypto`,
  and the headers-alone list gains `crypto/random_device.hpp`, `crypto/detail/entropy.hpp`
  and `crypto/detail/windows.hpp`; `meta/include-boundaries.json`'s `headers` gains
  `include/webcpp/crypto/random_device.hpp`; `doc/changelog.adoc` gains `* Added
  random_device, which reads the system's or the browser's generator and nothing else.`
- [ ] **Step 5: Run** the four lanes. Expected: PASS: `random_device`,
  `random_device_failing` and their twins on every target; `random_device_no_call` on native
  and wasip2. On MSVC (the CI's lanes) the Windows branch compiles and fills.
- [ ] **Step 6: Write the lint rule** (spec 8.4's last item). Its failing case first,
  `test_entropy_sources` in `tools/lint/lint_test.py`, in the form of `test_world_rule`:
  `prepare(root)` on the fixture `demo`, then `write(root, header, ...)` of
  `header = 'libs/demo/include/webcpp/demo/entropy.hpp'`, a header that compiles alone (the
  licence notice, an include guard) and holds only comment lines, so that no other rule
  fails: `// Calls rand(). lint-world: a test` and `// Calls getentropy(p, 16). lint-world: a
  test`. Then `expect_alone(lint(root), 'entropy sources', [at(root, header, 'rand()')],
  spared=(at(root, header, 'getentropy'),))`: the lint prints the section
  `== entropy sources ==`, the `rand()` line with the suffix ": a header that reads entropy
  names rand, time, clock or __rdrand", and ends `lint: failed: entropy sources`; the
  `getentropy` line is not named. Run `python3 tools/lint/lint_test.py test_entropy_sources`.
  Expected: FAIL. Then in `tools/lint/lint.sh`, after rule 8 (its list of headers is
  NUL-separated, so the rule takes its own through `files_matching`):

```bash
# 8a. A header that reads entropy names no weaker source: no rand, time, clock or
#    __rdrand, even with lint-world:, in any header whose path ends entropy.hpp or
#    random_device.hpp (DESIGN, 2026-10-11 "entropy, safest and readable").
rule 'entropy sources'
files_matching '^(libs|tools/test/fixtures)/[^/]+/include/.*(entropy|random_device)\.hpp$' \
    > "${work_directory}/entropy_headers"
weak='(^|[^_[:alnum:]])(s?rand|time|clock|__rdrand(16|32|64)?_step|__rdrand)[[:space:]]*\('
weak_found="$(on_files "${work_directory}/entropy_headers" grep -nHE -e "${weak}" || true)"
if [ -n "${weak_found}" ]; then
    printf '%s\n' "${weak_found}" \
        | sed 's/$/: a header that reads entropy names rand, time, clock or __rdrand/'
    fail "a header that reads entropy names a weaker source"
else
    printf 'no header that reads entropy names a weaker source\n'
fi
```

  `CODING-GUIDELINES.md` chapter 9's table of the lint's rules gains, after the row
  `no clock, disk or network`, the row "entropy sources", whose "what fails" column reads: a
  library header whose path ends `entropy.hpp` or `random_device.hpp` that names `rand`,
  `srand`, `time`, `clock` or `__rdrand`, even with `lint-world:`;
  `doc/releases/0.1.0.adoc`, under "Tools and build", gains `* Added the lint rule entropy sources, which refuses a weaker source in a
  header that reads entropy.` Run the lint test again. Expected: PASS. Run the lint.
  Expected: `lint: clean`.
- [ ] **Step 7: Commit** (library) `"crypto: random_device on the system's one generator,
  in readable code"`; (superproject) `"tools: the lint refuses a weaker source in a header
  that reads entropy"` with `tools/lint/lint.sh tools/lint/lint_test.py CODING-GUIDELINES.md
  doc/releases/0.1.0.adoc`.

## Task 5: BearSSL from the mirror, file by file, with its canary

**Files:**
- Create (superproject): the submodule `third_party/libs/bearssl`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile`, `tools/test/third_party_test.py`
  (its cases of BearSSL), `CODING-GUIDELINES.md` (chapter 10, "Third-party code": the row
  `bearssl`), `doc/releases/0.1.0.adoc` ("Tools and build": a line).
- Create (library): `src/canary_bearssl.c`, `include/webcpp/crypto/detail/canary.h`,
  `test/c_suites/Jamfile`, `LICENSE-BEARSSL.txt`.
- Modify (library): `test/Jamfile` (`build-project c_suites ;`).

**Interfaces:**
- Produces: `/third_party//bearssl` (usage requirements: `<include>third_party/libs/bearssl/inc`,
  `WEBCPP_CRYPTO_HAS_BEARSSL=1`, the section garbage collection); the symbol
  `webcpp_crypto_bearssl_7bea48e_<getentropy|noseeder>__add_third_party_bearssl_to_your_link`;
  the macros of `detail/canary.h`: `WEBCPP_CRYPTO_DETAIL_CANARY(name, commit, configuration)`,
  `WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(name, commit, configuration)`,
  `WEBCPP_CRYPTO_BEARSSL_COMMIT` (`7bea48e`).

The superproject's pull request of this task compiles `libs/crypto/src/canary_bearssl.c`, and
the CI checks out the superproject's submodules alone: it lands only after G2 (the submodule
`libs/crypto` added by the owner), or carries that gitlink itself. The library's pull request
(the canary and the suite) goes first.

- [ ] **Step 1: Add the submodule** from the mirror, pinned, with a relative URL as webcpporg's
  other repositories have in `.gitmodules`:

```bash
git submodule add ../bearssl.git third_party/libs/bearssl
git -C third_party/libs/bearssl checkout 7bea48e5e850ab4cafbe68d3765cdaba13a86d6f
find third_party/libs/bearssl/src -name '*.c' | wc -l
```

  Expected: 294.
- [ ] **Step 2: Write the failing cases** in the existing `tools/test/third_party_test.py`
  (`harness.run_cases`, each case on a scratch superproject of its own): a scratch copy leaves
  `libs/` out and links `third_party/libs` to the checkout's (`tools/test/harness.py`,
  `copy_tree`), so each case first calls `harness.copy_integration_libraries(root, 'crypto')`,
  which copies crypto's `build.jam`, `include` and `src`, the canary among them, and the archive
  is built from the real BearSSL sources. The case builds
  `/third_party//bearssl` natively with `-a -d+2` and
  asserts that b2 compiled 295 C files (the 294 of `src/` and the canary; count the lines
  `clang -x c` or `gcc -x c` b2 prints at `-d+2`), that every one has
  `-ffunction-sections -fdata-sections`, `-DBR_RDRAND=0`, `-DBR_USE_URANDOM=0`,
  `-DBR_USE_GETENTROPY=1` and `-D_DEFAULT_SOURCE`, that none has `-std=`, and on macOS
  `-include sys/random.h`; then `nm` of the archive lists
  `webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link` (with the
  leading `_` on Mach-O). Run `python3 tools/test/third_party_test.py`. Expected: FAIL, no
  target `bearssl`.
- [ ] **Step 3: Write `include/webcpp/crypto/detail/canary.h`** (spec 4.3, readable as C and
  C++):

```c
/* Copyright (c) 2026 WebCpp.org
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * https://www.boost.org/LICENSE_1_0.txt)
 *
 * The link-time canaries of webcpp::crypto's backends, readable as C and C++. A backend's
 * header reads its canary inside every function that calls the C library, and webcpp's
 * src/canary_<name>.c, which /third_party//<name> compiles with the library's own flags,
 * defines it, so that a program linked without that library, or with one of another
 * commit or configuration, fails to link with the canary's name:
 *
 *     webcpp_crypto_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link
 *
 * On MSVC the same three facts also reach the linker as #pragma detect_mismatch. */
#ifndef WEBCPP_CRYPTO_DETAIL_CANARY_H
#define WEBCPP_CRYPTO_DETAIL_CANARY_H

/* The commit each library is pinned at, its first seven hex digits, which the CI compares
 * with `git -C third_party/libs/<name> rev-parse --short=7 HEAD`. A test of the canary
 * gives another through WEBCPP_CRYPTO_<NAME>_CANARY_COMMIT. */
#ifdef WEBCPP_CRYPTO_BEARSSL_CANARY_COMMIT
#define WEBCPP_CRYPTO_BEARSSL_COMMIT WEBCPP_CRYPTO_BEARSSL_CANARY_COMMIT
#else
#define WEBCPP_CRYPTO_BEARSSL_COMMIT 7bea48e
#endif

#define WEBCPP_CRYPTO_DETAIL_CANARY_I(name, commit, configuration) \
    webcpp_crypto_##name##_##commit##_##configuration##__add_third_party_##name##_to_your_link
#define WEBCPP_CRYPTO_DETAIL_CANARY(name, commit, configuration) \
    WEBCPP_CRYPTO_DETAIL_CANARY_I(name, commit, configuration)
#define WEBCPP_CRYPTO_DETAIL_STRING_I(x) #x
#define WEBCPP_CRYPTO_DETAIL_STRING(x) WEBCPP_CRYPTO_DETAIL_STRING_I(x)
#define WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(name, commit, configuration) \
    #name "-" WEBCPP_CRYPTO_DETAIL_STRING(commit) "-" WEBCPP_CRYPTO_DETAIL_STRING(configuration)

#endif
```

- [ ] **Step 4: Write `src/canary_bearssl.c`:**

```c
/* Copyright (c) 2026 WebCpp.org
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * https://www.boost.org/LICENSE_1_0.txt)
 *
 * The canary of the backend BearSSL, compiled into /third_party//bearssl with BearSSL's own
 * defines. Its configuration is BearSSL's seeder as those defines make it: getentropy, the
 * one source, with RDRAND and /dev/urandom off; noseeder, every seeder off (Windows); and
 * unconfigured for any other build, which no header expects. */
#include <webcpp/crypto/detail/canary.h>

#if defined(BR_USE_GETENTROPY) && BR_USE_GETENTROPY == 1 && defined(BR_RDRAND) && \
    BR_RDRAND == 0 && defined(BR_USE_URANDOM) && BR_USE_URANDOM == 0
#define WEBCPP_CRYPTO_BEARSSL_BUILT getentropy
#elif defined(BR_USE_GETENTROPY) && BR_USE_GETENTROPY == 0 && defined(BR_RDRAND) && \
    BR_RDRAND == 0 && defined(BR_USE_URANDOM) && BR_USE_URANDOM == 0 && \
    defined(BR_USE_WIN32_RAND) && BR_USE_WIN32_RAND == 0
#define WEBCPP_CRYPTO_BEARSSL_BUILT noseeder
#else
#define WEBCPP_CRYPTO_BEARSSL_BUILT unconfigured
#endif

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_backend_bearssl", \
    WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(bearssl, WEBCPP_CRYPTO_BEARSSL_COMMIT, WEBCPP_CRYPTO_BEARSSL_BUILT))
#endif

extern const char WEBCPP_CRYPTO_DETAIL_CANARY(bearssl, WEBCPP_CRYPTO_BEARSSL_COMMIT,
                                              WEBCPP_CRYPTO_BEARSSL_BUILT);
const char WEBCPP_CRYPTO_DETAIL_CANARY(bearssl, WEBCPP_CRYPTO_BEARSSL_COMMIT,
                                       WEBCPP_CRYPTO_BEARSSL_BUILT) = 0;
```

- [ ] **Step 5: Write the target** in `third_party/Jamfile`. After the header's existing
  paragraph, which ends "a build that needs none never looks" (extended with one sentence:
  "The C libraries of webcpp::crypto's backends are built here from their own sources, file by
  file."), the targets of the existing form, each with a `<name>-checked-out` rule of its own:

```jam
# The C libraries of webcpp::crypto's backends: each a static library compiled
# file by file in the library's own C dialect (no -std), every function and
# datum in a section of its own, so that a program keeps only what it calls
# (CODING-GUIDELINES.md chapter 5, "Bringing a C library into webcpp"). Upstream's
# warnings are not ours. webcpp's libs/crypto/src/canary_<name>.c joins each:
# it defines the symbol the backend's header reads, which names the commit
# and the configuration.
path-constant CRYPTO : ../libs/crypto ;

# The canaries and their header are webcpp::crypto's: a checkout without
# libs/crypto stops the build of a C library of crypto's, naming the command,
# and leaves the other targets of this Jamfile as they are.
rule crypto-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(CRYPTO)/src/canary_bearssl.c ] ]
    {
        errors.user-error "libs/crypto is not checked out: it holds no"
            "src/canary_bearssl.c, which the C libraries of webcpp::crypto's backends"
            "are built with. Check it out with `git submodule update --init libs/crypto`." ;
    }
}

.c-library = <link>static <warnings>off <warnings-as-errors>off
    <conditional>@crypto-checked-out
    <toolset>clang:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>gcc:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>emscripten:<cflags>"-ffunction-sections -fdata-sections"
    <toolset>msvc:<cflags>"/Gy /Gw"
    <include>$(CRYPTO)/include ;

# What a program that links one of them needs: the linker's removal of unused
# sections, which wasm-ld, Emscripten and MSVC's release link do by default
# (link.exe leaves /OPT:REF off under /DEBUG, so it is asked for).
.gc = <target-os>darwin:<linkflags>-Wl,-dead_strip
    <target-os>linux:<linkflags>-Wl,--gc-sections
    <toolset>msvc:<linkflags>/OPT:REF ;

# BearSSL (MIT), from webcpp's unmodified mirror webcpporg/bearssl. Every .c
# of src/, the POWER8 ones too, which off POWER compile to the stubs other
# files call. One source of entropy, getentropy, with RDRAND and /dev/urandom
# off (libs/crypto's random_device reads the same source): wasi-libc and
# Emscripten declare getentropy only under _DEFAULT_SOURCE, and macOS only in
# <sys/random.h>, which src/rand/sysrng.c does not include. On Windows
# BearSSL's only seeder is CryptGenRandom, not the system's BCryptGenRandom,
# so every seeder is off there and BearSSL has none.
path-constant BEARSSL : libs/bearssl ;

rule bearssl-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(BEARSSL)/inc/bearssl.h ] ]
    {
        errors.user-error "third_party/libs/bearssl is not checked out: it holds no"
            "inc/bearssl.h. Check it out with `git submodule update --init"
            "third_party/libs/bearssl`." ;
    }
}

lib bearssl
  : [ glob libs/bearssl/src/*.c libs/bearssl/src/*/*.c ] $(CRYPTO)/src/canary_bearssl.c
  : $(.c-library) <conditional>@bearssl-checked-out
    <include>libs/bearssl/inc <include>libs/bearssl/src
    <define>BR_RDRAND=0 <define>BR_USE_URANDOM=0
    <target-os>windows:<define>BR_USE_GETENTROPY=0
    <target-os>windows:<define>BR_USE_WIN32_RAND=0
    <toolset>gcc:<define>BR_USE_GETENTROPY=1 <toolset>gcc:<define>_DEFAULT_SOURCE
    <toolset>clang:<define>BR_USE_GETENTROPY=1 <toolset>clang:<define>_DEFAULT_SOURCE
    <toolset>emscripten:<define>BR_USE_GETENTROPY=1
    <toolset>emscripten:<define>_DEFAULT_SOURCE
    <target-os>darwin:<cflags>"-include sys/random.h"
  : : <include>libs/bearssl/inc <define>WEBCPP_CRYPTO_HAS_BEARSSL=1 $(.gc) ;
explicit bearssl ;
```

  On Windows with clang (none in the matrix) the gcc and clang lines would also give
  `BR_USE_GETENTROPY=1`; the MSVC lanes are the Windows ones, and a `msvc` toolset gets the
  `windows` lines alone. Step 7 checks the canary's name on every lane.
- [ ] **Step 6: Write BearSSL's own suite** (spec 8.1, "the C libraries' own suites",
  CODING-GUIDELINES.md chapter 5, rule 11) in `test/c_suites/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The C libraries' own test programs, built against webcpp's build of each, on
# every target (CODING-GUIDELINES.md chapter 5, rule 11). BearSSL's 88 tests take 28 to
# 46 s per target, so they are the own lane c-suites, outside the ordinary
# lanes.
import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

path-constant BEARSSL : ../../../../third_party/libs/bearssl ;
webcpp.run bearssl_test_crypto : $(BEARSSL)/test/test_crypto.c
  : <library>/third_party//bearssl <include>$(BEARSSL)/src <warnings>off
    <warnings-as-errors>off <toolset>emscripten:<linkflags>-sSTACK_SIZE=1048576 ;

# The lane names its targets, so that the CI runs it on each (an own lane that
# names none runs natively alone, tools/webcpp.jam's lane rule).
webcpp.lane c-suites : bearssl_test_crypto : native wasip2 wasip3 emscripten ;
```

  In `test/Jamfile`, `build-project c_suites ;`. `LICENSE-BEARSSL.txt` is a copy of
  `third_party/libs/bearssl/LICENSE.txt`, which the README names.
- [ ] **Step 7: Run** `python3 tools/test/third_party_test.py`. Expected: PASS. Then the own
  lane on each target, e.g.
  `$B2 -a --build-dir=bin/lane-c-suites-wasip2 toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test/c_suites//c-suites`
  (the lane is declared in `test/c_suites/Jamfile`, so its target is that directory's; WASI
  runs through `tools/boost/asio-<target>/run.sh`, as `tools/ci/matrix.py`'s `wasm_lane` does).
  Expected: PASS, `bearssl_test_crypto`'s output ending "All tests OK." (its own last line) on
  native, wasip2, wasip3 and emscripten. A test of BearSSL's that fails on a target is
  investigated as a build difference (a define, a dialect) before anything else; BearSSL's
  code is never changed. The Emscripten stack size is measured: start without
  `-sSTACK_SIZE`, and keep the line only if the run overflows Emscripten's 64 KiB default,
  with the measured size.
- [ ] **Step 8: Document and commit.** `CODING-GUIDELINES.md` chapter 10, "Third-party code",
  gains the row `bearssl`: "BearSSL from webcpp's unmodified mirror `webcpporg/bearssl` at
  `7bea48e`; a static library built file by file by `third_party/Jamfile` with
  webcpp::crypto's canary, on every target; `WEBCPP_CRYPTO_HAS_BEARSSL=1` | MIT".
  `doc/releases/0.1.0.adoc`, "Tools and build", gains a line: BearSSL, built from source as
  `/third_party//bearssl`. Commit (superproject)
  `"third_party: BearSSL from the mirror webcpporg/bearssl, built file by file with webcpp::crypto's canary"`
  (`.gitmodules third_party/libs/bearssl third_party/Jamfile tools/test/third_party_test.py
  CODING-GUIDELINES.md doc/releases/0.1.0.adoc`);
  (library) `"crypto: BearSSL's canary, and its own test suite on webcpp's build"`.

## Task 6: The configuration's refusals, the default backend, and BearSSL's three layers

**Files:**
- Modify: `include/webcpp/crypto/config.hpp`.
- Create: `include/webcpp/crypto/backends/bearssl.hpp` (its check and canary only; Task 7 adds
  its engines), `include/webcpp/crypto/detail/engines.hpp`,
  `test/messages/<case>/Jamfile` and `<case>.cpp` for the cases of Step 1.
- Modify: `test/Jamfile` (`build-project messages ;`), `test/messages/Jamfile`.

**Interfaces:**
- Produces: `webcpp::crypto::default_backend`; the macros `WEBCPP_CRYPTO_DETAIL_DEFAULT_<NAME>`
  and `WEBCPP_CRYPTO_DETAIL_NONE_LINKED`; the primary templates
  `template <class Backend, std::size_t KeySize> struct block_state;`,
  `block_engine`, `gcm_state`, `gcm_engine` (same parameters) and
  `template <class Backend> struct chacha_state;`, `chacha_engine` (declared, never defined);
  `struct webcpp::crypto::backends::bearssl { static constexpr std::string_view name = "bearssl"; };`;
  `void webcpp::crypto::detail::read_canary(char const volatile&) noexcept`.

- [ ] **Step 1: Write the failing message tests,** one directory each (spec 4.3's pitfall),
  each a `webcpp.compile-diagnostic` whose source states the exact text. The cases and their
  requirements:

| Directory | Requirements | Targets | Expected error, stated on the header that raises it |
| --- | --- | --- | --- |
| `no_backend_wasm` | none | wasip2 wasip3 | spec 4.3's first row, the whole text |
| `no_backend_native` | none | native | the second `#error` of Step 4 (spec 4.3's second row, with `/webcpp/crypto//openssl`) |
| `bearssl_not_linked` | `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` | native wasip2 | the third `#error` of Step 4 |
| `openssl_only_on_wasi` | `<define>WEBCPP_CRYPTO_HAS_OPENSSL=1` | wasip2 wasip3 | the third `#error` of Step 4: BearSSL is WebAssembly's default, and it names the define that selects OpenSSL |
| `two_defaults` | `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` | native | spec 4.3's ninth row |

  OpenSSL is a backend on native, wasip2 and wasip3, where the provider builds it
  (CODING-GUIDELINES.md chapter 5, rule 7), and `config.hpp` refuses it on Emscripten alone.
  The two refusals on Emscripten (spec 4.3's seventh and eighth rows, Emscripten's text) are not
  cases here: `webcpp.compile-diagnostic` is clang's alone and skips the toolset `emscripten`,
  so they are fixtures of `tools/test/crypto_jamfiles_test.py` (Task 10). A positive case,
  `openssl_on_wasi`, a `webcpp.compile` on wasip2 and wasip3 with
  `<define>WEBCPP_CRYPTO_HAS_OPENSSL=1 <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` that
  includes `<webcpp/crypto/config.hpp>`, must compile.

  Each source includes `<webcpp/crypto/backends/bearssl.hpp>` (the first four, through the
  default's header) or `<webcpp/crypto/config.hpp>` (`two_defaults`), and states its error on the
  header that raises it, as wasi's tests do (`libs/wasi/test/neither_target_refused.cpp`):
  `// expected-error@webcpp/crypto/backends/bearssl.hpp:* {{...}}` for the first four,
  `// expected-error@webcpp/crypto/config.hpp:* {{...}}` for `two_defaults`, each with the whole
  text. `no_backend_wasm.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A program for WebAssembly that links no backend and uses the default's header stops
// with the instruction to add BearSSL (spec section 4.3).

// expected-error@webcpp/crypto/backends/bearssl.hpp:* {{webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)}}
#include <webcpp/crypto/backends/bearssl.hpp>

int main() {
    return 0;
}
```

  `test/messages/no_backend_wasm/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/crypto//crypto ;

import webcpp ;

webcpp.compile-diagnostic no_backend_wasm : no_backend_wasm.cpp : : wasip2 wasip3 ;
```

  `test/messages/Jamfile` holds `build-project <case> ;` for each directory, and
  `test/Jamfile` `build-project messages ;`. Run the native and wasip2 lanes. Expected:
  FAIL, each case's header missing or its message absent.
- [ ] **Step 2: Complete `config.hpp`.** After `WEBCPP_CRYPTO_WEBASSEMBLY`:

```cpp
// The refusals of impossible combinations (spec section 4.3). Clang and gcc go on after an
// #error, so the first error is the instruction; MSVC stops at it. OpenSSL is built for
// native, wasip2 and wasip3 (/webcpp/crypto//openssl), never for Emscripten; with b2,
// /webcpp/crypto//openssl stops an Emscripten build first, and these say it without b2.
#if defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#error "webcpp::crypto: OpenSSL is not built for Emscripten. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl."
#endif
#if defined(WEBCPP_TARGET_EMSCRIPTEN) && defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL)
#error "webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL selects OpenSSL, which is not built for Emscripten. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL."
#endif
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL) && defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL)
#error "webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL are defined. Keep one."
#endif

// The default backend of the AEADs (spec section 5): the one the program selects; else
// natively the first linked of OpenSSL, BearSSL; else BearSSL, whose header then says that
// no backend is linked when neither is, and that BearSSL is not linked when OpenSSL alone is
// (on WebAssembly, where the program selects OpenSSL with its define).
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL 1
#elif defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BEARSSL 1
#elif !defined(WEBCPP_CRYPTO_WEBASSEMBLY) && defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL 1
#else
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BEARSSL 1
#if !defined(WEBCPP_CRYPTO_HAS_BEARSSL) && !defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_NONE_LINKED 1
#endif
#endif
```

  and, after the backends' forward declarations, inside `namespace webcpp::crypto`:

```cpp
/** The backend of `aes256<>`, `gcm<aes256<>>` and `aes256_gcm`, as section 5 chooses it. */
#if defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL)
using default_backend = backends::openssl;
#else
using default_backend = backends::bearssl;
#endif
```

  Each macro the program may define (`WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`,
  `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`, `WEBCPP_CRYPTO_HAS_<NAME>`) gets a Doc Comment under `#ifdef __MRDOCS__`, defined and
  undefined at once, as `WEBCPP_CRYPTO_NO_EXCEPTIONS`'s is.
- [ ] **Step 3: Write `include/webcpp/crypto/detail/engines.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_ENGINES_HPP
#define WEBCPP_CRYPTO_DETAIL_ENGINES_HPP

#include <webcpp/crypto/config.hpp>

#include <cstddef>

namespace webcpp::crypto::detail {

/**
 * What a backend's header specialises, per algorithm: a state, which an object stores and
 * which never refuses, and an engine, used only inside the algorithm's member functions,
 * whose specialisation for a backend without the algorithm holds the `static_assert` that
 * names the backends that have it. A backend whose header is not included leaves both
 * undefined, and the compiler names the specialisation it lacks.
 *
 * Tip: `gcm<aes256<backends::bearssl>>` reads `aes256<backends::bearssl>::key_size`, which
 * instantiates the class and its members' types, so a refusal in the block cipher's state
 * would refuse BearSSL's GCM too.
 */
template <class Backend, std::size_t KeySize>
struct block_state;
/** The work of the AES block cipher on a backend (see @ref block_state). */
template <class Backend, std::size_t KeySize>
struct block_engine;
/** What a `gcm` stores (see @ref block_state). */
template <class Backend, std::size_t KeySize>
struct gcm_state;
/** The work of GCM on a backend (see @ref block_state). */
template <class Backend, std::size_t KeySize>
struct gcm_engine;
/** What a `chacha20_poly1305` stores (see @ref block_state). */
template <class Backend>
struct chacha_state;
/** The work of ChaCha20-Poly1305 on a backend (see @ref block_state). */
template <class Backend>
struct chacha_engine;

/**
 * Reads a backend's canary with a volatile load, which no linker can drop: a program built
 * without the backend's library, or with one of another commit or configuration, then
 * fails to link, naming the canary (spec section 4.3).
 *
 * @param canary The canary's symbol.
 */
inline void read_canary(char const volatile& canary) noexcept {
    static_cast<void>(canary);
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_DETAIL_ENGINES_HPP
```

  `static_cast<void>` of a volatile glvalue is a read in C++20 ([expr.static.cast]/6 and
  [conv.lval] apply to a discarded volatile glvalue, [expr.context]/2): the load happens.
- [ ] **Step 4: Write the first part of `backends/bearssl.hpp`:** layer 1, layer 2's symbol
  and layer 3:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The backend BearSSL: BearSSL's own C code, built from its sources by
 `/third_party//bearssl`. It serves `gcm<aes128/192/256<backends::bearssl>>`, and has no
 block cipher.

 @see "The backends", in the guide.
*/
#ifndef WEBCPP_CRYPTO_BACKENDS_BEARSSL_HPP
#define WEBCPP_CRYPTO_BACKENDS_BEARSSL_HPP

#include <webcpp/crypto/config.hpp>

#if !defined(WEBCPP_CRYPTO_HAS_BEARSSL)
#if defined(WEBCPP_CRYPTO_DETAIL_NONE_LINKED) && defined(WEBCPP_CRYPTO_WEBASSEMBLY)
#error "webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"
#elif defined(WEBCPP_CRYPTO_DETAIL_NONE_LINKED)
#error "webcpp::crypto: no backend is linked. In the program's Jamfile, add /webcpp/crypto//openssl (OpenSSL built from source) or /third_party//bearssl (built from source) beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ; (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto, or define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"
#else
#error "webcpp::crypto: the backend BearSSL is used and not linked. In the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, or, with /webcpp/crypto//openssl linked, select OpenSSL with <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL. (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"
#endif
#else

#include <webcpp/crypto/detail/canary.h>
#include <webcpp/crypto/detail/engines.hpp>

#include <bearssl.h>

#include <string_view>

// The configuration the canary names: BearSSL's seeder as third_party/Jamfile builds it.
#if defined(WEBCPP_CRYPTO_BEARSSL_CANARY_CONFIGURATION)
#define WEBCPP_CRYPTO_DETAIL_BEARSSL_CONFIGURATION WEBCPP_CRYPTO_BEARSSL_CANARY_CONFIGURATION
#elif defined(_WIN32)
#define WEBCPP_CRYPTO_DETAIL_BEARSSL_CONFIGURATION noseeder
#else
#define WEBCPP_CRYPTO_DETAIL_BEARSSL_CONFIGURATION getentropy  // lint-world: a canary's name
#endif
#define WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY                                          \
    WEBCPP_CRYPTO_DETAIL_CANARY(bearssl, WEBCPP_CRYPTO_BEARSSL_COMMIT, \
                                WEBCPP_CRYPTO_DETAIL_BEARSSL_CONFIGURATION)

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_backend_bearssl",                                 \
                        WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(bearssl,                     \
                                                            WEBCPP_CRYPTO_BEARSSL_COMMIT, \
                                                            WEBCPP_CRYPTO_DETAIL_BEARSSL_CONFIGURATION))
#endif

extern "C" const char WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY;

namespace webcpp::crypto::backends {

/** The backend BearSSL (spec section 3.2). */
struct bearssl {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "bearssl";
};

}  // namespace webcpp::crypto::backends

#endif  // WEBCPP_CRYPTO_HAS_BEARSSL
#endif  // WEBCPP_CRYPTO_BACKENDS_BEARSSL_HPP
```

- [ ] **Step 5: Run** the native and wasip2 lanes. Expected: PASS, each case of Step 1 on its
  targets, and the headers alone (`alone-crypto-backends-bearssl` gets
  `<library>/third_party//bearssl` in a headers-alone call of its own:
  `webcpp.headers-alone crypto : ../include : crypto/backends/bearssl.hpp : <library>/third_party//bearssl ;`;
  `detail/canary.h` is not a public `.hpp`, and naming it would stop the Jamfile (each glob of
  `only` must match a public header, I16): it is compiled through every backend header that
  includes it; `detail/engines.hpp`,
  `detail/entropy.hpp`, `detail/limits.hpp`, `detail/wipe.hpp` and `detail/windows.hpp` join
  the first call's list).
- [ ] **Step 6: Commit:** `"crypto: the configuration's refusals and the default backend,
  and BearSSL's three layers"`.

## Task 7: The block cipher's shape, and GCM one shot on BearSSL

**Files:**
- Create: `include/webcpp/crypto/aes.hpp`, `include/webcpp/crypto/gcm.hpp`,
  `test/support/hex.hpp`, `test/support/gcm_spec_vectors.hpp`, `test/support/gcm_checks.hpp`,
  `test/gcm_bearssl_test.cpp`, `test/messages/aes_on_bearssl/` (Jamfile and source).
- Modify: `include/webcpp/crypto/backends/bearssl.hpp` (the engines), `test/Jamfile`,
  `test/messages/Jamfile`, `include/webcpp/crypto.hpp`.

**Interfaces:**
- Consumes: `detail::{block,gcm}_{state,engine}`, `detail::read_canary`, `detail::wipe`,
  `detail::exceeds`, `error`, `equal`.
- Produces: `aes128<Backend>`, `aes192<Backend>`, `aes256<Backend>` (spec 2.3) over
  `detail::basic_aes<KeySize, Backend>`; `gcm<Cipher>` (spec 2.4) with `seal` and `open`;
  for a backend, the engine contract every later backend follows:

```cpp
// A backend B with GCM specialises, for KeySize 16, 24 and 32:
template <> struct gcm_state<B, KeySize> { /* trivially movable through the engine */ };
template <> struct gcm_engine<B, KeySize> {
    static boost::system::result<void> init(gcm_state<B, KeySize>&, std::span<std::uint8_t const> key) noexcept;
    static boost::system::result<void> start(gcm_state<B, KeySize>&, bool sealing,
        std::span<std::uint8_t const> iv, std::span<std::uint8_t const> additional_data) noexcept;
    static boost::system::result<void> run(gcm_state<B, KeySize>&, std::span<std::uint8_t> data) noexcept;
    static boost::system::result<void> tag(gcm_state<B, KeySize>&, boost::hash2::digest<16>& out) noexcept;
    static boost::system::result<void> check(gcm_state<B, KeySize>&, std::array<std::uint8_t, 16> const& tag) noexcept;
    static void move(gcm_state<B, KeySize>& to, gcm_state<B, KeySize>& from) noexcept;  // to is empty
    static void reset(gcm_state<B, KeySize>&) noexcept;  // wipes and releases; the state is empty
};
// check returns error::authentication_failed or error::backend_failure; start and run may
// return backend_failure; init may return not_enough_memory or backend_failure.
```

- [ ] **Step 1: Write the test support.** `test/support/hex.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_HEX_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_HEX_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace webcpp::test {

/** The bytes a string of hex digits writes, two per byte; a test's vectors are well formed. */
inline std::vector<std::uint8_t> from_hex(std::string_view hex) {
    auto const digit = [](char c) -> std::uint8_t {
        return static_cast<std::uint8_t>(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
    };
    std::vector<std::uint8_t> bytes(hex.size() / 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(digit(hex[2 * i]) << 4U | digit(hex[2 * i + 1]));
    }
    return bytes;
}

/** Lower-case hex of bytes, for a failure's message. */
inline std::string to_hex(std::span<std::uint8_t const> bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string hex;
    for (std::uint8_t b : bytes) {
        hex += digits[b >> 4U];
        hex += digits[b & 15U];
    }
    return hex;
}

}  // namespace webcpp::test

#endif
```

  `test/support/gcm_spec_vectors.hpp`, the 18 test cases of the GCM specification (McGrew and
  Viega, appendix B) as BearSSL's `test/test_crypto.c` transcribes them (`KAT_GCM`, its lines
  7313-7442 at the pin; order key, plaintext, additional data, IV, ciphertext, tag):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_GCM_SPEC_VECTORS_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_GCM_SPEC_VECTORS_HPP

#include <array>
#include <string_view>

namespace webcpp::test {

/** One case of the GCM specification, in hex. */
struct gcm_case {
    std::string_view key, plaintext, additional_data, iv, ciphertext, tag;
};

/** The 18 cases of the GCM specification (McGrew and Viega, appendix B), 1 to 18 in order. */
inline constexpr std::array<gcm_case, 18> gcm_spec_cases{{
    {"00000000000000000000000000000000", "", "", "000000000000000000000000", "",
     "58e2fccefa7e3061367f1d57a4e7455a"},
    {"00000000000000000000000000000000", "00000000000000000000000000000000", "",
     "000000000000000000000000", "0388dace60b6a392f328c2b971b2fe78",
     "ab6e47d42cec13bdf53a67b21257bddf"},
    {"feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b391aafd255",
     "", "cafebabefacedbaddecaf888",
     "42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091473f5985",
     "4d5c2af327cd64a62cf35abd2ba6fab4"},
    {"feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbaddecaf888",
     "42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091",
     "5bc94fbc3221a5db94fae95ae7121a47"},
    {"feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbad",
     "61353b4c2806934a777ff51fa22a4755699b2a714fcdc6f83766e5f97b6c742373806900e49f24b22b097544d4896b424989b5e1ebac0f07c23f4598",
     "3612d2e79e3b0785561be14aaca2fccb"},
    {"feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2",
     "9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b",
     "8ce24998625615b603a033aca13fb894be9112a5c3a211a8ba262a3cca7e2ca701e4a9a4fba43c90ccdcb281d48c7c6fd62875d2aca417034c34aee5",
     "619cc5aefffe0bfa462af43c1699d050"},
    {"000000000000000000000000000000000000000000000000", "", "", "000000000000000000000000",
     "", "cd33b28ac773f74ba00ed1f312572435"},
    {"000000000000000000000000000000000000000000000000", "00000000000000000000000000000000",
     "", "000000000000000000000000", "98e7247c07f0fe411c267e4384b0f600",
     "2ff58d80033927ab8ef4d4587514f0fb"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b391aafd255",
     "", "cafebabefacedbaddecaf888",
     "3980ca0b3c00e841eb06fac4872a2757859e1ceaa6efd984628593b40ca1e19c7d773d00c144c525ac619d18c84a3f4718e2448b2fe324d9ccda2710acade256",
     "9924a7c8587336bfb118024db8674a14"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbaddecaf888",
     "3980ca0b3c00e841eb06fac4872a2757859e1ceaa6efd984628593b40ca1e19c7d773d00c144c525ac619d18c84a3f4718e2448b2fe324d9ccda2710",
     "2519498e80f1478f37ba55bd6d27618c"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbad",
     "0f10f599ae14a154ed24b36e25324db8c566632ef2bbb34f8347280fc4507057fddc29df9a471f75c66541d4d4dad1c9e93a19a58e8b473fa0f062f7",
     "65dcc57fcf623a24094fcca40d3533f8"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2",
     "9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b",
     "d27e88681ce3243c4830165a8fdcf9ff1de9a1d8e6b447ef6ef7b79828666e4581e79012af34ddd9e2f037589b292db3e67c036745fa22e7e9b7373b",
     "dcf566ff291c25bbb8568fc3d376a6d9"},
    {"0000000000000000000000000000000000000000000000000000000000000000", "", "",
     "000000000000000000000000", "", "530f8afbc74536b9a963b4f1c4cb738b"},
    {"0000000000000000000000000000000000000000000000000000000000000000",
     "00000000000000000000000000000000", "", "000000000000000000000000",
     "cea7403d4d606b6e074ec5d3baf39d18", "d0d1c8a799996bf0265b98b5d48ab919"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b391aafd255",
     "", "cafebabefacedbaddecaf888",
     "522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662898015ad",
     "b094dac5d93471bdec1a502270e3cc6c"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbaddecaf888",
     "522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662",
     "76fc6ece0f4e1768cddf8853bb2d551b"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2", "cafebabefacedbad",
     "c3762df1ca787d32ae47c13bf19844cbaf1ae14d0b976afac52ff7d79bba9de0feb582d33934a4f0954cc2363bc73f7862ac430e64abe499f47c9b1f",
     "3a337dbf46a792c45e454913fe2ea8f2"},
    {"feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308",
     "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39",
     "feedfacedeadbeeffeedfacedeadbeefabaddad2",
     "9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b",
     "5a8def2f0c9e53f1f75d7853659e2a20eeb2b22aafde6419a058ab4f6f746bf40fc0c3b780f244452da3ebf1c5d82cdea2418997200ef82e44ae7e3f",
     "a44a8266ee1c8eb0c8b5d4cf5ae9f19a"},
}};

}  // namespace webcpp::test

#endif
```

- [ ] **Step 2: Write the checks,** templated on the backend, in `test/support/gcm_checks.hpp`;
  each backend's test calls them. They are written here in full; Task 8 adds the incremental
  ones to the same file.

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The checks of gcm on one backend: the GCM specification's 18 cases, every guard and
// its order, tampering with every byte, the zeroing of a failed open, moved-from
// objects, and aliasing spans (spec sections 2.4 and 8.1; Review Focus 2 and 3).

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_GCM_CHECKS_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_GCM_CHECKS_HPP

#include "gcm_spec_vectors.hpp"
#include "hex.hpp"

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/error.hpp>
#include <webcpp/crypto/gcm.hpp>

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace webcpp::test {

namespace crypto = webcpp::crypto;

/** Whether every byte is zero. */
inline bool all_zero(std::span<std::uint8_t const> bytes) {
    return std::all_of(bytes.begin(), bytes.end(), [](std::uint8_t b) { return b == 0; });
}

/** Runs f with the gcm of the key's size on Backend: 16, 24 or 32 bytes. */
template <class Backend, class F>
void with_gcm(std::span<std::uint8_t const> key, F&& f) {
    switch (key.size()) {
        case 16: f(crypto::gcm<crypto::aes128<Backend>>::make(key)); break;
        case 24: f(crypto::gcm<crypto::aes192<Backend>>::make(key)); break;
        default: f(crypto::gcm<crypto::aes256<Backend>>::make(key)); break;
    }
}

/** Seals and opens each of the 18 cases one shot, and checks ciphertext and tag. */
template <class Backend>
void check_spec_cases() {
    for (auto const& c : gcm_spec_cases) {
        auto const key = from_hex(c.key);
        auto const iv = from_hex(c.iv);
        auto const additional_data = from_hex(c.additional_data);
        auto const plaintext = from_hex(c.plaintext);
        auto const ciphertext = from_hex(c.ciphertext);
        auto const tag = from_hex(c.tag);
        with_gcm<Backend>(key, [&](auto made) {
            if (!BOOST_TEST(made.has_value())) {
                return;
            }
            auto data = plaintext;
            auto const sealed = made->seal(iv, additional_data, data);
            BOOST_TEST(sealed.has_value());
            BOOST_TEST_EQ(to_hex(data), to_hex(ciphertext));
            BOOST_TEST(sealed && crypto::equal(*sealed, std::span<std::uint8_t const>(tag)));
            BOOST_TEST(made->open(iv, additional_data, data, tag).has_value());
            BOOST_TEST_EQ(to_hex(data), to_hex(plaintext));
        });
    }
}

/** Every guard of spec section 2.4, in its order, each leaving data untouched. */
template <class Backend>
void check_guards() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    std::array<std::uint8_t, 31> const short_key{};
    BOOST_TEST(aead::make(short_key).error() == crypto::error::invalid_key_size);
    std::array<std::uint8_t, 16> const key16{};
    BOOST_TEST(aead::make(key16).error() == crypto::error::invalid_key_size);

    std::array<std::uint8_t, 32> const key{};
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    std::array<std::uint8_t, 20> data{};
    data.fill(0x33);
    auto const untouched = data;
    std::array<std::uint8_t, 129> const long_iv{};
    std::array<std::uint8_t, 16> const tag{};
    std::array<std::uint8_t, 15> const short_tag{};

    BOOST_TEST(made->seal({}, {}, data).error() == crypto::error::invalid_iv_size);
    BOOST_TEST(made->seal(long_iv, {}, data).error() == crypto::error::invalid_iv_size);
    BOOST_TEST(made->open({}, {}, data, tag).error() == crypto::error::invalid_iv_size);
    BOOST_TEST(made->open(std::span(long_iv).first(12), {}, data, short_tag).error() ==
               crypto::error::invalid_tag_size);
    // The IV is checked before the tag: both wrong gives the IV's error.
    BOOST_TEST(made->open(long_iv, {}, data, short_tag).error() == crypto::error::invalid_iv_size);
    BOOST_TEST(data == untouched);
    // IVs of 1 and of 128 bytes are accepted (spec section 2.4, LEDGER:22).
    BOOST_TEST(made->seal(std::span(long_iv).first(1), {}, data).has_value());
    BOOST_TEST(made->seal(std::span(long_iv).first(128), {}, data).has_value());
}

/** Changing any byte of ciphertext, tag or additional data fails, and zeroes data. */
template <class Backend>
void check_tampering() {
    using aead = crypto::gcm<crypto::aes128<Backend>>;
    auto const& c = gcm_spec_cases[3];  // case 4: plaintext and additional data
    auto const key = from_hex(c.key);
    auto const iv = from_hex(c.iv);
    auto const additional_data = from_hex(c.additional_data);
    auto const ciphertext = from_hex(c.ciphertext);
    auto const tag = from_hex(c.tag);
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    for (std::size_t i = 0; i < ciphertext.size(); ++i) {
        auto data = ciphertext;
        data[i] ^= 1;
        BOOST_TEST(made->open(iv, additional_data, data, tag).error() ==
                   crypto::error::authentication_failed);
        BOOST_TEST(all_zero(data));
    }
    for (std::size_t i = 0; i < tag.size(); ++i) {
        auto data = ciphertext;
        auto bad = tag;
        bad[i] ^= 0x80;
        BOOST_TEST(made->open(iv, additional_data, data, bad).error() ==
                   crypto::error::authentication_failed);
        BOOST_TEST(all_zero(data));
    }
    for (std::size_t i = 0; i < additional_data.size(); ++i) {
        auto data = ciphertext;
        auto bad = additional_data;
        bad[i] ^= 2;
        BOOST_TEST(made->open(iv, bad, data, tag).error() == crypto::error::authentication_failed);
        BOOST_TEST(all_zero(data));
    }
    // The object works after failures.
    auto data = ciphertext;
    BOOST_TEST(made->open(iv, additional_data, data, tag).has_value());
}

/** A moved-from gcm answers invalid_state to everything but assignment and destruction. */
template <class Backend>
void check_moved_from() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    std::array<std::uint8_t, 32> const key{};
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    aead moved = std::move(*made);
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 16> data{};
    data.fill(0x6c);  // a guard's refusal leaves this pattern, not only zeroes, untouched
    auto const untouched = data;
    std::array<std::uint8_t, 16> const tag{};
    auto const all_refused = [&](aead& empty) {
        BOOST_TEST(empty.seal(iv, {}, data).error() == crypto::error::invalid_state);
        BOOST_TEST(empty.open(iv, {}, data, tag).error() == crypto::error::invalid_state);
        BOOST_TEST(empty.begin_seal(iv, {}).error() == crypto::error::invalid_state);
        BOOST_TEST(empty.update(data).error() == crypto::error::invalid_state);
        BOOST_TEST(empty.result().error() == crypto::error::invalid_state);
        BOOST_TEST(data == untouched);
    };
    all_refused(*made);
    // Moving an object that was moved from gives an empty object, and crashes nothing (I1).
    aead again = std::move(*made);
    all_refused(again);
    all_refused(*made);
    again = std::move(*made);
    all_refused(again);
    // The destination works: case 14, AES-256, one zero block.
    data.fill(0);
    auto const sealed = moved.seal(iv, {}, data);
    BOOST_TEST_EQ(to_hex(data), "cea7403d4d606b6e074ec5d3baf39d18");
    BOOST_TEST(sealed && to_hex(*sealed) == "d0d1c8a799996bf0265b98b5d48ab919");
    // A swap of an empty object and a live one moves the key, both ways.
    std::swap(again, moved);
    BOOST_TEST(again.open(iv, {}, data, *sealed).has_value());
    data.fill(0x6c);
    all_refused(moved);
    // Assignment back makes the source whole again.
    std::array<std::uint8_t, 16> block{};
    *made = std::move(again);
    auto const resealed = made->seal(iv, {}, block);
    BOOST_TEST(resealed && to_hex(*resealed) == "d0d1c8a799996bf0265b98b5d48ab919");
}

/** Empty inputs, and spans that touch or overlap (Review Focus 2 and 3). */
template <class Backend>
void check_edges() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    std::array<std::uint8_t, 32> const key{};
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    std::array<std::uint8_t, 12> const iv{};
    // Case 13: empty plaintext and additional data, through spans whose data() is null.
    auto const empty = made->seal(iv, std::span<std::uint8_t const>(), std::span<std::uint8_t>());
    BOOST_TEST(empty && to_hex(*empty) == "530f8afbc74536b9a963b4f1c4cb738b");
    BOOST_TEST(made->open(iv, {}, std::span<std::uint8_t>(), *empty).has_value());

    // A wire buffer: ciphertext then tag, the tag a subspan right after the data; the open
    // gives the plaintext back and leaves the tag's bytes as they were.
    std::array<std::uint8_t, 32> wire{};
    std::fill(wire.begin(), wire.begin() + 16, 0x33);
    auto const sealed = made->seal(iv, {}, std::span(wire).first(16));
    if (!BOOST_TEST(sealed.has_value())) {
        return;
    }
    std::copy(sealed->begin(), sealed->end(), wire.begin() + 16);
    BOOST_TEST(made->open(iv, {}, std::span(wire).first(16), std::span(wire).last(16)).has_value());
    BOOST_TEST(std::all_of(wire.begin(), wire.begin() + 16, [](std::uint8_t b) { return b == 0x33; }));
    BOOST_TEST(std::equal(sealed->begin(), sealed->end(), wire.begin() + 16));

    // A tag that overlaps the data's end fails cleanly: the data is zeroed, and the bytes
    // after the data, which only the tag covers, are as they were.
    std::array<std::uint8_t, 24> overlap{};
    overlap.fill(0x5a);
    BOOST_TEST(made->open(iv, {}, std::span(overlap).first(20), std::span(overlap).last(16))
                   .error() == crypto::error::authentication_failed);
    BOOST_TEST(all_zero(std::span(overlap).first(20)));
    BOOST_TEST(std::all_of(overlap.begin() + 20, overlap.end(), [](std::uint8_t b) { return b == 0x5a; }));

    // An IV and additional data inside the sealed buffer itself give what copies of them give:
    // both are consumed before the first byte of data is written.
    std::array<std::uint8_t, 44> buffer{};
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<std::uint8_t>(i * 7 + 1);
    }
    std::array<std::uint8_t, 12> iv_copy{};
    std::array<std::uint8_t, 16> additional_copy{};
    std::array<std::uint8_t, 32> data_copy{};
    std::copy(buffer.begin(), buffer.begin() + 12, iv_copy.begin());
    std::copy(buffer.begin() + 12, buffer.begin() + 28, additional_copy.begin());
    std::copy(buffer.begin() + 12, buffer.end(), data_copy.begin());
    auto const expected = made->seal(iv_copy, additional_copy, data_copy);
    auto const in_place = made->seal(std::span(buffer).first(12), std::span(buffer).subspan(12, 16),
                                     std::span(buffer).subspan(12));
    BOOST_TEST(expected && in_place && to_hex(*expected) == to_hex(*in_place));
    BOOST_TEST(std::equal(data_copy.begin(), data_copy.end(), buffer.begin() + 12));
}

}  // namespace webcpp::test

#endif
```

  The last check compares the in-place seal with a seal of copies (review m3): `gcm` consumes
  the IV and the additional data before the first byte of `data` is written. Buffers start
  from patterns, never zero, so that "untouched" and "zeroed" are each checked against a value
  that differs (m3, m4). `test/gcm_bearssl_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// gcm on BearSSL: the GCM specification's cases, the guards, tampering, moved-from
// objects and aliasing spans (support/gcm_checks.hpp).

#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/test/exit_status.hpp>

#include "support/gcm_checks.hpp"

int main() {
    using webcpp::crypto::backends::bearssl;
    webcpp::test::check_spec_cases<bearssl>();
    webcpp::test::check_guards<bearssl>();
    webcpp::test::check_tampering<bearssl>();
    webcpp::test::check_moved_from<bearssl>();
    webcpp::test::check_edges<bearssl>();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile`: `crypto-run gcm_bearssl : gcm_bearssl_test.cpp : <library>/third_party//bearssl ;`.
  Run the native lane. Expected: FAIL, `aes.hpp` not found.
- [ ] **Step 3: Write `include/webcpp/crypto/aes.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The AES block cipher, `aes128`, `aes192` and `aes256`: a building block, not encryption
 by itself. A program that encrypts messages uses `gcm`.

 @see "The block cipher", in the guide.
*/
#ifndef WEBCPP_CRYPTO_AES_HPP
#define WEBCPP_CRYPTO_AES_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/system/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace webcpp::crypto {

namespace detail {

/**
 * The AES block cipher with keys of `KeySize` bytes on `Backend`, of which `aes128`, `aes192`
 * and `aes256` are the named forms.
 *
 * @tparam KeySize 16, 24 or 32.
 * @tparam Backend A backend tag of `backends`.
 */
template <std::size_t KeySize, class Backend>
class basic_aes {
    using state_type = block_state<Backend, KeySize>;
    using engine = block_engine<Backend, KeySize>;

public:
    /** The backend. */
    using backend_type = Backend;
    /** The key's size in bytes. */
    static constexpr std::size_t key_size = KeySize;
    /** AES's block, 16 bytes. */
    static constexpr std::size_t block_size = 16;

    /**
     * Moves the key schedule; the source is wiped and empty. Moving an empty object gives an
     * empty one: the engine is not asked to move a state it wiped (I1).
     */
    basic_aes(basic_aes&& other) noexcept : keyed_(std::exchange(other.keyed_, false)) {
        if (keyed_) {
            engine::move(state_, other.state_);
        }
    }

    /** Moves the key schedule over this one, which is wiped first. */
    basic_aes& operator=(basic_aes&& other) noexcept {
        if (this != &other) {
            engine::reset(state_);
            keyed_ = std::exchange(other.keyed_, false);
            if (keyed_) {
                engine::move(state_, other.state_);
            }
        }
        return *this;
    }

    basic_aes(basic_aes const&) = delete;
    basic_aes& operator=(basic_aes const&) = delete;

    /** Wipes the key schedule. */
    ~basic_aes() { engine::reset(state_); }

    /**
     * Encrypts whole blocks in place.
     *
     * @param blocks The data, a multiple of @ref block_size bytes.
     * @return Nothing; `invalid_state` moved from; `invalid_input_size` for a partial block.
     */
    boost::system::result<void> encrypt(std::span<std::uint8_t> blocks) noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        if (blocks.size() % block_size != 0) {
            return error::invalid_input_size;
        }
        return engine::encrypt(state_, blocks);
    }

    /**
     * Decrypts whole blocks in place.
     *
     * @param blocks The data, a multiple of @ref block_size bytes.
     * @return Nothing; `invalid_state` moved from; `invalid_input_size` for a partial block.
     */
    boost::system::result<void> decrypt(std::span<std::uint8_t> blocks) noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        if (blocks.size() % block_size != 0) {
            return error::invalid_input_size;
        }
        return engine::decrypt(state_, blocks);
    }

protected:
    basic_aes() noexcept = default;

    /** The work of `make`, after which the derived type is the result. */
    template <class Derived>
    static boost::system::result<Derived> make_as(std::span<std::uint8_t const> key) noexcept {
        if (key.size() != key_size) {
            return error::invalid_key_size;
        }
        Derived made;
        auto const keyed = engine::init(made.state_, key);
        if (!keyed) {
            return keyed.error();
        }
        made.keyed_ = true;
        return made;
    }

private:
    state_type state_{};
    bool keyed_ = false;
};

}  // namespace detail

/**
 * AES-128, the block cipher, on a backend that exports it: ctaes, OpenSSL.
 *
 * @tparam Backend The backend; the default is the AEADs' (`default_backend`), which on
 * WebAssembly is BearSSL and has none: name `aes128<backends::ctaes>` there.
 */
template <class Backend = default_backend>
class aes128 : public detail::basic_aes<16, Backend> {
public:
    /**
     * Expands a key.
     *
     * @param key 16 bytes.
     * @return The cipher; `invalid_key_size`; the backend's errors.
     */
    static boost::system::result<aes128> make(std::span<std::uint8_t const> key) noexcept {
        return detail::basic_aes<16, Backend>::template make_as<aes128>(key);
    }

private:
    friend class detail::basic_aes<16, Backend>;
    aes128() noexcept = default;
};
```

  and `aes192` (24) and `aes256` (32) the same, each with its own Doc Comment. The engine's
  `init`, `encrypt` and `decrypt` are the block-cipher contract of every backend:

```cpp
template <> struct block_engine<B, KeySize> {
    static boost::system::result<void> init(block_state<B, KeySize>&, std::span<std::uint8_t const> key) noexcept;
    static boost::system::result<void> encrypt(block_state<B, KeySize>&, std::span<std::uint8_t> blocks) noexcept;
    static boost::system::result<void> decrypt(block_state<B, KeySize>&, std::span<std::uint8_t> blocks) noexcept;
    static void move(block_state<B, KeySize>& to, block_state<B, KeySize>& from) noexcept;  // to is empty
    static void reset(block_state<B, KeySize>&) noexcept;
};
```

- [ ] **Step 4: Write `include/webcpp/crypto/gcm.hpp`** with spec 2.4's interface; `make`,
  `seal`, `open`, the moves and the destructor here, and Task 8 the incremental seal:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `gcm<Cipher>`, AES-GCM over the cipher's backend.

 @see "AES-GCM", in the guide.
*/
#ifndef WEBCPP_CRYPTO_GCM_HPP
#define WEBCPP_CRYPTO_GCM_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/detail/limits.hpp>
#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/hash2/digest.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace webcpp::crypto {

/**
 * AES-GCM (NIST SP 800-38D) with the key size of `Cipher`, on the backend of `Cipher`, which
 * must export GCM over its own AES: BearSSL or OpenSSL.
 *
 * An object holds a key and the state of one message: it is used by one thread at a time,
 * and it is not shareable. An IV is never reused under one key, and with random IVs, or IVs
 * other than 12 bytes, a key seals at most 2^32 messages (NIST SP 800-38D section 8.3).
 *
 * @tparam Cipher `aes128<B>`, `aes192<B>` or `aes256<B>`.
 */
template <class Cipher>
class gcm {
    using backend = typename Cipher::backend_type;
    using state_type = detail::gcm_state<backend, Cipher::key_size>;
    using engine = detail::gcm_engine<backend, Cipher::key_size>;

public:
    /** The block cipher. */
    using cipher_type = Cipher;
    /** The backend. */
    using backend_type = backend;
    /** The tag, Boost.Hash2's digest of 16 bytes. */
    using result_type = boost::hash2::digest<16>;
    /** The key's size in bytes. */
    static constexpr std::size_t key_size = Cipher::key_size;
    /** The tag's size. */
    static constexpr std::size_t tag_size = 16;
    /** The recommended IV size. */
    static constexpr std::size_t iv_size = 12;
    /** The longest IV accepted, OpenSSL's limit, applied on every backend. */
    static constexpr std::size_t max_iv_size = 128;
    /** The longest message, 2^36 - 32 bytes (NIST SP 800-38D section 5.2.1.1). */
    static constexpr std::uint64_t max_message_size = (std::uint64_t{1} << 36) - 32;
    /** The longest additional data, 2^61 - 1 bytes (the same section, 2^64 - 1 bits). */
    static constexpr std::uint64_t max_additional_data_size = (std::uint64_t{1} << 61) - 1;

    /**
     * Expands a key into a new object.
     *
     * @param key `key_size` bytes.
     * @return The object; `invalid_key_size`, `errc::not_enough_memory` or `backend_failure`.
     */
    static boost::system::result<gcm> make(std::span<std::uint8_t const> key) noexcept {
        if (key.size() != key_size) {
            return error::invalid_key_size;
        }
        gcm made;
        auto const keyed = engine::init(made.state_, key);
        if (!keyed) {
            return keyed.error();
        }
        made.keyed_ = true;
        return made;
    }

    /**
     * Moves the key; a message in progress is abandoned, and the source is wiped and empty.
     * Moving an empty object gives an empty one: BearSSL's move initialises the destination's
     * context through the schedule's vtable, which a wiped state does not have (I1).
     */
    gcm(gcm&& other) noexcept : keyed_(std::exchange(other.keyed_, false)) {
        if (keyed_) {
            engine::move(state_, other.state_);
        }
        other.end_message();
    }

    /** Moves the key over this one, which is wiped first. */
    gcm& operator=(gcm&& other) noexcept {
        if (this != &other) {
            engine::reset(state_);
            keyed_ = std::exchange(other.keyed_, false);
            if (keyed_) {
                engine::move(state_, other.state_);
            }
            end_message();
            other.end_message();
        }
        return *this;
    }

    gcm(gcm const&) = delete;
    gcm& operator=(gcm const&) = delete;

    /** Wipes the key and the state. */
    ~gcm() { engine::reset(state_); }

    /**
     * Seals a message in place: `data` holds the plaintext before and the ciphertext after.
     *
     * @param iv 1 to `max_iv_size` bytes, never reused under this key; 12 recommended.
     * @param additional_data Authenticated, not encrypted.
     * @param data The message.
     * @return The tag; `invalid_state` moved from or during an incremental message;
     * `invalid_iv_size`; `message_too_long`; `backend_failure`.
     */
    boost::system::result<result_type> seal(std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data) noexcept {
        auto const refused = guard_message(iv, additional_data, data.size());
        if (!refused) {
            return refused.error();
        }
        auto const started = engine::start(state_, true, iv, additional_data);
        if (!started) {
            return started.error();
        }
        auto const ran = engine::run(state_, data);
        if (!ran) {
            return ran.error();
        }
        result_type tag{};
        auto const tagged = engine::tag(state_, tag);
        if (!tagged) {
            return tagged.error();
        }
        return tag;
    }

    /**
     * Opens a message in place: `data` holds the ciphertext before and the plaintext after.
     *
     * While it runs, the decrypted bytes are in `data` before the tag is checked; when the
     * tag does not match, or the backend fails, `data` is zeroed. A guard's refusal leaves it
     * untouched.
     *
     * @param iv The IV of the seal.
     * @param additional_data The additional data of the seal.
     * @param data The ciphertext.
     * @param tag The seal's tag, 16 bytes, which may lie anywhere, `data` included.
     * @return Nothing; `invalid_state`; `invalid_iv_size`; `invalid_tag_size`;
     * `message_too_long`; `authentication_failed`; `backend_failure`.
     */
    boost::system::result<void> open(std::span<std::uint8_t const> iv,
                                     std::span<std::uint8_t const> additional_data,
                                     std::span<std::uint8_t> data,
                                     std::span<std::uint8_t const> tag) noexcept {
        if (!keyed_ || in_message_) {
            return error::invalid_state;
        }
        if (iv.empty() || iv.size() > max_iv_size) {
            return error::invalid_iv_size;
        }
        if (tag.size() != tag_size) {
            return error::invalid_tag_size;
        }
        auto const refused = guard_message(iv, additional_data, data.size());
        if (!refused) {
            return refused.error();
        }
        // The tag may lie in data, which decryption overwrites: a copy is checked.
        std::array<std::uint8_t, tag_size> expected{};
        std::copy(tag.begin(), tag.end(), expected.begin());
        auto const started = engine::start(state_, false, iv, additional_data);
        auto const opened = started ? engine::run(state_, data) : started;
        auto const checked = opened ? engine::check(state_, expected) : opened;
        if (!checked) {
            detail::wipe(data.data(), data.size());
            return checked.error();
        }
        return {};
    }

private:
    gcm() noexcept = default;

    boost::system::result<void> guard_message(std::span<std::uint8_t const> iv,
                                              std::span<std::uint8_t const> additional_data,
                                              std::size_t size) const noexcept {
        if (!keyed_ || in_message_) {
            return error::invalid_state;
        }
        if (iv.empty() || iv.size() > max_iv_size) {
            return error::invalid_iv_size;
        }
        if (detail::exceeds(size, max_message_size) ||
            detail::exceeds(additional_data.size(), max_additional_data_size)) {
            return error::message_too_long;
        }
        return {};
    }

    void end_message() noexcept {
        in_message_ = false;
        message_bytes_ = 0;
    }

    state_type state_{};
    bool keyed_ = false;
    bool in_message_ = false;
    std::uint64_t message_bytes_ = 0;
};

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_GCM_HPP
```

  `open` checks the IV and the tag first and repeats the IV inside `guard_message`, so that
  both orders of spec 2.4's table hold: moved-from, IV, tag, length.
- [ ] **Step 5: Write BearSSL's engines** in `backends/bearssl.hpp`, after the tag type,
  with the includes `<array>`, `<boost/hash2/digest.hpp>`, `<boost/system/result.hpp>`,
  `<cstddef>`, `<cstdint>`, `<span>`, `<webcpp/crypto/detail/declassify.hpp>`,
  `<webcpp/crypto/detail/wipe.hpp>` and `<webcpp/crypto/error.hpp>`. The first is new here:
  the one mark of a value derived from secrets as public, the tag's verdict, for the
  constant-time checks of Task 20 (I8), `include/webcpp/crypto/detail/declassify.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_DECLASSIFY_HPP
#define WEBCPP_CRYPTO_DETAIL_DECLASSIFY_HPP

#include <webcpp/crypto/config.hpp>

/**
 * Marks a value derived from secrets as public, where webcpp's code branches on it by design:
 * the verdict of a tag's check. Nothing in a program's build; a constant-time test defines
 * WEBCPP_CRYPTO_DETAIL_CT_VALGRIND or WEBCPP_CRYPTO_DETAIL_CT_MSAN, and the mark is then
 * Valgrind's or MemorySanitizer's, so that the checker reports every other branch on a
 * secret and not this one (Task 20, spec section 8.3).
 *
 * Tip: the object marked is never const, so the branch that follows reads it again after the
 * mark instead of a register the compiler kept from before it (BoringSSL's practice, n1).
 */
#if defined(WEBCPP_CRYPTO_DETAIL_CT_VALGRIND)
#include <valgrind/memcheck.h>
#define WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address, size) \
    static_cast<void>(VALGRIND_MAKE_MEM_DEFINED((address), (size)))
#elif defined(WEBCPP_CRYPTO_DETAIL_CT_MSAN)
#include <sanitizer/msan_interface.h>
#define WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address, size) __msan_unpoison((address), (size))
#else
#define WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address, size) static_cast<void>(0)
#endif

#endif  // WEBCPP_CRYPTO_DETAIL_DECLASSIFY_HPP
```

  It joins the first headers-alone call's list. The engines:

```cpp
namespace webcpp::crypto::detail {

/**
 * The CTR class: AES-NI, then POWER8, then the constant-time 64-bit code, which BearSSL would
 * not pick on wasm32 by itself (its default there is the 32-bit code, half as fast). The test
 * seam WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE keeps the constant-time code on every CPU.
 */
inline br_block_ctr_class const* bearssl_ctr() noexcept {
#ifndef WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE
    if (auto const* x86 = br_aes_x86ni_ctr_get_vtable()) {
        return x86;
    }
    if (auto const* power = br_aes_pwr8_ctr_get_vtable()) {
        return power;
    }
#endif
    return &br_aes_ct64_ctr_vtable;
}

/** The GHASH code: PCLMUL, then POWER8, then the constant-time 64-bit code. */
inline br_ghash bearssl_ghash() noexcept {
#ifndef WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE
    if (br_ghash const pclmul = br_ghash_pclmul_get()) {
        return pclmul;
    }
    if (br_ghash const power = br_ghash_pwr8_get()) {
        return power;
    }
#endif
    return &br_ghash_ctmul64;
}

/** GCM's state on BearSSL: the key schedule, the context and the direction, in the object. */
template <std::size_t KeySize>
struct gcm_state<backends::bearssl, KeySize> {
    br_aes_gen_ctr_keys keys;
    br_gcm_context context;
    bool sealing;
};

/** GCM's work on BearSSL (spec section 3.2). */
template <std::size_t KeySize>
struct gcm_engine<backends::bearssl, KeySize> {
    using state = gcm_state<backends::bearssl, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        bearssl_ctr()->init(&s.keys.vtable, key.data(), key.size());
        br_gcm_init(&s.context, &s.keys.vtable, bearssl_ghash());
        return {};
    }

    static boost::system::result<void> start(state& s, bool sealing,
                                             std::span<std::uint8_t const> iv,
                                             std::span<std::uint8_t const> additional_data) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        s.sealing = sealing;
        br_gcm_reset(&s.context, iv.data(), iv.size());
        br_gcm_aad_inject(&s.context, additional_data.data(), additional_data.size());
        br_gcm_flip(&s.context);
        return {};
    }

    static boost::system::result<void> run(state& s, std::span<std::uint8_t> data) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        br_gcm_run(&s.context, s.sealing ? 1 : 0, data.data(), data.size());
        return {};
    }

    static boost::system::result<void> tag(state& s, boost::hash2::digest<16>& out) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        br_gcm_get_tag(&s.context, out.data());
        return {};
    }

    static boost::system::result<void> check(state& s,
                                             std::array<std::uint8_t, 16> const& tag) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        // The verdict is public by design (a failed open is reported): declassified here,
        // where webcpp's code first branches on it, for the constant-time checks (Task 20).
        // Not const (n1): the branch below must read the variable again after the declassifying
        // client request, whose memory clobber reaches a mutable object only.
        // NOLINTNEXTLINE(misc-const-correctness): see above
        std::uint32_t matches = br_gcm_check_tag(&s.context, tag.data());
        WEBCPP_CRYPTO_DETAIL_DECLASSIFY(&matches, sizeof matches);
        if (matches != 1) {
            return error::authentication_failed;
        }
        return {};
    }

    // The context points at the key schedule (br_gcm_context::bctx), so a move copies the
    // schedule and initialises the destination's context again: a message is abandoned.
    static void move(state& to, state& from) noexcept {
        to.keys = from.keys;
        br_gcm_init(&to.context, &to.keys.vtable, bearssl_ghash());
        to.sealing = false;
        reset(from);
    }

    static void reset(state& s) noexcept { wipe_object(s); }
};

/** BearSSL has no AES block cipher: its state is empty, so `aes256<backends::bearssl>` is complete. */
template <std::size_t KeySize>
struct block_state<backends::bearssl, KeySize> {};

/**
 * BearSSL's refusal of the block cipher (spec section 2.3). The assertions sit in member
 * templates, which only `make`, `encrypt` and `decrypt` instantiate: the class's moves and
 * destructor, which `gcm<aes256<backends::bearssl>>` needs, never fire them.
 */
template <std::size_t KeySize>
struct block_engine<backends::bearssl, KeySize> {
    using state = block_state<backends::bearssl, KeySize>;

    template <bool Never = false>
    static void refuse() noexcept {
        static_assert(Never || KeySize != 16, "webcpp::crypto: aes128<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in aes128<backends::ctaes> or aes128<backends::openssl>, and add its library to the program's Jamfile.");
        static_assert(Never || KeySize != 24, "webcpp::crypto: aes192<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in aes192<backends::ctaes> or aes192<backends::openssl>, and add its library to the program's Jamfile.");
        static_assert(Never || KeySize != 32, "webcpp::crypto: aes256<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in aes256<backends::ctaes> or aes256<backends::openssl>, and add its library to the program's Jamfile.");
    }

    template <bool Never = false>
    static boost::system::result<void> init(state&, std::span<std::uint8_t const>) noexcept {
        refuse<Never>();
        return error::backend_failure;
    }

    template <bool Never = false>
    static boost::system::result<void> encrypt(state&, std::span<std::uint8_t>) noexcept {
        refuse<Never>();
        return error::backend_failure;
    }

    template <bool Never = false>
    static boost::system::result<void> decrypt(state&, std::span<std::uint8_t>) noexcept {
        refuse<Never>();
        return error::backend_failure;
    }

    static void move(state&, state&) noexcept {}
    static void reset(state&) noexcept {}
};

}  // namespace webcpp::crypto::detail
```

  `basic_aes` calls `engine::init(...)`, `engine::encrypt(...)` and `engine::decrypt(...)`
  without template arguments; on a backend whose engine has plain functions those are plain
  calls, and on BearSSL they deduce `Never = false` and fire the assertion. A
  `static_assert` whose condition depends on a template parameter is checked only when its
  template is instantiated.
- [ ] **Step 6: Write the message test** `test/messages/aes_on_bearssl/` (spec 2.3's text,
  native and wasip2): its source calls `aes256<backends::bearssl>::make(key)` and states
  `// expected-error@webcpp/crypto/backends/bearssl.hpp:* {{webcpp::crypto: aes256<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in aes256<backends::ctaes> or aes256<backends::openssl>, and add its library to the program's Jamfile.}}`;
  its Jamfile names `<library>/third_party//bearssl`. A second source of the same directory,
  `gcm_names_aes_on_bearssl.cpp`, declared with `webcpp.compile`, names
  `gcm<aes256<backends::bearssl>>`, makes it and seals, and must compile: the block cipher's
  refusal does not reach GCM.
- [ ] **Step 7: Run** the four lanes. Expected: PASS, `gcm_bearssl` and its twin on every
  target, both message tests, and the headers alone of `aes.hpp` and `gcm.hpp` (in the first
  call's list: they include no backend).
- [ ] **Step 8: Commit:** `"crypto: the block cipher's shape, and AES-GCM one shot on
  BearSSL, every guard before the work"`.


## Task 8: The incremental seal, and the limits

**Files:**
- Modify: `include/webcpp/crypto/gcm.hpp` (`begin_seal`, `update`, `result`),
  `include/webcpp/crypto/backends/bearssl.hpp` (the engine's `abandon`),
  `test/support/gcm_checks.hpp` (the machine's checks), `test/gcm_bearssl_test.cpp`,
  `test/Jamfile`.
- Create: `test/support/reserved.hpp`, `test/gcm_limits_bearssl_test.cpp`.

**Interfaces:**
- Consumes: Task 7's engine contract.
- Produces: `gcm::begin_seal(iv, additional_data)`, `gcm::update(data)`, `gcm::result()`
  (spec 2.4); one more member of every GCM engine:
  `static void abandon(gcm_state<B, KeySize>&) noexcept;` (the message's state wiped, the key
  kept); the checks `check_in_pieces(made, iv, additional_data, plaintext, ciphertext_hex, tag_hex, piece)`,
  `check_every_piece(...)` (the same without `piece`), `check_incremental<Backend>()` (I4: the 18
  cases, every piece size), `check_machine<Backend>()`,
  `check_one_shot_during_message<Backend>()` and `check_limits<Backend>()`;
  `std::span<std::uint8_t> webcpp::test::reserved(std::size_t)`.

- [ ] **Step 1: Write the failing checks** in `test/support/gcm_checks.hpp`:

```cpp
/**
 * Seals `plaintext` through the incremental seal in pieces of `piece` bytes (and one empty
 * piece), checks the ciphertext and the tag against the expected hex, and opens the result
 * one shot (spec 8.1: every case sealed incrementally as well).
 */
template <class Aead>
void check_in_pieces(Aead& made, std::span<std::uint8_t const> iv,
                     std::span<std::uint8_t const> additional_data,
                     std::vector<std::uint8_t> const& plaintext, std::string_view ciphertext,
                     std::string_view tag_hex, std::size_t piece) {
    auto data = plaintext;
    BOOST_TEST(made.begin_seal(iv, additional_data).has_value());
    for (std::size_t at = 0; at < data.size(); at += piece) {
        auto const size = (std::min)(piece, data.size() - at);
        BOOST_TEST(made.update(std::span(data).subspan(at, size)).has_value());
    }
    BOOST_TEST(made.update(std::span<std::uint8_t>()).has_value());  // an empty piece
    auto const tag = made.result();
    BOOST_TEST_EQ(to_hex(data), ciphertext);
    BOOST_TEST(tag && to_hex(*tag) == tag_hex);
    BOOST_TEST(tag && made.open(iv, additional_data, data, *tag).has_value());
    BOOST_TEST(data == plaintext);
}

/** Every piece size from 1 to the whole message, the whole included (at least 1). */
template <class Aead>
void check_every_piece(Aead& made, std::span<std::uint8_t const> iv,
                       std::span<std::uint8_t const> additional_data,
                       std::vector<std::uint8_t> const& plaintext, std::string_view ciphertext,
                       std::string_view tag_hex) {
    for (std::size_t piece = 1; piece <= (std::max)(plaintext.size(), std::size_t{1}); ++piece) {
        check_in_pieces(made, iv, additional_data, plaintext, ciphertext, tag_hex, piece);
    }
}

/** The 18 cases of the GCM specification, each sealed in pieces of every size. */
template <class Backend>
void check_incremental() {
    for (auto const& c : gcm_spec_cases) {
        auto const iv = from_hex(c.iv);
        auto const additional_data = from_hex(c.additional_data);
        auto const plaintext = from_hex(c.plaintext);
        with_gcm<Backend>(from_hex(c.key), [&](auto made) {
            if (BOOST_TEST(made.has_value())) {
                check_every_piece(*made, iv, additional_data, plaintext, c.ciphertext, c.tag);
            }
        });
    }
}

/** The incremental seal's machine (spec section 2.4): each wrong call, then a fresh message. */
template <class Backend>
void check_machine() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    std::array<std::uint8_t, 16> data{};
    auto const fresh_message_works = [&] {
        data.fill(0);
        BOOST_TEST(made->begin_seal(iv, {}).has_value());
        BOOST_TEST(made->update(data).has_value());
        auto const tag = made->result();
        BOOST_TEST(tag && to_hex(*tag) == "d0d1c8a799996bf0265b98b5d48ab919");  // case 14
    };
    BOOST_TEST(made->update(data).error() == crypto::error::invalid_state);
    BOOST_TEST(made->result().error() == crypto::error::invalid_state);
    fresh_message_works();
    BOOST_TEST(made->result().error() == crypto::error::invalid_state);  // a second result

    BOOST_TEST(made->begin_seal(iv, {}).has_value());
    BOOST_TEST(made->begin_seal(iv, {}).error() == crypto::error::invalid_state);
    BOOST_TEST(made->result().error() == crypto::error::invalid_state);  // the error ended it
    fresh_message_works();

    BOOST_TEST(made->begin_seal(iv, {}).has_value());
    std::array<std::uint8_t, 129> const long_iv{};
    BOOST_TEST(made->begin_seal(long_iv, {}).error() == crypto::error::invalid_state);
    fresh_message_works();
    BOOST_TEST(made->begin_seal(long_iv, {}).error() == crypto::error::invalid_iv_size);
    fresh_message_works();

    BOOST_TEST(made->begin_seal(iv, {}).has_value());
    aead moved = std::move(*made);  // a move abandons the message on both sides
    BOOST_TEST(moved.result().error() == crypto::error::invalid_state);
    BOOST_TEST(made->update(data).error() == crypto::error::invalid_state);
    *made = std::move(moved);
    fresh_message_works();
}

/** A one-shot seal or open during an incremental message is refused (Review Focus 1). */
template <class Backend>
void check_one_shot_during_message() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    auto made = aead::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    std::array<std::uint8_t, 16> message{};
    BOOST_TEST(made->begin_seal(iv, {}).has_value());
    BOOST_TEST(made->update(std::span(message).first(5)).has_value());
    std::array<std::uint8_t, 16> other{};
    other.fill(9);
    auto const untouched = other;
    std::array<std::uint8_t, 16> const tag{};
    BOOST_TEST(made->seal(iv, {}, other).error() == crypto::error::invalid_state);
    BOOST_TEST(made->open(iv, {}, other, tag).error() == crypto::error::invalid_state);
    BOOST_TEST(other == untouched);
    BOOST_TEST(made->update(std::span(message).subspan(5)).has_value());
    auto const result = made->result();
    BOOST_TEST_EQ(to_hex(message), "cea7403d4d606b6e074ec5d3baf39d18");
    BOOST_TEST(result && to_hex(*result) == "d0d1c8a799996bf0265b98b5d48ab919");
}
```

  `test/support/reserved.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_RESERVED_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_RESERVED_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace webcpp::test {

/**
 * Address space of `size` bytes that faults on any access, never released: a span a guard
 * must refuse by its size alone, since reading one byte of it ends the program. Natively on
 * 64-bit targets only.
 */
inline std::span<std::uint8_t> reserved(std::size_t size) {
#ifdef _WIN32
    void* const at = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* at = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (at == MAP_FAILED) {
        at = nullptr;
    }
#endif
    return {static_cast<std::uint8_t*>(at), at != nullptr ? size : 0};
}

}  // namespace webcpp::test

#endif
```

  and in `gcm_checks.hpp`, guarded by `#if !defined(__wasi__) && !defined(__EMSCRIPTEN__)`
  with `#include "reserved.hpp"`. The additional data's own limit (2^61 - 1 bytes) cannot be
  reached by a span a guard refuses before reading, since a smaller one would be read and
  fault; it is pinned by `detail::exceeds`'s test (Task 3) and read in review:

```cpp
template <class Backend>
void check_limits() {
    using aead = crypto::gcm<crypto::aes256<Backend>>;
    static_assert(sizeof(std::size_t) == 8, "the limits are reachable only with 64-bit sizes");
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    auto made = aead::make(key);
    auto const huge = reserved(static_cast<std::size_t>(aead::max_message_size) + 1);
    if (!BOOST_TEST(made.has_value()) || !BOOST_TEST(!huge.empty())) {
        return;
    }
    BOOST_TEST(made->seal(iv, {}, huge).error() == crypto::error::message_too_long);
    std::array<std::uint8_t, 16> const tag{};
    BOOST_TEST(made->open(iv, {}, huge, tag).error() == crypto::error::message_too_long);
    std::array<std::uint8_t, 16> first{};
    BOOST_TEST(made->begin_seal(iv, {}).has_value());
    BOOST_TEST(made->update(first).has_value());
    BOOST_TEST(made->update(huge.first(static_cast<std::size_t>(aead::max_message_size) - 15))
                   .error() == crypto::error::message_too_long);
    BOOST_TEST(made->result().error() == crypto::error::invalid_state);  // the error ended it
}
```

  `test/gcm_bearssl_test.cpp` calls `check_incremental`, `check_machine` and
  `check_one_shot_during_message` after Task 7's checks. `test/gcm_limits_bearssl_test.cpp`
  includes `backends/bearssl.hpp` and `support/gcm_checks.hpp` and calls
  `check_limits<bearssl>()`; in `test/Jamfile`:
  `crypto-run gcm_limits_bearssl : gcm_limits_bearssl_test.cpp : <library>/third_party//bearssl : native ;`.
  Run the native lane. Expected: FAIL, `begin_seal` is no member of `gcm`.
- [ ] **Step 2: Write the incremental seal** in `gcm.hpp`, after `open`:

```cpp
    /**
     * Begins an incremental seal: the IV and the whole additional data first.
     *
     * Any error of `begin_seal`, `update` or `result` ends the message, which is abandoned:
     * the pieces already encrypted have no tag and are not to be sent.
     *
     * @param iv 1 to `max_iv_size` bytes, never reused under this key.
     * @param additional_data Authenticated, not encrypted.
     * @return Nothing; `invalid_state` moved from or with a message in progress, which ends
     * it; `invalid_iv_size`; `message_too_long`; `backend_failure`.
     */
    boost::system::result<void> begin_seal(std::span<std::uint8_t const> iv,
                                           std::span<std::uint8_t const> additional_data) noexcept {
        if (in_message_) {
            abandon();
            return error::invalid_state;
        }
        auto const refused = guard_message(iv, additional_data, 0);
        if (!refused) {
            return refused.error();
        }
        auto const started = engine::start(state_, true, iv, additional_data);
        if (!started) {
            engine::abandon(state_);
            return started.error();
        }
        in_message_ = true;
        message_bytes_ = 0;
        return {};
    }

    /**
     * Encrypts the next piece of the message in place.
     *
     * @param data The piece, of any size, empty included.
     * @return Nothing; `invalid_state` without a message; `message_too_long` when the pieces
     * pass `max_message_size`, which ends the message; `backend_failure`, which ends it.
     */
    boost::system::result<void> update(std::span<std::uint8_t> data) noexcept {
        if (!keyed_ || !in_message_) {
            return error::invalid_state;
        }
        if (detail::exceeds(data.size(), max_message_size - message_bytes_)) {
            abandon();
            return error::message_too_long;
        }
        auto const ran = engine::run(state_, data);
        if (!ran) {
            abandon();
            return ran.error();
        }
        message_bytes_ += data.size();
        return {};
    }

    /**
     * Ends the message with its tag.
     *
     * @return The tag; `invalid_state` without a message; `backend_failure`.
     */
    boost::system::result<result_type> result() noexcept {
        if (!keyed_ || !in_message_) {
            return error::invalid_state;
        }
        result_type tag{};
        auto const tagged = engine::tag(state_, tag);
        end_message();
        if (!tagged) {
            engine::abandon(state_);
            return tagged.error();
        }
        return tag;
    }
```

  and in the private part:

```cpp
    void abandon() noexcept {
        engine::abandon(state_);
        end_message();
    }
```

  `begin_seal`'s first guard is `in_message_` because a refused `begin_seal` during a
  message ends that message (spec 2.4, "any error from begin_seal ... ends the message");
  `guard_message` then refuses a moved-from object.
- [ ] **Step 3: Give BearSSL's engine `abandon`** (in `gcm_engine<backends::bearssl, K>`):

```cpp
    // The message's state is wiped and the context initialised again on the kept schedule.
    static void abandon(state& s) noexcept {
        wipe_object(s.context);
        br_gcm_init(&s.context, &s.keys.vtable, bearssl_ghash());
        s.sealing = false;
    }
```

- [ ] **Step 4: Run** the four lanes. Expected: PASS: `gcm_bearssl` (with the new checks) on
  every target; `gcm_limits_bearssl` natively, where the reservation of 64 GiB of address
  space succeeds on Linux, macOS and Windows (MSVC lanes), and the guards refuse before any
  byte is read: a guard that read would end the program with SIGSEGV or an access violation.
- [ ] **Step 5: Commit:** `"crypto: GCM's incremental seal, its machine, and the limits
  refused before any byte is read"`.

## Task 9: Wycheproof, through BearSSL

**Files:**
- Create (superproject): the submodule `third_party/libs/wycheproof`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (the target `wycheproof`),
  `tools/test/third_party_test.py` (its case `test_wycheproof_is_pinned`), `CODING-GUIDELINES.md`
  (chapter 10, "Third-party code": the row `wycheproof`), `doc/releases/0.1.0.adoc` ("Tools and
  build": a line). The CI needs no change: every job checks out the superproject's submodules
  (`.github/workflows/library.yml`, `submodules: true`).
- Create (library): `test/support/test_data.hpp`, `test/support/wycheproof_gcm.hpp`,
  `test/gcm_wycheproof_bearssl_test.cpp`.
- Modify (library): `test/Jamfile`.

**Interfaces:**
- Produces: `/third_party//wycheproof` (define `WEBCPP_TEST_WYCHEPROOF`, the checkout's
  absolute path with forward slashes, as a string literal; the directory under wasmtime; a
  `<dependency>` on the files the tests read);
  `std::string webcpp::test::read_file(std::string const& path)`;
  `template <class Backend> void webcpp::test::check_wycheproof_gcm()`.

(Deferred after v0.1: NIST CAVP's archives, their action, target and tests, with owner gate O3.)

- [ ] **Step 1: Add Wycheproof's submodule:**

```bash
git submodule add --depth 1 https://github.com/C2SP/wycheproof.git third_party/libs/wycheproof
git -C third_party/libs/wycheproof fetch --depth 1 origin 12fd3aaf33eb5fa1f52e026912ee00c054f9d984
git -C third_party/libs/wycheproof checkout 12fd3aaf33eb5fa1f52e026912ee00c054f9d984
git config -f .gitmodules submodule.third_party/libs/wycheproof.shallow true
```

  Check: `ls third_party/libs/wycheproof/testvectors_v1/aes_gcm_test.json`.
- [ ] **Step 2: Write the target** in `third_party/Jamfile`, the JSON Schema Test Suite's
  form, with a `<dependency>` on the files a test reads, so that a changed file runs it again
  (CODING-GUIDELINES.md chapter 8):

```jam
# Wycheproof (Apache-2.0), C2SP/wycheproof: test data, never compiled. A test
# that names wycheproof reads it from the directory WEBCPP_TEST_WYCHEPROOF
# names, with forward slashes, the directory given to wasmtime on WASI
# (webcpp.data), and runs again when one of its files changes.
path-constant WYCHEPROOF : libs/wycheproof ;
.wycheproof = [ regex.replace $(WYCHEPROOF) "\\\\" "/" ] ;
.wycheproof-files = [ glob libs/wycheproof/testvectors_v1/*.json ] ;

rule wycheproof-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(WYCHEPROOF)/testvectors_v1/aes_gcm_test.json ] ]
    {
        errors.user-error "third_party/libs/wycheproof is not checked out: it holds no"
            "testvectors_v1/aes_gcm_test.json. Check it out with `git submodule update"
            "--init third_party/libs/wycheproof`." ;
    }
}

alias wycheproof : : <conditional>@wycheproof-checked-out : :
    <define>"\"WEBCPP_TEST_WYCHEPROOF=\\\"$(.wycheproof)\\\"\""
    [ webcpp.data $(WYCHEPROOF) ]
    <dependency>$(.wycheproof-files) ;
explicit wycheproof ;
```

  The glob may be narrowed to the files crypto's tests read (`aes_gcm_test.json` here; the
  secp256k1 files of Task 28) if the whole directory's dependency costs too much.
  `tools/test/third_party_test.py` gains `test_wycheproof_is_pinned`, the form of
  `test_the_suite_is_pinned`: the submodule is at `12fd3aaf33eb5fa1f52e026912ee00c054f9d984`.
- [ ] **Step 3: Write the failing tests.** `test/support/test_data.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_TEST_DATA_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_TEST_DATA_HPP

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace webcpp::test {

/** The whole file at path; a test whose data is missing ends, naming it. */
inline std::string read_file(std::string const& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot read %s\n", path.c_str());
        std::exit(1);
    }
    std::ostringstream all;
    all << in.rdbuf();
    return all.str();
}

}  // namespace webcpp::test

#endif
```

  `test/support/wycheproof_gcm.hpp` (Boost.JSON, the 316 cases of spec 8.1 by class):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Wycheproof's 316 AES-GCM tests at 12fd3aaf (spec section 8.1): the 307 with an IV of 1
// to 128 bytes seal and open, the 226 valid matching and the 81 invalid failing with
// authentication_failed and data zeroed; the 6 with an empty IV give invalid_iv_size; the
// 3 with a 257-byte IV (tcId 268, 272, 276), valid in Wycheproof, give invalid_iv_size
// with data untouched, by the cap of section 2.4.

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_WYCHEPROOF_GCM_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_WYCHEPROOF_GCM_HPP

#include "gcm_checks.hpp"
#include "hex.hpp"
#include "test_data.hpp"

#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <string>

namespace webcpp::test {

template <class Backend>
void check_wycheproof_gcm() {
    auto const document = boost::json::parse(
        read_file(std::string(WEBCPP_TEST_WYCHEPROOF) + "/testvectors_v1/aes_gcm_test.json"));
    std::size_t valid = 0;
    std::size_t invalid = 0;
    std::size_t empty_iv = 0;
    std::size_t over_cap = 0;
    for (auto const& group : document.at("testGroups").as_array()) {
        for (auto const& test : group.at("tests").as_array()) {
            auto const key = from_hex(test.at("key").as_string());
            auto const iv = from_hex(test.at("iv").as_string());
            auto const additional_data = from_hex(test.at("aad").as_string());
            auto const message = from_hex(test.at("msg").as_string());
            auto const ciphertext = from_hex(test.at("ct").as_string());
            auto const tag = from_hex(test.at("tag").as_string());
            bool const is_valid = test.at("result").as_string() == "valid";
            with_gcm<Backend>(key, [&](auto made) {
                if (!BOOST_TEST(made.has_value())) {
                    return;
                }
                auto data = ciphertext;
                auto const opened = made->open(iv, additional_data, data, tag);
                if (iv.empty() || iv.size() > 128) {
                    BOOST_TEST(opened.error() == crypto::error::invalid_iv_size);
                    BOOST_TEST(data == ciphertext);
                    ++(iv.empty() ? empty_iv : over_cap);
                    return;
                }
                if (is_valid) {
                    BOOST_TEST(opened.has_value());
                    BOOST_TEST(data == message);
                    auto sealed_data = message;
                    auto const sealed = made->seal(iv, additional_data, sealed_data);
                    BOOST_TEST(sealed_data == ciphertext);
                    BOOST_TEST(sealed && to_hex(*sealed) == to_hex(tag));
                    // Sealed incrementally as well, in pieces of every size (I4).
                    check_every_piece(*made, iv, additional_data, message, to_hex(ciphertext),
                                      to_hex(tag));
                    ++valid;
                } else {
                    BOOST_TEST(opened.error() == crypto::error::authentication_failed);
                    BOOST_TEST(all_zero(data));
                    ++invalid;
                }
            });
        }
    }
    BOOST_TEST_EQ(valid, 226U);
    BOOST_TEST_EQ(invalid, 81U);
    BOOST_TEST_EQ(empty_iv, 6U);
    BOOST_TEST_EQ(over_cap, 3U);
}

}  // namespace webcpp::test

#endif
```

  `test/gcm_wycheproof_bearssl_test.cpp` includes
  `backends/bearssl.hpp` and its support header, and calls the check for `bearssl`. In
  `test/Jamfile`:

```jam
# The vectors of spec section 8.1, read at run time from third_party/libs.
crypto-run gcm_wycheproof_bearssl : gcm_wycheproof_bearssl_test.cpp
  : <library>/third_party//bearssl <library>/third_party//wycheproof
    <library>/webcpp//boost_json ;
```

  Run the native lane. Expected: FAIL first on the missing support headers; once written,
  PASS. Then wasip2, wasip3 and emscripten. Expected: PASS on each (wasmtime given the
  directory by `webcpp.data`; node reading it under `-sNODERAWFS=1`).
- [ ] **Step 4: Document and commit.** `CODING-GUIDELINES.md` chapter 10, "Third-party code",
  gains the row `wycheproof`: "C2SP's Wycheproof at `12fd3aaf` (2026-10-06), shallow, test
  data: crypto's tests read its `testvectors_v1/` at run time under the directory its define
  `WEBCPP_TEST_WYCHEPROOF` gives, on every target (wasmtime is given it, `webcpp.data`), and
  run again when one of its files changes | Apache-2.0". `doc/releases/0.1.0.adoc`, "Tools and
  build", gains a line: Wycheproof's test vectors, as `/third_party//wycheproof`. Commit
  (superproject) `"third_party: Wycheproof, pinned and shallow"` (`.gitmodules
  third_party/libs/wycheproof third_party/Jamfile tools/test/third_party_test.py
  CODING-GUIDELINES.md doc/releases/0.1.0.adoc`); (library) `"crypto: AES-GCM against
  Wycheproof on BearSSL"`.

## Task 10: The user's Jamfiles as fixtures of a test of the tools

**Why this form** (owner item O4, the controller's ruling). The superproject already tests b2
projects that are not a library's ordinary tests: fixtures under `tools/test/fixtures/`, built
on a scratch copy of the superproject by a Python test of `tools/test/` (`harness.py`,
`emscripten_test.py`, `third_party_test.py`), which the CI's tools job runs. A user's Jamfile
(spec section 7, every build stop of section 8.6) is such a project: b2's raw `exe`, built as a
user builds it, its outcome checked. So this plan adds no kind of library directory and runs no
b2 inside b2: the cases are fixtures `tools/test/fixtures/crypto_jamfiles/<case>/` (a path the
lint's raw-rules pattern, `(libs|tools/test/fixtures)/[^/]+/(test|example)/`, does not match,
so a user's `exe` is allowed there, as it is not under a library's `test/`), and
`tools/test/crypto_jamfiles_test.py` builds them.

**What the pattern cannot express, exactly.** The tools job runs on `ubuntu-24.04` alone
(`.github/workflows/library.yml`, job `tools`), so on the CI it builds the cases with Linux's
native toolset (GNU ld), wasip2, wasip3 and Emscripten (wasm-ld), and never with ld64 or
MSVC's `link.exe` and `cl`. Spec 8.6 asks for the canary's link messages "on ld64, wasm-ld,
Emscripten and MSVC", for MSVC's `C1189`, and for LNK2038. The recorded limit (the owner's
decision): MSVC's cases run on the MSVC lanes, on every push, as `webcpp.compile-fail` and
`webcpp.link-fail` of the library's tests, each failing, and their exact text is checked on
clang, GNU ld and wasm-ld alone; ld64's text is checked by a developer's run on macOS, which
the ledger records. The tools job gains no macOS or Windows leg, and an outcome's
`[native-darwin]` blocks apply only to such a local run.

**Files (superproject):**
- Create: `tools/test/crypto_jamfiles_test.py`; the fixtures of the test itself,
  `tools/test/fixtures/crypto_jamfiles/hello/` (`Jamfile`, `hello.cpp`, `hello.outcome`) and
  `tools/test/fixtures/crypto_jamfiles/broken/` (`Jamfile`, `broken.cpp`, `broken.outcome`).
- Modify: `CODING-GUIDELINES.md` (chapter 8, "Tests": the fixtures of `crypto_jamfiles` and
  their outcome files; chapter 1, "Process": the list of the tests of the build and of the
  tools, which names `emscripten_test.py`),
  `doc/releases/0.1.0.adoc` ("Tools and build": a line).

Every v0.1 backend is built by b2 (BearSSL, ctaes and libsecp256k1 by `third_party/Jamfile`,
OpenSSL by its provider), so no case finds a library on the machine or is left out without
one: the test has no roots, no requirements of the machine and no `--require-external`.

**Interfaces:**
- Produces: `tools/test/crypto_jamfiles_test.py` and its outcome files, which this task's
  Emscripten refusals and Tasks 11, 18, 24 and 29 add cases to (a directory under
  `tools/test/fixtures/crypto_jamfiles/` each).

**The outcome file** (`<case>.outcome`, beside the case's Jamfile):

```
# A comment line starts with #. Blocks apply by the target and the system:
# [all], [native], [native-linux], [native-darwin], [native-windows],
# [wasip2], [wasip3], [emscripten]; [all] and every block that matches apply.
[all]
build: succeeds
run: app
aes256_gcm ok backend=bearssl
```

- `targets: <target> ...` (first line of `[all]`, optional): the targets the case is built
  for; without it, native only;
- `build: succeeds` or `build: fails` (b2's exit status);
- after `build: fails`, every other line of the applying blocks must appear, whole, in b2's
  output, in any order (a compiler's or linker's message, an `error:` of b2's);
- `run: <program>`, after `build: succeeds`: the program the build made is run as the lane
  runs one (natively itself, `tools/boost/asio-wasip2/run.sh` or `tools/boost/asio-wasip3/run.sh`
  for a `.wasm` of wasip2 or wasip3, as `tools/ci/matrix.py`'s `wasm_lane` gives them,
  `node` for emscripten's `.js`), and every other line of the applying blocks is its standard
  output, exactly, line for line;
- `arguments: <word> ...`: words given to b2 for this case alone, each `@scratch` replaced by
  the case's scratch directory (a directory the test makes empty).

- [ ] **Step 1: Write the fixtures.** The fixtures of the test itself:
  `hello/Jamfile` is `exe hello : hello.cpp ;` (with the licence notice), `hello.cpp` prints
  `hello from a user's Jamfile`, `hello.outcome` is `[all]`, `targets: native wasip2`,
  `build: succeeds`, `run: hello`, `hello from a user's Jamfile`; `broken/Jamfile` is
  `exe broken : broken.cpp ;`, `broken.cpp` `#error "broken on purpose"`, and its outcome
  `build: fails`, `broken on purpose`. The refusals of OpenSSL on Emscripten, which
  `webcpp.compile-diagnostic` cannot test (it is clang's alone and skips the toolset
  `emscripten`), as Task 6 writes them in `config.hpp`, each a program that includes
  `<webcpp/crypto/config.hpp>`, given the define without b2's alias, as a user without b2 would:
  `openssl_on_emscripten/Jamfile` is
  `exe app : app.cpp /webcpp/crypto//crypto : <define>WEBCPP_CRYPTO_HAS_OPENSSL=1 ;`, and its
  outcome `[all]`, `targets: emscripten`, `build: fails`,
  `webcpp::crypto: OpenSSL is not built for Emscripten. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl.`;
  `default_openssl_on_emscripten/Jamfile` the same with
  `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`, and its outcome's line
  `webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL selects OpenSSL, which is not built for Emscripten. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL.`
- [ ] **Step 2: Write `tools/test/crypto_jamfiles_test.py`:**

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Builds every fixture of tools/test/fixtures/crypto_jamfiles, a Jamfile as a user of
webcpp::crypto writes it (b2's raw exe), with b2 as a user runs it, and checks its outcome file:
the build's status, the lines a failure prints, and a program's output (CODING-GUIDELINES.md
chapter 8). One scratch superproject per target, with libs/crypto and the fixtures in it; every
case of a target shares one build directory, so BearSSL's 295 files are compiled once per
target. Run with the names of some cases to run only those, and with --target <name> to run one
target."""

from __future__ import annotations

import platform
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

import harness

CASES = harness.FIXTURES / 'crypto_jamfiles'
TOOLSETS = {'native': [], 'wasip2': ['toolset=clang-wasip2'], 'wasip3': ['toolset=clang-wasip3'],
            'emscripten': ['toolset=emscripten']}
SYSTEM = {'Linux': 'linux', 'Darwin': 'darwin', 'Windows': 'windows'}[platform.system()]


def launcher(target: str) -> Path:
    """The script the lanes run a WASI program with (tools/ci/matrix.py, wasm_lane)."""
    return harness.ROOT / f'tools/boost/asio-{target}/run.sh'


@dataclass
class Outcome:
    """What an outcome file says for one target."""
    targets: list[str] = field(default_factory=lambda: ['native'])
    succeeds: bool = True
    run: str | None = None
    lines: list[str] = field(default_factory=list)
    arguments: list[str] = field(default_factory=list)


def outcome(path: Path, target: str) -> Outcome:
    """The blocks of path that apply to target on this system, read into an Outcome."""
    applying = {'all', target} | ({'native-' + SYSTEM} if target == 'native' else set())
    result = Outcome()
    block = None
    for raw in path.read_text().splitlines():
        if not raw.strip() or raw.startswith('#'):
            continue
        if raw.startswith('[') and raw.endswith(']'):
            block = raw[1:-1]
            continue
        if block is None:
            raise SystemExit(f'{path}: a line before the first [block]: {raw!r}')
        if block not in applying:
            continue
        key, _, value = raw.partition(': ')
        if key == 'targets':
            result.targets = value.split()
        elif key == 'build':
            result.succeeds = value == 'succeeds'
        elif key == 'run':
            result.run = value
        elif key == 'arguments':
            result.arguments += value.split()
        else:
            result.lines.append(raw)
    return result


def program_command(build_dir: Path, name: str, target: str) -> list[str]:
    """The command that runs the program name the case's build made, as its lane runs one."""
    suffix = {'native': '.exe' if SYSTEM == 'windows' else '', 'wasip2': '.wasm',
              'wasip3': '.wasm', 'emscripten': '.js'}[target]
    found = [p for p in build_dir.rglob(name + suffix) if p.is_file()]
    if len(found) != 1:
        raise AssertionError(f'the build made {len(found)} files named {name}{suffix}')
    if target == 'emscripten':
        return ['node', str(found[0])]
    if target.startswith('wasi'):
        return [str(launcher(target)), str(found[0])]
    return [str(found[0])]


def run_target(root: Path, target: str, names: list[str]) -> list[str]:
    """Builds every case of target in root and returns the faults, one line each."""
    faults: list[str] = []
    build_dir = root / 'bin' / f'user-{target}'
    for case in sorted(p.parent for p in CASES.glob('*/*.outcome')):
        if names and case.name not in names:
            continue
        expected = outcome(next(case.glob('*.outcome')), target)
        if target not in expected.targets:
            continue
        scratch = root / 'scratch' / case.name
        scratch.mkdir(parents=True, exist_ok=True)
        arguments = [a.replace('@scratch', str(scratch)) for a in expected.arguments]
        launch = [f'testing.launcher={launcher(target)}'] if target.startswith('wasi') else []
        built = harness.run_b2(root, f'--build-dir={build_dir}', *TOOLSETS[target], *launch,
                               *arguments, f'user/{case.name}')
        output = built.stdout
        if (built.returncode == 0) != expected.succeeds:
            faults.append(f'{case.name} on {target}: the build '
                          f'{"succeeded" if built.returncode == 0 else "failed"}\n{output[-3000:]}')
            continue
        if not expected.succeeds:
            faults += [f'{case.name} on {target}: b2 did not print: {line}'
                       for line in expected.lines if line not in output]
            continue
        if expected.run is not None:
            command = program_command(build_dir / 'user' / case.name, expected.run, target)
            ran = subprocess.run(command, capture_output=True, text=True, check=False)
            printed = ran.stdout.replace('\r\n', '\n').splitlines()
            if ran.returncode != 0 or printed != expected.lines:
                faults.append(f'{case.name} on {target}: {expected.run} exited {ran.returncode} '
                              f'and printed {printed!r}; the outcome says {expected.lines!r}')
    return faults


def main(argv: list[str]) -> int:
    targets = list(TOOLSETS)
    if '--target' in argv:
        targets = [argv[argv.index('--target') + 1]]
        argv = [a for a in argv if a not in ('--target', targets[0])]
        if targets[0] not in TOOLSETS:
            raise SystemExit(f'crypto_jamfiles: no target {targets[0]}; the targets are {list(TOOLSETS)}')
    names = [a for a in argv if not a.startswith('--')]
    faults: list[str] = []
    for target in targets:
        started = time.monotonic()
        root = harness.scratch_superproject()
        try:
            harness.copy_integration_libraries(root, 'crypto')
            shutil.copytree(CASES, root / 'user', ignore=harness.built)
            if target == 'emscripten':
                harness.link_emsdk(root)
            elif target.startswith('wasi'):
                harness.link_wasi_tools(root)
            faults += run_target(root, target, names)
        finally:
            shutil.rmtree(root)
        print(f'{target}: done in {time.monotonic() - started:.0f} s')
    for fault in faults:
        print(f'crypto_jamfiles: {fault}', file=sys.stderr)
    print('crypto_jamfiles_test: ' + ('failed' if faults else 'ok'))
    return 1 if faults else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

  A scratch copy links `third_party/libs` to the checkout's and leaves `libs/` out
  (`tools/test/harness.py`, `copy_tree`); `copy_integration_libraries` brings crypto's
  `build.jam`, `include` and `src`. A target whose toolset the machine lacks (`clang-wasip2`
  not registered, no emsdk) stops the run with `harness`'s own message, never a silent skip.
  The CI's tools job runs it as it runs every `tools/**/*_test.py`: a test that does not call
  `harness.run_cases` runs whole, without arguments, in one of the job's eight shards
  (`.github/workflows/library.yml`, job `tools`); the job needs no new step.

  The cost (m20): the cases of a target run one after another, never two inner b2 at once,
  so each inner b2 keeps its default of every CPU and no outer `-j` multiplies it; and they
  share one build directory per target (`bin/user-<target>` in the scratch root), so
  BearSSL's 295 files, libsecp256k1's and OpenSSL's provider are built once per target, not
  once per case. A shard of the tools job has 90 minutes; the test prints each target's
  time (`native: done in <s> s`), Task 25's green records the four in the ledger, and when
  their sum passes 30 minutes the test moves to a job of its own in `library.yml`, one
  matrix entry per target (`crypto_jamfiles_test.py --target <name>`, which `main` takes), and
  the tools job's loop leaves it out by name.
- [ ] **Step 3: Run** `python3 tools/test/crypto_jamfiles_test.py hello broken`. Expected:
  `native: done in <s> s` and `wasip2: done in <s> s`, then `crypto_jamfiles_test: ok`. Change `hello.outcome`'s
  last line to `hello from elsewhere`: expected `crypto_jamfiles_test: failed` with the
  printed and the expected lines; put it back. Change `broken.outcome`'s message to `not this`:
  expected a fault naming the missing line; put it back. Then
  `python3 tools/test/crypto_jamfiles_test.py --target emscripten openssl_on_emscripten default_openssl_on_emscripten`.
  Expected: `crypto_jamfiles_test: ok`.
- [ ] **Step 4: Run** `python3 tools/test/third_party_test.py`, `python3 tools/test/harness_test.py`
  and the lint. Expected: PASS and `lint: clean` (the fixtures' raw `exe` is outside the
  raw-rules pattern).
- [ ] **Step 5: Document** in `CODING-GUIDELINES.md` chapter 8, "Tests", the fixtures of
  `crypto_jamfiles` (a user's Jamfile per case, its outcome file's format, the test that builds
  them, and why they are fixtures of the tools' test and not a library's tests), and name
  `crypto_jamfiles_test.py` in chapter 1's list of the tests of the build and of the tools,
  beside `emscripten_test.py`; `doc/releases/0.1.0.adoc`, "Tools and build", gains a line: the user's
  Jamfiles of webcpp::crypto, tested as fixtures.
- [ ] **Step 6: Commit** (after `git add -- tools/test/crypto_jamfiles_test.py tools/test/fixtures/crypto_jamfiles`):
  `"tools: the user's Jamfiles of webcpp::crypto as fixtures of a test of the tools"`
  (`tools/test/crypto_jamfiles_test.py tools/test/fixtures/crypto_jamfiles CODING-GUIDELINES.md
  doc/releases/0.1.0.adoc`).

## Task 11: crypto's OpenSSL alias over the source-built provider

OpenSSL is not found on the machine: the superproject builds OpenSSL 3.5.9 (`45e844f`) from
the submodule `third_party/libs/openssl` as `/webcpp/third_party//openssl`, static, through
`tools/webcpp-openssl.jam` (CODING-GUIDELINES.md chapter 5 rule 7), for native Clang, GCC and
MSVC, wasip2 single-threaded and wasip3 single or cooperative; Emscripten stops. The provider's
usage requirements give its generated include directory, `libssl` and `libcrypto` in link order
and their link flags, and no define (`tools/webcpp-openssl.jam:96-133`). crypto's backend needs
`WEBCPP_CRYPTO_HAS_OPENSSL`, so crypto wraps the provider in an alias of its own, as asio does
(`libs/asio` at `6c4e2c8`, `build.jam:104-126`). No OpenSSL canary: the provider forbids
injecting into upstream, and its archives are named by exact path (chapter 5 rule 7; chapter 19,
"Asio/Beast closure").

**Files:**
- Modify (library): `build.jam` (the alias `openssl`), `test/Jamfile` (`openssl_one_copy`),
  `doc/changelog.adoc`.
- Create (library): `src/openssl_on_emscripten.hpp`, `test/openssl_one_copy_test.cpp`.
- Create (superproject): the fixtures `tools/test/fixtures/crypto_jamfiles/openssl_on_emscripten/`
  and `openssl_linked/`, each a `Jamfile`, an `app.cpp` and an outcome file.
- Modify (superproject): `CODING-GUIDELINES.md` (chapter 10's `openssl` row, chapter 17's
  crypto section; Step 5).

**Interfaces:**
- Consumes: `/webcpp/third_party//openssl` (the provider); `webcpp-target.target-of`
  (`tools/target.jam:26`); Task 10's `tools/test/crypto_jamfiles_test.py`.
- Produces: `/webcpp/crypto//openssl`: on native, wasip2 and wasip3,
  `<library>/webcpp/third_party//openssl <define>WEBCPP_CRYPTO_HAS_OPENSSL=1`; on Emscripten,
  `src/openssl_on_emscripten.hpp` forced into every C and C++ source, whose `#error` names the
  fix. Every later task names OpenSSL this way, never the provider directly.

- [ ] **Step 1: Write the failing fixtures**, each a user's Jamfile with its licence notice
  (Task 10's format):

| Case | Its Jamfile | Outcome |
| --- | --- | --- |
| `openssl_on_emscripten` | `exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ;` | `targets: emscripten`; `build: fails`; `webcpp::crypto: /webcpp/crypto//openssl is named in a build for Emscripten, where webcpp does not build OpenSSL.` |
| `openssl_linked` | the same | `targets: native wasip2 wasip3`; `build: succeeds`; `run: app`; `HAS_OPENSSL=1`; `tag 530f8afbc74536b9a963b4f1c4cb738b` |

  Both cases share one `app.cpp`: it prints `HAS_OPENSSL=1` when the define is set, then seals
  an empty message under the zero key of 32 bytes and the zero IV of 12 bytes with
  `gcm<aes256<backends::openssl>>` and prints `tag <hex>` (McGrew and Viega's test case 13).
  Until Task 12 writes the backend's header, `openssl_linked`'s app prints the define and calls
  `OpenSSL_version_num()` alone; Task 12 Step 3 gives it the seal. Run
  `python3 tools/test/crypto_jamfiles_test.py openssl_on_emscripten openssl_linked`. Expected:
  FAIL, no target `openssl` in `/webcpp/crypto`.
- [ ] **Step 2: Write `src/openssl_on_emscripten.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_SRC_OPENSSL_ON_EMSCRIPTEN_HPP
#define WEBCPP_CRYPTO_SRC_OPENSSL_ON_EMSCRIPTEN_HPP

// Forced into every source when an Emscripten program names /webcpp/crypto//openssl.
#if defined(__EMSCRIPTEN__)
#error \
    "webcpp::crypto: /webcpp/crypto//openssl is named in a build for Emscripten, where webcpp does not build OpenSSL. In the program's Jamfile, replace /webcpp/crypto//openssl with /third_party//bearssl, or name OpenSSL for native and WASI builds alone, by a condition in the project's requirements: project : requirements <conditional>@native-openssl ; rule native-openssl ( properties * ) { if ! <toolset>emscripten in $(properties) { return <library>/webcpp/crypto//openssl ; } } (without b2: link OpenSSL into native and WASI programs only)"
#endif
#endif  // WEBCPP_CRYPTO_SRC_OPENSSL_ON_EMSCRIPTEN_HPP
```

  The text holds `$(properties)` as b2 source the user copies; in the header it is a string, and
  b2 never reads it.
- [ ] **Step 3: Write the alias** in `libs/crypto/build.jam`, after `alias crypto`, with
  `import webcpp-target ;` among its imports:

```jam
# tag::openssl[]
# OpenSSL, the backend backends::openssl: the superproject's source-built
# OpenSSL 3.5.9 (/webcpp/third_party//openssl, static; CODING-GUIDELINES.md
# chapter 5 rule 7) with the define the backend's header checks, on native,
# wasip2 and wasip3. Never a system OpenSSL, never shared.
rule openssl-linked ( properties * )
{
    if [ webcpp-target.target-of $(properties) ] in native wasip2 wasip3
    {
        return <library>/webcpp/third_party//openssl
            <define>WEBCPP_CRYPTO_HAS_OPENSSL=1 ;
    }
}

# Refused on Emscripten: every source of a program built for emscripten that
# names it stops at the forced include, whose message says what to write.
# Quoted, as the Jamroot quotes Boost's directory, for a path that holds a
# space.
path-constant WEBCPP_CRYPTO_ROOT : . ;
alias openssl : : : : <conditional>@openssl-linked
    <toolset>emscripten:<cflags>"-include \"$(WEBCPP_CRYPTO_ROOT)/src/openssl_on_emscripten.hpp\""
    <toolset>emscripten:<cxxflags>"-include \"$(WEBCPP_CRYPTO_ROOT)/src/openssl_on_emscripten.hpp\"" ;
# end::openssl[]
```

  A program that names both `/webcpp/asio//openssl` and `/webcpp/crypto//openssl` gets the one
  provider target under one property set, so each archive once (chapter 5 rule 10). The test
  of that, `test/openssl_one_copy_test.cpp`, prints both defines and the version:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// asio's and crypto's OpenSSL aliases in one program: both defines, one OpenSSL 3.5.9.

#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <openssl/crypto.h>

int main() {
#if !defined(WEBCPP_CRYPTO_HAS_OPENSSL) || !defined(WEBCPP_ASIO_HAS_OPENSSL)
    BOOST_ERROR("both aliases give their define");
#endif
    BOOST_TEST_EQ(OpenSSL_version_num(), 0x30500090UL);  // tools/openssl/pin.json: 3.5.9
    return webcpp::test::exit_status(boost::report_errors());
}
```

  and in `test/Jamfile`:

```jam
# asio's and crypto's OpenSSL in one program: one provider, each archive once
# (CODING-GUIDELINES.md chapter 5 rule 10).
crypto-run openssl_one_copy : openssl_one_copy_test.cpp
  : <library>/webcpp/crypto//openssl <library>/webcpp/asio//openssl : native wasip2 wasip3 ;
```

  `doc/changelog.adoc` gains `* /webcpp/crypto//openssl links the superproject's OpenSSL 3.5.9
  on native, wasip2 and wasip3, and refuses Emscripten with the line to write.`
- [ ] **Step 4: Run** the fixtures, the test and the link line:

```bash
python3 tools/test/crypto_jamfiles_test.py openssl_on_emscripten openssl_linked
$B2 -a --build-dir=bin/lane-native libs/crypto/test//openssl_one_copy
$B2 -a -d+2 --build-dir=bin/lane-native libs/crypto/test//openssl_one_copy \
  | grep -oE '[^ "]*/lib(ssl|crypto)\.a' | sort | uniq -c
$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 \
  testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test//openssl_one_copy
$B2 -a --build-dir=bin/lane-wasip3 toolset=clang-wasip3 \
  testing.launcher=tools/boost/asio-wasip3/run.sh libs/crypto/test//openssl_one_copy
```

  Expected: `crypto_jamfiles_test: ok`; `openssl_one_copy` and its `_no_exceptions` twin pass
  on the three targets; the `grep` prints one line per archive and program, each count 1, both
  archives under one `config-<fingerprint>` directory (the twin has its own: measure there
  whether it reconfigures OpenSSL, and record the seconds in the ledger). With
  `third_party/libs/openssl` not checked out, the build stops with the provider's
  `OpenSSL sources are missing. Run: git submodule update --init third_party/libs/openssl`
  (`tools/webcpp-openssl.jam:44-46`).
- [ ] **Step 5: The rulebook.** In `CODING-GUIDELINES.md`:
  - chapter 10, "Third-party code", the `openssl` row: "for native Clang/GCC/MSVC and WASIp3"
    becomes "for native Clang/GCC/MSVC, WASIp2 single and WASIp3 single/cooperative" (what
    `tools/openssl/build.py:186-209` builds, and chapter 15's asio section says);
  - chapter 17, crypto, "Provider amendment, 2026-11-03": "whose current support is native and
    WASIp3" becomes "whose current support is native, WASIp2 single and WASIp3, and which crypto
    names through its own alias `/webcpp/crypto//openssl`";
  - the bullet "OpenSSL is the machine's libcrypto, 3.0 or later, found and never built, native
    only, ..." becomes: "**OpenSSL is the superproject's source-built OpenSSL 3.5.9, static,
    named through `/webcpp/crypto//openssl`, which gives `WEBCPP_CRYPTO_HAS_OPENSSL` on native,
    wasip2 and wasip3 and refuses Emscripten by a forced `#error`; the backend's header stops
    with an `#error` when it is not linked or older than 3.0.** Version 3 is needed for
    `EVP_CIPHER_fetch`; the alias follows asio's, and OpenSSL has no canary (chapter 5 rule 7).
    Source: `libs/crypto/build.jam`, `tools/webcpp-openssl.jam` (superproject).";
  - the bullet "OpenSSL and Botan are found by `webcpp.external` ..." keeps Botan alone:
    "**Botan is found by `webcpp.external` ... (later work, increment 5).**", OpenSSL's half and
    `webcpp-own` removed;
  - the bullet "OpenSSL's link, shared or static, is the propagated feature
    `webcpp-crypto-openssl-link` ..." is removed (Botan's feature returns with increment 5);
  - the bullet "One copy per program ...": "and asio's OpenSSL moves onto
    `/third_party//openssl`" becomes "and asio and crypto name the one source-built OpenSSL";
    "a process with two copies of libcrypto has two memory hooks, and the install sets only the
    one webcpp's backend calls" becomes "two copies of libcrypto would be two error queues and
    two allocation hooks in one process".
- [ ] **Step 6: Commit** (library, after `git add` of the new files)
  `git -C libs/crypto commit -m "crypto: /webcpp/crypto//openssl over the source-built OpenSSL, refused on Emscripten" -- build.jam src/openssl_on_emscripten.hpp test/Jamfile test/openssl_one_copy_test.cpp doc/changelog.adoc`;
  (superproject, after `git add` of the fixtures)
  `git commit -m "tools: crypto's OpenSSL fixtures; CODING-GUIDELINES.md: crypto names the source-built OpenSSL" -- tools/test/fixtures/crypto_jamfiles/openssl_on_emscripten tools/test/fixtures/crypto_jamfiles/openssl_linked CODING-GUIDELINES.md`.
  After each, `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.

## Task 12: The OpenSSL backend: AES and AES-GCM, and OpenSSL's error queue

**Files:**
- Create: `include/webcpp/crypto/backends/openssl.hpp`, `test/support/aes_checks.hpp`,
  `test/gcm_openssl_test.cpp`, `test/gcm_limits_openssl_test.cpp`,
  `test/gcm_wycheproof_openssl_test.cpp`, `test/aes_openssl_test.cpp`,
  `test/openssl_error_queue_test.cpp`, `test/gcm_openssl_threads_test.cpp`,
  `test/messages/openssl_not_linked/`, `test/messages/openssl_old/` (with
  `include/openssl/opensslv.h`).
- Modify: `test/Jamfile`, `test/messages/Jamfile`, `include/webcpp/crypto.hpp` (Task 14 adds
  the conditional includes; nothing here), `doc/changelog.adoc`; (superproject)
  `tools/test/fixtures/crypto_jamfiles/openssl_linked/app.cpp` (the seal, Task 11).

**Interfaces:**
- Consumes: Task 7's and Task 8's engine contracts; `/webcpp/crypto//openssl` (Task 11).
- Produces: `struct webcpp::crypto::backends::openssl { static constexpr std::string_view name = "openssl"; };`
  the specialisations `gcm_state`, `gcm_engine`, `block_state`,
  `block_engine` for `backends::openssl` and key sizes 16, 24, 32;
  `boost::system::result<EVP_CIPHER_CTX*> webcpp::crypto::detail::openssl_context(char const* cipher, std::span<std::uint8_t const> key, bool encrypting, bool padding) noexcept`;
  the checks of `test/support/aes_checks.hpp`: `check_fips197<Backend>()`,
  `check_aes_guards<Backend>()`, `check_aes_moved_from<Backend>()`.

- [ ] **Step 1: Write the failing tests.** `test/gcm_openssl_test.cpp` is
  `gcm_bearssl_test.cpp` with `backends/openssl.hpp` and `openssl` in place of BearSSL's, every
  check of Tasks 7 and 8 called; `gcm_limits_openssl_test.cpp` and
  `gcm_wycheproof_openssl_test.cpp` likewise. The block
  cipher's checks, `test/support/aes_checks.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The checks of the AES block cipher on one backend: FIPS-197's appendix C, the guards and
// moved-from objects (spec sections 2.3 and 8.1).

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_AES_CHECKS_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_AES_CHECKS_HPP

#include "hex.hpp"

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace webcpp::test {

namespace crypto = webcpp::crypto;

/** Runs f with the block cipher of the key's size on Backend. */
template <class Backend, class F>
void with_aes(std::span<std::uint8_t const> key, F&& f) {
    switch (key.size()) {
        case 16: f(crypto::aes128<Backend>::make(key)); break;
        case 24: f(crypto::aes192<Backend>::make(key)); break;
        default: f(crypto::aes256<Backend>::make(key)); break;
    }
}

/** FIPS-197 appendix C.1, C.2 and C.3: the key 00 01 02 ..., the block 00 11 22 ... ff. */
template <class Backend>
void check_fips197() {
    struct example {
        std::size_t key_size;
        std::string_view ciphertext;
    };
    for (auto const& e : {example{16, "69c4e0d86a7b0430d8cdb78070b4c55a"},
                          example{24, "dda97ca4864cdfe06eaf70a0ec0d7191"},
                          example{32, "8ea2b7ca516745bfeafc49904b496089"}}) {
        std::vector<std::uint8_t> key(e.key_size);
        for (std::size_t i = 0; i < key.size(); ++i) {
            key[i] = static_cast<std::uint8_t>(i);
        }
        auto const plain = from_hex("00112233445566778899aabbccddeeff");
        with_aes<Backend>(key, [&](auto made) {
            if (!BOOST_TEST(made.has_value())) {
                return;
            }
            auto block = plain;
            BOOST_TEST(made->encrypt(block).has_value());
            BOOST_TEST_EQ(to_hex(block), e.ciphertext);
            BOOST_TEST(made->decrypt(block).has_value());
            BOOST_TEST(block == plain);
        });
    }
}

/** The block cipher's guards, in their order (spec section 2.3). */
template <class Backend>
void check_aes_guards() {
    std::array<std::uint8_t, 16> const key16{};
    BOOST_TEST(crypto::aes256<Backend>::make(key16).error() == crypto::error::invalid_key_size);
    std::array<std::uint8_t, 32> const key{};
    auto made = crypto::aes256<Backend>::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    std::array<std::uint8_t, 17> partial{};
    partial.fill(4);
    auto const untouched = partial;
    BOOST_TEST(made->encrypt(partial).error() == crypto::error::invalid_input_size);
    BOOST_TEST(made->decrypt(partial).error() == crypto::error::invalid_input_size);
    BOOST_TEST(partial == untouched);
    BOOST_TEST(made->encrypt(std::span<std::uint8_t>()).has_value());  // zero blocks
}

/** A moved-from block cipher answers invalid_state; the destination works. */
template <class Backend>
void check_aes_moved_from() {
    std::array<std::uint8_t, 16> key{};
    auto made = crypto::aes128<Backend>::make(key);
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    auto moved = std::move(*made);
    std::array<std::uint8_t, 16> block{};
    block.fill(0x6c);
    auto const untouched = block;
    BOOST_TEST(made->encrypt(block).error() == crypto::error::invalid_state);
    BOOST_TEST(made->decrypt(block).error() == crypto::error::invalid_state);
    BOOST_TEST(block == untouched);
    // Moving an empty object gives an empty one, by construction and by assignment (I1).
    auto again = std::move(*made);
    BOOST_TEST(again.encrypt(block).error() == crypto::error::invalid_state);
    again = std::move(*made);
    BOOST_TEST(again.decrypt(block).error() == crypto::error::invalid_state);
    std::swap(again, moved);
    BOOST_TEST(moved.encrypt(block).error() == crypto::error::invalid_state);
    block.fill(0);
    BOOST_TEST(again.encrypt(block).has_value());
    BOOST_TEST_EQ(to_hex(block), "66e94bd4ef8a2c3b884cfa59ca342b2e");  // AES-128, zero key and block
}

}  // namespace webcpp::test

#endif
```

  (Deferred after v0.1: NIST's AESAVS known answers, `check_aes_kat`, and
  `gcm_cavp_openssl`, with Task 9's NIST CAVP.) `test/aes_openssl_test.cpp` calls the three
  checks for `openssl`. `test/openssl_error_queue_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// OpenSSL's error queue (spec section 3.3): an entry of the caller's survives a make, a
// seal, a good open and a failed one, and nothing of webcpp's is left beside it.

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/backends/openssl.hpp>
#include <webcpp/crypto/gcm.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <openssl/err.h>

#include <array>
#include <cstdint>

namespace {

/** The queue's entries, oldest first, emptied. */
std::size_t drain(unsigned long& first) {
    std::size_t count = 0;
    first = ERR_peek_error();
    while (ERR_get_error() != 0) {
        ++count;
    }
    return count;
}

}  // namespace

int main() {
    namespace crypto = webcpp::crypto;
    using aead = crypto::gcm<crypto::aes256<crypto::backends::openssl>>;
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 32> data{};

    ERR_raise(ERR_LIB_USER, ERR_R_PASSED_INVALID_ARGUMENT);
    unsigned long const ours = ERR_peek_last_error();
    auto made = aead::make(key);
    BOOST_TEST(made.has_value());
    auto const tag = made->seal(iv, {}, data);
    BOOST_TEST(tag.has_value());
    BOOST_TEST(made->open(iv, {}, data, *tag).has_value());
    std::array<std::uint8_t, 16> bad{};
    BOOST_TEST(made->open(iv, {}, data, bad).error() == crypto::error::authentication_failed);
    unsigned long first = 0;
    BOOST_TEST_EQ(drain(first), 1U);
    BOOST_TEST_EQ(first, ours);
    return webcpp::test::exit_status(boost::report_errors());
}
```

  The two message tests, each a directory with a `webcpp.compile-diagnostic` on native and
  wasip2:
  `openssl_not_linked` (`<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`, the source including
  `backends/openssl.hpp`, stating spec 4.3's fourth row) and `openssl_old`
  (`<define>WEBCPP_CRYPTO_HAS_OPENSSL=1 <include>include`, whose `include/openssl/opensslv.h`
  is `#define OPENSSL_VERSION_NUMBER 0x1010117fL` under its licence notice and a guard, and
  whose source states
  `webcpp::crypto: the backend OpenSSL needs OpenSSL 3.0 or later, and the headers found are of an older OpenSSL. In the program's Jamfile, name /webcpp/crypto//openssl, which builds OpenSSL 3.5.9. (without b2: put OpenSSL 3's include/ first)`).

  `test/gcm_openssl_threads_test.cpp`: two `std::thread`s, each with an `aead` of its own made
  on that thread, seal and open 1,000 messages of 1 KiB with the error queue test's check that
  nothing is left in the thread's queue. It is built `<threading>multi`, natively only (wasip2
  has no threads, `tools/webcpp-openssl.jam:146-149`): the provider follows the consumer's
  `<threading>`, so that the ordinary lanes' OpenSSL is configured `no-threads`
  (`tools/openssl/build.py:177-178`), whose error queue is the process's, and a program that
  runs OpenSSL on two threads must be built `<threading>multi` (the backend's page says so,
  Task 24).

  In `test/Jamfile`, each OpenSSL test declared on native, wasip2 and wasip3 with
  `<library>/webcpp/crypto//openssl` (and `<library>/third_party//wycheproof
  <library>/webcpp//boost_json` for the vectors):

```jam
# The backend OpenSSL: the source-built OpenSSL 3.5.9 on native, wasip2 and
# wasip3 (Task 11); Emscripten has none.
crypto-run gcm_openssl : gcm_openssl_test.cpp : <library>/webcpp/crypto//openssl
  : native wasip2 wasip3 ;
crypto-run gcm_limits_openssl : gcm_limits_openssl_test.cpp
  : <library>/webcpp/crypto//openssl : native wasip2 wasip3 ;
crypto-run gcm_wycheproof_openssl : gcm_wycheproof_openssl_test.cpp
  : <library>/webcpp/crypto//openssl <library>/third_party//wycheproof
    <library>/webcpp//boost_json : native wasip2 wasip3 ;
crypto-run aes_openssl : aes_openssl_test.cpp : <library>/webcpp/crypto//openssl
  : native wasip2 wasip3 ;
crypto-run openssl_error_queue : openssl_error_queue_test.cpp
  : <library>/webcpp/crypto//openssl : native wasip2 wasip3 ;
# OpenSSL on two threads: the provider configured with threads.
crypto-run gcm_openssl_threads : gcm_openssl_threads_test.cpp
  : <library>/webcpp/crypto//openssl <threading>multi : native ;
```

  Run the native lane. Expected: FAIL, `backends/openssl.hpp` not found.
- [ ] **Step 2: Write `backends/openssl.hpp`:** layer 1 and the version check:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The backend OpenSSL: OpenSSL 3.5.9 built from `third_party/libs/openssl` by
 `/webcpp/crypto//openssl`, static, on native, wasip2 and wasip3; never on Emscripten. It
 serves `aes128/192/256<backends::openssl>` and `gcm<...<backends::openssl>>`. Its `make`
 allocates through OpenSSL's own functions. On MSVC, wasip2 and wasip3 OpenSSL is compiled
 without assembly: table-driven AES and 4-bit GHASH, no AES-NI, not constant time.
 OpenSSL's default initialisation reads the configuration file of the directory it was
 configured with (`/etc/ssl/openssl.cnf`, when the machine has one), as any program of
 OpenSSL does; what it raises meanwhile is popped with the backend's mark.

 @see "The backends", in the guide.
*/
#ifndef WEBCPP_CRYPTO_BACKENDS_OPENSSL_HPP
#define WEBCPP_CRYPTO_BACKENDS_OPENSSL_HPP

#include <webcpp/crypto/config.hpp>

#if !defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#error "webcpp::crypto: the backend OpenSSL is used and not linked. In the program's Jamfile, add /webcpp/crypto//openssl beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_OPENSSL=1 and link libcrypto)"
#else

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#error "webcpp::crypto: the backend OpenSSL needs OpenSSL 3.0 or later, and the headers found are of an older OpenSSL. In the program's Jamfile, name /webcpp/crypto//openssl, which builds OpenSSL 3.5.9. (without b2: put OpenSSL 3's include/ first)"
#else

#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/hash2/digest.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/result.hpp>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace webcpp::crypto::backends {

/** The backend OpenSSL (spec section 3.3). */
struct openssl {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "openssl";
};

}  // namespace webcpp::crypto::backends

namespace webcpp::crypto::detail {

/** EVP takes lengths as `int`: a span goes in pieces of at most this many bytes, whole blocks. */
inline constexpr std::size_t openssl_piece = static_cast<std::size_t>(INT_MAX) / 16 * 16;

/** OpenSSL's name of AES-GCM with keys of `KeySize` bytes. */
template <std::size_t KeySize>
inline constexpr char const* openssl_gcm_name =
    KeySize == 16 ? "AES-128-GCM" : KeySize == 24 ? "AES-192-GCM" : "AES-256-GCM";

/** OpenSSL's name of the AES block cipher, ECB without padding, of `KeySize` bytes. */
template <std::size_t KeySize>
inline constexpr char const* openssl_ecb_name =
    KeySize == 16 ? "AES-128-ECB" : KeySize == 24 ? "AES-192-ECB" : "AES-256-ECB";

/**
 * A context keyed for `cipher`, or the error of the call that failed (spec section 3.3):
 * `errc::not_enough_memory` when `EVP_CIPHER_CTX_new` returns null, `backend_failure` when
 * `EVP_CIPHER_fetch` returns null or the key's `EVP_CipherInit_ex` fails. The thread's
 * error queue is left as it was found, through a mark set before the first call that can
 * raise.
 *
 * @param cipher OpenSSL's name of the cipher.
 * @param key The key, of the cipher's size.
 * @param encrypting The context's direction.
 * @param padding Whether the block cipher pads; GCM ignores it.
 * @return The context, which the caller frees with `EVP_CIPHER_CTX_free`.
 */
inline boost::system::result<EVP_CIPHER_CTX*> openssl_context(char const* cipher,
                                                              std::span<std::uint8_t const> key,
                                                              bool encrypting,
                                                              bool padding) noexcept {
    ERR_set_mark();
    EVP_CIPHER* const fetched = EVP_CIPHER_fetch(nullptr, cipher, nullptr);
    if (fetched == nullptr) {
        ERR_pop_to_mark();
        return error::backend_failure;
    }
    EVP_CIPHER_CTX* const context = EVP_CIPHER_CTX_new();
    if (context == nullptr) {
        EVP_CIPHER_free(fetched);
        ERR_pop_to_mark();
        return boost::system::errc::make_error_code(boost::system::errc::not_enough_memory);
    }
    // The context holds a reference of its own to the fetched cipher.
    bool const keyed = EVP_CipherInit_ex(context, fetched, nullptr, key.data(), nullptr,
                                         encrypting ? 1 : 0) == 1 &&
                       EVP_CIPHER_CTX_set_padding(context, padding ? 1 : 0) == 1;
    EVP_CIPHER_free(fetched);
    ERR_pop_to_mark();
    if (!keyed) {
        EVP_CIPHER_CTX_free(context);
        return error::backend_failure;
    }
    return context;
}

/**
 * The error of a call of a message's that OpenSSL's contract says does not fail: the queue
 * is cleared, so that nothing of webcpp's is left for OpenSSL's other users, the caller's
 * earlier entries with it (spec section 3.3).
 */
inline boost::system::error_code openssl_failed() noexcept {
    ERR_clear_error();
    return error::backend_failure;
}

/**
 * Runs `EVP_CipherUpdate` over `data` in place, in pieces EVP's `int` can hold.
 *
 * @return Nothing, or `backend_failure`.
 */
inline boost::system::result<void> openssl_update(EVP_CIPHER_CTX* context,
                                                  std::span<std::uint8_t> data) noexcept {
    for (std::size_t at = 0; at < data.size(); at += openssl_piece) {
        std::size_t const piece = (std::min)(openssl_piece, data.size() - at);
        int written = 0;
        if (EVP_CipherUpdate(context, data.data() + at, &written, data.data() + at,
                             static_cast<int>(piece)) != 1 ||
            static_cast<std::size_t>(written) != piece) {
            return openssl_failed();
        }
    }
    return {};
}

/** GCM's state on OpenSSL: one context, keyed once, its expanded key OpenSSL's. */
template <std::size_t KeySize>
struct gcm_state<backends::openssl, KeySize> {
    EVP_CIPHER_CTX* context = nullptr;
};

/** GCM's work on OpenSSL. */
template <std::size_t KeySize>
struct gcm_engine<backends::openssl, KeySize> {
    using state = gcm_state<backends::openssl, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        auto const made = openssl_context(openssl_gcm_name<KeySize>, key, true, true);
        if (!made) {
            return made.error();
        }
        s.context = *made;
        return {};
    }

    // Each message re-initialises the context with a null key, which keeps the expanded one,
    // and switches its direction (CR-PROBES ossl_open.c: "good open: ok=1").
    static boost::system::result<void> start(state& s, bool sealing,
                                             std::span<std::uint8_t const> iv,
                                             std::span<std::uint8_t const> additional_data) noexcept {
        int const direction = sealing ? 1 : 0;
        if (EVP_CipherInit_ex(s.context, nullptr, nullptr, nullptr, nullptr, direction) != 1 ||
            EVP_CIPHER_CTX_ctrl(s.context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()),
                                nullptr) != 1 ||
            EVP_CipherInit_ex(s.context, nullptr, nullptr, nullptr, iv.data(), direction) != 1) {
            return openssl_failed();
        }
        // A null output makes EVP_CipherUpdate take additional data; none is fed as no call.
        for (std::size_t at = 0; at < additional_data.size(); at += openssl_piece) {
            std::size_t const piece = (std::min)(openssl_piece, additional_data.size() - at);
            int written = 0;
            if (EVP_CipherUpdate(s.context, nullptr, &written, additional_data.data() + at,
                                 static_cast<int>(piece)) != 1) {
                return openssl_failed();
            }
        }
        return {};
    }

    static boost::system::result<void> run(state& s, std::span<std::uint8_t> data) noexcept {
        return openssl_update(s.context, data);
    }

    static boost::system::result<void> tag(state& s, boost::hash2::digest<16>& out) noexcept {
        std::array<std::uint8_t, 16> final_block{};
        int written = 0;
        if (EVP_EncryptFinal_ex(s.context, final_block.data(), &written) != 1 ||
            EVP_CIPHER_CTX_ctrl(s.context, EVP_CTRL_GCM_GET_TAG, 16, out.data()) != 1) {
            return openssl_failed();
        }
        return {};
    }

    static boost::system::result<void> check(state& s,
                                             std::array<std::uint8_t, 16> const& tag) noexcept {
        // OpenSSL takes the expected tag through a pointer to mutable bytes, and only reads it.
        std::array<std::uint8_t, 16> expected = tag;
        if (EVP_CIPHER_CTX_ctrl(s.context, EVP_CTRL_GCM_SET_TAG, 16, expected.data()) != 1) {
            return openssl_failed();
        }
        std::array<std::uint8_t, 16> final_block{};
        int written = 0;
        // A tag that does not match raises nothing in the queue (CR-PROBES ossl_open.out).
        if (EVP_DecryptFinal_ex(s.context, final_block.data(), &written) != 1) {
            return error::authentication_failed;
        }
        return {};
    }

    // The next start re-initialises the context; this one resets the message now.
    static void abandon(state& s) noexcept {
        if (EVP_CipherInit_ex(s.context, nullptr, nullptr, nullptr, nullptr, -1) != 1) {
            ERR_clear_error();
        }
    }

    static void move(state& to, state& from) noexcept {
        to.context = std::exchange(from.context, nullptr);
    }

    // EVP_CIPHER_CTX_free cleanses the expanded key before it frees.
    static void reset(state& s) noexcept {
        EVP_CIPHER_CTX_free(std::exchange(s.context, nullptr));
    }
};

/**
 * The block cipher's state on OpenSSL: a context per direction, since a context keyed for
 * encryption keeps the encryption schedule only.
 */
template <std::size_t KeySize>
struct block_state<backends::openssl, KeySize> {
    EVP_CIPHER_CTX* encrypting = nullptr;
    EVP_CIPHER_CTX* decrypting = nullptr;
};

/** The block cipher's work on OpenSSL: ECB without padding over whole blocks. */
template <std::size_t KeySize>
struct block_engine<backends::openssl, KeySize> {
    using state = block_state<backends::openssl, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        auto const encrypting = openssl_context(openssl_ecb_name<KeySize>, key, true, false);
        if (!encrypting) {
            return encrypting.error();
        }
        auto const decrypting = openssl_context(openssl_ecb_name<KeySize>, key, false, false);
        if (!decrypting) {
            EVP_CIPHER_CTX_free(*encrypting);
            return decrypting.error();
        }
        s.encrypting = *encrypting;
        s.decrypting = *decrypting;
        return {};
    }

    static boost::system::result<void> encrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        return openssl_update(s.encrypting, blocks);
    }

    static boost::system::result<void> decrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        return openssl_update(s.decrypting, blocks);
    }

    static void move(state& to, state& from) noexcept {
        to.encrypting = std::exchange(from.encrypting, nullptr);
        to.decrypting = std::exchange(from.decrypting, nullptr);
    }

    static void reset(state& s) noexcept {
        EVP_CIPHER_CTX_free(std::exchange(s.encrypting, nullptr));
        EVP_CIPHER_CTX_free(std::exchange(s.decrypting, nullptr));
    }
};

}  // namespace webcpp::crypto::detail

#endif  // OPENSSL_VERSION_NUMBER
#endif  // WEBCPP_CRYPTO_HAS_OPENSSL
#endif  // WEBCPP_CRYPTO_BACKENDS_OPENSSL_HPP
```

  `openssl_update` with an empty span makes no call, which keeps a null `data()` away from
  OpenSSL (Review Focus 3).
- [ ] **Step 3: Run** the native lane, gcc-15's, wasip2's and wasip3's (their launchers
  `tools/boost/asio-wasip2/run.sh` and `tools/boost/asio-wasip3/run.sh`, as
  `tools/ci/matrix.py`'s `wasm_lane` gives them), then Task 11's fixture `openssl_linked`,
  whose `app.cpp` now seals (`python3 tools/test/crypto_jamfiles_test.py openssl_linked`).
  Expected: PASS: every OpenSSL program and its twin; the two message tests; the fixture's
  `tag 530f8afbc74536b9a963b4f1c4cb738b`. `gcm_wycheproof_openssl` agrees with the vectors as
  BearSSL's did (226, 81, 6, 3), so the cap of 128 bytes and the zeroing hold on OpenSSL
  too: OpenSSL 3.5.9 refuses an IV of 0 or more than 128 bytes
  (`providers/implementations/ciphers/ciphercommon_gcm.c:295`, `GCM_IV_MAX_SIZE` 1024 / 8 in
  `providers/implementations/include/prov/ciphercommon_gcm.h:21`), and a tag that does not
  match raises nothing (`ossl_gcm_cipher_final`, `ciphercommon_gcm_hw.c:38-47`). A difference
  here is a fault of the engine, never of the expected counts.
- [ ] **Step 4: Commit** (after `git add` of the new files), with `doc/changelog.adoc`'s line
  `* backends::openssl: AES and AES-GCM on OpenSSL 3.5.9, native, wasip2 and wasip3.`:
  `git -C libs/crypto commit -m "crypto: the OpenSSL backend: AES and AES-GCM on EVP, the error queue left as found" -- include test doc/changelog.adoc`;
  (superproject) `git commit -m "tools: crypto's openssl_linked fixture seals" -- tools/test/fixtures/crypto_jamfiles/openssl_linked/app.cpp`.
  After each, `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.

## Task 13: BearSSL and OpenSSL agree

**Files:**
- Create: `test/support/agreement.hpp`, `test/agreement_gcm_test.cpp`,
  `test/agreement_gcm_portable_test.cpp`.
- Modify: `test/Jamfile`.

**Interfaces:**
- Produces: `template <class A, class B> void webcpp::test::agree_gcm(std::uint64_t seed)`,
  which Task 35 extends to ChaCha20-Poly1305 after v0.1.

- [ ] **Step 1: Write the failing test.** `test/support/agreement.hpp` (spec 8.2):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Two backends on the same inputs (spec section 8.2): random keys of 16, 24 and 32 bytes,
// IVs of 12 bytes and of random lengths from 1 to 64, additional data of 0 to 64 bytes,
// messages of 0 to 4,096 bytes and every size within 1 of a multiple of 16 up to 4,096,
// from a seeded generator whose seed a failure prints. What one seals the other opens, and
// the ciphertexts and tags are byte-identical.

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_AGREEMENT_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_AGREEMENT_HPP

#include "gcm_checks.hpp"

#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

namespace webcpp::test {

/** Random bytes from the test's generator. */
inline std::vector<std::uint8_t> bytes(std::mt19937_64& generator, std::size_t size) {
    std::vector<std::uint8_t> out(size);
    for (auto& b : out) {
        b = static_cast<std::uint8_t>(generator());
    }
    return out;
}

/** One case on both backends. */
template <class A, class B, class Cipher_A, class Cipher_B>
void agree_once(std::mt19937_64& generator, std::size_t message_size, std::uint64_t seed) {
    auto const key = bytes(generator, Cipher_A::key_size);
    auto const iv = bytes(generator, generator() % 2 == 0 ? 12 : 1 + generator() % 64);
    auto const additional_data = bytes(generator, generator() % 65);
    auto const message = bytes(generator, message_size);
    auto a = crypto::gcm<Cipher_A>::make(key);
    auto b = crypto::gcm<Cipher_B>::make(key);
    if (!BOOST_TEST(a.has_value()) || !BOOST_TEST(b.has_value())) {
        return;
    }
    auto data_a = message;
    auto data_b = message;
    auto const tag_a = a->seal(iv, additional_data, data_a);
    auto const tag_b = b->seal(iv, additional_data, data_b);
    bool const same = tag_a && tag_b && data_a == data_b && to_hex(*tag_a) == to_hex(*tag_b);
    if (!BOOST_TEST(same)) {
        std::fprintf(stderr, "seed %llu, message of %zu bytes\n",
                     static_cast<unsigned long long>(seed), message_size);
        return;
    }
    BOOST_TEST(b->open(iv, additional_data, data_a, *tag_a).has_value());
    BOOST_TEST(a->open(iv, additional_data, data_b, *tag_b).has_value());
    BOOST_TEST(data_a == message && data_b == message);
}

/** Every case of spec section 8.2 for GCM, on backends A and B, for the three key sizes. */
template <class A, class B>
void agree_gcm(std::uint64_t seed) {
    std::mt19937_64 generator(seed);
    std::vector<std::size_t> sizes;
    for (std::size_t size = 0; size <= 4096; size += 16) {
        for (std::size_t near : {size == 0 ? 0 : size - 1, size, size + 1}) {
            if (near <= 4096) {
                sizes.push_back(near);
            }
        }
    }
    for (int i = 0; i < 256; ++i) {
        sizes.push_back(static_cast<std::size_t>(generator() % 4097));
    }
    for (std::size_t size : sizes) {
        agree_once<A, B, crypto::aes128<A>, crypto::aes128<B>>(generator, size, seed);
        agree_once<A, B, crypto::aes192<A>, crypto::aes192<B>>(generator, size, seed);
        agree_once<A, B, crypto::aes256<A>, crypto::aes256<B>>(generator, size, seed);
    }
}

}  // namespace webcpp::test

#endif
```

  `test/agreement_gcm_test.cpp` includes both backends' headers and `support/agreement.hpp`,
  and calls `agree_gcm<bearssl, openssl>(20261011)` and, with the seed printed first,
  `agree_gcm<bearssl, openssl>(std::random_device{}())` (a test may use the standard library's
  device: the lint's world rule reads library headers only); `agreement_gcm_portable_test.cpp`
  is the same source with a different name. In `test/Jamfile`:

```jam
# BearSSL against OpenSSL (spec section 8.2), on native, wasip2 and wasip3.
# The portable twin keeps BearSSL on aes_ct64 and ghash_ctmul64 through the
# backend's test seam, so that on x86-64 both of BearSSL's code paths are
# compared with OpenSSL's. On WASI OpenSSL runs its portable C code (no-asm,
# tools/openssl/build.py:208), another pair of implementations.
crypto-run agreement_gcm : agreement_gcm_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl : native wasip2 wasip3 ;
crypto-run agreement_gcm_portable : agreement_gcm_portable_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl
    <define>WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE : native wasip2 wasip3 ;
```

  The two programs have sources of their own, so b2 compiles each with its own defines (spec
  4.3's pitfall). Run the native, wasip2 and wasip3 lanes. Expected: FAIL first on the missing
  header; then PASS.
- [ ] **Step 2: Prove that the seam selects what it says.** On the CI's x86-64 runner (the
  gcc-14 and clang-18 lanes), the first program's BearSSL runs AES-NI and PCLMUL and the twin
  `aes_ct64` and `ghash_ctmul64`: a check in each program, natively on x86-64 only
  (`#if defined(__x86_64__) || defined(_M_X64)`), compares
  `webcpp::crypto::detail::bearssl_ctr()` with `br_aes_x86ni_ctr_get_vtable()` (expected
  equal in the first unless the CPU lacks AES-NI, which the check then prints) and with
  `&br_aes_ct64_ctr_vtable` (expected equal in the twin).
- [ ] **Step 3: Commit** (after `git add` of the new files):
  `git -C libs/crypto commit -m "crypto: BearSSL and OpenSSL agree on AES-GCM, both of BearSSL's code paths" -- test`;
  then `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.

## Task 14: The default backend, the convenience headers, and their net on MSVC

**Files:**
- Create: `include/webcpp/crypto/aes256_gcm.hpp`, `test/default_both_test.cpp`,
  `test/default_selected_test.cpp`, `test/default_bearssl_test.cpp`,
  `test/default_both_wasm_test.cpp`, `test/default_openssl_wasm_test.cpp`.
- Modify: `include/webcpp/crypto.hpp`, `test/Jamfile`, `doc/changelog.adoc`.

**Interfaces:**
- Produces: `webcpp::crypto::aes128_gcm`, `webcpp::crypto::aes256_gcm`; the MSVC key
  `webcpp_crypto_default_backend` (Review Focus 4).

- [ ] **Step 1: Write the failing tests.** `test/default_both_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Natively, with OpenSSL and BearSSL linked, OpenSSL is the default (spec section 5), on MSVC
// too.

#include <webcpp/crypto/aes256_gcm.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

int main() {
    BOOST_TEST_EQ(webcpp::crypto::aes256_gcm::backend_type::name, "openssl");
    BOOST_TEST_EQ(webcpp::crypto::aes128_gcm::backend_type::name, "openssl");
    BOOST_TEST_EQ(webcpp::crypto::aes256<>::backend_type::name, "openssl");
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `default_selected_test.cpp` expects `"bearssl"` with both linked and
  `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` defined; `default_bearssl_test.cpp` expects
  `"bearssl"` with BearSSL alone, on every target. On WebAssembly OpenSSL may be linked
  (wasip2, wasip3) and is never the default unless selected (spec section 5; `config.hpp`
  refuses OpenSSL on Emscripten alone, Task 6): `default_both_wasm_test.cpp` expects
  `"bearssl"` with both linked, and `default_openssl_wasm_test.cpp` expects `"openssl"` with
  both linked and `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` defined. In `test/Jamfile`:

```jam
crypto-run default_both : default_both_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl : native ;
crypto-run default_selected : default_selected_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl
    <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL : native wasip2 wasip3 ;
crypto-run default_bearssl : default_bearssl_test.cpp : <library>/third_party//bearssl ;
crypto-run default_both_wasm : default_both_wasm_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl : wasip2 wasip3 ;
crypto-run default_openssl_wasm : default_openssl_wasm_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl
    <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL : wasip2 wasip3 ;
```

  Run the native lane. Expected: FAIL, `aes256_gcm.hpp` not found.
- [ ] **Step 2: Write `include/webcpp/crypto/aes256_gcm.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `aes128_gcm` and `aes256_gcm` on the default backend, whose header it includes: natively
 OpenSSL when it is linked, else BearSSL; on WebAssembly BearSSL, unless
 `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` selects OpenSSL on wasip2 or wasip3 (spec section 5).
 On MSVC, wasip2 and wasip3 the source-built OpenSSL has no assembly (table-driven AES,
 4-bit GHASH, no AES-NI, not constant time): a program that wants BearSSL there defines
 `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` or names `gcm<aes256<backends::bearssl>>`.

 @see "Using webcpp::crypto in your Jamfile", in the guide.
*/
#ifndef WEBCPP_CRYPTO_AES256_GCM_HPP
#define WEBCPP_CRYPTO_AES256_GCM_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/gcm.hpp>

#if defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL)
#include <webcpp/crypto/backends/openssl.hpp>
#else
#include <webcpp/crypto/backends/bearssl.hpp>
#endif

// Two files of one program that choose different defaults name two types aes256_gcm: on
// MSVC the link then fails with LNK2038 on this key (Review Focus 4).
#if defined(_MSC_VER) && defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL)
#pragma detect_mismatch("webcpp_crypto_default_backend", "openssl")
#elif defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_default_backend", "bearssl")
#endif

namespace webcpp::crypto {

/** AES-128-GCM on the default backend. */
using aes128_gcm = gcm<aes128<>>;
/** AES-256-GCM on the default backend. */
using aes256_gcm = gcm<aes256<>>;

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_AES256_GCM_HPP
```

  `include/webcpp/crypto.hpp` becomes:

```cpp
#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/error.hpp>
#include <webcpp/crypto/gcm.hpp>
#include <webcpp/crypto/random_device.hpp>  // lint-world: the system's entropy, a purpose of crypto

#if defined(WEBCPP_CRYPTO_HAS_BEARSSL)
#include <webcpp/crypto/backends/bearssl.hpp>
#endif
#if defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#include <webcpp/crypto/backends/openssl.hpp>
#endif
#if defined(WEBCPP_CRYPTO_HAS_BEARSSL) || defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#include <webcpp/crypto/aes256_gcm.hpp>
#endif
```

  (Increment 2's tasks add ctaes and `secp256k1.hpp` under their own `HAS` defines.) The
  headers-alone calls: `crypto/aes256_gcm.hpp` joins BearSSL's call (with
  `<library>/third_party//bearssl`), and a call on native, wasip2 and wasip3 takes
  `crypto/backends/openssl.hpp` with `<library>/webcpp/crypto//openssl`; `crypto.hpp` stays
  in the first call (no backend: it includes none).
- [ ] **Step 3: Run** the four lanes. Expected: PASS, the five tests on their targets.
- [ ] **Step 4: Commit** (after `git add` of the new files), with `doc/changelog.adoc`'s line
  `* aes128_gcm and aes256_gcm on the default backend; <webcpp/crypto.hpp>.`:
  `git -C libs/crypto commit -m "crypto: aes256_gcm on the default backend, and the convenience header" -- include test doc/changelog.adoc`;
  then `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.

## Task 15: OpenSSL's memory functions, an option of the config: the adapter and the install

Deferred after v0.1 (revision 2026-11-14). This task is kept as written for the memory
option's own change; before it runs, re-read it against the source-built provider
(CODING-GUIDELINES.md chapter 5 rule 7): OpenSSL is static, built by
`/webcpp/third_party//openssl` and named through `/webcpp/crypto//openssl` (Task 11), version
3.5.9, on native, wasip2 and wasip3; the feature `webcpp-crypto-openssl-link`, `OPENSSL_ROOT`
and the shared link it names are gone; the error values 13 and 14 are reserved and undeclared;
and the measured counts are 3.6.4's.

**Files:**
- Modify: `include/webcpp/crypto/backends/openssl.hpp`.
- Create: `test/openssl_memory/Jamfile`, `test/openssl_memory/adapter_test.cpp`.
- Modify: `test/Jamfile` (`build-project openssl_memory ;`).

**Interfaces:**
- Produces, under `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` (spec 3.7):
  `std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept;` (declared;
  the program defines it);
  `static boost::system::result<void> webcpp::crypto::backends::openssl::install_memory_functions() noexcept;`;
  `extern "C" void* webcpp_crypto_openssl_malloc(std::size_t, char const*, int) noexcept;`,
  `webcpp_crypto_openssl_realloc(void*, std::size_t, char const*, int)`,
  `webcpp_crypto_openssl_free(void*, char const*, int)`, inline, external linkage;
  `inline constexpr std::size_t webcpp::crypto::detail::openssl_block_alignment`;
  `bool webcpp::crypto::detail::openssl_adapter_installed() noexcept`;
  `bool webcpp::crypto::detail::openssl_defaults_installed() noexcept`.

- [ ] **Step 1: Write the failing test** `test/openssl_memory/adapter_test.cpp`, the adapter's
  edges called directly (spec 8.5), with a resource of the test's that counts, can be armed to
  fail, and is never destroyed:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The adapter's C functions, called directly (spec section 3.7): 0 bytes and a size whose
// header overflows give null without asking the resource; a block is aligned as malloc's;
// realloc grows, shrinks, frees at 0 and, when the resource fails, returns null and
// leaves the old block valid; a resource that throws (with exceptions) or returns null
// (without) is a null for OpenSSL.

#include <webcpp/crypto/backends/openssl.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/throw_exception.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory_resource>
#include <new>

namespace {

class test_resource final : public std::pmr::memory_resource {
public:
    std::size_t allocations = 0;
    std::size_t live = 0;
    bool failing = false;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (failing) {
#ifdef WEBCPP_CRYPTO_NO_EXCEPTIONS
            return nullptr;
#else
            boost::throw_exception(std::bad_alloc());
#endif
        }
        ++allocations;
        ++live;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }
    void do_deallocate(void* p, std::size_t size, std::size_t alignment) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(p, size, alignment);
    }
    bool do_is_equal(memory_resource const& other) const noexcept override {
        return this == &other;
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): never destroyed, by design
alignas(test_resource) unsigned char storage[sizeof(test_resource)];

test_resource& the_resource() noexcept {
    static auto* const resource = new (storage) test_resource;
    return *resource;
}

}  // namespace

std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept {
    return the_resource();
}

int main() {
    namespace detail = webcpp::crypto::detail;
    auto& resource = the_resource();
    BOOST_TEST(webcpp_crypto_openssl_malloc(0, __FILE__, __LINE__) == nullptr);
    BOOST_TEST(webcpp_crypto_openssl_malloc((std::numeric_limits<std::size_t>::max)() - 3,
                                            __FILE__, __LINE__) == nullptr);
    BOOST_TEST_EQ(resource.allocations, 0U);

    void* const block = webcpp_crypto_openssl_malloc(10, __FILE__, __LINE__);
    BOOST_TEST(block != nullptr);
    BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(block) % detail::openssl_block_alignment, 0U);
    std::memcpy(block, "0123456789", 10);

    void* const grown = webcpp_crypto_openssl_realloc(block, 1000, __FILE__, __LINE__);
    BOOST_TEST(grown != nullptr && std::memcmp(grown, "0123456789", 10) == 0);
    void* const shrunk = webcpp_crypto_openssl_realloc(grown, 4, __FILE__, __LINE__);
    BOOST_TEST(shrunk != nullptr && std::memcmp(shrunk, "0123", 4) == 0);

    resource.failing = true;
    BOOST_TEST(webcpp_crypto_openssl_malloc(8, __FILE__, __LINE__) == nullptr);
    BOOST_TEST(webcpp_crypto_openssl_realloc(shrunk, 64, __FILE__, __LINE__) == nullptr);
    BOOST_TEST(std::memcmp(shrunk, "0123", 4) == 0);  // the old block is still valid
    resource.failing = false;

    BOOST_TEST(webcpp_crypto_openssl_realloc(shrunk, 0, __FILE__, __LINE__) == nullptr);
    BOOST_TEST_EQ(resource.live, 0U);
    void* const fresh = webcpp_crypto_openssl_realloc(nullptr, 16, __FILE__, __LINE__);
    BOOST_TEST(fresh != nullptr);
    webcpp_crypto_openssl_free(fresh, __FILE__, __LINE__);
    webcpp_crypto_openssl_free(nullptr, __FILE__, __LINE__);
    BOOST_TEST_EQ(resource.live, 0U);
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/openssl_memory/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# OpenSSL's memory functions under WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE (spec
# sections 3.7 and 8.5): each test is a program of its own, since OpenSSL's
# functions are the process's and cannot be put back once allocated through;
# each runs with OpenSSL linked shared and static, with exceptions on and off.
project : requirements <library>/webcpp/crypto//crypto <library>/third_party//openssl
    <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE ;

import webcpp ;

webcpp.targets native ;

rule memory-run ( name : sources + : requirements * )
{
    webcpp.run $(name) : $(sources) : $(requirements) ;
    webcpp.run $(name)_no_exceptions : $(sources) : $(requirements) <exception-handling>off ;
    webcpp.run $(name)_static : $(sources)
      : $(requirements) <webcpp-crypto-openssl-link>static ;
    webcpp.run $(name)_static_no_exceptions : $(sources)
      : $(requirements) <webcpp-crypto-openssl-link>static <exception-handling>off ;
}

memory-run adapter : adapter_test.cpp ;
```

  Run the native lane. Expected: FAIL, `webcpp_crypto_openssl_malloc` undeclared.
- [ ] **Step 2: Write the option** in `backends/openssl.hpp`. The includes gain
  `<cstring>`, `<limits>`, `<memory_resource>` and, on `_WIN32` under the option,
  `<webcpp/crypto/detail/windows.hpp>`. After the includes:

```cpp
#if defined(WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE)

namespace webcpp::crypto {

/**
 * The resource every allocation of OpenSSL's in the process goes to, from any thread:
 * declared here under `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` and defined once by the
 * program. It returns a resource that is never destroyed and is thread-safe whenever
 * OpenSSL may run on two threads at once: OpenSSL frees through it after `main` returns,
 * and on Windows with a shared libcrypto after every static destructor of the program.
 *
 * @return The program's resource.
 * @see "OpenSSL's allocations in your own memory resource", in the guide.
 */
std::pmr::memory_resource& openssl_memory_resource() noexcept;

}  // namespace webcpp::crypto

namespace webcpp::crypto::detail {

/**
 * The alignment of the adapter's blocks, and the size of the header before each that
 * holds its size: at least what the system's `malloc` gives OpenSSL without the option
 * (16 on x86-64 and arm64; on macOS arm64 `alignof(std::max_align_t)` is only 8).
 */
inline constexpr std::size_t openssl_block_alignment =
    (std::max)(alignof(std::max_align_t), std::size_t{__STDCPP_DEFAULT_NEW_ALIGNMENT__});

}  // namespace webcpp::crypto::detail

// The adapter: three C functions given to OpenSSL, with C linkage since OpenSSL calls
// them as C, inline with external linkage so that every file of the program sees one
// function, which the install and make recognise by its address.
extern "C" {

/**
 * OpenSSL's `malloc` under the option: a block of the program's resource, after a header
 * that holds its size.
 *
 * @param size The bytes OpenSSL asks for; 0, or a size whose header overflows, gives null.
 * @return The block, or null, which OpenSSL takes as a failure.
 */
inline void* webcpp_crypto_openssl_malloc(std::size_t size, char const* /*file*/,
                                          int /*line*/) noexcept {
    constexpr std::size_t header = webcpp::crypto::detail::openssl_block_alignment;
    if (size == 0 || size > (std::numeric_limits<std::size_t>::max)() - header) {
        return nullptr;
    }
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    try {
#endif
        void* const block = webcpp::crypto::openssl_memory_resource().allocate(header + size,
                                                                              header);
        if (block == nullptr) {
            return nullptr;
        }
        std::memcpy(block, &size, sizeof size);
        return static_cast<unsigned char*>(block) + header;
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    } catch (...) {  // OpenSSL is C: no exception may cross it
        return nullptr;
    }
#endif
}

/**
 * OpenSSL's `free` under the option: the block back to the program's resource.
 *
 * @param address A block of the adapter's, or null.
 */
inline void webcpp_crypto_openssl_free(void* address, char const* /*file*/,
                                       int /*line*/) noexcept {
    if (address == nullptr) {
        return;
    }
    constexpr std::size_t header = webcpp::crypto::detail::openssl_block_alignment;
    auto* const block = static_cast<unsigned char*>(address) - header;
    std::size_t size = 0;
    std::memcpy(&size, block, sizeof size);
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    try {
#endif
        webcpp::crypto::openssl_memory_resource().deallocate(block, header + size, header);
#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS
    } catch (...) {  // a resource's deallocate throws nothing; none may cross OpenSSL
    }
#endif
}

/**
 * OpenSSL's `realloc` under the option, as C's: null is `malloc`, 0 frees and gives null,
 * and a failure gives null and leaves the old block valid.
 *
 * @param address The block, or null.
 * @param size The new size.
 * @return The new block, or null.
 */
inline void* webcpp_crypto_openssl_realloc(void* address, std::size_t size, char const* file,
                                           int line) noexcept {
    if (address == nullptr) {
        return webcpp_crypto_openssl_malloc(size, file, line);
    }
    if (size == 0) {
        webcpp_crypto_openssl_free(address, file, line);
        return nullptr;
    }
    void* const moved = webcpp_crypto_openssl_malloc(size, file, line);
    if (moved == nullptr) {
        return nullptr;
    }
    constexpr std::size_t header = webcpp::crypto::detail::openssl_block_alignment;
    std::size_t old = 0;
    std::memcpy(&old, static_cast<unsigned char*>(address) - header, sizeof old);
    std::memcpy(moved, address, (std::min)(old, size));
    webcpp_crypto_openssl_free(address, file, line);
    return moved;
}

}  // extern "C"

namespace webcpp::crypto::detail {

/** Whether OpenSSL's functions are the adapter's: three loads, no call into OpenSSL's heap. */
inline bool openssl_adapter_installed() noexcept {
    CRYPTO_malloc_fn m = nullptr;
    CRYPTO_realloc_fn r = nullptr;
    CRYPTO_free_fn f = nullptr;
    CRYPTO_get_mem_functions(&m, &r, &f);
    return m == &webcpp_crypto_openssl_malloc && r == &webcpp_crypto_openssl_realloc &&
           f == &webcpp_crypto_openssl_free;
}

/**
 * Whether `installed` is OpenSSL's own function `name` at `own`: the same address, or, on
 * Windows, the address the module that holds `installed` exports under `name`, since a
 * program linked against libcrypto's DLL takes `&CRYPTO_malloc` as its own import thunk
 * (OpenSSL declares its functions without dllimport, spec section 3.7).
 */
template <class Function>
bool openssl_own(Function installed, Function own, char const* name) noexcept {
    if (installed == own) {
        return true;
    }
#ifdef _WIN32
    HMODULE module = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): an address, as Windows asks
    auto const address = reinterpret_cast<LPCWSTR>(installed);
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           address, &module) == 0) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): GetProcAddress's own type
    return reinterpret_cast<Function>(GetProcAddress(module, name)) == installed;
#else
    static_cast<void>(name);
    return false;
#endif
}

/** Whether OpenSSL's functions are its own defaults (`mem.c:23-25`), on every link. */
inline bool openssl_defaults_installed() noexcept {
    CRYPTO_malloc_fn m = nullptr;
    CRYPTO_realloc_fn r = nullptr;
    CRYPTO_free_fn f = nullptr;
    CRYPTO_get_mem_functions(&m, &r, &f);
    return openssl_own<CRYPTO_malloc_fn>(m, &CRYPTO_malloc, "CRYPTO_malloc") &&
           openssl_own<CRYPTO_realloc_fn>(r, &CRYPTO_realloc, "CRYPTO_realloc") &&
           openssl_own<CRYPTO_free_fn>(f, &CRYPTO_free, "CRYPTO_free");
}

}  // namespace webcpp::crypto::detail

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_openssl_memory", "resource")
#endif

#elif defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_openssl_memory", "openssl")
#endif  // WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE
```

  The tag type gains the install, and `openssl_memory_guard` its check:

```cpp
struct openssl {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "openssl";

#if defined(WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE)
    /**
     * Installs webcpp's adapter as OpenSSL's memory functions, so that every allocation of
     * OpenSSL's in the process goes to `openssl_memory_resource()`. Called as the first
     * statement of `main`, before any thread starts and anything uses OpenSSL.
     *
     * @return Nothing, also when the adapter is installed already;
     * `memory_functions_refused` when another party installed its own functions, or OpenSSL
     * has allocated with its own and refuses the change.
     */
    static boost::system::result<void> install_memory_functions() noexcept {
        if (detail::openssl_adapter_installed()) {
            return {};
        }
        if (!detail::openssl_defaults_installed()) {
            return error::memory_functions_refused;
        }
        // The resource is constructed here, outside OpenSSL's allocation path.
        static_cast<void>(openssl_memory_resource());
        if (CRYPTO_set_mem_functions(&webcpp_crypto_openssl_malloc, &webcpp_crypto_openssl_realloc,
                                     &webcpp_crypto_openssl_free) != 1) {
            return error::memory_functions_refused;
        }
        return {};
    }
#endif
};
```

```cpp
inline boost::system::result<void> openssl_memory_guard() noexcept {
#if defined(WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE)
    if (!openssl_adapter_installed()) {
        return error::memory_functions_not_installed;
    }
#endif
    return {};
}
```

  The adapter and the install are written before the tag type in the file, since the install
  names them; the declaration of `openssl_memory_resource` precedes both.
- [ ] **Step 3: Run** the native lane and gcc-15's. Expected: PASS, `adapter` and its three
  twins. Then `$B2 -a --build-dir=bin/lane-native libs/crypto/test` whole: every earlier
  OpenSSL test still passes, without the option.
- [ ] **Step 4: Commit:** `"crypto: OpenSSL's memory functions as an option of the config:
  the adapter, the install, and make's guard"`.

## Task 16: The memory option, process by process

Deferred after v0.1 (revision 2026-11-14). This task is kept as written for the memory
option's own change; before it runs, re-read it against the source-built provider
(CODING-GUIDELINES.md chapter 5 rule 7): OpenSSL is static, built by
`/webcpp/third_party//openssl` and named through `/webcpp/crypto//openssl` (Task 11), version
3.5.9, on native, wasip2 and wasip3; the feature `webcpp-crypto-openssl-link`, `OPENSSL_ROOT`
and the shared link it names are gone; the error values 13 and 14 are reserved and undeclared;
and the measured counts are 3.6.4's.

**Files:**
- Create, in `test/openssl_memory/`: `support.hpp` (the counting resource that is never
  destroyed, the exit check, the counter of the C library's allocator), `interpose_darwin.c`,
  `interpose_linux.cpp`, `everything_test.cpp`, `off_reported_test.cpp`, `late_test.cpp`,
  `foreign_test.cpp`, `fresh_install_test.cpp`, `forgotten_test.cpp`, `replaced_test.cpp`,
  `second_install_test.cpp`, `fail_second_test.cpp`, `fail_init_test.cpp`.
- Create (superproject), fixtures of Task 10's test: `tools/test/fixtures/crypto_jamfiles/memory_missing_definition_install/`
  and `memory_missing_definition_make/` (a Jamfile, an `app.cpp` and an `.outcome` each), and
  `memory_two_files/` (`Jamfile`, `main.cpp`, `with.cpp`, `.outcome`).
- Modify: `test/openssl_memory/Jamfile`.

**Interfaces:**
- Consumes: Task 15.
- Produces: `webcpp::test::counting_resource` (in `support.hpp`):
  `std::size_t allocations() const`, `std::size_t live() const`, `void arm(std::size_t after)`
  (the allocation after `after` more fails, as Task 15's resource fails), `bool foreign_free() const`;
  `webcpp::test::the_counting_resource()`; `void webcpp::test::main_returns() noexcept`;
  `extern "C" std::size_t webcpp_test_c_allocations(void)` and
  `void webcpp_test_count_c_allocations(int on)` (the interposers').

- [ ] **Step 1: Write `support.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// What every program of the memory option shares: a counting resource that is never
// destroyed (spec section 3.7), on a fixed buffer through a synchronized pool whose
// upstream is null_memory_resource(), so that the resource itself never calls the C
// library; its table of the blocks it gave, which finds a free of another's block; and the
// exit check: main returns 3, and when OpenSSL's cleanup has given every block back after
// main, the resource writes its line and ends the process with 0. Its destructor, which
// must never run, writes a line and ends it with 4 (C7).

#ifndef WEBCPP_TEST_CRYPTO_OPENSSL_MEMORY_SUPPORT_HPP
#define WEBCPP_TEST_CRYPTO_OPENSSL_MEMORY_SUPPORT_HPP

#include <webcpp/crypto/backends/openssl.hpp>

#include <boost/throw_exception.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <mutex>
#include <new>

#ifdef _WIN32
#include <webcpp/crypto/detail/windows.hpp>

#include <io.h>
#define WEBCPP_TEST_WRITE(text) _write(1, text, static_cast<unsigned>(sizeof(text) - 1))
#else
#include <unistd.h>
#define WEBCPP_TEST_WRITE(text) static_cast<void>(::write(1, text, sizeof(text) - 1))
#endif

extern "C" std::size_t webcpp_test_c_allocations(void);
extern "C" void webcpp_test_count_c_allocations(int on);

namespace webcpp::test {

class counting_resource final : public std::pmr::memory_resource {
public:
    counting_resource() : pool_(&monotonic_) {}

    ~counting_resource() override {
        WEBCPP_TEST_WRITE("the resource was destroyed\n");
        end(4);
    }

    /**
     * Ends the process with status, now. On Windows OpenSSL's cleanup may run inside
     * DLL_PROCESS_DETACH, under the loader lock, where `_Exit` would wait for that lock;
     * TerminateProcess does not (m18).
     */
    [[noreturn]] static void end(int status) noexcept {
#ifdef _WIN32
        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(status));
#endif
        std::_Exit(status);
    }

    std::size_t allocations() const { return allocations_.load(); }
    std::size_t live() const { return live_.load(); }
    bool foreign_free() const { return foreign_.load(); }
    void arm(std::size_t after) { fail_after_.store(after + 1); }
    void main_returned() { main_returned_.store(true); }

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (fail_after_.load() != 0 && fail_after_.fetch_sub(1) == 1) {
#ifdef WEBCPP_CRYPTO_NO_EXCEPTIONS
            return nullptr;
#else
            boost::throw_exception(std::bad_alloc());
#endif
        }
        std::lock_guard<std::mutex> const lock(mutex_);
        void* const block = pool_.allocate(size, alignment);
        for (auto& slot : given_) {
            if (slot == nullptr) {
                slot = block;
                break;
            }
        }
        ++allocations_;
        ++live_;
        return block;
    }

    void do_deallocate(void* block, std::size_t size, std::size_t alignment) override {
        {
            std::lock_guard<std::mutex> const lock(mutex_);
            bool known = false;
            for (auto& slot : given_) {
                if (slot == block) {
                    slot = nullptr;
                    known = true;
                    break;
                }
            }
            if (!known) {
                foreign_.store(true);
                return;
            }
            pool_.deallocate(block, size, alignment);
        }
        if (--live_ == 0 && main_returned_.load()) {
            if (foreign_.load()) {
                WEBCPP_TEST_WRITE("a foreign block was freed through the adapter\n");
                end(5);
            }
            WEBCPP_TEST_WRITE("OpenSSL gave every block back after main\n");
            end(0);
        }
    }

    bool do_is_equal(memory_resource const& other) const noexcept override {
        return this == &other;
    }

    // 64 MiB: OpenSSL's first make takes 6,061 blocks, 4,621 still live after it (PROBE-R3).
    alignas(std::max_align_t) std::array<std::byte, 64 << 20> buffer_{};
    std::pmr::monotonic_buffer_resource monotonic_{buffer_.data(), buffer_.size(),
                                                   std::pmr::null_memory_resource()};
    std::pmr::synchronized_pool_resource pool_;
    std::mutex mutex_;
    std::array<void*, 16384> given_{};
    std::atomic<std::size_t> allocations_{0};
    std::atomic<std::size_t> live_{0};
    std::atomic<std::size_t> fail_after_{0};
    std::atomic<bool> foreign_{false};
    std::atomic<bool> main_returned_{false};
};

/** The one resource, in static storage, constructed by placement and never destroyed. */
inline counting_resource& the_counting_resource() noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): never destroyed
    alignas(counting_resource) static unsigned char storage[sizeof(counting_resource)];
    static auto* const resource = new (storage) counting_resource;
    return *resource;
}

}  // namespace webcpp::test

std::pmr::memory_resource& webcpp::crypto::openssl_memory_resource() noexcept {
    return webcpp::test::the_counting_resource();
}

#endif
```

  Each program includes it once (it defines `openssl_memory_resource()`). The 64 MiB array
  lives inside the never-destroyed object, in static storage, not on the stack.
- [ ] **Step 2: Write the interposers** (C5). `interpose_darwin.c`, PROBE-R3's
  `interpose.c` reduced to the counter, built as a shared library linked at launch (M5):

```c
/* Copyright (c) 2026 WebCpp.org
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * https://www.boost.org/LICENSE_1_0.txt)
 *
 * Counts the C library's malloc, calloc, realloc, posix_memalign and aligned_alloc on macOS,
 * from any caller, while the test says so: dyld applies the __interpose section of a library
 * loaded at launch (measured, M5). */
#include <malloc/malloc.h>
#include <stdatomic.h>
#include <stdlib.h>

static atomic_size_t counted;
static atomic_int counting;

size_t webcpp_test_c_allocations(void) { return atomic_load(&counted); }
void webcpp_test_count_c_allocations(int on) { atomic_store(&counting, on); }

static void count(void) {
    if (atomic_load(&counting)) {
        atomic_fetch_add(&counted, 1);
    }
}

static void* counted_malloc(size_t n) { count(); return malloc(n); }
static void* counted_calloc(size_t c, size_t n) { count(); return calloc(c, n); }
static void* counted_realloc(void* p, size_t n) { count(); return realloc(p, n); }
static int counted_posix_memalign(void** p, size_t a, size_t n) { count(); return posix_memalign(p, a, n); }
static void* counted_aligned_alloc(size_t a, size_t n) { count(); return aligned_alloc(a, n); }

#define INTERPOSE(replacement, original) \
    __attribute__((used)) static struct { const void* r; const void* o; } \
    interpose_##original __attribute__((section("__DATA,__interpose"))) = \
        {(const void*)(unsigned long)&replacement, (const void*)(unsigned long)&original}

INTERPOSE(counted_malloc, malloc);
INTERPOSE(counted_calloc, calloc);
INTERPOSE(counted_realloc, realloc);
INTERPOSE(counted_posix_memalign, posix_memalign);
INTERPOSE(counted_aligned_alloc, aligned_alloc);
```

  `interpose_linux.cpp`, definitions in the program that replace glibc's for the whole
  process and forward to glibc's own entry points, never through `dlsym`, which allocates:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Counts the C library's allocations on Linux, from any caller, while the test says so:
// the program's own malloc and its kin replace glibc's in the whole process, the shared
// libcrypto's calls included, and forward to glibc's __libc_* functions.

#include <atomic>
#include <cerrno>
#include <cstddef>

extern "C" {
void* __libc_malloc(std::size_t);
void* __libc_calloc(std::size_t, std::size_t);
void* __libc_realloc(void*, std::size_t);
void* __libc_memalign(std::size_t, std::size_t);
void __libc_free(void*);
}

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the interposer's count
std::atomic<std::size_t> counted{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the interposer's switch
std::atomic<bool> counting{false};

void count() noexcept {
    if (counting.load(std::memory_order_relaxed)) {
        counted.fetch_add(1, std::memory_order_relaxed);
    }
}
}  // namespace

extern "C" {
std::size_t webcpp_test_c_allocations(void) { return counted.load(); }
void webcpp_test_count_c_allocations(int on) { counting.store(on != 0); }

void* malloc(std::size_t n) { count(); return __libc_malloc(n); }
void* calloc(std::size_t c, std::size_t n) { count(); return __libc_calloc(c, n); }
void* realloc(void* p, std::size_t n) { count(); return __libc_realloc(p, n); }
void free(void* p) { __libc_free(p); }
void* aligned_alloc(std::size_t a, std::size_t n) { count(); return __libc_memalign(a, n); }
int posix_memalign(void** p, std::size_t a, std::size_t n) {
    count();
    *p = __libc_memalign(a, n);
    return *p == nullptr ? ENOMEM : 0;
}
}
```

  On Windows no interposition exists (spec 8.5): `interpose_windows.cpp` defines the two
  functions as a counter that stays 0 and a switch that does nothing, and the programs say so
  in their output; the Jamfile picks the source by `<target-os>`.
- [ ] **Step 3: Write `everything_test.cpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Everything in the resource, refused otherwise (spec section 8.5): installed first, the
// configuration file not read, make, the one-shot and incremental seals, good and failed
// opens and the destructors, for gcm and aes256 on OpenSSL, make no call of the C
// library's allocator from any caller; seal, open and the incremental seal allocate
// nothing anywhere; a second thread does the same (Review Focus 5); no foreign block is
// freed; and after main OpenSSL's cleanup gives every block back (support.hpp's exit).

#include "support.hpp"

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/gcm.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <openssl/crypto.h>

#include <array>
#include <cstdint>
#include <thread>

namespace {

namespace crypto = webcpp::crypto;
using aead = crypto::gcm<crypto::aes256<crypto::backends::openssl>>;

void messages(aead& made) {
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 64> data{};
    auto& resource = webcpp::test::the_counting_resource();
    std::size_t const before = resource.allocations();
    auto const tag = made.seal(iv, {}, data);
    BOOST_TEST(tag.has_value());
    BOOST_TEST(made.open(iv, {}, data, *tag).has_value());
    std::array<std::uint8_t, 16> const bad{};
    BOOST_TEST(made.open(iv, {}, data, bad).error() == crypto::error::authentication_failed);
    BOOST_TEST(made.begin_seal(iv, {}).has_value());
    BOOST_TEST(made.update(data).has_value());
    BOOST_TEST(made.result().has_value());
    BOOST_TEST_EQ(resource.allocations(), before);
}

}  // namespace

int main() {
    if (!BOOST_TEST(crypto::backends::openssl::install_memory_functions().has_value())) {
        return webcpp::test::exit_status(boost::report_errors());
    }
    // The configuration file makes the C library allocate its FILE and buffer, which no
    // hook of OpenSSL's reaches (spec section 3.7).
    BOOST_TEST_EQ(OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG, nullptr), 1);
    webcpp_test_count_c_allocations(1);
    {
        std::array<std::uint8_t, 32> const key{};
        auto made = aead::make(key);
        BOOST_TEST(made.has_value());
        messages(*made);
        auto block = crypto::aes256<crypto::backends::openssl>::make(key);
        BOOST_TEST(block.has_value());
        std::array<std::uint8_t, 32> blocks{};
        auto& resource = webcpp::test::the_counting_resource();
        std::size_t const before = resource.allocations();
        BOOST_TEST(block->encrypt(blocks).has_value());
        BOOST_TEST(block->decrypt(blocks).has_value());
        BOOST_TEST_EQ(resource.allocations(), before);
        // Starting and joining a thread allocates in the C library, which is not OpenSSL's:
        // the counter is off around both, and the thread turns it on for its own work.
        webcpp_test_count_c_allocations(0);
        std::thread other([&key] {
            webcpp_test_count_c_allocations(1);
            auto made_there = aead::make(key);
            BOOST_TEST(made_there.has_value());
            messages(*made_there);
            webcpp_test_count_c_allocations(0);
        });
        other.join();
        webcpp_test_count_c_allocations(1);
    }
    webcpp_test_count_c_allocations(0);
    BOOST_TEST_EQ(webcpp_test_c_allocations(), 0U);
    BOOST_TEST(!webcpp::test::the_counting_resource().foreign_free());
    int const errors = boost::report_errors();
    if (errors != 0) {
        return errors;
    }
    webcpp::test::the_counting_resource().main_returned();
    return 3;  // the resource ends the process with 0 once OpenSSL has freed everything
}
```

- [ ] **Step 4: Write the other programs**, each `main` a few lines over `support.hpp`:
  - `off_reported_test.cpp`, in a sibling directory, `test/openssl_memory_off/`, with a
    Jamfile of its own (`project : requirements <library>/webcpp/crypto//crypto <library>/third_party//openssl ;`,
    `webcpp.targets native ;`, `webcpp.run off_reported : off_reported_test.cpp ;` and its
    twins) named by `build-project openssl_memory_off ;` in `test/Jamfile`: a subdirectory of
    `test/openssl_memory/` would inherit that project's requirement
    `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` (I7). It installs test functions with
    `CRYPTO_set_mem_functions` that count, as CR-PROBES `ossl_open.c` does, makes, seals and
    opens, and prints `make allocated <n> blocks, seal 0, open 0`; it passes when seal and
    open counted 0 and prints the make's count, which the documentation quotes (spec 8.5,
    "Off, reported");
  - `late_test.cpp`: `EVP_CIPHER_free(EVP_CIPHER_fetch(nullptr, "AES-256-GCM", nullptr))`,
    then `install_memory_functions()` must give `memory_functions_refused` with spec 3.7's
    message (`.error().message()` compared), and `detail::openssl_defaults_installed()` is
    still true; `main` returns `boost::report_errors()` (the resource never received a
    block, so the exit check does not apply: `main_returned` is not called);
  - `foreign_test.cpp`: `CRYPTO_set_mem_functions` with three counting functions of the
    test's, `OPENSSL_free(OPENSSL_malloc(1))` through them, then `install_memory_functions()`
    gives `memory_functions_refused`, and `CRYPTO_get_mem_functions` still gives the test's;
  - `fresh_install_test.cpp`: `install_memory_functions()` succeeds in a program that has not
    used OpenSSL; this is the program that proves, on MSVC's shared link, that OpenSSL's
    defaults are recognised through the import (spec 8.5);
  - `forgotten_test.cpp`: without the install, `gcm<aes256<openssl>>::make` gives
    `memory_functions_not_installed` with spec 3.7's message, and then
    `CRYPTO_set_mem_functions(<the adapter>)` returns 1: OpenSSL has allocated nothing, so
    the mark came after the guard (spec 8.5's last item of "A failing resource"); the program
    then makes and seals once, and ends through the exit check;
  - `replaced_test.cpp`: install, make; a second party's functions installed with
    `CRYPTO_set_mem_functions` (it returns 1, as OpenSSL accepts it, PROBE-R3 mode `on`);
    the next `make` gives `memory_functions_not_installed`; the adapter is put back with
    `CRYPTO_set_mem_functions` before `main` returns, and the exit check holds;
  - `second_install_test.cpp`: two installs succeed, and `openssl_adapter_installed()`;
  - `fail_second_test.cpp`: install, a good make; then `arm(0)`: the next make's first
    allocation fails and gives `errc::not_enough_memory`; a make after it succeeds; `arm(1)`:
    the second allocation fails and gives `backend_failure`; before each make the test
    raises `ERR_raise(ERR_LIB_USER, ERR_R_PASSED_INVALID_ARGUMENT)` and after it the queue
    holds that one entry (Task 12's `drain`);
  - `fail_init_test.cpp`: install, `arm(0)`, then make gives `backend_failure`; the resource
    no longer fails, and a second make gives `backend_failure` again (the permanence of
    spec 3.7); the queue holds the test's entry alone after each.
  In `test/openssl_memory/Jamfile`, after `memory-run adapter`:

```jam
# The C library's allocator, counted: an interposer linked at launch on macOS
# (C5), the program's own malloc on Linux, nothing on Windows.
lib interpose_darwin : interpose_darwin.c : <link>shared <warnings>off ;
explicit interpose_darwin ;
.counter = <target-os>darwin:<library>interpose_darwin
    <target-os>linux:<source>interpose_linux.cpp
    <target-os>windows:<source>interpose_windows.cpp ;

memory-run everything : everything_test.cpp : $(.counter) ;
memory-run late : late_test.cpp : $(.counter) ;
memory-run foreign : foreign_test.cpp : $(.counter) ;
memory-run fresh_install : fresh_install_test.cpp : $(.counter) ;
memory-run forgotten : forgotten_test.cpp : $(.counter) ;
memory-run replaced : replaced_test.cpp : $(.counter) ;
memory-run second_install : second_install_test.cpp : $(.counter) ;
memory-run fail_second : fail_second_test.cpp : $(.counter) ;
memory-run fail_init : fail_init_test.cpp : $(.counter) ;
```

  `lib` is not one of the raw rules the lint refuses (AGENTS chapter 6). The static twins on
  Windows depend on Task 11's measurement: if the image has no static libcrypto, the
  `_static` programs are declared `<target-os>linux` and `<target-os>darwin` only, through
  a conditional of the Jamfile's own that returns `<build>no` on Windows, with the reason.
- [ ] **Step 5: Write the link cases** (spec 8.6, crypto-r3-review I2), fixtures of Task 10's
  test under `tools/test/fixtures/crypto_jamfiles/`, each with `root: OPENSSL_ROOT`:
  - `memory_missing_definition_install/`: `project app : requirements <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE <library>/third_party//openssl ;`
    and `exe app : app.cpp /webcpp/crypto//crypto ;`, whose `app.cpp` calls
    `install_memory_functions()` and defines no resource;
  - `memory_missing_definition_make/`: the same, `app.cpp` making a `gcm<aes256<openssl>>`;
  - their outcome, on native: `build: fails` and `webcpp::crypto::openssl_memory_resource()`,
    and per linker, measured by this step's first run and then written into the outcome:
    `[native-darwin]` `Undefined symbols for architecture` (ld64, PROBE-R3 `linkerr.log`),
    `[native-linux]` the GNU ld or lld line that names it (gcc's lanes link with GNU ld,
    clang's with the system's), `[native-windows]` `LNK2019` and `LNK1120`;
  - `memory_two_files/`: `obj with : with.cpp /webcpp/crypto//crypto /third_party//openssl : <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE ;`
    and `exe app : main.cpp with /webcpp/crypto//crypto /third_party//openssl ;`, `with.cpp`
    defining the resource and a function that makes a `gcm` under the option, `main.cpp`
    making one without; outcome `[native-windows]` `build: fails`,
    `LNK2038: mismatch detected for 'webcpp_crypto_openssl_memory'`; `[native-linux]` and
    `[native-darwin]` `build: succeeds`, which records that only MSVC's linker catches it
    (the documentation says so).
  Each is native only (no `targets:` line). These Jamfiles are also the first b2
  build of spec 7.3's project-requirements form (crypto-r3-review's unmeasured fact): if b2
  refuses `<library>` in a project's requirements, or the define does not reach a target,
  the form is replaced in spec 7.3's text and the documentation by an alias the program's
  targets name, `alias openssl-memory : : : : <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE <library>/third_party//openssl ;`,
  measured the same way, and the owner is told.
- [ ] **Step 6: Run** the native lane (Apple clang), gcc-15's, and
  `python3 tools/test/crypto_jamfiles_test.py memory_missing_definition_install memory_missing_definition_make memory_two_files`.
  Expected: PASS, every program of `test/openssl_memory` and `test/openssl_memory_off` and
  their twins, and the link cases. On the CI's MSVC lanes the Windows branch of the identity check, the
  `_onexit` order and the LNK messages are measured for the first time (spec 3.7, 8.5):
  - `fresh_install` fails on the shared link: the identity check is wrong. The test prints
    `f`, `GetProcAddress`'s answer and `&CRYPTO_malloc`; the task stops and the owner
    chooses between comparing with the import table's `__imp_CRYPTO_malloc` and refusing the
    option on MSVC's shared link;
  - `everything` exits 3 on the shared link: `OPENSSL_cleanup` did not give every block back
    before the process ended (the DLL's `_onexit` handler did not run, or not to the end);
    a hang (`TerminateProcess` not ending the process inside `DLL_PROCESS_DETACH`, m18) is
    the other outcome the first MSVC run measures: if it hangs, the Windows twins run through
    `webcpp.drive` with `<webcpp-drive-timeout>60`, whose keeper kills the process and fails
    the test, so that no CI job waits for its own timeout.
    The resource's line is then absent; the test is changed on that lane alone to pass when
    neither the destructor's line appears nor a foreign free, and to print the live count,
    and spec 3.7's statement and the documentation say what was measured;
  - the LNK lines are written into the outcomes as measured.
- [ ] **Step 7: Commit:** `"crypto: the memory option proved process by process: every
  allocation in the resource, the late, foreign and forgotten installs, a failing resource,
  the exit"`.

## Task 17: Allocation on every backend, and BearSSL's seeder

**Files:**
- Create: `test/support/counting.hpp`, `test/counting_test.cpp`,
  `test/allocation_bearssl_test.cpp`, `test/allocation_openssl_test.cpp`,
  `test/bearssl_seeder_test.cpp`.
- Modify: `test/Jamfile`, `doc/changelog.adoc`.

**Interfaces:**
- Produces: `test/support/counting.hpp`: `webcpp::test::allocations()` (every `operator new`
  form and the C library's allocator where a program can count it, I3),
  `webcpp::test::c_allocations_counted`, used by Tasks 27 and 29.

- [ ] **Step 1: Write the failing tests.** One counting support for every allocation test
  of the plan (I3), which sees both `operator new` and the C library's allocator, as trystero's
  `allocation_test.cpp` and asio's tests do: `test/support/counting.hpp`, included by one
  source per program:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Counts every allocation of the program that includes this header once (spec section 8.5):
// every form of the global operator new, and the C library's malloc, calloc, realloc,
// aligned_alloc and posix_memalign, where a program can see them: glibc's replaced and
// forwarded to its __libc_* functions, Emscripten's replaced and forwarded to its
// emscripten_builtin_* (its dlmalloc defines them weak), macOS's through malloc_logger, the
// hook libsystem_malloc calls on every allocation. WASI's wasi-libc defines malloc and its
// __libc_malloc in one object (dlmalloc.c.obj, measured with nm), so a program cannot
// replace one and keep the other, and MSVC's C runtime has no hook: there the C library's
// allocations are not counted, and c_allocations_counted says so, which the tests print.
// A block of operator new is counted twice where the C library is counted (once by new, once
// by the malloc it calls): every test compares a count before and after a window, so this
// changes no verdict. Nothing here reads a thread_local: on macOS the first read of one on a
// thread allocates through dyld (_tlv_get_addr), inside malloc_logger, which recursed until
// the stack ended (N1, measured in the review's rr1/logger.cpp).

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_COUNTING_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_COUNTING_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/heap.h>
#endif

namespace webcpp::test {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the replaced new's count
inline std::atomic<std::size_t> counted_new{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the C library's count
inline std::atomic<std::size_t> counted_c{0};
static_assert(std::atomic<std::size_t>::is_always_lock_free,
              "the counters are called from inside the allocator and must not allocate or lock");

/** Every allocation counted so far: operator new's and the C library's. */
inline std::size_t allocations() {
    return counted_new.load() + counted_c.load();
}

/** Whether this target counts the C library's allocations too. */
inline constexpr bool c_allocations_counted =
#if defined(__GLIBC__) || defined(__EMSCRIPTEN__) || defined(__APPLE__)
    true;
#else
    false;
#endif

inline void count_c() noexcept {
    ++counted_c;
}

inline void* new_block(std::size_t size, std::size_t alignment) {
    ++counted_new;
    void* p = nullptr;
    if (alignment <= alignof(std::max_align_t)) {
        p = std::malloc(size == 0 ? 1 : size);
    } else {
#ifdef _WIN32
        p = _aligned_malloc(size == 0 ? 1 : size, alignment);
#else
        p = std::aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
#endif
    }
    return p;
}

}  // namespace webcpp::test

[[noreturn]] inline void webcpp_test_out_of_memory() {
#ifdef __cpp_exceptions
    throw std::bad_alloc();  // the replaced operator's contract
#else
    std::abort();
#endif
}

void* operator new(std::size_t size) {
    if (void* const p = webcpp::test::new_block(size, alignof(std::max_align_t))) {
        return p;
    }
    webcpp_test_out_of_memory();
}
void* operator new[](std::size_t size) {
    return operator new(size);
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (void* const p = webcpp::test::new_block(size, static_cast<std::size_t>(alignment))) {
        return p;
    }
    webcpp_test_out_of_memory();
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return operator new(size, alignment);
}
void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return webcpp::test::new_block(size, alignof(std::max_align_t));
}
void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
    return webcpp::test::new_block(size, alignof(std::max_align_t));
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
#ifdef _WIN32
void operator delete(void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { _aligned_free(p); }
#else
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
#endif

#if defined(__GLIBC__) || defined(__EMSCRIPTEN__)
#ifdef __GLIBC__
extern "C" {
void* __libc_malloc(std::size_t);
void* __libc_calloc(std::size_t, std::size_t);
void* __libc_realloc(void*, std::size_t);
void* __libc_memalign(std::size_t, std::size_t);
void __libc_free(void*);
}
#define WEBCPP_TEST_C(name) __libc_##name
#else
#define WEBCPP_TEST_C(name) emscripten_builtin_##name
#endif
// The C library's allocation functions, replaced for this program alone; cert-dcl58-cpp takes
// a replacement of what <cstdlib> declares for a change to namespace std.
// NOLINTBEGIN(readability-inconsistent-declaration-parameter-name,cert-dcl58-cpp)
extern "C" {
void* malloc(std::size_t size) {
    webcpp::test::count_c();
    return WEBCPP_TEST_C(malloc)(size);
}
void* calloc(std::size_t count, std::size_t size) {
    webcpp::test::count_c();
    return WEBCPP_TEST_C(calloc)(count, size);
}
void* realloc(void* p, std::size_t size) {
    webcpp::test::count_c();
    return WEBCPP_TEST_C(realloc)(p, size);
}
void* aligned_alloc(std::size_t alignment, std::size_t size) {
    webcpp::test::count_c();
    return WEBCPP_TEST_C(memalign)(alignment, size);
}
int posix_memalign(void** p, std::size_t alignment, std::size_t size) {
    webcpp::test::count_c();
    *p = WEBCPP_TEST_C(memalign)(alignment, size);
    return *p == nullptr ? 12 /* ENOMEM */ : 0;
}
void free(void* p) {
    WEBCPP_TEST_C(free)(p);
}
}
// NOLINTEND(readability-inconsistent-declaration-parameter-name,cert-dcl58-cpp)
#elif defined(__APPLE__)
// libsystem_malloc's hook, called on every allocation and release of the process.
using webcpp_test_malloc_logger = void(std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t,
                                       std::uintptr_t, std::uint32_t);
extern "C" webcpp_test_malloc_logger* malloc_logger;
inline void webcpp_test_count_c_allocation(std::uint32_t type, std::uintptr_t, std::uintptr_t,
                                           std::uintptr_t, std::uintptr_t, std::uint32_t) {
    constexpr std::uint32_t allocate = 2;  // MALLOC_LOG_TYPE_ALLOCATE
    if ((type & allocate) != 0) {
        webcpp::test::count_c();
    }
}
namespace webcpp::test {
/** Installs the hook once, from the first counting call. */
inline bool const logger_installed = (malloc_logger = &webcpp_test_count_c_allocation, true);
}  // namespace webcpp::test
#endif

#endif
```

  The `inline bool const logger_installed` is initialised at the program's start, before
  `main` (an inline variable's dynamic initialisation in the one translation unit that
  includes the header). `test/allocation_bearssl_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// On BearSSL nothing allocates (spec section 3.2): make, seal, open, the incremental seal,
// equal and random_device's fill make no call of operator new, nor of the C library's
// allocator where the target lets a program count it; a control proves the counter moves.

#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/random_device.hpp>  // lint-world: a test
#include <webcpp/test/exit_status.hpp>

#include "support/counting.hpp"
#include "support/gcm_checks.hpp"

#include <cstdio>
#include <cstdlib>

int main() {
    namespace crypto = webcpp::crypto;
    // The control: a malloc of the C library's moves the counter where it is counted.
    std::size_t const control = webcpp::test::allocations();
    void* volatile block = std::malloc(40);
    std::free(block);
    BOOST_TEST_EQ(webcpp::test::allocations() - control,
                  webcpp::test::c_allocations_counted ? 1U : 0U);
    if (!webcpp::test::c_allocations_counted) {
        std::printf("the C library's allocations are not counted on this target\n");
    }
    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> iv{};
    std::array<std::uint8_t, 4096> data{};
    std::size_t const before = webcpp::test::allocations();
    {
        crypto::random_device device;  // lint-world: a test
        BOOST_TEST(device.fill(key).has_value());
        auto made = crypto::gcm<crypto::aes256<crypto::backends::bearssl>>::make(key);
        BOOST_TEST(made.has_value());
        auto const tag = made->seal(iv, {}, data);
        BOOST_TEST(tag.has_value());
        BOOST_TEST(made->open(iv, {}, data, *tag).has_value());
        BOOST_TEST(made->begin_seal(iv, {}).has_value());
        BOOST_TEST(made->update(data).has_value());
        BOOST_TEST(made->result().has_value());
        BOOST_TEST(crypto::equal(*tag, *tag));
    }
    BOOST_TEST_EQ(webcpp::test::allocations(), before);
    return webcpp::test::exit_status(boost::report_errors());
}
```

  (`BOOST_TEST`'s own reporting is outside the counted window: the window holds the calls
  alone, and a failed check's message is written after `before` is compared.) Tasks 27 and 29
  use the same header for libsecp256k1 and ctaes.

  `test/allocation_openssl_test.cpp`: before anything, `CRYPTO_set_mem_functions` with three
  counting functions of the test's (OpenSSL's allocations, outside the user's allocator by
  the accepted departure); then `make` of `gcm<aes256<openssl>>` and of `aes256<openssl>`,
  whose counts it prints (`make: <n> allocations through OpenSSL's functions, first make
  included` and `second make: <m>`) and does not refuse; then seal, open (good and failed),
  the incremental seal, encrypt and decrypt, each with OpenSSL's count and
  `webcpp::test::allocations()` unchanged, the C library's allocator included where counted
  (spec 8.5; measured by this test on OpenSSL 3.5.9 on each target: native with assembly,
  MSVC, wasip2 and wasip3 without it; the spec's 0 was measured on 3.6.4). OpenSSL's test
  functions forward to the C library's own (`__libc_malloc` on glibc, `malloc` elsewhere), so
  that on glibc what OpenSSL allocates is counted once, as OpenSSL's; on macOS
  `malloc_logger` counts it again, which changes no window's difference. `test/bearssl_seeder_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// BearSSL's own seeder is getentropy on every target but Windows, where it has none
// (spec sections 4.1, 6 and 8.4): no backend chooses its entropy.

#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>

int main() {
    char const* name = nullptr;
    br_prng_seeder const seeder = br_prng_seeder_system(&name);
#ifdef _WIN32
    BOOST_TEST(seeder == nullptr);
    BOOST_TEST_EQ(std::string_view(name), "none");
#else
    BOOST_TEST(seeder != nullptr);
    BOOST_TEST_EQ(std::string_view(name), "getentropy");
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/counting_test.cpp`, the counter itself (N1): it counts each allocation exactly as
  documented and allocates nothing of its own, re-entrantly or not, on the first thread and on
  a new one (where a `thread_local` read would have allocated through dyld on macOS):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// support/counting.hpp counts each allocation as it documents, no more: n mallocs are n where
// the C library is counted, an operator new is one, or two where its malloc is counted too,
// and a fresh thread counts the same, so the counter allocates nothing itself (N1).

#include <webcpp/test/exit_status.hpp>

#include "support/counting.hpp"

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdlib>
#if !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#include <thread>
#endif

namespace {

constexpr std::size_t times = 1000;
constexpr std::size_t c_counted = webcpp::test::c_allocations_counted ? 1 : 0;

/** Counts `times` mallocs of each size and `times` operator news, and checks both counts. */
void check_counts() {
    for (std::size_t const size : std::array<std::size_t, 3>{40, 300 * 1024, 1024 * 1024}) {
        std::size_t const before = webcpp::test::allocations();
        for (std::size_t i = 0; i < times; ++i) {
            void* volatile block = std::malloc(size);
            std::free(block);
        }
        BOOST_TEST_EQ(webcpp::test::allocations() - before, times * c_counted);
    }
    std::size_t const before = webcpp::test::allocations();
    for (std::size_t i = 0; i < times; ++i) {
        int* volatile block = new int(0);
        delete block;
    }
    BOOST_TEST_EQ(webcpp::test::allocations() - before, times * (1 + c_counted));
}

}  // namespace

int main() {
    check_counts();
#if !defined(__EMSCRIPTEN__) && !defined(__wasi__)
    // A thread's first allocations: its own state is made by the runtime before the body runs.
    std::thread second(check_counts);
    second.join();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile`:

```jam
crypto-run counting : counting_test.cpp ;
crypto-run allocation_bearssl : allocation_bearssl_test.cpp : <library>/third_party//bearssl ;
crypto-run allocation_openssl : allocation_openssl_test.cpp
  : <library>/webcpp/crypto//openssl : native wasip2 wasip3 ;
crypto-run bearssl_seeder : bearssl_seeder_test.cpp : <library>/third_party//bearssl ;
```

  Run the four lanes, and on macOS (Apple clang, the native lane) `counting` and
  `allocation_bearssl` by name first (`$B2 -a libs/crypto/test//counting libs/crypto/test//allocation_bearssl`):
  macOS is the one target where a counter that allocates inside `malloc_logger` crashes before
  `main` (N1). Expected: FAIL first on the missing header, then PASS on every target, `counting`
  with 1,000 of 1,000 at each size on both threads; the OpenSSL counts of `make` are printed
  in the test's output per target, recorded in the ledger and quoted by Task 24's page, and
  the message's calls are 0 on every target.
- [ ] **Step 2: Commit** (after `git add` of the new files), with `doc/changelog.adoc`'s line
  `* No allocation in a message on BearSSL or OpenSSL; BearSSL seeds from getentropy alone.`:
  `git -C libs/crypto commit -m "crypto: no allocation in a message on BearSSL or OpenSSL, and BearSSL's seeder getentropy alone" -- test doc/changelog.adoc`;
  then `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.

## Task 18: Every message at link time and on MSVC, and the canary's pins

**Files:**
- Create, fixtures of Task 10's test under `tools/test/fixtures/crypto_jamfiles/`:
  `canary_missing/`, `canary_other_commit/`,
  `canary_other_configuration/`, `canary_without/`, each a `Jamfile`, an `app.cpp` and an
  `.outcome`.
- Create (library): `test/msvc/Jamfile` and one directory per MSVC case under it,
  `test/msvc/<case>/Jamfile` and its sources: `no_backend`, `bearssl_not_linked`,
  `openssl_not_linked`, `two_defaults`, `aes_on_bearssl`, `canary_missing`,
  `canary_other_commit`, `canary_other_configuration`, `canary_two_files`,
  `default_two_files`.
- Modify (library): `test/Jamfile` (`build-project msvc ;`), `doc/changelog.adoc` nothing (no
  user sees a test).
- Modify (superproject): `tools/lint/lint.sh` (the rule `canary pins`), `tools/lint/lint_test.py`,
  `CODING-GUIDELINES.md` (chapter 9's table: the rule), `doc/releases/0.1.0.adoc` ("Tools and
  build": the rule).

**Interfaces:**
- Consumes: `tools/test/crypto_jamfiles_test.py` and its outcome files (Task 10), the canary
  seams (Task 5, Task 6), `/webcpp/crypto//openssl` (Task 12). The fixtures are copied to
  `<scratch>/user/<case>/`, so a path into the superproject is `../../<path>`.

Where each message's text is checked: the fixtures check the exact text on clang, GNU ld and
wasm-ld, which the CI's tools job (Linux) runs, and ld64's in their `[native-darwin]` blocks,
which run on a developer's macOS and are recorded in the ledger. The tools job has no macOS
or Windows leg. MSVC's cases are tests of the library, each in a directory of its own, built
on the CI's `msvc-14.3` and `msvc-14.5` lanes on every push as `webcpp.compile-fail` or
`webcpp.link-fail`: they prove that each case fails on MSVC, not its text (`C1189`, `C2338`,
`LNK2001`, `LNK2038`), a limit CODING-GUIDELINES.md chapter 19 records (Task 25). OpenSSL has
no canary: the source-built provider is never patched, and the canary requirement was not
adopted for it (CODING-GUIDELINES.md chapter 19, "Asio/Beast closure"); its layers are the
compile-time `#error`s and MSVC's `detect_mismatch` on the default backend.

- [ ] **Step 1: Write the canary cases** (spec 4.3 layer 2, 8.6). Every case's `app.cpp` makes
  a `gcm<aes256<backends::bearssl>>` and seals once. The Jamfiles:
  - `canary_missing`: `exe app : app.cpp /webcpp/crypto//crypto : <define>WEBCPP_CRYPTO_HAS_BEARSSL=1 <include>../../third_party/libs/bearssl/inc ;`
    (the header found, the library not linked);
  - `canary_other_commit`: `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl : <define>WEBCPP_CRYPTO_BEARSSL_CANARY_COMMIT=0000000 ;`;
  - `canary_other_configuration`: the same with
    `<define>WEBCPP_CRYPTO_BEARSSL_CANARY_CONFIGURATION=sysrng`;
  - `canary_without`: a library of BearSSL's sources without the canary,

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# BearSSL built by other means, without webcpp's canary: the program links every
# BearSSL function it calls, and only the canary is missing.
path-constant BEARSSL : ../../third_party/libs/bearssl ;
lib bearssl-plain : [ glob $(BEARSSL)/src/*.c $(BEARSSL)/src/*/*.c ]
  : <link>static <warnings>off <include>$(BEARSSL)/inc <include>$(BEARSSL)/src
  : : <include>$(BEARSSL)/inc <define>WEBCPP_CRYPTO_HAS_BEARSSL=1 ;
exe app : app.cpp bearssl-plain /webcpp/crypto//crypto ;
```

  The outcomes, `targets: native wasip2 wasip3 emscripten` and `build: fails` in `[all]` and
  the canary's name, which every linker prints, then each linker's own line (spec 4.3's
  table):

```
[all]
targets: native wasip2 wasip3 emscripten
build: fails
[native-darwin]
Undefined symbols for architecture
_webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link
[native-linux]
webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link
[wasip2]
wasm-ld: error:
undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link
[wasip3]
wasm-ld: error:
undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link
[emscripten]
wasm-ld: error:
undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link
em++: error:
```

  for `canary_missing` and `canary_without`; `canary_other_commit`'s name has `0000000` in
  place of `7bea48e`, `canary_other_configuration`'s `sysrng` in place of `getentropy`. The
  GNU ld line on Linux (`undefined reference to `...'`) is measured by the first run on the
  gcc and clang lanes and written into `[native-linux]` as measured. The `[native-darwin]`
  block is measured and run on a developer's macOS, and the ledger records that run.
- [ ] **Step 2: Write the MSVC cases** (spec 4.3 layers 1 to 3, 8.6; Review Focus 4), each a
  directory of its own under `test/msvc/` (b2 shares one object file between two programs of
  one source that differ only by a free define or a `/third_party` library, spec 4.3).
  `test/msvc/Jamfile` builds them on MSVC alone, through its project's requirements, which
  every case's Jamfile inherits:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# Each message of the build on MSVC (spec section 8.6), a directory per case:
# on the CI's msvc lanes each case must fail to compile or to link. Its text
# is checked on clang, GNU ld and wasm-ld by tools/test/crypto_jamfiles_test.py;
# MSVC's is not checked (CODING-GUIDELINES.md chapter 19). Elsewhere than MSVC
# nothing here is built.
rule msvc-only ( properties * )
{
    if ! ( <toolset>msvc in $(properties) )
    {
        return <build>no ;
    }
}

project : requirements <conditional>@msvc-only ;

build-project no_backend ;
build-project bearssl_not_linked ;
build-project openssl_not_linked ;
build-project two_defaults ;
build-project aes_on_bearssl ;
build-project canary_missing ;
build-project canary_other_commit ;
build-project canary_other_configuration ;
build-project canary_two_files ;
build-project default_two_files ;
```

  Each case's Jamfile is `import webcpp ;` and one line, native:
  - the compile-time cases, `webcpp.compile-fail <case> : app.cpp : <its requirements> : native ;`,
    whose `app.cpp` is the compile-diagnostic case of the same name of Tasks 6, 7 and 12
    without its `expected-error` comment: `no_backend` (no requirement), `bearssl_not_linked`
    (`<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL`), `openssl_not_linked`
    (`<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL`), `two_defaults` (both defines) and
    `aes_on_bearssl` (`<library>/third_party//bearssl`), each with
    `<library>/webcpp/crypto//crypto`; MSVC stops at `fatal error C1189` for an `#error` and
    `error C2338` for a `static_assert`, which the first CI run shows and the ledger records;
  - the link-time cases, `webcpp.link-fail <case> : app.cpp : <its requirements> : native ;`:
    `canary_missing`, `canary_other_commit` and `canary_other_configuration` with the
    requirements of Step 1's fixtures of the same names (`<include>` written
    `../../../../../third_party/libs/bearssl/inc` from the case's directory), which fail with
    LNK2001 (through a static library a mismatching canary member is not loaded, so another
    commit gives LNK2001, not LNK2038, spec 4.3); `canary_without` has no MSVC case, since its
    message is `canary_missing`'s;
  - the LNK2038 cases (layer 3), which succeed elsewhere and so are MSVC's alone:
    `canary_two_files`, `webcpp.link-fail canary_two_files : app.cpp other.cpp : <library>/webcpp/crypto//crypto <library>/third_party//bearssl : native ;`
    with `other.cpp` defining `WEBCPP_CRYPTO_BEARSSL_CANARY_CONFIGURATION getentropy` before
    its includes (MSVC's own configuration differs, so the two files' `detect_mismatch`
    values differ: `LNK2038: mismatch detected for 'webcpp_crypto_backend_bearssl'`), and
    `default_two_files`, `webcpp.link-fail default_two_files : main.cpp with.cpp : <library>/webcpp/crypto//crypto <library>/third_party//bearssl <library>/webcpp/crypto//openssl : native ;`
    with `with.cpp` defining `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` before its includes and
    each source naming `aes256_gcm` (`LNK2038: mismatch detected for 'webcpp_crypto_default_backend'`).
  In `test/Jamfile`, `build-project msvc ;`.
- [ ] **Step 3: Run** `python3 tools/test/crypto_jamfiles_test.py canary_missing canary_other_commit canary_other_configuration canary_without`
  (the canary cases on the four targets). Expected: `crypto_jamfiles_test: ok`; the GNU ld
  lines of `[native-linux]` are written from the first Linux run (the CI's tools job, or a
  Linux machine), as measured. `b2 -a libs/crypto/test/msvc` natively here builds nothing
  (each case `<build>no`) and succeeds; the MSVC cases are proven by the library's CI, on the
  `msvc-14.3` and `msvc-14.5` lanes, whose report shows each case passed (it failed as
  expected); the ledger records the run and each code MSVC printed.
- [ ] **Step 4: Write the lint rule `canary pins`** (spec 8.6's last paragraph,
  CODING-GUIDELINES.md chapter 5, rule 11). Its failing case in `tools/lint/lint_test.py`: a
  scratch tree whose `libs/crypto/include/webcpp/crypto/detail/canary.h` pins
  `#define WEBCPP_CRYPTO_BEARSSL_COMMIT 1234567` while the superproject records
  `third_party/libs/bearssl` at another commit must fail with
  `canary pins: libs/crypto/include/webcpp/crypto/detail/canary.h: bearssl is pinned at 1234567 and third_party/libs/bearssl is at <short commit>`.
  Then the rule:

```bash
# 19. Each canary's commit is its submodule's (CODING-GUIDELINES.md chapter 5, rule 11):
#     every `#define WEBCPP_CRYPTO_<NAME>_COMMIT <hex>` of crypto's detail/canary.h names
#     the first seven digits of the commit the superproject records for
#     third_party/libs/<name>, read from git, so that no checkout is needed.
rule 'canary pins'
pins="$(grep -hoE '^#define WEBCPP_CRYPTO_[A-Z0-9]+_COMMIT [0-9a-f]{7}$' \
    libs/crypto/include/webcpp/crypto/detail/canary.h 2>/dev/null || true)"
pin_faults=''
while read -r _ macro hex; do
    [ -n "${macro}" ] || continue
    name="$(printf '%s' "${macro}" | sed -E 's/^WEBCPP_CRYPTO_(.*)_COMMIT$/\1/' | tr 'A-Z' 'a-z')"
    head="$(git rev-parse --short=7 "HEAD:third_party/libs/${name}" 2>/dev/null || printf 'missing')"
    if [ "${head}" != "${hex}" ]; then
        pin_faults="${pin_faults}libs/crypto/include/webcpp/crypto/detail/canary.h: ${name} is pinned at ${hex} and third_party/libs/${name} is at ${head}
"
    fi
done <<EOF_PINS
${pins}
EOF_PINS
if [ -n "${pin_faults}" ]; then
    printf '%s' "${pin_faults}"
    fail "a canary's commit is not its submodule's"
else
    printf "every canary's commit is its submodule's\n"
fi
```

  The rule's number follows the last of `lint.sh` (18, `changelog form`, on the base). The
  canary pins are written `#define WEBCPP_CRYPTO_BEARSSL_COMMIT 7bea48e` on one line each,
  inside their `#ifdef`'s `#else` (Task 5's form), which the pattern matches. The library's CI
  lints against the superproject's `main`, so a pin moves in the superproject's pull request
  together with the library's commit that moves the canary (CODING-GUIDELINES.md chapter 5,
  rule 12). CODING-GUIDELINES.md chapter 9's table gains the row
  `| canary pins | a `WEBCPP_CRYPTO_<NAME>_COMMIT` of crypto's `detail/canary.h` that is not the first seven digits of the commit the superproject records for `third_party/libs/<name>` |`,
  and `doc/releases/0.1.0.adoc`, "Tools and build", the line
  `* Added the lint rule canary pins, which checks crypto's canaries against their submodules.`
  Run `python3 tools/lint/lint_test.py` and the lint. Expected: PASS and `lint: clean`.
- [ ] **Step 5: Commit** (superproject, after `git add` of the new fixtures)
  `"tools: every link-time message of webcpp::crypto from a fixture of its own, as a user
  meets it"` and `"tools: the lint checks each canary's commit against its submodule"`;
  (library) `"crypto: each message of the build fails on MSVC, a directory per case"`.

## Task 19: `<windows.h>` before and after every header, on MSVC

**Files:**
- Create: `test/windows_h_before_test.cpp`, `test/windows_h_after_test.cpp`.
- Modify: `test/Jamfile`.

- [ ] **Step 1: Write the tests** (spec 2.2, 8.6). `windows_h_before_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A bare <windows.h> (no NOMINMAX, no WIN32_LEAN_AND_MEAN) before every public header,
// the backends' included (random_device's BCryptGenRandom reaches <windows.h> through
// detail/windows.hpp), and max used as a function after them. Elsewhere than Windows the
// program is the headers and the uses alone.

#ifdef _WIN32
#include <windows.h>
#endif

#include <webcpp/crypto.hpp>
#include <webcpp/crypto/backends/openssl.hpp>

#include <algorithm>
#include <limits>

int main() {
    int const larger = (std::max)(1, 2);
    auto const largest = (webcpp::crypto::random_device::max)();  // lint-world: a test
    return larger == 2 && largest == (std::numeric_limits<unsigned int>::max)() ? 0 : 1;
}
```

  The test calls `max` in parentheses because a bare `<windows.h>` included first defines
  the macros for the whole program, which is what its author asked for; the headers' own
  `(max)()` is what is under test. `windows_h_after_test.cpp` includes the webcpp headers
  first and a bare `<windows.h>` after them, which its include guard makes add nothing
  (spec 2.2), and calls `std::max(1, 2)` and `webcpp::crypto::random_device::max()` without
  parentheses: they compile because webcpp's include defined `NOMINMAX` for its own
  `<windows.h>` and undefined it after.
  In `test/Jamfile`:

```jam
# <windows.h> before and after every header (spec section 2.2), on MSVC with
# both backends linked; elsewhere the headers alone.
crypto-run windows_h_before : windows_h_before_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl : native ;
crypto-run windows_h_after : windows_h_after_test.cpp
  : <library>/third_party//bearssl <library>/webcpp/crypto//openssl : native ;
```

- [ ] **Step 2: Run** natively. Expected: PASS here. The test itself is the CI's `msvc-14.3`
  and `msvc-14.5` lanes, which build every native test on every push and build the
  source-built OpenSSL with MSVC already (asio's `tls_client_context` links it there): the
  first run of the library's CI is the evidence (`winsock.h` not dragged in, `max` not
  broken), recorded in the ledger. The provider must agree with the program's threading and
  runtime: MSVC's shared runtime makes b2 build `threading=multi`, as asio's
  `test/tls/Jamfile` notes.
- [ ] **Step 3: Commit:** `"crypto: every header compiles beside a bare <windows.h>, before
  and after"`.

## Task 20: Constant time

Deferred after v0.1 (revision 2026-11-14). Kept as written for its own change; before it runs,
re-read it against the tree of that day: OpenSSL is source-built (`/webcpp/crypto//openssl`,
static, compiled `no-asm` on MSVC and WASI), there is no `webcpp-own`, and the CI has no levels
yet, so the lane `constant-time` runs on every push unless levels exist by then;
`detail/declassify.hpp` is already in the library as the empty seam (Task 7), and this task
gives it its body. Task 26's `ctime_tests.c` and Task 29's run of this lane come back with it.

**Files:**
- Create: `test/constant_time/Jamfile`, `ctgrind_test.cpp`, `msan_test.cpp`, `dudect.hpp`,
  `dudect_test.cpp`, `valgrind.mjs`, `msan_report.mjs`, `openssl.supp`, `package.json`,
  `package-lock.json`.
- Modify: `test/Jamfile`
  (`build-project constant_time ;`); (superproject) `tools/ci/matrix.py` (an own lane's
  `apt` packages), `tools/ci/matrix_test.py`, `.github/workflows/library.yml` (the own lanes'
  job installs them).

**Interfaces:**
- Produces: the own lane `constant-time` (native: Valgrind and MemorySanitizer on Linux;
  dudect natively and on wasip2), which `runners.yml` also runs on `ubuntu-24.04-arm`
  (Task 22).

- [ ] **Step 0: The declassification seam** (I8). A constant-time checker reports a branch on
  any value derived from a marked secret, and an `open` branches on whether the tag matched,
  which is public by design but derived from the key and the tag. BoringSSL's
  `constant_time_declassify` is the precedent: the library marks the verdict public where it
  first branches on it. `include/webcpp/crypto/detail/declassify.hpp`, written in Task 7
  (Step 5) with the BearSSL engine that uses it, is that mark. The documentation's page on
  constant time (`doc/aead.adoc`) says that the tag's verdict is the one value webcpp marks
  public, and that OpenSSL's verdict is OpenSSL's own internal branch, which webcpp cannot
  mark: OpenSSL's `open` is therefore checked with the key secret only during `seal`.
- [ ] **Step 1: Write `ctgrind_test.cpp`** (spec 8.3, deterministic, in the style of ctgrind),
  built with `WEBCPP_CRYPTO_DETAIL_CT_VALGRIND`, which also proves it can fail: with
  `WEBCPP_TEST_CRYPTO_PLANTED_LEAK` it branches on a secret byte itself, and the lane expects
  that build to fail:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Under Valgrind's memcheck, the key, the plaintext and the tag are marked undefined: a
// branch or a memory index that depends on them is an error, and the run fails
// (--error-exitcode). It covers equal, and make, seal and open of BearSSL and OpenSSL.

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/backends/openssl.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/gcm.hpp>

#include <valgrind/memcheck.h>

#include <array>
#include <cstdint>
#include <cstdio>

namespace {

/** make, seal and open of BearSSL with the key, the data and the tag secret. */
int bearssl_seal_and_open() {
    namespace crypto = webcpp::crypto;
    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 64> data{};
    key.fill(0x42);
    data.fill(0x17);
    VALGRIND_MAKE_MEM_UNDEFINED(key.data(), key.size());
    VALGRIND_MAKE_MEM_UNDEFINED(data.data(), data.size());
#ifdef WEBCPP_TEST_CRYPTO_PLANTED_LEAK
    // The planted leak: a branch on a secret byte, which memcheck must report.
    if (key[0] == 0x42) {
        std::printf("planted\n");
    }
#endif
    auto made = crypto::gcm<crypto::aes256<crypto::backends::bearssl>>::make(key);
    auto tag = made->seal(iv, {}, data);
    VALGRIND_MAKE_MEM_UNDEFINED(tag->data(), tag->size());
    // The verdict is declassified inside the engine (WEBCPP_CRYPTO_DETAIL_DECLASSIFY).
    auto const opened = made->open(iv, {}, data, *tag);
    return opened ? 0 : 1;
}

/**
 * make and seal of OpenSSL with the key and the data secret. Its open is not checked with
 * secrets: OpenSSL branches on the tag's verdict inside EVP_DecryptFinal_ex, where webcpp
 * cannot declassify it (I8); the documentation says so.
 */
int openssl_seal() {
    namespace crypto = webcpp::crypto;
    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 64> data{};
    key.fill(0x42);
    data.fill(0x17);
    VALGRIND_MAKE_MEM_UNDEFINED(key.data(), key.size());
    VALGRIND_MAKE_MEM_UNDEFINED(data.data(), data.size());
    auto made = crypto::gcm<crypto::aes256<crypto::backends::openssl>>::make(key);
    auto const tag = made->seal(iv, {}, data);
    return tag ? 0 : 1;
}

}  // namespace

int main() {
    std::array<std::uint8_t, 16> a{};
    std::array<std::uint8_t, 16> b{};
    VALGRIND_MAKE_MEM_UNDEFINED(a.data(), a.size());
    VALGRIND_MAKE_MEM_UNDEFINED(b.data(), b.size());
    bool equal = webcpp::crypto::equal(a, b);
    VALGRIND_MAKE_MEM_DEFINED(&equal, sizeof equal);  // the test's own branch below
    int const failures = (equal ? 0 : 1) + bearssl_seal_and_open() + openssl_seal();
    std::printf("constant time under memcheck: %s\n", failures == 0 ? "ok" : "FAILED");
    return failures;
}
```

  BearSSL's tag check (`EQ0`) is branch-free; its verdict, a value derived from the key and
  the tag, is declassified by the engine before `gcm` branches on it (Step 0), so memcheck
  reports a branch on a secret anywhere else. Valgrind may report inside libcrypto's own
  start-up code, which no secret reaches; a suppression file, `openssl.supp`, holds each such
  frame the first Linux run shows, each with its reason, never a frame of `seal`.
  `valgrind.mjs`:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Runs the program under Valgrind's memcheck and passes with its exit status: 99 when
// memcheck found a use of an undefined value, which a branch on a secret is.
import {spawnSync} from 'node:child_process'
// With --expect-leak, it passes only when memcheck found one: the planted leak's build.
import {dirname, join} from 'node:path'
import {fileURLToPath} from 'node:url'

const program = process.argv[process.argv.length - 1]
const expectLeak = process.argv.includes('--expect-leak')
const suppressions = join(dirname(fileURLToPath(import.meta.url)), 'openssl.supp')
const run = spawnSync('valgrind', ['--error-exitcode=99', '--track-origins=yes', '--quiet',
    `--suppressions=${suppressions}`, program], {stdio: 'inherit'})
if (run.error) {
    console.error(`valgrind could not run: ${run.error.message}; install valgrind (Linux)`)
    process.exit(1)
}
if (expectLeak) {
    process.exit(run.status === 99 ? 0 : 1)
}
process.exit(run.status ?? 1)
```

  `msan_report.mjs`, the driver of `msan_planted` (n2): a `run-fail` would pass on any failure,
  a crash or a missing sanitizer runtime included, so the driver passes only on
  MemorySanitizer's report of the planted branch:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Runs the planted build under MemorySanitizer and passes only when MemorySanitizer reported a
// use of an uninitialised value in the planted branch, with its exit code 77. Any other end of
// the program, a crash or a success, fails. The report is symbolised by the toolchain's
// llvm-symbolizer (llvm-symbolizer-18 on the lane's runner, installed with the lane's
// packages), named in MSAN_SYMBOLIZER_PATH; without symbols the frames are bare addresses,
// so the driver refuses to judge and says what is missing.
import {spawnSync} from 'node:child_process'

const program = process.argv[process.argv.length - 1]
const symbolizer = ['llvm-symbolizer-18', 'llvm-symbolizer']
    .map(name => spawnSync('sh', ['-c', `command -v ${name}`], {encoding: 'utf8'}).stdout.trim())
    .find(path => path !== '')
if (symbolizer === undefined) {
    console.error('msan_planted: no llvm-symbolizer-18 or llvm-symbolizer on PATH; install ' +
        'llvm-18 (the lane constant-time lists it among its packages)')
    process.exit(1)
}
const run = spawnSync(program, [], {
    encoding: 'utf8',
    env: {...process.env, MSAN_OPTIONS: 'exitcode=77', MSAN_SYMBOLIZER_PATH: symbolizer},
})
process.stdout.write(run.stdout ?? '')
process.stderr.write(run.stderr ?? '')
const stderr = run.stderr ?? ''
const reported = stderr.includes('WARNING: MemorySanitizer: use-of-uninitialized-value')
// The planted branch is in main, in msan_test.cpp: the first frame or the SUMMARY line names it.
const inTheTest = /msan_test\.cpp:\d+/.test(stderr) ||
    /^SUMMARY: MemorySanitizer: use-of-uninitialized-value .* in main$/m.test(stderr)
if (run.status === 77 && reported && inTheTest) {
    console.log('msan_planted: MemorySanitizer reported the planted branch')
    process.exit(0)
}
console.error(`msan_planted: expected MemorySanitizer's report in msan_test.cpp and exit 77; ` +
    `got status ${run.status}, signal ${run.signal}, report ${reported}, in the test ${inTheTest}`)
process.exit(1)
```

  `package.json` is `{"name": "webcpp-crypto-constant-time", "private": true}` under no
  dependency, and `package-lock.json` npm's lockfile of it (`npm install --package-lock-only`
  writes it).
- [ ] **Step 2: Write `msan_test.cpp`**, the same checks through MemorySanitizer, every line
  of C and C++ instrumented: the test Jamfile builds BearSSL's sources once more with
  `-fsanitize=memory` (MemorySanitizer reports falsely through code it did not compile), and
  the program keeps BearSSL on its portable code (`WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE`),
  whose plain C MemorySanitizer follows exactly. It is built with
  `WEBCPP_CRYPTO_DETAIL_CT_MSAN`, so that the engine declassifies the tag's verdict (Step 0),
  and a twin with `WEBCPP_TEST_CRYPTO_PLANTED_LEAK`, a branch on a poisoned byte, must be
  reported by MemorySanitizer (`webcpp.drive msan_planted` through `msan_report.mjs`, Step 4,
  which passes on that report alone, n2). It prints with `std::printf` alone, since libstdc++'s compiled parts
  are not instrumented.

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// MemorySanitizer's poisoning (spec section 8.3): the key, the plaintext and the tag are
// poisoned, and a branch or an index that depends on them, in webcpp's code or BearSSL's,
// both instrumented, ends the run with a report.

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/gcm.hpp>

#include <sanitizer/msan_interface.h>

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    namespace crypto = webcpp::crypto;
    std::array<std::uint8_t, 16> a{};
    std::array<std::uint8_t, 16> b{};
    __msan_poison(a.data(), a.size());
    __msan_poison(b.data(), b.size());
    bool same = crypto::equal(a, b);
    __msan_unpoison(&same, sizeof same);

    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 64> data{};
    __msan_poison(key.data(), key.size());
    __msan_poison(data.data(), data.size());
#ifdef WEBCPP_TEST_CRYPTO_PLANTED_LEAK
    if (key[0] == 0) {  // the planted leak: MemorySanitizer must report it
        std::printf("planted\n");
    }
#endif
    auto made = crypto::gcm<crypto::aes256<crypto::backends::bearssl>>::make(key);
    auto tag = made->seal(iv, {}, data);
    __msan_poison(tag->data(), tag->size());
    // The verdict is declassified inside the engine (WEBCPP_CRYPTO_DETAIL_DECLASSIFY).
    auto const opened = made->open(iv, {}, data, *tag);
    bool const ok = opened.has_value() && same;
    std::printf("constant time under MemorySanitizer: %s\n", ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
```

- [ ] **Step 3: Write `dudect.hpp` and `dudect_test.cpp`** (spec 8.3, statistical): the
  method of Reparaz, Balasch and Verbauwhede (dudect, 2017): two classes of inputs, a fixed
  secret and random secrets, measured interleaved in random order, the measurements cropped
  at the percentiles `1 - 0.5^(10 * (k + 1) / 100)` for k in 0..99, and Welch's t-test on
  each cropped set and on the whole; the test fails when any `|t|` exceeds 10, dudect's
  threshold, after 2,000,000 measurements (about 1,000,000 per class). Timer:
  `std::chrono::steady_clock`, natively and under wasmtime. `dudect.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Welch's t-test over two classes of timings, with dudect's cropping.

#ifndef WEBCPP_TEST_CRYPTO_DUDECT_HPP
#define WEBCPP_TEST_CRYPTO_DUDECT_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace webcpp::test {

/** Online mean and variance of one class, Welford's. */
struct moments {
    double n = 0;
    double mean = 0;
    double m2 = 0;
    void push(double x) {
        n += 1;
        double const delta = x - mean;
        mean += delta / n;
        m2 += delta * (x - mean);
    }
    double variance() const { return n > 1 ? m2 / (n - 1) : 0; }
};

/** Welch's t of two classes. */
inline double welch(moments const& a, moments const& b) {
    double const denominator = std::sqrt(a.variance() / a.n + b.variance() / b.n);
    return denominator == 0 ? 0 : (a.mean - b.mean) / denominator;
}

/** The largest |t| over the whole set and dudect's 100 croppings. */
inline double largest_t(std::vector<double> const& timings, std::vector<int> const& classes) {
    std::vector<double> sorted = timings;
    std::sort(sorted.begin(), sorted.end());
    double largest = 0;
    for (int k = -1; k < 100; ++k) {
        double const keep = k < 0 ? 1.0 : 1.0 - std::pow(0.5, 10.0 * (k + 1) / 100.0);
        double const limit = sorted[static_cast<std::size_t>(keep * static_cast<double>(sorted.size() - 1))];
        std::array<moments, 2> m{};
        for (std::size_t i = 0; i < timings.size(); ++i) {
            if (timings[i] <= limit) {
                m[static_cast<std::size_t>(classes[i])].push(timings[i]);
            }
        }
        largest = (std::max)(largest, std::fabs(welch(m[0], m[1])));
    }
    return largest;
}

/**
 * The largest |t| of `count` measurements of `run`, each after `prepare(class, generator)`
 * with a class drawn at random from the seeded generator, prepare unmeasured.
 */
template <class Prepare, class Run>
double measure(std::uint64_t seed, std::size_t count, Prepare&& prepare, Run&& run) {
    std::mt19937_64 generator(seed);
    std::vector<double> timings;
    std::vector<int> classes;
    timings.reserve(count);
    classes.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        int const which = static_cast<int>(generator() & 1U);
        prepare(which, generator);
        auto const start = std::chrono::steady_clock::now();
        run();
        auto const end = std::chrono::steady_clock::now();
        timings.push_back(std::chrono::duration<double, std::nano>(end - start).count());
        classes.push_back(which);
    }
    return largest_t(timings, classes);
}

/** The largest |t| of a first run, and of a second with another seed when the first exceeds 10. */
template <class Prepare, class Run>
double twice(std::size_t count, Prepare&& prepare, Run&& run) {
    double const first = measure(20261011, count, prepare, run);
    return first < 10 ? first : (std::min)(first, measure(20261012, count, prepare, run));
}

}  // namespace webcpp::test

#endif
```

  `dudect_test.cpp`, whose rerun policy is the lane's answer to shared, noisy runners (m7): a
  secret whose `|t|` exceeds 10 is measured once more with another seed (the program's own
  `20261012`), and the test fails only when both runs exceed it; the threshold never moves.
  `measure` takes the seed as its first argument, and the program's `main` calls it through a
  helper `twice(...)` that does the rerun:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// dudect's statistics on BearSSL (spec section 8.3): seal's time against its key, open's
// against its tag (the failure's zeroing included), and equal's against its second
// argument; natively and under wasmtime, where the engine's compiler decides whether
// BearSSL's 64-bit multiplication stays constant time.

#include "dudect.hpp"

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/equal.hpp>
#include <webcpp/crypto/gcm.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>

int main() {
    namespace crypto = webcpp::crypto;
    using aead = crypto::gcm<crypto::aes256<crypto::backends::bearssl>>;
    constexpr std::size_t count = 2'000'000;
    std::array<std::uint8_t, 32> key{};
    std::array<std::uint8_t, 12> const iv{};
    std::array<std::uint8_t, 64> data{};
    std::optional<aead> made;

    double const seal_t = webcpp::test::twice(
        count,
        [&](int which, auto& generator) {
            // Both classes draw the same random bytes and do the same work (m7); the class
            // only selects which key is used.
            for (auto& b : key) {
                auto const drawn = static_cast<std::uint8_t>(generator());
                b = which == 0 ? 0 : drawn;
            }
            made.emplace(std::move(*aead::make(key)));
        },
        [&] { static_cast<void>(made->seal(iv, {}, data)); });

    key.fill(0x42);
    made.emplace(std::move(*aead::make(key)));
    std::array<std::uint8_t, 64> const plaintext{};
    std::array<std::uint8_t, 64> ciphertext = plaintext;
    auto const right = *made->seal(iv, {}, ciphertext);
    std::array<std::uint8_t, 16> tag{};
    // Both classes fail (I9): the right tag with its last byte flipped, against random wrong
    // tags, the classic probe of an early exit; a success and a failure differ by design (the
    // failure zeroes data and returns another way), and that difference is public.
    double const open_t = webcpp::test::twice(
        count,
        [&](int which, auto& generator) {
            data = ciphertext;
            for (std::size_t i = 0; i < tag.size(); ++i) {
                auto const drawn = static_cast<std::uint8_t>(generator());
                tag[i] = which == 0 ? right.data()[i] : drawn;
            }
            if (which == 0) {
                tag[15] ^= 1;
            } else if (std::equal(tag.begin(), tag.end(), right.data())) {
                tag[15] ^= 1;  // a random tag equal to the right one, which never happens
            }
        },
        [&] { static_cast<void>(made->open(iv, {}, data, tag)); });

    std::array<std::uint8_t, 16> a{};
    std::array<std::uint8_t, 16> b{};
    bool volatile sink = false;  // keeps the comparison from being optimised away
    double const equal_t = webcpp::test::twice(
        count,
        [&](int which, auto& generator) {
            for (auto& byte : b) {
                auto const drawn = static_cast<std::uint8_t>(generator());
                byte = which == 0 ? 0 : drawn;
            }
        },
        [&] { sink = crypto::equal(a, b); });

    std::printf("|t| seal %.2f, open %.2f, equal %.2f (threshold 10)\n", seal_t, open_t,
                equal_t);
    return seal_t < 10 && open_t < 10 && equal_t < 10 ? 0 : 1;
}
```

- [ ] **Step 4: Write `test/constant_time/Jamfile`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# Constant time (spec section 8.3): Valgrind and MemorySanitizer on Linux, and
# dudect's statistics natively and under wasmtime, as the own lane
# constant-time, outside the ordinary lanes.
project : requirements <library>/webcpp/crypto//crypto ;

import webcpp ;

webcpp.original node ;

webcpp.drive ctgrind : ctgrind_test.cpp
  : <library>/third_party//bearssl <library>/third_party//openssl <variant>release
    <debug-symbols>on <define>WEBCPP_CRYPTO_DETAIL_CT_VALGRIND <webcpp-drive-timeout>300
  : valgrind.mjs : : native ;
# The planted leak: the same program must fail under memcheck (I8).
webcpp.drive ctgrind_planted : ctgrind_planted_test.cpp
  : <library>/third_party//bearssl <library>/third_party//openssl <variant>release
    <debug-symbols>on <define>WEBCPP_CRYPTO_DETAIL_CT_VALGRIND
    <define>WEBCPP_TEST_CRYPTO_PLANTED_LEAK <webcpp-drive-timeout>300
  : valgrind.mjs : --expect-leak : native ;
# BearSSL's sources once more, instrumented, for MemorySanitizer alone, with
# third_party/Jamfile's defines for Linux and webcpp's canary.
path-constant BEARSSL : ../../../../third_party/libs/bearssl ;
lib bearssl_msan
  : [ glob $(BEARSSL)/src/*.c $(BEARSSL)/src/*/*.c ] ../../src/canary_bearssl.c
  : <link>static <warnings>off <warnings-as-errors>off <include>$(BEARSSL)/inc
    <include>$(BEARSSL)/src <include>../../include <define>BR_RDRAND=0
    <define>BR_USE_URANDOM=0 <define>BR_USE_GETENTROPY=1 <define>_DEFAULT_SOURCE
    <cflags>-fsanitize=memory
  : : <include>$(BEARSSL)/inc <define>WEBCPP_CRYPTO_HAS_BEARSSL=1 ;
explicit bearssl_msan ;
webcpp.run msan : msan_test.cpp
  : <library>bearssl_msan <define>WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE
    <define>WEBCPP_CRYPTO_DETAIL_CT_MSAN <cxxflags>-fsanitize=memory
    <linkflags>-fsanitize=memory <variant>release <debug-symbols>on : native ;
# The planted leak under MemorySanitizer: passes only on MemorySanitizer's own
# report of it, never on any other failure (n2).
webcpp.drive msan_planted : msan_planted_test.cpp
  : <library>bearssl_msan <define>WEBCPP_CRYPTO_DETAIL_BEARSSL_PORTABLE
    <define>WEBCPP_CRYPTO_DETAIL_CT_MSAN <define>WEBCPP_TEST_CRYPTO_PLANTED_LEAK
    <cxxflags>-fsanitize=memory <linkflags>-fsanitize=memory <variant>release
    <debug-symbols>on
  : msan_report.mjs : : native ;
webcpp.run dudect_bearssl : dudect_test.cpp
  : <library>/third_party//bearssl <variant>release : native wasip2 ;

webcpp.drive ctgrind_speed : ctgrind_test.cpp
  : <library>/third_party//bearssl <library>/third_party//openssl <variant>release
    <optimization>speed <debug-symbols>on <define>WEBCPP_CRYPTO_DETAIL_CT_VALGRIND
    <webcpp-drive-timeout>300
  : valgrind.mjs : : native ;
webcpp.drive ctgrind_space : ctgrind_test.cpp
  : <library>/third_party//bearssl <library>/third_party//openssl <variant>release
    <optimization>space <debug-symbols>on <define>WEBCPP_CRYPTO_DETAIL_CT_VALGRIND
    <webcpp-drive-timeout>300
  : valgrind.mjs : : native ;

webcpp.lane constant-time : ctgrind ctgrind_planted ctgrind_speed ctgrind_space msan
    msan_planted dudect_bearssl
  : native wasip2 ;
```

  `equal` compiles in the user's translation unit, so it is checked at both optimisations of
  spec 2.6: `ctgrind_test.cpp` built twice more, `ctgrind_speed` and `ctgrind_space`, with
  `<optimization>speed` and `<optimization>space`, each a `webcpp.drive` of its own named by
  the lane. `ctgrind_planted_test.cpp` and `msan_planted_test.cpp` are one line each,
  `#include "ctgrind_test.cpp"` and `#include "msan_test.cpp"`: a source of their own, so
  that b2 compiles each with its own defines (spec 4.3's pitfall). `msan` builds only with
  clang on Linux, where the lane runs; spec 8's table puts the deterministic checks on Linux
  x86-64 and arm64, and the lane is not run on macOS. On the CI the own lanes run on the
  runner, never in the container (`matrix.py`'s `run_own_lane` does not contain them, I10), so
  the lane's packages come with its entry: `matrix.py` gains
  `OWN_LANE_PACKAGES = {('crypto', 'constant-time'): ('valgrind', 'llvm-18')}` (`llvm-18`
  for `llvm-symbolizer-18`, which `msan_report.mjs` needs to read MemorySanitizer's frames,
  r2; a no-op where the image already has it), an own lane's entry gains
  `"apt"`, the list for its library and lane (`[]` otherwise), and library.yml's own-lanes job
  a step `if: runner.os == 'Linux' && join(matrix.apt, ' ') != ''` that runs
  `sudo apt-get install --yes --no-install-recommends ${{ join(matrix.apt, ' ') }}`;
  `matrix_test.py` gains its case. The runner's clang-18 brings MemorySanitizer's runtime.
- [ ] **Step 5: Run** the lane on a Linux machine with valgrind, clang-18 and llvm-symbolizer-18 (the CI's
  own-lanes job, or `python3 tools/ci/matrix.py own-lane '<the entry of own-lanes for
  constant-time on native>'` on such a machine), and on wasip2 locally. Expected: PASS;
  `ctgrind` prints `constant time under memcheck: ok`; `ctgrind_planted` passes on memcheck's
  exit status 99 alone and `msan_planted` on MemorySanitizer's report of the planted line
  alone (n2), each failing on any other way its program could end; each `|t|` printed is
  below 10. A `|t|` above 10 after the
  rerun is investigated (the program alone, the classes' means) before anything else; the
  threshold is never raised to pass. On macOS the native part of the lane is not run, which
  the ledger records; the CI and `runners.yml`'s arm64 job run it.
- [ ] **Step 6: Commit** (library) `"crypto: constant time under Valgrind, MemorySanitizer
  and dudect's statistics, each shown to fail on a planted leak"`; (superproject)
  `"tools/ci: an own lane's packages, installed with it"`.

## Task 21: A library named and not called adds no byte (`webcpp.same-program`)

**Files (superproject):**
- Create: `tools/same_program/same_program.jam`, `tools/same_program/compare.py`.
- Modify: `tools/webcpp.jam` (load the module; the rule; the list of rules at its head),
  `tools/test/webcpp_jam_test.py` (cases), `AGENTS.md` (chapter 4, "The Jamfile API
  (`tools/webcpp.jam`)": the rule's row), `doc/releases/0.1.0.adoc` ("Tools and build": the
  rule's line).
- Create (library): `test/unused/Jamfile`, `test/unused/app.cpp`.

**Interfaces:**
- Produces: `webcpp.same-program <name> : <sources> + : <library> : <targets> * : <requirements> * ;`
  (C9): a test of type `SAME_PROGRAM`, listed by `--dump-tests` and recorded by `--out-xml`,
  that passes when `<sources>` built with `<library>` and without it, as two `exe`s of b2 under
  one output name in two directories of their own, give byte-identical files (on emscripten
  the `.js` and the `.wasm` beside it). `<requirements>` are properties
  (`<library>/webcpp/crypto//crypto`). No b2 runs inside b2 (owner item O4). Nothing on the
  base builds a pair and compares it; `tools/generated/generated.jam`, which compares outputs
  byte for byte in b2, is the nearest pattern.

- [ ] **Step 1: Write the failing tools tests** in `tools/test/webcpp_jam_test.py`, on the
  fixture `demo` (a static library `unused` added to its `test/Jamfile` by the case, one
  function never called, and a program `hello.cpp`):
  - `test_a_library_named_and_not_called_adds_nothing`:
    `webcpp.same-program hello_unused : hello.cpp : unused ;` passes natively and on wasip2
    (`harness.link_wasi_tools`);
  - `test_a_library_called_differs`: with `hello.cpp` calling the function, the test fails,
    and its output names both files and their sizes;
  - `test_the_pair_shares_one_name`: `-d+2` shows both programs linked as `hello_unused`
    (`hello_unused.wasm` on wasip2) in two directories.
  Each case is a function `test_...(root)` added to the list the file gives
  `harness.run_cases`, as the existing cases are, so the CI's tools job shards them.
  Run them. Expected: FAIL, `webcpp.same-program` unknown.
- [ ] **Step 2: Write `tools/same_program/same_program.jam`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# The rule of a library named and not called (CODING-GUIDELINES.md chapter 5,
# rule 11),
# which tools/webcpp.jam offers as webcpp.same-program: two exes of the same
# sources, one with the library and one without, each renamed by a tag rule
# to the test's name so that both outputs have the same name (ld64's ad hoc
# signature embeds it), each in a directory of its own (b2's directory of a
# target is named after the target), and a test whose action compares them
# byte for byte. Release, without debug information, and, with MSVC,
# /Brepro, which keeps a time stamp out of the link. This file is loaded as
# the module webcpp-same-program.
import feature ;
import generators ;
import modules ;
import os ;
import path ;
import project ;
import testing ;
import type ;

type.register SAME_RESULT : same-result ;
type.register SAME_PROGRAM : : TEST ;
generators.register-composing webcpp-same-program.compare : EXE : SAME_RESULT ;
generators.register-standard testing.expect-success : SAME_RESULT : SAME_PROGRAM ;

# The output name of both programs of a pair. Free and incidental: the two
# programs' directories differ by their targets' names, never by this.
feature.feature webcpp-same-name : : free incidental ;

if [ os.name ] = NT { .python = python ; } else { .python = python3 ; }
.here = [ path.parent [ path.root [ path.make [ modules.binding $(__name__) ] ] [ path.pwd ] ] ] ;
.compare = [ path.native $(.here)/compare.py ] ;

# The tag rule: an executable takes the pair's name and keeps its suffix.
rule tag ( name : type ? : property-set )
{
    local same = [ $(property-set).get <webcpp-same-name> ] ;
    if $(same) && [ type.is-derived $(type) EXE ]
    {
        return $(same)$(name:S) ;
    }
}

# found is tools/webcpp.jam's `where`; only is the program's requirements of
# its targets.
rule same-program ( found + : name : sources + : library : only * : requirements * )
{
    local pair = $(requirements) $(only) <variant>release <debug-symbols>off
        <toolset>msvc:<linkflags>/Brepro <toolset>msvc:<cxxflags>/Brepro
        <webcpp-same-name>$(name) <tag>@webcpp-same-program.tag ;
    exe $(name)_with : $(sources) $(library) : $(pair) ;
    exe $(name)_without : $(sources) : $(pair) ;
    $(found[1]).mark-target-as-explicit $(name)_with $(name)_without ;
    testing.make-test same-program : $(name)_with $(name)_without : $(only) : $(name) ;
}

rule compare ( target : sources * : properties * )
{
    PYTHON on $(target) = $(.python) ;
    COMPARE on $(target) = $(.compare) ;
}

actions compare
{
    "$(PYTHON)" "$(COMPARE)" --output "$(<)" "$(>[1])" "$(>[2])"
}
```

  `tools/same_program/compare.py` compares the two files, and for a `.js` the `.wasm` of the
  same stem beside each, writes what it compared to `--output`, and exits 1 naming each pair
  that differs with both sizes:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Compares the two programs of webcpp.same-program byte for byte, and on emscripten the .wasm
beside each .js. Usage: compare.py --output FILE WITH WITHOUT"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    parser.add_argument('with_library')
    parser.add_argument('without_library')
    args = parser.parse_args(argv)
    pairs = [(Path(args.with_library), Path(args.without_library))]
    if pairs[0][0].suffix == '.js':
        pairs.append((pairs[0][0].with_suffix('.wasm'), pairs[0][1].with_suffix('.wasm')))
    lines, faults = [], []
    for a, b in pairs:
        same = a.read_bytes() == b.read_bytes()
        line = f'{a} ({a.stat().st_size} bytes) and {b} ({b.stat().st_size} bytes)'
        lines.append(('same: ' if same else 'differ: ') + line)
        if not same:
            faults.append(line)
    Path(args.output).write_text('\n'.join(lines) + '\n')
    for fault in faults:
        print(f'same_program: differ: {fault}', file=sys.stderr)
    return 1 if faults else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

  In `tools/webcpp.jam`, after the loads of the drive and generated modules:

```jam
# The rule of a library named and not called, loaded as the module
# webcpp-same-program, as the documentation's are.
modules.load webcpp-same-program : same_program.jam
  : [ path.native [ path.join $(.tools) same_program ] ] ;
import webcpp-same-program ;

# A test that builds sources with library and without it, under one output
# name, in two directories of their own, and passes when the two files are
# byte-identical: a library named and not called adds no byte
# (CODING-GUIDELINES.md chapter 5, rule 11).
rule same-program ( name : sources + : library : targets * : requirements * )
{
    local only = [ program same-program : $(name) : $(targets) ] ;
    webcpp-same-program.same-program [ where ] : $(name) : $(sources) : $(library) : $(only)
      : $(requirements) ;
}
```

  The `exe` rule is b2's, called from a module of `tools/`, as `webcpp.example` calls it: the
  lint's raw-rules check reads a library's test and example Jamfiles, not `tools/`. The list
  of rules at the head of `tools/webcpp.jam` gains
  `#     webcpp.same-program name : sources + : library : targets * : requirements * ;`,
  AGENTS.md chapter 4's Jamfile API a row for it (what it passes on, and that its pair is
  release, without debug information, `/Brepro` on MSVC), and the release notes the line
  `* Added webcpp.same-program, which proves a library named and not called adds no byte.`
- [ ] **Step 3: Run** the three tools cases. Expected: PASS. If the tag rule does not rename
  the emscripten pair's `.wasm` as it renames the `.js` (the asio plan measured the `.js`,
  `Q/b2-tag/`), `compare.py` reads the `.wasm` by the name the link wrote, which `-d+2` shows,
  and the case `test_the_pair_shares_one_name` gains the emscripten form; on MSVC, if
  `/Brepro` leaves the pair different, the first run of the CI's MSVC lanes records the
  differing bytes' offsets and the case is reported to the owner, the rule otherwise
  unchanged.
- [ ] **Step 4: Write crypto's cases** (spec 8.7), `test/unused/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# A backend built from source, named and not called, adds no byte, on every
# target it builds on (spec sections 4.2 and 8.7). OpenSSL is refused on
# emscripten.
import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

webcpp.same-program unused_bearssl : app.cpp : /third_party//bearssl
  : : <library>/webcpp/crypto//crypto ;
webcpp.same-program unused_openssl : app.cpp : /webcpp/crypto//openssl
  : native wasip2 wasip3 : <library>/webcpp/crypto//crypto ;
```

  `app.cpp` draws 16 bytes with `random_device` and prints them in hex (so that the program
  is crypto's and calls nothing of either backend's; it includes
  `<webcpp/crypto/random_device.hpp>` alone). Tasks 26 and 29 add libsecp256k1's and ctaes's
  lines. In `test/Jamfile`, `build-project unused ;`. `unused_openssl` is a measurement: the
  provider's usage requirements add link flags (`-ldl` on Linux, Windows' system libraries,
  WASI's emulation libraries, `tools/webcpp-openssl.jam`'s `declare`), which may leave the
  pair different. If it differs on a lane, the ledger records the differing bytes and the
  flag that adds them, the case is reported to the owner, and Task 25 records the limit in
  CODING-GUIDELINES.md chapter 19; if it passes everywhere, nothing is recorded.
- [ ] **Step 5: Run** natively, `toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh`,
  `toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh` and
  `toolset=emscripten`, each `b2 -a libs/crypto/test/unused`. Expected: PASS:
  `unused_bearssl` on each target (C-LIBS:495-501 measured it on all four), and
  `unused_openssl` as measured. The library's CI then runs both on every lane of the matrix
  (gcc-14, gcc-15, clang-18, clang-22, Apple Clang, MSVC 14.3 and 14.5, emscripten, wasip2,
  wasip3); the ledger records each lane's result.
- [ ] **Step 6: Commit** (superproject, after `git add -- tools/same_program`)
  `"tools: webcpp.same-program, a library named and not called adds no byte"`; (library)
  `"crypto: a backend named and not called adds no byte on any target"`.

## Task 22: Sizes and throughput

Deferred after v0.1 (revision 2026-11-14). Kept as written for its own change; before it runs,
re-read it against the tree of that day: OpenSSL is source-built and static
(`/webcpp/crypto//openssl`), so `test/sizes/native_static/` and a shared OpenSSL's rows go;
there is no `webcpp-own`, and the CI has no levels yet; the own-lanes job already uploads
`bin/**/*.sizes.md` (`library.yml`, "Upload the lane's size tables"). Until it runs, crypto has
no size table, a limit CODING-GUIDELINES.md chapter 19 records (Task 25), which this task
removes; Task 29's signing program in `test/sizes/` comes back with it.

**Files:**
- Create: `test/sizes/Jamfile`, `test/sizes/wasm/Jamfile`, `test/sizes/native/Jamfile`,
  `test/sizes/native_static/Jamfile`, `test/sizes/baseline.cpp`, `test/sizes/gcm_bearssl.cpp`,
  `test/sizes/gcm_openssl.cpp`, `test/sizes/sizes.py`, `doc/sizes.adoc` (generated, checked);
  `test/throughput/Jamfile`, `test/throughput/throughput.cpp`;
  `.github/workflows/runners.yml`.
- Modify: `test/Jamfile` (`build-project sizes ; build-project throughput ;`).

**Interfaces:**
- Produces: the own lanes `sizes` (native, wasip2, wasip3, emscripten) and `throughput`
  (native, wasip2, wasip3, emscripten); `sizes.py table <json>...` and `sizes.py doc <json>...`;
  the job summaries of `runners.yml`.

- [ ] **Step 1: Write the sized programs and their Jamfiles.** `baseline.cpp` prints
  `baseline` and returns 0; `gcm_bearssl.cpp` and `gcm_openssl.cpp` are spec 7.2's `app.cpp`
  with the backend named (`gcm<aes256<backends::bearssl>>`, resp. `openssl`), so that each
  measures its backend whatever the default. b2 skips a `make` whole when one of its sources
  is skipped on a target, and the build still succeeds (measured, review `R/skip_make_Jamroot`:
  "...found 1 target...", exit status 0, no file; I11). So every `make` of sizes lists only
  programs its Jamfile builds on every target it declares: one Jamfile per set of targets,
  `test/sizes/wasm/` (wasip2, wasip3, emscripten), `test/sizes/native/` (BearSSL and a shared
  OpenSSL) and `test/sizes/native_static/` (a static OpenSSL, which a machine without
  `libcrypto.a` leaves out with Task 11's line, alone). `test/sizes/wasm/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The size of a program per backend on WebAssembly (spec section 8.8), each
# linked and never run, at b2's two optimisations of a release, speed (-O3 with
# clang) and space (-Os), stripped; written by sizes.py into sizes.json and
# compared with the documentation's table (doc/sizes.adoc). Every program here
# is built on every target this Jamfile declares, so the make below is never
# skipped whole on one of them (I11).
import path ;
import webcpp ;

webcpp.targets wasip2 wasip3 emscripten ;

.speed = <variant>release <optimization>speed <strip>on ;
.space = <variant>release <optimization>space <strip>on ;

webcpp.link baseline_speed : ../baseline.cpp : $(.speed) ;
webcpp.link baseline_space : ../baseline.cpp : $(.space) ;
webcpp.link gcm_bearssl_speed : ../gcm_bearssl.cpp
  : <library>/webcpp/crypto//crypto <library>/third_party//bearssl $(.speed) ;
webcpp.link gcm_bearssl_space : ../gcm_bearssl.cpp
  : <library>/webcpp/crypto//crypto <library>/third_party//bearssl $(.space) ;

path-constant HERE : .. ;
.programs = baseline_speed-program baseline_space-program gcm_bearssl_speed-program
    gcm_bearssl_space-program ;

rule sizes ( targets * : sources * : properties * )
{
    SCRIPT on $(targets) = [ path.native $(HERE)/sizes.py ] ;
    DOC on $(targets) = [ path.native $(HERE)/../../doc/sizes.adoc ] ;
    TARGET on $(targets) = [ webcpp.target-of $(properties) ] ;
    EXPECTED on $(targets) = $(.programs:S=) ;
}

actions sizes
{
    python3 "$(SCRIPT)" table --target $(TARGET) --doc "$(DOC)" --out "$(<)" --expect $(EXPECTED) -- $(>)
}

make sizes.json : $(.programs) : @sizes ;
explicit sizes.json ;
webcpp.lane sizes : sizes.json : wasip2 wasip3 emscripten ;
```

  `test/sizes/native/Jamfile` is the same with `webcpp.targets native ;`, the four BearSSL
  and baseline programs and `gcm_openssl_speed` (`<library>/third_party//openssl`), and
  `webcpp.lane sizes : sizes.json : native ;`; `test/sizes/native_static/Jamfile` holds
  `gcm_openssl_static_speed` (`<webcpp-crypto-openssl-link>static`) and its own `sizes.json`
  in the same lane. The three directories are `build-project`ed from `test/sizes/Jamfile`,
  which declares nothing else. `--expect` gives `sizes.py` the programs the make was written
  for: a program that is not among its sources fails the action, naming it, a second net
  under the split.

  `webcpp.target-of` is `tools/target.jam`'s `target-of`, exported by `tools/webcpp.jam` in
  this task (one line, `rule target-of ( properties * ) { return [ webcpp-target.target-of $(properties) ] ; }`,
  with its case in `tools/test/webcpp_jam_test.py`), and the same file gains a case that pins
  b2's behaviour this step works around, `test_a_make_with_a_skipped_source_is_skipped_whole`
  (the review's `skip_make_Jamroot`, so that a later b2 that changes it is noticed). Python is
  run as `python3`, as the other actions of `tools/` run it (`python` on Windows, through the
  same `if [ os.name ] = NT` of `tools/webcpp.jam`).
- [ ] **Step 2: Write `test/sizes/sizes.py`:**

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""The sizes of crypto's sized programs, and the documentation's table of them.

Usage: sizes.py table --target T --doc FILE --out JSON --expect NAME... -- PROGRAM...
       sizes.py doc JSON...

table writes JSON, one row per program file: target, program, optimisation, raw bytes and
gzip -9n bytes (the .wasm on WebAssembly, the .js beside it on emscripten, the executable
natively). On a WebAssembly target, whose toolchain is pinned, it fails when a row differs from
the table of FILE, naming the command that writes the table again; natively it checks nothing.
doc prints the AsciiDoc table of every row of the JSON files given."""

from __future__ import annotations

import argparse
import gzip
import json
import sys
from pathlib import Path


def gzipped(data: bytes) -> int:
    """The size gzip -9n gives: no name and no time stamp in the header."""
    return len(gzip.compress(data, compresslevel=9, mtime=0))


def rows(target: str, programs: list[str]) -> list[dict[str, object]]:
    out = []
    for program in programs:
        path = Path(program)
        files = [path]
        if path.suffix == '.js':
            files.append(path.with_suffix('.wasm'))
        name = path.stem.removesuffix('-program')
        base, _, optimisation = name.rpartition('_')
        for file in files:
            data = file.read_bytes()
            out.append({'target': target, 'program': base, 'optimisation': optimisation,
                        'file': file.suffix or 'executable', 'raw': len(data),
                        'gzip': gzipped(data)})
    return out


def table(all_rows: list[dict[str, object]]) -> str:
    lines = ['[%header,cols="2,3,1,1,>1,>1"]', '|===',
             '| Target | Program | Optimisation | File | Raw bytes | gzip -9n bytes']
    for r in sorted(all_rows, key=lambda r: (str(r['target']), str(r['program']),
                                             str(r['optimisation']), str(r['file']))):
        lines.append(f"| {r['target']} | {r['program']} | {r['optimisation']} | {r['file']} "
                     f"| {r['raw']:,} | {r['gzip']:,}")
    lines.append('|===')
    return '\n'.join(lines) + '\n'


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest='command', required=True)
    write = commands.add_parser('table')
    write.add_argument('--target', required=True)
    write.add_argument('--doc', required=True)
    write.add_argument('--out', required=True)
    write.add_argument('--expect', nargs='+', required=True)
    write.add_argument('programs', nargs='+')
    doc = commands.add_parser('doc')
    doc.add_argument('files', nargs='+')
    args = parser.parse_args(argv)
    if args.command == 'doc':
        sys.stdout.write(table([r for f in args.files for r in json.loads(Path(f).read_text())]))
        return 0
    built = {Path(p).stem.removesuffix('-program') for p in args.programs}
    absent = sorted(set(e.removesuffix('-program') for e in args.expect) - built)
    if absent:
        print(f'sizes.py: not built: {", ".join(absent)}', file=sys.stderr)
        return 1
    measured = rows(args.target, args.programs)
    Path(args.out).write_text(json.dumps(measured, indent=4) + '\n')
    if args.target == 'native':
        sys.stdout.write(table(measured))
        return 0
    documented = Path(args.doc).read_text()
    missing = [line for line in table(measured).splitlines()
               if line.startswith(f'| {args.target} ') and line not in documented]
    for line in missing:
        print(f'sizes.py: doc/sizes.adoc lacks: {line}', file=sys.stderr)
    if missing:
        print('sizes.py: run the lane sizes on every target, then python3 '
              'libs/crypto/test/sizes/sizes.py doc <each sizes.json> > libs/crypto/doc/sizes.adoc',
              file=sys.stderr)
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

  b2's `speed` is `-O3` with clang and gcc and spec 8.8 says -O2: the table measures what a
  user's b2 release builds (C17), and the documentation says which flags.
- [ ] **Step 3: Write the table.** The pinned toolchains should give the same WebAssembly bytes
  on macOS and Linux; that is measured first (m19): the lane `sizes` on wasip2 locally and in
  the CI's own lane, its two `sizes.json` compared. If they agree, the table may be written
  locally; if not, its WebAssembly rows are written from the CI's artifacts alone (the own
  lane's job uploads `sizes.json`, `gh run download` fetches it) and the check runs on the CI
  alone, which the action's comment says. Run the lane `sizes` on each target, each with its
  own build directory, then
  `python3 libs/crypto/test/sizes/sizes.py doc bin/lane-sizes-native/**/sizes.json bin/lane-sizes-wasip2/**/sizes.json bin/lane-sizes-wasip3/**/sizes.json bin/lane-sizes-emscripten/**/sizes.json > libs/crypto/doc/sizes.adoc`
  (expanding the globs with `find`), with the licence notice prepended. Run the lane again on
  wasip2. Expected: PASS. The BearSSL rows are compared with C-LIBS's (wasip2 -Os stripped
  59,887 bytes; Emscripten -Os 26,387): a difference of more than 5% is investigated (a flag
  of the build, the strip) before the table is committed.
- [ ] **Step 4: Write `test/throughput/throughput.cpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Seal and open of 100 B and of 1 MiB per backend, three runs each, in MiB/s (spec section
// 8.8): the measurement the documentation's table of section 5 quotes. It passes when it
// ran: its output is the result.

#include <webcpp/crypto/aes.hpp>
#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/gcm.hpp>
#if defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#include <webcpp/crypto/backends/openssl.hpp>
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

template <class Backend>
void measure(std::size_t size) {
    namespace crypto = webcpp::crypto;
    std::array<std::uint8_t, 32> const key{};
    std::array<std::uint8_t, 12> const iv{};
    std::vector<std::uint8_t> data(size);
    auto made = crypto::gcm<crypto::aes256<Backend>>::make(key);
    std::size_t const rounds = size >= (1U << 20) ? 64 : 200'000;
    for (int run = 0; run < 3; ++run) {
        auto const start = std::chrono::steady_clock::now();
        crypto::gcm<crypto::aes256<Backend>>::result_type tag{};
        for (std::size_t i = 0; i < rounds; ++i) {
            tag = *made->seal(iv, {}, data);
        }
        auto const sealed = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < rounds; ++i) {
            static_cast<void>(made->open(iv, {}, data, tag));
            static_cast<void>(made->seal(iv, {}, data));
        }
        auto const opened = std::chrono::steady_clock::now();
        double const mib = static_cast<double>(size * rounds) / (1 << 20);
        double const seal_s = std::chrono::duration<double>(sealed - start).count();
        double const open_s = std::chrono::duration<double>(opened - sealed).count() - seal_s;
        std::printf("%s seal %zu B %.1f MiB/s\n", Backend::name.data(), size, mib / seal_s);
        std::printf("%s open %zu B %.1f MiB/s\n", Backend::name.data(), size, mib / open_s);
    }
}

}  // namespace

int main() {
    for (std::size_t size : {std::size_t{100}, std::size_t{1} << 20}) {
        measure<webcpp::crypto::backends::bearssl>(size);
#if defined(WEBCPP_CRYPTO_HAS_OPENSSL)
        measure<webcpp::crypto::backends::openssl>(size);
#endif
    }
    return 0;
}
```

  The open loop seals again after each open, since a failed open zeroes the data and a good
  one leaves plaintext: the open's time is the loop's less the seals'. `test/throughput/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/crypto//crypto <variant>release ;

import webcpp ;

webcpp.run throughput : throughput.cpp : <library>/third_party//bearssl
  : wasip2 wasip3 emscripten ;
webcpp.run throughput_native : throughput.cpp
  : <library>/third_party//bearssl <library>/third_party//openssl : native ;
webcpp.lane throughput : throughput throughput_native : native wasip2 wasip3 emscripten ;
```

- [ ] **Step 5: Write `.github/workflows/runners.yml`** (C4, C16), nightly (03:17 UTC, owner
  gate O2) and on `workflow_dispatch`, never on a push:

```yaml
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# What crypto measures on runners the superproject's matrix lacks (spec sections 8.3 and
# 8.8): throughput on Linux x86-64 and arm64 and on macOS, natively, and on x86-64 under
# wasmtime and node; and the deterministic constant-time checks on Linux arm64. Each job
# checks out the superproject with its submodules, puts this commit in libs/crypto, and
# runs crypto's own lanes through the superproject's tools/ci/matrix.py.
name: Runners

on:
  # The nightly level (owner gate O2): never on a push.
  schedule:
    - cron: '17 3 * * *'
  workflow_dispatch:

permissions:
  contents: read

jobs:
  throughput:
    name: Throughput (${{ matrix.os }})
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-24.04, ubuntu-24.04-arm, macos-15]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          repository: webcpporg/webcpp
          submodules: recursive
          persist-credentials: false
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          path: libs/crypto
          persist-credentials: false
      - uses: ./tools/ci/actions/boost
      - uses: ./tools/ci/actions/openssl
      - name: Throughput, natively
        shell: bash
        run: |
          entry="$(python3 tools/ci/matrix.py own-lanes --library crypto \
            | python3 -c 'import json,sys; print(json.dumps(next(e for e in json.load(sys.stdin)["include"] if e["lane"] == "throughput" and e.get("platform") == "native")))')"
          python3 tools/ci/matrix.py own-lane "$entry" --out-dir out
          find bin -name 'throughput_native.output' -exec cat {} + >> "$GITHUB_STEP_SUMMARY"

  constant-time-arm:
    name: Constant time (ubuntu-24.04-arm)
    runs-on: ubuntu-24.04-arm
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          repository: webcpporg/webcpp
          submodules: recursive
          persist-credentials: false
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          path: libs/crypto
          persist-credentials: false
      - uses: ./tools/ci/actions/boost
      - uses: ./tools/ci/actions/openssl
      - uses: ./tools/ci/actions/node
      - name: Valgrind and clang
        run: sudo apt-get install --yes valgrind clang
      - name: The lane constant-time, natively
        shell: bash
        run: |
          entry="$(python3 tools/ci/matrix.py own-lanes --library crypto \
            | python3 -c 'import json,sys; print(json.dumps(next(e for e in json.load(sys.stdin)["include"] if e["lane"] == "constant-time" and e.get("platform") == "native")))')"
          python3 tools/ci/matrix.py own-lane "$entry" --out-dir out
```

  The checkout action is pinned as `library.yml` pins it (`3d3c42e5...`, v7.0.1, read on
  2026-10-11; if `library.yml` has moved it when this task runs, the same SHA as there). The
  WebAssembly runs of the throughput on x86-64 are the CI's own lanes' (the own lane
  `throughput` on wasip2, wasip3 and emscripten runs in the superproject's own-lanes job).
- [ ] **Step 6: Run** `actionlint libs/crypto/.github/workflows/runners.yml` (the CI's
  actionlint job's command) and the lanes `sizes` and `throughput` natively. Expected:
  clean; PASS.
- [ ] **Step 7: Commit** (library) `"crypto: sizes per target and throughput per runner,
  the documentation's table checked where the toolchains are pinned"`; (superproject)
  `"tools: webcpp.target-of for a library's own actions"`.

## Task 23: The browser: WebCrypto as an outside oracle, the audio worklet, throughput in Chrome

Deferred after v0.1 (revision 2026-11-14). Kept as written for its own change; before it runs,
re-read it against the tree of that day: OpenSSL is source-built (`/webcpp/crypto//openssl`)
and refused on Emscripten, there is no `webcpp-own`, and the CI has no levels yet, so the lane
`browser` runs on every push; `CHROME_LANES` in `tools/ci/matrix.py` and the chrome action's
`product` choice in `library.yml` have changed since the plan was written.

**Files:**
- Create: `test/browser/Jamfile`, `test/browser/package.json`,
  `test/browser/package-lock.json`, `test/browser/run.mjs`, `test/browser/page.html`,
  `test/browser/webcrypto.cpp`, `test/browser/worklet.cpp`.
- Modify: `test/Jamfile` (`build-project browser ;`); (superproject) `tools/ci/matrix.py`
  (`CHROME_LANES` gains `('crypto', 'browser')`), `tools/ci/matrix_test.py`.

**Interfaces:**
- Consumes: the driven-test rule `webcpp.drive` and its Chrome (the CI's `chrome` action
  gives `CHROME`), trystero's `findChrome` (`libs/trystero/test/oracle/interop/run.mjs`),
  copied with its comment.
- Produces: the own lane `browser` on emscripten.

- [ ] **Step 1: Write the program** `webcrypto.cpp`, which reads a file of cases, one per
  line, `<key> <iv> <additional data> <plaintext> <ciphertext and tag>` in hex (`-` for an
  empty field), seals each with `aes128_gcm` or `aes256_gcm` on BearSSL by the key's size,
  compares, opens the expected form, and prints `webcrypto agrees: <n> cases` or each case
  that differs:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// WebCrypto as an outside oracle (spec section 8.1): each case was sealed by node's or
// Chrome's crypto.subtle (AES-GCM, 128-bit tag); webcpp::crypto on BearSSL must give the
// same bytes, ciphertext then tag as WebCrypto appends it, and open them.

#include <webcpp/crypto/aes256_gcm.hpp>
#include <webcpp/crypto/equal.hpp>

#include "../support/hex.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> field(std::string const& hex) {
    return hex == "-" ? std::vector<std::uint8_t>() : webcpp::test::from_hex(hex);
}

template <class Aead>
bool agrees(std::vector<std::uint8_t> const& key, std::vector<std::uint8_t> const& iv,
            std::vector<std::uint8_t> const& additional_data, std::vector<std::uint8_t> data,
            std::vector<std::uint8_t> const& sealed) {
    auto made = Aead::make(key);
    if (!made || sealed.size() != data.size() + 16) {
        return false;
    }
    auto const tag = made->seal(iv, additional_data, data);
    std::vector<std::uint8_t> ours(data);
    if (tag) {
        ours.insert(ours.end(), tag->begin(), tag->end());
    }
    std::vector<std::uint8_t> theirs(sealed.begin(), sealed.end() - 16);
    return tag && ours == sealed &&
           made->open(iv, additional_data, theirs,
                      std::span<std::uint8_t const>(sealed).last(16)).has_value();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: webcrypto <cases>\n");
        return 2;
    }
    std::ifstream in(argv[1]);
    std::string line;
    std::size_t cases = 0;
    std::size_t differ = 0;
    while (std::getline(in, line)) {
        std::istringstream words(line);
        std::string key, iv, additional_data, plaintext, sealed;
        if (!(words >> key >> iv >> additional_data >> plaintext >> sealed)) {
            continue;
        }
        auto const k = field(key);
        bool const ok =
            k.size() == 16
                ? agrees<webcpp::crypto::aes128_gcm>(k, field(iv), field(additional_data),
                                                     field(plaintext), field(sealed))
                : agrees<webcpp::crypto::aes256_gcm>(k, field(iv), field(additional_data),
                                                     field(plaintext), field(sealed));
        ++cases;
        if (!ok) {
            ++differ;
            std::printf("differs: %s\n", line.c_str());
        }
    }
    std::printf("webcrypto agrees: %zu cases\n", cases - differ);
    return differ == 0 && cases > 0 ? 0 : 1;
}
```

- [ ] **Step 2: Write the driver** `run.mjs`, run by `webcpp.drive` as
  `node run.mjs --node|--chrome <program.js>`:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Runs crypto's browser tests (spec sections 8.1, 8.4, 8.8). --node <program.js>: node's
// WebCrypto seals the cases, and the program, under node, must agree. --chrome <program.js>:
// a page of Chrome's loads the program built for the web, Chrome's WebCrypto seals the
// cases there, the program runs in the page, and the page posts its output here; --page
// <name> picks what the page runs (webcrypto, throughput or worklet). Chrome is CHROME,
// --chrome-path, or, on macOS, the newest chrome-headless-shell in Playwright's cache.
import {spawn, spawnSync} from 'node:child_process'
import {existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync} from 'node:fs'
import {createServer} from 'node:http'
import {homedir, tmpdir} from 'node:os'
import {basename, dirname, join, resolve} from 'node:path'
import {fileURLToPath} from 'node:url'

const here = dirname(fileURLToPath(import.meta.url))
const args = process.argv.slice(2)
const option = name => (args.includes(name) ? args[args.indexOf(name) + 1] : undefined)
const program = resolve(args[args.length - 1])

// A small seeded generator (mulberry32): the same cases on every run and every machine.
const generator = seed => () => {
    seed = (seed + 0x6d2b79f5) | 0
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed)
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296
}
const random = generator(20261011)
const bytes = n => Uint8Array.from({length: n}, () => Math.floor(random() * 256))
const hex = b => (b.length === 0 ? '-' : Buffer.from(b).toString('hex'))

// The GCM specification's cases with 16- and 32-byte keys (cases 1 to 6 and 13 to 18), and
// 1,000 random messages under 16-byte IVs, the ones trystero exchanges with Trystero.
const specCases = JSON.parse(readFileSync(join(here, 'gcm_spec_cases.json'), 'utf8'))
const inputs = [
    ...specCases.filter(c => c.key.length !== 48).map(c => ({
        key: Buffer.from(c.key, 'hex'), iv: Buffer.from(c.iv, 'hex'),
        aad: Buffer.from(c.additional_data, 'hex'), plaintext: Buffer.from(c.plaintext, 'hex')})),
    ...Array.from({length: 1000}, (_, i) => ({
        key: bytes(i % 2 === 0 ? 16 : 32), iv: bytes(16),
        aad: bytes(Math.floor(random() * 33)), plaintext: bytes(Math.floor(random() * 301))})),
]

const seal = async (subtle, c) => {
    const key = await subtle.importKey('raw', c.key, 'AES-GCM', false, ['encrypt'])
    const parameters = {name: 'AES-GCM', iv: c.iv, tagLength: 128}
    if (c.aad.length > 0) parameters.additionalData = c.aad
    return new Uint8Array(await subtle.encrypt(parameters, key, c.plaintext))
}

const line = (c, sealed) => [hex(c.key), hex(c.iv), hex(c.aad), hex(c.plaintext), hex(sealed)].join(' ')

if (args.includes('--node')) {
    const lines = []
    for (const c of inputs) lines.push(line(c, await seal(globalThis.crypto.subtle, c)))
    const directory = mkdtempSync(join(tmpdir(), 'webcpp-crypto-'))
    const cases = join(directory, 'cases.txt')
    writeFileSync(cases, lines.join('\n') + '\n')
    const run = spawnSync('node', [program, cases], {stdio: 'inherit'})
    rmSync(directory, {recursive: true, force: true})
    process.exit(run.status ?? 1)
}
```

  The Chrome half, in the same file, after the node half:

```js
// Chrome as trystero's interop driver finds it (libs/trystero/test/oracle/interop/run.mjs).
const findChrome = () => {
    const explicit = option('--chrome-path') ?? process.env.CHROME
    if (explicit) return explicit
    if (process.platform !== 'darwin') {
        throw new Error('CHROME is not set: off macOS, the browser lane runs the Chrome that CHROME or --chrome-path names')
    }
    const cache = join(homedir(), 'Library/Caches/ms-playwright')
    const shells = existsSync(cache) ? readdirSync(cache).filter(d => d.startsWith('chromium_headless_shell-')).sort() : []
    for (const dir of shells.reverse()) {
        for (const sub of ['chrome-headless-shell-mac-arm64', 'chrome-headless-shell-mac-x64']) {
            const path = join(cache, dir, sub, 'chrome-headless-shell')
            if (existsSync(path)) return path
        }
    }
    throw new Error(`no Chrome found: set CHROME, pass --chrome-path, or install Playwright's chrome-headless-shell in ${cache}`)
}

if (args.includes('--chrome')) {
    const page = option('--page')
    const expected = {webcrypto: `webcrypto agrees: ${inputs.length} cases`, throughput: 'done',
        worklet: 'not_supported'}[page]
    const types = {'.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm'}
    // An audio worklet needs shared memory, which needs a cross-origin isolated page.
    const isolation = page === 'worklet'
        ? {'Cross-Origin-Opener-Policy': 'same-origin', 'Cross-Origin-Embedder-Policy': 'require-corp'} : {}
    const served = {'/page.html': join(here, 'page.html'), [`/${basename(program)}`]: program,
        [`/${basename(program, '.js')}.wasm`]: program.replace(/\.js$/, '.wasm')}
    let finish
    const finished = new Promise(r => { finish = r })
    const server = createServer((request, response) => {
        const url = new URL(request.url, 'http://127.0.0.1')
        if (request.method === 'POST' && url.pathname === '/result') {
            let body = ''
            request.on('data', chunk => { body += chunk })
            request.on('end', () => { response.end(); finish(body) })
            return
        }
        if (url.pathname === '/inputs.json') {
            response.writeHead(200, {'Content-Type': 'application/json', ...isolation})
            response.end(JSON.stringify(inputs.map(c => ({key: hex(c.key), iv: hex(c.iv), aad: hex(c.aad),
                plaintext: hex(c.plaintext)}))))
            return
        }
        const file = served[url.pathname]
        if (!file || !existsSync(file)) {
            response.writeHead(404)
            response.end()
            return
        }
        response.writeHead(200, {'Content-Type': types[extname(file)] ?? 'application/octet-stream', ...isolation})
        response.end(readFileSync(file))
    })
    await new Promise(r => server.listen(0, '127.0.0.1', r))
    const profile = mkdtempSync(join(tmpdir(), 'webcpp-crypto-chrome-'))
    const address = `http://127.0.0.1:${server.address().port}/page.html?page=${page}&program=${basename(program)}`
    const chrome = spawn(findChrome(), [
        '--headless=new', `--user-data-dir=${profile}`, '--no-first-run', '--no-default-browser-check',
        '--disable-component-update', '--disable-background-networking', '--disable-sync',
        '--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1',
        '--autoplay-policy=no-user-gesture-required',
        ...(process.getuid?.() === 0 ? ['--no-sandbox'] : []), address], {stdio: 'ignore'})
    const text = await Promise.race([finished, new Promise(r => setTimeout(() => r(null), 120_000))])
    if (chrome.exitCode === null && chrome.signalCode === null) {
        chrome.kill()
        await new Promise(r => chrome.once('exit', r))
    }
    server.close()
    rmSync(profile, {recursive: true, force: true})
    if (text === null) {
        console.error('the page posted nothing within 120 s')
        process.exit(1)
    }
    process.stdout.write(text + '\n')
    process.exit(text.trim().split('\n').pop() === expected ? 0 : 1)
}
```

  with `extname` added to the `node:path` import. `inputs.length` is 1,012: the 12
  specification cases whose keys are of 16 or 32 bytes (WebCrypto refuses 24, spec 2.4) and
  the 1,000 random ones. `gcm_spec_cases.json` is the 18 cases of
  `test/support/gcm_spec_vectors.hpp` as JSON
  (`[{"key": ..., "plaintext": ..., "additional_data": ..., "iv": ..., "ciphertext": ..., "tag": ...}]`),
  written once from the header with a one-off script and committed beside the driver.
- [ ] **Step 3: Write `page.html`:**

```html
<!-- Copyright (c) 2026 WebCpp.org

     Distributed under the Boost Software License, Version 1.0. (See
     accompanying file LICENSE_1_0.txt or copy at
     https://www.boost.org/LICENSE_1_0.txt) -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>webcpp::crypto in Chrome</title></head>
<body>
<script>
// Runs one of crypto's browser programs and posts what it printed to the driver (run.mjs).
const query = new URLSearchParams(location.search)
const page = query.get('page')
const out = []
const post = text => fetch('/result', {method: 'POST', body: text})
const bytes = h => (h === '-' ? new Uint8Array() : Uint8Array.from(h.match(/../g), b => parseInt(b, 16)))
const hex = b => (b.length === 0 ? '-' : Array.from(b, x => x.toString(16).padStart(2, '0')).join(''))
window.onerror = message => { post(`error: ${message}`) }

// Chrome's WebCrypto seals each case; the program must give the same bytes.
const webcrypto = async () => {
    const inputs = await (await fetch('/inputs.json')).json()
    const lines = []
    for (const c of inputs) {
        const key = await crypto.subtle.importKey('raw', bytes(c.key), 'AES-GCM', false, ['encrypt'])
        const parameters = {name: 'AES-GCM', iv: bytes(c.iv), tagLength: 128}
        if (c.aad !== '-') parameters.additionalData = bytes(c.aad)
        const sealed = new Uint8Array(await crypto.subtle.encrypt(parameters, key, bytes(c.plaintext)))
        lines.push([c.key, c.iv, c.aad, c.plaintext, hex(sealed)].join(' '))
    }
    Module.FS.writeFile('/cases.txt', lines.join('\n') + '\n')
    Module.callMain(['/cases.txt'])
}

// Chrome's WebCrypto timed beside the program (spec 8.8, "in Chrome against WebCrypto"):
// AES-256-GCM seal and open of 100 B and 1 MiB, three runs each, in the program's line form.
const webcryptoThroughput = async () => {
    const key = await crypto.subtle.importKey('raw', new Uint8Array(32), 'AES-GCM', false,
        ['encrypt', 'decrypt'])
    const iv = new Uint8Array(12)
    for (const size of [100, 1 << 20]) {
        const data = new Uint8Array(size)
        const rounds = size >= (1 << 20) ? 64 : 20000
        for (let run = 0; run < 3; ++run) {
            let sealed = null
            const start = performance.now()
            for (let i = 0; i < rounds; ++i) sealed = await crypto.subtle.encrypt({name: 'AES-GCM', iv}, key, data)
            const middle = performance.now()
            for (let i = 0; i < rounds; ++i) await crypto.subtle.decrypt({name: 'AES-GCM', iv}, key, sealed)
            const end = performance.now()
            const mib = size * rounds / (1 << 20)
            out.push(`webcrypto seal ${size} B ${(mib / ((middle - start) / 1000)).toFixed(1)} MiB/s`)
            out.push(`webcrypto open ${size} B ${(mib / ((end - middle) / 1000)).toFixed(1)} MiB/s`)
        }
    }
}

// The worklet's answer arrives on the main thread later: wait for it, at most 10 s.
const worklet = async () => {
    Module.callMain([])
    for (let i = 0; i < 100 && !out.some(l => l === 'not_supported' || l.startsWith('error')); ++i) {
        await new Promise(r => setTimeout(r, 100))
    }
}

var Module = {
    noInitialRun: true,
    print: line => out.push(line),
    printErr: line => out.push(line),
    onRuntimeInitialized: async () => {
        try {
            if (page === 'webcrypto') await webcrypto()
            if (page === 'throughput') { Module.callMain([]); await webcryptoThroughput(); out.push('done') }
            if (page === 'worklet') await worklet()
        } catch (e) {
            out.push(`error: ${e}`)
        }
        post(out.join('\n'))
    },
}
const script = document.createElement('script')
script.src = query.get('program')
document.body.appendChild(script)
</script>
</body>
</html>
```

  The web programs are linked with
  `-sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=FS,callMain -sFORCE_FILESYSTEM=1 -sINVOKE_RUN=0`.
- [ ] **Step 4: Measure the audio worklet first** (spec 8.4: "in an Emscripten audio
  worklet, fill returns errc::not_supported"). `worklet.cpp`, Emscripten 6.0.11's Web Audio
  API (`emscripten/webaudio.h`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// In an audio worklet, which has no crypto, random_device's fill gives
// errc::not_supported (spec sections 6 and 8.4): the first call of the processor fills 16
// bytes and posts the answer to the main thread, which prints it for the page.

#include <webcpp/crypto/random_device.hpp>

#include <boost/system/errc.hpp>

#include <emscripten/webaudio.h>

#include <array>
#include <cstdint>
#include <cstdio>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the worklet's stack
alignas(16) std::uint8_t worklet_stack[16384];
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): set once, by the worklet
bool reported = false;

void report(int code) {
    std::printf("%s\n", code == 0 ? "not_supported" : "error: fill did not give not_supported");
}

bool process(int, AudioSampleFrame const*, int, AudioSampleFrame*, int, AudioParamFrame const*,
             void*) {
    if (!reported) {
        reported = true;
        std::array<std::uint8_t, 16> bytes{};
        auto const filled = webcpp::crypto::random_device().fill(bytes);
        bool const expected =
            !filled && filled.error() == boost::system::errc::not_supported;
        emscripten_audio_worklet_post_function_vi(EMSCRIPTEN_AUDIO_MAIN_THREAD, &report,
                                                  expected ? 0 : 1);
    }
    return true;
}

void processor_created(EMSCRIPTEN_WEBAUDIO_T context, bool success, void*) {
    if (!success) {
        std::printf("error: the worklet processor was not created\n");
        return;
    }
    EMSCRIPTEN_WEBAUDIO_T const node =
        emscripten_create_wasm_audio_worklet_node(context, "fill", nullptr, &process, nullptr);
    emscripten_audio_node_connect(node, context, 0, 0);
    emscripten_resume_audio_context_sync(context);
}

void thread_started(EMSCRIPTEN_WEBAUDIO_T context, bool success, void*) {
    if (!success) {
        std::printf("error: the worklet thread did not start\n");
        return;
    }
    WebAudioWorkletProcessorCreateOptions const options{"fill", 0, nullptr};
    emscripten_create_wasm_audio_worklet_processor_async(context, &options, &processor_created,
                                                         nullptr);
}

}  // namespace

int main() {
    EMSCRIPTEN_WEBAUDIO_T const context = emscripten_create_audio_context(nullptr);
    emscripten_start_wasm_audio_worklet_thread_async(context, worklet_stack, sizeof worklet_stack,
                                                     &thread_started, nullptr);
    return 0;
}
```

  It needs `-sAUDIO_WORKLET=1 -sWASM_WORKERS=1`, which need shared memory, while webcpp's
  Emscripten target is single-threaded (`<threading>single`, AGENTS chapter 9). Build it with
  `<threading>multi` in its own requirements, as the drive `worklet_chrome` (`run.mjs --chrome --page worklet`):
  - if it links and runs, it joins the lane `browser`, served with the cross-origin headers,
    and the page posts `not_supported`;
  - if it does not link (BearSSL's archive and the C library built without atomics refuse a
    shared memory: `wasm-ld: error: --shared-memory is disallowed by ... because it was not
    compiled with 'atomics' or 'bulk-memory' features`), the worklet's path of `read_entropy`
    is the `ENOTSUP` mapping that Task 4's planted source pins, the program is not added, and
    the owner is told that the worklet check needs a threaded variant of the Emscripten
    target, with the linker's message.
- [ ] **Step 5: Write `test/browser/Jamfile`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The browser's tests (spec sections 8.1, 8.4, 8.8): node's and Chrome's WebCrypto
# as oracles of AES-GCM on BearSSL, and the throughput in Chrome. Own lane
# browser, on emscripten.
project : requirements <library>/webcpp/crypto//crypto <library>/third_party//bearssl ;

import webcpp ;

webcpp.original node ;

.web = <linkflags>-sENVIRONMENT=web <linkflags>-sEXPORTED_RUNTIME_METHODS=FS,callMain
    <linkflags>-sFORCE_FILESYSTEM=1 <linkflags>-sINVOKE_RUN=0 ;

# Under node the program reads the host's temporary file of cases: webcpp.drive
# links no -sNODERAWFS=1 (tools/webcpp.jam gives it to run, run-fail and
# example alone), so this program asks for it (I12).
webcpp.drive webcrypto_node : webcrypto.cpp : <toolset>emscripten:<linkflags>-sNODERAWFS=1
  : run.mjs : --node : emscripten ;
webcpp.drive webcrypto_chrome : webcrypto.cpp : $(.web) <webcpp-drive-timeout>180
  : run.mjs : --chrome --page webcrypto : emscripten ;
webcpp.drive throughput_chrome : ../throughput/throughput.cpp
  : $(.web) <variant>release <webcpp-drive-timeout>300
  : run.mjs : --chrome --page throughput : emscripten ;

webcpp.lane browser : webcrypto_node webcrypto_chrome throughput_chrome : emscripten ;
```

  (`worklet` joins it if Step 4 links.) In `tools/ci/matrix.py`,
  `CHROME_LANES = {('trystero', 'interop'), ('crypto', 'browser')}`, and `matrix_test.py`'s
  case for `CHROME_LANES` gains crypto's entry.
- [ ] **Step 6: Run** the lane on emscripten with `CHROME` naming the CI's pinned
  chrome-headless-shell (locally, Playwright's, found by the driver on macOS). Expected:
  PASS: `webcrypto agrees: 1012 cases` under node and in Chrome; the throughput's lines and
  `done`.
- [ ] **Step 7: Commit** (library) `"crypto: node's and Chrome's WebCrypto as oracles of
  AES-GCM, and the throughput in Chrome"`; (superproject) `"tools/ci: crypto's browser lane
  meets Chrome"`.

## Task 24: The documentation: the page, the user's Jamfiles, the examples

**Files:**
- Create, fixtures of Task 10's test under `tools/test/fixtures/crypto_jamfiles/` (each a
  `Jamfile`, its sources and an `.outcome`): `web/`, `server/`, `both_bearssl_default/`.
- Create: `doc/jamfile.adoc`, `doc/aead.adoc`, `doc/block.adoc`, `doc/equal.adoc`,
  `doc/entropy.adoc`, `doc/backends.adoc`, `doc/memory.adoc`, `doc/errors.adoc`;
  `example/aes256_gcm_bearssl.cpp` and `.expected`, `example/equal_hmac.cpp` and `.expected`,
  `example/bearssl_prng.cpp` and `.expected` (m11).
- Modify: `doc/crypto.adoc`, `doc/overview.adoc`, `doc/testing.adoc`, `doc/changelog.adoc`,
  `example/Jamfile`, `test/Jamfile` (the fixtures' sources compiled, m21), `README.md`;
  (superproject) `tools/test/crypto_jamfiles_test.py` (the outcome line `same-as:`),
  `doc/releases/0.1.0.adoc` (crypto under "New Libraries").

The page follows AGENTS.md chapter 3: one page, `doc/crypto.adoc`, its sections in files
beside it, built by `doc/Jamfile` (`webcpp.doc crypto : crypto.adoc ;` and
`webcpp.reference crypto ;`), with the page's attribute header, anchors that start with
`crypto-`, a testing chapter, the reference under `[#reference]` and the changelog last;
the index lists it from `meta/libraries.json`, and the site's catalogue is generated from the
same file, so `site/` needs no change.

- [ ] **Step 1: Write the user's Jamfiles of spec section 7**, fixtures of
  `tools/test/crypto_jamfiles_test.py` (Task 10), each built and run on the targets the
  section names (its outcome's `targets:` line), so that every Jamfile line and every printed
  result of the page was built and run. Each fixture's Jamfile marks the lines the page shows
  between `# tag::jamfile[]` and `# end::jamfile[]`, after the licence notice:

| Case | Jamfile (the tagged lines) | Source | Targets | Outcome |
| --- | --- | --- | --- | --- |
| `web` | `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;` | spec 7.2's `app.cpp` | all four | `build: succeeds`, `run: app`, `aes256_gcm ok backend=bearssl` |
| `server` | `exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl ;` | a copy of `web/app.cpp` | native wasip2 wasip3 | `same-as: web/app.cpp app.cpp`, `build: succeeds`, `run: app`, `aes256_gcm ok backend=openssl` |
| `both_bearssl_default` | `exe app : app.cpp /webcpp/crypto//crypto /webcpp/crypto//openssl /third_party//bearssl : <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL ;` | the same copy | native wasip2 wasip3 | `build: succeeds`, `run: app`, `aes256_gcm ok backend=bearssl` |

  OpenSSL is static and built from source by `/webcpp/third_party//openssl`, which
  `/webcpp/crypto//openssl` names; there is no other link of it to show. Its refusal on
  Emscripten is Task 10's fixture, which the page shows with the other messages.
  Copies, not symbolic links: a checkout on Windows reads a link as a text file
  (`core.symlinks` off). Task 10's test gains the outcome line
  `same-as: <case>/<file> <file>`, two files that must be byte-identical, with a case of its
  self-check, so the copies cannot drift.
- [ ] **Step 2: Write the examples** the ordinary lanes run and clang-tidy reads:
  `example/aes256_gcm_bearssl.cpp`, spec 7.2's program with `gcm<aes256<backends::bearssl>>`
  named, expected `aes256_gcm ok backend=bearssl`; `example/equal_hmac.cpp`, an HMAC-SHA-256
  of Boost.Hash2 checked with `equal` against RFC 4231's test case 2 (key `Jefe`, data
  `what do ya want for nothing?`, tag
  `5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843`), expected
  `hmac verified with equal`. In `example/Jamfile`:
  `webcpp.example aes256_gcm_bearssl.cpp : <library>/third_party//bearssl ;`,
  `webcpp.example equal_hmac.cpp ;` and
  `webcpp.example bearssl_prng.cpp : <library>/third_party//bearssl ;`, the last the program of
  `entropy.adoc` that seeds BearSSL's own PRNG (`br_hmac_drbg_init` with SHA-256 and 32 bytes
  of `random_device::fill`) and prints `seeded` (m11). The user's programs of the fixtures are
  compiled once each in the ordinary lanes, so that clang-tidy reads the documentation's code
  (m21): in `test/Jamfile`,
  `webcpp.compile doc_web_app : ../../../tools/test/fixtures/crypto_jamfiles/web/app.cpp : <library>/third_party//bearssl ;`
  and one line per other fixture's program with its libraries (`server` and
  `both_bearssl_default` on native, wasip2 and wasip3), the failing cases (`canary_*` and the
  refusals) excepted.
- [ ] **Step 3: Write the page's sections**, each a file of `doc/` included by
  `crypto.adoc` in this order, each holding what the spec section named says, rewritten for a
  user, with what it shows included from a tracked file, never typed: C++ as
  `include::{examples}/<file>[tag=...]` or a tagged region of `test/` or `example/`, outputs
  from the examples' `.expected` files, and a Jamfile's lines as a `[listing]` block of
  `include::{webcpp-root}/tools/test/fixtures/crypto_jamfiles/<case>/Jamfile[tag=jamfile]`
  (doc-check refuses a whole file and lines chosen by number). A printed result of a fixture
  is shown only from a tagged region of a tracked file, its `.outcome` or an example's
  `.expected`, as `tools/doc/doc-check.py` accepts it (read its rule for outputs first; when
  it refuses an outcome, the page shows the example's `.expected` of the same program):
  - `overview.adoc` "Why crypto": spec 1.1, 1.2; what stays with Boost.Hash2.
  - `jamfile.adoc` "Using webcpp::crypto in your Jamfile", written so that an AI agent
    writes a correct Jamfile at once (DESIGN, 2026-10-11 "C libraries built from their own
    sources, and the backend named in the user's Jamfile"): first the rule in three lines
    (spec 7.1: name `/webcpp/crypto//crypto` and the backend, `/third_party//bearssl` or
    `/webcpp/crypto//openssl`; which backend for which target, OpenSSL on native, wasip2 and
    wasip3 and never on Emscripten; a backend named and not called costs nothing); then each
    case of Step 1 with its Jamfile, its source, its command and what it prints (7.2, 7.3,
    the choice of default); the backends' table (7.6); every message and its fix (7.7), with
    the link-time messages as Task 18 measured them on each linker, and the note that on
    MSVC each message is proven to stop the build and its text is MSVC's own; and, for
    Windows, spec 2.2's note: crypto's headers include `<windows.h>` under `NOMINMAX` and
    `WIN32_LEAN_AND_MEAN` when the program has not defined them, so a program that wants the
    macros `min` and `max`, or what `WIN32_LEAN_AND_MEAN` leaves out, includes `<windows.h>`
    itself before any header of crypto.
  - `aead.adoc` "AES-GCM": spec 2.4's interface and every decision of its "Decisions on the
    behaviour" as a rule for the user (in place with the tag apart; a failed open zeroes
    `data`; plaintext in `data` while `open` runs, and when to open into private memory; no
    incremental open; IVs of 1 to 128 bytes, 12 recommended; keys of 24 bytes and browsers'
    WebCrypto; the usage limit of 2^32 messages with random IVs; an IV never reused, with
    Joux's attack named; one thread at a time; not shareable), the guards' table, and the
    warning of spec 5 on ARM natively without OpenSSL.
  - `block.adoc` "The block cipher": spec 2.3; not encryption by itself; `aes256<backends::ctaes>`
    on WebAssembly (written now with the backends that exist, ctaes added by Task 29).
  - `equal.adoc` "Comparing tags": spec 2.6, with `example/equal_hmac.cpp`.
  - `entropy.adoc` "Entropy": spec 6 and 2.7: the one source per platform with its documented
    contract; no fallback; the errors; `operator()` with and without exceptions; seeding
    BearSSL's own PRNG from `random_device` (a code block built as `example/` is: a third
    example, `bearssl_prng.cpp`, which seeds `br_hmac_drbg_context` from 32 bytes of `fill`
    and prints `seeded`).
  - `backends.adoc` "The backends": spec 3.1's table, OpenSSL on native, wasip2 and wasip3;
    per backend its Jamfile line, its header, what it exports, its own API reachable, its
    allocation (BearSSL none; OpenSSL's `make` through OpenSSL's functions, the counts Task 17
    printed for OpenSSL 3.5.9), constant time (BearSSL's `bmul64` on a 32-bit WebAssembly
    host), its licence; the default per target (spec 5) with the reasons measured: BearSSL on
    WebAssembly, natively OpenSSL when linked, else BearSSL. The source-built OpenSSL is
    compiled `no-asm` on MSVC, wasip2 and wasip3: T-table AES and 4-bit GHASH, no AES-NI, not
    constant time unless OpenSSL is built with `OPENSSL_AES_CONST_TIME`; a program that wants
    BearSSL there defines `WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL` or names `aes256_gcm` with
    `backends::bearssl`.
  - `memory.adoc` "Allocation": spec 2.11; what each backend allocates and through what; no
    type is shareable, and why.
  - `errors.adoc` "Errors and exceptions": the table of spec 2.9, the messages, spec 2.10.
  - `testing.adoc`: the lanes and own lanes (`c-suites`, on every push), the MSVC message
    cases, `{n-tests}`, the vectors and their counts, the report's address
    (https://webcpporg.github.io/report/).
  The page names no algorithm, backend or option that v0.1 does not build: ChaCha20-Poly1305,
  Botan, OpenSSL's memory option, sizes and throughput are not mentioned.
  `doc/changelog.adoc` gains under "Unreleased" the version's one documentation line,
  `* Added the page: every Jamfile a user writes, and every message with its fix.`
  (AGENTS.md chapter 5), and `doc/releases/0.1.0.adoc` lists crypto under "New Libraries":
  `* link:{library-pages}crypto/{library-page}[crypto]: AES-GCM, AES, random_device, equal and
  secp256k1 over C libraries built from source, from R.`
- [ ] **Step 4: Run** `$B2 -a --build-dir=bin/b-doc libs/crypto/doc`, `$B2 -a doc`,
  `python3 tools/test/crypto_jamfiles_test.py web server both_bearssl_default`,
  and the examples natively, on wasip2 and wasip3 (each with its launcher,
  `tools/boost/asio-wasip2/run.sh` and `tools/boost/asio-wasip3/run.sh`) and on emscripten.
  Expected: PASS; doc-check finds nothing; the index lists crypto, linked to its page; the
  page shows every Jamfile and program from the tree.
- [ ] **Step 5: Check the text:** `grep -rn $'\xe2\x80\x94' libs/crypto` prints nothing;
  `git -C libs/crypto grep -I -i -e 'veru[s]'` prints nothing; the lint's `changelog form`
  passes.
- [ ] **Step 6: Commit:** `"crypto: the page: every Jamfile a user writes, built and run on
  every target, and every message with its fix"`; (superproject) the fixtures and the release
  notes' line, `"doc: crypto among the new libraries of 0.1.0"`.

## Task 25: Increment 1 whole: the rulebook, green, review and the pull requests

**Files (superproject):**
- Modify: `CODING-GUIDELINES.md` (chapters 5, 9, 10, 15, 16, 17 and 19, below), `AGENTS.md`
  (chapter 1's table of libraries and layout; chapter 4's lanes), `README.md` (a row for
  crypto).

Task 25a is obsolete (below); the nightly level, Task 25b, is deferred after v0.1: every own
lane, `c-suites` included, runs on every push.

- [ ] **Step 1: Write the rulebook** (CODING-GUIDELINES.md chapter 1: a change that builds a
  proposal moves its decisions from chapter 17 into chapter 15, and one that resolves an entry
  of chapter 19 removes it):
  - CODING-GUIDELINES.md chapter 15 gains `### crypto (authenticated encryption, randomness
    and secp256k1 over C backends; own; implemented in 0.1.0)`, the decisions of chapter 17's
    crypto section that v0.1 builds, as built: the backends BearSSL, OpenSSL (the
    source-built provider through `/webcpp/crypto//openssl`, native, wasip2 and wasip3,
    refused on Emscripten), libsecp256k1 and ctaes; the default backend; the three layers,
    with no OpenSSL canary; `webcpp.same-program`; the fixtures of
    `tools/test/crypto_jamfiles_test.py`; the own lane `c-suites` on every push. Chapter 17's
    crypto section keeps only the later work, with its source: OpenSSL's memory option,
    constant time, sizes and throughput, the browser lane, the CI's nightly level, NIST
    CAVP, ChaCha20-Poly1305, Botan, and what of trystero's migration is not yet built; its
    paragraph "Provider amendment, 2026-11-03" and every statement of a found or shared
    OpenSSL leave it.
  - Chapter 5: rule 5 names crypto as the library that follows it, with both project ids,
    `/third_party//<name>` the user's.
  - Chapter 9: the row `canary pins` (Task 18).
  - Chapter 10, "Third-party code": a row per submodule increment 1 added, with its pin and
    licence (`bearssl`, `wycheproof`), if the task that added it has not.
  - Chapter 16, the roadmap: crypto's later work, as chapter 17 lists it.
  - Chapter 19: remove the entry of "The text of the rules" that sets chapter 10's
    `/webcpp/third_party` against chapter 5's `/third_party//bearssl` (both ids exist since
    Task 1), and the words calling crypto's specification "in review". Add, under "The
    libraries against the rules", crypto's limits against chapter 5's rules 11 and 12:
    - MSVC's messages: each case fails on the `msvc-14.3` and `msvc-14.5` lanes on every push
      (`webcpp.compile-fail`, `webcpp.link-fail`, Task 18), and its exact text (`C1189`,
      `C2338`, `LNK2001`, `LNK2038`) is checked on no CI; the text is checked on clang, GNU ld
      and wasm-ld by the tools job, and ld64's on a developer's macOS, recorded in the ledger;
      the tools job has no macOS or Windows leg;
    - crypto has no size table until its sizes lane (plan Task 22), so a pin's move (rule 12)
      updates the submodule, the canary's commit and the tests on every target, and no table;
    - the source-built OpenSSL is compiled `no-asm` on MSVC, wasip2 and wasip3: T-table AES
      and 4-bit GHASH, no AES-NI, not constant time unless `OPENSSL_AES_CONST_TIME`; crypto's
      default backend there is still OpenSSL when it is linked, and the page says how to
      choose BearSSL;
    - `unused_openssl`, only if Task 21 measured the pair different on a lane: the lane, the
      bytes and the flag that adds them.
  - AGENTS.md chapter 1's table of libraries:
    `| crypto | AES-GCM, the AES block cipher, random_device, a constant-time equal and secp256k1 over C libraries built from source (BearSSL, OpenSSL, libsecp256k1, ctaes), webcpp's own | native, wasip2, wasip3, emscripten (OpenSSL not on emscripten); its own lane c-suites | a submodule at libs/crypto |`;
    the layout names `third_party/libs/`'s new submodules; chapter 4's lanes name crypto's
    own lane and its MSVC message cases.
  - README.md's table gains
    `| [crypto](https://github.com/webcpporg/crypto) | <meta/libraries.json's description> | native, wasip2, wasip3, emscripten |`.
  - `doc/releases/0.1.0.adoc` already holds crypto's line (Task 24) and the tools' lines
    (Tasks 18 and 21).
  Commit: `"AGENTS.md, CODING-GUIDELINES.md and README.md: crypto, built"`. Then
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- [ ] **Step 2: Run the definition of green** (CODING-GUIDELINES.md chapter 1, "Green, before
  any merge"), each from scratch, recording each result in the ledger:
  - the ordinary lanes:
    `b2 -a libs/crypto/test libs/crypto/example` natively (Apple Clang, and gcc-15 with
    `toolset=gcc-15`);
    `b2 -a toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh libs/crypto/test libs/crypto/example`;
    `b2 -a toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh libs/crypto/test libs/crypto/example`;
    `b2 -a toolset=emscripten libs/crypto/test libs/crypto/example`;
  - the own lane `c-suites` on each target it names, `b2 -a <the target's toolset and
    launcher> libs/crypto/test/c_suites//c-suites`;
  - `python3 tools/test/crypto_jamfiles_test.py` (every fixture), its `done in` times in the
    ledger and Task 10's 30-minute rule applied (m20);
  - `b2 -a doc` (the index lists crypto, linked to its page);
  - the lint: `lint: clean`;
  - every `tools/**/*_test.py` as the CI runs them, each `python3 <path>`, among them
    `tools/test/jamroot_test.py`, `tools/test/webcpp_jam_test.py`,
    `tools/test/third_party_test.py`, `tools/test/harness_test.py`,
    `tools/test/doc_test.py`, `tools/test/crypto_jamfiles_test.py` and
    `tools/lint/lint_test.py`;
  - `python3 tools/ci/matrix.py plan --library crypto`: the ten lanes, `gcc-14`, `gcc-15`,
    `clang-18`, `clang-22`, `apple-clang`, `msvc-14.3`, `msvc-14.5`, `emscripten`,
    `wasip2` and `wasip3`, each with `"external": false` (every backend is built by b2);
    `python3 tools/ci/matrix.py own-lanes --library crypto`: `c-suites` once per target it
    names, ids `<target>.crypto.c-suites`.
  Expected: every one passes. MSVC and Linux's container lanes are proven by the CI alone
  (Step 5).
- [ ] **Step 3:** Dispatch the whole-increment review (the library and the superproject), on
  the most capable model, with this plan's Review Focus, its Conflicts, and the spec.
- [ ] **Step 4:** Fix what it finds, each with a failing test first, and run Step 2 again.
- [ ] **Step 5: The pull requests**, in this order (CODING-GUIDELINES.md chapter 10, "The
  checklist of a new library"; an agent never creates a repository, never pushes and never
  merges; the owner does each):
  1. **The library (G1).** The owner creates `webcpporg/crypto`, public, and pushes the
     library's branch; its pull request into `main` holds increment 1 and the library's
     `.github/workflows/ci.yml`, which calls `webcpporg/webcpp/.github/workflows/library.yml@main`
     with `library: crypto`. Its CI reads the superproject's `main`, which has no
     `third_party/libs/bearssl` and no `/third_party` yet, so it cannot be green before
     pull request 2 merges; it waits, open.
  2. **The superproject, one pull request (G2):** the submodules `third_party/libs/bearssl`
     and `third_party/libs/wycheproof`, `third_party/Jamfile`'s targets (which compile
     `libs/crypto/src/canary_*.c`, so they cannot land before the library's submodule),
     `/third_party` in the Jamroot, `webcpp.same-program`, `tools/test/crypto_jamfiles_test.py`
     and its fixtures, the lint rule `canary pins`, the rulebook of Step 1, and the submodule
     `libs/crypto` that the owner adds (`git submodule add ../crypto.git libs/crypto`, never
     `git add -A`) at the head of pull request 1. Its CI, which builds every library at the
     commits the pull request records, is green: every lane, the own lanes, docs, lint,
     tools, actionlint, report and the site. The owner merges it.
  3. **The library again.** Pull request 1's CI is run again against the new `main` and is
     green; the owner merges it by fast-forward, so that the commit the superproject records
     is the library's `main`. If it is merged otherwise, a superproject pull request moves
     `libs/crypto` to the library's `main` and is green before step 4.
  4. **The proposals.** A pull request in `webcpporg/proposals` records increment 1 as
     implemented: merged into the library's `main`, recorded by the superproject's `main`,
     CI green on both (the proposals' CONTRIBUTING.md), with the run of each CI.
  The ledger records each pull request, its CI run and its merge commit.

## Task 25a: asio's OpenSSL onto the source-built provider (obsolete)

Obsolete: asio already links the source-built provider. Its `build.jam`, at the commit the
superproject's `main` pins (`libs/asio` at `6c4e2c8`, lines 104-122), gives its alias
`openssl` `<library>/webcpp/third_party//openssl <define>WEBCPP_ASIO_HAS_OPENSSL=1` on
native, wasip2 and wasip3 through `rule openssl-linked`, and refuses Emscripten by a forced
include; CODING-GUIDELINES.md chapter 5, rule 7: "The Asio alias links this dependency
explicitly." A program of asio and crypto therefore holds one libcrypto, the provider's.

## Task 25b: The nightly level: the heavy own lanes, asio's asyncify, the tools' macOS and Windows legs (owner gate O2)

Deferred after v0.1 (revision 2026-11-14). Kept as written for its own change; before it runs,
re-read it against the tree of that day: OpenSSL is source-built (`/webcpp/crypto//openssl`),
there is no `webcpp-own`, the CI has no levels yet, and `matrix.py`, `library.yml` and the
tools job (8 shards, Linux alone) have changed. In v0.1 every own lane runs on every push,
MSVC's messages are checked by Task 18's MSVC cases without their text, and ld64's text on a
developer's macOS; this task's Windows and macOS legs of the tools job would check both texts
and remove those limits from CODING-GUIDELINES.md chapter 19.

Written with the controller's recommendation for O2; if the owner keeps every lane on every
push instead, this task is dropped, `runners.yml` (Task 22) takes back its push trigger, and
the tools job's macOS and Windows legs run on every push.

What the nightly level runs, plainly (N5): `nightly.yml` does **not** rerun any library's
ordinary lanes, docs, lint or Linux tools tests; those stay on every push, as today. Each night
it runs only:
- the own lanes of `NIGHTLY_LANES`: crypto's `c-suites`, `constant-time`, `browser` and
  `throughput`, and asio's `asyncify` (DESIGN 2026-10-10 "webcpp::asio, section 11" puts
  Asyncify at the nightly and release levels; the asio plan's departure 1, approved
  2026-10-11, keeps it on every push only until the CI's levels exist, which this task makes);
- the tools job's `macos-15` and `windows-2022` legs, `crypto_jamfiles_test.py --target native`
  alone, for ld64's and MSVC's messages (O4', the controller's ruling: free minutes on the
  public repository, the cost being concurrency and wall time, so not on every push);
- the report job, which publishes those results (r3).

Where the nightly level runs (r3): only in the superproject, from `nightly.yml` on `main`. A
library repository's CI calls `library.yml@main` with its library and no level, so `push`
(trystero's `ci.yml` today), and never runs a nightly lane; each night the superproject tests
the library commits its `main` pins. The one scheduled workflow of a library is crypto's
`runners.yml` (Task 22, C4 and C16), which is not a lane of the matrix: it measures throughput
and constant time on the runners the matrix has no lane for (`ubuntu-24.04-arm`, `macos-15`),
at 03:17 UTC, on the library's own `main`.

**Files (superproject):**
- Modify: `tools/ci/matrix.py` (`NIGHTLY_LANES`; `own-lanes --level push|nightly`),
  `tools/ci/matrix_test.py`, `.github/workflows/library.yml` (the input `level`, default
  `push`; every job but `plan`, `own-lanes`, the tools job's nightly legs and `report`
  skipped at `nightly`; the report's nightly form), AGENTS.md (chapter 9's CI; chapter 13's levels item, m17), and asio's ledger and
  AGENTS.md (departure 1 closed: `asyncify` is nightly).
- Create: `.github/workflows/nightly.yml`.

- [ ] **Step 1: Write the failing cases** in `tools/ci/matrix_test.py`:
  - `own-lanes --library crypto --level push` lists `sizes` alone among crypto's own lanes,
    and `--level nightly` lists `c-suites`, `constant-time`, `browser` and `throughput`;
  - `own-lanes --library asio --level push` lists `browser` and `tls` and not `asyncify`, and
    `--level nightly` lists `asyncify` alone;
  - every other library's own lanes are unchanged at `push` (trystero's `oracle` and
    `interop`, wasi's `http`, xstate's oracle) and absent at `nightly`;
  - a pair of `NIGHTLY_LANES` whose library declares no such lane is left out of the listing
    silently only when the library is not checked out (asio before its integration), and
    fails the listing by name when the library is there and lacks the lane (a renamed lane);
  - `plan --level nightly` gives an empty matrix of ordinary lanes, and `report` with that
    empty plan, the nightly own lanes and their XML writes a test matrix of those lanes alone,
    titled with the level (`webcpp nightly <date>`), with no column missing for an ordinary
    lane that did not run (r3).
  Run `python3 tools/ci/matrix_test.py`. Expected: FAIL.
- [ ] **Step 2: Write it.**
  - `matrix.py`:
    `NIGHTLY_LANES = {('crypto', 'c-suites'), ('crypto', 'constant-time'), ('crypto', 'browser'), ('crypto', 'throughput'), ('asio', 'asyncify')}`,
    with a comment naming spec 8's table and DESIGN's asio section 11; `own-lanes` takes
    `--level`, `push` by default, and lists the lanes of `NIGHTLY_LANES` at `nightly` alone
    and the others at `push` alone; `plan` takes `--level` too and gives no ordinary lane at
    `nightly`; `report` takes `--level` and titles the matrix with it.
  - `library.yml`: `inputs.level` (`type: string`, default `push`), which the plan job passes
    to `plan` and `own-lanes`; `container`, `lanes`, `docs`, `lint` and `actionlint` gain
    `if: inputs.level != 'nightly'` (joined to their existing conditions); the `report` job
    runs at both levels, passes `--level` to `matrix.py report`, uploads its artifact as
    `report` at `push` and `report-nightly` at `nightly`, writes the matrix into the job's
    summary too, and its last step expects `lanes` skipped at `nightly` (success at `push`,
    as today) and `own-lanes` success whenever the plan found own lanes; the `tools` job
    becomes a matrix, `ubuntu-24.04` at `push` (its steps unchanged) and `macos-15` and
    `windows-2022` at `nightly`, whose steps install the actions the native lane of that OS
    installs (`boost`, `openssl`, `nist-cavp`) and run
    `python3 tools/test/crypto_jamfiles_test.py --target native --require-external` alone.
    The nightly results are seen in the `nightly.yml` run of the superproject's Actions page:
    the report job's summary and its `report-nightly` artifact, and the run's status; a red
    night is a failed run of `main`, which GitHub notifies as any failed run.
  - `.github/workflows/nightly.yml`, scheduled at `0 2 * * *` on `main` and on
    `workflow_dispatch`, calls `./.github/workflows/library.yml` with `level: nightly` and no
    library, as `ci.yml` calls it, `permissions: contents: read`; and `runners.yml` (Task 22)
    keeps its own schedule.
  - AGENTS.md chapter 9 names the level, the workflow, what it runs (the list above), where
    its results are seen (the report job's summary and `report-nightly`), and that library
    repositories never run nightly lanes, `runners.yml` being crypto's measurement and not a
    lane;
    chapter 13's levels item says the nightly level exists for these own lanes and the tools'
    macOS and Windows legs, and that the ordinary lanes' levels, the native lanes without
    exceptions, the release level and crypto's `asyncify` variant are still to come (m16,
    m17).
  Run the case, `actionlint` and the lint. Expected: PASS. Then, with the owner's agreement
  (G4), one `workflow_dispatch` of `nightly.yml` on the pushed branch: its jobs are `plan`,
  the own lanes above, the two tools legs and `report`, whose summary lists the nightly lanes
  alone, and nothing else.
- [ ] **Step 3: Commit:** `"tools/ci: the nightly level: the heavy own lanes, asio's asyncify
  and the tools' macOS and Windows legs"` (after `git add -- .github/workflows/nightly.yml`).

---

# Increment 2: libsecp256k1 and ctaes

## Task 26: libsecp256k1 built file by file, its tables, its canary and its own tests

**Files:**
- Create (superproject): the submodule `third_party/libs/secp256k1`.
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (the feature and the target),
  `tools/test/third_party_test.py` (its cases), CODING-GUIDELINES.md chapter 10, "Third-party
  code" (a `secp256k1` row: libsecp256k1 v0.8.0 at `6e2c8bc`, built file by file by
  `third_party/Jamfile`, MIT), `doc/releases/0.1.0.adoc` ("Tools and build").
- Create (library): `src/canary_secp256k1.c`, `include/webcpp/crypto/backends/secp256k1.hpp`,
  `test/messages/secp256k1_not_linked/`, `LICENSE-SECP256K1.txt` (a copy of the submodule's
  `COPYING`, MIT, which the README names, as `LICENSE-BEARSSL.txt` is BearSSL's).
- Modify (library): `include/webcpp/crypto/detail/canary.h` (the pin),
  `test/c_suites/Jamfile` (`tests.c`), `test/messages/Jamfile`, `test/unused/Jamfile`.

**Interfaces:**
- Produces: `/third_party//secp256k1`; the feature `webcpp-secp256k1-tables`
  (`auto`, `small`, `large`, propagated); the usage requirement
  `WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES=1` where the tables are small;
  `struct webcpp::crypto::backends::secp256k1 { static constexpr std::string_view name = "secp256k1"; };`;
  the macro `WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY`; the canary
  `webcpp_crypto_secp256k1_6e2c8bc_<w15c11x6|w2c2x5>__add_third_party_secp256k1_to_your_link`.

- [ ] **Step 1: Add the submodule:**

```bash
git submodule add https://github.com/bitcoin-core/secp256k1.git third_party/libs/secp256k1
git -C third_party/libs/secp256k1 checkout 6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d
git -C third_party/libs/secp256k1 describe --tags
```

  Expected: `v0.8.0`.
- [ ] **Step 2: Write the failing tests** in `tools/test/third_party_test.py`: the target
  builds 4 C files (`src/secp256k1.c`, `src/precomputed_ecmult.c`,
  `src/precomputed_ecmult_gen.c`, the canary) with the seven `ENABLE_MODULE_*=1` defines and,
  natively, no table define (libsecp256k1's defaults are the large tables, window 15 and comb
  11x6, `src/ecmult.h`, `src/ecmult_gen.h`), and the archive defines
  `webcpp_crypto_secp256k1_6e2c8bc_w15c11x6__add_third_party_secp256k1_to_your_link`; with
  `webcpp-secp256k1-tables=small` the three small defines and the canary `w2c2x5`; with
  `toolset=emscripten` and no request, the same small defines and canary. And the message
  test `test/messages/secp256k1_not_linked/` (native, wasip2), whose source includes
  `<webcpp/crypto/secp256k1.hpp>` and states spec 4.3's sixth row. Run them. Expected: FAIL.
- [ ] **Step 3: Write the target** in `third_party/Jamfile`:

```jam
# The precomputed tables of libsecp256k1 (spec section 3.5): auto (the
# default) is small on Emscripten, the web's target, and large natively and on
# WASI, the servers' (a ruling, spec section 10); small is window 2 and comb
# 2x5, about 1 MB smaller and a third slower to verify; large is the library's
# default, window 15 and comb 11x6. Propagated, so that a program's request
# reaches the library.
feature.feature webcpp-secp256k1-tables : auto small large : propagated ;

# libsecp256k1 v0.8.0 (MIT) as its CMake builds it: src/secp256k1.c and the two
# precomputed tables, with every module, recovery included. A program that
# uses MSVC's static library needs SECP256K1_STATIC, or the header declares
# every function imported from a DLL.
path-constant SECP256K1 : libs/secp256k1 ;

rule secp256k1-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(SECP256K1)/include/secp256k1.h ] ]
    {
        errors.user-error "third_party/libs/secp256k1 is not checked out: it holds no"
            "include/secp256k1.h. Check it out with `git submodule update --init"
            "third_party/libs/secp256k1`." ;
    }
}

.secp256k1-small = <define>ECMULT_WINDOW_SIZE=2 <define>COMB_BLOCKS=2 <define>COMB_TEETH=5 ;
lib secp256k1
  : libs/secp256k1/src/secp256k1.c libs/secp256k1/src/precomputed_ecmult.c
    libs/secp256k1/src/precomputed_ecmult_gen.c $(CRYPTO)/src/canary_secp256k1.c
  : $(.c-library) <conditional>@secp256k1-checked-out
    <include>libs/secp256k1/include <include>libs/secp256k1/src
    <define>ENABLE_MODULE_ECDH=1 <define>ENABLE_MODULE_RECOVERY=1
    <define>ENABLE_MODULE_EXTRAKEYS=1 <define>ENABLE_MODULE_SCHNORRSIG=1
    <define>ENABLE_MODULE_MUSIG=1 <define>ENABLE_MODULE_ELLSWIFT=1
    <define>ENABLE_MODULE_SILENTPAYMENTS=1 <toolset>msvc:<define>SECP256K1_STATIC
    <webcpp-secp256k1-tables>small:$(.secp256k1-small)
    <toolset>emscripten,<webcpp-secp256k1-tables>auto:$(.secp256k1-small)
  : : <include>libs/secp256k1/include <define>WEBCPP_CRYPTO_HAS_SECP256K1=1
    <toolset>msvc:<define>SECP256K1_STATIC
    <webcpp-secp256k1-tables>small:<define>WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES=1
    <toolset>emscripten,<webcpp-secp256k1-tables>auto:<define>WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES=1
    $(.gc) ;
explicit secp256k1 ;
```

  `<webcpp-secp256k1-tables>small:$(.secp256k1-small)` expands to the condition on each of
  the three defines (Jam's product: a condition followed by a list gives one conditional
  property per element); the step checks it in the test's `-d+2` output.
- [ ] **Step 4: Write the canary and the backend's header.** `src/canary_secp256k1.c` is the
  prototype's (`PROBES jam/crypto/src/canary_secp256k1.c`) with the commit through
  `detail/canary.h`, which gains:

```c
#ifdef WEBCPP_CRYPTO_SECP256K1_CANARY_COMMIT
#define WEBCPP_CRYPTO_SECP256K1_COMMIT WEBCPP_CRYPTO_SECP256K1_CANARY_COMMIT
#else
#define WEBCPP_CRYPTO_SECP256K1_COMMIT 6e2c8bc
#endif
```

  `include/webcpp/crypto/backends/secp256k1.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The backend libsecp256k1: its v0.8.0, built from its sources by `/third_party//secp256k1`,
 every module on. It serves `secp256k1::keypair`, `verify_schnorr` and `verify_ecdsa`, and
 its whole C API stays reachable through `<secp256k1*.h>`.

 @see "secp256k1", in the guide.
*/
#ifndef WEBCPP_CRYPTO_BACKENDS_SECP256K1_HPP
#define WEBCPP_CRYPTO_BACKENDS_SECP256K1_HPP

#include <webcpp/crypto/config.hpp>

#if !defined(WEBCPP_CRYPTO_HAS_SECP256K1)
#error "webcpp::crypto: <webcpp/crypto/secp256k1.hpp> needs libsecp256k1, which is not linked. In the program's Jamfile, add /third_party//secp256k1 beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_SECP256K1=1 and link libsecp256k1 built with webcpp's src/canary_secp256k1.c)"
#else

#include <webcpp/crypto/detail/canary.h>
#include <webcpp/crypto/detail/engines.hpp>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_preallocated.h>
#include <secp256k1_schnorrsig.h>

#include <string_view>

// The configuration the canary names: the tables third_party/Jamfile built.
#if defined(WEBCPP_CRYPTO_SECP256K1_CANARY_CONFIGURATION)
#define WEBCPP_CRYPTO_DETAIL_SECP256K1_CONFIGURATION WEBCPP_CRYPTO_SECP256K1_CANARY_CONFIGURATION
#elif defined(WEBCPP_CRYPTO_SECP256K1_SMALL_TABLES)
#define WEBCPP_CRYPTO_DETAIL_SECP256K1_CONFIGURATION w2c2x5
#else
#define WEBCPP_CRYPTO_DETAIL_SECP256K1_CONFIGURATION w15c11x6
#endif
#define WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY                                   \
    WEBCPP_CRYPTO_DETAIL_CANARY(secp256k1, WEBCPP_CRYPTO_SECP256K1_COMMIT, \
                                WEBCPP_CRYPTO_DETAIL_SECP256K1_CONFIGURATION)

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_backend_secp256k1",                                    \
                        WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(secp256k1,                        \
                                                            WEBCPP_CRYPTO_SECP256K1_COMMIT,   \
                                                            WEBCPP_CRYPTO_DETAIL_SECP256K1_CONFIGURATION))
#endif

extern "C" const char WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY;

namespace webcpp::crypto::backends {

/** The backend libsecp256k1 (spec section 3.5). */
struct secp256k1 {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "secp256k1";
};

}  // namespace webcpp::crypto::backends

#endif  // WEBCPP_CRYPTO_HAS_SECP256K1
#endif  // WEBCPP_CRYPTO_BACKENDS_SECP256K1_HPP
```

- [ ] **Step 5: libsecp256k1's own tests on webcpp's build** (spec 8.1, CODING-GUIDELINES.md
  chapter 5 rule 11). In `test/c_suites/Jamfile` (Task 5's lane `c-suites`, which runs on
  every push): `src/tests.c` includes `secp256k1.c` itself (`src/tests.c:21`) but not the
  precomputed tables, which libsecp256k1's CMake links into it as `secp256k1_precomputed`
  (`src/CMakeLists.txt:134-135`), so it is built from `tests.c` and the two table files with
  the library's defines, never linked with the archive:

```jam
# libsecp256k1's tests, which include secp256k1.c and so recompile it with these
# defines, third_party/Jamfile's for the library (a change of one is a change of
# both), and VERIFY, as libsecp256k1's CMake builds them. The archive webcpp
# links is tested by the vectors of the library's own tests.
path-constant SECP256K1 : ../../../../third_party/libs/secp256k1 ;
.secp256k1 = <include>$(SECP256K1)/include <include>$(SECP256K1)/src
    <define>ENABLE_MODULE_ECDH=1 <define>ENABLE_MODULE_RECOVERY=1
    <define>ENABLE_MODULE_EXTRAKEYS=1 <define>ENABLE_MODULE_SCHNORRSIG=1
    <define>ENABLE_MODULE_MUSIG=1 <define>ENABLE_MODULE_ELLSWIFT=1
    <define>ENABLE_MODULE_SILENTPAYMENTS=1 <warnings>off <warnings-as-errors>off ;
# Optimised in every variant: VERIFY's checks at -O0 under wasmtime and node would
# not end within the own-lanes job's 60 minutes (measured in Step 6).
webcpp.run secp256k1_tests : $(SECP256K1)/src/tests.c
    $(SECP256K1)/src/precomputed_ecmult.c $(SECP256K1)/src/precomputed_ecmult_gen.c
  : $(.secp256k1) <define>VERIFY <optimization>speed ;
```

  and the lane's list gains `secp256k1_tests`. Its time is measured on each target in
  Step 6 (m6): the framework runs 16 iterations of each test unless told otherwise
  (`src/unit_test.c`, `--iterations=<num>`, default 16). A target on which one run passes
  20 minutes, a third of the job's 60, takes `<testing.arg>--iterations=4` by a conditional
  on that target, with the measured times in the comment above; natively it stays at 16, so
  the lane on the native compilers runs the suite as libsecp256k1's CI does.
  (Deferred after v0.1: libsecp256k1's `src/ctime_tests.c` in the lane of Task 20.)
  The unused-library case gains
  `webcpp.same-program unused_secp256k1 : app.cpp : /third_party//secp256k1 : : <library>/webcpp/crypto//crypto ;`.
- [ ] **Step 6: Run** the third-party test, the message test, the lane `c-suites` on the four
  targets, and `unused` on the four. Expected: PASS. Record `secp256k1_tests`' time on each
  of the four targets in the ledger (m6) and apply Step 5's rule.
  `secp256k1_tests` runs its tests in sequence on every target: the framework forks workers
  only with `SUPPORTS_CONCURRENCY` (`src/unit_test.c:10`), which no source defines; CMake adds
  it only where `sys/types.h`, `sys/wait.h` and `unistd.h` exist (`src/CMakeLists.txt:128-131`),
  and webcpp's build never does.
- [ ] **Step 7: Commit** (superproject, with the chapter 10 row and the line of
  `doc/releases/0.1.0.adoc`) `"third_party: libsecp256k1 v0.8.0, its tables by
  target, with webcpp::crypto's canary"`; (library, with its line of `doc/changelog.adoc`) `"crypto: libsecp256k1's backend header,
  its canary, and its own tests on webcpp's build"`.

## Task 27: `secp256k1::keypair`, `verify_schnorr` and `verify_ecdsa`

**Files:**
- Create: `include/webcpp/crypto/secp256k1.hpp`, `test/secp256k1_test.cpp`,
  `test/secp256k1_allocation_test.cpp`.
- Modify: `include/webcpp/crypto.hpp`, `test/Jamfile`.

**Interfaces:**
- Consumes: `backends::secp256k1`, `random_device`, `detail::wipe`, `error`.
- Produces: spec 2.8's interface exactly: `webcpp::crypto::secp256k1::basic_keypair<Backend>`
  with `make(secret, memory)`, `xonly_public_key()`, `public_key()`, `sign_schnorr(message, aux)`,
  `sign_ecdsa(hash)`, `ecdh(public_key)`; `using keypair = basic_keypair<>;`;
  `verify_schnorr<Backend>(signature, message, xonly_public_key)`,
  `verify_ecdsa<Backend>(signature, hash, public_key)`.

- [ ] **Step 1: Write the failing test** `test/secp256k1_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// secp256k1's key pair and verifications (spec section 2.8): signatures that verify and a
// changed message that does not, ECDSA in low-S compact form, ECDH agreeing between two
// parties, every error of make, a moved-from key pair, and public keys refused in every way
// they can be wrong.

#include <webcpp/crypto/random_device.hpp>
#include <webcpp/crypto/secp256k1.hpp>
#include <webcpp/test/exit_status.hpp>

#include "support/hex.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/hash2/sha2.hpp>
#include <boost/system/errc.hpp>

#include <array>
#include <cstdint>
#include <utility>

namespace crypto = webcpp::crypto;
namespace secp = webcpp::crypto::secp256k1;

namespace {

secp::keypair from_byte(std::uint8_t b) {
    std::array<std::uint8_t, 32> secret{};
    secret.fill(b);
    return std::move(*secp::keypair::make(secret));
}

void schnorr() {
    auto const key = from_byte(0x42);
    std::array<std::uint8_t, 32> message{};
    message.fill(0x17);
    std::array<std::uint8_t, 32> const aux{};
    auto const signature = key.sign_schnorr(message, aux);
    auto const public_key = key.xonly_public_key();
    BOOST_TEST(signature && public_key);
    BOOST_TEST(secp::verify_schnorr(*signature, message, *public_key).has_value());
    message[0] ^= 1;
    BOOST_TEST(secp::verify_schnorr(*signature, message, *public_key).error() ==
               crypto::error::invalid_signature);
    // Any length, the empty message included (BIP340).
    auto const empty = key.sign_schnorr(std::span<std::uint8_t const>(), aux);
    BOOST_TEST(empty && secp::verify_schnorr(*empty, {}, *public_key).has_value());
    std::array<std::uint8_t, 32> const not_a_point{};  // x = 0 has no point
    BOOST_TEST(secp::verify_schnorr(*signature, message, not_a_point).error() ==
               crypto::error::invalid_public_key);
}

void ecdsa() {
    auto const key = from_byte(0x24);
    std::array<std::uint8_t, 32> hash{};
    hash.fill(0x99);
    auto const signature = key.sign_ecdsa(hash);
    auto const public_key = key.public_key();
    BOOST_TEST(signature && public_key);
    BOOST_TEST(secp::verify_ecdsa(*signature, hash, *public_key).has_value());
    // Low-S: s is at most half the group order, so its first byte is below 0x80.
    BOOST_TEST((*signature)[32] < 0x80);
    // The high-S form of the same signature is refused, as libsecp256k1's verify refuses it.
    auto high = *signature;
    // n - s, big-endian, with n the group order.
    auto const order = webcpp::test::from_hex(
        "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
    int borrow = 0;
    for (int i = 31; i >= 0; --i) {
        int const d = order[static_cast<std::size_t>(i)] - (*signature)[static_cast<std::size_t>(32 + i)] - borrow;
        high[static_cast<std::size_t>(32 + i)] = static_cast<std::uint8_t>(d & 0xff);
        borrow = d < 0 ? 1 : 0;
    }
    BOOST_TEST(secp::verify_ecdsa(high, hash, *public_key).error() ==
               crypto::error::invalid_signature);
    std::array<std::uint8_t, 64> overflow{};
    overflow.fill(0xff);  // r and s not below the order: the compact parser refuses it
    BOOST_TEST(secp::verify_ecdsa(overflow, hash, *public_key).error() ==
               crypto::error::invalid_signature);
    std::array<std::uint8_t, 34> wrong_size{};
    BOOST_TEST(secp::verify_ecdsa(*signature, hash, wrong_size).error() ==
               crypto::error::invalid_public_key);
    auto bad_prefix = *public_key;
    bad_prefix[0] = 0x05;
    BOOST_TEST(secp::verify_ecdsa(*signature, hash, bad_prefix).error() ==
               crypto::error::invalid_public_key);
}

void ecdh() {
    auto const a = from_byte(0x11);
    auto const b = from_byte(0x22);
    auto const shared_a = a.ecdh(*b.public_key());
    auto const shared_b = b.ecdh(*a.public_key());
    BOOST_TEST(shared_a && shared_b && *shared_a == *shared_b);
    std::array<std::uint8_t, 33> off_curve{};
    off_curve[0] = 0x02;  // x = 0: no point
    BOOST_TEST(a.ecdh(off_curve).error() == crypto::error::invalid_public_key);
}

void make_errors() {
    std::array<std::uint8_t, 31> short_secret{};
    BOOST_TEST(secp::keypair::make(short_secret).error() == crypto::error::invalid_key_size);
    std::array<std::uint8_t, 32> const zero{};
    BOOST_TEST(secp::keypair::make(zero).error() == crypto::error::invalid_secret_key);
    std::array<std::uint8_t, 32> const order{
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
        0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41};
    BOOST_TEST(secp::keypair::make(order).error() == crypto::error::invalid_secret_key);
    // No generator (an audio worklet) is entropy_unavailable to a key pair's caller (m15).
    namespace errc = boost::system::errc;
    BOOST_TEST(secp::detail::seed_error(errc::make_error_code(errc::not_supported)) ==
               crypto::error::entropy_unavailable);
    BOOST_TEST(secp::detail::seed_error(crypto::error::entropy_unavailable) ==
               crypto::error::entropy_unavailable);
}

void moved_from() {
    auto key = from_byte(0x42);
    secp::keypair moved = std::move(key);
    std::array<std::uint8_t, 32> const message{};
    std::array<std::uint8_t, 32> const aux{};
    BOOST_TEST(key.xonly_public_key().error() == crypto::error::invalid_state);
    BOOST_TEST(key.public_key().error() == crypto::error::invalid_state);
    BOOST_TEST(key.sign_schnorr(message, aux).error() == crypto::error::invalid_state);
    BOOST_TEST(key.sign_ecdsa(message).error() == crypto::error::invalid_state);
    BOOST_TEST(key.ecdh(*moved.public_key()).error() == crypto::error::invalid_state);
    BOOST_TEST(moved.sign_schnorr(message, aux).has_value());
    // A key pair moved from a moved-from one is empty too (I1).
    secp::keypair again = std::move(key);
    BOOST_TEST(again.public_key().error() == crypto::error::invalid_state);
    std::swap(again, moved);
    BOOST_TEST(moved.public_key().error() == crypto::error::invalid_state);
    key = std::move(again);
    BOOST_TEST(key.sign_schnorr(message, aux).has_value());
}

}  // namespace

int main() {
    schnorr();
    ecdsa();
    ecdh();
    make_errors();
    moved_from();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/secp256k1_allocation_test.cpp` (spec 8.5): a counting `std::pmr::memory_resource`
  given to `make`, and `support/counting.hpp`'s `operator new`: `make` allocates exactly once
  in the resource (the context, of `secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE)`
  bytes, aligned to `alignof(std::max_align_t)`) and never through `operator new`; the
  signatures, `ecdh`, the public keys and both verifications allocate nothing; the destructor
  gives the block back. In `test/Jamfile`:

```jam
crypto-run secp256k1 : secp256k1_test.cpp : <library>/third_party//secp256k1 ;
crypto-run secp256k1_allocation : secp256k1_allocation_test.cpp
  : <library>/third_party//secp256k1 ;
```

  Run the native lane. Expected: FAIL, `secp256k1.hpp` not found.
- [ ] **Step 2: Write `include/webcpp/crypto/secp256k1.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 secp256k1 on libsecp256k1: BIP340 Schnorr and ECDSA signatures, their verification, and
 ECDH.

 @see "secp256k1", in the guide.
*/
#ifndef WEBCPP_CRYPTO_SECP256K1_HPP
#define WEBCPP_CRYPTO_SECP256K1_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/backends/secp256k1.hpp>
#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/crypto/error.hpp>
#include <webcpp/crypto/random_device.hpp>  // lint-world: the context's randomisation, its purpose

#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <utility>

namespace webcpp::crypto::secp256k1 {

namespace detail {

// A context that signs and verifies needs no flag. The macro shifts a signed constant, which
// no check of webcpp's .clang-tidy reports (hicpp-signed-bitwise is not among them), so it
// carries no NOLINT.
inline constexpr unsigned context_flags = SECP256K1_CONTEXT_NONE;

/**
 * The error of a key pair whose context could not be randomised: every failure of the
 * generator is `entropy_unavailable`, spec 2.8's one word for it.
 *
 * Tip: the system's generator answers `errc::not_supported` where there is none (an
 * Emscripten audio worklet); a key pair's caller handles one error for both.
 */
inline boost::system::error_code seed_error(boost::system::error_code /*generator*/) noexcept {
    return error::entropy_unavailable;
}

/** The secret of a key pair, out of it, wiped when the scope ends on every path. */
struct secret {
    std::array<unsigned char, 32> bytes{};
    ~secret() { webcpp::crypto::detail::wipe(bytes.data(), bytes.size()); }
};

/** A public key of 33 or 65 bytes, parsed; `invalid_public_key` in every way it can be wrong. */
inline boost::system::result<secp256k1_pubkey> parse(std::span<std::uint8_t const> key) noexcept {
    secp256k1_pubkey parsed{};
    if ((key.size() != 33 && key.size() != 65) ||
        secp256k1_ec_pubkey_parse(secp256k1_context_static, &parsed, key.data(), key.size()) == 0) {
        return webcpp::crypto::error::invalid_public_key;
    }
    return parsed;
}

}  // namespace detail

/**
 * A secp256k1 key pair on libsecp256k1, with a signing context of its own in the resource it
 * is given, randomised from the random device as libsecp256k1 recommends.
 *
 * @tparam Backend `backends::secp256k1`, its only backend.
 */
template <class Backend = backends::secp256k1>
class basic_keypair {
public:
    /** The backend. */
    using backend_type = Backend;

    /**
     * Makes a key pair from a secret key.
     *
     * @param secret 32 bytes, not zero and below the group order.
     * @param memory The resource of the signing context, the one allocation; it must outlive
     * the key pair.
     * @return The key pair; `invalid_key_size`; `invalid_secret_key`;
     * `entropy_unavailable`, also where there is no generator at all (an audio worklet,
     * `detail::seed_error`). A failed allocation is the resource's to report.
     */
    static boost::system::result<basic_keypair> make(
        std::span<std::uint8_t const> secret,
        std::pmr::memory_resource* memory = std::pmr::get_default_resource()) {
        if (secret.size() != 32) {
            return error::invalid_key_size;
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        if (secp256k1_ec_seckey_verify(secp256k1_context_static, secret.data()) == 0) {
            return error::invalid_secret_key;
        }
        std::array<std::uint8_t, 32> seed{};
        auto const seeded = random_device().fill(seed);  // lint-world: the context's randomisation
        if (!seeded) {
            return detail::seed_error(seeded.error());
        }
        basic_keypair made;
        made.memory_ = memory;
        made.context_ = secp256k1_context_preallocated_create(
            memory->allocate(context_size(), alignof(std::max_align_t)), detail::context_flags);
        int const randomised = secp256k1_context_randomize(made.context_, seed.data());
        webcpp::crypto::detail::wipe(seed.data(), seed.size());
        if (randomised == 0 ||
            secp256k1_keypair_create(made.context_, &made.keypair_, secret.data()) == 0) {
            return error::backend_failure;
        }
        made.keyed_ = true;
        return made;
    }

    /** Moves the key pair and its context; the source is wiped and empty. */
    basic_keypair(basic_keypair&& other) noexcept
        : context_(std::exchange(other.context_, nullptr)),
          memory_(other.memory_),
          keypair_(other.keypair_),
          keyed_(std::exchange(other.keyed_, false)) {
        webcpp::crypto::detail::wipe_object(other.keypair_);
    }

    /** Moves over this key pair, which is wiped and gives its context back first. */
    basic_keypair& operator=(basic_keypair&& other) noexcept {
        if (this != &other) {
            release();
            context_ = std::exchange(other.context_, nullptr);
            memory_ = other.memory_;
            keypair_ = other.keypair_;
            keyed_ = std::exchange(other.keyed_, false);
            webcpp::crypto::detail::wipe_object(other.keypair_);
        }
        return *this;
    }

    basic_keypair(basic_keypair const&) = delete;
    basic_keypair& operator=(basic_keypair const&) = delete;

    /** Wipes the key pair and gives the context back to its resource. */
    ~basic_keypair() { release(); }

    /**
     * The BIP340 x-only public key.
     *
     * @return 32 bytes; `invalid_state` moved from.
     */
    boost::system::result<std::array<std::uint8_t, 32>> xonly_public_key() const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        secp256k1_xonly_pubkey xonly{};
        std::array<std::uint8_t, 32> out{};
        secp256k1_keypair_xonly_pub(context_, &xonly, nullptr, &keypair_);
        secp256k1_xonly_pubkey_serialize(context_, out.data(), &xonly);
        return out;
    }

    /**
     * The public key, SEC1 compressed.
     *
     * @return 33 bytes; `invalid_state` moved from.
     */
    boost::system::result<std::array<std::uint8_t, 33>> public_key() const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        secp256k1_pubkey key{};
        std::array<std::uint8_t, 33> out{};
        std::size_t size = out.size();
        secp256k1_keypair_pub(context_, &key, &keypair_);
        secp256k1_ec_pubkey_serialize(context_, out.data(), &size, &key, SECP256K1_EC_COMPRESSED);
        return out;
    }

    /**
     * A BIP340 signature of a message of any length.
     *
     * @param message The message.
     * @param aux 32 bytes of fresh randomness, the caller's.
     * @return 64 bytes; `invalid_state` moved from; `backend_failure`.
     */
    boost::system::result<std::array<std::uint8_t, 64>> sign_schnorr(
        std::span<std::uint8_t const> message, std::span<std::uint8_t const, 32> aux) const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        std::array<std::uint8_t, 32> randomness{};
        std::copy(aux.begin(), aux.end(), randomness.begin());
        secp256k1_schnorrsig_extraparams extra = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
        extra.ndata = randomness.data();
        std::array<std::uint8_t, 64> out{};
        if (secp256k1_schnorrsig_sign_custom(context_, out.data(), message.data(), message.size(),
                                             &keypair_, &extra) == 0) {
            return error::backend_failure;
        }
        return out;
    }

    /**
     * An ECDSA signature of a 32-byte hash: RFC 6979's nonce, low-S, compact form.
     *
     * @param hash The hash.
     * @return 64 bytes, r then s; `invalid_state` moved from; `backend_failure`.
     */
    boost::system::result<std::array<std::uint8_t, 64>> sign_ecdsa(
        std::span<std::uint8_t const, 32> hash) const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        detail::secret secret;
        secp256k1_keypair_sec(context_, secret.bytes.data(), &keypair_);
        secp256k1_ecdsa_signature signature{};
        std::array<std::uint8_t, 64> out{};
        if (secp256k1_ecdsa_sign(context_, &signature, hash.data(), secret.bytes.data(), nullptr,
                                 nullptr) == 0) {
            return error::backend_failure;
        }
        secp256k1_ecdsa_signature_serialize_compact(context_, out.data(), &signature);
        return out;
    }

    /**
     * The SHA-256 of the compressed shared point with a peer's public key, libsecp256k1's
     * default.
     *
     * @param public_key The peer's key, 33 or 65 bytes.
     * @return 32 bytes; `invalid_state`; `invalid_public_key`; `backend_failure`.
     */
    boost::system::result<std::array<std::uint8_t, 32>> ecdh(
        std::span<std::uint8_t const> public_key) const noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        auto const parsed = detail::parse(public_key);
        if (!parsed) {
            return parsed.error();
        }
        webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
        detail::secret secret;
        secp256k1_keypair_sec(context_, secret.bytes.data(), &keypair_);
        std::array<std::uint8_t, 32> out{};
        if (secp256k1_ecdh(context_, out.data(), &*parsed, secret.bytes.data(), nullptr,
                           nullptr) == 0) {
            return error::backend_failure;
        }
        return out;
    }

private:
    basic_keypair() noexcept = default;

    static std::size_t context_size() noexcept {
        return secp256k1_context_preallocated_size(detail::context_flags);
    }

    void release() noexcept {
        webcpp::crypto::detail::wipe_object(keypair_);
        keyed_ = false;
        if (context_ != nullptr) {
            secp256k1_context_preallocated_destroy(context_);
            memory_->deallocate(context_, context_size(), alignof(std::max_align_t));
            context_ = nullptr;
        }
    }

    secp256k1_context* context_ = nullptr;
    std::pmr::memory_resource* memory_ = nullptr;
    secp256k1_keypair keypair_{};
    bool keyed_ = false;
};

/** A key pair on libsecp256k1. */
using keypair = basic_keypair<>;

/**
 * Verifies a BIP340 signature with libsecp256k1's static context, which allocates nothing,
 * after its self-test (`include/secp256k1.h:235-247`).
 *
 * @param signature 64 bytes.
 * @param message The message, of any length.
 * @param xonly_public_key 32 bytes.
 * @return Nothing; `invalid_public_key`; `invalid_signature`.
 */
template <class Backend = backends::secp256k1>
boost::system::result<void> verify_schnorr(std::span<std::uint8_t const, 64> signature,
                                           std::span<std::uint8_t const> message,
                                           std::span<std::uint8_t const, 32> xonly_public_key) noexcept {
    webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
    secp256k1_selftest();
    secp256k1_xonly_pubkey key{};
    if (secp256k1_xonly_pubkey_parse(secp256k1_context_static, &key, xonly_public_key.data()) == 0) {
        return error::invalid_public_key;
    }
    if (secp256k1_schnorrsig_verify(secp256k1_context_static, signature.data(), message.data(),
                                    message.size(), &key) == 0) {
        return error::invalid_signature;
    }
    return {};
}

/**
 * Verifies an ECDSA signature in compact form with the static context, after its self-test;
 * a high-S signature is refused, as libsecp256k1 refuses it.
 *
 * @param signature 64 bytes, r then s.
 * @param hash The 32-byte hash signed.
 * @param public_key 33 or 65 bytes, SEC1.
 * @return Nothing; `invalid_public_key`; `invalid_signature`.
 */
template <class Backend = backends::secp256k1>
boost::system::result<void> verify_ecdsa(std::span<std::uint8_t const, 64> signature,
                                         std::span<std::uint8_t const, 32> hash,
                                         std::span<std::uint8_t const> public_key) noexcept {
    webcpp::crypto::detail::read_canary(WEBCPP_CRYPTO_DETAIL_SECP256K1_CANARY);
    secp256k1_selftest();
    auto const key = detail::parse(public_key);
    if (!key) {
        return key.error();
    }
    secp256k1_ecdsa_signature parsed{};
    if (secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &parsed,
                                                signature.data()) == 0 ||
        secp256k1_ecdsa_verify(secp256k1_context_static, &parsed, hash.data(), &*key) == 0) {
        return error::invalid_signature;
    }
    return {};
}

}  // namespace webcpp::crypto::secp256k1

#endif  // WEBCPP_CRYPTO_SECP256K1_HPP
```

  `make`'s order follows spec 2.8's errors: the size, the secret, the entropy, then the one
  allocation, which comes after every guard so that a refused `make` allocates nothing.
  `include/webcpp/crypto.hpp` gains
  `#if defined(WEBCPP_CRYPTO_HAS_SECP256K1)` `#include <webcpp/crypto/secp256k1.hpp>`
  `#endif`, and the headers-alone calls a call of their own for `crypto/secp256k1.hpp` and
  `crypto/backends/secp256k1.hpp` with `<library>/third_party//secp256k1` on the four
  targets.
- [ ] **Step 3: Run** the four lanes. Expected: PASS, `secp256k1`, `secp256k1_allocation`
  and their twins on every target (the moved-from twins too: no exception is involved).
- [ ] **Step 4: Commit:** `"crypto: secp256k1's key pair, BIP340 and ECDSA signatures, their
  verifications and ECDH on libsecp256k1"`.

## Task 27b: what a Bitcoin wallet needs from secp256k1 (DER, low-S, the Taproot tweak)

Added on 2026-11-14 by the owner's decision ("Incluir na v0.1"), after the guide's Bitcoin
section showed three gaps. Builds on Task 27's `include/webcpp/crypto/secp256k1.hpp`
(`basic_keypair`, `verify_schnorr`, `verify_ecdsa`), Task 26's `/third_party//secp256k1`
target and Task 28's `bips` submodule. Thin wrappers over libsecp256k1 v0.8.0's own
functions: webcpp writes no curve arithmetic and no DER of its own.

## Interface (namespace `webcpp::crypto::secp256k1`, in `secp256k1.hpp`)

```cpp
/// The longest strict DER encoding of a secp256k1 ECDSA signature.
inline constexpr std::size_t max_der_size = 72;

/// Encodes a compact signature (r then s, 64 bytes) as strict DER (BIP66) into `out`.
/// @return the number of bytes written; invalid_signature for a compact signature
/// libsecp256k1 refuses; invalid_input_size when `out` is shorter than max_der_size.
template <class Backend = backends::secp256k1>
result<std::size_t> to_der(std::span<std::uint8_t const, 64> compact,
                           std::span<std::uint8_t> out) noexcept;

/// Decodes a strict DER signature (secp256k1_ecdsa_signature_parse_der) to compact form.
/// @return the 64 bytes; invalid_signature for anything that is not strict DER.
template <class Backend = backends::secp256k1>
result<std::array<std::uint8_t, 64>> from_der(std::span<std::uint8_t const> der) noexcept;

/// Decodes a DER signature as Bitcoin Core does for historical transactions
/// (libsecp256k1's contrib/lax_der_parsing.c, ecdsa_signature_parse_der_lax).
template <class Backend = backends::secp256k1>
result<std::array<std::uint8_t, 64>> from_lax_der(std::span<std::uint8_t const> der) noexcept;

/// The low-S form of a compact signature (secp256k1_ecdsa_signature_normalize), which
/// verify_ecdsa requires; a signature already low-S is returned unchanged.
template <class Backend = backends::secp256k1>
result<std::array<std::uint8_t, 64>> normalize_ecdsa(
    std::span<std::uint8_t const, 64> compact) noexcept;

/// The BIP341 output key: the internal x-only key tweaked by
/// t = tagged_hash("TapTweak", internal || merkle_root), merkle_root empty (key-path only)
/// or 32 bytes. Returns the 32-byte x-only output key and its parity (0 or 1).
template <class Backend = backends::secp256k1>
result<taproot_output> taproot_output_key(std::span<std::uint8_t const, 32> internal_key,
                                          std::span<std::uint8_t const> merkle_root) noexcept;

struct taproot_output {
    std::array<std::uint8_t, 32> key;
    int parity;
};

// In basic_keypair:
/// Tweaks this key pair in place into the BIP341 output key pair a key-path spend signs
/// with (secp256k1_keypair_xonly_tweak_add with the TapTweak tagged hash, computed with
/// secp256k1_tagged_sha256). No allocation.
/// @return invalid_input_size for a merkle_root that is neither empty nor 32 bytes;
/// invalid_state on a moved-from pair; invalid_secret_key if the tweak makes the key
/// invalid (negligible, but checked).
result<void> tweak_taproot(std::span<std::uint8_t const> merkle_root) noexcept;
```

Every function starts with the canary read, as the other secp256k1 functions do, and every
guard runs before any secret byte is touched. The tweaked pair keeps the wiping and
moved-from behaviour of Task 27.

## Build

- Compile `contrib/lax_der_parsing.c` into `/third_party//secp256k1` (it is part of the
  pinned v0.8.0 tree, unmodified; include it with the same flags; its header
  `contrib/lax_der_parsing.h` reaches only the library's own source, never a user's include
  path, so declare its function in webcpp's backend header with its exact prototype or add
  `contrib/` to the library's private includes only). Update the third_party_test case that
  counts the compiled files.
- The canary configuration is unchanged.

## Tests (test-first; each seen failing first)

1. `secp256k1_der_test`: round trips compact -> DER -> compact for random signatures from
   `sign_ecdsa`; every DER is <= 72 bytes and strict; `from_der` refuses non-strict
   encodings that `from_lax_der` accepts (cases from Wycheproof's
   `ecdsa_secp256k1_sha256_bitcoin_test.json`, whose signatures are DER: count accepted and
   refused per flag and assert the counts); `to_der` with a too-short `out` gives
   invalid_input_size and writes nothing.
2. `normalize_ecdsa`: a high-S signature (s' = n - s of a valid one) is refused by
   `verify_ecdsa`, normalized back to the original, and then verifies.
3. `secp256k1_taproot_test`, against `third_party/libs/bips/bip-0341/wallet-test-vectors.json`
   (read through the `bips` data alias): every `scriptPubKey` case's `internalPubkey` and
   `merkleRoot` give `tweakedPubkey` via `taproot_output_key`; every `keyPathSpending`
   input's `internalPrivkey` tweaked by `tweak_taproot` gives its `tweakedPrivkey`
   (compare through the public key, or add a test-only accessor if the brief's API cannot
   show the secret: prefer comparing `xonly_public_key()` with the expected tweaked key and
   the signature of the given sigHash against the vector's expected witness, BIP340 with an
   all-zero aux as the vectors do). Assert the number of cases read.
4. The guide's Bitcoin example: add a `bitcoin_sign` example program under
   `libs/crypto/example/` that signs a P2WPKH input using BIP143's published example
   (the native P2WPKH case of BIP143) and checks the DER signature and sighash against it;
   its `.expected` holds the witness in hex. Use Boost.Hash2's sha2_256 for the hashes.

All on native, wasip2, wasip3, emscripten (and gcc-15 natively), through `crypto-run`, with
headers-alone and the changelog line ("Added DER encoding and decoding of ECDSA signatures,
low-S normalization and the BIP341 Taproot tweak to secp256k1.").

## Task 28: secp256k1's vectors: BIP340, Wycheproof's Bitcoin ECDSA, and ECDH

**Files:**
- Create (superproject): the submodule `third_party/libs/bips` (C12).
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (the target `bips`),
  CODING-GUIDELINES.md chapter 10, "Third-party code" (a `bips` row: `bitcoin/bips` at
  `927b6de9`, shallow, test data, BIP340's `test-vectors.csv`, BSD-2-Clause as BIP340's header states), `doc/releases/0.1.0.adoc`.
- Create (library): `test/secp256k1_bip340_test.cpp`, `test/secp256k1_wycheproof_test.cpp`.
- Modify (library): `test/Jamfile`.

**Interfaces:**
- Produces: `/third_party//bips` (define `WEBCPP_TEST_BIPS`, the checkout's absolute path, and
  the directory under wasmtime).

- [ ] **Step 1: Add the submodule** (shallow from the first clone, m10: `shallow = true` in
  `.gitmodules` reaches later clones only, so the add itself takes `--depth 1` and the pinned
  commit is fetched alone):

```bash
git submodule add --depth 1 https://github.com/bitcoin/bips.git third_party/libs/bips
git -C third_party/libs/bips fetch --depth 1 origin 927b6de9915c9262615a6399de51b200f81e5aa4
git -C third_party/libs/bips checkout 927b6de9915c9262615a6399de51b200f81e5aa4
git config -f .gitmodules submodule.third_party/libs/bips.shallow true
git -C third_party/libs/bips rev-parse HEAD:bip-0340/test-vectors.csv
```

  Expected: `672339129a844a060591bb22f444158ff45438ed`, the blob of the CSV at the pin (its
  last change is `200f9b2`, M6; a shallow checkout has no history to show it, so the blob is
  what is checked; measured 2026-10-11 with these commands, 9.4 MB in `.git/modules`). The target `bips` in `third_party/Jamfile` is `wycheproof`'s
  with the marker `bip-0340/test-vectors.csv` and the define `WEBCPP_TEST_BIPS`.
- [ ] **Step 2: Write the failing tests.** `test/secp256k1_bip340_test.cpp`: it reads
  `WEBCPP_TEST_BIPS "/bip-0340/test-vectors.csv"`, whose header is
  `index,secret key,public key,aux_rand,message,signature,verification result,comment`; for
  each row with a secret key it makes a `keypair`, checks `xonly_public_key()` against the
  public key, signs the message (of any length, empty included) with `aux_rand` and compares
  with the signature, as bytes (the CSV's hex is upper-case, `from_hex` reads both); for every row it calls `verify_schnorr` and expects success when the
  result is `TRUE`; rows 5 ("public key not on the curve") and 14 ("public key is not a
  valid X coordinate because it exceeds the field size") give `invalid_public_key`, and the
  other `FALSE` rows `invalid_signature`. Asserted, as read at the pin on 2026-10-11: 19 rows
  (0 to 18), 8 of them with a secret key, signed and compared, 9 `TRUE`, 2 refused keys,
  8 refused signatures. The CSV is split on commas; no field of it holds one, and a row's
  last field, the comment, may be empty. `test/secp256k1_wycheproof_test.cpp` (spec 8.1,
  CR-PROBES `secp/wp_secp.out`):
  - ECDSA, `testvectors_v1/ecdsa_secp256k1_sha256_bitcoin_test.json` (464 tests): each
    group's `publicKey.uncompressed`, each test's `msg` hashed with Boost.Hash2's SHA-256, its
    DER `sig` converted with `secp256k1_ecdsa_signature_parse_der` and
    `secp256k1_ecdsa_signature_serialize_compact` on `secp256k1_context_static`; a DER the
    parser refuses is counted as skipped (an encoding webcpp's compact interface does not
    take); the others go through `verify_ecdsa`, a valid one succeeding and an invalid one
    giving `invalid_signature`. Asserted: 162 valid verified, 190 skipped, 112 invalid refused;
  - ECDH through the C API (`secp256k1_ecdh` with a hash function of the test's that copies
    x), on `ecdh_secp256k1_test.json` (752 tests, ASN.1 public keys) and
    `ecdh_secp256k1_webcrypto_test.json` (496, JWK): the SEC1 point taken out of a key in the
    standard secp256k1 encoding, the others skipped with that reason. A key is taken when it
    is an ASN.1 `SubjectPublicKeyInfo` of exactly one of the two secp256k1 forms, the
    uncompressed prefix `3056301006072a8648ce3d020106052b8104000a034200` followed by 65 bytes
    or the compressed prefix `3036301006072a8648ce3d020106052b8104000a032200` followed by 33,
    or a JWK whose `crv` is `P-256K` (at the pin 491 of 496; tcIds 490, 493 and 494 are
    `P-256`, 495 `P-384`, 496 `P-521`). A taken case agrees or is refused by what the C API
    does: a `valid` case must agree, an `invalid` one must be refused, and an `acceptable` one
    (230 in the ASN.1 file, the label Wycheproof gives what a library may take or refuse) is
    counted by its behaviour, never failed by its label. The counts are re-measured at
    implementation and asserted as measured, each recorded in the ledger; read from the files
    at the pin on 2026-11-14, the rule takes 496 ASN.1 keys (492 uncompressed, 4 compressed)
    and skips 256, and takes 491 JWKs (473 `valid`, 18 `invalid`) and skips 5. Every key the
    C API refuses also gives `invalid_public_key` through `keypair::ecdh`;
  - webcpp's `ecdh` equals the SHA-256 of the compressed shared point the C API computes with
    its default hash, on every valid ECDH case: `keypair::make(private)`, `ecdh(point)`
    against `secp256k1_ecdh(..., nullptr, nullptr)`;
  - the private keys are normalised to the 32 bytes `keypair::make` and `secp256k1_ecdh`
    take (m5), by one function of the test, `scalar32`: in `ecdh_secp256k1_test.json` the
    `private` field is hex of the ASN.1 integer, 274 of 32 bytes, 476 of 33 (a leading `00`,
    the sign byte), one of 1 byte (tcId 459, `03`) and one of 29 (tcId 460, `00` and 28
    bytes `ff`); `scalar32` drops leading zero bytes and left-pads with zeros to 32, and a
    value longer than 32 bytes once stripped fails the test by name (none at the pin). In
    `ecdh_secp256k1_webcrypto_test.json` the `private` field is a JWK whose `d` is
    base64url without padding, 32 bytes in all 496 cases; a small base64url decoder in the
    test reads it (no padding, `-` and `_`, any other character a failure by name) and the
    result goes through the same `scalar32`. Counted at the pin on 2026-10-11 from the
    files themselves.
  In `test/Jamfile`:

```jam
crypto-run secp256k1_bip340 : secp256k1_bip340_test.cpp
  : <library>/third_party//secp256k1 <library>/third_party//bips ;
crypto-run secp256k1_wycheproof : secp256k1_wycheproof_test.cpp
  : <library>/third_party//secp256k1 <library>/third_party//wycheproof
    <library>/webcpp//boost_json ;
```

  `/third_party//wycheproof` is Task 9's. Run the four lanes. Expected: FAIL first on the
  reader; PASS once written, with every count above (the ECDH counts as measured).
- [ ] **Step 3: Commit** (superproject, with the chapter 10 row and the line of
  `doc/releases/0.1.0.adoc`) `"third_party: bitcoin/bips, for BIP340's vectors"`;
  (library, with its line of `doc/changelog.adoc`) `"crypto: secp256k1 against BIP340, Wycheproof's Bitcoin ECDSA, and its ECDH"`.

## Task 29: ctaes, Bitcoin Core's copy: the block cipher alone

**Files:**
- Create (superproject): the submodule `third_party/libs/bitcoin` (`bitcoin/bitcoin` at v31.1,
  shallow; only `src/crypto/ctaes` is used).
- Modify (superproject): `.gitmodules`, `third_party/Jamfile` (the target `ctaes`),
  `tools/test/third_party_test.py`, CODING-GUIDELINES.md chapter 10, "Third-party code" (a
  `ctaes` row: `src/crypto/ctaes` of `bitcoin/bitcoin` v31.1 at `9be056a`, the submodule
  `third_party/libs/bitcoin`, shallow, MIT; its `src/secp256k1/` subtree is never on an include
  path and no glob over `third_party/libs` reaches it), `doc/releases/0.1.0.adoc`.
- Create (library): `LICENSE-CTAES.txt` (a copy of `src/crypto/ctaes/COPYING` at the pin,
  MIT), `src/canary_ctaes.c`, `include/webcpp/crypto/backends/ctaes.hpp`,
  `test/aes_ctaes_test.cpp`, `test/agreement_aes_test.cpp`,
  `test/messages/ctaes_not_linked/`, `test/messages/gcm_on_ctaes/`,
  and, fixtures of Task 10's test under `tools/test/fixtures/crypto_jamfiles/`: `block/`, `sign/`,
  `sign_small/`, `encrypt_and_sign/`, `canary_secp256k1_missing/`, `canary_secp256k1_small/`,
  `canary_ctaes_missing/` (their text on clang, GNU ld and wasm-ld).
- Modify (library): `include/webcpp/crypto/detail/canary.h`, `include/webcpp/crypto.hpp`,
  `test/Jamfile`, `test/c_suites/Jamfile` (ctaes's `test.c`), `test/unused/Jamfile`,
  `test/messages/Jamfile` (with the MSVC cases of `gcm_on_ctaes` and of the canaries, below),
  `doc/changelog.adoc`, `doc/block.adoc`, `doc/jamfile.adoc`, `doc/backends.adoc`, and a
  new `doc/secp256k1.adoc`.

**Interfaces:**
- Produces: `/third_party//ctaes`; `backends::ctaes` (`name = "ctaes"`); `block_state` and
  `block_engine` for ctaes and 16, 24, 32; `gcm_state<backends::ctaes, K>` holding the
  refusal of spec 2.4; the canary `webcpp_crypto_ctaes_9be056a_default__add_third_party_ctaes_to_your_link`.

- [ ] **Step 1: Add the submodule** (spec 3.4), shallow from the first clone (m10: Bitcoin
  Core's whole history is several hundred megabytes, and `shallow = true` in `.gitmodules`
  reaches later clones only), Task 9's and Task 28's form. It is named after the repository it
  holds: the checkout at the pin is Bitcoin Core's whole tree (2,923 files, about 49 MB, no
  nested submodules), of which webcpp compiles `src/crypto/ctaes/ctaes.c` alone; its own copy
  of libsecp256k1 under `src/secp256k1/` is never on an include path and never built, so the
  program still holds one libsecp256k1, `/third_party//secp256k1`'s (CODING-GUIDELINES.md
  chapter 5 rule 10):

```bash
git submodule add --depth 1 https://github.com/bitcoin/bitcoin.git third_party/libs/bitcoin
git -C third_party/libs/bitcoin fetch --depth 1 origin 9be056a8a72b624dae9623b2f7bded92c2a21c91
git -C third_party/libs/bitcoin checkout 9be056a8a72b624dae9623b2f7bded92c2a21c91
git config -f .gitmodules submodule.third_party/libs/bitcoin.shallow true
git -C third_party/libs/bitcoin rev-parse HEAD:src/crypto/ctaes
```

  Expected: `1b6c31139a71f80245c09597c343936a8e41d021`.
- [ ] **Step 2: Write the failing tests**: in `third_party_test.py`, the target compiles 2
  files (`src/crypto/ctaes/ctaes.c` and the canary) and defines
  `webcpp_crypto_ctaes_9be056a_default__add_third_party_ctaes_to_your_link`;
  `test/aes_ctaes_test.cpp` calls Task 12's `check_fips197`, `check_aes_kat`,
  `check_aes_guards` and `check_aes_moved_from` for `ctaes`, on every target;
  `test/agreement_aes_test.cpp` (native, `<library>/webcpp/crypto//openssl`, the
  source-built OpenSSL): ctaes against OpenSSL on 10,000 random keys of each
  size and random messages of 0 to 64 blocks, from a seeded generator, encrypt and decrypt
  byte-identical (spec 8.2's second row); the messages `ctaes_not_linked` (spec 4.3's fifth
  row, `<define>` none and the source naming `aes256<backends::ctaes>` after including
  `backends/ctaes.hpp`) and `gcm_on_ctaes` (the source makes a `gcm<aes256<backends::ctaes>>`
  and states spec 2.4's text,
  `webcpp::crypto: gcm<aes256<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: gcm<aes256<backends::bearssl>> or gcm<aes256<backends::openssl>>.`;
  spec 2.4's text names a third backend, which increment 5 adds back to the message with
  it). On the MSVC lanes the same source is a `webcpp.compile-fail` case (it fails to compile;
  its text is checked on clang only). Run them. Expected: FAIL.
- [ ] **Step 3: Write the target** in `third_party/Jamfile`:

```jam
# ctaes (MIT), Pieter Wuille's constant-time AES as Bitcoin Core v31.1 ships it
# in src/crypto/ctaes (the owner's "ctaes do Bitcoin Core", spec section 3.4):
# the submodule libs/bitcoin is Bitcoin Core's tree at the tag, shallow, and
# its one file of the library is compiled. Only src/crypto/ctaes is on an
# include path: the tree's own libsecp256k1 (src/secp256k1) is never reached.
path-constant BITCOIN : libs/bitcoin ;

rule bitcoin-checked-out ( properties * )
{
    if ! [ path.exists [ path.native $(BITCOIN)/src/crypto/ctaes/ctaes.h ] ]
    {
        errors.user-error "third_party/libs/bitcoin is not checked out: it holds no"
            "src/crypto/ctaes/ctaes.h. Check it out with `git submodule update --init"
            "--depth 1 third_party/libs/bitcoin`." ;
    }
}

lib ctaes : libs/bitcoin/src/crypto/ctaes/ctaes.c $(CRYPTO)/src/canary_ctaes.c
  : $(.c-library) <conditional>@bitcoin-checked-out
    <include>libs/bitcoin/src/crypto/ctaes
  : : <include>libs/bitcoin/src/crypto/ctaes <define>WEBCPP_CRYPTO_HAS_CTAES=1 $(.gc) ;
explicit ctaes ;
```

  `bitcoin-checked-out` is written as Task 5's `bearssl-checked-out` is. `canary_ctaes.c` is the prototype's
  (configuration `default`), with
  `WEBCPP_CRYPTO_CTAES_COMMIT 9be056a` in `detail/canary.h` beside the others, under its
  `_CANARY_COMMIT` seam.
- [ ] **Step 4: Write `backends/ctaes.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The backend ctaes: Bitcoin Core's constant-time AES, built from its source by
 `/third_party//ctaes`. It serves `aes128/192/256<backends::ctaes>`, the block cipher
 alone, on every target.

 @see "The backends", in the guide.
*/
#ifndef WEBCPP_CRYPTO_BACKENDS_CTAES_HPP
#define WEBCPP_CRYPTO_BACKENDS_CTAES_HPP

#include <webcpp/crypto/config.hpp>

#if !defined(WEBCPP_CRYPTO_HAS_CTAES)
#error "webcpp::crypto: the backend ctaes is used and not linked. In the program's Jamfile, add /third_party//ctaes beside /webcpp/crypto//crypto. (without b2: define WEBCPP_CRYPTO_HAS_CTAES=1 and link ctaes.c with webcpp's src/canary_ctaes.c)"
#else

#include <webcpp/crypto/detail/canary.h>
#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/detail/wipe.hpp>

#include <boost/system/result.hpp>

// ctaes.h declares its functions without extern "C" (Bitcoin Core wraps it
// itself, src/crypto/aes.cpp:9-10).
extern "C" {
#include <ctaes.h>
}

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#if defined(WEBCPP_CRYPTO_CTAES_CANARY_CONFIGURATION)
#define WEBCPP_CRYPTO_DETAIL_CTAES_CONFIGURATION WEBCPP_CRYPTO_CTAES_CANARY_CONFIGURATION
#else
#define WEBCPP_CRYPTO_DETAIL_CTAES_CONFIGURATION default
#endif
#define WEBCPP_CRYPTO_DETAIL_CTAES_CANARY \
    WEBCPP_CRYPTO_DETAIL_CANARY(ctaes, WEBCPP_CRYPTO_CTAES_COMMIT, WEBCPP_CRYPTO_DETAIL_CTAES_CONFIGURATION)

#if defined(_MSC_VER)
#pragma detect_mismatch("webcpp_crypto_backend_ctaes",                                   \
                        WEBCPP_CRYPTO_DETAIL_MISMATCH_VALUE(ctaes, WEBCPP_CRYPTO_CTAES_COMMIT, \
                                                            WEBCPP_CRYPTO_DETAIL_CTAES_CONFIGURATION))
#endif

extern "C" const char WEBCPP_CRYPTO_DETAIL_CTAES_CANARY;

namespace webcpp::crypto::backends {

/** The backend ctaes (spec section 3.4). */
struct ctaes {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "ctaes";
};

}  // namespace webcpp::crypto::backends

namespace webcpp::crypto::detail {

/** ctaes's context and functions for a key size. */
template <std::size_t KeySize>
struct ctaes_of;
template <>
struct ctaes_of<16> {
    using context = AES128_ctx;
    static void init(context* c, unsigned char const* key) { AES128_init(c, key); }
    static void encrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES128_encrypt(c, n, out, in); }
    static void decrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES128_decrypt(c, n, out, in); }
};
template <>
struct ctaes_of<24> {
    using context = AES192_ctx;
    static void init(context* c, unsigned char const* key) { AES192_init(c, key); }
    static void encrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES192_encrypt(c, n, out, in); }
    static void decrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES192_decrypt(c, n, out, in); }
};
template <>
struct ctaes_of<32> {
    using context = AES256_ctx;
    static void init(context* c, unsigned char const* key) { AES256_init(c, key); }
    static void encrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES256_encrypt(c, n, out, in); }
    static void decrypt(context const* c, std::size_t n, unsigned char* out, unsigned char const* in) { AES256_decrypt(c, n, out, in); }
};

/** The block cipher's state on ctaes: the key schedule, inside the object. */
template <std::size_t KeySize>
struct block_state<backends::ctaes, KeySize> {
    typename ctaes_of<KeySize>::context schedule;
};

/**
 * The block cipher's work on ctaes, in place: ctaes reads each block into its state before
 * it writes the block out (`ctaes.c`, `AES_encrypt`), so the same pointer is both.
 */
template <std::size_t KeySize>
struct block_engine<backends::ctaes, KeySize> {
    using state = block_state<backends::ctaes, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_CTAES_CANARY);
        ctaes_of<KeySize>::init(&s.schedule, key.data());
        return {};
    }

    static boost::system::result<void> encrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_CTAES_CANARY);
        ctaes_of<KeySize>::encrypt(&s.schedule, blocks.size() / 16, blocks.data(), blocks.data());
        return {};
    }

    static boost::system::result<void> decrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_CTAES_CANARY);
        ctaes_of<KeySize>::decrypt(&s.schedule, blocks.size() / 16, blocks.data(), blocks.data());
        return {};
    }

    static void move(state& to, state& from) noexcept {
        to = from;
        wipe_object(from);
    }

    static void reset(state& s) noexcept { wipe_object(s); }
};

/** ctaes exports no GCM, and webcpp builds none on another library's AES (spec section 2.4). */
template <std::size_t KeySize>
struct gcm_state<backends::ctaes, KeySize> {
    static_assert(KeySize != 16, "webcpp::crypto: gcm<aes128<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: gcm<aes128<backends::bearssl>> or gcm<aes128<backends::openssl>>.");
    static_assert(KeySize != 24, "webcpp::crypto: gcm<aes192<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: gcm<aes192<backends::bearssl>> or gcm<aes192<backends::openssl>>.");
    static_assert(KeySize != 32, "webcpp::crypto: gcm<aes256<backends::ctaes>>: ctaes exports the AES block cipher alone, and webcpp builds no GCM on another library's AES. Use a backend that exports GCM: gcm<aes256<backends::bearssl>> or gcm<aes256<backends::openssl>>.");
};

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_HAS_CTAES
#endif  // WEBCPP_CRYPTO_BACKENDS_CTAES_HPP
```

  `gcm<aes256<backends::ctaes>>` refuses where it is instantiated, as a member's type, so
  that the class does not compile (spec 2.4); `aes256<backends::ctaes>` stays complete.
  `include/webcpp/crypto.hpp` gains `backends/ctaes.hpp` under `WEBCPP_CRYPTO_HAS_CTAES`.
  The engine's in-place call passes the same pointer as input and output: the test's FIPS
  and AESAVS cases pin it on every target.
- [ ] **Step 5: ctaes's own test** (spec 8.1): in `test/c_suites/Jamfile`,
  `webcpp.run ctaes_test : $(CTAES)/test.c : <library>/third_party//ctaes <warnings>off <warnings-as-errors>off ;`
  with `path-constant CTAES : ../../../../third_party/libs/bitcoin/src/crypto/ctaes ;`, in the
  lane `c-suites` and also in the ordinary lanes (spec 8's table puts ctaes's `test.c` on the
  fast set): it is declared a second time outside the lane as `ctaes_test_fast`.
- [ ] **Step 6: The rest of increment 2's cases:**
  - `test/unused/Jamfile`: `webcpp.same-program unused_ctaes : app.cpp : /third_party//ctaes : : <library>/webcpp/crypto//crypto ;`;
  - (Deferred after v0.1: the signing program in the size table, both tables, with Task 22.)
  - the user's Jamfiles of spec 7.4 and 7.5, fixtures of Task 10's test, each with its
    outcome: `sign` (`exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1 ;`,
    `targets: native wasip2 wasip3 emscripten`, `schnorr ok`), `sign_small` (the same with `: <webcpp-secp256k1-tables>small ;`),
    `encrypt_and_sign` (`exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl /third_party//secp256k1 ;`,
    a program that seals and signs, printing `aes256_gcm ok backend=bearssl` and
    `schnorr ok`), `block` (`exe block : block.cpp /webcpp/crypto//crypto /third_party//ctaes ;`,
    `ctaes aes256 ok`); each of their programs also compiled in `test/Jamfile` (Task 24's
    `doc_*` lines, m21);
  - the canary cases of Task 18 for libsecp256k1 (missing; other configuration with
    `WEBCPP_CRYPTO_SECP256K1_CANARY_CONFIGURATION=w2c2x5` against the large tables, the
    case CR-REVIEW M12 named) and ctaes (missing), their text checked on clang, GNU ld and
    wasm-ld as Task 18 measured them; on the MSVC lanes each is a `webcpp.link-fail` case in
    `test/messages/Jamfile` (it fails to link), and spec 2.4's assertion a
    `webcpp.compile-fail` case (Step 2), every push, their text not checked there
    (CODING-GUIDELINES.md chapter 19 records the limit, Task 25);
  - the documentation: `doc/secp256k1.adoc` (spec 2.8 for a user: the shape, the context in
    the user's resource and its randomisation, the static context and its self-test,
    `ecdh`'s secret, high-S refused, every error, the whole C API reachable, one copy per
    program), `doc/block.adoc` completed with ctaes, `doc/jamfile.adoc` with 7.4 and 7.5,
    `doc/backends.adoc` with ctaes and libsecp256k1 (their tables, licences, allocation:
    none, and the signing context in the user's resource).
- [ ] **Step 7: Run** the four lanes, the own lane `c-suites` on its targets,
  `python3 tools/test/third_party_test.py`, `python3 tools/test/crypto_jamfiles_test.py`
  and the doc. Expected: PASS.
- [ ] **Step 8: Commit** (superproject, with the chapter 10 row and the line of
  `doc/releases/0.1.0.adoc`) `"third_party: ctaes, as Bitcoin Core v31.1 ships it"`;
  (library, with its line of `doc/changelog.adoc`) `"crypto: the AES block cipher on ctaes, GCM refused on it, and increment 2's
  pages"`.

## Task 30: Increment 2 whole: green, review, pull requests

- [ ] **Step 1: Open the pull requests**, in this order. The library's, on `webcpporg/crypto`:
  Tasks 26 to 29's commits of `libs/crypto`. Then the superproject's, rebased onto
  `origin/main` (`git rebase origin/main` on the branch, never a merge): the submodules
  `third_party/libs/secp256k1`, `third_party/libs/bips` and `third_party/libs/bitcoin` with
  their `.gitmodules` entries, `third_party/Jamfile` (the feature `webcpp-secp256k1-tables`,
  the targets `secp256k1`, `bips` and `ctaes`), `tools/test/third_party_test.py`, the three
  rows of CODING-GUIDELINES.md chapter 10, "Third-party code", the lines of
  `doc/releases/0.1.0.adoc`, and the new pin of `libs/crypto` at the library's merged commit.
  The superproject's pull request records the library's pin only after the library's has
  merged.
- [ ] **Step 2: Green is the CI's verdict on both**, every lane it plans for them, each on
  every target it names: the ordinary lanes (the native compilers, MSVC 14.3 and 14.5
  included, wasip2 and wasip3 through `tools/boost/asio-wasip2/run.sh` and
  `tools/boost/asio-wasip3/run.sh`, and emscripten), the own lane `c-suites` on native,
  wasip2, wasip3 and emscripten (within the own-lanes job's 60 minutes), the docs, the lint
  (`lint: clean`), and the tools job (`third_party_test.py`, `crypto_jamfiles_test.py`). A
  lane that fails, flakes or warns is fixed in the pull request, whatever caused it.
- [ ] **Step 3:** Dispatch the review of increment 2, with the Review Focus and spec sections
  2.3, 2.8, 3.4, 3.5, 8.
- [ ] **Step 4:** Fix what it finds, each with a failing test first, on the same pull
  requests; then each commit's message is checked: `git log -1 --format=%B | git
  interpret-trailers --parse` prints nothing. The owner merges the library's pull request,
  then the superproject's.

## Task 30a: trystero's build onto `/third_party//secp256k1`

trystero's code is unchanged: only where its libsecp256k1 comes from. It starts after Task 30's
superproject pull request has merged, so that `/third_party//secp256k1` and `libs/crypto` (whose
`src/canary_secp256k1.c` the target compiles) are on `origin/main`. Increment 3 (Tasks 31 and 32)
still moves trystero's code onto webcpp::crypto later; Task 32 Step 3's build half is this task.

**Files:**
- Modify (trystero, `libs/trystero` at its branch off its `main`, pinned today at `e9326e9`):
  `build.jam` (lines 13-40 at `e9326e9`: the comment "Only libsecp256k1 is externally
  installed", the two `webcpp.external` calls `secp256k1-root` and
  `secp256k1-emscripten-root`, the three `lib secp256k1` alternatives and their `explicit`,
  and the rule `trystero-dependencies`; the alias `trystero` loses
  `<conditional>@trystero-dependencies` and `<library>secp256k1` and gains
  `<library>/third_party//secp256k1`), `test/provider/Jamfile` (line 11's comment, "Keep
  ownership with Trystero so CI provisions its external secp256k1"), `.github/workflows/ci.yml`
  (line 8's comment), `README.md` (the libsecp256k1 rows and section "5. Build libsecp256k1"),
  `AGENTS.md` ("The dependencies", line 54 onward), `doc/building.adoc` (the libsecp256k1 row,
  its `-s` options, its MSVC paragraph and its CMake build, lines 12-60 and 123-153),
  `doc/roadmap.adoc` (line 24, "libsecp256k1 from webcpp's own build", done),
  `doc/testing.adoc` (line 265), `doc/overview.adoc` (line 73), `doc/changelog.adoc`.
- Modify (superproject): `tools/ci/matrix.py` (`EXTERNAL_DEPENDENCIES = {}`,
  `EXTERNAL_VARIABLES = ()`, their comment saying that no library needs one today; the
  machinery stays for a later library), `tools/ci/matrix_test.py`, `tools/ci/actions_test.py`
  (the secp256k1 action's cases, from `SECP256K1 = CI / 'actions/secp256k1/install.sh'` on),
  `.github/workflows/library.yml` (the anchor `&secp256k1` and its uses, the step that writes
  `SECP256K1_ROOT` and `SECP256K1_EMSCRIPTEN_ROOT`, and the comments on libsecp256k1 remaining
  external), `AGENTS.md` (the prerequisites, the `.local` table, the actions list and the
  sentences on `-sSECP256K1_ROOT`), `README.md` (trystero's external library, the "left out"
  example), CODING-GUIDELINES.md (the bullets that record the external provisioning: chapter
  4's "A library webcpp does not build yet (currently trystero's libsecp256k1)", trystero's
  design bullets on libsecp256k1's provisioning and its MSVC library in chapter 15, chapter
  16's "External libraries built from `third_party/`", chapter 17's "One copy per program"
  bullet, and chapter 19's "Bringing a C library (chapter 5)" entry, each rewritten to the
  fact that trystero links `/third_party//secp256k1`), `doc/releases/0.1.0.adoc` ("Tools and
  build"), and `libs/trystero`'s pin.
- Delete (superproject): `tools/ci/actions/secp256k1/` (`action.yml`, `install.sh`).

**Interfaces:**
- Consumes: `/third_party//secp256k1` and `webcpp-secp256k1-tables` (Task 26).
- Produces: `/webcpp/trystero//trystero` with libsecp256k1 from webcpp's build; no library of
  `libs/` needs an external library (`matrix.py external` prints `false`).

- [ ] **Step 1: Write the failing tests.** In `tools/ci/matrix_test.py`: the real tree's
  `matrix.py external --library trystero` prints `false`, `matrix.py external` prints `false`,
  and the plan of trystero's lanes says `"external": false` on every lane and own lane, with
  no `webcpp-require-external=on` and no `-sSECP256K1_ROOT` in their commands. In
  `tools/ci/actions_test.py`: no workflow step uses `./tools/ci/actions/secp256k1`, and the
  directory does not exist. Run `python3 tools/ci/matrix_test.py` and
  `python3 tools/ci/actions_test.py`. Expected: FAIL (`external --library trystero` prints
  `true`; `library.yml` uses the action).
- [ ] **Step 2: trystero's build.** In `libs/trystero/build.jam`, the dependency block becomes:

```jam
# libsecp256k1 (every room) is webcpp's build of it, /third_party//secp256k1:
# v0.8.0 from the submodule third_party/libs/secp256k1, with the tables the
# feature webcpp-secp256k1-tables chooses (small on emscripten, large natively;
# CODING-GUIDELINES.md chapter 5). Native WebRTC and OpenSSL are pinned source
# dependencies too, built together for the consumer's B2 variant.
```

  and the alias `trystero` names `<library>/third_party//secp256k1` in place of
  `<library>secp256k1`, without `<conditional>@trystero-dependencies` (the alias keeps its
  `tag::trystero` region, which the documentation includes). `SECP256K1_STATIC` comes from
  the target's usage requirements on MSVC (Task 26). Build trystero's test directory natively
  with no `-sSECP256K1_ROOT` and nothing in `.local/secp256k1-*`. Expected: PASS, where it
  failed before with "trystero is left out: libsecp256k1 was not found" under
  `webcpp-require-external=on`.
- [ ] **Step 3: The tables' consequence, documented.** trystero's external build was
  window 4 and comb 2x5 on both targets (`-DSECP256K1_ECMULT_WINDOW_SIZE=4
  -DSECP256K1_ECMULT_GEN_KB=2`, `tools/ci/actions/secp256k1/install.sh`, trystero's
  `doc/building.adoc:143`). With `auto` it becomes window 15 and comb 11x6 natively (a larger
  program, faster signing and verification) and window 2 and comb 2x5 on emscripten (the
  same signing, verification a little slower than window 4, a smaller program). A program
  that wants otherwise asks for `<webcpp-secp256k1-tables>small` or `large`. `doc/building.adoc`
  says so in place of its libsecp256k1 rows and CMake build, `doc/changelog.adoc` gains the
  line, and `doc/roadmap.adoc` drops the item.
- [ ] **Step 4: The superproject's CI.** `EXTERNAL_DEPENDENCIES = {}` and
  `EXTERNAL_VARIABLES = ()` in `tools/ci/matrix.py`; `library.yml` loses the secp256k1 steps
  (its anchor `&secp256k1`, the `*secp256k1` alias, the step of the docs and lint jobs, and
  the loop over `SECP256K1_ROOT SECP256K1_EMSCRIPTEN_ROOT`); `tools/ci/actions/secp256k1/` is
  deleted; `actions_test.py` loses the action's cases; AGENTS.md, README.md and
  CODING-GUIDELINES.md are rewritten as listed above; `doc/releases/0.1.0.adoc` gains, under
  "Tools and build", that trystero links webcpp's libsecp256k1 and the CI action
  `secp256k1` is gone. Run `python3 tools/ci/matrix_test.py`, `python3 tools/ci/actions_test.py`,
  `python3 tools/test/external_test.py` and the lint. Expected: PASS, `lint: clean`.
- [ ] **Step 5: Commit** (trystero, with its `doc/changelog.adoc` line) `"trystero:
  libsecp256k1 from webcpp's third_party build"`; (superproject) `"tools/ci: libsecp256k1 is no
  external library any more"`. After each, `git log -1 --format=%B | git interpret-trailers
  --parse` prints nothing.
- [ ] **Step 6: The pull requests and green.** First trystero's, on `webcpporg/trystero`: its
  CI (`library.yml@main`) runs every lane trystero declares, the native compilers (MSVC 14.3
  and 14.5 included) and emscripten, and its own lanes (`oracle`, `interop`,
  `provider-integration`); green is all of them, with no secp256k1 step left that it needs.
  The owner merges it. Then the superproject's, rebased onto `origin/main`, recording
  trystero's merged commit as `libs/trystero`'s pin, with Step 4's changes; green is every
  lane of its CI, the tools job included. The owner merges it.
---

# Increment 3: trystero's migration

**Deferred after version 0.1 (revision of 2026-11-14).** Kept as written; before it starts,
re-read it against the tree of that day. Already true at the revision: trystero's
shared-connection change has integrated (the precondition below holds, but its table of call
sites must still be re-read); trystero links `/third_party//secp256k1` after Task 30a, so Task
32's Step 3 (the build) is done and only its code half remains; OpenSSL and libdatachannel are
source-built, so `EXTERNAL_DEPENDENCIES` holds neither; Task 33 belongs to the memory option;
the identity checks and fixed commit identity give way to the Global Constraints' commit rule;
`~/.buzz` paths name private notes that are not carried.

trystero is changed in its own repository, `libs/trystero`, on a branch `crypto` of it
(made in the precondition below, from trystero's `main` after the shared-connection track),
and its commits are `trystero: <what>`. Every
test of trystero passes before each commit: its four lanes (native, gcc-15, emscripten, and
the own lanes `oracle` and `interop`, the second with Chrome), as its AGENTS.md lists them.

**Precondition (I17): trystero's shared-connection track has integrated.** That track
(`~/.buzz/PLANS/WEBCPP_TRYSTERO_SHARED_SPEC.md`, approved in advance, DESIGN 2026-10-11
"three documents approved in advance"; its plan `~/.buzz/PLANS/WEBCPP_TRYSTERO_SHARED_PLAN.md`,
worktree `~/dev/webcpp-trystero-shared`, superproject branch `trystero-shared`, trystero's
branch `shared-connection`) moves connections, self ids and negotiations from the room to an
`app`, in `room.hpp`, `signaling.hpp` and the backend interface, which this increment changes
too. It goes first ("The base, the order of the tracks, and the rebase points", point 5).
Before Task 31 starts:
1. `git -C ~/dev/webcpp log --oneline -1 main` shows the superproject's integration of that
   track (its plan's last task), and `git -C libs/trystero merge-base --is-ancestor
   origin/main HEAD` holds after `git -C libs/trystero fetch origin`; if the track has not
   integrated, this increment waits, and the ledger says so (increments 4 and 5 do not
   depend on it and may go first).
2. This worktree rebases onto `main`
   (`git -c user.name=R -c user.email=github.overboard312@passmail.net rebase main`, then
   `git submodule update --init libs/trystero`), runs the identity check of "The base, the
   order of the tracks", point 3, here and in `libs/trystero` (expected `identity: ok` in both, no
   `stop` line), and the branch `crypto` of trystero is made from trystero's `main` after the
   rebase (`git -C libs/trystero switch -c crypto origin/main`).
3. Task 31's Step 4 table of call sites is re-read against the rebased files: each row names
   today's function (`room.hpp`'s `start()` and `random_id`, `signaling.hpp`'s seal and open
   and `make_id`, `backend::factory`'s `make_sealer` and `random`), and each is mapped to
   where the track put it (the `app`, a connection, the room) by `git grep -n` of the row's
   call (`make_sealer(`, `random(`, `make_id(`) in `libs/trystero/include`; the table is
   rewritten with the new places before Step 4 runs, and a row whose call the track
   removed is struck with the reason. The ledger records the base's commit and the
   rewritten table.

## Task 31: trystero seals with `aes256_gcm` directly, and draws from `random_device`

**Files (in `libs/trystero`):**
- Create: `include/webcpp/trystero/detail/sealer.hpp`, `test/sealer_test.cpp`.
- Delete: `include/webcpp/trystero/native/sealer.hpp`, `test/native_sealer_test.cpp`,
  `test/browser_sealer_test.cpp`.
- Modify: `include/webcpp/trystero/backend.hpp` (no `sealer`, no `make_sealer`; `random`
  returns a result), `include/webcpp/trystero/native/backend.hpp`,
  `include/webcpp/trystero/browser/backend.hpp` (its `sealer` class removed, `random` on
  `random_device`), `include/webcpp/trystero/room.hpp`,
  `include/webcpp/trystero/detail/signaling.hpp`,
  `include/webcpp/trystero/detail/nostr_strategy.hpp`, `test/support/memory_backend.hpp`
  (no `memory_sealer`; `random` with a failure seam), `test/Jamfile`,
  `test/CONVERSION.md`, `build.jam`, `doc/native.adoc`, `doc/browser.adoc`,
  `doc/design.adoc`, `doc/allocation.adoc`, `doc/testing.adoc`, `doc/overview.adoc`,
  `doc/guarantees.adoc`.
- Modify (superproject): `tools/test/third_party_test.py` (the link-line case of Step 5, I14).

**Interfaces:**
- Consumes: `webcpp::crypto::aes256_gcm`, `webcpp::crypto::random_device`.
- Produces: `class webcpp::trystero::detail::sealer` with
  `static boost::system::result<sealer> make(std::array<std::uint8_t, 32> key, backend::factory& factory, std::pmr::memory_resource* memory)`,
  `boost::system::result<std::pmr::string> seal(std::string_view plaintext)`,
  `boost::system::result<std::pmr::string> open(std::string_view sealed)`;
  `virtual boost::system::result<void> backend::factory::random(std::span<std::uint8_t> out) = 0;`.

- [ ] **Step 1: Write the failing test** `test/sealer_test.cpp`, on the memory backend's
  factory (seeded), on native and emscripten (trystero's targets): a text sealed under a
  room key opens to itself; the sealed form is Trystero's (`protocol::parse_sealed` reads a
  16-byte IV and the ciphertext with its 16-byte tag); a text sealed under another key, a
  changed byte, a text that is not a sealed text and an IV of another size give
  `trystero::error::wrong_password` (what `open` reported before); a vector sealed by
  Trystero's JavaScript (the one `native_sealer_test.cpp` held, moved here) opens; a factory
  whose `random` fails makes `seal` fail with `webcpp::crypto::error::entropy_unavailable`.
  The two deleted tests' cases are each carried here or named in `CONVERSION.md` as
  replaced, case by case (AGENTS chapter 9, "a test moved ... records the conversion"). In
  `test/Jamfile`, `webcpp.run sealer : sealer_test.cpp ;` and its `_no_exceptions` twin, in
  place of `native_sealer` and `browser_sealer` and their twins. Run trystero's native lane.
  Expected: FAIL, `detail/sealer.hpp` not found.
- [ ] **Step 2: Write `detail/sealer.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TRYSTERO_DETAIL_SEALER_HPP
#define WEBCPP_TRYSTERO_DETAIL_SEALER_HPP

#include <webcpp/trystero/config.hpp>

#include <webcpp/crypto/aes256_gcm.hpp>
#include <webcpp/trystero/backend.hpp>
#include <webcpp/trystero/error.hpp>
#include <webcpp/trystero/protocol/signal.hpp>

#include <boost/system/result.hpp>

#include <array>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

namespace webcpp::trystero::detail {

/**
 * AES-256-GCM under one room key, in Trystero's sealed text form (protocol/signal.hpp), what
 * Trystero's `encrypt` and `decrypt` do with `crypto.subtle`: webcpp::crypto's `aes256_gcm`
 * on the default backend (OpenSSL natively when the native backend links it, BearSSL in the
 * browser), with a fresh 16-byte IV of the factory's for each text. Synchronous: a seal or an
 * open is a call, which allocates only its text in the room's resource.
 */
class sealer {
public:
    /**
     * Makes the sealer under a room's key.
     *
     * @param key The raw AES-256 key, `protocol::seal_key`'s.
     * @param factory What draws the IVs; it outlives the sealer.
     * @param memory The room's resource, which the texts are allocated in.
     * @return The sealer, or the backend's error from `aes256_gcm::make`.
     */
    static boost::system::result<sealer> make(std::array<std::uint8_t, 32> const& key,
                                              backend::factory& factory,
                                              std::pmr::memory_resource* memory) {
        auto aead = webcpp::crypto::aes256_gcm::make(key);
        if (!aead) {
            return aead.error();
        }
        return sealer(std::move(*aead), factory, memory);
    }

    /**
     * Seals a text under a fresh IV.
     *
     * @param plaintext The text.
     * @return The sealed text; the factory's error when it has no entropy
     * (`webcpp::crypto::error::entropy_unavailable`), or the backend's.
     */
    boost::system::result<std::pmr::string> seal(std::string_view plaintext) {
        std::array<std::uint8_t, protocol::seal_iv_size> iv{};
        auto const drawn = factory_->random(iv);
        if (!drawn) {
            return drawn.error();
        }
        backend::bytes data(plaintext.size() + 16, memory_);
        std::copy(plaintext.begin(), plaintext.end(), data.begin());
        auto const tag = aead_.seal(iv, {}, std::span(data).first(plaintext.size()));
        if (!tag) {
            return tag.error();
        }
        std::copy(tag->begin(), tag->end(), data.begin() + static_cast<std::ptrdiff_t>(plaintext.size()));
        return protocol::format_sealed(iv, data, memory_);
    }

    /**
     * Opens a sealed text.
     *
     * @param sealed The sealed text.
     * @return The plaintext; `trystero::error::wrong_password` when the text was sealed under
     * another key or is not a sealed text.
     */
    boost::system::result<std::pmr::string> open(std::string_view sealed) {
        auto parts = protocol::parse_sealed(sealed, memory_);
        if (!parts || parts->iv.empty() || parts->ciphertext.size() < 16) {
            return make_error_code(error::wrong_password);
        }
        auto const body = std::span(parts->ciphertext).first(parts->ciphertext.size() - 16);
        auto const opened = aead_.open(parts->iv, {}, body, std::span(parts->ciphertext).last(16));
        if (!opened) {
            return make_error_code(error::wrong_password);
        }
        return std::pmr::string(body.begin(), body.end(), memory_);
    }

private:
    sealer(webcpp::crypto::aes256_gcm aead, backend::factory& factory,
           std::pmr::memory_resource* memory)
        : aead_(std::move(aead)), factory_(&factory), memory_(memory) {}

    webcpp::crypto::aes256_gcm aead_;
    backend::factory* factory_;
    std::pmr::memory_resource* memory_;
};

}  // namespace webcpp::trystero::detail

#endif  // WEBCPP_TRYSTERO_DETAIL_SEALER_HPP
```

  `open` keeps what trystero reported before for every failure, `wrong_password`, as
  Trystero's `decrypt` does: an IV of any length from 1 to 128 bytes opens (crypto's cap); a
  longer or empty one is `invalid_iv_size` from crypto, which `open` reports as
  `wrong_password`, with a case in the test.
- [ ] **Step 3: Change the backend interface.** In `backend.hpp`, the class `sealer` and
  `factory::make_sealer` go, with their Doc Comments, and `random` becomes:

```cpp
    /**
     * Fills `out` with bytes from a cryptographically secure source: they become secret keys,
     * ids and IVs.
     *
     * @param out The bytes to fill.
     * @return Nothing, or why the source failed (`webcpp::crypto::error::entropy_unavailable`),
     * which fails the operation that needed the bytes.
     */
    virtual boost::system::result<void> random(std::span<std::uint8_t> out) = 0;
```

  The native backend's `random` and `make_sealer` become:

```cpp
    /**
     * Fills `out` from the system's generator, through webcpp::crypto's random device.
     *
     * @param out The bytes to fill.
     * @return Nothing, or `webcpp::crypto::error::entropy_unavailable`.
     */
    boost::system::result<void> random(std::span<std::uint8_t> out) override {
        return webcpp::crypto::random_device().fill(out);  // lint-world: Trystero's randomness
    }
```

  (and `#include <webcpp/crypto/random_device.hpp>  // lint-world: Trystero's randomness`;
  `#include <openssl/rand.h>` and `std::abort` go); the browser backend's `random` is the
  same line, its loop over `getentropy` and its `#include <sys/random.h>` gone (the result
  was ignored there, spec 6), and its class `sealer`, with `make_sealer`, goes. The memory
  backend's `random` returns `{}` after its seeded bytes, unless the test set
  `fail_random = true`, a member of `memory_network`, when it returns
  `webcpp::crypto::error::entropy_unavailable`.
- [ ] **Step 4: Change the call sites**, each failure reported through the room's errors:

| Call site | Today | Now |
| --- | --- | --- |
| `room.hpp`, `start()`: `sealer_ = factory_.make_sealer(...)` | an asynchronous sealer of the backend's | `auto made = detail::sealer::make(protocol::seal_key(...), factory_, memory_);` then, on error, `report("", made.error(), "the room's sealer could not be made");` and the join stops there (no wire, no signaling); else `sealer_ = std::allocate_shared<detail::sealer>(allocator(), std::move(*made));` |
| `room.hpp`, `random_id(n)` | ignores the source's failure | returns `boost::system::result<std::pmr::string>`; its callers (the self id when the configuration has none, an action's transfer id) report `entropy_unavailable` through `report("", ec, "no entropy for an id")` and fail what needed it: a join, a send (its completion gets the error) |
| `signaling.hpp`, `make_id(n)` | the same | the same change: an offer that cannot get an id is not made, and the negotiation with that peer is reported failed through `cb_.failed` (the callback the room gives signaling for a failed negotiation) with the error |
| `signaling.hpp`, `sealer_->seal(sdp, <completion>)` (offer, answer, candidate) | posted completion `on_*_sealed(ec, sealed)` | `auto const sealed = sealer_->seal(sdp);` then the same `on_*_sealed(peer, pc, sealed ? boost::system::error_code{} : sealed.error(), sealed ? std::string_view(*sealed) : std::string_view())` called directly, at the end of the function, where the completion was posted |
| `signaling.hpp`, `sealer_->open(text, <completion>)` (offer, answer, candidate) | posted completion `on_*_opened(ec, plaintext)` | the same, with `open` |
| `nostr_strategy.hpp`, `make_signer` | loops on `factory.random` until a key | Task 32 |
| `nostr_strategy.hpp`, `make_subscription_id` | ignores failure | returns a result; a relay that cannot subscribe is reported through the strategy's log and `report` with the error, and retried as a lost relay is |
| `nostr_strategy.hpp`, `event_for` (aux) | ignores failure | returns a result; an event that cannot get its auxiliary randomness is not published, and the failure is reported |
| `nostr_strategy.hpp`, the reconnect's jitter | ignores failure | on failure the reconnect waits the whole `retry_ms`, which needs no randomness, and the failure is reported |

  The `on_*_sealed` and `on_*_opened` functions keep their checks (`find(peer)`, the
  connection's identity), which a direct call needs as the posted one did: they read the
  state again after the call. The direct calls change one guarantee of the old backend
  interface, "a handler never runs inside the call that started it"; none of these functions
  is a user's handler, and the test of Step 6 runs the room's whole negotiation, both
  directions, on the memory backend.
- [ ] **Step 5: Name the libraries** in trystero's `build.jam`: the alias `trystero` gains
  `<library>/webcpp/crypto//crypto <library>/third_party//bearssl` in its usage requirements
  (BearSSL on every target: in the browser it is the backend, natively the default only
  without OpenSSL); the alias `native` gains `<library>/third_party//openssl` in place of its
  `<library>ssl <library>crypto` (OpenSSL is then the default natively, spec 5). That target
  is libssl and libcrypto as one choice, shared or static by
  `<webcpp-crypto-openssl-link>` (Task 11, I14), so Beast's TLS and libdatachannel's DTLS
  take libssl from it too, and a program holds one libcrypto whichever link it chose (spec
  3.7, "a process with two copies of libcrypto ... has two hooks"). trystero's own OpenSSL
  goes whole: both `lib ssl` and both `lib crypto` alternatives, the line
  `local openssl-root = [ webcpp.external trystero : OpenSSL ... ]`, and `OpenSSL` in
  `native-dependencies`' `webcpp.external-found` (the stop a user meets is then
  `/third_party//openssl`'s, Task 11's text, and a test of trystero's that pinned its own
  stop pins that one). `test/Jamfile`'s headers-alone call that named
  `trystero/native/sealer.hpp` drops it. A case of `tools/test/third_party_test.py` (the
  superproject's, in this increment's superproject commit) builds a program of
  `/webcpp/trystero//native` with `-d+2` once per link and asserts that its link line names
  libssl and libcrypto once each, both static (`.a`, or `libssl.lib` from the static
  directory with MSVC) with `static` and both shared with `shared`; written first, it fails
  on today's `build.jam` (a shared `-lssl` beside the static libcrypto).
- [ ] **Step 6: Run** trystero's lanes: native (Apple clang), gcc-15, emscripten, and the own
  lanes `oracle` and `interop` (native and in Chrome against Trystero's JavaScript, which
  proves the sealed texts are Trystero's). Expected: PASS, every test, the interop's
  handshakes included.
- [ ] **Step 7: Write the documentation:** `doc/native.adoc` and `doc/browser.adoc` (the sealer
  is webcpp::crypto's `aes256_gcm`, the backends' randomness `random_device`), `doc/design.adoc`
  (the factory makes no sealer), `doc/allocation.adoc` (the sealer allocates its texts in the
  room's resource; OpenSSL's `make` allocates outside it, the departure crypto states),
  `doc/guarantees.adoc` and `doc/testing.adoc` (`sealer` in place of the two tests), and the
  change of behaviour: natively, a machine without entropy no longer aborts; the operation
  that needed the bytes fails with `entropy_unavailable` (spec 9, increment 3).
- [ ] **Step 8: Commit** (trystero) `"trystero: AES-GCM through webcpp::crypto, a direct call,
  and randomness that reports its failure"`; (superproject)
  `"tools/test: trystero's OpenSSL is one choice of libssl and libcrypto"`.

## Task 32: trystero signs with `secp256k1::keypair`, and libsecp256k1 comes from `third_party`

**Files (in `libs/trystero`):**
- Modify: `include/webcpp/trystero/protocol/nostr.hpp` (`signer`, `verify`),
  `include/webcpp/trystero/detail/nostr_strategy.hpp` (`make_signer`), `build.jam`,
  `test/Jamfile`, `test/protocol_test.cpp`, `doc/building.adoc`, `doc/native.adoc`,
  `README.md`.
- Modify (superproject): `tools/ci/matrix.py` (`EXTERNAL_DEPENDENCIES`,
  `EXTERNAL_VARIABLES`), `tools/ci/matrix_test.py`, `.github/workflows/library.yml` (the
  `secp256k1` steps), `tools/ci/actions_test.py`; delete `tools/ci/actions/secp256k1/` when no
  other library needs it (none does).

**Interfaces:**
- Consumes: `webcpp::crypto::secp256k1::keypair`, `verify_schnorr`.
- Produces: `protocol::nostr::signer` unchanged in shape for its callers, over a `keypair`;
  `static boost::system::result<signer> signer::from_secret(std::span<std::uint8_t const, 32>, std::pmr::memory_resource*)`
  (it returned `std::optional`); `verify` unchanged.

- [ ] **Step 1: Write the failing test** in `test/protocol_test.cpp`: `signer::from_secret` of
  a zero secret gives `webcpp::crypto::error::invalid_secret_key`; a signature of a known key
  and event id, with zero auxiliary randomness, equals BIP340's (row 0 of
  `bip-0340/test-vectors.csv`: secret `00...03`, message `00...00`, aux `00...00`, signature
  `e907831f80848d1069a5371b402410364bdf1c5f8307b0084c55f1ce2dca821525f66a4a85ea8b71e482a74f382d2ce5ebeee8fdb2172f477df4900d310536c0`);
  `verify` of it is true, of a changed id false. Run. Expected: FAIL to compile,
  `from_secret` returns an optional.
- [ ] **Step 2: Move `signer` and `verify` onto crypto** (spec 9, increment 3):

```cpp
class signer {
public:
    /**
     * Makes a key from 32 random bytes.
     *
     * @param secret The secret key.
     * @param memory The resource the signing context and the public key's text are allocated in;
     * it must outlive the signer.
     * @return The signer; `webcpp::crypto::error::invalid_secret_key` when the bytes are not a
     * valid secret key, which is negligibly rare; `entropy_unavailable` when the context's
     * randomisation finds no entropy.
     */
    static boost::system::result<signer> from_secret(
        std::span<const std::uint8_t, 32> secret,
        std::pmr::memory_resource* memory = std::pmr::get_default_resource()) {
        auto key = webcpp::crypto::secp256k1::keypair::make(secret, memory);
        if (!key) {
            return key.error();
        }
        auto const xonly = key->xonly_public_key();
        return signer(std::move(*key), to_hex(*xonly, memory));
    }

    [[nodiscard]] std::string_view pubkey() const { return pubkey_; }

    [[nodiscard]] std::array<std::uint8_t, 64> sign(std::span<const std::uint8_t, 32> message,
                                                    std::span<const std::uint8_t, 32> aux) const {
        return *key_.sign_schnorr(message, aux);
    }

private:
    signer(webcpp::crypto::secp256k1::keypair key, std::pmr::string pubkey)
        : key_(std::move(key)), pubkey_(std::move(pubkey)) {}

    webcpp::crypto::secp256k1::keypair key_;
    std::pmr::string pubkey_;
};

inline bool verify(std::span<const std::uint8_t, 64> sig, std::span<const std::uint8_t, 32> id,
                   std::span<const std::uint8_t, 32> pubkey) {
    return webcpp::crypto::secp256k1::verify_schnorr(sig, id, pubkey).has_value();
}
```

  with each member's Doc Comment kept from today's, `sign` saying that a key pair made by
  `from_secret` always signs (a moved-from signer is the only `invalid_state`, and trystero
  never signs with one). `make_signer` in `nostr_strategy.hpp` returns
  `boost::system::result<protocol::nostr::signer>`: it draws 32 bytes (a failure is
  returned), retries on `invalid_secret_key`, returns any other error; the strategy's
  creation, and so the room's join, fails with it, reported as Task 31's join failures are.
  `#include <secp256k1*.h>` lines go from `nostr.hpp`.
- [ ] **Step 3: Name the library** (spec 4.5, one copy per program): trystero's `build.jam`
  loses `secp256k1-root`, `secp256k1-emscripten-root`, the three `lib secp256k1`
  alternatives and `trystero-dependencies`' check of them; the alias `trystero` gains
  `<library>/third_party//secp256k1`. In the superproject, `EXTERNAL_DEPENDENCIES` becomes
  `{'trystero': ('libdatachannel', 'openssl'), 'crypto': ('openssl',)}` and
  `EXTERNAL_VARIABLES` loses `SECP256K1_ROOT` and `SECP256K1_EMSCRIPTEN_ROOT`; the workflow's
  two `secp256k1` steps and `tools/ci/actions/secp256k1/` go, with their cases in
  `tools/ci/actions_test.py`; AGENTS.md chapter 1's prerequisites and `.local` table lose
  `secp256k1-native` and `secp256k1-emscripten`.
- [ ] **Step 4: Run** trystero's lanes, as Task 31's Step 6, then
  `python3 tools/ci/matrix_test.py`, `python3 tools/ci/actions_test.py` and the lint.
  Expected: PASS; `lint: clean`.
- [ ] **Step 5: Commit** (trystero) `"trystero: Nostr's signatures on webcpp::crypto's
  secp256k1, libsecp256k1 from third_party"`; (superproject) `"tools/ci: libsecp256k1 is no
  external library any more"`.

## Task 33: The memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake

**Files (in `libs/trystero`):**
- Create: `test/openssl_memory/Jamfile`, `test/openssl_memory/neighbours_test.cpp`,
  `test/openssl_memory/handshake_test.cpp`.
- Modify: `test/Jamfile` (`build-project openssl_memory ;`).

- [ ] **Step 1: Write the tests** (spec 8.5, "Its neighbours in the process"), each over
  crypto's `support.hpp` (`#include "../../../crypto/test/openssl_memory/support.hpp"`,
  whose counting resource, exit check and interposers they share, so that both libraries'
  tests check the option the same way):
  - `neighbours_test.cpp`: includes `<boost/asio/ssl.hpp>` and libdatachannel's `<rtc/rtc.hpp>`,
    and touches one symbol of each so the linker keeps them (`boost::asio::ssl::context` is
    not constructed; `rtc::InitLogger` is not called: their address is taken into a
    volatile pointer); `main`'s first statement is `install_memory_functions()`, which must
    succeed: neither library used OpenSSL during static initialisation; then the exit check;
  - `handshake_test.cpp`: installs, then runs a TLS 1.3 handshake through libssl in one
    process over a pair of memory BIOs (`BIO_new_bio_pair`): a server whose key and
    self-signed certificate it makes at run time (`EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256")`,
    `X509_new`, `X509_set_pubkey`, `X509_sign`), a client that does not verify, both stepped
    with `SSL_do_handshake` until both finish, one application record each way, then the
    `SSL`, `SSL_CTX`, key and certificate freed; it passes when the handshake finished, no
    foreign block was freed through the adapter, the C library's allocator was called 0 times
    while it ran (the interposers), and the exit check holds. A pair of memory BIOs stands for
    the loopback socket of spec 8.5: libssl's allocations are the same and the container lane
    needs no socket.
  `test/openssl_memory/Jamfile`: `project : requirements <library>/webcpp/trystero//native <exception-handling>on <define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE ;`
  (the native backend needs exceptions, trystero's rule), `webcpp.targets native ;`, the
  counter's sources by `<target-os>` as crypto's Jamfile names them (by path into
  `libs/crypto/test/openssl_memory/`), and `webcpp.run neighbours` and `webcpp.run handshake`,
  each also `<webcpp-crypto-openssl-link>static` as a twin. The static twins link libssl
  statically too, since `/webcpp/trystero//native` takes both from `/third_party//openssl`
  (Task 31's Step 5, I14): the handshake's libssl then calls the one libcrypto whose memory
  functions the test installed, and a shared libssl, which would bring a second libcrypto
  outside the adapter, cannot be linked by these Jamfiles (Task 31's link-line case is the
  check).
- [ ] **Step 2: Run** trystero's native lane and gcc-15's. Expected: PASS, both programs and
  their static twins.
- [ ] **Step 3: Commit** (trystero) `"trystero: OpenSSL's memory option beside Asio's SSL,
  libdatachannel and a TLS handshake"`.

## Task 34: Increment 3 whole: green, review, the gates

- [ ] **Step 1: Run the definition of green**: crypto's lanes and own lanes (Task 25's list),
  trystero's lanes and own lanes, the lint, the doc (`$B2 -a --build-dir=bin/b-doc doc`), the
  tools' tests touched.
- [ ] **Step 2:** Dispatch the review of increment 3 (trystero's branch, crypto's changes,
  the superproject's), with the Review Focus and spec section 9's increment 3.
- [ ] **Step 3:** Fix what it finds, each with a failing test first.
- [ ] **Step 4 (controller, G4):** with the owner's agreement, push crypto, trystero (merged
  into its `main` by the owner), then the superproject with both submodules moved.

---

# Increment 4: ChaCha20-Poly1305

**Deferred after version 0.1 (revision of 2026-11-14).** Kept as written; before it starts,
re-read it against the tree of that day: OpenSSL is `/webcpp/crypto//openssl` over the
source-built provider (3.5.9, also on wasip2 and wasip3), NIST is gate O3's, and the memory
option's and sizes' parts wait for Tasks 15, 16 and 22.

## Task 35: ChaCha20-Poly1305 on BearSSL and OpenSSL

**Files:**
- Create: `include/webcpp/crypto/chacha20_poly1305.hpp`, `test/support/chacha_checks.hpp`,
  `test/chacha_bearssl_test.cpp`, `test/chacha_openssl_test.cpp`,
  `test/chacha_wycheproof_bearssl_test.cpp`, `test/chacha_wycheproof_openssl_test.cpp`,
  `test/agreement_chacha_test.cpp`, `test/messages/chacha_on_ctaes/`,
  `test/sizes/chacha_bearssl.cpp`; (superproject) the fixture
  `tools/test/fixtures/crypto_jamfiles/msvc_chacha_on_ctaes/`.
- Modify: `include/webcpp/crypto/backends/bearssl.hpp`, `backends/openssl.hpp`,
  `backends/ctaes.hpp` (the refusal), `include/webcpp/crypto.hpp`, `test/Jamfile`,
  `test/support/agreement.hpp`, `test/allocation_bearssl_test.cpp`,
  `test/allocation_openssl_test.cpp`, `test/openssl_memory/everything_test.cpp`,
  `test/openssl_error_queue_test.cpp`, `test/constant_time/ctgrind_test.cpp`,
  `test/constant_time/msan_test.cpp`, `test/constant_time/dudect_test.cpp`,
  `test/sizes/wasm/Jamfile`, `test/sizes/native/Jamfile`, `doc/aead.adoc`, `doc/backends.adoc`,
  `doc/jamfile.adoc`.

**Interfaces:**
- Consumes: `detail::chacha_state`, `detail::chacha_engine` (declared in Task 6).
- Produces: `webcpp::crypto::chacha20_poly1305<Backend>` (spec 2.5); the ChaCha engine
  contract:

```cpp
template <> struct chacha_engine<B> {
    static boost::system::result<void> init(chacha_state<B>&, std::span<std::uint8_t const> key) noexcept;
    static boost::system::result<void> seal(chacha_state<B>&, std::span<std::uint8_t const> iv,
        std::span<std::uint8_t const> additional_data, std::span<std::uint8_t> data,
        boost::hash2::digest<16>& tag) noexcept;
    static boost::system::result<void> open(chacha_state<B>&, std::span<std::uint8_t const> iv,
        std::span<std::uint8_t const> additional_data, std::span<std::uint8_t> data,
        std::array<std::uint8_t, 16> const& tag) noexcept;  // authentication_failed, backend_failure
    static void move(chacha_state<B>& to, chacha_state<B>& from) noexcept;
    static void reset(chacha_state<B>&) noexcept;
};
```

- [ ] **Step 1: Write the failing checks** `test/support/chacha_checks.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The checks of chacha20_poly1305 on one backend (spec sections 2.5 and 8.1): RFC 8439's
// section 2.8.2 and appendix A.5 (M4), Wycheproof's 325 tests, the guards, tampering, a
// moved-from object, empty inputs and an overlapping tag.

#ifndef WEBCPP_TEST_CRYPTO_SUPPORT_CHACHA_CHECKS_HPP
#define WEBCPP_TEST_CRYPTO_SUPPORT_CHACHA_CHECKS_HPP

#include "gcm_checks.hpp"
#include "hex.hpp"
#include "test_data.hpp"

#include <webcpp/crypto/chacha20_poly1305.hpp>

#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace webcpp::test {

/** One RFC 8439 case; the plaintext as text. */
struct chacha_case {
    std::string_view key, iv, additional_data, plaintext, ciphertext, tag;
};

inline constexpr chacha_case rfc8439_2_8_2{
    "808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f",
    "070000004041424344454647", "50515253c0c1c2c3c4c5c6c7",
    "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the "
    "future, sunscreen would be it.",
    "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc3ff4def08e4b7a9de576d26586cec64b6116",
    "1ae10b594f09e26a7e902ecbd0600691"};

inline constexpr chacha_case rfc8439_a_5{
    "1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0",
    "000000000102030405060708", "f33388860000000000004e91",
    "Internet-Drafts are draft documents valid for a maximum of six months and may be "
    "updated, replaced, or obsoleted by other documents at any time. It is inappropriate to "
    "use Internet-Drafts as reference material or to cite them other than as "
    "/\xe2\x80\x9cwork in progress./\xe2\x80\x9d",
    "64a0861575861af460f062c79be643bd5e805cfd345cf389f108670ac76c8cb24c6cfc18755d43eea09ee94e382d26b0bdb7b73c321b0100d4f03b7f355894cf332f830e710b97ce98c8a84abd0b948114ad176e008d33bd60f982b1ff37c8559797a06ef4f0ef61c186324e2b3506383606907b6a7c02b0f9f6157b53c867e4b9166c767b804d46a59b5216cde7a4e99040c5a40433225ee282a1b0a06c523eaf4534d7f83fa1155b0047718cbc546a0d072b04b3564eea1b422273f548271a0bb2316053fa76991955ebd63159434ecebb4e466dae5a1073a6727627097a1049e617d91d361094fa68f0ff77987130305beaba2eda04df997b714d6c6f2c29a6ad5cb4022b02709b",
    "eead9d67890cbb22392336fea1851f38"};

template <class Backend>
void check_rfc8439() {
    for (auto const& c : {rfc8439_2_8_2, rfc8439_a_5}) {
        auto made = crypto::chacha20_poly1305<Backend>::make(from_hex(c.key));
        if (!BOOST_TEST(made.has_value())) {
            return;
        }
        std::vector<std::uint8_t> data(c.plaintext.begin(), c.plaintext.end());
        auto const plaintext = data;
        auto const tag = made->seal(from_hex(c.iv), from_hex(c.additional_data), data);
        BOOST_TEST_EQ(to_hex(data), c.ciphertext);
        BOOST_TEST(tag && to_hex(*tag) == c.tag);
        BOOST_TEST(made->open(from_hex(c.iv), from_hex(c.additional_data), data,
                              from_hex(c.tag)).has_value());
        BOOST_TEST(data == plaintext);
    }
}

template <class Backend>
void check_chacha_guards_and_tampering() {
    using aead = crypto::chacha20_poly1305<Backend>;
    std::array<std::uint8_t, 31> const short_key{};
    BOOST_TEST(aead::make(short_key).error() == crypto::error::invalid_key_size);
    auto const& c = rfc8439_2_8_2;
    auto made = aead::make(from_hex(c.key));
    if (!BOOST_TEST(made.has_value())) {
        return;
    }
    auto ciphertext = from_hex(c.ciphertext);
    auto const untouched = ciphertext;
    for (std::size_t size : {0, 11, 13, 16}) {
        std::vector<std::uint8_t> const iv(size);
        BOOST_TEST(made->seal(iv, {}, ciphertext).error() == crypto::error::invalid_iv_size);
        BOOST_TEST(made->open(iv, {}, ciphertext, from_hex(c.tag)).error() ==
                   crypto::error::invalid_iv_size);
    }
    std::array<std::uint8_t, 15> const short_tag{};
    BOOST_TEST(made->open(from_hex(c.iv), {}, ciphertext, short_tag).error() ==
               crypto::error::invalid_tag_size);
    BOOST_TEST(ciphertext == untouched);
    auto const iv = from_hex(c.iv);
    auto const additional_data = from_hex(c.additional_data);
    auto const tag = from_hex(c.tag);
    for (std::size_t i = 0; i < ciphertext.size(); ++i) {
        auto data = ciphertext;
        data[i] ^= 1;
        BOOST_TEST(made->open(iv, additional_data, data, tag).error() ==
                   crypto::error::authentication_failed);
        BOOST_TEST(all_zero(data));
    }
    for (std::size_t i = 0; i < tag.size() + additional_data.size(); ++i) {
        auto data = ciphertext;
        auto bad_tag = tag;
        auto bad_data = additional_data;
        (i < tag.size() ? bad_tag[i] : bad_data[i - tag.size()]) ^= 0x80;
        BOOST_TEST(made->open(iv, bad_data, data, bad_tag).error() ==
                   crypto::error::authentication_failed);
        BOOST_TEST(all_zero(data));
    }
    // An overlapping tag fails cleanly (Review Focus 2); empty inputs seal and open.
    std::array<std::uint8_t, 24> overlap{};
    BOOST_TEST(made->open(iv, {}, std::span(overlap).first(20), std::span(overlap).last(16))
                   .error() == crypto::error::authentication_failed);
    auto const empty = made->seal(iv, {}, std::span<std::uint8_t>());
    BOOST_TEST(empty && made->open(iv, {}, std::span<std::uint8_t>(), *empty).has_value());
    // A moved-from object answers invalid_state, and so does one moved from it (I1).
    aead moved = std::move(*made);
    std::array<std::uint8_t, 16> data{};
    data.fill(0x6c);
    BOOST_TEST(made->seal(iv, {}, data).error() == crypto::error::invalid_state);
    BOOST_TEST(made->open(iv, {}, data, tag).error() == crypto::error::invalid_state);
    aead again = std::move(*made);
    BOOST_TEST(again.seal(iv, {}, data).error() == crypto::error::invalid_state);
    again = std::move(*made);
    std::swap(again, moved);
    BOOST_TEST(moved.seal(iv, {}, data).error() == crypto::error::invalid_state);
    BOOST_TEST(again.seal(iv, {}, data).has_value());
}

/** Wycheproof's 325 tests at 12fd3aaf: 256 valid, 60 invalid, 9 nonces not of 12 bytes. */
template <class Backend>
void check_wycheproof_chacha() {
    auto const document = boost::json::parse(read_file(
        std::string(WEBCPP_TEST_WYCHEPROOF) + "/testvectors_v1/chacha20_poly1305_test.json"));
    std::size_t valid = 0;
    std::size_t invalid = 0;
    std::size_t other_nonce = 0;
    for (auto const& group : document.at("testGroups").as_array()) {
        for (auto const& test : group.at("tests").as_array()) {
            auto made = crypto::chacha20_poly1305<Backend>::make(from_hex(test.at("key").as_string()));
            if (!BOOST_TEST(made.has_value())) {
                return;
            }
            auto const iv = from_hex(test.at("iv").as_string());
            auto const additional_data = from_hex(test.at("aad").as_string());
            auto const message = from_hex(test.at("msg").as_string());
            auto const ciphertext = from_hex(test.at("ct").as_string());
            auto const tag = from_hex(test.at("tag").as_string());
            auto data = ciphertext;
            auto const opened = made->open(iv, additional_data, data, tag);
            if (iv.size() != 12) {
                BOOST_TEST(opened.error() == crypto::error::invalid_iv_size);
                ++other_nonce;
            } else if (test.at("result").as_string() == "valid") {
                BOOST_TEST(opened.has_value() && data == message);
                auto sealed = message;
                auto const sealed_tag = made->seal(iv, additional_data, sealed);
                BOOST_TEST(sealed == ciphertext && sealed_tag && to_hex(*sealed_tag) == to_hex(tag));
                ++valid;
            } else {
                BOOST_TEST(opened.error() == crypto::error::authentication_failed);
                BOOST_TEST(all_zero(data));
                ++invalid;
            }
        }
    }
    BOOST_TEST_EQ(valid, 256U);
    BOOST_TEST_EQ(invalid, 60U);
    BOOST_TEST_EQ(other_nonce, 9U);
}

}  // namespace webcpp::test

#endif
```

  `test/support/agreement.hpp` gains ChaCha's agreement, `agree_once`'s form with 12-byte
  nonces and messages near every multiple of 64:

```cpp
template <class A, class B>
void agree_chacha(std::uint64_t seed) {
    std::mt19937_64 generator(seed);
    for (std::size_t size = 0; size <= 4096; ++size) {
        if (size % 64 > 1 && size % 64 < 63 && generator() % 16 != 0) {
            continue;  // every size near a multiple of 64, and one in sixteen of the others
        }
        auto const key = bytes(generator, 32);
        auto const iv = bytes(generator, 12);
        auto const additional_data = bytes(generator, generator() % 65);
        auto const message = bytes(generator, size);
        auto a = crypto::chacha20_poly1305<A>::make(key);
        auto b = crypto::chacha20_poly1305<B>::make(key);
        auto data_a = message;
        auto data_b = message;
        auto const tag_a = a->seal(iv, additional_data, data_a);
        auto const tag_b = b->seal(iv, additional_data, data_b);
        if (!BOOST_TEST(tag_a && tag_b && data_a == data_b && to_hex(*tag_a) == to_hex(*tag_b))) {
            std::fprintf(stderr, "seed %llu, message of %zu bytes\n",
                         static_cast<unsigned long long>(seed), size);
            return;
        }
        BOOST_TEST(b->open(iv, additional_data, data_a, *tag_a).has_value());
        BOOST_TEST(a->open(iv, additional_data, data_b, *tag_b).has_value());
        BOOST_TEST(data_a == message && data_b == message);
    }
}
```

  Each backend's test file calls `check_rfc8439` and `check_chacha_guards_and_tampering`; the
  Wycheproof files `check_wycheproof_chacha`; `agreement_chacha_test.cpp` calls
  `agree_chacha<bearssl, openssl>(20261011)`. In `test/Jamfile` the BearSSL ones on every
  target, the OpenSSL ones natively, the Wycheproof ones with
  `<library>/third_party//wycheproof <library>/webcpp//boost_json`, the agreement natively.
  The message case `chacha_on_ctaes` states
  `webcpp::crypto: chacha20_poly1305<backends::ctaes>: ctaes exports the AES block cipher alone. Use a backend that exports ChaCha20-Poly1305: chacha20_poly1305<backends::bearssl>, chacha20_poly1305<backends::openssl> or chacha20_poly1305<backends::botan>.`
  Run the native lane. Expected: FAIL, `chacha20_poly1305.hpp` not found.
- [ ] **Step 2: Write `include/webcpp/crypto/chacha20_poly1305.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `chacha20_poly1305<Backend>`, RFC 8439's AEAD, one shot.

 @see "ChaCha20-Poly1305", in the guide.
*/
#ifndef WEBCPP_CRYPTO_CHACHA20_POLY1305_HPP
#define WEBCPP_CRYPTO_CHACHA20_POLY1305_HPP

#include <webcpp/crypto/config.hpp>

#include <webcpp/crypto/detail/engines.hpp>
#include <webcpp/crypto/detail/limits.hpp>
#include <webcpp/crypto/detail/wipe.hpp>
#include <webcpp/crypto/error.hpp>

#include <boost/hash2/digest.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace webcpp::crypto {

/**
 * ChaCha20-Poly1305 (RFC 8439) on a backend that exports it: BearSSL, OpenSSL or Botan. One
 * shot only: BearSSL's is one function over the whole message. A nonce is never repeated
 * under one key; with random nonces a key seals at most 2^32 messages.
 *
 * @tparam Backend The backend; the default is the AEADs'.
 */
template <class Backend = default_backend>
class chacha20_poly1305 {
    using state_type = detail::chacha_state<Backend>;
    using engine = detail::chacha_engine<Backend>;

public:
    /** The backend. */
    using backend_type = Backend;
    /** The tag, Boost.Hash2's digest of 16 bytes. */
    using result_type = boost::hash2::digest<16>;
    /** The key's size. */
    static constexpr std::size_t key_size = 32;
    /** The nonce's size, exactly (RFC 8439 section 2.8). */
    static constexpr std::size_t iv_size = 12;
    /** The tag's size. */
    static constexpr std::size_t tag_size = 16;
    /** The longest message, 2^38 - 64 bytes (RFC 8439 section 2.8). */
    static constexpr std::uint64_t max_message_size = (std::uint64_t{1} << 38) - 64;

    /**
     * Makes an object under a key.
     *
     * @param key 32 bytes.
     * @return The object; `invalid_key_size`; the backend's errors, as `gcm`'s.
     */
    static boost::system::result<chacha20_poly1305> make(std::span<std::uint8_t const> key) noexcept {
        if (key.size() != key_size) {
            return error::invalid_key_size;
        }
        chacha20_poly1305 made;
        auto const keyed = engine::init(made.state_, key);
        if (!keyed) {
            return keyed.error();
        }
        made.keyed_ = true;
        return made;
    }

    /** Moves the key; the source is wiped and empty. */
    chacha20_poly1305(chacha20_poly1305&& other) noexcept
        : keyed_(std::exchange(other.keyed_, false)) {
        if (keyed_) {
            engine::move(state_, other.state_);
        }
    }

    /** Moves the key over this one, which is wiped first; an empty source gives an empty one. */
    chacha20_poly1305& operator=(chacha20_poly1305&& other) noexcept {
        if (this != &other) {
            engine::reset(state_);
            keyed_ = std::exchange(other.keyed_, false);
            if (keyed_) {
                engine::move(state_, other.state_);
            }
        }
        return *this;
    }

    chacha20_poly1305(chacha20_poly1305 const&) = delete;
    chacha20_poly1305& operator=(chacha20_poly1305 const&) = delete;

    /** Wipes the key. */
    ~chacha20_poly1305() { engine::reset(state_); }

    /**
     * Seals a message in place.
     *
     * @param iv 12 bytes, never repeated under this key.
     * @param additional_data Authenticated, not encrypted.
     * @param data The message.
     * @return The tag; `invalid_state`; `invalid_iv_size`; `message_too_long`;
     * `backend_failure`.
     */
    boost::system::result<result_type> seal(std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data) noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        if (iv.size() != iv_size) {
            return error::invalid_iv_size;
        }
        if (detail::exceeds(data.size(), max_message_size)) {
            return error::message_too_long;
        }
        result_type tag{};
        auto const sealed = engine::seal(state_, iv, additional_data, data, tag);
        if (!sealed) {
            return sealed.error();
        }
        return tag;
    }

    /**
     * Opens a message in place; `data` is zeroed when the tag does not match or the backend
     * fails, and untouched when a guard refuses the call.
     *
     * @param iv The nonce of the seal.
     * @param additional_data The additional data of the seal.
     * @param data The ciphertext.
     * @param tag The tag, 16 bytes, which may lie anywhere, `data` included.
     * @return Nothing; `invalid_state`; `invalid_iv_size`; `invalid_tag_size`;
     * `message_too_long`; `authentication_failed`; `backend_failure`.
     */
    boost::system::result<void> open(std::span<std::uint8_t const> iv,
                                     std::span<std::uint8_t const> additional_data,
                                     std::span<std::uint8_t> data,
                                     std::span<std::uint8_t const> tag) noexcept {
        if (!keyed_) {
            return error::invalid_state;
        }
        if (iv.size() != iv_size) {
            return error::invalid_iv_size;
        }
        if (tag.size() != tag_size) {
            return error::invalid_tag_size;
        }
        if (detail::exceeds(data.size(), max_message_size)) {
            return error::message_too_long;
        }
        std::array<std::uint8_t, tag_size> expected{};
        std::copy(tag.begin(), tag.end(), expected.begin());
        auto const opened = engine::open(state_, iv, additional_data, data, expected);
        if (!opened) {
            detail::wipe(data.data(), data.size());
            return opened.error();
        }
        return {};
    }

private:
    chacha20_poly1305() noexcept = default;

    state_type state_{};
    bool keyed_ = false;
};

}  // namespace webcpp::crypto

#endif  // WEBCPP_CRYPTO_CHACHA20_POLY1305_HPP
```

- [ ] **Step 3: Write the engines.** BearSSL (in `backends/bearssl.hpp`; `<webcpp/crypto/equal.hpp>`
  joins its includes):

```cpp
/** ChaCha20-Poly1305's state on BearSSL: the key, in the object, since BearSSL takes it per call. */
template <>
struct chacha_state<backends::bearssl> {
    std::array<std::uint8_t, 32> key;
};

/**
 * ChaCha20-Poly1305 on BearSSL: SSE2's ChaCha20 or the constant-time code, and the 64-bit
 * multiplication's Poly1305 or the 32-bit one (inc/bearssl_block.h:2456-2612). BearSSL
 * leaves the tag's check to its caller (inc/bearssl_block.h:2500): `equal` does it.
 */
template <>
struct chacha_engine<backends::bearssl> {
    using state = chacha_state<backends::bearssl>;

    static br_chacha20_run chacha() noexcept {
        br_chacha20_run const sse2 = br_chacha20_sse2_get();
        return sse2 != nullptr ? sse2 : &br_chacha20_ct_run;
    }

    static br_poly1305_run poly() noexcept {
        br_poly1305_run const ctmulq = br_poly1305_ctmulq_get();
        return ctmulq != nullptr ? ctmulq : &br_poly1305_ctmul_run;
    }

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        std::copy(key.begin(), key.end(), s.key.begin());
        return {};
    }

    static boost::system::result<void> seal(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            boost::hash2::digest<16>& tag) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        poly()(s.key.data(), iv.data(), data.data(), data.size(), additional_data.data(),
               additional_data.size(), tag.data(), chacha(), 1);
        return {};
    }

    static boost::system::result<void> open(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            std::array<std::uint8_t, 16> const& tag) noexcept {
        read_canary(WEBCPP_CRYPTO_DETAIL_BEARSSL_CANARY);
        std::array<std::uint8_t, 16> computed{};
        poly()(s.key.data(), iv.data(), data.data(), data.size(), additional_data.data(),
               additional_data.size(), computed.data(), chacha(), 0);
        // Public by design (Task 20); not const, so the branch reads it after the request (n1).
        // NOLINTNEXTLINE(misc-const-correctness): see above
        bool matches = equal(computed, tag);
        WEBCPP_CRYPTO_DETAIL_DECLASSIFY(&matches, sizeof matches);
        wipe(computed.data(), computed.size());
        if (!matches) {
            return error::authentication_failed;
        }
        return {};
    }

    static void move(state& to, state& from) noexcept {
        to = from;
        wipe_object(from);
    }

    static void reset(state& s) noexcept { wipe_object(s); }
};
```

  OpenSSL (in `backends/openssl.hpp`): the state holds one `EVP_CIPHER_CTX*` keyed by
  `openssl_context("ChaCha20-Poly1305", key, true, true)`; `seal` and `open` run Task 12's
  GCM sequence with the AEAD controls (`EVP_CTRL_AEAD_SET_IVLEN` 12, `EVP_CTRL_AEAD_GET_TAG`,
  `EVP_CTRL_AEAD_SET_TAG`), whose GCM names are the same values in OpenSSL 3
  (`include/openssl/evp.h`, `EVP_CTRL_GCM_SET_IVLEN` defined as `EVP_CTRL_AEAD_SET_IVLEN`):

```cpp
template <>
struct chacha_state<backends::openssl> {
    EVP_CIPHER_CTX* context = nullptr;
};

template <>
struct chacha_engine<backends::openssl> {
    using state = chacha_state<backends::openssl>;
    using gcm_like = gcm_engine<backends::openssl, 32>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        auto const made = openssl_context("ChaCha20-Poly1305", key, true, true);
        if (!made) {
            return made.error();
        }
        s.context = *made;
        return {};
    }

    static boost::system::result<void> seal(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            boost::hash2::digest<16>& tag) noexcept {
        gcm_state<backends::openssl, 32> view{s.context};
        auto const started = gcm_like::start(view, true, iv, additional_data);
        auto const ran = started ? gcm_like::run(view, data) : started;
        return ran ? gcm_like::tag(view, tag) : ran;
    }

    static boost::system::result<void> open(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            std::array<std::uint8_t, 16> const& tag) noexcept {
        gcm_state<backends::openssl, 32> view{s.context};
        auto const started = gcm_like::start(view, false, iv, additional_data);
        auto const ran = started ? gcm_like::run(view, data) : started;
        return ran ? gcm_like::check(view, tag) : ran;
    }

    static void move(state& to, state& from) noexcept {
        to.context = std::exchange(from.context, nullptr);
    }

    static void reset(state& s) noexcept { EVP_CIPHER_CTX_free(std::exchange(s.context, nullptr)); }
};
```

  The GCM engine's `start`, `run`, `tag` and `check` take a state that only names the
  context, so ChaCha's borrows them on its own context: the controls they send are the AEAD
  ones under GCM's names. ctaes's refusal, in `backends/ctaes.hpp`:

```cpp
template <class Backend>
    requires std::is_same_v<Backend, backends::ctaes>
struct chacha_state<Backend> {
    static_assert(!std::is_same_v<Backend, backends::ctaes>, "webcpp::crypto: chacha20_poly1305<backends::ctaes>: ctaes exports the AES block cipher alone. Use a backend that exports ChaCha20-Poly1305: chacha20_poly1305<backends::bearssl>, chacha20_poly1305<backends::openssl> or chacha20_poly1305<backends::botan>.");
};
```

  A partial specialisation constrained to ctaes, so that its assertion depends on the
  template's parameter and fires only when `chacha20_poly1305<backends::ctaes>` is
  instantiated (a full specialisation's assertion would fire wherever the header is
  included); `<type_traits>` joins the header's includes. `include/webcpp/crypto.hpp` includes
  `chacha20_poly1305.hpp` (no backend) beside `gcm.hpp`.
- [ ] **Step 4: Extend the other lanes' tests**, each case written first and seen failing
  (no `chacha20_poly1305.hpp` before Step 2, or no engine before Step 3):
  - allocation (BearSSL: nothing; OpenSSL: the messages nothing, `make` reported), through
    Task 17's `counting.hpp`, the C library's allocator counted where the target lets it be;
  - the memory option's `everything_test.cpp` (spec 8.5: "The parts on chacha20_poly1305
    land with it");
  - OpenSSL's error queue (spec 3.3, m14): `openssl_error_queue_test.cpp`'s `main` becomes
    a function template `check_queue<Aead>(key, iv)` over the AEAD, run for
    `gcm<aes256<openssl>>` (12-byte IV) and `chacha20_poly1305<openssl>` (32-byte key,
    12-byte nonce): for each, the caller's `ERR_raise` entry survives a make, a seal, a good
    open and a failed one, and nothing of webcpp's is left beside it (one entry, the
    caller's);
  - `ctgrind_test.cpp` (seal and open of ChaCha20-Poly1305 on BearSSL; seal on OpenSSL, as
    Task 20 does for GCM, since OpenSSL's tag check cannot be declassified, I8): BearSSL's
    open branches only on the verdict of `equal`, declassified by the engine (Step 3), so
    memcheck reports nothing; the planted leak still fails as Task 20's;
  - `msan_test.cpp` the same cases on BearSSL;
  - `dudect_test.cpp` (m14), Task 20's form for ChaCha20-Poly1305 on BearSSL natively and on
    wasip2: a seal whose two classes draw the same random bytes and select by class (a zero
    key against random ones, m7), and an open whose two classes both fail (the right tag
    with its last byte flipped against random wrong tags, I9), each through `twice` under
    the threshold 10;
  - the sizes (`chacha_bearssl.cpp`, both optimisations, in `test/sizes/wasm/Jamfile` and
    `test/sizes/native/Jamfile` with their `--expect` lists);
  - the MSVC case of the refusal (the fixture `msvc_chacha_on_ctaes`).
- [ ] **Step 5: Run** the four lanes, the own lanes `constant-time` and `sizes`, and
  `python3 tools/test/crypto_jamfiles_test.py msvc_chacha_on_ctaes`; regenerate
  `doc/sizes.adoc`. Expected: PASS: RFC 8439's two vectors and Wycheproof's 325
  on both backends, the agreement, the refusal.
- [ ] **Step 6: Write the documentation:** `doc/aead.adoc` gains ChaCha20-Poly1305 (spec 2.5:
  one shot, exactly 12-byte nonces, the limit, nonces never repeated and the 2^32 with random
  ones, the backends), `doc/backends.adoc` and `doc/jamfile.adoc` their rows and the new
  message.
- [ ] **Step 7: Commit:** `"crypto: ChaCha20-Poly1305 on BearSSL and OpenSSL, against RFC
  8439 and Wycheproof"`.

## Task 36: Increment 4 whole: green, review, push

- [ ] **Step 1:** The definition of green, as Task 25's Step 2.
- [ ] **Step 2:** Review of increment 4 (spec 2.5, 8.1's ChaCha part, 8.2, 8.5's ChaCha part),
  with the Review Focus; fixes, each with a failing test first.
- [ ] **Step 3 (G4):** push the library and the superproject, with the owner's agreement.

---

# Increment 5: Botan, native, with exceptions

**Deferred after version 0.1 (revision of 2026-11-14).** Kept as written; before it starts,
re-read it against the tree of that day: `webcpp-own`, `webcpp.external-root` and
`webcpp.external-looked` were not built in version 0.1, so Task 37 adds what Botan's discovery
needs; the tools job has no macOS or Windows leg.

## Task 37: Botan found, or the build stops

**Files (superproject):**
- Create: `tools/ci/actions/botan/action.yml`, `tools/ci/actions/botan/install.sh`.
- Modify: `third_party/Jamfile` (the feature and the target), `tools/ci/matrix.py`
  (`EXTERNAL_DEPENDENCIES['crypto'] = ('openssl', 'botan')`, `EXTERNAL_VARIABLES` gains
  `BOTAN_ROOT`), `tools/ci/matrix_test.py`, `tools/ci/actions_test.py`,
  `.github/workflows/library.yml` (the step `botan`, on its name, natively).
- Modify (library): `build.jam` (the declaration).
- Create (superproject): Botan's stop cases, fixtures of Task 10's test under
  `tools/test/fixtures/crypto_jamfiles/botan_*/`.

**Interfaces:**
- Produces: `/third_party//botan` (usage requirements: Botan's include directory
  `include/botan-3`, `WEBCPP_CRYPTO_HAS_BOTAN=1`, the section garbage collection); the feature
  `webcpp-crypto-botan-link` (`shared`, `static`, propagated); `.local/botan` laid out by the
  action.

- [ ] **Step 1: Measure where Botan 3 comes from on each runner**, in the action's first run
  (the measurement decides the action's branches, each kept only if measured):
  - macOS: Homebrew's `botan` (3.13.0 on 2026-10-11, M6), linked into `.local/botan` as the
    OpenSSL action links `openssl@3`;
  - Linux: `pkg-config --modversion botan-3` on the runner (ubuntu-24.04 and 26.04); when it
    answers a 3.x, the system's, linked; when it does not, Botan 3.13.0 built from its release
    archive, `https://botan.randombit.net/releases/Botan-3.13.0.tar.xz`, checked with
    `tools/ci/download.sh` against
    `12f5a8358890bbee82edfe9d2e7769b0a610b6dd0e0698aea13d20a675d84620` (M6), configured with
    `python3 configure.py --prefix="$PWD/.local/botan" --minimized-build --enable-modules=aes,gcm,chacha20poly1305 --without-documentation`
    (the modules webcpp's backend needs; `configure.py` adds what they require), built with
    `make -j"$(nproc)" install`, and cached by `actions/cache` (pinned as `library.yml` pins
    its third-party actions) under a key of the archive's SHA-256, the runner's OS and the
    compiler;
  - Windows: the same archive built with MSVC (`python configure.py --cc=msvc --os=windows ...`
    then `nmake install`), cached the same way.
  The container lanes use the host's `.local/botan`, as they use `.local/openssl`: the
  container mounts the superproject at its own path.
- [ ] **Step 2: Write the failing build cases** (spec 3.6, 4.4, 8.6), fixtures of Task 10's
  test: `botan_on_wasm` (`targets: wasip2 wasip3 emscripten`:
  `error: /third_party//botan: Botan is a native backend, and this build is for WebAssembly. In the program's Jamfile, name /third_party//bearssl in its place.`),
  `botan_not_found` (native, with Task 11's knobs:
  `error: /third_party//botan: the program names Botan, and it was not found: ` and
  `. Install it (apt install libbotan-3-dev; brew install botan, then give -sBOTAN_ROOT=<the directory that brew --prefix botan prints>), give -sBOTAN_ROOT=<dir>, the directory of its include/ and lib/, or name /third_party//bearssl or /third_party//openssl in its place.`),
  `botan_wrong_root` (`holds no include/botan-3/botan/aead.h`), `botan_static_missing`
  (Task 11's form). Run the lane. Expected: FAIL, no target `botan`.
- [ ] **Step 3: Write the target**, Task 11's form for Botan: in crypto's `build.jam`,
  `webcpp.external crypto : Botan : BOTAN_ROOT : botan : botan-3/botan/aead.h : native ;`; in
  `third_party/Jamfile`:

```jam
# Botan 3 (BSD-2-Clause), which webcpp does not build: found where
# libs/crypto/build.jam declares it, natively only, for now (spec section 3.6),
# linked shared (the default) or static. Botan reports every error by
# throwing, so every program of its backend declares <exception-handling>on.
feature.feature webcpp-crypto-botan-link : shared static : propagated ;

rule botan-found ( properties * )
{
    if <toolset>emscripten in $(properties) || <toolset-clang:version>wasip2 in $(properties)
        || <toolset-clang:version>wasip3 in $(properties)
    {
        errors.user-error "/third_party//botan: Botan is a native backend, and this build is"
            "for WebAssembly. In the program's Jamfile, name /third_party//bearssl in its"
            "place." ;
    }
    local looked = [ webcpp.external-looked crypto : Botan : $(properties) ] ;
    if $(looked) && <webcpp-own>on in $(properties)
    {
        return [ webcpp.external-found crypto : Botan : $(properties) ] ;
    }
    if $(looked)
    {
        errors.user-error "/third_party//botan: the program names Botan, and it was not"
            "found: $(looked). Install it (apt install libbotan-3-dev; brew install botan,"
            "then give -sBOTAN_ROOT=<the directory that brew --prefix botan prints>), give"
            "-sBOTAN_ROOT=<dir>, the directory of its include/ and lib/, or name"
            "/third_party//bearssl or /third_party//openssl in its place." ;
    }
    # gcc links libstdc++, and a Mac's Botan is Homebrew's, built by Apple clang against
    # libc++: Botan's interface passes std::string and std::unique_ptr, so the program would
    # not link (I13). On Linux the system's Botan and both compilers use libstdc++. A user's
    # program is not stopped here: a Botan built by that g++ links (doc/backends.adoc).
    if <webcpp-own>on in $(properties) && <toolset>gcc in $(properties)
        && <target-os>darwin in $(properties)
    {
        if ! $(.botan-gcc-told)
        {
            .botan-gcc-told = true ;
            ECHO "webcpp: crypto's programs of Botan are left out with gcc on macOS: the"
                "Botan found is Homebrew's, built against libc++, and gcc links libstdc++." ;
        }
        return <build>no ;
    }
    if <webcpp-crypto-botan-link>static in $(properties)
    {
        local name = libbotan-3.a ;
        if <toolset>msvc in $(properties)
        {
            name = botan-3.lib ;
        }
        local root = [ webcpp.external-root crypto : Botan : native ] ;
        local file = <BOTAN_ROOT>/lib/$(name) ;
        if $(root)
        {
            file = $(root)/lib/$(name) ;
        }
        if ! $(root) || ! [ path.exists $(file) ]
        {
            errors.user-error "/third_party//botan: <webcpp-crypto-botan-link>static links"
                "$(file), which does not exist: give -sBOTAN_ROOT=<dir>, the directory of a"
                "Botan whose lib/ holds it, or link Botan shared." ;
        }
    }
}

.botan-root = [ webcpp.external-root crypto : Botan : native ] ;
lib botan-3 : : <name>botan-3 <toolset>msvc <webcpp-crypto-botan-link>shared
    <search>$(.botan-root)/lib
  : : <include>$(.botan-root)/include/botan-3 ;
lib botan-3 : : <file>$(.botan-root)/lib/botan-3.lib <toolset>msvc
    <webcpp-crypto-botan-link>static
  : : <include>$(.botan-root)/include/botan-3 ;
lib botan-3 : : <name>botan-3 <webcpp-crypto-botan-link>shared <search>$(.botan-root)/lib
  : : <include>$(.botan-root)/include/botan-3 <dll-path>$(.botan-root)/lib ;
lib botan-3 : : <file>$(.botan-root)/lib/libbotan-3.a <webcpp-crypto-botan-link>static
  : : <include>$(.botan-root)/include/botan-3 <target-os>linux:<linkflags>-pthread ;
explicit botan-3 ;

alias botan : botan-3 : <conditional>@botan-found
  : : <define>WEBCPP_CRYPTO_HAS_BOTAN=1 $(.gc) ;
explicit botan ;
```

  Botan's MSVC library names are the ones its `configure.py` writes (`botan-3.lib` for both
  links in an MSVC build of 3.x, the shared one an import library beside `botan-3.dll`):
  the first Windows run of the action lists `lib/` in its log, and the Jamfile follows what it
  lists.
- [ ] **Step 4: Write the action**, `tools/ci/actions/botan/install.sh` with the branches Step
  1 kept, in the OpenSSL action's form (its `refuse`, its `link`, `BOTAN_ROOT` written to
  `GITHUB_ENV`), and its case in `tools/ci/actions_test.py` (a fake Linux prefix with a
  `botan-3.pc` is linked). Run `python3 tools/ci/actions_test.py` and
  `python3 tools/ci/matrix_test.py` (crypto's native lanes list `["botan", "openssl"]`).
  Expected: PASS.
- [ ] **Step 5: Run** `python3 tools/test/crypto_jamfiles_test.py botan_on_wasm botan_not_found botan_wrong_root botan_static_missing`
  once Homebrew's `botan` is installed and linked into `.local/botan` (the action's macOS
  branch; `brew install botan`, the one machine-wide change of this plan, departure 4, which
  the owner agrees to first). Expected: `crypto_jamfiles_test: ok`.
- [ ] **Step 6: Commit** (superproject) `"third_party: Botan 3 found, natively, or the build
  stops; tools/ci: Botan's action"`; (library) `"crypto: Botan declared, and its stops as a
  user meets them"`.

## Task 38: The Botan backend, with exceptions only

**Files:**
- Create: `include/webcpp/crypto/backends/botan.hpp`, `test/botan/Jamfile`,
  `test/botan/gcm_botan_test.cpp`, `test/botan/chacha_botan_test.cpp`,
  `test/botan/aes_botan_test.cpp`, `test/botan/mapping_test.cpp`,
  `test/botan/additional_data_test.cpp`, `test/botan/allocation_botan_test.cpp`,
  `test/botan/headers_no_exceptions_test.cpp`, `test/messages/botan_*` (below).
- Modify: `include/webcpp/crypto/config.hpp` (Botan's refusals and its place in the default),
  `include/webcpp/crypto/aes256_gcm.hpp` (its include), `include/webcpp/crypto.hpp`,
  `test/Jamfile` (`build-project botan ;`), `test/messages/Jamfile`; (superproject)
  `tools/lint/compile_commands.py` and `tools/lint/lint_test.py` only if the lint's existing
  rule for a header that declares `<exception-handling>on` (AGENTS chapter 6) needs Botan's
  header named, which Step 6 checks.

**Interfaces:**
- Produces: `struct webcpp::crypto::backends::botan { static constexpr std::string_view name = "botan"; };`
  (declared only with exceptions); the six traits for Botan; the macros
  `WEBCPP_CRYPTO_DETAIL_DEFAULT_BOTAN`, `WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN`.

- [ ] **Step 1: Write the failing message tests** (spec 3.6, 4.3's new texts), each a
  directory of `test/messages/`:
  - `botan_on_wasm` (wasip2, wasip3; `<define>WEBCPP_CRYPTO_HAS_BOTAN=1`):
    `webcpp::crypto: Botan is a native backend and this program is built for WebAssembly. In the program's Jamfile, replace /third_party//botan with /third_party//bearssl.`;
  - `botan_not_linked` (native; `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN`, the source
    including `aes256_gcm.hpp`):
    `webcpp::crypto: the backend Botan is used and not linked. In the program's Jamfile, add /third_party//botan beside /webcpp/crypto//crypto; b2 links it when it finds Botan 3 (-sBOTAN_ROOT=<dir> names it). (without b2: define WEBCPP_CRYPTO_HAS_BOTAN=1 and link Botan 3)`;
  - `botan_without_exceptions` (native; `<define>WEBCPP_CRYPTO_HAS_BOTAN=1 <exception-handling>off`,
    Botan the only AEAD backend):
    `webcpp::crypto: the backend Botan needs exceptions, and this program is built without them. In the program's Jamfile, name /third_party//bearssl or /third_party//openssl in its place.`;
  - `default_openssl_and_botan`, `default_bearssl_and_botan` (native):
    `webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN are defined. Keep one.`
    and the same with BEARSSL;
  - `default_botan_on_wasm` (wasip2):
    `webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN selects a native backend and this program is built for WebAssembly. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL.`;
  - `botan_old` (native; a fixture `include/botan-3/botan/build.h` defining
    `BOTAN_VERSION_MAJOR 2`):
    `webcpp::crypto: the backend Botan needs Botan 3, and the headers found are of another major version. Give -sBOTAN_ROOT=<dir>, the directory of Botan 3's include/ and lib/.`.
  Run the native and wasip2 lanes. Expected: FAIL.
- [ ] **Step 2: Extend `config.hpp`:** the refusals of Botan on WebAssembly (its `HAS` and its
  default), the two pairs of defaults with Botan, and the default's order natively
  `openssl`, `bearssl`, `botan`:

```cpp
#if defined(WEBCPP_CRYPTO_WEBASSEMBLY) && defined(WEBCPP_CRYPTO_HAS_BOTAN)
#error "webcpp::crypto: Botan is a native backend and this program is built for WebAssembly. In the program's Jamfile, replace /third_party//botan with /third_party//bearssl."
#endif
#if defined(WEBCPP_CRYPTO_WEBASSEMBLY) && defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN)
#error "webcpp::crypto: WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN selects a native backend and this program is built for WebAssembly. Remove that define, or make it WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL."
#endif
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL) && defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN)
#error "webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN are defined. Keep one."
#endif
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL) && defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN)
#error "webcpp::crypto: both WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL and WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN are defined. Keep one."
#endif
```

  and the selection becomes, whole:

```cpp
#if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL 1
#elif defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BEARSSL 1
#elif defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BOTAN)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BOTAN 1
#elif !defined(WEBCPP_CRYPTO_WEBASSEMBLY) && defined(WEBCPP_CRYPTO_HAS_OPENSSL)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_OPENSSL 1
#elif !defined(WEBCPP_CRYPTO_WEBASSEMBLY) && !defined(WEBCPP_CRYPTO_HAS_BEARSSL) && \
    defined(WEBCPP_CRYPTO_HAS_BOTAN)
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BOTAN 1
#else
#define WEBCPP_CRYPTO_DETAIL_DEFAULT_BEARSSL 1
#if !defined(WEBCPP_CRYPTO_HAS_BEARSSL)
#define WEBCPP_CRYPTO_DETAIL_NONE_LINKED 1
#endif
#endif
```

  (natively OpenSSL, then BearSSL, then Botan, spec 5), then
  `#if defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_BOTAN) && defined(WEBCPP_CRYPTO_NO_EXCEPTIONS)`
  with the `botan_without_exceptions` text, and `default_backend` is `backends::botan` under
  `WEBCPP_CRYPTO_DETAIL_DEFAULT_BOTAN`. `aes256_gcm.hpp` includes `backends/botan.hpp` for that
  default, and its `detect_mismatch` value is `"botan"` there.
- [ ] **Step 3: Write `backends/botan.hpp`:** layer 1 (`botan_not_linked`'s text when
  `WEBCPP_CRYPTO_HAS_BOTAN` is not defined), then everything inside
  `#ifndef WEBCPP_CRYPTO_NO_EXCEPTIONS` with a comment naming Botan, which reports errors by
  throwing (AGENTS chapter 6's rule for a header that wraps a throwing dependency):

```cpp
#include <botan/aead.h>
#include <botan/block_cipher.h>
#include <botan/build.h>
#include <botan/exceptn.h>
#include <botan/secmem.h>

#if BOTAN_VERSION_MAJOR != 3
#error "webcpp::crypto: the backend Botan needs Botan 3, and the headers found are of another major version. Give -sBOTAN_ROOT=<dir>, the directory of Botan 3's include/ and lib/."
#endif

namespace webcpp::crypto::backends {

/** The backend Botan (spec section 3.6), only with exceptions. */
struct botan {
    /** The backend's name, which the examples print. */
    static constexpr std::string_view name = "botan";
};

}  // namespace webcpp::crypto::backends

namespace webcpp::crypto::detail {

/**
 * Runs a call into Botan, every exception caught at the boundary and mapped (spec 3.6):
 * an invalid tag is authentication_failed, an invalid IV or key length invalid_iv_size or
 * invalid_key_size, std::bad_alloc not_enough_memory, any other exception backend_failure.
 */
template <class F>
boost::system::result<void> botan_call(F&& f) noexcept {
    try {
        f();
        return {};
    } catch (Botan::Invalid_Authentication_Tag const&) {
        return error::authentication_failed;
    } catch (Botan::Invalid_IV_Length const&) {
        return error::invalid_iv_size;
    } catch (Botan::Invalid_Key_Length const&) {
        return error::invalid_key_size;
    } catch (std::bad_alloc const&) {
        return boost::system::errc::make_error_code(boost::system::errc::not_enough_memory);
    } catch (...) {
        return error::backend_failure;
    }
}

/** Botan's name of AES with keys of `KeySize` bytes. */
template <std::size_t KeySize>
inline constexpr char const* botan_aes_name =
    KeySize == 16 ? "AES-128" : KeySize == 24 ? "AES-192" : "AES-256";

/** GCM's state on Botan: a mode per direction, and the scratch `finish` appends the tag to. */
template <std::size_t KeySize>
struct gcm_state<backends::botan, KeySize> {
    std::unique_ptr<Botan::AEAD_Mode> sealing;
    std::unique_ptr<Botan::AEAD_Mode> opening;
    Botan::secure_vector<std::uint8_t> scratch;
    bool seal_direction = false;
};

template <std::size_t KeySize>
struct gcm_engine<backends::botan, KeySize> {
    using state = gcm_state<backends::botan, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        return botan_call([&] {
            std::string name = std::string(botan_aes_name<KeySize>) + "/GCM";
            s.sealing = Botan::AEAD_Mode::create_or_throw(name, Botan::Cipher_Dir::Encryption);
            s.opening = Botan::AEAD_Mode::create_or_throw(name, Botan::Cipher_Dir::Decryption);
            s.sealing->set_key(key);
            s.opening->set_key(key);
            // GCM's update granularity is 1 in Botan 3 (M6): process takes every byte, and
            // finish's scratch holds the tag alone, so it never grows past this.
            s.scratch.reserve(s.sealing->update_granularity() + 16);
        });
    }

    // The additional data is set on every message, empty included: Botan keeps it between
    // messages (botan_ghash.cpp:173, :178-186).
    static boost::system::result<void> start(state& s, bool sealing, std::span<std::uint8_t const> iv,
                                             std::span<std::uint8_t const> additional_data) noexcept {
        s.seal_direction = sealing;
        auto& mode = sealing ? *s.sealing : *s.opening;
        return botan_call([&] {
            mode.set_associated_data(additional_data);
            mode.start(iv);
        });
    }

    static boost::system::result<void> run(state& s, std::span<std::uint8_t> data) noexcept {
        auto& mode = s.seal_direction ? *s.sealing : *s.opening;
        return botan_call([&] { static_cast<void>(mode.process(data)); });
    }

    static boost::system::result<void> tag(state& s, boost::hash2::digest<16>& out) noexcept {
        return botan_call([&] {
            s.scratch.clear();
            s.sealing->finish(s.scratch, 0);
            std::copy(s.scratch.begin(), s.scratch.end(), out.data());
            s.scratch.clear();
        });
    }

    static boost::system::result<void> check(state& s, std::array<std::uint8_t, 16> const& tag) noexcept {
        return botan_call([&] {
            s.scratch.assign(tag.begin(), tag.end());
            s.opening->finish(s.scratch, 0);
            s.scratch.clear();
        });
    }

    static void abandon(state& s) noexcept {
        if (s.sealing) {
            static_cast<void>(botan_call([&] { s.sealing->reset(); }));
        }
    }

    static void move(state& to, state& from) noexcept {
        to.sealing = std::move(from.sealing);
        to.opening = std::move(from.opening);
        to.scratch = std::move(from.scratch);
        to.seal_direction = false;
    }

    // Botan's clear() wipes the key schedule; secure_vector wipes on release.
    static void reset(state& s) noexcept {
        if (s.sealing) {
            s.sealing->clear();
        }
        if (s.opening) {
            s.opening->clear();
        }
        s.sealing.reset();
        s.opening.reset();
        s.scratch.clear();
    }
};
```

  The block cipher's engine and ChaCha20-Poly1305's, in the same file:

```cpp
/** The block cipher's state on Botan: Botan's object, which holds the key schedule. */
template <std::size_t KeySize>
struct block_state<backends::botan, KeySize> {
    std::unique_ptr<Botan::BlockCipher> cipher;
};

template <std::size_t KeySize>
struct block_engine<backends::botan, KeySize> {
    using state = block_state<backends::botan, KeySize>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        return botan_call([&] {
            s.cipher = Botan::BlockCipher::create_or_throw(botan_aes_name<KeySize>);
            s.cipher->set_key(key);
        });
    }

    static boost::system::result<void> encrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        return botan_call([&] { s.cipher->encrypt_n(blocks.data(), blocks.data(), blocks.size() / 16); });
    }

    static boost::system::result<void> decrypt(state& s, std::span<std::uint8_t> blocks) noexcept {
        return botan_call([&] { s.cipher->decrypt_n(blocks.data(), blocks.data(), blocks.size() / 16); });
    }

    static void move(state& to, state& from) noexcept { to.cipher = std::move(from.cipher); }

    // Botan's clear() wipes the key schedule.
    static void reset(state& s) noexcept {
        if (s.cipher) {
            s.cipher->clear();
            s.cipher.reset();
        }
    }
};

/** ChaCha20-Poly1305's state on Botan: a mode per direction and finish's scratch. */
template <>
struct chacha_state<backends::botan> {
    std::unique_ptr<Botan::AEAD_Mode> sealing;
    std::unique_ptr<Botan::AEAD_Mode> opening;
    Botan::secure_vector<std::uint8_t> scratch;
};

template <>
struct chacha_engine<backends::botan> {
    using state = chacha_state<backends::botan>;

    static boost::system::result<void> init(state& s, std::span<std::uint8_t const> key) noexcept {
        return botan_call([&] {
            s.sealing = Botan::AEAD_Mode::create_or_throw("ChaCha20Poly1305", Botan::Cipher_Dir::Encryption);
            s.opening = Botan::AEAD_Mode::create_or_throw("ChaCha20Poly1305", Botan::Cipher_Dir::Decryption);
            s.sealing->set_key(key);
            s.opening->set_key(key);
            s.scratch.reserve(s.sealing->update_granularity() + 16);
        });
    }

    static boost::system::result<void> seal(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            boost::hash2::digest<16>& tag) noexcept {
        return botan_call([&] {
            s.sealing->set_associated_data(additional_data);
            s.sealing->start(iv);
            static_cast<void>(s.sealing->process(data));
            s.scratch.clear();
            s.sealing->finish(s.scratch, 0);
            std::copy(s.scratch.begin(), s.scratch.end(), tag.data());
            s.scratch.clear();
        });
    }

    static boost::system::result<void> open(state& s, std::span<std::uint8_t const> iv,
                                            std::span<std::uint8_t const> additional_data,
                                            std::span<std::uint8_t> data,
                                            std::array<std::uint8_t, 16> const& tag) noexcept {
        return botan_call([&] {
            s.opening->set_associated_data(additional_data);
            s.opening->start(iv);
            static_cast<void>(s.opening->process(data));
            s.scratch.assign(tag.begin(), tag.end());
            s.opening->finish(s.scratch, 0);
            s.scratch.clear();
        });
    }

    static void move(state& to, state& from) noexcept {
        to.sealing = std::move(from.sealing);
        to.opening = std::move(from.opening);
        to.scratch = std::move(from.scratch);
    }

    static void reset(state& s) noexcept {
        if (s.sealing) {
            s.sealing->clear();
        }
        if (s.opening) {
            s.opening->clear();
        }
        s.sealing.reset();
        s.opening.reset();
        s.scratch.clear();
    }
};

}  // namespace webcpp::crypto::detail
```

  ChaCha20-Poly1305's update granularity in Botan 3 is 1 too (M6), so `process` takes the
  whole message and the scratch holds the tag alone. `Botan::Cipher_Mode::reset()` ends a
  message and keeps the key (Botan 3's `cipher_mode.h`); `clear()` wipes the key.
  `make` of a Botan
  object allocates (`create_or_throw`, `secure_vector`): the departure the owner accepted on
  2026-10-11, which `allocation_botan_test.cpp` counts and reports, and the backend's page
  states; every later call allocates nothing, which the same test proves with the counting
  `operator new` (Botan's `secure_allocator` goes to `calloc`, which the interposers of Task
  16 count: the test links them as Task 16's Jamfile does).
- [ ] **Step 4: Write the Botan tests** in `test/botan/` (its Jamfile:
  `project : requirements <library>/webcpp/crypto//crypto <library>/third_party//botan <exception-handling>on ;`
  with the reason in a comment, `webcpp.targets native ;`): the GCM checks of Tasks 7, 8 and
  9 on `botan` (spec cases, guards, tampering, moved-from, edges, incremental, machine, the
  one-shot during a message of Review Focus 1, limits, Wycheproof, CAVP), the block cipher's
  of Task 12, ChaCha's of Task 35; `mapping_test.cpp`, each mapping of `botan_call` by an
  exception thrown from a lambda given to it; `additional_data_test.cpp` (spec 3.6: a seal
  with additional data, then one without, compared with BearSSL's, so that the second
  message is not authenticated with the first's data); and
  `headers_no_exceptions_test.cpp`, a headers-alone compile of `backends/botan.hpp` with
  `<exception-handling>off` that only includes it (the header declares nothing then, AGENTS
  chapter 6), declared `webcpp.compile`, outside the Jamfile's project requirement through a
  directory of its own, `test/botan/no_exceptions/`.
- [ ] **Step 5: Run** the native lane (Apple clang) with `-sBOTAN_ROOT`, and the gcc-15 lane
  in its Linux container (`python3 tools/ci/matrix.py run gcc-15`, where the action's Botan is
  libstdc++'s). Expected: PASS. Then gcc-15 on the Mac itself, natively
  (`$B2 -a toolset=gcc-15 --build-dir=bin/lane-gcc-15-darwin libs/crypto/test`). Expected: the
  line `webcpp: crypto's programs of Botan are left out with gcc on macOS: ...` once, every
  Botan program skipped and the rest passing (I13: Homebrew's Botan is libc++'s, Task 37's
  rule).
- [ ] **Step 6: The lint.** Run it. A finding in `backends/botan.hpp` under the lint's
  no-exceptions twin is the case AGENTS chapter 6 names ("the lint leaves that header out of
  the analysis without exceptions, and names it"): the header's headers-alone call declares
  `<exception-handling>on`, which is what the lint reads; if the lint still analyses it
  without exceptions, `compile_commands.py`'s rule for that call is the fix, with its case in
  `lint_test.py`. Expected: `lint: clean`.
- [ ] **Step 7: Commit:** `"crypto: the Botan backend, with exceptions, its errors caught at
  its boundary, its additional data set on every message"`.

## Task 39: Botan in every native lane, its page, and the last green

**Files:**
- Create: `test/constant_time/ctgrind_botan_test.cpp`, `ctgrind_botan_planted_test.cpp`,
  `dudect_botan_test.cpp`.
- Modify: `test/support/agreement.hpp` (three-way), `test/botan/Jamfile` (the agreement
  programs), `test/constant_time/Jamfile`, `doc/backends.adoc`, `doc/jamfile.adoc`,
  `doc/memory.adoc`, `README.md`; (superproject) the fixture
  `tools/test/fixtures/crypto_jamfiles/botan/` (a user's Botan program), `tools/ci/matrix.py`
  and `tools/ci/matrix_test.py` (the lane `constant-time` needs `botan`).

- [ ] **Step 1: Agreement** (spec 8.2): `agree_gcm<bearssl, botan>`, `agree_gcm<openssl, botan>`,
  `agree_chacha<bearssl, botan>`, `agree_chacha<openssl, botan>`, and the block cipher's
  ctaes, OpenSSL and Botan, in `test/botan/` with `<exception-handling>on`, natively.
- [ ] **Step 1a: Botan in the constant-time checks** (spec 8.3, "every backend", m14), each
  written first and seen failing (no file):
  - `test/constant_time/ctgrind_botan_test.cpp`, Task 20's ctgrind form: the seal of
    `gcm<aes256<botan>>` and of `chacha20_poly1305<botan>` with the key and the plaintext
    marked undefined, and memcheck reporting nothing; the open is left out of it and the
    test's comment says why, as for OpenSSL (I8): Botan checks the tag inside itself and
    throws on a failure, a branch on the verdict that webcpp cannot declassify (Botan's own
    `CT::unpoison` exists only in a Botan configured `--with-valgrind`, which neither the
    system's nor Homebrew's is); its planted leak behind `WEBCPP_TEST_CRYPTO_PLANTED_LEAK`,
    as Task 20's, in a one-line twin `ctgrind_botan_planted_test.cpp`
    (`#include "ctgrind_botan_test.cpp"`), so a memcheck that sees nothing fails;
  - `test/constant_time/dudect_botan_test.cpp`, Task 20's dudect form through `twice`, under
    the threshold 10: the seal of both AEADs, two classes of keys drawn alike (m7), and the
    open of both, two failing classes (I9), which measures the whole of Botan's failed open,
    its exception included;
  - in `test/constant_time/Jamfile`, Task 20's forms with
    `<library>/third_party//botan <exception-handling>on` added (the rest of that Jamfile
    keeps exceptions off): `webcpp.drive ctgrind_botan` and `ctgrind_botan_planted` (the
    second with `--expect-leak`) through `valgrind.mjs`, and `webcpp.run dudect_botan`, each
    `native`, the three added to the `constant-time` lane's list; `matrix.py`'s
    `EXTERNAL_DEPENDENCIES` gives the lane `botan` beside `openssl`, and its `matrix_test.py`
    case says so;
  - `doc/backends.adoc`'s Botan section states what is checked (the seal under Valgrind,
    seal and open by timing) and what is not (the open under Valgrind).
  Run the lane `constant-time` on a Linux machine with valgrind (Task 20's Step 5).
  Expected: PASS, and the planted leak's twin failing as it should.
- [ ] **Step 2: A user's Botan program**, the fixture `tools/test/fixtures/crypto_jamfiles/botan/`
  (`exe app : app.cpp /webcpp/crypto//crypto /third_party//botan : <exception-handling>on ;`,
  outcome `root: BOTAN_ROOT`, `requires: botan`,
  spec 7.2's `app.cpp`, printing `aes256_gcm ok backend=botan` natively, the default when Botan
  is the only AEAD backend linked).
- [ ] **Step 3: The page:** Botan's row and section in `doc/backends.adoc` (spec 3.6 for a
  user: found, native only for now, exceptions only, its errors mapped, the additional data on
  every message, its allocation outside the user's allocator with no hook that could route it
  and why, measured counts from `allocation_botan_test`), its messages in `doc/jamfile.adoc`,
  and `doc/memory.adoc`'s line that Botan keeps the departure.
- [ ] **Step 4: Run the definition of green** (Task 25's list, Botan's lanes with it), the
  review of increment 5 with the Review Focus, its fixes, and G4's push, with the owner's
  agreement.

---

## Spec coverage

The tasks of version 0.1 are 0 to 14, 17 to 19, 21, 24 to 30a; the others are deferred (the
revision of 2026-11-14).

| Spec | Tasks |
| --- | --- |
| 1.1, 1.2 what the library is, what stays with Hash2 | 1, 24 |
| 1.3 the owner's decisions | every task; Conflicts with the spec |
| 2.1 the shape, results, spans | 2, 7 |
| 2.2 the headers, `<windows.h>` | 1, 4, 6, 7, 14, 19 |
| 2.3 block ciphers, BearSSL's refusal | 7, 12, 29, 38 |
| 2.4 AES-GCM: interface, guards, incremental seal, moved-from, decisions | 7, 8, 9, 12, 38 |
| 2.5 ChaCha20-Poly1305 | 35, 38 |
| 2.6 `equal` | 3, 20 |
| 2.7 `random_device` | 4 |
| 2.8 secp256k1 | 26, 27, 28 |
| 2.9 errors | 2 |
| 2.10 exceptions | 1, 4, 15, 38; C15 |
| 2.11 allocation and keys | 3, 17, 27, 38 |
| 3.1 what each exports | 24, 29, 39 |
| 3.2 BearSSL | 5, 7, 8, 13, 35 |
| 3.3 OpenSSL | 11, 12, 17 |
| 3.4 ctaes | 29 |
| 3.5 libsecp256k1 | 26 |
| 3.6 Botan | 37, 38, 39 |
| 3.7 OpenSSL's memory functions | 6, 15, 16, 33 |
| 4.1 where the C libraries live, defines, usage requirements | 5, 26, 29 |
| 4.2 file by file; a library named and not called | 5, 21 |
| 4.3 three layers, every message | 5, 6, 12, 18, 26, 29, 35, 38 |
| 4.4 external libraries: OpenSSL source-built (C2), Botan found | 11, 37 |
| 4.5 one copy per program | 11, 30a, 32 |
| 5 the default backend per target | 6, 14, 22, 38 |
| 6 entropy | 4, 5, 17, 23, 31 |
| 7 the user's Jamfile | 10, 24, 29, 39 |
| 8 levels | C3 |
| 8.1 test vectors, C suites, tampering, machine, moved-from, WebCrypto | 5, 7, 8, 9, 12, 23, 26, 28, 29, 35 |
| 8.2 cross-backend agreement | 13, 29, 35, 39 |
| 8.3 constant time | 7 (the seam), 20, 26, 35, 39 |
| 8.4 entropy's tests | 4, 17, 23 |
| 8.5 allocation, the memory option's tests | 15, 16, 17, 27, 33, 35, 38 |
| 8.6 every message, the pins | 6, 11, 12, 16, 18, 19, 29, 35, 37, 38 |
| 8.7 the unused library | 21, 26, 29 |
| 8.8 sizes and throughput | 22, 23, 29, 35 |
| 9 increments 1 to 5 | 0-25, 26-30a, 31-34, 35-36, 37-39 |
| 10 departures and rulings | applied where each section is; C1 to C19; the revision of 2026-11-14 |
