# Tasks

The current draft incorporates the owner's 2026-11-02 WebAssembly-first scope and all 53
upstream example applications; no Beast implementation exists. There is no plan and no reference code yet, so
group 0 comes first and every later task is refined by the plan. `reference/spec.md` is the
historical baseline, `specs/beast/spec.md` the current requirements, `reference/STATUS.md` says where things
stand.

## 0. Before anything: the plan and the reference code

- [x] 0.1 The specification (`reference/spec.md`, draft of 2026-10-10, with the owner's four decisions)
- [ ] 0.2 Write the implementation plan, `reference/plan.md`: every task below with its files, interfaces, steps, code and expected output, in the form of the other libraries' plans
- [ ] 0.3 Write the reference code: the library's first tree (skeleton, configuration, the native names, `set_server_name`, `teardown`) as a local branch, in the form `reference/source/` takes in the other proposals
- [ ] 0.4 Settle the open questions O1 to O3 of `design.md` with the owner and record them in the plan
- [ ] 0.5 Submit the proposal for a decision (status `proposed`) once 0.2 and 0.3 exist

## 0A. Portability preflight and shared prerequisites

- [ ] 0A.1 Pin Beast 1.92 at `7c1e061f91e2ef542217b76286c314d006c0c8fc`; turn the 53-row design inventory into a checked manifest, reconcile all Jamfile/CMake targets, support files and documentation snippets
- [ ] 0A.2 Build and run the original native examples with controlled fixtures to establish the oracle; record EH, thread, SSL, Context and OS dependencies
- [ ] 0A.3 Probe WASIp2 Asio timers, TCP connect/listen/accept/read/write, cancellation and DNS on SDK 34; reproduce gaps and implement fixes in Asio/libc with a linked change before enabling the Beast target
- [ ] 0A.4 Extend shared OpenSSL, CAs and B2 for WASIp2; validate source-built TLS, entropy source, hostname verification and replacement CAs, with an independent result for each supported thread/EH profile
- [ ] 0A.5 Probe WASIp2 thread/EH combinations; retain unavailable profiles with specific diagnostics, without inferring support from WASIp3
- [ ] 0A.6 Instantiate upstream Beast over the integrated WASIp3 Asio/TLS backend; test real client/server exchanges, timers, shutdown and cancellation with threads disabled and cooperative threads where available
- [ ] 0A.7 Reproduce and record the WASIp3 single async DNS gap; provide explicitly labelled endpoint-resolution counterparts, and track original async-DNS compatibility as blocked until fixed
- [ ] 0A.8 Probe Boost.Coroutine/Context for each Wasm profile with its EH/unwinding requirements; track missing backends in the owning Boost integration, never replace stackful examples and count them as passing originals
- [ ] 0A.9 Probe browser synchronous transport suspension from outer main on JSPI/Asyncify with a heartbeat and a reentrancy diagnostic; keep synchronous compatibility blocked until demonstrated

## 1. The skeleton and the configuration (requirements: Library, Config)

- [ ] 1.1 The repository layout: `build.jam` (project `/webcpp/beast`, alias `beast`), `meta/libraries.json` (`"port-of": null`), `README.md`, `AGENTS.md`, licence, `doc/changelog.adoc`, CI calling the superproject's workflow
- [ ] 1.2 `<webcpp/beast/config.hpp>`: `WEBCPP_BEAST_UNSUPPORTED_ERROR` (default), `WEBCPP_BEAST_UNSUPPORTED_IGNORE`, the `#error` when both, `WEBCPP_BEAST_NO_EXCEPTIONS`
- [ ] 1.3 The b2 feature `webcpp-beast-unsupported=error|ignore` and its usage requirement
- [ ] 1.4 The inline namespace named after the setting for the browser classes; test a cross-translation-unit stream interface with mismatched settings, without promising universal mismatch detection
- [ ] 1.5 `test/config_test.cpp` built under each setting and with exceptions on and off

## 2. The native and WASI names (requirements: Names)

- [ ] 2.1 The aliases of Boost.Beast's types, and the network-free names as Beast's own on every target
- [ ] 2.2 `websocket::stream` derived from Beast's with `stream(std::allocator_arg_t, const Allocator&, Args&&...)`
- [ ] 2.3 `set_server_name(ssl_stream&, std::string_view)` returning `boost::system::result<void>`, natively through OpenSSL
- [ ] 2.4 `teardown` and `async_teardown` for webcpp::asio's sockets, found by argument-dependent lookup
- [ ] 2.5 Static-assertion tests on native and WASI: the aliases are Beast's types, the stream derives from Beast's; also instantiate and run real protocol operations

## 3. The browser streams and the layers (requirements: Resolver, WebSocket layers)

- [ ] 3.1 Probe `basic_resolver_results::create` of Boost 1.92 (outside the documented interface) and pin the Boost version it needs
- [ ] 3.2 The browser `tcp_stream`: records the resolver's entry, keeps Beast's timeout interface on a Boost timer
- [ ] 3.3 `async_connect(results)` and `async_connect(endpoint)` (IPv6 in brackets, `browser_errc::no_host` on an unspecified address)
- [ ] 3.4 The browser `ssl_stream<tcp_stream>`: records the TLS settings, the scheme `wss` or `https`, and the handshake that judges them; reject exact-version methods under ERROR, including tlsv12_client
- [ ] 3.5 `set_server_name` in the browser: succeeds, records nothing

## 4. The browser WebSocket client (requirements: WebSocket, Settings, Back-pressure)

- [ ] 4.1 `async_handshake(host, target)`: the URL, the subprotocols, `binaryType = "arraybuffer"`, the `Host` rule, completion on `open`
- [ ] 4.2 `async_read` (binary from the `ArrayBuffer`, text with `TextEncoder.encodeInto`, the view retaken after a memory growth) and `async_read_some` with `is_message_done()`
- [ ] 4.3 `async_write`, `text()`, `binary()`, `got_text()`, `got_binary()`, and `async_write_some` buffering until `fin`
- [ ] 4.4 `async_close` with the codes the browser accepts, the reason bound of 123 UTF-8 bytes, and `read_message_max`
- [ ] 4.5 The settings table: honoured, unsupported and ignored settings, the decorator, `stream_base::timeout`, `suggested(role_type::client)`
- [ ] 4.6 The two unsupported policies at the operation that depends on the setting
- [ ] 4.7 The send bound and the receive queue, with their options, `send_limit` and `receive_limit`
- [ ] 4.8 Any completion token through `boost::asio::async_initiate`, handlers on webcpp::asio's io_context through its bridge
- [ ] 4.9 Synchronous browser handshake/read/write/close facade following the successful 0A.9 probe; preserve the synchronous examples, test heartbeat progress and reject unsupported nested entry
- [ ] 4.10 Test writes before OPEN and after close; never report a silently discarded send as successful peer delivery

## 5. Cancellation, lifetime, allocation and errors (requirements: Lifetime, Allocation, Errors)

- [ ] 5.1 The cancellation table (terminal, partial, total) for handshake, read, write and close
- [ ] 5.2 The destructor: abort pending operations (posted), close without a code, unregister from the bridge
- [ ] 5.3 The operation state through the handler's allocator; the stream's own state through the given allocator
- [ ] 5.4 The category `webcpp.beast` with seven fixed `browser_errc` values and the mapping of every browser outcome; preserve Boost.Beast's own error enum/category and test timeout identity
- [ ] 5.5 The counting test: nothing goes outside the given allocator, on both variants

## 6. The HTTP client in the browser (requirements: HTTP; second increment)

- [ ] 6.1 The write side: parse Beast's bytes with `request_parser`, one `fetch` per whole request, `redirect "follow"`, `mode "cors"`, body buffered
- [ ] 6.2 The read side: serialize the response with Beast's serializer, chunked for responses that permit bodies, header-only for HEAD/204/304; preserve the reason phrase and document omitted representation metadata; test the next response for leftover bytes
- [ ] 6.3 The field rules (`Host`, `Content-Length`, `Transfer-Encoding`, `Connection`, `Keep-Alive`, forbidden fields)
- [ ] 6.4 Redirects (`response.redirected`), `CONNECT` and unsendable bodies
- [ ] 6.5 `socket()` with `shutdown`, `close`, `is_open`, `cancel`, `get_executor`
- [ ] 6.6 `fetch_failed` for a rejected `fetch`
- [ ] 6.7 Synchronous HTTP client operations over the suspending facade; exercise methods, custom JSON bodies, bodyless responses and EOF without recursive context runs

## 7. Testing (requirements: Testing)

- [ ] 7.1 Native lanes: the names, the guide's programs against a local echo server
- [ ] 7.2 The native Beast echo servers, plain and TLS, built by the native lane; the local CA for `wss` (node: `NODE_EXTRA_CA_CERTS`; headless Chrome: the lane's certificate options)
- [ ] 7.3 Emscripten lanes under node on both suspension variants, each item of the specification's section 8
- [ ] 7.4 The headless-browser lane: CORS and mixed-content failures
- [ ] 7.5 The original three smoke-test clients, followed by all 53 inventory entries; no three-example completion criterion
- [ ] 7.6 Per-profile manifest/results: exact source diff, command, build result, runtime result, evidence and restriction; fail on a missing upstream target or a fallback/no-op main
- [ ] 7.7 Every HTTP client: sync/async, SSL, awaitable, stackful, local sockets, system executor, JSON body, methods and bounded crawler; distinguish original and adapted counterparts
- [ ] 7.8 Every WebSocket client: sync/async, SSL, awaitable, stackful, local sockets and system executor; test text/binary payloads and close behaviour
- [ ] 7.9 Every HTTP/WebSocket server, advanced/flex server, doc SSL example and echo-op on native/WASI where available; port thread/signal/lifetime dependencies explicitly, not invisibly
- [ ] 7.10 Chat multi-file application and HTML client with multiple participants; native/WASI servers serving Emscripten clients, with bounded startup and cleanup
- [ ] 7.11 Browser capability diagnostics for listeners, raw sockets and Unix sockets; retain unavailable original applications in the matrix
- [ ] 7.12 Expand all runnable entries across WASIp2/WASIp3 thread and EH profiles and browser JSPI/Asyncify with EH on/off; keep backend blockers visible and separate from platform-unavailable operations
- [ ] 7.13 Support headers and standalone documentation snippets receive compile/runtime harnesses; fixtures retain upstream licences and are not production trust stores
- [ ] 7.14 TLS trust/hostname failures, slow peers, cancellation during transfer, shutdown errors and cleanup across supported profiles; compile-only and Node-only results never count as browser success
- [ ] 7.15 Compare native upstream and webcpp payloads/status/observable behaviour, keeping raw output and documented HTTP normalization; publish totals including failed, blocked and unavailable rows

## 8. Documentation and the first release

- [ ] 8.1 Diataxis in AsciiDoc: tutorials, how-to guides, explanations and MrDocs API reference; the differences table names each unsupported and ignored case
- [ ] 8.2 Feature and all-example matrices with target, suspension, threading, EH, source changes and reproducible commands; every listing/output comes from a compiled/run example
- [ ] 8.3 The lint, the site, CI green on every declared native, WASIp2, WASIp3 and browser lane, with unresolved compatibility blockers reported separately, the changelog line, the tag
