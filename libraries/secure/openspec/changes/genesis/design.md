## Context

The primitives this library gathers exist today in three places, two of them copies:

- webcpp::crypto (`libs/crypto`, `main` at `3314a1d`):
  - `include/webcpp/crypto/detail/timing.hpp`: `detail::timing_guard`, `cpu_has_dit()`,
    `dit_is_set()`, `WEBCPP_CRYPTO_DETAIL_TIMING_COVERED` on arm64 under Apple's systems and Linux,
    the seam `WEBCPP_CRYPTO_DETAIL_TIMING_ENTERED(where)` taking a `std::source_location`; held as
    the first statement of every public function that runs a backend's code (187 sites in 26
    headers).
  - `include/webcpp/crypto/detail/wipe.hpp`: `detail::wipe(void*, std::size_t)` (volatile stores,
    then a signal fence) and `detail::wipe_object(T&)` for a C library's plain state.
  - `include/webcpp/crypto/erase_secret.hpp`: the public `crypto::erase_secret(range)`, a
    contiguous sized range of writable, trivially copyable elements; no pointer and size form.
  - `include/webcpp/crypto/constant_time_equal.hpp`: the public `crypto::constant_time_equal` over
    two `std::span<std::uint8_t const>` and over two `boost::hash2::digest<N>`, BearSSL's
    accumulate-then-reduce, under the guard.
  - `include/webcpp/crypto/secret.hpp`: the public `crypto::secret<N>` (design D9 of crypto).
  - `include/webcpp/crypto/detail/declassify.hpp`: `WEBCPP_CRYPTO_DETAIL_DECLASSIFY(address,
    size)`, empty in a program's build, memcheck's or MemorySanitizer's under
    `WEBCPP_CRYPTO_DETAIL_CT_VALGRIND` or `WEBCPP_CRYPTO_DETAIL_CT_MSAN`, called at nine verdicts
    (BearSSL's and libsodium's tag checks, the P-curves' scalar checks, X25519's all-zero check,
    silent payments' label). The marking of secrets lives in the tests
    (`test/constant_time/ctgrind_test.cpp`'s `memcheck` and `msan_test.cpp`'s primitives:
    `mark_secret`, `mark_public`, `traced`, `verdict`).
- webcpp::hash (`libs/hash`): `include/webcpp/hash/detail/timing.hpp`, the same guard with
  `WEBCPP_HASH_*` names and `timing_guard(bool engaged = true)`, a disengaged guard doing nothing,
  for functions keyed only by their arguments (28 sites); and its own `detail/wipe.hpp`.
- config's change `timing-guard`, withdrawn on 2026-11-24 before any push (its branch is local:
  config `ba72ad4`, hash `43b6732`, crypto `b0c1f4f`, superproject `83c9a31`). It built one guard
  with hash's `engaged` flag, added Windows on ARM64, moved crypto and hash onto it, and put a
  shared seam recorder (`<webcpp/test/timing_seam.hpp>`) and DIT helper
  (`<webcpp/test/timing_dit.hpp>`) in the superproject's `tools/node-test`. Verified: clang over
  config (27 tests), hash (295) and crypto (541), 0 failed; the Windows branch once on the owner's
  Windows 11 ARM64 VM, MSVC 14.3 and 14.5, arm64 and x64, debug builds, `cpu_has_dit()` true and
  DIT kept across a sleep. Not verified: a release build on the VM, clang-cl ARM64, a CPU without
  FEAT_DIT (the probe's filter path), crypto's dudect on Windows ARM64. `reference/prior/` holds
  the guard, its tests, the shared seam and crypto's headers and tests that move.
- webcpp::codecs' genesis (approved 2026-11-24) needs the guard (engaged for secret operations
  only), a wipe, fixed secret storage and constant-time arithmetic on masks for hex, base32,
  base64, bech32, the armor and secret base58.

The prior art for the blocks: BoringSSL's `crypto/internal.h` (`constant_time_msb_w`,
`constant_time_is_zero_w`, `constant_time_eq_w`, `constant_time_lt_w`, `constant_time_select_w`,
`value_barrier_w`, `CONSTTIME_SECRET` and `CONSTTIME_DECLASSIFY`), libsodium's codecs
(`sodium_bin2hex`, `sodium_bin2base64`: alphabets by range masks), BearSSL's `inner.h` (`EQ`,
`NEQ`, `GT`, `MUX`, `CCOPY`).

