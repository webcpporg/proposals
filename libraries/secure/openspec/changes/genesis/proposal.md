## Why

Three webcpp libraries handle secrets, and each either holds its own copy of the same small
primitives or would have to reach into another library's internals to get them:

- webcpp::crypto holds the DIT guard (`include/webcpp/crypto/detail/timing.hpp`), the volatile wipe
  (`detail/wipe.hpp`), the public `crypto::erase_secret`, `crypto::constant_time_equal` and
  `crypto::secret<N>`, and the constant-time lane's declassify marker (`detail/declassify.hpp`).
- webcpp::hash holds a second copy of the guard (`include/webcpp/hash/detail/timing.hpp`, with an
  `engaged` flag) and a second wipe (`detail/wipe.hpp`), because it depends on no other webcpp
  library and crypto's `detail` is not its to include; a test of crypto keeps the two guards in step.
- webcpp::codecs, whose genesis the owner approved on 2026-11-24, needs the guard, a wipe, a secret
  type and constant-time selection and lookup for its secret encodings, and would have made a third
  copy of each.

config's change `timing-guard` (2026-11-24) moved the guard alone into webcpp::config. It was built
and verified natively, the Windows on ARM64 branch once on the owner's Windows 11 ARM64 VM, then
withdrawn before any push: on 2026-11-24 the owner decided instead that these primitives are a
library of their own, webcpp::secure, and that config stays macros only.

## What Changes

- **New library `webcpp::secure`**, of webcpp's own design, header-only C++20,
  `/webcpp/secure//secure`, repository `webcpporg/secure` (the owner creates it), submodule
  `libs/secure`, BSL-1.0. It holds no cryptographic algorithm: only the side-channel and
  secret-in-memory primitives that crypto, hash and codecs share. It depends on Boost's headers and
  `/webcpp/config//config` (the platform macros) alone. Every name is in `webcpp::secure` and every
  header under `<webcpp/secure/...>` (the owner's rule: the top-level namespace and include root are
  for libraries with a repository of their own).
- **`<webcpp/secure/timing_guard.hpp>`**, public, since it is the library's purpose:
  `secure::timing_guard` (RAII, `explicit timing_guard(bool engaged = true)`, not copyable or
  movable), `secure::cpu_has_dit()`, `secure::dit_is_set()`, the macro
  `WEBCPP_SECURE_TIMING_COVERED` where the guard acts, and the test seam
  `WEBCPP_SECURE_TIMING_ENTERED(where)`. It sets arm64's DIT for its scope when the CPU has FEAT_DIT
  and DIT is clear, and clears it on exit only when it set it, so guards nest across libraries and
  a program that set DIT keeps it. Covered: arm64 under Apple's systems, Linux and Android, and
  Windows (`_M_ARM64`, not ARM64EC, through `_ReadStatusReg`, `_WriteStatusReg` and `__isb`, with
  FEAT_DIT probed once under structured exception handling); an empty class elsewhere.
- **`<webcpp/secure/erase_secret.hpp>`**: `secure::erase_secret(range)`, moved from crypto with its
  name and contract, over secure's volatile wipe.
- **`<webcpp/secure/constant_time_equal.hpp>`**: `secure::constant_time_equal(first, second)`,
  moved from crypto with its name, now over any two contiguous ranges of bytes (a span, an array, a
  `secret<N>`, a Boost.Hash2 digest) so that secure names no Boost.Hash2 type; it holds the guard.
- **`<webcpp/secure/secret.hpp>`**: `secure::secret<N>`, moved from crypto unchanged in behaviour:
  `N` owned bytes, wiped on destruction and on move, copied only by `clone()`, viewed through
  `bytes()`, `writable()` and an lvalue's implicit span.
