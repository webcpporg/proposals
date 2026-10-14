# Working on asio

webcpp's rules apply here: read the superproject's
[AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first. This file
holds only what is specific to asio.

## What asio is

A library of webcpp's own on Boost.Asio, not a port: `meta/libraries.json`
has `"port-of": null`, and asio has no oracle, no twins and no "Differences"
appendix. Its page, `doc/asio.adoc`, with the Doc Comments of its headers,
and its specification are what a change is checked against.

Natively asio is Boost.Asio itself. On emscripten it is still Boost.Asio's
own `io_context`, whose `run` suspends into the browser's event loop, and
asio adds only what Boost lacks there: the configuration Asio needs to
build, a run family that yields to the page after every turn of
`max_handlers_per_turn` handlers, and a bridge through which browser events
reach the `io_context`.

## Cautions

- **Asyncify's stack is measured, not guessed.** `-sASYNCIFY_STACK_SIZE`
  is 8192 unless a program raises it with `<asyncify-stack-size>`, declared
  in `build.jam`: twice 4096, the smallest power of two with which every
  test passed in every build b2 links today (2026-10-11, Emscripten 6.0.11).
  The deepest is release, compiled with `-O3` and linked without
  optimization, as b2's emscripten toolset links it: 3,540 bytes in
  `bridge_asyncify`, against 1,272 in a debug build. Linked with `-O3`, the
  deepest test wrote 416 and the run family's deepest path 384, against
  2,404 linked without. The asyncify lane builds debug alone, so the
  measurement covers the other builds itself: the bytes are read from
  Asyncify's own data when an unwind stops, and `asyncify_stack` does it in
  debug, release and release linked with `-O3`, for every suspension of the
  run family, failing when one leaves less than half of the stack. A change
  that adds a way to suspend (a coroutine's, a promise's) measures it there,
  and a test that overflows traps with `RuntimeError: unreachable`, every
  time. When b2's release link gets its `-O`, measure again: the deepest
  build becomes debug's.
- **The stack test must reach the intended wait.** Its reactor cases hold
  work until a JavaScript macrotask posts the completing handler, delay entry
  by 25 ms and require a nonzero-timeout poll. An already-expired C++ timer
  can suspend in the initial poll without reaching the deeper wait. The
  test reads `doPollAsync` as well as Asyncify's private runtime; a runtime
  change that removes either observation must fail rather than skip it.
  The 2026-10-14 re-review measured maxima of 1164/844 bytes in debug,
  2896/1940 in release and 384/288 in release linked with `-O3`, with/without
  exceptions respectively. `run_until` has its own yield measurement too.
- **The run family is called on `main`'s stack alone**, or on the stack of
  another export the program lists for suspension. Anywhere else its first
  suspension fails: under JSPI it throws a `SuspendError`, and under
  Asyncify it starts an unwind that corrupts a suspended `main`. Nothing in
  asio, and nothing in a library built on it, calls `run`, `run_one`,
  `poll` or `dispatch` from a JavaScript event: a library is given an
  executor, and a program's own `run` says why after `lint-run:`.
- **The bridge only posts.** Delivering a browser event looks its
  registration up and posts its handler to the `io_context`; it never polls,
  runs or dispatches. While `run` is suspended, an event sees
  `running_in_this_thread()` true, so a `dispatch` there would run its
  handler inside the event, on the wrong stack; and under Asyncify a wake
  must come from JavaScript with no compiled code on the stack, which only a
  post gives.
- **A test ends with `webcpp::test::exit_status(...)`**: every test's `main`
  returns `webcpp::test::exit_status(boost::report_errors())`, from the
  superproject's `<webcpp/test/exit_status.hpp>`. Under Asyncify a `main`
  that suspended and then returns a failure exits 0, and one that never
  finishes exits 0 on both variants; the superproject's test support fails a
  test whose `main` did not finish, and `exit_status` is how a `main` tells
  it that it did, and with which status. A test that returns
  `boost::report_errors()` alone reads as passed whatever it found.
- **Two translation units under `src/`.** asio is header-only but for
  `src/browser_src.cpp`, the JavaScript of the yield, of the bridge and of
  the promises, and the export the bridge delivers through, which the usage
  requirements of `/webcpp/asio//asio` link on emscripten; and
  `src/cobalt_main.cpp`, webcpp's definition of Boost.Cobalt's
  `run_main`, which gives `co_main` the turn limit and which
  `/webcpp/asio//cobalt` links. A program never compiles either itself: the
  targets link each once, and a second definition fails at link.

## The turn limit, measured in Chrome

On 2026-10-14, macOS arm64, HeadlessChrome 153.0.8010.12 from the local
Playwright cache, Emscripten 6.0.11, Boost 1.92 and b2 debug builds:

| Run | Variant | Animation callbacks | Handlers | Full turns | Turn p95 (ms) | Maximum (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | jspi | 60 | 1724312 | 1724 | 0.700 | 1.800 |
| 1 | asyncify | 61 | 652723 | 652 | 1.600 | 9.200 |
| 2 | jspi | 60 | 1734454 | 1734 | 0.700 | 1.800 |
| 2 | asyncify | 61 | 644126 | 644 | 1.600 | 9.200 |
| 3 | jspi | 60 | 1744071 | 1744 | 0.700 | 1.900 |
| 3 | asyncify | 61 | 653795 | 653 | 1.600 | 8.900 |

Each run rebuilt the browser lane from scratch with `-a -j1`, using a
separate build directory. The programs self-post trivial handlers for one
second, with the default 1,000 handlers per turn. Callback entry times
come from `performance.now()`. Full turns end at the yield's message post;
the last partial turn is excluded. Both unbounded Boost controls had zero
animation callbacks in every run. These observations show animation
opportunities, not proof that particular pixels were painted.

The three runs also passed idle and loader checks on both variants: one
`setTimeout(300000)` during each three-second idle observation, and three
handlers from the artifact actually selected by the loader. The JSPI
factory awaits `main`, so the test identifies the imported URL before
awaiting that factory; an intentionally idle main never returns.

The p95 stayed below the task's 8 ms stop threshold, so the default remains
1,000. The maximum is recorded without hiding outliers. These are local
measurements, not a guarantee for heavy user handlers or other hardware.
The CI-pinned Chrome version was not executed in this session; no CI ran.
The driver fails when p95 exceeds 8 ms or no animation callbacks occur;
changing the default or replacing that witness requires the owner's decision.
