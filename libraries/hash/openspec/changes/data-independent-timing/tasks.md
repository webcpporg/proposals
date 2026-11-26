# Tasks

Each task points to its section in `reference/plan.md`. The change starts after webcpp::hash's
increment 6 (Botan) merges (design D8).

## 1. The guard

- [x] 1.1 `detail/timing.hpp` (hash's copy, with the engaged flag), the seam, the test support, the guard's own tests (plan: Task 1)

## 2. Every keyed function holds the guard

- [x] 2.1 `mac` on every backend, and the keyed modes of BLAKE2b and BLAKE3 (plan: Task 2)
- [x] 2.2 The key derivations and the password hashes (plan: Task 3)
- [x] 2.3 The coverage test per backend, driven by the capability table, with the unkeyed boundary (plan: Task 4)
- [x] 2.4 The asynchronous forms on the thread that computes (plan: Task 5)
- [x] 2.5 crypto's test of the two guards together (plan: Task 6)

## 3. The measurements, the documentation and the delivery

- [x] 3.1 The guard's cost from a release build (the throughput tables are left to the quiet pass, H16), the pages, the changelog source, the guidelines (plan: Task 7)
- [x] 3.2 Full local verification, pull requests, merge (plan: Task 8);
  CI deferred by the owner, run later by the coordinator

## Open

- [ ] The final quiet measurement pass regenerates the throughput tables (design.md, "As built");
  the guard's cost was measured again on an idle machine on 2026-11-26 (clear 37.0 ns, set 1.2 ns).
- [ ] crypto's `detail/timing.hpp` names hash's copy at crypto's next change of it.
- [ ] The MSVC lane's known-failures list gains the seven no-exceptions builds of the Boost.JSON
  class.
