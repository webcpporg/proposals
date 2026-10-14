## Why

Trystero shares one WebRTC connection between two peers across every room of an app that both are
in, each frame naming its room; webcpp::trystero 0.1.0 does not. There, each `room` is a world of
its own: its own peer id (drawn per room unless named), its own negotiations and its own
connections, and a link ignores every frame that names another room's token. The page lists it as
a difference and a roadmap item, and the interop lane works around it by giving each C++ room an id
of its own. A program with several rooms therefore makes several connections to the same peer,
appears under several ids on the relays, and two rooms of one app given the same id meet a Trystero
JS peer in one of them only. The owner decided on 2026-10-11 that trystero shares one connection
between the rooms of an app, as Trystero does, and that the difference goes away.

## What Changes

- **A new public `app`** (`<webcpp/trystero/app.hpp>`): one peer id per app, one connection per
  remote peer, shared by every room of the app that both peers are in. `app_config` holds what the
  rooms share (app id, self id, immutable password, relays, ICE servers, `idle_timeout`, log, memory) and
  `room_options` what a room adds (handshake timeout, receive bound and its own
  memory). Rooms are joined through `app::join`, which returns a `boost::system::result<room>`.
- **BREAKING:** the standalone `room(executor, factory, room_config, room_id)` and `room_config`
  are removed. The migration is mechanical and the page states it.
- **BREAKING:** the default peer id is now one per app, not one per room: the rooms of a program
  are linked on the relays and to peers by default, and rooms that must stay unlinkable are joined
  through separate `app` objects.
- **BREAKING:** one immutable `app_config::password` for all rooms of an app, with no per-room
  override. Different passwords require separate app objects and distinct self ids. Initial
  sealed signaling authenticates the domain before room reuse. The JS one-way handshake wire is
  preserved; a remote JS page that mixes room passwords retains its own upstream exposure.
- **New errors** `already_joined = 14` and `too_many_rooms = 15`; the message of
  `invalid_configuration` becomes "the configuration is unusable".
- A connection layer (`detail::app_impl`, `detail::connection`, `detail::binding`) replaces
  `detail::peer_link`: presence frames advertise a room on every connection of the app, room frames
  are routed by token to the room's binding, frames for a room not bound yet are kept briefly, and
  a kick or a departure ends a binding in its room only.
- Binding rules and a rebinding schedule: a room binds a connected peer only if that peer
  advertised the room's token itself; after a kick the pair rebinds at scheduled times (11 s
  doubling to 5 min) with only the lower id refusing meanwhile; the initiator accepts an answer to
  any of its last 8 challenges. A forged announcement binds nothing.
- Wire unchanged: the oracle lane stays byte-identical.
- Test support: the memory backend gains a virtual clock, a buffering channel, a frame filter,
  deferred opens, injection and failure hooks, and its connection ids become `std::size_t`.
- The interop lane gains the sharing scenarios against Trystero JS 0.26.0 on both lanes; the page,
  the rulebook and the header comments follow.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `trystero`: ADDED requirements for the app (`App:`), the shared connection (`Shared:`), the binding rules and the rebinding schedule (`Binding:`, `Rebinding:`), what sharing changes for security (`Security:`), the app's and a room's own resources (`Memory:`), the test support and tests (`Tests:`) and the pages (`Docs:`); MODIFIED requirements for what the genesis states for a standalone room (joining, peer id, relays, ICE servers, the configuration, the handshake, kicks, peer removal, leaving, the log, the peer link, the error enumeration and messages, the resource per room, the convenience header, the guarantees, the memory backend's description, the interop scenarios and the pages); REMOVED the requirement `Room: room_config members and defaults`.

## Impact

- **Kind:** a port: it gives the port Trystero 0.26.0's model of one connection per remote peer
  per app (`"port-of"`: Trystero 0.26.0).
- **Implementation:** integrated in [trystero PR #1](https://github.com/webcpporg/trystero/pull/1),
  library commit `2e2dcd4`, then pinned by [webcpp PR #3](https://github.com/webcpporg/webcpp/pull/3).
  The recovered original branch remains recorded in `reference/patches/` and
  `reference/uncommitted.diff`.
- **Dependencies:** none new.
- **Status:** resumed implementation on 2026-10-14, with the owner-approved password-domain
  amendment in `reference/password-domains.md` and the live task checklist in `tasks.md`.
  `reference/plan.md`, `reference/spec.md`, patches and `reference/uncommitted.diff` remain
  historical evidence. Code is integrated and locally validated; the status remains `implementing` pending a release.
  No CI was executed and no release was published.
- **Other proposals:** modifies the capability the `genesis` creates; the move of trystero onto
  webcpp::asio, webcpp::webrtc and webcpp::crypto is a change of its own that comes after this one.
