## Context

The primary goal is to compile existing Boost.Beast programs to WebAssembly, on WASIp2,
WASIp3 and Emscripten, with minimal source changes. Native Boost.Beast is the reference.
webcpp::beast is a compatibility facade with environment-specific implementations.
Use the platform's HTTP/WebSocket services where they exist; do not require an upstream
Beast protocol engine merely because the application uses its programming interface. Compiled with
Emscripten, the same names and interfaces drive the browser's `WebSocket` and `fetch`: the
layers the browser does not expose (resolve, connect, the TLS handshake) record what the browser
needs, and the last one (the WebSocket handshake, or an HTTP request) opens the connection.

The historical baseline is `reference/spec.md` (2026-10-10). The owner expanded the scope
on 2026-11-02 to all upstream example applications and explicitly included WASIp2 and
WASIp3. On 2026-11-03 the owner confirmed that guest sockets/TLS follow Asio's targets:
initially native and WASIp3. Later that day the owner approved a single-thread Asio
WASIp2 core and asynchronous supplied-channel WebSocket. Portable HTTP and host HTTP
remain independent of the reactor; the full P2 guest-socket corpus needs its own validation. This design and `specs/beast/spec.md` are the current proposal;
the historical baseline is not rewritten. The committed implementation has native, WASI and actual browser
measurements; `reference/STATUS.md` identifies the exact source snapshot, evidence and
remaining acceptance gates. The library builds on webcpp::asio
(`libraries/asio`, change `genesis`): the io_context, the bridge that posts the browser's
events to it, Boost's timers, the `ip::tcp` types with the resolver of its decision 5, and the
`ssl` record of the program's TLS settings.

Constraints every decision below works within, from the superproject's rulebook:

- header-only C++20, namespace `webcpp::beast`, macros `WEBCPP_BEAST_*`;
- targets native, WASIp2, WASIp3 and Emscripten; WASI host-HTTP and stream/socket backends
  are separate. The newly approved Asio P2 core makes sockets possible, while the Beast P2 guest-socket corpus remains an integration/validation task;
- errors as values; exceptions optional through `WEBCPP_BEAST_NO_EXCEPTIONS`;
- allocation through allocators the program gives; Boost and std before custom code; outside
  interfaces wrapped; fixed-width types;
- BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- Account for all 53 executable targets in the pinned Beast examples, plus their support
  files and documentation snippets. Run every application whose required facilities the
  target exposes, and provide useful host-service or stream counterparts for applications
  whose original OS transport is absent. Keep both results visible.
- Preserve the original programming model and protocol behaviour. Every source adaptation
  has an inspectable diff; a counterpart with a different model is reported separately.
- Natively, no cost and no difference: Beast's own types, one derived class, two functions.
- In the browser, every Beast setting classified: honoured, unsupported, or ignored with a
  reason; a program learns at the operation what the browser cannot do.
- Bound the library-owned receive queue and pending writes in the browser; browser-internal
  memory is outside the C++ allocator and cannot be bounded by this wrapper.
- A defined cancellation, destructor and allocation contract, proven by a counting test.

**Non-Goals:**

- `WebSocketStream` (increment 3: Chromium only, doubles the surface behind one interface).
- A resolver of webcpp::beast's own (rejected, decision D4).
- Server-side WebSocket or HTTP in the browser; the browser has no listening socket.
- Ping, pong, client certificates, custom headers other than `Sec-WebSocket-Protocol`, and
  protocol-level fragmentation: the browser does not expose them, and they fail as values.
- Trystero's relays on webcpp::beast (increment 4, a later change).

## Decisions

### D1. A library of its own on webcpp::asio; WebSocket first, then the HTTP client

Owner's decision 1. Beast is a library on Asio, so webcpp::beast is a library on webcpp::asio,
not a part of it. Increment 1 is WebSocket across the target workstreams; increment 2 adds HTTP browser
transport and completes HTTP example coverage. WASI prerequisites can advance independently.
All upstream examples are covered by this change; WebSocketStream and trystero migration
remain later changes.

### D2. Preserve the interface while selecting the platform implementation

Native uses upstream Beast. Wasm HTTP can translate Beast-shaped requests, responses and
completion tokens directly to host HTTP services; browser WebSocket uses the browser object.
Application compatibility does not require that the implementation be Boost.Beast. A raw
stream/socket backend can retain upstream protocol algorithms where useful and supported.

The owner authorized implementation after the recommendation to reuse upstream data
types while delegating operations to the environment. The initial implementation therefore
retains Boost.Beast headers and type identity for portable data; host operations remain
separate adapters. This is not a dependency-free Wasm library. Compatible observable
operations, allocator/error contracts and inspectable application changes remain required.

