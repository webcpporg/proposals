---
title: "webcpp::trystero: one connection shared by the rooms of an app"
tags: [webcpp, trystero, webrtc, spec]
status: active
created: 2026-10-11
---

# webcpp::trystero: one connection shared by the rooms of an app (specification)

Approved by the owner on 2026-10-11, in advance of a clean review: the independent review
(`~/dev/webcpp/.superpowers/sdd/plan-reviews/trystero-shared-review.md`) approved it after its fourth
re-review, and its two last items, N1 and N2, are applied (ruling 14).

Sources cited below. `C/` is Trystero 0.26.0's core as pinned,
`~/dev/webcpp/libs/trystero/test/oracle/node_modules/@trystero-p2p/core/dist/` (the package
`@trystero-p2p/core-0.26.0.tgz`, integrity `sha512-CZpeXPgf...`, in `test/oracle/package-lock.json`);
`N/` is `.../@trystero-p2p/nostr/dist/` and `T/` is `.../trystero/dist/` of the same tree. `src/` is
the TypeScript each `dist` file's source map carries (`sourcesContent`), read for its comments only;
every behaviour is cited in `dist`, which is what runs. `P/` is the port, `~/dev/webcpp/libs/trystero/`
at `54f96d8`: every citation of `P/` refers to that commit and is checked again when the plan is
written, after the fix of the `trystero/room` CI regression at that commit (ruling 6). `W/` is `~/dev/webcpp/AGENTS.md`. `D/` is `~/.buzz/PLANS/WEBCPP_DESIGN.md`. "The probe"
is a node script that drives the pinned `SharedPeerManager` (`C/shared-peer.mjs`) with two fake
peers joined by an in-memory channel, run with node 26.7.0 from a scratch directory
(`~/.claude/jobs/fcf3a902/tmp/trystero-shared/shared_peer_probe.mjs`, not kept); its results are
quoted in section 2.12. The independent review of this specification
(`~/dev/webcpp/.superpowers/sdd/plan-reviews/trystero-shared-review.md`) reran it and added a second
probe (`~/.claude/jobs/fcf3a902/tmp/trystero-shared-review/probe2.mjs`), whose result is
quoted there too.

## 1. Purpose and the owner's decision

The owner, 2026-10-11 (`D/:874`): "trystero shares one connection between the rooms of an app, as
Trystero does. Two peers in several rooms of the same app meet over one WebRTC connection, each
frame naming its room, instead of one connection per room; the difference listed on trystero's page
('One connection shared by every room of an app between two peers') goes away."

Today each `room` is a world of its own: its own `self_id`, drawn per room unless named
(`P/include/webcpp/trystero/room.hpp:578-584`), its own negotiations and its own connections
(`room.hpp:1084-1128`), and a link that ignores every frame naming another room's token
(`P/include/webcpp/trystero/detail/peer_link.hpp:35-36,175-177`). The page states it as a difference
(`P/doc/differences.adoc:50-53,74-84`) and as a roadmap item (`P/doc/roadmap.adoc:21-24`), and the
interop lane works around it by giving each C++ room an id of its own
(`P/test/oracle/interop/run.mjs:578-583`). The wire already carries what sharing needs: every
message is a room frame naming its room's token, and presence frames exist
(`P/doc/protocol.adoc:73-79`, `P/include/webcpp/trystero/protocol/envelope.hpp:42-148`), proven
byte for byte against Trystero (`P/test/oracle/oracle.mjs:152-175`).

This change gives the port Trystero's model: one identity per app in a program, one connection per
remote peer, shared by every room of the app that both peers are in, each room's membership, handshake
and actions kept apart by the room token. The wire does not change. The owner's rulings on this
specification (section 9) bind every section.

## 2. What Trystero 0.26.0 does

### 2.1 Where the connections live

- A strategy (`createStrategy`) makes one `SharedPeerManager` (`C/strategy.mjs:18-21`), which keeps
  the connections by app id and peer id, `byApp[appId][peerId]` (`C/shared-peer.mjs:72,209`), and
  the rooms by app id and room id, `rooms: Map<appId, Map<roomId, registration>>`
  (`C/shared-peer.mjs:69,73-82`). Nostr's `joinRoom` is one module-level strategy
  (`N/index.mjs:198`), which `trystero` re-exports (`T/index.mjs:1`). So in a page there is one
  connection per (strategy, app id, remote peer id), shared by every room of that app joined through
  that strategy.
- The manager is a library object of the page, not the room's: a room only registers with it
  (`C/strategy.mjs:270-291`) and gives it the connection its negotiation made
  (`C/strategy.mjs:60-62`).

### 2.2 The self id

- `selfId` is drawn once per JavaScript realm, at module load: `genId(20)` (`C/utils.mjs:7`). Every
  app, room and strategy of the page uses it: announcements (`C/topic-strategy.mjs:52-57`), signals
  (`C/signal-handler.mjs:249,256,336`), the offer tie-break (`C/signal-handler.mjs:281,409,481`) and
  the handshake's initiator (`C/handshake.mjs:112`).

### 2.3 The room token and the frames

- The token is the hexadecimal SHA-256 of `Trystero:<app>:<room>` (`C/crypto.mjs:13`): it does not
  depend on the password, unlike the signaling key, the SHA-256 of `<password>:<app>:<room>`
  (`C/crypto.mjs:12`).
- A room frame is `1`, the token's length (16 bits, big-endian), the token, the action frame
  (`C/shared-peer.mjs:7-16`); a presence frame is `2`, present (0 or 1), the length, the token
  (`C/shared-peer.mjs:17-26`). A frame is refused when it is shorter than 3 bytes, longer than
  16,515 (`C/shared-peer.mjs:39`, `C/data-limits.mjs:3`), of another kind, or with a token of 0 or
  more than 128 bytes (`C/shared-peer.mjs:27-36`).

### 2.4 How a connection is made, and a second one dropped

- Each room negotiates on its own topics (`C/strategy.mjs:39-41,141`), sealed with its own key
  (`C/strategy.mjs:42,46-51`), with a peer made from that room's configuration
  (`C/strategy.mjs:52`, `C/signal-handler.mjs:283`).
- When a negotiation's channel opens, the room hands the connection to the manager
  (`C/signal-handler.mjs:164,304` to `C/strategy.mjs:60-62`). `connect` (`C/shared-peer.mjs:100-112`):
  if the app already holds a connection to that peer id that is not stale (open, or still opening:
  `:58-67,142-149`), the new one is destroyed and the room is bound to the existing one (`:105-108`); otherwise the new one becomes the app's (`register`,
  `:186-224`, which takes over its handlers, `:213-222`) and a presence frame is sent on it for every
  active room of the app (`:109-111`).
- A stale connection (closed or failed, or its channel closing or closed, `C/shared-peer.mjs:58-61`)
  is cleared and destroyed when it is looked up for reuse (`:142-149`).
- The rule is "first registered wins" at each side independently. When the two sides register
  different connections (two rooms' negotiations opening in crossed order), each destroys the one the
  other kept, and both lose every binding (measured, section 2.12); each room then announces again
  (`C/strategy.mjs:263-269,64-71`, `C/topic-strategy.mjs:17,60`) and they meet anew.

### 2.5 How a second room comes to use the connection

Two paths, both in the manager; no announcement of its own is made for a peer already connected:
every room keeps announcing `{peerId}` on its own root topic (`C/strategy.mjs:141-201`,
`C/topic-strategy.mjs:52-62`).

- **Presence, over the data channel.** When a room registers, once its token is known, it is bound
  to every connection of the app whose remote side has advertised that token, then advertised as
  present on every connection of the app (`C/shared-peer.mjs:90-98`). Receiving `presence(1, T)`
  records `T` among the remote's rooms, at most 64 (a 65th drops the connection,
  `C/shared-peer.mjs:457-460`), and binds the local room of token `T`, if any (`:461-462`).
- **Signaling reuse.** A room's signal handler, given any well-formed message from a peer id (an
  announcement, an offer, an answer or a candidate, before any of them is opened), first asks
  whether the app holds a connection to that peer that is not stale (`C/shared-peer.mjs:142-149`); if so, the room is bound to it and the
  message is not processed further (`C/signal-handler.mjs:478`, `C/strategy.mjs:119`,
  `C/shared-peer.mjs:113-119`). A room already bound to a live connection to that peer ignores the
  peer's signals (`C/signal-handler.mjs:461-468`); one bound to a connection whose channel is not
  open waits 7,533 ms before dropping it (`:469-477`, `:8`).
- A binding is made once per (room, connection) (`C/shared-peer.mjs:225-233`); a new one calls the
  room's `onPeer` (`:150-154`), which ends the room's own negotiation with that peer
  (`C/strategy.mjs:272-286`, `C/signal-handler.mjs:90-95,115-132`).

### 2.6 Routing by room token, and frames for a room not bound yet

- A binding's `sendData` wraps the action frame in a room frame with the room's token
  (`C/shared-peer.mjs:269-275`).
- `dispatchData` (`C/shared-peer.mjs:448-491`): a frame that does not decode drops the connection
  for every room (`:451-453`, `failData`, `:444-447`); a presence frame is handled as above
  (`:455-471`); a room frame goes to the binding of its token (`:473,482`).
- A frame whose token has no binding on that connection is kept only if a room of the app could still
  claim it (a registered room with that token, or one whose token is not known yet,
  `C/shared-peer.mjs:437-439`) and fewer than 64 frames are kept on that connection (`:475-479`);
  they are discarded 10 s after the first was kept (`:425-428`, `C/data-limits.mjs:4`). Any other
  frame is dropped silently and the connection stays (`:475`; measured, section 2.12). The source
  says why: "A registered room may receive its handshake before signaling binds it"
  (`src/core/shared-peer.ts:858`).
- A bound room whose handler is not yet set may hold 64 frames per connection; beyond, or after 10 s,
  the connection is dropped (`C/shared-peer.mjs:483-490,432-435`).

### 2.7 The handshake, the join and the leave, per room

- Each binding is a peer of its room: `onPeer` puts it in the room's peer map and starts the room's
  handshake (`C/room.mjs:148-170`). The handshake runs in room frames of that room: `@_hsdata`,
  then `@_hsready` (`C/room.mjs:108-131`). The password challenge uses the room's password
  (`C/strategy.mjs:204-206`, `C/handshake.mjs:5-27`); the initiator is the lower id
  (`C/handshake.mjs:112`); it times out after 10 s (`C/room.mjs:7`, `C/handshake.mjs:92`).
- `onPeerJoin` fires in that room only, once both sides passed (`C/handshake.mjs:43-50`,
  `C/room.mjs:123-129`). `onPeerLeave` fires in that room only, when its binding ends and the peer
  was active (`C/room.mjs:70-79`).
- A failure of one room's handshake, or an action-level violation in one room (`C/action-wire.mjs:87-94`
  to `onPeerError`, `C/room.mjs:43`), kicks the peer from that room only: `@_leave` is sent in the
  room and its binding is detached (`C/room.mjs:37-42,70-79`, `C/shared-peer.mjs:245-256`). The other
  rooms keep the connection.

### 2.8 One room leaving, a peer leaving every room, and when the connection closes

- `leave` (`C/room.mjs:80-98`) sends `@_leave` to the room's peers, waits 99 ms, detaches each of the
  room's bindings, then `cleanupRoom` unregisters the room and sends `presence(0, T)` on every
  connection of the app (`C/strategy.mjs:219-245`, `C/shared-peer.mjs:125-136`).
- The remote's room sees `@_leave` (`C/room.mjs:145`) or `presence(0, T)`
  (`C/shared-peer.mjs:463-470`) and detaches its binding; its `onPeerLeave` fires in that room.
- A connection whose last binding is detached is kept idle for `sharedPeerIdleMs`, 123,333 ms
  (`C/strategy.mjs:17,44`, `C/shared-peer.mjs:255,367-375`), then destroyed; a binding made meanwhile
  cancels the timer (`:321`). Only a connection that carries media, local streams or tracks or
  remote media, is destroyed at once (`:247,254`).
- A connection that closes or fails is cleared: every binding's `close` runs, so every room bound to
  it sees the peer leave (`C/shared-peer.mjs:162-185,210-222`), and each room announces again
  (`C/strategy.mjs:263-269`).

### 2.9 Scope of a failure

