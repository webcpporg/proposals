---
title: "webcpp::crypto, the API gaps of version 0.2: Implementation Plan"
tags: [webcpp, crypto, gcm, aes, secp256k1, dudect, ci, plan]
status: accepted
created: 2026-11-20
---

# webcpp::crypto, the API gaps of version 0.2: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** close the six gaps the owner approved on 2026-11-20: GCM's failed seal zeroes its
data; GCM opens incrementally in two passes, releasing nothing before the tag; the AES block
cipher has a default on every target; secp256k1's key pair is generated; `pubkey_negate`
returns a result; dudect is printed, warned on and judged on hosted runners by two of three
fresh measurements, with an A/A control and the evidence of every high value.

**Architecture:** every library change is in the families' headers (`ciphers/gcm.hpp`,
`ciphers/aes.hpp`, `curves/secp256k1.hpp`, `curves/secp256k1/tweak.hpp`), the generated tables
(`doc/tables/crypto.json` through `generate.py`) and the constant-time harness
(`test/constant_time/`); the CI's judgment is one Python script in the library, run by one new
step of the superproject's `library.yml`. No backend's engine changes.

**Tech Stack:** C++20 header-only, b2, Boost.Core's lightweight_test, the superproject's lanes
(native clang and gcc-15, wasip2, wasip3, Emscripten, MSVC 14.3 and 14.5 under Wine), Python 3.

**Spec:** `../proposal.md`, `../design.md` (D1 to D8, binding), `../specs/crypto/spec.md`,
`../tasks.md`; and, merged before this plan starts, the change `data-independent-timing`
(`../../data-independent-timing/`), whose guard, seam and coverage tests every task extends.
Executors read all of them.

Paths are relative to the library, `libs/crypto/` of the superproject (its own repository
`webcpporg/crypto`), unless they start with `webcpp/` (the superproject) or `proposals/`.
`$B2` is `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
--user-config=.local/user-config.jam`, run from the superproject's root.

## Global Constraints

- Everything written to a file, a commit or a pull request is in English; no em dash (U+2014)
  anywhere; the banned word of `CODING-GUIDELINES.md` chapter 2 nowhere.
- Commits: `git -c user.name=... -c user.email=... commit -- <paths>` with explicit paths,
  subject `crypto: ...` (library), `ci: ...` or `tools: ...` (superproject), a descriptive
  sentence like the history, no trailer of any kind.
- Every new source file opens with the licence notice of chapter 2; every public or detail
  symbol carries its doc comment (chapter 7); `.clang-format` and `.clang-tidy` clean.
- Never edit a generated file (`doc/generated/*`, `doc/llms.txt`, `detail/capabilities.hpp`,
  `detail/defaults.hpp`, `build.jam`'s defaults block, `doc/performance.*`,
  `test/capabilities_test.cpp`): change `doc/tables/crypto.json` or `generate.py` and run
  `python3 doc/tables/generate.py`, then `python3 doc/tables/generate.py --check`.
- Every new public function starts with `detail::timing_guard const timing;` and is called by
  its backend's timing coverage test, valid input and input refused before the backend (design
  D8).
- Every test is written first and seen failing. Sizes and speed come from release builds only.
- The threshold is 10; the hosted-runner rule is "at least two of three values >= 10" (D7).
- The incremental open's scratch block is 256 bytes; the generate loop draws at most 4 times.

## Review Focus

1. **A ciphertext that changes between the open's two passes.** A reasonable user expects
   that the second pass never decrypts more than was authenticated. Pinned by Task 2's
   `incremental_open_longer_second_pass` (refused, piece untouched) and
   `incremental_open_shorter_second_pass` (`end_open` refuses); the change of content itself
   cannot be detected, which the documentation says (D2).
2. **The caller's ciphertext in the first pass.** `authenticate` takes a `const` span; a user
   who authenticates a buffer that is also its output expects it unchanged. Pinned by Task 2's
   `incremental_open_wrong_tag`, which compares the ciphertext after `verify` fails.
3. **A one-shot call or a `begin_seal` in the middle of an open**, and a move during it. Pinned
   by Task 2's `incremental_open_interleaved` and its move check on the mock.
4. **`aes256<>` and `gcm<aes256<>>` in one translation unit on WebAssembly**, a program that
   links only one of ctaes and BearSSL. Pinned by Task 3's `default_aes_block_wasm` and the two
   message tests.
5. **A dudect output that holds no case line** (a program that crashed, or a planted run).
   The judge must not count it as a pass silently. Pinned by Task 7's
   `judge_output_without_case_lines` (exit 1, naming the file) and `judge_skips_planted`.

---

### Task 1: GCM's failed seal zeroes `data` (design D1)

