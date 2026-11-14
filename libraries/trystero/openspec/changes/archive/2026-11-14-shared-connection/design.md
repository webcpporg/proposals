## Context

webcpp::trystero 0.1.0 (`genesis`) gives each `room` its own world: its own peer id, its own
negotiations and connections. Trystero 0.26.0 does otherwise: its `SharedPeerManager` keeps one
connection per (strategy, app id, remote peer id) and routes every frame to the room named by the
frame's token, with presence frames advertising rooms and a 123,333 ms idle time. The owner
decided on 2026-10-11 that trystero shares one connection between the rooms of an app, and
approved the specification (`reference/spec.md`) the same day, in advance of a clean review: the
independent review approved it after its fourth re-review. The plan (`reference/plan.md`) cuts it
into five increments and 22 tasks, executed task by task with a review of each. This document
records those original decisions and the owner's 2026-10-14 amendment in
[reference/password-domains.md](reference/password-domains.md). The active specification in
`specs/trystero/spec.md` supersedes mixed-password sharing in the preserved original references.

The wire does not change. What does change is who owns a connection (the app, not the room), what
a kick means (one room's binding, not the connection) and how a peer that misbehaves in one room
is kept from costing the others.

## Goals / Non-Goals

**Goals:**

- Two peers in several rooms of one app meet over one WebRTC connection, each frame naming its
  room, as Trystero 0.26.0 does, natively and on Emscripten, with and without exceptions, against
  Trystero JS in both interop lanes.
- Each room's membership, handshake, actions, queues and bounds stay its own: a failure in one room
  ends that room's binding and no other.
- Every bound of the specification holds and is tested; no state of the tree has both ways to join.

**Non-Goals:**

- Media over a shared connection (renegotiation, streams per room), passive rooms, other
  strategies: not ported, as in the genesis.
- Sharing relay sockets, batching topics and one Nostr key per app: relays stay per room, configured
  once for the app (ruling 5), a change of its own.
- Moving trystero onto webcpp::asio, webcpp::webrtc and webcpp::crypto (ruling 6), and running on
  xactor: trystero runs on the executor it is given.
- A registry outside the app: nothing is keyed by the backend or kept in static storage.

## Decisions

### D1. The public shape: `app`, and the standalone room removed (ruling 1)

Rooms are joined through an `app` with one immutable password; `app::join` returns `boost::system::result<room>`. The standalone
`room` constructor and `room_config` are removed in the increment that brings `app`, so no program
can fall back into one id per room (two standalone rooms with one id meet a Trystero JS peer in one
of them only). Alternative rejected: keeping the standalone form as an app of one room; it can come
back later without breaking anything.

### D2. Ownership: the app owns connections, rooms hold bindings

`detail::app_impl` owns the configuration, the self id, the logger, the registry of rooms (by token
and by room id) and every connection; `detail::connection` owns one remote peer's peer connection
and data channel, the tokens the remote advertised, the frames kept for unbound rooms and the idle
timer; `detail::binding` (the action wire's `channel_link`) is one room's use of one connection,
owned by its room. A room holds a `std::shared_ptr<app_impl>`; the app holds no strong reference to
a room; a binding holds a `std::weak_ptr` to its connection, which holds plain pointers to its
bindings. Scoping is by an object the user makes, so two `app` objects with the same app id in one
process are two peers.

### D3. One self id per app (ruling 10)

The app draws its 20-character id once, or takes `app_config::self_id`. Every room announces,
signals, tie-breaks and handshakes with it. One id per app links the app's rooms for relay
observers and peers, as Trystero's one `selfId` per page does; rooms that must stay unlinkable use
separate `app` objects. The page and the security section say so.

### D4. Presence and routing as Trystero's

A presence frame (`2`, present, token) advertises a room on every live connection of the app, when
a room joins and leaves and when a connection is adopted; a room frame (`1`, token, action frame) is
routed to the binding of its token. A frame that does not decode, a 65th advertised token, or a
connection that closes ends the connection for every room; a frame whose token no room of the app
has is dropped silently. Frames for a room of the app not bound yet are kept (64 per connection,
10 s after the first) so that a handshake arriving before signaling binds the room is not lost.

### D5. Adoption: first registered wins, a second connection is closed

A negotiation that opens hands its connection to the app. If the room is leaving, or the app holds a
live connection to that peer, the new one is closed and the room is bound to the existing one,
Trystero's rule. Adoption ends a stale connection to the peer before replacing it, as Trystero's
reuse does, which the plan's silent replacement got wrong (a late close erased the new entry). When
the two sides register different connections (crossed negotiations), each closes the one the other
kept, both lose every binding and the rooms meet anew.

### D6. A binding needs the peer's own presence (ruling 9)

A room binds a connected peer only when the peer itself advertised the room's token on that
connection, the room is not leaving, and no backoff runs. It binds at the first of: the room joins;
the peer's presence arrives; a room frame of the token arrives; the signaling-reuse path on the
first meeting; the rebinding time. A bound room keeps its negotiation entry for that peer, marked
connected, so it starts no negotiation with it. An announcement or signal published in X's name by
anyone else binds nothing.

### D7. Rebinding after a kick, and the lower id's backoff (ruling 9)

When a binding ends while the connection stays live, the room schedules its rebinding time: the
larger of `handshake_timeout` and 10 s, plus 1 s (11 s by default), doubling per end of the same
pair in the same room to 5 minutes, until a completed handshake or the connection's end. Meanwhile
only the lower id (the handshake's initiator) refuses the peer, and keeps its frames; the higher id
refuses nothing but drops the peer's frames of that room until it sends presence. So two honest peers
cannot alternate kicks, invalid room proofs from an already authenticated peer cost at most six reports in ten minutes on the lower id
only, and no announcement is restarted while the connection stays live. Alternative rejected:
Trystero's behaviour (bound again by the next signal), under which repeated invalid proofs in one room loop
reports and announcements without end.

### D8. The initiator accepts an answer to any of its last 8 challenges (ruling 9)

A response carries only the digest of challenge, password, app and room, so the initiator keeps
the last 8 challenges it sent that peer since their last completed handshake and accepts a correct
digest of any of them; one that matches none is ignored before the current binding's challenge has
been sent and is `wrong_password` after. Each challenge is the initiator's own fresh nonce and
nobody else can write on a DTLS-protected data channel, so this proves the password as well as the
current challenge does; a late answer of an ended binding no longer reports a false error.

### D9. The connection's end

A connection with no binding left is kept idle for `app_config::idle_timeout` (123,333 ms, 0 closes
at once) after a departure and closed at once after a kick (a bound: a failed peer holds no
connection, as today). A connection that closes or fails, stays `disconnected` for 5 s, receives a
frame that does not decode or whose remote advertises a 65th room, or loses the app's last user
ends for every bound room; it clears its handlers, moves its bindings out, detaches each and
notifies its room, moving each callback out before calling it. `app_impl`'s destructor closes
nothing.

### D10. Joining twice is refused; 64 rooms (rulings 2)

`join` of a room already joined fails with `error::already_joined`: a `room` is a move-only handle
whose destruction leaves, and two handles to one membership would make that depend on another
handle. `app::max_rooms` is 64, counting a room until its `presence(0)` is sent, because a Trystero
peer advertised 65 tokens would drop every connection.

### D11. Resources: the app's, plus an optional one per room (ruling 3)

`app_config::memory` receives every allocation of the app, including the peer connections and data
channels every room's negotiations make; `room_options::memory`, the app's when null, receives the
rest of a room. A binding fails and frees its own drain waiters in the room's resource when it
detaches (ruling 8), so no object of the app keeps an allocation of a room's resource after the room
has ended. The app copies its immutable password into its own resource, and rooms read it without a
per-room password copy. A binary payload moves when its message buffer and room resources match,
and copies once otherwise; chunked transfers assemble in the room resource. Text always copies.

### D12. One immutable password domain per app (owner amendment, 2026-10-14)

`app_config::password`, immediately after `self_id`, is copied into the app's resource and
remains immutable. `room_options` has no password and every room reads the app's. Different
passwords require separate app objects with distinct self ids; their connections, registry,
presence and token-advertisement state are never reused. A changed password requires a new app.
Initial signaling sealed with this password authenticates the domain before connection adoption,
so every peer that reaches another protected room through presence already knows its password.
A passwordless app's connected peers can still enter its other rooms by token.

The Trystero JS 0.26.0 wire remains unchanged, including its one-way room handshake. The responder
answers and passes its side; the lower id verifies the response. An upstream JS page that mixes
passwords still exposes its own higher-id rooms to admission without their password, and can
provide offline-guess material. The C++ app-domain guarantee concerns admission to the C++ app,
not repair of a remote mixed-password JS page. JS pages should also use one password per app,
with separate page contexts for different domains. Known-password peers that send wrong room
proofs are still kicked and paced under the existing rebinding rules.

The historical mixed-password decision and its stronger handshake claim are superseded by this
amendment. Adding reciprocal wire proofs was rejected because it would change JS compatibility.

### D13. Relays stay per room (ruling 5)

No peer sees the difference; sharing sockets, batching topics and one Nostr key per app touch the
strategy's bounds and reconnection tests, which deserve their own specification.

### D14. The tests: a virtual clock and hooks in the memory backend (ruling 11)

The cases cannot be written without a filter that delivers, drops or holds frames in channel order,
deferred opens, injection, failure, a buffering channel that drains byte by byte, `on_made` with the
connection's id and ICE servers, `peer_of`, and a virtual clock that only the test advances, so that
timing cases assert exact instants and wait no wall-clock time. Every virtual run is bounded by
10^6 timers. The sanitizer run of `shared` and `connection` and a hardened-library build of
`shared_test.cpp` are part of each increment's gate.

### D15. Increments (ruling 12)

1. the connection layer, one room per app; 2. the app; 3. allocation; 4. interop; 5. the pages. Each
leaves every lane green and each test uses only what its increment has; the change is published once
the pages follow. No state of the tree has both the standalone constructor and `app`.

## Risks / Trade-offs

- [Existing users rewrite how they join] -> mechanical, tabled in the page; the standalone form can
  return later as an app of one room.
- [Programs relied on unlinked rooms by default] -> they are linked until they split their apps
  (ruling 10); documented.
- [Changing one room's password now requires another app] -> passwords belong to immutable app
  domains, with distinct self ids and connections. Correcting a faulty room proof within the same
  authenticated domain retains the existing backoff.
- [A lower-id peer can rebind a higher-id C++ room at its own pace] -> one `handshake_timed_out` per
  `handshake_timeout`, as a connection does today.
- [Two residual cases with a Trystero JS lower id: a reuse binding inside the dropping window, and a
  page whose timers run later than the 1 s margin] -> neither reports anything false from the C++
  side; the first joins by the second rebinding time (at most 43 s by default), the second costs one
  false password error on the JS side and one more step, at most three times at the defaults.
- [The branch is based on `a7d04b8`, before the genesis commit `160f431`] -> it must be rebased onto
  the genesis commit before the gates; the interop flood fix `6b1e4ba` is superseded by `4aef1c0`
  and dropped then.
- [A remote JS page mixes room passwords] -> its upstream one-way handshake exposure remains;
  C++ domain isolation protects the C++ app only. The docs state this boundary explicitly.
- [Passwordless app peers enter its other rooms by token] -> documented; protected rooms use a
  separate app with a password and distinct self id.

## Migration Plan

Existing users replace `room r(ex, f, {.app_id = "a", .password = "p"}, "lobby");` by
`app a(ex, f, {.app_id = "a", .password = "p"}); auto r = a.join("lobby");` and test the result:
`room_config::{app_id, self_id, password, relay_urls, relay_redundancy, ice_servers}` become `app_config`,
`room_config::log` becomes `app_config::log`, `room_config::{handshake_timeout, max_receive_bytes}` become `room_options`, `room_config::memory` becomes `app_config::memory` (and
`room_options::memory` for a room's own), `room::on_error` reports only problems with peers while
`app::on_error` reports an unusable configuration, two peers in one process become two `app`
objects, and rooms that must not be linked to one program use one `app` each. The library is
unpublished at 0.1.0's successor, so the break reaches only users of 0.1.0.

## Open Questions

The authentication decision is resolved by the owner's 2026-10-14 password-domain amendment.
Implementation, local validation and integration remain tracked in `tasks.md`. The original
reference files, patch history and uncommitted diff remain immutable evidence. No CI is authorized
for this execution.
