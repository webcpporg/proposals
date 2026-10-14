# organization Specification

## Purpose
webcpp is a collection of header-only C++20 libraries for the web, organized as Boost is: a
superproject, `webcpporg/webcpp`, holds each library as a submodule under `libs/<name>`, one build
(b2), one rulebook (`AGENTS.md`), one lint, one documentation toolchain, one CI and one site. This
capability is the organization as built at the superproject's commit `c53f3f9` (`../../changes/archive/2026-10-07-genesis/reference/superproject/`):
with subsequent changes recorded in `../../changes/archive/2026-10-13-agent-skills/`:
how it is laid out, how every library is built, tested, linted, documented, released and published,
and what every contributor, human or coding agent, must do. A library's own proposal relies on
these requirements and adds only what is specific to the library.

## Requirements

### Requirement: Superproject: The collection and its model
The organization SHALL be a superproject, `webcpporg/webcpp`, that plays the part of `boostorg/boost` for the GitHub organisation `webcpporg` (the equivalent of `boostorg`): every library is header-only C++20 under the Boost Software License 1.0, lives in a repository of its own, and is checked out in the superproject as a submodule under `libs/<name>`. webcpp brings to C++ the libraries that other languages use for the web, each ported at an exact version of an original and proven against it, or a library of webcpp's own, and serves three targets with a purpose each: `emscripten` (the browser), `wasip2` and `wasip3` (WASI components run by wasmtime and by clouds), and `native` (a program that integrates closely with its operating system). Every webcpp-authored file SHALL be BSL-1.0 and the copyright notice is "Copyright (c) 2026 WebCpp.org". The six generated OpenSpec skill documents in the proposals repository SHALL retain their upstream MIT licence and metadata, with the full upstream copyright and permission notice preserved in `THIRD_PARTY_NOTICES.md`.

#### Scenario: A reader asks what a target is for
- **WHEN** `AGENTS.md` chapter 1 and `README.md` are read for the targets
- **THEN** emscripten is the browser, wasip2 and wasip3 are WASI components run by wasmtime and by clouds, and native is a program that wants close integration with its operating system

#### Scenario: A library of the collection
- **WHEN** `libs/xactor` is inspected
- **THEN** it is a git repository of its own whose remote is `webcpporg/xactor`, checked out as a submodule of the superproject, with header-only C++20 headers under `include/webcpp/xactor/`

### Requirement: Superproject: The libraries of 0.1.0
The superproject SHALL register nine libraries as submodules of `libs/`: `config`, `xactor`, `xstate`, `pratt`, `wasi`, `trystero`, `document`, `idna` and `lexical`, each with the targets it declares: config on native, wasip2, wasip3 and emscripten; xactor on native, wasip2 and wasip3 (its Asio driver native only); xstate (a port of XState 5.33.2, with an oracle lane) on native, wasip2 and wasip3; pratt on native, emscripten, wasip2 and wasip3; wasi on wasip2 and wasip3 (its `response.hpp` also natively) with its own lanes `http`; document on native, wasip2, wasip3 and emscripten; idna on native, wasip2, wasip3 and emscripten with its own lane `tables`; lexical on native, wasip2, wasip3 and emscripten with its own lanes `generated` and `oracle`; trystero (a port of Trystero 0.26.0) on native and emscripten with its own lanes `oracle` and `interop`. `AGENTS.md` chapter 1 and `README.md` list them in a table of name, what it is, targets and state.

#### Scenario: .gitmodules lists the libraries
- **WHEN** `.gitmodules` is read at `c53f3f9`
- **THEN** it holds the submodules `libs/config`, `libs/xactor`, `libs/xstate`, `libs/pratt`, `libs/wasi`, `libs/trystero`, `libs/document`, `libs/idna` and `libs/lexical`, each with the url `../<name>.git`, besides `site` and the two under `third_party/libs/`

#### Scenario: A library's declared targets are discoverable
- **WHEN** `b2 declared-targets -d0` runs
- **THEN** it prints one `<library> <target>` line for each target a library's test and example Jamfiles declare, and these lines are the lanes the CI plans

### Requirement: Superproject: Submodule URLs and what the superproject records
A library submodule SHALL be added with a URL relative to the superproject's, as Boost's `.gitmodules` writes its libraries' (`libs/xactor` is `../xactor.git`), so that a clone fetches the libraries over the same protocol as the superproject, and the superproject MUST record only the library's commit: nobody runs `git add -A` or `git add libs` in the superproject. A change to a library is committed in the library on its `main`, then the superproject commits the library's new commit by path; the superproject's CI tests every library at the commit its submodule points to, and `main` is the only branch.

#### Scenario: Adding a submodule
- **WHEN** the owner adds the library `<name>` once `webcpporg/<name>` exists
- **THEN** the command is `git submodule add ../<name>.git libs/<name>`

#### Scenario: Moving a library's pointer
- **WHEN** a library has a new commit on its `main`
- **THEN** the superproject commits `libs/<name>` by path, with a subject such as `libs/lexical: ...`, and never stages `libs` as a whole

### Requirement: Superproject: The layout of the repository
The superproject SHALL hold at its root `Jamroot` (the single build configuration), `AGENTS.md` (the rulebook), `README.md`, `CONTRIBUTING.md`, `LICENSE_1_0.txt`, `.clang-format`, `.clang-tidy` and `pyrightconfig.json` (the style, checks and Python settings of every repository), `libs/`, `third_party/`, `doc/` (the index page `index.adoc` and its `Jamfile`, with `doc/releases/` for each version's release notes), `site/` (the submodule of the site), `tools/`, `.github/` (`workflows/library.yml`, `workflows/ci.yml`, `actionlint.yaml`), the git-ignored `.local/` (machine-local setup) and the git-ignored `bin/` (b2's build directory). `tools/` MUST hold `webcpp.jam` (the Jamfile API), `target.jam`, `throw_exception.cpp`, `boost_json.cpp`, `boost_test_runner.cpp`, and the directories `boost/`, `oracle/`, `lint/`, `doc/`, `example/`, `component/`, `drive/`, `generated/`, `node/`, `report/`, `test/`, `ci/`, `site/` and `release/`.

#### Scenario: The root of the published commit
- **WHEN** `../../changes/archive/2026-10-07-genesis/reference/superproject/` is listed
- **THEN** it shows `.clang-format`, `.clang-tidy`, `.gitattributes`, `.github`, `.gitignore`, `.gitmodules`, `AGENTS.md`, `CONTRIBUTING.md`, `Jamroot`, `LICENSE_1_0.txt`, `README.md`, `doc`, `libs`, `pyrightconfig.json`, `site`, `third_party` and `tools`

#### Scenario: Ignored files
- **WHEN** `.gitignore` is read
- **THEN** it ignores `.local/`, `bin/`, `node_modules`, `.node-modules/`, `doc/html/`, `__pycache__/` and `.DS_Store`, and `.gitattributes` sets `* text=auto eol=lf` and `*.bat text eol=crlf`

### Requirement: Superproject: The rulebook and who reads it
`AGENTS.md` SHALL be webcpp's rulebook, written for coding agents and human contributors alike, holding every rule needed to port a library or change one so that an agent with no other context can do it by following it; it describes the tree as it is, with what is still to come only in its chapter 13 (the roadmap), to which every other mention of it refers. `AGENTS.md` has thirteen chapters: what webcpp is, choosing and registering a port, names, fidelity, evidence (oracle and twins), C++ rules, Doc Comments and the API reference, documentation, tests/lanes/the report/the site/CI, process, commits and text, b2 facts, and the roadmap. Each library's `AGENTS.md` MUST hold only what is specific to that library (the original and its version, how its oracle drives it, its cautions) and link the rulebook as `../../AGENTS.md`; `CONTRIBUTING.md` summarises the rules for external contributors and, where it and `AGENTS.md` differ, `AGENTS.md` rules.

#### Scenario: A library's AGENTS.md
- **WHEN** `libs/config/AGENTS.md` is read
- **THEN** it begins "Working on config", tells the reader to read webcpp's `AGENTS.md` first, and holds only config's own rules (macros only, `version.hpp` outside `webcpp/config/`, the version written once, the target taken from the compiler)

#### Scenario: Every command runs from the root
- **WHEN** `AGENTS.md` shows a command
- **THEN** it runs from the superproject's root, in a shell where `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` are unset

### Requirement: Superproject: Prerequisites and pinned versions
The prerequisites SHALL be: Boost 1.92 or newer installed, and, for a library that links a compiled Boost library, Boost's sources of the same version; b2 (B2 5.5.3 measured); a C++20 compiler; Python 3.9 or newer; for WebAssembly, wasi-sdk 34 and wasmtime 47 (47.0.3 measured); for components, wit-bindgen 0.62.0 and the `wasi:http` WIT of WASI 0.2.12 and 0.3.0 (the `wit/deps` directories of the Rust crates `wasip2` 1.0.4 and `wasip3` 0.9.0); for the documentation, Node, MrDocs 2026.9.29 and clang++; for a library's oracle lane, Node and npm; for the lint, wasi-sdk 34's clang-format, clang-tidy and clang++ with the wasip2, wasip3 and (when a library declares it) emscripten toolsets; for emscripten, Emscripten 6.0.11 from emsdk and Node; for trystero, libsecp256k1 0.8.0, libdatachannel 0.24.6 and OpenSSL 3 until webcpp builds them from `third_party/`. Each toolchain MUST be installed by hand and configured in `user-config.jam`, until webcpp bundles the toolchains.

#### Scenario: The pins of the CI
- **WHEN** the CI's actions are read
- **THEN** they install Boost 1.92.0, wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0, the WASI WIT of crates `wasip2` 1.0.4 and `wasip3` 0.9.0, MrDocs 2026.9.29, Emscripten 6.0.11 and Node 26.7.0

#### Scenario: A toolset registered by hand
- **WHEN** `user-config.jam` holds the lines of the regions `wasi-sdk`, `wasip2` and `wasip3` of `tools/ci/wasi-sdk.jam`
- **THEN** b2 knows the toolsets `clang-wasip2` and `clang-wasip3` built against wasi-sdk, and `WASI_SDK` is poked to wasi-sdk's directory

### Requirement: Superproject: Machine-local setup and where tools are found
`.local/` SHALL be git-ignored and hold what one machine needs: `.local/user-config.jam`, `.local/wasi-sdk/`, `.local/mrdocs/`, `.local/wit-bindgen/`, `.local/wasi-wit/p2` and `p3`, `.local/emsdk` with Emscripten's cache in `.local/emscripten-cache`, `.local/boost-source`, and trystero's `.local/secp256k1-native`, `.local/secp256k1-emscripten`, `.local/libdatachannel` and `.local/openssl`. A tool the build looks up itself MUST be taken from the path its `-s` option gives on b2's command line (never the environment), else from `.local/`, else from `PATH`: `-sMRDOCS`, `-sWIT_BINDGEN`, `-sWASI_WIT_P2` and `-sWASI_WIT_P3`, `-sWASMTIME` (looked up when a served test runs), `-sWASI_SDK` (for a component a script builds) and `-sWEBCPP_EMSDK`. When a tool is needed and not found the build stops, naming it and every place it looked; a native `b2 test` needs none of the component tools.

#### Scenario: wit-bindgen is looked up only when needed
- **WHEN** `b2 test` runs with the host's toolset
- **THEN** wit-bindgen, the WIT and wasmtime are never looked for

#### Scenario: A missing tool is named
- **WHEN** `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test` runs on a machine without wit-bindgen
- **THEN** the build stops naming wit-bindgen, `-sWIT_BINDGEN=<path>`, `.local/wit-bindgen/wit-bindgen` and `PATH`

### Requirement: Superproject: The build commands
The build SHALL be b2, and `AGENTS.md` chapter 1 MUST document these commands: `b2 test` (every library's tests, natively), `b2 example` (each example built, run with its `.input` as standard input when it has one, and its output compared with its `.expected`), `b2 doc` (the index page `doc/html/index.html` and every library's `libs/<name>/doc/html/index.html`), `b2 doc -sWEBCPP_INDEX=site`, `b2 libs/<name>/test` (and `example`, `doc`), `b2 libs/<name>/test//<test>`, `b2 libs/<name>/doc//reference`, `b2 toolset=clang-wasip2 testing.launcher=wasmtime ...` (and `clang-wasip3`, one toolset per command), `b2 toolset=emscripten ...` (never with a launcher), `b2 install --prefix=<dir>`, `b2 declared-targets -d0`, `b2 declared-lanes -d0`, `b2 libs/<name>/test/oracle//oracle` and `//update-expected`, `b2 libs/<name>/test//generated-<name>` and `//update-generated`, `b2 -a ...` (from scratch, the only build that counts as evidence), `tools/lint/lint.sh`, `python3 tools/<dir>/<name>_test.py`, `python3 tools/ci/matrix.py ...`, `b2 site`, `b2 site-serve`, `b2 check-release`, `b2 whats-new` and `b2 dist`.

#### Scenario: One library's tests
- **WHEN** a contributor runs `b2 libs/xactor/test//scheduler`
- **THEN** only the test named `scheduler` of xactor's test project is built and run

#### Scenario: Install
- **WHEN** `b2 install --prefix=/tmp/w` runs
- **THEN** every library's headers are copied to `/tmp/w/include/webcpp/` and its `LICENSE_1_0.txt` and any `LICENSE-<ORIGIN>.txt` land beside them in `/tmp/w/include/webcpp/<name>/`

### Requirement: Superproject: Third-party code
What a library or its tests use and webcpp does not own SHALL never be copied into a library: it MUST be a submodule under `third_party/libs/<name>`, pinned at a commit, with the URL of its own repository, and `third_party/Jamfile` declares it as `/webcpp/third_party//<target>` whose usage requirements give a program what it needs. It keeps its own licence and notices, and the lint never reads it. Adding one is the owner's decision and moving its pin is a change of its own, with its evidence. A target whose submodule is not checked out stops the build that needs it, naming `git submodule update --init third_party/libs/<name>`; a build that needs none never looks. The superproject pins `json-schema-test-suite` (the JSON Schema Test Suite at `5b0ee16`, 2026-09-21, MIT) and `nativejson-benchmark` (Milo Yip's at `478d572`, 2022-10-28, MIT).

#### Scenario: A test reads the suite
- **WHEN** a test names `/webcpp/third_party//json-schema-test-suite`
- **THEN** it receives the define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` with the suite's directory in forward slashes and quoted whole, wasmtime is given the directory (`webcpp.data`), and the test runs again when a file of the suite changes

#### Scenario: The submodule is not checked out
- **WHEN** a build names `json-schema-test-suite` and `third_party/libs/JSON-Schema-Test-Suite/tests/draft2020-12` does not exist
- **THEN** the build stops with "third_party/libs/JSON-Schema-Test-Suite is not checked out" and the command `git submodule update --init third_party/libs/JSON-Schema-Test-Suite`

### Requirement: Superproject: Bringing a C library
A webcpp library that builds on a C library SHALL bring it in so that the C code stays exactly its authors' and the program carries only what it calls: the C library is a submodule under `third_party/libs/<name>` pinned at a commit and never edited (an unmodified `webcpporg/<name>` mirror when upstream is not on GitHub); `third_party/Jamfile` builds it from its own sources, file by file, as a static library in its own C dialect on every target, never as a single unity source; every file is compiled with `-ffunction-sections -fdata-sections` (`/Gy /Gw` on MSVC) with `<warnings>off`; its usage requirements give the public include directory, the define `WEBCPP_<LIB>_HAS_<NAME>=1` and the link-time removal of unused sections (`-Wl,--gc-sections`, `-Wl,-dead_strip`, `/OPT:REF`); a program names the C library in its own Jamfile beside the webcpp library that uses it (`exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;`). Three layers MUST stop a wrong build, each saying exactly what to add: a compile-time `#error` when the backend's header sees no `WEBCPP_<LIB>_HAS_<NAME>`; a link-time canary `webcpp_<lib>_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link`; and on MSVC `#pragma detect_mismatch("webcpp_<lib>_backend_<name>", "<name>-<commit7>-<configuration>")`. A library webcpp does not build (OpenSSL) is found by `webcpp.external`, refused on WebAssembly, and never left out in silence when a program names it. One copy of a C library per program, and moving a pin is a change of its own: the submodule, the canary's commit, the tests on every target and the size table.

#### Scenario: A program forgets the backend
- **WHEN** a program links a webcpp library whose backend is a C library and its Jamfile does not name the C library
- **THEN** the program fails to link with the canary's name, which says to add the third-party library to the link

#### Scenario: A library never called adds nothing
- **WHEN** a program names the C library in its Jamfile and never calls it
- **THEN** it is byte-identical to one that does not name it, since sections are discarded at link time

### Requirement: Superproject: Commit identity and form
A maintainer's commits, and those of the maintainer's agents, SHALL be made as `R <github.overboard312@passmail.net>` with no trailer of any kind (no `Co-authored-by`, no `Signed-off-by`, no agent name), by `git -c user.name=R -c user.email=github.overboard312@passmail.net commit -- <paths>`; an external contributor commits under their own name and address, also with no trailer. Commits MUST stage explicit paths and never `git add -A`; a subject is plain, says what the change does, and is prefixed by the part it touches (`tools/doc: ...`, `xactor: ...`); a last body paragraph MUST NOT start with `word:` since git reads it as a trailer (`std::` does). Every commit message is English, with no em dash.

#### Scenario: A commit by an agent
- **WHEN** an agent commits a change to `tools/lint`
- **THEN** `git log -1 --format='%an %ae'` prints `R github.overboard312@passmail.net` and `git log -1 --format=%B | git interpret-trailers --parse` prints nothing

#### Scenario: An accidental trailer
- **WHEN** the last paragraph of a message starts with `std::`
- **THEN** git would read it as a trailer, so the paragraph is rewritten to start otherwise

### Requirement: Superproject: Outward actions belong to the owner
Outward actions SHALL belong to the owner: an agent never pushes to a webcpporg repository, never merges, never creates a remote repository, never changes a GitHub setting and never touches an SSH key or credential; an external contributor proposes a change by a pull request from a fork. Until the owner makes `webcpporg/<name>` (public from its first commit), a new library is a local git repository at `libs/<name>` (`git init`), which the lint and the build already treat as a library. A webcpp repository starts from one import commit, and the history of the code it was moved from is not carried over.

#### Scenario: A new library before its repository exists
- **WHEN** `libs/<name>` is a local `git init` with the checklist files
- **THEN** `b2 libs/<name>/test` builds it and the lint reads its tracked and untracked files

#### Scenario: A contribution
- **WHEN** an external contributor wants a change in a library
- **THEN** they fork the library's repository (or `webcpporg/webcpp` for the build, tools and docs) and open a pull request, two pull requests that name each other when a change spans a library and the superproject

### Requirement: Superproject: Text rules for every file
Everything written to a file SHALL be English: code, comments, documents and commit messages. U+2014 (the em dash) MUST NOT appear in any file; a plain dash is used. The word that the lint's banned-word pattern matches (the name of the private project webcpp was extracted from) MUST NOT appear, in any case, in a file, a file name or a commit (author, committer or message) of any repository. Every source file (`.hpp`, `.cpp`, `.py`, `.mjs`, `.js`, `.sh`, `.jam`, `Jamroot`, `Jamfile`, `build.jam`, `.yml`, `.adoc`, `.html`) opens with the licence notice, with `//` for C++, JavaScript and AsciiDoc and `#` for Jam, Python, shell and YAML (a `#!` line staying first), and an HTML file opens with a comment whose first line is `<!--` alone and whose next lines are the notice as it is.

#### Scenario: The notice in C++
- **WHEN** a header is created
- **THEN** it starts with `// Copyright (c) 2026 WebCpp.org`, a blank `//`, and the three lines "Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)"

#### Scenario: The notice in HTML
- **WHEN** `tools/doc/docinfo.html` is read
- **THEN** it opens with `<!--`, then `Copyright (c) 2026 WebCpp.org`, a blank line, the licence sentence, and a description of what the file is, inside the comment

