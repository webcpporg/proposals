---
title: "webcpp::asio, increments 1 to 4: Implementation Plan"
tags: [webcpp, asio, emscripten, cobalt, plan]
status: draft
created: 2026-10-11
---

# webcpp::asio, increments 1 to 4: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** a new library, webcpp::asio, at `libs/asio`: Boost.Asio itself natively, and on
emscripten Boost.Asio's own `io_context` suspended into the browser's event loop, with what
Boost lacks there (the threads configuration, a reactor made at construction, a run family
that yields to the page every `max_handlers_per_turn` handlers, a bridge through which
browser events post), two suspension variants built side by side (`jspi`, `asyncify`) with a
loader that picks one, Boost.Cobalt's `co_main` given the turn limit, promises and events as
Asio operations, and sockets and a resolver that fail as values behind a transport point.

**Architecture:**
- **The library** is header-only, as every webcpp library, with two translation units the
  build compiles for a program: `src/browser_src.cpp` (the JavaScript of the yield, the
  bridge and the promise glue, and the export the bridge delivers through), which the
  usage requirements of `/webcpp/asio//asio` link on emscripten, and `src/cobalt_main.cpp`
  (webcpp's definition of `boost::cobalt::detail::main_promise::run_main`), which
  `/webcpp/asio//cobalt` links.
- **On emscripten** `webcpp::asio::io_context` derives from `boost::asio::io_context`; Boost's
  scheduler and `select_reactor` do the work, Emscripten suspends their `select`, and the
  class adds only an unarmed `steady_timer` member (the reactor at construction), the run
  family composed of `poll_one` and `run_one` with a `MessageChannel` yield, and a flag that
  refuses a nested or in-event call. Natively it is Boost's, with options that change
  nothing.
- **The superproject** gains the propagated composite feature `webcpp-suspend` (`jspi`, the
  default, with WebAssembly exceptions; `asyncify` with JavaScript exceptions), the tag rule
  that names an artifact Boost's way (`page-60-d-jspi.js`), the Jamfile rules
  `webcpp.variants` (both variants of a page program and the manifest the loader reads) and
  `webcpp.size-table`, and `/webcpp//boost_cobalt`, Boost.Cobalt's sources but `main.cpp`,
  which the Boost action now keeps.
- **The lanes:** native (every native lane of the CI), emscripten (the `jspi` variant, with
  and without exceptions), the own lane `asyncify` on emscripten (every browser test again
  under Asyncify, and the size table), and the own lane `browser` on emscripten (the
  loader under node, and headless Chrome: no polling, rendering continues, the loader picks
  the variant); and the own lane `tls` on native and emscripten (a program built for both
  targets that names OpenSSL for native builds alone, by the condition in its project).

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Asio, Beast, Cobalt, Core's lightweight_test), Emscripten
  6.0.11, node 26.7.0, Python 3 for the tools, chrome-headless-shell 155.0.8059.39 over the
  DevTools protocol.
- No new third-party dependency. Natively, `<webcpp/asio/ssl.hpp>` needs OpenSSL 3, which a
  program names in its own Jamfile (`/webcpp/asio//openssl`), found by `webcpp.external`.

**Spec:** `~/.buzz/PLANS/WEBCPP_ASIO_SPEC.md` (approved by the owner on
2026-10-11).
- The review and its probes: `~/dev/webcpp/.superpowers/sdd/plan-reviews/asio-beast-review.md`,
  `asio-beast-revision.md` and `asio-beast/` (`P/` below).
- This plan's own probes: `~/dev/webcpp/.superpowers/sdd/plan-reviews/asio-plan/` (`Q/`
  below), each measured on 2026-10-11 with em++ 6.0.11 and node 26.7.0:
  - `Q/b2-tag/`: `<tag>@rule` given as a usage requirement renames a dependent's
    executable (`chat-60-d-jspi.js`, `webcpp-suspend-asyncify/chat-60-d-asyncify.js`); a
    `make` target whose sources are `chat/<webcpp-suspend>jspi chat/<webcpp-suspend>asyncify`
    gets both artifacts and lands beside the `jspi` one; a test whose source is the
    executable's main target runs that very artifact.
  - `Q/alloc/alloc.cpp`: an `io_context` built with an allocator, an unarmed and an armed
    `steady_timer`, ten posts bound to an allocator: 0 calls of `operator new`, 18
    allocations through the given allocator, natively (Apple Clang) and on JSPI.
  - `Q/exitcode/`: a `main` that suspended and then returns 3 exits 3 under JSPI and **0
    under Asyncify**, with or without `-sEXIT_RUNTIME=1` (`exitcode.cpp`); `std::exit(3)`
    exits 3 on both (`exit_explicit.cpp`); a `main` that never finishes exits 0 under JSPI
    (`P/reenter.cpp`), and under Asyncify exits 0 even when the program set
    `process.exitCode = 1` first (`never.cpp`); a `process.on('exit')` listener that sets
    the status to 1 unless `main` said it finished gives 1 on both, and leaves an explicit
    `std::exit(0)` or `std::exit(4)` as it is (`never2.cpp`); built without a variant's
    flags, `select`'s `poll` busy-waits ("warning: non-zero poll() timeout not supported")
    and `main` keeps its value (`novariant.js` exits 3).
  - `Q/two-tu/`: an `EM_ASYNC_JS` import declared in a header with `EM_IMPORT` and defined
    in another translation unit suspends on both variants.
  - `Q/beast-timer/beast_timer.cpp`: Beast's WebSocket handshake timeout over
    `beast::test::stream` fires after 100 ms on both variants.
  - `Q/support/`: the pre-js of Task 2's test support (`node-pre.js`) over one program built
    without a variant, with `-sJSPI` and with `-sASYNCIFY` (`cases.cpp`): the statuses of
    Task 2's table, the watchdog's 3 s bound, and trystero's style of setting
    `process.exitCode` after `main` returned, kept.
- The independent review of this plan, `~/dev/webcpp/.superpowers/sdd/plan-reviews/asio-plan-review.md`
  and its probes (`asio-plan-review/`, `R/` there), measured the reserved-word `EM_JS`
  parameter, the nested `run` that waits for good, the exits inside a coroutine and a handler,
  and the setTimeout-or-MessageChannel order that is not fixed under node.

**Not in this plan:**
- **webcpp::beast's plan:** its streams (`tcp_stream`, `ssl_stream`, the browser
  `websocket::stream`, the HTTP client), the `teardown` and `async_teardown` overloads for
  webcpp::asio's sockets, the WebSocket and `fetch` lanes, and the reading of the `ssl`
  record that Task 21 writes.
- **webcpp::webrtc's plan:** WebRTC peer connections and data channels, on the bridge of
  Task 7.
- **The relay transports** and the b2 feature that enables one for a whole program
  (spec section 7: "designed with that library").
- **trystero on webcpp::asio** (spec increment 5): its event-side polling removed, its
  programs calling `run`; and moving trystero's Chrome driver and this plan's into one
  `tools/drive/chrome.mjs`.
