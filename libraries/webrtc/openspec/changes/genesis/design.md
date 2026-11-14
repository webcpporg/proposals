## Context

webcpp::webrtc is a header-only C++20 library for peer connections and data channels, shaped as
the W3C WebRTC API and written in Asio's model. The original specification, dated 2026-10-10 with
the owner's decisions of that day and the controller's rulings on a review, is preserved in
`reference/spec.md`. This revision, of 2026-11-14, checks it against what was built since and
supersedes it where they differ; "What changed since the specification" below lists every
difference, and where this document and `reference/spec.md` disagree, this document and the
requirements rule.

What the library stands on, as built on 2026-11-14:

- **webcpp::asio** (`libraries/asio`, status `implementing`, repository `webcpporg/asio`, the
  superproject's `libs/asio` at `6c4e2c8`): Boost.Asio itself natively, the shipped backend on
  Emscripten and WASIp2/P3, with the portable names `webcpp::asio::any_io_executor`,
  `webcpp::asio::post` and `webcpp::asio::error`. Its browser bridge exists:
  `<webcpp/asio/browser/bridge.hpp>`, a service of the io_context whose JavaScript functions
  deliver to a `registration` or an `intrusive_registration` by a key that is never reused, and
  only post (asio's D7, D12).
- **webcpp::beast** (`libraries/beast`, the superproject's `libs/beast` at `6c53f85`): its
  browser WebSocket (`include/webcpp/beast/browser/websocket.hpp`, `detail/imports.hpp`,
  `detail/support.hpp`) is the first browser object built on that bridge, and the patterns it
  proved (beast's D3, D7, D9 to D12) are the ones this library follows in the browser.
- **libdatachannel 0.24.6**, pinned and built from its sources by the superproject:
  `third_party/libs/libdatachannel` at `6b1e2e6`, with libjuice, libsrtp, usrsctp and plog at the
  gitlinks `tools/libdatachannel/pin.json` lists, built as static archives by upstream CMake
  through B2 (`/webcpp/third_party//libdatachannel`) against the same source-built OpenSSL as
  Asio and Beast (`CODING-GUIDELINES.md` chapter 10, "Third-party code"). trystero's native
  backend already ships it.
- **trystero's `backend`** at trystero `e9326e9` (`include/webcpp/trystero/backend.hpp`,
  `native/peer_connection.hpp`, `native/data_channel.hpp`, `browser/backend.hpp`): the nearest
  existing code, a virtual interface with handler callbacks and one implementation per target,
  which this library replaces with value types and asynchronous operations. Since the
  specification was written it gained the shared connection (one peer connection per remote peer
  per app, adopted by the app when its data channel opens and multiplexed among the app's rooms)
  and the preservation of native frames that arrive before a handler is installed (a FIFO of 64
  frames for 10 seconds).

Constraints every decision works within, from the superproject's `CODING-GUIDELINES.md`:

- header-only C++20, namespace `webcpp::webrtc`, headers under `<webcpp/webrtc/...>`, internals
  in `detail`, macros `WEBCPP_WEBRTC_*`, the b2 target `/webcpp/webrtc//webrtc`, the error
  category `webcpp.webrtc` (chapter 3);
- errors as values; no global or static mutable state; run-time objects allocate through the
  allocator their user gives; asynchronous code runs on the executor its user supplies, without
  driving the run family; an outside library is wrapped so that the user never names it
  (chapter 4);
- every library works with exceptions on and off, and its `config.hpp` defines
  `WEBCPP_WEBRTC_NO_EXCEPTIONS` (chapter 6);
- third-party code only as a pinned submodule, by the owner's decision (chapter 10);
- every test on each target its Jamfile declares (chapter 8); BSL-1.0, "Copyright (c) 2026
  WebCpp.org" on every source file; English; no em dash (chapter 2).

## Goals / Non-Goals

**Goals**

- One interface for peer connections and data channels that a program writes once and runs
  natively and in a browser, with the W3C API's names and shapes.
- Operations in Asio's model: completion tokens, handlers on the object's executor, the
  cancellation Asio defines, outstanding work, the execution context outliving the object.
- Events as pending operations with stated policies, so that no event is lost silently and every
  lifetime stays in the caller's hands.
- Every limit webcpp imposes is a named option and fails as a value; every platform limit is
  stated in a table of differences.
- The library works with exceptions on and off on both targets.
- A native peer and a browser peer connect to each other in the tests.
- trystero can be rewritten on it without loss (the table "trystero's backend, part by part"
  below).

**Non-Goals**

- Media: tracks, codecs, transceivers. Media will get an interface of its own.
- wasip2 and wasip3 targets (D3). A later WASI target is a change of its own.
- `createOffer`, `createAnswer` and the `negotiationneeded` event (D15).
- A native handle (`rtc` ids, `rtc::` or `emscripten::val`) in any public header.
- Zero-copy receive, for now (D9).
- A byte stream over a data channel (an `AsyncReadStream` and `AsyncWriteStream` adapter), which
  webcpp::beast's D6 names as an opt-in next layer for its WebSocket: a later change of this
  library, which beast would then use (D1).
- Policies of trystero's: the shared connection and its adoption, dropping text messages,
  dropping frames larger than a room frame, the bound of 64 frames and 10 seconds on frames no
  room has read, the per-minute window of connection attempts.
- An integration with xactor inside the library (D17).

## Decisions

### D1. A library of its own, of kind own, on webcpp::asio

**The owner's decision of 2026-11-14:** webcpp::webrtc stays a library of its own, not a part of
webcpp::beast, on webcpp::asio, following the browser patterns webcpp::beast proved. The
reasons:

- webcpp::beast is a source-compatible facade of Boost.Beast, whose types and control flow it
  keeps; WebRTC has no Boost counterpart to be faithful to, so its interface is webcpp's own.
- libdatachannel (MPL-2.0, with libjuice, libsrtp, usrsctp and plog) and its threads would be a
  burden on every user of webcpp::beast, most of whom want HTTP or a WebSocket.
- webcpp::beast targets WASIp2 and WASIp3, where no WebRTC stack exists (D3).
- webcpp::beast's D6 already foresees a WebRTC channel's byte stream as an opt-in next layer of
  its WebSocket, so the useful relation is beast over webrtc, later, never webrtc inside beast.

This confirms the owner's decision 2.1.1 of 2026-10-10 ("a library of its own, on
webcpp::asio"). The metadata says `kind: own`: on the native target the library wraps
libdatachannel, but the interface, the semantics and the bounds are webcpp's (the event
policies, the `runtime`, the error category, the back-pressure rules, the table of differences),
specified independently of libdatachannel, and its second implementation is not a library at
all but the browser's objects. There is no upstream whose tests prove the library (so not
`port`), and no upstream interface it keeps (so not `wrapper`). libdatachannel is justified in
the documentation as a dependency of one backend (D16).

Rejected: WebRTC inside webcpp::beast, for the four reasons above.

### D2. The W3C API's shape and names, in Asio's model

Types and operations carry the W3C names (`peer_connection`, `data_channel`,
`session_description`, `ice_candidate`, the `RTC*` enumerations in snake case, `ready_state()`,
`buffered_amount()`, `close()`). Operations are `async_*` members taking any completion token.

The executor is a template parameter defaulting to `webcpp::asio::any_io_executor`, the portable
name webcpp::asio gives: natively Boost.Asio's own type, on Emscripten the shipped backend's,
so that the browser implementation shares the io_context the rest of the program runs on. Asio's
codes are named through `webcpp::asio::error` for the same reason. The allocator is a second
template parameter, defaulting to `std::allocator<std::byte>`, that the object's own state and
its outputs use (D12). Inputs are views read during the call; outputs are owned, with their text
in the object's allocator.

The specification named `boost::asio::any_io_executor` and said the allocator parameter is "as
Asio's own I/O objects do". Both are corrected: Boost.Asio's type is not the browser's, and
Asio's I/O objects take an executor parameter only, their operations allocating through the
handler's associated allocator, which this library does too (D12).

### D3. Targets: native and Emscripten

wasip2 and wasip3 are not targets, because there is no WebRTC stack for WASI, neither in the
guest nor from the host: libdatachannel has no WASI port (its thread pool, and libjuice's and
usrsctp's threads, need preemptive threads that wasip2 does not give and that wasip3's
cooperative profile, experimental in wasi-sdk 34, does not give either; its ICE and SCTP layers
were never ported to WASI's sockets), and no WASI host interface offers WebRTC as the browser
offers `RTCPeerConnection`. The build declares native and emscripten only, and the convenience
header stops with `#error` under `__wasi__`, naming this reason. Rejected: a stub for WASI (it
would let a program compile and fail at run time).

The specification's reason also said that Boost.Asio does not build for WASI with the pinned
wasi-sdk. That is no longer true: webcpp::asio runs on WASIp2 and WASIp3 since 2026-11-03, and
webcpp::beast's guest sockets with it. The reason above is the one that still holds.

### D4. Exceptions: on and off on both targets, through libdatachannel's C API

libdatachannel reports errors by throwing, and has no option to build without exceptions: its
sources throw in about three hundred places, and the superproject's build compiles its archives
with `-fexceptions` whatever the consumer asks (`tools/libdatachannel/build.py`). Its C++ API
(`rtc::PeerConnection`, `rtc::DataChannel`) lets those exceptions reach the caller, which is why
trystero's native backend, written on it, needs exceptions. Its C API (`rtc/rtc.h`) does not:
every function of `src/capi.cpp` runs inside `wrap()`, which catches `std::invalid_argument` as
`RTC_ERR_INVALID` and any other `std::exception` as `RTC_ERR_FAILURE`, so no exception crosses
it.

**The owner's decision of 2026-11-14:** the native backend calls libdatachannel's C API only, so
webcpp::webrtc works with exceptions on and off on both targets, as chapter 6 asks of every
library. Measured on 2026-11-14 (`reference/STATUS.md`): a program compiled with
`-fno-exceptions` against the pinned libdatachannel connected two peers over the loopback through
the C API, exchanged a message, and received `RTC_ERR_INVALID` for a malformed ICE server URL,
with no exception. Each negative code becomes a value of `webcpp.webrtc` (D10); nothing of the C
API appears in a public header.

What it costs: the C API names objects by `int` ids in a map of libdatachannel's own, behind its
mutex, and passes each callback a `void*` user pointer instead of a closure; and
`rtcReceiveMessage` copies a message out of libdatachannel's queue (`Channel::peek`) before it
copies it into the caller's buffer, one copy and one allocation more per received message than the
C++ API's `receive()`, both inside libdatachannel and in its allocator. Increment 1 measures that
cost (tasks.md 1.11).

Rejected: the C++ API with exceptions required natively (the specification's ruling 2.2.2), whose
native headers declare nothing without exceptions; it leaves a program built without exceptions
without the native backend, when the C API gives it at a small cost. Rejected with it: a declared
stub whose use fails a `static_assert`, which departs from chapter 6. The specification's open
question on the compile-time message is closed: there is no such message.

### D5. Events are pending operations the caller re-arms

An event is a wait (`async_wait_ice_candidate`, `async_wait_state_change`,
`async_wait_ice_connection_state_change`, `async_accept_data_channel`, `async_wait_open`,
`async_wait_close`, `async_wait_buffered_amount_low`, and `async_receive`), not a callback. This
keeps every handler on the executor and every lifetime in the caller's hands, as an acceptor's
`async_accept`. While no wait is pending, each kind has a stated policy: state changes coalesce
to the latest value; candidates queue up to `max_queued_candidates` and the end of gathering has
a place the bound keeps for it; incoming channels queue up to `max_queued_channels` and a channel
past the bound is closed at once, as a connection past a listen backlog is refused. One wait of a
kind at a time (a second completes with `already_started`); a cancelled wait consumes no event.
Rejected: one wait returning a variant of events (the review's answer), and callbacks.

webcpp::beast has one read and keeps arrivals in a queue whatever is pending, which is this
library's `async_receive`; it has no other event kinds. Its rule that a queued arrival does not
keep `run` alive holds here too: only a pending operation is outstanding work (D11).

### D6. A `runtime` owns the platform's process-wide state

libdatachannel keeps process-wide state (`rtcPreload`, `rtcCleanup`, `rtcSetThreadPoolSize`,
`rtcInitLogger`), and its threads must be stopped through `rtcCleanup()` before static
destruction. webcpp keeps no global or static mutable state, so the program owns that state
through a `runtime`: made before any peer connection and destroyed after the last; `make` takes
it by reference, so no connection can exist without one. Its other duty is the cap of peer
connections on both targets (`max_peer_connections`, default 256, half of Chromium's 500 per
page, counted from construction until garbage collection): natively it counts, in a member, the
live connections made with it; in the browser it reads the page's census, which lives in
JavaScript.

At most one `runtime` exists at a time in a native process, because libdatachannel's state is the
process's: a second one's destructor would clean up the first's connections. This is a
precondition the documentation states, not a check: a check would need a flag of the process,
which is the static mutable state chapter 4 forbids. The specification required that the library
refuse a second `runtime`; that requirement is corrected to the precondition.

Rejected: a hidden singleton (static state), a per-connection cap (it cannot see the page), and a
check through an unused id of libdatachannel's user-pointer map (it relies on what libdatachannel
does not document).

### D7. Native events are posted, and destruction is synchronous with libdatachannel's threads

libdatachannel's callbacks run on its own threads. Each callback the backend installs reads its
user pointer, a small control block the object owns, copies what the event carries into storage
of the standard allocator, and posts to the executor through a `std::weak_ptr` to the object's
state; it never runs a handler there, and touches nothing of the user's allocator: the queues of
D5 are filled on the executor. Destroying a channel calls `rtcDeleteDataChannel`, destroying a
connection `rtcDeletePeerConnection`, which libdatachannel documents to block "until all scheduled
callbacks of `pc` return" and to call no callback for it afterwards (`DOC.md`,
`rtcDeletePeerConnection` and `rtcDelete`); only then is the inbox freed, and what was posted
before finds nothing through the weak pointer. Destruction runs on the executor, never on
a libdatachannel thread, so it never waits for itself.

The specification relied on `resetCallbacks()` of the C++ API, which synchronises with a
callback in flight through each callback's recursive mutex; `rtcDelete*` does the same, and says
so in libdatachannel's documentation rather than in its internals.

The execution context outlives the object, as for Asio's I/O objects, which is what keeps a
libdatachannel thread from posting to a destroyed `io_context`.

### D8. Browser events enter through webcpp::asio's bridge, as webcpp::beast's do

The browser implementation follows webcpp::beast's browser WebSocket as built:

- **One registration per object.** The connection's state and each channel's state is an
  `intrusive_registration` of the io_context's bridge (`<webcpp/asio/browser/bridge.hpp>`), so
  the bridge indexes it without allocating, and one bridge function per object receives all its
  events, which `deliver()` sorts by the event's `type`.
- **Arrivals are staged, and one notification is posted.** `deliver()` never runs a handler and
  never touches a caller's buffer: it records the arrival in the object's state (charging a
  message to the receive budget at once, D9) and posts at most one notification to the executor
  for all arrivals not yet drained, which completes the pending waits there. A generation number
  makes a notification posted for an earlier state find nothing.
- **JavaScript stays in JavaScript.** The browser calls are `EM_ASM` functions in inline
  functions of `detail/imports.hpp`, which catch every JavaScript exception in JavaScript, before
  it can reach a WebAssembly frame, and report it as a value (the DOMException's `name`); a
  promise is followed by the same bridge function, whose event says how it settled. So the
  emscripten backend raises nothing of its own and works with exceptions on and off.
- **Destruction and shutdown.** The destructor completes every pending operation with
  `operation_aborted` (posted), sets the JavaScript handlers to null, and removes the
  registration. The io_context's shutdown calls the registration's `shutdown()`, which destroys
  the pending handlers without calling them, as Asio's own operations are destroyed.

Where it differs from beast: a peer connection has several event kinds and the waits of D5,
where a WebSocket has one read; and a promise of `setLocalDescription` or
`setRemoteDescription` cannot be cancelled, so its operation supports terminal cancellation only
(D11).

### D9. Receiving and sending

`async_receive` appends one whole message to the caller's dynamic buffer and reports whether it
was text. No zero-copy receive for now: in the browser the bytes must leave the JavaScript heap
anyway, and natively they leave libdatachannel's queue through `rtcReceiveMessage`, which copies
into the caller's buffer directly. Messages are kept from the channel's creation, so a channel
accepted later, or handed from one owner to another, loses nothing it received before its first
`async_receive`: trystero's adoption of a shared connection relies on it (the table below).

- **Natively** the queue is libdatachannel's own, in its pull mode (no message callback;
  `rtcReceiveMessage`, `rtcGetAvailableAmount` and the available callback), bounded at 1,024
  messages (`RECV_QUEUE_LIMIT`, `src/impl/internals.hpp:46` at `6b1e2e6`). Past it libdatachannel's
  thread waits for room (`src/impl/queue.hpp:93`), SCTP stops reading and the peer is slowed:
  real flow control, never a failure. That thread delivers the connection's other channels too:
  measured on 2026-11-14 (`test/stall_test.cpp`), a channel flooded past 1,024 unread messages
  stalls every other channel of its connection until it is read or destroyed. So a program reads
  every channel it holds, or closes it, and the documentation says so. Closing a channel does not
  stop libdatachannel's queue (`DataChannel::close()`, `src/impl/datachannel.cpp:97-113`, closes
  the stream and resets the callbacks only), so the backend drains it when the channel's owner
  closes it, when its connection closes and when it is destroyed, which releases the waiting
  thread; increment 1 tests it (3,000 messages never read, then the channel destroyed in under
  two seconds). Draining discards what is unread, as the W3C discards a message received once the
  channel left "open"; messages that arrived before the peer closed the channel are still
  delivered, then `async_receive` completes with `error::closed`.
- **In the browser**, which has no flow control on receipt, a queue bounded in bytes
  (`max_received_bytes`) is the only protection. As webcpp::beast's `queue_limits::receive`
  does, the budget counts each message's payload and the bookkeeping of its record, and is
  charged when the browser delivers the message, before the executor drains it, so that a
  stopped or busy io_context does not let the page's memory grow. Past it the channel closes with
  `receive_overflow`.

`send` and `send_text` queue and return; the `async_send` forms complete once the message is
queued (at once, posted), because both of trystero's backends send synchronously, fire and
forget, as Asio's sockets offer `send` beside `async_send`. Each fails as a value:
`invalid_state` when the channel is not open, `message_too_large` past the smaller of
`max_message_size` and the peer's limit, `buffered_amount_exceeded` when the message would pass
`max_buffered_amount`. The caller paces itself with `async_wait_buffered_amount_low`.

Where it differs from webcpp::beast's D7: beast closes the WebSocket with
`browser_errc::send_limit` when a write passes its bound, because a fragmented message already
partly given to the browser cannot be taken back; a data channel's message is sent whole by one
call and refused before the browser sees it, so the channel stays open and the caller can wait
and retry. The defaults differ too: beast's two bounds are 4 MiB each; this library keeps the
specification's 16 MiB for both, since data channels carry files where a WebSocket carries
signalling.

### D10. Errors: the category `webcpp.webrtc`, the browser's names and the C API's codes

Errors are values: `webcpp::webrtc::error`, category `webcpp.webrtc`, ten enumerators with fixed
values that are never reused (`invalid_state` 1 to `too_many_peer_connections` 10). Asio's codes
keep their meaning: `operation_aborted`, `already_started`, `operation_not_supported`, through
`webcpp::asio::error`.

- **In the browser** a rejected promise or a thrown exception arrives as its DOMException's
  `name`, read in JavaScript as beast's `webcpp_beast_open` reads `SyntaxError` and
  `SecurityError`: `InvalidStateError`, `OperationError`, `InvalidAccessError`, `TypeError` and
  `SyntaxError` become the enumerator of the same name, any other name `operation_error`.
  webcpp::asio's `promise_rejected` never reaches the caller.
- **Natively** a negative code of the C API becomes, per call, the code the browser gives for the
  same call (`RTC_ERR_INVALID` on `rtcCreatePeerConnection` is a malformed ICE server URL,
  `syntax_error`; on `rtcSetRemoteDescription` a description that cannot be parsed,
  `operation_error`, as Chromium reports it), and `operation_error` where nothing more precise
  applies. The table is in the plan (Task 1.4) and the documentation.

Like webcpp::beast's D10, the category holds only what has no Asio counterpart; unlike beast,
whose `browser_errc` covers the browser alone, its values cover both targets, because the W3C
errors are the interface's own.

### D11. Cancellation, per operation and per type

Every operation installs a cancellation handler on its associated cancellation slot, as
webcpp::beast's operations do (beast's D11), and states the types it supports:

| Operation | Supports | Effect of a cancellation |
| --- | --- | --- |
| the waits of D5 and `async_receive` | terminal, partial, total | completes with `operation_aborted`; consumes no event and no message |
| `async_set_local_description`, `async_set_remote_description` | terminal | completes with `operation_aborted`; the description may still be applied, after the handler ran |
| `async_add_ice_candidate`, `async_send`, `async_send_text` | terminal, partial, total | none in practice: each completes at once, posted |

A pending operation is outstanding work on the object's executor and on its handler's associated
executor (two work guards, as beast's `operation` holds), so natively `run()` does not return while
a candidate or a state change is awaited; a program that re-arms its waits keeps `run()` alive
until it closes the connection, and that is the way to end it.

### D12. Allocation

- **The object's state** (the connection, its channels, the queues of D5, the browser's receive
  queue, the text of descriptions, candidates and labels) is allocated through the object's
  `Allocator`, on the executor.
- **An operation's state** is allocated through its handler's associated allocator, as Asio's
  and webcpp::beast's operations are (beast's D12).