## Goals / Non-Goals

**Goals:** one home for the side-channel and secret-in-memory primitives every webcpp library
shares; the guard as the withdrawn change built it, Windows on ARM64 included, public; crypto's
erase, comparison and secret type moved with their names and contracts; the declassify marker a
library may call; the constant-time blocks codecs needs, small, tested exhaustively where the
domain allows, and checked on a constant-time lane; every target webcpp builds for.

**Non-goals:** any cryptographic algorithm (hashes, ciphers, curves stay in hash and crypto); a
wiping allocator or heap secret types (codecs' `secret_string` and `secret_byte_vector` are codecs'
own, over secure's erase); memory locking (`mlock`, `VirtualLock`) or guard pages; x86's DOITM (a
kernel setting); WebAssembly's timing (the engine decides); codecs' use of secure, which is
codecs' genesis's.

**Anti-goals:** a cryptographic algorithm in secure; a dependency on any webcpp library but config;
a name outside `webcpp::secure` or a header outside `<webcpp/secure/...>` and `<webcpp/secure.hpp>`;
an alias left in crypto or hash for a name that moved; a second copy of any primitive anywhere in
webcpp.

## Owner decisions (2026-11-24, binding)

1. **A new library, webcpp::secure,** the side-channel and secret-in-memory primitives shared by
   crypto, hash and codecs, with no cryptographic algorithm; its own repository `webcpporg/secure`.
2. **Dependencies:** webcpp::config alone (its platform macros). config stays macros only: its
   change `timing-guard` is withdrawn.
3. **Names:** the top-level `webcpp` namespace and the `<webcpp/...>` include root are for
   libraries with a repository of their own, so everything is under `webcpp::secure` and
   `<webcpp/secure/...>`.
4. **The guard:** `timing_guard` (DIT on arm64: Apple's systems, Linux and Android, Windows
   `_M_ARM64` through `_ReadStatusReg` and `_WriteStatusReg` with `isb`, FEAT_DIT probed once under
   structured exception handling; empty elsewhere), `cpu_has_dit()`, `dit_is_set()`, the `engaged`
   flag, the nesting rules and the test seam, with `WEBCPP_SECURE_*` macros; a public API of
   `webcpp::secure`, not `detail`, since it is the library's purpose.
5. **Moved from crypto with their names:** `erase_secret`, `constant_time_equal` and `secret<N>`;
   crypto's go away with no alias, and a follow-up change of crypto moves its callers.
6. **The declassify and constant-time checkers' markers** crypto uses on its constant-time lane,
   their public part in secure and the rest as test support.
7. **The constant-time building blocks codecs needs** (select, a table lookup by masking,
   constant-time comparisons on bytes), as primitives.
8. **Test support:** secure's own seam and DIT helpers live in secure's `test/support/`; the
   helpers shared by several libraries' tests stay in each library's own `test/support/` (O1,
   decided 2026-11-24).
9. **The migrations are in this delivery** (2026-11-24): hash's guard and wipe, then crypto's every
   caller, move to secure after secure's own tasks, hash first since crypto depends on hash; the
   genesis had named them follow-up changes.

## Decisions

### D1. The library, its target and its headers

Namespace `webcpp::secure`, with `secure::constant_time` and `secure::detail`; macros
`WEBCPP_SECURE_*`; include guards `WEBCPP_SECURE_<PATH>_HPP`. `<webcpp/secure/config.hpp>`,
included first by every public header, includes `<webcpp/config.hpp>` first and defines
`WEBCPP_SECURE_NO_EXCEPTIONS` by chapter 6's rule. No function of the library throws, and every
function is `noexcept` (the guard's destructor is implicitly).

```
include/webcpp/secure.hpp                       the convenience header: every header below
include/webcpp/secure/config.hpp
include/webcpp/secure/timing_guard.hpp          timing_guard, cpu_has_dit, dit_is_set (D2, D3)
include/webcpp/secure/erase_secret.hpp          erase_secret (D4)
include/webcpp/secure/constant_time_equal.hpp   constant_time_equal (D5)
include/webcpp/secure/secret.hpp                secret<N> (D6)
include/webcpp/secure/declassify.hpp            declassify (D7)
include/webcpp/secure/constant_time.hpp         constant_time:: blocks (D8)
include/webcpp/secure/detail/wipe.hpp           the volatile wipe
```

`build.jam` declares one target, `secure`: `<include>include`, `/boost//headers`,
`/webcpp/config//config`, nothing compiled, nothing linked (the Apple, Linux and Windows calls are
the system's C library and compiler intrinsics). `meta/libraries.json` names it with
`"port-of": null`, category "Cryptography". The library's name is none config reserves (`target`,
`has`, `no`, `lib`, `version`, `test`).

### D2. The guard

```cpp
namespace webcpp::secure {
[[nodiscard]] bool cpu_has_dit() noexcept;   // FEAT_DIT, read once per process; false where not covered
[[nodiscard]] bool dit_is_set() noexcept;    // PSTATE.DIT of this thread; false where not covered
class timing_guard {                         // not copyable, not movable
public:
    explicit timing_guard(bool engaged = true) noexcept;  // + std::source_location under the seam
    ~timing_guard();
};
}
// WEBCPP_SECURE_TIMING_COVERED: defined where the guard acts.
// WEBCPP_SECURE_TIMING_ENTERED(where): the seam, defined by a test alone before its first include.
```

Behaviour, the two copies' and the withdrawn change's: an engaged guard runs the seam (in a test's
build), then sets DIT if the CPU has it and it is clear, and remembers to clear it; its destructor
clears DIT only if it set it. A disengaged guard does nothing, the seam included. So guards nest
in any order of libraries (crypto inside hash, hash inside crypto, codecs inside either): an inner
guard finds DIT set and leaves it, and a program that set DIT itself (Apple's
`timingsafe_enable_if_supported`) keeps it. DIT is a thread's state; a guard never changes another
thread's. Each write keeps its `memory` clobber (or the intrinsics' ordering on MSVC) and an `isb`
after it; which barrier each write needs (Apple asks for `sb` or `dsb nsh; isb sy` after setting)
stays crypto's open measurement and applies here when made.

The seam: without `WEBCPP_SECURE_TIMING_ENTERED` the constructor takes no source location, so no
function name or path reaches a program. With it, the constructor's last parameter is
`std::source_location where = std::source_location::current()`, evaluated where the guard is
declared, so `function_name()` is the guarded function (crypto's, hash's, codecs' or secure's own
`constant_time_equal`) and `file_name()` tells the library (`webcpp/crypto/`, `webcpp/hash/`,
`webcpp/codecs/`, `webcpp/secure/`, either separator).

Public because it is the library's purpose (owner decision 4): a program may hold one around its
own constant-time code. The page says what DIT covers (Arm's list of data-independent
instructions) and what it does not (memory access patterns, branches).

Rejected: a guard in `secure::detail` (another library may not include a `detail` of secure, and
three of them need it); separate engaged and disengaged types (hash chooses at run time from its
arguments).

### D3. Coverage by target, and FEAT_DIT on Windows

| Target | `WEBCPP_SECURE_TIMING_COVERED` | FEAT_DIT detection | Register access |
| --- | --- | --- | --- |
| arm64, Apple's systems | yes | `sysctlbyname("hw.optional.arm.FEAT_DIT")`, once | `mrs`/`msr s3_3_c4_c2_5`, `isb` |
| arm64, Linux and Android | yes | `getauxval(AT_HWCAP) & HWCAP_DIT` (bit 24), once | as above |
| arm64, Windows (`_MSC_VER`, `_M_ARM64`, not `_M_ARM64EC`), MSVC and clang-cl | yes | the probe below | `_ReadStatusReg`, `_WriteStatusReg` of `ARM64_SYSREG(3, 3, 4, 2, 5)`, `__isb(_ARM64_BARRIER_SY)` |
| x86, x86-64, ARM64EC, WebAssembly, anything else | no | none (`false`) | none; the guard is an empty class |

Windows documents no `IsProcessorFeaturePresent` flag for DIT and no user-mode read of
`ID_AA64PFR0_EL1`. The guard reads the DIT register once per process inside `__try` and
`__except`, in a function of its own holding no object that needs unwinding (C2712): on a CPU
without FEAT_DIT the read raises `STATUS_ILLEGAL_INSTRUCTION` (0xC000001D), which the filter takes
as absent; any other exception continues its search. The result is cached in a function-local
static. It needs `<excpt.h>` and `<intrin.h>`, not `<windows.h>`, and works with C++ exceptions on
or off. That read is the only DIT instruction executed where FEAT_DIT may be absent. Rejected: the
registry's per-processor `CP 4020` value (undocumented, needs advapi32); leaving Windows on ARM64
uncovered.

### D4. `erase_secret` and the wipe

`secure::erase_secret(Range&&)` is crypto's, moved: a contiguous sized range of non-const,
trivially copyable elements, `size() * sizeof(element)` bytes zeroed from the first element (a
`std::string`'s size, not its capacity), no pointer and size form, and a refused range (const or
not trivially copyable elements) stops the build with a message naming the ranges it takes, now
spelled `webcpp::secure:`. It calls `secure::detail::wipe(void*, std::size_t)`: stores through a
volatile pointer, then `std::atomic_signal_fence(std::memory_order_seq_cst)`.

A C library's plain state (BearSSL's contexts, which crypto wipes with `detail::wipe_object`) is
erased as a range of one: `erase_secret(std::span(&context, 1))`. Builder's call B1: no second
public name for an object, since a span of one says the same and keeps the contract one function.