- **A WASI backend** (`wasi:io/poll`, p3's async): Boost.Asio 1.92 does not compile for
  wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34 (`AGENTS.md`, chapter 9, xactor's example
  Jamfile), so webcpp::asio declares native and emscripten only and has no wasip2 or wasip3
  lane (spec section 3).
- **The CI's levels** (AGENTS.md chapter 13): until `matrix.py` takes a level, the own lane
  `asyncify` runs wherever the own lanes run, on every push; moving it to the nightly and
  release levels (spec section 11) is the levels' change, which Task 22 records in
  AGENTS.md chapter 13's levels item.
- **Offering upstream** to Boost.Asio what webcpp::asio proves (spec section 1.1).

## Departures from the spec

Each is recorded here and on the page where it shows. **The owner approved departures 1, 2,
3, 4 and 5 as written below on 2026-10-11** (for 4, the system's OpenSSL, shared, until
webcpp::crypto's `/third_party//openssl`; he rejected ignoring `/webcpp/asio//openssl` on
WebAssembly, which Task 21 now refuses with an instruction):

1. **Approved 2026-10-11. The own lane `asyncify` runs on every push** (spec section 11 settled: "the Asyncify
   lanes run at the nightly and release levels of the CI, not on every push"). Reason: the
   CI's levels do not exist yet (AGENTS.md chapter 13), and the alternative, no Asyncify in
   the CI at all, breaks the owner's "every variant tested" of 2026-10-10; running it on
   every push tests more, not less. It moves when the levels come (Task 22 writes it into
   chapter 13's levels item).
2. **Approved 2026-10-11. Two more constructors of `io_context`** (spec section 4.2 lists three): Boost's
   concurrency hint, with and without an allocator, forwarded, so that Boost's idiom
   `io_context io{1}` compiles and webcpp's Cobalt `run_main` keeps Cobalt's
   `BOOST_ASIO_CONCURRENCY_HINT_1` (Tasks 4 and 16).
3. **Approved 2026-10-11. `/webcpp//boost_cobalt` leaves out Cobalt's `io/ssl.cpp`** (spec section 5.2 says "every
   source but `main.cpp`"): it needs OpenSSL, which the program names (Task 21); a program
   that uses `boost::cobalt::io::ssl_stream` does not link with it until Cobalt's TLS sources
   are built against the program's OpenSSL, a later change (Tasks 15 and 16, the Cobalt page).
4. **Approved 2026-10-11 (its part a). `/webcpp/asio//openssl` links the system's OpenSSL,
   shared, with no static choice yet** (AGENTS.md chapter 6, item 7, asks for a propagated
   shared-or-static feature): the feature comes with `/third_party//openssl`, which
   webcpp::crypto's plan builds, and asio's target moves onto it then (Task 22 records it in
   chapter 13). The target is refused on WebAssembly, as item 7 asks (Task 21).
5. **Approved 2026-10-11. Native suites without exceptions run only on emscripten for now** (spec section 8:
   "built and tested both ways"): every browser test has its `_no_exceptions` twins, and the
   lint analyses every native program without exceptions; native test runs without
   exceptions wait for the CI's native no-exceptions lane, a levels item (AGENTS.md chapter
   13).
- **Offering upstream** to Boost.Asio what webcpp::asio proves (spec section 1.1).

## Global Constraints

**Repositories and paths:**
- `~/dev/webcpp` is the superproject (`git@github.com:webcpporg/webcpp.git`, branch `main`).
- `~/dev/webcpp/libs/asio` is the library's own git repository, created in Task 1, branch
  `main`. Until the owner creates `webcpporg/asio` and adds the submodule (gates G1 and G2),
  the superproject ignores it through `.git/info/exclude`, never through a tracked line.
- Machine-local: `~/dev/webcpp/.local/` (git-ignored): `user-config.jam`, `emsdk/` (6.0.11),
  `emscripten/node` (node 26.7.0), `emscripten-cache/`, `mrdocs/`, `wasi-sdk/`, `openssl/`,
  `boost-source/` (Boost 1.92's sources, which hold `libs/cobalt/src`). Task 15 adds
  `.local/boost-cobalt`.
- Every b2 command runs from `~/dev/webcpp`, as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`
  (the plan writes `$B2` for that). A verdict is `-a`. Each lane has its own `--build-dir`.
  No cap on concurrent builds (owner, 2026-10-11): a test that fails under load is flaky and is fixed, never protected by running less at once.
- The lanes, as the CI runs them:
  - native: `$B2 -a --build-dir=bin/lane-native libs/asio/test libs/asio/example`;
  - emscripten: `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test libs/asio/example`;
  - own lane `asyncify`: `$B2 -a --build-dir=bin/lane-asyncify toolset=emscripten libs/asio/test//asyncify libs/asio/example//asyncify`;
  - own lane `browser`: `$B2 -a --build-dir=bin/lane-browser toolset=emscripten libs/asio/test/browser//browser`;
  - own lane `tls` (Task 21), on native and emscripten:
    `$B2 -a --build-dir=bin/lane-tls libs/asio/example/tls//tls` and the same with
    `toolset=emscripten` and `--build-dir=bin/lane-tls-emscripten`.
- The lint: `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`, ending `lint: clean`.

**The library's rules (AGENTS.md chapters 2, 3, 6, 7, 8, 9, 11; spec):**
- Names: namespace `webcpp::asio`; `detail` for what is not public; macros `WEBCPP_ASIO_*`;
  test macros `WEBCPP_TEST_*`; include guards `WEBCPP_ASIO_<HEADER>_HPP`; b2 target
  `/webcpp/asio//asio`; error category `webcpp.asio`; `snake_case` everywhere.
- `<webcpp/asio/config.hpp>` first in every public header, with
  `WEBCPP_ASIO_NO_EXCEPTIONS`; an API that throws sits behind `#ifndef
  WEBCPP_ASIO_NO_EXCEPTIONS` and raises through `boost::throw_exception`, never a bare
  `throw`.
- Every library header line that names Boost.Asio or Beast (`boost/asio/`), a clock
  (`steady_clock`), a timer (`steady_timer`) or `this_thread::` says why after
  `lint-world:`. Every call of `run`, `run_one` or `run_for` on an `io_context`, and every
  `io_context::run_one` written out, says why after `lint-run:`; in programs:
  `io.run();  // lint-run: the program's event loop, suspended into the browser's`.
- Fixed-width types; `std::size_t` for counts; `int` only where Boost's interface asks
  (a protocol's `family()`).
- Errors as values: `boost::system::error_code` and `boost::system::result<T>`;
  `webcpp::asio::browser::error::promise_rejected` is value 1 of category `webcpp.asio`,
  never reused.
- Run-time objects allocate through their user's allocator: the `io_context` through the
  one its constructor takes, an operation through its handler's associated allocator, the
  bridge's map through the context's allocator. The counting test lists, by name, what the
  runtime allocates with its own `malloc` (spec section 8).
- Text is `std::string_view` where nothing must own it.
- A function starts with its guards. A comment says what the code does not.
- Doc Comments: a one-sentence brief, `@param`, `@tparam`, `@return`, only the allowed
  commands; every public symbol documented (AGENTS.md chapter 7).
- No global or static mutable state in the library. A test's replaced `operator new` and
  its counters are global, each line with
  `// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)` and the reason,
  as `libs/trystero/test/allocation_test.cpp` does.
- Tests use `<boost/core/lightweight_test.hpp>`; every test's `main` ends with
  `return webcpp::test::exit_status(boost::report_errors());` (`<webcpp/test/exit_status.hpp>`,
  Task 2), which every `webcpp.run`, `webcpp.run-fail` and `webcpp.example` gets from
  `/webcpp//node-test` with the pre-js that fails a test under node whose `main` did not
  finish, and stops one past its bound of 120 s (Review Focus 1 and 7).
- Values typed by the plan are the spec's or measured: the default `max_handlers_per_turn`
  is 1000; `ASYNCIFY_STACK_SIZE` starts at Emscripten's default, 4096 (`E/src/settings.js:834`),
  and Task 9 measures it; the reactor's longest wait is 300 s.

**Commits and text:**
- Every commit-writing git command is
  `git -c user.name=R -c user.email=github.overboard312@passmail.net commit ...` (in the library,
  `git -C libs/asio -c user.name=R -c user.email=github.overboard312@passmail.net commit ...`), with
  no trailer of any kind (no `Co-authored-by`, no `Signed-off-by`) and never an agent
  co-author.
- After each commit, `git log -1 --format='%an %ae %cn %ce'` prints
  `R github.overboard312@passmail.net R github.overboard312@passmail.net`, and
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage and commit by path (`commit -m "<message>" -- <paths>`). Never `git add -A`,
  `git add .` or `git add libs` in the superproject.
- Subjects: `asio: <what>` in the library; `tools: <what>`, `Jamroot: <what>`,
  `AGENTS.md: <what>`, `tools/ci: <what>` in the superproject.
- Everything written to a file is English. No em dash (U+2014) anywhere. The word the lint
  bans (`tools/lint/rules.py`) appears nowhere, in files or commits.
- Every source file opens with the licence notice (`//` for C++, JavaScript and AsciiDoc,
  `#` for Jam, Python, shell and YAML, the `<!--` form for HTML).

**Gates (the controller alone, each with the owner's agreement first; never a subagent):**
- **G1:** the owner creates `webcpporg/asio`, public, BSL-1.0.
- **G2:** the owner adds the submodule: `git submodule add ../asio.git libs/asio`, and
  removes the `.git/info/exclude` line.
- **G3:** pushes: the library first, then the superproject.
- A subagent never pushes, never creates a repository, never changes a GitHub setting, and
  never prints an environment variable (no `env` alone, `export`, `printenv`, `set`).

**Names fixed by this plan:**

| Name | What it is |
| --- | --- |
| `/webcpp/asio//asio` | the library: headers, Boost's, and on emscripten `BOOST_ASIO_DISABLE_THREADS`, `-lembind`, `browser-src`, the variant's link flags and the tag |
| `/webcpp/asio//browser-src` | the explicit object of `src/browser_src.cpp` |
| `/webcpp/asio//cobalt` | a Cobalt program's target: `asio`, `cobalt-main` and `/webcpp//boost_cobalt` |
| `/webcpp/asio//cobalt-main` | the static library of `src/cobalt_main.cpp` |
| `/webcpp/asio//openssl` | what a program names to use `<webcpp/asio/ssl.hpp>` natively: OpenSSL 3, `WEBCPP_ASIO_HAS_OPENSSL`; the build stops without it |
| `/webcpp/asio//openssl-tests` | the same for asio's own tests, left out on a machine that lacks OpenSSL |
| `/webcpp//boost_cobalt` | Boost.Cobalt's sources but `main.cpp` and `io/ssl.cpp`, static |
| `webcpp-suspend` | the propagated composite feature: `jspi` (default), `asyncify` (composes `<exception-handling-method>js`) |
| `webcpp.suspend-tag` | the tag rule, `<tag>@webcpp%suspend-tag` |
| `webcpp.variants <name> : <sources> + : <requirements> *` | an explicit page program `<name>` and its manifest `<name>.variants.json` |
| `webcpp.size-table <name> : <variants> +` | the explicit `<name>.sizes.md`, the size of each variant of each program, which the CI keeps as the artifact `sizes-<lane>` |
| `tools/variants/variants.py` | `manifest` and `table`, which those two rules run |
| `-sBOOST_COBALT_SOURCE=<dir>`, `.local/boost-cobalt` | where `/webcpp//boost_cobalt` finds Cobalt's sources |
| `include/webcpp/asio/webcpp-asio-loader.mjs` | the loader: `supportsJspi`, `pick`, `load` |
| `webcpp_asio_yield`, `webcpp_asio_function`, `webcpp_asio_await`, `webcpp_asio_string`, `webcpp_asio_report` | the JavaScript imports `browser/src.hpp` defines and `detail/browser_imports.hpp` declares |
| `webcpp_asio_deliver` | the export the bridge's JavaScript functions call |
| `/webcpp//node-test`, `tools/node-test/node-pre.js`, `<webcpp/test/exit_status.hpp>` | the superproject's test support on emscripten: `webcpp::test::exit_status`, `webcpp::test::bound` (Task 2) |
| `libs/asio/test/browser_values.hpp` | the tests' JavaScript values (`EM_JS`), included by one source per program |
| own lanes `asyncify`, `browser`, `tls` | `asyncify` and `browser` on emscripten (ids `emscripten.asio.asyncify`, `emscripten.asio.browser`); `tls` on native and emscripten (`native.asio.tls`, `emscripten.asio.tls`) |
| `~/dev/webcpp/.superpowers/sdd/WEBCPP_ASIO_PLAN/progress.md` | the ledger |

## Review Focus

1. **A browser test that fails and reads as passed.** Under node, a `main` that suspended and
   then returns non-zero exits 0 under Asyncify, and a `main` that never finishes (a `run`
   that never returns) exits 0 on both variants, under Asyncify even with
   `process.exitCode` set first (measured, `Q/exitcode/`, `P/reenter.cpp`). Every lane of
   `asyncify` would be green whatever it tested, and so would beast's, webrtc's and
   trystero's once on asio. Task 2 puts the remedy in the build, where no library can forget
   it: every `webcpp.run`, `webcpp.run-fail` and `webcpp.example` links a pre-js under which
   a `main` that did not finish fails, and `<webcpp/test/exit_status.hpp>`; its fixture pins
   each case's exact status. Task 5 pins both hazards again in asio's lanes, and Task 16 makes
   webcpp's `run_main` exit explicitly under Asyncify, pinned by a `run-fail` Cobalt program
   whose status is read from its output.
2. **The run family's inputs at their edges.** A `run_for` or `run_until` whose deadline
   passes while handlers are always ready (a self-posting handler) must return at the
   deadline, though every turn reaches the limit and yields; a `max_handlers_per_turn` of 0
   must not yield forever without running anything (it is taken as 1); a `stop()` made by
   the handler that reaches the limit returns at once, without yielding first. Task 6 tests
   the three.
3. **An exception that leaves the run family.** A handler's exception propagates out of
   `run`, as Boost's; the flag that refuses a nested call must be cleared on the way, or the
   program's next `restart(); run();` after its `catch` is refused as nested. Task 8 tests it
   on both variants.
4. **Browser events that come late.** An event of a listener the bridge removed, an event of
   a bridge whose `io_context` was destroyed, and a promise that settles after its
   `io_context` was destroyed must find nothing and corrupt nothing; an event wait pending at
   the destruction removes its listener. Task 7 tests the first two; Task 17 the third;
   Task 18 the fourth.
5. **Values at the browser boundary.** A promise already settled, a value that is no promise,
   a rejection whose reason is `undefined`; an event type given as a view into a longer
   string; resolver services `"WS"`, `"65536"`, `""` (port 0, as Boost's resolver gives),
   `"80x"` and `"0"`. Tasks 17, 18 and 20 test them.
6. **A public run-family member that calls another is refused by its own flag.** `run_for`
   written as a call of the public `run_until` would set the flag and then trip it: every
   `run_for` would assert, or return 0 having run nothing. Each public member admits exactly
   once and calls only private, unchecked code. Task 8 tests every timed form, with handlers
   that span several turns and with only a timer pending, on both variants.
7. **A browser test that waits forever instead of failing.** A `run` that never returns
   keeps node alive on the reactor's 300 s `setTimeout`, again and again, and b2 gives
   `webcpp.run` no bound. Task 2's pre-js stops every test past 120 s (its fixture's
   `never_finishes` past 3 s); Task 8's nested call is a bounded `run_for`, so its failing
   step fails rather than hangs.
8. **An allocation that leaves the user's allocator through the C library.** Asio's default
   handler memory ends in `aligned_alloc` (macOS, glibc) or Boost.Align (Emscripten), never
   in `operator new`, so a test that counts only `operator new` reads 0 for an unbound
   handler. Task 11 counts the C library's functions too, with a control case that sees the
   unbound handler's allocation.

---

## Task 0: The ledger and the baseline

**Files:**
- Create: `~/dev/webcpp/.superpowers/sdd/WEBCPP_ASIO_PLAN/progress.md`

- [ ] **Step 1:** Create the ledger with the first line
  `# SDD ledger - plan: ~/.buzz/PLANS/WEBCPP_ASIO_PLAN.md`.
- [ ] **Step 2: Record the baseline:**
  - `git -C ~/dev/webcpp rev-parse HEAD` and `git -C ~/dev/webcpp submodule status`;
  - `$B2 -d0 declared-targets` and `$B2 -d0 declared-lanes`;
  - `.local/emsdk/upstream/emscripten/em++ --version | head -1` (expected: the line ends
    `6.0.11 (a0014542110d6078c3a1a7941fa1ddb3a2281f16)`) and `.local/emscripten/node --version`
    (expected: `v26.7.0`);
  - `ls .local/boost-source/libs/cobalt/src` (expected: `channel.cpp detail error.cpp io
    main.cpp this_thread.cpp thread.cpp`);
  - the lint: expected `lint: clean`. A failure found here is fixed first, in its own
    commit, as AGENTS.md chapter 10 asks.

## Task 1: The library's skeleton

**Files:**
- Create (in `~/dev/webcpp/libs/asio`, a new git repository):
  - `LICENSE_1_0.txt` (a copy of the superproject's), `README.md`, `AGENTS.md`,
    `.gitattributes`, `.gitignore`, `.github/workflows/ci.yml`, `meta/libraries.json`;
  - `build.jam`;
  - `include/webcpp/asio.hpp`, `include/webcpp/asio/config.hpp`;
  - `test/Jamfile`, `test/config_test.cpp`, `test/.clang-tidy`;
  - `example/Jamfile`, `example/.clang-tidy`;
  - `doc/Jamfile`, `doc/asio.adoc`, `doc/overview.adoc`, `doc/testing.adoc`.
- Modify: `~/dev/webcpp/.git/info/exclude` (the line `/libs/asio/`).

**Interfaces:**
- Produces: the project `/webcpp/asio` and its target `asio`; the macro
  `WEBCPP_ASIO_NO_EXCEPTIONS`; the page `libs/asio/doc/html/index.html`.

- [ ] **Step 1: Create the repository.**

```bash
mkdir -p ~/dev/webcpp/libs/asio && cd ~/dev/webcpp/libs/asio && git init -b main
cp ../../LICENSE_1_0.txt .
echo /libs/asio/ >> ~/dev/webcpp/.git/info/exclude
```

- [ ] **Step 2: See the build not find it:** `$B2 libs/asio/test`. Expected: FAIL, b2 cannot
  find a Jamfile in `libs/asio/test`.
- [ ] **Step 3: Write the failing test** `test/config_test.cpp`, pratt's with the names
  changed (`libs/pratt/test/config_test.cpp`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/asio/config.hpp> defines WEBCPP_ASIO_NO_EXCEPTIONS exactly when the
// build has no exceptions, and keeps the developer's own definition in a build
// that has them. test/Jamfile builds this file twice: as the build asks, and
// with the macro defined as a developer defines it, on the command line, which
// WEBCPP_TEST_ASIO_DEVELOPER_DEFINED tells this file.

#include <webcpp/asio/config.hpp>

// Boost.Config, which config.hpp includes, has said whether the build has
// exceptions: without it, BOOST_NO_EXCEPTIONS would never be defined.
#if defined(__GNUC__) && defined(__cpp_exceptions) == defined(BOOST_NO_EXCEPTIONS)
#error "config.hpp does not include <boost/config.hpp>, which defines BOOST_NO_EXCEPTIONS"
#endif

#ifdef WEBCPP_TEST_ASIO_DEVELOPER_DEFINED
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
#error "config.hpp drops WEBCPP_ASIO_NO_EXCEPTIONS, which the developer defined"
#endif
#elif defined(BOOST_NO_EXCEPTIONS) != defined(WEBCPP_ASIO_NO_EXCEPTIONS)
#error "WEBCPP_ASIO_NO_EXCEPTIONS is not defined exactly when the build has no exceptions"
#endif
```

- [ ] **Step 4: Write `build.jam`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# asio: Boost.Asio natively, and on emscripten Boost.Asio's own io_context
# suspended into the browser's event loop. A program uses
# /webcpp/asio//asio.

project /webcpp/asio ;

# On emscripten Asio needs BOOST_ASIO_DISABLE_THREADS: without it,
# <boost/asio/io_context.hpp> stops at "Only Windows and POSIX are
# supported!" (B/asio/detail/signal_blocker.hpp:27). Its inline namespace
# differs with and without threads, so a translation unit that missed the
# define fails at link. The browser's values reach C++ through embind:
# -lembind, a free feature, which a usage requirement keeps.
# tag::asio[]
alias asio : : : : <include>include <library>/boost//headers
  <toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS
  <toolset>emscripten:<linkflags>-lembind ;
# end::asio[]
```

- [ ] **Step 5: Write `include/webcpp/asio/config.hpp`**, pratt's
  (`libs/pratt/include/webcpp/pratt/config.hpp`) with `PRATT` as `ASIO` and the macro's Doc
  Comment saying: "asio raises an exception only where Boost.Asio's own interface throws: the
  throwing overloads of its sockets, resolver and `ssl::context` (an `error_code&` overload
  stands beside each), which are absent while this is defined." and
  `include/webcpp/asio.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every public header of asio but <webcpp/asio/ssl.hpp>, which natively brings OpenSSL.
*/
#ifndef WEBCPP_ASIO_HPP
#define WEBCPP_ASIO_HPP

#include <webcpp/asio/config.hpp>

#endif  // WEBCPP_ASIO_HPP
```

- [ ] **Step 6: Write `test/Jamfile`:**

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/asio//asio ;

import webcpp ;

webcpp.targets native emscripten ;

webcpp.headers-alone asio : ../include ;

# config.hpp defines WEBCPP_ASIO_NO_EXCEPTIONS exactly when the build has no
# exceptions, and keeps it when a developer defines it in a build that has
# them.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_ASIO_NO_EXCEPTIONS <define>WEBCPP_TEST_ASIO_DEVELOPER_DEFINED ;
```

  `test/.clang-tidy` and `example/.clang-tidy` are xactor's (`libs/xactor/test/.clang-tidy`,
  `libs/xactor/example/.clang-tidy`), with their comments. `example/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/asio//asio ;

import webcpp ;

webcpp.targets native emscripten ;
```

- [ ] **Step 7: Write the repository's files:**
  - `meta/libraries.json`:

```json
{
    "key": "asio",
    "name": "asio",
    "authors": ["R"],
    "description": "Boost.Asio natively, and in the browser Boost.Asio's own io_context suspended into the page's event loop.",
    "category": ["Concurrent", "IO"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

  - `.gitignore`: `doc/html/`, `node_modules`, `.node-modules/` (the browser lane's driver
    has a lockfile, Task 12);
  - `.gitattributes`: pratt's first two lines (`* text=auto eol=lf`, `*.bat text eol=crlf`);
  - `.github/workflows/ci.yml`: pratt's, with `library: asio`;
  - `README.md`: what asio is (spec section 1, in four sentences), how to build and test it
    inside the superproject (the four lane commands above; Task 21 adds the fifth, `tls`), where its page is published
    (`https://webcpporg.github.io/webcpp/libs/asio/`), and its licence;
  - `AGENTS.md`: pratt's opening paragraph; "What asio is" (a library of webcpp's own on
    Boost.Asio, `"port-of": null`, no oracle and no "Differences" appendix; its page and the
    spec are what a change is checked against); "Cautions": the run family is called on
    `main`'s stack alone; the bridge only posts; a test ends with
    `webcpp::test::exit_status(...)`, which the superproject's test support needs under
    Asyncify; the two translation units under `src/`.
- [ ] **Step 8: Write the page.** `doc/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

import webcpp ;

webcpp.doc asio : asio.adoc ;

# No single target builds every header: those of browser/ build only on
# emscripten, whose own headers hold only for wasm32's types, so the
# reference reads every header for Emscripten's target, as em++ compiles
# (/webcpp//emscripten-reference), with the define of build.jam that a
# requirement conditioned on the emscripten toolset does not give it.
webcpp.reference asio : <library>/webcpp//emscripten-reference
  <define>BOOST_ASIO_DISABLE_THREADS ;
```

  `doc/asio.adoc` opens with the licence notice and the attributes of AGENTS.md chapter 8
  (`= asio: Boost.Asio in the browser's event loop for {cpp}`, `:toc: left`,
  `:toclevels: 1`, `:idprefix:`, `:sectanchors:`, `:example-caption!:`,
  `:attribute-missing: warn`), one paragraph (what asio is, one of the libraries of
  webcpp), then `include::overview.adoc[]`, `include::testing.adoc[]` and

```
[#reference]
== Reference

include::{reference}[leveloffset=+1]
```

  `overview.adoc` (`[#asio-overview]`, "Why asio") holds spec section 1.1 as prose.
  `testing.adoc` (`[#asio-testing]`) says how asio is tested: the four lanes and their
  commands (five from Task 21, which adds `tls` and its two commands to it and to the README), `{n-tests}` tests, the report at `https://webcpporg.github.io/webcpp/report/`
  (`{n-examples}` joins in Task 14: a count that finds nothing fails the doc build).
- [ ] **Step 9: Run** `$B2 -a --build-dir=bin/lane-native libs/asio/test` and
  `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test`. Expected:
  PASS, `alone-asio`, `alone-asio-config`, `config` and `config_developer_defined` on each.
  Then `$B2 -a --build-dir=bin/b-doc libs/asio/doc`. Expected: PASS, the page written.
  Then the lint. Expected: `lint: clean`.
- [ ] **Step 10: Commit** (library), by path:

```bash
git -C libs/asio add -- LICENSE_1_0.txt README.md AGENTS.md .gitattributes .gitignore \
    .github meta build.jam include test example doc
git -C libs/asio -c user.name=R -c user.email=github.overboard312@passmail.net commit \
    -m "asio: the library's skeleton: its configuration, its page and its CI"
```

## Task 2: The suspension feature, and the test support every emscripten test links (superproject)

**Files:**
- Modify: `~/dev/webcpp/tools/webcpp.jam` (the feature and its composition; `.node-test` on
  `run`, `run-fail` and `example`), `~/dev/webcpp/Jamroot` (the target `node-test`).
- Create: `tools/node-test/node-pre.js`, `tools/node-test/include/webcpp/test/exit_status.hpp`;
  the fixture library `tools/test/fixtures/node_test_demo/` (`build.jam`, `test/Jamfile`,
  `test/returns.cpp`, `test/exits_zero.cpp`, `test/returns_after_suspension.cpp`,
  `test/never_finishes.cpp`, `test/exit_in_coroutine.cpp`, `test/exit_in_handler.cpp`).
- Modify: `tools/test/emscripten_test.py` (five cases, and `node_test_demo` among its
  fixtures), `AGENTS.md` (chapter 1's layout of `tools/`; chapter 9, after "Targets", and the
  rows of `webcpp.run`, `webcpp.run-fail` and `webcpp.example`).

**Interfaces:**
- Produces:
  - the propagated composite feature `webcpp-suspend`, values `jspi` (default) and
    `asyncify`, whose `asyncify` composes `<exception-handling-method>js`;
  - `/webcpp//node-test`, which `webcpp.run`, `webcpp.run-fail` and `webcpp.example` add to
    every program on every target: its usage requirements give `<include>tools/node-test/include`
    everywhere and, on emscripten, `--pre-js tools/node-test/node-pre.js`, so that no library's
    Jamfile can leave it out;
  - `tools/node-test/include/webcpp/test/exit_status.hpp`:

```cpp
namespace webcpp::test {
[[nodiscard]] int exit_status(int status);   // natively returns status; on emscripten marks
                                             // the run finished and calls std::exit(status)
void bound(std::chrono::seconds limit);      // on emscripten re-arms the watchdog; natively nothing
}
```

  - what the pre-js makes of a program under node (measured,
    `.superpowers/sdd/plan-reviews/asio-plan/support/`, Emscripten 6.0.11 and node 26.7.0,
    the same source built without a variant, with `-sJSPI` and with `-sASYNCIFY`):

| `main` | without suspension | JSPI | Asyncify |
| --- | --- | --- | --- |
| returns 3, no suspension | 3 | 3 | 3 |
| suspends, returns 0 | (aborts: no suspension built) | 0 | **1**, with the line below |
| suspends, returns 3 | (aborts) | 3 | **1**, with the line below |
| suspends, `exit_status(4)` | (aborts) | 4 | 4 |
| suspends, `std::exit(7)` unmarked | (aborts) | 7 | 7 |
| sleeps 600 s after `bound(3s)` | (aborts) | 1 after 3 s | 1 after 3 s |
| returns 1, a timer sets `process.exitCode = 0` later (trystero's tests) | 0 | 0 | 0 |

    The line, on standard error: `webcpp: main did not finish: it never returned, or it
    returned after a suspension under Asyncify, which loses the value; end it with
    webcpp::test::exit_status`; and for the bound:
    `webcpp: the test ran past its bound of <n> s`.
  - The bound is 120 s by default: longer than any emscripten test of webcpp today (Step 9
    checks pratt's and trystero's lanes under it), and short enough that a test that waits
    forever (a `run` whose reactor waits its 300 s, again and again) fails inside the CI job
    instead of running to the job's own timeout. A test that needs longer calls
    `webcpp::test::bound` with its own, and says why. The watchdog is `unref`'d: it ends only
    a test that something else keeps alive. It bounds a test that waits, not one that spins
    inside WebAssembly, which no JavaScript timer interrupts: a program built for a variant
    never spins on `select` (Task 5, Step 3).

- [ ] **Step 1: Write the failing cases** in `tools/test/emscripten_test.py`, added to
  `CASES`:

```python
def test_suspend_variants_build_apart(root):
    # jspi, the default, compiles with WebAssembly exceptions and adds no directory; asyncify
    # composes JavaScript exceptions, which Asyncify needs (an Asyncify program built with
    # -fwasm-exceptions traps, P/timer_catch.cpp), and builds in a directory of its own.
    result = harness.run_b2(root, '-a', '-n', '-d+2', *EMSCRIPTEN,
                            'libs/browser_demo/test//catches')
    harness.expect(result, True)
    assert '-fwasm-exceptions' in result.stdout and 'webcpp-suspend-' not in result.stdout, (
        result.stdout[-4000:])
    result = harness.run_b2(root, '-a', '-n', '-d+2', *EMSCRIPTEN, 'webcpp-suspend=asyncify',
                            'libs/browser_demo/test//catches')
    harness.expect(result, True)
    assert '-fexceptions' in result.stdout, result.stdout[-4000:]
    assert '-fwasm-exceptions' not in result.stdout, result.stdout[-4000:]
    assert '/webcpp-suspend-asyncify/' in result.stdout, result.stdout[-4000:]


def test_a_native_build_ignores_the_variant(root):
    # The feature exists on every toolset; natively its default adds nothing.
    harness.without_emscripten(root)
    result = harness.run_b2(root, '-a', 'libs/browser_demo/test')
    harness.expect(result, True)
    assert not [path for path in (root / 'bin').rglob('*') if 'webcpp-suspend' in path.name]
```

- [ ] **Step 2: Run** `python3 tools/test/emscripten_test.py test_suspend_variants_build_apart test_a_native_build_ignores_the_variant`.
  Expected: FAIL, b2 stops at `webcpp-suspend=asyncify`, a feature it does not know.
- [ ] **Step 3: Implement** in `tools/webcpp.jam`, after the existing features:

```jam
# The suspension variant of a program on emscripten (WEBCPP_ASIO_SPEC.md
# section 4.4): jspi, JavaScript Promise Integration, the default, with
# WebAssembly exceptions; or asyncify, Emscripten's Asyncify, which traps
# with WebAssembly exceptions and so composes JavaScript-based ones.
# Composite, since b2 takes only free features from usage requirements;
# propagated, so that every library a program uses is built for its
# variant. A non-default value builds in a directory of its own.
# tag::suspend[]
feature.feature webcpp-suspend : jspi asyncify : propagated composite ;
feature.compose <webcpp-suspend>asyncify : <exception-handling-method>js ;
# end::suspend[]
```

- [ ] **Step 4: Run** the two cases. Expected: PASS. Then the whole
  `python3 tools/test/emscripten_test.py` and `python3 tools/test/webcpp_jam_test.py`.
  Expected: PASS, every case.
- [ ] **Step 5: Write the test support's fixture**, a library of its own so that no count of
  browser_demo's page (`tools/test/doc_test.py`) changes. `build.jam`:
  `project /webcpp/node_test_demo ; alias node_test_demo : : : : <library>/boost//headers ;`
  after the licence notice. `test/Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The test support every program on emscripten links: a main that did not
# finish fails, a status is kept, and a test past its bound is stopped.
project : requirements <library>/webcpp/node_test_demo//node_test_demo
  <define>BOOST_ASIO_DISABLE_THREADS ;

import webcpp ;

webcpp.targets emscripten ;

local jspi = <linkflags>-sJSPI ;
local asyncify = <webcpp-suspend>asyncify <linkflags>-sASYNCIFY ;

webcpp.run returns_without_suspension : returns.cpp ;
webcpp.run exits_zero_after_suspension_jspi : exits_zero.cpp : $(jspi) ;
webcpp.run exits_zero_after_suspension_asyncify : exits_zero.cpp : $(asyncify) ;
webcpp.run-fail returns_one_after_suspension_jspi : returns_after_suspension.cpp
  : $(jspi) <define>WEBCPP_TEST_STATUS=1 ;
webcpp.run-fail returns_zero_after_suspension_asyncify : returns_after_suspension.cpp
  : $(asyncify) <define>WEBCPP_TEST_STATUS=0 ;
webcpp.run-fail never_finishes : never_finishes.cpp : $(jspi) ;
webcpp.run-fail exit_in_coroutine_asyncify : exit_in_coroutine.cpp : $(asyncify) ;
webcpp.run-fail exit_in_handler_jspi : exit_in_handler.cpp : $(jspi) ;
```

  The programs, each after the licence notice and a one-line comment:
  - `returns.cpp`: `int main() { return 0; }`;
  - `exits_zero.cpp`: `emscripten_sleep(10);` then
    `return webcpp::test::exit_status(0);`;
  - `returns_after_suspension.cpp`: `emscripten_sleep(10);` then `return WEBCPP_TEST_STATUS;`;
  - `never_finishes.cpp`: `webcpp::test::bound(std::chrono::seconds(3));`,
    `emscripten_sleep(600000);`, then `return webcpp::test::exit_status(0);`;
  - `exit_in_coroutine.cpp`: a coroutine type of its own (a `promise_type` with
    `std::suspend_never` at both ends and `unhandled_exception` that calls `std::terminate`),
    whose body calls `emscripten_sleep(10)` and then `webcpp::test::exit_status(3)`;
  - `exit_in_handler.cpp`: a `boost::asio::io_context` and a 10 ms `boost::asio::steady_timer`
    whose handler runs `try { (void)webcpp::test::exit_status(3); } catch (...) { }` (the
    review measured that neither WebAssembly's nor JavaScript's `catch (...)` stops the
    exit), then `io.run();  // lint-run: the test's event loop` and
    `return webcpp::test::exit_status(0);`.
- [ ] **Step 6: Write the failing cases** in `tools/test/emscripten_test.py` (the run's
  fixtures become `('browser_demo', 'node_test_demo')`):

```python
NODE_TEST = 'libs/node_test_demo/test'


def exit_status(root: Path, name: str) -> tuple[int, str]:
    """The status b2 recorded for the program name, and what it printed."""
    outputs = built(root, f'{name}.output')
    assert len(outputs) == 1, (name, outputs)
    text = outputs[0].read_text()
    found = re.search(r'^EXIT STATUS: ([0-9]+)$', text, re.MULTILINE)
    assert found, (name, text)
    return int(found.group(1)), text


def test_a_test_ends_as_its_main_ends(root):
    started = time.monotonic()
    result = harness.run_b2(root, '-a', '-d+2', *EMSCRIPTEN, NODE_TEST)
    harness.expect(result, True)
    assert passed(result) == {
        'returns_without_suspension', 'exits_zero_after_suspension_jspi',
        'exits_zero_after_suspension_asyncify', 'returns_one_after_suspension_jspi',
        'returns_zero_after_suspension_asyncify', 'never_finishes',
        'exit_in_coroutine_asyncify', 'exit_in_handler_jspi'}, result.stdout[-4000:]
    assert exit_status(root, 'returns_one_after_suspension_jspi')[0] == 1
    status, text = exit_status(root, 'returns_zero_after_suspension_asyncify')
    assert status == 1 and 'webcpp: main did not finish' in text, text
    status, text = exit_status(root, 'never_finishes')
    assert status == 1 and 'webcpp: the test ran past its bound of 3 s' in text, text
    assert exit_status(root, 'exit_in_coroutine_asyncify')[0] == 3
    assert exit_status(root, 'exit_in_handler_jspi')[0] == 3
    assert time.monotonic() - started < 600, 'the bound did not stop never_finishes'


def test_every_test_and_example_links_the_support(root):
    # webcpp.run, webcpp.run-fail and webcpp.example link the pre-js on emscripten; a program
    # webcpp.link links, which a browser runs, does not.
    result = harness.run_b2(root, '-a', '-d+2', *EMSCRIPTEN, 'libs/browser_demo/test',
                            'libs/browser_demo/example')
    harness.expect(result, True)
    assert 'node-pre.js' in link_line(root, result, 'reads')
    assert 'node-pre.js' in link_line(root, result, 'hello')
    assert 'node-pre.js' not in link_line(root, result, 'page')


def test_natively_the_support_is_a_header(root):
    harness.without_emscripten(root)
    result = harness.run_b2(root, '-a', '-d+2', 'libs/browser_demo/test//reads')
    harness.expect(result, True)
    assert 'tools/node-test/include' in result.stdout and 'node-pre.js' not in result.stdout
```

  Run them. Expected: FAIL: the fixture's programs do not compile
  (`webcpp/test/exit_status.hpp` not found), and no link line names `node-pre.js`.
- [ ] **Step 7: Implement.** `tools/node-test/node-pre.js`, the code measured in
  `asio-plan/support/node-pre.js`, with the licence notice and a comment of what it does:

```js
// webcpp's test support under node, which every test and example on emscripten links
// (--pre-js, tools/webcpp.jam): a test whose main did not finish fails, and a test that runs
// past its bound is stopped. In a browser, which has no process, it does nothing.
if (typeof process === 'object' && typeof process.on === 'function') {
    let finished = false
    let watchdog = null
    const bound = seconds => {
        if (watchdog) {
            clearTimeout(watchdog)
        }
        watchdog = setTimeout(() => {
            process.stderr.write(`webcpp: the test ran past its bound of ${seconds} s\n`)
            process.exit(1)
        }, seconds * 1000)
        watchdog.unref()
    }
    bound(120)
    Module['webcppTestBound'] = bound
    Module['webcppTestFinished'] = () => {
        finished = true
    }
    // After main returned: under Asyncify a main that suspended returns early, unwound, and
    // Asyncify.currData is then set; that return is not main's end.
    Module['postRun'] = [].concat(Module['postRun'] ?? [], () => {
        if (typeof Asyncify === 'undefined' || !Asyncify.currData) {
            finished = true
        }
    })
    // A status other than 0 is a failure already; only a 0 that main never said is replaced.
    process.on('exit', () => {
        if (!finished && !process.exitCode) {
            process.exitCode = 1
            process.stderr.write('webcpp: main did not finish: it never returned, or it returned ' +
                'after a suspension under Asyncify, which loses the value; end it with ' +
                'webcpp::test::exit_status\n')
        }
    })
}
```

  `exit_status.hpp` (include guard `WEBCPP_TEST_EXIT_STATUS_HPP`; `<chrono>`, `<cstdlib>`, and
  on emscripten `<emscripten/em_asm.h>`):

```cpp
namespace webcpp::test {

/**
 Ends main with status: natively returns it; on emscripten tells webcpp's test support that
 main finished, then exits.

 Tip: under Asyncify a main that suspended loses the value it returns, and node would exit 0
 (measured, asio-plan/exitcode); exit keeps it on both variants.
*/
[[nodiscard]] inline int exit_status(int status) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (Module['webcppTestFinished']) {
            Module['webcppTestFinished']();
        }
    });
    std::exit(status);
#else
    return status;
#endif
}

/** Re-arms the bound of this test under node to limit, from now; natively does nothing. */
inline void bound(std::chrono::seconds limit) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (Module['webcppTestBound']) {
            Module['webcppTestBound']($0);
        }
    }, static_cast<double>(limit.count()));
#else
    (void)limit;
#endif
}

}  // namespace webcpp::test
```

  The Jamroot, after `path-constant WEBCPP_ROOT : . ;`:

```jam
# What every test and example of a library links (tools/webcpp.jam): the
# header webcpp/test/exit_status.hpp on every target, and on emscripten the
# pre-js that fails a test under node whose main did not finish, and stops
# one past its bound (tools/node-test/node-pre.js). Quoted, as the Boost
# directory is, so that a path that holds a space builds. The pre-js is also a
# dependency of every program that links it, so that an edit of it relinks them
# in an incremental build (b2 does not read a link flag's file as a source).
alias node-test : : : : <include>tools/node-test/include
  <toolset>emscripten:<linkflags>"--pre-js \"$(WEBCPP_ROOT)/tools/node-test/node-pre.js\""
  <toolset>emscripten:<dependency>$(WEBCPP_ROOT)/tools/node-test/node-pre.js ;
```

  Measure the dependency once, without `-a`: build a test, touch `node-pre.js`, build again;
  the program must relink. If b2 refuses a plain file as `<dependency>`, declare it as a
  target (`make` or a `file` target wrapping it) and depend on that; the re-review's N1.

  (and `node-test` joins the `explicit` list). `tools/webcpp.jam`, beside `.node-raw-fs`:

```jam
# What every test and example links (Jamroot's node-test): no library's
# Jamfile can leave it out, and one that links -sJSPI or -sASYNCIFY gets it
# with the rest.
.node-test = <library>/webcpp//node-test ;
```

  added to the requirements of `run`, `run-fail` and the `exe` of `example`, never to
  `link`, `variants` or `drive` (a browser runs those, and a driver bounds its own).
- [ ] **Step 8: Run** the three cases, then the whole `emscripten_test.py`,
  `webcpp_jam_test.py`, `doc_test.py` and `lint_test.py`. Expected: PASS.
- [ ] **Step 9: Every existing library adopts it in this task**, since the rules link it:
  run pratt's and trystero's emscripten lanes from scratch
  (`$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/pratt/test libs/pratt/example`,
  and the same for trystero with `webcpp-require-external=on`). Expected: PASS, as before.
  A test the bound stops gets `webcpp::test::bound(<twice its measured time>)` in its own
  library, with the reason, in a commit of that library; record each in the ledger.
  xactor, xstate and wasi declare no emscripten. webcpp::beast and webcpp::webrtc inherit it
  by the same rules when they are born.
- [ ] **Step 10: Document** in AGENTS.md, chapter 9, after "Targets", a paragraph "**The
  suspension variant on emscripten.**" that includes the region `suspend` of
  `tools/webcpp.jam` (as a `[listing]` the rulebook quotes), says what each value builds
  with, and that a build asks for one with `webcpp-suspend=asyncify` or a program with the
  requirement `<webcpp-suspend>asyncify`; a paragraph "**A test under node ends as its main
  ends.**" with the table of Interfaces, the bound and `webcpp::test::bound`, and the rule:
  a test whose `main` may suspend under Asyncify ends through `webcpp::test::exit_status`;
  the rows of `webcpp.run`, `webcpp.run-fail` and `webcpp.example` say they link
  `/webcpp//node-test`; chapter 1's layout names `tools/node-test/`.
- [ ] **Step 11: Commit** (superproject): `"tools: the suspension variant on emscripten, and a
  test under node that ends as its main ends, within a bound"` with
  `Jamroot tools/webcpp.jam tools/node-test tools/test/emscripten_test.py tools/test/fixtures/node_test_demo AGENTS.md`.

## Task 3: Names Boost's way, the variants' manifest and the size table (superproject)

**Files:**
- Create: `tools/variants/variants.py`, `tools/variants/variants_test.py`,
  `tools/test/fixtures/browser_demo/test/variants/Jamfile`,
  `tools/test/fixtures/browser_demo/test/variants/shell.cpp`.
- Modify: `tools/webcpp.jam` (the tag rule; the rules `variants` and `size-table`, their
  actions), `tools/test/emscripten_test.py`, `.github/workflows/library.yml` (the own lanes'
  size tables kept), `AGENTS.md` (chapter 1's layout of `tools/`, chapter 9's Jamfile API and
  its table and the own lanes' job, chapter 10's list of tool tests, chapter 12's facts).

**Interfaces:**
- Consumes: `webcpp-suspend` (Task 2).
- Produces:
  - `rule suspend-tag ( name : type ? : property-set )`, used as `<tag>@webcpp%suspend-tag`:
    an executable on emscripten named `<base>-60-d-<variant>.js` (debug) or
    `<base>-60-<variant>.js` (release), every other target and toolset unchanged;
  - `webcpp.variants <name> : <sources> + : <requirements> * ;`: the explicit executable
    `<name>`, on emscripten alone, linked with `-sMODULARIZE -sEXPORT_ES6` and the tag, and
    the explicit `<name>.variants.json` beside its `jspi` artifact;
  - the manifest's form, which the loader (Task 12) reads:

```json
{
    "asyncify": {
        "bytes": 297636,
        "script": "webcpp-suspend-asyncify/page-60-d-asyncify.js"
    },
    "jspi": {
        "bytes": 102988,
        "script": "page-60-d-jspi.js"
    }
}
```

    (`script` relative to the manifest, with `/`; `bytes` the size of the `.wasm` beside the
    script; keys sorted, four spaces);
  - `webcpp.size-table <name> : <variants> + ;`: the explicit `<name>.sizes.md`, a Markdown table
    of every manifest, printed into the build's output too:

```
| program | jspi | asyncify | asyncify / jspi |
| --- | ---: | ---: | ---: |
| page | 102,988 | 297,636 | 2.89 |
```

  - `python3 tools/variants/variants.py manifest --out <file> --jspi <script> --asyncify <script>`
    and `python3 tools/variants/variants.py table --out <file> <manifest>...`.

- [ ] **Step 1: Write the failing test** `tools/variants/variants_test.py`, in the form of
  `tools/oracle/compare_test.py` (a `CASES` list, `main` running each in a temporary
  directory), with these cases:

```python
def scripts(root: Path) -> tuple[Path, Path]:
    """A jspi and an asyncify artifact as b2 lays them out, each with its .wasm."""
    jspi = root / 'page-60-d-jspi.js'
    asyncify = root / 'webcpp-suspend-asyncify/page-60-d-asyncify.js'
    for script, size in ((jspi, 102988), (asyncify, 297636)):
        script.parent.mkdir(parents=True, exist_ok=True)
        script.write_text('// glue\n')
        script.with_suffix('.wasm').write_bytes(b'\0' * size)
    return jspi, asyncify


def test_manifest_names_each_artifact_from_its_own_directory(root: Path) -> None:
    jspi, asyncify = scripts(root)
    out = root / 'page.variants.json'
    result = variants('manifest', '--out', out, '--jspi', jspi, '--asyncify', asyncify)
    assert result.returncode == 0, result.stderr
    assert json.loads(out.read_text()) == {
        'asyncify': {'bytes': 297636, 'script': 'webcpp-suspend-asyncify/page-60-d-asyncify.js'},
        'jspi': {'bytes': 102988, 'script': 'page-60-d-jspi.js'}}
    assert out.read_text().startswith('{\n    "asyncify": {\n'), out.read_text()


def test_manifest_refuses_an_artifact_without_its_wasm(root: Path) -> None:
    jspi, asyncify = scripts(root)
    asyncify.with_suffix('.wasm').unlink()
    result = variants('manifest', '--out', root / 'page.variants.json', '--jspi', jspi,
                      '--asyncify', asyncify)
    assert result.returncode == 1, result.returncode
    assert f'{asyncify.with_suffix(".wasm")} does not exist' in result.stderr, result.stderr
    assert not (root / 'page.variants.json').exists()


def test_table_lists_each_program_with_its_ratio(root: Path) -> None:
    jspi, asyncify = scripts(root)
    manifest = root / 'page.variants.json'
    variants('manifest', '--out', manifest, '--jspi', jspi, '--asyncify', asyncify)
    out = root / 'sizes.md'
    result = variants('table', '--out', out, manifest)
    assert result.returncode == 0, result.stderr
    table = ('| program | jspi | asyncify | asyncify / jspi |\n'
             '| --- | ---: | ---: | ---: |\n'
             '| page | 102,988 | 297,636 | 2.89 |\n')
    assert out.read_text() == table and result.stdout == table, (out.read_text(), result.stdout)


def test_table_refuses_a_manifest_that_is_not_one(root: Path) -> None:
    bad = root / 'bad.variants.json'
    bad.write_text('{"jspi": {}}\n')
    result = variants('table', '--out', root / 'sizes.md', bad)
    assert result.returncode == 1 and f'{bad}: ' in result.stderr, result.stderr
```

  with `variants(*arguments)` running `sys.executable tools/variants/variants.py` as
  `compare(...)` runs `compare.py`.
- [ ] **Step 2: Run** `python3 tools/variants/variants_test.py`. Expected: FAIL, `variants.py`
  does not exist.
- [ ] **Step 3: Implement** `tools/variants/variants.py`: `manifest` resolves each script's
  `.wasm` (`Path.with_suffix('.wasm')`), fails with status 1 and
  `variants.py: <path> does not exist` before writing anything, and writes
  `json.dumps(manifest, indent=4, sort_keys=True) + '\n'` with each `script` as
  `os.path.relpath(script, out.parent)` in POSIX form; `table` reads each manifest (one that
  lacks a variant, a `script` string or an integer `bytes` fails with `<path>: <what is
  missing>`), names each row by the manifest's file name less `.variants.json`, sorts rows
  by that name, writes the numbers with a thousands separator (`f'{n:,}'`) and the ratio
  with two decimals, and prints the table. Python 3.9-compatible, Pyright-clean, lines of
  100 columns at most, the licence notice and a docstring per function.
- [ ] **Step 4: Run** `python3 tools/variants/variants_test.py`. Expected: PASS, 4 cases.
- [ ] **Step 5: Write the failing b2 cases.** The fixture: `shell.cpp` is
  `int main() { return 0; }` after the licence notice, and its `Jamfile`, a directory of its
  own so that no program of browser_demo's `test/Jamfile` (whose counts `doc_test.py` reads)
  changes, and with neither rule recording a program:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# A program as a page loads it, in both suspension variants, named Boost's
# way, with its manifest, and their sizes.
import webcpp ;

webcpp.variants shell : shell.cpp ;
webcpp.size-table shell : shell ;
```

  and `emscripten_test.py` gains:

```python
def test_artifacts_are_named_boost_s_way(root):
    # common.format-name's tags joined by '-': the toolset's version alone (60), d for debug,
    # and the variant; a release drops the d; asyncify builds in its own directory.
    for request, name in (((), 'shell-60-d-jspi.js'),
                          (('webcpp-suspend=asyncify',), 'shell-60-d-asyncify.js'),
                          (('variant=release',), 'shell-60-jspi.js')):
        result = harness.run_b2(root, '-a', *EMSCRIPTEN, *request,
                                'libs/browser_demo/test/variants//shell')
        harness.expect(result, True)
        assert built(root, name), (request, name, sorted((root / 'bin').rglob('shell*.js')))
    asyncify = built(root, 'shell-60-d-asyncify.js')
    assert asyncify[0].parent.name == 'webcpp-suspend-asyncify', asyncify


def test_variants_write_the_manifest_beside_the_jspi_artifact(root):
    result = harness.run_b2(root, '-a', '-d+2', *EMSCRIPTEN,
                            'libs/browser_demo/test/variants//shell.variants.json')
    harness.expect(result, True)
    manifests = built(root, 'shell.variants.json')
    assert len(manifests) == 1, manifests
    manifest = json.loads(manifests[0].read_text())
    assert manifest['jspi']['script'] == 'shell-60-d-jspi.js', manifest
    assert manifest['asyncify']['script'] == (
        'webcpp-suspend-asyncify/shell-60-d-asyncify.js'), manifest
    for entry in manifest.values():
        wasm = (manifests[0].parent / entry['script']).with_suffix('.wasm')
        assert wasm.stat().st_size == entry['bytes'], (entry, wasm)
    # A page loads the glue as an ES module.
    assert '-sMODULARIZE' in link_line(root, result, 'shell-60-d-jspi'), result.stdout[-4000:]
    assert '-sEXPORT_ES6' in link_line(root, result, 'shell-60-d-asyncify')


def test_the_size_table_lists_every_variant(root):
    result = harness.run_b2(root, '-a', *EMSCRIPTEN,
                            'libs/browser_demo/test/variants//shell.sizes.md')
    harness.expect(result, True, '| shell |')
    tables = built(root, 'shell.sizes.md')
    assert len(tables) == 1 and tables[0].read_text().startswith('| program | jspi |'), tables


def test_variants_are_not_built_natively(root):
    harness.without_emscripten(root)
    result = harness.run_b2(root, '-a', 'libs/browser_demo/test/variants//shell.sizes.md')
    harness.expect(result, True)
    assert not built(root, 'shell.sizes.md') and not built(root, 'shell.variants.json')
```

  (`import json` at the top of the file.) Run them. Expected: FAIL, `webcpp.variants` is no
  rule.
- [ ] **Step 6: Implement** in `tools/webcpp.jam` (`import common ;` among its imports; after
  `link`; `.variants-script` is `[ path.native [ path.join $(.tools) variants/variants.py ] ]`,
  as `.runner` is):

```jam
# The name of a program on emscripten, Boost's way: its tags joined by '-',
# the toolset's version (60 for Emscripten 6.0.11, which common.jam knows by
# its version alone), d for a debug build, and the variant:
# page-60-d-jspi.js. Other targets, and other toolsets, keep their names.
rule suspend-tag ( name : type ? : property-set )
{
    if $(type) = EXE && [ $(property-set).get <toolset> ] = emscripten
    {
        return [ common.format-name <base> <toolset> <threading> <runtime>
            <property:webcpp-suspend> : $(name) : $(type) : $(property-set) ] ;
    }
}

# A program a page loads, in both suspension variants, and its manifest:
# the explicit executable <name>, on emscripten alone, linked as an ES
# module whose factory a page calls (-sMODULARIZE -sEXPORT_ES6), named
# Boost's way; and the explicit <name>.variants.json, beside the jspi
# artifact, which names each variant's script relative to itself and the
# size of its .wasm, which webcpp-asio-loader.mjs reads. Never run by
# node: no -sNODERAWFS=1.
rule variants ( name : sources + : requirements * )
{
    local only = <webcpp-targets>emscripten <conditional>@webcpp%filter ;
    exe $(name) : $(sources)
      : $(requirements) $(only) $(.handler) <tag>@webcpp%suspend-tag
        <linkflags>-sMODULARIZE <linkflags>-sEXPORT_ES6 ;
    make $(name).variants.json
      : $(name)/<webcpp-suspend>jspi $(name)/<webcpp-suspend>asyncify
      : @webcpp%write-variants : $(only) ;
    local found = [ where ] ;
    $(found[1]).mark-target-as-explicit $(name) $(name).variants.json ;
}

rule write-variants ( targets * : sources * : properties * )
{
    PYTHON on $(targets) = $(.python) ;
    SCRIPT on $(targets) = $(.variants-script) ;
}

actions write-variants
{
    "$(PYTHON)" "$(SCRIPT)" manifest --out "$(<)" --jspi "$(>[1])" --asyncify "$(>[2])"
}

# The sizes of the variants of each program webcpp.variants declared in this
# Jamfile under the names variants: the explicit <name>.sizes.md, a Markdown
# table the build also prints, and which the CI keeps from an own lane's job
# (.github/workflows/library.yml, the artifact sizes-<lane>).
rule size-table ( name : variants + )
{
    make $(name).sizes.md : $(variants).variants.json : @webcpp%write-size-table
      : <webcpp-targets>emscripten <conditional>@webcpp%filter ;
    local found = [ where ] ;
    $(found[1]).mark-target-as-explicit $(name).sizes.md ;
}

rule write-size-table ( targets * : sources * : properties * )
{
    PYTHON on $(targets) = $(.python) ;
    SCRIPT on $(targets) = $(.variants-script) ;
}

actions write-size-table
{
    "$(PYTHON)" "$(SCRIPT)" table --out "$(<)" "$(>)"
}
```

  Neither rule calls `program`: neither declares a test or an example, so neither is
  counted on a page nor listed by `declared-targets`.
- [ ] **Step 7: Run** the four cases, then the whole `emscripten_test.py`, `webcpp_jam_test.py`,
  `doc_test.py` and `variants_test.py`. Expected: PASS. Then the lint (Pyright and the Python
  rules read the new files). Expected: `lint: clean`.
- [ ] **Step 8: Document** in AGENTS.md: chapter 1's layout of `tools/` (`variants/`,
  `variants.py`, the manifest and the size table, and its test); chapter 9's Jamfile API, the
  two rules and a table row each, and the tag rule with the names `page-60-d-jspi.js`,
  `webcpp-suspend-asyncify/page-60-d-asyncify.js`, `page-60-jspi.js` (release); chapter 10's
  list of tool tests, `tools/variants/variants_test.py`; chapter 12, a fact: "**A `<tag>`
  given as a usage requirement renames the dependent's executable** (measured,
  `.superpowers/sdd/plan-reviews/asio-plan/b2-tag`), and a target whose sources name
  `<program>/<feature>value` builds that program with that value."
- [ ] **Step 9: Keep the size tables in the CI** (spec section 4.4: "the size table of the
  CI records"). In `.github/workflows/library.yml`, the job `own-lanes` gains, after "Upload
  the lane's XML":

```yaml
      # The size tables a lane wrote (webcpp.size-table), kept beyond the job.
      - name: Upload the lane's size tables
        if: always()
        uses: actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9 # v7.0.2
        with:
          name: sizes-${{ matrix.id || matrix.name }}
          path: bin/**/*.sizes.md
          if-no-files-found: ignore
```

  Run `actionlint -config-file .github/actionlint.yaml .github/workflows/library.yml`
  (actionlint 1.7.12, the CI's). Expected: no finding. AGENTS.md chapter 9's own lanes'
  job says it uploads `sizes-<id>`.
- [ ] **Step 10: Commit** (superproject): `"tools: programs named Boost's way, a page program's
  variants, their manifest and their size table, kept by the CI"` with `tools/webcpp.jam tools/variants tools/test/emscripten_test.py tools/test/fixtures/browser_demo/test/variants .github/workflows/library.yml AGENTS.md`.

## Task 4: The io_context and the timers, natively and on emscripten

**Files:**
- Create: `include/webcpp/asio/io_context.hpp`, `include/webcpp/asio/basic_waitable_timer.hpp`,
  `include/webcpp/asio/steady_timer.hpp`, `include/webcpp/asio/system_timer.hpp`,
  `test/names_test.cpp`, `doc/names.adoc`.
- Modify: `include/webcpp/asio.hpp`, `test/Jamfile`, `doc/asio.adoc`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::asio {
struct io_context_options {
    std::size_t max_handlers_per_turn = 1000;
};
class io_context : public boost::asio::io_context {
public:
    io_context();
    explicit io_context(io_context_options options);
    template <class Allocator>
    io_context(std::allocator_arg_t, const Allocator& allocator, io_context_options options = {});
    explicit io_context(int concurrency_hint, io_context_options options = {});
    template <class Allocator>
    io_context(std::allocator_arg_t, const Allocator& allocator, int concurrency_hint,
               io_context_options options = {});
    std::size_t max_handlers_per_turn() const noexcept;
    void max_handlers_per_turn(std::size_t n) noexcept;  // 0 is taken as 1
    // natively: Boost's run, run_one, run_for, run_until, run_one_for, run_one_until (Task 6 on emscripten)
};
template <class Clock, class WaitTraits = boost::asio::wait_traits<Clock>,
          class Executor = boost::asio::any_io_executor>
using basic_waitable_timer = boost::asio::basic_waitable_timer<Clock, WaitTraits, Executor>;
using steady_timer = boost::asio::steady_timer;
using system_timer = boost::asio::system_timer;
}
```

  The two constructors with a concurrency hint are Boost's (`B/asio/io_context.hpp:243,256`),
  forwarded: Boost's idiom `io_context io{1}` compiles, and Cobalt's `run_main` keeps
  `BOOST_ASIO_CONCURRENCY_HINT_1` (Task 16). The spec's section 4.2 lists the other three;
  these two are an addition, recorded under "Departures from the spec".
- Consumes: `<webcpp/test/exit_status.hpp>` (Task 2).

- [ ] **Step 1: Write the page's section** `doc/names.adoc` (`[#asio-names]`, "The names"):
  spec section 3's table and its two paragraphs, and that a program writes
  `webcpp::asio::` for those types and keeps naming the rest of Boost.Asio directly.
  Include it from `asio.adoc` after the overview.
- [ ] **Step 2: Write the failing test** `test/names_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The names of asio are Boost.Asio's: its io_context derives from Boost's and shares its
// executor, its timers are Boost's own (doc: #asio-names). The options are accepted on
// every target.

#include <webcpp/asio.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <type_traits>

static_assert(std::is_base_of_v<boost::asio::io_context, webcpp::asio::io_context>);
static_assert(std::is_convertible_v<webcpp::asio::io_context&, boost::asio::io_context&>);
static_assert(std::is_same_v<webcpp::asio::io_context::executor_type,
                             boost::asio::io_context::executor_type>);
static_assert(std::is_same_v<webcpp::asio::steady_timer, boost::asio::steady_timer>);
static_assert(std::is_same_v<webcpp::asio::system_timer, boost::asio::system_timer>);
static_assert(std::is_same_v<webcpp::asio::basic_waitable_timer<std::chrono::steady_clock>,
                             boost::asio::basic_waitable_timer<std::chrono::steady_clock>>);

namespace {

void options_are_kept() {
    webcpp::asio::io_context io;
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 1000U);
    io.max_handlers_per_turn(250);
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 250U);
    io.max_handlers_per_turn(0);
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 1U);
    const webcpp::asio::io_context configured(
        webcpp::asio::io_context_options{.max_handlers_per_turn = 64});
    BOOST_TEST_EQ(configured.max_handlers_per_turn(), 64U);
    const webcpp::asio::io_context zero(webcpp::asio::io_context_options{.max_handlers_per_turn = 0});
    BOOST_TEST_EQ(zero.max_handlers_per_turn(), 1U);
}

void a_posted_handler_runs() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
}

// Boost's idiom, a concurrency hint of 1, with the options or without.
void a_concurrency_hint_is_boost_s() {
    webcpp::asio::io_context io{1};
    BOOST_TEST_EQ(io.max_handlers_per_turn(), 1000U);
    const webcpp::asio::io_context hinted(BOOST_ASIO_CONCURRENCY_HINT_1,
                                          webcpp::asio::io_context_options{.max_handlers_per_turn = 8});
    BOOST_TEST_EQ(hinted.max_handlers_per_turn(), 8U);
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
}

}  // namespace

int main() {
    options_are_kept();
    a_posted_handler_runs();
    a_concurrency_hint_is_boost_s();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  and in `test/Jamfile`: `webcpp.run names : names_test.cpp ;`.
- [ ] **Step 3: Run** `$B2 -a --build-dir=bin/lane-native libs/asio/test//names`. Expected:
  FAIL, `no member named 'io_context' in namespace 'webcpp::asio'`.
- [ ] **Step 4: Implement.** `include/webcpp/asio/io_context.hpp`, in this task:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_IO_CONTEXT_HPP
#define WEBCPP_ASIO_IO_CONTEXT_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/io_context.hpp>  // lint-world: the program's event loop, asio's purpose

#include <cstddef>
#include <memory>

namespace webcpp::asio {

/** The options of an io_context. */
struct io_context_options {
    /** At most this many handlers run before the run family yields to the browser; 0 is taken as 1. */
    std::size_t max_handlers_per_turn = 1000;
};

/**
 Boost.Asio's io_context, whose run family on emscripten suspends into the browser's event loop
 and yields to it every max_handlers_per_turn handlers.

 Natively it is Boost's, and its options change nothing.

 @see "The run family", in the guide.
*/
class io_context : public boost::asio::io_context {
public:
    /** Constructs an io_context with the default options. */
    io_context() : io_context(io_context_options{}) {}

    /**
     Constructs an io_context with options.

     @param options Its options.
    */
    explicit io_context(io_context_options options) : options_(normal(options)) {}

    /**
     Constructs an io_context that allocates its services and its objects' state with allocator.

     @tparam Allocator A standard allocator.
     @param allocator What the context allocates with.
     @param options Its options.
    */
    template <class Allocator>
    io_context(std::allocator_arg_t, const Allocator& allocator, io_context_options options = {})
        : boost::asio::io_context(std::allocator_arg, allocator), options_(normal(options)) {}

    /**
     Constructs an io_context with Boost's hint of the concurrency it will see.

     @param concurrency_hint Boost's hint: 1, or BOOST_ASIO_CONCURRENCY_HINT_1, for one thread.
     @param options Its options.
    */
    explicit io_context(int concurrency_hint, io_context_options options = {})
        : boost::asio::io_context(concurrency_hint), options_(normal(options)) {}

    /**
     Constructs an io_context with an allocator and Boost's hint of its concurrency.

     @tparam Allocator A standard allocator.
     @param allocator What the context allocates with.
     @param concurrency_hint Boost's hint.
     @param options Its options.
    */
    template <class Allocator>
    io_context(std::allocator_arg_t, const Allocator& allocator, int concurrency_hint,
               io_context_options options = {})
        : boost::asio::io_context(std::allocator_arg, allocator, concurrency_hint),
          options_(normal(options)) {}

    /**
     The most handlers one turn runs.

     @return The option's value, at least 1.
    */
    [[nodiscard]] std::size_t max_handlers_per_turn() const noexcept {
        return options_.max_handlers_per_turn;
    }

    /**
     Sets the most handlers one turn runs.

     @param n The new value; 0 is taken as 1.
    */
    void max_handlers_per_turn(std::size_t n) noexcept { options_.max_handlers_per_turn = n == 0 ? 1 : n; }

private:
    static io_context_options normal(io_context_options options) noexcept {
        if (options.max_handlers_per_turn == 0) {
            options.max_handlers_per_turn = 1;
        }
        return options;
    }

    io_context_options options_;
};

}  // namespace webcpp::asio

#endif  // WEBCPP_ASIO_IO_CONTEXT_HPP
```

  The timer headers each hold one alias with its Doc Comment
  (`#include <boost/asio/steady_timer.hpp>  // lint-world: Boost's timers are asio's`), and
  `asio.hpp` includes the four new headers.
- [ ] **Step 5: Run** `names` natively and on emscripten
  (`$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test//names`).
  Expected: PASS on both. Then both lanes whole (`alone-asio-io_context` and the timers'
  among them). Expected: PASS.
- [ ] **Step 6: Commit** (library): `"asio: the io_context and the timers, Boost's names"`.

## Task 5: Emscripten: the variants' flags, the reactor and the exit status

**Files:**
- Modify: `build.jam` (the variants' link flags, the tag), `include/webcpp/asio/io_context.hpp`
  (the reactor), `test/Jamfile`.
- Create: `test/reactor_test.cpp`, `test/never_finishes_test.cpp`,
  `test/return_after_suspension_test.cpp`, `doc/building.adoc`.

**Interfaces:**
- Consumes: `webcpp-suspend` and `/webcpp//node-test` (Task 2), `webcpp.suspend-tag`
  (Task 3).
- Produces: the usage requirements of the variants; the own lane `asyncify` and the
  Jamfile's list `browser-tests`, which every later browser test joins.

- [ ] **Step 1: Write the page's section** `doc/building.adoc` (`[#asio-building]`,
  "Building"): the Jamfile line a program writes (`exe app : app.cpp /webcpp/asio//asio ;`),
  `BOOST_ASIO_DISABLE_THREADS` and why, the two variants (spec section 4.4's table), how a
  build asks for one (`webcpp-suspend=asyncify`), the exceptions of each, the artifacts'
  names and directories, and "After a suspension, Asyncify loses the value main returns:
  a program whose exit status matters, under node, ends with `std::exit`; webcpp's own tests
  end through `webcpp::test::exit_status`, and a test that does not fails (AGENTS.md chapter
  9)." Show the
  regions `asio` of `{library-root}/build.jam` and `suspend` of `{webcpp-root}/tools/webcpp.jam`
  in `[listing]` blocks.
- [ ] **Step 2: Write the failing tests.** `test/reactor_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The reactor exists from construction: a run that has work and no timer or socket of the
// program's waits in the reactor's select, which a post made from a browser event wakes.
// Without it Boost's run waits in ::pause(), which nothing wakes (P/reenter.cpp). Work posted
// before run waits for run (doc: #asio-invariant-2).

#include <webcpp/asio/io_context.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/em_macros.h>

#include <optional>

namespace webcpp::test {

/** An io_context held by outstanding work that only a browser event releases. */
struct waiting {
    webcpp::asio::io_context io;
    std::optional<boost::asio::executor_work_guard<webcpp::asio::io_context::executor_type>> guard{
        io.get_executor()};
    int released = 0;
};

}  // namespace webcpp::test

// Called by a JavaScript setTimeout while main is suspended in run: it posts, nothing more.
extern "C" EMSCRIPTEN_KEEPALIVE void test_release(webcpp::test::waiting* state) {
    boost::asio::post(state->io, [state] {
        ++state->released;
        state->guard.reset();
    });
}

namespace {

void a_post_from_an_event_wakes_run() {
    webcpp::test::waiting state;
    EM_ASM({ setTimeout(() => _test_release($0), 20); }, &state);
    BOOST_TEST_EQ(state.io.run(), 1U);  // lint-run: the test's event loop, woken by an event
    BOOST_TEST_EQ(state.released, 1);
}

void work_posted_before_run_waits_for_it() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(ran, 0);
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
}

}  // namespace

int main() {
    a_post_from_an_event_wakes_run();
    work_posted_before_run_waits_for_it();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/never_finishes_test.cpp` (after the notice and a comment saying it pins Review
  Focus 1 in asio's own lanes): `main` runs a `boost::asio::io_context` (no reactor: Boost's
  own) held by a work guard that nothing releases, and would
  `return webcpp::test::exit_status(0);` after it. `test/return_after_suspension_test.cpp`:
  `main` runs a `webcpp::asio::io_context` with a 20 ms `steady_timer`, then
  `return 1;` (no helper). In `test/Jamfile`:

```jam
# Every test of the browser runs in each variant, with exceptions and without
# them: jspi, the default, in the emscripten lane; asyncify in the own lane
# asyncify. A test joins by its name.
local browser-tests = reactor ;
for local test in $(browser-tests)
{
    webcpp.run $(test) : $(test)_test.cpp : : emscripten ;
    webcpp.run $(test)_no_exceptions : $(test)_test.cpp : <exception-handling>off
      : emscripten ;
    webcpp.run $(test)_asyncify : $(test)_test.cpp : <webcpp-suspend>asyncify
      : emscripten ;
    webcpp.run $(test)_asyncify_no_exceptions : $(test)_test.cpp
      : <webcpp-suspend>asyncify <exception-handling>off : emscripten ;
    .asyncify += $(test)_asyncify $(test)_asyncify_no_exceptions ;
}

# Review Focus 1, in asio's lanes: a main that never finishes fails, on both
# variants; a main that suspended and returns 1 fails on both, under Asyncify
# because the test support turns the 0 node would give into 1 (measured,
# asio-plan/support). Each status is read from the .output in Step 6.
webcpp.run-fail never_finishes : never_finishes_test.cpp : : emscripten ;
webcpp.run-fail never_finishes_asyncify : never_finishes_test.cpp
  : <webcpp-suspend>asyncify : emscripten ;
webcpp.run-fail return_after_suspension : return_after_suspension_test.cpp : : emscripten ;
webcpp.run-fail return_after_suspension_asyncify : return_after_suspension_test.cpp
  : <webcpp-suspend>asyncify : emscripten ;
.asyncify += never_finishes_asyncify return_after_suspension_asyncify ;

webcpp.lane asyncify : $(.asyncify) : emscripten ;
```

  (`webcpp.lane` stays the Jamfile's last line in every later task.)
- [ ] **Step 3: Implement the variants' flags first; do not run the tests without them.**
  Without a variant's flags Emscripten's `poll` returns at once ("warning: non-zero poll()
  timeout not supported", measured, `Q/exitcode/novariant.js`), Boost's reactor loops on it
  inside WebAssembly, and no JavaScript timer, the test support's watchdog included, ever
  runs: `reactor` would spin for good. `build.jam`'s target `asio` gains, inside
  its region:

```jam
  <toolset>emscripten,<webcpp-suspend>jspi:<linkflags>-sJSPI
  <toolset>emscripten,<webcpp-suspend>asyncify:<linkflags>-sASYNCIFY
  <toolset>emscripten,<webcpp-suspend>asyncify:<linkflags>-sASYNCIFY_STACK_SIZE=4096
  <toolset>emscripten:<tag>@webcpp%suspend-tag
```

  with a comment: the suspending imports are Emscripten's `__syscall_poll` and asio's yield,
  which Emscripten finds itself; `ASYNCIFY_IGNORE_INDIRECT` is not set, since the path from
  `main` to `select` passes the virtual `scheduler_task::run`; 4096 is Emscripten's default,
  which Task 9 measures; the tag names every program on asio Boost's way.
- [ ] **Step 4: Run** `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test//reactor`.
  Expected: FAIL, exit status 1 and the test support's
  line `webcpp: main did not finish`: `run` never returns (no reactor: Boost's `::pause()`,
  `P/reenter.cpp`), node finds nothing pending and exits, and the support turns its 0 into 1.
- [ ] **Step 5: Implement the reactor.** `io_context.hpp` gains, on emscripten:

```cpp
#ifdef __EMSCRIPTEN__
#include <boost/asio/steady_timer.hpp>  // lint-world: the unarmed timer that makes the reactor
#endif
```

  and, after `options_`:

```cpp
#ifdef __EMSCRIPTEN__
    // Never armed: its construction makes the reactor, which the scheduler then waits on, so a
    // run with work and no timer of the program's own suspends in select, which a post wakes,
    // rather than in ::pause(), which nothing wakes (P/reenter.cpp, P/reenter_reactor.cpp).
    boost::asio::steady_timer reactor_{*this};  // lint-world: never armed
#endif
```

- [ ] **Step 6: Run** the emscripten lane and the asyncify lane whole. Expected: PASS: on
  each, `reactor`, `reactor_no_exceptions`, `never_finishes` and `return_after_suspension`
  (the last two as `run-fail`). Read each `run-fail`'s status in its `.output`
  (`grep "EXIT STATUS" $(find bin/lane-* -name '<name>.output')`): `never_finishes` 1 with
  `webcpp: main did not finish`, `return_after_suspension` 1, and under Asyncify
  `return_after_suspension_asyncify` 1 with `webcpp: main did not finish`. Then the native
  lane. Expected: PASS (nothing of this task builds natively but the headers).
- [ ] **Step 7: Commit** (library): `"asio: the variants' flags, the reactor made at
  construction, and the exit status of a browser test"`.

## Task 6: The run family and the turn limit

**Carried from Task 4 (controller, 2026-10-11):** `include/webcpp/asio/io_context.hpp:35` has
`@see "The names"` as a stand-in, because "The run family" did not exist until this task;
once `doc/run.adoc` adds that section, point the `@see` back to `"The run family"` and check it
in this task's doc build (doc-check stays green either way, so this step is the only guard).
Also carried from Task 5: a comment that should read `(doc: #asio-invariant-2)` reads
`(doc: #asio-names)` until that anchor exists; point it back here too.

**Files:**
- Create: `include/webcpp/asio/detail/browser_imports.hpp`, `include/webcpp/asio/browser/src.hpp`,
  `src/browser_src.cpp`, `test/page_timeout.hpp`, `test/run_family_test.cpp`,
  `test/turn_limit_test.cpp`, `doc/run.adoc`.
- Modify: `build.jam` (the object `browser-src`), `include/webcpp/asio/io_context.hpp`,
  `test/Jamfile` (the browser headers alone on emscripten; the two tests).

**Interfaces:**
- Produces, on emscripten, in `webcpp::asio::io_context` (hiding Boost's):

```cpp
count_type run();
count_type run_one();
template <class Rep, class Period>
std::size_t run_for(const std::chrono::duration<Rep, Period>& rel_time);
template <class Clock, class Duration>
std::size_t run_until(const std::chrono::time_point<Clock, Duration>& abs_time);
template <class Rep, class Period>
std::size_t run_one_for(const std::chrono::duration<Rep, Period>& rel_time);
template <class Clock, class Duration>
std::size_t run_one_until(const std::chrono::time_point<Clock, Duration>& abs_time);
```

  and natively `using boost::asio::io_context::run;` (and the five others);
  `extern "C" void webcpp_asio_yield()` (`EM_IMPORT(__asyncjs__webcpp_asio_yield)`), whose
  JavaScript `browser/src.hpp` defines; the explicit object `/webcpp/asio//browser-src`.

- [ ] **Step 1: Write the page's section** `doc/run.adoc` (`[#asio-run]`, "The run
  family"): spec sections 4.2 and 4.3 (the four steps, the counts, the macrotask yield, the
  default of 1,000 and the option), 4.5 (who may call it; the lint's `lint-run:`; xactor's
  `asio_driver`, whose own io_context has no reactor and whose `poll` is safe from a
  handler), and 4.8 (without `run` nothing runs; after `main`). Include it after `building.adoc`.
- [ ] **Step 2: Write the failing tests.** `test/run_family_test.cpp` (the notice, a comment
  naming the cases and `(doc: #asio-invariant-3)`; includes `<webcpp/asio/io_context.hpp>`,
  `<webcpp/asio/steady_timer.hpp>`, `"page_timeout.hpp"`, `<webcpp/test/exit_status.hpp>`,
  `<boost/asio/post.hpp>`, `<boost/core/lightweight_test.hpp>`, `<chrono>`). First
  `test/page_timeout.hpp`, which every test that watches the page's event loop shares; its
  JavaScript never calls into C++, so a timeout that runs after its case ended touches
  nothing:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_ASIO_PAGE_TIMEOUT_HPP
#define WEBCPP_TEST_ASIO_PAGE_TIMEOUT_HPP

#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#endif

namespace webcpp::test {

/**
 A JavaScript setTimeout(0), armed at construction, whose running only JavaScript records.

 Tip: natively there is no page, and it never runs.
*/
class page_timeout {
public:
    page_timeout()
#ifdef __EMSCRIPTEN__
        : id_(EM_ASM_INT({
              const state = {ran: false};
              const id = (globalThis.webcppTimeouts ??= []).push(state) - 1;
              setTimeout(() => {
                  state.ran = true;
              }, 0);
              return id;
          }))
#endif
    {
    }

    /** Whether the timeout has run. */
    [[nodiscard]] bool ran() const {
#ifdef __EMSCRIPTEN__
        return EM_ASM_INT({ return globalThis.webcppTimeouts[$0].ran ? 1 : 0; }, id_) != 0;
#else
        return false;
#endif
    }

private:
    int id_ = -1;
};

/**
 Counts the run family's yields: every MessagePort message the page posts, which only the
 yield posts in these tests.

 Tip: deterministic where a setTimeout(0) is not: node delivers a timeout and a
 MessageChannel message in either order (asio-plan-review, R/two_tu-jspi.js).
*/
class yield_counter {
public:
    yield_counter() {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (!globalThis.webcppYieldsCounted) {
                globalThis.webcppYieldsCounted = true;
                globalThis.webcppYields = 0;
                const post = MessagePort.prototype.postMessage;
                MessagePort.prototype.postMessage = function(...values) {
                    ++globalThis.webcppYields;
                    return post.apply(this, values);
                };
            }
        });
#endif
    }

    /** The yields so far. */
    [[nodiscard]] int count() const {
#ifdef __EMSCRIPTEN__
        return EM_ASM_INT({ return globalThis.webcppYields; });
#else
        return 0;
#endif
    }
};

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_ASIO_PAGE_TIMEOUT_HPP
```

  Then the test:

```cpp
namespace {

using namespace std::chrono_literals;

/** Counts its runs, stops the context at stop_at, and posts itself again even then. */
struct stop_then_post {
    webcpp::asio::io_context* io;
    std::size_t* count;
    std::size_t stop_at;
    void operator()() const {
        if (++*count == stop_at) {
            io->stop();
        }
        boost::asio::post(*io, *this);
    }
};

/** Posts itself until stopped, counting its runs. */
struct spin {
    webcpp::asio::io_context* io;
    std::size_t* count;
    std::size_t stop_at = 0;  // 0: never stops the context
    void operator()() const {
        ++*count;
        if (stop_at != 0 && *count == stop_at) {
            io->stop();
            return;
        }
        boost::asio::post(*io, *this);
    }
};

void run_returns_out_of_work_with_the_handlers_run() {
    webcpp::asio::io_context io;
    int ran = 0;
    for (int i = 0; i < 3; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    BOOST_TEST_EQ(io.run(), 3U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 3);
    BOOST_TEST(io.stopped());
}

void run_one_runs_one() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run_one(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
}

void run_waits_for_a_timer() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 30ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code ec) { fired = !ec; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(fired);
}

void the_timed_forms_return_at_their_deadline() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer later(io, 10s);
    bool cancelled = false;
    later.async_wait([&cancelled](boost::system::error_code ec) {
        cancelled = ec == boost::asio::error::operation_aborted;
    });
    BOOST_TEST_EQ(io.run_for(50ms), 0U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() + 50ms), 0U);
    BOOST_TEST_EQ(io.run_one_for(50ms), 0U);
    BOOST_TEST_EQ(io.run_one_until(std::chrono::steady_clock::now() + 50ms), 0U);
    BOOST_TEST(!io.stopped());
    later.cancel();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(cancelled);
}

void stop_and_restart() {
    webcpp::asio::io_context io;
    int ran = 0;
    boost::asio::post(io, [&] {
        ++ran;
        io.stop();
    });
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST(io.stopped());
    io.restart();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 2);
}

// Review Focus 2: every turn reaches the limit and yields, and the deadline still ends it.
void a_deadline_ends_a_run_whose_handlers_are_always_ready() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 100});
    std::size_t count = 0;
    boost::asio::post(io, spin{&io, &count});
    const auto start = std::chrono::steady_clock::now();
    const std::size_t ran = io.run_for(100ms);  // lint-run: the test's event loop
    const auto took = std::chrono::steady_clock::now() - start;
    BOOST_TEST(ran > 0);
    BOOST_TEST_EQ(ran, count);
    BOOST_TEST(took >= 100ms && took < 2s);
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() + 50ms) > 0, true);
}

// Review Focus 2: a stop by the handler that reaches the limit returns at once, without a
// yield (a MessageChannel message, which the yield counter sees). The handler posts itself
// again after its stop, so the context still has work: only the stopped() check ends the run.
void a_stop_at_the_limit_returns_at_once() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 10});
    std::size_t count = 0;
    const webcpp::test::yield_counter yields;
    const int before = yields.count();
    boost::asio::post(io, stop_then_post{&io, &count, 10});
    BOOST_TEST_EQ(io.run(), 10U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(count, 10U);
    BOOST_TEST_EQ(yields.count(), before);
}

// Review Focus 2: a limit of 0 is taken as 1, which still runs every handler.
void a_limit_of_zero_still_runs() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 0});
    std::size_t count = 0;
    boost::asio::post(io, spin{&io, &count, 50});
    BOOST_TEST_EQ(io.run(), 50U);  // lint-run: the test's event loop
}

}  // namespace