**Files:**
- Modify: `include/webcpp/crypto/ciphers/gcm.hpp` (`seal`, `update` and their doc comments)
- Modify: `test/gcm_mock_test.cpp` (`check_one_shot_failures`, `check_incremental_failures`)
- Modify: `doc/aead.adoc` (the bullet "A failed `open` zeroes `data_bytes`" becomes "A failure
  zeroes `data_bytes`", covering `seal` and `update`)

**Interfaces:** unchanged signatures.

- [ ] **Step 1: The failing tests.** In `check_one_shot_failures`, the seal loop fills `data`
  with `0x5A` before each call and adds `BOOST_TEST(webcpp::test::all_zero(data));` after the
  failed `seal`, for each of the knobs `start`, `run`, `tag`. A new block: `mock_control.start
  = failed`, `seal` with a 129-byte IV over `0x5A` data returns `invalid_iv_size` and `data` is
  still all `0x5A`. In `check_incremental_failures`, the `update` whose `run` fails is given
  `0x5A` data and `BOOST_TEST(webcpp::test::all_zero(data));` follows; an `update` refused with
  `message_too_long` (the mock's `message_bytes_` path, a piece of `max_message_size + 1`
  bytes of `webcpp::test::reserved` natively, as the existing limit test) leaves its piece
  untouched.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//gcm_mock`; expected: FAIL on the new `all_zero`
  checks.
- [ ] **Step 3: Implement.** In `seal`, on `!tag_result`, call
  `detail::wipe(data_bytes.data(), data_bytes.size());` before `engine::abandon(state_)`; in
  `update`, on `!run_result`, the same wipe of the piece before `abandon()`. Doc comments: "When
  the backend fails, `data_bytes` is zeroed, as a failed open's is; a guard's refusal leaves it
  untouched."
- [ ] **Step 4: Run** `gcm_mock`, `gcm_bearssl`, `gcm_openssl`, `gcm_limits_mock`, natively and
  with `toolset=gcc-15`; expected: PASS.
- [ ] **Step 5: Commit** `crypto: a failed GCM seal zeroes its data, as the other AEADs do`.

### Task 2: GCM's incremental open, in two passes (design D2)

**Files:**
- Modify: `include/webcpp/crypto/ciphers/gcm.hpp` (the phase enumeration, the kept IV, five
  members)
- Modify: `test/support/gcm_checks.hpp` (a new `check_incremental_open<Backend>()`, called by
  `gcm_bearssl_test.cpp`, `gcm_openssl_test.cpp`; `check_machine` gains the open's states)
- Modify: `test/gcm_mock_test.cpp` (engine failures during the open, the move)
- Modify: `test/support/timing_coverage.hpp` (`cover_gcm_messages` covers the five, and
  `cover_gcm_moves` a move during `begin_open`)
- Modify: `test/support/mock_backend.hpp` only if its `gcm_engine` needs a counter of `start`
  calls (`mock_control.starts`) for the second start's check
- Create: `example/opened_in_two_passes.cpp`, `example/opened_in_two_passes.expected`;
  modify `example/Jamfile` (`webcpp.example opened_in_two_passes.cpp :
  <library>/webcpp/crypto//gcm-default ;`)
- Modify: `doc/aead.adoc` (the bullet "No incremental `open`" and the section
  `crypto-gcm-incremental` gain `[#crypto-gcm-incremental-open]` "Opening what does not fit in
  memory"), `doc/tables/crypto.json` (the `gcm` operation's `functions` gain `begin_open`,
  `authenticate`, `verify`, `decrypt`, `end_open`)

**Interfaces:**
- Produces, members of `ciphers::gcm<Cipher>`, all `noexcept`, `[[nodiscard]]`:
  - `boost::system::result<void> begin_open(std::span<std::uint8_t const> iv_bytes, std::span<std::uint8_t const> additional_data_bytes)`
  - `boost::system::result<void> authenticate(std::span<std::uint8_t const> ciphertext_piece)`
  - `boost::system::result<void> verify(std::span<std::uint8_t const> tag_bytes)`
  - `boost::system::result<void> decrypt(std::span<std::uint8_t> data_piece)`
  - `boost::system::result<void> end_open()`
- Private state: `enum class phase : std::uint8_t { none, sealing, authenticating, decrypting };`
  `phase phase_`, `std::array<std::uint8_t, max_iv_size> iv_`, `std::size_t iv_size_`,
  `std::uint64_t message_bytes_` (sealed or authenticated), `std::uint64_t decrypted_bytes_`;
  `in_message_` becomes `phase_ != phase::none`.

- [ ] **Step 1: The failing tests,** in `gcm_checks.hpp`, each over the 18 specification cases
  where a case is named, else case 14 (`cases[13]`):
  - `incremental_open_pieces`: for each case, seal one shot; for piece sizes 1, 7, 16, 17 and
    the whole, `begin_open`, `authenticate` each piece, `verify(tag)`, then `decrypt` in pieces
    of 5 and the rest, `end_open()`: every call has a value, the plaintext equals the case's.
  - `incremental_open_wrong_tag`: tag bit 0 of byte 0 flipped: `verify` gives
    `authentication_failed`; the ciphertext buffer equals its copy; `decrypt` then gives
    `invalid_state`; `end_open` gives `invalid_state`; a fresh `begin_open` works.
  - `incremental_open_decrypt_before_verify`: `decrypt` after `authenticate` gives
    `invalid_state`, its piece untouched; `verify` then gives `invalid_state` (the message
    ended).
  - `incremental_open_longer_second_pass`: authenticate 32 bytes, verify, `decrypt` 33 bytes:
    `invalid_input_size`, piece untouched, message ended.
  - `incremental_open_shorter_second_pass`: authenticate 32, verify, decrypt 31, `end_open`:
    `invalid_input_size`.
  - `incremental_open_guards`: `begin_open` with an empty IV (`invalid_iv_size`), with 129
    bytes (`invalid_iv_size`); `verify` with 15 bytes (`invalid_tag_size`, message ended);
    `authenticate` without a message (`invalid_state`); natively, `authenticate` of
    `max_message_size + 1` bytes of `webcpp::test::reserved` (`message_too_long`).
  - `incremental_open_interleaved`: during an open, one-shot `seal` and `open` give
    `invalid_state` and the open still completes; `begin_seal` during it gives `invalid_state`
    and ends it; `update` and `result` during it give `invalid_state`; `begin_open` during a seal
    gives `invalid_state` and ends the seal.
  - `incremental_open_moved`: a `gcm` moved during `authenticate` leaves the destination with no
    message (`verify` gives `invalid_state`) and a fresh open on it works.
  In `gcm_mock_test.cpp`: with `run` failing during `authenticate`, the message ends, the
  caller's piece is unchanged and `abandons` grows by one; with `run` failing during `decrypt`,
  the piece is all zero; with `check` failing, `verify` returns that error and the message
  ends. `check_machine` gains the eight transitions above as rows. Call
  `check_incremental_open<bearssl>()`, `<openssl>()` and the mock's.

  The shape of each check, here the wrong tag's:

```cpp
template <class Backend>
void incremental_open_wrong_tag(gcm_cases cases = gcm_spec_cases) {
    auto const& c = cases[13];
    with_gcm<Backend>(from_hex(c.key), [&](auto& made) {
        auto const iv = from_hex(c.iv);
        auto ciphertext = from_hex(c.ciphertext);
        auto const kept = ciphertext;
        auto tag = from_hex(c.tag);
        tag[0] ^= 1U;
        BOOST_TEST(made.begin_open(iv, {}).has_value());
        BOOST_TEST(made.authenticate(ciphertext).has_value());
        BOOST_TEST(made.verify(tag).error() == webcpp::crypto::error::authentication_failed);
        BOOST_TEST(ciphertext == kept);
        BOOST_TEST(made.decrypt(ciphertext).error() == webcpp::crypto::error::invalid_state);
        BOOST_TEST(ciphertext == kept);
        BOOST_TEST(made.end_open().error() == webcpp::crypto::error::invalid_state);
        BOOST_TEST(made.begin_open(iv, {}).has_value());
    });
}
```
- [ ] **Step 2: Run** `$B2 libs/crypto/test//gcm_bearssl libs/crypto/test//gcm_mock`;
  expected: compile errors naming `begin_open`.
- [ ] **Step 3: Implement** the five members in `gcm.hpp`, each starting with its timing guard:
  - `begin_open`: `!keyed_` gives `invalid_state`; a message in progress is abandoned and gives
    `invalid_state`; `guard_message(iv, ad, 0, tag_size)`; copy the IV into `iv_`;
    `engine::start(state_, false, iv, ad)`, on failure `engine::abandon` and its error; phase
    `authenticating`, counters zero.
  - `authenticate`: phase check; `detail::exceeds(piece.size(), max_message_size -
    message_bytes_)` abandons and gives `message_too_long`; then, for each block of at most 256
    bytes, copy into `std::array<std::uint8_t, 256> scratch`, `engine::run(state_,
    std::span(scratch).first(n))`, and `detail::wipe(scratch.data(), n)`; a failure abandons
    and returns it; add the size to `message_bytes_`.
  - `verify`: phase check; `tag_bytes.size() != tag_size` abandons and gives
    `invalid_tag_size`; copy into a local `std::array<std::uint8_t, tag_size>`;
    `engine::check`; on failure abandon and return it; on success `engine::abandon(state_)`,
    then `engine::start(state_, false, std::span(iv_).first(iv_size_), {})` (failure: abandon,
    error), phase `decrypting`, `decrypted_bytes_ = 0`.
  - `decrypt`: phase check (`invalid_state` ends a message in another phase, except `none`,
    which only answers); `piece.size() > message_bytes_ - decrypted_bytes_` abandons and gives
    `invalid_input_size`; `engine::run` in place; failure: wipe the piece, abandon, error; add
    to `decrypted_bytes_`.
  - `end_open`: phase check; `decrypted_bytes_ != message_bytes_` abandons and gives
    `invalid_input_size`; else `abandon()` (which wipes the message's state through the engine
    and ends it) and `{}`.
  `end_message()` wipes `iv_` and zeroes the counters. The class comment gains: "The
  incremental open reads the ciphertext twice: `authenticate` every piece, `verify` the tag,
  then `decrypt` the same pieces; no plaintext reaches the caller before the tag matched. The
  second pass must read the bytes the first read."
- [ ] **Step 4: Coverage.** `cover_gcm_messages` gains, in order: `begin_open` (value),
  `authenticate` of `data` (value), `verify` of the seal's tag (value), `decrypt` of `data`
  (value), `end_open` (value), then `begin_open` with an empty IV (`invalid_iv_size`),
  `authenticate`, `verify`, `decrypt`, `end_open` without a message (`invalid_state` each);
  `cover_gcm_moves` gains "gcm(gcm&&) during begin_open". Each check's second argument is
  `"gcm::<member>"`.
- [ ] **Step 5: Run** `gcm_bearssl gcm_openssl gcm_mock timing_coverage_bearssl
  timing_coverage_openssl` natively, gcc-15, wasip2, wasip3 (OpenSSL's there too) and
  Emscripten (BearSSL's), exceptions on and off as `crypto-run` builds them; expected: PASS.
- [ ] **Step 6: The example and the page.** `example/opened_in_two_passes.cpp`: a 1 MiB file's
  bytes in a `std::vector` read in pieces of 64 KiB (the test writes it to a temporary file and
  reads it back twice with `std::ifstream`), sealed incrementally, then opened with the five
  calls, printing `authenticated 1048576 bytes`, `tag verified`, `decrypted 1048576 bytes`,
  `equal: yes`, which is its `.expected`. The page's new section: when to use it (a file on the
  program's own disk, too large for memory), the two passes and their cost, the rule that the
  second pass reads the same bytes (and why: a changed byte is decrypted unauthenticated, and
  GCM lets it be chosen), and records for a stream that cannot be read twice; the bullet "No
  incremental `open`" becomes "No plaintext before the tag", pointing to it. Run
  `python3 doc/tables/generate.py` and `--check`, and `$B2 libs/crypto/doc` (doc-check clean).
- [ ] **Step 7: Commit** `crypto: GCM opens incrementally in two passes, authenticating every
  piece before it decrypts any`.

### Task 3: The AES block cipher's default per target (design D3)

**Files:**
- Modify: `doc/tables/crypto.json` (the family `aes_block`'s `default` rows; the operation
  `aes_block`'s `default` text and its `"default_of": "gcm"` removed)
- Modify: `doc/tables/generate.py` (the docstring it writes for `default_backend`, and anything
  that special-cases `default_of`); `doc/tables/generate_test.py`
- Regenerate: `include/webcpp/crypto/detail/defaults.hpp`, `detail/capabilities.hpp`,
  `build.jam`'s defaults block, `doc/generated/*`, `doc/llms.txt`
- Modify: `include/webcpp/crypto/config.hpp` (the tag `backends::family_default`, beside the
  other backend tags), `ciphers/aes.hpp` (the resolver, the default arguments, the default's
  header), `ciphers/gcm.hpp` (its backend); no backend header changes
- Modify: `test/default_backend_test.cpp`, `test/default_selection_test.cpp`,
  `test/default_both_test.cpp`, `test/default_both_wasm_test.cpp`,
  `test/default_bearssl_test.cpp`, `test/Jamfile`
- Create: `test/default_aes_block_test.cpp`; `test/messages/aes_block_default_not_linked/`
  (`Jamfile`, `aes_block_default_not_linked.cpp`, native, ctaes alone) and
  `test/messages/aes_block_default_not_linked_wasm/` (wasip2, OpenSSL alone); their MSVC
  fixture `test/msvc/aes_block_default_not_linked/`
- Modify: `doc/block.adoc` (the paragraph on `aes256<>`), `doc/choosing_backend.adoc:36`,
  `doc/jamfile.adoc:38`

**Interfaces:**
- Produces: `namespace webcpp::crypto::backends { struct family_default; }` in `config.hpp`
  (declared, never defined: a name only, documented as "the default backend of whichever family
  takes it"); `namespace webcpp::crypto::detail { template <class Backend, class
  Family> struct resolved_backend { using type = Backend; }; template <class Family> struct
  resolved_backend<backends::family_default, Family> { using type = default_backend_t<Family>;
  }; template <class Backend, class Family> using resolved_backend_t = typename
  resolved_backend<Backend, Family>::type; }` (in `ciphers/aes.hpp`, which includes
  `detail/defaults.hpp`).
- `template <class Backend = backends::family_default> class aes128` (and `aes192`, `aes256`);
  `basic_aes<KeySize, Backend>::backend_type` is `detail::resolved_backend_t<Backend,
  families::aes_block>`; new `basic_aes<KeySize, Backend>::named_backend_type` is `Backend`.
- `gcm<Cipher>::backend_type` is `detail::resolved_backend_t<typename
  Cipher::named_backend_type, families::gcm>`.
- Generated: `default_backend_of<families::aes_block>` (OpenSSL natively, ctaes on WebAssembly),
  `WEBCPP_CRYPTO_DETAIL_DEFAULT_AES_BLOCK_OPENSSL` / `_CTAES`, the alias
  `/webcpp/crypto//aes_block-default`.

- [ ] **Step 1: The failing tests.** `default_aes_block_test.cpp` (built natively with OpenSSL,
  and on wasip2, wasip3, Emscripten with ctaes and BearSSL linked):
  `static_assert(std::is_same_v<crypto::ciphers::aes256<>::backend_type, block_expected>)`,
  `block_expected` OpenSSL natively and ctaes on WebAssembly;
  `static_assert(std::is_same_v<crypto::ciphers::aes256_gcm::backend_type, gcm_expected>)` and
  the same for `gcm<aes256<>>` and `aes128_gcm`; at run time `aes256<>::make(key)` and
  `encrypt` of FIPS-197's C.3 block give `8e a2 b7 ca 51 67 45 bf ea fc 49 90 4b 49 60 89`, and
  `aes256_gcm::make(key)` has a value. `default_backend_test.cpp`: `!has_default<aes_block>`
  becomes `std::is_same_v<default_backend_t<families::aes_block>, block_expected>`.
  `default_selection_test.cpp:35` asserts `aes256<>::backend_type` and
  `gcm<aes256<>>::backend_type` instead of the type identity. `default_both_wasm_test.cpp`,
  `default_bearssl_test.cpp`: `aes256<>::backend_type::name` is `"ctaes"`. The message tests:
  natively with ctaes alone, `aes256<>::make` stops with the generated "AES block cipher's
  default backend on this target is OpenSSL, which is not linked ... as in
  ciphers::aes256<backends::ctaes> ..." text (copy the exact text from the regenerated
  `capabilities.hpp` once Step 3 ran, as the existing `gcm_default_openssl_not_linked` does);
  on wasip2 with OpenSSL alone, the ctaes one.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//default_aes_block libs/crypto/test//default_backend`;
  expected: FAIL (no default for `aes_block`).
- [ ] **Step 3: The table.** `crypto.json`'s family `aes_block` `default`:
  `{"targets": ["native"], "compilers": ["clang", "gcc"], "backend": "openssl", "tests":
  ["default_aes_block", "default_backend_all_linked"], "why": "OpenSSL's assembly uses the
  CPU's AES instructions on x86-64 and ARMv8 ({openssl-armv8}); ctaes is bitsliced, constant
  time and several times slower. OpenSSL whatever the program links: a program that links ctaes
  alone names `aes256<backends::ctaes>`"}`, the MSVC row with GCM's MSVC note, and
  `{"targets": ["wasip2", "wasip3", "emscripten"], "backend": "ctaes", "tests":
  ["default_aes_block", "default_both_wasm"], "why": "constant time by design ({ctaes}), where
  OpenSSL is built no-asm and its AES is table-based ({openssl-install}), and webcpp does not
  build OpenSSL for Emscripten"}`; the operation's `default` text: "`aes128<>`, `aes192<>` and
  `aes256<>` name the default backend; `gcm<aes256<>>` names AES-GCM's." Run
  `python3 doc/tables/generate.py`; the generated files gain the default, its refusals and its
  alias. `generate_test.py` gains a case for a family whose default differs by target from the
  family it is a cipher of.
- [ ] **Step 4: The tag.** In `aes.hpp`: the declarations of the Interfaces block; the default
  arguments; `backend_type` and `named_backend_type`; `block_state` and `block_engine` take
  `backend_type`; at the end of the file, `#if defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_AES_BLOCK_OPENSSL)
  && defined(WEBCPP_CRYPTO_HAS_OPENSSL)` include `backends/openssl.hpp`, `#elif
  defined(WEBCPP_CRYPTO_DETAIL_DEFAULT_AES_BLOCK_CTAES) && defined(WEBCPP_CRYPTO_HAS_CTAES)`
  include `backends/ctaes.hpp`. In `gcm.hpp`: `using backend = detail::resolved_backend_t<typename
  Cipher::named_backend_type, families::gcm>;`. Doc comments of `aes128`, `aes192`, `aes256`:
  "The backend; by default the block cipher's own, OpenSSL natively and ctaes on WebAssembly."
- [ ] **Step 5: Run** every default test, `aes_openssl`, `aes_ctaes`, `aes_mock`,
  `agreement_aes`, `gcm_*`, the message tests and `tools/test/crypto_jamfiles_test.py`
  (`webcpp/`), on all five lanes; the MSVC fixture under the MSVC lane; expected: PASS.
- [ ] **Step 6: The pages.** `block.adoc`: "`aes256<>` names the block cipher's default, OpenSSL
  natively and ctaes on WebAssembly (<<crypto-backends-defaults>>); `aes256_gcm` and
  `gcm<aes256<>>` name AES-GCM's." The two other pages' sentences likewise. Regenerate,
  `--check`, doc build.
- [ ] **Step 7: Commit** `crypto: the AES block cipher has a default on every target, OpenSSL
  natively and ctaes on WebAssembly, and aes256<> names it`.

### Task 4: `secp256k1::basic_keypair::generate` (design D4)

**Files:**
- Modify: `include/webcpp/crypto/curves/secp256k1.hpp` (`generate`, after `make`)
- Create: `test/secp256k1_generate_test.cpp`; modify `test/Jamfile` (`crypto-run
  secp256k1_generate : secp256k1_generate_test.cpp : <library>/third_party//secp256k1
  <define>WEBCPP_CRYPTO_DETAIL_TEST_ENTROPY ;`)
- Modify: `test/timing_coverage_secp256k1_test.cpp` (two checks), `test/secp256k1_test.cpp`
  (a generated pair signs and verifies with the system's source), `doc/tables/crypto.json` (the
  operation's `functions` gain `generate`), `doc/secp256k1.adoc` (the key pair's list)

**Interfaces:**
- Produces: `[[nodiscard]] static boost::system::result<basic_keypair> generate(std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());`

- [ ] **Step 1: The failing tests,** in `secp256k1_generate_test.cpp` with a planted
  `webcpp_test_entropy` that answers from a script of byte values (as
  `secp256k1_musig_entropy_test.cpp`'s):
  - `generate_redraws`: script `0xFF, 0x01, 0x07` (each fills its whole request): `generate()`
    has a value whose `xonly_public_key()` equals `keypair::make(32 x 0x01)`'s; three calls of
    the source.
  - `generate_four_refusals`: script `0xFF` four times: `entropy_unavailable`; four calls.
  - `generate_source_fails`: the source fails at once: `entropy_unavailable`, one call; a
    counting resource (`test/support/counting.hpp`) records no allocation.
  - `generate_resource`: with the script `0x01, 0x07` and a counting resource, one allocation
    of `basic_keypair::context_size()` bytes from that resource.
  The redraw check:

```cpp
void generate_redraws() {
    script = {0xFF, 0x01, 0x07};  // not a secret key, the secret key, the context's seed
    calls = 0;
    auto const generated = secp::keypair::generate();
    BOOST_TEST_EQ(calls, 3U);
    std::array<std::uint8_t, 32> expected_secret{};
    expected_secret.fill(0x01);
    script = {0x07};
    auto const expected = secp::keypair::make(expected_secret);
    if (BOOST_TEST(generated.has_value()) && BOOST_TEST(expected.has_value())) {
        BOOST_TEST(*generated->xonly_public_key() == *expected->xonly_public_key());
    }
}
```
- [ ] **Step 2: Run** `$B2 libs/crypto/test//secp256k1_generate`; expected: FAIL (no member
  `generate`).
- [ ] **Step 3: Implement** `generate`: its timing guard; `for (int draw = 0; draw < 4;
  ++draw)`: a `secret<32> secret_key;`, `random_device().fill(secret_key.writable())` (failure:
  `entropy_unavailable`); `auto made = make(secret_key.bytes(), memory_resource);` returns
  unless `made.error() == error::invalid_secret_key`; after the loop `entropy_unavailable`. The
  include of `random_device.hpp` carries the `lint-world` note ed25519's has.
- [ ] **Step 4: Coverage:** `covered("secp256k1::basic_keypair::generate on secp256k1",
  "basic_keypair::generate", ...)` with the system's source has a value; the program is not built
  with the seam, so the refused path is covered by `generate_four_refusals` alone.
- [ ] **Step 5: Run** the new test, `secp256k1`, `timing_coverage_secp256k1` on all five
  lanes; expected: PASS. Regenerate and check the tables.
- [ ] **Step 6: Commit** `crypto: a secp256k1 key pair is generated from the system's source,
  drawn again for 32 bytes that are not a secret key`.

### Task 5: `pubkey_negate` returns a result; ECDH's types pinned (design D5)

**Files:**
- Modify: `include/webcpp/crypto/curves/secp256k1/tweak.hpp` (`pubkey_negate`)
- Modify: `test/secp256k1_tweak_test.cpp:67-72, 204, 215, 363`,
  `test/secp256k1_allocation_test.cpp:249`, `test/secp256k1_silent_payments_test.cpp:569, 698`,
  `test/timing_coverage_secp256k1_test.cpp:808-809`, `test/secp256k1_ecdh_test.cpp` (the pin),
  `doc/secp256k1.adoc:320`

**Interfaces:**
- Produces: `template <class Backend = default_backend_t<families::secp256k1>> [[nodiscard]] boost::system::result<public_key> pubkey_negate(public_key const& parsed_key) noexcept;`

- [ ] **Step 1: The failing tests.** `secp256k1_tweak_test.cpp:67`: the `static_assert` names
  `boost::system::result<secp::public_key>`; the uses read `.value()` after
  `BOOST_TEST(negated.has_value())`; `negate_twice`: P negated twice has values and equals P.
  `secp256k1_ecdh_test.cpp`: `static_assert`s that `ecdh`, `ecdh_x` and `ecdh_point` of
  `secp::keypair const&` with a `std::span<std::uint8_t const>` return
  `boost::system::result<webcpp::crypto::secret<32>>`, `...secret<32>` and `...secret<65>>`.
- [ ] **Step 2: Run** `$B2 libs/crypto/test//secp256k1_tweak`; expected: FAIL at the
  `static_assert`.
- [ ] **Step 3: Implement:** keep the guard, the refusal, the canary and the self-test; `if
  (secp256k1_ec_pubkey_negate(...) == 0) { return error::backend_failure; }`; the doc comment's
  return line: "-P, the public key of the negated secret key; `backend_failure` should
  libsecp256k1 refuse a parsed key, which v0.8.0 does not." Update the other callers.
- [ ] **Step 4: Run** `secp256k1_tweak secp256k1_ecdh secp256k1_allocation
  secp256k1_silent_payments timing_coverage_secp256k1` on all five lanes; expected: PASS.
- [ ] **Step 5: Commit** `crypto: pubkey_negate returns a result, as the other tweaks do`.

### Task 6: dudect's case lines, evidence, A/A control and one-case runs (design D6)

**Files:**
- Modify: `test/constant_time/dudect.hpp`, `dudect_test.cpp`, `dudect_sodium_test.cpp`,
  `dudect_secp256k1_test.cpp`
- Create: `test/constant_time/dudect_harness_test.cpp` (the statistics and the argument
  parser on synthetic data, no timing); modify `test/constant_time/Jamfile`
  (`webcpp.run dudect_harness : dudect_harness_test.cpp : : native wasip2 ;` in the lane)

**Interfaces:**
- Produces, in `webcpp::test` (`dudect.hpp`):
  - `struct class_summary { std::size_t count; double mean_ns; };`
  - `struct t_statistic { double t; int crop; double limit_ns; std::array<class_summary, 2> classes; };`
    (`crop` is -1 for the whole set, else 0 to 99)
  - `t_statistic largest_t(std::vector<double> const& timings, std::vector<int> const& classes);`
  - `template <class Prepare, class Run> t_statistic measure(std::uint64_t seed, std::size_t count, Prepare&& prepare, Run&& run);`
    and `twice` returning the `t_statistic` of the run it keeps
  - `template <bool Control = false, std::size_t Size, class Generator> void choose(int which, Generator& generator, std::array<std::uint8_t, Size> const& fixed, std::array<std::uint8_t, Size>& input);`
    (`Control`: the drawn bytes are drawn, then overwritten with `fixed` through the same
    volatile mask, so both classes write the fixed value with the same instructions)
  - `struct address { char const* label; void const* where; };`
  - `void report_case(char const* name, t_statistic const& result, bool judged, std::initializer_list<address> buffers);`
    printing the case line, and, when `result.t >= 10`, the evidence and addresses lines
  - `void report_machine();` printing the machine line once
  - `struct case_selection { char const* only = nullptr; bool runs(char const* name) const; };`
    and `std::optional<case_selection> select_case(int argc, char** argv, std::initializer_list<char const*> names);`
    (empty optional after printing `unknown case <x>; the cases: <names>` to stderr)
- Case names: `dudect_bearssl`: `gcm_seal`, `gcm_open`, `chacha_seal`, `chacha_open`,
  `ctaes_encrypt`, `equal`, `chacha_open_aa`; `dudect_sodium`: `x25519_agree`, `ed25519_sign`,
  `x25519_agree_aa`; `dudect_secp256k1`: `ecdh_x`, `ecdh_x_aa`; `dudect_planted`:
  `x25519_agree`.

- [ ] **Step 1: The failing tests,** in `dudect_harness_test.cpp`:
  - `largest_t_whole_set`: 1000 timings, class 0 all 100.0 and class 1 all 100.0 except one
    of 10000.0 at the largest: the whole set's crop is -1 or the crop that cuts the outlier, and
    `classes[0].count` plus `classes[1].count` is the kept count; a crafted set whose class 1
    is 1.0 higher with variance 1 gives `t > 10`, `crop` in [-1, 99], and means that differ by
    about 1.0.
  - `choose_control`: `choose<true>` for class 0 and class 1 from generators of one seed both
    write `fixed`, and advance the generator by `Size` draws as `choose<false>` does.
  - `select_case`: `{"dudect_x"}` gives a selection that runs every case;
    `{"dudect_x", "--case", "equal"}` runs `equal` alone; an unknown name gives an empty
    optional.
  - `report_case_format`: with `stdout` redirected to a temporary file
    (`std::freopen`), `report_case("equal", {12.3456, 4, 101.5, {{{10, 100.0}, {12, 101.0}}}},
    true, {{"a", reinterpret_cast<void const*>(0x1234)}})` writes exactly
    `dudect case=equal t=12.35 threshold=10 judged=yes`, then
    `dudect evidence case=equal crop=4 limit_ns=101.50 class0_n=10 class0_mean_ns=100.00 class1_n=12 class1_mean_ns=101.00`,
    then `dudect addresses case=equal a=0x234`; with t 3.0, the first line only.
- [ ] **Step 2: Run** `$B2 libs/crypto/test/constant_time//dudect_harness`; expected: compile
  errors naming `t_statistic`.
- [ ] **Step 3: Implement** the Interfaces in `dudect.hpp`. `report_machine`: on Linux read
  `/proc/cpuinfo`'s first `model name` and `microcode` lines; on macOS
  `sysctlbyname("machdep.cpu.brand_string")` and microcode `unavailable`; under `__wasi__`
  `cpu="unavailable" microcode=unavailable`; the clock's resolution the smallest nonzero
  `steady_clock` difference of 10,000 serialised back-to-back pairs (0 printed as
  `below_resolution`). Line: `dudect machine cpu="<model>" microcode=<value>
  clock_resolution_ns=<x with 2 decimals>`.
- [ ] **Step 4: The programs.** Each `main(int argc, char** argv)` calls `select_case` (exit 2
  on an empty optional), `report_data_independent_timing`, `report_machine`, then measures
  each selected case and calls `report_case` with its buffers (`dudect_bearssl`'s open: `data`,
  `tag`, `object` = `&*made`; seal: `data`, `key`, `object`; ctaes: `block`, `key`, `object`;
  equal: `a`, `b`; sodium's agree: `secret_key`, `peer`; sign: `secret_key`, `message`,
  `object`; secp256k1's: `secret_key`, `object`). The A/A cases repeat their source case with
  `choose<true>`. The human line keeps its form, listing only the cases run. `judged=no` is
  passed for `x25519_agree` on `__wasip2__` only. The verdict covers the selected cases, the
  A/A case included.
- [ ] **Step 5: Run** the lane natively in release (`$B2 libs/crypto/test/constant_time
  variant=release`, Linux in the linux-lint container and macOS) and on wasip2; expected: PASS,
  each output holding its case lines and the machine line; `dudect_planted` still finds its
  leak. Run `dudect_bearssl --case chacha_open_aa` by hand: one case line.
- [ ] **Step 6: Commit** `crypto: dudect prints one line per case, the evidence of every high
  value and an A/A control, and measures one case on request`.

### Task 7: The CI prints, warns and judges two of three on hosted runners (design D7)

**Files:**
- Create: `test/constant_time/dudect_judge.py`
- Create: `webcpp/tools/test/crypto_dudect_judge_test.py` (beside `crypto_collect_test.py`)
- Modify: `webcpp/.github/workflows/library.yml` (job `own-lanes`, after the step "Own lane")
- Modify: `test/constant_time/dudect.hpp` (`verdict`'s unjudged line), `test/constant_time/Jamfile`
  (its comment), `doc/backends.adoc` (the paragraph on hosted runners), `doc/testing.adoc` if it
  repeats it

**Interfaces:**
- Produces: `python3 libs/crypto/test/constant_time/dudect_judge.py --bin DIR --target
  {native,wasip2} [--rerun] [--launcher COMMAND]`; exit 0 when nothing failed, 1 when a case
  failed or an output holds no case line, 2 on a usage error.
- Consumes: Task 6's lines and `--case`.

- [ ] **Step 1: The failing tests,** `crypto_dudect_judge_test.py`, each building a scratch
  `bin/` with `<name>.output` files and, for reruns, a fake program (a Python script made
  executable, printing scripted case lines) beside each:
  - `judge_prints_and_warns`: an output with `t=69.80` for `chacha_open` and `t=3.10` for
    `equal`: stdout holds `::group::dudect_bearssl (native)`, the file's text,
    `::endgroup::`, and exactly one
    `::warning title=dudect dudect_bearssl chacha_open (native)::dudect case=chacha_open t=69.80 ...`;
    exit 0 without `--rerun`.
  - `judge_two_of_three_pass`: `--rerun`, the fake program answering 3.10 then 2.70: exit 0;
    stdout names the three values `69.80, 3.10, 2.70`.
  - `judge_two_of_three_fail`: answering 12.50 then 3.00 for `equal` at 14.00: exit 1, stderr
    `dudect: equal (dudect_bearssl, native) exceeded 10 in 2 of 3 runs: 14.00, 12.50, 3.00`.
  - `judge_unjudged_case`: `judged=no` at 30.0: warned, never rerun, exit 0.
  - `judge_skips_planted`: `dudect_planted.output` at 66.7: printed, not warned, not rerun.
  - `judge_output_without_case_lines`: an output with only `EXIT STATUS: 0`: exit 1 naming
    the file.
  - `judge_wasip2_launcher`: `--target wasip2 --launcher "<fake launcher>"`: the rerun command
    is the launcher, `<dir>/<name>.wasm`, `--case`, `<case>`.
  The two-of-three failure:

```python
def judge_two_of_three_fail(scratch: Path) -> None:
    directory = write_output(scratch, 'dudect_bearssl',
                             'dudect case=equal t=14.00 threshold=10 judged=yes\n')
    write_fake_program(directory / 'dudect_bearssl', answers=['12.50', '3.00'], case='equal')
    done = run_judge('--bin', str(scratch), '--target', 'native', '--rerun')
    assert done.returncode == 1, done
    assert ('dudect: equal (dudect_bearssl, native) exceeded 10 in 2 of 3 runs: '
            '14.00, 12.50, 3.00') in done.stderr, done.stderr
```
- [ ] **Step 2: Run** `python3 tools/test/crypto_dudect_judge_test.py`; expected: FAIL (no
  script).
- [ ] **Step 3: Implement** `dudect_judge.py` (licence notice, a module docstring with the
  usage, `argparse`, `pathlib`, `subprocess.run(..., capture_output=True, text=True,
  timeout=1800)`): find `DIR/**/dudect_*.output`, sorted; a case line matches
  `^dudect case=(\S+) t=([0-9.]+) threshold=10 judged=(yes|no)$`; the program is the
  output's sibling `<name>` (`<name>.wasm` on wasip2); the `::warning` text is the case line
  followed by its evidence line when there is one.
- [ ] **Step 4: The workflow step,** after "Own lane":

```yaml
      # crypto's constant-time lane: every dudect output printed, a warning for every |t| of 10
      # or more, and on a runner GitHub hosts a case over 10 measured again in two fresh
      # processes, failing when two of the three exceed 10 (libs/crypto's change api-gaps, D7).
      - name: Judge dudect
        if: always() && matrix.library == 'crypto' && matrix.lane == 'constant-time'
        shell: bash
        run: |
          python3 libs/crypto/test/constant_time/dudect_judge.py --bin bin \
            --target "${{ matrix.platform }}" \
            ${{ runner.environment == 'github-hosted' && '--rerun' || '' }} \
            --launcher "tools/boost/asio-wasip2/run.sh"
```

  (`matrix.platform` is the own lane's target, which `tools/ci/matrix.py`'s `own_lane_entry`
  writes; the matrix does not change, so `tools/ci/matrix_test.py` does not.)
- [ ] **Step 5: The texts.** `verdict`'s unjudged line: `printed here; the CI's judge
  (dudect_judge.py) measures a case over 10 again on a hosted runner`. `backends.adoc`'s
  paragraph: on a hosted runner the lane prints each |t| and its evidence, warns at 10, measures
  a case over 10 again in two fresh processes and fails when two of three exceed 10; why (the
  69.8 of 2026-11-20 and the research's hypothesis, a layout effect a fresh process varies);
  each program's A/A control.
- [ ] **Step 6: Run** the tool test; run `actionlint` on `library.yml` if the superproject's
  lint runs it (the macOS lint); expected: PASS.
- [ ] **Step 7: Commit** (library) `crypto: dudect's judge prints every output, warns at 10 and
  judges two of three fresh runs on hosted runners`; (superproject)
  `ci: crypto's constant-time lane runs dudect's judge after the lane`.

### Task 8: Verification and delivery

- [ ] **Step 1:** crypto's tests and examples on native clang and gcc-15, wasip2, wasip3,
  Emscripten, in the variants each lane builds (`AGENTS.md` chapter 4); the constant-time lane
  natively in release on macOS and in the arm64 Linux container, and on wasip2; the judge run
  by hand on that `bin/` without `--rerun` (exit 0, every output printed).
- [ ] **Step 2:** the full macOS lint; the Linux lint (`linux-lint/run.sh`, exit 0); the MSVC
  lane (14.3 and 14.5, no failure beyond `known-failures.txt`); `tools/test/*_test.py` touched
  by this change. Any failure or flake, related or not, is fixed at its cause.
- [ ] **Step 3:** the changelog's first line gains "GCM's two-pass incremental open, the block
  cipher's default per target, secp256k1's `generate`" as one clause (the ten-line rule);
  `python3 doc/tables/generate.py --check`; the doc build clean.
- [ ] **Step 4:** tick `../tasks.md`; pull requests in crypto, the superproject (the workflow
  step, the tool test, the gitlink) and proposals; merged; one CI run per repository at the end
  of the queue (the owner's rule of 2026-11-20), the hosted constant-time lane's judge output
  read and recorded in `../tasks.md` 4.3.
