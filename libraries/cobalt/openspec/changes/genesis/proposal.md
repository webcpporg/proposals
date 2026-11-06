## Why

Existing Boost.Cobalt applications should keep their C++20 coroutine APIs when compiled for
native systems, a browser through Emscripten, or WASIp2/P3. Cobalt integration already exists
inside Asio and the superproject, but its ownership, application coverage and measurements
need an independent library contract before extraction or broader support can be claimed.

## What Changes

- Establish `webcpp::cobalt` as a wrapper in `webcpporg/cobalt`, integrated at `libs/cobalt`,
  with its own configuration, build targets, tests, examples and documentation. Preserve
  `boost::cobalt` headers, types, completion tokens, `co_main`, executor associations and
  cancellation semantics; do not require users to rewrite their coroutine algorithms.
- Extract the replacement runner, Cobalt-specific overlays, compiled-source recipe and
  Cobalt-owned tests/examples from Asio and the superproject. Keep dependency direction
  Cobalt -> Asio, with no Asio core dependency on Cobalt. Retain the old
  `/webcpp/asio//cobalt` and `cobalt_ssl` names as compatibility aliases.
- Define an explicit feature and application matrix by target, actual runtime, exception
  mode, thread profile and, on Emscripten, JSPI or Asyncify. Every cell has evidence or a
  named limitation: supported/tested, SDK/runtime blocked, not applicable, or implementation
  missing. A successful Asio test does not establish a Cobalt application result.
- Account for all 12 C++ examples in pinned Boost.Cobalt 1.92.0, including examples absent
  from its default Jam targets. Provide runnable counterparts and interactive browser pages
  for feasible examples, with real C++ work, start/cancel controls, progress and errors.
  Preserve upstream and adapted-source identities separately.
- Retain optional guest TLS through the one shared OpenSSL/CA provider. Preserve the
  existing no-exceptions warning for upstream issue 287 without imposing a blanket ban;
  validate each operation and retain explicit SDK restrictions.
- Measure optimized Release size, performance and memory with native controls, effective
  compile/link flags, raw machine-readable data and repeatable commands. Keep Asio's
  measured default of 100 handlers per turn and its existing configuration controls.
- Publish a Diataxis AsciiDoc guide and MrDocs reference, capability guards, the complete
  matrix, application adaptations, benchmarks, guarantees and known limits.

## Capabilities

### New Capabilities

- `cobalt`: independent ownership and source-compatible Boost.Cobalt integration, target
  capabilities, application inventory, browser demonstrations, Release benchmarks,
  documentation and extraction/conformance gates.

### Modified Capabilities

None in this OpenSpec root. Acceptance must coordinate the Asio ownership references and
the organization's compiled-support/build rules before extraction; this genesis does not
silently rewrite another root's specification or certify its implementation.

## Impact

- **Kind:** wrapper, `port_of: null`, BSL-1.0. The wrapped Boost release is pinned to 1.92.0;
  Cobalt's tag resolves to `8b1cecba91d0e868ee0947b40fb04e3594680297`.
- **Implementation:** proposed repository <https://github.com/webcpporg/cobalt>, future
  superproject submodule `libs/cobalt`. No repository creation or extraction is performed by
  this proposal. `reference/source/` contains selected, unmodified upstream and existing
  integration files with per-file provenance, not a completed standalone library.
- **Status:** `implementing`. Rodrigo approved the proposal and requested execution on
  2026-11-06. `decided` records that date; `release` remains null. Implementation tasks
  are checked only after their work and verification are committed.
- **Dependencies:** `webcpp::asio`, `webcpp::config`, matching Boost headers/sources and
  target toolchains. Optional TLS reuses the superproject provider. Application-only
  dependencies such as Beast, JSON, URL, Outcome and Python/nanobind are declared per example.
- **Affected areas:** Asio's runner, Cobalt guide/example/tests and compatibility targets;
  `tools/boost/cobalt-common`, `cobalt-wasip3`, `cobalt-ssl` and `tools/boost/Jamfile`;
  documentation, install/discovery, capability reporting and local/CI lane declarations.
  Existing native, Asio, Beast and combined OpenSSL consumers must retain behavior.
- **Evidence boundary:** this work inventories source and validates planning artifacts only.
  Historical Asio test reports and measurements are not copied as Cobalt support claims.