- **What runs on libdatachannel's threads** allocates through the standard allocator only (the
  copies an event carries, Asio's posts with no associated allocator), never through the user's,
  which may be a resource used only on the executor. **What libdatachannel and the browser hold
  is theirs:** the C API's maps and the messages in libdatachannel's queues, or what
  `emscripten::val` holds in the JavaScript heap.

None of the types is shareable in chapter 4's sense: they are run-time objects. The allocator
outlives the io_context's pending handlers, as trystero's resource does, since a posted
notification may hold the state until it runs.

Where it differs from webcpp::beast's D12: beast takes the allocator through a constructor
`stream(std::allocator_arg_t, const Allocator&, Args&&...)` and erases it into a memory resource,
because its stream type must stay Boost.Beast's `websocket::stream<NextLayer>`; this library has
no upstream type to keep, so the allocator is a template parameter, as a standard container's,
given to `make`, and its outputs carry it by value.

### D13. `close()` fires no state change

As the W3C's `close()`: every pending operation of the connection completes with
`operation_aborted`, `state()` reads `closed`, no state-change event is delivered (natively the
backend drops the `Closed` states libdatachannel reports while closing), and the connection's
channels then close with `error::closed` on their pending operations. A data channel's `close()`
completes its pending operations with `operation_aborted`, except `async_wait_close`, which
completes once the channel is closed. When a channel closes by other causes (the peer, the
connection, its transport) its pending operations complete with `error::closed` and
`async_wait_close` completes successfully; trystero ends a link on that.

