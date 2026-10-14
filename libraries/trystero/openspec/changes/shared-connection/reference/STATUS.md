# webcpp::trystero, shared connection: where the implementation stands, and how to resume

## Resumed execution, 2026-10-14

The owner requested completion on 2026-10-14 and explicitly prohibited CI during this session.
Work resumed in an isolated checkout of the current superproject. The library branch
`resume-shared-connection` is based on published main `957eae5`, preserving the newer mesh and
configuration fixes. The recorded patches were recovered, with the superseded interop patch
`0022` dropped before the rebase. Recorded patches and the original plan remain unchanged.

Binding by presence and kept frames, signaling reuse, farewell completion, rebinding and recent
challenge history, per-room allocation, local regressions, interop scenarios and documentation
have been implemented and committed locally through `c870c1c`. Ordinary validation is recorded
below; the authentication decision blocks integration. No feature merge or proposal archive has occurred, and the proposal
remains `implementing`.

### Acceptance blocker: authentication contract

A local executable regression, `a_protected_room_requires_proof_from_either_id`, fails the
promised password isolation for a higher-id room over an existing connection. The responder
answers the lower-id peer's challenge and accepts its ready message without receiving a proof
that the challenger knows the password. This is stronger than the already acknowledged offline
password-guess exposure. The pinned Trystero JS handshake has the same one-way structure.

The unchanged-wire compatibility requirement and guarantee 11 cannot both be met for arbitrary
passwords on rooms that share a connection. The owner has been asked to choose mutual
authentication with a revised protocol, or JS 0.26.0 compatibility with sharing restricted by
password. Do not merge, mark implemented, or archive this proposal before resolving the conflict.

### Local evidence

- Final native, GCC 15 and Emscripten builds completed. All ordinary tests passed, including
  allocation and hardened variants; only the bilateral-authentication regression fails in each
  shared variant. There are 44 shared cases: 43 pass and one exposes the unresolved contract.
- Address and undefined sanitizers (`norecover`) passed for connection, app_impl and all 43
  non-authentication shared cases after the review fixes.
- The oracle remained byte-identical; no protocol constant or layout was changed.
- Native loopback tests passed after enabling local loopback networking for the sandbox.
- Strict MrDocs, doc-check and rendered documentation checks passed.
- Final review reproduced log-callback reentrancy (an ASan heap-use-after-free), same-binding
  drain waiter bugs and malformed challenge nonces. Fixes and regressions pass in `fff06e3`.
- `fff06e3` adds backoff, challenge history and room allocation; `f0eb96b` separately preserves
  the authentication regression; `fae4eef` and superproject `ea183f0` record the documentation.
- The largest-frame regression retains 64 of 65 frames of 16,515 bytes and expires them at 10 s.
- Final static analysis and global text/style rules are clean; Jamroot tests passed.
- Text payload clarification: pmr strings copy once from byte storage; only binary vectors can
  move their matching-resource buffer. The historical specification's vector-to-string move
  wording is not implementable, so the public documentation states the actual guarantee.
- Interop commit `c870c1c`: native full run passed 27/27. Browser full run passed 23/24;
  its transient-handshake scenario exposed a concurrent-room setup race. After correcting that
  precondition, the final transient scenario passed separately on both lanes (about 53 s each).
  A unified final browser full-suite pass has not been claimed.
- Corrected fixtures keep room1 established, wait out initial relay announcement warmup for
  password pacing, correlate every flood ping with completion, measure idle closure from final
  departure and capture connection baselines before delayed room3 joins. Original bounds remain.
- No CI, push, pull request, merge, upstream report or proposal archive has occurred.

### Resume from the current branch

Resolve the authentication contract first, keep its negative regression, complete final local
validation and whole-branch review, then integrate the library before updating the superproject's
submodule. Update this execution record and the committed task checklist before archiving.

## Historical handoff, 2026-10-13

The following text records the original handoff and is historical. Its branch names, worktree
paths and pending tasks describe that earlier state, not the resumed execution above.

Written 2026-10-13, when this proposal took the work in. `spec.md` is the specification of
2026-10-11 (approved by the owner that day); `plan.md` is the plan (22 tasks, its frontmatter still
says `draft`, its checkboxes are all unchecked: the ledger, not the plan, recorded progress);
`../tasks.md` is the checklist; `patches/` holds the branch's 25 commits as patches, `0001` to
`0025` in order, applying on `a7d04b8` (an ancestor of the published `160f431`); and
`uncommitted.diff` is `git diff HEAD` of the branch's working tree on 2026-10-13, task 6's
stopped work on `room.hpp` and `shared_test.cpp`. A change carries only its diff from the commit
it was requested against, never the library's whole tree: the tree at the branch's tip is
`a7d04b8` with the patches applied.


`plan.md` was written before the history of webcpp was rewritten on 2026-10-12 and 2026-10-13
(the commit identity became R), so the hashes it cites are the earlier ones: its `8e8911f` is
`a7d04b8` today, the same commit, an ancestor of the published `160f431`.

## Where it stands

