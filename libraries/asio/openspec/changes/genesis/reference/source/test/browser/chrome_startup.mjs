// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Browser startup has its own deadline inside the driver's overall bound.
import assert from 'node:assert/strict'
import {existsSync, readFileSync, readdirSync} from 'node:fs'
import {homedir} from 'node:os'
import {join} from 'node:path'
import {setTimeout as delay} from 'node:timers/promises'

export const findChrome = () => {
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

export const waitForChrome = async (chrome, profile, {timeout = 30000, requestTimeout = 1000} = {}) => {
    const stopped = new AbortController()
    let failure
    let said = ''
    let stage = 'waiting for DevToolsActivePort (file has not appeared)'
    let lastResponse = '(none)'
    const onError = error => {
        failure = `Chrome failed to start: ${error.message}`
        stopped.abort()
    }
    const onExit = (code, signal) => {
        failure = `Chrome exited during startup (code ${code}, signal ${signal})`
        stopped.abort()
    }
    const onData = data => { said = (said + data).slice(-8000) }
    chrome.on('error', onError)
    chrome.on('exit', onExit)
    chrome.stderr?.on('data', onData)
    if (chrome.exitCode !== null || chrome.signalCode !== null) {
        onExit(chrome.exitCode, chrome.signalCode)
    }
    const deadline = performance.now() + timeout
    try {
        while (!failure && performance.now() < deadline) {
            let port = ''
            try {
                port = readFileSync(join(profile, 'DevToolsActivePort'), 'utf8').split('\n')[0]
                stage = `waiting for valid DevToolsActivePort (read ${JSON.stringify(port.slice(0, 128))})`
            } catch (error) {
                if (error.code !== 'ENOENT') throw error
            }
            if (/^\d+$/.test(port) && Number(port) > 0 && Number(port) <= 65535) {
                const devtools = `http://127.0.0.1:${port}`
                const endpoint = `${devtools}/json/version`
                try {
                    const remaining = Math.max(1, Math.ceil(deadline - performance.now()))
                    const signal = AbortSignal.any([stopped.signal,
                        AbortSignal.timeout(Math.min(requestTimeout, remaining))])
                    const response = await fetch(endpoint, {signal})
                    lastResponse = `HTTP ${response.status} ${response.statusText}`
                    if (!response.ok) throw new Error(`HTTP ${response.status} ${response.statusText}`)
                    const version = await response.json()
                    if (typeof version.Browser !== 'string' || !version.Browser) {
                        throw new Error('missing Browser in discovery response')
                    }
                    if (!failure) return {devtools, version}
                } catch (error) {
                    const cause = error.cause?.code ? ` (${error.cause.code})` : ''
                    stage = `${endpoint}: ${error.name}: ${error.message}${cause}`
                }
            }
            if (!failure && performance.now() < deadline) {
                await delay(Math.min(50, deadline - performance.now()), undefined,
                    {signal: stopped.signal}).catch(error => {
                    if (error.name !== 'AbortError') throw error
                })
            }
        }
        throw new Error(`${failure ?? `Chrome startup timed out after ${timeout} ms`}; ` +
            `last stage: ${stage}; last discovery response: ${lastResponse}; ` +
            `stderr: ${said || '(empty)'}`)
    } finally {
        chrome.off('error', onError)
        chrome.off('exit', onExit)
        chrome.stderr?.off('data', onData)
    }
}