### D14. What a target cannot honour follows webcpp::beast's rule, and its mechanism

`operation_not_supported` under `WEBCPP_WEBRTC_UNSUPPORTED_ERROR` (the default), ignored under
`WEBCPP_WEBRTC_UNSUPPORTED_IGNORE`, as beast's D3; a setting is ignored by default only where the
platform does an equivalent thing of its own, with the reason. As beast's D9: both macros defined
is an `#error`; the b2 feature `webcpp-webrtc-unsupported=error|ignore`, propagated, sets one; and
the browser classes live in an inline namespace named after the policy (`unsupported_error`,
`unsupported_ignore`), so that translation units that disagree name different types and fail to
link. The native classes, which honour every setting, are outside it.

Natively a remote candidate's `username_fragment` is ignored whatever the policy, which is such
an equivalent: libdatachannel creates one ICE transport per connection and never restarts it (no
path of `src/impl/peerconnection.cpp` at `6b1e2e6` does), so every candidate it is given belongs
to that transport's session, and a candidate's own `ufrag` attribute, when its text has one, still
reaches it. A browser peer's candidates carry the field, so refusing it would
break the interop that D18 tests.

Natively a text message holding a zero cannot be sent: the C API takes a text as a
zero-terminated string. The send fails with `operation_not_supported` whatever the policy, since
ignoring it would truncate the message silently, which beast's rule forbids.

