// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// This process executes the actual Emscripten artifact in Node, not a browser controller.
import {pathToFileURL} from 'node:url'
import {resolve} from 'node:path'
const [artifact, plain, secure, expected] = process.argv.slice(2)
globalThis.location = {port: plain}
globalThis.beastPeerPort = plain
globalThis.beastSecurePort = secure
globalThis.beastNodeRuntime = true
const originalFetch = globalThis.fetch
globalThis.fetch = (url, options) => originalFetch(typeof url === 'string' && url.startsWith('/') ? `http://127.0.0.1:${plain}${url}` : url, options)
let complete
const completed = new Promise(resolve => { complete = resolve })
const imported = await import(pathToFileURL(resolve(artifact)).href)
await imported.default({print(line) { console.log(line); if (line === expected) complete() }, printErr: console.error})
await completed
await new Promise(setImmediate)
