## Context

The primary goal is to compile existing Boost.Beast programs to WebAssembly, on WASIp2,
WASIp3 and Emscripten, with minimal source changes. Native Boost.Beast is the reference.
webcpp::beast is a wrapper of webcpp, not a replacement protocol library. Compiled with
Emscripten, the same names and interfaces drive the browser's `WebSocket` and `fetch`: the
layers the browser does not expose (resolve, connect, the TLS handshake) record what the browser
needs, and the last one (the WebSocket handshake, or an HTTP request) opens the connection.

The historical baseline is `reference/spec.md` (2026-10-10). The owner expanded the scope
on 2026-11-02 to all upstream example applications and explicitly included WASIp2 and
WASIp3. This design and `specs/beast/spec.md` are the current draft; the historical baseline
is not rewritten. It rests on a review of Boost.Beast 1.92 and Emscripten 6.0.11 whose probes are
cited there; this change has not re-run them. The library builds on webcpp::asio
(`libraries/asio`, change `genesis`): the io_context, the bridge that posts the browser's
events to it, Boost's timers, the `ip::tcp` types with the resolver of its decision 5, and the
`ssl` record of the program's TLS settings.

Constraints every decision below works within, from the superproject's rulebook:

- header-only C++20, namespace `webcpp::beast`, macros `WEBCPP_BEAST_*`;
- targets native, WASIp2, WASIp3 and Emscripten; the Asio WASIp2 backend and its shared TLS
  integration are prerequisite work, while WASIp3 builds on the existing Asio port;
- errors as values; exceptions optional through `WEBCPP_BEAST_NO_EXCEPTIONS`;
- allocation through allocators the program gives; Boost and std before custom code; outside
  interfaces wrapped; fixed-width types;
- BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- Account for all 53 executable targets in the pinned Beast examples, plus their support
  files and documentation snippets. Run every application whose required facilities the
  target exposes, and keep blockers visible until fixed or explicitly decided.
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

### D2. Reuse Boost.Beast on native and WASI; browser classes exist only on Emscripten

Every name that touches no network (buffers, `http` messages, parsers, serializers, `websocket`
close reasons and errors) is Beast's own on every target. On native and supported WASI profiles, `tcp_stream` and
`ssl_stream` are Beast's, and `websocket::stream` is a class derived from Beast's, adding the
allocator constructor, so that all targets have one constructor set. A static-assertion test
proves the aliases are Beast's types. WASI tests must also instantiate the algorithms over
the actual Asio backend; type aliases alone do not establish a working port.

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
browser WebSocket; over any other next layer (a relay socket, a WebRTC channel's byte stream) it
is Beast's, as natively. `teardown` and `async_teardown` for webcpp::asio's sockets are declared
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

### D8. The HTTP client reuses Beast's algorithms by being a byte stream

The browser `tcp_stream` accepts the bytes Beast's serializer writes, parses them back with
Beast's `request_parser`, makes one `fetch` per whole request, and serializes the response with
Beast's serializer into the bytes the reads yield (chunked, with `Content-Length` and
`Content-Encoding` left out). webcpp::beast overloads no HTTP algorithm. The cost is one parse
and one serialization per message, for Beast's code unchanged. Responses to HEAD and
status 204/304 contain headers only, never a synthetic chunk terminator; request-method
tracking and back-to-back exchanges test that no framing remains in the next response.

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

### D14. Target profiles and ownership of the WASI prerequisites

This is an implementation recommendation, not a claim of tested Beast support. Keep Beast's
protocol code upstream; put socket/reactor/resolver changes in Asio or wasi-libc and common
TLS/build changes in the existing shared targets. Extend the WASIp3 overlay only where a
WASIp2 probe proves the assumptions valid. Otherwise add a target-specific backend there.
A WASI HTTP handler alone does not supply the byte-stream or listening-socket semantics
needed by the complete Beast examples.

| Profile | Threading | Exceptions | Current dependency position |
| --- | --- | --- | --- |
| Native | single or OS threads | on/off | reference, with example-specific dependencies |
| WASIp2 | single first; multi probed separately | probe on/off independently | Asio and TLS integration missing; required work |
| WASIp3 single | disabled | on/off | Asio/TLS integrated; async DNS unsupported |
| WASIp3 multi | cooperative, not CPU-parallel | off | Asio/TLS integrated |
| WASIp3 multi | cooperative | on | blocked by the tested SDK 34 exception runtime |
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

- Ordinary WASI TCP/HTTP/WebSocket applications use real sockets, including server listen and
  accept when the host grants those capabilities. Test permissions denied as well as success.
