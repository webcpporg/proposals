## Acceptance

The owner accepted this proposal and requested its merge on 2026-11-03. Acceptance covers the
proposed direction and scope. Remaining backend, version and migration choices are implementation
gates; no runtime compatibility, implementation completion or policy exception is claimed.

## Context

The owner requested that the generator source live in the renamed wasm library and that the
superproject install it through `b2 install`. A cross build needs two platforms: the machine
running generation and the target consuming generated code. The library proposal owns the
interface and runtime behaviour; this companion owns registration, build and installation.

## Goals / Non-Goals

**Goals:** install a usable host generator, preserve cross compilation, support staged prefixes,
keep library headers portable and prove the installed consumer path outside the checkout.

**Non-Goals:** implement the generator in this proposal, choose an unmeasured backend, bundle mutable
toolchains, install example-specific Bitcoin APIs or silently relax dependency policies.

## Decisions

### D1. One source location and two platform roles

The source lives at `libs/wasm/tools/wasm-bindgen/`. The superproject defines a host build target
that produces `webcpp-wasm-bindgen`, even if the default application toolset is Emscripten or
WASI. A configured prebuilt host tool may be used with version compatibility checks; this must
be a host-tool override, not an application library dependency. Missing host configuration fails
with a diagnostic naming the required host toolchain. No target Wasm is executed as a host tool.

### D2. Install a usable tool with b2 install

The install graph includes the host executable, public headers and declared support assets.
The installed executable belongs in `<prefix>/bin`, headers in `<prefix>/include/webcpp/wasm/`
and `<prefix>/include/webcpp/wasm.hpp`, and required data in `<prefix>/share/webcpp/wasm/`.
Libraries and platform-specific executable suffixes follow the existing platform install rules.
The executable must find its support files without absolute checkout paths. Staged installation
must not leak staging paths into generated output. Installed external runtime dependencies must
be documented and checked, with exact versions and licences recorded.

### D3. Library registration migrates coherently

Rename `libs/wasi` to `libs/wasm` with a relative repository URL, preserving library history.
Update library discovery, build target paths, CI selection, docs/site metadata and live links.
Preserve existing WASI test coverage. Do not rewrite archived proposal sources or historical
release records. Resolve the proposal-root migration and documentation redirects with the
library proposal before the repository rename is executed.

### D4. Build dependencies and generated files

Generation is a declared build step with component/interface/configuration/tool-version inputs
and all output files listed. Outputs belong to the build directory and are regenerated when
inputs change. A clean build must not depend on previously generated source-tree files. Rust
and BDK dependencies belong to the example build; they are not required to build the generic
tool unless an explicitly approved implementation choice introduces a Rust toolchain dependency.
Installation must not fetch unpinned network content implicitly.

### D5. Verify the installed experience

Tests install into a temporary prefix containing spaces, invoke its tool outside the source
checkout, generate a minimal fixture and build/run native and browser consumers. A separate
cross-build test proves that the installed generator is native for an Emscripten-targeted
consumer. The WASI cross-build check proves host-tool selection and supported compile-only
output; it does not instantiate nested components inside a WASI guest. The supported uninstall/staging behaviour follows the superproject's install convention.

## Risks / Trade-offs

- Tool dependencies may exceed current C dependency rules: resolve with an explicit proposal
  amendment and pinned source builds before implementation.
- Existing install targets may need extension: inspect them and add failing tests before
  implementation rather than assuming a current install contract that has not been measured.
- Jco/Wasmtime or another backend can require deployment assets beyond installed C++ headers:
  list them and test the actual deployment bundle.

## Migration Plan

Implement library identity migration first, then minimal host-tool build/install, then generation
and BDK integration. Existing header-only consumers must not acquire unrelated runtime links.
Before acceptance, add precise organization specification deltas for the existing layout,
library registration and external dependency rules affected by the selected implementation.

## Open Questions

- **O1.** What host-toolset and prebuilt-tool override syntax matches the existing b2 conventions?
- **O2.** Which external dependencies, languages and install assets are required by the selected generator/backend implementation?
- **O3.** Does the existing prefix/staging interface need extension, and what platform matrix will be supported initially?
- **O4.** What compatibility redirects and proposal-root migration accompany the library rename?