| What happens | Ends | Source |
| --- | --- | --- |
| a frame that does not decode | the connection, every room | `C/shared-peer.mjs:451-453` |
| a 65th advertised room | the connection, every room | `C/shared-peer.mjs:457-460` |
| 64 frames waiting for a bound room's handler, or 10 s | the connection, every room | `C/shared-peer.mjs:432-435,483-487` |
| channel closed, connection failed | the connection, every room | `C/shared-peer.mjs:210-222` |
| idle for 123,333 ms with no binding | the connection | `C/shared-peer.mjs:367-375` |
| handshake failure or timeout in a room | that room's binding | `C/room.mjs:37-42,130` |
| an action-level violation in a room | that room's binding | `C/action-wire.mjs:87-94`, `C/room.mjs:43` |
| `@_leave` or `presence(0, T)` from the peer | that room's binding | `C/room.mjs:145`, `C/shared-peer.mjs:463-470` |

### 2.10 Joining twice, relays, and configuration in effect

- `joinRoom` for an app and room already joined returns the same room object
  (`C/strategy.mjs:37`); joining one that is leaving finishes its cleanup first (`:38,251-254`).
- Relays are set up once per strategy, by the first room's configuration, until every room has left
  (`C/strategy.mjs:131-136,242-244`); Nostr shares one socket per relay URL across rooms
  (`N/index.mjs:4,199-200`, `C/utils.mjs:143-146`) and batches every room's topics into its
  subscriptions, up to 250 per `REQ` (`N/index.mjs:11,81-153`). Its Nostr key is one per module
  (`N/index.mjs:8`).
- A connection's WebRTC configuration is that of the room whose negotiation made it
  (`C/strategy.mjs:52`, `C/signal-handler.mjs:283`), and its idle time is the registering room's
  (`C/shared-peer.mjs:186,206`).

### 2.11 Strategies

Every strategy goes through the same core (`C/topic-strategy.mjs:17`), so sharing behaves alike in
each; but each strategy module has its own manager (`C/strategy.mjs:21`), so rooms of one app joined
through two strategies do not share. Only Nostr is ported, so this has no counterpart here.

### 2.12 Measured (the probe)

The probe registers rooms `r1` and `r2` of app `app` on two managers, A and P, and connects them
through fake peers whose `sendData` delivers to the other side:

- Both in both rooms, one connection made through `r1`: each side sends `presence(1)` for `r1` and
  `r2` on it, `r2` binds on both sides from the presence frames, and each room's frames reach only
  that room. One connection.
- Connected through `r1`, then A joins `r2`, then P joins `r2`: A sends `presence(1, r2)`; P's `r2`
  binds when it registers and sends its own presence; both `r2` exchange frames. A leaves `r1`: A
  sends `presence(0, r1)`; only `r1`'s bindings close on both sides. A leaves `r2` (its last room):
  `presence(0, r2)`, then, after the idle time given (1,000 ms), A destroys the connection.
- A second connection to the same peer id handed over through `r2`: destroyed, `r2` bound to the
  first.
- Crossed: A registers `c1` first, P registers `c2` first. A destroys `c2`, P destroys `c1`, and
  every binding of both sides closes; neither keeps a connection.
- P advertises a 65th room: A logs "too many advertised rooms; disconnecting peer P".
- Frames for a token no room of A has: dropped; the connection stays.
- The review's second probe (C1): A in `lobby` and `secret-7f3a`, both without a password, P
  connected through `lobby` and echoing as `presence(1, T)` every token A advertised:
  `[ 'lobby: bound P', 'secret-7f3a: bound P' ]`. The token alone binds a room whose id P never
  knew; with no password, the handshake has nothing to check (`C/handshake.mjs:8`). The port keeps
  this, as the owner ruled (ruling 7); section 4.12 states it.

## 3. What the port does today, and what must change

| Concern | Today (`P/`) | After |
| --- | --- | --- |
| Who owns a connection | the room that negotiated it, through `peer_link` (`room.hpp:1096-1114`) | the app (section 4.1); rooms hold bindings |
| Self id | per room, drawn unless named (`room.hpp:141,578-584`) | per app, drawn once unless named |
| A frame for another token | ignored (`peer_link.hpp:175-177`) | routed to that room's binding; kept briefly for a room not bound yet; else dropped |
| `presence(0)` from the peer | ends the whole link (`peer_link.hpp:178-183`) | detaches that room's binding |
| Presence sent | the room's own token, at link start and close (`peer_link.hpp:116,156`) | every room of the app, on every connection; and on each join and leave |
| A kick (handshake, action violation) | `@_leave`, then the link is closed (`room.hpp:1255-1262,1274-1276`; `action_wire.hpp:266`) | `@_leave` in the room, the binding detached; the connection stays for other rooms |
| A room's own negotiation opening when a connection exists | cannot happen (connections are per room) | the new connection is closed, the room bound to the existing one |
| A signal from a peer already connected through another room | starts a negotiation (`signaling.hpp:165-197`) | binds the room to the existing connection, if that peer advertised the room |
| `presence(1, T)` for another room of the app | ignored, like any other token's (`peer_link.hpp:175-177`) | binds that room, as in Trystero, with or without a password; a passwordless room is then open to the app's other rooms' peers (ruling 7) |
| A peer kicked from a room | a new peer at its next negotiation | bound again over the connection at a rebinding time, 11 s doubling to 5 min; the lower id refuses it meanwhile (ruling 9) |
| An announcement or signal in a connected peer's name | starts or feeds a negotiation | binds nothing unless that peer advertised the room itself (ruling 9) |
| Rooms of one program, on the relays and to peers | unlinked: one id per room by default | linked by the app's one id; separate `app` objects keep them apart (ruling 10) |
| Leaving | `@_leave`, 99 ms, every link closed (`room.hpp:811-829,1292-1318`) | `@_leave`, 99 ms, bindings detached, `presence(0)` sent; connections stay for other rooms or go idle |
| Joining the same room twice | two independent rooms, two peers | refused (section 4.7) |
| Connection objects' memory | the room's resource (`signaling.hpp:386`) | the app's resource |
| Relays and their sockets | per room (`room.hpp:676-690`) | per room, unchanged (section 5) |
| Negotiation bounds, replay cache, action bounds | per room (`signaling.hpp:110,118,126`; `action_wire.hpp:312-314`; `room.hpp:931`) | per room, unchanged; new bounds per connection and per app (section 4.9) |
| Threads | one executor per room (`room.hpp:554`) | one executor per app, its rooms' too |

trystero has no actors and does not use xactor; it runs on the executor it is given, with
`std::shared_ptr` and weak references between its objects. The superproject's rule "asynchronous code
is built on xactor" (`W/:675-680`) is not applied to trystero by this change: how a library on Asio
meets xactor is still to be designed (`W/:679-680`), and the planned move of trystero onto
webcpp::asio, webcpp::webrtc and webcpp::crypto (`D/:837,840`) is a change of its own, which comes
after this one (ruling 6).

## 4. The design

### 4.1 Ownership

| Object | Owns | Lives | Resource |
| --- | --- | --- | --- |
| `app` (public handle) | a reference to `detail::app_impl`, counted as one of its users | as the user keeps it | none |
| `detail::app_impl` | the configuration, the self id, the logger, the rooms' registry (by token and by room id), every connection, the count of its users (the `app` handle and every room not yet ended) | while any `std::shared_ptr` to it lives; it closes its connections when its last user goes (section 4.2) | `app_config::memory` |
| `detail::connection` (replaces `peer_link`) | one remote peer's peer connection and data channel, the remote's advertised tokens, the frames kept for unbound rooms, the idle timer, and plain pointers to its bindings: by token, and in the order they wait for the channel to drain | from adoption to its end (section 4.2) | the app's |
| `detail::binding` (a `channel_link`, `action_wire.hpp:105-120`) | one room's use of one connection: a `std::weak_ptr` to the connection, its own drain waiters, the delivery to its room | from binding to detachment; owned by its room's peer state | the room's |
| `room` (public handle) | a reference to `detail::room_impl` | as today | none |
| `detail::room_impl` | as today (signaling, strategy, action wire, peers, actions, handlers), each peer's state holding its `binding` (`std::shared_ptr`) in place of a `peer_link`; the rebinding counts and the backoffs of section 4.2; a `std::shared_ptr` to `app_impl` | as today | `room_options::memory`, the app's by default |

- **References.** A room holds `std::shared_ptr<app_impl>`; the app holds no strong reference to a
  room. A binding holds a `std::weak_ptr` to its connection, whose control block is in the app's
  resource, which outlives every room; the connection holds plain pointers to its bindings, which a
  binding registers when it is made and removes, with every place it holds in the connection's drain
  order, when it detaches, before it returns; a binding's destructor detaches it if it was not. The
  action wire's weak link (`action_wire.hpp:141-165`) points at the binding, an object of the room.
  The registry reaches a room through a weak reference or a handler of the room, which the room
  erases when it ends (leave, abandon, or the destruction of its state). So no cycle, and after a room
  has ended the app holds nothing allocated in the room's resource (the rule of
  `P/doc/allocation.adoc:16` and `P/AGENTS.md:261`, unchanged for the room).
- **No registry outside the app.** Sharing is scoped by an object the user makes; nothing is keyed by
  the backend or kept in static storage (`W/:639`). Two `app` objects with the same app id in one
  process are two peers, which is how a test, or the interop's peers, put several peers in one
  process.
- **The action wire does not change.** It already writes room frames with its room's token and
  sends through a `channel_link` (`action_wire.hpp:105-120,138`); a binding is that link, so each
  room's actions, transfers, queues and bounds stay its own.

### 4.2 Lifecycle

**A live connection** is one that is not stale, as Trystero says it (`C/shared-peer.mjs:58-67,142-149`):
its peer connection is neither closed nor failed and its channel is neither closing nor closed; a
channel not yet open counts as live.

**Adoption.** A room's negotiation that opens hands `(peer, pc, channel)` to the app
(`signaling.hpp:1056-1062`, the `connected` callback). If the room is leaving (its farewell has
begun), the new connection is closed, its handlers cleared first, and nothing else happens, as
`on_connected` refuses today (`room.hpp:1087-1091`). If the app holds a live connection to that peer,
the new one is closed the same way and the room is bound to the existing one, Trystero's rule
(`C/shared-peer.mjs:105-108`). Otherwise the app makes a `connection`, replaces every handler of the pc
and channel with its own, in the app's resource, sends `presence(1, T)` for every room of the app,
and binds the room.

**Binding.** A room is bound to a live connection at most once, by one of three paths:

1. its own negotiation (adoption, above);
2. presence: a room that joins is bound to every live connection whose remote advertised its
   token, then the app sends `presence(1, T)` on every live connection
   (`C/shared-peer.mjs:90-98`); a `presence(1, T)` received binds the room of token `T`, if any
   (`:461-462`), a room with a password and one without alike (ruling 7); and a room frame of its
   token from that peer binds it too;
3. signaling reuse: the room's `signaling::handle`, once a message has parsed and names a peer id
   other than the app's (`signaling.hpp:169-174`), asks the room whether the app holds a live
   connection to that peer. If it does, the message goes no further, whatever
   follows, as Trystero ignores a peer it is bound to (`C/signal-handler.mjs:461-468,478`): the app
   keeps one connection per peer, so a negotiation would only make a duplicate. The room binds then
   only on the first meeting of that room and that peer over that connection; after a binding of
   them has ended, only the other events of the next paragraph bind them again (ruling 9). Without a
   live connection the negotiation goes on as today.

Over a live connection to a peer P, a room R binds P only when every condition holds: P has itself
advertised R's token `T` on that connection (its own `presence(1, T)`, not withdrawn since), so that
an announcement or a signal published in P's name by anyone else never binds P's connection (ruling
9); R is not leaving; and R holds no running backoff for P (below). It then binds at the first of
these events: R joins; P's `presence(1, T)` arrives; a room frame of `T` arrives from P; the reuse of path 3, on the first
meeting; R's rebinding time for P comes (below).

