# Status of the foundation change

**Date:** 2026-11-17 (accepted by the owner on 2026-11-14; revised on 2026-11-17).

## Where things stand

- Nothing of this change is built. Every task of `../tasks.md` is open.
- **Its base is ready.** crypto's version 0.1 (the genesis's groups 1 and 2) is merged:
  `webcpporg/crypto` `main` at `d242be8`, recorded by the superproject's `main` (`3fa7ca5` on
  2026-11-17). webcpp::hash's genesis is merged with `/third_party//sodium` (libsodium 1.0.22 at
  `77e1ce5`) and `backends::hash2` as every key derivation's default (hash's D33). The
  superproject's `webcpp.size-table` requires release builds (`69bf8ab`).
- **The revision of 2026-11-17** (`../design.md` D32 to D35, `plan.md`'s section of that day,
  `roadmap.md`): the genesis's increments 3 to 5 are placed here without trystero's migration
  (ChaCha20-Poly1305 on BearSSL and OpenSSL in increment 1, Botan's genesis cells in increment
  7, the memory option as stream M of the genesis); defaults are one per family and target by
  a stated reason, with no profile and no measured default; the capability table is version
  0.1's `doc/tables/crypto.json` grown; sizes and throughput come from release builds only and
  decide nothing. The owner answered Q1 to Q7 the same day, each as recommended (`roadmap.md`,
  "The owner's answers"); wave 1 (streams A, B and M) starts.
- Pins checked with `gh api` on 2026-11-14: libsodium 1.0.22 is the latest stable release
  (`77e1ce5d6dee871c49ef211222ba18ef0c486bda`, 2026-04-09), the pin webcpp::hash uses; blst's
  latest release is v0.3.17 (`54e6e55674722fc2797ebb4bbb71b26d881eb4b8`, 2026-07-24); Botan's
  latest 3.x tag is 3.14.1 (`01af88c039fcfaedc418b1af4d6d3a139674e591`, 2026-11-06);
  libsodium's documentation `jedisct1/libsodium-doc` master is
  `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56` (ISC).
- Pins of increments 9 to 12, checked with `gh api` on 2026-11-15: c-kzg-4844's latest release
  v2.1.8 (`e125905e5e01186e6ccb7a0ced4845bf7eddbcfe`, 2026-07-09, Apache-2.0, its own blst
  submodule at v0.3.16); mcl's latest tag v4.43 (`50e4c6f2826de6b041f857caf9f253cc736ea926`,
  BSD-3-Clause; mcl publishes tags, not releases); dsprenkels/sss's latest commit
  `9f2a10b1c0d391bed485c4c7e93788ce37c360f1` (2025-08-18, MIT; its only tag is v0.1.0); Botan
  3.14.1 holds the `hpke` module (`src/lib/pubkey/hpke/`); `novifinancial/ed25519-speccheck`
  (Apache-2.0), "Taming the many EdDSAs"' suite, at `65519336fda78a3d016e947df6d82848aca0c9da`
  (2021-02-26), the test data of design D31.
- An upstream fact found while writing: libsodium 1.0.22 has the ristretto255 and
  edwards25519 maps from hashes, and not the full RFC 9380 `crypto_core_*_from_string`
  functions, which are on libsodium's `master` since 2026-02-20 and in no release (design
  D17).

## What to redo

Nothing: no work was done.

## How to resume

1. Read `roadmap.md`: the overlaps resolved, the defaults, the waves and their streams.
2. Wave 1: stream A follows `plan.md` from Task 1 through its revision of 2026-11-17, in the
   task groups A1 to A4, with superpowers:subagent-driven-development; stream B's tasks
   are written in full (`plan.md` Tasks 2.1 to 2.7, a proposals pull request of their own); it
   branches from A's branch once A's Task 3 (`secret<N>`) is committed and runs beside it; stream M follows the genesis's plan, Tasks 15,
   16, 33 (in crypto's own tests) and 34, as `../../genesis/reference/plan.md`'s revision of
   2026-11-17 says.
3. Wave 2, after stream A merges: each of increments 3 to 8, 10 and 11 writes its tasks in full
   first, then runs as its own stream (design D35), rebasing and regenerating with
   `generate.py` after every merge. Wave 3: 9 after 6, 12 after 8.
4. Record progress in the untracked ledger
   `.superpowers/sdd/WEBCPP_CRYPTO_FOUNDATION/progress.md` of the worktree, and check a box of
   `../tasks.md` only for work done and committed.
