## ADDED Requirements

### Requirement: Build: Bindings generator is a host tool
The superproject SHALL build or select `webcpp-wasm-bindgen` as an executable for the machine
running the build independently of the consumer's target toolset. Its sources SHALL live in
`libs/wasm/tools/wasm-bindgen/`. Missing host configuration or incompatible tool versions MUST
fail with an actionable diagnostic. WASI-targeted application builds MUST NOT imply support for
hosting arbitrary components inside WASI guests.

#### Scenario: Emscripten cross compilation
- **WHEN** an Emscripten application needs generated component bindings
- **THEN** the build executes a host-native generator and compiles its output for Emscripten

#### Scenario: Host toolchain missing
- **WHEN** the generator needs building and no usable host toolchain is configured
- **THEN** the build fails naming the required host configuration instead of attempting to execute target Wasm

### Requirement: Install: Headers and host bindings tool
`b2 install` SHALL install `webcpp-wasm-bindgen` under `<prefix>/bin`, the wasm public headers
under `<prefix>/include/webcpp`, and required support data under `<prefix>/share/webcpp/wasm`.
The installed tool SHALL work outside the source checkout and discover its declared support
assets without source-tree paths. Staged prefixes and prefixes containing spaces SHALL work.
External dependencies and runtime deployment assets MUST be documented explicitly.

#### Scenario: Clean-prefix native consumer
- **WHEN** installation completes into a prefix containing spaces and the tool is invoked outside the checkout
- **THEN** it generates a supported fixture and a consumer builds and runs using installed headers and declared dependencies

#### Scenario: Browser deployment
- **WHEN** the installed tool generates bindings for an Emscripten consumer
- **THEN** the documented deployment assets suffice to run that consumer in a real browser

### Requirement: Build: Generation inputs and outputs are tracked
The build SHALL declare the component, selected interface/world, configuration, tool version
and required assets as generation inputs, and all generated files as outputs in the build
directory. Regeneration SHALL occur when relevant inputs change. Builds and installation MUST
NOT silently download unpinned dependencies or depend on manually edited generated sources.

#### Scenario: Interface input changes
- **WHEN** a component interface or generator configuration changes
- **THEN** the next build regenerates affected bindings before compiling their consumer

### Requirement: Migration: wasm registration preserves WASI coverage
The coordinated migration SHALL register `libs/wasm` with the renamed implementation repository
using a relative submodule URL and update live build, CI, documentation and metadata references.
It SHALL preserve the existing WASI lanes and Git history. Archived proposal reference sources
and generated outputs MUST NOT be manually rewritten as part of a bulk rename.

#### Scenario: Library discovery after migration
- **WHEN** the superproject enumerates the migrated library and its declared lanes
- **THEN** wasm is discoverable under its new identity and the prior WASI HTTP coverage remains present
