## Context

webcpp::crypto's change data-independent-timing (`libraries/crypto/openspec/changes/data-independent-timing/`,
its design D1 to D7 and its plan's rulings R1 to R10) puts an RAII guard,
`webcpp::crypto::detail::timing_guard`, first in every public function that runs a backend's
code, so that PSTATE.DIT is set for the call on 64-bit ARM under Apple's systems and Linux where
the CPU has FEAT_DIT, and the thread's state is as the program left it afterwards. It names
webcpp::hash as out of scope and its keyed functions as a follow-up. This change is that
follow-up, decided by the controller on 2026-11-20 with the owner on autopilot.

webcpp::hash today (`main`, with increment 6, Botan, on its way: the stream on the branches
`hash/botan-6`): Boost.Hash2's algorithms and `hmac<H>` by alias; Keccak-256 and BLAKE3 in Hash2's
form over XKCP and BLAKE3's C code; the families `digest_of`, `hasher`, `mac`; HKDF and PBKDF2
(webcpp's own over Hash2's `hmac` on `backends::hash2`, and libsodium's, BearSSL's, OpenSSL's,
WebCrypto's); `blake2b_subkey`; KBKDF, SSKDF and X9.63 KDF; Argon2id, Argon2 with lanes, scrypt;
from increment 6 Botan's hashes, MACs and KDFs (`backends/botan.hpp`) and bcrypt (`bcrypt.hpp`);
the asynchronous family (`async.hpp`), which posts a synchronous function to a pool's thread for
a heavy function and runs a cheap one at once, and which on Emscripten can name
`backends::webcrypto`, the browser's.

The libraries' dependencies: crypto's target `/webcpp/crypto//crypto` gives
`/webcpp/hash//hash` (its `build.jam`: Ed25519ph's prehash is hash's SHA-512, crypto's design D1);
hash's target gives Boost's headers and `/webcpp/config//config` and nothing of crypto. crypto
depends on hash; hash does not depend on crypto.

## Goals / Non-Goals

**Goals:** every computation of webcpp::hash on a key, a password or a key derivation's secret
input runs with DIT set where the CPU has it, with no call in the program, on whatever thread
computes it; the thread's state is as the program left it after every call; a test fails when a
keyed function forgets its guard, and when the capability table gains a keyed cell no test calls.

**Non-goals:** unkeyed hashing (D2); Boost.Hash2's types used by alias (D3); a constant-time lane
for hash (none exists; D9); Windows on ARM64, x86's DOITM and WebAssembly, as crypto's design
D7 and its non-goals rule; a public guard type.

**Anti-goals:** leaving DIT set in the program's thread after a call returns; a call that traps on
an arm64 CPU without DIT; a dependency of hash on crypto.

## Decisions

### D1. Where the guard applies: a key, a password or a secret input

A public function of webcpp::hash holds `detail::timing_guard` as its first statement when one of
its inputs is a key, a password or a key derivation's secret input. By family:

- **MACs:** `mac<Mac, Backend>`, every `macs::` tag on every backend (HMAC over every digest,
  KMAC, SipHash-2-4 and its 128-bit form, BLAKE2b's and BLAKE2s's MACs, Botan's).
- **Key derivation:** `hkdf`, `hkdf_extract`, `hkdf_expand`, `pbkdf2`, `kbkdf`, `sskdf`,
  `x963kdf` (their secret is the shared secret Z, not called a key), `blake2b_subkey`.
- **Password hashing:** `argon2id`, `argon2<Variant>`, `scrypt`, and from increment 6 `bcrypt`,
  `bcrypt_verify`, `bcrypt_pbkdf`.
- **Keyed modes of a digest,** engaged by the arguments (D4): `digest_of<digests::blake2b>` and
  `hasher<digests::blake2b>` (`make`, `update`, `finish`) when the options carry a key; the
  `blake3` class's `keyed`, `derive_key`, its seeded constructors when the seed makes it keyed (a
  non-empty byte seed, a non-zero integer seed: BLAKE3's keyed hash, as the class documents), and
  `update`, `result`, `squeeze` of an object in one of those modes.

The rule is mechanical, as crypto's is: the family says whether a function is guarded, never a
judgment per backend. A forwarder holds its own guard (`hkdf` calls `hkdf_extract` and
`hkdf_expand`, each guarded); guards nest (crypto's R2). A function's size and parameter
checks run inside the guard: the guard is the first statement.

### D2. Unkeyed hashing holds no guard

`digest_of`, `hasher` and the Hash2-form classes of an unkeyed digest or XOF (SHA-1, SHA-2,
SHA-3, SHAKE, Keccak, cSHAKE, TupleHash, ParallelHash, KangarooTwelve, TurboSHAKE, BLAKE2s,
BLAKE2b without a key, BLAKE3's plain mode, RIPEMD-160, SM3, Streebog, MD5) hold none, for three
reasons:

1. **Their input is not a secret by their contract.** A digest is made to be published; a program
   that must keep a value secret and derive from it uses a MAC or a KDF, which hold the guard.
   Hashing a secret without a key is the misuse the library's pages already steer away from
   (`doc/kdfs.adoc`, `doc/password_hashing.adoc`).
2. **They are the streaming case.** An unkeyed `hasher::update` or a Hash2 class's `update` is
   called once per chunk, often per network read; a guard per call is an `mrs`, and where DIT is
   clear an `msr`, an `isb`, another `msr` and `isb` on each, a real fraction of a short chunk's
   cost (crypto's measured figure, `doc/performance.adoc` of crypto), paid by every checksum and
   content address for no secret.
3. **The rule would not hold anyway.** The same `sha2_256` is also Boost.Hash2's type by alias
   (D3), which hash cannot guard; guarding `digest_of<digests::sha2_256>` on OpenSSL and not
   `webcpp::hash::sha2_256` would make the protection depend on the spelling.

A keyed mode of a digest is a MAC or a KDF, and is guarded (D1, D4). Keccak-256's seeded
constructors follow Boost.Hash2's seeding, a hash table's defence, not a keyed construction a
standard defines: they are unkeyed in this sense, unlike BLAKE3's, whose seed selects BLAKE3's
own keyed hash.

### D3. Boost.Hash2's types by alias hold no guard

`hmac<H>`, `hmac_sha2_256` and the other `hmac_*`, `siphash_32`, `siphash_64` and every Hash2
algorithm are Boost.Hash2's own classes, which hash names by alias (the genesis's design, spec
"Boost.Hash2: every algorithm by alias", "HMAC by alias"). A guard inside them would need a
wrapper type of hash's in place of the alias, which changes the public type a program already
uses with Boost.Hash2's own code (`boost::hash2::hash_append`, its `hmac<H>` of any `H`) and
breaks the alias's promise. They stay unguarded and the documentation says so: a program that
computes an HMAC with `hmac<H>` directly sets DIT itself (Apple's
`timingsafe_enable_if_supported`) or calls `mac<macs::hmac<D>>`, which runs the same Boost.Hash2
code on `backends::hash2` inside a guard; HKDF and PBKDF2 on `backends::hash2` run Hash2's
`hmac` inside theirs. Hash2's `siphash_*`, a hash table's seeded hash, are not MACs by their
documentation; `mac<macs::siphash_2_4>` is.

### D4. hash's guard is its own copy, with an engaged flag

hash cannot use crypto's guard: crypto depends on hash, and the reverse would make a cycle;
crypto's `detail` is crypto's; webcpp's configuration library declares no namespace
(`CODING-GUIDELINES.md` chapter 3); and a public guard in either library would widen its
interface with something that is neither cryptography nor hashing. So hash has
`webcpp::hash::detail::timing_guard` in `detail/timing.hpp`, crypto's code with crypto's
reasons in its comments (the register by its encoding `s3_3_c4_c2_5`, bit 24, `isb` after each
write, `sysctlbyname("hw.optional.arm.FEAT_DIT")` on Apple, `getauxval(AT_HWCAP) & HWCAP_DIT` on
Linux, nothing executed on a CPU without DIT, an empty class on every other target), its macros
`WEBCPP_HASH_DETAIL_TIMING_COVERED` and `WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)`, and one
addition:

```cpp
explicit timing_guard(bool engaged = true,
                      std::source_location where = std::source_location::current()) noexcept;
```

A disengaged guard does nothing and fires no seam; the keyed modes of D1 construct it with
whether the call is keyed. crypto's guard, which needs no such flag, is not changed.

The two copies are kept in step by a test in crypto, which can include both
(`libs/crypto/test/timing_hash_interop_test.cpp`): a hash guard inside a crypto guard and the
reverse leave the thread's DIT as it was in both starting states, both libraries' `cpu_has_dit()`
and `dit_is_set()` agree, and a crypto call that uses hash (Ed25519ph's SHA-512, unkeyed, so no
hash guard) enters crypto's guard alone. Each header's comment names the other copy and the
test.

### D5. The seam and the coverage tests, driven by the capability table

`WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)` expands to nothing unless a test defines it, as
crypto's does; `test/support/timing_seam.hpp` (hash's, the same matcher as crypto's) records the
number of guards a thread entered and the function of the first. One coverage test per backend,
`test/timing_coverage_<backend>_test.cpp` for `hash2`, `sodium`, `bearssl`, `openssl`, `xkcp`,
`blake3` and `botan`, built where the backend is and with exceptions on and off where it allows
(Botan: exceptions on, natively and on Emscripten), holds a registry of calls, one per keyed cell
of that backend, each named as the table names the cell (`mac<macs::hmac<digests::sha2_256>>`,
`hkdf<hmac_sha2_256>`, `argon2<kdfs::argon2id>`, ...). It reads `doc/capabilities.json` at run
time (the `.hash-tables` properties `capabilities_test` uses) and fails when a keyed cell of its
backend (D1's families: the chapters `macs`, `kdfs` and `password_hashing`, kinds other than
`type`, and the keyed digests of D1) has no call, or a call names no cell; then it runs each call
with valid input and with input refused before the backend, and fails when the call enters no
guard, when the first guard it enters is not its function's own, or when DIT is not as before
after it. The unkeyed functions of D2 are called through `uncovered`, which fails if they enter a
guard: the boundary of D2 is tested too. A new keyed cell, Botan's or a later backend's, fails
the test until its call is written.

### D6. The asynchronous forms

The initiating functions of `async.hpp` hold no guard: they check sizes and post. The computation
is the synchronous function they call, which holds its guard on the thread that runs it: a
heavy function (`async_pbkdf2`, `async_argon2id`, `async_argon2`, `async_scrypt`, and from
increment 6 `async_bcrypt`, `async_bcrypt_verify`, `async_bcrypt_pbkdf`) on a thread of the work
executor's pool, so that the pool's thread has DIT for the computation and gets its state back
after it; a cheap one (`async_hmac`, `async_hkdf`) on a synchronous backend runs at once in the
initiating call, on the caller's thread, inside its guard. `async_digest` computes an unkeyed
digest (D2). On `backends::webcrypto` the browser's JavaScript engine computes, outside the
program's thread state, on a target the guard does not cover; nothing is added there. A test
proves each form on the thread that computes (`test/timing_async_test.cpp`).

### D7. Measurements

The guard's cost is measured by hash's own release program (`bench/timing.cpp`, as crypto's) and
rendered by hash's `bench/collect.py --timing` into `doc/performance.adoc` and its data; the
keyed functions' throughput tables are regenerated from release builds. No number is typed by
hand. Two libraries, two pages, each from its own build: hash's documentation does not depend on
crypto's.

### D8. Order

The change starts after hash's increment 6 (Botan) merges, so that its inventory and coverage
tests include `backends/botan.hpp` and `bcrypt.hpp` from the start. If it must start before, it
covers `main`'s functions and Botan's stream adds its guards and its coverage file; the
coverage test's check against the table (D5) makes either order safe: a Botan cell without a
call fails.

### D9. No constant-time lane for hash in this change

hash has no constant-time lane (crypto's `test/constant_time/` is crypto's). The guard is proven
by its own tests and the coverage tests; whether the keyed functions' timing is independent of
their secrets on each backend is upstream's statement, which `doc/capabilities.json`'s
`constant_time` cells record. A dudect lane for hash's keyed functions is a separate change,
not decided.

## Risks / Trade-offs

- [A program hashes a secret with an unkeyed digest] -> it gets no DIT from hash; the pages say
  so and point to MACs and KDFs (D2).
- [A program uses `hmac<H>` directly] -> no DIT from hash (D3); the HMAC page says what to call
  instead.
- [The two copies of the guard drift] -> crypto's interop test fails on a behavioural
  difference, and each header names the other; a change to one is a change to both, which the
  guidelines' hash section states.
- [A guard per `update` of a keyed `hasher` or `blake3`] -> the price of keyed streaming, the same
  as crypto's GCM `update`; measured (D7).
- [A worker thread of a pool keeps DIT] -> it does not: the guard restores the state the thread
  had, on that thread, when the synchronous function returns.

## As built (2026-11-22)

The change was built on the branches `hash/dit` of the library and the superproject (and
`hash/dit-interop` of crypto for its one test), by `reference/plan.md`, whose rulings H1 to H17
stand; H10 to H17 patched the plan against `main` after increment 6 (Botan) and P-256's BearSSL
pin move. What differs from the decisions above, or adds to them:

- **D1, `hkdf` holds one guard.** It calls `hkdf_guards` and its backend's `extract_and_expand`,
  not the public `hkdf_extract` and `hkdf_expand`: a call to `hkdf` enters one guard, its own.
  Each of the three holds its guard as its first statement. Botan's engines run inside the
  templates' guards; `detail::botan_boundary` holds none (H10). `bcrypt_verify` is guarded,
  though the capability table holds it in `bcrypt`'s row (H12).
- **D1, the `blake3` class keeps a `keyed_` flag:** its object grew from 1,920 to 1,928 bytes,
  which its documentation states and a `static_assert` pins.
- **D4, the guard is crypto's as built.** Its constructor takes a `std::source_location` only
  where a test defines the seam (H14), and `timing_unseamed_release` reads its own release
  program for `mac`'s name and header path. A disengaged guard does nothing and fires no seam.
  crypto's header does not yet name hash's copy (this change owned one file of crypto, its test):
  hash's header, crypto's `test/timing_hash_interop_test.cpp` and `CODING-GUIDELINES.md` name
  both. **Open item:** crypto's header names the other copy at crypto's next change of it.
- **D4, the interop test pins exact counts:** an `hkdf` beside an Ed25519 signature enters one
  guard of each library, each the function's own; Ed25519ph over hash's SHA-512 enters crypto's
  alone.
- **D5, the coverage calls come from the code's table.** Each backend's test builds its calls for
  the families the test support knows from `backend_has_v`, which `capabilities_test` keeps equal
  to `doc/capabilities.json`; a cell of a family or function the support does not know fails
  until its call is written, and a call naming no cell fails. Each call runs with valid input and
  with input its guard refuses (the refusal's own error checked). The support names three keyed
  functions beside the table's cells: keyed `digests::blake2b`, `blake3`'s keyed modes and
  `bcrypt_verify`.
- **D5, DIT is observed during the computation,** not only on the way out: `timing_during` runs
  `hkdf_extract`, `hkdf_expand`, `hkdf`, `pbkdf2` and `mac` over a probe hash whose `update`
  records DIT, and `timing_during_async` runs `async_pbkdf2` on a pool; a guard moved after the
  engine call, or written as a temporary, failed them while the coverage tests still passed.
  `timing_botan_failure` runs Botan's exception path inside a guard.
- **D6** held as decided: every asynchronous form computes by calling its synchronous function
  (`timing_async`, `timing_async_botan`).
- **D7, the guard's cost only** (H16): 45.4 ns per call where DIT was clear, 0.7 ns where it was
  set, 0.0 ns for a disengaged guard (Apple M5 Max, release, 2026-11-22 UTC, load averages
  61/271/333). **Open item:** these figures are provisional, measured on a loaded machine; the
  final quiet measurement pass measures them again and regenerates the throughput tables, which
  this change did not.
- **Verification:** an arm64 Linux container (the Linux lint's image) reports HWCAP_DIT, and the
  timing, coverage and asynchronous tests pass there on clang 18 and gcc 14. On MSVC 14.3 and
  14.5 under Wine, seven no-exceptions builds that include Boost.JSON (`timing_keyed`, the
  coverage tests) stop with no diagnostic, the lane's documented class; their twins with
  exceptions pass. **Open item:** the lane's known-failures list gains them (the owner's tool).
- **Found on the way:** a reference (MrDocs) that includes hash now reads only the backends its
  build links, and the refusal of OpenSSL on Emscripten never stops a reference (trystero's
  reference targets Emscripten); crypto's `config.hpp` has the same exposure (its
  `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` refusal), crypto's to fix. `async_heavy`'s cancellation
  case was made deterministic.
