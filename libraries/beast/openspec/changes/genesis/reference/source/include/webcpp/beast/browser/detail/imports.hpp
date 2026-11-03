// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_DETAIL_IMPORTS_HPP
#define WEBCPP_BEAST_BROWSER_DETAIL_IMPORTS_HPP

#include <webcpp/beast/config.hpp>

#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <cstddef>
#include <memory_resource>
#include <string>
#include <string_view>

// EM_ASM supports definitions in inline functions across translation units.
// Browser exceptions are caught in JavaScript before they can cross a Wasm frame.
// clang-format off
inline emscripten::EM_VAL webcpp_beast_open(emscripten::EM_VAL url, emscripten::EM_VAL protocols, emscripten::EM_VAL callback) {
    return reinterpret_cast<emscripten::EM_VAL>(EM_ASM_PTR({
    const url = $0;
    const protocols = $1;
    const callback = $2;

    const deliver = Emval.toValue(callback);
    try {
        const socket = new WebSocket(Emval.toValue(url), Emval.toValue(protocols));
        socket.binaryType = 'arraybuffer';
        for (const type of ['open', 'message', 'close', 'error'])
            socket['on' + type] = event => deliver(event);
        return Emval.toHandle({socket, error: 0});
    } catch (error) {
        return Emval.toHandle({error: error.name === 'SyntaxError' ? 1 :
            error.name === 'SecurityError' ? 2 : 3});
    }
    }, url, protocols, callback));
}
inline int webcpp_beast_send(emscripten::EM_VAL handle, const void* data, std::size_t size, bool text) {
    return EM_ASM_INT({
    const handle = $0;
    const data = $1;
    const size = $2;
    const text = $3;

    const socket = Emval.toValue(handle);
    if (socket.readyState !== WebSocket.OPEN) return 1;
    try {
        const bytes = HEAPU8.subarray(data, data + size);
        socket.send(text ? new TextDecoder().decode(bytes) : bytes);
        return 0;
    } catch (_) { return 1; }
    }, handle, data, size, text);
}
inline void webcpp_beast_close(emscripten::EM_VAL handle, int code, emscripten::EM_VAL reason) {
    EM_ASM({
    const handle = $0;
    const code = $1;
    const reason = $2;

    const socket = Emval.toValue(handle);
    if (!socket) return;
    try { if (code) socket.close(code, Emval.toValue(reason)); else socket.close(); } catch (_) {}
    }, handle, code, reason);
}
inline std::size_t webcpp_beast_message_size(emscripten::EM_VAL value) {
    return static_cast<std::size_t>(EM_ASM_INT({
    const value = $0;

    const data = Emval.toValue(value);
    return typeof data === 'string' ? new TextEncoder().encode(data).length : data.byteLength;
    }, value));
}
// NOLINTNEXTLINE(misc-const-correctness): JavaScript writes to the pointed Wasm storage
inline void webcpp_beast_copy(emscripten::EM_VAL value, void* data, std::size_t size) {
    EM_ASM({
    const value = $0;
    const data = $1;
    const size = $2;

    const source = Emval.toValue(value);
    // Take the Wasm view after C++ has prepared storage and may have grown memory.
    const destination = HEAPU8.subarray(data, data + size);
    if (typeof source === 'string') new TextEncoder().encodeInto(source, destination);
    else destination.set(new Uint8Array(source));
    }, value, data, size);
}
inline emscripten::EM_VAL webcpp_beast_fetch(emscripten::EM_VAL url, emscripten::EM_VAL init, emscripten::EM_VAL callback) {
    return reinterpret_cast<emscripten::EM_VAL>(EM_ASM_PTR({
    const url = $0;
    const init = $1;
    const callback = $2;

    const controller = new AbortController();
    const options = Emval.toValue(init);
    options.signal = controller.signal;
    const limit = options.webcppBodyLimit;
    delete options.webcppBodyLimit;
    const deliver = Emval.toValue(callback);
    fetch(Emval.toValue(url), options).then(async response => {
        const headers = Array.from(response.headers);
        const metadata = {status: response.status, statusText: response.statusText,
            redirected: response.redirected, headers};
        deliver({type: 'fetch-header', ...metadata});
        const parts = [];
        let size = 0;
        if (response.body) {
            const reader = response.body.getReader();
            for (;;) {
                const chunk = await reader.read();
                if (chunk.done) break;
                if (chunk.value.byteLength > limit - size) {
                    await reader.cancel();
                    deliver({type: 'fetch-limit'});
                    return;
                }
                parts.push(chunk.value);
                size += chunk.value.byteLength;
            }
        }
        const bytes = new Uint8Array(size);
        let offset = 0;
        for (const part of parts) { bytes.set(part, offset); offset += part.byteLength; }
        const body = bytes.buffer;
        deliver({type: 'fetch', ...metadata, body});
    }).catch(() => deliver({type: 'fetch-error'}));
    return Emval.toHandle(controller);
    }, url, init, callback));
}
// clang-format on

namespace webcpp::beast::detail::browser_strings {
/** Copies a bounded UTF-8 view directly into JavaScript without a temporary C++ string. */
inline emscripten::val js_string(std::string_view text) {
    return emscripten::val::take_ownership(reinterpret_cast<emscripten::EM_VAL>(
        EM_ASM_PTR({ return Emval.toHandle(UTF8ToString($0, $1)); }, text.data(), text.size())));
}

/** Copies a JavaScript string into caller-allocated UTF-8 storage. */
inline std::pmr::string js_text(const emscripten::val& text, std::pmr::memory_resource* resource) {
    std::pmr::string result(webcpp_beast_message_size(text.as_handle()), '\0', resource);
    webcpp_beast_copy(text.as_handle(), result.data(), result.size());
    return result;
}
}  // namespace webcpp::beast::detail::browser_strings

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::detail {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace browser {
/** Copies a bounded UTF-8 view without allocating temporary C++ storage. */
using browser_strings::js_string;
/** Copies JavaScript text with the stream's memory resource. */
using browser_strings::js_text;
}  // namespace browser
}
}  // namespace webcpp::beast::detail
#endif
