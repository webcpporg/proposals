// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Loads the artifact named by the build's manifest (doc: #asio-building-loader).
// A supported JSPI runtime selects JSPI; other runtimes select Asyncify.

/** Whether scope has JavaScript Promise Integration. */
export function supportsJspi(scope = globalThis) {
    return typeof scope.WebAssembly?.Suspending === 'function'
}

/** The selected variant and its script path, relative to the manifest. */
export function pick(manifest, jspi = supportsJspi()) {
    const variant = jspi ? 'jspi' : 'asyncify'
    const script = manifest?.[variant]?.script
    if (typeof script !== 'string') {
        throw new Error(`the manifest names no ${variant} artifact`)
    }
    return {variant, script}
}

/** Fetches the manifest, imports its selected artifact and awaits the module factory. */
export async function load(manifestUrl, options = {}) {
    const {jspi = supportsJspi(), module = {}, fetch = globalThis.fetch,
        importModule = url => import(url)} = options
    const base = new URL(manifestUrl, globalThis.location?.href ?? import.meta.url)
    const response = await fetch(base.href)
    if (!response.ok) {
        throw new Error(`${base.href}: ${response.status} ${response.statusText}`)
    }
    const {variant, script} = pick(await response.json(), jspi)
    const artifactBase = response.url || base
    const factory = (await importModule(new URL(script, artifactBase).href)).default
    return {variant, module: await factory(module)}
}
