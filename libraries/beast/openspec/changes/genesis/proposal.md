## Why

The primary purpose of webcpp::beast is to take existing Boost.Beast applications to
WebAssembly: WASIp2, WASIp3 and Emscripten in a real browser. Native Boost.Beast is the
behavioural reference. Keeping the original operations, types and control flow with minimal,
reviewable source changes matters more than providing a new HTTP or WebSocket interface.
The implementation should use the target's own facilities: browser fetch/WebSocket and WASI
HTTP/streams, rather than requiring Boost.Beast's transport/protocol implementation everywhere.

The owner's scope clarification of 2026-11-02 includes every application in Boost.Beast's
examples. The pinned Boost 1.92 catalogue contains 53 executable targets, including clients,
servers, TLS, synchronous, asynchronous, stackful and C++20 coroutine examples. Every target
must be accounted for; a rewritten application, a compile-only result and a working port are
not interchangeable. Browsers expose WebSocket and fetch instead of raw listening sockets,
so compatibility has to state the platform boundaries rather than promise impossible parity.

The original specification of 2026-10-10 is preserved in `reference/spec.md`. This revision
supersedes its native/Emscripten-only scope and its three-example acceptance criterion.
The library remains a draft with no implementation or measured Beast runtime results.

## What Changes

- **WebAssembly is the main deliverable.** Native is the reference; WASIp2, WASIp3 and
  Emscripten are required target workstreams. Missing backend support is tracked work, not
  a reason to drop a target from the specification.
- **A separate library `webcpp::beast`**, a C++20 wrapper over Boost.Beast and webcpp::asio,
  presenting Beast-compatible operations and data interfaces while selecting an environment
  backend. Native retains upstream Beast. The Wasm header/type dependency policy is pending
  the owner's clarification; protocol execution need not use upstream Beast.
  The first increment remains WebSocket, followed by HTTP.
- **WASI HTTP uses the host's HTTP interfaces.** Adapt client requests and server handlers to
  `wasi:http` on P2/P3, reusing and extending webcpp::wasi. The host owns transport and may
  own TLS; this path does not require the raw-socket Asio port or guest OpenSSL. Keep a
  separate stream/socket backend for applications that need wire-level control, with shared
  Asio/OpenSSL dependencies where available.
- **Stream counterparts preserve useful applications.** For Unix-domain socket examples,
  add a pair-of-streams counterpart, including host-wired stdin/stdout, keeping the exchange
  logic. Document startup and endpoint changes; do not pretend to implement Unix-domain
  addressing, ancillary data or peer credentials. Similar applications are part of the
  requested deliverable, not merely rejected examples.
- **Emscripten uses browser transports.** The layered connection calls record URL and TLS
  intent; the final handshake or request uses WebSocket or fetch. Add synchronous client
  coverage through suspending adapters as well as the asynchronous interface. Run both
  JSPI and Asyncify in the browser.
- **All 53 upstream executables are the acceptance inventory**, with their original paths,
  dependency restrictions, source differences and per-profile build and runtime evidence.
  Supporting sources, certificate fixtures, the chat HTML and documentation snippets are
  included. The detailed initial inventory is in `design.md`.
- **A capability matrix** separates target, suspension mechanism, threading and exception
  mode. It distinguishes tested parity, documented adaptation, implementation blockers and
  platform-unavailable operations. No silent skips or trivial replacement programs count.
- **Browser differences stay explicit.** The default unsupported-setting policy reports
  errors as values; the optional ignore policy cannot create a listening socket, Unix-domain
  socket or unsupported execution backend. An explicit host-service/stream adaptation can
  nevertheless provide the application with a supported execution path. Raw servers run on native/WASI and serve the
  browser clients; they are not advertised as browser server ports.
- **Validation uses real exchanges.** Test each runnable example with local controlled peers,
  compare application payloads and outcomes against native Boost.Beast, and exercise failure,
  timeout, cancellation and TLS trust behaviour. Compilation alone is a separate result.
- **Documentation** follows Diataxis in AsciiDoc, with MrDocs reference and compiled examples.
  It publishes the capability and example matrices and reproducible commands.

## Capabilities

### New Capabilities

- `beast`: source-compatible migration of Boost.Beast programs to WebAssembly, its native
  reference, browser stream adaptations, WASI transport integration, unsupported-setting
  policy, allocation and lifetime contracts, exhaustive example inventory and evidence.

### Modified Capabilities

None in this OpenSpec root. Extensions to webcpp::wasi for complete HTTP requests/responses
and stream I/O, the optional raw-socket Asio WASIp2 backend, shared TLS/B2 changes and any
Boost.Context work belong to their owning components and must have linked changes and tests;
this proposal does not declare those dependencies implemented.

## Impact

- **Kind:** wrapper (`port_of: null`), BSL-1.0; intended repository `webcpporg/beast`, not
  created, checked out as `libs/beast`. Retain upstream copyright and licence on adapted code.
- **Implementation:** not started. The plan and reference code are still required before
  acceptance. This revision is a specification and a source inventory, not a port result.
- **Dependencies:** pinned Boost.Beast 1.92, webcpp::asio, Boost.System and webcpp::config;
  webcpp::wasi for host HTTP/streams; shared OpenSSL and CA targets only for guest-owned
  TLS in native/WASI socket backends; wasi-sdk and Wasmtime for WASI;
  Emscripten, Node and a real browser for JSPI/Asyncify. Stackful examples additionally
  require a proven Boost.Coroutine/Context backend; replacing them with C++20 coroutines
  does not establish compatibility of the original examples.
- **Status:** draft; no release. Intended targets: native, wasip2, wasip3, emscripten.
  Asio currently declares native, wasip3 and emscripten; WASIp2 raw sockets remain
  prerequisite work for that backend, not a gate for host-provided WASI HTTP.
- **Other proposals:** build on the published Asio integration and record extensions in its
  proposal. Trystero's later relay migration and an upstream Boost proposal remain separate.
