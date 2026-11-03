// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Native Beast owns HTTP and WebSocket protocols; Node only launches and reaps it.
import assert from 'node:assert/strict'
import {spawn, spawnSync} from 'node:child_process'
import {once} from 'node:events'
import {existsSync, mkdtempSync, readdirSync, rmSync, writeFileSync} from 'node:fs'
import {tmpdir} from 'node:os'
import {join, resolve} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'

function executables(directory) {
    if (!existsSync(directory)) return []
    return readdirSync(directory, {withFileTypes: true}).flatMap(entry => {
        const path = join(directory, entry.name)
        return entry.isDirectory() ? executables(path) : entry.name === 'native_peer-program' || entry.name === 'native_peer-program.exe' ? [path] : []
    })
}

export async function startPeer() {
    const executable = process.env.WEBCPP_BEAST_NATIVE_PEER ?? executables(resolve('bin/libs/beast/test/browser/peer'))[0]
    assert.ok(executable, 'build libs/beast/test/browser/peer//native_peer-program with a native toolset, or set WEBCPP_BEAST_NATIVE_PEER')
    const directory = mkdtempSync(join(tmpdir(), 'beast-peer-'))
    let child
    const stop = async () => {
        if (child?.pid && child.exitCode === null && child.signalCode === null) {
            const ended = once(child, 'exit')
            const deadline = setTimeout(() => child.kill('SIGKILL'), 3000)
            child.kill('SIGTERM')
            try { await ended } finally { clearTimeout(deadline) }
        }
        child?.stdout.destroy()
        child?.stderr.destroy()
        rmSync(directory, {recursive: true, force: true})
    }
    try {
        const openssl = (...args) => {
            const result = spawnSync('openssl', args, {cwd: directory, encoding: 'utf8', timeout: 10000})
            assert.equal(result.status, 0, result.stderr)
            return result.stdout
        }
        writeFileSync(join(directory, 'ca.cnf'), '[req]\ndistinguished_name=dn\n[dn]\nCN=Beast contract CA\n[v3_ca]\nbasicConstraints=critical,CA:true\nkeyUsage=critical,keyCertSign,cRLSign\nsubjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid:always\n')
        openssl('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '2', '-subj', '/CN=Beast contract CA', '-keyout', 'ca.key', '-out', 'ca.crt', '-config', 'ca.cnf', '-extensions', 'v3_ca')
        writeFileSync(join(directory, 'leaf.cnf'), 'subjectAltName=DNS:localhost,IP:127.0.0.1\nextendedKeyUsage=serverAuth\nbasicConstraints=critical,CA:false\nkeyUsage=critical,digitalSignature,keyEncipherment\n')
        openssl('req', '-newkey', 'rsa:2048', '-nodes', '-subj', '/CN=localhost', '-keyout', 'server.key', '-out', 'server.csr')
        openssl('x509', '-req', '-in', 'server.csr', '-CA', 'ca.crt', '-CAkey', 'ca.key', '-CAcreateserial', '-days', '2', '-extfile', 'leaf.cnf', '-out', 'server.crt')
        openssl('x509', '-in', 'ca.crt', '-outform', 'DER', '-out', 'ca.der')
        child = spawn(executable, [join(directory, 'server.crt'), join(directory, 'server.key')], {stdio: ['ignore', 'pipe', 'pipe']})
        let output = '', errors = '', launchError
        child.on('error', error => { launchError = error })
        child.stdout.on('data', value => { output += value })
        child.stderr.on('data', value => { errors += value })
        const deadline = Date.now() + 10000
        while (!output.includes('\n') && Date.now() < deadline && child.exitCode === null && !launchError) await delay(10)
        if (launchError) throw launchError
        assert.ok(output.includes('\n'), `native peer failed to announce ports: ${errors}`)
        return {...JSON.parse(output.split('\n')[0]), ca: join(directory, 'ca.crt'), caDer: join(directory, 'ca.der'), cert: join(directory, 'server.crt'), key: join(directory, 'server.key'), stop}
    } catch (error) { await stop(); throw error }
}