Binding, in this order (ruling 9 for the first step): the room's negotiation entry for that peer is
kept, its connections closed and its timers cancelled, and marked connected, as
`on_channel_open` marks it today (`signaling.hpp:1056-1059`), so that every later signal of that peer
is ignored while the binding lasts (`signaling.hpp:184-186`); the connection's idle timer is
cancelled; a room that binds by reuse or at its rebinding time sends `presence(1, T)` on the
connection, which makes the other side bind at once, a Trystero JS peer included
(`C/shared-peer.mjs:461-462`); the peer's state is made, with its handshake timer, as `on_connected`
does today (`room.hpp:1093-1125`); the frames the connection kept for that token are delivered, in
order, except that a kept `@_leave` discards itself and every frame kept before it for that token,
since they belong to a binding of the other side that has ended (ruling 14, R3); then the challenge or
`@_hsready` is sent (`room.hpp:1130-1142`), as Trystero adds the peer before it flushes
(`C/room.mjs:155-169`).

**Handshake frames across a rebinding** (ruling 9; today's `on_handshake_data`,
`room.hpp:1144-1177`, changes in one place, the initiator's check; nothing changes on the wire). A
response carries only `h`, the SHA-256 of `<challenge>:<password>:<app>:<room>`
(`C/handshake.mjs:6,23-26`), and no challenge id, so the initiator matches a response to a challenge
by computing that digest for each challenge it may be answered for, and comparing:

- the initiator keeps, per peer, every challenge it sent that peer in that room since their last
  completed handshake, at most the last 8, with the rebinding count (erased with it: at a completed
  handshake or at the connection's end). A response that is the correct digest of any of them
  passes, whichever binding it was sent for and whenever it arrives; a passed initiator sends
  `@_hsready` and checks nothing more. Before it has sent the current binding's challenge, a response
  that matches none is ignored; after, it is `wrong_password`. Today only the current challenge
  passes and any mismatch fails (`room.hpp:1157-1163`). This is sound: each challenge is the
  initiator's own fresh random nonce (`random_id(36)`, `room.hpp:1136`), a correct digest over any of
  them proves the password as well as one over the current one, and nobody but the peer can put a
  frame on that connection, a WebRTC data channel encrypted end to end with DTLS. A peer whose
  password is wrong matches none, and is still reported;
- the initiator always sends its challenge when it binds, even when a kept answer has already passed
  it, so that the responder's new binding has a challenge to answer;
- the responder answers one challenge per binding and then checks nothing more, as today
  (`room.hpp:1145-1147`) and as a Trystero JS responder (`C/handshake.mjs:21-26`). A late answer
  given in an ended binding is not lost: the initiator accepts it (first item);
- after its own kick of P in R, the side with the higher id drops P's frames of `T` instead of keeping
  them, and does not bind on them, until it next sends `presence(1, T)`: at its rebinding time, or
  in reply to P's `presence(1, T)`, on which it then binds. Those frames were sent for the binding
  that ended; kept, a late challenge would be answered in the next binding in place of the new one,
  and, if the other side is a Trystero JS initiator, a stale answer flushed into its next binding
  would be checked against its new challenge (`C/handshake.mjs:15-18,96-104`) and reported as a false
  password error.

A room that binds by reuse sends nothing on the relays in answer: its `presence(1, T)` on the
connection is what makes the other side bind (ruling 7).

**Unbinding.** A binding ends, in that room only, when the peer sends `@_leave` in it
(`room.hpp:1233-1234`), when it sends `presence(0, T)`, when the room kicks the peer (a handshake
that fails or times out, an action-level violation, `action_wire.hpp:266`: `@_leave` first,
`room.hpp:1255-1262`), or when the room leaves (after `@_leave` and Trystero's 99 ms,
`room.hpp:811-829`) or is destroyed or moved over (at once, `room.hpp:832-840`). Detaching calls each
of the binding's drain waiters with `false` and frees them, in the room's resource, and removes the
binding from its connection, before it returns (ruling 8). The negotiation entry is then erased, as
`forget` does (`signaling.hpp:200-206`). A room that leaves or is destroyed is removed from the app,
which sends `presence(0, T)` on every live connection (`C/shared-peer.mjs:125-136`); until then it
counts among the app's rooms (section 4.9). The room sees the peer leave as today
(`room.hpp:1264-1290`): pending pings and transfers fail, and `on_peer_leave` fires if the peer was
active. It restarts its announcements only when no live connection to that peer remains:
announcements find peers without a connection, and over a live one every rebinding goes through the
connection (ruling 9). Trystero restarts them after every leave (`C/strategy.mjs:63-71,263-269`).

**Rebinding after a kick or a peer's `@_leave`, and the lower id's backoff** (ruling 9). When a
binding of R with P ends while the connection stays live, by a kick of either side (ours, or P's
`@_leave`), R schedules its rebinding time for P: one step after the end, the first step being the
larger of `handshake_timeout` and 10 s, Trystero's time to keep the frames of an unbound room
(`C/data-limits.mjs:4`, `C/shared-peer.mjs:425-428`), plus 1 s, so that a Trystero JS peer's frames
kept from the ended binding are dropped before the next binding even when the last of them crossed
its `@_leave` (11 s by default; ruling 14, N1); each further such end of
the same pair in the same room doubles the step, to at most 5 minutes. The count lasts until a
handshake of R with P completes or the connection ends; nothing else resets it, so a peer cannot
shorten it by leaving and joining again. At the rebinding time R binds P if the conditions above
hold, sending `presence(1, T)` first. A peer that left the room in earnest follows its `@_leave` with
`presence(0, T)`, so its rebinding time passes without a binding. Until then:

- the side with the lower id, the handshake's initiator (`room.hpp:1135`), which is the side that
  detects a wrong password (`room.hpp:1157-1163`), holds a backoff, after its own kicks only: R
  does not bind P, ignores P's signals (path 3), and keeps P's frames of `T` as for any unbound
  room (64 per connection, 10 s, section 4.3);
- the side with the higher id refuses no binding, but after its own kicks only it drops P's frames
  of `T` until it sends presence (above); it binds at any event the conditions allow, and its
  rebinding time makes it bind, and send presence, if nothing has bound it before.

So only one side of a pair ever refuses a binding, and two honest peers cannot alternate kicks: the
higher id never refuses the lower's presence, and the lower id's backoff, started by its own kick,
ends when the higher id's rebinding time for the same end comes, give or take a round trip, both
counted from the same ends. Pairings, with A the lower id and B the higher, the late frame being one
that the other side's handshake waited for past its timeout:

| Case | What happens | Joins |
| --- | --- | --- |
| A and B C++, B's password wrong | A kicks at 0 (`wrong_password`); B departs, unbound; at A's time both bind, A challenges, B answers, A kicks again | never, as it must: A reports at 0, 11, 33, 77, 165, 341 and 641 s at most, six in ten minutes, then one per 5 minutes; B reports nothing |
| A and B C++, A times out (B's answer, or without a password B's `@_hsready`, late) | A kicks at 0 and keeps B's frames; at 11 s A binds, sends presence; a kept or later answer to the old challenge passes A; A sends a new challenge, B binds on the presence and answers it | by the first rebinding time, 11 s; no `wrong_password` |
| A and B C++, B times out (A's challenge, or A's `@_hsready`, late) | B kicks at 0 and drops A's frames of `T` until its presence; A departs; at 11 s both bind, each sending presence, and the handshake runs anew | by the first rebinding time; no `wrong_password` |
| A C++, B Trystero JS, A times out | A kicks and keeps B's frames; B departs, announces again (A ignores it), and may bind by reuse on A's regular announcements and wait; at 11 s A binds and sends presence, B binds on it or is bound already; A passes on a kept answer or on B's answer to its new challenge | by the first rebinding time |
| A C++, B Trystero JS, B times out | B kicks and keeps A's late challenge (`C/shared-peer.mjs:475-479`); A departs; at 11 s A binds, sends presence and a new challenge; B binds and answers the challenge it took first, the old one if still kept; A accepts either | by the first rebinding time |
| A Trystero JS, B C++, B times out | B kicks and drops A's frames of `T` until its presence; A departs and announces again; at 11 s B binds and sends presence, A binds on it, challenges, B answers | by the first rebinding time, but for the residual case below |
| A Trystero JS, B C++, A times out | A kicks and keeps B's late answer; B departs; B's presence, at its rebinding time (at least 11 s after the end, by which A has normally dropped what it kept), makes A bind with nothing stale; A challenges, B answers | by the first rebinding time, but for the second residual case below |
| A Trystero JS, B C++, B's password wrong | A kicks; B departs; at B's rebinding time B binds and sends presence, A binds on it, challenges; A also binds by reuse on B's regular announcements (`C/signal-handler.mjs:478`) | never, as it must: A's reports at most one per B's rebinding time and, since every relay delivers B's announcement separately and a failed handshake ends within a round trip, one per relay of B's in each of its announcement rounds (`relay_redundancy`, 5 by default) |
| A C++, B Trystero JS, B's password wrong | as in the first row, B announcing again after each kick, which A ignores | never: A's reports as in the first row |
| a corrected password: B leaves R and joins it again with the right one | B's new `presence(1, T)` binds at once if A's backoff has run out, else at its end | within the backoff step that runs, at most 5 minutes after many failures |
| an announcement forged in X's name before X joins R | binds nothing: X has not advertised `T` | X joins at once when it does |

Two residual cases remain, both with a Trystero JS lower id; in neither does the C++ side report
anything false:

1. **A Trystero JS lower id that binds by reuse inside B's dropping window.** In the row "A Trystero
   JS, B C++, B times out", if one of B's regular announcements reaches A between B's kick and B's
   presence, A binds by reuse (`C/signal-handler.mjs:478`) and sends its challenge, which B drops.
   A's handshake times out once (Trystero's "handshake timed out", not a password error) and A
   kicks; that is a second end for B, whose step doubles. They join by B's second rebinding time, at
   most 43 s after the first kick plus the one-way delay, by default (the first end at 0, the second
   at most 21 s, plus a 22 s step), or earlier, when A binds by reuse again outside the window. It happens at most once
   per failure: B is bound again from its rebinding time on, and A's handshake, started inside the
   window, times out before B's (both take 10 s, and A's started before B's rebinding at 11 s), so the second end is A's kick,
   which opens no dropping window on B. That holds while B's `handshake_timeout` is at least
   Trystero's 10 s, the default. No false password error is reported, on either side.
2. **A Trystero JS lower id whose keep timer fires after B's presence arrives.** In the row "A
   Trystero JS, B C++, A times out", A keeps B's late answer from its arrival, `s1 + d`, where `s1` is
   when B sent it and `d` the one-way delay, and drops it 10 s later (`C/shared-peer.mjs:417-428`).
   B's presence reaches A at `e + 11 + d`, where `e`, B's end, is when A's `@_leave` reached B; B sent
   its answer before that, so `s1 < e`, and the answer expires before the presence arrives by
   `e - s1 + 1 s`: the 1 s added to the step, plus how long before A's `@_leave` reached B the answer
   was sent, which can be close to zero. When A's timers run late by
   more than that margin (a busy page, or one throttled in the background), A flushes the stale answer
   into its new binding, checks it against its new challenge (`C/handshake.mjs:15-18,96-104`),
   reports a false password error once and kicks, and they join at the next step. Under Chrome's
   intensive throttling, where a background page's timers run once a minute, it can recur at each
   step until the step exceeds the page's timer lag: at most three times at the default settings
   (steps of 11, 22 and 44 s, then 88 s, beyond a one-minute lag), each with one false error on the
   JS side and one more step. The C++ side cannot prevent it, since the answer was sent before B
   learnt the binding had ended, and reports nothing false.

Over a shared connection a wrong password is reported by the lower id only: the higher id answers,
passes its side and sees the peer leave, with no report. Today the higher id learns it through its
signals that do not open, reported paced (`P/doc/protocol.adoc:122-127`), since there is one
connection per room; over a connection made through another room no signal of this room is
exchanged. The usage page says so (section 7).

A kick reports to `on_error` as today (`wrong_password`, `handshake_timed_out`), once per kick. When
this app has the higher id, a peer with the lower id that misbehaves can bind our room again at its
own pace, since the higher side never refuses: it costs that room at most one `handshake_timed_out`
report per `handshake_timeout`, as a connection does today.

**The connection's end.** A connection with no binding left:

- after a departure (the peer's `@_leave` or `presence(0)`, or our room's leave or destruction), is
  kept idle for `app_config::idle_timeout`, 123,333 ms by default, Trystero's (`C/strategy.mjs:17`),
  then closed; a binding meanwhile keeps it; an `idle_timeout` of 0 closes it at once;
- after a kick, is closed at once, as the port does today for its single room
  (`room.hpp:1274-1276`). Trystero keeps it idle in both cases (`C/shared-peer.mjs:255`); the
  difference is a bound (section 4.9) and changes nothing a Trystero JS peer can rely on, since a
  connection may close at any time. The rebinding count of every room with that peer ends with the
  connection. The peer can then meet that room again only through the room's own signaling, which
  needs the room's key: a peer with a wrong password cannot, since its signals do not open; one with
  the key renegotiates as a new peer does today, within the room's window (64 connections per 57 s)
  and the signaling's pacing, and the connection that opens is adopted and bound as any new one is,
  with a count of zero.

A connection ends for every room bound to it when its channel closes or its peer connection fails
or closes, when it stays `disconnected` for 5 s (`peer_link.hpp:188-208`, kept), when a frame does
not decode, when its remote advertises a 65th room (section 4.9), or when the app's last user goes.
Ending, it first clears the handlers it set, then moves its bindings out of itself, as Trystero's
`clear` does (`C/shared-peer.mjs:170-180`), then detaches each and notifies its room, moving each
callback out before calling it, as `peer_link::end` does (`peer_link.hpp:232-233`): a room may
destroy its binding, its handlers or its own state from inside the call. Each bound room sees the
peer leave, with the reason, and restarts its announcements, since no connection to that peer
remains; its rebinding counts for that peer are erased.

**The app's users.** The `app` handle and each room count as users of the app's state until the
handle is destroyed or moved from, and until the room ends (its farewell completes, it is destroyed
or moved over). When the last user goes, the app closes every connection, its handlers cleared first,
at that call, which runs in the user's code or in a handler of the executor. The destructor of
`app_impl` closes nothing: it may run while the `io_context` is destroyed, when the last operation
pending there releases the last reference, and then it only frees, as `room_impl`'s does today
(ruling 14, M10). Destroying the `app` handle while rooms remain does nothing else: they go on, and
their connections with them.

### 4.3 Routing

The connection reads every message of its channel as today's link does (`peer_link.hpp:169-186`),
then:

| Frame | Done |
| --- | --- |
| does not decode (`envelope.hpp:123-148`) | the connection ends, for every room |
| `presence(1, T)`, `T` new | `T` recorded; a 65th distinct token ends the connection; the room of `T`, if any, is bound by path 2 |
| `presence(1, T)`, `T` known | the room of `T`, if any and not bound, is bound by path 2 |
| `presence(0, T)` | `T` forgotten, frames kept for `T` dropped, the binding of `T`, if any, ends (a departure) |
| room frame, `T` bound | handed to that room's action wire with the frame, as today (`room.hpp:1099-1106`) |
| room frame, `T` a room of the app not bound to this connection, from a peer the room with the higher id kicked and has not yet sent presence to since | dropped (section 4.2) |
| room frame, `T` another room of the app not bound to this connection (its backoff running for this peer or not) | kept: at most 64 frames per connection, all of them dropped 10 s after the first was kept (`C/shared-peer.mjs:475-479,425-428`); the room binds if the conditions of section 4.2 hold |
| room frame, any other `T` | dropped silently; the connection stays (`C/shared-peer.mjs:475`) |

A frame kept for a room is delivered when that room binds, before any later frame, a kept `@_leave`
discarding itself and what was kept before it (section 4.2). Trystero's
second buffer, for a bound room whose handler is not set (`C/shared-peer.mjs:483-490`), has no
counterpart: a binding's delivery is set when it is made, in the same call.

Sending is the action wire's, through its binding: a room frame goes to the shared channel. A send
that must wait for the channel to drain (`action_wire.hpp:387-405`) waits in its binding, and the
binding takes one place in the connection's drain order; at each drain the connection calls every
binding in that order, and each its own waiters, in theirs. A room's chunked transfer therefore sends
one chunk per drain while another room's waiting sends get their turn at the same drain; each wait
keeps its 10 s bound (`error::channel_stalled`, `action_wire.hpp:313`). A send that does not wait (an
inline frame while the channel buffers at most 64 KiB, `action_wire.hpp:389-392`) goes to the channel
at once, without a bound of trystero's: a flood of `@_ping` in one room is answered with as many
`@_pong` (`room.hpp:1221-1224`), which delays traffic to that peer, in every room, and to that peer
only.

### 4.4 Self id

- `app_config::self_id`, empty by default: the app draws 20 characters of `protocol::id_alphabet`
  once, at construction (today's `random_id`, `room.hpp:965-973`). One longer than
  `protocol::max_peer_id_bytes` (64) is replaced and reported to `app::on_error` with
  `error::invalid_configuration`, as a room does today (`room.hpp:578-581,1037-1041`).
- Every room of the app announces, signals, tie-breaks and handshakes with it. `room::self_id()`
  returns it. One id per app links the app's rooms (section 4.12); rooms that must stay unlinkable
  are joined through separate `app` objects.

### 4.5 The public API

```cpp
namespace webcpp::trystero {

/** What an app is made with: everything its rooms share. Move-only, as its `log` is. */
struct app_config {
    std::pmr::string app_id = {};
    std::pmr::string self_id = {};                      // empty: 20 random characters, once
    std::pmr::vector<std::pmr::string> relay_urls = {}; // empty: Trystero's defaults
    std::size_t relay_redundancy = 5;
    std::pmr::vector<backend::ice_server> ice_servers = {};  // empty: default_ice_servers()
    std::chrono::milliseconds idle_timeout{123'333};    // Trystero's sharedPeerIdleMs; 0: none
    detail::logger::sink log = {};                      // the app's and every room's sentences
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();
};

/** What one room adds to its app's configuration. */
struct room_options {
    std::pmr::string password = {};
    std::chrono::milliseconds handshake_timeout{10'000};
    std::size_t max_receive_bytes = std::size_t{256} * 1024 * 1024;
    std::pmr::memory_resource* memory = nullptr;        // null: the app's resource
};

/** This program in one app: one peer id, one connection per remote peer, shared by its rooms. */
class app {
public:
    using executor_type = boost::asio::any_io_executor;
    /** The most rooms an app holds at once: a peer drops a connection that advertises more. */
    static constexpr std::size_t max_rooms = 64;

    app(executor_type ex, backend::factory& factory, app_config config);
    app(app&&) noexcept = default;
    app& operator=(app&&) noexcept;   // releases the state it held; its rooms go on
    ~app();                           // likewise

    [[nodiscard]] const executor_type& get_executor() const;
    [[nodiscard]] std::string_view app_id() const;
    [[nodiscard]] std::string_view self_id() const;
    [[nodiscard]] std::span<const std::pmr::string> relay_urls() const;

    /** Configuration the app set aside (an over-long self id, an unusable ICE server). */
    template <class F> void on_error(F&& f);

    /** Joins a room of the app: error::already_joined, error::too_many_rooms. */
    boost::system::result<room> join(std::string_view room_id, room_options options = {});
};

}  // namespace webcpp::trystero
```

- `room` keeps every member it has (`room.hpp:1430-1571`): `get_executor` (the app's), `self_id`
  (the app's), `relay_urls` (the app's), `peers`, `on_peer_join`, `on_peer_leave`, `on_error`,
  `make_action`, `async_ping`, `async_leave`, moving, and leaving at destruction. `action` is
  unchanged.
- `join` fails, as a value: `error::already_joined` while a room of that id is joined in the app (a
  room in its 99 ms farewell is finished at once first, its `presence(0)` sent, as Trystero's rejoin
  does, `C/strategy.mjs:38`); `error::too_many_rooms` when `max_rooms` rooms count (section 4.9).
  Two enumerators are added after `invalid_configuration` (13): `already_joined = 14`,
  `too_many_rooms = 15` (`W/:636-638`: values are fixed and never reused). `invalid_configuration`'s
  message, "the room's configuration is unusable" (`error.hpp:120`), becomes "the configuration is
  unusable", and its comment names the app.
- Preconditions, checked by `BOOST_ASSERT`: `app_config::memory` is not null (as today's,
  `room.hpp:1578-1579`); `join`, `on_error` and the accessors are not called on a moved-from `app`.
  A negative `idle_timeout` is replaced with 0 and reported to `app::on_error` with
  `error::invalid_configuration`.
- `join` touches the app's state at once, so it is called where the app's executor runs (on its
  thread, or within its strand: section 4.6).
- The relays stay per room (section 5), configured once for the app: each room runs its strategy over
  `app::relay_urls()`.
- **Compatibility.** The standalone constructor `room(ex, factory, room_config, room_id)` and
  `room_config` are removed (ruling 1): a room is joined through an `app` only, so no program can
  fall back into one id per room. `room` keeps no public constructor but its move constructor. The
  migration for existing users, which the page states (section 7), is mechanical:

| Today | After |
| --- | --- |
| `room r(ex, f, {.app_id = "a", .password = "p"}, "lobby");` | `app a(ex, f, {.app_id = "a"}); auto r = a.join("lobby", {.password = "p"});` then `if (!r) ...` |
| `room_config::{app_id, self_id, relay_urls, relay_redundancy, ice_servers}` | `app_config` |
| `room_config::log` | `app_config::log`: one sink for the app and its rooms, its sentences composed in the app's resource |
| `room_config::{password, handshake_timeout, max_receive_bytes}` | `room_options` |
| `room_config::memory` | `app_config::memory`, and `room_options::memory` for a room's own |
| `room::on_error` reports an unusable configuration, with no peer | `app::on_error` does; `room::on_error` reports only problems with peers |
| two rooms of one app, each its own id, to meet Trystero JS in both | two `join`s on one `app` |
| two peers in one process (the tests) | two `app` objects |
| rooms that must not be linked to one program | one `app` each (section 4.12) |

### 4.6 Executors and threads

An app and its rooms run on one executor, the app's: every handler, completion and timer of either,
and every event its backend reports. With an `io_context` on several threads the executor is a
strand, as for a room today (`P/doc/native.adoc:46-48`). Nothing is locked: the state is touched only
there, and the resources need not be thread-safe on one thread (`room.hpp:169-170`).

### 4.7 Joining twice

Trystero returns the same room (`C/strategy.mjs:37`). Here a `room` is a move-only handle whose
destruction leaves (`room.hpp:1418-1428`); two handles to one membership would make "destroying a
room leaves" depend on another handle. So `join` refuses a room already joined in the app with
`error::already_joined` (ruling 2). Two rooms of one token on one connection are impossible
anyway: the token is the route.

### 4.8 Memory and allocation

- `app_config::memory` receives every allocation of the app: its state, the copy of its
  configuration, its logger and sink, and every sentence composed for it, the app's and its rooms'
  (ruling 14, M12), the self id, the registry, every connection, the tokens and frames a connection
  keeps, its drain order, the presence frames, the peer connections and data channels every room's
  negotiations make (the signaling is handed the app's resource for `make_peer_connection`,
  `signaling.hpp:386`), and the frames the backend receives on them.
- `room_options::memory`, the app's when null (ruling 3), receives everything else of the room, as
  `room_config::memory` does today (`P/doc/allocation.adoc:16-30`): its state, its bindings and their
  drain waiters, negotiations' records, its rebinding counts and backoffs, sealed texts, relays, actions,
  queues, the frames it writes, transfers, its JSON storage, its handlers.
- A room's handlers on a connection it negotiated are replaced when the app adopts it, and cleared
  when the room ends a negotiation (`signaling.hpp:1085-1096`, kept); a binding's waiters are failed
  and freed when it detaches (section 4.2). So no object of the app keeps an allocation of a room's
  resource after the room has ended (section 4.1).
- A received frame is in the app's resource and becomes the message's buffer, as today
  (`action_wire.hpp:43-95`). A `pmr_bytes` or `std::pmr::string` received is moved out of it when
  the room's resource is the app's (the default), and copied once into the room's otherwise: a
  `std::pmr` container moved with another resource's allocator copies.
- The app's resource must outlive the app's state, that is the `app`, every room of it, every value
  they hand out that is in it, and every operation pending on the executor: declared before the
  `io_context`, as a room's is today (`P/doc/allocation.adoc`, "The resource's lifetime"). A room's
  own resource keeps today's rule.
- The app parses no JSON, so it has no JSON storage; each room keeps its own.
- `std::allocate_shared` over `std::pmr::polymorphic_allocator<>` for every shared object, the
  `handler` type for every callable, `detail::post` for every post (`P/AGENTS.md:258-448`), as today.

### 4.9 Bounds

| Bound | Scope | Value | Beyond it | Source |
| --- | --- | --- | --- | --- |
| rooms of an app | app | `app::max_rooms`, 64, counting a room until its `presence(0)` is sent | `join` fails, `error::too_many_rooms` | the next row: a peer that advertised 65 would lose every connection |
| rooms a remote advertises | connection | 64 distinct tokens | the connection ends | `C/shared-peer.mjs:457-460` |
| frames for a room of the app not bound to the connection | connection | 64 frames (at most 64 x 16,515 bytes), 10 s | the frame is dropped; all are dropped 10 s after the first | `C/shared-peer.mjs:475-479,425-428` |
| frames for any other token | connection | none kept | dropped | `C/shared-peer.mjs:475` |
| a frame | connection | 16,515 bytes, token 1 to 128 bytes | the connection ends | `envelope.hpp:23-29,123-148` |
| a connection with no binding | connection | `idle_timeout` after a departure; none after a kick | closed | section 4.2 |
| a handshake | binding | `handshake_timeout`, 10 s | the room kicks the peer | `room.hpp:1115-1125` |
| rebinding after a kick or a peer's `@_leave` over a live connection | room and peer | one step after the end, the first the larger of `handshake_timeout` and 10 s, doubling per end to 5 min, until a completed handshake or the connection's end; the lower id refuses meanwhile, after its own kicks | the lower id keeps the peer's frames and ignores its signals; the higher id, after its own kicks, drops them until it sends presence | section 4.2 |
| challenges an initiator accepts an answer to | room and peer | the last 8 it sent since the last completed handshake | the oldest is forgotten | section 4.2 |
| a binding over a live connection | room and peer | only a peer that advertised the room's token itself | not bound | section 4.2 |
| negotiations, connections made, replay cache, counter-announcements, signal fields | room | 32, 64 per 57 s, 1024, 5.3 s per relay, `protocol/signal.hpp` | as today | `signaling.hpp:110,118,126,230`; `P/doc/protocol.adoc:114-143` |
| pending frames, transfers, receive reservation, queued messages | room and peer | 64 per peer for 120 s, one incoming transfer per peer, `max_receive_bytes` across peers, 1024 per action | as today | `action_wire.hpp:312-314`; `room.hpp:931,1246-1250` |
| peer connections a backend makes | backend | 256 held in a page, 256 a minute natively | the backend refuses | `browser/backend.hpp:926`; `native/backend.hpp:126` |

Consequences, stated on the page:

- A connection made for a room counts against that room's window (64 per 57 s) also after the app
  adopts it, and a room bound by presence or reuse makes none. A connection becomes idle only after a
  departure, and only a holder of a room's key makes a connection (with the key, the negotiation;
  a peer that departs at once, even before its handshake, leaves one idle); a connection whose last
  binding was kicked is closed at once. Fresh ids that make a connection and depart make each a new
  connection, within the window, so they keep at most `64 x ceil(idle_timeout / 57 s)` idle per room
  (192 with the defaults); a peer that comes back with its id reuses its idle connection. When a room
  with many joined peers leaves, their connections stay idle for `idle_timeout`: never more than the
  app already held, within each backend's own bound (the browser's 256 held).
- A remote bound to `k` rooms can hold, in each, what one peer can hold in a room today; it binds only
  rooms the app has joined, at most 64. The app
  itself holds at most about 1 MiB of kept frames and 8 KiB of tokens per connection.
- A peer that floods one room spends that room's bounds, not another's: each room has its own queues,
  transfers, reservation, handshake and backoff, frames are routed in arrival order without being
  held for another room, and waiting sends take turns (section 4.3). Inline sends are not bounded by
  trystero, and delay only traffic to the peer that provoked them (section 4.3).
- A remote connected through one room can bind another room the app has joined by advertising its
  token itself: a room with a password then reaches the handshake (section 4.12), and a room without
  one is joined (ruling 7). A binding that fails its handshake is kicked; the rebinding
  times of section 4.2 then space every new binding of that pair in that room: when this app has the
  lower id, at most six kicks and six `on_error` reports in the first ten minutes, then one per 5
  minutes; when it has the higher id, at most one `handshake_timed_out` report per
  `handshake_timeout`. No announcement is restarted while the connection stays live.

### 4.10 Errors and exceptions

- Errors are values: `join` returns `boost::system::result<room>`; every operation of a room
  completes as today. The app's constructor does not fail: what it sets aside is reported to
  `app::on_error` after it returns, with an empty peer id, as `room::on_error` does today
  (`room.hpp:1037-1073`).
- A connection that ends is logged with its reason and seen by each bound room as the peer leaving,
  without an `on_error` report, as a link's end is today (`peer_link.hpp:224-234`,
  `room.hpp:1264-1290`).
