# Cobalt extraction and validation plan

## Owner-approved execution amendment, 2026-11-06

The current design/spec supersedes the original Boost-namespace and consumer-overlay
instructions below. Implement explicit `webcpp::cobalt` source distribution, actual
Boost native core/runner identity, and the approved corrected native SSL exception.
Use the superproject's `2026-11-06-portable-network-libraries` plan for the coordinated
Asio/Cobalt/Beast migration. Retain every original application/benchmark obligation.
The numbered task bodies below remain the historical extraction plan, not authority
to restore consumer patching. Current evidence and uncommitted gates are in STATUS.md.

This is an implementation plan, not work performed. Every entry maps to `../tasks.md`.
Execute only after proposal acceptance and an explicit implementation request. Commands
below are from the superproject root unless a different directory is stated. Planned target
names are established by the corresponding task; they do not exist merely because this
document names them. Preserve collection rules, run fresh evidence builds with `b2 -a`,
and first reproduce defects through the user-facing command or browser experience.

## 1. Baseline and inventory

### 1.1 Freeze existing behavior

Files: new `libs/cobalt/test/baseline/` manifest and captured run artifacts; read existing
Asio Cobalt tests, `build.jam`, `tools/boost/Jamfile` and checked overlays. Record exact
superproject/Asio/Boost revisions, tool versions, source hashes and dependency/compile/link
commands. Run old public target clients before changing ownership, including `co_main`
return 0/7, a custom main, timer cancellation, pending composed accept and optional TLS.

```sh
b2 -a variant=release libs/asio/example//asio_cobalt.output libs/asio/test//cobalt
b2 -a toolset=emscripten variant=release libs/asio/test//cobalt_asyncify
```

Also use the existing P2/P3 and thread/TLS lanes from the actual Jamfiles. Expected output:
named pass/fail artifacts, effective source lists and explicitly absent tools; no blanket
support claim. A baseline failure is reproduced and reported before extraction proceeds.

### 1.2 Lock complete source inventory

Files: planned `libs/cobalt/test/applications/manifest.json`, inventory checker and regression
fixtures. Start from `applications.json`, `profiles.json` and the original source export.
Verify upstream tree membership/hash, active/absent/commented Jam registration and conditional
CMake dependencies. Require 12 examples, 5 benchmark sources and all support files.
First add an unclassified fixture source and see the checker fail; make the classified
fixture pass. Expected output: stable IDs, no omissions and a machine-readable completeness
verdict. Do not edit original upstream files to make their hashes fit.

### 1.3 Bind profiles to real environments

Files: planned `test/matrix/profiles.json`, runtime probes and result schema. Expand the
28 planning tuples by actual native lanes and browser/runtime versions. Derive available
toolchain flags from current root configuration. Probe P2 pthread refusal, P3 cooperative
EH runtime absence, JSPI feature detection, browser isolation and Asyncify pthread return.
Each blocked result names versions, command/diagnostic and re-evaluation condition.
Expected output: unique complete cells with `status`, `reason`, `source_revision`,
`source_form`, `profile`, `command`, `result_artifact` and `environment`; no absent runtime
is converted to a pass or generic platform impossibility.

## 2. Ownership and compatibility

### 2.1 Skeleton and dependency boundary

Files: planned `libs/cobalt/{AGENTS.md,README.md,LICENSE_1_0.txt,build.jam}`, `meta/libraries.json`,
`include/webcpp/cobalt.hpp`, `include/webcpp/cobalt/config.hpp`, test/example/doc Jamfiles.
Write failing build fixtures for canonical targets and Asio-only isolation, then create
the library structure according to collection conventions. Configuration includes Asio
and collection headers first; retained Boost APIs are not reimplemented.
Expected output: canonical targets discovered, public headers compile alone, metadata
names `wrapper`, and Asio-only build commands contain no Cobalt/OpenSSL objects.

### 2.2 Transfer checked implementation

Files: planned `src/cobalt_main.cpp`, `build-support/cobalt-{common,wasi,ssl}/` and compiled
recipe; source locations in the provenance manifest identify exact inputs. Copy runner
and correction bytes first, preserving notices. Move generic shared utilities only if they
are Cobalt-exclusive. Adapt paths/project IDs without altering behavior. Exclude upstream
`main.cpp` from runner target and SSL from plain core; custom main links core only.
Add source/hash mismatch negative fixtures before moving manifests. Verify one object
definition, consumer/archive overlay precedence and matching EH/thread macros on native,
Emscripten and P2/P3. Expected output: checked input/output hashes and identical baseline
behavior; no project-wide warning weakening.