### D3. What the browser cannot honour fails as a value, by default

Owner's decision 2. Each setting is classified honoured, unsupported, or ignored (only where the
browser does an equivalent thing of its own, with the reason). `WEBCPP_BEAST_UNSUPPORTED_ERROR`
(the default) makes the operation that depends on an unsupported setting complete with
`boost::asio::error::operation_not_supported`, the connection staying as it was;
`WEBCPP_BEAST_UNSUPPORTED_IGNORE` ignores the setting. Silent divergence was rejected: a program
that asks for a client certificate or a ping must learn so.

### D4. The resolver is webcpp::asio's, and its entry is what the browser streams accept

Owner's decision 4 (option A of the review's C4). On Emscripten webcpp::asio's resolver completes
with one entry that carries the host name, the service name and an unspecified endpoint with the
service's port. A plain socket's connect on it fails with `operation_not_supported`, so
webcpp::asio's "fails as a value" holds where a TCP connection would be needed; only
webcpp::beast's browser streams accept it, and Beast's layered client code runs unchanged. A
resolver of webcpp::beast's own, with webcpp::asio's failing like its sockets, was rejected:
every Beast client would change a line.

### D5. The layers record; the last one opens

`async_connect` records the host and service, the TLS handshake judges the recorded settings and
records the scheme, and `ws.async_handshake(host, target)` builds the URL and calls
`new WebSocket(url, subprotocols)`. This is the only way to keep Beast's code shape, because the
browser has one call that does resolution, connection, TLS and the upgrade together.

### D6. The browser WebSocket is selected by the next layer's type

