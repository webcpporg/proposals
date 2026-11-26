# webcpp::trystero to Trystero 0.27.0: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port webcpp::trystero from Trystero 0.26.0 to Trystero 0.27.0 (Nostr strategy), so that a
room joined from C++ and the same room joined from Trystero 0.27.0's JavaScript are one room.

**Architecture:** Three phases on one long-lived branch, merged to `main` once, when the interop
lane is green against 0.27.0 JS peers (design D1). Phase 1 (this plan in full) builds everything
that needs no P-256: the 0.27.0 crypto layer over webcpp::hash and webcpp::crypto, the new topics,
ids, relays and Nostr details, the action wire v3, and the room's new public surface (listeners,
errors, users and peers, self, relay connectivity). Phase 2 (in full since 2026-11-22) gives
each room its own connections, adds identity and the dual-signature handshake on webcpp::crypto's
P-256, `max_users` and the remaining room errors, and turns interop on. Phase 3 (outlined) adds
the plugins: signed state, roles, then `scale()`.

**Tech Stack:** C++20, header-only; webcpp::hash (SHA-256, HMAC-SHA-256, HKDF, PBKDF2, async
PBKDF2 over WebCrypto on Emscripten); webcpp::crypto (`aes256_gcm`, `random_device`,
`curves::secp256k1` BIP-340; in Phase 2 `curves::p256`); webcpp::asio, webcpp::webrtc,
webcpp::beast, webcpp::document; b2; Node for the vectors and the interop lane.

**Spec:** `../proposal.md`, `../design.md` (decisions D1 to D23), `../specs/trystero/spec.md`.

## Global Constraints

- Upstream pin: Trystero 0.27.0, `dmotz/trystero` commit `8d3f57e` (merge of PR #212), its
  `packages/core/src` and `packages/nostr/src`. Every wire value is proven against that code.
- Vectors: `test/fixtures/expected/trystero-0.27.0.json`, a copy of this change's
  `reference/vectors/trystero-0.27.0.json` (named `phase1.json` until Phase 2 was written),
  regenerated only by `test/oracle/record_0_27.mjs` (a copy of `reference/vectors/record.mjs`)
  run against that pin with the fixture as its previous output, which keeps its recorded P-256
  signatures (design D19): `node test/oracle/record_0_27.mjs <checkout> <fixture>`, or
  `--dist <node_modules>` for the published packages.
- Header-only, namespace `webcpp::trystero`; every public symbol has its doc comment (chapter 7 of
  `CODING-GUIDELINES.md`); English, never the em dash; no exception of trystero's own; every
  operation that can fail returns `boost::system::result` or completes with an `error_code`.
- Every allocation of a room goes through its `std::pmr::memory_resource` (guarantee 6); what a
  dependency allocates outside it is listed in the page's "What stays outside the resource".
- Secrets (room keys, PBKDF2 master, HMAC key, Nostr secret key) are held in
  `webcpp::crypto::secret<N>` or wiped with `webcpp::crypto::wipe` before release.
- Comparisons of MACs and tags are constant time (`webcpp::crypto::equal`).
- Branches: library `port/trystero-0.27` from trystero `main`; superproject
  `trystero/port-0.27` from webcpp `main`. Nothing reaches either `main` before Phase 2's
  interop is green (design D1). Commit subjects are sentences starting `trystero: `, no trailer.
- Run the CI's Linux lint locally in Docker before every push (the owner's rule); sizes and
  speeds are measured on release builds only.
- Every test target builds natively and with `toolset=emscripten`, with and without exceptions
  where the Jamfile already doubles a target.

## Review Focus

1. **Strict base64url.** A sealed text, a room auth token or a compact key with padding, a
   non-canonical last character or a character outside `[A-Za-z0-9_-]` is refused, as
   `fromBase64Url` + re-encode refuses it; tests `open_refuses_what_trystero_refuses` (Task 5)
   and `room_auth_token_is_strict` (Task 4).
2. **Non-ASCII app ids, room ids and passwords.** `JSON.stringify` writes them raw in UTF-8 and
   escapes only `"`, `\` and controls (and lone surrogates as `\udXXX`, which UTF-8 input cannot
   hold); tests on `café` and the emoji vectors in Tasks 2, 4 and 5.
3. **PBKDF2 off the room's thread.** 600,000 iterations must never block a page or a room's
   executor; test `password_room_keys_complete_after_the_join_returns` (Task 6).
4. **A wrong password.** A signal sealed under another key is reported once per peer and stage per
   60 s as `decrypt_failed`, and the room goes on; test `decrypt_failed_is_reported_once_a_minute`
   (Task 10).
5. **Listener lifetime.** A listener that unsubscribes itself, or another, while it runs, and a
   `once` listener, each run exactly as many times as Trystero's emitter runs them; test
   `listeners_unsubscribe_while_dispatching` (Task 9).

---

## File Structure (Phase 1)

| File (library `libs/trystero`) | Responsibility | Task |
| --- | --- | --- |
| `build.jam` | `parts` links `/webcpp/hash//hash`, `/webcpp/crypto//crypto`, `/webcpp/crypto//gcm-default`, `/webcpp/crypto//secp256k1-default`; drops `/third_party//secp256k1` | 1 |
| `test/Jamfile`, `test/crypto_layer_test.cpp`, `test/fixtures/expected/trystero-0.27.0.json`, `test/oracle/record_0_27.mjs` | the vectors and the test that reads them | 1 |
| `include/webcpp/trystero/protocol/text.hpp` | add strict base64url | 2 |
| `include/webcpp/trystero/protocol/topics.hpp` | rewritten: domain tags, `topic_path`, `hash_topic`, root and peer topics, `hash32`, `str_to_num` | 2 |
| `include/webcpp/trystero/protocol/relays.hpp` | 0.27.0's relay list and seeded shuffle | 3 |
| `include/webcpp/trystero/protocol/ids.hpp` (new) | base62 ids, `encode_base62`, UUID v4, compact public keys, peer and user id derivation | 3 |
| `include/webcpp/trystero/protocol/room_keys.hpp` (new) | room AES key without password, PBKDF2 salt, HKDF room keys, room auth | 4 |
| `include/webcpp/trystero/protocol/seal.hpp` (new) | AES-256-GCM seal and open, base64url(iv \|\| ct \|\| tag), the signal AAD | 5 |
| `include/webcpp/trystero/protocol/signal.hpp` | loses `seal_key`, `format_sealed`, `parse_sealed`, `seal_iv_size`; bounds recomputed | 5 |
| `include/webcpp/trystero/backend.hpp` | `sealer` and `make_sealer` removed; `derive_password_master` added | 6 |
| `include/webcpp/trystero/detail/room_keys_cache.hpp` (new) | the app's bounded cache of derived room keys (64, LRU) | 6 |
| `include/webcpp/trystero/platform/backend.hpp` | `random` from `crypto::random_device`; `derive_password_master` natively on a worker, on Emscripten through WebCrypto | 6 |
| `include/webcpp/trystero/platform/sealer.hpp`, `platform/entropy.hpp` | deleted | 6 |
| `include/webcpp/trystero/protocol/nostr.hpp` | `topic_kind` (FNV-1a), signer over webcpp::crypto, REQ with `since` and deduplicated kinds, 250 topics per subscription | 7 |
| `include/webcpp/trystero/detail/nostr_strategy.hpp` | per-room signer, relay health (healthy, cooldown, probation, retired), EOSE replay, announcement nonce | 7 |
| `include/webcpp/trystero/protocol/action_wire.hpp`, `detail/action_wire.hpp` | action wire version 3 (wide bulk frames) | 8 |
| `include/webcpp/trystero/subscription.hpp` (new) | `subscription`, `listen_options`, `detail::listeners<Signature>` | 9 |
| `include/webcpp/trystero/room_error.hpp` (new), `error.hpp` | 0.27.0's room error codes and `room_error` | 10 |
| `include/webcpp/trystero/detail/user_table.hpp` (new) | users and their peers, join and leave by user | 11 |
| `include/webcpp/trystero/room.hpp`, `app.hpp`, `options.hpp` | the new surface: `on_*`, `self()`, `users()`, `peers()`, `set_relay_connectivity`, removed options | 9 to 12 |
| `test/support/memory_backend.hpp` | `derive_password_master` (synchronous, posted), deterministic `random` | 6 |
| `doc/*.adoc`, `README.md`, `AGENTS.md` | Phase 1's part of the pages (Task 13) | 13 |

Phase 1 builds on a provisional identity: a room's peer id is 20 base62 characters drawn at
random and its user id equals its peer id, until Phase 2 derives both from keys. The branch does
not interoperate with JS 0.27.0 before Phase 2 (design D1), so the interop lane stays on the
library's `main` and is off on the branch until Task 2.6.

---

## Phase 1

### Task 1: Dependencies, the vectors and their test

**Files:**
- Modify: `build.jam` (the `parts` alias), `test/Jamfile`
- Create: `test/fixtures/expected/trystero-0.27.0.json` (copy of `reference/vectors/phase1.json`, now
  `trystero-0.27.0.json`),
  `test/oracle/record_0_27.mjs` (copy of `reference/vectors/record.mjs`),
  `test/crypto_layer_test.cpp`

**Interfaces:**
- Produces: the program `crypto_layer`, run as `webcpp.run crypto_layer : crypto_layer_test.cpp :
  <testing.input-file>fixtures/expected/trystero-0.27.0.json ;`, with a helper
  `fixture_value section(std::string_view name)` that later tasks' test cases read.

- [ ] **Step 1:** Create the branches: in the superproject `git switch -c trystero/port-0.27
  origin/main`; in `libs/trystero` `git switch -c port/trystero-0.27 origin/main`.
- [ ] **Step 2:** Write `crypto_layer_test.cpp` with one case, `fixture_is_trystero_0_27_0`:
  `BOOST_TEST_EQ(root["trystero"].as_string(), "0.27.0")`. Load the fixture as
  `protocol_test.cpp`'s `load` does (webcpp::document).
- [ ] **Step 3:** Add the Jamfile line above; run `b2 libs/trystero/test//crypto_layer`;
  expected: PASS.
- [ ] **Step 4:** In `build.jam`'s `parts`, replace `<library>/third_party//secp256k1` by
  `<library>/webcpp/hash//hash <library>/webcpp/crypto//crypto
  <library>/webcpp/crypto//gcm-default <library>/webcpp/crypto//secp256k1-default`; keep the
  `webcpp-secp256k1-tables` comment, now about crypto's secp256k1 backend. Run
  `b2 libs/trystero/test//crypto_layer` and `b2 toolset=emscripten
  libs/trystero/test//crypto_layer`; expected: PASS on both (the protocol headers still include
  libsecp256k1's headers, which crypto's alias brings).
- [ ] **Step 5:** Commit: `trystero: the port to Trystero 0.27.0 starts from its vectors, recorded
  from its own code, and links webcpp::hash and webcpp::crypto`.

### Task 2: Strict base64url and the 0.27.0 topics

**Files:**
- Modify: `include/webcpp/trystero/protocol/text.hpp`, `include/webcpp/trystero/protocol/topics.hpp`
- Test: `test/crypto_layer_test.cpp`; `test/protocol_test.cpp` loses its 0.26 topic cases

