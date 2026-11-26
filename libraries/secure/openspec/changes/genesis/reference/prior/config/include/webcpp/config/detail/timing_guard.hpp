// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CONFIG_DETAIL_TIMING_GUARD_HPP
#define WEBCPP_CONFIG_DETAIL_TIMING_GUARD_HPP

#include <webcpp/config.hpp>

/*
 * timing_guard sets 64-bit ARM's DIT (Data Independent Timing) bit, PSTATE.DIT, for the scope of
 * one secret computation and restores the thread's state after it. With DIT set, the instructions
 * Arm lists take a time independent of their data; Apple requires it of constant-time code on
 * Apple silicon, and none of the C libraries webcpp calls sets it. It is the one guard of webcpp's
 * libraries: every function of crypto, hash or codecs that computes on a secret holds one as its
 * first statement, and none keeps a copy. It is config's internal header, named as every
 * library's internals are, which every webcpp library may include and a program's interface never
 * names; <webcpp/config.hpp> does not include it.
 *
 * Some functions are keyed only by their arguments (a BLAKE2b digest of a call that passes a key),
 * and an unkeyed call computes on public data and must not pay for DIT: such a function constructs
 * the guard with `engaged` false when the call is not keyed. A disengaged guard sets nothing,
 * clears nothing and runs no seam, on every target.
 *
 * The guard applies on arm64 under macOS, iOS, Linux and Android, where the system says whether
 * the CPU has FEAT_DIT, and on Windows on ARM64 built by MSVC or clang-cl, where one read of the
 * register under structured exception handling says it (probe_dit, below). Everywhere else (x86,
 * ARM64EC, WebAssembly, where the engine decides) it is an empty class that executes no
 * instruction.
 */
#if defined(__aarch64__) && (defined(__APPLE__) || defined(__linux__))
/** Defined where timing_guard sets DIT: arm64 under an Apple system or Linux, Android included. */
#define WEBCPP_CONFIG_DETAIL_TIMING_COVERED 1
#include <cstddef>
#include <cstdint>
#ifdef __APPLE__
#include <sys/sysctl.h>
#else
#include <sys/auxv.h>
#endif
#elif defined(_MSC_VER) && defined(_M_ARM64) && !defined(_M_ARM64EC)
/** Defined where timing_guard sets DIT: Windows on ARM64, MSVC and clang-cl, not ARM64EC. */
#define WEBCPP_CONFIG_DETAIL_TIMING_COVERED 1
#include <excpt.h>
#include <intrin.h>
#include <atomic>
#include <cstdint>
#endif

/*
 * WEBCPP_CONFIG_DETAIL_TIMING_ENTERED(where), the seam, is defined by a test alone, before its
 * first include, to record the guards a call enters, so that a guarded function that forgets its
 * guard fails on every lane, also where DIT does not apply and also when it calls another guarded
 * function. Then timing_guard's constructor takes the std::source_location of the guard's
 * declaration, whose function_name() is the function that holds it and whose file_name() tells
 * which library declares it (webcpp/crypto/, webcpp/hash/, webcpp/codecs/), and runs the seam with
 * it first, on every target, when the guard is engaged. Nothing in a program's build: without the
 * seam the constructor takes no location, so that no function's name or header's path reaches a
 * program, whether the constructor is inlined or not.
 */
#ifdef WEBCPP_CONFIG_DETAIL_TIMING_ENTERED
#include <source_location>
#endif

