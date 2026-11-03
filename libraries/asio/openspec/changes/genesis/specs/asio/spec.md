## Purpose

webcpp::asio is a header-only C++20 wrapper over Boost.Asio, for native, Emscripten and WASIp3 targets.
Natively it is Boost.Asio itself; compiled with Emscripten it is still Boost's own `io_context`,
whose `run` suspends into the browser's event loop, and it adds only what Boost lacks there: the
configuration Asio needs to build, a reactor made at construction, a limit on the handlers one
turn runs so that the page keeps rendering, and a bridge through which browser events post.
These requirements reconcile the original accepted browser specification with the owner's
later WASIp3, source-built TLS, Cobalt and embedded-CA approvals, ratified on 2026-11-03.
The source snapshot and completed validation are identified in `reference/STATUS.md`;
requirements alone do not establish that a gate has passed.

## ADDED Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20 in namespace `webcpp::asio`, with public headers under `<webcpp/asio/...>`, internals in `webcpp::asio::detail`, the browser's types in `webcpp::asio::browser`, macros prefixed `WEBCPP_ASIO_` and include guards `WEBCPP_ASIO_<PATH>_HPP`, except for the two translation units under `src/` that the build compiles for a program: `src/browser_src.cpp` (the JavaScript of the yield and of the bridge, linked on Emscripten) and, from the plan's task 16, `src/cobalt_main.cpp`.

#### Scenario: A header is the only include of its translation unit
- **WHEN** each of `asio.hpp`, `asio/config.hpp`, `asio/io_context.hpp`, `asio/steady_timer.hpp`, `asio/system_timer.hpp` and `asio/basic_waitable_timer.hpp` is compiled alone with `-std=c++20` and Boost 1.92's headers
- **THEN** each compiles (`-fsyntax-only` exit 0), and `test/Jamfile` declares `webcpp.headers-alone` for them on both targets and for `asio/browser/*.hpp` and `asio/detail/*.hpp` on Emscripten alone

### Requirement: Library: Native, Emscripten and WASIp3 targets
The library SHALL declare `webcpp.targets native emscripten wasip2 wasip3`. Its WASIp3 backend SHALL use checked, version-pinned Boost overlays for timers, TCP and UDP. Its WASIp2 core SHALL support timers, TCP, UDP and pollable-based asynchronous forward DNS in the single-thread profile with exceptions on or off. Unsupported P2 thread, Cobalt and guest TLS build targets SHALL be rejected explicitly. Libraries using Asio's socket reactor MUST follow these targets; portable messages and host APIs without that reactor may support additional targets independently.

#### Scenario: The target declarations match the backend
- **WHEN** the test and example Jamfiles are read
- **THEN** their supported target declarations include native, Emscripten, WASIp2 and WASIp3, with P2 limited to its verified single-thread feature profile

### Requirement: WASIp3: Networking, TLS and threading profiles
WASIp3 SHALL support timer waits, TCP accept/connect/read/write, UDP send/receive and cancellation using the host's permitted network. Optional source-built OpenSSL SHALL provide guest TLS, embedded or replacement CA trust, peer verification and explicit per-connection hostname/SNI configuration. Single-thread profiles SHALL support exceptions on and off. Cooperative-thread profiles SHALL require the matching SDK sysroot; a request for exceptions without a supplied C++ exception runtime MUST fail by name instead of silently disabling exceptions.

#### Scenario: Real network and encrypted operations
- **WHEN** loopback TCP/UDP and TLS tests run on single-thread profiles and the supported cooperative profile
- **THEN** real peers exchange data, cancellation completes with the documented errors, untrusted peers fail verification, and entropy failure prevents key generation

#### Scenario: Unsupported cooperative exception runtime
- **WHEN** a WASIp3 multi-thread build requests exceptions using an SDK whose cooperative sysroot has no C++ exception runtime
- **THEN** configuration stops with an actionable diagnostic naming that profile limitation

### Requirement: Library: Convenience header
`include/webcpp/asio.hpp` SHALL include `config.hpp`, `basic_waitable_timer.hpp`, `io_context.hpp`, `steady_timer.hpp` and `system_timer.hpp` and, on Emscripten alone, `browser/bridge.hpp`; it MUST leave out `<webcpp/asio/ssl.hpp>`, which natively brings OpenSSL.

#### Scenario: One include gives the io_context and the timers
- **WHEN** a program includes only `<webcpp/asio.hpp>` and names `webcpp::asio::io_context`, `webcpp::asio::steady_timer`, `webcpp::asio::system_timer` and `webcpp::asio::basic_waitable_timer<std::chrono::steady_clock>`
- **THEN** it compiles natively, and on Emscripten `webcpp::asio::browser::bridge` is also declared

### Requirement: Library: Configuration header and the no-exceptions macro
`<webcpp/asio/config.hpp>`, which every public header includes first, SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_ASIO_NO_EXCEPTIONS` when `WEBCPP_NO_EXCEPTIONS` or `BOOST_NO_EXCEPTIONS` is defined, and MUST keep it when a developer defines it in a build that has exceptions; an API of asio that throws sits behind `#ifndef WEBCPP_ASIO_NO_EXCEPTIONS`.

#### Scenario: The macro follows the build
- **WHEN** `config_test.cpp` is compiled with exceptions, without exceptions (`-fno-exceptions`), and with `-DWEBCPP_ASIO_NO_EXCEPTIONS -DWEBCPP_TEST_ASIO_DEVELOPER_DEFINED`
- **THEN** each compiles: the macro follows the compiler or global WebCPP exception setting, and the developer's definition is kept

#### Scenario: The test lane builds the configuration twice
- **WHEN** `test/Jamfile` is read
- **THEN** it declares `webcpp.compile config` and `webcpp.compile config_developer_defined`

### Requirement: Library: A size of Asyncify's stack below asio's is refused at compile time
`config.hpp` SHALL stop the compile with an `#error` whose text says that `asyncify-stack-size` is below 8192 bytes, the least that asio's measured suspensions need with half of it left to the program, when `WEBCPP_ASIO_ASYNCIFY_STACK_SIZE_REFUSED` is defined, which `build.jam` defines for such a size.

#### Scenario: A size of 4096 does not compile
- **WHEN** `asyncify_stack_size_test.cpp` is built with `<webcpp-suspend>asyncify <asyncify-stack-size>4096`
- **THEN** it does not compile (`asyncify_stack_size_refused`, a `compile-fail`), while the same source with `<asyncify-stack-size>8192` compiles (`asyncify_stack_size_control`)

#### Scenario: Global configuration reaches Asio
- **WHEN** `WEBCPP_NO_EXCEPTIONS` is defined before including an Asio header
- **THEN** Asio's no-exceptions setting is active, and its B2 target propagates the config library dependency

### Requirement: Library: Repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (key `asio`, `"port-of": null`, categories `["Concurrent", "IO"]`, `cxxstd` `20`, maintainer `R <github.overboard312@passmail.net>`), `README.md`, `AGENTS.md` (only what is specific to asio), `LICENSE_1_0.txt`, `.gitignore` (at least `doc/html/`), `.gitattributes`, `.github/workflows/ci.yml` calling the superproject's reusable `library.yml` with `library: asio`, `doc/` including a maintained `doc/changelog.adoc` as the last page appendix, `test/` and `example/`.

#### Scenario: The metadata names no port
- **WHEN** `meta/libraries.json` is read
- **THEN** it holds `"key": "asio"`, `"port-of": null`, `"cxxstd": "20"` and the description "Boost.Asio natively, and in the browser Boost.Asio's own io_context suspended into the page's event loop."

### Requirement: Library: Every source file carries the licence and every world access says why
Every source file SHALL open with the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License, Version 1.0; every library header line that names Boost.Asio, a clock, a timer or `this_thread::` MUST say why after `lint-world:`; every call of `run`, `run_one` or `run_for` on an `io_context` MUST say why after `lint-run:`.

#### Scenario: The run family's own calls of Boost's are annotated
- **WHEN** `io_context.hpp` is read
- **THEN** its calls of `boost::asio::io_context::run_one()` carry `// lint-run: step 4, the one wait` and `// lint-run: step 4 alone`, and its `using` declarations of Boost's `run`, `run_for` and `run_one` carry `// lint-run: Boost's own, natively`

### Requirement: Build: One target gives a program everything on every target
`build.jam` SHALL declare `project /webcpp/asio` and an alias `asio` whose usage requirements are `<include>include` and `<library>/boost//headers` on every target and, on Emscripten, `<define>BOOST_ASIO_DISABLE_THREADS`, `<linkflags>-lembind`, `<source>browser-src`, the link flags of the program's suspension variant, the conditional `asyncify-stack` and `<tag>@webcpp%suspend-tag`; a program MUST reach the library through `/webcpp/asio//asio`.

#### Scenario: A program names one target
- **WHEN** a Jamfile writes `exe app : app.cpp /webcpp/asio//asio ;` and is built with `toolset=emscripten`
- **THEN** the program compiles with `BOOST_ASIO_DISABLE_THREADS`, links embind and `browser-src`, and its name carries the variant

### Requirement: Build: Boost.Asio is built without threads on Emscripten
On Emscripten the usage requirements SHALL define `BOOST_ASIO_DISABLE_THREADS`, because without it Asio decides it has threads under any `__GNUC__` outside Linux and `<boost/asio/io_context.hpp>` stops at "Only Windows and POSIX are supported!"; since Asio's inline namespace differs with and without threads, a translation unit that missed the define MUST fail at link and never silently.

#### Scenario: Every translation unit of a program shares the define
- **WHEN** `build.jam`'s `alias asio` and `obj browser-src` are read
- **THEN** both carry `<toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS`

### Requirement: Build: The browser's JavaScript is compiled once and linked into every program
`build.jam` SHALL declare the explicit object `browser-src` from `src/browser_src.cpp`, whose only content is `#include <webcpp/asio/browser/src.hpp>`, and the usage requirements MUST link it into every program on Emscripten, so that the JavaScript functions asio calls and the export the bridge delivers through exist exactly once; a program built otherwise includes `browser/src.hpp` in exactly one source file.

