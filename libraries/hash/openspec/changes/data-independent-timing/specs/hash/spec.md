## ADDED Requirements

### Requirement: Timing: the library sets DIT around its keyed computations
On 64-bit ARM under macOS, iOS, Linux and Android, where the CPU has FEAT_DIT, every public function of webcpp::hash one of whose inputs is a key, a password or a key derivation's secret input SHALL run its whole body with PSTATE.DIT set, through `detail::timing_guard`, on every backend, and MUST return with the thread's DIT as it was before the call: `mac`; `hkdf`, `hkdf_extract`, `hkdf_expand`, `pbkdf2`, `kbkdf`, `sskdf`, `x963kdf`, `blake2b_subkey`; `argon2id`, `argon2`, `scrypt`, `bcrypt`, `bcrypt_verify`, `bcrypt_pbkdf`; `digest_of` and `hasher` of `digests::blake2b` when the options carry a key; and the `blake3` class in its keyed, key-derivation and seeded keyed modes. Where the CPU has no DIT, or on any other target, the guard SHALL do nothing and MUST NOT execute an instruction that traps.

#### Scenario: A password derived on Apple silicon
- **WHEN** `argon2id` runs on a thread whose DIT is clear
- **THEN** DIT is set while the backend computes, and clear again when `argon2id` returns

#### Scenario: A program that set DIT itself
- **WHEN** `mac<macs::hmac<digests::sha2_256>>` runs on a thread whose DIT is already set
- **THEN** it is still set when the call returns

#### Scenario: A keyed BLAKE2b hasher
- **WHEN** a `hasher<digests::blake2b>` made with a key is updated and finished
- **THEN** each of `make`, `update` and `finish` enters the guard, and the same hasher made without a key enters none

### Requirement: Timing: unkeyed hashing and Boost.Hash2's aliases hold no guard
Unkeyed digests and XOFs through `digest_of`, `hasher`, `keccak_256` and `blake3` in its plain mode, and Boost.Hash2's types named by alias (`hmac<H>` and the `hmac_*` aliases, `siphash_32`, `siphash_64`, every Hash2 algorithm) SHALL NOT hold a guard, and the documentation MUST say so with the reason and name the guarded function to call instead (`mac<macs::hmac<D>>`, the KDFs).

#### Scenario: A checksum
- **WHEN** `digest_of<digests::sha2_256>` runs on any backend
- **THEN** it enters no guard

### Requirement: Timing: hash's guard is its own and agrees with crypto's
webcpp::hash SHALL hold its own `webcpp::hash::detail::timing_guard`, with crypto's behaviour and a constructor that takes whether it is engaged, and MUST NOT depend on webcpp::crypto. A test of webcpp::crypto, which depends on hash, MUST check that the two guards nest in either order and leave the thread's DIT as they found it, and that both libraries report the same `cpu_has_dit()`.

#### Scenario: A crypto guard around a hash guard
- **WHEN** a hash guard is entered inside a crypto guard on a thread whose DIT was clear
- **THEN** DIT is set inside both, still set after the hash guard, and clear after the crypto guard

### Requirement: Timing: every keyed cell is tested to enter its guard
A seam `WEBCPP_HASH_DETAIL_TIMING_ENTERED(where)`, empty unless a test defines it, SHALL fire on every engaged guard's construction with the guard's source location. A coverage test per backend MUST read `doc/capabilities.json` and fail when a keyed cell of that backend has no call in the test or a call names no cell, then call each with valid input and with input refused before the backend, and fail when the call enters no guard, when the first guard it enters is not its function's own, or when DIT is not as it was after it; the functions of the requirement above MUST be checked to enter none. The tests SHALL run on every lane where their backend builds.

#### Scenario: A new keyed cell
- **WHEN** a backend gains a MAC in `doc/capabilities.json` and no coverage call is written for it
- **THEN** that backend's coverage test fails, naming the cell

### Requirement: Timing: the asynchronous forms set DIT where the computation runs
An asynchronous function of `async.hpp` on a synchronous backend SHALL compute through the synchronous function that holds the guard, on the thread that computes: a thread of the work executor for a heavy function, the calling thread for a cheap one; its initiating call MUST NOT hold a guard otherwise. On `backends::webcrypto` nothing SHALL be added. A test MUST observe each keyed asynchronous function's guard on the thread that computed it.

#### Scenario: Argon2id on a pool
- **WHEN** `async_argon2id` runs on a one-thread `webcpp::asio::thread_pool` from a thread whose DIT is clear
- **THEN** the pool's thread entered `argon2id`'s guard, the calling thread entered none, and both threads' DIT is clear after the completion
