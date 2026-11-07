## Context

webcpp::asio is a header-only C++20 wrapper over Boost.Asio for two targets, native and
Emscripten. Its specification was written on 2026-10-10 from an independent review whose probes
(built with em++ 6.0.11 and run with node 26.7.0) measured how Boost.Asio already behaves on
Emscripten (`reference/spec.md`), approved by the owner on 2026-10-11, and turned into a plan of
24 tasks, 0 to 23 (`reference/plan.md`), whose five departures from the spec the owner approved
the same day. Tasks 1 and 4 to 9 are implemented in a local, unpublished repository; this
document is the approved design in OpenSpec's form.

Constraints every decision below works within, from the superproject's rulebook (its
`AGENTS.md`):

- header-only C++20, namespace `webcpp::asio`, headers under `<webcpp/asio/...>`, internals in
  `detail`, macros `WEBCPP_ASIO_*`, include guards `WEBCPP_ASIO_<HEADER>_HPP`, the b2 target
  `/webcpp/asio//asio`, the error category `webcpp.asio`;
- the library is built for native, Emscripten and WASIp2/P3; every test runs on each
  target its Jamfile names, with exceptions on and off, in both suspension variants;
- errors as values (`boost::system::error_code`); an API that throws sits behind
  `WEBCPP_ASIO_NO_EXCEPTIONS` and raises through `boost::throw_exception`;
- run-time objects allocate through the user's allocator; no global or static mutable state in
  the library;
- every line that names Boost.Asio, a clock, a timer or `this_thread::` says why after
  `lint-world:`, and every call of the run family says why after `lint-run:`;
- Boost and std before custom code; no new third-party code; BSL-1.0, "Copyright (c) 2026
  WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- A program written against Boost.Asio, `main()` and `io_context::run()` included, runs unchanged
  in a browser; its code runs only when the browser delivers an event, never by polling, and its
  stack and objects stay as they were.
- Natively, webcpp::asio is Boost.Asio itself and costs nothing.
- On Emscripten, add only what Boost lacks: the threads configuration, a reactor made at
  construction, a limit on the handlers one turn runs, and a bridge from browser events to
  `post`.
- Both suspension variants (JSPI and Asyncify) built from the same sources, tested, sized, and
  picked at load time.
- Boost.Cobalt, Boost.Beast (through webcpp::beast) and any library on Asio's model run on it
  unchanged.
- The browser's promises and events as Asio operations, with every completion token.
- What a browser cannot do (TCP, UDP, name resolution) compiles and fails as a value.

**Non-Goals:**

- WASIp2 multi-thread execution without an SDK/runtime profile; portable WASI host APIs remain separate.
- WebSocket and HTTP clients (webcpp::beast), WebRTC (webcpp::webrtc), relay transports and the b2
  feature that enables one, trystero's migration, and offering the additions upstream to
  Boost.Asio.
- Polling the page from C++, running the run family from a JavaScript event, or blocking the
  page's thread without suspending.
- Implicit workers in a single-thread browser build. Explicit pthread builds retain their separate environment and affinity requirements.

## Decisions

### D1. Actual native Boost types and explicit Wasm source distribution

The owner-approved 2026-11-06 migration supersedes the former derived native
context and include-overlay approach. Native equivalents are aliases of actual
Boost types. Wasm uses versioned implementation files under explicit WebCPP paths,
with one backend across Asio, Cobalt and Beast. Browser turn options are target
extensions, not extra native constructors. Applications migrate includes/namespaces
and use the selected backend's executors, traits and completion tokens consistently.

`sources.json` identifies compiled support units and feeds the B2 source list.
Direct compiler and CMake consumers use an extracted archive with explicit SDK and
provider inputs, without Git, B2, patch programs or undeclared checkout paths.
Maintainer generators retain upstream hashes/licenses; consumers never execute them.
Known native Boost defects remain documented under the native identity contract.
Cobalt owns its core, runner and corrected SSL wrapper, including the separately
approved native SSL identity exception. Old Asio Cobalt names only forward.

