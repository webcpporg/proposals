## Context

webcpp::trystero 0.1.0 ports Trystero 0.26.0's serverless WebRTC rooms over Nostr, byte for byte
on the wire, with two backends folded into one platform backend (webcpp::webrtc, webcpp::beast,
webcpp::asio). Its cryptography is its own: SHA-1 and SHA-256 from Boost.Hash2, a `sealer` per
backend (OpenSSL's AES-GCM natively, `crypto.subtle` through EM_ASM in a page), randomness from
OpenSSL's `RAND_bytes` or the page's `getRandomValues`, and a BIP-340 signer on libsecp256k1.

Trystero 0.27.0 (upstream commit `8d3f57e`) changes the wire throughout. Everything it computes
with WebCrypto is available in webcpp::hash and webcpp::crypto today except ECDSA P-256 and P-256
ECDH, which webcpp::crypto's foundation increment 3 plans (`curves::p256`, design D36 there). The
P-256 handshake runs on every room, so no 0.27.0 room can be joined before it exists.

The owner asked for the port on 2026-11-20 and is on autopilot: the rulings below are the
controller's, recorded here.

## Goals / Non-Goals

**Goals:**

- A room joined from C++ and the same room joined from Trystero 0.27.0's JavaScript are one room.
- Every value of the wire is proven against 0.27.0's own code, run in Node (`reference/vectors/`).
- trystero's cryptography is webcpp::hash's and webcpp::crypto's, on every target.
- The port reaches `main` once, whole and interoperable.

**Non-Goals:**

- The strategies other than Nostr (BitTorrent, MQTT, Supabase, Firebase, the WebSocket relay and
  its server): D3.
- Media (`addStream`, `onPeerStream`), as in 0.1.0.
- Persistence of an identity by the library (IndexedDB's `identity: 'local'`): D15.
- Compatibility with 0.26.x peers: 0.27.0 has none either.

## Decisions

### D1. One long-lived branch, merged once

0.27.0's wire is incompatible with 0.26.0 and its handshake needs P-256 on every room, so any
partial merge would leave `main` meeting nobody. The port lives on the library branch
`port/trystero-0.27` and the superproject branch `trystero/port-0.27`, both from `main`, rebased
onto `main` at each phase's end. Phase 1 turns off the oracle and interop lanes on the branch (an
`explicit` on their targets) and Phase 2's Task 2.6 turns them back on against 0.27.0; nothing is
merged until they are green and the CI of both repositories is green. Draft pull requests carry
the branch for review. Phase 3's plugins are features on top of an interoperable library and
merge as separate changes after it. Rejected: merging Phase 1 behind a macro (two wires in one
library, and a `main` whose default meets nobody); waiting with all work until P-256 merges
(Phase 1 is half the port and needs nothing of P-256).

### D2. trystero moves onto webcpp::crypto and webcpp::hash (supersedes 2026-11-17)

On 2026-11-17 the owner dropped trystero's migration onto webcpp::crypto (crypto genesis D23,
Tasks 31 and 32). On 2026-11-20 he asked for the 0.27.0 port on webcpp::crypto; that request
supersedes the earlier decision, and this change is "the later change of trystero's own" crypto's
genesis anticipated. The mapping:

