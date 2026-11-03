## Why

Boost.Asio is the ground of asynchronous C++: Boost.Beast, Boost.Cobalt, Boost.MySQL, Boost.Redis
and many other libraries are written on it, as templates over its executors, its completion
tokens and its stream concepts. None of that runs in a browser today in the form a C++ programmer
writes it: a program with `main()` and `io_context::run()` has no way to wait for a timer, a
message or a promise without blocking the page, and a page that is never given back to the
browser stops rendering. Emscripten 6.0.11 can suspend `io_context::run` into the browser's event
loop, but Boost.Asio does not build there without a configuration, nothing wakes a suspended
`run` from a browser event, and a self-posting handler starves the page (measured: 300,000 runs
before a `setTimeout(0)` ran).

webcpp needs a thin layer that closes exactly those gaps and keeps everything else Boost's, so
that a library written on webcpp::asio (webcpp::trystero first, then webcpp::beast and
webcpp::webrtc) and every Boost library built on Asio never knows where it runs. The browser's
own asynchronous model (promises, events) should reach C++ as ordinary Asio operations too.

## What Changes

- **New library `webcpp::asio`**, a wrapper (webcpp's interface over Boost.Asio, not a port):
  C++20 over Boost.Asio on native, Emscripten, WASIp2 and WASIp3. Native names retain Boost's
  behavior. Emscripten suspends Boost's own `io_context` into the browser event loop.
  A checked Boost overlay provides timers and TCP/UDP on WASIp2 and WASIp3.
  WASIp2 adds pollable-driven forward async DNS and supplied CLI streams, with a
  pinned private wasi-libc descriptor bridge. It is single-threaded and rejects
  guest TLS/Cobalt explicitly. WASIp3 retains optional guest TLS/Cobalt and its
  validated cooperative thread profile. Unsupported combinations are refused by name.
- `webcpp::asio::io_context`, derived from `boost::asio::io_context` across targets, with
  `io_context_options{max_handlers_per_turn}` (100 by default, configurable in config.hpp or by compiler define), a reactor made at construction
  and a run family composed of `poll_one` and `run_one` that yields to the browser through a
  `MessageChannel` macrotask after every turn on Emscripten; timers are aliases of Boost's.
- The bridge, `webcpp::asio::browser::bridge`, a service of the io_context through which browser
  events only post; and the detection that refuses a run family call made from a handler or from
  a delivery.
- Two suspension variants built side by side from the same sources, `jspi` (default, WebAssembly
  exceptions) and `asyncify` (JavaScript exceptions, a measured stack, `asyncify-stack-size`),
  with names Boost's way, a manifest and a loader that picks the variant a browser runs.
- Boost.Cobalt's `co_main` given the turn limit by webcpp's own `run_main`; JavaScript promises
  and DOM events as Asio operations (`async_await`, `async_wait_event`); sockets and a resolver
  that fail as values behind a transport point; the `ssl` names.
- Superproject changes the library needs: the propagated composite feature `webcpp-suspend`,
  a test support that fails a test whose `main` did not finish (`webcpp::test::exit_status`),
  `webcpp.variants`, `webcpp.size-table`, and `/webcpp//boost_cobalt`.
- Beast builds on this foundation; the owner intends a later Trystero migration to Asio
  and Beast. Combining the libraries remains supported through one OpenSSL provider.

## Capabilities

### New Capabilities

- `asio`: webcpp::asio, Boost.Asio natively and in the browser Boost.Asio's own io_context
  suspended into the page's event loop, with its io_context, run family, turn limit, bridge,
  detection, suspension variants, Asyncify stack, loader, Cobalt `run_main`, promises and
  events as operations, transport point, resolver, `ssl` names, allocation and exception
  guarantees, WASI timers/networking and supplied streams, tests, examples and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a wrapper (`"port-of": null`), repository `webcpporg/asio`, checked out as
  the superproject submodule `libs/asio`, under BSL-1.0.
- **Implementation:** tasks 1-21, the later WASIp3/TLS increments and the approved
  single-thread WASIp2 core/supplied-stream increment are implemented locally.
  The approved capability extension adds P2 TLS/Cobalt, native asynchronous P3 DNS,
  complete WASI UDP sequences, browser pthread handoff and compile-time guards.
  The repository was created on 2026-11-03 UTC and the owner-authorized superproject
  integration is commit `2ba5116`. Tasks 22/23 close the documentation, conformance,
  complete-branch review and final CI gates. `reference/STATUS.md` distinguishes tested
  commits from work in progress and identifies the exact reference source snapshot.
- **Dependencies:** Boost 1.92, WebCPP config, Emscripten 6.0.11, the pinned Node/browser
  test tools, WASI SDK 34 and Wasmtime 50.0.1. Guest TLS explicitly selects source-built
  OpenSSL 3.5.9 and optionally the verified embedded Mozilla CA snapshot. xactor is a
  user of Asio, not a dependency of it.
- **Status:** `implementing`, for 0.1.0 in preparation. The owner accepted the original
  specification and five departures on 2026-10-11, approved subsequent WASIp3/TLS work,
  and ratified the reconciled scope on 2026-11-03. No release has been published.
- **Other proposals:** Beast has its own proposal and uses the browser and socket
  facilities. WebRTC and Trystero's application migration remain separate workstreams.
