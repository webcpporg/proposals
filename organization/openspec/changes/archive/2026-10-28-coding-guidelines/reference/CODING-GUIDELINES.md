# webcpp's coding guidelines

This file is the single source of webcpp's rules on how code is written and
of every decision of its software design, each with its reason. It is
written for coding agents and human contributors alike. With
[AGENTS.md](AGENTS.md), which says how to work in the tree (the tools, the
commands, the API reference and the pages, the lanes, the CI and the
releases), it holds everything needed to port a library to webcpp or to
change one: an agent with no other context does it by following the two.
Read this file first, then `AGENTS.md`, then the library's own `AGENTS.md`
(`libs/<name>/AGENTS.md`), which holds only what is specific to that
library: the original and its version, how its oracle drives the original,
its particular cautions.

**One home for each rule and each decision.** A rule or a decision is
written here once. Every other file that needs it, `AGENTS.md`,
`CONTRIBUTING.md`, an agent skill, a library's `AGENTS.md` or page, a tool's
message, a proposal, points to its chapter instead of restating it; where
one of them seems to differ from this file, this file rules.

**Where a decision is made, and where it lives.** A decision is made in a
proposal, in [webcpporg/proposals](https://github.com/webcpporg/proposals),
and decided by the owner. Chapters 1 to 15 describe the tree as it is.
Chapters 16 to 18 hold what is decided and not yet built, and what is
proposed and not yet decided, each with the proposal that holds it in full
and rules over this summary while it is open. Chapter 19 holds the
conflicts still open between a decision and a rule, or between a design
document and its tree. The change that builds a decision moves it into
chapters 1 to 15, in the same change; the proposal's `design.md` stays as
the record of how it was decided.

Chapters 1 to 13 apply to every change: read them whole. Chapters 14 to 19
are read by section: the organization's, and those of the libraries a
change touches.

A chapter of this file is cited as "chapter N", one of the superproject's
`AGENTS.md` as "`AGENTS.md` chapter N". Every command below runs from the
superproject's root, in a shell where `CPATH`, `CPLUS_INCLUDE_PATH` and
`C_INCLUDE_PATH` are unset (`AGENTS.md` chapter 6).

## Contents

Working:

1. Process
2. Commits and text

Writing code:

3. Names
4. The language
5. Bringing a C library into webcpp
6. Exceptions and RTTI
7. Doc Comments
8. Tests
9. What the lint enforces

Libraries and ports:

10. Choosing and registering a port
11. Fidelity
12. Evidence: oracle and twins
13. Porting a library, step by step

Design:

14. The organization's design
15. The libraries' design

Not built yet:

16. Roadmap
17. Decided, not built
18. Proposed, not decided
19. Conflicts and open questions

## 1. Process

- **The specification before the code, a failing test before the
  implementation.** A behaviour is written down (in the page, or in the
  library's `AGENTS.md` for what is internal) before it is built, and a test
  is seen failing before the code that makes it pass.
- **A defect is first reproduced the way a user meets it:** the command a
  user runs, the page a reader opens, the lane CI runs. Then it is fixed, and
  the reproduction becomes a test.
- **Only a build from scratch counts as evidence:** `b2 -a`. b2 compares
  timestamps, and an incremental build can leave a stale result in place. An
  incremental build is for iterating.
- **Green, before any merge,** means all of these, run and read, not assumed:
  - every declared lane from scratch: `b2 -a libs/<name>/test
    libs/<name>/example` natively, and with `toolset=clang-wasip2
    testing.launcher=wasmtime` and `toolset=clang-wasip3
    testing.launcher=wasmtime` where declared, and every own lane the
    library declares, `b2 -a libs/<name>/test/oracle//oracle` for an oracle
    and, on each target it names, `b2 -a toolset=clang-wasip2
    testing.launcher=wasmtime libs/wasi/test//http` for one on wasip2;
  - `b2 -a doc`;
  - the lint: `lint: clean`;
  - the tests of the build and of the tools, when they or what they test
    changed: `tools/test/jamroot_test.py`, `tools/test/webcpp_jam_test.py`,
    `tools/test/oracle_jam_test.py`, `tools/test/generated_jam_test.py`,
    `tools/test/component_jam_test.py`, `tools/generated/generated_test.py`,
    `tools/test/emscripten_test.py`, `tools/test/external_test.py`,
    `tools/test/doc_test.py`,
    `tools/test/harness_test.py`, `tools/test/third_party_test.py`,
    `tools/test/boost_libraries_test.py`,
    `tools/lint/lint_test.py`, `tools/report/report_test.py`,
    `tools/doc/doc_check_test.py`, `tools/doc/doc_comments_test.py`,
    `tools/doc/extensions_test.py`, `tools/doc/counts_test.py`,
    `tools/oracle/twins_test.py`, `tools/oracle/compare_test.py`,
    `tools/example/run_example_test.py`, `tools/component/serve_test.py`,
    `tools/drive/drive_test.py`, `tools/node/install_test.py`,
    `tools/ci/matrix_test.py`, `tools/ci/assemble_test.py`,
    `tools/ci/actions_test.py`, `tools/release/changelog_test.py` and
    `tools/release/release_test.py`: every `tools/**/*_test.py`, as the CI
    runs them, each as `python3 <path>`. They
    build in scratch copies under `$TMPDIR`, whose path holds a space, so they
    run beside a build of the tree;
  - CI green, the library's and the superproject's.
- **Fix the lint, the failures and the flakiness you meet,** even when they
  are not yours; report what you cannot fix.
- **A change of the build or of a tool** has a test that fails without it:
  the tests of `tools/` pin every behaviour this file and `AGENTS.md` state.
  A change to b2's `--out-xml` format shows as a red `report_test.py`, whose
  samples are recorded, never edited by hand
  (`python3 tools/report/record_samples.py`).
- **A change that alters a decision updates it here,** in the same change:
  the rule of chapters 1 to 13, or the decision of chapter 14 or 15, with
  its reason. A change that builds a proposal moves its decisions from
  chapter 17 or 18 into chapter 14 or 15, and one that resolves an entry of
  chapter 19 removes it.

## 2. Commits and text

- **Identity.** Every contributor, human or agent, maintainer or external,
  commits under their own git identity: a name and an e-mail address they
  control, of which a GitHub noreply address
  (`<id>+<login>@users.noreply.github.com`) is one. An agent commits under
  the identity of the account it acts for or of its own account, never under
  another person's; these rules fix no address. Either way, with no trailer
  of any kind (no `Co-authored-by`, no `Signed-off-by`, no agent name):

  ```
  git -c user.name=<name> -c user.email=<address> commit -- <paths>
  ```

  Stage and commit with explicit paths, never `git add -A`. A subject is
  plain: what the change does, prefixed by the part it touches
  (`tools/doc: ...`, `xactor: ...`).
- **No accidental trailer.** git reads a last body paragraph that starts with
  `word:` as a trailer (`std::` does). Start such a paragraph otherwise.
- **English** in everything written to a file: code, comments, documents,
  commit messages.
- **No em dash** (U+2014) anywhere; use a plain dash.
- **The banned word.** No file, file name or commit mentions the word that
  `veru[s]` matches, in any case: `git grep -I -i -e 'veru[s]'` prints
  nothing in any repository, and the lint checks every commit's author,
  committer and message too.
- **The licence notice.** Every source file (`.hpp`, `.cpp`, `.py`, `.mjs`,
  `.js`, `.sh`, `.jam`, `Jamroot`, `Jamfile`, `build.jam`, `.yml`, `.adoc`,
  `.html`) opens with it: with `//` for C++, JavaScript and AsciiDoc, with
  `#` for Jam, Python, shell and YAML; a `#!` line stays first:

  ```
  // Copyright (c) 2026 WebCpp.org
  //
  // Distributed under the Boost Software License, Version 1.0. (See
  // accompanying file LICENSE_1_0.txt or copy at
  // https://www.boost.org/LICENSE_1_0.txt)
  ```

  An HTML file opens with a comment whose first line is `<!--` alone and
  whose next lines are the notice as it is, the comment going on with what
  the file is, as `tools/doc/docinfo.html` does:

  ```
  <!--
  Copyright (c) 2026 WebCpp.org

  Distributed under the Boost Software License, Version 1.0. (See
  accompanying file LICENSE_1_0.txt or copy at
  https://www.boost.org/LICENSE_1_0.txt)

  The style every page adds to Asciidoctor's, in its head (docinfo=shared).
  -->
  ```

  The lint checks each of these. Markdown files, including `SKILL.md`, do
  not need this source-file notice; upstream licence and attribution
  notices are preserved.
- **History.** A repository whose code was moved from elsewhere starts from one
  import commit; the history of the code it was moved from is not carried
  over.
- **Outward actions** belong to the owner: no push to a webcpporg
  repository, no merge, no remote repository created, no GitHub setting
  changed, no SSH key or credential touched by an agent. An external
  contributor proposes a change by a pull request from a fork
  (CONTRIBUTING.md).

## 3. Names

| What | Rule | xactor's |
| --- | --- | --- |
| namespace | `webcpp::<name>` | `webcpp::xactor` |
| what is not public | a namespace `detail`, at any depth | `webcpp::xactor::detail` |
| convenience header | `<webcpp/<name>.hpp>`, which includes every public header, but may leave out one that brings an optional heavy dependency (below) | `<webcpp/xactor.hpp>` |
| headers | `<webcpp/<name>/...>`, one per responsibility, `snake_case` | `<webcpp/xactor/scheduler.hpp>` |
| macros | `WEBCPP_<NAME>_*` | `WEBCPP_XACTOR_*` |
| configuration | `<webcpp/<name>/config.hpp>`, which every public header includes first, which includes `<webcpp/config.hpp>` first, with `WEBCPP_<NAME>_NO_EXCEPTIONS` (chapter 6) | `<webcpp/xactor/config.hpp>` |
| test-only macros | `WEBCPP_TEST_*` | |
| include guards | `WEBCPP_<NAME>_<HEADER>_HPP` | `WEBCPP_XACTOR_ACTOR_LOGIC_HPP` |
| b2 target | `/webcpp/<name>//<name>` | `/webcpp/xactor//xactor` |
| error category | `webcpp.<name>` | `webcpp.xactor` |
| identifiers | `snake_case`, Boost's convention; `.clang-tidy` enforces it | |

config is the one exception: it holds the collection's macros, `WEBCPP_*`, as
Boost.Config holds `BOOST_*`, in `<webcpp/config.hpp>` and
`<webcpp/version.hpp>`, whose include guards are `WEBCPP_CONFIG_HPP` and
`WEBCPP_VERSION_HPP`, and declares no namespace.

A convenience header may leave out a public header that brings an optional
heavy dependency, which a user of the rest should not pay for, and names in
its comment the header it leaves out and the dependency:
`<webcpp/pratt.hpp>` includes pratt's engine, and leaves out
`<webcpp/pratt/calculator.hpp>`, which brings Boost.Decimal.

A test's helpers that are shared between tests go in `webcpp::test`
(`libs/xactor/test/require.hpp`). Example programs alias the namespace
(`namespace xactor = webcpp::xactor;`), as Boost's do.

## 4. The language

- C++20, header-only. The Jamroot sets `<cxxstd>20`, `<warnings>extra` and
  `<warnings-as-errors>on` for everything.
- **What a library builds on, in this order:** what webcpp already has;
  then Boost (boost.org); then the C++ Alliance's libraries
  ([cppalliance](https://github.com/cppalliance)), whose authors are Boost's;
  then the standard library; then a library from outside that follows Boost,
  the standard library and the standards of its field; then a C++ port of a
  very widely used C library; and only then an implementation of the
  library's own. A library from outside comes in after the owner's review,
  with the reason written in the library's documentation (OpenSSL, for one,
  because it is the standard of its field). The same order governs each
  function an implementation of its own writes: it builds on those bases
  first, and follows their standards. So a hash is Boost.Hash2's before
  anything else, and a JSON parser is Boost.JSON's.
- **Written for the web.** Every library is written to be taken to the web:
  its memory, its I/O and its scheduling are its user's to customize (the
  allocators below; the executors and schedulers it runs on), and on the
  emscripten target it uses what the browser offers, so that a C++ program
  in a web page has a development environment as good as the web's own.
- **Within Boost, the newest library of a kind:** Boost.Coroutine2 rather
  than Boost.Coroutine, Boost.Variant2 rather than Boost.Variant, Boost.Hash2
  rather than Boost.ContainerHash for new hashing, Boost.Unordered's flat
  maps rather than an older map, Boost.Charconv rather than a hand-written
  conversion.
- **A library from outside is wrapped,** behind an interface that follows its
  counterpart in the standard library or in Boost as closely as it can, so
  that the outside library is an implementation detail the user never names
  and another implementation can replace it: as webcpp::trystero hides
  libdatachannel behind its backend interface, with the browser's WebRTC as
  the other backend.
- **Types of a fixed meaning.** An integer whose width matters is one of the
  fixed-width types of `<cstdint>` (`std::uint8_t`, `std::int64_t`,
  `std::uint32_t`), as Boost.Config's `<boost/cstdint.hpp>` guidelines give
  them; a size or an index is `std::size_t`, a difference
  `std::ptrdiff_t`; plain `int`, `long`, `short` and `unsigned` appear only
  where an interface of the standard library or of Boost asks for them.
- **Errors as values.** An operation that can fail returns its error:
  `boost::system::result<T>`, with an error category of the library's own
  (`webcpp.<name>`) and fixed enumerator values that are never reused.
- **No global or static mutable state.** `.clang-tidy` enforces part of it
  (`cppcoreguidelines-avoid-non-const-global-variables`).
- **Determinism.** No clock, file, network, process, thread, environment or
  entropy, unless that is the library's purpose. The lint's world rule finds
  them by name in every library header; a line that names one without
  reaching the world (a simulated clock), or that reaches it because it is the
  library's purpose, says why after `lint-world:` on the same line:

  ```cpp
  #include <boost/asio/io_context.hpp>  // lint-world: posts handlers only
  ```

- **Allocation.** A library is to avoid dynamic allocation as far as its job
  allows, and to let its user customize the allocator of what it does
  allocate, so that it can be used with Boost.Interprocess:
  - **A data structure that makes sense to share** (a parsed JSON document,
    a state machine's definition) takes an `Allocator` template parameter,
    allocates through `std::allocator_traits` of it, and holds every pointer
    as the allocator's `pointer` type (Boost.Interprocess's `offset_ptr`) or
    as an offset, never an absolute address, a vtable or a `std::function`: it
    can be built in a shared segment and used in place by another process
    that maps it elsewhere.
  - **Everything else** (run-time objects: actors, rooms, sockets, handlers)
    routes every allocation through an allocator its user gives, a template
    parameter or a `std::pmr::memory_resource`, with no requirement to live
    in a shared segment.
  - Each library's page says which of its types are shareable, and a test
    that counts allocations proves that none goes elsewhere.

  A library ported or written from now on is born with this rule, as pratt
  is: its calculator's environment takes the user's `Allocator`; and as
  trystero is, through `std::pmr`: a `std::pmr::memory_resource` given in
  its `app_config`, and in a room's `room_options`, reaches every allocation
  of the app and its rooms, including the immutable password in the app's
  resource, which a counting test
  proves natively and on emscripten. The libraries written before it are
  brought to it by a review (chapter 16).
- **Asynchronous code is built on xactor.** A library that ports anything
  asynchronous runs it as actors on xactor, whose scheduler drives every
  library the same way, deterministically, and in a browser from the page's
  event loop, which owns the system's events. How such a library meets
  Boost.Asio and Boost.Cobalt is still to be designed (chapter 16).
- **Text** is passed and held as `std::string_view` where nothing must own
  it; `std::string` only where something does.
- **A function starts with its guards:** every condition it needs is checked
  first, and what fails returns there, before any work begins.
- **A comment says what the code does not make clear:** why, a constraint, an
  example of a value. A descriptive comment is one sentence of what the code
  that follows does, then an optional reason, in a few lines.
- **JSON literals in code are laid out for their reader:** a raw string
  literal that opens a line with `R"({` or `R"([` has its members four spaces
  deeper than that line, one per line, and closes at that line's indentation.

## 5. Bringing a C library into webcpp

A webcpp library that builds on a C library (webcpp::crypto on BearSSL and
libsecp256k1, for example) brings it in this way, so that the C code stays
exactly its authors' and the program carries only what it calls.

1. The C library is a submodule under `third_party/libs/<name>`, pinned at a
   commit and never edited. When upstream is not on GitHub, an unmodified
   `webcpporg/<name>` mirror stands in (BearSSL's is `webcpporg/bearssl`),
   refreshed only when the pin moves.
2. `third_party/Jamfile` builds it from its own sources, file by file, as a
   static library, in the library's own C dialect, on every target and with
   every file: a file that does not apply to a target still compiles, to the
   stubs other files call. A single unity source is not used.
3. Every file is compiled with `-ffunction-sections -fdata-sections` (`/Gy
   /Gw` on MSVC); upstream's warnings are not webcpp's (`<warnings>off`); each
   define the library cannot work out for itself on a target sits beside the
   sources with its reason (BearSSL's `BR_USE_GETENTROPY=1` on WebAssembly).
4. Its usage requirements give the public include directory, the define
   `WEBCPP_<LIB>_HAS_<NAME>=1`, and the link-time removal of unused sections
   (`-Wl,--gc-sections`, `-Wl,-dead_strip`, `/OPT:REF`).
5. A program names the C library in its own Jamfile, beside the webcpp library
   that uses it, so that its choice is explicit:
   `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;`. A
   library named and never called adds no byte to the program.
6. Three layers stop a wrong build, each saying exactly what to add:
   - at compile time, the backend's header checks `WEBCPP_<LIB>_HAS_<NAME>`
     and `#error`s with the Jamfile line to add;
   - at link time, every function that calls the C library references a
     canary, `webcpp_<lib>_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link`,
     defined by a small file of webcpp's that the Jamfile adds to the library,
     so that a missing library, or one of another commit or configuration,
     fails to link with that name;
   - on MSVC, the header and the canary carry `#pragma
     detect_mismatch("webcpp_<lib>_backend_<name>", "<name>-<commit7>-<configuration>")`.
7. A library webcpp does not build (OpenSSL) is found by `webcpp.external`,
   linked shared or static by a propagated feature, refused on WebAssembly,
   and announces itself by the same `HAS` define. When a program names it and
   the machine lacks it, the build stops with the instruction (how to install
   it, `-s<NAME>_ROOT=<dir>`, or another backend): it is never left out in
   silence. Only webcpp's own tests and examples are left out, as `AGENTS.md`
   chapter 4 says, and the CI requires them anyway.
8. The webcpp library's `config.hpp` chooses the default backend from the
   target and the `HAS` defines, and refuses impossible combinations with
   `#error`. The backend is named after the C library (`backends::bearssl`),
   never after a quality.
9. The documentation gives, per backend, its Jamfile line, what it exports
   (headers and functions), and every message above with its fix, written so
   that an AI agent writes a correct Jamfile at once.
10. One copy per program: webcpp's build of a C library is never linked
    together with another copy of it.
11. The CI proves, on every target: the C library's own tests pass on
    webcpp's build; a program that uses it runs; one that names it without
    calling it is byte-identical to one without it; each message above comes
    from a test of its own, in a directory of its own; the canary's commit is
    the submodule's.
12. Moving a pin is a change of its own: the submodule, the canary's commit,
    the tests on every target and the size table.

## 6. Exceptions and RTTI

- **Every target builds with exceptions on.** Whether a program uses them is
  the choice of whoever builds it, never an imposition of a library or of
  webcpp. Natively the compiler's default holds. On wasip2 and wasip3 the
  Jamroot compiles with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`
  and links with `-fwasm-exceptions -lunwind` (its region
  `wasi-exceptions`): wasi-sdk 34 emits the legacy encoding by default, which
  wasmtime 47 refuses to run ("legacy_exceptions feature required"). A
  program that uses exceptions on wasip2 needs a host that runs them:
  wasmtime 47 or later (wasmtime 37 to 46 with `-W exceptions=y`), jco
  with `jco transpile --bindgen-enable-wasm-exnref`, or another Wasm 3.0
  engine.
- **A user's build without exceptions.** `exception-handling=off` compiles
  with `-fno-exceptions` on any target, Boost.Config then defines
  `BOOST_NO_EXCEPTIONS`, and `webcpp.jam` links `tools/throw_exception.cpp`,
  a `boost::throw_exception` handler that prints and aborts; MSVC also gets
  `_HAS_EXCEPTIONS=0`. Whether a build has exceptions is the user's choice
  alone: every library supports both (below), the lint analyses every
  program both ways, and a library proves what behaves otherwise without
  exceptions by tests of its own that declare `<exception-handling>off`
  (trystero's `browser_*_no_exceptions`).
- **Each library has its own configuration macro,**
  `WEBCPP_<NAME>_NO_EXCEPTIONS`, as Boost.Asio has `BOOST_ASIO_NO_EXCEPTIONS`,
  in `<webcpp/<name>/config.hpp>`, which the convenience header and every
  public header include first, and which includes `<webcpp/config.hpp>` first.
  It is defined when `WEBCPP_HAS_EXCEPTIONS` is not: when the code is compiled
  without exceptions (`BOOST_NO_EXCEPTIONS`, which Boost.Config defines under
  `-fno-exceptions` and so under `exception-handling=off`), or when the
  developer defines `WEBCPP_NO_EXCEPTIONS`, which disables every library's at
  once; and a developer may define it alone, to turn one library's exceptions
  off in a build that has them. A program sets `WEBCPP_NO_EXCEPTIONS` for
  every library alike, without knowing which one throws, so a library that
  raises no exception of its own has the macro too, and says so in its Doc
  Comment. Its test, `test/config_test.cpp`, is built three times: as the
  build asks (`config`), where the macro is defined exactly when
  `WEBCPP_HAS_EXCEPTIONS` is not; with `WEBCPP_NO_EXCEPTIONS` defined
  (`config_all_off`); and with the macro itself defined on the command line
  (`config_developer_defined`). wasi, which includes nothing of Boost, reads
  `WEBCPP_NO_EXCEPTIONS` itself, beside the compiler's own macros, until its
  redesign includes `<webcpp/config.hpp>`. MrDocs parses a library as a build
  with exceptions does, so `config.hpp` defines the macro for the reference
  under `#ifdef __MRDOCS__` and undefines it at once: the reference lists it,
  and every API that throws stays in the parse.
- **An exception is raised through `boost::throw_exception`, never a bare
  `throw`,** and every API that throws sits behind
  `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, so that code that uses it does not
  compile without exceptions. The lint's bare throw rule refuses a `throw`
  expression in a library's public header, outside comments and literals,
  wherever it is: inside that `#ifndef` region too, in a template that
  nothing instantiates, and as a rethrow, `throw;`, which
  `std::rethrow_exception` replaces.
- **A library works without exceptions.** Its headers compile with
  `-fno-exceptions`: what throws, tries or catches sits behind
  `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, and a failure that cannot be
  returned goes through `boost::throw_exception`. The lint checks it, by
  analysing every aggregate translation unit, and every test and example, a
  second time as b2 compiles them with `exception-handling=off` (below).
  Clang refuses a `throw`, a `try` or a `catch` there, in a template only
  where a test or an example instantiates it: in a template that none of them
  instantiates, only the bare throw rule sees a `throw`, and a `try` or a
  `catch` is not checked. The one exception is a header that wraps a
  dependency which reports errors by throwing: its library declares it with
  `<exception-handling>on`, with the reason in a comment, on each
  headers-alone call, test and program of the backend that brings the
  dependency, since b2 lets no usage requirement override a build's
  `exception-handling=off` (measured: a usage requirement of the backend's
  target leaves its dependents compiled with `-fno-exceptions`); the lint
  leaves that header out of the analysis without exceptions, and names it;
  and the header declares nothing without exceptions: what follows its
  `config.hpp` sits inside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, with a
  comment that names the dependency, so that a program built without
  exceptions that uses it fails on the first name it uses (trystero's:
  `no member named 'backend' in namespace 'webcpp::trystero::native'`), and
  one that only includes it compiles. The
  fixture browser_demo's `native.hpp` is one, against the fake dependency of
  its `deps/include` (a fixture without `config.hpp`, it tests
  `BOOST_NO_EXCEPTIONS`).
- **A program that needs exceptions says so.** A test or an example that
  throws, tries or catches on purpose declares `<exception-handling>on` in
  its requirements, as the fixture demo's
  `webcpp.example catches.cpp : <exception-handling>on ;` does: b2 then
  builds it with exceptions whatever the build asks, a user's
  `exception-handling=off` included, and the lint analyses it with them
  alone and names it ("built with exceptions whatever the build asks").
- **RTTI** is never restricted by webcpp, on any target, and no variant is
  built without it; a user imposes their own.

## 7. Doc Comments

How the API reference is generated from them, and where it sits, is
`AGENTS.md` chapter 2.

### The format

Doc Comments are Javadoc/Doxygen style, `/** ... */`, before the symbol they
document:

```cpp
/**
 Handles one message, the payload of `cause`.

 The scheduler calls it once for each message delivered to the actor, and
 delivers the actor's next message only after it returns.

 @param turn What the actor may do while it handles the message.
 @param cause The message, in the envelope that says who sent it.
 @return Success, or an error of any category, which ends the actor with
 the status @ref status::error.
 @note An error ends this actor and only it.
 @see "Writing an actor logic", in the guide.
*/
virtual result<void> handle(xactor::turn<Message>& turn, const envelope<Message>& cause) = 0;
```

### What every public symbol needs

A public symbol is one at namespace scope of `webcpp::<name>`, or a public
member of a public class, outside every namespace `detail`. Every one is
fully documented: functions, classes, structs, enums and enumerators,
aliases, concepts, variables, data members and macros.

- **A brief,** the first paragraph, which is a single sentence: MrDocs makes
  the whole first paragraph the brief, and `doc_comments.py` fails a brief of
  more than one sentence (an abbreviation such as `e.g.` and a code span do
  not count).
- **`@param`** for every parameter, **`@tparam`** for every named template
  parameter (of a function, class, partial specialization, alias, variable or
  concept, hidden friends and member templates included), and **`@return`**
  for every function that returns a value. MrDocs reports a missing `@param`
  or `@return`; `doc_comments.py` reports a missing `@tparam`, which MrDocs
  does not.
- **A deleted function takes no `@return`.** Nobody can call it, so a
  `@return` would say nothing, and MrDocs accepts its absence:
  `actor_logic& operator=(const actor_logic&) = delete;` has a brief and no
  `@return`.
- **`@throws`, `@pre`, `@post`, `@note`, `@see` and `@code`** where they
  apply.
- **Each enumerator and each data member has its own Doc Comment.** clang
  attaches one comment to every declarator up to the next `;`, `{`, `}`, `#`
  or `@`, so in `/** A. */ a, b,` the enumerator `b` has `a`'s comment, and in
  `/** The x. */ int x, y;` so does `y`. `doc_comments.py` reports the second
  as undocumented, public or `detail`. Comment each one itself, and declare
  one data member per declaration.
- **A `detail` symbol** needs only its brief.

## 8. Tests

- Unit tests use Boost.Core's lightweight_test,
  `<boost/core/lightweight_test.hpp>`, declared with `webcpp.run`. `main`
  runs the cases and returns `boost::report_errors()`.
- A case that cannot go on after a failed check returns:
  `if (!BOOST_TEST(x.has_value())) { return; }`. A helper, which cannot
  return from its case, calls `require(BOOST_TEST(...))`, which ends the
  program with the errors counted so far
  (`libs/xactor/test/require.hpp`): a user may build a test without
  exceptions.
- Boost.Test is for a test that needs it (fixtures, data-driven suites),
  declared with `webcpp.boost-test`, natively only. The CI installs no
  compiled `unit_test_framework`: the suite compiles Boost.Test's header-only
  framework, `tools/boost_test_runner.cpp`, as an object of its own.
- A test that reads files at run time is given their directory by a
  `path-constant`, a `WEBCPP_TEST_*` define with forward slashes, quoted
  whole, `[ webcpp.data <directory> ]` in its requirements, and a
  `<dependency>` on the files it reads (a `glob-tree-ex` of them), so that a
  changed file runs it again; `/webcpp/third_party//json-schema-test-suite`
  gives all four for the JSON Schema Test Suite.
- Every test runs on every target its Jamfile declares. What a target cannot
  build is excluded by declaration (`: native`), and inside a program by the
  same condition the library uses (`#ifndef __wasi__`), never by skipping
  silently.
- A test moved from the original's or another suite records the conversion,
  case by case, and each converted case is seen failing on a planted defect
  where the old one had a pinning test (`libs/xactor/test/CONVERSION.md`).

## 9. What the lint enforces

`tools/lint/lint.sh` lints the superproject and every library of `libs/` that
is a repository: what git tracks and what it would track (untracked, not
ignored), so a new file is linted before it is added, but nothing under
`third_party/libs/`, third-party code that keeps its own style and licence
(chapter 10). It prints each rule's section and ends with `lint: clean`, or
with `lint: failed: <rule>` for each rule that failed.

| Rule | What fails |
| --- | --- |
| clang-format | a C++ file not formatted as `.clang-format` says (Google-based, 4 spaces, 100 columns); `clang-format -i` fixes it |
| clang-tidy | a finding of `.clang-tidy` (every warning is an error) in the compilation database `tools/lint/compile_commands.py` writes from b2's dry runs, one per target the libraries declare: every test and example natively, with every program an own lane builds on that target, or on none (a served component, which only its own lane builds, among them; a served program in no own lane fails it, as it fails `b2 declared-lanes`), and each source that no native program compiles with the command of the first target that compiles it (wasip2, wasip3, else emscripten). A WebAssembly command names wasi-sdk's `clang++`, with that target's `--target`, to which the lint adds no host target or SDK, once the dry run of that target has generated the bindings its commands include (so the lint needs wit-bindgen and the WIT, `AGENTS.md` chapter 1); an emscripten command names Emscripten's `em++`, which clang-tidy cannot run, and is analysed with wasi-sdk's `clang++` in its place (the one beside the lint's clang-tidy), after the words `em++ <its options> --cflags` prints (the target `wasm32-unknown-emscripten`, the sysroot in Emscripten's cache, the `-iwithsysroot` directories compat and fakesdl, and the words of its exceptions), less the settings of em++ alone (`-sWASM_WORKERS`), which it reads and clang refuses, so the lint needs the emscripten toolset when a library declares emscripten. Plus each library's aggregate translation units, through which every public header is analysed, each compiled as a headers-alone translation unit of its target: a library whose public headers one target builds whole has one, of every header (`bin/aggregate/<name>.cpp`), natively, or, for a library whose headers build only for WASI, on wasip2 and on wasip3, each reading its version's branch; a library whose headers no single target builds (a native backend against a dependency of the host's, a browser backend for emscripten) has one per target its headers-alone units run on, of the headers that target builds (`bin/aggregate/<name>-native.cpp`, `<name>-emscripten.cpp`). Every command, a program's and an aggregate's, on every target, has its twin: the same translation unit as b2 compiles it with `exception-handling=off` on the same target (natively with the handler `tools/throw_exception.cpp`), so that what only a user's build without exceptions compiles (`#ifdef BOOST_NO_EXCEPTIONS`) is analysed too, and a `throw`, `try` or `catch` outside it fails, in a template where a test or an example instantiates it; a program that declares `<exception-handling>on` has none, and an aggregate's twin leaves out each header whose headers-alone call declares it (then a translation unit of its own, `<aggregate>-exception-handling-off.cpp`); the database names both. A public header without a headers-alone translation unit on any target, a declared target whose toolset is not configured, and a WebAssembly command whose compiler is neither wasi-sdk's `clang++` nor, on emscripten, Emscripten's `em++` (the lint would analyse it as a native command), fail it by name. Findings are reported in a library's public headers, in the headers of its tests and examples (`libs/xactor/test/require.hpp`) and in those of its JavaScript module (`libs/document/js/binding.hpp`), never in Boost's, the standard library's or a dependency's (`.clang-tidy`'s `HeaderFilterRegex`) |
| io_context::run | a call of Boost.Asio's `run`, `run_one` or `run_for`, which block, on an `io_context` however its type is qualified (`boost::asio::io_context`, `asio::io_context` through a namespace alias, `io_context` after a using); a driver drains with `poll` and `poll_one`. A program whose purpose is network I/O says why on the line, after `lint-run:` (`io.run_for(timeout);  // lint-run: the memory relays post to it`); a library has no blanket exemption |
| fluent chains | three calls chained in one expression |
| returns `*this` | a function other than an assignment operator, a prefix `operator++` or `operator--` (the iterator requirements make them return `*this`), or a function whose head follows a line `// lint-std: <reason>` naming the member of the standard it reproduces (`std::basic_regex::assign`), returning `*this`; the reason holds none of `;`, `{`, `}`, `#` or `@`, so that clang and MrDocs still attach the Doc Comment above the line to the function |
| em dash | U+2014 in any file |
| JSON literals | a raw JSON literal laid out otherwise than chapter 4 says |
| licence notice | a source file (C++, JavaScript, AsciiDoc, Python, shell, Jam, YAML, HTML) that does not open with the notice (chapter 2) |
| banned word | the word that the pattern `veru[s]` matches, in any case, in a file, a file name or a commit (its author, committer or message) of any repository |
| no clock, disk or network | a library header that names a clock, a file, a socket, a process, a thread, the environment or entropy (`random_device`, `getentropy`, `getrandom`, `arc4random`, OpenSSL's `RAND_bytes`, `<sys/random.h>`) without `lint-world:` |
| raw b2 rules | `run`, `run-fail`, `compile`, `compile-fail`, `exe`, `unit-test`, `link` or `link-fail` in a library's test or example Jamfile (`AGENTS.md` chapter 4) |
| Doc Comments | a command webcpp does not allow, a bare `@`, or a colon after a reference (chapter 7) |
| Pyright | an error or a warning in any Python file, with `pyrightconfig.json` (unused imports and variables are errors) |
| Python line length | a Python line over 100 columns |
| Python blank lines | a top-level `def` or `class` without two blank lines before it, its decorators and the comments just above it |
| Jam comment width | a comment line of a Jam file (`.jam`, `Jamroot`, `Jamfile`) over 80 columns, the width Jam comments are wrapped at |
| octal escapes | an octal escape of a C or C++ literal followed by a decimal digit (`"\\2200"`), which C++ reads as `"\\220"` and `"0"` and MSVC's warning C4125 refuses under `/WX` |
| path.exists native | a `path.exists` of a Jam file given a path other than `[ path.native ... ]`: b2's builtin asks the system about the path as written, and b2's form of a Windows path, `/D:/a/...`, names no file there |
| include boundaries | an `#include` that crosses a boundary the library declares in its `meta/include-boundaries.json` (below), at its line, with the boundary's reason; and a malformed file, or a glob that matches no file of the library |
| bare throw | a `throw` expression in a library's public header (`libs/*/include/**`), outside comments and literals, at its line: inside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS` too, in a template that nothing instantiates, which clang-tidy's analysis without exceptions does not see, and as a rethrow, `throw;`; `boost::throw_exception` is the one way to raise, `std::rethrow_exception` the one way to rethrow (above) |
| changelog form | a library's changelog or a version's release notes out of `AGENTS.md` chapter 5's form: a section or a region missing or out of order, a line that starts with no verb of the list, a cross-reference, more than ten lines for a library and a version, text outside a bullet |

A source b2 expects not to compile (`webcpp.compile-fail`), and one that
must stop with the error it states (`webcpp.compile-diagnostic`), are left
out of clang-tidy only; every other rule reads them.

**A library's include boundaries.** Every header compiles alone, so only a
boundary check sees a part of a library start to depend on another. A
library declares its boundaries in `meta/include-boundaries.json`, each a set
of headers (a `headers` glob, less an `except` glob, each matched against the
whole path under `libs/<name>/`, a `*` within one path segment) that must not
include a path starting with one of its `must-not-include` prefixes, and
`why`. xstate's keeps its machine core free of xactor and of the actor layer:

```json
{
    "boundaries": [
        {
            "headers": ["include/webcpp/xstate.hpp", "include/webcpp/xstate/*.hpp"],
            "except": ["include/webcpp/xstate/actors.hpp"],
            "must-not-include": ["webcpp/xactor", "webcpp/xstate/actors"],
            "why": "the machine core runs without xactor and the actor layer"
        }
    ]
}
```

**A directory's own `.clang-tidy`.** A library's tests or examples may need
one that inherits the root's (`InheritParentConfig: true`) and switches off
what does not apply to them, each with its reason in a comment. xactor's
`test/.clang-tidy` turns off `bugprone-unchecked-optional-access` (a test
asserts with `BOOST_TEST` before it reads, which the check does not see) and
`bugprone-exception-escape.CheckMain` (a test's `main` lets an exception end
the process, which fails the test as it should); `example/.clang-tidy` turns
off the second. The library's headers are still analysed with every check,
through the aggregate translation unit. A check that applies to a library
header is never switched off for a directory, since a header's templates are
analysed only where a test or an example instantiates them: the header states
its exemption on the line clang-tidy reports, `NOLINT(<check>)` (or
`NOLINTNEXTLINE(<check>)` on the line above, when that line has no room; a tag
comment between it and its function cancels the suppression), or, when a
page's listing must hide the exemption, with `NOLINTBEGIN(<check>)` and
`NOLINTEND(<check>)` around the exempt function or functions alone. The
reason still goes in the comment above, in every form.

**Sharding.** `--shard K/N` analyses the K-th of N interleaved slices with
clang-tidy, and runs every other rule; the N shards together analyse every
file once.

### The style and the checks: `.clang-format` and `.clang-tidy`

The two files at the superproject's root are every repository's: a library
adds a directory's own `.clang-tidy` only as above, and never a style of its
own. Their decisions, with the reasons each file records:

- **Google's style, at 4 spaces and 100 columns.** `.clang-format` is based on
  Google's, for C++20, with an indent of 4, a continuation indent of 4, no
  tabs, access modifiers at the class's indentation and pointers bound to the
  type (`int* p`). Short functions stay on one line only when they are
  defined in the class; an `if` or a loop never does; a short `case` may.
  Definitions are separated by a blank line, always. (Reason not recorded.)
- **Includes are kept in the blocks their author wrote, sorted within each.**
  A reader sees the product's own dependencies first, then Boost, then the
  standard library (`IncludeBlocks: Preserve`, `SortIncludes: CaseSensitive`).
- **A comment line that starts with `|` is a row of a table.** clang-format
  neither splits it at the column limit nor joins it with the next
  (`CommentPragmas`), since a table is read by its columns.
- **clang-tidy runs `bugprone-*`, `cert-*`, `misc-*`, `modernize-*`,
  `performance-*` and `readability-*`, with six checks of the core
  guidelines,** `cppcoreguidelines-avoid-non-const-global-variables`,
  `-init-variables`, `-pro-type-cstyle-cast`, `-pro-type-member-init`,
  `-slicing` and `-virtual-class-destructor`, and every warning is an error
  (`WarningsAsErrors: '*'`).
- **Findings are reported in webcpp's own headers only:** a library's public
  headers and those of its tests, examples and JavaScript module
  (`HeaderFilterRegex`), never in Boost's or the standard library's, which
  webcpp does not change.
- **Identifiers are `snake_case`, macros `UPPER_CASE`, a private member ends
  in `_`.** It is the libraries' rule and Boost's convention (chapter 3).
  Template parameters are not constrained, so they keep the standard's form
  (`Allocator`).
- **A function's cognitive complexity is at most 25**
  (`readability-function-cognitive-complexity.Threshold`). (Reason not
  recorded.)
- **`performance-enum-size` is off:** a library's error enum has to be `int`,
  since `boost::system::error_code::value()` returns `int`, and a narrower
  base type would only move the conversion.
- **`misc-header-include-cycle` is off:** it reports Boost's `.ipp` idiom, a
  header that includes an implementation file that includes it back behind
  an include guard; webcpp's own headers are flat and have no cycle.
- **`readability-redundant-member-init` is off:** every aggregate member a
  user can designate-initialize has a `{}` default member initializer, so
  that a designated initializer that leaves members out never triggers GCC's
  `-Wmissing-field-initializers` in a user's own `-Wextra` build; the check
  would ask for those initializers to be removed again.
- **Seven more checks are off,** `bugprone-easily-swappable-parameters`,
  `misc-include-cleaner`, `misc-non-private-member-variables-in-classes`,
  `modernize-use-trailing-return-type`, `readability-identifier-length`,
  `readability-magic-numbers` and `readability-uppercase-literal-suffix`.
  (Reason not recorded.)
- **Three rules have no clang-tidy check:** the ban on `io_context::run`,
  `run_one` and `run_for`, the ban on fluent chains that return `*this`, and
  the em dash. The lint enforces them by pattern (the table above).

## 10. Choosing and registering a port

### Choosing

- **What.** A library widely used for the web in another language, ported at
  an exact version of the original: a tag or a published release, recorded in
  `meta/libraries.json` and in the library's `AGENTS.md`. A port follows that
  version; moving to another one is a change of its own, with its evidence.
- **The licence.** An original under a licence compatible with BSL-1.0 can be
  ported: MIT, BSD and Apache-2.0. GPL cannot. MPL is decided case by case,
  by the owner. A port that derives from the original's code keeps the
  original's notice, unchanged, in `LICENSE-<ORIGIN>.txt` (`LICENSE-XSTATE.txt`,
  `LICENSE-TRYSTERO.txt`), and its README names the parts that derive from
  it. That adds no licence to webcpp's code: every file of webcpp is BSL-1.0
  only, and no repository has a `LICENSE-MIT.txt`. A library of webcpp's own,
  like xactor and pratt, has no `LICENSE-<ORIGIN>.txt`.
- **The name.** `[a-z][a-z0-9_]*`, the name of the original where it has one
  (xstate, trystero). `index` is reserved, in any case: the index page is the
  doc build's page `index`, and the report refuses a library of that name,
  nor `target`, `has`, `no`, `lib` or `version`, which config's macros take
  (`WEBCPP_TARGET_*`, `WEBCPP_HAS_EXCEPTIONS`, ...), nor `test`, which the
  test-only macros take (`WEBCPP_TEST_*`): the index refuses each.

### The checklist of a new library

Each item is done before the port's first green CI; the first two are the
owner's:

1. **The repository** `webcpporg/<name>`, public from its first commit, made
   by the owner. An agent never creates a remote repository, never pushes,
   and never changes a GitHub setting. Until the repository exists, the
   library is a local repository at `libs/<name>` (`git init`), which the
   lint and the build already treat as a library.
2. **The submodule**, added by the owner once the repository exists, with a
   URL relative to the superproject's, as Boost's `.gitmodules` writes its
   libraries' (`libs/xactor` is `../xactor.git`), so that a clone fetches
   the libraries over the same protocol as the superproject:

   ```
   git submodule add ../<name>.git libs/<name>
   ```

   Never `git add -A` or `git add libs` in the superproject: a library is a
   repository of its own, and the superproject records only its commit.
3. **`meta/libraries.json`**, Boost's file, with webcpp's `"port-of"`:

   ```json
   {
       "key": "xstate",
       "name": "xstate",
       "authors": ["R"],
       "description": "One sentence that says what the library is.",
       "category": ["State"],
       "maintainers": ["R <github.overboard312@passmail.net>"],
       "cxxstd": "20",
       "port-of": {
           "name": "XState",
           "language": "TypeScript",
           "version": "5.33.2",
           "url": "https://github.com/statelyai/xstate",
           "licence": "MIT"
       }
   }
   ```

   `"port-of"` is `null` for a library of webcpp's own (xactor's is). `name`,
   `description` and `port-of` are required, and every field of `port-of`,
   whose `url` is an `https` URL; `tools/doc/libraries.py` fails the doc build,
   naming the file and the field, otherwise.
4. **`build.jam`**, which the Jamroot finds by its glob `libs/*/build.jam` and
   registers as `/webcpp/<name>`. It declares one target, named after the
   library, that adds its headers and its dependencies:

   ```
   # Copyright (c) 2026 WebCpp.org
   # ... the licence notice (chapter 2)

   project /webcpp/<name> ;

   alias <name> : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;
   ```

   Every library adds config's target, `<library>/webcpp/config//config`,
   since its `config.hpp` includes `<webcpp/config.hpp>` (chapter 3). A
   dependency on another library is written as its target,
   `<library>/webcpp/xactor//xactor`. A compiled Boost library is built by
   webcpp from Boost's sources (`AGENTS.md` chapter 1): a library that needs
   Boost.URL adds `<library>/webcpp//boost_url` to its target's usage
   requirements, static, built once per variant with Boost.URL's own macros;
   never `/boost//url`, which would link the system's, nor
   `/webcpp/boost-libraries//boost_url`, which the Jamroot's alias checks the
   sources' version for. Boost.Charconv and Boost.Locale likewise,
   `<library>/webcpp//boost_charconv` and `<library>/webcpp//boost_locale`:
   Boost.Locale without ICU, with what it gives on each target in this table:

   | Target | `/webcpp//boost_locale` | `/webcpp//boost_charconv` |
   | --- | --- | --- |
   | native Linux (gcc, clang, glibc) | backends posix and std; iconv from glibc; no ICU (measured) | yes (measured) |
   | native macOS (Apple clang, gcc) | backends posix and std; iconv from the system's libiconv, `-liconv` given by the target; no ICU (measured) | yes (measured) |
   | native Windows (MSVC) | backends winapi and std, no iconv, no ICU: Boost's own configuration on Windows. Not measured here: no Windows machine; the first CI run that builds it is its measurement, and a failure is reported to the owner, never hidden | yes; not measured here |
   | wasip2, wasip3 | backends posix and std; iconv from wasi-libc; number, currency and date formatting in the C locale's form (no locale data, no ICU); a catalog is read from a directory the test gives (`webcpp.data`) (measured) | yes (measured) |
   | emscripten | backends posix and std; iconv from Emscripten's libc; a catalog is read from the host's files, as natively; single-threaded: Boost.Locale's sources alone are compiled with `BOOST_HAS_PTHREADS`, so that Boost.Thread's mutex, which three of them include, builds against Emscripten's pthread functions, which do nothing there; no program gets it, nor `-pthread`, and no public header of Boost.Locale depends on it, which `tools/test/boost_libraries_test.py` checks by preprocessing every one with it and without (measured) | yes (measured) |
   | any target, `exception-handling=off` | refused by name: Boost.Locale throws and catches unguarded (61 `throw`, 14 `try` in its sources, 8 `throw` in its headers) | yes (measured natively and on wasip2) |

   Since Boost.Locale needs exceptions, a program that names
   `/webcpp//boost_locale`, directly or through a library, declares
   `<exception-handling>on` in its own requirements, which keeps it with
   exceptions in a build without them, the lint's twins included (chapter 6):
   b2 ignores it as a usage requirement, and the refusal stops the whole
   build, naming that fix but not the program, which b2 does not give it.

   A library that needs Boost.JSON adds `<library>/webcpp//boost_json` to its
   target's usage requirements, as xstate's `build.jam` does: the Jamroot's
   `boost_json`, `tools/boost_json.cpp`, compiles Boost.JSON's definitions
   (`<boost/json/src.hpp>`, as Boost.JSON documents for a header-only build)
   into a static library, once per variant, so a user's build without
   exceptions gets them built without exceptions too. A library that builds
   WebAssembly components declares the C bindings of its worlds there too,
   with `webcpp.wit-bindings` (`AGENTS.md` chapter 4), as wasi's `build.jam`
   does for the world of each WASI version under its `wit/`. Nothing else
   registers a library: the test, example and doc directories join the
   aggregates `test`, `example` and `doc` by themselves, and
   `include/webcpp/**` joins `install`.
5. **`test/Jamfile`, `example/Jamfile` and `doc/Jamfile`**, with the rules of
   `AGENTS.md` chapters 4 and 3.
6. **The files of the repository:** `LICENSE_1_0.txt`, `README.md` (what the
   library is, how to build and test it inside the superproject, where its
   page is published, its licence), `AGENTS.md` (only what is specific to it,
   with links to the superproject's `AGENTS.md` as `../../AGENTS.md` and to
   this file as `../../CODING-GUIDELINES.md`), `doc/changelog.adoc` (its
   changelog, the page's last appendix, `AGENTS.md` chapter 5),
   `.gitattributes`, and a `.gitignore` that holds at least `doc/html/`, and
   for a library with an oracle `node_modules`, the link to the installed
   packages, which a pattern with a slash would miss, and `.node-modules/`,
   where `tools/node/install.py` installs them: the superproject's own
   `.gitignore` names only its top-level `doc/html/`.
7. **CI.** `.github/workflows/ci.yml`, which calls the superproject's reusable
   workflow (`AGENTS.md` chapter 4), as `libs/xactor/.github/workflows/ci.yml`
   does:

   ```yaml
   jobs:
     ci:
       uses: webcpporg/webcpp/.github/workflows/library.yml@main
       with:
         library: <name>
   ```

8. **The index page.** It lists every library of `libs/` from its
   `meta/libraries.json`, so the entry exists once the file does, and `b2 doc`
   shows it in `doc/html/index.html`, linked to the library's page, and its
   line under New Libraries in the release notes of the version in
   preparation. A library without a page fails the index.
9. **`LICENSE-<ORIGIN>.txt`**, where the port derives from the original's
   code (above).

### Third-party code

What a library or its tests use and webcpp does not own is never copied into
a library: it is a submodule under `third_party/libs/<name>`, pinned at a
commit, with the URL of its own repository, and `third_party/Jamfile`
declares it as `/webcpp/third_party//<target>`, whose usage requirements
give a program what it needs. It keeps its own licence and notices, and the
lint never reads it (chapter 9). Adding one is the owner's decision; moving
its pin is a change of its own, with its evidence. A target whose submodule
is not checked out stops the build that needs it, naming `git submodule
update --init third_party/libs/<name>`; a build that needs none never looks.

| Target | What it is | Licence |
| --- | --- | --- |
| `json-schema-test-suite` | the JSON Schema Test Suite at `5b0ee16` (2026-09-21), test data: a test that names it reads the suite at run time from the directory its define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` gives, on every target (wasmtime is given it, `webcpp.data`), and runs again when a file of the suite changes | MIT |
| `nativejson-benchmark` | Milo Yip's nativejson-benchmark at `478d572` (2022-10-28), test data: document's benchmarks (`libs/document/bench`) read its `data/canada.json`, `data/citm_catalog.json` and `data/twitter.json` at run time under the directory its define `WEBCPP_TEST_NATIVEJSON_BENCHMARK` gives, on every target (wasmtime is given it, `webcpp.data`), and run again when one of them changes; its own submodules are never checked out | MIT |

## 11. Fidelity

- A port behaves as the original, at the version it ports: the same results,
  the same order of events, the same errors, for the same inputs.
- Every difference is a decision, recorded in the page's appendix
  "Differences": what the original does, what the port does, and why. A
  difference forced by C++ (no garbage collector, value types where the
  original shares objects) is still recorded.
- Each difference has an example `example/diff_<topic>.cpp`, with its
  `.expected`, and its twin in the original's language (chapter 12), which
  shows the original's behaviour; the page shows both outputs side by side.
  xstate is the model, with its `diff_spawn_id`: XState keys a child spawned
  without an id as `"undefined"`, the port as an empty string, and the page
  shows the two outputs, each printed by its program.
- No divergence goes unrecorded. A difference found later, in a test, a
  review or a user's report, is either fixed or recorded in the same change
  that finds it.

## 12. Evidence: oracle and twins

A port's evidence is the original itself. The port's tests do not state by
hand what the original does: the pinned original runs the same cases and the
twins of the examples, and the port must produce what it produced. The
oracle that does it is shared by every port, in `tools/oracle/`; xstate is
its first user, and the model:

| Shared, in `tools/oracle/` | Per library, in `libs/<name>/test/oracle/` |
| --- | --- |
| the twin runner, `twins.py`, with its agreeing, divergent and without-twin bookkeeping | the pinned original (`package.json` and `package-lock.json`, for JavaScript) |
| output and tree comparison, `twins.py` and `compare.py` | the oracle scripts that drive the original over the cases (xstate's `oracle.mjs` and `actors.mjs`) |
| `update-expected`, with its refusals | the cases, and their C++ runner, under the library's `test/` |
| the `.jam` module of rules, `oracle.jam`, that the library's `test/oracle/Jamfile` calls | the twins, `twins/`, with `without-twin.txt` |
| the CI action that installs the language's runtime, `tools/ci/actions/node` | |

- **Cases.** The original's tests are ported as data-driven cases, which the
  C++ runner runs against the port.
- **Expected results.** The oracle runs the pinned original over the same
  cases and writes the expected results. The port must produce them.
- **Twins.** Every documented example has a twin in the original's language,
  `<name>.mjs` beside the C++ program in the library's twin directory, which
  the page includes through `{twins}`. A twin either agrees (prints exactly
  what the C++ example prints), or diverges, with the original's output
  recorded as its own `.expected` and the difference in the appendix
  (chapter 11), or is listed as without a twin, with the reason, as a line
  `<path>: <reason>` of the twin directory's `without-twin.txt`.
- **Nothing expected is written by hand.** Only `update-expected` writes an
  expected result, and it refuses to write a twin's output for a twin that
  agrees, since that output must be the C++ example's. Nor is a generated
  file: only `update-generated` writes it (`AGENTS.md` chapter 4).
- **How the original runs.** Each library declares it: for JavaScript and
  TypeScript, `node --conditions=development`.

### The rules (`tools/oracle/oracle.jam`)

A library's `test/oracle/Jamfile` declares its oracle with four rules of
`tools/webcpp.jam`. xstate's, whole after its licence notice and its first
comment:

```
import webcpp ;

webcpp.original node --conditions=development ;
webcpp.twins ../../example : twins : .mjs
  : --test-reporter=spec --test-reporter-destination=stderr ;
webcpp.cases machines : oracle.mjs : ../fixtures/cases : ../fixtures/expected ;
webcpp.cases actors : actors.mjs : ../fixtures/actors/cases : ../fixtures/actors/expected ;
webcpp.lane oracle : twins cases-machines cases-actors ;
```

| Rule | What it declares |
| --- | --- |
| `webcpp.original <word> + [ : <directory> ] ;` | how a program of the original's language runs, once and first; and the target `node-modules`, which installs what `package-lock.json` beside the Jamfile, or in the `<directory>` given (relative to the Jamfile: document's `test/js` runs the tests of `js/`, whose package pins Playwright and TypeScript), pins with `npm ci`, through `tools/node/install.py`: once per lockfile under `.node-modules/`, linked at `node_modules`, so that runs at once never install over each other |
| `webcpp.twins <examples> : <twins> : <suffix> : <extra-word> * ;` | the target `twins`: `twins.py` runs the twin `<twins>/<path><suffix>` of every program `<examples>/<path>.cpp`, at any depth, with the original's words and the extra words, and compares what it prints with the program's `.expected`, or with the twin's own `.expected` for a difference, which must then differ from the program's. Declared once per library: the page shows and counts its twins (`AGENTS.md` chapter 3) |
| `webcpp.cases <name> : <script> : <cases> : <expected> ;` | the target `cases-<name>`: the original runs `<script> <cases> <output>` into the build directory, and `compare.py` finds the output equal to `<expected>`, file by file and byte by byte. The Jamfile stops loading at a cases directory that does not exist, and at an expected directory whose removal would take the oracle's directory or the cases |
| `webcpp.lane <name> : <b2-target> + : <target> * ;` | an own lane of the library, the explicit alias `<name>` over the b2 targets, each a main target of the same Jamfile, which the lane makes explicit too, so that the ordinary lanes never build them; the alias compiles nothing, and leaves the Jamroot's check of Boost to the targets it names; `b2 declared-lanes` lists it and the CI runs it (`AGENTS.md` chapter 4). An oracle's names no target. One that names targets runs on each, as the CI's lane of that target does, and its tests reach the report: each is one of the Jamfile's `webcpp.targets` (`native` without any), or the build stops naming it. Every lane-only program, of `webcpp.serve`, `webcpp.serve-script` or `webcpp.drive`, must be named by a lane of its Jamfile on every target it is built for, or `b2 declared-lanes`, and with it the CI's plan, stops naming it. Only a Jamfile under the library's `test/` or `example/` declares one |

The first `webcpp.twins` or `webcpp.cases` also declares the target
`update-expected`, which writes every expected directory again from the
original, and each divergent twin's own output (`twins.py --update`, which
refuses one that agrees). Directories are relative to the Jamfile.

Every target is explicit: only a request by name, or a lane that names it,
runs Node, and `b2 test` never does. Node and npm are looked for on `PATH`
when an oracle target runs, and the build stops, naming the one that is
missing. The targets compile nothing, but they sit in the library's test
project and inherit its requirements, with the Jamroot's check of Boost, so
an oracle lane needs Boost and a toolset beside Node and npm.

| Command | What it does |
| --- | --- |
| `b2 -a libs/<name>/test/oracle//oracle` | the oracle lane, from scratch: every case and every twin on the original, compared |
| `b2 libs/<name>/test/oracle//twins`, `//cases-<name>` | one part of it, while working on it |
| `b2 libs/<name>/test/oracle//update-expected` | the original's results written again, over the committed ones; never a check |
| `python3 tools/oracle/twins.py --list --suffix .mjs --examples libs/<name>/example --twins libs/<name>/test/oracle/twins` | each program as agreeing, divergent or without twin, and their total, running nothing |

**What a port proves.** Its cases pass natively and on every target it
declares, against expected results the original wrote; its oracle lane is
green, so the original still writes every committed expected result and
every twin agrees, or diverges as recorded; and its page shows each twin's
code and each difference's two outputs. The runner accounts for every
program by name, so a twin that is no longer compared fails the lane rather
than leave it green on the rest.

## 13. Porting a library, step by step

1. Choose it, check its licence, and pin its version (chapter 10).
2. Make `libs/<name>` with its checklist files (chapter 10), and see `b2
   libs/<name>/test` build the headers-alone tests of an empty convenience
   header.
3. Port the original's tests as cases, and the oracle that writes their
   expected results from the pinned original (chapter 12); see them fail.
4. Port the code, header by header, with its Doc Comments (chapter 7), until
   the cases pass natively, then on every declared target.
5. Write the page: the guide with its examples and twins, the guarantees, the
   testing chapter, the reference and the differences (chapter 11 and
   `AGENTS.md` chapter 3).
6. Run the definition of green above, from scratch.

## 14. The organization's design

The decisions that shape the whole collection, each with its reason and its
source, then the site's. A source is a path in its repository at the commit
this chapter was written from: `webcpp` is the superproject at `ddb6cb2`,
where `AGENTS.md` was the rulebook before this file took its rules, and its
chapters are cited by the numbers they had there; `site` is the site's
repository at the commit the superproject pins; `proposals` is
[webcpporg/proposals](https://github.com/webcpporg/proposals). Where a source
records no reason, the bullet says so.

### The organization

The choices that shape the whole collection, each with its reason as its
source writes it, checked against the superproject's `main` at `ddb6cb2`.
Where a source and the tree disagree, the bullet says what the tree does. The
site's own decisions follow in the next sections.

**The superproject.**

- **webcpp is organized as Boost is: the superproject `webcpporg/webcpp` plays
  the part of `boostorg/boost`, and each library is a repository of its own, a
  submodule under `libs/<name>`.** Independent repositories, each with a build
  of its own, were rejected: they would let the rules drift and make a
  cross-library change two unrelated commits with no joint CI.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Every library is header-only C++20 under the Boost Software License 1.0,
  and every file webcpp writes is BSL-1.0 only.** (reason not recorded) A port
  that derives from its original's code keeps the original's notice in
  `LICENSE-<ORIGIN>.txt`, which adds no licence to webcpp's code.
  Source: `AGENTS.md` (webcpp).
- **webcpp brings to C++ the libraries other languages use for the web, each
  ported at an exact version of an original and proven against it, or a
  library of webcpp's own.** It builds infrastructure in both directions: the
  best of C++ within reach of the web, compiled to WebAssembly, and the
  libraries web developers use most brought to C++, so that programs written
  in C++ and compiled to WebAssembly have them.
  Source: `AGENTS.md` (webcpp).
- **A library's submodule has a URL relative to the superproject's
  (`../<name>.git`), and the superproject records only the library's commit.**
  A clone then fetches the libraries over the same protocol as the
  superproject, and a library stays a repository of its own, so `libs` is
  never staged as a whole. The site's submodule is the exception in the tree:
  its URL is the absolute SSH address
  `git@github.com:webcpporg/webcpporg.github.io.git`.
  Source: `AGENTS.md` (webcpp); `.gitmodules` (webcpp).
- **`main` is the only branch: a library's commit reaches its own `main`, then
  a superproject commit moves the submodule by path, and the superproject's CI
  tests every library at the commits its submodules record.**
  (reason not recorded) What fails there is fixed with a test, by more commits
  on `main`.
  Source: `AGENTS.md` (webcpp).
- **A repository whose code was moved from elsewhere starts from one import
  commit, without the history of the code it was moved from.**
  (reason not recorded) The tree's `AGENTS.md` limits the rule to moved code;
  the organization's specification and `CONTRIBUTING.md` still say every
  webcpp repository, and the superproject's own history has 276 commits.
  Source: `AGENTS.md` (webcpp).
- **`libs/config` is a library of its own, as Boost.Config is Boost's, and
  every library's `config.hpp` includes `<webcpp/config.hpp>` first.** The
  owner decided it on 2026-10-08, in the philosophy of `boost/config.hpp`: one
  header gives every library the target, whether webcpp has exceptions, and
  the version. In the tree every library but wasi includes it; wasi reads
  `WEBCPP_NO_EXCEPTIONS` itself until its redesign.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/release-design.md`
  (proposals).
- **One rulebook for every library, and each rule a tool can check is
  checked by one: the lint, a Jam rule that refuses a wrong declaration, or
  the report, each pinned by a test of `tools/`.** One rulebook, one build and
  one lint for every library keep a rule enforced by a tool rather than by
  memory; a rule that needs judgment is the reviewer's, and says so.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **The rulebook is two files: `CODING-GUIDELINES.md`, the single source of
  the rules on how code is written and of every design decision with its
  reason, and `AGENTS.md`, how to work in the tree.** A contributor, human or
  agent, reads in one place how code is written and why it is designed as it
  is, and a decision written once does not drift; `AGENTS.md` keeps what an
  agent needs to run anything. Until 2026-10-26 `AGENTS.md` alone was the
  rulebook, and the decisions were spread over the proposals, each library's
  `AGENTS.md` and its pages.
  Source: `organization/openspec/changes/coding-guidelines/design.md`, D1 and
  D2 (proposals).
- **Third-party code a library or its tests use is never copied into a
  library: it is a submodule under `third_party/libs/<name>`, pinned at a
  commit, and `third_party/Jamfile` declares it as
  `/webcpp/third_party//<target>`.** It keeps its own licence and notices, the
  lint never reads it, and moving its pin is a change of its own, with its
  evidence.
  Source: `AGENTS.md` (webcpp).
- **What a library builds on follows one order: what webcpp has, then Boost,
  the C++ Alliance's libraries, the standard library, an outside library that
  follows their standards, a C++ port of a widely used C library, and only
  then code of its own.** The aim is that every library carries the quality
  and the peer review of Boost and the standard; an outside library is wrapped
  behind an interface that follows its standard or Boost counterpart, so that
  it can be replaced.
  Source: `CONTRIBUTING.md` (webcpp).

**The build.**

- **b2 builds, tests, documents, serves and releases everything; a Python or
  Node script is a procedure a b2 target runs, never another entry point.** A
  CMake or Makefile front end was rejected: Boost's own libraries do not need
  one, and it would hold a second copy of the build knowledge.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **The superproject's `Jamroot` is the single build configuration: a library
  has no Jamroot, and its `build.jam` is found by the glob `libs/*/build.jam`
  and registered as `/webcpp/<name>`.** A library is developed inside a
  checkout of the superproject, as a Boost library is developed inside Boost,
  and nothing else registers it: its test, example and doc directories join
  the aggregates by themselves.
  Source: `AGENTS.md` (webcpp).
- **Boost 1.92 or newer is required, and the Jamroot refuses an older or
  unreadable Boost before it compiles anything, printing the `using boost`
  line to add.** (reason not recorded)
  Source: `organization/openspec/specs/organization/spec.md` (proposals).
- **The compiled Boost libraries a library needs, Boost.URL, Boost.Charconv,
  Boost.Locale without ICU and Boost.Cobalt, are built by webcpp from Boost's
  own source archive, for every target, as `/webcpp//boost_url` and its kin.**
  Native, wasip2, wasip3 and emscripten then build the same sources with the
  same macros; an installed Boost cannot be relied on (Homebrew's has no
  `libs/url/src`), and the sources' version must equal the configured Boost's.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Boost.JSON's definitions and Boost.Test's header-only framework are
  objects of webcpp's own build, `/webcpp//boost_json` and
  `tools/boost_test_runner.cpp`.** Boost.JSON is built once per variant, so
  that a user's build without exceptions builds it without exceptions too, and
  the Boost.Test object spares the CI any compiled `unit_test_framework`.
  Source: `AGENTS.md` (webcpp).
- **A tool the build looks up itself is taken from its `-s` option on b2's
  command line, else from `.local/`, else from `PATH`, and never from the
  environment.** A variable a shell keeps from another day would replace
  `.local/` without a word, and b2 tells the command line's value apart only
  through `ARGV`.
  Source: `AGENTS.md` (webcpp).
- **Every b2 run refuses a shell that sets `CPATH`, `CPLUS_INCLUDE_PATH` or
  `C_INCLUDE_PATH`.** The compiler would read another Boost from them before
  the configured one, without a word; Homebrew's shell often sets them.
  Source: `AGENTS.md` (webcpp).
- **Each toolchain is installed by hand and registered in `user-config.jam`,
  and the lines that register the WASI and emscripten toolsets are tagged
  regions of `tools/ci/wasi-sdk.jam` and `tools/ci/emsdk.jam`.** The CI writes
  a lane's toolset from those regions and a page includes them, so that
  neither drifts from the other; bundled toolchains are on the roadmap.
  Source: `AGENTS.md` (webcpp).
- **A library webcpp does not build yet (trystero's libsecp256k1,
  libdatachannel and OpenSSL) is declared with `webcpp.external`: a build that
  does not find it leaves its owner out with one line, and the CI and the
  lint, with `webcpp-require-external=on`, stop instead.** Left out, every
  other library still builds with `b2 test`; required, a library not found
  stops the build rather than leave the library out. The CI's actions build
  them until webcpp builds them from `third_party/`.
  Source: `AGENTS.md` (webcpp).

**Targets and toolchains.**

- **webcpp builds for three families of targets, each with a purpose:
  `emscripten` (the browser), `wasip2` and `wasip3` (WASI components run by
  wasmtime and by clouds) and `native` (a program that integrates closely with
  its operating system).** A C++ program becomes a web page with everything
  the browser offers, and a WASI component is a container lighter than Docker,
  for distributed computing at scale.
  Source: `AGENTS.md` (webcpp).
- **A program's targets are declared in its Jamfile, the target of a build is
  told by `<toolset>` and its version alone, and a program not declared for a
  target gets `<build>no`.** `<build>no` conditioned on a derived feature such
  as `<target-os>` never settles in b2's evaluation of conditional
  requirements; the declarations are what `b2 declared-targets` prints, from
  which the CI plans its lanes.
  Source: `AGENTS.md` (webcpp).
- **On emscripten every program is wasm32, single-threaded and static.**
  Browsers do not all run memory64, pthreads need cross-origin isolation in a
  page, and Emscripten's shared libraries are experimental.
  Source: `AGENTS.md` (webcpp).
- **wasip2 and wasip3 are b2's `clang` registered against wasi-sdk
  (`clang-wasip2`, `clang-wasip3`), linked statically, and the Jamroot adds
  `<target-os>wasi` only after the toolsets have loaded.** wasm32-wasip2 and
  wasm32-wasip3 are no system b2 knows and have no shared libraries, and the
  late value keeps gcc's Unix link options (`--start-group`, `-Bstatic`,
  rpath), which wasm-component-ld rejects, from being chosen.
  Source: `AGENTS.md` (webcpp); `Jamroot` (webcpp).
- **Every target builds with exceptions on and RTTI is never restricted; a
  build without exceptions is its builder's choice, which every library
  supports.** Whether a program uses exceptions is the choice of whoever
  builds it, never an imposition of a library or of webcpp.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **On wasip2 and wasip3 exceptions are standard WebAssembly exceptions
  (exnref): compiled with
  `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`, linked with
  `-fwasm-exceptions -lunwind`.** wasi-sdk 34 emits the legacy encoding by
  default, which wasmtime 47 refuses to run ("legacy_exceptions feature
  required"); a program that throws then needs wasmtime 47 or later, jco with
  exnref, or another Wasm 3.0 engine.
  Source: `AGENTS.md` (webcpp);
  `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Every tool has one pinned version: Boost 1.92.0 and the b2 it bootstraps
  (B2 5.5.3), wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0 with the WASI
  WIT of the crates `wasip2` 1.0.4 and `wasip3` 0.9.0, Emscripten 6.0.11,
  MrDocs 2026.9.29 and Node 26.7.0.** The lint takes clang-format and
  clang-tidy from the pinned wasi-sdk so that every machine and the CI lint
  with one version, and the reference refuses an emsdk that is not 6.0.11; why
  each of these versions was chosen is not recorded.
  Source: `organization/openspec/specs/organization/spec.md` (proposals).
- **Every archive the CI downloads is checked against a recorded SHA-256,
  every third-party action is pinned by its full commit SHA, and a tool
  fetched with git is checked against its pinned commit.** emsdk checks none
  of its downloads, a tag that moved (libdatachannel's) would otherwise have
  git fetch from the URLs its `.gitmodules` names, and the interop lanes get a
  pinned chrome-headless-shell so that a red lane is never a browser nobody
  chose.
  Source: `AGENTS.md` (webcpp).

**Documentation.**

- **Each library's API reference is generated by MrDocs from its Doc Comments,
  always to one AsciiDoc page that the library's page includes, with every
  warning an error.** The owner decided MrDocs, always to AsciiDoc; the single
  page (`multipage: false`, `embedded: true`) is included into the
  hand-written page with no warning, where the multipage output's `xref` links
  are Antora's, and `auto-function-metadata` and `auto-relates` are off
  because at their defaults they hid 48 findings on the first library
  measured.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/MRDOCS_FOR_WEBCPP.md`
  (proposals).
- **Each library has one AsciiDoc page, converted by Asciidoctor.js: its
  guide, its numbered guarantees, its testing chapter, the reference, the
  differences of a port and its changelog.** A guarantee keeps its number and
  anchor forever, since code, tests and comments cite them; why AsciiDoc and
  Asciidoctor.js were chosen is not recorded.
  Source: `AGENTS.md` (webcpp).
- **Documentation is generated and checked, never typed twice: every listing
  is a compiled example, every shown output a `.expected` file, every count
  computed by the build and every link between pages checked.** No listing,
  output or count then drifts from the tree, and a page includes the build's
  own configuration by its tag, never a copy.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals); `AGENTS.md` (webcpp).

**Tests and CI.**

- **A port's evidence is the pinned original itself: a shared oracle in
  `tools/oracle/` runs the original over the port's cases and the twins of its
  examples, and only `update-expected` writes an expected result.** The port's
  tests never state by hand what the original does; a library of webcpp's own
  is proven by its tests and the oracles that fit it, and each difference from
  an original is a recorded decision with an example and a twin.
  Source: `organization/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Unit tests use Boost.Core's lightweight_test, and Boost.Test is kept for a
  test that needs it, natively only.** lightweight_test was measured to
  compile and report correctly with `-fno-exceptions -fno-rtti` and on
  wasm32-wasip2 under wasmtime, where Boost.Test 1.92 does not compile.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_REGRESSION_MATRIX.md`
  (proposals).
- **Only a build from scratch, `b2 -a`, counts as evidence.** b2 compares
  timestamps, so an incremental build misses an edit made in the same second
  as the last build and can leave a stale result in place.
  Source: `AGENTS.md` (webcpp).
- **A lane builds and runs one library's tests and examples for one toolset,
  from scratch, with `--dump-tests` and `--out-xml`, in a build directory of
  its own.** `testing.launcher` applies to every toolset of one b2 request, so
  a lane is always one toolset; `--dump-tests` lists the tests a lane skips
  too, and two concurrent b2 runs never share a build directory.
  Source: `AGENTS.md` (webcpp).
- **The test matrix is written by webcpp's own `tools/report/report.py` from
  b2's `--out-xml`, and its exit status is the verdict.**
  boostorg/regression's tool chain was rejected: it needs Python 2 or a
  rewrite, a Boost checkout with five compiled libraries and manual
  workarounds, and its `library_status` mislabels columns; with `--out-xml` b2
  exits 0 even when a test fails.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_REGRESSION_MATRIX.md`
  (proposals).
- **What an ordinary lane must not run is an own lane of its library
  (`webcpp.lane`): an oracle, a served component, a driven test or the check
  of generated files; a lane-only program in no lane stops the CI's plan.** An
  oracle needs Node where a toolset lane does not, a generator may need the
  network, and a served or driven program left out of every lane would go
  untested rather than fail before any job runs.
  Source: `AGENTS.md` (webcpp).
- **The CI is one reusable workflow, `.github/workflows/library.yml`, which
  every library's `ci.yml` calls with its name and the superproject's `ci.yml`
  without one, and it runs exactly the commands the rulebook documents.** A
  lane is then run locally exactly as the CI runs it; why one reusable
  workflow serves every repository is not recorded.
  Source: `AGENTS.md` (webcpp).
- **The CI never lists a library's targets by hand: its plan job reads
  `b2 declared-targets` and `b2 declared-lanes`, and a target the CI has no
  lane for fails the plan by name.** A lane is then never left out.
  Source: `organization/openspec/specs/organization/spec.md` (proposals).
- **The lanes `gcc-14` and `clang-linux-18` run in a Linux container with no
  network but the loopback, built from an image whose every package comes from
  one dated Ubuntu snapshot.** A test that reaches beyond the machine fails
  there, which every library's tests hold and trystero's guarantee 6 rests on;
  the snapshot makes one Dockerfile one set of packages.
  Source: `AGENTS.md` (webcpp).
- **The native lanes are GCC 14 and 15, Clang 18 and 22 on libstdc++, Apple
  Clang and MSVC 14.3 and 14.5, beside one lane each for emscripten, wasip2
  and wasip3.** The Clang lanes on libstdc++ stay because a regression of
  xactor's guarantee 28 is caught only there; why the other compilers were
  chosen is not recorded.
  Source: `AGENTS.md` (webcpp).
- **A run on `main` is never cancelled by the next, and a pull request's newer
  push cancels its older run.** A run on `main` is the record of `main`, and
  it publishes the site.
  Source: `organization/openspec/specs/organization/spec.md` (proposals);
  `.github/workflows/ci.yml` (webcpp).

**Releases.**

- **webcpp has one version for the whole collection, written once as
  `WEBCPP_VERSION` in `<webcpp/version.hpp>` of `libs/config`, with Boost's
  formula (0.1.0 is `100`); a library has no version of its own.** Unlike
  Boost, which repeats its version by hand, the header is the only place, read
  by the Jamroot and by the release tooling; a port records its original's
  version elsewhere.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **The version moves to the next one in the commit right after a tag, which
  also opens the next notes from `tools/release/template.adoc`.** `main` then
  always names the release in preparation.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **Every change a user can see adds its changelog line in the same commit,
  and agents write the changelogs, which replaces for webcpp the global rule
  never to edit one.** The owner decided on 2026-10-08 that changelogs are
  written with every change, so that they are ready when a release comes; a
  library's changelog is the base of the release's "What's new".
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/release-design.md`
  (proposals).
- **A library's changes are written once, in its `doc/changelog.adoc`, and the
  release notes include that region by its tag instead of a copy.** Boost
  keeps two hand-written copies, a library's changelog and its entry in the
  release notes, and they drift; webcpp keeps one source per fact.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **The release notes follow Boost's sections, with a section Removed
  Libraries that Boost does not have.** The owner wants what went out written
  down.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **A version's Compilers tested lists only the CI lanes actually tested for
  that release, a nonempty ordered subset of the CI's lanes.** A local-only
  release does not claim coverage of other CI lanes; 0.1.0's lists GCC 15 and
  Emscripten 6.0.11.
  Source: `AGENTS.md` (webcpp).
- **`b2 check-release` fails a release in which a library changed since the
  last tag has no changelog line, besides checking the notes, the version and
  their form.** It is the check the owner asked for, and Boost has no
  equivalent.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **`b2 dist` writes a byte-for-byte reproducible `webcpp-<version>.tar.gz`
  and its `sha256sum` file, read from git's objects at the recorded commits,
  with the built pages and no dot file.** Anyone can rebuild it from the tag
  and compare the hash; it takes GitHub's form of name, since webcpp publishes
  on GitHub and the tag names it.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **The archive also carries the sources and upstream licences of the pinned
  submodules `site` and `third_party/libs/<name>` at their recorded commits,
  and refuses any other submodule by name.** Required sources are then never
  silently omitted.
  Source: `AGENTS.md` (webcpp); `tools/release/release.py` (webcpp).
- **A release is the tag `webcpp-<version>`, on the superproject and on every
  library at the commit the superproject records, and a GitHub Release that
  carries the archive and its checksum.** A library's release state is then
  found by its tag.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/research/BOOST_RELEASE_NOTES.md`
  (proposals).
- **Every release has a "What's new", the body of its GitHub Release, whose
  skeleton `b2 whats-new` writes from the release notes in Boost's form, with
  at most five highlights that an agent drafts and the owner approves before
  the tag.** The owner decided that every release has one, with what came in
  and went out, in the pattern of Boost's release notes.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/release-design.md`
  (proposals); `AGENTS.md` (webcpp).
- **Nothing is released until the owner asks, and whether a tag is signed is
  the owner's decision then.** (reason not recorded) 0.1.0 is prepared in the
  tree, and no tag names it yet.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/release-design.md`
  (proposals).

**Proposals and decisions.**

- **Every library, and every change to one, begins as an OpenSpec proposal in
  `webcpporg/proposals`, and enters webcpp only once it is accepted, built
  from that proposal.** (reason not recorded) The superproject's
  `CONTRIBUTING.md` still sends a new library to an issue on
  `webcpporg/webcpp`.
  Source: `CONTRIBUTING.md` (proposals).
- **The organization and each library are OpenSpec roots of their own
  (`organization/openspec/`, `libraries/<name>/openspec/`), and the
  organization's specification states as checkable requirements what every
  library inherits.** A library's proposal then states only what is specific
  to it, and an agent finds the rule it needs instead of reading the rulebook
  from the top.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/proposal.md`
  (proposals).
- **A library is of one of three kinds, `own`, `port` or `wrapper`, and each
  kind is proven by its own evidence.** A library of webcpp's own is proven by
  its specification, its tests and the references it cites; a port by its
  pinned original, run as an oracle; a wrapper by the wrapped library's own
  behaviour on every target webcpp builds for.
  Source: `CONTRIBUTING.md` (proposals).
- **A proposal carries its reference code before it is proposed, with its
  original specification and plan when there are any and a
  `reference/STATUS.md` of where the work stands.** The implementer starts
  from that code, and `STATUS.md` says how to resume the work.
  Source: `README.md` (proposals).
- **A proposal moves through draft, proposed, accepted, implementing and
  implemented, and a rejected or withdrawn one is archived with that status.**
  The reasons then stay readable.
  Source: `CONTRIBUTING.md` (proposals).
- **The maintainers decide a proposal after its public discussion on its pull
  request, until a governance of the members, through grants and a DAO, is
  specified by a proposal of its own.** (reason not recorded)
  Source: `CONTRIBUTING.md` (proposals).
- **Outward actions belong to the owner: an agent never pushes, merges,
  creates a repository, changes a GitHub setting or touches a credential, and
  a plan names each such act as a gate the owner resolves.**
  (reason not recorded)
  Source: `organization/openspec/specs/organization/spec.md` (proposals);
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).
- **History and recorded material are not rewritten: existing commits,
  archived proposals and recorded patches keep what they were made with.**
  They are records of what was done, and changing them would make them
  disagree with the commits they describe.
  Source:
  `organization/openspec/changes/archive/2026-10-13-contributor-identity/design.md`
  (proposals).

**Agents.**

- **The rules are written for coding agents and humans alike, so that an agent
  with no other context can port a library by following them, and nothing in
  webcpp depends on one agent system.** Each library is ported with the help
  of coding agents; an agent system that reads `AGENTS.md` at a repository's
  root finds it there, and others are pointed at it.
  Source: `CONTRIBUTING.md` (webcpp); `AGENTS.md` (webcpp).
- **Every contributor, human or agent, commits under their own git identity,
  an agent under that of the account it acts for or its own, never another
  person's; the rules fix no address.** A fixed identity cannot be followed by
  a second maintainer or an agent of another account without impersonation,
  and keeps a personal address in the rules; a shared organization identity
  was rejected because it hides who made a change, which the review and the
  history need.
  Source:
  `organization/openspec/changes/archive/2026-10-13-contributor-identity/design.md`
  (proposals).
- **A commit carries no trailer of any kind, stages explicit paths and has a
  plain subject prefixed by the part it touches.** These rules serve the
  history's readability, whoever's name is on a commit.
  Source:
  `organization/openspec/changes/archive/2026-10-13-contributor-identity/design.md`
  (proposals).
- **Agent skills live in `.agents/skills/<name>/SKILL.md`, the vendor-neutral
  layout, in the superproject and in the proposals repository only.** Several
  agents read that layout and OpenSpec writes its own skills there; one
  directory per agent product was rejected as a copy of every skill, and a
  library keeps what is specific to it in its own `AGENTS.md`.
  Source:
  `organization/openspec/changes/archive/2026-10-13-agent-skills/design.md`
  (proposals).
- **A skill points to the chapter that holds a rule and lists the steps and
  commands of its task; it never restates the rule, and the rule wins where
  the two disagree.** A skill that copied the rules would drift from
  `AGENTS.md` at the first change of a rule.
  Source:
  `organization/openspec/changes/archive/2026-10-13-agent-skills/design.md`
  (proposals).
- **The skill `webcpp-text-and-commits` is one file written twice, identical
  in both repositories, and a change to either is compared with `cmp` and made
  to both.** A symbolic link across repositories does not exist in a clone,
  and an agent reads the files of the repository it is in.
  Source:
  `organization/openspec/changes/archive/2026-10-13-agent-skills/design.md`
  (proposals).
- **OpenSpec's six skills are generated by OpenSpec 1.10.0 in a copy of the
  proposals repository and committed with two edits, keeping their MIT front
  matter; `openspec init` never runs in the repository itself.** `init` would
  create a root `openspec/`, which the repository's several roots rule out,
  and the kept front matter leaves the origin of the text visible.
  Source:
  `organization/openspec/changes/archive/2026-10-13-agent-skills/design.md`
  (proposals).
- **CI parses every skill's front matter as YAML and refuses a name that
  differs from its directory and a missing or blank description.** It catches
  the unquoted colons in descriptions that the review found.
  Source:
  `organization/openspec/changes/archive/2026-10-13-agent-skills/design.md`
  (proposals).

### The site (webcpporg.github.io; increment 1 built)

Increment 1 of the site's specification of 2026-10-10, as built: the site's
repository at `f1b4832`, which the superproject's `site/` records at
`ddb6cb2`, the superproject's `tools/site/` and `tools/ci/assemble.py`, and
the CI's jobs `site`, `publish` and `pages`. Increments 2 to 8 are the
roadmap; 4 and 5 are the next section.

**The repository and the build.**

- **The site is a repository of its own, `webcpporg/webcpporg.github.io`,
  checked out in the superproject as the submodule `site/`, as a library is
  under `libs/`.** The site is webcpp built with webcpp, a project that uses
  webcpp's libraries the way any user would, so that anyone who opens its
  repository on GitHub sees it. Unlike a library's, its submodule URL in
  `.gitmodules` is the absolute SSH address
  `git@github.com:webcpporg/webcpporg.github.io.git`, not a relative one.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals); `.gitmodules` (webcpp).
- **The whole site is built by one command at the superproject's root,
  `b2 site`, which calls the site's npm scripts through one wrapper rule,
  `site.npm`, and never reimplements them; only `b2 site`'s output is ever
  deployed.** (reason not recorded) It is the owner's decision 13 of
  2026-10-10: b2 also drives Emscripten for the site's C++, and npm alone
  still builds and runs the JavaScript parts for work on the site.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **What the superproject holds of the site is its assembly, `tools/site/` and
  `tools/ci/assemble.py`, and the libraries' pages stay Asciidoctor's, given
  the site's header and `assets/chrome.css` alone.** The libraries'
  documentation system stays one, the site's frame around it being OpenUI's,
  and a page Asciidoctor styled keeps its look.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals); `organization/openspec/specs/site/spec.md` (proposals).
- **Every step of `b2 site` writes `bin/site.tmp`, and the finished build
  moves into `bin/site.builds/`, onto which the link `bin/site` is swapped in
  one rename.** A server on `bin/site` reads one build or the other whole,
  never half of one, and a failed step leaves the last good build in place.
  Source: `AGENTS.md` (webcpp).
- **Every page of `bin/site` is valid HTML, checked by html-validate in two
  passes, the site's own pages against `htmlvalidate.json` and the libraries'
  Asciidoctor pages against `htmlvalidate.docs.json`, each rule turned off
  with its reason in the `Jamfile`.** The two configurations have no leading
  dot since the site's commit `d113e38`, so that a source release, which
  carries no dot file, keeps them; the specification still names them
  `.htmlvalidate.json` and `.htmlvalidate.docs.json`.
  Source: `Jamfile` (site).
- **No build or test reaches the network beyond `npm ci` from the lockfile;
  `npm run dev` alone downloads the examples' wasm from the preview, checked
  against the preview's `precache.json`.** (reason not recorded) A downloaded
  wasm whose hash `precache.json` does not list is refused.
  Source: `organization/openspec/specs/site/spec.md` (proposals).

**Channels, indexing and search.**

- **The site has two channels from the same command: the preview, built from
  every push to `main` and served at https://webcpporg.github.io/, and the
  release, built from a release tag for https://webcpp.org/.**
  (reason not recorded) They are the owner's decisions of 2026-10-10; the
  release channel is built but not published, and waits for the first release
  and the move of the domain's DNS.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **The preview is never indexed: every page of it, the libraries' and the
  report's included, carries `noindex`, it writes no `sitemap.xml`, and its
  `robots.txt` still lets every crawler in.** The preview is the development
  version (owner, 2026-10-11), and a crawler let in reads each page's
  `noindex`; Google's search and Search Console belong to webcpp.org.
  Source: `organization/openspec/specs/site/spec.md` (proposals).
- **The release's `robots.txt` allows every crawler, `Google-Extended`
  included, and names its `sitemap.xml`.** Google's AI then learns webcpp's
  documentation.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **Search is Google's, restricted to the site: until the owner creates the
  Programmable Search engine for webcpp.org, the search page is a form whose
  hidden `q` is `site:<channel host>`.** (reason not recorded) The engine and
  its id are the owner's to create, with the release site.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **SEO is part of done: every page's head is written from its `Page(...)` and
  the catalogue, and `seo/queries.json` names the searches each page answers,
  which a test holds to the page's title, `<h1>` or description.** The owner's
  aim is that Google ranks webcpp for C++ developers who search for what
  exists to develop C++ for the web.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).

**Pages.**

- **Every page is written in OpenUI Lang and rendered by the site's React
  components, each with its props validated by a zod schema, at build time
  with `renderToString` and in the browser by hydration; without JavaScript a
  reader gets one `<noscript>` message.** The owner decided that the site is a
  JavaScript application whose content OpenUI renders, its components the
  design system Claude Design imports; the first paint shows the page before
  the scripts run and lets a crawler that runs no script read it.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **The site is static files: no server, no runtime API, no cookie and no
  analytics, and its one third-party destination is Google's search, reached
  on the reader's submit.** (reason not recorded) It is the owner's decision 7
  of 2026-10-10.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **The bundles are production builds, OpenUI's lang-core telemetry is
  replaced by a module that does nothing, and `npm ci` runs with telemetry
  off.** In development OpenUI's web entry mounts a devtools widget fetched
  from a CDN, and lang-core posts to PostHog when the environment asks; with
  the replacement no reader's browser and no build can reach them.
  Source: `organization/openspec/specs/site/spec.md` (proposals).
- **Every link between pages is relative; the indexing files' base URL is the
  one place a URL of the site is absolute.** The site serves from whichever
  path it is published under, as under `/webcpp/` during the move or a fork's
  `/<repo>/`.
  Source: `AGENTS.md` (site);
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-plan.md`
  (proposals).
- **No value the build knows is typed on a page, and no page shows a number or
  an example that does not exist: the catalogue is read from every library's
  `meta/libraries.json` and its declared targets, and live examples appear
  only as a roadmap.** (reason not recorded)
  Source: `AGENTS.md` (site);
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-plan.md`
  (proposals).
- **Layouts are written mobile first, from 360 px up, with breakpoints at 768,
  1280 and 1440 px, in light and dark themes.** The owner wants the site
  mobile first and good on the desktop too, where developers read most.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **The social cards and the icons are rendered by the build from the logo's
  SVG, with a pinned font and no system font.** A card is then the same bytes
  on every machine, and no image is drawn twice.
  Source: `organization/openspec/specs/site/spec.md` (proposals).

**Readers.**

- **The site speaks to four readers, C++ developers, JavaScript developers,
  crypto developers and AI agents, through a libraries page listed as Boost
  lists its own and three doors, `cpp/`, `javascript/` and `ai/`.** The site
  is where webcpp shows its idea, where C++ and the web meet, and leads its
  reader to the libraries' documentation; the owner wants a section for
  JavaScript developers always.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).
- **The site writes `llms.txt` and `libraries.json` at its root for agents.**
  AI agents are first-class readers of webcpp: they use what it publishes to
  find libraries of high quality.
  Source: `organization/openspec/specs/site/spec.md` (proposals).
- **The site's repository has two ways in, each for its reader: a JavaScript
  developer works with npm alone, or builds the whole project step by step,
  and a C++ developer uses b2 alone; `test/readme_test.mjs` keeps the README
  so.** Neither reader is sent through the other's tools, and a JavaScript
  developer changes pages, components and the examples' JavaScript without a
  C++ toolchain.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).

**Publication.**

- **The superproject's CI builds the site on every run, a red matrix included,
  and on a push to `main` publishes it as the one commit of an orphan
  `gh-pages` branch of the site's repository, with that repository's deploy
  key.** The matrix is there to show what fails; `gh-pages` is build output,
  replaced whole, and the runs on `main` queue rather than cancel, so that an
  older site never replaces a newer one.
  Source: `.github/workflows/ci.yml` (webcpp).
- **The old address, https://webcpporg.github.io/webcpp/, redirects every path
  to the same path at the root, its query and fragment kept.** No published
  link breaks.
  Source:
  `organization/openspec/changes/archive/2026-10-07-genesis/reference/site-spec.md`
  (proposals).

## 15. The libraries' design

The design decisions of each library of the superproject, as its tree has
them, each with its reason and its source. A source is a path in its
repository: `libs/<name>` is the library's repository at the commit the
superproject pins, `webcpp` the superproject at `ddb6cb2`, and `proposals`
[webcpporg/proposals](https://github.com/webcpporg/proposals), under
`libraries/<name>/openspec/` unless the path says otherwise. A library's own
`AGENTS.md` holds what an agent needs to work on it, with the rules of its own
that its specification asks of it; the decisions behind them, with their
reasons, are here. Where a design document, or the library's `AGENTS.md`,
disagrees with the tree, the section says what the tree does, and chapter 19
records the disagreement.

### config (the collection's configuration macros; own; implemented in 0.1.0)

config is webcpp's counterpart of Boost.Config: `<webcpp/config.hpp>` names
the target a program is built for and says whether webcpp has exceptions, and
`<webcpp/version.hpp>` gives webcpp's version. Every webcpp library's
`<webcpp/<name>/config.hpp>` includes `<webcpp/config.hpp>` first, so config
is read before anything else of webcpp.

**Scope and dependencies.**

- **config is a library of webcpp's own, with a repository, a `build.jam`,
  metadata, a page and a CI file like every other library.** Every library
  needs the same three answers (the target, exceptions, the version) before it
  compiles a line of its own. The rejected alternative, each library reading
  `__wasip2__` and `BOOST_NO_EXCEPTIONS` itself, repeats the logic in every
  library and lets the answers drift apart.
  Source:
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`<webcpp/config.hpp>` includes `<boost/config.hpp>` and adds only what is
  webcpp's.** What Boost.Config already answers (compilers, the standard
  library, `BOOST_NO_*`) is not repeated, and a program that includes config
  has Boost.Config's macros too.
  Source: `include/webcpp/config.hpp` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **config is macros only: it declares no namespace, no type and no
  function.** Boost.Config is macros only, and config is to cost nothing: the
  same on native, wasip2, wasip3 and Emscripten, with nothing beyond Boost's
  headers.
  Source: `libraries/config/openspec/specs/config/spec.md` (proposals);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`/webcpp/config//config` is an alias with no sources whose usage
  requirements are `<include>include` and `<library>/boost//headers`.** config
  needs Boost's headers and nothing to build or link.
  Source: `build.jam` (libs/config).

**Names.**

- **config's macros are the collection's, `WEBCPP_*`, never `WEBCPP_CONFIG_*`,
  and its include guards are `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`.**
  Boost.Config's macros are `BOOST_*` in the same way. It is the one exception
  to `WEBCPP_<NAME>_*` and to a library's namespace, and the reference tooling
  treats config's prefix as `WEBCPP_` and requires no namespace of it.
  Source: `AGENTS.md` (libs/config); chapter 3 (this file).
- **config reserves the macro families `WEBCPP_HAS_*`, `WEBCPP_LIB_*`,
  `WEBCPP_NO_*`, `WEBCPP_TARGET_*` and `WEBCPP_VERSION*`, so no library is
  named `target`, `has`, `no`, `lib`, `version` or `test`, alone or followed
  by an underscore and more, in any case.** Such a library's `WEBCPP_<NAME>_*`
  macros would meet config's, or the tests' `WEBCPP_TEST_*`;
  `tools/doc/libraries.py` refuses the name.
  Source: `doc/introduction.adoc` (libs/config); `tools/doc/libraries.py`
  (webcpp).
- **config has exactly two public headers, `<webcpp/config.hpp>` and
  `<webcpp/version.hpp>`, named as `<boost/config.hpp>` and
  `<boost/version.hpp>` are, and `<webcpp/version.hpp>` lies outside
  `webcpp/config/`.** So does `<boost/version.hpp>` in Boost. The
  headers-alone test therefore does not take it, and the test `version`
  compiles it alone, as the first include of its translation unit.
  Source: `AGENTS.md`, `test/Jamfile` (libs/config).

**Targets.**

- **`<webcpp/config.hpp>` defines exactly one of `WEBCPP_TARGET_WASIP2`,
  `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` and
  `WEBCPP_TARGET_NATIVE`, each a plain definition without a value, testing
  `__wasip2__`, then `__wasip3__`, then `__EMSCRIPTEN__`, and naming the
  target native when the compiler defines none of them nor `__wasm__`.** A
  library chooses its code by one macro of webcpp's rather than by the
  compiler's own; the order of the tests has no recorded reason. Measured with
  wasi-sdk 34 on 2026-10-09: wasm32-wasip2 defines `__wasi__`, `__wasm__` and
  `__wasip2__`, wasm32-wasip3 likewise with `__wasip3__`, and Emscripten
  6.0.11 defines `__EMSCRIPTEN__`.
  Source: `include/webcpp/config.hpp`, `doc/targets.adoc` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The target macros come from the compiler's own macros alone, never from
  the build system.** A program compiled by hand, with a plain `clang++` or
  with wasi-sdk's, gets the macro b2 would give it.
  Source: `doc/targets.adoc`, `AGENTS.md` (libs/config).
- **A WebAssembly build for none of the three WebAssembly targets (the
  compiler defines `__wasm__`: wasm32-wasip1, wasm32-unknown-unknown) stops
  with the preprocessor error "unsupported target: webcpp builds for
  wasm32-wasip2, wasm32-wasip3, Emscripten and natively".** Read as native, it
  would compile code for a system the program does not run on (owner,
  2026-10-09). A new target gets a macro, a branch and a test of its own.
  Source: `include/webcpp/config.hpp` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The target block comes before `#include <boost/config.hpp>`, and a test
  pins the order.** wasm32-unknown-unknown has no C++ standard library, and
  Boost.Config stops there at a header that is not there: config's message
  must come first.
  Source: `include/webcpp/config.hpp`, `doc/targets.adoc`, `test/Jamfile`
  (libs/config).
- **A target macro or `WEBCPP_HAS_EXCEPTIONS` defined before the header stops
  the build with the single error "WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS
  are defined by <webcpp/config.hpp> alone", and the rest of the header is the
  `#else` of that test.** No macro is then defined a second time, and that
  message is the only one the build reports.
  Source: `include/webcpp/config.hpp` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).

**Exceptions.**

- **`WEBCPP_HAS_EXCEPTIONS` is defined when neither `BOOST_NO_EXCEPTIONS` nor
  `WEBCPP_NO_EXCEPTIONS` is defined.** Boost.Config already reads whether the
  compiler has exceptions (`-fno-exceptions`, which b2's
  `exception-handling=off` gives), so config adds only the developer's switch,
  and every library tests one macro of webcpp's.
  Source: `include/webcpp/config.hpp`, `doc/exceptions.adoc` (libs/config).
- **`WEBCPP_NO_EXCEPTIONS` is the developer's alone: config never defines it,
  and, defined on the command line or in a b2 requirement, it turns off the
  exceptions of every webcpp library at once, even in a build that has them.**
  A program sets it once, for every library alike, without knowing which of
  them throws; the rejected alternative, a per-library switch only, would make
  it know (owner, 2026-10-09, option (a)). A developer may still define one
  library's `WEBCPP_<NAME>_NO_EXCEPTIONS` alone.
  Source: `doc/exceptions.adoc` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`WEBCPP_NO_EXCEPTIONS` does not define `BOOST_NO_EXCEPTIONS`: Boost's own
  code keeps the exceptions the compiler has.** The rejected alternative would
  change the behaviour of Boost's headers in a build whose compiler supports
  exceptions. The test `exceptions_developer_off` pins it.
  Source: `doc/exceptions.adoc`, `test/exceptions_test.cpp` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).

**Version.**

- **`WEBCPP_VERSION` is a number, `major * 100000 + minor * 100 + patch`
  (0.1.0 is `100`, 1.2.3 is `100203`), and a macro rather than a constant.**
  It is numbered as `BOOST_VERSION` is, and a macro so that `#if` compares it.
  Source: `include/webcpp/version.hpp`, `doc/version.adoc` (libs/config).
- **`WEBCPP_LIB_VERSION` is a string literal, `"<major>_<minor>"`, with
  `"_<patch>"` after it only when the patch level is not 0 (`"0_1"` is
  0.1.0).** It is written as `BOOST_LIB_VERSION` is.
  Source: `include/webcpp/version.hpp`, `doc/version.adoc` (libs/config).
- **The version has three numbers, and its tag is `webcpp-0.1.0`.** Boost's
  tags have three numbers (`boost-1.92.0`).
  Source:
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The version is written in one place, the line `#define WEBCPP_VERSION` of
  `include/webcpp/version.hpp`, and nothing follows the number on that line.**
  The Jamroot and `tools/release/release.py` read that line itself and refuse
  anything else on it, a trailing comment included: hence the
  `NOLINTNEXTLINE(modernize-macro-to-enum)` on the line above it.
  Source: `include/webcpp/version.hpp` (libs/config); `AGENTS.md`, chapter 5,
  "Releases" (webcpp).
- **The version moves to the next one only by the release procedure, right
  after a tag.** Between two releases it names the version in preparation.
  Source: `AGENTS.md`, `doc/version.adoc` (libs/config).

**Documentation.**

- **`config.hpp` holds an `#ifdef __MRDOCS__` block that defines and at once
  undefines `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`,
  `WEBCPP_TARGET_EMSCRIPTEN` and `WEBCPP_NO_EXCEPTIONS`, each under its Doc
  Comment.** MrDocs parses webcpp natively and with exceptions, where only
  `WEBCPP_TARGET_NATIVE` and `WEBCPP_HAS_EXCEPTIONS` are defined, and the
  reference lists all eight macros.
  Source: `include/webcpp/config.hpp` (libs/config).
- **A compiler's macro in a Doc Comment is written with an escaped leading
  underscore, `\__wasip2__`.** Asciidoctor then keeps both underscores in the
  reference.
  Source: `include/webcpp/config.hpp` (libs/config);
  `libraries/config/openspec/specs/config/spec.md` (proposals).

**Evidence.**

- **Every test is compiled and never run, with three rules of
  `tools/webcpp.jam`: `webcpp.headers-alone`, `webcpp.compile` (checks by
  `#error` and `static_assert`) and `webcpp.compile-diagnostic` (clang
  `-verify` against the error a source states).** What config does, it does
  when a program is compiled.
  Source: `test/Jamfile`, `doc/testing.adoc` (libs/config).
- **The diagnostic tests, `predefined_*` and `unsupported_*`, are built on
  b2's clang toolsets alone, and GCC, MSVC and Emscripten skip them, as grey
  cells of the test matrix.** `-verify` is clang's; the macros they pin are
  the same source on every compiler.
  Source: `doc/testing.adoc` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **Each `predefined_*` test is built with warnings as errors off.** A macro
  that config went on to define a second time is then a warning `-verify`
  sees, not an error it ignores.
  Source: `test/Jamfile` (libs/config).
- **The unsupported-target tests build wasm32-wasip1 and
  wasm32-unknown-unknown with wasi-sdk's clang (on the wasip2 target, with an
  added `--target`), and `__wasm__` defined alone natively.** The native one
  runs in every native clang lane; the wasm32-unknown-unknown one pins that
  the target block comes before Boost.Config. The order of the two `--target`
  options on the compile line was measured first, so that clang takes the
  added one.
  Source: `test/Jamfile` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/tasks.md`
  (proposals).
- **The test `target` is told the toolset by `test/Jamfile`
  (`WEBCPP_TEST_TARGET_*`) and requires the matching `WEBCPP_TARGET_*`, and
  exactly one.** The toolset tells the test, and the compiler tells config.
  Source: `test/Jamfile`, `test/target_test.cpp` (libs/config).
- **`exceptions_test.cpp` is built three times (as the build asks, with
  `exception-handling=off`, with `WEBCPP_NO_EXCEPTIONS`), and the build
  without exceptions first requires `BOOST_NO_EXCEPTIONS`.** A flag that was
  not applied is refused rather than passed as a build without exceptions; the
  two builds without the developer's macro also refuse a
  `WEBCPP_NO_EXCEPTIONS` that config defined itself.
  Source: `test/Jamfile`, `test/exceptions_test.cpp` (libs/config).
- **`version_test.cpp` checks by `static_assert` that `WEBCPP_LIB_VERSION` is
  digits separated by single underscores, names `WEBCPP_VERSION`'s major and
  minor numbers, and holds the patch level exactly when it is not 0.** The two
  macros must name one version.
  Source: `test/version_test.cpp` (libs/config).
- **Every test and the example are declared for the four targets,
  `webcpp.targets native wasip2 wasip3 emscripten`.** config is to be the same
  on every target webcpp builds for.
  Source: `test/Jamfile`, `example/Jamfile` (libs/config);
  `libraries/config/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).

### xactor (a deterministic actor system; own; implemented in 0.1.0)

xactor runs actors that handle one message at a time, on the host's thread, in
an order that depends on the order of enqueuing alone; every execution is paid
from a budget of fuel, and time is a number the host brings. Its code is the
actor layer of an earlier private project, moved, renamed `webcpp::xactor` and
re-tested; webcpp's asynchronous libraries are to be built on it.

**Scope and dependencies.**

- **xactor is a library of webcpp's own, not a port, and its evidence is its
  own tests, with no oracle.** It ports nothing, so there is no original to
  run its cases: its tests came with its code from the earlier project and
  were converted with an equivalence record.
  Source:
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **xactor stands alone: no header, comment or example of it names another
  webcpp library, and its code depends on Boost's headers and config alone.**
  The libraries built on xactor depend on it, never the reverse, and every
  asynchronous library webcpp ports or writes is to run on it; the page names
  xstate once, as a library that builds on it.
  Source: `AGENTS.md` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/proposal.md`
  (proposals).
- **`/webcpp/xactor//xactor` is an alias that adds `include`,
  `/boost//headers` and `/webcpp/config//config`: xactor uses Boost.System for
  `result` and the error codes, Boost.Asio for `asio_driver` alone, and
  compiles nothing.** Boost.System gives the `result` every fallible call
  returns; only the driver that runs turns as Asio handlers needs Boost.Asio,
  so only it includes Boost.Asio.
  Source: `build.jam`, `doc/introduction.adoc` (libs/xactor).
- **What an actor system may add on top of xactor is left out: threads and a
  clock of its own, a child's end reaching its parent, a registry of actors by
  name, the inspection of what each actor does, supervision, and the
  persistence of actors.** They are left to the program or to a library built
  on xactor (reason not recorded). A child that must tell its parent that it
  ended sends it a message, to `turn.parent()`, in the turn it ends.
  Source: `doc/introduction.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).

**Model and interface.**

- **The program's messages are one closed `std::variant` of plain structs, and
  every type of xactor that carries a message is a template over it.** Closed
  means every message is known when the program is compiled, so `std::visit`
  checks that every alternative is handled, and the log records which
  alternative arrived by its `index()`.
  Source: `doc/model.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **xactor asks of `Message` only that it be a `std::variant` that can be
  moved: it need be neither copyable nor default-constructible.** The library
  names no message, id or error of any user (guarantee 28). The requirement is
  stated in the documentation, and the templates are unconstrained (reason not
  recorded).
  Source: `doc/guarantees.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).
- **An actor's behaviour is a class derived from `actor_logic<Message>`, whose
  one pure virtual function is
  `result<void> handle(turn<Message>&, const envelope<Message>&)`.** Any class
  with a `handle` is an actor, whether its behaviour is decided at run time or
  is a state machine compiled into the program, so xactor needs no adapter for
  a state machine library.
  Source: `include/webcpp/xactor/actor_logic.hpp`, `doc/static-actors.adoc`
  (libs/xactor).
- **`actor_logic` is neither copyable nor movable, and the scheduler owns each
  logic through a `std::unique_ptr`, where it was constructed, and never moves
  it.** A test that constructs a logic and hands it to `spawn` keeps a pointer
  to it and reads its members after each delivery.
  Source: `include/webcpp/xactor/actor_logic.hpp`, `doc/drivers.adoc`
  (libs/xactor).
- **A logic never sees the scheduler, its mailbox or the driver: all it does,
  it does through its `turn`, which lives for one `handle` and holds
  references to the scheduler and to the message.** The turn knows the actor
  that acts and the execution of the message, so a message the actor sends
  carries it as its sender and is paid from that execution. A logic keeps
  neither the turn nor a reference into the message beyond the call.
  Source: `doc/turns.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).
- **What a logic is constructed with is the actor's input and its members are
  the actor's state: `create_actor<Logic>(system, inputs...)` and
  `turn.spawn_child<Logic>(inputs...)` forward their arguments to the
  constructor of `Logic`.** `create_actor` deduces `Message` from the
  scheduler, so a call names the logic alone.
  Source: `doc/logic.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).
- **An actor is `active` as soon as it is created, with no separate start.**
  (reason not recorded) A caller that needs a start delivers the actor its
  first message.
  Source: `include/webcpp/xactor/scheduler.hpp` (libs/xactor).
- **An actor's address, an execution and a message's place are distinct
  structs, `actor_ref` (a `std::uint32_t`), `correlation_id` and
  `sequence_number` (each a `std::uint64_t`), not plain integers.** An address
  is then never passed where an execution is expected. The widths have no
  recorded reason.
  Source: `include/webcpp/xactor/ids.hpp` (libs/xactor).
- **An `actor_ref` is a number, not a handle: addresses start at 1 in spawn
  order, address 0 names no actor and is the `from` of a message the host
  sent, and an address is never reused.** It keeps nothing alive, and an actor
  that has ended keeps its address and its status, which `status_of` reports.
  Source: `include/webcpp/xactor/ids.hpp` (libs/xactor).

**Scheduling and concurrency.**

- **Each actor has a FIFO mailbox, and the actors with messages waiting wait
  in one FIFO queue, each at most once; `run_one` takes the actor at the
  front, hands it the first message of its mailbox, and puts it back at the
  end while its mailbox holds more, so a message sent in a turn is never
  handled inside that turn.** The order of deliveries is then a function of
  the order of enqueuing alone, which makes a run deterministic and lets every
  driver write one log, and no actor starves another however much it sends
  itself.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/scheduler.adoc`
  (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **xactor runs on one thread, the host's: the scheduler starts no thread, no
  turn blocks, and it keeps no static mutable state.** A run is a function of
  its deliveries, and xactor runs unchanged on wasm32-wasip2 and
  wasm32-wasip3, where there is no thread to lean on; two schedulers in one
  process are independent.
  Source: `doc/scheduler.adoc`, `doc/determinism.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/proposal.md`
  (proposals).
- **The scheduler keeps each actor in a slot of one `std::vector`, at the
  index of its address minus one, with its logic, its parent, its children in
  spawn order, its mailbox (a `std::deque`), its status, its output and its
  error.** Addresses are never reused and an ended actor keeps its status,
  output and error. A pointer `output_of` returns stays valid only until the
  scheduler takes in another actor, since the vector may grow then.
  Source: `include/webcpp/xactor/scheduler.hpp` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).

**Drivers.**

- **`scheduler::run_one` is the one step, and a driver only decides when to
  call it.** The scheduler decides what a turn is and the driver when it
  happens, so the same actors run under a plain loop, one step at a time in a
  test, or as Asio handlers, and record one envelope log.
  Source: `include/webcpp/xactor/drivers.hpp`, `doc/drivers.adoc`
  (libs/xactor).
- **`fifo_driver` offers `run_until_idle(log)` and `run_turn(n, log)`, which
  delivers at most `n` messages.** `run_turn` serves a host that has to take
  the thread back between turns.
  Source: `include/webcpp/xactor/drivers.hpp` (libs/xactor).
- **`test_driver` offers `step()` and `step(log)`, each one `run_one`, and
  nothing else.** A test sees exactly what one delivery changed; messages and
  time reach the scheduler through `deliver`, `clock_tick` and `release_next`.
  Source: `include/webcpp/xactor/drivers.hpp`, `doc/drivers.adoc`
  (libs/xactor).
- **`asio_driver` owns an `io_context` of concurrency hint 1, a work guard and
  a strand, posts each `run_one` onto the strand, and drains the context with
  `poll`, never with `run`, `run_one` or `run_for`; the context offers no
  executor.** Under the work guard `run` would never return, and `run_one` and
  `run_for` would block waiting for more work, so a turn never blocks. The
  context is the driver's alone: a program's own I/O objects run on a context
  of their own.
  Source: `include/webcpp/xactor/drivers.hpp`, `doc/drivers.adoc`
  (libs/xactor).
- **`asio_driver` and Boost.Asio's headers are declared only under
  `#ifndef __wasi__`, each include marked
  `// lint-world: posts handlers only`.** Boost.Asio 1.92 does not compile for
  wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34 (no `ESHUTDOWN`, no
  `::pause`, and a `signal.h` that stops with `#error`). The driver posts
  handlers and makes no I/O object, so it reaches no clock, file or socket.
  Source: `include/webcpp/xactor/drivers.hpp`, `example/Jamfile`
  (libs/xactor).
- **`run_one` returns a `result<bool>`, and the drivers forward its failure,
  though no failure of the scheduler exists today.** A failure of the
  scheduler itself then has a channel the drivers already forward; a handler's
  error ends its actor, not the run.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/scheduler.adoc`
  (libs/xactor).
- **An `envelope_log` entry, `logged_envelope`, holds the sequence number,
  `to`, `from`, the correlation, the execution's fuel at delivery and the
  payload's `index()`, never the message; `run_one` records it before the
  actor handles the message, and nothing for a message dropped.** An entry has
  a fixed width, the log is one type whatever the `Message`, and two logs
  compare cheaply, with `==`, which is how two runs or two drivers are shown
  to deliver alike.
  Source: `include/webcpp/xactor/envelope_log.hpp`, `doc/drivers.adoc`
  (libs/xactor).

**Fuel.**

- **Every message belongs to an execution, all the messages one host delivery
  causes, named by the `correlation_id` the host chooses; an execution's fuel,
  `budgets::fuel` (a `std::uint32_t`, 1,000,000 by default), opens with the
  first `deliver`, `clock_tick` or `release_next` that names it.** Every
  execution ends: no chain, fan-out or ping-pong of messages runs forever.
  Fuel is kept for each execution, not globally, so one input's work is
  bounded whatever another input does, and a host gives each input a
  correlation of its own.
  Source: `include/webcpp/xactor/budgets.hpp`, `doc/fuel.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Nothing refills an execution: the scheduler keeps every execution it
  opened in a `std::map` keyed by correlation, and a later delivery with the
  same correlation shares what is left.** The work of one execution is then
  bounded, whatever its actors send.
  Source: `include/webcpp/xactor/budgets.hpp`,
  `include/webcpp/xactor/scheduler.hpp` (libs/xactor).
- **`send`, `reply` and `wake_at` cost one unit, `spend(n)` costs n units, and
  every other call of a turn, a host's delivery and the release of a timer
  cost nothing.** Releasing a timer is free since arming it was paid, and
  `spend` lets an actor that computes without sending bound its work too. Why
  a host's delivery is free is not recorded.
  Source: `doc/fuel.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).
- **Payment is all or nothing: a call the execution cannot pay fails with
  `errc::resource_limit` and enqueues, arms and spends nothing, and a refused
  keyed `wake_at` leaves the timer under its key armed.** A refusal is a value
  the logic decides about: it goes on, or returns the failure to end its actor
  with it.
  Source: `doc/fuel.adoc`, `doc/guarantees.adoc` (libs/xactor).
- **A message to an actor that has ended is dropped and the call succeeds, a
  `send` being paid all the same, while a call that names no actor fails with
  `errc::invalid_argument`, and a `send` so refused is not paid.** The sender
  cannot know that the receiver has ended, so dropping is not an error of the
  sender's.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/turns.adoc`
  (libs/xactor).

**Time and timers.**

- **xactor reads no clock: time is a `std::uint64_t` that the host brings with
  `clock_tick` or `release_next`, in a unit of the host's choosing, and no
  timer fires without one of them.** A run with timers is then as repeatable
  as one without, and a test moves time by any amount at once.
  Source: `doc/timers.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).
- **Armed timers live in one `std::map` keyed by (deadline, arming sequence),
  and due timers are released in deadline order, equal deadlines in arming
  order.** No two entries collide, and a tick releases timers in one defined
  order on every platform.
  Source: `include/webcpp/xactor/scheduler.hpp` (libs/xactor).
- **`clock_tick` releases every due timer, and `release_next` only the
  first.** A host that runs the system between two calls of `release_next`
  lets one timer's handling cancel or replace a later timer of the same tick,
  which `clock_tick`, releasing them all before any is handled, cannot.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/timers.adoc`
  (libs/xactor).
- **A released timer's message belongs to the execution the releasing call
  names, and its sender is the actor itself.** (reason not recorded) Its
  release costs nothing, since arming it was paid.
  Source: `doc/timers.adoc`, `doc/guarantees.adoc` (libs/xactor).
- **`wake_at(deadline, payload, key)` replaces the same actor's timer under
  `key`, the replacement taking a new arming order, and `cancel_timer(key)`
  removes it for free.** Keys belong to their actor, so two actors may use the
  same key; why a replacement takes a new arming order is not recorded.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/timers.adoc`
  (libs/xactor).
- **A tick earlier than the current time is not refused: time and `now()` move
  back.** (reason not recorded) A host that wants time to move forward only
  never brings an earlier time.
  Source: `doc/timers.adoc` (libs/xactor).
- **`timer::payload` has no default member initializer.** A `Message` need not
  be default-constructible, and with libstdc++ `std::erase_if` asks whether a
  timer is, which makes Clang instantiate the initializer; a move-only message
  test (guarantee 28) and the Clang lanes on libstdc++ catch a regression.
  Source: `include/webcpp/xactor/scheduler.hpp`, `test/CONVERSION.md`
  (libs/xactor); `AGENTS.md`, chapter 4 (webcpp).

**Lifecycle.**

- **An actor is `active`, `done`, `error` or `stopped`, an
  `enum class status : std::uint8_t` with the fixed values 1 to 4, and the
  three last are final: an actor that ends loses its pending messages, its
  place in the queue and its timers, and every later message to it is dropped,
  never delivered and never logged.** The values never change and are never
  reused, because a system that persists its actors rebuilds them from these
  numbers.
  Source: `include/webcpp/xactor/status.hpp`, `doc/lifecycle.adoc`
  (libs/xactor).
- **An actor spawned in a turn is a child of that turn's actor, recorded in
  spawn order, and `scheduler::stop` and `turn::stop_child` stop a whole
  family, each actor after its own descendants, siblings in spawn order, and
  return the actors stopped in that order.** The order is one a caller may
  rely on, and an explicit stack (a preorder that visits the last child first,
  reversed) follows a family of any depth without recursion.
  Source: `include/webcpp/xactor/scheduler.hpp` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **`turn::stop_child` accepts only a child of the caller, and a member of the
  family that has already ended stays as it ended while its descendants are
  still stopped.** (reason not recorded) The caller itself, a grandchild and
  an address that names no actor are refused with `errc::invalid_argument`,
  which changes nothing.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/lifecycle.adoc`
  (libs/xactor).
- **`turn::finish` and `turn::stop` end the calling actor alone, its children
  running on, and xactor does not tell a parent that a child ended.** (reason
  not recorded) The children are the ending logic's to stop with `stop_child`,
  and a child that must tell its parent sends it a message.
  Source: `doc/lifecycle.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **An actor ends once: a second `finish` or `stop` fails with
  `errc::invalid_argument`, and an error its `handle` returns after the turn
  ended it changes nothing.** (reason not recorded) The actor stays as it
  ended, with its output or none, and `error_of` stays empty.
  Source: `include/webcpp/xactor/scheduler.hpp`, `doc/lifecycle.adoc`
  (libs/xactor).
- **In the rest of the turn that ended its actor, a logic can still `send`,
  paid as usual, but `wake_at` and `spawn_child` fail with
  `errc::invalid_argument` before anything is paid or constructed.** (reason
  not recorded)
  Source: `doc/turns.adoc`, `include/webcpp/xactor/scheduler.hpp`
  (libs/xactor).

**Errors and exceptions.**

- **xactor's own errors are two, `errc::invalid_argument = 1` (a call that
  names what it cannot act on) and `errc::resource_limit = 2` (the execution
  has no fuel left for it), and a call that fails with either changes
  nothing.** The values start at 1, so an `error_code` of value 0, which
  `error_of` returns for an actor that has not failed, is no error; they are
  fixed and never reused.
  Source: `include/webcpp/xactor/errors.hpp` (libs/xactor).
- **The category `webcpp.xactor` is a `constexpr` constant with the fixed
  identity `0x786163746f720000`, the ASCII of "xactor" followed by two zero
  bytes.** It holds no mutable state.
  Source: `include/webcpp/xactor/errors.hpp` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **A `handle` may return any `error_code`, of xactor's category or of its
  own, which ends its actor alone with the status `error` and which the
  scheduler keeps as it is for `error_of`.** Every other actor keeps its turns
  and the run goes on: `run_one` still returns `true`.
  Source: `include/webcpp/xactor/actor_logic.hpp`, `doc/logic.adoc`
  (libs/xactor).
- **xactor catches nothing: an exception that a `handle` throws leaves
  `run_one` and the driver in the middle of the run.** (reason not recorded)
  The page's rules of determinism tell a handler to return an error instead.
  Source: `doc/determinism.adoc` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Where exceptions are on, the operations that allocate (`spawn`, `deliver`,
  `send`, `wake_at`, the log's `record`) can still throw `std::bad_alloc`.**
  They do as a standard container does; the guarantee without exceptions is
  that xactor compiles without them, which the lint checks.
  Source: `doc/guarantees.adoc`, `doc/changelog.adoc` (libs/xactor).
- **`WEBCPP_XACTOR_NO_EXCEPTIONS` is defined as for every library although it
  changes nothing in xactor.** xactor raises no exception of its own; the
  macro exists so that one setting serves every webcpp library alike.
  Source: `include/webcpp/xactor/config.hpp`, `doc/testing.adoc`
  (libs/xactor).
- **xactor makes no promise about running without RTTI, and is built with
  it.** The promise was removed before 0.1.0, a breaking change, when the
  owner decided on 2026-10-08 that no variant is built without RTTI.
  Source: `doc/changelog.adoc`, `test/CONVERSION.md` (libs/xactor);
  chapter 6 (this file).

**Memory.**

- **Every allocation goes through the standard allocator (`std::make_unique`,
  `std::vector`, `std::deque`, `std::map`, `std::string`), and xactor takes no
  allocator or memory resource from its user.** xactor predates chapter 4's
  allocation rule, which a review is to bring it to first; actors' state and
  messages in a shared or relocatable segment are not part of 0.1.0.
  Source: `include/webcpp/xactor/scheduler.hpp` (libs/xactor); chapters 4 and 16
  (this file).

**Targets.**

- **xactor's tests and examples are declared for native, wasip2 and wasip3,
  and `xactor_asio` for native alone.** xactor must run unchanged natively and
  on WebAssembly, where Boost.Asio does not compile; why emscripten is not
  declared is not recorded.
  Source: `test/Jamfile`, `example/Jamfile` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/proposal.md`
  (proposals).

**Static actors.**

- **A logic that runs a state machine holds it as a member and hands it each
  message wrapped in an event type of the program's, `on<Event>`, which holds
  the message and the turn by reference.** Nothing in xactor knows the
  machine's library and that library does not know xactor, so any library
  whose machines take typed events joins the same way; the `on<Event>` lives
  for one call of the machine, so its references never outlive the turn.
  Source: `doc/static-actors.adoc`, `example/xactor_msm.cpp` (libs/xactor).
- **The example machine is written with Boost.MSM, used through its headers
  alone, never through `/boost/msm//boost_msm`.** Boost.MSM is one of Boost's
  libraries; its b2 target brings every library MSM depends on,
  Boost.Serialization among them, which is compiled.
  Source: `doc/static-actors.adoc` (libs/xactor).

**Evidence.**

- **The tests are Boost.Core lightweight_test programs, one per subject
  (`scheduler`, `lifecycle`, `fuel`, `create_actor`, `errors`,
  `sequence_and_log`), each case a function of the file's anonymous namespace
  named after the rule it checks, called from `main`.** lightweight_test is
  one header and builds on WASI too, where the tests run; a `static` function
  would have the same linkage, but clang-tidy's `misc-use-anonymous-namespace`
  rejects it.
  Source: `test/Jamfile`, `test/CONVERSION.md` (libs/xactor);
  `libraries/xactor/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **A case that cannot go on after a failed check returns,
  `if (!BOOST_TEST(...)) { return; }`, and a helper calls
  `webcpp::test::require`, which ends the program with
  `std::exit(boost::report_errors())`.** A helper cannot return from its
  caller's case, and a test program may be built without exceptions, so it
  cannot throw out of it either.
  Source: `test/require.hpp`, `test/CONVERSION.md` (libs/xactor).
- **The suites converted from Boost.Test carry an equivalence record,
  `test/CONVERSION.md`: 55 cases, 295 assertions in the source and 636 at run
  time, old and new equal, and three planted defects that fail the same cases
  in both suites.** The conversion can then be reviewed assertion by
  assertion.
  Source: `test/CONVERSION.md` (libs/xactor).
- **Determinism is proven by equal envelope logs: a `fifo_driver` and an
  `asio_driver` run of the same deliveries, a run cut by `run_turn` against
  one stepped by `test_driver`, and two runs of one input.** Two schedulers
  built with the same budgets and fed the same deliveries record equal logs
  (guarantees 15 and 17), and the log records every delivery as it was made.
  Source: `test/scheduler_test.cpp`, `example/xactor_drivers.cpp`,
  `example/xactor_testing.cpp` (libs/xactor).
- **The programs that use `asio_driver` sit under the header's own condition,
  `#ifndef __wasi__`, and every other test and example includes
  `<webcpp/xactor.hpp>`.** The WASI build of every other program is then the
  check that nothing of Boost.Asio enters a WASI build.
  Source: `doc/testing.adoc`, `AGENTS.md` (libs/xactor).

**Documentation.**

- **The page states 36 numbered guarantees under the anchors
  `xactor-invariant-<n>`, which tests and code cite as
  `(doc: #xactor-invariant-<n>)`, and the doc build fails on a citation that
  names no anchor.** A guarantee keeps its number forever, since code, tests
  and comments cite it; a new guarantee takes the next number.
  Source: `doc/guarantees.adoc` (libs/xactor); `AGENTS.md`, chapter 3
  (webcpp).
- **Every listing of the page includes a compiled example by `tag::` markers
  and every output its `.expected` file, and the page states no count of the
  tree's.** `example/Jamfile` runs every program the page shows and compares
  its output; a count typed by hand goes stale.
  Source: `AGENTS.md`, `example/Jamfile` (libs/xactor).
- **The reference is built strict, and `doc/mrdocs.yml` sets
  `show-enum-constants: true`.** The values of `errc` and `status` are fixed,
  and the enum's table of members would show each enumerator's brief alone.
  Source: `doc/mrdocs.yml`, `AGENTS.md` (libs/xactor).

### xstate (machines and actors; port of XState 5.33.2; implemented in 0.1.0)

webcpp::xstate runs the JSON configs of XState's `createMachine` in C++20, in
two layers: a machine core, `<webcpp/xstate.hpp>`, that steps a machine by
XState's pure functions or one microstep at a time with a macrostep cursor,
and an actor layer, `<webcpp/xstate/actors.hpp>`, that runs machines as
XState's actors on webcpp::xactor. It is the model port of webcpp's oracle and
twins, and its appendix "Where xstate differs from XState" lists each of its
departures, every one of which is a decision below.

**Scope.**

- **xstate ports XState 5.33.2's algorithm function by function, and each
  ported function names in its Doc Comment the TypeScript function it ports.**
  A difference from XState then points at one function on each side; xstate
  has no semantics of its own.
  Source: `doc/introduction/overview.adoc` (libs/xstate).
- **The original is pinned exactly, the npm package `xstate` at 5.33.2 with
  its lockfile, and moving to another version of XState is a change of its
  own.** The machine semantics, the order of actions and microsteps, and the
  errors follow that version, and the oracle's new expected results are the
  evidence for a move.
  Source: `AGENTS.md` (libs/xstate).
- **0.1.0 leaves out what a JSON config and a single-threaded, queued actor
  system cannot or did not yet carry: `enqueueActions`, callback and
  observable actors, `fromTransition`, the persistence of actors, `toPromise`
  and `waitFor`, `syncSnapshot`, `spawn` inside `assign`, `machine.provide()`,
  `options.maxIterations`, `xstate/graph` and the SCXML converter.** What is
  not ported is a gap, not a divergence, so the overview lists it rather than
  the appendix of differences; a stored snapshot comes back through
  `resolve_state`, a second `create_machine` with other implementations stands
  for `provide`, and a test walks a machine with `get_next_transitions` and
  `get_next_snapshot`.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Two gaps let a config load and then run otherwise than in XState, and the
  appendix names them: an invoke's `onSnapshot` loads as transitions on
  `xstate.snapshot.<id>` that no child's snapshot ever takes, and an
  `xstate.init` or `xstate.stop` sent from outside selects transitions as any
  other event does.** XState's `syncSnapshot` is not ported, and its
  macrostep's special handling of those two events is not followed yet;
  whether `onSnapshot` should rather be refused at creation is an open
  question.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **Compile-time state machines are left to libraries such as Boost.MSM, run
  as xactor actors; xstate interprets machines that are data.** Those
  libraries already do it well and a third inside xstate would duplicate them,
  and xactor needs no adapter, since an actor logic is any class with a
  `handle`.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).

**Machines as data.**

- **A machine is a `boost::json::value` in XState's config format, read when
  the program runs.** It can come from a file, a request or Stately Studio's
  export, and XState can run the same config, which is what lets XState judge
  the port; a machine described in C++, by a builder or a table of types, was
  rejected because it cannot come from a file or from Stately Studio and
  XState cannot run it.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **Every value a machine holds (its config, context, an event's payload, an
  action's `params`, a state value, an input and an output) is a Boost.JSON
  value.** Boost.JSON's `object` keeps its members in the order they were
  written, which the key-order rules need; its limits (a hashed index, numbers
  as doubles, a value that holds pointers) are accepted for 0.1.0, and a
  document type of webcpp's own is the owner's direction for a later change.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **What JSON cannot hold is C++ registered by name in `implementations`, as
  XState's `setup()` registers it, in seven members: `context`, `actions`,
  `actors`, `guards`, `delays`, `inputs` and `outputs`.** A name is the bridge
  between the data and the code, and XState already joins them by name, so a
  config written with `setup()` keeps its shape in xstate.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **An action name the implementations do not hold is a custom action,
  returned as `{type, params}` with `builtin` false, for the caller to
  perform; it is not an error.** What a machine does to the world belongs to
  the program that runs it: the machine says what is to be done, and the
  program decides how.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **An implementation receives an `action_args` (the context, the event and
  the params) and returns a `result`; it receives no actor and no system,
  where XState's does.** A context is JSON and holds no actor, and an effect
  that needs one is a custom action, which the host performs, or an
  `emit_action`; the cost is that a custom action carries its fixed `params`
  alone.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **Where XState's config holds a function, xstate's holds another form:
  `params` are fixed JSON, an invoke's `src` is the string name of an `actors`
  entry (any other `src` is `invalid_config`), and the context function
  receives the input only.** A function cannot be JSON, logic cannot be JSON,
  and a reference to an actor is not data.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **XState's higher-order guards are JSON objects of xstate's own, `{"type":
  "xstate.not", "guards": [...]}` (and `xstate.and`, `xstate.or`) and
  `{"type": "xstate.stateIn", "stateValue": ...}`, which `to_json` writes.**
  JSON holds no function and XState has no JSON for them; writing them lets a
  machine loaded back from its definition keep them.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A `sendTo` target is a string (a child's id, `#_<id>`, `#_parent`,
  `#_internal` or `#system:<systemId>`), and an event or an input carries no
  actor reference.** JSON holds neither a reference nor a function; a child
  answers its parent through `#_parent`, and any other actor is named by its
  id or its systemId.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **JavaScript's `undefined` is an empty `std::optional`: a `value_maker`
  returns `result<std::optional<boost::json::value>>`, a member computed as
  none is left out, and a null input is the same as no input.** JSON has
  `null` only, and a member left out is how XState itself writes `undefined`.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **An event is `struct event { std::string type; boost::json::object payload;
  }`, read from and written to XState's flat form by `event_from_json` and
  `to_json`.** Transitions match the type, and the payload is data for the
  implementations.
  Source: `doc/state-machines/transitions.adoc` (libs/xstate).
- **`create_machine` checks all it can when the machine is created, and
  refuses configs XState accepts and fails on later: an `initial` that names
  no child, a guard or a named `after` delay without an implementation, a
  default entry that leads back to its state, a null `entry`, `exit` or
  `actions`, an unknown actor, a malformed history target or `tags`.** The
  moment a machine is loaded is the moment its author can be told what is
  wrong with it, not when an event reaches the broken part, long after and
  perhaps in production.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **Two things are checked only when they are used, as in XState: the `#id` of
  a `stateIn` guard, which fails the macrostep with `unknown_state`, and the
  default of a history state at the root, which fails it with
  `unknown_target`.** XState accepts both configs, and a history at the root
  has no parent to resolve its default against.
  Source: `libraries/xstate/openspec/specs/xstate/spec.md` (proposals).
- **Implementations that hold an empty `std::function` refuse the machine with
  `invalid_config`.** Calling one would abort in a build without exceptions.
  Source: `include/webcpp/xstate/implementations.hpp` (libs/xstate).
- **A `machine` is a handle to immutable data shared by every copy,
  `std::shared_ptr<const detail::machine_data>`, with no default constructor
  and no `provide`.** A copy is cheap, so a cursor, an actor and a
  `machine_actor` each keep one; a program that runs a config with other
  implementations creates a second machine from it with the first's
  `registry()` changed.
  Source: `include/webcpp/xstate/machine.hpp` (libs/xstate).
- **A `machine_actor` holds its machine through a `std::shared_ptr<const
  machine>`.** A machine holds its implementations, which may hold this actor.
  Source: `include/webcpp/xstate/implementations.hpp` (libs/xstate).
- **A machine's nodes are one list in document order, named by index; its
  guards are one flat list, a higher-order guard naming its operands by index;
  and every recursion of the core is an explicit list or stack.** The tree
  holds no pointer and copies as a value, and a machine as deep as its JSON
  never exhausts the stack.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **`to_json` writes the definition XState's `machine.toJSON()` writes, and
  `create_machine_from_definition` loads one back, given the implementations
  and the context.** A definition holds names, not code, and no context, so
  both are given again; a machine written so becomes data that can be stored
  and loaded.
  Source: `libraries/xstate/openspec/specs/xstate/spec.md` (proposals).

**Fidelity to JavaScript.**

- **Where XState's observable behaviour is a property of JavaScript, xstate
  states it as a rule; the first is that object keys are enumerated array
  indices first, by number, then in insertion order, for `states`, `on`,
  `after`, state values and `children`.** A port must reproduce it to match
  the oracle: it decides the document order, the order of a state's
  transitions and the order of the raises an `after` schedules.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **The active nodes are an insertion-ordered set, `node_set`, and a microstep
  that leaves the machine done lists them by descending `order`.** XState
  sorts its array of active nodes in place when the machine is done, and the
  order decides which eventless transition a parallel region takes first.
  Source: `doc/guarantees.adoc` (libs/xstate).
- **XState's comparison of snapshots by identity is stated as "changed": after
  an eventless microstep, the eventless transitions are selected again only
  when the active nodes, the history, the context, the children or the status
  changed.** That is what XState's loop does, which selects them again when a
  microstep returned another snapshot than the one it began from.
  Source: `doc/guarantees.adoc` (libs/xstate).
- **Timers of equal deadline fire by (deadline, scheduling order), in the
  core's `simulated_clock` and in the actor layer's `clock_tick`, and the
  latest timer armed under an id is the one a cancel finds.** XState's order
  comes from V8's sort with a comparator that never returns 0; xstate states
  the result instead of leaving it to a sort.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A snapshot's status is `active`, `done` or `error`, without XState's
  `stopped`; `actor_system::status_of` tells that an actor stopped.** xstate
  runs `xstate.stop` as any other event, where XState gives `stopped` to a
  snapshot that receives it.
  Source: `include/webcpp/xstate/snapshot.hpp` (libs/xstate).
- **A config's context that is not an object is kept as written, and an
  `assign` on it fails with `implementation_failed`; a null or absent context
  is `{}`.** Truthiness and the spreading of a string are properties of
  JavaScript, a JSON machine whose context is not an object has a fault,
  reported where it shows, and `null` is how JSON writes no value.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A state's `meta` that is null is no meta, and a state's `description` that
  is not a string is left out.** A `null` stands for no value, and a
  description is text, as XState's own type declarations make it.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A child spawned without an id is keyed `""`, where XState keys it
  `"undefined"`, and a `stop_child_action` of `""` removes it.** A C++ string
  has no undefined: an id left out holds the empty string, a key a stop can
  name.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **The core returns a `sendParent` as `xstate.sendTo` to `#_parent` even for
  a machine without a parent, where XState's pure functions throw; the actor
  layer fails a root that sends to its parent with `unknown_target`.** Whether
  a machine has a parent depends on where it runs, not on the machine, and the
  pure functions describe the machine.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **xstate makes, in every build, the three checks of XState's development
  build that a JSON machine can meet (an external event of the type `*`, a
  truthy `cond`, a `forwardTo` to no actor), and writes no warning.** Each
  check stops an error that the production build lets through to do something
  worse, and xstate writes to no stream: what it has to tell its caller comes
  back as a value.
  Source: `doc/appendices/differences.adoc` (libs/xstate).

**Errors.**

- **`errc` has ten values, from `invalid_config` (1) to `system_id_taken`
  (10), and no `invalid_argument`: the actor layer refuses a call it cannot
  take with xactor's `invalid_argument`, and `system_id_taken` is its one
  refusal of its own.** The values start at 1 so that the empty `error_code`
  of a snapshot that has not failed is no error, and the actor layer refuses a
  call as xactor would.
  Source: `include/webcpp/xstate/errors.hpp` (libs/xstate).
- **The category's `message(int)` is the enumerator's spelling,
  `"implementation_failed"`, and `"unknown"` for any other value; a failed
  machine child sends its parent its `error_value` when an unhandled error
  event failed it, else that name, and its snapshot keeps both the code and
  the reported error.** An `error_code` carries no value of its own, where
  XState's implementation may throw any value, so the code's name is the JSON
  that says which failure it was, and a grandchild's error climbs unchanged.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A failure settles the macrostep at once with the snapshot it began from,
  its `status` `error` and its `error` the code, in the pure functions as in
  an actor; the failing microstep reports no action, and
  `progress::resolved_before_failure` holds what it resolved before the
  failure.** That snapshot is the one XState's actor keeps when an
  implementation throws, a failure in it cannot pass unseen, and a cursor's
  caller learns what a running actor of XState has already run.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **An implementation may fail with any `error_code`, which becomes the
  snapshot's `error` as it is.** A program tells its own failures apart, and a
  code carries its category, so the code of another library's call travels
  unchanged.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).

**The cursor and time.**

- **The macrostep cursor is the core's one primitive: `begin` and
  `begin_initial` return a `macrostep` whose `next()` runs exactly one
  microstep, `settled` is true on the closing microstep and no other, and
  XState's pure functions are the cursor run to its end.** A program that runs
  machines it did not write cannot know that each one settles, so the caller
  decides how much work an event may cost; the unit is XState's microstep, so
  a run stops only between two of its steps.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **A cursor is a value, holding a copy of its machine handle, its snapshots
  and its queue, so a copy resumes exactly where the original stopped.**
  Stopping keeps the work: a caller that runs out of fuel keeps the cursor and
  resumes it later, perhaps in a later request.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **The core never stops a macrostep on its own, and XState's
  `options.maxIterations` is not read.** A bound written in the config is its
  author's, unset by default, and ends the macrostep with a thrown error; the
  bound belongs to the program that runs the machine.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **The core reads no clock: a delayed transition comes back as a built-in
  `xstate.raise` with a delay, for a clock its caller owns.** Time is an
  input, so a delayed transition is tested without waiting, and the program
  decides what time is.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **The core's `simulated_clock` holds events, not functions: `apply`
  schedules the built-in delayed raises and applies the cancels, `pop_due()`
  hands the caller each due event, and `set` refuses a time before `now()`
  with `invalid_config`.** A clock that called a function would run the
  machine from inside the clock's call; one that hands over each due event
  lets the caller apply what its delivery returns before it takes the next,
  and a refusal is a value.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **Time is whole milliseconds and a delay a `std::uint64_t`; an `after` key
  is a number only when it is digits alone, and any other key that starts with
  a digit, a sign or a dot is `invalid_config`, even when a delay of that name
  is registered.** xactor's timers count whole milliseconds, which keeps every
  deadline an exact integer; a fraction has no timer to hold it, and a
  negative delay has no meaning.
  Source: `doc/appendices/differences.adoc` (libs/xstate).

**The actor layer.**

- **The library has two layers behind two convenience headers: the machine
  core knows XState's actors as data and resolves an invoke or a spawn into
  `xstate.spawnChild` and `xstate.stopChild` actions, and the actor layer,
  whose header includes the core's, executes them.** XState splits `resolve`
  from `execute` at the same place, and a program that only steps machines
  needs no scheduler and includes nothing of xactor or Boost.Asio.
  Source: `doc/introduction/overview.adoc` (libs/xstate).
- **The boundary is mechanical: `meta/include-boundaries.json` forbids the
  core's headers to include `webcpp/xactor` or `webcpp/xstate/actors`, which
  the lint enforces by file and line, and `test/core_alone.cpp` fails on any
  of their include guards.** The core stays usable by a program that does not
  want a scheduler.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **The actor layer runs on xactor rather than on an actor system of its own:
  each machine actor is an xactor actor whose logic drives the cursor, and the
  layer adds a child's done and error events, invoke, the systemId registry,
  emitted events and inspection.** A second system would duplicate xactor's
  scheduler, timers and fuel, and a machine's actor inherits xactor's
  determinism and bound on work.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **A delivery between actors is queued, never nested in the sender's call, so
  the order across actors can differ from XState's, never within one actor.**
  Queues make a run deterministic and replayable, keep one actor's work from
  nesting inside another's, and let fuel bound each turn.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **Every microstep, every message an actor sends and every timer it arms
  costs one unit of the execution's one reservoir of fuel; an actor that
  cannot pay parks, and `resume()` continues it.** A machine loaded at run
  time cannot hang the host, and one reservoir for each execution bounds a
  fan-out, whose every branch would start with almost the whole budget under a
  count for each message.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **While an actor is parked, the host's `start`, `send`, `clock_tick`,
  `resolve` and `reject` are checked, held in order and applied by `resume()`;
  `stop` is never held.** No event starts before the macrostep in progress has
  settled, and `stop` is how the host ends an actor that would never settle.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **`create_actor` runs nothing: the root is constructed, its initial
  macrostep run, on `start`.** Construction runs a macrostep, which costs
  fuel, and fuel is paid inside an execution, which only a host call that
  delivers something opens.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **The roots of one `actor_system` share one system, its systemIds and its
  count of session ids `x:<n>`, where XState gives each root a system of its
  own.** An `actor_system` owns one xactor scheduler, with its registry and
  its counter, and nothing of it is global; a program that wants XState's
  arrangement gives each root an `actor_system` of its own.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **`actor_system` is neither copyable nor movable, and one thread runs it.**
  Every actor holds a reference to the system's shared state.
  Source: `include/webcpp/xstate/actors/actor_system.hpp` (libs/xstate).
- **`clock_tick` accepts a time earlier than the last one and moves the time
  back, where the core's `simulated_clock` and XState refuse it.** The actor
  layer's timers are xactor's and keep xactor's rule; the core's clock ports
  XState's `SimulatedClock` and keeps its refusal.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **The host's callbacks run inside an actor's turn, and a call from one of
  them that changes the system returns xactor's `invalid_argument` and changes
  nothing; the `const` readers are free.** A call made inside a run cannot
  know that run's outcome, and running the system again from inside a turn
  would nest one run in another, where xactor runs one turn at a time.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A `host_actor` stands for `fromPromise`: once started it is a pending
  `host_request`, which the host answers with `resolve` or `reject` when it
  chooses.** The host does the work a promise would do, and a test is the host
  and has nothing to wait for.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).
- **Effects cross between the system and the host as data alone: custom
  actions reach the one `on_action` callback, logs the one `on_log` callback,
  emitted events their actor's `on_emitted` listeners, and snapshots its
  subscribers.** The host owns the effects, and nothing else crosses.
  Source: `doc/actors/actors.adoc` (libs/xstate).
- **`on_emitted` adds one kind of listener, which hears every event the actor
  emits in the order the listeners were added, an event of the type `*`
  once.** One kind of listener, heard in the order the program added it, is
  the whole rule, a type is one test inside it, and hearing an event twice, as
  XState's lookup makes a listener do, is a defect.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **The layer does not reproduce four of XState's accidents: it stops the
  unstarted children of a failing macrostep, drops every timer of an actor
  that ended, fires a timer that falls due during a `clock_tick` in the same
  call, and stops a root's whole family, a failed member's children
  included.** A child never started and never stopped would hold its systemId
  for good, an actor that has ended sends nothing, whether a due timer fires
  cannot depend on how many timers a flush had left to visit, and a stop that
  leaves actors running does not end the family.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **A `sendTo` that names an invoke of the state being entered carries
  `action::bound_at`, and the actor fails with `unknown_target` when no child
  has that id where XState binds it, where XState relays the event to
  `undefined` and throws a `TypeError`.** A send to an actor that is not there
  is an error of the machine, reported with the code of every other target
  that names no actor.
  Source: `doc/appendices/differences.adoc` (libs/xstate).
- **`actor_message` is a `std::variant` of nine alternatives in a fixed
  order.** xactor names an alternative in its log by its index, so the order
  is part of a recorded run.
  Source: `include/webcpp/xstate/actors/message.hpp` (libs/xstate).

**Build and targets.**

- **xstate is header-only, and the one part a program compiles is Boost.JSON's
  definitions, once, in a translation unit that includes
  `<boost/json/src.hpp>` (webcpp's `/webcpp//boost_json`).** The same headers
  build with every C++20 compiler and for WebAssembly with no binary to match,
  a build's flags (no exceptions, a sanitizer) are the library's, and xactor's
  scheduler and logic are templates in any case.
  Source: `doc/appendices/rationale.adoc` (libs/xstate).
- **The one b2 alias, `/webcpp/xstate//xstate`, brings xactor to every user,
  so a program of the machine core depends on xactor in name only.**
  (reason not recorded)
  Source: `libraries/xstate/openspec/specs/xstate/spec.md` (proposals).
- **The tests and examples are declared for native, wasip2 and wasip3;
  Emscripten is not a target of 0.1.0.** (reason not recorded)
  Source: `libraries/xstate/openspec/specs/xstate/spec.md` (proposals).
- **The code is under the Boost Software License alone, and what derives from
  XState (the ported semantics, the transcribed test cases, the twins and the
  shared examples) keeps XState's MIT notice, unchanged, in
  `LICENSE-XSTATE.txt`.** The derived parts carry their original's notice,
  which adds no licence to xstate's own code.
  Source:
  `libraries/xstate/openspec/changes/archive/2026-10-07-genesis/design.md`
  (proposals).

**Evidence.**

- **The oracle runs XState's development build only, `node
  --conditions=development`, and `test/oracle/development.mjs` refuses any
  other by checking that an event of the type `*` throws.** XState's own tests
  run that build, and xstate makes the checks it makes.
  Source: `test/oracle/development.mjs` (libs/xstate).
- **`oracle.mjs` runs each machine case's macrosteps after the initial one
  through `machine.transition` with an actor scope of its own whose actor has
  a parent, takes the initial microsteps from `getInitialMicrosteps`, and
  delivers delayed events from XState's `SimulatedClock`.** XState's pure
  functions cannot serve: their actor has no parent and throws on
  `sendParent`, and XState's inspection does not report the first initial
  microstep.
  Source: `doc/testing.adoc` (libs/xstate).
- **`actors.mjs` writes, for every actor keyed by its path of ids and a rank
  (`root/n#2`), the events it processed with each snapshot, the events it
  emitted and its status, never the order between two actors.** xactor's
  queues may change that order, while no nesting of deliveries changes the
  keys.
  Source: `doc/testing.adoc` (libs/xstate).
- **XState's tests are ported as JSON cases, one file per XState test file,
  each listing every test it leaves out under `excluded` with its reason;
  files of xstate's own pin what XState does where its tests are silent, and
  the implementations a case names come from a vocabulary written twice, in
  `test/oracle/vocabulary.mjs` and `test/vocabulary.hpp`.** A test that cannot
  be ported is never dropped in silence.
  Source: `doc/testing.adoc` (libs/xstate).
- **The suite `cases` runs each macrostep through the cursor, one microstep a
  call and at most 10,000, and compares every microstep with XState's, then
  the definition, then, where the definition holds the whole machine, the
  machine loaded back from it.** It runs a machine as a caller that pays fuel
  runs one.
  Source: `doc/testing.adoc` (libs/xstate).
- **The two case suites are Boost.Test programs built natively only, and every
  other test is a lightweight_test program run on native, wasip2 and wasip3.**
  The suites read the fixture files at run time, which a wasm program cannot
  without a directory its runtime opens, and register one test case per case
  so that a failure names it; a lightweight_test program runs as it is in a
  build without exceptions.
  Source: `doc/testing.adoc` (libs/xstate).
- **Every example has a twin on XState that agrees or diverges on purpose, 114
  and 22 of the 146, or a line in `without-twin.txt`, 10: the cursor, fuel, a
  machine reloaded from its definition, an event read from JSON and the
  registry of inputs and outputs.** The twins hold the page to XState, and
  what XState has no counterpart for cannot have a twin.
  Source: `test/oracle/twins/without-twin.txt` (libs/xstate).
- **The twins of the testing chapter are tests of Node's own runner,
  `node:test` with `node:assert`, where Stately's page uses Vitest.** So that
  each runs on Node alone.
  Source: `doc/testing.adoc` (libs/xstate).
- **The five actor test programs and `actors_cases` are built with `/bigobj`
  on MSVC, a decision made from section counts taken with Clang 18 and GCC
  14.** Their objects stay near the COFF limit of 65,279 sections that the old
  suite needed `/bigobj` for, and MSVC was not available to measure them.
  Source: `test/CONVERSION.md` (libs/xstate).
- **The actor layer's 73 test cases are split by responsibility into five
  programs, `actors_lifecycle`, `actors_sending`, `actors_children`,
  `actors_time` and `actors_host`, each case in exactly one.**
  (reason not recorded)
  Source: `test/CONVERSION.md` (libs/xstate).

**Documentation.**

- **The page's guarantees are numbered, 1 to 24 for the core and A1 to A12 for
  the actor layer, each under an anchor that code and tests cite as `(doc:
  #xstate-invariant-13)`; the doc build fails on a citation that names no
  anchor.** A guarantee keeps its number so that a comment can cite it, and a
  citation of nothing is caught.
  Source: `doc/guarantees.adoc` (libs/xstate).
- **A Doc Comment that names xactor's API names it in full,
  `webcpp::xactor::scheduler`, and the page links xactor's page where the
  layer leans on it.** MrDocs cannot link into another library's reference.
  Source: `libraries/xstate/openspec/specs/xstate/spec.md` (proposals).

### pratt (Pratt parser engine with a calculator; own; implemented in 0.1.0)

webcpp::pratt is a header-only engine for Pratt parsing (top-down operator
precedence) that knows no token, operator, tree or number type, with a
calculator, `calc`, built on it that computes exactly in decimal by default.
It was designed from an owner's draft with a verified reference
implementation, is the model of a library whose engine and user-facing
component are kept apart, and seeds a future parser of openui-lang.

**Scope and layers.**

- **pratt is a library of webcpp's own (`"port-of": null`): it has no oracle,
  no twins and no appendix of differences, and its page, with the Doc Comments
  of its headers, is what a change is checked against.** It ports nothing, so
  there is no original to judge it.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The engine is the Pratt loop and nothing else: not a parser generator, a
  grammar DSL, a lexer toolkit or an AST; it builds whatever node the grammar
  says and does nothing else with it.** Pratt parsing turns precedence and
  associativity into numbers and a new operator into a new answer to one
  question, where a hand-written recursive-descent parser spreads them over
  one function per level.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/proposal.md`
  (proposals).
- **The engine and the calculator are one library kept apart by an include
  boundary: the engine's headers, `pratt.hpp`, `errors.hpp` and `parser.hpp`,
  may not include `webcpp/pratt/calculator` or `boost/decimal`, and the
  convenience header `<webcpp/pratt.hpp>` includes the engine only.** The
  engine knows no grammar and no number type, and a program of the engine
  alone never reads Boost.Decimal, a heavy dependency; the calculator is
  included by its own name.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Interface.**

- **A token stream models the concept `token_stream` (`token_type`, `peek()
  const`, `advance()`, `at_end() const`) and a grammar models `grammar_for<G,
  S>` (`node_type`, `lbp(t)`, `nud(p, t)`, `led(p, t, left)`), with no base
  class, no virtual function, no `std::function` and no type erasure between
  them and the engine.** A base class of tokens and nodes, the alternative
  rejected, would make every program pay for a hierarchy and fix the node
  type; with concepts a grammar that consults run-time tables stays possible.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **`parser<Grammar, Stream>` holds a pointer to the grammar, the stream by
  value and two integers, and its constructor from a `const Grammar&&` is
  deleted; `parse` takes the grammar by `const&` and accepts a temporary.** A
  temporary grammar would leave the parser's pointer dangling, while the
  parser that `parse` builds lives only for the call.
  Source: `doc/grammar.adoc` (libs/pratt).
- **`parse` parses `expression(0)` and returns `trailing_input` when tokens
  remain; a program may build the `parser` itself.** Building it lets a
  program call `expression` with another power, or go on reading the stream
  after one expression.
  Source: `doc/grammar.adoc` (libs/pratt).
- **The six helpers, `literal`, `prefix`, `infix_left`, `infix_right`,
  `postfix` and `grouping`, each parse their operands through `expression`
  alone and call a `make` of the grammar's that builds the node; `infix_right`
  parses its right operand with `bp - 1`.** Every helper recursing through
  `expression` alone, a grammar written with them routes every token through
  its nud and its led and is bounded whole by the depth limit; `bp - 1` is the
  canonical Pratt way to make an operator of equal power on the right take the
  operand.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The parser offers `peek()` and `advance()` to a nud or a led.** A nud or a
  led that reads past its own token needs them, such as the argument loop of a
  call.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Errors.**

- **One enumeration, `pratt::error`, in the engine's header, holds the draft's
  seven errors and two more, `nesting_too_deep` (8) and `division_by_zero`
  (9), values 1 to 9, fixed and never reused; the engine produces three of
  them, and the others are there for a grammar to use.** A value stored or
  sent keeps its meaning from one version to the next, 0 is no error, and the
  calculator and the micro-subset report their own failures with them.
  Source: `doc/errors.adoc` (libs/pratt).
- **The category follows xactor's form, not the draft's function-local static:
  a `constexpr` object with the fixed identity `0x7072617474000000` (the ASCII
  of "pratt" and three zero bytes), the name `webcpp.pratt`, prose messages
  ("expression nested too deeply"), "unknown webcpp.pratt error" for any other
  value, and both `message` forms.** A category constructed `constexpr` with a
  fixed identity holds no mutable state.
  Source: `libraries/pratt/openspec/specs/pratt/spec.md` (proposals).
- **The engine and the calculator build an error into a `result` in place,
  `{boost::system::in_place_error, error::...}`.** A node type may convert
  from an error too, as a Boost.Multiprecision number does, and `result` then
  refuses the implicit conversion as ambiguous.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **An error of another category that a grammar or an environment returns is
  passed on unchanged; the engine never reads it.** An environment can refuse
  with an error of its own, such as a fixed storage that is full.
  Source: `doc/errors.adoc` (libs/pratt).

**Limits.**

- **The parser counts its active `expression` calls with an RAII guard and
  refuses the call that would exceed `max_depth` with `nesting_too_deep`,
  before it reads a token; a negative limit behaves as 0, and the limit is a
  constructor argument that `parse` and `calc::evaluate` forward.** Pratt
  recursion costs stack for each nesting level, so ten thousand `(` would
  overflow a stack, a crash rather than an error, and wasm's default stack is
  64 KiB.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **`default_max_depth` is 64, measured rather than chosen: the largest power
  of two at most half of the deepest parse that wasm's default 64 KiB stack
  holds in a debug build, in the costliest shape of the calculator's grammar
  and of openui-lang's (203 calls; half is 101).** A parse at the default
  depth then uses at most half the stack in the worst shape and the slowest
  build, on every target pratt is tested on.
  Source: `doc/limits.adoc` (libs/pratt).
- **The measurement is a table above the constant in `parser.hpp`, repeated
  verbatim in the "Limits" chapter, and `depth_test` is its canary: twice the
  default must still fit.** The canary fails on wasm, as a trap, the day the
  margin is gone, and the table, the value and the chapter then move together.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The limit bounds the recursion through `expression` alone: a nud or a led
  that recurses through functions of its own bounds that recursion itself.**
  The limit counts `expression` calls only, which is the whole recursion of a
  grammar whose operands all go through `expression` or the helpers.
  Source: `doc/limits.adoc` (libs/pratt).

**The calculator.**

- **The calculator is a grammar written with the engine's helpers alone and
  adds no parsing of its own.** Everything goes through the engine, which
  stays the only place that decides.
  Source: `doc/calculator.adoc` (libs/pratt).
- **Its lexer, `calc::stream`, is lazy: it holds the source, a position and
  one token of lookahead, and no vector of tokens exists.** Lexing everything
  first, the alternative rejected, would find an invalid character before any
  other error and allocate a token vector.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **A character that starts no token is a token of kind `invalid`, one
  character long, whose lbp is `std::numeric_limits<int>::max()` and whose nud
  and led return `invalid_character`.** The engine reaches it wherever it
  stands, so `1 $ 2` is `invalid_character` rather than `trailing_input`, and
  the engine stays the only place that decides.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The first error in reading order is the one returned, and a write made
  before it stays made; nothing is undone.** Undoing the writes of a failed
  evaluation is a non-goal: a caller that needs all or nothing evaluates in a
  copy of the environment and keeps it on success.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The calculator computes in any type that models `calc::number`: copyable,
  constructible from `int`, `+ - * /`, unary `-`, the six comparisons, and
  `pow`, `fmod` and `trunc` found by ADL after `using std::pow;` (the two-step
  call), each returning exactly the type; every `calc` template is constrained
  by it.** The calculator never computes through another type,
  `boost::decimal`'s overloads serve its types and `std::`'s the builtin
  floating-point ones; so `int` does not model it, nor a type with expression
  templates.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The default number type is `boost::decimal::decimal64_t`.** A decimal
  fraction written in the input is then exactly that number: `0.1 + 0.2 ==
  0.3` holds.
  Source: `doc/calculator.adoc` (libs/pratt).
- **The lexer builds a literal in the number type's own arithmetic, never
  through `double`: its first `digits10 + 1` significant digits (or
  `max_digits10`) make one integer mantissa, divided once by ten to the number
  of fraction digits; in a binary type a scaled-up mantissa is computed to
  twice the precision with Dekker's split and product.** A decimal fraction is
  exact in a decimal type and the nearest value in `double`, where a division
  by ten per digit misses `1.005` and `0.07` by one unit in the last place,
  and the largest `double` written out in full stays finite.
  Source: `libraries/pratt/openspec/specs/pratt/spec.md` (proposals).
- **Every multiplication and addition of the lexer, of the twofold product and
  of the power is a statement of its own.** No compiler then contracts them
  into a fused multiply-add, which would round differently on each target; a
  build with `-ffp-contract=fast` or `-ffast-math` may still change a `double`
  in its last place, which the page says.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **An integral exponent is computed by repeated squaring in the number type;
  `pow` is called only for a non-integral exponent, or a negative one whose
  positive power is out of range.** Every product is exact while it fits the
  precision, and the result is the same bits on every platform, where a
  library's `pow` may round differently; the price, documented, is that past
  the precision the relative error grows with the exponent.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **An integer whose successor rounds back to it counts as even when the power
  takes its sign.** Boost.Decimal's `fmod` is inexact past the precision, and
  such an integer is a multiple of its radix.
  Source: `include/webcpp/pratt/calculator.hpp` (libs/pratt).
- **`/` and `%` by zero, and `0 ^ -n`, return `division_by_zero`.** A
  floating-point type would answer an infinity or a NaN that the next line
  computes on; the calculator stops at the line that asked.
  Source: `doc/calculator.adoc` (libs/pratt).
- **The calculator's language, kept from the owner's draft, is numbers,
  variables and C's arithmetic and comparison operators with `^` for the
  power: `=` 10 (right), comparisons 20 giving 1 or 0, `+ -` 30, `* / %` 40
  (the remainder taking the left operand's sign), a unary sign 45, so that `-2
  ^ 2` is -4, `^` 50 (right), and `++ --` as in C, on variables only, 70 after
  an operand.** (reason not recorded)
  Source: `doc/calculator.adoc` (libs/pratt).
- **A node is a value, a `has_value` flag and, for a bare variable, its name
  as a view; a grouping keeps the name, a name the environment does not hold
  is an error only where it is read, and `=` checks its target before it reads
  its right operand.** The name is what `=`, `++` and `--` write, so `(a) = 1`
  writes `a`, and `a = 5` defines a name the environment does not hold yet.
  Source: `doc/calculator.adoc` (libs/pratt).
- **Where the variables live is a concept, `environment_for`: `number_type`,
  `lookup(std::string_view) const` returning `const number_type*`, and
  `assign(std::string_view, number_type)` returning `result<void>`.** An
  environment can refuse a write, a fixed storage that is full, with an error
  of its own category, which the calculator returns unchanged; the number type
  follows the environment.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **`calc::environment<Number, Allocator>` is a `std::map` with `std::less<>`
  whose nodes and keys allocate through `Allocator` rebound, its key a class
  of its own around a `std::basic_string`.** A lookup by `std::string_view`
  builds no key, an allocator written for any type serves, and a class key
  avoids libstdc++'s `std::pair` making clang instantiate a default argument
  that is a hard error for an allocator without a default constructor.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **`calc::environment::assign` always returns success, and a failure to
  allocate is the allocator's.** As in any standard container.
  Source: `include/webcpp/pratt/calculator.hpp` (libs/pratt).

**Memory.**

- **The engine allocates nothing, the calculator's parse allocates nothing
  (its tokens and nodes are views and numbers), and only an environment that
  stores names allocates, through the user's allocator.** pratt is born with
  webcpp's allocation rule: avoid allocation as far as the job allows, and let
  the user choose the allocator of what is allocated.
  Source: `doc/allocation.adoc` (libs/pratt).
- **`allocation_test` replaces the global `operator new`, over-aligned forms
  included, for its own program and counts zero calls over a fixed-storage
  environment and only the allocator's own over `calc::environment`; a test or
  example compares what a step adds and never states how many allocations a
  standard library makes.** MSVC's map allocates when it is built, and a debug
  build adds proxies, so totals differ between standard libraries.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Targets and exceptions.**

- **The tests and examples are declared for native, Emscripten, wasip2 and
  wasip3, each proven by a lane; Emscripten joined once the superproject had
  its lane.** Every target pratt declares is proven by a lane, so Emscripten
  waited for the superproject's.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **No header contains `throw`, `try` or `catch`, and
  `WEBCPP_PRATT_NO_EXCEPTIONS` changes nothing in pratt.** The same headers
  serve a build with exceptions and one without, and the macro exists so that
  a program can set it for every webcpp library alike.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Evidence.**

- **Three grammars prove the engine generic: the engine test's `table`
  (binding powers from a table, tokens `char`, nodes `std::string`), the
  draft's `sexpr`, and the openui-lang micro-subset of the examples.** Each
  has token and node types of its own and reaches the engine unchanged.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The openui-lang micro-subset gives `(` both of Pratt's denotations, a
  grouping as nud and a call of power 90 as led, whose argument loop reads
  `expression(0)`, `peek` and `advance` inside the led; its node is a
  `std::string`.** It proves that the engine parses calls, named arguments and
  assignments with no recursive descent around it; the string is the examples'
  output format, and the parser of openui-lang to come defines a tree of its
  own.
  Source: `doc/openui.adoc` (libs/pratt).
- **The two REPLs share one line loop and run as examples fed an `.input`
  file, an extension made to the superproject's `webcpp.example`, and
  `.gitattributes` keeps `*.input` byte for byte.** A REPL needs standard
  input to be tested as an example, and an input holds lines that end in
  `\r\n` on purpose.
  Source:
  `libraries/pratt/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The language is named openui-lang, alone, in every file and commit.**
  (reason not recorded)
  Source: `AGENTS.md` (libs/pratt).

**Documentation.**

- **Each `@see "<title>"` of a Doc Comment names a top-level chapter of the
  page, which doc-check enforces.** Renaming a chapter then renames its `@see`
  lines in the same commit, or the doc build fails.
  Source: `AGENTS.md` (libs/pratt).

### wasi (the HTTP handler a WebAssembly component exports, wasip2 and wasip3; own; implemented in 0.1.0)

`webcpp::wasi::http` exports the `wasi:http` handler of a component on
wasip2 (WASI 0.2.12) and wasip3 (WASI 0.3.0): a program writes one function
that turns a request's method and target into a response, and wasi reads the
request, calls it and writes the answer. It was moved into webcpp on
2026-10-08 from an earlier private project; its redesign, accepted and not
built, is recorded apart.

**Scope.**

- **One header serves both versions of WASI, its two handlers chosen by
  `WEBCPP_WASI_HTTP_P2` or `WEBCPP_WASI_HTTP_P3`, and a program's main is
  the same on both.** One header per version was rejected: a program would
  have to choose between them.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The handler hands the main the method and the target alone, and sends a
  status, a `content-type` header and a body: no request header, no request
  body, no other response header, no trailers, no streamed body.** (reason
  not recorded) The genesis lists each as a non-goal, a change of its own.
  Source: `doc/limits.adoc` (libs/wasi).
- **The handler parses, decodes and routes nothing: the target reaches the
  main as it was sent.** Routing, URL parsing and JSON belong to the
  libraries that exist for them: a main parses the target with Boost.URL's
  `parse_origin_form` and writes JSON with Boost.JSON.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **wasi exports the HTTP handler's world alone; a program whose world
  exports more uses the headers-alone target `/webcpp/wasi//wasi` and
  generates its own bindings, renamed `webcpp_wasi_http` too.** (reason not
  recorded)
  Source: `doc/build.adoc` (libs/wasi).
- **wasi targets `wasi:http` 0.2.12 and 0.3.0 alone, as measured with
  wasi-sdk 34, wit-bindgen 0.62.0 and wasmtime 47.0.3, and the page names
  each version as measured.** WASI 0.3 is young: a toolchain update may move
  the names the bindings' glue calls.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Interface.**

- **The program writes its main between `WEBCPP_WASI_HTTP_MAIN_BEGIN(method,
  target)` and `WEBCPP_WASI_HTTP_MAIN_END()`, which expand the `extern "C"`
  export and the head of `webcpp::wasi::http::http_main`.** The program
  defines the handler once, and the header holds only inline functions and
  macros, so every translation unit may include it (guarantee 2); a function
  the program registers was rejected, since it needs a global. The cost: the
  body is inside a function of `webcpp::wasi::http`, so it names its own code
  with a full namespace or an alias, and `.clang-format` names the two macros
  as `MacroBlockBegin` and `MacroBlockEnd` to indent it.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The main receives the method by its name on the wire (an extension
  method as it was sent) and the target as sent (`"/"` when the request
  carried none), two `std::string_view` that view the request.** The
  handler's own code allocates nothing: it views the request's strings where
  the bindings put them, and the main copies what it keeps beyond the
  request.
  Source: `libraries/wasi/openspec/specs/wasi/spec.md` (proposals).
- **`response` is an aggregate of three members with defaults, in this
  order: `unsigned status = 200`, `std::string_view content_type{}`,
  `std::string body{}`.** A designated initializer names only what differs
  (`{.status = 404}`); the order is an HTTP response's and the initializer's,
  and the reference keeps it with `sort-members: false`.
  Source: `doc/mrdocs.yml` (libs/wasi).
- **`content_type` is a view and `body` a copy.** A content type is nearly
  always a literal, which a view holds for nothing, where a `std::string` of
  16 characters allocates on wasm32 (libc++ keeps 10 inline); the body is
  sent after the main has returned, so it must not point into the main's
  strings.
  Source: `include/webcpp/wasi/http/response.hpp` (libs/wasi).
- **`<webcpp/wasi/http/response.hpp>` includes nothing of the component ABI
  and compiles natively; its test runs natively and on both versions.** The
  rest of a program builds responses, holds them and passes them around (as
  messages between actors) and is tested natively, without a WASI toolchain.
  Source: `include/webcpp/wasi/http/response.hpp` (libs/wasi).
- **The handler's names are in the nested namespace `webcpp::wasi::http`,
  and its headers under `<webcpp/wasi/http/>`.** wasi is a collection of
  helpers for C++ built as components, and the HTTP handler is its first
  part.
  Source: `meta/libraries.json` (libs/wasi).

**Bindings and build.**

- **The header includes the generated `webcpp_wasi_http.h`, and the build
  generates every world's C bindings with `--rename-world
  webcpp_wasi_http`.** Every generated name the header uses
  (`webcpp_wasi_http_string_t`, `WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`)
  is then the same whatever the program's world is called.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The bindings are generated into the build directory, never committed and
  never edited.** (reason not recorded)
  Source: `AGENTS.md` (libs/wasi).
- **`build.jam` declares a standard world per version, exporting the HTTP
  handler and nothing else, and the target `/webcpp/wasi//http`, which adds
  the headers, the bindings of the target's world and the target's macro.**
  A program that exports the handler and nothing else links that one target
  and is done; there is no compiled library of wasi's.
  Source: `build.jam` (libs/wasi).
- **The build defines the version's macro (`<toolset>clang-wasip2` gives
  `WEBCPP_WASI_HTTP_P2`), and the entry point stops with one `#error` under
  neither or both, before it includes the bindings.** The entry point
  compiles only for WASI and for one version (guarantee 1), and the refusal
  tests pin that one message; why the build rather than the compiler names
  the version is not recorded.
  Source: `include/webcpp/wasi/http/entrypoint.hpp` (libs/wasi).
- **A component is linked as a reactor, `-mexec-model=reactor`.** It exports
  a handler and has no C `main`; wasmtime 47 also serves it linked as a
  command, but webcpp builds and tests reactors.
  Source: `doc/build.adoc` (libs/wasi).
- **b2 looks for wit-bindgen, the `wasi:http` WIT and wasmtime only when a
  target needs them, each at its `-s` variable first, then under `.local/`
  or on `PATH`, and stops naming the tool and every place it looked.** A
  native build of the response and the configuration needs none of them.
  Source: `doc/build.adoc` (libs/wasi).

**Behaviour.**

- **A status outside 100 to 599 is not sent: the handler answers an internal
  error instead, before anything is committed, on both versions.**
  `set-status-code` refuses some statuses and leaves the response at 200, and
  the status's 16 bits read 65736 as 200, so a main's bug would reach the
  client as a success; clamping the status was rejected because it alters
  the response.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **A content type the host refuses as a field value (a CR, an LF, a NUL)
  answers an internal error instead of the response; a tab is sent as it
  is.** Dropping the header was rejected: a response is sent as the main
  wrote it or refused, never altered (guarantee 9).
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The handler frees the method, the target and the request only after the
  response is written.** A `content_type` that views the method or the target
  is then still valid when it is sent; a first version freed them earlier,
  and the host refused the overwritten content type.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The results of `set-status-code` and of `outgoing-body.finish` are not
  read.** `set-status-code` fails only outside 100 to 599, which the guard
  refused, and `finish` fails only when the body disagrees with a
  `content-length`, which the response never sends.
  Source: `include/webcpp/wasi/http/entrypoint.hpp` (libs/wasi).
- **On wasip2 the body is written in chunks of at most 4096 bytes through
  `blocking-write-and-flush`, then finished with no trailers.** 4096 bytes is
  the most `blocking-write-and-flush` accepts per call.
  Source: `include/webcpp/wasi/http/entrypoint.hpp` (libs/wasi).
- **On wasip3 the handler returns the response, then writes to the body's
  stream until the stream has taken every byte, then resolves the trailers
  future.** wasmtime 47 takes a whole body, 1 MiB too, in one write; the loop
  stands for a host that takes less per write.
  Source: `doc/guarantees.adoc` (libs/wasi).
- **On wasip3 the future of whether the host sent the response is dropped
  unread.** The guest could change nothing about a response already sent.
  Source: `include/webcpp/wasi/http/entrypoint.hpp` (libs/wasi).
- **The wasip3 handler is lifted asynchronously (`--async` on its bindings),
  waits for each blocked write inside the task on a waitable set, and always
  returns `EXIT`; the exported callback exists and returns `EXIT`.** Context
  slot 0 belongs to wasi-libc on wasip3, and using it trapped (measured); the
  callback exists because the glue calls it.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **A failure before the commit answers an internal error; after it, wasip2
  drops the body without `finish` and wasip3 resolves the trailers future
  with an internal error.** Once committed nothing can be taken back, and
  each is how its version tells the host the response failed (guarantee 6).
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).

**Exceptions and configuration.**

- **The handler throws, tries and catches nothing: an exception that escapes
  the main traps the instance, and wasmtime 47 answers its own HTML 500 and
  keeps serving.** The owner decided it on 2026-10-08, settling the open
  item of catching `std::exception` in the handler to print its `what()`:
  the host answers, and wasi needs neither exceptions nor RTTI and imposes
  neither.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **wasi's programs, and the by-hand commands its page shows, build wasip2
  with exceptions as wasip3: `-fwasm-exceptions -mllvm
  -wasm-use-legacy-eh=false`, linked with `-fwasm-exceptions -lunwind`.**
  Measured for wasi: wasmtime 47 runs the exnref encoding with its default
  flags and refuses the legacy one wasi-sdk 34 emits by default, and
  exceptions cost about 20% of a stripped component and nothing at a warm
  start, while their absence cost Boost.Locale, a second Boost configuration
  and a wasip2-only path in every header.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/reference/research/WASIP2_EXCEPTIONS_SPIKE.md`
  (proposals).
- **`<webcpp/wasi/config.hpp>` includes nothing and derives
  `WEBCPP_WASI_NO_EXCEPTIONS` itself, from `WEBCPP_NO_EXCEPTIONS`,
  `BOOST_NO_EXCEPTIONS`, `__cpp_exceptions` and `_CPPUNWIND`.** A deviation
  from the configuration rule: wasi includes neither Boost nor
  `<webcpp/config.hpp>` until its redesign, and the macro, which changes
  nothing in wasi, exists so that a program sets it for every webcpp library
  alike.
  Source: `include/webcpp/wasi/config.hpp` (libs/wasi).

**Dependencies.**

- **The headers include only the standard library and the bindings
  wit-bindgen writes, and name no other webcpp library; the target `wasi`
  still carries Boost's headers, as every library's target does.** wasi is
  to impose nothing on a component: no exceptions, no RTTI, no Boost in its
  headers.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **wasi takes no allocator: the request's strings come through the
  bindings' `cabi_realloc` and the body through `std::allocator`.** (reason
  not recorded) A caller-supplied allocator is a non-goal of 0.1.0, and the
  page says a program cannot yet give one.
  Source: `doc/limits.adoc` (libs/wasi).
- **wasi names nothing of the programs that use it: the only generated names
  it uses are the standard interfaces' and the rename's.** It was first
  written for one program, whose name the lint's banned-word rule refuses in
  every file and commit (guarantee 8).
  Source: `doc/guarantees.adoc` (libs/wasi).

**Evidence.**

- **Every guarantee is held by a component that `wasmtime serve` runs on
  wasip2 and wasip3, sent the requests of `<stem>.requests` and compared,
  byte for byte, with `<stem>.expected`.** Every guarantee is proven by a
  test that fails when it breaks, served by a real host.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The transcript writes each request as the curl command that sends it
  (`curl -I` for `HEAD`), leaves out `date` and `transfer-encoding`, and
  writes the host and port fixed.** A shell runs it as written (`curl -X
  HEAD` waits for a body that never comes), and one transcript holds on every
  machine and every lane.
  Source: `doc/testing.adoc` (libs/wasi).
- **The served programs run in the own lane `http`, one CI job per
  target.** The owner's decision C11 of 2026-10-08, since stage 4 needs the
  same for a browser lane; it keeps them out of the ordinary lanes.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The two refusals, `neither_target_refused` and `both_targets_refused`,
  pass only on the guard's own error (`expected-error` of clang's
  `-verify`).** A compile that fails for another reason fails the test.
  Source: `libraries/wasi/openspec/specs/wasi/spec.md` (proposals).
- **`http_bad_status` and `http_bad_content_type` hold wasmtime 47's HTML
  500 verbatim.** They fail when wasmtime changes how it answers an internal
  error, and are regenerated knowingly.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **Guarantee 6, and the second turn of wasip3's write loop, are held by
  review.** wasmtime offers no way to make a write fail on demand, and takes
  the whole body in one write.
  Source: `doc/guarantees.adoc` (libs/wasi).
- **The page states nine guarantees, each keeping its number and its anchor
  `wasi-invariant-<n>`, and `b2 libs/wasi/doc` fails on a citation that
  names no anchor.** The code and the tests cite a guarantee as
  `(doc: #wasi-invariant-<n>)`.
  Source: `doc/guarantees.adoc` (libs/wasi).
- **The README's example and the by-hand build are tests: doc-check compares
  the README's copies with `example/http_hello.*`, and `by_hand` runs
  `example/by_hand/build.sh` on both versions and serves what it builds.** A
  page that shows a command that stopped working fails the lane.
  Source:
  `libraries/wasi/openspec/changes/archive/2026-10-08-genesis/design.md`
  (proposals).
- **The reference parses the entry point natively with wasip2's bindings and
  macro, and checks the Doc Comments of the wasip3 branch with its own.** The
  entry point builds only for WASI.
  Source: `doc/Jamfile` (libs/wasi).
- **The page's chapter "Building for WASI with Boost" includes the
  superproject's configuration by tag through `{webcpp-root}`.** doc-check
  accepts only a tagged region of a tracked file, so a region renamed there
  fails the page rather than leave it stale.
  Source: `AGENTS.md` (libs/wasi).

### idna (internationalized domain names: Punycode, UTS 46 and IDNA2008 over generated Unicode 17.0.0 tables; own; implemented in 0.1.0)

idna judges and converts domain names: Punycode (RFC 3492), UTS 46's ToASCII
and ToUnicode (revision 35), and IDNA2008's validity (RFC 5890 to 5893). It
was specified and built from 2026-10-09 to 2026-10-12 as the library under
webcpp::lexical's JSON Schema formats `hostname`, `idn-hostname`, `email`
and `idn-email`.

**Why a library of its own.**

- **idna implements Punycode, UTS 46 and IDNA2008 itself, rather than wrap
  ICU or libidn2 or take ada-url/idna as it is.** Of the JSON Schema
  suite's `idn-hostname` cases, ada fails 16.3% and upa 15.2%; ICU (4.3%)
  and libidn2's registration check (2.2%) fail fewer but are compiled
  dependencies, not header-only, and no header-only C++ IDNA2008 library
  exists.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **ada-url/idna is read as a reference for algorithms and table packing
  only: nothing of it is copied, and no file of it enters the repository.**
  It is one of the owner's binding decisions of 2026-10-09, once taking ada
  as a dependency was rejected (above).
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/reference/spec.md`
  (proposals).
- **idna implements both standards, UTS 46's processing and IDNA2008's
  validity.** UTS 46 says how a typed name is mapped and IDNA2008 which
  labels are valid, and they disagree: UTS 46 accepts `¡` and has no
  CONTEXTO rules, while JSON Schema asks for IDNA2008's strictness.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/proposal.md`
  (proposals).
- **IDNA2003, registry-specific rules, confusable detection (UTS 39), the
  WHATWG URL host parser and DNS lookups are out of scope.** RFC 5891
  section 4.3 leaves registry rules to registries, a later URL library may
  build on `to_ascii` with its own options, and idna judges and converts
  strings only.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).

**Interface.**

- **The default `idna::options{}` is the strict profile: UTS 46 with every
  check on, nontransitional, the DNS lengths verified, and `idna2008 =
  true`; the default never becomes looser.** UTS 46 alone accepts the code
  points it marks NV8 and XV8 and every CONTEXTO violation; UTS 46 alone as
  the default was rejected, since every consumer would have to turn on the
  check it needs.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`options` is an aggregate of eight `bool`s, UTS 46's seven flags named
  after them and idna's own `idna2008`.** A caller names only what changes
  with a designated initializer: `idna::options{.idna2008 = false}` is
  UTS 46 alone.
  Source: `include/webcpp/idna/options.hpp` (libs/idna).
- **Every conversion has a span form, the primitive, which writes into the
  caller's `std::span` and allocates nothing, and a pmr form, which returns
  a `std::pmr` string from a memory resource (by default
  `std::pmr::get_default_resource()`) and allocates from it alone.** "Nothing
  allocates" then holds for every name the DNS accepts, whose sizes are
  bounded; an API of `std::string` alone was rejected, since it cannot honour
  it.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The span forms take their output first, `to_ascii(output, domain)`, as
  Boost.URL's encoders do.** No call is then ambiguous between the two
  forms.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The conversions (`uts46.hpp`) map a name first; the validators
  (`validity.hpp`: `check_label`, `check_domain`, `is_valid_label`,
  `is_valid_domain`) check it as given, map nothing, take no output and never
  allocate.** A program needs the validators when a name must already be in
  its final form, as a hostname's A-labels must.
  Source: `doc/validity.adoc` (libs/idna).
- **`check_domain` splits at U+002E alone, and `check_label` refuses a
  U+002E as `disallowed`.** The other full stops, U+3002, U+FF0E and
  U+FF61, separate labels only after UTS 46's mapping, so as given they are
  code points UTS 46 maps.
  Source: `doc/validity.adoc` (libs/idna).
- **An A-label is `xn--` in either ASCII case, must be ASCII, is decoded,
  validated as a U-label (NFC and nontransitional processing included) and
  encoded again, and is refused as `invalid_punycode` unless the round trip
  gives it back, compared in lowercase.** RFC 5890 compares an A-label in
  either case, and RFC 5891 section 5.4 asks for the round trip.
  Source: `doc/validity.adoc` (libs/idna).
- **`to_unicode` returns a struct of what it wrote and an `error_code`, goes
  on after an error, and puts no U+FFFD in place of what failed; in its span
  form `output_too_small` takes the place of any other error.** UTS 46 asks
  that the conversion be performed as far as possible, so that a name can be
  shown even when it is not valid; a caller whose span is too small needs a
  larger one before anything else.
  Source: `doc/converting.adoc` (libs/idna).
- **Under the DNS lengths `to_ascii` writes at most 253 chars, so a
  `std::array<char, idna::max_domain_length>` always holds the result, and
  it reports `output_too_small` last.** A span too small for a name that is
  too long anyway reports the name (guarantee 4).
  Source: `doc/converting.adoc` (libs/idna).
- **A final empty root label is `empty_label` in `to_ascii` under the DNS
  lengths and no error in `to_unicode`.** UTS 46 revision 35 disallows the
  empty root label under VerifyDnsLength, and its X4_2 exempts a final empty
  label in the Unicode form.
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).
- **Punycode's functions, in the nested namespace `webcpp::idna::punycode`,
  work without the `xn--` prefix: the encoder writes lowercase digits, the
  decoder reads digits in either case and copies basic code points as given,
  and a delimiter at the very start consumes nothing.** The basic code points
  keep RFC 3492's mixed-case annotation, and `-9uc` is refused, as RFC 3492
  says, so that `xn---9uc` is never read as `xn--9uc`.
  Source: `doc/punycode.adoc` (libs/idna).
- **The signatures of `errors.hpp`, `options.hpp`, `punycode.hpp`,
  `validity.hpp` and `uts46.hpp` are a contract, changed only with an
  announcement to webcpp::lexical; `detail` changes freely.** lexical was
  specified in parallel and codes against them.
  Source: `AGENTS.md` (libs/idna).

**Errors.**

- **Every fallible operation returns `idna::result<T>`, Boost.System's
  `result`, and its errors are the 19 values of `idna::error`, one per family
  of rules rather than one per UTS 46 status code.** IdnaTestV2 itself says
  an implementation "need only record that there is an error".
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The values start at 1, never change and are never reused; a new error
  takes the next value.** An `error_code` of value 0 is no error, and a
  stored value keeps its meaning across versions.
  Source: `include/webcpp/idna/errors.hpp` (libs/idna).
- **The category is one `constexpr` object, `detail::the_category`, with
  the fixed name `webcpp.idna` and the fixed identity `0x69646E6100000000`,
  the ASCII of "idna", in pratt's form.** idna then holds no mutable state.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The category overrides `message(int, char*, std::size_t)`, which copies
  the message into the caller's buffer, cut to fit, and is `noexcept`.**
  `message(int)` returns a `std::string` as Boost.System declares it and may
  throw what `operator new` throws; the buffer form throws nothing
  (guarantee 2).
  Source: `doc/guarantees.adoc` (libs/idna).
- **The first error is reported, in a fixed order: the first label's first
  fault, labels in order, a label's rules in UTS 46's order with IDNA2008's
  last; then `bidi`; then `domain_too_long`; `output_too_small` last in
  `to_ascii`.** Tests pin the order, so that lexical returns the same reason
  on every target.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The span forms and the validators are `noexcept`, the pmr forms are not,
  and a test's `static_assert`s state it.** A pmr form throws only what its
  memory resource throws, and idna raises nothing of its own (guarantee 2).
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).
- **A `noexcept` function reads a `result` it has checked with
  `unsafe_value()`, never with `operator*`.** Boost 1.92's `operator*`
  calls `value()`, which throws, and clang-tidy's `bugprone-exception-escape`
  then fails the function.
  Source: `AGENTS.md` (libs/idna).

**Memory.**

- **A single driver, `detail/process.hpp`, streams UTS 46 section 4 label by
  label for both the conversions and the validators, into a sink that takes
  the ASCII form, the Unicode form or nothing.** One label is held at a
  time, so the working storage is one label's whatever the name's length.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **A label is held in fixed storage of 256 code points,
  `detail::max_label_code_points`.** Every label the DNS lengths accept fits:
  an A-label of 63 octets decodes to at most 59 code points, and the longest
  full canonical decomposition of Unicode 17.0.0 is 4 code points (U+1FAF),
  which the generator measures and checks.
  Source: `include/webcpp/idna/detail/label_buffer.hpp` (libs/idna).
- **A label beyond the storage is `label_too_long` under the DNS lengths and
  `storage_exhausted` without them in a span form or a validator, while a pmr
  form grows it into its memory resource.** Such a label is no name the DNS
  can hold, and a span form allocates nothing.
  Source: `doc/limits.adoc` (libs/idna).
- **In the ASCII form under the DNS lengths, a pmr form holds a label to the
  inline capacity too.** No label past it passes the DNS lengths, so both
  forms report the same first error.
  Source: `include/webcpp/idna/detail/process.hpp` (libs/idna).
- **A label beyond the storage is reported while it is gathered, before its
  rules, and stops the driver, keeping an error recorded before it.** No form
  can go on without the label.
  Source: `include/webcpp/idna/detail/process.hpp` (libs/idna).
- **Canonical ordering sorts a run of up to 32 non-starters by insertion, a
  run up to 256 by counting through a never-inlined buffer on the stack, and
  a longer one by counting through the label's memory resource.** Each step
  of NFC then does a bounded amount of work per code point, and the stack
  holds the buffer only while such a run is sorted.
  Source: `include/webcpp/idna/detail/nfc.hpp` (libs/idna).
- **Hangul syllables are decomposed and composed by the arithmetic of Unicode
  section 3.12.** They take no table space.
  Source: `doc/unicode.adoc` (libs/idna).
- **idna does not recurse, and holds about 1.3 KiB of inline storage per
  call; its deepest call measured about 11 KiB of stack in a debug wasm
  build, with no margin promised.** idna runs inside a WebAssembly program,
  whose default stack is 64 KiB.
  Source: `doc/limits.adoc` (libs/idna).

**Limits.**

- **The conversions bound a label's Punycode at 256 code points, past which
  it is `punycode_overflow` whatever `ignore_invalid_punycode` says.**
  RFC 3492's encoder and decoder work in time that grows with the square of a
  label: at 256 the worst label costs about 4 times a linear one, where 1,024
  code points cost 14 times and 4,096 cost 43; a limit of the implementation
  is no invalid Punycode.
  Source: `include/webcpp/idna/detail/process.hpp` (libs/idna).
- **`punycode::encode` and `punycode::decode` are the primitive and have no
  such bound.** A span form stops when the caller's span is full, and the
  page tells a program to bound input it does not trust before a pmr form.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **CONTEXTJ reads Joining_Type from the generated table, never from a list
  of joining scripts.** A hand-written list misses scripts, NKo, Mandaic and
  Phags-pa among them, and those a later Unicode adds.
  Source: `include/webcpp/idna/detail/context.hpp` (libs/idna).
- **CONTEXTO's rules A.7 to A.9 read facts gathered in one pass over the
  label, never a pass per occurrence.** A pmr form may hold a label far
  beyond a DNS label's length, and the check stays linear in it.
  Source: `include/webcpp/idna/detail/context.hpp` (libs/idna).
- **The Bidi rules' direction is that of the label's first code point, and
  an empty label is neither right-to-left nor in breach.** Rule 1 leaves
  only L, R and AL first, so the verdict is RFC 5893's; whether a label may
  be empty is the caller's question.
  Source: `include/webcpp/idna/detail/bidi.hpp` (libs/idna).
- **`check_domain` does a bounded amount of work per label.** A test holds
  a name of 65,536 one-letter labels under 20 times a name as long made of
  labels of 255 letters, where a pass over the name per label would exceed
  100 times.
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).

**Data.**

- **`tools/generate_tables.py` (Python 3.9, standard library only) writes
  the tables and the conformance vectors from Unicode's files, pinned by URL
  and SHA-256 in `tools/unicode.json`; the Unicode files are not
  committed.** No Unicode data is written by hand; committing the files was
  rejected, since it would copy Unicode's data into the repository.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The tables are `constexpr` arrays of sorted runs searched by binary
  search, 147,324 bytes, never compressed and never built at run time.**
  Nothing is allocated and nothing is shared mutable state, so any number of
  threads may call idna at once.
  Source: `doc/unicode.adoc` (libs/idna).
- **Only `detail/properties.hpp` includes the tables, and refuses to compile
  while their `unicode_version` differs from `idna::unicode_version`;
  `punycode.hpp`, `errors.hpp` and `options.hpp` never reach them.** The
  tables cost about 0.7 s and 188 KB natively in a translation unit that
  calls `to_ascii`, and a move to another Unicode changes both versions
  together.
  Source: `doc/unicode.adoc` (libs/idna).
- **IDNA2008's property is derived from UTS 46's table (PVALID, CONTEXTJ or
  CONTEXTO exactly when valid or deviation without an NV8 or XV8 mark), and
  the generator compares the derivation with IANA's `idna-tables-12.0.0` on
  every run; no IANA table is shipped.** UTS 46's NV8 and XV8 marks carry
  IDNA2008's difference, and IANA's tables stop at Unicode 12.0.0, so they
  serve as a check, which agrees on every code point IANA assigns, U+002E
  aside.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`detail/context.hpp` hard-codes the rules of RFC 5892 appendix A, and
  the generator stops if IANA's CONTEXTJ and CONTEXTO code points differ from
  that appendix's.** RFC 5892 fixes the two sets and their rules, and IANA's
  12.0.0 table agrees, which the check repeats on every run.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/reference/spec.md`
  (proposals).
- **The generator checks every assumption idna makes of the data before it
  writes (five statuses, at most 4 code points per decomposition, no Hangul
  decomposition, every property value known), and stops naming the one that
  fails.** A value a new Unicode version adds or renames is looked at, never
  absorbed as other.
  Source: `tools/generate_tables.py` (libs/idna).
- **A download is checked against its digest before it is kept, nothing is
  written unless every check passed, and two runs write the same bytes.** A
  moved URL leaves nothing in the cache, and the committed files are exactly
  the generator's output (guarantee 5).
  Source: `tools/generate_tables.py` (libs/idna).
- **The vectors of IdnaTestV2.txt and NormalizationTest.txt are generated as
  C++ data under `test/generated/`.** No test reads a file on any target.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The vectors' strings are `sv` literals, printable ASCII as is, every
  other byte a three-digit octal escape, and a digit right after an escape
  escaped too.** A view made from a pointer counts its characters in the
  array's constant evaluation, where Clang on libstdc++ runs out of steps;
  MSVC's warning C4125 refuses a digit after an octal escape.
  Source: `tools/generate_tables.py` (libs/idna).
- **The own lane `tables` runs the generator into the build directory and
  compares its outputs byte for byte; `update-generated` is the one writer of
  the generated files, and the ordinary lanes never run the generator.** It
  may need the network until its cache is filled.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The tables follow Unicode 17.0.0 (UTS 46 revision 35), not 18.0.0.** The
  owner chose 17.0.0; a move is a change of its own, which the page's
  section "Moving to another version of Unicode" lists file by file.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The Unicode License v3 notice is whole in the page's appendix "Unicode
  data", each generated file names it in one comment line, and no separate
  licence file is added; idna's code is BSL-1.0 alone.** The licence asks for
  its notice with the copies or in the documentation, while every file of
  webcpp is BSL-1.0 only.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).

**Dependencies and targets.**

- **idna needs Boost's headers (Config, System, Core, Assert) and
  webcpp::config, and nothing built or linked; its include boundary forbids
  `boost/url`, `boost/json` and `webcpp/lexical`.** idna depends on
  webcpp::config and header-only Boost alone, and lexical is its
  consumer.
  Source: `meta/include-boundaries.json` (libs/idna).
- **Every test and example runs on native, wasip2, wasip3 and Emscripten
  with the same vectors.** The same input and options give the same output
  and error on every target (guarantee 3).
  Source: `test/Jamfile` (libs/idna).
- **The tests that compile the generated vectors are built with `/bigobj`
  on MSVC.** MSVC compiles those large files of data only with it.
  Source: `test/Jamfile` (libs/idna).

**Evidence.**

- **idna has no oracle, no twins and no "Differences" appendix; its evidence
  is Unicode's IdnaTestV2.txt and NormalizationTest.txt, RFC 3492's samples,
  RFC 5892's and RFC 5893's rules case by case, and, through lexical, the
  JSON Schema test suite.** It ports nothing; IdnaTestV2 does not test
  CONTEXTO, and nothing official tests the combination of the two
  standards.
  Source:
  `libraries/idna/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **`uts46_test` runs all 6,391 lines of IdnaTestV2 under UTS 46 alone, then
  the strict profile over the 3,084 lines whose toUnicode holds no NV8, XV8
  or CONTEXTO code point, in both forms.** The file assumes UTS 46 alone, an
  IDNA2008 implementation skips NV8 and XV8, and the file does not test
  CONTEXTO.
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).
- **A line with a lone surrogate is refused as `invalid_utf8`, and writes
  nothing.** No UTF-8 string holds a lone surrogate, UTS 46 works on code
  points, and the file allows the refusal.
  Source: `doc/testing.adoc` (libs/idna).
- **`test/known_failures.json` is empty, may hold only a failure that comes
  from a difference between two versions of Unicode, and a test fails on a
  listed line that passes as on an unlisted line that fails.** The list never
  lies, and IdnaTestV2 is written for the same Unicode version as the
  tables.
  Source: `doc/testing.adoc` (libs/idna).
- **The Punycode tests pin four inputs, checked against Python's codec,
  beside RFC 3492's nineteen samples.** The samples leave alike what the
  four tell apart in the bias (damp against damp + 1, adapt's loop ending at
  455, the thresholds under biases of 70 and 83).
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).
- **Every test builds with libc++'s extensive hardening and libstdc++'s
  assertions.** A read past the end of a view traps instead of reading a
  neighbour's memory.
  Source: `test/Jamfile` (libs/idna).
- **The allocation test replaces the global `operator new` for its program
  alone, and its counting resource takes from that `operator new`, not from
  `std::pmr::new_delete_resource()`.** Every span form and validator must
  call it zero times (guarantee 1); MSVC defines `new_delete_resource` in its
  runtime's DLL, whose calls never reach the program's replacement.
  Source: `test/allocation_test.cpp` (libs/idna).
- **A test that guards a complexity bound compares with a baseline of the
  same size in the same run, the fastest of three, never with an absolute
  time.** A machine's load moves an absolute time but hardly a ratio: a
  loaded machine took the CONTEXTO ratio to 41, against a bound of 1,000 and
  the 30,000 a pass per occurrence would pass.
  Source: `libraries/idna/openspec/specs/idna/spec.md` (proposals).
- **The page states five numbered guarantees with the anchors
  `idna-invariant-1` to `idna-invariant-5`, which keep their numbers
  forever.** Code, tests and comments cite them.
  Source: `doc/guarantees.adoc` (libs/idna).
- **The reference gives each enumerator a section of its own
  (`show-enum-constants: true`).** The values of `error` are fixed, and the
  enum's table of members would show each enumerator's brief alone.
  Source: `doc/mrdocs.yml` (libs/idna).

**Sources.**

- **A narrow string literal spells a byte above 0x7F as a three-digit octal
  escape, a `U""` literal uses `\u` and `\U`, and a default `options` is
  written `idna::options()`.** A hex escape runs into the letters after it,
  MSVC reads raw UTF-8 in the machine's code page, and clang-tidy's
  `readability-trailing-comma` misreads an empty `options{}` followed by a
  comma.
  Source: `AGENTS.md` (libs/idna).
- **An A-label in the page's prose is written `` `+xn--bcher-kva.example+` ``,
  with the plus passthrough.** Asciidoctor replaces `--` between two word
  characters with an em dash, inside backticks too, and no check of the build
  sees it.
  Source: `AGENTS.md` (libs/idna).

### lexical (JSON Schema's string formats; own; implemented in 0.1.0)

lexical answers whether a string is a well-formed instance of each of the 19
string formats JSON Schema names, as its RFC or ECMA-262 defines it, through a
validator `is_<name>` and, but for `regex`, a Boost.URL grammar rule. It was
specified on 2026-10-09 as webcpp::formats, the format layer of
webcpp::json_schema, and published as lexical in 0.1.0 on 2026-10-13. A source
in proposals is given relative to `libraries/lexical/openspec/`.

**Scope.**

- **lexical is a library of webcpp's own (`"port-of": null`) and ports
  nothing: its evidence is the RFCs it cites, its unit tests, the official
  JSON-Schema-Test-Suite run against its validators, and V8 for `regex`.** No
  C++ library answered, for every string format JSON Schema names, exactly as
  the RFCs and ECMA-262 define it, without allocating and without exceptions,
  natively and on WebAssembly; webcpp::json_schema needs one for its `format`
  keyword.
  Source: `changes/archive/2026-10-09-genesis/proposal.md` (proposals);
  `AGENTS.md` (libs/lexical).
- **lexical checks the lexical space of a format and nothing beyond it: it
  never resolves a URI, looks up a host, checks a mailbox, compiles or runs a
  pattern, or computes with a date beyond the days of its month.** (reason not
  recorded)
  Source: `changes/archive/2026-10-09-genesis/design.md`, D1 and Non-Goals
  (proposals).
- **The library was renamed from formats (`webcpp::formats`,
  `<webcpp/formats/...>`) to lexical on 2026-10-13, and no name of the old
  spelling remains.** "formats" read as Boost.Format or `std::format` to a C++
  developer.
  Source: `changes/archive/2026-10-09-genesis/design.md`, Context (proposals).
- **JSON Schema v1's RFC 9557 `date-time-ext` suffixes and its "published leap
  seconds", the formats of v1's registry beyond the 17 of its body (`int32`,
  `base64url`), and the content keywords are out of scope.** v1 is unpublished
  and drifting and no test covers them; when it is published, lexical adds
  them as a contract extension, a new function or policy and never a changed
  default; content is json_schema's.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 1
  and risk 8 (proposals).

**Interface.**

- **Each format is a validator `is_<name>(std::string_view)` returning `bool`,
  `[[nodiscard]]`, its hyphens written as underscores, and, but `regex`, a
  Boost.URL rule in `webcpp::lexical::grammar`.** A rule composes with
  Boost.URL's combinators into a program's own grammar; validators of their
  own beside Boost.URL, the alternative rejected, would compose with nothing.
  Source: `changes/archive/2026-10-09-genesis/design.md`, D2 (proposals).
- **A validator is its rule parsing the whole string,
  `boost::urls::grammar::parse(s, x_rule).has_value()`, so the validator
  refuses trailing input with `error::leftover` while the rule, inside a
  grammar, stops where its format ends.** The function and its rule then never
  disagree (the page's invariant 1), which each unit test checks on every
  input it holds.
  Source: `specs/lexical/spec.md`, "Library: Trailing input is refused through
  leftover" (proposals); `doc/guarantees.adoc` (libs/lexical).
- **Rule types are public, `<name>_rule_t`, each with an `inline constexpr`
  object `<name>_rule`, and documented as a contract.** A rule that takes a
  parameter is then spelled without a factory
  (`time_rule_t{leap_seconds::refused}`, `iri_rule_t{&resource}`); Boost.URL
  hides its own rule types as `implementation_defined`.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P2
  (proposals).
- **A rule yields the text it matched, `grammar::text`, an alias of
  `boost::core::string_view`; only `uri_rule` and `uri_reference_rule` yield
  Boost.URL's `url_view`.** It is Boost.URL's text type, which converts to
  `std::string_view`; parsed values (a date's fields) are out of scope, and
  adding them later is new rules, not a change of these.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P1
  (proposals); `include/webcpp/lexical/grammar.hpp` (libs/lexical).
- **`ipv4` and `ipv6` have no rule of lexical's: `is_ipv4` and `is_ipv6` parse
  with Boost.URL's `ipv4_address_rule` and `ipv6_address_rule`, which a
  program's grammar names directly.** The owner decided that ASCII grammars
  are written with Boost.URL's grammar library, reusing its IP rules, which
  refuse a leading zero and fail no test of the suite.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 2.1
  item 5 and section 5.5 (proposals).
- **`regex` has no rule.** A pattern has no grammar of its own inside a longer
  input, so `is_regex` judges a whole string.
  Source: `specs/lexical/spec.md`, "Regex: no rule" (proposals);
  `doc/regex.adoc` (libs/lexical).
- **What differs between JSON Schema's dialects is two policy enumerations,
  never a dialect argument: `leap_seconds` for `time` and `index_manipulation`
  for `relative-json-pointer`, both defaulting to `allowed`, each a public
  member of its rule type.** v1's suite inverts six leap-second tests of
  `time` ("This MUST NOT include leap seconds"); drafts 7 and 2019-09 cite
  draft-handrews-relative-json-pointer-01, which has no index manipulation,
  and 2020-12 and v1 draft-bhutton-relative-json-pointer-00, which has it.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P4, section
  1 and section 5.4 (proposals).
- **`date-time` allows a leap second in every dialect, v1 included.** The
  suite's `date-time` tests of every dialect, v1's among them, say so.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P4
  (proposals).
- **An index manipulation is accepted before `#` too (`1-1#`).** The draft's
  own example `0-1#` puts it there though its ABNF omits it, so lexical takes
  the union of what the draft treats as valid.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.4
  (proposals); `doc/identifiers.adoc` (libs/lexical).
- **`<webcpp/lexical.hpp>` includes every format but `regex`, whose header is
  included by name.** `<webcpp/lexical/regex.hpp>` brings the Unicode tables
  of ECMA-262's classes, which a program that never validates a pattern should
  not compile.
  Source: `include/webcpp/lexical.hpp` (libs/lexical).

**Errors and exceptions.**

- **lexical has no error category of its own: a rule answers Boost.URL's
  `error::mismatch`, `error::invalid` or `error::leftover`, or the
  `webcpp::idna` error that refused a name.** `mismatch` is kept for text that
  cannot start the format, so that a `variant_rule` tries its next
  alternative; `invalid` says the format started and broke a constraint; an
  internationalized rule hands idna's reason (`contexto`, `bidi`) to its
  caller.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P3 and
  section 6 (proposals).
- **A rule validates the whole run it takes and fails with `error::invalid`
  rather than match a shorter prefix; a rule over a run takes every byte its
  format may hold.** A grammar that composes the rule then meets a failure,
  not a silent short match.
  Source: `specs/lexical/spec.md`, "Library: A rule fails whole rather than
  match a prefix" (proposals).
- **A rule that fails leaves its iterator where it was, and a rule reads `[it,
  end)` alone, never a byte at or past `end`.** (reason not recorded)
  Source: `changes/archive/2026-10-09-genesis/design.md`, D2 (proposals);
  `doc/guarantees.adoc` (libs/lexical).
- **Where each rule stops inside a longer input is part of its contract, and
  an internationalized rule's run ends only at an ASCII byte outside letters,
  digits, `-` and `.`.** UTS 46 may map any non-ASCII code point, so every
  non-ASCII byte belongs to the name; likewise `json_pointer_rule` runs to the
  end of the input once a `/` starts it, since a token holds every character a
  grammar could continue with.
  Source: `doc/composing.adoc`; `include/webcpp/lexical/detail/idn_scan.hpp`
  (libs/lexical).
- **The e-mail rules look for `[` themselves and ask the address literal's
  rule or the host rule, rather than make the domain an alternative of a
  `variant_rule`, and turn the host rule's `mismatch` into `invalid`.**
  Boost.URL's `variant_rule` answers `mismatch` when every alternative fails,
  whatever each said, which would lose idna's reason for refusing the domain;
  once a local part and `@` matched, the Mailbox has started.
  Source: `include/webcpp/lexical/email.hpp` (libs/lexical);
  `changes/archive/2026-10-09-genesis/reference/spec.md`, section 4.3
  (proposals).
- **`is_regex` has neither a `grammar::result` nor an error code: a refusal is
  `false`, and the reason for the first fault, with a verdict of three values
  (`valid`, `invalid`, `beyond_bounds`), stays in `detail::check_regex` for
  tests and diagnostics.** Every byte string, a pattern past a bound included,
  is then answered with a value (the page's invariant 6).
  Source: `specs/lexical/spec.md`, "Regex: the verdict of the reader" and
  "Regex: always answers, never fails" (proposals).
- **lexical raises nothing of its own, so `WEBCPP_LEXICAL_NO_EXCEPTIONS`
  changes nothing in it; every validator and rule is `noexcept` but the four
  IRI functions and the two IRI rules.** Only an IRI's mapped copy beyond its
  inline buffer allocates, and a memory resource that refuses throws what it
  throws (`std::bad_alloc`), which lexical lets through rather than answer a
  verdict without the mapped copy.
  Source: `changes/archive/2026-10-09-genesis/design.md`, D7 (proposals);
  `include/webcpp/lexical/config.hpp` (libs/lexical).

**Memory.**

- **Nothing allocates but an IRI whose mapped URI exceeds 1,024 octets, which
  takes one buffer of its mapped length from the `std::pmr::memory_resource`
  given to an overload or a rule's `resource` member, else from
  `std::pmr::get_default_resource()`, and gives it back before it returns.**
  Boost.URL parses a contiguous `string_view`, so the mapped URI needs
  storage: an ASCII IRI is parsed as it is, never copied, and the inline
  buffer holds any IRI of up to 341 octets, since mapping at most triples a
  length; json_schema passes its validation's resource.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P5
  (proposals); `include/webcpp/lexical/detail/iri_mapping.hpp` (libs/lexical).
- **An IRI's code points are all checked before anything is mapped.** An IRI
  refused for a code point takes no buffer, so even a resource that refuses
  every allocation gets that answer.
  Source: `specs/lexical/spec.md`, "URIs: The mapped URI buffer holds 1,024
  octets" (proposals).
- **A `range_rule` takes a nested rule through `boost::urls::grammar::ref`.**
  Boost.URL keeps a rule larger than 128 bytes in `recycled_ptr`, a
  process-wide pool that allocates outside any resource and is static mutable
  state; the allocation test fails if the pool is ever reached.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, risk 3
  (proposals); `doc/composing.adoc` (libs/lexical).
- **Where the URI template's ABNF repeats, a `detail::repeat_rule` of
  lexical's own matches the repetitions and yields their text, in place of
  Boost.URL's `range_rule`.** `range_rule` builds a type-erased range through
  a virtual call for each repetition, about 30 ns per variable of a template,
  while `repeat_rule` keeps no value of its parts.
  Source: `include/webcpp/lexical/detail/rules.hpp`;
  `include/webcpp/lexical/uri_template.hpp` (libs/lexical).
- **idna works in fixed storage: an A-label is lowercased into a 63-octet
  stack buffer, and `to_ascii` writes into a `std::array<char,
  idna::max_domain_length>`.** No host rule allocates, and a hostile label of
  100,000 octets is stopped at that storage with
  `idna::error::label_too_long`.
  Source: `specs/lexical/spec.md`, "Hosts: hostile and oversized IDN input"
  (proposals).
- **`is_regex` takes no memory resource and stores nothing of the pattern:
  what it needs of the whole pattern (the count of its groups, the group a
  `\k` names, where two groups of one name sit) it reads again from the
  text.** It allocates nothing on any target, its state on the stack to the
  depth `max_regex_nesting` allows.
  Source: `include/webcpp/lexical/detail/regex_validator.hpp` (libs/lexical);
  `specs/lexical/spec.md`, "Regex: no allocation and no exceptions"
  (proposals).

**URIs and IRIs.**

- **`uri` and `uri-reference` are Boost.URL 1.92's `parse_uri` and
  `parse_uri_reference`, which lexical composes and never modifies, patches or
  copies; where Boost.URL is more liberal than RFC 3986, lexical guards it and
  offers the fix upstream.** The owner ruled lexical faithful to RFC 3986, the
  reference JSON Schema names for these formats (2026-10-09).
  Source: `changes/archive/2026-10-09-genesis/design.md`, D3, and
  `reference/spec.md`, P11 (proposals).
- **Four guards of `detail/uri_guard.hpp` refuse, after Boost.URL's rule
  succeeds, what Boost.URL accepts and RFC 3986 refuses: a relative reference
  whose path after an authority is neither empty nor led by `/`, `[` or `]` in
  a query, `#` in a fragment, and a zone identifier in an IP literal.** RFC
  3986 4.2 gives that path `path-abempty`, 3.2.2 holds brackets only around an
  IP literal, 3.5 holds a second `#` only as `%23`, and RFC 9844 (2025)
  obsoleted RFC 6874's zone identifier and reverted its change to RFC 3986.
  Source: `include/webcpp/lexical/detail/uri_guard.hpp`; `AGENTS.md`
  (libs/lexical).
- **A fifth guard, `refuses_authority_before_segment`, makes both URI rules
  refuse with `error::invalid` a reference that ends at its authority before a
  character that starts a path segment.** Boost.URL stops a URI at the second
  `@` of `http://a@b@example.com/`, so its rule would match `http://a@b`
  inside a larger grammar; this guard stays when Boost.URL refuses the
  relative forms, since a fixed Boost.URL still stops there.
  Source: `specs/lexical/spec.md`, "URIs: Guard against a reference that ends
  at its authority before a path segment" (proposals).
- **Each guard has a test that pins Boost.URL's wrong verdict
  (`boost_url_still_*`) and fails the day the configured Boost changes it; the
  guard then leaves in a change of its own with a changelog line, and the test
  of the RFC's verdict stays.** A Boost release that fixes or regresses a
  verdict is seen, and each guard leaves apart from the others.
  Source: `changes/archive/2026-10-09-genesis/design.md`, D3 (proposals);
  `AGENTS.md` (libs/lexical).
- **lexical never loosens Boost.URL: an `IPvFuture` led by an uppercase `V`
  stays refused, though RFC 5234 makes the ABNF's `"v"` case-insensitive, and
  is listed in the page's "Known deviations".** (reason not recorded) A test
  pins the verdict, which flips to RFC 3986's the day Boost.URL takes it.
  Source: `doc/uris.adoc`; `AGENTS.md` (libs/lexical).
- **An IRI is checked code point by code point, mapped to a URI with each
  octet of a non-ASCII code point as `%XX`, and parsed by `uri_rule` or
  `uri_reference_rule`, guards included; its host takes no IDNA.** Boost.URL
  parses RFC 3986 only, and `%XX` is valid exactly where RFC 3987 allows
  `ucschar`, so Boost.URL judges the structure, refuses a non-ASCII scheme or
  port, and every guard applies to the IRIs alike.
  Source: `changes/archive/2026-10-09-genesis/design.md`, D4 (proposals);
  `AGENTS.md` (libs/lexical).
- **A non-ASCII code point inside an IP literal is refused before anything is
  mapped.** RFC 3987 takes its IP literal from RFC 3986, ASCII alone, and
  refusing it there keeps that verdict independent of the zone guard.
  Source: `AGENTS.md` (libs/lexical).
- **`iprivate` is valid in the query alone, the query being the text after the
  first `?` that precedes the first `#`, and the bidirectional formatting
  characters are refused anywhere, the query included.** No valid IRI holds a
  `?` before its query or a `#` before its fragment (RFC 3986 appendix B); RFC
  3987 4.1 forbids the formatting characters outside its ABNF.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.8
  (proposals).
- **An IRI rule takes the run up to the first byte no IRI holds and validates
  it whole; `iri_reference_rule` matches empty text there, and never answers
  `mismatch`.** A rule fails whole rather than match a shorter prefix, and the
  empty string is an IRI reference.
  Source: `specs/lexical/spec.md`, "URIs: Where an IRI ends inside a larger
  grammar" (proposals).

**Host names and e-mail addresses.**

- **A `hostname` label that starts with `xn--`, in any case, is lowercased and
  judged by `idna::check_label`, so an A-label is valid in `hostname` exactly
  when it is valid as a one-label `idn-hostname`.** `XN--NXASMQ6B` is then
  valid in both formats, its case folded as UTS 46 maps an ASCII label.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P9
  (proposals); `doc/addresses.adoc` (libs/lexical).
- **An R-LDH label (`ab--cd`) is a valid `hostname` and an invalid
  `idn-hostname`.** RFC 1123 allows it and draft 4's suite says so, while RFC
  5890's R-LDH labels are not NR-LDH; the page shows the two verdicts side by
  side, so that the difference reads as a decision, not a bug.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P9 and risk
  11 (proposals).
- **`hostname` judges each A-label alone, and only `idn-hostname` checks RFC
  5893's Bidi rule across the labels of a name.** (reason not recorded) The
  page states it as a decision of lexical's own.
  Source: `doc/addresses.adoc` (libs/lexical).
- **`idn-hostname` hands its whole run to `idna::to_ascii` with idna's default
  options (UTS 46's mapping, nontransitional processing, every check,
  `verify_dns_length`, CONTEXTO, NV8 and XV8 refused), so a name's lengths are
  those of its ASCII form.** The suite's mapping cases and its strict cases
  (the DISALLOWED exceptions, CONTEXTO, CONTEXTJ, the Bidi rule across labels)
  are all idna's to answer.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.10
  (proposals).
- **Besides `.`, exactly U+3002, U+FF0E and U+FF61 end a label, and
  `hostname_test` converts every code point between two letters to keep it
  so.** They are the only code points UTS 46 maps to a sequence holding a dot
  in idna's tables (Unicode 17.0.0), so a label the rule accepts is one label,
  and a version of idna that adds one fails the test.
  Source: `include/webcpp/lexical/detail/idn_scan.hpp`; `AGENTS.md`
  (libs/lexical).
- **`idna_label_rule`, a terminal of lexical's own, judges one
  internationalized label alone; a grammar that joins labels checks the Bidi
  rule across them and the name's 253 octets itself, or hands the whole name
  to `idn_hostname_rule`.** A grammar then joins labels with separators of its
  own.
  Source: `doc/composing.adoc`; `include/webcpp/lexical/hostname.hpp`
  (libs/lexical).
- **lexical asks `webcpp::idna` through its public interface alone, and a
  verdict of idna's that a test of lexical finds wrong becomes a test in idna,
  red first, never a workaround in lexical.** No library reaches into
  another's `detail`, which an include boundary checks; a red test in idna is
  idna's own process (its specification, section 9).
  Source: `AGENTS.md` (libs/lexical);
  `changes/archive/2026-10-09-genesis/reference/spec.md`, P12 and section 9.3
  (proposals).
- **`email`'s domain is `hostname_rule`, A-labels included, and its address
  literals are `[` IPv4 `]` and `[IPv6:` IPv6 `]` alone, the tag in any
  case.** RFC 5321's General-address-literal needs a tag registered with IANA,
  and `IPv6` is the only one, so `[RFC-5322-domain-literal]` is invalid, as
  the suite says.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P10
  (proposals).
- **An address literal's address is judged by Boost.URL's IP rules, not by RFC
  5321 4.1.3's `Snum` and `IPv6-comp`, which changes two verdicts:
  `[001.2.3.4]` is invalid and `[IPv6:1:2:3:4:5:6::8]` valid.** The owner
  decided that ASCII grammars are written with Boost.URL's grammar library,
  reusing `ipv4_address_rule` and `ipv6_address_rule`, the latter for e-mail's
  `[IPv6:...]` too; RFC 4291 allows `::` for one group.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 2.1
  item 5 (proposals); `doc/addresses.adoc` (libs/lexical).
- **A Mailbox is at most 254 octets, and its local part at most 64, a
  Quoted-string's quotes counted.** RFC 5321 4.5.3.1.3 bounds a path at 256
  octets with its two angle brackets, and 4.5.3.1.1 a local part at 64.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.11
  (proposals); `include/webcpp/lexical/email.hpp` (libs/lexical).
- **An internationalized Mailbox's 64 and 254 octets are measured as written,
  in UTF-8, while its domain's DNS lengths are those of its ASCII form.** RFC
  6531 sets no limit of its own, RFC 5321's are octets, and an SMTPUTF8
  envelope carries the address as written.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.11
  (proposals).
- **An internationalized local part holds any well-formed non-ASCII UTF-8, C1
  controls and noncharacters included, and is not normalized, while a quoted
  pair stays ASCII.** RFC 6532 3.1 adds `UTF8-non-ascii` to `atext` and
  `qtextSMTP` alone, and the suite holds a local part not in NFC valid.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.11
  (proposals).

**Dates, identifiers and templates.**

- **The date and time rules take ASCII digits only, `T` and `Z` in either
  case, and a duration's designators in either case.** RFC 3339's note allows
  either case of `T` and `Z`, and RFC 5234 2.3 makes ABNF's quoted strings, a
  duration's designators among them, case-insensitive.
  Source: `doc/dates.adoc` (libs/lexical).
- **Second 60 is valid when the time, moved to UTC by its offset, is 23:59,
  and its date is never checked against IERS's list of leap seconds.** RFC
  3339 5.7 allows a leap second there; with no table, the answer is the same
  every year.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.1
  (proposals); `doc/dates.adoc` (libs/lexical).
- **The duration rule is RFC 3339 appendix A's ABNF, one rule per production
  in its order, and a duration's numbers, like a time's fraction, are never
  converted.** Any number of digits is then valid, and nothing overflows or
  allocates however long it is.
  Source: `include/webcpp/lexical/date_time.hpp`; `doc/dates.adoc`
  (libs/lexical).
- **A UUID is checked for its shape alone, hexadecimal digits in either case,
  its version and variant nibbles unchecked.** The suite accepts version 15
  and an undefined variant.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.2
  (proposals).
- **A JSON Pointer is decoded as strict UTF-8, a NUL is a valid character of a
  token, and its rule never answers `mismatch`, matching empty text where no
  `/` starts the input.** RFC 6901's `unescaped` is every code point but `/`
  and `~`, `%x00` included, and the empty string is a JSON Pointer.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.3
  (proposals); `include/webcpp/lexical/json_pointer.hpp` (libs/lexical).
- **A URI template is checked at any level of RFC 6570 without being expanded,
  with erratum 6937's apostrophe as a literal, the reserved operators of
  `op-reserve` accepted, and `ucschar` and `iprivate` code points as
  literals.** The suite holds `a'b` valid, the ABNF admits the reserved
  operators, and a template of an IRI is valid too.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, section 5.9
  (proposals); `doc/templates.adoc` (libs/lexical).

**Regular expressions.**

- **`is_regex` is a syntax validator of lexical's own, a recursive-descent
  reader of well-formed UTF-8 that answers whether a pattern is ECMA-262's
  `Pattern[+UnicodeMode]`, early errors included, with ECMAScript 2025's
  modifiers and duplicate group names.** The owner abandoned SRELL and
  webcpp::regex, which the first `is_regex` compiled with, on 2026-10-12;
  Boost.Regex was rejected as the validator because it allocates outside any
  memory resource and its syntax is not ECMA-262's.
  Source: `changes/archive/2026-10-09-genesis/design.md`, D6 (proposals).
- **Valid means that the syntax and the early errors accept the pattern, and
  nothing more: `is_regex` compiles nothing, runs nothing, reads no flag but
  `u`, so the `v` flag's class sets are not read, and takes a lookbehind of
  any width.** JSON Schema's `regex` is judged as `new RegExp(pattern, "u")`
  judges it, and ECMA-262 allows a lookbehind of any width, which a validator
  that compiles nothing never measures.
  Source: `specs/lexical/spec.md`, "Regex: what is not judged" (proposals);
  `doc/regex.adoc` (libs/lexical).
- **`is_regex` has two bounds and no other: a pattern longer than
  `max_regex_length` (65,536 bytes) is answered `false` unread, and one whose
  groups, lookarounds and modifier groups nest deeper than `max_regex_nesting`
  (32) is answered `false` unjudged.** The validator reads parts of a pattern
  again, so the length bounds its time, and it reads a group by calling
  itself, so the nesting bounds its stack: 32 levels were set so that the
  deepest pattern, met in a hostile JSON instance, fits wasm32-wasip2's 64 KiB
  stack at `-O0`; alternatives, quantifiers and classes cost neither.
  Source: `specs/lexical/spec.md`, "Regex: pattern length bound", "Regex:
  group nesting bound" (proposals);
  `changes/archive/2026-10-09-genesis/reference/STATUS.md`, item 4
  (proposals).
- **The reader clamps any nesting it is asked for to `nesting_limit`, 64.**
  The functions that find the disjunctions holding a group keep their places
  in fixed arrays of `nesting_limit + 1` entries on the stack.
  Source: `include/webcpp/lexical/detail/regex_validator.hpp` (libs/lexical).
- **The reader never backtracks on a pattern's structure; it rereads the
  pattern with one reader of single tokens (`next_token`, which takes an
  escape or a whole class as one) for each group counted, each `\k` and each
  named group.** Its work is then bounded by a polynomial of the pattern's
  length, which `max_regex_length` bounds.
  Source: `specs/lexical/spec.md`, "Regex: bounded time on hostile patterns"
  (proposals).
- **Two groups share a name only when some disjunction holds them in different
  alternatives, which the reader checks for each named group against the last
  earlier group of that name.** That is ECMA-262 2025's rule (22.2.1.1): no
  match can hold both; and checking each group against the last earlier one
  checks every pair.
  Source: `include/webcpp/lexical/detail/regex_validator.hpp` (libs/lexical).
- **A pattern that is not well-formed UTF-8 is refused before its grammar is
  read, a NUL is a literal, and a code point above U+FFFF is one atom.** Every
  format reads code points through lexical's strict decoder, so that a verdict
  never depends on an ill-formed sequence being repaired or skipped.
  Source: `specs/lexical/spec.md`, "Library: UTF-8 rule shared by all" and
  "Regex: well-formed UTF-8 and NUL" (proposals).
- **The names `\p{...}` accepts and the ranges of ID_Start and ID_Continue are
  sorted tables generated from Unicode 17.0.0's data files, the ranges packed
  as `(first << 21) | last`, read through `detail/regex_unicode.hpp` alone; a
  `\p{...}` body over 64 bytes names nothing.** The data are idna's Unicode
  version, pinned by SHA-256, and the generated file is checked byte for byte
  against its generator (the page's invariant 7); past 64 bytes, no body names
  a property.
  Source: `specs/lexical/spec.md`, "Regex: unicode tables" and "Regex: reading
  the tables" (proposals); `include/webcpp/lexical/detail/regex_validator.hpp`
  (libs/lexical).
- **The script `Katakana_Or_Hiragana` has no name in the tables, and `Unknown`
  has one although no data file names it.** ECMA-262's table of the values of
  Script leaves out the script no code point has, and names `Unknown`, the
  value of the unassigned code points.
  Source: `tools/generate_tables.py` (libs/lexical).

**Dependencies and targets.**

- **lexical depends on Boost.URL and Boost.System (Boost 1.92 or newer),
  Boost's headers, webcpp::config and webcpp::idna, and on no regex library
  and no JSON library, which include boundaries enforce.** `is_regex` is
  lexical's own validator, and JSON is the tests' alone (`/webcpp//boost_json`
  in `regex_v8` and `suite`); each boundary states its reason, which the lint
  prints.
  Source: `build.jam`; `meta/include-boundaries.json` (libs/lexical).
- **A program links Boost.URL's compiled part, `/webcpp//boost_url`, built
  from Boost's sources for every target, never an installed `libboost_url`.**
  `ipv4_address`'s constructor from bytes and its `to_bytes()` live in
  Boost.URL's compiled sources, and on MSVC, which never defines
  `BOOST_URL_HAS_CXX20_CONSTEXPR`, even the grammar's error category; one set
  of sources serves native, wasip2, wasip3 and emscripten (measured: a
  stripped wasip2 program that calls `is_uri` is 85,625 bytes larger than an
  empty one).
  Source: `specs/lexical/spec.md`, "Library: Boost.URL's compiled part"
  (proposals); `changes/archive/2026-10-09-genesis/reference/spec.md`, section
  8.2 and the owner's decision on S4 (proposals).
- **lexical has a strict UTF-8 decoder of its own, `detail::decode_utf8`,
  about 20 lines on RFC 3629's table, rather than idna's.** No library reaches
  into another's `detail`.
  Source: `changes/archive/2026-10-09-genesis/reference/spec.md`, P12
  (proposals); `include/webcpp/lexical/detail/utf8.hpp` (libs/lexical).
- **The tests and the examples take `/bigobj` on MSVC.** A program that
  includes `regex.hpp` holds the Unicode tables beside Boost.URL and idna,
  which MSVC compiles only with `/bigobj`.
  Source: `test/Jamfile`; `example/Jamfile` (libs/lexical).

**Evidence.**

- **The official JSON-Schema-Test-Suite's format tests run directly against
  the validators on every target, read at run time from the superproject's
  submodule, and no copy of its data is committed.** It is MIT data, and every
  file of webcpp is BSL-1.0; moving the submodule is a change of its own, with
  every new failure fixed or listed in it.
  Source: `AGENTS.md` (libs/lexical);
  `changes/archive/2026-10-09-genesis/reference/spec.md`, P8 (proposals).
- **Each dialect has a known-failure list that may name only a residue tied to
  a Unicode version, and the runner fails on an unlisted failure, on a listed
  test that passes or that the suite does not hold, on a test named twice and
  on an entry without its four fields.** A list then never lies; every list is
  empty, and the 3,540 string tests pass.
  Source: `specs/lexical/spec.md`, "Library: Known failures" (proposals);
  `AGENTS.md` (libs/lexical).
- **The runner calls the same validator in every dialect but where a dialect's
  text cites a policy, from a table each of whose entries must name a dialect
  and a format that exist.** The suite tests no index manipulation, so a
  misspelt entry would otherwise fall to the default validator without a word.
  Source: `specs/lexical/spec.md`, "Library: Dialect policies of the runner"
  (proposals).
- **The runner judges a test on its format alone: a non-string instance must
  expect valid and is counted apart, the `unknown` format is skipped by name,
  and a schema with a keyword besides `format` and `$schema` fails the run.**
  A format applies to strings only and an unknown format is json_schema's
  question, and a keyword that changes a verdict would be misread.
  Source: `specs/lexical/spec.md`, "Library: What the runner does not judge"
  (proposals).
- **V8 is `is_regex`'s oracle: the lane `oracle` runs `new RegExp(pattern,
  "u")` in the CI's Node (26.7.0, V8 14.6, Unicode 17.0), with no npm package,
  over the generated patterns, and `regex_v8` requires `is_regex` to give V8's
  verdict on all 24,297 of them, on every target.** Only `update-expected`
  writes V8's verdicts, with the CI's Node: a Node whose V8 or Unicode differs
  may write other verdicts, which is a change of its own.
  Source: `specs/lexical/spec.md`, "Library: The V8 oracle lane" (proposals);
  `test/oracle/Jamfile` (libs/lexical).
- **The patterns compared with V8 are generated: a grammar's, drawn from seeds
  2 to 9 of Python's `random.Random`, 3,000 each, 30% of them with one of 41
  defects; every name `\p{...}` accepts and the same names misspelt; every
  pattern of the suite; and edge cases the grammar does not draw.** The
  grammar's draws are those of the study of Boost.Regex as an ECMA-262 engine,
  four subjects drawn and dropped after each, so that the patterns are the
  study's.
  Source: `tools/generate_patterns.py` (libs/lexical).
- **The Unicode tables and the patterns are generated from Unicode files
  pinned by URL and SHA-256 in `tools/unicode.json`, never edited by hand, and
  compared byte for byte by the own lane `generated`; the ordinary lanes never
  run a generator.** A generator may need the network, and a digest that
  differs stops it before anything is written.
  Source: `specs/lexical/spec.md`, "Regex: the generated-file lane"
  (proposals); `doc/regex.adoc` (libs/lexical).
- **lexical's and Boost.URL 1.92's URI verdicts were compared on 2026-10-09
  with a transcription of RFC 3986's ABNF, on inputs that put every ASCII
  character but NUL and line feed into each part of a reference and on
  references generated at random.** The page's "Known deviations" then lists
  every difference found, each pinned by a test.
  Source: `doc/uris.adoc` (libs/lexical).
- **The allocation test replaces the global `operator new` and `operator
  delete`, plain and aligned and `BOOST_NOINLINE`, counts every call over
  every validator on long and hostile inputs (a megabyte of each family among
  them), and ends its memory resources in a resource of the program's own heap
  rather than `std::pmr::new_delete_resource()`.** MSVC's runtime library
  defines `new_delete_resource` in its DLL, whose calls of `operator new`
  never reach a replacement the program defines; GCC, inlining one operator,
  would pair its `malloc` with the other's `free` and warn.
  Source: `test/counting_resource.hpp`; `test/allocation_test.cpp`
  (libs/lexical).

### document (JSON with JavaScript's behaviour; own; implemented in 0.1.0)

document is a JSON document whose behaviour is JavaScript's (`JSON.parse`,
property access, assignment, `delete`, the array methods, `JSON.stringify`,
the same key order and the same text out), exact in decimal, whose tree lives
in one relocatable binary image that is copied, sent, opened in place and
shared, and which is also a JavaScript module and an npm package. A source in
proposals is given relative to `libraries/document/openspec/`.

**Scope and dependencies.**

- **document is webcpp's own library and ports nothing: its evidence is
  JavaScript itself, run in Node as an oracle.** The reasons it exists are
  properties of `boost::json::value` (key order, an `erase` that reorders,
  numbers as doubles, hashed objects, pointers), so there was nothing to port.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **Of Boost.JSON it uses `basic_parser` alone, whose SAX handler builds the
  tree, and none of its value types; `build.jam` links `/webcpp//boost_json`
  for it.** Wrapping `boost::json::value` was rejected, as it cannot be made
  relocatable or exact; Boost.JSON's parser is not header-only.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **Doubles are read and written through Boost.Charconv, so `build.jam` also
  links `/webcpp//boost_charconv`.** Xcode 16's libc++ deletes the
  floating-point `std::from_chars`, and one code path serves every target.
  Source: `include/webcpp/document/traits.hpp` (libs/document).
- **Of Boost.Interprocess only `offset_ptr.hpp`,
  `mem_algo/rbtree_best_fit.hpp` and `sync/null_mutex.hpp` are included, with
  a mutex family of document's own, `detail::no_mutexes`; never
  `managed_external_buffer`, `segment_manager`, `allocators/allocator.hpp` or
  `sync/mutex_family.hpp`.** Those throw bare and do not compile without
  exceptions.
  Source: `include/webcpp/document/detail/heap.hpp` (libs/document).
- **0.1.0 leaves out BigInt and fixed 256-bit integers.** `decimal128_t` holds
  34-digit integers exactly, and Boost.Multiprecision's `int256_t` is 48
  bytes, not trivially copyable, with bytes that differ between compilers and
  targets (measured).
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **0.1.0 also leaves out `toJSON`, replacers and revivers other than
  `revive_dates` and `revive_bytes`, Map, Set, RegExp, prototypes, getters,
  symbols, JSON Pointer, JSONPath beyond the selectors, schemas, a document
  inside a segment of several documents, durability, and the recovery of a
  dead process's references.** Each is left to a later proposal (reason not
  recorded).
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **Speed is a non-goal: 0.1.0 is not tuned against Boost.JSON or V8, and
  the page states where document loses, with measurements.** Every operation
  pays for JavaScript's behaviour, exact numbers and one relocatable image;
  honesty about that cost is a goal, and the profile names where tuning
  begins (the lookup by id behind every handle, the renumbering on a splice,
  the image `pop` allocates).
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).

**Behaviour.**

- **JavaScript's behaviour is the contract: the same key order, the same
  `JSON.stringify` bytes, the same array methods and index rules, each
  JavaScript name a member function of the same meaning.** A C++ program that
  must agree byte for byte with a JavaScript peer needs a document that makes
  JavaScript's decisions; one that knows JavaScript then knows document.
  Source: `changes/archive/2026-10-12-genesis/proposal.md` (proposals).
- **`set` of a handle or a document stores a deep copy, never an alias, and
  reads its source before it changes anything, so a value may be copied into
  its own subtree and `a.splice(0, 1, a)` inserts a copy.** A document is a
  tree, as JSON is: no value is shared and no cycle exists.
  Source: `doc/javascript.adoc` (libs/document).
- **`undefined` is never stored: `set(wd::undefined)` erases a member, stores
  `null` in an element, and makes a root hold `undefined`, which writes no
  text.** The text is the one `JSON.stringify` writes; only `Object.keys`
  would differ.
  Source: `doc/javascript.adoc` (libs/document).
- **Arrays are dense, `wd::dense` being the one array policy: a longer
  length or a write past the end fills with `null`, so `pop` or `shift` of a
  former hole returns `null`.** JSON has no holes and writes one as `null`, so
  the text is the same.
  Source: `doc/arrays.adoc` (libs/document).
- **A key reaches only a member and an index only an element: `arr["0"]` and
  `obj[3]` name nothing.** A handle names a member or an element, never both.
  Source: `doc/javascript.adoc` (libs/document).
- **`pop`, `shift`, `splice`, `slice` and `concat` return what they remove or
  copy in a new owned document, `result<document>`, which holds `undefined`
  for an empty array.** The elements then outlive the array; the cost, an
  image allocated per `pop`, is measured and named as a place to tune.
  Source: `doc/arrays.adoc` (libs/document).
- **Indices follow ECMA-262's relative index: a negative index counts from
  the end, clamped at 0, an index past the length is the length, and
  `INT64_MIN` and `INT64_MAX` clamp.** Indices follow JavaScript exactly.
  Source: `doc/arrays.adoc` (libs/document).
- **`==` compares structurally, key order aside: numbers by value, strings,
  dates and bytes by value, arrays element by element.** JavaScript has no
  deep equality, so this one is the document's, a ruling of the owner (reason
  not recorded).
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **`value_or(fallback)` is `??`, and also gives the fallback when the value
  cannot be read as the fallback's type.** (reason not recorded)
  Source: `include/webcpp/document/value.hpp` (libs/document).
- **`wd::object` and `wd::array` write JSON in C++ as `{...}` and `[...]`
  are written in JavaScript; a `wd::literal` converts implicitly from each
  value it holds and owns what it holds.** JavaScript's `[1, "x"]` needs no
  conversion written, and neither does `wd::array{1, "x"}`.
  Source: `include/webcpp/document/literal.hpp` (libs/document).

**Numbers.**

- **The default traits store a number exactly, in one of two forms chosen by
  value, never by spelling: an integer that fits `std::int64_t` is an
  `integer`, any other number a normalised `decimal128_t`; `-0` is `0`.**
  What is stored is what was sent, and `1`, `1.0` and `1e0` are one stored
  value as they are one number in JavaScript; `JSON.stringify(-0)` writes
  `0` too.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **Number text is read into its decimal digits by `scan_number`, never
  through a double; more than 34 significant digits or an exponent past
  `decimal128_t` is `errc::range`, and `1e999` is stored exactly.** The
  default traits keep what was sent and refuse what they cannot hold exactly;
  a JavaScript peer reads `1e+999` as `Infinity`.
  Source: `doc/numbers.adoc` (libs/document).
- **Numbers are written by ECMA-262's Number::toString on the stored digits,
  four rows on `k` and `n`, into a `number_text` of 64 characters held by
  value.** Every number `JSON.stringify` writes is then written back byte for
  byte, a number a double cannot hold stays exact, and writing a number or a
  date allocates nothing.
  Source: `include/webcpp/document/number.hpp` (libs/document).
- **`js_traits` hold JavaScript's own doubles: a number rounds as
  `JSON.parse` rounds it, is stored as an `integer` when that double is
  integral and fits, and NaN and the infinities are stored and written
  `null`.** A number then writes back as Node writes the double, and agrees
  with JavaScript on every digit.
  Source: `doc/numbers.adoc` (libs/document).
- **`get<T>()` never truncates or rounds: a number `T` cannot hold exactly is
  `errc::range`, a character type is no number, and a floating-point type
  converts only when the traits' `number` is binary.** A program cannot lose
  digits by accident.
  Source: `include/webcpp/document/detail/convert.hpp` (libs/document).

**Strings, dates and bytes.**

- **The traits' `string` is the stored encoding of strings and keys, `utf8`
  by default or `utf16` in `js_traits`; `length()` counts UTF-16 code units
  either way.** `utf16` is how JavaScript holds strings, and JavaScript counts
  a length in UTF-16 code units.
  Source: `doc/strings.adoc` (libs/document).
- **Text is validated where it enters, by the parser and by `set`
  (`errc::type`); a view reads only the stored encoding, and `std::string`
  and `std::u16string` transcode.** A view reads a string as it is stored and
  cannot transcode.
  Source: `doc/strings.adoc` (libs/document).
- **The parser refuses a lone surrogate with either encoding; `set` keeps one
  under `utf16`, and `stringify` writes it as `\u` and four digits.**
  Boost.JSON's parser cannot deliver one: it refuses it or, with
  `allow_invalid_utf16`, replaces it with U+FFFD, which would change the text
  silently, so the limit is documented and not worked around.
  Source: `doc/parsing.adoc` (libs/document).
- **Dates and bytes are kinds of their own beside JSON's.** JavaScript
  programs send dates and bytes through JSON, as text.
  Source: `doc/strings.adoc` (libs/document).
- **A date is a `date_ms`, milliseconds since the epoch within JavaScript's
  8.64e15 ms either way, written as `Date.prototype.toJSON` writes it, with
  Howard Hinnant's calendar algorithms.** That is how a JavaScript `Date`
  holds a date; `std::chrono::year` does not reach every year a 64-bit count
  of milliseconds does.
  Source: `include/webcpp/document/traits.hpp` (libs/document).
- **Bytes are stored raw and written by the traits' bytes policy: `base64`
  by default, `base64url`, or `javascript_bytes` (`{"0":1,...}`) in
  `js_traits`.** Bytes travel in the image as they are, and
  `javascript_bytes` restores JavaScript's text (reason for base64 as the
  default not recorded).
  Source: `doc/strings.adoc` (libs/document).
- **A parse keeps every string a string unless `revive_dates` or
  `revive_bytes` asks; `get<date_ms>()` and `get<std::vector<std::byte>>()`
  convert a string of the exact format anyway.** JSON has no dates and
  `JSON.parse` keeps strings.
  Source: `doc/strings.adoc` (libs/document).

**Key order.**

- **The traits' object policy orders keys: `javascript_order` by default
  (array-index keys, 0 to 4,294,967,294, ascending, then the others as
  created), `insertion_order`, or `sorted` by UTF-16 code units.**
  JavaScript's OrdinaryOwnPropertyKeys is not insertion order; `sorted` is
  the order of `sort()` and RFC 8785, for canonical JSON, signatures and
  hashes.
  Source: `doc/order.adoc` (libs/document).
- **Assigning a key keeps its place, erasing and creating it again puts it
  where a new key goes, and a key repeated in text, a literal or
  `assign_from` keeps the last value in the first one's place.** It is what
  JavaScript and `JSON.parse` do (the owner's approvals).
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **An object's members are kept in ordered indices, never in a hashed one;
  no hashed container is used anywhere.** Keys come from untrusted text: a
  crafted 4 MB input took Boost.JSON 9.23 s to parse, and no input makes an
  ordered lookup slower than its logarithm.
  Source: `doc/order.adoc` (libs/document).

**Traits.**

- **Traits choose the stored types: `document_traits` fixes `boolean`,
  `integer` and `date` to `bool`, `std::int64_t` and `date_ms`, lets `number`
  be any `stored_scalar` with a `number_codec`, and allows `dense` arrays
  only.** (reason for fixing the others not recorded)
  Source: `include/webcpp/document/traits.hpp` (libs/document).
- **A `stored_scalar` is trivially copyable, of 1, 2, 4, 8 or 16 bytes, and
  stored by its `scalar_codec` least significant byte first; `bool` takes one
  byte.** A scalar's bytes are then the same on every target, and the
  standard does not fix the size of `bool`.
  Source: `include/webcpp/document/traits.hpp` (libs/document).
- **The traits are part of the image: an FNV-1a hash of each codec's name and
  size and of the policies' names, with the sizes and alignments of the
  node, the container and the heap, is the header's layout tag.** An image of
  other traits or another layout is refused with
  `errc::incompatible_document`, never read wrongly.
  Source: `include/webcpp/document/detail/tree.hpp` (libs/document).

**Handles.**

- **A handle (`value`, `const_value`) holds the document's address and node
  ids, never a pointer into the tree, and resolves its place again at every
  call.** It stays valid when the image grows and moves, reads what is there
  now, and reads `undefined` once its node is erased.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **A handle names a node, a missing member or element of a container that
  exists, which `set` creates, or nothing; `operator[]` and `at` never
  fail.** JavaScript reads `undefined` where nothing is there, and `set`
  through nothing is `errc::type`, JavaScript's `TypeError`.
  Source: `doc/handles.adoc` (libs/document).
- **A handle is invalid once its document is moved from or destroyed, and a
  `std::string_view` read from a value is valid until the next change.** As a
  container's iterators are, a ruling of the owner.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **Assignment to a handle writes to its place and never rebinds it, so a
  handle is not a regular value and must not be swapped or sorted.** `h = x`
  is JavaScript's `v.key = x`, which writes; `std::swap` of two handles
  therefore writes through both.
  Source: `doc/handles.adoc` (libs/document).
- **`WEBCPP_DOCUMENT_NO_DANGLING` marks the handles, the ranges of a
  container's children and their iterators `[[gnu::no_dangling]]` on GCC 14
  and later.** Otherwise `-Wdangling-reference` takes
  `for (auto key : doc["a"].keys())` for a dangling reference.
  Source: `include/webcpp/document/config.hpp` (libs/document).

**Errors and exceptions.**

- **The core never throws: every operation that can fail returns `result`,
  and every writing function is `[[nodiscard]]` and completes or changes
  nothing, a full heap included, in a build with exceptions and in one
  without.** The parser's input is the network's, and a failure is a value
  the program tests, not a crash.
  Source: `doc/introduction.adoc` (libs/document).
- **`errc` names JavaScript's errors first, `syntax`, `type` and `range`,
  then `capacity`, `not_a_document`, `incompatible_document` and
  `damaged_document`, which JavaScript has no error for; the category's
  fixed identity is `0x646f63756d656e74`, the ASCII of "document".** The
  first three are the errors JavaScript throws for the same faults, and a
  fixed identity keeps the category free of mutable state.
  Source: `include/webcpp/document/errors.hpp` (libs/document).
- **The exception types are JavaScript's (`syntax_error` with the byte
  offset, `type_error`, `range_error`), with `capacity_error` and
  `image_error`, all derived from `error`, a `std::runtime_error` holding the
  code, and exist only with exceptions.** The sugar throws as JavaScript
  throws.
  Source: `doc/exceptions.adoc` (libs/document).
- **The sugar (`h = x`, `h = other`, `as<T>()`, `as_string()`, `as_bool()`,
  `parse_or_throw`, the owned document's default and copy constructors) is
  each a core function followed by `detail::raise`, with exceptions only.**
  It reads as JavaScript reads, and only what cannot return a `result` (an
  assignment, a constructor) needs exceptions, the owner's decision.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **Without exceptions the sugar stays declared, constrained by the concept
  `detail::exceptions_enabled<T>`, `exceptions_on && sizeof(T) != 0`; only
  its definitions sit behind `#ifndef WEBCPP_DOCUMENT_NO_EXCEPTIONS`.** A call
  then fails where it is made with a diagnostic that names the constraint, a
  generic program can ask with the concept whether the sugar is there, and
  the test `no_exceptions` holds the diagnostics.
  Source: `include/webcpp/document/config.hpp` (libs/document).
- **The process heap's `std::bad_alloc` is not an error value: the text
  `stringify` returns, a missing member's key in a handle, the parser's frames
  and pending text, and the staging of every tree change live there, mostly
  inside `noexcept` functions, where it ends the program.** The image's heap
  is never exhausted by surprise, since every operation finds its room
  first; the process heap fails as it does for any C++ container, which the
  library tolerates.
  Source: `include/webcpp/document/detail/tree.hpp` (libs/document).
- **An allocation in the image that fails after its probe passed is reported
  through `boost::throw_exception(std::bad_alloc())`.** It is a bug of the
  library, never a run-time event, since every operation finds its room
  first.
  Source: `include/webcpp/document/detail/heap.hpp` (libs/document).

**The image.**

- **A document is one image: a 64-byte header, then a Boost.Interprocess
  `rbtree_best_fit` heap, used directly and 16-aligned, holding one
  Boost.MultiIndex container of nodes.** One relocatable image can be copied,
  sent, opened in place, shared between threads and processes and handed to
  a Worker without a parse; a `boost::json::value` holds pointers and can do
  none of it.
  Source: `changes/archive/2026-10-12-genesis/proposal.md` (proposals).
- **Every pointer in an image is an `offset_ptr<T, std::int64_t,
  std::uint64_t, 8>` and every size a `std::uint64_t`.** The image then works
  at any 16-aligned address and has the same bytes on 64-bit native and
  wasm32 (measured: one digest on Clang, GCC, wasip2 and Emscripten).
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **The header holds the magic `WCPPDOC`, the format version, a byte-order
  mark, four reserved zero bytes, the layout tag, the size, the container's
  offset (always the heap's first block), the next node id and the seal, in
  the writer's byte order; another version, byte order, layout or nonzero
  reserved bytes is refused, never converted.** (reason for refusing rather
  than converting not recorded)
  Source: `include/webcpp/document/image.hpp` (libs/document).
- **The format version, `format_version` (1), is the library's own constant
  and the header carries no Boost version; a layout change raises it with a
  migration, and `format_test` compares golden compact images under both
  traits on every toolset.** Boost releases often and the structures the
  image holds rarely change, so a Boost upgrade must not change the format,
  and the goldens catch the release that does; they are written again only
  with a raised version.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **The tree is one `multi_index_container` with three ordered indices:
  `by_id`, `by_order` on parent, group and ordinal, and `by_key` on parent
  and key, by UTF-16 code units whatever the encoding.** `by_id` serves a
  handle's lookup, `by_order` an element by index and members in the
  policy's order, `by_key` a member by key and the key-ordered queries.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **Node ids are unique and never reused; the root's is 1.** Handles and
  iterators hold ids, so no change makes one name another node.
  Source: `specs/document/spec.md` (proposals).
- **A node takes 192 bytes whatever it holds, with a 16-byte scalar, and a
  string's or a key's bytes take a block of exactly their size, a
  `detail::byte_string`, never a `boost::container::string`.** A
  `boost::container::string` may allocate past the size and throws on
  failure.
  Source: `include/webcpp/document/detail/heap.hpp` (libs/document).
- **Free memory is zero: a buffer is zeroed when made or grown, the allocator
  zeroes each block it hands out and frees, nodes are built in place by
  `emplace`, and each index's empty key extractor and comparator carries a
  zeroed byte.** An erased value, a secret among them, leaves none of its
  bytes, and the image is a function of the operations alone; the byte is
  there because MSVC copies an empty temporary's byte without writing it,
  which put stack garbage in the image.
  Source: `include/webcpp/document/detail/tree.hpp` (libs/document).
- **An image is trusted input: opening checks the header, the heap's own
  sanity and the offsets to the root, not every offset inside the tree, and
  untrusted data enters as JSON text.** Those checks cannot prove every
  offset sound, so the page says "never open an image from a source you do
  not trust" wherever images appear.
  Source: `doc/images.adoc` (libs/document).

**Memory and storage.**

- **Storage is the second template parameter: `owned`, the document's own
  16-aligned buffer, which starts at 4,096 bytes and grows to twice its size
  or what an operation needs, up to `max_bytes` (1 GiB by default), or
  `external`, a caller's `std::span<std::byte>` such as a browser's memory,
  which never grows.** A document can live in memory it does not own, where a
  full buffer is `errc::capacity` and changes nothing (the 1 GiB default and
  the 4,096 start: reason not recorded).
  Source: `doc/images.adoc` (libs/document).
- **An owned buffer comes from the global aligned `operator new` with
  `std::nothrow`; document takes no `Allocator` parameter, and `external`
  storage is how a program places an image.** (reason not recorded)
  Source: `include/webcpp/document/detail/heap.hpp` (libs/document).
- **Every change finds its room first: it lists its blocks (a node bounded by
  `node_probe_bytes`, `sizeof(node) + 128`, a string at its exact size),
  probes the heap with the non-throwing `allocate` and frees the probes,
  grows an owned region or returns `errc::capacity`, then allocates for real
  and rearranges by steps that cannot allocate.** A full heap is then an
  error value and the document is unchanged; Boost.MultiIndex does not name
  its node type, hence the bound, and the heap is deterministic, so the
  probed blocks are found again.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **A `modify` only swaps prepared values.** Boost.MultiIndex erases an
  element whose `modify` throws.
  Source: `AGENTS.md` (libs/document).
- **`reserve` and `capacity` are `std::vector`'s for an image, and
  `shrink_to_fit` gives back only the free tail.** A batch of a known size
  then moves the image once; blocks never move, so a shrink cannot pack.
  Source: `doc/images.adoc` (libs/document).
- **`compact()` rebuilds an image in document order, ids renumbered, blocks
  one after another with no hole, so equal content gives byte-identical
  images and a compact image compacts to itself.** The owner asked that an
  image on the wire carry no waste and that equal documents have equal bytes,
  which can then be compared, hashed, stored and sealed by their bytes.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **`compact()` builds the new image before it replaces the old one, not in
  place.** That gives the strong guarantee; an in-place compaction would move
  blocks that handles point into.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **`compact_size()` is exact and allocates nothing, its block sizes taken
  from `rbtree_best_fit`'s arithmetic and a scratch heap on the stack, and
  `compact_into(buffer)` writes the compact image into a caller's buffer.**
  A program sizes a block with it; the figure follows the build's Boost
  release with no constant repeated, and `compact_into` serves documents
  that cannot reallocate, leased or shared.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).

**Seal.**

- **An image may be sealed: the CRC-32C of the whole image (Boost.CRC's
  header-only `crc_optimal`), its own eight bytes taken as zero, at header
  offset 48 with the high bit as a marker; any later change clears it.** It
  catches bytes changed by accident (a flipped bit, a torn or stray write),
  and a cleared seal means an image is never wrongly sealed.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **Opening (`open`, `from_image`, the image reader, `shared_document`)
  checks a sealed image first, before any other header field is trusted,
  unless `verify::none`; `async_write_image` seals before it writes.** A
  change anywhere, magic and size included, then reads as
  `errc::damaged_document`; `verify::none` is for a program that trusts its
  memory, as a check costs a pass over the image (about 600 MB/s).
  Source: `include/webcpp/document/image.hpp` (libs/document).
- **A seal is a checksum, not authentication.** Anyone can compute a valid
  seal for any bytes, so an image stays trusted input.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).

**Parsing and selectors.**

- **`parse` is `JSON.parse` over RFC 8259 and accepts nothing more: no
  comments, no trailing commas, whitespace of space, tab, LF and CR, and
  anything but whitespace after the text is `errc::syntax`.** It accepts what
  `JSON.parse` accepts.
  Source: `doc/parsing.adoc` (libs/document).
- **`basic_parser` takes text in chunks split anywhere, feeds Boost.JSON's
  `write_some` with numbers left as text, and builds the document node by
  node by ids.** The document may grow and move between chunks, numbers stay
  exact, and any split gives the document a whole parse gives.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **A parser that failed, by its text, a limit or a callback's failure or
  exception, stays failed, and a finished one takes no more text, until
  `reset()`.** (reason not recorded)
  Source: `include/webcpp/document/parser.hpp` (libs/document).
- **`parse_options` defaults to `max_depth` 256, `max_bytes` unlimited and
  counting every chunk, and `max_document_bytes` 64 MiB, where a document
  from `create` keeps 1 GiB.** A value takes at least 192 bytes of image: 2 MB
  of `[0,0,...]` made 268 MB, and under 1 GiB 11 MB of such text took 9 s and
  2.2 GB before failing where 64 MiB fails 700 KB in 0.4 s (the depth of 256
  is a ruling, reason not recorded).
  Source: `include/webcpp/document/parse.hpp` (libs/document).
- **Selectors are a subset of JSONPath: `$`, `.name`, `['name']`, `[n]`,
  `[*]` and `.*`; other text is `errc::type`.** A ruling of the owner;
  JSONPath beyond it is a non-goal (reason not recorded).
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **`on` delivers each matching value when it completes and erases it, its
  blocks zeroed and freed; `observe` delivers it and keeps it; a callback
  gets a `const_value` and the path in the text.** A stream of a million
  elements then needs the memory of one, and a callback cannot change the
  document being built: it copies what it keeps.
  Source: `doc/selectors.adoc` (libs/document).
- **`path_view::to_string()` escapes only `'` and `\`, so it is not RFC
  9535's normalized path.** Those are the two escapes the selectors read, so
  the text parses back as the selector of that place.
  Source: `include/webcpp/document/parser.hpp` (libs/document).

**Writing JSON.**

- **`stringify` returns a `std::string` of `JSON.stringify(v, null,
  indent)`'s bytes, the indent clamped to 10, and no text for `undefined`.**
  `JSON.stringify` clamps its third argument to 10 and returns no text for
  `undefined`.
  Source: `include/webcpp/document/detail/writer.hpp` (libs/document).
- **`stringify_to` writes the same bytes to a sink, a callable returning
  `void` or `result<void>` used in place and never copied, in pieces of at
  most `chunk` bytes (4096 by default), every piece but the last full.** A
  sink's failure stops the writing and is returned unchanged (the 4096
  default is a ruling, reason not recorded).
  Source: `include/webcpp/document/stringify.hpp` (libs/document).
- **The writer walks with a stack of its own that holds node ids, never
  recursion and never nodes.** A document's depth is then bounded by memory,
  not by the thread's stack, and nothing the walk holds is stale when the
  image moves.
  Source: `include/webcpp/document/detail/writer.hpp` (libs/document).

**Streams of documents and Boost.Asio.**

- **`basic_stream_parser` is sans-IO, with three framings, `concatenated`,
  `lines` (NDJSON) and `rfc7464`, each document parsed as `parse` parses one
  text, under the same limits per document, taken by `next()` or pushed to a
  callback.** Sans-IO serves any source: a socket read by hand, a file, an
  actor's messages, JavaScript through Emscripten.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **A bad document either stops the stream until `reset()` or, with `skip`,
  is dropped and its error delivered in its place; `skip` with
  `concatenated` makes every call return `invalid_argument`.** Concatenated
  JSON has no place to go on from after a bad document.
  Source: `doc/streams.adoc` (libs/document).
- **With `lines`, a document ends at its line's raw `\n`, and a line with two
  documents or an incomplete one is an error for that line.** JSON never holds
  a raw `\n` inside a string, so a raw `\n` always ends a line; indented JSON
  is therefore not NDJSON.
  Source: `doc/streams.adoc` (libs/document).
- **`asio.hpp` is the one header that includes Boost.Asio; the convenience
  header leaves it out, and every program that includes it is declared
  `native`.** Boost.Asio builds natively only.
  Source: `AGENTS.md` (libs/document).
- **`async_read_document` takes both of Asio's dynamic-buffer models,
  `DynamicBuffer_v1` by reference and `DynamicBuffer_v2` by value, reads a
  buffer of several pieces without joining them, and asks reads of 512 to
  65,536 bytes.** A body Beast left in a `multi_buffer` after a header is
  read with no byte lost, and reads are sized as Boost.Asio's own.
  Source: `include/webcpp/document/asio.hpp` (libs/document).
- **No completion is called inside its initiating function (it goes through
  `async_immediate` or a post), and a stream that ended completes with `eof`
  without another read.** Linux's epoll reactor does not retry a read that
  returned no bytes, so a second read of an ended socket never completed.
  Source: `specs/document/spec.md` (proposals).
- **`async_write_images` seals every document first, then writes all the
  images in one gather write with no copy.** (reason not recorded)
  Source: `doc/wire.adoc` (libs/document).

**Images on the wire.**

- **An image frames itself, its header stating its size, so a stream of
  images needs no separator and no length prefix.** A reader with the first
  64 bytes of an image knows where it ends.
  Source: `doc/wire.adoc` (libs/document).
- **`image_pool` allocates its blocks once, in one allocation; each image is
  read straight into a leased block and opened there as a `leased_document`,
  and a block given back is zeroed.** The receiver parses nothing and
  allocates nothing per message, and the next holder finds none of the last
  one's bytes.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **A full pool is `errc::capacity`, and the stream is not read until a
  block comes back.** That is the backpressure of a stream of images.
  Source: `doc/wire.adoc` (libs/document).
- **`basic_image_reader` is sans-IO with `prepare` and `commit`, checks a
  header before it takes another byte, and stops at its first refusal until
  `reset()`.** `prepare` and `commit` let a socket read straight into the
  block, and a stream that lost its framing cannot find the next header.
  Source: `specs/document/spec.md` (proposals).
- **An `image_pool` is not thread-safe.** (reason not recorded; a
  `shared_pool` is the pool for threads and processes)
  Source: `include/webcpp/document/image_stream.hpp` (libs/document).

**Sharing.**

- **`shared_pool` formats memory the program gives, the process heap or a
  shared-memory segment, as a 64-byte header and blocks of a 64-byte header
  and one image, and works through offsets from the region's start, never
  addresses.** Every process that maps the region, at whatever address, then
  uses the same pool.
  Source: `include/webcpp/document/shared.hpp` (libs/document).
- **A block's reference count is a lock-free `std::atomic<std::uint64_t>`
  in the block's header, never in the image, and the free blocks are a
  lock-free stack whose top carries a 32-bit tag against ABA;
  `is_always_lock_free` is asserted.** A lock-free atomic is address-free, no
  lock is held so no thread or process waits and a dying one leaves none
  behind, and the image stays the document's content: a count inside it
  would mean nothing elsewhere.
  Source: `include/webcpp/document/shared.hpp` (libs/document).
- **`shared_document` is `std::shared_ptr` for a document, and
  `make_mutable()` copies the image into a block of its own when others hold
  it.** Readers share one image in place, and a writer changes its copy while
  the others keep the document as it was.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **The library does not synchronise writers.** The owner's ruling: the
  process controls the memory.
  Source: `changes/archive/2026-10-12-genesis/reference/spec.md` (proposals).
- **A reference crosses processes by offset, `release` and `adopt`, and
  `shared_pool::drop(offset)` gives one back without opening the image.** A
  broken header then cannot make a block leak.
  Source: `specs/document/spec.md` (proposals).
- **A process that dies holding references leaves their blocks counted for
  good; recovering them is the program's.** Boost.Interprocess's own
  `shared_ptr` does the same, and recovery is left out of 0.1.0.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).

**Containers.**

- **`as_object()` and `as_array()` give views with the standard containers'
  interface, map-like and vector-like, two words that own nothing, ranges
  that are views and borrowed ranges, whose iterators hold ids.** A C++
  program walks a document with the standard algorithms, and since ids are
  never reused an insertion and any growth keep every iterator valid.
  Source: `include/webcpp/document/containers.hpp` (libs/document).
- **The views' iterators are bidirectional, not random access.** An index
  costs O(log n), since the tree finds an element by its index.
  Source: `specs/document/spec.md` (proposals).
- **`by_key()` gives an object's members in UTF-16 code-unit order with
  `lower_bound`, `upper_bound` and `equal_range`; a prefix query is two
  bounds.** The `by_key` index is already ordered, so nothing is sorted,
  where a JavaScript program sorts every key at each query.
  Source: `doc/containers.adoc` (libs/document).

**The JavaScript module and the npm package.**

- **The JavaScript API is the library compiled with Emscripten and embind,
  `js/binding.cpp` over `default_traits`, wrapped by `js/webcpp-document.mjs`
  in `Document` and `StreamParser`.** A JavaScript program keeps its values
  and errors and gets what JavaScript lacks: exact numbers (`getDecimal`),
  ordered-key queries (`keysInRange`), images, memory it frees.
  Source: `doc/js-module.adoc` (libs/document).
- **Nothing throws into JavaScript: each native call returns an `Outcome`
  (code, message, value), and the wrapper throws `SyntaxError`, `RangeError`
  for `range` and `capacity`, or `TypeError` for `type` and the image
  errors.** Each is the error JavaScript throws for the same fault, and the
  binding uses no sugar, so the module builds and passes without exceptions.
  Source: `js/binding.hpp` (libs/document).
- **A `Document` is addressed by paths, arrays of string keys and
  non-negative integer indices, and `get` returns a container as a copy.**
  (reason not recorded)
  Source: `doc/js-module.adoc` (libs/document).
- **`free()` releases a document at once, and a `FinalizationRegistry`
  releases one never freed.** The collector runs when the engine chooses, not
  when the program needs the memory, so the registry is a safety net and not
  a plan.
  Source: `doc/js-module.adoc` (libs/document).
- **An image goes to a Worker as a copy, `image().slice()`, in a transferred
  `ArrayBuffer`, and is opened there with one memory copy and no parse.** A
  view of WebAssembly memory cannot be transferred, and a transfer moves in
  constant time where `postMessage` of an object clones every node.
  Source: `doc/js-module.adoc` (libs/document).
- **A second module, `js/shared/`, runs on one `SharedArrayBuffer` with Wasm
  Workers on pthreads, a fixed 128 MiB memory, 64 blocks of 512 KiB, 32
  Worker slots and 1,024 tokens; a document crosses as a token adopted
  once.** Growing a shared memory would make every Worker refresh its views,
  and with fixed memory a full heap is an error, not an abort; Wasm Workers
  alone cannot link C++ exceptions in Emscripten 6.0.11.
  Source: `js/shared/Jamfile` (libs/document).
- **`join.js` reads internals of Emscripten 6.0.11 (`createWasm`,
  `wasmMemory`, `updateMemoryViews`), which ties the shared module to that
  version.** An accepted risk until it is checked against another version.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **`@webcpp/document` is staged from the `-O3` build with TypeScript
  declarations written by hand, has no dependencies and needs Node 22 or
  newer.** embind's own declarations describe the module the wrapper uses,
  not the wrapper; Emscripten's floor is Node 18.3, but the tests use
  `import.meta.resolve` and `node:test`.
  Source: `.github/workflows/npm.yml` (libs/document).
- **The package is published from a tag `v<version>` by npm trusted
  publishing (OIDC, no token secret), with provenance; the first
  publication is the owner's, by hand.** A trusted publisher can be added
  only to a package that exists.
  Source: `.github/workflows/npm.yml` (libs/document).

**Targets and portability.**

- **Tests and examples run natively (Clang, GCC), on Emscripten, wasip2 and
  wasip3; a program that needs Boost.Asio, Cobalt, Beast, threads or shared
  memory is native, and the xactor example native, wasip2 and wasip3.**
  Boost.Asio builds natively only.
  Source: `test/Jamfile` (libs/document).
- **`wd::array{wd::array{1}}` is `[1]` under MSVC and Clang before 20 and
  `[[1]]` elsewhere; the Doc Comment of `array` states it and the tests nest
  through `wd::literal`.** Those compilers apply CWG 1467 to every class and
  read a one-element nested braced list as a copy; recorded as an accepted
  risk.
  Source: `include/webcpp/document/literal.hpp` (libs/document).

**Evidence.**

- **Node is the oracle: a case is a JSON text, an indent and operations,
  each value written as JSON text; `oracle.mjs` writes Node's bytes and
  return values, which are committed, and `oracle_cases` must reproduce them
  under both traits.** The behaviour is proven by running the same cases in
  Node and in C++ and comparing bytes, and values as text keep either side
  from reading a number through a double.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).
- **54 cases are written by hand and 200 generated from a fixed seed; a case
  of numbers a double does not keep runs under `js_traits` alone, and the
  script refuses a case that holds a difference made on purpose.** Those
  differences are listed in the page's table instead, since the two sides
  differ by design.
  Source: `specs/document/spec.md` (proposals).
- **Compound changes are tested in external buffers of every size, and every
  text split at every byte and one byte at a time.** A review named these,
  with growth, look-alike keys and limits, as its focus areas.
  Source: `specs/document/spec.md` (proposals).
- **Allocation-freedom is proven by tests that replace the global
  `operator new` and `operator delete`, aligned forms included, and count
  calls: reading a stream of images and every shared-pool operation make
  none.** The replacements are the test program's instrument, the one place
  a mutable global is allowed.
  Source: `specs/document/spec.md` (proposals).
- **Each example's `.expected` is what Node prints for the same JavaScript,
  checked against Node before it is committed, and `document_image.expected`
  holds a digest of an image.** The page's output is then JavaScript's, and
  the digest, the same on every target, proves the image's bytes are.
  Source: `AGENTS.md` (libs/document).

**Benchmarks.**

- **Seventeen operations are timed three ways, Boost.JSON, document and V8's
  objects, the C++ natively and as WebAssembly in the same Node and Chrome:
  fourteen from recognised suites over nativejson-benchmark's files and
  seeded synthetic objects, and three of binary transmission.** Each row
  compares the three where a web application runs them; operations and data
  never come from document's own tests, and binary transmission, which no
  suite times, is the case the image is built for.
  Source: `doc/benchmarks.adoc` (libs/document).
- **Every implementation must give the same answer before it is timed,
  loading is never timed, the first pass is timed alone, and a result is the
  median of 10 samples of at least 10 ms.** A run whose implementations
  disagree writes no results, and the first pass shows the code before the
  JIT or the caches have seen it.
  Source: `doc/benchmarks.adoc` (libs/document).
- **The sizes on the wire, raw, gzip and brotli, of the text and of the image
  are measured apart, and the page says compressed JSON is the smaller
  payload on a network.** Honesty about cost is a goal.
  Source: `changes/archive/2026-10-12-genesis/design.md` (proposals).

### trystero (serverless WebRTC rooms; port of Trystero 0.26.0; implemented in 0.1.0)

trystero ports Trystero 0.26.0: the peers of a room meet over public Nostr
relays and talk over WebRTC data channels, on Trystero's wire byte for byte,
natively over libdatachannel and in the browser over the page's own WebRTC.
The decisions are those of the tree webcpp 0.1.0 pins (trystero `77fed8f`),
which includes the change `shared-connection` (trystero `2e2dcd4`, webcpp
`1a0dcac` and `d9e5fa6`, 2026-10-14) although its proposal's status is still
`implementing`.

**Scope and differences from Trystero JS.**

- **Only what rooms of messages need is ported: the Nostr strategy, the
  signaling, the password handshake, the action wire and the room API; audio
  and video are not, and a room ignores a frame of `@_stream`, `@_track` or
  `@_signal`.** Media is the largest part left: in the browser the page's
  `MediaStream` objects, natively libdatachannel's RTP tracks, whose capture,
  encoding and decoding would be the application's.
  Source: `doc/differences.adoc` (libs/trystero).
- **Nostr is the one strategy; BitTorrent, MQTT, Firebase, Supabase, IPFS and
  the WebSocket relay are not ported.** Each is a strategy of its own to port,
  and Nostr's public relays need no account and no server of the application's
  own.
  Source: `doc/differences.adoc` (libs/trystero).
- **Request actions, `onReceive`, the progress callbacks, cancelling with an
  `AbortSignal`, `onPeerHandshake` and passive rooms are left for later.**
  Meanwhile a room accepts every transfer for an action with a receiver within
  `max_receive_bytes`, as Trystero does when no `onReceive` is set, reports a
  transfer only when it completes, and always announces itself.
  Source: `doc/differences.adoc` (libs/trystero).
- **`trickleIce: false`, `turnConfig` and `rtcPolyfill` are not ported.**
  Candidates are always trickled, as Trystero's Nostr strategy does by
  default; `ice_servers` names every STUN and TURN server; the backend is what
  a polyfill would be.
  Source: `doc/differences.adoc` (libs/trystero).
- **The relay controls (`getRelaySockets`, pausing and resuming reconnection,
  `manualReconnection`, `warnOnRelayFailure`) are not ported.** A room
  reconnects with its own backoff and reports what its relays do through its
  log.
  Source: `doc/differences.adoc` (libs/trystero).
- **`on_peer_join` does not replay the peers already joined.** A join is
  reported from the executor after `join` has returned, so a handler set right
  after `join` sees every join; `peers()` lists the joined peers for a handler
  set later.
  Source: `doc/differences.adoc` (libs/trystero).
- **`peers()` lists ids, not connections.** A connection is the backend's
  object, a page's or libdatachannel's, and the room does not hand it out.
  Source: `doc/differences.adoc` (libs/trystero).
- **A send to a peer that has not joined completes with `error::no_such_peer`,
  where Trystero warns and resolves.** An operation that fails says so with
  its error code.
  Source: `doc/differences.adoc` (libs/trystero).

**Protocol and wire compatibility.**

- **The wire is Trystero 0.26.0's, byte for byte, and never changes: the topic
  prefixes `"Trystero@"` and `"Trystero:"`, the handshake's `"__trystero_pw"`,
  every constant and layout under `protocol/`, the default relays in their
  order, and the names embind exports.** A room joined from C++ and the same
  room joined from Trystero JS must be one room; the `oracle` lane fails when
  Trystero's own values differ from the committed ones.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **A rename never replaces the word `trystero` by pattern: it uses the exact
  C++ patterns (`namespace webcpp::trystero`, `<webcpp/trystero/`,
  `WEBCPP_TRYSTERO_`) and changes a string literal only when it is not on the
  wire.** The wire's strings must survive any rename; the error category
  `webcpp.trystero` and the examples' app ids are the literals that were
  renamed.
  Source: `AGENTS.md` (libs/trystero).
- **The protocol layer is pure functions over bytes and text, with no I/O and
  no platform, and takes every source of randomness as a parameter.** It
  builds on both targets, and its tests compare its values, byte for byte,
  with those Trystero JS computed from the same inputs.
  Source: `doc/protocol.adoc` (libs/trystero).
- **Every text the protocol writes is appended field by field into one
  reserved buffer, escaped by Boost.JSON's serializer, never built as a
  `boost::json::value`, and a Nostr event's id is hashed over those same
  bytes.** The texts are fixed shapes of a few strings and numbers, the
  escaping equals what `JSON.stringify` writes, and hashing the written bytes
  keeps the id and the text from ever disagreeing.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The relay shuffle computes `sin(seed++) * 10000` with `std::sin`.** Every
  peer of an app, Trystero JS included, must pick the same relays; this relies
  on `std::sin` and V8's `Math.sin` agreeing on these inputs, which the
  oracle's fixtures check.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **`str_to_num` sums the UTF-16 code units of a text, as JavaScript's
  `charCodeAt` sees them.** The Nostr kind and the relay shuffle's seed derive
  from that sum, so a non-ASCII id must count as in JavaScript.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Of two peers, the lower id offers, on the relay that carried the other's
  announcement.** Trystero answers only on the relay that delivered the
  message; the spike's probe, which published every candidate on every relay,
  was rate-limited by one.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/reference/research/TRYSTERO_CPP_SPIKE/FINDINGS.md`
  (proposals).
- **A sealed signal carries a 16-byte IV, not the usual 12.** It is the IV
  Trystero draws (`protocol::seal_iv_size`), so that each side opens what the
  other sealed.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).

**Layers and the backend interface.**

- **The library is layered (protocol, backend interface, app, room, and the
  browser and native backends), and the core above the interface never
  includes a backend, libdatachannel, OpenSSL or Emscripten.** The same room
  runs over the browser's APIs, over libdatachannel and over the tests' memory
  relays, on both targets; `meta/include-boundaries.json` states the boundary
  and the lint fails an include that crosses it.
  Source: `doc/design.adoc` (libs/trystero).
- **The backend is a handful of abstract classes, not template parameters.**
  Each operation is a call into JavaScript or a network stack, so a virtual
  call costs nothing measurable, while the room stays free of templates over
  the platform and of `#ifdef`, and the test and browser backends are
  interchangeable at run time.
  Source: `doc/design.adoc` (libs/trystero).
- **Every handler a backend invokes, and every completion of the library, runs
  on the executor after the call that caused it has returned, even when its
  result is known at once.** The room calls these operations from inside
  handlers whose state a completion run there would change under them.
  Source: `doc/design.adoc` (libs/trystero).
- **Text crosses the backend interface as views valid for the call; a data
  channel message is the one value handed over, as `backend::bytes`.** The
  receiver keeps the frame, and taking ownership of it is what spares a copy.
  Source: `include/webcpp/trystero/backend.hpp` (libs/trystero).
- **The backend factory gives a room its randomness and its clocks, and its
  `random` fills from a cryptographically secure source.** The bytes become
  secret keys and nonces.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **ICE servers are checked in the core: an unusable URL, or a TURN server
  without credentials, is dropped when the app is made and reported, and every
  scheme is lowercased.** WebRTC would refuse them; dropped at once, the app
  goes on with the others on every backend.
  Source: `include/webcpp/trystero/options.hpp` (libs/trystero).

**The browser backend.**

- **In the browser the backend binds the page's own `RTCPeerConnection`,
  `WebSocket`, `setTimeout` and `crypto.subtle` through `emscripten::val`, not
  libdatachannel's WebAssembly port.** datachannel-wasm has no media and hides
  the `RTCPeerConnection` it wraps; the direct binding is smaller, reports
  every error, and leaves the browser's whole WebRTC stack within reach for
  media.
  Source: `doc/design.adoc` (libs/trystero).
- **`browser::event_loop` drives the `io_context` without ever calling
  `run()`: each JavaScript event runs its handler and then `poll()`, and work
  posted outside an event schedules one `poll()` in a microtask.** Nothing may
  block in a page, and nothing runs while nothing happens. `poll()` from
  `emscripten_set_main_loop` was rejected for its 60 wake-ups a second and its
  stop in hidden tabs, and `poll()` on a fixed interval for its wake-ups and
  latency.
  Source: `doc/browser.adoc` (libs/trystero).
- **An event the page delivers while C++ is already running is posted, not
  run.** A handler never runs inside the call that caused it, as when node's
  WebSocket fires `close` from inside `close()`.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The browser has no threads: `/webcpp/trystero//trystero` defines
  `BOOST_ASIO_DISABLE_THREADS` on emscripten.** Emscripten's threads need a
  cross-origin isolated page.
  Source: `doc/browser.adoc` (libs/trystero).
- **JavaScript reaches C++ through one exported function, `trystero_dispatch`,
  and every object registers its callbacks with the loop by id and slot and
  removes them when it is destroyed.** A late event for a destroyed object
  finds nothing, and a destroyed loop marks every handler it made as dead.
  Source: `doc/browser.adoc` (libs/trystero).
- **No JavaScript exception enters WebAssembly: every call that can throw runs
  inside one `EM_JS` function that returns the exception as a value, and every
  entry from the page restores the stack pointer and the loop's depth when
  something escapes.** Measured before the fix, each escape left 128 bytes of
  stack and the loop stopped after about 500; a C++ exception that reached
  JavaScript kept its object for good, so `browser::dispatch` catches it
  first.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Text crosses into JavaScript and back through the loop's `TextDecoder` and
  `TextEncoder`, straight from and into WebAssembly memory, with its length
  checked before any copy.** No `std::string` is made to carry it, and a
  string over its bound is never copied into WebAssembly.
  Source: `doc/browser.adoc` (libs/trystero).
- **A data channel message that is not an `ArrayBuffer`, or is larger than any
  room frame, is dropped before it is read.** Neither can be a frame;
  measured, the text `1e300` threw a `RangeError` out of `new Uint8Array`, and
  `300000000` made a 300 MB frame.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A page makes at most `browser::backend::max_peer_connections` (256) peer
  connections across all its apps and rooms, counted, from construction until
  the garbage collector frees them, by a census every program of the page
  shares.** Chromium counts a connection against its 500 until a collection
  (measured: the 501st throws "Cannot create so many PeerConnections"), and
  the rest of its count stays free for what the page makes without a room.
  Source: `doc/browser.adoc` (libs/trystero).
- **Without `FinalizationRegistry` the census counts nothing.** The page never
  stops making connections for a count that never comes down.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A page's relay socket has no idle timeout.** A page's `WebSocket` cannot
  ping.
  Source: `doc/browser.adoc` (libs/trystero).
- **The browser backend requires a secure context.** `crypto.subtle`, which
  seals the signaling, exists only there; elsewhere every sealing operation
  fails with `error::backend_failure`.
  Source: `doc/browser.adoc` (libs/trystero).
- **The browser sealer imports its key on its first operation and queues the
  operations that arrive while it imports.** (reason not recorded)
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A browser program lives as long as its page: the examples allocate their
  application with `new`, and `main` returns at once.** The event loop must
  outlive every room and object it serves.
  Source: `doc/browser.adoc` (libs/trystero).

**The native backend.**

- **Natively WebRTC is libdatachannel's, the relays' WebSockets are
  Boost.Beast's, and TLS and AES-GCM are OpenSSL's, with libdatachannel built
  without its own WebSocket.** The relays then run on the room's executor like
  everything else.
  Source: `doc/native.adoc` (libs/trystero).
- **The sealer is asynchronous on both targets: the native one works
  synchronously and posts its completion.** Web Crypto is asynchronous, so the
  interface is, and a completion never runs inside the call that started it.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/reference/research/TRYSTERO_CPP_SPIKE/FINDINGS.md`
  (proposals).
- **Every event of libdatachannel copies what it carries and is posted to the
  executor through a `std::weak_ptr`; no handler runs on libdatachannel's
  threads.** Its callbacks run on its own threads under a recursive mutex, and
  an event that arrives after its object is gone must do nothing; an
  application with several threads gives the app a strand.
  Source: `doc/native.adoc` (libs/trystero).
- **The backend holds every object libdatachannel hands out.** libdatachannel
  keeps only weak references; the spike lost tracks and WebSockets it did not
  retain.
  Source: `doc/native.adoc` (libs/trystero).
- **libdatachannel's automatic negotiation is off.** An offer or an answer
  exists only when the signaling asks for one, as in the browser.
  Source: `doc/native.adoc` (libs/trystero).
- **A local candidate leaves as the JSON of an `RTCIceCandidateInit`, and a
  TURN URL with credentials is rewritten as `turn:user:credential@host`.** The
  first is the form Trystero JS adds with `addIceCandidate`, the second the
  form libdatachannel parses.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The native data channel drops a text message, or a binary one larger than
  any room frame, on libdatachannel's thread, before it is posted.** Neither
  can be a frame.
  Source: `doc/native.adoc` (libs/trystero).
- **The native timer numbers its waits, so that a wait cancelled, restarted or
  destroyed never fires.** Asio may already have queued its expiry as a
  success.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The native WebSocket writes one text at a time, and a connection that ends
  during a write clears its queue while the write finishes on the frame it
  moved out of it.** Beast allows one write at a time, and the write's
  completion must never touch the cleared container.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Relays are verified against the system's certificate authorities, and
  `tls_context()` hands the TLS context to an application with authorities of
  its own.** A relay whose certificate does not verify is unreachable, and the
  room retries it later as it would any other.
  Source: `doc/native.adoc` (libs/trystero).
- **A native relay that stops answering is dropped once
  `websocket_limits::idle_timeout` (30 s) passes with its keep-alive ping,
  sent halfway, unanswered.** Signaling texts are 2 to 4 KB, so a real room
  meets the cut only with a relay slower than about 200 B/s.
  Source: `doc/native.adoc` (libs/trystero).
- **`backend_config::bind_address` binds every peer connection to one address;
  empty means every interface but the loopback.** Bound to 127.0.0.1, the
  peers of one machine connect with no other network, which a test wants.
  Source: `doc/native.adoc` (libs/trystero).
- **`backend_config::max_peer_connections_per_minute` (256) bounds the
  connections a native backend makes for every app and room it serves.** A
  room makes at most 64 in any 57 s, so the default lets four rooms do so at
  once.
  Source: `doc/native.adoc` (libs/trystero).
- **`native::backend::random` aborts the program when OpenSSL has no secure
  randomness.** Without secure randomness nothing is safe.
  Source: `include/webcpp/trystero/native/backend.hpp` (libs/trystero).
- **The constructor of `native::backend` is the one native function that
  throws, a `boost::system::system_error` when OpenSSL cannot make the TLS
  context or load the system's certificate authorities.** (reason not
  recorded)
  Source: `doc/native.adoc` (libs/trystero).
- **`native::data_channel`, `native::peer_connection` and `native::backend`
  exist only with exceptions, and every program, test and headers-alone call
  that uses them declares `<exception-handling>on` itself.** libdatachannel
  reports errors by throwing, and b2 ignores a non-free feature among usage
  requirements, so `/webcpp/trystero//native` cannot declare it for them.
  Source: `AGENTS.md` (libs/trystero).
- **The native timer, sealer and WebSockets are compiled alone in a
  headers-alone call of their own, without exceptions.** The lint then
  analyses them without exceptions too.
  Source: `AGENTS.md` (libs/trystero).

**Rooms, actions and errors.**

- **Every asynchronous operation (`async_send`, `async_receive`, `async_ping`,
  `async_leave`) is an Asio initiating function.** The spike made Boost's
  style a principle: Asio as the execution model, and a completion token on
  every asynchronous operation.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/reference/research/TRYSTERO_CPP_SPIKE/FINDINGS.md`
  (proposals).
- **An asynchronous operation owns its arguments: a value is taken by value, a
  view copied once at the call, a `boost::json::value` serialized at once, and
  a peer id copied.** A token such as `boost::asio::deferred` starts the
  operation after the call.
  Source: `doc/usage.adoc` (libs/trystero).
- **Text is a view and a copy is a decision: digests are fed the parts of
  their input, signals and relay messages are read as views into the parsed
  document, a frame is written once inside its room envelope and shared by
  every target, and a log sentence is composed only when there is a sink.** A
  string is made only where something must own the text: a map key, a member,
  a capture of a handler that runs later.
  Source: `doc/design.adoc` (libs/trystero).
- **A `room` is a move-only handle of one membership: destroying it, or
  assigning another room over it, leaves at once, without the 99 ms
  farewell.** Two handles to one membership would make leaving depend on
  another handle.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **An action's type sets Trystero's format and how a message arrives: owning
  types receive a copy or the moved buffer, while `std::string_view` and
  `std::span<const std::uint8_t>` read the frame in place during the
  handler.** A received frame becomes the buffer of the message delivered from
  it, so a view handler reads it without a copy.
  Source: `doc/design.adoc` (libs/trystero).
- **`async_receive` refuses a view type at compile time.** A received view
  would outlive the message.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Actions of different types may share a name, which has one receiver
  slot.** One may receive as views while another sends by moving strings, as
  the chat example does.
  Source: `doc/usage.adoc` (libs/trystero).
- **Frames of an action not made yet are kept, 64 per peer for 120 s, and
  messages without a handler queue up to 1024 per action, the oldest dropped
  and logged.** An action made late still receives what arrived before it; for
  the numbers, (reason not recorded).
  Source: `doc/usage.adoc` (libs/trystero).
- **`async_send` completes once each target's local data channel has taken the
  inline frame, or the last chunk of an accepted transfer, with the first
  failure if any.** That is when Trystero's `send` resolves; nothing confirms
  that the receiver has the message.
  Source: `doc/usage.adoc` (libs/trystero).
- **A frame or a chunk goes to the data channel only while it buffers at most
  64 KiB, and a send that waits longer than 10 s completes with
  `error::channel_stalled`.** The threshold is Trystero's
  `bufferedAmountLowThreshold`; for the 10 s bound, (reason not recorded).
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A receiver takes one transfer per peer at a time, only for an action with
  a receiver, and reserves at most `max_receive_bytes` (256 MiB) across
  peers.** The bound is Trystero's `maxReceiveBytes`; for one transfer per
  peer, (reason not recorded).
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A peer that breaks the action wire (a frame that does not decode, a
  duplicate offer, a chunk at the wrong offset) is kicked.** Trystero drops
  the peer for any wire error; over a shared connection the kick ends that
  room's binding only.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A peer still in its handshake may send only `@_leave`, `@_hsdata` and
  `@_hsready`; its other frames are dropped.** (reason not recorded)
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **`error::handshake_failed` is reserved and reported by nothing; a handshake
  message of the wrong shape reports `error::wrong_password`.** Trystero's
  handshake reports it so, and the value 11 is kept so that the values after
  it do not move.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The error category's `message(int, char*, std::size_t)` returns a static
  text and allocates nothing.** A room logs an error without allocating.
  Source: `include/webcpp/trystero/error.hpp` (libs/trystero).
- **Diagnostics go to one sink, `app_config::log`, with a category per part
  (`app`, `nostr`, `signal`, `webrtc`, `room`, `action`), and a sentence is
  composed from its parts only when there is a sink.** An app without a sink
  composes no text at all.
  Source: `include/webcpp/trystero/detail/log.hpp` (libs/trystero).

**The app and the shared connection, as built.**

- **Rooms are joined through an `app`: `app::join` returns
  `boost::system::result<room>`, and the standalone `room` constructor and
  `room_config` are removed.** One way to join, so no program falls back into
  one id per room, where two standalone rooms with one id meet a Trystero JS
  peer in one of them only; the standalone form can return later as an app of
  one room without breaking anything.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **Two peers in several rooms of one app share one WebRTC connection, each
  frame naming its room by its token.** Trystero does so, and the owner
  decided on 2026-10-11 that the difference goes away.
  Source: `libraries/trystero/openspec/changes/shared-connection/proposal.md`
  (proposals).
- **Sharing is scoped by the `app` object the user makes: nothing is keyed by
  the backend or kept in static storage, and two apps of one app id in one
  process are two peers.** No static state, and several peers in one process,
  as the tests and the interop peers need; an id across apps is a program's
  choice.
  Source: `doc/differences.adoc` (libs/trystero).
- **The app owns the connections and a room its bindings: a room holds the app
  strongly, the app reaches rooms weakly, a binding holds its connection
  weakly, and a connection holds plain pointers to its bindings, which remove
  themselves before they go.** No cycle keeps the objects alive, and after a
  room has ended the app keeps no allocation of that room's resource.
  Source: `doc/design.adoc` (libs/trystero).
- **One self id per app, drawn once unless named, announces, signals,
  tie-breaks and handshakes in every room of the app.** Sharing a connection
  needs one id per app, as Trystero's one `selfId` per page; the program
  decides which rooms may be linked, and rooms that must stay unlinkable use
  separate apps.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **The password is the app's and immutable: `app_config::password` is copied
  into the app's resource, and `room_options` has no password.** The initial
  signaling sealed with it authenticates the domain before any connection is
  adopted, so a peer that reaches another room through presence already knows
  its password (owner amendment, 2026-10-14).
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/password-domains.md`
  (proposals).
- **Different passwords need separate apps with distinct self ids, whose
  connections, registries, presence and tokens are never shared.** A room
  token does not depend on the password, so isolation comes from scoped
  connections and registries, not from a new token format.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/password-domains.md`
  (proposals).
- **The room handshake stays Trystero's one-way handshake; no reciprocal room
  proof is added.** Reciprocal proofs would change compatibility with Trystero
  JS; a JS page that mixes passwords keeps its own exposure, which the page
  documents rather than repairs.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **A passwordless app's rooms are open, by token, to the peers connected
  through any of its rooms; no proof gate is added.** A gate by a message on
  the room's topics is passed by anyone who reads the relays, so it gave false
  assurance, and it cost up to 10 s against a Trystero JS peer that joined
  first; a room that must be private belongs to an app with a password.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **Adoption follows Trystero: the first connection registered wins, a second
  one to the same peer is closed and the room bound to the first, a stale one
  is ended before its replacement, and crossed connections are both lost and
  met anew.** Matching Trystero's rule keeps a C++ and a Trystero JS side
  agreeing; ending the stale one first fixes the plan's silent replacement, in
  which a late close erased the new entry.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **A room binds a connected peer only after that peer advertised the room's
  token itself on that connection.** An announcement or a signal forged in its
  name must not bind an innocent peer; in Trystero JS it binds the peer by
  reuse, which is kicked after its handshake timeout.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/specs/trystero/spec.md`
  (proposals).
- **A bound room keeps its negotiation entry marked connected, and binds by
  signaling reuse only on its first meeting with that peer over that
  connection.** Otherwise a room bound by presence or reuse would offer to the
  peer it is bound to, and later relay signals could shorten a backoff.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **A room that binds by reuse or at its rebinding time sends `presence(1, T)`
  on the connection.** It makes the other side, Trystero JS included, bind at
  once, with no answer on the relays.
  Source: `doc/differences.adoc` (libs/trystero).
- **A frame for a room of the app not bound yet is kept, 64 per connection,
  all dropped 10 s after the first, and a kept `@_leave` discards itself and
  what was kept before it.** A registered room may receive its handshake
  before signaling binds it, as Trystero's source notes; the frames before a
  kept `@_leave` belong to a binding of the other side that has ended.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **A bound room has no buffer for frames that wait for its handler, where
  Trystero keeps 64 for 10 s and then drops the connection.** A binding's
  delivery is set in the same call that makes it.
  Source: `doc/differences.adoc` (libs/trystero).
- **A frame for a token no room of the app has is dropped and the connection
  stays; a frame that does not decode, or a 65th advertised token, ends the
  connection for every room.** Presence and routing are Trystero's.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **A kick, a failed handshake or a peer's `@_leave` ends that room's binding
  only, and the other rooms keep the connection.** Each room's membership,
  handshake, actions, queues and bounds stay its own, so a failure in one room
  costs no other.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **After a binding ends over a live connection, the room rebinds one step
  later: the larger of `handshake_timeout` and 10 s, plus 1 s (11 s by
  default), doubling per further end to 5 minutes, until a completed handshake
  or the connection's end.** Rebinding on the next signal, as Trystero does,
  loops kicks, reports and announcements without end on a wrong proof; the
  added second lets a Trystero JS peer drop the frames it kept from the ended
  binding (10 s) before the next one.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **Meanwhile only the lower id refuses the peer and keeps its frames, while
  the higher id refuses nothing but drops the peer's frames of that room until
  it next sends presence.** Two honest peers cannot alternate kicks, and a
  stale frame of an ended binding is never answered in the next one; a wrong
  proof costs at most six reports in ten minutes, on the lower id only.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **The initiator accepts a correct answer to any of the last 8 challenges it
  sent the peer since their last completed handshake.** A response carries no
  challenge id, and each challenge is the initiator's own fresh nonce on a
  channel only the peer can write to, so any of them proves the password; a
  late answer of an ended binding no longer reports a false error.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **Announcements restart after a peer leaves only when no live connection to
  it remains.** Over a live connection every rebinding goes through the
  connection.
  Source: `doc/differences.adoc` (libs/trystero).
- **A connection with no binding left stays idle for
  `app_config::idle_timeout` (123,333 ms; 0 closes it at once) after a
  departure, and closes at once after a kick.** The idle time is Trystero's
  `sharedPeerIdleMs`, made a member for tests and programs that close sooner;
  closing after a kick is a bound: a failed peer holds no connection.
  Source: `doc/differences.adoc` (libs/trystero).
- **`join` of a room already joined in the app fails with
  `error::already_joined`, and joining an id in its 99 ms farewell finishes
  that farewell first.** A `room` is a move-only handle whose destruction
  leaves, so a second handle to one membership would make that depend on
  another; finishing the farewell sends the old room's `presence(0)` before
  the new one's `presence(1)`, as Trystero's rejoin does.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **An app holds at most `app::max_rooms` (64) rooms, a room counting until
  its `presence(0)` is sent.** A Trystero peer drops a connection whose remote
  advertises more than 64 tokens, so a 65th room would cost every connection
  of the app.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **The app handle and every room not yet ended are the users of the app's
  state: the last to go ends every connection at that call, and
  `detail::app_impl`'s destructor closes and posts nothing.** The destructor
  may run while the `io_context` is destroyed, from the last operation pending
  there, and then may only free.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **Relay sockets, subscriptions and the Nostr signing key stay per room,
  configured once for the app.** No peer sees the difference, and sharing
  sockets, batching topics and one key per app touch the strategy's bounds and
  reconnection tests, which deserve their own specification.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **The app's relays and ICE servers serve every room of it.** One
  configuration for what the rooms share, where Trystero takes the first
  room's relays and each negotiating room's RTC configuration.
  Source: `doc/differences.adoc` (libs/trystero).
- **What the app's configuration sets aside is reported to `app::on_error`
  after the constructor returns, once, to the handler current at delivery;
  `room::on_error` reports only problems with peers.** The app's constructor
  does not fail, and no handler can see a report made during construction.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **A send that must wait for the shared channel to drain waits in its
  binding, and at each drain the bindings take turns; an inline send while the
  channel buffers at most 64 KiB goes at once.** A room's chunked transfer
  then sends one chunk per drain while other rooms' waiting sends get their
  turn, and an inline flood, of pings for one, delays only traffic to the peer
  that provoked it.
  Source: `doc/protocol.adoc` (libs/trystero).
- **An app and all its rooms run on one executor, the app's, and nothing is
  locked.** The state is touched only there, so the resources need not be
  thread-safe on one thread; several threads need a strand.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **trystero runs on the Asio executor it is given, not on xactor.** How a
  library on Asio meets xactor is still to be designed, and the move onto
  webcpp::asio, webcpp::webrtc and webcpp::crypto is a change of its own,
  after this one.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **Two residual cases with a Trystero JS lower id are accepted: a reuse
  binding inside the higher id's dropping window, and a page whose timers run
  later than the 1 s margin.** In neither does the C++ side report anything
  false; the first joins by the second rebinding time (at most 43 s by
  default), the second costs one false error on the JS side, at most three
  times.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).

**Bounded signaling and security.**

- **Everything a peer or a relay can make a room hold is bounded.**
  Announcements are not sealed, and anyone who reads the relays can send any
  signal in any peer's name; Trystero JS bounds none of it.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **No signal changes a negotiation before it opens with the room key.** A
  forged answer sent in a peer's name must not reset an offer, and a forged
  offer must make no connection.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Signals that do not open are reported paced: the first of a quiet 10 s at
  once, with its peer, and the others of those 10 s in one report, with their
  number.** A flood of fresh ids makes each a negotiation.
  Source: `doc/protocol.adoc` (libs/trystero).
- **A room remembers, by SHA-256, the opened text of the last 1024 offers and
  answers it made or acted on, and refuses one that comes back to it as a
  peer's.** A sealed offer or answer holds neither its offer id nor a peer id,
  so one recorded from the relays still opens when resent, as does the room's
  own; the digest is of the opened text, whose sealed encoding a resender can
  vary.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A room holds at most 32 negotiations with peers it is not connected to: an
  idle one gives its place to a new peer, and a refused peer is logged once.**
  Anyone who knows the app and room ids can announce any number of peer ids.
  Source: `doc/protocol.adoc` (libs/trystero).
- **A room makes at most 64 connections in any 57,333 ms, counted from when
  they were made.** A place freed early by a connection that failed or closed
  at once is not taken again until the window has passed.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A room asks a peer below it to offer at most once a second per peer and
  relay, and once in 5.3 s, Trystero's announce interval, per relay.** A flood
  of low ids must not have the relays rate-limit the room.
  Source: `doc/protocol.adoc` (libs/trystero).
- **The strategy remembers the last 4096 event ids and drops a repeated one.**
  A signal reaches the room on several relays and must reach the signaling
  once.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A lost relay is retried after a random share of a bound that starts at
  3.3 s and doubles to 60 s, and comes back down only after a connection
  lasted 60 s.** A relay that closes the room at once after each open is then
  tried less and less often.
  Source: `doc/protocol.adoc` (libs/trystero).
- **A relay that answers `rate-limited:` is skipped for 60 s, doubling to
  15 minutes, and one that answers `blocked:`, `restricted:`, `auth-required:`
  or `pow:` is retired.** A public relay rate-limited the spike's probe, and
  the library must respect the backoff.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/reference/research/TRYSTERO_CPP_SPIKE/FINDINGS.md`
  (proposals).
- **A relay's text is bounded at 1 MiB, a longer one closing its connection
  before it is copied, and what the room holds unsent for a relay at 4 MiB.**
  A relay is a third party too.
  Source: `AGENTS.md` (libs/trystero).
- **A signal's fields are bounded: the peer id and the offer id at 64 bytes, a
  sealed offer or answer at 64 KiB, a sealed candidate at 16 KiB.** Trystero
  JS bounds none; the bounds refuse nothing a browser writes for a data
  channel, and a static assertion proves that every candidate Trystero accepts
  (8192 bytes of plaintext) fits sealed.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **What a flood or a watcher without the key can still do is stated on the
  page, not hidden.** Only a wire that seals the offer id and both peer ids,
  or announcements that prove the room key, closes it, a change Trystero would
  have to share (reported upstream).
  Source: `doc/protocol.adoc` (libs/trystero).

**Memory.**

- **An app's allocations go through `app_config::memory`, and a room's through
  `room_options::memory`, the app's resource when null.** The room keeps the
  resource the allocation rule gave it, while what the rooms share lives in
  the app's.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **Peer connections, data channels and the frames received on them use the
  app's resource even while a room negotiates them; relay sockets, sealers and
  room timers use the room's.** A connection belongs to the app once adopted,
  and serves every room bound to it.
  Source: `libraries/trystero/openspec/changes/shared-connection/design.md`
  (proposals).
- **A binding fails and frees its drain waiters, in the room's resource, when
  it detaches.** The waiters are the room's handlers, frames and timers; held
  by the app's connection, they would outlive a room with a resource of its
  own.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **A received binary payload moves out of its frame when the frame and the
  room share a resource, and is copied once otherwise; received text is always
  copied once.** A `std::pmr` container moved into another resource copies
  anyway; for text, (reason not recorded).
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **The log sink, and every sentence of the app and its rooms, are in the
  app's resource, the sink shared through a `std::shared_ptr`.** The sink then
  outlives a part that is still running when the app goes.
  Source: `include/webcpp/trystero/detail/log.hpp` (libs/trystero).
- **`handler<R(Args...)>` replaces `std::function` wherever trystero keeps a
  callable: move-only, inline when the target fits three pointers and is
  nothrow-movable, else in storage of its allocator.** `std::function` takes
  no allocator since C++17; the app moves `app_config::log` into its resource,
  which makes `app_config` move-only.
  Source: `doc/allocation.adoc` (libs/trystero).
- **`json_resource` adapts a `std::pmr::memory_resource` to Boost.JSON over
  Boost.Container's header-only base, and equals only itself.**
  `boost::container::pmr`'s global resources are compiled, and the CI installs
  Boost's headers alone; telling two adapters over one upstream apart would
  need RTTI, which a user may build without.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **A room makes its JSON storage once, at its construction, and parses with a
  `boost::json::parser` over it, never with `boost::json::parse`.** A value a
  handler keeps may outlive the room, and `boost::json::parse` takes its
  temporaries beyond a stack buffer from the default resource.
  Source: `doc/allocation.adoc` (libs/trystero).
- **A post goes through the `io_context`'s own executor when the room's
  executor is one.** Through `boost::asio::any_io_executor`, Asio wraps a post
  in a function object allocated with `std::allocator` (measured: two
  `std::aligned_alloc` per post in every round of five, natively and on
  emscripten).
  Source: `doc/allocation.adoc` (libs/trystero).
- **The room never binds its allocator over a user's completion handler, and
  records, when it erases the handler's type, whether its executor is the
  room's.** A completion handler is the user's, allocated as its own
  associated allocator says; telling the executor apart through the erased
  type cost two `operator new` calls per completion (measured).
  Source: `AGENTS.md` (libs/trystero).
- **`trystero::bytes` and `trystero::payload` keep `std::allocator`; every
  other container trystero declares is a `std::pmr` one.** Those two are the
  user's choice; `std::pmr::string` and `pmr_bytes` are the types that receive
  in the room's resource.
  Source: `doc/allocation.adoc` (libs/trystero).
- **libsecp256k1 signs with a context preallocated in the room's resource, and
  verifies with its static context.** No allocation of a room escapes its
  resource, and a verification allocates nothing.
  Source: `doc/allocation.adoc` (libs/trystero).
- **The allocations outside the resources are listed on the page, not claimed:
  libdatachannel's, OpenSSL's, `emscripten::val`'s, the JSON storage's
  counter, Asio's once per context and its wrapper on a foreign executor,
  Beast's stream state, and Emscripten's `select()`.** They belong to a
  dependency or to Asio, and the posts from libdatachannel's threads may not
  touch a resource used only on the executor.
  Source: `doc/allocation.adoc` (libs/trystero).
- **No check catches a resource that ends too early: it is declared before the
  `io_context`, and its lifetime is the user's.** trystero holds a raw pointer
  and cannot see the resource end, and the late use comes from an operation of
  Asio's or the browser's after the room is gone; a resource that asserts
  nothing is out when it is destroyed is the user's check.
  Source: `doc/allocation.adoc` (libs/trystero).
- **A pool suits a long-lived app or room, and a monotonic resource a
  short-lived one or a test.** A room allocates and frees on every event: a
  frame, a parsed text, a post, a log line.
  Source: `doc/allocation.adoc` (libs/trystero).
- **The browser's `event_loop` and the tests' `memory_network` allocate their
  registrations in their own resource, while each registered callback stays in
  its own.** They are shared by every room of a page or of a test.
  Source: `doc/allocation.adoc` (libs/trystero).

**Build and dependencies.**

- **libsecp256k1 0.8.0, with `schnorrsig` and `extrakeys`, signs the Nostr
  events of every room, built with `ECMULT_WINDOW_SIZE=4` and
  `ECMULT_GEN_KB=2`.** The small tables keep the browser's download small: the
  spike measured 152 KB of WebAssembly against 1.30 MB with the default
  tables.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/reference/research/TRYSTERO_CPP_SPIKE/FINDINGS.md`
  (proposals).
- **libdatachannel (MPL 2.0) and OpenSSL (Apache 2.0) are needed by the native
  backend only, through `/webcpp/trystero//native`.** A browser program never
  includes the native backend, so `/webcpp/trystero//trystero` depends on
  neither, as OpenSSL is optional for Asio.
  Source: `doc/design.adoc` (libs/trystero).
- **The external libraries are found where `-s<NAME>_ROOT` on b2's command
  line says, else in the superproject's `.local/`, else natively in
  `/usr/include` or `/usr/local/include`; the environment is never read.**
  `.local/` is where webcpp's CI installs them, until webcpp builds them from
  `third_party/`, and a variable a shell keeps from another day must never
  replace `.local/` without a word.
  Source: `doc/building.adoc` (libs/trystero).
- **A library found nowhere leaves trystero out with one message, unless
  `webcpp-require-external=on`, as the CI builds, stops the build.** `b2 test`
  then builds every other library.
  Source: `AGENTS.md` (libs/trystero).
- **The searched libraries are explicit targets.** Requested alone, at the
  superproject's root, their alternatives would be matched against a request
  without `<cxxstd>20`, and none chosen.
  Source: `build.jam` (libs/trystero).
- **embind is linked as the flag `-lembind`, not with b2's `<embind>on`.**
  `<embind>on` is not a free feature, and b2 ignores one among usage
  requirements (measured: a program then fails to link,
  `_embind_register_function` undefined).
  Source: `build.jam` (libs/trystero).
- **On MSVC the libraries take CMake's and Shining Light's names
  (`libsecp256k1` with `SECP256K1_STATIC`, `libssl`, `libcrypto`, also in
  `lib/VC/x64/MD`), and the tests and examples add `/bigobj`.** A static
  libsecp256k1 on Windows declares every function imported from a DLL without
  the define, and a test of rooms over Asio holds more sections than the
  default object format allows (C1128).
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **libdatachannel is built with the compiler and standard library of the
  programs that link it.** A program built with GCC and libstdc++ does not
  link against a libdatachannel built with libc++, though the native headers
  still compile alone.
  Source: `doc/building.adoc` (libs/trystero).

**Licensing.**

- **trystero's own code is under the Boost Software License, Trystero's MIT
  notice ships unchanged as `LICENSE-TRYSTERO.txt`, and the README names what
  derives from Trystero.** trystero ports Trystero's protocol and was written
  with Trystero's source open, so Trystero's notice comes with it.
  Source: `doc/design.adoc` (libs/trystero).

**Evidence.**

- **The `oracle` lane has Trystero 0.26.0's own functions, pinned by
  `package-lock.json`, compute every protocol value over the cases, with the
  action frames and envelopes captured from its action wire manager and
  `SharedPeerManager` over fake peers.** The wire is proven against Trystero
  itself, not against a reading of its source.
  Source:
  `libraries/trystero/openspec/changes/archive/2026-10-09-genesis/design.md`
  (proposals).
- **The oracle fixes the sealed fixture's IV by replacing
  `crypto.getRandomValues` while Trystero's `encrypt` runs.** The sealed text
  is then the same on every run.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **The `interop` lane meets the C++ peers, the browser's in headless Chrome
  and the native one as a process, with Trystero JS peers over an in-memory
  Nostr relay on 127.0.0.1 that verifies every event's id and signature.**
  Guarantee 1 is proven against Trystero JS itself, through a relay that
  checks events as a public relay does.
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **Every interop peer gathers its ICE candidates on the machine alone, and
  the driver has Chrome resolve no host but 127.0.0.1.** The lanes then need
  no network beyond the machine (measured passing under
  `docker run --network none`), and the usual flags do not stop a full Chrome
  for Testing 156 from contacting Google (measured).
  Source: `AGENTS.md` (libs/trystero).
- **Each interop driver is bounded by 2400 s.** The bound covers the
  scenarios, Chrome's start, the wrong-password observation windows and the
  transient-handshake recovery, so a scenario that does not complete fails by
  name with what each peer saw.
  Source: `test/oracle/Jamfile` (libs/trystero).
- **An interop scenario that lets two rooms negotiate at once fails, naming
  the counts, when a side makes more than four connections.** A livelock of
  crossed adoptions then fails by name, not as flakiness.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **The unit tests run whole rooms in one process over
  `test/support/memory_backend.hpp`, every event posted, and reach nothing
  beyond the loopback.** A room is tested on both targets without the network;
  guarantee 6 is held by the `gcc-14` and `clang-linux-18` lanes, run in a
  container with no network but the loopback.
  Source: `doc/guarantees.adoc` (libs/trystero).
- **The memory backend has a virtual clock, a deliver, drop or hold frame
  filter, deferred opens, injection, failure and a buffering channel, and a
  virtual run is bounded by 10^6 timers.** The sharing cases cannot be written
  without them, timing cases on a scaled wall clock flake on loaded machines,
  and a timer re-armed without delay fails a test by name instead of hanging
  it.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **The hooks' storage uses the default resource, and only when a test sets
  them.** The allocation test sets none, so its counts do not change.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **Every test case with a password fixes which side has the lower id.** Only
  the handshake's initiator reports a wrong proof.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **A test in which a peer leaves and comes back with the same id destroys the
  peer's `app` between visits, or sets `idle_timeout` to 0.** An idle
  connection kept by the peer's app would replace the new negotiation.
  Source:
  `libraries/trystero/openspec/changes/shared-connection/reference/spec.md`
  (proposals).
- **The allocation test replaces the global `operator new`, counts the C
  library's allocation functions where the platform shows them, and makes
  Asio's once-per-context allocations before it counts.** From construction to
  drain it must see only the rooms' JSON storage counters; without the warm-up
  the count is 8, not 2.
  Source: `doc/allocation.adoc` (libs/trystero).
- **On Emscripten the allocation test replaces `select()` with one that keeps
  its array on the stack.** Emscripten's `select()` callocs an array on every
  call, and Asio's reactor calls it on every wait, so the count would see the
  C library's wait rather than the rooms'.
  Source: `AGENTS.md` (libs/trystero).
- **The browser tests run twice, with exceptions and without
  (`*_no_exceptions`).** No JavaScript exception may enter WebAssembly in
  either build (owner decision, 2026-10-10).
  Source: `libraries/trystero/openspec/specs/trystero/spec.md` (proposals).
- **`native_websocket`, the native interop peer and `shared_hardened` are
  built with the standard libraries' own checks
  (`_LIBCPP_HARDENING_MODE_EXTENSIVE`, `_GLIBCXX_ASSERTIONS`).** A completion
  that touches a container its socket cleared, or a resource past its
  lifetime, fails at once.
  Source: `AGENTS.md` (libs/trystero).
- **Guarantees keep their numbers and anchors (`trystero-invariant-<n>`),
  which code and tests cite as `(doc: #trystero-invariant-3)`, and the page
  build fails on a citation that names no anchor.** A comment sends its reader
  to the promise it relies on, and a stale citation fails the build.
  Source: `doc/guarantees.adoc` (libs/trystero).
- **The examples are linked and never run, and none has a twin in Trystero's
  language; the interop lane compares trystero with Trystero instead.** Each
  example needs the public relays or a browser.
  Source: `doc/differences.adoc` (libs/trystero).

## 16. Roadmap

What webcpp does not have yet, and the chapters that mention it:

- **Every variant tested, in three levels of CI.** The lint and the CI test
  every variant a library supports: with exceptions on and off (the whole
  suites, not only a compile, chapter 6) and each library's own
  configurations. The CI runs in three levels: fast on every push and pull
  request (the changed library and its dependents, one native compiler and
  wasip2 in the default variant, one native lane without exceptions, the
  lint, the docs); nightly on main (every target and compiler, the suites
  without exceptions natively, sanitizers, the heavy own lanes, the
  container lanes); and release, when the whole of webcpp is released (every
  target times every variant). `tools/ci/matrix.py` takes the level, and
  the fast set runs locally as one command (`AGENTS.md` chapter 4).
- **The review of the existing libraries.** Once the pending work is done,
  every library is brought to the rules of exceptions, variants,
  allocators, the order of what a library builds on, written for the web, the newest Boost
  library of a kind, wrapped outside libraries, and fixed-width types
  (chapter 4); the lint then checks the last (no plain `int`, `long`,
  `short` or `unsigned` in a public header where no interface asks for it).
- **Asynchronous libraries on xactor with Boost.Asio and Boost.Cobalt.** An
  example library shows how a library built on xactor's scheduler meets
  Boost.Asio and Boost.Cobalt, so that asynchronous libraries are easy to
  write and are all driven from the browser's event loop (chapter 4); its
  design is still to be done.
- **webcpp::asio, webcpp::beast, webcpp::webrtc, webcpp::crypto.** Boost.Asio
  and Boost.Beast natively, and their interfaces over the browser's event
  loop, `WebSocket` and `fetch` on emscripten; WebRTC peer connections and
  data channels on both; authenticated encryption and randomness in C++ on
  every target. trystero then holds only its protocol. Their specifications
  are in review.
- **The libraries web developers use most,** brought to C++: a JavaScript
  interpreter (QuickJS), libp2p, HTTP fetch.
- **The site, increments 2 to 8.** Increment 1 is done: the site's
  repository and `b2 site`, the landing, the list of libraries and the three
  doors (C++, JavaScript, AI), the libraries' pages and the report under the
  site's theme, `robots.txt`, `llms.txt` and `libraries.json`, published at
  https://webcpporg.github.io/, the preview, with the redirect from
  `/webcpp/` (`AGENTS.md` chapter 4). To come, in this order:

  2. xstate for emscripten (its machine layer), and the site's wasm running
     its own machines, the player's and the install's: the PWA;
  3. the bridge visualizer and the calculator example;
  4. trystero's JavaScript API and the collaborative cursors;
  5. the examples of a state machine, of C++ calling the browser, and of
     C++ on the server;
  6. file transfer and video chat, as trystero's roadmap delivers them;
  7. Google search, for webcpp.org: the Programmable Search engine and the
     Search Console property are made for the release site; the preview,
     the development version, gets neither and is never indexed;
  8. the release channel at https://webcpp.org/, at the first release, with
     the move of the domain's DNS.
- **Documentation for web developers and for AI developers,** beside the
  documentation for C++ developers every library has today, and webcpp's
  integrations with the AIs.
- **External libraries built from `third_party/`.** trystero needs
  libsecp256k1, libdatachannel and OpenSSL, which webcpp does not build: a
  machine installs them by hand and the CI's actions build them, each found
  where a variable names it, else in `.local/` (`AGENTS.md` chapters 1 and 4),
  until webcpp builds them from `third_party/`, each pinned there by a commit
  or a SHA-256.
- **Every library on `<webcpp/config.hpp>`.** Each library's `config.hpp`
  includes it first: trystero (held with config's publication), and wasi with
  its redesign, which brings Boost into it.
- **Allocators.** The rule of chapter 4, shareable data structures with an
  `Allocator` template parameter and every other allocation through the
  user's allocator, applied first to xactor, xstate and wasi, whose response
  holds its body as a `std::string`; pratt is born with the rule, and
  trystero through `std::pmr` (chapter 4).
- **Compiled Boost libraries.** Boost.URL, Boost.Charconv, Boost.Locale and
  Boost.Cobalt are built from Boost's sources (`/webcpp//boost_url`,
  `/webcpp//boost_charconv`, `/webcpp//boost_locale`, `/webcpp//boost_cobalt`).
  Boost.Locale and Boost.Cobalt have no build without exceptions, and it is not yet measured on Windows. A
  Boost.Test suite still compiles the framework's header-only form (chapters
  2 and 9).
- **Bundled toolchains.** Each toolchain is installed by hand and configured
  in `user-config.jam` (`AGENTS.md` chapter 1), until webcpp bundles them.

## 17. Decided, not built

The proposals the owner accepted that are not in the superproject's `main`,
each with its decisions as decided. While a proposal is open, its own
`design.md` and specification hold it in full and rule over this summary; the
change that builds it moves its decisions into chapter 14 or 15. Their
conflicts with chapters 1 to 13 are in chapter 19.

### asio (Boost.Asio natively, the browser's event loop on Emscripten; wrapper; accepted 2026-10-11, implementing, not in main)

webcpp::asio is a header-only C++20 wrapper over Boost.Asio: natively it is
Boost.Asio itself, and on Emscripten it is Boost's own `io_context`, whose run
family suspends into the browser's event loop and yields to it after every
turn of `max_handlers_per_turn` handlers, with a bridge through which browser
events post, promises and DOM events as Asio operations, sockets and a
resolver that fail as values, and Boost.Cobalt's `co_main` on that loop. The
work is the local, unpublished repository `asio-task9-review/libs/asio`
(branch `work/task14-guide` at `f5fd7a8`, tasks 0 to 21 committed, 142 of the
154 items of `tasks.md` done) on the superproject branch
`work/asio-task9-review`, plus the uncommitted work of 2026-10-15 and
2026-10-16; no CI has run, by the owner's instruction. Left are task 22 (the
items of the rulebook's chapters 1, 6 and 13 as numbered at `ddb6cb2`, now
`AGENTS.md` chapter 1 and chapters 4 to 6 and 16 of this file, the rows of the
tables of libraries, the page in its order) and task 23 (green from scratch,
the whole-branch review and its fixes, then the owner's gates: G1 the owner
creates `webcpporg/asio`, G2 the owner adds the submodule `libs/asio`, G3 the
library is pushed, then the superproject).

Paths: `genesis/` stands for `libraries/asio/openspec/changes/genesis/` of the
proposals repository; (asio) is the library repository at `f5fd7a8` and (asio,
uncommitted) its working tree; (superproject branch) is `asio-task9-review` at
`d159022` and (superproject, uncommitted) its working tree; (main) is `webcpp`
at `ddb6cb2`.

**Decided.**

**Interface and names.**

- **`webcpp::asio::io_context` derives from `boost::asio::io_context` on both
  targets, shares its executor type and converts to
  `boost::asio::io_context&`; the timers are aliases of Boost's, and a program
  keeps naming `post`, `co_spawn`, `use_awaitable` and `bind_executor` from
  Boost.** A program sets its options the same way everywhere, code written
  against either type interoperates, and Cobalt and Beast are templates over
  Boost's executor. A fork of Asio's scheduler was rejected as a maintenance
  burden incompatible with every library on Asio.
  Source: `genesis/design.md`, D1 (proposals).
- **The library is header-only but for two translation units the build
  compiles for a program, `src/browser_src.cpp` (the JavaScript of the yield,
  the bridge and the promises, and the export the bridge delivers through) and
  `src/cobalt_main.cpp`, each linked once by a target.** The JavaScript
  functions and the export exist exactly once, and a second definition fails
  at link.
  Source: `genesis/specs/asio/spec.md`, Library and Build (proposals).
- **The convenience header `<webcpp/asio.hpp>` leaves out
  `<webcpp/asio/ssl.hpp>`.** Natively that header brings OpenSSL, which a user
  of the rest should not pay for.
  Source: `genesis/specs/asio/spec.md`, Library: Convenience header
  (proposals).
- **`<webcpp/asio/config.hpp>` includes `<boost/config.hpp>` and defines
  `WEBCPP_ASIO_NO_EXCEPTIONS` exactly when `BOOST_NO_EXCEPTIONS` is defined,
  keeping a developer's own definition; an API that throws sits behind it and
  raises through `boost::throw_exception`.** Code that uses a throwing API
  then does not compile without exceptions.
  Source: `genesis/specs/asio/spec.md`, Library: Configuration header
  (proposals).
- **On Emscripten `/webcpp/asio//asio` gives every translation unit,
  `browser-src` included, `BOOST_ASIO_DISABLE_THREADS`, and links embind,
  `browser-src`, the variant's link flags and the tag rule.** Without the
  define Asio decides it has threads and stops at "Only Windows and POSIX are
  supported!"; its inline namespace differs with and without threads, so a
  unit that missed it fails at link, never silently.
  Source: `genesis/specs/asio/spec.md`, Build (proposals).
- **The library's targets are native and Emscripten, and a library built on it
  declares native and Emscripten at most.** Boost.Asio 1.92 does not compile
  for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34; a WASI backend is a
  later increment with its own specification. The local WASIp3 port departs
  from this (below).
  Source: `genesis/design.md`, D16 (proposals).
- **A library built on webcpp::asio is given an executor and never calls
  `run`, `run_one` or `run_for`; a program's own call says why after
  `lint-run:`, and xactor's `asio_driver` keeps draining an `io_context{1}` of
  its own with `poll`.** The run family may suspend only where a program
  allows it, and a context without a reactor suspends nothing, so xactor is a
  user of asio, not a dependency.
  Source: `genesis/specs/asio/spec.md`, Run, and `genesis/proposal.md`
  (proposals).

**The run family and the turn limit.**

- **On Emscripten the six members of the run family hide Boost's and compose
  `poll_one` and `run_one`: a turn of ready handlers, a return if stopped or
  expired, a yield if the turn reached the limit, else one `run_one` that
  suspends in the reactor's `select`; natively they are Boost's through
  `using` declarations.** Boost's `run` is not virtual and cannot be told to
  yield: measured, a self-posting handler ran 300,000 times under it before a
  `setTimeout(0)` ran, because a zero-timeout `select` suspends only to a
  microtask. Code that calls `run` through a `boost::asio::io_context&` gets
  Boost's run without the limit, as documented.
  Source: `genesis/design.md`, D2 (proposals).
- **The yield is the `EM_ASYNC_JS` import `webcpp_asio_yield`, which awaits a
  promise that a `MessageChannel` message resolves.** A macrotask lets the
  browser render, run its timers and deliver its events, and `MessageChannel`
  exists in node and in every browser; the guide calls the yield an
  opportunity for browser work, not a promise that a given timer or frame runs
  first.
  Source: `genesis/design.md`, D3 (proposals); `doc/run.adoc` (asio).
- **A timed run checks its deadline before each handler, a limit of 0 is taken
  as 1, and a `stop()` by the handler that reaches the limit returns at once,
  without a yield.** A passed deadline then runs nothing, a self-posting
  handler cannot outrun `run_for`, and the run family never yields forever
  without running anything.
  Source: `genesis/design.md`, D3, and `genesis/specs/asio/spec.md`, Run
  (proposals).
- **The default `max_handlers_per_turn` is 100, the build default
  `WEBCPP_ASIO_DEFAULT_MAX_HANDLERS_PER_TURN` of `config.hpp`, which a context
  overrides at construction or with `max_handlers_per_turn(n)`.** The owner
  selected 100 on 2026-10-15, after the Release yield benchmark, to favor the
  browser's responsiveness: about ten times as many yields as 1,000, at about
  44.0/54.5 ms of scheduling per million trivial handlers (JSPI/Asyncify)
  against 4.64/5.73 ms, a policy and not a measured optimum. The earlier 1,000
  was an initial policy, kept after task 13's Chrome check (turn p95 0.7 ms
  under JSPI, 1.6 ms under Asyncify).
  Source: `doc/yield-cost.adoc`, `AGENTS.md`, commit `24c1e2c` (asio).
- **The build default is a compiler definition that every translation unit of
  a program shares, Cobalt's separately compiled runner included, and a
  negative value stops the compile.** The owner asked for a build-time
  configuration; objects built with different definitions are never mixed in
  one program.
  Source: `include/webcpp/asio/config.hpp`, `doc/run.adoc` (asio);
  `genesis/reference/STATUS.md` (proposals).
- **The limit counts handlers and is no time budget: up to
  `k + 1 + max_handlers_per_turn` handlers can run between two yields, and a
  long handler is never preempted.** Step 4's one handler belongs to no turn,
  so a short turn of `k` handlers followed by a wait that ends at once runs it
  with no macrotask between.
  Source: `genesis/design.md`, Risks (proposals); `doc/run.adoc` (asio).
- **The browser lane stops the work for the owner's decision when a turn's
  95th percentile exceeds 8 ms in any of three runs, and its driver fails when
  no animation callback occurs.** The criterion leaves roughly half of a 60 Hz
  frame's 16.7 ms to the browser and is a heuristic, not a frame-rate
  guarantee; with 100, Chrome measured a turn p95 of 0.100 ms (JSPI) and 0.300
  ms (Asyncify) and about 60 callbacks a second.
  Source: `genesis/design.md`, D3, and `genesis/reference/STATUS.md`
  (proposals); `doc/yield-cost.adoc` (asio).
- **The turn limit's tests count the run family's yields
  (`webcpp::test::yield_counter`, every `MessagePort.postMessage`) and assert
  where a `setTimeout(0)` runs, never after how many handlers.** When a
  `setTimeout(0)` runs is the host's: node delays it by 1 ms, a browser clamps
  a nested one to 4 ms and throttles a background tab's to 1 s or more.
  Source: `genesis/specs/asio/spec.md`, Testing (proposals).

**The reactor and who calls the run family.**

- **The io_context holds one unarmed `boost::asio::steady_timer` member,
  through public API, so that the reactor exists from construction.** With
  threads disabled, a `run` that has work and no reactor waits in
  `null_event::do_wait`'s `::pause()`, which nothing wakes (measured); with
  the reactor it suspends in `select`, which a post wakes.
  Source: `genesis/design.md`, D6 (proposals).
- **Handlers run only on the stack of a run family call: work and events that
  arrive before `run` wait for it, and after `main` returns, the runtime
  alive, nothing runs until an export listed for suspension calls the run
  family again.** The owner settled on 2026-10-10 that use without `run` is
  not supported, on every target, as natively.
  Source: `genesis/reference/spec.md`, sections 4.8 and 11 (proposals).
- **The run family is called on `main`'s stack or on an export the program
  lists for suspension (`-sJSPI_EXPORTS`, `ccall(..., {async: true})`), and
  nothing calls `run`, `poll` or `dispatch` from a JavaScript event.**
  Anywhere else its first suspension fails: a `SuspendError` under JSPI, an
  unwind that corrupts a suspended `main` under Asyncify.
  Source: `genesis/specs/asio/spec.md`, Run (proposals); `AGENTS.md` (asio).

**The bridge and the detection.**

- **Every browser event source is a JavaScript function made by
  `browser::bridge`, a service of the io_context made with its allocator,
  registered by a `std::uint64_t` key counted from 1 and never reused (a
  `BigInt` in JavaScript); a delivery looks the key up and only posts.** While
  `run` is suspended an event sees `running_in_this_thread()` true, so a
  `dispatch` would run its handler on the event's stack, and a run family call
  there fails at its first suspension; a key never reused lets a late event
  find nothing.
  Source: `genesis/design.md`, D7 (proposals).
- **The bridge's JavaScript functions keep the stack pointer, the count of
  deliveries in progress and the event's handle however a delivery ends; a C++
  exception is caught in C++, a trap goes on to the page, `ExitStatus` and
  `'unwind'` end the delivery unreported, and nothing enters WebAssembly after
  `ABORT`.** A JavaScript error passes every C++ `catch` and skips the frames'
  epilogues, and their destructors outside WebAssembly exceptions.
  Source: `genesis/design.md`, D7 (proposals).
- **`bridge::deliver` is private, with the export `webcpp_asio_deliver` as its
  only friend, and `delivering()` takes no value, both pinned by compile-fail
  tests.** An earlier public `unwound` let user code break the refusal of a
  run family call made from an event.
  Source: `genesis/design.md`, D7, and `genesis/reference/STATUS.md`
  (proposals).
- **Each of the six members admits its call once, first, by a flag
  (`BOOST_ASSERT_MSG`, else a return of 0 that runs nothing), then calls only
  private unchecked code; a `flag_scope` puts back the value it found, and the
  four calls it cannot detect are documented.** A public member that called
  another would be refused by its own flag, and an inner scope must never
  clear an outer one's; a C++ exception clears the flag on its way out of
  `run`.
  Source: `genesis/design.md`, D8 (proposals).
- **asio does not restore its flag after a JavaScript error escapes a handler:
  handlers catch JavaScript errors in JavaScript.** No boundary can restore
  Boost's own state (its work count, its call stack), and a recovered flag
  would trade a reported refusal for a `run` that never returns.
  Source: `genesis/design.md`, D8 (proposals).

**Suspension variants, artifacts and the loader.**

- **JSPI and Asyncify are build variants, the propagated composite feature
  `webcpp-suspend`: `jspi` by default, with WebAssembly exceptions, and
  `asyncify`, which composes `<exception-handling-method>js` and builds in a
  directory of its own.** The owner decided on 2026-10-10 that the variant is
  chosen at build time and the developer ships the artifact a page's loader
  picks; b2 takes only free features from usage requirements, and Asyncify
  traps at run time with WebAssembly exceptions.
  Source: `genesis/design.md`, D4 (proposals).
- **The variants link `-sJSPI` and `-sASYNCIFY` with no
  `ASYNCIFY_IGNORE_INDIRECT` and no list of suspending imports.** Emscripten
  finds `__syscall_poll` and the yield itself, and the path from `main` to
  `select` passes the virtual `scheduler_task::run`.
  Source: `genesis/specs/asio/spec.md`, Build (proposals).
- **Asyncify's stack is 8192 bytes unless a program raises it with the
  propagated feature `asyncify-stack-size` (powers of two up to 1048576, a
  smaller value stopping the compile), and `asyncify_stack` reads the bytes
  each unwind wrote from Asyncify's own data, in debug, release and release
  linked with `-O3`.** A suspension that does not fit traps with
  `RuntimeError: unreachable`; 8192 is twice the smallest power of two every
  build passed with (deepest 3,540 bytes, Cobalt's 1,088), and Emscripten's
  default of 4096 left 556 bytes.
  Source: `genesis/design.md`, D5 (proposals); `AGENTS.md` (asio).
- **Artifacts are named Boost's way (`app-60-d-jspi.js`,
  `webcpp-suspend-asyncify/app-60-d-asyncify.js`), `webcpp.variants` builds a
  page program in both variants with `<name>.variants.json`, and
  `webcpp-asio-loader.mjs` (`supportsJspi`, `pick`, `load`) reads that
  manifest, never a pattern.** The owner decided on 2026-10-10 that build
  output and names follow Boost's own build exactly, and the manifest lets the
  page load the variant its browser runs; the loader resolves scripts against
  `response.url`, so that a redirect does not misdirect it.
  Source: `genesis/design.md`, D10, and `genesis/reference/STATUS.md`, task 12
  (proposals).
- **Published binary sizes are release builds linked with `-O3` explicitly,
  and `webcpp.size-table` records every guide example in both variants.** b2's
  emscripten toolset does not pass the optimization level to the link; in
  release Asyncify's program is about 1.9 times JSPI's (2.9 in debug). The
  optimized JSPI link exports `emscripten_stack_get_current` and
  `_emscripten_stack_restore`, since Emscripten reads `EM_JS_DEPS` after
  native linking.
  Source: `genesis/reference/STATUS.md`, task 14 (proposals); `build.jam`
  (asio).

**Tests.**

- **A test under node ends as its `main` ends: `node-pre.js`, which every
  `webcpp.run`, `webcpp.run-fail` and `webcpp.example` links on Emscripten,
  fails a test whose `main` did not finish, keeps the first status
  `webcpp::test::exit_status` gives, and stops a test past its bound (120 s by
  default).** Under node a `main` that suspended and returns non-zero exits 0
  under Asyncify, and one that never finishes exits 0 on both variants, so
  every Asyncify lane would read green whatever it tested.
  Source: `genesis/design.md`, D9 (proposals); `AGENTS.md`, chapter 9
  (superproject branch).
- **Cobalt's exit statuses are checked by a driver, exactly 7 and 0, in lanes
  of their own (`cobalt`, `cobalt_asyncify`).** A `run-fail` test alone would
  also accept a trap or an unfinished `main`.
  Source: `genesis/reference/STATUS.md`, task 16 (proposals); `test/Jamfile`
  (asio).
- **Concurrent builds are not capped: a test that fails under load is flaky
  and is fixed.** The owner ruled on 2026-10-11 that a flaky test is never
  protected by running less at once.
  Source: `genesis/reference/plan.md`, Global Constraints (proposals).

**Exceptions.**

- **A handler's exception propagates out of `run` on both variants, after any
  number of suspensions, and each variant compiles with the exception model it
  can suspend through.** Handlers run on `main`'s stack inside the run family
  call, and Asyncify traps when it suspends inside a `catch` block of
  WebAssembly exceptions.
  Source: `genesis/specs/asio/spec.md`, Exceptions (proposals).
- **Every browser test is declared in four builds, JSPI and Asyncify each with
  exceptions on and off, the Asyncify ones in the own lane `asyncify`.** The
  owner asked on 2026-10-10 that every variant be tested; native runs without
  exceptions are departure 5.
  Source: `genesis/specs/asio/spec.md`, Exceptions (proposals).
- **`/webcpp//boost_cobalt` follows the consumer's exception setting, and
  without exceptions b2 prints a non-fatal warning that names Cobalt's issue
  287.** The owner asked on 2026-10-14 to remove the blanket refusal; in Boost
  1.92 moving a pending task calls `std::make_exception_ptr`, which aborts
  with libc++ and `-fno-exceptions`, so the warning states a run-time risk and
  claims no safety.
  Source: `genesis/reference/STATUS.md` (proposals); `AGENTS.md`, chapter 1
  (superproject branch).
- **Exception-free Cobalt consumers suppress Clang's
  `coroutine-missing-unhandled-exception` diagnostic, and no other.** Boost
  omits `main_promise::unhandled_exception` without exceptions.
  Source: `build.jam`, `AGENTS.md` (asio).

**Allocation.**

- **The io_context passes the user's allocator to Boost's constructor, an
  operation allocates through its handler's associated allocator, and the
  bridge's map through the context's.** Run-time objects route their
  allocations through the user's allocator; an allocator given to the context
  is not associated with handlers, which bind it with `bind_allocator`.
  Source: `genesis/specs/asio/spec.md`, Allocation (proposals);
  `doc/allocation.adoc` (asio).
- **The counting test counts every `operator new`, the C library's allocation
  functions and the aligned ones, with a control that sees an unbound handler,
  and names rather than tolerates what the runtime allocates itself:
  Emscripten's `select` poll array, Asyncify's unwind data and
  `basic_resolver_results`' entries.** Asio's default handler memory ends in
  `aligned_alloc` or Boost.Align, never in `operator new`, so counting
  `operator new` alone reads 0; native allocation tests compile with
  `-fno-builtin`, since optimized builtins elided or reordered the counted
  calls.
  Source: `genesis/specs/asio/spec.md`, Allocation, and
  `genesis/reference/STATUS.md` (proposals).

**Timers, promises and events.**

- **Timers are Boost's on both targets, in the reactor's timer queue: Beast's
  timeouts fire, and an idle program holds the reactor's one `setTimeout`, of
  at most 300 s, and schedules nothing else.** The program's code runs only
  when the browser delivers an event, never by polling.
  Source: `genesis/specs/asio/spec.md`, Timers and Browser (proposals).
- **`async_await(executor, promise, token)` and
  `async_wait_event(executor, target, type, token)` initiate through
  `async_initiate` as bridge registrations: the settlement or the event posts
  the handler, a rejection completes with `browser::error::promise_rejected`
  (value 1 of `webcpp.asio`, never reused) and the reason, a cancellation
  completes at once with `operation_aborted` and drops the registration (and
  the listener), and a pending operation is outstanding work that destroying
  the io_context destroys uncalled.** Every completion token then works (a
  callback, `deferred`, `use_awaitable`, Cobalt's `use_op`), and a late
  settlement finds nothing.
  Source: `genesis/design.md`, D12 (proposals).
- **A promise or event operation allocates through its handler's associated
  allocator and keeps its handler's executor work while pending; an event wait
  listens with `once: true` and makes the event type a JavaScript string from
  the `std::string_view`, with no `std::string`.** The final review found the
  handler's executor work lost, fixed in `f5fd7a8`.
  Source: `genesis/specs/asio/spec.md`, Promise and Event, and
  `genesis/reference/STATUS.md` (proposals).

**Sockets, the resolver and TLS.**

- **On Emscripten `ip::tcp` and `ip::udp` are webcpp's protocol classes with
  Boost's members, and the sockets and the acceptor are class templates over
  `browser::transport<Protocol>`, whose primary completes every operation with
  `operation_not_supported`, synchronously or through a post; natively all are
  aliases of Boost's.** Boost's sockets compile on Emscripten but connect
  through SOCKFS, which opens a WebSocket, and `getaddrinfo` invents
  addresses; the owner decided on 2026-10-10 that what a browser cannot do
  compiles and fails as a value behind a point a relay can fill.
  Source: `genesis/design.md`, D13 (proposals).
- **A transport specialisation is visible in every translation unit before its
  first use, is enabled for a whole program by a b2 feature designed with the
  first relay library, supplies `close()`, and leaves a moved-from source with
  a usable executor.** The one definition rule; the final review found `close`
  not forwarded and moved-from transports unusable.
  Source: `genesis/design.md`, D13, and `genesis/reference/STATUS.md`
  (proposals).
- **The resolver resolves nothing (option A): one entry with the host and
  service names, an unspecified address and the service's port; reverse
  resolution, and a plain socket's connect on the entry, fail with
  `operation_not_supported`.** Only webcpp::beast's browser streams accept the
  entry, so Beast's layered client code runs unchanged; a resolver that failed
  like the sockets would change a line of every Beast client.
  Source: `genesis/design.md`, D14 (proposals).
- **`<webcpp/asio/ssl.hpp>` is Boost's `ssl` natively and on Emscripten a
  record of the program's TLS settings, its host name in `std::pmr` storage,
  which only webcpp::beast's browser streams read; a program names
  `/webcpp/asio//openssl` for its native builds alone, and naming it for
  WebAssembly stops the compile with an instruction.** The browser owns TLS
  there, and the owner rejected ignoring the target on WebAssembly.
  Source: `genesis/design.md`, D15 (proposals); `include/webcpp/asio/ssl.hpp`
  (asio).

**Boost.Cobalt.**

- **Cobalt's `main_promise::run_main` and `final_suspend` are webcpp's
  (`src/cobalt_main.cpp`, `/webcpp/asio//cobalt`), with Cobalt's steps over a
  `webcpp::asio::io_context{BOOST_ASIO_CONCURRENCY_HINT_1}`, and
  `/webcpp//boost_cobalt` compiles every Cobalt source but `main.cpp`.**
  `co_main` cannot be replaced from a header: `main` is a hidden friend that
  calls `run_main`, compiled in Cobalt's `src/main.cpp`, which calls Boost's
  `run`; compiling against the pinned Boost probes the private members it
  reads, and Cobalt's own `co_main`, without the limit, is the fallback.
  Source: `genesis/design.md`, D11 (proposals).
- **Under Asyncify `run_main` ends the program with `std::exit` once the
  context and the signal set are destroyed, and `EXIT_RUNTIME` stays 0.** A
  `main` that suspended loses its return value under Asyncify, so the
  `co_return` value would not be the exit status; a full runtime teardown is
  not promised.
  Source: `genesis/design.md`, D11, and `genesis/reference/STATUS.md`, task 16
  (proposals).
- **`src/cobalt_main.cpp` keeps the copyright notice of Boost.Cobalt's
  `main.cpp`, beside webcpp's.** It adapts that file; the task's review found
  the attribution missing.
  Source: `genesis/reference/STATUS.md`, task 16 (proposals).

**Documentation and release.**

- **The guide is one AsciiDoc page with a MrDocs reference, written by
  learning, doing, understanding and reference needs (Diataxis), with thirteen
  numbered guarantees and examples checked against their `.expected` output,
  the portable ones natively and in the browser.** The owner asked for
  Diataxis writing on 2026-10-14 and kept the superproject's documentation
  pipeline.
  Source: `genesis/reference/STATUS.md`, task 14 (proposals);
  `doc/guarantees.adoc` (asio).
- **Nothing is published before the whole-branch review is fixed and the owner
  has taken the three gates (G1 the repository, G2 the submodule, G3 the
  library pushed before the superproject); no agent pushes, creates a
  repository or changes a GitHub setting.** The rulebook's checklist makes the
  repository and the submodule the owner's.
  Source: `genesis/specs/asio/spec.md`, Release (proposals).

**Departures the owner approved.** The owner approved all five on 2026-10-11;
recording that approval in the ledger is task 23.4, open.

- **1. The own lane `asyncify` runs on every push, not at the nightly and
  release levels the spec settled.** The CI's levels do not exist, and no
  Asyncify in the CI would break the owner's "every variant tested"; running
  it on every push tests more, not less. Status: in effect in `test/Jamfile`;
  no CI has run it; it moves when the levels come, which task 22 writes into
  the roadmap (chapter 16).
  Source: `genesis/reference/plan.md`, Departures from the spec, and
  `genesis/tasks.md` (proposals).
- **2. `io_context` has two constructors more than the spec's three,
  forwarding Boost's concurrency hint with and without an allocator.** Boost's
  idiom `io_context io{1}` compiles, and webcpp's Cobalt `run_main` keeps
  Cobalt's `BOOST_ASIO_CONCURRENCY_HINT_1`. Status: implemented in task 4
  (`names_test`), used by task 16.
  Source: `genesis/reference/plan.md`, Departures from the spec (proposals).
- **3. `/webcpp//boost_cobalt` leaves out Cobalt's `io/ssl.cpp`.** It needs
  OpenSSL, which the program names, so `boost::cobalt::io::ssl_stream` does
  not link until Cobalt's TLS sources are built against the program's OpenSSL.
  Status: implemented in superproject `8829e99`; the uncommitted
  `/webcpp//boost_cobalt_io_ssl` of 2026-10-16 now builds that source as a
  library of its own, natively and on WASIp3.
  Source: `genesis/reference/plan.md`, Departures from the spec (proposals);
  `tools/boost/cobalt-ssl/Jamfile` (superproject, uncommitted).
- **4. `/webcpp/asio//openssl` links the system's OpenSSL, shared, with no
  static choice, until webcpp::crypto's `/third_party//openssl` exists, and
  the target is refused on WebAssembly.** The shared-or-static feature of
  chapter 5's rule 7 comes with `/third_party//openssl`, which
  webcpp::crypto's plan builds; the owner approved this part a and rejected
  ignoring the target on WebAssembly. Status: implemented in `d438a61` through
  `webcpp.external`; superseded in the uncommitted tree by OpenSSL 3.5.9 built
  from source, static only, natively and on WASIp3, without webcpp::crypto.
  Source: `genesis/reference/plan.md`, Departures from the spec (proposals);
  `build.jam` (asio, uncommitted).
- **5. Native suites without exceptions run only on Emscripten for now.**
  Native runs without exceptions wait for the CI's native no-exceptions lane,
  a levels item, while every browser test has its no-exceptions twins and the
  lint analyses every native program without exceptions. Status: in effect;
  `flag_scope_no_exceptions`, `ssl_no_exceptions` and the Cobalt programs
  already run natively without exceptions, and the local matrices of
  2026-10-16 ran native profiles with exceptions off.
  Source: `genesis/reference/plan.md`, Departures from the spec (proposals);
  `test/Jamfile` (asio).

**Local work beyond the proposal (not proposed).** Uncommitted in
`asio-task9-review` and its `libs/asio`, of 2026-10-15 and 2026-10-16, each
increment with a spec or a plan under `docs/superpowers/` that records the
owner's approval of its scope (the Cobalt WASIp3 plan records none); no
proposal covers it and no CI ran.

- **Asio runs on WASIp3 (`clang-wasip3`) through a checked patch of Boost
  1.92's platform layer, applied to private headers under
  `bin/boost-asio-wasip3/include` for that toolset alone, keeping Boost's
  scheduler and select reactor.** Boost.Asio assumes POSIX signals, `::pause`
  and `ESHUTDOWN`; patching a private copy leaves the installed Boost
  untouched, and the preparation refuses another Boost version or a modified
  input rather than compile unpatched headers.
  Source: `docs/superpowers/specs/2026-10-15-asio-wasip3.md`,
  `tools/boost/asio-wasip3/README.md` (superproject, uncommitted).
- **On WASIp3 the io_context uses Boost's own run family, with no turn limit,
  bridge or yield.** Its system calls suspend through wasi-libc and the host's
  Component Model.
  Source: `doc/capabilities.adoc`, `include/webcpp/asio/io_context.hpp` (asio,
  uncommitted).
- **`threading=single` disables Asio's threads, with exceptions on or off;
  `threading=multi` selects wasi-sdk's experimental cooperative sysroot with
  `-pthread`, exceptions off, and a combination the SDK cannot supply stops
  the build.** A threaded build must never get the single-threaded
  implementation in silence, and wasi-sdk 34's cooperative sysroot has no C++
  exception runtime.
  Source: `docs/superpowers/specs/2026-10-15-asio-wasip3.md`,
  `tools/webcpp-wasi.jam` (superproject, uncommitted).
- **The WASIp3 tests run under Wasmtime 49.0.3 through the launcher
  `tools/boost/asio-wasip3/run.sh`, which grants network and name lookup and
  bounds a run to 15 s.** Wasmtime 47.0.3 cannot load the cooperative
  component, and the loopback tests need the network; the launcher is no
  production capability policy.
  Source: `tools/boost/asio-wasip3/README.md` (superproject, uncommitted).
- **UDP `send_to` and `async_send_to` gather several nonempty buffers into one
  datagram, in storage from the context's allocator (synchronous) or the
  handler's (asynchronous), released before the handler runs; a receive or a
  connected send of several nonempty buffers fails with
  `operation_not_supported`.** wasi-libc has no `sendmsg`, datagram boundaries
  never change to make a buffer sequence compile, and no hidden allocation
  emulates scatter/gather.
  Source: `docs/superpowers/specs/2026-10-15-asio-wasip3.md` (superproject,
  uncommitted); `AGENTS.md` (asio, uncommitted).
- **What WASI lacks is explicit: Asio's `shut_down` error maps to `EPIPE`,
  interface-name scopes are refused, a missing socket option reports
  `operation_not_supported`, and an asynchronous resolve without threads posts
  `operation_not_supported`.** WASI has no `ESHUTDOWN`, no interface names
  and, in the single profile, no thread for Boost's resolver; the port does
  not pretend those entry points exist.
  Source: `doc/capabilities.adoc` (asio, uncommitted).
- **The library's test and example Jamfiles declare `wasip3` beside native and
  Emscripten, Cobalt and OpenSSL refuse WASIp2 by name, and
  `doc/capabilities.adoc` with `doc/feature-matrix.adoc` is kept current for
  every feature, target and variant, telling unavailable from unverified.**
  The owner approved an AsciiDoc feature matrix that links its evidence.
  Source: `docs/superpowers/specs/2026-10-15-asio-wasip3.md` (superproject,
  uncommitted); `AGENTS.md` (asio, uncommitted).
- **`/webcpp/asio//cobalt` builds on WASIp3 from a checked patch of private
  copies of Cobalt's headers and sources (`tools/boost/cobalt-wasip3`): its
  signal, serial-port and seqpacket sources are left out, `thread.cpp` comes
  only with threads, and the archive and its consumers share the Asio overlay
  and the thread profile.** Unsupported local sockets, signals and serial
  ports must not break the IP APIs, and an archive and its program must agree
  about headers, threads and exceptions.
  Source: `docs/superpowers/plans/2026-10-16-cobalt-wasip3.md`,
  `tools/boost/Jamfile` (superproject, uncommitted).
- **On WASIp3 `run_main` sets the promise's `signal_set` to null and includes
  no POSIX signal service, keeping the executor, the initial post, the status
  and Cobalt's internal cancellation.** WASIp3 delivers no process signal; a
  host's graceful shutdown needs a protocol of the program's.
  Source: `src/cobalt_main.cpp`, `AGENTS.md` (asio, uncommitted).
- **On WASIp3 the Cobalt status driver expects host exit status 1 for a
  nonzero `co_return`.** (reason not recorded)
  Source: `test/cobalt_status_driver.mjs` (asio, uncommitted).
- **OpenSSL 3.5.9 (`45e844f`) is a pinned, unmodified submodule at
  `third_party/libs/openssl`, built as static `libssl.a` and `libcrypto.a`
  with their generated headers by upstream Configure and `make build_libs`,
  driven by B2 as `/webcpp/third_party//openssl`, in a directory keyed by the
  effective compiler configuration.** Upstream compiles file by file from its
  own manifests, which B2 would otherwise duplicate, and the headers and
  archives of one configuration must stay paired.
  Source: `docs/superpowers/plans/2026-10-16-openssl-source-build.md`,
  `tools/webcpp-openssl.jam` (superproject, uncommitted).
- **The OpenSSL recipe covers native Clang and GCC on macOS and Linux and
  WASIp3, single and cooperative; WASIp2, Emscripten and MSVC stop the build,
  and a host OpenSSL is never a fallback.** Windows needs an nmake integration
  of its own, and a silent system fallback could link a second copy.
  Source: `docs/superpowers/plans/2026-10-16-openssl-source-build.md`
  (superproject, uncommitted).
- **On WASIp3 OpenSSL uses the external Configure target `webcpp-wasip3` (no
  assembly, no DSO, no QUIC, `getrandom` seeding) and fails closed when host
  entropy fails.** wasi-libc's `getentropy` reaches `wasi:random`; the tests
  show entropy is requested and that its failure stops key generation, not its
  quality.
  Source: `tools/openssl/wasip3.conf`, `tools/openssl/build.py` (superproject,
  uncommitted); `doc/building.adoc` (asio, uncommitted).
- **`/webcpp/asio//openssl` forwards `/webcpp/third_party//openssl` with
  `WEBCPP_ASIO_HAS_OPENSSL` natively and on WASIp3, Emscripten keeps the
  forced-include refusal, and asio's tests that need OpenSSL fail rather than
  being left out.** A missing checkout is named
  (`git submodule update --init third_party/libs/openssl`), never skipped.
  Source: `build.jam`, `doc/building.adoc` (asio, uncommitted).
- **The OpenSSL build copies the unchanged sources to a temporary directory
  before Configure, and caches its configuration fingerprint per property
  set.** Configure canonicalizes source symlinks, which would bring a checkout
  path with spaces into the Makefiles, and an uncached fingerprint built two
  archives for one configuration, two OpenSSL copies in one program.
  Source: `docs/superpowers/plans/2026-10-16-openssl-source-build.md`,
  `docs/superpowers/plans/2026-10-16-cobalt-ssl.md` (superproject,
  uncommitted).
- **A dated curl export of Mozilla's root certificates (2026-09-25, 121 CAs)
  is checked in under `third_party/ca_certificates`, with a manifest of its
  URL, SHA-256 and DER fingerprints and its MPL-2.0 licence, verified and
  embedded as a generated header by `/webcpp/third_party//ca_certificates`,
  and changed only by `tools/ca_certificates/update.py`.** It is a published
  data artifact, not implementation code, so a snapshot rather than a
  submodule; builds need no network, and an update is a reviewed trust-policy
  change that lists removals beside additions.
  Source: `docs/superpowers/plans/2026-10-16-ca-certificates.md`,
  `third_party/ca_certificates/README.md` (superproject, uncommitted).
- **`/webcpp/asio//tls` and `make_tls_client_context()` return a
  `boost::system::result<ssl::context>` that verifies peers, requires TLS 1.2
  or newer and trusts the embedded roots, never the system's; the
  `std::string_view` overload trusts only the PEM it is given, refusing an
  empty, malformed or partly invalid set, and Emscripten returns
  `unsupported_target`.** Replacing roots must remove public trust, not add to
  it, and the browser owns trust there; hostname verification and SNI stay the
  program's, per connection.
  Source: `include/webcpp/asio/tls_client_context.hpp` (asio, uncommitted);
  `docs/superpowers/plans/2026-10-16-ca-certificates.md` (superproject,
  uncommitted).
- **The TLS client context and its trust store allocate through OpenSSL, not
  through an Asio allocator.** (reason not recorded)
  Source: `docs/superpowers/plans/2026-10-16-ca-certificates.md`
  (superproject, uncommitted).
- **`/webcpp//boost_cobalt_io_ssl` compiles Cobalt's `io/ssl.cpp` as a library
  of its own with a checked patch (`tools/boost/cobalt-ssl`), and
  `/webcpp/asio//cobalt_ssl` adds it and `/webcpp/asio//tls` to
  `/webcpp/asio//cobalt`, natively and on WASIp3; plain Cobalt takes no
  OpenSSL.** Against a real TLS server, Boost 1.92's wrapper finished a
  handshake with `secure` false and then wrote plaintext, and its verification
  callback's constraint named `ssl::context` for `ssl::verify_context`; the
  patch fixes both and adds the borrowed `native_ssl_handle()` for SNI.
  Source: `docs/superpowers/plans/2026-10-16-cobalt-ssl.md` (superproject,
  uncommitted).
- **Cobalt's SSL stream allows plaintext only before the first TLS attempt: a
  failed or cancelled handshake and a completed shutdown block application
  I/O, and the wrapper keeps the handler's associations.** A failed handshake
  must never fall back to plaintext, and an unbound lambda in their place
  would lose cancellation.
  Source: `docs/superpowers/plans/2026-10-16-cobalt-ssl.md` (superproject,
  uncommitted).
- **Every Cobalt target, native and Emscripten included, uses the checked
  overlay `tools/boost/cobalt-common`, whose composition promise connects its
  cancellation source to the handler's slot.** Reproduced: a composed read's
  cancellation hung, because Boost 1.92's composition promise did not attach
  to the received handler's slot; the overlay pins the integration to Boost
  1.92 until the patch is rebased and verified.
  Source: `docs/superpowers/plans/2026-10-16-cobalt-ssl-cancellation.md`
  (superproject, uncommitted).
- **TLS transfer cancellation is terminal: the connection is closed
  afterwards, with no promise of rollback or reuse, and the tests require a
  real `operation_aborted` from a Cobalt promise, one write per cancellable
  promise, and a fresh connection when a completion wins the race.** TCP
  backpressure does not exclude a queued successful SSL completion, which the
  review reproduced, and an abort manufactured from coroutine state would
  prove nothing.
  Source: `docs/superpowers/plans/2026-10-16-cobalt-ssl-cancellation.md`
  (superproject, uncommitted).
- **The superproject's README names Crossbind's OpenSSL recipe as an
  engineering reference for WASIp3 builds.** Its source-based recipes and WASI
  ports help to bring low-level libraries to the web; webcpp still validates
  the toolchain, the entropy and the features itself.
  Source: `README.md` (superproject, uncommitted).

Its conflicts with chapters 1 to 13 are in chapter 19.

### crypto (authenticated encryption, randomness and secp256k1 over C backends; wrapper; accepted 2026-10-11, not built)

The plan (`reference/plan.md`, a draft of 2026-10-11) builds the library in 43
tasks over five increments: the foundation and AES-GCM on BearSSL and OpenSSL;
libsecp256k1 and ctaes; webcpp::trystero moved onto the library;
ChaCha20-Poly1305 on BearSSL and OpenSSL; Botan, native and with exceptions
only. Nothing is built and `webcpporg/crypto` has no commit: the work waits on
the owner's rewrite of `main`'s history, then Task 0a (`infra/shared`
integrated into `main`) and asio's Task 2 (`webcpp::test::exit_status`), and
on the owner's gates G1 and G2 (the repository and its submodule), O2 (the
CI's nightly level), O3 (NIST's archives), O5 (the departures C14, C16 and
C17, and `brew install botan`) and G4 (every push).

**Scope.**

- **Hashing and HMAC stay with Boost.Hash2, and HKDF, when a library needs it,
  is written on `boost::hash2::hmac`.** The owner's amendment C1 of 2026-10-10
  moved them out of the library, which offers no second interface for them; no
  further reason is recorded.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 1.2 (proposals).
- **Every call is synchronous: no promise, callback or awaitable, and
  WebCrypto is not a backend.** The first design was asynchronous only because
  WebCrypto is promise-based; the owner removed WebCrypto (amendment C1), and
  with C code on every target nothing waits.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 1.1 (proposals).

**Interface.**

- **Algorithm types follow Boost.Hash2: sizes as `static constexpr` members
  (`key_size`, `iv_size`, `tag_size`, `block_size`), the output named
  `result_type`, modes as templates over a cipher (`gcm<aes256<>>`), tags as
  `boost::hash2::digest<16>`, and the backend a template parameter defaulting
  to `default_backend`.** Boost's own shape is what a Boost user expects, and
  it keeps `gcm` parallel to `hmac<H>`.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D2
  (proposals).
- **A keyed object is built by `static make(...)`, which returns
  `boost::system::result<T>`, never by a constructor.** A key can have the
  wrong size and a backend can fail to allocate; a constructor that throws was
  rejected because the library works without exceptions.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D2
  (proposals).
- **The algorithm types do every guard, the moved-from state, the incremental
  seal's state machine, the length accounting and the zeroing of a failed
  `open` once for every backend, and a backend supplies only six traits in
  `detail` (`block_state`, `block_engine`, `gcm_state`, `gcm_engine`,
  `chacha_state`, `chacha_engine`).** The same guards on every backend are
  what lets the agreement tests compare them. A state trait never refuses, so
  `aes256<backends::bearssl>` is a complete type `gcm` can name, and an engine
  trait refuses only where the algorithm is called.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D3
  (proposals).
- **A keyed object is move-only, its destructor wipes the key schedule, and a
  move leaves the source empty, every function of which but assignment and
  destruction returns `invalid_state` before it reads a secret byte.** Without
  that guard a moved-from BearSSL `gcm`, whose vtable survives the move, would
  seal under an all-zero key schedule and return a valid-looking tag.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).
- **Data and keys are `std::span<std::uint8_t const>` and
  `std::span<std::uint8_t>`, sizes are `std::size_t`, and a limit wider than
  `size_t` on wasm32 is a `std::uint64_t` compared through
  `detail::exceeds`.** The comparison goes through `detail::exceeds` so that
  no comparison is tautological on wasm32; the types are the rulebook's rule
  of types of a fixed meaning.
  Source: `libraries/crypto/openspec/changes/genesis/specs/crypto/spec.md`
  "Interface: results are values" (proposals).
- **There is one public header per responsibility, the check that a backend is
  linked sits in that backend's own header, and `config.hpp`, `error.hpp`,
  `equal.hpp`, `random_device.hpp` and the convenience header compile with no
  backend linked.** A program that only draws random bytes or compares tags
  names no backend library, and one that only signs names
  `/third_party//secp256k1` and no encryption backend.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.2 (proposals).
- **`equal` compares in constant time: it reads every byte, accumulates the
  XOR of each pair with OR in a `volatile` local read once after the loop and
  reduces it to one bit without a branch, and spans of different sizes compare
  unequal without their contents being read.** Hash2's `digest<N>::operator==`
  is `memcmp`, not constant time; `equal` compiles in the user's translation
  unit, where a compiler could otherwise turn the reduction into an early
  exit; a size is public.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.6 (proposals).
- **`<windows.h>` is reached only through
  `<webcpp/crypto/detail/windows.hpp>`, which defines `NOMINMAX` and
  `WIN32_LEAN_AND_MEAN` only when they are absent and undefines only what it
  defined, and every header writes `(max)()`, `(min)()` and
  `(std::numeric_limits<T>::max)()`.** `<windows.h>` defines `min` and `max`
  as function-like macros that break any `max()` after it, in webcpp's headers
  and in the user's, and without `WIN32_LEAN_AND_MEAN` it brings `winsock.h`,
  which clashes with a later `winsock2.h` (Boost.Asio's).
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.2 (proposals).

**Authenticated encryption.**

- **`seal` and `open` work in place on `data`, with the tag apart: a
  `digest<16>` out of `seal`, and a dynamic span into `open`.** `data` is both
  input and output, so no buffer is copied and no overlap can be partial; a
  separate tag makes a wire format that appends it (trystero's, WebCrypto's)
  or sends it apart a matter of the caller's spans, and a dynamic span takes a
  `digest<16>` and a `std::array<std::uint8_t, 16>` alike, without a cast.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).
- **An AEAD copies the tag into a local array and consumes the IV and the
  additional data before it writes the first byte of `data`.** So that a tag
  that follows or overlaps `data` in one buffer, and an IV or additional data
  in the same buffer, give correct results.
  Source: `libraries/crypto/openspec/changes/genesis/specs/crypto/spec.md`
  "GCM: in place, the tag apart" (proposals).
- **Every function checks its conditions first, in a fixed order (key size,
  the memory option, moved-from, the incremental state, IV size, tag size, the
  length limits), and returns before it reads a secret byte; a guard's refusal
  leaves `data` untouched.** The rulebook's rule that a function starts with
  its guards, before any work begins.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).
- **A failed `open` zeroes the whole of `data`, by `volatile` stores, after
  any failure that follows the start of decryption (`authentication_failed`,
  `backend_failure`).** Every backend decrypts and checks the tag in one pass,
  and not all of them clear what they decrypted; verifying before decrypting
  would need a GHASH of webcpp's own over the backend's AES, the port the
  owner refused (a ruling of the controller).
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D5
  (proposals).
- **There is no incremental `open`; a later increment may add one only with
  the plaintext withheld until the tag verifies.** An incremental decryption
  returns plaintext before the tag is known, and nothing prevents its use when
  the tag later fails; no user needs streaming decryption, since trystero
  opens whole messages.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).
- **The incremental seal is `begin_seal(iv, additional_data)`, `update(data)`
  and `result()`, and any error in it ends the message and wipes its state.**
  Botan's public interface takes the associated data whole, before the nonce,
  so pieces of additional data could only be joined in a growing buffer, an
  allocation.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D6
  (proposals).
- **GCM accepts IVs of 1 to 128 bytes on every backend, and the documentation
  recommends 12.** OpenSSL accepts no GCM IV longer than 128 bytes (measured
  on 3.6.4) while BearSSL and Botan accept longer ones, so without a cap of
  webcpp's the same call would succeed or fail by backend.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D7
  (proposals).
- **Keys of 16, 24 and 32 bytes are accepted, AES-192 included, and the
  documentation says that a key meant for a browser's WebCrypto is of 16 or 32
  bytes.** AES-192 is part of the standard, every AEAD backend has it, and it
  lets every test vector run; Chrome's WebCrypto refuses it.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D8
  (proposals).
- **Tags are of 16 bytes only.** Trystero and WebCrypto's default use 16-byte
  tags, so NIST CAVP's cases with shorter tags are skipped.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.1 (proposals).
- **`gcm` refuses a message over 2^36 - 32 bytes and additional data over
  2^61 - 1 bytes, and `chacha20_poly1305` a message over 2^38 - 64 bytes, with
  `message_too_long`.** These are the limits of NIST SP 800-38D section
  5.2.1.1 and of RFC 8439.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 2.4 and 2.5 (proposals).
- **An AEAD object is used by one thread at a time, its functions are not
  `const`, and its page lists it as not shareable.** It holds the state of a
  message, as a Hash2 algorithm does, and a key; OpenSSL's and Botan's hold
  pointers into their libraries' heaps.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).
- **The documentation states that an IV or a nonce is never repeated under one
  key, and gives the limit of 2^32 messages per key with random IVs, for GCM
  and ChaCha20-Poly1305 alike.** A repeated IV exposes the XOR of the
  plaintexts and lets an attacker forge tags (NIST SP 800-38D section 8, RFC
  8439 section 4), and 2^32 random 96-bit nonces repeat with a probability
  inside the 2^-32 that NIST sets for GCM's random IVs.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 2.4 and 2.5 (proposals).
- **ChaCha20-Poly1305 is one shot only, with an IV of exactly 12 bytes, and
  where BearSSL leaves the tag check to its caller the backend compares with
  `equal` and zeroes `data` on a mismatch.** BearSSL's ChaCha20-Poly1305 is
  one function over the whole message, so an incremental form would not be
  uniform across backends; 12 bytes is RFC 8439 section 2.8; the comparison is
  BearSSL's documented contract, not a mode of webcpp's.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.5 (proposals).

**Block cipher.**

- **`aes128`, `aes192` and `aes256` are complete types for every backend,
  their `make`, `encrypt` and `decrypt` exist only where the backend exports
  the block cipher (ctaes, OpenSSL, Botan), and BearSSL's `make` stops with a
  `static_assert` that names the backends that do.** So that
  `gcm<aes256<backends::bearssl>>` can name its cipher; BearSSL exports AES
  only inside its modes.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.3 (proposals).
- **The block cipher has no default backend of its own: on WebAssembly a
  program names `aes256<backends::ctaes>` and includes its header.**
  `aes256<>` must stay the cipher of `aes256_gcm = gcm<aes256<>>`, which a
  second default would break.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.3 (proposals).
- **webcpp builds no GCM on another library's AES:
  `gcm<aes256<backends::ctaes>>` does not compile, with a message that names
  the backends that export GCM.** That would be the port the owner refused.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.4 (proposals).

**secp256k1.**

- **libsecp256k1 is the only secp256k1 backend, and its whole C API of v0.8.0
  (82 functions, every module built) stays reachable through
  `<secp256k1*.h>`.** OpenSSL and Botan have ECDSA on secp256k1 but no BIP340,
  and the owner named libsecp256k1 for this; what the C++ types do not wrap is
  used directly.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.8 (proposals).
- **`keypair::make` takes a `std::pmr::memory_resource*`, creates
  libsecp256k1's context in it with `secp256k1_context_preallocated_create`,
  aligned to `alignof(std::max_align_t)`, randomizes it with 32 bytes of
  `random_device`, and is not `noexcept`.** The resource serves the context
  alone, so that the counting test sees every allocation; libsecp256k1 asks
  for memory aligned to hold an object of any type; the resource reports a
  failed allocation as it does.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.8 (proposals).
- **`verify_schnorr` and `verify_ecdsa` use `secp256k1_context_static`,
  allocate nothing, and call `secp256k1_selftest()` on every call, keeping no
  flag.** libsecp256k1 recommends the static context only after its self-test,
  and a run-once flag would be the static mutable state the rulebook forbids;
  the self-test is one SHA-256 of a 63-byte string beside a verification of
  about 20 microseconds.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.8 (proposals).
- **`ecdh` takes the secret out of the key pair into a local array, wipes it
  before returning on every path, and returns the SHA-256 of the compressed
  shared point.** libsecp256k1's `secp256k1_ecdh` takes the secret key; the
  hash is libsecp256k1's default.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.8 (proposals).

**Errors and exceptions.**

- **Every operation that can fail returns `boost::system::result<T>` or
  `result<void>`, with exceptions on and off.** The owner's rule that every
  library builds and works with exceptions on and off.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D4
  (proposals).
- **The category `webcpp.crypto` has the fixed values 1 to 14, never reused,
  and the two generic conditions use `errc::not_enough_memory` and
  `errc::not_supported`.** Nothing of the draft of 2026-10-10 was released, so
  the values start afresh; the two conditions are generic.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.9 (proposals).
- **One function throws, `random_device::operator()`, declared only with
  exceptions and raising through `boost::throw_exception`; `keypair::make`
  lets the user's resource report a failed allocation as it does.**
  `std::random_device`'s shape has no other way to report a failure.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.10 (proposals).
- **`config.hpp` defines `WEBCPP_CRYPTO_NO_EXCEPTIONS` under
  `BOOST_NO_EXCEPTIONS`, keeps a developer's definition, and its Doc Comment
  names what depends on it: `random_device::operator()`, the Botan backend and
  the `try` of OpenSSL's memory adapter.** The rulebook's rule for exceptions;
  no further reason is recorded.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.10 (proposals).
- **Botan's backend calls Botan inside one `try` and maps what it throws:
  `Invalid_Authentication_Tag` to `authentication_failed`, `Invalid_IV_Length`
  to `invalid_iv_size`, `Invalid_Key_Length` to `invalid_key_size`,
  `std::bad_alloc` to `not_enough_memory`, anything else to
  `backend_failure`.** So that no exception leaves a webcpp function.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.6 (proposals).

**Backends.**

- **A backend is the C or C++ library itself, named after it
  (`backends::bearssl`, `openssl`, `ctaes`, `secp256k1`, `botan`), and never a
  port of it.** The owner refused a webcpp port of BearSSL's AES-GCM on
  2026-10-10: a proven library is audited and measured, a port is a new thing
  to audit.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D1
  (proposals).
- **BearSSL comes unmodified from webcpp's mirror `webcpporg/bearssl` at
  `7bea48e5`; no BearSSL file is copied into webcpp, its notice ships as
  `LICENSE-BEARSSL.txt`, and webcpp's canary lives in the library's `src/`,
  never in the mirror.** BearSSL is published only on its author's host, and
  the rulebook keeps an unmodified mirror for an upstream that is not on
  GitHub.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 3.2 and 3.4 (proposals).
- **The BearSSL backend names its code itself: for GCM the CTR class of
  AES-NI, then POWER8, then the 64-bit constant-time `aes_ct64`, and GHASH by
  PCLMUL, then POWER8, then `ghash_ctmul64`; for ChaCha20-Poly1305
  `br_chacha20_sse2_get()` else `br_chacha20_ct_run`, and
  `br_poly1305_ctmulq_get()` else `br_poly1305_ctmul_run`.** On wasm32 `BR_64`
  is not set, and BearSSL's own default there is the 32-bit code, half as fast
  (159 against 79 MiB/s).
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.2 (proposals).
- **A move of BearSSL's `gcm` copies the key schedule and calls `br_gcm_init`
  again on the destination, through the public API, abandoning a message in
  progress.** BearSSL's GCM context points at the key schedule
  (`br_gcm_context::bctx`).
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.2 (proposals).
- **OpenSSL is the machine's libcrypto, 3.0 or later, found and never built,
  native only, and its header stops with an `#error` on an older version.**
  Version 3 is needed for `EVP_CIPHER_fetch`, and no OpenSSL for WASI or
  Emscripten exists in webcpp's tree.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.3 (proposals).
- **OpenSSL is given the key once, in `make`, webcpp keeps no copy of it, each
  message re-initialises the same context with a null key, and lengths are fed
  in pieces of at most `INT_MAX` rounded down to a multiple of 16.** A null
  key keeps the expanded key, and a seal then an `open` on the reused context
  works (measured on 3.6.4); EVP takes lengths as `int`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.3 (proposals).
- **OpenSSL's `make` maps its errors by the call that failed: a null
  `EVP_CIPHER_CTX_new` is `not_enough_memory`, a null `EVP_CIPHER_fetch` or a
  failed `EVP_EncryptInit_ex` is `backend_failure`.** The error queue cannot
  tell a memory failure apart across versions: 3.6.4 raises `malloc failure`,
  3.0.13 raises nothing.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.3 (proposals).
- **The OpenSSL backend leaves the thread's error queue as it found it: `make`
  sets `ERR_set_mark()` after its guards and pops to it on every path, and the
  functions of a message set no mark and call `ERR_clear_error()` only on a
  `backend_failure`.** Nothing webcpp raises may be left for the program's
  other users of OpenSSL (Beast's TLS) to misread; an earlier mark would
  initialise OpenSSL and make the memory option's install impossible, and the
  first `ERR_*` call of a thread allocates its error state (6 blocks), which
  would break the rule that a message allocates nothing.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.3 (proposals).
- **ctaes is the copy Bitcoin Core ships, `src/crypto/ctaes` of
  `bitcoin/bitcoin` at tag v31.1 as a shallow submodule, built from `ctaes.c`
  alone and exposed as the block cipher alone, and not the repository
  `bitcoin-core/ctaes`.** The owner named "the ctaes of Bitcoin Core";
  `bitcoin-core/ctaes` is a different copy, which adds CBC.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D12
  (proposals).
- **libsecp256k1 is v0.8.0 built as its CMake build does, every module on, and
  its precomputed tables are the b2 feature `webcpp-secp256k1-tables` (`auto`,
  `small`, `large`), whose `auto` is small on Emscripten and large natively
  and on WASI.** The small tables make an Emscripten program 141,922 bytes
  against 1,211,251, for verification about a third slower, and the large ones
  verify 1.7 times faster on wasip2; the owner's mission makes Emscripten the
  browser's target and WASI the server's and the cloud's (a ruling of the
  controller).
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D11
  (proposals).
- **Botan 3 is found and never built, native only and only with exceptions:
  `backends/botan.hpp` declares nothing under `WEBCPP_CRYPTO_NO_EXCEPTIONS`,
  and a program built without exceptions that names it gets an `#error`.** No
  Botan for Emscripten exists that webcpp's build can find, Botan 3.13.0 has
  no OS definition for wasip2 or wasip3, and Botan reports every error by
  throwing, the tag check included.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D19
  (proposals).
- **Botan's backend calls `set_associated_data` before every `start`, with an
  empty span when there is none, and runs the bulk of a message through
  `process` on the caller's span, finishing on a scratch `secure_vector`
  reserved in `make`.** Botan keeps the additional data between messages, so a
  message without it would be authenticated with the previous one's; the
  reserved scratch never grows, so `seal` and `open` allocate nothing.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.6 (proposals).

**Default backend.**

- **The AEADs' default backend is BearSSL on Emscripten, wasip2 and wasip3,
  OpenSSL natively when it is linked, and BearSSL natively otherwise; with
  several linked it is the first of OpenSSL, BearSSL and Botan, and a program
  picks another with `<define>WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>`.** The
  owner's rule is the smallest binary on the web and the fastest on servers:
  OpenSSL is 1.4 times as fast as BearSSL's AES-NI on x86-64 and 30 to 62
  times on ARM; Botan comes last because it needs exceptions.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D9
  (proposals).
- **The architecture does not enter the choice of the default backend.**
  BearSSL picks AES-NI and PCLMUL by CPUID inside its own code.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D9
  (proposals).
- **The backends' page and `gcm`'s page warn that natively without OpenSSL
  BearSSL is 30 to 62 times slower than OpenSSL on ARM, and that a server on
  ARM names `/third_party//openssl`.** BearSSL has no ARMv8 AES or PMULL code.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 5 (proposals).

**Randomness.**

- **`random_device` is shaped as `std::random_device` (`result_type`
  `unsigned int`, `(min)()`, `(max)()`, `entropy()`, `operator()`), adds
  `fill(std::span<std::uint8_t>)` returning `result<void>`, holds no state and
  needs no backend.** So that it is a UniformRandomBitGenerator with
  exceptions on; without exceptions `operator()` has no way to report a
  failure, and `fill` is the interface.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.7 (proposals).
- **Entropy comes from one source per platform and nothing else: `getentropy`
  on Linux, macOS and WASI (and through it `crypto.getRandomValues` on
  Emscripten), and `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` on
  Windows, with no cache, no generator in user space and no weaker fallback.**
  The owner's decision of 2026-10-11: entropy always takes the safest path, in
  code simple enough to be read and checked by anyone.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D10
  (proposals).
- **The entropy function asks in pieces of at most 256 bytes, checks the
  result of every call, fits on one screen with each branch commented with the
  contract it relies on, and on failure zeroes the buffer and returns
  `entropy_unavailable`, or `errc::not_supported` where the environment has no
  generator.** 256 bytes is the limit of `getentropy`; trystero's browser
  backend ignores the result today, which is not repeated; an Emscripten audio
  worklet has no `crypto`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 6 (proposals).
- **A lint rule fails a header whose path ends `entropy.hpp` or
  `random_device.hpp` that names `rand`, `time`, `clock` or `__rdrand`, even
  with `lint-world:`.** No weaker source, ever: no clock, no `rand`, no
  `rdrand` alone.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 6 and 8.4 (proposals).
- **The backends draw from the same source: BearSSL is compiled with
  `BR_USE_GETENTROPY=1`, `BR_RDRAND=0` and `BR_USE_URANDOM=0`, with every
  seeder compiled out on Windows; libsecp256k1's context is randomized from
  `random_device`; webcpp never calls `RAND_bytes` or a Botan RNG.** No
  backend chooses its own entropy: on the x86-64 runner BearSSL had picked
  `rdrand` by itself, and its only Windows seeder is `CryptGenRandom`, not the
  platform's `BCryptGenRandom` (a ruling of the controller).
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 6 (proposals).

**Memory and wiping.**

- **webcpp's own code allocates only `keypair::make`'s context, in the
  resource the user gives; no seal, open, step of the incremental seal,
  `encrypt`, `decrypt`, `fill`, signature, `ecdh` or verification allocates on
  any backend; BearSSL, ctaes and libsecp256k1 keep their key schedules and
  contexts inside the webcpp object.** The owner's allocators decision:
  run-time objects route allocation through the user's allocator, and a
  counting test proves that none goes elsewhere; a backend that needs memory
  takes it in `make`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.11 (proposals).
- **OpenSSL's and Botan's `make` allocate their contexts through their own
  allocators, outside the user's, and each backend's page states it.** A
  departure the owner accepted on 2026-10-11: OpenSSL's only hook,
  `CRYPTO_set_mem_functions`, is process-global and so not a library's to set
  on its own, and Botan 3.13.0 has no hook (fixed functions with no setter, a
  stateless allocator, objects from the global `operator new`).
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D17
  (proposals).
- **The key schedules inside webcpp's objects (BearSSL's, ctaes's,
  libsecp256k1's key pair, `ecdh`'s local secret) are wiped on destruction and
  on move by stores through a `volatile` pointer followed by
  `std::atomic_signal_fence(std::memory_order_seq_cst)`; OpenSSL and Botan
  wipe their own.** The optimiser cannot drop these stores as dead, where it
  can drop a plain `memset`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 2.11 (proposals).

**OpenSSL's memory option.**

- **`<define>WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE`, native only, sends every
  allocation of OpenSSL in the process to a `std::pmr::memory_resource`; the
  program gives it with `<library>/third_party//openssl` in its project's
  requirements, and `config.hpp` refuses it on WebAssembly and without OpenSSL
  linked.** The owner's decision of 2026-10-11 made `CRYPTO_set_mem_functions`
  an option of the config, so that a program that wants OpenSSL's allocations
  in its own allocator can say so; the library beside the define keeps every
  target that gets the define linked with OpenSSL.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **Under the option the program defines
  `webcpp::crypto::openssl_memory_resource()` once, a `noexcept` function that
  returns a `std::pmr::memory_resource&`, and calls
  `backends::openssl::install_memory_functions()` as the first statement of
  `main`; webcpp never installs from a static initialiser.** OpenSSL's hook
  carries no user data, so any other form would keep the resource in a global
  variable of webcpp's, which the rulebook forbids, and Boost uses this form
  for `boost::assertion_failed` and `boost::throw_exception`; static
  initialisation order is unspecified and cannot return an error.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D16
  (proposals).
- **The program's resource is never destroyed (made with `new` and never
  deleted, or constructed with placement `new` in static storage), is
  thread-safe wherever OpenSSL may run on two threads, and is reached without
  an exception.** OpenSSL frees through it after `main`, and on Windows with a
  shared libcrypto after every static destructor of the program.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D16
  (proposals).
- **`install_memory_functions()` only reads OpenSSL's state until its guards
  pass: webcpp's adapter already in place is a success that changes nothing,
  another party's functions are `memory_functions_refused`, then the resource
  is constructed, then the adapter is installed, and OpenSSL's refusal is
  `memory_functions_refused`.** Replacing another party's functions would make
  OpenSSL free that party's blocks through webcpp's adapter; OpenSSL refuses
  an install once it has allocated with its own functions.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **Under the option every OpenSSL `make` checks that OpenSSL's functions are
  webcpp's adapter and otherwise returns `memory_functions_not_installed`, and
  a program that does not define `openssl_memory_resource()` fails to link on
  that name.** So that a forgotten install, or functions another party
  replaced after it, gives an error and not an allocation outside the
  resource.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **The adapter is three `extern "C"`, `inline`, `noexcept` functions of
  external linkage, and each block starts with a header that holds its size,
  aligned to the larger of `alignof(std::max_align_t)` and
  `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.** OpenSSL calls them as C functions, and
  the install and `make` recognise them by address, so every file of the
  program must see one function; OpenSSL's free passes no size, which
  `deallocate` needs; the alignment is what the system's `malloc` gives
  OpenSSL without the option.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **With exceptions each call into the resource sits in a `try` whose
  `catch (...)` returns null; without them there is no `try`, and a resource
  that throws ends the process.** OpenSSL is C, and no exception may cross it.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **OpenSSL's default functions are recognised by address, and on Windows also
  when the module that holds the address exports `CRYPTO_malloc` there
  (`GetModuleHandleExW`, `GetProcAddress`).** OpenSSL declares its functions
  without `__declspec(dllimport)`, so a program linked against the DLL takes
  its own import thunk, never the address OpenSSL holds.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).
- **On MSVC `backends/openssl.hpp` carries
  `#pragma detect_mismatch("webcpp_crypto_openssl_memory", ...)`, `"resource"`
  with the option and `"openssl"` without it.** Two files of one program
  compiled one with the option and one without, whose `make` differ, then fail
  with LNK2038.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.7 (proposals).

**Build.**

- **BearSSL, libsecp256k1 and ctaes are pinned submodules under
  `third_party/libs/`, built by `third_party/Jamfile` from their own sources,
  one object per file, as static libraries, on every target and with every
  file (BearSSL's POWER8 files included), in the compiler's GNU dialect, with
  no `-std`.** Neither BearSSL nor libsecp256k1 compiles as C++, which
  superseded the decision of 2026-10-10 to include one source once, as
  Boost.JSON's `src.hpp`; off POWER the POWER8 files compile to the stubs
  other files call, and an archive without them fails to link; built file by
  file, BearSSL's AES-GCM program is never larger in code than a unity build.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 1.3, 4.1 and 4.2 (proposals).
- **The C libraries are compiled with `<warnings>off` and
  `-ffunction-sections -fdata-sections` (`/Gy /Gw` on MSVC), each define a
  library cannot work out sits beside its sources with its reason, and the
  usage requirements give the include directory, `WEBCPP_CRYPTO_HAS_<NAME>=1`,
  the marked defines and the linker's removal of unused sections (`/OPT:REF`
  asked for on MSVC).** Upstream's warnings are not webcpp's; a section per
  function and datum lets a program keep only what it calls; `link.exe` leaves
  `/OPT:REF` off under `/DEBUG`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 4.1 (proposals).
- **A program names `/webcpp/crypto//crypto`, a header-only alias, and
  `/third_party//<backend>` for each backend it uses, and a library named and
  not called adds no byte, which the new rule `webcpp.same-program` proves.**
  Building the libraries inside the header-only alias was rejected because a
  program would link what it does not name; no rule of `tools/webcpp.jam`
  built a pair and compared it, and the lint refuses b2's raw `exe` in a
  library's Jamfiles.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D13
  (proposals).
- **The Jamroot registers the one third-party project under `/third_party` as
  well as `/webcpp/third_party`.** The user's form is `/third_party//<name>`,
  as the owner's decision and the rulebook write it, and
  `/webcpp/third_party//<name>` keeps working for the existing consumers (a
  ruling of the plan, C1).
  Source: `libraries/crypto/openspec/changes/genesis/reference/plan.md`
  "Conflicts with the spec" (proposals).
- **Three layers stop a wrong build, each saying exactly what to add: an
  `#error` or `static_assert` at compile time from `config.hpp` and the
  backend's header; a link-time canary, a symbol named
  `webcpp_crypto_<name>_<commit7>_<configuration>` followed by
  `__add_third_party_<name>_to_your_link`, that every function calling a C
  library reads and `src/canary_<name>.c` defines; and on MSVC
  `#pragma detect_mismatch`, one key per backend and one for the default
  backend.** A library missing, of another commit or configuration, or built
  without the canary then fails to link with that name, for one byte and one
  load; one key for all backends would fail a program that uses BearSSL and
  libsecp256k1 with a false mismatch; the default backend's key turns a silent
  ODR violation into LNK2038.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D14
  (proposals).
- **OpenSSL and Botan are found by `webcpp.external` (`-s<NAME>_ROOT`, then
  `.local/<name>`, then the system), declared in `libs/crypto/build.jam`; a
  user's program that names a missing one stops the build with three ways out,
  a WebAssembly build that names one stops before compiling, and only webcpp's
  own tests and examples, marked by the incidental feature `webcpp-own`, are
  left out.** A program that names `/third_party//openssl` has asked for it
  (the owner's decision of 2026-10-11); the index's "left out" reads every
  library's `build.jam`, while `third_party/Jamfile` loads only when a target
  names it.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 4.4 (proposals).
- **OpenSSL's link, shared or static, is the propagated feature
  `webcpp-crypto-openssl-link`, shared by default (`webcpp-crypto-botan-link`
  for Botan), and libssl and libcrypto follow it as one target.** A header
  cannot change how a program is linked, so the feature carries the owner's
  choice, which the owner confirmed on 2026-10-11; one target keeps a process from
  holding two copies of libcrypto.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` D15
  (proposals).
- **One copy per program: webcpp's build of a C library is never linked with
  another copy of it, trystero leaves its external libsecp256k1 in the change
  that names `/third_party//secp256k1`, and asio's OpenSSL moves onto
  `/third_party//openssl`.** libsecp256k1's tables are global symbols; a
  process with two copies of libcrypto has two memory hooks, and the install
  sets only the one webcpp's backend calls.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 3.5, 3.7 and 4.5 (proposals).
- **Every error case of the build has a directory of its own.** b2 shares one
  object file between two `exe` of one source that differ only by a free
  define or a `/third_party` library.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 4.3 (proposals).

**Targets.**

- **The library targets native (Linux x86-64 and arm64, macOS arm64, Windows
  with MSVC), wasip2, wasip3 and Emscripten, and each backend runs where its
  library exists: BearSSL, ctaes and libsecp256k1 on every target, OpenSSL and
  Botan natively only; a Botan WebAssembly build is a later increment's.** The
  rulebook's targets; no OpenSSL or Botan for WebAssembly exists that webcpp's
  build can find.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 3.1 (proposals).
- **On Windows `/webcpp/crypto//crypto` adds `bcrypt` to its usage
  requirements, and a static libsecp256k1 gives `SECP256K1_STATIC` to its
  users.** MSVC has no `getentropy`, so `random_device` calls
  `BCryptGenRandom`; the static library's declarations need the define.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 3.5 and 6 (proposals).

**Evidence.**

- **Every vector runs on every backend that has the algorithm, sealed and
  opened one shot and, for GCM, sealed in pieces of every size from 1 byte to
  the whole: the GCM specification's 18 cases, Wycheproof's AES-GCM and
  ChaCha20-Poly1305 files, NIST CAVP's cases with 128-bit tags, RFC 8439, and
  FIPS-197 and AESAVS for the block cipher.** All 18 cases run because keys of
  24 bytes and IVs of 8 and 60 bytes are accepted; the three Wycheproof cases
  with a 257-byte IV are expected refusals, by the IV cap.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.1 (proposals).
- **secp256k1 is proven by BIP340's `test-vectors.csv`, Wycheproof's Bitcoin
  ECDSA file through `verify_ecdsa`, Wycheproof's ECDH files through
  libsecp256k1's C API, and two parties agreeing on `ecdh`.** Wycheproof's
  general ECDSA file gives DER signatures and counts high-S ones valid, and
  its ECDH file gives the raw x-coordinate as the secret, neither of which
  fits the interface.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.1 (proposals).
- **Every backend is compared with every other on the same inputs from a
  seeded generator, and on x86-64 BearSSL runs once on AES-NI and PCLMUL and
  once forced onto its portable code.** So that a failure replays and both of
  BearSSL's code paths are compared; on WebAssembly, where one backend exists,
  the vectors and WebCrypto stand in for a second.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.2 (proposals).
- **WebCrypto in node and in Chrome is an outside oracle, random messages
  under 16-byte IVs included.** Those are the IVs trystero exchanges with
  Trystero's JavaScript peers.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.1 (proposals).
- **Constant time is checked deterministically on Linux x86-64 and arm64
  (Valgrind memcheck and MemorySanitizer, each harness failing on a planted
  leak, and libsecp256k1's `ctime_tests`), and statistically by dudect's
  method natively and under wasmtime.** `equal` compiles in the user's
  translation unit, so it is checked in a test program at -O2 and -Os; under
  wasmtime the engine's compiler is what matters for BearSSL's `bmul64`, which
  assumes a constant-time 64-bit multiplication.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 3.2 and 8.3 (proposals).
- **The C libraries' own test programs run on webcpp's build on every target,
  in the own lane `c-suites`, outside the ordinary lanes.** The rulebook's
  rule for a C library asks it; BearSSL's 88 tests take 28 to 46 seconds per
  target.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 8 and 8.1 (proposals).
- **Allocation is proven by a counting `std::pmr::memory_resource`, a counting
  global `operator new` and, for OpenSSL, counting hooks and an interposer on
  the C library's allocator, and each test of the memory option is a program
  of its own.** OpenSSL's functions are the process's and cannot be put back
  once allocated through; what Botan's `make` allocates, and OpenSSL's without
  the option, is counted and reported, not refused.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  section 8.5 (proposals).
- **The test Jamfile's rule `crypto-run` declares every test program twice,
  the second with `<exception-handling>off` and the suffix `_no_exceptions`,
  and every test `main` ends with
  `return webcpp::test::exit_status(boost::report_errors());`.** No lane of
  the matrix builds without exceptions; under node `exit_status` bounds each
  test and turns a `main` that never finished into a failure.
  Source: `libraries/crypto/openspec/changes/genesis/reference/plan.md` C15
  and "The base" (proposals).
- **Every message (each `#error`, `static_assert`, canary, b2 stop and
  run-time error) comes from a test of its own that checks its exact text, and
  the user's Jamfiles are fixtures of `tools/test/crypto_jamfiles_test.py`.**
  The lint refuses b2's raw `exe` in a library's test and example Jamfiles (a
  ruling of the plan, C10).
  Source: `libraries/crypto/openspec/changes/genesis/reference/plan.md`
  "Conflicts with the spec" (proposals).
- **The heavy own lanes (`c-suites`, `constant-time`, `browser`, `throughput`)
  run at a nightly level the plan builds, and `sizes` on every push.**
  BearSSL's own suite alone takes 28 to 46 seconds per target; the owner's CI
  levels put the heavy lanes at night (gate O2).
  Source: `libraries/crypto/openspec/changes/genesis/design.md` Risks
  (proposals).
- **Sizes are measured at b2's `speed` and `space`, and throughput by webcpp's
  own program on ubuntu-24.04, ubuntu-24.04-arm and macos-15 (departures C17
  and C16, for the owner).** b2 has no -O2 that reaches the libraries a
  program names, and no tree holds the workflow of the run the specification
  named.
  Source: `libraries/crypto/openspec/changes/genesis/reference/plan.md`
  "Departures for the owner (O5)" (proposals).
- **NIST CAVP's archives are pinned by URL and SHA-256 and fetched into
  `.local/nist-cavp`, never tracked, and BIP340's vectors come from a shallow
  submodule `third_party/libs/bips`.** NIST publishes the archives as zip
  files on csrc.nist.gov only, and the rulebook's mirror is for C libraries,
  not test data; BIP340's file lives only in `bitcoin/bips`.
  Source: `libraries/crypto/openspec/changes/genesis/reference/plan.md` C11
  and C12 (proposals).

**Documentation.**

- **The documentation has a page on the user's Jamfiles and a page per
  subject, gives per backend its Jamfile line, its headers and functions and
  every message with its fix, builds and runs every Jamfile line and printed
  result as a fixture, and gives every public symbol a Doc Comment.** The
  rulebook asks it, so that an AI agent writes a correct Jamfile at once.
  Source: `libraries/crypto/openspec/changes/genesis/reference/spec.md`
  sections 3.1 and 7 (proposals).

**Migration.**

- **Increment 3 moves webcpp::trystero onto the library: `aes256_gcm` with its
  16-byte IVs replaces its native and browser sealers, `random_device::fill`
  its `random`, `secp256k1::keypair` and `/third_party//secp256k1` its signer
  and external libsecp256k1, and its direct use of OpenSSL goes.** Errors are
  values, so a system without entropy fails the operation that needed the
  bytes with `entropy_unavailable` where trystero aborts today; one copy of
  libsecp256k1 per program.
  Source: `libraries/crypto/openspec/changes/genesis/design.md` Migration Plan
  (proposals).

**Open questions for the owner.**

- The base: the owner's rewrite of `main`'s history, after which Task 0a
  integrates `infra/shared`; nothing of the plan starts before.
- Gates G1 and G2: the owner creates `webcpporg/crypto` and adds it as the
  submodule `libs/crypto`.
- Gate O2: the heavy own lanes at the CI's nightly level (recommended), built
  by Task 25b.
- Gate O3: NIST CAVP's archives pinned by URL and SHA-256 and fetched into
  `.local/nist-cavp` (recommended), or a mirror repository.
- Item O5: the departures C14, C16 and C17, and `brew install botan` on the
  development machine.
- Whether webcpp::webrtc and webcpp::beast use this library, and how.
- A later increment: an incremental `open` with the plaintext withheld until
  the tag verifies, an incremental ChaCha20-Poly1305, and a Botan WebAssembly
  build.

Source: `libraries/crypto/openspec/changes/genesis/design.md` Open Questions
(proposals).

### wasi redesign (accepted 2026-10-09, not built)

The owner reviewed wasi 0.1.0 on 2026-10-08 and approved this redesign on
2026-10-09; a spike measured it with wasi-sdk 34, wasmtime 47.0.3 and
wit-bindgen 0.62.0, and nothing of it exists in code. Where the spike and the
owner's design differ, the design wins.

**Purpose.**

- **wasi exists to hide the `wasi:http` bindings, resource handles with a
  different API on each version, behind a function that returns a plain
  `response`, keeping every guarantee of its page.** wasi-libc gives wasip2
  and wasip3 the same POSIX part (files, sockets, clocks, stdio) and nothing
  for `wasi:http`; the owner confirmed this purpose.
  Source: `libraries/wasi/openspec/changes/redesign/reference/design.md`
  (proposals).
- **The redesign changes no verdict of 0.1.0's handler (the status range,
  the content type, the chunks, the trailers), and every served test keeps
  its `.requests` and `.expected`.** Every guarantee of the page is kept, and
  the 0.1.0 transcripts prove it on the new layout.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).

**Dependencies.**

- **wasi depends on Boost and webcpp::config like every webcpp library: its
  `config.hpp` includes `<webcpp/config.hpp>` first, the target `wasi` brings
  `/webcpp/config//config`, and the by-hand build names the include paths of
  Boost and config.** The claim "wasi needs no Boost" was inherited from the
  README of the project wasi was written for, and is removed.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **`WEBCPP_WASI_NO_EXCEPTIONS` follows `WEBCPP_HAS_EXCEPTIONS` by
  webcpp::config's two lines, and wasi no longer reads the compiler's macros
  itself.** `<webcpp/config.hpp>`, included first, says whether webcpp has
  exceptions.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).

**Versions and targets.**

- **Each version of WASI has a namespace, `webcpp::wasi::p2` and
  `webcpp::wasi::p3`, in `wasi/p2/` and `wasi/p3/`, and the configured one is
  `inline`, as Boost.Process's `v1` and `v2`.** `webcpp::wasi::http::...`
  then names the configured target's, and the two versions read apart.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **A version holds only the feature namespaces it provides, today `http`
  alone; there is no `filesystem`, `sockets`, `clocks` or `environment`
  namespace.** As Boost.Asio's `posix` and `windows` exist only where the
  platform has them; files, sockets, clocks and the environment go through
  the same wasi-libc on both versions (measured).
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **What both versions share (`response`, `request`, the common internals)
  lives in `webcpp::wasi::detail` and appears in each version's `http` by
  `using`; a WASI build has no `webcpp::wasi::http` outside the versions.** A
  public `wasi::http` beside the inline version's would make every lookup of
  it ambiguous.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **Each version's handler is in a header of its own,
  `<webcpp/wasi/p2/http.hpp>` and `<webcpp/wasi/p3/http.hpp>`, with no `#if`
  in its logic; the one choice between them is made in
  `<webcpp/wasi/http.hpp>`.** 0.1.0's `entrypoint.hpp` interleaved the two
  handlers with `#if` in about 500 lines, and could not be read a version at
  a time.
  Source: `libraries/wasi/openspec/changes/redesign/proposal.md`
  (proposals).
- **The compiler chooses the version: `config.hpp` defines
  `WEBCPP_WASI_TARGET_P2` or `_P3` from `__wasip2__` or `__wasip3__`, and the
  build defines nothing.** The compiler already knows the target, and a
  program compiled by hand gets the version b2 would give it.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).
- **`WEBCPP_WASI_TARGET`, `p2` or `p3`, overrides the compiler's choice, as
  `BOOST_PROCESS_VERSION` does, and any other value stops with the one error
  `WEBCPP_WASI_TARGET is p2 or p3`.** The spike measured a p3 handler on the
  wasip2 libc and a p2 handler on the wasip3 libc both serving, so the
  override is a real choice.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **wasip1 and every other WebAssembly target stop at webcpp::config's error
  `unsupported target`, before wasi's own block is read.** (reason not
  recorded)
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).

**Interface.**

- **A program writes an ordinary function, `response fn(request const&)`,
  and names it once, at global scope, with
  `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)`; `WEBCPP_WASI_HTTP_MAIN_BEGIN`, `_END`,
  `WEBCPP_WASI_HTTP_EXPORTS` and `http_main` are removed.** The function has
  no ABI in its type, so it compiles natively and a native test calls it;
  0.1.0's main, defined inside macros that need the component ABI, ran only
  in a served component.
  Source: `libraries/wasi/openspec/changes/redesign/proposal.md`
  (proposals).
- **The function has no name the library fixes, may live in any namespace
  and may be defined in another translation unit than the macro.** The
  macro takes the function's qualified name, so the body is no longer inside
  a namespace of wasi's.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).
- **`WEBCPP_WASI_P2_HTTP_ENTRY_POINT(fn)` and
  `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(fn)` define each version's export, and
  `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` only forwards to the configured one.**
  A program can name a version whatever the configuration says, and the
  macros' names mirror the namespace path, as Boost's do, since the
  preprocessor has no namespaces.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **`request` is an aggregate of `std::string_view method` and
  `std::string_view target`, views valid during the call.** It carries what
  0.1.0's main received; headers and a body are a later extension.
  Source: `libraries/wasi/openspec/changes/redesign/reference/design.md`
  (proposals).
- **One source is built once per target into one component per target, each
  exporting one version's handler; a component with both exports is not
  built.** Measured: wasmtime always answers through the p3 export, a host
  without p3 cannot load the component, Spin 4.2.2 refuses two handlers, it
  grows 24% (16.5 KB stripped), and one module linked against both libcs
  fails in wasm-ld.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **One world per target keeps wit-bindgen's plain names and the two
  standard worlds of 0.1.0; no interface is renamed with `--rename`.**
  `--rename` served only the dual world, which is rejected.
  Source: `libraries/wasi/openspec/changes/redesign/reference/STATUS.md`
  (proposals).
- **Derived by the proposal, to be confirmed (O1): on a build with no WASI
  version, `<webcpp/wasi/http.hpp>` declares `webcpp::wasi::http` with
  `response` and `request` alone and defines the entry-point macro as
  nothing.** The program's function then compiles and runs natively.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **Derived by the proposal, to be confirmed (O2): the ABI-free `response`
  and `request` are the public headers `<webcpp/wasi/detail/response.hpp>`
  and `<webcpp/wasi/detail/request.hpp>`.** The alternative,
  `<webcpp/wasi/http/response.hpp>`, suggests a public `wasi::http` outside
  the versions, which the design refuses.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **The handler keeps no state, as 0.1.0's did.** A wasip3 instance and its
  globals outlive one request: wasmtime reuses it 128 times by default.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).

**Capabilities.**

- **`config.hpp` defines capability macros from the compiler's own macros,
  as Boost.Asio's `BOOST_ASIO_HAS_*`: they say what compiles, and what runs
  is the host's grant, which the page tables (`--dir`, `-S inherit-network`,
  `allow-ip-name-lookup`, `--env`).** A program can ask what the target
  offers; a grant belongs to the command that runs the program, not to a
  macro.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **`WEBCPP_WASI_HAS_SOCKETS` is `__wasip2__ || __wasip3__`.** wasip1 has no
  `<netdb.h>`, and sockets compile and run on both versions behind
  `-S inherit-network` (measured).
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).
- **`WEBCPP_WASI_HAS_THREADS` is `__wasi_cooperative_threads__`, never
  `__STDCPP_THREADS__`.** No thread runs on either version: wasip3
  predefines `__STDCPP_THREADS__` and links `std::thread`, which fails at run
  time with ENOTSUP (measured).
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **`WEBCPP_WASI_HAS_EXCEPTIONS` is `__wasm_exception_handling__`, never
  `__cpp_exceptions`.** wasm defines `__cpp_exceptions` without
  `-fwasm-exceptions`, a build that then fails to link.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).
- **Three capability macros are kept; the spike's
  `WEBCPP_WASI_HAS_FILESYSTEM` and `WEBCPP_WASI_HAS_PROCESS_CLOCKS` are
  not.** Files, clocks, random, the environment and stdio are on every WASI
  target; why the process clocks get no macro is not recorded.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).

**Run scripts.**

- **b2 writes a POSIX `sh` script, `<program>.sh`, beside every `.wasm` it
  builds for wasip2 or wasip3, by a rule of the build system that every
  library's programs get.** 0.1.0's programs ran through a wasmtime command
  line that the test harness composed and nobody else could copy, its flags
  differing by kind, target and exception setting.
  Source: `libraries/wasi/openspec/changes/redesign/proposal.md`
  (proposals).
- **The script finds wasmtime in `$WASMTIME` or on `PATH` (exit 127 and a
  message otherwise), adds `$WASMTIME_FLAGS`, finds the `.wasm` beside
  itself and `exec`s wasmtime.** The pair can be copied anywhere together,
  and a signal sent to the script reaches wasmtime, which the harness's
  keeper relies on.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).
- **The script states the explicit flags of the artifact
  (`-W exceptions=y`, `-S p3 -W component-model-async`), not the minimal ones
  of wasmtime 47.** The line then runs on the oldest wasmtime that can run
  the artifact: wasmtime 37 to 46 need `-W exceptions=y`, and WASI 0.3 needs
  46 or later.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **A served component's script takes an optional first argument that does
  not start with `-` as the address, `127.0.0.1:8080` by default, and passes
  the rest to `wasmtime serve`.** (reason not recorded) The spike measured
  that an option first keeps the default address.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).
- **`serve.py` and `run_example.py` run the programs through their scripts;
  b2's `webcpp.run` tests may keep `testing.launcher=wasmtime`.** The CI then
  tests the scripts, and the minimal line of a `run` test is the same on
  wasmtime 47.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).
- **The rule lives in `tools/component/component.jam`, where it replaces
  `.wasmtime-flags.p2` and `.p3` as the one place that knows wasmtime's
  flags, and declares a script for the WASI toolsets alone (`<build>no`
  elsewhere, read from the toolset and its version).** A native build
  declares no script and looks for no wasmtime.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).
- **The generator quotes `"on"`, builds each line as a list of words, and
  writes the script's `$(` through a variable.** Three b2 facts the
  prototype hit: `on` is a jam keyword, a string that holds an empty list
  vanishes whole, and `$(` in a string is b2's.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).
- **No `.cmd` script is written until a Windows WASI lane exists.** No WASI
  lane runs on Windows, and a script is for the machine that built it.
  Source: `libraries/wasi/openspec/changes/redesign/reference/spike.md`
  (proposals).

**Evidence.**

- **New tests hold the new layout: `target_namespace`,
  `target_override_p2`, `target_override_p3`, `target_refused`,
  `capabilities`, `request`, `function_native` and the served
  `http_explicit`; `target_refused` replaces the two refusals of 0.1.0.** The
  version no longer comes from a macro the build defines, so the refusals of
  neither and both are false.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).
- **A check of the run scripts proves them generated, clean under
  `shellcheck -s sh`, run by `dash` and from a path with a space, and exiting
  127 without wasmtime; shellcheck and dash are looked for by this check
  alone.** (reason not recorded) The spike ran each of these on its
  prototype.
  Source:
  `libraries/wasi/openspec/changes/redesign/specs/wasi/spec.md`
  (proposals).

**Process.**

- **The redesign is a breaking change made before webcpp's first release,
  after webcpp::config exists, with a mechanical migration listed in the
  changelog.** The breaking change then costs no user, and wasi needs
  `<webcpp/config.hpp>`.
  Source: `libraries/wasi/openspec/changes/redesign/proposal.md`
  (proposals).
- **The run-script rule is code of the superproject's tools, specified in
  wasi's specification for now.** The design places it in the redesign and
  wasi's tests exercise it; where it belongs is O4.
  Source: `libraries/wasi/openspec/changes/redesign/design.md` (proposals).

**Open questions for the owner.**

- **O1.** What does `<webcpp/wasi/http.hpp>` do on a build with no WASI
  version (native, Emscripten)? Recommended and specified: it declares
  `webcpp::wasi::http::response` and `request` and defines
  `WEBCPP_WASI_HTTP_ENTRY_POINT` as nothing, so the function compiles and a
  native test calls it.
- **O2.** Are the shared types public headers under `detail/`
  (`<webcpp/wasi/detail/response.hpp>`)? Recommended: yes, since they are
  the ABI-free headers; the alternative is `<webcpp/wasi/http/response.hpp>`,
  which suggests a public `wasi::http` outside the versions that the design
  refuses.
- **O3.** Host grants as a feature of the build (`<wasi-grant>filesystem`,
  `network`, `environment`), and `install` listing both the `.wasm` and its
  `.sh`, as the spike outlines? Recommended: later; the design asks for the
  flags of the kind and target, and wasi's tests need no grant.
  `WASMTIME_FLAGS` serves meanwhile.
- **O4.** Where is the run-script rule specified? It is code of the
  superproject's tools, shared by every library; this change specifies its
  behaviour in wasi's specification because the design places it in the
  redesign and wasi's tests exercise it. Recommended: the organization's
  specification records the rule, and wasi's refers to it.

### The site, increments 4 and 5 (accepted 2026-10-10, not built)

The open change `site-increments-4-5`: trystero's JavaScript package and the
collaborative cursors (increment 4), then the state machine, on the server,
and C++ calls the browser (increment 5). None of its plan's Tasks 20 to 35 is
built. While it is open, its `design.md` holds it in full and rules over this
summary.

**The package.**

- **trystero's JavaScript face is the package `@webcpp/trystero`, in
  `libs/trystero/js/` and built by a trystero target; the site serves it
  unbundled at `packages/trystero/`, and an example page reaches it through an
  import map.** The page then runs the very bytes a JavaScript developer gets;
  a copy built into the site was rejected, since it would not be a library
  artifact.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **The package's surface is Trystero 0.26.0's, and every member of Trystero's
  configuration and options it does not implement throws a `TypeError` naming
  it.** None is accepted in silence: a relay-only ICE policy ignored would
  expose the address it asks to hide.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **Every event is copied out of the wasm memory and delivered on a microtask,
  and `observe`, webcpp's one addition to the surface, hands the site's
  visualizer copies of each crossing.** As in Trystero, a handler may then
  leave its room or send at once.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **The package is built on trystero's shared connection, `app`, alone: one
  `selfId`, and one connection per peer across the rooms of an app.** Trystero
  does so, and webcpp::trystero does so only through `app`.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **trystero gains `raw_payload`, an action type that hands a message's format
  and bytes undecoded.** Without it a JSON message would be parsed in C++ and
  serialized again, which can change its text (`1.0` becomes `1`).
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).
- **The package is `@webcpp/trystero` 0.1.0, `"private": true` until the owner
  publishes it on npm; until then b2 builds it, the site serves it and no page
  shows `npm install`.** The name and the version are gate G5, decided on
  2026-10-11; publication is gate G6, still open.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).

**The examples.**

- **The collaborative cursors run Trystero's own 24-line sketch byte for byte
  but its import line, with its provenance archived and its SHA-256 held by a
  test, and the three helpers the sketch leaves to the application are the
  site's.** trystero.dev publishes the sketch in no repository and its page
  states no licence, so a wrong copy must be detectable, and the copy relies
  on the owner's ruling that everything of Trystero is MIT; the helpers treat
  a peer's messages as hostile.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **No page connects anywhere on load: an example reaches the public Nostr
  relays and STUN servers only on the reader's Join, under a notice, and no
  build or test reaches the public network.** The relays and STUN servers see
  the reader's address, as peers do; in tests Chrome resolves no host but
  127.0.0.1, a canary proves it, and the relay is the interop lane's.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **A live example's surface is a page of its own, a `Frame`, shown in an
  `<iframe>` of the example page, `noindex` on both channels and without a
  card.** The visualizer then sits beside it and plays the crossings the
  package reports.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **The state-machine example runs the site's own machines on webcpp::xstate,
  and XState's JavaScript is the oracle of the tests alone.** The site's state
  machines run on webcpp::xstate, not on XState's JavaScript (the owner's
  decision 10), so "the browser's own state" is the state React holds from the
  snapshot the C++ machine returned.
  Source:
  `organization/openspec/changes/site-increments-4-5/reference/plan.md`
  (proposals).
- **On the server, one handler, `answer(method, target)`, is called by two
  mains, a WASI component and a native host of the example's own on
  Boost.Beast, and both answer one committed transcript in the site's own lane
  `http` on native, wasip2 and wasip3.** webcpp::wasi has no native host (one
  comes with webcpp::beast), and `b2 site` has neither wasi-sdk nor wasmtime,
  so the page's terminal sessions are built from the committed transcript,
  never typed.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **C++ calls the browser is a program on webcpp::asio, scheduled last, with
  no version on `emscripten::val` alone.** The owner's decision makes
  webcpp::asio the way the browser's promises and events reach C++, so the
  example waits for the asio track's publication.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).

**The site's build.**

- **Every `b2 site` measures each port of a JavaScript original beside the
  original, raw, gzip and brotli, into `bin/site/sizes.json`, never committed,
  and a check holds every size a page shows to that measurement.** A committed
  size file would couple the site's repository to every measured library and
  to the build machine: any byte a library's wasm moves would turn `main` red
  until the site regenerated it. It is the recommendation for gate O5.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **A Jamfile under `site/examples` may declare own lanes, which the CI runs
  as a library's, and the site's programs never reach `b2 declared-targets`.**
  No ordinary lane is then planned for them.
  Source: `organization/openspec/changes/site-increments-4-5/design.md`
  (proposals).
- **The site's JavaScript path keeps working with npm alone: a test or type
  check that needs the package skips without b2, saying why and how to get it,
  and fails under b2, which requires it.** The owner's decision of 2026-10-10,
  two ways into the site's repository, keeps npm alone a way in, and a check
  that needs the package never passes in silence.
  Source:
  `organization/openspec/changes/site-increments-4-5/reference/plan.md`
  (proposals).

**Preconditions.**

- **The site's increments 2 and 3, Tasks 12 to 18 of the earlier site plan,
  are in the site's `main`, and Task 19 has reviewed and published them.** Not
  met: Tasks 12 to 15 (xstate on emscripten, the site's wasm, the PWA, the
  trace) exist only as unpublished patches under the change's `reference/`,
  and Tasks 16 to 19 (the visualizer, the calculator, the browser lane, the
  review) are not built. Tasks 26 to 30 and 32 wait on them.
  Source: `organization/openspec/changes/site-increments-4-5/tasks.md`
  (proposals).
- **trystero's shared connection, `app`, is in trystero's `main`, and the
  superproject's `main` points to it.** The change's `tasks.md` and
  `STATUS.md` record it as missing, but the superproject's `main` now records
  trystero at `77fed8f`, whose history holds the shared connection
  (`2e2dcd4`): it appears met, for Task 20 to confirm. Tasks 22 to 26 and 28
  wait on it.
  Source: `organization/openspec/changes/site-increments-4-5/tasks.md`
  (proposals); `libs/trystero` (webcpp).
- **webcpp::asio's Tasks 2 and 3 (`/webcpp//node-test`, `webcpp.variants`) are
  in the superproject's `main`, and for Task 33 `libs/asio` is published as a
  submodule.** Not met: `main` has neither rule and no `libs/asio` submodule.
  Task 23's Steps 1 and 2, Task 31 and Task 33 wait on them.
  Source: `organization/openspec/changes/site-increments-4-5/tasks.md`
  (proposals).
- **`/webcpp//boost_url` is in the superproject's `main`.** Met: the
  on-the-server example links it rather than compile Boost.URL's sources
  itself.
  Source: `organization/openspec/changes/site-increments-4-5/tasks.md`
  (proposals).

**The owner's gates.**

- **G4: trystero's JavaScript API is approved, naming `observe`, the refusal
  list and the delivery on a microtask.** Open; Tasks 22 to 25 wait on it.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals);
  `organization/openspec/changes/site-increments-4-5/reference/STATUS.md`
  (proposals).
- **G7: the public network from the site, (a) Trystero's app id and room, (b)
  a webcpp app id, or (c) no live Join, only a recorded trace.** Open, with
  (a) recommended; the push that publishes the cursors page carries the option
  the owner chose, never (a) by default.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).
- **O5: the sizes measured by every build, or a committed file.** Open, with
  the measurement by every build recommended; Task 27's `--check` and its
  Steps 4 to 6 wait on it.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals);
  `organization/openspec/changes/site-increments-4-5/reference/plan.md`
  (proposals).
- **G6: the publication of `@webcpp/trystero` on npm.** Open; the
  recommendation is from the CI alone, with npm provenance, under an
  organisation enforcing two-factor authentication, after one review cycle on
  the preview.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).
- **G2: the push of trystero, the site and the superproject after each
  increment's review.** The owner's act, as every outward act is.
  Source:
  `organization/openspec/changes/site-increments-4-5/specs/site/spec.md`
  (proposals).

## 18. Proposed, not decided

The proposals not yet decided, each briefly, with the questions they leave
for the owner. A draft decides nothing: it is here so that a contributor sees
what is coming, and its proposal holds it in full.

### beast (draft, not decided)

webcpp::beast would be a header-only C++20 wrapper over Boost.Beast and
webcpp::asio, for native and Emscripten only: Boost.Beast itself natively, and
in the browser the same names and layered code driving `WebSocket`
(increment 1) and `fetch` (increment 2). What exists is the draft
specification of 2026-10-10 (`reference/spec.md`) with four decisions of the
owner of that day; there is no plan and no reference code, and both are
required before the proposal can be accepted.

- **webcpp::beast is a library of its own on webcpp::asio, not a part of it:
  the WebSocket client first, then the HTTP client in the browser.** Beast is
  a library on Asio, so webcpp::beast is a library on webcpp::asio (the
  owner's decision 1 of 2026-10-10).
  Source: `libraries/beast/openspec/changes/genesis/design.md` D1 (proposals).
- **Natively it is Boost.Beast (aliases of Beast's types, and a
  `websocket::stream` derived from Beast's that adds an allocator
  constructor); the browser classes exist only on Emscripten; every name that
  touches no network is Beast's own on both targets.** Natively there is no
  cost and no difference: Beast's own types, one derived class and two
  functions; the derived stream gives the two targets one constructor set.
  Source: `libraries/beast/openspec/changes/genesis/design.md` D2 (proposals).
- **What the browser cannot honour fails as a value,
  `boost::asio::error::operation_not_supported` at the operation that depends
  on it, under the default `WEBCPP_BEAST_UNSUPPORTED_ERROR`;
  `WEBCPP_BEAST_UNSUPPORTED_IGNORE` ignores it; every setting is classified
  honoured, unsupported, or ignored with its reason.** Silent divergence was
  rejected: a program that asks for a client certificate or a ping must learn
  so (the owner's decision 2).
  Source: `libraries/beast/openspec/changes/genesis/design.md` D3 (proposals).
- **On Emscripten the resolver is webcpp::asio's: it completes with one entry
  that carries the host and the service and an unspecified endpoint, which
  only webcpp::beast's browser streams accept.** A resolver of webcpp::beast's
  own, with webcpp::asio's failing like its sockets, was rejected: every Beast
  client would change a line (the owner's decision 4).
  Source: `libraries/beast/openspec/changes/genesis/design.md` D4 (proposals).
- **The layers record and the last one opens: `async_connect` records the host
  and service, the TLS handshake judges the recorded settings, and
  `async_handshake(host, target)` calls `new WebSocket(url, subprotocols)`.**
  It is the only way to keep Beast's code shape, because the browser has one
  call that does resolution, connection, TLS and the upgrade together.
  Source: `libraries/beast/openspec/changes/genesis/design.md` D5 (proposals).
- **Back-pressure is the library's: a bound on `bufferedAmount` (default 4
  MiB) fails a write with `error::send_limit`, and a bounded receive queue in
  the stream's allocator closes the stream past its bound, the next read
  completing with `error::receive_limit`.** The browser's `send` never blocks
  and messages arrive whether a read is pending or not, and the goal is
  bounded memory in the browser; `WebSocketStream`, which would pause
  receiving, exists in Chromium only and doubles the surface, so it is a later
  increment.
  Source: `libraries/beast/openspec/changes/genesis/design.md` D7 (proposals).
- **The HTTP client reuses Beast's algorithms by making the browser
  `tcp_stream` a byte stream: the bytes Beast's serializer writes are parsed
  back, sent as one `fetch` per whole request, and the response is serialized
  into the bytes the reads yield.** webcpp::beast then overloads no HTTP
  algorithm; the cost is one parse and one serialization per message, for
  Beast's code unchanged.
  Source: `libraries/beast/openspec/changes/genesis/design.md` D8 (proposals).
- **The unsupported setting is read once, in `<webcpp/beast/config.hpp>`, both
  macros defined is an `#error`, a propagated b2 feature
  `webcpp-beast-unsupported` sets it for a whole program, and the browser
  classes live in an inline namespace named after the setting.** The two
  macros change inline code, so translation units that disagree would break
  the one definition rule; in the inline namespace they name different types
  and fail at link, never silently.
  Source: `libraries/beast/openspec/changes/genesis/reference/spec.md` section
  7 (proposals).

**Open questions for the owner.**

- O1, the header layout: the public headers and the convenience header, and
  whether `<webcpp/beast.hpp>` leaves `ssl` out natively, as webcpp::asio's
  convenience header does because it brings OpenSSL.
- O2, the default bound of the receive queue (the send bound is 4 MiB) and the
  names of the two options, to measure and decide in the plan.
- O3, how the decorator is called on an empty `websocket::request_type`:
  whether any field other than `User-Agent` can be told apart by name alone,
  or every field but the subprotocols is unsupported, as the specification
  says (to confirm with a test).
- O4, streaming request bodies: the specification buffers a body whole before
  the `fetch`; a streaming upload (Chromium only) belongs to a later change.
- O5, the proposal to Boost, made when both webcpp::asio and webcpp::beast
  have run in real programs, trystero first.

Source: `libraries/beast/openspec/changes/genesis/design.md` Open Questions
(proposals).

### webrtc (draft, not decided)

webcpp::webrtc would be a header-only C++20 library of webcpp's own
(`kind: own`, although it wraps libdatachannel natively) for peer connections
and data channels, shaped as the W3C WebRTC API in Asio's model, on native
(libdatachannel 0.24.6) and Emscripten (the browser's objects), media out of
scope, replacing trystero's `backend` in a later change. What exists is the
draft specification of 2026-10-10 (`reference/spec.md`), with the owner's
decisions and the controller's rulings on a review; there is no plan and no
reference code, and both are required before the proposal can be accepted.

- **The interface carries the W3C names and shapes (`peer_connection`,
  `data_channel`, `session_description`, `ice_candidate`, the `RTC*`
  enumerations in snake case) in Asio's model: `async_*` operations with any
  completion token, the executor and the allocator as template parameters
  defaulting to `boost::asio::any_io_executor` and
  `std::allocator<std::byte>`, inputs as views and outputs owned.** The
  owner's decision of 2026-10-10, so that a program, trystero first, is
  written once and runs natively and in a browser.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D2
  (proposals).
- **Its targets are native and Emscripten, and the convenience header stops
  with an `#error` under `__wasi__` that names the reason.** libdatachannel
  needs threads, which neither WASI target gives, and a UDP socket layer and a
  DTLS stack it has no WASI port of, and Boost.Asio does not build for WASI
  with the pinned wasi-sdk; a WASI stub was rejected because it would let a
  program compile and fail at run time.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D3
  (proposals).
- **The native backend exists only with exceptions: its headers declare
  nothing under `WEBCPP_WEBRTC_NO_EXCEPTIONS`, and inside it every call into
  libdatachannel that may throw is caught and becomes a value; the Emscripten
  backend works with exceptions on and off.** libdatachannel reports errors by
  throwing, and the rulebook's rule for a header that wraps a throwing
  dependency then applies.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D4
  (proposals).
- **Events are pending operations the caller re-arms
  (`async_wait_ice_candidate`, `async_wait_state_change`,
  `async_accept_data_channel`, `async_receive` and their kin), one wait of a
  kind at a time, with a stated policy for an event that arrives while none is
  pending.** This keeps every handler on the executor and every lifetime in
  the caller's hands, as an acceptor's `async_accept`; callbacks, and one wait
  returning a variant of events, were rejected.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D5
  (proposals).
- **A `runtime` the program owns holds libdatachannel's process-wide state, at
  most one at a time natively, made before any connection and destroyed after
  the last, and enforces `max_peer_connections` (default 256) on both
  targets.** webcpp keeps no global or static mutable state, and
  libdatachannel's threads must be stopped through `Cleanup()` before static
  destruction; a hidden singleton (static state) and a per-connection cap (it
  cannot see the page) were rejected, and 256 is half of Chromium's 500 per
  page.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D6
  (proposals).
- **`async_receive` copies one whole message once into the caller's dynamic
  buffer; natively unread messages wait in libdatachannel's own bounded queue,
  a flow control that never fails, and in the browser in a queue bounded by
  `max_received_bytes`, past which the channel closes with
  `receive_overflow`.** In the browser the bytes must leave the JavaScript
  heap anyway and natively `rtc::binary` is a `std::vector` in
  libdatachannel's allocator, so the one copy is the one that puts them under
  the user's allocator; the browser has no flow control on receipt.
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D8
  (proposals).
- **webcpp::webrtc knows nothing of xactor: an actor that needs a peer holds a
  `peer_connection` made on the executor its driver runs on, and turns each
  completion into a message to itself.** The rulebook builds asynchronous
  ports on xactor and the owner put webcpp::webrtc in Asio's model; the two
  meet at the executor.
  Source: `libraries/webrtc/openspec/changes/genesis/reference/spec.md`
  section 2.3 (proposals).
- **Natively the library wraps libdatachannel, unmodified and justified in its
  documentation, with nothing of `rtc::` in a public header; it finds
  libdatachannel through `-sLIBDATACHANNEL_ROOT` or `.local/libdatachannel`,
  and pins it only when webcpp builds it from `third_party/`.** No WebRTC
  exists in webcpp, Boost, the C++ Alliance or the standard library, and
  libdatachannel is the smallest complete implementation (data channels
  without the media stack).
  Source: `libraries/webrtc/openspec/changes/genesis/design.md` D14
  (proposals).

**Open questions for the owner.**

- libdatachannel's licence: MPL-2.0 is decided case by case by the owner and
  no entry records it for libdatachannel; webcpp uses it unmodified, so the
  decision to record is that MPL-2.0 is accepted for this dependency.
- The compile-time message without exceptions: the compiler's "no member named
  'peer_connection'", which the rule's "declares nothing" gives, or a declared
  stub whose use fails a `static_assert` naming libdatachannel and exceptions,
  which departs from the rule.
- Whether a later version synthesises `negotiationneeded` natively.
- The exact names of the headers, the Jamfile targets and the repository
  layout, which the plan fixes.
- The precise form of webcpp::asio's browser bridge that increment 2 uses
  (that proposal is a draft too).
- Whether libdatachannel is built from `third_party/` before or after the
  first release.

Source: `libraries/webrtc/openspec/changes/genesis/design.md` Open Questions
(proposals).

### json_schema (draft, not decided)

webcpp::json_schema would be a header-only C++20 JSON Schema validator of
webcpp's own for drafts 4, 6, 7, 2019-09 and 2020-12 and OpenAPI 3.0's Schema
Object, on native, wasip2, wasip3 and Emscripten, proven by the official
JSON-Schema-Test-Suite, whose IR, loader, emitter and validator webcpp::zod
would later reuse. Its design and specification were approved by the owner on
2026-10-08 and 2026-10-09 and its plan has 58 tasks (`reference/plan.md`);
tasks E1.1 to E1.9 were built on an unpublished local branch, kept as
`reference/source/`, E1.10 is written and uncommitted
(`reference/source-wip/`), E1.4 (the regex policy) must be redone, and this
draft revives a library the owner shelved on 2026-10-12.

- **Every dialect is an empty policy tag (`draft2020_12`, `draft2019_09`,
  `draft7`, `draft6`, `draft4`, `openapi3_0`) whose machinery lives in
  `detail`, and `basic_validator<Traits, Dialects...>` carries the set a
  program chooses.** A program that needs one dialect does not carry the
  others' tables, and adding v1 later touches no existing dialect.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D2
  (proposals).
- **Errors and annotations render as 2020-12's four output formats as
  `boost::json` values; messages are built lazily from one table of English
  templates, which a hook in the validate options replaces; Boost.Locale is
  reached only through one optional header, refused without exceptions, and
  the `.pot` catalogue is printed from the same table.** The official formats
  are what other implementations and Bowtie compare; a hook keeps localisation
  out of the core and out of global state; Boost.Locale is compiled and
  throws, so only one optional header needs it and json_schema stays
  header-only.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D4
  (proposals).
- **`pattern` and `patternProperties` run on Boost.Regex 1.92, header-only,
  through webcpp::lexical's ECMA-262 translator behind json_schema's unchanged
  regex policy, with a step budget that turns a runaway search into an error,
  and the appended `errc::unsupported_regex = 21` for a pattern the engine
  cannot run as ECMA-262 does.** The owner abandoned SRELL and webcpp::regex
  on 2026-10-12; the translator passes 881 of 881 regex-dependent suite tests
  on all five lanes and gave no wrong answer against V8 on 87,028 fuzz
  queries, and it already ships in lexical for `format: regex`, so json_schema
  shares one engine with it.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D5
  (proposals).
- **A schema is compiled once into an immutable flat plan of instructions
  whose operands are indices into constant tables, run by a small iterative VM
  on an explicit stack, with no separate tree-walking interpreter; `is_valid`
  and `validate` share the engine through a template sink.** One engine cannot
  drift from another, a flat plan of indices is relocatable, copyable and
  later serializable, and no input depth reaches the C++ stack.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D9
  (proposals).
- **`load`, `compile`, `validate`, `emit` and `to_json` return
  `boost::system::result<T, diagnostic>`, whose `diagnostic` holds a code and
  two 256-byte `static_string`s, where and what, and the policies are grouped
  in one `traits` type.** So a refusal under an exhausted allocator still
  names its place; `std::error_code` alone names no place, exceptions on the
  main path fail webcpp's rule, and a template parameter per policy makes
  signatures unwieldy.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D11
  (proposals).
- **Numbers are exact: `traits::number_type` is `boost::decimal::decimal64_t`
  by default, schema numbers are read from their lexeme, an instance double is
  compared as the exact decimal its shortest text denotes, and `multipleOf`
  works on coefficients and exponents in 128 bits, never with `fmod`.**
  Required suite tests fail otherwise: rounding to `decimal64_t` accepts 2^64
  under `maximum: 18446744073709551615`, a double's binary value makes `1.1`
  exceed `exclusiveMinimum: 1.1`, and Boost.Decimal's `fmod` fails a required
  test.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D13
  (proposals).
- **Nothing recurses on input depth, and allocation goes through two
  allocators the program gives, the schema's and the validation's (pmr by
  default), with `is_valid` allocating nothing on the common path; the stated
  exceptions are the user's callbacks, Boost.Locale, and Boost.Regex's
  compiled programs and backtracking stack.** No input depth may reach the C++
  stack, so a 100,000-level hostile instance is a depth error, not a stack
  overflow; Boost.Regex allocates through the global `operator new`, so the
  full guarantee holds for schemas without patterns, which the allocation test
  measures.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D20 and
  D5 (proposals).
- **The official suite, read at run time from the superproject's submodule,
  runs on every target through one runner per dialect that runs `is_valid` and
  `validate` on every test and requires them to agree, against known-failure
  lists that fail on an unlisted failure, on a listed test that passes and on
  a listed test it cannot find.** So the lists never lie; the suite is MIT
  test data, never copied.
  Source: `libraries/json_schema/openspec/changes/genesis/design.md` D21
  (proposals).

**Open questions for the owner.**

- The owner's go to revive the library, and where its canonical local
  repository lives now.
- lexical's public matching interface (lexical's change `strategies`, group
  3): its names, and whether its compiled pattern keeps the resource it was
  given (task E1.4).
- `errc::unsupported_regex = 21`, appended by this change, and the allocation
  exception for Boost.Regex: the owner confirms both.
- Amendment A1 (numbers, the exact decimal of the shortest text), which E1.2
  implements, and A2 (`format_entry::check_in`), which E5.5 needs.
- The licence of the bundled meta-schemas and of the OpenAPI schema inside a
  BSL-1.0 repository (recommended: commit them with `LICENSE-JSON-SCHEMA.txt`
  and `LICENSE-OPENAPI.txt`).
- jsonschema-benchmark's missing licence, the benchmark's pins (its commit,
  Blaze, ajv) and Bowtie's pin.

Source: `libraries/json_schema/openspec/changes/genesis/design.md` Open
Questions (proposals).

### lexical: strategies (draft, not decided)

The change `strategies` is the design the owner asked for on 2026-10-13,
reviewing lexical 0.1.0's interface: one entry point whose behaviour a
developer chooses, Boost.Regex as an optional strategy, and the matching
interface webcpp::json_schema needs. Nothing of it is built, and its open
questions O1 to O6 await the owner. A source is given relative to
`libraries/lexical/openspec/changes/strategies/` in proposals.

- **One entry point for every format, `is<Format>(std::string_view s, Strategy
  const& strategy = Format::default_strategy{})`, constrained by
  `strategy_for`: any callable `bool(std::string_view)` is a strategy, and one
  that declares `using format` is a strategy for that format alone.** 0.1.0's
  functions have four shapes, so a table of formats needs a wrapper per
  format; with one signature, templates and tables handle every format, and
  `is` adds nothing to the strategy's answer. The model is Boost.Geometry's
  strategies (a default per type, others passed in).
  Source: `design.md`, D1; `reference/design.md`, sections 1 to 3 and 13
  (proposals).
- **Each format becomes a tag type in `webcpp::lexical` that nests its
  strategies, each an aggregate named after its standard (`time::rfc3339`,
  `iri::rfc3987`, `regex::ecma262`), and the configuration moves into them
  (`.leap`, `.manipulation`, `.resource`, `.max_length`, `.max_nesting`).** A
  developer chooses webcpp's implementation or another, or writes their own;
  the default strategy of each format gives, for every string, the answer
  0.1.0 gives, since no default verdict is to change.
  Source: `design.md`, D1 and Goals; `reference/design.md`, section 4
  (proposals).
- **Each strategy webcpp ships gives its configured grammar rule through
  `rule()`, and `is<F>(s, S{c})` equals `parse(s, S{c}.rule()).has_value()`,
  which replaces 0.1.0's invariant that a validator and its rule agree; `is`
  is `constexpr`, and its `noexcept` is the strategy's.** A Boost.URL rule
  carries its configuration, as a strategy does; a developer's own strategy
  carries no guarantee; a constexpr strategy stays so, and the IRI strategies
  may throw `std::bad_alloc`.
  Source: `design.md`, D1; `reference/design.md`, sections 3 and 4.1
  (proposals).
- **`date_time::rfc3339` gains `leap`, a `leap_seconds` defaulting to
  `allowed`.** A `date-time` holds a `time`, and 0.1.0's `date_time_rule_t`
  always allows second 60, so JSON Schema v1's `date-time` cannot refuse it;
  the default changes no answer.
  Source: `design.md`, D2; `reference/design.md`, section 4 (proposals).
- **Boost.Regex becomes an optional strategy, `regex::via_boost_regex<Judge>`:
  it refuses a pattern over its length bound or not ECMA-262 without calling
  the judge, translates the pattern into Boost's Perl syntax over code points,
  and hands the judge a `boost_regex_pattern` (the translation, its flags, the
  source); `boost_regex_compiles` is the ready judge, `ecma262_traits` the
  traits of ECMA-262's classes, and `<webcpp/lexical/regex/boost_regex.hpp>`
  alone includes Boost.Regex.** A developer who runs the patterns with
  Boost.Regex wants to refuse, when validating, a valid pattern Boost.Regex
  cannot run and to compile with the syntax and flags the engine will use; the
  translator is the feasibility prototype's, identical to V8 on 20,130 fuzzed
  patterns, rewritten to webcpp's rules.
  Source: `design.md`, D3; `reference/design.md`, section 6; `proposal.md`,
  Impact (proposals).
- **`via_boost_regex` is never `noexcept` and writes its translation through
  the memory resource it is given, and a pattern the translator marks
  `unsupported` goes to the judge, which `boost_regex_compiles` answers
  `false`, from a committed list.** The translation allocates; only the
  judge's engine knows what it can run, and the list makes each difference
  from V8 known.
  Source: `design.md`, Risks; `reference/design.md`, sections 6.3, 6.4 and 10
  (proposals).
- **The same header offers the matching interface json_schema's regex policy
  needs: an ECMA-262 pattern compiled into `boost::basic_regex<char32_t,
  ecma262_traits>` as a value that says why it failed (`syntax` or
  `unsupported`), and a search over UTF-8 text with a step budget (10,000,000
  steps plus 64 per byte) that answers `found`, `not_found` or `limit`,
  allocating through the caller's resource or allocator.** json_schema
  compiles a schema's `pattern` once and searches instances with a bounded
  number of steps; its task 1.4 waits for this group.
  Source: `design.md`, D4; `proposal.md`, Impact (proposals).
- **The move from 0.1.0 is mechanical and done in one version, each `is_x`
  call mapped to `is<F>` with its changelog lines, and while both exist
  0.1.0's functions are the oracle of the new entry point's tests;
  composition, a strategy taking its parts' strategies, is staged for later.**
  The tests prove that every default strategy answers as 0.1.0 did over every
  string of the unit tests and the suite; nothing in the first step prevents
  adding part strategies as members with defaults.
  Source: `design.md`, Migration Plan and Risks; `reference/design.md`,
  sections 7, 8 and 10 (proposals).

Open questions for the owner:

- **O1.** Keep the `is_x` functions as one-line forms of `is<F>`, or remove
  them? Recommended: remove them in the version that adds `is<F>`.
- **O2.** Composition (a strategy taking the strategies of its parts, `email`
  its `hostname`'s) in the same version, or later? Recommended: later.
- **O3.** Boost.Regex as an optional dependency of one header needs a recorded
  decision.
- **O4.** Format tags in `webcpp::lexical` directly, or in a namespace of
  their own; strategies named by their standard (`rfc3339`, `ecma262`)?
- **O5.** `ecma262_traits` shipped by lexical (recommended), or left to the
  developer?
- **O6.** The matching interface (D4) in lexical, as json_schema's proposal
  assumes, or in json_schema itself?

### trystero: bounded JSON parse (draft, not decided)

A relay or a peer can stall a room with a text well under the 1 MiB bound:
keys that collide in Boost.JSON's object hash, seeded with the object's
address, make a parse quadratic (4 MB in 9.2 s against 4 ms for random keys;
256 keys of 128 bytes, a text of 35 KB, cost 32,640 comparisons). The change
`bounded-json-parse` is uncommitted work on `d2563a6` (branch
`fix/json-flood`) that the owner has not decided.

- **Every object of a text the room parses for the protocol holds at most
  `protocol::json::max_object_members` (18) members, tied to Boost.JSON's
  `detail::small_object_size_` by a `static_assert`.** Below that threshold
  Boost.JSON hashes no object, whatever the seed, the key length or the word
  size, and the protocol's largest object has 7 members; the assertion fails
  the build if a Boost moves the threshold.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **The parse is a `boost::json::basic_parser` over a handler of trystero's
  whose `max_object_size` is 18: it stops at the 19th member, before reading
  its key, with `boost::json::error::object_too_large`, and builds no
  object.** What follows is never read, and the handler's `value_stack` over
  the room's storage keeps the parse in the room's resource, as before.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **`protocol::json::parse(text, max_bytes, storage, ec)` refuses a longer
  text with `errc::message_size` before reading it, each caller naming its
  bound, with no default: 68,608 bytes for a relay's text, 66,560 for a
  signal, 1,024 for a handshake message, 16 KiB for a native candidate.** The
  bounds come from the fields' own bounds (the signal's is a sealed
  description of 64 KiB with the longest ids and 1 KiB to spare), not from the
  measures (762 bytes for the longest signal, 1,244 for the longest relay
  text), and `static_assert`s tie them to those fields and keep the relay
  bound under 1 MiB. Having no default makes a new caller choose.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **A relay's text or a signal the parse refuses is logged and dropped, and
  the relay's connection stays open; a handshake message over its bound fails
  the handshake as one of the wrong shape does.** A text of the wrong shape
  was already ignored without closing; only a text over 1 MiB closes the
  connection, as in 0.1.0.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **An application's JSON payload keeps the unbounded parse, as
  `protocol::json::parse_payload`.** It is any JSON by design, as Trystero's
  `JSON.parse` reads it, and the cost of its objects is the application's,
  which asked for a `boost::json::value`.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **The page gains a guarantee: no text of a relay costs a room more than its
  length to read.** It is the change's goal; the diff numbers it 9, which
  `shared-connection` also numbers, so whichever change is archived second
  takes the next free number, anchors being permanent.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **The tests build the attack: `json_bounds` checks the bounds to the member
  and to the byte, and `hostile_relay_texts_are_refused_and_the_room_goes_on`
  sends 256 keys of 128 bytes sharing one Boost.JSON digest under three seeds
  through a memory relay, in an event and in a signal.** A sample of random
  keys would prove nothing about colliding ones; the key generator, written
  for 64-bit and 32-bit words, does not terminate as it stands.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).
- **Nesting depth, array length and Boost.JSON's seed are left alone, and the
  wire does not change.** Boost.JSON's parser already bounds depth, arrays are
  not hashed, the seed is Boost.JSON's, and the oracle lane stays
  byte-identical.
  Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
  (proposals).

**Open questions for the owner.**

1. **Numbering with `shared-connection`.** Which change takes guarantee 9? The
   diff writes 9 and `shared-connection` 9 to 11; whichever is archived second
   renumbers, and the two modified requirements are merged by hand.
2. **Public relays.** Does any public Nostr relay send events with more than
   18 members? The `interop` lane's `--public` mode would show it; it was not
   run.
3. **The difference from Trystero JS.** The page's "Differences from Trystero"
   should list the bound: a relay's text over 67 KiB and an object over 18
   members are dropped.
4. **Candidates.** The native bound for a candidate is
   `max_sealed_candidate_bytes`, since an opened candidate is shorter than its
   sealed form; the browser backend's `add_ice_candidate` hands the text to
   the page's `JSON.parse`, untouched by the diff, and whether it needs a
   bound is not addressed.
5. **The allocation bound for a signal.** The room test asserts that the room
   handed out less than 8,192 bytes while reading the signal of 256 colliding
   members; with a stand-in key set the signal read 137,946 bytes, the event
   that carries it holding some 40 KB, so the assertion needs another bound or
   another measure.
6. **Instrumentation.** The sizes cited (1,244 and 762 bytes) were measured
   with an instrumented relay in an earlier copy of the work, which the change
   does not include.
7. **The key generator does not terminate.** The partner of a printable word
   differs from it by a constant that makes its top byte non-printable, for
   8-byte and 4-byte words alike, so the hostile case cannot reach its
   assertions; it needs another way to build colliding keys that are valid
   JSON strings, or another test of the claim.

Source: `libraries/trystero/openspec/changes/bounded-json-parse/design.md`
(proposals), "Open Questions".

## 19. Conflicts and open questions

What this chapter lists breaks a rule of chapters 1 to 13, or makes a design
document disagree with the tree it describes. Each was found on 2026-10-26,
when this file was written, by reading the superproject at `ddb6cb2`, each
library at the commit 0.1.0 pins, asio at its local branch and the open
proposals; each names where it is. None is resolved here. A conflict of a
proposal is the owner's to decide, in that proposal; a library that does not
keep a rule is brought to it, or the rule records an exemption with its
reason; a text that is wrong is corrected by a change of its own. The change
that resolves an entry removes it from this chapter.

### asio and the work beyond its proposal

asio's proposal (chapter 17) and the local work of 2026-10-15 and 2026-10-16
beyond it break these rules, each approved by the owner or not as it says.
Paths are asio's or the superproject branch's, as chapter 17's entry gives
them.

- **Asynchronous code on xactor.** Chapter 4: "A library that ports anything
  asynchronous runs it as actors on xactor"; chapter 4 and the roadmap
  (chapter 16) leave the meeting with Boost.Asio and Cobalt to be designed.
  asio makes Boost.Asio's own `io_context`, suspended into the page, the event
  loop, and xactor a user (its `asio_driver`), not a dependency
  (`genesis/proposal.md`, Impact). Approved: the owner's decision 4 of
  2026-10-10 limits xactor's scheduler to "where a library has actors"
  (`genesis/reference/spec.md`, section 2), accepted with the spec on
  2026-10-11; the amendment of chapter 4's item is planned in
  `genesis/reference/plan.md`, task 22, not made. Chapter 16 still calls
  asio's specification "in review".
- **Header-only.** Chapter 4: "C++20, header-only." asio compiles
  `src/browser_src.cpp`, an object every Emscripten program links, and
  `src/cobalt_main.cpp`, the static library `cobalt-main` of every Cobalt
  program. Approved: the spec's requirement "Header-only library in its
  namespace" names both units (`genesis/specs/asio/spec.md`), accepted
  2026-10-11.
- **How a test ends.** Chapter 8: "`main` runs the cases and returns
  `boost::report_errors()`". Every asio test returns
  `webcpp::test::exit_status(boost::report_errors())` (D9), a rule only the
  superproject branch's `AGENTS.md` states. Approved with the spec (D9),
  committed on the superproject branch; not in main.
- **`config.hpp` on `<webcpp/config.hpp>`.** Chapter 3: "which includes
  `<webcpp/config.hpp>` first"; chapters 10, 6 and 16 add config's
  b2 target, `WEBCPP_NO_EXCEPTIONS` and the test `config_all_off`. asio's
  `config.hpp` includes `<boost/config.hpp>` alone, ignores
  `WEBCPP_NO_EXCEPTIONS`, builds its test twice, and `build.jam` names no
  `/webcpp/config//config`. Not approved: the spec's requirement
  "Configuration header" omits it, and the rule's commit `a369f7a` is of
  2026-10-11, the day the plan was approved.
- **`config.hpp` first.** Chapter 3: "which every public header includes
  first". `browser/async_await.hpp`, `browser/async_wait_event.hpp` and
  `browser/error.hpp` (asio) and `tls_client_context.hpp` (asio, uncommitted)
  include Boost and standard headers before it. Not approved; asio's own spec
  requires it first.
- **The changelog.** Chapter 10: "`doc/changelog.adoc` (its changelog, the
  page's last appendix". asio has no `doc/changelog.adoc`, and its spec's list
  of repository files omits it. Not approved.
- **The error category.** Chapter 4: "an error category of the
  library's own (`webcpp.<name>`)". `tls_client_context.hpp` (asio,
  uncommitted) defines a second category, `webcpp.asio.tls`, values 1 to 3,
  beside `webcpp.asio`. Not approved: the CA plan names no category.
- **Namespaces and headers.** Chapter 3: namespace "`webcpp::<name>`",
  headers "`<webcpp/<name>/...>`". The generated
  `<webcpp_ca_certificates.hpp>` declares `webcpp::detail::ca_certificates`,
  outside any library's namespace (`tools/ca_certificates/generate.py`,
  superproject, uncommitted). Not approved.
- **Allocation, the runtime's own.** Chapter 4: "routes every
  allocation through an allocator its user gives", and a test "proves that
  none goes elsewhere". asio names three allocations it cannot route:
  Emscripten's `select` poll array, Asyncify's unwind data,
  `basic_resolver_results`' entries (`doc/allocation.adoc`). Approved with the
  spec (requirement "The counters see an unbound handler and the runtime's own
  allocations are named"), 2026-10-11.
- **Allocation, the resolver's names.** The same rule. The Emscripten
  resolver's asynchronous form copies the host and service names with the
  standard allocator, not the handler's (`doc/allocation.adoc`, commit
  `557d452`). Not approved: the spec names only the result entries.
- **Allocation, OpenSSL.** The same rule. The TLS client context and its
  trust store allocate through OpenSSL (`tls_client_context.hpp`, asio,
  uncommitted), and webcpp::crypto's accepted D16 offers a memory resource for
  OpenSSL that asio does not use. Approved locally: "OpenSSL owns its
  allocations" is in the CA plan's contract
  (`docs/superpowers/plans/2026-10-16-ca-certificates.md`); not in main.
- **Allocation, unproven on WASIp3.** Chapter 4: a test "proves that
  none goes elsewhere". `allocation_test` runs natively and on Emscripten
  only, and `doc/capabilities.adoc` (asio, uncommitted) marks WASIp3's
  counters "unverified". Not approved.
- **Exceptions, native suites.** Chapter 16: "with exceptions on and
  off (the whole suites, not only a compile". Native suites without exceptions
  run only on Emscripten (departure 5). Approved 2026-10-11
  (`genesis/reference/plan.md`).
- **Exceptions, Boost.Cobalt.** Chapter 16: "Boost.Locale and Boost.Cobalt
  have no build without exceptions"; chapter 6: "A library works without
  exceptions." The branch builds Cobalt without exceptions with a warning
  (issue 287) that moving a pending task aborts, and asio's Cobalt programs
  run that way. Approved: the owner's request of 2026-10-14
  (`genesis/reference/STATUS.md`), committed as superproject `d507a93`; not in
  main.
- **Exceptions, WASIp3 cooperative.** Chapter 6: "Every target builds with
  exceptions on." `threading=multi` on WASIp3 refuses `exception-handling=on`
  (`tools/webcpp-wasi.jam`), for asio, Cobalt, OpenSSL and TLS alike. Approved
  locally: the WASIp3 spec makes exceptions "subject to the runtimes actually
  supplied by the SDK"; not in main.
- **A WASI target.** `AGENTS.md` chapter 4 (xactor's Jamfile): "Boost.Asio
  1.92 does not compile for wasm32-wasip2 or wasm32-wasip3"; `AGENTS.md`
  chapter 1: `xactor_asio` native only. asio's own D16 and spec ("MUST NOT
  declare wasm32-wasip2 or wasm32-wasip3") agree, yet `test/Jamfile`,
  `example/Jamfile` and `example/tls/Jamfile` (asio, uncommitted) declare
  `wasip3` over patched Asio headers. Approved locally: "The owner approved
  implementation, local tests, and an AsciiDoc feature matrix"
  (`docs/superpowers/specs/2026-10-15-asio-wasip3.md`); no proposal amends
  D16.
- **The WASI runtime and launcher.** `AGENTS.md` chapter 1: "wasi-sdk 34 and
  wasmtime 47 (47.0.3 measured)"; `AGENTS.md` chapter 4: a WASI lane "adds
  `testing.launcher=wasmtime`". The WASIp3 tests need Wasmtime 49.0.3 and
  `testing.launcher=tools/boost/asio-wasip3/run.sh`, which grants network
  access. Not approved beyond the plan's record of the tested configuration.
- **Tests run on every declared target.** Chapter 8: "Every test runs on every
  target its Jamfile declares." `cobalt_thread` and `wasi_core_threads` (asio,
  uncommitted) are explicit, in no lane, so only a request by name runs them.
  Not approved.
- **Boost from Boost's own sources.** `AGENTS.md` chapter 1: "built from
  Boost's own sources, for every target". Asio's headers on WASIp3 and Cobalt
  on every target compile from patched private copies
  (`tools/boost/asio-wasip3`, `cobalt-common`, `cobalt-wasip3`, `cobalt-ssl`,
  superproject, uncommitted). Approved locally by the WASIp3 spec, the Cobalt
  SSL plan and the cancellation plan's ruling; not in main.
- **OpenSSL found, not built.** Chapter 5: "found by
  `webcpp.external`, linked shared or static by a propagated feature, refused
  on WebAssembly". OpenSSL 3.5.9 is built from source, static only, natively
  and for WASIp3 (`tools/webcpp-openssl.jam`; `build.jam`, asio, uncommitted),
  which also supersedes departure 4 and contradicts webcpp::crypto's accepted
  D15 ("OpenSSL and Botan are found, not built", native only). Approved
  locally: the OpenSSL plan's "Approved scope and design" and the rewritten
  rule 7 of the uncommitted `AGENTS.md`; departure 4 and crypto's D15 are not
  amended.
- **A C library built file by file, on every target.** Chapter 5:
  "builds it from its own sources, file by file" and "on every target and with
  every file". Upstream Configure and Make compile OpenSSL; WASIp2, Emscripten
  and MSVC are refused. Approved locally: the uncommitted rule 7 calls it "a
  deliberate exception to spelling out its source list in B2".
- **The link-time canary.** Chapter 5: "every function that calls the
  C library references a canary". OpenSSL has none; B2 ordering and exact
  archive paths stand in (uncommitted `AGENTS.md`, rule 7). Approved only by
  that uncommitted text.
- **The C library's own tests.** Chapter 5: "the C library's own tests
  pass on webcpp's build". OpenSSL is configured `no-tests`, and its suite is
  not run (`docs/superpowers/plans/2026-10-16-openssl-source-build.md`,
  "Explicit limits"). Not approved.
- **One copy per program.** Chapter 5: "webcpp's build of a C library
  is never linked together with another copy of it." asio links the
  source-built OpenSSL while trystero's native backend links the system's
  through `webcpp.external` (`libs/trystero/build.jam`, line 38, main), so a
  program on both carries two copies, which asio's `doc/building.adoc`
  (uncommitted) forbids without a remedy. Not approved; unresolved.
- **Third-party code is a submodule.** Chapter 10: "never copied into a
  library: it is a submodule under `third_party/libs/<name>`, pinned at a
  commit". The CA bundle is a checked-in snapshot under
  `third_party/ca_certificates`, MPL-2.0, which chapter 10 leaves to the owner
  case by case. Approved locally: "Data snapshots are an authorized exception
  to source-code submodules"
  (`docs/superpowers/plans/2026-10-16-ca-certificates.md`), and the
  uncommitted `AGENTS.md` calls it "an authorized exception"; not in main.

Open for the owner: whether asynchronous code is built on Boost.Asio's model
rather than on xactor (chapter 4's item, which asio's task 22 is to amend);
whether a proposal is written for the WASIp3 port, OpenSSL built from source
and the CA bundle, which no proposal holds, and whether it amends asio's D15,
D16 and departure 4, crypto's D15, and chapter 5's rules 1, 2, 6, 7, 10 and
11; and how a program that uses asio and trystero together holds one OpenSSL.

### The libraries against the rules

Each library below does not keep a rule of chapters 3 to 12 at the commit
0.1.0 pins. The roadmap (chapter 16) already brings some of them to it, as
it says.

- **Allocation (chapter 4).** xactor takes no allocator and has no counting
  test (`include/webcpp/xactor/scheduler.hpp`); xstate's machine has no
  `Allocator` parameter and holds `std::shared_ptr` and `std::function`
  (`machine.hpp:1322-1328`, `implementations.hpp:81-145`); wasi's response
  holds its body as a `std::string`; these three are on the roadmap. document
  has no `Allocator` parameter either, and is not on it. The pages of pratt,
  idna, trystero and document do not say which of their types are shareable
  (each library's `doc/allocation.adoc`).
- **Types of a fixed meaning (chapter 4).** Plain `int` or `unsigned` where no
  interface asks for it: pratt's binding powers and `max_depth`
  (`parser.hpp:54,175,199,309`, `calculator.hpp:988-1017`), wasi's
  `response.hpp:45` and `entrypoint.hpp:101,122,200,234` (with unqualified
  `uintN_t`), idna's `detail/nfc.hpp:229,233`, trystero's
  `detail/nostr_strategy.hpp:139-140`, `detail/signaling.hpp:232` and
  `protocol/nostr.hpp:49`, lexical's `date_time.hpp:54-144`,
  `detail/regex_validator.hpp:71` and `detail/regex_unicode.hpp:35`, and
  document's sizes as `std::uint64_t`. The roadmap's review covers it.
- **What a library builds on (chapter 4).** document hashes with an FNV-1a of
  its own rather than Boost.Hash2's; lexical has a UTF-8 decoder of its own in
  `detail`, the third in webcpp.
- **Asynchronous code on xactor (chapter 4).** trystero runs on Asio's
  executors, which its genesis ruling 6 defers; document's
  `<webcpp/document/asio.hpp>` is on Asio, not xactor; xactor itself declares
  no Emscripten target (`test/Jamfile:11`), with no reason recorded, although
  chapter 4 has asynchronous code run in a browser from the page's event
  loop. asio's entry above holds the decision these wait on.
- **Errors as values (chapter 4).** lexical has no error category of its own.
- **Text (chapter 4).** lexical passes text as `boost::core::string_view`,
  where the rule asks for `std::string_view`.
- **Bringing a C library (chapter 5).** trystero's libsecp256k1 and
  libdatachannel are not submodules under `third_party/libs/`, and have no
  `HAS` define and no canary (`build.jam:31-38`); the roadmap's external
  libraries built from `third_party/` covers it.
- **Exceptions (chapter 6).** lexical's throwing IRI functions
  (`uri.hpp:281-325`) and document's throwing conveniences are not behind
  their library's `NO_EXCEPTIONS` macro; xstate's `machine.hpp:1213,1233`
  and `node_set.hpp:121` call `.at()`, which throws `std::out_of_range` on a
  precondition violation, beside guarantee 15, "xstate throws no exception
  of its own". wasi's `config.hpp:38-48` reads `WEBCPP_NO_EXCEPTIONS` itself,
  as chapter 6 records until its redesign.
- **Fidelity and evidence (chapters 11 and 12).** Sixteen entries of xstate's
  appendix "Differences" have no `diff_` program (names, src, targets,
  undefined, system, time, deadlines, timers, tick, stop, gone, core-clock
  among them); trystero has no `diff_` program, no twins and no
  `without-twin.txt`, and its `test/oracle/Jamfile:16` runs the original
  without `--conditions=development`; document has no twins, and its Node is
  not pinned.

### The text of the rules

These are errors of the rules' own text, moved here unchanged (the proposal
`coding-guidelines`, design D3), to be corrected each by a change of its own.

- `AGENTS.md` chapter 1's table of libraries gives lexical's path as
  `libs/formats`; it is `libs/lexical`.
- Chapter 9's table leaves out the lint's rule "Doc Comment code spans"
  (`tools/lint/lint.sh`, rule 17), and the organization's specification counts
  twenty-one rules where the lint runs twenty-two (`octal escapes` added).
- Chapter 16 says trystero's `config.hpp` is not yet on
  `<webcpp/config.hpp>`; it includes it
  (`libs/trystero/include/webcpp/trystero/config.hpp:14`). It also calls the
  specifications of asio and crypto "in review"; the owner accepted both on
  2026-10-11.
- Chapter 10 registers the project `/webcpp/third_party` (`Jamroot`), while
  chapter 5's rule 5 writes `/third_party//bearssl` and crypto's plan
  `/third_party//<name>`.
- Chapter 8 lets a program exclude a target with `#ifndef __wasi__`, while
  config's design chose `WEBCPP_TARGET_*` so that no library reads the
  compiler's macros (config's genesis `design.md`); xactor's `drivers.hpp`
  tests `__wasi__`.
- Chapter 6 asks every library for `WEBCPP_<NAME>_NO_EXCEPTIONS` and
  `test/config_test.cpp`; config has neither, and chapter 3 exempts only its
  names and its namespace.
- Chapter 3 says nothing of the error enumeration's name: pratt's is `error`,
  xactor's and xstate's `errc`.
- Chapter 10's "every file BSL-1.0" has no exception for the Unicode-licensed
  data idna generates into `detail/tables.hpp`, and the owner's confirmation
  is not recorded (idna's genesis `reference/STATUS.md`).
- Chapter 2 limits "one import commit" to code moved from elsewhere;
  `CONTRIBUTING.md` and the organization's specification say every webcpp
  repository starts from one.
- `AGENTS.md` chapter 4 shows xactor's `test/Jamfile` "whole" without its
  `config_*`, `errors` and `sequence_and_log` tests, and names trystero's
  `interop` as the one Chrome lane, where `tools/ci/matrix.py` also gives
  document's `js` one.
- `AGENTS.md` chapter 5 and `tools/release/release.py` bundle `site` and
  `third_party` in `dist`, where the organization's specification says `dist`
  refuses a submodule that is not a library.
- `CONTRIBUTING.md` states as present the testing of every variant, which
  chapter 16 holds as a roadmap item, and, with the site's community page,
  says a library is proposed by an issue, where the proposals'
  `CONTRIBUTING.md` says by a proposal. `README.md`'s table of libraries
  leaves out config.
- `.gitmodules` gives `site` an absolute SSH URL, against chapter 10's
  relative URLs; whether a `git clone --recursive` over HTTPS then needs SSH
  access is not tested.

### Design documents and specifications against their trees

Each library's specification and design are in
[webcpporg/proposals](https://github.com/webcpporg/proposals), under
`libraries/<name>/openspec/`. Where one disagrees with its tree, chapter 15
says what the tree does.

- **Every library of 0.1.0.** The specifications of config, xactor, xstate,
  pratt, wasi, idna and lexical still put their changelog's entries under
  "Unreleased", which 0.1.0 froze under "webcpp 0.1.0". wasi's and idna's
  `.openspec.yaml` link implementations (`6e3c529`, `e2452e5`) older than the
  commits 0.1.0 pins (`b936324`, `f298a3e`).
- **config.** `README.md`, `doc/testing.adoc` and the specification still
  give the site as `webcpporg.github.io/webcpp/`; the genesis `design.md`
  gives the lint the check of reserved names that `tools/doc/libraries.py`
  makes.
- **xactor.** Its `AGENTS.md` and `doc/quick-start.adoc` say the target adds
  only its include directory and Boost; `build.jam` also adds config. The
  specification counts four test programs; the tree has six.
- **xstate.** The genesis says every failure has the category
  `webcpp.xstate`; the actor layer refuses a call with xactor's
  `invalid_argument`. `test/CONVERSION.md` counts 23 public headers; there are
  24. Its `AGENTS.md` and `<webcpp/xstate.hpp>` say it depends on Boost.JSON
  alone, leaving out config.
- **pratt.** `reference/design-source.md` says "without exceptions on wasip2
  only", which D11 and the tree superseded; `doc/pratt.adoc` and the
  specification leave Emscripten out of its targets.
- **wasi.** The genesis says an allocator is not planned; chapter 16 plans
  one, wasi's among the first. The redesign (chapter 17) says it lands before
  webcpp's first release, which 0.1.0 prepares with the current interface;
  its specification credits `serve.py` with flags that
  `tools/component/component.jam` composes, requires a refusal on wasip1 that
  an earlier requirement makes unreachable, and puts public types in
  `detail`, against chapter 3.
- **idna.** The specification's include boundary, allocation test and
  changelog differ from `meta/include-boundaries.json`,
  `test/allocation_test.cpp` and `doc/changelog.adoc`, and it leaves out the
  generator's rule for MSVC's warning C4125.
- **lexical.** Its `AGENTS.md` names `max_regex_alternatives`, which no longer
  exists, and the stack at the nesting bound was not measured again;
  `doc/regex.adoc` lists the old generator's checks; the comment of
  `tools/generate_patterns.py` says three kinds and lists four; the brief of
  `detail/regex_validator.hpp` is wrong and its `flags::i` is set and never
  read; the specification's counting resource differs from
  `test/counting_resource.hpp`; the genesis lists four guards where the tree
  has five, and its reference's section 5.8, step 5, is not built; the
  introduction and `json_pointer.hpp` cite only draft handrews-01;
  `test/Jamfile` calls `/bigobj` "not measured" while MSVC's CI is green.
  Its draft strategies (chapter 18) cites a decision D10 that does not exist,
  refuses leap seconds in v1 where the suite's `date-time.json` accepts them,
  counts 22 functions where there are 21, names `uuid::rfc4122` where RFC
  9562 replaced it, and leaves its O6 out of its reference.
- **document.** The tree accepts `float`, which the specification refuses;
  staging on the process heap inside a `noexcept` path contradicts the
  genesis and `doc/errors.adoc`; `stringify_to` allocates, against
  `doc/stringify.adoc`; the genesis keeps O5 open though it is resolved and
  counts 20 examples where there are 21; `doc/testing.adoc` and
  `doc/exceptions.adoc` are incomplete.
- **trystero.** 0.1.0 pins `77fed8f`, whose changelog lists `app::join`
  under 0.1.0, while the proposal `shared-connection` is still
  `implementing` with no release and waits for "the next release", and the
  genesis ties 0.1.0 to `160f431`. The specification gives the interop
  driver's timeout as 900 where `test/oracle/Jamfile` gives 2400; the shared
  connection's delta has the roadmap list shared relay sockets, which
  `doc/roadmap.adoc` leaves out; comments still say "the room's resource"
  where it is the app's (`backend.hpp`, `native/data_channel.hpp`), and
  `error.hpp` that `room::on_error` reports `invalid_configuration`;
  `doc/differences.adoc` still says "as today"; comments use a private
  shorthand, `C/`, for Trystero's source files. The draft bounded parse
  (chapter 18) claims guarantee 9, which 9 to 11 now hold, rebases on
  `160f431`, and says "about sixty times" where its own figures give 87.
- **asio.** The proposal's design, specification and impact give the default
  turn limit as 1000; the tree's is 100 (`24c1e2c`). They cite library and
  superproject commits (`7cfd449`, `df52767`) that the re-import replaced
  (tip `f5fd7a8`), and `STATUS.md` a superproject commit (`063343d`) behind
  the branch (`d159022`). Its open questions on task 9's re-review, the
  Chrome measurement and Cobalt's stack are answered. D16 and the
  specification have no WASI target; the uncommitted Jamfiles declare
  `wasip3`. D15 and departure 4 link the system's shared OpenSSL; the
  uncommitted `build.jam` links one built from source, statically. Three
  headers do not include `config.hpp` first; `doc/allocation.adoc` names
  allocations beyond the specification's three; four examples are
  Emscripten-only, against the specification, and the TLS examples have no
  page variants; the lanes `cobalt`, `cobalt_asyncify` and `cobalt_wasip3`
  are added; on WASIp3 Cobalt's `co_main` drops SIGINT and SIGTERM.
- **The site.** The site's specification names `.htmlvalidate*.json`, which
  the site's `d113e38` renamed, and that commit broke the wrapping of
  `site/Jamfile`'s comments. The proposal of increments 4 and 5 (chapter 17)
  says the shared connection is missing, which `main` has, keeps the old fixed
  commit identity in its `STATUS.md` and plan, and is `decided` a day before
  it was `created`.
- **The organization.** Its specification's requirement on the compilers the
  CI tests predates the rule that makes them a subset of the lanes
  (`AGENTS.md` chapter 5), and its roadmap requirement repeats chapter 16's
  sentence on trystero's configuration.
