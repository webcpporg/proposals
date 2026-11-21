# webcpp::trystero, port to Trystero 0.27.0: where it stands, and how to resume

Written 2026-11-20. Nothing is implemented. `plan.md` is the plan (Phase 1 in full, Phases 2
and 3 outlined); `vectors/record.mjs` is the recorder and `vectors/phase1.json` what it recorded.

## Where it stands

| What | Where |
| --- | --- |
| The plan | `plan.md` |
| The vectors | `vectors/phase1.json`, recorded 2026-11-20 with node v26.13.0 from Trystero 0.27.0 at `8d3f57e` |
| The code | none; branches `port/trystero-0.27` (library) and `trystero/port-0.27` (superproject) are created by Task 1 |
| Blocking Phase 2 | webcpp::crypto's foundation increment 3, P-256 (its plan's Tasks 3.1 to 3.3) |

## How the vectors were recorded

    node reference/vectors/record.mjs <trystero 0.27.0 checkout> > reference/vectors/phase1.json

The recorder imports 0.27.0's TypeScript sources directly (node strips the types), resolves
extensionless relative imports to `.ts`, maps `@trystero-p2p/core` to `primitives.ts` and stubs
`@noble/secp256k1` for `nostr/src/protocol.ts`, whose `topicToKind` it calls. It replaces
`crypto.getRandomValues` only inside the calls whose output it pins (GCM's IV, `genId`'s bytes).
It runs in about half a second. Task 8 extends it with the action wire's frames.

## How to resume

Start at `plan.md` Task 1. Phase 2 is written in full, task by task, when webcpp::crypto's P-256
part merges; its first task (2.1) confirms or amends design D9 before any code of the shared
connection is removed.