### Requirement: Superproject: Process
Work SHALL follow the specification before the code and a failing test before the implementation: a behaviour is written down (in the page, or in the library's `AGENTS.md` for what is internal) before it is built and a test is seen failing before the code that makes it pass; a defect is first reproduced the way a user meets it (the command a user runs, the page a reader opens, the lane CI runs), then fixed, and the reproduction becomes a test; only a build from scratch (`b2 -a`) counts as evidence. "Green" before any merge MUST mean all of: every declared lane from scratch (native, wasip2 and wasip3 where declared, every own lane), `b2 -a doc`, `lint: clean`, the tests of the build and of the tools when they or what they test changed, and CI green for the library and the superproject. Lint, failures and flakiness met on the way MUST be fixed even when not caused by the change, and what cannot be fixed is reported. A change of the build or of a tool has a test that fails without it.

#### Scenario: A bug report
- **WHEN** a defect is reported in a lane
- **THEN** the lane is reproduced with its CI command first, the fix is made, and the reproduction is kept as a test

#### Scenario: An incremental build
- **WHEN** a result is claimed from an incremental `b2` run
- **THEN** it does not count, since b2 compares timestamps and an edit in the same second is missed

### Requirement: Superproject: Porting step by step
Porting a library SHALL follow six steps in order: choose it, check its licence and pin its version; make `libs/<name>` with its checklist files and see `b2 libs/<name>/test` build the headers-alone tests of an empty convenience header; port the original's tests as cases and the oracle that writes their expected results from the pinned original, and see them fail; port the code header by header with its Doc Comments until the cases pass natively and then on every declared target; write the page (guide with examples and twins, guarantees, testing chapter, reference, differences); run the definition of green from scratch.

#### Scenario: Step 2 produces a green skeleton
- **WHEN** a library has only its checklist files and an empty convenience header
- **THEN** `b2 libs/<name>/test` builds one headers-alone test for each public header and passes

### Requirement: Superproject: b2 facts
The build SHALL account for these measured b2 facts, which `AGENTS.md` chapter 12 records: every b2 run unsets `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` (the Jamroot refuses to run while one is set); a verdict is built from scratch with `-a`; `$(` inside an action is b2's, not the shell's, so shell command substitution is written with backquotes; an action of more than one command opens with `set -e`; `<build>no` is conditioned on the toolset, never on a derived feature such as `<target-os>`; a syntax error in a Jamfile only prints, so a target "not found" means the first lines of output are read; a subproject's target is named with its project (`b2 libs/xactor/test//scheduler`); `testing.launcher` applies to every toolset of one request, so a wasm lane and a native lane are two runs; b2's emscripten toolset runs a test with node itself and `testing.launcher=node` is refused; with `--out-xml` b2 exits 0 even when a test fails and the report's exit status is the verdict (it needs `--dump-tests`); b2 5.5.3's `--command-database` writes nothing so the compilation database comes from `tools/lint/compile_commands.py` reading `b2 -n -a`; every `-s` and environment variable is a global that `ARGV` alone tells apart; a free feature plays no part in a target's build directory; wasm32-wasip2 and wasm32-wasip3 are no system b2 knows, so the Jamroot adds `<target-os>wasi` after the toolsets load; a checkout path with a space builds; a target named `release-<word>`, `debug-<word>` or `profile-<word>` is no target, so the release check is `check-release`.

#### Scenario: A shell with CPATH
- **WHEN** b2 runs in a shell that sets `CPATH`
- **THEN** the Jamroot stops with "CPATH is set; unset it, or the compiler reads headers from it before the configured Boost"

#### Scenario: A release target name
- **WHEN** a target is declared `release-check`
- **THEN** b2 reads it as the variant `release` with a subfeature and stops, which is why the target is `check-release`

### Requirement: Build: One Jamroot, no library Jamroot
The build SHALL be b2 with the superproject's `Jamroot` as the single build configuration, as Boost's is: a library has no Jamroot, is developed inside a checkout of the superproject, and its `libs/<name>/build.jam` is found by the Jamroot's glob `libs/*/build.jam` and registered as `/webcpp/<name>` (`use-project` and `build-project`). A library whose submodule is not checked out has no `build.jam` and is not registered. The Jamroot MUST set, for every target, `<cxxstd>20`, `<warnings>extra` and `<warnings-as-errors>on`, `<link>static` for `<target-os>wasi` and for emscripten, and default to the `debug` variant with the build directory `bin`.

#### Scenario: A library is registered by its build.jam
- **WHEN** `libs/idna/build.jam` exists
- **THEN** `/webcpp/idna//idna` is a target any Jamfile can name, and `libs/idna/test`, `libs/idna/example` and `libs/idna/doc` join the aggregates `test`, `example` and `doc`

#### Scenario: Warnings are errors
- **WHEN** a header of a library triggers a `-Wextra` warning in a test
- **THEN** the build fails, since `<warnings-as-errors>on` applies to everything

### Requirement: Build: The check of Boost 1.92
The Jamroot SHALL find the installed Boost through `boost.use-project` (the `using boost` of `user-config.jam`, else `BOOST_ROOT`, else the compiler's default include path), read the version of the configured Boost from its `boost/version.hpp` with `grep`, or `findstr` on Windows, and refuse any version older than 1.92 (`109200`) by compiling `tools/test/boost_version.cpp` as a configuration check cached by a name that holds Boost's directory and version. When Boost cannot be used the build MUST stop before it compiles anything and print the exact `using boost` line to add, naming the keg when a package manager's Boost is installed (`/opt/homebrew/opt/boost`, `/usr/local/opt/boost`, `/opt/local`, `/usr/local`, `/usr`); a Boost whose `#define BOOST_VERSION <number>` line cannot be read stops the build too, naming the directory. Boost's headers are system headers: `-isystem` on gcc, `--system-header-prefix=boost/` on clang and emscripten, `/external:I` and `/external:W0` on MSVC, which also defines `BOOST_ALL_NO_LIB`.

#### Scenario: Homebrew's Boost is not configured
- **WHEN** Boost 1.92.0 is installed in `/opt/homebrew/opt/boost` and no `using boost` names it
- **THEN** b2 stops with "Boost 1.92 or newer was not found" and prints `using boost : 1.92 : <include>/opt/homebrew/opt/boost/include <library>/opt/homebrew/opt/boost/lib ;`

#### Scenario: An older Boost
- **WHEN** the configured Boost is 1.91
- **THEN** b2 stops with "Boost 1.92 or newer is required, and Boost 1.91.0 was found in <directory>"

### Requirement: Build: The aggregates and the explicit targets
The Jamroot SHALL declare the aggregates `test`, `example`, `doc` and `install` over the registered libraries' directories, the notfile targets `declared-targets`, `declared-lanes`, `site`, `site-serve`, `check-release`, `whats-new` and `dist` (the site targets when `site/` is checked out, else `site` and `site-serve` stop with "site/, the site's repository (webcpporg/webcpporg.github.io), is not checked out ... git submodule update --init site"), and the explicit targets `boost_version`, `throw_exception`, `boost_json`, `boost_url`, `boost_charconv`, `boost_locale`, `boost_cobalt` and `emscripten-reference`. `declared-targets` and `declared-lanes` MUST read only Jamfiles and need neither Boost nor Node: they leave the Boost check out with `<webcpp-boost-check>off`, as every target that compiles nothing does.

#### Scenario: declared-targets needs no Boost
- **WHEN** `b2 -d0 declared-targets` runs on a CI job that installed no Boost
- **THEN** it prints the `<library> <target>` pairs and nothing else

#### Scenario: The site is not checked out
- **WHEN** `b2 site` runs without `site/Jamfile`
- **THEN** it stops with the message that `site/` is not checked out and the command `git submodule update --init site`

### Requirement: Build: Targets and the toolsets that build them
A program SHALL be built for a target, one of `native`, `emscripten`, `wasip2` and `wasip3` (`tools/target.jam`'s `known`): `emscripten` is b2's `emscripten` toolset (wasm32, single-threaded, static, run by node itself); `wasip2` and `wasip3` are b2's `clang` registered against wasi-sdk with the versions `wasip2` and `wasip3` (toolsets `clang-wasip2` and `clang-wasip3`, run with `testing.launcher=wasmtime`); `native` is any other toolset (gcc, clang, msvc, darwin, and a clang without a version). The target of a build is told by `<toolset>` and its version alone, in one place, never by a derived feature. `webcpp.targets` declares a Jamfile's default targets, a program may override them with its own, and a program with no declaration is built for `native` only; for any other target the program gets `<build>no` and b2 skips it without a word.

#### Scenario: A clang without a version is native
- **WHEN** the toolset is `clang` with no version
- **THEN** `target-of` returns `native`, and a program declared for `native` is built

#### Scenario: A program is skipped for a target it did not declare
- **WHEN** a Jamfile declares `webcpp.targets native wasip2 wasip3` and `webcpp.run x : x.cpp : : native ;`
- **THEN** `x` is built only on native, and on `toolset=clang-wasip2` it gets `<build>no`

### Requirement: Build: The WASI toolsets
The Jamroot SHALL add `wasi` to the values of `<target-os>` (after the toolsets of `user-config.jam` have loaded, so that gcc's Unix link options `--start-group`, `-Bstatic` and rpath, which wasm-component-ld rejects, are never chosen), set the executable suffix to `.wasm` for `<target-os>wasi`, set `<toolset>clang-wasip2` and `<toolset>clang-wasip3` to `<target-os>wasi`, and make `<target-os>wasi` link statically. With exceptions on, `clang-wasip2` and `clang-wasip3` MUST compile with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` and link with `-fwasm-exceptions -lunwind` (the region `wasi-exceptions`), since wasi-sdk 34 emits the legacy encoding that wasmtime 47 refuses ("legacy_exceptions feature required"). A program that uses exceptions on wasip2 needs wasmtime 47 or later (37 to 46 with `-W exceptions=y`), jco with `jco transpile --bindgen-enable-wasm-exnref`, or another Wasm 3.0 engine.

#### Scenario: The suffix on WASI
- **WHEN** a test is linked with `toolset=clang-wasip2`
- **THEN** the executable is named `<name>.wasm`

#### Scenario: Exceptions on wasip3
- **WHEN** a program that throws is built with `toolset=clang-wasip3` and `exception-handling=on`
- **THEN** it is compiled with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` and linked with `-fwasm-exceptions -lunwind`

### Requirement: Build: The emscripten target
On emscripten the Jamroot SHALL build wasm32 (`<address-model>32`), single-threaded (`<threading>single`) and static (`<link>static`): browsers do not all run memory64 (an `address-model=64` request would add `-sMEMORY64=1`), pthreads need cross-origin isolation in a page, and Emscripten's shared libraries are experimental. b2's toolset runs a program's JavaScript with the node it found, so a build that also gives `testing.launcher` MUST stop with "webcpp: b2's emscripten toolset runs a program with node itself; give no testing.launcher on emscripten". Every program that node runs (`webcpp.run`, `webcpp.run-fail`, `webcpp.example`) links `-sNODERAWFS=1` so that it reads the host's files and its standard input; a program of `webcpp.link`, which a browser runs, and a driven program never get it. Emscripten writes its cache where `EM_CACHE` names, and every b2 of the tests gets `EMCC_SKIP_SANITY_CHECK=1`.

#### Scenario: A launcher on emscripten
- **WHEN** `b2 toolset=emscripten testing.launcher=node libs/trystero/test` runs
- **THEN** the build stops with the message that b2's emscripten toolset runs a program with node itself

#### Scenario: A test reads its input
- **WHEN** an example with `foo.input` is run on emscripten
- **THEN** it is linked with `-sNODERAWFS=1` and reads the file as its standard input, not an empty file system

### Requirement: Build: Exceptions and RTTI
Every target SHALL build with exceptions on: whether a program uses them is the choice of whoever builds it, never an imposition of a library or of webcpp; natively the compiler's default holds. A user's build without exceptions, `exception-handling=off`, MUST compile with `-fno-exceptions` on any target, Boost.Config then defines `BOOST_NO_EXCEPTIONS`, `webcpp.jam` links `tools/throw_exception.cpp` (a `boost::throw_exception` handler that prints and aborts, compiled once per variant as `/webcpp//throw_exception`) into the program, and MSVC also gets `_HAS_EXCEPTIONS=0`. RTTI is never restricted by webcpp on any target and no variant is built without it. A test or example that throws, tries or catches on purpose MUST declare `<exception-handling>on` in its requirements, which builds it with exceptions whatever the build asks.

#### Scenario: A test built without exceptions
- **WHEN** `b2 exception-handling=off libs/xactor/test` runs
- **THEN** each program is compiled with `-fno-exceptions` and linked with the throw_exception handler, and a program declared `<exception-handling>on` is still built with exceptions

#### Scenario: Boost.Locale needs exceptions
- **WHEN** a program names `/webcpp//boost_locale` without `<exception-handling>on` in a build with `exception-handling=off`
- **THEN** the build refuses by name, since Boost.Locale throws and catches unguarded

### Requirement: Build: Boost's compiled libraries from its sources
A compiled Boost library that a webcpp library needs SHALL be built by webcpp from Boost's own sources for every target, as `/webcpp//boost_url`, `/webcpp//boost_charconv`, `/webcpp//boost_locale` and `/webcpp//boost_cobalt` (`tools/boost/Jamfile`, a project `/webcpp/boost-libraries`, each Jamroot alias checking the sources first), static and built once per variant; a library adds e.g. `<library>/webcpp//boost_url` to its target's usage requirements and never `/boost//url` (which would link the system's) nor `/webcpp/boost-libraries//boost_url`. The sources are a tree with Boost's `boost/version.hpp` and `libs/<library>/src` for each library, found at `-sBOOST_SOURCE=<dir>` on b2's command line (which wins), else `BOOST_SOURCE` as a `user-config.jam` `modules.poke` or the environment gives it, else `.local/boost-source`, which `bash tools/ci/actions/boost/install.sh source .local/boost-source` fills from the archive the CI uses, checked against its SHA-256 and marked by `.webcpp-boost-source`; the script replaces only a directory that holds the marker and refuses any other by name before downloading. When a target needs the tree and it is missing, its version is not the configured Boost's, the configured version is unknown (Boost on the default include path), or it lacks the library's sources, the build MUST stop naming why and the fix.

#### Scenario: lexical links Boost.URL
- **WHEN** `libs/lexical/build.jam` names `<library>/webcpp//boost_url`
- **THEN** `b2 libs/lexical/test` builds Boost.URL's sources once per variant and links them statically

#### Scenario: Sources of another version
- **WHEN** `.local/boost-source` holds Boost 1.91's sources and the configured Boost is 1.92.0
- **THEN** the build stops naming both versions

#### Scenario: A user's tree is never replaced
- **WHEN** `install.sh source <dir>` is given a directory with no marker, such as an installed Boost's include directory
- **THEN** it refuses by name and downloads nothing

### Requirement: Build: What Boost.Locale and Boost.Charconv give on each target
`/webcpp//boost_locale` SHALL be Boost.Locale without ICU and `/webcpp//boost_charconv` Boost.Charconv, with these behaviours (`AGENTS.md` chapter 2): native Linux builds the posix and std backends with glibc's iconv; native macOS the posix and std backends with the system's libiconv given as `-liconv` by the target; native Windows (MSVC) the winapi and std backends, not measured until the first CI run that builds it; wasip2 and wasip3 the posix and std backends with wasi-libc's iconv, formatting in the C locale's form, a catalog read from a directory the test gives with `webcpp.data`; emscripten the posix and std backends with Emscripten's iconv, single-threaded, Boost.Locale's sources alone compiled with `BOOST_HAS_PTHREADS` so Boost.Thread's mutex builds, with no public header of Boost.Locale depending on it (checked by `tools/test/boost_libraries_test.py`); any target with `exception-handling=off` refuses Boost.Locale by name (61 `throw`, 14 `try` in its sources, 8 `throw` in its headers) while Boost.Charconv builds. Boost.Cobalt is built for native programs and has no build without exceptions.

#### Scenario: Locale on wasip2
- **WHEN** a test using Boost.Locale runs with `toolset=clang-wasip2`
- **THEN** it formats numbers, currencies and dates in the C locale's form and reads a message catalog from the directory given by `webcpp.data`

#### Scenario: Charconv without exceptions
- **WHEN** `/webcpp//boost_charconv` is built with `exception-handling=off`
- **THEN** it builds, as measured natively and on wasip2

### Requirement: Build: Boost.JSON, Boost.Test and the throw handler as objects
The Jamroot SHALL provide `/webcpp//boost_json` (a static library of `tools/boost_json.cpp`, which compiles Boost.JSON's definitions with `<boost/json/src.hpp>` for a header-only build, once per variant so that a user's `exception-handling=off` reaches it) and `/webcpp//throw_exception`; `tools/boost_test_runner.cpp` compiles Boost.Test's header-only framework as one object of a `webcpp.boost-test` suite, always with exceptions, so that the CI installs no compiled `unit_test_framework`. A library that needs Boost.JSON puts `<library>/webcpp//boost_json` in its target's usage requirements, as xstate's `build.jam` does.

#### Scenario: xstate links Boost.JSON
- **WHEN** a program names `/webcpp/xstate//xstate`
- **THEN** `/webcpp//boost_json` is built once per variant and linked statically

#### Scenario: A Boost.Test suite
- **WHEN** a Jamfile declares `webcpp.boost-test machine : machine_test.cpp ;`
- **THEN** the suite is built native only, whatever the Jamfile's targets, with the framework object, and run with `--log_level=error --color_output=no`

### Requirement: Build: The Jamfile API of tests and examples
A library's test and example Jamfiles SHALL declare their programs only with the rules of `tools/webcpp.jam`, imported as `webcpp`: `webcpp.targets`, `webcpp.run`, `webcpp.run-fail`, `webcpp.compile`, `webcpp.compile-fail`, `webcpp.compile-diagnostic`, `webcpp.boost-test`, `webcpp.example`, `webcpp.headers-alone`, `webcpp.link`, `webcpp.data`, and the lane, oracle, component, drive, generated, external, documentation and index rules named in their own requirements; the lint rejects b2's own `run`, `run-fail`, `compile`, `compile-fail`, `exe`, `unit-test`, `link` and `link-fail` there, by file and line. A program is `<name> : <sources> + : <requirements> * : <targets> *`; `webcpp.targets` MUST be called before the Jamfile's first program, once, and each value is `native`, `emscripten`, `wasip2` or `wasip3`, or the build stops naming it.

#### Scenario: xactor's test Jamfile
- **WHEN** `libs/xactor/test/Jamfile` is read
- **THEN** it starts with `project : requirements <library>/webcpp/xactor//xactor ;`, imports `webcpp`, declares `webcpp.targets native wasip2 wasip3 ;`, calls `webcpp.headers-alone xactor : ../include ;` and declares programs such as `webcpp.run scheduler : scheduler_test.cpp ;`

#### Scenario: An unknown target
- **WHEN** a Jamfile writes `webcpp.targets native ios ;`
- **THEN** the build stops naming `ios`

### Requirement: Build: webcpp.run and webcpp.run-fail
`webcpp.run` SHALL pass when the program exits with 0 and `webcpp.run-fail` when it exits with another status; each is built once per target it declares, links `/webcpp//throw_exception` in a user's build without exceptions, and on emscripten is run by node with `-sNODERAWFS=1`. A test that reads files at run time MUST be given their directory by a `path-constant` or a `WEBCPP_TEST_*` define with forward slashes, quoted whole, with `[ webcpp.data <directory> ]` in its requirements and a `<dependency>` on the files it reads so that a changed file runs it again.

#### Scenario: A passing and a failing program
- **WHEN** a program returns `boost::report_errors()`
- **THEN** `webcpp.run` passes at 0 and fails at any other status, and `webcpp.run-fail` passes only at a non-zero status

### Requirement: Build: webcpp.compile, compile-fail and compile-diagnostic
`webcpp.compile` SHALL pass when the sources compile (no program is linked), `webcpp.compile-fail` when they do not compile (any error passes, the wrong one included), and `webcpp.compile-diagnostic` when the sources stop with every error stated in clang's `-verify` comments, `// expected-error@<file>:* {{<message>}}` for a header they include or `// expected-error@+1 {{<message>}}` for the next line. `compile-diagnostic` MUST be compiled with `-Xclang -verify -Xclang -verify-ignore-unexpected=error,note` on b2's `clang` toolset only (native clang, `clang-wasip2`, `clang-wasip3`) and be skipped elsewhere, since `-verify` is clang's; both `compile-fail` and `compile-diagnostic` are left out of clang-tidy only, and every other lint rule reads them.

#### Scenario: A compile diagnostic on gcc
- **WHEN** a `webcpp.compile-diagnostic` test is built with gcc
- **THEN** it is skipped, so that a refusal that holds does not turn every gcc and msvc lane red

#### Scenario: A compile-fail test
- **WHEN** the sources of a `webcpp.compile-fail` test compile
- **THEN** the test fails

### Requirement: Build: webcpp.example
`webcpp.example` SHALL pass when the program exits with 0 and its standard output, with carriage returns removed, equals `<stem>.expected` beside it; its standard input is `<stem>.input` beside it, else empty, never the terminal, on every target (wasmtime passes it through, node under `-sNODERAWFS=1`). It is run through `testing.launcher` for WASI, on emscripten with the node on `PATH` by `tools/example/run_example.py`, which names a failing exit status (or the signal) before the diff, it is always run again, and `<stem>.input` is a source of the run: a free feature plays no part in a target's build directory, so an output from one launcher would otherwise read as up to date under another.

#### Scenario: An example compared with its expected output
- **WHEN** `example/xactor_quick_start.cpp` prints text that differs from `example/xactor_quick_start.expected`
- **THEN** the example fails and the diff is shown after the exit status

#### Scenario: An example with input
- **WHEN** `lexical_quick_start.input` exists
- **THEN** the program reads it as standard input on native, wasip2, wasip3 and emscripten

### Requirement: Build: webcpp.headers-alone
`webcpp.headers-alone <library> : <include-root> : <only> * : <requirements> * : <targets> *` SHALL pass when each public header compiles alone: one test per header named `alone-<path>` with `/` written `-` (`alone-xactor-scheduler`), against `/webcpp/<library>//<library>` and the requirements given, for the targets given or the Jamfile's. `only` restricts a call to the public headers its globs match, relative to `<include-root>/webcpp/` (`wasi/http/response.hpp`), and a glob that matches none stops the build. A library may call it several times; a header two calls take stops the build, naming it. Every library MUST compile its headers alone.

#### Scenario: Headers-alone tests of xactor
- **WHEN** `webcpp.headers-alone xactor : ../include ;` is declared
- **THEN** there is a test `alone-xactor-<header>` for every public header under `../include/webcpp/xactor/` and for `webcpp/xactor.hpp`

#### Scenario: A header taken by two calls
- **WHEN** two calls of `webcpp.headers-alone` both match `webcpp/wasi/http/response.hpp`
- **THEN** the build stops, naming the header

### Requirement: Build: webcpp.link and webcpp.data
`webcpp.link` SHALL pass when the program links and never runs it: it is for a program a browser runs (`-sENVIRONMENT=web`), which node refuses, or one that needs the network; b2's `link` test, listed by `--dump-tests` and shown by the report as a link test, never linked with `-sNODERAWFS=1`; the explicit target `<name>-program` is the same program (`<name>-program.js` on emscripten) for a target that needs its files. An example Jamfile's linked program is shown on the page by its code and has no `.expected`. `webcpp.data <directory> +` SHALL be requirements that give a program of `webcpp.run` or `webcpp.run-fail` absolute directories, each read at the same path on every target: on wasip2 and wasip3 wasmtime preopens it (`wasmtime --dir="<directory>" <program>`), natively and on emscripten the program reads the host's files already; a relative directory stops the build, as does a path holding `"`, `$` or a backquote, naming the character.

#### Scenario: A page program that is only linked
- **WHEN** `libs/trystero/example` declares a program with `webcpp.link`
- **THEN** it is linked on emscripten, never run, and the page shows its code with no output

#### Scenario: A data directory with a dollar sign
- **WHEN** `webcpp.data` is given a directory whose path holds `$`
- **THEN** the build stops, naming the character the shell would read inside `--dir="..."`

### Requirement: Build: WebAssembly components
A library that builds WebAssembly components SHALL declare the C bindings of a world in its `build.jam` with `webcpp.wit-bindings <name> : <world-file> : <world> : <rename> : <version> : <wit-bindgen-argument> *`, which generates, with wit-bindgen, the world `<world>` of `<world-file>` renamed `<rename>` (`--rename-world`) for WASI version `p2` or `p3`: on that version's toolset alone (`clang-wasip2`, `clang-wasip3`) the usage requirements put the bindings' directory on the include path and link `<rename>.c` and `<rename>_component_type.o`, elsewhere they add nothing so that a native build needs no wit-bindgen; the explicit target `<name>-headers` puts the directory on the include path on any toolset, for a native parse such as the reference. The bindings are generated while b2 computes a target's properties (in a dry run too, safely when loads are concurrent) under `<build-dir>/generated/<library>/<name>/`, and again only when the world file, the WIT, wit-bindgen's version or an argument changed, compared by content. A name declared twice in a library stops the build, naming both. The tools of a component are looked up only when bindings are generated and when a served test runs.

#### Scenario: Bindings on wasip2 only
- **WHEN** wasi's `build.jam` declares its worlds and a native `b2 libs/wasi/test` runs
- **THEN** wit-bindgen is not run and no bindings reach the include path

#### Scenario: Concurrent loads
- **WHEN** two b2 runs load a Jamfile that declares the same bindings at once
- **THEN** the bindings are generated safely, each directory written by one and read by the other

### Requirement: Build: webcpp.serve and webcpp.serve-script
`webcpp.serve <source> : <requirements> * : <targets> *` SHALL pass when the transcript of the answers to the requests of `<stem>.requests` equals `<stem>.expected`: it builds `<source>` as a reactor component that exports an HTTP handler, for its own targets (which may be only wasip2 and wasip3) else for those of its Jamfile that are, linking the throw handler in a user's build without exceptions, and `tools/component/serve.py` serves it with `wasmtime serve` (`-S cli` on wasip2, `-S cli,p3 -W component-model-async` on wasip3) on a port the system chooses, sends each request on a connection of its own and stops wasmtime in every outcome, a SIGKILL of serve.py included (wasmtime runs under a keeper that kills it when serve.py's pipe to it ends). `webcpp.serve-script <name> : <script> : <stem> : <targets> *` is the same for a component the shell script `<script>` builds, run as `sh <script> <p2|p3> <component>` with `WASI_SDK`, `WIT_BINDGEN` and `WASI_WIT` in its environment, at every build. `<stem>.requests` holds one request per line, `<METHOD> <target>`; the transcript holds, per request, the `$ ` curl command a shell runs as written, the status line, the headers in lower case, sorted, without `date` and `transfer-encoding`, an empty line and the body, with the origin written `http://localhost:8080` whatever port was served. Both are tests of b2's, listed by `--dump-tests` so the report sees them fail, and run in an own lane, never in the ordinary ones.

#### Scenario: A served transcript
- **WHEN** a served component answers `GET /a?b=c%20d` with `GET /a?b=c%20d` as a text body
- **THEN** the transcript holds `$ curl -i -X GET 'http://localhost:8080/a?b=c%20d'`, `HTTP/1.1 200 OK`, `content-type: text/plain; method=GET`, an empty line and `GET /a?b=c%20d`

#### Scenario: wasmtime is killed with serve.py
- **WHEN** serve.py is killed with SIGKILL while a component is served
- **THEN** the keeper kills wasmtime when its pipe ends and no process is left

### Requirement: Build: webcpp.drive
`webcpp.drive <name> : <sources> + : <requirements> * : <script> : <argument> * : <targets> *` SHALL pass when `<original words> <script> <arguments> <program>` exits with 0 within its bound: it builds the program from the sources, for its own targets else its Jamfile's, and runs the script beside the Jamfile with the words of `webcpp.original`, the arguments and the path of the linked program (its JavaScript on emscripten, its executable natively). `tools/drive/drive.py` runs it under serve.py's keeper, as the leader of a process group of its own, bounded by 30 seconds or the seconds `<webcpp-drive-timeout>` gives; a driver that has not ended by then is stopped with every process of its group and the test fails naming it, the program and the bound; the group is also killed once the driver ends and when `drive.py` itself is killed. It depends on the Jamfile's `node-modules`, a Jamfile without `webcpp.original` stops at it by name, it is b2's test `DRIVE` (the report sees it fail), always run again, and a lane-only program.

#### Scenario: A driver that hangs
- **WHEN** a driver does not end within 30 seconds
- **THEN** its process group is killed and the test fails naming the driver, the program and the bound

#### Scenario: browser_demo's driver
- **WHEN** `tools/test/fixtures/browser_demo/test/driver/Jamfile` is read
- **THEN** it declares `webcpp.targets native emscripten ;`, `webcpp.original node ;`, `webcpp.drive driven : driven.cpp : : drive.mjs : --program ;` and `webcpp.lane driver : driven : native emscripten ;`

### Requirement: Build: Libraries webcpp does not build
A library's `build.jam` SHALL declare each library webcpp does not build with `webcpp.external <owner> : <library> : <variable> : <directory> : <header> : <target>`, which returns the directory its lib targets search (`-s<variable>=<dir>` on b2's command line, the last one, a relative one made absolute against b2's working directory, never the environment; else `.local/<directory>` when that holds `include/<header>`; else nothing, so that the compiler finds it on the system), check them with `webcpp.external-found <owner> : <library> + : <properties> *` (nothing when each library is there; a `-s` naming a directory without the header stops the build; when none is found, `<build>no` so that the owner's targets are skipped and one line `webcpp: <owner> is left out: <library> was not found: ...` names every place looked, once per owner and library; with `webcpp-require-external=on`, as the CI and the lint build, the build stops instead, naming the owner and the library), and name the owner's page left out with `webcpp.left-out <owner>`, which makes the index's table name the library with the reason and no link. Natively outside Windows a system install in `/usr/include` or `/usr/local/include` is legitimate.

#### Scenario: trystero without its libraries
- **WHEN** `b2 test` runs on a machine without libsecp256k1
- **THEN** trystero is left out with one line naming the library and every place looked, and every other library builds

#### Scenario: The CI requires them
- **WHEN** a lane builds with `webcpp-require-external=on` and libdatachannel is not found
- **THEN** the build stops naming trystero and libdatachannel

### Requirement: Build: Generated files
A file that a script writes from inputs it pins, such as a table of Unicode data, SHALL be committed and written only by its generator. A library's test Jamfile declares it with `webcpp.generated <name> : <script> : <argument> * : <output> +`, which declares the explicit target `generated-<name>`: `tools/generated/generated.py` runs the Python script in the Jamfile's directory as `<script> <argument>... <path>...`, each path where it writes one output under the Jamfile's build directory, and compares each with its committed file byte for byte (`tools/oracle/compare.py --files`), naming every one that differs, with the command that writes it again, `b2 libs/<name>/test//update-generated`, every one that is missing and every one the generator did not write; it writes nothing into the tree. The first declaration of a Jamfile also declares `update-generated`, the only writer, which runs each generator in a build directory apart from the check's and, once all succeeded, copies the outputs beside the committed files and renames each over its own (a failure leaves every committed file as it was). Paths are relative to the Jamfile; a script that is not a `.py` file or does not exist, a name declared twice and an output named twice stop the build. An own lane names the check, `webcpp.lane <lane> : generated-<name> ;`, of the kind `programs`; the ordinary lanes never build it since a generator may need the network; neither target compiles anything, so they leave the Jamroot's check of Boost out.

#### Scenario: A generated table drifts
- **WHEN** a committed table differs from what its generator writes
- **THEN** `generated-<name>` fails naming the file and the command `b2 libs/<name>/test//update-generated`

#### Scenario: Two generators write one output
- **WHEN** two `webcpp.generated` declarations of a Jamfile name the same output
- **THEN** the build stops

### Requirement: Build: The release and version targets
The Jamroot SHALL declare `check-release`, `whats-new` and `dist` as notfile targets that run only when named, calling `tools/release/release.py check`, `whats-new` and `dist` with `--root`, `--version` and (for dist) `--out bin/dist`; `dist` depends on `doc`. The version is read from the one `#define WEBCPP_VERSION <number>` line of `libs/config/include/webcpp/version.hpp`, the number alone in decimal without a leading zero and with nothing after it but spaces (a trailing comment is refused), as major, minor (three digits) and patch (two): `100` is 0.1.0. A checkout without `libs/config` builds everything else; the release targets stop, naming the header, when it is missing, cannot be read, defines the macro twice or has no such line.

#### Scenario: The version of 0.1.0
- **WHEN** `version.hpp` holds `#define WEBCPP_VERSION 100`
- **THEN** the Jamroot reads `100 0.1.0` and `check-release` passes `--version 0.1.0`

#### Scenario: A version with a trailing comment
- **WHEN** the `#define WEBCPP_VERSION` line is followed by a comment
- **THEN** the release targets stop, saying the header has no line `#define WEBCPP_VERSION <number>`, the number in decimal, without a leading zero

### Requirement: Library: Choosing a port
A port SHALL be of a library widely used for the web in another language, at an exact version of the original (a tag or published release) recorded in `meta/libraries.json` and in the library's `AGENTS.md`; the port follows that version and moving to another is a change of its own, with its evidence. The original's licence MUST be compatible with BSL-1.0: MIT, BSD and Apache-2.0 can be ported, GPL cannot, MPL is decided case by case by the owner. A port that derives from the original's code keeps the original's notice, unchanged, in `LICENSE-<ORIGIN>.txt` (`LICENSE-XSTATE.txt`, `LICENSE-TRYSTERO.txt`) and its README names the parts that derive from it; this adds no licence to webcpp's code: every file of webcpp is BSL-1.0 only, no repository has a `LICENSE-MIT.txt`, and a library of webcpp's own (xactor, pratt) has no `LICENSE-<ORIGIN>.txt`.

#### Scenario: A GPL original
- **WHEN** a library under the GPL is proposed for porting
- **THEN** it cannot be ported

#### Scenario: trystero keeps Trystero's notice
- **WHEN** `libs/trystero/` is read
- **THEN** it holds `LICENSE_1_0.txt` and `LICENSE-TRYSTERO.txt`, and `b2 install` copies both beside its headers

### Requirement: Library: Names
A library's name SHALL be `[a-z][a-z0-9_]*`, the name of the original where it has one (xstate, trystero); `index` is reserved in any case (the index page is the doc build's page `index`, and the report refuses a library of that name), and so are `target`, `has`, `no`, `lib` and `version`, which config's macros take (`WEBCPP_TARGET_*`, `WEBCPP_HAS_EXCEPTIONS`, `WEBCPP_LIB_VERSION`, `WEBCPP_VERSION`), and `test`, which the test-only macros take (`WEBCPP_TEST_*`), each alone or before `_`. The names MUST follow this table, `<name>` being the library's: namespace `webcpp::<name>`; what is not public in a namespace `detail` at any depth; convenience header `<webcpp/<name>.hpp>`; headers `<webcpp/<name>/...>`, one per responsibility, `snake_case`; macros `WEBCPP_<NAME>_*`; configuration `<webcpp/<name>/config.hpp>`; test-only macros `WEBCPP_TEST_*`; include guards `WEBCPP_<NAME>_<HEADER>_HPP`; b2 target `/webcpp/<name>//<name>`; error category `webcpp.<name>`; identifiers `snake_case`, enforced by `.clang-tidy`. A test's helpers shared between tests go in `webcpp::test`, and example programs alias the namespace (`namespace xactor = webcpp::xactor;`).

#### Scenario: xactor's names
- **WHEN** xactor is inspected
- **THEN** its namespace is `webcpp::xactor`, its detail namespace `webcpp::xactor::detail`, its convenience header `<webcpp/xactor.hpp>`, a header `<webcpp/xactor/scheduler.hpp>`, macros `WEBCPP_XACTOR_*`, a guard `WEBCPP_XACTOR_ACTOR_LOGIC_HPP`, its target `/webcpp/xactor//xactor` and its error category `webcpp.xactor`

#### Scenario: A reserved name
- **WHEN** a library is named `version` or `test_utils`
- **THEN** the index refuses it, since `version` and `test` (alone or before `_`) are reserved

### Requirement: Library: The files of a library
A library `libs/<name>/` SHALL be a repository of its own with: `build.jam` (project `/webcpp/<name>`), `include/webcpp/<name>.hpp` and `include/webcpp/<name>/...`, `wit/` for a library that builds components, `test/` (a `Jamfile`, the tests, `.clang-tidy` when the tests need one, `test/oracle/` for a port's oracle), `example/` (a `Jamfile`, the programs and their `.expected` outputs, a served program's `.requests`), `doc/` (a `Jamfile`, the page `<name>.adoc` and its sections, `mrdocs.yml`, `counts.py` when the page counts what only it holds, and `changelog.adoc` as the page's last appendix), `meta/libraries.json`, `meta/include-boundaries.json` when it has boundaries, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`, `LICENSE-<ORIGIN>.txt` for a port that derives from code, `.gitignore` (at least `doc/html/`, and `node_modules` and `.node-modules/` with an oracle), `.gitattributes`, an optional `.clang-format` that inherits the root's (`InheritParentConfig`) and adds only what its code needs with the reason in a comment (wasi names the macros its main is written between with `MacroBlockBegin` and `MacroBlockEnd`), and `.github/workflows/ci.yml`. A library has no Jamroot. The `README.md` MUST say what the library is, how to build and test it inside the superproject, where its page is published and its licence; its `AGENTS.md` holds only what is specific to it, with a link to `../../AGENTS.md`.

#### Scenario: lexical's tree
- **WHEN** `libs/lexical` is listed at `e286ca4`
- **THEN** it holds `AGENTS.md`, `build.jam`, `doc`, `example`, `include`, `LICENSE_1_0.txt`, `meta`, `README.md`, `test` and `tools`, and its `.gitignore` holds `doc/html/`, `node_modules` and `.node-modules/`

#### Scenario: A library with no Jamroot
- **WHEN** a contributor adds `libs/<name>/Jamroot`
- **THEN** it is wrong: the superproject's Jamroot is every library's build configuration

### Requirement: Library: The checklist of a new library
A new library SHALL complete nine items before its first green CI, the first two being the owner's: the repository `webcpporg/<name>`, public from its first commit (an agent never creates a remote repository); the submodule `git submodule add ../<name>.git libs/<name>`; `meta/libraries.json`; `build.jam`; the `test`, `example` and `doc` Jamfiles; the files of the repository (`LICENSE_1_0.txt`, `README.md`, `AGENTS.md`, `doc/changelog.adoc`, `.gitattributes`, `.gitignore`); the CI file; the index page entry, which exists once `meta/libraries.json` does (and its line under New Libraries in the release notes of the version in preparation), a library without a page failing the index; and `LICENSE-<ORIGIN>.txt` where the port derives from code. Nothing else registers a library: the test, example and doc directories join the aggregates by themselves, and `include/webcpp/**` joins `install`.

#### Scenario: The index lists a new library
- **WHEN** `libs/<name>/meta/libraries.json` exists with a page declared
- **THEN** `b2 doc` shows the library in `doc/html/index.html`, linked to its page

#### Scenario: A library without a page
- **WHEN** a library has `meta/libraries.json` and no `webcpp.doc` in its doc Jamfile
- **THEN** the index build fails naming the library

### Requirement: Library: meta/libraries.json
Each library's `meta/libraries.json` SHALL be Boost's file with webcpp's `"port-of"`: one object with `key` (equal to the directory's name), `name`, `authors` (`["R"]`), `description` (one sentence), `category` (a list of at least one string), `maintainers` (`["R <github.overboard312@passmail.net>"]`), `cxxstd` (`"20"`) and `port-of`, which is `null` for a library of webcpp's own and otherwise an object with `name`, `language`, `version`, `url` (an `https` URL) and `licence`. `name`, `description` and `port-of` are required, and every field of `port-of`; `tools/doc/libraries.py` MUST fail the doc build, naming the file and the field, otherwise, and a list of more than one object is an error (webcpp holds one library per directory).

#### Scenario: lexical's metadata
- **WHEN** `libs/lexical/meta/libraries.json` is read
- **THEN** its key is `lexical`, its category `["Parsing", "String"]`, its `cxxstd` `"20"` and its `port-of` `null`

#### Scenario: A port's metadata
- **WHEN** xstate's `port-of` is read
- **THEN** it names XState, TypeScript, 5.33.2, `https://github.com/statelyai/xstate` and MIT

#### Scenario: A malformed port-of
- **WHEN** a `port-of` lacks its `licence`
- **THEN** the doc build fails naming the file and the field

### Requirement: Library: build.jam
A library's `build.jam` SHALL start with the licence notice, declare `project /webcpp/<name> ;` and one target, named after the library, that adds its headers and dependencies: `alias <name> : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;`. Every library adds config's target since its `config.hpp` includes `<webcpp/config.hpp>`; a dependency on another library is its target (`<library>/webcpp/xactor//xactor`); Boost.URL, Boost.Charconv, Boost.Locale and Boost.Cobalt are `<library>/webcpp//boost_url`, `//boost_charconv`, `//boost_locale` and `//boost_cobalt`; Boost.JSON is `<library>/webcpp//boost_json`; a library that builds components declares its worlds' bindings there with `webcpp.wit-bindings`. The dependencies SHOULD sit between `tag::dependencies[]` and `end::dependencies[]` markers which the page includes with `include::{library-root}/build.jam[tag=dependencies]`.

#### Scenario: config's build.jam
- **WHEN** `libs/config/build.jam` is read
- **THEN** it declares `project /webcpp/config ;` and `alias config : : : : <include>include <library>/boost//headers ;`

#### Scenario: A dependency on another library
- **WHEN** xstate's actor layer depends on xactor
- **THEN** its target lists `<library>/webcpp/xactor//xactor` among its usage requirements

### Requirement: Library: The configuration header and no-exceptions macro
Each library SHALL have `<webcpp/<name>/config.hpp>`, which the convenience header and every public header include first, which includes `<webcpp/config.hpp>` first, and which defines `WEBCPP_<NAME>_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined and the macro is not already defined (`#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_<NAME>_NO_EXCEPTIONS)`): when the code is compiled without exceptions (`BOOST_NO_EXCEPTIONS`), when the developer defines `WEBCPP_NO_EXCEPTIONS` to disable every library's at once, or when the developer defines the library's macro alone, to turn one library's exceptions off in a build that has them. A program sets `WEBCPP_NO_EXCEPTIONS` for every library alike without knowing which one throws, so a library that raises no exception of its own has the macro too and says so in its Doc Comment. Because MrDocs parses with exceptions, `config.hpp` defines the macro for the reference under `#ifdef __MRDOCS__` and undefines it at once. The library's `test/config_test.cpp` MUST be built three times: as the build asks (`config`, where the macro is defined exactly when `WEBCPP_HAS_EXCEPTIONS` is not), with `WEBCPP_NO_EXCEPTIONS` defined (`config_all_off`) and with the macro itself defined on the command line (`config_developer_defined`).

#### Scenario: lexical's configuration
- **WHEN** `libs/lexical/include/webcpp/lexical/config.hpp` is read
- **THEN** it includes `<webcpp/config.hpp>`, documents that lexical raises no exception of its own, and defines `WEBCPP_LEXICAL_NO_EXCEPTIONS` under `#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_LEXICAL_NO_EXCEPTIONS)`

#### Scenario: The three builds of config_test
- **WHEN** `libs/lexical/test/Jamfile` is read
- **THEN** it declares `webcpp.compile config`, `config_developer_defined` and `config_all_off` over `config_test.cpp`

### Requirement: Library: Raising exceptions
A library SHALL raise an exception only through `boost::throw_exception`, never a bare `throw`, and every API that throws MUST sit behind `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, so that code using it does not compile without exceptions. Its headers compile with `-fno-exceptions`: what throws, tries or catches sits behind that macro, and a failure that cannot be returned goes through `boost::throw_exception`. The one exception is a header that wraps a dependency which reports errors by throwing: its library declares it with `<exception-handling>on` and a comment of the reason on each headers-alone call, test and program of the backend that brings the dependency (b2 lets no usage requirement override a build's `exception-handling=off`), the lint leaves that header out of the analysis without exceptions and names it, and the header declares nothing without exceptions: what follows its `config.hpp` sits inside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, with a comment naming the dependency, so a program built without exceptions that uses it fails on the first name it uses while one that only includes it compiles. wasi, which includes nothing of Boost, reads `WEBCPP_NO_EXCEPTIONS` itself until its redesign includes `<webcpp/config.hpp>`.

#### Scenario: A throwing API behind the macro
- **WHEN** a header declares a function that throws
- **THEN** it is inside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS`, raises through `boost::throw_exception`, and a program built without exceptions fails to compile if it names it

#### Scenario: A rethrow
- **WHEN** a header needs to rethrow
- **THEN** it uses `std::rethrow_exception`, since the lint's bare throw rule refuses `throw;` too

### Requirement: Library: The convenience header and optional heavy dependencies
`<webcpp/<name>.hpp>` SHALL include every public header of the library but may leave out one that brings an optional heavy dependency a user of the rest should not pay for, and MUST name in its comment the header it leaves out and the dependency: `<webcpp/pratt.hpp>` includes pratt's engine and leaves out `<webcpp/pratt/calculator.hpp>`, which brings Boost.Decimal; `<webcpp/lexical.hpp>` leaves out `regex.hpp`, which brings the Unicode tables of ECMA-262's classes. Every public header compiles alone (the headers-alone tests).

#### Scenario: pratt's convenience header
- **WHEN** a program includes only `<webcpp/pratt.hpp>`
- **THEN** it does not include `<webcpp/pratt/calculator.hpp>` and compiles without Boost.Decimal

### Requirement: Library: Include boundaries
A library MAY declare in `meta/include-boundaries.json` the include boundaries the lint holds it to: one key, `boundaries`, a list of objects with `headers` (globs matched against the whole path under `libs/<name>/`, a `*` within one path segment), an optional `except` (globs), `must-not-include` (prefixes of included paths) and `why`. Every header the globs match, less the excepted, MUST NOT `#include <...>` or `#include "..."` a path starting with a prefix of `must-not-include`; the lint reports an offending `#include` at its line with the boundary's reason, and a malformed file, an unknown key or a glob that matches no file of the library as a finding naming the file. A library without the file passes silently.

#### Scenario: xstate's machine core
- **WHEN** xstate's boundary has `headers` `["include/webcpp/xstate.hpp", "include/webcpp/xstate/*.hpp"]`, `except` `["include/webcpp/xstate/actors.hpp"]` and `must-not-include` `["webcpp/xactor", "webcpp/xstate/actors"]`
- **THEN** a core header that includes `webcpp/xactor/scheduler.hpp` fails the lint with "the machine core runs without xactor and the actor layer"

#### Scenario: A glob that matches nothing
- **WHEN** a boundary's `headers` glob matches no file
- **THEN** the lint fails naming `meta/include-boundaries.json` and the glob

### Requirement: Library: The language and what a library builds on
A library SHALL be C++20 and header-only, and build on, in this order: what webcpp already has; then Boost; then the C++ Alliance's libraries; then the standard library; then a library from outside that follows Boost, the standard library and the standards of its field; then a C++ port of a very widely used C library; and only then an implementation of its own. The same order governs each function an implementation of its own writes (a hash is Boost.Hash2's before anything else, a JSON parser Boost.JSON's), and within Boost the newest library of a kind is preferred (Coroutine2 over Coroutine, Variant2 over Variant, Hash2 over ContainerHash for new hashing, Unordered's flat maps, Charconv over a hand-written conversion). A library from outside comes in after the owner's review with the reason in the library's documentation, and MUST be wrapped behind an interface that follows its counterpart in the standard library or Boost, so that it is an implementation detail the user never names and another implementation can replace it (trystero hides libdatachannel behind its backend interface, with the browser's WebRTC as the other backend). Every library is written to be taken to the web: its memory, I/O and scheduling are its user's to customize, and on emscripten it uses what the browser offers.

#### Scenario: A hash in a new library
- **WHEN** a library needs a hash
- **THEN** it uses Boost.Hash2 before any custom hash

#### Scenario: OpenSSL
- **WHEN** a library needs OpenSSL
- **THEN** it comes in after the owner's review with the reason in its documentation, because OpenSSL is the standard of its field

### Requirement: Library: Types, errors and state
Integers whose width matters SHALL be the fixed-width types of `<cstdint>`, a size or an index `std::size_t`, a difference `std::ptrdiff_t`, and plain `int`, `long`, `short` and `unsigned` appear only where an interface of the standard library or Boost asks for them. An operation that can fail MUST return its error as a value, `boost::system::result<T>`, with an error category of the library's own (`webcpp.<name>`) and fixed enumerator values that are never reused (the enum is `int`-based since `error_code::value()` is `int`). A library has no global or static mutable state (`cppcoreguidelines-avoid-non-const-global-variables`). Text is passed and held as `std::string_view` where nothing must own it and `std::string` only where something does.

#### Scenario: An error category
- **WHEN** a library's operation fails
- **THEN** the error is a `boost::system::error_code` of the category named `webcpp.<name>`, and a code's value is never reused for another meaning

### Requirement: Library: Determinism and the world rule
A library SHALL be deterministic: no clock, file, network, process, thread, environment or entropy, unless that is the library's purpose. The lint's world rule finds them by name in every library header (a clock, a file, a socket, a process, a thread, the environment, `random_device`, `getentropy`, `getrandom`, `arc4random`, OpenSSL's `RAND_bytes`, `<sys/random.h>`); a line that names one without reaching the world (a simulated clock), or that reaches it because it is the library's purpose, MUST say why after `lint-world:` on the same line, as in `#include <boost/asio/io_context.hpp>  // lint-world: posts handlers only`.

#### Scenario: A header names a clock
- **WHEN** a library header contains `std::chrono::steady_clock` with no `lint-world:` on the line
- **THEN** the lint fails the rule "no clock, disk or network" at that line

#### Scenario: A justified exemption
- **WHEN** the same line ends with `// lint-world: the scheduler's simulated clock reads no time`
- **THEN** the rule passes

### Requirement: Library: Allocation
A library SHALL avoid dynamic allocation as far as its job allows and let its user customize the allocator of what it does allocate, so that it can be used with Boost.Interprocess. A data structure that makes sense to share (a parsed JSON document, a state machine's definition) takes an `Allocator` template parameter, allocates through `std::allocator_traits` of it and holds every pointer as the allocator's `pointer` type (`offset_ptr`) or as an offset, never an absolute address, a vtable or a `std::function`, so that it can be built in a shared segment and used in place by another process that maps it elsewhere. Everything else (actors, rooms, sockets, handlers) MUST route every allocation through an allocator its user gives, a template parameter or a `std::pmr::memory_resource`. Each library's page says which of its types are shareable and a test that counts allocations proves none goes elsewhere; a library written from now on is born with this rule (pratt through an `Allocator`, trystero through `std::pmr`), and the older libraries are brought to it by the review of chapter 13.

#### Scenario: trystero's counting test
- **WHEN** a `std::pmr::memory_resource` is given in a room's configuration
- **THEN** a counting test proves, natively and on emscripten, that every allocation of a room reaches it

### Requirement: Library: Asynchronous code on xactor
A library that ports anything asynchronous SHALL run it as actors on xactor, whose scheduler drives every library the same way, deterministically, and in a browser from the page's event loop, which owns the system's events. How such a library meets Boost.Asio and Boost.Cobalt is still to be designed (roadmap). A call of Boost.Asio's `run`, `run_one` or `run_for` on an `io_context` is refused by the lint: a driver drains with `poll` and `poll_one`, and a program whose purpose is network I/O says why on the line after `lint-run:`.

#### Scenario: A driver drains
- **WHEN** a driver processes ready handlers
- **THEN** it calls `poll` or `poll_one`, never `run`

#### Scenario: A network program
- **WHEN** a program must block on its `io_context`
- **THEN** the line reads `io.run_for(timeout);  // lint-run: the memory relays post to it`

### Requirement: Library: Code style rules
A library's code SHALL follow these rules: a function starts with its guards (every condition it needs is checked first and what fails returns there, before any work begins); a comment says what the code does not make clear (why, a constraint, an example of a value), and a descriptive comment is one sentence of what the code that follows does, then an optional reason, in a few lines; a JSON literal in code (a raw string that opens a line with `R"({` or `R"([`) has its members four spaces deeper than that line, one per line, and closes at that line's indentation; formatting is Google-based, 4 spaces, 100 columns, pointers aligned left, includes sorted case-sensitively with blocks preserved (the root `.clang-format`), and lines starting with `|` in a comment are table rows that clang-format leaves whole.

#### Scenario: A JSON literal
- **WHEN** a test writes a raw JSON literal
- **THEN** it opens with `R"({`, has one member per line four spaces deeper than the line that opens it, and closes at the opening line's indentation

### Requirement: Library: Fidelity of a port and its differences
A port SHALL behave as the original at the version it ports (the same results, the same order of events, the same errors for the same inputs). Every difference is a decision recorded in the page's appendix "Differences": what the original does, what the port does and why; one forced by C++ (no garbage collector, value types where the original shares objects) is still recorded. Each difference has an example `example/diff_<topic>.cpp` with its `.expected` and its twin in the original's language, and the page shows both outputs side by side. No divergence goes unrecorded: one found later is fixed or recorded in the same change that finds it.

#### Scenario: xstate's diff_spawn_id
- **WHEN** XState keys a child spawned without an id as `"undefined"` and the port as an empty string
- **THEN** the page shows both outputs, each printed by its program, and the divergence is in "Differences"

### Requirement: Library: Evidence by oracle and twins
A port's evidence SHALL be the original itself, run by a shared oracle in `tools/oracle/`: the original's tests are ported as data-driven cases the C++ runner runs against the port; the pinned original runs the same cases and writes the expected results the port must produce; every documented example has a twin in the original's language (`<name>.mjs` for JavaScript beside the C++ program in `test/oracle/twins/`), which either agrees (prints exactly what the C++ example prints), diverges (the original's output recorded as the twin's own `.expected` and the difference in the appendix), or is listed as without a twin with the reason as a line `<path>: <reason>` of `without-twin.txt`. Nothing expected is written by hand: only `update-expected` writes an expected result, and it refuses to write a twin's output for a twin that agrees. A port proves its cases pass natively and on every target it declares, its oracle lane is green, and its page shows each twin's code and each difference's two outputs; the runner accounts for every program by name so that a twin no longer compared fails the lane. A JavaScript or TypeScript original runs with `node --conditions=development`.

#### Scenario: A twin that agrees
- **WHEN** a twin prints exactly what the C++ example prints
- **THEN** `update-expected` refuses to write its output as the twin's own

#### Scenario: An unaccounted program
- **WHEN** an example has no twin and no line in `without-twin.txt`
- **THEN** the oracle lane fails naming it

### Requirement: Library: The oracle's rules
A library's `test/oracle/Jamfile` SHALL declare its oracle with `webcpp.original <word> + [ : <directory> ]` (how a program of the original's language runs, once and first; and the target `node-modules`, which installs what the `package-lock.json` beside the Jamfile or in the directory given pins, with `npm ci`, through `tools/node/install.py`, once per lockfile under `.node-modules/`), `webcpp.twins <examples> : <twins> : <suffix> : <extra-word> *` (the target `twins`, declared once per library: `twins.py` runs the twin of every program `<examples>/<path>.cpp` at any depth and compares what it prints with the program's `.expected`, or with the twin's own `.expected` for a difference, which must then differ), `webcpp.cases <name> : <script> : <cases> : <expected>` (the target `cases-<name>`: the original runs `<script> <cases> <output>` into the build directory and `compare.py` finds the output equal to `<expected>`, file by file and byte by byte) and `webcpp.lane <name> : <b2-target> + : <target> *`. The first `webcpp.twins` or `webcpp.cases` also declares `update-expected`. Every oracle target is explicit: only a request by name or a lane runs Node, and `b2 test` never does; the Jamfile stops loading at a cases directory that does not exist or an expected directory whose removal would take the oracle's directory or the cases.

#### Scenario: xstate's oracle Jamfile
- **WHEN** `libs/xstate/test/oracle/Jamfile` is read
- **THEN** it holds `webcpp.original node --conditions=development ;`, `webcpp.twins ../../example : twins : .mjs : --test-reporter=spec --test-reporter-destination=stderr ;`, two `webcpp.cases` (`machines`, `actors`) and `webcpp.lane oracle : twins cases-machines cases-actors ;`

#### Scenario: Running the oracle
- **WHEN** `b2 -a libs/xstate/test/oracle//oracle` runs
- **THEN** every case and every twin runs on the original and is compared, and node and npm missing stops the build naming the one missing

### Requirement: Library: The library's CI file
A library's `.github/workflows/ci.yml` SHALL call the superproject's reusable workflow: `jobs.ci.uses: webcpporg/webcpp/.github/workflows/library.yml@main` with `library: <name>`, on every push to `main`, every pull request and `workflow_dispatch`, with `permissions: contents: read` and the concurrency group `${{ github.workflow }}-${{ github.ref }}` whose `cancel-in-progress` is `${{ github.ref != 'refs/heads/main' }}` (a run on main is the record of main and never cut short by the next; a pull request's newer push replaces its older run).

#### Scenario: lexical's ci.yml
- **WHEN** `libs/lexical/.github/workflows/ci.yml` is read
- **THEN** it has `name: CI`, the three triggers, and a job `ci` with `uses: webcpporg/webcpp/.github/workflows/library.yml@main` and `with: library: lexical`

### Requirement: Library: The page, the changelog and the tests are part of the library
A library SHALL have a page (chapter on documentation), an API reference from its Doc Comments, a testing chapter, a changelog (`doc/changelog.adoc`), `headers-alone` tests for every public header, unit tests, examples that are compiled and compared, and every declared target green. A library declares the targets it runs on (native, wasip2, wasip3, emscripten) and passes on every one it declares; what a target cannot build is excluded by declaration (`: native`) and inside a program by the same condition the library uses (`#ifndef __wasi__`), never by skipping silently. A change a user can see adds its changelog line in the same commit.

#### Scenario: A target a test cannot build
- **WHEN** a test needs Boost.Asio, which does not compile for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34
- **THEN** the program is declared for `native` only (`webcpp.example xactor_asio.cpp : : native ;`) and `drivers.hpp` declares `asio_driver` only outside WASI

### Requirement: Config: webcpp's configuration library
`libs/config` SHALL be webcpp's configuration, which every library's own configuration includes first, as Boost.Config is Boost's: `<webcpp/config.hpp>` (include guard `WEBCPP_CONFIG_HPP`) and `<webcpp/version.hpp>` (guard `WEBCPP_VERSION_HPP`), declaring no namespace, with macros prefixed `WEBCPP_`. `config.hpp` MUST define exactly one of `WEBCPP_TARGET_WASIP2` (from the compiler's `__wasip2__`), `WEBCPP_TARGET_WASIP3` (`__wasip3__`), `WEBCPP_TARGET_EMSCRIPTEN` (`__EMSCRIPTEN__`) and `WEBCPP_TARGET_NATIVE`, computed from the compiler's own macros alone before `<boost/config.hpp>` (a plain compiler gets it too and a WebAssembly target without a C++ library, which stops inside Boost.Config, is named first), with `#error "unsupported target: webcpp builds for wasm32-wasip2, wasm32-wasip3, Emscripten and natively"` for any other `__wasm__`; then include `<boost/config.hpp>` and `<webcpp/version.hpp>`; and define `WEBCPP_HAS_EXCEPTIONS` when neither `BOOST_NO_EXCEPTIONS` nor `WEBCPP_NO_EXCEPTIONS` (the developer's switch for every webcpp library at once, leaving Boost's own exceptions as the compiler has them) is defined. A build that defines a target macro or `WEBCPP_HAS_EXCEPTIONS` before the header stops with "WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone".

#### Scenario: Target macros on wasip2
- **WHEN** a program is compiled for wasm32-wasip2
- **THEN** `WEBCPP_TARGET_WASIP2` is defined and no other `WEBCPP_TARGET_*`

#### Scenario: Exceptions switch
- **WHEN** the developer defines `WEBCPP_NO_EXCEPTIONS` in a build with exceptions
- **THEN** `WEBCPP_HAS_EXCEPTIONS` is not defined and `BOOST_NO_EXCEPTIONS` is not defined by it

### Requirement: Config: webcpp's version
`<webcpp/version.hpp>` SHALL be the one place webcpp's version is written: `#define WEBCPP_VERSION 100` (major * 100000 + minor * 100 + patch, so 0.1.0 is `100`, as Boost's `BOOST_VERSION` is a macro so that `#if` compares it, exempt from `modernize-macro-to-enum` on the line above) and `#define WEBCPP_LIB_VERSION "0_1"`. The `#define WEBCPP_VERSION` line holds nothing after the number but trailing spaces, because the Jamroot and `tools/release/release.py` both read that line and both refuse anything else there, a trailing comment included. The version moves only by the release procedure, right after a tag.

#### Scenario: The version of 0.1.0
- **WHEN** `version.hpp` is read at the commit of this proposal
- **THEN** `WEBCPP_VERSION` is `100` and `WEBCPP_LIB_VERSION` is `"0_1"`

### Requirement: Lint: Invocation, scope and verdict
`tools/lint/lint.sh --clang-format <path> --clang-tidy <path> [--shard K/N]` SHALL lint the superproject and every library of `libs/` that is a repository of its own (a directory with `.git`, so a local `git init` counts) on what git tracks and what it would track (untracked, not ignored), so a new file is linted before it is added, and nothing under `third_party/libs/`, which keeps its own style and licence; a symbolic link is not read. clang-format and clang-tidy MUST come from the pinned wasi-sdk so that every machine and the CI lint with one version. It prints each rule's section as `== <rule> ==`, ends with `lint: clean` and exit 0 when every rule passes, or with `lint: failed: <rule>` for each rule that failed and exit 1, and exits 2 on a usage error (an unknown argument, a missing or non-executable tool, a malformed `--shard`, or a root that is not a git repository). The twenty-one rules are `clang-format`, `clang-tidy`, `io_context::run`, `fluent chains`, `returns *this`, `em dash`, `JSON literals`, `licence notice`, `banned word`, `no clock, disk or network`, `raw b2 rules`, `Doc Comments`, `Pyright`, `Python line length`, `Python blank lines`, `Jam comment width`, `path.exists native`, `include boundaries`, `bare throw`, `Doc Comment code spans` and `changelog form`, in the order of the script, with the rest of the rules running after a failure.

#### Scenario: A clean tree
- **WHEN** `tools/lint/lint.sh --clang-format <wasi-sdk>/bin/clang-format --clang-tidy <wasi-sdk>/bin/clang-tidy` runs on a tree with no finding
- **THEN** it ends with `lint: clean` and exits 0

#### Scenario: A usage error
- **WHEN** `--shard 3/2` is given
- **THEN** the lint prints "--shard 3/2: K is greater than N" and exits 2

#### Scenario: A new file is linted before it is added
- **WHEN** an untracked, non-ignored `.hpp` has no licence notice
- **THEN** the rule `licence notice` fails naming it

### Requirement: Lint: Sharding
`--shard K/N` SHALL analyse the K-th of N interleaved slices of the files with clang-tidy and run every other rule as before; the N shards together analyse every file once. The CI runs the lint as eight shards (`--shard 1/8` to `8/8`).

#### Scenario: Eight shards
- **WHEN** the eight shards of the lint run
- **THEN** every file is analysed by clang-tidy in exactly one shard, and every other rule runs in each

### Requirement: Lint: clang-format and the shared style
The rule `clang-format` SHALL fail when any C++ file (`.cpp`, `.hpp`) is not formatted as the root `.clang-format` says (`clang-format --dry-run -Werror`): `Language: Cpp`, `BasedOnStyle: Google`, `Standard: c++20`, `ColumnLimit: 100`, `IndentWidth: 4`, `AccessModifierOffset: -4`, `ContinuationIndentWidth: 4`, `UseTab: Never`, `AllowShortFunctionsOnASingleLine: Inline`, `AllowShortIfStatementsOnASingleLine: Never`, `AllowShortLoopsOnASingleLine: false`, `AllowShortCaseLabelsOnASingleLine: true`, `DerivePointerAlignment: false`, `PointerAlignment: Left`, `IncludeBlocks: Preserve`, `SortIncludes: CaseSensitive`, `SeparateDefinitionBlocks: Always`, `EmptyLineBeforeAccessModifier: LogicalBlock` and `CommentPragmas: '^ IWYU pragma:|^ *\|'`. `clang-format -i` fixes it. A library's own `.clang-format` MAY inherit the root's with `InheritParentConfig` and add only what its code needs.

#### Scenario: A 101-column line
- **WHEN** a header holds a code line of 101 columns that clang-format would wrap
- **THEN** the rule fails with "some C++ files are not formatted; run clang-format -i on them"

#### Scenario: A table in a comment
- **WHEN** a comment line starts with `|`
- **THEN** clang-format neither splits it at the column limit nor joins it with the next

### Requirement: Lint: clang-tidy and its compilation database
The rule `clang-tidy` SHALL analyse, with every warning an error, the compilation database `tools/lint/compile_commands.py` writes from b2's dry runs (`b2 -n -a`, since b2 5.5.3's `--command-database` writes nothing), one per target the libraries declare: every test and example natively, with every program an own lane builds on that target, or on none (a served component, which only its lane builds), and each source that no native program compiles with the command of the first target that compiles it (wasip2, wasip3, else emscripten). A WebAssembly command MUST name wasi-sdk's `clang++` with that target's `--target`, to which the lint adds no host target or SDK, once the dry run of that target has generated the bindings its commands include (so the lint needs wit-bindgen and the WIT); an emscripten command names Emscripten's `em++`, which clang-tidy cannot run, and is analysed with wasi-sdk's `clang++` in its place after the words `em++ <its options> --cflags` prints (target `wasm32-unknown-emscripten`, the sysroot in Emscripten's cache, the `-iwithsysroot` directories, the words of its exceptions), less the settings of em++ alone. Findings are reported in a library's public headers, in the headers of its tests and examples and in those of its JavaScript module, never in Boost's, the standard library's or a dependency's (`HeaderFilterRegex: '(^|/)(include/webcpp|libs/[^/]+/(test|example|js))/'`). A WebAssembly command whose compiler is neither wasi-sdk's `clang++` nor, on emscripten, Emscripten's `em++`, a declared target whose toolset is not configured, a file of two kinds of compiler, and a served program in no own lane MUST fail it by name.

#### Scenario: A WebAssembly command with another compiler
- **WHEN** a dry run's WebAssembly command names a compiler that is not wasi-sdk's `clang++`
- **THEN** the lint fails by name, because it would analyse the command as a native one

#### Scenario: Boost is not analysed
- **WHEN** Boost's header triggers a clang-tidy check
- **THEN** it is not reported, since the header filter covers only webcpp's headers and its tests, examples and JavaScript module

### Requirement: Lint: Aggregate translation units and the twin without exceptions
Besides the programs, the lint SHALL analyse each library's aggregate translation units, through which every public header is analysed, each compiled as a headers-alone unit of its target: a library whose public headers one target builds whole has one, of every header (`bin/aggregate/<name>.cpp`), natively, or, for a library whose headers build only for WASI, on wasip2 and on wasip3 each reading its version's branch; a library whose headers no single target builds has one per target its headers-alone units run on, of the headers that target builds (`<name>-native.cpp`, `<name>-emscripten.cpp`). Every command, a program's and an aggregate's, on every target, MUST have its twin: the same translation unit as b2 compiles it with `exception-handling=off` on the same target (natively with the handler `tools/throw_exception.cpp`), so that what only a user's build without exceptions compiles (`#ifdef BOOST_NO_EXCEPTIONS`) is analysed too and a `throw`, `try` or `catch` outside it fails in a template where a test or an example instantiates it; a program that declares `<exception-handling>on` has none and the lint names it ("built with exceptions whatever the build asks"), and an aggregate's twin leaves out each header whose headers-alone call declares it (then a translation unit of its own, `<aggregate>-exception-handling-off.cpp`). A public header without a headers-alone translation unit on any target fails the lint by name.

#### Scenario: A try block outside the macro
- **WHEN** a header holds a `try` outside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS` and a test instantiates it
- **THEN** the twin without exceptions fails to compile under clang-tidy and the rule fails

#### Scenario: A header with no headers-alone unit
- **WHEN** a public header is covered by no `webcpp.headers-alone` call
- **THEN** the rule fails naming the header

### Requirement: Lint: The checks of .clang-tidy
The root `.clang-tidy` SHALL enable `bugprone-*`, `cert-*`, `cppcoreguidelines-avoid-non-const-global-variables`, `cppcoreguidelines-init-variables`, `cppcoreguidelines-pro-type-cstyle-cast`, `cppcoreguidelines-pro-type-member-init`, `cppcoreguidelines-slicing`, `cppcoreguidelines-virtual-class-destructor`, `misc-*`, `modernize-*`, `performance-*` and `readability-*`, less `bugprone-easily-swappable-parameters`, `misc-include-cleaner`, `misc-non-private-member-variables-in-classes`, `modernize-use-trailing-return-type`, `readability-identifier-length`, `readability-magic-numbers`, `readability-uppercase-literal-suffix`, `performance-enum-size` (a library's error enum has to be `int`), `misc-header-include-cycle` (Boost's `.ipp` idiom) and `readability-redundant-member-init` (aggregate members a user can designate-initialize keep a `{}` default so that a designated initializer never triggers `-Wmissing-field-initializers`); `WarningsAsErrors: '*'`; `FormatStyle: file`; and the naming options: `lower_case` for namespaces, classes, structs, enums, enum constants, functions, variables, parameters, members, type aliases, typedefs and constants, a private-member suffix `_`, `UPPER_CASE` macro definitions and a `readability-function-cognitive-complexity` threshold of 25.

#### Scenario: A CamelCase class
- **WHEN** a header declares `class SchedulerImpl`
- **THEN** `readability-identifier-naming` reports it and the lint fails

### Requirement: Lint: Exemptions and a directory's own .clang-tidy
A library's tests or examples MAY have a `.clang-tidy` that inherits the root's (`InheritParentConfig: true`) and switches off what does not apply to them, each with its reason in a comment (xactor's `test/.clang-tidy` turns off `bugprone-unchecked-optional-access` and `bugprone-exception-escape.CheckMain`; `example/.clang-tidy` the second). A check that applies to a library header MUST NOT be switched off for a directory, since a header's templates are analysed only where a test or an example instantiates them: the header states its exemption on the line clang-tidy reports, `NOLINT(<check>)` (or `NOLINTNEXTLINE(<check>)` on the line above when that line has no room; a tag comment between it and its function cancels the suppression) or, when a page's listing must hide the exemption, `NOLINTBEGIN(<check>)` and `NOLINTEND(<check>)` around the exempt function or functions alone, with the reason in the comment above in every form.

#### Scenario: Version macro exemption
- **WHEN** `version.hpp` defines `WEBCPP_VERSION`
- **THEN** the line above it holds `// NOLINTNEXTLINE(modernize-macro-to-enum)` because the build and release tooling read the `#define` line itself

### Requirement: Lint: Rules by pattern
The lint SHALL refuse these by pattern: the rule `io_context::run` fails a call of Boost.Asio's `run`, `run_one` or `run_for` on an `io_context` however its type is qualified (`boost::asio::io_context`, `asio::io_context` through a namespace alias, or `io_context` after a using) or `io_context::run`, unless the line says why after `lint-run:`; the rule `fluent chains` fails three calls chained in one expression (`.a().b().c(`); the rule `returns *this` fails a function other than an assignment operator, a prefix `operator++` or `operator--` returning `*this`, unless its head follows a line `// lint-std: <reason>` naming the member of the standard it reproduces (`std::basic_regex::assign`), the reason holding none of `;`, `{`, `}`, `#` or `@` so that clang and MrDocs still attach the Doc Comment above to the function; the rule `em dash` fails U+2014 in any file, searched with the character spelled as its UTF-8 bytes so the script does not contain it; the rule `JSON literals` fails a raw literal that opens a line with `R"({` or `R"([` whose members are not four spaces deeper than that line or that does not close at its indentation.

#### Scenario: A blocking run
- **WHEN** a program declares `boost::asio::io_context io;` and calls `io.run();` with no `lint-run:` on the line
- **THEN** the rule `io_context::run` fails at that line

#### Scenario: A misindented JSON literal
- **WHEN** a raw JSON literal's members are indented two spaces from the line that opens it
- **THEN** the rule `JSON literals` fails with "the members are not four spaces deeper than the line that opens the literal"

### Requirement: Lint: The licence notice and the banned word
The rule `licence notice` SHALL fail a source file (C++, JavaScript, AsciiDoc, Python, shell, Jam, YAML, HTML) that does not open with the notice, after its `#!` line when it has one (`tools/lint/rules.py licence`). The rule `banned word` MUST fail on the word that the script's regular expression matches (the name of the private project webcpp was extracted from), in any case, in a file's content (`grep -nHIi`), in a file's name, and in any commit of any repository (its author, committer or message, read with `git log -i -E --grep`, `--author` and `--committer`), the brackets keeping the script from matching its own pattern.

#### Scenario: A file without the notice
- **WHEN** a `.py` file starts with a docstring and no `#` notice
- **THEN** the rule `licence notice` fails naming the file

#### Scenario: A commit message
- **WHEN** a commit message of any repository contains the banned word
- **THEN** the rule `banned word` fails with "a commit (its author, committer or message) contains the banned word"

### Requirement: Lint: The world rule
The rule `no clock, disk or network` SHALL fail a library header (`libs/<name>/include/**/*.hpp` and the fixture libraries' of `tools/test/fixtures`) that names a real clock (`system_clock`, `steady_clock`, `high_resolution_clock`, `utc_clock`, `tai_clock`, `gps_clock`, `file_clock`, `time(`, `clock(`, `gettimeofday`, `clock_gettime`, `<ctime>`, `boost/chrono`, `boost/date_time`, `boost/timer`, `steady_timer`, `deadline_timer`), a thread (`<thread>`, `std::thread`, `std::jthread`, `this_thread::`), a file (`<fstream>`, `<filesystem>`, `fopen`, `fstream`, `mmap`, `boost/filesystem`, `boost/interprocess`, `std::filesystem`), a socket (`<sys/socket.h>`, `boost/asio`, `boost/beast`, `socket(`, `getaddrinfo`), a process or the environment (`<unistd.h>`, `boost/process`, `fork`, `exec*`, `popen`, `std::system(`, `getenv(`) or entropy (`random_device`, `getentropy`, `getrandom`, `arc4random`, `RAND_bytes`, `<sys/random.h>`), matched wherever the name appears and not only in an include, unless the same line holds `lint-world:` with a reason.

#### Scenario: A header includes <thread>
- **WHEN** a library header contains `#include <thread>` without `lint-world:`
- **THEN** the rule fails with the line and the message that the header names a real clock, file, socket, process, thread, the environment or entropy

### Requirement: Lint: Raw b2 rules and Jam rules
The rule `raw b2 rules` SHALL fail, by file and line, a library's test or example Jamfile that declares a program with `run`, `run-fail`, `compile`, `compile-fail`, `exe`, `unit-test`, `link` or `link-fail` instead of the `webcpp.*` rules (tokens read without comments). The rule `Jam comment width` MUST fail a comment line of a Jam file (`.jam`, `Jamroot`, `Jamfile`) over 80 columns, the width Jam comments are wrapped at, while its code may run longer. The rule `path.exists native` MUST fail a `path.exists` of a Jam file given a path other than `[ path.native ... ]`, since b2's builtin asks the system about the path as written and b2's form of a Windows path, `/D:/a/...`, names no file there.

#### Scenario: A raw run
- **WHEN** `libs/<name>/test/Jamfile` contains `run foo.cpp ;`
- **THEN** the rule fails naming the file and line and the message to use `webcpp.run`

#### Scenario: A Windows path
- **WHEN** a Jam file calls `path.exists $(header)` with a b2-form path
- **THEN** the rule `path.exists native` fails

### Requirement: Lint: Doc Comment rules
The rule `Doc Comments` SHALL fail a Doc Comment command that webcpp does not allow, a bare `@` that starts no allowed command, and a colon after a reference (`@ref` or `\ref`); the allowed commands are `@brief`, `@param`, `@tparam`, `@return`, `@returns`, `@throws`, `@pre`, `@post`, `@note`, `@see`, `@code`/`@endcode`, `@details`, `@par`, `@copydoc` and `@ref`, and the same with a backslash, and the lint names each forbidden one (`@sa`, `@deprecated`, `@since`, `@todo`, `@retval`, and every other command). A literal `@` in prose is written `\@`. The rule `Doc Comment code spans` MUST fail a code span of a Doc Comment that holds a name between two pairs of underscores unescaped (`__wasip2__`), which MrDocs reads as strong emphasis even in code; `\__wasip2__` is shown as written. A possessive after `@ref` is allowed.

#### Scenario: Forbidden commands
- **WHEN** a Doc Comment uses `@sa` or `@since`
- **THEN** the rule fails naming the command, since MrDocs drops it without a word

#### Scenario: A colon after a reference
- **WHEN** a Doc Comment writes `@ref status_of: it`
- **THEN** the rule fails, and `the result of @ref status_of, which` passes

### Requirement: Lint: Python rules
The rule `Pyright` SHALL run Pyright (pinned in `tools/lint/package.json`, installed by `npm ci` through `tools/node/install.py` under `tools/lint/.node-modules/` linked at `tools/lint/node_modules`, again when the pin changes; Node is a prerequisite) over every Python file with the root `pyrightconfig.json` (`pythonVersion` 3.9, `typeCheckingMode` `standard`, `reportUnusedFunction`, `reportUnusedImport` and `reportUnusedVariable` as errors, `extraPaths` the tools' directories) with `--warnings`, so a warning fails as an error does. The rule `Python line length` MUST fail a line over 100 columns, and `Python blank lines` a top-level `def` or `class` without exactly two blank lines before it, its decorators and the comments just above it.

#### Scenario: An unused import
- **WHEN** a Python file imports a module it never uses
- **THEN** Pyright reports an error and the rule fails

#### Scenario: One blank line
- **WHEN** a top-level `def` has one blank line before it
- **THEN** the rule `Python blank lines` fails

### Requirement: Lint: Bare throw
The rule `bare throw` SHALL fail a `throw` expression in a library's public header (`libs/*/include/**`), outside comments and literals, at its line: inside `#ifndef WEBCPP_<NAME>_NO_EXCEPTIONS` too, in a template that nothing instantiates (which clang-tidy's analysis without exceptions does not see), and as a rethrow, `throw;`; `boost::throw_exception` is the one way to raise and `std::rethrow_exception` the one way to rethrow.

#### Scenario: A throw in an uninstantiated template
- **WHEN** a header's template contains `throw std::runtime_error("x");` that no test instantiates
- **THEN** the rule `bare throw` fails at that line

### Requirement: Lint: Changelog form
The rule `changelog form` SHALL check every library changelog (`libs/<name>/doc/changelog.adoc`) and every version's release notes (`doc/releases/<a.b.c>.adoc`) with `tools/release/changelog.py`: a section or region missing or out of order, a line that does not start with one of the verbs `Added`, `Changed`, `Deprecated`, `Fixed`, `Improved`, `Removed` or `Updated` (or `*Breaking change:* ` followed by one), a line of more than 30 words, a cross-reference (`<<...>>`, `xref:`, `link:#`), more than ten lines for a library and a version, and text outside a bullet fail it. A list is read as Asciidoctor reads it: a nested list, a block or a directive inside a region fails, and a comment line is dropped.

#### Scenario: A line without a verb
- **WHEN** a changelog bullet reads "* The parser is faster."
- **THEN** the rule fails, since the line starts with no verb of the list

#### Scenario: Eleven lines
- **WHEN** a library's `unreleased` region holds eleven bullets
- **THEN** the rule fails, since at most ten lines per library and version are allowed

### Requirement: Tests: Unit tests and helpers
Unit tests SHALL use Boost.Core's lightweight_test (`<boost/core/lightweight_test.hpp>`), declared with `webcpp.run`, where `main` runs the cases and returns `boost::report_errors()`. A case that cannot go on after a failed check returns (`if (!BOOST_TEST(x.has_value())) { return; }`); a helper, which cannot return from its case, calls `require(BOOST_TEST(...))`, which ends the program with the errors counted so far (`libs/xactor/test/require.hpp`), since a user may build a test without exceptions. Boost.Test is only for a test that needs it (fixtures, data-driven suites), declared with `webcpp.boost-test`, natively only. A test moved from the original's or another suite records the conversion case by case, and each converted case is seen failing on a planted defect where the old one had a pinning test (`libs/xactor/test/CONVERSION.md`).

#### Scenario: A failed precondition in a case
- **WHEN** `BOOST_TEST(x.has_value())` fails in a case
- **THEN** the case returns instead of reading the empty optional

#### Scenario: A helper
- **WHEN** a helper's check fails
- **THEN** `require(BOOST_TEST(...))` ends the program with the errors counted so far, with no exception

### Requirement: Tests: Every test on every declared target
Every test SHALL run on every target its Jamfile declares. What a target cannot build is excluded by declaration (`: native`) and inside a program by the same condition the library uses (`#ifndef __wasi__`), never by skipping silently. A test that reads files at run time is given their directory (`WEBCPP_TEST_*` define, forward slashes, quoted whole, `webcpp.data`) and a `<dependency>` on the files it reads (a `glob-tree-ex` of them), so that a changed file runs it again.

#### Scenario: A WASI-only exclusion
- **WHEN** a test uses Boost.Asio
- **THEN** it is declared `: native` and the shared header is guarded with `#ifndef __wasi__`

### Requirement: Tests: Lanes
A lane SHALL build and run one library's tests and examples for one toolset, from scratch, and record what it built and ran: `b2 -a --dump-tests --out-xml=<lane>.xml toolset=<toolset> libs/<library>/test libs/<library>/example`, from the superproject's root; a wasip2 or wasip3 lane adds `testing.launcher=wasmtime` (which applies to every toolset of one b2 request, so a lane is always one toolset); an emscripten lane adds nothing. `--dump-tests` is required, since it lists every test, those the lane skips included, and the report refuses a file without it; with `--out-xml` b2 exits 0 even when a test fails, so b2's status is never the verdict. A lane is named after its target (`native`, `emscripten`, `wasip2`, `wasip3`) or after the directory b2 builds its toolset in (`gcc-14`, `gcc-15`, `clang-linux-18`, `clang-darwin-21`, `msvc-14.3`), the CI naming a native lane after its directory and a wasm or emscripten lane after its target; an own lane on a target is named `<target>.<library>.<lane>` (`wasip2.wasi.http`), a name no toolset directory takes. The report checks every name against the toolset directory the file records or exits 2 naming the lane, the directory and the names it may take; a lane that built nothing records no directory and is checked against the toolset its command line names.

#### Scenario: The lane command for wasip2
- **WHEN** a wasip2 lane of wasi runs
- **THEN** its command is `b2 -a --dump-tests --build-dir=bin/lane-wasip2 --out-xml=wasip2.xml toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test libs/wasi/example`

#### Scenario: A lane named for the wrong toolset
- **WHEN** a lane is named `wasip2` but was built with gcc
- **THEN** the report exits 2 naming the lane, the directory and the names it may take

### Requirement: Tests: Lanes in parallel
Independent lanes SHALL run at the same time, each with its own build directory (`--build-dir=bin/lane-<name>`) and be read once all have finished: two concurrent b2 runs never share a build directory, and at most one of them builds the documentation, which writes `doc/html/` and `libs/<name>/doc/html/` inside the tree.

#### Scenario: Two lanes at once
- **WHEN** gcc-15 and wasip2 lanes run together
- **THEN** their build directories are `bin/lane-gcc-15` and `bin/lane-wasip2`

### Requirement: Tests: Own lanes
A library MAY declare lanes of its own with `webcpp.lane <name> : <b2-target> + : <target> *` in a test or example Jamfile under `libs/<library>/test` or `libs/<library>/example` (a Jamfile elsewhere stops with "which is not a test or an example Jamfile of a library"): the alias `<name>` is explicit over the b2 targets, each a main target of the same Jamfile which the lane makes explicit too so that the ordinary lanes never build them, and it compiles nothing and leaves the Jamroot's check of Boost to the targets it names. `b2 declared-lanes -d0` prints, sorted, `<library> <lane> <directory> <kind>` for a lane that names no target (run as `b2 -a <directory>//<lane>`, whose exit status is its verdict, no XML, no column of the report) and `<library> <lane> <directory> <target> <kind>` once per target for a lane that names targets (run on each as that target's lane runs, from scratch, with `--dump-tests` and `--out-xml`, its tests reaching the report in a column of its own); the kind is `original` for a lane that runs the original's language (an oracle's twins, cases, `node-modules` or `update-expected`, or a driven test) and `programs` otherwise (wasi's served components). A target a lane names MUST be one of its Jamfile's `webcpp.targets` (`native` without any). Built for a target it does not name, such a lane stops, naming itself, its targets and the toolset to give (`webcpp.lane http in libs/wasi/test/Jamfile runs on wasip2 wasip3, and this build is for native: give toolset=clang-wasip2 or toolset=clang-wasip3, with testing.launcher=wasmtime`).

#### Scenario: wasi's served lane
- **WHEN** wasi's test Jamfile declares `webcpp.lane http : http_methods http_long_body http_no_content_type http_two_units : wasip2 wasip3 ;`
- **THEN** `b2 declared-lanes -d0` prints one `wasi http libs/wasi/test wasip2 programs` line and one for wasip3, and the lane runs as `b2 -a --dump-tests --out-xml=wasip2.wasi.http.xml toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/example//http libs/wasi/test//http`

#### Scenario: A lane built natively
- **WHEN** `b2 libs/wasi/test//http` runs with the native toolset
- **THEN** the build stops with the message naming the lane, its targets and the toolsets to give, rather than build nothing and read green

### Requirement: Tests: A lane-only program is always in a lane
A lane-only program, served (`webcpp.serve`, `webcpp.serve-script`) or driven (`webcpp.drive`), MUST run in an own lane on every target it is built for: `b2 declared-lanes` stops naming it when no lane of its Jamfile names it (`webcpp.drive driven in libs/browser_demo/test/driver/Jamfile is in no own lane; name it in a webcpp.lane that names its targets`), or when its lanes leave out a target it is built for, so that the CI's plan fails before any job runs rather than leave it untested. A lane on emscripten is declared like any other and runs as the emscripten lane runs (`toolset=emscripten`, no launcher), never natively in its place.

#### Scenario: A driven test in no lane
- **WHEN** a `webcpp.drive` program is named by no `webcpp.lane`
- **THEN** `b2 declared-lanes` stops with the message that it is in no own lane

### Requirement: Tests: The report
`python3 tools/report/report.py --lane <lane>=<lane>.xml [--lane ...] --out <dir>` SHALL write the test matrix: `index.html` (the libraries by lanes), `<library>.html` (a library's tests and examples by lanes) and `output/<lane>/...` (the log of each failure, linked from its cell). A cell is green when every build of it passed, red with the kind of its most significant failure (compile, compiled, link, linked, build, run, ran, not run) and grey (n/a) when the lane did not build it. The exit status is the verdict: 0 when every lane built something (a test or an example) of every library it lists and everything passed; 1 when a test or an example failed, an action outside every test failed, a lane built nothing, or a lane that built something built none of the tests and examples of a library it lists, each named (`wasip2: nativeonly: the lane built none of its tests and examples`); 2, with nothing written, when a file cannot be read, a lane spans more than one toolset, a lane named after a target was built for another, a lane named after no target is not named after its toolset directory, a test lies outside `libs/<name>/`, or a library is named `index`. The directory is served as the site's `report/`: each page's `webcpp` links the site's index (`../`), a library's page links its documentation (`../libs/<library>/`) and the footer links `github.com/webcpporg`.

#### Scenario: A failed test
- **WHEN** a lane's XML records a failed run of a test
- **THEN** the cell is red with kind "run", its output is linked from the cell, and the exit status is 1

#### Scenario: A lane that built nothing
- **WHEN** a planned lane's XML has no test
- **THEN** the report fails it as empty

### Requirement: Tests: The tests of the build and of the tools
Every behaviour of the build and of `tools/` SHALL be pinned by a test `tools/**/*_test.py`, run as `python3 <path>`: `tools/test/jamroot_test.py`, `webcpp_jam_test.py`, `oracle_jam_test.py`, `generated_jam_test.py`, `component_jam_test.py`, `emscripten_test.py`, `external_test.py`, `doc_test.py`, `harness_test.py`, `third_party_test.py`, `boost_libraries_test.py`, `tools/generated/generated_test.py`, `tools/lint/lint_test.py`, `tools/report/report_test.py`, `tools/doc/doc_check_test.py`, `doc_comments_test.py`, `extensions_test.py`, `counts_test.py`, `tools/oracle/twins_test.py`, `compare_test.py`, `tools/example/run_example_test.py`, `tools/component/serve_test.py`, `tools/drive/drive_test.py`, `tools/node/install_test.py`, `tools/ci/matrix_test.py`, `assemble_test.py`, `actions_test.py`, `tools/site/catalogue_test.py`, `redirect_test.py`, `site_test.py`, `tools/release/changelog_test.py` and `release_test.py`. They build in scratch copies under `$TMPDIR` whose path holds a space (so they run beside a build of the tree), through `tools/test/harness.py`, which reads `.local/user-config.jam` else the file `$WEBCPP_USER_CONFIG` names and stops with an error when neither exists, links the checkout's `.local/boost-source` into each scratch copy and gives each b2 the environment from one place; a suite run by `harness.run_cases` takes `--shard K/N`, and a run without `--shard` that chooses no case fails since it would pass having run nothing. A change to b2's `--out-xml` format shows as a red `report_test.py`, whose samples (`tools/report/samples/`) are recorded by `python3 tools/report/record_samples.py`, never edited by hand.

#### Scenario: A change of a tool
- **WHEN** a rule of `tools/webcpp.jam` changes
- **THEN** a test in `tools/test/webcpp_jam_test.py` fails without the change

#### Scenario: A sharded suite with no case
- **WHEN** `harness.run_cases` runs without `--shard` and chooses no case
- **THEN** it fails

### Requirement: Docs: The API reference from Doc Comments
The reference SHALL be generated by MrDocs 2026.9.29, always to AsciiDoc as a single page, from the library's Doc Comments: `webcpp.reference <name> ;` in `doc/Jamfile` declares it and `tools/doc/reference.py` runs it. The input is a compilation database of one aggregate translation unit including every public header (the one the lint analyses), read with the include directories and defines of the library's target and the requirements `webcpp.reference <name> : <requirements> * ;` gives, natively; a library whose headers build only for WASI gives the explicit target `<bindings>-headers` of `webcpp.wit-bindings` and the macro of one version. The shared settings are `tools/doc/mrdocs.yml.in`: `generator: adoc`, `multipage: false`, `embedded: true`, `warn-as-error: true`, `auto-function-metadata: false`, `auto-relates: false` (at their defaults they document a parameter with its type's brief and count a class with related functions as documented, which hid 48 findings on the first library measured), every `warn-*` on, the symbols of `webcpp::<name>::**`, `detail` namespaces as implementation-defined, the macros `WEBCPP_<NAME>_*` except the include guards (every `WEBCPP_*` for config), `base-url: https://github.com/webcpporg/<name>/blob/main/` and `inherit-base-members: never` (a derived class lists its own members alone). The page includes the result with `include::{reference}[leveloffset=+1]`, and `tools/doc/doc_comments.py` checks with clang++ what MrDocs does not.

#### Scenario: A header the reference cannot parse
- **WHEN** a public header does not parse in the reference's translation unit
- **THEN** the reference fails, naming the header

#### Scenario: Both versions of a branching header
- **WHEN** `webcpp.reference wasi : <wasip2 requirements> : <wasip3 requirements> ;` is declared
- **THEN** `reference.py` runs again for wasip3 as a sibling the page builds too, whose output no page shows, and an undocumented macro or `detail` symbol of the wasip3 branch fails the doc build

### Requirement: Docs: A library's mrdocs.yml
A library's `doc/mrdocs.yml` SHALL hold presentation keys only: `sort-members`, `sort-members-by`, `sort-namespace-members-by`, `sort-members-ctors-1st`, `sort-members-dtors-1st`, `sort-members-assignment-1st`, `sort-members-conversion-last`, `sort-members-relational-last`, `overloads`, `sfinae`, `inherit-base-members`, `inherit-hidden-friends`, `legible-names` and `show-enum-constants`. Any other key (what is documented, the input, the output, a warning) MUST be refused and fail the reference, so that every library is documented as strictly as every other (xactor's sets `show-enum-constants: true`).

#### Scenario: A library turns a warning off
- **WHEN** `doc/mrdocs.yml` sets `warn-if-undocumented: false`
- **THEN** the reference fails, refusing the key

### Requirement: Docs: The reference for emscripten
A library with a header that builds only on emscripten SHALL give `/webcpp//emscripten-reference` in its `webcpp.reference` requirements, whose usage requirements give the reference the emsdk (`<webcpp-emsdk>`): the translation unit is then read for Emscripten's target, as the emsdk's `em++` compiles it with `-fwasm-exceptions`, with the words `em++ -fwasm-exceptions --cflags` prints (target `wasm32-unknown-emscripten`, sysroot in Emscripten's cache) less its `-mllvm` options: `doc_comments.py` reads it with the emsdk's `clang++`, and MrDocs reads a command whose compiler is `emscripten-clang++`, a script `reference.py` writes, so that the include directories are Emscripten's. Every header is read for that one target in one translation unit, since MrDocs merges the symbols of two translation units by an identifier holding the types of a function's parameters, and the host's standard library and Emscripten's give one function two (trystero's reference read as two had 555 sections for 461). The emsdk is `-sWEBCPP_EMSDK=<dir>` on b2's command line, else `.local/emsdk`, never the environment's `EMSDK`; its `upstream/emscripten/emscripten-version.txt` MUST name 6.0.11; the build stops, naming both places, when its `em++` or `clang++` is not there and naming both versions on another.

#### Scenario: trystero's reference
- **WHEN** `webcpp.reference trystero : <library>/webcpp/trystero//native <library>/webcpp//emscripten-reference <define>BOOST_ASIO_DISABLE_THREADS ;` is declared
- **THEN** the reference is the same on Linux and on macOS, byte for byte

#### Scenario: Another Emscripten
- **WHEN** the emsdk named holds Emscripten 6.0.10
- **THEN** the build stops naming 6.0.10 and the pinned 6.0.11

### Requirement: Docs: What every public symbol needs
A public symbol (at namespace scope of `webcpp::<name>` or a public member of a public class, outside every `detail` namespace) SHALL be fully documented: functions, classes, structs, enums and enumerators, aliases, concepts, variables, data members and macros, in Javadoc/Doxygen-style `/** ... */` comments before the symbol. It needs a brief, the first paragraph, which is a single sentence (MrDocs makes the whole first paragraph the brief, and `doc_comments.py` fails a brief of more than one sentence; `e.g.` and a code span do not count); `@param` for every parameter, `@tparam` for every named template parameter (of a function, class, partial specialization, alias, variable or concept, hidden friends and member templates included; MrDocs reports a missing `@param` or `@return`, `doc_comments.py` a missing `@tparam`), and `@return` for every function that returns a value (a deleted function takes none); `@throws`, `@pre`, `@post`, `@note`, `@see` and `@code` where they apply; each enumerator and each data member has its own Doc Comment and one data member per declaration (clang attaches one comment to every declarator up to the next `;`, `{`, `}`, `#` or `@`); a `detail` symbol needs only its brief. A header's opening `/** ... */` is for its reader and does not reach the reference.

#### Scenario: Two enumerators in one comment
- **WHEN** `/** A. */ a, b,` documents two enumerators
- **THEN** `doc_comments.py` reports `b` as undocumented

#### Scenario: A two-sentence brief
- **WHEN** a Doc Comment opens with two sentences in its first paragraph
- **THEN** `doc_comments.py` fails the brief

### Requirement: Docs: Doc Comment links
`@see "<title>"` SHALL send the reader to a section of the library's page by its title, exactly as the heading writes it after its `=` marks, in quotes and on one line (`@see "Writing an actor logic", in the guide.`); the doc build fails on a title that names no section or that wraps onto a second line, and a section renamed changes its `@see` lines in the same commit. A `(doc: #<anchor>)` in a `//` comment of the library (code, test or example) sends its reader to an anchor of the page (`(doc: #xactor-invariant-13)`, a list `(doc: #a, #b and #c)` or a range `(doc: #a to #b)`), and the doc build fails on an anchor the page does not define; in a Doc Comment `@see` is used. A link by hand to an overload's anchor is never written, since MrDocs gives it a hash suffix that changes.

#### Scenario: A renamed section
- **WHEN** a section's heading changes and a Doc Comment's `@see` still names the old title
- **THEN** the doc build fails

### Requirement: Docs: After MrDocs
`postprocess.mjs` SHALL decode the character references MrDocs writes for the characters AsciiDoc would read as markup and the U+2010 it writes for an ASCII hyphen; `reference.py` MUST drop the near-empty sections of the global namespace and of `webcpp`, so the reference starts at `webcpp::<name>` with the library's macros, when it has any, in a section "Macros" of their own; a link MrDocs writes inside another link's text (`link:#a[handler<void(link:#b[bytes])>]`) is written as its text with the outer link kept; an anchor MrDocs writes with brackets becomes one an id holds.

#### Scenario: A hyphen
- **WHEN** MrDocs writes U+2010 in a signature
- **THEN** the page shows an ASCII hyphen

### Requirement: Docs: The page of a library
Each library SHALL have one AsciiDoc page `doc/<name>.adoc` with its sections in files of their own beside it, built by `doc/Jamfile` (`import webcpp ;`, `webcpp.doc <name> : <name>.adoc ;`, `webcpp.reference <name> ;`): either rule declared in another directory stops the build naming the rule and the directory, and so does a page whose Jamfile declares no reference or a reference whose Jamfile declares no page. `b2 libs/<name>/doc` converts the page with Asciidoctor.js into `libs/<name>/doc/html/index.html`. The page sets its own title and attributes (`= <name>: <what it is> for {cpp}`, `:toc: left`, `:toclevels: 1`, `:idprefix:`, `:sectanchors:`, `:example-caption!:`, `:attribute-missing: warn`), and the doc build provides `{examples}`, `{reference}`, `{twins}`, `{webcpp-root}`, `{library-root}`, the counts and the links to other pages, the highlighter and the shared style (`tools/doc/docinfo.html`: the system's fonts, no web fonts, no date, no footer, a table too wide scrolling in its own box and an identifier breaking only between its parts), none of which a page sets. The page holds the guide (taught the way the original's documentation teaches, concept by concept, with C++ only in listings, each section with an anchor starting with the library's name), the numbered guarantees (`. [[xactor-invariant-1]]...`, a number and anchor kept forever because code, tests and comments cite them), a testing chapter (how a program tests what it builds, how the library itself is tested, on which targets, with which commands, and the report at `https://webcpporg.github.io/report/`), the reference under `[#reference]`, the appendix "Differences" for a port, and the appendix Changelog (`include::changelog.adoc[]`) last.

#### Scenario: xactor's page header
- **WHEN** `libs/xactor/doc/xactor.adoc` is read
- **THEN** it starts with the licence notice and `= xactor: ... for {cpp}` and the attributes above

#### Scenario: A reference without a page
- **WHEN** a doc Jamfile declares `webcpp.reference` and no `webcpp.doc`
- **THEN** the build stops naming the rule and the directory

### Requirement: Docs: Listings are compiled examples
Every listing of a page SHALL be a compiled example and every shown output SHALL come from a `.expected` file: a listing includes a region of an example marked in the program by `// tag::<name>[]` and `// end::<name>[]` (`include::{examples}/xactor_quick_start.cpp[tag=messages,indent=0]`) and its output is `include::{examples}/xactor_quick_start.expected[]`, compared by `b2 example` on every build. A listing of shell commands is `[source,bash]`; the only languages a page may use are C++ (the default), JavaScript (a twin's code), JSON and bash; anything else (a WIT world, a Jamfile's lines, a served test's requests) is a `[listing]` block, which stays plain; a `----` block with no style is C++ and must be an include of an example or of a tagged region of a file of the library's `test/` or `example/`. A page shows the build's own configuration as it is, never a copy: `include::{webcpp-root}/tools/ci/wasi-sdk.jam[tag=wasip2]` for the superproject's tracked files and `include::{library-root}/build.jam[tag=dependencies]` for the library's own.

#### Scenario: A C++ block with typed code
- **WHEN** a page holds a `----` block of C++ that is not an include
- **THEN** doc-check fails it

### Requirement: Docs: What doc-check enforces
`tools/doc/doc-check.py` SHALL run before the page is converted and again on the rendered HTML, and any finding and any warning of Asciidoctor's fails the build: every `.adoc` under `doc/` is reached from the page's `include::` graph; every example's code or output is shown, every block's language is allowed, C++ is shown only as an include of an example or of a tagged region of `test/` or `example/`, and the page includes the reference; every program an example Jamfile links and never runs (`webcpp.link`) is shown by an include of its code and an `.expected` beside one is a fault ("<program> is linked, never run: it has no output to compare"); every include names a file that exists, inside `doc/`, `{examples}` or `{twins}`, and an output's include names a program that exists; an include of `{webcpp-root}/<path>` or `{library-root}/<path>` names a region (`tag=` or `tags=`) of a file that git of that root tracks, which holds the region (an untracked file, a link git does not track, a missing file, a whole file and lines chosen by number all fail); every `(doc: #anchor)` and `index.html#anchor` names an anchor the page defines and every `@see "<title>"` a section; every `(doc: <library>#<anchor>)` and every link of the rendered page into another library's page names an anchor that page defines; every C++ block of the library's README is opened by a comment `<!-- include::<file>[<attributes>] -->` and equals that region; no line inside a table starts with `//` and no `++` appears in prose (write `{cpp}`); the rendered page has no cross-reference left as text, no stray `++` or backtick, no undecoded escape of MrDocs's, no U+2010, no em dash (which Asciidoctor writes for `--` even inside inline code: `xn\--bcher`), none of Asciidoctor's other replacements inside inline code (an ellipsis, an arrow, (C), (R), (TM) or a curly apostrophe), no link inside inline code but for MrDocs's, no `<wbr>` (`postprocess.mjs` marks a break as an empty `<span class="wbr">` whose zero-width space the style generates) and no link to the dropped `#index` or `#webcpp` sections.

#### Scenario: A linked program with an expected file
- **WHEN** a program declared with `webcpp.link` has a `.expected` beside it
- **THEN** doc-check fails with "<program> is linked, never run: it has no output to compare"

#### Scenario: An untracked include
- **WHEN** a page includes a region of a file git does not track
- **THEN** doc-check fails

### Requirement: Docs: Counts
A page SHALL never type how many examples, tests, headers or twins its library has: `webcpp.doc` gives it attributes that `tools/doc/counts.py` computes at every build, from the programs b2 records as it loads the library's test and example Jamfiles (`{n-examples}`, `{n-examples-<target>}`, `{n-tests}`, `{n-tests-<target>}` where each header compiled alone is one test and so is each served component, linked program and driven test, `{n-served}`, `{n-linked}`, `{n-driven}`, their `-<target>` forms, `{n-boost-test-suites}`, `{n-headers}`), from `twins.py --list` for a library whose oracle declares twins (`{n-twins-agreeing}`, `{n-twins-divergent}`, `{n-examples-without-twin}`, `{n-examples-with-original}`) and from the library's own `doc/counts.py` run with the library's directory as its one argument, each line `<name>=<number>` it prints in plain digits. A count that finds nothing fails the build rather than put a zero on the page, and so does a name of a library's own that is also a generic one; a count reads as English writes a number, with a thousands separator, `1,234`.

#### Scenario: A count of zero
- **WHEN** a count finds nothing
- **THEN** the build fails instead of printing zero

### Requirement: Docs: Links between pages
A page SHALL link another library's page as `link:{webcpp-libs}/<library>/{webcpp-page}#<anchor>[...]`, with two attributes `webcpp.doc` sets by the layout (where b2 builds the pages by default, or where the site serves them with `-sWEBCPP_INDEX=site`); such a link written any other way after `{webcpp-libs}/` is a fault. A `//` comment sends its reader to another library's page with `(doc: <library>#<anchor>)`. The build makes every linked page first, and the check of the rendered page fails on an anchor that page does not define and on an overload's anchor, which ends in `-0<digit>` and which MrDocs may renumber.

#### Scenario: An overload anchor
- **WHEN** a page links `#xactor-send-02`
- **THEN** the check fails

### Requirement: Docs: The index page
`doc/index.adoc`, built by `doc/Jamfile` (`webcpp.index index.adoc ;`), SHALL introduce webcpp and include, at `{libraries}`, the table `tools/doc/libraries.py` writes from every library's `meta/libraries.json`: its name linked to its page, its description, and what it ports linked to the original, or "original" for a library of webcpp's own. The pages are those the build declares (`tools/doc/doc.jam` loads every library's `doc/Jamfile` and gives `libraries.py` the page each declares, so that no Jamfile's text is read twice); a library that declares none or whose page is not there fails the table, named; a library left out (`webcpp.left-out`) is named with the reason and no link. The table links a page as `{library-pages}<name>/{library-page}`, by default `../../libs/<name>/doc/html/index.html` from `doc/html/` and, with `-sWEBCPP_INDEX=site`, `libs/<name>/` beside the index at the site's root; any other value stops the build. The index also shows each version's release notes, `doc/releases/`, newest first, and names a library in them only once it is a submodule.

#### Scenario: The index on the site
- **WHEN** `b2 doc -sWEBCPP_INDEX=site` runs
- **THEN** the index links each library's page as `libs/<name>/`

#### Scenario: An invalid value
- **WHEN** `-sWEBCPP_INDEX=elsewhere` is given
- **THEN** the build stops

### Requirement: Docs: Node packages of the documentation
The Node packages of the doc build (Asciidoctor.js and highlight.js, pinned in `tools/doc/package.json` and `package-lock.json`), Pyright for the lint and an oracle's original SHALL be installed by `tools/node/install.py` with `npm ci`, once per lockfile under `.node-modules/` linked at `node_modules`, under a lock so that doc builds at once share one install; the install of an older lockfile is removed once a build no longer may read it, a directory the system refuses ends the install with a message, and `npm ci` runs with `OPENUI_TELEMETRY_DISABLED=1` and `DO_NOT_TRACK=1` so that no package's install script reports an install.

#### Scenario: Two doc builds at once
- **WHEN** two doc builds start together
- **THEN** they share one install of the Node packages, and neither installs over the other

### Requirement: Docs: One doc build at a time
A doc build writes `doc/html/` and `libs/<name>/doc/html/` inside the tree, so two concurrent doc builds MUST NOT run in one tree; the CI builds the documentation in one job.

#### Scenario: Concurrent doc builds
- **WHEN** two `b2 doc` runs share a checkout
- **THEN** they would write the same files, which is why the rulebook forbids it

### Requirement: CI: The two workflows
The CI SHALL be GitHub Actions in two workflows of the superproject, and it MUST run exactly the commands the rulebook documents. `.github/workflows/library.yml` is reusable (`on: workflow_call`, input `library`, default empty, and an output `docs`); a library's `.github/workflows/ci.yml` calls it as `webcpporg/webcpp/.github/workflows/library.yml@main` with its `library`, and the superproject's `.github/workflows/ci.yml` calls it as `./.github/workflows/library.yml` without a library, which builds every library of `libs/` at the commit its submodule points to and the index page with their pages. Every job checks out `webcpporg/webcpp` with its submodules (the caller's commit for the superproject, `main` for a library, whose own commit then replaces `libs/<library>`, the library name checked against `^[a-z][a-z0-9_]*$`), no checkout keeps its credentials, the workflows have `permissions: contents: read`, every third-party action is pinned by its full commit SHA with its tag in a comment, a run on `main` is never cancelled by the next and a pull request's newer push cancels its older run. No b2 call passes `-j`: b2 runs as many jobs as the runner has processors.

#### Scenario: A library's push
- **WHEN** a commit is pushed to `libs/lexical`'s `main`
- **THEN** its `ci.yml` calls `library.yml@main` with `library: lexical`, which checks the superproject out with submodules and puts the pushed commit into `libs/lexical`

#### Scenario: A pinned action
- **WHEN** a workflow uses `actions/checkout`
- **THEN** it is `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1`, a full SHA with the tag in a comment

### Requirement: CI: The plan job
The job `plan` SHALL run `python3 tools/ci/matrix.py plan [--library <name>]`, which runs `b2 declared-targets -d0` and prints the JSON matrix of lanes (`{"include": [...]}`): one lane per compiler for each target a library declares, building the libraries that declare it. The CI MUST never list a library's targets by hand; every target has a lane, and one that comes to have none fails the plan by name. Then `matrix.py own-lanes [--library <name>]` runs `b2 declared-lanes -d0` and prints the matrix of own lanes (one entry per library, lane and target, with `library`, `lane`, `directories`, and for a lane that names targets `platform` and `id`, and `kind`, `name`, `os`, `wasm`, `emsdk`, `cc`, `cxx`, `node`, `external`, `chrome`); `matrix.py declares emscripten` prints `true` when any library declares emscripten (the output `has-emscripten`, on which docs and lint install emsdk); `matrix.py external --library <name>` prints `true` when that library, or a library of `libs/` it uses at any depth, needs libraries webcpp does not build (output `external`; without `--library`, `lint-external`). The plan also outputs `has-container` and `has-own-lanes` (an own-lanes matrix with no entry fails its job, so the job runs only when there is one).

#### Scenario: A target with no lane
- **WHEN** a library declares a target the CI has no lane for
- **THEN** `matrix.py plan` fails naming the target, never leaving a lane out

#### Scenario: A library with no own lane
- **WHEN** xactor declares no `webcpp.lane`
- **THEN** its matrix of own lanes is `{"include":[]}` and the own-lanes job does not run

### Requirement: CI: The lanes
The jobs `lanes` SHALL run `matrix.py lane <entry>` once per lane of the plan, which registers the lane's toolset in `.local/user-config.jam` with its version, prints the lane command and runs it, and uploads `<lane>.xml` as the artifact `lane-<id>`. The lanes are: `gcc-14` (ubuntu-24.04, `gcc-14`, in a container with no network), `gcc-15` (ubuntu-26.04), `clang-linux-18` (ubuntu-24.04, `clang-18` on libstdc++, in a container with no network), `clang-linux-22` (ubuntu-26.04, `clang-22` on libstdc++), `clang-darwin-<version>` (macos-15, Apple Clang, its version read from `clang++ -dumpversion`), `msvc-14.3` (windows-2022, Visual Studio 2022), `msvc-14.5` (windows-2025, Visual Studio 2026), `emscripten` (ubuntu-24.04, Emscripten 6.0.11 from emsdk with its system libraries built in the cache the action restores, and Node 26.7.0 through the wrapper of the emsdk action) and `wasip2` and `wasip3` (ubuntu-24.04, `clang-wasip2` and `clang-wasip3` with wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0 and the WASI WIT, for every library). The Clang lanes on libstdc++ stay because a regression of xactor's guarantee 28 is caught only there; the MSVC lanes add `address-model=64 embed-manifest-via=linker --abbreviate-paths` (b2 abbreviates each word of a toolset directory, and `msvc-14.3` and `msvc-14.5` are their own abbreviations, which `tools/ci/matrix_test.py` checks with b2's own rule).

#### Scenario: The lane list
- **WHEN** `tools/ci/matrix.py`'s `LANES` is read
- **THEN** it holds gcc-14, gcc-15, clang-linux-18, clang-linux-22, clang-darwin, msvc-14.3, msvc-14.5, emscripten, wasip2 and wasip3

#### Scenario: A lane run locally
- **WHEN** a contributor runs `python3 tools/ci/matrix.py lane '<entry>' -- --build-dir=bin/lane-gcc-15`
- **THEN** the lane runs exactly as the CI runs it, with the arguments after `--` handed to b2

### Requirement: CI: The container lanes
The lanes `gcc-14` and `clang-linux-18` (their entries' `container`) SHALL run the lane's b2 in a Linux container with no network but the loopback (`docker run --rm --network none`, as the runner's user, with the superproject and b2's prefix mounted at their own paths, the external libraries' variables passed on), so that a test that reaches beyond the machine fails there. The image is `tools/ci/container/Dockerfile`'s: Ubuntu 24.04 pinned by the digest of its image index, with `g++-14`, `clang-18`, OpenSSL's development files and Python, every package from Ubuntu's snapshot of the archive at the date its `ARG SNAPSHOT` names (`Snapshot: yes` on the sources, `apt-get --snapshot`), `ca-certificates` included (the snapshot service is HTTPS only and the base image has no CA certificates, so apt reaches it once without verifying the server's certificate and verifies it from then on; apt checks the signed `InRelease` and every index and package whatever the transport), built for x86-64 alone (Ubuntu keeps no snapshot of `ports.ubuntu.com`), tagged `webcpp-lane:<the first 16 hex digits of the Dockerfile's SHA-256>`, and its `apt-get` retries a snapshot outage while still failing on a stale index. The job `container` MUST run the `container` action once per run, which builds the image when the cache does not hold it and saves it under that digest; every lane's job waits for it (`needs: [plan, container]`) and runs when it succeeded or was skipped; each container lane's job loads the image, and `matrix.py lane` builds it only when no image of that tag is there.

#### Scenario: No network inside the lane
- **WHEN** a test in the gcc-14 lane opens a connection beyond the loopback
- **THEN** it fails, because the container has `--network none`

#### Scenario: The image tag
- **WHEN** the Dockerfile's SHA-256 begins `0123456789abcdef`
- **THEN** the image is tagged `webcpp-lane:0123456789abcdef`

### Requirement: CI: External libraries
`EXTERNAL_DEPENDENCIES` in `tools/ci/matrix.py` SHALL name the libraries webcpp does not build (trystero's `secp256k1`, `libdatachannel` and `openssl`, each the CI's action of that name) until webcpp builds them from `third_party/`. A lane that builds such a library, or one that uses it, and an own lane of it, has `external` in its entry, and such a job runs the actions after the lane's own setup in this order: `openssl`, then `secp256k1` (with `emscripten: true` on an entry that has emsdk), then `libdatachannel` (built with the lane's compilers, its entry's `cc` and `cxx`: `gcc-14` and `g++-14`; Visual Studio for MSVC); an entry on emscripten has neither OpenSSL nor libdatachannel. Its b2 command MUST carry `webcpp-require-external=on` so that a library not found stops it, and `-s<VARIABLE>=<dir>` for each variable the actions exported (`EXTERNAL_VARIABLES`), on the command line; the docs job gives its b2 the same when the plan's `external` is `true`; a container lane passes no variable of the environment into its container.

#### Scenario: A lane that builds trystero
- **WHEN** the clang-linux-22 lane builds trystero
- **THEN** its entry says `external` and its job installs OpenSSL, libsecp256k1 and libdatachannel before it runs b2 with `webcpp-require-external=on`

### Requirement: CI: Own-lane jobs
The job `own-lanes` SHALL run one job per entry of the own-lanes matrix (`name` and `os` as the entry says), which runs `matrix.py own-lane <entry>` in every directory that declares the lane there. One that names no target, `Own lane (<library>, <lane>)`, runs on Linux x86-64 (ubuntu-24.04) with the Boost action, the Node action and `b2 -a toolset=clang-18 <directory>//<lane> ...`, whose exit status is its verdict. One that names a target, `Own lane (<library>, <lane>, <target>)`, runs on its entry's image with the Boost action and what the target's lane installs, the same steps by their YAML anchors (`&wasi-sdk`, `&wasmtime`, `&wit-bindgen`, `&wasi-wit`, `&emsdk`) so that a step added to a WASI or emscripten lane is added to it too, and the Node action; then `b2 -a --dump-tests --out-xml=<id>.xml toolset=<toolset> [<options>] <directory>//<lane> ...`, and it uploads `<id>.xml` as the artifact `lane-<id>`. An own lane of the kind `original` has the CI's Node (its entry's `node`, `&node`: a native one would otherwise run with the image's own Node); one on emscripten has it as the emscripten lane does; one of the kind `programs` on any other target has none. An own lane whose driver meets a browser, one of `CHROME_LANES` (trystero's `interop`, natively and on emscripten), has `chrome` and its job runs the `chrome` action, which gives it `CHROME`, so that a red interop lane is never a browser nobody chose. A target the CI has no lane to set up an own lane on fails the listing by name: an own lane is never run natively in its place.

#### Scenario: wasi's served lane
- **WHEN** wasi declares the lane `http` on wasip2
- **THEN** the job `Own lane (wasi, http, wasip2)` installs Boost, wasi-sdk, wasmtime, wit-bindgen and the WASI WIT, installs no Node, runs the lane and uploads `lane-wasip2.wasi.http`

### Requirement: CI: The documentation job
The job `docs` SHALL run with MrDocs on Linux x86-64 (it has no build for Linux arm64 or Intel macOS), `clang++-18`, Node, wit-bindgen and the WASI WIT (wasi's reference parses its bindings), and build `b2 -a libs/<library>/doc` for a library or, for the superproject, `b2 -a doc -sWEBCPP_INDEX=site`, whose pages are the site's; when the plan's `has-emscripten` is `true`, with emsdk in `.local/emsdk` with the sysroot in its cache and without its system libraries (`libraries: 'false'`), and when the plan's `external` is `true`, with the external libraries built with Clang 18. It uploads the pages: the artifact `docs` for the superproject (the pages `ci.yml` publishes) and `docs-<library>` (`libs/<library>/doc/html`) for a library, and its output `built` is the workflow's `docs` output.

#### Scenario: The superproject's docs artifact
- **WHEN** the superproject's CI runs
- **THEN** the job builds the index and every page with `-sWEBCPP_INDEX=site` and uploads the artifact `docs`, which the site job downloads

### Requirement: CI: The lint job
The job `lint` SHALL run `tools/lint/lint.sh` in eight shards (`--shard 1/8` to `8/8`, a matrix whose job names show `Lint (K/N)`), with wasi-sdk's clang-format and clang-tidy, Node, Clang 18 as b2's default toolset and the wasip2 and wasip3 toolsets after it (`matrix.py register clang-18 wasip2 wasip3`), wit-bindgen and the WASI WIT, which the WASI dry runs of the compilation database need; when the plan's `has-emscripten` is `true`, the emscripten toolset after them and emsdk without its system libraries; when `lint-external` is `true`, the external libraries built with Clang 18 (libsecp256k1 for emscripten too when `has-emscripten` is); and the full history (`fetch-depth: 0`) of the superproject and of the library, since the banned-word rule reads every commit.

#### Scenario: The history is fetched whole
- **WHEN** the lint job checks out
- **THEN** it uses `fetch-depth: 0`, so the banned-word rule can read every commit

### Requirement: CI: The tools job and actionlint
The job `tools`, for the superproject only (`if: inputs.library == ''`), SHALL run every `tools/**/*_test.py` in eight shards with Clang 18 and the wasip2, wasip3 and emscripten toolsets, wasi-sdk, wasmtime, wit-bindgen, the WASI WIT, emsdk, Node and MrDocs, each failure named; the tests of the emscripten target share the cache the emsdk action restores; a suite run by `harness.run_cases` runs its slice of cases in every shard and every other suite runs whole in one shard, in turn. The job `actionlint` MUST download actionlint 1.7.12, checked against its SHA-256, and run it on every workflow of the superproject and of `libs/*` with `.github/actionlint.yaml` (which declares the self-hosted-runner label `ubuntu-26.04`, a GitHub image GA since 2026-09-17 that actionlint 1.7.12 predates); it also runs clean locally before a workflow change is committed.

#### Scenario: A workflow with a typo
- **WHEN** a workflow references an undefined input
- **THEN** actionlint fails the job, naming the file and line

### Requirement: CI: The report job
The job `report` SHALL run `matrix.py report --plan <matrix> --own-lanes <matrix> --lanes bin/lanes --out bin/report`, merging the XML of every lane and of every own lane on a target with `tools/report/report.py` into the test matrix, uploaded as the artifact `report` (always, `if-no-files-found: error`, 7 days). Its exit status is the verdict; a planned lane or own lane on a target that wrote no XML fails it by name, and a final step MUST fail the job when a lane's job did not succeed or an own lane's job is other than `success` when the plan found own lanes (`skipped` otherwise).

#### Scenario: A lane's job failed after writing its XML
- **WHEN** the gcc-15 lane's job failed after b2 wrote its XML
- **THEN** the report may read the XML as whole and the step "Every lane's job succeeded" still fails the job

### Requirement: CI: The actions of the repository
The CI SHALL install its tools with composite actions under `tools/ci/actions/`, each a script beside its `action.yml`: `boost` downloads `boost_1_92_0.tar.gz` from `https://archives.boost.io/release/1.92.0/source/` (the gzip archive, since Windows Server 2022's `tar.exe` has no bzip2) checked against the SHA-256 it records, builds b2 with `bootstrap`, installs Boost's headers (`--with-headers`, `--layout=system`, in `<prefix>/include/boost`) and b2 (`b2-install-layout=standard`, in `<prefix>/bin`) into a prefix cached per image, keeps beside them in `<prefix>/source` the archive's `boost/version.hpp`, every `libs/<library>/src` and the marker `.webcpp-boost-source`, writes the `using boost` line of `.local/user-config.jam` with the `modules.poke : BOOST_SOURCE` that names it, puts b2 on `PATH` on Linux, macOS and Windows, fails the action before caching when the prefix lacks the headers, b2 or the sources, and refuses an empty or relative prefix (b2 given an empty `--prefix` installs into `/usr/local`); `wasi-sdk` installs wasi-sdk 34 into `.local/wasi-sdk`; `wasmtime` installs wasmtime 47.0.3 on `PATH`; `mrdocs` installs MrDocs 2026.9.29 into `.local/mrdocs`; `node` sets up Node 26.7.0 with npm's cache keyed on the lock files of `tools/doc`, `tools/lint`, every library's `test/oracle` and the site's; `wit-bindgen` installs wit-bindgen 0.62.0 (the release's archive for the runner's system and processor, the program alone) into `.local/wit-bindgen`; `wasi-wit` installs the `wit/deps` directories of the crates `wasip2` 1.0.4 and `wasip3` 0.9.0 from `https://static.crates.io/crates/` into `.local/wasi-wit/p2` and `.local/wasi-wit/p3`, both replacing what an earlier install left.

#### Scenario: The Boost prefix on Windows
- **WHEN** the Boost action installs on Windows
- **THEN** the headers go to `<prefix>/include/boost` and b2 to `<prefix>/bin`, since Boost's default layout there is versioned and b2's portable layout ignores `--bindir`

#### Scenario: An empty prefix
- **WHEN** the action is given an empty prefix
- **THEN** it refuses, because b2 would otherwise install into `/usr/local`

### Requirement: CI: The emsdk action
The `emsdk` action SHALL install Emscripten 6.0.11 into `.local/emsdk` on Linux and macOS, x86-64 and arm64: the emsdk repository at the commit of its tag `6.0.11`, fetched by its hash with git (the install fails unless `HEAD` is the pinned commit, and keeps no `.git`), and every archive `emsdk install 6.0.11` installs from (the release's binaries, Node 24.19.0 and, on macOS, Python 3.13.3), each downloaded first and checked against the SHA-256 it records (emsdk checks none; it runs with `EMSDK_KEEP_DOWNLOADS=1`, under which an archive it downloads besides them fails the action, naming it, before `emsdk activate`); a release whose binaries hold no `install/emscripten/node_modules` fails before emsdk runs. It checks that `emcc --version` names `6.0.11 (a0014542110d6078c3a1a7941fa1ddb3a2281f16)`, installs `tools/ci/actions/emsdk/node.sh` as `.local/emscripten/node` (the node the toolset runs a program with, which refuses `--experimental-wasm-threads` without a word since b2 1.92's emscripten toolset probes Node with that flag), gives the later steps `EM_CACHE` (`.local/emscripten-cache` by its resolved path, outside the emsdk) and warms that cache (it runs `emcc --version` once and fails unless `em++ --version` then says nothing more). On a cache miss it builds with `embuilder` the system libraries the lanes link (libc, libc++ and libc++abi in their debug, exception and noexcept variants, compiler-rt, dlmalloc, the unwinder and the default stubs); the emsdk and the cache are cached together, keyed on the version, the runner and the action's files; a job that only parses Emscripten's headers (docs, lint) gives `libraries: 'false'`. The toolset's lines are the regions `emsdk` and `emscripten` of `tools/ci/emsdk.jam`.

#### Scenario: A different commit of emsdk
- **WHEN** the fetched emsdk's `HEAD` is not the pinned commit
- **THEN** the action fails

#### Scenario: The node wrapper
- **WHEN** b2's emscripten toolset probes Node with `--experimental-wasm-threads`
- **THEN** `.local/emscripten/node` refuses the flag silently, so no output of b2 holds Node's error

### Requirement: CI: The actions for external libraries and the browser
The actions `openssl`, `secp256k1` and `libdatachannel` SHALL install trystero's external libraries where its `build.jam` looks for them, in `.local/`, and give the later steps their variables (`$GITHUB_ENV`): `openssl` lays out the runner's own OpenSSL 3 in `.local/openssl` and exports `OPENSSL_ROOT` (on Linux the system's, which `pkg-config` names; on macOS Homebrew's `openssl@3`, installed with `brew` when the image lacks it; on Windows copies of the headers and import libraries of the image's installation, whose `bin` it puts on `PATH`), as links to `include/openssl` and the two libraries so that a compiler is given a directory of its own and never `/usr/include`, with the headers of Debian's multiarch `opensslconf.h` and `configuration.h` linked beside the others, and fails, naming it, on a layout without `opensslconf.h` or on one that is not OpenSSL 3; `secp256k1` builds libsecp256k1 0.8.0 from GitHub's archive of its commit `6e2c8bc` checked against its SHA-256, with CMake, static, with its `schnorrsig` and `extrakeys` modules, into `.local/secp256k1-native` (and with `emscripten: true` into `.local/secp256k1-emscripten` with `emcmake`), exporting `SECP256K1_ROOT` and `SECP256K1_EMSCRIPTEN_ROOT`; `libdatachannel` clones the tag `v0.24.6`, fails unless `git rev-parse HEAD` is the tag's commit `6b1e2e6` (read with `gh api`), and only then fetches its submodules at the commits that commit's gitlinks name, builds it shared (`NO_WEBSOCKET`, `NO_EXAMPLES`, `NO_TESTS` on, `NO_MEDIA` off) against `OPENSSL_ROOT` into `.local/libdatachannel` and exports `LIBDATACHANNEL_ROOT`. The two that build are cached, keyed on the version, the runner, the compilers' own `--version`, the emscripten build or OpenSSL's version and the action's files; on Windows they build CMake's Debug configuration, whose `/MDd` standard types match b2's debug variant. The action `chrome` installs Chrome for Testing's `chrome-headless-shell` 155.0.8059.39 from the URL Google's `last-known-good-versions-with-downloads.json` lists, checked against the SHA-256 recorded for each build, into `.local/chrome-headless-shell` on Linux x86-64 and arm64, and exports `CHROME`, a wrapper that runs it with `--no-sandbox` (a decision kept until measured on a runner, with the threat model recorded: the shell loads only the interop driver's own pages on 127.0.0.1, with every other host unresolvable).

#### Scenario: A libdatachannel tag that moved
- **WHEN** `v0.24.6` points to a commit other than `6b1e2e6`
- **THEN** the action fails before it fetches any submodule

### Requirement: CI: The container action and the shared scripts
The action `container` SHALL give a job the container lanes' image: built from `tools/ci/container/Dockerfile` when the cache does not hold it, saved under the Dockerfile's digest and loaded from there otherwise. `tools/ci/download.sh <url> <sha256> <file>` MUST download each pinned file and leave no file and exit 1 when the download fails or the digest differs. `tools/ci/helpers.sh` holds what the actions share, which each `install.sh` sources beside `download.sh`: `digest` (the SHA-256 of files one after the other), `mixed` (a path as Windows programs read it), `configuration` (CMake's `Debug` on Windows and `Release` elsewhere) and `compiler_identity` (the first line of the `--version` of each of `CC` and `CXX` that is set, else the image), and no action defines a function of its own that `helpers.sh` holds. `tools/ci/actions_test.py` pins `download.sh` and `helpers.sh`, the Boost action's prefix checks and layouts, where the wit-bindgen, WASI WIT and emsdk actions install what they download and their refusals (a runner no build is pinned for, a crate that holds no WIT, an emsdk at another commit, an archive emsdk would download unpinned, an emcc that is not 6.0.11), the external libraries' commands, install places, exports and cache keys, the chrome action's shell and its `--no-sandbox` wrapper, and the container action's image.

#### Scenario: A digest mismatch
- **WHEN** a downloaded archive's SHA-256 differs from the pinned one
- **THEN** `download.sh` leaves no file and exits 1

### Requirement: CI: Where a change flows
A library's commits SHALL reach its own `main` first; then the superproject's commit that moves its submodule is pushed to the superproject's `main`, whose CI tests every library at the commits its submodules point to. What fails there is fixed with a test, in the library or in the superproject, by more commits on `main`; then each library's CI runs again against the superproject's new `main`. `main` is the only branch.

#### Scenario: A failure only the superproject sees
- **WHEN** the superproject's CI fails for a library at the commit its submodule records
- **THEN** a test reproducing it is added in the library or the superproject, the fix lands on `main`, and each library's CI runs again

### Requirement: Release: The version and the single source
webcpp SHALL have one version, `WEBCPP_VERSION` in `<webcpp/version.hpp>` (`libs/config`), 0.1.0 being `100` and `WEBCPP_LIB_VERSION` `"0_1"`, the one place it is written; the Jamroot reads it for the release targets and `tools/release/release.py` reads the same `#define` line, and both refuse anything but the number after `WEBCPP_VERSION` and trailing spaces. `release.py` MUST agree with the Jamroot on a decimal number without a leading zero (which C++ reads as octal), and name an unreadable header.

#### Scenario: A leading zero
- **WHEN** the line reads `#define WEBCPP_VERSION 0100`
- **THEN** both readers refuse it

### Requirement: Release: Changelogs and their form
Every change a user can see SHALL add its line, in the same commit: a library's goes in its `doc/changelog.adoc` under "Unreleased", one of the superproject's tools or build in the release notes of the version in preparation, `doc/releases/<version>.adoc`, under "Tools and build" (what changes for whoever builds, lints or tests with it: its b2 rules and targets, the lint's rules, the CI's lanes). Agents write them: for webcpp this replaces the rule of never editing a changelog. A line is one bullet, `* `, a past-tense verb (Added, Changed, Deprecated, Fixed, Improved, Removed or Updated), then one sentence of about ten words, what a user sees rather than how it was done; it may wrap onto the next lines. A breaking change starts with `*Breaking change:*` and says what the user must change; a deprecation names what replaces it; a fixed bug names the symptom with its issue or pull request when there is one, `https://github.com/webcpporg/<library>/issues/<n>[#<n>]`. A line holds no cross-reference `<<...>>`, because the index page shows it too, where the library's anchors are not. At most ten lines per library and version, the most important first, a version's documentation work being one line; a change no user sees (a test, its CI, a refactoring) gets none. The file is `[appendix]`, `[#<library>-changelog]`, `== Changelog`, a paragraph, then `=== Unreleased` and each released version `=== webcpp <version>`, newest first, each with its anchor (`[#<library>-changelog-unreleased]`, `[#<library>-changelog-0-1-0]`) and its lines between `// tag::unreleased[]` (`// tag::<version>[]`) and its `end::`; a released section is never rewritten, a correction is a line of the next version.

#### Scenario: A fixed bug
- **WHEN** a bug is fixed
- **THEN** its line starts "Fixed", names the symptom, and links its issue as `https://github.com/webcpporg/<library>/issues/<n>[#<n>]` when there is one

#### Scenario: A released section
- **WHEN** a correction to 0.1.0 is needed
- **THEN** the line goes under the next version, since a released section is never rewritten

### Requirement: Release: The release notes
The release notes `doc/releases/<version>.adoc` SHALL hold what belongs to the collection, in Boost's sections, with anchors `[#webcpp-<a-b-c>-<slug>]`: `= webcpp <version>` (anchor `[#webcpp-<a-b-c>]`), the paragraph "In preparation: no tag names this version yet, and its libraries' entries are their changelogs' "Unreleased" sections." while the version is in preparation, then New Libraries (one line each, `link:{library-pages}<name>/{library-page}[<name>]: <what it is>, from <authors>.`), Updated Libraries (each library a section `[#webcpp-<a-b-c>-<name>]` / `=== link:...[<name>]` that includes its changelog's region with `include::{webcpp-root}/libs/<name>/doc/changelog.adoc[tag=...]`, alphabetical), Removed Libraries (what went out and what replaces it), Tools and build, and Compilers tested (the CI's lanes); a section with nothing holds `None.`. While a version is in preparation its notes include each library's `tag=unreleased`. The index page includes each version's notes newest first. `tools/release/template.adoc` is the template of a new version's notes, with `@VERSION@` and `@ANCHOR@`.

#### Scenario: The notes of 0.1.0
- **WHEN** `doc/releases/0.1.0.adoc` is read at `c53f3f9`
- **THEN** it is "In preparation", lists config, document, idna, lexical, pratt, trystero, wasi, xactor and xstate under New Libraries, has Updated Libraries and Removed Libraries as `None.`, and lists the tools and build changes under Tools and build

### Requirement: Release: check-release
`b2 check-release` (`release.py check`) SHALL fail, naming each fault on a line `check: <fault>`, when it finds any of these: a change HEAD does not record in the superproject or a library (a changed, staged, deleted or new file git does not ignore; named first and alone); the version disagreeing anywhere (`WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` naming different versions, or the version the Jamroot read not being theirs); notes missing or breaking the form; the section Releases of `doc/index.adoc` not including every version's notes once, newest first, this version's first; notes in preparation including a region other than `tag=unreleased` or a changelog already holding the version's section; frozen notes including a region other than `tag=<a.b.c>`, an Unreleased holding lines, Compilers tested not `None.`, or a tag `webcpp-<a.b.c>` not at HEAD; Compilers tested listing other lanes than the CI's `LANES`; a library that is no git checkout of its own, no submodule at HEAD, not checked out or at another commit than the superproject records; New Libraries or Removed Libraries not exactly the libraries new or gone since the newest earlier tag `webcpp-<x.y.z>` (every library for the first version); an Updated Libraries name that is new, removed or no library, or whose region holds no line; a library whose changelog has lines for this version not under Updated Libraries; a library changed since that tag under `include/`, `build.jam`, `wit/`, `meta/libraries.json`, `README.md`, `example/`, `doc/` or `LICENSE*` with no line; and, once `doc/releases/<a.b.c>.whats-new.md` exists, HTML other than its placeholder comment or a Highlights section that breaks its form. It exits 0 when nothing is wrong, 1 with the faults and 2 on a usage error or an unreadable file. A submodule of HEAD not checked out fails it by name (`git submodule update --init libs/<name>`).

#### Scenario: A library changed since the last tag with no line
- **WHEN** `libs/pratt/include/` changed since the newest tag and its changelog region has no line
- **THEN** `check-release` fails naming pratt

#### Scenario: A dirty tree
- **WHEN** a tracked file has an uncommitted change
- **THEN** `check-release` names only that, since every other rule judges what the tag would carry

### Requirement: Release: whats-new
`b2 whats-new` (`release.py whats-new`) SHALL write `doc/releases/<version>.whats-new.md`, the skeleton of the GitHub Release's body, in Boost's form, from the version's release notes, in Markdown: the notes' own prose, each new and removed library's one line, each updated library's first three lines and the Tools and build section's first three, and a section Highlights holding a comment that names the form `check` reads back once an agent, and then the owner, have written them in its place: at most five highlights, each `- **<category>**: <sentence of at most 20 words>.`, categories in the order of `CATEGORIES` and a category with nothing to say left out. It never writes over a file that exists.

#### Scenario: A What's new that exists
- **WHEN** `doc/releases/0.1.0.whats-new.md` exists
- **THEN** `b2 whats-new` does not overwrite it

#### Scenario: Six highlights
- **WHEN** the section Highlights holds six highlights
- **THEN** `check-release` fails

### Requirement: Release: dist
`b2 dist` (`release.py dist`) SHALL write `bin/dist/webcpp-<version>.tar.gz` and `bin/dist/webcpp-<version>.tar.gz.txt` (its `sha256sum` line), after building every page: the files the superproject's HEAD records and those each library's HEAD records, read from git's objects and never from the working tree, and the pages the doc build writes (`doc/html/index.html` for the index and each library), all under `webcpp-<version>/`, no path with a part that starts with a dot. It MUST refuse, naming each fault, a change HEAD does not record, a version other than `version.hpp`'s, a library that is no checkout of its own, not checked out, no submodule at HEAD or at another commit than HEAD records, a submodule other than the libraries' own, a link whose target is not UTF-8, absolute, outside its repository or a dot file, a page not built, and any other file under a `doc/html`. The archive is byte-for-byte reproducible: its members in the order of their names' code points, each with the time of the superproject's HEAD commit, owner and group 0 and no names, mode 755 for a file git records as executable, 777 for a link and 644 otherwise; its gzip header with no name, no time and the operating system unknown, its deflate zlib's at level 9; a Python whose zlib deflates otherwise (zlib-ng) is refused before anything is read; no tar or gzip program is run; the archive and its checksum are written to temporaries then renamed, the archive first, a fault leaving both as they were and one renaming the checksum saying the new archive stands beside the old checksum.

#### Scenario: Reproducible bytes
- **WHEN** `b2 dist` runs twice from the same commits and pages on different machines and time zones
- **THEN** the two archives are the same bytes

#### Scenario: zlib-ng
- **WHEN** Python's zlib deflates differently from CPython's linked zlib
- **THEN** `dist` refuses before reading any file

### Requirement: Release: The procedure
Nothing SHALL be released until the owner asks. Then: (1) in every library whose "Unreleased" holds lines it becomes `=== webcpp <version>` with its anchor and region under a new empty "Unreleased", the notes lose their paragraph "In preparation: ..." and include `tag=<version>`, each repository commits and the superproject records each library's commit; (2) `b2 check-release` passes; (3) `b2 whats-new`, an agent writes the highlights from the notes and the owner approves them before the tag; (4) `b2 -a dist`; (5) the tag `webcpp-<version>` goes on the superproject and on every library at the commit the superproject records (whether it is signed is the owner's decision then); (6) the GitHub Release `webcpp-<version>` carries the archive, its checksum file and the "What's new" as its body (`gh release create webcpp-<version> bin/dist/webcpp-<version>.tar.gz bin/dist/webcpp-<version>.tar.gz.txt --title "webcpp <version>" --notes-file doc/releases/<version>.whats-new.md`); (7) right after, `version.hpp` names the next version, whose notes are opened from `tools/release/template.adoc` and included first by the index. At the commit of this proposal 0.1.0 is in preparation: no tag `webcpp-0.1.0` exists.

#### Scenario: After the tag
- **WHEN** the tag `webcpp-0.1.0` is placed
- **THEN** `version.hpp` is moved to the next version and `template.adoc` opens its notes with `sed -e 's/@VERSION@/<next>/g' -e 's/@ANCHOR@/<next with dashes>/g'`

### Requirement: Organization: The roadmap that is not built
The organization SHALL keep, in chapter 13 of `AGENTS.md`, what it does not have yet, and no other chapter may state it as present: every variant tested in three levels of CI (fast on every push, nightly on main, release); the review of the existing libraries against the rules of exceptions, variants, allocators, the order of what a library builds on, writing for the web, the newest Boost library of a kind, wrapped outside libraries and fixed-width types (the lint then checking the last); asynchronous libraries on xactor with Boost.Asio and Boost.Cobalt; webcpp::asio, webcpp::beast, webcpp::webrtc and webcpp::crypto; the libraries web developers use most (QuickJS, libp2p, HTTP fetch); documentation for web developers and AI developers and integrations with the AIs; external libraries built from `third_party/`; every library on `<webcpp/config.hpp>` (trystero held with config's publication, wasi with its redesign); the allocator rule applied to xactor, xstate and wasi; the compiled Boost libraries' remaining gaps (Boost.Locale and Boost.Cobalt without exceptions, Windows not measured, a Boost.Test suite compiling the framework's header-only form); bundled toolchains; and the site's increments 2 to 8.

#### Scenario: A rule not yet true
- **WHEN** the CI levels are asked about
- **THEN** `AGENTS.md` chapter 13 says they are a roadmap item and the CI today runs one level on every push

### Requirement: Superproject: Agent skills
The superproject and the proposals repository SHALL keep agent skills in `.agents/skills/<name>/SKILL.md`, one directory per skill named in kebab case, each `SKILL.md` opening with YAML front matter whose `name` equals the directory and whose `description` says when the skill applies. A skill MUST point to the rules that apply (the chapter of the superproject's `AGENTS.md`, `CONTRIBUTING.md` or the organization's specification) and list the steps and commands of its task, without restating those rules at length; where a skill and a rule disagree, the rule wins. Skills follow the rules on text of every file: English, no em dash, no banned word, no personal data. The proposals repository holds OpenSpec's own skills, generated by OpenSpec 1.10.0 and adapted to its several OpenSpec roots, and `openspec init` is never run in the repository itself, since it would create a root `openspec/`. Each repository's `AGENTS.md` names its skills. CI MUST parse skill front matter as YAML and reject invalid syntax, a name that differs from its kebab-case directory, or a missing or blank string description. Markdown files, including `SKILL.md`, need no webcpp source-file licence header; upstream notices MUST be preserved.

#### Scenario: An agent writes a proposal
- **WHEN** an agent in the proposals repository is asked to propose a change to a library
- **THEN** the skill `webcpp-openspec-proposal` sends it to `CONTRIBUTING.md` and the organization's specification, and it runs `openspec` from `libraries/<name>/`

#### Scenario: A skill with forbidden text
- **WHEN** a `SKILL.md` under `.agents/skills/` of the proposals repository holds an em dash
- **THEN** `python3 tools/proposals.py check` fails, naming the file and the line

#### Scenario: A skill and a rule disagree
- **WHEN** a chapter of the superproject's `AGENTS.md` changes a rule a skill cites
- **THEN** the chapter is followed, and the skill is updated to point to it

#### Scenario: Invalid skill metadata
- **WHEN** a skill's description contains an unquoted colon and space, its name differs from its directory, or its description is absent or blank
- **THEN** the Agent skills workflow fails and names the skill
