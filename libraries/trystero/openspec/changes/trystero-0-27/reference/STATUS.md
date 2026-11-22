# webcpp::trystero, port to Trystero 0.27.0: as built after Phase 1

Written 2026-11-22. Phase 1 (plan Tasks 1 to 13) is built on its branches, not interoperable
before Phase 2, and merged nowhere (design D1). Phase 2 waits for webcpp::crypto's P-256
(foundation increment 3, its plan's Tasks 3.1 to 3.3). `plan.md` is the plan, `vectors/record.mjs`
the recorder and `vectors/phase1.json` what it recorded.

## Where it stands

| What | Where |
| --- | --- |
| The library | `webcpporg/trystero`, branch `port/trystero-0.27`, from `main` `0f7e6f4` |
| The superproject | `webcpporg/webcpp`, branch `trystero/port-0.27`, from `main` `4c5cfcc`: chapter 15's trystero section and the library's pointer |
| The vectors | `vectors/phase1.json`, recorded 2026-11-20 with node v26.13.0 from Trystero 0.27.0 at `8d3f57e`, with the action frames since Task 8 (`49ecd70` here); the library's copy is `test/fixtures/expected/trystero-0.27.0.json`, written only by `test/oracle/record_0_27.mjs` |
| The lanes | the oracle and interop lanes are `explicit` on the branch (their Trystero is 0.26.0); every other test and example passes natively and on emscripten |
| Blocking Phase 2 | webcpp::crypto's `curves::p256` |

## Phase 1, task by task (library commits)

| Task | Commits | What |
| --- | --- | --- |
| 1 | `0f7e6f4..5b3087d` | the build moves onto webcpp::hash and webcpp::crypto; the fixture and its `section()` |
| 2 | `5b3087d..07c19ee` | topics, `hash32`, `str_to_num`, strict base64url, domain tags; one JSON string writer |
| 3 | `07c19ee..93fc181` | base62 ids, `gen_id`, UUIDs, derived ids; relays by `str_to_num` |
| 4 | `93fc181..1a8d3a6` | room keys, PBKDF2 master, HKDF, room auth tokens |
| 5 | `1a8d3a6..6578725` | sealing as 0.27.0 seals, with the signal's AAD; the whole error list (R2) |
| 6 | `6578725..38d82e4` | no sealer: the key derivation seam, PBKDF2 off the room's thread, the keys cache |
| 7 | `38d82e4..fbc66a1` | 0.27.0's Nostr: a signer per room over webcpp::crypto, kinds by FNV-1a, subscriptions since a minute ago, relay health |
| 8 | `fbc66a1..38d087f` | the action wire, version 3, its limits and the wide flag (received only) |
| 9 | `38d087f..487c99e` | listeners and subscriptions, receive gates, progress, request actions |
| 10 | `487c99e..ea6f2d1` | 0.27.0's room errors, `room_error`, `decrypt_failed` paced per peer and stage |
| 11 | `ea6f2d1..b4b87e9` | users and peers, `self()`, sends to users, `not_connected` |
| 12 | `b4b87e9..5c7f578` | relay connectivity per app; `self_id` and `handshake_timeout` gone |
| 12b | `5c7f578..9741f9b` | trystero's own `detail::wipe` (R19) |
| 13 | `9741f9b..1994bf2` | the pages, the README and AGENTS.md say what Phase 1 changed; `diff_peer_join` retired |

## Rulings that changed the plan

- **R2.** Task 5 appended Task 10's whole error list (`room_full` to `request_timed_out`, with
  0.27.0's messages), so that numbering never changes; `action_redefined` (R15) is 29.
- **R11, amended.** `signer::generate` draws 32 bytes from `webcpp::crypto::random_device` and
  makes the key with `keypair::make`, at most 4 draws, completing with `entropy_unavailable`
  otherwise, as webcpp::crypto's coming secp256k1 `keypair::generate` does. **Phase 2's merge-main
  step switches `signer::generate` to crypto's `keypair::generate` (one line).**
- **R14.** Frames of an action the room has not made follow 0.27.0: inline frames are dropped and
  offers rejected at once as "unregistered action"; the 32-per-action-and-peer buffer for 10 s is
  only for actions made that do not listen yet.
- **R15.** A name made both a message and a request action fails with `error::action_redefined`,
  where 0.27.0 throws. A lone surrogate is refused in a peer's JSON metadata and payload, and read
  as U+FFFD in a refusal's reason that 0.27.0's 512-unit cut left ending in one.
- **R16.** The `peer_connect_failed` hint names the port's option, "configure TURN via
  app_config::ice_servers"; `turn_configured` counts only the TURN servers the app kept.
- **R17.** The test user mapping is a detail seam (no public `factory::user_of`); `users()` and
  `peers()` return a `std::pmr::map` with `std::less<>`, a transparent comparator.
- **R18.** `visibilitychange` is not followed in Phase 1 (design D13 names `online` and `offline`
  only); a `relay_connectivity` value outside the enum is taken as `online`, where 0.27.0 throws.
- **R19** (supersedes R10). trystero calls no `webcpp::crypto::detail`: its own `detail::wipe`
  (`detail/wipe.hpp`) wipes every variable-size secret buffer.
- **R20.** The doc reference's failure is a merge blocker owned by the hash stream; trystero
  gives its reference BearSSL now, and the draft PRs' docs check is expected red until hash's fix.

Each is a row of the differences page where it differs from 0.27.0.

## What Phase 2 inherits

Dependencies outside trystero:

- webcpp::crypto's `curves::p256` (its increment 3), for 2.2 to 2.4.
- webcpp::crypto's secp256k1 `keypair::generate` (max_draws 4, `entropy_unavailable`), for the
  R11 switch.
- A public wipe in webcpp::crypto (or hash) for variable-size buffers, which trystero then calls
  in place of its own `detail::wipe` (R19).
- A guard in webcpp::crypto's `random_device` on Emscripten against a missing or throwing
  `crypto.getRandomValues` (design Open Question 1): trystero 0.1.0's own guard went with
  `entropy.hpp`.
- **Merge blocker, owned by the hash stream (R20):** the reference of trystero's page
  (`b2 libs/trystero/doc`) does not build, so the CI's docs check of both draft PRs is red and
  the spec's "One branch, merged once" cannot be met until it does. The reference is read for
  Emscripten's target (`/webcpp//emscripten-reference`) with the native platform's dependencies;
  under `__MRDOCS__` webcpp::hash's `config.hpp` defines `WEBCPP_HASH_HAS_OPENSSL` (and BLAKE3,
  XKCP, sodium) unconditionally, so its own Emscripten `#error` fires and `<blake3.h>` is
  missing. The fix is the hash stream's: keep each `WEBCPP_HASH_HAS_<NAME>` under `__MRDOCS__`
  only when the build defines it, as webcpp::crypto does (or skip OpenSSL's on
  `WEBCPP_TARGET_EMSCRIPTEN`). trystero's half is done: its reference gets BearSSL, Emscripten's
  GCM default, rather than the native `gcm-default`'s OpenSSL, which webcpp::crypto refuses there
  (`<webcpp-trystero-reference>on` in `doc/Jamfile`, `rooms-gcm` in `build.jam`); with it the
  crypto errors are gone, trystero's doc comments pass `doc_comments.py`, and the only failure
  left is webcpp::hash's. Once the hash fix lands, the doc target is run again before the merge.

