# Beast implementation plan

> **For agentic workers:** Use superpowers:executing-plans for the Beast work and superpowers:systematic-debugging for existing failures. Track independently verified increments; do not report the complete 53-example matrix from an initial working backend.

**Goal:** Compile Beast-shaped applications to native, WASIp2, WASIp3 and browser WebAssembly, using host services where available.

**Architecture:** Reuse upstream Beast's data types and native/socket protocol implementations. Keep browser WebSocket/fetch and WASI HTTP adapters separate from guest socket/TLS dependencies. Host-entry and supplied-stream examples preserve application behaviour with explicit transport changes.

**Tech Stack:** C++20, Boost 1.92, B2, wasi-sdk 34, Wasmtime, Emscripten 6.0.11, JavaScript WebSocket/fetch, WASI HTTP 0.2.12/0.3.0.

**Spec:** `../design.md` and `../specs/beast/spec.md`.

## Global constraints

- Public namespace `webcpp::beast`; macros `WEBCPP_BEAST_*`; public configuration includes webcpp config first.
- Errors as values; exceptions on/off; caller allocators and associated completion executors.
- Native remains the oracle. Keep all 53 original executable entries and report adapted applications separately.
- No claim of browser support from Node execution alone; no claim of runtime support from compilation alone.
- Tests use bounded controlled peers, deterministic payloads, explicit trust and cleanup.
- English files, BSL-1.0 source notices, no em dash, no hand edits of generated artifacts or CHANGELOG.md files.
- Current user authorization starts implementation and fixes the previous CI failures. No new remote run or push is required by this plan.

## Decisions at implementation start

- The user authorized implementation after the recommendation to reuse upstream data types. Keep those types; remove O6 as a blocker. This does not require executing upstream protocol code for host HTTP/WebSocket.
- The convenience header exposes portable data interfaces. Optional socket/TLS and host-entry headers are included explicitly, so message-only WASIp2 code does not import the missing socket reactor.
- Begin with a verified native/WASIp3 socket baseline and portable messages, then implement host backends. This is an implementation order, not a reduction of the required target matrix.
- Use a local repository at `libs/beast` until the organization repository exists, following the new-library checklist. Do not add an unpublishable gitlink to the superproject.

## Review focus

1. A message-only component must not acquire raw-socket or OpenSSL dependencies.
2. A connection that stalls must terminate by the declared deadline, including asynchronous shutdown.
3. A split or binary payload must survive buffering, framing and target conversions unchanged.
4. Cancellation/destruction must complete once without a late callback reaching freed memory.
5. Compilation variants must use matching dependency headers, exception mode and thread runtime.

## Task 1: Restore the existing build contract

**Files:** `tools/openssl/`, `tools/webcpp-openssl.jam`, `tools/test/openssl_test.py`, `tools/lint/`, `tools/boost/cobalt-wasip3/`, `tools/ci/`, affected Asio documentation and Cobalt usage requirements.

**Interfaces:** Preserve `/webcpp/third_party//openssl`, `/webcpp//boost_cobalt`, and CI matrix output contracts. Reproduce run 41563713062 from its recorded commands and artifacts.

- [x] Reproduce OpenSSL symlinked compiler/source-integrity failure; add regression and fix compiler selection/diagnostic ordering.
- [x] Implement MSVC/nmake recipe with correct archives, runtime variants and tool environment; test commands locally and identify Windows-only validation still needed.
- [x] Reproduce bare-throw fixture and Cobalt lint compilation failures; correct the fixture/commands without disabling the real checks.
- [x] Fix WASIp3 launch capabilities, Cobalt consumer header/library mismatch, missing container tools, and site title; add focused regressions.
- [x] Run affected complete tool suites and local native/WASIp3 application tests; record exact commands and results.
- [x] Commit reviewed corrections separately from the Beast library.

## Task 2: Portable data interfaces and native/WASIp3 socket baseline

**Files:** `libs/beast/build.jam`, `include/webcpp/beast.hpp`, `include/webcpp/beast/{config,core,http,websocket,ssl}.hpp`, `test/Jamfile`, `test/{config_test,message_test,socket_test}.cpp`, metadata and library instructions.

**Interfaces:** `webcpp::beast::http` exposes upstream message/parser/serializer types; `flat_buffer` retains upstream identity. Explicit socket headers expose `tcp_stream` and `websocket::stream<NextLayer>`; the latter accepts `std::allocator_arg_t` while retaining the documented native upstream allocation behaviour. TLS exposes `ssl_stream` and `set_server_name(stream, host) -> boost::system::result<void>` only when its dependency is selected.

- [x] Write config/message tests, including type identity, parser limits, binary content, and malformed messages; run before headers exist to observe the missing API.
- [x] Add minimal configuration and data aliases; run B2 on native, WASIp2, WASIp3 and Emscripten, exceptions on/off.
- [x] Write a bounded loopback HTTP request/response and WebSocket echo/close test using the proposed public socket interface; observe the missing socket API.
- [x] Implement native/WASIp3 aliases and wrapper; run real loopback exchanges in both exception modes and available threading profiles.
- [x] Add optional TLS helper with valid/error hostname checks; verify host-service/message builds have no OpenSSL dependency.
- [x] Add executable example and record its actual output, commands and initial feature matrix. Commit local library independently.

