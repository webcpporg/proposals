# Tasks

Each group is a task of `reference/plan.md` (24 tasks, 0 to 23), and each item one of its steps
in the plan's order, which holds the files, interfaces, code and expected output. A checked item
is done in `reference/source/` (the library, 13 commits, tip `7cfd449`) or, for the tasks marked
"superproject", in the 10 superproject commits of the asio branch (tip `df52767`; the commit
subjects are in `reference/STATUS.md`). The library's commits are named by their short hashes.
All of them are of 2026-10-11, as `R`.

## 0. The ledger and the baseline (plan: Task 0)

- [x] 0.1 Create the ledger with its first line (plan: Task 0, Step 1); kept outside the repositories
- [x] 0.2 Record the baseline: superproject commit, submodules, declared targets and lanes, em++ 6.0.11, node v26.7.0, Cobalt's sources, the lint (plan: Task 0, Step 2). The lint failed on one rule, the banned word in the committer field of 13 older superproject commits, left as found and escalated to the owner

## 1. The library's skeleton (plan: Task 1; library 307d5e6)

- [x] 1.1 Create the repository `libs/asio` and the worktree (plan: Task 1, Step 1)
- [x] 1.2 See the build not find the library (Step 2)
- [x] 1.3 Write the failing `test/config_test.cpp` (Step 3)
- [x] 1.4 Write `build.jam` (Step 4)
- [x] 1.5 Write `include/webcpp/asio/config.hpp` and `include/webcpp/asio.hpp` (Step 5)
- [x] 1.6 Write `test/Jamfile` (Step 6)
- [x] 1.7 Write the repository's files: README, AGENTS.md, LICENSE, `meta/libraries.json`, `.gitattributes`, `.gitignore`, the CI workflow (Step 7)
- [x] 1.8 Write the page: `doc/Jamfile`, `asio.adoc`, `overview.adoc`, `testing.adoc` (Step 8)
- [x] 1.9 Run the native and emscripten lanes (Step 9): native 4 of 4, emscripten 4 of 4 and the page green, as the ledger records
- [x] 1.10 Commit (Step 10): `307d5e6`, the root commit, 19 files

## 2. The suspension feature and the test support (superproject; plan: Task 2)

- [x] 2.1 Failing cases for the variants in `tools/test/emscripten_test.py` (plan: Task 2, Step 1)
- [x] 2.2 See them fail (Step 2)
- [x] 2.3 The propagated composite feature `webcpp-suspend` in `tools/webcpp.jam` (Step 3)
- [x] 2.4 The two cases pass (Step 4)
- [x] 2.5 The test support's fixture library `node_test_demo` (Step 5)
- [x] 2.6 Failing cases for the run's statuses (Step 6)
- [x] 2.7 `tools/node-test/node-pre.js` and `webcpp/test/exit_status.hpp` (Step 7)
- [x] 2.8 The cases pass (Step 8)
- [x] 2.9 Every existing library adopts the support in this task (Step 9): pratt's and trystero's emscripten lanes pass within the bound
- [x] 2.10 Document it in AGENTS.md, chapter 9 (Step 10)
- [x] 2.11 Commit (Step 11): `74a5592`, with `aac46de` (matrix_test reads a site of its own), `27743d9` (exit_status ends a test with its first status) and `5f46c28` (a test whose main never runs fails, and a factory flag among others is refused), the review's fixes

## 3. Names Boost's way, the variants' manifest and the size table (superproject; plan: Task 3)

- [x] 3.1 Failing `tools/variants/variants_test.py` (plan: Task 3, Step 1)
- [x] 3.2 See it fail (Step 2)
- [x] 3.3 `tools/variants/variants.py`: `manifest` and `table` (Step 3)
- [x] 3.4 Four cases pass (Step 4)
- [x] 3.5 Failing b2 cases for the names and the variants (Step 5)
- [x] 3.6 `webcpp.suspend-tag`, `webcpp.variants` and `webcpp.size-table` in `tools/webcpp.jam` (Step 6)
- [x] 3.7 The b2 cases, `emscripten_test.py`, `webcpp_jam_test.py` pass (Step 7)
- [x] 3.8 Document it in AGENTS.md (Step 8)
- [x] 3.9 The CI keeps the size tables as the artifact `sizes-<lane>` (Step 9)
- [x] 3.10 Commit (Step 10): `43ebf62`; two minors of its review were carried into task 4's dispatch and committed then: `0a229fc` (a variants manifest whose script is missing is refused) and `5d4e860` (a page program's variants asked for with JavaScript exceptions are refused: fixing the review minor showed a real defect, an asyncify request that linked the jspi artifact with JavaScript exceptions)

