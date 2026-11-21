# Tasks

Each task points to its section in `reference/plan.md`. The change starts after webcpp::hash's
increment 6 (Botan) merges (design D8).

## 1. The guard

- [ ] 1.1 `detail/timing.hpp` (hash's copy, with the engaged flag), the seam, the test support, the guard's own tests (plan: Task 1)

## 2. Every keyed function holds the guard

- [ ] 2.1 `mac` on every backend, and the keyed modes of BLAKE2b and BLAKE3 (plan: Task 2)
- [ ] 2.2 The key derivations and the password hashes (plan: Task 3)
- [ ] 2.3 The coverage test per backend, driven by the capability table, with the unkeyed boundary (plan: Task 4)
- [ ] 2.4 The asynchronous forms on the thread that computes (plan: Task 5)
- [ ] 2.5 crypto's test of the two guards together (plan: Task 6)

## 3. The measurements, the documentation and the delivery

- [ ] 3.1 The guard's cost and the throughput tables from release builds, the pages, the changelog source, the guidelines (plan: Task 7)
- [ ] 3.2 Full local verification, pull requests, merge, one CI per repository (plan: Task 8)
