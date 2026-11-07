// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Exercise startup failures at the process/HTTP boundary, then delay a real
// Chrome launch beyond the former ten-second limit and load both real variants.
import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {once} from 'node:events'
import {mkdtempSync, rmSync, writeFileSync} from 'node:fs'
import {createServer} from 'node:http'
import {tmpdir} from 'node:os'
import {join} from 'node:path'
import {fileURLToPath} from 'node:url'
import {findChrome, waitForChrome} from './chrome_startup.mjs'

const scratch = mkdtempSync(join(tmpdir(), 'asio-startup-test-'))
const fixture = async (response, check) => {
    const profile = mkdtempSync(join(scratch, 'profile-'))
    const child = spawn(process.execPath, ['-e', 'setInterval(() => {}, 10000)'])
    const server = response && createServer(response)
    try {
        if (server) {
            server.listen(0, '127.0.0.1')
            await once(server, 'listening')
            writeFileSync(join(profile, 'DevToolsActivePort'), `${server.address().port}\n`)
        }
        await check(child, profile)
    } finally {
        if (child.exitCode === null && child.signalCode === null) {
            const ended = once(child, 'exit')
            child.kill()
            await ended
        }
        if (server) {
            server.closeAllConnections()
            await new Promise(resolve => server.close(resolve))
        }
    }
}

try {
    await fixture(null, async (child, profile) => {
        await assert.rejects(waitForChrome(child, profile, {timeout: 200}),
            /startup timed out.*waiting for DevToolsActivePort/s)
    })
    await fixture((request, response) => response.writeHead(503).end('not ready'),
        async (child, profile) => {
            await assert.rejects(waitForChrome(child, profile, {timeout: 1000}),
                /startup timed out.*\/json\/version.*HTTP 503/s)
        })
    await fixture(() => {}, async (child, profile) => {
        const start = performance.now()
        await assert.rejects(waitForChrome(child, profile, {timeout: 250, requestTimeout: 100}),
            /startup timed out.*\/json\/version.*TimeoutError/s)
        assert.ok(performance.now() - start < 5000, 'an unanswered discovery request escaped its bound')
    })
    await fixture(null, async (child, profile) => {
        const waiting = waitForChrome(child, profile)
        child.kill('SIGTERM')
        await assert.rejects(waiting, /Chrome exited during startup.*SIGTERM/s)
    })
    const missing = spawn(join(scratch, 'no-such-chrome'))
    await assert.rejects(waitForChrome(missing, scratch), /Chrome failed to start.*ENOENT/s)

    const wrapper = join(scratch, 'delayed-chrome')
    const quoted = `'${findChrome().replaceAll("'", "'\\''")}'`
    writeFileSync(wrapper, `#!/bin/sh\nsleep 11\nexec ${quoted} "$@"\n`, {mode: 0o700})
    const driver = spawn(process.execPath, [fileURLToPath(new URL('./run.mjs', import.meta.url)),
        '--mode', 'loader', process.argv.at(-1)], {
        env: {...process.env, CHROME: wrapper}, stdio: ['ignore', 'pipe', 'pipe']
    })
    let output = ''
    driver.stdout.on('data', data => { output += data })
    driver.stderr.on('data', data => { output += data })
    const bound = setTimeout(() => driver.kill(), 55000)
    let status
    try { [status] = await once(driver, 'close') } finally { clearTimeout(bound) }
    assert.equal(status, 0, output)
    assert.match(output, /browser loader: ok/)
    process.stdout.write(output)
    console.log('browser startup: bounded failures and delayed real Chrome passed')
} finally {
    rmSync(scratch, {recursive: true, force: true})
}