`websocket::stream<NextLayer>` over the browser `tcp_stream` or `ssl_stream` is webcpp's
browser WebSocket. A supported explicit byte-stream next layer (a relay socket, a WebRTC
channel's byte stream) needs a guest protocol engine. Upstream Beast is the proposed engine
for that opt-in path, subject to O6; it is not a prerequisite for browser-native WebSocket.
Where Beast is reused, `teardown` and `async_teardown` for webcpp::asio's sockets are declared
in the sockets' namespace so that argument-dependent lookup finds them, as Beast's
`websocket/teardown.hpp` asks of a user-defined socket.

### D7. Back-pressure is a queue in the library, not the browser's

Browser `send` does not synchronously wait for delivery, but can reject an invalid state or
close when it cannot buffer. The library checks OPEN before sending, maps failures to error
values, bounds `bufferedAmount` (default 4 MiB, an option), and fails a write past it with
`browser_errc::send_limit`. Successful completion means accepted for sending, not peer delivery. Messages arrive whether a read is
pending or not and wait in a bounded queue in the stream's allocator; past the bound the stream
closes and the next read completes with `browser_errc::receive_limit`. A queued message does not keep
`run` alive. `WebSocketStream`, which would pause receiving while C++ does not read, is a later
increment.

### D8. HTTP host backends translate messages directly

The default browser HTTP backend maps request/response operations to fetch. The WASI host-HTTP
backend maps them to wasi:http, with separate P2/P3 ABI adapters. Facade overloads or dispatch
for HTTP operations are allowed; the old rule forbidding all overloads is removed. Do not
require serialize-parse-serialize round trips just to reuse upstream algorithms over an
imaginary socket. Message/body/serializer incremental interfaces still need explicit coverage;
this is not permission to drop them or ignore body limits, back-pressure or allocators.

A byte-oriented compatibility adapter remains available when an application explicitly needs
parser/serializer stream APIs or a supplied stream. Its cost and changed framing are recorded;
wire parsers are not claims about the host's real HTTP framing. HEAD/204/304 responses have no
synthetic body or chunk terminator. Direct and byte-adapter paths must agree on application
status, exposed headers, body and cancellation outcomes.

### D9. Configuration is propagated once, with distinct browser types

`<webcpp/beast/config.hpp>` reads `WEBCPP_BEAST_UNSUPPORTED_ERROR`,
`WEBCPP_BEAST_UNSUPPORTED_IGNORE` and `WEBCPP_BEAST_NO_EXCEPTIONS`; both unsupported macros
defined is an `#error`. A b2 feature drives the setting from the usage requirements. The
browser classes live in an inline namespace named after the setting, as Asio tags its own, so
translation units that disagree name different types. A test passes a configured stream
across translation units and requires a link failure for a mismatch. This does not claim
that unrelated functions using different configurations always produce a linker error.

### D10. Errors with no counterpart have a category of their own

The browser's `error` event carries no cause, and a failed connection closes with 1006, so
refusal, DNS, TLS and a blocked port are one error. Seven values in the category
`webcpp.beast` are fixed and never reused and are named by `browser_errc`, preserving
Boost.Beast's existing `error` enum and its category; every other outcome maps to an existing Beast or Asio
code (`websocket::error::closed`, `connection_reset`, `beast::error::timeout`,
`websocket::error::message_too_big`, `operation_not_supported`).

### D11. Cancellation is by terminal, partial and total; the destructor aborts and unregisters

The browser cannot unsend a message or undo an opening. The table of the specification states,
per operation and per cancellation type, what happens. The destructor completes every pending
operation with `operation_aborted` (posted, so no pending work keeps `run` alive), closes the
browser socket without a code, and removes its registrations from webcpp::asio's bridge.

### D12. An allocator constructor on every target

Operation state uses the handler's associated allocator, as Beast's does. The stream's own state
(the receive queue, the registrations, the buffered fragments) uses the allocator given to
`stream(std::allocator_arg_t, const Allocator&, Args&&...)`, and `std::allocator<std::byte>`
without it. Natively Beast's own state uses Beast's allocation, which the documentation names.

### D13. `set_server_name` replaces the one line that cannot compile in a browser

Beast's SSL examples set SNI through `SSL_set_tlsext_host_name(native_handle(), host)`; there is
no OpenSSL handle in the browser. Boost 1.92 has no SNI helper, so `set_server_name` is new on
every target and a program changes that line once. Exact-version TLS context methods are a separate
restriction: `tlsv12_client` fixes both minimum and maximum to TLS 1.2 upstream. Browser
strict mode rejects it; a documented `tls_client` counterpart delegates version negotiation
to the browser. Ignoring a version restriction is never reported as enforcing it.
For host-service backends, record the requested SNI hostname and validate it against the
eventual URL authority. The host controls SNI; a differing requested name follows the
unsupported-setting policy. Only guest-owned socket TLS calls OpenSSL.

### D14. Target profiles and ownership of the WASI prerequisites

Use the target's own service first for that protocol, selected explicitly by backend type or
build configuration; do not silently fall back to a different trust/transport policy after an
error. WASI host-HTTP uses webcpp::wasi and the component ABI. Its completion scheduling may
need Asio executor integration, but not TCP, a socket reactor, DNS workers or guest OpenSSL.
For a socket backend, keep socket/reactor/resolver fixes in Asio or wasi-libc and TLS changes
in the shared build. Guest sockets follow Asio's declared targets: native, WASIp2 single
and WASIp3. Guest TLS is currently validated only for native and WASIp3. These are distinct dependency chains
and are tested independently.

| Profile | Threading | Exceptions | Current dependency position |
| --- | --- | --- | --- |
| Native | single or OS threads | on/off | reference, with example-specific dependencies |
| WASIp2 host HTTP | host invocations; guest threads separately probed | on/off independently verified | independent Beast bridge and application counterparts measured |
| WASIp2 supplied streams | single; guest threads separately probed | on/off independently verified | HTTP counterparts and asynchronous WebSocket client/server counterparts measured |
| WASIp2 sockets/TLS | single for Asio core | on/off for Asio core | Asio core is available; Beast guest-socket applications and guest TLS remain unvalidated/not integrated |
| WASIp3 host HTTP | host invocations; guest threads separately probed | on/off independently verified | independent Beast bridge and application counterparts measured |
| WASIp3 sockets, single | disabled | on/off | Asio/TLS integrated; async DNS unsupported |
| WASIp3 sockets, multi | cooperative, not CPU-parallel | off | Asio/TLS integrated |
| WASIp3 sockets, multi | cooperative | on | blocked by the tested SDK 34 exception runtime |
| Emscripten JSPI | single | on/off | browser Asio integration available |
| Emscripten Asyncify | single | on/off | browser Asio integration available |

B2 must reject unsupported configurations with a specific reason, rather than silently
switching targets, enabling exceptions or turning off threads. Browser pthread support is
not implied by Emscripten supporting pthreads generally; the current Asio backend disables
threads. Unsupported thread profiles still appear in the matrix. WASIp2 threading and EH
are investigated independently; do not copy WASIp3's cooperative-thread result into them.
The existing Asio evidence is pinned to superproject `2ba5116` and Asio `3611a6e`;
`libs/asio/doc/capabilities.adoc` and `feature-matrix.adoc` are the dependency references.

### D15. Compatibility is demonstrated by the upstream applications

The owner's acceptance corpus is the entire example catalogue, not a hand-picked tutorial.
Pin Boost tag `boost-1.92.0`, Beast commit `7c1e061f91e2ef542217b76286c314d006c0c8fc`.
The source audit found 53 `exe` declarations under `example/**/Jamfile` and 53 entry-point
sources. Keep a future machine-readable manifest with one row per executable, all source
and fixture paths, target/profile, exact adaptations, upstream dependencies, build command,
runtime command, expected exchange, result and evidence. Reconcile it with upstream Jamfiles
and CMake definitions on upgrades. A missing row is a coverage failure.

The native upstream build is the oracle. Maintain an adapted webcpp twin with a small,
reviewable diff: includes/namespaces, the portable SNI helper, target build configuration and
explicit platform adaptations. Do not replace `spawn` with `co_await`, `system_executor` with
an event loop, or synchronous code with callbacks and then label the original compatible.
Such counterparts can be useful and are built too, but the original's blocked status remains.
Retain upstream notices. Common certificate headers are fixtures, not a production trust store.

### D16. Execution, scheduling and TLS remain faithful to the target

- WASI HTTP applications use host request/response resources by default. Applications that
  need real TCP/HTTP wire control or WebSocket framing can explicitly select a socket/stream
  backend. Host HTTP is not evidence of a WebSocket upgrade/tunnel API; probe such a
  facility independently before exposing it.
- On WASIp3 single, the existing async resolver's unsupported result is a required negative
  test. A pre-resolved endpoint (or synchronous resolution before running the context) is an
  explicit example adaptation. The program's URL hostname is still used for Host, SNI and
  certificate identity. Do not claim this establishes asynchronous DNS compatibility.
- Guest-owned TLS in native/WASIp3 socket backends uses existing OpenSSL and CA targets.
  WASIp2 guest-socket applications remain pending integration; guest TLS is not enabled. WASI host HTTP and browser services use the
  host's TLS and trust policy where provided; guest CAs do not automatically affect it.
  Never disable verification or silently switch backends to make an example pass.
- Browser synchronous client methods need a suspending facade over the asynchronous transport.
  Probe this before promising compatibility: JSPI/Asyncify must resume the original call and
  allow a browser heartbeat to run while it waits. No busy waits or nested `io_context::run`
  inside a running handler. The initial supported entry is the application's outer main;
  nested/reentrant synchronous use must be diagnosed and documented. It stays blocked until
  the probe and example tests pass, rather than silently converting the program to async.
- `coro` examples and `websocket-server-fast` use stackful Boost.Coroutine/Context. Test the
  upstream backend, unwinding and EH requirements independently. JSPI, Asyncify and C++20
  coroutines are not evidence for that backend. `stackless` examples use Asio's state-machine
  coroutine and are a different dependency class.
- Server examples that start threads or install signals need separate original and adapted
  results. A one-runner, explicitly stopped counterpart can work without those facilities;
  it does not prove parallelism, signals or the original threading model.

### D17. Controlled peers and honest browser results

Use bounded local fixtures instead of the examples' public hosts. Supply deterministic files,
JSON, method responses and crawler URLs; keep the crawler's algorithm and exercise multiple
hosts/paths without crawling the public internet. Grant only the WASI network/filesystem
capabilities the fixture requires. A host driver starts peers, waits for readiness, drives the
application, checks the response, imposes a deadline and reaps every process.

Run clients against native and supported WASI servers, WASI servers against a host client,
and browser clients against those servers. The chat example includes its HTML client and
multiple simultaneous participants. A browser client talking to a WASI server is a working
cross-target application, not evidence that the server runs inside the browser. Browser TCP
listeners, Unix-domain sockets and raw TCP echo remain unavailable without another transport;
provide the requested stream or host-service counterparts, labelled as such. A network
proxy or a different application protocol is a separate design, not an implicit dependency.

For HTTP compare status, exposed application headers and decoded payloads, using a documented
normalization of framing and browser-controlled fields. Do not erase meaningful errors, body
bytes or redirect outcomes. Raw output remains available. For WebSocket compare text/binary
mode, payloads and observable close outcomes. Test TLS trust failures, hostname mismatches,
timeouts, mid-transfer cancellation, peer close and cleanup. Servers have an explicit bounded
shutdown path. Compilation and execution are separate matrix fields: a fallback `main` that
prints 'not supported', a no-op build or Node-only execution cannot count as browser success.

### D18. Documentation and acceptance are driven by the matrix

Write tutorials, how-to guides, reference and explanations in AsciiDoc following Diataxis;
MrDocs supplies API reference from headers. Include compiled example regions and observed
output. Publish one feature matrix and one example matrix, with target, JSPI/Asyncify where
applicable, threads, EH, tool versions, flags and commands. Label results as planned, passed,
failed, blocked or platform-unavailable. Unsupported-policy IGNORE never changes a blocked
backend into a supported one. All promised runnable rows must pass. A documented host-service/stream counterpart can
fulfil application portability even when the original OS transport is unavailable; it does
not establish exact source/transport parity. Unresolved implementation blockers still remain
visible for their promised backend. Do not reduce the denominator to the examples that happen to build.

### D19. Adapt WASI HTTP servers at the host entrypoint

The owner explicitly wants WASI's HTTP infrastructure reused for Beast HTTP servers. Keep
request-handling logic and message operations, replacing transport bootstrap with a component
entry adapter. In P2 the host calls incoming-handler with a request and response-outparam;
in P3 it calls the versioned async handler. The host owns bind/listen, physical connections,
HTTP version and potentially TLS. An HTTP handler is not a physical TCP accept event.

Offer the host-adapted application alongside the raw-socket example. The diff identifies the
entrypoint and listener/session plumbing replaced by the adapter. It must not fake socket
options, peer identity, pipelined physical connection identity, keep-alive ownership, TLS
inspection or WebSocket upgrades. Request routing and response construction remain the
application's. A library entry adapter may standardize this bootstrap, but cannot infer an
arbitrary program's application handler from its main function.

Reuse webcpp::wasi's component/B2 machinery. Its current helper exposes method/target and a
response containing status, content type and a whole body; that is evidence of an existing
entrypoint, not a complete Beast bridge. Extend its reusable request/header/body/trailer,
streaming and lifetime interfaces in that library. Exactly one response is committed per
request; cancellation after commitment fails the remaining body rather than sending a second
response. P2 response-outparam and P3 async-resource lifetimes have independent tests.

### D20. Pair-of-streams counterparts for local-socket examples

Treat an input stream plus an output stream as one already-connected byte channel when the
host wires them to a peer. stdin/stdout are possible endpoints, not an implicit Unix socket.
The adapter implements synchronous/asynchronous read/write, partial progress, EOF,
back-pressure, cancellation and ownership to the extent the specific host streams provide.
A regular file, terminal and pipe can have different readiness/closure behaviour; test the
provided endpoints rather than infer capabilities from an integer descriptor.

Run the original HTTP or WebSocket application exchange over this explicit stream transport
where the protocol engine supports it. stdin/stdout carry protocol bytes only; diagnostics go
to stderr. The test driver wires both directions, drains output concurrently, injects partial
reads/writes, closes endpoints and enforces a deadline. One input/output pair is one peer;
multiple connections require additional host-supplied resources, not invented accept calls.
No pathname bind/connect, descriptor passing, Unix peer credentials or socket-only options
are promised. These are successful adapted applications when their stated exchanges pass,
while the original Unix-domain transport row remains unavailable on that profile.

## Risks / Trade-offs

- **`basic_resolver_results::create` is outside Boost's documented interface** (guarded by
  `GENERATING_DOCUMENTATION` in 1.92), and the resolver depends on it. Mitigation: a probe
  pins it, and the Boost version is pinned. Falls on webcpp::asio's resolver task as well.
