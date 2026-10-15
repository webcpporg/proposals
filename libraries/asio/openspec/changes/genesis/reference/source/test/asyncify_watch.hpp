// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_ASIO_ASYNCIFY_WATCH_HPP
#define WEBCPP_TEST_ASIO_ASYNCIFY_WATCH_HPP

#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <iostream>
#include <string_view>

EM_JS_DEPS(webcpp_test_asio_asyncify_stack, "$Asyncify,$doPollAsync");

// Watches every unwind from here on: when one stops, the bytes it wrote are its data's position
// less the stack's start, which is its limit less Asyncify's stack size. It reads Emscripten's
// private runtime, which may change: a reading that finds no unwind, or no byte written, fails.
// Count blocking polls too: an expired C++ timer can finish in poll_one, measuring only the
// initial nonblocking poll's shorter path while still reporting an unwind.
// clang-format off
EM_JS(void, test_watch_unwinds, (), {
    const watch = {deepest : 0, unwinds : 0, waits : 0};
    globalThis.webcppAsyncifyWatch = watch;
    const poll = doPollAsync;
    doPollAsync = function(fds, nfds, timeout) {
        if (timeout !== 0) {
            ++watch.waits;
        }
        return poll(fds, nfds, timeout);
    };
    const stop = Asyncify.maybeStopUnwind;
    Asyncify.maybeStopUnwind = function() {
        const data = Asyncify.currData;
        if (data && Asyncify.state === Asyncify.State.Unwinding && !Asyncify.exportCallStack.length) {
            const start = HEAPU32[(data + 4) >> 2] - Asyncify.StackSize;
            watch.deepest = Math.max(watch.deepest, HEAPU32[data >> 2] - start);
            ++watch.unwinds;
        }
        return stop.call(Asyncify);
    };
});
// clang-format on

namespace webcpp::test {

/** The unwinds seen since the last reset, and the stack Asyncify saves each in. */
struct unwinds {
    static void reset() {
        EM_ASM({
            globalThis.webcppAsyncifyWatch.deepest = 0;
            globalThis.webcppAsyncifyWatch.unwinds = 0;
            globalThis.webcppAsyncifyWatch.waits = 0;
        });
    }

    /** The bytes the deepest unwind since the last reset wrote. */
    static std::size_t deepest() {
        return static_cast<std::size_t>(
            EM_ASM_INT({ return globalThis.webcppAsyncifyWatch.deepest; }));
    }

    static std::size_t count() {
        return static_cast<std::size_t>(
            EM_ASM_INT({ return globalThis.webcppAsyncifyWatch.unwinds; }));
    }

    /** Polls that can wait, excluding the initial nonblocking reactor poll. */
    static std::size_t waits() {
        return static_cast<std::size_t>(
            EM_ASM_INT({ return globalThis.webcppAsyncifyWatch.waits; }));
    }

    /** ASYNCIFY_STACK_SIZE, as the program was linked with it. */
    static std::size_t stack_size() {
        return static_cast<std::size_t>(EM_ASM_INT({ return Asyncify.StackSize; }));
    }
};

/**
 Measures the unwinds of one path, which call makes on a fresh io_context, and checks that the
 deepest leaves half of the stack.

 @param path What call does, as printed.
 @param call Runs the io_context through the path.
 @param waits Whether the path must reach a blocking reactor poll.
*/
template <class Call>
void measure(std::string_view path, Call call, bool waits = true) {
    unwinds::reset();
    call();
    const std::size_t deepest = unwinds::deepest();
    std::cout << path << ": " << deepest << " bytes of " << unwinds::stack_size() << '\n';
    if (waits) {
        BOOST_TEST_GE(unwinds::waits(), 1U);
    }
    BOOST_TEST_GE(unwinds::count(), 1U);
    BOOST_TEST_GT(deepest, 0U);
    BOOST_TEST_LE(2 * deepest, unwinds::stack_size());
}

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_ASIO_ASYNCIFY_WATCH_HPP