- A peer that `room::on_error` names over a shared connection advertised the room itself, on its own
  connection: an announcement forged in its name binds nothing (section 4.2).
- trystero raises nothing of its own; the new code adds no `throw`, `try` or `catch`, and compiles
  with and without exceptions (`W/:763-830`). `result::value()` raises through
  `boost::throw_exception`, the user's choice; the examples test the result instead.

### 4.11 Browser and native

The core changes; the backends do not: their interface (`P/include/webcpp/trystero/backend.hpp`) is
unchanged, the browser's `max_peer_connections` and the native `max_peer_connections_per_minute` bound
all of a program's rooms as today, and fewer connections are made. The browser backend, the native
backend and the memory backend of the tests run the same app code. The backends' comments that name
the room's resource say that peer connections are made in the app's (section 7).

### 4.12 What sharing changes for security

- **A room without a password is not private from the peers of the app's other rooms** (ruling 7,
  as Trystero JS). Presence gives every peer connected to the app, through any of its rooms, the token
  of every room the app is in (`C/shared-peer.mjs:109-111`, section 4.2). The relays give anyone who
  reads them the room's topics: every Nostr event carries its topic in clear in its `x` tag
  (`N/index.mjs:45-52`, `P/doc/protocol.adoc:33-35`), and every announcement carries the announcer's
  id in clear (`C/topic-strategy.mjs:54-57`), which is the app's one id on every room's topic (ruling
  10). And the token alone binds a room over a shared connection (`C/shared-peer.mjs:461-462`); a room
  without a password has no other gate, since an empty password goes straight to `@_hsready`
  (`room.hpp:1130-1134`, `C/handshake.mjs:8`). So a peer connected to the app through one room can
  join any of its passwordless rooms without knowing their ids: it echoes their tokens and completes
  the handshake (the review's probe2, against the pinned manager, binds `lobby` and `secret-7f3a`
  alike). A watcher that is not connected to the app still cannot join without the room id, from
  which the topics and the key derive (`P/doc/protocol.adoc:117,166-176`). A room that must be
  private takes a password (its handshake keeps such a peer out, next item), or an `app` of its own,
  whose separate id neither its connections nor its announcements share with the app's other rooms.
  The page says this, with the mechanism, in "What remains" and in the usage page.
