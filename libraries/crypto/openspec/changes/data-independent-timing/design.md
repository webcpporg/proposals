## Context

The constant-time lane (`libs/crypto/test/constant_time/`) sets DIT in each dudect program
(`dudect.hpp`, `enter_data_independent_timing`), and `doc/backends.adoc` says that neither the
C libraries nor webcpp set it. The public functions reach the backends two ways: the families
of `ciphers/`, `hybrid/` and `curves/{ed25519,x25519}` through `detail::*_engine`
specialisations, and `curves/secp256k1*` by calling libsecp256k1 directly. The owner decided on
2026-11-20 that the library sets DIT itself (the owner's option 2 of that day; option 1 was to
document it and leave it to programs).

## Goals / Non-Goals

**Goals:** every cryptographic computation of webcpp::crypto runs with DIT set where the CPU
has it, with no call in the program; the thread's state is as the program left it after every
call; a test fails when a public function forgets the guard; dudect, run natively on Apple
silicon without the harness setting DIT, passes where it failed before.

**Non-goals:** webcpp::hash; Windows on ARM64; x86's DOITM (a kernel setting, not a user
one); WebAssembly, where the engine decides; a public guard type for programs (Apple's
`timingsafe_enable_if_supported` serves a program's own code).

**Anti-goals:** leaving DIT set in the program's thread after a call returns; a call that
traps (SIGILL) on an arm64 CPU without DIT; a guard in one backend's code and not another's.

## Approaches

1. **A guard in each public function, in the families' headers (chosen).** One place per
   operation, the same for every backend, the direct libsecp256k1 calls included; the coverage
   test reaches each one through the public interface. Cost: one read of the system register,
   and where DIT is not set a write, an `isb` and a write back, per call.
2. **A guard in each backend's engine specialisation.** Closer to the C call, but five times the
   sites, and secp256k1's functions have no engine; a new backend could forget it with no test
   noticing.
3. **Set DIT once per thread and never clear it.** Cheapest, but it changes the program's thread
   behind its back, which Apple's guidance and the anti-goal above rule out.

## Decisions

### D1. Where the guard applies

Every public function (member or free) whose body calls a backend's cryptographic code holds a
`detail::timing_guard` for its whole body, first statement. Verification, public-key
derivation and parsing of keys through a backend are included: the rule is mechanical, and the
cost of a guard is small against any of these. `equal`, webcpp's own constant-time code, holds
one too. Not included: `random_device` (the system's entropy call, not a computation on a
secret), constructors and destructors that only move or wipe bytes, `supports`, and anything
`constexpr`.

### D2. The guard

`detail/timing.hpp`: `class timing_guard` with a `noexcept` default constructor and destructor,
non-copyable, non-movable. Construction calls `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED()`, then,
on `__aarch64__` with `__APPLE__` or `__linux__`: if `detail::cpu_has_dit()` (a function-local
static, read once per process: `sysctlbyname("hw.optional.arm.FEAT_DIT")` on Apple, which
counts an unreadable value as absent; `getauxval(AT_HWCAP) & HWCAP_DIT` (bit 24) on Linux),
reads PSTATE.DIT (`mrs s3_3_c4_c2_5`); when clear, writes it set (`msr s3_3_c4_c2_5`) followed
by `isb`, and remembers to clear it. The destructor clears it only then, also followed by
`isb`. The register is named by its encoding so every assembler takes it. Everywhere else the
class is empty and its members do nothing. The harness's `dudect.hpp` keeps only its reporting
of what the CPU offers, built on `detail::cpu_has_dit()` and a `detail::dit_is_set()` read.

### D3. The seam

`WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED()` expands to nothing unless defined before the first
include, like `WEBCPP_CRYPTO_DETAIL_DECLASSIFY`. The coverage test defines it to count entries
in a thread-local counter and calls every public function of every family on every backend the
target builds, checking that each call raised the count. It runs on every lane, x86 and
WebAssembly included, because the seam fires where DIT does not apply.

### D4. Tests of the guard

On arm64 macOS and Linux: the bit is set inside a guard; cleared after it when it was clear
before; still set after it when it was set before (the guard leaves it); nested guards; a
public operation leaves the thread as it found it in both states. Elsewhere the test checks that
the guard compiles and the seam fires.

### D5. The constant-time lane

The dudect programs stop calling `enter_data_independent_timing`; they print whether the CPU has
DIT and judge as before. `dudect_planted` still must find its leak (DIT does not hide an
early-exit scan). The wasip2 finding stays open and unjudged: the WebAssembly cannot set DIT.

### D6. Measurements

The cost of a guard is measured in a release build on the owner's Apple M5 Max (a call that
sets and clears, a call where it is already set) and stated in the documentation; the
throughput tables are regenerated with the library's own tool from a release build, never
edited by hand.

