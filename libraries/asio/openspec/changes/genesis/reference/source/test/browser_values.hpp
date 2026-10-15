// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The JavaScript values asio's browser tests use, made and observed in JavaScript. It defines
// EM_JS functions, so that one source file of a program includes it, never two.

#ifndef WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP
#define WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP

#include <emscripten/em_js.h>
#include <emscripten/val.h>

// Emscripten's ExitStatus, which exit() throws, and the count of live Emval handles.
EM_JS_DEPS(webcpp_test_asio, "$ExitStatus,$count_emval_handles");

// clang-format off
EM_JS(emscripten::EM_VAL, test_event_target, (), {
    return Emval.toHandle(new EventTarget());
});

EM_JS(void, test_dispatch_after, (emscripten::EM_VAL target, int milliseconds, int detail), {
    const object = Emval.toValue(target);
    setTimeout(() => object.dispatchEvent(new CustomEvent('message', {detail})), milliseconds);
});

// A parameter's name is the JavaScript function's too: never a reserved word such as
// `function`, which links and then stops the program at load with a SyntaxError (measured with
// Emscripten 6.0.11). em_js_test proves every EM_JS here loads.
EM_JS(void, test_call_after, (emscripten::EM_VAL callback, int milliseconds, int value), {
    const call = Emval.toValue(callback);
    setTimeout(() => call(value), milliseconds);
});

// Calls callback times times from JavaScript, with 0, 1 and on: a C++ caller's epilogue would
// put WebAssembly's stack pointer back after each call, and hide one that a call left moved.
EM_JS(void, test_call_times, (emscripten::EM_VAL callback, int times), {
    const call = Emval.toValue(callback);
    for (let i = 0; i < times; ++i) {
        call(i);
    }
});

EM_JS(emscripten::EM_VAL, test_throwing_function, (), {
    return Emval.toHandle(() => {
        throw new Error('from JavaScript');
    });
});

// A function that throws what Emscripten's runtime throws: 1, a trap's WebAssembly.RuntimeError,
// as abort() throws too; 2, 'unwind'; 3, the ExitStatus that exit() throws.
EM_JS(emscripten::EM_VAL, test_throwing_runtime, (int kind), {
    return Emval.toHandle(() => {
        if (kind === 1) {
            throw new WebAssembly.RuntimeError('a trap');
        }
        throw kind === 2 ? 'unwind' : new ExitStatus(0);
    });
});

// Calls callback with 0: 0 when nothing escaped it, 1 when a WebAssembly.RuntimeError did, 2 when
// something else did.
EM_JS(int, test_call_caught, (emscripten::EM_VAL callback), {
    try {
        Emval.toValue(callback)(0);
        return 0;
    } catch (error) {
        return error instanceof WebAssembly.RuntimeError ? 1 : 2;
    }
});

EM_JS(int, test_emval_handles, (), {
    return count_emval_handles();
});

EM_JS(emscripten::EM_VAL, test_promise_after, (int milliseconds, int value, bool reject), {
    return Emval.toHandle(new Promise((resolve, fail) =>
        setTimeout(() => (reject ? fail(value) : resolve(value)), milliseconds)));
});

EM_JS(int, test_reported, (), {
    return globalThis.webcppTestReported ?? 0;
});

EM_JS(void, test_count_reports, (), {
    globalThis.webcppTestReported = 0;
    globalThis.reportError = () => { ++globalThis.webcppTestReported; };
});
EM_JS(emscripten::EM_VAL, test_never_settles, (), {
    return Emval.toHandle(new Promise(() => {}));
});
// Tracks the exact listener functions registered on this test target.
EM_JS(int, test_listener_count, (emscripten::EM_VAL handle), {
    const target = Emval.toValue(handle);
    if (!target.webcppListeners) {
        const listeners = new Set();
        target.webcppListeners = listeners;
        const add = target.addEventListener.bind(target);
        const remove = target.removeEventListener.bind(target);
        target.addEventListener = (type, listener, options) => {
            listeners.add(listener);
            add(type, listener, options);
        };
        target.removeEventListener = (type, listener, options) => {
            listeners.delete(listener);
            remove(type, listener, options);
        };
    }
    return target.webcppListeners.size;
});
// clang-format on

#endif  // WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP
