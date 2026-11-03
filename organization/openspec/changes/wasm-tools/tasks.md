## 1. Finalize integration contracts

- [ ] 1.1 Inspect existing install rules and host/target toolset handling.
- [ ] 1.2 Resolve dependency policy and write exact affected organization requirement deltas.
- [ ] 1.3 Agree host tool selection, prefix/staging conventions and supported install platforms.

## 2. Migrate library registration

- [ ] 2.1 Add regression checks for library discovery, live links and retained WASI lanes.
- [ ] 2.2 Coordinate libs/wasi to libs/wasm, relative URL and build target changes with the library.
- [ ] 2.3 Update CI, docs, metadata and site through their normal generators.

## 3. Build and install the host tool

- [ ] 3.1 Add failing tests for host-native generation during application cross compilation.
- [ ] 3.2 Register the host executable and explicit generation dependencies and outputs.
- [ ] 3.3 Add b2 install executable, headers and required support assets.
- [ ] 3.4 Test staged prefixes with spaces, missing dependencies and version mismatches.

## 4. Verify consumers and documentation

- [ ] 4.1 Generate/build/run a native minimal consumer outside the checkout from an installed prefix.
- [ ] 4.2 Generate/build/run an Emscripten consumer in a real browser from the installed tool.
- [ ] 4.3 Verify host-native tool execution for a WASI-targeted build without promising WASI hosting support.
- [ ] 4.4 Document deployment assets, dependency setup, migration and measured support.
- [ ] 4.5 Run clean build/tool tests, declared lanes, docs, lint and CI before implementation completion.