## Task 3: Browser WebSocket transport

**Files:** `include/webcpp/beast/browser/`, `src/browser_src.cpp`, browser tests/driver and the corresponding B2 targets.

**Interfaces:** Browser `tcp_stream` records resolver results, executor and timeout; TLS records policy; `websocket::stream` supplies Beast-shaped async operations, backed by JavaScript WebSocket and webcpp::asio's event bridge.

- [ ] Pin default queue sizes in the design with measured boundary tests; implement URL/TLS/unsupported-setting validation and category values 1 through 7.
- [ ] Implement connect/handshake, message read/write, binary/text, close, queue limits, cancellation and destructor completion, one contract at a time with a failing test first.
- [ ] Use controlled local WebSocket peers and a real browser; run JSPI/Asyncify and EH on/off, allocator counting, invalid URL, rejected handshake, timeout and late-event tests.
- [ ] Port the applicable upstream client examples and retain browser-unavailable server facilities in the manifest.

## Task 4: Host HTTP and supplied streams

**Files:** reusable WASI request/response/stream extensions in `libs/wasi`, Beast host adapters and example counterparts; browser fetch adapter and tests.

**Interfaces:** Preserve Beast request/response operations; map directly to fetch or pinned WASI outgoing interfaces. Server entry adapters invoke the application handler once per host request, commit one response, and retain body/resource ownership until completion.

- [ ] Extend reusable WASI interfaces beyond method/target/content-type/whole-body; test P2 resource ownership and P3 stream/future completion independently.
- [ ] Implement host client and server adapters without Asio raw sockets or guest OpenSSL; test headers, request bodies, response bodies, trailers, cancellation and failure after commitment.
- [ ] Implement browser fetch request/response mapping and optional byte adapter; test HEAD/204/304, CORS, unsupported fields, redirects, limits and cancellation.
- [ ] Implement host-supplied input/output channel and stdin/stdout application counterpart; test partial I/O, EOF, bounded buffering and cancellation with concurrent draining.

## Task 5: Complete application coverage and documentation

**Files:** checked upstream executable manifest, adapted examples/drivers, feature/results matrix, AsciiDoc guide, MrDocs configuration and public API comments.

- [ ] Reconcile all 53 executable targets, support sources and snippet harnesses against the pinned upstream source.
- [ ] Build/run native oracle and every promised target/variant/backend counterpart with exact adaptation diffs and evidence.
- [ ] Resolve WASIp2 socket/TLS and stackful coroutine prerequisites in their owning libraries; never hide blocked rows.
- [ ] Publish the Diataxis guide with compiled examples and observed outputs, plus independent feature and application matrices.
- [ ] Run local documented validation, perform independent review and fix regressions. Report any runtime/platform still unverified before publication.

## Verification observed for the initial increment

- Native Clang 21 Release: data/socket/SNI tests and example, EH on/off, threading single/multi.
- SDK 34 WASIp2 Release: data tests/example, EH on/off; no guest networking claim.
- SDK 34 WASIp3 Release: data/socket/SNI tests/example, single EH on/off and cooperative multi EH off. Wasmtime 49.0.3 ran the cooperative binaries; 47.0.3 rejected their component encoding.
- Emscripten 6.0.11 Release: data tests/example under Node, EH on/off; no browser transport claim.
- Independent review found missing default/converting WebSocket constructors and facade rebinding. Real construction, conversion and move regressions now pass.
- OpenSSL integration suite: nine cases passed, including real native source builds and consumers. Source-integrity suite: seven passed. Windows orchestration suite: four passed. Windows compilation/execution remains unverified.
- Lint framework suite: all 59 cases passed, including the new clean-checkout Cobalt overlay case and the bare-throw fixture. Actual combined-tree lint is a separate check.
- CI matrix/actions/report/site assembler suites passed locally. Six previously failing WASIp3 runtime tests passed with the CI-pinned Wasmtime 47.0.3 and corrected capabilities.
- The document Cobalt example passed with matching headers and compiled library. A Linux amd64 container built OpenSSL from source without network access. Source and rendered Asio title validation passed.
- No new CI run, merge, push or release was performed. Run 41563713062 remains the old failed remote result until a later run verifies the fixes.

## Initial implementation record

The first local Beast commit is `c7dfe2bffb16559aa4f5a6e3bece633e6999b0b4` on
`work/beast-and-wasmtime-50`. `source/` is an exact archive of its tracked tree. The
organization repository and superproject gitlink remain pending. This records the
initial increment, not completion of the browser, host-HTTP or 53-application work.

The owner's subsequent project-wide upgrade selected Wasmtime 50.0.1. Fresh P2/P3
data, socket, SNI, example and cooperative runs passed with that runtime; the
47.0.3/49.0.3 comparisons above remain historical evidence. The superproject's
`docs/superpowers/plans/2026-11-02-wasmtime-50.md` records the broader validation.
Combined lint checked 897 commands over 406 translation units, and affected
documentation was regenerated with MrDocs and passed source/rendered checks.

The superproject corrections and Wasmtime upgrade are recorded in local commit
`684c4feed137af2e1912fecff75119bad8c509c8`, separate from the Beast repository.
The broad P2/P3 logs retain the initial Lexical/Config failures; fresh focused
runs of every corrected target passed. No remote CI run or push followed.
