## Why

A web program in C++ that talks peer to peer needs peer connections and data channels, and today
it has two bad choices: write the program twice, once against a native WebRTC stack and once
against the browser's JavaScript objects, or hide both behind a virtual interface of its own.
trystero, webcpp's serverless WebRTC rooms, took the second road: its `backend` interface, with
`native/peer_connection.hpp`, `native/data_channel.hpp` and `browser/backend.hpp`, is a callback
interface with one implementation per target, null on refusal, and it keeps WebRTC details
(the browser's cap of peer connections, the merge of two state machines, the frames that arrive
before a handler is installed) inside trystero where no other program can use them.

webcpp::webrtc is that interface done as a library: the W3C WebRTC API's shape and names
(`RTCPeerConnection`, `RTCDataChannel`), whose operations follow Asio's model (completion
tokens, handlers on webcpp::asio's executors, errors as values), so that a program is written
once and runs natively (libdatachannel) and in a browser (Emscripten, driving the browser's own
objects through webcpp::asio's bridge, as webcpp::beast's browser WebSocket does). Media (tracks,
codecs) is out of scope: data channels only, what trystero needs. It is the precondition of
trystero's move onto webcpp::webrtc, webcpp::beast (its relays), webcpp::asio and webcpp::crypto.

## What Changes

- **New library `webcpp::webrtc`**, webcpp's own design, a library of its own on webcpp::asio and
  not a part of webcpp::beast (the owner's decision of 2026-11-14), header-only C++20, namespace
  `webcpp::webrtc`, for two targets: native and Emscripten (both suspension variants, JSPI and
  Asyncify). wasip2 and wasip3 are not targets: there is no WebRTC stack for WASI, and the
  library's configuration stops with `#error` under WASI, naming the reason.
- The interface: `basic_peer_connection` and `basic_data_channel` templated on an executor
  (default `webcpp::asio::any_io_executor`) and an allocator, value types for inputs
  (`configuration`, `ice_server`, `session_description_view`, `ice_candidate_view`,
  `data_channel_init`) and owned outputs (`basic_session_description`, `basic_ice_candidate`),
  the five state and policy enumerations, `options` (webcpp's own bounds), and a `runtime` that
  owns what the platform holds for the whole process.
- Operations are `async_*` members with any completion token, their state in the handler's
  allocator, with the cancellation types each honours; events (candidates, state changes,
  incoming channels, open, close, buffered amount low) are pending operations the caller re-arms,
  as Asio's `async_accept`, with a stated policy for an event that arrives while no wait is pending.
- Errors are values: `webcpp::webrtc::error`, an `error_category` named `webcpp.webrtc`, ten
  enumerators with fixed values, the browser's DOMException names and libdatachannel's codes
  mapped onto them, plus Asio's own codes with their usual meaning.
- Back-pressure and bounds: `max_message_size`, `max_buffered_amount`, `max_received_bytes`,
  `max_queued_candidates`, `max_queued_channels` and `max_peer_connections`, each failing as a
  value.
- The native backend calls libdatachannel 0.24.6, the superproject's pinned and source-built
  copy, through its C API only, so the library works with exceptions on and off on both targets
  (the owner's decision of 2026-11-14); libdatachannel's MPL-2.0 licence is accepted for it.
- The browser backend follows webcpp::beast's proven patterns: one intrusive registration of the
  bridge per object, arrivals staged and charged on arrival, one notification per batch,
  JavaScript's exceptions caught in JavaScript, the unsupported policy in an inline namespace.
- A native peer and a headless-Chrome peer connect to each other in the tests.
- Increment 3 moves trystero onto this library and removes the WebRTC of trystero's `backend`;
  that is a change of trystero's, not of this proposal.

## Capabilities

### New Capabilities

- `webrtc`: webcpp::webrtc, the W3C WebRTC data-channel API in Asio's model for native and
  Emscripten, with its types, operations, event policies, error codes, bounds, lifetime rules,
  runtime, exception and allocation guarantees, per-target differences, tests and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`port_of: null`): the interface and its semantics are
  webcpp's design; libdatachannel and the browser are implementations behind it (design.md, D1).
  Repository `webcpporg/webrtc`, to be created by the owner, checked out as the submodule
  `libs/webrtc`, BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** increment 1 is written as reference code: `reference/source/` holds the
  interface and the native implementation over libdatachannel's C API with their tests, built and
  passed on 2026-11-14 against the superproject's pinned libdatachannel, as the build asks, without
  exceptions and under ThreadSanitizer, clean of clang-format and clang-tidy (`reference/STATUS.md`).
  `reference/plan.md` holds the three increments. Increment 1's measurement of the C API's cost,
  its documentation and its repository remain (tasks.md 1.11 to 1.13), then increments 2 and 3.
- **Dependencies:** webcpp::asio (the executors and the portable names; on Emscripten its
  bridge); Boost.Asio through webcpp::asio, Boost.System (`result`, `error_code`) and Boost's
  other headers; webcpp::config (`<webcpp/config.hpp>`); natively libdatachannel 0.24.6 at
  `6b1e2e6` with libjuice, libsrtp, usrsctp and plog, unmodified, MPL-2.0 and their own licences,
  built by the superproject from `third_party/` through `/webcpp/third_party//libdatachannel`
  against the same OpenSSL as Asio and Beast, with `threading=multi`; the browser's
  `RTCPeerConnection` and `RTCDataChannel` on Emscripten. It knows nothing of xactor or of
  webcpp::beast.
- **Status:** proposed, for its public discussion on the pull request; no release; not decided.
- **Other proposals:** depends on webcpp::asio's genesis (`libraries/asio`, implementing) for the
  executors and the browser bridge, and follows webcpp::beast's genesis (`libraries/beast`) in its
  browser patterns; webcpp::beast may later use a byte stream of this library (beast's D6).
  trystero (`libraries/trystero`) is its first user: increment 3 is a later change of trystero,
  which replaces its `backend`'s WebRTC, keeps its own policies (the shared connection, dropping
  text messages and oversized frames, its bound on unread frames, its per-minute window of
  connections) and merges the two states itself. Native encryption and randomness stay in
  webcpp::crypto, WebSockets in webcpp::beast.
