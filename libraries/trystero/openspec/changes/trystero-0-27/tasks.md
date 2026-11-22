# Tasks

`reference/plan.md` is the plan: Phase 1 in full (its Tasks 1 to 13, built), Phase 2 in full
(its Tasks 2.1a to 2.7), Phase 3 outlined. The library branch is `port/trystero-0.27`, the superproject's
`trystero/port-0.27`; neither merges before group 5 (design D1).

## 1. Phase 1: the crypto layer (plan Tasks 1 to 6)

- [ ] 1.1 Branches, webcpp::hash and webcpp::crypto linked, the vectors and the `crypto_layer` test (Task 1)
- [ ] 1.2 Strict base64url and 0.27.0's topics, `hash32`, `str_to_num` (Task 2)
- [ ] 1.3 Relays, base62 ids, UUID v4, compact keys and derived ids (Task 3)
- [ ] 1.4 Room keys and room authentication (Task 4)
- [ ] 1.5 Sealing and opening, the signal AAD (Task 5)
- [ ] 1.6 The key derivation seam, the key cache, the backend without a sealer (Task 6)

## 2. Phase 1: the wire and the surface (plan Tasks 7 to 13)

- [ ] 2.1 Nostr as 0.27.0 speaks it (Task 7)
- [ ] 2.2 The action wire version 3 (Task 8)
- [ ] 2.3 Listeners returning a subscription (Task 9)
- [ ] 2.4 Room errors (Task 10)
- [ ] 2.5 Users and peers, `self()`, targeted sends (Task 11)
- [ ] 2.6 Options, joins and relay connectivity (Task 12)
- [ ] 2.7 Phase 1's pages, its gate, the draft pull requests (Task 13)

## 3. Phase 2: connections, identity and the handshake (webcpp::crypto's P-256 is on main)

- [ ] 3.1 One connection per room and remote peer, design D9 confirmed (Task 2.1a)
- [ ] 3.2 The handshake's frame without the password challenge; the signer's `keypair::generate` (Task 2.1b)
- [ ] 3.3 Wide bulk chunks sent (Task 2.1c)
- [ ] 3.4 Endpoint and user keys, import and export, `identity_failed` (Task 2.2)
- [ ] 3.5 Key records, low S, the verification caches (Task 2.3)
- [ ] 3.6 The dual-signature handshake's messages and one side's run (Task 2.4a)
- [ ] 3.7 The handshake in the room, verified users (Task 2.4b)
- [ ] 3.8 `max_users` and `room_full` (Task 2.5a)
- [ ] 3.9 `peer_error`, `banned`, `relay_failed`'s announcement path, `@_ping` as a request (Task 2.5b)

## 4. Phase 2: interop

- [ ] 4.1 The oracle on 0.27.0, the recorder over the published packages, the twins (Task 2.6a)
- [ ] 4.2 Interop with Trystero 0.27.0 on both lanes, the lanes on (Task 2.6b)

## 5. The merge

- [ ] 5.1 The pages whole, the changelog's lines, chapter 15, one merge of each branch, both CIs green (Task 2.7)

## 6. Phase 3: plugins (separate changes after the merge)

- [ ] 6.1 Signed records and shared state (Task 3.1)
- [ ] 6.2 Roles and moderation (Task 3.2)
- [ ] 6.3 `scale()` (Task 3.3, after webcpp::crypto's `ecdh_x`)
