# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces, tests
and expected output. A checked task of group 0 is done in this proposal; a checked task of an
increment is done in `reference/source/`, built and tested as `reference/STATUS.md` records.

## 0. Before the proposal can be accepted

- [x] 0.1 Write `reference/plan.md`: the implementation plan in the three increments, one task per step with its files, interfaces, tests and expected output, done 2026-11-14
- [x] 0.2 Write the reference code: the interface (types, `runtime`, `basic_peer_connection`, `basic_data_channel`, `error`) and the native implementation over libdatachannel's C API, under `reference/source/`, with the native tests it passes against the superproject's pinned libdatachannel; update `reference/STATUS.md`, done 2026-11-14
- [x] 0.3 Obtain the owner's decisions: libdatachannel's MPL-2.0 licence, accepted; and, in place of the question on the compile-time message without exceptions, the native backend on libdatachannel's C API, so that the library works with exceptions on and off (design.md, D4 and D16), done 2026-11-14
- [x] 0.4 Check the draft against webcpp::asio's genesis and its built bridge, against webcpp::beast's browser WebSocket as built (its D3, D7, D9 to D12), and against trystero's `backend` at `e9326e9` with the shared connection and the preservation of early native frames; fix every difference or say why it stays (design.md, "What changed since the specification"), done 2026-11-14
- [x] 0.5 The draft specification, with the owner's decisions of 2026-10-10 and the controller's rulings on the review (section 2), done 2026-10-10 (`reference/spec.md`)
- [x] 0.6 The review's answers: no zero-copy receive for now, and separate event operations with the policies of section 5.2 (section 10), done 2026-10-10
- [x] 0.7 Record the owner's decision that webcpp::webrtc stays a library of its own on webcpp::asio, not a part of webcpp::beast, with its reasons (design.md, D1), done 2026-11-14
- [x] 0.8 Propose: the status `proposed`, the pull request, the public discussion (the owner's), done 2026-11-14 on webcpporg/proposals#12, where the discussion is held

## 1. Increment 1: the interface, the `runtime` and the native implementation

- [x] 1.1 The library's build: `build.jam` with `//headers`, `//webrtc` and the feature `webcpp-webrtc-unsupported`, `meta/libraries.json`, `config.hpp` with `WEBCPP_WEBRTC_NO_EXCEPTIONS`, the unsupported policy and the `#error` under WASI, the configuration test built three times (plan: Task 1.1)
- [x] 1.2 The enumerations and the input and output value types (plan: Task 1.2)
- [x] 1.3 `options` and `runtime_options`, and the `error` category `webcpp.webrtc` with its ten enumerators (plan: Task 1.3)
- [x] 1.4 The event machinery common to both backends: pending operations in the handler's allocator with their work and cancellation, the coalescing of states, the bounded queues, one wait of a kind, the cancelled wait that consumes nothing; tested without a platform, with exceptions on and off (plan: Task 1.4)
- [x] 1.5 The native `runtime`: thread pool size, `rtcPreload` and `rtcCleanup`, the count of peer connections; one at a time a precondition (plan: Task 1.5)
- [x] 1.6 The native `basic_peer_connection` over the C API: `make`, descriptions, candidates and the end of candidates, states, `create_data_channel`, `async_accept_data_channel`, `close`, destruction through `rtcDeletePeerConnection` (plan: Task 1.6)
- [x] 1.7 The native `basic_data_channel`: open, close, receive in libdatachannel's pull mode, send, text, buffered amount, the bounds, the drain on close and destruction (plan: Task 1.7)
- [x] 1.8 Exceptions: the native suite passes built with `exception-handling=off`; no header throws, tries or catches (plan: Task 1.8)
- [x] 1.9 Native tests: two peers in one process, the cases the semantics name, allocation through a counting resource, destruction under ThreadSanitizer, `run()` returning only once closed (plan: Task 1.9)
- [x] 1.10 Measure whether a never-read native channel stalls the connection's other channels: it does, and destroying it releases them; pinned by `test/stall_test.cpp` (plan: Task 1.10)
- [ ] 1.11 Measure the C API's cost on receipt against the C++ API's `receive()`, and state the ratio in the documentation (plan: Task 1.11)
- [ ] 1.12 Documentation: the library's page, libdatachannel's justification and licence, the table of differences, the cancellation and error tables, Doc Comments checked by MrDocs, examples (plan: Task 1.12)
- [ ] 1.13 The repository `webcpporg/webrtc` from one import commit, the superproject's change (submodule, lanes, chapter 15; the libdatachannel target's message is already worded for every consumer, webcpporg/webcpp#15), the whole lint, the lanes of a native-only increment, the review (plan: Task 1.13)

## 2. Increment 2: the browser implementation and the interop lane

- [ ] 2.1 The browser imports, `EM_ASM` in inline functions that catch JavaScript's exceptions in JavaScript, as webcpp::beast's (plan: Task 2.1)
- [ ] 2.2 The emscripten `runtime`: the page's census of `RTCPeerConnection`s kept in JavaScript, `max_peer_connections` (plan: Task 2.2)
- [ ] 2.3 The emscripten `basic_peer_connection`: one intrusive registration of webcpp::asio's bridge, staged arrivals and one notification, promises followed through the bridge, errors by name, destruction and shutdown (plan: Task 2.3)
- [ ] 2.4 The emscripten `basic_data_channel`: the receive queue charged on arrival to `max_received_bytes`, `receive_overflow`, `message_too_large` closing the channel, sends refused before the browser sees them (plan: Task 2.4)
- [ ] 2.5 `bind_address` and the other differences under the unsupported policy, its inline namespace and its link test (plan: Task 2.5)
- [ ] 2.6 Emscripten tests: every applicable case in one page, both suspension variants, exceptions on and off, allocation (plan: Task 2.6)
- [ ] 2.7 The interop lane: a native peer and a headless-Chrome peer, signalled through a local relay, with Chrome's two flags and `bind_address` `127.0.0.1`, both suspension variants (plan: Task 2.7)
- [ ] 2.8 Lint, all lanes, the review; increment 2 complete (plan: Task 2.8)

## 3. Increment 3: trystero on webcpp::webrtc

- [ ] 3.1 A change of trystero's (its own proposal) that replaces the WebRTC of its `backend`, `native/peer_connection.hpp`, `native/data_channel.hpp` and `browser/backend.hpp` with this library, using design.md's table "trystero's backend, part by part" (plan: Increment 3, step 1)
- [ ] 3.2 trystero keeps its policies: the shared connection and its adoption, dropping text messages and binaries larger than a room frame, the bound of 64 frames and 10 seconds, its per-minute window of connections, and the merge of the two states (plan: Increment 3, step 2)
- [ ] 3.3 trystero's native and browser suites pass unchanged on the new library; webcpp::webrtc is recorded as its dependency (plan: Increment 3, step 3)
