## Purpose

webcpp::webrtc is one interface for peer connections and data channels, shaped as the W3C
WebRTC API, whose operations follow Asio's model (completion tokens, handlers on
webcpp::asio's executors), so that a program, and trystero first, is written once and runs
natively and in a browser. Natively it wraps libdatachannel through its C API; compiled with
Emscripten it drives the browser's `RTCPeerConnection` and `RTCDataChannel`. Media (tracks,
codecs) is out of scope: data channels only. The requirements below state what `design.md`
(revised 2026-11-14) decides; increment 1 is built as reference code (`reference/source/`).

## ADDED Requirements

### Requirement: Library: a library of its own
webcpp::webrtc SHALL be a library of its own on webcpp::asio, not a part of webcpp::beast, as the owner decided on 2026-11-14, and its headers MUST NOT include webcpp::beast; a later byte stream over a data channel, which webcpp::beast's D6 foresees as an opt-in next layer, would make webcpp::beast use this library, never the reverse.

#### Scenario: Header dependencies on beast
- **WHEN** the library's headers are scanned for includes
- **THEN** none is from `<webcpp/beast/...>` or `<boost/beast/...>`

### Requirement: Library: header-only, namespace and conventions
The library SHALL be a header-only C++20 library in the namespace `webcpp::webrtc`, with its headers under `<webcpp/webrtc/...>`, its internals in `detail`, its macros prefixed `WEBCPP_WEBRTC_`, the b2 target `/webcpp/webrtc//webrtc`, licensed BSL-1.0 with "Copyright (c) 2026 WebCpp.org" on every source file, and a convenience header `<webcpp/webrtc.hpp>` that includes the public interface of the selected target.

#### Scenario: A program includes the convenience header natively
- **WHEN** a native program includes the library's convenience header and names `webcpp::webrtc::peer_connection`
- **THEN** it compiles and the name is `basic_peer_connection<>`, the alias with the default executor and allocator

#### Scenario: Nothing outside the namespace
- **WHEN** the headers are scanned for names
- **THEN** every public name is in `webcpp::webrtc`, every macro starts with `WEBCPP_WEBRTC_`, and every implementation name is in `webcpp::webrtc::detail`

### Requirement: Library: targets are native and emscripten only
The library's build SHALL declare the targets native and emscripten and no other, and its configuration header, which every header includes first, MUST stop with `#error` under WASI, naming the reason: there is no WebRTC stack for WASI, because libdatachannel has no WASI port (its threads, and libjuice's and usrsctp's, need preemptive threads that neither wasip2 nor wasip3's cooperative profile gives, and its ICE and SCTP were never ported to WASI sockets) and no WASI host interface offers WebRTC.

#### Scenario: Included under WASI
- **WHEN** any header of the library is compiled for wasm32-wasip2 or wasm32-wasip3
- **THEN** compilation stops with an `#error` whose text names that there is no WebRTC stack for WASI, libdatachannel's threads and sockets, and the missing host interface

#### Scenario: The build lists its targets
- **WHEN** the library's test Jamfile is read
- **THEN** it declares native and emscripten and has no wasip2 or wasip3 lane

