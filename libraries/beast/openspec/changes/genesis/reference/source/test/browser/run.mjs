// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Execute Beast contracts or tutorial variants against controlled HTTP and WebSocket peers.
// node run.mjs [--program name] [--expected line] [--environment] <artifact>
// CHROME names full Chromium; on macOS the newest cached Playwright Chromium is used.
// Each invocation owns its loopback server, Chrome process and temporary profile.
import assert from 'node:assert/strict'
import {execFileSync, spawn} from 'node:child_process'
import {once} from 'node:events'
import {existsSync, mkdtempSync, readFileSync, readdirSync, rmSync} from 'node:fs'
import {createServer} from 'node:http'
import {createServer as createSecureServer} from 'node:https'
import {startPeer} from './peer.mjs'
import {homedir, tmpdir} from 'node:os'
import {dirname, extname, join, resolve, sep} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'
import {fileURLToPath} from 'node:url'

const args = process.argv.slice(2)
let program = 'browser_test'
let expected = 'browser websocket and fetch: ok'
let environment = false
while (args[0]?.startsWith('--')) {
    const option = args.shift()
    if (option === '--program') program = args.shift()
    else if (option === '--expected') expected = args.shift()
    else if (option === '--environment') environment = true
    else throw new Error(`unknown option ${option}`)
}
assert.equal(args.length, 1, 'usage: run.mjs [--program name] [--expected line] [--environment] <artifact>')
const mode = 'beast'
const here = dirname(fileURLToPath(import.meta.url))
const build = dirname(resolve(args[0]))
const findChrome = () => {
    if (process.env.CHROME) return process.env.CHROME
    assert.equal(process.platform, 'darwin', 'set CHROME to the Chrome executable')
    const cache = join(homedir(), 'Library/Caches/ms-playwright')
    const shells = existsSync(cache) ? readdirSync(cache)
        .filter(name => /^chromium-\d+$/.test(name))
        .sort((a, b) => Number(b.split('-').at(-1)) - Number(a.split('-').at(-1))) : []
    for (const shell of shells) {
        for (const arch of ['arm64', 'x64']) {
            const path = join(cache, shell, `chrome-mac-${arch}`, 'Google Chrome for Testing.app',
                'Contents/MacOS/Google Chrome for Testing')
            if (existsSync(path)) return path
        }
    }
    throw new Error('no cached full Chromium found; set CHROME')
}

const left = {chrome: null, profile: null, server: null, secureServer: null, secureOrigin: null, corsServer: null, peer: null, sockets: new Set()}
const exited = child => child.exitCode !== null || child.signalCode !== null
const signalChrome = (child, signal) => {
    if (!child.pid) return
    try {
        if (process.platform === 'win32') child.kill(signal)
        else process.kill(-child.pid, signal)
    } catch (error) {
        if (error.code !== 'ESRCH' && !(error.code === 'EPERM' && exited(child))) throw error
    }
}
const stopChrome = async () => {
    if (left.chrome && !exited(left.chrome) && left.chrome.pid) {
        const ended = once(left.chrome, 'exit')
        const stubborn = setTimeout(() => signalChrome(left.chrome, 'SIGKILL'), 5000)
        signalChrome(left.chrome, 'SIGTERM')
        try { await ended } finally { clearTimeout(stubborn) }
    }
    // Chrome helpers may retain an inherited stderr pipe after the main process exits.
    if (left.chrome) {
        signalChrome(left.chrome, 'SIGKILL')
        left.chrome.stderr.destroy()
    }
    left.chrome = null
    if (left.profile) rmSync(left.profile, {recursive: true, force: true, maxRetries: 3})
    left.profile = null
}
let cleaning
const cleanUp = () => (cleaning ??= (async () => {
    for (const socket of left.sockets) socket.close()
    await stopChrome()
    if (left.peer) await left.peer.stop()
    if (left.server) {
        left.server.closeAllConnections()
        await new Promise(resolve => left.server.close(resolve))
    }
    if (left.secureServer) {
        left.secureServer.closeAllConnections()
        await new Promise(resolve => left.secureServer.close(resolve))
    }
    if (left.corsServer) {
        left.corsServer.closeAllConnections()
        await new Promise(resolve => left.corsServer.close(resolve))
    }
})())
for (const [signal, code] of [['SIGHUP', 129], ['SIGINT', 130], ['SIGTERM', 143]]) {
    process.on(signal, () => cleanUp().finally(() => process.exit(code)))
}