- **`<webcpp/secure/declassify.hpp>`**: `secure::declassify(object)`, the marker library code
  calls on a value derived from secrets that it branches on by design (a tag's verdict): nothing in
  a program's build, memcheck's `VALGRIND_MAKE_MEM_DEFINED` when a constant-time test build defines
  `WEBCPP_SECURE_CT_VALGRIND`, MemorySanitizer's `__msan_unpoison` under `WEBCPP_SECURE_CT_MSAN`.
  The marking of secrets and the reading of the checkers' shadow, which only tests do, are test
  support, not public.
- **`<webcpp/secure/constant_time.hpp>`**, namespace `secure::constant_time`: the building blocks
  codecs needs, as primitives: `value_barrier`, masks from a bit and from comparisons of unsigned
  values (`mask_from_bit`, `is_zero_mask`, `is_equal_mask`, `is_less_mask`,
  `is_in_range_mask`), `select` by mask, `select_bytes` (a conditional copy), `lookup` (a table
  read whole, the entry chosen by masks), and comparisons of byte sequences returning masks
  (`equal_mask`, `is_zero_mask` of bytes, `less_mask`, big-endian).
- **`<webcpp/secure.hpp>`**, the convenience header, and `<webcpp/secure/config.hpp>`.
- **Tests**: the guard's (`timing_guard_test`, `_nesting_`, `_seam_`, `_threads_`, `_windows_`,
  header-alone), crypto's `erase_secret_test`, `constant_time_equal_test` and
  `declassify_compile_test` moved, `secret_test`, the constant-time blocks against a plain
  reference (every 8-bit pair, random wider ones), the allocation test, and the own lane
  `constant-time` (memcheck, MemorySanitizer, dudect, each with a planted leak that must fail). The
  seam recorder, the DIT helper and the checkers' marks live in secure's own `test/support/`, and
  hash and crypto keep or gain their own copies in theirs (design O1, decided 2026-11-24).
- **Documentation**: the library's page, the guard's coverage table, how to erase, the secret type,
  the constant-time blocks with their limits, and the changelog.

Not in this change: webcpp::codecs' genesis, which uses secure from its first line (amended the
same day). The migrations of hash and crypto, which this paragraph first named as their own
follow-up changes, the owner folded into the same delivery on 2026-11-24, after secure's own tasks
(tasks.md sections 7 and 8): webcpp::hash's guard and wipe move to secure and hash's copies are
deleted (hash then depends on secure), then webcpp::crypto's every caller moves to secure and
`crypto::erase_secret`, `crypto::constant_time_equal`, `crypto::secret`, `detail/timing.hpp`,
`detail/wipe.hpp` and `detail/declassify.hpp` are deleted with no alias.

## Capabilities

### New Capabilities

- `secure`: webcpp::secure, the DIT timing guard, `erase_secret`, `constant_time_equal`,
  `secret<N>`, the declassify marker, the constant-time building blocks, with their build, tests,
  lanes and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** own (`"port-of": null`): webcpp's own design, moved from webcpp::crypto and
  webcpp::hash and from config's withdrawn change `timing-guard`; the constant-time blocks follow
  BoringSSL's `constant_time_*` and libsodium's arithmetic. Repository `webcpporg/secure` (not
  created; the owner creates it), checked out as the submodule `libs/secure`, BSL-1.0, "Copyright
  (c) 2026 WebCpp.org".
- **Implementation:** nothing is built in secure's name. The prior code is in `reference/prior/`:
  the guard as config's withdrawn branch built it (Windows on ARM64 included) with its tests, the
  shared seam of that branch, and crypto's headers and tests that move. To build: the library's
  repository, `build.jam`, `include/webcpp/secure.hpp`, `include/webcpp/secure/*.hpp`, tests, the
  constant-time lane, `doc/`, the superproject's pointer, `CODING-GUIDELINES.md` chapter 17's entry
  and the index page; then hash and crypto moved onto it.
- **Dependencies:** Boost 1.92's headers (Core's `lightweight_test` in tests) and
  `/webcpp/config//config`. For the constant-time lane: valgrind, MemorySanitizer and dudect as
  webcpp::crypto's lane uses them. secure depends on no other webcpp library; hash, crypto and
  codecs depend on it.
- **Status:** approved by the owner on 2026-11-24 (`decided`), and `accepted` the same day once
  `reference/plan.md` carried the plan's code, which `tools/proposals.py` requires of an
  `accepted` proposal.
- **Other proposals:** replaces `libraries/config`'s change `timing-guard`, withdrawn the same day
  (archived as `2026-11-24-timing-guard`). `libraries/codecs`'s genesis uses secure's guard, erase,
  secret and constant-time blocks (its owner decision 14). hash's and crypto's migrations, which
  the genesis first named follow-up changes, are part of this delivery by the owner's decision of
  2026-11-24 (tasks.md sections 7 and 8): `libraries/hash` (its guard and wipe on secure's) and
  `libraries/crypto` (every caller on secure's names, its copies deleted with no alias, its
  `data-independent-timing` requirements read with secure's guard and seam).