### Requirement: Library: data channels only, no media
The library SHALL offer peer connections and data channels and MUST NOT offer tracks, transceivers or codecs, and no public header SHALL expose a native handle: nothing of libdatachannel (its ids, `rtc::` or `rtc/rtc.h`'s types) or of `emscripten::val` appears in a public member, and media will get an interface of its own.

#### Scenario: No native handle
- **WHEN** the public classes are inspected
- **THEN** none has a native handle member and none returns or takes a libdatachannel id or an `emscripten::val`

### Requirement: Library: no global or static mutable state
The library SHALL keep no global or static mutable state, and the platform's process-wide state (libdatachannel's thread pool, global resources and cleanup; the browser page's census of peer connections) MUST be held through a `runtime` object the program owns.

#### Scenario: Two runtimes in one process natively
- **WHEN** the documentation of `runtime` is read
- **THEN** it states that at most one exists at a time in a native process, because libdatachannel's state is the process's, as a precondition the library does not check, since a check would need static state

### Requirement: Types: the five enumerations
The library SHALL define `sdp_type { offer, answer, pranswer, rollback }`, `peer_connection_state { new_, connecting, connected, disconnected, failed, closed }`, `ice_connection_state { new_, checking, connected, completed, disconnected, failed, closed }`, `data_channel_state { connecting, open, closing, closed }` and `ice_transport_policy { all, relay }`, each an `enum class` whose enumerators carry the W3C names, with a trailing underscore on `new_` only.

#### Scenario: A fresh connection
- **WHEN** a peer connection has just been made
- **THEN** `state()` is `peer_connection_state::new_` and `ice_state()` is `ice_connection_state::new_`

#### Scenario: A channel being created
- **WHEN** `create_data_channel("chat")` has just returned a channel
- **THEN** its `ready_state()` is `data_channel_state::connecting` until the transport opens

### Requirement: Types: inputs are views read during the call
The library SHALL take its inputs as views read during the call that takes them: `ice_server { urls; username; credential; }`, `configuration { ice_servers; policy = all; bind_address; }`, `session_description_view { type; sdp; }`, `ice_candidate_view { candidate; sdp_mid; sdp_m_line_index; username_fragment; }` and `data_channel_init { ordered = true; max_packet_life_time; max_retransmits; protocol; negotiated = false; id; }`, every member with a default member initializer, and an implementation MUST NOT keep a view after the call returns, which an asynchronous operation honours by copying what it needs before it returns, whatever its completion token.

#### Scenario: A temporary string as an SDP
- **WHEN** a program passes a `session_description_view` over a `std::string` to `async_set_remote_description` with a deferred token and destroys the string before it starts the operation
- **THEN** the operation completes correctly, because the library copied the SDP during the call

#### Scenario: Defaults
- **WHEN** a `data_channel_init` and a `configuration` are default-constructed
- **THEN** `ordered` is true, `negotiated` is false, the optional members are empty, `policy` is `all` and `bind_address` is empty

### Requirement: Types: outputs are owned and use the object's allocator
The library SHALL return `basic_session_description<Allocator>` (`type` and `sdp`) and `basic_ice_candidate<Allocator>` (`candidate`, `sdp_mid`, `sdp_m_line_index`, `username_fragment`) as owned values whose text is a `std::basic_string` over the allocator rebound to `char`, each with a `view()` member returning the matching input view (an empty `sdp_mid` as no mid), and the aliases `session_description` and `ice_candidate` SHALL name the instantiations over `std::allocator<std::byte>`.

#### Scenario: A description round trip
- **WHEN** a program takes the `session_description` a completed `async_set_local_description` delivered and calls `view()`
- **THEN** the view has the same `type` and an `sdp` equal to the owned text, usable as `async_set_remote_description`'s argument on the other peer

#### Scenario: A description under a pmr allocator
- **WHEN** a connection is made with a polymorphic allocator over a counting resource and its local description is read
- **THEN** the description's text is allocated from that resource

### Requirement: Types: options are webcpp's own bounds
The library SHALL define `options` with the members and defaults `max_queued_candidates = 64`, `max_queued_channels = 16`, `max_message_size = 256 * 1024`, `max_buffered_amount = 16 * 1024 * 1024` and `max_received_bytes = 16 * 1024 * 1024`, the last applying on emscripten only, and an object MUST fail as a value (never throw, never block) when a bound is reached.

#### Scenario: Defaults
- **WHEN** `options{}` is default-constructed
- **THEN** `max_queued_candidates` is 64, `max_queued_channels` is 16, `max_message_size` is 262144, `max_buffered_amount` is 16777216 and `max_received_bytes` is 16777216

### Requirement: Runtime: process-wide platform state
The library SHALL provide `runtime`, constructed from `runtime_options { std::size_t max_peer_connections = 256; std::uint32_t thread_pool_size = 0; }` (explicit, default argument), neither copyable nor movable, which MUST be created before any peer connection and destroyed after the last connection and channel, and `basic_peer_connection::make` SHALL take it by reference so that no connection can exist without one.

#### Scenario: Native construction and destruction
- **WHEN** a native `runtime` is constructed with `thread_pool_size` 4 and later destroyed
- **THEN** its constructor called `rtcSetThreadPoolSize(4)` and `rtcPreload()`, its destructor `rtcCleanup()`, which returns once libdatachannel's threads stopped

#### Scenario: The default thread pool
- **WHEN** `thread_pool_size` is 0
- **THEN** libdatachannel's own default is used

### Requirement: Runtime: the cap of peer connections
The `runtime` SHALL enforce `max_peer_connections` (default 256, half of Chromium's limit of 500 per page) on both targets, counting natively, in a member, the connections made with it that are alive and in the browser reading the page's census of every `RTCPeerConnection` from its construction until the garbage collector frees it, kept in JavaScript and never in C++, and `make` past the cap MUST fail with `error::too_many_peer_connections`.

#### Scenario: Past the cap
- **WHEN** a `runtime` with `max_peer_connections` 1 has one live peer connection and `make` is called again
- **THEN** `make` returns a failed `result` whose error is `error::too_many_peer_connections`

#### Scenario: A connection gone frees a slot
- **WHEN** the live connection is destroyed and `make` is called again
- **THEN** it succeeds, natively; in the browser it succeeds once the census no longer counts the freed object

### Requirement: Peer connection: make
`basic_peer_connection<Executor, Allocator>::make(runtime&, const Executor&, const configuration&, const options& = options(), const Allocator& = Allocator())` SHALL return a `boost::system::result<basic_peer_connection>` that fails as a value: `error::too_many_peer_connections` past the runtime's cap, `error::syntax_error` for an ICE server URL that cannot be parsed, `error::type_error` for a bound out of the platform's range, and `error::operation_error` for another refusal.

#### Scenario: A server URL that cannot be parsed
- **WHEN** `make` is called with an ice server whose URL is `stun:`
- **THEN** the result holds `error::syntax_error` and the runtime's count is unchanged

#### Scenario: A valid configuration
- **WHEN** `make` is called with a configuration with no ice server and an `io_context`'s executor
- **THEN** the result holds a connection whose `get_executor()` equals that executor

### Requirement: Peer connection: move-only, executor and allocator
`basic_peer_connection` SHALL be movable and not copyable, with `executor_type`, `allocator_type`, `get_executor()` and `get_allocator()`, template parameters `Executor` (default `webcpp::asio::any_io_executor`, natively Boost.Asio's own type, on Emscripten the shipped backend's) and `Allocator` (default `std::allocator<std::byte>`), and the aliases `peer_connection`, `data_channel`, `session_description` and `ice_candidate` for the defaults.

#### Scenario: A move
- **WHEN** a connection with a pending `async_wait_state_change` is move-constructed and the new object is destroyed
- **THEN** the handler runs once, with `operation_aborted`, and the moved-from object holds nothing

### Requirement: Peer connection: set the local description
`async_set_local_description(CompletionToken&&)` SHALL create and apply the implicit local description (an offer or an answer as the connection's signaling state requires, the form trystero uses) and complete with `(error_code, session_description)`.

#### Scenario: An offer
- **WHEN** a connection that has just created a data channel awaits `async_set_local_description`
- **THEN** it completes with no error and a `session_description` of type `offer` whose `sdp` is non-empty, and `local_description()` then returns it, with the candidates gathered so far

#### Scenario: An answer
- **WHEN** a connection whose remote description is an offer awaits `async_set_local_description`
- **THEN** it completes with a description of type `answer`

### Requirement: Peer connection: set the remote description and add candidates
`async_set_remote_description(session_description_view, CompletionToken&&)` and `async_add_ice_candidate(ice_candidate_view, CompletionToken&&)` SHALL complete with `(error_code)`, a candidate given before a remote description MUST complete with `error::invalid_state`, and `async_add_ice_candidate` MUST accept the empty candidate, which adds none.

#### Scenario: Candidate before description
- **WHEN** `async_add_ice_candidate` is called on a connection with no remote description
- **THEN** it completes with `error::invalid_state`

#### Scenario: End of remote candidates
- **WHEN** `async_add_ice_candidate` is called with an empty `candidate` after the remote description
- **THEN** it completes with no error and no candidate is added

#### Scenario: has_remote_description
- **WHEN** `async_set_remote_description` has completed successfully
- **THEN** `has_remote_description()` is true; before it, false

### Requirement: Peer connection: event waits
The connection SHALL offer `async_wait_ice_candidate` (completing with `(error_code, ice_candidate)`), `async_wait_state_change` (`(error_code, peer_connection_state)`), `async_wait_ice_connection_state_change` (`(error_code, ice_connection_state)`) and `async_accept_data_channel` (`(error_code, data_channel)`), each a pending operation the caller re-arms, as Asio's `async_accept`.

#### Scenario: Re-arming
- **WHEN** a program loops on `async_wait_ice_candidate`, starting a new wait in each handler, and gives each candidate to the other peer
- **THEN** it receives every candidate in order, then the end-of-candidates value, and the other peer refuses none

### Requirement: Peer connection: channels, queries and close
The connection SHALL provide `create_data_channel(std::string_view label, const data_channel_init& = {})` returning `boost::system::result<data_channel>`, `local_description()` returning `std::optional<session_description>`, `has_remote_description()`, `state()`, `ice_state()` and `close()`, and `create_data_channel` MUST fail with `error::type_error` when both `max_packet_life_time` and `max_retransmits` are set, when `negotiated` is set without an `id`, for an `id` of 65535, and for a label or protocol past 65,535 bytes, and with `error::invalid_state` on a closed connection.

#### Scenario: Both reliability limits
- **WHEN** `create_data_channel("x", {.max_packet_life_time = 100, .max_retransmits = 3})` is called
- **THEN** the result holds `error::type_error`

#### Scenario: Negotiated channel
- **WHEN** `create_data_channel("x", {.negotiated = true, .id = 7})` is called on both peers before they negotiate
- **THEN** both channels open with `id()` 7 and neither peer receives the channel through `async_accept_data_channel`

#### Scenario: Before any description
- **WHEN** `local_description()` is called before `async_set_local_description` completed
- **THEN** it returns an empty optional

### Requirement: Peer connection: destruction
The destructor of a connection SHALL close it and complete every pending operation with `boost::asio::error::operation_aborted`, posted.

#### Scenario: Destroyed with a wait pending
- **WHEN** a connection with a pending `async_wait_state_change` is destroyed
- **THEN** the handler is invoked once, on the executor, with `operation_aborted`

### Requirement: Data channel: the interface
`basic_data_channel<Executor, Allocator>` SHALL be movable and not copyable and SHALL provide `async_wait_open` and `async_wait_close` (both `(error_code)`), `async_receive(DynamicBuffer&, CompletionToken&&)` (`(error_code, std::size_t, bool is_text)`), `async_send(const ConstBufferSequence&, CompletionToken&&)` and `async_send_text(std::string_view, CompletionToken&&)` (both `(error_code)`), `async_wait_buffered_amount_low`, the synchronous `send` and `send_text` returning `boost::system::error_code`, `set_buffered_amount_low_threshold(std::size_t)`, `buffered_amount()`, `ready_state()`, `label()` (a view into the object's allocator), `id()` (the stream id, once it has one) and `close()`, and its destructor MUST close it and complete every pending operation with `operation_aborted`.

#### Scenario: Open and send
- **WHEN** a channel is created, `async_wait_open` completes, and `send_text("ab")` is called
- **THEN** `send_text` returns a default `error_code` and the remote `async_receive` completes with 2 bytes and `is_text` true

#### Scenario: Label
- **WHEN** a channel is created with label "chat"
- **THEN** `label()` is "chat" on both the creator and the remote channel

### Requirement: Semantics: handlers run on the executor, never inside the call
Every completion handler SHALL be invoked on the object's executor through its associated executor and allocator, as Asio's I/O objects invoke theirs, and MUST NOT be invoked inside the call that started the operation: a completion known at once is posted.

#### Scenario: Immediate completion is posted
- **WHEN** a wait is started whose event is already there
- **THEN** the call returns first and the handler runs later from the executor, with no error

### Requirement: Semantics: native threads post to the executor
On the native target, an event arriving on a libdatachannel thread SHALL be copied into the standard allocator and posted to the executor through a weak pointer to the object's state, and MUST NOT run a handler there, and nothing of the user's allocator SHALL be touched on those threads: the library's queues are filled on the executor.

#### Scenario: An event from a foreign thread
- **WHEN** libdatachannel delivers candidates, states and messages on its threads while a program runs the io_context on its own thread, with a connection over a counting resource
- **THEN** every handler runs on the io_context's thread and the resource recorded no allocation made on another thread

### Requirement: Semantics: browser events enter through the bridge
On the emscripten target, each object SHALL be one `intrusive_registration` of webcpp::asio's bridge, whose `deliver` stages the event in the object's state and posts at most one notification to the executor for what is staged, as webcpp::beast's browser WebSocket does, and nothing of the library MUST call `poll`, `run` or `dispatch` from a JavaScript event or run a handler inside `deliver`.

#### Scenario: A burst of messages
- **WHEN** the browser fires `onmessage` three times on a channel before the executor runs
- **THEN** the three messages are staged, one notification is posted, and the pending receive completes in a later turn of the run family

### Requirement: Errors: the error category and its ten values
The library SHALL define `webcpp::webrtc::error`, an enumeration registered with a `boost::system::error_category` named `webcpp.webrtc`, with these enumerators whose values are fixed and never reused: 1 `invalid_state`, 2 `operation_error`, 3 `invalid_access`, 4 `type_error`, 5 `syntax_error`, 6 `closed`, 7 `message_too_large`, 8 `buffered_amount_exceeded`, 9 `receive_overflow`, 10 `too_many_peer_connections`.

#### Scenario: The category
- **WHEN** `make_error_code(error::closed)` is inspected
- **THEN** its `category().name()` is `webcpp.webrtc` and its `value()` is 6

#### Scenario: The values
- **WHEN** the ten enumerators are cast to int
- **THEN** they are 1 to 10 in the order above

### Requirement: Errors: meanings of the codes
The codes SHALL mean: `invalid_state` the W3C's `InvalidStateError` (a candidate before a remote description, a send on a channel that is not open, an operation on a closed connection); `operation_error` the W3C's `OperationError`, and natively a refusal of libdatachannel's C API with no more precise meaning; `invalid_access` the W3C's `InvalidAccessError`; `type_error` the W3C's `TypeError` (an argument out of its range); `syntax_error` the W3C's `SyntaxError` (an ICE server URL that cannot be parsed); `closed` the channel or the connection closed under a pending operation, or before a wait started; `message_too_large` a message past the size limit, sent or received; `buffered_amount_exceeded` a send that would pass `max_buffered_amount`; `receive_overflow` the browser's received bytes past `max_received_bytes`; `too_many_peer_connections` `make` past `max_peer_connections`; and in the browser a DOMException's `name` SHALL become the enumerator of that name, any other name `operation_error`, natively each negative code of the C API the code the browser gives for the same call.

#### Scenario: A send on a channel not open
- **WHEN** `send` is called on a channel whose `ready_state()` is not `open`
- **THEN** it returns `error::invalid_state`

#### Scenario: A refusal of libdatachannel
- **WHEN** libdatachannel's C API returns `RTC_ERR_INVALID` for a remote description it cannot parse
- **THEN** the handler receives `error::operation_error` and the program sees no exception

### Requirement: Errors: Asio's codes keep their meaning
The library SHALL use, through `webcpp::asio::error`, `operation_aborted` for an operation cancelled by `close()`, by destruction or by a cancellation slot, `already_started` for a second pending wait of one kind, `operation_not_supported` for what a target cannot honour, and `no_buffer_space` for a message larger than the room left in the caller's dynamic buffer, which then stays queued.

#### Scenario: Cancelled by a slot
- **WHEN** an `async_receive` is cancelled through its cancellation slot
- **THEN** its handler runs with `operation_aborted` and the next receive gets the next message

#### Scenario: A buffer too small
- **WHEN** `async_receive` is given a buffer whose `max_size()` leaves 8 bytes for a 16-byte message
- **THEN** it completes with `no_buffer_space`, and a receive into a larger buffer then gets the 16 bytes

### Requirement: Semantics: cancellation per operation
Every operation SHALL install a handler on its associated cancellation slot, and SHALL honour terminal, partial and total cancellation for the waits, `async_receive`, `async_add_ice_candidate` and the sends, completing with `operation_aborted` and consuming nothing, and terminal cancellation only for `async_set_local_description` and `async_set_remote_description`, whose description MUST be documented to stay applied: the browser's promise cannot be cancelled, and its effect may happen after the handler ran.

#### Scenario: Cancel a description in the browser
- **WHEN** `async_set_local_description` is cancelled before the browser's promise settles
- **THEN** the handler runs with `operation_aborted` and `local_description()` may later return the description

### Requirement: Events: state changes coalesce
While no `async_wait_state_change` or `async_wait_ice_connection_state_change` is pending, the library SHALL keep the latest value only: a wait completes with the current state once it differs from the one the previous wait of that kind completed with, or from `new_` for the first, and no bound is needed.

#### Scenario: Two changes before a wait
- **WHEN** the connection goes `connecting` then `connected` while no wait is pending and a wait is then started
- **THEN** the wait completes with `connected`, not `connecting`

#### Scenario: No change
- **WHEN** the previous wait completed with `connected` and the state is still `connected`
- **THEN** the next wait stays pending until the state differs

### Requirement: Events: candidates queue, bounded
While no `async_wait_ice_candidate` is pending, the library SHALL queue candidates in the object's allocator up to `max_queued_candidates`; a candidate past the bound MUST NOT be queued, it is still in `local_description()`, and the end of gathering SHALL always be queued, in a place the bound keeps for it.

#### Scenario: Past the bound
- **WHEN** three candidates arrive with `max_queued_candidates` 2 and no wait pending, then the end of gathering
- **THEN** two are queued, the third is not, and the end of gathering is still delivered after them

### Requirement: Events: incoming channels queue, bounded
While no `async_accept_data_channel` is pending, the library SHALL queue incoming channels in the object's allocator up to `max_queued_channels`, and a channel arriving past the bound MUST be closed at once, as a connection past a listen backlog is refused.

#### Scenario: Backlog full
- **WHEN** 17 channels arrive with `max_queued_channels` 16 and no accept pending
- **THEN** 16 are queued and the 17th is closed at once

### Requirement: Events: the end of candidates
`async_wait_ice_candidate` SHALL complete successfully with an `ice_candidate` whose `candidate` is empty, the W3C's end-of-candidates form, when gathering ends (natively when libdatachannel's gathering state reaches `RTC_GATHERING_COMPLETE`; in the browser from an `icecandidate` event whose candidate is null or empty), and this value MUST NOT be terminal: in the browser an ICE restart gathers again and the next wait completes with the next candidate.

#### Scenario: Gathering completes
- **WHEN** gathering finishes after three candidates
- **THEN** the fourth wait completes with no error and an empty `candidate`

### Requirement: Events: one wait of a kind at a time
A second pending wait of the same kind on one object SHALL complete at once, posted, with `already_started`, and the first MUST be unaffected.

#### Scenario: Two state waits
- **WHEN** `async_wait_state_change` is called twice without a change in between
- **THEN** the second completes with `already_started` and the first completes with the next change

### Requirement: Events: a cancelled wait consumes no event
A cancelled wait SHALL consume no event: the next wait of its kind MUST get the event the cancelled one would have had.

#### Scenario: Cancel then wait
- **WHEN** a pending `async_wait_ice_candidate` is cancelled, a candidate arrives, and a new wait is started
- **THEN** the new wait completes with that candidate

### Requirement: Events: satisfied waits complete at once
Waits that are already satisfied SHALL complete at once, posted: `async_wait_open` on an open channel, `async_wait_close` on a closed one, and `async_wait_buffered_amount_low` when `buffered_amount()` is at or below the threshold, including the browser case where `open` fires before the caller waits.

#### Scenario: Open before waiting
- **WHEN** a channel opens, and only then `async_wait_open` is called
- **THEN** it completes with no error

#### Scenario: Below the threshold
- **WHEN** `buffered_amount()` is 0 and `async_wait_buffered_amount_low` is called
- **THEN** it completes at once, posted, with no error

### Requirement: Events: a channel closing
When the peer closes a data channel, or its transport fails, the messages that arrived before SHALL still be delivered, then `async_receive` SHALL complete with `error::closed`; its pending `async_wait_open` and `async_wait_buffered_amount_low` SHALL complete with `error::closed`, `async_wait_close` SHALL complete successfully, and `ready_state()` MUST read `closed`.

#### Scenario: The peer closes
- **WHEN** the remote side closes a channel while a local `async_receive` is pending
- **THEN** the receive completes with `error::closed`, a pending `async_wait_close` completes with no error, and `ready_state()` is `closed`

### Requirement: Events: close fires no state change
`basic_peer_connection::close()` SHALL close the connection and its channels, as the W3C's `close()` fire no state change, complete every pending operation of the connection with `operation_aborted`, leave `state()` reading `closed`, and complete a wait started afterwards with `error::closed`; the connection's channels SHALL then close with `error::closed` on their pending operations; `basic_data_channel::close()` SHALL complete the channel's pending operations with `operation_aborted` except `async_wait_close`, which completes once the channel is closed, and both MUST discard what is unread, as the W3C discards a message received once the channel left "open".

#### Scenario: Connection close with a state wait pending
- **WHEN** `close()` is called while `async_wait_state_change` is pending
- **THEN** the wait completes with `operation_aborted` and not with a `closed` state, `state()` is `closed`, and a wait started afterwards completes with `error::closed`

#### Scenario: Channel close with a receive pending
- **WHEN** `close()` is called on a channel with a pending `async_receive` and a pending `async_wait_close`
- **THEN** the receive completes with `operation_aborted`, `ready_state()` reads `closing`, and the close wait completes with no error once the channel is closed

### Requirement: Lifetime: the execution context outlives the object
The execution context, the `runtime` and the object's allocator SHALL outlive every object made with them, as for Asio's I/O objects, which is what keeps a libdatachannel thread from posting to a destroyed `io_context`.

#### Scenario: Documented order
- **WHEN** a program declares a memory resource, an `io_context`, a `runtime` and a `peer_connection` in that order
- **THEN** destruction runs in reverse and no handler touches freed state

### Requirement: Lifetime: native destruction is synchronous with libdatachannel's threads
On the native target, destroying a channel SHALL call `rtcDeleteDataChannel` and destroying a connection `rtcDeletePeerConnection`, on the executor, which libdatachannel documents to block until every callback in flight returned and to call none afterwards, so that once it returns nothing more is posted, and what was posted before MUST find nothing, through a weak pointer.

#### Scenario: Destroyed while messages arrive
- **WHEN** a connection and its channel are destroyed while a peer floods the channel, repeatedly, under ThreadSanitizer
- **THEN** no data race, use after free or handler call on a destroyed object is reported

### Requirement: Lifetime: browser destruction removes the registration
In the browser, destroying an object SHALL remove its registration from the bridge and set the JavaScript handlers to null, and the io_context's shutdown SHALL destroy what waits on the object without calling it.

#### Scenario: Event after destruction
- **WHEN** the browser fires an event on an RTCDataChannel whose C++ object was destroyed
- **THEN** no C++ handler runs and no error is raised

### Requirement: Lifetime: pending operations are outstanding work
A pending operation SHALL count as outstanding work on the object's executor and on its handler's associated executor, as Asio's operations do, so that `run()` does not return while a candidate or a state change is awaited, and a program that re-arms its waits keeps `run()` alive until it closes the connection.

#### Scenario: run() until close
- **WHEN** a program waits for state changes in a loop and calls `close()` on the connection
- **THEN** `run()` returns once the aborted wait's handler has run and does not re-arm

### Requirement: Messages: receive appends one whole message
`async_receive` SHALL append one whole message to the caller's dynamic buffer and complete with the message's size and whether it was text, and messages MUST be kept from the channel's creation, so a channel accepted through `async_accept_data_channel`, or handed from one owner to another, loses nothing it received before its first `async_receive`.

#### Scenario: Received before the first receive
- **WHEN** a channel opens and the peer's three messages arrive before `async_receive` is first called
- **THEN** three successive receives return the three messages in order

#### Scenario: Binary and text
- **WHEN** the peer sends a 4-byte binary message and then the text "ab"
- **THEN** the receives complete with (no error, 4, false) and (no error, 2, true)

### Requirement: Messages: native receive uses libdatachannel's queue
On the native target the library SHALL use libdatachannel's own queue in its pull mode (no message callback; `rtcReceiveMessage`, `rtcGetAvailableAmount` and the available callback), bounded by 1,024 messages (libdatachannel's `RECV_QUEUE_LIMIT` at 0.24.6) each at most `max_message_size`, past which libdatachannel's thread waits for room and the peer is slowed, never a failure; the documentation MUST state the measured consequence that a channel never read stalls its connection's other channels, and the library MUST drain a channel's queue when its owner or its connection closes it and when it is destroyed, which releases the waiting thread.

#### Scenario: A slow reader
- **WHEN** a peer sends faster than the program calls `async_receive` for more than 1,024 messages
- **THEN** no message is lost, no error is delivered, and the peer's sends slow down

#### Scenario: A channel never read
- **WHEN** 3,000 messages are sent on a channel that is never read, and a message is then sent on another channel of the same connection
- **THEN** the other message does not arrive until the first channel is destroyed, and its destruction returns in under two seconds

### Requirement: Messages: emscripten receive is bounded in bytes
On the emscripten target the library SHALL keep unread messages in a queue of the channel in the object's allocator, staged by the bridge's `deliver` and charged at once, with each message's record, to `max_received_bytes`, as webcpp::beast's `queue_limits::receive` is, and past the bound the channel MUST close and the pending or next `async_receive` complete with `error::receive_overflow`, because the browser has no flow control on receipt.

#### Scenario: Overflow
- **WHEN** unread messages and their records in a browser channel exceed `max_received_bytes`, the io_context being stopped meanwhile
- **THEN** the channel closes and the next `async_receive` completes with `receive_overflow`

### Requirement: Messages: size limit
A message larger than `max_message_size` (default 256 KiB) SHALL fail as a value with `error::message_too_large`: on a send without closing the channel (the limit being the smaller of `max_message_size` and the peer's announced limit); natively on a receive it is refused by libdatachannel, which is given the limit as its local `maxMessageSize` and announces it in the SDP; in the browser, which cannot announce it, a received message past it MUST close the channel and the pending or next `async_receive` complete with `message_too_large`.

#### Scenario: Oversized send
- **WHEN** `send` is called with 1025 bytes and `max_message_size` 1024
- **THEN** it returns `error::message_too_large` and the channel stays open

#### Scenario: Oversized receive in the browser
- **WHEN** a browser channel receives a message larger than `max_message_size`
- **THEN** the channel closes and `async_receive` completes with `message_too_large`

### Requirement: Messages: send queues and returns
`send` and `send_text` SHALL queue the message and return an `error_code`, and `async_send` and `async_send_text` SHALL complete once it is queued, which on both targets is at once, posted; each MUST fail as a value with `invalid_state` when the channel is not open and with `buffered_amount_exceeded` when the message would pass `max_buffered_amount` (default 16 MiB), leaving the channel open, unlike webcpp::beast's WebSocket, whose fragmented write cannot be taken back.

#### Scenario: Not open
- **WHEN** `send` is called on a channel in the `connecting` state
- **THEN** it returns `error::invalid_state` and queues nothing

#### Scenario: Buffer full
- **WHEN** queued bytes plus the message would exceed `max_buffered_amount`
- **THEN** `send` returns `error::buffered_amount_exceeded`, the channel stays open, and the caller waits with `async_wait_buffered_amount_low` before sending again

### Requirement: Messages: buffered amount and threshold
`buffered_amount()` SHALL report the bytes queued and not yet sent, `set_buffered_amount_low_threshold(std::size_t)` SHALL set the threshold, and `async_wait_buffered_amount_low` SHALL complete when `buffered_amount()` is at or below it.

#### Scenario: Pacing
- **WHEN** a sender floods a channel until `send` returns `buffered_amount_exceeded` and then waits with a threshold of 16 KiB
- **THEN** the wait completes with no error once `buffered_amount()` falls to 16 KiB or below, and sending resumes

### Requirement: Allocation: run-time objects use the object's allocator
The object's state, its queues and the text of `session_description`, `ice_candidate` and `label()` SHALL be allocated through the object's `Allocator`, on the executor, an operation's state through its handler's associated allocator, and what libdatachannel and the browser hold (the C API's maps and the messages in libdatachannel's queues, or `emscripten::val` in the JavaScript heap) remains theirs; none of the types is shareable between processes.

#### Scenario: Counting resource
- **WHEN** a connection pair and a channel are made with a polymorphic allocator over a counting resource and a message cycle runs
- **THEN** the resource counted allocations during the exchange, the description's text uses it, and none happened on another thread than the io_context's

### Requirement: Exceptions: on and off on both targets
The library SHALL work with exceptions on and off on both targets, with `WEBCPP_WEBRTC_NO_EXCEPTIONS` in `config.hpp` as chapter 6 asks: natively it MUST call libdatachannel only through its C API, whose functions catch libdatachannel's exceptions and return negative codes, and in the browser its JavaScript imports MUST catch the browser's exceptions in JavaScript, before they reach a WebAssembly frame; no header of the library throws, tries or catches.

#### Scenario: The native suite without exceptions
- **WHEN** the native suite is built with `exception-handling=off` against the archives the superproject builds
- **THEN** it compiles with `-fno-exceptions` and passes whole

#### Scenario: A malformed URL without exceptions
- **WHEN** a program built without exceptions makes a connection with the URL `stun:`
- **THEN** `make` returns `error::syntax_error` and the program goes on

### Requirement: Targets: what a target cannot honour
What one target cannot honour SHALL follow webcpp::beast's rule: with `WEBCPP_WEBRTC_UNSUPPORTED_ERROR` (the default) the operation fails with `operation_not_supported`, and with `WEBCPP_WEBRTC_UNSUPPORTED_IGNORE` the setting is ignored where the platform does an equivalent thing of its own; both macros defined MUST be an `#error`, the propagated b2 feature `webcpp-webrtc-unsupported=error|ignore` SHALL set one, and the browser classes SHALL live in an inline namespace named after the policy, so that translation units that disagree fail to link.

#### Scenario: bind_address in the browser
- **WHEN** `make` is called on emscripten with a non-empty `configuration::bind_address`
- **THEN** it fails with `operation_not_supported` under the default, and ignores the field under the ignore policy

### Requirement: Targets: native limits
Natively a remote candidate's `username_fragment` SHALL be ignored, since libdatachannel keeps one ICE transport per connection and never restarts it, and a text message holding a zero MUST fail with `operation_not_supported` whatever the policy, since the C API takes a text as a zero-terminated string and ignoring it would truncate the message.

#### Scenario: A text holding a zero
- **WHEN** `send_text` is called natively with a text that holds a zero byte
- **THEN** it returns `operation_not_supported` and sends nothing

### Requirement: Targets: bind_address natively
`configuration::bind_address` SHALL make the native backend bind ICE to that address (libjuice only), an empty value meaning every interface but the loopback, and it is needed for loopback candidates.

#### Scenario: Loopback
- **WHEN** two native peers are made with `bind_address` "127.0.0.1" and no ice server
- **THEN** they gather loopback host candidates and connect

### Requirement: Targets: candidate fields of a local candidate
A local candidate's `sdp_m_line_index` SHALL be `0` natively (libdatachannel gives only a mid, and the connection's only m-line is the data channels') and the browser's own on emscripten, and its `username_fragment` SHALL be empty natively and the browser's own on emscripten.

#### Scenario: Native local candidate
- **WHEN** a native peer delivers a local candidate
- **THEN** `sdp_m_line_index` is 0 and `username_fragment` is empty

### Requirement: Targets: negotiated and id are honoured on both
`data_channel_init::negotiated` and `id` SHALL be honoured on both targets (libdatachannel 0.24.6's C API, `manualStream` and `stream`, and the browser both have them).

#### Scenario: Id on both
- **WHEN** a channel is created with `negotiated` true and `id` 5 on both peers
- **THEN** both channels use stream id 5 and open without a channel announcement

### Requirement: Interface: createOffer, createAnswer and negotiationneeded are not offered
The library MUST NOT offer `createOffer`, `createAnswer` or the `negotiationneeded` event on either target: libdatachannel marks the first two "for specific use cases only" and its `setLocalDescription` takes a type, not a description, and it has no such event, only a query; automatic negotiation SHALL be off natively (`disableAutoNegotiation`), so on both targets a description exists only when `async_set_local_description` asks for one and the program negotiates when it adds a channel.

#### Scenario: Adding a channel does not negotiate
- **WHEN** a program calls `create_data_channel` on a fresh connection and does nothing else
- **THEN** `local_description()` is empty until `async_set_local_description` is awaited

### Requirement: Documentation: the table of differences
The library's documentation SHALL keep the table of differences between the targets: `bind_address`, local `sdp_m_line_index`, local `username_fragment`, a remote `username_fragment` natively, a text holding a zero natively, a received message past `max_message_size`, unread received messages and the stall of a channel never read natively, `createOffer`/`createAnswer`/`negotiationneeded`, and a native handle, each with what native and emscripten do.

#### Scenario: The table exists
- **WHEN** the documentation page is built
- **THEN** it contains the table with those rows

### Requirement: Documentation: libdatachannel is justified
The documentation SHALL justify libdatachannel as the superproject's rule for a library from outside asks: no WebRTC exists in webcpp, Boost, the C++ Alliance or the standard library, libdatachannel is the smallest complete implementation (data channels without a media engine), its DTLS runs on the same source-built OpenSSL as Asio and Beast, it is licensed MPL-2.0, accepted by the owner on 2026-11-14 for this use, with its nested dependencies under their own licences, and webcpp uses it unmodified, as a dependency, and copies none of its files.

#### Scenario: The justification
- **WHEN** the library's page is read
- **THEN** it states the licence and its acceptance, the version 0.24.6 and its pin, that the library is unmodified and not copied, and why nothing else qualifies

### Requirement: Dependencies: libdatachannel is pinned and built from its sources
The native build SHALL link the superproject's pinned libdatachannel, 0.24.6 at `6b1e2e6` in `third_party/libs/libdatachannel` with its nested sources at the gitlinks of `tools/libdatachannel/pin.json`, through `/webcpp/third_party//libdatachannel`, built as static archives by upstream CMake through B2 against the same source-built OpenSSL as Asio and Beast, and a native consumer MUST build with `threading=multi`; no installed copy is looked for.

#### Scenario: A native lane
- **WHEN** b2 builds a native test of the library with `threading=multi`
- **THEN** it links the archives B2 built from `third_party/libs/libdatachannel`, and no `LIBDATACHANNEL_ROOT` or `.local/libdatachannel` is read

### Requirement: Integration: it knows nothing of xactor
The library MUST NOT depend on xactor; an xactor actor that needs a peer SHALL hold a `peer_connection` made on the executor its driver runs on, start operations from its turns, and turn each completion into a message to itself.

#### Scenario: Header dependencies
- **WHEN** the library's headers are scanned for includes
- **THEN** none is from `<webcpp/xactor/...>`

### Requirement: Integration: every part of trystero's backend has a home
The interface SHALL cover every part of trystero's `backend` at trystero `e9326e9` that touches WebRTC, as `design.md`'s table maps it: `make` as a `result` instead of a null factory; descriptions as values; candidates as values, not JSON; the waits instead of handlers, with both states separate and trystero merging them; `create_data_channel` as a `result`; `ready_state()` for `is_open`; the channel waits for open, message, close and buffered-amount-low; synchronous and asynchronous sends failing as values when not open; `buffered_amount()` and the threshold; `configuration::bind_address`; the cap through `runtime_options::max_peer_connections`; messages kept from a channel's creation, which trystero's adoption of a shared connection and its native frames before a handler need; and no native handle; while trystero's shared connection, its bound of 64 frames and 10 seconds, its per-minute window of connections and its dropping of text messages and oversized binaries SHALL stay trystero's.

#### Scenario: The merge of states
- **WHEN** trystero needs one state out of `connectionState` and `iceConnectionState`
- **THEN** it reads `state()` and `ice_state()` separately from this library and merges them itself

#### Scenario: Text on a room channel
- **WHEN** a text message arrives on a channel trystero uses
- **THEN** `async_receive` delivers it with `is_text` true and trystero ignores it

### Requirement: Testing: each target alone
Every test lane SHALL run on native and on emscripten, both suspension variants (`jspi` and `asyncify`), at the owner's three CI levels, and the lane for each target alone SHALL put two peers in one process (natively) or one page (browser) and cover: offer, answer, candidates, the end of candidates, a channel, messages both ways (binary and text), back-pressure, close from each side, cancellation, and destruction with operations pending.

#### Scenario: A full exchange
- **WHEN** the lane runs two peers in one process
- **THEN** an offer and an answer are exchanged, candidates are trickled to the end, a channel opens, binary and text messages cross in both directions, and both sides close

### Requirement: Testing: every case the semantics names has a test
The test suites SHALL contain a test for each case the semantics name: two waits of one kind, a cancelled wait consuming nothing, `async_wait_open` on a channel already open, a pending receive completing with `closed`, `close()` firing no state change, the coalescing of states, the candidate and channel bounds, a message past `max_message_size`, a message past the room of the caller's buffer, the browser's `max_received_bytes`, `make` past the runtime's cap, a channel accepted that received before its first `async_receive`, and a native channel never read stalling the others.

#### Scenario: The list is covered
- **WHEN** the suites' test names are compared with the cases above
- **THEN** each has a test on the targets where it applies

### Requirement: Testing: native lifetime
The native lane SHALL test destruction while libdatachannel's threads are delivering events, repeated, under ThreadSanitizer; that `run()` returns only once the connection is closed; that a flooded channel's destruction returns promptly; and that the `runtime` stops libdatachannel's threads before `main` returns.

#### Scenario: Repeated destruction
- **WHEN** a loop creates a connection pair, floods a channel and destroys one side, 100 times, under ThreadSanitizer
- **THEN** ThreadSanitizer reports nothing

### Requirement: Testing: native and browser peers interoperate
An interop lane SHALL connect a native peer and a headless-Chrome peer through a local signalling relay and exchange messages, in both suspension variants, with Chrome started with `--allow-loopback-in-peer-connection` and `--disable-features=WebRtcHideLocalIpsWithMdns` and the native peer using `bind_address` `127.0.0.1`.

#### Scenario: Message across
- **WHEN** the native peer sends "ping" and the Chrome page replies "pong"
- **THEN** both are received, in both the `jspi` and the `asyncify` page

### Requirement: Testing: allocation and exceptions lanes
The allocation lane SHALL prove with a counting resource that the queues, descriptions, candidates and labels use the object's allocator (natively none of it on libdatachannel's threads), and the exceptions lane SHALL run the native and emscripten suites whole with exceptions on and with `exception-handling=off`.

#### Scenario: Both builds natively
- **WHEN** the native suite runs as the build asks and with `exception-handling=off`
- **THEN** both pass

### Requirement: Testing: no network beyond loopback
CI SHALL gather ICE with host candidates only and use no STUN or TURN server.

#### Scenario: No server configured
- **WHEN** the CI configuration is read
- **THEN** every test configuration has an empty `ice_servers`

### Requirement: Delivery: three increments
The library SHALL be built in three increments: (1) the interface, the `runtime` and the native implementation with their tests; (2) the browser implementation with its tests and the interop lane; (3) trystero on webcpp::webrtc, with webcpp::asio, webcpp::beast for its relays and webcpp::crypto, its `backend`'s WebRTC gone, trystero keeping its policies.

#### Scenario: Order
- **WHEN** the tasks are read
- **THEN** increment 2 follows increment 1 and increment 3 is a change of trystero's

### Requirement: Delivery: the review's answers
The design SHALL provide no zero-copy receive for now (in the browser the bytes must leave the JavaScript heap anyway, and natively they leave libdatachannel's queue by one copy into the caller's buffer) and SHALL provide separate event operations with the policies above rather than one wait returning a variant.

#### Scenario: One copy into the caller's buffer
- **WHEN** a message is received natively into a dynamic buffer that offers one contiguous region for it
- **THEN** the library copies its bytes once, from libdatachannel's queue into that region
