## Context

The existing source is split between Asio and superproject build support. The captured
baseline is Asio `db4f7fc6afc516fdef057614cc971cdc39484099` and superproject
`d994334102d138e2847b7e8463fb1c06a859bd42`. The reference is a selected source export,
not their documentation's historical support claims; `reference/provenance.json` records
every byte source. No build or application benchmark was run for this proposal.

Boost.Cobalt is pinned to release 1.92.0, tag commit
`8b1cecba91d0e868ee0947b40fb04e3594680297`. The superproject's installed source subset
does not contain examples, so the inventory comes from that upstream commit, not a guessed
list or the active Jam targets alone. Its `example/` has 12 C++ programs plus `python.py`
and two build files. Its `bench/` has five C++ workloads and a CMake file.

| Current owner/path | Responsibility to preserve or extract |
| --- | --- |
| `libs/asio/src/cobalt_main.cpp` | Replacement `run_main`/`final_suspend`, bounded browser loop, cancellation and exit handling |
| `libs/asio/build.jam` | `cobalt-main`, `cobalt`, `cobalt_ssl`; core Asio/TLS targets remain Asio's |
| `libs/asio/test/*cobalt*`, `test/wasip2_tls_cobalt` | Runner, stack, status, threads, network, cancellation and mixed P2/TLS witnesses |
| `libs/asio/example/asio_cobalt.*`, `doc/cobalt.adoc` | Existing tutorial, page module and integration documentation |
| `tools/boost/cobalt-common` | Checked promise/cancellation corrections shared by all targets |
| `tools/boost/cobalt-wasip3` | Checked WASI header/source adaptation used by P2 and P3 despite its name |
| `tools/boost/cobalt-ssl` | Optional SSL wrapper source/header corrections |
| `tools/boost/Jamfile` | Source-built core archive, consistent overlays/macros, optional SSL target |
| `tools/openssl/test/cobalt*` | Cobalt TLS contract tests; common peer, certificates and provider remain shared |

## Goals / Non-Goals

**Goals:** preserve Boost source compatibility and existing working integrations, give
Cobalt independent ownership, make every feasible profile an explicit validation obligation,
and show real upstream-derived applications in a browser. Every support claim must identify
the program, revision, runtime, build configuration and result that supports it.

**Non-Goals:** replace Boost's coroutine model, implement a second event loop, change Asio's
turn default, infer browser raw TCP from WebSocket support, emulate process signals or
promise POSIX facilities that a host does not expose. This proposal does not create a
repository, extract production code, run CI, publish a release, or promise that arbitrary
Boost programs are portable regardless of their dependencies.

## Decisions

### D1. Explicit WebCPP entry points and shipped support units

Owner approval on 2026-11-06 supersedes the original requirement to preserve Boost
include paths in Wasm applications. The public entry is `<webcpp/cobalt/...>` and
`webcpp::cobalt`. Native coroutine and plain I/O types are aliases of actual Boost
1.92 types, with the unchanged compiled core and entry-point behavior. WebAssembly
uses versioned adaptations shipped in this library under explicit WebCPP paths.
Applications migrate includes/namespaces and keep the supported coroutine model.
Native and Wasm types have distinct ABIs and cannot be mixed across binary boundaries.

The owner approved one native exception after reproducing upstream TLS state and
cancellation defects: `webcpp::cobalt::io::ssl::stream` uses the corrected WebCPP
wrapper on native and WASI. Its headers/source ship explicitly, while native core,
runner and plain I/O remain upstream. Other native Boost defects stay named negative
probes, not silently corrected implementations or positive conformance results.

Core, runner and SSL are explicit once-per-program units. A header-only claim would
be false. `sources.json` and generated B2 lists declare one source inventory, with
matching executor/PMR/EH/thread settings in every translation unit. Arbitrary
cross-translation-unit ABI mismatches are a caller contract, not a promised universal
linker diagnostic. Clean compiler/CMake archive consumers audit actual dependencies.

