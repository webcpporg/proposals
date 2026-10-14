# Amendment: immutable app password domains

Approved by the owner on 2026-10-14 while resuming the shared-connection implementation.
This amendment preserves Trystero JS 0.26.0's wire and supersedes the mixed-password assumptions
in the historical `reference/spec.md` and `reference/plan.md`. Those references, the archived
patches and `reference/uncommitted.diff` remain unchanged. The active requirements are in
`specs/trystero/spec.md`; the active execution checklist is in `tasks.md`.

## Decision

An app is one immutable password domain. `app_config` gains `std::pmr::string password`, empty
by default, immediately after `self_id`. The app copies it into `app_config::memory` when it is
constructed. `room_options` loses `password`; every room reads its app's password, without a
per-room override or a copy into the room's resource. Changing the domain password requires a
new app object. Different passwords require separate apps with distinct self ids, including when
they use the same application id or room id.

Each app owns its connection registry and room registry. A connection, advertised token,
presence frame or binding from one app is never reused by another app. Room token bytes remain the unchanged hash of application id and room id and may therefore be
identical across two domains; isolation comes from scoped connections and registries, not a new
token format or secrecy. Same-password rooms in
one app continue to share one connection per remote peer; separate apps remain separate peers
and do not share connections even when their passwords are equal.

## Admission and compatibility

A protected C++ app authenticates its password through the initial encrypted signaling before
adopting a connection. All rooms that later bind through that connection use that same password.
The domain boundary therefore protects admission to the C++ app's rooms without requiring a new
handshake message or changing an existing frame, token, topic, digest or protocol constant.

The Trystero JS 0.26.0 room handshake remains one-way: the lower id challenges and verifies the
response, while the higher id answers and passes its local side. This amendment does not add
reciprocal room proofs and does not claim to repair the upstream wire.

A JS page that itself mixes passwords among rooms of one app retains the upstream exposure:
a peer connected through one room can reach another room's handshake, and a higher-id JS
responder can admit it without that room's password. Its challenge response can also provide
material for an offline password guess. The C++ admission guarantee concerns the C++ app's
own password domain, not a remote mixed-password JS page. JS peers should use one password for
all rooms of an app and separate page contexts for different password domains.

A passwordless app's connected peers can still join its other rooms by echoing advertised tokens.
A protected room instead belongs to a separate app with a password and a distinct self id; its
tokens and presence are not advertised over the passwordless app's connection.

## API and migration

| Previous placement or use | Amended placement or use |
| --- | --- |
| `room_options::password` | `app_config::password`, after `self_id` |
| `room_config::password` in the standalone API | `app_config::password` |
| Different room passwords within one C++ app | Separate app objects with distinct self ids |
| Leaving and rejoining a room to change its password | A new app with the changed password and a distinct self id |
| Room-resource copy of the password | One immutable app-resource copy, read by all rooms |
| Same-password rooms in one app | Shared connections and existing per-room actions, queues and bounds |

The standalone `room` constructor and `room_config` remain removed. Existing users construct an
app with the application id and password, then call `join` with the room id and room options,
and test the returned result. `room_options` contains handshake timeout, receive limit and an
optional room memory resource only.

## Faults within an authenticated domain

A peer that knows the app password can still send malformed handshake data or a wrong room
proof. This is a faulty domain member, not another password domain gaining admission. The room
kicks only that binding and retains the other rooms' transport. Only the lower id verifies a
response proof; either receiver can reject malformed handshake data. Higher-id timeout reports
are bounded by the handshake timeout, while malformed-challenge reports follow a malicious peer's
rebinding pace. Existing challenge history,
rebinding steps, the lower id's backoff, the higher id's dropping window, drain fairness and
resource bounds remain in effect.

Backoff and pacing tests first establish a connection under the common app password and then
corrupt selected room proofs. Correcting that proof behavior preserves the running backoff;
changing the actual password uses a new app. If the last binding is kicked, the transport closes
at once and a later retry requires a fresh encrypted negotiation within the construction window.
A known-password peer can authenticate that new negotiation; a different-password peer cannot.

## Required evidence

- Public API tests reject per-room password overrides and prove app-owned password storage.
- Same-password apps with several rooms exchange over one connection per remote peer.
- Separate passwordless and protected apps isolate presence, tokens, bindings and connections
  in both id orders, using distinct self ids.
- Shared backoff, challenge-history, fault-recovery and fairness tests run after initial domain
  authentication; malformed proofs retain the existing report and resource bounds.
- Native and browser interop use Trystero JS 0.26.0 unchanged, including a common-password shared
  connection, separate protected domains and incorrect-proof pacing.
- The protocol oracle remains byte-identical. Allocation, lifetime, exceptions-off, hardening,
  sanitizers, strict documentation and lint remain local validation requirements.

CI remains prohibited for this session. Integration follows the resumed implementation task
and its controller gates; this amendment does not request or publish a release.