The earlier decisions below record browser algorithm and integration history;
references to Boost's internals mean the selected Wasm backend, not permission to
restore native derived types, consumer overlays or Cobalt ownership inside Asio.

### D2. The run family is a composition of Boost's calls

Boost's `run` is not virtual and cannot be told to yield, so on Emscripten the six members hide
Boost's and compose `poll_one` and `run_one`: a turn of ready handlers, return if stopped or
expired, yield through a macrotask if the turn reached the limit, else `run_one` (which suspends
in the reactor's `select`). Natively they are Boost's own through `using` declarations.

- **Why:** measured, a self-posting handler ran 300,000 times under Boost's `run` before a
  `setTimeout(0)` ran, because a zero-timeout `select` suspends only to a microtask.
- **Consequence:** code that calls `run` through a `boost::asio::io_context&` gets Boost's run
  without the limit (documented); Cobalt's `co_main` gets the limit by D11.

### D3. A MessageChannel macrotask and a configurable default of 100

The yield is an `EM_ASYNC_JS` import whose promise a `MessageChannel` message resolves.
The original default was 1000. After the 2026-10-15 yield-cost and responsiveness measurements,
the owner selected 100. `WEBCPP_ASIO_MAX_HANDLERS_PER_TURN` in config sets the compile-time
default; an application may define it before inclusion and override it per context. Zero
normalizes to one. A deadline is checked before each handler. No setting preempts a handler.

The guide retains the benchmark methodology and observed costs. These observations justify
an adjustable compromise, not a portable time bound or an assurance of a fixed frame rate.

### D4. JSPI and Asyncify are build variants, a composite propagated feature

`webcpp-suspend` (`jspi` default, `asyncify`) is propagated, so every library a program uses is
built for its variant, and composite because Asyncify traps at run time with WebAssembly
exceptions: `asyncify` composes `<exception-handling-method>js`. `jspi` links `-sJSPI`,
`asyncify` `-sASYNCIFY`; a build with `exception-handling=off` compiles `-fno-exceptions` on both.
A non-default variant builds in a directory of its own.

- **Why:** the owner decided (2026-10-10) that the variant is chosen at build time and the
  developer ships the artifact a page's loader picks for its browser. A usage requirement
  cannot do it, because b2 takes only free features from usage requirements.
- **Alternatives:** JSPI only (excludes browsers without it); a run-time switch (the two
  differ in compiler instrumentation, so they cannot share an artifact).

### D5. Asyncify's stack is measured, 8192 bytes, raised by a feature

`-sASYNCIFY_STACK_SIZE` is 8192 unless a program asks for more with `asyncify-stack-size`
(propagated, powers of two from 8192 to 1048576; smaller values are declared only to be refused
by an `#error` of `config.hpp`). The size is twice the deepest suspension the tests reach in
any build b2 links today (3,540 bytes in a release build linked without optimization, measured
2026-10-11). `asyncify_stack` reads the bytes each unwind wrote from Asyncify's own data and
fails when a suspension of the run family leaves less than half the stack. The asio flag ends
last on emcc's line, where emcc keeps the last.

- **Why:** a suspension that does not fit traps with `RuntimeError: unreachable`, certain at
  every suspension that deep; Emscripten's default (4096) left 556 bytes in the deepest build.
- **Open point:** when b2's release link gets its `-O`, measure again.

### D6. The reactor is made at construction

With threads disabled, a `run` that has work but no reactor waits in `null_event::do_wait`,
`::pause()`, which nothing wakes (measured). The io_context holds one unarmed
`boost::asio::steady_timer`, through public API, so the reactor exists from construction and
`run` suspends in `select`, which a post wakes.

### D7. The bridge only posts

Every event source is a JavaScript function made by the io_context's bridge, a service of the
context (made with its allocator, shut down and destroyed with it), registered by a key that is
never reused (a `std::uint64_t` from 1, held in JavaScript as a `BigInt`). Delivering looks the
key up and posts; it never polls, runs or dispatches: while `run` is suspended an event sees
`running_in_this_thread()` true and a `dispatch` would run its handler on the event's stack, and
calling the run family there fails at its first suspension (a `SuspendError` under JSPI, a
corrupted `main` under Asyncify).

- **JavaScript boundary:** the bridge's functions keep what must hold however a delivery ends,
  because a JavaScript error passes every C++ `catch` and skips frames' epilogues (and their
  destructors outside WebAssembly exceptions): the stack pointer (`stackSave`/`stackRestore`),
  the count of deliveries in progress (`delivering()` reads it), and the event's handle (C++ only
  borrows it, through a union that never destroys the `val`). A C++ exception is caught in C++,
  which frees it. A trap (`WebAssembly.RuntimeError`, which `abort()` throws too) goes on to the
  page; `ExitStatus` and `'unwind'` end the delivery unreported; after `ABORT` nothing enters
  WebAssembly.
