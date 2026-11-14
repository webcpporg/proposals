<!-- The owner-approved password-domain amendment of 2026-10-14 supersedes mixed-password
sharing in reference/spec.md and reference/plan.md. See reference/password-domains.md. -->

## ADDED Requirements

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

## MODIFIED Requirements

### Requirement: Library: A header-only port in its namespace
The library SHALL be a header-only C++20 port of Trystero 0.26.0 (https://github.com/dmotz/trystero, MIT, JavaScript), in namespace `webcpp::trystero`, with internals in `webcpp::trystero::detail`, the wire functions in `webcpp::trystero::protocol`, the interface a platform implements in `webcpp::trystero::backend`, the browser backend in `webcpp::trystero::browser` and the native backend in `webcpp::trystero::native`. Public headers are under `<webcpp/trystero/...>`, one per responsibility, macros are prefixed `WEBCPP_TRYSTERO_` and include guards are `WEBCPP_TRYSTERO_<PATH>_HPP` (`WEBCPP_TRYSTERO_HPP`, `WEBCPP_TRYSTERO_BACKEND_HPP`, `WEBCPP_TRYSTERO_DETAIL_POST_HPP`). The library's metadata (`meta/libraries.json`) names the key `trystero`, the author `R`, the categories `Concurrent` and `IO`, `cxxstd` 20 and `port-of` Trystero, language JavaScript, version 0.26.0, licence MIT. A rename of the library MUST NOT replace the word `trystero` by pattern: it uses the exact C++ patterns `namespace webcpp::trystero`, `<webcpp/trystero/` and `WEBCPP_TRYSTERO_`, and changes a string literal only when it is not on the wire.

#### Scenario: A program names the namespace
- **WHEN** a program includes `<webcpp/trystero.hpp>` and writes `namespace trystero = webcpp::trystero;`
- **THEN** `trystero::app`, `trystero::room`, `trystero::error`, `trystero::app_config`, `trystero::room_options`, `trystero::backend::factory` and `trystero::json_resource` are found

#### Scenario: The metadata names the original
- **WHEN** `meta/libraries.json` is read
- **THEN** it holds `"key": "trystero"`, `"authors": ["R"]`, `"cxxstd": "20"` and a `port-of` whose `name` is `Trystero`, `language` `JavaScript`, `version` `0.26.0`, `url` `https://github.com/dmotz/trystero` and `licence` `MIT`

### Requirement: Library: Messages, binary data and pings are ported; media is not
The library SHALL port Trystero's text, JSON and binary messages and its pings, and MUST NOT port audio and video tracks. Not ported are, each with its reason in the page's "Differences from Trystero": the media API (`addStream`, `removeStream`, `addTrack`, `removeTrack`, `replaceTrack`, `onPeerStream`, `onPeerTrack`, with the actions `@_stream`, `@_track` and `@_signal`), request actions (`kind: 'request'`), `onReceive` and the progress callbacks and cancelling with an `AbortSignal`, `onPeerHandshake`, passive rooms, renegotiation and media over a shared connection, relay sockets and a Nostr key shared by the rooms of an app, the BitTorrent, MQTT, Firebase, Supabase, IPFS and WebSocket relay strategies, `getRelaySockets` and its relatives, and `trickleIce: false`, `turnConfig` and `rtcPolyfill`. A room MUST ignore a frame of one of the media actions. Where a ported feature behaves otherwise, the page says so: relays and their sockets kept per room rather than shared by the rooms of an app, no replay of the joined peers to a newly set `on_peer_join`, `peers()` listing ids rather than connections, and `async_send` to a peer that has not joined completing with `error::no_such_peer`.

#### Scenario: A send to a stranger fails with a code
- **WHEN** `ping_measures_a_round_trip` calls `a.async_ping("nobody", ...)`
- **THEN** the handler receives `trystero::error::no_such_peer`

### Requirement: Library: The convenience header and the public headers
The library SHALL ship the convenience header `include/webcpp/trystero.hpp`, which includes `<webcpp/trystero/config.hpp>` first and then `app.hpp`, `backend.hpp`, `error.hpp`, `json_resource.hpp`, `options.hpp` and `room.hpp` of `include/webcpp/trystero/`, and neither backend. The headers directly under `include/webcpp/trystero/` are `config.hpp` (the no-exceptions macro), `app.hpp` (`app`), `error.hpp` (`error`, its category), `handler.hpp` (`handler<R(Args...)>`), `json_resource.hpp` (`json_resource`), `backend.hpp` (the backend interface) and `options.hpp` (`app_config`, `room_options`, `default_ice_servers`) and `room.hpp` (`room`, `action<T>` and the rest of the public API). Every header of the library but `config.hpp` itself MUST include `<webcpp/trystero/config.hpp>` first, and `config.hpp` includes `<webcpp/config.hpp>`. The subdirectories are `protocol/` (`action_wire.hpp`, `envelope.hpp`, `json.hpp`, `nostr.hpp`, `relays.hpp`, `signal.hpp`, `text.hpp`, `topics.hpp`), `detail/` (`action_wire.hpp`, `app_impl.hpp`, `connection.hpp`, `construction_window.hpp`, `ice_servers.hpp`, `log.hpp`, `nostr_strategy.hpp`, `post.hpp`, `replay_cache.hpp`, `signaling.hpp`), `browser/` (`backend.hpp`, `event_loop.hpp`, `js.hpp`, `src.hpp`) and `native/` (`backend.hpp`, `data_channel.hpp`, `peer_connection.hpp`, `sealer.hpp`, `websocket.hpp`). The headers under `detail/` are internals.

#### Scenario: The convenience header brings the rooms and no backend
- **WHEN** a program includes only `<webcpp/trystero.hpp>` and makes a `trystero::app` over a `trystero::backend::factory&` and joins a room
- **THEN** it compiles on native and on emscripten, and no header of `native/` or `browser/`, no `rtc/` header, no `openssl/` header and no `emscripten/` header is included

#### Scenario: Every header compiles alone
- **WHEN** `test/Jamfile` compiles each public header as its own translation unit (`webcpp.headers-alone trystero`) with `trystero.hpp`, `trystero/*.hpp`, `trystero/protocol/*.hpp` and `trystero/detail/*.hpp` on both targets, `trystero/native/*.hpp` natively and `trystero/browser/*.hpp` on emscripten
- **THEN** each compiles, so no header depends on another being included before it

### Requirement: Library: Layers
The library SHALL be built in layers, each knowing only the one below it: the protocol (`protocol/*.hpp`, pure functions over bytes and text, with no I/O and no platform), the backend interface (`backend.hpp`, the WebSocket, peer connection, data channel, timer and AES-GCM sealer a platform provides), the app and the room (`app.hpp`, `options.hpp`, `room.hpp` and `detail/*.hpp`: the app's connections shared by its rooms and their bindings, the Nostr strategy, the signaling, the action wire and the public API), the browser backend (`browser/*.hpp`, `emscripten::val` over the browser's APIs and the event loop) and the native backend (`native/*.hpp`, libdatachannel, Boost.Beast, OpenSSL and Asio's timers). The same room MUST run over the browser's APIs, over libdatachannel and over the memory relays of the tests. The backend is a handful of abstract classes rather than template parameters, so that the room's code stays free of templates over the platform and of `#ifdef`, and a test backend and the browser backend are interchangeable at run time.

#### Scenario: A room runs over the tests' backend
- **WHEN** `room_test` joins rooms of two `trystero::app` objects over `trystero::test::memory_network` (`test/support/memory_backend.hpp`), a `backend::factory`
- **THEN** two rooms join, exchange messages and leave in one process, on native and on emscripten, with no network

### Requirement: Library: Guarantees of the room
The library SHALL hold six guarantees about a room. Guarantee 3, anchor `trystero-invariant-3`: a peer with another password never joins a protected C++ app's rooms because its initial signals do not open, with every room of that app using the same immutable password; the guarantee does not repair a remote JS page that mixes room passwords; held by `room` (`the_same_password_joins_and_another_does_not`), `native_room` (`another_password_never_joins`) and the wrong-password scenario of the `interop` lane. Guarantee 4, anchor `trystero-invariant-4`: a payload larger than a frame travels in chunks of at most 16,370 bytes, both ways, and arrives whole from either peer; held by `room` (`large_payloads_travel_in_chunks_both_ways`, 100 KB each way) and the file transfers of the `interop` lane. Guarantee 7, anchor `trystero-invariant-7`: a room MUST refuse a replayed or reflected offer or answer, a sealed offer or answer it made or acted on that comes back to it as a peer's changing nothing; held by `room` (`a_replayed_answer_is_refused`, `a_replayed_offer_is_refused`, `a_reflected_offer_is_refused_as_an_answer`, `a_reflected_offer_is_refused_as_an_offer`, `a_reflected_answer_is_refused_as_an_answer`) and the replay and reflection scenarios of the `interop` lane. Guarantee 9, anchor `trystero-invariant-9`: two peers that meet in several rooms of the same app MUST share one connection, one per remote peer per app; held by `shared` and the first sharing scenario of the `interop` lane. Guarantee 10, anchor `trystero-invariant-10`: a room's departure MUST leave the app's other rooms joined; held by `shared` (a room leaves) and the interop scenario of one room leaving. Guarantee 11, anchor `trystero-invariant-11`: different password domains MUST share no C++ app connection or presence; held by `shared` (passwordless membership and separate protected-app isolation in both id orders) and the interop scenario of the passwordless room. That a passwordless room is open to those peers is stated in the page's "What remains", not as a guarantee.

#### Scenario: Another password reports only wrong_password
- **WHEN** `the_same_password_joins_and_another_does_not` runs an app with the password `s3cret` and an intruder app with another one for 1500 ms
- **THEN** the intruder has no peers, the room has exactly one peer, errors were reported, and every reported code equals `trystero::error::wrong_password`

#### Scenario: A large payload travels in chunks
- **WHEN** `large_payloads_travel_in_chunks_both_ways` sends 100 KB from each peer to the other
- **THEN** each arrives whole

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

### Requirement: Memory: A room's whole life allocates through its resource
`a_room_allocates_through_its_resource_from_join_to_destruction` SHALL make two apps, each configured with password `a password` and joining one room, a log sink and a relay of their own (`z` first, 100 ms ahead, then `A`), join them, exchange a 100,000-byte `pmr_bytes` payload of 7s that travels in chunks, measure a round trip with `async_ping`, have `z` say goodbye with `async_leave`, destroy both and drain the `io_context`, with counting from the first construction to the drain. Then `joined`, `exchanged` (`files == 1 && pings == 1`) and `farewell` MUST be true, both log sinks were called (`a_lines`, `z_lines` greater than 0), the network's own resource saw allocations, `global_allocations` is exactly 2 and `c_allocations` is 0, `a_memory.outstanding` and `z_memory.outstanding` are 0 after the drain, and `foreign` is 0 for `a_memory`, `z_memory` and `network_memory`. Each configuration is a temporary (`app_config_in(...)` and `options_in(...)`), because MSVC's debug standard library keeps in a moved-from `std::pmr` container 16 bytes of its resource, which the test would count as the room's.

#### Scenario: Two operator new calls in a room's whole life
- **WHEN** the test finishes
- **THEN** `BOOST_TEST_EQ(global_allocations, 2U)` and `BOOST_TEST_EQ(c_allocations, 0U)` hold

#### Scenario: Nothing is out after the drain
- **WHEN** both rooms are destroyed and the `io_context` has run 300 ms more
- **THEN** both resources have 0 bytes outstanding, although they had more than 0 just after the rooms were destroyed

### Requirement: Protocol: Boost.JSON over a std::pmr resource
`webcpp::trystero::json_resource` (`json_resource.hpp`) SHALL be a `final` `boost::container::pmr::memory_resource` constructed from a `std::pmr::memory_resource*` upstream (`noexcept`), whose `do_allocate` and `do_deallocate` forward to it, whose `upstream()` returns it, and whose `do_is_equal(other)` is true only for itself (telling another adapter apart would need RTTI, which a user may build without). It needs only the header-only base class, because `boost::container::pmr`'s global resources are compiled. A room makes one over its resource (`room_options::memory`, the app's when null) as the storage of every JSON value of the room.

#### Scenario: Allocation reaches the upstream
- **WHEN** a `json_resource` over a counting `std::pmr::memory_resource` allocates and deallocates through a `boost::json::storage_ptr`
- **THEN** the counting resource sees both calls with the same size and alignment

#### Scenario: Equality is identity
- **WHEN** two `json_resource` objects over the same upstream are compared with `is_equal`
- **THEN** they are not equal, and each is equal to itself

### Requirement: Actions: Receiving transfers
A room SHALL accept an offered transfer from a peer only for an action that has a receiver, one transfer per peer at a time, and only while the bytes it has reserved across all peers plus the offered size stay within `room_options::max_receive_bytes`; it replies `accepted`, assembles the chunks into one buffer that becomes the delivered message (no copy for `pmr_bytes`), and releases the reservation when the transfer ends. An offer larger than `max_receive_bytes` is answered `rejected` with the reason `payload exceeds receiver size limit`, an offer when the peer already has 64 pending items with `too many pending transfers`, and a transfer that receives no accepted chunk for 120 s is dropped and answered `rejected` with `action transfer timed out`. A `cancelled` frame from the sender releases the transfer.

#### Scenario: An offer above the limit
- **WHEN** `max_receive_bytes` is 1,000,000 and a peer offers a 2,000,000-byte payload
- **THEN** the room answers `rejected` and the sender's send completes with `error::transfer_rejected`

### Requirement: Native: The peer connection wraps libdatachannel with negotiation driven by the signaling
`native::peer_connection::create(ex, servers, bind_address, memory)` SHALL return a `std::shared_ptr<native::peer_connection>` (a `backend::peer_connection`, constructible only through `create`) over an `rtc::PeerConnection` with `disableAutoNegotiation` set, so that an offer or an answer exists only when `set_local_description` asks for one, as in the browser. It adds each URL of each `backend::ice_server` to the configuration, rewriting `turn:host` with a username and credential as `turn:user:credential@host`, the form libdatachannel parses, and skips a URL libdatachannel cannot parse. A non-empty `bind_address`, as `"127.0.0.1"`, makes the connection gather on that address alone, so that two peers of one machine connect with no other network; an empty one gathers a host candidate on each interface but the loopback. `create` throws what libdatachannel throws when it cannot make the connection. The `rtc::PeerConnection` object and its control block are made with `std::allocate_shared` in the resource it is created with, the app's; `native_handle()` returns that `std::shared_ptr<rtc::PeerConnection>`, which no copy may outlive `app_config::memory`. The destructor resets the callbacks and closes the connection without reporting the close.

#### Scenario: Two connections open a data channel and exchange bytes
- **WHEN** `native_peer_connection` wires two connections bound to `127.0.0.1`: the offerer creates the channel `data`, `set_local_description` completes with type `offer`, the answerer applies it (`has_remote_description()` is true) and completes with type `answer`
- **THEN** both channels open, `is_open()` is true on both, the 4 bytes `{1,2,3,250}` arrive at the answerer, and `buffered_amount()` is at most `backend::data_channel::low_threshold`

#### Scenario: Closing one end is seen by the other
- **WHEN** the offerer calls `close()` once both channels are open
- **THEN** the answerer's channel reports exactly one close (`closing_one_end_is_seen_by_the_other`)

### Requirement: Room: Joining by construction
The library SHALL join a room through `app::join(std::string_view room_id, room_options options = {})` (see "App: join"), which makes the room with the private `detail::room_impl::make` and returns it as a `room`; `room` has no public constructor but its move constructor. The room copies its options into its resource, derives the room token and the root and self topics from the app id, the room id and the app's self id, then starts the Nostr strategy, which connects to every relay of the app, subscribes to the root topic and the peer's own topic, and announces the peer. The backend `factory` MUST outlive the app and its rooms, and `room_id` is read during the call only. Accessors are `get_executor()` (the app's, a `boost::asio::any_io_executor`), `self_id()` (the app's, a `std::string_view` valid as long as the app), `relay_urls()` (the app's, a `std::span<const std::pmr::string>`) and `peers()`.

#### Scenario: Two rooms of one app meet
- **WHEN** an `app` with `self_id` `AAAAAAAAAAAAAAAAAAAA` and an `app` with `self_id` `zzzzzzzzzzzzzzzzzzzz`, each holding the room `lobby` (and `elsewhere`) of one app id, join of the app `webcpp-trystero-tests` on one executor, over the memory backend with two relays
- **THEN** `room_test`'s `two_peers_join_and_exchange_every_payload_format` sees `a` report one join of `zzzzzzzzzzzzzzzzzzzz` and `z` one join of `AAAAAAAAAAAAAAAAAAAA`, and `a.peers()` equals `{"zzzzzzzzzzzzzzzzzzzz"}`

#### Scenario: Three peers form a mesh
- **WHEN** three apps with ids `KKKKKKKKKKKKKKKKKKKK`, `LLLLLLLLLLLLLLLLLLLL` and `MMMMMMMMMMMMMMMMMMMM` each join the room `mesh`, and the first sends `all` on the action `shout`
- **THEN** (`three_peers_form_a_mesh`) every room lists two peers, and both other rooms receive `all`

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

### Requirement: Room: Unusable ICE servers are dropped and reported
When the app is made it SHALL lowercase the scheme of every ICE server URL, drop each server `detail::ice_server_fault` refuses, log it in the category `app`, and report each to `app::on_error` once, after the constructor has returned, with an empty peer id, `error::invalid_configuration` and a text that begins `the ICE server <first URL> ` and ends `, so the app goes on without it`; the app goes on with the usable servers on every backend, and the backends receive the scheme in lowercase.

#### Scenario: Five servers, two unusable
- **WHEN** `unusable_ice_servers_are_dropped_and_reported` configures `stun:127.0.0.1:9`, `http://127.0.0.1:9`, `turn:127.0.0.1:9` without credentials, `turn:127.0.0.1:9` with user `user` and credential `secret`, and `STUN:127.0.0.2:9`
- **THEN** exactly two errors are reported to `app::on_error`, each with an empty peer and `error::invalid_configuration`, the rooms still join their peer, the backend never sees `http://127.0.0.1:9`, and it sees `turn:127.0.0.1:9` and the lowercase `stun:127.0.0.2:9`

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

### Requirement: Room: Features not ported
The library SHALL NOT offer, and a room MUST ignore the frames of, the parts of Trystero 0.26.0 that `doc/differences.adoc` lists as not ported: audio and video (`addStream`, `addTrack` and the actions `@_stream`, `@_track`, `@_signal`) and the renegotiation and media routing over a shared connection, request actions (`request`, `requestMany`, `onRequest`), `onReceive` and the progress callbacks `onProgress` and `onReceiveProgress`, cancelling a send with an `AbortSignal`, `onPeerHandshake`, passive rooms, relay sockets and a Nostr key shared by the rooms of an app, the strategies other than Nostr, and `trickleIce: false`, `turnConfig` and `rtcPolyfill`. A send to a peer that has not joined fails with `error::no_such_peer` where Trystero warns and resolves, and `on_peer_join` does not replay joined peers.

#### Scenario: Two rooms of one app given one id
- **WHEN** a C++ `app` joins `room` and `room2` and meets a Trystero JS page that joins both, the two rooms having the app's one `self_id`
- **THEN** it meets the page in both rooms, over one connection on each side, which the first sharing scenario of the `interop` lane checks

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

### Requirement: Tests: The memory backend runs whole rooms in one process
`test/support/memory_backend.hpp`, in the namespace `webcpp::trystero::test`, SHALL provide a backend in memory: `memory_network` (a `backend::factory`, constructed from an `io_context`, a seed defaulting to 1 and a memory resource) whose `relay(url)` makes a `memory_relay` (a NIP-01 relay holding nothing: events reach only the subscriptions open when they arrive, as for ephemeral kinds), `connect` returns a `memory_websocket`, `make_peer_connection` a `memory_peer_connection` (pairing through descriptions `mem:<id>`), data channels that deliver in order, a `memory_timer` that behaves as the native timer (a wait cancelled, replaced or destroyed never fires), and a `memory_sealer` that seals as `<iv>$<base64(plaintext)>` with the IV the key's first 16 bytes so that only the same key opens it. Every event is posted to the `io_context`, never delivered inside the call that caused it. Its controls are: `memory_relay::events_seen`, `rate_limit_next` (answer that many EVENTs with `rate-limited:`), `close_after_subscribe`, `close_all()`; `memory_network::make_unreachable(url)`, `time_scale` (timers and clock run that many times faster), `refuse_peer_connections` (the next n are null), `record_servers` with `server_urls`, `made_connections` (refused ones included), `peak_connections` and `live_connections()`. The unix time starts at 1,759,700,000. Its hooks for the tests of shared connections (connection ids of `std::size_t`, a frame filter, injection, failure, deferred opens, a buffering channel and a virtual clock) are the subject of "Tests: The memory backend's hooks" and "Tests: The virtual clock". It is outside the library's public headers, and a program may copy it.

#### Scenario: A relay that rate-limits
- **WHEN** a test sets `relay(url).rate_limit_next` to a positive number
- **THEN** the relay answers that many EVENTs with a `rate-limited:` notice so that the strategy's backoff is exercised

#### Scenario: Peers pair through their descriptions
- **WHEN** two rooms over one `memory_network` exchange offer and answer
- **THEN** their connections pair, the state `connected` is reported on both and the offerer's data channel opens on both ends

### Requirement: Tests: The interop scenarios of the browser and the native peers
`interop/run.mjs` SHALL run twenty-four scenarios for the browser's C++ peer and twenty-seven for the native peer (the fourteen shared scenarios listed first, then the eight sharing scenarios of "Tests: Interop with Trystero JS on both lanes", a wrong password and a mesh of three for both, and three more for the native peer), each bounded: C++ offering (lower id `00000000000000000000`) with the C++ peer leaving; Trystero JS offering, the C++ peer answering (id `zzzzzzzzzzzzzzzzzzzz`) with the password `secret` and the JS peer leaving; a Trystero JS peer that also sends malformed ICE candidates (`js.html`'s `malformed-candidate-to`); a first relay URL no WebSocket can be made to (`ftp://relay.invalid`), which reports a close and the room goes on; a Trystero JS peer that also sends text messages on its data channel; a C++ peer given a TURN server without credentials, which the room drops and reports; a watcher without the key (`forger.html`) answering every offer with a forged answer; the same watcher replaying a recorded sealed answer and a recorded sealed offer (`mode=replay`) after the JS peer's connections drop; the watcher reflecting the C++ peer's own sealed offer back as an answer and as an offer through a decoy id (`mode=reflect`); a flood of announcements and unsealed offers from fresh ids (300 of them, bounded by 32 unconnected negotiations, each call limited to 20 s); a relay text longer than 1 MiB, once and then after every subscription, which the C++ peer closes and reconnects with a bound that doubles; the C++ peer and a Trystero JS page each in two rooms of one app, which share one connection; a wrong password (8 s of quiet, no `peer-join`, at least one `error` event); and a mesh of two JS peers and one C++ peer in which every peer joins both others and receives both files within 30 s. The native peer meets three more: a relay that ends the connection during a write (`relay.options.strike`, a text longer than 1 MiB or a reset, 12 peers in turn at delays between 231 and 233 ms, which each survive and reconnect), a room moved over a joined one (`replace`) which leaves it as destroying it does with its pending receive completing with `room left`, and the driver's own check that a native peer killed by SIGABRT is named at once (under 5 s). A file of 100,000 bytes travels both ways with the metadata `{"name":"from-cpp.bin"}` or `{"name":"from-js.bin"}` and the checksum the scenario computes. A native peer that dies by a signal the driver did not send is named as `native#N: killed by SIGSEGV` and every wait of its scenario ends. A wire string changed in the C++ library (`"__trystero_pw"`) fails the password scenario of both.

#### Scenario: The scenarios the browser peer runs
- **WHEN** `run.mjs --browser` runs without `--public`
- **THEN** twenty-four scenarios run and the run ends with `all 24 scenarios passed` when they hold

#### Scenario: The scenarios the native peer runs
- **WHEN** `run.mjs --native` runs without `--public`
- **THEN** twenty-seven scenarios run, including `A relay that ends the connection during a write: native C++ survives and reconnects` and `A native C++ peer killed by a signal is named at once`

### Requirement: Docs: The roadmap, the changelog and the acknowledgements
`doc/roadmap.adoc` SHALL be an appendix listing what trystero does not port or build yet: media (audio and video tracks and streams, and Trystero's `@_stream`, `@_track` and `@_signal` actions with renegotiation), Trystero's other features (request actions, decisions on incoming transfers and their progress, the application's own handshake, passive rooms, relay sockets shared between the rooms of an app, and the other strategies, among them MQTT and BitTorrent trackers) and the external libraries from webcpp's own build (libsecp256k1, libdatachannel and OpenSSL, found where b2's command line or `.local/` says until webcpp builds them from `third_party/`). `doc/changelog.adoc` SHALL be an appendix with the section Unreleased (`trystero-changelog-unreleased`, the region `unreleased`) holding: Added a changelog, the page's last appendix, with each version's changes; Changed `config.hpp` to include `<webcpp/config.hpp>`; Added `WEBCPP_NO_EXCEPTIONS`, which disables trystero's exceptions with every webcpp library's; breaking migration to app::join and an immutable app_config::password, with separate apps and self ids for different passwords. `doc/acknowledgements.adoc` SHALL state that trystero ports the wire protocol of Trystero 0.26.0, written with Trystero's source open, whose MIT notice is `LICENSE-TRYSTERO.txt`; that the BIP-340 signatures are libsecp256k1's (MIT) and the native WebRTC is libdatachannel's (Mozilla Public License 2.0).

#### Scenario: The unreleased changelog
- **WHEN** `doc/changelog.adoc` is read
- **THEN** it keeps the existing unreleased entries and records the breaking migration to app::join and the immutable app_config password, requiring separate app objects and self ids for different passwords

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

## REMOVED Requirements

### Requirement: Room: room_config members and defaults
**Reason**: A room is joined through an `app` only, so that no program can fall back into one peer id per room; `room_config` is removed with the standalone room constructor.
**Migration**: `room_config::{app_id, self_id, password, relay_urls, relay_redundancy, ice_servers}` become `app_config` (see "App: app_config members and defaults"), `room_config::log` becomes `app_config::log`, `room_config::{handshake_timeout, max_receive_bytes}` become `room_options` (see "App: room_options members and defaults"), `room_config::memory` becomes `app_config::memory` and `room_options::memory`; `room r(ex, f, {.app_id = "a", .password = "p"}, "lobby");` becomes `app a(ex, f, {.app_id = "a", .password = "p"}); auto r = a.join("lobby");` followed by a test of the result.
