// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_CRYPTO_DETAIL_WIPE_HPP
#define WEBCPP_CRYPTO_DETAIL_WIPE_HPP

#include <webcpp/crypto/config.hpp>

#include <atomic>
#include <cstddef>
#include <type_traits>

namespace webcpp::crypto::detail {

/**
 * Zeroes `byte_count` bytes at `wiped_data` with stores the optimiser cannot drop.
 *
 * Tip: a `memset` before an object dies is a dead store a compiler may remove; stores through
 * a volatile pointer, then a signal fence, are kept.
 *
 * @param wiped_data The first byte, or null when `byte_count` is 0.
 * @param byte_count How many bytes.
 */
inline void wipe(void* wiped_data, std::size_t byte_count) noexcept {
    auto* const wiped_bytes = static_cast<unsigned char volatile*>(wiped_data);
    for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
        wiped_bytes[byte_index] = 0;
    }
    std::atomic_signal_fence(std::memory_order_seq_cst);
}

/**
 * Zeroes an object's bytes, as @ref wipe does.
 *
 * @tparam T A trivially copyable type, a C library's key schedule or context.
 * @param wiped_object The object.
 */
template <class T>
void wipe_object(T& wiped_object) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "only a C library's plain state is wiped");
    wipe(&wiped_object, sizeof wiped_object);
}

}  // namespace webcpp::crypto::detail

#endif  // WEBCPP_CRYPTO_DETAIL_WIPE_HPP
