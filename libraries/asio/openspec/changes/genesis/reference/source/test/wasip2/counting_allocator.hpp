// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_TEST_P2_COUNTING_ALLOCATOR_HPP
#define WEBCPP_ASIO_TEST_P2_COUNTING_ALLOCATOR_HPP
#include <memory>

struct counts {
    unsigned allocations = 0;
    unsigned live = 0;
};

template <class T>
struct allocator {
    using value_type = T;
    counts* counter;

    explicit allocator(counts& value) : counter(&value) {}

    template <class U>
    explicit allocator(allocator<U> const& other) : counter(other.counter) {}

    T* allocate(std::size_t size) {
        ++counter->allocations;
        ++counter->live;
        return std::allocator<T>{}.allocate(size);
    }

    void deallocate(T* ptr, std::size_t size) {
        --counter->live;
        std::allocator<T>{}.deallocate(ptr, size);
    }

    template <class U>
    bool operator==(allocator<U> const& other) const {
        return counter == other.counter;
    }
};

#endif