Maintainer generators verify upstream hashes, preserve licenses and produce checked-in
files. Consumer builds run neither these generators nor patches and never shadow an
installed `boost/` tree. The original include-preserving overlay architecture is
rejected because it hid essential implementation behind the chosen build system.

### D2. One-way ownership and compatible B2 entry points

Canonical targets are `/webcpp/cobalt//core` (compiled Cobalt without a replacement entry
point), `/webcpp/cobalt//cobalt` (core plus the runner), `/webcpp/cobalt//io_ssl` (optional
SSL wrapper for custom-main consumers) and `/webcpp/cobalt//cobalt_ssl` (runner and SSL/TLS).
The canonical Cobalt build depends on `/webcpp/asio//asio`; TLS additionally consumes the
existing Asio trust helper and the one superproject OpenSSL/CA provider. Asio's core, browser
bridge and TLS targets never depend on any Cobalt target.

Existing `/webcpp/asio//cobalt` and `cobalt_ssl` become thin forwarding aliases. The low-level
`/webcpp//boost_cobalt` and `boost_cobalt_io_ssl` names remain compatible aliases to the new
owner. Root project registration resolves the names once; aliases must not recursively load
one another's build files. An Asio-only build must work when Cobalt is unselected and must
not compile Cobalt or resolve OpenSSL. A graph witness checks actual source/object lists,
including old and new target names in the same program.

Cobalt owns native originals, Wasm adaptations, runner and corrected SSL source.
Asio owns its shipped reactor and SDK adapters. Shared OpenSSL, CA and WIT providers
have one owner. Historical patch inputs under `upstream/` are maintainer provenance,
not a consumer step. Plain Asio must build without Cobalt or TLS, and old/new aliases
must resolve one core, one selected runner and at most one optional SSL provider.

### D3. Preserve the established scheduling contract

The browser runner uses `webcpp::asio::io_context`, preserves upstream executor initialization,
initial posting, result propagation and final suspension, and reads the existing
`WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN` setting. Its default stays 100. Explicit context
users retain `io_context_options{max_handlers_per_turn}`; all current allowed values,
including the documented opt-out, retain their meaning. No independent Cobalt turn-limit
macro or new default is introduced. Consumers and the separately compiled runner receive
the same configuration, tested across translation units.

JSPI and Asyncify are separate modules built from the same sources. The existing loader,
variant names and Asyncify stack controls remain Asio's. Browser reentry, creator-thread
affinity, wakeup, queued callback lifetime, cancellation and responsive rendering need
Cobalt witnesses even when Asio has its own passing tests. A yield allows host work; it does
not guarantee a frame deadline. Native process signal behavior remains upstream-compatible;
WASI has explicit application cancellation and no automatic host signal delivery. Browser
page cancellation is a C++ cancellation operation, never a pretend POSIX signal.

`webcpp::cobalt::run(task)` owns a separate context in its header, independently of
`co_main`. Its behavior, task move/future path and browser fairness require separate probes;
changing the runner does not establish its support. Native/WASI retain their target reactor; the browser helper uses the bounded WebCPP context. Any necessary checked adaptation must
preserve its signature and document its behavior, or retain an explicit missing/blocked
feature cell. The same audit applies to user-supplied executors and custom-main applications.

### D4. Matrix dimensions and evidence states

`reference/profiles.json` defines the initial 28 target/runtime/EH/thread/suspension tuples;
`reference/applications.json` lists all 12 applications; `reference/application-matrix.json`
expands every application across those tuples. These are planning data, not test results.
Implementation must expand the native/compiler/OS and actual browser-engine dimensions
from the superproject's declared lanes and record exact versions. A single macOS or Chromium
result cannot stand for Linux, Windows or another browser.

