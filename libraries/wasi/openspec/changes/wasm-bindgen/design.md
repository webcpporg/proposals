## Context

The owner wants generic infrastructure that turns a foreign Wasm library into a usable C++
API. BDK is an end-to-end example produced using this infrastructure. The owner explicitly
rejected a permanent `webcpp::bdk` library and requested that the current wasi repository grow
into wasm, retaining WASI below it, with the generator's sources in that repository's tools
and its executable installed by the superproject's `b2 install`.

The repository's accepted `redesign` introduces `wasi::p2` and `wasi::p3`, a plain-function
entrypoint and target selection. It is not yet reflected in the published source examined for
this draft. This proposal changes their enclosing namespace, not those accepted decisions.

The current BDK Wasm project uses `wasm32-unknown-unknown`, wasm-bindgen and JavaScript bindings.
It is not a WIT component merely because its payload is Wasm. WIT describes component
interfaces; WAT is WebAssembly's textual representation. Neither is an execution engine.

## Goals / Non-Goals

**Goals:**

- One library family for WebAssembly integration, with WASI as a subsystem.
- Generated application-owned C++20 types and calls, hiding ABI memory bookkeeping.
- A reproducible BDK component and C++ example on native and in a real browser.
- A host-native generator installed and usable independently of the source checkout.
- Explicit ownership, errors, resource cleanup, dependencies and supported feature profiles.
- A design that can later support other languages, components and execution backends.

**Non-Goals of the first implementation:**

- A general replacement for Wasmtime, Jco or wasm-bindgen.
- Automatic recovery of Rust types from arbitrary core Wasm exports.
- Direct consumption of published wasm-bindgen packages, including BDK's JavaScript package.
- A built-in Bitcoin namespace or application-specific types in generic headers.
- Full WASIp1, WASIp2 and WASIp3 hosting parity on every target.
- Automatically granting OS capabilities that browsers cannot provide.
- Arbitrary runtime downloads, a package registry, or a production wallet UI.
- A promise that every supported component can already become a single statically linked Wasm.

## Decisions

### D1. Evolve the existing library rather than fork its responsibilities

Rename the implementation repository to `wasm` and the superproject submodule to `libs/wasm`.
The public namespace is `webcpp::wasm`; WASI lives at `webcpp::wasm::wasi`. Use
`webcpp::wasm::component` for component-specific support. Reserve `wat` for actual text-format
operations if those are added; do not create empty namespaces to reserve possible features.

Keep the proposal under the existing `libraries/wasi` OpenSpec root while it is a draft. This
retains its relationship with the genesis and accepted redesign. A later history-preserving
proposal-root migration must update metadata and links coherently, without rewriting archived
reference material or introducing a second genesis for the same library.

### D2. Preserve guest behaviour and introduce explicit include boundaries

All implemented WASI behaviour and regression coverage remain. Apply the new prefix to the
versioned design: `webcpp::wasm::wasi::p2` and `p3`, including its configured inline namespace.
The final macro prefix and compatibility aliases are O2, not silently decided by a text rename.

The portable aggregate `<webcpp/wasm.hpp>` includes common configuration and portable support.
Guest-specific entrypoints require explicit headers under `<webcpp/wasm/wasi/...>`. Engine
adapters require explicit opt-in headers and link dependencies. Header-only refers to webcpp's
C++ support interface, not to an assertion that execution engines or generated glue need no
compiled code. Existing allocation and exception policies apply to the new support.

### D3. Proposed first contract: WIT components

WIT-first is the architectural recommendation of this draft, not a previously approved owner
choice. It requires explicit acceptance (O8). A component carries machine-readable interface types. The generator may consume those types
from the component, or an explicitly supplied WIT world checked against that component.
Supported records, strings, lists, results and resources must have documented C++ mappings.
Unsupported types or features fail generation with actionable diagnostics before partial output
is published. WIT alone describes an interface; it does not supply the implementation.

A raw `.wasm` or wasm-bindgen package needs a distinct supported ABI. Do not infer compatibility
from its extension or TypeScript declarations. Support for such packages is a possible later
frontend, not an implicit promise of this draft.

### D4. Separate the host generator, generated API and execution support

`tools/wasm-bindgen/` holds the tool source. The installed name is `webcpp-wasm-bindgen`.
Generation runs on the build machine. The generated API belongs to the application-selected
namespace and invokes reusable support from `webcpp::wasm`. It must not depend on source-tree
paths or require users to edit generated files.

The following CLI is illustrative, pending O3:

```sh
webcpp-wasm-bindgen --input bdk_wallet.wasm \
  --namespace example::bdk_wallet --output generated/
```

The output includes `bdk_wallet.hpp`, implementation/adapters required by the selected backend
and a machine-readable manifest naming inputs, versions, imports and required runtime assets.
Generated files are reproducible for identical inputs and contain no wall-clock timestamps or
absolute checkout paths. A missing import is diagnosed at generation or instantiation as
appropriate; it is never silently replaced with a fake success.

### D5. Bindings and executable packaging are distinct stages

The initial backend candidates are Wasmtime for native hosts and Jco-generated integration
with the browser engine for Emscripten. These are recommendations to validate, not approved
version pins. A backend must implement the selected component ABI, instantiate modules, supply
imports, perform conversions and manage resources; bindings alone do not execute Wasm.

The same C++ example-facing API should work on both targets for the agreed supported profile.
JavaScript glue on Emscripten may remain internal. Document actual runtime files and semantics.
Application-visible asynchronous operations need a shared explicit model; no synchronous bridge
may block the browser event loop waiting for its own completion.

