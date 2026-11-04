## Purpose

Make existing Boost.Cobalt coroutine programs usable through an independently owned WebCPP
integration on native systems, browsers and WASIp2/P3, with explicit capability boundaries,
source compatibility, application evidence and reproducible measurements.

## ADDED Requirements

### Requirement: Independent library and one-way dependencies

The library SHALL be owned by `webcpporg/cobalt` and integrated at `libs/cobalt`, with its
own build, configuration, tests, examples and documentation. It MUST depend on Asio rather
than making Asio's core depend on Cobalt. Cobalt-specific implementation and checked overlay
ownership MUST move to this library; shared provider/overlay utilities MUST have one owner.

#### Scenario: Asio without Cobalt
- **WHEN** a consumer builds only the public Asio core target
- **THEN** Cobalt sources, replacement entry point and optional OpenSSL are not compiled or linked

#### Scenario: Independent Cobalt consumer
- **WHEN** a consumer selects the independent Cobalt target
- **THEN** its Asio dependency and required compiled support units are supplied once with matching configuration

### Requirement: Existing source and build entry points remain compatible

The library SHALL preserve Boost.Cobalt's headers, namespace, coroutine types, `use_op`,
executor/cancellation associations and `co_main` signature. Its public targets MUST support
both `co_main` and custom-main consumers. Existing `/webcpp/asio//cobalt`, `cobalt_ssl`,
`/webcpp//boost_cobalt` and `boost_cobalt_io_ssl` entry points MUST remain forwarding
compatibility aliases without a dependency cycle or duplicate implementation.

#### Scenario: Existing coroutine source
- **WHEN** an existing supported program using `<boost/cobalt/main.hpp>` and `boost::cobalt::main co_main` is built through old and new target names
- **THEN** no coroutine API rewrite is required and output, cancellation and completion behavior agree

#### Scenario: Custom main
- **WHEN** a program with its own `main` uses the core or optional I/O SSL target
- **THEN** it receives Cobalt's compiled implementation without linking a replacement `co_main` runner

#### Scenario: Both target names
- **WHEN** one application selects both the compatibility and canonical targets
- **THEN** exactly one compiled definition of each Cobalt support unit is linked

### Requirement: Pinned and checked upstream integration

The initial integration SHALL use Boost 1.92.0 headers and matching sources, with Cobalt
upstream commit `8b1cecba91d0e868ee0947b40fb04e3594680297`. Every local correction MUST retain
its upstream provenance, license, original and resulting hashes and validation witnesses.
Unknown source hashes, incompatible private SDK interfaces or mismatched source/header
versions MUST fail deterministically instead of applying a best-effort patch.

#### Scenario: Changed upstream input
- **WHEN** a checked overlay sees an unexpected Boost source hash
- **THEN** the build names the mismatched file and expected version and refuses the unsupported overlay

#### Scenario: Archive and consumer agreement
- **WHEN** EH, threading or target changes
- **THEN** both compiled sources and consumers use the same selected overlays and settings

### Requirement: Preserved scheduling, completion and cancellation

The runner SHALL use Asio's established event-loop contract and default of 100 handlers per
browser turn, honoring its existing compile-time and runtime configuration controls where
applicable. It MUST preserve executor setup, initial posting, final cleanup and upstream
result behavior subject to the target's explicit process/component completion mapping.
Cancellation MUST propagate to pending operations on the correct executor and permit cleanup.

#### Scenario: Browser remains usable
- **WHEN** a Cobalt workload continuously posts handlers under the default bounded loop
- **THEN** the page receives host event/render opportunities and the Cobalt work completes without changing the default turn limit

#### Scenario: Configured runner
- **WHEN** the existing Asio default macro is changed consistently across the program and compiled runner
- **THEN** the runner uses that value and the same workload validates the resulting yield count

#### Scenario: Nonzero completion
- **WHEN** `co_main` returns 7 after an asynchronous wait
- **THEN** native and applicable Emscripten process drivers observe their specified status, the browser reports completion explicitly, and WASI records its versioned command success/failure mapping

