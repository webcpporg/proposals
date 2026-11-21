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
errors, users and peers, self, relay connectivity). Phase 2 (outlined) adds identity and the
dual-signature handshake on webcpp::crypto's P-256 (its foundation increment 3). Phase 3 (outlined)
adds the plugins: signed state, roles, then `scale()`.

**Tech Stack:** C++20, header-only; webcpp::hash (SHA-256, HMAC-SHA-256, HKDF, PBKDF2, async
PBKDF2 over WebCrypto on Emscripten); webcpp::crypto (`aes256_gcm`, `random_device`,
`curves::secp256k1` BIP-340; in Phase 2 `curves::p256`); webcpp::asio, webcpp::webrtc,
webcpp::beast, webcpp::document; b2; Node for the vectors and the interop lane.

**Spec:** `../proposal.md`, `../design.md` (decisions D1 to D16), `../specs/trystero/spec.md`.

## Global Constraints

- Upstream pin: Trystero 0.27.0, `dmotz/trystero` commit `8d3f57e` (merge of PR #212), its
  `packages/core/src` and `packages/nostr/src`. Every wire value is proven against that code.
- Vectors: `test/fixtures/expected/trystero-0.27.0.json`, a copy of this change's
  `reference/vectors/phase1.json`, regenerated only by `test/oracle/record_0_27.mjs` (a copy of
  `reference/vectors/record.mjs`) run against that pin: `node test/oracle/record_0_27.mjs <checkout>`.
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
- Create: `test/fixtures/expected/trystero-0.27.0.json` (copy of `reference/vectors/phase1.json`),
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

## Phase 2 (outlined; written in full when crypto's increment 3 P-256 part merges)

Depends on webcpp::crypto `curves::p256` (`<webcpp/crypto/curves/p256.hpp>`):
`keypair::generate()`, `keypair::make(scalar)`, `keypair::secret_scalar()`,
`keypair::public_key().uncompressed()`, `public_key::make(65 bytes)`, `keypair::sign_ecdsa(digest)`,
`verify_ecdsa(signature, digest, key)`, `normalize_ecdsa`, `check_low_s` (crypto's plan,
Tasks 3.1 and 3.2). Each task records its vectors in the recorder first; P-256 signatures are
random in WebCrypto, so vectors fix keys and check that each side verifies the other's
signatures, never signature bytes.

- **2.1 The connection model.** Read 0.27.0's `peer.ts`, `peer-transport.ts`, `room.ts` and
  `strategy.ts` and record the frames of one room's data channel. 0.27.0 has no connection shared
  across rooms (0.26.0's `shared-peer.ts` is gone); the default ruling (design D9) is one
  connection per room and remote peer, the app-level sharing of the change `shared-connection`,
  its presence frames and `room_token` retired. The task confirms or amends D9 before 2.2.
- **2.2 Endpoint and user keys.** `detail/identity.hpp`: an endpoint key pair per app (`peerId`
  from its compact key, Task 3's `derive_peer_id`), a user key pair (`userId`), the
  `user-proof` binding (`domainTag('user-proof', 3)`), `identity` option: `ephemeral` (default),
  `imported(user_identity)`; `create_user_identity()`, `export_user_identity()` (0.27.0's
  `trystero_uk_` text, vectors from `exportUserIdentity`) and `import_user_identity()`; no
  implicit persistence (design D15). Needs P-256 `generate`, `make`, `secret_scalar`.
- **2.3 Signed key records and their cache.** `getVerifiedPubKeyRecord`, `verifyWithP256` with
  low S required (`check_low_s` before `verify_ecdsa`), signing with `normalize_ecdsa`; the
  bounded signature cache (8,192 entries, 2 MiB).
- **2.4 The dual-signature handshake.** `handshake.ts`: init and response with UUID nonces
  (Task 3), both endpoint and user signatures over a payload that binds app, room and both
  SDP DTLS fingerprints; `handshake_failed` on any mismatch; the signal's 0.27.0 fields.
- **2.5 Users for real.** The user table fed by verified user ids; `max_users` (`room_full`,
  refresh-safe per user id); multi-endpoint users (several apps of one user identity in one or
  several processes are "tabs").
- **2.6 Interop.** The oracle lane pinned to 0.27.0 (the npm packages `trystero@0.27.0` and
  `@trystero-p2p/core@0.27.0`), the twins rewritten, the interop scenarios against Chrome on
  both lanes, turned back on; vectors regenerated and byte-identical.
- **2.7 The merge.** Docs whole (README, every page, chapter 15), the changelog's entry, one
  merge of `port/trystero-0.27` into trystero `main`, the superproject's pointer, both CIs green
  (design D1).

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
