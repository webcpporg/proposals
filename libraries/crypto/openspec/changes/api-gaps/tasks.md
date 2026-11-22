# Tasks

Each task points to its section in `reference/plan.md`. The change starts once the change
`data-independent-timing` is merged; the foundation's increment 4, part A (the asynchronous
family) starts after this change merges.

## 1. GCM

- [x] 1.1 A failed seal and a failed `update` zero their data, on every backend (plan: Task 1)
- [x] 1.2 The incremental open in two passes: `begin_open`, `authenticate`, `verify`, `decrypt`,
  `end_open`, its coverage, example and section (plan: Task 2)

## 2. Defaults and secp256k1

- [x] 2.1 The AES block cipher's default per target, `backends::family_default`, the generated
  refusals and alias (plan: Task 3)
- [x] 2.2 `secp256k1::basic_keypair::generate` (plan: Task 4)
- [x] 2.3 `pubkey_negate` returns `result<public_key>`; ECDH's three types pinned (plan: Task 5)

## 3. The constant-time lane

- [x] 3.1 dudect's case lines, evidence, A/A controls and `--case` (plan: Task 6)
- [x] 3.2 The CI's judge: every output printed, warnings at 10, two of three fresh runs on hosted
  runners (plan: Task 7)

## 4. Delivery

- [x] 4.1 Full local verification: five lanes, the constant-time lane, the lints, the MSVC lane
  (plan: Task 8, Steps 1 and 2)
- [x] 4.2 The changelog clause, the generated tables checked, the pull requests, the merges
  (plan: Task 8, Steps 3 and 4)
- [ ] 4.3 The hosted constant-time lane's first judged run, read and recorded here (deferred:
  the owner's rule of 2026-11-20 runs the CI at the end of the queue, in the final measurement
  pass, `design.md` "As built")
