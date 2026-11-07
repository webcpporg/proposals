// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Measure yield latency in a real browser, one fresh page per case.
// node run.mjs <cases.json> <output-directory>
// CHROME names the browser; on macOS the newest cached Playwright headless shell is used.
// Each invocation owns its loopback server, Chrome process and temporary profile.
import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {once} from 'node:events'
import {existsSync, mkdtempSync, readFileSync, readdirSync, rmSync, mkdirSync, writeFileSync} from 'node:fs'
import {createServer} from 'node:http'
import {homedir, tmpdir} from 'node:os'
import {dirname, extname, join, resolve, sep} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'
import {fileURLToPath} from 'node:url'

const fetchLocal = (url, options = {}) => fetch(url, {
    ...options, signal: AbortSignal.timeout(10000)
})
const args = process.argv.slice(2)
assert.equal(args.length, 2, 'usage: node run.mjs <cases.json> <output-directory>')
const config = JSON.parse(readFileSync(resolve(args[0]), 'utf8'))
const cases = config.cases
const output = resolve(args[1])
mkdirSync(output, {recursive: true})
const here = dirname(fileURLToPath(import.meta.url))
const build = resolve(config.build_root)
const mode = 'yield benchmark'
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
const roots = {'/build/': build, '/': here}
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
            const headers = {'Content-Type': types[extname(file)] ?? 'application/octet-stream', 'Cache-Control': 'no-store'}
            if (new URL(request.url, 'http://127.0.0.1').searchParams.get('isolate') === '1') {
                headers['Cross-Origin-Opener-Policy'] = 'same-origin'
                headers['Cross-Origin-Embedder-Policy'] = 'require-corp'
            }
            response.writeHead(200, headers)
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
                const version = await (await fetchLocal(`${devtools}/json/version`)).json()
                console.log(`browser ${mode}: ${version.Browser}`)
                return devtools
            } catch {}
        }
        await delay(50)
    }
    throw new Error(`Chrome startup timed out: ${said}`)
}


let version
async function runCase(devtools, base, item) {
    const target = await (await fetchLocal(`${devtools}/json/new?about:blank`, {method: 'PUT'})).json()
    const ws = new WebSocket(target.webSocketDebuggerUrl)
    left.sockets.add(ws)
    const pending = new Map()
    const errors = []
    let id = 0
    await new Promise((resolve, reject) => {
        const timer = setTimeout(() => reject(new Error('WebSocket open timed out')), 10000)
        ws.addEventListener('open', () => { clearTimeout(timer); resolve() }, {once: true})
        ws.addEventListener('error', error => { clearTimeout(timer); reject(error) }, {once: true})
    })
    const send = (method, params = {}) => new Promise((resolve, reject) => {
        const current = ++id
        const timer = setTimeout(() => { pending.delete(current); reject(new Error(`${method} timed out`)) }, 60000)
        pending.set(current, {resolve, reject, timer})
        ws.send(JSON.stringify({id: current, method, params}))
    })
    ws.addEventListener('message', event => {
        const message = JSON.parse(event.data)
        const entry = pending.get(message.id)
        if (entry) {
            clearTimeout(entry.timer)
            pending.delete(message.id)
            if (message.error) entry.reject(new Error(JSON.stringify(message.error)))
            else entry.resolve(message.result)
        }
        if (message.method === 'Runtime.exceptionThrown') errors.push(message.params.exceptionDetails)
        if (message.method === 'Runtime.consoleAPICalled' && message.params.type === 'error') errors.push(message.params)
    })
    try {
        await send('Runtime.enable')
        await send('Page.enable')
        await send('Page.bringToFront')
        await send('Page.addScriptToEvaluateOnNewDocument', {source: `
            let resolve; let value;
            window.benchmarkPromise = new Promise(done => { resolve = done });
            Object.defineProperty(window, 'benchmark', {get: () => value, set: next => { value = next; if (next?.done) resolve(next) }});
        `})
        const query = new URLSearchParams(Object.fromEntries(Object.entries(item).map(([k,v]) => [k,String(v)])))
        const loaded = new Promise((resolve, reject) => {
            const timer = setTimeout(() => reject(new Error("page load timed out")), 60000)
            const onLoad = event => { if (JSON.parse(event.data).method === 'Page.loadEventFired') { ws.removeEventListener('message', onLoad); clearTimeout(timer); resolve() } };
            ws.addEventListener('message', onLoad);
        });
        await send('Page.navigate', {url: `${base}/page.html?${query}`});
        await loaded
        const observed = await send('Runtime.evaluate', {expression: 'window.benchmarkPromise', awaitPromise: true, returnByValue: true})
        assert.ok(!observed.exceptionDetails, JSON.stringify(observed.exceptionDetails))
        const data = observed.result.value
        assert.ok(data && !data.error, data?.error)
        assert.deepEqual(errors, [], JSON.stringify(errors))
        assert.equal(data.result.environment.isolated, item.isolate === 1)
        assert.equal(data.result.environment.visibility, 'visible')
        assert.equal(data.result.status, 0)
        data.result.case_id = item.id
        data.result.host_duration_ms = performance.now() - item.host_started
        writeFileSync(join(output, `${item.id}.json`), JSON.stringify(data.result))
        console.log(`${item.id}: ${data.result.operations} operations, ${data.result.elapsed_ms.toFixed(3)} ms, ${data.result.pairs.length / 2} samples`)
    } finally {
        ws.close()
        left.sockets.delete(ws)
        for (const entry of pending.values()) { clearTimeout(entry.timer); entry.reject(new Error('page closed')) }
        pending.clear()
        await fetchLocal(`${devtools}/json/close/${target.id}`)
    }
}

try {
    const base = await startServer()
    const devtools = await startChrome()
    version = await (await fetchLocal(`${devtools}/json/version`)).json()
    writeFileSync(join(output, 'environment.json'), JSON.stringify({version, started: new Date().toISOString(), config}, null, 2))
    for (const item of cases) {
        console.log(`START ${item.id}`)
        await runCase(devtools, base, {...item, host_started: performance.now()})
    }
    console.log(`Completed ${cases.length} cases.`)
} catch (error) {
    console.error(error)
    process.exitCode = 1
} finally {
    await cleanUp()
}
