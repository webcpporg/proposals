## MODIFIED Requirements

### Requirement: Library: Header-only library in its namespace
The existing wasi implementation repository SHALL evolve into `webcpporg/wasm`, checked out
under `libs/wasm`, retaining its history. Its webcpp-authored C++ support interface SHALL remain
header-only C++20 under BSL-1.0, with portable support in `webcpp::wasm` and existing WASI guest
functionality in `webcpp::wasm::wasi`. Public headers SHALL live under `<webcpp/wasm/...>`.
Existing WASI guest behaviour MUST remain covered by the current tests. The accepted `redesign`
proposal's versioned namespaces and entrypoint semantics MUST be reconciled under the new
prefix before implementation. Optional compiled backends and host tools SHALL be identified
as dependencies, not described as header-only execution engines. Wallet-specific API names
MUST NOT be added to the library's public namespace.

#### Scenario: A program includes the whole library
- **WHEN** a native, Emscripten or WASI translation unit includes `<webcpp/wasm.hpp>`
- **THEN** portable support is declared without implicitly exporting an HTTP guest entrypoint

#### Scenario: The library stands alone
- **WHEN** generic headers and backend integration are inspected
- **THEN** domain-specific bindings remain application-owned and optional engine dependencies have explicit include and link boundaries

#### Scenario: Existing HTTP behaviour survives migration
- **WHEN** the current HTTP example is migrated to the new paths and built on wasip2 and wasip3
- **THEN** its existing expected responses and failure behaviour remain unchanged

#### Scenario: Versioned redesign moves under wasm
- **WHEN** the accepted redesign and the namespace migration are implemented together
- **THEN** versioned functionality is under `webcpp::wasm::wasi::p2` and `p3`, preserving the agreed target-selection semantics

### Requirement: Library: Public headers and the convenience header
The library SHALL provide `<webcpp/wasm.hpp>` and common configuration under
`<webcpp/wasm/config.hpp>`. The aggregate MUST compile alone natively and on Emscripten without
WASI guest bindings or a required optional execution engine. Guest-only entrypoints SHALL be
included explicitly through headers under `<webcpp/wasm/wasi/...>`. Engine integration SHALL
have explicit include/link boundaries. Existing portable request/response types SHALL remain
usable without the guest ABI, with exact paths finalized against `redesign` before acceptance.

#### Scenario: Each header compiles alone
- **WHEN** every public header is compiled as the only include on each target it declares
- **THEN** portable headers pass on native and Emscripten and guest entrypoint headers pass on their WASI targets

#### Scenario: The convenience header brings the entry point
- **WHEN** a migrated application previously relied on the old aggregate to provide its guest entrypoint
- **THEN** the migration instructions require an explicit WASI guest header; the new portable aggregate does not implicitly bring that entrypoint

#### Scenario: Native application includes the portable aggregate
- **WHEN** a native translation unit includes only `<webcpp/wasm.hpp>` without a WASI SDK or execution engine
- **THEN** it compiles without selecting a guest entrypoint or requiring generated WASI headers

#### Scenario: Browser application includes the portable aggregate
- **WHEN** an Emscripten translation unit includes only `<webcpp/wasm.hpp>`
- **THEN** it compiles without defining a WASI guest target

#### Scenario: Target-specific entrypoint remains explicit
- **WHEN** a WASI guest needs its HTTP entrypoint
- **THEN** it includes the appropriate WASI header rather than obtaining that entrypoint implicitly from the portable aggregate
