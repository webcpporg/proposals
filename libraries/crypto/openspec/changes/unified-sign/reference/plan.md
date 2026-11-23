# Unified sign and verify Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** One name per signing operation in webcpp::crypto, the scheme always named, `key.sign(scheme, input)` and `crypto::verify(scheme, signature, input, public_key)` (and their `async_` forms), on Ed25519, P-256 and secp256k1, the input a message or `crypto::prehashed{digest}`.

**Architecture:** A new header `<webcpp/crypto/signing.hpp>` holds the scheme tags, the marker, the input checks (each a `static_assert` with a message of its own), the traits each curve specializes for its key types, the public concepts `can_sign` and `can_verify`, and a constrained catch-all `crypto::verify` that refuses a scheme and key that do not go together. Each curve's header implements its own `sign` member template and `crypto::verify` overloads over private workers, and removes its old names; `async.hpp` takes the same forms. Every caller in libs/crypto moves in the task that removes the name it calls, so that the library builds at the end of every task.

**Tech Stack:** C++20, header-only, b2 (`webcpp.run`, `webcpp.compile-diagnostic`), BearSSL (P-256), libsodium (Ed25519), libsecp256k1, Boost.Hash2 through webcpp::hash (`webcpp::hash::sha2_256`), webcpp::asio for the asynchronous family.

**Spec:** `libraries/crypto/openspec/changes/unified-sign/` in webcpporg/proposals (proposal.md, design.md with D1-D7 and R1-R11, specs/crypto/spec.md). Read design.md's D3 table before any task.

## Global Constraints