- **The browser hides the cause of a failed connection**, so `connection_failed` covers refusal,
  DNS, TLS, a blocked port and CORS-like failures. Accepted: the Web APIs offer no more.
- **Node enforces neither CORS nor mixed content** (measured by the review), so those failures
  are only seen in the headless-browser lane.
- **Synthetic HTTP framing costs and differences.** The explicit byte-stream adapter may
  parse/serialize messages and change framing. Measure and document that optional path;
  direct request/response mapping to host services is the default architecture.
- **The `ERROR` default makes some of Beast's examples fail as written** (`User-Agent`, added
  roots); the specification lists, per example, what changes under each setting. The trade-off
  is honesty over compatibility.
- **Idle-timeout activity** is messages and close frames only, because the browser answers the
  peer's pings itself and shows none; stated in the table of differences.
- **Backend gaps are not Beast features.** The integrated Asio already has browser and WASIp3
  support and now a single-thread WASIp2 core with asynchronous forward DNS. Its WASIp3 single-thread async resolver reports
  `operation_not_supported`; cooperative threads currently require exceptions off. Keep
  those constraints in the matrix and fix common transport issues in Asio/libc, not Beast.
- **Stackful coroutines and operating-system facilities.** Boost.Context, Unix sockets,
  signal handlers and CPU-parallel thread pools cannot be inferred from a working TCP test.
  Each example is checked for these dependencies before a profile can be called supported.

