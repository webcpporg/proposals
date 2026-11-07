// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {readFileSync} from 'node:fs'
import {dirname, join, resolve} from 'node:path'
import {fileURLToPath} from 'node:url'
import {startPeer} from './peer.mjs'
assert.equal(Number(process.versions.node.split('.')[0]), 26, 'the Beast Node contracts require Node 26')
const args = process.argv.slice(2)
let program = 'browser_test', expected = 'browser websocket and fetch: ok'
while (args[0]?.startsWith('--')) {
    const option = args.shift()
    if (option === '--program') program = args.shift()
    else if (option === '--expected') expected = args.shift()
    else throw new Error(`unknown option ${option}`)
}
assert.equal(args.length, 1)
const build = dirname(resolve(args[0]))
const manifest = JSON.parse(readFileSync(join(build, `${program}.variants.json`)))
const peer = await startPeer()
try {
    for (const variant of ['jspi', 'asyncify']) {
        const artifact = join(build, manifest[variant].script)
        const result = await new Promise((resolve, reject) => {
            const child = spawn(process.execPath, [join(dirname(fileURLToPath(import.meta.url)), 'node-entry.mjs'), artifact, String(peer.plain), String(peer.secure), expected], {env: {...process.env, NODE_EXTRA_CA_CERTS: peer.ca}, stdio: ['ignore', 'pipe', 'pipe']})
            let output = ''
            child.stdout.on('data', data => { output += data; process.stdout.write(data) })
            child.stderr.on('data', data => process.stderr.write(data))
            const timeout = setTimeout(() => child.kill('SIGKILL'), 20000)
            child.once('error', error => { clearTimeout(timeout); reject(error) })
            child.once('exit', (code, signal) => { clearTimeout(timeout); resolve({code, signal, output}) })
        })
        assert.equal(result.code, 0, `${program}/${variant}: ${result.signal ?? result.output}`)
        assert.ok(result.output.includes(expected))
        console.log(`node ${process.versions.node} ${variant}: ${program} passed`)
    }
} finally { await peer.stop() }
