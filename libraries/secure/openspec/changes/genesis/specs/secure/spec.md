## ADDED Requirements

### Requirement: Library: header-only, names, headers and dependencies
The library SHALL be header-only C++20 in namespace `webcpp::secure` (with `webcpp::secure::constant_time` and `webcpp::secure::detail`), with public headers `<webcpp/secure.hpp>` and `<webcpp/secure/...>`, macros prefixed `WEBCPP_SECURE_` and include guards `WEBCPP_SECURE_<PATH>_HPP`, and MUST declare no name outside `webcpp::secure`. Every public header MUST include `<webcpp/secure/config.hpp>` first, which includes `<webcpp/config.hpp>` first and defines `WEBCPP_SECURE_NO_EXCEPTIONS` exactly when `WEBCPP_HAS_EXCEPTIONS` is not defined or a developer defines it. `build.jam` MUST declare the target `/webcpp/secure//secure` with the headers, Boost's headers and `/webcpp/config//config` alone, building and linking nothing. The library MUST include no header of another webcpp library but config's, and MUST hold no cryptographic algorithm. Every function MUST be `noexcept` and none SHALL throw.

#### Scenario: A program that erases a buffer builds nothing
- **WHEN** a Jamfile reads `exe app : app.cpp /webcpp/secure//secure ;` and `app.cpp` includes `<webcpp/secure.hpp>`, holds a `secure::timing_guard`, compares two buffers with `secure::constant_time_equal` and erases one with `secure::erase_secret`
- **THEN** it builds and runs on native, wasip2, wasip3 and Emscripten, with exceptions on and off, and no library is compiled for secure

#### Scenario: Each header compiles first and alone
- **WHEN** each public header of the library is included first and alone in a translation unit
- **THEN** it compiles, and `WEBCPP_VERSION` is defined after it

### Requirement: Timing guard: DIT for a scope, nesting across libraries
`<webcpp/secure/timing_guard.hpp>` SHALL declare `secure::timing_guard`, `secure::cpu_has_dit()` and `secure::dit_is_set()`, public names of the library and the only DIT guard of webcpp. An engaged guard (`timing_guard{}` or `timing_guard{true}`) MUST set PSTATE.DIT for its scope when the CPU has FEAT_DIT and DIT is clear, and clear it on destruction only when it set it; a guard that finds DIT set MUST leave it set; a disengaged guard (`timing_guard{false}`) MUST do nothing. Each write of DIT MUST be followed by an `isb`. The guard MUST NOT be copyable or movable, MUST NOT change another thread's DIT, and MUST NOT execute a DIT instruction on a CPU without FEAT_DIT but the one probe Windows needs. `cpu_has_dit()` MUST be read once per process.

#### Scenario: Guards nest
- **WHEN** a test constructs three engaged guards nested, as codecs inside crypto inside hash would, with a disengaged one between them, on a CPU with DIT and DIT clear
- **THEN** DIT is set inside every level, still set after each inner guard ends, and clear after the outermost ends

#### Scenario: A program that set DIT keeps it
- **WHEN** a thread sets DIT itself and then constructs and destroys an engaged guard
- **THEN** DIT is set after the guard ends

#### Scenario: Another thread is untouched
- **WHEN** one thread holds an engaged guard while another thread, with DIT clear, reads `dit_is_set()`
- **THEN** the other thread reads false, natively where the build has threads

### Requirement: Timing guard: the targets it covers
The guard SHALL act, and `WEBCPP_SECURE_TIMING_COVERED` SHALL be defined, on arm64 under Apple's systems (FEAT_DIT from `sysctlbyname("hw.optional.arm.FEAT_DIT")`), under Linux and Android (`HWCAP_DIT` of `AT_HWCAP`), and under Windows with `_MSC_VER`, `_M_ARM64` and not `_M_ARM64EC` (the register through `_ReadStatusReg` and `_WriteStatusReg` and `__isb`, FEAT_DIT from one read of the register under structured exception handling, `STATUS_ILLEGAL_INSTRUCTION` meaning absent, other exceptions passed on). On every other target the guard MUST be an empty class that executes no instruction, and `cpu_has_dit()` and `dit_is_set()` MUST return false.