- **Merge blocker, owned by the crypto stream (R21):** the lint's clang-tidy fails on
  webcpp::crypto's `backends/secp256k1.hpp:66` (`bugprone-dynamic-static-initializers` on the
  secp256k1 canary) in every trystero translation unit that includes it, natively on the Linux
  lint and with Emscripten's flags on crypto's own `secp256k1_keys_test.cpp`. main's lint does not
  meet it only because no other library includes that header in a unit the lint analyses so. The
  fix is crypto's; trystero's own sources pass the whole lint.

Carries inside trystero:

- **2.1** confirms or amends design D9 before the shared connection, its presence frames, room
  tokens, the 64-room bound (`too_many_rooms`) and the password handshake `__trystero_pw` are
  removed; their rows move off the differences page.
- **2.4** gives `handshake_failed` its emitter and replaces the provisional private
  `room_impl::handshake_proof` (0.26.0's `hashChallenge`) by the dual-signature handshake;
  `wrong_password` and `handshake_timed_out` go with the shared handshake.
- **2.5** replaces the `user_resolver` seam (the memory backend's `assign_user`,
  `test::group_users`) with the handshake's verified user ids, and adds `max_users`.
- Wide bulk sending, once the room envelope goes and frames may reach
  `max_bulk_frame_bytes`: the wire receives wide transfers and offers every one narrow.
- Emitters for `peer_error`, `room_full`, `banned`, `identity_failed` and `plugin_failed`
  (Phases 2 and 3); `relay_failed` has only its boot path (every relay's first connection failed),
  not 0.27.0's "every initial announcement failed".
- The receive decision limits (128 across peers, 8 per peer) and 64 concurrent requests per peer,
  once `on_receive` and `on_request` may answer asynchronously; a gate set after an offer arrived
  should then be asked of it at its next drain, as 0.27.0 asks.
- `requestMany`, a send's and a request's progress and `signal`, and rejecting an unexpected
  `@_response` offer "unexpected response" (the port drops it); no test sends one yet.
- 2.6 pins the oracle to 0.27.0, rewrites every twin (`diff_peers.mjs` reads `PeerInfo`), records
  their outputs again, and retires `diff_relay_config` if 0.27.0's per-room configuration then
  prints what the C++ program prints; `diff_rooms_per_app` goes with the shared connection.
- A test of a page whose `crypto.getRandomValues` is missing or throws (no JavaScript exception
  enters WebAssembly) comes back with the guard in `random_device` above: the deleted
  `platform_page_sealer` test covered it for 0.1.0's sealer, and nothing replaces it yet.
- Commit subjects of the review-fix commits (`5c7f578`, `ea6f2d1`, `b4b87e9`, `487c99e`) are
  paragraphs; Phase 2's commits take a one-sentence subject and the list in the body (no history
  rewrite).
- The clauses "not interoperable before Phase 2" in `room.hpp`'s doc comments, and the same
  marking on the library's pages, come out in 2.6 once the interop lanes pass.
- 2.7 writes the docs whole (README, every page, chapter 15), the changelog's entry, and merges.

## How to resume

Phase 2 is written in full, task by task, when webcpp::crypto's P-256 part merges; its first task
(2.1) confirms or amends design D9 before any code of the shared connection is removed. Each task
records its vectors in `vectors/record.mjs` first (P-256 signatures are random in WebCrypto:
vectors fix keys and check each side verifies the other's signatures).