#### Scenario: A second definition fails at link
- **WHEN** a program on Emscripten links `/webcpp/asio//asio` and also includes `<webcpp/asio/browser/src.hpp>` in one of its sources
- **THEN** the link fails, since a second definition of the export is an error (asio's `AGENTS.md`: a second definition fails at link)

### Requirement: Build: The suspension variant is a propagated composite feature
The superproject SHALL declare the feature `webcpp-suspend` with values `jspi` (default) and `asyncify`, propagated and composite, whose `asyncify` composes `<exception-handling-method>js`, so that `webcpp-suspend=jspi exception-handling=on` compiles with `-fwasm-exceptions` and links `-sJSPI`, `webcpp-suspend=asyncify exception-handling=on` compiles with `-fexceptions` and links `-sASYNCIFY`, a build with `exception-handling=off` compiles `-fno-exceptions` on both, and a non-default variant builds in the directory `webcpp-suspend-asyncify`; natively the feature is ignored.

#### Scenario: The variants build apart
- **WHEN** `libs/browser_demo/test//catches` is built as a dry run with and without `webcpp-suspend=asyncify` (`test_suspend_variants_build_apart`)
- **THEN** the default has `-fwasm-exceptions` and no `webcpp-suspend-` directory, and asyncify has `-fexceptions`, no `-fwasm-exceptions` and the directory `/webcpp-suspend-asyncify/`

#### Scenario: A native build ignores the variant
- **WHEN** the native build of the demo library runs (`test_a_native_build_ignores_the_variant`)
- **THEN** no path under `bin` has `webcpp-suspend` in its name

### Requirement: Build: Each variant links its own flags and no list of imports
The alias SHALL link `-sJSPI` under `<webcpp-suspend>jspi` and `-sASYNCIFY` under `<webcpp-suspend>asyncify` on Emscripten, and MUST NOT set `ASYNCIFY_IGNORE_INDIRECT` or list the suspending imports, because Emscripten finds `__syscall_poll` and the `__asyncjs__` yield itself and the path from `main` to the reactor's `select` passes the virtual `scheduler_task::run`.

#### Scenario: The alias carries the flags
- **WHEN** `build.jam` is read
- **THEN** it holds `<toolset>emscripten,<webcpp-suspend>jspi:<linkflags>-sJSPI` and `<toolset>emscripten,<webcpp-suspend>asyncify:<linkflags>-sASYNCIFY`, and no `ASYNCIFY_IGNORE_INDIRECT`

### Requirement: Build: Asyncify's stack size is a propagated feature that ends last on the link line
`build.jam` SHALL declare the propagated feature `asyncify-stack-size` with the values 8192, 16384, 32768, 65536, 131072, 262144, 524288 and 1048576 (and 256 to 4096, declared only so that they are refused), default 8192, and the usage requirement MUST link `-sASYNCIFY_STACK_SIZE=<size>` under Emscripten and `asyncify`, as a conditional so that it ends last on emcc's command line, where emcc keeps the last flag; a size below 8192 MUST give `<define>WEBCPP_ASIO_ASYNCIFY_STACK_SIZE_REFUSED` instead.

#### Scenario: The default and a raised size are what the program was linked with
- **WHEN** `asyncify_stack_size_test.cpp` reads `Asyncify.StackSize` in a program built with no feature and with `<asyncify-stack-size>16384`
- **THEN** it equals 8192 (`asyncify_stack_size_default`) and 16384 (`asyncify_stack_size_raised`)

### Requirement: Build: Artifacts are named and placed Boost's way
The superproject SHALL name an artifact on Emscripten by b2's own tags joined by `-` (`<base> <toolset> <threading> <runtime> <property:webcpp-suspend>`), the toolset tag being the version alone (`60` for Emscripten 6.0.11), `d` marking a debug build and the variant ending the name, placing a non-default property in a directory of its own, so that the same program `app` is `bin/emscripten-6.0.11/debug/.../app-60-d-jspi.js`, `.../webcpp-suspend-asyncify/app-60-d-asyncify.js` in debug and `.../app-60-jspi.js` in release, each beside its `.wasm`.

#### Scenario: The tag rule is a usage requirement
- **WHEN** a program links `/webcpp/asio//asio` on Emscripten
- **THEN** its executable's name carries the variant (`reactor-60-d-jspi.js`), because the alias gives `<tag>@webcpp%suspend-tag`

### Requirement: Build: A page program is built in both variants with a manifest
The superproject SHALL provide `webcpp.variants <name> : <sources> : <requirements>`, which declares the explicit executable `<name>` on Emscripten alone, linked as an ES module (`-sMODULARIZE -sEXPORT_ES6`) in both variants, and the explicit `<name>.variants.json` beside the `jspi` artifact, which names each variant's script relative to itself and the size of its `.wasm`, as `tools/variants/variants.py manifest` writes it; a manifest whose script is missing, and a variant asked for with JavaScript exceptions under `jspi` or with WebAssembly exceptions under `asyncify`, MUST be refused.

#### Scenario: The manifest names both artifacts
- **WHEN** `webcpp.variants page_program : page_program.cpp ;` is built
- **THEN** `page_program.variants.json` holds the keys `jspi` and `asyncify`, each with a `script`, and `variants_test.py` passes its cases, a missing script among them

### Requirement: Build: The size of each variant of each example is recorded
The superproject SHALL provide `webcpp.size-table <name> : <variants> +`, the explicit `<name>.sizes.md`, a Markdown table of the size of each variant of each program that the build also prints and that the CI keeps as the artifact `sizes-<lane>`; the table records every example of the guide in both variants.

#### Scenario: Asyncify's artifact is larger
- **WHEN** the size table is built for the plan's probe program
- **THEN** its Asyncify row is about three times its JSPI row (the review measured 283,409 bytes against 97,429 for the timer program, 297,636 against 102,988 for the turn-limit program)

### Requirement: Build: Every test and example links the test support
The superproject SHALL give every `webcpp.run`, `webcpp.run-fail` and `webcpp.example` the target `/webcpp//node-test`, whose usage requirements are `<include>tools/node-test/include` everywhere and, on Emscripten, `--pre-js tools/node-test/node-pre.js` (a dependency of every program that links it), so that no library's Jamfile can leave it out.

#### Scenario: The support reaches a program that links a variant's flags
- **WHEN** `emscripten_test.py`'s `test_every_test_and_example_links_the_support` runs
- **THEN** a program built by `webcpp.run` links the pre-js on Emscripten and finds `<webcpp/test/exit_status.hpp>` on every target

### Requirement: Names: The io_context is a class derived from Boost's
`webcpp::asio::io_context` SHALL derive from `boost::asio::io_context` on both targets, convert to `boost::asio::io_context&` and share its executor type, so that code written against either interoperates.

#### Scenario: The static assertions of names_test hold
- **WHEN** `names_test.cpp` is compiled
- **THEN** `std::is_base_of_v<boost::asio::io_context, webcpp::asio::io_context>`, `std::is_convertible_v<webcpp::asio::io_context&, boost::asio::io_context&>` and `std::is_same_v<webcpp::asio::io_context::executor_type, boost::asio::io_context::executor_type>` all hold

#### Scenario: A posted handler runs
- **WHEN** one handler is posted to a `webcpp::asio::io_context` and `run()` is called
- **THEN** `run()` returns 1 and the handler ran once

### Requirement: Names: The timers are Boost's own
`webcpp::asio::steady_timer`, `system_timer` and `basic_waitable_timer<Clock, WaitTraits, Executor>` (with Boost's default arguments) SHALL be aliases of the Boost.Asio types of the same names, in `<webcpp/asio/steady_timer.hpp>`, `system_timer.hpp` and `basic_waitable_timer.hpp`, so that their interface, any completion token, the per-operation cancellation slot and the allocator associations are Boost's unchanged; on Emscripten they sit in the reactor's timer queue, which `run` waits on.

#### Scenario: The aliases are the same types
- **WHEN** `names_test.cpp` is compiled
- **THEN** `std::is_same_v<webcpp::asio::steady_timer, boost::asio::steady_timer>`, `std::is_same_v<webcpp::asio::system_timer, boost::asio::system_timer>` and `std::is_same_v<webcpp::asio::basic_waitable_timer<std::chrono::steady_clock>, boost::asio::basic_waitable_timer<std::chrono::steady_clock>>` hold

### Requirement: Names: Every other name of Boost.Asio keeps being Boost's
A program SHALL keep naming `post`, `dispatch`, `co_spawn`, `use_awaitable`, `bind_executor`, executors and completion tokens directly from `boost::asio::`, and webcpp::asio MUST NOT alias them; the only aliases and classes of asio are the io_context, the three timer names and, from the plan's tasks 19 to 21, `ip::tcp`, `ip::udp`, the socket, acceptor and resolver templates and `ssl`.

#### Scenario: Boost's operations run on asio's context
- **WHEN** a program calls `boost::asio::post(io, handler)` and `boost::asio::dispatch` on a `webcpp::asio::io_context`
- **THEN** they behave as on Boost's context (`names_test`: `a_posted_handler_runs`)

### Requirement: Options: The io_context's options and their default
`webcpp::asio::io_context_options` SHALL be a struct with one member, `std::size_t max_handlers_per_turn`, defaulting to `WEBCPP_ASIO_MAX_HANDLERS_PER_TURN` (100 unless defined by the application before including config), which bounds the handlers one turn of the run family runs on Emscripten before it yields to the browser; a value of 0 MUST be taken as 1.

#### Scenario: The default, a given value and zero
- **WHEN** `names_test`'s `options_are_kept` constructs a default context, a context with `io_context_options{.max_handlers_per_turn = 64}` and one with `{.max_handlers_per_turn = 0}`
- **THEN** `max_handlers_per_turn()` is 100, 64 and 1

### Requirement: Options: The option can be read and changed later
`io_context` SHALL provide `std::size_t max_handlers_per_turn() const noexcept` and `void max_handlers_per_turn(std::size_t n) noexcept`, where 0 is taken as 1, on both targets.

#### Scenario: Setting 250 and then 0
- **WHEN** a default context is given `max_handlers_per_turn(250)` and then `max_handlers_per_turn(0)`
- **THEN** the getter returns 250 and then 1

### Requirement: Options: The constructors take the options last
`io_context` SHALL provide `io_context()`, `explicit io_context(io_context_options)`, `io_context(std::allocator_arg_t, const Allocator&, io_context_options = {})`, `explicit io_context(int concurrency_hint, io_context_options = {})` and `io_context(std::allocator_arg_t, const Allocator&, int concurrency_hint, io_context_options = {})`, the allocator forms passing the allocator to Boost's constructor, which allocates the context's services and its I/O objects' state with it, and the hint forms forwarding Boost's concurrency hint, so that Boost's idiom `io_context io{1}` compiles unchanged (departure 2 of the plan: the spec lists three constructors).

#### Scenario: Boost's idiom and a hint with options
- **WHEN** `names_test`'s `a_concurrency_hint_is_boost_s` constructs `webcpp::asio::io_context io{1}` and one with `BOOST_ASIO_CONCURRENCY_HINT_1` and `.max_handlers_per_turn = 8`
- **THEN** the first reports 100 and the second 8, and a posted handler runs once under `run()`

### Requirement: Options: Natively the options change nothing and the run family is Boost's
Natively `io_context` SHALL bring Boost's `run`, `run_for`, `run_one`, `run_one_for`, `run_one_until` and `run_until` into its scope with `using` declarations and MUST accept the options without effect, since no page waits on a native loop.

#### Scenario: The native twin of the run family keeps Boost's meaning
- **WHEN** `run_family_test.cpp` runs natively (`run_family_native`)
- **THEN** it passes: `run` returns the handlers run, `run_one` runs one, a timer is waited for, the timed forms return at their deadline, `stop` and `restart` work, a passed deadline runs nothing, and a limit of 0 still runs

### Requirement: Reactor: The reactor exists from construction
On Emscripten the io_context SHALL hold one unarmed `boost::asio::steady_timer` member, made through public API, so that the reactor is the scheduler's task from construction, and a `run` that has work and no timer of the program's own waits in `select`, which a post wakes, and never in `null_event::do_wait`'s `::pause()`, which nothing wakes.

#### Scenario: A post from a browser event wakes run
- **WHEN** `reactor_test`'s `a_post_from_an_event_wakes_run` holds a work guard, a JavaScript `setTimeout` of 20 ms calls an exported function that posts a handler resetting the guard, and `io.run()` is called
- **THEN** `run()` returns 1 and the handler ran once

### Requirement: Reactor: Work posted before run waits for it
On Emscripten, as natively, a handler SHALL run only on the stack of a run family call: work posted before `run` is called waits for it.

#### Scenario: Nothing runs without run
- **WHEN** `reactor_test`'s `work_posted_before_run_waits_for_it` posts a handler and reads its counter before and after `io.run()`
- **THEN** the counter is 0 before and 1 after, and `run()` returns 1

### Requirement: Run: The six members keep Boost's meaning on every target
`run()`, `run_one()`, `run_for(d)`, `run_until(t)`, `run_one_for(d)` and `run_one_until(t)` SHALL keep Boost.Asio's meaning: `run` returns when the io_context has no outstanding work or is stopped, `run_one` after one handler, `run_for` and `run_until` at their deadline at most, and each returns the number of handlers it ran; `poll()` and `poll_one()` are Boost's on every target.

#### Scenario: Out of work
- **WHEN** `run_family_test`'s `run_returns_out_of_work_with_the_handlers_run` posts three handlers and calls `run()`
- **THEN** `run()` returns 3, all three ran and `io.stopped()` is true

#### Scenario: One handler at a time
- **WHEN** two handlers are posted and `run_one()` is called, then `run()`
- **THEN** `run_one()` returns 1 with one handler run, and `run()` returns 1

#### Scenario: A timer is waited for
- **WHEN** a `steady_timer` of 30 ms is armed and `run()` is called
- **THEN** `run()` returns 1 and the timer's handler saw no error

#### Scenario: The timed forms return at their deadline
- **WHEN** a 10 s timer is pending and `run_for(50ms)`, `run_until(now + 50ms)`, `run_one_for(50ms)` and `run_one_until(now + 50ms)` are called in turn
- **THEN** each returns 0 and the context is not stopped; after `later.cancel()`, `run()` returns 1 and the handler saw `operation_aborted`

#### Scenario: Stop and restart
- **WHEN** the first of two posted handlers calls `io.stop()`, `run()` returns, then `io.restart()` and `run()` are called
- **THEN** the first `run()` returns 1 with `io.stopped()` true, the second returns 1, and both handlers ran

### Requirement: Run: On Emscripten run is a composition of Boost's calls
On Emscripten the six members SHALL hide Boost's (which are not virtual) and `run()` MUST repeat four steps: (1) call `poll_one()` until it returns 0 or `max_handlers_per_turn` handlers have run; (2) return if the io_context is stopped; (3) if the turn reached the limit, yield to the browser and go to step 1; (4) otherwise call Boost's `run_one()`, which suspends in the reactor's `select` until a timer, a post or I/O makes a handler ready and runs it, returning if it ran none; `run_for` and `run_until` are the same with Boost's `run_one_until` in step 4, `run_one` is step 4 alone, and `run_one_for` and `run_one_until` are Boost's, running one handler at most.

#### Scenario: The yield count of a self-posting handler
- **WHEN** `turn_limit_test`'s `run_yields_every_turn` runs a handler that posts itself 5000 times under `run()` with the default limit
- **THEN** `run()` returns 5000, the longest run of handlers between two yields is 100 and the yield counter reads 49

#### Scenario: run_for yields the same way
- **WHEN** `run_for_yields_every_turn` sets the limit to 250 and runs 5000 self-posts under `run_for(10s)`
- **THEN** `run_for` returns 5000, the longest run is 250 and the yield counter reads 19

### Requirement: Run: The yield is a MessageChannel macrotask
The yield SHALL be the `EM_ASYNC_JS` import `webcpp_asio_yield`, which awaits a promise that a message on a `MessageChannel` resolves (closing the port first), so that it is a macrotask and never a microtask, and the browser renders, runs its timers and delivers its events before the next turn.

#### Scenario: A page timer runs at a yield
- **WHEN** `the_page_s_timer_runs_at_a_yield` arms a JavaScript `setTimeout(0)`, sets the limit to 250 and runs a self-posting handler under `run_for(10s)` until the timeout has run
- **THEN** the first handler that sees the timeout run is the first handler of a turn (`seen_since` is 1), at least one yield has happened and no turn exceeded 250 handlers

### Requirement: Run: A turn is bounded, and the bound between two yields is stated
The run family SHALL run at most `max_handlers_per_turn` handlers in one turn; step 4's single handler belongs to no turn, so that up to `k + 1 + max_handlers_per_turn` handlers, fewer than twice the limit and one, can run between two yields when a short turn of `k` handlers is followed by a wait that ends at once; the owner selected the default of 100 after the 2026-10-15 yield-cost measurements. The guide SHALL retain the measured costs, methodology and responsiveness limits; a handler is never preempted by the turn limit.

#### Scenario: Boost's own run has no limit
- **WHEN** `boost_s_own_run_never_yields` runs 5000 self-posts through a `boost::asio::io_context&` reference to the context
- **THEN** Boost's `run` returns 5000, the yield counter reads 0, the longest run is 5000 and the page's timeout never ran

### Requirement: Run: A deadline is checked before each handler
Each turn SHALL check the deadline before each handler (as Boost's `run_until` does), so that a deadline already passed runs nothing, and a `run_for` or `run_until` over handlers that are always ready MUST return once the deadline has passed even though every turn reaches the limit.

#### Scenario: A passed deadline runs nothing
- **WHEN** three handlers are posted and `run_for(0ms)` and `run_until(now - 1s)` are called, then `run()`
- **THEN** both timed forms return 0 with no handler run, and `run()` returns 3

#### Scenario: A deadline ends an always-ready run
- **WHEN** a handler posts itself without end, the limit is 100, and `run_for(100ms)` is called
- **THEN** it returns the number of handlers run (more than 0, equal to the handler's own count) after at least 100 ms and less than 2 s, and a following `run_until(now + 50ms)` also runs some

### Requirement: Run: A stop at the limit returns at once and a limit of zero still runs
A `stop()` made by the handler that reaches the limit SHALL make the run family return at once without yielding first, and a limit of 0 MUST be taken as 1 so that the run family never yields forever without running anything.

#### Scenario: A stop at the limit
- **WHEN** the limit is 10 and a handler stops the context at its tenth run and posts itself again
- **THEN** `run()` returns 10, the handler ran 10 times and the yield counter reads 0

#### Scenario: A limit of zero
- **WHEN** the limit is 0 and a handler posts itself until its 50th run
- **THEN** `run()` returns 50

### Requirement: Run: A context with a connection open and nothing pending returns
`run` SHALL return when no operation is pending, as natively, even with a connection open: a listener registered with the bridge is no outstanding work.

#### Scenario: A listener and nothing pending
- **WHEN** `bridge_test`'s `run_returns_with_a_listener_and_nothing_pending` adds a registration and calls `run()`
- **THEN** `run()` returns 0; after the registration's function is called with 3, `restart()` and `run()` return 1 with `last` equal to 3

### Requirement: Run: After main returns, nothing runs without a call
After `main` returns on Emscripten the runtime SHALL stay alive (`EXIT_RUNTIME` is off by default), events keep being posted, and nothing runs them unless an export listed for suspension calls the run family again.

#### Scenario: The page documents it
- **WHEN** `doc/run.adoc` is read
- **THEN** its section "Before run, and after main" states it

### Requirement: Run: The run family is called on main's stack or on an export listed for suspension
The run family SHALL be called on the stack of `main`, or on the stack of another export the program lists for suspension (`-sJSPI_EXPORTS` under JSPI, whose call then returns a Promise, and `ccall(..., {async: true})` under Asyncify); anywhere else its first suspension fails (a `SuspendError` under JSPI; under Asyncify an unwind that overwrites `Asyncify.currData` and corrupts a suspended `main`), and a program MUST NOT block the page's thread without suspending, in a busy loop or a blocking system call; the documentation names both.

#### Scenario: The page states where to call it
- **WHEN** `doc/run.adoc`'s section "Who may call it" (`asio-run-callers`) is read
- **THEN** it names `main`, `-sJSPI_EXPORTS`, `ccall(..., {async: true})`, the `SuspendError` and the corrupted `main`, and that nothing calls `run`, `poll` or `dispatch` from a JavaScript event

### Requirement: Run: The library never calls the run family
A library built on webcpp::asio SHALL be given an executor and never call `run`, `run_one` or `run_for` on an `io_context`; a program's own call MUST say why after `lint-run:` (`io.run();  // lint-run: the program's event loop, suspended into the browser's`), and xactor's `asio_driver`, which drains an `io_context{1}` of its own with `poll`, stays safe because that context makes no reactor.

#### Scenario: The lint's rule is unchanged
- **WHEN** a program's line calls `io.run()` without a `lint-run:` reason
- **THEN** the superproject's lint refuses it

### Requirement: Detection: A nested run family call is refused
On Emscripten each of the six members SHALL admit its call once, first, before any work; a call made while another call of the same io_context is active (a nested call from a handler) MUST trip `BOOST_ASSERT_MSG` and, with assertions off, return 0 and run nothing, the run around it going on.

#### Scenario: A nested run_for from a handler
- **WHEN** `detection_test`'s `a_nested_run_is_refused` posts a handler that calls `io.run_for(100ms)` and a second handler, with the assertion handler counting
- **THEN** the outer `run()` returns 2, the nested call returned 0, the second handler ran, and `seen.failed` grew by 1 (0 with `BOOST_DISABLE_ASSERTS`)

#### Scenario: Every member in turn
- **WHEN** `every_nested_member_is_refused_in_turn` posts handlers that call `run_one`, `run_for`, `run_until`, `run_one_for` and `run_one_until` and a last handler
- **THEN** `run()` returns 6, each nested result is 0, the last handler ran and the assertions grew by 5 (0 with assertions off)

### Requirement: Detection: A call from a browser event is refused
A call of the run family made while the bridge delivers an event, or made from a JavaScript event while `run` waits, MUST be refused as a nested call is, and the bridge's `delivering()` MUST be what tells.

#### Scenario: A run from a setTimeout while run waits
- **WHEN** `a_call_from_an_event_while_run_waits_is_refused` holds a work guard, a JavaScript `setTimeout(0)` calls an export that calls `io->run()` and then posts a handler resetting the guard, and `io.run()` is called
- **THEN** the outer `run()` returns 1, the call from the event returned 0 and the assertions grew by 1 (0 with assertions off)

#### Scenario: A run from inside a delivery
- **WHEN** `a_call_while_the_bridge_delivers_is_refused` calls a bridge function whose registration calls `io.run()` while a handler is ready
- **THEN** the call returns 0, the ready handler did not run, the context is not stopped, the assertions grew by 1, and a later `run()` returns 1 having run the handler

### Requirement: Detection: Each member admits once and calls only private code
Each public member of the run family SHALL admit exactly once and then call only private unchecked code (`run_until_unchecked`, `compose`, `drain`) or Boost's own, never another public member, because its admission would refuse the call its caller flagged.

#### Scenario: The timed forms are not refused
- **WHEN** `the_timed_forms_are_not_refused` runs 100 posts under `run_for(5s)` with a limit of 10, a 20 ms timer under `run_for(5s)`, and one post each under `run_until`, `run_one_for` and `run_one_until`
- **THEN** they return 100, 1, 1, 1 and 1, 104 handlers ran in all and no assertion failed

### Requirement: Detection: A flag scope puts back the value it found
`webcpp::asio::detail::flag_scope` SHALL set a `bool` flag for its lifetime and put back the value the flag had when the scope ends, however it ends (a C++ exception included), and MUST be neither copyable nor movable, so that an inner scope never clears what an outer one holds.

#### Scenario: Inner and outer scopes
- **WHEN** `flag_scope_test` makes an outer scope and an inner one on the same flag
- **THEN** the flag is set inside both, still set after the inner ends, and cleared after the outer ends

#### Scenario: An exception puts the flag back
- **WHEN** a `std::runtime_error` is thrown inside a scope
- **THEN** after the `catch` the flag is false (with exceptions)

### Requirement: Detection: A C++ exception out of run clears the flag
A handler's C++ exception that leaves `run` SHALL clear the run family's flag on its way, so that the program's `restart(); run();` after its `catch` is no nested call.

#### Scenario: Run again after a catch
- **WHEN** `an_exception_out_of_run_clears_the_flag` lets a 20 ms timer's handler throw, catches it, posts a handler, calls `restart()` and `run()`
- **THEN** the exception was caught, `run()` returns 1, the handler ran and no assertion failed

### Requirement: Detection: What it does not detect is documented
Four calls SHALL go undetected and the documentation MUST say so: a call from an export of the program's own that is not listed for suspension, a call through a `boost::asio::io_context&`, `poll`, `poll_one` and `dispatch` called from a JavaScript event, and a call on another io_context whose flag and bridge are its own.

#### Scenario: A reactor-less context polled from a handler
- **WHEN** `a_reactor_less_context_polled_from_a_handler` posts a handler to a `boost::asio::io_context driver{1}` and, from a handler of the webcpp context, calls `driver.poll()`
- **THEN** it returns 1, the driven handler ran and no assertion failed (xactor's `asio_driver` is safe on both variants)

### Requirement: Detection: A JavaScript error out of a handler leaves the flag set
A JavaScript error that a handler lets escape SHALL NOT be repaired by asio: where the build skips destructors (JavaScript exceptions, no exceptions) the run family's flag stays set so that every later call asserts and returns 0, which reports the broken context where a `run` that never returns would not; the documentation MUST say that a handler which calls JavaScript that may throw catches the error in JavaScript.

#### Scenario: The page states what is left
- **WHEN** `doc/browser.adoc`'s paragraph `asio-browser-errors` is read
- **THEN** it names Boost's work count kept, `running_in_this_thread()` true outside any `run`, and the flag set, with the measurement on Emscripten 6.0.11 in both variants with exceptions and without

### Requirement: Bridge: The bridge is a service of the io_context
`webcpp::asio::browser::bridge` SHALL be a `boost::asio::execution_context::service` of the io_context, made on first use by `boost::asio::use_service<bridge>(io)` with the context's allocator, shut down and destroyed with the context, on Emscripten alone, with a static `id` and a constructor taking the `execution_context&`; it MUST be neither copyable nor movable and its registrations MUST be held in a `boost::unordered_flat_map<std::uint64_t, registration*>` allocated with the context's allocator.

#### Scenario: The io_context reaches its bridge
- **WHEN** a program calls `boost::asio::use_service<webcpp::asio::browser::bridge>(io)` twice on one context
- **THEN** both calls return the same bridge, and the io_context's constructor holds the reference it looked up once

### Requirement: Bridge: A registration is what an event reaches
`webcpp::asio::browser::registration` SHALL be an abstract class with `virtual bool deliver(const emscripten::val& event) = 0`, which posts what the event completes or notifies to the io_context and never polls, runs or dispatches, returning whether the registration is done so that the bridge removes it, and `virtual void shutdown() noexcept = 0`, which destroys what is pending without calling it; its constructor and destructor are protected, and its owner allocates it as it chooses, adds it, and removes it before destroying it.

#### Scenario: A registration that is done is removed
- **WHEN** `a_done_registration_is_removed` adds a counter that is done after 2 deliveries and its function is called with 1, 2 and 3
- **THEN** `run()` returns 2, two events were received and the last is 2

### Requirement: Bridge: Keys are never reused
`bridge::add(registration&)` SHALL return a `std::uint64_t` key, the first being 1, counted up and never given twice, which JavaScript holds as a `BigInt`; `bridge::remove(key) noexcept` MUST remove the registration and ignore a key already removed.

#### Scenario: A later key is greater
- **WHEN** `late_events_find_nothing` adds a registration, removes it, and adds another
- **THEN** the second key is greater than the first

### Requirement: Bridge: Each key has a JavaScript function that only posts
`bridge::function(key)` SHALL return a JavaScript function that delivers its argument to the registration of the key; the delivery looks the key up and posts, an event that arrives while no `run` is active is posted and waits for the next one, and a registered listener MUST NOT be outstanding work.

#### Scenario: An event posts and wakes run
- **WHEN** `an_event_posts_and_wakes_run` holds a work guard, a JavaScript `setTimeout` of 20 ms calls the key's function with 7, and the posted handler resets the guard, then `io.run()`
- **THEN** `run()` returns 1, one event was received with the value 7, the registration saw `delivering()` true and the bridge is not delivering after

#### Scenario: Events before run wait for it
- **WHEN** a registration's function is called with 1 and 2 before `run`
- **THEN** nothing was received until `io.run()` returns 2 with 2 received, and `bridge.remove(key)` succeeds

### Requirement: Bridge: Late events find nothing
An event of a key that was removed, or of a bridge whose io_context was destroyed, SHALL deliver nothing and corrupt nothing: each function carries its key and a token that the bridge clears at its shutdown and destruction, and an event after the io_context was destroyed MUST NOT reach WebAssembly at all; the context's shutdown calls the `shutdown` of each registration still held and not of one removed.

#### Scenario: A stale function and an orphan
- **WHEN** `late_events_find_nothing` calls a removed key's function, destroys the io_context and calls the orphaned function of a held key
- **THEN** nothing was received, the held registration was shut down and the removed one was not, and another context still runs

### Requirement: Bridge: An exception that escapes a delivery is reported and the stack restored
An exception that escapes a delivery, a C++ exception from a registration's `deliver` or a JavaScript error from a function it calls, SHALL be caught at the bridge's boundary and reported with `reportError` (or `console.error` where there is none), the event being lost; a C++ exception is caught in C++, which frees its object; the JavaScript side MUST put WebAssembly's stack pointer back (`stackSave`, `stackRestore`), put the count of deliveries back and free the event's handle, which C++ only borrows.

#### Scenario: 3,000 C++ exceptions
- **WHEN** `an_escaping_exception_is_reported_and_the_stack_restored` calls a throwing registration's function 3,000 times from JavaScript
- **THEN** 3,000 reports are counted, the stack pointer and the count of Emval handles are what they were, and the bridge is not delivering (with exceptions)

#### Scenario: 3,000 JavaScript errors
- **WHEN** `a_javascript_error_is_reported_and_the_stack_restored` does the same with a registration that calls a JavaScript function that throws
- **THEN** the same four conditions hold

### Requirement: Bridge: delivering() reads a count that survives any error
`bridge::delivering() const` SHALL return true while a function of the bridge delivers, an outer delivery around a nested one included, by reading the count of deliveries in progress that the bridge's JavaScript functions keep and put back in a `finally`, so that nothing in C++, whose destructors a JavaScript error may skip, keeps it.

#### Scenario: A nested delivery leaves the outer one delivering
- **WHEN** `a_nested_delivery_leaves_the_outer_one_delivering` has a registration dispatch an event to another registration's function
- **THEN** the inner saw `delivering()` true, the outer still saw it true after the inner returned, and the bridge is not delivering afterwards; `a_nested_error_leaves_the_outer_delivery_delivering` holds when the inner throws a JavaScript error (one report)

### Requirement: Bridge: A trap and the runtime's own control flow pass through
A `WebAssembly.RuntimeError`, which Emscripten's `abort()` throws too, SHALL go on to the page after the stack pointer is put back, and the `ExitStatus` that `exit()` throws and Emscripten's `'unwind'` MUST end the delivery without a report; and once the runtime aborted (`ABORT` is true) the functions MUST enter nothing.

#### Scenario: Three kinds of escape
- **WHEN** `a_trap_escapes_and_the_runtime_s_own_pass_unreported` calls three functions that throw a `WebAssembly.RuntimeError`, `'unwind'` and `new ExitStatus(0)`
- **THEN** the caller sees the trap only (results 1, 0, 0), no report was made, the stack pointer and handle count are unchanged and the bridge is not delivering

#### Scenario: An event after an abort
- **WHEN** `bridge_abort_test` calls a bridge function after `abort()` was caught and before `ABORT` is reset
- **THEN** the registration saw one delivery (the control) and still one after the aborted call

### Requirement: Bridge: deliver and delivering cannot be reached around
`bridge::deliver` SHALL be private, with the export `webcpp_asio_deliver` as its only friend and caller, and `delivering()` MUST take no value, so that through the bridge a program can neither deliver outside the count the JavaScript functions keep nor change what `delivering()` reads.

#### Scenario: Two refusals and a control
- **WHEN** `bridge_private_test.cpp` is compiled plain, with `WEBCPP_TEST_ASIO_DELIVER` (a call of `bridge.deliver(1, val::undefined())`) and with `WEBCPP_TEST_ASIO_SET_DELIVERING` (a call of `bridge.delivering(false)`)
- **THEN** the plain build compiles (`bridge_private`) and the other two do not (`bridge_deliver_refused`, `bridge_set_delivering_refused`)

### Requirement: Bridge: The event is borrowed, never destroyed twice
The export `webcpp_asio_deliver(void* bridge, std::uint64_t key, EM_VAL event)` SHALL wrap the event handle in a `detail::borrowed_val` union that never destroys the `val` (so that neither the delivery's end nor an unwinding that runs destructors frees the handle twice), catch a `std::exception` or anything else when exceptions are enabled and report it with `webcpp_asio_report`, and be defined once in the one translation unit that includes `browser/src.hpp`, never inline.

#### Scenario: The export is the bridge's friend
- **WHEN** `bridge.hpp` and `src.hpp` are read
- **THEN** `bridge` declares `friend void ::webcpp_asio_deliver(void*, std::uint64_t, emscripten::EM_VAL)` and `src.hpp` defines it with `extern "C" EMSCRIPTEN_KEEPALIVE`

### Requirement: Bridge: The JavaScript functions are declared once and loaded before main
`detail/browser_imports.hpp` SHALL declare, with `EM_IMPORT`, `webcpp_asio_yield`, `webcpp_asio_function`, `webcpp_asio_await`, `webcpp_asio_string` and `webcpp_asio_report` and the export `webcpp_asio_deliver`; `browser/src.hpp` MUST define them with `EM_JS` and `EM_ASYNC_JS`, declare the runtime symbols they need with `EM_JS_DEPS` (`$Emval`, `_emval_decref`, `$stackSave`, `$stackRestore`, `$UTF8ToString`, `$ExitStatus`), and a test MUST load every one so that a function that does not parse fails before `main`.

#### Scenario: Every EM_JS parses
- **WHEN** `em_js_test.cpp` runs, calling `webcpp_asio_string("asio, more", 4)` and the test helpers
- **THEN** the string is "asio" and no report was made

### Requirement: Exceptions: A handler's exception propagates out of run on both variants
A handler's C++ exception SHALL propagate out of `run`, as Boost's, on both variants, after any number of suspensions and yields, because handlers run on `main`'s stack inside the run family call; a `try` around `run` catches what a handler throws after `run` suspended, and a `run` called inside a `catch` block suspends and resumes there.

#### Scenario: A try encloses a suspended run
- **WHEN** `exceptions_test`'s `a_try_encloses_a_suspended_run` runs a 20 ms timer under a `try { io.run(); } catch (...)`
- **THEN** the timer fired and nothing was caught

#### Scenario: A catch block suspends
- **WHEN** `a_catch_block_runs_and_suspends` calls `io.run()` inside a `catch` block that handled a `std::runtime_error`
- **THEN** the timer fired and the program resumed in the block

#### Scenario: A handler's exception after a suspension
- **WHEN** a 20 ms timer's handler throws `std::runtime_error("handler")`
- **THEN** `run` throws it to `main` (message "handler") and, after `restart()`, the next `run()` returns 1 having run its handler

#### Scenario: A throw after two yields
- **WHEN** 2,000 handlers and then a throwing handler are posted with the default limit
- **THEN** the exception reaches `main`, all 2,000 ran, and the yield counter reads 2 on Emscripten (0 natively)

### Requirement: Exceptions: Each variant compiles with the exception model it can suspend through
The `jspi` variant SHALL use WebAssembly exceptions and the `asyncify` variant JavaScript-based exceptions, because Asyncify traps with `RuntimeError: unreachable` when it suspends inside a `catch` block of WebAssembly exceptions; the `exceptions` tests MUST be built with exceptions on whatever the build asks, natively, on `jspi` and on `asyncify`.

#### Scenario: The exceptions test runs three ways
- **WHEN** `test/Jamfile` is read
- **THEN** it declares `webcpp.run exceptions` for native and emscripten with `<exception-handling>on`, and `exceptions_asyncify` with `<webcpp-suspend>asyncify`, the latter in the lane `asyncify`

### Requirement: Exceptions: Every browser test runs in all four builds
Every browser test (`reactor`, `run_family`, `turn_limit`, `bridge`, `bridge_abort`, `em_js`) SHALL be declared in four builds, `jspi` and `asyncify`, each with exceptions on and off, the detection test in all four with the assertion handler and with `BOOST_DISABLE_ASSERTS`, and the Asyncify builds MUST join the lane `asyncify`.

#### Scenario: The Jamfile's twins
- **WHEN** `test/Jamfile` is read
- **THEN** each browser test has `<name>`, `<name>_no_exceptions`, `<name>_asyncify` and `<name>_asyncify_no_exceptions`, and `detection_<variant>_exceptions_<on|off>` with a `_disabled` twin

### Requirement: Stack: Asyncify's stack is measured, not guessed
`-sASYNCIFY_STACK_SIZE` SHALL be 8192 unless a program raises it, which is twice 4096, the smallest power of two with which every test passed in every build b2 links (measured 2026-10-11 with Emscripten 6.0.11: the deepest suspension, in a release build compiled with `-O3` and linked without optimization as b2's emscripten toolset links it, wrote 3,540 bytes, against 1,272 in a debug build; Emscripten's default of 4096 left it 556 bytes and 2048 trapped), so that half of the stack is left to the program's own frames.

#### Scenario: Every suspension leaves half the stack
- **WHEN** `asyncify_stack_test` measures `run`, `run_one`, `run_for`, `run_until`, `run_one_for` and `run_one_until` at the reactor's wait, `run`, `run_for` and `run_until` at the yield, and `run` inside a `try` and inside a `catch` block (with exceptions)
- **THEN** each reading finds at least one unwind and more than 0 bytes, and twice the deepest is at most the stack size; hand-built at `-O0` with `-sASYNCIFY_STACK_SIZE=8192` on 2026-10-13 the deepest were 1,104 bytes (`run_for`, the reactor's wait) and 324 (`run`, the yield)

### Requirement: Stack: The measurement is exact and covers every build
The bytes each unwind wrote SHALL be read from Asyncify's own data when the unwind stops (its position less the stack's start), never inferred from whether a smaller stack traps, in debug, in release and in release linked with `-O3`, each with exceptions and without (`asyncify_stack_<build>_exceptions_<on|off>`), and a reading that finds no unwind or no byte written MUST fail.

#### Scenario: Six builds are declared
- **WHEN** `test/Jamfile` is read
- **THEN** `asyncify_stack_debug_exceptions_on`, `_off`, `asyncify_stack_release_...` and `asyncify_stack_optimized_link_...` are declared, the last with `<variant>release <linkflags>-O3`, and they join the lane `asyncify`

#### Scenario: A delayed entry still measures the blocking wait
- **WHEN** a reactor-wait case schedules a JavaScript macrotask, keeps outstanding Asio work, and delays entry to the run family by 25 ms
- **THEN** the macrotask posts the handler that releases the work, and the measurement observes a nonzero-timeout poll as well as an unwind; the initial nonblocking poll alone cannot satisfy the case

### Requirement: Stack: An overflow traps and a program may ask for more
A suspension that does not fit SHALL trap with `RuntimeError: unreachable` when the unwind stops, and nothing of the program runs afterwards; a program that calls `run` deep in its code asks for more with `<asyncify-stack-size>` or `asyncify-stack-size=<n>` on b2's command line, a value being a power of two from 8192 to 1048576.

#### Scenario: A 64-byte stack traps
- **WHEN** `exceptions_asyncify` is linked with `-sASYNCIFY_STACK_SIZE=64` (the plan's red step)
- **THEN** it traps at the first suspension

### Requirement: Testing: A test ends as its main ends
Every test's `main` SHALL end with `return webcpp::test::exit_status(boost::report_errors());` from `<webcpp/test/exit_status.hpp>`, which natively returns its status and on Emscripten hands the status to the test support (`Module['webcppTestFinished']`) without calling `std::exit`; the support (`node-pre.js`) MUST fail a test whose `main` did not finish with the line `webcpp: main did not finish: ...` and status 1, keep the first status it is given, and stop a test past its bound (`webcpp::test::bound`, 120 s by default) with `webcpp: the test ran past its bound of <n> s`.

#### Scenario: The statuses of the fixture
- **WHEN** `emscripten_test.py`'s `test_a_test_ends_as_its_main_ends` runs the eight programs of `node_test_demo`
- **THEN** a `main` that returns without suspension passes, one that suspends and `exit_status(0)` passes on both variants, a `main` that suspends and returns 1 gives status 1 under JSPI, one that suspends and returns 0 under Asyncify gives status 1 with the line "main did not finish", `never_finishes` gives 1 after its bound of 3 s, and an `exit_status(3)` inside a coroutine (Asyncify) or a handler (JSPI) gives 3

### Requirement: Testing: The two false passes are pinned in each variant
`never_finishes_test` and `return_after_suspension_test` SHALL be `run-fail` tests in both variants: a `main` that never finishes (a `run` held by a work guard that nothing releases) and a `main` that suspended on a timer and returns 1 without `exit_status`, whose 1 Asyncify loses and the support turns back into 1; each program's `.output` shows the status it gave and the support's line.

#### Scenario: Four run-fail tests
- **WHEN** `test/Jamfile` is read
- **THEN** `never_finishes`, `never_finishes_asyncify`, `return_after_suspension` and `return_after_suspension_asyncify` are `webcpp.run-fail`, the Asyncify ones in the lane

### Requirement: Testing: The turn limit is tested by counting yields, not by racing a timer
Tests of the turn limit SHALL count the run family's yields with `webcpp::test::yield_counter`, which counts every `MessagePort.postMessage` the page makes from its construction, and assert where a `setTimeout(0)` runs (at a yield, via `webcpp::test::page_timeout`), never after how many handlers, because when a `setTimeout(0)` runs is the host's (node delays it by 1 ms, a browser clamps a nested one to 4 ms and throttles a background tab's to 1 s or more).

#### Scenario: Natively there is no page
- **WHEN** `yield_counter` is used in a native test
- **THEN** its `count()` is 0

### Requirement: Testing: The lanes
asio SHALL be proven in four lanes, each built from scratch with `b2 -a` and a `--build-dir` of its own: native (`b2 libs/asio/test libs/asio/example`), emscripten in the default variant (`b2 toolset=emscripten libs/asio/test libs/asio/example`), the own lane `asyncify` (`b2 toolset=emscripten libs/asio/test//asyncify libs/asio/example//asyncify`, every browser test again under Asyncify and the size table) and, from task 12, the own lane `browser` (`b2 toolset=emscripten libs/asio/test/browser//browser`); `b2 libs/asio/test//config` builds and runs one test alone.

#### Scenario: The lane is declared
- **WHEN** `test/Jamfile` ends
- **THEN** it holds `webcpp.lane asyncify : $(.asyncify) : emscripten ;`

#### Scenario: Recorded results at the tip
- **WHEN** the ledger of the plan's execution is read after task 9's fix round
- **THEN** it records native 13 and gcc-15 13 tests, JSPI 37, no-exceptions 37, Asyncify 29 (and release, and release linked with -O3), the page 67 and the lint clean, which this proposal did not re-run through b2

### Requirement: Testing: The documentation is a page with a generated reference
The page `doc/asio.adoc` SHALL include `overview.adoc`, `names.adoc`, `building.adoc`, `run.adoc`, `browser.adoc` and `testing.adoc`, then a generated reference built by MrDocs from the Doc Comments of the headers read for Emscripten's target (`webcpp.reference asio : <library>/webcpp//emscripten-reference <define>BOOST_ASIO_DISABLE_THREADS`), every public symbol documented with a brief, `@param`, `@tparam` and `@return`; the page states the test counts through `{n-tests}`, `{n-tests-native}` and `{n-tests-emscripten}`.

#### Scenario: The page is built
- **WHEN** `b2 libs/asio/doc` runs
- **THEN** it builds `doc/html/index.html` with no finding of the doc check (the ledger records "doc 67" green at the tip)

### Requirement: Timers: Boost's timers fire in the reactor's queue on both targets
Three `steady_timer`s of 60, 20 and 40 ms SHALL complete in the order 20, 40, 60; a `system_timer` of 30 ms completes; a timer cancelled before its expiry completes with `boost::asio::error::operation_aborted`; a per-operation cancellation through `boost::asio::bind_cancellation_slot` (a `cancellation_signal` emitted with `cancellation_type::terminal` from a 10 ms timer's handler) completes a 10 s wait with `operation_aborted`; the reactor caps one wait at five minutes, so an idle program holds one `setTimeout` of at most 300 s; the test `timers` runs natively and joins the browser tests.

#### Scenario: Order and cancellation
- **WHEN** `timers_test` starts timers of 60, 20 and 40 ms and a 10 s wait cancelled by its slot
- **THEN** the completions arrive 20, 40, 60, and the cancelled wait completes with `operation_aborted`

### Requirement: Timers: Beast's timeouts are Boost timers and fire
Beast's timeouts SHALL fire in the browser's event loop on both variants because Beast's timers are Boost's (`basic_stream.hpp:251`, `websocket/impl/stream_impl.hpp:72`).

#### Scenario: A WebSocket handshake timeout
- **WHEN** `beast_s_handshake_timeout_fires` sets a handshake timeout of 100 ms on a `websocket::stream<test::stream>`, connects it to a peer and calls `async_accept`, then `io.run()`
- **THEN** the handler's error equals `boost::beast::error::timeout`

### Requirement: Allocation: The io_context allocates through the user's allocator
`io_context(std::allocator_arg, allocator, options)` SHALL pass the allocator to Boost's constructor, which allocates the context's services and its I/O objects' state with it; operations MUST allocate through their handler's associated allocator, as Boost's do, and the bridge's map through the context's.

#### Scenario: Everything goes through the counting allocator
- **WHEN** `allocation_test`'s `the_user_s_allocator_takes_everything` builds a context with `counted<void>`, arms a 5 ms timer, makes ten posts and (on Emscripten) a registration receiving two events, all bound with `bind_allocator(counted<void>(&given), ...)`, and runs it
- **THEN** `operator_new_calls`, `c_allocations` and `aligned_allocations` are 0 and `given` is above 0 (18 for the same scenario without the bridge, natively and on JSPI, in the plan's probe)

### Requirement: Allocation: The counters see an unbound handler and the runtime's own allocations are named
The counting test SHALL count `operator new` in all its forms and the C library's `malloc`, `calloc`, `realloc`, `aligned_alloc`, `posix_memalign` and `memalign`, with a control case that proves an unbound handler's allocation is seen (Asio's default handler memory ends in `aligned_alloc`, never in `operator new`); and the documentation MUST name, instead of tolerating, what the runtime allocates with its own `malloc`: Emscripten's `select` (a calloc of its poll array on every call), Asyncify's unwind data on every suspension, and `basic_resolver_results`' entries (`new`).

#### Scenario: The control case
- **WHEN** `the_counters_see_an_unbound_handler` posts one handler with no associated allocator outside `run`
- **THEN** `c_allocations + aligned_allocations` is above 0 and `operator_new_calls` is 0

### Requirement: Loader: A page loads the variant its browser runs
`include/webcpp/asio/webcpp-asio-loader.mjs` SHALL export `supportsJspi(scope = globalThis)` (true when `typeof scope.WebAssembly?.Suspending === 'function'`), `pick(manifest, jspi = supportsJspi())` returning `{variant, script}` and throwing "the manifest names no <variant> artifact" when the script is missing, and `async load(manifestUrl, options = {})` returning `{variant, module}`, whose options `jspi`, `module`, `fetch` and `importModule` replace the browser's own; it reads the names from the manifest `webcpp.variants` writes, never from a pattern.

#### Scenario: The loader under node
- **WHEN** `loader_test.mjs` runs against `page_program`'s manifest
- **THEN** `supportsJspi({WebAssembly: {Suspending: function () {}}})` is true and the other two scopes false, `pick(manifest, true)` gives `jspi` and `pick(manifest, false)` gives `asyncify`, a manifest without `asyncify` throws, and `load` of each variant runs the program, which prints `ran 3 handlers`

### Requirement: Browser: An idle program schedules nothing but the reactor's wait
In headless Chrome an idle program (a context held by a work guard that nothing releases) SHALL schedule no task and no timer beyond the reactor's one `setTimeout` whose delay is above 0 and at most 300,000 ms, counted through `setTimeout`, `setInterval` and `MessagePort.postMessage` wrappers installed before the module, over 3 s after the program printed `idle`.

#### Scenario: browser_idle
- **WHEN** the driver `run.mjs --mode idle` loads `idle_program` (twice, the second time with `WebAssembly.Suspending` deleted so that Asyncify runs)
- **THEN** exactly one `setTimeout` is recorded in the 3 s window and the page's `report.variant` says which variant ran

### Requirement: Browser: Rendering continues while a self-posting handler runs
A program that runs a self-posting handler for one second on a `webcpp::asio::io_context` SHALL let the page paint at least 10 animation frames, while the same program through `static_cast<boost::asio::io_context&>(io).run()` paints none; the driver MUST print `render <variant>: <frames> frames, <handlers> handlers in 1 s, <turns> turns, turn p95 <ms> ms, max <ms> ms`, and the work stops for the owner when the 95th percentile of a turn of 1,000 trivial handlers is above 8 ms in any of three runs.

#### Scenario: browser_render
- **WHEN** `run.mjs --mode render` runs `render_program` and `render_unlimited_program`
- **THEN** the limited one saw at least 10 frames and the unlimited one none

### Requirement: Browser: The loader picks the variant in Chrome
`run.mjs --mode loader` SHALL open `page_program` and expect the variant `jspi` and the line `ran 3 handlers`, then with `WebAssembly.Suspending` deleted expect `asyncify` and the same line; any `Runtime.exceptionThrown`, `console.error` or `report.error` MUST fail the mode, and a `SyntaxError` of an `EM_JS` fails it at load.

#### Scenario: browser_loader
- **WHEN** the driven test `browser_loader` runs in the lane `browser`
- **THEN** both variants load and print the line

### Requirement: Docs: The guarantees are numbered and the examples are twins
`doc/guarantees.adoc` SHALL state nine numbered guarantees with anchors `asio-invariant-1` to `asio-invariant-9` (1 natively every name is Boost's; 2 handlers run only on the stack of a run family call; 3 at most `max_handlers_per_turn` handlers run before a yield and a timed form returns at its deadline; 4 a delivered event only posts; 5 a key is never reused; 6 a nested or in-event call asserts and returns 0; 7 a handler's exception propagates; 8 timers are Boost's; 9 every allocation is the user's but the runtime's, named), tasks 17 to 20 adding 10 to 13; each example of the guide (`asio_quick_start`, `asio_turn_limit`, `asio_timers`, then `asio_cobalt`, `asio_promise`, `asio_event`, `asio_sockets`, `asio_resolver`) MUST print the same natively and in the browser and be built in both variants for the size table.

#### Scenario: The quick start's output
- **WHEN** `asio_quick_start` (a 50 ms `steady_timer` and a post) runs
- **THEN** it prints `posted`, `timer fired` and `run returned 2 handlers`, natively and on Emscripten, matching its `.expected`

#### Scenario: The turn-limit example
- **WHEN** `asio_turn_limit` runs with `io_context_options{.max_handlers_per_turn = 250}` and 5,000 self-posts
- **THEN** it prints `max_handlers_per_turn: 250` and `handlers run: 5000`

### Requirement: Cobalt: Boost.Cobalt's sources are compiled but main.cpp and io/ssl.cpp
The superproject SHALL provide the static target `/webcpp//boost_cobalt`, every `.cpp` of Boost.Cobalt's `src/`, `src/detail/` and `src/io/` but `main.cpp` (whose `run_main` webcpp defines) and `io/ssl.cpp` (departure 3: it needs OpenSSL), with `BOOST_COBALT_SOURCE`, `BOOST_COBALT_IO_SOURCE` and, on Emscripten, `BOOST_ASIO_DISABLE_THREADS`, from `-sBOOST_COBALT_SOURCE=<dir>` or `.local/boost-cobalt` (never the environment); a build that uses it and finds neither MUST stop, naming both places.

#### Scenario: Sources found and left out
- **WHEN** `jamroot_test.py` builds `/webcpp//boost_cobalt` with a directory holding `channel.cpp`, `main.cpp`, `detail/util.cpp`, `io/sleep.cpp` and `io/ssl.cpp`
- **THEN** exactly `channel.cpp`, `detail/util.cpp` and `io/sleep.cpp` are compiled, and for Boost 1.92 that is 24 sources per toolset

### Requirement: Cobalt: A co_main program changes nothing and gets the turn limit
`src/cobalt_main.cpp` SHALL define `boost::cobalt::detail::main_promise::run_main(::boost::cobalt::main)` and `final_suspend` with the same steps as Cobalt's (the executor, the signal set of `SIGINT` and `SIGTERM`, the post that resumes the coroutine) over a `webcpp::asio::io_context{BOOST_ASIO_CONCURRENCY_HINT_1}` and its run family, `/webcpp/asio//cobalt` MUST link `asio`, `cobalt-main` and `/webcpp//boost_cobalt`, and under Asyncify (`emscripten_has_asyncify() == 1`) `run_main` MUST end the program with `std::exit(res)` so that the `co_return` value is the exit status.

#### Scenario: The unchanged co_main
- **WHEN** `cobalt_test` runs a `co_main` of 5,000 `co_await post(exec, use_op)`, a 50 ms Cobalt timer wait and a 20 ms Asio timer wait, natively and on both variants
- **THEN** it passes, and on Emscripten the page's `setTimeout(0)` ran within 1,000 posts

#### Scenario: The co_return value is the status
- **WHEN** `cobalt_status_test`'s `co_main` waits 20 ms and does `co_return 7;`
- **THEN** as a `run-fail` test its `.output` says `EXIT STATUS: 7` natively and on both variants, and with the `std::exit` removed in a scratch copy under Asyncify it says `EXIT STATUS: 1`

### Requirement: Cobalt: The definition is kept honest and has a fallback
The definition reads `main_promise`'s members (`result`, `exec`, `exec_`, `signal_set`, `signal`), which are Boost's implementation, so compiling it against the pinned Boost on every lane MUST be the probe that fails when Boost changes them; if it cannot be kept, Cobalt's own `co_main` is the fallback, unchanged and without the turn limit; a program that uses `boost::cobalt::io::ssl_stream` does not link with it until Cobalt's TLS sources are built against the program's OpenSSL.

#### Scenario: Cobalt's own co_main works unchanged
- **WHEN** the probe `cobalt_main.cpp` of the review is built with Cobalt's own `main.cpp`
- **THEN** its `co_main` sleeps 150 ms on a timer and returns (JSPI), without the limit

### Requirement: Promise: A JavaScript promise is an Asio operation
`webcpp::asio::browser::async_await(const boost::asio::any_io_executor&, emscripten::val promise, CompletionToken&&)` SHALL initiate through `boost::asio::async_initiate` with the completion signature `void(boost::system::error_code, emscripten::val)`, so that every completion token works (a callback, `deferred`, `use_awaitable`, Cobalt's `use_op`), delivering through the bridge: the settlement posts the handler, never inline; a value that is no promise completes with itself.

#### Scenario: A resolved promise
- **WHEN** `a_resolved_promise_completes_with_its_value` awaits `test_promise_after(20, 42, false)` with a callback
- **THEN** the error code is success and the value is 42

#### Scenario: Every token
- **WHEN** the same promise is awaited with `boost::asio::deferred`, with `use_awaitable` in a `co_spawn`, and with Cobalt's `use_op` in a `co_main` (`promise_cobalt`)
- **THEN** each completes with the value

#### Scenario: Values at the boundary
- **WHEN** an already resolved promise, the plain value `emscripten::val(6)` and a rejection with `undefined` are awaited
- **THEN** the resolved one completes through a post (a flag set after the call returns is seen set), the plain value completes with 6, and the rejection completes with `promise_rejected` and `isUndefined()`

### Requirement: Promise: A rejection is an error of category webcpp.asio
A rejection SHALL complete with `webcpp::asio::browser::error::promise_rejected`, value 1 of the category named `webcpp.asio` (its `message(1)` being "the promise was rejected" and any other value "unknown webcpp.asio error"), never reused, with the rejection's reason as the value; `browser/error.hpp` MUST provide `errors`, `category()`, `make_error_code` and the `is_error_code_enum` specialization.

#### Scenario: A rejected promise
- **WHEN** `a_rejected_promise_completes_with_promise_rejected` awaits `test_promise_after(20, 9, true)`
- **THEN** `ec == promise_rejected`, `ec.category().name() == "webcpp.asio"`, `ec.value() == 1` and the value is 9

### Requirement: Promise: Cancellation completes at once and a pending promise is outstanding work
A `terminal`, `partial` or `total` cancellation SHALL complete the operation at once with `operation_aborted` and drop the registration, so that a later settlement finds nothing; a promise that never settles MUST keep `run` from returning until it is cancelled; destroying the io_context MUST destroy the pending handler without calling it, and a settlement after the destruction MUST find nothing.

#### Scenario: Cancelling a slow promise
- **WHEN** a promise settling after 1 s is awaited and a `cancellation_signal` is emitted from a 20 ms timer's handler, for each of the three types
- **THEN** the operation completes with `operation_aborted` and `run` returns before 500 ms

#### Scenario: A promise that never settles
- **WHEN** `new Promise(() => {})` is awaited and `run_for(200ms)` is called
- **THEN** it returns 0 with the context not stopped; after cancellation `run` returns

#### Scenario: Destruction with a pending promise
- **WHEN** a handler holding a `shared_ptr` awaits a pending promise and the io_context is destroyed
- **THEN** the use count is back to 1 and the handler was not called, and another context's 100 ms timer runs without a crash

### Requirement: Promise: The operation allocates through the handler's allocator and needs embind
The registration and the operation's state SHALL use the handler's associated allocator, taken before the handler moves and the operation ends, and a program using the promise operations links embind (`-lembind`, in the usage requirements).

#### Scenario: Still no operator new
- **WHEN** `allocation_test` awaits a resolved and a rejected promise with handlers bound to `counted<void>`
- **THEN** `operator_new_calls` stays 0

### Requirement: Event: A browser event is an Asio operation
`webcpp::asio::browser::async_wait_event(const boost::asio::any_io_executor&, emscripten::val target, std::string_view type, CompletionToken&&)` SHALL add a listener for `type` on the DOM `EventTarget` with `once: true` and complete with the next event (signature `void(boost::system::error_code, emscripten::val)`), the event type being made a JavaScript string from the view without a `std::string`.

#### Scenario: An event completes with itself
- **WHEN** `an_event_completes_with_itself` waits for `message` on an `EventTarget` that a `setTimeout` of 20 ms dispatches with detail 5
- **THEN** the error code is success and `event["detail"]` is 5

#### Scenario: Only the next event
- **WHEN** two events are dispatched 20 ms apart and one wait is made
- **THEN** the second is not delivered to it but is received by a second wait

#### Scenario: A type given as a view
- **WHEN** the type is `std::string_view("messages and more", 7)`
- **THEN** the event `message` completes it

### Requirement: Event: Cancellation and destruction remove the listener
A cancellation SHALL call `removeEventListener`, drop the registration and complete with `operation_aborted`; the shutdown of the io_context MUST clear the cancellation slot, remove the listener too and destroy the handler without calling it.

#### Scenario: A cancelled wait leaves no listener
- **WHEN** a wait is cancelled with each of `terminal`, `partial` and `total`, and the target's listener count is read through `test_listener_count`
- **THEN** the wait completed with `operation_aborted`, the count is 0 and a later dispatch reaches nothing

#### Scenario: A wait pending at destruction
- **WHEN** the io_context is destroyed with a wait pending
- **THEN** the handler was not called, the listener count is 0 and a dispatch afterwards reaches nothing

### Requirement: Socket: Sockets and the acceptor fail as values on Emscripten
On Emscripten `webcpp::asio::ip::tcp` and `ip::udp` SHALL be protocol classes of webcpp's own with Boost's members (`v4()`, `v6()`, `type()`, `protocol()`, `family()`, `endpoint` as `boost::asio::ip::basic_endpoint<tcp>`, `operator==`), and `basic_stream_socket<Protocol, Executor>`, `basic_datagram_socket<Protocol, Executor>` and `basic_socket_acceptor<Protocol, Executor>` MUST be webcpp's own class templates over `browser::transport<Protocol>`, whose primary template completes every operation with `boost::asio::error::operation_not_supported`; natively `ip::tcp`, `ip::udp` and the three templates are aliases of Boost's (`names_test` asserts it).

#### Scenario: Every operation fails and returns at once
- **WHEN** `sockets_test` calls each operation of `ip::tcp::socket`, `ip::udp::socket` and `ip::tcp::acceptor` in its `error_code&` form
- **THEN** each gives `operation_not_supported` and returns at once, never blocking

#### Scenario: Asynchronous operations complete through a post
- **WHEN** each asynchronous operation is initiated with a callback and with `boost::asio::deferred`
- **THEN** it completes with `operation_not_supported` through a post (a flag set after the call returns is seen set by the handler)

#### Scenario: Open and close succeed
- **WHEN** `open` and `close` are called, and `open` a second time
- **THEN** the first two succeed, `is_open` follows them and the second `open` gives `already_open`

#### Scenario: The throwing forms
- **WHEN** a throwing form is called (with exceptions)
- **THEN** it raises, through `boost::throw_exception`, a `boost::system::system_error` whose code is `operation_not_supported`

### Requirement: Socket: The subset of Boost's interface is reproduced with Boost's signatures
The socket templates SHALL provide constructors (from an executor or an `ExecutionContext&`, with a protocol or an endpoint, the endpoint form behind `#ifndef WEBCPP_ASIO_NO_EXCEPTIONS`), `get_executor`, `open`, `is_open`, `close`, and through the transport `cancel`, `shutdown`, `connect`, `async_connect`, `read_some`, `async_read_some`, `write_some`, `async_write_some`, `send`, `async_send`, `receive`, `async_receive`, `set_option`, `get_option`, `local_endpoint`, `remote_endpoint`, `available` and both forms of `non_blocking`; the datagram socket `bind`, `send_to`, `async_send_to`, `receive_from` and `async_receive_from` in place of the stream operations; the acceptor `open`, `bind`, `listen`, both `accept` and both `async_accept` and `close`; each synchronous operation having an `error_code&` overload that never throws.

#### Scenario: A program written for Boost's sockets compiles
- **WHEN** `sockets_test` instantiates the three templates with `tcp` and `udp` and calls each member above
- **THEN** it compiles on Emscripten and every call but `open`, `close` and `is_open` reports `operation_not_supported`

### Requirement: Socket: A transport specialisation provides the operations
A specialisation of `webcpp::asio::browser::transport<Protocol>`, visible in every translation unit that uses the socket type before its first use (the one definition rule), SHALL provide the operations, and the program's code MUST NOT change; enabling a relay library for a whole program is a b2 feature in the usage requirements that `config.hpp` reads, designed with that library.

#### Scenario: A test protocol with a transport
- **WHEN** `transport_test` specialises `transport<test_protocol>` with a `connect` that succeeds and records the endpoint
- **THEN** `basic_stream_socket<test_protocol>::connect` succeeds, and so does `async_connect`, and the recorded endpoint is the one given

### Requirement: Resolver: The resolver resolves nothing and completes with one entry
On Emscripten `basic_resolver<InternetProtocol, Executor>` SHALL be webcpp's own class with Boost's `resolver_base`, constructors from an executor or an `ExecutionContext&`, `get_executor`, `cancel` (which does nothing), `resolve` and `async_resolve` (host and service, with flags; an endpoint) with the completion signature `void(boost::system::error_code, results_type)`, and `resolve(host, service)` MUST make one entry whose `host_name()` and `service_name()` are the arguments and whose endpoint is the unspecified address and the service's port; natively it is Boost's alias.

#### Scenario: One entry
- **WHEN** `resolve_makes_one_entry` resolves `("example.com", "https")`
- **THEN** there is no error, one entry with host "example.com", service "https", an unspecified address and port 443

#### Scenario: Asynchronous resolution completes through a post
- **WHEN** `async_resolve` is used with a callback and with `deferred`
- **THEN** it completes through a post with the same entry

### Requirement: Resolver: The service decides the port
The port SHALL be 0 for an empty service (as Boost's resolver gives), the number when the service is all digits and at most 65535 (`"0"` included), 80 for `http` and `ws`, 443 for `https` and `wss` unless the flags hold `numeric_service`, and any other service MUST fail with `boost::asio::error::service_not_found` and no entry; the port is parsed with `std::from_chars` in a function of its own that starts with its guards.

#### Scenario: The table of services
- **WHEN** the services `80`, `0`, `65535`, `65536`, `http`, `ws`, `https`, `wss`, `WS`, `80x` and the empty string are resolved
- **THEN** the ports are 80, 0, 65535, not found, 80, 80, 443, 443, not found, not found and 0, and with `numeric_service` `https` is not found and `443` gives 443

### Requirement: Resolver: Reverse resolution and plain connects fail
Reverse resolution (`resolve(endpoint, ec)`, `async_resolve(endpoint, ...)`) SHALL fail with `operation_not_supported`, and a plain socket's `connect` on a resolved entry MUST fail with `operation_not_supported`, so that only webcpp::beast's browser streams accept the entry; with exceptions `resolve("h", "nope")` throws a `system_error` of `service_not_found`.

#### Scenario: A plain socket cannot connect to the entry
- **WHEN** `a_plain_socket_cannot_connect_to_the_entry` connects a `tcp::socket` to the resolved entry
- **THEN** the error is `operation_not_supported`

### Requirement: Ssl: Boost's ssl natively and a record on Emscripten
`<webcpp/asio/ssl.hpp>`, which the convenience header leaves out, SHALL define `namespace ssl = boost::asio::ssl;` in `webcpp::asio` natively and on WASIp3 (needing source-built OpenSSL 3) and on Emscripten a record of the program's TLS settings (`context`, `stream_base`, `verify_*`, `host_name_verification`) that links no library and that only webcpp::beast's browser streams read; without `WEBCPP_ASIO_HAS_OPENSSL` on those guest TLS targets the header MUST stop with a message telling the program to add `/webcpp/asio//openssl` beside `/webcpp/asio//asio`.

#### Scenario: Natively without OpenSSL
- **WHEN** a native program includes `<webcpp/asio/ssl.hpp>` without naming `/webcpp/asio//openssl` (`ssl_without_openssl_test`)
- **THEN** the compile stops with the instruction

### Requirement: Ssl: OpenSSL is an explicit source-built dependency
`/webcpp/asio//openssl` SHALL select `/webcpp/third_party//openssl`, built from the pinned unmodified upstream source for native or WASIp3, with matching generated headers and archives for the requested compiler, optimization, runtime and threading profile. It MUST NOT silently fall back to system OpenSSL. Emscripten MUST refuse a guest OpenSSL target by name because browser streams use host-managed TLS. `/webcpp/asio//tls` SHALL add the verified embedded CA snapshot and support an explicit replacement trust set without combining it with implicit system roots.

#### Scenario: Native and WASIp3 source builds
- **WHEN** a program names `/webcpp/asio//openssl` or `/webcpp/asio//tls` on native or WASIp3
- **THEN** its OpenSSL headers and binaries come from the same B2 variant and pinned source, without requiring a system OpenSSL installation

#### Scenario: Guest OpenSSL requested in the browser
- **WHEN** a program names `/webcpp/asio//openssl` for Emscripten
- **THEN** its compile fails with the instruction to use the browser settings adapter

#### Scenario: Cobalt's optional SSL wrapper
- **WHEN** a program names `/webcpp/asio//cobalt_ssl` on native or WASIp3
- **THEN** the checked Cobalt SSL overlay and source-built OpenSSL are linked, and TLS state, encrypted transfers, cancellation and shutdown are covered by real-peer tests

### Requirement: Release: The rulebook, index, documentation and gates
Task 22 SHALL reconcile CODING-GUIDELINES chapters 15/16 and AGENTS with the actual targets, compiled dependencies and named compatibility departures. The page SHALL retain the owner's Diataxis organization and generated MrDocs reference, followed by the changelog. Task 23 SHALL review the complete library and superproject integration, correct important findings with failing regressions first, and verify the exact candidate tree in CI before final integration. G1 records repository creation, G2 the superproject submodule, and G3 publication plus successful library and superproject CI. Existing owner-authorized publication is recorded as history, not repeated or hidden. No subagent pushes, creates a repository or changes a GitHub setting.

#### Scenario: Validation covers the actual target variants
- **WHEN** the CI plan and declared own lanes are inspected
- **THEN** they include native, Emscripten and WASIp3, JSPI/Asyncify, browser runtime checks and the supported networking/TLS/threading variants; unsupported profiles are refused and documented explicitly

### Requirement: WASIp2 pollable readiness and cancellation

The P2 backend SHALL combine socket, timer and forward-DNS readiness through the
executor without resolver worker threads or fixed-interval lookup polling. It
SHALL preserve deferred initiation, associated executors and allocators, and
resolver-wide and per-operation cancellation. Unsupported resolver policies
SHALL complete with explicit error values. Any private libc ABI bridge SHALL
verify its SDK/libc revision and copied declaration hashes before building.

#### Scenario: A pending CLI read is cancelled by a timer

- **GIVEN** a P2 CLI input stream that remains open without input
- **WHEN** an Asio timer cancels its pending asynchronous read
- **THEN** the read completes once with operation_aborted, with no dependency on EOF
- **AND** this guarantee excludes a host import that blocks inside the host itself

#### Scenario: A deferred operation has not been initiated

- **GIVEN** a deferred asynchronous operation and an intervening cancellation
- **WHEN** the application later initiates that operation
- **THEN** the earlier cancellation does not cancel the newly initiated operation