**Interfaces:**
- Produces, in `webcpp::trystero::protocol`:
  - `void append_base64url(std::pmr::string& out, std::span<const std::uint8_t> in);`
  - `std::pmr::string to_base64url(std::span<const std::uint8_t> in, std::pmr::memory_resource* memory = std::pmr::get_default_resource());`
  - `std::optional<bytes> from_base64url(std::string_view text, std::pmr::memory_resource* memory = std::pmr::get_default_resource());`
    strict: nonempty, only `[A-Za-z0-9_-]`, no padding, length mod 4 not 1, and re-encoding the
    bytes gives `text` (the unused low bits of the last character are zero).
  - `inline constexpr std::string_view lib_name = "Trystero";`, `lib_id = "trystero";`
  - `std::pmr::string domain_tag(std::string_view name, unsigned version, memory)`:
    `"trystero:<name>:v<version>"`.
  - `void append_json_string(std::pmr::string& out, std::string_view utf8)`: `JSON.stringify`'s
    string form (escapes `"`, `\`, `\b \f \n \r \t`, other controls as `\u00XX` lowercase; all
    else raw). If `protocol/json.hpp` already has it, reuse that one.
  - `std::pmr::string topic_path(std::initializer_list<std::string_view> parts, memory)`:
    `JSON.stringify(parts)`, `["a","b"]`.
  - `std::pmr::string hash_topic(std::string_view text, memory)`: base64url of SHA-256 of the
    UTF-8 bytes, through `webcpp::hash::digest_of<webcpp::hash::digests::sha2_256>`; 43 chars.
  - `std::pmr::string root_topic_text(std::string_view app_id, std::string_view room_id, memory)`
    = `topic_path({lib_name, app_id, room_id})`;
    `root_topic(app_id, room_id, memory)` = `hash_topic(root_topic_text(...))`;
    `peer_topic(app_id, room_id, peer_id, memory)` = `hash_topic(topic_path({root_topic_text(...), peer_id}))`.
  - `std::uint32_t hash32(std::string_view utf8)`: FNV-1a 32 over the UTF-8 bytes, then
    MurmurHash3's fmix32 (`h ^= h >> 16; h *= 0x85ebca6b; h ^= h >> 13; h *= 0xc2b2ae35;
    h ^= h >> 16`), as `primitives.ts`.
  - `std::uint64_t str_to_num(std::string_view utf8, std::uint64_t limit = 9007199254740991)`
    = `hash32(utf8) % limit`. The 0.26 `double str_to_num`, `sha1_topic*`, the 0.26
    `root_topic`/`peer_topic` and `id_alphabet` are removed; `room_token` stays until Task 2.1
    decides it (design D9).
- [ ] **Step 1:** Write the failing cases in `crypto_layer_test.cpp`, each looping over its
  fixture section: `base64url_matches` (`base64url`: `to_base64url(hex(bytes)) == text` and
  `from_base64url(text) == bytes`), `base64url_is_strict` (`from_base64url` refuses `""`, `"A"`,
  `"AB=="`, `"AB+/"`, `"AR"` (non-zero unused bits) and `"A B"`), `topic_path_matches`
  (`topic_path`), `hash_topic_matches` (`hash_topic`), `room_topics_match` (`room_topics`:
  `root_topic_text == root_text`, `root_topic == root_topic`, `peer_topic == peer_topic`),
  `hash32_matches` (`hash32`), `str_to_num_matches` (`str_to_num`, limit 10000), and
  `domain_tags_match` (`domain_tags`).
- [ ] **Step 2:** Run `b2 libs/trystero/test//crypto_layer`; expected: compile errors.
- [ ] **Step 3:** Implement the interfaces above.
- [ ] **Step 4:** Run `b2 libs/trystero/test//crypto_layer libs/trystero/test//protocol`;
  expected: PASS (remove the 0.26 topic and `strToNum` cases from `protocol_test.cpp` and their
  entries in the 0.26 fixture's reader in the same step).
- [ ] **Step 5:** Commit: `trystero: topics are 0.27.0's, the base64url SHA-256 of a JSON path,
  and base64url is read strictly`.

### Task 3: Relays and ids

**Files:**
- Modify: `include/webcpp/trystero/protocol/relays.hpp`, `include/webcpp/trystero/detail/random_id.hpp`
- Create: `include/webcpp/trystero/protocol/ids.hpp`
- Test: `test/crypto_layer_test.cpp`

**Interfaces:**
- Consumes: `hash32`, `str_to_num`, `to_base64url`, `from_base64url`, `domain_tag` (Task 2).
- Produces:
  - `relays.hpp`: `default_nostr_relays` becomes the 30 URLs of `packages/nostr/src/index.ts`
    (`wss://bucket.coracle.social` to `wss://yabu.me`, in that order);
    `template <class T> void seeded_shuffle(std::span<T> xs, std::uint32_t seed)`:
    ```text
    state = (trunc(seed) || 1) >>> 0
    rand(): state = (state + 0x6d2b79f5) mod 2^32
            t = imul(state ^ (state >>> 15), 1 | state)
            t = (t + imul(t ^ (t >>> 7), 61 | t)) ^ t
            return ((t ^ (t >>> 14)) >>> 0) / 2^32
    i = n; while (i) { j = floor(rand() * i); i -= 1; swap(xs[i], xs[j]) }
    ```
    (`j` is computed with the `i` before the decrement, as `floor(rand() * i--)`);
    `derive_relays(app_id, redundancy, memory)` shuffles with `str_to_num(app_id)` (an unsigned
    32-bit value, so the seed is exact) and takes the first `redundancy`.
  - `ids.hpp`: `base62_alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"`;
    `peer_id_size = 20`, `user_id_size = 22`, `offer_id_size = 12`, `nonce_size = 8`,
    `subscription_id_size = 64`, `uuid_text_size = 36`;
    `constexpr std::size_t gen_id_draw_size(std::size_t n)` = `ceil(n * 1.15) + 4` computed in
    integers as `(n * 115 + 99) / 100 + 4`;
    `template <class Draw> std::pmr::string gen_id(std::size_t n, Draw&& draw, memory)`: `draw`
    is `void(std::span<std::uint8_t>)` called with `gen_id_draw_size(n)` bytes each round; a byte
    `b < 248` appends `base62_alphabet[b % 62]`, a byte `>= 248` is skipped; rounds repeat until
    `n` characters;
    `std::pmr::string encode_base62(std::span<const std::uint8_t> big_endian, std::size_t length, memory)`:
    the low `length` base-62 digits of the number, most significant first (long division by 62,
    `length` times, over a copy);
    `std::array<char, 36> uuid_v4(std::array<std::uint8_t, 16> random)`: RFC 9562 version 4
    (`random[6] = (random[6] & 0x0f) | 0x40`, `random[8] = (random[8] & 0x3f) | 0x80`), lowercase
    `8-4-4-4-12`, as `crypto.randomUUID()`;
    `std::pmr::string compact_public_key(std::span<const std::uint8_t, 65> uncompressed, memory)`:
    `to_base64url(x) + "." + to_base64url(y)`, 87 characters;
    `std::optional<std::array<std::uint8_t, 65>> parse_compact_public_key(std::string_view)`:
    87 characters, `.` at 43, both halves strict base64url of 32 bytes (no curve check here:
    Phase 2's `p256::public_key::make` checks the point);
    `std::pmr::string derive_peer_id(std::string_view compact, memory)` =
    `encode_base62(sha256(domain_tag("pid", 1) + ":" + compact), 20)`;
    `derive_user_id(...)` the same with `"uid"` and 22.
  - `detail/random_id.hpp`: `random_id(factory, n, memory)` calls `protocol::gen_id` with
    `factory.random`; `max_random_id_size` stays 64.
- [ ] **Step 1:** Failing cases: `shuffle_matches` (`shuffle`: indices 0 to 29 shuffled with
  `seed` equal `order`; `str_to_num(app) == seed`), `gen_id_matches` (`gen_id`: a `draw` that
  cycles `random_cycle`, as the recorder's `withRandom` does, gives `id` for each `n`),
  `identity_ids_match` (`identity_ids`: `parse_compact_public_key` then `compact_public_key`
  round-trips; `derive_peer_id == peer_id`, `derive_user_id == user_id`),
  `uuid_v4_has_its_form` (16 bytes of `0xff` give `ffffffff-ffff-4fff-bfff-ffffffffffff`, 16 zero
  bytes `00000000-0000-4000-8000-000000000000`), `compact_key_is_strict` (refuses 86 and 88
  characters, `.` elsewhere, a padded half).
- [ ] **Step 2:** Run `b2 libs/trystero/test//crypto_layer`; expected: compile errors.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Run `b2 libs/trystero/test//crypto_layer libs/trystero/test//protocol
  libs/trystero/test//room`; expected: PASS (the room's provisional self id is now base62).
- [ ] **Step 5:** Commit: `trystero: ids are unbiased base62, relays are 0.27.0's list in its
  seeded order, and peer and user ids derive from a compact key`.

### Task 4: Room keys and room authentication

**Files:**
- Create: `include/webcpp/trystero/protocol/room_keys.hpp`
- Test: `test/crypto_layer_test.cpp`

**Interfaces:**
- Consumes: `domain_tag`, `append_json_string`, `to_base64url`, `from_base64url` (Task 2).
- Produces, in `webcpp::trystero::protocol`:
  - `inline constexpr std::uint64_t pbkdf2_iterations = 600'000;`
  - `webcpp::crypto::secret<32> room_aes_key_without_password(std::string_view app_id, std::string_view room_id);`
    = SHA-256 of `JSON.stringify(["trystero:room-aes:v3", app_id, room_id])`.
  - `std::array<std::uint8_t, 32> pbkdf2_salt(std::string_view app_id, std::string_view room_id);`
    = SHA-256 of `JSON.stringify(["trystero:pbkdf2-master:v3", app_id, room_id])`.
  - `struct room_keys { webcpp::crypto::secret<32> aes; webcpp::crypto::secret<32> hmac; };`
  - `boost::system::result<room_keys> derive_room_keys(std::span<const std::uint8_t, 32> master);`
    two `webcpp::hash::hkdf<webcpp::hash::hmac_sha2_256>` calls, salt 32 zero bytes, IKM the
    master, info `"trystero:room-aes:v3"` and `"trystero:room-hmac:v3"`, 32 bytes each (WebCrypto's
    `deriveKey` of a 256-bit AES or HMAC key is HKDF's first 32 bytes).
  - `std::pmr::string room_auth_payload(app_id, room_id, peer_id, std::uint64_t seq, memory)`
    = `JSON.stringify(["trystero:ov-auth:v3", app_id, room_id, peer_id, seq])`; `seq` at most
    2^53 - 1, written as a decimal integer.
  - `std::pmr::string sign_room_auth(const webcpp::crypto::secret<32>& hmac_key, app_id, room_id, peer_id, seq, memory)`:
    base64url of HMAC-SHA-256 over the payload (43 characters).
  - `bool verify_room_auth(const webcpp::crypto::secret<32>& hmac_key, app_id, room_id, peer_id, seq, std::string_view token)`:
    false unless `from_base64url(token)` holds 32 bytes, then `webcpp::crypto::equal` with the
    recomputed MAC.
- [ ] **Step 1:** Failing cases: `room_key_without_password_matches` (`room_key_no_password`),
  `room_keys_from_master_match` (`room_keys_password`: `pbkdf2_salt == salt`;
  `derive_room_keys(master)` gives `aes_key` and `hmac_key`), `pbkdf2_master_matches` (one
  vector only, `webcpp::hash::pbkdf2<hmac_sha2_256>` of the password's UTF-8 with `salt` and
  600,000 iterations gives `master`; the case is in the release variant's run, as it takes about
  0.2 s natively), `room_auth_matches` (`room_auth`: `room_auth_payload == payload`,
  `sign_room_auth == token`, `verify_room_auth` true and false for `seq + 1`),
  `room_auth_token_is_strict` (the token with `=` appended, with its last character changed to
  another that decodes to the same bytes, and 42 characters, are refused).
- [ ] **Step 2:** Run; expected: compile errors.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Run `b2 libs/trystero/test//crypto_layer` and the emscripten build of it;
  expected: PASS.
- [ ] **Step 5:** Commit: `trystero: a room's keys are 0.27.0's, SHA-256 without a password and
  PBKDF2 then HKDF with one, and its auth tokens are HMACs read strictly`.

### Task 5: Sealing and opening signals

**Files:**
- Create: `include/webcpp/trystero/protocol/seal.hpp`
- Modify: `include/webcpp/trystero/protocol/signal.hpp`
- Test: `test/crypto_layer_test.cpp`, `test/protocol_test.cpp`

**Interfaces:**
- Consumes: Task 2's base64url; Task 4's keys.
- Produces, in `webcpp::trystero::protocol`:
  - `inline constexpr std::size_t gcm_iv_size = 12, gcm_tag_size = 16;`
  - `constexpr std::size_t sealed_size(std::size_t plaintext)` = base64url length of
    `12 + plaintext + 16` (`(4 * k + 2) / 3` for `k` bytes).
  - `boost::system::result<std::pmr::string> seal(const webcpp::crypto::secret<32>& key, std::span<const std::uint8_t, 12> iv, std::string_view plaintext, std::optional<std::string_view> aad, memory);`
    `webcpp::crypto::aes256_gcm::make(key)`, `seal(iv, aad bytes or empty, data)`, then
    base64url(iv || ciphertext || tag). An absent AAD and an empty AAD are the same to GCM;
    the parameter keeps Trystero's distinction readable.
  - `boost::system::result<std::pmr::string> open(const webcpp::crypto::secret<32>& key, std::string_view sealed, std::optional<std::string_view> aad, memory);`
    refuses (`error::decrypt_failed`, Task 10's code; until then a local placeholder) what
    `from_base64url` refuses, fewer than 28 bytes, a failed tag, and a plaintext that is not
    UTF-8 (design D12).
  - `std::pmr::string signal_aad(std::string_view type, std::string_view peer_id, std::string_view offer_id, memory)`
    = `type + "|" + peer_id + "|" + offer_id`.
  - `signal.hpp`: `seal_key`, `seal_iv_size`, `format_sealed`, `parse_sealed`, `sealed_parts`
    and `handshake_proof` are removed; `max_sealed_description_bytes` and
    `max_sealed_candidate_bytes` keep their values, now in base64url characters; the reading of
    a signal is unchanged in this task (its 0.27.0 fields are Task 2.4's).
- [ ] **Step 1:** Failing cases: `seal_matches` (`seal`: for each entry the key is
  `room_key_no_password` or the password vector's `aes_key`, `seal(key, iv, plaintext, aad)
  == sealed` and `open(...) == plaintext`), `password_probe_opens` (each `room_keys_password`
  entry's `probe_sealed` opens to `"probe"` with AAD `"offer|p|o"`),
  `open_refuses_what_trystero_refuses` (`open_refused`: every text refused),
  `open_refuses_another_aad` (a `seal` entry opened with the other AAD fails),
  `signal_aad_matches` (`signal_aad`).
- [ ] **Step 2:** Run; expected: compile errors.
- [ ] **Step 3:** Implement; delete the 0.26 sealing cases of `protocol_test.cpp`.
- [ ] **Step 4:** Run `b2 libs/trystero/test//crypto_layer libs/trystero/test//protocol` on both
  targets; expected: PASS. The room does not build until Task 6 replaces the sealer: build only
  these two programs.
- [ ] **Step 5:** Commit: `trystero: signals are sealed as 0.27.0 seals them, AES-256-GCM with a
  12-byte IV and the signal's AAD, in base64url`.

### Task 6: The room's keys in the room, the backend without a sealer

**Files:**
- Modify: `include/webcpp/trystero/backend.hpp`, `include/webcpp/trystero/platform/backend.hpp`,
  `include/webcpp/trystero/detail/signaling.hpp`, `include/webcpp/trystero/room.hpp`,
  `include/webcpp/trystero/detail/app_impl.hpp`, `test/support/memory_backend.hpp`, `test/Jamfile`
- Create: `include/webcpp/trystero/detail/room_keys_cache.hpp`
- Delete: `include/webcpp/trystero/platform/sealer.hpp`, `platform/entropy.hpp`,
  `test/platform_sealer_test.cpp`, `test/platform_page_sealer_test.cpp`,
  `test/platform_page_sealer_allocation_test.cpp`
- Test: `test/room_test.cpp`, `test/platform_backend_test.cpp`

**Interfaces:**
- Consumes: Tasks 4 and 5.
- Produces:
  - `backend::factory`: `make_sealer` and `class sealer` removed; added
    `virtual void derive_password_master(std::span<const std::uint8_t> password, std::span<const std::uint8_t, 32> salt, std::span<std::uint8_t, 32> out, handler<void(boost::system::error_code)> done) = 0;`
    with the contract of every factory completion: `done` runs on the room's executor after the
    call returns; `password` and `salt` are the caller's until `done`; on error `out` is zeroed.
  - `platform::backend`: `random` fills from `webcpp::crypto::random_device::fill` (a failure
    is fatal as today's is); natively `derive_password_master` runs
    `webcpp::hash::async_pbkdf2<webcpp::hash::hmac_sha2_256>` on a one-thread
    `webcpp::asio::thread_pool` the backend owns (created on first use, joined by its
    destructor), and on Emscripten
    `async_pbkdf2<hmac_sha2_256, webcpp::hash::backends::webcrypto>` on the loop's executor
    (design D5).
  - `detail::room_keys_cache` (in the app's resource): `void get(std::string_view room_id, handler<void(boost::system::error_code, const protocol::room_keys&)>)`
    without a password returns `room_aes_key_without_password` synchronously (posted); with
    one, derives once per room id, queues waiters while it runs, keeps at most 64 rooms (least
    recently used out), and forgets a failed derivation, as `derivedRoomKeysCache` does. The
    PBKDF2 master is wiped after HKDF.
  - The signaling seals and opens with Task 5's functions under the room's AES key; it waits for
    the key before its first announcement (0.27.0's room waits for `genKey`).
  - `memory_backend`: `derive_password_master` computes synchronously with
    `webcpp::hash::pbkdf2` and posts `done`; a test can set `derive_delay` to hold it.
- [ ] **Step 1:** Failing cases in `room_test.cpp`:
  `password_room_keys_complete_after_the_join_returns` (a room of an app with a password, over a
  memory backend whose `derive_delay` holds the derivation: no announcement leaves before the
  derivation completes, `join` returned first, and two peers with that password meet);
  `rooms_of_one_password_derive_once_per_room` (two rooms of one app derive twice, a third join
  of the first room's id after its leave derives none); `a_failed_derivation_is_retried` (the
  memory backend fails the first derivation: the room reports `error::backend_failure` through
  its error handler, announces nothing, and a rejoin derives again; 0.27.0 has no room error
  code for it, design D6).
- [ ] **Step 2:** Run `b2 libs/trystero/test//room`; expected: compile errors.
- [ ] **Step 3:** Implement; delete the files listed and their Jamfile targets; the
  `headers-alone` list loses `platform/sealer.hpp` and `platform/entropy.hpp`.
- [ ] **Step 4:** Run `b2 libs/trystero/test libs/trystero/example` and `b2 toolset=emscripten
  libs/trystero/test libs/trystero/example`; expected: every test passes but the interop and
  oracle lanes, which the branch turns off until Task 2.6 (`test/oracle/Jamfile`: an
  `explicit` on their targets, with a comment naming Task 2.6).
- [ ] **Step 5:** Commit: `trystero: a room's keys come from webcpp::hash, PBKDF2 off the room's
  thread, and the backend keeps no sealer and no randomness of its own`.

### Task 7: Nostr as 0.27.0 speaks it

**Files:**
- Modify: `include/webcpp/trystero/protocol/nostr.hpp`, `include/webcpp/trystero/detail/nostr_strategy.hpp`
- Test: `test/crypto_layer_test.cpp`, `test/signaling_test.cpp`, `test/room_test.cpp`

**Interfaces:**
- Consumes: Task 3's `gen_id`; webcpp::crypto's `curves::secp256k1::keypair` (`make`,
  `generate`, `xonly_public_key`, `sign_schnorr`) and `verify_schnorr`.
- Produces:
  - `std::uint32_t topic_kind(std::string_view topic)`: FNV-1a 32 over the topic's UTF-16 code
    units (`h = 0x811c9dc5; h = (h ^ unit) * 0x01000193`), then `h % 10000 + 20000` (no fmix,
    unlike `hash32`).
  - `class signer`: over `webcpp::crypto::curves::secp256k1::keypair`; `signer::generate()`
    returns `boost::system::result<signer>`; `pubkey_hex()`, `sign(event_id)`. The libsecp256k1
    includes leave trystero.
  - `subscribe_message(sub_id, topics, since, memory)`: `["REQ", sub_id, {"kinds": [...],
    "since": since, "#x": [...]}]`, kinds deduplicated in first-seen order; `since` =
    `max(0, unix_now - 60)`; at most `max_topics_per_subscription = 250` topics per REQ, extra
    REQs for the rest, surplus subscriptions closed.
  - The announcement's content is `{"peerId":"<id>","nonce":"<gen_id(8)>"}` (Trystero adds the
    nonce at publish time on every announcement).
  - The strategy: one signer per room (keyed by root topic and self topic, made on first
    publish, dropped with the room's last subscription), relay health `healthy`, `cooldown`
    (`60 s` doubling to `15 min`), `probation`, `retired`; `OK true` ends probation; a CLOSED
    or rejected OK whose reason starts with `blocked:`, `restricted:`, `auth-required:` or
    `pow:` retires the relay; `rate-limited:` cools it down; a CLOSED of a subscription id not
    ours is ignored; an `EOSE` closing the last pending subscription 1 s or more after the REQ
    replays the announcements; `duplicate:` rejections are not logged.
- [ ] **Step 1:** Failing cases: `topic_kind_matches` (`topic_to_kind`, and
  `room_topics[*].root_kind`), in `signaling_test.cpp`
  `subscriptions_hold_250_topics_and_distinct_kinds`, `announcements_carry_a_fresh_nonce`,
  `terminal_rejections_retire_a_relay`, `rate_limits_cool_a_relay_down_and_probation_ends_on_ok`,
  `closed_of_another_subscription_is_ignored`, `slow_eose_replays_announcements`; in
  `room_test.cpp` `each_room_signs_with_its_own_key`.
- [ ] **Step 2:** Run `b2 libs/trystero/test//crypto_layer libs/trystero/test//signaling
  libs/trystero/test//room`; expected: failures.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Run the whole `libs/trystero/test` on both targets; expected: PASS (the lanes
  off). Run the allocation tests and record in `doc/allocation.adoc`'s "What stays outside the
  resource" what webcpp::crypto's secp256k1 context allocates, if anything (design D7).
- [ ] **Step 5:** Commit: `trystero: Nostr is 0.27.0's, a signer per room over webcpp::crypto,
  kinds by FNV-1a, subscriptions since a minute ago, and relays retired, cooled and probed`.

### Task 8: The action wire, version 3

**Files:**
- Modify: `test/oracle/record_0_27.mjs` (and this change's `reference/vectors/record.mjs`),
  `test/fixtures/expected/trystero-0.27.0.json`,
  `include/webcpp/trystero/protocol/action_wire.hpp`, `include/webcpp/trystero/detail/action_wire.hpp`
- Test: `test/crypto_layer_test.cpp`, `test/room_test.cpp`

**Interfaces:**
- Produces: frames byte-identical to 0.27.0's `action-wire.ts`: version byte 3; kinds inline 0,
  offer 1, chunk 2, accepted 3, rejected 4, cancelled 5 in the low four bits of byte 1, the
  format (text 0, JSON 1, binary 2) above them as today, and the flag `wide_bulk_flag = 64` on
  a chunk of a transfer whose chunks may be up to `max_bulk_frame_bytes = 65536` bytes; header
  sizes 36, 48, 6 and 14 unchanged; 0.27.0's limits (`maxPendingTransfers` 1024, per peer 64,
  decisions 128, per peer 8, pre-listener messages 32, control buffer 1 MiB, receive budget
  256 MiB).
- [ ] **Step 1:** Extend the recorder: import `packages/core/src/action-wire.ts`, make two
  managers whose `resolveTransport` returns a transport that records `sendFrame(bytes,
  transferId, isOffer)` and delivers to the other manager's `handleData`; record, as hex, the
  frames of: a text, a JSON value and a 10-byte binary message; a message with metadata
  `{"a":1}`; a 200 KiB binary message (offer, chunks, accepted); a transfer the receiver's
  `onReceive` declines (rejected); a request and its response (`kind: 'request'`); a cancelled
  send (abort signal). Add the section `action_frames` to the fixture.
- [ ] **Step 2:** Failing case `action_frames_match`: the port's encoder writes each recorded
  frame from the same inputs, and its decoder reads each back to the same message.
- [ ] **Step 3:** Run; expected: FAIL (version 2).
- [ ] **Step 4:** Implement the deltas the vectors show.
- [ ] **Step 5:** Run `b2 libs/trystero/test` on both targets; expected: PASS.
- [ ] **Step 6:** Commit: `trystero: the action wire is version 3, with wide bulk frames, proven
  against 0.27.0's frames`.

### Task 9: Listeners that return a subscription

**Files:**
- Create: `include/webcpp/trystero/subscription.hpp`
- Modify: `include/webcpp/trystero/room.hpp`, `include/webcpp/trystero/app.hpp`,
  `include/webcpp/trystero.hpp`, `example/*.cpp`, `example/chat/chat.cpp`
- Test: `test/subscription_test.cpp` (new, `webcpp.run subscription`), `test/room_test.cpp`

**Interfaces:**
- Produces:
  - `struct listen_options { bool once = false; bool replay = false; };`
  - `class subscription` (move-only): `void unsubscribe() noexcept;` `[[nodiscard]] bool
    active() const noexcept;` destruction leaves the listener in place, as dropping the function
    Trystero returns does (design D10).
  - `template <class Signature> class detail::listeners`: `subscription add(handler<Signature>,
    listen_options)`, `void emit(Args...)`; listeners run in the order added; a listener added
    while emitting does not run for that emission; one removed while emitting does not run if
    not yet run; `once` removes before it runs.
  - Room: `subscription on_peer_join(handler<void(const peer_event&)>, listen_options = {})`,
    `on_peer_leave`, `on_user_join(handler<void(const user_event&)>, ...)`, `on_user_leave`;
    `struct peer_event { std::string_view peer_id; std::string_view user_id; };`
    `struct user_event { std::string_view user_id; std::string_view peer_id; };` `replay`
    emits the current peers or users at once, inside the call.
  - Actions: `subscription on_message(handler<...>, listen_options = {})` (several);
    `boost::system::result<subscription> on_request(...)` and `on_receive(...)` (one at a time;
    a second while one is active is `error::already_subscribed`); `on_receive_progress(...)`.
    The setters of 0.1.0 (`on_peer_join(f)` returning `void`) are removed.
- [ ] **Step 1:** Failing cases in `subscription_test.cpp`: `listeners_run_in_order`,
  `listeners_unsubscribe_while_dispatching` (a listener that removes itself and the next one;
  the next does not run), `once_runs_once`, `added_while_dispatching_waits`,
  `dropping_a_subscription_keeps_the_listener`; in `room_test.cpp`
  `replay_reports_current_peers`, `second_on_request_is_refused`.
- [ ] **Step 2 to 4:** Run (fail), implement, run `b2 libs/trystero/test libs/trystero/example`
  on both targets (PASS).
- [ ] **Step 5:** Commit: `trystero: listeners are added with on_ and return a subscription, as
  0.27.0's are`.

### Task 10: Room errors

**Files:**
- Create: `include/webcpp/trystero/room_error.hpp`
- Modify: `include/webcpp/trystero/error.hpp`, `room.hpp`, `app.hpp`, `detail/signaling.hpp`,
  `detail/nostr_strategy.hpp`
- Test: `test/room_test.cpp`, `test/config_test.cpp`

**Interfaces:**
- Produces:
  - `error` gains, after `too_many_rooms`: `room_full`, `banned`, `relay_failed`,
    `identity_failed`, `plugin_failed`, `decrypt_failed`, `peer_connect_failed`, `peer_error`,
    `already_subscribed`, `not_connected`, `action_aborted`, `request_rejected`,
    `request_timed_out`; `handshake_failed` is no longer reserved. Values are appended, so
    existing values keep their numbers. Each message is 0.27.0's (`"room full"`,
    `"peer banned"`, `"failed to decrypt offer (incorrect password or corrupted signal)"` and so
    on, read from `utils.ts` and `signal-handler.ts`).
  - `std::string_view room_error_code_name(boost::system::error_code)`: `"room-full"`,
    `"banned"`, `"relay-failed"`, `"identity-failed"`, `"plugin-failed"`, `"decrypt-failed"`,
    `"peer-connect-failed"`, `"handshake-failed"`, `"peer-error"`, else `""`.
  - `struct room_error { boost::system::error_code code; std::string_view message; std::string_view app_id; std::string_view room_id; std::string_view peer_id; std::optional<signal_stage> stage; bool turn_configured = false; std::string_view reason; std::string_view plugin; };`
    with `enum class signal_stage { offer, answer };`
  - `subscription room::on_error(handler<void(const room_error&)>, listen_options = {})`;
    `app::on_error` likewise (configuration reports).
  - `decrypt_failed` is reported at most once per (peer, stage) per 60 s;
    `peer_connect_failed` carries `turn_configured` (any ICE URL `turn:` or `turns:`);
    `relay_failed` is reported once when every relay of the room has failed.
- [ ] **Step 1:** Failing cases: `decrypt_failed_is_reported_once_a_minute` (two apps with
  different passwords, virtual clock), `relay_failed_when_every_relay_fails`,
  `peer_connect_failed_says_whether_turn_was_configured`, `error_names_match_trystero`.
- [ ] **Step 2 to 4:** Run (fail), implement, run the whole test directory on both targets.
- [ ] **Step 5:** Commit: `trystero: a room reports 0.27.0's error codes through on_error, each
  with its context`.

### Task 11: Users and peers

**Files:**
- Create: `include/webcpp/trystero/detail/user_table.hpp`
- Modify: `include/webcpp/trystero/room.hpp`, `include/webcpp/trystero/app.hpp`
- Test: `test/user_table_test.cpp` (new, `webcpp.run user_table`), `test/room_test.cpp`

**Interfaces:**
- Produces:
  - `detail::user_table` (in the room's resource): `bool add(std::string_view peer_id, std::string_view user_id)`
    (true when the user is new), `std::optional<std::pmr::string> remove(std::string_view peer_id)`
    (the user when its last peer left), `std::span<const std::pmr::string> peers_of(user_id)`,
    `std::optional<std::string_view> user_of(peer_id)`.
  - `struct user_info { std::pmr::vector<std::pmr::string> peer_ids; std::pmr::vector<std::pmr::string> roles; };`
    `struct peer_info { std::pmr::string user_id; bool direct = true; std::uint32_t hops = 1; std::pmr::vector<std::pmr::string> roles; };`
    `room::users()` -> `std::pmr::map<std::pmr::string, user_info>`, `room::peers()` ->
    `std::pmr::map<std::pmr::string, peer_info>` (no `connection`: design D11).
  - `struct self_info { std::string_view peer_id; std::string_view user_id; identity_persistence persistence; std::string_view room_id; std::string_view base_room_id; bool is_admin = false; std::span<const std::pmr::string> roles; };`
    `room::self()`, synchronous (design D11); `enum class identity_persistence { ephemeral, imported };`
    Phase 1 fills `user_id = peer_id`, `ephemeral`.
  - `on_user_join` fires when a user's first peer joins, after `on_peer_join`; `on_user_leave`
    when its last peer leaves, after `on_peer_leave`.
  - `action.send(data, target)`: `target` is a user id (all its peers), a peer id, or a list;
    a target not connected completes with `error::not_connected`; a broadcast with no peer
    completes with success.
- [ ] **Step 1:** Failing cases: `user_table_test.cpp` `first_peer_joins_the_user`,
  `last_peer_leaves_the_user`, `a_peer_moves_nowhere` (adding a known peer under another user is
  refused); `room_test.cpp` `send_to_a_user_reaches_each_of_its_peers` (the memory backend's
  test hook `assign_user(peer, user)` maps two peers to one user),
  `send_to_an_absent_target_fails`, `self_names_the_room`.
- [ ] **Step 2 to 4:** Run (fail), implement, run the whole test directory on both targets.
- [ ] **Step 5:** Commit: `trystero: a room knows its users and their peers, and sends to either`.

### Task 12: Options, joins and relay connectivity

**Files:**
- Modify: `include/webcpp/trystero/options.hpp`, `app.hpp`, `room.hpp`,
  `detail/nostr_strategy.hpp`, `detail/app_impl.hpp`, `example/diff_*.cpp` and their twins
- Test: `test/app_impl_test.cpp`, `test/room_test.cpp`

**Interfaces:**
- Produces:
  - `room_options`: `handshake_timeout` removed (0.27.0 removed `handshakeTimeoutMs`);
    `max_receive_bytes` stays; `app_config::self_id` removed (0.27.0 removed a custom
    `peerId`; Phase 1 draws it, Phase 2 derives it); `app_config::identity` reserved for
    Phase 2 (absent in Phase 1).
  - `enum class relay_connectivity { online, offline, automatic };`
    `relay_connectivity app::set_relay_connectivity(relay_connectivity)` and
    `relay_connectivity app::relay_connectivity() const`: `offline` closes the relays' sockets
    and holds them, `online` reconnects, `automatic` is `online` natively and on Emscripten
    follows the page's `online` and `offline` events (design D13).
  - A join of a room id already active in the app keeps returning `error::already_joined` (0.27.0
    now throws there too: the difference `diff_already_joined` is removed with its twin).
- [ ] **Step 1:** Failing cases: `offline_holds_and_online_reconnects`,
  `automatic_is_online_natively`, `no_custom_self_id`.
- [ ] **Step 2 to 4:** Run (fail), implement, run tests and examples on both targets.
- [ ] **Step 5:** Commit: `trystero: relays go offline and online on request, and the options
  0.27.0 removed are gone`.

### Task 13: Phase 1's pages and its gate

**Files:**
- Modify: `README.md`, `doc/differences.adoc`, `doc/design.adoc`, `doc/protocol.adoc`,
  `doc/allocation.adoc`, `doc/usage.adoc`, `doc/guarantees.adoc`, `AGENTS.md`; superproject
  `CODING-GUIDELINES.md` chapter 15's trystero section (on the superproject branch)

- [ ] **Step 1:** Write what Phase 1 changed, marked "on the branch `port/trystero-0.27`, not
  interoperable before Phase 2": the protocol page's topics, keys, sealing, ids and relays; the
  design page's D5, D6 and D7 (the backend without a sealer, the key derivation, the
  dependencies); the differences that Phase 1 adds or removes (design D14's table); the
  allocation page's list; chapter 15's lines that name the sealer, OpenSSL's AES-GCM and
  `RAND_bytes`, libsecp256k1 as a direct dependency, and the 2026-11-17 decision (superseded,
  design D2). README's dependency table moves to webcpp::hash and webcpp::crypto.
- [ ] **Step 2:** Run `b2 libs/trystero/test libs/trystero/example libs/trystero/doc` and
  `b2 toolset=emscripten libs/trystero/test libs/trystero/example`; the lint in Docker
  (`local-linux-lint`); expected: all green but the two lanes turned off.
- [ ] **Step 3:** Push both branches (not `main`); open draft pull requests titled
  `trystero: port to Trystero 0.27.0 (draft, merges after its interop)`; the CI must be green
  except the lanes marked off.
- [ ] **Step 4:** Commit: `trystero: the pages say what Phase 1 of the port to 0.27.0 changed`.

---

## Phase 2

Written in full on 2026-11-22, once webcpp::crypto's P-256 (`<webcpp/crypto/curves/p256.hpp>`,
BearSSL on every target, the alias `/webcpp/crypto//p256-default`) and its secp256k1
`keypair::generate` reached `main` and the branch. Its rulings are the notes P2-R1 to P2-R24
(`.superpowers/sdd/plan2/plan-writer-notes.md` of the superproject's worktree) and design D17 to
D23. Every task runs on the library branch `port/trystero-0.27`, and the superproject branch
`trystero/port-0.27` for chapter 15 and the pointer.

**How to run (every task).** `lanes.sh <clang|gcc|emscripten> [targets]` and `heavy.sh` are the
port's helpers, named with their full paths in the implementers' context file
(`.superpowers/sdd/plan2/context.md` of the port's superproject worktree); targets are relative to
the superproject, for instance `libs/trystero/test//room`; with none, `lanes.sh` runs
`libs/trystero/test libs/trystero/example`. A full lane, a lint, MSVC or an interop lane goes
through `heavy.sh trystero-027 <command>`, its output in the port's `logs/` directory. "Run both" below means the focused targets on
clang and on emscripten; "Run all" means the whole `libs/trystero/test` and
`libs/trystero/example` on clang and emscripten through `heavy.sh`. Ruling R1 of Phase 1 holds:
a task that removes or renames something moves every caller in the same task, and the whole
library builds after every task.

**The vectors.** `reference/vectors/trystero-0.27.0.json` (this change; Phase 1's `phase1.json`
renamed, its sections unchanged but `domain_tags`, which gained `id-hs` and
`identity-self-test`) holds Phase 2's sections too: `signatures`, `p256_keys`, `compact_keys`,
`user_identity`, `user_identity_imports`, `user_proof`, `low_s`, `handshake`,
`handshake_frames`, `ping_frames`, `identity_limits`. Task 2.2's implementer copies it over
`test/fixtures/expected/trystero-0.27.0.json` and `reference/vectors/record.mjs` over
`test/oracle/record_0_27.mjs`; nothing else writes either file. Keys are fixed (a key's scalar is
SHA-256 of `webcpp-trystero-vector:<label>`, its `d` in the fixture); a signature in the fixture is
WebCrypto's, made by 0.27.0's `ecdsaSign` and kept across runs by the recorder (design D19): the
C++ side verifies every JS signature, and the interop lane is where JS verifies the C++ side's
(Task 2.6b's scenario `js_verifies_cpp_signatures`).

### Review Focus (Phase 2)

1. **A peer claiming an identity it does not hold.** A step whose `uid` is not the user key's id,
   whose `pk` is not the sending peer id's key, whose signatures are of another key or another
   transcript, or high S, never joins and is reported once as `handshake_failed`; tests
   `handshake_failures_match` (2.4a) and `a_forged_step_fails_the_handshake` (2.4b).
2. **A relay-side man in the middle that holds the password.** The handshake binds both DTLS
   fingerprints: a step whose `dtls` differ from the description the room applied fails with
   "DTLS fingerprint mismatch"; tests `handshake_failures_match` (2.4a) and
   `another_fingerprint_fails_the_handshake` (2.4b).
3. **`max_users` under refreshes and races.** It counts users, not peers, the room's own user
   included; another peer of a user already present is admitted when the room is full; two
   newcomers handshaking at once cannot both take the last place; tests
   `another_peer_of_a_present_user_is_admitted` and `concurrent_admissions_respect_the_limit`
   (2.5a).
4. **A user identity text from outside.** Spaces around it, its JSON form, a wrong prefix, a `d`
   that is zero, the group order, 31 bytes or another key's, a missing coordinate: each imports or
   fails as 0.27.0's `importUserIdentity` does, with `error::invalid_user_identity`, and the
   secret never outlives the call in trystero's buffers; test `user_identity_imports_match` (2.2).
5. **Rooms of one app are independent.** Leaving, failing or refusing one room never touches
   another room's connection to the same peer, and an app holds more than 64 rooms; tests
   `leaving_one_room_keeps_the_other_rooms_connection` and `an_app_joins_more_than_64_rooms`
   (2.1a); and a room left during a handshake reports nothing after its leave,
   `leaving_during_a_handshake_reports_nothing` (2.4b).

### File Structure (Phase 2)

| File (library `libs/trystero`) | Responsibility | Task |
| --- | --- | --- |
| `detail/connection.hpp`, `detail/rebinding.hpp`, `protocol/envelope.hpp`, `test/shared_test.cpp`, `test/connection_test.cpp`, `example/diff_rooms_per_app.cpp`, `test/oracle/twins/diff_rooms_per_app.*` | deleted: the shared connection, its presence frames, room tokens and rebinding | 2.1a |
| `detail/peer_link.hpp` (new), `test/peer_link_test.cpp` (new) | one room's connection to one peer: frames bare, 0.27.0's pending-frame and size bounds | 2.1a |
| `detail/app_impl.hpp`, `app.hpp`, `options.hpp`, `room.hpp`, `detail/signaling.hpp`, `detail/action_wire.hpp`, `protocol/topics.hpp`, `error.hpp` | no sharing: no adoption, seconds, idle time, claim, room token, `max_rooms`, `too_many_rooms` | 2.1a |
| `detail/handshake_manager.hpp` (new), `test/handshake_manager_test.cpp` (new) | 0.27.0's handshake manager: phases, 10 s, 16 buffered messages, ready frames | 2.1b |
| `room.hpp`, `error.hpp`, `protocol/signal.hpp` | the password handshake, `wrong_password`, `handshake_timed_out`, `max_handshake_bytes` gone | 2.1b |
| `protocol/nostr.hpp` | `signer::generate` over `keypair::generate` (R11) | 2.1b |
| `backend.hpp`, `platform/backend.hpp`, `platform/page.hpp` (if the page's call lives there), `test/support/memory_backend.hpp`, `detail/action_wire.hpp` | `peer_connection::max_message_size`; wide chunks sent | 2.1c |
| `protocol/ecdsa.hpp` (new), `protocol/identity.hpp` (new), `identity.hpp` (new, public), `trystero.hpp` | P-256 signing and verification with low S; the user key text; the user-proof; `user_identity` | 2.2 |
| `detail/app_impl.hpp`, `app.hpp`, `options.hpp`, `room.hpp`, `test/support/rooms.hpp`, `build.jam` | the endpoint key and the user identity of an app; ids derived; `identity_failed`; `p256-default` linked | 2.2 |
| `detail/identity_cache.hpp` (new), `test/identity_cache_test.cpp` (new) | key records (4,096) and verified signatures (8,192, 2 MiB) per app | 2.3 |
| `protocol/handshake.hpp` (new), `detail/identity_handshake.hpp` (new) | the handshake's messages, transcript and fingerprints; one side's run of it | 2.4a |
| `room.hpp`, `detail/signaling.hpp`, `detail/app_impl.hpp`, `test/support/memory_backend.hpp`, `test/support/rooms.hpp` | the handshake in the room; verified user ids; the `user_resolver` seam gone | 2.4b |
| `options.hpp`, `room.hpp`, `error.hpp` | `max_users`, `room_full` | 2.5a |
| `room.hpp`, `detail/nostr_strategy.hpp`, `detail/action_wire.hpp` | `peer_error`, `banned`, `relay_failed`'s announcement path, `@_ping` as a request, "unexpected response" | 2.5b |
| `test/oracle/package.json`, `package-lock.json`, `oracle.mjs`, `Jamfile`, `twins/*`, `fixtures/cases/protocol.json`, `fixtures/expected/expected.json`, `test/protocol_test.cpp`, `test/signaling_test.cpp` | the oracle on 0.27.0; the recorder over the published packages; the twins | 2.6a |
| `test/oracle/interop/*`, `test/platform_page_entropy_test.cpp` (new) | interop with 0.27.0 on both lanes; the lanes on | 2.6b |
| `README.md`, `AGENTS.md`, `doc/*.adoc`, superproject `CODING-GUIDELINES.md` chapter 15, this change's `STATUS.md` | the pages whole; the changelog's lines | 2.7 |

---

### Task 2.1a: One connection per room and remote peer

Design D9 is confirmed (design.md, "D9" as amended by this plan): in 0.27.0 each room's
`strategy.ts` makes its own offers (`createOfferManager(() => initPeer(true, ...))`) and answers
(`signal-handler.ts`'s `initPeerFn(false, ...)`), a room's frames are action frames written bare
on the channel (`peer-transport.ts`'s `sendFrame`), and nothing is shared across rooms but the
identity and the relays.

**Files:**
- Delete: `include/webcpp/trystero/detail/connection.hpp`, `detail/rebinding.hpp`,
  `protocol/envelope.hpp`, `test/shared_test.cpp`, `test/connection_test.cpp`,
  `example/diff_rooms_per_app.cpp`, `test/oracle/twins/diff_rooms_per_app.mjs`,
  `test/oracle/twins/diff_rooms_per_app.expected`
- Create: `include/webcpp/trystero/detail/peer_link.hpp`, `test/peer_link_test.cpp`
- Modify: `detail/app_impl.hpp`, `app.hpp`, `options.hpp`, `room.hpp`, `detail/signaling.hpp`,
  `detail/action_wire.hpp`, `protocol/topics.hpp`, `error.hpp`, `test/Jamfile`,
  `test/app_impl_test.cpp`, `test/room_test.cpp`, `test/allocation_test.cpp`,
  `test/protocol_test.cpp`, `test/signaling_test.cpp`, `test/support/rooms.hpp`,
  `example/Jamfile` (if it lists the example), `doc/differences.adoc` (the row of
  `diff_rooms_per_app` only)

**Interfaces:**
- Produces, in `webcpp::trystero::detail` (`peer_link.hpp`):
  ```cpp
  class peer_link final : public channel_link, public std::enable_shared_from_this<peer_link> {
  public:
      struct callbacks {
          handler<void(bytes frame)> frame;               // an action frame, in arrival order
          handler<void(std::string_view reason)> ended;   // once
      };
      static constexpr std::size_t max_pending_frames = 64;                  // maxQueuedDataFrames
      static constexpr std::chrono::milliseconds pending_frames_timeout{10'000};
      static constexpr std::size_t max_unbound_frame_bytes = 16 * 1024 + 1024; // maxRoomFrameBytes
      static constexpr std::size_t max_bound_frame_bytes = 64 * 1024;          // maxBulkFrameBytes
      static std::shared_ptr<peer_link> make(std::string_view peer,
          std::shared_ptr<backend::peer_connection> pc, std::shared_ptr<backend::data_channel> dc,
          backend::factory& factory, std::pmr::memory_resource* memory);
      void bind(callbacks cb);          // replays the frames kept before it, in order
      void end(std::string_view reason);
      // channel_link: send(frame), buffered_amount(), wait_drained(done)
  };
  ```
  Before `bind` a frame is kept (at most 64, at most 17 KiB each, the first kept starting a 10 s
  timer); a 65th frame, a longer one or the timer ends the link, as `peer.ts`'s `failData`. After
  `bind` a frame above 64 KiB ends it. The connection's state handling (the 5 s grace of a
  `disconnected` state, `closed` and `failed`) moves over from `connection::on_state` unchanged.
- `channel_link::send` takes the action frame itself (its parameter renamed `frame`);
  `action_wire`'s constructor loses its `room_token` parameter; `begin_room_frame`,
  `room_frame`, `presence_frame`, `decode_envelope`, `max_room_token_bytes`,
  `max_room_frame_bytes` and `protocol::room_token` are gone.
- `app`: `max_rooms` gone; `join` never fails with `too_many_rooms`, which leaves `error`;
  `app_config::idle_timeout` gone (0.27.0 has no idle time: a room's connection ends with the room
  or with the peer). `app_impl` keeps its room registry (`find_room`, `counted_rooms`, the offline
  fan-out, `finish_farewell`) and loses `connections_`, `seconds_`, `adopt`, `share_or_adopt`, the
  `hold_*` family, `switch_to_second`, `on_connection_end`, `live_connections`, `find`,
  `idle_timeout`; `room_endpoint` loses `presence_changed`, `keeps_frames_from`, `frame_kept`,
  `connection_ended`.
- `signaling::callbacks::claim` is gone; the signaling makes a room's connections in the room's
  resource (the backend's doc: "the room's for every object it makes"), since no connection
  outlives its room any more.
- A room keeps one link per peer: a second connection that opens for a peer whose link is live is
  closed (0.27.0's `connectPeer` keeps a connected peer that is not stale); one whose link is
  ending replaces it.

- [ ] **Step 1: Write the failing tests.** `peer_link_test.cpp` (`webcpp.run peer_link`, both
  targets, replacing `connection`): `frames_before_bind_are_kept_in_order`,
  `a_65th_pending_frame_ends_the_link`, `pending_frames_end_the_link_after_10_s` (virtual clock),
  `an_unbound_frame_above_17_kib_ends_the_link` (17,408 passes, 17,409 ends),
  `a_bound_frame_above_64_kib_ends_the_link` (65,536 passes, 65,537 ends),
  `a_disconnected_state_ends_after_its_grace`. In `room_test.cpp`:
  `rooms_of_one_app_have_their_own_connections` (apps A and B both join `r1` and `r2`:
  `net.made_connections()` counts two connections between them, one per room),
  `leaving_one_room_keeps_the_other_rooms_connection` (A leaves `r1`; B's `r2` keeps A, its
  messages flow, and no `on_peer_leave` fires in `r2`), `frames_travel_bare` (the first frame
  A's room writes to B, through `net.filter`, starts with the action wire's version byte 3 and
  is no envelope), `an_app_joins_more_than_64_rooms` (65 joins succeed).
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//peer_link libs/trystero/test//room`;
  expected: compile errors (`peer_link` and the new cases).
- [ ] **Step 3: Implement** the interfaces above; delete the files listed and the Jamfile targets
  `shared`, `shared_no_exceptions`, `shared_hardened`, `connection` and their `_no_exceptions`
  twins; delete the cases of `app_impl_test.cpp` that adopt connections (adopt, seconds, ends,
  last user) and of `room_test.cpp` that exercise envelopes, presence and kept frames
  (`a_frame_of_an_unknown_token_is_dropped`, `malformed_envelopes_end_the_connection`,
  `a_withdrawn_presence_is_a_departure`, `a_kick_by_the_last_room_ends_the_connection`,
  `a_room_left_with_no_idle_time_sends_its_presence`, `a_connections_end_is_seen_and_the_peers_meet_again`,
  `a_late_end_of_another_connection_keeps_the_peer`); a case that tests a still-valid behaviour
  through the old machinery (`three_peers_form_a_mesh`) is kept and rewritten. The provisional
  password handshake stays, now on the peer's link, until Task 2.1b. `protocol_test.cpp` loses its
  envelope and `room_token` cases. The row of `diff_rooms_per_app` leaves `doc/differences.adoc`.
- [ ] **Step 4: Run all**; expected: PASS but the lanes off (oracle, interop). Run the
  allocation tests on clang alone first (`lanes.sh clang libs/trystero/test//allocation`): the
  connections are in the room's resource now; update the counts the tests pin only where a
  connection moved resources, and say so in the commit's body.
- [ ] **Step 5: Commit:** `trystero: each room has its own connection to each peer, as 0.27.0's
  rooms do, and the shared connection, its presence frames and room tokens are gone`.

### Task 2.1b: The handshake's frame, without the password challenge

0.27.0's rooms authenticate a password by the sealed signals alone; their handshake is
`handshake.ts`'s manager, whose steps are the identity handshake (Task 2.4b). This task ports the
manager with no steps, and makes the Nostr signer's key webcpp::crypto's.

**Files:**
- Create: `include/webcpp/trystero/detail/handshake_manager.hpp`, `test/handshake_manager_test.cpp`
- Modify: `room.hpp`, `error.hpp`, `room_error.hpp`, `protocol/signal.hpp`, `detail/app_impl.hpp`,
  `protocol/nostr.hpp`, `test/Jamfile`, `test/room_test.cpp`, `test/allocation_test.cpp`,
  `test/protocol_test.cpp`, `test/support/rooms.hpp`, `example/diff_passwords.cpp` (its comment
  only)

**Interfaces:**
- Produces, in `webcpp::trystero::detail`:
  ```cpp
  struct handshake_callbacks {
      // Runs the steps (none before Task 2.4b); `done` takes "" when they passed, else the reason.
      handler<void(std::string_view peer, bool initiator, handler<void(std::string_view)> done)> run_steps;
      handler<void(std::string_view peer, std::string_view ready_json)> send_ready;  // @_hsready
      handler<bool(std::string_view peer)> local_passed;   // onLocalHandshakePassed (2.5a: ban, full)
      handler<bool(std::string_view peer)> admit;          // acquireAdmissionSlot (2.5a)
      handler<void(std::string_view peer)> activate;
      handler<void(std::string_view peer, std::string_view reason)> failed;
  };
  class handshake_manager {
  public:
      static constexpr std::chrono::milliseconds timeout{10'000};   // defaultHandshakeTimeoutMs
      static constexpr std::size_t max_buffered_messages = 16;      // maxBufferedHandshakeMessages
      handshake_manager(std::string_view self_id, bool defer_responder_ready, handshake_callbacks cb,
                        backend::factory& factory, std::pmr::memory_resource* memory);
      void add(std::string_view peer);
      void start(std::string_view peer);
      void clear(std::string_view peer);
      void receive_data(std::string_view peer, std::string_view json);  // an @_hsdata payload
      void next_message(std::string_view peer, handler<void(boost::system::error_code, std::string_view json)> next);
      void receive_ready(std::string_view peer);
      [[nodiscard]] bool can_receive(std::string_view peer, bool receive_while_pending) const;
  };
  ```
  Phases and transitions are `createHandshakeManager`'s (`added`, `running_steps`,
  `awaiting_remote_ready`, `preparing_ready`, `awaiting_peer_ready`, `active`); the initiator is
  the side whose id is lower (`self_id < peer`, byte order); the ready frame is sent before
  `activate`, so that a send inside `on_peer_join` follows it; a 17th message buffered before the
  steps read it fails the handshake ("too many pending handshake messages"); the timer fails it
  with "handshake timed out after 10000ms"; `clear` fails a pending `next_message` with
  "peer disconnected during handshake". `preparing_ready` sends `""` (0.27.0's
  `payload || ''`; the `{ok: true, ...}` form is the state plugin's, Phase 3).
- `@_hsdata` payloads are bounded to 64 KiB and `@_hsready`'s to 128 KiB (room.ts's
  `maxPayloadBytes`); `protocol::max_handshake_bytes` is gone.
- Every failure the manager reports is a `room_error` with `error::handshake_failed`, the
  reason as its message (0.27.0's message without its `Trystero: ` prefix, as Phase 1's
  messages), and the peer id; the peer gets `@_leave` and leaves the room. `handshake_failed`'s
  doc loses "Reserved".
- `error`: `wrong_password` and `handshake_timed_out` are removed (design D23); `__trystero_pw`,
  `room_impl::handshake_proof`, `start_handshake`, `on_handshake_data`, the rebinding challenges
  and `app_impl::set_handshake_timeout` are gone; tests advance the virtual clock by 10 s instead.
- `protocol::signer::generate(memory)` returns
  `webcpp::crypto::curves::secp256k1::keypair::generate(memory)`'s key pair, turned into a
  signer as `from_secret` does today; `max_key_draws` and the draw loop are gone (Phase 1's R11).

- [ ] **Step 1: Write the failing tests.** `handshake_manager_test.cpp` (`webcpp.run
  handshake_manager`, both targets): `the_lower_id_initiates`,
  `ready_goes_before_activate`, `both_ready_activate_once`,
  `a_17th_buffered_message_fails_the_handshake`, `the_timer_fails_after_10_s`,
  `clear_fails_a_waiting_step`. In `room_test.cpp`:
  `a_handshake_that_never_completes_fails_after_10_s` (`net.filter` drops B's `@_hsready`; after
  10 s A's `on_error` has `handshake_failed`, message `handshake timed out after 10000ms`, peer
  B's id; neither side ever reports a join), `ready_frames_carry_an_empty_text` (the `@_hsready`
  frame A sends B, through `net.filter`, is the fixture's `handshake_frames` entry `ready` byte
  for byte), `sends_inside_on_peer_join_follow_the_ready_frame`,
  `two_passwords_never_meet` (kept: `decrypt_failed`, no join), and in `crypto_layer_test.cpp`
  `handshake_frames_match` for the entry `ready` (the port's encoder writes its frame).
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//handshake_manager
  libs/trystero/test//room libs/trystero/test//crypto_layer`; expected: compile errors.
- [ ] **Step 3: Implement**; delete the handshake cases named in the as-built map (the password
  challenge, proofs, rebinding steps, `a_handshake_message_past_its_bound_fails`) and the
  hand-written `__trystero_pw` frame of `allocation_test.cpp`.
- [ ] **Step 4: Run all**; expected: PASS but the lanes off.
- [ ] **Step 5: Commit:** `trystero: the handshake is 0.27.0's frame and timer, and the
  password challenge 0.27.0 dropped is gone`.
- [ ] **Step 6:** In `protocol/nostr.hpp`, the `signer::generate` switch (Interfaces); run
  `lanes.sh clang libs/trystero/test//signaling libs/trystero/test//room` and the same on
  emscripten (the cases `a_strategy_draws_the_key_of_its_event_ids` and
  `each_room_signs_with_its_own_key` pass unchanged).
- [ ] **Step 7: Commit:** `trystero: the Nostr signer's key comes from webcpp::crypto's secp256k1
  keypair::generate`.

### Task 2.1c: Wide bulk chunks are sent

With the envelope gone a chunk may fill a 64 KiB SCTP message: 0.27.0 sends wide chunks to a
peer whose `RTCPeerConnection.sctp.maxMessageSize` is at least 65,536 (`peer-transport.ts`).

**Files:**
- Modify: `include/webcpp/trystero/backend.hpp`, `platform/backend.hpp` (native and page),
  `detail/peer_link.hpp`, `detail/action_wire.hpp`, `test/support/memory_backend.hpp`
- Test: `test/room_test.cpp`, `test/platform_peer_connection_test.cpp`,
  `test/platform_page_peer_connection_test.cpp`

**Interfaces:**
- `backend::peer_connection`: `[[nodiscard]] virtual std::optional<std::size_t>
  max_message_size() const = 0;` the SCTP transport's largest message once known, else
  `std::nullopt`; natively libdatachannel's `rtc::PeerConnection::remoteMaxMessageSize()`, in a
  page `pc.sctp?.maxMessageSize` (a non-finite or absent value is `nullopt`).
- `channel_link`: `[[nodiscard]] virtual std::size_t max_bulk_frame_bytes() const = 0;` 65,536
  when the connection's `max_message_size()` is at least 65,536, else 16,384, as 0.27.0's getter.
- `action_wire` offers wide (`chunk_width::wide`) when the link's `max_bulk_frame_bytes()` is
  65,536.
- `memory_network::max_message_size` (default `std::size_t{256} * 1024`, Chrome's), which a
  test sets to 16 KiB or to `std::nullopt`.

- [ ] **Step 1:** Failing cases in `room_test.cpp`: `a_wide_link_sends_wide_chunks` (a
  200 KiB binary message over the default memory network: every chunk frame carries the flag 64,
  and the offer and chunk headers equal the fixture's `action_frames` case `bulk_wide`
  frame by frame, `head` and `size`; the payload arrives whole),
  `a_narrow_link_sends_narrow_chunks` (`max_message_size` 16 KiB: the case `bulk`'s frames);
  in the platform tests, `max_message_size_is_known_once_connected`.
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//room`; expected: compile errors.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run all**, and `lanes.sh clang libs/trystero/test//platform_peer_connection`;
  expected: PASS.
- [ ] **Step 5: Commit:** `trystero: a transfer is sent in wide chunks to a peer whose
  connection takes 64 KiB messages, as 0.27.0 sends it`.

### Task 2.2: Endpoint and user keys, and the user identity

**Files:**
- Create: `include/webcpp/trystero/protocol/ecdsa.hpp`, `protocol/identity.hpp`,
  `include/webcpp/trystero/identity.hpp`
- Modify: `build.jam` (`parts` links `/webcpp/crypto//p256-default`), `include/webcpp/trystero.hpp`,
  `options.hpp`, `app.hpp`, `detail/app_impl.hpp`, `room.hpp`, `error.hpp`,
  `test/support/rooms.hpp`, `test/fixtures/expected/trystero-0.27.0.json` and
  `test/oracle/record_0_27.mjs` (copies of this change's `reference/vectors/` files)
- Test: `test/crypto_layer_test.cpp`, `test/room_test.cpp`, `test/app_impl_test.cpp`

**Interfaces:**
- Consumes: `webcpp::crypto::curves::p256` (`keypair::generate()`, `keypair::make(scalar)`,
  `keypair::public_key()`, `keypair::secret_scalar()`, `keypair::sign_ecdsa(digest)`,
  `public_key::make(uncompressed)`, `public_key::uncompressed()`, `verify_ecdsa(signature,
  digest, key)`, `normalize_ecdsa(signature)`, `check_low_s(signature)`); Phase 1's
  `compact_public_key`, `parse_compact_public_key`, `derive_peer_id`, `derive_user_id`,
  `to_base64url`, `from_base64url`, `json::append_string_total`.
- Produces, in `webcpp::trystero::protocol` (`ecdsa.hpp`):
  ```cpp
  using p256_signature = std::array<std::uint8_t, 64>;
  // ecdsaSign: sign_ecdsa of SHA-256(payload), then normalize_ecdsa (low S).
  boost::system::result<p256_signature> sign_p256(const webcpp::crypto::curves::p256::keypair& key,
                                                  std::span<const std::uint8_t> payload);
  // isLowSP256Signature: 64 bytes, 0 < r < n, 0 < s <= (n - 1) / 2 (check_low_s).
  [[nodiscard]] bool is_low_s(std::span<const std::uint8_t> signature);
  // verifyWithP256 without its cache: 64 bytes, low S, verify_ecdsa of SHA-256(payload).
  [[nodiscard]] bool verify_p256(std::span<const std::uint8_t> signature,
                                 std::span<const std::uint8_t> payload,
                                 const webcpp::crypto::curves::p256::public_key& key);
  ```
- Produces, in `webcpp::trystero::protocol` (`identity.hpp`):
  ```cpp
  inline constexpr std::string_view user_key_prefix = "trystero_uk_";
  struct user_key_material { webcpp::crypto::secret<32> d; std::array<std::uint8_t, 65> point; };
  // tryParsePrivateJwk: trims what String.prototype.trim trims; a text that starts with the prefix
  // is base64url of JSON, one that starts with '{' is JSON; d, x, y strings, d strict base64url of
  // 32 bytes and a valid scalar, x and y strict base64url of 32 bytes; kty "EC" and crv "P-256"
  // where present. Every intermediate buffer holding d is wiped.
  [[nodiscard]] std::optional<user_key_material> parse_user_key(std::string_view text);
  // trystero_uk_ + base64url(JSON {"d":..,"x":..,"y":..}), as buildExtractableUserIdentity writes.
  std::pmr::string write_user_key(const user_key_material& key, std::pmr::memory_resource* memory);
  // userEndpointProofPayload: JSON ["trystero:user-proof:v3",app,room,peer,compact,seq].
  std::pmr::string user_proof_payload(std::string_view app_id, std::string_view room_id,
      std::string_view peer_id, std::string_view peer_compact, std::uint64_t seq,
      std::pmr::memory_resource* memory);
  ```
  The user-proof has no caller in 0.27.0's core (design D21); the port keeps the payload, and
  `sign_p256` and `verify_p256` prove it against the vectors.
- Produces, in `webcpp::trystero` (`identity.hpp`, public, design D17):
  ```cpp
  class user_identity {   // copyable; its key is shared and immutable
  public:
      [[nodiscard]] std::string_view user_id() const noexcept;
      [[nodiscard]] std::string_view public_key() const noexcept;   // compact, 87 characters
      [[nodiscard]] boost::system::result<protocol::p256_signature>
          sign(std::span<const std::uint8_t> payload) const;        // low S
  };
  [[nodiscard]] boost::system::result<user_identity>
      create_user_identity(std::pmr::memory_resource* memory = std::pmr::get_default_resource());
  [[nodiscard]] boost::system::result<user_identity>
      import_user_identity(std::string_view text,
                           std::pmr::memory_resource* memory = std::pmr::get_default_resource());
  [[nodiscard]] std::pmr::string
      export_user_identity(const user_identity& identity,
                           std::pmr::memory_resource* memory = std::pmr::get_default_resource());
  ```
  `import_user_identity` refuses with the new `error::invalid_user_identity` (appended after
  `action_redefined`; message 0.27.0's "invalid user identity key material; expected exported
  userKey or P-256 JWK") what `parse_user_key` refuses and a key whose self-test fails (it signs
  the bytes of `domain_tag("identity-self-test", 1)`, `trystero:identity-self-test:v1`, and verifies
  with the given point). `create_user_identity` fails with webcpp::crypto's
  `entropy_unavailable`. The key lives in `memory` (`std::allocate_shared` with a
  `std::pmr::polymorphic_allocator`); the allocation page lists it.
- `app_config::identity` (after `ice_servers`, before `log`): `std::optional<user_identity> identity = {};` absent, the app makes an
  ephemeral user identity at construction; given, rooms report `identity_persistence::imported`
  (0.27.0's `'custom'`, design D17). `self_info::user_id` is the identity's; `app::user_id()`
  (new) returns it.
- The endpoint key: `app_impl` makes `p256::keypair::generate()` at construction, and its
  peer id is `derive_peer_id(compact_public_key(its point))` (`app::self_id()`, unchanged API).
  The random 20-character draw goes. If the endpoint key or the ephemeral user key cannot be
  made, `self_id()` is empty, and every room joined reports, posted after `join` returns, a
  `room_error` with `error::identity_failed` and the message "endpoint identity resolution
  failed" (or "identity resolution failed" for the user key), then ends without announcing
  (0.27.0's `failSetup`).
- Test seam (replaces `app_access::make(..., self_id)`):
  `static app app_access::make(executor, backend::factory&, app_config,
  std::span<const std::uint8_t, 32> endpoint_scalar);` a zero scalar makes the endpoint fail.
  `test/support/rooms.hpp` gains `std::array<std::uint8_t, 32> endpoint_scalar(std::string_view
  label)` (SHA-256 of the label) and `make_app_with(label, ...)`; a test that needs one peer
  to initiate orders its labels by the peer ids they derive (the helper
  `ordered_labels(n)` returns labels whose ids ascend).

- [ ] **Step 1:** Copy `reference/vectors/trystero-0.27.0.json` and `record.mjs` over the
  library's copies; `fixture_is_trystero_0_27_0` still passes.
- [ ] **Step 2: Write the failing tests** in `crypto_layer_test.cpp`, each over its fixture
  section: `p256_keys_derive_their_ids` (`keypair::make(d)`'s point equals `uncompressed`;
  `compact_public_key` equals `compact`; `derive_peer_id` and `derive_user_id` equal
  `peer_id` and `user_id`), `compact_keys_are_read_as_0_27_0_reads_them`
  (`parse_compact_public_key` succeeds exactly when `text_valid`; `p256::public_key::make` of
  its point succeeds exactly when `record`), `user_identity_text_matches`
  (`export_user_identity(import_user_identity(text))` equals `text`; `user_id` equals),
  `user_identity_imports_match` (each case: accepted or `invalid_user_identity`, and its
  `user_id`), `user_proof_matches` (`user_proof_payload` equals `payload`; its SHA-256 equals
  `digest`; `verify_p256(proof, payload, user key)` true and false for `seq + 1`; a
  `sign_p256` of the payload by `user-a`'s key is low S and verifies), `low_s_matches` (each
  case: `is_low_s` equals `is_low_s`, `verify_p256` against the user-proof's payload and
  `user-a`'s key equals `verifies`; for a case in range, `normalize_ecdsa` equals
  `normalized`), `cpp_signatures_are_deterministic_and_low_s` (two `sign_p256` of one payload
  are equal, RFC 6979). In `room_test.cpp`: `an_apps_ids_derive_from_its_keys`,
  `two_apps_of_one_imported_identity_share_a_user_id`, `self_reports_the_identity`
  (ephemeral without `identity`, imported with), `an_endpoint_that_cannot_be_made_reports_identity_failed`
  (zero scalar: `on_error` gets `identity_failed` with its message, nothing is published to the
  relay, the room ends).
- [ ] **Step 3: Run** `lanes.sh clang libs/trystero/test//crypto_layer libs/trystero/test//room`;
  expected: compile errors.
- [ ] **Step 4: Implement**; move every caller of `app_access::make(..., self_id)` to the
  scalar seam (R1).
- [ ] **Step 5: Run all**, and `lanes.sh clang libs/trystero/test//allocation`; record the user
  key's and endpoint key's allocations in `doc/allocation.adoc`'s "What stays outside the
  resource" if any (the endpoint's is in the app's resource).
- [ ] **Step 6: Commit:** `trystero: an app's peer id and user id derive from its P-256 endpoint
  and user keys, and a user identity is created, imported and exported as 0.27.0's`.

### Task 2.3: Key records and the verification caches

**Files:**
- Create: `include/webcpp/trystero/detail/identity_cache.hpp`, `test/identity_cache_test.cpp`
- Modify: `detail/app_impl.hpp`, `test/Jamfile`

**Interfaces:**
- Consumes: Task 2.2's `verify_p256`, `is_low_s`, `parse_compact_public_key`, `derive_peer_id`,
  `derive_user_id`.
- Produces, in `webcpp::trystero::detail` (one per app, in the app's resource; design D18):
  ```cpp
  struct key_record {
      webcpp::crypto::curves::p256::public_key key;
      std::pmr::string compact, peer_id, user_id;
  };
  class identity_cache {
  public:
      static constexpr std::size_t max_records = 4096;                 // createBoundedCache's
      static constexpr std::size_t max_signatures = 8192;              // maxSigCacheEntries
      static constexpr std::size_t max_signature_bytes = 2 * 1024 * 1024;  // maxSigCacheTotalBytes
      static constexpr std::size_t max_cached_payload_bytes = 4096;    // maxCachedSigPayloadBytes
      explicit identity_cache(std::pmr::memory_resource* memory);
      // getVerifiedPubKeyRecord: nullptr when the text or the point is refused; LRU.
      const key_record* record(std::string_view compact);
      // verifyWithP256: 64 bytes; a cached (key, signature) answers whether its payload is this
      // one, without verifying; otherwise low S, verify_p256, and a payload of at most 4096 bytes
      // that verifies is cached (LRU by entries and by bytes).
      bool verify(std::string_view compact, std::span<const std::uint8_t> signature,
                  std::span<const std::uint8_t> payload);
      bool verify_peer_id(std::string_view peer_id, std::string_view compact);  // verifyPeerIdKey
      bool verify_user_id(std::string_view user_id, std::string_view compact);  // verifyUserIdKey
  };
  ```
  `verify_user_id` refuses an id that is not 22 base62 characters before any lookup
  (`userIdRegex`); `verify_peer_id` refuses an empty one. The fixture's `identity_limits` names the
  four bounds.
- [ ] **Step 1:** Failing cases (`webcpp.run identity_cache`, both targets):
  `records_are_evicted_least_recently_used_at_4096`, `an_off_curve_key_has_no_record`,
  `a_cached_signature_answers_for_its_payload_only`, `payloads_over_4096_bytes_are_not_cached`,
  `the_cache_holds_at_most_8192_signatures_and_2_mib`, `a_high_s_signature_is_refused_before_verification`
  (the fixture's `low_s` high form, never cached), `ids_are_checked_against_their_keys`
  (`p256_keys`), `limits_are_0_27_0s` (the constants equal `identity_limits`).
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//identity_cache`; expected: compile
  errors.
- [ ] **Step 3: Implement**; `app_impl` owns one `identity_cache` and gives rooms a reference.
- [ ] **Step 4: Run both** (`identity_cache`, `crypto_layer`); expected: PASS.
- [ ] **Step 5: Commit:** `trystero: verified keys and signatures are cached per app within
  0.27.0's bounds, and a high S is refused before any verification`.

### Task 2.4a: The handshake's messages and one side's run of it

**Files:**
- Create: `include/webcpp/trystero/protocol/handshake.hpp`,
  `include/webcpp/trystero/detail/identity_handshake.hpp`
- Test: `test/crypto_layer_test.cpp`

**Interfaces:**
- Consumes: Tasks 2.2 and 2.3; Phase 1's `uuid_v4`, `domain_tag`, `json::append_string_total`,
  `protocol/json.hpp`'s reader.
- Produces, in `webcpp::trystero::protocol` (`handshake.hpp`):
  ```cpp
  inline constexpr std::string_view handshake_step_key = "__trystero_id";
  inline constexpr std::size_t min_nonce_units = 16;   // n.length, in UTF-16 code units
  namespace handshake_reason {   // 0.27.0's messages, without "Trystero: "
      inline constexpr std::string_view identity = "peer identity verification failed";
      inline constexpr std::string_view dtls_mismatch = "DTLS fingerprint mismatch";
      inline constexpr std::string_view dtls_missing = "DTLS fingerprint missing";
      inline constexpr std::string_view too_many_messages = "too many pending handshake messages";
      inline constexpr std::string_view timed_out = "handshake timed out after 10000ms";
      inline constexpr std::string_view disconnected = "peer disconnected during handshake";
  }
  // extractDtlsFingerprints: each line "a=fingerprint:<v>" (multiline, the line's end before \r
  // or \n), v trimmed and lowercased (ASCII), deduplicated, sorted by UTF-16 order.
  std::pmr::vector<std::pmr::string> dtls_fingerprints(std::string_view sdp, std::pmr::memory_resource* memory);
  struct handshake_claims {
      int step = 0;
      std::pmr::string nonce, user_id, endpoint_key, user_key, endpoint_signature, user_signature;
      std::pmr::vector<std::pmr::string> dtls;
  };
  // {"__trystero_id":<step>,"n":..,"dtls":[..],"uid":..,"pk":..,"uk":..[,"ps":..,"us":..]}, the
  // key order of handshake.ts's sendClaims; step 3 is {"__trystero_id":3,"ps":..,"us":..}.
  std::pmr::string write_handshake_step(const handshake_claims& claims, std::pmr::memory_resource* memory);
  // parseAndVerifyPeerIdentityClaims without its key checks: the shape (step, n of 16 units or
  // more, dtls an array whose non-strings are dropped, uid non-empty, pk and uk strings, ps and
  // us at step 2), then the DTLS rule; on failure `failure` is a handshake_reason.
  struct handshake_read { std::optional<handshake_claims> claims; std::string_view failure; };
  handshake_read read_handshake_step(std::string_view json, int expected_step,
      std::span<const std::pmr::string> expected_remote_dtls,
      std::span<const std::pmr::string> local_dtls, std::pmr::memory_resource* memory);
  // encodeTranscript: JSON ["trystero:id-hs:v2",app,room,init peer,resp peer,init nonce,
  // resp nonce,[init dtls],[resp dtls],init user,resp user].
  std::pmr::string handshake_transcript(std::string_view app_id, std::string_view room_id,
      std::string_view initiator_peer, std::string_view responder_peer,
      std::string_view initiator_nonce, std::string_view responder_nonce,
      std::span<const std::pmr::string> initiator_dtls, std::span<const std::pmr::string> responder_dtls,
      std::string_view initiator_user, std::string_view responder_user, std::pmr::memory_resource* memory);
  ```
- Produces, in `webcpp::trystero::detail` (`identity_handshake.hpp`), `createIdentityHandshake`'s
  `run` as a synchronous step machine:
  ```cpp
  struct local_identity {
      const webcpp::crypto::curves::p256::keypair& endpoint;
      std::string_view peer_id, endpoint_key;   // the endpoint's id and compact key
      const user_identity& user;
  };
  class identity_handshake {
  public:
      struct verified { std::pmr::string user_id, endpoint_key, user_key; };
      struct outcome {
          std::pmr::string send;               // an @_hsdata payload to send, or empty
          std::optional<verified> done;        // the remote's identity, verified
          std::string_view failure;            // a handshake_reason, or empty
      };
      identity_handshake(local_identity self, identity_cache& cache, std::string_view app_id,
          std::string_view room_id, std::string_view remote_peer, bool initiator,
          std::span<const std::pmr::string> local_dtls, std::span<const std::pmr::string> remote_dtls,
          std::array<std::uint8_t, 16> nonce_random, std::pmr::memory_resource* memory);
      outcome start();                         // initiator: step 1; responder: nothing
      outcome receive(std::string_view json);  // the next @_hsdata payload
  };
  ```
  Order and checks are `run`'s: `start` fails with `dtls_missing` when either list is empty; the
  nonce is `uuid_v4(nonce_random)`; each step's `pk` and `uid` are checked with the cache's
  `verify_peer_id(remote_peer, pk)` and `verify_user_id(uid, uk)`; the transcript's remote DTLS
  are the description's when known, else the claimed; both signatures are verified with
  `cache.verify` over the transcript's bytes; the initiator reports `done` with step 3 in `send`
  (0.27.0's `onVerified` runs between them, where its roles plugin checks bans: Phase 3's); the
  responder reports `done` after step 3 verifies. Signatures are `sign_p256` by the endpoint key and `user.sign`.

- [ ] **Step 1: Write the failing tests** in `crypto_layer_test.cpp`, over the fixture's
  `handshake`: `dtls_fingerprints_match` (each case's `sdp_local` gives the `dtls` its side
  sent), `handshake_steps_match` (the case `verified`: `write_handshake_step` of each recorded
  message's fields equals its `json`), `handshake_transcript_matches` (`transcript` and
  `transcript_digest`), `js_handshake_signatures_verify` (step 2's `ps` and `us` with
  `endpoint-b` and `user-b`, step 3's with `endpoint-a` and `user-a`, through
  `identity_cache::verify`), `cpp_responder_meets_the_js_initiator` (an `identity_handshake`
  with `endpoint-b` and `user-b` made from the fixture's `d`, `nonce_random` giving the
  recorded second nonce, fed the recorded step 1: its step 2 equals the recorded one but for
  `ps` and `us`, which verify; fed the recorded step 3, it reports `done` with `user-a`'s id),
  `cpp_initiator_meets_the_js_responder` (the mirror: fed the recorded step 2, it reports
  `done` with `user-b`'s id and a step 3 whose signatures verify), `handshake_failures_match`
  (each failing case: the C++ side that the record shows failing, fed the tampered message it
  received (the recorded `messages`), fails with the recorded error less `Trystero: `; a side
  recorded `waits` gets no message and reports nothing), `uuid_nonces_have_16_units_or_more`.
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//crypto_layer`; expected: compile errors.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run both** (`crypto_layer`); expected: PASS.
- [ ] **Step 5: Commit:** `trystero: the dual-signature handshake's messages and transcript are
  0.27.0's, and each side's run of it verifies the other's JS signatures`.

### Task 2.4b: The handshake in the room, and verified users

**Files:**
- Modify: `room.hpp`, `detail/signaling.hpp`, `detail/app_impl.hpp`, `detail/user_table.hpp` (if
  its API needs no change, untouched), `test/support/memory_backend.hpp`,
  `test/support/rooms.hpp`, `test/memory_backend_test.cpp`
- Test: `test/room_test.cpp`, `test/allocation_test.cpp`

**Interfaces:**
- Consumes: Tasks 2.1b (`handshake_manager`), 2.2, 2.3, 2.4a.
- `signaling::callbacks::connected` gains the descriptions the negotiation set:
  `handler<void(std::string_view peer, std::shared_ptr<backend::peer_connection>,
  std::shared_ptr<backend::data_channel>, std::string_view local_sdp, std::string_view remote_sdp)>`
  (the last description `set_local_description` gave and the last one applied with
  `set_remote_description`); the room extracts their fingerprints (design D20). The backend is
  unchanged.
- `handshake_callbacks::run_steps` runs one `identity_handshake` per peer over `@_hsdata`
  (`next_message` feeds `receive`); its nonce bytes come from `factory.random`; the room keeps the
  `verified` identity in the peer's state before it calls `done("")`.
- A verified peer's user id is the handshake's; `user_table::add(peer, user)` takes it.
  `app_impl::user_resolver`, `resolve_users_with`, `user_of`, `memory_network::assign_user` and
  `user_of`, and `test::group_users` are removed; a test that needs one user with two peers makes
  two apps that import one `user_identity`.
- The memory backend's SDP becomes `mem:<id>\r\na=fingerprint:sha-256 <F>\r\n`, `F` the 32 bytes
  of SHA-256 of `mem-fingerprint:<id>`, uppercase hexadecimal pairs joined by `:`; the reader
  takes the id from the first line.
- A failed handshake: `room_error` `handshake_failed` with the reason and the peer id, `@_leave`
  sent, the peer gone; a ban (2.5b) is never reported as `handshake_failed`.

- [ ] **Step 1: Write the failing tests** in `room_test.cpp`:
  `peers_verify_each_other_and_join_with_their_user_ids` (each `on_peer_join`'s `user_id` is the
  other app's `app::user_id()`), `a_forged_step_fails_the_handshake` (the responder's step 1 from
  A is dropped by `net.filter` and a step 1 claiming `user-b`'s id with A's keys is injected with
  `net.inject`: B reports `handshake_failed` "peer identity verification failed", no join on
  either side), `another_fingerprint_fails_the_handshake` (the injected step 1 claims other
  `dtls`: "DTLS fingerprint mismatch"), `leaving_during_a_handshake_reports_nothing` (B leaves
  while A's step 2 is held; nothing is reported after the leave, no handler runs on a destroyed
  room), and the users cases that used `assign_user`, rewritten with two apps importing one
  identity: `a_user_leaves_with_its_last_peer`, `a_peer_of_its_own_user_is_no_other_user`,
  `send_to_a_user_reaches_each_of_its_peers`, `leaves_are_reported_peer_then_user`. In
  `memory_backend_test.cpp`: `descriptions_carry_a_fingerprint`.
- [ ] **Step 2: Run** `lanes.sh clang libs/trystero/test//room libs/trystero/test//memory_backend`;
  expected: compile errors.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run all**; expected: PASS but the lanes off.
- [ ] **Step 5: Commit:** `trystero: a peer joins a room only after the dual-signature handshake
  binds its endpoint and user keys to both DTLS fingerprints, and users are the ids it verified`.

### Task 2.5a: max_users and room_full

**Files:**
- Modify: `options.hpp`, `room.hpp`, `app.hpp`, `error.hpp`, `detail/signaling.hpp` (its
  should-connect hook)
- Test: `test/room_test.cpp`, `test/crypto_layer_test.cpp`

**Interfaces:**
- `room_options::max_users` (after `max_json_document_bytes`): `std::optional<std::size_t> max_users = {};` absent, unbounded; `0` or
  a value above 2^53 - 1 is refused by `app::join` with the new `error::invalid_max_users`
  (appended; message "maxUsers must be a positive safe integer", 0.27.0's `posSafeIntErr`).
- `room.ts`'s admission, ported: users counted are the room's own user, every active peer's user
  and the users of admissions under way (`reservedAdmissionUsers`); a candidate whose user is
  among them is never full; `handshake_manager` is made with `defer_responder_ready =
  max_users.has_value()`; `local_passed` and `admit` refuse a full candidate, and `activate`
  re-checks; a refused peer gets `@_hsready` `{"ok":false,"err":"room full"}` (the fixture's
  `handshake_frames` entry `room full`), then leaves; its user is remembered (256 peers, LRU,
  `rejectedFullPeerUsers`) and the signaling does not dial a remembered peer while the room is
  full (`canConnectPeer`).
- Receiving `{"ok":false,"err":"room full"}` from a pending peer: `room_error` `room_full`,
  message "room full", the peer id; the peer leaves.

- [ ] **Step 1:** Failing cases in `room_test.cpp`: `a_full_room_refuses_a_new_user` (max 2, a
  third app: it reports `room_full`, the two never report it joining),
  `another_peer_of_a_present_user_is_admitted` (max 2; a second app importing a present user's
  identity joins), `concurrent_admissions_respect_the_limit` (max 2, two newcomers whose
  handshakes complete in one turn: exactly one is admitted), `a_refused_user_is_not_dialed_again`
  (no new connection to it while the room stays full), `max_users_zero_is_refused`;
  `handshake_frames_match` gains the entry `room full`.
- [ ] **Step 2 to 4:** Run (fail), implement, **run all** (PASS).
- [ ] **Step 5: Commit:** `trystero: a room bounds its users with max_users, counted by verified
  user id and refused with room_full, as 0.27.0's rooms are`.

### Task 2.5b: The remaining emitters and the internal actions 0.27.0 changed

Which room errors emit in Phase 2 (from `room.ts`, `strategy.ts` and `utils.ts`): `room_full`
(2.5a), `identity_failed` (2.2), `handshake_failed` (2.1b, 2.4b), `decrypt_failed` and
`peer_connect_failed` (Phase 1), and here `peer_error`, `banned` (a peer that bans us: a JS peer
with `roles()` sends it) and `relay_failed`'s second path. `plugin_failed` stays for Phase 3:
only a plugin's setup reports it.

**Files:**
- Modify: `room.hpp`, `detail/action_wire.hpp`, `detail/nostr_strategy.hpp`,
  `test/support/memory_backend.hpp` (a hook `fail_publish(url)`)
- Test: `test/room_test.cpp`, `test/signaling_test.cpp`, `test/crypto_layer_test.cpp`

**Interfaces:**
- `peer_error`: the action wire's `fail_peer(peer, reason)` reports a `room_error` `peer_error`
  with the reason and the peer id, then the room sends `@_leave` and drops the peer
  (`onPeerError`); a peer not in the room is dropped silently (`hasAnyPeer`).
- `banned`: `@_hsready` `{"ok":false,"err":"peer banned"[,"reason":r]}` from a pending peer:
  `room_error` `banned`, message "peer banned" or "peer banned: <r>", `reason` r when r is a
  string of at most 512 UTF-16 units, the peer id; the peer leaves.
- `relay_failed`, second path (`strategy.ts`): when no relay's first announcement succeeded and
  every relay either failed to boot or failed its first announcement, once per room (with the
  boot path, at most one report). An announcement fails when it cannot be signed or written.
- `@_ping` is an internal request action: a ping is a request of `""` answered `""` through
  `@_response` (the fixture's `ping_frames`); `@_pong` is gone; `room::async_ping` keeps its
  signature.
- An offer of `@_response` that answers no request of ours is rejected with "unexpected
  response" (0.27.0), not dropped.

- [ ] **Step 1:** Failing cases: `a_protocol_violation_reports_peer_error` (a malformed action
  frame injected from an active peer: `peer_error`, the peer leaves),
  `a_ban_from_a_peer_reports_banned` (the fixture's `handshake_frames` entry `banned`, its frame
  injected byte for byte from a peer whose handshake is under way:
  `banned`, message "peer banned: spam", reason "spam"),
  `relay_failed_when_no_first_announcement_succeeds` (`fail_publish` on every relay),
  `ping_is_a_request_action` (`crypto_layer_test.cpp`: the port's ping frames equal
  `ping_frames`; `room_test.cpp`: `async_ping` completes over the memory network),
  `an_unexpected_response_offer_is_rejected`.
- [ ] **Step 2 to 4:** Run (fail), implement, **run all** (PASS).
- [ ] **Step 5: Commit:** `trystero: a room reports peer_error, banned and every relay's failed
  first announcement, and pings as 0.27.0 pings, through a request`.

### Task 2.6a: The oracle on 0.27.0, the recorder over the published packages, the twins

**Files:**
- Modify: `test/oracle/package.json` (`@trystero-p2p/core`, `@trystero-p2p/nostr` and
  `trystero` 0.27.0; `@noble/secp256k1` 3.2.0; `werift` 0.24.4 and `ws` 8.22.0 kept),
  `package-lock.json` (regenerated by `npm install --package-lock-only`), `oracle.mjs`,
  `test/oracle/Jamfile`, `fixtures/cases/protocol.json`, `fixtures/expected/expected.json`,
  `test/protocol_test.cpp`, `test/signaling_test.cpp`, every `test/oracle/twins/*.mjs` and
  `*.expected`, `example/diff_*.cpp` (comments and printed text only where 0.27.0's behaviour
  changed)
- Test: the `oracle` lane

**Interfaces:**
- `oracle.mjs` computes, from `@trystero-p2p/core@0.27.0`'s `dist`, exactly the sections that
  `protocol_test.cpp` and `signaling_test.cpp` still read (signal texts, Nostr events and event
  messages, payload texts, the answerer, duplicate keys, numbers beyond doubles); every 0.26.0
  section (topics, `strToNum`, relays, shuffle, the password handshake, sealing, the envelope,
  the 0.26 action frames) leaves `cases/protocol.json`, `expected.json` and the tests.
- A target `vectors` in `test/oracle/Jamfile`, in the `oracle` lane: `node record_0_27.mjs
  --dist <the lane's node_modules> ../fixtures/expected/trystero-0.27.0.json` must write the
  fixture byte for byte (design D19); use the superproject's rule for a Node script compared with
  an expected file (`webcpp.cases` with a cases directory holding the fixture, if no other rule
  fits; AGENTS.md, "Jamfile API").
- Twins: each twin runs 0.27.0's API (`joinRoom` from `@trystero-p2p/nostr`, `onPeerJoin`
  returning its unsubscribe, `getPeers()` of `PeerInfo`, `getSelf()`), prints what its C++
  example prints where they agree and its own line where they differ; each `.expected` is
  recorded again from a run. `diff_relay_config` is retired with its twin and its row if 0.27.0's
  per-room configuration now prints what the C++ program prints; `diff_passwords` keeps its
  point (one password per app, per room in JS).
- [ ] **Step 1:** `npm install` in `test/oracle` (the lane's own install, as the Jamfile does it);
  run `lanes.sh clang libs/trystero/test/oracle//vectors`; expected: PASS at once (the recorder
  already supports `--dist`); a FAIL means the published dist differs from `8d3f57e`: stop and
  report it.
- [ ] **Step 2:** Rewrite `oracle.mjs` and the twins; run `lanes.sh clang
  libs/trystero/test/oracle//oracle`; expected: FAIL until the expected files are recorded again,
  then PASS.
- [ ] **Step 3: Run all**, then the `oracle` lane through `heavy.sh` on clang and emscripten;
  expected: PASS.
- [ ] **Step 4: Commit:** `trystero: the oracle is Trystero 0.27.0, the recorder proves its
  published packages equal the pinned source, and the twins speak 0.27.0`.

### Task 2.6b: Interop with Trystero 0.27.0, the lanes on

**Files:**
- Modify: `test/oracle/interop/run.mjs`, `js.html`, `cpp.html`, `forger.html`, `peer.cpp`,
  `native_peer.cpp`, `relay.mjs` (if the relay must answer 0.27.0's REQ with `since`),
  `test/oracle/Jamfile` (the `explicit` line and its comment removed), `room.hpp` and every page,
  README and AGENTS.md sentence that says "not interoperable before Phase 2" (grep `Phase 2` and
  `interoperable`)
- Create: `test/platform_page_entropy_test.cpp` (`webcpp.run platform_page_entropy`, emscripten)

**Interfaces:**
- The scenarios, each against Chrome (the browser C++ peer, `--browser`) and the native C++ peer
  (`--native`), Trystero 0.27.0 in the page:
  1. `ids_and_messages_both_ways`: each side sees the other's peer and user ids (the C++ peer
     prints `app::self_id()` and `app::user_id()`, the page `getSelf()`), and text, JSON and
     binary messages go both ways.
  2. `js_verifies_cpp_signatures`: the page's `onPeerJoin` fires for the C++ peer, which it does
     only after verifying the C++ peer's endpoint and user signatures; the C++ peer's
     `on_peer_join` likewise proves the converse.
  3. `wide_transfers_both_ways`: 1 MiB each way.
  4. `passwords`: one password meets; another gives `decrypt_failed` on the C++ side and
     `decrypt-failed` on the page, and no join.
  5. `imported_identities`: the C++ peer imports a `trystero_uk_` text the page exported
     (`createUserIdentity`, `exportUserIdentity`), and the page imports one the C++ peer
     exported; each sees the expected user id.
  6. `one_user_two_cpp_peers`: two C++ apps importing one identity and a page: the page sees two
     peers and one user.
  7. `max_users_both_ways`: a page room with `maxUsers: 2` refuses the C++ peer as a third user
     (`room_full` on the C++ side), and a C++ room with `max_users` 2 refuses a page
     (`room-full` on the page).
  8. `requests_and_pings`: a request each way, and a ping each way.
  9. `leaves`: a C++ leave is the page's `onPeerLeave` and `onUserLeave`, and the converse.
  10. `a_forged_user_signature`: `forger.html` joins with a custom identity whose `sign` passes
      0.27.0's startup check and then signs with another key: the C++ peer reports
      `handshake_failed` and never joins it.
  11. `mesh`: two pages and a C++ peer.
  12. `relay_connectivity`: offline then online, the peers meet again.
  The 0.26-only scenarios (presence before open, rooms over one connection, the room moved over a
  joined one) are removed.
- `platform_page_entropy`: a page whose `crypto.getRandomValues` is missing or throws: `app`
  construction reports `identity_failed` on its rooms and no JavaScript exception enters
  WebAssembly. If webcpp::crypto's `random_device` lets one through, the test stays, `explicit`
  with a comment naming the crypto stream's guard, and the task reports the dependency (P2-R16).
- [ ] **Step 1:** Rewrite the pages, peers and scenarios; run one scenario at a time with
  `--scenario` while writing.
- [ ] **Step 2:** Remove the `explicit` line; run `heavy.sh trystero-027 lanes.sh clang
  libs/trystero/test/oracle//interop_native` and `heavy.sh trystero-027 lanes.sh emscripten
  libs/trystero/test/oracle//interop_browser`; expected: every scenario passes. A flaky scenario is
  a defect, fixed at its cause.
- [ ] **Step 3:** Remove every "not interoperable before Phase 2" clause; run all and the
  `oracle` and `interop` lanes; expected: PASS.
- [ ] **Step 4: Commit:** `trystero: C++ peers meet Trystero 0.27.0 peers in Chrome and in a
  process, on every interop scenario, and the oracle and interop lanes are on`.

### Task 2.7: The pages whole, the changelog, and the merge

**Files:**
- Modify: `README.md`, `AGENTS.md`, every `doc/*.adoc` (`overview`, `quick-start`, `usage`,
  `design`, `protocol`, `guarantees`, `allocation`, `differences`, `platform`, `testing`,
  `building`, `roadmap`, `acknowledgements`, `changelog`), the superproject's
  `CODING-GUIDELINES.md` chapter 15 (trystero's section), this change's `reference/STATUS.md`
  (by the controller)

- [ ] **Step 1:** Write the pages whole for 0.27.0: the design page's connection per room (D9),
  identity (D17, D18), the handshake and its fingerprints (D20); the protocol page's identity,
  handshake, `@_hsdata`, `@_hsready`, `@_ping` and wide chunks; usage's identities, `max_users`,
  every room error with its emitter; differences: added rows for D17 (identity per app, no
  `'local'`), D18 (deterministic signatures, strict base64url of signatures and key texts where
  0.27.0 decodes loosely), no media, `requestMany` and the progress and `signal` of sends not
  ported, the 64-concurrent-requests and 128/8 decision bounds not reached by synchronous gates;
  rows removed for the shared connection, `diff_rooms_per_app` and what 2.6a retired; the
  acknowledgements name Trystero 0.27.0 and webcpp::crypto's P-256 (BearSSL); the migration
  section (design's "Migration Plan") in usage and README.
- [ ] **Step 2:** The changelog: `doc/changelog.adoc`, "Unreleased", is the repository's
  mechanism (superproject AGENTS.md chapter 5: agents write a library's lines there, and the
  release notes `doc/releases/<version>.adoc` include its `tag=unreleased`); there is no
  `CHANGELOG.md` to touch. Add the port's lines, at most ten for trystero in all, the most
  important first, merging Phase 1's lines where the count would pass ten: `*Breaking change:*
  Changed the wire to Trystero 0.27.0's ...`, the identity, `max_users`, the removed options and
  errors.
- [ ] **Step 3:** Merge `origin/main` into both branches; check the parked merge blockers:
  R20 (`b2 libs/trystero/doc` builds: webcpp::hash's MrDocs `config.hpp`) and R21 (the lint's
  clang-tidy on webcpp::crypto's `backends/secp256k1.hpp`); if either is still open, stop and
  report BLOCKED with the failing command. Run the lanes, each through `heavy.sh`: native clang,
  gcc-15, Emscripten (all three: `libs/trystero/test libs/trystero/example libs/trystero/doc`),
  the oracle and interop lanes, the macOS lint, the Linux lint in Docker, MSVC 14.3 and 14.5 (the
  local MSVC lane); expected: all green but MSVC's known Wine artifacts (the platform units and
  the exception-free Boost.JSON and `shared_test` targets, the latter gone with 2.1a).
  Ruling R19 stays: webcpp::crypto still has no public wipe of a variable-size buffer, so
  trystero keeps its `detail::wipe` (a dependency of the crypto stream, after the merge).
- [ ] **Step 4: Commit** (library and superproject): `trystero: the pages say what the port to
  Trystero 0.27.0 is, whole, and the changelog has its lines`.
- [ ] **Step 5 (controller):** push both branches and the proposals branch; take
  webcpporg/trystero#10, webcpporg/webcpp#51 and webcpporg/proposals#28 out of draft; when their
  CI is green, merge each with a merge commit, the library first, then the superproject's
  pointer, then the proposals; record the merge commits in `STATUS.md`.

## Phase 3 (outlined; after Phase 2 merges, each a change of its own on `main`)

- **3.1 Signed records and shared state.** `signed-record.ts`, `state.ts`, `anti-entropy.ts`:
  `make_state` and `make_user_state` (JSON values through webcpp::document), late joiners,
  `validate`. Needs P-256 sign and verify only.
- **3.2 Roles and moderation.** `authority.ts`: `roles(...)` with `self_as_admin` (the
  `~<userId>` room id), `admins`, role certificates (`RoleCertificate` v1), `grant_role`,
  `revoke_role`, `ban`, `unban`, role-gated actions and state.
- **3.3 scale().** `scale/*`: topology, routing, descriptors with `verifyRoomAuth`,
  `crypto-frame.ts`'s pairwise keys (`derivePairwiseAesKey`: P-256 `ecdh_x`, HKDF with the
  `e2e-salt` and `e2e-info` v2 tags), multi-hop data plane. Needs crypto Task 3.3 (`ecdh_x`).
