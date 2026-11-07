// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The real browser checks: idle waits, animation opportunities and variant loading.
// node run.mjs --mode <idle|render|loader> <program>
// CHROME names full Chromium; on macOS the newest cached Playwright Chromium is used.
// Each invocation owns its loopback server, Chrome process and temporary profile.
import assert from 'node:assert/strict'
import {spawn, execFileSync} from 'node:child_process'
import {once} from 'node:events'
import {existsSync, mkdtempSync, readFileSync, readdirSync, rmSync} from 'node:fs'
import {createServer} from 'node:http'
import {createServer as createSecureServer} from 'node:https'
import {createHash} from 'node:crypto'
import {homedir, tmpdir} from 'node:os'
import {dirname, extname, join, relative, resolve, sep} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'
import {fileURLToPath} from 'node:url'

const isMain = process.argv[1] === fileURLToPath(import.meta.url)
const args = isMain ? process.argv.slice(2) : ["."]
let program = 'browser_test'
let expected = 'browser websocket and fetch: ok'
while (args[0]?.startsWith('--')) {
    const option = args.shift()
    if (option === '--program') program = args.shift()
    else if (option === '--expected') expected = args.shift()
    else throw new Error(`unknown option ${option}`)
}
assert.equal(args.length, 1, 'usage: run.mjs [--program name] [--expected line] <artifact>')
const mode = program
const here = dirname(fileURLToPath(import.meta.url))
const artifact = resolve(args[0])
const manifest = join(dirname(artifact).replace(`${sep}exception-handling-off`, ''),
    `${program}.variants.json`)
let build = dirname(artifact)
while (!manifest.startsWith(build + sep) && dirname(build) !== build) build = dirname(build)
const manifestUrl = `/build/${relative(build, manifest).split(sep).join('/')}`
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

const left = {chrome: null, profile: null, server: null, corsServer: null, secureServer: null, certificate: null, sockets: new Set(), networkSockets: new Set()}
const ownConnections = server => server.on('connection', socket => {
    left.networkSockets.add(socket)
    socket.on('close', () => left.networkSockets.delete(socket))
})
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
    for (const socket of left.networkSockets) socket.destroy()
    if (left.server) {
        left.server.closeAllConnections()
        await new Promise(resolve => left.server.close(resolve))
    }
    if (left.corsServer) {
        left.corsServer.closeAllConnections()
        await new Promise(resolve => left.corsServer.close(resolve))
    }
    if (left.secureServer) {
        left.secureServer.closeAllConnections()
        await new Promise(resolve => left.secureServer.close(resolve))
    }
    if (left.certificate) rmSync(left.certificate, {recursive: true, force: true})
    if (left.profile) rmSync(left.profile, {recursive: true, force: true, maxRetries: 3})
})())
for (const [signal, code] of [['SIGHUP', 129], ['SIGINT', 130], ['SIGTERM', 143]]) {
    process.on(signal, () => cleanUp().finally(() => process.exit(code)))
}

const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript',
    '.wasm': 'application/wasm', '.json': 'application/json'}
const roots = {'/build/': build, '/loader/': resolve(here, '../../../../asio/include/webcpp/asio'), '/': here}
let certificateAuthority
const trustHelper = join(here, '../host-tls/trust.py')
const trust = (...args) => execFileSync(process.env.PYTHON ?? 'python3', [trustHelper, ...args],
    {encoding: 'utf8', timeout: 30000})
