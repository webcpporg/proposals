## Why

On 64-bit ARM, the instructions Arm lists take a time independent of their data only while the
thread's DIT bit (PSTATE.DIT, data-independent timing) is set, and Apple requires it of
constant-time code on Apple silicon. None of the C libraries webcpp::crypto wraps sets it, and
neither does webcpp: today a program on Apple silicon must set it around its secret operations
itself, and almost none will. The constant-time lane measured the cost of leaving it to them:
libsodium's X25519 agreement on an Apple M5 Max, 2026-11-19, |t| 6.7 to 14.5 against dudect's
threshold of 10 without DIT, 1 to 3.5 with it; the lane's harness sets DIT itself so that its
other measurements mean something.

On 2026-11-20 the owner decided that webcpp::crypto sets DIT itself around its secret operations
and restores the thread's previous state afterwards: secure by default, with nothing for a
program to do.

## What Changes

- **A guard, `detail::timing_guard`**, an RAII type in `detail/timing.hpp`: on macOS and Linux
  on arm64 (iOS and Android included, by the same system calls), where the CPU has DIT, it sets
  the bit when the thread does not have it and clears it again on destruction; where the bit is
  already set it does nothing. Elsewhere it compiles to nothing.
- **Every public function that runs a backend's cryptographic computation holds a guard** for
  its whole body: every family (the AES block cipher, GCM, the AEADs, secretbox, box and sealed
  boxes, Ed25519, X25519 and kx, secp256k1 with its groups, the key formats and PEM), on every
  backend. Verification and public-key operations are included, so that the rule needs no
  judgment per function.
- **A test seam, `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED(where)`**, empty unless a test defines it,
  through which a test proves that every such function enters the guard, on every target.
- **The constant-time lane no longer sets DIT**: its dudect programs measure what a program gets
  from the library, and print what the CPU offers.
- **The documentation** says what the library sets, where, and what a program on a target
  without it (x86, WebAssembly, Windows on ARM64) gets.

Not changed: webcpp::hash (its keyed functions are a follow-up the owner has not decided), the
public interface, the errors, any backend's library.

## Impact

`libs/crypto/include/webcpp/crypto/` (the new header and the families' headers),
`libs/crypto/test/` (the guard's tests, the coverage test, the constant-time lane),
`libs/crypto/doc/`, the throughput tables regenerated from a release build.