- **No public way around it:** `deliver` is private (the export is a friend), `delivering()`
  takes no value, pinned by compile-fail tests.

### D8. Detection by a flag and by the bridge's count

Each of the six members admits once, first (`BOOST_ASSERT_MSG`, returning 0 and running nothing
when assertions are off), then calls only private unchecked code, never another public member.
A flag scope puts back the value it found, so an inner scope never clears an outer one's flag
and a C++ exception clears it on its way out. Four calls go undetected and are documented.

- **A JavaScript error out of a handler** skips destructors in the Asyncify and no-exception
  builds and leaves Boost's own state broken (its work count, its call stack); asio does not put
  its flag back, because no boundary can restore Boost's state and a recovered flag would trade
  a reported refusal for a run that never returns. Handlers catch JavaScript errors in
  JavaScript. Measured in all four builds on 2026-10-11.
- `admitted()` is not `noexcept`: whether the bridge delivers is read from JavaScript.

### D9. A test ends as its `main` ends, in the build, not in each library

Under node a `main` that suspended and returns non-zero exits 0 under Asyncify, and a `main` that
never finishes exits 0 on both variants. The superproject's test support (`node-pre.js`, linked
by every `webcpp.run`, `run-fail` and `example`) makes a test whose `main` did not finish fail,
keeps the first status given to `webcpp::test::exit_status`, and stops a test past its bound
(120 s by default). Every test ends with `return webcpp::test::exit_status(boost::report_errors());`.

### D10. Artifacts named Boost's way, a manifest, and a loader

b2 separates builds by directory and an artifact's name carries its tags joined by `-`
(`app-60-d-jspi.js`, `.../webcpp-suspend-asyncify/app-60-d-asyncify.js`). `webcpp.variants`
builds both variants of a page program and `<name>.variants.json`, which
`webcpp-asio-loader.mjs` reads (never a pattern): `supportsJspi`, `pick`, `load`. `webcpp.size-table`
records the size of each variant of each example (Asyncify's program is about 2.9 times JSPI's).

### D11. Cobalt's `run_main` is webcpp's, built from Cobalt's sources

Cobalt's `co_main` cannot be replaced from a header: `main` is a hidden friend that calls
`main_promise::run_main`, compiled in Cobalt's `src/main.cpp`, which owns an `io_context` and
calls Boost's `run`. webcpp compiles every Cobalt source but `main.cpp` (and, departure 3,
`io/ssl.cpp`) as `/webcpp//boost_cobalt` and defines `run_main` itself in
`src/cobalt_main.cpp`, with a `webcpp::asio::io_context` and its run family; under Asyncify it
ends the program with `std::exit` so that the `co_return` value is the exit status. It reads
`main_promise`'s members, which are Boost's implementation, so compiling it against the pinned
Boost is the probe; if it cannot be kept, Cobalt's own `co_main` is the fallback, without the
limit.