#### Scenario: Cancellation cleanup
- **WHEN** cancellation is requested while a channel, timer, socket or composed operation is pending
- **THEN** its observable outcome and cleanup match the supported upstream contract and a bounded test detects lost completion or stale callbacks

### Requirement: Explicit configuration and runtime capability matrix

The published matrix SHALL enumerate features and applications by target, runtime/version,
EH on/off, single/multi-thread profile and JSPI/Asyncify where meaningful. It MUST distinguish
`supported-tested`, `sdk-runtime-blocked`, `not-applicable` and `implementation-missing`,
identify reasons and evidence, and keep unchanged-source and adapted-source rows separate.
Compile-only, inherited source and Asio-only results MUST NOT become Cobalt runtime support.

#### Scenario: Unsupported SDK tuple
- **WHEN** WASIp3 cooperative threads with EH-on are requested on SDK 34.0
- **THEN** the matrix and deterministic diagnostic identify the missing cooperative EH runtime with that version, while feasible tuples remain independently testable

#### Scenario: P2 multiple threads
- **WHEN** WASIp2 multiple-thread support is requested on the initial pinned SDK profile
- **THEN** it is recorded as SDK/runtime blocked and is not silently run as a single-thread case

#### Scenario: Node result
- **WHEN** an Emscripten example passes under Node
- **THEN** only that Node tuple gains runtime evidence and corresponding real-browser tuples remain unverified until run

#### Scenario: Independent upstream run helper
- **WHEN** `boost::cobalt::run(task)` or an experimental stackful adapter is evaluated
- **THEN** it receives its own behavior/dependency evidence and is not declared supported merely because the replacement `co_main` runner passed

#### Scenario: SDK upgrade
- **WHEN** the toolchain or runtime pin changes
- **THEN** versioned blocked rows are re-probed and all affected support evidence identifies the new versions

### Requirement: Deterministic capability guards

The library SHALL expose derived configuration/capability information consistent with Asio
and the actual backend. Explicit requirements for unsupported facilities MUST fail with a
diagnostic naming the feature, target/profile and available remedy. A consumer MUST NOT be
able to force a capability true by defining a derived macro. Runtime-only prerequisites
MUST be detected and reported at runtime.

#### Scenario: Browser raw sockets
- **WHEN** a browser program requires a raw TCP/UDP backend or the guest socket OpenSSL target
- **THEN** it receives the documented unavailable-capability error and never a fake transport or successful no-op

#### Scenario: Browser worker hosting
- **WHEN** a pthread page lacks required isolation or a JSPI module lacks engine support
- **THEN** startup reports the actual missing runtime prerequisite and does not claim successful Cobalt execution

#### Scenario: Negative-test specificity
- **WHEN** an unavailable feature is tested
- **THEN** the test requires the intended capability diagnostic instead of accepting an unrelated compilation failure

### Requirement: Exceptions follow the caller with precise limitations

Every feasible profile SHALL support building with EH on and off and preserve caller
selection throughout its dependencies. The warning and targeted reproducer for upstream
issue 287 MUST remain visible until superseded by verified upstream-compatible behavior.
The library MUST NOT infer universal no-EH support from result adapters or impose a
categorical no-EH ban. Sources with explicit exception syntax MUST be classified honestly,
with any value-error counterpart carrying its own source identity and evidence.

#### Scenario: Result path without exceptions
- **WHEN** a feasible result-based timer/cancellation program is built with EH-off
- **THEN** the setting reaches all compiled Cobalt units and the program runs with documented outcomes despite the nonfatal upstream warning

#### Scenario: Pending task move
- **WHEN** the known pending-task move is exercised with libc++ and EH-off
- **THEN** a bounded subprocess records its actual behavior, environment and EH-on control, without claiming other runtimes behave identically

#### Scenario: Upstream try/catch example
- **WHEN** the unchanged `echo_server.cpp` is classified for EH-off
- **THEN** the matrix states that its exception syntax requires EH-on and separately tracks the result-based counterpart rather than silently forcing EH-on

