// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The real browser checks: idle waits, animation opportunities and variant loading.
// node run.mjs --mode <idle|render|loader> <program>
// CHROME names the browser; on macOS the newest cached Playwright headless shell is used.
// Each invocation owns its loopback server, Chrome process and temporary profile.
import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {once} from 'node:events'
import {existsSync, mkdtempSync, readFileSync, readdirSync, rmSync} from 'node:fs'
import {createServer} from 'node:http'
import {homedir, tmpdir} from 'node:os'
import {dirname, extname, join, resolve, sep} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'
import {fileURLToPath} from 'node:url'

const args = process.argv.slice(2)
assert.equal(args.length, 3, 'usage: run.mjs --mode <idle|render|loader> <program>')
assert.equal(args[0], '--mode')
const mode = args[1]
assert.ok(['idle', 'render', 'loader'].includes(mode), `unknown mode ${mode}`)
const here = dirname(fileURLToPath(import.meta.url))
const build = dirname(resolve(args[2]))
const findChrome = () => {
    if (process.env.CHROME) return process.env.CHROME
    assert.equal(process.platform, 'darwin', 'set CHROME to the Chrome executable')
    const cache = join(homedir(), 'Library/Caches/ms-playwright')
    const shells = existsSync(cache) ? readdirSync(cache)
        .filter(name => /^chromium_headless_shell-\d+$/.test(name))
        .sort((a, b) => Number(b.split('-').at(-1)) - Number(a.split('-').at(-1))) : []
    for (const shell of shells) {
        for (const arch of ['arm64', 'x64']) {
            const path = join(cache, shell, `chrome-headless-shell-mac-${arch}`, 'chrome-headless-shell')
            if (existsSync(path)) return path
        }
    }
    throw new Error('no cached headless Chrome found; set CHROME')
}

const left = {chrome: null, profile: null, server: null, sockets: new Set()}
const exited = child => child.exitCode !== null || child.signalCode !== null
let cleaning
const cleanUp = () => (cleaning ??= (async () => {
    for (const socket of left.sockets) socket.close()
    if (left.chrome && !exited(left.chrome) && left.chrome.pid) {
        const ended = once(left.chrome, 'exit')
        const stubborn = setTimeout(() => left.chrome.kill('SIGKILL'), 5000)
        left.chrome.kill()
        try { await ended } finally { clearTimeout(stubborn) }
    }
    if (left.server) {
        left.server.closeAllConnections()
        await new Promise(resolve => left.server.close(resolve))
    }
    if (left.profile) rmSync(left.profile, {recursive: true, force: true, maxRetries: 3})
})())
for (const [signal, code] of [['SIGHUP', 129], ['SIGINT', 130], ['SIGTERM', 143]]) {
    process.on(signal, () => cleanUp().finally(() => process.exit(code)))
}

const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript',
    '.wasm': 'application/wasm', '.json': 'application/json'}
const roots = {'/build/': build, '/loader/': resolve(here, '../../include/webcpp/asio'), '/': here}
const startServer = async () => {
    const server = createServer((request, response) => {
        try {
            const path = decodeURIComponent(new URL(request.url, 'http://127.0.0.1').pathname)
            const [prefix, root] = Object.entries(roots).find(([prefix]) => path.startsWith(prefix))
            const file = resolve(root, path.slice(prefix.length))
            if (!file.startsWith(root + sep)) {
                response.writeHead(404).end()
                return
            }
            const content = readFileSync(file)
            response.writeHead(200, {'Content-Type': types[extname(file)] ?? 'application/octet-stream'})
                .end(content)
        } catch {
            response.writeHead(404).end()
        }
    })
    left.server = server
    server.listen(0, '127.0.0.1')
    await once(server, 'listening')
    return `http://127.0.0.1:${server.address().port}`
}

const startChrome = async () => {
    left.profile = mkdtempSync(join(tmpdir(), `asio-browser-${process.pid}-`))
    const executable = findChrome()
    const chrome = spawn(executable, [
        '--headless=new', '--remote-debugging-address=127.0.0.1', '--remote-debugging-port=0',
        `--user-data-dir=${left.profile}`, '--no-first-run', '--no-default-browser-check',
        '--disable-component-update', '--disable-background-networking', '--disable-sync',
        '--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1', 'about:blank'
    ], {stdio: ['ignore', 'ignore', 'pipe']})
    left.chrome = chrome
    let error
    let said = ''
    chrome.on('error', value => { error = value })
    chrome.stderr.on('data', data => { said = (said + data).slice(-2000) })
    const deadline = Date.now() + 10000
    while (Date.now() < deadline) {
        if (error) throw error
        if (exited(chrome)) throw new Error(`Chrome exited during startup: ${said}`)
        const active = join(left.profile, 'DevToolsActivePort')
        const port = existsSync(active) ? readFileSync(active, 'utf8').split('\n')[0] : ''
        if (/^\d+$/.test(port)) {
            const devtools = `http://127.0.0.1:${port}`
            try {
                const version = await (await fetch(`${devtools}/json/version`)).json()
                console.log(`browser ${mode}: ${version.Browser}`)
                return devtools
            } catch {}
        }
        await delay(50)
    }
    throw new Error(`Chrome startup timed out: ${said}`)
}

