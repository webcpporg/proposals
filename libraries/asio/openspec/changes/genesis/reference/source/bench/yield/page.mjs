// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

const q = new URLSearchParams(location.search)
const mode = q.get('mode')
const samples = Number(q.get('samples'))
const traceEnabled = q.get('trace') === '1'
const injectedDelayMs = Number(q.get('delay_us') ?? 0) / 1000
const state = {active: false, count: 0, trace: new Float64Array(samples * 6)}
window.benchmark = {done: false, error: null}
window.benchSyncCalls = 0
const result = {
    environment: {userAgent: navigator.userAgent, isolated: crossOriginIsolated,
        visibility: document.visibilityState, timeOrigin: performance.timeOrigin},
    parameters: Object.fromEntries(q),
    clock_checks: []
}
const require = (condition, message) => { if (!condition) throw new Error(message) }

function clockStudy(read, count = 100000) {
    let previous = read()
    let positive = 0
    let minimum = Infinity
    let backwards = 0
    const start = performance.now()
    for (let i = 0; i < count; ++i) {
        const current = read()
        const delta = current - previous
        if (delta > 0) { positive++; minimum = Math.min(minimum, delta) }
        if (delta < 0) backwards++
        previous = current
    }
    return {count, positive, minimum_positive_ms: positive ? minimum : null,
        backwards, observed_loop_ns_per_read: (performance.now() - start) * 1e6 / count}
}

function clocks(module, label) {
    let maxOutside = 0
    let maxWallOutside = 0
    let offsetMin = Infinity
    let offsetMax = -Infinity
    for (let i = 0; i < 2000; ++i) {
        const before = performance.now()
        const cpp = module._bench_cpp_now()
        const after = performance.now()
        maxOutside = Math.max(maxOutside, before - cpp, cpp - after)
        const offset = cpp - (before + after) / 2
        offsetMin = Math.min(offsetMin, offset)
        offsetMax = Math.max(offsetMax, offset)
        const wallBefore = Date.now()
        const cppWall = module._bench_cpp_wall()
        const wallAfter = Date.now()
        maxWallOutside = Math.max(maxWallOutside, wallBefore - cppWall, cppWall - wallAfter)
    }
    result.clock_checks.push({label, max_outside_bracket_ms: maxOutside,
        wall_max_outside_bracket_ms: maxWallOutside, midpoint_offset_min_ms: offsetMin,
        midpoint_offset_max_ms: offsetMax})
    require(maxOutside < 0.002, 'C++ monotonic clock does not match performance.now')
    require(maxWallOutside < 1.01, 'C++ wall clock does not match Date.now')
}

if (traceEnabled) {
    const NativeChannel = MessageChannel
    const descriptor = Object.getOwnPropertyDescriptor(MessagePort.prototype, 'onmessage')
    window.MessageChannel = class {
        constructor() {
            // Exercise the same observer during warmup, discarding its samples.
            const index = state.active ? state.count++ * 6 : 0
            require(index < state.trace.length, 'too many observed yields')
            state.trace[index] = performance.now()
            const channel = new NativeChannel()
            state.trace[index + 1] = performance.now()
            const post = channel.port2.postMessage.bind(channel.port2)
            channel.port2.postMessage = value => {
                state.trace[index + 2] = performance.now()
                post(value)
                state.trace[index + 3] = performance.now()
            }
            Object.defineProperty(channel.port1, 'onmessage', {
                get() { return descriptor.get.call(this) },
                set(handler) {
                    descriptor.set.call(this, event => {
                        state.trace[index + 4] = performance.now()
                        // Validation only: a known delay must appear in the handler segment.
                        if (injectedDelayMs > 0) {
                            const until = performance.now() + injectedDelayMs
                            while (performance.now() < until) {}
                        }
                        handler.call(channel.port1, event)
                        state.trace[index + 5] = performance.now()
                    })
                }
            })
            return channel
        }
    }
}

function phase(active) {
    state.active = active
    if (active) {
        state.count = 0
        state.jsStart = performance.now()
        state.wallStart = Date.now()
    } else {
        result.js_elapsed_ms = performance.now() - state.jsStart
        result.wall_elapsed_ms = Date.now() - state.wallStart
    }
}

async function jsYield() {
    await new Promise(resolve => {
        const channel = new MessageChannel()
        channel.port1.onmessage = () => { channel.port1.close(); resolve() }
        channel.port2.postMessage(0)
    })
}

try {
    result.performance_clock = clockStudy(() => performance.now())
    result.wall_clock = clockStudy(() => Date.now())
    require(result.performance_clock.backwards === 0, 'nonmonotonic performance clock')
    if (mode === 'js-direct') {
        for (let i = 0; i < 512; ++i) await jsYield()
        const pairs = new Float64Array(samples * 2)
        phase(true)
        const start = performance.now()
        for (let i = 0; i < samples; ++i) {
            pairs[2 * i] = performance.now()
            await jsYield()
            pairs[2 * i + 1] = performance.now()
        }
        result.elapsed_ms = performance.now() - start
        phase(false)
        result.pairs = Array.from(pairs)
        result.operations = samples
        result.status = 0
        result.completions = 1
    } else {
        const {default: createModule} = await import(q.get('script'))
        let module
        let completed = false
        let factoryDone = false
        const done = new Promise((resolve, reject) => {
            createModule({
                arguments: [mode, q.get('limit'), String(samples), q.get('work'), q.get('control')],
                onRuntimeInitialized() {
                    module = this
                    clocks(this, 'before')
                    result.cpp_clock = clockStudy(() => this._bench_cpp_now())
                    let value = 0
                    for (let i = 0; i < 10000; ++i) value = this._bench_cpp_touch(value)
                    const start = performance.now()
                    for (let i = 0; i < 1000000; ++i) value = this._bench_cpp_touch(value)
                    result.js_wasm_sync_ns_per_call = (performance.now() - start) * 1e6 / 1000000
                    require(value === 1010000, 'JS to Wasm control was not executed')
                },
                benchPhase: phase,
                benchData: data => Object.assign(result, data),
                webcppTestFinished(status) {
                    result.status = status
                    result.completions = (result.completions ?? 0) + 1
                    completed = true
                    if (factoryDone) resolve()
                },
                print: line => { (result.stdout ??= []).push(line) },
                printErr: line => { (result.stderr ??= []).push(line) },
                onAbort: value => reject(new Error(String(value)))
            }).then(() => { factoryDone = true; if (completed) resolve() }, reject)
        })
        await done
        clocks(module, 'after')
        require(result.status === 0 && result.completions === 1, 'C++ failed or did not complete once')
        require(!result.stderr?.length && !result.stdout?.length, 'unexpected program output')
    }
    if (traceEnabled) {
        const expected = q.get('control') === '1' || mode === 'clock' || mode === 'sync' ? 0 : samples
        require(state.count === expected, `observed ${state.count} yields, expected ${expected}`)
        result.trace = Array.from(state.trace.slice(0, state.count * 6))
    }
    result.sync_calls = window.benchSyncCalls
    require(result.pairs.length === samples * 2, 'incomplete sample array')
    for (let i = 0; i < samples; ++i) {
        require(Number.isFinite(result.pairs[2 * i]) && result.pairs[2 * i + 1] >= result.pairs[2 * i],
            'invalid C++ timing pair')
    }
    window.benchmark = {done: true, result}
} catch (error) {
    window.benchmark = {done: true, error: error.stack ?? String(error)}
}
