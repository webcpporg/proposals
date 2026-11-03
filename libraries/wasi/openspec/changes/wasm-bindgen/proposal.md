## Why

C++ applications should be able to reuse libraries implemented in Rust and distributed as
WebAssembly without rewriting their algorithms or manually maintaining memory and ABI glue.
BDK is the motivating example, not a new Bitcoin API to add to webcpp. The application should
obtain its own generated `bdk_wallet` bindings from a generic webcpp tool.

The existing `wasi` library helps write WASI guests. Its name and WASI-only aggregate header
are too narrow for a library that also helps native and Emscripten applications consume Wasm.
Execution, interface binding and implementation of WASI are separate responsibilities.

## What Changes

- Evolve the existing `webcpporg/wasi` repository into `webcpporg/wasm`, checked out as
  `libs/wasm`, with namespace `webcpp::wasm` and headers under `<webcpp/wasm/...>`.
- Nest existing WASI functionality under `webcpp::wasm::wasi`, preserving its behaviour and
  accounting for the already accepted `redesign` proposal and its versioned namespaces.
- Keep `<webcpp/wasm.hpp>` usable on native and Emscripten without including guest entrypoints
  or forcing an optional execution engine dependency.
- Add the host tool `webcpp-wasm-bindgen` in `tools/wasm-bindgen/` of the renamed library.
  Its first input contract is a WebAssembly component with WIT types, not an arbitrary
  `wasm-bindgen` package or a bare Wasm module with unknown conventions.
- Generate application-owned C++20 bindings in a caller-selected namespace. The shared support
  lives in `webcpp::wasm`; the example uses `example::bdk_wallet`, never `webcpp::bdk`.
- Provide a reproducible `example/bdk_wallet/`: a small Rust wrapper over a pinned BDK release,
  its WIT contract and component build, generated bindings, and C++ consumers on native and
  Emscripten. This produces a new BDK component; it does not directly consume the published
  JavaScript-oriented `bdk-wasm` package.
- Install the host executable with the superproject's `b2 install`, alongside the headers and
  any required support assets, as specified by the companion organization change `wasm-tools`.
- Separate generated bindings from execution and static conversion. Embedding Wasm bytes is
  not the same as translating and statically linking their implementation into one final Wasm.

## Capabilities

### New Capabilities

- `wasm`: reusable execution support, component bindings generation and the BDK example.

### Modified Capabilities

- `wasi`: library identity, namespace and header boundaries. Unchanged HTTP behaviour remains
  governed by its existing specification; `redesign` must be reconciled before implementation.

## Impact

- **Kind:** evolution of a library of webcpp's own, with optional external backend dependencies.
- **Implementation:** not started. The existing implementation is at
  https://github.com/webcpporg/wasi; its intended renamed destination is
  https://github.com/webcpporg/wasm. No repository rename is performed by this draft.
- **Status:** draft. Architecture and acceptance criteria are recorded; backend selection,
  compatibility policy, exact CLI and supported type profile remain to be measured and decided.
- **Breaking changes:** repository, submodule path, headers, namespaces, macros, build targets
  and documentation addresses need a coordinated migration.
- **Other proposals:** `organization/openspec/changes/wasm-tools/` owns installation and host
  tool build rules. The accepted `redesign` remains accepted; this draft does not supersede it.
- **Dependencies:** a Rust toolchain for the BDK example, a component toolchain and optional
  execution backends. Exact versions, licensing and build integration are implementation gates.
