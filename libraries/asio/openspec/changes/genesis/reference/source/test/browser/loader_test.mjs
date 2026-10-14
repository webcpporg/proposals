// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Detects and selects a variant, then loads both real artifacts built by b2.
// Completion is signalled by the program's output, never inferred from a fixed delay.
import assert from 'node:assert/strict'
import {readFileSync} from 'node:fs'
import {createServer} from 'node:http'
import {once} from 'node:events'
import {dirname, join} from 'node:path'
import {pathToFileURL} from 'node:url'
import {load, pick, supportsJspi} from '../../include/webcpp/asio/webcpp-asio-loader.mjs'

const program = process.argv.at(-1)
const manifestPath = join(dirname(program), 'page_program.variants.json')
const manifestUrl = pathToFileURL(manifestPath)
const fetchFile = async url => ({ok: true, status: 200, statusText: 'OK',
    json: async () => JSON.parse(readFileSync(new URL(url), 'utf8'))})

assert.equal(supportsJspi({WebAssembly: {Suspending: function () {}}}), true)
assert.equal(supportsJspi({WebAssembly: {Suspending: true}}), false)
assert.equal(supportsJspi({WebAssembly: {}}), false)
assert.equal(supportsJspi({}), false)
const fixture = {
    jspi: {script: './nested/modern.mjs'},
    asyncify: {script: '../fallback/compatible.mjs'}
}
assert.deepEqual(pick(fixture, true), {variant: 'jspi', script: './nested/modern.mjs'})
assert.deepEqual(pick(fixture, false), {variant: 'asyncify', script: '../fallback/compatible.mjs'})
assert.deepEqual(pick(fixture), supportsJspi() ? pick(fixture, true) : pick(fixture, false))
for (const invalid of [null, {}, {jspi: {script: 7}}]) {
    assert.throws(() => pick(invalid, true), /no jspi artifact/)
}
assert.throws(() => pick({jspi: fixture.jspi}, false), /no asyncify artifact/)

// URL resolution, module options and the returned factory result are observable loader contracts.
const options = {noInitialRun: true}
const instance = {loaded: true}
let imported
let passed
const selected = await load('https://example.invalid/app/config/variants.json', {
    jspi: false,
    module: options,
    fetch: async url => {
        assert.equal(url, 'https://example.invalid/app/config/variants.json')
        return {ok: true, json: async () => fixture}
    },
    importModule: async url => {
        imported = url
        return {default: async argument => { passed = argument; return instance }}
    }
})
assert.equal(imported, 'https://example.invalid/app/fallback/compatible.mjs')
assert.equal(passed, options)
assert.deepEqual(selected, {variant: 'asyncify', module: instance})
await assert.rejects(load(manifestUrl, {
    fetch: async () => ({ok: false, status: 404, statusText: 'Not Found'})
}), /404 Not Found/)
const failure = new Error('module import failed')
await assert.rejects(load(manifestUrl, {
    jspi: true, fetch: fetchFile, importModule: async () => { throw failure }
}), error => error === failure)

// A real HTTP redirect changes the base of the manifest's relative artifact paths.
const server = createServer((request, response) => {
    if (request.url === '/latest/variants.json') {
        response.writeHead(302, {Location: '/releases/v2/variants.json'})
        response.end()
    } else {
        response.writeHead(200, {'Content-Type': 'application/json'})
        response.end(JSON.stringify(fixture))
    }
})
server.listen(0, '127.0.0.1')
await once(server, 'listening')
try {
    const origin = `http://127.0.0.1:${server.address().port}`
    await load(`${origin}/latest/variants.json`, {
        jspi: true,
        importModule: async url => {
            assert.equal(url, `${origin}/releases/v2/nested/modern.mjs`)
            return {default: async () => instance}
        }
    })
} finally {
    await new Promise((resolve, reject) => server.close(error => error ? reject(error) : resolve()))
}

for (const jspi of [true, false]) {
    const lines = []
    let completed
    const completion = new Promise(resolve => { completed = resolve })
    const {variant, module} = await load(manifestUrl, {
        jspi,
        fetch: fetchFile,
        module: {print: line => { lines.push(line); completed() }}
    })
    assert.equal(variant, jspi ? 'jspi' : 'asyncify')
    assert.equal(typeof module, 'object')
    // The driver bounds a missing completion at 60 seconds and fails the test.
    await completion
    assert.deepEqual(lines, ['ran 3 handlers'])
    console.log(`loader: ${variant}: ran 3 handlers`)
}
console.log('loader: ok')
