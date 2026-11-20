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