The documentation keeps the table of differences (`bind_address`, local `sdp_m_line_index` and
`username_fragment`, a remote candidate's `username_fragment` natively, a text holding a zero
natively, a received message past `max_message_size`, unread messages and the stall of D9, the
omitted operations, no native handle).

### D15. Left out of the W3C API: `createOffer`, `createAnswer`, `negotiationneeded`

libdatachannel marks `createOffer` and `createAnswer` "for specific use cases only", and its
`setLocalDescription` takes a type, not a description, so a description created that way cannot
be applied as the W3C allows; the implicit `setLocalDescription`, the one trystero uses, does
both. libdatachannel has no `negotiationneeded` event, only a query (`rtcIsNegotiationNeeded`).
Natively automatic negotiation is off (`rtcConfiguration::disableAutoNegotiation`), so on both
targets a description exists only when `async_set_local_description` asks for one, and the
program negotiates when it adds a channel. A later version may add the event, synthesised
natively. Both omissions are in the table of differences.

### D16. libdatachannel, pinned, wrapped and justified

The native implementation wraps libdatachannel behind the interface, through its C API (D4);
nothing of it appears in a public header. The documentation justifies it as chapter 4 asks: no
WebRTC exists in webcpp, Boost, the C++ Alliance or the standard library, and libdatachannel is
the smallest complete implementation (data channels without a media engine), with its DTLS on
the same source-built OpenSSL as Asio and Beast. webcpp uses it unmodified, as a dependency, and
copies none of its files.