int main() {
    run_returns_out_of_work_with_the_handlers_run();
    run_one_runs_one();
    run_waits_for_a_timer();
    the_timed_forms_return_at_their_deadline();
    stop_and_restart();
    a_deadline_ends_a_run_whose_handlers_are_always_ready();
    a_stop_at_the_limit_returns_at_once();
    a_limit_of_zero_still_runs();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `test/turn_limit_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A self-posting handler lets a JavaScript setTimeout(0) run within max_handlers_per_turn
// handlers, under run and under run_for (doc: #asio-invariant-3). Boost's own run, reached
// through a boost::asio::io_context&, has no limit: the setTimeout runs only once it returns
// (P/starve.cpp measured 300,000 handlers first).

#include <webcpp/asio/io_context.hpp>

#include "page_timeout.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <cstddef>

namespace {

/** A self-posting spin and the page's setTimeout(0) it watches for. */
struct turn {
    webcpp::asio::io_context io;
    webcpp::test::page_timeout timeout;  // armed here, before any handler runs
    std::size_t handlers = 0;
    std::size_t seen_at = 0;  // how many handlers had run when the timeout was first seen run
    std::size_t until = 0;
};

struct spin {
    turn* state;
    void operator()() const {
        ++state->handlers;
        if (state->seen_at == 0 && state->timeout.ran()) {
            state->seen_at = state->handlers - 1;
        }
        if (state->handlers < state->until) {
            boost::asio::post(state->io, *this);
        }
    }
};

void run_yields_within_the_limit() {
    turn state;
    state.until = 5000;
    boost::asio::post(state.io, spin{&state});
    BOOST_TEST_EQ(state.io.run(), 5000U);  // lint-run: the test's event loop
    BOOST_TEST(state.seen_at >= 1 && state.seen_at <= state.io.max_handlers_per_turn());
}

void run_for_yields_within_the_limit() {
    turn state;
    state.io.max_handlers_per_turn(250);
    state.until = 5000;
    boost::asio::post(state.io, spin{&state});
    state.io.run_for(std::chrono::seconds(10));  // lint-run: the test's event loop
    BOOST_TEST_EQ(state.handlers, 5000U);
    BOOST_TEST(state.seen_at >= 1 && state.seen_at <= 250);
}

void boost_s_own_run_has_no_limit() {
    turn state;
    state.until = 5000;
    boost::asio::post(state.io, spin{&state});
    boost::asio::io_context& boost_s = state.io;
    BOOST_TEST_EQ(boost_s.run(), 5000U);  // lint-run: Boost's own run, which has no limit
    BOOST_TEST_EQ(state.seen_at, 0U);
}

}  // namespace

int main() {
    run_yields_within_the_limit();
    run_for_yields_within_the_limit();
    boost_s_own_run_has_no_limit();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `run_family` and `turn_limit` join `browser-tests`; `run_family` also runs natively:
  `webcpp.run run_family_native : run_family_test.cpp : : native ;`.
- [ ] **Step 3: Run** `turn_limit` and `run_family` on the emscripten lane. Expected:
  `turn_limit` FAILS: `seen_at` is 0 in `run_yields_within_the_limit` (the class still
  inherits Boost's run, which never yields). `run_family` PASSES on every lane before the
  implementation, and that is expected: Boost's own run family holds every case, its
  `run_for` included (Boost's `run_one_until` checks the clock), and
  `a_stop_at_the_limit_returns_at_once` holds too, since Boost's `run` never yields; the
  file pins that the composition keeps Boost's semantics. See the stop case able to fail:
  in a scratch copy of the composition (after Step 4), move the `stopped()` check below the
  yield, run `run_family`, see `a_stop_at_the_limit_returns_at_once` FAIL on the yield count,
  and restore.
- [ ] **Step 4: Implement.** `detail/browser_imports.hpp` (emscripten only; the notice; a
  brief):

```cpp
#ifndef WEBCPP_ASIO_DETAIL_BROWSER_IMPORTS_HPP
#define WEBCPP_ASIO_DETAIL_BROWSER_IMPORTS_HPP

#include <webcpp/asio/config.hpp>

#include <emscripten/em_macros.h>
#include <emscripten/val.h>

#include <cstddef>
#include <cstdint>

// The JavaScript functions <webcpp/asio/browser/src.hpp> defines, which every program on
// emscripten links once (src/browser_src.cpp).
extern "C" {
/** Suspends until a MessageChannel message: a macrotask, never a microtask. */
void webcpp_asio_yield() EM_IMPORT(__asyncjs__webcpp_asio_yield);
/** The JavaScript function that delivers an event to the bridge's registration key. */
emscripten::EM_VAL webcpp_asio_function(emscripten::EM_VAL token, void* bridge, std::uint64_t key)
    EM_IMPORT(webcpp_asio_function);
/** Settles fulfil with {ok, value} once promise settles. */
void webcpp_asio_await(emscripten::EM_VAL promise, emscripten::EM_VAL fulfil)
    EM_IMPORT(webcpp_asio_await);
/** A JavaScript string of size bytes of UTF-8 at data, which need not end with a null. */
emscripten::EM_VAL webcpp_asio_string(const char* data, std::size_t size)
    EM_IMPORT(webcpp_asio_string);
/** Reports error to the page (reportError). */
void webcpp_asio_report(emscripten::EM_VAL error) EM_IMPORT(webcpp_asio_report);
}

#endif  // WEBCPP_ASIO_DETAIL_BROWSER_IMPORTS_HPP
```

  `browser/src.hpp`, in this task (Task 7 adds the bridge's export; the notice and a
  file-level comment: what a program compiles once, linked by `/webcpp/asio//asio` through
  `src/browser_src.cpp`, or included in one source file of a program built otherwise):

