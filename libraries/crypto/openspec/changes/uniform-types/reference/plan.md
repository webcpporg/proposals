# Uniform types Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every signature and every X25519 key of webcpp::crypto has a type of its own; Ethereum's recoverable ECDSA is a scheme; key pairs, AEADs and boxes are checked by concepts; constructions are grouped by purpose; every fallible factory is named by its input; plus three small fixes (a throwing `getRandomValues` in the browser, a public `crypto::wipe`, no one-word local in a header).

**Architecture:** One class template `detail::basic_signature<Tag, Size>` (new `detail/signature.hpp`) gives the five signature types (and Ed25519ph's) by tag; the trait `detail::signature_of<Key, Scheme>` pairs a key with its signature type, and every `verify` overload takes `Signature const&` and checks it with a `static_assert` that names the type expected. `curves/x25519.hpp` gains `public_key` and `basic_keypair<B>`; `key_exchange/kx.hpp`, `public_key_encryption/box.hpp` and `sealed_box.hpp` (moved from `curves/x25519/` and `hybrid/`) take them. `keypair.hpp` splits the contract into `keypair`, `signing_keypair`, `agreement_keypair`; a new `aead.hpp` holds `aead`, `streaming_aead`, `authenticated_box` and the free `seal`/`open`; `factories.hpp` (was `make.hpp`) holds `from_secret`, `from_bytes`, `from_key`, `generate`. `async.hpp` defines every asynchronous member, as on crypto/sign-unified. Every caller of a removed name moves in the task that removes it (design D7), so libs/crypto builds and its tests pass at the end of every task.

**Tech Stack:** C++20, header-only, b2 (`webcpp.run`, `webcpp.compile-diagnostic`, `webcpp.headers-alone`), BearSSL, libsodium, libsecp256k1, OpenSSL, ctaes, webcpp::hash, webcpp::asio; Emscripten's `EM_JS`.

**Spec:** `libraries/crypto/openspec/changes/uniform-types/` in webcpporg/proposals (proposal.md; design.md with owner decisions 1-10, D1-D8, R1-R20; specs/crypto/spec.md; tasks.md). Binding; read design.md D1, D3c and D5 before any task.

**Order, and why:** X3, 2.5, 2.4, X2, 1.1, 1.2, 1.3, 2.1, 2.2, 2.3, 3.1, 3.2, 3.3, X1, X1b, 4.1, 4.2.
- X3 first (the controller's ruling): mechanical, so every later task writes its code with the new local names.
- 2.5 (factories) second, before any task creates a type: the signature types, X25519's key pair and kx's factories are then born with their final names (`from_bytes`, `from_secret`, `from_keypair`, `derive_from_seed`) instead of a `make` that a later task renames across ~250 files a second time, and 3.1's concept can require `from_secret` without a migration of its own. The cost: 2.5 renames crypto_box's byte factory to `from_keys(secret_bytes, peer_bytes)`, which 2.2 retypes (about ten call sites edited twice).
- 2.4 (namespaces) third, also mechanical, so that 2.2 edits the moved files once.
- X2 is independent and small; 1.1-1.2 next (no dependency on X25519); 1.3 (owner decision 10) right after 1.2, whose typed `recoverable_signature` it returns; 2.1 before 2.2 and 2.3 (both need the X25519 types); 3.1 needs every key pair typed; 3.2 before 3.3; X1 and X1b are independent, placed before 4.1 so that the page and the changelog see them; X1b after X1, whose entropy path it reuses.

**Build and test:** from the superproject worktree, `b2 --user-config=/Users/you/dev/webcpporg/webcpp/.local/user-config.jam -j10 toolset=clang <targets>` (written `b2 ... toolset=clang` below), targets as `libs/crypto/test//x25519_sodium`, `libs/crypto/test/messages//make_removed`, `libs/crypto/example//kx_session`. A whole-directory build (`libs/crypto/test libs/crypto/example libs/crypto/bench`) and any gcc, Emscripten or MSVC run go through `/Users/you/dev/webcpporg/heavy.sh crypto-types`. Emscripten: `EM_CACHE=/Users/you/dev/webcpp/.local/emscripten-cache b2 ... toolset=emscripten`. Generated files: `python3 libs/crypto/doc/tables/generate.py`, then `--check` (exit 0) and `python3 libs/crypto/doc/tables/generate_test.py`. Fixtures: `python3 tools/test/crypto_jamfiles_test.py <case>... [--target native]`.

**Superproject touch points** (separate superproject commits, listed per task): `tools/test/fixtures/crypto_jamfiles/*` (2.5, 2.4, 1.2, 2.1, 2.2, 2.3, 3.2), `CODING-GUIDELINES.md` crypto chapter (4.1), the `libs/crypto` pointer (4.2). Increment 1's dated record in CODING-GUIDELINES.md (around line 8753, "`hybrid::box` and `sealed_box`") keeps the names of its time.

## Global Constraints

- Worktree `/Users/you/dev/webcpporg/crypto-types/webcpp` (superproject, branch `crypto/uniform-types` from d53bf53); `libs/crypto` on branch `crypto/uniform-types` from 0255206. Commit in `libs/crypto` unless a task says superproject.
- Read `libs/crypto/AGENTS.md`, the superproject's `AGENTS.md` and `CODING-GUIDELINES.md` (crypto chapter) first: doc comments, `[[nodiscard]]`, `noexcept`, include order, the timing guard rule, generated files, message tests (`fixes` in `crypto.json` in the same commit).
- English only. Never the em dash character; a plain "-". Commit messages `crypto: <what the change makes true>`, no trailer of any kind.
- Never edit a generated file (`doc/generated/*`, `doc/llms.txt`, `detail/capabilities.hpp`, `detail/defaults.hpp`, `test/capabilities_test.cpp`, the defaults' aliases in `build.jam`) nor `CHANGELOG.md`, `doc/tables/sizes.json`, `doc/performance.json`; the changelog is `doc/changelog.adoc`'s unreleased list (10 lines at most, 30 words each), edited in 4.1 only.
- Removed with no alias, shim or deprecation (D7, D3c): every `make`/`async_make` (member and free) and `make.hpp`; `curves::x25519::public_key(secret)`, `curves::x25519::agree(secret, peer)`, the byte-key `async_agree`; `x25519_secret()`, `to_x25519_public`; `crypto::hybrid`, `hybrid/box.hpp`, `hybrid/sealed_box.hpp`, `curves/x25519/kx.hpp`, `curves::x25519::kx_keypair`, `session_keys`; every `verify`/`recover` taking a signature as bytes; `sign`'s array results; secretbox's additional data and its `ciphers::aead` aliases.
- A public function that calls a backend holds `detail::timing_guard const timing_scope;` (X3's name) first and joins `test/timing_coverage_<backend>_test.cpp`; one that calls none (every `from_bytes`, `bytes()`, X25519's `public_key()` and `secret()`, `crypto::wipe`) is checked to enter none.
- On a backend whose `detail::asynchronous_backend_v` is true every synchronous member refuses with "webcpp::crypto: <name> on this backend completes asynchronously: call async_<name>." (as P-256's do).
- A flaky or failing test or a lint warning seen on the way is fixed or reported to the controller, never skipped.

## Review Focus

1. A signature of the wrong length from the wire: 63 and 65 bytes (64 and 66 for the recoverable type) give `invalid_signature` from `from_bytes` on all six types, and nothing reaches a backend (Task 1.1, `signature_from_wire`).
2. A moved-from X25519 key pair: `public_key()`, `secret()`, `agree` return `invalid_state`, and the source's secret storage reads zero (Task 2.1, `moved_from`).
3. A failed `open` of a secret box or a crypto_box zeroes `data`; a guard's refusal (a 12-byte nonce) leaves it untouched (Task 3.2 and 2.2, `open_failures`).
4. `async_seal`/`async_open` of every new AEAD and box on a synchronous backend never complete inside the initiating call (Task 3.3, `never_inline`).
5. A throwing `crypto.getRandomValues` gives `entropy_unavailable` with the output zeroed, and a missing `crypto` object `not_supported`, never a terminated page (Task X1).

---

### Task X3: No one-word local in a header (MSVC C4459)

crypto-sign's handback: a program global named like a header's local (`timing`, `executor`, `token`, `state`) trips C4459, an error at /W4 /WX. There is no existing "hides no user name" test: 48ca7ef renamed `make.hpp`'s parameters and a200e69 moved a test's global into a local. This task creates the test.

**Files:**
- Modify: every header under `libs/crypto/include/webcpp/crypto/` (backends/, detail/, curves/, ciphers/, hybrid/, root), except the generated `detail/capabilities.hpp` and `detail/defaults.hpp` (if they hold a one-word local, change `doc/tables/generate.py` and regenerate).
- Create: `libs/crypto/test/user_globals_test.cpp`; `test/Jamfile`: `crypto-run user_globals : user_globals_test.cpp : <library>/third_party//bearssl <library>/third_party//ctaes <library>/third_party//secp256k1 <library>/third_party//sodium <library>/webcpp/crypto//openssl <library>/webcpp/asio//asio : native ;`

- [ ] **Step 1: Survey the names.** An aggregate translation unit of every public header, its AST dumped, every `ParmVarDecl`, `VarDecl` and `BindingDecl` inside a function of `libs/crypto/include` whose name matches `^[a-z][a-z0-9]*$`:

```sh
cd /Users/you/dev/webcpporg/crypto-types/webcpp
(cd libs/crypto/include && find webcpp -name '*.hpp' | sort | sed 's/.*/#include <&>/'; echo 'int main() {}') > /tmp/all.cpp
clang++ -std=c++20 -fsyntax-only -Xclang -ast-dump $INCLUDES \
  -DWEBCPP_CRYPTO_HAS_BEARSSL=1 -DWEBCPP_CRYPTO_HAS_SECP256K1=1 -DWEBCPP_CRYPTO_HAS_SODIUM=1 \
  -DWEBCPP_CRYPTO_HAS_CTAES=1 -DWEBCPP_CRYPTO_HAS_OPENSSL=1 -DWEBCPP_CRYPTO_HAS_THREADS=1 /tmp/all.cpp > /tmp/ast.txt
python3 - /tmp/ast.txt <<'EOF'
import re, sys, collections
here, seen = None, collections.Counter()
for line in open(sys.argv[1], errors='replace'):
    m = re.search(r'[<, ]([\w./-]+\.(?:hpp|h)):\d+:\d+', line)
    if m: here = m.group(1)
    d = re.search(r'(ParmVarDecl|VarDecl|BindingDecl) 0x\w+ <[^>]*> \S+(?: used| referenced)* ([a-z][a-z0-9]*) ', line)
    if d and here and 'libs/crypto/include/' in here: seen[d.group(2)] += 1
for name, count in seen.most_common(): print(count, name)
EOF
```

`$INCLUDES` is `-I` of `libs/*/include`, the third-party include directories (BearSSL `inc`, libsecp256k1 `include`, libsodium `src/libsodium/include`, ctaes, OpenSSL's) and Boost. Drop namespace-scope variables (`eddsa`, `schnorr`, `strict`, the generated tables' `supported`, `linked`) and names the location tracking attributes to std's algorithms. The survey on 0255206 found (count): timing 162, executor 39, token 33, out 26, keyed 19, s 18, ready 14, signature 14, key 13, scheme 11, message 10, data 9, work 9, c 9, size 8, encrypt 8, decrypt 8, iv 8, tag 8, output 8, other 8, handler 8, borrow 7, i 7, text 7, input 7, der 6, label 6, prefix 6, allocator 6, code 6, state 5, decoded 5, at 5, begun 5, run 5, count 4, expected 4, name 4, half 4, characters 4, bytes 4, seed 4, done 4, and some 70 names of 1 to 3 uses (verdict, order, sealed, held, policy, nonce, digest, peer, scalar, point, receive, transmit, beforenm, spki, pkcs8, piece, agreed, opened, guard, job, ...; `value`, `extent`, `implicit` to confirm by hand). Record the confirmed list in the test's comment.

- [ ] **Step 2: Write the failing test `user_globals_test.cpp`.** (1) Every header of another library that a crypto header includes (`grep -rhoE '#include <[^>]+>' libs/crypto/include | grep -v webcpp/crypto | sort -u`: Boost, webcpp::asio, webcpp::hash, `<sodium/...>`, `<secp256k1*.h>`, `<bearssl*.h>`, `<openssl/...>`, `<ctaes.h>`, the standard library), first, so that their own names are parsed before the globals exist. (2) In the global namespace, `extern int const <name>;` for every confirmed name. (3) Every public header: `<webcpp/crypto.hpp>`, `<webcpp/crypto/async.hpp>`, `<webcpp/crypto/backends/openssl.hpp>` and each header the convenience header leaves out. (4) `int main() { return 0; }`. File comment: why it exists (C4459 at /W4 /WX; clang and GCC never warn across namespaces, so the test is MSVC's) and how the list was made.
- [ ] **Step 3: See it fail on MSVC:** `webcpp-msvc 14.5 -- libs/crypto/test//user_globals` - Expected: FAIL, C4459 "declaration of 'timing' hides global declaration" and one per name. On clang it passes already (`b2 ... toolset=clang libs/crypto/test//user_globals`). If MSVC reports C4459 inside webcpp::hash or another library, stop and report to the controller (out of scope).
- [ ] **Step 4: Rename** every confirmed local and parameter, rule "the role, then the kind", two words joined by an underscore; doc comments' `@param` follow. Fixed names:

| one word | new name | one word | new name |
|---|---|---|---|
| `timing` (`timing_guard const`) | `timing_scope` | `executor` (cheap ops, completion) | `completion_executor` |
| `work` (heavy.hpp's executor) | `work_executor` | `token` | `completion_token` |
| `handler` | `completion_handler` | `code` | `completion_code` |
| `state` (heavy.hpp) | `operation_state` | `state` (timing.hpp) | `saved_state` |
| `s` (`state& s`, backends) | `engine_state` | `keyed` | `keyed_state` |
| `key` (bytes / state) | `key_bytes` / `key_state` | `out`, `output` | `output_bytes` |
| `ready` | `library_ready` | `signature` | `signature_bytes` |
| `scheme` | `signing_scheme` | `input` | `signing_input` |
| `message` | `message_bytes` | `data`, `iv`, `tag`, `nonce`, `digest`, `seed`, `scalar`, `point`, `der` | `<word>_bytes` |
| `peer` | `peer_key` | `policy` | `verification_policy` |
| `encrypt`, `decrypt` (libsodium calls) | `encrypt_call`, `decrypt_call` | `i` (limbs) | `limb_index` |
| `c`, `text`, `label` (pem.hpp) | `pem_character`, `pem_text`, `pem_label` | `size`, `count` | `byte_count`, `item_count` |

- [ ] **Step 5: Run** the survey again - Expected: only the dropped namespace-scope names. `webcpp-msvc 14.5 -- libs/crypto/test//user_globals` - Expected: PASS. Then the clang whole build through `heavy.sh crypto-types` of `libs/crypto/test libs/crypto/example libs/crypto/bench` and `python3 libs/crypto/doc/tables/generate.py --check` - Expected: all pass.
- [ ] **Step 6: Commit** `crypto: no local name in a header hides a program's global on MSVC`.

### Task 2.5: Factories named by their input (D3c, owner decision 9)

**Files:** `include/webcpp/crypto/make.hpp` -> `factories.hpp` (`git mv`, guard `WEBCPP_CRYPTO_FACTORIES_HPP`), `crypto.hpp`, `keypair.hpp`, `async.hpp`, `ciphers/{aes,gcm,aead}.hpp`, `curves/{ed25519,p256,secp256k1}.hpp` and their `formats.hpp`, `curves/x25519/kx.hpp`, `hybrid/box.hpp`, `backends/{bearssl,openssl,sodium,secp256k1}.hpp` (engine renames), `detail/{p_curves,heavy}.hpp`, `meta/include-boundaries.json`, `test/Jamfile` (headers-alone list), every caller (`grep -rlE '\b(async_)?make\b' libs/crypto`: ~169 tests, 37 examples, bench, 12 doc pages, `crypto.json`, the MSVC and message tests, `test/constant_time/*`); new message test `test/messages/make_removed/`. Superproject: `tools/test/fixtures/crypto_jamfiles/*` (51 files).

**Interfaces (Produces):**

| Class | Before | After |
|---|---|---|
| `ciphers::aes128/192/256` (aes.hpp) | `make(key)` | `from_key(key)` |
| `ciphers::gcm<C>` | `make`, `async_make` | `from_key`, `async_from_key` |
| `ciphers::aead<F,B>` (and secretbox, an alias until 3.2) | `make(key)` | `from_key(key)` |
| `hybrid::basic_box` | `make(secret_key, peer_public_key)` | `from_keys(secret_key, peer_public_key)` (bytes until 2.2) |
| `curves::x25519::kx_keypair` | `make(seed)` | `derive_from_seed(seed)` (`generate()` unchanged) |
| `ed25519::public_key`, `secp256k1::public_key` | `make(bytes)` | `from_bytes(bytes)` (secp256k1's keeps its `Backend` parameter first) |
| `p256::basic_public_key<B>` | `make(point)`, `make(x, y)` | `from_bytes(point)`, `from_coordinates(x, y)` |
| `ed25519`, `p256`, `secp256k1` `basic_keypair<B>` | `make(secret[, memory_resource])`, `async_make` | `from_secret(secret_bytes[, memory_resource])`, `async_from_secret` |
| free (`factories.hpp`) | `crypto::make<A>(...)` | `from_secret<K>(...)`, `from_bytes<T>(...)`, `from_key<C>(...)`; `generate<K>(...)` moves here unchanged |
| free (`async.hpp`) | `async_make<A>(ex, bytes, token)` | `async_from_secret<K>(ex, secret_bytes, token)` (key pairs WebCrypto has), `async_from_key<C>(ex, key_bytes, token)` (AES-GCM) |
| detail engines | `p_curve_engine::make`, Ed25519's `engine::make`, `kx_engine::make`, `heavy_state::make` | `import_secret`, `seed_keypair`, `seed_keypair`, `allocate` (ruling P4) |

- Each free factory: `template <class Keypair, class... Arguments> requires requires { Keypair::from_secret(std::declval<Arguments>()...); } [[nodiscard]] auto from_secret(Arguments&&... forwarded_arguments) noexcept(noexcept(...)) -> decltype(Keypair::from_secret(std::forward<Arguments>(forwarded_arguments)...))`, holding the guard as `make` did; `from_bytes<Type>` and `from_key<Cipher>` the same over their members.
- `async.hpp`: `detail::initiate_make` becomes `initiate_from_secret<K>` and `initiate_from_key<C>`; `async_engine`'s documented shapes `from_secret<Keypair>(executor, secret_bytes, token)` and `from_key<Cipher>(executor, key_bytes, token)`; refusal texts name `async_from_secret`/`async_from_key`.
- `keypair.hpp`'s concept requires `Keypair::from_secret(std::span<std::uint8_t const>&)`.
- Asynchronous refusals: "webcpp::crypto: from_secret on this backend completes asynchronously: call async_from_secret." (and `from_key`/`async_from_key`).

- [ ] **Step 1: Failing tests.** `test/synchronous_forms_test.cpp`: `crypto::from_secret<p256::keypair>(scalar)`, `<ed25519::keypair>(seed)`, `<secp256k1::keypair>(secret)` have the members' public keys; `crypto::from_bytes<p256::public_key>(point) == p256::public_key::from_bytes(point)`; `p256::public_key::from_coordinates(x, y) == from_bytes(04 || x || y)`; `crypto::from_key<gcm>(key)` seals as `gcm::from_key(key)`. `test/kx_sodium_test.cpp`: `kx_keypair::derive_from_seed(seed)` gives the keys the old `make(seed)` vectors give. `test/async_p256_test.cpp`, `async_25519_test.cpp`, `async_gcm_test.cpp`: the `async_from_secret`/`async_from_key` forms, member and free.
- [ ] **Step 2: Run** `b2 ... toolset=clang libs/crypto/test//synchronous_forms` - Expected: compile errors, no `from_secret`.
- [ ] **Step 3: Implement** the table; `git mv make.hpp factories.hpp`; `include-boundaries.json` names `factories.hpp`.
- [ ] **Step 4: Migrate every caller**, the doc pages' prose and code (`curves.adoc`'s "`crypto::make` ... in `make.hpp`"), `crypto.json` (the root row "`make`, `generate`: Making by type" becomes "`from_secret`, `from_bytes`, `from_key`, `generate`: Making by type", the `fixes` texts that say `make`, the message `async_make_xchacha` renamed `async_from_key_xchacha` with its MSVC twin); regenerate.
- [ ] **Step 5: Message test `make_removed`**: `crypto::curves::p256::keypair::make(scalar)` and `crypto::make<crypto::curves::p256::keypair>(scalar)`, two `expected-error` lines (`{{no member named 'make' in}}`, substring), `alone`, native wasip2 wasip3, its `crypto.json` entry; regenerate.
- [ ] **Step 6: Run** `grep -rnE '\b(async_)?make\b *(<[^>]*>)? *\(|make\.hpp' libs/crypto/include libs/crypto/test libs/crypto/example libs/crypto/bench libs/crypto/doc/*.adoc libs/crypto/doc/tables/crypto.json tools/test/fixtures/crypto_jamfiles` - Expected: only `make_removed`'s two lines. Then the clang whole build through `heavy.sh crypto-types`, `generate.py --check`, and `python3 tools/test/crypto_jamfiles_test.py --target native` - Expected: all pass.
- [ ] **Step 7: Commit** `crypto: every factory is named by its input, and make is gone`; superproject: `crypto fixtures: key pairs are made by from_secret, public keys by from_bytes, ciphers by from_key`.

### Task 2.4: Constructions grouped by purpose (D3b, owner decision 8)

**Files:** `git mv` `hybrid/box.hpp` -> `public_key_encryption/box.hpp`, `hybrid/sealed_box.hpp` -> `public_key_encryption/sealed_box.hpp`, `curves/x25519/kx.hpp` -> `key_exchange/kx.hpp` (guards renamed); `crypto.hpp`, `test/Jamfile` (headers-alone), `doc/hybrid.adoc` -> `doc/public_key_encryption.adoc` (anchor `crypto-public-key-encryption`, every xref; `crypto.adoc`'s include), `doc/kx.adoc`, `crypto.json` (`namespaces` entries and rows naming the headers or types), callers (14 tests, 2 examples, `test/constant_time/paths.hpp`, the MSVC and message tests `box_*`, `kx_*`); new message tests `hybrid_header_removed`, `hybrid_names_removed`. Superproject: fixtures `box_*`, `sealed_box_default`, `kx_*` (8 cases).

**Interfaces (Produces):** namespaces `webcpp::crypto::public_key_encryption` (`basic_box`, `box`, `box_xchacha20poly1305`, `basic_sealed_box`, `sealed_box`) and `webcpp::crypto::key_exchange` (`kx_keypair`, `session_keys`); `detail::box_engine`, `box_state`, `sealed_box_engine`, `kx_engine` keep their names; families and capability rows unchanged.

- [ ] **Step 1: Failing tests**: `box_sodium_test.cpp`, `kx_sodium_test.cpp` include the new headers and name the new namespaces. Run `b2 ... toolset=clang libs/crypto/test//box_sodium libs/crypto/test//kx_sodium` - Expected: file not found.
- [ ] **Step 2: Implement** the moves; the file comments say why the namespaces (D3b: "hybrid" names post-quantum hybrids; HPKE later goes to `public_key_encryption`).
- [ ] **Step 3: Migrate** every caller and page; regenerate.
- [ ] **Step 4: Message tests** `hybrid_header_removed` (`#include <webcpp/crypto/hybrid/box.hpp>`, `{{'webcpp/crypto/hybrid/box.hpp' file not found}}`) and `hybrid_names_removed` (`crypto::hybrid::box<>`, `crypto::curves::x25519::kx_keypair<>`: `{{no member named 'hybrid'}}`, `{{no member named 'kx_keypair'}}`), `alone`, with `crypto.json` entries.
- [ ] **Step 5: Run** `grep -rn 'hybrid::\|hybrid/\|x25519/kx.hpp\|x25519::kx_keypair\|x25519::session_keys' libs/crypto tools/test/fixtures/crypto_jamfiles --include=*.hpp --include=*.cpp --include=*.adoc --include=*.json --include=Jamfile` - Expected: only the two message tests and `changelog.adoc` (4.1). The touched targets, the two message tests, `example//box_messages example//kx_session example//file_share`, `generate.py --check`, the fixtures named above - Expected: pass.
- [ ] **Step 6: Commit** `crypto: crypto_box and sealed boxes are public_key_encryption's, crypto_kx is key_exchange's, and crypto::hybrid is free`; superproject `crypto fixtures: the boxes and crypto_kx in their namespaces by purpose`.

### Task X2: A public `crypto::wipe` (controller ruling)

**Files:** Create `include/webcpp/crypto/wipe.hpp`, `test/wipe_public_test.cpp` (`crypto-run wipe_public : wipe_public_test.cpp : $(OPTIMISED) <inlining>full ;`), `test/messages/wipe_const_range/`, `doc/wipe.adoc` (`[#crypto-wipe] === Wiping a buffer`, included after `equal.adoc` in `crypto.adoc`). Modify `crypto.hpp`, `test/Jamfile` (headers-alone list), `meta/include-boundaries.json` (`wipe.hpp` in the backend-free boundary), `crypto.json` (root row, `fixes`).

**Interfaces (Produces):**

```cpp
namespace webcpp::crypto {
template <class Range>
    requires std::ranges::contiguous_range<Range> && std::ranges::sized_range<Range> &&
             (!std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<Range>>>) &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>
void wipe(Range&& wiped_range) noexcept {
    detail::wipe(std::ranges::data(wiped_range),
                 std::ranges::size(wiped_range) * sizeof(std::ranges::range_value_t<Range>));
}
// A contiguous sized range of const or non-trivially-copyable elements: static_assert with
// "webcpp::crypto: wipe zeroes a buffer the program may write, of trivially copyable elements: a std::vector<std::uint8_t>, a std::string, a std::array, a std::span<std::byte>; this range's elements are const or not trivially copyable."
}
```

No `(void*, std::size_t)` overload: every caller found (trystero's `detail::wipe(x.data(), x.size())`) has a range. No guard (no backend call).
- [ ] **Step 1: Failing test** `wipe_public_test.cpp`: after `crypto::wipe(x)`, every byte is zero for `std::vector<std::uint8_t>`, `std::string` (its size, not its capacity), `std::array<std::uint8_t, 33>`, `std::span<std::byte>` over a stack buffer (bytes around the span untouched), `unsigned char[16]`, `std::vector<std::uint32_t>`; an empty vector is a no-op; `static_assert(noexcept(crypto::wipe(vector)))`; `static_assert(!requires { crypto::wipe(std::declval<std::list<char>&>()); })` and for `std::vector<bool>`; a buffer wiped at the end of a function whose storage the test inspects afterwards as `secret_test.cpp` does (`$(OPTIMISED) <inlining>full`). Run `b2 ... toolset=clang libs/crypto/test//wipe_public` - Expected: `wipe.hpp` not found.
- [ ] **Step 2: Implement** `wipe.hpp` (file comment: the dead-store hazard, `@see` the page); `grep -rn '[^:_]wipe(' libs/crypto/include` - Expected: only `detail::`-qualified calls or calls inside `namespace detail`, so the template never captures one.
- [ ] **Step 3: Message test `wipe_const_range`** (`crypto::wipe(std::string const&)`, the message above, `alone`); the page section (why `memset` is dropped, the call, what it is not: no `mlock`); the root row `["\`wipe\`", "Erasing a password, a key or a plaintext from a buffer the program owns", "\`<webcpp/crypto/wipe.hpp>\`", "none", "{cwe14}", ...]` (add `cwe14` to the attributes if absent); regenerate.
- [ ] **Step 4: Run** `wipe_public`, `wipe`, `convenience_header`, the message test, `generate.py --check`, the doc build `b2 ... libs/crypto/doc` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: crypto::wipe zeroes any buffer the program owns, with stores the optimiser keeps`.

### Task 1.1: The signature types (D1)

**Files:** Create `include/webcpp/crypto/detail/signature.hpp`, `test/signature_types_test.cpp` (`crypto-run signature_types : signature_types_test.cpp ;`). Modify `signing.hpp` (the trait), `curves/{ed25519,p256,secp256k1}.hpp` (tags, aliases, trait specializations), `test/Jamfile` (headers-alone).

**Interfaces (Produces):**

```cpp
namespace webcpp::crypto::detail {
template <class Tag, std::size_t Size>
class basic_signature {
public:
    static constexpr std::size_t size = Size;
    basic_signature() noexcept = default;  // all zeros: the types are std::regular (ruling P6)
    [[nodiscard]] static boost::system::result<basic_signature> from_bytes(
        std::span<std::uint8_t const> signature_bytes) noexcept;  // invalid_signature unless Size bytes
    [[nodiscard]] std::array<std::uint8_t, Size> const& bytes() const noexcept;
    [[nodiscard]] std::uint8_t recovery_id() const noexcept requires Tag::recoverable;  // bytes()[64]
    [[nodiscard]] bool operator==(basic_signature const&) const noexcept = default;
private:
    friend struct signature_access;  // signature_access::of<T>(std::array<std::uint8_t, T::size> const&)
    std::array<std::uint8_t, Size> signature_bytes_{};
};
template <class Key, class Scheme> struct signature_of { using type = void; };
template <class Key, class Scheme> using signature_of_t = typename signature_of<Key, Scheme>::type;
}
```

- Tags (empty structs in `detail`, `recoverable` true only on the recoverable one) and aliases: `curves::ed25519::signature` (64), `curves::ed25519::ph_signature` (64, ruling P5), `curves::p256::ecdsa_signature` (64), `curves::secp256k1::ecdsa_signature` (64), `recoverable_signature` (65), `schnorr_signature` (64).
- `signature_of` specialized in each curve header for its key pairs and public keys under each scheme of `signs_with_v`/`verifies_with_v` (Ed25519 `eddsa`; P-256 `ecdsa_sha256`; secp256k1 the three, and the 32-byte x-only key under `schnorr`).
- [ ] **Step 1: Failing test** `signature_types_test.cpp`: `signature_from_wire` - for each of the six types, `from_bytes` of `size` bytes round-trips through `bytes()`, of `size - 1` and `size + 1` bytes returns `invalid_signature`; `recoverable_signature::from_bytes(65 bytes ending 0x02)->recovery_id() == 2`; `static_assert(std::regular<T>)` for each; `!std::is_convertible_v` and `!std::is_constructible_v` between every pair of the six and between each and `std::span<std::uint8_t const>` / `std::array<std::uint8_t, 64>`, both ways; `!requires { ecdsa_signature{}.recovery_id(); }`; `std::is_same_v<detail::signature_of_t<p256::keypair, ecdsa_sha256_t>, p256::ecdsa_signature>` and the other pairs; `signature_of_t<p256::keypair, schnorr_t>` is `void`. Run - Expected: header not found.
- [ ] **Step 2: Implement**; `timing_coverage_{sodium,bearssl,secp256k1}_test.cpp` check `from_bytes` and `bytes()` enter no guard.
- [ ] **Step 3: Run** `signature_types`, `signing`, `keypair`, the three timing coverage tests, `generate.py --check` - Expected: pass.
- [ ] **Step 4: Commit** `crypto: a signature type per curve and scheme, made from bytes of its size alone`.

### Task 1.2: Every sign returns its type, every verify and recover takes it (D1, R1, owner decision 6)

**Files:** `signing.hpp`, `keypair.hpp`, `async.hpp`, `curves/{ed25519,p256,secp256k1}.hpp`, `curves/secp256k1/musig.hpp`, every caller: tests (`p256_*`, `ed25519_*`, `ed25519ph_*`, `secp256k1_*`, `crypto_secp256k1_test`, `signing_*`, `keypair_test`, `synchronous_forms_test`, `async_*`, `timing_coverage_*`, `oracle_25519_test`, `formats_25519_*`, `allocation_*`, `test/support/secp256k1_checks.hpp`), `test/constant_time/{paths.hpp,dudect_*}`, `test/sizes/*`, examples (`p256_sign`, `ecdsa_der`, `p256_handshake`, `signed_update`, `ed25519_sign`, `signed_message`, `arena_signer`, `schnorr_sign`, `recover_signer`, `bitcoin_sighash`, `musig_*`, `wallet_key`, `taproot_tweak`, `four_forms`, `async_sign`; `.expected` only if output changes), `bench/operations.hpp`, the doc pages' code, the message and MSVC tests that call `verify`; new message tests below. Superproject: fixtures `sign`, `sign_small`, `encrypt_and_sign`, `secp256k1_*`, `ed25519_default` where they verify or print a signature.

**Interfaces (Produces):**
- `sign` returns `result<detail::signature_of_t<basic_keypair, Scheme>>` on all three curves (secp256k1's hedged and explicit-aux overloads too); Ed25519ph's `sign(key_pair)` returns `ph_signature`; `async_sign` completes with `void(error_code, T)`, the default `T` on an error.
- Every `verify` (free, member, asynchronous, free and member) takes `Signature const& signature_value` as a template parameter and first calls `detail::check_signature<Expected, Signature>()`, a `static_assert(std::is_same_v<Expected, Signature>, <text>)` with these texts:
  - "webcpp::crypto: verify: a P-256 key verifies a curves::p256::ecdsa_signature: make one from 64 bytes with curves::p256::ecdsa_signature::from_bytes."
  - "webcpp::crypto: verify: a secp256k1 key verifies a curves::secp256k1::ecdsa_signature under crypto::ecdsa_sha256: make one from 64 bytes with curves::secp256k1::ecdsa_signature::from_bytes."
  - "webcpp::crypto: verify: crypto::schnorr verifies a curves::secp256k1::schnorr_signature: make one from 64 bytes with curves::secp256k1::schnorr_signature::from_bytes."
  - "webcpp::crypto: verify: an Ed25519 key verifies a curves::ed25519::signature under crypto::eddsa: make one from 64 bytes with curves::ed25519::signature::from_bytes." (Ed25519ph's: `curves::ed25519::ph_signature`)
  - `recover`: "webcpp::crypto: recover takes a curves::secp256k1::recoverable_signature: make one from 65 bytes with curves::secp256k1::recoverable_signature::from_bytes."
- The size checks inside the workers go (the type has its size); `p256_verify`, `ed25519_verify` and secp256k1's workers take `std::span<std::uint8_t const, N>` of `signature_value.bytes()`.
- `can_verify<Scheme, Signature, Input, PublicKey>` (ruling P7): adds `std::is_same_v<std::remove_cvref_t<Signature>, detail::signature_of_t<std::remove_cvref_t<PublicKey>, Scheme>>`.
- Format helpers typed on both curves (ruling P8): `to_der(ecdsa_signature const&, std::span<std::uint8_t> der_output) -> result<std::size_t>`, `from_der(std::span<std::uint8_t const>) -> result<ecdsa_signature>`, `normalize_low_s(ecdsa_signature const&) -> result<ecdsa_signature>`, `is_low_s(ecdsa_signature const&) -> bool`.
- MuSig2's `partial_signature_aggregate` returns `result<schnorr_signature>` (ruling P9); partial signatures (32 bytes) unchanged.
- [ ] **Step 1: Failing tests**, every existing case kept: `p256_ecdsa_bearssl_test.cpp` - `sign` returns `p256::ecdsa_signature`; RFC 6979 A.2.5's high-S signature, made with `from_bytes`, verifies and `normalize_low_s` of it equals `sign`'s; `signature_from_vector`: a `std::vector<std::uint8_t>` of 64 bytes made into the type verifies through `key.verify`, `crypto::verify`, `key.async_verify`, `crypto::async_verify` (in `async_p256_test.cpp`), 63 and 65 bytes give `invalid_signature` from `from_bytes`; the same on Ed25519 (`ed25519_sodium_test.cpp`, `async_25519_test.cpp`) and synchronously on secp256k1 (`secp256k1_test.cpp`: the three types, `recover` with a `recoverable_signature`); `static_assert(!crypto::can_verify<ecdsa_sha256_t, secp256k1::ecdsa_signature, std::span<std::uint8_t const>, p256::public_key>)`; `to_der`/`from_der` round trip typed on both curves; MuSig2's aggregate verifies with `crypto::verify(crypto::schnorr, aggregate, message, key)`. Run `b2 ... toolset=clang libs/crypto/test//p256_ecdsa_bearssl` - Expected: compile errors.
- [ ] **Step 2: Implement** the interfaces; the old "verify takes bytes of any size" tests of f21d798 become the `from_bytes` checks above.
- [ ] **Step 3: Migrate every caller** (`.bytes()` where bytes were used; `T::from_bytes(bytes)` where a signature arrives as bytes); constant-time paths sign and verify through the types (what they measure is unchanged).
- [ ] **Step 4: Message tests**, each `alone` with its `crypto.json` entry: `verify_p256_secp256k1_signature` (the spec's wrong-curve scenario: the P-256 text), `verify_schnorr_as_ecdsa` (a `schnorr_signature` under `ecdsa_sha256` with a secp256k1 key), `verify_ed25519_bytes_signature` (a `std::array<std::uint8_t, 64>`), `recover_ecdsa_signature`, `async_verify_p256_bytes_signature`; regenerate.
- [ ] **Step 5: Run** `grep -rnE 'std::span<std::uint8_t const> signature' libs/crypto/include` - Expected: nothing. The clang whole build through `heavy.sh crypto-types`; `EM_CACHE=... b2 ... toolset=emscripten libs/crypto/test//p256_webcrypto_oracle libs/crypto/test//formats_25519_webcrypto_oracle libs/crypto/test//async_p256 libs/crypto/test//async_25519` through `heavy.sh`; the fixtures named above; `generate.py --check` - Expected: all pass.
- [ ] **Step 6: Commit** `crypto: every sign returns its curve's signature type and every verify and recover takes it`; superproject `crypto fixtures: signatures are typed values`.

### Task 1.3: Ethereum's recoverable ECDSA (owner decision 10, design D8, R18-R20)

**Files:** `signing.hpp` (tag, value, `all_schemes`, `scheme_traits`, the digest-size text, the catch-all `verify`'s text), `curves/secp256k1.hpp` (`signs_with_v`, `signature_of`, `sign`'s dispatch, `recover`'s overload, the Keccak digest), new `test/secp256k1_ethereum_test.cpp` (`crypto-run secp256k1_ethereum : secp256k1_ethereum_test.cpp : <library>/third_party//secp256k1 <library>/third_party//xkcp ;`), `test/signing_test.cpp`, `test/keypair_test.cpp`, `timing_coverage_secp256k1_test.cpp`, `test/constant_time/paths.hpp` (`secp256k1_signing` signs under the new scheme too), new `example/ethereum_personal_sign.cpp` (+ `.expected`, `example/Jamfile` with `<library>/third_party//xkcp`), `doc/secp256k1.adoc` (a section "Ethereum"), `doc/wallets.adoc` (a link), `crypto.json` (the signing row's text, `fixes`), message tests below; every hand list of the schemes `grep -rn 'ecdsa_sha256_recoverable' libs/crypto` shows (the generator included).

**Interfaces (Produces):**
- `struct ecdsa_keccak256_recoverable_t { explicit ecdsa_keccak256_recoverable_t() = default; }; inline constexpr ecdsa_keccak256_recoverable_t ecdsa_keccak256_recoverable{};` in `all_schemes` (after `ecdsa_sha256_recoverable_t`); `scheme_traits`: `signature_size` 65, `digest_size` 32; `check_input`'s marker-size text for it: "webcpp::crypto: crypto::ecdsa_keccak256_recoverable takes a 32-byte digest, Keccak-256's: crypto::prehashed{digest} of a std::array<std::uint8_t, 32>."
- `signs_with_v<secp256k1::basic_keypair<B>, ecdsa_keccak256_recoverable_t>` true; `signature_of` of it `secp256k1::recoverable_signature` (R20); `sign`'s scheme refusal lists four schemes.
- `secp256k1.hpp`: `#if defined(WEBCPP_HASH_HAS_XKCP)` `#include <webcpp/hash/keccak.hpp>`; `template <class Input> std::array<std::uint8_t, 32> detail::secp256k1_keccak_digest(Input const&)` (the marker's bytes, or `webcpp::hash::keccak_256` over `message_bytes`); a message input without XKCP stops at R19's text through a dependent `static_assert`; the signing worker is the existing `signed_recoverable(digest)`.
- `template <class Backend = default_backend_t<families::secp256k1>, class Signature, class Input> [[nodiscard]] result<public_key> recover(ecdsa_keccak256_recoverable_t, Signature const& signature_value, Input const& signing_input) noexcept`, guard first, `check_signature<recoverable_signature, Signature>()`, the checks of `ecdsa_sha256_recoverable`'s `recover` (high S accepted, as libsecp256k1's recovery and Ethereum's `ecrecover` accept it). No `crypto::verify` overload (R18): the catch-all's text gains "a crypto::ecdsa_keccak256_recoverable signature gives its key to curves::secp256k1::recover". No asynchronous form (crypto-sign's R8).
- [ ] **Step 1: Failing tests** `secp256k1_ethereum_test.cpp`, each vector's source URL and commit in a comment: `geth_recover`: go-ethereum `crypto/signature_test.go`'s `testmsg`, `testsig`, `testpubkey` (record the commit): `recover(ecdsa_keccak256_recoverable, recoverable_signature::from_bytes(testsig), prehashed{testmsg})` gives `testpubkey` (uncompressed); `personal_sign`: web3.js 1.x documentation's `web3.eth.accounts.sign('Some data', '0x4c08...2318')` vector (record the URL): the EIP-191 digest built in the test with `webcpp::hash::keccak_256`, `sign(ecdsa_keccak256_recoverable, prehashed{digest})` gives the documented r and s and `recovery_id() == v - 27`; `keccak_not_sha3`: `keccak_256("")` is `c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470` and SHA3-256's `a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a` differs; `message_is_hashed`: `sign(scheme, m) == sign(scheme, prehashed{keccak_256(m)})` and `!=` over `sha3_256(m)`, for a 32-byte `m` too; `recover` with the message gives the signer; `always_low_s`: 256 signatures, `is_low_s` of their first 64 bytes; `empty_message`; `wrong_scheme_key`: recovering a keccak signature under `ecdsa_sha256_recoverable` gives another key. Run `b2 ... toolset=clang libs/crypto/test//secp256k1_ethereum` - Expected: compile errors.
- [ ] **Step 2: Implement**; `timing_coverage_secp256k1_test.cpp`: `sign` and `recover` under the scheme, message and marker, each enter their own guard.
- [ ] **Step 3: Message tests**, each `alone` with `crypto.json` `fixes`: `sign_keccak_digest_20` (the digest-size text), `sign_p256_keccak` and `sign_ed25519_keccak` (their curves' scheme texts), `verify_keccak_recoverable` (the catch-all's text), `sign_keccak_without_xkcp` (R19's text, a Jamfile without `/third_party//xkcp`, native wasip2 wasip3); MSVC twin of `sign_keccak_without_xkcp` (a build message).
- [ ] **Step 4: Example** `ethereum_personal_sign.cpp`: builds EIP-191's digest (`"\x19Ethereum Signed Message:\n"`, the length in decimal, the message) with `webcpp::hash::keccak_256`, signs it as `prehashed`, prints r, s and `v = 27 + recovery_id()`, recovers the key and prints the address (the last 20 bytes of Keccak-256 over the uncompressed key without its 0x04); comments say that crypto computes no digest, no `v` (EIP-155's `35 + 2 x chain_id + id`, typed transactions' `yParity`) and no address. The page section shows it by tag, with D8's table of `v`.
- [ ] **Step 5: Run** `secp256k1_ethereum`, `secp256k1_test`, `secp256k1_recovery`, `signing`, `keypair`, the coverage test, the message tests, `example//ethereum_personal_sign`, `generate.py --check`; wasip2 and Emscripten `secp256k1_ethereum` through `heavy.sh` - Expected: pass.
- [ ] **Step 6: Commit** `crypto: secp256k1 signs and recovers Ethereum's recoverable ECDSA over Keccak-256`.
- [ ] **Step 7: Review**: a fresh reviewer reads D8, R18-R20 and this task's diff (the vectors' provenance, the Keccak padding, the refusal without XKCP) before 2.1 starts.

### Task 2.1: X25519 typed (D2, R3)

**Files:** `curves/x25519.hpp` (rewritten), `backends/sodium.hpp` (engine unchanged but for X3/2.5 names), `async.hpp`, `keypair.hpp` (the `agree` catch-all's text), callers: `x25519_sodium_test`, `x25519_mock_test`, `x25519_wycheproof_sodium_test`, `oracle_25519_test`, `formats_25519_*`, `async_25519_test`, `timing_coverage_sodium_test`, `test/constant_time/{paths.hpp (sodium_x25519), dudect_sodium_test.cpp}`, `bench/operations.hpp`, examples `x25519_agree`, `ecdh_session` and any `grep -rn 'x25519::agree\|x25519::public_key(' libs/crypto` hit (the boxes and kx are fed `secret()->bytes()` and `public_key()->bytes()` until 2.2), `doc/x25519.adoc`. Superproject: fixtures `x25519_default`, `x25519_default_not_linked_on_emscripten`.

**Interfaces (Produces):** in `curves::x25519`:
- `class public_key`: `static constexpr std::size_t public_key_size = 32;` `[[nodiscard]] static result<public_key> from_bytes(std::span<std::uint8_t const>) noexcept` (`invalid_public_key` unless 32 bytes; nothing else checked; no guard); `[[nodiscard]] std::array<std::uint8_t, 32> const& bytes() const noexcept`; `operator==`; private default constructor, `friend struct detail::x25519_access;`.
- `template <class Backend = default_backend_t<families::x25519>> class basic_keypair`: `family_type`, `name_type`, `backend_type`, `using public_key_type = x25519::public_key;`, `static constexpr std::size_t secret_key_size = 32;`; `static result<basic_keypair> from_secret(std::span<std::uint8_t const> secret_bytes) noexcept` (guard; `invalid_key_size`; the public key computed once by `engine::base`); `static result<basic_keypair> generate() noexcept` (guard; `random_device`; `entropy_unavailable`); `result<public_key_type> public_key() const noexcept` and `result<crypto::secret<32>> secret() const noexcept` (copies, no guard; `invalid_state` when moved from); `result<crypto::secret<32>> agree(public_key_type const& peer_key) const noexcept` (guard; `weak_public_key` for an all-zero result, declassified as today); `async_from_secret`, `async_generate` (static), `async_agree(completion_executor, peer_key, completion_token) const` declared here, defined in `async.hpp`. State: `crypto::secret<32> secret_value_`, `std::array<std::uint8_t, 32> public_bytes_`, `bool keyed_` (moves exchange `keyed_`; `secret<32>`'s move wipes the source). `using keypair = basic_keypair<>;`
- `detail::x25519_access` (friend): `secret_bytes(kp) -> std::span<std::uint8_t const, 32>` and `public_bytes(kp)`, for kx, the boxes and the sealed box without a nested guard (ruling P13); `curves::x25519::algorithm<B>` becomes `detail::x25519_operations<B>` (unguarded `base` and `agree` workers).
- `async.hpp`: `is_x25519_keypair_v` joins `is_webcrypto_keypair_v`; `async_agree(executor, x25519::basic_keypair<B> const&, x25519::public_key const&, token)` calls the member; the byte form goes; `async_engine`'s shapes add `agree(executor, key_pair, peer_public_key, token)` for X25519.
- Removed: free `public_key(secret)`, `agree(secret, peer)`, `algorithm<B>`.
- [ ] **Step 1: Failing tests** in `x25519_sodium_test.cpp`: `rfc7748`: section 6.1's Alice and Bob (the vectors already in the file) made with `from_secret` give the RFC's public keys and agree in both directions on the shared secret, and through `async_agree` (in `async_25519_test.cpp`, member and free); `secret_round_trip`: `from_secret(k.secret()->bytes())` has `k`'s public key; `from_bytes` of 31 and 33 bytes is `invalid_public_key`; `weak_peer`: `agree` with `from_bytes` of 32 zero bytes is `weak_public_key`; `moved_from` (Review Focus 2): a moved-from key pair's three members return `invalid_state`; `crypto::agree(k, peer)` and `crypto::from_secret<x25519::keypair>` equal the members; `generate()` twice differ. `x25519_wycheproof_sodium_test.cpp` drives the typed forms. Run - Expected: compile errors.
- [ ] **Step 2: Implement**; `timing_coverage_sodium_test.cpp`: `from_secret`, `generate`, `agree` and the asynchronous forms enter their own guard, `public_key()`, `secret()`, `from_bytes` none.
- [ ] **Step 3: Migrate** the callers; `keypair.hpp`'s agree refusal text names `to_x25519()` once 2.3 lands (until then "an Ed25519 key pair agrees through X25519").
- [ ] **Step 4: Run** `grep -rn 'x25519::agree\|x25519::public_key(\|x25519::algorithm' libs/crypto tools/test/fixtures/crypto_jamfiles` - Expected: nothing. Touched targets, `example//x25519_agree`, Emscripten `oracle_25519 formats_25519_webcrypto_oracle async_25519`, the two fixtures, `generate.py --check` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: X25519 has a typed public key and a key pair, made, drawn and agreed like the other curves'`; superproject `crypto fixtures: X25519 agrees with typed keys`.

### Task 2.2: crypto_kx, the boxes and the sealed box on typed keys (D3, R5, R6, owner decision 7)

**Files:** `key_exchange/kx.hpp`, `public_key_encryption/{box,sealed_box}.hpp`, `backends/sodium.hpp` (kx engine over bytes; `kx_state` removed; the sealed box engine unchanged), callers: `kx_sodium_test`, `kx_wipe_sodium_test`, `box_sodium_test`, `aead_box_mock_test`, `timing_coverage_sodium_test`, `paths.hpp` (`sodium_box`, `sodium_sealed_box`, `sodium_kx`), examples `kx_session`, `box_messages`, `file_share`, `doc/kx.adoc`, `doc/public_key_encryption.adoc`. Superproject: fixtures `kx_default`, `box_*_default`, `sealed_box_default`.

**Interfaces (Produces):**
- `key_exchange::kx_keypair<Backend>`: `static result<kx_keypair> from_keypair(x25519::basic_keypair<Backend> key_pair) noexcept` (a sink, by value; ruling P14); `static result<kx_keypair> derive_from_seed(std::span<std::uint8_t const> seed_bytes) noexcept` (`invalid_key_size` unless 32; `engine::seed_keypair` = `crypto_kx_seed_keypair`, its public half wiped, the secret given to `x25519::basic_keypair<Backend>::from_secret`); `static result<kx_keypair> generate() noexcept`; `result<x25519::public_key> public_key() const noexcept`; `x25519::basic_keypair<Backend> const& keypair() const noexcept`; `result<session_keys> client_session_keys(x25519::public_key const& server_public_key) const noexcept` and `server_session_keys(x25519::public_key const& client_public_key)`; `weak_public_key` where libsodium refuses. Engine: `seed_keypair(seed, secret_out)`, `client(secret, public, peer, receive, transmit)`, `server(...)`, each over `std::span<std::uint8_t const, 32>`; a `static_assert(supports_v<families::x25519, Backend>)` with the text "webcpp::crypto: key_exchange::kx_keypair holds an X25519 key pair of its own backend, which has no X25519."
- `public_key_encryption::basic_box<F, B>::from_keys(x25519::basic_keypair<B> const& own_keypair, x25519::public_key const& peer_public_key)`; the byte form goes.
- `basic_sealed_box<B>::seal(x25519::public_key const& recipient_public_key, std::span<std::uint8_t const> message_bytes, std::span<std::uint8_t> output_bytes)`, `open(x25519::basic_keypair<B> const& recipient_keypair, std::span<std::uint8_t const> sealed_bytes, std::span<std::uint8_t> output_bytes)`; the byte forms go.
- [ ] **Step 1: Failing tests**: `kx_sodium_test.cpp` - `client_server`: the client's transmit key is the server's receive key and the reverse; `seed_is_not_secret` (spec): `derive_from_seed(seed)` gives the public key and session keys of libsodium's `crypto_kx_seed_keypair(seed)` called in the test, and its public key differs from `from_keypair(*x25519::keypair::from_secret(seed))`'s; `from_keypair` keeps the key pair (`keypair().public_key()`); a session with a zero peer key is `weak_public_key`. `box_sodium_test.cpp` - two parties' `from_keys` seal and open each other's messages; `open_failures` (Review Focus 3): one flipped byte gives `authentication_failed` with `data` zeroed, a 12-byte nonce `invalid_iv_size` with `data` untouched; a small-order peer gives `weak_public_key`; the sealed box round trip and its tampered case (`authentication_failed`, `output` zeroed). `kx_wipe_sodium_test.cpp` probes the inner key pair's secret storage after a move and after destruction. Run - Expected: compile errors.
- [ ] **Step 2: Implement**; coverage: every new function enters its own guard; `keypair()` and `public_key()` none.
- [ ] **Step 3: Migrate** the callers and pages; the kx page says `derive_from_seed` hashes the seed (BLAKE2b-256) and `from_keypair` takes the secret as is.
- [ ] **Step 4: Run** the touched tests, `example//kx_session example//box_messages example//file_share`, the fixtures, `generate.py --check` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: crypto_kx, crypto_box and the sealed box take X25519's typed keys`; superproject `crypto fixtures: crypto_kx and the boxes on typed X25519 keys`.

### Task 2.3: Ed25519 to X25519, typed (D3, R4)

**Files:** `curves/ed25519.hpp` (includes `curves/x25519.hpp`), `keypair.hpp` (agree refusal text), callers (`grep -rn 'x25519_secret\|to_x25519_public' libs/crypto`: 6 tests, 1 example, 7 doc pages, `README.md`), `paths.hpp` (`sodium_ed25519` converts).

**Interfaces (Produces):**
- `[[nodiscard]] result<x25519::basic_keypair<Backend>> ed25519::basic_keypair<Backend>::to_x25519() const noexcept` (guard; `static_assert(supports_v<families::x25519, Backend>, "webcpp::crypto: to_x25519: this backend has no X25519; libsodium has both.")`; `engine::x25519_secret`, then `x25519::basic_keypair<Backend>::from_secret`, the intermediate secret a `secret<32>`; ruling P20).
- `template <class Backend = default_backend_t<families::ed25519>> [[nodiscard]] result<x25519::public_key> ed25519::public_key::to_x25519() const noexcept` (guard; `invalid_public_key` when libsodium's `pk_to_curve25519` refuses: not canonical or of small order).
- Free `template <class Value> requires requires(Value const& v) { v.to_x25519(); } [[nodiscard]] auto to_x25519(Value const& converted_value) noexcept` in `ed25519.hpp`, guard, calling the member. Synchronous only (R4).
- Removed: `x25519_secret()`, `to_x25519_public`. Engine functions keep their names.
- [ ] **Step 1: Failing tests** in `ed25519_sodium_test.cpp`: `conversion` (spec): RFC 8032 test 1's key pair `to_x25519()` has `public_key()` equal to `public_key()->to_x25519()`, and both equal libsodium's `crypto_sign_ed25519_sk_to_curve25519`/`pk_to_curve25519` called in the test; ed25519-speccheck's small-order key 0 and a non-canonical key give `invalid_public_key`; `crypto::to_x25519` equals both members; the converted key pair agrees with an X25519 peer. Run - Expected: compile errors.
- [ ] **Step 2: Implement**; coverage: both members and the free form enter their own guard.
- [ ] **Step 3: Migrate** the callers; `agree`'s refusal text: "... an Ed25519 key pair agrees through X25519: convert it with to_x25519()."
- [ ] **Step 4: Run** `grep -rn 'x25519_secret\|to_x25519_public' libs/crypto --include=*.hpp --include=*.cpp --include=*.adoc --include=*.json --include=*.md` - Expected: only `changelog.adoc` (4.1). Touched tests, Emscripten `oracle_25519`, `generate.py --check` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: Ed25519's key pair and public key convert to X25519's typed keys`.

### Task 3.1: Three key pair concepts (D4)

**Files:** `keypair.hpp`, `signing.hpp` (`crypto::sign`'s refusal), `async.hpp`, `curves/{ed25519,x25519,p256,secp256k1}.hpp` (`static_assert`s where the default backend is linked, as today), `test/keypair_test.cpp`, `test/signing_curves_test.cpp`, new message tests `sign_x25519`, `agree_ed25519`, `async_sign_x25519`.

**Interfaces (Produces):**

```cpp
template <class Keypair>
concept keypair = requires {
    typename Keypair::public_key_type;
    { Keypair::from_secret(std::declval<std::span<std::uint8_t const>&>()) } -> std::same_as<boost::system::result<Keypair>>;
    { Keypair::generate() } -> std::same_as<boost::system::result<Keypair>>;
    { std::declval<Keypair const&>().public_key() } -> std::same_as<boost::system::result<typename Keypair::public_key_type>>;
    { std::declval<Keypair const&>().secret() } -> std::same_as<boost::system::result<secret<Keypair::secret_key_size>>>;
};
namespace detail {
// signs_as_declared<K, S> also requires the result to be result<signature_of_t<K, S>>
template <class Keypair, class List>
concept signing_keypair_over = keypair<Keypair> && signs_some_scheme_v<Keypair, List> && signs_every_declared_v<Keypair, List>;
template <class Type> inline constexpr bool is_secret_result_v = false;  // true for result<secret<N>>
}
template <class Keypair> concept signing_keypair = detail::signing_keypair_over<Keypair, detail::all_schemes>;
template <class Keypair> concept agreement_keypair = keypair<Keypair> && requires {
    requires detail::is_secret_result_v<decltype(std::declval<Keypair const&>().agree(
        std::declval<typename Keypair::public_key_type const&>()))>;
};
```

- `crypto::sign`/`async_sign` require `signing_keypair`; `crypto::agree`/`async_agree` require `agreement_keypair` (`async_agree` becomes one template for P-256 and X25519). Refusals, each the first `static_assert` reached: `crypto::sign` of an `agreement_keypair` that is not a `signing_keypair`: "webcpp::crypto: sign: X25519 key pairs agree; they do not sign: sign with an Ed25519 key pair, which to_x25519() converts for agreement."; `crypto::agree` of a `signing_keypair` that is not an `agreement_keypair`: "webcpp::crypto: agree: Ed25519 key pairs sign; they do not agree: convert one with to_x25519() and agree with the X25519 key pair."; the existing texts stay for the other mismatches.
- `static_assert`s per D4's table in each curve header.
- [ ] **Step 1: Failing tests** `keypair_test.cpp`: stand-ins (signing only, agreement only, both, missing `from_secret`, `sign` returning bytes) give the three concepts' verdicts; the four curves' rows of D4's table; `from_secret_round_trip` (spec) for the four curves. Run - Expected: failures.
- [ ] **Step 2: Implement**; the message tests `sign_x25519`, `agree_ed25519`, `async_sign_x25519` (`alone`, `crypto.json` entries); regenerate.
- [ ] **Step 3: Run** `keypair`, `signing`, `signing_curves`, `synchronous_forms`, the async tests, the message tests, a clang whole build through `heavy.sh crypto-types`, `generate.py --check` - Expected: pass.
- [ ] **Step 4: Commit** `crypto: key pairs are checked by what they do: keypair, signing_keypair and agreement_keypair`.

### Task 3.2: The AEAD and box concepts, secretbox without additional data (D5, R7)

**Files:** Create `include/webcpp/crypto/aead.hpp` (concepts and the free `seal`/`open`, no backend; in the include boundary), `test/aead_concepts_test.cpp`, message test `seal_box_additional_data`. Modify `ciphers/aead.hpp` (secretbox families and `aead_family` specializations out; the free forms out), `ciphers/secretbox.hpp` (`basic_secretbox`, the families, `detail::secretbox_state`/`secretbox_engine` primaries with the generated `REFUSE_SECRETBOX_*` macros), `ciphers/gcm.hpp`, `public_key_encryption/box.hpp`, `backends/sodium.hpp` (`secretbox_state`/`secretbox_engine` specializations replace the `aead_engine` ones, no additional-data parameter; ruling P10), `test/messages/aead_not_a_family` and its MSVC twin (its text lists four families fewer), callers (17 tests, 2 examples, `secretbox.adoc`, `aead.adoc`, `everyday.adoc`, `paths.hpp`'s `seal_and_open`). Superproject: fixtures `secretbox_*_default`.

**Interfaces (Produces):**
- `crypto::aead<C>`: `C::key_size`, `C::iv_size`, `C::tag_size` convertible to `std::size_t`; `typename C::result_type`; `{ C::from_key(span&) } -> same_as<result<C>>`; `{ c.seal(span const&, span const&, std::span<std::uint8_t>&) } -> same_as<result<typename C::result_type>>`; `{ c.open(span, span, std::span<std::uint8_t>, span) } -> same_as<result<void>>`.
- `crypto::streaming_aead<C>`: `aead<C>` and `begin_seal(iv, ad)`, `update(data)`, `result()`, `begin_open(iv, ad)`, `authenticate(ad_piece)`, `decrypt(data)`, `verify(tag)`, `end_open()` with `gcm`'s types.
- `crypto::authenticated_box<C>`: the sizes, `result_type`, `{ c.seal(span, std::span<std::uint8_t>&) } -> same_as<result<result_type>>`, `{ c.open(span, std::span<std::uint8_t>, span) } -> same_as<result<void>>`.
- `template <class Family, class Backend> class ciphers::basic_secretbox` with `from_key`, `seal(iv, data)`, `open(iv, data, tag)`, the guards, zeroing and move rules of `ciphers::aead`; `secretbox<B>` and `secretbox_xchacha20poly1305<B>` its aliases.
- Free forms in `aead.hpp`: `seal(C&, iv, ad, data)`/`open(C&, iv, ad, data, tag)` requiring `aead<C>`; `seal(C&, iv, data)`/`open(C&, iv, data, tag)` requiring `authenticated_box<C>`; a catch-all `seal(C&, iv, ad, data)` for an `authenticated_box` with "webcpp::crypto: seal: a secret box or a crypto_box takes no additional data: crypto::seal(box, nonce, data)." (and `open`'s twin).
- `static_assert`s per D5's table in `ciphers/gcm.hpp`, `ciphers/chacha20_poly1305.hpp`, `ciphers/secretbox.hpp` and `public_key_encryption/box.hpp`.
- [ ] **Step 1: Failing tests**: `secretbox_sodium_test.cpp` - `seal(nonce, data)` gives `crypto_secretbox_detached`'s ciphertext and tag (called in the test), and `crypto::seal(box, nonce, data)` the same; `!requires { box.seal(nonce, ad, data); }`; `open_failures` (Review Focus 3); `aead_concepts_test.cpp` - D5's table and stand-ins (a type without `open` is no `aead`; `gcm` is a `streaming_aead`, `chacha20_poly1305` an `aead` and not a `streaming_aead`; no type is both `aead` and `authenticated_box`). Run - Expected: failures.
- [ ] **Step 2: Implement**; coverage of `basic_secretbox`'s three members on sodium.
- [ ] **Step 3: Migrate**; message test `seal_box_additional_data`; regenerate.
- [ ] **Step 4: Run** `grep -rn 'aead<families::secretbox' libs/crypto` - Expected: nothing. The touched tests, `example//file_share example//xchacha_seal`, the fixtures, a clang whole build through `heavy.sh crypto-types`, `generate.py --check` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: AEADs and boxes are checked by concepts, and a secret box takes no additional data`; superproject `crypto fixtures: secret boxes seal without additional data`.

### Task 3.3: Asynchronous seal, open and from_key on every native backend (D6)

**Files:** `async.hpp`, `ciphers/aead.hpp`, `ciphers/secretbox.hpp`, `public_key_encryption/box.hpp` (member declarations), new `test/async_aead_test.cpp` (`crypto-run async_aead : async_aead_test.cpp : <library>/third_party//sodium <library>/webcpp/asio//asio ;`), `test/async_post_test.cpp`; message tests `async_seal_xchacha`, `async_open_xchacha`, `async_from_key_xchacha` (messages and MSVC twins) rewritten on a stand-in browser backend; `doc/async.adoc`, the file comment of `async.hpp`.

**Interfaces (Produces):**
- Free `async_seal(executor, C& c, iv, ad, data, token)` and `async_open(executor, C& c, iv, ad, data, tag, token)` requiring `aead<C>`; `async_seal(executor, C& c, iv, data, token)` and `async_open(executor, C& c, iv, data, tag, token)` requiring `authenticated_box<C>`; `async_from_key<C>(executor, key_bytes, token)` for every `aead` and secretbox. Members `c.async_seal`, `c.async_open` on `ciphers::aead`, `basic_secretbox`, `basic_box`, and static `async_from_key` on `ciphers::aead` and `basic_secretbox`, declared in the classes, defined in `async.hpp`. No `async_from_keys` for crypto_box (ruling P11).
- On a synchronous backend: `complete_by_post` of the synchronous member. On a backend with `asynchronous_backend_v`: AES-GCM goes to `async_engine`; any other type stops at the first statement after the guard, before any member of `C` is named (ruling P12): "webcpp::crypto: WebCrypto has no ChaCha20-Poly1305, XChaCha20-Poly1305, secret box or crypto_box: their async_seal, async_open and async_from_key run on a native backend; on the browser's backend only AES-GCM is asynchronous."
- [ ] **Step 1: Failing tests** `async_aead_test.cpp`: `xchacha_async` (spec): `co_await crypto::async_seal(executor, aead, nonce, ad, data, use_awaitable)` with `ciphers::xchacha20_poly1305<backends::sodium>` completes with the synchronous `seal`'s tag and ciphertext, member and free, then `async_open` restores the plaintext; the same for `chacha20_poly1305`, `secretbox`, `box`; `async_from_key` equals `from_key`; `never_inline` (Review Focus 4): a callback's flag is unset when `async_seal`/`async_open` return and set after `run()`; a wrong tag completes with `authentication_failed` and zeroed data. Run - Expected: compile errors.
- [ ] **Step 2: Implement**; coverage: each new free and member function enters its own guard.
- [ ] **Step 3: Message tests**: the three cases take a `stand_in_browser` backend (`asynchronous_backend_v` specialized true, as `seal_asynchronous_backend` does) and a function taking `ciphers::xchacha20_poly1305<stand_in_browser>&` (or `secretbox`), expecting the text above, `alone`; their MSVC twins likewise; `crypto.json` `fixes` rewritten; regenerate.
- [ ] **Step 4: Run** `async_aead`, `async_gcm`, `async_post`, the message tests, `example//async_seal`, `EM_CACHE=... b2 ... toolset=emscripten libs/crypto/test//async_aead` through `heavy.sh`, `generate.py --check` - Expected: pass.
- [ ] **Step 5: Commit** `crypto: every AEAD and box seals and opens asynchronously on a native backend, the browser keeping AES-GCM`.

### Task X1: A throwing `getRandomValues` never terminates the page (controller ruling)

**Context:** on Emscripten `read_entropy` calls `getentropy`, which Emscripten's `libwasi.js` serves with `randomFill` (`crypto.getRandomValues`, or node's `randomFillSync` while `MIN_NODE_VERSION` is below 19; `random_get__nothrow`): an exception thrown there unwinds through WebAssembly and aborts the program. webcpp::trystero's note P2-R16 keeps it out of WebAssembly the same way.

**Files:** `include/webcpp/crypto/detail/entropy.hpp`, new `test/random_device_browser_test.cpp` (`crypto-run random_device_browser : random_device_browser_test.cpp : <library>/third_party//bearssl : emscripten ;`), `doc/entropy.adoc`.

- [ ] **Step 1: Reproduce end to end.** `random_device_browser_test.cpp`, under node: `EM_ASM` replaces `globalThis.crypto.getRandomValues` and, when `typeof require === 'function'`, `require('node:crypto').randomFillSync` with functions that throw `new Error('planted')`, before any draw; then `random_device{}.fill(buffer)` must return `error::entropy_unavailable` with `buffer` zeroed (it was filled with `0xa5`), and `curves::p256::basic_keypair<backends::bearssl>::generate()` `entropy_unavailable`; restoring the originals, `fill` succeeds; `Object.defineProperty(globalThis, 'crypto', {value: undefined, configurable: true})` gives `errc::not_supported` (an audio worklet's case). Run `EM_CACHE=... b2 ... toolset=emscripten libs/crypto/test//random_device_browser` - Expected today: the program aborts with "planted" (record the output in the commit message's body).
- [ ] **Step 2: Fix** in `entropy.hpp`, under `#elif defined(__EMSCRIPTEN__)` (the test seam keeps priority):

```cpp
// 0: filled; 1: no crypto.getRandomValues (an audio worklet); 2: it threw.
EM_JS(int, webcpp_crypto_fill_random, (std::uint8_t* output_bytes, std::size_t byte_count), {
    const source = globalThis.crypto;
    if (!source || typeof source.getRandomValues !== 'function') return 1;
    try {
        const drawn = new Uint8Array(byte_count);  // a fresh buffer: never a view of shared memory
        source.getRandomValues(drawn);
        HEAPU8.set(drawn, output_bytes);
        return 0;
    } catch (error) {
        return 2;
    }
});
```

`read_entropy` maps 1 to `errc::not_supported` and 2 to `error::entropy_unavailable` (ruling P15); the doc comment says why Emscripten's `getentropy` is not called.
- [ ] **Step 3: Run** `random_device_browser`, `random_device`, `random_device_failing` on Emscripten and `random_device` natively - Expected: pass.
- [ ] **Step 4: The page** (`entropy.adoc`): the browser case in the errors' list.
- [ ] **Step 5: Commit** `crypto: a throwing getRandomValues gives entropy_unavailable in the browser, never a terminated page`.

### Task X1b: libsodium's randomness through crypto on Emscripten (controller ruling)

**Context:** on Emscripten libsodium draws from its own JavaScript (`randombytes.c`: `javascript_stir`, which `sodium_init` runs, tries `crypto.getRandomValues`, then node's `require('crypto').randomBytes`, then throws "No secure random number generator found"; `javascript_random` serves every `randombytes_buf`, the sealed box's ephemeral key among them). A throw there unwinds through WebAssembly. Feasible without patching libsodium: `randombytes_set_implementation` is public API, and an implementation installed before the first `sodium_init` replaces the JavaScript one, stir included. Its `buf` returns `void`, so a failure is observed through a flag that crypto reads around the calls it makes.

**Files:** `backends/sodium.hpp` (`sodium_ready`, the sealed box engine's `seal`), new `test/sodium_entropy_browser_test.cpp` (`crypto-run sodium_entropy_browser : sodium_entropy_browser_test.cpp : <library>/third_party//sodium : emscripten ;`, a program of its own so that no libsodium call precedes the plant), `doc/entropy.adoc`.

- [ ] **Step 1: Reproduce.** `EM_ASM` replaces `globalThis.crypto.getRandomValues` and `require('crypto').randomBytes` with throwing functions before any libsodium call; then `public_key_encryption::sealed_box::seal(recipient, message, output)` (`recipient` from `x25519::public_key::from_bytes`, which calls no backend). Run `EM_CACHE=... b2 ... toolset=emscripten libs/crypto/test//sodium_entropy_browser` - Expected today: the program aborts ("No secure random number generator found"); record the output in the commit message's body.
- [ ] **Step 2: Implement** in `backends/sodium.hpp`, under `#ifdef __EMSCRIPTEN__`:

```cpp
namespace webcpp::crypto::detail {
inline thread_local int sodium_draws_watched = 0;     // the depth of the calls crypto observes
inline thread_local bool sodium_draw_failed = false;  // set by a failed draw inside one
inline void sodium_entropy_buf(void* output_data, std::size_t byte_count) {
    if (!fill_from_system({static_cast<std::uint8_t*>(output_data), byte_count})) {  // zeroed
        if (sodium_draws_watched == 0) {
            sodium_misuse();  // no caller could see the failure: libsodium's own reaction
        }
        sodium_draw_failed = true;
    }
}
// random() draws 4 bytes through buf; stir() does nothing; implementation_name() "webcpp-crypto";
// uniform and close are null (libsodium derives uniform from random).
inline randombytes_implementation const sodium_entropy{&sodium_entropy_name, &sodium_entropy_random,
                                                       &sodium_entropy_stir, nullptr,
                                                       &sodium_entropy_buf, nullptr};
struct sodium_draw_watch {  // RAII: ++watched and clears the flag; --watched on exit
    [[nodiscard]] bool failed() const noexcept { return sodium_draw_failed; }
};
}
```

`sodium_ready()` calls `randombytes_set_implementation(&sodium_entropy)` once (a function-local static), before its first `sodium_init()`, on Emscripten only. The sealed box engine's `seal` holds a `sodium_draw_watch` around `crypto_box_seal` and, when a draw failed, wipes `out` and returns `error::entropy_unavailable`.
- [ ] **Step 3: Tests** in the same program, after the plant: `seal` returns `entropy_unavailable` with `output` zeroed; `ed25519::keypair::from_secret(seed)` succeeds (`sodium_init`'s stir is crypto's); restoring the originals, `seal` succeeds and `open` with the recipient's key pair restores the message. Run it, `box_sodium` and `random_device_browser` on Emscripten, `box_sodium` natively - Expected: pass.
- [ ] **Step 4: The page** (`entropy.adoc`): libsodium draws through crypto on Emscripten; a program that sets its own libsodium implementation sets it after crypto's first libsodium call.
- [ ] **Step 5: Commit** `crypto: libsodium draws through crypto's entropy in the browser, and a sealed box reports a failed draw`.

### Task 4.1: The page, the tables, the changelog

**Files:** `doc/curves.adoc` (in "Signatures: the scheme and the input": a subsection "Signatures as values": the six types, `from_bytes` at the wire, `bytes()`, `recovery_id()`, the typed format helpers; a subsection "Factories named by their input" with D3c's table and the predictable-key hazard; "Key pairs by what they do" with D4's table), `doc/x25519.adoc` (typed keys, the four forms, the conversion), `doc/kx.adoc` (`derive_from_seed` against `from_keypair`), `doc/aead.adoc` (the three concepts, D5's table, the free forms, asynchronous on native, the browser's AES-GCM), `doc/public_key_encryption.adoc` (the namespaces by purpose), `doc/tables/crypto.json` (the signing, key pair and factories rows; a row for `aead.hpp`'s concepts), `doc/changelog.adoc`. Superproject: `CODING-GUIDELINES.md` crypto chapter, a bullet beside "One name per signing operation" ("Every value has its type: signatures per curve and scheme, X25519's keys; key pairs, AEADs and boxes checked by concepts; constructions by purpose; factories named by their input", with its Source lines).
- [ ] **Step 1:** Write the sections; each code snippet is an example's own code included by tag (`four_forms.cpp`, `x25519_agree.cpp`, `kx_session.cpp`, `xchacha_seal.cpp`, `signed_message.cpp`).
- [ ] **Step 2: Changelog** (no new line; ten at most): line 8 becomes "Added ciphers::secretbox and secretbox_xchacha20poly1305 without additional data, public_key_encryption's box, box_xchacha20poly1305 and sealed_box, key_exchange::kx_keypair on libsodium, on typed X25519 keys." (30 words or fewer); line 10 becomes "Added signing.hpp, keypair.hpp, factories.hpp, aead.hpp, wipe.hpp: typed signatures, sign(scheme, input), verify; keypair, signing_keypair, agreement_keypair; aead, authenticated_box; from_secret, from_bytes, from_key; four forms per operation."; the other lines stay unless they name a removed function. Count the words.
- [ ] **Step 3: Run** `grep -rn 'hybrid::\|x25519_secret\|to_x25519_public\|make\.hpp\|crypto::make\b' libs/crypto/doc` - Expected: nothing. `generate.py`, `--check`, `generate_test.py`, `b2 ... libs/crypto/doc` - Expected: no warning about an anchor or include.
- [ ] **Step 4: Commit** `crypto: the page shows typed signatures, typed X25519, the concepts and the factories by input`; superproject `guidelines: crypto's values are typed, checked by concepts and made by factories named by their input`.

### Task 4.2: Full local verification and delivery (controller)

Each through `/Users/you/dev/webcpporg/heavy.sh crypto-types`:
- [ ] clang full lane `-a` of `libs/crypto/test libs/crypto/example libs/crypto/bench`, with `libs/hash/test libs/hash/example`; gcc-15 (`toolset=gcc-15`) and Emscripten full lanes of the same three crypto directories.
- [ ] wasip2 (`toolset=clang-wasip2 testing.launcher=wasmtime`) and wasip3 (`toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh`) of `libs/crypto/test libs/crypto/example`.
- [ ] MSVC 14.3 and 14.5 with msvc-lane on a self-contained tree (`run-lane.sh <v> --tree DIR --fresh -- libs/crypto/test libs/crypto/example`); `user_globals` passes on both.
- [ ] Linux lint `--full` (self-contained clone, `linux-lint`) and the macOS full lint.
- [ ] Constant-time lane (memcheck, MemorySanitizer, dudect) for the changed secret paths: secp256k1's `sign(ecdsa_keccak256_recoverable, ...)`, the sealed box's seal with crypto's entropy (X1b, Emscripten has no checker: natively the path is unchanged), X25519's typed key pair (`from_secret`, `agree`, the moved-from wipe), kx `derive_from_seed` and `from_keypair`, Ed25519's `to_x25519`, secretbox `seal`/`open` without additional data, `crypto::wipe`, the renamed factories (`from_secret` on the three signing curves, `from_key` of AES-GCM, ChaCha20-Poly1305, secretbox), and P-256 and secp256k1 signing returning the typed signature; `paths.hpp` and the suppressions (`bearssl_verdicts.supp`, `sodium_verdicts.supp` frames renamed by X3 and 2.5) updated by the tasks; dudect natively and on wasip2.
- [ ] `python3 tools/test/crypto_jamfiles_test.py` (every case, native and Emscripten); `generate.py --check`; the X3 survey (empty).
- [ ] Merge `origin/main` into the three branches; pull requests in webcpporg/crypto, webcpporg/proposals (tasks ticked, design.md's "As built", status `implementing`), webcpporg/webcpp (the pointer); merge commits; one CI at the end.

## Rulings made in the plan

- **P1, the order** (header): X3, 2.5 and 2.4 first. Cost if wrong: none in behaviour; a late 2.5 would rename the new types' `make` twice.
- **P2, X3's test is new.** The brief's "existing MSVC hides-no-user-name test" (48ca7ef) does not exist: that commit renamed `make.hpp`'s parameters; a200e69 moved a test's global into a local. `user_globals_test.cpp` declares the survey's names before every public header, its third-party headers included first; only MSVC diagnoses (clang and GCC `-Wshadow` never warn across namespaces, checked). Cost: a one-word name added later outside the list is caught only when it collides on MSVC.
- **P3, X3's names**: the table in X3, "role then kind"; `timing_scope` for the guard. Cost: a rename of renames if the owner prefers another word.
- **P4, engine statics named `make` are renamed** (`import_secret`, `seed_keypair`, `heavy_state::allocate`): the spec says no function is named `make`; decision 9's "backends' internal capabilities keep their names" means the capability rows (`*_import`), which stay. Cost: internal only.
- **P5, Ed25519ph's signature is `curves::ed25519::ph_signature`**, distinct: D1's table omits Ed25519ph; the spec's "a type per curve and scheme" decides. Cost: one more alias; a ph signature given to the pure verify stops the build instead of failing at run time.
- **P6, signatures have a public default constructor (all zeros)**, so they are `std::regular` as the spec says and the asynchronous error completion's empty value exists. Cost: a zero signature can be named; every verify refuses it.
- **P7, `can_verify` gains the signature: `can_verify<Scheme, Signature, Input, PublicKey>`** (D1 says it is false for a signature of another curve), with `detail::signature_of<Key, Scheme>` as "the trait gains the signature type" (a scheme alone does not know its curve). Cost: callers of the concept add one argument.
- **P8, `to_der`, `from_der`, `normalize_low_s`, `is_low_s` take and give the typed ECDSA signature** on both curves (D1 names the last two; the first two would otherwise be the last byte-signature functions). Cost: the formats change builds on typed values, as it intends.
- **P9, MuSig2's aggregate is a `schnorr_signature`**; partial signatures keep their bytes (crypto-sign's R7). Cost: MuSig2 callers add `.bytes()`.
- **P10, secretbox gets its own engine** (`detail::secretbox_state`/`secretbox_engine`, no additional-data parameter) and its families move to `secretbox.hpp`, so that `ciphers::aead` names only AEADs. Cost: sodium.hpp's two specializations rewritten.
- **P11, no `async_from_keys` for crypto_box**: D6 lists `async_from_key` alone; a box is made from typed keys. Cost: a program that wants it posts the synchronous call itself.
- **P12, D6's refusal fires before any member of the type is named**, and its text lists the algorithms (a C++20 `static_assert` cannot compose the type's name). Cost: the message does not name the one type used.
- **P13, `detail::x25519_access`** gives kx, the boxes and the sealed box the key pair's bytes without a nested guard or a copy. Cost: one friend struct.
- **P14, `kx_keypair::from_keypair` takes the key pair by value** (a sink; the key pair is move-only), and `keypair()` gives it back by reference. Cost: an lvalue needs `std::move`.
- **P15, X1 calls `crypto.getRandomValues` from its own `EM_JS`** through a fresh `Uint8Array`: no `crypto` object is `not_supported` (the audio worklet's case, kept), a throw is `entropy_unavailable`. Cost: node versions without `globalThis.crypto` (before 19, which webcpp does not run) would read `not_supported`.
- **P16, `crypto::wipe` is one constrained template plus a refusing overload**; no `(void*, size)` form (no caller needs it). Cost: a C buffer of runtime size goes through `std::span`.
- **P17, the free X25519 byte `async_agree` is removed in 2.1** and `async_agree` becomes one template constrained by `agreement_keypair` in 3.1.
- **P18, `doc/hybrid.adoc` becomes `doc/public_key_encryption.adoc`** with its anchor; `kx.adoc` keeps its name.
- **P19, the free factories hold a guard**, as `crypto::make` did, even over a member that calls no backend (`from_bytes` of a signature). Cost: DIT set around a copy.
- **P20, `to_x25519()` gives a key pair of the same backend**, refused at compile time when that backend has no X25519.
- **P21, 1.1's message tests for a signature of another curve move to 1.2**, where `verify` first takes the types.

- **P22, the Keccak message form needs `/third_party//xkcp` in the program's Jamfile** (R19): secp256k1's family default does not bring XKCP, which builds on every target and toolset; a marker needs none. Cost: an Ethereum program adds one Jamfile line.
- **P23, both recoverable schemes share `recoverable_signature`** (the controller's ruling, R20). Cost: a signature recovered under the wrong scheme gives a wrong key at run time, not a build error.
- **P24, `ecdsa_keccak256_recoverable` has no `verify`**: `ecdsa_sha256_recoverable` has none (the catch-all sends its signature to `recover`), so symmetry says none (R18).
- **P25, X1b installs crypto's implementation unconditionally on Emscripten**, before crypto's first `sodium_init`: libsodium has no way to read the current implementation without initialising it, which runs the stir that throws. Cost: a program's own implementation set earlier is replaced (the page says to set it later).
- **P26, a failed draw outside a call crypto observes calls `sodium_misuse()`**, libsodium's abort: handing zeros to a caller that cannot see a failure would be worse. Cost: the program's own libsodium calls still stop on a failed generator, as libsodium intends.

## Design defects and gaps found

Recorded in design.md (R8/R9 noted, R14-R20, decisions 1 to 10, the spec's `make` sentence aligned with R17); what remains:

- D6's refusal cannot be the first message unless it precedes every use of the AEAD's members: naming an AEAD's engine on an unknown or browser backend fires the capability table's refusal first (P12).
- Open item: if webcpp::hash, which shares libsodium, calls `sodium_init` before crypto's first call, libsodium's JavaScript stir runs first and can still throw; crypto cannot reach it (hash would install the same implementation).
- Open item: natively, libsodium's `randombytes_sysrandom` aborts on a failed generator; X1b changes Emscripten only, as ruled.