- Worktree: `/Users/you/dev/webcpporg/crypto-sign/webcpp` (superproject, branch `crypto/sign-unified`); the library is `libs/crypto` (its own repository, branch `crypto/sign-unified`). Commit in `libs/crypto` only, unless a task says otherwise.
- Read `libs/crypto/AGENTS.md`, the superproject's `CODING-GUIDELINES.md` and `AGENTS.md` first; follow them (doc comment style, names, `[[nodiscard]]`, `noexcept`, include order, the timing guard rule).
- Everything in English. Never the em dash character; a plain "-". Commit messages: `crypto: <what>`, no trailer of any kind (no Co-Authored-By, no "Generated with").
- Never edit a generated file (`doc/generated/*`, `doc/llms.txt`, `include/webcpp/crypto/detail/capabilities.hpp`, `detail/defaults.hpp`, `test/capabilities_test.cpp`, the defaults' aliases in `build.jam`): change `doc/tables/crypto.json` or the generator and run `python3 doc/tables/generate.py`. Never edit `doc/tables/sizes.json` or `doc/performance.json` by hand. `doc/changelog.adoc`'s unreleased lines are the changelog source (each line at most 30 words); no `CHANGELOG.md` is edited.
- Removed with no alias, shim or deprecation: Ed25519's `basic_keypair::sign(message)` and free `curves::ed25519::verify`; P-256's `basic_keypair::sign_ecdsa` and free `verify_ecdsa`; secp256k1's `basic_keypair::sign_ecdsa` (both), `sign_schnorr`, `sign_recoverable`, free `verify_ecdsa`, `verify_schnorr`, and `recover(signature, hash)`.
- Every public function that calls a backend holds `detail::timing_guard const timing;` as its first statement and is called by its backend's `test/timing_coverage_<backend>_test.cpp`; a public overload that forwards calls a private worker that holds no guard, so the first guard the call enters is its own.
- No overload ever picks message versus digest by a span's extent or size.
- ECDSA signs low S on both curves (P-256 normalizes after signing); P-256's verification accepts high and low S, secp256k1's refuses high S (design D3). The format helpers are `to_der(signature, output)`, `from_der(der)`, `normalize_low_s(signature)`, `is_low_s(signature) -> bool` on both curves; `normalize_ecdsa` and `check_low_s` are removed (design D4b, R13).
- Build commands run from the superproject: `b2 --user-config=/Users/you/dev/webcpporg/webcpp/.local/user-config.jam -j10 toolset=clang <targets>` (targets like `libs/crypto/test//p256_ecdsa_bearssl`). A build of all of `libs/crypto/test libs/crypto/example`, and any gcc/Emscripten whole-directory run, is heavy: prefix it with `/Users/you/dev/webcpporg/heavy.sh crypto-sign`. Never rename or delete a file in `/Users/you/dev/webcpporg/HEAVY.queue`.
- Emscripten needs `EM_CACHE=/Users/you/dev/webcpp/.local/emscripten-cache`; gcc is `toolset=gcc-15`.
- A flaky or failing test, or a lint warning, seen on the way is a defect to fix or to report to the controller; never skip one.

## Review Focus

1. A 32-byte `std::array` message given with `ecdsa_sha256` must be hashed, never read as a digest: Task 2 and Task 4 each assert `sign(ecdsa_sha256, array32) == sign(ecdsa_sha256, prehashed{sha256(array32)})` and `!= sign(ecdsa_sha256, prehashed{array32})`.
2. `crypto::prehashed{dynamic_span}` must not compile (Task 1, a message test) and `prehashed` must never convert to or from a span (Task 1, `static_assert(!std::is_convertible_v<...>)` both ways).
3. An Ed25519 call with the old argument order (key, message, signature) must fail closed: Task 3 asserts `crypto::verify(eddsa, public_key_32, message, signature_64)` returns `invalid_signature`.
4. secp256k1 `sign(schnorr, message)` without aux must succeed and verify, and its draw must fail closed: Task 4 tests the draw path, and the failing-entropy test (`random_device_failing_test.cpp`'s mechanism, if it reaches secp256k1) gives `entropy_unavailable`.
5. The empty message is a message on every scheme (Task 2, 3, 4: sign and verify `std::span<std::uint8_t const>{}`).
6. A high-S P-256 signature from WebCrypto must still verify after signing became low S (Task 2: the RFC 6979 section's high-S signature verifies; the WebCrypto oracle verifies WebCrypto's own signatures).

---

### Task 1: The shared vocabulary, `<webcpp/crypto/signing.hpp>`

**Files:**
- Create: `libs/crypto/include/webcpp/crypto/signing.hpp`
- Create: `libs/crypto/test/signing_test.cpp`; message tests `libs/crypto/test/messages/prehashed_dynamic/` (a `.cpp` and a `Jamfile`, as `messages/async_sign_secp256k1/`)
- Modify: `libs/crypto/test/Jamfile` (`crypto-run signing : signing_test.cpp ;`), `libs/crypto/test/messages/Jamfile`, `libs/crypto/doc/tables/crypto.json` (the message test's entry, `fixes`), `libs/crypto/include/webcpp/crypto.hpp` if the convenience header lists every public header, `meta/include-boundaries.json` if it lists headers.

**Interfaces (Produces):**
- Tags, namespace `webcpp::crypto`: `struct eddsa_t { explicit eddsa_t() = default; };` and likewise `ecdsa_sha256_t`, `ecdsa_sha256_recoverable_t`, `schnorr_t`; `inline constexpr eddsa_t eddsa{};` `ecdsa_sha256`, `ecdsa_sha256_recoverable`, `schnorr`.
- `template <std::size_t N> struct prehashed { std::span<std::uint8_t const, N> digest; };` with `static_assert(N != std::dynamic_extent, "webcpp::crypto: crypto::prehashed takes a digest of a fixed size: a std::array<std::uint8_t, N> or a std::span<std::uint8_t const, N>.")`; deduction guides from `std::array<std::uint8_t, N> const&`, `std::span<std::uint8_t const, N>`, `std::span<std::uint8_t, N>`, `boost::hash2::digest<N> const&` (span over `data()`).
- `detail::is_prehashed_v<T>`; `detail::is_message_v<T>` (convertible to `std::span<std::uint8_t const>` and not a `prehashed`).
- Per scheme, in `detail::scheme_traits<Scheme>`: `signature_size` (64; 65 for `ecdsa_sha256_recoverable_t`), `digest_size` (32 for both ECDSA tags; 0 for `eddsa_t`, `schnorr_t`, meaning "takes no digest").
- Traits each curve specializes: `template <class Key, class Scheme> inline constexpr bool detail::key_has_scheme_v = false;` (for key pairs and public keys alike) and `template <class Key> struct detail::key_backend { using type = void; };` (the backend of a key type; `void` for byte keys).
- `template <class Backend> inline constexpr bool detail::signs_prehashed_v = true;` here; Task 5 redefines nothing: `async.hpp` adds the partial rule through `asynchronous_backend_v` (see Task 5). To avoid a cycle, move `detail::asynchronous_backend_v` (today in `async.hpp`) into `signing.hpp` and define `signs_prehashed_v<B> = !asynchronous_backend_v<B>` here.
- `detail::input_valid_v<Scheme, Input, Backend>`: message, or `prehashed<N>` with `N == scheme_traits<Scheme>::digest_size != 0` and `signs_prehashed_v<Backend>`.
- `template <class Keypair, class Scheme, class Input> concept can_sign = detail::key_has_scheme_v<Keypair, Scheme> && detail::input_valid_v<Scheme, std::remove_cvref_t<Input>, typename detail::key_backend<Keypair>::type>;` and `template <class Scheme, class Input, class PublicKey> concept can_verify` the same over the public key type (byte keys: `std::remove_cvref_t` of what the caller passes; a curve specializes `key_has_scheme_v` for byte keys of its scheme by convertibility, see Tasks 3 and 4).
- `template <class Scheme, class Input, class Backend> constexpr void detail::check_input() noexcept`: an `if constexpr` chain that fires exactly one `static_assert` for the first failing reason, with these exact messages:
  - not a message nor a marker: `"webcpp::crypto: a signature's input is a message (bytes, as std::span<std::uint8_t const>) or crypto::prehashed{digest}."`
  - `eddsa_t` and a marker: `"webcpp::crypto: crypto::eddsa signs the message itself (RFC 8032's pure Ed25519) and takes no digest; Ed25519ph, which hashes the message, is curves::ed25519::ph."`
  - `schnorr_t` and a marker: `"webcpp::crypto: crypto::schnorr (BIP340) signs a message of any length and has no prehash mode: pass the message."`
  - a marker of the wrong size: `"webcpp::crypto: crypto::ecdsa_sha256 takes a 32-byte digest, SHA-256's: crypto::prehashed{digest} of a std::array<std::uint8_t, 32>."`
  - a marker on a backend that cannot sign one: `"webcpp::crypto: this backend cannot sign or verify a digest (WebCrypto has no prehash mode): pass the message."`
- `detail::message_bytes(Input const&)` returning `std::span<std::uint8_t const>`, and `detail::sha256_of(std::span<std::uint8_t const>)` returning `std::array<std::uint8_t, 32>` over `webcpp::hash::sha2_256` (move `detail::p256_digest` here under this name; `async.hpp` stops defining it).
- Catch-all `crypto::verify`: `template <class Scheme, class Signature, class Input, class PublicKey> requires(!detail::has_verifier_v<Scheme, std::remove_cvref_t<PublicKey>>) boost::system::result<void> verify(Scheme, Signature const&, Input const&, PublicKey const&) noexcept` whose body is `static_assert(detail::refused_v<Scheme>, "webcpp::crypto: verify: this public key does not go with this scheme: crypto::eddsa takes Ed25519's 32 bytes, crypto::ecdsa_sha256 a curves::p256::basic_public_key or a curves::secp256k1::public_key, crypto::schnorr a 32-byte x-only key.");` then `return {};`. `detail::has_verifier_v<Scheme, Key>` is false here; the curves specialize it (Tasks 2-4). Move `detail::refused_v` from `async.hpp` here.

- [ ] **Step 1: Write the failing test `test/signing_test.cpp`** with `static_assert`s and a `main` returning 0, using a stand-in key type declared in the test (`struct stand_in_keypair {}; struct stand_in_backend {};` with `key_has_scheme_v<stand_in_keypair, ecdsa_sha256_t> = true`, `key_backend<stand_in_keypair>::type = stand_in_backend`; a second stand-in backend with `asynchronous_backend_v` true):
  - `can_sign<stand_in_keypair, ecdsa_sha256_t, std::array<std::uint8_t, 32>>`, `..., std::span<std::uint8_t const>>`, `..., prehashed<32>>` are true; `..., prehashed<48>>`, `..., schnorr_t, std::span<std::uint8_t const>>` are false; on the asynchronous stand-in, `prehashed<32>` false and the message true.
  - `decltype(prehashed{std::array<std::uint8_t, 32>{}})` is `prehashed<32>`; `decltype(prehashed{boost::hash2::digest<32>{}})` is `prehashed<32>`.
  - `!std::is_convertible_v<std::span<std::uint8_t const, 32>, prehashed<32>>`, `!std::is_convertible_v<prehashed<32>, std::span<std::uint8_t const>>`, `!std::is_convertible_v<std::array<std::uint8_t, 32>, prehashed<32>>`.
  - `!std::is_default_constructible_v` is not required; instead `!std::is_convertible_v<decltype({}), ...>` cannot be written: assert `!std::is_constructible_v<ecdsa_sha256_t, int>` and that `ecdsa_sha256_t t = {};` is ill-formed through `!std::is_convertible_v`-style check `requires { [](ecdsa_sha256_t) {}({}); }` is false.
  - `detail::scheme_traits<ecdsa_sha256_recoverable_t>::signature_size == 65`.
  - At run time, `detail::sha256_of` of "abc" equals FIPS 180-2's `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad`.
- [ ] **Step 2: Run it to see it fail:** `b2 ... toolset=clang libs/crypto/test//signing` - Expected: compile error, `signing.hpp` not found.
- [ ] **Step 3: Write `signing.hpp`** with the interfaces above, its file comment in the style of the other headers (what it is, `@see` the guide's signing section), each tag and member doc-commented. Move `asynchronous_backend_v`, `refused_v` and the SHA-256 helper out of `async.hpp` into it, and make `async.hpp` include it (no behaviour change in `async.hpp` yet).
- [ ] **Step 4: Write the message test `prehashed_dynamic`**: `crypto::prehashed{std::span<std::uint8_t const>(bytes)}` with `expected-error` the N message above, `alone`, on native, wasip2, wasip3 (as `async_sign_secp256k1`'s Jamfile), and its `crypto.json` entry; run `python3 doc/tables/generate.py` and commit what it regenerates.
- [ ] **Step 5: Run** `b2 ... toolset=clang libs/crypto/test//signing libs/crypto/test/messages//prehashed_dynamic libs/crypto/test//async_p256 libs/crypto/test//async_25519` - Expected: all pass. `python3 libs/crypto/doc/tables/generate.py --check` - Expected: exit 0.
- [ ] **Step 6: Commit** `crypto: signing.hpp, the scheme tags, prehashed and the signing concepts`.

### Task 2: P-256 signs and verifies with `ecdsa_sha256`

**Files:**
- Modify: `libs/crypto/include/webcpp/crypto/curves/p256.hpp`, `libs/crypto/include/webcpp/crypto/async.hpp` (its internal calls only), every P-256 caller: `test/p256_ecdsa_bearssl_test.cpp`, `p256_wycheproof_bearssl_test.cpp`, `p256_openssl_oracle_test.cpp`, `p256_webcrypto_oracle_test.cpp`, `p256_keys_bearssl_test.cpp`, `allocation_bearssl_test.cpp`, `timing_coverage_bearssl_test.cpp`, `test/support/timing_coverage.hpp`, `test/constant_time/paths.hpp`, `dudect_test.cpp`, `bearssl_verdicts.supp` (function names in frames), `test/sizes/p256_bearssl_verify_ecdsa.cpp` (keeps its name; calls `crypto::verify(ecdsa_sha256, signature, prehashed{digest}, key)`), `bench/operations.hpp`, `bench/throughput.cpp` (case names unchanged), `example/p256_sign.cpp`, `ecdsa_der.cpp`, `p256_handshake.cpp`, `signed_update.cpp` (and their `.expected` only if output changes), and any other file `grep -rn 'sign_ecdsa\|verify_ecdsa' libs/crypto` shows that uses P-256.

**Interfaces:**
- Consumes: Task 1's tags, `prehashed`, `detail::check_input`, `detail::sha256_of`, `detail::message_bytes`, `key_has_scheme_v`, `key_backend`, `has_verifier_v`.
- Produces, in `p256.hpp`:
  - `template <class Scheme, class Input> [[nodiscard]] boost::system::result<std::array<std::uint8_t, signature_size>> basic_keypair<Backend>::sign(Scheme, Input const& input) const noexcept;` - body: `timing_guard` first; `static_assert(std::is_same_v<Scheme, ecdsa_sha256_t>, "webcpp::crypto: a P-256 key signs with crypto::ecdsa_sha256.")`; `detail::check_input<Scheme, Input, Backend>()`; the message's `sha256_of` or the marker's digest goes to a private `signed_digest(std::span<std::uint8_t const, 32>)` (the old `sign_ecdsa` body, no guard).
  - In namespace `webcpp::crypto`: `template <class Backend, class Input> [[nodiscard]] boost::system::result<void> verify(ecdsa_sha256_t, std::span<std::uint8_t const, curves::p256::signature_size> signature, Input const& input, curves::p256::basic_public_key<Backend> const& public_key) noexcept;` - guard, `check_input`, then the old `verify_ecdsa` body (range check, engine verify).
  - Specializations: `key_has_scheme_v<curves::p256::basic_keypair<B>, ecdsa_sha256_t>`, `key_has_scheme_v<curves::p256::basic_public_key<B>, ecdsa_sha256_t>`, `key_backend` of both, `has_verifier_v<ecdsa_sha256_t, curves::p256::basic_public_key<B>>`, all true.
  - `signed_digest` normalizes to low S after the engine signs: the signature is public once made, so `detail::declassify` it (as the constant-time lane needs, `detail/declassify.hpp`) before `detail::p_curve_low_s` branches on s, then negate s with `detail::p_curve_negate_scalar` when high.
  - Renamed: `normalize_ecdsa` to `normalize_low_s` (same contract); `check_low_s` to `[[nodiscard]] inline bool is_low_s(std::span<std::uint8_t const, signature_size> signature) noexcept` (true exactly when 0 < r < n and 0 < s <= (n - 1) / 2).
  - Removed: `sign_ecdsa`, `verify_ecdsa`, `normalize_ecdsa`, `check_low_s`. Doc comments (file comment, the helpers) name the new calls and say that signing gives low S and verification accepts both.
- `async.hpp`: its P-256 lambdas call `key_pair.sign(ecdsa_sha256, message)` and `crypto::verify(ecdsa_sha256, ..., message, public_key)`; its public signatures do not change in this task.

- [ ] **Step 1: Write the failing tests** in `test/p256_ecdsa_bearssl_test.cpp` (replace the old calls; keep every existing case):
  - `rfc6979_sample_message`: RFC 6979 A.2.5's key, `sign(crypto::ecdsa_sha256, "sample" bytes)` gives the section's r (`efd48b2aacb6a8fd1140dd9cd45e81d69d2c877b56aaf991c34d0ea84eaf3716`) and n - s, the section's s (`f7cb1c942d657c41d436c7a1b6e29f65f3e900dbb9aff4064dc4ab2f843acda8`) being high; `crypto::verify` accepts the section's own high-S signature too; `is_low_s` is false on the section's signature and true on `sign`'s; `normalize_low_s` of the section's signature equals `sign`'s.
  - `always_low_s`: 256 signatures of random messages under random keys all satisfy `is_low_s`.
  - `message_is_hashed`: for a 32-byte `std::array` message `m`, `sign(ecdsa_sha256, m) == sign(ecdsa_sha256, prehashed{sha256_of(m)})` and `!= sign(ecdsa_sha256, prehashed{m})`; `crypto::verify(ecdsa_sha256, sig, m, pk)` passes, and with `prehashed{m}` returns `invalid_signature`.
  - `empty_message`: sign and verify `std::span<std::uint8_t const>{}`.
  - `zero_digest`: the existing 0-mod-n case through `prehashed`.
- [ ] **Step 2: Run** `b2 ... toolset=clang libs/crypto/test//p256_ecdsa_bearssl` - Expected: compile errors on `sign` with a scheme.
- [ ] **Step 3: Implement** the interfaces above in `p256.hpp` and `async.hpp`'s internal calls.
- [ ] **Step 4: Migrate every P-256 caller** listed above (the OpenSSL oracle compares with OpenSSL's deterministic signature normalized to low S; the WebCrypto oracle still verifies both ways, and also checks that WebCrypto's own signatures, high or low, verify with `crypto::verify`); the Wycheproof driver's low-S assertions use `is_low_s`; `timing_coverage_bearssl_test.cpp` checks `sign` with a message and with a marker, and `crypto::verify` with both, each entering its own guard. In `constant_time/paths.hpp` the signing path signs `prehashed{digest}` (the secret path, unchanged in what it measures); rename frames in `bearssl_verdicts.supp` to the new function names as the compiler mangles them (check with a memcheck run on Linux in Task 7, or read the demangled names from a native build's symbols now).
- [ ] **Step 5: Add a message test `sign_p256_schnorr`**: `keypair.sign(crypto::schnorr, message)` on P-256, `expected-error` "webcpp::crypto: a P-256 key signs with crypto::ecdsa_sha256.", `alone`, with its `crypto.json` entry; and `sign_p256_digest_48`: `prehashed{std::array<std::uint8_t, 48>{}}` with the 32-byte message of Task 1. Regenerate.
- [ ] **Step 6: Run** `grep -rn 'sign_ecdsa\|verify_ecdsa\|normalize_ecdsa\|check_low_s' libs/crypto --include=*.hpp --include=*.cpp | grep -i p256` - Expected: nothing (secp256k1's remain until Task 4). Then the P-256 targets: `b2 ... toolset=clang libs/crypto/test//p256_ecdsa_bearssl libs/crypto/test//p256_wycheproof_bearssl libs/crypto/test//p256_keys_bearssl libs/crypto/test//p256_openssl_oracle libs/crypto/test//allocation_bearssl libs/crypto/test//timing_coverage_bearssl libs/crypto/test//async_p256 libs/crypto/test/messages//sign_p256_schnorr libs/crypto/test/messages//sign_p256_digest_48 libs/crypto/example//p256_sign libs/crypto/example//ecdsa_der libs/crypto/example//p256_handshake libs/crypto/example//signed_update libs/crypto/bench` and the Emscripten oracle `b2 ... toolset=emscripten libs/crypto/test//p256_webcrypto_oracle` - Expected: all pass.
- [ ] **Step 7: Commit** `crypto: P-256 signs with sign(ecdsa_sha256, input) and verifies with crypto::verify`.

### Task 3: Ed25519 signs and verifies with `eddsa`

**Files:**
- Modify: `libs/crypto/include/webcpp/crypto/curves/ed25519.hpp`, `async.hpp` (internal calls only), every Ed25519 caller: `test/ed25519_sodium_test.cpp`, `ed25519_speccheck_test.cpp`, `ed25519_wycheproof_sodium_test.cpp`, `ed25519ph_sodium_test.cpp`, `ed25519_wipe_sodium_test.cpp`, `oracle_25519_test.cpp`, `formats_25519_webcrypto_oracle_test.cpp`, `formats_25519_*`, `timing_coverage_sodium_test.cpp`, `test/msvc/ed25519_unknown_policy/`, `test/messages/ed25519_unknown_policy/`, `example/ed25519_sign.cpp`, `signed_message.cpp`, `arena_signer.cpp` and any other file `grep -rn 'ed25519::verify\|\.sign(' libs/crypto` shows that uses Ed25519.

**Interfaces:**
- Consumes: Task 1.
- Produces, in `ed25519.hpp`:
  - `template <class Scheme, class Input> [[nodiscard]] boost::system::result<std::array<std::uint8_t, 64>> basic_keypair<Backend>::sign(Scheme, Input const& message) const noexcept;` - guard; `static_assert(std::is_same_v<Scheme, eddsa_t>, "webcpp::crypto: an Ed25519 key signs with crypto::eddsa.")`; `check_input<Scheme, Input, Backend>()`; the old body over `detail::message_bytes(message)`.
  - In namespace `webcpp::crypto`: `template <class Policy = curves::ed25519::strict, class Backend = default_backend_t<families::ed25519>, class Input> [[nodiscard]] boost::system::result<void> verify(eddsa_t, std::span<std::uint8_t const> signature, Input const& message, std::span<std::uint8_t const> public_key) noexcept;` - guard, the policy `static_assert` (its existing message), `check_input`, `invalid_public_key` for a key of another size, `invalid_signature` for a signature of another size, then the engine.
  - `ph::verify(std::span<std::uint8_t const> signature, std::span<std::uint8_t const> public_key)` - signature first.
  - Specializations: `key_has_scheme_v<curves::ed25519::basic_keypair<B>, eddsa_t>`, `key_backend` of it; for byte public keys, `has_verifier_v<eddsa_t, K>` and `key_has_scheme_v<K, eddsa_t>` true when `K` converts to `std::span<std::uint8_t const>`, with `key_backend<K>::type` `void` (signs_prehashed is irrelevant: `eddsa` refuses a marker).
  - Removed: `sign(message)` and `curves::ed25519::verify`.

- [ ] **Step 1: Write the failing tests** in `test/ed25519_sodium_test.cpp`: RFC 8032 7.1 test 1 with `sign(crypto::eddsa, message)` and `crypto::verify(crypto::eddsa, signature, message, public_key)`; `old_order_fails_closed`: `crypto::verify(crypto::eddsa, public_key, message, signature)` (the arguments of the old order) returns `invalid_signature`; `empty_message`; the small-order key with `crypto::verify<curves::ed25519::strict, backends::sodium>(crypto::eddsa, ...)` gives `invalid_signature`. In `ed25519ph_sodium_test.cpp`, RFC 8032 7.3 with `ph::verify(signature, public_key)`.
- [ ] **Step 2: Run** `b2 ... toolset=clang libs/crypto/test//ed25519_sodium` - Expected: compile errors.
- [ ] **Step 3: Implement** the interfaces above; `async.hpp`'s Ed25519 lambdas call the new forms (public async signatures unchanged in this task).
- [ ] **Step 4: Migrate every Ed25519 caller**, the speccheck and Wycheproof drivers included (argument order: signature, message, key), and the timing coverage of `sign`, `crypto::verify` and `ph::verify`.
- [ ] **Step 5: Add a message test `sign_ed25519_prehashed`**: `key.sign(crypto::eddsa, crypto::prehashed{digest})`, `expected-error` the `eddsa` message of Task 1, `alone`, `crypto.json` entry; regenerate.
- [ ] **Step 6: Run** `grep -rn 'ed25519::verify' libs/crypto` - Expected: nothing outside doc prose Task 6 rewrites; then `b2 ... toolset=clang` over every Ed25519 test, `async_25519`, the message tests, the MSVC test's sources where they build natively, and the examples above; `toolset=emscripten libs/crypto/test//formats_25519_webcrypto_oracle libs/crypto/test//oracle_25519` - Expected: all pass.
- [ ] **Step 7: Commit** `crypto: Ed25519 signs with sign(eddsa, message) and verifies with crypto::verify`.

### Task 4: secp256k1 signs with `ecdsa_sha256`, `ecdsa_sha256_recoverable` and `schnorr`

**Files:**
- Modify: `libs/crypto/include/webcpp/crypto/curves/secp256k1.hpp` (and `secp256k1/musig.hpp`'s doc comment that names `verify_schnorr`), every secp256k1 caller: `test/secp256k1_test.cpp`, `secp256k1_recovery_test.cpp`, `secp256k1_der_test.cpp`, `secp256k1_allocation_test.cpp`, `secp256k1_formats_test.cpp`, `secp256k1_wycheproof_test.cpp`, `secp256k1_bip340_test.cpp`, `secp256k1_xonly_tweak_test.cpp`, `secp256k1_c_api_test.cpp`, `secp256k1_musig_test.cpp`, `secp256k1_silent_payments_test.cpp`, `crypto_secp256k1_test.cpp`, `test/support/secp256k1_checks.hpp`, `timing_coverage_secp256k1_test.cpp`, `test/messages/secp256k1_other_backend/`, `test/messages/async_sign_secp256k1/` (its synchronous line), `test/msvc/secp256k1_other_backend/`, `test/msvc/async_sign_secp256k1/`, `test/msvc/canary_secp256k1_two_files/`, `bench/operations.hpp`, examples `schnorr_sign.cpp`, `recover_signer.cpp`, `taproot_tweak.cpp`, `key_tweak.cpp`, `musig_taproot.cpp`, `musig_network.cpp`, `musig_wallet.cpp`, `wallet_key.cpp`, `silent_payment.cpp` and any other `grep -rn 'sign_ecdsa\|verify_ecdsa\|sign_schnorr\|verify_schnorr\|sign_recoverable\|recover(' libs/crypto` hit.

**Interfaces:**
- Consumes: Task 1.
- Produces, in `secp256k1.hpp`:
  - `template <class Scheme, class Input> [[nodiscard]] auto basic_keypair<Backend>::sign(Scheme, Input const& input) const noexcept` returning `result<std::array<std::uint8_t, 64>>` for `ecdsa_sha256_t` and `schnorr_t`, `result<std::array<std::uint8_t, 65>>` for `ecdsa_sha256_recoverable_t` (declare the return type as `boost::system::result<std::array<std::uint8_t, detail::scheme_traits<Scheme>::signature_size>>`). Guard; a scheme check `static_assert(std::is_same_v<Scheme, ecdsa_sha256_t> || std::is_same_v<Scheme, ecdsa_sha256_recoverable_t> || std::is_same_v<Scheme, schnorr_t>, "webcpp::crypto: a secp256k1 key signs with crypto::ecdsa_sha256, crypto::ecdsa_sha256_recoverable or crypto::schnorr.")`; `check_input`; then private workers without guards: `signed_ecdsa(digest, ndata)` (exists), `signed_recoverable(digest)` (the old `sign_recoverable` body), `signed_schnorr(message, aux)` (the old `sign_schnorr` body). `schnorr` without aux draws 32 bytes into a `detail::secp256k1_secret` with `random_device` and returns `error::entropy_unavailable` if the draw fails.
  - `[[nodiscard]] boost::system::result<std::array<std::uint8_t, 64>> sign(ecdsa_sha256_t, Input const& input, std::span<std::uint8_t const, 32> extra_entropy) const noexcept` (template on `Input`, hedged) and `sign(schnorr_t, Input const& message, std::span<std::uint8_t const, 32> aux_random) const noexcept`.
  - In namespace `webcpp::crypto`: `template <class Input> [[nodiscard]] boost::system::result<void> verify(ecdsa_sha256_t, std::span<std::uint8_t const, 64> signature, Input const& input, curves::secp256k1::public_key const& public_key) noexcept` (static context, self-test, high S refused, as `verify_ecdsa` did) and `template <class Input> [[nodiscard]] boost::system::result<void> verify(schnorr_t, std::span<std::uint8_t const, 64> signature, Input const& message, std::span<std::uint8_t const, 32> xonly_public_key) noexcept`. The `Backend` template parameter of the old free functions goes: libsecp256k1 is the only backend, and `secp256k1_refuses_others` stays on the key pair; keep `test/messages/secp256k1_other_backend` meaningful by making it name the key pair or `parse_public_key<OtherBackend>` (update its expected text only if the refusal itself moves).
  - `template <class Input> [[nodiscard]] boost::system::result<public_key> recover(ecdsa_sha256_recoverable_t, std::span<std::uint8_t const, recoverable_signature_size> signature, Input const& input) noexcept` (keeps a `Backend` template parameter only if `parse_public_key` keeps one: match it).
  - Specializations: `key_has_scheme_v<curves::secp256k1::basic_keypair<B>, S>` for the three tags; `key_has_scheme_v<curves::secp256k1::public_key, ecdsa_sha256_t>`; `has_verifier_v<ecdsa_sha256_t, curves::secp256k1::public_key>`; `has_verifier_v<schnorr_t, K>` and `key_has_scheme_v<K, schnorr_t>` when `K` converts to `std::span<std::uint8_t const, 32>`; `key_backend` of the key pair `B`, of `public_key` `backends::secp256k1`.
  - Renamed: `normalize_ecdsa` to `normalize_low_s`; new `[[nodiscard]] bool is_low_s(std::span<std::uint8_t const, 64> signature) noexcept` (libsecp256k1's compact parse refuses r or s not below n; s zero or r zero is false; `secp256k1_ecdsa_signature_normalize` reports whether s was high), with a timing guard if it calls libsecp256k1, and `to_der`/`from_der` unchanged.
  - Removed: every old name in the Global Constraints.
- MuSig2's and silent payments' functions keep their names (design R7); the MuSig2 examples and tests verify aggregate signatures with `crypto::verify(crypto::schnorr, ...)`.

- [ ] **Step 1: Write the failing tests** in `test/secp256k1_test.cpp`: the genesis's BIP340 case (secret 32 x `0x42`, message 32 x `0x17`, aux 32 zero bytes) with `sign(crypto::schnorr, message, aux)` and `crypto::verify(crypto::schnorr, ...)`, flipped first byte refused; `schnorr_draws_aux`: `sign(crypto::schnorr, message)` twice gives two different signatures that both verify; `ecdsa_message_is_hashed` as Task 2's for `ecdsa_sha256`; `sighash`: `sign(ecdsa_sha256, prehashed{d})` equals libsecp256k1's `secp256k1_ecdsa_sign` of `d` (C API, as `secp256k1_c_api_test.cpp` calls it) and verifies with `crypto::verify(ecdsa_sha256, sig, prehashed{d}, parse_public_key(pk).value())`; `hedged_differs`: `sign(ecdsa_sha256, m, entropy)` differs from `sign(ecdsa_sha256, m)` and verifies; `empty_message` under `schnorr` and `ecdsa_sha256`. In `secp256k1_recovery_test.cpp`: `sign(ecdsa_sha256_recoverable, prehashed{d})` then `recover(ecdsa_sha256_recoverable, sig, prehashed{d})` is the signer's key, and with the message form likewise. BIP340's official vectors in `secp256k1_bip340_test.cpp` through the new calls (aux from the vectors).
- [ ] **Step 2: Run** `b2 ... toolset=clang libs/crypto/test//secp256k1` - Expected: compile errors.
- [ ] **Step 3: Implement** the interfaces above.
- [ ] **Step 4: Migrate every secp256k1 caller** listed above, the examples' `.expected` output unchanged unless a printed name changes; add `example/bitcoin_sighash.cpp` (and `.expected`, its `example/Jamfile` entry, and its `crypto.json` entry if examples are listed there): a fixed preimage's SHA-256d computed with `webcpp::hash::sha2_256` twice (the program's work, not crypto's), signed with `sign(crypto::ecdsa_sha256, crypto::prehashed{sighash})`, verified with `crypto::verify`, printed as DER; a comment says crypto computes no sighash (design D4c) and that a taproot sighash goes to `schnorr` as the message (see `musig_taproot.cpp`); `timing_coverage_secp256k1_test.cpp` checks each `sign` overload (all three schemes, hedged, explicit aux, drawn aux), both `crypto::verify` overloads and `recover`, each entering its own guard.
- [ ] **Step 5: Add message tests** `sign_schnorr_prehashed` (the `schnorr` message of Task 1) and `verify_ecdsa_bytes_key` (`crypto::verify(ecdsa_sha256, sig, message, std::array<std::uint8_t, 33>{})`: the catch-all's message), each `alone` with its `crypto.json` entry; regenerate.
- [ ] **Step 6: Run** `grep -rn 'sign_ecdsa\|verify_ecdsa\|sign_schnorr\|verify_schnorr\|sign_recoverable\|normalize_ecdsa\|check_low_s' libs/crypto --include=*.hpp --include=*.cpp --include=*.supp --include=Jamfile` - Expected: nothing. Then all of `libs/crypto/test libs/crypto/example libs/crypto/bench` on clang, through `heavy.sh crypto-sign` - Expected: all pass.
- [ ] **Step 7: Commit** `crypto: secp256k1 signs with ecdsa_sha256, ecdsa_sha256_recoverable and schnorr`.

### Task 4b: The backend first, Ed25519's policy as an argument (owner decision R14)

**Files:**
- Modify: `include/webcpp/crypto/curves/p256.hpp`, `curves/ed25519.hpp`, `curves/secp256k1.hpp`, `async.hpp` (internal calls only), the tests, message tests (`ed25519_unknown_policy` and its MSVC twin), examples, benches and superproject fixtures that name a policy or a backend in a `crypto::verify` or `recover` call.

**Interfaces (Produces):**
- Every `crypto::verify` overload has `Backend` as its first template parameter: P-256 `template <class Backend, class Input> verify(ecdsa_sha256_t, std::span<std::uint8_t const, 64>, Input const&, curves::p256::basic_public_key<Backend> const&)` (deduced; an explicit other backend does not compile); Ed25519 `template <class Backend = default_backend_t<families::ed25519>, class Input, class Policy = curves::ed25519::strict_t> verify(eddsa_t, std::span<std::uint8_t const> signature, Input const& message, std::span<std::uint8_t const> public_key, Policy policy = {})`; secp256k1 `template <class Backend = backends::secp256k1, class Input> verify(ecdsa_sha256_t, ...)` and `verify(schnorr_t, ...)`, each `static_assert(detail::secp256k1_refuses_others<families::secp256k1, Backend>)` as the old free functions did; `template <class Backend = backends::secp256k1, class Input> recover(ecdsa_sha256_recoverable_t, ...)`.
- Ed25519's policy is a tag object: `struct strict_t { explicit strict_t() = default; }; inline constexpr strict_t strict{};` in `curves::ed25519`; the policy `static_assert` keeps its text, naming `curves::ed25519::strict`. `ph` keeps its own strict verification.
- [ ] **Step 1: Tests**: `crypto::verify<backends::sodium>(crypto::eddsa, sig, msg, key)` and the same with `curves::ed25519::strict` as fifth argument pass; `crypto::verify<backends::secp256k1>(crypto::schnorr, ...)` and `(crypto::ecdsa_sha256, ...)` pass; `crypto::verify<backends::bearssl>(crypto::ecdsa_sha256, sig, msg, p256_key)` passes; a `static_assert` through a dependent requires-expression that `crypto::verify<backends::openssl>(ecdsa_sha256, sig, msg, basic_public_key<backends::bearssl>)` is not callable; `recover<backends::secp256k1>(...)` passes; the message test `ed25519_unknown_policy` passes an unknown policy object as the fifth argument and keeps its expected text.
- [ ] **Step 2: Implement**, migrate every call that passed `<strict, ...>` or `<Policy, Backend>`; grep `verify<` across libs/crypto and the superproject's fixtures.
- [ ] **Step 3: Run** the touched tests on clang and gcc-15, the message tests, timing coverage of all three backends, `generate.py --check`, and one whole clang build of `libs/crypto/test libs/crypto/example libs/crypto/bench` through `heavy.sh crypto-sign`.
- [ ] **Step 4: Commit** `crypto: crypto::verify takes the backend first on every curve, and Ed25519's policy as an argument`.

### Task 5: `async_sign` and `async_verify` take the scheme

**Files:**
- Modify: `libs/crypto/include/webcpp/crypto/async.hpp`, `test/async_p256_test.cpp`, `test/async_25519_test.cpp`, `test/async_post_test.cpp`, `timing_coverage_bearssl_test.cpp` and `timing_coverage_sodium_test.cpp` (their async calls), `example/async_sign.cpp`, `test/messages/async_sign_secp256k1/`, `test/msvc/async_sign_secp256k1/`.
- Create: message tests `async_sign_ed25519_prehashed`, `async_verify_wrong_key`.

**Interfaces:**
- Consumes: Tasks 1-4.
- Produces:
  - `template <class Keypair, class Executor, class Scheme, class Input, class CompletionToken> auto async_sign(Executor const& executor, Keypair const& key_pair, Scheme scheme, Input const& input, CompletionToken&& token);` - guard; the existing WebCrypto refusal (`is_webcrypto_keypair_v`) first, unchanged text; then `static_assert` on `can_sign<Keypair, Scheme, Input>` after `detail::check_input` for the input reasons (one diagnostic); synchronous backends post `key_pair.sign(scheme, input)`; asynchronous backends call `async_engine<backend>::sign(executor, key_pair, scheme, input, token)`.
  - `template <class Backend = default_async_backend_t<families::ed25519>, class Executor, class Input, class CompletionToken> auto async_verify(Executor const&, eddsa_t, std::span<std::uint8_t const> signature, Input const& message, std::span<std::uint8_t const> public_key, CompletionToken&&);` and the same with a `curves::ed25519::strict_t` policy argument before the token (Task 4b's policy tag).
  - `template <class Backend, class Executor, class Input, class CompletionToken> auto async_verify(Executor const&, ecdsa_sha256_t, std::span<std::uint8_t const> signature, Input const& input, curves::p256::basic_public_key<Backend> const& public_key, CompletionToken&&);` - a signature of another size completes with `invalid_signature`, as today.
  - A catch-all `async_verify` constrained like Task 1's `crypto::verify`, `static_assert` text: `"webcpp::crypto: async_verify takes crypto::eddsa with Ed25519's 32-byte key or crypto::ecdsa_sha256 with a curves::p256::basic_public_key, the algorithms WebCrypto has: call crypto::verify for the others."`
  - The `async_engine` comment block names the new shapes (`sign(executor, key_pair, scheme, input, token)`, `verify(executor, scheme, signature, input, public_key, token)`).
- [ ] **Step 1: Write the failing tests**: `async_p256_test.cpp` - `async_sign(executor, key, crypto::ecdsa_sha256, message, token)` equals `key.sign(crypto::ecdsa_sha256, message)`; `async_verify(executor, crypto::ecdsa_sha256, signature, crypto::prehashed{sha256_of(message)}, public_key, token)` completes without error; a wrong-size signature completes with `invalid_signature`; the completion is never inline (existing checks kept). `async_25519_test.cpp` - the same with `eddsa`. A `static_assert(!crypto::can_sign<curves::p256::basic_keypair<stand_in_async_backend>, ecdsa_sha256_t, prehashed<32>>)` and the message form true, with `stand_in_async_backend` a test type for which `detail::asynchronous_backend_v` is specialized true (the concept must not instantiate the key pair).
- [ ] **Step 2: Run** `b2 ... toolset=clang libs/crypto/test//async_p256 libs/crypto/test//async_25519` - Expected: compile errors.
- [ ] **Step 3: Implement** the interfaces; update the file comment's P-256 and Ed25519 bullets.
- [ ] **Step 4: Migrate** the remaining async callers; `async_sign_secp256k1` keeps its refusal text and its synchronous line becomes `made->sign(crypto::ecdsa_sha256, message)`; add `async_sign_ed25519_prehashed` (the `eddsa` message) and `async_verify_wrong_key` (a secp256k1 `public_key` with `ecdsa_sha256`: the catch-all's message), each `alone` with `crypto.json` entries; regenerate.
- [ ] **Step 5: Run** the async tests on clang, the message tests, `example/async_sign`, and `b2 ... toolset=emscripten libs/crypto/test//async_p256 libs/crypto/test//async_25519` - Expected: all pass.
- [ ] **Step 6: Commit** `crypto: async_sign and async_verify take the scheme and the same inputs`.

### Task 5a: crypto's refusals never stop a dependent's reference (coordinator item)

Brief: `.superpowers/sdd/plan/task-5b-brief.md` in the superproject worktree (written before the key pair contract took the name 5b). Done in commits faa04b0, f876418, ef32cab.

### Task 5b: The key pair contract on Ed25519 and P-256 (owner decision R15)

**Files:**
- Create: `libs/crypto/include/webcpp/crypto/keypair.hpp`, `libs/crypto/test/keypair_test.cpp` (stand-in types that pass and fail the concept).
- Modify: `curves/ed25519.hpp` (new `ed25519::public_key`, `secret()`, the typed key in `crypto::verify` and `async_verify`, the `static_assert`), `curves/p256.hpp` (`secret_scalar` -> `secret`, `ecdh_x` -> `agree`, the `static_assert`), `curves/ed25519/formats.hpp`, `curves/p256/formats.hpp`, `async.hpp` (`async_agree` for P-256 calls `agree`; Ed25519's `async_verify` takes the typed key), every caller in libs/crypto (tests, examples, bench, constant-time harnesses, MSVC and message tests, `doc/tables/crypto.json` where it names functions) and the superproject's `tools/test/fixtures/crypto_jamfiles/` (edited, committed by the controller).

**Interfaces (Produces):**
- `template <class K> concept crypto::keypair` (design D8): `K::make(std::span<std::uint8_t const>)` and `K::generate()` give `result<K>`; `k.public_key()` gives `result<typename K::public_key_type>`; `k.secret()` gives `result<secret<K::secret_key_size>>`; for each scheme tag `S` with `detail::signs_with_v<K, S>`, `k.sign(S{}, std::span<std::uint8_t const>{})` is valid; at least one such `S`.
- `curves::ed25519::public_key`: `static result<public_key> make(std::span<std::uint8_t const>)` (`invalid_public_key` for a size other than 32, nothing else checked: design R16), `std::array<std::uint8_t, 32> bytes() const noexcept`, equality; `basic_keypair::public_key_type`, `secret_key_size` (32), `secret()` (the seed; the engine gets a `seed` function: libsodium's `crypto_sign_ed25519_sk_to_seed`). `crypto::verify(eddsa, signature, message, ed25519::public_key const&[, policy])`: the byte-key form goes (R17), `verifies_with_v`/`has_verifier_v` for byte keys under `eddsa` removed.
- P-256: `public_key_type = basic_public_key<Backend>`; `secret()` (was `secret_scalar`); `agree(basic_public_key<Backend> const&)` (was `ecdh_x`), `async_agree` calls it.
- In each curve header: `static_assert(crypto::keypair<basic_keypair<backends::...default...>>)` for the family's default where it is available, or a static_assert in the test per backend where a header cannot name one.
- Timing guards: `secret()` on Ed25519 calls the backend, so it holds the guard and joins `timing_coverage_sodium_test.cpp`; renamed functions keep their guards and coverage entries.
- [ ] **Step 1: Tests first**: `keypair_test.cpp` (the concept on both curves' key pairs, and false for a stand-in missing `secret()`); in `ed25519_sodium_test.cpp`: `make(secret())` round trip gives the same public key; RFC 8032 test 1 through `crypto::verify(eddsa, sig, msg, *key.public_key())`; `public_key::make` of 31 bytes is `invalid_public_key`; in `p256_*`: `agree` equals the old ECDH vectors, `secret()` round trip.
- [ ] **Step 2: Implement**, remove old names with no aliases, migrate every caller; grep `secret_scalar\|ecdh_x\|\.public_key()` uses that expected bytes.
- [ ] **Step 3: Run** touched tests on clang and gcc-15, Emscripten's `p256_webcrypto_oracle` and `formats_25519_webcrypto_oracle`, message tests, timing coverage, `generate.py --check`, and the whole clang build of `libs/crypto/test libs/crypto/example libs/crypto/bench` through `heavy.sh crypto-sign`.
- [ ] **Step 4: Commit** `crypto: the key pair contract, typed public keys and agree on Ed25519 and P-256`.

### Task 5c: The key pair contract on secp256k1 (owner decision R15)

**Files:**
- Modify: `curves/secp256k1.hpp` and its group headers (`musig.hpp`, `silent_payments.hpp`, `ellswift.hpp`, `formats.hpp`, `tweak.hpp`) where they take or give public keys as bytes from the key pair, every secp256k1 caller (tests, examples including `musig_network.cpp` and `musig_taproot.cpp`, bench, MSVC and message tests, fixtures).

**Interfaces (Produces):**
- `secp256k1::public_key`: `static result<public_key> make(std::span<std::uint8_t const>)` (33 or 65 bytes; was `parse_public_key`, with its `Backend` template parameter first if it had one), `compressed()` (33), `uncompressed()` (65), `xonly()` (32), equality and ordering as today. `parse_public_key` and `serialize_public_key` removed.
- Key pair: `public_key_type = public_key`, `secret_key_size` (32), `public_key()` gives `result<public_key>`; `xonly_public_key()` and `public_key_uncompressed()` removed; `secret()` (libsecp256k1's `secp256k1_keypair_sec`, guarded); `agree(public_key const&)` (raw x, was `ecdh_x`), `agree_hashed(public_key const&)` (was `ecdh`), `agree_point(public_key const&)` (was `ecdh_point`); the private template that served them renamed so nothing public is called `agree` but these.
- `crypto::verify(schnorr, signature, message, public_key const&)` added beside the 32-byte x-only form; `crypto::verify(ecdsa_sha256, ...)` unchanged.
- `static_assert(crypto::keypair<basic_keypair<backends::secp256k1>>)`.
- MuSig2 and silent payments keep their names; where they take a peer's key as bytes they may keep bytes (protocol wire forms) - record each choice in the report.
- [ ] **Step 1: Tests first**: the concept; `make(secret())` round trip; `agree` symmetric between two key pairs and equal to the old `ecdh_x` vectors; `agree_hashed` equals libsecp256k1's `secp256k1_ecdh` default; `crypto::verify(schnorr, ...)` with the typed key and with `xonly()` both pass.
- [ ] **Step 2: Implement**, remove old names with no aliases, migrate every caller.
- [ ] **Step 3: Run** as Task 5b's Step 3, with secp256k1's targets.
- [ ] **Step 4: Commit** `crypto: the key pair contract, typed public keys and agree on secp256k1`.

### Task 6b: One list of schemes (owner decision R20)

**Files:** `include/webcpp/crypto/signing.hpp`, `keypair.hpp`, `test/signing_test.cpp`, `test/keypair_test.cpp`, and any hand enumeration of the four tags (grep `eddsa_t.*ecdsa_sha256_t\|schnorr_t.*eddsa_t` and similar) in headers, tests and `doc/tables/generate.py`.

**Interfaces (Produces):** `template <class... Schemes> struct detail::scheme_list {};` and `using detail::all_schemes = scheme_list<eddsa_t, ecdsa_sha256_t, ecdsa_sha256_recoverable_t, schnorr_t>;` beside the tags; `detail::is_scheme_v<T>` derived from it; `crypto::keypair`'s "signs at least one scheme" and "signs as declared for each scheme" checks fold over a `scheme_list` parameter defaulted to `all_schemes` (e.g. a `detail::keypair_over<K, List>` concept with `keypair<K> = keypair_over<K, all_schemes>`), so a test can pass a list with a test-only scheme.

- [ ] **Step 1: Test**: a test-only scheme tag and a stand-in key pair that declares it (`signs_with_v`) but lacks its `sign` overload fail `keypair_over<K, scheme_list<..., test_scheme>>`, and pass when the overload exists; `is_scheme_v` true for each member of `all_schemes`.
- [ ] **Step 2: Implement**; no enumeration of the four tags by hand remains where it can derive from the list (record any that cannot, e.g. a generator that reads the header's text).
- [ ] **Step 3: Run** signing, keypair, the curves' tests and message tests on clang and gcc-15; `generate.py --check`.
- [ ] **Step 4: Commit** `crypto: one list of schemes, which the key pair concept folds over`.

### Task 5d-1: Member verify and the free synchronous forms (owner decision R21)

**Interfaces (Produces):** `pub.verify(scheme, signature, input[, policy])` on `ed25519::public_key` (Backend a defaulted member template parameter first, policy trailing), `p256::basic_public_key<B>` and `secp256k1::public_key` (`ecdsa_sha256` and `schnorr`, R22), each forwarding to the same worker as `crypto::verify` (one implementation; `crypto::verify` may forward to the member); `crypto::sign(key, scheme, input[, extra])`, `crypto::make<A>(bytes)`, `crypto::generate<K>()`, `crypto::agree(key, peer)`, `crypto::seal(aead, iv, ad, data)`, `crypto::open(aead, iv, ad, data, tag)` as one-line wrappers (R23); on a backend with `detail::asynchronous_backend_v`, every synchronous form refuses with "webcpp::crypto: <name> on this backend completes asynchronously: call async_<name>." (members and free forms; test with a stand-in only where it does not instantiate a refusing engine, else record it for part B).
- [ ] Tests: each operation in its member and free synchronous forms gives equal results on every curve and AES-GCM; message tests for misuse of the new forms (wrong scheme through `crypto::sign`, `pub.verify` with a marker under `eddsa`).
- [ ] Commit `crypto: every typed public key verifies, and each operation has its free synchronous form`.

### Task 5d-2: The asynchronous members (owner decision R21)

**Interfaces (Produces):** `K::async_make(ex, bytes, token)`, `K::async_generate(ex, token)`, `key.async_sign(ex, scheme, input, token)`, `key.async_agree(ex, peer, token)` (P-256), `pub.async_verify(ex, scheme, signature, input[, policy], token)` (Ed25519, P-256), `aead.async_seal(ex, iv, ad, data, token)`, `aead.async_open(ex, iv, ad, data, tag, token)` (AES-GCM): declared in the classes as member templates with `auto` return type and defined out of class in `async.hpp`; the free `async_*` functions and the members share one initiation each. secp256k1 has none (R8).
- [ ] Tests: each operation in all four forms gives equal results (`async_p256`, `async_25519`, `async_gcm`); a message test calling a member `async_` function without including `async.hpp` pins the compiler's diagnostic on clang (record gcc's and MSVC's wording in the report).
- [ ] Commit `crypto: the asynchronous member functions, defined in async.hpp`.

### Task 6c: The page shows the four forms (amends Task 6)

- [ ] Each curve's and AES-GCM's chapter shows the synchronous and asynchronous members side by side; the guide (`curves.adoc` or `async.adoc`) shows the free forms and D9's table; `crypto.json` rows and the changelog name what changed (within the 10-line, 30-word rules); `-a libs/crypto/doc` passes.

### Task 6: The page, the generated files and the changelog

**Files:**
- Modify: `libs/crypto/doc/p256.adoc`, `ed25519.adoc`, `secp256k1.adoc`, `async.adoc`, `curves.adoc`, `backends.adoc`, `p2p.adoc`, `wallets.adoc`, `everyday.adoc`, `overview.adoc` and any `.adoc` that names a removed function; `doc/tables/crypto.json` (the use cases and backend rows that name functions); `doc/changelog.adoc` (unreleased lines); `README.md` if it shows a signature; then regenerate.
- Create: a section "Signatures: the scheme and the input" in `doc/curves.adoc` (or the page that introduces the curves), with design.md's D3 table (low S signed on both curves, verified per curve), the format helpers (D4b), the hazard it prevents, the Bitcoin sighash with `example/bitcoin_sighash.cpp` included by tag (D4c), the Ethereum note (design.md Risks), and links from each curve's page.
- [ ] **Step 1:** `grep -rn 'sign_ecdsa\|verify_ecdsa\|sign_schnorr\|verify_schnorr\|sign_recoverable\|normalize_ecdsa\|check_low_s\|ed25519::verify\|\.sign(message\|p256_digest' libs/crypto --include=*.adoc --include=*.json --include=*.md --include=*.py --include=*.txt` and rewrite each hit (prose and code) in the new form; code snippets in the page must be the examples' own code where the page includes them by tag.
- [ ] **Step 2:** In `changelog.adoc`'s unreleased list, edit the lines that name signing so they describe the final API (each at most 30 words), for instance the secp256k1 line names "sign with crypto::ecdsa_sha256, ecdsa_sha256_recoverable and schnorr", and add one line: "Added signing.hpp: sign(scheme, input) and crypto::verify(scheme, signature, input, key) on every curve, synchronously and asynchronously; a digest only as crypto::prehashed."
- [ ] **Step 3: Run** `python3 libs/crypto/doc/tables/generate.py` then `--check` (exit 0), `python3 libs/crypto/doc/tables/generate_test.py` (pass), and the doc build of `libs/crypto/doc` (`b2 ... libs/crypto/doc` as the superproject builds it) - Expected: no warning about a missing anchor or include.
- [ ] **Step 4: Commit** `crypto: the page and its tables name sign and crypto::verify with their schemes`.

### Task 7: Full local verification and delivery (controller)

- [ ] clang, gcc-15 and Emscripten full lanes of `libs/crypto/test libs/crypto/example libs/crypto/bench` (`-a`), each through `heavy.sh crypto-sign`.
- [ ] macOS lint (full) and linux-lint (`--full`, on a self-contained full clone made with a `make-git-tree.py` like trystero-027's), both through `heavy.sh`.
- [ ] msvc-lane 14.3 and 14.5 on a self-contained copied tree (`run-lane.sh <v> --tree DIR --fresh -- libs/crypto/test libs/crypto/example`), through `heavy.sh`; `async_p256_no_exceptions` is the known Wine artifact.
- [ ] Merge `origin/main` into the three branches; resolve; re-run what the merge touches.
- [ ] Pull requests in webcpporg/crypto, webcpporg/proposals (this change, its tasks ticked, design.md's "As built"), webcpporg/webcpp (the pointer); merge with merge commits; cancel the CI runs; delete the branches.
