## Why

On 64-bit ARM, the instructions Arm lists take a time independent of their data only while the
thread's DIT bit (PSTATE.DIT, data-independent timing) is set, and Apple requires it of
constant-time code on Apple silicon. On 2026-11-20 the owner decided that webcpp::crypto sets
DIT itself around its secret operations (the change
`libraries/crypto/openspec/changes/data-independent-timing/`), and named webcpp::hash's keyed
functions as a follow-up. webcpp::hash computes on secrets too: an HMAC's key, an HKDF's input
keying material, a password under PBKDF2, Argon2, scrypt or bcrypt, a KMAC's or a keyed BLAKE2b's
key. None of the C libraries it wraps (libsodium, BearSSL, OpenSSL, XKCP, BLAKE3, Botan) sets
DIT, and neither does Boost.Hash2, so a program on Apple silicon that derives a key from a
password today runs that derivation without it unless it sets DIT itself, which almost none will.

## What Changes

- **A guard of hash's own, `detail::timing_guard`** in `detail/timing.hpp`, the same type as
  crypto's (on macOS and Linux on arm64, iOS and Android included, where the CPU has DIT, it sets
  the bit when the thread does not have it and clears it again on destruction; elsewhere it
  compiles to nothing), with one addition: a guard can be constructed disengaged, for a function
  that is keyed or not by its arguments.
- **Every public function that computes on a secret holds a guard** for its whole body: `mac`
  (HMAC, KMAC, SipHash, the keyed BLAKE2 MACs, on every backend), `hkdf`, `hkdf_extract`,
  `hkdf_expand`, `pbkdf2`, `kbkdf`, `sskdf`, `x963kdf`, `blake2b_subkey`, `argon2id`, `argon2`,
  `scrypt`, `bcrypt`, `bcrypt_verify`, `bcrypt_pbkdf`, on every backend, Botan's included; and,
  engaged only when keyed, `digest_of` and `hasher` of BLAKE2b with a key and the `blake3` class
  in its keyed, key-derivation and seeded modes.
- **Unkeyed hashing holds no guard**: a digest of public data, an XOF, Keccak-256, and Boost.Hash2's
  types used by alias (its `hmac<H>` included, which hash cannot change without wrapping it).
  The reasons are in the design (D2, D3).
- **The asynchronous forms set DIT on the thread that computes**: the synchronous function each
  one runs holds the guard, on the pool's thread for a heavy function and on the calling thread
  for a cheap one; WebCrypto's computations are the JavaScript engine's.
- **A seam, `WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)`**, and a coverage test per backend
  driven by `doc/capabilities.json`, which fails when a keyed cell of the table has no call, when
  a call enters no guard, or when the first guard it enters is not its function's own; and a test
  of the asynchronous forms on the computing thread.
- **The documentation** says what the library sets, where, what it does not cover and why, and
  the guard's cost from a release build.

Not changed: the public interface (no signature changes; `hasher` and `blake3` gain a private
member), the errors, any C library, webcpp::crypto's guard (crypto gains one test that the two
guards agree).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `hash`: every public function that computes on a secret runs with arm64's DIT set where the
  CPU has FEAT_DIT, and restores the thread's state; a test proves each enters its own guard.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Implementation:** none yet; the tasks of `reference/plan.md` are open.
- **Status:** proposed: the owner decided DIT for crypto on 2026-11-20 and named hash as the
  follow-up, which awaits its own decision.

`libs/hash/include/webcpp/hash/` (the new header and the headers of the keyed functions),
`libs/hash/test/` (the guard's tests, the coverage tests, the asynchronous test), `libs/hash/bench/`
(the guard's cost), `libs/hash/doc/`; `libs/crypto/test/` (one test of the two guards together);
the superproject's `CODING-GUIDELINES.md` (hash's section).