const startServer = async () => {
    left.certificate = mkdtempSync(join(tmpdir(), 'beast-browser-tls-'))
    const certificates = JSON.parse(trust('create', left.certificate))
    certificateAuthority = certificates.der
    const fixture = (request, response) => {
        response.setHeader('Access-Control-Allow-Origin', '*')
        response.setHeader('Access-Control-Allow-Methods', 'GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS')
        response.setHeader('Access-Control-Allow-Headers', 'content-type')
        if (request.method === 'OPTIONS') { response.writeHead(204).end(); return }
        const chunks = []
        request.on('data', data => chunks.push(data))
        request.on('end', () => {
            response.writeHead(200,
                {'Content-Type': request.headers['content-type'] ?? 'text/plain'})
            console.log(JSON.stringify({fixture: request.method, target: request.url,
                headers: response.getHeaders()}))
            response.end(request.method === 'GET' ? 'fetch body' : Buffer.concat(chunks))
        })
    }
    left.secureServer = createSecureServer({
        key: readFileSync(join(left.certificate, 'key.pem')),
        cert: readFileSync(join(left.certificate, 'cert.pem'))
    }, fixture)
    ownConnections(left.secureServer)
    left.secureServer.listen(0, '127.0.0.1')
    await once(left.secureServer, 'listening')
    const securePort = String(left.secureServer.address().port)
    left.corsServer = createServer((request, response) => response.writeHead(200).end('cross origin'))
    ownConnections(left.corsServer)
    left.corsServer.listen(0, '127.0.0.1')
    await once(left.corsServer, 'listening')
    const corsPort = String(left.corsServer.address().port)
    const server = createServer((request, response) => {
        if (request.url === '/redirect') { response.writeHead(302, {Location: '/http'}).end(); return }
        if (['/', '/http', '/get', '/post', '/put', '/patch', '/delete'].includes(request.url)) {
            fixture(request, response)
            return
        }
        try {
            const path = decodeURIComponent(new URL(request.url, 'http://127.0.0.1').pathname)
            const [prefix, root] = Object.entries(roots).find(([prefix]) => path.startsWith(prefix))
            const file = resolve(root, path.slice(prefix.length))
            if (!file.startsWith(root + sep)) {
                response.writeHead(404).end()
                return
            }
            let content = readFileSync(file)
            if (path === '/page.html') content = Buffer.from(content.toString().replace('<script type="module">', `<script>globalThis.beastCorsPort = '${corsPort}'; globalThis.beastSecurePort = '${securePort}'</script><script type="module">`))
            response.writeHead(200, {'Content-Type': types[extname(file)] ?? 'application/octet-stream'})
                .end(content)
        } catch {
            response.writeHead(404).end()
        }
    })
    const upgrade = (request, socket) => {
        const key = request.headers['sec-websocket-key']
        const accept = createHash('sha1').update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64')
        const protocol = request.headers['sec-websocket-protocol']?.split(',')[0].trim()
        socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + accept + '\r\n' +
            (protocol ? 'Sec-WebSocket-Protocol: ' + protocol + '\r\n' : '') + '\r\n')
        let pending = Buffer.alloc(0)
        socket.on('error', () => {})
        socket.on('data', data => {
            pending = Buffer.concat([pending, data])
            while (pending.length >= 2) {
                const opcode = pending[0] & 15
                let size = pending[1] & 127
                let offset = 2
                if (size === 126) { if (pending.length < 4) return; size = pending.readUInt16BE(2); offset = 4 }
                if (size === 127) { if (pending.length < 10) return; size = Number(pending.readBigUInt64BE(2)); offset = 10 }
                if (size > 8 * 1024 * 1024) { socket.destroy(); return }
                const masked = pending[1] & 128
                const mask = pending.subarray(offset, offset + 4)
                if (masked) offset += 4
                if (pending.length < offset + size) return
                const payload = Buffer.from(pending.subarray(offset, offset + size))
                if (masked) for (let i = 0; i < size; ++i) payload[i] ^= mask[i % 4]
                pending = pending.subarray(offset + size)
                let header
                if (size < 126) header = Buffer.from([128 | opcode, size])
                else if (size < 65536) { header = Buffer.alloc(4); header[0] = 128 | opcode; header[1] = 126; header.writeUInt16BE(size, 2) }
                else { header = Buffer.alloc(10); header[0] = 128 | opcode; header[1] = 127; header.writeBigUInt64BE(BigInt(size), 2) }
                socket.write(Buffer.concat([header, payload]))
                if (opcode === 8) socket.end()
            }
        })
    }
    ownConnections(server)
    server.on('upgrade', upgrade)
    left.secureServer.on('upgrade', upgrade)
    left.server = server
    server.listen(0, '127.0.0.1')
    await once(server, 'listening')
    return `http://127.0.0.1:${server.address().port}`
}

