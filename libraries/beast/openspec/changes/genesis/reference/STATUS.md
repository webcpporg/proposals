# webcpp::beast: implementation status

Updated 2026-11-03. The owner approved submission with status `proposed`, with
no formal acceptance date, release or tag. Published source is followed by local
integration commits. The original 53-application inventory remains in scope.

## Current committed snapshot

- Repository: [webcpporg/beast](https://github.com/webcpporg/beast).
- Published commit: `7d41654d750061ecf4b1bac873cd66af28cf8a09`.
- `source/` is an exact `git archive` of that commit: 774 tracked files, with
  every Git blob identity and executable mode checked against the commit.
- Published superproject candidate: `d7a6614516facca397a60e8c3192c3328ebcfc46`.
- Local superproject follow-up: `52c1120778b28402c9b6fdf77f1def72eae6cbd3`, not yet pushed.
- [Superproject pull request 7](https://github.com/webcpporg/webcpp/pull/7) carries
  the completed Asio capabilities. Windows/WASI driven-test fixes are committed
  locally; generated-header preparation and final CI remain in progress.
- Full CI [41597577370](https://github.com/webcpporg/webcpp/actions/runs/41597577370)
  passed for root `55a72c056de1e6522e1fc6dbc3ee2b9ef3715327` only. It does not
  establish a green result for the current local tree.

## Publication checks, 2026-11-03

Asio commit `9455076` supplies WASIp2 guest OpenSSL and Cobalt, along with the
completed WASIp3 asynchronous resolver. Those Asio results do not establish
Beast's separate P2 guest-socket application corpus or remove the existing
WASIp3 fixture-resolver adaptations. The generated matrix preserves 571 passed,
216 blocked and 432 platform-unavailable cells. The correction passed ten
inventory tests and a fresh Beast MrDocs/rendered-documentation build.

The owner authorized publishing and integration after green CI. The first new
root run, [41675267321](https://github.com/webcpporg/webcpp/actions/runs/41675267321),
failed during checkout because a pinned Trystero commit had not been pushed.
The missing Trystero and WASI dependency commits are now published. Asio's
library CI still selects the superproject's old main, which lacks the new P2
build project. Current-candidate root CI and the post-integration library CI
remain required. These failures are not passing validation evidence.

The current root run is
[41686583077](https://github.com/webcpporg/webcpp/actions/runs/41686583077), on the
published parent above. It found stale build fixtures, a Windows Asio test
expectation, Windows runner portability failures, empty WASIp3 TLS fixture
executables and lint timeouts. Local corrections are being validated; the
partial passing XML reports do not establish a green final candidate. The Beast
source export and 571/216/432 matrix remain unchanged.

## Earlier WASIp2 follow-up, 2026-11-03

The following subsection records the earlier core-only increment. Its TLS,
Cobalt and publication limitations are superseded by the current snapshot above.

The final documentation-only root commit is `15e2f44d2bdff0602b8e761a5a9c89b98c3a5d97`; implementation
remains root `e80ffab`, Asio `dad1aef` and Beast `ab95e84`. A later fresh Emscripten
Release regression passed 271 B2 test targets, covering the default suites and
Asio Asyncify/Cobalt lanes with EH on/off. This is Node-based execution, not a
fresh complete browser run. Full lint compilation-database preparation passed
with 1,419 commands, including P2 aggregate EH twins. The focused clang-tidy scope
remains the 20 configurations below; database generation is not a full tidy run.

The owner approved both the Asio pollable core and supplied-stream WebSocket work.
The local implementation commits are Asio `dad1aef`, Beast `ab95e84` and superproject
`e80ffab`. Each library's `reference/source/` is exported from its exact committed
tree, with every Git blob and executable mode verified. These new commits have not
been pushed or tested by remote CI. The older green root run does not validate them.

Fresh Release builds passed 138 native, 131 WASIp3 and 68 WASIp2 B2 test targets,
including compile/header and runtime targets, with exceptions on/off. The selected
scope covers the default suites and supplied-stream lanes; it is not a new complete
all-platform CI run. Focused clang-tidy passed 20 translation-unit configurations.
Formatting, text rules, Pyright, Asio/Beast MrDocs and rendered-guide checks passed.
Build tests verify the public P2 target, explicit multi-thread/Cobalt/TLS rejection,
pinned SDK/private-ABI refusal and unchanged P3 profile selection. Root
`b2 check-release` passed after the implementation commits.

One independent review found two defects, each reproduced before its correction:
guard-only idle trapped on an empty poll list, and a valid 64-bit host write permit
could narrow to zero on wasm32. Idle now waits on a clock pollable; the permit is
clamped before narrowing. Added consumers cover readiness at registration and
pending-write cancellation. A controlled ABI fixture supplies the large/zero
permit; it does not claim every host's stdout import is nonblocking or preemptible.

P2 core supports timers, TCP/UDP, forward asynchronous DNS and the supplied CLI
stream in a single thread. DNS preserves deferred initiation and handler
associations. P2 guest TLS, Cobalt and multiple threads remain explicitly rejected.
Both supplied WebSocket roles pass with EH on/off. Their actual exchanges cover
upgrade, text/binary frames, fragmentation, ping/pong, close, truncated input,
consumer pause/resume and pending-read cancellation. The application matrix records
571 passed, 216 blocked and 432 platform-unavailable cells. P2 raw-socket Beast
application integration remains blocked despite the now-working Asio core.

## Earlier integration evidence

The following records explain fixes already present in the published baseline.
They are historical evidence, not a rerun of the current local candidate.

- Run [41588828531](https://github.com/webcpporg/webcpp/actions/runs/41588828531)
  exposed a response-before-observation race in the HTTP test peer. The fix records
  each complete request before transmitting its response. A deterministic real-TCP
  regression failed for GET, HEAD and POST with the old ordering and passes with the
  correction. The follow-up passed 56 P2/P3 host-client executions across both exception
  modes and four native protocol checks. The new CI also explicitly expands browser
  contracts across both exception modes and ERROR/IGNORE policies: 56 test executions
  plus eight tutorial executions. All 39 CI planner tests passed.
- Run [41590629467](https://github.com/webcpporg/webcpp/actions/runs/41590629467)
  exposed nine WASIp3 TLS client timeouts under the old eight-second total wall limit.
  Six affected clients reproduced that timeout locally under cold-cache CPU contention,
  before accepting any TCP connection. The fixture now separates at most 15 seconds
  of process startup from five seconds of protocol and process completion, starting
  at real TCP acceptance before TLS. The same six contended clients passed with
  startup times of 9.98-14.61 seconds and protocol times of 0.055-0.104 seconds.
  Nineteen additional exception-off, cooperative, HTTP-method and native executions
  passed. Six real-process/socket regression methods cover slow startup, stalled
  protocol and TLS handshake, missing connections, early exit and supervisor
  descheduling. Independent review reproduced and verified the acceptance timestamp
  boundary correction; all six methods and ten inventory tests pass. The final source
  also passed two actual Wasmtime TLS checks. The previous full run remains available
  for its complete diagnostics; only the new candidate can establish the merge gate.


- The same earlier full run exposed 13 failures on each MSVC version. The shared
  include ordering selected the original Boost headers before the checked Asio/Cobalt
  overlays. Quoting only the external path restores the overlays' precedence while
  retaining external warning suppression for the same Boost root. A real no-exceptions
  compile using the emitted search order fails with the old ordering and passes with
  the correction, including paths with spaces and compiled Cobalt sources. Additional
  corrections protect the TLS factory from Windows' `max` macro, use upstream's static
  Context export definition for MASM, enable extended COFF sections for Beast tests,
  and suppress CRT deprecation diagnostics only in controlled application fixtures.
  Seven affected native tests passed a fresh `b2 -a` run; all three Context assembly
  sources failed without the definition and passed LLVM MASM with it. The Beast socket
  and documentation-snippet tests, build regressions, generator inventory, static
  analysis and independent review passed. The later full baseline CI passed; fresh Windows results for the current local candidate remain pending.

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
| Supplied streams | Portable callback channels and explicit WASI/POSIX stdin/stdout counterparts with partial I/O, EOF, deadlines, cancellation and SIGPIPE handling. They do not claim Unix-domain addressing. P2 WebSocket counterparts now use asynchronous readiness-driven streams. |
| Guest TLS | Real native/WASIp3 HTTP and WebSocket TLS scenarios: trust/hostname failures, cancellation, close, truncated shutdown and deadlines. Eight scenarios across TLS 1.2/1.3 and the supported thread/EH profiles produce 112 recorded scenario executions. |
| Host TLS | Private process/profile CAs with normal verification. Six WASI HTTPS application counterparts plus the shared body/trailer contract produce 35 driver executions and 125 TLS attempts; trusted localhost succeeds and unknown issuers/wrong hostnames fail. The documentation client also exercises mutual TLS. Full Chrome uses disposable profile trust and explicit untrusted/wrong-host controls. |
| Application corpus | Every upstream Jamfile/CMake executable is reconciled at Beast commit `7c1e061f91e2ef542217b76286c314d006c0c8fc`. Immutable upstream hashes, generated adaptations, observable native oracle comparisons, crawler report/concurrency/failure tests, chat and documentation snippets remain inspectable. |
| Documentation | Diataxis AsciiDoc, compiled examples and MrDocs API comments. The final local Beast documentation build passed source checks, reference extraction and rendered HTML checks, updating 13 targets. |

The application matrix retains **53 applications across 23 profiles**, or 1,219
compile/run cells: **571 passed, 216 blocked and 432 platform-unavailable**.
All four P2 supplied-stream WebSocket cells now have runtime evidence. The P06/P07
raw-socket profiles retain 98 blocked cells because the Asio P2 core is now available
but the Beast raw-socket application corpus is not integrated. The eight Unix-domain
cells in those profiles remain unavailable. No cell is discarded from the denominator.

The other 118 blocked cells comprise 53 WASIp3 cooperative EH-on cells without an SDK
exception runtime, 27 WASIp3 socket cells without a verified Boost.Context backend,
18 native EH-off cells requiring stack unwinding, 16 browser cells without a
Boost.Context backend, and four browser documentation clients unable to select the
upstream mutual-TLS identity. These are application/profile outcomes, not a count of
unchanged-source ports or CI jobs. Native oracle normalization compares observable
payload/status/mode results while excluding fixture addresses and elapsed time.
Each adaptation and the host-blocking boundary remain explicit.

## Remaining acceptance work

- **0.5:** the owner-approved submission metadata and reference source are committed
  as `proposed`. Acceptance remains a separate decision; `decided` and `release` are null.
- Former **0A.3 and 0A.4** were removed by an earlier approved scope decision. The
  subsequent P2 approval adds Asio core and supplied WebSocket support. Raw-socket
  Beast guest-socket and guest-TLS applications remain blocked integration work,
  even though Asio now supplies P2 guest OpenSSL and Cobalt.
- **8.3:** complete final-candidate CI and release steps remain pending. Local checks
  and published source do not close the gate. No tag or release publication is authorized.

Other classified limitations remain visible even where the inventory/accounting task is
complete: Boost.Context lacks a verified Wasm backend; WASIp3 cooperative EH-on lacks the
SDK exception runtime; some original application DNS paths still use explicitly labelled local
endpoint counterparts; Asio P2 forward async DNS now has its own pollable backend; host HTTP has no WebSocket upgrade stream; browsers have no raw
listeners or Unix namespace; and standard-descriptor adaptation requires WASI or POSIX.
No unbounded host write can be preempted by a guest cancellation callback.

The final committed follow-up also distinguishes normal WebSocket closure in exception-off
sync/SSL server counterparts: clean closure is recorded and the server continues accepting,
while genuine read/accept/write failures retain the explicit failure path. The targeted
normal-close and marker-evidence regressions passed. The complete final WASIp3 application
rerun passed all 120 cases with an explicit successful exit. The final native application
rerun passed all 247 cases with exit 0: 53 upstream oracle, 53 threaded port, 44 threaded
exception-off, 53 single-thread exception-on and 44 single-thread exception-off executions.
All eight exception-off synchronous WebSocket server cases observed normal closure inside
the engine. Independent review of the frozen candidate reported no remaining findings;
the earlier full CI subsequently passed. The new local candidate still needs fresh CI.


## How to resume current work

1. Read this record, `../tasks.md`, the current `../specs/beast/spec.md`, and the
   committed verification/matrix documents under `source/doc/`.
2. Review and commit any acceptance follow-up, then refresh `source/` with `git archive`
   of that exact committed Beast tree and verify its blob identities. Check only
   tasks whose full criterion is satisfied by committed work and measured evidence.
3. Push the new candidate, run fresh superproject CI for pull request 7 and record
   its exact commit and result. Fix any failed declared lane before claiming the gate complete.
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
