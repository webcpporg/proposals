// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_SRC_HPP
#define WEBCPP_ASIO_BROWSER_SRC_HPP

#include <webcpp/asio/config.hpp>

#if !WEBCPP_ASIO_HAS_BROWSER_BRIDGE
#error \
    "webcpp::asio::browser::src requires the Emscripten browser bridge; use native/WASI Asio operations on this target"
#else

/**
 What a program on emscripten compiles once: the JavaScript functions asio calls
 (detail/browser_imports.hpp), the yield of the run family and the bridge's functions among them,
 and the export the bridge's functions deliver through.

 Tip: /webcpp/asio//asio links it into every program on emscripten, through
 src/browser_src.cpp; a program built otherwise includes it in exactly one source file.
*/
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/detail/browser_imports.hpp>

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/threading.h>
#endif

#include <cstdint>
#include <exception>

// Emval turns the handles C++ passes into values and back, and _emval_decref frees one;
// stackSave and stackRestore keep WebAssembly's stack pointer; UTF8ToString reads a string; and
// ExitStatus is what exit() throws. EM_JS_DEPS has Emscripten link them.
EM_JS_DEPS(webcpp_asio, "$Emval,_emval_decref,$stackSave,$stackRestore,$UTF8ToString,$ExitStatus");

#ifdef __EMSCRIPTEN_PTHREADS__
// Emscripten 6.0.11 routes worker poll through its main-thread proxy, but main-runtime
// poll takes the synchronous path even in a suspending entry stack. Reuse its own
// descriptor wait queues there; keep worker calls on the original proxy path.
EM_JS_DEPS(webcpp_asio_threaded_poll, "$doPollAsync");
// clang-format off
EM_ASYNC_JS(int, webcpp_asio_main_poll, (void* descriptors, unsigned count, int timeout), {
    return await doPollAsync(descriptors, count, timeout);
});
// clang-format on

// The linker reserves this spelling for its wrapped syscall import.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" int __real___syscall_poll(void* descriptors, unsigned count, int timeout);

// --wrap=__syscall_poll selects this only in the pthread build. The pinned wasm32
// libc import has three i32 arguments and an i32 result. Nonblocking poll uses a
// different import and remains nonsuspending, including inside browser callbacks.
// One definition is required, exactly as for the bridge export below.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming,misc-definitions-in-headers)
extern "C" int __wrap___syscall_poll(void* descriptors, unsigned count, int timeout) {
    if (emscripten_is_main_runtime_thread()) {
        return webcpp_asio_main_poll(descriptors, count, timeout);
    }
    return __real___syscall_poll(descriptors, count, timeout);
}
#endif

// clang-format off
EM_ASYNC_JS(void, webcpp_asio_yield, (), {
    await new Promise(resolve => {
        const channel = new MessageChannel();
        channel.port1.onmessage = () => {
            channel.port1.close();
            resolve();
        };
        channel.port2.postMessage(0);
    });
});

EM_JS(void, webcpp_asio_await, (emscripten::EM_VAL promise, emscripten::EM_VAL fulfil), {
    const deliver = Emval.toValue(fulfil);
    Promise.resolve(Emval.toValue(promise)).then(
        value => deliver({ok: true, value}),
        value => deliver({ok: false, value}));
});

EM_JS(emscripten::EM_VAL, webcpp_asio_string, (const char* data, std::size_t size), {
    return Emval.toHandle(UTF8ToString(data, size));
});

EM_JS(void, webcpp_asio_report, (emscripten::EM_VAL error), {
    (globalThis.reportError ?? console.error)(Emval.toValue(error));
});

