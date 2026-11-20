## ADDED Requirements

### Requirement: Timing: the library sets DIT around its cryptographic computation
On 64-bit ARM under macOS, iOS, Linux and Android, where the CPU has FEAT_DIT, every public function of webcpp::crypto whose body runs a backend's cryptographic code, and `equal`, SHALL run its whole body with PSTATE.DIT set, through `detail::timing_guard`, and MUST return with the thread's DIT as it was before the call. Where the CPU has no DIT, or on any other target, the guard SHALL do nothing and MUST NOT execute an instruction that traps.

#### Scenario: A program that never set DIT signs on Apple silicon
- **WHEN** `curves::ed25519::keypair::sign` runs on a thread whose DIT is clear
- **THEN** DIT is set while libsodium signs, and clear again when `sign` returns

#### Scenario: A program that set DIT itself
- **WHEN** a public operation runs on a thread whose DIT is already set
- **THEN** it is still set when the operation returns

### Requirement: Timing: every guarded function is tested to enter the guard
A seam `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED()`, empty unless a test defines it, SHALL fire on every guard's construction, and a test MUST call every public function the first requirement names, on every backend the target builds, and fail when one does not fire it. The test SHALL run on every lane.

#### Scenario: A new function without a guard
- **WHEN** a public function is added that calls a backend and holds no guard, and the coverage test calls it
- **THEN** the test fails, naming the function

### Requirement: Timing: the constant-time lane measures what a program gets
The dudect programs SHALL NOT set DIT themselves; they MUST print whether the CPU has it, and on Apple silicon natively libsodium's X25519 agreement MUST pass the threshold with the library's guard alone, while `dudect_planted` still fails.

#### Scenario: X25519 on an Apple M5 Max
- **WHEN** the lane runs natively in release on Apple silicon
- **THEN** X25519's agreement passes dudect without the harness setting DIT