// Installed before any program code; only page calls are recorded, not driver polling.
function observeIdle() {
    window.timers = []
    for (const name of ['setTimeout', 'setInterval']) {
        const original = window[name]
        window[name] = (callback, delay = 0, ...rest) => {
            window.timers.push({name, delay, at: performance.now()})
            return original(callback, delay, ...rest)
        }
    }
    const post = MessagePort.prototype.postMessage
    MessagePort.prototype.postMessage = function (...values) {
        window.timers.push({name: 'postMessage', delay: 0, at: performance.now()})
        return post.apply(this, values)
    }
}

function observeTurns() {
    window.turns = {posted: [], resumed: []}
    const Channel = window.MessageChannel
    const descriptor = Object.getOwnPropertyDescriptor(MessagePort.prototype, 'onmessage')
    window.MessageChannel = class extends Channel {
        constructor() {
            super()
            const post = this.port2.postMessage.bind(this.port2)
            this.port2.postMessage = value => {
                window.turns.posted.push(performance.now())
                post(value)
            }
            Object.defineProperty(this.port1, 'onmessage', {
                get() { return descriptor.get.call(this) },
                set(handler) {
                    descriptor.set.call(this, event => {
                        window.turns.resumed.push(performance.now())
                        handler(event)
                    })
                }
            })
        }
    }
}

const openPage = async (devtools, base, program, variant) => {
    const target = await (await fetch(`${devtools}/json/new?about:blank`, {method: 'PUT'})).json()
    const ws = new WebSocket(target.webSocketDebuggerUrl)
    left.sockets.add(ws)
    const pending = new Map()
    const problems = []
    let id = 0
    const send = (method, params = {}) => new Promise((resolve, reject) => {
        const current = ++id
        const timer = setTimeout(() => {
            pending.delete(current)
            reject(new Error(`${method} did not answer within 10 s`))
        }, 10000)
        pending.set(current, {resolve, reject, timer})
        ws.send(JSON.stringify({id: current, method, params}))
    })
    ws.addEventListener('message', event => {
        const message = JSON.parse(event.data)
        const waiting = pending.get(message.id)
        if (waiting) {
            clearTimeout(waiting.timer)
            pending.delete(message.id)
            if (message.error) waiting.reject(new Error(JSON.stringify(message.error)))
            else waiting.resolve(message.result)
        }
        if (message.method === 'Runtime.exceptionThrown') {
            const details = message.params.exceptionDetails
            problems.push(details.exception?.description ?? details.text)
        }
        if (message.method === 'Runtime.consoleAPICalled' && message.params.type === 'error') {
            problems.push(message.params.args.map(arg => arg.value ?? arg.description).join(' '))
        }
    })
    ws.addEventListener('close', () => {
        left.sockets.delete(ws)
        for (const entry of pending.values()) {
            clearTimeout(entry.timer)
            entry.reject(new Error('Chrome debugging connection closed'))
        }
        pending.clear()
    })
    await new Promise((resolve, reject) => {
        const timer = setTimeout(() => reject(new Error('Chrome WebSocket connection timed out')), 10000)
        ws.addEventListener('open', () => { clearTimeout(timer); resolve() }, {once: true})
        ws.addEventListener('error', () => { clearTimeout(timer); reject(new Error('Chrome WebSocket failed')) }, {once: true})
    })
    await send('Runtime.enable')
    await send('Page.enable')
    if (variant === 'asyncify') {
        await send('Page.addScriptToEvaluateOnNewDocument', {source: 'delete WebAssembly.Suspending'})
    }
    const observer = mode === 'idle' ? observeIdle : observeTurns
    if (mode !== 'loader') {
        await send('Page.addScriptToEvaluateOnNewDocument', {source: `(${observer.toString()})()`})
    }
    await send('Page.navigate', {url: `${base}/page.html?manifest=/build/${program}.variants.json`})
    const snapshot = async () => {
        const result = await send('Runtime.evaluate', {
            expression: '({report: window.report, timers: window.timers, turns: window.turns})',
            returnByValue: true
        })
        assert.ok(!result.exceptionDetails, JSON.stringify(result.exceptionDetails))
        assert.deepEqual(problems, [], 'Chrome reported an exception or console.error')
        const state = result.result.value
        assert.ok(!state.report?.error, state.report?.error)
        return state
    }
    return {
        snapshot,
        async waitFor(line) {
            const deadline = Date.now() + 20000
            while (Date.now() < deadline) {
                const state = await snapshot()
                if (state.report?.variant && state.report.lines.some(entry => line.test(entry.line))) {
                    assert.equal(state.report.variant, variant)
                    return state
                }
                await delay(25)
            }
            const last = await snapshot()
            throw new Error(`${program} (${variant}) did not complete ${line} within 20 s: ${JSON.stringify({lines: last.report?.lines, variant: last.report?.variant, timers: last.timers})}`)
        },
        async close() {
            ws.close()
            await fetch(`${devtools}/json/close/${target.id}`)
        }
    }
}