- On WASIp3 single, the existing async resolver's unsupported result is a required negative
  test. A pre-resolved endpoint (or synchronous resolution before running the context) is an
  explicit example adaptation. The program's URL hostname is still used for Host, SNI and
  certificate identity. Do not claim this establishes asynchronous DNS compatibility.
- Native/WASI TLS uses the existing OpenSSL and embedded/replacement-CA targets. WASIp2 needs
  its own source build and trust/entropy validation. In the browser the browser owns TLS:
  never turn certificate verification off to make an example pass.
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
adding a proxy or replacing the protocol requires a separately labelled design.

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
backend into a supported one. All runnable rows must pass; blockers keep the full portability
objective incomplete. Do not reduce the denominator to the examples that happen to build.

## Risks / Trade-offs

- **`basic_resolver_results::create` is outside Boost's documented interface** (guarded by
  `GENERATING_DOCUMENTATION` in 1.92), and the resolver depends on it. Mitigation: a probe
  pins it, and the Boost version is pinned. Falls on webcpp::asio's resolver task as well.
- **The browser hides the cause of a failed connection**, so `connection_failed` covers refusal,
  DNS, TLS, a blocked port and CORS-like failures. Accepted: the Web APIs offer no more.
- **Node enforces neither CORS nor mixed content** (measured by the review), so those failures
  are only seen in the headless-browser lane.
- **One parse and one serialization per HTTP message** in the browser. Accepted for Beast's code
  unchanged; a direct path is a later optimisation if measured necessary.
- **The `ERROR` default makes some of Beast's examples fail as written** (`User-Agent`, added
  roots); the specification lists, per example, what changes under each setting. The trade-off
  is honesty over compatibility.
- **Idle-timeout activity** is messages and close frames only, because the browser answers the
  peer's pings itself and shows none; stated in the table of differences.
- **Backend gaps are not Beast features.** The integrated Asio already has browser and WASIp3
  support, but no WASIp2 target. Its WASIp3 single-thread async resolver reports
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

- **O1. Header layout.** The public headers and the convenience header are not fixed by the
  specification (the plan will fix them). webcpp::asio's convenience header leaves `ssl` out
  natively because it brings OpenSSL; whether `<webcpp/beast.hpp>` does the same is to decide.
- **O2. The defaults of the receive queue bound** (the send bound is 4 MiB) and the names of the
  two options. To measure and decide in the plan.
- **O3. How a decorator is called** on an empty `websocket::request_type`: whether any other
  field can be told apart from `User-Agent` by name alone, or every non-subprotocol field is
  unsupported (the specification says the latter: confirm with a test).
- **O4. Streaming request bodies.** The specification buffers a request body whole before the
  `fetch`; whether a streaming upload (Chromium only) is wanted is open, and belongs to a later
  change.
- **O5. The Boost proposal** (specification section 10) is made when both libraries have run in
  real programs, trystero first; no work in this change.

## Upstream executable inventory

Source: [Boost.Beast 1.92 examples](https://github.com/boostorg/beast/tree/7c1e061f91e2ef542217b76286c314d006c0c8fc/example),
read on 2026-11-02. This is a source audit, not build or runtime evidence.

`T`: intended direct twin after common include/namespace/SNI changes; `A`: an explicit
platform adaptation is also expected; `B`: dependency or implementation probe blocks a
compatibility claim; `U`: required platform facility is unavailable in the selected backend.
All cells are prospective. Native denotes an OS/EH/thread configuration that supplies the
example's dependencies; every single-thread and EH-off variant is tested and reported
separately. WASIp2 `B` always includes the missing Asio/TLS integration. Browser columns
must expand to both JSPI/Asyncify and EH modes in the measured matrix.

Notes: `DNS` = async DNS gap on WASIp3 single; `CTX` = stackful coroutine backend;
`LOCAL` = Unix-domain socket; `EXEC` = system executor/thread pool; `SYNC` = browser suspension
facade; `RUN` = thread/signal/server-lifetime adaptation; `TLS` = trust/SNI adaptation;
`FIX` = controlled fixture input; `STREAM` = raw Asio stream needs browser Beast transport.
A `U` is documented and negatively tested, never counted as a passed application.

| Upstream path under example/ | Executable | Native | WASIp2 | WASIp3 single | WASIp3 cooperative | Browser | Notes |
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
| `http/client/async-ssl-system-executor` | `http-client-async-ssl-system-executor` | A | B | B | A | B | DNS, EXEC, TLS |
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
| `websocket/client/async-ssl-system-executor` | `websocket-client-async-ssl-system-executor` | A | B | B | A | B | DNS, EXEC, TLS |
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