### Requirement: Thread profiles have actual behavioral evidence

Threaded support SHALL be tested with Cobalt worker creation, execution, cross-thread
handoff, completion, cancellation and join/cleanup as applicable. The documentation MUST
distinguish native S from a strictly thread-disabled build, browser pthread execution from
main-thread work, and WASIp3 cooperative threads from simultaneous CPU parallelism. Known
runtime-path restrictions MUST remain versioned and narrower than a blanket threading ban.

#### Scenario: Browser worker
- **WHEN** a browser M example is marked supported/tested
- **THEN** evidence includes worker execution and main-thread UI handoff in that EH/suspension tuple, including cleanup after cancellation

#### Scenario: Asyncify pthread return
- **WHEN** a suspended pthread's raw return-value path is evaluated on Emscripten 6.0.11
- **THEN** the dedicated probe's limitation and a separately validated explicit-result-storage counterpart remain distinguishable

### Requirement: Optional TLS and truthful host boundaries

Optional Cobalt SSL and Asio SSL through `use_op` SHALL reuse the shared source-built
OpenSSL/CA provider without imposing it on plain Cobalt. TLS tests MUST cover peer/hostname
verification, failures, cancellation, state transitions and cleanup on feasible native/WASI
profiles. Browser and WASI process signals, local sockets and serial ports MUST NOT be
silently emulated as native facilities.

#### Scenario: Combined TLS consumer
- **WHEN** a program links Cobalt, Asio and Beast TLS together
- **THEN** the effective build uses one matching OpenSSL/CA provider and no duplicate archive or ABI configuration

#### Scenario: Failed Cobalt handshake
- **WHEN** the first handshake fails or is cancelled
- **THEN** subsequent application I/O follows the documented closed/error state and never falls back to plaintext

#### Scenario: WASI host shutdown
- **WHEN** a host requests graceful component shutdown
- **THEN** the application uses an explicit cancellation protocol and no POSIX signal delivery is claimed by the runner

### Requirement: Complete pinned example inventory

All 12 upstream C++ examples at the pinned revision SHALL have stable inventory entries
with path/hash, dependencies, upstream registration, target/profile classifications and
required adaptations. Supporting `python.py` and build files MUST be retained. Every future
upstream example addition/removal MUST make the completeness check fail until classified.
The five upstream benchmark sources MUST also be accounted for before reuse or exclusion.

#### Scenario: Non-default upstream example
- **WHEN** the inventory is checked
- **THEN** `http`, `python`, `signals`, `spsc` and commented-out `ticker` are present even though they are not active upstream Jam targets

#### Scenario: Signals2 example
- **WHEN** `signals.cpp` is classified on browser/WASI
- **THEN** it is assessed as an in-process Boost.Signals2 awaitable rather than incorrectly excluded as a POSIX signal program

#### Scenario: Network application counterpart
- **WHEN** HTTP, echo-server or ticker cannot retain its native transport semantics in a browser
- **THEN** the original remains visibly unavailable and any feasible browser counterpart identifies changed transport/role semantics and missing implementation separately

### Requirement: Interactive browser demonstrations execute real C++

The library SHALL provide interactive browser pages for representative feasible upstream
examples, initially delay/custom-operation, channel and Signals2, with Outcome and
worker/SPSC counterparts in applicable profiles. Pages MUST invoke real compiled Cobalt
logic and expose input, start, cancel, progress/output and error/completion state. Bounded
adaptations MUST be labeled. Tests MUST cover pending cancellation, restart, duplicate start,
errors, stale callbacks, page/worker cleanup, responsiveness and usable narrow-screen layout.

#### Scenario: Start and cancel a timer operation
- **WHEN** the user starts a long delay and cancels it while pending
- **THEN** C++ cancellation determines the displayed result, the page stays interactive and a subsequent start operates independently

#### Scenario: Fast completion races cancellation
- **WHEN** C++ completes before the cancel request is delivered
- **THEN** the UI reports the real completed outcome without manufacturing cancellation or retaining stale progress

