# Status of the foundation change

**Date:** 2026-11-14 (accepted by the owner the same day).

## Where things stand

- Nothing of this change is built. Every task of `../tasks.md` is open.
- Its base is not ready yet: crypto's version 0.1 (the genesis's groups 1 and 2) is being
  built in the superproject's work tree and has not merged, and webcpp::hash's genesis
  (`libraries/hash/openspec/changes/genesis/`), which builds `/third_party//sodium`, is
  proposed beside this change and not built.
- Pins checked with `gh api` on 2026-11-14: libsodium 1.0.22 is the latest stable release
  (`77e1ce5d6dee871c49ef211222ba18ef0c486bda`, 2026-04-09), the pin webcpp::hash uses; blst's
  latest release is v0.3.17 (`54e6e55674722fc2797ebb4bbb71b26d881eb4b8`, 2026-07-24); Botan's
  latest 3.x tag is 3.14.1 (`01af88c039fcfaedc418b1af4d6d3a139674e591`, 2026-11-06);
  libsodium's documentation `jedisct1/libsodium-doc` master is
  `6d1ea27b79b853b2a04f09ae9e1a8231c90e3b56` (ISC).
- An upstream fact found while writing: libsodium 1.0.22 has the ristretto255 and
  edwards25519 maps from hashes, and not the full RFC 9380 `crypto_core_*_from_string`
  functions, which are on libsodium's `master` since 2026-02-20 and in no release (design
  D17).

## What to redo

Nothing: no work was done.

## How to resume

1. Check the base: `crypto` version 0.1 merged on the superproject's `main` (the genesis's
   Tasks 25 and 30 done), and webcpp::hash's Tasks 1 to 3 and 8 merged
   (`/webcpp/hash//hash`, `/third_party//sodium`). If either is missing, increment 2 may
   still start (it needs only version 0.1); increment 1 waits.
2. Follow `plan.md` from Task 1 of increment 1, with superpowers:subagent-driven-development.
   Start increment 2 on its own branch beside it once version 0.1 has merged; write its tasks
   in full first, from the outline at the end of `plan.md`.
3. After increment 1 merges, write each of increments 3 to 8 in full task form and run them
   as parallel streams (design D21), regenerating the generated files after every rebase.
4. Record progress in the untracked ledger
   `.superpowers/sdd/WEBCPP_CRYPTO_FOUNDATION/progress.md` of the worktree, and check a box of
   `../tasks.md` only for work done and committed.
