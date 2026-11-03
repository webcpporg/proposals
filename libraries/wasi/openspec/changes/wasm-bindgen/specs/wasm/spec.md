## ADDED Requirements

### Requirement: Bindings: Component contract and explicit supported profile
`webcpp-wasm-bindgen` SHALL accept components with WIT interface types and generate C++20
bindings for a documented supported profile. An explicitly supplied WIT world MUST match the
component. The tool MUST reject unsupported input ABIs, type features, ambiguous world selection
and malformed inputs with actionable diagnostics and a nonzero exit status. The first profile
MUST NOT claim direct support for arbitrary wasm-bindgen packages or raw Wasm exports.

#### Scenario: Unsupported raw module
- **WHEN** a bare core module without a supported component contract is supplied
- **THEN** generation fails explaining the required component contract rather than inventing C++ types

#### Scenario: Valid component
- **WHEN** a valid component in the supported profile and an output namespace are supplied
- **THEN** the tool generates bindings matching the selected world and records its required imports

#### Scenario: Mismatched explicit interface
- **WHEN** a separately supplied WIT world does not match the component
- **THEN** generation fails and identifies the mismatch before publishing output

### Requirement: Bindings: Application-owned namespace and reproducible output
The generator SHALL accept a caller-selected C++ namespace and emit a public header plus the
implementation/adapters and manifest required by the selected backend. Identical inputs and
versions SHALL produce identical output independent of checkout location. Output MUST NOT
contain absolute source-checkout paths or generation timestamps. Generated application APIs
SHALL use reusable support from `webcpp::wasm` without adding domain-specific APIs there.

#### Scenario: Two independent applications
- **WHEN** two applications generate bindings for the same component under different namespaces
- **THEN** their generated public types belong to their chosen namespaces rather than webcpp's namespace

#### Scenario: Reproducible generation
- **WHEN** the same pinned inputs are generated from two different checkout directories
- **THEN** the output files are byte-identical

### Requirement: Bindings: Ownership and failure semantics
Generated bindings SHALL document and implement memory ownership, resource lifetime, string
and list conversion, result/error mapping and backend lifetime requirements. They MUST validate
cross-boundary memory accesses and release owned resources exactly once. They SHALL distinguish
recoverable component errors from traps and host integration failures. Unsupported mappings
SHALL fail generation rather than expose unsafe casts as a typed interface. Allocation and
exception behaviour SHALL conform to the agreed project policies and document external engine
boundaries.

#### Scenario: Resource is moved and destroyed
- **WHEN** an owning generated resource wrapper is moved and both wrappers leave scope
- **THEN** the component resource is released exactly once and the moved-from wrapper owns nothing

#### Scenario: Recoverable operation failure
- **WHEN** a component returns a declared error for malformed input
- **THEN** C++ receives the documented error representation and the instance remains usable where the contract permits

### Requirement: Execution: Native and browser support is explicit
The library SHALL provide native and Emscripten execution paths for the agreed component
profile. Each backend SHALL declare its dependencies, supported imports and runtime assets.
The browser implementation SHALL execute in a real browser and obey its capabilities. Missing
imports MUST fail explicitly. Any asynchronous mapping MUST preserve progress without blocking
the browser event loop waiting for work scheduled on that same event loop. Claims of support
SHALL be backed by tests of pinned versions on the declared target.

#### Scenario: Missing host capability
- **WHEN** a component requires an import that the selected host has not provided
- **THEN** instantiation or the specified capability operation fails explicitly without granting broader access

#### Scenario: Same supported operation on two targets
- **WHEN** the generated API is called by native and Emscripten consumers using the supported profile
- **THEN** both produce the expected result with the same documented C++ interface

### Requirement: Packaging: Bindings do not imply static conversion
Documentation and generated manifests SHALL distinguish embedded bytes, external runtime assets
and translated/statically linked implementations. A header or byte array alone MUST NOT be
claimed to eliminate the need for an execution backend. Native and Emscripten linkable outputs
SHALL be built for their own target. Static conversion beyond the initial backend profile
SHALL require a separate measured feature profile before being advertised as supported.

#### Scenario: Browser deployment uses JavaScript assets
- **WHEN** the selected backend requires generated JavaScript and separate core Wasm modules
- **THEN** the manifest and deployment instructions list those assets instead of claiming a single self-contained Wasm output

### Requirement: Example: BDK through the generic generator
`example/bdk_wallet/` SHALL contain a pinned Rust wrapper over BDK, its locked dependencies,
WIT interface, reproducible component build, C++ consumer and expected results. The example
build SHALL invoke `webcpp-wasm-bindgen` to generate application-owned `bdk_wallet` bindings
under `example::bdk_wallet` in the build directory. The example MUST NOT substitute handwritten
C++ bindings for the generated interface or add `webcpp::bdk` to the library. The initial tested
subset SHALL include deterministic descriptor/address operations, recoverable errors and
resource disposal compared with direct Rust reference results on native and in a real browser.

#### Scenario: Example rebuilt from clean inputs
- **WHEN** the documented example is built from its pinned Rust and component inputs
- **THEN** the generic generator produces its C++ bindings and both target consumers match the direct Rust expected addresses and errors

#### Scenario: Generic tool remains independent of BDK
- **WHEN** BDK example sources and dependencies are excluded from a build
- **THEN** the generator and generic library can still be built and used with the minimal component fixture
