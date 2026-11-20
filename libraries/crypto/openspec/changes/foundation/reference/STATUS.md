# Status of the foundation change

**Date:** 2026-11-19 (accepted by the owner on 2026-11-14; revised on 2026-11-17; increment 2
recorded as built on 2026-11-19).

## Where things stand

- **Increment 2 (stream B, `../tasks.md` group 2) is built and awaits its pull requests.** The
  library's branch `crypto/secp256k1-modules` (head `e7c71db`, 54 commits on `1fc5140`) and the
  superproject's branch of the same name (head `221357e`, 21 commits on `c00bf7a`, its
  `libs/crypto` at `e7c71db`) hold Tasks 2.1 to 2.7 and the owner's Task 2.7d; this proposals
  branch, `crypto/foundation-increment-2`, holds the tasks and their record as built. Nothing
  of it is pushed. Every task was reviewed, the whole branch had a final review and one fix
  wave, and a full local verification passed on the final heads (`plan.md`, increment 2's
  "As built"); where the code differs from the plan's text, each task's "As built" says so.
- **Increment 1 (stream A, group 1) is merged** without the CI's run: `webcpporg/crypto` `main`
  `dae9e84` and the superproject's `main` `485c5e0`, 2026-11-19, merges of stream A's final
  heads `1fc5140` and `c00bf7a`. Its boxes in `../tasks.md` wait for its own record as built.
- **Its base was ready.** crypto's version 0.1 (the genesis's groups 1 and 2) is merged:
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

Nothing of increment 2. What remains of it:

- **The pull requests** (`plan.md` Task 2.7, Step 7), opened together: this proposals branch
  (on proposals `main` `f7896ad`), the library's into `webcpporg/crypto`, then the
  superproject's (`third_party/Jamfile`, `tools/test/third_party_test.py`, the three
  `secp256k1_*_default` fixtures, the library's pin). They need no rebase: both code branches
  descend from the trees `main`'s merges of stream A hold. If the library's pull request lands
  as commits other than `e7c71db`, the superproject's `libs/crypto` is re-pointed first.
- **The CI's run** on both code pull requests, every lane it plans (Task 2.7, Step 5), the Linux
  x86-64 `constant-time` lane included, which the local verification did not run.
- **The rulebook:** `CODING-GUIDELINES.md` chapter 17's crypto entry still lists libsecp256k1's
  remaining modules among what is not built; Task 2.7's superproject commit
  `guidelines: crypto's increment 2 in chapter 17` was not made.
- **The owner's review and merge**; then task 13.1 once the CI is green on both `main`s.

## How to resume

1. Read `roadmap.md`: the overlaps resolved, the defaults, the waves and their streams.
2. Increment 2: push the three branches and open the pull requests above; fix on the same
   branches whatever the CI or the review finds, with a failing test first, and record each
   difference from the plan in its task's "As built". Stream B's ledger is the untracked
   `.superpowers/sdd/plan/progress.md` of its superproject worktree, with its `briefs/` (every
   task's brief, report and review, the rebases, the final review and verification).
3. Increment 1: its record as built (`../tasks.md` group 1, `plan.md`), from stream A's ledger.
4. Wave 2, now that stream A is merged: each of increments 3 to 8, 10 and 11 writes its tasks in
   full first, then runs as its own stream (design D35), rebasing and regenerating with
   `generate.py` after every merge. Wave 3: 9 after 6, 12 after 8.
5. Check a box of `../tasks.md` only for work done and committed.
