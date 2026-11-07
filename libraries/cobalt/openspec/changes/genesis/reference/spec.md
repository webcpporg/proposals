# Scope recorded for the Cobalt genesis

This file records the requested scope on 2026-11-04. It is not an older accepted
specification or a claim that extraction has occurred. The normative proposal is
`../specs/cobalt/spec.md`, with decisions in `../design.md` and executable work ordered in
`plan.md` and `../tasks.md`.

The intended library is independent `webcpp::cobalt` in `webcpporg/cobalt`, integrated as
`libs/cobalt`, depending on Asio without a reverse core dependency. Existing Boost.Cobalt
application source, namespace and `co_main` remain compatible; previous B2 entry points
remain aliases. Current integration is the starting code, not a second implementation.

The compatibility objective covers native, browser Emscripten and WASIp2/P3 across feasible
EH on/off, single/multi-thread and JSPI/Asyncify tuples. Each feature and application must
have a tested result or an explicit SDK/runtime, applicability or missing-work state. All
12 pinned upstream C++ examples are inventoried, including non-default examples. Browser
pages must execute representative actual C++ algorithms with start/cancel/progress/errors.

Measure optimized Release size, performance and memory with effective compile/link flags,
LTO, native controls, repeatable inputs, environment/tool provenance, statistics and raw
machine-readable data. Keep cold startup distinct from steady state, wall from CPU time,
heap/allocation counts from Wasm linear memory and process RSS. Reuse Asio's measured
default of 100 handlers and existing configuration controls.

The delivery includes Diataxis AsciiDoc, MrDocs, capability guards and versioned limits.
It must not emulate absent browser TCP/process signals or turn issue 287 into a categorical
no-exceptions ban. This proposal authorizes planning only, with no repository creation,
production extraction, CI run or release implied by its existence.