The wipe has a test seam, hash's `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD` moved (builder's call B9):
a program built with `WEBCPP_SECURE_TEST_WIPE_RECORD` defined (by its Jamfile, so that every
translation unit agrees) defines `void webcpp::secure::test::wipe_record(void const*, std::size_t)
noexcept`, which `detail::wipe` declares under the macro and calls once its bytes are zero, with
what it zeroed; without the macro nothing is declared or called. hash's tests that check a wipe
(of a keyed hmac, a block, a C library's context) read it, as they read hash's own seam today.

### D5. `constant_time_equal`

`secure::constant_time_equal(first, second)` takes any two contiguous sized ranges whose elements
are one byte (`std::uint8_t`, `unsigned char`, `char`, `std::byte`), so that a `std::span`, a
`std::array`, a `secret<N>`, a `std::string_view` and a Boost.Hash2 `digest<N>` all work and secure
includes no Boost.Hash2 header. Sizes are public: different sizes compare unequal without reading
the contents. Otherwise every byte is read, the XOR of each pair accumulated with OR in a volatile
local, read once after the loop and reduced to one bit without a branch (BearSSL's tag check, as
crypto's). It holds an engaged guard. Builder's call B2: one generic function over crypto's two
overloads, since crypto's digest overload named Boost.Hash2.

### D6. `secret<N>`

`secure::secret<N>` is crypto's `secret<N>`, moved with its interface: `N > 0`; zero-initialised;
move-only, the source wiped on move and the target wiped before it takes new bytes, a self-move
changing nothing; `clone()` the only copy; `bytes()` and the implicit conversion to
`std::span<std::uint8_t const, N>` on an lvalue, deleted on an rvalue so no view dangles;
`writable()` on an lvalue; `size()` and `size_bytes`; wiped on destruction through
`detail::wipe`. The page keeps crypto's warning: the wipe covers the object's own storage, not
copies a move or a `clone` leaves in registers or temporaries.

### D7. `declassify`, and the checkers' marks as test support

```cpp
namespace webcpp::secure {
template <class T>
    requires std::is_trivially_copyable_v<T> && (!std::is_const_v<T>)
void declassify(T& value) noexcept;
}
// WEBCPP_SECURE_CT_VALGRIND or WEBCPP_SECURE_CT_MSAN: defined by a constant-time test build alone.
```

Library code calls it on a value derived from secrets that it branches on by design, right before
the branch. In a program's build it does nothing and includes nothing. Under
`WEBCPP_SECURE_CT_VALGRIND` it is `VALGRIND_MAKE_MEM_DEFINED(&value, sizeof value)` (and
`<valgrind/memcheck.h>` is included); under `WEBCPP_SECURE_CT_MSAN` it is
`__msan_unpoison(&value, sizeof value)`; both at once stop the build. The value is never const, so
the branch after the mark reads it again rather than a register kept from before (BoringSSL's
practice, crypto's tip). Builder's call B3: a function template, not a macro, since it is a public
name of the library and needs no address and size from its caller.

The other half, marking a secret (`VALGRIND_MAKE_MEM_UNDEFINED`, `__msan_poison`), reading the
checker's shadow (`VALGRIND_GET_VBITS`) and checking a verdict, only a test does: it is test
support (D9), from crypto's `memcheck` and MemorySanitizer primitives. A constant-time build of
crypto or codecs defines `WEBCPP_SECURE_CT_VALGRIND` or `WEBCPP_SECURE_CT_MSAN`, so that their
verdicts and secure's are marked alike.

### D8. The constant-time building blocks

`<webcpp/secure/constant_time.hpp>`, namespace `webcpp::secure::constant_time`, every function
`constexpr` where the language allows, `noexcept`, branch-free and index-free on its data:

```cpp
namespace webcpp::secure::constant_time {
template <std::unsigned_integral U> U value_barrier(U value) noexcept;          // opaque to the optimiser
template <std::unsigned_integral U> U mask_from_bit(U bit) noexcept;            // 0 or 1 -> 0 or ~0
template <std::unsigned_integral U> U is_zero_mask(U value) noexcept;           // ~0 when value == 0
template <std::unsigned_integral U> U is_equal_mask(U first, U second) noexcept;
template <std::unsigned_integral U> U is_less_mask(U first, U second) noexcept; // unsigned
template <std::unsigned_integral U> U is_in_range_mask(U value, U low, U high) noexcept; // low <= value <= high
template <std::unsigned_integral U> U select(U mask, U when_set, U when_clear) noexcept;
template <std::unsigned_integral Element, std::size_t N>
Element lookup(std::span<Element const, N> table, std::size_t index) noexcept;  // reads every entry
void select_bytes(std::uint8_t mask, std::span<std::uint8_t> destination,
                  std::span<std::uint8_t const> when_set,
                  std::span<std::uint8_t const> when_clear) noexcept;            // equal sizes
std::uint8_t equal_mask(std::span<std::uint8_t const> first, std::span<std::uint8_t const> second) noexcept;
std::uint8_t is_zero_mask(std::span<std::uint8_t const> bytes) noexcept;
std::uint8_t less_mask(std::span<std::uint8_t const> first, std::span<std::uint8_t const> second) noexcept; // big-endian
}
```

- `std::unsigned_integral` excludes `bool`; a mask is all zeros or all ones of its type, never a
  `bool`, so that callers combine masks without converting to a branchable value. Only
  `constant_time_equal` returns a `bool`, after its reduction (D5), and a caller that must branch
  on a mask calls `declassify` first.
- `value_barrier` is an empty `asm volatile` with the value in a register on GCC and clang
  (BoringSSL's `value_barrier_w`) and a volatile round trip on MSVC; `mask_from_bit`, `select` and
  `lookup` pass their mask through it, so the optimiser cannot turn a mask back into a branch or a
  conditional move it may later turn into a branch.
- `lookup` reads every entry of the table in order and keeps the one whose index equals `index`
  by `is_equal_mask` and `select`; its cost is the table's size. An index past the table yields a
  value-initialised `Element` (no entry selected), so it never reads out of bounds. `Element` is
  an unsigned integral type.
- The byte comparisons read every byte of both sequences; their sizes are public and must be
  equal (a precondition, checked by `BOOST_ASSERT` in a debug build; unequal sizes give a zero
  mask without reading). `less_mask` compares as big-endian unsigned numbers (a scalar against a
  group order, a value against a bound), carrying the borrow through every byte.
- What codecs uses: `is_in_range_mask` and `select` compute an alphabet's character from a 4, 5 or
  6-bit value and back (libsodium's method), `lookup` serves base58's and bech32's alphabets and
  bech32's generator, `select_bytes` and `less_mask` serve secret base58's limb arithmetic, and
  the masks accumulate a decode's verdict.

Builder's call B4: these names, BoringSSL's operations spelled out (`is_less_mask` for
`constant_time_lt_w`), in a sub-namespace so that `secure::constant_time::select` reads as what it
is. Builder's call B5: no signed variants and no `msb`; callers that need them are none today.

### D9. Test support

secure's tests need a seam recorder (counting guards entered, keeping the first's
`function_name()`, filtering by `file_name()`), a helper that sets DIT by hand (`force_dit`, to
test "a program that set DIT keeps it") and the checkers' marks (D7). They live in secure's own
`test/support/`: `timing_seam.hpp`, `timing_dit.hpp`, `constant_time_marks.hpp`, in namespace
`webcpp::secure::test`, ported from the withdrawn branch's `tools/node-test` headers and crypto's
lane. crypto, hash and codecs need the same three for their own guards and lanes: each keeps or
gains its own copies in its own `test/support/` (O1, decided 2026-11-24): hash keeps its seam
recorder and DIT helper, crypto keeps its seam recorder and its checkers' marks and gains a DIT
helper, each recorder pointed at secure's seam and counting its own library's guards by
`file_name()`. secure's tests keep using `tools/node-test`'s `<webcpp/test/exit_status.hpp>` as
every library's do.

### D10. Tests

- **The guard:** `timing_guard_test` (DIT set inside, as found after; a guard that finds DIT set
  leaves it set; a disengaged guard does nothing; `cpu_has_dit()` stable),
  `timing_guard_nesting_test` (three levels, every order of engaged and disengaged),
  `timing_guard_seam_test` (the seam's `function_name()` and `file_name()`, `timing_guard{true}`,
  nothing fired by a disengaged guard, nothing named in a build without the seam),
  `timing_guard_threads_test` (another thread's DIT unchanged, natively where the build has
  threads), `timing_guard_windows_test` (D3, on the Windows ARM64 lane), each header compiled first
  and alone; on a target not covered, `std::is_empty_v<secure::timing_guard>` and both functions
  `false`. Ported from `reference/prior/config/test/`.
- **Moved from crypto:** `erase_secret_test` (every byte zeroed and none around, the refused ranges,
  a buffer erased before its scope ends reads zero at `-O2` with full inlining),
  `constant_time_equal_test` (equal, every single-byte difference, sizes, empty, a digest, a
  `secret<N>`, a `std::string_view`, the guard entered through the seam), `declassify_compile_test`
  (every header compiles with `WEBCPP_SECURE_CT_VALGRIND`, and with `WEBCPP_SECURE_CT_MSAN` under
  clang), and a `secret_test` from crypto's tests of `secret<N>` (moves, self-move, clone, deleted
  rvalue views, storage zero after destruction and after a move, read through placement storage).
- **The blocks:** every function against a plain reference, exhaustively over every pair (and
  triple for `is_in_range_mask`) of `std::uint8_t`, at random over `std::uint16_t`,
  `std::uint32_t` and `std::uint64_t` including their edges; `lookup` at every index and past the
  end; the byte comparisons over random sequences and their edges; `constexpr` evaluation where
  declared.
- **Allocation:** a counting `operator new` sees zero allocations in every function.
- **Constant time:** the own lane `constant-time` (`test/constant_time/`): memcheck with the
  operands marked undefined (ctgrind) over every block, `constant_time_equal` and `secret<N>`'s
  moves, MemorySanitizer where crypto runs it, dudect natively with its A/A control over
  `constant_time_equal`, `lookup`, `select_bytes` and `less_mask`, and a planted leak (a `select`
  written with a branch, a `lookup` that indexes) that must fail on each checker.

### D11. Lanes, documentation, measurements

Every test runs on native (clang and gcc 15), wasip2, wasip3 and Emscripten, each with its
`_no_exceptions` twin; MSVC 14.3 and 14.5 (x64 and arm64) and Apple clang as the other libraries;
the Windows ARM64 lane on the owner's VM for the guard (D3), in a release build too, which the
withdrawn change did not run; the own lane `constant-time`; the lint. The page (`doc/`): what
secure is and is not, the guard (coverage table, nesting, engaged, the seam), erasing, `secret<N>`,
`constant_time_equal`, `declassify`, the blocks with their limits (what DIT and masks cover and what
the compiler may still do), and the changelog. One measurement: the guard's cost on release builds
(engaged with DIT clear, engaged with DIT set, disengaged) on an idle machine, since the withdrawn
branch's figure was taken under load.

## Builder's calls

Settled while reading the genesis as a builder would; the plan may change them without returning
to the owner.

- **B1:** no public erase of an object; a span of one (D4).
- **B2:** `constant_time_equal` is one function over byte ranges, naming no Boost.Hash2 type (D5).
- **B3:** `declassify` is a function template, and its switches are `WEBCPP_SECURE_CT_VALGRIND`
  and `WEBCPP_SECURE_CT_MSAN` (D7).
- **B4:** the blocks' names and the sub-namespace `constant_time` (D8).
- **B5:** no signed blocks and no `msb` (D8).
- **B6:** one seam, the library told by `file_name()` (D2), as the withdrawn change's B1.
- **B7:** FEAT_DIT on Windows by an SEH-guarded read of the register (D3), as the withdrawn
  change's B2.
- **B8:** the convenience header `<webcpp/secure.hpp>` includes every header; none brings a heavy
  dependency (D1).
- **B9:** the wipe's test seam `WEBCPP_SECURE_TEST_WIPE_RECORD` and
  `secure::test::wipe_record`, hash's seam moved, so that hash's tests of its wipes keep their
  evidence once hash's `detail/wipe.hpp` is gone (D4).

## Risks / Trade-offs

- [hash and crypto keep their copies until their migrations land] -> both migrations are tasks of
  this delivery, right after secure's own; until each lands the copies behave alike, as today, and
  crypto's `timing_hash_interop_test` keeps them in step until hash's migration deletes it.
- [hash gains a dependency] -> secure is header-only and depends on config alone, the dependency
  hash already has; hash's "depends on no other webcpp library" becomes "on config and secure", in
  its migration.
- [The optimiser turns a mask into a branch] -> `value_barrier` on every mask a block makes, and
  the constant-time lane (memcheck and MemorySanitizer see a branch on a marked value) on every
  compiler that lane runs; dudect natively.
- [A Windows release changes the illegal-instruction behaviour] -> the probe fails closed (no DIT,
  an empty guard's effect); the Windows lane's test fails if a CPU known to have DIT reads absent.
- [The probe's absent path is untested] -> no machine without FEAT_DIT runs Windows on ARM64 here;
  the filter is small and reviewed, and the page says so.
- [`lookup` costs the table's size] -> codecs' tables are at most 64 entries; a caller with a
  larger public table indexes it directly.

## Migration Plan

Nothing to migrate in secure: it is new. The owner created `webcpporg/secure` and added the
submodule; the library is built on its own branch and the superproject's (pointer,
`CODING-GUIDELINES.md` chapter 17's entry, the index page). Then, in the same delivery (owner
decision 9): hash (its guard and wipe on secure's, its `engaged` calls unchanged in meaning, its
wipe seam on secure's, B9), then crypto (every guard, wipe, erase, comparison, secret and
declassify on secure's; its six headers deleted with no alias; `timing_hash_interop_test` replaced
by a test that nests a guarded hash call in a guarded crypto call; dudect and benches on
`secure::cpu_has_dit()`). Everything is verified on every local lane and merged with one CI at the
end; codecs is built on secure from the start. The withdrawn branch's commits are the worked
example of moving every guard site.

## Open Questions

- **O1 (decided by the owner, 2026-11-24):** where the test support shared by several libraries'
  tests lives: the seam recorder, the DIT helper and the constant-time checkers' marks. They stay
  in each library's own `test/support/` for now: secure's in secure's (D9), and hash and crypto
  keep or gain their own copies in theirs. Not in `tools/node-test`, and no `webcpp::core` library
  is made for them.
