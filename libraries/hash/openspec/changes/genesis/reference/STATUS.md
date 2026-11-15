# webcpp::hash: where the implementation stands, and how to start

Written 2026-11-14, when this proposal was made. The owner decided the library's shape, widened
it the same day (every backend's whole hashing and KDF surface, in increments; one profile for
every default backend; documentation in noble's and libsodium's manner) and answered O1 (the
browser backend through a parallel asynchronous family), and accepted it. `spec.md` is the
specification, `plan.md` the implementation plan of 48 tasks (0 to 47) in seven increments,
`../tasks.md` the checklist. **The reference code is the code written in the plan's tasks**
(listed in `../proposal.md`, "Implementation"). There is no `source/` directory: nothing is
implemented.

## Where it stands

- **The library's repository has no commit,** and `libs/hash` exists nowhere.
  `webcpporg/hash` has not been created (gate G1), the submodule not added (G2).
- **No task is done.** Every step of `plan.md` is unchecked.
- **The pins were read on 2026-11-14** with `gh api` (read-only): libsodium 1.0.22
  (`77e1ce5d6dee871c49ef211222ba18ef0c486bda`, tag `1.0.22-RELEASE`), XKCP `master`
  (`d20c8fcaecc543f816a0721d0dfc13b4cf27c14d`, 2026-11-10; XKCP has no release tags), BLAKE3
  1.8.7 (`f3149ec5bb5449af877ba20377a11008ff499fa2`), BLAKE2's test vectors
  (`ed1974ea83433eba7b2d95c5dcd9ac33cb847913`, increment 2), libsodium-doc
  (`6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56`, ISC, for the documentation), Botan's latest tag
  3.14.1 (`01af88c`, for increment 6's reading). BearSSL and OpenSSL are the superproject's
  (`7bea48e`, `45e844f`).
- **What was read upstream for the spec** is listed in `plan.md`'s "Spec" paragraph (libsodium's
  file counts and define sets, its private `argon2_ctx`, XKCP's include graph and `config.h`,
  BLAKE3's files and its 35 vectors, OpenSSL 3.5.9's `HKDF_MAXINFO` and PBKDF2's `pkcs5`, OpenSSL's
  KECCAK and ARGON2 names). Nothing was built or measured for this proposal: the size and
  throughput tables are the plan's Tasks 21 and 22.

## What it waits on

- **webcpp::crypto's version 0.1** for `/third_party//bearssl` (Task 13) and
  `webcpp.same-program` (Task 19); every other task of increment 1 starts from today's `main`.
- **webcpp::crypto's Botan provisioning** (its increment 5) for increment 6.
- **webcpp::asio's `browser::async_await`** on `main` for increment 7 (it is in the asio track's
  tree at the time of writing).
- **The owner's gates** G1, G2, G4 and G5 (`plan.md`, "Owner gates"). No push, no pull request
  and no CI run by an agent; all evidence is local until the owner asks.

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1): Boost 1.92, b2 5.5.3, a C++20
compiler, wasi-sdk 34, wasmtime, Emscripten 6.0.11 with node 26.7.0, Python 3.9 or newer,
`.local/user-config.jam`.

1. **Task 0:** the worktree on `hash/genesis` from `origin/main`, the ledger, the baseline.
2. **Task 1,** then the five streams of wave 1 in parallel (Tasks 2, 3, 4 and 5, 6 and 7, 8),
   each in its own worktree, as `plan.md`'s table of streams says; the controller merges each
   reviewed stream.
3. **Waves 2 to 6** in the table's order; Task 24 closes increment 1.
4. **Increments 2 to 7** after Task 25, in parallel streams by backend; Task 47 closes each.

Every b2 command runs as `$B2` (`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
--user-config=.local/user-config.jam`); only a build from scratch (`-a`) counts as evidence. The
lanes and their commands are in `plan.md`'s "Global Constraints".

## What to verify first

1. `git -C third_party/libs/libsodium rev-parse HEAD` after Task 8's Step 1 prints `77e1ce5d...`,
   and `find ... -name '*.c' | wc -l` prints 141.
2. After Task 5: Keccak-256 of "" is `c5d24601...5d85a470` on the four targets, and Hash2's
   `keccak_base<0x01, 512, 256>` agrees on random call sequences.
3. After Task 9: RFC 5869's seven cases pass on `backends::hash2` on the four targets.

## Decisions pending

None. O1 is decided (design D23). The owner's gates are actions, not decisions.