- **The password exposure is accepted** (ruling 4). A room with a password bound by presence or reuse
  was not reached through its sealed signaling, so a remote that knows another room's password, or a
  room without one, reaches this room's handshake without this room's key. The handshake keeps it out
  (`C/handshake.mjs:5-27`; Trystero names the error "incorrect password for overlapping room",
  `C/handshake.mjs:4`). But the initiator is the lower id (`C/handshake.mjs:112`), which a remote
  chooses, and the responder answers any challenge with the SHA-256 of
  `<challenge>:<password>:<app>:<room>` (`C/handshake.mjs:21-26`, `room.hpp:1164-1175`): a peer of
  another room of the app obtains material for an offline guess of this room's password, which today
  only a holder of this room's key could. The responder answers one challenge per binding, as today
  and as Trystero JS, but a lower-id peer can bind it again at its own pace (section 4.2), so the
  number of answers it collects is not bounded, and one is enough for an offline guess.
  Trystero JS
  has this exposure; closing it needs a wire change Trystero would share. The page's security
  section, "What remains" of `doc/protocol.adoc`, states it, and an upstream report is listed among
  the pending reports (`~/.buzz/PLANS/PENDING.md`).
- **What presence reveals.** `presence(1, T)` gives every peer connected to the app, through any
  room, the token of every room the app is in: enough to join each passwordless one (above), and, for
  one with a password, enough to reach its handshake (previous item). The page states it.
- **One self id per app links the app's rooms** (ruling 10). Every room of an app announces the same
  `peerId` in clear on every room topic (`P/doc/protocol.adoc:40-42`), and every connected peer sees
  one id across the rooms it shares with the app: relay observers and peers can link the rooms of one
  program, as Trystero's one `selfId` per page lets them. Today each room draws its own id unless
  named (`room.hpp:578-584`), so a program's rooms are unlinked by default; after the change they are
  linked by default. Rooms that must stay unlinkable are joined through separate `app` objects (two
  ids, two connections, nothing shared). The page states it in "What remains" and in the usage
  page's table of `app_config`.
- **A forged announcement binds nothing over a connection.** In Trystero JS, anyone who knows the
  app and room ids can announce in a room in the name of X, a peer connected to the app through
  another room, and the room binds X's connection by reuse (`C/signal-handler.mjs:478`); X drops the
  frames of a room it is not in, and the room kicks X after its handshake timeout. Here a room binds
  over a connection only a peer that advertised the room's token itself, on that connection
  (section 4.2): a forged announcement or signal in X's name binds nothing, reports nothing about
  X, and starts no rebinding count, so X joins the room at once when it does.
- Every existing protection holds per room: signals that do not open, replays and reflections, the
  negotiation and construction bounds (`P/doc/protocol.adoc:114-185`).

## 5. What still differs from Trystero JS, and why

| Behaviour | Trystero JS | Here | Why |
| --- | --- | --- | --- |
| Scope of sharing and of the self id | the page: one `selfId` for every app and room (`C/utils.mjs:7`), one manager per strategy (`C/strategy.mjs:21`) | an `app` object: its self id and its connections | no static state (`W/:639`); several peers in one process; an id across apps is a program's choice (`app_config::self_id`) and never reaches the wire of another app |
| A peer kicked from a room, or that kicked us, over a live connection | bound again by its next signal or presence (`C/signal-handler.mjs:478`) | bound again at a rebinding time, 11 s doubling to 5 min; the lower id refuses it meanwhile and keeps its frames | a wrong password in one room would otherwise loop reports and announcements without end; one refusing side cannot alternate with another (ruling 9) |
| Reuse over a live connection | binds on any signal of that peer id (`C/signal-handler.mjs:478`) | binds only a peer that advertised the room's token itself, and by reuse only on their first meeting over that connection | a forged announcement must not bind an innocent peer (ruling 9) |
| Announcements after a peer leaves | restarted after every leave (`C/strategy.mjs:63-71,263-269`) | restarted only when no live connection to that peer remains | over a live connection, rebinding goes through the connection (ruling 9) |
| Presence after binding | sent when a room registers (`C/shared-peer.mjs:90-98,109-111`) | also sent again when a room binds by reuse or at its rebinding time | it makes the other side, Trystero JS included, bind at once (ruling 9) |
| An answer to an earlier challenge | checked against the current challenge only (`C/handshake.mjs:15-18`) | accepted when it is correct for any of the last 8 challenges sent since the last completed handshake | a late answer of an ended binding is honest and proves the password; failing it reports a false error (ruling 9) |
| Frames after the higher id's own kick | kept for the next binding (`C/shared-peer.mjs:475-479`) | dropped until the higher id sends presence | they belong to the ended binding; flushed into the next one, they would answer or be answered in its place (ruling 9) |
| Joining a room already joined | the same room (`C/strategy.mjs:37`) | `error::already_joined` | a `room` is a handle whose destruction leaves (section 4.7; ruling 2) |
| Rooms per app | unbounded; a peer advertising a 65th is dropped by the other side (`C/shared-peer.mjs:457-460`) | `join` refuses the 65th, counting rooms in their farewell | the 65th would cost every connection of the app |
| A connection whose last room was kicked | idle 123,333 ms (`C/shared-peer.mjs:255`) | closed at once | a bound: a failed peer holds no connection, as today |
| Relay sockets and the Nostr key | shared by the rooms of the page, topics batched per socket (`N/index.mjs:4,8,81-153`) | per room, as today | not part of this decision; invisible to peers; shared through the app in a later change (ruling 5) |
| Relay and ICE configuration | the first room's relays (`C/strategy.mjs:131-136`); each negotiating room's RTC configuration (`C/strategy.mjs:52`) | the app's, for every room | one configuration for what the rooms share |
| `idle_timeout` | test-only (`_test_only_sharedPeerIdleMs`, `C/strategy.mjs:44`) | a configuration member, Trystero's default | tests and programs that close sooner |
| Frames waiting for a bound room's handler | 64, 10 s, then the connection drops (`C/shared-peer.mjs:483-490`) | no such state | a binding's delivery is set when it is made |
| Renegotiation and media over a shared connection | routed through a control room (`C/shared-peer.mjs:320,376-385,492-496`), streams owned per room (`:281-314`); a connection with any media is destroyed when its last room goes (`:247,254`) | not ported | media is on the roadmap (`P/doc/roadmap.adoc:14-20`) |
| Passive rooms | presence only for active rooms (`C/shared-peer.mjs:110,120-124`) | not ported | as today (`P/doc/differences.adoc:47-48`) |
| Crossed duplicate connections | first registered wins at each side; crossed, both lose and meet again (section 2.12) | the same | matching Trystero's rule is what keeps a C++ and a Trystero JS side agreeing |

