# webcpp::beast: implementation status

Updated 2026-11-02. The owner authorized implementation and corrections for the
previous superproject CI failures. `plan.md` records the implementation sequence.
The original 53-application acceptance corpus and all target requirements remain.

## Initial local implementation

A local repository exists at `asio-task9-review/libs/beast` in the organization's
isolated checkout. It has no organization remote or superproject gitlink yet.
The organization repository is still to be created by the owner.
Its first commit is `c7dfe2bffb16559aa4f5a6e3bece633e6999b0b4`; `source/` is an exact archive
of its tracked tree.

- Portable upstream HTTP messages, parsers, serializers and buffers compile and run
  on native, WASIp2, WASIp3 and Emscripten, with exceptions on and off.
- Native and WASIp3 socket backends passed real loopback HTTP, binary WebSocket echo,
  close handshake and timeout tests, plus optional TLS SNI validation.
- WASIp3 cooperative threading passed with exceptions off using Wasmtime 50.0.1;
  SDK 34 has no cooperative C++ exception runtime. Initial comparisons showed that
  Wasmtime 47.0.3 rejected the cooperative components while 49.0.3 ran them. The
  project-wide runtime and current regression runs now use 50.0.1.
- Emscripten tests here exercise data under Node, not browser networking.
- Constructors preserve defaultable layers, same-type move, executor conversion and
  facade rebinding; independent review found the missing cases and they were fixed.
- The guide uses Diataxis, AsciiDoc source inclusions and MrDocs API extraction.
  Exact commands and limitations are in `source/doc/verification.adoc`.

O1 now selects explicit transport/TLS headers and a data-only convenience header.
O6 follows the recommended upstream type reuse after authorization to implement.
This makes no dependency-free Wasm claim and does not make host-service execution
use the upstream protocol engine. O2 and O3 remain browser implementation decisions.

## Remaining acceptance work

1. Complete browser WebSocket and fetch adapters, including lifetime, cancellation,
   allocator, unsupported-setting and JSPI/Asyncify tests in a real browser.
2. Implement WASI host HTTP and supplied-stream applications without requiring guest
   Asio sockets or OpenSSL for that profile.
3. Complete Beast-level TLS/cancellation tests and WASIp2 socket/TLS prerequisites.
4. Probe synchronous browser suspension and Boost.Context before declaring support.
5. Implement and validate all 53 upstream applications or explicitly classified
   counterparts, then publish the complete target/variant/backend feature matrix.
6. Create the organization repository and register its published commit before adding
   the superproject gitlink. No release or new remote CI run is claimed.

The plan's first two tasks are the initial increment, not completion of this proposal.
The retained historical section below does not describe the current source state.

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