It is pinned, as the superproject builds it today: libdatachannel 0.24.6 at `6b1e2e6` in
`third_party/libs/libdatachannel`, its nested sources at the gitlinks of
`tools/libdatachannel/pin.json`, through `/webcpp/third_party//libdatachannel`, whose usage
requirements give the include directory, `RTC_STATIC` and the archives; a native consumer builds
with `threading=multi`. The specification's `-sLIBDATACHANNEL_ROOT` and
`.local/libdatachannel` no longer exist: neither selects an installed copy now. A constant this
library quotes from libdatachannel (`RECV_QUEUE_LIMIT` 1,024, `DEFAULT_LOCAL_MAX_MESSAGE_SIZE`
256 KiB, `DEFAULT_REMOTE_MAX_MESSAGE_SIZE` 64 KiB) is checked again when the pin moves.

**The owner's decision of 2026-11-14 on the licence:** MPL-2.0 is accepted for libdatachannel as
a dependency of webcpp::webrtc, used unmodified, as trystero already distributes it; its nested
dependencies keep their own licences (the superproject's third-party table). MPL's file-level
copyleft reaches none of webcpp's files while the dependency is unmodified.

### D17. How it meets xactor

It does not: webcpp::webrtc knows nothing of xactor. They meet at the executor. An xactor actor
that needs a peer holds a `peer_connection` made on the executor its driver runs on, starts
operations from its turns, and turns each completion into a message to itself.

### D18. Tests: every lane on both targets, interop with a real browser

Every lane runs on native and on emscripten, both suspension variants, at the owner's three CI
levels. Each target alone: two peers in one process or one page, every case the semantics name.
Native lifetime: destruction while libdatachannel's threads deliver events, repeated, under
ThreadSanitizer (100 rounds clean on 2026-11-14; libdatachannel's archives are not instrumented, so
what it checks is this library's side of each hand-off). Interop: a native peer and a headless-Chrome peer, signalled through a local
relay, with Chrome started with `--allow-loopback-in-peer-connection` and
`--disable-features=WebRtcHideLocalIpsWithMdns`, and the native peer binding to `127.0.0.1`.
Allocation: a counting resource. Exceptions: every suite, native and emscripten, once as the build
asks and once with `<exception-handling>off`. No network beyond loopback: host candidates only,
no STUN.

### D19. Increments

1. The interface, the `runtime` and the native implementation, with their tests (reference code
   in `reference/source/`, `reference/plan.md` increment 1).
2. The browser implementation, its tests and the interop lane.
3. trystero on webcpp::webrtc (with webcpp::asio, webcpp::beast for its relays and
   webcpp::crypto), its `backend` gone: a change of trystero's.

## trystero's backend, part by part

Every part of trystero's `backend` at `e9326e9` that touches WebRTC has a home here; this replaces
the specification's section 3, whose line numbers were trystero's of 2026-10-10.

| trystero at `e9326e9` | webcpp::webrtc |
| --- | --- |
| `factory::make_peer_connection`, null on refusal (`backend.hpp:353`) | `basic_peer_connection::make`, a `result` |
| `peer_connection::set_local_description`, completing with the type and SDP (`backend.hpp:251`) | `async_set_local_description`, completing with a `session_description`; `local_description()` |
| `set_remote_description`, `add_ice_candidate` (the candidate as JSON), `has_remote_description` | the same operations; the candidate as a value, its JSON trystero's |
| `peer_connection::handlers::ice_candidate`, `data_channel`, `state` | `async_wait_ice_candidate`, `async_accept_data_channel`, `async_wait_state_change` and `async_wait_ice_connection_state_change` |
| the browser's merge of `connectionState` and `iceConnectionState` (`browser/backend.hpp:650`) | both states, separately; trystero merges them itself |
| `create_data_channel`, null on refusal (`backend.hpp:242`) | `create_data_channel`, a `result` |
| `data_channel::is_open` (`backend.hpp:171`) | `ready_state()` |
| `data_channel::handlers::open`, `message`, `close`, `buffered_amount_low`, and `set_handlers`, which a handler may call during its own call | `async_wait_open`, `async_receive`, `async_wait_close`, `async_wait_buffered_amount_low`, each re-armed by its owner |
| native frames that arrive before a message handler is installed, kept in a FIFO of 64 frames for 10 seconds (`native/data_channel.hpp:47`, `:224-245`) | messages are kept from the channel's creation until `async_receive` (D9); the bound of 64 frames and 10 seconds, Trystero's pending-data bound, stays trystero's, over `async_receive` |
| the shared connection: one peer connection per remote peer per app, adopted when its channel opens and routed among the app's rooms (`doc/protocol.adoc`, "The data channel") | trystero's policy over this library: a `peer_connection` and its `data_channel` are movable values the app owns, one reader at a time per channel, and nothing is lost while one owner hands them to another |
| `data_channel::send`, synchronous, dropped when not open (`backend.hpp:178`) | `send`, synchronous, and `async_send`; both fail as values when not open |
| `buffered_amount`, `low_threshold` (65,535 bytes) | `buffered_amount()`, `set_buffered_amount_low_threshold` |
| `backend_config::bind_address` (`native/backend.hpp:59`) | `configuration::bind_address`, native only (D14) |
| the page's cap of 256 `RTCPeerConnection`s (`browser/backend.hpp:919-927`) | `runtime_options::max_peer_connections` (D6) |
| `native_handle()` (`native/peer_connection.hpp:256`, `browser/backend.hpp:642`) | none; media, which the handle was kept for, will get an interface of its own |
| `backend_config::max_peer_connections_per_minute` (`native/backend.hpp:68`) | stays trystero's: a policy of its rooms against floods, not a limit of a platform |
| dropping text messages and binaries larger than a room frame, on libdatachannel's thread (`native/data_channel.hpp:189-192`) or in the page (`browser/backend.hpp:402-404`) | stays trystero's, on the executor: this library delivers text with `is_text` true and fails a message past `max_message_size` |
| the TURN URL rewritten as `turn:user:credential@host` (`native/peer_connection.hpp:261`) | `ice_server`'s `username` and `credential`, which the native backend writes in that form, the one the C API parses |

The sealer, `random`, the timers, the clocks and the WebSockets go to webcpp::crypto,
webcpp::asio and webcpp::beast.

## What changed since the specification

- D1: the owner's decision of 2026-11-14, a library of its own, not inside webcpp::beast.
- D2: the executor defaults to `webcpp::asio::any_io_executor`; Asio's codes through
  `webcpp::asio::error`; the claim that Asio's I/O objects take an allocator parameter removed.
- D3: the WASI reason corrected (Boost.Asio runs on WASIp2 and WASIp3 now).
- D4: the native backend uses libdatachannel's C API and works without exceptions; ruling 2.2.2
  and section 6 of the specification, the headers that declare nothing and the
  `webcpp.compile-diagnostic` test are withdrawn; the owner's open question on the message is
  closed.
- D6: one native `runtime` at a time is a precondition, not a refusal.
- D7: destruction through `rtcDelete*`, documented by libdatachannel, instead of
  `resetCallbacks()`.
- D9: measured, a native channel never read stalls its connection's others; channels drain their
  queue when closed by their owner or connection, and when destroyed.
- D8, D9, D11, D12, D14: aligned with webcpp::beast as built (one intrusive registration, staged
  arrivals, the receive budget charged on arrival with each record's bookkeeping, cancellation
  per type, operation state in the handler's allocator, the inline namespace and the b2 feature of
  the unsupported policy), with the differences each states.
- D10: the browser's DOMException names and the C API's codes mapped to the category.
- D16: libdatachannel pinned and built from `third_party/`; MPL-2.0 accepted by the owner.
- The table of trystero's backend at `e9326e9`, with the shared connection and the preservation of
  early native frames.

## Risks / Trade-offs

- [The C API costs a copy and an allocation per received message inside libdatachannel] ->
  to be measured against the C++ API's `receive()` before increment 1 closes (tasks.md 1.11); the
  cost is libdatachannel's, not the user's allocator's.
- [A native channel that is never read stalls its connection's other channels once 1,024
  messages wait] -> measured in increment 1 and pinned by a test; the documentation tells a
  program to read every channel it holds or close it; closing or destroying the channel drains its
  queue and releases the others (D9). A per-channel byte bound natively, as the browser's, would
  trade the flow control for independence (O3).
- [A cancelled description operation still takes effect] -> stated on every such operation's
  page; the W3C promise cannot be cancelled, and the operation supports terminal cancellation
  only.
- [The browser has no flow control on receipt] -> `max_received_bytes` closes the channel with
  `receive_overflow`; documented as a difference.
- [A second native `runtime`] -> a precondition, documented on the class and on the library's
  page; the library cannot detect it without static state.
- [Constants quoted from libdatachannel] -> checked again when the pin moves (D16).
- [The superproject's libdatachannel target speaks of trystero] -> its threading message names
  "Trystero's native backend"; increment 1's superproject change words it for every consumer.
- [Interop depends on Chrome's flags] -> the flags are named in the tests' documentation; a
  change in Chrome's switches fails the lane loudly.
- [Chromium's limit of peer connections] -> the page's census and the default cap of 256.

## Migration Plan

There is nothing to migrate: no program depends on webcpp::webrtc. trystero's `backend` goes in
increment 3, a later change of trystero's; until then trystero keeps its own.

To start the implementation: the proposal is decided, the owner creates the repository
`webcpporg/webrtc`, it starts from one import commit of `reference/source/`, is checked out as the
superproject's submodule `libs/webrtc`, and `reference/plan.md` is followed from increment 1.

Rollback: the proposal is a draft; withdrawing it deletes `openspec/changes/genesis/`.

## Open Questions

- O1. Whether a later version synthesises `negotiationneeded` natively.
- O2. Whether a later change adds a byte stream over a data channel, for webcpp::beast's D6.
- O3. Whether natively a channel should keep its own queue bounded in bytes, read from
  libdatachannel at once as the browser's is, so that a channel never read cannot stall its
  connection's others, at the price of the flow control that slows the peer (D9).