The differences of `P/doc/differences.adoc:86-98` (`on_peer_join` does not replay, `peers()` lists
ids, a send to a peer not joined fails) stay, and hold per room.

## 6. Tests

### 6.1 The memory backend's hooks

`test/support/memory_backend.hpp` today delivers every frame, opens both ends of a pair in one post
(`memory_peer_connection::connect_to`, `memory_backend.hpp:639-664`), and never buffers
(`buffered_amount() const override { return 0; }`, `:324`). The cases below need the following, on
`memory_network` (ruling 11). Each end of a pair is named by its id in the network's record of its
peer connections (`memory_backend.hpp:308-312`). Those ids are `int` today (`connection_registry`,
`memory_peer_connection::id`, `next_pc_`: `memory_backend.hpp:312,472,613`); increment 1 makes them
`std::size_t`, the type webcpp's rules give an index, plain `int` being kept for interfaces that ask
for it (`W/:630-635`). Ids follow the order in which connections are made, and the test learns each
one, with the app that made it, through `on_made`; `peer_of` gives the other end of a pair once its
descriptions have paired it. A buffering channel drains byte by byte: each post drains up to
`drain_per_post` bytes from the head of its queue, and a frame is delivered once its last byte has
drained, so a frame larger than `drain_per_post` takes several posts. A virtual clock (ruling 11)
makes every timer of the backend, and its clock, wait for the test:

```cpp
/** Set before any room exists: every timer this network makes, and now_ms and unix_seconds, run
 * on a clock only advance() moves. */
bool virtual_clock = false;
/** Fires, in order, every timer due up to now + by, running the io_context's ready work between
 * them, and leaves the clock at now + by. */
void advance(std::chrono::milliseconds by);
/** The bytes the channel of `pc` holds unsent. */
[[nodiscard]] std::size_t buffered(std::size_t pc) const;
```

The other hooks:

```cpp
/** Called when a connection is made, before make_peer_connection returns, with its id and the ICE
 * servers it was made with: a test gives each app ICE servers of its own to tell them apart. */
std::function<void(std::size_t pc, std::span<const backend::ice_server> servers)> on_made;
/** The other end of `pc`, once the two are paired; `pc` itself until then. */
std::size_t peer_of(std::size_t pc) const;

/** What a filter does with a frame one end sends to the other. */
enum class verdict { deliver, drop, hold };

/** Called with the sending end, the receiving end and the frame; empty: every frame delivered. A held
 * frame holds every later frame from that end behind it, unfiltered until released, as a real data
 * channel delivers in order (ruling 14, N2). */
std::function<verdict(std::size_t from, std::size_t to, std::span<const std::uint8_t>)> filter;
/** Delivers the frames held for `to`, in the order they were sent, each then filtered again. */
void release(std::size_t to);
/** When true, a pair's channels open only through open(), one end at a time. */
bool defer_opens = false;
void open(std::size_t pc);
/** Delivers `frame` on the channel of `pc`, as if its other end had sent it. */
void inject(std::size_t pc, std::span<const std::uint8_t> frame);
/** Reports `failed` on `pc`, then closes its channel. */
void fail(std::size_t pc);
/** 0: buffered_amount() stays 0, as today. Otherwise each channel holds what it was sent, up to no
 * limit, and delivers `drain_per_post` bytes per post, firing buffered_amount_low at the threshold;
 * buffered_amount() is the bytes sent and not yet delivered. */
std::size_t channel_capacity = 0;
std::size_t drain_per_post = 16 * 1024;
```

A frame a buffering channel holds is the copy `send` makes today, in the receiving end's resource
(`memory_backend.hpp:326-340`), held until its turn to be delivered, so buffering moves no
allocation from one resource to another. The `std::function` objects and the frames a filter holds
use the default resource, and only when a test sets them, as `server_urls` does
(`memory_backend.hpp:600-605`): the allocation test sets none, so its counts do not change.

### 6.2 Unit, over the memory backend

`shared_test.cpp`, declared like `room` (`P/test/Jamfile`, `webcpp.run` on native and emscripten) and
once more with `<exception-handling>off` on both targets (`shared_no_exceptions`), as the browser
tests are. Two `app` objects stand for two peers. Every case that involves a password fixes which side
has the lower id, since only the initiator reports `wrong_password` (`room.hpp:1157-1163`) and the
responder answers, passes and sees `@_leave` (ruling 13). The increment that brings each case is in
brackets (section 8).

1. **Two rooms, one connection** [2]. A and B each join `r1` and `r2`; both rooms join both ways, one
   connection is made, and messages of an action of the same name in each room reach only their own
   room.
2. **Three rooms, one connection** [2], joined in a different order on each side.
3. **A late join, both directions** [2]. Connected through `r1`, A joins `r2`, then B; and the
   reverse. `r2` joins over the existing connection, no connection is made, and for the whole case
   no offer and no answer is published on `r2`'s topics (the relay's record, as
   `room_test.cpp:760-811` reads it): a bound room starts no negotiation with its peer (ruling 9's
   first step, I1).
4. **Binding needs the peer's own presence** [2] (ruling 9). The filter drops B's `presence(1, r2)`
   while B joins `r2`, A having joined it first: B binds at its join, on A's presence, and its
   handshake frames reach A, which keeps them and does not bind, since B has not advertised `r2`;
   B's announcements in `r2` bind nothing on A's side either and start no negotiation (no offer and
   no answer on `r2`'s topics). B's handshake times out 10 s after its join and B kicks; at its
   rebinding time, 21 s after its join on the virtual clock, B binds again and sends its presence,
   which the filter now lets through, and A binds at that instant.
5. **Kept frames** [2]. The filter holds B's presence for `r2`; B's `r2` handshake frames reach A
   first; A keeps them and, once bound, delivers them after making B's peer state, so the handshake
   completes. 65 such frames keep 64; 10 s later the kept frames are gone (the app resource's bytes
   back to their level).
6. **A second connection is dropped** [2]. Both rooms negotiate at once; one connection stays, both
   rooms join over it.
7. **Crossed connections recover** [2]. A and B get ICE servers of their own, and `on_made` records
   each connection's id with its maker. With `defer_opens`, A and B join `r1`; the first connection A
   makes is its end of `r1`'s, and `peer_of` gives B's once paired. They then join `r2`, which
   negotiates too, no connection being open: the next pair is `r2`'s. The test opens A's end of `r1`'s
   and B's end of `r2`'s first, then the others: both sides lose both, then both rooms join again
   within 15 s, over one connection.
8. **One room leaves** [2]. A leaves `r1` (and, separately, destroys it, and moves a room over it): B
   sees `on_peer_leave` in `r1` only, `presence(0, r1)` arrives, `r2` keeps exchanging, the
   connection stays, and a send in `r1` completes with `error::room_left`.
9. **A peer leaves every room** [2]. A leaves `r1` and `r2`: B sees both leaves; the connection
   closes after `idle_timeout` (set to 200 ms), not before; a room joined again within it reuses the
   connection; with `idle_timeout` 0 it closes at once.
10. **A kick is per room** [2]. B's `r2` has another password, A the lower id: A reports
    `error::wrong_password` in `r2` only, `r1` stays joined; with only `r2` in common, the connection
    closes at once, and B, whose `r2` signals do not open with A's key, makes no new connection. A
    handshake that times out in one room likewise (B with the lower id, its challenge dropped by the
    filter once); with only `r2` in common, B renegotiates through `r2`'s signaling, and the new
    connection is adopted and bound with no rebinding count.
11. **An action-level kick is per room** [2]. `inject` delivers on A's end a room frame of `r2`'s token
    whose action frame does not decode: A kicks B from `r2` only, `r1` keeps exchanging (ruling 13).
12. **A connection's end is every room's** [1 for one room, 2 for two]. `fail` ends it: both rooms see
    the peer leave, then meet again over one new connection. A frame that does not decode, and a 65th
    advertised token, both by `inject`, end it the same way.
13. **Unknown tokens** [1] are dropped by `inject`, the connection stays, and the app's resource does
    not grow.
14. **Flood isolation** [2]. B floods `r1` with 10,000 inline messages to an action without a handler
    (its queue keeps 1024) while `r2` pings and chats: every `r2` message arrives and each `r2` ping
    completes within 1 s. With `channel_capacity` set, B sends a 5 MB transfer in `r1` while `r2`
    sends 20 chats that must wait for the drain: every chat completes before the transfer does.
15. **Joining** [2]. `join` of a joined room fails with `error::already_joined`; after `async_leave`
    completes, and during the farewell, it succeeds; with 64 rooms joined, the 65th fails with
    `error::too_many_rooms`, and still does while one of the 64 is in its farewell, until its
    `presence(0)` is sent (ruling 14, M6).
16. **The farewell** [2]. During a room's 99 ms farewell, B's presence for it and an announcement in
    it bind nothing, and a connection its own negotiation opens then is closed, not adopted (ruling
    15, M5).
17. **Lifetime** [2]. The `app` destroyed before its rooms: the rooms go on; the last room destroyed:
    every connection closes at that call. Then the `io_context` is destroyed with operations of the
    room still pending: nothing is closed or posted from `app_impl`'s destructor, and the resources'
    counts reach zero (ruling 14, M10).
18. **Self id** [2]. Every room reports the app's id; one named longer than 64 bytes is replaced and
    reported to `app::on_error`; a negative `idle_timeout` is replaced with 0 and reported.
19. **A passwordless room is open to the app's other rooms** [2] (ruling 7). A is in `lobby` and
    `secret`, without a password, and in `vault`, with one; B joins `lobby` only. Through `inject`,
    B's end sends A `presence(1, T)` and an `@_hsready` room frame for each token A advertised: A's
    `secret` reports B joined and sends B its broadcasts, as Trystero JS does; A's `vault` never does,
    and reports the handshake's failure (`handshake_timed_out`, or `wrong_password` when the test has
    B, the higher id, send a wrong response).
20. **The rebinding rate** [2] (ruling 9). A, the lower id, and B share `r1`; B's `r2` password is
    wrong. On the virtual clock, over 100 s: A reports `wrong_password` 4 times, at 0, 11, 33 and
    77 s after the first, exactly, and no fifth before 165 s; B
    reports nothing; neither side publishes on `r2`'s topics more than its regular announcement
    cadence (no restart after a kick), and no offer and no answer. The same with the ids swapped and
    A's password wrong: B, now the lower id, reports at those times, and A nothing.
21. **A room's own resource returns to zero** [3] (ruling 8). With `channel_capacity` set and B
    reading nothing, A's `r2`, in a resource of its own, sends a 1 MB transfer that waits on the
    full channel; A destroys `r2` while `r1` goes on exchanging: once the `io_context` has run the
    posts, `r2`'s resource has no bytes out, and `r1` still exchanges.