### 2.3 Forward old names and split tests

Files: Asio `build.jam`, Cobalt `build.jam`, root target registration/Boost aliases and
Cobalt-owned test/example paths. Establish the following conceptual mapping using actual
B2 registration rules, never a reciprocal eager import:

```jam
# Compatibility names point to one canonical owner.
alias cobalt : : : : <library>/webcpp/cobalt//cobalt ;
alias cobalt_ssl : : : : <library>/webcpp/cobalt//cobalt_ssl ;
```

Keep `core` independent of runner and `io_ssl` independent of `co_main`. Split P2 fixtures
by Cobalt-specific clients versus shared TLS/entropy peer logic. An old/new/mixed alias
matrix must compile/link/run, and graph traversal must remain acyclic. Expected output:
identical declared semantics, one runner/provider and Asio-only build independence.

## 3. Core behavior and guards

### 3.1 Configuration and refusal contracts

Files: `include/webcpp/cobalt/config.hpp`, build guards, `test/config_test.cpp` and negative
compile/link fixtures. Before implementation, add failing witnesses for derived macro
tampering, unsupported capability requirements, P2 M, P3 M/EH-on and mismatched archives.
Test default/collection-wide/per-library exception overrides. Negative drivers assert
the intended diagnostic; runtime probes handle facts that macros cannot know.
Expected output: predictable named errors and unaffected feasible configurations.

### 3.2 Runner and coroutine contracts

Files: migrated `test/cobalt*`, new coroutine contracts, browser driver and application
fixtures. Reuse baseline status/stack/cancellation tests; add task/promise/generator/channel,
gather/join/race/spawn/wait_group/with, detached, unique_handle, async_for/noop, this_coro/
this_thread and custom composition witnesses as inventory requires. Probe `run(task)`
separately: it owns an upstream Asio context and is not fixed by replacing `co_main`.
Account for experimental Context/yield_context interoperability and its Wasm backend blocker.
Test immediate completion, deferred initiation, cancellation while provably pending,
completion races, lifetime/allocator/executor association and stale callbacks.
Build separate translation units with default 100 and existing override controls; inspect
runner flags. Check process status 0/7 and WASI command mapping in dedicated drivers.
Expected output: bounded deterministic results for every supported profile, and no new
scheduling default or independent Cobalt budget control.

### 3.3 No-exceptions limitations

Files: bounded `test/no_exceptions/` reproducer/driver and matrix annotations. Reproduce
the pending-task move with EH-off and EH-on control under actual standard libraries.
Retain the nonfatal issue-287 warning and checked promise support. Probe successful
result/tuple paths and cancellation separately; explicit exception-syntax applications
get a separate value-error counterpart. Expected output: exact abort/success evidence
with environment and source revision, not a blanket claim of prohibition or full support.

## 4. Threads, network and TLS

### 4.1 Worker lifecycle

Files: Cobalt thread tests, browser-worker driver and P3 cooperative lane. Establish
worker identity, thread-safe handoff, creator-thread context affinity, pending cancellation,
join and teardown. Exercise both browser suspension modes and EH settings. Recheck the
native witnesses in both B2 S and M because native S does not disable Boost threads. Treat
explicit `BOOST_ASIO_DISABLE_THREADS` as its own unverified profile. Recheck the
Asyncify raw return-value probe independently; use explicit result storage where documented.
Expected output: worker behavior observed, not merely a successful pthread link; P3 cooperative
results never labeled CPU parallelism.

### 4.2 I/O operations

Files: Cobalt network/descriptor tests and shared peer drivers. Drive actual TCP accept/connect,
partial read/write, EOF, UDP including empty/gathered data and forward DNS with exact supported
policy. Check cancellation and association through Cobalt's wrappers and `use_op`. Audit
files, pipes and descriptors per runtime and record unavailable or missing wrappers.
Expected output: real bytes/addresses/errors on public targets, bounded peers, explicit host
permissions and no browser raw-socket or POSIX-signal emulation.

### 4.3 Optional SSL

Files: Cobalt SSL tests and recipe; retain shared `tools/openssl` certificate/peer utilities
and provider. Migrate existing TLS clients/state/cancellation witnesses without weakening
verification. Test trust and hostname failures, SNI, STARTTLS, failed/cancelled handshake,
buffered handshake, partial transfer, shutdown and no plaintext fallback. Add a combined
Asio/Beast/Cobalt consumer that proves one archive/provider configuration.
Expected output: real TLS success/failure/state evidence in feasible native/P2/P3 tuples;
plain target links no TLS, browser target fails with its named boundary.

## 5. All upstream examples

### 5.1 Portable coroutine examples

Files: `test/applications/{channel,delay,delay_op,outcome,signals}/`, original-source manifest
and patches. First build/run unchanged files where feasible, preserving hashes and input
arguments. Add separate bounded/output-observable counterparts using the exact algorithms.
Check ordered channel values, timer/custom-op pending/ready paths, both Outcome values and
Signals2 ownership/value 42. Expected output: per-cell evidence and source-form identity;
Signals2 is not excluded as a process-signal example.

### 5.2 Worker applications

Files: `test/applications/{thread,thread_pool,spsc}/`, browser host adapters and fixtures.
Check original results before parameterizing. Preserve request/response and SPSC logic;
record finite bounds and exact sequence/count/checksum. Keep custom-main pool linking
separate from runner linking. Write EH-off counterparts explicitly and surface pending-task
move limitations. Run both native B2 S and M because neither label proves thread absence.
Expected output: native S/M and applicable Wasm worker/join evidence, with accurate
thread-disabled or SDK-blocked cells, without serializing an example and calling it equivalent.

### 5.3 Network applications

Files: `test/applications/{echo_server,http,ticker}/`, controlled TLS/WebSocket/TCP peers,
adaptation records and optional browser counterparts. Replace external-service reliance
with explicit fixture arguments or separately hashed patches, never hidden DNS/proxy changes.
Retain original source classification and add finite server shutdown. Test cancellation,
partial I/O, protocol success/errors and ticker subscription lifecycle/session bounds.
Expected output: native/WASI socket application evidence and explicit browser role/transport
differences; no original raw-TCP browser pass is fabricated.

### 5.4 Python optional dependency fixture

Files: `test/applications/python/`, script adaptation patch and pinned dependency manifest.
First retain the original script's `NameError`: it imports `boost_cobalt_example_python`
but calls `boost_async_example_python`. `python-script-check.json` records the safe minimal
module-stub reproduction, not C++ binding evidence. Add a separate patch changing all three
references to the imported module name and retain both hashes. Then establish a native
CPython/nanobind environment, run the corrected counterpart and test generator values,
promise 42, cross-language error and cancellation behavior. Record absent dependencies
independently of the source defect. Expected output: original failure and optional corrected
native result or precise blocker, with Wasm integration still blocked unless separately
supplied and validated. Do not invent a Python port or edit the exported original script.

## 6. Browser demonstrations

### 6.1 First pages

Files: `example/browser/{timer,channel,signals}/` C++, host bindings and page assets, and
variant manifests using existing shared rules. Start from 5.1 algorithms. UI actions start
real C++ jobs, request C++ cancellation and display actual output/errors. Parameterized
derivatives identify their changes. Expected output: compiled modules with source provenance
and actual-browser assertions for every feasible JSPI/Asyncify/EH/thread tuple.

### 6.2 Outcome and worker pages

Files: `example/browser/{outcome,workers}/`, adapters from 5.2 and runtime capability checks.
Retain real Outcome operations and SPSC/worker channel logic. Display unsupported-runtime
diagnostics and keep workers' UI updates on the page thread. Expected output: correct C++
results, visible progress under bounded repetition and clean worker completion/cancellation.

### 6.3 End-user and visual verification

Files: browser E2E drivers and evidence manifest. Run real click/keyboard sequences for
valid/invalid inputs, start twice, cancel pending work, race completion, restart and unload.
Assert old callbacks cannot mutate a new run. Observe a host heartbeat/animation while work
is pending. Inspect wide/narrow rendering for overflow, clipping, labels, errors, focus and
disabled controls. Expected output: passing behavior plus captured UI evidence for actual
engine versions. Node tests remain distinct and cannot close this gate.