The work is a **local branch, unpublished**: the library's repository in the worktree
`webcpp-trystero-shared` of the superproject (branch `trystero-shared`), branch `shared-connection`
of `libs/trystero`, tip `84d4eeb` of 2026-10-11, 25 commits as `R` on `a7d04b8`. Nothing was
pushed; no CI ran. `a7d04b8` precedes the genesis commit `160f431` (the changelog, `config.hpp`
including `<webcpp/config.hpp>` and `WEBCPP_NO_EXCEPTIONS`, the maintainer's public address, and
the interop flood fixes `d2563a6` and `571a2e4` came after it), so the branch must be rebased
onto `160f431` before its gates. The controller's ledger records that on 2026-10-11 at 18:50 the
owner stopped every task to review them one by one, with task 6 stopped mid-work.

| Increment | Task | State |
| --- | --- | --- |
| 1 | 0 baseline | done |
| 1 | 1 the memory backend's hooks | done, patches `0001`, `0002` |
| 1 | 2 `detail::connection` and `detail::binding` | done, patches `0003` to `0006` |
| 1 | 3 `app_config`, `room_options`, `detail::app_impl` | done, patches `0007` to `0010` |
| 1 | 4 the room on the connection layer, `peer_link` removed | done, patches `0011` to `0017` |
| 2 | 5a shared test helpers and the virtual clock in them | done, patches `0018` to `0020` |
| 2 | 5b `already_joined`, `too_many_rooms` | done, patch `0021` |
| 2 | 5c `app`, `app::join`, `room_config` and the standalone room removed | done, patches `0022` to `0025` |
| 2 | 6 binding by presence and kept frames | started: the working tree of the branch held uncommitted edits to `room.hpp` and `shared_test.cpp` when this proposal was written; they are `uncommitted.diff` |
| 2 | 7 to 12 | not started |
| 3 | 13 `room_options::memory`, payload copied once | not started |
| 4 | 14a to 14c interop scenarios | not started |
| 5 | 15 pages, 16 gates | not started |

What exists at the branch's tip: `include/webcpp/trystero/app.hpp` (`class app`, `max_rooms` 64, `join`,
`on_error`, move and destruction that release the state), `options.hpp` (`app_config`,
`room_options` without `memory`, `default_ice_servers`), `detail/app_impl.hpp`,
`detail/connection.hpp` (connection and binding), `error.hpp` with the two new errors, `room.hpp`
without `room_config` and with the connection layer, the extended memory backend and
`test/support/rooms.hpp`, and the tests `memory_backend`, `connection`, `app_impl` and `shared`
(10 cases: the app's id in every room, the setup reports, refusal of a second join and of the 65th
room, two apps of one id never meeting, two and three rooms on one connection, a second connection
dropped, and the moved-from, destroyed and assigned app). What the branch does not have yet:
binding by presence and by kept frames in the rooms, signaling reuse, the binding conditions, the
rebinding schedule, the acceptance of the last 8 challenges, `room_options::memory`, the interop
scenarios and the pages. The `shared_hardened` and `shared_no_exceptions` targets exist.

### What was measured

Nothing was run for this proposal. What the controller's ledger records (not re-run here): at
task 5c's fix round, on 2026-10-11, native 49 tests, Emscripten 57, the gcc-15 gate 9 of 9, the
oracle byte-identical, interop native 19 of 19 and Emscripten 16 of 16, the sanitizer run
(address and undefined, `norecover`) of `shared` and `connection`, lint clean, doc-check clean,
15 of 15 runnable mutants killed. Each earlier task has a recorded review and fix rounds in the
same ledger (mutants killed per task: 28 of 28 runnable for task 1, 30 of 41 for task 2 with every
survivor but one argued undetectable, 15 of 15 for task 5c).

## Open items

1. Tasks 6 to 16 of the plan (`../tasks.md`).
2. The rebase onto `160f431`, dropping `6b1e4ba` (cherry-picked as patch `0022`) for the owner's
   ruling that `4aef1c0` supersedes it.
3. The gates G1 (push `shared-connection` to `webcpporg/trystero` and merge) and G2 (bump the
   submodule in the superproject, together with the superproject's `jamroot_test` check that the
   explicit library search needs), both the controller's, with the owner's agreement.
4. An upstream report of the password exposure of the shared connection (listed among the pending
   reports); the spec's ruling 4.
5. The documentation: every page and file section 7 of the spec lists, and the migration page.

## How to resume

1. Work in the worktree `webcpp-trystero-shared` of the superproject on branch `trystero-shared`
   (or make a new one; `libs/trystero` on `shared-connection`), with `.local/` linked to the main
   checkout's, as the plan's Global Constraints say. Run every b2 command from the worktree with
   the user config and `-u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH`.
2. Read `spec.md` sections 4, 6 and 9 and the plan's "Conflicts with the spec" table (18 rulings
   where the plan departs from the text) before any task: the plan, not the spec, fixes the
   task-level choices.
3. Take the next task, task 6: its brief is in the controller's ledger directory; the red test is
   `shared_test.cpp` cases 5 and 12 with two rooms. Check the uncommitted edits of the branch
   before starting; they are the first draft of that task.
4. Each task: a failing test first, the implementation, the lanes (native, Emscripten, gcc-15
   core tests, the sanitizer run), the lint, a review; commits as `R` without a trailer, staged by
   path.
5. After increment 5: every lane from scratch, a whole-branch review against the spec, then the
   gates.
