# Tasks

`reference/plan.md` is the plan: Phase 1 in full (its Tasks 1 to 13), Phases 2 and 3 outlined.
Nothing is implemented yet. The library branch is `port/trystero-0.27`, the superproject's
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

## 3. Phase 2: identity and the handshake (after webcpp::crypto's P-256 merges)

- [ ] 3.1 The connection model confirmed or amended (design D9) (Task 2.1)
- [ ] 3.2 Endpoint and user keys, import and export (Task 2.2)
- [ ] 3.3 Signed key records, low S, the signature cache (Task 2.3)
- [ ] 3.4 The dual-signature handshake (Task 2.4)
- [ ] 3.5 Verified users, `max_users` (Task 2.5)

## 4. Phase 2: interop

- [ ] 4.1 The oracle and interop lanes on 0.27.0, the twins rewritten (Task 2.6)

## 5. The merge

- [ ] 5.1 The pages whole, the changelog, chapter 15, one merge of each branch, both CIs green (Task 2.7)

## 6. Phase 3: plugins (separate changes after the merge)

- [ ] 6.1 Signed records and shared state (Task 3.1)
- [ ] 6.2 Roles and moderation (Task 3.2)
- [ ] 6.3 `scale()` (Task 3.3, after webcpp::crypto's `ecdh_x`)