| 0.27.0 through WebCrypto | webcpp |
| --- | --- |
| SHA-256 | `webcpp::hash::digest_of<digests::sha2_256>` |
| HMAC-SHA-256 sign and verify | `webcpp::hash::hmac_sha2_256`, `webcpp::crypto::equal` |
| HKDF-SHA-256 | `webcpp::hash::hkdf<hmac_sha2_256>` |
| PBKDF2-SHA-256, 600,000 iterations | `webcpp::hash::async_pbkdf2` (D5) |
| AES-256-GCM, 12-byte IV, AAD | `webcpp::crypto::aes256_gcm` |
| `getRandomValues`, `randomUUID` | `webcpp::crypto::random_device`, `protocol::uuid_v4` |
| BIP-340 (noble's `schnorr`) | `webcpp::crypto::curves::secp256k1::keypair::sign_schnorr`, `verify_schnorr` |
| ECDSA P-256, raw `r || s`, low S | `webcpp::crypto::curves::p256` (increment 3): `sign_ecdsa`, `verify_ecdsa`, `normalize_ecdsa`, `check_low_s` |
| ECDH P-256 raw x (scale) | `curves::p256::keypair::ecdh_x` |

### D3. Nostr only

Today only Nostr is ported, and the library's tests run a Nostr relay of their own. 0.27.0's other
strategies share the core, so a later change can add one by porting its `init`, `subscribeTopic`
and `publishTopic`; none is in scope, and the WebSocket relay's server is not ported either.

### D4. The crypto layer is synchronous protocol code, in the core

Hashing, key derivation from a master, sealing and opening, room auth and ids are pure functions
in `protocol/` (`topics.hpp`, `ids.hpp`, `room_keys.hpp`, `seal.hpp`), tested against the
vectors on both targets. The backend's `sealer` and `factory::make_sealer` are removed: a signal
of a few kilobytes seals in microseconds with BearSSL or OpenSSL, and an asynchronous seal only
existed because the page's WebCrypto is asynchronous. webcpp::crypto's browser backend (its
increment 4, part B) is therefore not needed; its asynchronous family (increment 4, part A) is
not used either.

### D5. PBKDF2 never runs on a room's thread

600,000 iterations take about 0.2 s natively and several seconds in WebAssembly; Trystero runs
them in WebCrypto, off the page's thread. The platform backend implements
`factory::derive_password_master`: natively with `webcpp::hash::async_pbkdf2<hmac_sha2_256>` on a
one-thread `webcpp::asio::thread_pool` it owns (made on first use; the native backend already
requires threads), on Emscripten with `async_pbkdf2<hmac_sha2_256, backends::webcrypto>` on the
page's loop executor. Everything else of the crypto layer is synchronous with webcpp's default
backends (BearSSL on Emscripten and WASI, as webcpp::crypto's defaults pick). Rejected: a
synchronous PBKDF2 on the room's executor (stalls a page for seconds); a work executor in
`app_config` (an option every native program would have to fill for a once-per-room cost).

### D6. Room keys, their cache and their failure

A room without a password keys AES with SHA-256 of `["trystero:room-aes:v3",app,room]`; with one,
the app derives a PBKDF2 master (salt SHA-256 of `["trystero:pbkdf2-master:v3",app,room]`), then
HKDF to the AES key and the HMAC key, wipes the master, and keeps at most 64 rooms' keys (least
recently used out), as 0.27.0's `derivedRoomKeysCache`; a failed derivation is forgotten. The
password stays per app (0.1.0's ruling: one password per app), so the cache is keyed by room id.
A room announces nothing before its key exists. A failed derivation is reported as
`error::backend_failure`, since 0.27.0 has no room error code for it.

### D7. Dependencies and allocation

`parts` links webcpp::hash and webcpp::crypto (`crypto`, `gcm-default`, `secp256k1-default`);
libsecp256k1 arrives through webcpp::crypto, with the same `webcpp-secp256k1-tables` feature.
OpenSSL stays natively for TLS and libdatachannel, but trystero's own code no longer calls it.
What webcpp::crypto's secp256k1 context or webcpp::hash's WebCrypto bridge allocates outside a
room's resource is measured by the allocation tests and listed in the allocation page.

### D8. Phase 1 runs on a provisional identity

Before P-256, a room's peer id is 20 base62 characters drawn at random and its user id equals it.
The values have 0.27.0's form, the rooms meet each other over the memory backend and local
relays, and nothing claims interoperability until Phase 2.

### D9. One connection per room and remote peer (default; confirmed by Task 2.1)

0.27.0 removed 0.26.0's `shared-peer.ts`: each room negotiates its own peer connection, and the
handshake binds app and room. The default ruling is that the port follows: the app keeps one
identity, its relays and its key cache, and each room its own connections; the archived
`shared-connection` change's sharing of connections across rooms, its presence frames and
`room_token` are retired. Phase 2's first task reads 0.27.0's peer and transport code and records
a room's data-channel frames; if 0.27.0 shares after all, this decision is amended before any
code is removed.

### D10. Listeners return a subscription

`on_*` adds a listener and returns a move-only `subscription` whose `unsubscribe()` removes it;
destroying the token leaves the listener, as dropping the function Trystero returns does (a
RAII token would silently remove listeners whose token a program did not keep). `listen_options`
has `once` and `replay`; Trystero's `signal` (an `AbortSignal`) has no C++ counterpart and
`unsubscribe` replaces it. `on_request` and `on_receive` take one listener at a time and refuse a
second with `error::already_subscribed` (Trystero throws).

### D11. self(), users() and peers()

`self()` is synchronous: an app's identity exists once it is constructed (Trystero's `getSelf()`
is a promise because WebCrypto's key generation is). `peers()` has no `connection` member: the
backend's peer connection is not public in C++.

### D12. An opened text must be UTF-8

Trystero decodes a plaintext with `TextDecoder`, which replaces invalid UTF-8 by U+FFFD. The port
refuses such a plaintext as `decrypt_failed`: an SDP is ASCII and a replaced text would be another
text than the peer sealed. Listed as a difference.

### D13. Relay connectivity belongs to the app

Trystero's `setRelayConnectivity` is process-wide; a C++ process may hold several independent
apps, so it is `app::set_relay_connectivity`. `automatic` follows the page's `online` and
`offline` events on Emscripten and is `online` natively, where no such signal exists.

### D14. The differences page

Removed: `diff_already_joined` (0.27.0 now refuses a second join too), `diff_self_id` and
`diff_self_id_not_utf8` (no custom self id in 0.27.0), `diff_peer_join` if its listener model is
now the same. Added: D10's token and `signal`, D11's synchronous `self()` and missing
`connection`, D12, D13, D15, and the password per app (kept from 0.1.0). Each added difference
gets its program and its twin as today.

### D15. No implicit identity persistence

0.27.0's default `identity: 'local'` keeps the user key in IndexedDB. C++ has no such store: the
default is an ephemeral identity, and a program keeps one across runs with
`export_user_identity` and `import_user_identity` (Phase 2), storing the text where it wants.

### D16. Where 0.27.0's features land

Phase 1: everything without P-256 (this plan's Tasks 1 to 13). Phase 2: identity, the handshake,
verified users, `max_users`, interop. Phase 3: `make_state` and `make_user_state` (core in
0.27.0, but signed records need P-256 and the shared-state machinery is large), `roles()`, then
`scale()`. Rough size: Phase 1 thirteen tasks, some 6,000 to 8,000 lines changed with tests;
Phase 2 seven tasks, some 5,000 to 7,000; Phase 3 three changes, some 10,000 to 14,000.

## Risks / Trade-offs

- [P-256 slips] Phase 2 and the merge wait; Phase 1 stays on its branch and is rebased onto
  `main` as `main` moves.
- [0.27.x patch releases change the wire again] the pin is `8d3f57e`; a later patch is a
  follow-up change, its vectors regenerated by the recorder.
- [WebCrypto's PBKDF2 under node] Emscripten tests run under node, which has WebCrypto; the
  memory backend's synchronous PBKDF2 keeps unit tests independent of it.
- [The retired shared connection] D9 removes 0.1.0 work; it is confirmed by reading 0.27.0 first.

## Migration Plan

Programs replace setters by `on_*` calls, read `self()` instead of `self_id()`, drop
`self_id` and `handshake_timeout` from their configuration, and handle the new error codes. The
README and the pages carry a migration section modelled on Trystero's `MIGRATION.md`.

## Open Questions

1. Does webcpp::crypto's `random_device` on Emscripten keep a JavaScript exception out of
   WebAssembly when the page has no `crypto` (0.1.0's entropy caught it)? Default: check in
   Task 6 and keep a guard of trystero's if it does not.
2. Is the action wire's request/response protocol carried in metadata or in frames of its own?
   Task 8's recorded frames answer it.
