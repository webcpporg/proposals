# webcpp proposals

Every library of [webcpp](https://github.com/webcpporg/webcpp), and every change to one, begins
here as a proposal: an [OpenSpec](https://github.com/Fission-AI/OpenSpec) specification with its
design, its tasks and the reference code an implementer starts from. The organization's own
proposal, `organization`, says how webcpp is organized and how a library is built inside it; read
it before anything else.

[CONTRIBUTING.md](CONTRIBUTING.md) says how to propose a library or a change, and how a proposal
is decided.

## Layout

The organization and each library are OpenSpec roots of their own:

```
organization/openspec/            the superproject, its build, documentation, CI, releases, site
libraries/<name>/openspec/
  specs/<name>/spec.md            the library's specification as implemented
  changes/genesis/                the proposal of a new library, while it is open
  changes/<change>/               a proposal that changes an existing library
  changes/archive/<date>-<id>/    every proposal implemented, rejected or withdrawn
```

A new library is proposed by its `genesis`; a change to an existing library is a proposal of its
own, beside it. Every proposal holds `proposal.md` (why, what, impact), `design.md` (the
decisions), `tasks.md`, `specs/<capability>/spec.md` (the requirements, each with scenarios),
`.openspec.yaml` (its status and release) and `reference/` (the reference code, the original
specification and plan, and `STATUS.md`, where the work stands and how to resume it).

## Proposals

The table is written by `python3 tools/proposals.py readme` from every proposal's
`.openspec.yaml`, and the CI fails when it is not current. Release 0.1.0 is not tagged yet: the
libraries marked with it are published, in the superproject's `main`.

<!-- proposals:begin -->
| Library | Proposal | Title | Kind | Status | Release | Decided | Implementation |
| --- | --- | --- | --- | --- | --- | --- | --- |
| organization | [genesis](organization/openspec/changes/archive/2026-10-07-genesis) | organization: the superproject, its build, its rules, its CI and its site | organization | implemented | 0.1.0 | 2026-10-07 | [code](https://github.com/webcpporg/webcpp/tree/c53f3f9) |
| organization | [agent-skills](organization/openspec/changes/agent-skills) | organization: agent skills in .agents/skills, in the proposals and the superproject | organization | draft |  |  | [code](https://github.com/webcpporg/webcpp) |
| organization | [contributor-identity](organization/openspec/changes/contributor-identity) | organization: every contributor commits under their own git identity | organization | draft |  |  | [code](https://github.com/webcpporg/webcpp) |
| organization | [site-increments-4-5](organization/openspec/changes/site-increments-4-5) | organization: the site, increments 4 and 5 (trystero's JavaScript API and the cursors; the state machine, on the server, C++ calls the browser) | organization | accepted |  | 2026-10-10 |  |
| asio | [genesis](libraries/asio/openspec/changes/genesis) | asio: Boost.Asio natively, the browser's event loop on Emscripten | wrapper | implementing |  | 2026-10-11 |  |
| beast | [genesis](libraries/beast/openspec/changes/genesis) | beast: Boost.Beast natively, the browser's WebSocket and fetch on Emscripten | wrapper | draft |  |  |  |
| config | [genesis](libraries/config/openspec/changes/archive/2026-10-09-genesis) | config: webcpp's configuration, the target, exceptions and version every library includes first | own | implemented | 0.1.0 | 2026-10-09 | [code](https://github.com/webcpporg/config/tree/2fad162) |
| crypto | [genesis](libraries/crypto/openspec/changes/genesis) | crypto: authenticated encryption, randomness and secp256k1 over C backends | wrapper | accepted |  | 2026-10-11 |  |
| document | [genesis](libraries/document/openspec/changes/archive/2026-10-12-genesis) | document: a JSON document for C++20 that behaves as JavaScript does, in one relocatable image | own | implemented | 0.1.0 | 2026-10-12 | [code](https://github.com/webcpporg/document/tree/ac29296) |
| idna | [genesis](libraries/idna/openspec/changes/archive/2026-10-09-genesis) | idna: internationalized domain names for C++20 | own | implemented | 0.1.0 | 2026-10-09 | [code](https://github.com/webcpporg/idna/tree/e2452e5) |
| json_schema | [genesis](libraries/json_schema/openspec/changes/genesis) | json_schema: a JSON Schema validator for every dialect | own | draft |  |  |  |
| lexical | [genesis](libraries/lexical/openspec/changes/archive/2026-10-09-genesis) | lexical: the lexical forms of JSON Schema's string formats | own | implemented | 0.1.0 | 2026-10-09 | [code](https://github.com/webcpporg/lexical/tree/e286ca4) |
| lexical | [strategies](libraries/lexical/openspec/changes/strategies) | lexical: one entry point, strategies and Boost.Regex | own | draft |  |  |  |
| pratt | [genesis](libraries/pratt/openspec/changes/archive/2026-10-08-genesis) | pratt: a generic Pratt parser engine for C++20, with a decimal calculator | own | implemented | 0.1.0 | 2026-10-08 | [code](https://github.com/webcpporg/pratt/tree/d78451b) |
| trystero | [genesis](libraries/trystero/openspec/changes/archive/2026-10-09-genesis) | trystero: serverless WebRTC rooms, wire compatible with Trystero | port of Trystero 0.26.0 | implemented | 0.1.0 | 2026-10-09 | [code](https://github.com/webcpporg/trystero/tree/160f431) |
| trystero | [bounded-json-parse](libraries/trystero/openspec/changes/bounded-json-parse) | trystero: bound the text and the objects a room parses | port of Trystero 0.26.0 | draft |  |  |  |
| trystero | [shared-connection](libraries/trystero/openspec/changes/shared-connection) | trystero: one connection shared by the rooms of an app | port of Trystero 0.26.0 | implementing |  | 2026-10-11 |  |
| wasi | [genesis](libraries/wasi/openspec/changes/archive/2026-10-08-genesis) | wasi: the HTTP handler a WebAssembly component exports, for C++ on wasip2 and wasip3 | own | implemented | 0.1.0 | 2026-10-08 | [code](https://github.com/webcpporg/wasi/tree/6e3c529) |
| wasi | [redesign](libraries/wasi/openspec/changes/redesign) | wasi: one namespace per WASI version, one entry point, target features and run scripts | own | accepted |  | 2026-10-09 |  |
| webrtc | [genesis](libraries/webrtc/openspec/changes/genesis) | webrtc: the W3C WebRTC API in Asio's model, native and in the browser | own | draft |  |  |  |
| xactor | [genesis](libraries/xactor/openspec/changes/archive/2026-10-07-genesis) | xactor: a deterministic actor system for C++20 | own | implemented | 0.1.0 | 2026-10-07 | [code](https://github.com/webcpporg/xactor/tree/62f3e77) |
| xstate | [genesis](libraries/xstate/openspec/changes/archive/2026-10-07-genesis) | xstate: XState's state machines and actors for C++20 | port of XState 5.33.2 (TypeScript, MIT) | implemented | 0.1.0 | 2026-10-07 | [code](https://github.com/webcpporg/xstate/tree/5299db2) |
<!-- proposals:end -->

## Statuses

| Status | Meaning |
| --- | --- |
| draft | The specification is being written; it may lack reference code. |
| proposed | Submitted for a decision, with its reference code. |
| accepted | Accepted; not built yet. |
| implementing | Being built. |
| implemented | Built and in a release; archived. |
| rejected | Decided against; archived. |
| withdrawn | Withdrawn by its author; archived. |

## Checking the repository

```
for root in organization libraries/*; do (cd "$root" && openspec validate --all --strict --no-interactive); done
python3 tools/proposals_test.py
python3 tools/proposals.py check
```

OpenSpec is `npm install --global @fission-ai/openspec@1.10.0`.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