```cpp
#ifndef WEBCPP_ASIO_BROWSER_SRC_HPP
#define WEBCPP_ASIO_BROWSER_SRC_HPP

#include <webcpp/asio/config.hpp>

#include <webcpp/asio/detail/browser_imports.hpp>

#include <emscripten/em_js.h>

EM_JS_DEPS(webcpp_asio, "$Emval,$stackSave,$stackRestore,$UTF8ToString");

// clang-format off
EM_ASYNC_JS(void, webcpp_asio_yield, (), {
    await new Promise(resolve => {
        const channel = new MessageChannel();
        channel.port1.onmessage = () => {
            channel.port1.close();
            resolve();
        };
        channel.port2.postMessage(0);
    });
});

EM_JS(void, webcpp_asio_await, (emscripten::EM_VAL promise, emscripten::EM_VAL fulfil), {
    const deliver = Emval.toValue(fulfil);
    Promise.resolve(Emval.toValue(promise)).then(
        value => deliver({ok: true, value}),
        value => deliver({ok: false, value}));
});

EM_JS(emscripten::EM_VAL, webcpp_asio_string, (const char* data, std::size_t size), {
    return Emval.toHandle(UTF8ToString(data, size));
});

EM_JS(void, webcpp_asio_report, (emscripten::EM_VAL error), {
    (globalThis.reportError ?? console.error)(Emval.toValue(error));
});
// clang-format on

#endif  // WEBCPP_ASIO_BROWSER_SRC_HPP
```

  `src/browser_src.cpp` is the notice, a one-line comment and
  `#include <webcpp/asio/browser/src.hpp>`. `build.jam` gains, before `asio`:

```jam
# What a program on emscripten compiles once: the JavaScript of the yield, of
# the bridge and of the promises, and the export the bridge delivers through
# (include/webcpp/asio/browser/src.hpp), linked into every program on
# emscripten by asio's usage requirements below.
obj browser-src : src/browser_src.cpp : <include>include <library>/boost//headers
  <toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS ;
explicit browser-src ;
```

  and `asio` gains `<toolset>emscripten:<source>browser-src`. In `io_context.hpp`, on
  emscripten, the run family (the existing members stay; `<chrono>` and
  `<webcpp/asio/detail/browser_imports.hpp>` included):

```cpp
#ifdef __EMSCRIPTEN__
    /**
     Runs handlers until the io_context is out of work or stopped, yielding to the browser
     every max_handlers_per_turn handlers.

     @return The handlers run.
    */
    count_type run() {
        return compose([this] {
            return boost::asio::io_context::run_one();  // lint-run: step 4, the one wait
        }, [] { return false; });
    }

    /**
     Runs one handler, waiting in the browser's event loop for one to be ready.

     @return The handlers run: 1, or 0 when stopped or out of work.
    */
    count_type run_one() {
        return boost::asio::io_context::run_one();  // lint-run: step 4 alone
    }

    /**
     Runs handlers as run does, until abs_time at most.

     @tparam Clock The deadline's clock.
     @tparam Duration The deadline's duration.
     @param abs_time The deadline.
     @return The handlers run.
    */
    template <class Clock, class Duration>
    std::size_t run_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        return run_until_unchecked(abs_time);
    }

    /**
     Runs handlers as run does, for rel_time at most.

     @tparam Rep The duration's representation.
     @tparam Period The duration's period.
     @param rel_time How long.
     @return The handlers run.
    */
    template <class Rep, class Period>
    std::size_t run_for(const std::chrono::duration<Rep, Period>& rel_time) {
        // The private composition, never the public run_until, which Task 8 makes admit: one
        // public call admits once (Review Focus 6).
        return run_until_unchecked(std::chrono::steady_clock::now() + rel_time);  // lint-world: the deadline
    }

    /**
     Runs one handler, waiting until abs_time at most.

     @tparam Clock The deadline's clock.
     @tparam Duration The deadline's duration.
     @param abs_time The deadline.
     @return The handlers run: 1 or 0.
    */
    template <class Clock, class Duration>
    std::size_t run_one_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        return boost::asio::io_context::run_one_until(abs_time);
    }

    /**
     Runs one handler, waiting for rel_time at most.

     @tparam Rep The duration's representation.
     @tparam Period The duration's period.
     @param rel_time How long.
     @return The handlers run: 1 or 0.
    */
    template <class Rep, class Period>
    std::size_t run_one_for(const std::chrono::duration<Rep, Period>& rel_time) {
        return boost::asio::io_context::run_one_for(rel_time);
    }

private:
    /** What run_until and run_for run: the composition until abs_time, admitting nothing. */
    template <class Clock, class Duration>
    std::size_t run_until_unchecked(const std::chrono::time_point<Clock, Duration>& abs_time) {
        return compose([this, &abs_time] { return boost::asio::io_context::run_one_until(abs_time); },
                       [&abs_time] { return Clock::now() >= abs_time; });
    }

    /** Runs the ready handlers, max_handlers_per_turn at most, without waiting. */
    std::size_t drain() {
        std::size_t ran = 0;
        while (ran < options_.max_handlers_per_turn && poll_one() == 1) {
            ++ran;
        }
        return ran;
    }

    /**
     The run family's four steps: a turn of ready handlers; return when stopped or expired;
     yield through a macrotask when the turn reached the limit; else wait for one handler.
    */
    template <class Wait, class Expired>
    count_type compose(Wait wait, Expired expired) {
        count_type total = 0;
        for (;;) {
            const std::size_t ran = drain();
            total += ran;
            if (stopped() || expired()) {
                return total;
            }
            if (ran == options_.max_handlers_per_turn) {
                webcpp_asio_yield();
                continue;
            }
            const std::size_t one = wait();
            if (one == 0) {
                return total;
            }
            total += one;
        }
    }

public:
#else
    using boost::asio::io_context::run;  // lint-run: Boost's own, natively
    using boost::asio::io_context::run_for;  // lint-run: Boost's own, natively
    using boost::asio::io_context::run_one;  // lint-run: Boost's own, natively
    using boost::asio::io_context::run_one_for;
    using boost::asio::io_context::run_one_until;
    using boost::asio::io_context::run_until;
#endif
```

  (each `using` with a Doc Comment brief: "Boost's own, natively".) In `test/Jamfile` the
  first headers-alone call becomes
  `webcpp.headers-alone asio : ../include : asio.hpp asio/*.hpp ;` and a second takes
  `asio/browser/*.hpp asio/detail/*.hpp` on emscripten alone:
  `webcpp.headers-alone asio : ../include : asio/browser/*.hpp asio/detail/*.hpp : : emscripten ;`.
- [ ] **Step 5: Run** the native, emscripten and asyncify lanes. Expected: PASS, every test of
  both new files on both variants. Then the lint. Expected: `lint: clean`.
- [ ] **Step 6: Commit** (library): `"asio: the run family, a turn of ready handlers at a time,
  yielding through a macrotask"`.

## Task 7: The bridge

**Files:**
- Create: `include/webcpp/asio/browser/bridge.hpp`, `test/browser_values.hpp`,
  `test/bridge_test.cpp`, `test/em_js_test.cpp`, `doc/browser.adoc`.