### D12. Promises and events are bridge registrations

`async_await(executor, promise, token)` and `async_wait_event(executor, target, type, token)`
go through `async_initiate`, so every completion token works. Each is a registration: the
settlement or the event posts the handler; a rejection completes with
`webcpp::asio::browser::error::promise_rejected` (category `webcpp.asio`, value 1, never
reused) and the reason as the value; a cancellation completes at once with `operation_aborted`
and drops the registration (for an event, the listener is removed); a pending operation is
outstanding work; destroying the io_context destroys the handler without calling it; the
operation and the registration allocate through the handler's associated allocator.

### D13. Sockets that fail as values behind a transport point

Boost's sockets compile on Emscripten but connect through SOCKFS, which opens a WebSocket, and
`getaddrinfo` invents addresses. So on Emscripten `ip::tcp` and `ip::udp` are webcpp's protocol
classes with Boost's members, and the sockets and the acceptor are class templates over a policy
`browser::transport<Protocol>` whose primary completes every operation with
`operation_not_supported`, synchronously and asynchronously (through a post), never blocking;
`open` and `close` succeed. A relay library specialises the transport; the specialisation must
be visible in every translation unit before first use (one definition rule), so it is enabled
for a whole program through a b2 feature that `config.hpp` reads, designed with that library.

### D14. The resolver resolves nothing (option A)

`resolve(host, service)` completes with one entry carrying the host and service names and an
unspecified address with the service's port (`""` is 0; digits to 65535; `http` and `ws` 80,
`https` and `wss` 443; anything else `service_not_found`; `numeric_service` refuses names).
Reverse resolution fails with `operation_not_supported`; a plain socket's `connect` on the entry
fails the same way, so only webcpp::beast's browser streams accept it and Beast's layered client
code runs unchanged. A resolver that failed like the sockets was rejected: every Beast client
would change a line.

### D15. Explicit source-built TLS on native and WASI

`<webcpp/asio/ssl.hpp>` is Boost's SSL interface on native and WASIp2/P3. The program explicitly
selects `/webcpp/asio//openssl` or `//tls`, backed by `/webcpp/third_party//openssl` and a
matching generated header/archive pair. The latter includes verified, replaceable embedded
CA data. Emscripten retains the browser TLS settings adapter and refuses guest OpenSSL.
Cobalt's optional `//cobalt_ssl` uses the same provider and checked SSL state/cancellation fixes.

This supersedes the original departure 4 (system shared OpenSSL). The source build and CA
package were approved in October and integrated locally on 2026-11-02; the owner ratified
reconciliation of this proposal with that implementation on 2026-11-03. OpenSSL's process-global
memory hooks are not an Asio per-operation allocator; the guide states that allocation boundary.

### D16. WASIp3 sockets, with explicit profile limitations

The original accepted browser scope excluded WASI because unmodified Boost 1.92 did not build
there. The owner approved the WASIp3 port on 2026-10-15, subsequently approved source-built TLS,
and approved applying these changes to this proposal on 2026-11-03. A checked private Boost
overlay now supplies timers, TCP/UDP and Cobalt integration on WASIp3. Native and browser
semantics remain distinct from host-controlled WASI network permissions.

WASIp3 single-thread builds support exceptions on/off. Cooperative threads require the SDK's
matching experimental sysroot; SDK 34 lacks its C++ exception runtime, so that profile refuses
exceptions on. The later owner-approved P2 extension below adds a single-thread reactor. Beast socket profiles follow
Asio's targets; Beast's portable messages, host HTTP and supplied streams do not require this
reactor and may support WASIp2 independently.

### D17. Coexistence with Trystero

The owner clarified on 2026-11-03 that Asio and Beast are intended foundations for Trystero's
later migration. Combining the libraries MUST NOT be prohibited as a workaround for OpenSSL
provenance. A combined native program must use one OpenSSL provider, including libdatachannel's
transitive dependency. Migrating Trystero's application APIs remains a separate workstream.