namespace webcpp::config::detail {

#ifdef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
/** DIT's bit in the value of the register s3_3_c4_c2_5, bit 24 as in PSTATE. */
inline constexpr std::uint64_t dit_bit = std::uint64_t{1} << 24U;

#ifdef _MSC_VER
/** DIT's register for `_ReadStatusReg` and `_WriteStatusReg`: `ARM64_SYSREG(3, 3, 4, 2, 5)`. */
inline constexpr int dit_register = (1 << 14) | (3 << 11) | (4 << 7) | (2 << 3) | 5;
#ifdef ARM64_SYSREG
static_assert(dit_register == ARM64_SYSREG(3, 3, 4, 2, 5), "dit_register is DIT's encoding");
#endif

/** The full-system option of `isb`, `_ARM64_BARRIER_SY`. */
inline constexpr unsigned int isb_full_system = 0xFU;
#ifdef _ARM64_BARRIER_SY
static_assert(isb_full_system == _ARM64_BARRIER_SY, "isb_full_system is isb sy");
#endif

/** The code of the exception an undefined instruction raises, `STATUS_ILLEGAL_INSTRUCTION`. */
inline constexpr unsigned long status_illegal_instruction = 0xC000001DUL;
#endif

/**
 * Writes the DIT register, then executes `isb`.
 *
 * The register is named by its encoding (op0 3, op1 3, CRn 4, CRm 2, op2 5), which every
 * assembler takes, with or without an architecture flag. Apple's "Writing ARM64 code for Apple
 * platforms" ("Enable DIT for constant-time cryptographic operations") asks for a speculation
 * barrier after turning DIT on, so that the timing of the instructions after it reflects the new
 * state: `sb` where the CPU has FEAT_SB, else `dsb nsh` then `isb sy`; it asks for none after
 * turning DIT off. The `isb` here, after both writes, is not that barrier: which barrier each
 * write needs is the open measurement of crypto's change data-independent-timing.
 *
 * MSVC takes no inline assembly on ARM64, and clang-cl takes the same intrinsics, so that one
 * spelling serves both: `_WriteStatusReg` writes the register and `__isb` executes `isb sy`,
 * between two `std::atomic_signal_fence(seq_cst)`, which stand for the asm's `memory` clobber.
 *
 * @param dit_value `dit_bit` to set DIT, 0 to clear it.
 */
inline void write_dit(std::uint64_t dit_value) noexcept {
#ifdef _MSC_VER
    std::atomic_signal_fence(std::memory_order_seq_cst);
    _WriteStatusReg(dit_register, static_cast<__int64>(dit_value));
    __isb(isb_full_system);
    std::atomic_signal_fence(std::memory_order_seq_cst);
#else
    __asm__ volatile("msr s3_3_c4_c2_5, %0\n\tisb" : : "r"(dit_value) : "memory");
#endif
}

#ifdef _MSC_VER
/**
 * Whether the CPU has FEAT_DIT, by reading the DIT register once under structured exception
 * handling: Windows documents no processor feature flag for DIT and no user-mode read of
 * ID_AA64PFR0_EL1, and on a CPU without FEAT_DIT the read is undefined and raises
 * STATUS_ILLEGAL_INSTRUCTION, which the filter takes as "absent"; any other exception continues
 * its search. A function of its own holding no object that needs unwinding, which `__try` refuses
 * beside one (C2712); structured exception handling is not C++'s, so it works with exceptions on
 * or off. The value read is kept in a volatile, so that the read is never dropped.
 *
 * @return Whether the read succeeded.
 */
inline bool probe_dit() noexcept {
    __try {
        std::uint64_t volatile const state =
            static_cast<std::uint64_t>(_ReadStatusReg(dit_register));
        static_cast<void>(state);
        return true;
    } __except (GetExceptionCode() == status_illegal_instruction ? EXCEPTION_EXECUTE_HANDLER
                                                                 : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}
#endif
#endif

/**
 * Whether the CPU has DIT, read once per process.
 *
 * `hw.optional.arm.FEAT_DIT` on Apple's systems, where a value that cannot be read counts as
 * absent; on Windows, probe_dit's one read of the register under structured exception handling,
 * an illegal instruction meaning absent; `HWCAP_DIT` (bit 24 of `AT_HWCAP`) on Linux. Executing
 * DIT's instructions on a CPU without it traps, so nothing else reads or writes the register
 * unless this is true.
 *
 * @return Whether the CPU has FEAT_DIT; `false` on every target the guard does not cover.
 */
[[nodiscard]] inline bool cpu_has_dit() noexcept {
#if defined(WEBCPP_CONFIG_DETAIL_TIMING_COVERED) && defined(__APPLE__)
    static bool const has_dit = [] {
        int dit_feature = 0;
        std::size_t byte_count = sizeof dit_feature;
        return sysctlbyname("hw.optional.arm.FEAT_DIT", &dit_feature, &byte_count, nullptr, 0) ==
                   0 &&
               dit_feature != 0;
    }();
    return has_dit;
#elif defined(WEBCPP_CONFIG_DETAIL_TIMING_COVERED) && defined(_MSC_VER)
    static bool const has_dit = probe_dit();
    return has_dit;
#elif defined(WEBCPP_CONFIG_DETAIL_TIMING_COVERED)
    constexpr unsigned long hwcap_dit = 1UL << 24U;  // HWCAP_DIT, asm/hwcap.h
    static bool const has_dit = (getauxval(AT_HWCAP) & hwcap_dit) != 0;
    return has_dit;
#else
    return false;
#endif
}

/**
 * Whether DIT is set for the calling thread.
 *
 * @return PSTATE.DIT, read with `mrs` (`_ReadStatusReg` on MSVC and clang-cl), where the CPU has
 * DIT on a target the guard covers; `false` elsewhere, without executing `mrs`.
 */
[[nodiscard]] inline bool dit_is_set() noexcept {
#ifdef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
    if (!cpu_has_dit()) {
        return false;
    }
#ifdef _MSC_VER
    std::uint64_t const saved_state = static_cast<std::uint64_t>(_ReadStatusReg(dit_register));
#else
    std::uint64_t saved_state = 0;
    __asm__ volatile("mrs %0, s3_3_c4_c2_5" : "=r"(saved_state) : : "memory");
#endif
    return (saved_state & dit_bit) != 0;
#else
    return false;
#endif
}

/**
 * Sets DIT for its scope where the CPU has it, and leaves the thread's DIT as it found it.
 *
 * Construction, when engaged, finds DIT clear, sets it and remembers to clear it; the destructor
 * clears it then only. A guard that finds DIT set does nothing and leaves it set: the program set
 * it itself (as Apple's `timingsafe_enable_if_supported` does), or an outer guard did, of the same
 * library or of another, so that guards nest in any order of libraries and a thread whose program
 * asked for DIT keeps it after every call. DIT is a thread's own state, so a guard never changes
 * another thread's. The writes' `memory` clobber orders them against memory accesses and calls
 * (the backends' out-of-line C calls, a volatile accumulator), not against arithmetic on
 * registers alone, which the compiler may move across them: the same limit as every program that
 * sets DIT.
 */
class timing_guard {
public:
#ifdef WEBCPP_CONFIG_DETAIL_TIMING_ENTERED
    /**
     * Where `engaged`, runs the seam, then sets DIT where the CPU has it and it is clear: a test's
     * build alone.
     *
     * @param engaged Whether the call computes on a secret; false makes the guard do nothing, the
     * seam included.
     * @param where Where the guard is declared, the default: the seam's argument alone.
     */
    explicit timing_guard(bool engaged = true,
                          std::source_location where = std::source_location::current()) noexcept {
        if (!engaged) {
            return;
        }
        WEBCPP_CONFIG_DETAIL_TIMING_ENTERED(where);
        enter();
    }
#else
    /**
     * Where `engaged`, sets DIT where the CPU has it and it is clear.
     *
     * @param engaged Whether the call computes on a secret; false makes the guard do nothing.
     */
    explicit timing_guard(bool engaged = true) noexcept {
        if (engaged) {
            enter();
        }
    }
#endif

    /** Clears DIT when this guard set it. */
    ~timing_guard() {
#ifdef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
        if (armed_) {
            write_dit(0);
        }
#endif
    }

    /** Not copyable: the state it restores is the scope's. */
    timing_guard(timing_guard const&) = delete;

    /** Not assignable. */
    timing_guard& operator=(timing_guard const&) = delete;

    /** Not movable: the state it restores is the scope's. */
    timing_guard(timing_guard&&) = delete;

    /** Not move-assignable. */
    timing_guard& operator=(timing_guard&&) = delete;

private:
    /** Sets DIT where the CPU has it and it is clear, and remembers to clear it. */
    void enter() noexcept {
#ifdef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
        if (cpu_has_dit() && !dit_is_set()) {
            write_dit(dit_bit);
            armed_ = true;
        }
#endif
    }

#ifdef WEBCPP_CONFIG_DETAIL_TIMING_COVERED
    /** Whether this guard set DIT, and so clears it. */
    bool armed_ = false;
#endif
};

}  // namespace webcpp::config::detail

#endif  // WEBCPP_CONFIG_DETAIL_TIMING_GUARD_HPP