- Modify: `include/webcpp/asio/browser/src.hpp` (the bridge's function and export),
  `include/webcpp/asio.hpp`, `test/Jamfile`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::asio::browser {
/** What an event delivered through the bridge does: posts, nothing else. */
class registration {
public:
    /** Posts what event completes or notifies; returns whether the registration is done. */
    virtual bool deliver(const emscripten::val& event) = 0;
    /** The bridge's shutdown: destroys what is pending without calling it. */
    virtual void shutdown() noexcept = 0;
protected:
    registration() = default;
    ~registration() = default;
    registration(const registration&) = default;
    registration& operator=(const registration&) = default;
};
class bridge : public boost::asio::execution_context::service {
public:
    static inline boost::asio::execution_context::id id;
    explicit bridge(boost::asio::execution_context& context);
    ~bridge() override;
    [[nodiscard]] std::uint64_t add(registration& r);           // keys from 1, never reused
    [[nodiscard]] emscripten::val function(std::uint64_t key);
    void remove(std::uint64_t key) noexcept;
    [[nodiscard]] bool delivering() const noexcept;
    void deliver(std::uint64_t key, const emscripten::val& event); // the export's
private:
    void shutdown() override;
};
}
extern "C" void webcpp_asio_deliver(void* bridge, std::uint64_t key, emscripten::EM_VAL event);
```

  The key is a `std::uint64_t` (a JavaScript `BigInt`: `WASM_BIGINT` is on by default,
  `E/src/settings.js:1480`), so it is never reused. The registration is owned by whoever adds it (an operation, Tasks 17 and 18; a stream of
  webcpp::beast; a channel of webcpp::webrtc), allocated as its owner chooses; the bridge
  holds a pointer in a `boost::unordered_flat_map` allocated with the context's allocator
  (`boost::asio::execution_context::allocator`).

- [ ] **Step 1: Write the page's section** `doc/browser.adoc` (`[#asio-browser]`, "Browser
  events"): spec sections 4.6 and 4.5's second and third items: the bridge, registration by
  id never reused, posting only, the boundary that reports and restores the stack, what an
  event may call, and why `dispatch` from an event would run inline (`P/reenter.cpp`).
- [ ] **Step 2: Write `test/browser_values.hpp`** (included by one source file per program,
  since it defines `EM_JS` functions; the notice and that sentence):

```cpp
#ifndef WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP
#define WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP

#include <emscripten/em_js.h>
#include <emscripten/val.h>

// clang-format off
EM_JS(emscripten::EM_VAL, test_event_target, (), {
    return Emval.toHandle(new EventTarget());
});

EM_JS(void, test_dispatch_after, (emscripten::EM_VAL target, int milliseconds, int detail), {
    const object = Emval.toValue(target);
    setTimeout(() => object.dispatchEvent(new CustomEvent('message', {detail})), milliseconds);
});

// A parameter's name is the JavaScript function's too: never a reserved word such as
// `function`, which links and then stops the program at load with a SyntaxError
// (asio-plan-review, R/em_js_reserved.cpp). em_js_test proves every EM_JS here loads.
EM_JS(void, test_call_after, (emscripten::EM_VAL callback, int milliseconds, int value), {
    const call = Emval.toValue(callback);
    setTimeout(() => call(value), milliseconds);
});

EM_JS(emscripten::EM_VAL, test_throwing_function, (), {
    return Emval.toHandle(() => {
        throw new Error('from JavaScript');
    });
});

EM_JS(emscripten::EM_VAL, test_promise_after, (int milliseconds, int value, bool reject), {
    return Emval.toHandle(new Promise((resolve, fail) =>
        setTimeout(() => (reject ? fail(value) : resolve(value)), milliseconds)));
});

EM_JS(int, test_reported, (), {
    return globalThis.webcppTestReported ?? 0;
});

EM_JS(void, test_count_reports, (), {
    globalThis.webcppTestReported = 0;
    globalThis.reportError = () => { ++globalThis.webcppTestReported; };
});
// clang-format on

#endif  // WEBCPP_TEST_ASIO_BROWSER_VALUES_HPP
```

- [ ] **Step 3: Write the failing test** `test/bridge_test.cpp` (the notice, a comment naming
  the cases, `(doc: #asio-invariant-4 and #asio-invariant-5)`; includes
  `<webcpp/asio/browser/bridge.hpp>`, `<webcpp/asio/io_context.hpp>`,
  `<webcpp/asio/steady_timer.hpp>`, `"browser_values.hpp"`, `<webcpp/test/exit_status.hpp>`,
  `<boost/asio/executor_work_guard.hpp>`, `<boost/asio/post.hpp>`,
  `<boost/core/lightweight_test.hpp>`, `<emscripten/stack.h>`, `<emscripten/val.h>`,
  `<chrono>`, `<memory>`, `<stdexcept>`):

```cpp
namespace {

/** A registration that posts a count of the events it receives, until it is done. */
class counter final : public webcpp::asio::browser::registration {
public:
    counter(webcpp::asio::io_context& io, int once_after) : io_(io), once_after_(once_after) {}
    bool deliver(const emscripten::val& event) override {
        const int value = event.as<int>();
        boost::asio::post(io_, [this, value] {
            ++received;
            last = value;
        });
        return once_after_ != 0 && ++delivered_ == once_after_;
    }
    void shutdown() noexcept override { shut_down = true; }
    int received = 0;
    int last = 0;
    bool shut_down = false;

private:
    webcpp::asio::io_context& io_;
    int once_after_;
    int delivered_ = 0;
};

webcpp::asio::browser::bridge& bridge_of(webcpp::asio::io_context& io) {
    return boost::asio::use_service<webcpp::asio::browser::bridge>(io);
}

void an_event_posts_and_wakes_run() {
    webcpp::asio::io_context io;
    counter listener(io, 1);
    auto& bridge = bridge_of(io);
    const std::uint64_t key = bridge.add(listener);
    BOOST_TEST(key >= 1);
    auto guard = boost::asio::make_work_guard(io);
    test_call_after(bridge.function(key).as_handle(), 20, 7);
    webcpp::asio::steady_timer release(io, std::chrono::milliseconds(100));
    release.async_wait([&guard](boost::system::error_code) { guard.reset(); });
    io.run();  // lint-run: the test's event loop
    BOOST_TEST_EQ(listener.received, 1);
    BOOST_TEST_EQ(listener.last, 7);
}

void events_before_run_wait_for_it() {
    webcpp::asio::io_context io;
    counter listener(io, 0);
    auto& bridge = bridge_of(io);
    const std::uint64_t key = bridge.add(listener);
    const emscripten::val deliver = bridge.function(key);
    deliver(1);
    deliver(2);
    BOOST_TEST_EQ(listener.received, 0);
    BOOST_TEST_EQ(io.run(), 2U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(listener.received, 2);
    bridge.remove(key);
}

// A listener is no outstanding work: run returns with one registered and nothing pending.
void run_returns_with_a_listener_and_nothing_pending() {
    webcpp::asio::io_context io;
    counter listener(io, 0);
    auto& bridge = bridge_of(io);
    const std::uint64_t key = bridge.add(listener);
    BOOST_TEST_EQ(io.run(), 0U);  // lint-run: the test's event loop
    bridge.function(key)(3);
    io.restart();
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(listener.last, 3);
    bridge.remove(key);
}

// Review Focus 4: an event of a removed key, and one of a destroyed context, find nothing;
// destroying the context shuts its registrations down. The registrations outlive the
// context, as their owners must.
void late_events_find_nothing() {
    auto io = std::make_unique<webcpp::asio::io_context>();
    counter removed(*io, 0);
    counter kept(*io, 0);
    auto& bridge = bridge_of(*io);
    const std::uint64_t removed_key = bridge.add(removed);
    const emscripten::val stale = bridge.function(removed_key);
    bridge.remove(removed_key);
    stale(1);
    BOOST_TEST_EQ(io->run(), 0U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(removed.received, 0);
    const std::uint64_t kept_key = bridge.add(kept);
    BOOST_TEST(kept_key > removed_key);
    const emscripten::val orphan = bridge.function(kept_key);
    io.reset();
    BOOST_TEST(kept.shut_down);
    orphan(2);  // its bridge is gone: the function does nothing
    stale(3);
    BOOST_TEST_EQ(kept.received, 0);
    webcpp::asio::io_context other;
    BOOST_TEST_EQ(other.run(), 0U);  // lint-run: the test's event loop
}

#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
/** A registration whose delivery throws, as an allocator that fails would. */
class thrower final : public webcpp::asio::browser::registration {
public:
    bool deliver(const emscripten::val&) override { throw std::runtime_error("delivery failed"); }
    void shutdown() noexcept override {}
};

// 3000 escapes in a row: each is reported, and WebAssembly's stack pointer is where it was.
void an_escaping_exception_is_reported_and_the_stack_restored() {
    test_count_reports();
    webcpp::asio::io_context io;
    thrower failing;
    auto& bridge = bridge_of(io);
    const std::uint64_t key = bridge.add(failing);
    const emscripten::val deliver = bridge.function(key);
    const auto before = emscripten_stack_get_current();
    for (int i = 0; i < 3000; ++i) {
        deliver(i);
    }
    BOOST_TEST_EQ(emscripten_stack_get_current(), before);
    BOOST_TEST_EQ(test_reported(), 3000);
    bridge.remove(key);
}
#endif

/** A registration whose delivery calls a JavaScript function that throws. */
class javascript_thrower final : public webcpp::asio::browser::registration {
public:
    bool deliver(const emscripten::val&) override {
        throwing_();
        return false;
    }
    void shutdown() noexcept override {}

private:
    emscripten::val throwing_ = emscripten::val::take_ownership(test_throwing_function());
};

// The JavaScript boundary of the bridge's function: a JavaScript error leaves WebAssembly
// through it. Without exceptions nothing in C++ catches it, so the boundary's catch, its
// stackRestore and its reportError are what this case proves (the _no_exceptions twins); with
// them, the export's catch (...) may take it first, and the same holds. 3000 in a row.
void a_javascript_error_is_reported_and_the_stack_restored() {
    test_count_reports();
    webcpp::asio::io_context io;
    javascript_thrower failing;
    auto& bridge = bridge_of(io);
    const std::uint64_t key = bridge.add(failing);
    const emscripten::val deliver = bridge.function(key);
    const auto before = emscripten_stack_get_current();
    for (int i = 0; i < 3000; ++i) {
        deliver(i);
    }
    BOOST_TEST_EQ(emscripten_stack_get_current(), before);
    BOOST_TEST_EQ(test_reported(), 3000);
    bridge.remove(key);
}

}  // namespace

int main() {
    an_event_posts_and_wakes_run();
    events_before_run_wait_for_it();
    run_returns_with_a_listener_and_nothing_pending();
    late_events_find_nothing();
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    an_escaping_exception_is_reported_and_the_stack_restored();
#endif
    a_javascript_error_is_reported_and_the_stack_restored();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  `bridge` joins `browser-tests`. So does `em_js`, `test/em_js_test.cpp`, which proves that
  every `EM_JS` of the library (`browser/src.hpp`, linked through `browser-src`) and of the
  tests (`browser_values.hpp`) loads: a `SyntaxError` at load fails it before `main`.

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Every EM_JS of asio and of its tests loads: EM_JS copies its C parameters' names into
// JavaScript, and a name JavaScript reserves links and then stops the program at load
// (asio-plan-review, R/em_js_reserved.cpp). This program only loads them, under node here
// and in Chrome through page_program (Task 12), which includes the same headers.

#include <webcpp/asio.hpp>

#include "browser_values.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/core/lightweight_test.hpp>

int main() {
    return webcpp::test::exit_status(boost::report_errors());
}
```
- [ ] **Step 4: Run** `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test//bridge`.
  Expected: FAIL, `<webcpp/asio/browser/bridge.hpp>` not found.
- [ ] **Step 5: Implement** `browser/bridge.hpp` (the notice; includes `config.hpp`,
  `detail/browser_imports.hpp`, `<boost/asio/execution_context.hpp>  // lint-world: a
  service of the io_context`, `<boost/unordered/unordered_flat_map.hpp>`, `<emscripten/val.h>`,
  `<cstdint>`, `<functional>`, `<utility>`), with every member documented:

```cpp
namespace webcpp::asio::browser {

class bridge : public boost::asio::execution_context::service {
public:
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    static inline boost::asio::execution_context::id id;  // Boost.Asio's key, by address

    explicit bridge(boost::asio::execution_context& context)
        : boost::asio::execution_context::service(context),
          registrations_(map_allocator(context)),
          token_(emscripten::val::object()) {
        token_.set("alive", true);
    }

    bridge(const bridge&) = delete;
    bridge& operator=(const bridge&) = delete;
    ~bridge() override { token_.set("alive", false); }

    [[nodiscard]] std::uint64_t add(registration& r) {
        const std::uint64_t key = next_++;
        registrations_.emplace(key, &r);
        return key;
    }

    [[nodiscard]] emscripten::val function(std::uint64_t key) {
        return emscripten::val::take_ownership(webcpp_asio_function(token_.as_handle(), this, key));
    }

    void remove(std::uint64_t key) noexcept { registrations_.erase(key); }

    [[nodiscard]] bool delivering() const noexcept { return delivering_; }

    void deliver(std::uint64_t key, const emscripten::val& event) {
        const auto found = registrations_.find(key);
        if (found == registrations_.end()) {
            return;
        }
        const scope delivering(delivering_);
        // A registration that is done may have freed itself: it is erased by its key.
        if (found->second->deliver(event)) {
            registrations_.erase(key);
        }
    }

private:
    using map = boost::unordered_flat_map<
        std::uint64_t, registration*, boost::hash<std::uint64_t>, std::equal_to<>,
        boost::asio::execution_context::allocator<std::pair<const std::uint64_t, registration*>>>;

    static map::allocator_type map_allocator(boost::asio::execution_context& context) {
        return map::allocator_type(context);
    }

    /** Sets a flag for its lifetime, and clears it however the scope ends. */
    struct scope {
        explicit scope(bool& flag) noexcept : flag_(flag) { flag_ = true; }
        scope(const scope&) = delete;
        scope& operator=(const scope&) = delete;
        ~scope() { flag_ = false; }
        bool& flag_;
    };

    void shutdown() override {
        token_.set("alive", false);
        auto pending = std::move(registrations_);
        for (auto& [key, r] : pending) {
            r->shutdown();
        }
    }

    map registrations_;
    emscripten::val token_;
    std::uint64_t next_ = 1;
    bool delivering_ = false;
};

}  // namespace webcpp::asio::browser
```

  `browser/src.hpp` gains
  `#include <webcpp/asio/browser/bridge.hpp>`, `<emscripten/emscripten.h>`, `<exception>`,
  `<utility>`, and:

```cpp
EM_JS(emscripten::EM_VAL, webcpp_asio_function, (emscripten::EM_VAL token, void* bridge, std::uint64_t key), {
    const state = Emval.toValue(token);
    const address = bridge >>> 0;
    return Emval.toHandle(event => {
        if (!state.alive) {
            return;
        }
        // An exception leaves WebAssembly without its frames' epilogues: the stack pointer is
        // put back where it was before the call.
        const top = stackSave();
        try {
            _webcpp_asio_deliver(address, key, Emval.toHandle(event));
        } catch (error) {
            stackRestore(top);
            (globalThis.reportError ?? console.error)(error);
        }
    });
});
```

  (inside the `clang-format off` region), and after it:

```cpp
/** Delivers event to the bridge's registration key; the bridge's JavaScript functions call it. */
extern "C" EMSCRIPTEN_KEEPALIVE void webcpp_asio_deliver(void* bridge, std::uint64_t key,
                                                         emscripten::EM_VAL event) {
    auto& target = *static_cast<webcpp::asio::browser::bridge*>(bridge);
    const auto value = emscripten::val::take_ownership(event);
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    // A C++ exception is caught here, where catching it frees it: one that reached the page
    // would keep its object for good (332 bytes an escape, measured in trystero's
    // browser_event_loop_test.cpp).
    try {
        target.deliver(key, value);
    } catch (const std::exception& e) {
        webcpp_asio_report(emscripten::val::u8string(e.what()).release_ownership());
    } catch (...) {
        webcpp_asio_report(emscripten::val("an exception that is not a std::exception").release_ownership());
    }
#else
    target.deliver(key, value);
#endif
}
```

  Natively the bridge does not exist: `browser/` builds on emscripten alone. `asio.hpp`
  includes it `#ifdef __EMSCRIPTEN__`.
- [ ] **Step 6: Run** the emscripten and asyncify lanes. Expected: PASS, `bridge` and `em_js`
  and their three twins each. See `em_js` able to fail: in a scratch copy, name
  `test_call_after`'s parameter `function` again, run `em_js`, see it FAIL with
  `SyntaxError: Unexpected token 'function'` before `main`, and restore. Then the lint.
  Expected: `lint: clean`.
- [ ] **Step 7: Commit** (library): `"asio: the bridge, through which a browser event only
  posts"`.

## Task 8: Who may call the run family

**Files:**
- Create: `test/detection_test.cpp`.
- Modify: `include/webcpp/asio/io_context.hpp` (the flag, the admission), `test/Jamfile`,
  `doc/run.adoc` (the detection).

**Interfaces:**
- Consumes: `browser::bridge::delivering()` (Task 7).
- Produces: on emscripten, each run family member returns 0 and runs nothing when called
  while another runs or while the bridge delivers, after
  `BOOST_ASSERT_MSG(false, "webcpp::asio: the run family is called on main's stack alone, never from a handler or a browser event")`.
  Admission happens exactly once per public call: a public member never calls another
  public member, only the private, unchecked `run_until_unchecked`, `compose` and `drain`
  (Review Focus 6).

- [ ] **Step 1: Write the failing test** `test/detection_test.cpp` (the notice; a comment:
  the nested and in-event calls of spec section 4.5, caught by the assertion handler, with
  assertions on and off; Review Focus 3 and 6; xactor's driver; `(doc: #asio-invariant-6)`):

```cpp
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include "browser_values.hpp"

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/em_macros.h>

#include <chrono>
#include <cstddef>
#include <stdexcept>

namespace webcpp::test {

/** The assertions the run family made. */
struct assertions {
    int failed = 0;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
assertions seen;  // the assertion handler, a free function of Boost's, reaches it

}  // namespace webcpp::test

namespace boost {

void assertion_failed(const char*, const char*, const char*, long) { ++webcpp::test::seen.failed; }

void assertion_failed_msg(const char*, const char*, const char*, const char*, long) {
    ++webcpp::test::seen.failed;
}

}  // namespace boost

extern "C" EMSCRIPTEN_KEEPALIVE void test_run_from_an_event(webcpp::asio::io_context* io,
                                                            std::size_t* result) {
    *result = io->run();  // lint-run: the call the run family refuses
}

namespace {

#ifdef BOOST_DISABLE_ASSERTS
constexpr int asserted = 0;
#else
constexpr int asserted = 1;
#endif

// A bounded nested call: before this task's admission it runs the other handler and returns
// 1 after 100 ms (a nested run() would wait for good, since the running handler counts as
// outstanding work, asio-plan-review, R/nested_run.cpp); after it, it is refused.
void a_nested_run_is_refused() {
    webcpp::asio::io_context io;
    std::size_t nested = 99;
    bool after = false;
    boost::asio::post(io, [&] { nested = io.run_for(std::chrono::milliseconds(100)); });  // lint-run: the nested call refused
    boost::asio::post(io, [&after] { after = true; });
    const int before = webcpp::test::seen.failed;
    BOOST_TEST_EQ(io.run(), 2U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(nested, 0U);
    BOOST_TEST(after);
    BOOST_TEST_EQ(webcpp::test::seen.failed - before, asserted);
}

void a_call_from_an_event_while_run_waits_is_refused() {
    webcpp::asio::io_context io;
    std::size_t from_event = 99;
    webcpp::asio::steady_timer wait(io, std::chrono::milliseconds(100));
    wait.async_wait([](boost::system::error_code) {});
    EM_ASM({ setTimeout(() => _test_run_from_an_event($0, $1), 20); }, &io, &from_event);
    const int before = webcpp::test::seen.failed;
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(from_event, 0U);
    BOOST_TEST_EQ(webcpp::test::seen.failed - before, asserted);
}

/** A registration whose delivery calls run, as an allocator inside an event could. */
class runs_inside final : public webcpp::asio::browser::registration {
public:
    explicit runs_inside(webcpp::asio::io_context& io) : io_(io) {}
    bool deliver(const emscripten::val&) override {
        result = io_.run();  // lint-run: the in-delivery call refused
        return true;
    }
    void shutdown() noexcept override {}
    std::size_t result = 99;

private:
    webcpp::asio::io_context& io_;
};

void a_call_while_the_bridge_delivers_is_refused() {
    webcpp::asio::io_context io;
    runs_inside r(io);
    auto& bridge = boost::asio::use_service<webcpp::asio::browser::bridge>(io);
    const int before = webcpp::test::seen.failed;
    bridge.function(bridge.add(r))(0);
    BOOST_TEST_EQ(r.result, 0U);
    BOOST_TEST_EQ(webcpp::test::seen.failed - before, asserted);
}

#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
// Review Focus 3: the exception leaves run, and the next run is no nested call.
void an_exception_out_of_run_clears_the_flag() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer later(io, std::chrono::milliseconds(20));
    later.async_wait([](boost::system::error_code) { throw std::runtime_error("from a handler"); });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_TEST(caught);
    int ran = 0;
    boost::asio::post(io, [&ran] { ++ran; });
    io.restart();
    const int before = webcpp::test::seen.failed;
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(ran, 1);
    BOOST_TEST_EQ(webcpp::test::seen.failed, before);
}
#endif

}  // namespace

// Review Focus 6: each public member admits once. run_for spans several turns (100 ready
// handlers, a limit of 10) and waits on a timer alone; run_until, run_one_for and
// run_one_until each run one. No assertion is counted, and every handler runs.
void the_timed_forms_are_not_refused() {
    webcpp::asio::io_context io(webcpp::asio::io_context_options{.max_handlers_per_turn = 10});
    const int before = webcpp::test::seen.failed;
    int ran = 0;
    for (int i = 0; i < 100; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    BOOST_TEST_EQ(io.run_for(std::chrono::seconds(5)), 100U);  // lint-run: the test's event loop
    io.restart();
    webcpp::asio::steady_timer timer(io, std::chrono::milliseconds(20));
    timer.async_wait([&ran](boost::system::error_code) { ++ran; });
    BOOST_TEST_EQ(io.run_for(std::chrono::seconds(5)), 1U);  // lint-run: the test's event loop
    io.restart();
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run_until(std::chrono::steady_clock::now() + std::chrono::seconds(5)), 1U);
    io.restart();
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run_one_for(std::chrono::seconds(5)), 1U);
    io.restart();
    boost::asio::post(io, [&ran] { ++ran; });
    BOOST_TEST_EQ(io.run_one_until(std::chrono::steady_clock::now() + std::chrono::seconds(5)), 1U);
    BOOST_TEST_EQ(ran, 104);
    BOOST_TEST_EQ(webcpp::test::seen.failed, before);
}

// Spec section 4.5: xactor's asio_driver drains an io_context{1} of its own with poll; that
// context has no timer and no socket, so no reactor, and its poll never suspends: polling it
// from a handler of the program's context is safe on both variants.
void a_reactor_less_context_polled_from_a_handler() {
    webcpp::asio::io_context io;
    boost::asio::io_context driver{1};
    int driven = 0;
    boost::asio::post(driver, [&driven] { ++driven; });
    std::size_t polled = 99;
    boost::asio::post(io, [&] { polled = driver.poll(); });
    const int before = webcpp::test::seen.failed;
    BOOST_TEST_EQ(io.run(), 1U);  // lint-run: the test's event loop
    BOOST_TEST_EQ(polled, 1U);
    BOOST_TEST_EQ(driven, 1);
    BOOST_TEST_EQ(webcpp::test::seen.failed, before);
}

int main() {
    a_nested_run_is_refused();
    a_call_from_an_event_while_run_waits_is_refused();
    a_call_while_the_bridge_delivers_is_refused();
    the_timed_forms_are_not_refused();
    a_reactor_less_context_polled_from_a_handler();
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    an_exception_out_of_run_clears_the_flag();
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile` (not through `browser-tests`, since it needs the handler's define):

```jam
# The run family refuses a nested call, and one made while the bridge
# delivers, with assertions on (the handler counts them) and off (it returns
# 0 and runs nothing), on both variants, with and without exceptions.
for local suspend in jspi asyncify
{
    for local handling in on off
    {
        local name = detection_$(suspend)_exceptions_$(handling) ;
        webcpp.run $(name) : detection_test.cpp
          : <define>BOOST_ENABLE_ASSERT_HANDLER <webcpp-suspend>$(suspend)
            <exception-handling>$(handling) : emscripten ;
        webcpp.run $(name)_disabled : detection_test.cpp
          : <define>BOOST_DISABLE_ASSERTS <webcpp-suspend>$(suspend)
            <exception-handling>$(handling) : emscripten ;
        if $(suspend) = asyncify
        {
            .asyncify += $(name) $(name)_disabled ;
        }
    }
}
```

- [ ] **Step 2: Run** `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/asio/test//detection_jspi_exceptions_on`.
  Expected: FAIL, within about a second: `nested` is 1 (the nested `run_for`, admitted, runs
  the other handler and returns at its 100 ms deadline), `after` is set by it, and no
  assertion is counted. `the_timed_forms_are_not_refused` and
  `a_reactor_less_context_polled_from_a_handler` pass already.
- [ ] **Step 3: Implement** in `io_context.hpp`, on emscripten: include
  `<webcpp/asio/browser/bridge.hpp>` and `<boost/assert.hpp>`; a member
  `browser::bridge& bridge_ = boost::asio::use_service<browser::bridge>(*this);` declared
  after `reactor_`, and `bool running_ = false;`; and:

```cpp
    /** Whether a run family call may start: refused while another runs or the bridge delivers. */
    [[nodiscard]] bool admitted() const noexcept {
        const bool allowed = !running_ && !bridge_.delivering();
        BOOST_ASSERT_MSG(allowed, "webcpp::asio: the run family is called on main's stack alone, "
                                  "never from a handler or a browser event");
        return allowed;
    }

    /** Sets running_ for a call's lifetime, and clears it however the call ends. */
    struct running_scope {
        explicit running_scope(bool& flag) noexcept : flag_(flag) { flag_ = true; }
        running_scope(const running_scope&) = delete;
        running_scope& operator=(const running_scope&) = delete;
        ~running_scope() { flag_ = false; }
        bool& flag_;
    };
```

  Each of the six public members starts with `if (!admitted()) { return 0; }` and then
  `const running_scope running(running_);` before its work, and none calls another public
  member: `run_for` and `run_until` call `run_until_unchecked`, `run` calls `compose`,
  `run_one`, `run_one_for` and `run_one_until` call Boost's. State it in the header's comment
  above the run family.
- [ ] **Step 4: Run** the eight detection programs (four in each lane). Expected: PASS,
  `the_timed_forms_are_not_refused` included. See Review Focus 6 able to fail: in a scratch
  copy make `run_for` call the public `run_until`, run `detection_jspi_exceptions_on`, see
  `the_timed_forms_are_not_refused` FAIL on its assertion count, and restore. Then every
  lane. Expected: PASS.
- [ ] **Step 5: Commit** (library): `"asio: the run family refuses a nested call and one made
  from a browser event"`.

## Task 9: Exceptions across a suspension, and Asyncify's stack

**Files:**
- Create: `test/exceptions_test.cpp`.
- Modify: `test/Jamfile`, `build.jam` (the measured `ASYNCIFY_STACK_SIZE`), `AGENTS.md`
  (library: the measurement), `doc/building.adoc`.

**Interfaces:**
- Consumes: the run family (Task 6), the detection (Task 8).
- Produces: the value of `-sASYNCIFY_STACK_SIZE`, measured.

- [ ] **Step 1: Write the failing test** `test/exceptions_test.cpp` (the notice; a comment
  citing `P/timer_try.cpp`, `P/timer_catch.cpp`, `P/turn_limit.cpp`;
  `(doc: #asio-invariant-7)`):

```cpp
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <stdexcept>

namespace {

using namespace std::chrono_literals;

void a_try_encloses_a_suspended_run() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code) { fired = true; });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (...) {
        caught = true;
    }
    BOOST_TEST(fired);
    BOOST_TEST(!caught);
}

void a_catch_block_runs_and_suspends() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    bool fired = false;
    timer.async_wait([&fired](boost::system::error_code) { fired = true; });
    try {
        throw std::runtime_error("before run");
    } catch (const std::runtime_error&) {
        io.run();  // lint-run: the test's event loop, suspended inside a catch block
    }
    BOOST_TEST(fired);
}

void a_handler_s_exception_reaches_main_after_a_suspension() {
    webcpp::asio::io_context io;
    webcpp::asio::steady_timer timer(io, 20ms);
    timer.async_wait([](boost::system::error_code) { throw std::runtime_error("handler"); });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "handler";
    }
    BOOST_TEST(caught);
}

// After a turn's yield too: 2,000 posts, then a throw.
void a_throw_after_a_yield_reaches_main() {
    webcpp::asio::io_context io;
    int ran = 0;
    for (int i = 0; i < 2000; ++i) {
        boost::asio::post(io, [&ran] { ++ran; });
    }
    boost::asio::post(io, [] { throw std::runtime_error("after a yield"); });
    bool caught = false;
    try {
        io.run();  // lint-run: the test's event loop
    } catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_TEST(caught);
    BOOST_TEST_EQ(ran, 2000);
}

}  // namespace

int main() {
    a_try_encloses_a_suspended_run();
    a_catch_block_runs_and_suspends();
    a_handler_s_exception_reaches_main_after_a_suspension();
    a_throw_after_a_yield_reaches_main();
    return webcpp::test::exit_status(boost::report_errors());
}
```

  In `test/Jamfile`:

```jam
# Throws and catches across a suspension need exceptions, whatever the build
# asks: WebAssembly's under jspi, JavaScript's under asyncify (the feature's
# composition), and natively for the twin.
webcpp.run exceptions : exceptions_test.cpp : <exception-handling>on : native emscripten ;
webcpp.run exceptions_asyncify : exceptions_test.cpp
  : <exception-handling>on <webcpp-suspend>asyncify : emscripten ;
.asyncify += exceptions_asyncify ;
```

- [ ] **Step 2: Run** `exceptions_asyncify` with `-sASYNCIFY_STACK_SIZE` set to 64 in
  `build.jam` for the run. Expected: FAIL, a `RuntimeError` (the unwind data overflows),
  which shows the test reaches the deepest suspension. Put 4096 back.
- [ ] **Step 3: Measure the stack size.** For each n of 256, 512, 1024, 2048 and 4096: set
  `-sASYNCIFY_STACK_SIZE=<n>` in `build.jam`, run
  `$B2 -a --build-dir=bin/b-stack toolset=emscripten libs/asio/test//asyncify`, and record in
  the ledger whether every test passed. Let S be the smallest n with which every test passes;
  set the value to the larger of 2·S and 4096, with a comment that says S, the date, and that
  Task 16 measures it again with Cobalt. Record the result in `libs/asio/AGENTS.md`
  ("Cautions") and in `doc/building.adoc` (as the reason, without the number: the page names
  the setting).
- [ ] **Step 4: Run** every lane. Expected: PASS, `exceptions` natively and on both variants.
- [ ] **Step 5: Commit** (library): `"asio: exceptions across a suspension on both variants,
  and Asyncify's stack measured"`.

## Task 10: Timers: Boost's, Beast's

**Files:**
- Create: `test/timers_test.cpp`, `doc/timers.adoc`.
- Modify: `test/Jamfile`, `doc/asio.adoc`.

**Interfaces:**
- Consumes: the run family, the timer aliases (Tasks 4, 6).
- Produces: nothing new; the proof that Boost's timers and Beast's run in the reactor's queue.

- [ ] **Step 1: Write the page's section** `doc/timers.adoc` (`[#asio-timers]`, "Timers"):
  spec section 5.1, and that Beast's timeouts are Boost timers
  (`B/beast/core/basic_stream.hpp:251`, `B/beast/websocket/impl/stream_impl.hpp:72`).
- [ ] **Step 2: Write the failing test** `test/timers_test.cpp` (natively and in the browser;
  `(doc: #asio-invariant-8)`): three `steady_timer`s of 60, 20 and 40 ms complete in the order
  20, 40, 60; a `system_timer` of 30 ms completes; a timer cancelled before its expiry
  completes with `boost::asio::error::operation_aborted`; a per-operation cancellation through
  `boost::asio::bind_cancellation_slot` (a `boost::asio::cancellation_signal` emitted with
  `cancellation_type::terminal` from a 10 ms timer's handler) completes a 10 s wait with
  `operation_aborted`; and Beast's WebSocket handshake timeout, as `Q/beast-timer/beast_timer.cpp`
  measured:

```cpp
void beast_s_handshake_timeout_fires() {
    webcpp::asio::io_context io;
    boost::beast::test::stream peer(io);
    boost::beast::websocket::stream<boost::beast::test::stream> ws(io);
    boost::beast::websocket::stream_base::timeout timeout{};
    timeout.handshake_timeout = std::chrono::milliseconds(100);
    timeout.idle_timeout = boost::beast::websocket::stream_base::none();
    timeout.keep_alive_pings = false;
    ws.set_option(timeout);
    ws.next_layer().connect(peer);
    boost::system::error_code accepted;
    ws.async_accept([&accepted](boost::system::error_code ec) { accepted = ec; });
    io.run();  // lint-run: the test's event loop
    BOOST_TEST(accepted == boost::beast::error::timeout);
}
```

  `timers` runs natively (`webcpp.run timers_native : timers_test.cpp : : native ;`) and
  joins `browser-tests`.
- [ ] **Step 3: Run** `timers` on each lane. Expected: PASS: the timers are Boost's and need
  no code of asio's, so the test is the proof itself. See that it can fail: in a scratch copy
  expect the order 60, 20, 40, run it on the emscripten lane, see `timers` FAIL naming the
  order, and restore.
- [ ] **Step 4: Commit** (library): `"asio: Boost's timers and Beast's timeouts in the
  browser's event loop"`.

## Task 11: Allocation

**Files:**
- Create: `test/allocation_test.cpp`, `doc/allocation.adoc`.
- Modify: `test/Jamfile`, `doc/asio.adoc`.

**Interfaces:**
- Consumes: `io_context(std::allocator_arg, allocator, options)`; the bridge.
- Produces: the counting test the spec's section 8 asks for, which Tasks 17, 18, 19 and 20
  extend with their operations.

- [ ] **Step 1: Write the page's section** `doc/allocation.adoc` (`[#asio-allocation]`): the
  io_context takes the user's allocator; operations allocate through their handler's
  associated allocator; the runtime's own `malloc`s, by name (Emscripten's `select` callocs
  its poll array per call, `E/system/lib/libc/musl/src/select/select.c:71`; Asyncify
  allocates its unwind data per suspension, `E/src/lib/libasync.js:415-416`; Asio's
  `basic_resolver_results` allocates its entries with `new`,
  `B/asio/ip/basic_resolver_results.hpp:159-166`); which types are shareable (none: all are
  run-time objects).
- [ ] **Step 2: Write the failing test** `test/allocation_test.cpp`. It counts what spec
  section 8 names, both ways a program can allocate, as `libs/trystero/test/allocation_test.cpp`
  does (its lines 52-321, taken over with `TRYSTERO` as `ASIO` in the macros' names):
  - every form of the global `operator new` (plain, array, aligned, nothrow), replaced,
    counted in `operator_new_calls`;
  - the C library's `malloc`, `calloc`, `realloc`, counted in `c_allocations`, and
    `aligned_alloc`, `posix_memalign` and `memalign`, counted in `aligned_allocations`
    (Asio's default handler memory ends in `std::aligned_alloc` on macOS and glibc and in
    Boost.Align's `aligned_alloc` on Emscripten, `B/asio/detail/memory.hpp:103-138`,
    `B/asio/detail/config.hpp:383-412`, never in `operator new`): replaced and forwarded to
    `__libc_*` on glibc and to `emscripten_builtin_*` on Emscripten, seen through
    `malloc_logger` on macOS (where both kinds land in `c_allocations`), not counted on MSVC
    (`WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS` is then undefined and the C library's cases are
    skipped, as trystero's are);
  - the counting allocator takes its memory from the uncounted C library (trystero's `take`
    and `release`), never from `std::malloc`, so that what it gives is counted once, in
    `given`.

  The two runtime allocations spec section 8 names are kept out, by name, not by a
  tolerance: Emscripten's `select`, which callocs its poll array on every call
  (`select.c:71`), is replaced by trystero's own `select`, which keeps the array on the stack;
  and Asyncify's unwind data (`libasync.js:415-416`) is taken while a flag in WebAssembly's
  memory is set, which the C library's counters read:

```cpp
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::uint8_t inside_runtime = 0;  // set by JavaScript while Asyncify allocates its unwind data

/** Marks Asyncify's own allocations, which spec section 8 names as the runtime's. */
void mark_the_runtime_s_allocations() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const flag = $0;
        if (typeof Asyncify === 'object' && typeof Asyncify.allocateData === 'function') {
            const allocate = Asyncify.allocateData;
            Asyncify.allocateData = function(...values) {
                HEAPU8[flag] = 1;
                try {
                    return allocate.apply(this, values);
                } finally {
                    HEAPU8[flag] = 0;
                }
            };
        }
    }, &inside_runtime);
#endif
}
```

  (`counted(pointer)` counts nothing while `inside_runtime` or `inside_operator_new` is set.)
  The cases, natively and on both variants, each counting only inside its window:
  - `the_user_s_allocator_takes_everything`: an `io_context` built with `counted<void>`, an
    armed `steady_timer` of 5 ms whose wait is bound with
    `boost::asio::bind_allocator(counted<void>(&given), ...)`, ten posts bound likewise, and on
    emscripten a bridge registration whose `deliver` posts through
    `bind_allocator(counted<void>(&given), ...)` and receives two events; then `run`.
    Expected: `operator_new_calls == 0`, `c_allocations == 0`, `aligned_allocations == 0`, and
    `given > 0` (18 for the same scenario without the bridge, natively and on JSPI,
    `Q/alloc/alloc.cpp`, which counted `operator new` alone);
  - Review Focus 8, `the_counters_see_an_unbound_handler` (where the C library is counted):
    the same `io_context`, and one `boost::asio::post` of a handler with no associated
    allocator, made outside `run`. Expected: `c_allocations + aligned_allocations > 0` and
    `operator_new_calls == 0`: the allocation Asio's default handler memory makes is seen,
    which a test that counted `operator new` alone would have missed.

  `allocation` runs natively (`webcpp.run allocation_native : allocation_test.cpp : : native ;`)
  and joins `browser-tests`. Every later task that adds an operation to this file binds every
  handler and every post it makes to `counted<void>`.
- [ ] **Step 3: Run** with the io_context built without the allocator (a one-line change in a
  scratch copy). Expected: FAIL, `operator_new_calls` above 0 (without one, Boost allocates
  the context's services with `std::allocator`, which calls `operator new`). Restore; run
  every lane.
  Expected: PASS, the control case included.
- [ ] **Step 4: Commit** (library): `"asio: every allocation through the user's allocator, but
  the runtime's own"`.

## Task 12: The loader

**Files:**
- Create: `include/webcpp/asio/webcpp-asio-loader.mjs`; in `test/browser/`: `Jamfile`,
  `package.json`, `package-lock.json`, `loader_test.mjs`, `page_program.cpp`.
- Modify: `doc/building.adoc` (the loader), `README.md`.

**Interfaces:**
- Consumes: `webcpp.variants` and the manifest (Task 3).
- Produces:

```js
export function supportsJspi(scope = globalThis)          // boolean
export function pick(manifest, jspi = supportsJspi())      // {variant, script}
export async function load(manifestUrl, options = {})      // {variant, module}
// options: jspi, module (the factory's argument), fetch, importModule
```

  and the own lane `browser` on emscripten, in `libs/asio/test/browser/`, with its first
  driven test `loader_node`.

- [ ] **Step 1: Write the page's part** in `doc/building.adoc`: "Loading the variant a
  browser runs": a page imports `webcpp-asio-loader.mjs` (installed with the headers, in
  `include/webcpp/asio/`) and calls `load('<name>.variants.json')`; the Jamfile line
  `webcpp.variants page : page.cpp ;`; JSPI detected by
  `typeof WebAssembly.Suspending === "function"`; the manifest's form.
- [ ] **Step 2: Write the driven test's files.** `page_program.cpp` (the notice): includes
  `<webcpp/asio.hpp>` and `"../browser_values.hpp"`, so that loading it parses every `EM_JS`
  of the library and of the tests (I1's check, in Chrome, Task 13's `--mode loader`); posts
  three handlers to a `webcpp::asio::io_context`, runs it
  (`// lint-run: the page's event loop, suspended into the browser's`), prints
  `ran 3 handlers` with `std::puts`, and returns 0: a page's program, not a test.
  `package.json`:

```json
{
    "name": "webcpp-asio-browser-tests",
    "private": true,
    "type": "module",
    "description": "The browser lane's drivers: node built-ins only."
}
```

  and its lockfile from `npm install --package-lock-only`. `loader_test.mjs`:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The loader under node: what it detects, what it picks, and a load of each variant of the
// program b2 passes last, through file URLs (node's fetch reads no file: URL, so the test
// gives the loader its own).
import assert from 'node:assert/strict'
import {readFileSync} from 'node:fs'
import {dirname, join} from 'node:path'
import {pathToFileURL} from 'node:url'
import {load, pick, supportsJspi} from '../../include/webcpp/asio/webcpp-asio-loader.mjs'

const program = process.argv.at(-1)
const manifestPath = join(dirname(program), 'page_program.variants.json')
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'))
const fetchFile = async url => ({ok: true, status: 200, statusText: 'OK',
    json: async () => JSON.parse(readFileSync(new URL(url), 'utf8'))})

assert.equal(supportsJspi({WebAssembly: {Suspending: function () {}}}), true)
assert.equal(supportsJspi({WebAssembly: {}}), false)
assert.equal(supportsJspi({}), false)
assert.deepEqual(pick(manifest, true), {variant: 'jspi', script: manifest.jspi.script})
assert.deepEqual(pick(manifest, false), {variant: 'asyncify', script: manifest.asyncify.script})
assert.throws(() => pick({jspi: manifest.jspi}, false), /no asyncify artifact/)
for (const jspi of [true, false]) {
    const lines = []
    const {variant} = await load(pathToFileURL(manifestPath), {jspi, fetch: fetchFile,
        module: {print: line => lines.push(line)}})
    assert.equal(variant, jspi ? 'jspi' : 'asyncify')
    await new Promise(resolve => setTimeout(resolve, 200))
    assert.deepEqual(lines, ['ran 3 handlers'])
}
console.log('loader: ok')
```

  `Jamfile`:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The browser lane: the loader under node, and the pages in headless Chrome
# (run.mjs, Task 13). Each driver gets the jspi artifact of a page program,
# beside its manifest.
project : requirements <library>/webcpp/asio//asio ;

import webcpp ;

webcpp.targets emscripten ;

webcpp.original node ;

webcpp.variants page_program : page_program.cpp ;
webcpp.drive loader_node : page_program
  : <dependency>page_program.variants.json <webcpp-drive-timeout>60
  : loader_test.mjs ;

webcpp.lane browser : loader_node : emscripten ;
```

- [ ] **Step 3: Run** `$B2 -a --build-dir=bin/lane-browser toolset=emscripten libs/asio/test/browser//browser`.
  Expected: FAIL, `Cannot find module .../webcpp-asio-loader.mjs`.
- [ ] **Step 4: Implement** `include/webcpp/asio/webcpp-asio-loader.mjs`:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Loads the variant of a webcpp::asio program that this browser runs: jspi where JavaScript
// Promise Integration exists, asyncify elsewhere, by the names the build wrote into the
// program's manifest, <name>.variants.json (webcpp.variants), never by a pattern.

/** Whether scope has JavaScript Promise Integration. */
export function supportsJspi(scope = globalThis) {
    return typeof scope.WebAssembly?.Suspending === 'function'
}

/** The variant to load from manifest, and its script relative to the manifest. */
export function pick(manifest, jspi = supportsJspi()) {
    const variant = jspi ? 'jspi' : 'asyncify'
    const script = manifest?.[variant]?.script
    if (typeof script !== 'string') {
        throw new Error(`the manifest names no ${variant} artifact`)
    }
    return {variant, script}
}

/**
 * Fetches the manifest at manifestUrl, imports the variant this browser runs, and calls its
 * factory with options.module. options.fetch and options.importModule replace the browser's
 * own, for a page that loads otherwise and for a test under node.
 */
export async function load(manifestUrl, options = {}) {
    const {jspi = supportsJspi(), module = {}, fetch = globalThis.fetch,
        importModule = url => import(url)} = options
    const base = new URL(manifestUrl, globalThis.location?.href ?? import.meta.url)
    const response = await fetch(base.href)
    if (!response.ok) {
        throw new Error(`${base.href}: ${response.status} ${response.statusText}`)
    }
    const {variant, script} = pick(await response.json(), jspi)
    const factory = (await importModule(new URL(script, base).href)).default
    return {variant, module: await factory(module)}
}
```

- [ ] **Step 5: Run** the browser lane. Expected: PASS, `loader_node`. Then the lint (it reads
  the `.mjs` for its notice and the Jamfile for its raw rules). Expected: `lint: clean`.
- [ ] **Step 6: Commit** (library): `"asio: the loader, which picks the variant a browser
  runs from the build's manifest"`.

## Task 13: The browser lane in headless Chrome (and the CI's Chrome)

**Files:**
- Create in `libs/asio/test/browser/`: `run.mjs`, `page.html`, `idle_program.cpp`,
  `render_program.cpp`, `render_unlimited_program.cpp`.
- Modify: `libs/asio/test/browser/Jamfile`, `libs/asio/AGENTS.md` (the measurement).
- Modify (superproject): `tools/ci/matrix.py` (`CHROME_LANES`), `tools/ci/matrix_test.py`,
  `AGENTS.md` (chapter 9).

**Interfaces:**
- Consumes: the loader (Task 12), the run family (Task 6).
- Produces: the driven tests `browser_idle`, `browser_render`, `browser_loader` in the lane
  `browser`; `CHROME_LANES = {('trystero', 'interop'), ('asio', 'browser')}`.

- [ ] **Step 1: Write the failing superproject test.** In `tools/ci/matrix_test.py`, a case
  that `own_lane_entry` of `OwnLane('asio', 'browser', ('libs/asio/test/browser',),
  'emscripten', 'original')` has `chrome` true. Run `python3 tools/ci/matrix_test.py`.
  Expected: FAIL. Add `('asio', 'browser')` to `CHROME_LANES` in `tools/ci/matrix.py`, with
  its comment updated ("trystero's interop, natively and on emscripten, and asio's browser
  lane"), and the same words in AGENTS.md chapter 9 where it names `CHROME_LANES`. Run.
  Expected: PASS. Commit (superproject): `"tools/ci: asio's browser lane gets the pinned
  Chrome"` with `tools/ci/matrix.py tools/ci/matrix_test.py AGENTS.md`.
- [ ] **Step 2: Write the programs.** `idle_program.cpp`: prints `idle` (through `std::puts`,
  which the page's `print` receives), then runs a `webcpp::asio::io_context` held by a work
  guard that nothing releases. `render_program.cpp`: prints `spinning`, then runs a
  self-posting handler on a `webcpp::asio::io_context` for one second (`steady_clock`), then
  prints `done <handlers>`. `render_unlimited_program.cpp`: the same through
  `static_cast<boost::asio::io_context&>(io).run()` (`// lint-run: Boost's own run, the
  control`).
- [ ] **Step 3: Write `page.html`** (the HTML licence form; its comment: what the drivers
  open):

```html
<!--
Copyright (c) 2026 WebCpp.org

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
https://www.boost.org/LICENSE_1_0.txt)

The page the browser lane opens: it loads the program the query names with
webcpp-asio-loader.mjs and records what it prints, with the time of each line.
-->
<!doctype html>
<meta charset="utf-8">
<title>asio browser lane</title>
<script type="module">
import {load} from '/loader/webcpp-asio-loader.mjs'
const report = (window.report = {lines: [], variant: null, error: null, frames: []})
const tick = time => {
    report.frames.push(time)
    requestAnimationFrame(tick)
}
requestAnimationFrame(tick)
try {
    const manifest = new URLSearchParams(location.search).get('manifest')
    const print = line => report.lines.push({line, at: performance.now()})
    report.variant = (await load(manifest, {module: {print}})).variant
} catch (error) {
    report.error = String(error)
}
</script>
```

- [ ] **Step 4: Write `run.mjs`**, the driver, `node run.mjs --mode <idle|render|loader>
  <program>`, modelled on `libs/trystero/test/oracle/interop/run.mjs` (its `findChrome`,
  `startChrome` and `openPage`, lines 46-200, copied with the trystero-specific parts left
  out: no relay, no native peers, no `--host-resolver-rules` exception but `127.0.0.1`):
  - it serves `/build/` from the program's directory, `/loader/` from
    `libs/asio/include/webcpp/asio/`, and `/` from its own directory, on `127.0.0.1` at a
    port the system chooses;
  - `--mode idle`: before navigation, `Page.addScriptToEvaluateOnNewDocument` installs the
    wrapper:

```js
window.timers = []
for (const name of ['setTimeout', 'setInterval']) {
    const original = window[name]
    window[name] = (callback, delay = 0, ...rest) => {
        window.timers.push({name, delay, at: performance.now()})
        return original(callback, delay, ...rest)
    }
}
const post = MessagePort.prototype.postMessage
MessagePort.prototype.postMessage = function (...values) {
    window.timers.push({name: 'postMessage', delay: 0, at: performance.now()})
    return post.apply(this, values)
}
```

    opens `page.html?manifest=/build/idle_program.variants.json`, waits for the line `idle`,
    then 3 s, and fails unless the timers recorded after that line are exactly one
    `setTimeout` whose delay is above 0 and at most 300000 (the reactor's one wait, spec
    section 9);
  - `--mode render`: before navigation, a second init script times each turn itself, at the
    yield's own `MessageChannel` (the only one these programs make): the moment the yield
    posts its message ends a turn, and the moment its port's `onmessage` runs, which resumes
    WebAssembly, starts the next:

```js
window.turns = {posted: [], resumed: []}
const Channel = window.MessageChannel
const onmessage = Object.getOwnPropertyDescriptor(MessagePort.prototype, 'onmessage')
window.MessageChannel = class extends Channel {
    constructor() {
        super()
        const sender = this.port2
        const post = sender.postMessage.bind(sender)
        sender.postMessage = value => {
            window.turns.posted.push(performance.now())
            post(value)
        }
        const receiver = this.port1
        Object.defineProperty(receiver, 'onmessage', {
            get: () => onmessage.get.call(receiver),
            set: handler => onmessage.set.call(receiver, event => {
                window.turns.resumed.push(performance.now())
                handler(event)
            }),
        })
    }
}
```

    It opens the page for `render_program`, waits for `done`, counts the animation frames
    between `spinning` and `done`, and takes each turn's time: the first from the `spinning`
    line to the first post, each next one from a resume to the next post. Then the same for
    `render_unlimited_program`. It fails unless the limited program saw at least 10 frames
    (10 frames a second, a sixth of a 60 Hz display) and the unlimited one saw none, and it
    prints, for the ledger,
    `render <variant>: <frames> frames, <handlers> handlers in 1 s, <turns> turns, turn p95 <ms> ms, max <ms> ms`
    (the 95th percentile: the turns sorted, the one at index `ceil(0.95 n) - 1`);
  - `--mode loader`: opens the page for `page_program` and expects `variant` `jspi` and the
    line `ran 3 handlers`; then, with `Page.addScriptToEvaluateOnNewDocument` running
    `delete WebAssembly.Suspending` first, expects `asyncify` and the same line; a
    `SyntaxError` of any `EM_JS` the program links fails it at load;
  - `--mode idle` and `--mode render` run twice, the second time with
    `delete WebAssembly.Suspending` first, so that the loader picks `asyncify`, whose
    composition unwinds the whole stack at each yield: the checks are the same, and the page's
    `report.variant` must say which ran;
  - every mode fails on a `Runtime.exceptionThrown`, a `console.error` or `report.error`, and
    leaves no process or profile behind (trystero's `cleanUp`).
- [ ] **Step 5: Declare the drives** in the Jamfile, before the lane, which becomes
  `webcpp.lane browser : loader_node browser_idle browser_render browser_loader : emscripten ;`:

```jam
webcpp.variants idle_program : idle_program.cpp ;
webcpp.variants render_program : render_program.cpp ;
webcpp.variants render_unlimited_program : render_unlimited_program.cpp ;

# Each in headless Chrome, which CHROME names (the CI's chrome action), or on
# macOS Playwright's chrome-headless-shell: 120 s, beyond Chrome's start and
# the 3 s an idle page is watched.
webcpp.drive browser_idle : idle_program
  : <dependency>idle_program.variants.json <webcpp-drive-timeout>120
  : run.mjs : --mode idle ;
webcpp.drive browser_render : render_program
  : <dependency>render_program.variants.json
    <dependency>render_unlimited_program.variants.json <webcpp-drive-timeout>120
  : run.mjs : --mode render ;
webcpp.drive browser_loader : page_program
  : <dependency>page_program.variants.json <webcpp-drive-timeout>120
  : run.mjs : --mode loader ;
```

- [ ] **Step 6: Run** the browser lane before `run.mjs` exists. Expected: FAIL, `there is no
  run.mjs beside it`. Then with it. Expected: PASS, the four driven tests. If the headless
  shell paints no frame at all (the limited program's count 0), stop and report it: the test
  then needs another witness of rendering, which is the owner's call.
- [ ] **Step 7: Record the measurement.** Run the lane three times; record each `render`
  line, both variants, in the ledger and in `libs/asio/AGENTS.md` ("The turn limit, measured
  in Chrome"): frames, turns, and the 95th percentile and the maximum of a turn of 1,000
  trivial handlers. **The stop rule: if the 95th percentile of either variant is above 8 ms
  in any of the three runs, stop; the default is the owner's to change.** Why that bound: at
  60 Hz a frame is 16.7 ms, in which the browser must also run its own work (style, layout,
  paint, input); a turn of at most half the frame leaves it the other half, so a page whose
  turns stay under 8 ms keeps its frame rate. The 95th percentile, not the mean, because the
  mean over a second also counts the yields and the frames painted between turns, which
  hides the long turns a frame suffers from; not the maximum, because one turn the engine's
  garbage collector or the machine's scheduler stretched should not decide a default. The
  maximum is recorded beside it, for the owner.
- [ ] **Step 8: Commit** (library): `"asio: the browser lane: no polling, rendering goes on,
  and the loader picks the variant, in headless Chrome"`.

## Task 14: The guide's examples and the size table

**Carried from Task 6 (controller, 2026-10-11):** `doc/run.adoc` already defines `[[asio-invariant-2]]` and `[[asio-invariant-3]]`; this task's numbered list takes them over (move them, never define them twice), and the doc build checks the anchors resolve once.
Also from Task 6's re-review: run_family_test.cpp:7 cites `#asio-invariant-3` for "keeps Boost.Asio's meaning", wider than the turn limit: repoint it when the guarantees are numbered; and run.adoc's "fewer than twice the limit and one" reads "at most twice the limit".

**Files:**
- Create in `libs/asio/example/`: `asio_quick_start.cpp` and `.expected`,
  `asio_turn_limit.cpp` and `.expected`, `asio_timers.cpp` and `.expected`.
- Create: `doc/quick-start.adoc`, `doc/guarantees.adoc`.
- Modify: `example/Jamfile`, `doc/asio.adoc`, `doc/run.adoc`, `doc/timers.adoc`,
  `README.md` (its minimal example, included from `asio_quick_start.cpp`).

**Interfaces:**
- Consumes: Tasks 3 to 11.
- Produces: the examples, twins natively and on emscripten (spec section 9, "Twins"); the size
  table `asio.sizes.md` in the lane `asyncify`; the numbered guarantees.

- [ ] **Step 1: Write the guarantees** `doc/guarantees.adoc` (`[#asio-guarantees]`), each
  with its anchor, which earlier tasks' tests cite:
  1. `[[asio-invariant-1]]` Natively every name of asio is Boost.Asio's, and its io_context
     is Boost's with options that change nothing.
  2. `[[asio-invariant-2]]` On emscripten, handlers run only on the stack of a run family call
     on `main`'s stack; work posted and events arrived before it wait for it.
  3. `[[asio-invariant-3]]` At most `max_handlers_per_turn` handlers run before the run family
     yields to the browser through a macrotask, and a timed form returns at its deadline.
  4. `[[asio-invariant-4]]` A browser event delivered through the bridge only posts; a
     listener is no outstanding work.
  5. `[[asio-invariant-5]]` A registration's id is never reused; an event of a removed id, or
     of a destroyed io_context, finds nothing.
  6. `[[asio-invariant-6]]` A run family call made from a handler, or while the bridge
     delivers, asserts, and returns 0 having run nothing.
  7. `[[asio-invariant-7]]` A handler's exception propagates out of `run`, as Boost's, on both
     variants.
  8. `[[asio-invariant-8]]` Timers are Boost's, in the reactor's queue, which `run` waits on.
  9. `[[asio-invariant-9]]` Every allocation goes through the io_context's allocator or the
     handler's associated one, but the runtime's own, named.
  Tasks 17 to 21 add 10 to 13.
- [ ] **Step 2: Write the examples** (each with `// tag::program[]` regions the page shows,
  `namespace asio = webcpp::asio;`, and output with no time in it):
  - `asio_quick_start.cpp`: a `steady_timer` of 50 ms and a post; prints
    `posted`, `timer fired`, `run returned 2 handlers`;
  - `asio_turn_limit.cpp`: an `io_context_options{.max_handlers_per_turn = 250}`, 5,000
    self-posts; prints `max_handlers_per_turn: 250` and `handlers run: 5000`;
  - `asio_timers.cpp`: timers of 60, 20 and 40 ms and one cancelled; prints
    `20 ms`, `40 ms`, `60 ms`, `cancelled: operation_aborted`.
  Each `.expected` holds exactly those lines. `example/Jamfile` gains
  `webcpp.example asio_quick_start.cpp ;`, `webcpp.example asio_turn_limit.cpp ;`,
  `webcpp.example asio_timers.cpp ;` (native and emscripten, the Jamfile's), and:

```jam
# Each example as a page loads it, in both variants, and the table of their
# sizes, built in the own lane asyncify: Asyncify's artifact is about three
# times the size of JSPI's (P/timer.cpp: 283,409 bytes against 97,429).
webcpp.variants page-asio_quick_start : asio_quick_start.cpp ;
webcpp.variants page-asio_turn_limit : asio_turn_limit.cpp ;
webcpp.variants page-asio_timers : asio_timers.cpp ;
webcpp.size-table asio
  : page-asio_quick_start page-asio_turn_limit page-asio_timers ;
webcpp.lane asyncify : asio.sizes.md : emscripten ;
```

- [ ] **Step 3: Run** the native and emscripten lanes before the `.expected` files exist.
  Expected: FAIL, each example naming its missing `.expected`. Write them; run again.
  Expected: PASS, the same output on both targets. Run the asyncify lane. Expected: PASS, and
  the build prints the table of three rows.
- [ ] **Step 4: Write the page's** `quick-start.adoc` (the quick start's listing and output)
  and include the other two examples in `run.adoc` and `timers.adoc`; include
  `guarantees.adoc` before `testing.adoc`. Run `$B2 -a --build-dir=bin/b-doc libs/asio/doc`.
  Expected: PASS, no finding of doc-check.
- [ ] **Step 5: Commit** (library): `"asio: the guide's examples, twins natively and in the
  browser, their sizes, and the guarantees"`.

## Task 15: Boost.Cobalt's sources (superproject)

**Files:**
- Modify: `Jamroot` (`/webcpp//boost_cobalt`), `tools/ci/actions/boost/install.sh`,
  `tools/ci/actions/boost/action.yml` (its description), `tools/ci/actions_test.py`,
  `tools/test/jamroot_test.py`, `AGENTS.md` (chapters 1, 2, 9, 13).
- Machine-local: `ln -s boost-source/libs/cobalt/src ~/dev/webcpp/.local/boost-cobalt`.

**Interfaces:**
- Produces: `/webcpp//boost_cobalt` (static, built once per variant): every `.cpp` of
  `src/`, `src/detail/` and `src/io/` but `main.cpp` and `io/ssl.cpp`, with
  `BOOST_COBALT_SOURCE` and `BOOST_COBALT_IO_SOURCE`, and on emscripten
  `BOOST_ASIO_DISABLE_THREADS` (in its requirements and its usage requirements); Cobalt's
  sources from `-sBOOST_COBALT_SOURCE=<dir>`, else `.local/boost-cobalt`; the Boost action
  keeps them in `<prefix>/cobalt-src` and `configure` copies them to `.local/boost-cobalt`.

- [ ] **Step 1: Write the failing tests.** In `tools/test/jamroot_test.py`:

```python
def test_boost_cobalt_names_where_it_looked(root):
    # Without Cobalt's sources, a build that uses /webcpp//boost_cobalt stops, naming both
    # places; one that does not use it builds.
    result = harness.run_b2(root, '/webcpp//boost_cobalt')
    harness.expect(result, False, '-sBOOST_COBALT_SOURCE=<dir>', '.local/boost-cobalt')
    harness.expect(harness.run_b2(root, 'libs/demo/test'), True)


def test_boost_cobalt_builds_from_the_given_sources(root):
    sources = root / 'cobalt-src'
    for name in ('channel.cpp', 'main.cpp', 'detail/util.cpp', 'io/sleep.cpp', 'io/ssl.cpp'):
        (sources / name).parent.mkdir(parents=True, exist_ok=True)
        (sources / name).write_text('int webcpp_test_' + name.replace('/', '_').replace('.', '_')
                                    + ' = 0;\n')
    result = harness.run_b2(root, '-a', '-d+2', f'-sBOOST_COBALT_SOURCE={sources}',
                            '/webcpp//boost_cobalt')
    harness.expect(result, True)
    compiled = set(re.findall(r'cobalt-src/([a-z/_]+\.cpp)"', result.stdout))
    assert compiled == {'channel.cpp', 'detail/util.cpp', 'io/sleep.cpp'}, compiled
```

  In `tools/ci/actions_test.py`, the stand-in archive gains `libs/cobalt/src/channel.cpp`,
  `libs/cobalt/src/main.cpp` and `libs/cobalt/src/io/sleep.cpp`, and
  `test_boost_installs_b2_and_the_headers_where_configure_finds_them` also asserts
  `(prefix / 'cobalt-src/channel.cpp').is_file()` and, after `configure`,
  `(scratch / '.local/boost-cobalt/io/sleep.cpp').is_file()`; and
  `test_boost_install_fails_when_the_prefix_is_incomplete`'s stand-in that installs nothing
  still fails, now also naming `cobalt-src`. Run both files. Expected: FAIL.
- [ ] **Step 2: Implement.** The Boost action: `install` copies `libs/cobalt/src` of the
  checked archive to `<prefix>/cobalt-src` before removing the work directory; `check`
  requires `<prefix>/cobalt-src/channel.cpp` too, and its message becomes
  `install.sh: <prefix> holds no Boost headers, no b2 or no cobalt-src`; `configure` copies
  `<prefix>/cobalt-src` to `.local/boost-cobalt` (`rm -rf` first; a copy, which every runner's
  shell makes the same way); the header comment's last line becomes "Only headers are
  installed, and Boost.Cobalt's sources kept: webcpp::asio supplies Cobalt's `run_main`
  (WEBCPP_ASIO_SPEC.md section 5.2), so webcpp compiles the rest." The Jamroot, after
  `path-constant WEBCPP_ROOT : . ;` and Task 2's `node-test` (the block reads
  `$(WEBCPP_ROOT)`, which `boost_json`'s place, earlier in the file, does not have yet):

```jam
# Boost.Cobalt's definitions, compiled from its sources, as Boost.JSON's are:
# every source of libs/cobalt/src but main.cpp, whose
# main_promise::run_main webcpp::asio defines in its place
# (libs/asio/src/cobalt_main.cpp), and io/ssl.cpp, which needs OpenSSL.
# The sources are -sBOOST_COBALT_SOURCE=<dir> on the command line, else
# .local/boost-cobalt, where the Boost action copies them; never the
# environment. A build that uses the target and finds neither stops.
.cobalt-local = [ path.join [ path.root [ path.make $(WEBCPP_ROOT) ] [ path.pwd ] ]
    .local/boost-cobalt ] ;
.cobalt-given = [ MATCH "^-sBOOST_COBALT_SOURCE=(.+)$" : [ modules.peek : ARGV ] ] ;
.cobalt = $(.cobalt-local) ;
if $(.cobalt-given)
{
    .cobalt = [ path.root [ path.make $(.cobalt-given[-1]) ] [ path.pwd ] ] ;
}

rule cobalt-found ( properties * )
{
    if ! [ path.exists $(.cobalt)/channel.cpp ]
    {
        local looked = "no -sBOOST_COBALT_SOURCE=<dir> was given, and"
            [ path.native $(.cobalt-local) ] "holds no channel.cpp" ;
        if $(.cobalt-given)
        {
            looked = "-sBOOST_COBALT_SOURCE=$(.cobalt-given[-1]) holds no channel.cpp" ;
        }
        errors.user-error "Boost.Cobalt's sources (libs/cobalt/src of Boost 1.92) were"
            "not found: $(looked). Give them with -sBOOST_COBALT_SOURCE=<dir>, or link"
            "them at .local/boost-cobalt." ;
    }
}

lib boost_cobalt
  : [ path.glob $(.cobalt) $(.cobalt)/detail $(.cobalt)/io : *.cpp
      : main.cpp ssl.cpp ]
  : <link>static <conditional>@cobalt-found
    <define>BOOST_COBALT_SOURCE <define>BOOST_COBALT_IO_SOURCE
    <toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS
  : : <toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS ;
```

  and `boost_cobalt` joins the `explicit` list. Link `.local/boost-cobalt` on this machine.
- [ ] **Step 3: Run** `python3 tools/test/jamroot_test.py` and
  `python3 tools/ci/actions_test.py`. Expected: PASS. Then
  `$B2 -a --build-dir=bin/b-cobalt /webcpp//boost_cobalt` and the same with
  `toolset=emscripten` and with `toolset=emscripten webcpp-suspend=asyncify`. Expected: PASS,
  24 sources each (the 26 of Boost 1.92's `libs/cobalt/src` less `main.cpp` and `io/ssl.cpp`;
  each of the 24 compiles with em++ 6.0.11 and `BOOST_ASIO_DISABLE_THREADS`, measured with
  `-fsyntax-only` while this plan was written).
- [ ] **Step 4: Document** in AGENTS.md: chapter 1, the prerequisites and `.local/` list
  (`.local/boost-cobalt`, `-sBOOST_COBALT_SOURCE`); chapter 2, item 4: "Only Boost's
  header-only libraries are available, and Boost.Cobalt, which `/webcpp//boost_cobalt`
  compiles from its sources for webcpp::asio"; chapter 9, the Boost action's paragraph;
  chapter 13, the compiled Boost libraries item names Cobalt as the first.
- [ ] **Step 5: Commit** (superproject): `"Jamroot: Boost.Cobalt's sources compiled, but
  main.cpp, which webcpp::asio replaces"` with `Jamroot tools/ci/actions/boost tools/ci/actions_test.py tools/test/jamroot_test.py AGENTS.md`.

## Task 16: Boost.Cobalt's `co_main` with the turn limit

**Files:**
- Create: `src/cobalt_main.cpp`, `test/cobalt_test.cpp`, `test/cobalt_status_test.cpp`,
  `example/asio_cobalt.cpp`, `example/asio_cobalt.expected`, `doc/cobalt.adoc`.
- Modify: `build.jam` (`cobalt-main`, `cobalt`), `test/Jamfile`, `example/Jamfile`,
  `doc/asio.adoc`.

**Interfaces:**
- Consumes: `/webcpp//boost_cobalt` (Task 15); `webcpp::asio::io_context` (Tasks 4 to 8).
- Produces: `int boost::cobalt::detail::main_promise::run_main(::boost::cobalt::main mn)`,
  defined by webcpp; `/webcpp/asio//cobalt`.

- [x] **Step 1: Write the page's section** `doc/cobalt.adoc` (`[#asio-cobalt]`): spec
  section 5.2 whole: a Cobalt program changes nothing and links `/webcpp/asio//cobalt`; why
  not a `co_main` of webcpp's own; the members it reads and the probe; the fallback; and
  that under Asyncify webcpp's `run_main` ends the program with `std::exit` so its
  `co_return` value is its exit status; and that `/webcpp//boost_cobalt` leaves out Cobalt's
  `io/ssl.cpp`, which needs OpenSSL, so a program that uses `boost::cobalt::io::ssl_stream`
  does not link with it (a difference from spec section 5.2's "every source but `main.cpp`",
  under "Departures from the spec").
- [x] **Step 2: Write the failing tests.** `test/cobalt_test.cpp`: an unchanged `co_main` of
  5,000 `co_await boost::asio::post(exec, cobalt::use_op)`, a 50 ms
  `boost::cobalt::io::steady_timer` wait and a 20 ms `boost::asio::steady_timer` wait with
  `cobalt::use_op`, a `webcpp::test::page_timeout` (`"page_timeout.hpp"`, Task 6) armed first
  whose `ran()` each post's continuation reads, recording how many
  posts had completed (as `turn_limit_test.cpp` does), then
  `co_return webcpp::test::exit_status(boost::report_errors());` after checking that the
  `setTimeout` ran within 1,000 posts on emscripten (natively the check is skipped: no page
  waits).
  `test/cobalt_status_test.cpp`: a `co_main` that waits 20 ms on a timer and `co_return 7;`,
  without `exit_status`: it tests `run_main`'s own way out. In `test/Jamfile`:

```jam
# Cobalt's co_main, unchanged, on webcpp's run_main: the turn limit, Cobalt's
# timers and Asio's, natively and on both variants. Its co_return is the exit
# status, under Asyncify too, where run_main exits explicitly.
for local test in cobalt cobalt_status
{
    local runner = run ;
    if $(test) = cobalt_status { runner = run-fail ; }
    webcpp.$(runner) $(test)_native : $(test)_test.cpp : <library>/webcpp/asio//cobalt : native ;
    webcpp.$(runner) $(test) : $(test)_test.cpp : <library>/webcpp/asio//cobalt : emscripten ;
    webcpp.$(runner) $(test)_asyncify : $(test)_test.cpp
      : <library>/webcpp/asio//cobalt <webcpp-suspend>asyncify : emscripten ;
    .asyncify += $(test)_asyncify ;
}
```

  Run the emscripten lane. Expected: FAIL, `/webcpp/asio//cobalt` not found.
- [x] **Step 3: Implement** `src/cobalt_main.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// webcpp's definition of Boost.Cobalt's main_promise::run_main and final_suspend, in place of
// Cobalt's src/main.cpp (Boost 1.92, BS/libs/cobalt/src/main.cpp:26-63): the same steps, with a
// webcpp::asio::io_context and its run family in place of ctx.run(), so that a co_main program
// gets the turn limit on emscripten and behaves as Cobalt's natively. It reads main_promise's
// members, Boost's implementation (B/cobalt/detail/main.hpp:36,145-148): compiling this file
// against the pinned Boost on every lane is the probe that fails when Boost changes them.

#include <webcpp/asio/io_context.hpp>

#include <boost/asio/post.hpp>        // lint-world: the post that resumes co_main
#include <boost/asio/signal_set.hpp>  // lint-world: Cobalt's signal set, which also makes the reactor
#include <boost/cobalt/main.hpp>

#include <csignal>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace boost::cobalt::detail {

auto main_promise::final_suspend() noexcept -> std::suspend_never {
    system::error_code ec;
    if (signal_set != nullptr) {
        signal_set->cancel(ec);
    }
    return std::suspend_never();
}

int main_promise::run_main(::boost::cobalt::main mn) {
    int res = -1;
    {
        webcpp::asio::io_context ctx{BOOST_ASIO_CONCURRENCY_HINT_1};
        boost::cobalt::this_thread::set_executor(ctx.get_executor());
        mn.promise->result = &res;
        mn.promise->exec.emplace(ctx.get_executor());
        mn.promise->exec_ = mn.promise->exec->get_executor();
        auto p = std::coroutine_handle<detail::main_promise>::from_promise(*mn.promise);
        asio::basic_signal_set<executor_type> ss{ctx, SIGINT, SIGTERM};
        mn.promise->signal_set = &ss;
        struct work {
            asio::basic_signal_set<executor_type>& ss;
            asio::cancellation_signal& signal;
            void operator()(system::error_code ec, int sig) const {
                if (sig == SIGINT) {
                    signal.emit(asio::cancellation_type::total);
                }
                if (sig == SIGTERM) {
                    signal.emit(asio::cancellation_type::terminal);
                }
                if (!ec) {
                    ss.async_wait(*this);
                }
            }
        };
        ss.async_wait(work{ss, mn.promise->signal});
        asio::post(ctx.get_executor(), [p] { p.resume(); });
        ctx.run();  // lint-run: Cobalt's event loop, the program's
    }
#ifdef __EMSCRIPTEN__
    // Under Asyncify a main that suspended loses the value it returns (measured,
    // asio-plan/exitcode): exit says it. JSPI keeps it (emscripten_has_asyncify() is 2).
    if (emscripten_has_asyncify() == 1) {
        std::exit(res);
    }
#endif
    return res;
}

}  // namespace boost::cobalt::detail
```

  `build.jam`:

```jam
# A Cobalt program's target: Boost.Cobalt's sources (/webcpp//boost_cobalt)
# and webcpp's definition of its run_main (src/cobalt_main.cpp), which runs
# co_main on asio's io_context and so gives it the turn limit.
# tag::cobalt[]
lib cobalt-main : src/cobalt_main.cpp
  : <link>static <library>asio <library>/webcpp//boost_cobalt ;
explicit cobalt-main ;
alias cobalt : : : : <library>asio <library>cobalt-main <library>/webcpp//boost_cobalt ;
# end::cobalt[]
```

- [x] **Step 4: Run** every lane. Expected: PASS: `cobalt` natively and on both variants;
  `cobalt_status` fails as a `run-fail` on all three, and its `.output` says
  `EXIT STATUS: 7` on each, the asyncify one included. Then run `cobalt_status_asyncify`
  with the `std::exit` removed from a scratch copy. Expected: `EXIT STATUS: 1` and the test
  support's line `webcpp: main did not finish`: the 7 was lost, and only the explicit exit
  keeps it. Restore.
- [x] **Step 5: Measure Asyncify's stack again** with the Cobalt tests in the lane, as Task
  9's Step 3 does; raise the value if S grew, and record it.
- [x] **Step 6: Write the twin example** `asio_cobalt.cpp` (a `co_main` that sleeps 50 ms on
  a `steady_timer` with `cobalt::use_op`, prints `cobalt slept` and `co_return 0;`) with its
  `.expected`, in `example/Jamfile` as
  `webcpp.example asio_cobalt.cpp : <library>/webcpp/asio//cobalt ;`, a
  `webcpp.variants page-asio_cobalt : asio_cobalt.cpp : <library>/webcpp/asio//cobalt ;` and
  its name in the size table. Include it in `cobalt.adoc`. Run both lanes and the doc build.
  Expected: PASS, the same output natively and in the browser.
- [x] **Step 7: Commit** (library): `"asio: Boost.Cobalt's co_main, unchanged, with the turn
  limit"`.

## Task 17: Promises as operations

**Files:**
- Create: `include/webcpp/asio/browser/error.hpp`, `include/webcpp/asio/browser/async_await.hpp`,
  `test/promise_test.cpp`, `test/promise_cobalt_test.cpp`, `example/asio_promise.cpp`,
  `example/asio_promise.expected`, `doc/promises.adoc`.
- Modify: `include/webcpp/asio.hpp`, `test/Jamfile`, `test/allocation_test.cpp`,
  `example/Jamfile`, `doc/asio.adoc`, `doc/guarantees.adoc` (10 and 11).

**Interfaces:**
- Consumes: the bridge and its `registration` (Task 7); `webcpp_asio_await`,
  `webcpp_asio_string` (Task 6).
- Produces:

```cpp
namespace webcpp::asio::browser::error {
enum class errors : int { promise_rejected = 1 };
const boost::system::error_category& category() noexcept;   // name(): "webcpp.asio"
boost::system::error_code make_error_code(errors e) noexcept;
inline constexpr errors promise_rejected = errors::promise_rejected;
}
// with boost::system::is_error_code_enum<webcpp::asio::browser::error::errors> true
namespace webcpp::asio::browser {
template <class CompletionToken>
auto async_await(const boost::asio::any_io_executor& executor, emscripten::val promise,
                 CompletionToken&& token);
// completion signature: void(boost::system::error_code, emscripten::val)
}
```

- [ ] **Step 1: Write the page's section** `doc/promises.adoc` (`[#asio-promises]`): spec
  section 4.7 for promises: every token works; the error; cancellation; lifetime;
  allocation; embind. Guarantees: `[[asio-invariant-10]]` "A promise completes its operation
  through the bridge with its value, or with `promise_rejected` and its reason; a cancellation
  completes it at once with `operation_aborted`, and a later settlement finds nothing.";
  `[[asio-invariant-11]]` "A pending operation is outstanding work; destroying the io_context
  destroys its handler without calling it."
- [ ] **Step 2: Write the failing test** `test/promise_test.cpp` (cases, each a function;
  `browser_values.hpp` gives the promises):
  - `a_resolved_promise_completes_with_its_value`: `test_promise_after(20, 42, false)`, a
    callback; `ec` is success, the value 42;
  - `a_rejected_promise_completes_with_promise_rejected`: `test_promise_after(20, 9, true)`;
    `ec == webcpp::asio::browser::error::promise_rejected`,
    `ec.category().name() == std::string_view("webcpp.asio")`, `ec.value() == 1`, the value 9;
  - `every_token_works`: the same promise through `boost::asio::deferred` (then invoked) and
    `boost::asio::use_awaitable` in a `co_spawn` (exceptions only); and a test of its own,
    `test/promise_cobalt_test.cpp`, a `co_main` that awaits a resolved promise with
    `co_await webcpp::asio::browser::async_await(exec, promise, boost::cobalt::use_op)`
    (Cobalt's `use_op` completes with an exception for an error, so it awaits a resolved one,
    and a rejected one inside `try`, with exceptions), declared as
    `webcpp.run promise_cobalt : promise_cobalt_test.cpp : <library>/webcpp/asio//cobalt : emscripten ;`
    with its `promise_cobalt_asyncify` twin in the lane;
  - `cancellation_completes_at_once` for each of `terminal`, `partial` and `total`: a
    promise that settles after 1 s, a `cancellation_signal` emitted from a 20 ms timer's
    handler; the operation completes with `operation_aborted` and `run` returns before
    500 ms; the settlement, a second later, finds nothing, and keeps node alive no longer
    than that (a JavaScript timer outlives the case and `exit`, `R/exit_pending_timer`);
  - `a_promise_that_never_settles_keeps_run`: `new Promise(() => {})` (an `EM_JS` helper
    `test_never_settles` in `browser_values.hpp`) awaited, and `run_for(200ms)` returns 0 with
    the context not stopped (outstanding work); then cancelled, and `run` returns;
  - `destroying_the_context_destroys_the_handler`: a handler holding a `std::shared_ptr` whose
    use count is checked: after the io_context's destruction with the promise pending, the
    count is back to 1 and the handler was not called;
  - Review Focus 4, `a_settlement_after_destruction_finds_nothing`: the promise of
    `test_promise_after(30, 1, false)` awaited, the io_context destroyed at once, then another
    io_context runs a 100 ms timer: nothing crashes, nothing is called;
  - Review Focus 5, `values_at_the_boundary`: an already resolved promise
    (`emscripten::val::global("Promise").call<emscripten::val>("resolve", 5)`) completes
    through a post, never inline (a flag set after `async_await` returns is seen set by the
    handler); a value that is no promise (`emscripten::val(6)`) completes with 6; a rejection
    whose reason is `undefined` completes with `promise_rejected` and `isUndefined()`.
  `promise` joins `browser-tests`; `allocation_test.cpp` gains, on emscripten, a resolved and
  a rejected promise awaited with handlers bound to `counted<void>`, still with no
  `operator new`.
- [ ] **Step 3: Run** the emscripten lane. Expected: FAIL, `async_await.hpp` not found.
- [ ] **Step 4: Implement** `browser/error.hpp` (a category whose `name()` is `"webcpp.asio"`
  and whose `message(1)` is `"the promise was rejected"`, any other value
  `"unknown webcpp.asio error"`; `make_error_code`; `is_error_code_enum`), and
  `browser/async_await.hpp`:

```cpp
namespace webcpp::asio::browser {
namespace detail {

/** One await: a registration of the bridge, which its settlement or a cancellation completes. */
template <class Handler, class Executor>
class await_operation final : public registration {
public:
    using allocator_type = typename std::allocator_traits<
        boost::asio::associated_allocator_t<Handler>>::template rebind_alloc<await_operation>;

    await_operation(bridge& b, Executor executor, Handler handler)
        : bridge_(b), work_(boost::asio::prefer(executor, boost::asio::execution::outstanding_work.tracked)),
          handler_(std::move(handler)) {}

    void start(const emscripten::val& promise) {
        id_ = bridge_.add(*this);
        auto slot = boost::asio::get_associated_cancellation_slot(handler_);
        if (slot.is_connected()) {
            slot.assign([this](boost::asio::cancellation_type) { cancel(); });
        }
        webcpp_asio_await(promise.as_handle(), bridge_.function(id_).as_handle());
    }

    bool deliver(const emscripten::val& envelope) override {
        const bool fulfilled = envelope["ok"].as<bool>();
        complete(fulfilled ? boost::system::error_code() : make_error_code(error::promise_rejected),
                 envelope["value"]);
        return true;
    }

    void shutdown() noexcept override {
        boost::asio::get_associated_cancellation_slot(handler_).clear();
        destroy(allocator_type(boost::asio::get_associated_allocator(handler_)));
    }

private:
    void cancel() {
        bridge_.remove(id_);
        complete(boost::asio::error::operation_aborted, emscripten::val::undefined());
    }

    /** Frees this operation, then posts the handler with its result, before the handler runs. */
    void complete(boost::system::error_code ec, emscripten::val value) {
        boost::asio::get_associated_cancellation_slot(handler_).clear();
        allocator_type allocator(boost::asio::get_associated_allocator(handler_));
        auto handler = std::move(handler_);
        auto executor = work_;
        destroy(allocator);
        boost::asio::post(executor, boost::asio::append(std::move(handler), ec, std::move(value)));
    }

    /** Ends and frees this operation with the allocator taken from its handler. */
    void destroy(allocator_type allocator) noexcept {
        std::allocator_traits<allocator_type>::destroy(allocator, this);
        std::allocator_traits<allocator_type>::deallocate(allocator, this, 1);
    }

    bridge& bridge_;
    boost::asio::any_io_executor work_;
    Handler handler_;
    std::uint64_t id_ = 0;
};

/** The initiation of async_await, which async_initiate calls with the handler. */
struct initiate_await {
    template <class Handler>
    void operator()(Handler&& handler, const boost::asio::any_io_executor& executor,
                    const emscripten::val& promise) const {
        using operation = await_operation<std::decay_t<Handler>, boost::asio::any_io_executor>;
        auto& context = boost::asio::query(executor, boost::asio::execution::context);
        auto& b = boost::asio::use_service<bridge>(context);
        typename operation::allocator_type allocator(boost::asio::get_associated_allocator(handler));
        operation* op = std::allocator_traits<typename operation::allocator_type>::allocate(allocator, 1);
        std::allocator_traits<typename operation::allocator_type>::construct(
            allocator, op, b, executor, std::forward<Handler>(handler));
        op->start(promise);
    }
};

}  // namespace detail

template <class CompletionToken>
auto async_await(const boost::asio::any_io_executor& executor, emscripten::val promise,
                 CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, emscripten::val)>(
        detail::initiate_await{}, token, executor, std::move(promise));
}

}  // namespace webcpp::asio::browser
```

  (The allocator is taken from the handler before the handler moves and the operation ends;
  the completion's own allocation, the post, is the handler's.) Each member gets its Doc
  Comment; every `boost/asio/` include says why after `lint-world:`.
- [ ] **Step 5: Run** every lane, then the lint. Expected: PASS, `lint: clean`.
- [ ] **Step 6: Write the example** `asio_promise.cpp` (emscripten only:
  `webcpp.example asio_promise.cpp : : emscripten ;`): awaits a resolved and a rejected
  promise with callbacks, prints `resolved: 42` and `rejected: promise_rejected 9`; its
  `.expected`; include it in `promises.adoc`.
  `example/Jamfile` also gains `webcpp.variants page-asio_promise : asio_promise.cpp ;` and the
  name `page-asio_promise` in `webcpp.size-table asio` (spec sections 4.4 and 9: every
  example, both variants); the asyncify lane then prints one more row.
  Run the emscripten lane, the asyncify lane and the doc build. Expected: PASS.
- [ ] **Step 7: Commit** (library): `"asio: a JavaScript promise as an Asio operation"`.

## Task 18: Events as operations

**Files:**
- Create: `include/webcpp/asio/browser/async_wait_event.hpp`, `test/event_test.cpp`,
  `example/asio_event.cpp`, `example/asio_event.expected`.
- Modify: `include/webcpp/asio.hpp`, `test/Jamfile`, `test/allocation_test.cpp`,
  `example/Jamfile`, `doc/promises.adoc` (renamed in the page "Promises and events").

**Interfaces:**
- Consumes: the bridge (Task 7), `webcpp_asio_string` (Task 6).
- Produces:

```cpp
namespace webcpp::asio::browser {
template <class CompletionToken>
auto async_wait_event(const boost::asio::any_io_executor& executor, emscripten::val target,
                      std::string_view type, CompletionToken&& token);
// completion signature: void(boost::system::error_code, emscripten::val)
}
```

- [ ] **Step 1: Write the failing test** `test/event_test.cpp`:
  - `an_event_completes_with_itself`: `test_event_target()`, `test_dispatch_after(target, 20,
    5)`, awaited for `"message"`; success, and `event["detail"].as<int>() == 5`;
  - `only_the_next_event`: two dispatches 20 ms apart, one wait; the second event is not
    delivered (the listener was `once`), checked by a second wait that receives it;
  - `cancellation_removes_the_listener` for `terminal`, `partial`, `total`: the wait
    completes with `operation_aborted`, and a later dispatch reaches no listener (an `EM_JS`
    helper `test_listener_count` in `browser_values.hpp`, which counts through an
    `addEventListener` wrapper installed on the target, reads 0 after the cancellation);
  - Review Focus 5, `a_type_given_as_a_view`: the type `std::string_view("messages and more",
    7)`; the event `"message"` completes it;
  - Review Focus 4, `a_wait_pending_at_destruction_removes_its_listener`: a wait on a target,
    then the `io_context` destroyed with the wait pending: the handler is destroyed without
    being called, `test_listener_count` reads 0, and a dispatch afterwards reaches nothing.
  (`test_listener_count(target)` installs, on its first call for a target, wrappers of that
  target's own `addEventListener` and `removeEventListener` that keep a count, and returns it.)
  `event` joins `browser-tests`; `allocation_test.cpp` gains an event awaited with a handler
  bound to `counted<void>`, still with no `operator new`.
- [ ] **Step 2: Run.** Expected: FAIL, `async_wait_event.hpp` not found.
- [ ] **Step 3: Implement** `async_wait_event.hpp` in the shape of `async_await.hpp`: its
  operation keeps `target`, the type as a JavaScript string made by `webcpp_asio_string(type.data(),
  type.size())` (no `std::string`: the view needs no null and nothing is allocated), and the
  bridge's function; `start` calls
  `target.call<void>("addEventListener", type, function, options)` with
  `options` an object whose `once` is true; `deliver` completes with success and the event;
  `cancel` calls `removeEventListener(type, function)`, removes the id and completes with
  `operation_aborted`; `shutdown` (the io_context's destruction) clears the slot, calls
  `removeEventListener(type, function)` too, and destroys the handler without calling it.
- [ ] **Step 4: Run** every lane and the lint. Expected: PASS.
- [ ] **Step 5: Write the example** `asio_event.cpp` (emscripten): waits for a `message` on an
  `EventTarget` that a `setTimeout` dispatches, prints `event: message 5`; its `.expected`;
  in the page.
  `example/Jamfile` also gains `webcpp.variants page-asio_event : asio_event.cpp ;` and the
  name `page-asio_event` in `webcpp.size-table asio` (spec sections 4.4 and 9: every
  example, both variants); the asyncify lane then prints one more row.
  Run the emscripten lane, the asyncify lane and the doc build. Expected: PASS.
- [ ] **Step 6: Commit** (library): `"asio: a browser event as an Asio operation"`.

## Task 19: Sockets and the transport point

**Files:**
- Create: `include/webcpp/asio/ip/tcp.hpp`, `include/webcpp/asio/ip/udp.hpp`,
  `include/webcpp/asio/basic_stream_socket.hpp`, `include/webcpp/asio/basic_datagram_socket.hpp`,
  `include/webcpp/asio/basic_socket_acceptor.hpp`, `include/webcpp/asio/browser/transport.hpp`,
  `test/sockets_test.cpp`, `test/transport_test.cpp`, `example/asio_sockets.cpp`,
  `example/asio_sockets.expected`, `doc/sockets.adoc`.
- Modify: `include/webcpp/asio.hpp`, `test/Jamfile`, `test/names_test.cpp`, `example/Jamfile`,
  `doc/asio.adoc`, `doc/guarantees.adoc` (12).

**Interfaces:**
- Produces, natively: `webcpp::asio::ip::tcp` and `udp`, aliases of Boost's; and
  `template <class Protocol, class Executor = boost::asio::any_io_executor> using
  basic_stream_socket = boost::asio::basic_stream_socket<Protocol, Executor>;` and the same for
  `basic_datagram_socket` and `basic_socket_acceptor`. On emscripten:

```cpp
namespace webcpp::asio::ip {
class tcp {
public:
    using endpoint = boost::asio::ip::basic_endpoint<tcp>;
    using socket = basic_stream_socket<tcp>;
    using acceptor = basic_socket_acceptor<tcp>;
    using resolver = basic_resolver<tcp>;  // Task 20
    static tcp v4() noexcept;
    static tcp v6() noexcept;
    int type() const noexcept;     // SOCK_STREAM
    int protocol() const noexcept; // IPPROTO_TCP
    int family() const noexcept;   // AF_INET or AF_INET6
    friend bool operator==(const tcp&, const tcp&) noexcept;
};
class udp;  // the same, SOCK_DGRAM and IPPROTO_UDP, socket = basic_datagram_socket<udp>
}
namespace webcpp::asio::browser {
template <class Protocol>
class transport;  // primary: every operation fails with operation_not_supported
}
```

  The socket templates hold `browser::transport<Protocol> transport_`, `Executor`, the
  protocol, and whether they are open. Their members, with Boost's signatures (spec section 7's
  table): constructors from an executor or an `ExecutionContext&`, alone or with a protocol
  (which opens) or an endpoint (which opens and binds; behind `#ifndef
  WEBCPP_ASIO_NO_EXCEPTIONS`, since a constructor reports by throwing); `get_executor`; `open`
  (an already open socket: `boost::asio::error::already_open`), `is_open`, `close`, which
  succeed; and through the transport: `cancel`, `shutdown`, `connect`, `async_connect`,
  `read_some`, `async_read_some`, `write_some`, `async_write_some`, `send`, `async_send`,
  `receive`, `async_receive` (with and without `message_flags`), `set_option`, `get_option`,
  `local_endpoint`, `remote_endpoint`, `available`, `non_blocking` (both forms); the datagram
  socket has `bind`, `send_to`, `async_send_to`, `receive_from`, `async_receive_from` in place
  of the stream operations; the acceptor has `open`, `bind`, `listen`, `accept(peer)`,
  `accept()`, `async_accept(peer, token)`, `async_accept(token)`, `close`. Each synchronous
  operation has an `error_code&` overload that never throws and never blocks, and a throwing
  one behind `#ifndef WEBCPP_ASIO_NO_EXCEPTIONS`, raising through `boost::throw_exception`.
  Each asynchronous one goes through `boost::asio::async_initiate` and completes through a
  post.

- [ ] **Step 1: Write the page's section** `doc/sockets.adoc` (`[#asio-sockets]`): spec
  section 7: why webcpp's own templates on emscripten (SOCKFS opens a WebSocket, getaddrinfo
  invents 172.29.x.x addresses), the subset, the failures as values, the transport point and
  its one-definition rule, and that Beast's `tcp_stream` wraps Boost's socket, so a relay
  reaches Beast only as a named next layer. Guarantee `[[asio-invariant-12]]`: "On emscripten
  each socket and acceptor operation but `open` and `close` fails with
  `operation_not_supported` unless a transport provides it; a synchronous one never blocks, an
  asynchronous one completes through a post."
- [ ] **Step 2: Write the failing tests.** `test/sockets_test.cpp` (emscripten): for
  `ip::tcp::socket`, `ip::udp::socket` and `ip::tcp::acceptor`, every operation of the list
  above in its `error_code&` form gives `operation_not_supported` and returns at once; every
  asynchronous one completes with it through a post (a flag set after the call returns is seen
  set by the handler) under a callback and under `boost::asio::deferred`; `open` and `close`
  succeed, `is_open` follows them, a second `open` gives `already_open`; with exceptions, each
  throwing form raises a `boost::system::system_error` whose code is
  `operation_not_supported`. `test/transport_test.cpp` (emscripten): a protocol of the test's
  own, `test_protocol` (its `endpoint` Boost's `basic_endpoint<test_protocol>`), and a
  specialisation `webcpp::asio::browser::transport<test_protocol>` whose `connect` succeeds and
  records the endpoint; `basic_stream_socket<test_protocol>::connect` then succeeds, and its
  `async_connect` completes with success: the point is used, and the program's code is the
  same. `names_test.cpp` gains, natively, `static_assert`s that `webcpp::asio::ip::tcp` is
  `boost::asio::ip::tcp` and `webcpp::asio::ip::tcp::socket` is Boost's socket. `sockets` and
  `transport` join `browser-tests`; `allocation_test.cpp` gains an `async_connect` bound to
  `counted<void>`, with no `operator new`.
- [ ] **Step 3: Run.** Expected: FAIL, `ip/tcp.hpp` not found.
- [ ] **Step 4: Implement** the headers. The transport's primary:

```cpp
namespace webcpp::asio::browser {

/**
 How the sockets of Protocol reach the network in the browser, which gives them none: every
 operation fails with operation_not_supported.

 A later library (relays that carry TCP or UDP over a WebSocket or a WebRTC channel)
 specialises it for a protocol; the specialisation is visible in every translation unit that
 uses the socket type, before its first use.

 @tparam Protocol The protocol: ip::tcp or ip::udp.
*/
template <class Protocol>
class transport {
public:
    using endpoint_type = typename Protocol::endpoint;

    explicit transport(const boost::asio::any_io_executor& executor) : executor_(executor) {}

    boost::system::error_code connect(const endpoint_type&) { return not_supported(); }

    template <class Handler>
    void async_connect(const endpoint_type&, Handler&& handler) {
        fail(std::forward<Handler>(handler));
    }

    template <class MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence&, boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class MutableBufferSequence, class Handler>
    void async_read_some(const MutableBufferSequence&, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    template <class ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence&, boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class ConstBufferSequence, class Handler>
    void async_write_some(const ConstBufferSequence&, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    template <class ConstBufferSequence>
    std::size_t send(const ConstBufferSequence&, boost::asio::socket_base::message_flags,
                     boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class ConstBufferSequence, class Handler>
    void async_send(const ConstBufferSequence&, boost::asio::socket_base::message_flags,
                    Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    template <class MutableBufferSequence>
    std::size_t receive(const MutableBufferSequence&, boost::asio::socket_base::message_flags,
                        boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class MutableBufferSequence, class Handler>
    void async_receive(const MutableBufferSequence&, boost::asio::socket_base::message_flags,
                       Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    template <class ConstBufferSequence>
    std::size_t send_to(const ConstBufferSequence&, const endpoint_type&,
                        boost::asio::socket_base::message_flags, boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class ConstBufferSequence, class Handler>
    void async_send_to(const ConstBufferSequence&, const endpoint_type&,
                       boost::asio::socket_base::message_flags, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    template <class MutableBufferSequence>
    std::size_t receive_from(const MutableBufferSequence&, endpoint_type&,
                             boost::asio::socket_base::message_flags,
                             boost::system::error_code& ec) {
        ec = not_supported();
        return 0;
    }

    template <class MutableBufferSequence, class Handler>
    void async_receive_from(const MutableBufferSequence&, endpoint_type&,
                            boost::asio::socket_base::message_flags, Handler&& handler) {
        fail(std::forward<Handler>(handler), std::size_t{0});
    }

    boost::system::error_code bind(const endpoint_type&) { return not_supported(); }

    boost::system::error_code listen(int /*backlog*/) { return not_supported(); }

    template <class Socket>
    boost::system::error_code accept(Socket& /*peer*/) {
        return not_supported();
    }

    template <class Socket, class Handler>
    void async_accept(Socket& /*peer*/, Handler&& handler) {
        fail(std::forward<Handler>(handler));
    }

    boost::system::error_code shutdown(boost::asio::socket_base::shutdown_type) {
        return not_supported();
    }

    boost::system::error_code cancel() { return not_supported(); }

    template <class SettableSocketOption>
    boost::system::error_code set_option(const SettableSocketOption&) {
        return not_supported();
    }

    template <class GettableSocketOption>
    boost::system::error_code get_option(GettableSocketOption&) const {
        return not_supported();
    }

    endpoint_type local_endpoint(boost::system::error_code& ec) const {
        ec = not_supported();
        return endpoint_type();
    }

    endpoint_type remote_endpoint(boost::system::error_code& ec) const {
        ec = not_supported();
        return endpoint_type();
    }

    std::size_t available(boost::system::error_code& ec) const {
        ec = not_supported();
        return 0;
    }

    boost::system::error_code non_blocking(bool /*mode*/) { return not_supported(); }

    [[nodiscard]] bool non_blocking() const noexcept { return false; }

private:
    static boost::system::error_code not_supported() noexcept {
        return boost::asio::error::operation_not_supported;
    }

    template <class Handler, class... Values>
    void fail(Handler&& handler, Values... values) {
        boost::asio::post(executor_, boost::asio::append(std::forward<Handler>(handler),
                                                         not_supported(), std::move(values)...));
    }

    boost::asio::any_io_executor executor_;
};

}  // namespace webcpp::asio::browser
```

  (each member documented in the header). The acceptor's `async_accept(token)`, whose
  completion is `void(boost::system::error_code, socket)`, makes a socket on the acceptor's
  executor and gives it to `transport_.async_accept(peer, handler)` through a handler that
  completes with the error and the socket. A socket's operation is a guard and a forward, for
  one:

```cpp
    template <class ConnectToken>
    auto async_connect(const endpoint_type& peer, ConnectToken&& token) {
        return boost::asio::async_initiate<ConnectToken, void(boost::system::error_code)>(
            [this](auto handler, const endpoint_type& to) {
                transport_.async_connect(to, std::move(handler));
            },
            token, peer);
    }

    boost::system::error_code connect(const endpoint_type& peer, boost::system::error_code& ec) {
        ec = transport_.connect(peer);
        return ec;
    }

#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    void connect(const endpoint_type& peer) {
        boost::system::error_code ec;
        if (connect(peer, ec)) {
            boost::throw_exception(boost::system::system_error(ec, "connect"));
        }
    }
#endif
```

  The `ip/` headers branch on `__EMSCRIPTEN__`: natively the aliases; on emscripten the
  protocol classes with `BOOST_ASIO_OS_DEF(AF_INET)`, `BOOST_ASIO_OS_DEF(SOCK_STREAM)` and their
  kin from `<boost/asio/detail/socket_types.hpp>` (each `lint-world:`).
  `headers-alone`'s first call takes `asio/ip/*.hpp` with the rest; the emscripten-only
  `browser/transport.hpp` is in the second call's glob.
- [ ] **Step 5: Run** every lane and the lint. Expected: PASS.
- [ ] **Step 6: Write the example** `asio_sockets.cpp` (emscripten): a `tcp::socket` opened
  and connected to `127.0.0.1:80`; prints `open: true`,
  `connect: operation_not_supported` (comparing the code, never its message), `close: ok`;
  its `.expected`; in `sockets.adoc`.
  `example/Jamfile` also gains `webcpp.variants page-asio_sockets : asio_sockets.cpp ;` and the
  name `page-asio_sockets` in `webcpp.size-table asio` (spec sections 4.4 and 9: every
  example, both variants); the asyncify lane then prints one more row.
  Run the emscripten lane, the asyncify lane and the doc build. Expected: PASS.
- [ ] **Step 7: Commit** (library): `"asio: sockets that fail as values, behind the transport
  point"`.

## Task 20: The resolver (option A)

**Files:**
- Create: `include/webcpp/asio/ip/basic_resolver.hpp`, `test/resolver_test.cpp`,
  `example/asio_resolver.cpp`, `example/asio_resolver.expected`.
- Modify: `include/webcpp/asio/ip/tcp.hpp`, `include/webcpp/asio/ip/udp.hpp`,
  `include/webcpp/asio.hpp`, `test/Jamfile`, `test/names_test.cpp`, `example/Jamfile`,
  `doc/sockets.adoc`, `doc/guarantees.adoc` (13), `doc/allocation.adoc`.

**Interfaces:**
- Produces, natively, `template <class InternetProtocol, class Executor =
  boost::asio::any_io_executor> using basic_resolver = boost::asio::ip::basic_resolver<InternetProtocol, Executor>;`.
  On emscripten:

```cpp
namespace webcpp::asio::ip {
template <class InternetProtocol, class Executor = boost::asio::any_io_executor>
class basic_resolver : public boost::asio::ip::resolver_base {
public:
    using executor_type = Executor;
    using protocol_type = InternetProtocol;
    using endpoint_type = typename InternetProtocol::endpoint;
    using results_type = boost::asio::ip::basic_resolver_results<InternetProtocol>;
    explicit basic_resolver(const executor_type& executor);
    template <class ExecutionContext> explicit basic_resolver(ExecutionContext& context);
    executor_type get_executor() noexcept;
    void cancel();
    results_type resolve(std::string_view host, std::string_view service, boost::system::error_code& ec);
    results_type resolve(std::string_view host, std::string_view service, flags resolve_flags,
                         boost::system::error_code& ec);
    results_type resolve(const endpoint_type& e, boost::system::error_code& ec);
    // the throwing forms of the three, behind #ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    template <class ResolveToken> auto async_resolve(std::string_view host, std::string_view service, ResolveToken&& token);
    template <class ResolveToken> auto async_resolve(std::string_view host, std::string_view service, flags resolve_flags, ResolveToken&& token);
    template <class ResolveToken> auto async_resolve(const endpoint_type& e, ResolveToken&& token);
    // completion signature: void(boost::system::error_code, results_type)
};
}
```

  `resolve(host, service)` makes one entry with
  `results_type::create(endpoint_type(boost::asio::ip::address_v4::any(), port), std::string(host), std::string(service))`,
  where the port is 0 for an empty service (Boost's own resolver passes `""` to
  `getaddrinfo` as no service, whose entries carry port 0), the service's number when it is
  all digits and at most 65535 (`"0"` included), else 80 for `"http"` and `"ws"` and 443 for
  `"https"` and `"wss"` unless the flags hold `numeric_service`; any other service fails with
  `boost::asio::error::service_not_found`. Reverse resolution fails with
  `operation_not_supported`. `cancel` does nothing: every resolution has completed when it
  starts (as an operation Boost had completed).

- [ ] **Step 1: Write the page's part** in `doc/sockets.adoc`: spec section 7, "The resolver"
  (decision 5): the entry, the port of each service, why not a failing resolver (every Beast
  client would change a line), reverse resolution. Guarantee `[[asio-invariant-13]]`: "On
  emscripten, `resolve(host, service)` completes with one entry whose `host_name()` and
  `service_name()` are the arguments and whose endpoint is the unspecified address and the
  service's port; a plain socket's `connect` on it fails with `operation_not_supported`."
- [ ] **Step 2: Write the failing test** `test/resolver_test.cpp` (emscripten):
  - `resolve_makes_one_entry`: `tcp::resolver r(io); r.resolve("example.com", "https", ec)`:
    no error, one entry, `host_name() == "example.com"`, `service_name() == "https"`,
    `endpoint().address().is_unspecified()`, port 443;
  - Review Focus 5, the services as a table:

```cpp
struct expected_port {
    std::string_view service;
    bool found;
    std::uint16_t port;
};
constexpr std::array<expected_port, 11> services{{
    {"80", true, 80},     {"0", true, 0},       {"65535", true, 65535}, {"65536", false, 0},
    {"http", true, 80},   {"ws", true, 80},     {"https", true, 443},   {"wss", true, 443},
    {"WS", false, 0},     {"80x", false, 0},    {"", true, 0},
}};
```

    each gives the port, or `boost::asio::error::service_not_found` and no entry;
    with `resolver::numeric_service`, `"https"` gives `service_not_found` and `"443"` gives
    443;
  - `async_resolve_completes_through_a_post` under a callback and `deferred`, with the same
    entry;
  - `reverse_resolution_fails`: `resolve(endpoint, ec)` and `async_resolve(endpoint, ...)`
    give `operation_not_supported`;
  - `a_plain_socket_cannot_connect_to_the_entry`: `tcp::socket s(io); s.connect(*entry, ec)`
    gives `operation_not_supported` (decision 2 holds);
  - with exceptions, `resolve("h", "nope")` throws a `system_error` of `service_not_found`.
  `names_test.cpp` gains, natively, `static_assert` that
  `webcpp::asio::ip::tcp::resolver` is `boost::asio::ip::tcp::resolver`. `resolver` joins
  `browser-tests`.
- [ ] **Step 3: Run.** Expected: FAIL, `ip/basic_resolver.hpp` not found.
- [ ] **Step 4: Implement** the header; `tcp::resolver` and `udp::resolver` become
  `basic_resolver<tcp>` and `basic_resolver<udp>`. The port is parsed with `std::from_chars`
  into a `std::uint32_t` after the all-digits check, a guard at the top of a function of its
  own, `std::optional<std::uint16_t> port_of(std::string_view service, flags f) noexcept`.
- [ ] **Step 5: Run** every lane and the lint. Expected: PASS. `allocation.adoc` names the
  resolver's results as the third of the runtime's allocations (`new`, in Boost's
  `basic_resolver_results::create`).
- [ ] **Step 6: Write the example** `asio_resolver.cpp` (emscripten): resolves
  `("example.com", "wss")` asynchronously and prints `example.com wss 443 unspecified: true`;
  its `.expected`; in `sockets.adoc`.
  `example/Jamfile` also gains `webcpp.variants page-asio_resolver : asio_resolver.cpp ;` and the
  name `page-asio_resolver` in `webcpp.size-table asio` (spec sections 4.4 and 9: every
  example, both variants); the asyncify lane then prints one more row.
  Run the emscripten lane, the asyncify lane and the doc build. Expected: PASS.
- [ ] **Step 7: Commit** (library): `"asio: the resolver of option A, whose entry the
  browser's streams accept"`.

## Task 21: The `ssl` names, and OpenSSL named by the program

**Files:**
- Create: `include/webcpp/asio/ssl.hpp`, `src/openssl_on_webassembly.hpp`,
  `test/ssl_test.cpp`, `test/ssl_without_openssl_test.cpp`,
  `test/openssl_on_webassembly_test.cpp`, `test/openssl_named_test.cpp`;
  `example/tls/Jamfile`, `example/tls/asio_tls.cpp`, `example/tls/asio_tls.expected`.
- Modify: `build.jam` (OpenSSL natively: the targets `openssl` and `openssl-tests`, and the
  refusal on emscripten), `test/Jamfile` (the tests, and a headers-alone call of its own),
  `doc/building.adoc` (the section "TLS: name OpenSSL for native builds alone") and
  `doc/sockets.adoc`.
- Modify (superproject): `tools/ci/matrix.py` (`EXTERNAL_DEPENDENCIES['asio'] = ('openssl',)`),
  `tools/ci/matrix_test.py`.

**Interfaces:**
- Follows the owner's decision of 2026-10-11, "an external library a program names and the
  machine lacks stops the build" (WEBCPP_DESIGN.md), and AGENTS.md chapter 6, "Bringing a C
  library into webcpp", items 5 to 7: **the program names OpenSSL in its own Jamfile**, and a
  program that names it on a machine that lacks it stops with an instruction; only webcpp's
  own tests and examples leave it out (and the CI, with `webcpp-require-external=on`, stops
  instead).
- Produces, natively:
  - `namespace webcpp::asio { namespace ssl = boost::asio::ssl; }`, in
    `<webcpp/asio/ssl.hpp>`, which the convenience header leaves out (it brings OpenSSL);
    without `WEBCPP_ASIO_HAS_OPENSSL` the header stops with:
    `webcpp::asio: <webcpp/asio/ssl.hpp> needs OpenSSL natively, and it is not linked. In the program's Jamfile, add /webcpp/asio//openssl beside /webcpp/asio//asio, as in: exe app : app.cpp /webcpp/asio//asio /webcpp/asio//openssl ; b2 links it when it finds OpenSSL 3 (-sOPENSSL_ROOT=<dir> names it). (without b2: define WEBCPP_ASIO_HAS_OPENSSL=1 and link libssl and libcrypto)`;
  - `/webcpp/asio//openssl`, what a program names: OpenSSL 3's `libssl` and `libcrypto`,
    found by `webcpp.external`, linked as the system's (shared), with
    `WEBCPP_ASIO_HAS_OPENSSL=1`; not found, the build stops (webcpp.jam's stop, which names
    asio, OpenSSL, every place it looked and `-sOPENSSL_ROOT=<dir>`);
  - **refused on WebAssembly** (AGENTS.md chapter 6, item 7; the owner, 2026-10-11, who
    rejected ignoring it there: a user would think OpenSSL is in the `.wasm`). Named in a
    build for emscripten, it gives every source of the program the forced include
    `src/openssl_on_webassembly.hpp` (`-include`, a usage requirement on emscripten), which
    stops the compile with the guideline's kind of message, naming exactly what to write:

    `webcpp::asio: /webcpp/asio//openssl is named in a build for WebAssembly, and OpenSSL is native only: there <webcpp/asio/ssl.hpp> is a record of the TLS settings and links no library, and nothing of OpenSSL would be in the .wasm. In the program's Jamfile, name OpenSSL for native builds alone, by a condition in the project's requirements: project : requirements <conditional>@native-openssl ; rule native-openssl ( properties * ) { if ! <toolset>emscripten in $(properties) { return <library>/webcpp/asio//openssl ; } } (without b2: link OpenSSL into the native program only)`

    A compile stop rather than b2's `errors.user-error`: b2's stop could not be pinned by a
    test in a lane, while this one is, twice (Step 1);
  - **the form a program built for both targets writes**, the condition in the project's
    requirements, which the message and the page give:

```jam
project : requirements <conditional>@native-openssl ;

rule native-openssl ( properties * )
{
    if ! <toolset>emscripten in $(properties)
    {
        return <library>/webcpp/asio//openssl ;
    }
}
```

    Chosen as the form b2 handles most simply, measured with b2 5.5.3 on native (Apple Clang)
    and emscripten (`.superpowers/sdd/plan-reviews/asio-plan/openssl-forms/`, the program
    printing whether `HAS_OPENSSL` was defined): this form links OpenSSL natively and
    nothing on emscripten. It sits where the owner wants the conditions, in the project; one
    declaration covers every program of the project; and it composes with any rule that
    declares a program, so webcpp's own example builds the very lines a user copies. Two
    alternatives of one target (`exe app : app.cpp /webcpp/asio//openssl ;` and
    `exe app : app.cpp : <toolset>emscripten ;`) also work, measured, but repeat every
    program. The negated condition `<toolset>!emscripten:<library>/webcpp/asio//openssl`,
    which looks simpler, **silently links nothing natively** (measured: the native program
    printed `no openssl`): the page warns against it;
  - `/webcpp/asio//openssl-tests`, the same for asio's own tests: left out with one line on a
    machine that lacks OpenSSL, unless the build gives `webcpp-require-external=on`.
  - The guideline's propagated shared-or-static feature comes with `/third_party//openssl`,
    which webcpp::crypto's plan builds (WEBCPP_CRYPTO_SPEC.md section 4.4); asio's target
    moves onto it then, which Task 22 records in AGENTS.md chapter 13.
- Produces, on emscripten, a record of the program's TLS settings, which only
  webcpp::beast's browser streams read (WEBCPP_BEAST_SPEC.md section 4.2):

```cpp
namespace webcpp::asio::ssl {
class context_base;    // Boost's enum method (every value, Boost's order), file_format, options
                       // constants, verify_mode and verify_none, verify_peer,
                       // verify_fail_if_no_peer_cert, verify_client_once (0x00, 0x01, 0x02, 0x04)
class stream_base;     // enum handshake_type { client, server }
class host_name_verification {
public:
    explicit host_name_verification(std::string_view host);
    std::string_view host() const noexcept;
};
enum class verify_callback_kind : std::uint8_t { none, host_name_verification, other };
class context : public context_base {
public:
    explicit context(method m);
    context(method m, std::pmr::memory_resource* resource);
    // recorded, each with an error_code& form and a throwing form:
    void set_verify_mode(verify_mode v);
    void set_default_verify_paths();
    template <class VerifyCallback> void set_verify_callback(VerifyCallback callback);
    void add_certificate_authority(const boost::asio::const_buffer& ca);
    void load_verify_file(std::string_view filename);
    void add_verify_path(std::string_view path);
    void use_certificate(const boost::asio::const_buffer& certificate, file_format format);
    void use_certificate_file(std::string_view filename, file_format format);
    void use_certificate_chain(const boost::asio::const_buffer& chain);
    void use_certificate_chain_file(std::string_view filename);
    void use_private_key(const boost::asio::const_buffer& key, file_format format);
    void use_private_key_file(std::string_view filename, file_format format);
    void set_options(options o);
    // what the browser streams read:
    method recorded_method() const noexcept;
    verify_mode recorded_verify_mode() const noexcept;
    bool default_verify_paths() const noexcept;
    verify_callback_kind recorded_callback() const noexcept;
    std::string_view verification_host() const noexcept;   // a host_name_verification's
    bool roots_added() const noexcept;                      // add_certificate_authority, load_verify_file, add_verify_path
    bool client_credentials() const noexcept;               // use_certificate*, use_private_key*
};
}
```

  Each setting is recorded and succeeds; whether the browser honours it is webcpp::beast's
  judgement. The verification host is held in a `std::pmr::string` of the given resource.

- [ ] **Step 1: Write the failing tests.** `test/ssl_test.cpp`: natively, `static_assert`s
  that `webcpp::asio::ssl::context` is `boost::asio::ssl::context` and
  `webcpp::asio::ssl::stream_base` Boost's, and a `context(tlsv12_client)` made; on
  emscripten: a context of `tlsv12_client` records its method; `set_verify_mode(verify_peer)`,
  `set_default_verify_paths()`, `set_verify_callback(host_name_verification("example.com"))`
  are recorded (`host_name_verification` kind, host `example.com`); a lambda callback is
  `other`; `add_certificate_authority` sets `roots_added`; `use_private_key` sets
  `client_credentials`; every `error_code&` form leaves `ec` clear; the host's storage comes
  from the given `std::pmr::memory_resource` (a counting resource sees one allocation, and the
  replaced `operator new` none). `test/ssl_without_openssl_test.cpp`: includes
  `<webcpp/asio/ssl.hpp>` with
  `// expected-error@webcpp/asio/ssl.hpp:* {{webcpp::asio: <webcpp/asio/ssl.hpp> needs OpenSSL natively, and it is not linked}}`.
  `test/Jamfile`:

```jam
# ssl.hpp natively needs OpenSSL, which a program names: asio's own tests
# name openssl-tests, which leaves them out on a machine that lacks it
# (unless webcpp-require-external=on, as the CI gives); and without OpenSSL
# named, the header stops with the instruction.
webcpp.run ssl : ssl_test.cpp : <library>/webcpp/asio//openssl-tests ;
webcpp.run ssl_asyncify : ssl_test.cpp
  : <library>/webcpp/asio//openssl-tests <webcpp-suspend>asyncify : emscripten ;
webcpp.compile-diagnostic ssl_without_openssl : ssl_without_openssl_test.cpp : : native ;
webcpp.headers-alone asio : ../include : asio/ssl.hpp
  : <library>/webcpp/asio//openssl-tests ;

# /webcpp/asio//openssl named for emscripten stops the compile with the
# message of src/openssl_on_webassembly.hpp: pinned word for word natively
# (compile-diagnostic is clang's -verify, which b2's emscripten toolset is
# not), and shown to stop a program built for emscripten that names it.
webcpp.compile-diagnostic openssl_on_webassembly : openssl_on_webassembly_test.cpp
  : <define>WEBCPP_TEST_ASIO_AS_WEBASSEMBLY : native ;
webcpp.compile-fail openssl_named_on_emscripten : openssl_named_test.cpp
  : <library>/webcpp/asio//openssl : emscripten ;
```

  `test/openssl_on_webassembly_test.cpp` includes `"../src/openssl_on_webassembly.hpp"` with
  `// expected-error@*:* {{webcpp::asio: /webcpp/asio//openssl is named in a build for WebAssembly, and OpenSSL is native only}}`;
  `test/openssl_named_test.cpp` is `int main() { return 0; }` after the notice and a
  comment.

  (`ssl_asyncify` joins `.asyncify`; the first headers-alone call's glob `asio/*.hpp` becomes
  the explicit list of the other top-level headers, so that `ssl.hpp` is taken once.) In
  `tools/ci/matrix_test.py`, a case that `needs_external('asio')` is true. Run. Expected:
  FAIL: `ssl.hpp` and `src/openssl_on_webassembly.hpp` do not exist, `openssl_named_on_emscripten`
  compiles (nothing refuses it yet, so the compile-fail test fails), and
  `needs_external('asio')` is false.
- [ ] **Step 2: Implement.** `build.jam` (with `import webcpp ;` at its top):

```jam
# OpenSSL 3, natively, for <webcpp/asio/ssl.hpp>, found by webcpp.external
# (-sOPENSSL_ROOT=<dir>, else .local/openssl, else the system's). A program
# names it in its own Jamfile, and the build stops when the machine lacks it
# (the owner, 2026-10-11, "an external library a program names and the
# machine lacks stops the build"); asio's own tests name openssl-tests,
# which leaves them out instead. On emscripten the ssl names are a record of
# the settings that links nothing, and a build that names openssl there is
# refused (below). Shared, the system's: the static choice comes with
# /third_party//openssl (AGENTS.md chapter 13).
# tag::openssl[]
local openssl-root = [ webcpp.external asio : OpenSSL : OPENSSL_ROOT : openssl
    : openssl/ssl.h : native ] ;
lib openssl-ssl : : <name>libssl <toolset>msvc <search>$(openssl-root)/lib
    <search>$(openssl-root)/lib/VC/x64/MD : : <include>$(openssl-root)/include ;
lib openssl-ssl : : <name>ssl <search>$(openssl-root)/lib
  : : <include>$(openssl-root)/include ;
lib openssl-crypto : : <name>libcrypto <toolset>msvc <search>$(openssl-root)/lib
    <search>$(openssl-root)/lib/VC/x64/MD : : <include>$(openssl-root)/include ;
lib openssl-crypto : : <name>crypto <search>$(openssl-root)/lib
  : : <include>$(openssl-root)/include ;
explicit openssl-ssl openssl-crypto ;

rule openssl-required ( properties * )
{
    if ! <toolset>emscripten in $(properties)
    {
        return [ webcpp.external-found asio : OpenSSL
            : $(properties) <webcpp-require-external>on ] ;
    }
}

rule openssl-linked ( properties * )
{
    if ! <toolset>emscripten in $(properties)
    {
        return <library>openssl-ssl <library>openssl-crypto
            <define>WEBCPP_ASIO_HAS_OPENSSL=1 ;
    }
}

# Refused on WebAssembly: every source of a program built for emscripten
# that names it stops at the forced include, whose message says what to
# write. Quoted, as the Jamroot quotes Boost's directory, for a path that
# holds a space.
path-constant WEBCPP_ASIO_ROOT : . ;
alias openssl : : <conditional>@openssl-required
  : : <conditional>@openssl-linked
    <toolset>emscripten:<compileflags>"-include \"$(WEBCPP_ASIO_ROOT)/src/openssl_on_webassembly.hpp\"" ;
# end::openssl[]

rule openssl-for-tests ( properties * )
{
    if ! <toolset>emscripten in $(properties)
    {
        return [ webcpp.external-found asio : OpenSSL : $(properties) ] ;
    }
}

alias openssl-tests : : <conditional>@openssl-for-tests
  : : <conditional>@openssl-linked ;
explicit openssl-tests ;
```

  `src/openssl_on_webassembly.hpp` (the notice, and a comment saying what forces it in):

```cpp
#if defined(__EMSCRIPTEN__) || defined(WEBCPP_TEST_ASIO_AS_WEBASSEMBLY)
#error "webcpp::asio: /webcpp/asio//openssl is named in a build for WebAssembly, and OpenSSL is native only: there <webcpp/asio/ssl.hpp> is a record of the TLS settings and links no library, and nothing of OpenSSL would be in the .wasm. In the program's Jamfile, name OpenSSL for native builds alone, by a condition in the project's requirements: project : requirements <conditional>@native-openssl ; rule native-openssl ( properties * ) { if ! <toolset>emscripten in $(properties) { return <library>/webcpp/asio//openssl ; } } (without b2: link OpenSSL into the native program only)"
#endif
```

  `openssl-tests`, asio's own tests' target, has no refusal: on emscripten it adds nothing, for
  the `ssl_asyncify` twin. `ssl.hpp` branches on `__EMSCRIPTEN__`: natively, after `config.hpp`, the `#error` above
  unless `WEBCPP_ASIO_HAS_OPENSSL`, then `#include <boost/asio/ssl.hpp>  // lint-world: TLS
  over Asio's sockets` and the namespace alias; on emscripten the record. `tools/ci/matrix.py`'s
  `EXTERNAL_DEPENDENCIES` gains `'asio': ('openssl',)`, with its comment naming asio's `ssl`.
- [ ] **Step 3: Run** every lane, `python3 tools/ci/matrix_test.py` and the lint. Expected:
  PASS, `ssl_without_openssl` among them. Then the two ways OpenSSL can be missing:
  - `$B2 -sOPENSSL_ROOT=/nonexistent libs/asio/test//ssl`. Expected: the build stops, naming
    `/nonexistent` (webcpp.external's refusal of a wrong `-s`).
  - With `.local/openssl` moved aside (`mv .local/openssl .local/openssl.aside`) and no `-s`
    (this machine's `/usr/include` and `/usr/local/include` hold no OpenSSL):
    `$B2 -n /webcpp/asio//openssl` stops with webcpp's message naming asio, OpenSSL and every
    place it looked; `$B2 -a --build-dir=bin/lane-native libs/asio/test` leaves `ssl` and
    `alone-asio-ssl` out with one line naming OpenSSL and builds every other test. Put it back
    (`mv .local/openssl.aside .local/openssl`) in the same step, and record both outputs in
    the ledger.
  - The compile-fail test passes on any compile error, so check that it fails for the right
    reason: `$B2 -a -d+2 --build-dir=bin/lane-emscripten libs/asio/test//openssl_named_on_emscripten`
    and confirm the output carries `/webcpp/asio//openssl is named in a build for WebAssembly`;
    a "file not found" (broken quoting, a wrong `WEBCPP_ASIO_ROOT`) is a failure of this step
    (re-review 2, R2-2). The refusal is a `<compileflags>`, so a C source of the program is
    refused as a C++ one is (R2-1).
- [ ] **Step 4: The form a program built for both targets writes, built by an example in
  the CI.** `example/tls/Jamfile`, a project of its own whose requirements are the user's
  lines, verbatim:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# A program built for native and for the browser that uses the ssl names:
# OpenSSL is named for native builds alone, by the condition in the
# project's requirements between the tags, the lines a program's own Jamfile
# writes (doc: #asio-tls). Its own lane, tls, on native and emscripten,
# builds and runs it, so that the ordinary lanes keep leaving OpenSSL out on
# a machine that lacks it, as webcpp's own examples do.
# tag::project[]
project : requirements <conditional>@native-openssl ;

rule native-openssl ( properties * )
{
    if ! <toolset>emscripten in $(properties)
    {
        return <library>/webcpp/asio//openssl ;
    }
}
# end::project[]

import webcpp ;

webcpp.targets native emscripten ;

webcpp.example asio_tls.cpp ;

webcpp.lane tls : asio_tls asio_tls.output : native emscripten ;
```

  (the parent `example/Jamfile`'s requirements give `/webcpp/asio//asio`). `asio_tls.cpp`
  makes `webcpp::asio::ssl::context ctx{webcpp::asio::ssl::context::tlsv12_client};`, calls
  `ctx.set_verify_mode(webcpp::asio::ssl::verify_peer);` and prints
  `context: tlsv12_client, verify_peer` (constants of its own, the same text natively, where
  Boost's context calls OpenSSL, and in the browser, where it is the record); its
  `.expected` holds that line. Run `$B2 -a --build-dir=bin/lane-tls libs/asio/example/tls//tls`
  and `$B2 -a --build-dir=bin/lane-tls-emscripten toolset=emscripten libs/asio/example/tls//tls`.
  Expected: PASS on both; the native link names `-lssl -lcrypto` and the emscripten one
  neither (`-d+2`). Then `$B2 -d0 declared-lanes`: lines `asio tls libs/asio/example/tls
  native programs` and `... emscripten programs`; and `python3 tools/ci/matrix.py own-lanes
  --library asio` gives the native entry `external` true, so the CI's job installs OpenSSL.
- [ ] **Step 5: Write the page's parts**, for a reader who may be an AI agent writing a
  Jamfile at once:
  - in `building.adoc`, a section `[#asio-tls]` "TLS: name OpenSSL for native builds alone":
    a program that includes `<webcpp/asio/ssl.hpp>` and is built only natively writes
    `exe app : app.cpp /webcpp/asio//asio /webcpp/asio//openssl ;`; one built for native and
    the browser writes the condition in its project, shown complete by including the region
    `project` of `{library-root}/example/tls/Jamfile` (built and run on both targets by the
    lane `tls`), then its programs as usual (`exe app : app.cpp /webcpp/asio//asio ;`); why
    that form (one place, every program, and the negated condition that silently links
    nothing, measured); the three messages a wrong Jamfile meets, each with its fix (the
    header's without OpenSSL, the build's stop when the machine lacks it, and the refusal on
    WebAssembly), and the region `openssl` of `{library-root}/build.jam`;
  - in `sockets.adoc`, "TLS settings": natively Boost's `ssl`; in the browser a record that
    webcpp::beast's streams judge; and the example's listing and output.
- [ ] **Step 6: Commit** (library): `"asio: the ssl names, Boost's natively with OpenSSL named
  by the program and refused on WebAssembly, and a record of the settings in the browser"`; (superproject):
  `"tools/ci: asio's ssl needs OpenSSL natively"` with `tools/ci/matrix.py tools/ci/matrix_test.py`.

## Task 22: The rulebook, the index and the page whole

**Files:**
- Modify (superproject): `AGENTS.md` (chapter 1's table of libraries and layout; chapter 6's
  "Asynchronous code is built on xactor" item: how a library meets Boost.Asio is now asio's,
  the rest still the roadmap's; chapter 13's asio item), `README.md` (a row for asio).
- Modify (library): `README.md`, `AGENTS.md`, `doc/asio.adoc` (the order of its sections).

- [ ] **Step 1: Write chapter 13's items.** The item "Every variant tested, in three levels of
  CI" gains: "asio's own lane `asyncify`, which runs on every push until the levels exist,
  moves to the nightly and release levels (WEBCPP_ASIO_SPEC.md section 11), and the native
  suites run without exceptions in the native no-exceptions lane." The item "External
  libraries built from `third_party/`" gains: "asio's `/webcpp/asio//openssl` moves onto
  `/third_party//openssl` and its shared-or-static feature once webcpp::crypto builds it."
  The item "webcpp::asio, webcpp::beast, webcpp::webrtc, webcpp::crypto" keeps beast, webrtc
  and crypto, and says asio is a library of `libs/` (chapter 1).
- [ ] **Step 2: Write the rows.** AGENTS.md's table of libraries:
  `| asio | Boost.Asio natively, and its own io_context suspended into the browser's event loop, webcpp's own | native, emscripten; its own lanes asyncify and browser on emscripten, and tls on both | a local repository at libs/asio until webcpporg/asio exists (chapter 2) |`;
  the layout's `libs/<name>/` line names asio; README.md's table gains asio with its
  targets `native, emscripten`.
- [ ] **Step 3: Order the page:** overview, names, quick start, building, the run family,
  browser events, promises and events, timers, Cobalt, sockets and resolution, allocation,
  guarantees, testing, reference. Run `$B2 -a --build-dir=bin/b-doc doc`. Expected: PASS, the
  index lists asio, linked to its page; doc-check finds nothing on either page.
- [ ] **Step 4: Check the text:** `grep -rn $'\xe2\x80\x94' libs/asio AGENTS.md README.md` prints
  nothing; `git -C libs/asio grep -I -i -e 'veru[s]'` prints nothing.
- [ ] **Step 5: Commit** both repositories: (library) `"asio: the page in its order, the
  README and the library's rules"`; (superproject) `"AGENTS.md and README.md: asio, the
  library on Boost.Asio for the browser"`.

## Task 23: Green, review and the gates

- [ ] **Step 1: Run the definition of green** (AGENTS.md chapter 10), each from scratch, and
  record each result in the ledger:
  - the native lane, with `clang` (Apple Clang here) and `gcc-15`
    (`--build-dir=bin/lane-gcc toolset=gcc-15`);
  - the emscripten lane; the own lanes `asyncify`, `browser` and `tls` (both targets);
  - `$B2 -a --build-dir=bin/b-doc doc`;
  - the lint: `lint: clean`;
  - every `tools/**/*_test.py` touched or testing what was touched:
    `tools/test/emscripten_test.py`, `tools/test/webcpp_jam_test.py`,
    `tools/test/jamroot_test.py`, `tools/test/doc_test.py`, `tools/variants/variants_test.py`,
    `tools/ci/matrix_test.py`, `tools/ci/actions_test.py`, `tools/doc/counts_test.py`,
    `tools/lint/lint_test.py`;
  - pratt's and trystero's emscripten lanes, which Task 2's test support now reaches;
  - actionlint on `.github/workflows/library.yml`: no finding;
  - `python3 tools/ci/matrix.py plan --library asio` and `own-lanes --library asio`: the
    lanes of every native compiler and emscripten, and the own lanes
    `emscripten.asio.asyncify`, `emscripten.asio.browser` (with `chrome` true),
    `native.asio.tls` and `emscripten.asio.tls` (with `external` true).
- [ ] **Step 2:** Dispatch the whole-branch review (the library and the superproject), on the
  most capable model, with this plan's Review Focus and the spec.
- [ ] **Step 3:** Fix what it finds, each with a failing test first.
- [ ] **Step 3a (controller):** record in the ledger the owner's approvals of 2026-10-11
  (departures 1, 2, 3, 4 and 5, as the "Departures from the spec" section states them);
  nothing is asked of him again unless the review of Step 2 changes one of them.
- [ ] **Step 4 (controller, G1):** with the owner's agreement, the owner creates
  `webcpporg/asio`.
- [ ] **Step 5 (controller, G2):** the owner adds the submodule
  (`git submodule add ../asio.git libs/asio`) and the exclude line goes; the superproject
  commits the submodule: `"libs/asio: the library on Boost.Asio for the browser"`.
- [ ] **Step 6 (controller, G3):** with the owner's agreement, push the library, then the
  superproject; watch both CIs, and the report's columns for asio
  (`https://webcpporg.github.io/webcpp/report/asio.html`).

---

## Spec coverage

| Spec | Tasks |
| --- | --- |
| 1, 1.1 purpose and why | 1 (overview), 22 |
| 2.1 variants at build time, named after the variant | 2, 3, 5, 12, 13 |
| 2.2 what a browser cannot do fails as a value | 19, 20 |
| 2.3 webcpp::webrtc a library of its own | Not in this plan |
| 2.4 exceptions on and off, every variant tested, allocators, building order, fixed-width types | 1 (config), 5 (twins), 9, 11 (the C library counted too), 17, 21; exceptions off run on emscripten only, natively once the CI's levels exist (Departure 5) |
| 2.5 resolution option A | 20, with `""` as port 0 |
| 3 targets (native, emscripten; no WASI), the names table, `BOOST_ASIO_DISABLE_THREADS`, the io_context class | 1, 4 (and Departure 2), 19, 20, 21 |
| 4.1 the reactor made at construction | 5 |
| 4.2 the run family and its composition | 6; admission once per public call, 8 |
| 4.3 the turn limit, the `MessageChannel` yield, the default measured in the browser | 6, 13 (each turn timed, the 95th percentile) |
| 4.4 the variants, exceptions per variant, Asyncify's flags and stack, names, the loader, ES modules, the size table of every example | 2, 3 (and the CI keeps the table), 5, 9, 12, 13, 14, 16, 17, 18, 19, 20 |
| 4.5 who may call the run family, detection, the lint rule, xactor's driver | 6 (page), 8 (the driver's case too) |
| 4.6 the bridge, its JavaScript boundary | 7 (the boundary reached without exceptions) |
| 4.7 promises and events | 17, 18 (a pending wait's listener removed at destruction) |
| 4.8 live objects and after `main` | 6 (page), 7 (test) |
| 5.1 timers | 4, 10, and 13 for the reactor's one wait |
| 5.2 Cobalt's `run_main`, its build, its probe, the twin | 15, 16 (Departure 3) |
| 6 webcpp::beast | Not in this plan |
| 7 sockets, the transport point, the resolver, `ssl`'s owner | 19, 20, 21 (OpenSSL named by the program, refused on WebAssembly, the project's condition built on both targets by the lane `tls`; Departure 4) |
| 8 allocation, exceptions, determinism (`lint-world:`) | 11, 9, every header |
| 9 testing: native, both variants under node, the browser lane (both variants), sizes, twins | 2 (every test bounded and ending as its main ends), 4 to 21, 13, 14 |
| 10 increments 1 to 4 | 1 to 14, 15 and 16, 17 and 18, 19 to 21 |
| 11 settled: default `jspi`; Asyncify at nightly and release | 2; the own lane `asyncify` runs on every push until the levels exist (Departure 1) |
