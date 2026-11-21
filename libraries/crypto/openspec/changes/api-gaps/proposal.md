## Why

Version 0.2 of webcpp::crypto (the foundation's increments 1 and 2, merged on 2026-11-20) left
six gaps that its reviews and its first CI runs found, which the owner approved closing on
2026-11-20, before the asynchronous family and the foundation's increment 3:

1. `ciphers::gcm`'s one-shot `seal` leaves `data` half encrypted when the backend fails, where
   `ciphers::aead`, `hybrid::box` and the sealed boxes zero it.
2. `gcm` has no way to open a message too large to hold whole: the genesis left it out
   ("GCM: no incremental open") until one could withhold the plaintext until the tag verifies.
3. `ciphers::aes256<>` names AES-GCM's default backend, which on wasip2, wasip3 and Emscripten
   is BearSSL, which has no block cipher: its `make` does not compile on three of the four
   targets, and the documentation states it as a rule rather than fix it.
4. `curves::secp256k1::basic_keypair` has no `generate()`, where `ed25519::basic_keypair` and
   `x25519::kx_keypair` have one.
5. `curves::secp256k1::pubkey_negate` returns a `public_key` where its six neighbours of
   `tweak.hpp` return `result<>`. (`ecdh_x` was named with it; it already returns
   `result<secret<32>>`, as `ecdh` and `ecdh_point` do, since increment 2: this change pins
   that with a test and changes nothing else of it.)
6. The constant-time lane's dudect is printed and not judged on GitHub's hosted runners, since
   one run of 24 read |t| 69.8 on an AMD EPYC 7763 (2026-11-20), on a case below 4.4 in every
   other run, and its value is lost: b2 shows a test's output only when it fails. The research
   of 2026-11-20 found no code leak (the x86 code is the same for both classes) and no
   class-blind noise that reaches it (a simulation's maximum was 2.4); its hypothesis is a
   value effect that depends on memory layout, which only a real Zen 3 machine could settle.

## What Changes

- **`gcm::seal` and `gcm::update` zero `data` when the backend fails**, on every backend, as
  `aead::seal` does; a guard's refusal still leaves it untouched.
- **GCM opens in two passes, incrementally:** `begin_open(iv, additional_data)`,
  `authenticate(piece)` over every piece of the ciphertext, read only, then `verify(tag)`; only
  after the tag matched, `decrypt(piece)` over the same pieces in place and `end_open()`. No
  byte of plaintext reaches the caller before the tag is verified. The trade-off is stated: the
  ciphertext is read twice, so it must not change between the passes, which only the caller
  can ensure; a stream that cannot be read twice is still sealed in records.
- **The AES block cipher gets a default per target** (BREAKING for a program that named
  `aes256<>` alone on WebAssembly, which did not compile, and for code that compared the types
  `aes256<>` and `aes256<backends::openssl>`): OpenSSL natively, ctaes on wasip2, wasip3 and
  Emscripten. `aes128<>`, `aes192<>` and `aes256<>` name it; `gcm<aes256<>>`,
  `aes128_gcm` and `aes256_gcm` keep AES-GCM's default, through a default-backend tag that each
  family resolves for itself.
- **`curves::secp256k1::basic_keypair::generate(memory_resource)`**, drawing the secret key
  from `random_device` and drawing again for the rare 32 bytes that are not a secret key.
- **`pubkey_negate` returns `result<public_key>`** (BREAKING), `backend_failure` should
  libsecp256k1 refuse a parsed key.
- **The dudect harness:** each case prints a line a script reads, with evidence when |t| >= 10
  (each class's mean and count at the cropping that gave the largest |t|, the CPU's model and
  microcode, the clock's resolution, the low 12 bits of the measured buffers' addresses); each
  program measures an A/A control case; each takes `--case <name>` to measure one case alone.
  The CI prints every dudect output, warns on GitHub for every |t| >= 10, and on hosted runners
  measures a case over 10 again in two fresh processes, failing only when two of three exceed
  10: dudect is judged again on hosted runners.

Every new public function holds `detail::timing_guard` as its first statement and is called by
the timing coverage test of its backend (the change `data-independent-timing`).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crypto`: GCM's failed seal zeroes its data and GCM gains a two-pass incremental open; the
  AES block cipher gets a default per target; secp256k1's key pair is generated; `pubkey_negate`
  returns a result; the constant-time lane's dudect is judged on hosted runners by two of three
  fresh measurements, with an A/A control and the evidence of every high value.

## Impact

- **Kind:** a change to a wrapper (`"port-of": null`), as the genesis and the foundation.
- **Implementation:** not started. `reference/plan.md` is the plan, written on 2026-11-20 on
  the library's tree with the change `data-independent-timing` (the guard, its seam and the
  coverage tests per backend), which this change requires merged first.
- **Status:** accepted on 2026-11-20 (the owner approved the six items that day and rules the
  rest on autopilot; the rulings are `design.md`'s); nothing built.
- **Dependencies:** none new. The CI's judge is Python 3, as the superproject's tools.
- **Other proposals:** it modifies requirements of `../genesis` (the block cipher's default,
  GCM's incremental open) and of `../foundation` (the defaults); the foundation's asynchronous
  family (its increment 4, part A, written the same day) is built after it, on the types this
  change leaves. The superproject's
  `.github/workflows/library.yml` gains one step for crypto's constant-time lane.
