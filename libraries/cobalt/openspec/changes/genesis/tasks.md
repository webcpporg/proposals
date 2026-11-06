## 1. Acceptance, baseline and inventory

- [x] 1.1 After acceptance and an implementation request, freeze the working revisions and capture existing user-facing Cobalt build/run and dependency-graph baselines; verify old targets reproduce their documented behavior before edits. (plan: 1.1)
- [x] 1.2 Add a completeness/provenance check for every pinned example and benchmark source; verify all 12 examples, 5 benchmarks and 28 starting profiles are accounted for and that adding/removing a fixture source fails the check. (plan: 1.2)
- [ ] 1.3 Expand matrix rows to actual compiler/OS/browser/runtime lanes and reproduce each versioned SDK/runtime limitation; verify no blank or unsupported claim lacks a reason and no supported/tested cell lacks exact run evidence. (plan: 1.3)

## 2. Independent ownership and compatibility

- [ ] 2.1 Add failing public-target/Asio-only/mixed-alias build witnesses, then establish the independent library skeleton, configuration and metadata; verify Asio-only consumers do not resolve Cobalt or OpenSSL. (plan: 2.1)
- [ ] 2.2 Transfer the runner and Cobalt overlay/source recipes with matching consumer/archive settings; verify exact source provenance, hash/version refusals and one definition per support unit. (plan: 2.2)
- [ ] 2.3 Forward the four legacy target names and migrate Cobalt-owned tests/examples while splitting mixed fixtures; verify old/new/custom-main consumers and combined Asio/Beast/Cobalt links have no graph cycle, duplicate runner or provider. (plan: 2.3)

## 3. Core behavior and capabilities

- [ ] 3.1 Add deterministic capability/configuration tests before guards; verify unavailable feature, forged macro, SDK mismatch and archive/consumer mismatch errors identify the intended cause. (plan: 3.1)
- [ ] 3.2 Migrate and extend runner, coroutine composition, timer/channel/cancellation and exit tests; verify native, P2/P3 and both browser suspension variants preserve results, cleanup and the existing default/configured turn budget. (plan: 3.2)
- [ ] 3.3 Preserve the issue-287 warning and add bounded per-runtime no-EH regressions with EH-on controls; verify feasible result paths still run and exception-syntax examples remain separately classified. (plan: 3.3)

## 4. Threads, sockets and optional TLS

- [x] 4.1 Add Cobalt worker lifecycle, cross-thread handoff and cancellation witnesses; verify native B2 S and M, browser pthread tuples and P3 cooperative M-off independently, including versioned Asyncify return-value limits. (plan: 4.1)
- [ ] 4.2 Exercise Cobalt TCP/UDP/DNS and optional files/pipes/descriptors through public targets; verify real peer traffic and cancellation on each feasible native/WASI profile while unsupported browser/host operations remain explicit. (plan: 4.2)
- [ ] 4.3 Move Cobalt TLS witnesses to its ownership and retain shared fixture/provider utilities; verify TLS trust/hostname/state/cancel cases and one-provider combined consumers, with plain Cobalt staying TLS-free. (plan: 4.3)

## 5. Upstream application conformance

- [x] 5.1 Run unchanged channel, delay, delay_op, outcome and signals sources, then add explicit finite/UI adaptations; verify outputs, immediate/pending paths, cancellation and hashes for every feasible profile. (plan: 5.1)
- [ ] 5.2 Run thread, thread_pool and spsc in both native B2 S/M and applicable Wasm worker profiles and create separate EH-off/browser lifecycle counterparts; verify worker execution, result ordering/checksums, pool join and cleanup, keeping B2 S distinct from thread-disabled execution and P3 cooperative execution distinct from CPU parallelism. (plan: 5.2)
- [ ] 5.3 Run echo_server, http and ticker against controlled peers, classify every unavailable original cell, and implement feasible documented counterparts; verify protocol, partial I/O, failure and cancellation outcomes without relying on live external services. (plan: 5.3)
- [x] 5.4 Preserve the original python.py module-name failure, create a separately identified script correction and pin the optional native Python/nanobind fixture; verify the real corrected counterpart's generator/promise/error/cancel behavior and preserve explicit source defects and dependency blockers. (plan: 5.4)

## 6. Browser pages and E2E

- [x] 6.1 Build upstream-derived timer/custom-op, channel and Signals2 pages with real C++ start/cancel/progress/error state; verify actual browser JSPI/Asyncify, EH and S/M tuples and source-to-module identity. (plan: 6.1)
- [x] 6.2 Add Outcome and worker/SPSC demonstrations under feasible capabilities; verify missing-runtime diagnostics, actual worker handoff and bounded repeatable inputs rather than JavaScript substitutes. (plan: 6.2)
- [x] 6.3 Exercise keyboard/input validation, duplicate start, pending cancellation, completion races, restart, unload and stale events; inspect narrow/wide layouts and responsiveness and fix visible defects before recording browser evidence. (plan: 6.3)

## 7. Release benchmark evidence

- [ ] 7.1 Audit all five upstream benchmark sources and define matched native/Wasm workloads, correctness witnesses and isolated Release profiles; verify effective compile/link flags and LTO for each supported optimization/EH/thread/suspension tuple. (plan: 7.1)
- [ ] 7.2 Collect size and cold-start/steady-state/throughput/CPU/yield/responsiveness samples with the frozen methodology; verify raw JSON/CSV, artifact hashes, environment manifests, sample counts and reproducible statistics, recording unavailable metrics and blocked rows. (plan: 7.2)
- [ ] 7.3 Measure allocations, live/peak heap, linear memory and attributed process RSS separately; verify instrumentation coverage/overhead and generate reports with matched native baselines and explicit boundaries. (plan: 7.3)

## 8. Documentation and superproject integration

- [ ] 8.1 Write the Diataxis AsciiDoc guide, MrDocs configuration/reference and compiled example listings; verify local doc/reference builds and rendered links, code, matrices and narrow-screen tables. (plan: 8.1)
- [ ] 8.2 Update Asio ownership links/legacy anchors and organization integration references, library discovery/install metadata and lane declarations; verify installed public headers, declared targets/lanes and Asio/Beast coexistence. (plan: 8.2)

## 9. Final candidate and status

- [ ] 9.1 Run fresh targeted and full declared implementation-stage checks, lint and documentation, then obtain independent review; verify failures/flakiness are fixed and exact candidate evidence covers every claimed matrix cell. (plan: 9.1)
- [ ] 9.2 When separately authorized, validate the library/superproject CI and integrate the candidate; verify matching commits and update source provenance/STATUS without implying a release or checking uncommitted work complete. (plan: 9.2)
