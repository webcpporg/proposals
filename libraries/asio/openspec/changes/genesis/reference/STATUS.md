# webcpp::asio: where the implementation stands, and how to resume

## Task 9 re-review completed, 2026-10-14

This update supersedes the open task 9 review recorded below. Tasks 10 to 23 remain unstarted.
The historical sections below describe the imported 2026-10-13 baseline; `source/` now includes
this review's corrections, matching local library commit `ad6367b` on top of import `a9a48f0`.

The active superproject worktree is `asio-task9-review`, beside `webcpp` and `proposals` in the
organization directory, on branch `work/asio-task9-review`, tip `eba8545`. It starts at
`9a268b7` and carries the ten recorded superproject patches. Integration preserves both the
current Boost.Cobalt support and the node test support. `libs/asio` is its own local repository,
without a remote or a registered submodule. The historical patch files are unchanged.

### Findings and corrections

- An expired 1 ms timer still suspends in Emscripten's initial nonblocking poll, so the suspected
  missing-suspension failure was not reproduced. The real gap was a shorter measurement path:
  delaying entry by 25 ms changed `run_for` from 1104 to 1060 bytes and `run_until` from 1092 to
  1048, while the old test passed.
- The test now observes nonzero-timeout calls to `doPollAsync` and requires one for every
  reactor-wait case. With the old timer and the 25 ms delay, all eight wait assertions failed.
- Reactor cases now keep outstanding work until a JavaScript macrotask posts the completing
  handler. They retain the 25 ms entry delay as a regression case. Pending callbacks are
  cancelled at destruction, and the fixture cannot move or copy.
- The yield of `run_until` is measured separately. The bridge comments now state the actual
  guarantee, no entry after Emscripten sets `ABORT`, without claiming every raw Wasm trap sets it.
- An independent review checked the corrected instrumentation, callback lifetime and test
  evidence, and found no remaining blocking issue in this scoped task 9 review.

### Validation performed locally

With Emscripten 6.0.11, Node 26.7.0 and Boost 1.92:

- Native Asio suite: 13 passing test markers, built from scratch with Apple Clang 21.
- Asio JSPI and Asyncify suites: 66 passing test markers, built from scratch; expected compile
  and runtime failures were accepted only by their corresponding negative tests.
- After the final measurement correction, all six changed Asyncify stack builds were rebuilt
  from scratch and passed: debug, release and release linked with `-O3`, each with and without
  exceptions. There are eleven measured paths with exceptions and nine without.
- Their maxima (exceptions on/off) were 1164/844 bytes in debug, 2896/1940 in release and 384/288
  with the optimized link, below half of the configured 8192 bytes in every case.
- The five variants-tool cases and three selected superproject integration cases passed:
  suspension variants, node test support linkage and the variants manifest.
- clang-tidy passed all four compile configurations of the changed test; clang-format,
  changed-file text rules, source documentation checks and `git diff --check` passed.

This was a scoped local review, not the release gate: CI, a full organization lint, rendered
HTML generation, GCC and Chrome were not run. No remote repository, push, merge or CI run was
created. The user's instruction is to stop here before task 10.

### Resume task 10

Use the existing worktree and `libs/asio` repository. `.local/review-b2` supplies its isolated
Emscripten cache, Python 3.14 and `.local/user-config.jam`; it accepts ordinary b2 arguments.
Read the original plan's task 10 before starting. The toolchains are reused from the older
local superproject, and the build outputs remain isolated in this worktree's `bin/`.

## Historical handoff, 2026-10-13

Written 2026-10-13, when this proposal took the library in as a partly implemented genesis.
`spec.md` (approved by the owner on 2026-10-11, marked active) and `plan.md` (24 tasks, 0 to 23,
every task with its code) are the specification and the plan; `../tasks.md` is the checklist;
`source/` is the library's tree at the tip of its local repository, and
`superproject-patches/` the 10 superproject commits of plan tasks 2 and 3 and their fixes, as
`git format-patch` writes them from the rebased branch `asio-rebase` (on `97f71e0`), the
superproject side of the reference. The owner chose on 2026-10-13 to keep the library as this
proposal's reference code rather than publish it before its plan is complete. The library is not
published: no repository `webcpporg/asio` exists, no submodule, no release, and nothing was
pushed.

