# trystero Specification

## Purpose
webcpp::trystero is a header-only C++20 port of Trystero 0.26.0's serverless WebRTC rooms: peers find
each other over public Nostr relays and exchange messages directly over WebRTC data channels, on
Trystero's wire, byte for byte, so that a room joined from C++ and the same room joined from
Trystero's JavaScript are one room. The rooms of an app share one connection to each remote peer,
as Trystero's do. It runs on two targets through a backend of each (the browser's WebRTC through
embind on Emscripten, libdatachannel natively), raises no exception of its own, allocates through
the `std::pmr::memory_resource` of an app's configuration and of a room's options, and is proven
against Trystero 0.26.0 itself by an oracle and an interop lane. These requirements describe 0.1.0
as built: the genesis (`../../changes/archive/2026-10-09-genesis/reference/source/`) and the change
`shared-connection` (`../../changes/archive/2026-11-14-shared-connection/`).

## Requirements

### Requirement: Library: A header-only port in its namespace
The library SHALL be a header-only C++20 port of Trystero 0.26.0 (https://github.com/dmotz/trystero, MIT, JavaScript), in namespace `webcpp::trystero`, with internals in `webcpp::trystero::detail`, the wire functions in `webcpp::trystero::protocol`, the interface a platform implements in `webcpp::trystero::backend`, the browser backend in `webcpp::trystero::browser` and the native backend in `webcpp::trystero::native`. Public headers are under `<webcpp/trystero/...>`, one per responsibility, macros are prefixed `WEBCPP_TRYSTERO_` and include guards are `WEBCPP_TRYSTERO_<PATH>_HPP` (`WEBCPP_TRYSTERO_HPP`, `WEBCPP_TRYSTERO_BACKEND_HPP`, `WEBCPP_TRYSTERO_DETAIL_POST_HPP`). The library's metadata (`meta/libraries.json`) names the key `trystero`, the author `R`, the categories `Concurrent` and `IO`, `cxxstd` 20 and `port-of` Trystero, language JavaScript, version 0.26.0, licence MIT. A rename of the library MUST NOT replace the word `trystero` by pattern: it uses the exact C++ patterns `namespace webcpp::trystero`, `<webcpp/trystero/` and `WEBCPP_TRYSTERO_`, and changes a string literal only when it is not on the wire.

#### Scenario: A program names the namespace
- **WHEN** a program includes `<webcpp/trystero.hpp>` and writes `namespace trystero = webcpp::trystero;`
- **THEN** `trystero::app`, `trystero::room`, `trystero::error`, `trystero::app_config`, `trystero::room_options`, `trystero::backend::factory` and `trystero::json_resource` are found

#### Scenario: The metadata names the original
- **WHEN** `meta/libraries.json` is read
- **THEN** it holds `"key": "trystero"`, `"authors": ["R"]`, `"cxxstd": "20"` and a `port-of` whose `name` is `Trystero`, `language` `JavaScript`, `version` `0.26.0`, `url` `https://github.com/dmotz/trystero` and `licence` `MIT`

### Requirement: Library: The wire is Trystero's and never changes
The library SHALL keep Trystero 0.26.0's wire byte for byte: what a Trystero JS peer or a page sees is unchanged, including the topic prefixes `"Trystero@"` and `"Trystero:"`, the handshake's `"__trystero_pw"`, every protocol constant and message layout under `include/webcpp/trystero/protocol/`, the default relays in their order, and the names embind exports, `trystero_dispatch` and the chat example's `join` and `send`. A room joined from C++ and the same room joined from Trystero's JavaScript MUST be one room: the peers see each other, and an action sent from one side arrives on the other. An exported or wire string MUST NOT be renamed by a renaming of the library; the error category name `webcpp.trystero` and the examples' app ids `webcpp-trystero-quick-start` and `webcpp-trystero-chat` are not on the wire and are the strings that were renamed.

#### Scenario: Changing a wire string fails the password scenario
- **WHEN** the string `"__trystero_pw"` is changed in the C++ library
- **THEN** the wrong-password scenario of the `interop` lane fails on both the native and the browser peer, as the C++ peer no longer completes the handshake with a Trystero JS peer

#### Scenario: A C++ peer and a Trystero JS peer share a room
- **WHEN** the `interop` lane joins a C++ peer and Trystero 0.26.0 peers in headless Chrome to one app and room over a Nostr relay of its own
- **THEN** they join each other, exchange text, JSON and binary messages, files both ways with metadata, and pings, and leave

### Requirement: Library: The convenience header and the public headers
The library SHALL ship the convenience header `include/webcpp/trystero.hpp`, which includes `<webcpp/trystero/config.hpp>` first and then `app.hpp`, `backend.hpp`, `error.hpp`, `json_resource.hpp`, `options.hpp` and `room.hpp` of `include/webcpp/trystero/`, and neither backend. The headers directly under `include/webcpp/trystero/` are `config.hpp` (the no-exceptions macro), `app.hpp` (`app`), `error.hpp` (`error`, its category), `handler.hpp` (`handler<R(Args...)>`), `json_resource.hpp` (`json_resource`), `backend.hpp` (the backend interface) and `options.hpp` (`app_config`, `room_options`, `default_ice_servers`) and `room.hpp` (`room`, `action<T>` and the rest of the public API). Every header of the library but `config.hpp` itself MUST include `<webcpp/trystero/config.hpp>` first, and `config.hpp` includes `<webcpp/config.hpp>`. The subdirectories are `protocol/` (`action_wire.hpp`, `envelope.hpp`, `json.hpp`, `nostr.hpp`, `relays.hpp`, `signal.hpp`, `text.hpp`, `topics.hpp`), `detail/` (`action_wire.hpp`, `app_impl.hpp`, `connection.hpp`, `construction_window.hpp`, `ice_servers.hpp`, `log.hpp`, `nostr_strategy.hpp`, `post.hpp`, `replay_cache.hpp`, `signaling.hpp`), `browser/` (`backend.hpp`, `event_loop.hpp`, `js.hpp`, `src.hpp`) and `native/` (`backend.hpp`, `data_channel.hpp`, `peer_connection.hpp`, `sealer.hpp`, `websocket.hpp`). The headers under `detail/` are internals.

#### Scenario: The convenience header brings the rooms and no backend
- **WHEN** a program includes only `<webcpp/trystero.hpp>` and makes a `trystero::app` over a `trystero::backend::factory&` and joins a room
- **THEN** it compiles on native and on emscripten, and no header of `native/` or `browser/`, no `rtc/` header, no `openssl/` header and no `emscripten/` header is included

#### Scenario: Every header compiles alone
- **WHEN** `test/Jamfile` compiles each public header as its own translation unit (`webcpp.headers-alone trystero`) with `trystero.hpp`, `trystero/*.hpp`, `trystero/protocol/*.hpp` and `trystero/detail/*.hpp` on both targets, `trystero/native/*.hpp` natively and `trystero/browser/*.hpp` on emscripten
- **THEN** each compiles, so no header depends on another being included before it

### Requirement: Library: The core is free of the two backends
The library SHALL keep a core of `include/webcpp/trystero.hpp`, the headers directly under `include/webcpp/trystero/`, `protocol/*.hpp` and `detail/*.hpp`, which builds on both targets, native and emscripten, and which MUST NOT include `webcpp/trystero/native`, `webcpp/trystero/browser`, `rtc/` (libdatachannel), `emscripten/` or `openssl/`. `meta/include-boundaries.json` records this as one boundary (`"why": "the core builds on both targets, without libdatachannel, OpenSSL or Emscripten"`), and the superproject's lint fails an include that crosses it. The browser backend (`browser/`) builds only on emscripten, over the page's WebRTC through embind; the native backend (`native/`) builds only natively, over libdatachannel, Boost.Beast and OpenSSL; `browser/src.hpp` holds the embind registration of `trystero_dispatch` and a program includes it once.

#### Scenario: The boundary file lists the forbidden includes
- **WHEN** `meta/include-boundaries.json` is read
- **THEN** its one boundary lists the headers `include/webcpp/trystero.hpp`, `include/webcpp/trystero/*.hpp`, `include/webcpp/trystero/protocol/*.hpp` and `include/webcpp/trystero/detail/*.hpp`, and the must-not-include entries `webcpp/trystero/native`, `webcpp/trystero/browser`, `rtc/`, `emscripten/` and `openssl/`

#### Scenario: A core header that includes a backend is refused
- **WHEN** a header under `detail/` includes `<webcpp/trystero/native/backend.hpp>`
- **THEN** the lint fails on that include

### Requirement: Library: Layers
The library SHALL be built in layers, each knowing only the one below it: the protocol (`protocol/*.hpp`, pure functions over bytes and text, with no I/O and no platform), the backend interface (`backend.hpp`, the WebSocket, peer connection, data channel, timer and AES-GCM sealer a platform provides), the app and the room (`app.hpp`, `options.hpp`, `room.hpp` and `detail/*.hpp`: the app's connections shared by its rooms and their bindings, the Nostr strategy, the signaling, the action wire and the public API), the browser backend (`browser/*.hpp`, `emscripten::val` over the browser's APIs and the event loop) and the native backend (`native/*.hpp`, libdatachannel, Boost.Beast, OpenSSL and Asio's timers). The same room MUST run over the browser's APIs, over libdatachannel and over the memory relays of the tests. The backend is a handful of abstract classes rather than template parameters, so that the room's code stays free of templates over the platform and of `#ifdef`, and a test backend and the browser backend are interchangeable at run time.

#### Scenario: A room runs over the tests' backend
- **WHEN** `room_test` joins rooms of two `trystero::app` objects over `trystero::test::memory_network` (`test/support/memory_backend.hpp`), a `backend::factory`
- **THEN** two rooms join, exchange messages and leave in one process, on native and on emscripten, with no network

### Requirement: Library: Exceptions policy and WEBCPP_TRYSTERO_NO_EXCEPTIONS
The library SHALL raise no exception of its own: an operation that can fail completes with an error code, and a raise would go through `boost::throw_exception`. `<webcpp/trystero/config.hpp>` MUST define `WEBCPP_TRYSTERO_NO_EXCEPTIONS` exactly when `WEBCPP_HAS_EXCEPTIONS` is not defined by `<webcpp/config.hpp>` (the compiler has no exceptions, or the developer defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library), and MUST keep a definition the developer gave on the command line in a build that has exceptions, to disable trystero's alone. The rooms, the protocol and the browser backend compile without exceptions, and so do the native timer, sealer and WebSockets. The headers `native/data_channel.hpp` and `native/peer_connection.hpp`, and `native::backend`, which makes their objects, sit behind `#ifndef WEBCPP_TRYSTERO_NO_EXCEPTIONS`: libdatachannel reports errors by throwing, so while the macro is defined those three types do not exist and code that uses them does not compile, and the rest of the library is unchanged.

#### Scenario: Exceptions follow webcpp's configuration
- **WHEN** `test/config_test.cpp` is compiled as the build asks (target `config`)
- **THEN** it compiles only if `defined(WEBCPP_HAS_EXCEPTIONS) != defined(WEBCPP_TRYSTERO_NO_EXCEPTIONS)`, else `#error "WEBCPP_TRYSTERO_NO_EXCEPTIONS is not defined exactly when webcpp has no exceptions"`

#### Scenario: WEBCPP_NO_EXCEPTIONS defines the macro
- **WHEN** `config_test.cpp` is compiled with `WEBCPP_NO_EXCEPTIONS` and `WEBCPP_TEST_TRYSTERO_ALL_OFF` defined (target `config_all_off`)
- **THEN** it compiles, `WEBCPP_TRYSTERO_NO_EXCEPTIONS` being defined, else `#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_TRYSTERO_NO_EXCEPTIONS"`

#### Scenario: A developer's own definition is kept
- **WHEN** `config_test.cpp` is compiled with `WEBCPP_TRYSTERO_NO_EXCEPTIONS` and `WEBCPP_TEST_TRYSTERO_DEVELOPER_DEFINED` defined (target `config_developer_defined`) in a build that has exceptions
- **THEN** it compiles, the macro still being defined after `config.hpp` is included, else `#error "config.hpp drops WEBCPP_TRYSTERO_NO_EXCEPTIONS, which the developer defined"`

### Requirement: Library: The native backend needs exceptions for libdatachannel
The library SHALL expose `native::data_channel`, `native::peer_connection` and `native::backend` only when exceptions are enabled, because libdatachannel reports errors by throwing and `native/data_channel.hpp` and `native/peer_connection.hpp` catch what it throws. Every test and program that uses the native backend's WebRTC declares `<exception-handling>on`, with the reason in a comment, and so does the headers-alone call of those two headers, which declare nothing without exceptions: `/webcpp/trystero//native` cannot declare it itself, because b2 ignores a non-free feature among usage requirements, so a user's `exception-handling=off` would reach them. The lint names those two headers as left out of its analysis without exceptions; `native/backend.hpp`, `native/sealer.hpp` and `native/websocket.hpp` are compiled alone in a call of their own, without that requirement, so the native timer, sealer and WebSockets are analysed without exceptions too.

#### Scenario: The headers-alone calls separate the two groups
- **WHEN** `test/Jamfile` is read
- **THEN** `trystero/native/backend.hpp`, `trystero/native/sealer.hpp` and `trystero/native/websocket.hpp` are compiled alone under `<library>/webcpp/trystero//native`, and `trystero/native/data_channel.hpp` and `trystero/native/peer_connection.hpp` under `<library>/webcpp/trystero//native <exception-handling>on`

### Requirement: Library: No JavaScript exception enters WebAssembly
The browser backend SHALL make every call into JavaScript that can throw (a constructor, `JSON.parse`, a `send`, a WebRTC or Web Crypto method) through `browser/js.hpp`, whose JavaScript, defined once in `browser/src.hpp` with `EM_JS`, runs it inside `try` and returns a value or the exception as a value, which the C++ turns into the error its interface has. Every event of the page MUST enter C++ through a function of the same file that catches what still escapes, puts WebAssembly's stack pointer and the event loop's depth back as they were before the call, reports it and lets the loop poll again. With exceptions, `browser::dispatch` catches a C++ exception first, which frees it, since one that reached JavaScript would keep its object for good; a message that is not a frame is dropped before it is read.

#### Scenario: The browser tests run both ways
- **WHEN** `test/Jamfile` declares the browser tests (`browser_event_loop`, `browser_websocket`, `browser_websocket_bound`, `browser_websocket_late_events`, `browser_sealer`, `browser_data_channel`, `browser_peer_connection`)
- **THEN** each also has a `*_no_exceptions` twin built with `<exception-handling>off`, and all run under node on emscripten

### Requirement: Library: External dependencies and their versions
The library SHALL depend on Boost 1.92 (Asio, Container for its `pmr` base class, Core, Hash2, JSON and System, and Beast for the native backend), on libsecp256k1 0.8.0 with its `schnorrsig` and `extrakeys` modules (every room, for the BIP-340 signatures of Nostr events), on libdatachannel 0.24.6 and OpenSSL 3 (the native backend alone), and, in the browser, on Emscripten 6.0.11 and a page served from a secure context (https, or http on `localhost`) where `crypto.subtle` exists. libdatachannel (Mozilla Public License 2.0) and OpenSSL (Apache License 2.0) are needed only by the native backend, which a browser program never includes: `/webcpp/trystero//trystero` depends on neither and `/webcpp/trystero//native` adds them. libsecp256k1, libdatachannel and OpenSSL are not part of Boost: `build.jam` finds them with `webcpp.external` and `webcpp.external-found`, from `-sSECP256K1_ROOT`, `-sSECP256K1_EMSCRIPTEN_ROOT`, `-sLIBDATACHANNEL_ROOT` and `-sOPENSSL_ROOT` on b2's command line (never the environment), else from the superproject's `.local/` directories, else, natively outside Windows, from `/usr/include` or `/usr/local/include`. A library found nowhere MUST leave trystero out with one message naming the library and where it looked, and `webcpp-require-external=on` stops the build instead.

#### Scenario: A missing library leaves trystero out
- **WHEN** b2 runs `b2 libs/trystero/test` without libsecp256k1 found in any candidate and without `webcpp-require-external=on`
- **THEN** trystero's tests and examples are skipped, b2 says so once naming the library and every place it looked, and the other libraries build

#### Scenario: CI requires the libraries
- **WHEN** the CI builds with `webcpp-require-external=on` and a library is not found
- **THEN** the build stops

### Requirement: Library: Licensing and the notice of Trystero
The library SHALL be distributed under the Boost Software License, Version 1.0 (`LICENSE_1_0.txt`), with the copyright line "Copyright (c) 2026 WebCpp.org" in each source file, and MUST ship `LICENSE-TRYSTERO.txt`, Trystero's MIT licence and copyright notice exactly as Trystero 0.26.0 ships them, since the wire protocol, its constants, its message layouts, its default relays (`protocol/relays.hpp`, with their order and the shuffle that derives an app's relays from its id) and its default STUN servers derive from Trystero. That notice adds no licence to trystero's own code, which is under the Boost Software License alone. The README, in "What derives from Trystero", and the page's acknowledgements name what derives from Trystero, and credit libsecp256k1 (MIT) and libdatachannel (Mozilla Public License 2.0).

#### Scenario: The notice is unchanged
- **WHEN** `LICENSE-TRYSTERO.txt` is compared with the notice Trystero 0.26.0 ships
- **THEN** it is the MIT licence text with Trystero's copyright line, unchanged

#### Scenario: The README lists what derives
- **WHEN** the README's section "What derives from Trystero" is read
- **THEN** it lists the wire protocol, the constants (topic prefixes, id alphabet and lengths, announcement cadence, event kinds, backpressure threshold), the message layouts (signals, action frames, envelope) and the default relays and STUN servers

### Requirement: Library: Guarantees of the wire
The library SHALL hold two guarantees about the wire, each with the test that fails when it breaks. Guarantee 1, anchor `trystero-invariant-1`: a C++ peer, in the browser or native, and Trystero JS peers in one room join, exchange text, JSON and binary messages, files both ways with metadata, and pings, leave, and form a mesh, with either side offering; held by the `interop` lane, `interop_browser` on emscripten and `interop_native` natively, against Trystero 0.26.0 in headless Chrome. Guarantee 2, anchor `trystero-invariant-2`: topics, room tokens, the relay shuffle, sealed text, Nostr event ids, inline and chunked action frames, control frames, envelopes and presence MUST equal what Trystero's own functions compute from the same inputs, byte for byte; held by `protocol` against `test/fixtures/expected/expected.json`, by the `oracle` lane, which fails when Trystero would write other values than those committed, and by `native_sealer`, which opens what Trystero JS sealed with Web Crypto.

#### Scenario: The oracle writes the committed values
- **WHEN** the `oracle` lane runs Trystero's functions over `test/fixtures/cases/protocol.json`
- **THEN** its output equals `test/fixtures/expected/expected.json` byte for byte, and `update-expected` is the only writer of that file

#### Scenario: The interop lane meets both C++ peers
- **WHEN** the `interop` lane runs `interop_native` and `interop_browser`
- **THEN** each meets Trystero JS peers in sixteen scenarios, the native peer in three more

### Requirement: Library: Guarantees of the room
The library SHALL hold six guarantees about a room. Guarantee 3, anchor `trystero-invariant-3`: a peer with another password never joins a protected C++ app's rooms because its initial signals do not open, with every room of that app using the same immutable password; the guarantee does not repair a remote JS page that mixes room passwords; held by `room` (`the_same_password_joins_and_another_does_not`), `native_room` (`another_password_never_joins`) and the wrong-password scenario of the `interop` lane. Guarantee 4, anchor `trystero-invariant-4`: a payload larger than a frame travels in chunks of at most 16,370 bytes, both ways, and arrives whole from either peer; held by `room` (`large_payloads_travel_in_chunks_both_ways`, 100 KB each way) and the file transfers of the `interop` lane. Guarantee 7, anchor `trystero-invariant-7`: a room MUST refuse a replayed or reflected offer or answer, a sealed offer or answer it made or acted on that comes back to it as a peer's changing nothing; held by `room` (`a_replayed_answer_is_refused`, `a_replayed_offer_is_refused`, `a_reflected_offer_is_refused_as_an_answer`, `a_reflected_offer_is_refused_as_an_offer`, `a_reflected_answer_is_refused_as_an_answer`) and the replay and reflection scenarios of the `interop` lane. Guarantee 9, anchor `trystero-invariant-9`: two peers that meet in several rooms of the same app MUST share one connection, one per remote peer per app; held by `shared` and the first sharing scenario of the `interop` lane. Guarantee 10, anchor `trystero-invariant-10`: a room's departure MUST leave the app's other rooms joined; held by `shared` (a room leaves) and the interop scenario of one room leaving. Guarantee 11, anchor `trystero-invariant-11`: different password domains MUST share no C++ app connection or presence; held by `shared` (passwordless membership and separate protected-app isolation in both id orders) and the interop scenario of the passwordless room. That a passwordless room is open to those peers is stated in the page's "What remains", not as a guarantee.

#### Scenario: Another password reports only wrong_password
- **WHEN** `the_same_password_joins_and_another_does_not` runs an app with the password `s3cret` and an intruder app with another one for 1500 ms
- **THEN** the intruder has no peers, the room has exactly one peer, errors were reported, and every reported code equals `trystero::error::wrong_password`

#### Scenario: A large payload travels in chunks
- **WHEN** `large_payloads_travel_in_chunks_both_ways` sends 100 KB from each peer to the other
- **THEN** each arrives whole

### Requirement: Library: Guarantees of the resource, the loopback and the relay bound
The library SHALL hold three further guarantees. Guarantee 5, anchor `trystero-invariant-5`: every allocation of a room goes through its resource, but what the "Memory: What stays outside the resource" requirement lists; from its construction until its `io_context` has drained, the global `operator new` sees only the room's JSON storage counter and the C library nothing; held by `allocation`, natively and on emscripten. Guarantee 6, anchor `trystero-invariant-6`: no unit test reaches beyond the loopback, the rooms of the unit tests meeting over the memory relays, the native tests' peer connections binding to 127.0.0.1 and gathering their candidates there alone, and the WebSocket test talking to a server of its own on 127.0.0.1; held by the container lanes `gcc-14` and `clang-linux-18`, where every unit test, built with GCC 14 and with clang 18 against libstdc++, passes in a Linux container with no network but the loopback (`docker run` with the network `none`). Guarantee 8, anchor `trystero-invariant-8`: a relay that stops reading MUST be dropped once the text held unsent for it would pass 4 MiB, reporting one close, after which the room reconnects with its backoff; held by `native_websocket` (`a_relay_that_stops_reading_is_dropped_at_the_queue_bound`, a close before 2000 texts of 64 KiB, and one only) natively and `browser_websocket_bound` on emscripten, with and without exceptions, which also checks that the socket never holds more than 4 MiB, and by `room` (`a_relay_that_closes_at_once_is_tried_less_and_less_often`).

#### Scenario: The unit tests run with no network
- **WHEN** every unit test is built with GCC 14 and with clang 18 and run in a Linux container started with the network `none`
- **THEN** every test passes

#### Scenario: A relay that stops reading is dropped once
- **WHEN** `native_websocket` queues texts of 64 KiB to a relay that does not read
- **THEN** the connection is closed before 2000 texts, and the close is reported once

### Requirement: Library: The guarantees keep their anchors
The page's section "Guarantees" SHALL number eight guarantees, each keeping its number and its anchor `trystero-invariant-<n>` for good, so that a comment of the code or of a test cites it as `(doc: #trystero-invariant-3)`. Building the page (`b2 libs/trystero/doc`) MUST fail on a citation that names no anchor of it. The test files cite guarantees this way: `allocation_test.cpp` cites `#trystero-allocation`.

#### Scenario: A citation of a missing anchor fails the page
- **WHEN** a comment cites `#trystero-invariant-9` and the page is built
- **THEN** the build of `libs/trystero/doc` fails

### Requirement: Library: Messages, binary data and pings are ported; media is not
The library SHALL port Trystero's text, JSON and binary messages and its pings, and MUST NOT port audio and video tracks. Not ported are, each with its reason in the page's "Differences from Trystero": the media API (`addStream`, `removeStream`, `addTrack`, `removeTrack`, `replaceTrack`, `onPeerStream`, `onPeerTrack`, with the actions `@_stream`, `@_track` and `@_signal`), request actions (`kind: 'request'`), `onReceive` and the progress callbacks and cancelling with an `AbortSignal`, `onPeerHandshake`, passive rooms, renegotiation and media over a shared connection, relay sockets and a Nostr key shared by the rooms of an app, the BitTorrent, MQTT, Firebase, Supabase, IPFS and WebSocket relay strategies, `getRelaySockets` and its relatives, and `trickleIce: false`, `turnConfig` and `rtcPolyfill`. A room MUST ignore a frame of one of the media actions. Where a ported feature behaves otherwise, the page says so: relays and their sockets kept per room rather than shared by the rooms of an app, no replay of the joined peers to a newly set `on_peer_join`, `peers()` listing ids rather than connections, and `async_send` to a peer that has not joined completing with `error::no_such_peer`.

#### Scenario: A send to a stranger fails with a code
- **WHEN** `ping_measures_a_round_trip` calls `a.async_ping("nobody", ...)`
- **THEN** the handler receives `trystero::error::no_such_peer`

### Requirement: Library: Operations are Asio initiating functions
Every asynchronous operation of the library SHALL be an Asio initiating function that completes with a callback, `boost::asio::use_awaitable`, `boost::asio::deferred` or any other completion token, with a completion signature starting with `boost::system::error_code`. The library MUST NOT bind its own allocator over a user's completion handler: it is allocated as its own associated allocator says, which `boost::asio::bind_allocator` sets. Where text only needs reading, a parameter is a `std::string_view` and bytes are a `std::span`; a `std::string` is used where something must own the text. Asynchronous operations are the exception to zero-copy, since a token such as `asio::deferred` starts the operation later, so their arguments are owned.

#### Scenario: A coroutine receives a message
- **WHEN** `async_receive_works_in_a_coroutine` awaits an action's `async_receive` with `boost::asio::use_awaitable`
- **THEN** the coroutine receives the message sent by the other room

### Requirement: Errors: The error enumeration and its values
The library SHALL declare `enum class error` in `<webcpp/trystero/error.hpp>`, in `webcpp::trystero`, with these enumerators and fixed values: `room_left` = 1, `no_such_peer` = 2, `peer_disconnected` = 3, `invalid_action_name` = 4, `metadata_too_large` = 5, `transfer_rejected` = 6, `transfer_timed_out` = 7, `channel_stalled` = 8, `wrong_password` = 9, `handshake_timed_out` = 10, `handshake_failed` = 11, `backend_failure` = 12, `invalid_configuration` = 13, `already_joined` = 14 and `too_many_rooms` = 15; values are fixed and never reused. Each is an error Trystero rejects a promise with or reports to `onPeerError`, as a code of `error_category` rather than a message: an operation completes with it, `room::on_error` reports it with the peer it concerns and `app::join` returns the last two as values. The reference shows each enumerator with its value.

#### Scenario: The values are fixed
- **WHEN** `static_cast<int>(trystero::error::room_left)`, `static_cast<int>(trystero::error::invalid_configuration)`, `static_cast<int>(trystero::error::already_joined)` and `static_cast<int>(trystero::error::too_many_rooms)` are read
- **THEN** they are 1, 13, 14 and 15

### Requirement: Errors: Meaning and origin of each code
The library SHALL complete or report with each error for this reason and no other. `room_left`: the room was left (by `async_leave`, destruction or a move over it) while the operation was pending, Trystero's "room left", also for a send to a left room. `no_such_peer`: the target is not an active peer of the room, Trystero's "no active peer with id". `peer_disconnected`: the peer went away before the operation finished. `invalid_action_name`: the action's name is not 1 to 32 UTF-8 bytes without NUL, as Trystero requires. `metadata_too_large`: the action's metadata does not fit one frame. `transfer_rejected`: the receiver refused a transfer, or a received payload is not of the awaited type. `transfer_timed_out`: the receiver neither accepted nor refused a transfer in time. `channel_stalled`: the data channel stopped draining while a transfer waited for it. `wrong_password`: a peer's signal or handshake used another password, or the peer sent a handshake message the protocol does not expect, Trystero's "incorrect password for overlapping room"; a sealer's `open` also completes with it when the text was sealed under another key or is not a sealed text. `handshake_timed_out`: a bound peer did not finish the handshake in `room_options::handshake_timeout`, Trystero's "handshake timed out". `backend_failure`: the browser or the native stack refused an operation, as a refused description or candidate, a failed seal or a peer connection the factory could not make. `invalid_configuration`: the app's configuration names something no backend can use, an ICE server whose URL WebRTC refuses, a `self_id` longer than 64 bytes or a negative `idle_timeout`; the app goes on without it and `app::on_error` reports it with no peer. `already_joined`: `app::join` was given a room id that a room of the app already holds. `too_many_rooms`: `app::join` was called while `app::max_rooms` (64) rooms count, a Trystero peer dropping a connection that advertises more.

#### Scenario: A move over a joined room leaves it
- **WHEN** `moving_a_room_over_a_joined_one_leaves_it` assigns a new `trystero::room` over a joined room that has a pending `async_receive`
- **THEN** the old room's peers leave, `peers()` of the other room is empty, and the pending receive completes with `trystero::error::room_left`

#### Scenario: Unusable ICE servers are reported with no peer
- **WHEN** `unusable_ice_servers_are_dropped_and_reported` makes an app whose `ice_servers` hold `stun:127.0.0.1:9`, `http://127.0.0.1:9`, a `turn:` server without credentials, a `turn:` server with the user `user` and credential `secret`, and `STUN:127.0.0.2:9`
- **THEN** `app::on_error` is called twice, each time with an empty peer and `trystero::error::invalid_configuration`, the backend is never given `http://127.0.0.1:9`, and it is given `turn:127.0.0.1:9` and the lowercased `stun:127.0.0.2:9`

#### Scenario: A second join and a 65th room are refused
- **WHEN** `joining_is_refused_twice_and_beyond_64` joins `r1` twice and then 64 more rooms
- **THEN** `join` returns `trystero::error::already_joined` for the second `r1` and `trystero::error::too_many_rooms` for the 65th room

#### Scenario: A refused peer connection is reported and tried again
- **WHEN** `a_refused_peer_connection_is_reported_and_tried_again` runs with the memory network refusing one peer connection
- **THEN** exactly one error is reported, equal to `trystero::error::backend_failure`, and a peer then joins on a later announcement

### Requirement: Errors: handshake_failed is reserved
The library SHALL keep `error::handshake_failed` as a reserved value that nothing reports: a handshake message of the wrong shape reports `error::wrong_password`, as Trystero's handshake does. Its message is "the handshake failed". It MUST keep its value 11 so that the values after it do not move.

#### Scenario: No source reports it
- **WHEN** the library's headers are searched for `error::handshake_failed`
- **THEN** it appears only in `error.hpp`, in the enumeration and in the category's message table

### Requirement: Errors: The category webcpp.trystero and its messages
The library SHALL declare `class error_category_impl final : public boost::system::error_category` whose `name()` is `noexcept` and returns `"webcpp.trystero"`, and whose messages are fixed static texts: `room_left` "room left", `no_such_peer` "no active peer with this id", `peer_disconnected` "peer disconnected", `invalid_action_name` "action names must contain 1 to 32 UTF-8 bytes and no NUL", `metadata_too_large` "action metadata is too large", `transfer_rejected` "the receiver rejected the transfer", `transfer_timed_out` "the action transfer timed out", `channel_stalled` "the data channel stopped draining", `wrong_password` "the peer uses another room password", `handshake_timed_out` "the handshake timed out", `handshake_failed` "the handshake failed", `backend_failure` "the WebRTC backend refused an operation" and `invalid_configuration` "the configuration is unusable", `already_joined` "the app has already joined a room of that id" and `too_many_rooms` "the app holds as many rooms as a peer accepts, 64". A value that `error` does not name MUST give "unknown webcpp.trystero error". `message(int)` returns a `std::string`; the overload `message(int, char*, std::size_t)` is `noexcept`, does not use its buffer and returns the static text, so a room logs an error without allocating.

#### Scenario: The name is the category's
- **WHEN** `trystero::make_error_code(trystero::error::room_left).category().name()` is read
- **THEN** it is `"webcpp.trystero"`

#### Scenario: An unnamed value
- **WHEN** `trystero::error_category().message(99)` is called
- **THEN** it returns `"unknown webcpp.trystero error"`

#### Scenario: A message without a buffer
- **WHEN** `error_category().message(static_cast<int>(trystero::error::wrong_password), nullptr, 0)` is called
- **THEN** it returns `"the peer uses another room password"`, writes nothing and allocates nothing

### Requirement: Errors: error_category(), make_error_code and the enum trait
The library SHALL give `const inline boost::system::error_category& error_category() noexcept`, which returns the one instance of `error_category_impl` (a function-local static), `inline boost::system::error_code make_error_code(error e) noexcept`, which returns `{static_cast<int>(e), error_category()}` and is found by argument-dependent lookup, and the specialization `boost::system::is_error_code_enum<webcpp::trystero::error>` deriving from `std::true_type`. An `error` MUST therefore convert implicitly to a `boost::system::error_code` and compare with one: `ec == trystero::error::no_such_peer` is true for the code a failed operation completed with. Two codes of the same value from different calls compare equal because the category instance is one.

#### Scenario: A code compares with the enumerator
- **WHEN** `ping_measures_a_round_trip` pings `"nobody"` and the handler receives `ec`
- **THEN** `*missing == trystero::error::no_such_peer` is true and `!*result` is true for the successful ping

#### Scenario: A code is built from the enumerator
- **WHEN** a program writes `boost::system::error_code ec = trystero::error::room_left;`
- **THEN** `ec.value()` is 1 and `ec.category().name()` is `"webcpp.trystero"`

### Requirement: Errors: Errors are values, never exceptions
The library SHALL report every failure of an operation as an `error_code` and MUST NOT throw one of its own: the backend interface's completions report `boost::system::error_code` (alias `backend::error_code`), empty on success; a backend that cannot make an object returns null where its contract says so and the room reports the negotiation as failed through `room::on_error`; and `room::on_error` reports the code, the peer it concerns (empty when none) and a sentence. Library errors cross the C++/JavaScript boundary of the browser backend as values (see "Library: No JavaScript exception enters WebAssembly"), and a library error reaches a user's handler only through a completion handler or `on_error`.

#### Scenario: A failed operation completes, it does not throw
- **WHEN** a room is built with exceptions disabled (`WEBCPP_TRYSTERO_NO_EXCEPTIONS`) and `async_send` targets a peer that is not joined
- **THEN** the completion handler receives `trystero::error::no_such_peer` and nothing is thrown

### Requirement: Backend: The seam between the room and a platform
The library SHALL define the backend interface in `<webcpp/trystero/backend.hpp>`, namespace `webcpp::trystero::backend`, as the abstract classes `timer`, `websocket`, `data_channel`, `peer_connection`, `sealer` and `factory`, the value types `ice_server` and `connection_state`, and the aliases `bytes` (`std::pmr::vector<std::uint8_t>`, a data channel frame in the memory resource of the room it is for) and `error_code` (`boost::system::error_code`). It is what a target must provide for a room to run on it: the browser provides it over `emscripten::val` (`webcpp/trystero/browser`), a native target over libdatachannel, Boost.Beast and OpenSSL (`webcpp/trystero/native`), and the tests over memory (`test/support/memory_backend.hpp`). Every abstract class has a virtual destructor, and the room holds the objects of the interface as `std::shared_ptr`.

#### Scenario: Three implementations exist
- **WHEN** the repository is searched for classes deriving from `backend::factory`
- **THEN** `browser::backend`, `native::backend` and `test::memory_network` are found

### Requirement: Backend: Handlers run on the room's executor, after the call returns
Every handler a backend invokes SHALL run on the room's executor, one at a time, after the call that caused it has returned and never inside it, and a handler may destroy the object that invoked it. A completion (`done`) of a `peer_connection` or a `sealer`, and a `websocket`'s `close` when no socket can be made, MUST be posted even when their result is known at once, because the room calls these operations from inside handlers whose state a completion run there would change under them. A backend whose events arrive on other threads (libdatachannel) posts them to the executor first. The room's own handling follows: `set_handlers` of a `data_channel` or a `peer_connection` may be called by a handler during its own call.

#### Scenario: A seal completes after it returns
- **WHEN** a room calls `sealer::seal(plaintext, done)` on the memory backend
- **THEN** `done` is posted to the `io_context` and is not called before `seal` returns

### Requirement: Backend: Text and bytes cross the interface as views
The backend interface SHALL pass text as `std::string_view` valid for the duration of the call or of the handler, and bytes as `std::span<const std::uint8_t>`: a backend reads what it is given before returning, and a handler copies what it keeps. The one exception is a data channel message, handed to the handler by value as `backend::bytes` (`handler<void(bytes)>`), so that the receiver keeps the frame and taking ownership of it spares a copy. A `peer_connection`'s `ice_candidate` handler is given the JSON of an `RTCIceCandidateInit` as a view valid for the call, and the `done` of `set_local_description` receives views of the description's type (`"offer"` or `"answer"`) and its SDP valid for the call.

#### Scenario: A received frame is owned by the receiver
- **WHEN** a data channel reports a message to `handlers::message`
- **THEN** the handler receives a `backend::bytes` by value, in the room's memory resource, which it may move out

### Requirement: Backend: ice_server
The library SHALL define `struct backend::ice_server` as a STUN or TURN server, an `RTCIceServer` of the `iceServers` Trystero gives `RTCPeerConnection`, with the members `std::pmr::vector<std::pmr::string> urls` (as `"stun:stun.l.google.com:19302"` or `"turn:host:3478"`), `std::pmr::string username` and `std::pmr::string credential`, the last two empty for a STUN server. A room copies those of its configuration into its own resource. `factory::make_peer_connection` receives them as `std::span<const ice_server>`, read during the call.

#### Scenario: A TURN server carries credentials
- **WHEN** a configuration holds the server `{urls: {"turn:127.0.0.1:9"}, username: "user", credential: "secret"}`
- **THEN** the room keeps it, and the backend is given `turn:127.0.0.1:9` with that user name and credential

### Requirement: Backend: timer
The library SHALL define `backend::timer` as a one-shot timer, what Trystero's `setTimeout` calls become, with `virtual void start(std::chrono::milliseconds delay, handler<void()> fire) = 0` and `virtual void cancel() = 0`. `start` calls `fire` on the room's executor once `delay` has passed, in place of a pending expiry (starting again replaces it), `cancel` ends the pending expiry so that its handler is never called, and destroying the timer cancels it. The timer is made by `factory::make_timer(memory)` and not started.

#### Scenario: A restarted timer fires once
- **WHEN** a timer is started with 100 ms and started again with 50 ms before the first expiry
- **THEN** only the second handler is called, once

### Requirement: Backend: websocket
The library SHALL define `backend::websocket`, a WebSocket to a Nostr relay as the `WebSocket` Trystero's relay manager opens, with `virtual void send(std::string_view text) = 0` and `virtual void close() = 0`, and the nested `struct handlers { handler<void()> open; handler<void(std::string_view)> message; handler<void()> close; }`, each called on the room's executor. `send` before the connection opens or after it closes drops the text. `close` is called once, also when the socket never opened or could not be made, and reports the close of `close()` too. Destroying the websocket closes it without reporting the close. `factory::connect(url, handlers, memory)` takes a `ws://` or `wss://` URL, read during the call, and returns the websocket, which reports a close when it cannot connect.

#### Scenario: A failed connection reports a close
- **WHEN** the factory is asked to connect to a URL no WebSocket can be made to
- **THEN** `handlers::close` is called once, after `connect` returned, and `open` never is

### Requirement: Backend: data_channel
The library SHALL define `backend::data_channel`, the `RTCDataChannel` a Trystero peer exchanges its frames over, with `set_handlers(handlers)`, `is_open() const`, `send(std::span<const std::uint8_t>)`, `buffered_amount() const` (the `bufferedAmount`) and `close()`, and the nested `struct handlers { handler<void()> open; handler<void(bytes)> message; handler<void()> close; handler<void()> buffered_amount_low; }`. `static constexpr std::size_t low_threshold = 0xffff` is Trystero's `bufferedAmountLowThreshold`, the backpressure threshold: `buffered_amount_low` is called when the buffered amount falls to `low_threshold` or below after being above it, and senders wait while more is buffered. `send` on a channel that is not open drops the message. `set_handlers` replaces the handlers, an empty one is not called, and a handler may replace them during its own call. `close` is reported once through `handlers::close`; destroying the channel closes it without reporting.

#### Scenario: The threshold is Trystero's
- **WHEN** `backend::data_channel::low_threshold` is read
- **THEN** it equals 0xffff (65535)

#### Scenario: A send on a closed channel is dropped
- **WHEN** `send` is called on a channel whose `is_open()` is false
- **THEN** nothing is transmitted and no error is raised

### Requirement: Backend: connection_state
The library SHALL define `enum class backend::connection_state` with the enumerators `fresh` ("new"), `connecting` ("connecting", or ICE's "checking"), `connected` ("connected", or ICE's "completed"), `disconnected` ("disconnected", perhaps for a moment), `failed` ("failed") and `closed` ("closed"), the states of an `RTCPeerConnection`, in this order, reported through `peer_connection::handlers::state`.

#### Scenario: The states are the six of WebRTC
- **WHEN** the enumerators of `connection_state` are listed
- **THEN** they are `fresh`, `connecting`, `connected`, `disconnected`, `failed` and `closed`

### Requirement: Backend: peer_connection
The library SHALL define `backend::peer_connection`, the `RTCPeerConnection` of a Trystero peer, whose negotiation the signaling drives (it makes an offer or an answer only when asked), with `set_handlers(handlers)`, `create_data_channel(std::string_view label)`, `set_local_description(handler<void(error_code, std::string_view type, std::string_view sdp)> done)`, `set_remote_description(std::string_view type, std::string_view sdp, handler<void(error_code)> done)`, `add_ice_candidate(std::string_view candidate_json, handler<void(error_code)> done)`, `has_remote_description() const` and `close()`, and the nested `struct handlers { handler<void(std::string_view)> ice_candidate; handler<void(std::shared_ptr<data_channel>)> data_channel; handler<void(connection_state)> state; }`. `create_data_channel` returns the channel made in the connection's resource, or null when the platform refuses (as on a closed connection). `set_local_description` creates the offer or answer the signaling state calls for and applies it; its `done` is called with the type and SDP, or with `error::backend_failure` and empty views. `set_remote_description` and `add_ice_candidate` complete with `error::backend_failure` when refused. A candidate MUST be added only after `has_remote_description()` is true. `close` closes the connection and its channels; destroying the connection closes it without reporting.

#### Scenario: A refused description reports backend_failure
- **WHEN** a backend refuses a remote description
- **THEN** the `done` of `set_remote_description` is called with `trystero::error::backend_failure`

#### Scenario: A closed connection gives no channel
- **WHEN** `create_data_channel` is called on a closed connection
- **THEN** it returns a null `std::shared_ptr`

### Requirement: Backend: sealer
The library SHALL define `backend::sealer` as AES-GCM under one room key in Trystero's sealed text form (`protocol/signal.hpp`), what Trystero's `encrypt` and `decrypt` do with `crypto.subtle`, with `seal(std::string_view plaintext, handler<void(error_code, std::string_view sealed)> done)` and `open(std::string_view sealed, handler<void(error_code, std::string_view plaintext)> done)`. `seal` uses a fresh random IV and completes with the sealed text, or with `error::backend_failure` and an empty view; `open` completes with the plaintext, or with `trystero::error::wrong_password` and an empty view when the text was sealed under another key or is not a sealed text. The views are valid for the call of `done`. `factory::make_sealer(std::array<std::uint8_t, 32> key, memory)` makes one under the raw AES-256 key `protocol::seal_key` derives.

#### Scenario: Another key does not open
- **WHEN** a text sealed by a sealer under one key is opened by a sealer under another
- **THEN** the completion carries `trystero::error::wrong_password` and an empty view

#### Scenario: Trystero's sealed text opens
- **WHEN** `native_sealer` opens a text Trystero JS sealed with Web Crypto, from `fixtures/expected/expected.json`
- **THEN** it completes with the plaintext, and what the native sealer seals opens in Trystero's `decrypt`

### Requirement: Backend: factory
The library SHALL define `backend::factory`, which makes everything above and gives a room its randomness and its clocks (one per page in the browser, one per application natively, one per test), with these pure virtual operations: `connect(std::string_view url, websocket::handlers h, std::pmr::memory_resource* memory)` returning `std::shared_ptr<websocket>`, `make_peer_connection(std::span<const ice_server> servers, std::pmr::memory_resource* memory)` returning `std::shared_ptr<peer_connection>` (null when the platform refuses to make one, whereupon the room reports the negotiation with that peer as failed through `room::on_error`), `make_timer(memory)` returning `std::shared_ptr<timer>`, `make_sealer(std::array<std::uint8_t, 32> key, memory)` returning `std::shared_ptr<sealer>`, `random(std::span<std::uint8_t> out)`, `unix_seconds()` returning `std::int64_t` (wall-clock seconds since the Unix epoch, for Nostr's `created_at` and `since`) and `now_ms()` returning `double` (a monotonic clock in milliseconds from an origin of the backend's, as `performance.now()`, for rate limits and timeouts). `random` fills from a cryptographically secure source, since the bytes become secret keys and nonces. The factory MUST outlive every room that uses it.

#### Scenario: The room draws its ids from the factory
- **WHEN** `app_config::self_id` is empty
- **THEN** the app draws its id of 20 random characters from `factory::random`, once

### Requirement: Backend: Objects are made in the room's memory resource
Every method of `backend::factory` that makes an object SHALL take a `std::pmr::memory_resource*` as its last parameter, and the backend MUST allocate the object, its handlers and what it hands to its owner (a frame, a text) through it, on the app's executor only. The signaling hands the factory the app's resource for `make_peer_connection`, so the peer connections and data channels that every room's negotiations make, and the frames received on them, are in the app's resource; the other objects (WebSockets, timers, sealers) are made in the resource of the room that asks. The resource must outlive the object and every operation the object leaves pending. The shared state of a backend (a browser's `event_loop`, the tests' memory relays) is shared by every room of a page or of a test and allocates the entries of its registrations and its relays through the resource it is constructed with, while a registered callback stays in its own resource.

#### Scenario: The memory backend allocates in the room's resource
- **WHEN** `allocation_test` runs two apps, each with rooms, over `memory_network` with the counting resources of the apps and of the rooms with a resource of their own
- **THEN** the peer connections the network makes are counted by their app's resource, the other objects by their room's, and the network's own state by `network_memory` (`network_memory.allocations > 0`)

### Requirement: Memory: One resource per room
The library SHALL route every allocation of an app through the one `std::pmr::memory_resource` that `app_config::memory` names, and every allocation of a room through its own, `room_options::memory`, the app's when null, from construction to destruction. The app's resource receives the app's state, the copy of its configuration, its logger and sink and every sentence composed for it, the self id, the registry, every connection, the tokens and frames a connection keeps, its drain order, the presence frames, the peer connections and data channels the rooms' negotiations make and the frames the backend receives on them. A room's resource receives the room itself, its peers, bindings and their drain waiters, actions, queues, frames, transfers, negotiations' records, rebinding counts and backoffs, sealed texts, relays, handlers, the objects its backend makes for it, the context libsecp256k1 signs with (`secp256k1_context_preallocated_create`, in the resource), and the JSON it parses and writes. `app_config::memory` defaults to `std::pmr::get_default_resource()`; a null one is a programming error that `BOOST_ASSERT_MSG(memory != nullptr, "app_config::memory names no resource")` catches when the app is made. A message an action's handler receives as a `std::pmr::string`, a `trystero::pmr_bytes` or a `boost::json::value` is in the room's resource. Received frames start in the app's resource; binary payloads move when the message buffer and room resources match and copy once otherwise, while chunked transfers assemble in the room's resource. Received std::pmr::string text always copies once. The library touches the resources only from the app's executor, so they need not be thread-safe when that executor runs on one thread. `app_config` is move-only (its `log` is), so it is passed moved or built in place with designated initializers.

#### Scenario: A room is configured in a resource
- **WHEN** `allocation_test` builds `trystero::app_config{.app_id = std::pmr::string("allocation", &memory), .self_id = ..., .password = std::pmr::string("a password", &memory), .relay_urls = ..., .log = {std::allocator_arg, std::pmr::polymorphic_allocator<>(&memory), sink}, .memory = &memory}` and joins a room with `options_in`
- **THEN** the app and the room construct, and every allocation up to their destruction is counted by `memory`

### Requirement: Memory: The resource's lifetime
The resource SHALL outlive the room, every value the room handed out (a `std::pmr::string`, a `pmr_bytes` or a `boost::json::value` a handler kept) and every operation the room left pending, because destroying a room closes everything but what it closed still has operations queued that free into the resource. On Asio the resource lives until the `io_context` of the room's executor has run out of the room's work or has been destroyed, so it is declared before the `io_context`, which is then destroyed first: the resource, then the `io_context`, then the room, in that order of declaration. In the browser the resource lives until the room's pending promises have settled, in practice for the page's life. No check catches the misuse, since trystero holds a raw pointer and cannot see the resource end; a resource that asserts nothing is out when it is destroyed is the check, and it is the user's. A pool such as `std::pmr::unsynchronized_pool_resource` suits a long-lived room, since a room allocates and frees on every event; a `std::pmr::monotonic_buffer_resource` grows with every event and suits a short-lived one, or a test.

#### Scenario: Operations outlive the rooms
- **WHEN** `a_room_allocates_through_its_resource_from_join_to_destruction` destroys both rooms
- **THEN** the sum of `a_memory.outstanding` and `z_memory.outstanding` is greater than 0 at that point, and is 0 after the `io_context` has run 300 ms more

### Requirement: Memory: Containers and shared objects
Every container the library declares SHALL be a `std::pmr` one over the app's or the room's resource: `std::pmr::string` (and `peer_id`), `std::pmr::vector` (and `backend::bytes`, `protocol::bytes` and `pmr_bytes`), `std::pmr::deque`, `std::pmr::map`, `std::pmr::set`, `std::pmr::unordered_map` and `std::pmr::unordered_set`; `app_config`, `room_options`, `send_options`, `message_info` and `backend::ice_server` hold them too. Two types stay the user's choice of `std::allocator`: `trystero::bytes`, a `std::vector<std::uint8_t>`, and `trystero::payload`, a `std::variant<std::string, boost::json::value, bytes>`; a received `bytes`, or the binary of a `payload`, is a copy of the frame, which is in the room's resource. A struct that holds `std::pmr` containers MUST take the room's `std::pmr::polymorphic_allocator<>` in its constructor, and a container of such structs is given it when it makes one (`try_emplace(key, allocator)`). A `std::pmr` container is never copied without an allocator, since its copy constructor takes the default resource; moving keeps the resource. Shared objects are made with `std::allocate_shared` over `std::pmr::polymorphic_allocator<>`, never with `std::make_shared`, `std::make_unique` or `new`, and a timer is a `std::shared_ptr` for that reason.

#### Scenario: A received pmr_bytes is in the receiver's resource
- **WHEN** a room receives a payload into a `trystero::pmr_bytes` handler
- **THEN** the value's allocator resource is the receiving room's resource (`room_options::memory`, the app's when null), the buffer its transfer assembled moved out without a copy, or a copy of its bytes when it arrived inline in one frame

### Requirement: Memory: handler is a move-only callable that allocates through a resource
The library SHALL define `template <class R, class... Args> class handler<R(Args...)>` in `<webcpp/trystero/handler.hpp>`, a move-only callable of the signature `R(Args...)` that replaces `std::function` wherever trystero keeps a handler (the backend's, the room's and the log sink), since `std::function` takes no allocator. It holds its target inline when the target fits in `3 * sizeof(void*)`, has alignment at most `alignof(void*)` and is nothrow-move-constructible, and otherwise in storage allocated through its `allocator_type` (`std::pmr::polymorphic_allocator<>`). It is constructible empty, from `nullptr`, from a callable (default resource), from `(std::allocator_arg, allocator, f)` and by the allocator-extended move `(std::allocator_arg, allocator, handler&&)`, which moves the target into another resource when the two differ. It MUST NOT be copied. `explicit operator bool` says whether it has a target, `operator()` calls it (asserting with `BOOST_ASSERT` that it is not empty), `get_allocator()` returns its allocator, and a move assignment carries the source's allocator with its target. An app moves a handler it is given, `app_config::log`, into its own resource with the allocator-extended move.

#### Scenario: A small closure is held inline
- **WHEN** a `handler<void()>` is constructed with `std::allocator_arg`, an allocator over a counting resource and a lambda capturing two pointers
- **THEN** the counting resource sees no allocation

#### Scenario: A large closure is allocated in the resource
- **WHEN** the lambda captures 64 bytes
- **THEN** the counting resource sees one allocation, which is given back when the handler is destroyed or reset

#### Scenario: An empty handler is not callable
- **WHEN** a handler is default-constructed
- **THEN** `static_cast<bool>(h)` is false

### Requirement: Memory: json_resource
The library SHALL define `class json_resource final : public boost::container::pmr::memory_resource` in `<webcpp/trystero/json_resource.hpp>`, an adapter from a `std::pmr::memory_resource` to Boost.JSON's resource, with `explicit json_resource(std::pmr::memory_resource* upstream) noexcept`, `[[nodiscard]] std::pmr::memory_resource* upstream() const noexcept`, `do_allocate` and `do_deallocate` forwarding to the upstream's `allocate` and `deallocate` with the same size and alignment, and `do_is_equal` returning true for itself alone (telling another adapter over the same upstream apart from another resource would need RTTI, which a user may build without). It exists because Boost.JSON's `storage_ptr` holds a `boost::container::pmr::memory_resource`, whose base class is header-only while `boost::container::pmr`'s global resources are compiled, and the CI installs Boost's headers alone.

#### Scenario: Allocation reaches the upstream
- **WHEN** a `boost::json::value` is built with `boost::json::make_shared_resource<trystero::json_resource>(&counting)` and given a string of 33 bytes
- **THEN** the counting resource sees the allocation (`exchange_allocates_through_the_rooms_resources<boost::json::value>` builds its payload this way)

#### Scenario: A resource equals itself alone
- **WHEN** two `json_resource` objects over the same upstream are compared with `is_equal`
- **THEN** the result is false, and `is_equal` of a resource with itself is true

### Requirement: Memory: Boost.JSON allocates through the room's resource
The room SHALL make its JSON storage once, when it is constructed, as `boost::json::make_shared_resource<json_resource>(memory)`, since a value a handler keeps may outlive the room; the storage is counted, so a value a handler keeps keeps it alive. Text MUST be parsed by a `boost::json::parser` over that storage, never by `boost::json::parse`, whose temporaries beyond a stack buffer come from the default resource, and JSON is written by a `boost::json::serializer` whose stack is the resource (`protocol::json::append_string` and `append_value` build a `json_resource` over the destination string's allocator resource). The `boost::json::value` an action's handler receives is in the room's JSON storage, which the message carries. Boost.JSON allocates the storage's reference count with `operator new`, once per room.

#### Scenario: A JSON payload is allocated in the room's resource
- **WHEN** `exchange_allocates_through_the_rooms_resources<boost::json::value>` sends a `boost::json::value` of a 33-byte string between two rooms
- **THEN** the global `operator new` count and the C library count are both 0 during the exchange, and the counting resources see allocations

### Requirement: Memory: Asio posts and completions carry the room's allocator
Every `boost::asio::post`, `dispatch`, timer wait and `any_completion_handler` the room makes SHALL carry `boost::asio::bind_allocator` over `std::pmr::polymorphic_allocator<>(memory)`. `detail::post_handler(ex, handler)` posts through the `io_context::executor_type` directly when the room's executor is one (`ex.target<boost::asio::io_context::executor_type>()`), where Asio allocates the operation as the handler's allocator says, because a post through `boost::asio::any_io_executor` is wrapped by Asio in a function object allocated with `std::allocator` (measured: two `std::aligned_alloc` per post, in every round of five posts, natively and on emscripten); on another executor, as the browser's `event_loop` executor, the wrapper stays Asio's. `detail::post(ex, memory, f)` posts `f` allocated in `memory`. A user's completion handler is allocated as its own associated allocator says; when the room erases its type (an `any_completion_handler`) it records whether its associated executor is the room's, and such a completion runs from an operation the room posts, allocated with the handler's allocator, while a handler bound to another executor is handed to that executor by Asio, whose type-erased executor wraps it with `std::allocator`.

#### Scenario: The user binds the room's resource to a completion
- **WHEN** `allocation_test` calls `async_send(pmr_bytes(100'000, 7, &a_memory), boost::asio::bind_allocator(std::pmr::polymorphic_allocator<>(&a_memory), [](boost::system::error_code){}))` and `async_ping(z.self_id(), boost::asio::bind_allocator(...))`
- **THEN** the transfer and the ping complete (`files == 1 && pings == 1`) and their completions are allocated in `a_memory`

### Requirement: Memory: The protocol takes the resource
Every function of `protocol/` that returns what it allocates SHALL take a `std::pmr::memory_resource*` as its last parameter, with `std::pmr::get_default_resource()` as its default, and return `std::pmr` types; `protocol::nostr::verify` uses `secp256k1_context_static` and allocates nothing. The payload codecs follow: `detail::payload_traits` has `std::pmr::string` and `std::pmr::vector<std::uint8_t>` (`pmr_bytes`) beside the standard types, `encode(value, memory)` takes the resource for what keeps the payload alive, and `decode(incoming& message, memory)` takes the resource for every type, building the `std::pmr` types in it and a `boost::json::value` in the room's JSON storage; `std::string`, `bytes` and `payload` stay with `std::allocator`.

#### Scenario: A pmr string payload is delivered in the resource
- **WHEN** `payload_traits<std::pmr::string>::decode` is given a text message and the room's resource
- **THEN** it returns a `std::pmr::string` constructed with that resource

### Requirement: Memory: What stays outside the resource
The library MUST NOT claim the following allocations, which stay outside the room's resource: libdatachannel's and OpenSSL's own allocations, with the `std::string` and `rtc::Description` libdatachannel's API takes and returns; what `emscripten::val` holds, in the JavaScript heap; the counter of the room's JSON storage, which Boost.JSON allocates with `operator new` once per room; Asio's own once-per-context allocations (its services and the scheduler's first operation); the wrapper Asio puts around a post through an executor that is not the executor of an `io_context`; the native backend's posts from libdatachannel's threads, which carry Asio's default allocator, since the resource is touched only on the room's executor; Boost.Beast's and Asio's state inside the native WebSocket (its stream, its resolver's results, its coroutines' frames); the payload types `std::string`, `bytes` and `payload`, with `std::allocator`, the user's choice; and, on Emscripten, the C library's `select()`, which Asio's reactor calls on every wait of an `io_context` with a timer or a socket and which callocs an array on every call.

#### Scenario: The counter is the one operator new
- **WHEN** `a_room_allocates_through_its_resource_from_join_to_destruction` runs two rooms from construction to the drain
- **THEN** the replaced global `operator new` is called exactly twice (`BOOST_TEST_EQ(global_allocations, 2U)`), once for each room's JSON storage counter, and the C library's allocation functions never (`c_allocations == 0`)

### Requirement: Memory: The allocation test counts every allocation of two rooms
`test/allocation_test.cpp` SHALL be the proof of the allocation guarantee, run on both targets: it replaces the global `operator new` and `operator delete` (all four forms, raising an alignment below `sizeof(void*)` and rounding the size to a multiple of the alignment, since `std::aligned_alloc` refuses both, throwing `std::bad_alloc` where there are exceptions and using `_aligned_malloc` on MSVC) and counts the C library's allocation functions where the platform lets a program see them: glibc's functions are replaced and forward to `__libc_*`, Emscripten's dlmalloc has weak definitions the test replaces and forwards to `emscripten_builtin_*`, and macOS reports each allocation to `malloc_logger`; MSVC's are not counted, nor is anything under AddressSanitizer. Only the thread `main` runs on is counted. Its counting resource holds a 64 MiB buffer (`std::pmr::monotonic_buffer_resource` over a `std::vector<std::byte>` with `std::pmr::null_memory_resource()` upstream), counts `allocations`, `outstanding` bytes and `foreign` returns, which name memory the resource never gave (a pmr object freed through the wrong resource), and `foreign` MUST be 0. On Emscripten the test also replaces `select()` with one that keeps its `pollfd` array on the stack, and the Jamfile adds `-sALLOW_MEMORY_GROWTH`, since two 64 MiB buffers exceed Emscripten's 16 MiB. Before counting, Asio's once-per-context allocations are made (`warm`: 256 timers waited and cancelled, a post, 20 ms of running); without them the count is 8, not 2.

#### Scenario: The counts can see an allocation
- **WHEN** `the_counts_see_an_allocation` allocates 40 bytes through `operator new` while counting and, where the C library is counted, through `std::malloc`
- **THEN** `global_allocations` is 1 and `c_allocations` is 1, so a zero below means something

#### Scenario: The replaced operators serve every alignment
- **WHEN** `the_replaced_operators_serve_every_alignment` allocates sizes 1, 24 and 100 with alignments 1, 2, 4, 8, 16, 32 and 64 through `operator new` and `std::pmr::new_delete_resource()`
- **THEN** each pointer is a multiple of its alignment, and `operator new(0)` is non-null

#### Scenario: A foreign return is counted
- **WHEN** memory given by one counting resource is deallocated through another
- **THEN** the other's `foreign` is 1, and the giver's `outstanding` is 48 before and 0 after its own deallocation of 48 bytes

### Requirement: Memory: A payload exchange allocates nothing outside the resources
`exchange_allocates_through_the_rooms_resources` SHALL run two rooms (`AAAAAAAAAAAAAAAAAAAA` with `a_memory` and `zzzzzzzzzzzzzzzzzzzz` with `z_memory`) over a `memory_network` relay `wss://relay.test` (app id `allocation`, room `room`), join them, send one payload through before the window opens (a payload sent before the peers meet reaches nobody), then count a second send: during the exchange the global `operator new` count MUST be 0, the C library count 0, the counting resources' allocations greater than before, and their `foreign` 0. It does so for three payload types in turn: a `std::pmr::string` of `"a text that no small string holds"`, a `std::pmr::vector<std::uint8_t>` of 4096 bytes of value 7, and a `boost::json::value` of that same string in a `json_resource` storage.

#### Scenario: A pmr string payload
- **WHEN** a second `std::pmr::string` payload is sent and has arrived (`received == 2`) within 5 s
- **THEN** `global_allocations` and `c_allocations` are 0, `a_memory.allocations + z_memory.allocations` increased, and `foreign` is 0

#### Scenario: A pmr_bytes payload
- **WHEN** the same exchange is run with a `std::pmr::vector<std::uint8_t>(4096, 7, memory)` payload
- **THEN** the same verdicts hold

#### Scenario: A JSON payload
- **WHEN** the same exchange is run with a `boost::json::value` payload in the shared JSON storage
- **THEN** the same verdicts hold

### Requirement: Memory: A room's whole life allocates through its resource
`a_room_allocates_through_its_resource_from_join_to_destruction` SHALL make two apps, each configured with password `a password` and joining one room, a log sink and a relay of their own (`z` first, 100 ms ahead, then `A`), join them, exchange a 100,000-byte `pmr_bytes` payload of 7s that travels in chunks, measure a round trip with `async_ping`, have `z` say goodbye with `async_leave`, destroy both and drain the `io_context`, with counting from the first construction to the drain. Then `joined`, `exchanged` (`files == 1 && pings == 1`) and `farewell` MUST be true, both log sinks were called (`a_lines`, `z_lines` greater than 0), the network's own resource saw allocations, `global_allocations` is exactly 2 and `c_allocations` is 0, `a_memory.outstanding` and `z_memory.outstanding` are 0 after the drain, and `foreign` is 0 for `a_memory`, `z_memory` and `network_memory`. Each configuration is a temporary (`app_config_in(...)` and `options_in(...)`), because MSVC's debug standard library keeps in a moved-from `std::pmr` container 16 bytes of its resource, which the test would count as the room's.

#### Scenario: Two operator new calls in a room's whole life
- **WHEN** the test finishes
- **THEN** `BOOST_TEST_EQ(global_allocations, 2U)` and `BOOST_TEST_EQ(c_allocations, 0U)` hold

#### Scenario: Nothing is out after the drain
- **WHEN** both rooms are destroyed and the `io_context` has run 300 ms more
- **THEN** both resources have 0 bytes outstanding, although they had more than 0 just after the rooms were destroyed

### Requirement: Memory: A test helper ends the program when a check cannot be gone on from
The test tree SHALL provide `webcpp::test::require(bool held)` in `test/require.hpp`, for a check a helper's caller cannot go on without: when `held` is false it MUST end the program with `std::exit(boost::report_errors())`, with the errors counted so far, since a helper cannot return from its caller and a `lightweight_test` program may be built without exceptions. A test case that fails such a check itself returns (`if (!BOOST_TEST(...)) { return; }`); `BOOST_TEST` has already reported the failure.

#### Scenario: A failed require ends the program
- **WHEN** `webcpp::test::require(false)` is called after one failed `BOOST_TEST`
- **THEN** the program exits with the status `boost::report_errors()` returns, 1

#### Scenario: A held check goes on
- **WHEN** `webcpp::test::require(true)` is called
- **THEN** it returns and the test goes on

### Requirement: Protocol: Namespace, headers and layering
The protocol layer SHALL be the pure functions and types of the headers `relays.hpp`, `json.hpp`, `envelope.hpp`, `topics.hpp`, `text.hpp`, `signal.hpp`, `nostr.hpp` and `action_wire.hpp` under `include/webcpp/trystero/protocol/`, in namespace `webcpp::trystero::protocol` (the JSON helpers in `webcpp::trystero::protocol::json`, the Nostr messages in `webcpp::trystero::protocol::nostr`, internals in `detail`). It MUST do no I/O and know no platform: it includes neither backend, libdatachannel, OpenSSL nor Emscripten, and depends on Boost (Hash2, JSON, Container's `pmr` base class), libsecp256k1 (`nostr.hpp` only) and the standard library. Each header includes `<webcpp/trystero/config.hpp>` first and compiles alone on every target.

#### Scenario: The protocol headers compile alone
- **WHEN** each of `trystero/protocol/*.hpp` is compiled as the only include of a translation unit, natively and on emscripten (`webcpp.headers-alone trystero : ../include : trystero.hpp trystero/*.hpp trystero/protocol/*.hpp trystero/detail/*.hpp`)
- **THEN** every one compiles

#### Scenario: The protocol layer includes no backend
- **WHEN** the includes of `protocol/*.hpp` are read
- **THEN** none names `browser/`, `native/`, libdatachannel, OpenSSL or Emscripten

### Requirement: Protocol: Allocation, exceptions and determinism
Every function of the protocol layer that returns what it allocates SHALL take a `std::pmr::memory_resource*` as its last parameter, defaulting to `std::pmr::get_default_resource()`, and MUST return `std::pmr` types (`std::pmr::string`, `protocol::bytes` which is `std::pmr::vector<std::uint8_t>`, `std::pmr::vector<std::string_view>`). The layer MUST raise no exception of its own: a malformed input is answered with `std::optional`, an empty result or an error value, never a throw. Its functions are deterministic: the same inputs give the same bytes, and the sources of randomness (the generator of `make_id`, the 32 auxiliary bytes of a signature, the nonce of a signal) are parameters, never read inside. `nostr::verify` allocates nothing, since it uses `secp256k1_context_static`.

#### Scenario: A result is allocated in the resource given
- **WHEN** `protocol::room_token("my-app", "room-1", &resource)` is called with a `std::pmr::monotonic_buffer_resource` named `resource`
- **THEN** the returned `std::pmr::string` is allocated in `resource`

#### Scenario: Randomness is a parameter
- **WHEN** `make_id(20, generator)` is called twice with two `std::mt19937` generators seeded with 42
- **THEN** the two ids are equal

### Requirement: Protocol: Id alphabet and lengths
The library SHALL define `id_alphabet`, the 62 characters `"0123456789AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"` Trystero draws ids from (its `charSet`), and the lengths `peer_id_size` = 20 (Trystero's `selfId`), `offer_id_size` = 12 and `nonce_size` = 8 (the nonce that makes repeated announcements distinct Nostr events), all in `topics.hpp`.

#### Scenario: The constants
- **WHEN** `id_alphabet.size()`, `peer_id_size`, `offer_id_size` and `nonce_size` are read
- **THEN** they are 62, 20, 12 and 8

### Requirement: Protocol: Random ids
`make_id<Generator>(std::size_t n, Generator& generator, std::pmr::memory_resource* memory)` SHALL return a `std::pmr::string` of `n` characters, each drawn from `id_alphabet` by a `std::uniform_int_distribution<std::size_t>` over the generator given (Trystero's `genId`), for any uniform random bit generator.

#### Scenario: The form of an id
- **WHEN** `make_id(peer_id_size, generator)` is called with `std::mt19937 generator(42)` (`protocol_test`, `signals`)
- **THEN** the id has 20 characters and `id.find_first_not_of(id_alphabet)` is `std::string::npos`

### Requirement: Protocol: SHA-256 of parts
`sha256_of(std::initializer_list<std::string_view> parts)` SHALL return the SHA-256 (Boost.Hash2 `sha2_256`) of the concatenation of the parts as a `std::array<std::uint8_t, 32>`, feeding the hash one part at a time without concatenating them, and `sha256(text)` MUST equal `sha256_of({text})`. Every digest of the protocol (the room token, the AES key, the handshake proof) is computed this way.

#### Scenario: Parts hash as their concatenation
- **WHEN** `to_hex(sha256_of({"secret", ":", "my-app", ":", "room-1"}))` is computed
- **THEN** it is `a11095bfc9cdd863704127af9c923044169c2784ed35e2ca808601f62878e886`, the SHA-256 of `secret:my-app:room-1` that Trystero's `hashWith` writes (`protocol_test`, `handshake_and_sealing`)

### Requirement: Protocol: Topic names
`sha1_topic_of(std::initializer_list<std::string_view> parts, memory)` SHALL return Trystero's `sha1` text: the SHA-1 (Boost.Hash2 `sha1_160`) of the concatenation of the parts, each of its 20 digest bytes written in base 36 with the digits `0123456789abcdefghijklmnopqrstuvwxyz`, without padding, the pieces concatenated (so the text is not fixed width, and is only ever compared). `sha1_topic(text, memory)` MUST equal `sha1_topic_of({text}, memory)`.

#### Scenario: A topic is the base 36 form of a SHA-1
- **WHEN** `root_topic("trystero-cpp-probe", "lobby")` is computed
- **THEN** it is `2t3c266f57311q4o1j4w3j6o611y1m1u4d6st6b`, the value Trystero JS 0.26 writes

### Requirement: Protocol: Root and peer topics
`root_topic(app_id, room_id, memory)` SHALL be `sha1_topic_of({"Trystero@", app_id, "@", room_id})`, the topic every member of a room announces on, and `peer_topic(app_id, room_id, peer_id, memory)` SHALL be `sha1_topic_of({"Trystero@", app_id, "@", room_id, "@", peer_id})`, where a peer receives its offers, answers and candidates. The prefix `Trystero@` is on the wire and never changes.

#### Scenario: Topics of the fixtures
- **WHEN** `root_topic` and `peer_topic` are computed for ("trystero-cpp-probe", "lobby", "00000000000000000000"), for ("my-app", "room-1", "zzzzzzzzzzzzzzzzzzzz"), for ("café ☕ app", "sala ünïcode 🎉", "AbCdEfGhIjKlMnOpQrSt") and for ("a", "b", "c")
- **THEN** each equals the `root_topic` and `self_topic` the oracle wrote, for instance `2t3c266f57311q4o1j4w3j6o611y1m1u4d6st6b` and `2i443c4a4z2u3a2k3o3qg4b454i1r2x4d283624` for the first, `731p275e6u4l455l3849481dl46536q3g1r6h5y` and `4i5p4v3i2d3s5p6t196qp3o3yf4h1v33z605s` for the second

### Requirement: Protocol: Room token
`room_token(app_id, room_id, memory)` SHALL return the lowercase hexadecimal SHA-256 of `"Trystero:" + app_id + ":" + room_id`, 64 digits, the token that names a room inside a data channel's envelope (Trystero's `deriveRoomNamespace`). The prefix `Trystero:` is on the wire and never changes.

#### Scenario: The token of my-app/room-1
- **WHEN** `room_token("my-app", "room-1")` is computed
- **THEN** it is `2e4f09c7740641f4d3233ddd1ee0cd54aba1e7fdc69e8b3e2ddfcded88bacd3d`

#### Scenario: A non-ASCII token
- **WHEN** `room_token("café ☕ app", "sala ünïcode 🎉")` is computed
- **THEN** it is `fc05f72d7a821209b4f5cbb492b5857a069978603eb8bf3c98f71c53161c90da`

### Requirement: Protocol: Text as UTF-16 code units
`utf16_code_unit_sum(std::string_view utf8)` SHALL return the sum of the UTF-16 code units of a UTF-8 text, as JavaScript's `charCodeAt` sees them, a code point above U+FFFF counting as its two surrogates (0xd800 + ((p - 0x10000) >> 10) and 0xdc00 + ((p - 0x10000) & 0x3ff)); `str_to_num(text, limit = 9007199254740991.0)` (Trystero's `strToNum`) SHALL be `std::fmod` of that sum by `limit`, in [0, limit). Non-ASCII ids count as in JavaScript, since the Nostr kind and the relay shuffle seed derive from the sum.

#### Scenario: The fixtures of strToNum
- **WHEN** `str_to_num` is called without a limit and with 10000 on "", "a", "trystero-cpp-probe", "café ☕", "🎉🎉" and "585q4w3ia403j31626l2p3k1p3kh6u2v2g5n4v"
- **THEN** the results are (0, 0), (97, 97), (1857, 1857), (10312, 312), (225162, 5162) and (2890, 2890)

### Requirement: Protocol: Default Nostr relays
`default_nostr_relays` SHALL be a `std::array<std::string_view, 29>` holding Trystero 0.26's `defaultRelayUrls` in its order, from `wss://0x-nostr-relay.fly.dev`, `wss://bendernostur.duckdns.org:8443`, `wss://cdn.satellite.earth`, `wss://nos.lol` and `wss://nostr.chaima.info` to `wss://relay.routstr.com` and `wss://testr.nymble.world`; the order is part of the relay derivation and MUST NOT change.

#### Scenario: The list equals Trystero's
- **WHEN** `default_nostr_relays` is compared with the `default_relays` of `expected.json`, which the oracle takes from the `defaultRelayUrls` Trystero's Nostr package exports
- **THEN** the two sequences are equal, with 29 entries

### Requirement: Protocol: Seeded shuffle
`shuffle<Sequence>(Sequence items, double seed)` SHALL return a copy of the items shuffled as Trystero's `shuffle` does: a Fisher-Yates from the end, where each random number is `x - floor(x)` with `x = sin(seed++) * 10000`, so that every peer of an app picks the same order. It relies on `std::sin` and V8's `Math.sin` agreeing on these inputs, which the oracle's fixtures check.

#### Scenario: Seeds 0 and 1 over 29 items
- **WHEN** `shuffle` is called on the indexes 0 to 28 with seed 0, then with seed 1
- **THEN** the orders are `11, 2, 6, 25, 15, 9, 21, 8, 16, 7, 23, 28, 13, 20, 17, 10, 4, 1, 14, 3, 12, 22, 27, 18, 24, 5, 26, 19, 0` and `6, 21, 7, 9, 2, 16, 10, 23, 26, 17, 8, 22, 0, 14, 13, 24, 11, 4, 1, 15, 3, 12, 19, 28, 18, 25, 5, 27, 20`, and the orders of seeds 1234567 and 9007199254740991 of the fixtures too

### Requirement: Protocol: Relays derived from the app id
`derive_relays(app_id, defaults, redundancy, memory)` SHALL return, as a `std::pmr::vector<std::string_view>` of views into `defaults`, the defaults shuffled with the seed `str_to_num(app_id)` and cut to the first `redundancy` entries (Trystero's `getRelays`); with fewer defaults than `redundancy` it returns all of them. A room that names no relays uses `derive_relays(app_id, default_nostr_relays, 5)`.

#### Scenario: The relays of two apps
- **WHEN** `derive_relays("my-app", default_nostr_relays, 5)` and `derive_relays("trystero-cpp-probe", default_nostr_relays, 5)` are computed
- **THEN** the first is `wss://nostr.red5d.dev`, `wss://relay.nostrmap.net`, `wss://relay.piazza.today`, `wss://relay.bitmacro.cloud`, `wss://relay.nostr.dev.br`, and the second is `wss://testr.nymble.world`, `wss://nostr.stakey.net`, `wss://relay.grigic.org`, `wss://relay.nostr.dev.br`, `wss://bendernostur.duckdns.org:8443`

### Requirement: Protocol: Bounds on a relay's texts
`relays.hpp` SHALL define `max_relay_message_bytes` = 1 MiB (1,048,576), the longest text a room reads from a relay, and `max_queued_relay_bytes` = 4 MiB (4,194,304), the most text a room holds unsent for one relay. Both backends close the connection of a relay that sends a longer text (before copying it) or lets more than the queue bound pile up, and the room reconnects as it does to a relay that went away.

#### Scenario: The two bounds
- **WHEN** `max_relay_message_bytes` and `max_queued_relay_bytes` are read
- **THEN** they are `std::size_t{1} << 20` and `std::size_t{4} << 20`

### Requirement: Protocol: Hexadecimal text
`to_hex(data, memory)` and `append_hex(out, data)` SHALL write two lowercase hexadecimal digits per byte (Trystero's `toHex`), and `from_hex(text, memory)` SHALL return the bytes of a hexadecimal text of either case, or `std::nullopt` when its length is odd or a character is not a hexadecimal digit.

#### Scenario: Round trip and refusals
- **WHEN** `to_hex` is applied to the bytes `0, 1, 2, 253, 254, 255` and `from_hex` to `"0001fdfeff"`, `"abc"` and `"zz"`
- **THEN** the text is `000102fdfeff`, the bytes of the second are `0, 1, 253, 254, 255`, and the last two give `std::nullopt`

### Requirement: Protocol: Base64 text
`base64_encode(in, memory)` and `append_base64(out, in)` SHALL write standard base64 with the alphabet `A-Za-z0-9+/` and `=` padding, as `btoa` does over binary text; `base64_decode(in, memory)` SHALL read it padded or not, as `atob` does, and return `std::nullopt` for a character outside the alphabet or a length that is 1 modulo 4 once the padding is removed.

#### Scenario: The RFC 4648 vectors
- **WHEN** `base64_encode` is applied to the bytes of "", "f", "fo" and "foo" (`protocol_test`, `text`)
- **THEN** the results are `""`, `Zg==`, `Zm8=` and `Zm9v`

#### Scenario: A decode and a refusal
- **WHEN** `base64_decode("Zm8=")` and `base64_decode("Z!==")` are called
- **THEN** the first returns the bytes `'f', 'o'` and the second returns `std::nullopt`

### Requirement: Protocol: JSON writing helpers
`protocol::json` SHALL offer `append_string(out, text)` (a JSON string literal with its quotes, escaped by Boost.JSON's serializer so that the bytes equal what `JSON.stringify` writes), `append_value(out, value)` (a `boost::json::value` serialized as `boost::json::serialize` writes it), `append_integer(out, std::int64_t)` (decimal, as JavaScript writes a safe integer) and `append_key(out, key)` (`"key":`, with a leading comma unless `out` ends with `{`), all appending to a `std::pmr::string` and allocating the serializer's stack in that string's resource. A text the protocol writes is a fixed shape of a few strings and numbers and is appended field by field, never built as a `boost::json::value`.

#### Scenario: Escaping as JSON.stringify
- **WHEN** `write_signal({.peer_id = "q\"\\\n"})` is called
- **THEN** it returns `{"peerId":"q\"\\\n"}` (quote, backslash and newline escaped)

#### Scenario: A control character in a Nostr event's content
- **WHEN** the event id of the content `quote " backslash \ newline <LF> tab <TAB> unicode é ☕ 🎉 control U+0001` is computed
- **THEN** it equals the id Trystero JS computes over the same content, `JSON.stringify` having escaped it the same way

### Requirement: Protocol: JSON parsing in the caller's storage
`protocol::json::parse(text, storage, ec)` SHALL parse one JSON value with a `boost::json::parser` constructed over `storage`, so that the value and the parser's temporaries are allocated in that storage and not in the default resource as `boost::json::parse` would for text beyond its stack buffer; it sets `ec` and returns a null value of `storage` when the text is not one JSON value.

#### Scenario: Malformed text
- **WHEN** `parse("[1,", storage, ec)` is called
- **THEN** `ec` is set and the value is null

### Requirement: Protocol: Boost.JSON over a std::pmr resource
`webcpp::trystero::json_resource` (`json_resource.hpp`) SHALL be a `final` `boost::container::pmr::memory_resource` constructed from a `std::pmr::memory_resource*` upstream (`noexcept`), whose `do_allocate` and `do_deallocate` forward to it, whose `upstream()` returns it, and whose `do_is_equal(other)` is true only for itself (telling another adapter apart would need RTTI, which a user may build without). It needs only the header-only base class, because `boost::container::pmr`'s global resources are compiled. A room makes one over its resource (`room_options::memory`, the app's when null) as the storage of every JSON value of the room.

#### Scenario: Allocation reaches the upstream
- **WHEN** a `json_resource` over a counting `std::pmr::memory_resource` allocates and deallocates through a `boost::json::storage_ptr`
- **THEN** the counting resource sees both calls with the same size and alignment

#### Scenario: Equality is identity
- **WHEN** two `json_resource` objects over the same upstream are compared with `is_equal`
- **THEN** they are not equal, and each is equal to itself

### Requirement: Protocol: The seal key and the handshake proof
`seal_key(password, app_id, room_id)` SHALL return the 32-byte AES-256 key Trystero's `genKey` imports: the SHA-256 of `<password>:<app>:<room>`, the password empty when there is none; `handshake_proof(challenge, password, app_id, room_id, memory)` SHALL return Trystero's `hashChallenge`: the lowercase hexadecimal SHA-256 of `<challenge>:<password>:<app>:<room>`, 64 digits.

#### Scenario: The key of a room
- **WHEN** `to_hex(seal_key("secret", "my-app", "room-1"))` is computed
- **THEN** it is `a11095bfc9cdd863704127af9c923044169c2784ed35e2ca808601f62878e886`

#### Scenario: Two proofs
- **WHEN** `handshake_proof` is computed for ("ch4ll3ng3", "secret", "my-app", "room-1") and for ("x", "", "a", "b")
- **THEN** the proofs are `3eb149e6ff08bcbab230d022e4ff330dbfa7df9fcac6c378695fee988587e579` and `fccc86c33cc191182c6e4a032153814c58a4864256ca5bf60a024e013c40f8c8`

### Requirement: Protocol: The text form of a sealed signal
`format_sealed(iv, ciphertext, memory)` SHALL write what Trystero's `encrypt` returns: the IV's bytes in decimal joined by commas, `$`, and the ciphertext (with its 16-byte tag) in padded base64; `parse_sealed(text, memory)` SHALL return a `sealed_parts{iv, ciphertext}` and `std::nullopt` when the text has no `$`, an IV item that is not a decimal number from 0 to 255 (an empty item included), or a ciphertext that is not base64. The IV is `seal_iv_size` = 16 bytes, not the usual 12, as Trystero draws it.

#### Scenario: The sealed text of the fixtures round trips
- **WHEN** `parse_sealed` reads the oracle's sealed text `3,20,37,54,71,88,105,122,139,156,173,190,207,224,241,2$ZtkZKcLsGfFd/0pp36cUJzLpIzV1VmUSSVJx8q0QBlJDrnUySIas/ifQDiq8+UqNj6dm1CMiqIxicYa1+fdkfVE=` and `format_sealed` writes the parts back
- **THEN** the IV has 16 bytes, the ciphertext has the plaintext's size plus 16 bytes, and the text written equals the text read

#### Scenario: Refused texts
- **WHEN** `parse_sealed("1,2,300$AAAA")` and `parse_sealed("no dollar")` are called
- **THEN** both return `std::nullopt`

### Requirement: Protocol: Sealed size bounds
`signal.hpp` SHALL define `sealed_size(plaintext)`, a `constexpr` function giving the longest sealed text of a plaintext, `(16 * 4) - 1 + 1 + (((plaintext + 16 + 2) / 3) * 4)` bytes (the IV in decimal and commas, `$`, the base64 of the ciphertext and its tag), and the bounds `max_peer_id_bytes` = 64, `max_offer_id_bytes` = 64, `max_sealed_description_bytes` = 64 KiB and `max_sealed_candidate_bytes` = 16 KiB. A static assertion MUST hold that `sealed_size(8192) <= max_sealed_candidate_bytes`: Trystero drops a candidate whose plaintext is longer than 8192 bytes, and every candidate up to that size, sealed, fits. The bounds refuse nothing a browser writes for a data channel; Trystero JS bounds none of them.

#### Scenario: The longest candidate Trystero accepts
- **WHEN** `format_sealed` seals a ciphertext of 8192 + 16 bytes under an IV of sixteen bytes of 255
- **THEN** the text has `sealed_size(8192)` = 11008 bytes and `read_signal` accepts a signal carrying it as its candidate

### Requirement: Protocol: Reading a signal
`signal_message` SHALL hold views `peer_id`, `offer_id`, `offer`, `answer` and `candidate` and a `bool passive`, with `is_announcement()` true when none of offer, answer and candidate is present; `read_signal(const boost::json::value& document)` SHALL return the message as views into the document, or `std::nullopt` when Trystero would ignore it: the document is not an object, the peer id is missing or empty, an `offer`, `answer` or `candidate` is present but not a non-empty string, or a field exceeds its bound. A `passive` that is not a boolean is read as false.

#### Scenario: An announcement and refused documents
- **WHEN** `read_signal` reads `{"peerId":"abc","nonce":"x1"}`, `{"peerId":"abc","offer":""}`, `{"offer":"x"}` and `["peerId"]`
- **THEN** the first is an announcement (`is_announcement()` is true) and the other three give `std::nullopt`

### Requirement: Protocol: Signal field bounds are exact
`read_signal` SHALL accept a candidate of exactly `max_sealed_candidate_bytes` and refuse one byte more, accept an offer or an answer of exactly `max_sealed_description_bytes` and refuse one byte more, and accept an offer id of exactly `max_offer_id_bytes` and refuse one byte more; the peer id is accepted from 1 to `max_peer_id_bytes` bytes.

#### Scenario: Each bound
- **WHEN** a signal with peer id "p" and offer id "o" carries a candidate of 16384 `a`, then of 16385; an offer of 65536, then 65537; an answer of 65536, then 65537; and an offer id of 64 `a` with offer "x", then of 65 (`protocol_test`, `signal_bounds`)
- **THEN** the first of each pair is read and the second gives `std::nullopt`

### Requirement: Protocol: Writing a signal
`write_signal(message, nonce, memory)` SHALL return the message's JSON text with its members in Trystero's order `peerId`, `offerId`, `offer`, `answer`, `candidate`, `passive` (only when true, written `true`), `nonce`, leaving out every empty one, appended field by field into a buffer reserved up front.

#### Scenario: The member order
- **WHEN** `write_signal({.peer_id = "me", .offer_id = "o1", .candidate = "1,2$AA==", .passive = true}, "n0nce")` is called
- **THEN** it returns `{"peerId":"me","offerId":"o1","candidate":"1,2$AA==","passive":true,"nonce":"n0nce"}`, and `read_signal` of its parse gives candidate `1,2$AA==`, offer id `o1` and passive true

### Requirement: Protocol: Nostr kind and event id
`nostr::topic_kind(topic)` SHALL return `20000 + str_to_num(topic, 10000)`, in the ephemeral range 20000 to 29999 so that relays forward the events and keep none (Trystero's `topicToKind`), and `nostr::event_id(pubkey, created_at, kind, topic, content, memory)` SHALL return the NIP-01 id: the SHA-256 of the JSON array `[0,"<pubkey>",<created_at>,<kind>,[["x",<topic>]],<content>]`, with the topic and the content escaped as JSON strings, fed to the hash piece by piece. The tag name `topic_tag` is `x`.

#### Scenario: The ids of the fixtures
- **WHEN** the events of `expected.json` are recomputed: pubkey `4f355bdcb7cc0af728ef3cceb9615d90684bb5b2ca5f859ab0f0b704075871aa`, created_at 1759700000, topic `2t3c266f57311q4o1j4w3j6o611y1m1u4d6st6b`, and the contents `{"peerId":"abc","nonce":"12345678"}`, a content with a quote, a backslash, a newline, a tab, non-ASCII text and U+0001, and the empty content
- **THEN** the kind is 22943 for all three and the ids equal the oracle's, the first being `37f4a4dff48b6313b6821bc0a11811b40cdca11e2de6630578de1299250f483b`

### Requirement: Protocol: The BIP-340 signer
`nostr::signer` SHALL be a BIP-340 key pair over libsecp256k1, the one key a peer signs every event with: `signer::from_secret(secret, memory)` takes 32 bytes and returns `std::optional<signer>` (nothing when the bytes are not a valid secret key), `pubkey()` returns the x-only public key as 64 lowercase hexadecimal digits valid as long as the signer, and `sign(message, aux)` returns the 64-byte Schnorr signature of a 32-byte message with 32 bytes of auxiliary randomness. Its signing context MUST be made in the resource given (`secp256k1_context_preallocated_create`, never `secp256k1_context_create`), which must outlive the signer.

#### Scenario: A key from a fixed secret
- **WHEN** `signer::from_secret` is called with 32 bytes of 7
- **THEN** it returns a signer whose `pubkey()` has 64 hexadecimal digits

### Requirement: Protocol: Verifying a signature
`nostr::verify(sig, id, pubkey)` SHALL return whether a 64-byte BIP-340 signature over a 32-byte event id verifies under a 32-byte x-only public key, false too when the key is not a valid point, using libsecp256k1's static context and allocating nothing.

#### Scenario: A signed event verifies
- **WHEN** `event_message(*key, 1759700000, "topic-a", R"({"peerId":"p"})", aux)` is parsed and its `sig`, `id` and `pubkey` members are decoded from hexadecimal
- **THEN** `verify(sig, id, pubkey)` is true

### Requirement: Protocol: The EVENT message a peer publishes
`nostr::event_message(key, created_at, topic, content, aux, memory)` SHALL return the client message `["EVENT",{"kind":<kind>,"tags":[["x",<topic>]],"created_at":<created_at>,"content":<content>,"pubkey":"<pubkey>","id":"<id>","sig":"<sig>"}]` as Trystero's `createEvent` makes it, written once into one buffer: the topic and the content are escaped once and the id is hashed over those same bytes, so the id and the text never disagree; `kind` is `topic_kind(topic)`, `id` is the event id and `sig` the signature of the id with `aux`.

#### Scenario: The message round trips through a relay's echo
- **WHEN** the message for topic `topic-a` and content `{"peerId":"p"}` is parsed, its event wrapped as `["EVENT","sub",<event>]` and read by `read_relay_message`
- **THEN** the event has subscription `sub`, topic `topic-a` and content `{"peerId":"p"}`

### Requirement: Protocol: Subscribing and closing
`nostr::subscribe_message(subscription, topics, since, memory)` SHALL return `["REQ",<subscription>,{"kinds":[...],"since":<since>,"#x":[...]}]` (Trystero's `subscribe`), where `kinds` lists the `topic_kind` of the topics once each in increasing order and `#x` lists the topics in the order given; `nostr::close_message(subscription, memory)` SHALL return `["CLOSE",<subscription>]`. Subscription ids and topics are escaped as JSON strings.

#### Scenario: Two topics with the same kind
- **WHEN** `subscribe_message("s", {t, t}, 100)` is called with one topic `t` of kind 22943 given twice
- **THEN** the filter's `kinds` is `[22943]` and `#x` lists `t` twice

#### Scenario: Closing
- **WHEN** `close_message("s")` is called
- **THEN** it returns `["CLOSE","s"]`

### Requirement: Protocol: What a relay sends
`nostr::read_relay_message(document)` SHALL return, as views into the parsed document, a `relay_message` (`std::variant<event, ok, eose, closed, notice>`) or `std::nullopt` when Trystero would ignore the text: the document is not a non-empty array whose first element is a string, the type is none of `EVENT`, `OK`, `EOSE`, `CLOSED` and `NOTICE`, or an `EVENT` has fewer than three elements or a third that is not an object. An `event` holds the subscription, the id, the content and the value of the first `x` tag whose second element is a string (empty when there is none); an `ok` holds the id, `accepted` (true only for a boolean true) and the reason; `eose` holds the subscription; `closed` holds the subscription and the reason; `notice` holds the text. A missing field reads as an empty view.

#### Scenario: A rejected event
- **WHEN** `["OK","abc",false,"rate-limited: slow down"]` is read
- **THEN** the message is an `ok` with id `abc`, `accepted` false and reason `rate-limited: slow down`

#### Scenario: Ignored texts
- **WHEN** `{"a":1}`, `[]`, `[1]`, `["EVENT","s"]` and `["PING"]` are read
- **THEN** each gives `std::nullopt`

### Requirement: Protocol: Room envelope frames
`envelope.hpp` SHALL define `max_action_frame_bytes` = 16 KiB, `max_room_token_bytes` = 128 and `max_room_frame_bytes` = `max_action_frame_bytes + 3 + max_room_token_bytes` (16515), and SHALL write the two frames every data channel message is: `room_frame(token, payload, memory)` writes `1`, the token's length as a big-endian uint16, the token, then the action frame (`begin_room_frame(token, payload_size, memory)` writes the header alone, with room reserved for the payload, so the action frame is appended in place and allocated once); `presence_frame(token, present, memory)` writes `2`, `1` or `0`, the token's length as a big-endian uint16, and the token (Trystero's `wrapRoomFrame` and `wrapRoomPresenceFrame`).

#### Scenario: The frames of the fixtures
- **WHEN** the token of my-app/room-1 is wrapped: `presence_frame(token, true)`, `room_frame(token, {2, 0, 99, 104, 97, 116})` and `presence_frame(token, false)`
- **THEN** the frames are `0201` + `0040` + the 64 ASCII bytes of the token, `01` + `0040` + the token + `020063686174`, and `0200` + `0040` + the token, equal to the three the oracle captured from Trystero's shared peer manager

### Requirement: Protocol: Decoding an envelope
`decode_envelope(frame)` SHALL return an `envelope` of views into the frame (`kind` room or presence, `token`, `present` for a presence frame, `payload` for a room frame), with the checks of Trystero's `unwrapFrame`, and `std::nullopt` when the frame is shorter than 3 or longer than `max_room_frame_bytes` bytes, its first byte is neither 1 nor 2, it is shorter than its header, or its token is empty or longer than `max_room_token_bytes`. A presence frame is `present` exactly when its second byte is 1.

#### Scenario: Decoding the fixtures
- **WHEN** the room frame and the absence frame of the fixtures are decoded
- **THEN** the first has kind `room`, the token of my-app/room-1 and the payload `2, 0, 99, 104, 97, 116`, and the second has kind `presence` and `present` false

#### Scenario: Malformed frames
- **WHEN** `decode_envelope` is called with the bytes `1, 0, 0` (an empty token) and `9, 0, 1, 65` (an unknown kind)
- **THEN** both give `std::nullopt`

### Requirement: Protocol: Action wire constants and names
`action_wire.hpp` SHALL define the version of the action wire `action_wire_version` = 2 (every frame's first byte) and the sizes `inline_header_size` = 36, `offer_header_size` = 48, `control_header_size` = 6, `chunk_header_size` = 14, `chunk_size` = `max_action_frame_bytes - chunk_header_size` = 16370, `max_action_name_bytes` = 32 and `max_reject_reason_bytes` = 200, `valid_action_name(name)` (1 to 32 UTF-8 bytes and no NUL, Trystero's check in `makeAction`) and `fits_inline(metadata_size, payload_size)` (true when `36 + metadata + payload <= 16384`). The enum `payload_format` is `text` = 0, `json` = 1, `binary` = 2 and `frame_kind` is `inline_payload` = 0, `offer` = 1, `chunk` = 2, `accepted` = 3, `rejected` = 4, `cancelled` = 5.

#### Scenario: Name validity
- **WHEN** `valid_action_name` is called with `abcdefghijklmnopqrstuvwxyz012345` (32 bytes), the same with a 33rd character, and ""
- **THEN** the results are true, false and false

#### Scenario: The inline limit
- **WHEN** `fits_inline(0, 16348)` and `fits_inline(0, 16349)` are called
- **THEN** the results are true and false

### Requirement: Protocol: Inline frames
`append_inline(out, name, format, metadata, payload)` and `encode_inline(name, format, metadata, payload, memory)` SHALL write an inline frame: byte 0 `2`; byte 1 `kind | (format << 4)`; bytes 2 to 33 the name, NUL padded to 32 bytes; bytes 34 and 35 the metadata's length as a big-endian uint16; then the metadata (JSON text, or none), then the payload. `append_inline` appends to a buffer that may already hold a room envelope's header, with the precondition `valid_action_name(name)` and `fits_inline`.

#### Scenario: Text, JSON and binary frames equal Trystero's
- **WHEN** `encode_inline` is called for ("chat", text, "", "hello from Trystero JS"), ("drink", json, "", `{"drink":"negroni","withIce":true,"n":[1,2.5,null]}`) and ("pic", binary, "", the bytes 0, 1, 2, 253, 254, 255)
- **THEN** the frames begin `0200` + `63686174`, `0210` + `6472696e6b` and `0220` + `706963` (the name padded with zeros to 32 bytes, then `0000`) and equal byte for byte the frames Trystero JS sent in the oracle

#### Scenario: Metadata, an empty payload and a name of 32 bytes
- **WHEN** ("chat", text, `{"from":"js","n":7}`, "hi"), ("@_hsready", text, "", "") and ("abcdefghijklmnopqrstuvwxyz012345", text, "", "x") are encoded
- **THEN** the first has the length `0013` (19) before its metadata and ends with `6869`, the second is exactly the 36-byte header, and the third carries all 32 bytes of the name without a NUL

### Requirement: Protocol: Offer and chunk frames
`append_offer`/`encode_offer(name, format, metadata, id, size, memory)` SHALL write an offer (kind 1): the 36-byte inline header with kind 1 (so the second byte is `0x01 | format << 4`), the transfer id as a big-endian uint32 at byte 36, the payload size as a big-endian float64 at byte 40, then the metadata (header 48 bytes); `append_chunk`/`encode_chunk(id, offset, data, memory)` SHALL write a chunk (kind 2): `2, 2`, the transfer id as a big-endian uint32, the offset as a big-endian float64, then at most `chunk_size` bytes of data (header 14 bytes). A payload that does not fit inline is offered, accepted by the receiver, then sent in chunks of `chunk_size` bytes in order, the last carrying the rest.

#### Scenario: A 40000-byte payload
- **WHEN** the fixture's 40000-byte binary payload with metadata `{"name":"big.bin"}` is offered and chunked
- **THEN** the offer is 66 bytes (48 + 18) beginning `0221` + the name `file` padded, with the size `40e3880000000000` (40000 as a float64) at byte 40, and the three chunks are 16384, 16384 and 7274 bytes (14 + 16370, 14 + 16370, 14 + 7260), each equal to the frame Trystero JS sent, the offsets decoding as 0, 16370 and 32740

### Requirement: Protocol: Transfer control frames
`append_accepted`/`encode_accepted(id)` and `append_cancelled`/`encode_cancelled(id)` SHALL write the 6 bytes `2, 3, <id uint32>` and `2, 5, <id uint32>`, and `append_rejected`/`encode_rejected(id, reason)` SHALL write `2, 4, <id uint32>` followed by the reason in UTF-8 cut to `max_reject_reason_bytes` (200) bytes.

#### Scenario: A rejection equals Trystero's
- **WHEN** `encode_rejected(id, "payload exceeds receiver size limit")` is called with the id of the fixture's offer
- **THEN** the frame is `0204` + the id + the 35 reason bytes, equal to the `rejected_too_large` frame Trystero JS answered with when its receiver limit was 30000 bytes

#### Scenario: A long reason is cut
- **WHEN** `encode_rejected(1, reason)` is called with a reason of 300 bytes
- **THEN** the frame is 206 bytes long

### Requirement: Protocol: Decoding an action frame
`decode_action(frame)` SHALL decode one action frame with exactly the checks Trystero's `handleData` makes and return a `decode_result{std::optional<decoded_action> frame, wire_error error}`, where `decoded_action` is `std::variant<proposal, chunk, control_message>` of views into the frame: a frame shorter than 2 or longer than `max_action_frame_bytes` bytes, a kind above 5, a format above 2, or a format other than 0 on a kind above 1 yields no frame and `wire_error::invalid_frame`; a first byte other than `action_wire_version` yields `incompatible_version`; kinds 0 and 1 are decoded as proposals, kinds 2 to 5 as controls. The enum `wire_error` is `invalid_frame`, `incompatible_version`, `invalid_control`, `invalid_header` and `invalid_proposal`; Trystero drops the peer for any of them.

#### Scenario: A wrong version
- **WHEN** a frame beginning `3, 0` is decoded
- **THEN** there is no frame and the error is `incompatible_version`

#### Scenario: Out-of-range kind and format
- **WHEN** frames whose second byte is `0x06` (kind 6) and `0x30` (format 3) are decoded
- **THEN** both give no frame and `invalid_frame`

### Requirement: Protocol: Decoding a proposal
For kinds 0 and 1 `decode_action` SHALL return a `proposal{kind, name, format, metadata, id, size, payload}`: the frame must hold its header (36 bytes inline, 48 for an offer) or the error is `invalid_header`; the name is the bytes 2 to 33 up to the first NUL and the metadata length is the uint16 at byte 34, and an empty name or a metadata that runs past the frame is `invalid_proposal`; an inline frame's `payload` is what follows the metadata and its `size` is that length; an offer's `id` is the uint32 at byte 36 and its `size` the float64 at byte 40, which MUST be an integer from 1 to 9007199254740991 (2^53 - 1), and the offer MUST end exactly after its metadata, else `invalid_proposal`.

#### Scenario: A frame with metadata
- **WHEN** the oracle's text-with-metadata frame is decoded
- **THEN** the proposal has kind `inline_payload`, name `chat`, format `text`, metadata `{"from":"js","n":7}` and payload `hi`

#### Scenario: A decoded offer
- **WHEN** the oracle's offer of 40000 bytes is decoded
- **THEN** the proposal has kind `offer`, `size` 40000 and metadata `{"name":"big.bin"}`, and re-encoding it with `encode_offer` reproduces the frame byte for byte

#### Scenario: Short and empty-named frames
- **WHEN** a 20-byte frame `2, 0, ...` and a 36-byte frame with an all-NUL name are decoded
- **THEN** the first gives `invalid_header` and the second `invalid_proposal`

### Requirement: Protocol: Decoding chunks and controls
For kinds 2 to 5 `decode_action` SHALL return a `chunk{id, offset, data}` or a `control_message{kind, id, reason}`: a frame shorter than 6 bytes, an accepted or cancelled frame that is not exactly 6 bytes, or a rejected frame longer than 6 + 200 bytes is `invalid_control`; the id is the uint32 at byte 2; a chunk's offset is the float64 at byte 6 and its data what follows byte 14, and a chunk of 6 to 13 bytes is returned with a NaN offset and no data; a rejected frame's `reason` is the bytes after byte 6.

#### Scenario: Chunks decode to their offsets
- **WHEN** the three chunk frames of the oracle are decoded
- **THEN** the offsets are 0, 16370 and 32740 as doubles and the data sizes 16370, 16370 and 7260

#### Scenario: A rejection decodes to its reason
- **WHEN** the oracle's `rejected_too_large` frame is decoded
- **THEN** it is a `control_message` of kind `rejected` with reason `payload exceeds receiver size limit`, and an accepted frame of the same id encodes to 6 bytes (`control_header_size`)

#### Scenario: Wrong-sized controls
- **WHEN** a 7-byte accepted frame and a 5-byte frame of kind 3 are decoded
- **THEN** both give `invalid_control`

### Requirement: Protocol: The protocol test against Trystero's values
The unit test `protocol` (`test/protocol_test.cpp`, registered `webcpp.run protocol : protocol_test.cpp : <testing.input-file>fixtures/expected/expected.json`, on both targets) SHALL reproduce, byte for byte, the values Trystero JS 0.26 computed, which it reads from the fixture whose path is its one argument: the topics, the room token and `str_to_num` (`topics`), the default relays, the derived relays and the shuffles (`relays`), the handshake proofs and the sealed text (`handshake_and_sealing`), the Nostr event ids, a signature's verification and a relay's echo (`nostr_events`), the inline frames, the chunked frames, the rejection and the envelopes (`inline_frames`, `chunked_frames`, `envelopes`), the signals and their bounds (`signals`, `signal_bounds`) and base64 (`text`). With no argument it MUST fail, printing `usage: protocol_test <fixtures/expected/expected.json>`.

#### Scenario: The test passes on both targets
- **WHEN** `b2 libs/trystero/test//protocol` is run natively and with `toolset=emscripten`
- **THEN** it reports no failure

### Requirement: Oracle: The inputs
`test/fixtures/cases/protocol.json` SHALL hold the inputs the oracle feeds Trystero's functions: four `topics` triples (app, room, peer: ("trystero-cpp-probe", "lobby", twenty zeros), ("my-app", "room-1", twenty `z`), ("café ☕ app", "sala ünïcode 🎉", "AbCdEfGhIjKlMnOpQrSt") and ("a", "b", "c")), six `str_to_num` texts, five `relays` app ids with `relay_redundancy` 5, the `shuffle` seeds 0, 1, 1234567 and 9007199254740991 over a length of 29, two `handshake` tuples, the `sealed` case (password `secret`, app `my-app`, room `room-1`, a two-line SDP plaintext and the fixed random generator multiplier 17 and offset 3), the `nostr_events` case (a public key, created_at 1759700000 and three contents, one with a quote, a backslash, a newline, a tab, non-ASCII text and U+0001, and one empty), the `actions` (text, json, binary, text with metadata, empty text, a 32-byte name, and a `chunked` payload of 40000 bytes with multiplier 7 and metadata `{"name":"big.bin"}`), `controls.max_receive_bytes` 30000, and the `envelope` case (my-app, room-1, the frame `2, 0, 99, 104, 97, 116`, timeout 60000).

#### Scenario: Non-ASCII inputs are covered
- **WHEN** the topics and the `str_to_num` texts are read
- **THEN** they include a text with a BMP character beyond ASCII (`é`, `☕`) and one with astral characters (`🎉`), which exercise the UTF-16 sum

### Requirement: Oracle: The expected values
`test/fixtures/expected/expected.json` SHALL be the file `oracle.mjs` writes (`JSON.stringify(out, null, 4)` and a newline) with the keys `topics`, `str_to_num`, `default_relays`, `relays`, `shuffle`, `handshake`, `sealed`, `nostr_events`, `actions`, `controls` and `envelope`, each computed by Trystero 0.26.0's own functions (`sha1`, `hashWith`, `deriveRoomNamespace`, `encrypt`, `genKey`, `strToNum`, `getRelays`, `toHex`, `toJson`, `topicPath` and `shuffle` of `@trystero-p2p/core`, `defaultRelayUrls` of `@trystero-p2p/nostr`, the action wire manager and the shared peer manager) and never by the C++ code. The action frames are captured from `createActionWireManager` sending to a fake peer, the chunked frames after the receiver's accepted frame, the `rejected_too_large` frame from a receiver with `maxReceiveBytes` 30000, and the envelope and presence frames from `SharedPeerManager` over a fake physical peer.

#### Scenario: The sections of the file
- **WHEN** `expected.json` is read
- **THEN** it has 4 topics, 6 `str_to_num` entries, 29 default relays, 5 relays entries, 4 shuffles, 2 handshakes, a `sealed` object, 3 Nostr events, 7 action entries (`text`, `json`, `binary`, `text_with_metadata`, `empty_text`, `thirty_two_byte_name`, `chunked`), one control (`rejected_too_large`) and an envelope with three frames

### Requirement: Oracle: Deterministic sealing
The oracle SHALL make the sealed fixture deterministic by replacing `crypto.getRandomValues` while Trystero's `encrypt` runs with `(i * multiplier + offset) % 256` per byte (Trystero draws the IV from it), so that the IV is `3,20,37,54,71,88,105,122,139,156,173,190,207,224,241,2` and the sealed text is the same on every run; it restores the real function afterwards.

#### Scenario: The fixed IV
- **WHEN** the oracle seals the plaintext `v=0\r\no=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n` under the key of secret/my-app/room-1
- **THEN** the text begins `3,20,37,54,71,88,105,122,139,156,173,190,207,224,241,2$` and its `key_material` is `a11095bfc9cdd863704127af9c923044169c2784ed35e2ca808601f62878e886`

### Requirement: Oracle: Trystero pinned at 0.26.0
The oracle SHALL run Trystero itself, pinned by `test/oracle/package.json` (private, `type: module`, `devDependencies` `@noble/secp256k1` 3.2.0, `@trystero-p2p/core` 0.26.0, `@trystero-p2p/nostr` 0.26.0, `trystero` 0.26.0 and `ws` 8.22.0) and `package-lock.json`, with Node, importing the modules of `node_modules/@trystero-p2p/core/dist` (`crypto.mjs`, `utils.mjs`, `action-wire.mjs`, `shared-peer.mjs`). It MUST be called as `node oracle.mjs <cases> <output-directory>`; with another number of arguments it prints `usage: node oracle.mjs <cases> <output-directory>` and exits 2.

#### Scenario: Wrong arguments
- **WHEN** `node oracle.mjs` is run without arguments
- **THEN** it prints the usage line and exits with status 2

### Requirement: Oracle: The lane and its only writer
`test/oracle/Jamfile` SHALL declare the lane with `webcpp.original node`, `webcpp.cases protocol : oracle.mjs : ../fixtures/cases : ../fixtures/expected` and `webcpp.lane oracle : cases-protocol`, targeting native and emscripten (the program runs once). `b2 libs/trystero/test/oracle//oracle` MUST re-run Trystero over the cases and fail when what it writes differs from the committed `expected.json` byte for byte; `b2 libs/trystero/test/oracle//update-expected` is the only writer of that file, which is never edited by hand. The lane is one of trystero's own lanes in the continuous integration, run once, and its values are what the `protocol` and `native_sealer` tests reproduce.

#### Scenario: A stale fixture fails the lane
- **WHEN** Trystero's output for a case differs from the committed `expected.json`
- **THEN** the `oracle` lane fails, and `update-expected` writes the file again

### Requirement: Room: Joining by construction
The library SHALL join a room through `app::join(std::string_view room_id, room_options options = {})` (see "App: join"), which makes the room with the private `detail::room_impl::make` and returns it as a `room`; `room` has no public constructor but its move constructor. The room copies its options into its resource, derives the room token and the root and self topics from the app id, the room id and the app's self id, then starts the Nostr strategy, which connects to every relay of the app, subscribes to the root topic and the peer's own topic, and announces the peer. The backend `factory` MUST outlive the app and its rooms, and `room_id` is read during the call only. Accessors are `get_executor()` (the app's, a `boost::asio::any_io_executor`), `self_id()` (the app's, a `std::string_view` valid as long as the app), `relay_urls()` (the app's, a `std::span<const std::pmr::string>`) and `peers()`.

#### Scenario: Two rooms of one app meet
- **WHEN** an `app` with `self_id` `AAAAAAAAAAAAAAAAAAAA` and an `app` with `self_id` `zzzzzzzzzzzzzzzzzzzz`, each holding the room `lobby` (and `elsewhere`) of one app id, join of the app `webcpp-trystero-tests` on one executor, over the memory backend with two relays
- **THEN** `room_test`'s `two_peers_join_and_exchange_every_payload_format` sees `a` report one join of `zzzzzzzzzzzzzzzzzzzz` and `z` one join of `AAAAAAAAAAAAAAAAAAAA`, and `a.peers()` equals `{"zzzzzzzzzzzzzzzzzzzz"}`

#### Scenario: Three peers form a mesh
- **WHEN** three apps with ids `KKKKKKKKKKKKKKKKKKKK`, `LLLLLLLLLLLLLLLLLLLL` and `MMMMMMMMMMMMMMMMMMMM` each join the room `mesh`, and the first sends `all` on the action `shout`
- **THEN** (`three_peers_form_a_mesh`) every room lists two peers, and both other rooms receive `all`

### Requirement: Room: Movable and not copyable
The `room` handle SHALL be one membership: it MUST be non-copyable (copy constructor and copy assignment deleted) and `noexcept` move-constructible; a moved-from room MUST only be destroyed or assigned. The move assignment `room& operator=(room&&) noexcept` leaves the membership the target held at once, exactly as the destructor does, then takes the source's membership.

#### Scenario: Moving a room over a joined one leaves it
- **WHEN** room `b` (id `LLLLLLLLLLLLLLLLLLLL`, joined with `a`) holds an action `pending` with an `async_receive` outstanding, and `b = trystero::room(..., config("MMMMMMMMMMMMMMMMMMMM"), "elsewhere")` is executed
- **THEN** (`moving_a_room_over_a_joined_one_leaves_it`) `a` reports one peer leave and `a.peers()` is empty, and the pending receive completes with `error::room_left` although the action still holds the room's state

### Requirement: Room: Destruction leaves at once
The destructor of a joined `room` SHALL leave without the farewell's grace period: it sends `@_leave` to every peer, detaches every binding, sends `presence(0)` for the room on every live connection of the app (a connection with no binding left then idles for `idle_timeout`, and one that other rooms use stays), stops the strategy and the signaling, and completes every pending operation (receives, pings, transfers) with `error::room_left`. Operations the room leaves pending on its executor free into the room's memory resource, so that resource MUST outlive them.

#### Scenario: A room destroyed while peers are joined
- **WHEN** a joined `room` and its `app` go out of scope in `leaving_is_seen_by_the_others_and_a_return_reconnects` after the `async_leave` completed, and a new `app` with the same id `JJJJJJJJJJJJJJJJJJJJ` joins the room
- **THEN** the remaining room `a` has reported one peer leave and, after the new room is constructed, reports a second join (`joined.size() == 2`)

### Requirement: Room: The configuration is copied into the room's resource
The app SHALL copy every part of its configuration (strings, relay list, ICE servers) into `app_config::memory` at construction and move the `log` sink there, and a room SHALL retain its scalar options and use its app's password without copying it into the room resource, `room_options::memory`, the app's when null; the caller's strings need not outlive the call. Every allocation of the app goes through its resource and every allocation of a room through the room's, each of which MUST outlive the app or the room, every value handed out and every operation left pending, and each of which is used from the app's executor only.

#### Scenario: A counting resource sees the room's allocations
- **WHEN** `a_flood_of_announcements_is_bounded` builds an app with `app_config_in` over a counting resource declared before the `io_context`
- **THEN** the app's state, queues and strings are allocated through that resource, whose `in_use` is compared before and after the flood

### Requirement: Room: Default relays derive from the app id
When `app_config::relay_urls` is empty the app SHALL use `relay_redundancy` (5 by default) of the 29 `protocol::default_nostr_relays`, chosen by `protocol::derive_relays(app_id, default_nostr_relays, relay_redundancy, memory)`, a shuffle seeded by the app id so that every peer of an app, and Trystero JS, picks the same relays; when `relay_urls` names relays they are used as given and `relay_redundancy` is ignored. Every room of the app runs its strategy over the same relays, and `app::relay_urls()` and `room::relay_urls()` return them.

#### Scenario: Two rooms of one app pick the same relays
- **WHEN** two rooms are joined through one app with empty `relay_urls`, and a second app of the same `app_id` is made
- **THEN** the two rooms' `relay_urls()` lists are the app's, equal to the second app's, with five entries each

### Requirement: Room: Self id
The room SHALL use the app's self id, `app_config::self_id` when it is not empty and otherwise 20 random characters (`protocol::peer_id_size`) from `protocol::id_alphabet` drawn through the backend's `random` once per app, so that every room of an app announces, signals, tie-breaks and handshakes with one id, as Trystero's one `selfId` per page does. Ids are ordered by plain string comparison (`<`) wherever the protocol compares them.

#### Scenario: A random id has the default length
- **WHEN** an app is made with an empty `self_id` and two rooms are joined
- **THEN** both rooms report the same `self_id()` of 20 characters, each belonging to `protocol::id_alphabet`

### Requirement: Room: An over-long self id is replaced and reported
When `app_config::self_id` is longer than `protocol::max_peer_id_bytes` (64) the app SHALL replace it with 20 random characters, log it in the category `app`, and report it to `app::on_error` once, after the constructor has returned, with an empty peer id, `error::invalid_configuration` and the text `self_id is longer than any peer accepts, so the app uses a random one`; an id of exactly 64 bytes is kept.

#### Scenario: A 65-byte id
- **WHEN** an app is made with `self_id` of 65 `x` characters and an `app::on_error` handler is set right after the constructor
- **THEN** (`long_peer_ids_are_bounded`) `self_id().size() == protocol::peer_id_size` and exactly one error `error::invalid_configuration` is reported; `protocol::read_signal` accepts a signal whose `peerId` has 64 bytes and rejects one of 65

### Requirement: Room: Default ICE servers
When `app_config::ice_servers` is empty the app SHALL use `default_ice_servers(memory)` (declared in `options.hpp`), Trystero's `defaultIceServers`: four STUN servers, each with one URL and no credentials, in this order: `stun:stun.l.google.com:19302`, `stun:stun1.l.google.com:19302`, `stun:stun2.l.google.com:19302` and `stun:stun.cloudflare.com:3478`. `backend::ice_server` has `urls` (a `std::pmr::vector<std::pmr::string>`), `username` and `credential`.

#### Scenario: The default list
- **WHEN** `trystero::default_ice_servers()` is called
- **THEN** it returns four servers whose single URLs are the four listed above, with empty `username` and `credential`

### Requirement: Room: ICE server URL grammar
The library SHALL accept as an ICE server URL exactly `scheme:host[:port]` with the scheme `stun`, `stuns`, `turn` or `turns` (compared without regard to case, RFC 7064 and RFC 7065), the host a name or an address (an IPv6 address in brackets), the port a number from 1 to 65535, and for `turn` and `turns` an optional query `?transport=udp` or `?transport=tcp`; `detail::ice_url_fault(url)` returns an empty view for a usable URL and otherwise the reason. A server MUST also name at least one URL, and a TURN server (a `turn` or `turns` URL) MUST have a non-empty user name and credential (`detail::ice_server_fault`).

#### Scenario: Accepted and refused URLs
- **WHEN** `ice_urls_are_checked_as_webrtc_checks_them` checks `STUN:stun.example`, `Turn:host:3478?transport=udp`, `TURNS:relay.example`, `stun:stun.l.google.com:19302`, `stun:127.0.0.1`, `stuns:host:5349`, `turn:[::1]:3478?transport=udp` and `turns:relay.example?transport=tcp`
- **THEN** all eight are usable; and `http://host`, `stun:`, `stun:host:`, `stun:host:0`, `stun:host:70000`, `stun:host:x`, `stun://host`, `stun:host?transport=udp`, `turn:host?transport=sctp`, `stun:[::1`, `stun:::1` and `no scheme` all return a non-empty fault

### Requirement: Room: Unusable ICE servers are dropped and reported
When the app is made it SHALL lowercase the scheme of every ICE server URL, drop each server `detail::ice_server_fault` refuses, log it in the category `app`, and report each to `app::on_error` once, after the constructor has returned, with an empty peer id, `error::invalid_configuration` and a text that begins `the ICE server <first URL> ` and ends `, so the app goes on without it`; the app goes on with the usable servers on every backend, and the backends receive the scheme in lowercase.

#### Scenario: Five servers, two unusable
- **WHEN** `unusable_ice_servers_are_dropped_and_reported` configures `stun:127.0.0.1:9`, `http://127.0.0.1:9`, `turn:127.0.0.1:9` without credentials, `turn:127.0.0.1:9` with user `user` and credential `secret`, and `STUN:127.0.0.2:9`
- **THEN** exactly two errors are reported to `app::on_error`, each with an empty peer and `error::invalid_configuration`, the rooms still join their peer, the backend never sees `http://127.0.0.1:9`, and it sees `turn:127.0.0.1:9` and the lowercase `stun:127.0.0.2:9`

### Requirement: Room: Joined peers
`room::peers()` SHALL return, as a `std::pmr::vector<peer_id>` in the room's resource (`peer_id` is `std::pmr::string`), the ids of the peers whose handshake completed, Trystero's `getPeers`; it does not list peers still in their handshake and does not hand out their connections. `on_peer_join(F)` sets what is called with each peer, as a `std::string_view` valid for the call, once its handshake completes, and `on_peer_leave(F)` what is called with each joined peer that leaves; each setter takes any callable invocable with a `std::string_view`, keeps it in the room's resource and replaces the one set before.

#### Scenario: A handler set after construction sees every join
- **WHEN** `on_peer_join` is set right after a room is constructed and a peer then joins
- **THEN** the handler is called with the peer's id, because a join is reported from the room's executor after the constructing call has returned; `on_peer_join` does not replay peers already joined when it is set later, and `peers()` lists them

### Requirement: Room: on_error reports
`room::on_error(F)` SHALL set what is called with three views valid for the call, the peer's id, a `boost::system::error_code` of `error_category()` and a description, for each problem with one peer that the room handled itself (Trystero's `onPeerError`): `error::wrong_password`, `error::handshake_timed_out`, and `error::backend_failure`; the configuration problems (`error::invalid_configuration`, with an empty peer id) are reported to `app::on_error`; the handler replaces the one set before and is posted to the room's executor. Signals that do not open with the room key are reported as `error::wrong_password` at most once per negotiation and are paced (see "Signaling: Failed opens are reported at a bounded pace").

#### Scenario: A refused peer connection
- **WHEN** the memory backend refuses the first peer connection (`refuse_peer_connections = 1`) in `a_refused_peer_connection_is_reported_and_tried_again`
- **THEN** exactly one error `error::backend_failure` is reported across both rooms, and the next announcement starts a negotiation that joins within 30 s

### Requirement: Room: The handshake
The room SHALL use its app's immutable `app_config::password`, already authenticated by the initial sealed negotiation before connection adoption, to run Trystero's password handshake with each peer the room binds to a connection (a binding is the room's use of a shared connection, made when the room joins, adopts or reuses it), over the internal actions `@_hsdata` and `@_hsready`: with an empty password it passes its local side at once; with a password the peer whose id is lower (`self_id < peer`) sends the JSON `{"__trystero_pw":"challenge","c":<36 random characters>}` as `@_hsdata`, the other answers `{"__trystero_pw":"response","h":<proof>}` with `proof = protocol::handshake_proof(challenge, password, app_id, room_id)`, and the challenger passes its local side only when the response is `"response"` with the digest `h` of any of the last 8 challenges it sent that peer since their last completed handshake (see "Rebinding: The initiator accepts any of its last 8 challenges"). The higher-id responder answers and passes its local side without requiring a reciprocal proof, exactly as Trystero JS 0.26.0 does; app-domain authentication, rather than this one-way room exchange alone, protects the C++ app's admission. Passing the local side sends `@_hsready`; a peer becomes active, and `on_peer_join` is posted, only when its local side passed and the peer's `@_hsready` arrived.

#### Scenario: The same password joins
- **WHEN** two apps with `self_id` `FFFFFFFFFFFFFFFFFFFF` and `GGGGGGGGGGGGGGGGGGGG` are each configured with app password `s3cret` and join the room `private`
- **THEN** (`the_same_password_joins_and_another_does_not`) both list one peer

### Requirement: Room: A peer with another password never joins
When a handshake message has the wrong shape, a response has the wrong proof, or a challenge's `c` is missing or not a string, the room SHALL report the peer with `error::wrong_password` and the text `incorrect password for overlapping room`, send the peer `@_leave`, and detach its binding from the room (the connection stays for the app's other rooms) without ever calling `on_peer_join` or `on_peer_leave` for it; a peer whose signals do not open with the room key never reaches the handshake. Over a shared connection the room binds the peer again at its rebinding times (see "Rebinding: The schedule after a kick"), and only the lower id, the handshake's initiator, verifies a response proof. Either side reports malformed handshake data that its pending handshake rejects.

#### Scenario: An intruder with another password
- **WHEN** a third app with self id `HHHHHHHHHHHHHHHHHHHH` and password `guess` attempts to join the room `private` where `FFFF...` and `GGGG...` use `s3cret`, and `a` and the intruder have `on_error` handlers
- **THEN** after 1.5 s the intruder lists no peers, `a` still lists exactly one, and every reported error equals `error::wrong_password`

### Requirement: Room: Handshake timeout
The room SHALL start a timer of `room_options::handshake_timeout` (10 s by default, Trystero's `handshakeTimeoutMs`) when the room binds the peer, cancel it when the peer becomes active, and when it fires report the peer with `error::handshake_timed_out` and the text `handshake timed out`, send `@_leave` and detach the peer's binding.

#### Scenario: A silent peer
- **WHEN** a connected peer never sends `@_hsready` within `handshake_timeout`
- **THEN** the room reports `error::handshake_timed_out` for that peer, removes it, and does not call `on_peer_leave` because it never joined

### Requirement: Room: Pending peers may only speak the internal handshake
The room SHALL deliver an action frame from a peer only when the peer is active, except the internal actions `@_leave`, `@_hsdata` and `@_hsready`, which a peer still in its handshake may send; every other frame of a pending peer is dropped. The room's own internal sends (handshake, ping, farewell) address pending peers too, while an action a program sends goes only to active peers.

#### Scenario: A message before the handshake completes
- **WHEN** a peer sends an inline frame of the action `chat` before its handshake passed
- **THEN** the room does not deliver it to `chat`'s handler

### Requirement: Room: Internal actions
The room SHALL reserve action names that begin with `@_` for internal use: `@_hsdata`, `@_hsready`, `@_ping`, `@_pong` and `@_leave` are handled by the room, and a frame of any other `@_` action (Trystero's media and renegotiation actions `@_stream`, `@_track` and `@_signal`, not ported) is ignored without error. An `@_leave` received from a peer removes it with the reason `peer left the room`.

#### Scenario: An unknown internal action
- **WHEN** a peer sends a frame of the action `@_stream`
- **THEN** the room ignores it and the peer stays joined

### Requirement: Room: Ping
`room::async_ping(std::string_view peer, CompletionToken&&)` SHALL send `@_ping` to an active peer, answer a received `@_ping` with `@_pong`, and complete with `void(boost::system::error_code, std::chrono::milliseconds)`: the time from sending to receiving the `@_pong` as `factory.now_ms()` measures it. Pings of one peer complete in the order sent, the oldest on each `@_pong`. It completes with `error::no_such_peer` and zero milliseconds when the peer is not active, with `error::room_left` when the room has been left, and with `error::peer_disconnected` when the peer goes away first; the peer id is copied at the call.

#### Scenario: Ping a peer and a stranger
- **WHEN** `ping_measures_a_round_trip` pings the joined peer `EEEEEEEEEEEEEEEEEEEE` and then `nobody`
- **THEN** the first completes with no error and a duration of at least 0 ms, and the second with `error::no_such_peer`

### Requirement: Room: Leaving with the farewell
`room::async_leave(CompletionToken&&)` SHALL send `@_leave` to every peer, wait 99 ms (Trystero's farewell grace), then detach every binding, send `presence(0)` for the room on every live connection of the app (a connection other rooms use stays; one with no binding left idles for `app_config::idle_timeout`), close every signaling negotiation and relay socket, complete every pending operation of the room with `error::room_left`, and complete the leave with `void(boost::system::error_code)` and no error; when the room was already left it completes at once with no error. The other rooms see the leave through `on_peer_leave`.

#### Scenario: Leaving is seen and a return reconnects
- **WHEN** `b` (`JJJJJJJJJJJJJJJJJJJJ`) joins `a` (`IIIIIIIIIIIIIIIIIIII`) in the room `leave`, calls `async_leave`, and later a new room with the same id joins
- **THEN** (`leaving_is_seen_by_the_others_and_a_return_reconnects`) the leave completes without error, `a` reports one peer leave and then lists no peers, and the new room produces a second join on `a`

### Requirement: Room: Operations after a leave
Once a room has been left (by `async_leave`, its destructor or a move assignment over it) the room SHALL complete `async_send` and `async_ping` with `error::room_left`, complete a pending and a later `action::async_receive` with `error::room_left` and an empty value, and bind nothing: a connection that finishes opening is closed, not adopted, and presence or an announcement binds nothing, without reporting a join.

#### Scenario: A receive outstanding at the leave
- **WHEN** `async_receive` is outstanding on the action `pending` of a room that is then moved over
- **THEN** the handler is called with `error::room_left`

### Requirement: Room: Peer removal
When a peer's binding ends (the peer sends `@_leave` or `presence(0)`, the room drops it, or the connection ends), the room SHALL remove the peer once: detach its binding (unless the connection itself ended it), complete its pending pings with `error::peer_disconnected`, fail its transfers (`error::peer_disconnected`), forget its negotiation, restart the announcement cadence only when no live connection to that peer remains (over a live one every rebinding goes through the connection), and call `on_peer_leave` only if the peer had been active.

#### Scenario: A peer that left before joining
- **WHEN** a pending peer is dropped for a failed handshake
- **THEN** `on_peer_leave` is not called for it

### Requirement: Room: Executor discipline
Every handler the room calls and every completion it posts SHALL run on the app's executor, the one the app was constructed with and every room of it shares, never inside the call that caused them: completions are always posted, even those that complete immediately with an error. A completion whose associated executor is the room's is posted through the room's executor and allocated as its associated allocator says; one bound to another executor is handed to it. The room and the app are used from that executor only; with a single-threaded executor neither the room nor its memory resource needs to be thread-safe.

#### Scenario: An immediate failure is still asynchronous
- **WHEN** `async_ping("nobody", handler)` is called
- **THEN** the handler has not run when `async_ping` returns, and runs later from the executor with `error::no_such_peer`

### Requirement: Room: The diagnostic log
The app and its rooms SHALL send their diagnostics to `app_config::log`, a `handler<void(std::string_view category, std::string_view text)>`, with the categories `app`, `nostr`, `signal`, `webrtc`, `room` and `action`; a sentence is composed in the app's resource from its parts (texts, integers, error codes) only when there is a sink, so an app without one composes no text. All parts of the app and its rooms share one sink held through a `std::shared_ptr`. The room logs `joining room <room> of app <app> as <self> over <n> relays` at start and `leaving room <room>` on leaving.

#### Scenario: A sink sees a relay's refusal
- **WHEN** a log sink is set on a room whose backend refuses a connection
- **THEN** the sink receives the category `webrtc` and a sentence beginning `the backend made no connection for `

### Requirement: Room: Announcement cadence
The Nostr strategy SHALL announce the peer on the root topic as soon as a relay opens, then after 233, 533, 1333 and 5333 ms, then every 60 s (five announcements in the warm-up, Trystero's cadence); each announcement is a signed event whose content is a signal with only the peer id and a random nonce of 8 characters (`protocol::nonce_size`). A closed or backing-off relay is not announced on: the next attempt is scheduled for the longer of 60 s and the remaining backoff. When a peer leaves and no live connection to it remains, the cadence starts over from the first step on every open relay.

#### Scenario: A relay stays open
- **WHEN** a room has been connected to one relay for 60 s of the clock
- **THEN** (`a_relay_that_closes_at_once_is_tried_less_and_less_often`) the relay has seen the warm-up's five announcements and then the first of the steady ones

### Requirement: Room: Relay reconnection backoff
When a relay closes, the strategy SHALL reconnect after a random share (a uniform fraction from two random bytes divided by 65536) of a bound that starts at 3333 ms, doubles after each attempt to a maximum of 60,000 ms, and returns to 3333 ms only when the connection that was lost had stayed open for 60 s; it logs `lost <url>, next attempt within <bound> ms` (or `could not reach` when it never opened) in the category `nostr` and `reconnected to <url>, subscribed to the room's topics` when it reopens. A stopped strategy and a retired relay are not reconnected.

#### Scenario: A relay that closes after every subscription
- **WHEN** a relay with `close_after_subscribe` is used with the clock 100 times faster
- **THEN** (`a_relay_that_closes_at_once_is_tried_less_and_less_often`) the logged bounds begin `3333, 6666, 13332, 26664, 53328, 60000`, and after a connection that lasted 60 s and was then closed the next bound is 3333

### Requirement: Room: Rate-limited and retired relays
The strategy SHALL treat a relay's `OK` with `false` and a reason beginning `rate-limited:`, or a `CLOSED` with that reason, as a backoff: the relay is not published to or announced on for 60 s, the pause doubling to a maximum of 15 minutes while the replies persist, and an accepted event resets it once the pause has passed. A reason beginning `blocked:`, `restricted:`, `auth-required:` or `pow:` retires the relay for the life of the room (its announcements stop and its socket closes). A `duplicate:` refusal is ignored, any other refusal and every `NOTICE` is logged, and a `CLOSED` that is not terminal makes the strategy resubscribe after the longer of the remaining backoff and 5333 ms.

#### Scenario: One relay refuses everything
- **WHEN** relay A answers `rate-limited:` to every event while relay B works
- **THEN** (`a_rate_limited_relay_does_not_stop_discovery`) both rooms still join through B, and fewer than 12 events were refused by A

### Requirement: Room: Features not ported
The library SHALL NOT offer, and a room MUST ignore the frames of, the parts of Trystero 0.26.0 that `doc/differences.adoc` lists as not ported: audio and video (`addStream`, `addTrack` and the actions `@_stream`, `@_track`, `@_signal`) and the renegotiation and media routing over a shared connection, request actions (`request`, `requestMany`, `onRequest`), `onReceive` and the progress callbacks `onProgress` and `onReceiveProgress`, cancelling a send with an `AbortSignal`, `onPeerHandshake`, passive rooms, relay sockets and a Nostr key shared by the rooms of an app, the strategies other than Nostr, and `trickleIce: false`, `turnConfig` and `rtcPolyfill`. A send to a peer that has not joined fails with `error::no_such_peer` where Trystero warns and resolves, and `on_peer_join` does not replay joined peers.

#### Scenario: Two rooms of one app given one id
- **WHEN** a C++ `app` joins `room` and `room2` and meets a Trystero JS page that joins both, the two rooms having the app's one `self_id`
- **THEN** it meets the page in both rooms, over one connection on each side, which the first sharing scenario of the `interop` lane checks

### Requirement: Actions: Making an action
`room::make_action<T>(std::string_view name)` SHALL return an `action<T>` for a named channel of messages, Trystero's `makeAction`, where `T` is one of `std::string`, `std::pmr::string`, `std::string_view`, `boost::json::value`, `bytes`, `pmr_bytes`, `std::span<const std::uint8_t>` and `payload`; `action<T>::name()` returns the name. The action keeps the room's state alive, registers a receiver for the name, and from then on delivers the frames of that name that arrived earlier; a name that is not 1 to 32 UTF-8 bytes without NUL is accepted by `make_action` but makes every send fail with `error::invalid_action_name`.

#### Scenario: An action made after the peer joined
- **WHEN** `a.make_action<std::string>("chat")` is made and the room has peers
- **THEN** `chat_a.name()` is `chat` and a send reaches the peer's action of that name

### Requirement: Actions: Payload types and Trystero formats
The library SHALL map an action's type to Trystero's payload format (text 0, JSON 1, binary 2) and to how a message is received: `std::string` is text received as a copy with `std::allocator`; `std::pmr::string` is text copied into the room's resource; `std::string_view` is text received as a view of the frame; `boost::json::value` is JSON, serialized once at the send into the room's resource and received parsed into the room's JSON storage; `bytes` (`std::vector<std::uint8_t>`) is binary received as a copy; `pmr_bytes` (`std::pmr::vector<std::uint8_t>`) is binary received as the transfer's buffer moved, in the room's resource; `std::span<const std::uint8_t>` is binary received as a view; and `payload` (`std::variant<std::string, boost::json::value, bytes>`) carries any of the three, the alternative chosen by the received format. A value that owns its storage is moved into the send; a view is copied once, at the call, into the room's resource.

#### Scenario: Every format between two peers
- **WHEN** `a` sends `"hello"` with metadata `{"n":1}` on the text action `chat`, `{"drink":"negroni"}` on the JSON action `drink` and `bytes{0, 1, 255}` on the binary action `pic`
- **THEN** (`two_peers_join_and_exchange_every_payload_format`) all three sends complete without error, and `z` receives `hello` with `info.from == "AAAAAAAAAAAAAAAAAAAA"` and `info.metadata == {"n":1}`, a JSON object with `drink` equal to `negroni`, and the three bytes

### Requirement: Actions: Handlers and message_info
`action<T>::on_message(F)` SHALL set what is called with every message, Trystero's `onMessage`, for any callable invocable with `T` and `const message_info&`; the handler is kept in the room's resource and replaces the one set before, and messages that queued while the action had no handler are delivered to it first, in arrival order. `message_info` has `peer_id from` (the sender's id) and `std::pmr::string metadata` (the JSON text the sender passed, or empty), both in the receiving room's resource. A view type given to the handler is valid for the duration of the call only.

#### Scenario: Views read the frame in place
- **WHEN** `b` makes `std::string_view` action `note` and `std::span<const std::uint8_t>` action `blob`, and `a` sends the text `a view, copied once at the call` and 50,000 bytes with metadata `{"k":1}`
- **THEN** (`view_handlers_read_the_frame_without_copying`) `b` sees that text, 50,000 bytes and the metadata `{"k":1}`

### Requirement: Actions: A message of another format is not delivered
The room SHALL NOT deliver to an action's handler a message whose payload format is not the one `T` receives (a text message to a `boost::json::value` action, a JSON text that does not parse, and so on); the message is consumed. The one exception is `payload`, which accepts all three formats.

#### Scenario: Text sent to a binary action
- **WHEN** a peer sends the text `x` on the name of a `bytes` action
- **THEN** that action's handler is not called

### Requirement: Actions: Actions of one name share a receiver
The room SHALL keep one receiver slot per action name: several `action` objects of different types may send under the same name (one may send by moving strings, another by views), but the handler and the queue are the name's, so `on_message` of any action of that name replaces the one set before.

#### Scenario: Sending and receiving on one name with two types
- **WHEN** `a.make_action<std::string>("any")` and `a.make_action<boost::json::value>("any")` both send, and `b.make_action<trystero::payload>("any")` receives
- **THEN** (`async_receive_works_in_a_coroutine`) `b`'s coroutine receives the text `text` and then the JSON `[1,2]`

### Requirement: Actions: The receive queue
Without a handler, messages of an action SHALL wait for `action<T>::async_receive`: a pending receive takes the next message to arrive, and up to 1024 messages per action name are queued; at the 1025th the oldest is dropped and logged in the category `action` as `dropping the oldest unread message of <name>`. `async_receive(CompletionToken&&)` completes with `void(boost::system::error_code, T, message_info)`: the oldest queued message at once, else the next to arrive; a message of another payload format completes with `error::transfer_rejected`, an empty `T{}` and the sender's `message_info`, and leaving the room with `error::room_left`. `T` MUST own its storage: `std::string_view` and `std::span<const std::uint8_t>` fail a `static_assert` ("a received view would outlive the message: await an owning type").

#### Scenario: A coroutine awaits two messages
- **WHEN** a coroutine does `auto [ec, value, info] = co_await inbox.async_receive(asio::as_tuple(asio::use_awaitable))` twice on a `payload` action
- **THEN** (`async_receive_works_in_a_coroutine`) it sees the text `text` and then the serialized JSON `[1,2]`

### Requirement: Actions: Frames that arrive before the action exists
The room SHALL keep an inline frame whose action has no receiver yet, at most 64 pending items per peer (waiting frames and offered transfers together) for 120 s, and deliver it when `make_action` registers the name; an offer for an action without a receiver is held, and accepted only once a receiver exists.

#### Scenario: A message sent before the receiver is made
- **WHEN** a peer sends on the name `late` and `make_action("late")` is called within 120 s
- **THEN** the action's handler receives the message

### Requirement: Actions: Sending
`action<T>::async_send(T value, send_options options, CompletionToken&&)` and `async_send(T value, CompletionToken&&)` (every active peer, no metadata) SHALL send a message to `options.targets` (an empty list means every active peer) and complete with `void(boost::system::error_code)`; `send_options` has `std::pmr::vector<peer_id> targets` and `std::pmr::string metadata` (JSON text, empty for none) and owns its strings. The value is taken at the call. The operation completes once each target's local data channel has taken the inline frame, or the last chunk of a transfer the receiver accepted, as Trystero's `send` resolves; nothing confirms that the receiver has the message. It completes with the first failure among the targets.

#### Scenario: Sending to nobody
- **WHEN** a room with no active peer sends a message to every peer
- **THEN** the send completes with no error and sends nothing

### Requirement: Actions: Send failures
`async_send` SHALL complete with: `error::room_left` when the room has been left; `error::invalid_action_name` when the name is not 1 to 32 bytes or contains NUL; `error::metadata_too_large` when the metadata is longer than `protocol::max_action_frame_bytes - protocol::offer_header_size` (16,336 bytes); `error::no_such_peer` for a target that is not an active peer (the other targets still receive the message); `error::transfer_rejected` when a receiver refuses a transfer; `error::transfer_timed_out` when a receiver neither accepts nor refuses within 120 s; `error::channel_stalled` when a data channel keeps more than 64 KiB buffered for 10 s; and `error::peer_disconnected` when a channel closes or a peer is lost first.

#### Scenario: A name of 33 bytes
- **WHEN** a send is made on an action whose name has 33 bytes
- **THEN** it completes with `error::invalid_action_name` and no frame is sent

### Requirement: Actions: Inline frames
A payload SHALL travel inline, in one data-channel frame, when `protocol::inline_header_size` (36) plus the metadata size plus the payload size is at most `protocol::max_action_frame_bytes` (16 KiB, 16,384 bytes), and the frame is written once, inside its room envelope, and shared by every target. A frame is handed to the data channel only while the channel buffers at most 64 KiB (`backend::data_channel::low_threshold`, 0xffff); otherwise the send waits for the channel to drain.

#### Scenario: A payload at the boundary
- **WHEN** the payload plus 36 header bytes and the metadata total exactly 16,384 bytes
- **THEN** it travels inline; one byte more makes it an offer with chunks

### Requirement: Actions: Chunked transfers
A payload that does not fit one frame SHALL be offered (an offer frame with a transfer id from a per-room counter starting at 0, the size and the metadata), and sent in chunks of at most `protocol::chunk_size` (16,370 bytes) in order, each handed to the channel only after the previous one and only while the channel buffers at most 64 KiB, waiting at most 10 s per chunk; the chunks start only after the receiver's `accepted` control frame, which must arrive within 120 s. A refusal (`rejected`) fails the send with `error::transfer_rejected`; a failure other than a rejection or a peer loss sends the receiver a `cancelled` control frame. A transfer in flight keeps its bytes alive through the payload's owner, so the caller's value need not outlive the call.

#### Scenario: A hundred kilobytes each way
- **WHEN** both peers send 100,000 bytes on the action `file`, one with metadata `{"name":"big.bin"}`
- **THEN** (`large_payloads_travel_in_chunks_both_ways`) both sends complete without error, each side receives the other's bytes unchanged (the second side reversed), and the metadata `{"name":"big.bin"}` arrives with the first

### Requirement: Actions: Receiving transfers
A room SHALL accept an offered transfer from a peer only for an action that has a receiver, one transfer per peer at a time, and only while the bytes it has reserved across all peers plus the offered size stay within `room_options::max_receive_bytes`; it replies `accepted`, assembles the chunks into one buffer that becomes the delivered message (no copy for `pmr_bytes`), and releases the reservation when the transfer ends. An offer larger than `max_receive_bytes` is answered `rejected` with the reason `payload exceeds receiver size limit`, an offer when the peer already has 64 pending items with `too many pending transfers`, and a transfer that receives no accepted chunk for 120 s is dropped and answered `rejected` with `action transfer timed out`. A `cancelled` frame from the sender releases the transfer.

#### Scenario: An offer above the limit
- **WHEN** `max_receive_bytes` is 1,000,000 and a peer offers a 2,000,000-byte payload
- **THEN** the room answers `rejected` and the sender's send completes with `error::transfer_rejected`

### Requirement: Actions: Protocol violations drop the peer
The room SHALL drop a peer (send it `@_leave` and remove it) that breaks the action wire: an action frame that does not decode (`invalid action frame`), a second offer with an id already in use (`duplicate action offer`), a chunk of a transfer that was not accepted, with an offset other than the bytes already received, empty, or whose size is not `min(chunk_size, remaining)` (`invalid action chunk offset or length`), or a control frame the room cannot send back (`unable to send action control`); a chunk of an unknown or cancelled transfer is ignored.

#### Scenario: A chunk with the wrong offset
- **WHEN** a peer sends a chunk whose offset is not the number of bytes received so far
- **THEN** the room drops the peer

### Requirement: Actions: Completion tokens and ownership
Every asynchronous operation of the library (`async_send`, `async_receive`, `async_ping`, `async_leave`) SHALL be an Asio initiating function (`boost::asio::async_initiate`) that works with a callback, `boost::asio::use_awaitable`, `boost::asio::deferred`, `boost::asio::detached`, `boost::asio::as_tuple` and any other completion token. Because a deferred operation starts after the call, arguments are owned: `async_send` takes its value by value (moved when the type owns its storage, copied once into the room's resource when it is a view, serialized at once when it is a `boost::json::value`), `send_options` owns its strings, and `async_ping` copies the peer id; a callback is called on the room's executor, and a handler's associated allocator is used for what carries it.

#### Scenario: A send with detached
- **WHEN** `a.make_action<std::string_view>("note").async_send(std::string_view("..."), asio::detached)` is called
- **THEN** the message is sent and the result is discarded

### Requirement: Actions: Features not yet ported
The library SHALL NOT report the progress of a transfer, let the receiver decline an incoming transfer, answer a message with `request`/`onRequest`, or cancel a send with an `AbortSignal`; a room accepts every transfer for an action with a receiver within `max_receive_bytes`, as Trystero does when no `onReceive` is set, and reports a transfer only when it completes.

#### Scenario: A large file is received without progress events
- **WHEN** a peer receives a 100,000-byte transfer
- **THEN** the handler is called once, after the last chunk, with the whole payload

### Requirement: Signaling: The Nostr strategy
The strategy (`detail::nostr_strategy`) SHALL open one WebSocket per relay and, as each opens, send a `REQ` with a subscription id of 64 random characters of `protocol::id_alphabet` (kept across reconnections of that relay) for the room's root topic and the peer's own topic, then announce. Every event it publishes is signed (BIP-340) with a secret key drawn from the backend's `random` once per room, retrying until the key is valid, and carries the topic and the content; an event it receives is passed to the signaling only if its subscription is the relay's own, its topic is one of the room's two topics and its id has not been seen. `stop()` stops announcing and reconnecting, sends `CLOSE` on each open subscription and closes each socket.

#### Scenario: Two relays carry the same event
- **WHEN** a signal reaches the room on both relays
- **THEN** the signaling sees it once, because the strategy remembers the last `nostr_strategy::remembered_events` (4096) event ids and drops a repeated one

### Requirement: Signaling: Which peer offers
Of two peers the one with the lower id SHALL offer, on the relay that carried the other's announcement; the higher answers an announcement by announcing itself on the other's peer topic so that the lower one offers. A peer that already answers a peer (or has answered) ignores that peer's announcements, and a higher-id peer that already has an offer to a peer does not counter-announce to it again. The ids compared are the plain string order of the two ids, and a signal carrying the room's own id is ignored.

#### Scenario: A below-z offerer
- **WHEN** rooms `AAAAAAAAAAAAAAAAAAAA` and `zzzzzzzzzzzzzzzzzzzz` meet
- **THEN** `A...` offers and `z...` answers (`two_peers_join_and_exchange_every_payload_format`)

### Requirement: Signaling: Counter-announcements are paced
When a room whose id is higher sees an announcement from a lower id it SHALL publish an announcement on that peer's topic (asking it to offer) at most once per second per peer and relay and at most once per 5333 ms per relay for the whole room (Trystero's `announceIntervalMs`), logging `announcement from <peer>: their id is lower, asking them to offer`; the peers not asked offer at the room's next announcement.

#### Scenario: A flood of lower ids
- **WHEN** 300 fresh ids below the room's `zzzzzzzzzzzzzzzzzzzz` announce within a few milliseconds
- **THEN** (`counter_announcements_are_paced`) the room logs at least one and at most `1 + elapsed / 5333 ms` counter-announcements

### Requirement: Signaling: Offers and their retries
The signaling SHALL create, for each peer it offers to, one connection and a data channel named `data`, seal the offer's SDP, publish it as a signal with a 12-character offer id (`protocol::offer_id_size`) on every relay that carried the announcement, then candidates trickled after it, and repeat the offer on each relay twice: after 2000 ms and after 4800 ms more (three sends in all), unless the peer has answered. An offer that gets no answer expires after 57,333 ms (`offer_ttl`); after an answer is applied the connection has 23,333 ms to open. Expiry closes the connection and drops the negotiation if nothing is left.

#### Scenario: An unanswered offer
- **WHEN** the peer never answers
- **THEN** the offer is sent on the relay at 0, 2000 and 6800 ms, and the negotiation is reset at 57,333 ms with the log `offer to <peer> expired`

### Requirement: Signaling: Answering an offer
The signaling SHALL answer a peer's offer only after the sealed offer opens with the room key: it makes a connection, applies the SDP as its remote description, creates and seals the answer, publishes it with the offer's id on every relay that carried the offer, and then its candidates; if it had an offer to the same peer and its id is higher, that offer is reset; if its id is lower, the incoming offer is ignored ("we lead"). The same offer arriving again on a relay is answered again by replaying the stored answer messages, at most once a second per relay. An answering connection that does not open within 23,333 ms is closed.

#### Scenario: A duplicate offer
- **WHEN** the same offer arrives twice within a second on one relay
- **THEN** the answer is republished once

### Requirement: Signaling: Candidates
The signaling SHALL seal each local ICE candidate and publish it under the offer id of its connection, and open each remote candidate before using it: a candidate is applied to the connection its offer id names (the one it answers, else the one it offered, when none is named), is kept until that connection has a remote description (at most 128 per side), and a candidate the backend refuses is dropped and logged `dropped a candidate from <peer>`; candidates gathered before the offer or answer was sealed are published after it.

#### Scenario: A candidate arrives early
- **WHEN** a remote candidate arrives before the offer is applied
- **THEN** it is held and added right after the remote description is set

### Requirement: Signaling: Nothing changes before a signal opens
A signal SHALL change no negotiation until it opens with the room key: an offer, an answer or a candidate that does not open leaves the negotiation as it was, so a forged answer sent in a peer's name cannot reset an offer, and a forged offer makes no connection. Signals are sealed with the key `protocol::seal_key(password, app_id, room_id)`; the signaling reads fields that are bounded in `protocol/signal.hpp`.

#### Scenario: 200 unsealed offers
- **WHEN** an attacker publishes 200 offers `not a sealed offer` from fresh ids on a room's topic
- **THEN** (`forged_offers_make_no_connection`) the backend has made no connection

#### Scenario: A forged answer
- **WHEN** a watcher answers every offer of `A` to `B` with `not a sealed answer` in `B`'s name before `B`'s own answer
- **THEN** (`a_forged_answer_changes_nothing`) `A` and `B` still join, and `A` reports the failed open at most once

### Requirement: Signaling: Failed opens are reported at a bounded pace
A sealed signal that does not open SHALL be counted, logged once per negotiation (`could not open the <offer|answer|candidate> from <peer>: another password, or forged in its name; ignored`) and reported to `on_error` as `error::wrong_password`: the first of a quiet interval of 10 s at once, with its peer and the error's own message, and the others of the interval in one report when it ends, with an empty peer and a description that begins with their number (`<n> more peers sent signals that did not open with the room key, in the last 10 s: another password, or forged in their names`).

#### Scenario: Two thousand garbage offers
- **WHEN** 2000 unsealed offers arrive from fresh ids `0garbage0` to `0garbage1999` with the clock 10 times faster
- **THEN** (`signals_that_do_not_open_are_reported_once_per_interval`) the first report names `0garbage0`, the counts of the reports add up to 2000, and there are at most `2 + elapsed / 10 s` reports

### Requirement: Signaling: The replay cache
`detail::replay_cache(capacity, memory)` SHALL remember the SHA-256 digests of the last `capacity` opened texts it was given, a ring that overwrites the oldest first: `insert(opened)` remembers a text, `contains(opened)` answers whether it is among the remembered, and a capacity of 0 remembers nothing. The digest is of the opened text, not the sealed one, whose encoding a resender can vary. The signaling gives it `signaling::remembered_descriptions` (1024), at least twice `max_connections_per_window`.

#### Scenario: A cache of three
- **WHEN** `one`, `two`, `three`, `four`, `five` are inserted in a cache of capacity 3
- **THEN** (`the_replay_cache_remembers_the_last_ones`) after `four` the cache no longer contains `one` and contains `two`, `three`, `four`; after `five` it no longer contains `two`; a cache of capacity 0 never contains an inserted text

### Requirement: Signaling: Replayed and reflected offers and answers are refused
The signaling SHALL remember every offer and answer it made or acted on, and SHALL refuse an offer or answer that opens with the room key but comes to it as a peer's after that: a recorded answer resent in the peer's name, a recorded offer resent, or the room's own offer or answer sent back; the refusal changes nothing, is counted, logged once per negotiation (`refused the <offer|answer> from <peer>: the room made or used it before, so someone sent it again, maybe in its name`) and is not reported to `on_error`.

#### Scenario: A replayed answer
- **WHEN** a watcher records `B`'s sealed answer to `A`, `B` leaves and returns with the same id, and the watcher sends the recorded answer in `B`'s name with each new offer
- **THEN** (`a_replayed_answer_is_refused`) `B` joins again and `A` reports no `error::wrong_password`

#### Scenario: A reflected offer
- **WHEN** a watcher sends `A`'s own offer to `B` back to `A` as `B`'s answer
- **THEN** (`a_reflected_offer_is_refused_as_an_answer`) `A` refuses it and `B`'s own answer joins them

#### Scenario: A reflected answer
- **WHEN** `A` (`MMMMMMMMMMMMMMMMMMMM`) answered `B` and offers to `C` (`ZZZZZZZZZZZZZZZZZZZZ`), and a watcher sends `A`'s answer to `B` back as `C`'s answer
- **THEN** (`a_reflected_answer_is_refused_as_an_answer`) `A` logs `refused the answer from ZZZZZZZZZZZZZZZZZZZZ` and `C` joins

### Requirement: Signaling: Bounded negotiations
The signaling SHALL hold at most `signaling::max_unconnected_peers` (32) negotiations with peers it is not connected to, each with at most two pending connections: a new peer beyond the bound takes the place of a negotiation that holds no connection and has been idle longest, else is refused and logged once (`refusing <peer>: 32 negotiations are under way, the next announcement may find a slot`). Refused ids are remembered (64 of them) so that a peer refused again and again is logged once. A negotiation is dropped when its offer and answer expire without a connection.

#### Scenario: A flood of announcements
- **WHEN** 40 fresh ids (the bound plus 8) are announced to a room whose id is lower than all of them, and then one id is announced five times
- **THEN** (`a_refused_peer_is_logged_once`) nine `refusing` lines are logged, one of them for `1same`, and no more than `2 * 32` connections are live

### Requirement: Signaling: Bounded connections
The signaling SHALL make at most `signaling::max_connections_per_window` (64, twice `max_unconnected_peers`) connections in any window of 57,333 ms, counted from when they are made (`detail::construction_window(limit, window, memory)`: it keeps the times of the last `limit` constructions and allows another only when the oldest is a window old), so that a place freed early by a connection that failed or closed at once is not taken again until the window has passed; beyond it the negotiation gets no connection and is logged once per peer (`refusing <peer>: 64 connections were made in the last 57333 ms, a later announcement may find one`).

#### Scenario: Five hundred offers that fail at once
- **WHEN** a member of the room that holds the key sends 500 sealed offers, each of a different text, whose SDP every connection refuses
- **THEN** (`sealed_offers_that_fail_at_once_are_bounded`) the backend makes at most 64 connections and every other offer is refused: connections made plus refusals equal 500

### Requirement: Signaling: Flood resistance
A flood of announcements, forged answers and unsealed offers SHALL leave a joined room working: the peer it is connected to keeps working, the page's connections stay within `2 * max_unconnected_peers` more than before the flood, and the memory the room holds after a flood twice as large is no more than half again what a flood of `remembered_events + 1000` left; once the offers to the flooders expire their negotiations are gone and a new peer joins.

#### Scenario: A flood of 5096 announcements
- **WHEN** an attacker announces 5096 fresh ids below a joined room's id, and then twice as many
- **THEN** (`a_flood_of_announcements_is_bounded`) the room still delivers `after the flood` to its peer each time, the peak of live connections stays within `2 * 32` of what was held, the late peer `CCCCCCCCCCCCCCCCCCCC` joins after the first flood, and the second flood leaves at most 1.5 times the memory the first left

#### Scenario: Forged answers to announcements
- **WHEN** a watcher announces 300 ids above the room's and answers each of its offers with garbage
- **THEN** (`announcements_with_forged_answers_are_bounded`) the room makes no more than 32 connections and refuses the others

### Requirement: Signaling: The end of a negotiation
A negotiation SHALL end when its data channel opens: the signaling hands the connection and the channel to the app, which adopts them (see "Shared: Adoption of a negotiated connection"), resets both sides, and ignores signals from that peer while the room is bound to it (`forget`, when the binding ends and no live connection to that peer remains, makes its next announcement start a new negotiation). A room bound to a peer keeps its negotiation entry for that peer, marked connected, so that a bound room starts no negotiation with it. A connection that fails or closes before the channel opens is reset and logged (`the connection we offered to <peer> failed`, `the connection we answered for <peer> failed`); `close()` ends every negotiation and ignores every signal from then on. A backend that makes no connection or no channel is reported as `error::backend_failure` for that peer, and the next announcement starts another negotiation.

#### Scenario: A refused connection is retried
- **WHEN** the first peer connection is refused by the memory backend
- **THEN** a later negotiation connects and the peers join

### Requirement: Signaling: Peer links and the room envelope
A connected peer SHALL be a `detail::connection` of the app, which replaces `detail::peer_link` and is shared by the bindings of the rooms that use it: it sends a presence frame for each of the app's rooms as soon as it starts, hands every action frame of a bound room's token (with the frame it arrived in) to that room's binding, keeps or drops a frame of another token (see "Shared: Routing by room token"), and ends the connection (reported once, never to a room twice) on a frame that is not a valid envelope (`invalid or incompatible room frame`), when the data channel closes (`data channel closed`), and when the connection state is `failed` (`connection failed`) or `closed` (`connection closed`). A presence frame that says a room is absent ends that room's binding only (`peer left the room`), not the connection.

#### Scenario: A frame for another room
- **WHEN** a frame carrying the token of another room of the app, not bound to this connection, arrives on a peer's channel
- **THEN** it is kept for that room, up to 64 frames and 10 s, and the connection stays open; a frame carrying a token no room of the app has is dropped and the connection stays

### Requirement: Signaling: A disconnected peer has a grace period
A `connection` SHALL treat a connection state of `disconnected` as the end only after 5000 ms (Trystero's five seconds), cancelling the wait if the state recovers, and then end it with the reason `connection lost`. Waiters for the channel to drain are released with `false` when the connection ends or their binding detaches, and with `true` as soon as the channel buffers no more than 64 KiB.

#### Scenario: A brief disconnection
- **WHEN** the connection is `disconnected` and returns to `connected` within 5 s
- **THEN** the peer stays joined

### Requirement: Native: The native backend's header, namespace and layering
The library SHALL ship the native backend as the headers `native/backend.hpp`, `native/peer_connection.hpp`, `native/data_channel.hpp`, `native/sealer.hpp` and `native/websocket.hpp` under `include/webcpp/trystero/`, all in the namespace `webcpp::trystero::native`, each including `<webcpp/trystero/config.hpp>` first; `native/backend.hpp` MUST include the other four. The native backend implements the backend interface of `backend.hpp` over libdatachannel for WebRTC, Boost.Beast for the relays' WebSockets, OpenSSL for TLS and AES-GCM and Asio's `steady_timer` for timers, and builds only natively. The core (`<webcpp/trystero.hpp>` and the headers directly under `trystero/`, `protocol/` and `detail/`) MUST NOT include it, libdatachannel, OpenSSL or Emscripten, which `meta/include-boundaries.json` states with the reason "the core builds on both targets, without libdatachannel, OpenSSL or Emscripten".

#### Scenario: A native program includes one header
- **WHEN** a native program includes `<webcpp/trystero/native/backend.hpp>` and `<webcpp/trystero/room.hpp>`
- **THEN** it can construct `trystero::native::backend backend(io.get_executor())` and a `trystero::room` over it, as `example/native_quick_start.cpp` does

#### Scenario: The core stays free of the platform
- **WHEN** a header matching `include/webcpp/trystero/*.hpp`, `protocol/*.hpp` or `detail/*.hpp` includes `webcpp/trystero/native`, `webcpp/trystero/browser`, `rtc/`, `emscripten/` or `openssl/`
- **THEN** the lint fails it, per the single boundary of `meta/include-boundaries.json`

### Requirement: Native: Exceptions are needed by the WebRTC types and by nothing else
The native backend's `native::data_channel`, `native::peer_connection` and `native::backend` (with `native::backend_config`) SHALL exist only when `WEBCPP_TRYSTERO_NO_EXCEPTIONS` is not defined, each behind `#ifndef WEBCPP_TRYSTERO_NO_EXCEPTIONS`, because libdatachannel reports errors by throwing and they catch what it throws and complete with `error::backend_failure`. Without exceptions, or with `WEBCPP_NO_EXCEPTIONS` or `WEBCPP_TRYSTERO_NO_EXCEPTIONS` defined, `data_channel.hpp` and `peer_connection.hpp` declare nothing and code that uses the three types does not compile; `native::timer`, `native::sealer`, `native::websocket_limits`, `native::connect_websocket` and everything else of trystero are unchanged and compile without exceptions. trystero raises no exception of its own; the one function of the native backend that throws is the constructor of `native::backend`, a `boost::system::system_error` when OpenSSL cannot make the TLS context or load the system's certificate authorities. Each program and test that uses the native WebRTC types declares `<exception-handling>on` itself.

#### Scenario: The timer, sealer and WebSockets compile without exceptions
- **WHEN** `test/Jamfile` compiles `trystero/native/backend.hpp`, `trystero/native/sealer.hpp` and `trystero/native/websocket.hpp` alone against `/webcpp/trystero//native` with exceptions off
- **THEN** each compiles, and the lint analyses the timer, the sealer and the WebSockets without exceptions

#### Scenario: The WebRTC headers are compiled with exceptions whatever the build asks
- **WHEN** `test/Jamfile` compiles `trystero/native/data_channel.hpp` and `trystero/native/peer_connection.hpp` alone
- **THEN** the call declares `<exception-handling>on`, and the lint names those two headers as left out of its analysis without exceptions

### Requirement: Native: Events of libdatachannel's threads are posted to the executor
The native backend SHALL never run a handler on a thread of libdatachannel: each event (a local description, a local candidate, a data channel, a state change, an open, a message, a close, a drained buffer) copies what it carries and is posted to the room's `boost::asio::any_io_executor` through a `std::weak_ptr` of the object, so that an event arriving after the object is gone does nothing, and every handler and completion runs on the executor, after the call that caused it has returned. An application that runs the `io_context` on several threads MUST give the room a strand. A received data channel message is copied once, out of libdatachannel's `std::vector<std::byte>` into the room's resource, on the executor.

#### Scenario: Every handler runs on the io_context's thread
- **WHEN** `native_peer_connection` connects two peer connections bound to `127.0.0.1` in one process, opens a data channel and exchanges frames
- **THEN** no ice-candidate, data-channel, state, open, message or close handler runs off the thread that called `run_for` (`BOOST_TEST(!p.off_thread)`)

#### Scenario: A late event for a destroyed object does nothing
- **WHEN** libdatachannel posts an event for a `native::data_channel` or `native::peer_connection` whose wrapper has been destroyed
- **THEN** the weak reference does not lock and no handler runs

### Requirement: Native: The timer never fires a wait that was cancelled, restarted or destroyed
`native::timer` (a `backend::timer`, constructed from an executor and a `std::pmr::memory_resource*`) SHALL call `fire` on the executor once `delay` has passed, and MUST NOT call it after `cancel()`, a later `start()` or the destruction of the timer, even when Asio had already queued the expiry as a success. Each wait carries its number from a counter shared through a `std::shared_ptr<std::uint64_t>`, `cancel()` and `start()` change it, and the handler compares before it fires; the waits are allocated in the room's resource through `boost::asio::bind_allocator`.

#### Scenario: A wait that is not cancelled fires once
- **WHEN** `native_timer` starts a timer at 1 ms and runs the io_context for 100 ms
- **THEN** `fired` is 1 (`an_uncancelled_wait_fires_once`)

#### Scenario: A wait cancelled after its expiry was queued never fires
- **WHEN** two timers both started at 0 ms have expired when the io_context first looks, and the first one's handler calls `second->cancel()`
- **THEN** the second never fires (`a_cancelled_wait_never_fires`, fired is 0), and likewise when the first handler destroys the second timer (`a_destroyed_timer_never_fires`)

#### Scenario: A restarted wait fires only its new wait
- **WHEN** the first handler calls `second->start(10ms, ...)` after the second's 0 ms expiry was queued
- **THEN** the old handler never runs and the new one runs once (`a_restarted_timer_fires_only_its_new_wait`: fired 0, restarted 1)

### Requirement: Native: The sealer is AES-256-GCM in Trystero's sealed text form
`native::sealer` (a `backend::sealer`, constructed from an executor, the raw 32-byte key `protocol::seal_key` returns and a memory resource) SHALL seal with AES-256-GCM, OpenSSL's `EVP_aes_256_gcm`, under a fresh 16-byte IV from `RAND_bytes` (`protocol::seal_iv_size`), the ciphertext followed by its 16-byte tag as Web Crypto's `encrypt` returns it, written by `protocol::format_sealed`; and `open` SHALL parse with `protocol::parse_sealed` and decrypt, so that it opens what Trystero JS sealed with Web Crypto. It works synchronously and posts each completion to the executor, never calling `done` inside the call; a seal completes with `error::backend_failure` when OpenSSL fails, and an open completes with `error::wrong_password` when the text was sealed under another key, fails its tag, is not a sealed text, or has an empty IV. The completion's text is a view valid for the call. It needs no exceptions.

#### Scenario: It opens what Trystero JS sealed
- **WHEN** `native_sealer` opens `sealed.text` of `fixtures/expected/expected.json` with `tp::seal_key("secret", "my-app", "room-1")`
- **THEN** the completion has no error and its text equals `sealed.plaintext`

#### Scenario: It seals in Trystero's form and opens it back
- **WHEN** it seals `{"candidate":"candidate:1 1 udp 2122260223 192.0.2.1 54321 typ host"}` under `seal_key("", "app", "room")`
- **THEN** `parse_sealed` finds an IV of `seal_iv_size` bytes and a ciphertext of `plaintext.size() + 16` bytes, and opening the sealed text returns the plaintext (`seals_in_trystero_form_and_opens_it_back`)

#### Scenario: Another key or a stranger's text is a wrong password
- **WHEN** a sealer under `seal_key("guess", "my-app", "room-1")` opens the fixture's text, and the right sealer opens `not sealed text`
- **THEN** both completions carry `error::wrong_password` (`another_key_is_a_wrong_password`)

### Requirement: Native: connect_websocket reaches a relay over Boost.Beast
`native::connect_websocket(ex, tls, url, handlers, memory, limits = {})` SHALL return a `std::shared_ptr<backend::websocket>` that connects to a Nostr relay: `ws://` in the clear and `wss://` over TLS with `tls` (a `boost::asio::ssl::context` that must outlive the connection), with SNI set to the host and `boost::asio::ssl::host_name_verification` as the verify callback. A coroutine owns the session: it resolves, connects with a 30 s expiry, handshakes TLS for `wss`, performs the WebSocket handshake with the `Host` header carrying the port only when it is not the scheme's default (443 or 80), reports `open`, and then reads in a loop, handing each text to `message` as a `std::string_view` of the read buffer. A URL it cannot parse reports a close, as an unreachable relay does; `ws.read_message_max` is `protocol::max_relay_message_bytes` (1 MiB), so a longer message fails the read and closes the connection as the browser backend does. The session, its handlers and its queued texts are allocated in the room's resource; Beast's stream state, the resolver's results and the coroutines' frames are Beast's and Asio's. It needs no exceptions.

#### Scenario: Open, in-order echo, one close
- **WHEN** `native_websocket` connects to its echo server on 127.0.0.1, sends `one`, `two` and `["EVENT",{"content":"three"}]`, then calls `close()` twice
- **THEN** `open` fires, the three texts come back in that order, and `closes` is exactly 1 (`opens_sends_in_order_receives_and_closes_once`)

#### Scenario: A message longer than a relay sends closes the socket
- **WHEN** the echo server returns a text of exactly `max_relay_message_bytes` and then one of `max_relay_message_bytes + 1`
- **THEN** the first is delivered (its size is `max_relay_message_bytes`), the second is never handed over, and the socket reports one close (`a_message_longer_than_a_relay_sends_closes_the_socket`)

#### Scenario: An unreachable relay closes without opening
- **WHEN** the URL names a port on 127.0.0.1 where nothing listens
- **THEN** `closes` becomes 1 and `opened` stays false (`an_unreachable_relay_closes_without_opening`)

### Requirement: Native: WebSocket URLs are parsed by detail::parse_url
`native::detail::parse_url(url, memory)` SHALL return a `std::optional<url_parts>` for a `ws://` or `wss://` URL and nothing for any other scheme or an empty host or port. `url_parts` holds `secure` (true for `wss`), `host`, `port` (443 for `wss` and 80 for `ws` when the URL names none; an IPv6 literal's bracket is not taken for a port separator) and `target` (the path and query, `/` when the URL has neither, and a leading `/` inserted before a query that starts the target), all `std::pmr::string` in the given resource.

#### Scenario: A secure URL with no port or path
- **WHEN** `parse_url("wss://relay.example.org")` is called
- **THEN** it returns `secure` true, host `relay.example.org`, port `443` and target `/`

#### Scenario: A plain URL with a port, path and query
- **WHEN** `parse_url("ws://127.0.0.1:7447/nostr?x=1")` is called
- **THEN** it returns `secure` false, host `127.0.0.1`, port `7447` and target `/nostr?x=1`

#### Scenario: Other schemes and empty hosts are refused
- **WHEN** `parse_url("https://example.org")` and `parse_url("wss://")` are called
- **THEN** both return no value (`urls_name_a_scheme_host_port_and_path`)

### Requirement: Native: WebSocket writes are queued one at a time and a connection ends once
The native WebSocket SHALL queue the texts it is given (copied once, into a `std::pmr::deque<std::pmr::string>` of the room's resource) and write them one at a time, as Beast requires; a text given while the socket is not open or is closing is dropped. `close()` closes with a WebSocket close frame (a connection still being made is dropped, and its run ends with an error), is idempotent, and lets a write already queued finish first; the handler `close` is called at most once, when the connection ends, whichever of the read and the write fails first. A connection that ends while a write is in flight clears the queue, and the write finishes afterwards on the text it moved out of the queue into its own frame, so that its completion never touches the cleared container. Destroying the returned handle abandons the session: nothing is reported any more, and the connection is dropped.

#### Scenario: Closing right after a send lets the write finish
- **WHEN** a socket sends `["CLOSE","subscription"]` and calls `close()` at once
- **THEN** one close is reported (`closing_right_after_a_send_lets_the_write_finish_first`)

#### Scenario: A dropped socket reports nothing
- **WHEN** the returned handle is destroyed right after `connect_websocket` returns, and the io_context runs for 200 ms
- **THEN** no open, message or close handler ran (`a_dropped_socket_reports_nothing`: events is 0)

#### Scenario: A connection that ends during a write closes once
- **WHEN** a hostile server answers the first message with a text of `max_relay_message_bytes + 1` bytes (`too_long_text`) or resets the TCP connection (`reset`) while the client has 200 texts of 64 KiB queued, repeated for 10 connections
- **THEN** each socket reports exactly one close, still 1 after the socket is destroyed and the io_context runs 50 ms more; the test is built with `_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`

### Requirement: Native: websocket_limits drops a relay that stops reading or answering
`native::websocket_limits` SHALL have two members: `max_queued_bytes`, default `protocol::max_queued_relay_bytes` (4 MiB), and `idle_timeout`, a `std::chrono::seconds` default 30 s. A text that would take the queued bytes beyond `max_queued_bytes` drops the connection (the lowest layer is closed, the read fails, and `close` is reported once). The WebSocket sets Beast's suggested client timeouts with `idle_timeout` replaced and `keep_alive_pings` true: half of the idle timeout a ping is sent, and a relay that answers nothing within the whole of it is closed, which reports a close. The timeout counts bytes received and a ping waits for the write in flight. The browser backend has no idle timeout, since a page's `WebSocket` cannot ping.

#### Scenario: A relay that stops reading is dropped at the queue bound
- **WHEN** `a_relay_that_stops_reading_is_dropped_at_the_queue_bound` sends texts of 64 KiB (up to 2000) to a server that never reads after its handshake
- **THEN** `closes` becomes 1 before 2000 texts are sent, and is still 1 after 100 ms more

#### Scenario: A relay that stops answering times out
- **WHEN** a stalled server holds the connection and `idle_timeout` is 1 s
- **THEN** a close is reported within 10 s (`a_relay_that_stops_answering_times_out`)

#### Scenario: A quiet but healthy relay stays connected
- **WHEN** an echo server sends nothing for 3 s with `idle_timeout` 1 s and then a text is sent
- **THEN** no close is reported and `still there` is echoed back (`a_quiet_relay_that_answers_pings_stays_connected`)

### Requirement: Native: The peer connection wraps libdatachannel with negotiation driven by the signaling
`native::peer_connection::create(ex, servers, bind_address, memory)` SHALL return a `std::shared_ptr<native::peer_connection>` (a `backend::peer_connection`, constructible only through `create`) over an `rtc::PeerConnection` with `disableAutoNegotiation` set, so that an offer or an answer exists only when `set_local_description` asks for one, as in the browser. It adds each URL of each `backend::ice_server` to the configuration, rewriting `turn:host` with a username and credential as `turn:user:credential@host`, the form libdatachannel parses, and skips a URL libdatachannel cannot parse. A non-empty `bind_address`, as `"127.0.0.1"`, makes the connection gather on that address alone, so that two peers of one machine connect with no other network; an empty one gathers a host candidate on each interface but the loopback. `create` throws what libdatachannel throws when it cannot make the connection. The `rtc::PeerConnection` object and its control block are made with `std::allocate_shared` in the resource it is created with, the app's; `native_handle()` returns that `std::shared_ptr<rtc::PeerConnection>`, which no copy may outlive `app_config::memory`. The destructor resets the callbacks and closes the connection without reporting the close.

#### Scenario: Two connections open a data channel and exchange bytes
- **WHEN** `native_peer_connection` wires two connections bound to `127.0.0.1`: the offerer creates the channel `data`, `set_local_description` completes with type `offer`, the answerer applies it (`has_remote_description()` is true) and completes with type `answer`
- **THEN** both channels open, `is_open()` is true on both, the 4 bytes `{1,2,3,250}` arrive at the answerer, and `buffered_amount()` is at most `backend::data_channel::low_threshold`

#### Scenario: Closing one end is seen by the other
- **WHEN** the offerer calls `close()` once both channels are open
- **THEN** the answerer's channel reports exactly one close (`closing_one_end_is_seen_by_the_other`)

### Requirement: Native: Descriptions, candidates and states cross the peer connection as the browser's do
The native peer connection SHALL complete `set_local_description(done)` on the executor with the description's `typeString()` and SDP (queued in FIFO order, so each local description answers the oldest request), or with `error::backend_failure` when libdatachannel throws; `set_remote_description(type, sdp, done)` builds an `rtc::Description` and completes on the executor, with `error::backend_failure` when libdatachannel refuses it; `add_ice_candidate(json, done)` parses the JSON of an `RTCIceCandidateInit` (with the room's JSON resource), takes `candidate` and `sdpMid` (default `"0"`), adds none for the empty candidate that ends the remote gathering, and completes with `error::backend_failure` when the text is not JSON, has no string `candidate`, or libdatachannel refuses it. A local candidate is handed to `ice_candidate` as the JSON `{"candidate":<text>,"sdpMid":<mid>,"sdpMLineIndex":0}` with both strings escaped, the form Trystero JS adds with `addIceCandidate`. libdatachannel's states `New`, `Connecting`, `Connected`, `Disconnected`, `Failed` and `Closed` map to `connection_state::fresh`, `connecting`, `connected`, `disconnected`, `failed` and `closed`. `create_data_channel(label)` returns null when libdatachannel refuses, as on a closed connection.

#### Scenario: A malformed candidate fails without throwing
- **WHEN** `add_ice_candidate("not json", done)` is called on a fresh connection
- **THEN** `done` runs on the executor with `error::backend_failure` and nothing is thrown (`a_malformed_candidate_fails_without_throwing`)

#### Scenario: The answerer sees the connection states
- **WHEN** the two connections of the pair test connect
- **THEN** the answerer's `state` handler receives states mapped from libdatachannel's, on the executor's thread

### Requirement: Native: The data channel wraps an rtc::DataChannel and drops what is no frame
`native::data_channel::create(ex, channel, memory)` SHALL wrap an `rtc::DataChannel` (constructible only through `create`), set its buffered-amount-low threshold to `backend::data_channel::low_threshold` (`0xffff`, 64 KiB less one) and post its events to the executor. `is_open()` returns `isOpen()`; `send(frame)` sends a binary message only when the channel is open and ignores a refusal (a channel closed in between), whose close event reports the close; `buffered_amount()` returns `bufferedAmount()`; `close()` closes and ignores a refusal. A message that is text, as a peer can send, or binary and larger than `protocol::max_room_frame_bytes`, is dropped on libdatachannel's thread before it is posted, since neither can be a frame; every other message is copied once into a `backend::bytes` of the room's resource. The `close` handler is called at most once. The destructor resets the callbacks and closes the channel without reporting the close.

#### Scenario: One byte more than any room frame is dropped, the largest arrives
- **WHEN** the answerer sends `backend::bytes(protocol::max_room_frame_bytes + 1, 6)` and then `backend::bytes(protocol::max_room_frame_bytes, 7)`
- **THEN** the offerer receives exactly one message, of `protocol::max_room_frame_bytes` bytes

### Requirement: Native: backend_config bounds the connections a native backend makes
`native::backend_config` SHALL hold `bind_address` (a `std::pmr::string`, default empty), `max_peer_connections_per_minute` (default 256: the most peer connections the backend makes in any minute for every room it serves together, so that a room, which makes at most 64 in any 57 s, lets four rooms do so at once) and `relay_limits` (a `websocket_limits`, default-constructed). `native::backend::make_peer_connection(servers, memory)` SHALL return null when the backend already made `max_peer_connections_per_minute` in the last minute (a `detail::construction_window` of that limit and one minute, taking `now_ms()`), or when libdatachannel refuses to make one by throwing; otherwise it returns `peer_connection::create(ex, servers, config.bind_address, memory)`. The window counts what was made, not what is still held, so a closed and dropped connection still counts until the minute passes.

#### Scenario: The backend makes at most its budget in a minute
- **WHEN** `native_peer_connection` constructs a backend with `{.bind_address = "127.0.0.1", .max_peer_connections_per_minute = 3}` and calls `make_peer_connection({}, ...)` four times
- **THEN** the first three are not null and the fourth is null

#### Scenario: Closed and dropped connections still count
- **WHEN** the three connections are closed and destroyed and a fourth is requested
- **THEN** it is still null (`the_backend_makes_at_most_its_budget_in_a_minute`)

### Requirement: Native: The native backend is the factory of one application
`native::backend` SHALL derive from `trystero::backend::factory`, be constructed from an `any_io_executor` and an optional `backend_config`, and serve every room of the application beside that executor, which the application runs itself (blocking on the `io_context` or polling it). Its TLS context is a `boost::asio::ssl::context::tls_client` with `set_default_verify_paths()` and `verify_peer`, so relays are verified against the system's certificate authorities, and `tls_context()` returns it for an application with authorities of its own; a relay whose certificate does not verify is unreachable, and the room retries it later. `connect` calls `connect_websocket` with the config's `relay_limits`; `make_timer` returns a `native::timer`; `make_sealer` a `native::sealer`; `random` fills from `RAND_bytes` and calls `std::abort()` when OpenSSL has no secure randomness; `unix_seconds` is the system clock's whole seconds since the epoch; `now_ms` is the steady clock in milliseconds. Each object is made in the memory resource the room gives it with `std::allocate_shared`.

#### Scenario: A test replaces only the relays
- **WHEN** `native_room` derives `native_over_memory_relays` from `native::backend`, overriding `connect` to return the memory relays' socket
- **THEN** rooms over it use real libdatachannel connections, OpenSSL sealing and Asio timers with only the relays in memory

#### Scenario: Whole native rooms exchange every format
- **WHEN** two rooms with the password `s3cret` and ids `AAAAAAAAAAAAAAAAAAAA` and `zzzzzzzzzzzzzzzzzzzz` join `lobby` over the backend bound to `127.0.0.1`, and one sends the text `hello over libdatachannel`, the JSON `{"drink":"negroni"}` and 100,000 bytes with metadata `{"name":"big.bin"}`
- **THEN** the other receives all three unchanged and answers a ping without error (`native_rooms_join_and_exchange_every_format`)

#### Scenario: Another password never joins, and leaving is seen
- **WHEN** two rooms in `private` use the passwords `s3cret` and `guess`, and later a peer calls `async_leave`
- **THEN** neither lists a peer after 2 s and the guesser's `on_error` fired (`another_password_never_joins`); the other room's `on_peer_leave` fires once (`leaving_is_seen_over_a_real_connection`)

### Requirement: Native: Memory of the native backend
Every object the native backend makes for a room SHALL be allocated in the room's `std::pmr::memory_resource`: the WebSocket, its handlers and queued texts, the peer connection with its `rtc::PeerConnection` and control block, the data channels, the timers with their waits, the sealers with their texts and completions, and the frames and texts handed to the room. What libdatachannel and OpenSSL allocate, the `std::string` and `rtc::Description` libdatachannel's API takes and returns, Beast's and Asio's state inside the WebSocket (stream, resolver results, coroutine frames) and the posts from libdatachannel's threads (which carry Asio's default allocator, since the resource is touched only on the room's executor) stay theirs. The backend's own `construction_window` is allocated in `std::pmr::get_default_resource()`.

#### Scenario: A wrapper is shared-allocated in the room's resource
- **WHEN** `native::data_channel::create` or `peer_connection::create` is given a resource
- **THEN** the wrapper and its handlers are made with `std::allocate_shared` over `std::pmr::polymorphic_allocator<>(memory)` rather than `std::make_shared`

### Requirement: Browser: The browser backend's headers and target
The library SHALL ship the browser backend as `browser/backend.hpp`, `browser/event_loop.hpp`, `browser/js.hpp` and `browser/src.hpp` under `include/webcpp/trystero/`, in the namespace `webcpp::trystero::browser` (internals in `browser::detail`), building only on emscripten, over the page's own `WebSocket`, `RTCPeerConnection`, `RTCDataChannel`, `setTimeout` and `crypto.subtle` through embind's `emscripten::val`. It needs a secure context (https, or http on `localhost` or `127.0.0.1`), where `crypto.subtle` exists, no threads (`BOOST_ASIO_DISABLE_THREADS`, since Emscripten's threads need a cross-origin isolated page), and a program that lives as long as the page: the `event_loop` MUST outlive every room and object it serves. Its objects, their handlers and the frames and texts they hand to the room are allocated in the room's resource; what `emscripten::val` holds is in the JavaScript heap, and the loop keeps its registrations in the resource it is constructed with.

#### Scenario: The application a page keeps alive
- **WHEN** `example/quick_start.cpp` defines `struct application` with an `asio::io_context`, a `browser::event_loop loop{io}`, a `browser::backend backend{loop}` and a `trystero::room` over `loop.get_executor()`
- **THEN** the program `main` allocates it with `new` and returns at once, and the room lives as long as the tab

#### Scenario: Each header compiles alone on emscripten
- **WHEN** `test/Jamfile` compiles `trystero/browser/*.hpp` alone with `<library>/webcpp/trystero//trystero`
- **THEN** each compiles on the `emscripten` toolset and none is compiled natively

### Requirement: Browser: The event loop drives Asio from the page's events and never calls run()
`browser::event_loop(io, memory = std::pmr::get_default_resource())` SHALL hold an `executor_work_guard` on the `io_context` and drive it without ever calling `run()`: every JavaScript event that reaches C++ through `dispatch` runs its handler and then `io_context::poll()` (restarting a stopped context) when no other call into C++ is running (`depth() == 0`). Work given to `browser::executor` while no event is being handled is posted to the `io_context` and schedules one `poll()` in a `queueMicrotask`, unless one is pending or a poll is running. An event the page delivers while C++ is already running (depth above 1), as node's WebSocket firing `close` from inside `close()`, is posted instead of run, so that a handler never runs inside the call that caused it. The loop is not copyable, and its destructor marks every handler it made as dead by setting `alive` to false on its token.

#### Scenario: A callback posts work and then throws
- **WHEN** `browser_event_loop` runs a callback from a timer event that posts work to the loop's executor and then calls `JSON.parse("not JSON at all")`
- **THEN** the exception is reported and stops at the page's function, and the posted work still runs

#### Scenario: Recovery from a nested entry keeps the depth
- **WHEN** a callback with `loop.depth() == 1` calls `loop.recover(loop.depth())` and posts work
- **THEN** `depth()` is still 1 afterwards, and the posted work runs in the poll with depth 1

### Requirement: Browser: browser::executor is an Asio executor of the loop
`browser::executor`, constructed from an `event_loop&` and returned by `event_loop::get_executor()`, SHALL satisfy Asio's executor properties: `query(execution::context_t)` returns the loop's `io_context`, `query(execution::blocking_t)` returns `blocking.never`, `require(blocking.never)` returns a copy, and `execute(f)` posts `f` to the loop's `io_context` and calls `schedule_poll()`; two executors compare equal exactly when they belong to the same loop. A room in the browser is constructed with it, through `boost::asio::any_io_executor`.

#### Scenario: Work posted from outside an event runs without a polling loop
- **WHEN** a program posts to `loop.get_executor()` while no JavaScript event is being handled
- **THEN** one `poll()` is scheduled in a microtask and the work runs, with no polling loop and no `poll()` call by the application

### Requirement: Browser: Registrations connect JavaScript events to C++ callbacks
`event_loop::add(callback)` SHALL register a `handler<void(int slot, emscripten::val event)>` and return a unique registration id (starting at 1, never reused; id 0 is the loop's own poll); `remove(id)` erases it, so that a late event for a destroyed object finds nothing; `handler(id, slot)` returns a JavaScript function of one argument that delivers its argument to registration `id` through slot `slot` by calling `trystero_dispatch(token, loop, id, slot, event)`. The registration's entry is in the loop's resource while the callback stays in its own allocator's resource. `dispatch(id, slot, event)` delivers an event by id at the time it runs, calling the callback through a copy of its shared pointer so that it may remove itself.

#### Scenario: An event for a removed registration runs nothing
- **WHEN** an object removes its registration in its destructor and the page then fires an event through a handler made for it
- **THEN** nothing runs and nothing throws

### Requirement: Browser: Text crosses without an intermediate std::string
`event_loop::text(utf8)` SHALL return a JavaScript string decoded by the loop's `TextDecoder` straight from the WebAssembly memory a view points into (an `emscripten::typed_memory_view`). `event_loop::utf8(text, memory)` SHALL return the string's UTF-8 bytes, encoded by the loop's `TextEncoder`, copied straight into a `std::pmr::string` of `memory`, or an empty one when it cannot be encoded; the overload `utf8(text, memory, max_bytes)` returns `std::optional<std::pmr::string>` and nothing when the string's length in UTF-16 units, checked first, or its encoded length exceeds `max_bytes`, so that a longer string is never copied into WebAssembly.

#### Scenario: A relay text over the bound is not copied
- **WHEN** a relay message longer than `protocol::max_relay_message_bytes` arrives
- **THEN** `utf8(data, memory, protocol::max_relay_message_bytes)` returns no value and the socket closes

### Requirement: Browser: No JavaScript exception enters WebAssembly
The browser backend SHALL make every call into JavaScript that can throw (a constructor, `JSON.parse`, a `send`, a WebRTC or Web Crypto method) through `browser::detail::js_try`, `js_call(object, method, args...)` or `js_new(constructor, args...)` of `browser/js.hpp`, which run it inside `try` in the `EM_JS` function `webcpp_trystero_js_try` and return a `js_outcome{ok, value}`, the exception being the value when `ok` is false; the C++ turns that into the error its interface has. Every event of the page enters C++ through the function `webcpp_trystero_js_handler` makes, which saves `stackSave()` and the loop's depth, calls `Module.trystero_dispatch`, and on an exception restores the stack pointer, reports it with `reportError` (or `console.error` where there is none, as in node) and calls `Module.trystero_recover(token, loop, depth)`. With C++ exceptions, `browser::dispatch` catches a `std::exception` (or anything else) first, reports `what()` (or "an exception that is not a std::exception") through `detail::report_escape`, and recovers, so that the exception is freed. `event_loop::recover(depth)` sets the depth to `max(depth, 0)`, clears the poll-scheduled flag and schedules a poll. A program that uses the browser backend may be built with exceptions or without them.

#### Scenario: 3000 escapes leave the stack where it was
- **WHEN** `browser_event_loop` makes 3000 events whose callback throws from a function with a 4 KiB frame, with `console.error` counted, measuring `emscripten_stack_get_current()` before and after from one callback
- **THEN** the stack pointer is equal before and after, the loop still serves an event and runs posted work, and each escape was reported (before the fix each escape left 128 bytes of stack and the loop silently stopped after about 500)

#### Scenario: 20,000 C++ exceptions free their memory
- **WHEN** built with exceptions, 20,000 events whose callback throws `std::runtime_error(std::string(200, 'x'))` are delivered, `reportError` counting those whose message is 200 `x`
- **THEN** all 20,000 are reported with their `what()` and `mallinfo().uordblks` is the same after them as before (before the fix each escape kept 332 bytes)

### Requirement: Browser: src.hpp is included once and exports the three functions of the page
`<webcpp/trystero/browser/src.hpp>` SHALL be included in exactly one source file of a browser program. It defines, with `EM_JS` under `__EMSCRIPTEN__`, `webcpp_trystero_js_try`, `webcpp_trystero_js_handler` and `webcpp_trystero_js_census` (with `EM_JS_DEPS(webcpp_trystero, "$Emval,$stackSave,$stackRestore")`), and registers with `EMSCRIPTEN_BINDINGS(trystero_browser)` the embind functions `trystero_dispatch` (`browser::dispatch(token, loop, id, slot, event)`), `trystero_recover` (`browser::recover(token, loop, depth)`) and `trystero_depth` (`browser::depth(token, loop)`). Events for a destroyed loop are dropped: `dispatch` and `recover` return and `depth` returns 0 when the token's `alive` is false. Boost.JSON's definitions are not in it; a program links them from `/webcpp//boost_json`.

#### Scenario: A test program includes it once
- **WHEN** a browser test or `example/quick_start.cpp` includes `<webcpp/trystero/browser/src.hpp>` after the backend
- **THEN** `trystero_dispatch`, `trystero_recover` and `trystero_depth` are exported by embind, and the program links without `webcpp_trystero_js_*` undefined

### Requirement: Browser: The page's census of peer connections
`webcpp_trystero_js_census()` SHALL return one object per page, stored under `Symbol.for("webcpp.trystero.census")` on `globalThis` and shared by every program of the page, with `held` (the number of `RTCPeerConnection` objects counted from their construction until the garbage collector frees them, as Chromium counts them against its limit), `add(connection)` (counts one and registers it with a `FinalizationRegistry`), and `onrelease`, a function of the new count called after each release, for a test. Without `FinalizationRegistry` nothing is counted, so the page never stops making connections for a count that never comes down.

#### Scenario: A collection releases counted connections
- **WHEN** `browser_peer_connection` has made 256 connections that were closed and dropped and exposes node's `gc`
- **THEN** `census()["held"]` is 256 before the collection, and `onrelease` is called with a count below `max_peer_connections` after it, after which the backend makes a connection again

### Requirement: Browser: The timer is setTimeout
`browser::timer(loop, memory)` SHALL register with the loop, `start(delay, fire)` SHALL cancel any pending wait and call `setTimeout(loop.handler(id, 0), delay)`, `cancel()` SHALL `clearTimeout` and drop `fire` so that it is never called, the expiry clears the handle before calling `fire` (which may destroy the timer), and the destructor cancels and removes its registration.

#### Scenario: A cancelled timer's handler is never called
- **WHEN** a timer is started and then cancelled before the page's turn
- **THEN** `clearTimeout` is called with its handle and `fire` does not run

### Requirement: Browser: The WebSocket is the page's WebSocket and reports one close
`browser::websocket(loop, url, handlers, memory)` SHALL open a `new WebSocket(loop.text(url))` through `js_new`, setting `onopen`, `onmessage` and `onclose` to loop handlers of slots 0, 1 and 2. A URL the page's `WebSocket` refuses with an exception (one that does not parse, another scheme, a fragment, mixed content, a page policy) makes no socket, and the handler `close` is reported after the call, never inside it. `send(text)` drops the text unless `readyState` is 1; a text that would take `bufferedAmount` plus its size beyond `protocol::max_queued_relay_bytes` drops the socket: the three handler properties are set to null, `close()` is called, and the close is reported once after the call. A message that is a string is copied once with `utf8(data, memory, protocol::max_relay_message_bytes)`; a longer one closes the socket. A message that is not a string is ignored. The handler `close` is called at most once. The destructor removes the registration, nulls the handlers and closes without reporting.

#### Scenario: Each URL node's WebSocket makes or refuses reports one close
- **WHEN** `browser_websocket` connects to `ftp://relay.invalid`, `wss://relay.invalid/#fragment`, `not a url`, `/relative`, `http://127.0.0.1:9` and `https://127.0.0.1:9`, sends `ignored` and closes each
- **THEN** every URL reports exactly one close after the call (`closes` is 0 inside it), none opens, and nothing is thrown

#### Scenario: A relay that stops reading is dropped at the bound
- **WHEN** `browser_websocket_bound` sends 100 texts of 64 KiB (6.4 MiB) on a stand-in whose `bufferedAmount` only grows and whose close never completes
- **THEN** one close is reported at once, the stand-in never held more than `protocol::max_queued_relay_bytes`, and `close()` was called once

#### Scenario: A dropped socket runs no handler again
- **WHEN** `browser_websocket_late_events` drops a socket after 64 texts were sent, on a stand-in that has queued a message event for each text and a close event after `close()`, and a second socket sends one text of `max_queued_relay_bytes + 1` bytes
- **THEN** each socket reports one close, no message reaches the room after its close, and `lateCloses` is 2

### Requirement: Browser: The data channel drops what is no frame
`browser::data_channel(loop, channel, memory)` SHALL set `binaryType` to `arraybuffer` and `bufferedAmountLowThreshold` to `low_threshold`, route `onopen`, `onmessage`, `onclose` and `onbufferedamountlow` to slots 0 to 3, and report them through handlers kept in a `std::shared_ptr` that `set_handlers` replaces. `is_open()` is `readyState === "open"`; `send(frame)` copies the frame into a new `Uint8Array` and sends it through `js_call` only when open, dropping what the channel refuses; `buffered_amount()` is `bufferedAmount`; `close()` closes. A message is delivered only if it is an `ArrayBuffer` of at most `protocol::max_room_frame_bytes` bytes, copied once into a `backend::bytes` of the room's resource; a text message, or a larger one, is dropped before it is read. The `close` handler is called at most once.

#### Scenario: What is no frame is dropped before it is read
- **WHEN** `browser_data_channel` delivers `"1e300"`, `"300000000"`, an `ArrayBuffer` of 20,000 bytes and an `ArrayBuffer` of 3 bytes to the wrapper
- **THEN** only the last reaches the handler (`messages` is 1, size 3) and the largest allocation of the watching resource is under 1024 bytes (the text `1e300` once threw a `RangeError` out of `new Uint8Array`, and `300000000` made a 300 MB frame)

### Requirement: Browser: The peer connection wraps RTCPeerConnection and is refused at the page's limit
`browser::peer_connection::create(loop, servers, memory)` SHALL build a configuration with `iceServers` (each server's `urls`, and `username` and `credential` when not empty), make `new RTCPeerConnection(config)` through `js_new`, register the connection with the census, and return null when the browser refuses, as it throws for a server it cannot use or when the page holds as many connections as it allows. `browser::backend::max_peer_connections` SHALL be 256 (half of the 500 Chromium allows a page, counted the same way), and `backend::make_peer_connection` returns null without calling the constructor when the census's `held` is at least that. The connection routes `onicecandidate`, `ondatachannel`, `onconnectionstatechange` and `oniceconnectionstatechange` to slots 0, 1, 2 and 2. `create_data_channel(label)` returns null when `createDataChannel` throws; `set_local_description` calls `setLocalDescription()` and completes with the `localDescription`'s type and SDP, or `error::backend_failure` when rejected or the connection is gone; `set_remote_description` and `add_ice_candidate` (which first `JSON.parse`s the text and drops what it refuses) complete with `error::backend_failure` on rejection; `has_remote_description()` is `remoteDescription` not null; a candidate event with a null candidate is ignored and otherwise reaches `ice_candidate` as `JSON.stringify(candidate)`. The state it reports merges `connectionState` and `iceConnectionState`, by priority `failed`, `closed`, `disconnected`, `connected` (or ice `connected` or `completed`), `connecting` (or ice `checking`), else `fresh`. `native_handle()` returns the `emscripten::val`.

#### Scenario: A page without RTCPeerConnection makes none
- **WHEN** `browser_peer_connection` runs under node, which has no `RTCPeerConnection`, and calls `make_peer_connection({}, memory)`
- **THEN** it returns null, and with a constructor that throws a `DOMException` it still returns null and the census's `held` is 0

#### Scenario: A connection whose createDataChannel throws opens no channel
- **WHEN** the stand-in `RTCPeerConnection` has a `createDataChannel` that throws `InvalidStateError`
- **THEN** `create_data_channel("data")` returns null

#### Scenario: The page's budget
- **WHEN** one connection whose `createDataChannel` throws was made earlier and stays counted, then 256 requests are made and all that were made are closed
- **THEN** 255 are made (`budget - 1`), the next request is null, `held` equals 256, a request is still null, and once node's `gc` releases them the backend makes one again

### Requirement: Browser: The sealer is crypto.subtle's AES-GCM under a key imported once
`browser::sealer(loop, key, memory)` SHALL import the raw 32-byte key with `crypto.subtle.importKey("raw", ..., "AES-GCM", false, ["encrypt", "decrypt"])` on its first operation; an operation arriving while the key is importing waits in a queue of the room's resource with a copy of its text; an operation after a failed import completes after the call with `error::backend_failure`. `seal` encrypts with a fresh 16-byte IV from `getentropy` and writes `protocol::format_sealed`; `open` completes with `error::wrong_password` when the text is not a sealed text, has an empty IV, or fails to decrypt, and with `error::backend_failure` when the key cannot be imported. Every completion runs after the call that started it, never inside it. Without `crypto.subtle`, as outside a secure context, every operation fails with `error::backend_failure`.

#### Scenario: Completions run after the call
- **WHEN** `browser_sealer` seals `hello`, opens the sealed text and opens `not a sealed text`
- **THEN** the sealed text opens to `hello`, the stranger's text completes with `error::wrong_password`, and no completion has `inline_call` set (before the fix three completions ran inside the call)

#### Scenario: Without crypto.subtle every operation fails after the call
- **WHEN** `globalThis.crypto` is redefined to an empty object and a sealer seals `hello`, then opens `x$AAAA`
- **THEN** both complete with `error::backend_failure`, after their calls

### Requirement: Browser: The browser factory's services
`browser::backend(loop)` SHALL derive from `trystero::backend::factory` and make each object in the memory resource the room gives it: `connect` a `browser::websocket`, `make_peer_connection` a `browser::peer_connection` (or null as above), `make_timer` a `browser::timer`, `make_sealer` a `browser::sealer`. `random(out)` fills from `getentropy` in pieces of at most 256 bytes (drawn from `crypto.getRandomValues`), `unix_seconds()` is `floor(Date.now() / 1000)` and `now_ms()` is `performance.now()`. The backend needs a secure context.

#### Scenario: A large buffer is filled in pieces
- **WHEN** `random` is asked for more than 256 bytes
- **THEN** it calls `getentropy` repeatedly with at most 256 bytes each

### Requirement: Browser: The browser tests run under node with and without exceptions
`test/Jamfile` SHALL declare the seven browser tests, `browser_event_loop`, `browser_websocket`, `browser_websocket_bound`, `browser_websocket_late_events`, `browser_sealer`, `browser_data_channel` and `browser_peer_connection`, on emscripten only, under node (which has the page's `WebSocket`, Web Crypto and microtasks but no WebRTC), and the same seven again as `<name>_no_exceptions` with `<exception-handling>off`, since no exception may enter WebAssembly there either. Each `main` sets `process.exitCode` to 1, starts its first step, and ends with `boost::report_errors()` as the exit code; each step starts from the previous one's completion, and a 10 s deadline fails by name if the chain stops.

#### Scenario: The no-exceptions variant exists for every test
- **WHEN** `test/Jamfile` is read
- **THEN** each of the seven `browser_*` targets has a `browser_*_no_exceptions` target built from the same file with `<exception-handling>off`, both on `emscripten`

### Requirement: Build: build.jam declares the project and its two usable targets
`build.jam` SHALL declare `project /webcpp/trystero` and two aliases, between the markers `tag::trystero[]` and `tag::native[]` which the documentation page includes. `/webcpp/trystero//trystero` brings the rooms and the browser backend: its usage requirements are `<include>include`, `<library>/boost//headers`, `<library>/webcpp//boost_json` (Boost.JSON's definitions, compiled once per variant), `<library>/webcpp/config//config` and `<library>secp256k1`, and on emscripten `<define>BOOST_ASIO_DISABLE_THREADS` and `<linkflags>-lembind`. `/webcpp/trystero//native` brings the native backend: `<library>trystero`, `<library>datachannel`, `<library>ssl` and `<library>crypto`. The alias `trystero` carries `<conditional>@trystero-dependencies`, which checks libsecp256k1 only when a target that uses trystero is built, and `native` carries `<conditional>@native-dependencies`, which checks libdatachannel and OpenSSL. embind is linked as the flag `-lembind` rather than b2's `<embind>on`, because that feature is not free and b2 takes only free features from usage requirements (measured: a program then fails to link with undefined `_embind_register_function`). The native backend's `<exception-handling>on` is not a usage requirement, since b2 lets no usage requirement override a build's `exception-handling=off`; each target that uses it declares it.

#### Scenario: A program uses the rooms and the browser backend
- **WHEN** a b2 program names `/webcpp/trystero//trystero` on emscripten
- **THEN** trystero's headers, Boost's, libsecp256k1, Boost.JSON's definitions, webcpp's configuration, `BOOST_ASIO_DISABLE_THREADS` and `-lembind` reach its compile and link lines

#### Scenario: A native program names the native alias
- **WHEN** a native program names `/webcpp/trystero//native` with `<exception-handling>on`
- **THEN** libdatachannel, OpenSSL's `ssl` and `crypto` and everything of `trystero` reach its link line

### Requirement: Build: The three external libraries are found where b2's command line or .local says
`build.jam` SHALL find libsecp256k1 0.8.0 (with its `schnorrsig` and `extrakeys` modules; needed by every room), libdatachannel 0.24.6 and OpenSSL 3 (both needed by the native backend only) with webcpp's `webcpp.external`, between the markers `tag::dependencies[]` and `end::dependencies[]`: first the directory named by `-sSECP256K1_ROOT=<dir>` (natively), `-sSECP256K1_EMSCRIPTEN_ROOT=<dir>` (for the emscripten build of libsecp256k1), `-sLIBDATACHANNEL_ROOT=<dir>` or `-sOPENSSL_ROOT=<dir>` on b2's command line, a relative one read from b2's working directory, and never the environment; else the superproject's `.local/secp256k1-native`, `.local/secp256k1-emscripten`, `.local/libdatachannel` or `.local/openssl` when it holds the library's header (`secp256k1.h`, `rtc/rtc.hpp`, `openssl/ssl.h`); else, natively outside Windows, `/usr/include` or `/usr/local/include`. A `-s` that names a directory without the library's header MUST stop the build, naming it. Each root holds the library's `include/` and `lib/`; the library names are `secp256k1` (`libsecp256k1` on MSVC), `datachannel`, `ssl` and `crypto`, and libdatachannel's `lib` is also a `<dll-path>`.

#### Scenario: An explicit root wins
- **WHEN** b2 is run with `-sLIBDATACHANNEL_ROOT=/opt/dc` and `.local/libdatachannel` also exists
- **THEN** the build looks in `/opt/dc/include` and `/opt/dc/lib`, and does not read an environment variable

#### Scenario: A root without the header stops the build
- **WHEN** `-sOPENSSL_ROOT=<dir>` names a directory with no `include/openssl/ssl.h`
- **THEN** b2 stops, naming that directory

### Requirement: Build: A missing external library leaves trystero out, or stops the build when required
When no candidate holds a library, the build SHALL leave trystero out: its tests and examples are skipped, b2 says so once, naming the library and every place it looked, and every other library builds. With `webcpp-require-external=on`, as the continuous integration builds, the same condition MUST stop the build instead. The libraries are found only when a target that uses trystero (`trystero-dependencies`, for libsecp256k1) or the native backend (`native-dependencies`, for libdatachannel and OpenSSL) is built, through `webcpp.external-found`.

#### Scenario: libdatachannel missing, everything else present
- **WHEN** libdatachannel is found nowhere and a target that only uses `/webcpp/trystero//trystero` is built
- **THEN** it builds, since only the native alias checks libdatachannel

#### Scenario: The CI requires them
- **WHEN** the CI builds with `webcpp-require-external=on` and a library is found nowhere
- **THEN** the build stops, naming the library, instead of skipping trystero

### Requirement: Build: MSVC names and the static libsecp256k1
On MSVC the build SHALL use CMake's names: `libsecp256k1.lib` with `<define>SECP256K1_STATIC` in its usage requirements (a static libsecp256k1 on Windows declares every function imported from a DLL without it), and OpenSSL's `libssl.lib` and `libcrypto.lib`, also searched in `lib/VC/x64/MD` under the OpenSSL root, where the installer of Shining Light Productions puts them. The tests and the examples MUST add `<cxxflags>/bigobj` on MSVC (C1128: a test of rooms over Asio holds more sections than the default object format allows), and the tests add `<define>_CRT_SECURE_NO_WARNINGS` so that `std::getenv`, which `room_test.cpp` reads, is not an error under `/WX` (C4996).

#### Scenario: The MSVC library names
- **WHEN** `build.jam` is read for the `msvc` toolset
- **THEN** `secp256k1` resolves to `libsecp256k1` with `SECP256K1_STATIC`, `ssl` to `libssl` and `crypto` to `libcrypto`, with `lib/VC/x64/MD` as a second search path

### Requirement: Build: The libraries are built with the toolchain that builds the programs
libdatachannel MUST be built with the compiler and standard library that build the programs that link it (libc++ for clang on macOS, libstdc++ for GCC): a program built with GCC and `libstdc++` does not link against a libdatachannel built with `libc++`, though the native headers still compile alone. libdatachannel is built without its own WebSocket (`-DNO_WEBSOCKET=ON`, `-DNO_EXAMPLES=ON`, `-DNO_TESTS=ON`) with `OPENSSL_ROOT_DIR` set where needed; libsecp256k1 is built twice, natively and with Emscripten, from tag `v0.8.0` with `-DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON`, `-DSECP256K1_ENABLE_MODULE_EXTRAKEYS=ON`, `-DSECP256K1_ECMULT_WINDOW_SIZE=4` and `-DSECP256K1_ECMULT_GEN_KB=2`, whose small tables keep the browser's download small. The building page and the README show the commands, with Emscripten pinned at 6.0.11.

#### Scenario: The README's build steps
- **WHEN** `README.md` is read from "From an empty directory to a native peer and the chat"
- **THEN** its steps install the tools, create a workspace, install Emscripten 6.0.11 with emsdk, clone webcpp with its submodules, build libsecp256k1 twice, build libdatachannel v0.24.6, write `user-config.jam`, build `libs/trystero/example//native_quick_start-program` and `toolset=emscripten libs/trystero/example/chat//site`, and run them

### Requirement: Build: The library's continuous integration is the superproject's reusable workflow
`.github/workflows/ci.yml` SHALL run on pushes to `main`, on pull requests and on `workflow_dispatch`, with `permissions: contents: read` and a concurrency group `${{ github.workflow }}-${{ github.ref }}` whose `cancel-in-progress` is `${{ github.ref != 'refs/heads/main' }}` (a run on `main` is the record of main and is never cut short), and one job, `ci`, that calls `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: trystero`. The workflow builds this commit inside a checkout of webcpp, on every lane trystero declares and its own lanes, with libsecp256k1, libdatachannel and OpenSSL installed by its actions, its page, the lint and the test matrix.

#### Scenario: The workflow names the library
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** the job `ci` is named `CI`, uses the reusable `library.yml` and passes `library: trystero`

### Requirement: Build: The library's metadata, licence files and ignore rules
`meta/libraries.json` SHALL give the key and name `trystero`, the authors `["R"]`, the description "Serverless WebRTC rooms for C++20, wire compatible with Trystero.", the categories `Concurrent` and `IO`, the maintainer `R <github.overboard312@passmail.net>`, `cxxstd` `20` and `port-of` with the name `Trystero`, language `JavaScript`, version `0.26.0`, url `https://github.com/dmotz/trystero` and licence `MIT`. The repository SHALL contain `LICENSE_1_0.txt` (the Boost Software License, which governs trystero's own code) and `LICENSE-TRYSTERO.txt` (Trystero's MIT notice, with its 2021 copyright line, exactly as Trystero 0.26.0 ships it, adding no licence to trystero's own code), `.gitattributes` with `* text=auto eol=lf` and `*.bat text eol=crlf`, and a `.gitignore` of `doc/html/`, `node_modules` and `.node-modules/`.

#### Scenario: The port-of record
- **WHEN** `meta/libraries.json` is parsed
- **THEN** `port-of.name` is `Trystero`, `port-of.version` is `0.26.0` and `port-of.licence` is `MIT`

### Requirement: Tests: Every public header compiles alone on each target that builds it
`test/Jamfile` SHALL compile every public header alone with `webcpp.headers-alone trystero : ../include`: in the first call `trystero.hpp`, `trystero/*.hpp`, `trystero/protocol/*.hpp` and `trystero/detail/*.hpp` on both targets (`webcpp.targets native emscripten`); in the second `trystero/native/backend.hpp`, `trystero/native/sealer.hpp` and `trystero/native/websocket.hpp` against `/webcpp/trystero//native` natively; in the third `trystero/native/data_channel.hpp` and `trystero/native/peer_connection.hpp` against `/webcpp/trystero//native` with `<exception-handling>on` natively; in the fourth `trystero/browser/*.hpp` on emscripten. The project requirement of the tests is `<library>/webcpp/trystero//trystero`.

#### Scenario: Four calls
- **WHEN** `test/Jamfile` is read
- **THEN** it holds exactly four `webcpp.headers-alone trystero` calls, for the core, the native timer/sealer/WebSockets, the native WebRTC headers with exceptions on, and the browser headers

### Requirement: Tests: config.hpp defines WEBCPP_TRYSTERO_NO_EXCEPTIONS exactly when webcpp has no exceptions
`test/config_test.cpp` SHALL be compiled three times by `webcpp.compile`: as `config` as the build asks, as `config_developer_defined` with `<define>WEBCPP_TRYSTERO_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_TRYSTERO_DEVELOPER_DEFINED`, and as `config_all_off` with `<define>WEBCPP_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_TRYSTERO_ALL_OFF`. It first requires `WEBCPP_CONFIG_HPP` (`config.hpp` includes `<webcpp/config.hpp>`, else the error "config.hpp does not include <webcpp/config.hpp>"), then in `config_all_off` requires `WEBCPP_TRYSTERO_NO_EXCEPTIONS`, in `config_developer_defined` requires that the developer's definition is kept in a build that has exceptions, and in `config` requires that `defined(WEBCPP_HAS_EXCEPTIONS) != defined(WEBCPP_TRYSTERO_NO_EXCEPTIONS)`. The test is a compile-time check; a violation is an `#error`.

#### Scenario: The all-off macro disables trystero's exceptions
- **WHEN** `config_all_off` compiles with `WEBCPP_NO_EXCEPTIONS` in a build that has exceptions
- **THEN** `WEBCPP_TRYSTERO_NO_EXCEPTIONS` is defined, else the error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_TRYSTERO_NO_EXCEPTIONS" stops it

### Requirement: Tests: The test programs and their conventions
Each test SHALL be a Boost.Core `lightweight_test` program (`<boost/core/lightweight_test.hpp>`) declared in `test/Jamfile` with `webcpp.run`, whose `main` calls its cases in declaration order and returns `boost::report_errors()`; a case that cannot go on after a failed check returns with `if (!BOOST_TEST(x)) { return; }`, and a helper that cannot return from its caller calls `webcpp::test::require(BOOST_TEST(x))` of `test/require.hpp`, which ends the program with `std::exit(boost::report_errors())` after `BOOST_TEST` has reported the failure (a program built without exceptions cannot throw out of it). On both targets: `protocol` (with `<testing.input-file>fixtures/expected/expected.json`), `room` and `allocation` (with `-sALLOW_MEMORY_GROWTH` on emscripten, since its two counting resources hold 64 MiB each, beyond Emscripten's 16 MiB). Natively only: `native_sealer` (with the fixture), `native_websocket` (with `_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`), `native_timer`, `native_peer_connection` and `native_room` (both with `<exception-handling>on`). On emscripten only: the seven `browser_*` and the seven `browser_*_no_exceptions`. `test/.clang-tidy` and `example/.clang-tidy` inherit the superproject's configuration and switch off `bugprone-exception-escape.CheckMain` (and, for the tests, `bugprone-unchecked-optional-access`).

#### Scenario: protocol and native_sealer take the fixture path
- **WHEN** `protocol_test` and `native_sealer_test` run without exactly one argument
- **THEN** each prints a usage line (`usage: native_sealer_test <fixtures/expected/expected.json>`) and returns `boost::report_errors()`, failing

#### Scenario: Targets per program
- **WHEN** `test/Jamfile` is read
- **THEN** `native_sealer`, `native_websocket`, `native_timer`, `native_peer_connection` and `native_room` are declared for `native` and the fourteen `browser_*` programs for `emscripten`

### Requirement: Tests: The memory backend runs whole rooms in one process
`test/support/memory_backend.hpp`, in the namespace `webcpp::trystero::test`, SHALL provide a backend in memory: `memory_network` (a `backend::factory`, constructed from an `io_context`, a seed defaulting to 1 and a memory resource) whose `relay(url)` makes a `memory_relay` (a NIP-01 relay holding nothing: events reach only the subscriptions open when they arrive, as for ephemeral kinds), `connect` returns a `memory_websocket`, `make_peer_connection` a `memory_peer_connection` (pairing through descriptions `mem:<id>`), data channels that deliver in order, a `memory_timer` that behaves as the native timer (a wait cancelled, replaced or destroyed never fires), and a `memory_sealer` that seals as `<iv>$<base64(plaintext)>` with the IV the key's first 16 bytes so that only the same key opens it. Every event is posted to the `io_context`, never delivered inside the call that caused it. Its controls are: `memory_relay::events_seen`, `rate_limit_next` (answer that many EVENTs with `rate-limited:`), `close_after_subscribe`, `close_all()`; `memory_network::make_unreachable(url)`, `time_scale` (timers and clock run that many times faster), `refuse_peer_connections` (the next n are null), `record_servers` with `server_urls`, `made_connections` (refused ones included), `peak_connections` and `live_connections()`. The unix time starts at 1,759,700,000. Its hooks for the tests of shared connections (connection ids of `std::size_t`, a frame filter, injection, failure, deferred opens, a buffering channel and a virtual clock) are the subject of "Tests: The memory backend's hooks" and "Tests: The virtual clock". It is outside the library's public headers, and a program may copy it.

#### Scenario: A relay that rate-limits
- **WHEN** a test sets `relay(url).rate_limit_next` to a positive number
- **THEN** the relay answers that many EVENTs with a `rate-limited:` notice so that the strategy's backoff is exercised

#### Scenario: Peers pair through their descriptions
- **WHEN** two rooms over one `memory_network` exchange offer and answer
- **THEN** their connections pair, the state `connected` is reported on both and the offerer's data channel opens on both ends

### Requirement: Tests: The oracle lane checks Trystero's own values
`test/oracle/Jamfile` SHALL declare `webcpp.original node`, `webcpp.cases protocol : oracle.mjs : ../fixtures/cases : ../fixtures/expected` and `webcpp.lane oracle : cases-protocol`. `test/oracle/package.json` (private, `type: module`, name `webcpp-trystero-tests`) and `package-lock.json` pin `trystero`, `@trystero-p2p/core` and `@trystero-p2p/nostr` at `0.26.0`, `ws` at `8.22.0` and `@noble/secp256k1` at `3.2.0`. `oracle.mjs <cases> <output-directory>` runs Trystero's own functions (`sha1`, `hashWith`, `deriveRoomNamespace`, `encrypt`, `genKey`, `strToNum`, `getRelays`, `toHex`, `toJson`, `topicPath`, `shuffle`, `createActionWireManager`, `SharedPeerManager`, `defaultRelayUrls`) over `test/fixtures/cases/protocol.json` and writes `expected.json` with the keys `topics`, `str_to_num`, `default_relays`, `relays`, `shuffle`, `handshake`, `sealed`, `nostr_events`, `actions`, `controls` and `envelope`; the lane fails when the output differs byte for byte from the committed `test/fixtures/expected/expected.json`, and `update-expected` is its only writer. A usage error exits with 2. For the sealed values it fixes the IV by replacing `crypto.getRandomValues`, so the fixture is deterministic.

#### Scenario: Trystero would write other values
- **WHEN** the pinned Trystero computes a topic, a relay order or a frame that differs from `expected.json`
- **THEN** `b2 -a libs/trystero/test/oracle//oracle` fails

#### Scenario: The fixture feeds two tests
- **WHEN** `protocol` and `native_sealer` run
- **THEN** both read `test/fixtures/expected/expected.json` as `<testing.input-file>`

### Requirement: Tests: The interop lane meets Trystero JS in headless Chrome over a local relay
`test/oracle/Jamfile` SHALL declare `webcpp.drive interop_browser` (`interop/peer.cpp` with `-sENVIRONMENT=web`, `-sALLOW_MEMORY_GROWTH` and `<webcpp-drive-timeout>900`, driven by `interop/run.mjs --browser`, on emscripten), `webcpp.drive interop_native` (`interop/native_peer.cpp` against `/webcpp/trystero//native` with `<exception-handling>on`, `<webcpp-drive-timeout>900`, `_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`, driven by `interop/run.mjs --native`, natively) and `webcpp.lane interop : interop_browser interop_native` on native and emscripten. `interop/run.mjs` serves the pages and the build directory from a static HTTP server on 127.0.0.1, starts Chrome (the one `--chrome` or `CHROME` names, else on macOS the newest `chrome-headless-shell` of Playwright's cache; elsewhere `CHROME` is required) with `--headless=new`, `--remote-debugging-port=0`, a fresh profile, `--no-first-run`, `--no-default-browser-check`, `--disable-component-update`, `--disable-background-networking`, `--disable-sync`, `--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1` and `--disable-features=WebRtcHideLocalIpsWithMdns` (plus `--no-sandbox` as root), and starts the in-memory Nostr relay `interop/relay.mjs` on 127.0.0.1. The relay speaks NIP-01 (REQ with kinds, since and `#x` filters, CLOSE, EVENT), verifies every event's id (SHA-256 of the canonical serialization) and BIP-340 signature as a public relay does, answers `OK` with `invalid: event id does not match` or `invalid: bad signature` on failure, and forwards only to the subscriptions open at that moment. With `--public` the pages resolve Trystero's public relays instead. Every peer gathers its ICE candidates on the machine alone: the C++ peers get `stun:127.0.0.1:9`, the native peer binds to `127.0.0.1` and the Trystero JS peers have no ICE server. Chrome, its profile, the relay, the server and the native peers are removed when the run ends however it ends (scenarios' verdict, an error outside every scenario, or SIGTERM, SIGINT, SIGHUP; a profile left by a SIGKILL is removed by the next run). The run prints `all N scenarios passed` or `K of N scenarios failed` and exits 0 or 1.

#### Scenario: The driver names the Chrome it needs
- **WHEN** the run is not on macOS and neither `--chrome` nor `CHROME` is set
- **THEN** the driver fails with "CHROME is not set: off macOS, the interop test runs the Chrome that CHROME or --chrome names"

#### Scenario: The relay rejects a forged event
- **WHEN** a client publishes an EVENT whose id does not match the SHA-256 of its canonical serialization
- **THEN** the relay answers `["OK", id, false, "invalid: event id does not match"]` and counts it in `stats.rejected`

### Requirement: Tests: The interop scenarios of the browser and the native peers
`interop/run.mjs` SHALL run twenty-four scenarios for the browser's C++ peer and twenty-seven for the native peer (the fourteen shared scenarios listed first, then the eight sharing scenarios of "Tests: Interop with Trystero JS on both lanes", a wrong password and a mesh of three for both, and three more for the native peer), each bounded: C++ offering (lower id `00000000000000000000`) with the C++ peer leaving; Trystero JS offering, the C++ peer answering (id `zzzzzzzzzzzzzzzzzzzz`) with the password `secret` and the JS peer leaving; a Trystero JS peer that also sends malformed ICE candidates (`js.html`'s `malformed-candidate-to`); a first relay URL no WebSocket can be made to (`ftp://relay.invalid`), which reports a close and the room goes on; a Trystero JS peer that also sends text messages on its data channel; a C++ peer given a TURN server without credentials, which the room drops and reports; a watcher without the key (`forger.html`) answering every offer with a forged answer; the same watcher replaying a recorded sealed answer and a recorded sealed offer (`mode=replay`) after the JS peer's connections drop; the watcher reflecting the C++ peer's own sealed offer back as an answer and as an offer through a decoy id (`mode=reflect`); a flood of announcements and unsealed offers from fresh ids (300 of them, bounded by 32 unconnected negotiations, each call limited to 20 s); a relay text longer than 1 MiB, once and then after every subscription, which the C++ peer closes and reconnects with a bound that doubles; the C++ peer and a Trystero JS page each in two rooms of one app, which share one connection; a wrong password (8 s of quiet, no `peer-join`, at least one `error` event); and a mesh of two JS peers and one C++ peer in which every peer joins both others and receives both files within 30 s. The native peer meets three more: a relay that ends the connection during a write (`relay.options.strike`, a text longer than 1 MiB or a reset, 12 peers in turn at delays between 231 and 233 ms, which each survive and reconnect), a room moved over a joined one (`replace`) which leaves it as destroying it does with its pending receive completing with `room left`, and the driver's own check that a native peer killed by SIGABRT is named at once (under 5 s). A file of 100,000 bytes travels both ways with the metadata `{"name":"from-cpp.bin"}` or `{"name":"from-js.bin"}` and the checksum the scenario computes. A native peer that dies by a signal the driver did not send is named as `native#N: killed by SIGSEGV` and every wait of its scenario ends. A wire string changed in the C++ library (`"__trystero_pw"`) fails the password scenario of both.

#### Scenario: The scenarios the browser peer runs
- **WHEN** `run.mjs --browser` runs without `--public`
- **THEN** twenty-four scenarios run and the run ends with `all 24 scenarios passed` when they hold

#### Scenario: The scenarios the native peer runs
- **WHEN** `run.mjs --native` runs without `--public`
- **THEN** twenty-seven scenarios run, including `A relay that ends the connection during a write: native C++ survives and reconnects` and `A native C++ peer killed by a signal is named at once`

### Requirement: Tests: The interop peers are driven by arguments and report JSON events
`interop/peer.cpp` (browser) SHALL be configured by the URL query parameters `app`, `room`, `password`, `self`, `relays` (comma-separated), `ice`, `room2` and `program`, and report what happened in `window.events`; `interop/native_peer.cpp` SHALL be configured by `--app`, `--room`, `--password`, `--self`, `--relays`, `--ice` and `--room2`, bind its peer connections to `127.0.0.1`, write one JSON object per line on stdout (with a `kind` such as `joined`, `peer-join`, `peer-leave`, `error`, `chat`, `drink`, `file`, `sent`, `ping`, `pending`, `replaced`, `left`, `made-connection`) flushed as it goes, write diagnostics to stderr, and read commands on stdin: `leave` (Trystero's farewell, then the process ends), `replace` (move a room of another room id over the joined one with a receive of the joined room's pending) and `abort` (die by SIGABRT, as a crash does). `interop/js.html` is the Trystero JS peer (Trystero 0.26 doing exactly what the C++ peer does, importing `trystero`, `@trystero-p2p/core`, `@trystero-p2p/nostr` and `@noble/secp256k1` from the pinned `node_modules`), `interop/forger.html` the watcher without the room key (modes `garbage`, `replay` and `reflect`), and `interop/cpp.html` the page that loads the browser peer's program from `/build/` and counts the page's `RTCPeerConnection` objects made, live and at peak.

#### Scenario: A native peer leaves on request
- **WHEN** the driver writes `leave` on the native peer's stdin
- **THEN** the peer calls `async_leave`, records a `left` event with its error, and the process ends

### Requirement: Tests: No unit test reaches beyond the loopback
Every unit test SHALL pass in a Linux container that has no network but the loopback: the rooms of the unit tests meet over the memory relays, the native tests' peer connections bind to 127.0.0.1 and gather their candidates there alone, with a STUN server `stun:127.0.0.1:9` where nothing answers, and the WebSocket test talks to a server of its own on 127.0.0.1. The continuous integration holds this with two container lanes, `gcc-14` and `clang-linux-18`, which build and run the unit tests with GCC 14 and with clang 18 against `libstdc++` in an Ubuntu 24.04 container run with `docker run` and the network `none`; libsecp256k1 and libdatachannel are built before, on the runner, with the lane's compiler against the runner's OpenSSL 3, and the container sees them where the runner put them. The interop lanes were measured passing under `--network none` with one interface that holds the default route and leads nowhere, a dummy device, since Chrome offers host candidates only on the interface of its default route.

#### Scenario: A unit test that reached the network
- **WHEN** a unit test needs any host beyond 127.0.0.1
- **THEN** it fails in the `gcc-14` or `clang-linux-18` lane, which is guarantee 6 of the documentation

### Requirement: Tests: The test matrix and the commands that run it
The continuous integration SHALL run trystero's tests in lanes of one toolset each, and its own lanes (the oracle once, the interop natively and on emscripten as `native.trystero.interop` and `emscripten.trystero.interop`), each building from scratch and recording what b2 built and ran; webcpp's `tools/report/report.py` merges them into a matrix published at `https://webcpporg.github.io/report/`. From the root of a webcpp checkout, with the three libraries found, `b2 libs/trystero/test`, `b2 libs/trystero/test address-sanitizer=norecover undefined-sanitizer=norecover` and `b2 toolset=emscripten libs/trystero/test` build and run the tests natively, under AddressSanitizer and UndefinedBehaviorSanitizer, and on emscripten under node; `b2 libs/trystero/test libs/trystero/example` builds the tests and the examples. A result that counts is built from scratch with `b2 -a`, since b2 compares timestamps.

#### Scenario: The interop lane's commands
- **WHEN** a maintainer runs `b2 -a --dump-tests --out-xml=native.trystero.interop.xml libs/trystero/test/oracle//interop`
- **THEN** the native interop lane runs and writes its XML for the matrix, and the emscripten one does the same with `toolset=emscripten`

### Requirement: Tests: The conversion from Boost.Test is recorded
`test/CONVERSION.md` SHALL record how the six unit test files that came from the C++ Trystero repository this library was moved from (`main`, `d3f4f9a`) were converted from Boost.Test to `lightweight_test`: `protocol` and `room` on native and emscripten, the native backend's four (`native_sealer`, `native_websocket`, `native_peer_connection`, `native_room`) natively only; the fixture `test/unit/fixtures/expected.json` is now `test/fixtures/expected/expected.json` byte for byte; `BOOST_TEST_REQUIRE` became `if (!BOOST_TEST(x)) { return; }` in a case and `require(BOOST_TEST(x))` in a helper; collections compare with `BOOST_TEST_ALL_EQ`; the totals are 199 assertions by the static count (82, 47, 12, 22, 20 and 16 for the six files) and 250 by the run-time count, old and new. The old suite had no variant without exceptions; the browser backend's recovery tests run a second time as `browser_*_no_exceptions` (owner decision, 2026-10-10).

#### Scenario: The record states the counts
- **WHEN** `test/CONVERSION.md` is read
- **THEN** it gives the per-file totals 82, 47, 12, 22, 20 and 16 (199 in all) and the commands that derive them again from the old files at `d3f4f9a` and the new ones

### Requirement: Examples: The native quick start joins lobby and greets every peer
`example/native_quick_start.cpp` SHALL be a native program that constructs `trystero::native::backend backend(io.get_executor())` and `trystero::room room(io.get_executor(), backend, {.app_id = "webcpp-trystero-quick-start"}, "lobby")`, makes the action `chat` (`room.make_action<std::string>("chat")`), sends `hello from a native peer` to every peer that joins (`send_options{.targets = {peer_id(peer)}, .metadata = {}}`), prints each received text as `<info.from>: <text>` flushed, and calls `io.run()` until the program is stopped. The documentation includes the region `tag::program`. It is linked and never run, since it needs the public relays.

#### Scenario: It meets the browser quick start
- **WHEN** the native quick start and the chat page joined to app `webcpp-trystero-quick-start` and room `lobby` run together over the public relays
- **THEN** the page shows `1 peer`, the native peer's id joined and its greeting `hello from a native peer`, and what the page sends is printed in the native terminal after the page's peer id

### Requirement: Examples: The browser quick start keeps an application alive and echoes from a coroutine
`example/quick_start.cpp` SHALL define `struct application` (an `asio::io_context`, a `browser::event_loop loop{io}`, a `browser::backend backend{loop}`, a `trystero::room room{loop.get_executor(), backend, {.app_id = "webcpp-trystero-quick-start"}, "lobby"}` and the actions `chat` and `echo` of `std::string`), a coroutine `echo_back` that awaits `echo.async_receive(asio::as_tuple(asio::use_awaitable))`, returns when the error is set (the room was left) and answers each message with `"echo: " + text` to `info.from`, and a `main` that allocates the application with `new`, greets every joining peer with `hello` (printing `not delivered: <message>` to `std::cerr` on error), prints each `chat` message as `<from>: <text>`, starts the coroutine with `asio::co_spawn(app->loop.get_executor(), echo_back(app->echo), asio::detached)` and returns at once. The documentation includes the regions `includes`, `application`, `coroutine` and `main`.

#### Scenario: main returns at once
- **WHEN** the program's `main` runs in a page
- **THEN** it returns immediately, and every JavaScript event that reaches the library runs its handlers and then whatever became ready on the `io_context`

### Requirement: Examples: The chat page is a room in C++ and a page that only draws
`example/chat/` SHALL hold `chat.cpp`, `index.html` and a `Jamfile`. `chat.cpp` keeps one `struct chat` for the page's life (loop, backend, a `std::unique_ptr<trystero::room>` and two actions named `chat`: `action<std::string_view>` to receive a message as a view of its frame and `action<std::string>` to send by moving the text), exports through `EMSCRIPTEN_BINDINGS(chat_example)` the functions `join` (app id, room id: configures `config.log` to call `chatUi.log`, makes the room, reports `chatUi.joined`, `peerJoined`, `peerLeft` and `message`) and `send` (shows the message at once with `mine` true and reports `not delivered: <ec.message()>` in the log on failure), and ignores an empty text or a second join. `index.html` defaults the app to `webcpp-trystero-chat` and the room to `lobby`, defines `window.chatUi`, enables Join once `Module.onRuntimeInitialized`, and loads `chat-program.js`. The Jamfile builds `chat` with `-sENVIRONMENT=web` and `-sALLOW_MEMORY_GROWTH` on emscripten, and the target `site` (explicit) copies the program's `.js`, its `.wasm` and `index.html` into `site/` beside the program under the build directory, out of the tree, and prints that directory; on another toolset it stops the build with "libs/trystero/example/chat//site is the chat page for a browser, built on emscripten alone: run b2 toolset=emscripten libs/trystero/example/chat//site."

#### Scenario: Building the chat site
- **WHEN** `b2 toolset=emscripten libs/trystero/example/chat//site` runs
- **THEN** the build prints a directory holding `index.html`, `chat-program.js` and `chat-program.wasm`

#### Scenario: Another toolset
- **WHEN** `b2 libs/trystero/example/chat//site` runs on a native toolset
- **THEN** it stops with the user error naming emscripten

### Requirement: Examples: The examples are linked and never run
`example/Jamfile` SHALL declare `webcpp.link native_quick_start` (`native_quick_start.cpp` against `/webcpp/trystero//native` with `<exception-handling>on`, native only) and `webcpp.link quick_start` (`-sENVIRONMENT=web` and `-sALLOW_MEMORY_GROWTH`, emscripten only), add `<toolset>msvc:<cxxflags>/bigobj` (C1128), and `build-project chat`. Each example needs the public relays or a browser, so each has no `.expected` and no twin in Trystero's language. The native quick start is run through the target `native_quick_start-program`, and its executable is found under `bin/libs/trystero/example`. The examples' app ids are `webcpp-trystero-quick-start` and `webcpp-trystero-chat`, strings that are not on the wire.

#### Scenario: Linking the examples
- **WHEN** `b2 libs/trystero/example` builds natively and `b2 toolset=emscripten libs/trystero/example` on emscripten
- **THEN** `native_quick_start` links natively, `quick_start` and `chat` link on emscripten, and none is run

### Requirement: Docs: The documentation page is one AsciiDoc built with the reference
`doc/trystero.adoc` SHALL be the page "trystero: serverless WebRTC rooms for C++", with `:toc: left`, `:toclevels: 1` and `:attribute-missing: warn`, whose sections are included in this order: `overview.adoc`, `quick-start.adoc`, `building.adoc`, `usage.adoc`, `allocation.adoc`, `browser.adoc`, `native.adoc`, `protocol.adoc`, `design.adoc`, `guarantees.adoc`, `testing.adoc`, then the generated Reference (`include::{reference}[leveloffset=+1]`), then the appendices `differences.adoc`, `roadmap.adoc`, `acknowledgements.adoc` and `changelog.adoc`. `doc/Jamfile` SHALL declare `webcpp.doc trystero : trystero.adoc` and `webcpp.reference trystero` against `/webcpp/trystero//native`, `/webcpp//emscripten-reference` and `<define>BOOST_ASIO_DISABLE_THREADS` (no single target builds every header: the browser backend's build only on emscripten, so the reference reads every header for Emscripten's target together with the native backend's target); `doc/mrdocs.yml` sets `show-enum-constants: true`, so each enumerator of `error` has a section of its own. Every program the page shows is an example of the library, included by tag; the page shows no output of theirs. `b2 libs/trystero/doc` fails on an unresolved reference, such as a guarantee citation naming no anchor.

#### Scenario: The page includes every section
- **WHEN** `doc/trystero.adoc` is read
- **THEN** it includes the eleven sections, the reference and the four appendices in the order above

### Requirement: Docs: The native and browser sections state the backends' contracts
`doc/native.adoc` (anchor `trystero-native`) SHALL have the sections Exceptions (`trystero-native-exceptions`), Threads (`trystero-native-threads`), What the backend asks of libdatachannel (`trystero-native-libdatachannel`), Its configuration (`trystero-native-config`, a table of `bind_address`, `max_peer_connections_per_minute` and `relay_limits`) and The relays (`trystero-native-relays`, stating the 1 MiB text bound, the 4 MiB unsent bound and the 30 s `idle_timeout` with a keep-alive ping halfway through, and that a relay that reads under about 2 KB/s is closed during a 64 KiB text while a real room meets it only with a relay slower than about 200 B/s). `doc/browser.adoc` (anchor `trystero-browser`) SHALL have Driving Asio from the page's event loop (`trystero-browser-loop`, which rejects `poll()` from `emscripten_set_main_loop` for its 60 wake-ups a second and its stopping in hidden tabs, and `poll()` on a fixed interval), How JavaScript reaches C++ (`trystero-browser-javascript`), No JavaScript exception enters WebAssembly (`trystero-browser-exceptions`), What a page bounds (`trystero-browser-bounds`, including the 256 peer connections against Chromium's 500, measured with headless Chrome: the 501st throws "Cannot create so many PeerConnections"), What a page needs (`trystero-browser-page`) and A page and its program (`trystero-browser-chat`, which includes the tags `state`, `join`, `send` and `bindings` of `chat.cpp`).

#### Scenario: The anchors resolve
- **WHEN** another section cites `<<trystero-native-relays>>` or `<<trystero-browser-bounds>>`
- **THEN** the page builds with no unresolved reference

### Requirement: Docs: The building, testing and quick start sections
`doc/building.adoc` (`trystero-building`) SHALL state the three external libraries and where each is found (a table), the b2 usage (`trystero-building-project`, including the `tag::dependencies[]`, `tag::trystero[]` and `tag::native[]` regions of `build.jam` and of `example/Jamfile` and `test/Jamfile`), the emscripten toolset registered against Emscripten 6.0.11 (`trystero-building-emscripten`), the three libraries' build commands (`trystero-building-dependencies`) and Running the examples (`trystero-building-examples`). `doc/testing.adoc` (`trystero-testing`) SHALL have Testing a program that uses trystero (`trystero-testing-program`: the backend interface is the seam, the memory backend may be copied, and a native test binds to the loopback with `bind_address` `"127.0.0.1"` and names the STUN server `stun:127.0.0.1:9`), How trystero is tested (`trystero-testing-library`, a table of what each test checks, with the placeholders `{n-tests}`, `{n-tests-native}`, `{n-tests-emscripten}` and `{n-linked}` filled by the build), The oracle and the interop lanes (`trystero-testing-lanes`), The container lanes (`trystero-testing-container`) and The test matrix (`trystero-testing-report`). `doc/quick-start.adoc` (`trystero-quick-start`) SHALL show the two smallest programs: In the browser (`trystero-quick-start-browser`, from the regions of `quick_start.cpp`) and Natively (`trystero-quick-start-native`, from the region `program` of `native_quick_start.cpp`), and say that a Trystero JS page that calls `joinRoom({appId: 'webcpp-trystero-quick-start'}, 'lobby')` meets them both.

#### Scenario: The tests table names the lanes
- **WHEN** `doc/testing.adoc` is read
- **THEN** its table has rows for the public headers, `config`/`config_developer_defined`/`config_all_off`, `protocol`, `room`, `allocation`, `native_sealer`/`native_websocket`/`native_timer`, `native_peer_connection`/`native_room` and the seven `browser_*` tests

### Requirement: Docs: The guarantees keep their numbers and cite their tests
`doc/guarantees.adoc` (`trystero-guarantees`) SHALL list eleven numbered guarantees, each with an anchor `trystero-invariant-N` that a comment of the code or of a test cites as `(doc: #trystero-invariant-N)`, and each naming the test that holds it and fails when the promise breaks: 1 the wire is Trystero 0.26.0's (the `interop` lane); 2 every protocol value equals Trystero JS's (`protocol`, the `oracle` lane, `native_sealer`); 3 a peer with another password never joins a protected C++ app's rooms because initial signaling authenticates one immutable app password, with the remote mixed-password JS caveat documented (`room`: `the_same_password_joins_and_another_does_not`, `native_room`: `another_password_never_joins`, `shared`, and the interop wrong-password scenarios); 4 a payload larger than a frame travels in chunks of at most 16,370 bytes, both ways (`room`: `large_payloads_travel_in_chunks_both_ways`, 100 KB each way); 5 every allocation of a room goes through its resource (`allocation`, natively and on emscripten); 6 no unit test reaches beyond the loopback (the container lanes); 7 a room refuses a replayed or reflected offer or answer (`room`: `a_replayed_answer_is_refused`, `a_replayed_offer_is_refused`, `a_reflected_offer_is_refused_as_an_answer`, `a_reflected_offer_is_refused_as_an_offer`, `a_reflected_answer_is_refused_as_an_answer`, and the interop scenarios); 8 a relay that stops reading is dropped once the unsent text would pass 4 MiB (`native_websocket`: `a_relay_that_stops_reading_is_dropped_at_the_queue_bound`, a close before 2000 texts of 64 KiB and one only; `browser_websocket_bound`; and `room`: `a_relay_that_closes_at_once_is_tried_less_and_less_often`); 9 two peers share one connection per app (`shared` and the first sharing scenario of the interop lane); 10 a room's departure leaves the app's other rooms joined (`shared`, the interop scenario of one room leaving); 11 different password domains share no C++ app connection or presence (`shared`, the interop scenario of the passwordless room). A guarantee keeps its number.

#### Scenario: A citation resolves
- **WHEN** a comment cites `(doc: #trystero-invariant-3)`
- **THEN** `b2 libs/trystero/doc` finds the anchor `trystero-invariant-3` on the page, and fails on a citation naming no anchor

### Requirement: Docs: The README states what the library is and how it is used
`README.md` SHALL describe trystero as a header-only port of Trystero 0.26.0's serverless WebRTC rooms for C++20 and one of the libraries of webcpp, whose wire is Trystero's (a room joined from C++ and the same room joined from Trystero's JavaScript are one room); give the table of the two backends (the browser's `<webcpp/trystero/browser/backend.hpp>` on emscripten, the native one `<webcpp/trystero/native/backend.hpp>` natively) and say that `<webcpp/trystero.hpp>` is the rooms themselves and includes neither backend; say that messages, binary data and pings are ported and audio and video tracks are not yet; state that trystero raises no exception of its own, that the rooms, the protocol and the browser backend compile without exceptions, and that without exceptions, or with `WEBCPP_NO_EXCEPTIONS` or `WEBCPP_TRYSTERO_NO_EXCEPTIONS`, `native::data_channel`, `native::peer_connection` and `native::backend` do not exist; state that rooms are joined through an `app`, whose `app_config` and `room_options` replace `room_config`, with the immutable password in `app_config`, no per-room override, separate apps and self ids for different passwords, and the upstream mixed-password JS caveat; that every allocation of an app goes through the `std::pmr::memory_resource` its `app_config::memory` names and every allocation of a room through its `room_options::memory`, the app's when null, each of which must outlive the app or room, every value it handed out and the operations it leaves pending; that a pool suits a long-lived app; and that `app_config` is move-only; and list the three examples, the two lanes (`oracle` and `interop`), what derives from Trystero (the wire protocol, its constants, its message layouts, its default relays with their order and the shuffle, and its default STUN servers) with the MIT notice in `LICENSE-TRYSTERO.txt`, the documentation URL `https://webcpporg.github.io/libs/trystero/`, and the licence (Boost Software License, Version 1.0).

#### Scenario: The README's trouble-shooting list
- **WHEN** the README's "When something goes wrong" is read
- **THEN** it explains that b2 leaves trystero out when a library is not in `.local/`, that a `-s` naming a directory without the library's header stops the build, that `node: bad option: --experimental-wasm-threads` is harmless, that the page must be opened at `localhost` or `127.0.0.1`, and that both sides must use exactly the same app and room

### Requirement: Docs: The roadmap, the changelog and the acknowledgements
`doc/roadmap.adoc` SHALL be an appendix listing what trystero does not port or build yet: media (audio and video tracks and streams, and Trystero's `@_stream`, `@_track` and `@_signal` actions with renegotiation), Trystero's other features (request actions, decisions on incoming transfers and their progress, the application's own handshake, passive rooms, relay sockets shared between the rooms of an app, and the other strategies, among them MQTT and BitTorrent trackers) and the external libraries from webcpp's own build (libsecp256k1, libdatachannel and OpenSSL, found where b2's command line or `.local/` says until webcpp builds them from `third_party/`). `doc/changelog.adoc` SHALL be an appendix with the section Unreleased (`trystero-changelog-unreleased`, the region `unreleased`) holding: Added a changelog, the page's last appendix, with each version's changes; Changed `config.hpp` to include `<webcpp/config.hpp>`; Added `WEBCPP_NO_EXCEPTIONS`, which disables trystero's exceptions with every webcpp library's; breaking migration to app::join and an immutable app_config::password, with separate apps and self ids for different passwords. `doc/acknowledgements.adoc` SHALL state that trystero ports the wire protocol of Trystero 0.26.0, written with Trystero's source open, whose MIT notice is `LICENSE-TRYSTERO.txt`; that the BIP-340 signatures are libsecp256k1's (MIT) and the native WebRTC is libdatachannel's (Mozilla Public License 2.0).

#### Scenario: The unreleased changelog
- **WHEN** `doc/changelog.adoc` is read
- **THEN** it keeps the existing unreleased entries and records the breaking migration to app::join and the immutable app_config password, requiring separate app objects and self ids for different passwords

### Requirement: App: The app is the unit of sharing
The library SHALL offer `class app` (`<webcpp/trystero/app.hpp>`, in `webcpp::trystero`): this program in one application and immutable password domain, with one peer id, one connection per remote peer, shared by every room of the app that both peers are in, and the registry of those rooms. The app and every room of it MUST run on one executor, the app's; sharing is scoped by the `app` object alone, so nothing is keyed by the backend or kept in static storage, and two `app` objects with the same app id in one process are two peers. `app` is non-copyable and non-copy-assignable, `noexcept` move-constructible and move-assignable, and its state is shared with its rooms: destroying the `app` while rooms remain changes nothing for them.

#### Scenario: Two apps of one id are two peers
- **WHEN** `shared_test`'s `two_apps_with_one_id_never_meet` makes two `app` objects of one app id and one self id and joins `r1` through each
- **THEN** they never meet, make no negotiation loop and do not crash

#### Scenario: An app destroyed leaves its rooms going
- **WHEN** `an_app_destroyed_leaves_its_rooms_going` destroys the `app` handle while its rooms are joined with a peer
- **THEN** the rooms stay joined, exchange messages as before, and the peer sees no departure

### Requirement: App: Public headers
The library SHALL add the public headers `include/webcpp/trystero/app.hpp` (`class app`), `include/webcpp/trystero/options.hpp` (`app_config`, `room_options` and `default_ice_servers`) and the internals `detail/app_impl.hpp` (`detail::app_impl` and `detail::room_endpoint`) and `detail/connection.hpp` (`detail::connection`, `detail::binding` and `detail::connection_callbacks`), and remove `detail/peer_link.hpp`. The convenience header `<webcpp/trystero.hpp>` MUST include `app.hpp` and `options.hpp` besides the headers it includes in 0.1.0. Every new header opens with the WebCpp.org licence notice, includes `<webcpp/trystero/config.hpp>` first and compiles alone on both targets.

#### Scenario: The convenience header brings the app
- **WHEN** a program includes only `<webcpp/trystero.hpp>` and writes `trystero::app a(ex, factory, {.app_id = "x"});`
- **THEN** it compiles on native and on emscripten, and no header of `native/` or `browser/` is included

### Requirement: App: app_config members and defaults
The library SHALL define `struct app_config` with these members and defaults, in this order: `std::pmr::string app_id` (empty), `std::pmr::string self_id` (empty: 20 random characters, drawn once), `std::pmr::string password` (empty: a passwordless app; immutable for this app and shared by every room), `std::pmr::vector<std::pmr::string> relay_urls` (empty: Trystero's defaults derived from the app id), `std::size_t relay_redundancy = 5`, `std::pmr::vector<backend::ice_server> ice_servers` (empty: `default_ice_servers()`), `std::chrono::milliseconds idle_timeout{123'333}` (Trystero's `sharedPeerIdleMs`; 0 closes an unused connection at once), `detail::logger::sink log` (the sentences of the app and of every room, composed in the app's resource; empty means no text is composed) and `std::pmr::memory_resource* memory = std::pmr::get_default_resource()`. It is an aggregate built with designated initializers and move-only because its `log` is a move-only `handler`. The app copies every part of it, including the password, into `memory`. No setter or room override changes that password. Different passwords MUST use separate app objects with distinct self ids; a changed password requires a new app.

#### Scenario: A configuration in place
- **WHEN** a program writes `trystero::app_config c{.app_id = "webcpp-trystero-tests", .self_id = "AAAAAAAAAAAAAAAAAAAA"};` and passes it moved to an `app`
- **THEN** `app_id()` is `webcpp-trystero-tests`, `self_id()` is `AAAAAAAAAAAAAAAAAAAA`, `relay_urls()` lists five derived relays, and every other member has the default above (`app_impl_test`'s `the_configuration_is_copied_and_completed`)

#### Scenario: What the configuration names is kept
- **WHEN** `what_the_configuration_names_is_kept` names relays, an id and ICE servers
- **THEN** the app keeps exactly those, copied into its resource

### Requirement: App: room_options members and defaults
The library SHALL define `struct room_options` with the members a room adds to its app's configuration, in this order: `std::chrono::milliseconds handshake_timeout{10'000}` (Trystero's `handshakeTimeoutMs`), `std::size_t max_receive_bytes = std::size_t{256} * 1024 * 1024` (Trystero's `maxReceiveBytes`, across peers), and `std::pmr::memory_resource* memory = nullptr` (null: the app's resource). It is an aggregate built with designated initializers. It MUST NOT contain a password or allow a per-room password override; every room reads its app's immutable password.

#### Scenario: One app password for every room
- **WHEN** a program constructs an app with `{.app_id = "a", .password = "p"}` and joins `lobby` and `elsewhere` with default room options
- **THEN** both rooms use password `p`, a handshake timeout of 10 s and a receive bound of 256 MiB, and a room option naming `password` does not compile

#### Scenario: The password belongs to the app resource
- **WHEN** an app is made with a password string in temporary caller storage and a counted `app_config::memory`, then joins a room with its own resource
- **THEN** the app keeps its password in the app resource after caller storage is gone, and the room uses that password without copying it into its own resource

### Requirement: App: Construction and accessors
`app(executor_type ex, backend::factory& factory, app_config config)` SHALL make the app: it draws its 20-character id from `protocol::id_alphabet` through the backend's `random` unless the configuration names one, derives the relays, copies the configuration into `config.memory`, counts the handle as a user of the app's state, and starts no relay or negotiation (those are rooms'). The backend `factory` MUST outlive the app and its rooms. Its accessors are `get_executor()` (the app's `boost::asio::any_io_executor`), `app_id()` and `self_id()` (each a `std::string_view` valid as long as the app) and `relay_urls()` (a `std::span<const std::pmr::string>`: the relays every room of the app uses, derived from the app id unless the configuration named them). The preconditions `config.memory` not null, and the app not moved from for `join`, `on_error` and the accessors, are checked by `BOOST_ASSERT`.

#### Scenario: Every room has the app's id
- **WHEN** `every_room_has_the_apps_id` joins two rooms through one app
- **THEN** both report `self_id()` equal to the app's, and `app::self_id().size() == 20` when the configuration named none

### Requirement: App: One peer id per app
The app SHALL announce, signal, tie-break and handshake in every room of the app with its one `self_id`, drawn once at construction, so that a peer connected through any room sees one id across the rooms it shares with the app, as Trystero's one `selfId` per page does. `room::self_id()` returns the app's. A `self_id` longer than `protocol::max_peer_id_bytes` (64) MUST be replaced by 20 random characters and reported to `app::on_error` with `error::invalid_configuration`; one of exactly 64 bytes is kept. Rooms that must stay unlinkable on the relays and to peers are joined through separate `app` objects, which have two ids, two connections and nothing shared.

#### Scenario: A 65-byte id is replaced
- **WHEN** an app is configured with a `self_id` of 65 `x` characters and `on_error` is set right after the constructor
- **THEN** `self_id().size()` is 20 and exactly one report of `error::invalid_configuration` with an empty peer is delivered

### Requirement: App: The configuration reports of the app
`app::on_error(F)` SHALL set what is called with the peer's id, an `error_code` of `error_category()` and a description (each a view valid for the call), for each part of the configuration the app set aside: an over-long `self_id`, a negative `idle_timeout` (replaced with 0) and an ICE server WebRTC refuses (dropped, and the app goes on with the others on every backend), always with an empty peer id and `error::invalid_configuration`. The reports are posted to the app's executor once the constructor has returned, each delivered once to the handler current at delivery (a handler replaced before then gets none), and `room::on_error` reports only problems with peers. The handler is kept in the app's resource and replaces the one set before.

#### Scenario: A setup report reaches the current handler once
- **WHEN** `each_setup_report_reaches_the_current_handler_once` configures an unusable ICE server, sets one handler, then another before the reports are delivered
- **THEN** only the second handler is called, once per report

#### Scenario: What no peer could use is set aside
- **WHEN** `what_no_peer_could_use_is_set_aside` configures a `self_id` of 65 bytes, an `idle_timeout` of -1 ms, the ICE servers `http://example` and `stun:stun.example` and one with no URL
- **THEN** the app uses a random 20-character id, an `idle_timeout` of 0 and the one usable server, and sets aside four reports: `self_id is longer than any peer accepts, so the app uses a random one`, `idle_timeout is negative, so the app closes an unused connection at once`, `the ICE server http://example is not stun:, stuns:, turn: or turns:, so the app goes on without it` and `the ICE server (no URL) names no URL, so the app goes on without it`, each logged in the category `app`

### Requirement: App: join
`boost::system::result<room> app::join(std::string_view room_id, room_options options = {})` SHALL join a room of the app, Trystero's `joinRoom` for an app already configured: it connects to the app's relays, subscribes to the room's topics and announces the peer, with the room's token, key and topics derived from the app id, the room id and the app's `self_id`. It MUST fail as a value, never by exception: `error::already_joined` while a room of that id is joined in the app (a room in its 99 ms farewell is finished at once first, its `presence(0)` sent, as Trystero's rejoin does) and `error::too_many_rooms` when `app::max_rooms` (64) rooms count. `room_id` is read during the call. `join` touches the app's state at once, so it is called where the app's executor runs (on its thread, or within its strand).

#### Scenario: Joining twice and beyond 64
- **WHEN** `joining_is_refused_twice_and_beyond_64` joins `r1`, joins `r1` again, then joins 64 distinct rooms and a 65th
- **THEN** the second `r1` fails with `error::already_joined`, the 65th room fails with `error::too_many_rooms`, and the first 64 succeed

#### Scenario: Joining after a leave
- **WHEN** a room's `async_leave` has completed and `join` is called with the same room id, and again during the room's farewell
- **THEN** both succeed

### Requirement: App: At most 64 rooms
`app::max_rooms` SHALL be 64, a `static constexpr std::size_t`: a Trystero peer drops a connection whose remote advertises more than 64 room tokens, so a 65th room would cost every connection of the app. The count includes a room in its farewell until its `presence(0)` has been sent: `join` of a 65th room still fails with `error::too_many_rooms` while one of the 64 is in its farewell, and succeeds once the `presence(0)` is sent.

#### Scenario: A room in its farewell still counts
- **WHEN** 64 rooms are joined, one calls `async_leave`, and a new room is joined during its 99 ms farewell
- **THEN** the join fails with `error::too_many_rooms` until the leaving room has sent `presence(0)`

### Requirement: App: Move, destruction and the users of the state
The app and each room SHALL count as users of the app's state, from the handle's construction until it is destroyed or moved from, and for a room until it ends (its farewell completes, it is destroyed or moved over). `app& operator=(app&&) noexcept` releases the state it held (its rooms go on), guards self-assignment and takes the state of the source, which may then only be destroyed or assigned. When the last user goes the app MUST close every connection, with its handlers cleared first, at that call, which runs in the user's code or in a handler of the executor; the destructor of `detail::app_impl` closes nothing and posts nothing, since it may run while the `io_context` is destroyed, and then only frees.

#### Scenario: A moved-from app releases nothing
- **WHEN** `a_moved_from_app_releases_nothing` moves an app that has joined rooms and destroys the moved-from handle
- **THEN** the peer sees no departure and `made_connections` does not change

#### Scenario: An app assigned over releases its state
- **WHEN** `an_app_assigned_over_releases_its_state` move-assigns a new app over one without rooms
- **THEN** the state it held is released (its connections end), and assigning an app to itself keeps its state (`an_app_assigned_to_itself_keeps_its_state`)

#### Scenario: The last user ends every connection
- **WHEN** `the_last_user_ends_every_connection` adopts connections to B and C with two users counted and releases one user, then the other
- **THEN** after the first release both connections are live and the room has no event, and after the second both end and the room hears `ended B: the app has no room left` and `ended C: the app has no room left`; a release with no user left does nothing, and a connection adopted afterwards ends at the next release

#### Scenario: The destruction is quiet
- **WHEN** `the_apps_destruction_is_quiet` resets the last reference to the state while an adopted connection is live and frames of a registered room's token then arrive
- **THEN** the connection stays live, keeps no frame and notifies no room, and ending it logs only its own line `webrtc: connection with B ended: test`

### Requirement: App: The standalone room and room_config are removed
The library SHALL NOT offer `room(executor_type, backend::factory&, room_config, std::string_view)` nor `room_config`: a room is joined through an `app` only, so no program can fall back into one id per room. `room` keeps no public constructor but its move constructor (the others are private to `detail::room_impl::make` and `app`), and `room` keeps every member it has (`get_executor`, `self_id`, `relay_urls`, `peers`, `on_peer_join`, `on_peer_leave`, `on_error`, `make_action`, `async_ping`, `async_leave`, moving, and leaving at destruction), `get_executor`, `self_id` and `relay_urls` being the app's. `action<T>` is unchanged. This is a **BREAKING** change; the migration is `room r(ex, f, {.app_id = "a", .password = "p"}, "lobby");` becoming `app a(ex, f, {.app_id = "a", .password = "p"}); auto r = a.join("lobby");` followed by a test of the result.

#### Scenario: The migration of a quick start
- **WHEN** `example/native_quick_start.cpp` makes an `app`, then `join("lobby")`
- **THEN** it prints the reason and returns 1 when the join fails, and otherwise uses the room as before

### Requirement: App: The idle time of a connection
`app_config::idle_timeout` SHALL be how long a connection with no binding left after a departure stays open: Trystero's `sharedPeerIdleMs`, 123,333 ms by default. A binding made meanwhile cancels the timer and keeps the connection; when it fires the connection is closed; an `idle_timeout` of 0 closes it at once. After a kick the connection is closed at once, whatever the idle time.

#### Scenario: A peer leaves every room
- **WHEN** A leaves `r1` and `r2`, with `idle_timeout` 200 ms
- **THEN** B sees both leaves, the connection closes after 200 ms and not before, a room joined again within it reuses the connection, and with `idle_timeout` 0 it closes at once

### Requirement: Shared: Ownership of a connection
The library SHALL give a connection to the app, not to a room: `detail::app_impl` owns the configuration, the self id, the logger, the registry of rooms (by token and by room id) and every `detail::connection`; a `detail::connection` owns one remote peer's peer connection and data channel, the tokens the remote advertised, the frames kept for rooms not bound yet and the idle timer, and holds plain pointers to its bindings, by token and in the order in which they wait for the channel to drain; a `detail::binding` (the action wire's `channel_link`) is one room's use of one connection, owned by its room's peer state, holding a `std::weak_ptr` to its connection. A room holds a `std::shared_ptr<detail::app_impl>` and the app holds no strong reference to a room, so there is no cycle; the registry reaches a room through a weak reference or a handler that the room erases when it ends. A binding registers itself in its connection when it is made and removes itself, with every place it holds in the drain order, when it detaches, before the call returns; a binding's destructor detaches it if it was not.

#### Scenario: The registry holds rooms weakly
- **WHEN** `the_registry_holds_rooms_by_a_weak_reference` registers rooms `r1` and `r2`, unregisters a token with another room object, then drops the last strong reference to the room of `r2`
- **THEN** `find_room("r1")` still finds `r1`, `counted_rooms()` counts registered rooms, a later registration of the same token replaces the earlier entry, and `find_room("r2")` is null: the registry did not keep the room

#### Scenario: A detached binding neither sends nor waits
- **WHEN** `a_detached_binding_neither_sends_nor_waits` detaches a binding and then sends or waits on it
- **THEN** the send fails and the wait completes with `false`

### Requirement: Shared: One connection per remote peer per app
Two peers that meet in several rooms of the same app SHALL share one WebRTC connection: the app MUST hold at most one live connection to a given remote peer id, and every room of the app that both peers are in uses it, each frame naming its room by its token. A connection is live when its peer connection is neither closed nor failed and its channel is neither closing nor closed; a channel not yet open counts as live. The wire does not change: a Trystero JS peer sees what it sees from a Trystero JS peer with several rooms.

#### Scenario: Two rooms, one connection
- **WHEN** `two_rooms_share_one_connection` has A and B each join `r1` and `r2`
- **THEN** both rooms join in both directions, exactly one connection is made, and messages of an action of the same name in each room reach only their own room

#### Scenario: Three rooms, one connection
- **WHEN** `three_rooms_share_one_connection` has A and B join `r1`, `r2` and `r3` in a different order on each side
- **THEN** all three rooms join over one connection

### Requirement: Shared: Adoption of a negotiated connection
When a room's negotiation opens, the signaling SHALL hand `(peer, peer connection, data channel)` to the app. If the room is leaving, the new connection MUST be closed, its handlers cleared first, and nothing else happens. If the app holds a live connection to that peer, the new one MUST be closed the same way and the room bound to the existing one, Trystero's rule that the first registered wins. Otherwise the app makes a `connection`, replaces every handler of the peer connection and channel with its own, in the app's resource, sends `presence(1, T)` for every room of the app, and binds the room. Adoption ends a stale connection to the peer before replacing it, as Trystero's reuse does, and is re-entrant: an adoption from inside the end of a stale connection is the one kept, a nested adoption gone stale ends before the outer one, and a late end erases only its own entry.

#### Scenario: A second connection is dropped
- **WHEN** `a_second_connection_is_dropped` has both rooms negotiate at once
- **THEN** one connection stays and both rooms join over it

#### Scenario: One connection per peer
- **WHEN** `adopt_starts_one_connection_per_peer` registers rooms `r1` and `r2`, adopts a connection to B and then a second connection to B
- **THEN** the first sends `presence(1, T1)` and `presence(1, T2)`, the second is closed and the first returned, the log reads `webrtc: connection with B adopted` and `webrtc: a second connection with B closed: the first is shared`, and the rooms hear nothing

#### Scenario: A stale connection ends before its replacement
- **WHEN** `a_stale_connection_ends_before_its_replacement` adopts a connection to B, closes its channel, and adopts a second connection to B
- **THEN** the app finds no live connection to B before the second is adopted, the room hears `ended B: replaced by a new connection to the peer` once, and the app then holds the new connection

#### Scenario: An adoption from inside a stale end
- **WHEN** `an_adoption_from_inside_a_stale_end_is_the_one_kept` has a room adopt a connection to the peer from inside the end of the stale one
- **THEN** the replacement the outer adoption brought is closed, the room's is the one kept, and the peer has one connection

### Requirement: Shared: Crossed connections recover
When the two sides register different connections to each other (two rooms' negotiations opening in crossed order), each side SHALL close the connection the other kept, so both lose every binding, and each room announces again and they meet anew over one connection, as Trystero's rule does. The interop scenarios that let both rooms negotiate at once allow 60 s, record how many connections each side made, and fail, naming the counts, when a side made more than 4.

#### Scenario: Crossed connections recover
- **WHEN** A and B get ICE servers of their own, `defer_opens` is set, A and B join `r1`, then `r2`, and the test opens A's end of `r1`'s pair and B's end of `r2`'s first, then the others
- **THEN** both sides lose both connections, then both rooms join again within 15 s over one connection

### Requirement: Shared: Presence frames
A connection SHALL advertise the app's rooms with presence frames (`protocol` envelope kind 2: present, token length, token) and keep the remote's advertised tokens, at most 64. When a room joins, once its token is known, the app binds it to every live connection whose remote advertised that token and then sends `presence(1, T)` on every live connection of the app; when a room leaves or is destroyed it sends `presence(0, T)` on every live connection, until which the room counts among the app's rooms; and when the app adopts a connection it sends `presence(1, T)` for every room. A `presence(1, T)` received records `T` among the remote's rooms (a 65th distinct token ends the connection for every room) and binds the room of token `T`, if any, by the binding conditions; `presence(0, T)` forgets `T`, drops the frames kept for `T` and ends the binding of `T`, if any, as a departure.

#### Scenario: Every token is advertised at start
- **WHEN** `start_advertises_every_token` starts a connection for an app with several rooms
- **THEN** a presence frame is sent for each room's token

#### Scenario: A withdrawn token is forgotten
- **WHEN** `a_withdrawn_token_is_forgotten` receives `presence(0, T)` after `presence(1, T)`
- **THEN** `T` is no longer among the remote's tokens

#### Scenario: A 65th token ends the connection
- **WHEN** `a_65th_advertised_token_ends_the_connection` receives `presence(1, T)` for 64 distinct tokens, then a known one again, then a 65th
- **THEN** the first 64 do not end the connection, a known token is not a new one, and the 65th ends it with the reason `too many advertised rooms`, the remote's channel then closing

### Requirement: Shared: Routing by room token
The connection SHALL read every message of its channel and route it: a frame that does not decode (`protocol::decode_envelope`: shorter than 3 bytes, longer than 16,515, of another kind, or with a token of 0 or more than 128 bytes) ends the connection for every room; a room frame whose token is bound is handed to that room's action wire with the frame; a room frame of a token no room of the app has is dropped silently and the connection stays; and a room frame of another room of the app that is not bound to this connection is kept (see "Shared: Frames kept for a room not bound yet") unless a dropping window applies. Sending is the action wire's, through its binding: a room frame goes to the shared channel wrapped with the room's token.

#### Scenario: A bound token reaches its binding and another is kept
- **WHEN** `a_bound_token_reaches_its_binding_and_another_is_kept` sends room frames of the tokens `T1` (bound), `T2` (a room that keeps frames) and `nobody` (no room)
- **THEN** the binding of `T1` receives its frame, `T2`'s frame is kept (`kept_count()` is 1 and `take_kept("T2")` returns it), and `nobody`'s is dropped

#### Scenario: A malformed frame ends the connection
- **WHEN** `a_malformed_frame_ends_the_connection` delivers the bytes `3, 0, 1, 'x'` (a kind that does not exist)
- **THEN** the connection ends with the reason `invalid or incompatible room frame`

#### Scenario: The frame size limit
- **WHEN** a room frame of exactly 16,515 bytes arrives, and another of 16,516 bytes
- **THEN** the first does not end the connection and the second does

#### Scenario: Unknown tokens are dropped
- **WHEN** `inject` delivers on A's end a room frame of a token no room of the app has
- **THEN** the frame is dropped, the connection stays, and the app's resource does not grow

### Requirement: Shared: Frames kept for a room not bound yet
The connection SHALL keep a room frame whose token names a room of the app that is not bound to this connection, at most 64 frames per connection (at most 64 x 16,515 bytes), and drop every kept frame 10 s after the first was kept; frames for any other token are not kept. Withdrawing the token (`presence(0, T)`) drops the frames kept for it; taking every kept frame, or a withdrawal that empties them, restarts the window. A frame kept for a room is delivered when that room binds, in order and before any later frame, except that a kept `@_leave` discards itself and every frame kept before it for that token, since they belong to a binding of the other side that has ended. Trystero's second buffer, for a bound room whose handler is not yet set, has no counterpart: a binding's delivery is set when it is made, in the same call.

#### Scenario: Kept frames are bounded
- **WHEN** `kept_frames_are_bounded_and_dropped_after_10_s` keeps 65 frames for an unbound room
- **THEN** 64 are kept, and 10 s later they are dropped

#### Scenario: A kept leave discards what precedes it
- **WHEN** before A's `r2` binds B, `inject` delivers on A's end B's `@_hsready` and `@_leave` of `r2`, and B's presence is then let through
- **THEN** A binds, discards both, and the handshake completes on B's next frames, with no leave reported

### Requirement: Shared: The drain order
A send that must wait for the channel to drain SHALL wait in its binding, and the binding takes one place in the connection's drain order; at each drain the connection calls every binding in that order, and each binding its own waiters in theirs, so that a room's chunked transfer sends one chunk per drain while another room's waiting sends get their turn at the same drain. Each wait keeps its 10 s bound (`error::channel_stalled`). A send that does not wait (an inline frame while the channel buffers at most 64 KiB) goes to the channel at once. A wait at the threshold completes at once; a binding made during a drain waits for the next.

#### Scenario: One turn per drain
- **WHEN** `a_binding_has_one_turn_per_drain` has binding `T1` wait, re-wait from inside its turn, and a second wait of `T1` after it
- **THEN** the turns are `a`, `b`, `again`: each wait gets one turn per drain, in the order of waiting, and the re-wait's turn is the next drain's, once the channel buffers no more than `low_threshold`

#### Scenario: Other rooms chat during a transfer
- **WHEN** with `channel_capacity` set, B sends a 5 MB transfer in `r1` while `r2` sends 20 chats that must wait for the drain
- **THEN** every chat completes before the transfer does

### Requirement: Shared: A binding's end is its room's alone
A kick, a departure or a failure in one room SHALL end that room's binding and no other: a handshake that fails or times out, an action-level violation, the peer's `@_leave` in a room and `presence(0, T)` end that room's binding (a kick sends `@_leave` in the room first), while a frame that does not decode, a 65th advertised room, a closed or failed channel and an idle connection end the connection. Detaching a binding calls each of its drain waiters with `false` and frees them, in the room's resource, and removes the binding from its connection, before it returns; the room's negotiation entry is then erased, pending pings and transfers fail and `on_peer_leave` fires if the peer was active. The other rooms keep the connection.

#### Scenario: A kick is per room
- **WHEN** B first authenticates with the same app password, then sends an incorrect proof in `r2` and A is the lower id
- **THEN** A reports `error::wrong_password` in `r2` only and `r1` stays joined

#### Scenario: An action-level kick is per room
- **WHEN** `inject` delivers on A's end a room frame of `r2`'s token whose action frame does not decode
- **THEN** A kicks B from `r2` only and `r1` keeps exchanging

#### Scenario: One room leaves
- **WHEN** A leaves `r1` (and separately destroys it, and moves a room over it)
- **THEN** B sees `on_peer_leave` in `r1` only, `presence(0, r1)` arrives, `r2` keeps exchanging, the connection stays, and a send in `r1` completes with `error::room_left`

### Requirement: Shared: A connection's end
A connection that closes, fails, stays `disconnected` for 5 s, receives a frame that does not decode or whose remote advertises a 65th room, or loses the app's last user SHALL end for every bound room: it clears the handlers it set, then moves its bindings out of itself, then detaches each and notifies its room, moving each callback out before calling it, since a room may destroy its binding, its handlers or its own state from inside the call. Each bound room sees the peer leave with the reason, restarts its announcements, since no connection to that peer remains, and erases its rebinding counts for that peer; a connection's end names the connection that ended, so a room acts only on its own. The end fails every drain waiter, in the order of waiting.

#### Scenario: A failed connection ends every room's binding
- **WHEN** `fail` ends the connection of two joined rooms
- **THEN** both rooms see the peer leave, then meet again over one new connection

#### Scenario: A disconnection ends after 5 s unless it reconnects
- **WHEN** `a_disconnection_ends_after_5_s_unless_it_reconnects` reports `disconnected` and, in a second case, `connected` again after 4 s
- **THEN** the first ends the connection after exactly 5 s with the reason `connection lost`, and the second stays live

### Requirement: Shared: A connection with no binding left
A connection with no binding left SHALL be kept idle for `app_config::idle_timeout` after a departure (the peer's `@_leave` or `presence(0)`, or our room's leave or destruction), then closed, a binding meanwhile cancelling the timer; after a kick it is closed at once, as the port does today for its single room, where Trystero keeps it idle in both cases, a difference that is a bound and changes nothing a Trystero JS peer can rely on since a connection may close at any time. The peer can then meet that room again only through the room's own signaling, which needs the room's key: one with a wrong password cannot, since its signals do not open, and one with the key renegotiates as a new peer does, within the room's window (64 connections per 57 s), and the connection that opens is adopted and bound with a count of zero.

#### Scenario: The last binding ends or idles the connection
- **WHEN** `the_last_binding_ends_or_idles_the_connection` detaches the last of two bindings by a kick, the only binding with an idle time of 0, and the only binding with an idle time of 1000 ms to which a second binding is made after 999 ms
- **THEN** the connection ends at once with the reason `its last room kicked the peer` for the kick (and not while another binding remains), with `no room left` for the 0 ms idle time, and in the third case it stays open while a binding is made before the timer fires and ends with the reason `idle` only 1000 ms after the last departure

#### Scenario: A kick of the last binding closes the connection
- **WHEN** B knows the app password but sends an incorrect room proof, and only `r2` remains in common and A is the lower id
- **THEN** the connection closes at once; a later retry requires a fresh negotiation under the app password, within the room's construction window, and begins with no old connection's rebinding count

### Requirement: Shared: Relays and configuration stay per room, configured once
The relays SHALL stay per room, configured once for the app: each room runs its Nostr strategy over `app::relay_urls()`, with its own sockets, subscriptions and bounds, and the app's ICE servers are the WebRTC configuration of every connection its rooms make. Sharing relay sockets, batching the topics of several rooms into one subscription and one Nostr key per app are not part of this change; no peer sees the difference.

#### Scenario: Each room announces on its own topics
- **WHEN** two rooms of one app are joined
- **THEN** each publishes its announcements on its own root topic over the app's relays, and a room bound by presence or reuse publishes no offer and no answer

### Requirement: Shared: Failure scopes
The library SHALL scope each failure like this: a frame that does not decode, a 65th advertised room, a channel that closes and a connection that fails end the connection and so every room's binding on it; a connection idle for `idle_timeout` with no binding ends the connection; a handshake that fails or times out in a room, an action-level violation in a room and the peer's `@_leave` or `presence(0, T)` end that room's binding alone. A peer that floods one room spends that room's bounds, not another's: each room has its own queues, transfers, reservation, handshake and backoff, frames are routed in arrival order without being held for another room, and waiting sends take turns.

#### Scenario: Flood isolation
- **WHEN** B floods `r1` with 10,000 inline messages to an action without a handler while `r2` pings and chats
- **THEN** the `r1` queue keeps 1024, every `r2` message arrives and each `r2` ping completes within 1 s

### Requirement: Shared: Bounds
The library SHALL bound a shared connection as follows: rooms of an app at `app::max_rooms` 64; rooms a remote advertises at 64 distinct tokens; frames kept for a room not bound at 64 per connection for 10 s; a frame at 16,515 bytes with a token of 1 to 128 bytes; a connection with no binding at `idle_timeout` after a departure and none after a kick; a handshake at `handshake_timeout`; challenges an initiator accepts an answer to at the last 8; and the bounds per room of 0.1.0 unchanged (32 negotiations, 64 connections per 57 s, 1024 remembered descriptions, 5.3 s of counter-announcements per relay, 64 pending frames per peer for 120 s, one incoming transfer per peer, `max_receive_bytes` across peers, 1024 queued messages per action), and a backend's own bounds (256 peer connections held in a page, 256 a minute natively). The app itself holds at most about 1 MiB of kept frames and 8 KiB of tokens per connection. A connection made for a room counts against that room's window also after the app adopts it, and a room bound by presence or reuse makes none.

#### Scenario: The kept frames' memory is bounded
- **WHEN** 65 frames of 16,515 bytes are kept for an unbound room
- **THEN** the connection holds 64 of them, about 1 MiB, and none 10 s after the first

### Requirement: Binding: The three paths and the conditions
A room SHALL be bound to a live connection at most once, by one of three paths: its own negotiation (adoption); presence (a room that joins is bound to every live connection whose remote advertised its token, and a `presence(1, T)` received, or a room frame of its token from that peer, binds the room of token `T`, in a protected or passwordless app alike); or signaling reuse (the signaling, once a message has parsed and names a peer id other than the app's, asks the room whether the app holds a live connection to that peer, and if it does the message goes no further). Over a live connection to a peer P, a room R MUST bind P only when every condition holds: P has itself advertised R's token `T` on that connection (its own `presence(1, T)`, not withdrawn since), so that an announcement or a signal published in P's name by anyone else never binds P's connection; R is not leaving; and R holds no running backoff for P. It then binds at the first of these events: R joins; P's `presence(1, T)` arrives; a room frame of `T` arrives from P; the reuse of the third path, on the first meeting; R's rebinding time for P comes.

#### Scenario: A late join, both directions
- **WHEN** connected through `r1`, A joins `r2`, then B (and the reverse)
- **THEN** `r2` joins over the existing connection, no connection is made, and no offer and no answer is published on `r2`'s topics

#### Scenario: Binding needs the peer's own presence
- **WHEN** the filter drops B's `presence(1, r2)` while B joins `r2`, A having joined it first
- **THEN** B binds at its join on A's presence, A keeps B's handshake frames and does not bind since B has not advertised `r2`, B's handshake times out 10 s after its join and B kicks, and at its rebinding time, 21 s after its join on the virtual clock, B binds again and sends its presence, and A binds at that instant

### Requirement: Binding: The steps of a binding
Binding SHALL perform these steps in this order: the room's negotiation entry for that peer is kept, its connections closed and its timers cancelled, and marked connected (so that every later signal of that peer is ignored while the binding lasts and a bound room starts no negotiation with it); the connection's idle timer is cancelled; a room that binds by reuse or at its rebinding time sends `presence(1, T)` on the connection, which makes the other side bind at once, a Trystero JS peer included; the peer's state is made, with its handshake timer; the frames the connection kept for that token are delivered in order, except that a kept `@_leave` discards itself and what was kept before it; then the challenge or `@_hsready` is sent. A room that binds by reuse sends nothing on the relays in answer.

#### Scenario: A bound room starts no negotiation
- **WHEN** `r2` binds over the connection of `r1`
- **THEN** for the whole case no offer and no answer is published on `r2`'s topics

### Requirement: Binding: Signaling reuse on the first meeting
When a signal (an announcement, an offer, an answer or a candidate, before any of them is opened) parses and names a peer id other than the app's, the room's signaling SHALL ask the room whether the app holds a live connection to that peer; if it does, the message goes no further, whatever follows, as Trystero ignores a peer it is bound to, since the app keeps one connection per peer and a negotiation would only make a duplicate. The room binds by this path only on the first meeting of that room and that peer over that connection; after a binding of them has ended, only the other events bind them again. Without a live connection the negotiation goes on as in 0.1.0.

#### Scenario: A forged announcement before X joins
- **WHEN** X shares `r1` with A and a third socket on the relay announces in `r2` in X's name
- **THEN** A's `r2` binds nothing, reports nothing and publishes no offer, and X then joins `r2` and is joined within a round trip

### Requirement: Binding: Unbinding and the announcements that follow
A binding SHALL end, in that room only, when the peer sends `@_leave` in it, when it sends `presence(0, T)`, when the room kicks the peer, or when the room leaves (after `@_leave` and Trystero's 99 ms) or is destroyed or moved over (at once). A room that leaves or is destroyed is removed from the app, which sends `presence(0, T)` on every live connection. The room restarts its announcements after a peer leaves only when no live connection to that peer remains: over a live one every rebinding goes through the connection, where Trystero restarts them after every leave.

#### Scenario: Leaving ends one room's binding
- **WHEN** A leaves `r1` while `r2` is joined with B over the same connection
- **THEN** only `r1`'s binding ends, `presence(0, r1)` is sent, and `r2` goes on

### Requirement: Rebinding: The schedule after a kick
When a binding of R with P ends while the connection stays live, by a kick of either side (ours, or P's `@_leave`), R SHALL schedule its rebinding time for P one step after the end, the first step being the larger of `handshake_timeout` and 10 s, plus 1 s (11 s by default), and each further such end of the same pair in the same room doubles the step, to at most 5 minutes. The count lasts until a handshake of R with P completes or the connection ends; nothing else resets it, so a peer cannot shorten it by leaving and joining again. At the rebinding time R binds P if the binding conditions hold, sending `presence(1, T)` first. A peer that left the room in earnest follows its `@_leave` with `presence(0, T)`, so its rebinding time passes without a binding. A late end of an old connection never shortens a backoff: the count is erased only when counted on the connection that ended.

#### Scenario: The rebinding rate
- **WHEN** A, the lower id, and B first share `r1` with the same app password, then B sends incorrect proofs in `r2`, on the virtual clock for 100 s
- **THEN** A reports `error::wrong_password` 4 times at exactly 0, 11, 33 and 77 s after the first and no fifth before 165 s, B reports nothing, and neither side publishes on `r2`'s topics more than its regular announcement cadence

#### Scenario: The ids swapped
- **WHEN** the same with the ids swapped and the responder sending incorrect room proofs
- **THEN** B, now the lower id, reports at those times and A nothing

### Requirement: Rebinding: Only the lower id refuses
While a rebinding time runs, only the side with the lower id, the handshake's initiator, SHALL refuse the peer, after its own kicks only: it does not bind P, ignores P's signals and keeps P's frames of `T` as for any unbound room (64 per connection, 10 s). The side with the higher id refuses no binding, but after its own kicks only it drops P's frames of `T` instead of keeping them, and does not bind on them, until it next sends `presence(1, T)` (at its rebinding time or in reply to P's `presence(1, T)`, on which it then binds); those frames were sent for the binding that ended, and kept, a late challenge would be answered in the next binding in place of the new one. So two honest peers cannot alternate kicks.

#### Scenario: A wrong room proof over an authenticated shared connection
- **WHEN** A (lower id) and B (higher id) share a connection authenticated with the same app password, and B sends incorrect proofs in `r2`
- **THEN** A kicks at 0 with `wrong_password`, B departs unbound, at A's time both bind, A challenges, B answers, and A kicks again; A reports at 0, 11, 33, 77, 165, 341 and 641 s at most (six in ten minutes, then one per 5 minutes), and B reports nothing

### Requirement: Rebinding: The initiator accepts any of its last 8 challenges
The initiator SHALL keep, per peer and room, every challenge it sent that peer since their last completed handshake, at most the last 8, with the rebinding count (erased at a completed handshake or at the connection's end). A response that is the correct digest of any of them passes, whichever binding it was sent for and whenever it arrives, and a passed initiator sends `@_hsready` and checks nothing more. Before it has sent the current binding's challenge, a response that matches none is ignored; after, it is `wrong_password`. The initiator always sends its challenge when it binds, even when a kept answer has already passed it, so that the responder's new binding has a challenge to answer. The responder answers one challenge per binding and then checks nothing more.

#### Scenario: A late answer to an old challenge passes
- **WHEN** a hold releases B's answers to A's first challenge during A's third binding, once A has bound again and sent its third challenge
- **THEN** A passes on that answer and reports no `wrong_password`

#### Scenario: An answer older than 8 challenges fails
- **WHEN** B's answers are held across eight bindings and released during the ninth
- **THEN** the answer to the first challenge, no longer among the last 8, is `wrong_password`

### Requirement: Rebinding: A transient failure converges
After a transient handshake failure the pair SHALL join by the first rebinding time with no `wrong_password` and no other `on_error` report than the one `handshake_timed_out` of the side that timed out, in both id orders and with and without a password: the side that timed out reports it once, and both rooms are joined at the later of the first rebinding time (11 s after the kick) and the release of a held frame; a held frame holds its end's every later frame, as a data channel delivers in order. The side that must not time out has `handshake_timeout` 30 s so that which side times out first is fixed. With a Trystero JS lower id two residual cases remain, in neither of which the C++ side reports anything false: a reuse binding inside the higher id's dropping window (joined by the second rebinding time, at most 43 s by default plus the one-way delay, once at most per failure), and a page whose keep timer fires after B's presence arrives (one false password error on the JS side and one more step, at most three times at the default settings under Chrome's intensive throttling).

#### Scenario: A times out, B's answer is late
- **WHEN** A, the lower id, has B's answer held past A's `handshake_timeout`, then released once before and once after the rebinding time
- **THEN** A reports one `handshake_timed_out`, a kept or later answer to the old challenge passes A, A sends a new challenge, and both rooms are joined at the later of 11 s after the kick and the release

#### Scenario: B times out, A's frame is late
- **WHEN** B, the higher id, has A's `@_hsready` (or with a password A's challenge) held past B's timeout
- **THEN** B reports one `handshake_timed_out`, drops A's late frames until it sends presence, and both rooms are joined at the later of 11 s after the kick and the release

### Requirement: Rebinding: Correcting room proofs and changing password domains
The app password SHALL remain immutable. Changing it requires a new app object with a distinct self id and newly authenticated signaling; leaving and rejoining a room in the old app MUST NOT change its password. A peer that already authenticated the domain but sent an invalid room proof can instead correct its proof behavior without changing the app; the running backoff is preserved until a completed handshake or the connection's end.

#### Scenario: Correcting an invalid proof preserves the backoff
- **WHEN** two peers authenticate with the same app password, the responder sends incorrect proofs in `r2` through the third kick at 33 s, then starts sending correct proofs at 35 s
- **THEN** the same app domain joins `r2` at the scheduled 77 s rebinding time, with no per-room password change

#### Scenario: Changing the domain password requires another app
- **WHEN** an app with password `old` has joined a room and the program needs password `new`
- **THEN** it constructs another app with password `new` and a distinct self id; its connections, presence and token-advertisement state are not reused from the old app

### Requirement: Rebinding: Reports are bounded
A kick SHALL report to `on_error` as in 0.1.0 (`wrong_password`, `handshake_timed_out`), once per kick. When the app has the lower id, an incorrect room response proof from an already authenticated domain peer costs at most six kicks and six reports in the first ten minutes, then one per 5 minutes, on the lower id only; when it has the higher id, a lower-id peer that misbehaves can bind the room again at its own pace, which costs at most one `handshake_timed_out` report per `handshake_timeout`, as a connection does today. Over an authenticated shared connection an incorrect room response is reported as `wrong_password` by the lower id only: the higher id answers, passes its side and sees the peer leave with no response-proof report. A malformed challenge is rejected by its receiver and can also report wrong_password; the one-per-handshake_timeout bound applies to timeout reports, not to malicious malformed challenges sent at the peer's pace. No announcement is restarted while the connection stays live.

#### Scenario: The higher id reports nothing
- **WHEN** B is the higher id and sends an incorrect room response after authenticating the common app password
- **THEN** B reports nothing and sees A leave

### Requirement: Security: A passwordless room is open to the app's other rooms
A room of a passwordless app SHALL be open to peers connected to that app through any other room: presence gives every such peer the token of every room of the app, relay events expose topics and peer ids, and the token binds the existing connection while an empty app password goes straight to `@_hsready`. A peer can therefore join another room of the same passwordless app without knowing its id. A watcher not connected to that app still needs the room id to derive topics and signaling keys. A protected room MUST belong to a separate app with a password and a distinct self id. Connections, presence, token advertisements and lookup registries MUST be scoped to each app object, with no reuse between those apps even when their `app_id` or room id is equal.

#### Scenario: Password domains are isolated in both id orders
- **WHEN** a program holds a passwordless app with `lobby` and `secret`, and a separate protected app with a distinct self id and `vault`, and a peer joins only the passwordless `lobby`
- **THEN** it can enter `secret` by echoing its advertised token, but sees no `vault` presence on that connection, obtains no protected connection or binding by injecting the protected token, and no protected peer join is reported, with either local id order

#### Scenario: The same-password rooms still share
- **WHEN** two C++ apps configured with the same nonempty password join two rooms each
- **THEN** both rooms exchange over one adopted connection per remote peer

### Requirement: Security: Password domains preserve the upstream wire
The library SHALL preserve the Trystero JS 0.26.0 wire, including its one-way room handshake. Admission to a protected C++ app's rooms SHALL be authenticated by the initial signaling sealed with that app's immutable password before the connection is adopted. Every room of that app uses the same password, so a peer authenticated through another room already knows the domain password; presence MUST NOT cross into an app with a different password. This is an app-domain restriction, not a bilateral handshake change on the wire.

A Trystero JS page that itself mixes passwords across rooms of one app retains the upstream exposure: its higher-id responder can pass after answering a challenge from a peer that does not know that room's password, and its answer can supply material for an offline password guess. The library MUST document that its C++ app-domain guarantee protects its own admission and does not repair a mixed-password remote JS page. JS peers SHOULD use one password for all rooms of one app and separate page contexts for different password domains.

#### Scenario: The page states the boundary and the upstream caveat
- **WHEN** the protocol page's "What remains" is read
- **THEN** it states the immutable C++ app password, initial signaling authentication, isolation between app objects, passwordless app membership by token, and the unchanged one-way exposure of an upstream JS page that mixes passwords, without claiming a wire fix

#### Scenario: A domain member sends an invalid proof
- **WHEN** a peer first authenticates the same app password through signaling, then sends an incorrect room response on that connection
- **THEN** the room kicks the faulty binding, reports `wrong_password` at the lower id only and retains the other rooms and their connection, under the existing rebinding bounds

### Requirement: Security: A forged announcement binds nothing
Over a shared connection a room SHALL bind only a peer that advertised the room's token itself, on that connection, so an announcement or a signal published in X's name by anyone else binds nothing, reports nothing about X and starts no rebinding count, and X joins the room at once when it does. In Trystero JS such an announcement binds X's connection by reuse and X is then kicked after its handshake timeout.

#### Scenario: An announcement in X's name
- **WHEN** a third socket on the relay announces in `r2` in X's name while X has not joined `r2`
- **THEN** A's `r2` binds nothing, reports nothing and publishes no offer, and X then joins `r2` within a round trip

### Requirement: Security: What sharing reveals and every existing protection holds
Presence SHALL give every peer connected to the app, through any room, the token of every room the app is in, enough to join other rooms of a passwordless app and, within a protected app, to reach another room's handshake after domain authentication; one `self_id` per app links the app's rooms for relay observers and peers, where 0.1.0 drew an id per room unless named. Every existing protection holds per room: signals that do not open, replays and reflections, the negotiation and construction bounds, and a peer that floods one room spends that room's bounds, not another's. The page states these in "What remains" and in the usage page's table of `app_config`.

#### Scenario: Two rooms of one app show one id
- **WHEN** two rooms of one app announce on their root topics
- **THEN** both announcements carry the app's one `self_id`

### Requirement: Memory: The app's resource
`app_config::memory` SHALL receive every allocation of the app: its state, the copy of its configuration including the immutable password, its logger and sink and every sentence composed for it, the app's and its rooms', the self id, the registry, every connection, the tokens and frames a connection keeps, its drain order, the presence frames, the peer connections and data channels every room's negotiations make (the signaling is handed the app's resource for `make_peer_connection`), and the frames the backend receives on them. The app's resource MUST outlive the app, every room of it, every value they hand out that is in it, and every operation pending on the executor: declared before the `io_context`. The app parses no JSON, so it has no JSON storage, and each room keeps its own. Every shared object is made with `std::allocate_shared` over `std::pmr::polymorphic_allocator<>`, every callable is a `handler` and every post goes through `detail::post`.

#### Scenario: Every allocation goes through the app's resource
- **WHEN** `every_allocation_goes_through_the_apps_resource` makes an app in a counting resource and joins rooms
- **THEN** the app's state, connections and frames are counted by that resource

### Requirement: Memory: A room's own resource
`room_options::memory` SHALL receive everything else of a room, as `room_config::memory` does in 0.1.0: its state, its bindings and their drain waiters, its negotiations' records, its rebinding counts and backoffs, sealed texts, relays, actions, queues, the frames it writes, transfers, its JSON storage and its handlers; null means the app's resource. A binding's waiters are failed and freed in the room's resource when it detaches, so no object of the app keeps an allocation of a room's resource after the room has ended. A received frame is in the app's resource and becomes the message's buffer; a received `pmr_bytes` moves from its message buffer when that buffer and the room use the same resource, and copies once otherwise; chunked transfers assemble their buffer in the room's resource. A received `std::pmr::string` always copies text once into the room's resource. A room's own resource keeps the lifetime rule of 0.1.0.

#### Scenario: A room's own resource returns to zero
- **WHEN** with `channel_capacity` set and B reading nothing, A's `r2`, in a resource of its own, sends a 1 MB transfer that waits on the full channel, and A destroys `r2` while `r1` goes on exchanging
- **THEN** once the `io_context` has run the posts, `r2`'s resource has no bytes out and `r1` still exchanges

#### Scenario: A received binary payload is copied once
- **WHEN** a binary payload is received by a room with its own resource, and by a room in the app's resource
- **THEN** the first copies it once into its resource and the second moves it

### Requirement: Tests: The memory backend's hooks
`test/support/memory_backend.hpp`'s `memory_network` SHALL offer, for the tests of shared connections: connection ids that are `std::size_t` in the order connections are made; `on_made(pc, servers)`, called when a connection is made, before `make_peer_connection` returns, with its id and the ICE servers it was made with; `peer_of(pc)`, the other end once paired; a `filter(from, to, frame)` returning `verdict::deliver`, `drop` or `hold` (a held frame holds every later frame from that end behind it, unfiltered until released) with `release(to)` delivering the held frames in order, each filtered again; `defer_opens` and `open(pc)`; `inject(pc, frame)`, delivering a frame as if the other end had sent it; `fail(pc)`, reporting `failed` then closing the channel; reports of a disconnection and a reconnection; and a buffering channel (`channel_capacity`, `drain_per_post`, default 16 KiB) that holds what it was sent in the receiving end's resource, delivers a frame once its last byte has drained and fires `buffered_amount_low` at the threshold, `buffered(pc)` giving the bytes held. A release after the sending channel was replaced or the sender went drops its frames. The `std::function` hooks use the default resource, and only when a test sets them.

#### Scenario: A held frame holds the later ones
- **WHEN** `a_held_frame_holds_every_later_frame_of_its_end` holds one frame and sends two more
- **THEN** none of the three is delivered before `release`, and then all three are, in order

#### Scenario: A frame is delivered when its last byte drained
- **WHEN** `a_frame_is_delivered_once_its_last_byte_drained` sends a frame larger than `drain_per_post`
- **THEN** it is delivered after as many posts as the bytes need, and not before

#### Scenario: A failed connection reports failed then closes
- **WHEN** `a_failed_connection_reports_failed_then_closes` calls `fail`
- **THEN** the state `failed` is reported, then the channel closes

### Requirement: Tests: The virtual clock
With `virtual_clock` set before any room exists, every timer the memory backend makes, and its `now_ms` and `unix_seconds`, SHALL run on a clock that only the test advances: `advance(by)` fires, in order, every timer due up to now + by, running the `io_context`'s ready work between them, and leaves the clock at now + by. A virtual timer leaves the clock when it is restarted, cancelled or destroyed, so a room's resource is empty as soon as the room is gone. Every virtual run is bounded by `max_steps` timers (10^6): `advance` and `run_until` fail the case by name past it, so a timer re-armed without delay fails a test instead of hanging it. The timing cases of `shared_test.cpp` assert instants exactly, with no wall-clock slack and no wall-clock wait.

#### Scenario: The clock moves only when advanced
- **WHEN** `the_virtual_clock_moves_only_when_advanced` waits on a timer without advancing
- **THEN** the timer does not fire, and fires after `advance` passes its time

#### Scenario: A timer re-armed without delay stops the clock
- **WHEN** `a_timer_rearmed_without_delay_stops_the_clock` re-arms a timer with delay 0 forever
- **THEN** the run fails by name at `max_steps`

### Requirement: Tests: The shared connection and the app are tested over the memory backend
The library SHALL test the connection layer in `connection_test.cpp` (the connection alone over a memory pair, on the virtual clock), the app's state in `app_impl_test.cpp`, the memory backend's hooks in `memory_backend_test.cpp`, and the sharing in `shared_test.cpp`, with the targets `shared`, `shared_no_exceptions` (`<exception-handling>off`) and, natively, `shared_hardened` (built with the standard libraries' checks: `_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`), each declared in `test/Jamfile`. Two `app` objects stand for two peers; every shared-proof case first authenticates the common app password and fixes which side has the lower id, since only the initiator reports an incorrect room response as `wrong_password`. Tests MUST NOT construct mixed-password rooms in one C++ app. A case in which a peer leaves and comes back with the same id destroys the peer's `app` between visits or sets `idle_timeout` to 0, since an idle connection kept by the peer's `app` would replace the new negotiation. The cases of the specification (two rooms, three rooms, a late join, binding needs the peer's own presence, kept frames, a second connection dropped, crossed connections, one room leaves, a peer leaves every room, a kick is per room, an action-level kick, a connection's end, unknown tokens, flood isolation, joining, the farewell, lifetime, the self id, a passwordless room, the rebinding rate, a room's own resource, a transient failure, a corrected room proof, a changed password requiring a new app, password-domain isolation, a forged announcement, a kept `@_leave`) are written to the increment that brings each.

#### Scenario: The suite of the app
- **WHEN** `b2 libs/trystero/test//shared` runs
- **THEN** `every_room_has_the_apps_id`, `each_setup_report_reaches_the_current_handler_once`, `joining_is_refused_twice_and_beyond_64`, `two_apps_with_one_id_never_meet`, `two_rooms_share_one_connection`, `three_rooms_share_one_connection`, `a_second_connection_is_dropped`, `a_moved_from_app_releases_nothing`, `an_app_destroyed_leaves_its_rooms_going`, `an_app_assigned_over_releases_its_state` and `an_app_assigned_to_itself_keeps_its_state` pass

### Requirement: Tests: Allocation of two apps
`allocation_test.cpp` SHALL gain a case with two apps, each with two rooms, one room in the app's resource and one in a resource of its own: from construction until the `io_context` drains, the global `operator new` sees only the rooms' JSON storage counters and the C library nothing, natively and on Emscripten; a binary payload received by the room with its own resource is copied into it once and by the other moved.

#### Scenario: Two apps count their own resources
- **WHEN** the case runs with counting resources for each app and each own-resource room
- **THEN** the global `operator new` sees only the JSON storage counters, and each resource is zero once the `io_context` has drained

### Requirement: Tests: Interop with Trystero JS on both lanes
The `interop_browser` and `interop_native` lanes SHALL gain the sharing scenarios against Trystero JS 0.26.0, the C++ peers (`peer.cpp`, `native_peer.cpp`) making one `app` and joining `room`, `room2` and, with a new parameter, `room3` through it, and `js.html` honouring `idle` (`_test_only_sharedPeerIdleMs`) and recording each `RTCPeerConnection` it makes and its state: two rooms on one connection (the page holds exactly one connected `RTCPeerConnection` at the end); three rooms with the third joined by the C++ peer 5 s after the others, by presence; a room the page joined first, bound over the existing connection by the page's presence within 5 s; passwordless-app room admission by token and isolation from a separate protected app with a distinct self id, in both id orders; one room leaving; a peer leaving every room, the connection closing after the idle time both sides are given (2 s); incorrect room proofs after authenticated common-password connection establishment, in both id orders (the C++ peer as the lower id reporting `wrong_password` 3 times in 60 s at about 0, 11 and 33 s; as the higher id reporting nothing while the page records Trystero's password error at most 4 times); a flood in one room while `room2` pings within 2 s; and a transient handshake failure with Trystero JS in both id orders. The existing scenarios pass unchanged in meaning with the C++ peer an `app` of one room. The `oracle` lane and the `protocol` test do not change: no wire value changes.

#### Scenario: Two rooms on one connection
- **WHEN** the C++ peer and the page each join `room` and `room2`, no password
- **THEN** both rooms' exchanges complete both ways (join, chat, the 100 KB file) and the page holds exactly one connected `RTCPeerConnection`

### Requirement: Docs: The page, the rulebook and the migration
The documentation SHALL state the app and the migration: `doc/differences.adoc` loses the row "One connection shared by every room of an app between two peers" and "A peer id per room, not per app" becomes the table of the differences that remain (the scope of sharing and of the self id, a peer kicked from a room, reuse over a live connection, announcements after a peer leaves, presence after binding, an answer to an earlier challenge, frames after the higher id's own kick, joining a room already joined, rooms per app, a connection whose last room was kicked, relay sockets, relay and ICE configuration, `idle_timeout`, frames waiting for a bound room's handler, renegotiation and media, passive rooms, crossed duplicate connections); `doc/roadmap.adoc` loses "one connection shared between several rooms of an app"; `doc/usage.adoc`'s "Joining" becomes the app and its rooms with the tables of `app_config` and `room_options`, a subsection "Moving from `room_config`" mapping each member and use to its new place, what a kick does over a shared connection and when the peer is bound again, and that over a shared connection an incorrect room response is reported as wrong_password by the lower id only; `doc/protocol.adoc` says what presence does and how frames are routed, the new bounds and the backoff, and the security items; `doc/design.adoc` gains a section on ownership; `doc/allocation.adoc`, `doc/guarantees.adoc` (invariant 3 scoped to C++ password-domain admission and new invariants: one connection per remote peer per app, a room's departure leaving the app's other rooms joined, different password domains sharing no C++ connection or presence), `doc/testing.adoc`, `doc/overview.adoc`, `doc/quick-start.adoc`, `doc/browser.adoc`, `doc/native.adoc`, `README.md` and `AGENTS.md` follow, with the examples and the interop peers, and the header comments that name `room_config` or the room's resource.

#### Scenario: The migration table is in the usage page
- **WHEN** `doc/usage.adoc` is read
- **THEN** its subsection "Moving from `room_config`" maps `room_config::{app_id, self_id, password, relay_urls, relay_redundancy, ice_servers}` to `app_config`, `room_config::log` to `app_config::log`, `room_config::{handshake_timeout, max_receive_bytes}` to `room_options`, `room_config::memory` to `app_config::memory` and `room_options::memory`, and the standalone constructor to `app::join`