#### Scenario: Windows on ARM64
- **WHEN** `timing_guard_windows_test` runs on the Windows 11 ARM64 lane with MSVC 14.3 and 14.5, debug and release, with exceptions on and off
- **THEN** `cpu_has_dit()` is true on that CPU, DIT is set inside a guard, still set after the thread sleeps inside it, and clear after it

#### Scenario: WebAssembly and x86
- **WHEN** the guard's tests run on wasip2, wasip3, Emscripten and x86-64
- **THEN** `std::is_empty_v<webcpp::secure::timing_guard>` holds and both functions return false

### Requirement: Timing guard: one seam for the tests
A macro `WEBCPP_SECURE_TIMING_ENTERED(where)`, defined by a test alone before its first include, SHALL be run with a `std::source_location` of the guard's declaration on every engaged guard's construction, before DIT is touched, on every target; a disengaged guard MUST NOT run it. Without the macro the constructor MUST take no source location, so that no function name or path reaches a program. The location's `function_name()` MUST name the function that declares the guard and its `file_name()` MUST let a test tell one library's guards from another's.

#### Scenario: The seam names the guarded function
- **WHEN** a test with the seam defined calls a function of its own that declares an engaged guard, then one that declares a disengaged guard
- **THEN** the seam records one entry, whose `function_name()` is the first function's, and none for the second

#### Scenario: Nothing reaches a program
- **WHEN** a translation unit without the seam constructs a guard
- **THEN** `<source_location>` is not included by the guard's header and the constructor takes only `engaged`

### Requirement: Memory: erase_secret
`<webcpp/secure/erase_secret.hpp>` SHALL declare `secure::erase_secret(range)`, which zeroes `size() * sizeof(element)` bytes from the first element of a contiguous sized range of non-const, trivially copyable elements, with volatile stores followed by a signal fence that the optimiser keeps, and touches no byte around them. A range of const or not trivially copyable elements MUST stop the build with a message naming the ranges it takes; a range that is not contiguous, and a pointer with a size, MUST NOT be accepted.

#### Scenario: A buffer erased before its scope ends
- **WHEN** a test erases a stack buffer as its last use before its scope ends, built at `-O2` with full inlining, and reads that storage afterwards through the test's placement storage
- **THEN** every byte reads zero, on every lane

#### Scenario: A refused range
- **WHEN** a test asks whether `erase_secret` accepts a `std::list<char>&`, a `std::vector<bool>&` and a `std::span<std::byte const>`
- **THEN** it accepts none of them, and accepts a `std::vector<std::uint8_t>&`, a `std::string&`, a `std::span<std::byte>` and a span of one C library context

### Requirement: Memory: secret
`<webcpp/secure/secret.hpp>` SHALL declare `secure::secret<N>` for `N > 0`: `N` bytes held in place, zero-initialised, move-only (the source wiped after a move, the target wiped before it takes new bytes, a self-move changing nothing), copied only by `clone()`, read through `bytes()` and an implicit conversion to `std::span<std::uint8_t const, N>` on an lvalue, written through `writable()` on an lvalue, with those views deleted on an rvalue, `size()` and `size_bytes` equal to `N`, and its storage wiped on destruction as `erase_secret` wipes.

#### Scenario: Destroyed and moved-from storage reads zero
- **WHEN** a test constructs a `secret<32>` in placement storage, writes it, moves it into another and destroys both
- **THEN** the moved-from storage reads zero after the move and both storages read zero after destruction

#### Scenario: No view of a temporary
- **WHEN** a test asks whether `bytes()`, `writable()` or the span conversion are callable on an rvalue `secret<16>`
- **THEN** none is

### Requirement: Comparison: constant_time_equal
`<webcpp/secure/constant_time_equal.hpp>` SHALL declare `secure::constant_time_equal(first, second)` for any two contiguous sized ranges of one-byte elements (`std::uint8_t`, `unsigned char`, `char`, `std::byte`), returning whether they have the same size and the same bytes. Sequences of different sizes MUST compare unequal without reading their contents; otherwise every byte MUST be read, with no branch and no index that depends on a byte, and the result reduced to one bit without a branch. It MUST hold an engaged guard, and MUST NOT include a Boost.Hash2 header.