## Which copy is the reference

Two local copies exist; `source/` is the first.

| | Reference: `~/dev/webcpp-asio` | Rebased: `~/dev/webcpp-asio-rebase` |
| --- | --- | --- |
| Superproject worktree | branch `asio`, tip `df52767`, 10 commits on top of `5bc046f` (an older `main`) | branch `asio-rebase`, tip `d632294`, the same 10 commits rebased onto `97f71e0` (a later `main`; the superproject's `main` is now `c945be2`, 7 commits further) |
| `libs/asio` | its own repository (ignored by the superproject through `.git/info/exclude`, no remote), branch `main`, **13 commits, tip `7cfd449`** | a clone of the reference at **10 commits, `cb5c3ce`** (detached HEAD, working tree clean; git reports it detached from `2b6dccb`, the earlier hash of the same task 8 commit) |
| Tasks in `libs/asio` | 1, 4, 5, 6, 7, 8 and 9 | 1, 4, 5, 6, 7 and 8 without the fix round of its review |

`source/` is byte for byte the reference's `libs/asio` at `7cfd449` (`diff -r`, checked
2026-10-13), build output excepted. The rebased clone is what the reference was before three
commits: `ec99f2d` (task 8's fix round: the delivery case fails with assertions off too, and the
page says what a JavaScript error out of a handler leaves), `27aa662` (task 9: exceptions across
a suspension, Asyncify's stack measured) and `7cfd449` (task 9's fix round: the stack measured in
every link, the feature `asyncify-stack-size`, the bridge enters nothing after an abort). They
touch `AGENTS.md`, `build.jam`, `doc/browser.adoc`, `building.adoc`, `run.adoc`,
`browser/bridge.hpp`, `browser/src.hpp`, `config.hpp`, `detail/flag_scope.hpp` and
`test/Jamfile`, and add `test/asyncify_stack_test.cpp`, `asyncify_stack_size_test.cpp`,
`bridge_abort_test.cpp` and `exceptions_test.cpp`. The clone also lacks the built page `doc/html`.

The two superproject branches differ only in their base. The rebased one is the better base
for the superproject part: it sits on a later `main`, and its commits carry the changelog lines
that `doc/releases/0.1.0.adoc` asks for (added in an edit rebase, two commits that only change
tests getting none), which the reference's commits do not. Use the rebased superproject commits
with the reference's library.

The 10 superproject commits (reference hash, rebased hash, subject after "tools: "):

| Reference | Rebased | Subject | Plan task |
| --- | --- | --- | --- |
| `74a5592` | `7affa37` | the suspension variant on emscripten, and a test under node that ends as its main ends, within a bound | 2 |
| `aac46de` | `6024341` | (tools/ci) matrix_test's plan of every lane reads a site of its own, never the checkout's | 2 |
| `27743d9` | `deb9e86` | exit_status ends a test under node with its first status, and the test support's edges pinned | 2 |
| `5f46c28` | `a4e5bc8` | a test whose main never runs fails, and a factory flag among others is refused | 2 |
| `43ebf62` | `1a90ac9` | programs named Boost's way, a page program's variants, their manifest and their size table, kept by the CI | 3 |
| `0a229fc` | `a42ae04` | a variants manifest whose script is missing is refused, pinned by a case | 3 (carried to 4) |
| `5d4e860` | `e1f60cb` | a page program's variants asked for with JavaScript exceptions are refused | 3 (carried to 4) |
| `bc851f3` | `a7680d6` | the lint reads a program of JavaScript-based exceptions, the asyncify variant's, in clang 23's spelling | 5 |
| `dfa65d2` | `b15338d` | exit_status returns its status on emscripten too, without std::exit, so that no test prints Emscripten's keepRuntimeAlive notice | 5 |
| `df52767` | `d632294` | the bare throw rule leaves the JavaScript of an EM_JS function or an EM_ASM block alone | 7 |