There are three packaging models to distinguish:

1. Embed component bytes and execute them with a backend.
2. Package browser modules and generated JavaScript as deployment assets.
3. Translate supported core Wasm to compilable code, then link target-specific objects into
   the final native executable or Emscripten Wasm.

A future wasm2c-based route could implement model 3, but component lowering, Canonical ABI,
imports, memory/trap semantics and feature coverage still need implementation. A native `.a`
cannot be reused as an Emscripten `.a`. A native `.so` is not a browser side module. A feasibility
report must measure this route; implementing it is deferred until its profile is approved.

### D6. BDK is an example consumer, not a public webcpp subsystem

`example/bdk_wallet/` contains the Rust wrapper crate, locked dependencies, WIT world, component
build instructions and C++ program. The wrapper exposes a small useful wallet interface,
initially deterministic descriptor/address operations and recoverable errors. Pin a BDK version
and compare results with direct Rust calls using public test descriptors and expected addresses.

The build invokes the generic generator to produce the bindings in its build directory under
`example::bdk_wallet`. No handwritten replacement of the generated API is allowed. Generic
headers, tool code and public namespaces contain no wallet-specific API. BDK-specific types
and dependencies stay in the example. The example is not an oracle for the entire BDK API.

Network synchronization, persistence, signing and async resources are later increments unless
explicitly added to the supported profile. No private keys or live funds are needed for tests.
The original BDK Wasm package remains useful research, not the component input for this route.

### D7. Build and install through the superproject

The companion organization proposal defines host/target separation, installation locations and
clean-prefix consumer checks. A Rust wrapper used to produce the example is distinct from the
implementation language of the generator, which remains O4. Version and licence checks apply
to every dependency. Installation must not quietly download mutable toolchains or packages.

## Risks / Trade-offs

- A repository rename crosses build, CI, site and documentation boundaries. Stage it first and
  test preserved WASI behaviour before adding new host support.
- The accepted redesign names the old paths. Reconcile its requirements explicitly before
  accepting implementation; do not change its accepted status or archived evidence here.
- Component support differs across engines and versions. Publish a measured profile rather
  than promising equivalent support for all WASI versions.
- Strings, resource handles and errors cross memory boundaries. Use generated lifetime-aware
  wrappers, validate ranges and encodings, and test repeated creation, failure and disposal.
- BDK's dependencies may need imports or features unavailable in one backend. Begin with the
  deterministic wallet subset and record unsupported imports rather than granting broad access.
- Execution engines may allocate outside the C++ allocator policy. Document and resolve this
  dependency boundary before acceptance, rather than asserting complete allocator control.

## Migration Plan

1. Inventory live headers, namespaces, macros, build targets, CI and site links, and reconcile
   the accepted redesign. Choose compatibility and proposal-root migration policies.
2. Rename the implementation repository and submodule in coordinated changes, keeping Git
   history and relative URLs. Update active documentation, metadata, include guards and build
   references. Never bulk-rewrite archived sources, generated files or changelogs by hand.
3. Run the existing HTTP and portable type tests on the migrated paths, with explicit
   target-only includes and portable aggregate header checks.
4. Add the host generator and install plumbing, with a minimal component fixture before BDK.
5. Add backend support and the BDK wrapper/example, then test installed consumers on both hosts.
6. Publish a migration table and measured support matrix. Keep this proposal draft until the
   open questions and dependency/rule conflicts have concrete resolutions.

## Open Questions

- **O1. Backend profile:** select and pin native/browser backends, supported component/WASI
  versions, imports and initial synchronous/async feature profile after a small measured spike.
- **O2. Migration compatibility:** temporary old headers/namespace aliases or a clean break?
  Which macros acquire `WEBCPP_WASM_WASI_` and how is accepted `redesign` rebased?
- **O3. Generator contract:** finalize CLI, WIT world selection, generated filenames, C++ type
  and error mappings, resource ownership and ABI/version compatibility checks.
- **O4. Tool implementation:** C++ implementation, Rust-based component tooling or a wrapper
  around upstream tooling? Resolve against project language/dependency policies before coding.
- **O5. Static conversion:** measure wasm2c/component lowering on the chosen profile and decide
  whether a later backend should emit linkable native/Emscripten objects or archives.
- **O6. Organization policy:** record any required exception for compiled support and non-C
  backend/tool dependencies; do not treat draft recommendations as existing permission.
- **O7. Proposal migration:** when and how should `libraries/wasi` become `libraries/wasm`
  without rewriting historical proposal artifacts or invalidating the accepted redesign?

- **O8. Input contract:** confirm the proposed WIT-first scope versus prioritizing consumption
  of existing wasm-bindgen packages. WIT-first requires a new Rust wrapper/component for BDK.

## References

- [BDK Wasm, current repository](https://github.com/bitcoindevkit/bdk-wasm): existing JS-facing
  packaging; the earlier MetaMask repository moved here.
- [Component Model concepts](https://component-model.bytecodealliance.org/design/component-model-concepts.html).
- [Jco transpilation](https://bytecodealliance.github.io/jco/transpiling.html).
- [Wasmtime C/C++ embedding](https://docs.wasmtime.dev/c-api/).
- [wasm2c](https://github.com/WebAssembly/wabt/blob/main/wasm2c/README.md).

References establish mechanisms and candidates, not tested compatibility of this unbuilt design.