## Risks / Trade-offs

- **Reading Emscripten and Boost internals.** The reactor behavior, Asyncify's data layout (the
  stack measurement), `ABORT`, `Emval` and `main_promise`'s members are implementation details
  of Emscripten 6.0.11 and Boost 1.92; the tests are the probes that fail when they change.
- **Asyncify's cost** is size (about 2.9 times) and a stack that must be measured; JSPI needs a
  browser that has it. Both are built and tested.
- **Boost's own `run` through a `boost::asio::io_context&`** has no limit and no detection.
- **A JavaScript error out of a handler** can break the context for good in three builds (D8).
- **The turn limit is a default, not a guarantee of frame rate**: up to `k + 1 +
  max_handlers_per_turn` handlers can run between two yields.
- **Cobalt's `run_main` depends on Boost's private layout** (D11) and the build keeps Cobalt's
  sources.

## Migration Plan

The repository `webcpporg/asio` exists and the superproject integrated its submodule in
`2ba5116` on 2026-11-02. Tasks 22/23 now close the reconciled rulebook, complete-branch review
and both CI gates. `reference/STATUS.md` records exact tested commits and remaining work.
Beast already uses Asio; Trystero's migration onto Asio and Beast follows this foundation.

## Open Questions

- The b2 feature that enables a relay transport for a whole program (D13) is designed with the
  first relay library.
- `asyncify-stack-size` should be measured again when b2's release link passes the optimization
  level, and again when the Cobalt tests join the lane (task 16 step 5).
- The CI's levels (nightly, release) do not exist, so the Asyncify lanes run on every push
  (departure 1) until they do.
- The final review must cover the complete Asio branch and root integration, not only the
  earlier task 9, tasks 17-21 or WASIp3/TLS review subsets.

### WASIp2 scope amendment approved on 2026-11-03

After the earlier P2 exclusion, Rodrigo explicitly approved both an Asio P2
pollable-based core and the four Beast supplied-stream WebSocket combinations.
This supersedes the no-P2-reactor restriction. Retain the Boost scheduler and
libc select/poll translation for timers, TCP and UDP. Forward asynchronous DNS
uses native address-stream pollables, with deferred initiation, cancellation and
associated executor/allocator semantics. The generic pollable descriptor bridge
is isolated and pinned to SDK 34.0's private libc ABI. P2 single-thread execution
supports both exception modes; the follow-up below adds verified guest TLS and
Cobalt. Multi-thread execution remains unavailable with the pinned SDK. Host HTTP and supplied streams remain separate
capabilities. This amendment does not declare every Beast P2 socket application
validated or restore the removed proposal tasks without their own evidence.


### Capability completion approved on 2026-11-03

The owner approved completing WebCPP-owned gaps and leaving SDK/host limits
explicitly documented and guarded. P2 receives the shared source-built OpenSSL,
CA and Cobalt/SSL targets in both exception modes. WASI UDP gains connected gather
send and scatter receive without weakening datagram boundaries. P3 asynchronous
forward DNS uses native subtasks/waitables in both thread profiles, with a checked
private libc descriptor bridge, deferred initiation and cancellation.

Explicit Emscripten pthread builds retain the selected JSPI/Asyncify mechanism.
Contexts run only on their creation thread; cross-thread posting and an explicit
main-runtime executor provide handoff without sharing JavaScript values across
threads. The main-thread poll adapter preserves ordinary application main/DOM
behavior rather than implicitly proxying main to a worker. Page isolation and
engine support remain external requirements.

`config.hpp` owns derived 0/1 capabilities. Optional provider selection is
separate, and explicit application requirements reject compiler-visible unsupported
combinations. Target-specific headers explain missing facilities. Runtime grants,
peer identities and host-blocking imports cannot be diagnosed by preprocessing.
No SDK upgrade, upstream Cobalt issue 287 repair or new Beast application-corpus
claim is implied by this amendment. Fresh publication gates remain necessary.