const startChrome = async (trusted = true) => {
    left.profile = mkdtempSync(join(tmpdir(), `asio-browser-${process.pid}-`))
    if (trusted && certificateAuthority) trust('chrome', left.profile, certificateAuthority)
    const executable = findChrome()
    const chrome = spawn(executable, [
        '--headless=new', '--remote-debugging-address=127.0.0.1', '--remote-debugging-port=0',
        `--user-data-dir=${left.profile}`, '--no-first-run', '--no-default-browser-check',
        '--disable-component-update', '--disable-background-networking', '--disable-sync',
        '--host-resolver-rules=MAP localhost 127.0.0.1, MAP * ~NOTFOUND, EXCLUDE 127.0.0.1',
        'about:blank'
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

const openPage = async (devtools, base, program, variant, url) => {
    const target = await (await fetch(`${devtools}/json/new?about:blank`, {method: 'PUT'})).json()
    const ws = new WebSocket(target.webSocketDebuggerUrl)
    left.sockets.add(ws)
    const pending = new Map()
    const problems = []
    const networkErrors = []
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
        if (message.method === 'Network.loadingFailed') networkErrors.push(message.params.errorText)
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
    await send('Page.navigate', {url: url ?? `${base}/page.html?manifest=${manifestUrl}`})
    const snapshot = async () => {
        const result = await send('Runtime.evaluate', {
            expression: '({report: window.report, timers: window.timers, turns: window.turns})',
            returnByValue: true
        })
        assert.ok(!result.exceptionDetails, JSON.stringify(result.exceptionDetails))
        assert.deepEqual(problems, [], 'Chrome reported an exception or console.error')
        const state = result.result.value
        assert.ok(!state.report?.error, state.report?.error)
        assert.deepEqual(state.report?.stderr?.filter(line => !/^Progress:/.test(line)) ?? [], [],
            'application reported an unexpected error: ' + JSON.stringify(state.report))
        return state
    }
    return {
        snapshot,
        networkErrors,
        async evaluate(expression) {
            const result = await send('Runtime.evaluate', {
                expression, returnByValue: true, awaitPromise: true
            })
            assert.ok(!result.exceptionDetails, JSON.stringify(result.exceptionDetails))
            assert.deepEqual(problems, [], 'Chrome reported an exception or console.error')
            return result.result.value
        },
        async screenshot() {
            const {cssContentSize} = await send('Page.getLayoutMetrics')
            return (await send('Page.captureScreenshot', {captureBeyondViewport: true,
                clip: {...cssContentSize, scale: 1}})).data
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
            await fetch(`${devtools}/json/close/${target.id}`)
        }
    }
}

if (isMain) {
try {
    const base = await startServer()
    let untrustedRejected = null
    let wrongNameRejected = null
    if (program.includes('ssl')) {
        const unpinned = await startChrome(false)
        const control = await openPage(unpinned, base, program, 'jspi', `${base}/http`)
        try {
            const deadline = Date.now() + 5000
            while (!await control.evaluate('document.readyState === "complete"')) {
                assert.ok(Date.now() < deadline, 'TLS control page did not load')
                await delay(25)
            }
            const endpoint = `https://localhost:${left.secureServer.address().port}/http`
            untrustedRejected = await control.evaluate(
                `fetch(${JSON.stringify(endpoint)}).then(() => false, () => true)`)
            assert.equal(untrustedRejected, true, 'untrusted TLS fixture was accepted')
            assert.ok(control.networkErrors.includes('net::ERR_CERT_AUTHORITY_INVALID'),
                `expected an authority failure: ${control.networkErrors}`)
        } finally {
            await control.close()
            await stopChrome()
        }
    }
    const devtools = await startChrome()
    if (program.includes('ssl')) {
        const control = await openPage(devtools, base, program, 'jspi', `${base}/http`)
        try {
            const endpoint = `https://localhost:${left.secureServer.address().port}/http`
            assert.equal(await control.evaluate(`fetch(${JSON.stringify(endpoint)})`
                + '.then(response => response.text())'), 'fetch body', 'trusted TLS peer failed')
            wrongNameRejected = await control.evaluate(
                `fetch(${JSON.stringify(endpoint.replace('localhost', '127.0.0.1'))})`
                + '.then(() => false, () => true)')
            assert.equal(wrongNameRejected, true, 'trusted certificate accepted the wrong hostname')
            assert.ok(control.networkErrors.includes('net::ERR_CERT_COMMON_NAME_INVALID'),
                `expected a hostname failure: ${control.networkErrors}`)
        } finally { await control.close() }
    }
    for (const variant of ['jspi', 'asyncify']) {
        const page = await openPage(devtools, base, program, variant)
        try {
            const {report} = await page.waitFor(new RegExp('^' + expected.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '$'))
            assert.ok(report.lines.some(entry => entry.line === expected))
            console.log(JSON.stringify({application: program, backend: variant, result: 'passed',
                tls_fixture: {mode: 'isolated-profile-ca', untrusted_rejected: untrustedRejected,
                    wrong_name_rejected: wrongNameRejected, trust_and_hostname_verified:
                    untrustedRejected === true && wrongNameRejected === true},
                output: report.lines.map(entry => entry.line)}))
        } finally { await page.close() }
    }
} catch (error) {
    console.error(error)
    process.exitCode = 1
} finally { await cleanUp() }

}
export {startChrome, openPage, cleanUp}