## 7. Measurement plan

### 7.1 Workloads and build profiles

Files: `benchmark/` workload manifests, scripts and correctness fixtures. Audit all five
upstream benchmark files; reuse the Cobalt/Asio arms with bounded inputs. Separate optional
Boost.Context arms, which have no verified Wasm backend. Define empty-runner and matched
Asio baselines, native counterparts and application workload boundaries. Calibrate then
freeze counts/seeds before comparison. Build Release in isolated directories with each
optimization and LTO setting, preserving EH/thread/suspension dimensions.

```sh
b2 -a variant=release toolset=emscripten exception-handling=on \
    webcpp-suspend=jspi threading=single \
    --build-dir=bin/cobalt-bench-oz-lto-jspi-on-single \
    "cxxflags=-Oz -flto" "linkflags=-Oz -flto" libs/cobalt/benchmark
```

Expected output: inspected effective commands, valid LTO verification, artifact hashes,
workload correctness and no Debug row mislabeled Release. `libs/cobalt/benchmark` is a
planned target defined by this task, not an existing command validated in this proposal.

### 7.2 Size and timing observations

Files: benchmark runner, `results/<run-id>/{environment.json,commands.json,samples.jsonl,
sizes.csv,summary.json}` and analysis scripts. Follow design D8: raw deployable artifact
parts, optional compressed payload, cold fetch/compile/instantiate/start and steady state,
throughput, wall/available CPU time and browser yield/response distributions. Use at least
30 independent steady-state and 10 cold-start samples, declared warmup and randomized order.
Retain unsuccessful runs with reasons. Expected output: repeatable statistics with sample
counts/spread/confidence method; no unmeasured CPU or missing configuration fabricated.

### 7.3 Memory observations

Files: counting allocator/resource instrumentation, linear-memory probes, process-metric
collector and analysis output. Distinguish live/peak requested heap bytes and allocation
counts from opaque runtime/JS/TLS storage, Wasm pages and native/host/browser RSS. Record
measurement overhead and sampling attribution limits. Expected output: separate labeled
metrics, matched native baselines and explicit unavailable values; a single RSS total is
never described as Cobalt memory cost.

## 8. Documentation and integration

### 8.1 Guide and reference

Files: `doc/cobalt.adoc`, Diataxis includes, `doc/Jamfile`, MrDocs configuration, public Doc
Comments and compiled example tags. Write tutorial/how-to/reference/explanation paths,
capability/application matrices, guarantees, browser hosting and benchmarking reproduction.
Generate API reference for WebCPP additions and link retained Boost APIs. Include actual
compiled source, not duplicated snippets. Build docs sequentially, inspect rendered output
and resolve broken links/overflow/warnings. Expected output: usable guide and exact reference
with measured claims bound to artifacts, no hand-edited generated files.

### 8.2 Coordinated ownership references

Files: Asio guide/anchors, organization and Asio specifications, root discovery/install and
declared lanes, submodule/metadata registration only when authorized. Preserve old Cobalt
guide links as forwarding sections. Verify `b2 declared-targets`, `b2 declared-lanes`, header
installation and examples use the new owner without duplicated discovery. Expected output:
consistent source/spec ownership and unchanged Asio/Beast/combined consumers.

## 9. Candidate closure

### 9.1 Fresh complete local evidence and review

Files: candidate matrix/result manifest and reviewed changes. Run `b2 -a` for every declared
test/example and own lane, including browser E2E, applicable tool tests, full lint and docs.
Inspect every failure and flaky test; reproduce user-facing defects before fixes. Review
source compatibility, dependency direction, error/cancellation/lifetime and measurement
boundaries independently. Expected output: exact candidate evidence, no unexplained gaps
among claimed support, and recorded blockers for genuinely unavailable profiles.

### 9.2 Authorized CI/integration and status

Files: exact source export/provenance and `STATUS.md`, integration commit metadata. CI and
repository/publication operations occur only in the authorized implementation stage.
Require library and superproject results for the actual final candidate, not an older tree.
Check boxes only for committed completed work, preserve `release: null` until an actual
release, and record decisions only when made. Expected output: traceable integration and
status without turning file movement, baseline tests or proposal validation into a release.
