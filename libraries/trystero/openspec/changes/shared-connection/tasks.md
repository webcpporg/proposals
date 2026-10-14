# Tasks

Resumed on 2026-10-14 against library main `957eae5`. The original patch history below is
preserved; new tasks are checked only after their changes are committed. The owner resolved
the authentication contract conflict on 2026-10-14 by approving immutable app password domains, preserving the JS
0.26.0 wire. See `reference/password-domains.md`; its implementation, local checks and
integration are recorded below. A release remains required before the proposal can become `implemented` and be archived. No CI may run in
this session.

The checklist follows `reference/plan.md` in its order and numbering (22 tasks in five
increments). Tasks through 5c are recorded in the original 25 patches, `0001` to `0025`, based
on `a7d04b8`. Task 6 had stopped uncommitted on 2026-10-11. Recovery and subsequent commits
now live on `resume-shared-connection`, based on current main; checked resumed tasks name
their new commits below. See `reference/STATUS.md` for the original handoff and current evidence.

## 1. Baseline (plan: Task 0)

- [x] 1.1 The worktree of the superproject, the library's branch `shared-connection` from `a7d04b8`, the ledger, the baseline of every lane (plan: Task 0; no commit in the library)

## 2. Increment 1: the connection layer, one room per app (plan: Tasks 1 to 4)

- [x] 2.1 The memory backend's hooks: `std::size_t` ids, `on_made`, `peer_of`, the frame filter (deliver, drop, hold) with `release`, `defer_opens` and `open`, `inject`, `fail`, a buffering channel draining `drain_per_post` bytes per post, the virtual clock with `advance` and `max_steps`, and their tests (plan: Task 1; patches `0001`, `0002`)
- [x] 2.2 `detail::connection` and `detail::binding`: decoding of every envelope, the remote's advertised tokens, kept frames, routing by token, the drain order, the end of a connection failing every waiter, and their tests, and the explicit search of the libraries in `build.jam` (plan: Task 2; patches `0003` to `0006`)
- [x] 2.3 `app_config`, `room_options` and `detail::app_impl`: the self id, the registry, adoption that ends a stale connection first and is re-entrant without recursion (plan: Task 3; patches `0007` to `0010`)
- [x] 2.4 The room on the connection layer, `peer_link` removed, `presence(0)` and kicks scoped to the room, a connection's end naming the connection (plan: Task 4; patches `0011` to `0017`)

## 3. Increment 2: the app (plan: Tasks 5a to 12)

- [x] 3.1 The tests' shared helpers (`test/support/rooms.hpp`) and the virtual clock in them (plan: Task 5a; patches `0018` to `0020`)
- [x] 3.2 The two new errors `already_joined = 14` and `too_many_rooms = 15` (plan: Task 5b; patch `0021`)
- [x] 3.3 `app`, `app::join`, the standalone room and `room_config` removed, every test, example and interop peer moved to `app`, the setup reports delivered once to the current handler (plan: Task 5c; patches `0022` to `0025`)
- [x] 3.4 Binding by presence and by kept frames, safely (plan: Task 6; resumed commit `cc147e3`)
- [x] 3.5 Signaling reuse, the binding conditions and a bound room's negotiation (plan: Task 7; commit `de28778`)
- [x] 3.6 Departures, kicks and a connection's end, one room at a time (plan: Task 8a; commits `9a6a7ba`, `fff06e3`)
- [x] 3.7 The farewell, joining during it and the app's users (plan: Task 8b; commit `9a6a7ba`)
- [x] 3.8 Rebinding times, the lower id's backoff and the higher id's dropping window (plan: Task 9; commit `fff06e3`)
- [x] 3.9 The handshake across a rebinding, and a kept `@_leave` (plan: Task 10; commit `fff06e3`)
- [x] 3.10 Crossed connections, flood isolation and the drain order (plan: Task 11; commit `fff06e3`)
- [x] 3.11 A passwordless app's rooms remain open within its domain; protected apps use separate
  passwords and self ids. The original mixed-password acceptance is superseded by section 8
  (plan: Task 12; original regression `f0eb96b` is adapted to the approved app-domain boundary
  in `ea184f8`).

## 4. Increment 3: allocation (plan: Task 13; commit `fff06e3`)

- [x] 4.1 `room_options::memory`, a binary payload copied once into a room's own resource, the counting test and the room whose resource returns to zero (plan: Task 13; commit `fff06e3`)

## 5. Increment 4: interop (plan: Tasks 14a to 14c)

- [x] 5.1 The interop peers' and the page's parameters, events and hooks; scenario 1, two rooms on one connection (plan: Task 14a; commit `c870c1c`)
- [x] 5.2 Sharing scenarios 2, 3, 5 and 6 (plan: Task 14b; commit `c870c1c`)
- [x] 5.3 Security and timing scenarios 4, 7, 8 and 9 (plan: Task 14c; commit `c870c1c`)

## 6. Increment 5: the pages (plan: Tasks 15 and 16)

- [x] 6.1 The page, the rulebook and the header comments: differences, roadmap, usage, protocol, design, allocation, guarantees, testing, overview, quick start, browser, native, README, AGENTS.md (plan: Task 15; library `fae4eef`, superproject `ea183f0`)
- [x] 6.2 Complete local compiler, oracle, interop, sanitizer, documentation and lint checks,
  whole-branch review against the amended specification, and controller gates G1/G2
  (plan: Task 16; library PR #1 and superproject PR #3). CI is omitted by the owner's instruction;
  platform lanes unavailable locally are represented by the documented compiler lanes.

## 7. Beyond the plan

- [x] 7.1 Recover and rebase the branch onto current main `957eae5`, which includes genesis `160f431`, dropping patch `0022` as superseded by the published mesh fix

## 8. Owner amendment: immutable password domains (2026-10-14)

The historical reference plan remains unchanged. These tasks supersede its per-room password
API and mixed-password shared-connection assumptions; every validation is local, with no CI.

- [x] 8.1 Add app_config::password after self_id, copy it into the app resource, remove
  room_options::password and use the app password throughout signaling and room handshakes.
- [x] 8.2 Migrate examples, ordinary tests, allocation helpers and native tests to app passwords;
  verify that no per-room override exists and a changed password requires a new app.
- [x] 8.3 Adapt shared tests: same-password rooms share, different-password apps with distinct
  self ids isolate connections and presence in both id orders, and malformed room proofs after
  successful domain authentication exercise the existing backoff and recovery bounds.
- [x] 8.4 Adapt both interop fixtures and scenarios to domain isolation and corrupted proofs,
  preserving Trystero JS 0.26.0 protocol values and reporting its mixed-password-page limitation.
- [x] 8.5 Update active proposal, design, requirements and library docs, including migration,
  resource ownership, invariant scopes and the unchanged upstream one-way handshake caveat.
- [x] 8.6 Run local unit, allocation, exceptions-off, hardened, sanitizer, browser/native interop,
  oracle, strict documentation and lint checks; resolve findings and record evidence.
- [x] 8.7 Review the complete implementation against the amended specification, then integrate
  with controller gates G1/G2, preserving original reference artifacts.

Implementation amendment and review fixes are committed in library `ea184f8` and merged as
`2e2dcd4`; superproject integration is PR #3. Final local interop is native 27/27 and browser
24/24. All 50 shared cases pass in the applicable compiler, exceptions-off, hardened and
sanitizer variants. See `reference/STATUS.md` for scope and limitations.

## 9. Release follow-up

- [ ] 9.1 Publish the next Trystero release through the release workflow, record that version,
  then mark the proposal `implemented` and archive it according to CONTRIBUTING. Release
  publication is outside this implementation task.
