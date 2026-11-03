// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_DETAIL_WASI_POLLABLE_DESCRIPTOR_HPP
#define WEBCPP_ASIO_DETAIL_WASI_POLLABLE_DESCRIPTOR_HPP
#include <array>
#include <cstddef>
#include <cstdint>
#include <webcpp/asio/config.hpp>

namespace webcpp::asio::detail {
/** Stable storage borrowed by the pinned wasi-libc descriptor table.
    Close the descriptor before destroying this storage or its pollable. */
struct wasi_pollable_descriptor {
    /** Opaque storage for the pinned libc descriptor, owned by this object. */
    alignas(std::max_align_t) std::array<std::byte, 32> opaque{};
    /** Whether libc currently borrows this object. */
    bool active = false;
    /** Borrowed native WASIp2 pollable handle. */
    std::int32_t handle = 0;
};
}  // namespace webcpp::asio::detail

/** Registers borrowed storage as a read-readiness-only descriptor.
    @param storage Stable descriptor state, alive until close returns.
    @return The descriptor, or -1 with errno set. */
extern "C" int webcpp_asio_wasi_pollable_descriptor(
    webcpp::asio::detail::wasi_pollable_descriptor* storage);
#endif