| State | Meaning and required record |
| --- | --- |
| `supported-tested` | Exact source and configuration built and ran through the public target, with assertions, command, log/result artifact, runtime versions and commit IDs |
| `sdk-runtime-blocked` | A versioned SDK/runtime or unavailable dependency prevents this profile/path; record a diagnostic or reproducible probe and the condition to re-evaluate |
| `not-applicable` | The requested semantics are absent in that environment, or the unchanged source intentionally needs another mode; name the semantic reason and any separate counterpart |
| `implementation-missing` | Feasible work, integration or verification is still missing; compile-only and unrun inherited source are here with progress recorded separately |

No blank cells, generic green platform badges, silent skips or successful compile treated
as runtime evidence. Support is qualified per operation: an EH-off timer pass cannot prove
pending-task moves. States for unchanged upstream source and an adapted counterpart are
separate. A blocked upstream source does not hide a feasible counterpart's missing work.

Initial profile obligations follow the currently pinned dependencies, not future SDK guesses:

| Target/runtime | Threads | EH | Suspension | Initial independent-library state / boundary |
| --- | --- | --- | --- | --- |
| Native OS process | S and M | on/off | N/A | Implementation/verification missing for the extracted target; expand actual OS/compiler/runtime lanes |
| Browser, actual Chromium | S and M/pthreads | on/off | JSPI and Asyncify | Implementation/verification missing; JSPI engine capability and M isolation headers are prerequisites |
| Emscripten under Node 26.7.0 | S and M/pthreads | on/off | JSPI and Asyncify | Separate implementation/verification rows; never substitutes for browser execution |
| WASIp2, Wasmtime 50.0.1, SDK 34.0 | S | on/off | N/A | Existing integration source is a starting point; fresh independent-target evidence missing |
| WASIp2, same versions | M | on/off | N/A | SDK/runtime blocked: no supported P2 pthread profile in this pinned integration |
| WASIp3, Wasmtime 50.0.1, SDK 34.0 | S | on/off | N/A | Existing integration source is a starting point; fresh independent-target evidence missing |
| WASIp3, same versions | M/cooperative | off | N/A | Fresh Cobalt evidence required; cooperative execution is not CPU parallelism |
| WASIp3, same versions | M/cooperative | on | N/A | SDK/runtime blocked: SDK 34.0 lacks this cooperative C++ EH runtime |

Native `threading=single` does not set `BOOST_ASIO_DISABLE_THREADS` and can still use internal
threads. An explicitly thread-disabled native build is a distinct, unverified optional
profile, never conflated with S. Browser S disables Asio threads. M tests must exercise real
worker execution, completion and cleanup, not only link with pthread flags. Suspended pthread
return values under Emscripten 6.0.11 Asyncify have a known versioned limitation; explicit
result storage is a documented counterpart, not a declaration that all pthreads are broken.

### D5. Feature coverage and deterministic refusals

Each row below expands across D4. `M` means implementation/verification missing for the
independent target, `B` a named SDK/runtime blocker and `N` semantics not applicable.
Inherited source is not promoted to `supported-tested` here. Global profile blockers apply
before feature eligibility; additional unverified overloads remain visible.

| Feature | Native | Browser / Node Emscripten | WASIp2 S | WASIp3 S / M-off |
| --- | --- | --- | --- | --- |
| `co_main`, custom main/core, status and cleanup | M | M; browser has completion rather than a process exit | M; command success/failure mapping | M; command success/failure mapping |
| Promise, task, generator, detached, channel, spawn, gather/join/race, wait_group/with | M | M | M | M |
| `run(task)`, custom executors, this_coro/this_thread, unique_handle, async_for/noop | M | M; separate loop/fairness probe for `run` | M | M |
| Operation composition, `use_op`, result/tuple adapters, cancellation | M | M | M | M |
| Timers, executor association, scheduling and allocator hooks | M | M; both suspension variants | M | M |
| Cobalt thread, pool/strand interoperability, cross-thread handoff | M in both S and M; probe each actual build profile | M in pthread profiles; N in single-thread workloads | B for M | M for cooperative profile only |
| TCP accept/connect, partial I/O, UDP and forward DNS through Cobalt | M | N for raw browser sockets; Node does not certify browser semantics | M, host permissions required | M, host permissions required |
| Cobalt SSL and Asio SSL through `use_op` | M | N for this guest OpenSSL socket target | M, shared provider | M, shared provider |
| Browser Promise/DOM operations through `use_op` | N | M in actual browser; DOM N under Node | N | N |
| POSIX signals, local sockets, serial ports | M where native backend exposes them | N for browser contract | N | N |
| Files, pipes, descriptors and optional upstream I/O wrappers | M where native backend exposes them | M pending exact Emscripten filesystem contract; no native-fd claim | M pending Cobalt-specific probes | M pending Cobalt-specific probes |
| Core callable Signals2 integration | M | M | M | M |
| Experimental Context/yield_context stackful interoperability | M with Boost.Context backend | B: no verified Wasm Boost.Context backend | B: same | B: same |
| Python/nanobind embedding example | M with separately pinned dependencies | B: no packaged matching host/runtime in this scope | B: same | B: same |

