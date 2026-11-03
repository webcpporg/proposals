## 1. Resolve the draft and measure the first profile

- [ ] 1.1 Inventory existing wasi and the accepted redesign; write a coordinated migration table.
- [ ] 1.2 Spike one WIT resource and string/result operation on native and in a real browser.
- [ ] 1.3 Select pinned tool/backend versions, type profile, dependency policies and async model.
- [ ] 1.4 Measure static conversion separately and record its supported features and limitations.
- [ ] 1.5 Resolve design questions O1-O8 and record acceptance before implementation.

## 2. Migrate wasi to wasm

- [ ] 2.1 Add migration and portable aggregate include checks before changing the library.
- [ ] 2.2 Coordinate repository/submodule rename, headers, namespaces, macros and build targets.
- [ ] 2.3 Reconcile and preserve the accepted redesign and all current HTTP behaviour.
- [ ] 2.4 Update live CI, metadata, docs, references and site redirects using their generators.
- [ ] 2.5 Run clean native, wasip2, wasip3, Emscripten header and existing HTTP lanes.

## 3. Implement generic generation and support

- [ ] 3.1 Add a minimal component fixture and failing generation/diagnostic/reproducibility tests.
- [ ] 3.2 Add tools/wasm-bindgen with explicit contract/world/namespace/backend selection.
- [ ] 3.3 Generate C++ types, resource ownership, string/list conversion and error handling.
- [ ] 3.4 Add native component backend support and missing-import/trap/cleanup tests.
- [ ] 3.5 Add Emscripten integration and real-browser resource/error/lifetime tests.
- [ ] 3.6 Verify bounded allocation/error policies and document engine dependency boundaries.

## 4. Build and install

- [ ] 4.1 Implement companion organization wasm-tools tasks and clean-prefix tests.
- [ ] 4.2 Test host-tool execution during Emscripten/WASI cross compilation.
- [ ] 4.3 Verify installed generation and consumption outside the source checkout.

## 5. Add the BDK example

- [ ] 5.1 Pin BDK and Rust/component toolchain versions, Cargo.lock and third-party notices.
- [ ] 5.2 Add deterministic Rust reference fixtures for descriptor/address operations and errors.
- [ ] 5.3 Implement the example Rust wrapper and WIT world; build a component reproducibly.
- [ ] 5.4 Invoke the generic generator from the example build for example::bdk_wallet.
- [ ] 5.5 Run the same C++ operations natively and in a real browser against Rust expectations.
- [ ] 5.6 Verify repeated resource disposal, malformed input, independent instances and errors.

## 6. Document and verify

- [ ] 6.1 Publish installation, generation, migration and BDK walkthroughs from tested examples.
- [ ] 6.2 Publish the measured support matrix and distinguish bindings from static packaging.
- [ ] 6.3 Run clean declared lanes, tool/build tests, strict docs, lint and installed consumers.
- [ ] 6.4 Add reference code, exact measurements and resumable STATUS before acceptance.