The library's 13 commits, all of 2026-10-11, all as `R`, subjects after "asio: ": `307d5e6` the
library's skeleton (task 1); `9ba6bd6` the io_context and the timers, Boost's names (4);
`d1864ed` the variants' flags, the reactor made at construction, and the exit status of a
browser test (5); `bd99586` the tests and the reactor cite the page and the measurement, not the
plan (5); `58af480` the run family, a turn of ready handlers at a time, yielding through a
macrotask (6); `e56ea03` a timed run checks its deadline before each handler, and the turn limit
is tested by counting the yields (6); `ceb7fab` the bridge, through which a browser event only
posts (7); `ed53b11` and `2f3d29b` the bridge's two review rounds (7); `cb5c3ce` the run family
refuses a nested call and one made from a browser event (8); `ec99f2d` (8); `27aa662` (9);
`7cfd449` (9).

## Where it stopped, task by task

| Task | State |
| --- | --- |
| 0 the ledger and the baseline | done; the lint failed on one rule at the baseline (the committer field of 13 older superproject commits), left as found and escalated to the owner |
| 1 skeleton | done (`307d5e6`) |
| 2 suspension feature and test support (superproject) | done, with two fix rounds |
| 3 names, manifest, size table (superproject) | done |
| 4 io_context and timers | done |
| 5 variants' flags, reactor, exit status | done, one fix round |
| 6 run family and turn limit | done, one fix round (a timer race replaced by a deterministic yield count) |
| 7 bridge | done, two fix rounds |
| 8 who may call the run family | done, one fix round |
| 9 exceptions and Asyncify's stack | implemented, one fix round done; **the scoped re-review of the fix round was stopped** when the owner asked, on 2026-10-11 at 18:50, that every task be stopped and reviewed one by one |
| 10 to 23 | not started |

## What was measured

Recorded by the plan's ledger on 2026-10-11 (not re-run for this proposal):

- Task 9's fix round, from scratch with b2: native 13 and gcc-15 13 tests passed, emscripten
  JSPI 37, no-exceptions 37, Asyncify 29 (and the release variant, and release linked with
  `-O3`), the page 67, lint clean; 600 load runs of the Asyncify tests without a failure; a
  64-byte stack traps (the red step); four mutants killed.
- `ASYNCIFY_STACK_SIZE` 8192: the deepest suspension in any build b2 links wrote 3,540 bytes
  (release, compiled with `-O3` and linked without optimization), 1,272 in debug, 2,404 for the
  run family's deepest path linked without optimization and 384 linked with `-O3`.
- The rebased branch at its earlier base `b0d2043`: asio JSPI 34, Asyncify 16, no-exceptions 34,
  native 12, gcc-15 12, the other libraries native 115 and emscripten 64, config native 12 and
  emscripten 6, lint clean, the tools' tests 33 of 33.
- The spec's own probes (Emscripten 6.0.11, node 26.7.0): a self-posting handler runs 300,000
  times before a `setTimeout(0)` under Boost's `run`; with the limit of 1,000 the timeout ran at
  iteration 1,000 under JSPI and 999 under Asyncify; Asyncify's program is about 2.9 times JSPI's
  (283,409 against 97,429 bytes).

Verified by the author of this proposal on 2026-10-13, on the files of `source/`, by compiling
with Apple clang 21 and Emscripten 6.0.11 directly (not through b2, so not the lanes of the CI),
the include path being `source/include`, `source/test`, the superproject's
`tools/node-test/include` and Boost 1.92's headers, in a scratch directory outside every
repository:

- native: `names_test`, `flag_scope_test`, `run_family_test` and `exceptions_test` compile, run
  and print "No errors detected", exit 0; `flag_scope_test` also without exceptions;
  `config_test.cpp` compiles (`-fsyntax-only`) with exceptions, with `-fno-exceptions` and with
  `-DWEBCPP_ASIO_NO_EXCEPTIONS -DWEBCPP_TEST_ASIO_DEVELOPER_DEFINED`; `asio.hpp` and each of
  `config.hpp`, `io_context.hpp`, `steady_timer.hpp`, `system_timer.hpp` and
  `basic_waitable_timer.hpp` compile alone.