22. **A transient failure converges** [2] (ruling 9), in both id orders and in a room with and
    without a password: the filter holds one handshake frame of `r2` past the receiver's
    `handshake_timeout`, then releases it, once before and once after the rebinding time. A the lower
    id times out (B's answer held, or B's `@_hsready`); B the higher id times out (A's `@_hsready`
    held, or, with a password, A's challenge). The side that timed out reports one
    `handshake_timed_out`; on the virtual clock both rooms are joined at the later of the first
    rebinding time, 11 s after the kick, and the release (a held frame holds its end's every later
    frame, so that end completes nothing before the release); no `wrong_password` and no other
    `on_error` report is made. The hold keeps the channel's
    order (section 6.1), and the side that must not time out has `handshake_timeout` 30 s, so that
    which side times out first is fixed (ruling 14, N2); that side's own rebinding step is then 31 s,
    and it binds on the other side's presence at 11 s. A late answer to the old
    challenge passes A whenever it arrives; B drops A's late frames until it sends presence. A third
    variant holds B's answers across A's first two bindings and releases them during A's third, once
    A has bound again and sent its third challenge, so that B's answer to the first challenge reaches a
    bound initiator: A passes on it and reports no `wrong_password`. (Released after A's third kick,
    the answers would reach A while it refuses, kept 10 s, and expire before it binds.) A fourth
    holds them across eight bindings and releases them during the ninth: the answer to the first
    challenge, no longer among the last 8, is `wrong_password`.
23. **A corrected password rejoins** [2] (ruling 9). A the lower id, B's `r2` password wrong: after A's
    third kick, at 33 s, B leaves `r2` and joins it with the right password at 35 s; B joins at 77 s,
    A's rebinding time, exactly on the virtual clock, having reported at most two
    `handshake_timed_out` meanwhile (it
    binds A at once and times out while A refuses it). With B the lower id and its own password wrong, B's new `join`
    joins at once: its rebinding count was its old room's.
24. **A forged announcement before X joins** [2] (ruling 9). X shares `r1` with A; a third socket
    on the relay announces in `r2` in X's name: A's `r2` binds nothing, reports nothing and publishes no
    offer. X then joins `r2` and is joined within a round trip.
25. **A kept `@_leave`** [2] (ruling 14, R3). Before A's `r2` binds B, `inject` delivers on A's end
    B's `@_hsready` and `@_leave` of `r2`; then B's presence is let through: A binds, discards both,
    and the handshake completes on B's next frames, with no leave reported.

`room_test.cpp`, `native_room_test.cpp` and `allocation_test.cpp` move to `app` in increment 2: each
peer is an `app`. Their cases keep their meaning, but a case in which a peer leaves and comes back
with the same id expects a new negotiation, which an idle connection kept by that peer's `app` would
replace: such a case destroys the peer's `app` between its visits, or sets `idle_timeout` to 0 (ruling
13). They are `leaving_is_seen_by_the_others_and_a_return_reconnects` (`room_test.cpp:231-248`),
`a_replayed_answer_is_refused` and `a_replayed_offer_is_refused` (through `join_and_leave`,
`room_test.cpp:797-806,848,882`), and `leaving_is_seen_over_a_real_connection`
(`native_room_test.cpp:158`).

### 6.3 Allocation

`allocation_test.cpp` gains a case [3]: two apps, each with two rooms, one room in the app's resource
and one in a resource of its own; from construction until the `io_context` drains, the global
`operator new` sees only the rooms' JSON storage counters and the C library nothing, natively and on
emscripten (`P/doc/guarantees.adoc` invariant 5); a binary payload received by the room with its own
resource is copied into it once, and by the other moved. Case 21 above is its companion.

### 6.4 Interop, against Trystero JS 0.26.0, on both lanes

`interop_browser` and `interop_native` (`P/test/oracle/Jamfile`) [4]. `peer.cpp` and `native_peer.cpp`
make one `app` and join `room`, `room2` and, with a new parameter, `room3` through it; `js.html`
joins the same rooms, honours `idle` (`_test_only_sharedPeerIdleMs`, `C/strategy.mjs:44`) and
records, through its `RTCPeerConnection` wrapper (`js.html:53-60`), each connection it makes and its
state. The C++ peers record each connection their app adopts, from a log line the app writes on
adoption, which each peer's sink (`peer.cpp:66,103`, and the native peer's) turns into an event;
the native peer's `counting_backend` (`native_peer.cpp:93-106`) records each connection made
(ruling 14, M11). Scenarios that let both rooms negotiate at once can meet the crossed race of section
2.4: they allow 60 s, record how many connections each side made, and fail, naming the counts, when a
side made more than 4, so that a livelock fails by name and not as flakiness.

1. **Two rooms on one connection** (replaces `twoRoomsScenario`, `run.mjs:578-601`), no password:
   both rooms' exchanges complete both ways (join, chat, the 100 KB file); at the end the page holds
   exactly one connected `RTCPeerConnection` and the C++ peer's app has adopted one connection.
2. **Three rooms on one connection**, the third joined by the C++ peer 5 s after the others, by
   presence.
3. **A room the page joined first.** The page joins `room` and `room2`; 15 s later the C++ peer joins
   `room`, then `room2`: `room2` binds over the existing connection by the page's presence and joins
   within 5 s of the C++ peer's join.
4. **A passwordless room is open to the app's other rooms** (ruling 7). The C++ peer joins `room`, a
   passwordless `room2` and a `room3` with a password; the page joins `room` only, and, through its
   own handle on its channel, sends `presence(1, T)` and an `@_hsready` room frame (encoded as
   Trystero's action wire encodes an internal action, `C/action-wire.mjs`) for every token the C++ peer
   advertised: the C++ peer's `room2` reports the page joined and sends it its broadcasts, and its
   `room3` never reports it joined.
5. **One room leaving.** The C++ peer leaves `room`: the page records `peer-leave` in `room` only;
   a chat in `room2` sent afterwards arrives both ways; no connection is made. The same with the
   page leaving.
6. **A peer leaving every room.** The page leaves both rooms: the C++ peer records both leaves, and
   the connection closes after the idle time both sides are given (2 s), on both sides.
7. **A wrong password in one room, in both id orders** (ruling 9). The page joins `room2` with
   another password. With the C++ peer the lower id (`cppSelf` `0...0`, as in `run.mjs:605`), the C++
   peer reports `wrong_password` in `room2` 3 times in 60 s, at about 0, 11 and 33 s. With the C++
   peer the higher id (`z...z`, as in `run.mjs:606`), the page records Trystero's password error at
   most 4 times in 60 s, paced by the C++ peer's rebinding times and, at most once, by its regular
   announcement, which the interop's one relay delivers once (`run.mjs:731`; with `relay_redundancy`
   relays it would be once per relay), and the C++ peer reports nothing. In both, `room` keeps exchanging over the same
   connection, and no new connection is made.
8. **A flood in one room.** The page sends 5,000 messages on `room`'s `chat` while `room2` pings:
   every ping completes within 2 s, and the C++ peer stays joined in both.
9. **A transient handshake failure with Trystero JS, in both id orders** (ruling 9), `room2` with a
   password. `js.html` gains `hold-hsdata`: a wrapper on `RTCDataChannel.prototype.send`, as the page
   already wraps `WebSocket.prototype.send` (`js.html:47-52`), that holds the page's first `@_hsdata`
   room frame of `room2` for 12 s, with every later frame of that channel behind it, as a data channel
   delivers in order; and the page's `joinRoom` is given `handshakeTimeoutMs: 30000`
   (`C/strategy.mjs:32`), so that the C++ peer, with its 10 s, always times out first (ruling 14, N2).
   The held frames then reach the C++ peer 2 s after its kick, inside its dropping window or kept by
   its backoff. With the page the higher id, its answer is held: the C++ peer times
   out once, accepts the late answer, and both are joined in `room2` within 15 s of its kick. With the
   page the lower id, its challenge is held: the C++ peer times out once and drops the page's frames
   until its presence at 11 s, and both are joined within 15 s of its kick, or, when the page bound by
   reuse inside that window (the first residual case of section 4.2, recorded), within 50 s, the
   page having reported one handshake timeout. In both, no password error is reported on either
   side, and `room` keeps exchanging over the same connection.
10. The existing scenarios (`P/AGENTS.md:182-215`) pass unchanged in meaning, with the C++ peer an
   `app` of one room; the replay scenario's page drops its connections (`js.html:164-169`), so no
   idle connection is reused there.

### 6.5 What does not change

The `oracle` lane and `protocol` test: no wire value changes. The browser and native backend tests.

## 7. Pages and files to update

- `doc/differences.adoc`: the row "One connection shared by every room of an app between two peers"
  (`differences.adoc:50-53`) goes; "A peer id per room, not per app" becomes the table of section 5.
- `doc/roadmap.adoc`: "one connection shared between several rooms of an app" leaves item 2.
- `doc/usage.adoc`: "Joining" becomes the app and its rooms (the tables of `app_config` and
  `room_options`); `self_id`'s advice to give each room an id becomes: one id per app links its rooms,
  and rooms that must stay unlinkable use one `app` each. A new subsection, "Moving from
  `room_config`", states the migration for existing users (ruling 1): the standalone constructor and
  `room_config` are gone, the table of section 4.5 maps each member and each use to its new place
  (the log, the configuration reports and the resources among them), two peers in one process become
  two `app` objects, and `join` returns a result to test. "Peers" says what a kick does over a shared
  connection and when the peer is bound again (section 4.2), and that over a shared connection a
  wrong password is reported by the lower id only: the higher id, which today learns it through its
  signals that do not open, gets no report there. The README's example and its paragraph on
  `room_config` (`P/README.md:31,42`) say the same.
- `doc/protocol.adoc`: "The data channel" says what presence does and how frames are routed; "Bounded signaling" gains the bounds of section 4.9 and
  the backoff; "What remains", the page's security section, gains section 4.12's items: the password
  exposure accepted as Trystero JS has it (ruling 4); that a passwordless room is not private from
  the peers of the app's other rooms, with the mechanism, and that a private room takes a password or
  an `app` of its own (ruling 7); what presence reveals; the linking of an app's rooms by its id; and
  that a forged announcement binds no connected peer. Its sentence that a watcher without the key
  cannot join a room (`protocol.adoc:166-176`) is narrowed to a watcher not connected to the app.
- `doc/design.adoc`: a section on ownership (section 4.1), and the layers' table names the app.
- `doc/allocation.adoc`: "One resource per room" becomes the app's resource and the room's, the log's
  sentences in the app's.
