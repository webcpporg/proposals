# webcpp::beast: implementation status

Updated 2026-11-03. The implementation is published and under integration review;
the formal proposal remains `draft`, with no acceptance date, release or tag.
The original 53-application inventory and all target requirements remain in scope.

## Current committed snapshot

- Repository: [webcpporg/beast](https://github.com/webcpporg/beast).
- Commit: [`4da3e791c30a5ffb2598009828ab0a2b59e8d58c`](https://github.com/webcpporg/beast/commit/4da3e791c30a5ffb2598009828ab0a2b59e8d58c).
- `source/` is an exact `git archive` of that commit: 772 tracked files, with
  every file's Git blob identity and executable mode checked against the commit.
  No uncommitted implementation files or build artifacts are included.
- The superproject candidate registers the published library and its shared prerequisites
  in [pull request 7](https://github.com/webcpporg/webcpp/pull/7).
- Final candidate CI [41588828531](https://github.com/webcpporg/webcpp/actions/runs/41588828531)
  targets superproject commit `a8064a1c3907bc16a70930f12a1f92f9fef27632`;
  its result is pending.
  Earlier candidate runs [41587540931](https://github.com/webcpporg/webcpp/actions/runs/41587540931)
  and [41587845453](https://github.com/webcpporg/webcpp/actions/runs/41587845453)
  were cancelled because they preceded later reviewed corrections. They do not establish
  a green final candidate.

The reference snapshot will be refreshed if further reviewed implementation commits land.
This status does not turn publication of source into formal proposal acceptance or release.

## Implemented and measured

The committed [verification guide](source/doc/verification.adoc),
[application matrix](source/doc/application-matrix.adoc), and
[browser differences](source/doc/browser-differences.adoc) contain the reproduction
commands, adaptations and backend boundaries. Local evidence was collected with Boost
1.92, native Clang 21 on macOS ARM64, wasi-sdk 34, Wasmtime 50.0.1, Emscripten 6.0.11,
Node 26 and actual full Chrome. It does not imply Windows or Linux execution.

| Workstream | Committed implementation and observed evidence |
| --- | --- |
| Portable/native interfaces | Upstream messages, parsers, serializers and buffers; explicit native/WASIp3 socket and TLS layers; real HTTP/WebSocket exchanges, constructor conversions and error-policy checks. Portable data builds cover native, WASIp2/P3 and Emscripten, with exceptions on/off. |
| Browser transport | Browser WebSocket/fetch, synchronous suspension and completion-token APIs; URL/TLS intent, ERROR/IGNORE, deadlines, cancellation, destruction, response reuse and separate guest byte-stream adapters. Actual Chrome and Node exercise JSPI and Asyncify; Node is not a browser-policy substitute. |
| Browser environment | All eight actual Chrome JSPI/Asyncify, ERROR/IGNORE and EH on/off combinations pass the HTTPS/WSS suspension probe: at least three timer and animation callbacks while the native peer withholds bytes, nested-entry rejection followed by successful outer operations, and mixed-content rejection with an HTTP-origin positive control. |
| Browser ownership | Caller/associated allocator witnesses, queued-event generation checks, bounded ingress before executor dispatch, same-object reuse and cancelled-open regressions. O2 and O3 are settled in `../design.md`. |
| WASI host HTTP | Independent P2/P3 worlds; reusable request/response/header/body/trailer ownership in webcpp::wasi; Beast client/server adapters, readiness-driven completion tokens, cancellation and failure after response commitment. Real hosted tests transfer 256 KiB messages and trailers. |
| Supplied streams | Portable callback channels and explicit WASI/POSIX stdin/stdout counterparts with partial I/O, EOF, deadlines, cancellation and SIGPIPE handling. They do not claim Unix-domain addressing or the original asynchronous scheduling model. |
| Guest TLS | Real native/WASIp3 HTTP and WebSocket TLS scenarios: trust/hostname failures, cancellation, close, truncated shutdown and deadlines. Eight scenarios across TLS 1.2/1.3 and the supported thread/EH profiles produce 112 recorded scenario executions. |
| Host TLS | Private process/profile CAs with normal verification. Six WASI HTTPS application counterparts plus the shared body/trailer contract produce 35 driver executions and 125 TLS attempts; trusted localhost succeeds and unknown issuers/wrong hostnames fail. The documentation client also exercises mutual TLS. Full Chrome uses disposable profile trust and explicit untrusted/wrong-host controls. |
| Application corpus | Every upstream Jamfile/CMake executable is reconciled at Beast commit `7c1e061f91e2ef542217b76286c314d006c0c8fc`. Immutable upstream hashes, generated adaptations, observable native oracle comparisons, crawler report/concurrency/failure tests, chat and documentation snippets remain inspectable. |
| Documentation | Diataxis AsciiDoc, compiled examples and MrDocs API comments. The final local Beast documentation build passed source checks, reference extraction and rendered HTML checks, updating 13 targets. |

The committed application matrix contains **53 applications across 23 profiles**, or
1,219 compile/run cells: **567 measured pass/pass, 220 blocked/blocked, and 432
unavailable/unavailable**. No cell is discarded from the denominator. These are
application/profile outcomes, not 567 unchanged-source ports or a count of CI jobs.
Native oracle normalization compares observable payload/status/mode results while
excluding fixture addresses and elapsed time. Each adaptation remains explicit.

## Remaining acceptance work

The checklist retains four unchecked items at this snapshot:

- **0.5:** formal proposal submission/decision is pending. Metadata remains `draft`;
  `decided` and `release` remain null.
- **0A.3 and 0A.4:** WASIp2 raw sockets and guest-owned TLS remain prerequisite work.
  Successful host HTTP/HTTPS does not implement those profiles.
- **8.3:** complete final candidate CI, the release and its tag are pending. Local
  checks, focused reviewer reruns and publication of the source do not close this gate.

Other classified limitations remain visible even where the inventory/accounting task is
complete: Boost.Context lacks a verified Wasm backend; WASIp3 cooperative EH-on lacks the
SDK exception runtime; original async DNS is replaced only in explicitly labelled local
endpoint counterparts; host HTTP has no WebSocket upgrade stream; browsers have no raw
listeners or Unix namespace; and standard-descriptor adaptation requires WASI or POSIX.
No unbounded host write can be preempted by a guest cancellation callback.

The final committed follow-up also distinguishes normal WebSocket closure in exception-off
sync/SSL server counterparts: clean closure is recorded and the server continues accepting,
while genuine read/accept/write failures retain the explicit failure path. The targeted
normal-close and marker-evidence regressions passed. The complete final WASIp3 application
rerun passed all 120 cases with an explicit successful exit. The final 247-case native
application rerun is still in progress at this closeout; it is not counted as a completed
final-candidate pass. Independent review of the frozen candidate reported no remaining
findings, but does not replace the pending runtime and CI results.


## How to resume current work

1. Read this record, `../tasks.md`, the current `../specs/beast/spec.md`, and the
   committed verification/matrix documents under `source/doc/`.
2. Review and commit any acceptance follow-up, then refresh `source/` with `git archive`
   of that exact published Beast commit and verify its blob identities. Check only
   tasks whose full criterion is satisfied by committed work and measured evidence.
3. Follow the final superproject CI run from pull request 7. Record its final result;
   fix any failed declared lane before claiming the integration gate complete.
4. Keep the formal proposal decision and release/tag separate. Do not mark the proposal
   `implemented`, archive it, or invent a release before those actions happen.
5. Regenerate the proposal README, run proposal checks/tests, and run strict OpenSpec
   validation. The historical intake below and `spec.md` remain unchanged.

The historical intake below describes what was known on 2026-10-13, not this snapshot.

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