## Migration Plan

The library is new: nothing depends on it. A program that uses Boost.Beast changes `boost::beast`
to `webcpp::beast`, `boost::asio` to `webcpp::asio` (for the names webcpp::asio owns), and the
one SNI line to `set_server_name`. Beast's examples change as section 4.6 of the specification
lists. Trystero moves onto the library in a later change.

## Open Questions

- **O1. Header layout, initial decision.** The convenience header contains portable data.
  Byte-stream algorithms, sockets and TLS have explicit headers and dependencies. Host-entry
  headers are also explicit. This keeps message-only WASIp2 builds free of a reactor.
- **O2. Queue defaults, implemented decision.** `websocket::queue_limits` exposes `send` and
  `receive`, both defaulting to 4 MiB. The send bound includes browser `bufferedAmount`
  and unsent application fragments. The receive bound includes retained payload capacity
  and message records, including arrivals before executor dispatch and partially consumed
  records. Empty messages consume record budget. Ingress is charged before retention and
  uses one allocator-bound notification for a batch; a paused executor cannot accumulate
  an unbounded queue of notifications. `test/browser/browser_test.cpp` and
  `test/browser/lifecycle_test.cpp` exercise limits with real native Beast peers, including
  paused empty/payload bursts. Browser-internal memory remains outside this bound.