#### Scenario: Every single-byte difference
- **WHEN** a test compares a 32-byte sequence with each copy of it that differs in one bit, and with itself, as spans, arrays, a `secret<32>`, a `std::string_view` and a Boost.Hash2 `digest<32>`
- **THEN** only the identical ones compare equal, and the seam records the guard in `constant_time_equal`

### Requirement: Constant-time checks: declassify
`<webcpp/secure/declassify.hpp>` SHALL declare `secure::declassify(value)` for a non-const, trivially copyable object, which does nothing and includes nothing in a program's build, marks the object's bytes defined for memcheck (`VALGRIND_MAKE_MEM_DEFINED`) when `WEBCPP_SECURE_CT_VALGRIND` is defined, and unpoisons them for MemorySanitizer (`__msan_unpoison`) when `WEBCPP_SECURE_CT_MSAN` is defined; defining both MUST stop the build. Marking a value secret and reading a checker's shadow MUST NOT be public names of the library: they are its test support.

#### Scenario: The marks compile in a checker's build
- **WHEN** `declassify_compile_test` includes every header of the library with `WEBCPP_SECURE_CT_VALGRIND` defined, and under clang with `WEBCPP_SECURE_CT_MSAN` defined
- **THEN** it compiles and runs, and a branch on a declassified verdict is not reported by the checker while a branch on an undeclassified one is

### Requirement: Constant-time blocks
`<webcpp/secure/constant_time.hpp>` SHALL declare, in `secure::constant_time`, `value_barrier`, `mask_from_bit`, `is_zero_mask`, `is_equal_mask`, `is_less_mask`, `is_in_range_mask` and `select` over any unsigned integral type but `bool`, `lookup` over a table of unsigned integral entries, and `select_bytes`, `equal_mask`, `is_zero_mask` and `less_mask` (big-endian) over byte sequences. A mask MUST be all zeros or all ones of its type. No block MUST branch on, or index memory by, a value it receives but a public size; `lookup` MUST read every entry of its table and yield a value-initialised entry for an index past it; every mask a block makes MUST pass through `value_barrier`. Each block MUST equal a plain reference implementation on every input.

#### Scenario: Exhaustive over bytes
- **WHEN** the blocks' test runs every pair of `std::uint8_t` (every triple for `is_in_range_mask`) and random values of 16, 32 and 64 bits with their edges against the plain reference
- **THEN** every result is equal, on every lane

#### Scenario: memcheck sees no secret-dependent access
- **WHEN** the constant-time lane marks the operands undefined and runs every block, `constant_time_equal` and `secret<N>`'s moves under memcheck, and under MemorySanitizer where crypto runs it
- **THEN** no conditional jump and no address depends on them, while the planted branching `select` and indexing `lookup` are reported

#### Scenario: dudect judges
- **WHEN** dudect measures `constant_time_equal`, `lookup`, `select_bytes` and `less_mask` with two classes of input natively
- **THEN** each case's |t| stays below the threshold, its A/A control too, and the planted leak exceeds it

### Requirement: Tests and lanes
Every test of the library SHALL run on native (clang and gcc 15), wasip2, wasip3 and Emscripten with its `_no_exceptions` twin, and on MSVC 14.3 and 14.5 and Apple clang as the other libraries; the guard's tests MUST also run on the Windows ARM64 lane; a counting `operator new` MUST see no allocation in any function of the library. The seam recorder, the DIT helper and the checkers' marks that secure's tests use MUST live in secure's own `test/support/`.

#### Scenario: No heap
- **WHEN** `allocation_test` replaces every form of `operator new` with a counter and calls every function of the library
- **THEN** the counter is zero after every call, on every target

### Requirement: Documentation
The library SHALL have its page under `libs/secure/doc/`, built by `b2 doc`: what secure is and is not (no algorithm), the guard (its coverage table, nesting, `engaged`, the seam), `erase_secret`, `secret<N>`, `constant_time_equal`, `declassify`, the constant-time blocks with their limits, the guard's measured cost from release builds, and the changelog, every example built and run in the lanes.

#### Scenario: The page builds
- **WHEN** `b2 doc` runs in the superproject
- **THEN** secure's page is produced with every section above, and its examples' expected outputs match their runs
