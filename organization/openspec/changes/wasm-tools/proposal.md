## Why

The companion library draft `libraries/wasi/openspec/changes/wasm-bindgen/` evolves wasi into
wasm and adds `webcpp-wasm-bindgen`. A consumer needs the generator installed with webcpp,
including when the application target is Emscripten or WASI and cannot execute on the build
machine. Library-only rules do not define that superproject responsibility.

## What Changes

- Register the renamed `wasm` library and migrate the `libs/wasi` submodule, live build/CI/site
  references and documentation links in coordination with the library change.
- Build `libs/wasm/tools/wasm-bindgen/` as the host-native `webcpp-wasm-bindgen` executable.
- Include the tool in the superproject's `b2 install`: executable under `<prefix>/bin`, headers
  under `<prefix>/include/webcpp`, required support assets under `<prefix>/share/webcpp/wasm`.
- Define host/target tool selection, reproducible generation, clean-prefix installation tests
  and diagnostics for missing build dependencies.
- Keep generated bindings and component build outputs in build directories, not source control.

## Capabilities

### New Capabilities

- `wasm-tools`: host tool building, installation and migrated library registration.

### Modified Capabilities

None. The new capability adds the host-tool contract and a coordinated identity migration;
the existing organization requirements are reconciled before acceptance, as design.md states.

## Impact

- **Kind:** organization build and installation change, paired with the wasm library draft.
- **Implementation:** not started; intended implementation in https://github.com/webcpporg/webcpp
  with generator sources in the library repository, currently https://github.com/webcpporg/wasi.
- **Status:** draft. Host/target selection syntax and backend/tool dependency policy are open.
- **Other proposals:** the library's `wasm-bindgen` draft owns generated API and BDK behaviour;
  this change owns superproject integration. No rename, installation or runtime code is made here.
- **Policy impact:** compiled backend/tool dependencies and any non-C tool implementation need
  explicit reconciliation with current project rules before acceptance.