Capability macros are derived by `config.hpp` from the actual Asio/backend/build profile,
not force-enabled by consumers. Required-capability macros and B2 guards fail at the earliest
deterministic boundary with the target, feature, profile and remedy. Negative compile/link
tests must identify the expected diagnostic, not merely any failure. Unknown Boost hashes,
mismatched SDK private ABI and inconsistent archive/consumer flags are refused.

No compile-time macro can prove a browser's JSPI support, cross-origin isolation or a raw
pthread return after suspension. Those conditions have runtime detection/probes and explicit
errors. No fake DNS/TCP, process signals or TLS success is substituted for missing semantics.

### D6. Exceptions are a per-operation contract

All feasible profiles are built with EH on and off, and the choice reaches compiled sources.
Retain the nonfatal configuration warning for
[upstream issue 287](https://github.com/boostorg/cobalt/issues/287): the reported pending-task
move constructs an exception object and can abort with libc++ under `-fno-exceptions`.
The report is specific evidence, not a categorical ban on Cobalt, Wasm, or every operation.
Result adapters cannot repair arbitrary upstream internal exception construction.

Add a bounded process-level reproducer and EH-on control, recording standard library and
target-specific observations. Keep compile support, coroutine `unhandled_exception`
availability, valid result paths, cancellation and failure behavior distinct. Programs with
explicit `try`/`catch`/`throw` are tested unchanged under EH-on; their EH-off result-based
counterparts are separately named and reviewed. EH remains the default on SDK profiles
that provide its runtime unless the consumer explicitly selects EH-off. Choosing value
errors does not force EH-off, and requesting EH-off is honored wherever the profile permits
it. Suppressing errors, silently forcing EH-on or changing upstream signatures is rejected.

### D7. The complete upstream application inventory

Read `reference/applications.md` and the machine-readable inventory. The seven active Jam
targets are channel, delay, delay_op, echo_server, outcome, thread and thread_pool. Ticker is
commented there; CMake scans all C++ examples and conditionally builds ticker and Python.
Neither build file defines the completeness boundary: the pinned `example/` tree does.

The original `python.py` imports `boost_cobalt_example_python` but calls the undefined
`boost_async_example_python`. A minimal import stub reproduces that script-level `NameError`
without loading C++/nanobind; see `reference/python-script-check.json`. Preserve this upstream
failure and identify a patched script separately. Supplying dependencies alone does not
make the original script runnable, and the stub check proves no binding/coroutine behavior.

Unchanged-source runs preserve their identity. Adaptation patches separately describe
argument/fixture injection, finite stop conditions, browser host I/O, status handling and
EH-off changes. Native counterparts use the same adapted C++ workload, input and assertions
when compared with Wasm. Network examples use controlled local peers rather than depending
on live external markets or `boost.org` availability. Original programs remain inventoried
even when their full semantics are unavailable in a browser.

The first browser pages are timer/custom operation (`delay`, `delay_op`), channel and
Signals2; add Outcome and a worker/SPSC page in feasible configurations. Keep all program
logic in compiled C++. A bounded parameterized derivative for long-running or immediate
examples gives the user observable progress and cancellation without falsely claiming that
the original finite, synchronous program offered those controls. Cancellation is cooperative
and may lose to completion; both outcomes are shown correctly.

Each page has labeled inputs, start, cancel, progress/output, state and error presentation,
and a link to its upstream source, adaptation and support cell. Start twice, cancel during a
pending operation, restart after completion/error, worker termination, page unload and
stale-event delivery are tested. Keyboard use, focus/disabled states, narrow screens,
layout overflow and a visible animation/heartbeat are checked in a real browser. JavaScript
may bind UI and expose events, but cannot replace the Cobalt computation or fabricate progress.

The original HTTP/ticker/echo-server socket programs cannot run unchanged as browser raw-TCP
applications. Optional browser counterparts use explicit browser transport through existing
Beast/Asio adapters and retain the Cobalt algorithms where possible; source changes and server
role differences remain visible. A JS reimplementation, hidden network proxy or timer-only
substitute does not count as upstream application conformance. Python has a tracked dependency
blocker, not a fabricated browser port.

### D8. Release size, performance and memory are separate reproducible experiments

Benchmark builds use `variant=release`, `NDEBUG`, the same pinned sources, inputs and artifact
boundary, and isolated build directories. Cross `-O2`, `-O3`, `-Os`, `-Oz` with LTO off/on
for toolchains that support those flags; verify the final effective compile and link commands,
including inherited flags and their precedence. Record a compiler-specific equivalent or a
named unsupported optimization row for other native compilers. Verify LTO objects/linker
behavior rather than trusting a label. No debug results populate Release tables.

| Experiment | Required outputs and controls |
| --- | --- |
| Size | Raw Wasm, linked component including adapters, required JS/worker files, loader and total deployable payload as separate columns; native executable as a different format; optional gzip/Brotli with exact settings; baseline empty-runner and matched Asio program for marginal Cobalt cost |
| Performance | Throughput, elapsed wall time and process/thread CPU time where measurable; first module fetch/compile/instantiate/start distinguished from warm startup and steady state; total and operation counts; channel/coroutine/timer workload and applicable I/O workload |
| Scheduling | Per-yield latency distribution, handler work, host timer/frame delay and main-thread long tasks; default 100 and predeclared sweep of existing turn-limit controls; both browser suspension modes and worker configurations |
| Memory | Cobalt/application allocator calls, live/peak requested bytes and lifetime; separately measured opaque/runtime allocation; Wasm linear-memory initial/peak/current pages; native/Wasmtime/Node/browser-process RSS with explicit process attribution and baseline |

Wasm linear memory is not live heap and process RSS is not module memory. A browser that
cannot expose trustworthy module CPU/RSS measurements records unavailable values, not zero
or a synthetic substitute. Count PMR/custom allocator traffic separately from unhooked
runtime, JS glue, OpenSSL and library allocations. Published totals state the boundary and
limitations. Coroutine frames/executors are runtime objects, not shared-memory relocatable
data structures.

Use the same machine, workload semantics, count, input seed, compiler family/version where
possible, EH and thread behavior for native/Wasm comparisons. Record CPU/OS, power policy,
background load, runtime/compiler versions, host permissions, browser flags/isolation,
OpenSSL/CA revision, timer source, resolution and instrumentation overhead. Do not label
cooperative P3 execution as parallel throughput. Validate results/checksums outside timed
regions, and prevent dead-code elimination. Retain unsuccessful runs and exclusion reasons.

Predeclare warmup, at least 30 independent steady-state samples and at least 10 fresh-process
or fresh-page cold-start samples per feasible row, with fixed bounded workload duration and
seed. Report sample count, median, p95, spread and a stated confidence-interval method with
raw observations; a small sample never silently produces a reliable tail claim. CPU metrics
unavailable in browser remain unavailable. Alternate/randomize profile order to reduce drift.
Provide JSON/CSV raw data, environment/run manifests, commands, checksums, analysis script and
generated tables. Store no fabricated baseline values in the proposal. Existing Asio/Cobalt
size and stack observations are historical context only, not fresh benchmark results.

### D9. Documentation and validation remain part of the library

Use `doc/cobalt.adoc` and AsciiDoc includes organized by Diataxis: a working first coroutine,
how to configure/link/cancel/build/use TLS, exact capability/matrix/reference tables, and
explanations of scheduling, ownership, target limits and measurements. MrDocs generates the
new public configuration/adapter API reference using the established toolchain; link upstream
Boost.Cobalt API documentation for retained names rather than copying an inaccurate facade.
Every code listing includes an actual compiled example. Preserve incoming Asio guide anchors
as redirects or brief forwarding sections. Generated measurement/reference files are updated
through their generators only.

Guarantees cover lifetime, cancellation, executor/allocator association, errors, unavailable
features, browser hosting, signal/exit mapping and thread limits. `testing` names exact
commands and artifact provenance. Shared documentation and install/discovery rules must
discover Cobalt without duplicated reference pages or Cobalt files installed by Asio.

## Risks / Trade-offs

- Private Boost promise members and SDK descriptor APIs can change. Checked version/hash
  manifests, negative mismatch tests and exact-source upgrade review prevent silent drift.
- Forwarding aliases can form build-loading cycles or duplicate objects. Asio-only and
  mixed old/new target graph and link witnesses gate extraction.
- An upstream example may fail even natively or depend on a service that changed. Record
  the unchanged failure, reproduce it, and label any deterministic counterpart separately.
- Browser timers and scheduler load vary. Functional bounds and repeated measurements are
  different gates; no universal latency guarantee is inferred from a benchmark.
- Existing no-EH defects and runtime restrictions may prevent full conformance. Retain
  precise blocked cells and feasible result-path work rather than hiding the failures.
- Source-compatible source selection is delicate across Windows, P2/P3 and pthread builds.
  Public-target source lists, include order and compiled archive flags are tested.

## Migration Plan

1. After acceptance and a separate implementation request, freeze exact source revisions and
   collect baseline user-facing commands, graph/compile commands, output and behavior.
2. Build the independent tree and import it as one initial commit when repository creation
   is authorized. Preserve upstream notices; do not carry unrelated source history.
3. Move Cobalt-specific code/patches/tests/docs, split mixed fixtures without duplicating
   provider ownership, and redirect the compatibility names. Do not delete old definitions
   until all old/new entry-point checks pass against the same candidate.
4. Populate operation and application matrices through fresh public-target builds/runs,
   execute browser pages, produce Release measurements, then build/lint/render documentation.
5. Coordinate Asio and organization specification ownership references, submodule/installation
   metadata and declared lanes. Validate dependency isolation and combined consumers.
6. Review the complete candidate. Library and superproject CI are implementation-stage gates
   requiring their separately authorized execution; this proposal starts neither.

Rollback is an atomic return to the pre-extraction superproject and Asio revisions, including
target ownership and submodule registration. Never keep both runner definitions or two
providers. No release/status advancement follows merely from moving files.

## Open Questions

- **O1:** Additional browser engines beyond the initial real Chromium gate can be added
  after runtime capability probes. Each version receives its own row; this does not reduce
  the required initial JSPI/Asyncify, EH and pthread matrix.
- **O2:** Python/nanobind version pins and a native optional-fixture environment are resolved
  during inventory execution. Until supplied and tested, Python remains explicitly blocked
  by dependencies; enabling a Wasm Python host is a separate scoped integration decision.
- **O3:** Choose stable benchmark workload sizes after a bounded calibration run on the
  reference machine, then freeze them before collecting comparative samples. The required
  dimensions, statistics and raw-data contract are fixed by D8.

These are proposed defaults and deferrable details, not claims of owner acceptance. Changes
that would remove inventory rows, browser demonstrations or benchmark dimensions require a
proposal revision rather than silently shrinking implementation scope.
