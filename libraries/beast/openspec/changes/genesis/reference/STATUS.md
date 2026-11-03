# webcpp::beast: current draft and how to resume

Updated 2026-11-02. The owner clarified the primary goal: existing Boost.Beast applications
compiled to WASIp2, WASIp3 and Emscripten, with minimal changes and execution through the environment's own HTTP/WebSocket/stream facilities. Useful stream and host-HTTP adaptations are requested
for absent OS facilities. All upstream example
applications are the acceptance corpus; native Boost.Beast is the behavioural reference.

**Reference code: none. No Beast implementation, build or runtime success is claimed.**
`spec.md` below this directory is the preserved 2026-10-10 baseline. Current intent and
requirements are `../proposal.md`, `../design.md`, `../specs/beast/spec.md` and `../tasks.md`.
They supersede the baseline's WASI exclusion and three-example acceptance boundary.

## Evidence of this revision

- Read the upstream Boost 1.92 source at Beast commit
  `7c1e061f91e2ef542217b76286c314d006c0c8fc`.
- Counted 53 executable declarations in `example/**/Jamfile`, matched to 53 entry-point
  sources, including the doc SSL pair and legacy echo-op. Recorded every executable in
  `../design.md`, with support files and dependency constraints.
- Read the integrated Asio capabilities at Asio `3611a6e`, superproject `2ba5116`:
  browser and WASIp3 support exists; Asio WASIp2 does not. WASIp3 single async DNS is
  unsupported; cooperative threads use the tested SDK's EH-off runtime.
- Read webcpp::wasi's existing P2/P3 HTTP entrypoint, response helper, WIT worlds and B2
  targets. The method/target/whole-body helper needs extension for the full Beast bridge;
  host HTTP does not require the missing Asio WASIp2 socket backend or guest OpenSSL.
- Inspected the pinned `wasi:http@0.2.12` and `@0.3.0` handler interfaces and CLI stdio
  resources/streams. Added independent host-HTTP and supplied-stream application paths,
  with ownership, cancellation and explicit differences from Unix-domain sockets.
- The Beast examples have not been compiled or executed in this revision. Asio results
  are prerequisite evidence only. Candidate classifications are not passing tests.

## Pending work

1. Review this expanded draft and write the implementation plan and reference code.
2. Implement host-HTTP and supplied-stream adapters using webcpp::wasi; extend Asio and
   shared TLS for the distinct WASIp2 raw-socket profile, without making it a host-HTTP gate.
3. Validate upstream Beast over the existing WASIp3 Asio/TLS integration.
4. Probe stackful Context support and synchronous browser suspension before making promises.
5. Implement the browser WebSocket and HTTP transports, then all-example application tests.
6. Publish target/variant/backend feature and example matrices, separating original parity
   from successfully adapted applications.
7. Resolve whether Wasm can reuse Boost.Beast headers/data types or must be independent of
   them; this question is pending the owner's answer and no dependency-free claim is made.

No new implementation task is checked off. The proposal remains `draft`, with no release.
The following dated record is retained as history, not the current dependency status.

## Historical intake record, 2026-10-13

Written 2026-10-13, when this proposal took beast in as a specified library with nothing built.
`spec.md` is the specification; `../tasks.md` is the checklist; `../design.md` the decisions.

**Reference code: none yet; required before the proposal is accepted.** There is no
implementation repository, no branch, no plan and no source tree. `reference/` holds the
specification alone.

## Where it stands

| What | State |
| --- | --- |
| The specification, `spec.md` | written, draft, dated 2026-10-10, with four decisions of the owner of that date |
| The plan | not written |
| The reference code | none |
| The repository `webcpporg/beast` | not created (the owner creates it) |
| The superproject submodule `libs/beast` | not added |
| The release | none |

### What was measured

Nothing by this proposal: no code exists to run, and no command was executed against the
library. The specification states, as measured by a review of Boost.Beast 1.92 and Emscripten
6.0.11 that is not part of this proposal, that node 26 has `WebSocket`, `fetch` and
`MessageChannel` and not `WebSocketStream`, and that node enforces neither CORS nor mixed
content; and that `basic_resolver_results::create` is outside Boost's documented interface
(`GENERATING_DOCUMENTATION` guards it in `boost/asio/ip/basic_resolver_results.hpp`). None of
this was re-run here.

## Open items

1. The plan and the reference code (`../tasks.md`, 0.2 and 0.3).
2. The open questions O1 to O3 of `../design.md` (header layout, the receive queue's default and
   the options' names, how the decorator is classified).
3. The dependency on webcpp::asio: its genesis (`libraries/asio`) must provide the io_context,
   the bridge, the `ip::tcp` types and resolver (its decision 5) and the `ssl` record before this
   library's browser streams can be built or tested.
4. A probe of `basic_resolver_results::create` on the pinned Boost (`../tasks.md`, 3.1).

## How to resume

1. Read `spec.md` and `../design.md`, then the webcpp::asio proposal's specification
   (`libraries/asio/openspec/changes/genesis/reference/spec.md`), whose sections 3, 4.6, 4.8 and
   7 this library relies on.
2. Write `reference/plan.md`: each task of `../tasks.md` with its files, interfaces, steps, code
   and expected output, in the form of the other libraries' plans. Settle O1 to O3 with the owner
   first.
3. In a worktree of the superproject, create the library's tree at `libs/beast`, following the
   superproject's `AGENTS.md` (chapter "Working on a library"), and build the reference code in
   the order of `../tasks.md`: skeleton and configuration, native names, browser streams, the
   WebSocket client, cancellation and errors, then the HTTP client.
4. Prerequisites are the superproject's: Boost 1.92, b2, a C++20 compiler, Emscripten 6.0.11
   (both suspension variants), node 26 and headless Chrome for the lanes.
5. When the reference code exists, copy its tree to `reference/source/`, update this file with the
   repository, the commit, and what was measured (only what was seen), and set the proposal's
   status to `proposed`.
