# Tasks

Each task points to its section in `reference/plan.md`.

## 1. The guard and its tests

- [x] 1.1 `detail/timing.hpp`: `cpu_has_dit`, `dit_is_set`, `timing_guard`, the seam; its unit test (plan: Task 1)

## 2. Every family holds the guard

- [x] 2.1 The ciphers, `equal` and the hybrid boxes (plan: Task 2)
- [x] 2.2 Ed25519, X25519, kx, their formats and PEM (plan: Task 3)
- [x] 2.3 secp256k1 and its groups (plan: Task 4)
- [x] 2.4 The coverage test over every family and backend (plan: Task 5)

## 3. The lane, the measurements and the documentation

- [x] 3.1 The constant-time lane without the harness's DIT (plan: Task 6)
- [x] 3.2 The guard's cost, the throughput tables, the pages and the changelog source (plan: Task 7)
- [x] 3.3 Full local verification, pull requests, merge, one CI (plan: Task 8);
  CI deferred by the owner, run later by the coordinator

## Open

- [ ] The guard's cost and the throughput tables, measured under load, are measured again in a final
  pass on a quiet machine after the streams of 2026-11-20 merge (design.md, "As built"); the pass
  also measures the guard again now that it takes no `std::source_location` outside tests, writes
  UTC dates, regenerates the release size matrix (`generate.py --sizes`), and tests which barrier
  each DIT write needs (Apple's `sb` on entry, none on exit, against the guard's two `isb`) with
  dudect and the cost
