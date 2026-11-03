// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Two real browser tabs exercise the retained upstream chat UI against its C++ server.
import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {once} from 'node:events'
import {copyFileSync, mkdtempSync, rmSync, writeFileSync} from 'node:fs'
import {createServer} from 'node:net'
import {tmpdir} from 'node:os'
import {dirname, join, resolve} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'
import {fileURLToPath} from 'node:url'
import {startChrome, openPage, cleanUp} from '../browser/run.mjs'

const here = dirname(fileURLToPath(import.meta.url))
const executable = resolve(process.argv[2])
const directory = mkdtempSync(join(tmpdir(), 'beast-chat-ui-'))
copyFileSync(resolve(here, '../../../example/upstream/websocket/server/chat-multi/chat_client.html'),
    join(directory, 'chat_client.html'))
const reserve = createServer()
reserve.listen(0, '127.0.0.1')
await once(reserve, 'listening')
const port = reserve.address().port
await new Promise(resolve => reserve.close(resolve))
let command = executable
let args = ['127.0.0.1', String(port), directory, '1']
if (executable.endsWith('.wasm')) {
    command = process.env.WASMTIME ?? 'wasmtime'
    args = ['run', '-S', 'p3', '-S', 'inherit-network=y', '-S', 'tcp=y',
        '-W', 'component-model-async=y', '-W', 'component-model-threading=y',
        '-W', 'timeout=30s', '--dir', directory, executable, ...args]
}
const server = spawn(command, args, {stdio: ['ignore', 'pipe', 'pipe']})
let diagnostics = ''
server.stderr.on('data', data => { diagnostics += data })
const pages = []
try {
    const base = `http://127.0.0.1:${port}`
    const until = Date.now() + 10000
    while (true) {
        if (server.exitCode !== null) throw new Error(diagnostics)
        try { if ((await fetch(`${base}/chat_client.html`)).ok) break } catch {}
        assert.ok(Date.now() < until, 'chat server readiness timed out')
        await delay(25)
    }
    const devtools = await startChrome()
    for (const name of ['Alice', 'Bob']) {
        const page = await openPage(devtools, base, '', null, `${base}/chat_client.html`)
        pages.push(page)
        const deadline = Date.now() + 10000
        while (!await page.evaluate('document.getElementById("send") !== null')) {
            assert.ok(Date.now() < deadline, 'chat UI loading timed out')
            await delay(25)
        }
        await page.evaluate(`uri.value = 'ws://127.0.0.1:${port}'; userName.value = '${name}'; connect.click()`)
    }
    const waitFor = async text => {
        const until = Date.now() + 10000
        while (true) {
            const transcripts = await Promise.all(pages.map(page =>
                page.evaluate('messages.innerText')))
            if (transcripts.every(value => value.includes(text))) return transcripts
            assert.ok(Date.now() < until, `missing chat message ${text}: ${transcripts}`)
            await delay(25)
        }
    }
    await waitFor('[connection opened]')
    await pages[0].evaluate('sendMessage.value = "hello from Alice"; send.click()')
    await waitFor('Alice: hello from Alice')
    await pages[1].evaluate('sendMessage.value = "hello from Bob"; send.click()')
    const transcripts = await waitFor('Bob: hello from Bob')
    await pages[1].evaluate('disconnect.click()')
    const closeDeadline = Date.now() + 10000
    while (!(await pages[1].evaluate('messages.innerText')).includes('[connection closed]')) {
        assert.ok(Date.now() < closeDeadline, 'chat close timed out')
        await delay(25)
    }
    await pages[0].evaluate('sendMessage.value = "remaining participant"; send.click()')
    while (!(await pages[0].evaluate('messages.innerText')).includes('remaining participant')) {
        assert.ok(Date.now() < closeDeadline, 'remaining participant broadcast timed out')
        await delay(25)
    }
    const screenshot = join(dirname(executable), 'chat-ui.png')
    writeFileSync(screenshot, Buffer.from(await pages[0].screenshot(), 'base64'))
    await pages[0].evaluate('disconnect.click()')
    console.log(JSON.stringify({application: 'websocket-chat-multi', backend: 'browser-ui',
        result: 'passed', participants: 2, broadcasts: 3, transcripts, screenshot}))
} finally {
    for (const page of pages) await page.close()
    await cleanUp()
    if (server.exitCode === null && server.signalCode === null) {
        const ended = once(server, 'exit')
        const timeout = setTimeout(() => server.kill('SIGKILL'), 3000)
        server.kill()
        try { await ended } finally { clearTimeout(timeout) }
    }
    rmSync(directory, {recursive: true, force: true})
}