### D7. Windows on ARM64

Out of this change: no documented way to ask Windows whether the CPU has FEAT_DIT was found, and
there is no machine to test on. The guard is empty there; the documentation says so.

## Risks / Trade-offs

- A per-call `isb` slows the shortest calls (a 16-byte AES block): measured (D6) and published.
- A new public function without a guard: the coverage test fails, as long as the test is
  extended with the function, which the review checklist of every later increment carries.
- A kernel that does not preserve DIT across a context switch: Linux and Darwin keep PSTATE in
  the saved state, so the bit follows the thread.

## As built (2026-11-20)

The change was built on the branch `crypto/dit` of the library and the superproject, by the plan
`reference/plan.md`, whose rulings R1 to R10 stand. What differs from the decisions above, or
adds to them:

- **D3, the seam is strict.** The seam is `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED(where)`, and
  where a test defines it, `timing_guard` takes the `std::source_location` of its declaration;
  without it the constructor takes no argument, since clang on arm64 does not inline the
  constructor in release and kept every guarded function's signature and header path in the
  program (test `timing_unseamed`, which reads its own release executable natively). The coverage test
  records the function of the first guard a call enters and fails, naming the check, when no
  guard fires or when that first guard is not the function's own: a count alone passed a
  forwarder without its guard whenever its callee held one (the free `pem::unarmour`, `generate`
  calling `make`). Each of these deleted guards was shown to fail the test, naming it.
- **D1, `gcm`'s moves hold the guard** (ruling R3): on BearSSL a move runs `br_gcm_init`, an AES
  encryption under the key, and abandoning a message runs the cipher on both BearSSL and
  OpenSSL.
- **D5, the planted leak runs inside the guard,** so that `dudect_planted` shows that DIT does
  not hide an early-exit scan: |t| 66.72 and 65.53 (Apple M5 Max, 2026-11-21 UTC).
- **D5, what the lane measured** (Apple M5 Max, natively, release, 2026-11-21 UTC): libsodium's
  X25519 agreement with the library's guard alone |t| 1.78 to 2.82 over five runs; without DIT
  (the library before the change, the harness's call removed) 4.34 to 7.78 over three runs,
  below the threshold that day where the record of 2026-11-19 read 6.7 to 14.5; with the old
  harness setting DIT 1.48 to 2.40. On wasip2 the case read 25.25, printed and not judged: the
  finding stays open.
- **D6, the cost of a guard** is measured by `bench/timing.cpp` and rendered by
  `bench/collect.py --timing`: 40.3 ns per call where DIT was clear, 0.7 ns where it was set
  (Apple M5 Max, release). **Open item:** these numbers and the throughput tables regenerated
  with them were measured on a loaded machine; the page says so, and they are measured again in
  a final measurement pass on a quiet machine after the streams of 2026-11-20 merge. That pass
  also: (i) measures the guard again after its constructor stopped taking a
  `std::source_location` outside tests (the 40.3 ns include an out-of-line call and its
  argument); (ii) writes UTC dates (`doc/performance.json` holds local ones, 2026-11-20 for
  measurements made on 2026-11-21 UTC); (iii) regenerates the release size matrix
  (`doc/tables/sizes.json`, `generate.py --sizes`), which the guard changes on native arm64; and
  (iv) tests which barrier each DIT write needs: Apple's guide ("Writing ARM64 code for Apple
  platforms", "Enable DIT for constant-time cryptographic operations") asks for `sb` after
  turning DIT on (`dsb nsh` then `isb sy` without FEAT_SB) and for none after turning it off,
  while the guard executes `isb` after both; drop the exit `isb`, then both, try Apple's
  barrier, and check dudect and the cost of each.