## 4. The io_context and the timers (plan: Task 4; library 9ba6bd6)

- [x] 4.1 Write the page's section `doc/names.adoc` (plan: Task 4, Step 1)
- [x] 4.2 Write the failing `test/names_test.cpp` (Step 2)
- [x] 4.3 See it fail (Step 3)
- [x] 4.4 Implement `io_context.hpp`, `steady_timer.hpp`, `system_timer.hpp`, `basic_waitable_timer.hpp` (Step 4)
- [x] 4.5 Run `names` natively and on emscripten (Step 5)
- [x] 4.6 Commit (Step 6): `9ba6bd6`

## 5. The variants' flags, the reactor and the exit status (plan: Task 5; library d1864ed, bd99586)

- [x] 5.1 Write the page's section `doc/building.adoc` (plan: Task 5, Step 1)
- [x] 5.2 Write the failing tests: `reactor_test.cpp`, `never_finishes_test.cpp`, `return_after_suspension_test.cpp` (Step 2)
- [x] 5.3 Implement the variants' flags in `build.jam` first (Step 3)
- [x] 5.4 Run `reactor` on the emscripten lane (Step 4)
- [x] 5.5 Implement the reactor made at construction (Step 5)
- [x] 5.6 Run the emscripten and asyncify lanes whole (Step 6)
- [x] 5.7 Commit (Step 7): `d1864ed`; the review's fix `bd99586` (the tests and the reactor cite the page and the measurement, not the plan) and, in the superproject, `bc851f3` (the lint reads a program of JavaScript-based exceptions in clang 23's spelling) and `dfa65d2` (exit_status returns its status on emscripten without `std::exit`, so no test prints Emscripten's keepRuntimeAlive notice)

## 6. The run family and the turn limit (plan: Task 6; library 58af480, e56ea03)

- [x] 6.1 Write the page's section `doc/run.adoc` (plan: Task 6, Step 1)
- [x] 6.2 Write the failing `run_family_test.cpp` and `turn_limit_test.cpp` (Step 2)
- [x] 6.3 See them fail (Step 3)
- [x] 6.4 Implement `detail/browser_imports.hpp`, `browser/src.hpp`, `src/browser_src.cpp` and the run family (Step 4)
- [x] 6.5 Run the native, emscripten and asyncify lanes (Step 5)
- [x] 6.6 Commit (Step 6): `58af480`; the review's fix `e56ea03` (a timed run checks its deadline before each handler, and the turn limit is tested by counting the yields instead of racing a 5 ms timer)

## 7. The bridge (plan: Task 7; library ceb7fab, ed53b11, 2f3d29b)

- [x] 7.1 Write the page's section `doc/browser.adoc` (plan: Task 7, Step 1)
- [x] 7.2 Write `test/browser_values.hpp` (Step 2)
- [x] 7.3 Write the failing `bridge_test.cpp` (Step 3)
- [x] 7.4 See it fail (Step 4)
- [x] 7.5 Implement `browser/bridge.hpp` and the bridge's JavaScript in `browser/src.hpp` (Step 5)
- [x] 7.6 Run the emscripten and asyncify lanes (Step 6)
- [x] 7.7 Commit (Step 7): `ceb7fab`; review round 1 `ed53b11` (`delivering()` reads the count the JavaScript functions keep, no public `unwound`, the event's handle is the boundary's, a trap and the runtime's control flow are left to Emscripten) and round 2 `2f3d29b` (comments and page say what the refusals hold, `delivering()` pinned to take no value); superproject `df52767` (the bare-throw lint rule leaves EM_JS and EM_ASM JavaScript alone)

## 8. Who may call the run family (plan: Task 8; library cb5c3ce, ec99f2d)

- [x] 8.1 Write the failing `detection_test.cpp` (plan: Task 8, Step 1)
- [x] 8.2 See `detection_jspi_exceptions_on` fail (Step 2)
- [x] 8.3 Implement the admission, the flag and `detail/flag_scope.hpp` (Step 3)
- [x] 8.4 Run the eight detection programs, four in each lane (Step 4)
- [x] 8.5 Commit (Step 5): `cb5c3ce`; the review's fix `ec99f2d` (the delivery case posts a handler first so that it fails with assertions off too; the page says what a JavaScript error out of a handler leaves and which calls go undetected)

## 9. Exceptions across a suspension, and Asyncify's stack (plan: Task 9; library 27aa662, 7cfd449)

- [x] 9.1 Write the failing `exceptions_test.cpp` (plan: Task 9, Step 1)
- [x] 9.2 Run `exceptions_asyncify` with a 64-byte stack and see it trap (Step 2)
- [x] 9.3 Measure the stack size (Step 3): 8192, twice 4096, measured as the bytes each unwind wrote
- [x] 9.4 Run every lane (Step 4)
- [x] 9.5 Commit (Step 5): `27aa662`; fix round 1 `7cfd449` (the stack measured in every link, `asyncify-stack-size`, the bridge enters nothing after an abort)
- [x] 9.6 The scoped re-review of fix round 1, completed on 2026-10-14 with deterministic blocking-wait measurements, the run_until yield case and corrected ABORT comments (local library ad6367b; evidence in reference/STATUS.md)

## 10. Timers: Boost's, Beast's (plan: Task 10)

- [x] 10.1 Write the page's section `doc/timers.adoc` (plan: Task 10, Step 1)
- [x] 10.2 Write the failing `test/timers_test.cpp`: order 20, 40, 60 ms, a `system_timer`, cancellation, per-operation cancellation, Beast's WebSocket handshake timeout (Step 2)
- [x] 10.3 Run `timers` on each lane and see it can fail (Step 3)
- [x] 10.4 Commit (Step 4): local library `ded30cb`; native 14 and WebAssembly 70 passing markers, wrong-order negative controls on JSPI and Asyncify, six clang-tidy configurations and rendered documentation validated locally (reference/STATUS.md)

## 11. Allocation (plan: Task 11)

- [x] 11.1 Write the page's section `doc/allocation.adoc` (plan: Task 11, Step 1)
- [x] 11.2 Write the failing `test/allocation_test.cpp`: `operator new`, the C library and the aligned allocations counted, the runtime's own allocations kept out by name (Step 2)
- [x] 11.3 Run it with the allocator removed and see it fail, restore, run every lane (Step 3)
- [x] 11.4 Commit (Step 4): local library `a179adc`; native 15 and WebAssembly 74 passing markers, five scratch negative controls, six clang-tidy configurations and rendered documentation validated locally (reference/STATUS.md)

## 12. The loader (plan: Task 12)

- [x] 12.1 Write the page's part of `doc/building.adoc` (plan: Task 12, Step 1)
- [x] 12.2 Write the driven test's files in `test/browser/` (Step 2)
- [x] 12.3 Run the browser lane and see it fail (Step 3)
- [x] 12.4 Implement `include/webcpp/asio/webcpp-asio-loader.mjs` (Step 4)
- [x] 12.5 Run the browser lane and the lint (Step 5)
- [x] 12.6 Commit (Step 6)

## 13. The browser lane in headless Chrome (plan: Task 13)

- [x] 13.1 Superproject: `('asio', 'browser')` in `CHROME_LANES`, test first (plan: Task 13, Step 1)
- [x] 13.2 Write the idle, render and unlimited-render programs (Step 2)
- [x] 13.3 Write `page.html` (Step 3)
- [x] 13.4 Write the driver `run.mjs` with its three modes (Step 4)
- [x] 13.5 Declare the drives in the Jamfile (Step 5)
- [x] 13.6 Run the lane before and after the driver exists (Step 6)
- [x] 13.7 Record the turn measurement three times, under the stop rule (Step 7)
- [x] 13.8 Commit (Step 8)

## 14. The guide's examples and the size table (plan: Task 14)

- [x] 14.1 Write `doc/guarantees.adoc`, nine numbered guarantees (plan: Task 14, Step 1)
- [x] 14.2 Write the three examples with their `.expected` and the size table (Step 2)
- [x] 14.3 Run the native and emscripten lanes before and after the `.expected` files (Step 3)
- [x] 14.4 Write the quick start and include the examples (Step 4)
- [x] 14.5 Commit (Step 5)

## 15. Boost.Cobalt's sources (superproject; plan: Task 15)

- [x] 15.1 Failing tests in `tools/test/boost_libraries_test.py` and installer coverage in `tools/ci/actions_test.py` (plan: Task 15, Step 1)
- [x] 15.2 Implement `/webcpp//boost_cobalt` and the Boost action (Step 2)
- [x] 15.3 Run the tests and build the target for native, JSPI and Asyncify (Step 3)
- [x] 15.4 Document it in AGENTS.md (Step 4)
- [x] 15.5 Commit (Step 5)

## 16. Boost.Cobalt's `co_main` with the turn limit (plan: Task 16)

- [x] 16.1 Write the page's section `doc/cobalt.adoc` (plan: Task 16, Step 1)
- [x] 16.2 Write the failing `cobalt_test.cpp` and `cobalt_status_test.cpp` (Step 2)
- [x] 16.3 Implement `src/cobalt_main.cpp` and the targets `cobalt-main` and `cobalt` (Step 3)
- [x] 16.4 Run every lane (Step 4)
- [x] 16.5 Measure Asyncify's stack again with the Cobalt tests (Step 5)
- [x] 16.6 Write the twin example `asio_cobalt.cpp` (Step 6)
- [x] 16.7 Commit (Step 7)

## 17. Promises as operations (plan: Task 17)

- [x] 17.1 Write the page's section `doc/promises.adoc` with guarantees 10 and 11 (plan: Task 17, Step 1)
- [x] 17.2 Write the failing `promise_test.cpp` and `promise_cobalt_test.cpp` (Step 2)
- [x] 17.3 See them fail (Step 3)
- [x] 17.4 Implement `browser/error.hpp` and `browser/async_await.hpp` (Step 4)
- [x] 17.5 Run every lane and the lint (Step 5)
- [x] 17.6 Write the example `asio_promise.cpp` (Step 6)
- [x] 17.7 Commit (Step 7)

## 18. Events as operations (plan: Task 18)

- [x] 18.1 Write the failing `event_test.cpp` (plan: Task 18, Step 1)
- [x] 18.2 See it fail (Step 2)
- [x] 18.3 Implement `browser/async_wait_event.hpp` (Step 3)
- [x] 18.4 Run every lane and the lint (Step 4)
- [x] 18.5 Write the example `asio_event.cpp` (Step 5)
- [x] 18.6 Commit (Step 6)

## 19. Sockets and the transport point (plan: Task 19)

- [x] 19.1 Write the page's section `doc/sockets.adoc` with guarantee 12 (plan: Task 19, Step 1)
- [x] 19.2 Write the failing `sockets_test.cpp` and `transport_test.cpp` (Step 2)
- [x] 19.3 See them fail (Step 3)
- [x] 19.4 Implement `ip/tcp.hpp`, `ip/udp.hpp`, the three socket templates and `browser/transport.hpp` (Step 4)
- [x] 19.5 Run every lane and the lint (Step 5)
- [x] 19.6 Write the example `asio_sockets.cpp` (Step 6)
- [x] 19.7 Commit (Step 7)

## 20. The resolver, option A (plan: Task 20)

- [x] 20.1 Write the page's part of `doc/sockets.adoc` with guarantee 13 (plan: Task 20, Step 1)
- [x] 20.2 Write the failing `resolver_test.cpp` with the services table (Step 2)
- [x] 20.3 See it fail (Step 3)
- [x] 20.4 Implement `ip/basic_resolver.hpp` (Step 4)
- [x] 20.5 Run every lane and the lint (Step 5)
- [x] 20.6 Write the example `asio_resolver.cpp` (Step 6)
- [x] 20.7 Commit (Step 7)

## 21. The `ssl` names, and OpenSSL named by the program (plan: Task 21)

- [x] 21.1 Write the failing tests `ssl_test.cpp`, `ssl_without_openssl_test.cpp`, `openssl_on_webassembly_test.cpp`, `openssl_named_test.cpp` (plan: Task 21, Step 1)
- [x] 21.2 Implement `ssl.hpp`, `src/openssl_on_webassembly.hpp` and the `openssl` targets in `build.jam` (Step 2)
- [x] 21.3 Run every lane, `matrix_test.py` and the lint (Step 3)
- [x] 21.4 The form a program built for both targets writes, built by an example in `example/tls/` (Step 4)
- [x] 21.5 Write the page's parts (Step 5)
- [x] 21.6 Commit (Step 6)

## 22. The rulebook, the index and the page whole (plan: Task 22)

- [ ] 22.1 Reconcile CODING-GUIDELINES chapters 15/16/19 and AGENTS with the approved WASIp3/TLS scope (supersedes the old chapter 13 references)
- [ ] 22.2 Write the rows of the table of libraries (Step 2)
- [ ] 22.3 Keep the owner-approved Diataxis page order, generated MrDocs reference and final changelog appendix (supersedes the older overview-first order)
- [ ] 22.4 Check the text for the banned characters (Step 4)
- [ ] 22.5 Commit both repositories (Step 5)

## 23. Green, review and the gates (plan: Task 23)

- [ ] 23.1 Run the definition of green, each lane from scratch (plan: Task 23, Step 1)
- [ ] 23.2 Dispatch the whole-branch review (Step 2)
- [ ] 23.3 Fix what it finds, each with a failing test first (Step 3)
- [x] 23.4 Record the owner's five approvals of 2026-10-11, already present in reference/plan.md; later amendments supersede only the explicitly changed departures
- [x] 23.5 Gate G1: `webcpporg/asio` was created with owner authorization on 2026-11-03 (UTC)
- [x] 23.6 Gate G2: the owner-authorized submodule integration is recorded by superproject commit `2ba5116`
- [ ] 23.7 Gate G3: push the library, then the superproject, and watch both CIs (Step 6)

## Approved WASIp2 follow-up, 2026-11-03

- [x] P2.1 Implement the single-thread pollable core for timers and TCP/UDP; explicitly
  reject unsupported multi-thread targets. The initial Cobalt/guest TLS exclusions
  are superseded by the capability-completion extension below.
- [x] P2.2 Implement deferred forward async DNS with pollables, cancellation and handler
  associations; pin and verify the private wasi-libc descriptor ABI.
- [x] P2.3 Implement supplied CLI streams with partial I/O, readiness, cancellation,
  associated allocation/execution and explicit host-blocking boundaries.
- [x] P2.4 Reproduce and fix idle empty-poll trapping and write-permit narrowing; add
  readiness-at-registration and pending-write cancellation consumers.
- [x] P2.5 Commit local native/P3/P2 regressions, focused lint and rendered documentation;
  export the exact committed reference tree. See reference/STATUS.md for scope and counts.
- [ ] P2.6 Complete fresh remote CI and the existing integration gates for the current tree.


## Approved capability completion, 2026-11-03

- [x] C1 Complete WASI connected UDP gather and scatter receive, native P3 asynchronous
  DNS, and cancellation/resource-lifetime regressions.
- [x] C2 Enable the shared source-built OpenSSL/CAs and Cobalt/SSL targets on P2 with
  both exception profiles; retain the pinned SDK thread restriction.
- [x] C3 Enable explicit browser pthread execution and main-thread handoff, test both
  suspension mechanisms and document dependency-only counterexamples.
- [x] C4 Centralize derived capability macros and explicit requirement/header guards,
  update Diataxis/AsciiDoc/MrDocs documentation and the target/variant matrix.
- [x] C5 Complete local reviews/checks and commit/export the exact candidate. Publication
  gates remain separate; this extension does not claim new remote CI or a merge.