- `doc/guarantees.adoc`: invariant 3 says a peer with another password never joins, through its
  signals or over a shared connection; new invariants: one connection per remote peer per app, shared
  by its rooms (held by `shared` and the interop's first scenario); a room's departure leaves the
  app's other rooms joined (`shared` 8, interop 5); a room with a password never admits a peer of
  another room of the app without that password (`shared` 19, interop 4). That a passwordless room is
  open to those peers is stated in "What remains", not as a guarantee.
- `doc/testing.adoc`, `doc/overview.adoc` (the memory resource per room), `doc/quick-start.adoc`,
  `doc/browser.adoc`, `doc/native.adoc` (a room's window, `native.adoc:86-90`), `README.md`.
- `AGENTS.md` (trystero's): the interop scenarios (`AGENTS.md:205-206`), the allocation rules
  (`:261`), the bounded signaling paragraph, the memory backend's hooks.
- Header comments that name `room_config` or the room's resource (ruling 14, M9): `handler.hpp:44`,
  `json_resource.hpp:29`, `detail/log.hpp:29,38`, `native/backend.hpp:105,135`,
  `browser/backend.hpp:899`, `native/peer_connection.hpp:251`; the backends' say that peer
  connections are made in the app's resource. `error.hpp`'s `invalid_configuration`.
- The examples `quick_start.cpp`, `native_quick_start.cpp`, `chat/chat.cpp`, and the interop peers.
- The superproject's `AGENTS.md` mentions trystero's resource per room (`W/:668-671`).

## 8. Increments

Before increment 1: the `trystero/room` CI regression at `54f96d8` is fixed (a separate change), and
every citation of `P/` in this specification is checked again against the commit the plan starts
from (ruling 6). Each increment leaves every lane green, and each test it adds uses only what it has;
the change is published once increment 5 has brought the pages in line (ruling 12).

1. **The connection layer, one room per app.** `detail::app_impl`, `detail::connection` and
   `detail::binding` replace `peer_link`; presence and routing as section 4.3; `presence(0)` and kicks
   scoped to the room; connection objects in the app's resource; the memory backend's hooks
   (section 6.1), with the network's connection ids made `std::size_t`. The public API is unchanged: each standalone `room` builds an `app_impl` of its own.
   Tests: every existing suite, and cases 12 (one room) and 13, written against the standalone
   `room` in `room_test.cpp`.
2. **The app.** `app`, `app_config`, `room_options` (without `memory`), `app::join`, the two errors,
   the self id per app, adoption, the three binding paths and their conditions, the rebinding times
   and the lower id's backoff, kept frames, the idle timer, the bounds; the standalone `room(ex, factory, room_config, room_id)` and
   `room_config` removed (ruling 1), and with them every test, example and interop peer moved to
   `app`. Every room is in the app's resource. Tests: `shared` cases 1 to 20 and 22 to 25, both targets, both
   exception modes; the moved suites, with the cases of section 6.2's last paragraph adjusted.
3. **Allocation.** `room_options::memory`, the room's own resource; the counting test of section 6.3
   and case 21.
4. **Interop.** The scenarios of section 6.4 on both lanes.
5. **The pages.** Every page and file of section 7, the migration among them.

No state of the tree has both the standalone constructor and `app`: increment 1 has the first,
increment 2 replaces it with the second.

## 9. Rulings

The owner ruled on each open choice on 2026-10-11; every section above follows these rulings, and the
owner may reverse any of them.

1. **The public shape: `app`, and the standalone room removed.** Rooms are joined through an `app`
   object, `app::join` returning `boost::system::result<room>` (section 4.5); the standalone
   `room(ex, factory, room_config, room_id)` and `room_config` are removed, in increment 2, and the
   page states the migration for existing users (section 7). *Why:* one way to join, and no program
   falls back into one id per room, the trap the decision of 2026-10-11 removes (two standalone rooms
   with one id meet a Trystero JS peer in one of them only). *If wrong:* every existing user rewrites
   how it joins (a mechanical change, section 4.5); bringing the standalone form back later, as an app
   of one room, adds a constructor and breaks nothing.
2. **Joining a room already joined: `error::already_joined`** (section 4.7). *Why:* a `room` is a
   move-only handle whose destruction leaves; a second handle to one membership would make that depend
   on another handle. *If wrong:* a program that joins the same room from two places, as Trystero JS
   lets it, must share its one `room` itself; switching to shared handles later changes what
   destroying a `room` means, a breaking change.
3. **Resources: the app's, plus an optional resource per room, the app's by default** (section 4.8).
   *Why:* it keeps the room's resource the allocation decision of 2026-10-08 gave every room
   (`D/:648`), while what rooms share lives in the app's. *If wrong:* one copy of a received binary or
   text payload when the two resources differ, a second lifetime rule to document and test; removing
   `room_options::memory` later breaks the programs that set it.
4. **The password exposure is accepted and documented** (section 4.12), as Trystero JS has it, on the
   page's security section ("What remains" of `doc/protocol.adoc`); an upstream report is listed in
   `~/.buzz/PLANS/PENDING.md`. *Why:* the only local remedy, never binding a password room over a
   connection its own signaling did not make, means never meeting a Trystero JS peer in that room;
   closing it needs a wire change Trystero would share. *If wrong:* a peer of another room of the app
   can collect material for an offline guess of a room's password, which a strong password resists
   and a weak one does not.
5. **Relays per room now, shared later** (sections 3 and 5). *Why:* no peer sees the difference, and
   sharing sockets, batching topics and one Nostr key per app touch the strategy's bounds and
   reconnection tests, which deserve their own specification. *If wrong:* each room of an app keeps its
   own sockets (5 per room by default) until that change.
6. **Built now, on the current backend interface, after the `trystero/room` CI regression at
   `54f96d8` is fixed** (section 8), by a separate change already under way. The citations of `P/` refer
   to `54f96d8` and are checked again when the plan is written. *Why:* the backend interface does not
   change here, and the ownership designed here is what the later move onto webcpp::asio,
   webcpp::webrtc and webcpp::crypto (`D/:837,840`) carries over. *If wrong:* the move rewrites the
   connection layer's calls into the backend, but not its ownership, routing or bounds.
7. **No proof gate: a passwordless room binds by its token, as in Trystero JS** (the owner,
   2026-10-11, on the re-review's R1; sections 4.2, 4.12). Presence and kept frames bind a room with
   or without a password, as Trystero does, and a binding by reuse sends no counter-announcement; the
   security section and the page state plainly that a passwordless room is not private from the
   peers of the app's other rooms (presence gives them its token, the relays its topics, and the
   token alone binds), and that a room that must be private takes a password or an `app` of its own.
   *Why:* the gate first proposed for C1, a message on the room's topics, is passed by anyone who
   reads the relays, where every event carries its topic and every announcement the app's id, so it
   gave false assurance, and it cost up to 10 s against a Trystero JS peer that joined first, and
   code. *If wrong:* a program that relies on an unguessable room id for privacy, against peers of
   its other rooms, is exposed, as it is with Trystero JS; it takes a password or a separate `app`.
8. **Each binding owns, fails and frees its own drain waiters** (I2; sections 4.1, 4.2, 4.3, 4.8): the
   connection keeps only plain pointers in its drain order, and detaching a binding fails its
   waiters and frees them, in the room's resource, before it returns. *Why:* the waiters are the
   room's handlers, frames and timers; held by the app's connection they would outlive a room with a
   resource of its own. *If wrong:* a little more bookkeeping per binding than one list per
   connection; the order of turns is the same.
9. **A bound room keeps its negotiation's connected state; a binding needs the peer's own presence;
   after a kick, the pair rebinds at scheduled times, and only the lower id refuses meanwhile** (I1,
   and I3 as rewritten for the re-review's R2, with the R6, R8 and N1 of the later re-reviews;
   section 4.2). Binding keeps the room's negotiation entry for that peer, marked connected, so a
   bound room starts no negotiation with it (case 3). A room binds over a live connection only a peer
   that advertised the room's token itself, and by reuse only on their first meeting over that
   connection. When a binding ends while the connection stays live, by a kick of either side, the
   room rebinds one step after the end, the first step the larger of `handshake_timeout` and
   Trystero's 10 s keep time, plus 1 s (11 s by default), doubling per such end to 5 minutes, the
   count lasting until a completed handshake or the connection's end, and sends `presence(1, T)` when
   it does; meanwhile the lower id, after its own kicks only, refuses the peer, ignores its signals
   and keeps its frames, while the higher id refuses nothing. No announcement is restarted while the
   connection stays live. Across a rebinding, the initiator accepts a correct answer to any of the
   last 8 challenges it sent that peer since their last completed handshake (its own nonces, on a
   channel no third party can write to), and only an answer matching none is `wrong_password`; the
   responder answers one challenge per binding, as today and as Trystero JS; and after its own kick,
   the higher id drops the peer's frames of that room until it next sends presence. *Why:* without
   the first, a room bound by presence or reuse would offer to the peer it is bound to; without the
   rest, a wrong password in one room of two honest peers loops a kick, a report and an announcement
   burst per round trip, two refusing sides alternate forever, a forged announcement in an innocent
   peer's name binds and locks it out, and stale handshake frames of an ended binding make honest
   peers report false password errors and miss a step. With them, a transient failure joins by the
   first rebinding time in every pairing of section 4.2, with two named residual cases, both with a
   Trystero JS lower id and neither reporting anything false from the C++ side: a reuse binding
   inside the higher id's dropping window (joined by the second rebinding time, at most 43 s by
   default plus the one-way delay, once at most per failure), and a Trystero JS page whose timers run
   later than the margin of the 1 s added to the step (one false password error on the JS side and
   one more step, recurring under Chrome's intensive throttling until the step exceeds the page's
   timer lag, at most three times at the default settings); a wrong password costs at most six
   reports in ten minutes, on the lower id only; and a forgery binds nothing. *If wrong:* a peer
   whose password is corrected waits at most the step that runs, up to 5 minutes after many
   failures; a peer with the lower id can rebind a higher-id C++ room at its own pace, one
   `handshake_timed_out` per `handshake_timeout`; an initiator keeps up to 8 challenges of 36
   characters per peer and room until a handshake completes.
10. **One self id per app, and its linking documented** (I4; sections 4.4, 4.12): the security
    section and the usage page say that one id per app links its rooms for relays and peers, that
    today's port drew an id per room, and that rooms that must stay unlinkable use separate `app`
    objects. *Why:* sharing a connection needs one id per app, as Trystero's one `selfId` per page;
    the user decides which rooms may be linked. *If wrong:* programs that relied on today's unlinked
    rooms are linked until they split their apps.
11. **The memory backend's hooks are specified** (I5 and R4; section 6.1): `on_made` with the
    connection's id and ICE servers, `peer_of`, a frame filter that delivers, drops or holds,
    `release`, deferred opens and `open`, `inject`, `fail`, and a channel that buffers and drains,
    holding what it was sent in the receiving end's resource and draining it byte by byte; the
    network's ids become `std::size_t`. A virtual clock is one of these hooks: with
    `virtual_clock` set before any room exists, every timer the memory backend makes, and its
    `now_ms` and `unix_seconds`, run on a clock that only the test advances, so that the cases of
    section 6.2 assert instants in virtual time, exactly, with no wall-clock slack, and take no
    wall-clock time to wait. *Why:* the cases of section 6.2 cannot be written without them, and
    timing cases on a scaled wall clock flake on loaded machines. *If wrong:* a hook added later, in
    the test support only.
12. **The removal goes with `app`, in increment 2, and each increment tests only what it has** (I6;
    section 8). *Why:* no state of the tree has two ways to join, each increment leaves every lane
    green, and the change is published once the pages follow. *If wrong:* increment 2 is larger than
    it would be with the removal last.
13. **The missing tests are added** (I7; sections 6.2, 6.4): an action-level kick in one room, peers
    that leave and come back with the same id destroying their `app` between visits or setting
    `idle_timeout` to 0 (the cases listed in section 6.2), a fixed lower-id side in every password
    case, a passwordless room open to the app's other rooms and a password room closed to them, the
    rebinding rate, the convergence of a transient failure in both id orders, in unit tests and
    against Trystero JS (interop 9), an answer to a challenge two bindings old, a corrected password, a
    forged announcement, a kept `@_leave`, a room's own resource returning to zero, and the farewell's
    edges. *Why:* each guards a behaviour of this change that no other case reaches. *If
    wrong:* longer suites.
14. **The independent review's twelve minor findings** (section 4 and section 7) are applied as it
    proposed: the order at binding (M1, section 4.2), forged announcements (M2, sections 4.2 and 4.12,
    where ruling 9 now makes them bind nothing),
    re-entrancy at a connection's end (M3, section 4.2), the references between binding and
    connection (M4, section 4.1), a leaving room binds and adopts nothing (M5, section 4.2),
    `max_rooms` counts a room until its `presence(0)` (M6, sections 4.5, 4.9), the small inaccuracies
    (M7, sections 1, 2, 4.3, 4.9), the API details (M8, section 4.5), the header comments (M9, section
    7), the app's state destroyed with the `io_context` (M10, section 4.2), the interop's
    observations (M11, section 6.4) and the log (M12, sections 4.5, 4.8); and the re-review's two
    minor findings: a kept `@_leave` discards itself and what was kept before it, and an initiator
    ignores `@_hsdata` until it has sent its challenge (R3, section 4.2), and the hooks' ids, `on_made`
    and the buffering channel's resource (R4, section 6.1); the fourth re-review's N1, a first step
    of the larger of `handshake_timeout` and 10 s plus 1 s, with residual case 2 stated with its
    margin and its recurrence under intensive throttling (section 4.2), and N2, the tests' holds kept
    in channel order and the side that times out first fixed by a longer handshake timeout on the
    other side (case 22, interop 9); and the second re-review's minor finding
    R7: a Trystero JS lower id's password errors follow each relay's delivery of an announcement, and
    the usage page says that over a shared connection only the lower id reports a wrong password
    (section 4.2, section 6.4 scenario 7, section 7). *Why:* each corrects or
    completes the design without changing a ruling. *If wrong:* each is local to the section named.