EM_JS(emscripten::EM_VAL, webcpp_asio_function, (emscripten::EM_VAL token, void* bridge, std::uint64_t key), {
    const state = Emval.toValue(token);
    const address = bridge >>> 0;
    return Emval.toHandle(event => {
        // Once Emscripten marks the runtime aborted, nothing enters WebAssembly, as its own
        // callbacks do nothing then. A raw WebAssembly trap need not set ABORT.
        if (!state.alive || ABORT) {
            return;
        }
        // A JavaScript error passes every C++ catch and leaves WebAssembly without running its
        // frames' epilogues, in every build, and without running their destructors unless the
        // build has WebAssembly exceptions (measured with Emscripten 6.0.11). So what must hold
        // however the call ends is kept here: the stack pointer, put back after an error; the
        // count of deliveries in progress, which delivering() reads; and the event's handle,
        // which C++ only borrows.
        const top = stackSave();
        const handle = Emval.toHandle(event);
        const depth = state.depth;
        state.depth = depth + 1;
        try {
            _webcpp_asio_deliver(address, key, handle);
        } catch (error) {
            stackRestore(top);
            // A trap, which abort() throws too, goes on to the page; the runtime's own control
            // flow, exit()'s ExitStatus and 'unwind', ends the delivery, as Emscripten's own
            // callbacks let it; anything else is reported.
            if (error instanceof WebAssembly.RuntimeError) {
                throw error;
            }
            if (!(error instanceof ExitStatus) && error !== 'unwind') {
                (globalThis.reportError ?? console.error)(error);
            }
        } finally {
            state.depth = depth;
            __emval_decref(handle);
        }
    });
});
// clang-format on

namespace webcpp::asio::detail {

/**
 The event a delivery borrows: a val over a handle that the bridge's JavaScript function owns and
 frees however the delivery ends.

 Tip: a union, so that the val is never destroyed, by the delivery's end or by an unwinding that
 runs destructors (WebAssembly exceptions), and never frees the handle twice. A copy of it is a
 reference of its own.
*/
union borrowed_val {
    /**
     Borrows handle.

     @param handle The event's handle, which outlives the delivery.
    */
    explicit borrowed_val(emscripten::EM_VAL handle)
        : value(emscripten::val::take_ownership(handle)) {}

    /** Not copyable: one borrow of one handle. */
    borrowed_val(const borrowed_val&) = delete;
    /** Not copy-assignable: one borrow of one handle. */
    borrowed_val& operator=(const borrowed_val&) = delete;
    /** Not movable: one borrow of one handle. */
    borrowed_val(borrowed_val&&) = delete;
    /** Not move-assignable: one borrow of one handle. */
    borrowed_val& operator=(borrowed_val&&) = delete;

    /** Leaves value as it is, its handle being the JavaScript function's. */
    // Written out: = default is deleted in a union whose member has a non-trivial destructor.
    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~borrowed_val() {}

    /** The event. */
    emscripten::val value;
};

}  // namespace webcpp::asio::detail

// The export is defined here, in the one translation unit of a program that includes this file,
// never inline: an export is one definition the linker keeps.

/** Delivers event to the bridge's registration key; the bridge's JavaScript functions call it. */
// NOLINTNEXTLINE(misc-definitions-in-headers)
extern "C" EMSCRIPTEN_KEEPALIVE void webcpp_asio_deliver(void* bridge, std::uint64_t key,
                                                         emscripten::EM_VAL event) {
    auto& target = *static_cast<webcpp::asio::browser::bridge*>(bridge);
    const webcpp::asio::detail::borrowed_val borrowed(event);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    // A C++ exception is caught here, where catching it frees it: one that reached the page
    // would keep its object for good (332 bytes an escape, measured in trystero's
    // browser_event_loop_test.cpp).
    try {
        target.deliver(key, borrowed.value);
    } catch (const std::exception& e) {
        const auto message = emscripten::val::u8string(e.what());
        webcpp_asio_report(message.as_handle());
    } catch (...) {
        const emscripten::val message("an exception that is not a std::exception");
        webcpp_asio_report(message.as_handle());
    }
#else
    target.deliver(key, borrowed.value);
#endif
}

#endif  // WEBCPP_ASIO_HAS_BROWSER_BRIDGE
#endif  // WEBCPP_ASIO_BROWSER_SRC_HPP