const renderStats = ({report, turns}, limited) => {
    const start = report.lines.find(entry => entry.line === 'spinning')
    const finish = report.lines.find(entry => /^done \d+$/.test(entry.line))
    assert.ok(start && finish && finish.at > start.at, 'missing render interval')
    const handlers = Number(finish.line.split(' ')[1])
    assert.ok(handlers > 0, 'no handlers ran')
    const frames = report.frames.filter(at => at > start.at && at < finish.at).length
    if (!limited) {
        assert.equal(frames, 0, 'the unlimited control yielded animation callbacks')
        assert.equal(turns.posted.length, 0, 'the unlimited control used the bounded yield')
        return {frames, handlers}
    }
    assert.ok(frames > 0, 'STOP: headless Chrome produced no animation callbacks; owner decision required')
    assert.ok(frames >= 10, `only ${frames} animation callbacks during the limited run`)
    assert.ok(turns.posted.length > 0, 'no bounded turns were observed')
    assert.equal(turns.posted.length, turns.resumed.length, 'a yielded turn never resumed')
    const durations = turns.posted.map((at, index) => at - (index ? turns.resumed[index - 1] : start.at))
    assert.ok(durations.every(value => value >= 0 && Number.isFinite(value)), 'invalid turn timestamps')
    durations.sort((a, b) => a - b)
    return {frames, handlers, turns: durations.length,
        p95: durations[Math.ceil(0.95 * durations.length) - 1], max: durations.at(-1)}
}

try {
    const base = await startServer()
    const devtools = await startChrome()
    for (const variant of ['jspi', 'asyncify']) {
        const programs = mode === 'render' ? ['render_program', 'render_unlimited_program']
            : [mode === 'idle' ? 'idle_program' : 'page_program']
        for (const program of programs) {
            const page = await openPage(devtools, base, program, variant)
            try {
                if (mode === 'loader') {
                    const {report} = await page.waitFor(/^ran 3 handlers$/)
                    assert.deepEqual(report.lines.map(entry => entry.line), ['ran 3 handlers'])
                    console.log(`loader ${variant}: ran 3 handlers`)
                } else if (mode === 'idle') {
                    await page.waitFor(/^idle$/)
                    await delay(3000)
                    const {report, timers} = await page.snapshot()
                    const start = report.lines.find(entry => entry.line === 'idle').at
                    const recorded = timers.filter(entry => entry.at >= start)
                    assert.equal(recorded.length, 1, JSON.stringify(recorded))
                    assert.equal(recorded[0].name, 'setTimeout', JSON.stringify(recorded))
                    assert.ok(recorded[0].delay > 0 && recorded[0].delay <= 300000, JSON.stringify(recorded))
                    console.log(`idle ${variant}: one setTimeout(${recorded[0].delay}), no polling in 3 s`)
                } else {
                    const limited = program === 'render_program'
                    const result = renderStats(await page.waitFor(/^done \d+$/), limited)
                    if (limited) {
                        console.log(`render ${variant}: ${result.frames} frames, ${result.handlers} handlers in 1 s, ${result.turns} turns, turn p95 ${result.p95.toFixed(3)} ms, max ${result.max.toFixed(3)} ms`)
                        assert.ok(result.p95 <= 8, `STOP: ${variant} turn p95 exceeds 8 ms; owner decision required`)
                    } else {
                        console.log(`control ${variant}: ${result.frames} frames, ${result.handlers} handlers in 1 s`)
                    }
                }
            } finally { await page.close() }
        }
    }
    console.log(`browser ${mode}: ok`)
} catch (error) {
    console.error(error)
    process.exitCode = 1
} finally {
    await cleanUp()
}