- Emscripten, `-O0`, `-DBOOST_ASIO_DISABLE_THREADS`, `-lembind`, `src/browser_src.cpp` linked, the
  support's `node-pre.js`, `-sNODERAWFS=1`, run by node v26.7.0: with `-sJSPI -fwasm-exceptions`,
  `turn_limit`, `reactor`, `run_family`, `bridge`, `detection` (with the assertion handler),
  `exceptions`, `bridge_abort`, `em_js` and `flag_scope` each print "No errors detected" and exit
  0; with `-sASYNCIFY -sASYNCIFY_STACK_SIZE=8192 -fexceptions`, `turn_limit`, `reactor`,
  `run_family`, `bridge`, `detection`, `exceptions`, `bridge_abort`, `em_js` and `asyncify_stack`
  do the same, and `asyncify_stack` printed 948, 324, 900, 1104, 428, 1092, 1004, 996, 968 and
  980 bytes of 8192 for its ten paths (a `-O0` build, so not b2's debug figure).
- Not reproduced: the no-exceptions builds (they link `boost::throw_exception`, which the
  superproject's build supplies), the compile-fail tests, `never_finishes` and
  `return_after_suspension`, gcc, the release variants, the lint, the page and anything in the
  superproject's tests.

## Differences between the source and the spec

- **Constructors:** five, not three (departure 2, approved): Boost's concurrency hint, with and
  without an allocator, is forwarded.
- **The bridge:** a registration is identified by a key (`std::uint64_t`, from 1), not an "id";
  `delivering()` reads the count the JavaScript functions keep in a token (`depth`), because an
  earlier public `unwound` let user code break the refusal and was removed; `deliver` is private
  with the export as its friend; `admitted()` is not `noexcept`.
- **Asyncify's stack:** 8192 and the feature `asyncify-stack-size`, which the plan did not list
  (task 9's fix round); the spec's "4,096 bytes" is Emscripten's default and is not asio's.
- **`flag_scope`** was deleted by task 7 and recreated by task 8, as the ledger says.
- **Guarantee anchors:** `asio-invariant-2` to `-7` are defined in `run.adoc` and `browser.adoc`;
  task 14 moves 2 and 3 into `guarantees.adoc` and adds 1, 8 and 9.
- **Pages and lanes announced early:** `README.md` and `doc/testing.adoc` name four lanes, the
  fourth (`browser`) not existing until tasks 12 to 14 (accepted as transitional by the plan's
  controller; task 14 checks the commands run).
- **Departures 1, 3, 4 and 5** (the Asyncify lane on every push, Cobalt without `io/ssl.cpp`,
  the system's OpenSSL shared and refused on WebAssembly, native no-exceptions runs only on
  Emscripten) concern tasks not started.

## What is open

- The scoped re-review of task 9's fix round (above), and the owner's one-by-one review of every
  task.
- Parked for the superproject by the ledger: the emscripten toolset should pass the optimization
  level to the link (then re-measure the stack: the deepest build becomes debug's); release
  artifacts ship with assertions on; the CI's asyncify lane should gain the release variant; a
  `run-fail` test passes on a trap, a test-infrastructure defect (a run-fail should name the
  failure it expects).
- Carried into task 14 by the ledger: `run.adoc`'s two invariant anchors, the citation of
  invariant 3 in `run_family_test.cpp` (it is wider than the turn limit) and the wording "fewer
  than twice the limit and one".
- The turn limit's default (1,000) is not measured in Chrome yet (task 13's stop rule).
- The gates G1 to G3 (repository, submodule, pushes) are the owner's.

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1): Boost 1.92 with its sources in
`.local/boost-source` (task 15 needs `libs/cobalt/src`), b2 5.5.3, a C++20 compiler, Python 3.9
or newer, wasi-sdk 34, wasmtime 47, Emscripten 6.0.11 from emsdk with node 26.7.0, MrDocs for the
documentation, chrome-headless-shell 155.0.8059.39 for task 13, OpenSSL 3 for task 21, and a
`.local/user-config.jam` registering the toolsets.

1. **A superproject worktree** from `main`, with the 10 superproject commits of the rebased
   branch applied in order (`git cherry-pick 97f71e0..asio-rebase` from the rebased worktree's
   repository, or rebase `asio-rebase` onto `main` again); resolve conflicts by keeping both
   sides, as the earlier rebase did (AGENTS.md's tools tree, `Jamroot`'s `explicit` line,
   `tools/webcpp.jam`).
2. **The library's repository.** On the machine that holds it, `libs/asio` of the reference
   worktree is the repository itself (13 commits). Elsewhere, create it from `source/` as one
   import commit (the superproject's rule: a repository starts from one import commit):

   ```bash
   mkdir libs/asio
   cp -R <proposals>/libraries/asio/openspec/changes/genesis/reference/source/. libs/asio/
   git -C libs/asio init -b main
   git -C libs/asio add -- $(cd libs/asio && find . -type f -not -path './.git/*' | sort)
   git -C libs/asio -c user.name=R -c user.email=github.overboard312@passmail.net \
       commit -m "asio: import, as the library stood after task 9"
   ```

   Until the owner creates `webcpporg/asio` and adds the submodule (G1, G2), the superproject
   ignores `libs/asio` through `.git/info/exclude`, never through a tracked line.
3. **Check the baseline** with the commands below (task 0's step 2); the lint's one failing
   rule at the first baseline concerned older commits, not these files.
4. **Close task 9's open review**, then start the next tasks. After task 9, the plan's
   dependencies leave these tracks free to run in parallel: tasks 10, 11 and 12 then 13
   (needing Beast's headers for 10); 15 then 16 (Cobalt); 17 then 18 (promises, events, on the
   bridge); 19 then 20 (sockets, resolver); 21 (ssl). Task 14 follows 3 to 11, and 22 and 23
   close.
5. Each task follows its section of `plan.md`: the failing test first, seen failing, the
   implementation, the lanes and the lint, the commit as `R`, by path, with no trailer and no
   co-author.

### The commands

From the superproject worktree's root. The plan writes `$B2` for
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.

| What | Command |
| --- | --- |
| one test, while working on it | `$B2 libs/asio/test//<name>` (`reactor`, `run_family`, `bridge`, ...) |
| native, from scratch | `$B2 -a --build-dir=bin/lane-native libs/asio/test libs/asio/example` |
| native with gcc 15 | `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/asio/test libs/asio/example` |
| emscripten (jspi) | `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test libs/asio/example` |
| emscripten without exceptions | `$B2 -a --build-dir=bin/lane-ehoff toolset=emscripten exception-handling=off libs/asio/test libs/asio/example` |
| the own lane asyncify | `$B2 -a --build-dir=bin/lane-asyncify toolset=emscripten libs/asio/test//asyncify libs/asio/example//asyncify` |
| the own lane browser, from task 12 | `$B2 -a --build-dir=bin/lane-browser toolset=emscripten libs/asio/test/browser//browser` |
| the page | `$B2 -a --build-dir=bin/lane-doc libs/asio/doc` |
| the lint | `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`, last line `lint: clean` |
| the tools' tests | `python3 tools/test/emscripten_test.py`, `tools/variants/variants_test.py`, `tools/test/webcpp_jam_test.py`, `tools/ci/matrix_test.py` |

Only a build from scratch (`-a`) counts as evidence, each lane in its own `--build-dir`.

## What to verify first

- That the superproject part builds on today's `main`: `emscripten_test.py` (the variants and the
  test support's statuses) and `variants_test.py`.
- That the library's tests still pass on every lane after the rebase, task 9's `asyncify_stack`
  in particular, which reads Emscripten's private runtime (`Asyncify.maybeStopUnwind`,
  `currData`, `StackSize`) and fails when a reading finds no unwind.
- That `ASYNCIFY_STACK_SIZE` 8192 still holds if the emscripten toolset's link has gained its
  optimization level.
