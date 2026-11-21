## Why

Trystero 0.27.0 (2026-11-11) is a major release with a new wire protocol that is not compatible
with 0.26.x peers: topics, ids, relay order, Nostr kinds, sealing and the action wire all changed,
and every room now runs a dual-signature handshake over ECDSA P-256 that binds unforgeable user
and endpoint identities to the WebRTC connection. webcpp::trystero ports 0.26.0, so a C++ peer no
longer meets a current Trystero JS peer. The owner asked on 2026-11-20 for the port to 0.27.0, and
for it to rest on webcpp::crypto and webcpp::hash; that supersedes his decision of 2026-11-17
(webcpp::crypto's genesis, design D23) that trystero would not move onto webcpp::crypto.

## What Changes

- **BREAKING (wire):** every value a peer exchanges follows 0.27.0: topics are the base64url
  SHA-256 of a JSON path (`["Trystero",app,room]`), ids are unbiased base62 (peer ids 20 and user
  ids 22 characters), relays are 0.27.0's 30 defaults in a new seeded order, Nostr kinds are
  FNV-1a, a REQ asks `since` a minute ago, announcements carry a nonce, signals are sealed with
  AES-256-GCM under a 12-byte IV with the signal's AAD and packed as base64url(iv || ct || tag),
  a password derives the room's keys by PBKDF2-SHA-256 (600,000 iterations) then HKDF, the action
  wire is version 3.
- **BREAKING (wire, Phase 2):** identity and the handshake: an endpoint key and a user key
  (ECDSA P-256) per peer, `peerId` and `userId` derived from them, a dual-signature handshake
  binding both SDP fingerprints; import and export of a user identity.
- **BREAKING (API):** listeners are `on_*` calls returning a `subscription` (several per event,
  `once`, `replay`); `on_user_join`/`on_user_leave` beside `on_peer_join`/`on_peer_leave`;
  `on_error` reports 0.27.0's room error codes (`room-full`, `banned`, `relay-failed`,
  `identity-failed`, `plugin-failed`, `decrypt-failed`, `peer-connect-failed`,
  `handshake-failed`, `peer-error`) with their context; `self()`, `users()`, `peers()`;
  sends target a user or a peer and fail on an absent target; `set_relay_connectivity`;
  `handshake_timeout` and a custom self id removed.
- **Dependencies:** trystero's sealer, randomness and Nostr signer move onto webcpp::crypto
  (`aes256_gcm`, `random_device`, `curves::secp256k1`) and its hashing onto webcpp::hash
  (SHA-256, HMAC, HKDF, PBKDF2; WebCrypto's PBKDF2 on Emscripten); the backend's `sealer` and
  the EM_ASM WebCrypto and OpenSSL code of trystero's own are removed; Phase 2 needs
  webcpp::crypto's P-256 (its foundation increment 3).
- **Plugins (Phase 3):** signed shared and per-user state, roles and moderation, then `scale()`.
- **Scope:** the Nostr strategy only, as today; media stays unported.
- **One merge:** the port lives on the branch `port/trystero-0.27` and reaches `main` once, when
  interop with Trystero 0.27.0 JS peers is green.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `trystero`: ADDED requirements for 0.27.0's crypto layer, topics, ids, relays, Nostr details,
  action wire v3, listeners, room errors, users and peers, relay connectivity, the key
  derivation seam, the dependencies, the vectors, the branch and the Phase 2 and 3 features;
  REMOVED requirements for 0.26.0's ids, topics, shuffle, sealing, sealer backends, signer,
  self id and oracle pin.

## Impact

- **Kind:** port, of Trystero 0.27.0 (`port_of`), from 0.26.0.
- **Implementation:** none yet; `reference/plan.md` is the plan (Phase 1 in full, Phases 2 and 3
  outlined), `reference/vectors/` the recorder and the vectors it recorded from 0.27.0's code.
  Library branch `port/trystero-0.27`, superproject branch `trystero/port-0.27`.
- **Dependencies:** webcpp::hash and webcpp::crypto (new); libsecp256k1 through webcpp::crypto
  instead of directly; Phase 2 waits on webcpp::crypto's P-256 (foundation increment 3, Tasks
  3.1 to 3.3 of its plan).
- **Status:** draft; release none.
- **Other proposals:** supersedes the part of webcpp::crypto's genesis D23 that kept trystero off
  webcpp::crypto; `bounded-json-parse` (draft) applies to the same parser and is rebased on this
  branch if it is accepted first; the archived `shared-connection`'s app-level sharing is retired
  by design D9 unless Phase 2's Task 2.1 finds 0.27.0 shares connections.