const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript',
    '.wasm': 'application/wasm', '.json': 'application/json'}
const roots = {'/build/': build, '/loader/': resolve(here, '../../../asio/include/webcpp/asio'), '/': here}
const startServer = async () => {
    left.peer = await startPeer()
    left.corsServer = createServer((request, response) => response.writeHead(200).end('cross origin'))
    left.corsServer.listen(0, '127.0.0.1')
    await once(left.corsServer, 'listening')
    const corsPort = String(left.corsServer.address().port)
    const serve = (request, response) => {
        try {
            const path = decodeURIComponent(new URL(request.url, 'http://127.0.0.1').pathname)
            if (path === '/tls-control') {
                response.writeHead(200, {'Content-Type': 'text/html'}).end('<title>Beast TLS control</title>')
                return
            }
            const [prefix, root] = Object.entries(roots).find(([prefix]) => path.startsWith(prefix))
            const file = resolve(root, path.slice(prefix.length))
            if (!file.startsWith(root + sep)) { response.writeHead(404).end(); return }
            let content = readFileSync(file)
            if (path === '/page.html') content = Buffer.from(content.toString().replace('<script type="module">', `<script>globalThis.beastCorsPort = '${corsPort}'; globalThis.beastPeerPort = '${left.peer.plain}'; globalThis.beastSecurePort = '${left.peer.secure}'</script><script type="module">`))
            response.writeHead(200, {'Content-Type': types[extname(file)] ?? 'application/octet-stream'}).end(content)
        } catch { response.writeHead(404).end() }
    }
    const server = createServer(serve)
    if (environment) {
        left.secureServer = createSecureServer({
            key: readFileSync(left.peer.key), cert: readFileSync(left.peer.cert)
        }, serve)
        left.secureServer.listen(0, '127.0.0.1')
        await once(left.secureServer, 'listening')
        left.secureOrigin = `https://127.0.0.1:${left.secureServer.address().port}`
    }
    left.server = server
    server.listen(0, '127.0.0.1')
    await once(server, 'listening')
    return `http://127.0.0.1:${server.address().port}`
}