- **O3. Decorator shape and classification, implemented decision.** The browser calls the
  decorator on an empty `websocket::request_type` with PMR fields backed by the stream's
  allocator. Only `Sec-WebSocket-Protocol` is representable, including comma-separated
  protocol tokens. Every other field, including `User-Agent`, is unsupported: ERROR
  rejects the dependent handshake and IGNORE omits the field. Actual Chrome and Node
  JSPI/Asyncify tests exercise both policies; the allocation witness covers long field
  values and allocator-owned decorator state. Guest protocol decorators remain a separate
  explicit adapter and do not relax the browser host restriction.
- **O4. Streaming request bodies.** The specification buffers a request body whole before the
  `fetch`; whether a streaming upload (Chromium only) is wanted is open, and belongs to a later
  change.
- **O6. Upstream header/type dependency, implementation decision.** Following the authorized
  recommendation, reuse upstream data types. Browser and WASI host operations still map
  directly to environment services; they need not execute the upstream protocol engine.
- **O5. The Boost proposal** (specification section 10) is made when both libraries have run in
  real programs, trystero first; no work in this change.

## Upstream executable inventory: original transport/programming model

Source: [Boost.Beast 1.92 examples](https://github.com/boostorg/beast/tree/7c1e061f91e2ef542217b76286c314d006c0c8fc/example),
read on 2026-11-02. This is a source audit, not build or runtime evidence.

`T`: intended direct twin after common include/namespace/SNI changes; `A`: an explicit
platform adaptation is also expected; `B`: dependency or implementation probe blocks a
compatibility claim; `U`: required platform facility is unavailable in the selected backend.
All cells are prospective. This table describes the original transport/model, not the
complete application result: the measured manifest must add host-HTTP and pair-of-streams
counterparts for each applicable executable. An original `U` does not exclude those
counterparts from the deliverable. Native denotes an OS/EH/thread configuration that supplies the
example's dependencies; every single-thread and EH-off variant is tested and reported
separately. WASIp2 socket cells are `B` until integrated and tested against the newly
approved Asio P2 core. Unix-domain and system-executor requirements remain `U`. Host HTTP and supplied-stream results remain
separate from these original socket cells. Browser columns
must expand to both JSPI/Asyncify and EH modes in the measured matrix.

Notes: `DNS` = async DNS gap on WASIp3 single; `CTX` = stackful coroutine backend;
`LOCAL` = Unix-domain socket; `EXEC` = system executor/thread pool; `SYNC` = browser suspension
facade; `RUN` = thread/signal/server-lifetime adaptation; `TLS` = trust/SNI adaptation;
`FIX` = controlled fixture input; `STREAM` = raw Asio stream needs browser Beast transport.
A `U` is documented and negatively tested, never counted as a passed application.

| Upstream path under example/ | Executable | Native | WASIp2 sockets | WASIp3 single | WASIp3 cooperative | Browser | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `advanced/server` | `advanced-server` | A | B | A | A | U | RUN |
| `advanced/server-flex` | `advanced-server-flex` | A | B | A | A | U | RUN, TLS |
| `advanced/server-flex-awaitable` | `advanced-server-flex-awaitable` | A | B | A | A | U | RUN, TLS |
| `doc/ssl` | `doc-ssl-client` | A | B | A | A | A | TLS, STREAM |
| `doc/ssl` | `doc-ssl-server` | A | B | A | A | U | TLS, STREAM |
| `echo-op` | `echo-op` | T | B | A | A | U | Common migration |
| `http/client/async` | `http-client-async` | T | B | A | T | A | DNS |
| `http/client/async-local` | `http-client-async-local` | T | U | U | U | U | LOCAL |
| `http/client/async-ssl` | `http-client-async-ssl` | A | B | A | A | A | DNS, TLS |
| `http/client/async-ssl-system-executor` | `http-client-async-ssl-system-executor` | A | U | B | A | B | DNS, EXEC, TLS |
| `http/client/awaitable` | `http-client-awaitable` | T | B | A | T | A | DNS |
| `http/client/awaitable-ssl` | `http-client-awaitable-ssl` | A | B | A | A | A | DNS, TLS |
| `http/client/body` | `json_client` | A | B | T | T | B | SYNC, FIX |
| `http/client/coro` | `http-client-coro` | T | B | B | B | B | DNS, CTX |
| `http/client/coro-ssl` | `http-client-coro-ssl` | A | B | B | B | B | DNS, CTX, TLS |
| `http/client/crawl` | `http-crawl` | A | B | A | A | A | DNS, RUN, FIX |
| `http/client/methods` | `http-client-methods` | A | B | T | T | B | SYNC, FIX |
| `http/client/sync` | `http-client-sync` | A | B | T | T | B | SYNC |
| `http/client/sync-ssl` | `http-client-sync-ssl` | A | B | A | A | B | SYNC, TLS |
| `http/server/async` | `http-server-async` | A | B | A | A | U | RUN |
| `http/server/async-local` | `http-server-async-local` | A | U | U | U | U | LOCAL, RUN |
| `http/server/async-ssl` | `http-server-async-ssl` | A | B | A | A | U | RUN, TLS |
| `http/server/awaitable` | `http-server-awaitable` | A | B | A | A | U | RUN |
| `http/server/coro` | `http-server-coro` | A | B | B | B | U | CTX, RUN |
| `http/server/coro-ssl` | `http-server-coro-ssl` | A | B | B | B | U | CTX, RUN, TLS |
| `http/server/fast` | `http-server-fast` | T | B | A | A | U | Common migration |
| `http/server/flex` | `http-server-flex` | A | B | A | A | U | RUN, TLS |
| `http/server/small` | `http-server-small` | T | B | A | A | U | Common migration |
| `http/server/stackless` | `http-server-stackless` | A | B | A | A | U | RUN |
| `http/server/stackless-ssl` | `http-server-stackless-ssl` | A | B | A | A | U | RUN, TLS |
| `http/server/sync` | `http-server-sync` | A | B | A | A | U | RUN, SYNC |
| `http/server/sync-ssl` | `http-server-sync-ssl` | A | B | A | A | U | RUN, SYNC, TLS |
| `websocket/client/async` | `websocket-client-async` | T | B | A | T | A | DNS |
| `websocket/client/async-local` | `websocket-client-async-local` | T | U | U | U | U | LOCAL |
| `websocket/client/async-ssl` | `websocket-client-async-ssl` | A | B | A | A | A | DNS, TLS |
| `websocket/client/async-ssl-system-executor` | `websocket-client-async-ssl-system-executor` | A | U | B | A | B | DNS, EXEC, TLS |
| `websocket/client/awaitable` | `websocket-client-awaitable` | T | B | A | T | A | DNS |
| `websocket/client/coro` | `websocket-client-coro` | T | B | B | B | B | DNS, CTX |
| `websocket/client/coro-ssl` | `websocket-client-coro-ssl` | A | B | B | B | B | DNS, CTX, TLS |
| `websocket/client/sync` | `websocket-client-sync` | A | B | T | T | B | SYNC |
| `websocket/client/sync-ssl` | `websocket-client-sync-ssl` | A | B | A | A | B | SYNC, TLS |
| `websocket/server/async` | `websocket-server-async` | A | B | A | A | U | RUN |
| `websocket/server/async-local` | `websocket-server-async-local` | A | U | U | U | U | LOCAL, RUN |
| `websocket/server/async-ssl` | `websocket-server-async-ssl` | A | B | A | A | U | RUN, TLS |
| `websocket/server/awaitable` | `websocket-server-awaitable` | A | B | A | A | U | RUN |
| `websocket/server/chat-multi` | `websocket-chat-multi` | A | B | A | A | U | RUN, FIX |
| `websocket/server/coro` | `websocket-server-coro` | A | B | B | B | U | CTX, RUN |
| `websocket/server/coro-ssl` | `websocket-server-coro-ssl` | A | B | B | B | U | CTX, RUN, TLS |
| `websocket/server/fast` | `websocket-server-fast` | A | B | B | B | U | CTX, RUN |
| `websocket/server/stackless` | `websocket-server-stackless` | A | B | A | A | U | RUN |
| `websocket/server/stackless-ssl` | `websocket-server-stackless-ssl` | A | B | A | A | U | RUN, TLS |
| `websocket/server/sync` | `websocket-server-sync` | A | B | A | A | U | RUN, SYNC |
| `websocket/server/sync-ssl` | `websocket-server-sync-ssl` | A | B | A | A | U | RUN, SYNC, TLS |

The catalogue also includes `common/root_certificates.hpp`, `common/server_certificate.hpp`,
`doc/http_examples.hpp`, `http/client/body/json_body.hpp`, crawler input sources,
`http/server/fast/fields_alloc.hpp`, the multi-file chat implementation and `chat_client.html`.
They remain attached to their examples, with dedicated compile/runtime harnesses for the
standalone documentation snippets. They are not silently counted as additional applications.


## Draft review corrections, 2026-11-02

The builder check identified three contradictions in the historical browser contract, corrected
in this draft: use `browser_errc` for browser-only errors so upstream `beast::error::timeout`
keeps its identity; reject fixed-version TLS methods such as `tlsv12_client` in strict browser
mode; and emit no synthetic chunk framing for HEAD/204/304 responses. These corrections follow
source inspection, not an implementation measurement. The plan must include their regression
scenarios. Primary references:

- [Beast core error enum](https://github.com/boostorg/beast/blob/7c1e061f91e2ef542217b76286c314d006c0c8fc/include/boost/beast/core/error.hpp)
- [Asio TLS context methods](https://github.com/boostorg/asio/blob/boost-1.92.0/include/boost/asio/ssl/impl/context.ipp)
- [Beast HTTP parser](https://github.com/boostorg/beast/blob/7c1e061f91e2ef542217b76286c314d006c0c8fc/include/boost/beast/http/impl/basic_parser.ipp)
- [Browser WebSocket interface](https://websockets.spec.whatwg.org/)
- [Fetch transport and response model](https://fetch.spec.whatwg.org/)

The environment-backend review additionally removed universal WASI socket/OpenSSL
prerequisites and mandatory HTTP serialization in the browser. Host HTTP and supplied-stream
counterparts now count as application portability with their original transport limitations
retained. SNI intent is validated against the host URL rather than silently discarded.

Sources inspected for this revision include webcpp::wasi's `http/entrypoint.hpp`,
`http/response.hpp`, `example/http_hello.cpp`, `build.jam` and the HTTP P2/P3 WIT worlds.
The current helper accepts method/target and returns status/content-type/whole-body; it
does not yet provide the complete request/body/trailer bridge described in D19. The pinned
interfaces are `wasi:http@0.2.12` (incoming-handler/response-outparam) and `@0.3.0`
(async handler). P2 CLI stdio uses input/output resources; P3 uses asynchronous streams and
completion futures. Source inspection establishes available interfaces, not passing Beast
tests. Further references:

- [WASI HTTP host interface rationale](https://github.com/WebAssembly/wasi-http)
- [WASI stream abstraction](https://github.com/WebAssembly/wasi-io)
- [Wasmtime P2 HTTP embedding](https://docs.wasmtime.dev/api/wasmtime_wasi_http/p2/index.html)
- [Wasmtime P3 HTTP embedding](https://docs.wasmtime.dev/api/wasmtime_wasi_http/p3/index.html)

### Later P2 core and supplied-WebSocket approval, 2026-11-03

The owner subsequently approved Asio P2 core and the four supplied-stream
WebSocket combinations. This supersedes the earlier explanation that no Asio P2
target exists. The current P2 WebSocket counterparts retain asynchronous
operations via Asio pollables and preserve host channel ownership. Their client
and server exchanges pass with exceptions on/off, including 128 KiB binary
frames, ping/pong and close. Other supplied counterparts remain synchronous.
The new Asio capability does not establish the separate Beast guest-socket
application corpus or P2 guest OpenSSL. Its cells are blocked integration work,
not an inherent WASI platform impossibility. The generated matrix now has 571
measured passing cells, 216 blocked cells and 432 platform-unavailable cells.