#### Scenario: Runnable page evidence
- **WHEN** a page is marked supported/tested
- **THEN** an actual browser run identifies its C++ module, upstream/adapted source, profile, assertions and rendered UI checks

### Requirement: Reproducible Release size measurements

Size measurements SHALL use optimized Release builds across supported `-O2`, `-O3`, `-Os`,
`-Oz` and LTO on/off, with EH/thread/suspension dimensions and matching native controls.
Effective compiler and linker commands, source/tool/runtime versions and artifact hashes
MUST be recorded. Raw Wasm/component, required JS/worker/loader, total payload and native
executable boundaries MUST remain distinct. Compressed sizes MUST name the compressor and
settings. Unsupported flag profiles MUST be recorded rather than omitted or relabeled.

#### Scenario: Link optimization differs from compile optimization
- **WHEN** a requested optimization flag is overridden or absent in the final link command
- **THEN** validation rejects the mislabeled measurement until the effective flags match its declared profile

#### Scenario: Marginal size
- **WHEN** Cobalt overhead is reported
- **THEN** the comparison includes matched baseline programs and does not call the complete application module Cobalt-only overhead

### Requirement: Reproducible performance and responsiveness measurements

Performance measurements SHALL report throughput, wall time, available CPU time, cold
startup, steady state and yield/browser-responsiveness latency separately. Workload, input,
warmup, clock, sample count, environment and statistics MUST be explicit, with raw
machine-readable observations and reproducible analysis. Native comparisons MUST use
matched workload semantics and configuration. Existing Asio scheduling controls and the
default 100 MUST be preserved; benchmark results MUST NOT invent a new default.

#### Scenario: Cold versus warm run
- **WHEN** a benchmark includes module compilation/instantiation in its first run
- **THEN** that cost is identified separately from warm execution and steady-state throughput

#### Scenario: Comparative claim
- **WHEN** native and browser throughput are compared
- **THEN** source, workload/output checks, environment, repetitions and confidence/spread methodology are supplied and unmatched semantics are disclosed

#### Scenario: Browser CPU unavailable
- **WHEN** the browser cannot provide a trustworthy CPU-time measurement
- **THEN** the report records it as unavailable and does not substitute wall time under the CPU label

### Requirement: Memory metrics preserve their measurement boundaries

Memory reports SHALL separate application/Cobalt allocation counts and live/peak bytes,
opaque runtime allocations, Wasm linear memory and process RSS. Allocator hook coverage,
baseline, units, process attribution, sampling limitations and available metrics MUST be
documented. No unmeasurable value SHALL be fabricated or treated as zero.

#### Scenario: Linear memory stays allocated
- **WHEN** a completed workload frees heap objects but Wasm memory pages remain allocated
- **THEN** the report shows live heap and linear-memory size as separate observations

#### Scenario: Shared browser process
- **WHEN** RSS includes browser engine or unrelated page work
- **THEN** the report states that attribution limit and does not label the whole value Cobalt heap usage

### Requirement: Documentation, provenance and completion gates

The library SHALL provide a Diataxis AsciiDoc guide, MrDocs-generated public reference,
compiled-source listings, configuration/capability tables, guarantees, testing and benchmark
reproduction guidance. Reference exports MUST identify exact sources and hashes without
copying stale status as current truth. Extraction completion MUST require fresh public-target
build/runtime evidence, dependency/link checks, browser E2E, documentation/lint and the
authorized library/superproject CI gates. Proposal metadata MUST NOT imply acceptance or
release from artifact existence alone.

#### Scenario: Resume from reference
- **WHEN** an implementer reads `reference/STATUS.md`
- **THEN** it distinguishes exported starting code from completed tasks and supplies the next commands and unresolved gates

#### Scenario: Accepted support claim
- **WHEN** an implementation task or matrix cell is marked complete
- **THEN** exact committed evidence exists for its scope and no unrun configuration is included in the claim
