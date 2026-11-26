## ADDED Requirements

### Requirement: Port: Trystero pinned at 0.27.0 and proven by its own code
The library SHALL port Trystero 0.27.0 (`dmotz/trystero` commit `8d3f57e`), Nostr strategy only. Every value a peer exchanges MUST equal what 0.27.0's own code computes for the same inputs, as recorded in `test/fixtures/expected/trystero-0.27.0.json` by `test/oracle/record_0_27.mjs`, which imports 0.27.0's `packages/core/src` and `packages/nostr/src` unchanged (or, with `--dist`, the published `@trystero-p2p/core` and `@trystero-p2p/nostr` 0.27.0) and pins randomness only around the calls whose output it decides. WebCrypto's P-256 signatures being random, the recorder MUST keep each signature of its previous output that 0.27.0's verification still accepts, and sign only those missing.

#### Scenario: The fixture names its version
- **WHEN** the `crypto_layer` test reads the fixture
- **THEN** its `trystero` member is `0.27.0`

#### Scenario: The recorder reproduces the fixture
- **WHEN** `node test/oracle/record_0_27.mjs` runs against a checkout of the pinned commit, with the fixture as its previous output
- **THEN** its output equals the fixture byte for byte

#### Scenario: The published packages are the pinned source
- **WHEN** the oracle lane runs the recorder with `--dist` over its installed `@trystero-p2p/core` and `@trystero-p2p/nostr` 0.27.0, with the fixture as its previous output
- **THEN** its output equals the fixture byte for byte

### Requirement: Port: One branch, merged once
The port SHALL be developed on the library branch `port/trystero-0.27` and the superproject branch `trystero/port-0.27`, and MUST NOT reach either `main` before the interop lane against Trystero 0.27.0 JS peers and every CI check of both repositories are green on the branch. While the branch cannot interoperate, its oracle and interop lanes MUST be turned off by `explicit` targets that name the task that turns them back on.

#### Scenario: Phase 1 on the branch
- **WHEN** Phase 1 is complete
- **THEN** both branches are pushed with draft pull requests, and neither `main` holds any of it

### Requirement: Protocol: Strict base64url
`protocol::to_base64url` SHALL write RFC 4648's URL alphabet without padding, and `protocol::from_base64url` MUST accept a text only when it is nonempty, holds only `[A-Za-z0-9_-]`, has no padding, and re-encoding its bytes gives the same text, as Trystero's `fromBase64Url` followed by its re-encoding check accepts it.

#### Scenario: Canonical and non-canonical texts
- **WHEN** `from_base64url` reads `AQID`, `AR`, `AB==` and `A B`
- **THEN** the first gives `01 02 03` and the other three are refused