const startChrome = async (trusted = true) => {
    left.profile = mkdtempSync(join(tmpdir(), `beast-browser-${process.pid}-`))
    if (trusted) {
        execFileSync(process.env.PYTHON ?? 'python3',
            [join(here, '../applications/host-tls/trust.py'), 'chrome', left.profile, left.peer.caDer],
            {encoding: 'utf8', timeout: 30000})
    }
    const executable = findChrome()
    const chrome = spawn(executable, [
        '--headless=new', '--remote-debugging-address=127.0.0.1', '--remote-debugging-port=0',
        `--user-data-dir=${left.profile}`, '--no-first-run', '--no-default-browser-check',
        '--disable-component-update', '--disable-background-networking', '--disable-sync',
        '--host-resolver-rules=MAP mixed.beast.test 127.0.0.1, MAP * ~NOTFOUND, EXCLUDE 127.0.0.1', 'about:blank'
    ], {stdio: ['ignore', 'ignore', 'pipe'], detached: process.platform !== 'win32'})
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
                const version = await (await fetch(`${devtools}/json/version`, {signal: AbortSignal.timeout(10000)})).json()
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

const openPage = async (devtools, base, program, variant, url = `${base}/page.html?manifest=/build/${program}.variants.json`) => {
    const target = await (await fetch(`${devtools}/json/new?about:blank`, {method: 'PUT', signal: AbortSignal.timeout(10000)})).json()
    const ws = new WebSocket(target.webSocketDebuggerUrl)
    left.sockets.add(ws)
    const pending = new Map()
    const problems = []
    const networkErrors = []
    const networkFailures = []
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
        if (message.method === 'Network.loadingFailed') {
            networkFailures.push(message.params)
        }
        if (message.method === 'Network.webSocketFrameError') {
            networkErrors.push(message.params.errorMessage)
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
    await send('Network.enable')
    if (variant === 'asyncify') {
        await send('Page.addScriptToEvaluateOnNewDocument', {source: 'delete WebAssembly.Suspending'})
    }
    await send('Page.navigate', {url})
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
        networkErrors,
        networkFailures,
        snapshot,
        async evaluate(expression) {
            const result = await send('Runtime.evaluate', {expression, awaitPromise: true, returnByValue: true})
            assert.ok(!result.exceptionDetails, JSON.stringify(result.exceptionDetails))
            return result.result.value
        },
        async waitFor(line) {
            const deadline = Date.now() + 20000
            while (Date.now() < deadline) {
                const state = await snapshot()
                if (state.report?.lines.some(entry => /^check failed|^error:/.test(entry.line))) throw new Error(JSON.stringify(state.report.lines))
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
            const response = await fetch(`${devtools}/json/close/${target.id}`, {signal: AbortSignal.timeout(10000)})
            await response.text()
        }
    }
}

try {
    const base = await startServer()
    const untrusted = await startChrome(false)
    const control = await openPage(untrusted, base, program, 'jspi', `${base}/tls-control`)
    try {
        const deadline = Date.now() + 5000
        while (!await control.evaluate('document.readyState === "complete"')) {
            assert.ok(Date.now() < deadline, 'TLS control page did not load')
            await delay(25)
        }
        if (environment) {
            const plain = `http://mixed.beast.test:${left.peer.plain}/http`
            const body = await control.evaluate(`fetch(${JSON.stringify(plain)}).then(response => response.text())`)
            assert.equal(body, 'fetch body', 'the mixed-content destination must work from an HTTP page')
            console.log('browser mixed-content control: HTTP destination reachable from HTTP origin')
        }
        const endpoint = `wss://127.0.0.1:${left.peer.secure}/echo`
        const result = await control.evaluate(`new Promise(resolve => {
            const socket = new WebSocket(${JSON.stringify(endpoint)});
            const timer = setTimeout(() => { socket.close(); resolve('timeout'); }, 5000);
            socket.onopen = () => { clearTimeout(timer); socket.close(); resolve('accepted'); };
            socket.onerror = () => { clearTimeout(timer); resolve('rejected'); };
        })`)
        assert.equal(result, 'rejected', 'WSS accepted a certificate without the fixture CA')
        assert.ok(control.networkErrors.some(error => error.includes('ERR_CERT_AUTHORITY_INVALID')),
            `expected a WSS authority failure: ${control.networkErrors}`)
        console.log('browser WSS: untrusted fixture rejected with ERR_CERT_AUTHORITY_INVALID')
    } finally {
        await control.close()
        await stopChrome()
    }
    const devtools = await startChrome()
    for (const variant of ['jspi', 'asyncify']) {
        const page = await openPage(devtools, environment ? left.secureOrigin : base, program, variant)
        try {
            const {report} = await page.waitFor(new RegExp('^' + expected.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '$'))
            assert.ok(report.lines.some(entry => entry.line === expected))
            if (environment) {
                for (const operation of ['http', 'websocket']) {
                    const start = report.lines.find(entry => entry.line === `heartbeat ${operation}: start`)
                    const finish = report.lines.find(entry => entry.line === `heartbeat ${operation}: finish`)
                    assert.ok(start && finish, `${operation} synchronous wait interval was not reported`)
                    const duration = finish.at - start.at
                    assert.ok(duration >= 250 && duration < 3000,
                        `${operation} native peer release took ${duration} ms`)
                    const frames = report.frames.filter(at => at > start.at && at < finish.at).length
                    assert.ok(frames >= 3, `only ${frames} animation callbacks during ${operation} synchronous wait`)
                    console.log(`browser ${variant}: ${operation} synchronous read yielded ${frames} animation callbacks`)
                }
                assert.ok(page.networkFailures.some(failure => failure.blockedReason === 'mixed-content'),
                    `Chrome did not identify the mixed-content block: ${JSON.stringify(page.networkFailures)}`)
                console.log(`browser ${variant}: nested entry rejected; mixed content blocked`)
            }
            console.log(`browser ${variant}: WebSocket and fetch contracts passed with isolated-profile CA trust`)
        } finally { await page.close() }
    }
} catch (error) {
    console.error(error)
    process.exitCode = 1
} finally { await cleanUp() }