### Requirement: Protocol: Topics of 0.27.0
`protocol::topic_path(parts)` SHALL write the parts as `JSON.stringify` writes an array of strings, `protocol::hash_topic(text)` MUST be the base64url SHA-256 of the text's UTF-8 bytes, a room's root topic MUST be `hash_topic(topic_path({"Trystero", app_id, room_id}))` and a peer's topic `hash_topic(topic_path({<the root's path>, peer_id}))`.

#### Scenario: The lobby of example.com/app
- **WHEN** the root topic of app `example.com/app` and room `lobby` is computed
- **THEN** its path is `["Trystero","example.com/app","lobby"]` and the topic is `Ez_9ACs-onxWTTRRn7pFD2rqRce5y0HrAay9k2X7Ip0`, and the peer topic for `T5sdTwb84yyey80e4qjv` is `j0YMUkrR_ecDTTcdbXvJV9GW8StO2JaSN1Do2CyT75w`

### Requirement: Protocol: hash32, strToNum and topic kinds
`protocol::hash32` SHALL be FNV-1a 32 over UTF-8 bytes followed by MurmurHash3's fmix32, `protocol::str_to_num(text, limit)` MUST be `hash32(text) % limit`, and `protocol::topic_kind(topic)` MUST be FNV-1a 32 over the topic's UTF-16 code units, modulo 10000, plus 20000, without fmix.

#### Scenario: lobby
- **WHEN** `hash32("lobby")` and `topic_kind("lobby")` are computed
- **THEN** they are 1937637236 and 24083

### Requirement: Protocol: Relays of 0.27.0 in their seeded order
`protocol::default_nostr_relays` SHALL hold 0.27.0's 30 default relays in its order, and `derive_relays` MUST shuffle them with 0.27.0's seeded shuffle (the 32-bit generator `state += 0x6d2b79f5` and its two mixing steps, a Fisher-Yates from the end) seeded with `str_to_num(app_id)`, and take the first `relay_redundancy`.

#### Scenario: example.com/app
- **WHEN** the indexes 0 to 29 are shuffled with `str_to_num("example.com/app")` = 3213600185
- **THEN** the order begins `24, 4, 9, 29, 12` and equals the fixture's whole order

### Requirement: Protocol: Base62 ids, UUIDs and derived ids
`protocol::gen_id(n, draw)` SHALL draw `ceil(n * 1.15) + 4` bytes a round and append `base62_alphabet[b % 62]` for each byte below 248, skipping the others, until `n` characters, with the alphabet `0-9A-Za-z`; peer ids are 20, user ids 22, offer ids 12, nonces 8 and subscription ids 64 characters. `uuid_v4` MUST format 16 random bytes as RFC 9562's version 4. `derive_peer_id(compact)` MUST be the low 20 base-62 digits of SHA-256 of `trystero:pid:v1:<compact>` and `derive_user_id` the low 22 of `trystero:uid:v1:<compact>`, where a compact key is `base64url(x).base64url(y)` of an uncompressed P-256 point.

#### Scenario: Bytes 248 and above are skipped
- **WHEN** `gen_id(8, draw)` draws the cycle `00 01 3d 3e 7b f7 f8 f9 ff 0a ...`
- **THEN** the id is `01z0zzAE`

#### Scenario: The ids of a compact key
- **WHEN** the fixture's compact key is derived
- **THEN** the peer id is `T5sdTwb84yyey80e4qjv` and the user id `2Ag9nxCt2tqpTCQ2x1bNyf`

### Requirement: Protocol: Room keys and room authentication
Without a password a room's AES key SHALL be SHA-256 of `JSON.stringify(["trystero:room-aes:v3", app_id, room_id])`. With one, the master MUST be PBKDF2-HMAC-SHA-256 of the password's UTF-8 with salt SHA-256 of `JSON.stringify(["trystero:pbkdf2-master:v3", app_id, room_id])`, 600,000 iterations, 32 bytes, and the AES and HMAC keys HKDF-SHA-256 of the master with 32 zero bytes of salt and the infos `trystero:room-aes:v3` and `trystero:room-hmac:v3`. A room auth token MUST be the base64url HMAC-SHA-256 of `JSON.stringify(["trystero:ov-auth:v3", app_id, room_id, peer_id, seq])`, and verification MUST refuse a token that is not strict base64url of 32 bytes and compare MACs in constant time.

#### Scenario: The keys of example.com/app's lobby
- **WHEN** the room key without password and the keys for password `hunter2` are derived
- **THEN** they are `b03810047a4001c5c48b2a81106f3649447465c05eca736b56c02f94facf207a` and, for the AES key with the password, `4ae994c98ce84eb65c425d575f6041fd4b2d746e9aeadf55e8f5dec459b63f05`, as the fixture holds

#### Scenario: A token for another sequence number
- **WHEN** a token signed for `seq` is verified for `seq + 1`
- **THEN** it is refused

### Requirement: Protocol: Sealing as 0.27.0 seals
`protocol::seal(key, iv, plaintext, aad)` SHALL be AES-256-GCM with a 12-byte IV and the AAD, written as base64url of `iv || ciphertext || tag`, and `protocol::open` MUST refuse a text that is not strict base64url, shorter than 28 bytes, whose tag fails, or whose plaintext is not UTF-8. A signal's AAD is `<type>|<peer id>|<offer id>`. Each seal draws its IV from the backend's randomness.

#### Scenario: Every sealed vector
- **WHEN** each fixture entry is sealed with its key, IV and AAD, and opened again
- **THEN** the texts equal the fixture's and open to the plaintexts, and the fixture's refused texts are refused

### Requirement: Backend: The key derivation seam
`backend::factory` SHALL have no `sealer` and no `make_sealer`, and MUST offer `derive_password_master(password, salt, out, done)`, completing on the room's executor after the call returns, with `out` zeroed on error. The platform backend MUST run it off the room's thread: natively `webcpp::hash::async_pbkdf2` on a one-thread pool it owns, on Emscripten through WebCrypto (`backends::webcrypto`). Its randomness MUST be `webcpp::crypto::random_device`.

#### Scenario: A password room joins at once
- **WHEN** a room of an app with a password is joined over a memory backend that holds the derivation
- **THEN** `join` returns before the derivation completes, nothing is announced until it does, and two such peers then meet

### Requirement: Room: Room keys are cached per app
An app SHALL derive a password room's keys once per room id and keep at most 64 rooms' keys, least recently used out, wipe the PBKDF2 master after HKDF, and forget a failed derivation so that a later join derives again; a failed derivation MUST be reported as `error::backend_failure`.

#### Scenario: Rejoining a room
- **WHEN** a room is left and joined again within the app
- **THEN** no second derivation runs

### Requirement: Signaling: Nostr as 0.27.0 speaks it
The Nostr strategy SHALL sign with one BIP-340 key per room (webcpp::crypto's `curves::secp256k1`), made on the room's first publish; subscribe with `since` = `max(0, now - 60)` and the deduplicated kinds of its topics, at most 250 topics per subscription; add a fresh 8-character nonce to every announcement (`{"peerId":...,"nonce":...}`); retire a relay on a rejection starting `blocked:`, `restricted:`, `auth-required:` or `pow:`; cool a relay down on `rate-limited:` for 60 s doubling up to 15 minutes, then probe it, an `OK true` ending the probation; ignore a CLOSED of a subscription not its own; and replay its announcements when the last pending EOSE arrives 1 s or more after its REQ.

#### Scenario: A terminal rejection
- **WHEN** a relay answers an EVENT with `["OK", id, false, "blocked: no"]`
- **THEN** the relay is retired and closed, and the room goes on over its other relays

### Requirement: Actions: The action wire version 3
Action frames SHALL be byte-identical to 0.27.0's `action-wire.ts`: version byte 3, the kinds inline, offer, chunk, accepted, rejected and cancelled, the wide bulk flag 64 for chunks of up to 65,536 bytes, and its limits, as the fixture's recorded frames show.

#### Scenario: Recorded frames
- **WHEN** the port encodes each message the recorder sent
- **THEN** it writes the recorded frames and decodes them back to the messages

### Requirement: Room: Listeners return a subscription
`on_peer_join`, `on_peer_leave`, `on_user_join`, `on_user_leave`, `on_error` and an action's `on_message` and `on_receive_progress` SHALL add a listener and return a `subscription`; several listeners of one event MUST run in the order added; `listen_options::once` removes a listener before its first run and `replay` reports the current peers or users within the call; a listener added while an event is emitted MUST NOT run for it, and one removed before its turn MUST NOT run; destroying a `subscription` leaves its listener. `on_request` and `on_receive` MUST refuse a second listener while one is active with `error::already_subscribed`.

#### Scenario: A listener removes the next
- **WHEN** the first of two join listeners unsubscribes the second while it runs
- **THEN** the second does not run for that join

### Requirement: Room: 0.27.0's room errors
`room::on_error` SHALL report a `room_error` with a code of the `webcpp.trystero` category among `room_full`, `banned`, `relay_failed`, `identity_failed`, `plugin_failed`, `decrypt_failed`, `peer_connect_failed`, `handshake_failed` and `peer_error`, whose `room_error_code_name` is 0.27.0's (`room-full` and so on), with the app id, the room id, the peer id where 0.27.0 gives one, the stage of a `decrypt_failed`, and `turn_configured` for a `peer_connect_failed`. A `decrypt_failed` MUST be reported at most once per peer and stage per 60 s.

#### Scenario: Another password
- **WHEN** two apps with different passwords join one room and their offers keep arriving
- **THEN** each reports `decrypt_failed` once a minute for the other, and neither joins

### Requirement: Room: Users and peers
A room SHALL keep its users and their peers: `on_user_join` fires after the `on_peer_join` of a user's first peer, `on_user_leave` after the `on_peer_leave` of its last; `users()` and `peers()` list them (a peer with its user id, `direct`, `hops` and roles; no connection); `self()` gives the peer id, user id, persistence, room id and base room id; an action's send MUST take a user id (all its peers), a peer id or a list, and MUST complete with `error::not_connected` for a target not connected, while a broadcast to an empty room succeeds.

#### Scenario: A user with two peers
- **WHEN** two peers of one user are in a room and one leaves
- **THEN** `on_peer_leave` fires and `on_user_leave` does not, until the second leaves too

### Requirement: App: Relay connectivity
`app::set_relay_connectivity` SHALL take `online`, `offline` or `automatic`: `offline` closes the app's relay sockets and holds them closed, `online` reconnects them, and `automatic` follows the page's `online` and `offline` events on Emscripten and is `online` natively.

#### Scenario: Offline and back
- **WHEN** an app goes offline, then online
- **THEN** its relays close and no reconnection is tried until it is online, when they reconnect and resubscribe

### Requirement: App: Options 0.27.0 removed are gone
`app_config::self_id` and `room_options::handshake_timeout` SHALL NOT exist, and joining a room id already active in the app MUST fail with `error::already_joined`, as 0.27.0 throws.

#### Scenario: A second join
- **WHEN** an app joins `lobby` twice without leaving
- **THEN** the second join fails with `already_joined`

### Requirement: Room: One connection per room and remote peer (Phase 2)
Each room SHALL negotiate its own connection with each remote peer, in the room's resource, and write its action frames bare on the data channel, as 0.27.0 does (design D9): no connection is shared by two rooms, and there are no presence frames, room tokens, rebinding, idle time of a connection or bound on the rooms of an app. A room MUST keep one connection per peer, closing a second one while the first is live. Before the room takes a connection's frames it MUST keep at most 64 frames of at most 17,408 bytes for at most 10 s, and after it at most 65,536 bytes a frame, ending the connection otherwise.

#### Scenario: Two rooms of one app
- **WHEN** apps A and B both join rooms `r1` and `r2`, and A leaves `r1`
- **THEN** A and B made two connections, and `r2`'s keeps working with no departure reported in it

#### Scenario: Many rooms
- **WHEN** an app joins 65 rooms
- **THEN** every join succeeds

### Requirement: Identity: Endpoint and user keys (Phase 2)
Each app SHALL hold an endpoint key pair and a user identity on ECDSA P-256 (webcpp::crypto's `curves::p256`, BearSSL), its peer id derived from the endpoint key's compact form and its user id from the user key's (design D17). The user identity MUST be `app_config::identity` when given (`identity_persistence::imported`, 0.27.0's `'custom'`) and an ephemeral one made with the app otherwise; `create_user_identity`, `export_user_identity` (0.27.0's `trystero_uk_` text) and `import_user_identity` (that text or its JSON key, trimmed, checked as 0.27.0's `importUserIdentity` checks it, refused with `error::invalid_user_identity`) let a program keep a user across apps and runs. A key that cannot be made MUST make every room of the app report `identity_failed` and end without announcing. Signatures MUST be normalized to low S before they are sent and refused when their S is high, and verified keys and signatures are cached per app within 0.27.0's bounds (4,096 key records; 8,192 signatures, 2 MiB, payloads of at most 4,096 bytes). The user-proof payload is ported as protocol code, which 0.27.0's core never sends (design D21).

#### Scenario: Two apps of one user
- **WHEN** two apps import one user identity and join one room with a third peer
- **THEN** the third peer sees two peers and one user

#### Scenario: The exported text
- **WHEN** the fixture's `user-export` key is imported and exported again
- **THEN** the text equals the fixture's, and every refused import of `user_identity_imports` is refused

### Requirement: Identity: The dual-signature handshake (Phase 2)
A room SHALL admit a peer only after 0.27.0's handshake over `@_hsdata` and `@_hsready`: the lower peer id initiates; each side claims its UUID nonce, its DTLS fingerprints (read from the descriptions the room's negotiation set), its user id and both compact keys; both sides sign the transcript `["trystero:id-hs:v2", app, room, initiator, responder, nonces, both fingerprint lists, both user ids]` with the endpoint and the user key, and each verifies the other's ids against its keys and both signatures. Any failure, and a handshake not complete after 10 s or with more than 16 messages pending, MUST be reported as `handshake_failed` with 0.27.0's reason and the connection closed. `max_users` MUST refuse a user beyond the limit with `room_full`, counting verified users (the room's own included), not peers, and admitting another peer of a user already present.

#### Scenario: A forged user signature
- **WHEN** a peer presents a user id whose signature does not verify
- **THEN** the room reports `handshake_failed` and the peer never joins

#### Scenario: A man in the middle with the password
- **WHEN** a peer's step claims DTLS fingerprints other than those of the description the room applied
- **THEN** the room reports `handshake_failed` with "DTLS fingerprint mismatch"

#### Scenario: A full room
- **WHEN** a room with `max_users` 2 holds two users and a third user's peer completes its handshake
- **THEN** that peer gets `{"ok":false,"err":"room full"}` and reports `room_full`, and the room reports no join

### Requirement: Room: The emitters and internal actions of Phase 2
A room SHALL report `peer_error` when the action wire drops a peer for a protocol violation, `banned` when a peer answers its handshake with `{"ok":false,"err":"peer banned"}` (with the reason of at most 512 units), and `relay_failed` once when no relay's first announcement succeeded and every relay failed to boot or to announce; `plugin_failed` waits for Phase 3. A ping MUST be the internal request action `@_ping`, answered through `@_response`, and an `@_response` offer that answers no request MUST be rejected with "unexpected response". A transfer MUST be sent in 64 KiB chunks to a peer whose connection takes 64 KiB messages (`peer_connection::max_message_size`).

#### Scenario: Recorded frames
- **WHEN** the port pings, sends a ready frame, a room-full frame and a ban frame
- **THEN** its frames equal the fixture's `ping_frames` and `handshake_frames`

### Requirement: Tests: Interop with Trystero 0.27.0 (Phase 2)
The oracle lane SHALL pin Trystero 0.27.0 (its packages `trystero`, `@trystero-p2p/core` and `@trystero-p2p/nostr` 0.27.0, werift as their WebRTC in Node) and the interop lane MUST meet Trystero 0.27.0 JS peers in headless Chrome over a local relay, on both lanes (the browser's C++ peer and the native one), before the branch merges: ids, messages, wide transfers, passwords, imported identities, one user with two C++ peers, `max_users` both ways, requests and pings, leaves, a forged user signature, a mesh and relay connectivity.

#### Scenario: A C++ peer and a JS peer
- **WHEN** a native peer and a Chrome page running Trystero 0.27.0 join one room
- **THEN** each sees the other's user and peer ids, each having verified the other's signatures, and they exchange messages both ways

### Requirement: Plugins: State, roles and scale (Phase 3)
After the port merges, separate changes SHALL add signed shared and per-user state (`make_state`, `make_user_state`), roles and moderation (`roles` with admins, role certificates, ban), and `scale()` with its pairwise ECDH keys, each against vectors and interop with 0.27.0.

#### Scenario: Phase 3 waits for the merge
- **WHEN** the port has not merged
- **THEN** no plugin is on the branch

## MODIFIED Requirements

### Requirement: App: The app is the unit of sharing
The library SHALL offer `class app` (`<webcpp/trystero/app.hpp>`, in `webcpp::trystero`): this program in one application and immutable password domain, with one endpoint key and peer id, one user identity, the relays' configuration, the derived room keys' cache, the identity caches and the registry of its rooms; each room negotiates its own connections (design D9). The app and every room of it MUST run on one executor, the app's; nothing is keyed by the backend or kept in static storage, and two `app` objects with the same app id in one process are two peers. `app` is non-copyable and non-copy-assignable, `noexcept` move-constructible and move-assignable, and its state is shared with its rooms: destroying the `app` while rooms remain changes nothing for them.

#### Scenario: Two apps of one id
- **WHEN** a process makes two apps with the same app id and both join one room
- **THEN** each sees the other as a peer, with two peer ids

### Requirement: App: app_config members and defaults
The library SHALL define `struct app_config` with these members and defaults, in this order: `std::pmr::string app_id` (empty), `std::pmr::string password` (empty: a passwordless app; immutable for this app and shared by every room), `std::pmr::vector<std::pmr::string> relay_urls` (empty: Trystero's defaults derived from the app id), `std::size_t relay_redundancy = 5`, `std::pmr::vector<backend::ice_server> ice_servers` (empty: `default_ice_servers()`), `std::optional<user_identity> identity` (empty: an ephemeral user identity made with the app), `detail::logger::sink log` (the sentences of the app and of every room, composed in the app's resource; empty means no text is composed) and `std::pmr::memory_resource* memory = std::pmr::get_default_resource()`. It is an aggregate built with designated initializers and move-only because its `log` is a move-only `handler`. The app copies every part of it into `memory`. Different passwords MUST use separate app objects.

#### Scenario: A configuration in place
- **WHEN** a program writes `trystero::app_config c{.app_id = "webcpp-trystero-tests"};` and passes it moved to an `app`
- **THEN** `app_id()` is `webcpp-trystero-tests`, `self_id()` is 20 base62 characters derived from the endpoint key, `relay_urls()` lists five derived relays, and every other member has the default above

### Requirement: App: room_options members and defaults
The library SHALL define `struct room_options` with the members a room adds to its app's configuration, in this order: `std::size_t max_receive_bytes = std::size_t{256} * 1024 * 1024` (Trystero's `maxReceiveBytes`, across peers), `std::uint64_t max_json_document_bytes` (webcpp::document's default), `std::optional<std::size_t> max_users` (empty: unbounded; 0 or above 2^53 - 1 refused by `join` with `error::invalid_max_users`) and `std::pmr::memory_resource* memory = nullptr` (null: the app's resource). It is an aggregate built with designated initializers. It MUST NOT contain a password or allow a per-room password override; every room reads its app's immutable password.

#### Scenario: One app password for every room
- **WHEN** a program constructs an app with `{.app_id = "a", .password = "p"}` and joins `lobby` and `elsewhere` with default room options
- **THEN** both rooms use password `p`, no user bound and a receive bound of 256 MiB, and a room option naming `password` does not compile

### Requirement: App: One peer id per app
The app SHALL announce, signal, tie-break and handshake in every room of the app with its one `self_id`, derived from its endpoint key (`derive_peer_id` of the key's compact form), so that a peer meeting the app in several rooms sees one id, as 0.27.0's endpoint identity per app does. `room::self()` reports it with the user id. Rooms that must stay unlinkable on the relays and to peers are joined through separate `app` objects, which have two endpoint keys and nothing shared.

#### Scenario: The id is the key's
- **WHEN** an app is made with a known endpoint scalar (the test seam)
- **THEN** `self_id()` is `derive_peer_id` of that key's compact form

## REMOVED Requirements

### Requirement: Protocol: Id alphabet and lengths
**Reason**: 0.27.0 draws ids from `0-9A-Za-z` without bias and has user ids.
**Migration**: "Protocol: Base62 ids, UUIDs and derived ids".

### Requirement: Protocol: Topic names
**Reason**: 0.27.0's topics are base64url SHA-256, not SHA-1 in base 36.
**Migration**: "Protocol: Topics of 0.27.0".

### Requirement: Protocol: Root and peer topics
**Reason**: the topic texts are JSON paths in 0.27.0.
**Migration**: "Protocol: Topics of 0.27.0".

### Requirement: Protocol: Seeded shuffle
**Reason**: 0.27.0 replaced the sine generator by a 32-bit integer generator.
**Migration**: "Protocol: Relays of 0.27.0 in their seeded order".

### Requirement: Protocol: The seal key and the handshake proof
**Reason**: 0.27.0 derives room keys by SHA-256 of a domain-tagged path or PBKDF2 and HKDF, and its handshake is the dual-signature one.
**Migration**: "Protocol: Room keys and room authentication", "Identity: The dual-signature handshake (Phase 2)".

### Requirement: Protocol: The text form of a sealed signal
**Reason**: 0.27.0 seals with a 12-byte IV and AAD and packs base64url(iv || ct || tag).
**Migration**: "Protocol: Sealing as 0.27.0 seals".

### Requirement: Protocol: The BIP-340 signer
**Reason**: the signer is webcpp::crypto's, one per room.
**Migration**: "Signaling: Nostr as 0.27.0 speaks it".

### Requirement: Backend: sealer
**Reason**: sealing is synchronous protocol code over webcpp::crypto.
**Migration**: "Backend: The key derivation seam".

### Requirement: Native: The sealer is AES-256-GCM in Trystero's sealed text form
**Reason**: the backend has no sealer.
**Migration**: "Protocol: Sealing as 0.27.0 seals".

### Requirement: Browser: The sealer is crypto.subtle's AES-GCM under a key imported once
**Reason**: the backend has no sealer; WebCrypto is used only for PBKDF2.
**Migration**: "Backend: The key derivation seam".

### Requirement: Room: Self id
**Reason**: 0.27.0 removed custom peer ids; ids derive from keys.
**Migration**: "Room: Users and peers", "Identity: Endpoint and user keys (Phase 2)".

### Requirement: Oracle: Trystero pinned at 0.26.0
**Reason**: the port follows 0.27.0.
**Migration**: "Port: Trystero pinned at 0.27.0 and proven by its own code".

### Requirement: App: At most 64 rooms
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: App: The idle time of a connection
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Ownership of a connection
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: One connection per remote peer per app
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Adoption of a negotiated connection
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Crossed connections recover
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Presence frames
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Routing by room token
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Frames kept for a room not bound yet
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: The drain order
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: A binding's end is its room's alone
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: A connection's end
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: A connection with no binding left
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Relays and configuration stay per room, configured once
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Failure scopes
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Shared: Bounds
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Binding: The three paths and the conditions
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Binding: The steps of a binding
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Binding: Signaling reuse on the first meeting
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Binding: Unbinding and the announcements that follow
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: The schedule after a kick
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: Only the lower id refuses
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: The initiator accepts any of its last 8 challenges
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: A transient failure converges
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: Correcting room proofs and changing password domains
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Rebinding: Reports are bounded
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Security: A passwordless room is open to the app's other rooms
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Security: Password domains preserve the upstream wire
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Security: A forged announcement binds nothing
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Security: What sharing reveals and every existing protection holds
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Protocol: Room token
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Tests: The shared connection and the app are tested over the memory backend
**Reason**: 0.27.0 shares no connection between rooms (design D9).
**Migration**: "Room: One connection per room and remote peer (Phase 2)".

### Requirement: Room: The handshake
**Reason**: 0.27.0 dropped the password handshake; its handshake is the dual-signature one.
**Migration**: "Identity: The dual-signature handshake (Phase 2)".

### Requirement: Room: A peer with another password never joins
**Reason**: another password is refused by the sealed signals, as `decrypt_failed`; `wrong_password` is gone (design D23).
**Migration**: "Room: 0.27.0's room errors".

### Requirement: Room: Handshake timeout
**Reason**: 0.27.0's handshake times out after a fixed 10 s, reported as `handshake_failed`.
**Migration**: "Identity: The dual-signature handshake (Phase 2)".

### Requirement: Errors: handshake_failed is reserved
**Reason**: `handshake_failed` has its emitter, and the values after the removed errors move (design D23).
**Migration**: "Room: 0.27.0's room errors", "Identity: The dual-signature handshake (Phase 2)".

### Requirement: Tests: Interop with Trystero JS on both lanes
**Reason**: its scenarios are 0.26.0's sharing ones.
**Migration**: "Tests: Interop with Trystero 0.27.0 (Phase 2)".
