# Working on webcpp

This file is how to work in webcpp's tree, for coding agents and human
contributors alike: what webcpp is and where everything is, the
prerequisites and the commands, the API reference and the pages, the tests,
the lanes, the report, the site and the CI, the changelogs and the
releases, and the facts of b2. The rules every change keeps, how code is
written and every decision of webcpp's design with its reason, are in
[CODING-GUIDELINES.md](CODING-GUIDELINES.md), the single source of them.
Read it first, then this file, then the library's own `AGENTS.md`
(`libs/<name>/AGENTS.md`), which holds only what is specific to that
library: the original and its version, how its oracle drives the original,
its particular cautions. With the two files, an agent with no other context
ports a library or changes one.

Agent skills, short playbooks that point into the two files for recurring
tasks, are in `.agents/skills/<name>/SKILL.md`: the library's conventions, a
port, the build and its lanes, the documentation and the site, and text and
commits. They never replace a rule; where one disagrees, the file it points
to wins.

This file describes the tree as it is. What is still to come is in the
roadmap, `CODING-GUIDELINES.md` chapter 16, to which every other mention of
it refers. Every command below runs from the superproject's root, in a
shell where `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` are unset
(chapter 6). A chapter of this file is cited as "chapter N", one of the
guidelines as "`CODING-GUIDELINES.md` chapter N".

## Contents

1. What webcpp is
2. The API reference
3. Documentation
4. Tests, lanes, the report, the site and CI
5. Changelogs and releases
6. b2 facts

## 1. What webcpp is

webcpp brings to C++ the libraries that are widely used for the web in other
languages. Each library is ported with the help of coding agents and proven
against the original. webcpp follows Boost's guidelines and model: the GitHub
organisation `webcpporg` is the equivalent of `boostorg`, and this
superproject, `webcpporg/webcpp`, is the equivalent of `boostorg/boost`. Every
library is header-only C++20, under the Boost Software License 1.0, in a
repository of its own, checked out here as a submodule under `libs/<name>`.

### Why webcpp

webcpp builds infrastructure for every developer, in both directions. It
brings the best of C++ within easy reach of the web: the libraries of
highest quality, compiled to WebAssembly. And it brings to C++ the libraries
web developers use most (a JavaScript interpreter, trystero, libp2p,
HTTP fetch), so that programs written in C++ and compiled to WebAssembly
have them. It speaks to C++ developers, crypto developers and AI developers
who want to build decentralized applications for the web. AI agents are
first-class readers: they use what webcpp publishes to find libraries of
high quality, and webcpp will release integrations with the AIs, to bring
the best technology in the world within reach of C++ programs.

Each target has its purpose:

| Target | For |
| --- | --- |
| emscripten | the browser: a C++ program as a web page, with everything the browser offers |
| wasip2, wasip3 | WASI components run by wasmtime and by clouds: containers lighter than Docker, distributed computing at scale |
| native | a program that wants close integration with its operating system |

The documentation is written for C++ developers, and will also be written
for web developers and for AI developers (`CODING-GUIDELINES.md` chapter 16).

### The libraries

| Library | What it is | Targets | State |
| --- | --- | --- | --- |
| config | webcpp's configuration, which every library includes first: the target, exceptions and webcpp's version, as Boost.Config is Boost's; webcpp's own | native, wasip2, wasip3, emscripten | a submodule at `libs/config` |
| xactor | a deterministic actor system, webcpp's own | native, wasip2, wasip3 (`xactor_asio` native only) | a submodule at `libs/xactor`; the model for every port |
| xstate | a port of XState 5.33.2's state machines and actors; depends on xactor and Boost.JSON | native, wasip2, wasip3; its oracle lane | a submodule at `libs/xstate`; the first user of the shared oracle (`CODING-GUIDELINES.md` chapter 12) |
| pratt | a Pratt parser engine, generic through concepts, with a calculator built on it, webcpp's own | native, emscripten, wasip2, wasip3 | a submodule at `libs/pratt`; born with the allocation rule (`CODING-GUIDELINES.md` chapter 4) |
| wasi | header-only helpers for C++ built as WebAssembly components: the HTTP handler a component exports, webcpp's own | wasip2, wasip3 (`response.hpp` also natively); its own lanes `http`, the components wasmtime serves | a submodule at `libs/wasi`; the first library built as components (chapter 4) |
| document | a JSON document that behaves as JavaScript does (key order, `JSON.stringify`'s bytes, the array methods), exact in decimal, in one relocatable image; webcpp's own; its parser is Boost.JSON's | native, wasip2, wasip3, emscripten; its oracle lane | a submodule at `libs/document`; its core never throws (`CODING-GUIDELINES.md` chapter 6) |
| idna | internationalized domain names: Punycode, UTS 46's ToASCII and ToUnicode, conformant to IdnaTestV2, and IDNA2008's validity, over Unicode 17.0.0 tables generated into its headers; webcpp's own | native, wasip2, wasip3, emscripten; its own lane `tables` | a submodule at `libs/idna`; its errors are values, and its span forms allocate nothing |
| lexical | the string formats JSON Schema names (date-time, email, hostname, ipv4/ipv6, uri, iri, uri-template, json-pointer, uuid, regex), each a validator that allocates nothing and a Boost.URL grammar rule; regex is an ECMA-262 syntax validator of its own, checked against V8; depends on idna and Boost.URL; webcpp's own | native, wasip2, wasip3, emscripten; its own lanes `generated` and `oracle` | a submodule at `libs/formats` |
| trystero | a port of Trystero 0.26.0, serverless WebRTC rooms | native, emscripten; its own lanes `oracle` and `interop` | a submodule at `libs/trystero`; born with the allocation rule through `std::pmr` |

### The layout

```
webcpp/
  Jamroot             the single build configuration: finds Boost, checks its version,
                      sets what every library shares, registers each library
  AGENTS.md           how to work in the tree: the tools, the commands, the
                      lanes, the CI and the releases
  CODING-GUIDELINES.md  the rules on how code is written and every design
                      decision, with its reason: the single source of them
  README.md           what webcpp is, the prerequisites, getting started
  LICENSE_1_0.txt
  .clang-format       clang-format's style, every repository's, which a library's own may
                      extend (below)
  .clang-tidy         clang-tidy's checks, every repository's
  pyrightconfig.json  Pyright's settings for every Python file
  libs/<name>/        a library: a repository of its own, a submodule here (config, xactor,
                      xstate, pratt, wasi, trystero, document, idna, lexical)
  third_party/        third-party code the libraries and their tests use, not webcpp's: each
                      a submodule under third_party/libs/, pinned at a commit and keeping its
                      own licence, and the Jamfile that declares it as
                      /webcpp/third_party//<target> (guidelines chapter 10)
  doc/                the index page (index.adoc, Jamfile)
    releases/         each version's release notes, which the index page shows
  site/               the site, a repository of its own, webcpporg/webcpporg.github.io, a
                      submodule here: its pages, components, tools and tests (chapter 4)
  tools/
    webcpp.jam        the Jamfile API (chapter 4): webcpp.targets, webcpp.run, ...
    target.jam        the targets, and the target a toolset builds for, which webcpp.jam and
                      component.jam both read
    throw_exception.cpp  what Boost calls in place of a throw, in a user's build without
                      exceptions
    boost_json.cpp    Boost.JSON's definitions, built as /webcpp//boost_json
                      (guidelines chapter 10)
    boost_test_runner.cpp  Boost.Test's header-only framework, for webcpp.boost-test
                      (guidelines chapter 8)
    boost/            Jamfile: the compiled Boost libraries built from Boost's sources
                      (/webcpp//boost_url, boost_charconv, boost_locale and
                      boost_cobalt)
    oracle/           the shared oracle (guidelines chapter 12): oracle.jam,
                      twins.py, compare.py and their tests
    lint/             lint.sh, rules.py, compile_commands.py and their test
    doc/              the documentation toolchain: doc.jam, reference.py, doc_comments.py,
                      doc-check.py, libraries.py, counts.py, the Asciidoctor.js extensions, the
                      style
    example/          run_example.py, which runs an example and compares its output
    component/        component.jam, the rules of a WebAssembly component (webcpp.wit-bindings,
                      webcpp.serve, webcpp.serve-script), and serve.py, which serves one with
                      wasmtime and compares its answers, and its test
    drive/            drive.jam, the rule of a driven test (webcpp.drive), which a script of the
                      original's language runs over a linked program, drive.py, which runs it
                      bounded, and its test
    generated/        generated.jam, the rule of a library's generated files (chapter 4),
                      generated.py, which runs a generator and checks or writes what it writes,
                      and its test
    node/             install.py, which installs the Node packages of the documentation, an
                      oracle and the lint once per lockfile, and its test
    report/           report.py, lanes.py, pages.py: the test matrix, and the CI verdict
    test/             the tests of the Jamroot, the compiled Boost libraries, webcpp.jam,
                      third-party code, the oracle's rules, the check of generated files,
                      the component rules, the emscripten target, the libraries webcpp does
                      not build and the doc build, their harness, and the fixture libraries
                      demo, oracle_demo, component_demo and browser_demo
    ci/               matrix.py (the lanes), assemble.py (the site), download.sh and
                      helpers.sh (what the actions share), wasi-sdk.jam
                      and emsdk.jam (the lines that register the WASI and emscripten
                      toolsets), container/Dockerfile (the image of the container lanes),
                      and actions/{boost,wasi-sdk,wasmtime,wit-bindgen,wasi-wit,emsdk,
                      secp256k1,libdatachannel,openssl,chrome,container,mrdocs,node}/
                      (chapter 4)
    site/             site.jam, the rules of b2 site and b2 site-serve, catalogue.py (the
                      site's libraries.json), redirect.py (the old address's two pages) and
                      their tests (chapter 4)
    release/          changelog.py, the form of a changelog, which the lint checks; release.py,
                      the release tooling; template.adoc; scratch.py and their tests
  .github/            workflows/library.yml, workflows/ci.yml, actionlint.yaml (chapter 4)
  .local/             machine-local, git-ignored (below)
  bin/                b2's build directory, git-ignored
```

A library's layout:

```
libs/<name>/
  build.jam                  project /webcpp/<name>: its headers and dependencies
  include/webcpp/<name>.hpp  the convenience header, which includes every public header but
                             one that brings an optional heavy dependency
                             (guidelines chapter 3)
  include/webcpp/<name>/...  one header per responsibility
  wit/                       the worlds of a library that builds WebAssembly components, whose
                             bindings its build.jam declares (wasi's http-p2.wit and http-p3.wit)
  test/                      Jamfile, the tests, and .clang-tidy when the tests need one
  test/oracle/               a port's oracle (guidelines chapter 12): its Jamfile,
                             the pinned original, the scripts that drive it, and the
                             twins
  example/                   Jamfile, the programs and their .expected outputs, and a served
                             program's .requests (chapter 4)
  doc/                       Jamfile, the page (<name>.adoc and its sections), mrdocs.yml,
                             and counts.py when the page counts what only it holds (chapter 3)
  doc/changelog.adoc         its changelog, the page's last appendix
  meta/libraries.json        Boost's fields, plus "port-of"
  meta/include-boundaries.json
                             the include boundaries the lint checks, when it has any
                             (guidelines chapter 9)
  README.md
  AGENTS.md                  only what is specific to this library
  LICENSE_1_0.txt
  LICENSE-<ORIGIN>.txt       the original's notice, for a port that derives from its code
  .gitignore                 doc/html/ at least, and node_modules and .node-modules/ with an oracle
  .gitattributes
  .clang-format              when the library needs one: it inherits the root's
                             (InheritParentConfig) and adds only what its code needs, with the
                             reason in a comment, as wasi's names the macros its main is
                             written between (MacroBlockBegin, MacroBlockEnd)
  .github/workflows/ci.yml   calls the superproject's library.yml (chapter 4)
```

A library has no Jamroot. It is developed inside a checkout of the
superproject, as a Boost library is developed inside Boost: the superproject's
Jamroot is its build configuration, and `libs/<name>` is its repository.

### Prerequisites

- Boost 1.92 or newer, installed, and, for a library that links a compiled
  Boost library (Boost.URL, Boost.Charconv, Boost.Locale), Boost's sources of
  the same version (below);
- b2 (B2 5.5.3 measured);
- a C++20 compiler;
- Python 3.9 or newer;
- for WebAssembly: wasi-sdk 34 and wasmtime 47 (47.0.3 measured);
- for WebAssembly components, such as wasi's: wit-bindgen 0.62.0, which
  generates a world's C bindings, and the `wasi:http` WIT of WASI 0.2.12 and
  0.3.0, the `wit/deps` directories of the Rust crates `wasip2` 1.0.4 and
  `wasip3` 0.9.0, which a world resolves its packages against; wasmtime
  serves the components;
- for the documentation: Node, MrDocs 2026.9.29 and clang++, and wit-bindgen
  and the WIT for a reference that parses a component's bindings (wasi's),
  and emsdk, whose `em++` and `clang++` read a reference for Emscripten's
  target, that of a library with a header that builds only on emscripten
  (`-sWEBCPP_EMSDK=<dir>`, else `.local/emsdk`, chapter 2);
- for a library's oracle lane: Node and npm, with Boost and a C++ toolset;
- for the lint: wasi-sdk 34's clang-format, clang-tidy and clang++, Node,
  the wasip2 and wasip3 toolsets, the emscripten toolset when a library
  declares emscripten, and wit-bindgen and the WIT, since it reads what those
  toolsets compile;
- for emscripten: Emscripten 6.0.11, from emsdk, and Node, which runs its
  programs; the tests of the build need it too;
- for trystero, until webcpp builds them from `third_party/`
  (`CODING-GUIDELINES.md` chapter 16): libsecp256k1 0.8.0, static, with its
  `schnorrsig` and `extrakeys` modules, natively and, for emscripten, built
  with Emscripten; and, for its native backend, libdatachannel 0.24.6, built
  with the compiler and the standard library that build the programs, and
  OpenSSL 3. Each is found in the directory of its `include/` and `lib/` that
  `-sSECP256K1_ROOT=<dir>`, `-sSECP256K1_EMSCRIPTEN_ROOT=<dir>`,
  `-sLIBDATACHANNEL_ROOT=<dir>` and `-sOPENSSL_ROOT=<dir>` on b2's command
  line name, never the environment, else in `.local/secp256k1-native`,
  `.local/secp256k1-emscripten`, `.local/libdatachannel` and `.local/openssl`
  when it holds the library's header (below), else, natively outside Windows,
  in `/usr/include` or `/usr/local/include`, where a system install is
  legitimate; a relative `-s` is read from b2's working directory. When none
  holds a library's header, trystero is left out, its programs skipped, with
  one line that names the library and every place it looked, so that `b2 test`
  builds every other library, and the index names trystero without a link to
  its page; with `webcpp-require-external=on` on b2's command line, as the CI
  and the lint build, the build stops instead, naming both libraries
  (`webcpp.external` and its kin, chapter 4). trystero's page says how each is
  built. The CI's actions build them so (chapter 4).

Each toolchain is installed by hand and configured in `user-config.jam`, until
webcpp bundles the toolchains (`CODING-GUIDELINES.md` chapter 16). b2 reads
`~/user-config.jam`, or the file `--user-config=<file>` names:

```
using clang ;      # or gcc, or msvc: the native toolset
using boost : 1.92 : <include>/opt/homebrew/opt/boost/include <library>/opt/homebrew/opt/boost/lib ;
local wasi-sdk = /path/to/wasi-sdk ;
modules.poke : WASI_SDK : $(wasi-sdk) ;
using clang : wasip2 : $(wasi-sdk)/bin/clang++
  : <cflags>--target=wasm32-wasip2 <cxxflags>--target=wasm32-wasip2
    <linkflags>--target=wasm32-wasip2
    <archiver>$(wasi-sdk)/bin/llvm-ar <ranlib>$(wasi-sdk)/bin/llvm-ranlib ;
using clang : wasip3 : $(wasi-sdk)/bin/clang++
  : <cflags>--target=wasm32-wasip3 <cxxflags>--target=wasm32-wasip3
    <linkflags>--target=wasm32-wasip3
    <archiver>$(wasi-sdk)/bin/llvm-ar <ranlib>$(wasi-sdk)/bin/llvm-ranlib ;
```

The lines from `local wasi-sdk` on are the regions `wasi-sdk`, `wasip2` and
`wasip3` of `tools/ci/wasi-sdk.jam`, which the CI writes a lane's toolset
from and wasi's page includes, so that neither drifts from the other. The
region `wasi-sdk` also gives the build wasi-sdk's directory as `WASI_SDK`,
the one a component that a script builds (`webcpp.serve-script`) is built
with.

**The tools of a component** are looked up only when b2 generates a world's
bindings (on `clang-wasip2` or `clang-wasip3`, or for the explicit
`<bindings>-headers` of a reference) and when a served test runs, so a
native `b2 test` needs none of them. When one is needed and not found, the
build stops, naming it and every place it looked:

- wit-bindgen: `-sWIT_BINDGEN=<path>`, else `.local/wit-bindgen/wit-bindgen`,
  else `wit-bindgen` on `PATH`;
- the WIT of each version: `-sWASI_WIT_P2=<dir>` and `-sWASI_WIT_P3=<dir>`,
  each the `wit/deps` directory of its crate, else `.local/wasi-wit/p2` and
  `.local/wasi-wit/p3`;
- wasmtime: `testing.launcher=wasmtime` runs the one on `PATH`; a served
  test's is `-sWASMTIME=<path>`, else `wasmtime` on `PATH`, looked up when
  the test runs, which fails naming it;
- wasi-sdk, for a component that a script builds: `-sWASI_SDK=<dir>`, which
  the region `wasi-sdk` above gives, else `.local/wasi-sdk`, a directory
  that holds `bin/clang++`.

A `user-config.jam` may set any of them as `-s` does, with
`modules.poke : WIT_BINDGEN : <path> ;`.

When Boost cannot be used, the build stops before it compiles anything and
prints the exact `using boost` line to add. The Jamroot reads the version of
the configured Boost from its `boost/version.hpp`, with `grep`, or with
`findstr` on Windows, where b2 runs a command with `cmd.exe`, and keys its
cached check on it; a Boost whose `#define BOOST_VERSION <number>` line it
cannot read stops the build too, naming the directory.

**Boost's sources.** A compiled Boost library that a webcpp library needs is
built from Boost's own sources, for every target, as Boost.JSON's definitions
are: `/webcpp//boost_url`, `/webcpp//boost_charconv`, `/webcpp//boost_locale`
and `/webcpp//boost_cobalt` (`tools/boost/Jamfile`). An installed Boost has none
(Homebrew's ships its own `libboost_url` and no `libs/url/src`), so the build
reads them from `-sBOOST_SOURCE=<dir>` on b2's command line (or `-s
BOOST_SOURCE=<dir>`), which wins, else `BOOST_SOURCE` as a user-config.jam's
`modules.poke : BOOST_SOURCE : <dir> ;` or the environment gives it, which b2
cannot tell apart (chapter 6), else `.local/boost-source`: a tree with
Boost's `boost/version.hpp` and the `libs/<library>/src` of each library,
which `bash tools/ci/actions/boost/install.sh source .local/boost-source`
fills from the archive the CI uses, checked against its SHA-256. It writes
last a marker, `.webcpp-boost-source`, with the version and the archive's
SHA-256, and replaces only a directory that holds one: any other, a user's, an
installed Boost's include directory or a whole Boost tree, is refused by name
before anything is downloaded, and a tree it wrote before the marker existed
is removed by hand once. When a target needs the tree, the build stops if it
is missing, naming both places, if its version is not the configured Boost's,
naming both versions, if the configured Boost's version is unknown, Boost
being on the compiler's default include path, naming the `using boost` line to
add, and if it lacks the library's sources; a build that needs none never
looks.

**Machine-local setup.** `.local/` is git-ignored and holds what one machine
needs: `.local/user-config.jam`, `.local/wasi-sdk/`, `.local/mrdocs/`,
`.local/wit-bindgen/` and `.local/wasi-wit/p2` and `p3`, `.local/emsdk` (an
install of emsdk, or a link to one) with Emscripten's cache in
`.local/emscripten-cache`, and trystero's libraries in
`.local/secp256k1-native`, `.local/secp256k1-emscripten`,
`.local/libdatachannel` and `.local/openssl` (`include/openssl` and the two
libraries of the system's OpenSSL 3, linked, as the CI's `openssl` action
lays them out), where the CI's actions install them too (chapter 4). The
build finds each of them there with no `-s`: `b2 -a doc`, whose reference
parses trystero's native backend, runs as it is. Another directory is named
with `-sSECP256K1_ROOT=<dir>` and its kin on b2's command line, never with
the environment, as for the emsdk below: a variable a shell keeps from
another day would replace `.local/` without a word. A tool the build looks up itself is
taken from the path its `-s` option gives, else from `.local/`, else from
`PATH`: MrDocs (`-sMRDOCS`), wit-bindgen and the WIT (above); wasi-sdk is
where the `using clang` lines of `user-config.jam` name it, and, for a
script's component, `-sWASI_SDK` (above). The emsdk is where the `using
emscripten` line of `user-config.jam` names its `em++`, for the emscripten
toolset, and, for a reference read for Emscripten's target,
`-sWEBCPP_EMSDK=<dir>` on b2's command line, else `.local/emsdk` (chapter 2):
not a `modules.poke` of `user-config.jam`, nor the environment's `EMSDK`.
`tools/lint/compile_commands.py` runs b2 with `.local/user-config.jam`, else
with the file `$WEBCPP_USER_CONFIG` names, else with b2's own search. The
tests of the build and of the tools read `.local/user-config.jam`, else the
file `$WEBCPP_USER_CONFIG` names, and stop with an error when neither exists
(`tools/test/harness.py`); each scratch copy links the checkout's
`.local/boost-source` when it has one, and a test that needs wit-bindgen and
the WIT links `.local/wit-bindgen`, `.local/wasi-wit` and `.local/wasi-sdk`,
or the directories `$WIT_BINDGEN_ROOT`, `$WASI_WIT_ROOT` and `$WASI_SDK` name,
into its scratch copy, one that needs Emscripten `.local/emsdk`, or
`$EMSDK_ROOT`, and one that needs MrDocs `.local/mrdocs`, or `$MRDOCS_ROOT`.
Emscripten writes its cache where `EM_CACHE` names, else inside the emsdk:
every b2 of a test gets the cache the shell's `EM_CACHE` names, as the CI's
emsdk action names the one it restores with its system libraries built, so
that no test builds them again; else one of its run, beside its scratch
copies. Either is named by its resolved path, since Emscripten 6.0.11 builds a
system library from a relative path that misses its sources through a link, as
macOS's `$TMPDIR` is one. Sharing it is safe: Emscripten builds a library that
is missing under a lock of the cache's own and looks again once it holds it,
and three test runs at once on one empty cache pass. Each b2 also gets
`EMCC_SKIP_SANITY_CHECK=1`: Emscripten checks its configuration when it first
meets a cache, which a run's own is in every run, and prints "Running sanity
checks" into the output of the b2 that configures the toolset, which a test
compares whole (`b2 -d0 declared-targets`); the check's one other use,
clearing a cache made by another toolchain, has nothing to clear in a cache of
the run's own, and the CI's was checked when the action warmed it. The doc
build finds MrDocs at `.local/mrdocs/bin/mrdocs`. b2 itself reads it only when
told: `b2 --user-config=.local/user-config.jam ...`.

### The build commands

| Command | What it does |
| --- | --- |
| `b2 test` | every library's tests, natively |
| `b2 example` | every library's examples: each is built, run with its `.input` as its standard input when it has one, and its output compared with its `.expected` |
| `b2 doc` | the index page, `doc/html/index.html`, and every library's page, `libs/<name>/doc/html/index.html` |
| `b2 doc -sWEBCPP_INDEX=site` | the same, with the index linking each page where the site serves it, `libs/<name>/` (chapter 3) |
| `b2 libs/<name>/test` | one library's tests; `libs/<name>/example` and `libs/<name>/doc` likewise |
| `b2 libs/<name>/test//<test>` | one test, while working on it (`scheduler`) |
| `b2 libs/<name>/doc//reference` | one library's API reference alone, MrDocs strict |
| `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/<name>/test libs/<name>/example` | the same for wasm32-wasip2; `clang-wasip3` for wasm32-wasip3; one toolset per command |
| `b2 toolset=emscripten libs/<name>/test libs/<name>/example` | the same for emscripten, whose programs b2's toolset runs with node itself: never with a `testing.launcher` (chapter 4) |
| `b2 install --prefix=<dir>` | copies every library's headers to `<dir>/include/webcpp/`, and its licence files, `LICENSE_1_0.txt` and a port's `LICENSE-<ORIGIN>.txt`, beside them in `<dir>/include/webcpp/<name>/` |
| `b2 declared-targets -d0` | prints each `<library> <target>` pair the libraries declare: the CI's lanes |
| `b2 declared-lanes -d0` | prints each `<library> <lane> <directory> <kind>` line of a library's own lanes, such as its oracle's, and `<library> <lane> <directory> <target> <kind>` once per target for a lane that names the targets it runs on; the kind is `original` for a lane that runs the original's language, else `programs` (`CODING-GUIDELINES.md` chapter 12 and chapter 4) |
| `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/<name>/test//<lane>` | an own lane on wasip2, such as wasi's served tests, `libs/wasi/test//http` and `libs/wasi/example//http` (chapter 4) |
| `b2 -sWIT_BINDGEN=<path> -sWASI_WIT_P2=<dir> -sWASI_WIT_P3=<dir> -sWASMTIME=<path> ...` | any of these with the tools of a component where the defaults do not find them (above) |
| `b2 libs/<name>/test/oracle//oracle` | a port's oracle lane: the original runs the cases and the twins, and the results are compared (`CODING-GUIDELINES.md` chapter 12) |
| `b2 libs/<name>/test/oracle//update-expected` | writes the original's results again, the only writer of an expected result (`CODING-GUIDELINES.md` chapter 12) |
| `b2 libs/<name>/test//generated-<name>` | a library's generated files checked against their generator: written into the build directory and compared, byte for byte (chapter 4) |
| `b2 libs/<name>/test//update-generated` | writes every generated file of that Jamfile again, the only writer of a generated file (chapter 4) |
| `b2 -a ...` | any of these from scratch; the only build that counts as evidence |
| `tools/lint/lint.sh --clang-format <wasi-sdk>/bin/clang-format --clang-tidy <wasi-sdk>/bin/clang-tidy` | the lint, of the superproject and every library (`CODING-GUIDELINES.md` chapter 9) |
| `python3 tools/<dir>/<name>_test.py` | a test of the build or of a tool (`CODING-GUIDELINES.md` chapter 1) |
| `python3 tools/ci/matrix.py plan`, `own-lanes`, `lane`, `own-lane`, `register`, `report` | the CI's lanes and own lanes, run the way the CI runs them (chapter 4) |
| `b2 site`, `b2 site-serve` | the whole site, the site's pages with every library's page and the report, into `bin/site/`, every link checked (`tools/site/site.jam`, `tools/ci/assemble.py`); `site-serve` then serves it on http://127.0.0.1:8765/; `-sWEBCPP_SITE=release -sWEBCPP_SITE_VERSION=<version>` builds the release channel (chapter 4) |
| `b2 check-release` | what a tag needs: every library's change has a line, the release notes are complete, and the version agrees everywhere (chapter 5) |
| `b2 whats-new` | writes the skeleton of a version's "What's new", from its release notes (chapter 5) |
| `b2 dist` | writes the version's reproducible archive and its checksum (chapter 5) |

## 2. The API reference

The rules of a Doc Comment, its format and what every public symbol needs,
are `CODING-GUIDELINES.md` chapter 7. This chapter is how the reference is
generated from them, and where it sits.

### Generation

The reference is generated by MrDocs 2026.9.29, always to AsciiDoc and as a
single page, from the library's Doc Comments. `webcpp.reference <name> ;` in
`doc/Jamfile` declares it; `tools/doc/reference.py` runs it:

- the input is a compilation database of one aggregate translation unit,
  which includes every public header (the same one the lint analyses), read
  with the include directories and defines of the library's target and of
  the requirements `webcpp.reference <name> : <requirements> * ;` gives,
  natively: a library whose headers build only for WASI gives those a native
  parse needs, the explicit target `<bindings>-headers` of
  `webcpp.wit-bindings`, which generates the bindings on any toolset, and the
  macro of one version;
- a library with a header that builds only on emscripten gives
  `/webcpp//emscripten-reference`, whose usage requirements give the
  reference the emsdk (`<webcpp-emsdk>`): the translation unit is then read
  for Emscripten's target, as the emsdk's `em++` compiles it with
  `-fwasm-exceptions`, with the words `em++ -fwasm-exceptions --cflags`
  prints (its target `wasm32-unknown-emscripten` and its sysroot in
  Emscripten's cache among them) less its `-mllvm` options, which a parse
  never reaches: `doc_comments.py` reads it with the emsdk's `clang++`, and
  MrDocs, which adds the include directories that a command's compiler
  reports when run alone, with no environment, reads a command whose
  compiler is `emscripten-clang++`, a script `reference.py` writes, which
  runs the emsdk's `clang++` with those words, so that the directories are
  Emscripten's (the emsdk's `clang++` alone names no target and reports
  none, and MrDocs would take the host compiler's). Emscripten's own headers
  hold only for wasm32's types: `<emscripten/wire.h>` declares a binding of
  `long` and one of `int64_t`, which a native parse on Linux x86-64, where
  `int64_t` is `long`, reads as one declared twice (the CI's red docs job of
  2026-10-10). Every header is read for that target, in the one translation
  unit: MrDocs merges the symbols of two translation units by an identifier
  that holds the types of a function's parameters, and the host's standard
  library and Emscripten's give one function two (trystero's reference read
  as a native and an emscripten translation unit had 555 sections for 461).
  Such a library also gives the target of its native backend, if it has
  one, which brings that backend's dependencies, and the defines its
  browser build has, which a requirement conditioned on the emscripten
  toolset does not give a reference built natively: `webcpp.reference
  trystero : <library>/webcpp/trystero//native
  <library>/webcpp//emscripten-reference <define>BOOST_ASIO_DISABLE_THREADS
  ;`. Read so, trystero's reference is the same on Linux and on macOS, byte
  for byte. The emsdk is `-sWEBCPP_EMSDK=<dir>`, read from b2's command line
  alone, else `.local/emsdk`, never the environment's `EMSDK`, which
  `emsdk_env.sh` exports and b2 would read as `-sEMSDK`; its
  `upstream/emscripten/emscripten-version.txt` must name 6.0.11, the version
  the emsdk action pins, and Emscripten's cache is where `EM_CACHE` names, as
  for any `em++` (chapter 1). A build that uses the target stops, naming both
  places, when its `em++` or its `clang++` is not there, and naming both
  versions on another. A header the reference cannot parse fails it, naming
  the header;
- headers that branch by that macro have each branch's Doc Comments checked:
  `webcpp.reference <name> : <requirements> * : <also-checked> * ;` gives
  the other version's requirements, with which `reference.py` runs again,
  MrDocs and `doc_comments.py` both, as a sibling of the reference that the
  target `reference` and the page build too, and whose output no page shows.
  wasi's doc Jamfile gives wasip2's bindings and macro, then wasip3's, so an
  undocumented macro or `detail` symbol of the wasip3 branch fails the doc
  build. The check is never a dependency of the reference: a dependency's
  usage requirements, the other version's bindings, would reach the
  reference's parse;
- the shared settings are `tools/doc/mrdocs.yml.in`: `generator: adoc`,
  `multipage: false`, `embedded: true`, `warn-as-error: true`,
  `auto-function-metadata: false`, `auto-relates: false`, every `warn-*` on,
  the symbols of `webcpp::<name>::**`, `detail` namespaces as
  implementation-defined, the macros `WEBCPP_<NAME>_*` except the include
  guards, every `WEBCPP_*` for config,
  `base-url: https://github.com/webcpporg/<name>/blob/main/` and
  `inherit-base-members: never` (below);
- `tools/doc/doc_comments.py` then checks with clang++ what MrDocs does not
  (below);
- the page includes the result with
  `include::{reference}[leveloffset=+1]`.

`auto-function-metadata` and `auto-relates` are off because, at their
defaults, they document a parameter with its type's brief and count a class
with related functions as documented, which hid 48 findings on the first
library measured. `inherit-base-members` is `never`, so a derived class lists
its own members alone and an interface's are in the interface's section:
MrDocs reads an override whose parameter is spelled through an alias as an
overload of its own, and at its default copies the interface's member into
the derived class under the interface's anchor, which Asciidoctor refuses as
a duplicate; no reference of xactor, xstate, wasi or pratt changed with it.
A link MrDocs writes inside another link's text,
`link:#a[handler<void(link:#b[bytes])>]`, which Asciidoctor would end at the
inner `]`, is written as its text, the outer link kept (`reference.py`).

**A library's `doc/mrdocs.yml` holds presentation keys only.** It may set
`sort-members`, `sort-members-by`, `sort-namespace-members-by`,
`sort-members-ctors-1st`, `sort-members-dtors-1st`,
`sort-members-assignment-1st`, `sort-members-conversion-last`,
`sort-members-relational-last`, `overloads`, `sfinae`,
`inherit-base-members`, `inherit-hidden-friends`, `legible-names` and
`show-enum-constants`. Any other key (what is documented, the input, the
output, a warning) is refused and fails the reference: every library is
documented as strictly as every other. xactor's sets `show-enum-constants:
true`.

### The commands

- **Allowed:** `@brief`, `@param`, `@tparam`, `@return`, `@returns`,
  `@throws`, `@pre`, `@post`, `@note`, `@see`, `@code`/`@endcode`,
  `@details`, `@par`, `@copydoc`, `@ref` (and the same with `\`).
- **Forbidden,** because MrDocs drops them without a word: `@sa`,
  `@deprecated`, `@since`, `@todo`, `@retval`, and every other command. The
  lint names each.
- **A literal `@` in prose** is written `\@`; a bare `@` that starts no
  allowed command fails the lint.
- **No colon after `@ref x`.** MrDocs drops the colon: write
  `the result of @ref status_of, which`, never `@ref status_of: it`. The lint
  rejects it. A possessive is fine (`@ref scheduler's queue`); MrDocs renders
  it.
- **`@see "<title>"`** sends the reader to a section of the library's page by
  its title, exactly as the heading writes it after its `=` marks, in
  quotes, and on one line: `@see "Writing an actor logic", in the guide.` The
  doc build fails on a title that names no section of the page, or that
  wraps onto a second line, since MrDocs writes the title as plain text that
  no link checks. A section renamed changes its `@see` lines in the same
  commit.
- **A `(doc: #<anchor>)`** in a `//` comment of the library (code, test or
  example) sends its reader to an anchor of the page, such as a guarantee:
  `(doc: #xactor-invariant-13)`, or a list, `(doc: #a, #b and #c)`, or a
  range, `(doc: #a to #b)`. The doc build fails on an anchor that the page
  does not define. In a Doc Comment, use `@see` with the section's title.
- **Never link by hand to an overload's anchor:** MrDocs gives it a hash
  suffix that changes.
- **File-level comments do not reach the reference.** A header's opening
  `/** ... */` is for its reader; what the user needs is in the symbols'
  comments.

### What the doc build does after MrDocs

`postprocess.mjs` decodes the character references MrDocs writes for the
characters AsciiDoc would read as markup, and the U+2010 it writes for an
ASCII hyphen; `reference.py` drops the near-empty sections of the global
namespace and of `webcpp`, so the reference starts at `webcpp::<name>`, with
the library's macros, when it has any, in a section "Macros" of their own.

### Where the reference sits

It replaces any hand-written reference chapter. The numbered guarantees stay
in the guide part of the page, and a reference like `(doc: #anchor)` in a Doc
Comment becomes `@see "<title>"`.

## 3. Documentation

### The page

Each library has one AsciiDoc page, `doc/<name>.adoc`, with its sections in
files of their own beside it, built by `doc/Jamfile`:

```
import webcpp ;

webcpp.doc <name> : <name>.adoc ;
webcpp.reference <name> ;
```

Either rule declared in another directory stops the build, naming the rule
and the directory, and so does a page whose Jamfile declares no reference,
or a reference whose Jamfile declares no page.
`b2 libs/<name>/doc` converts the page with Asciidoctor.js into
`libs/<name>/doc/html/index.html`, with the reference included. The page
sets its own title and attributes, as xactor's does:

```
= <name>: <what it is> for {cpp}
:toc: left
:toclevels: 1
:idprefix:
:sectanchors:
:example-caption!:
:attribute-missing: warn
```

The doc build provides `{examples}` (the library's `example/` directory),
`{reference}`, `{twins}` (the twin directory, for a library whose oracle
declares twins, `CODING-GUIDELINES.md` chapter 12), `{webcpp-root}` (the
superproject, whose tracked files a page includes by tag, below),
`{library-root}` (the library's own directory, whose tracked files it includes
the same way), the counts and the links to other pages (below), and sets the
highlighter, the shared style (`tools/doc/docinfo.html`, the system's fonts,
no web fonts), no date and no footer: a page sets none of these. In that style
a table too wide for the page scrolls in its own box, and an identifier breaks
only between its parts.

**What a page holds:**

- **the guide,** taught the way the original's documentation teaches, concept
  by concept, with C++ only in listings: each concept's purpose, its smallest
  example, its variations and its rules. Each section has an anchor that
  starts with the library's name, `[#xactor-testing]`;
- **the guarantees,** numbered, each with an anchor named after its number,
  `. [[xactor-invariant-1]]The messages of one mailbox are handled in the
  order they were enqueued.` A guarantee keeps its number and its anchor
  forever, since code, tests and comments cite them;
- **a testing chapter:** how a program tests what it builds with the library,
  how the library itself is tested, on which targets, with which commands,
  and where the test matrix is (https://webcpporg.github.io/report/);
- **the reference,** `include::{reference}[leveloffset=+1]`, under a section
  `[#reference]`;
- **the appendix "Differences"** for a port (`CODING-GUIDELINES.md` chapter
  11);
- **the appendix Changelog,** `include::changelog.adoc[]`, last (chapter 5).

**Every listing is a compiled example, and every shown output comes from a
`.expected` file.** A listing includes a region of an example, marked in the
program by `// tag::<name>[]` and `// end::<name>[]` lines:

```
[source]
----
include::{examples}/xactor_quick_start.cpp[tag=messages,indent=0]
----
```

and the output it prints, which `b2 example` compares on every build:

```
[source]
----
include::{examples}/xactor_quick_start.expected[]
----
```

A listing of shell commands is `[source,bash]`; the only languages a page may
use are C++ (the default), JavaScript (a twin's code), JSON and bash. A
listing of anything else, a WIT world, a Jamfile's lines or a served test's
requests, is a `[listing]` block, which stays plain: a `----` block with no
style is C++, which must be an include of an example, or a tagged region of
a file of the library's `test/` or `example/`, which its Jamfiles compile,
`include::{library-root}/test/allocation_test.cpp[tag=bind_allocator]`.

A page shows the build's own configuration as it is, never a copy: it
includes a region of a file the superproject's git tracks,
`include::{webcpp-root}/tools/ci/wasi-sdk.jam[tag=wasip2]`, as wasi's page
does the lines that register the WASI toolsets, and a region of a file its
library's own git tracks for the library's own build, the dependencies its
`build.jam` declares or a test Jamfile's lines,
`include::{library-root}/build.jam[tag=dependencies]`.

The Node packages of the doc build, Asciidoctor.js and highlight.js, are
installed by `tools/node/install.py` with `npm ci`, once per lockfile under
`tools/doc/.node-modules/`, linked at `tools/doc/node_modules`, under a lock,
so that doc builds at once share one install; the install of an older
lockfile is removed once a build no longer may read it. The lint's Pyright
and an oracle's original are installed the same way.

### What doc-check enforces

`tools/doc/doc-check.py` runs before the page is converted, and again on the
rendered HTML; any finding, and any warning of Asciidoctor's, fails the
build:

- every `.adoc` under `doc/` is reached from the page's `include::` graph;
- every example's code or output is shown, every block's language is allowed,
  C++ is shown only as an include of an example or of a tagged region of the
  library's `test/` or `example/`, and the page includes the reference;
- every program an example Jamfile links and never runs (`webcpp.link`),
  which `doc.jam` gives as `--linked <program.cpp>`, is shown by an include of
  its code, and an `.expected` beside one is a fault ("<program> is linked,
  never run: it has no output to compare");
- every include names a file that exists, inside `doc/`, `{examples}` or
  `{twins}`; an output's include names a program that exists;
- an include of `{webcpp-root}/<path>` names a region, `tag=` or `tags=`, of
  a file the superproject's git tracks, which holds that region, and one of
  `{library-root}/<path>` a region of a file the library's own git tracks:
  an untracked file, a link git does not track, a missing file, a whole file
  and lines chosen by number, which drift, each fail, and so does a library
  directory with no git of its own; an attribute the page sets to either
  root is that root, checked the same way. Under `libs/<library>/`,
  `{webcpp-root}` names a file the library's own git tracks when
  `libs/<library>` is a repository of its own, as the release notes include
  a library's changelog, and one the superproject's git tracks when it is
  not;
- every `(doc: #anchor)` and `index.html#anchor` of the library's files
  names an anchor the page defines, and every `@see "<title>"` a section;
- every `(doc: <library>#<anchor>)` of the library's files, and every link
  of the rendered page into another library's page, names an anchor that
  page defines, as the build made it first (below);
- every C++ block of the library's README is opened by a comment
  `<!-- include::<file>[<attributes>] -->` and equals that region of the
  file, as an include would give it;
- no line inside a table starts with `//` (Asciidoctor drops it as a
  comment), and no `++` appears in prose (two of them swallow what lies
  between): write `{cpp}`;
- the rendered page has no cross-reference left as text, no stray `++` or
  backtick, no undecoded escape of MrDocs's, no U+2010, no em dash, which
  Asciidoctor writes for `--` even inside inline code (`xn\--bcher` keeps
  the two hyphens), none of Asciidoctor's other replacements inside inline
  code, an ellipsis, an arrow, (C), (R), (TM) or a curly apostrophe, which
  change the code (`+p->f(...)+` keeps it as written), no link inside
  inline code, which Asciidoctor makes of a
  URL there (`\http://a/?[x]` or `+http://a/?[x]+` keeps it as text), but
  for MrDocs's, the whole of the code one link, to a symbol's section or to
  the header where it is declared, no `<wbr>`, after which WebKit paints a
  part of a name twice at some widths (`postprocess.mjs` marks a break as an
  empty `<span class="wbr">`, whose zero-width space the style generates and
  no copy reads), and no link to the dropped `#index` or `#webcpp` sections.

### Counts

A page never types how many examples, tests, headers or twins its library
has: `webcpp.doc` gives it attributes that `tools/doc/counts.py` computes at
every build, so that no count drifts from the tree:

- from the programs b2 records as it loads the library's test and example
  Jamfiles: `{n-examples}` and `{n-examples-<target>}`, `{n-tests}` and
  `{n-tests-<target>}` (each header compiled alone is one test, and so is
  each served component, each linked program and each driven test),
  `{n-served}` and `{n-served-<target>}`, the served components alone,
  `{n-linked}` and `{n-linked-<target>}`, the programs `webcpp.link` links
  and never runs, `{n-driven}` and `{n-driven-<target>}`, the tests
  `webcpp.drive` drives, `{n-boost-test-suites}` and `{n-headers}`;
- for a library whose oracle declares twins, from `twins.py --list`:
  `{n-twins-agreeing}`, `{n-twins-divergent}`, `{n-examples-without-twin}`
  and `{n-examples-with-original}`;
- from the library's own `doc/counts.py`, when it has one, run with the
  library's directory as its one argument: each line `<name>=<number>` it
  prints, a count of what only that library holds (xstate's counts the cases
  of its fixtures, `{n-machine-cases}` among them).

A count that finds nothing fails the build rather than put a zero on the
page, and so does a name of a library's own that is also a generic one. A
count reads as English writes a number, with a thousands separator, `1,234`;
a library's own `counts.py` prints plain digits, `n-machine-steps=1234`.

### Links between pages

A page links another library's page as
`link:{webcpp-libs}/<library>/{webcpp-page}#<anchor>[...]`, with two
attributes `webcpp.doc` sets by the layout: where b2 builds the pages, by
default, or where the site serves them, with `-sWEBCPP_INDEX=site` (chapter
4). Such a link written any other way after `{webcpp-libs}/` is a fault. A
`//` comment of the library's files sends its reader to another library's page
with `(doc: <library>#<anchor>)`, or a list or a range of them, as with its
own page; a Doc Comment still uses `@see` (`CODING-GUIDELINES.md` chapter 7).
The build makes every linked page first, and the check of the rendered page
fails on an anchor that page does not define, and on an overload's anchor,
which ends in `-0<digit>` and which MrDocs may renumber.

### The index page

`doc/index.adoc`, built by `doc/Jamfile` (`webcpp.index index.adoc ;`),
introduces webcpp and includes, at `{libraries}`, the table that
`tools/doc/libraries.py` writes from every library's `meta/libraries.json`:
its name, linked to its page, its description, and what it ports, linked to
the original, or "original" for a library of webcpp's own (`"port-of":
null`). The pages are those the build declares: `tools/doc/doc.jam` loads
every library's `doc/Jamfile` and gives `libraries.py` the page each
declares with `webcpp.doc`, so that no Jamfile's text is read a second time.
A library that declares none, or whose page is not there, fails the table,
named. `b2 doc` builds it with every library's page.

The table links a page as `{library-pages}<name>/{library-page}`, two
attributes `tools/doc/doc.jam` converts the index with. By default they point
where b2 builds the page, `../../libs/<name>/doc/html/index.html` from
`doc/html/`; with `-sWEBCPP_INDEX=site` they point where the site serves it,
`libs/<name>/`, beside the index at the site's root (chapter 4). Any other
value stops the build.

**One doc build at a time.** A doc build writes `doc/html/` and
`libs/<name>/doc/html/` inside the tree, so two concurrent doc builds would
write the same files.

The index also shows each version's release notes, `doc/releases/`, newest
first (chapter 5). The notes name a library only once it is a submodule,
since the site's link check refuses a page that is not there.

## 4. Tests, lanes, the report, the site and CI

### Targets

A target is what a program is built for:

| Target | Toolset | Runs with |
| --- | --- | --- |
| `native` | any toolset not below: gcc, clang, msvc, darwin | the host |
| `emscripten` | b2's `emscripten`, wasm32, single-threaded, static | node, run by the toolset itself; no `testing.launcher` |
| `wasip2` | `clang-wasip2`, a clang registered against wasi-sdk with version `wasip2` | `testing.launcher=wasmtime`; a component, `wasmtime serve` (below) |
| `wasip3` | `clang-wasip3`, likewise | `testing.launcher=wasmtime`; a component, `wasmtime serve` (below) |

A Jamfile declares its default with `webcpp.targets`, a program can override
it with its own targets, and with no declaration a program is built for
`native` only. For any other target, the program gets `<build>no` and b2 skips
it without a word. This filter is conditioned on `<toolset>` and its version
only, never on a derived feature (chapter 6).

On emscripten the Jamroot builds wasm32, single-threaded and static (its
region `emscripten-target`): browsers do not all run memory64, and an
`address-model=64` request would add `-sMEMORY64=1`; pthreads need
cross-origin isolation in a page; and Emscripten's shared libraries are
experimental. b2's toolset runs a program's JavaScript with the node it
found, so a build that also gives `testing.launcher` stops ("webcpp: b2's
emscripten toolset runs a program with node itself; give no
testing.launcher on emscripten", chapter 6). Every program that node runs
(`webcpp.run`, `webcpp.run-fail`, `webcpp.example`) links
`-sNODERAWFS=1`, so that it reads the host's files and its standard input:
without it, node gives it an empty file system of Emscripten's own, and a
test reads its input file as empty. No other program gets it.

### The Jamfile API (`tools/webcpp.jam`)

A library's test and example Jamfiles declare their programs only with these
rules; the lint rejects b2's own `run`, `run-fail`, `compile`,
`compile-fail`, `exe`, `unit-test`, `link` and `link-fail` there, by file and
line.

```
import webcpp ;
webcpp.targets native wasip2 wasip3 ;
webcpp.run          <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.run-fail     <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.compile      <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.compile-fail <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.compile-diagnostic <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.boost-test   <name> : <sources> + : <requirements> * ;
webcpp.example      <source> : <requirements> * : <targets> * ;
webcpp.headers-alone <library> : <include-root> : <only> * : <requirements> * : <targets> * ;
webcpp.link         <name> : <sources> + : <requirements> * : <targets> * ;
webcpp.data         <directory> + ;   (requirements)
```

| Rule | Passes when | Notes |
| --- | --- | --- |
| `webcpp.targets t ...` | | the Jamfile's default targets; before its first program, once; each is `native`, `emscripten`, `wasip2` or `wasip3`, or the build stops naming it |
| `webcpp.run` | the program exits with 0 | built once per target it declares; a user's `exception-handling=off` links `tools/throw_exception.cpp`; on emscripten, run by node, linked with `-sNODERAWFS=1` |
| `webcpp.run-fail` | the program exits with another status | |
| `webcpp.compile` | the sources compile | no program is linked |
| `webcpp.compile-fail` | the sources do not compile | left out of clang-tidy; any error passes it, the wrong one included |
| `webcpp.compile-diagnostic` | the sources stop with every error they state in clang's `-verify` comments, `// expected-error@<file>:* {{<message>}}` for a header they include or `// expected-error@+1 {{<message>}}` for the next line | compiled with `-Xclang -verify -Xclang -verify-ignore-unexpected=error,note`, so an error not stated and a note are ignored; built on b2's `clang` toolset only (native clang, `clang-wasip2`, `clang-wasip3`) and skipped elsewhere, since `-verify` is clang's: a refusal would turn every gcc and msvc lane red for a declaration that holds; left out of clang-tidy |
| `webcpp.boost-test` | every case of the Boost.Test suite passes | native only, whatever the Jamfile declares; the header-only framework, `tools/boost_test_runner.cpp`, is one object of the suite's, always compiled with exceptions |
| `webcpp.example` | the program exits with 0, and its standard output, carriage returns removed, equals `<stem>.expected` beside it | its standard input is `<stem>.input` beside it, else empty, never the terminal, on every target (wasmtime passes it through, and node under `-sNODERAWFS=1`); run through `testing.launcher` for WASI, and on emscripten with the node on `PATH`, not the one b2's toolset registered, which b2 gives only to its own testing rules, by `tools/example/run_example.py`, which names a failing exit status (or the signal) before the diff; always run again, and `<stem>.input` is a source of the run |
| `webcpp.link` | the program links | never run: a program for a browser, which node refuses to run (`-sENVIRONMENT=web`), or one that needs the network; b2's `link` test, which `--dump-tests` lists and the report shows as a link test; never linked with `-sNODERAWFS=1`. The explicit target `<name>-program` is the same program, an `exe` of the same sources and requirements, `<name>-program.js` on emscripten, for a target that needs its files, such as a page that loads it. An example Jamfile's is shown on the page by its code, and has no `.expected` (chapter 3) |
| `webcpp.headers-alone` | each public header compiles alone | one test per header, `alone-<path>` with `/` as `-` (`alone-xactor-scheduler`), against `/webcpp/<library>//<library>` and the requirements given, for the targets given or the Jamfile's; `only` restricts a call to the public headers its globs match, relative to `<include-root>/webcpp/` (`wasi/http/response.hpp`), a glob that matches none stopping the build; a library may call it several times, and a header two calls take stops the build, naming it |
| `webcpp.data` | | requirements, or usage requirements, that give a program of `webcpp.run` or `webcpp.run-fail` the absolute directories, each read at the same path on every target: on wasip2 and wasip3 wasmtime preopens it (`wasmtime --dir="<directory>" <program>`); natively and on emscripten (`-sNODERAWFS=1`) the program reads the host's files already. A relative directory stops the build, as does one whose path holds `"`, `$` or a backquote, which the shell would read inside `--dir="..."`, naming the character |

**WebAssembly components** (`tools/component/component.jam`). A library's
`build.jam` declares the C bindings of a world, and its test and example
Jamfiles the HTTP components that wasmtime serves:

```
webcpp.wit-bindings <name> : <world-file> : <world> : <rename> : <version> : <wit-bindgen-argument> * ;
webcpp.serve        <source> : <requirements> * : <targets> * ;
webcpp.serve-script <name> : <script> : <stem> : <targets> * ;
```

| Rule | Passes when | Notes |
| --- | --- | --- |
| `webcpp.wit-bindings` | | the target `<name>`, the C bindings wit-bindgen generates for the world `<world>` of `<world-file>`, renamed `<rename>` (`--rename-world`), for the WASI version `<version>`, `p2` or `p3`, with the arguments given: on that version's toolset alone (`clang-wasip2`, `clang-wasip3`) its usage requirements put their directory on the include path and link `<rename>.c` and `<rename>_component_type.o`, and elsewhere they add nothing, so a native build needs no wit-bindgen. The explicit target `<name>-headers` puts the directory on the include path on any toolset, for a native parse such as the reference (chapter 2). The bindings are generated while b2 computes a target's properties, in a dry run too, under `<build-dir>/generated/<library>/<name>/`, and again only when the world file, the WIT, wit-bindgen's version or an argument changed, compared by content. A name declared twice in a library stops the build, naming both |
| `webcpp.serve` | the transcript of the answers to the requests of `<stem>.requests` equals `<stem>.expected` | builds `<source>` as a reactor component that exports an HTTP handler, for its own targets, which may be only wasip2 and wasip3, else for those of its Jamfile that are, linking `tools/throw_exception.cpp` in a user's build without exceptions; `tools/component/serve.py` serves it with `wasmtime serve` (`-S cli` on wasip2, `-S cli,p3 -W component-model-async` on wasip3) on a port the system chooses, sends each request on a connection of its own, and stops wasmtime in every outcome, a SIGKILL of serve.py included, as b2 sends to an action that outlasts its `-l`: wasmtime runs under a keeper that kills it when serve.py's pipe to it ends. A test of b2's, which `--dump-tests` lists and `--out-xml` records, so the report sees it fail (as a run). It runs in an own lane, never in the ordinary ones (Lanes, below) |
| `webcpp.serve-script` | as `webcpp.serve`, with the requests and the transcript of `<stem>` | the component is built by the shell script `<script>`, run as `sh <script> <p2\|p3> <component>` with `WASI_SDK` (wasi-sdk's directory, chapter 1), `WIT_BINDGEN` and `WASI_WIT` (the WIT of the lane's version) in its environment, the tools b2 found: a build by hand that a page shows, run as written. It runs at every build |

`<stem>.requests` holds one request per line, `<METHOD> <target>`.
The transcript holds, per request, the curl command that sends it after
`$ `, which a shell runs as written (`curl -I` for HEAD, `--request-target`
for a target that is not a path, such as the `*` of OPTIONS); the status
line, `HTTP/1.1 <status> <reason>`; the response's headers in lower case,
sorted, without the two wasmtime adds to every response, `date` and
`transfer-encoding`; an empty line; and the body. An empty line separates
two requests. The origin is written `http://localhost:8080` whatever port
was served, so one transcript holds on every machine and lane:

```
$ curl -i -X GET 'http://localhost:8080/a?b=c%20d'
HTTP/1.1 200 OK
content-type: text/plain; method=GET

GET /a?b=c%20d
```

**Driven tests** (`tools/drive/drive.jam`). A test that only a script can
drive, such as one of a peer that a browser or another process meets, is
declared in a test or example Jamfile that says first how a program of the
original's language runs, with `webcpp.original` (`CODING-GUIDELINES.md`
chapter 12), beside the `package.json` and `package-lock.json` that pin what
the script needs:

```
webcpp.drive <name> : <sources> + : <requirements> * : <script> : <argument> * : <targets> * ;
```

| Rule | Passes when | Notes |
| --- | --- | --- |
| `webcpp.drive` | `<original words> <script> <arguments> <program>` exits with 0 within its bound | builds the program from the sources, for its own targets, else its Jamfile's, as `webcpp.serve` builds its component, and runs the script beside the Jamfile with the words of `webcpp.original`, the arguments and the path of the linked program: its JavaScript on emscripten, its executable natively. `tools/drive/drive.py` runs it under `tools/component/serve.py`'s keeper, the leader of a process group of its own, bounded by 30 s, or the seconds the requirement `<webcpp-drive-timeout>` gives (`<webcpp-drive-timeout>60`): a driver that has not ended by then, because it or its program hangs, is stopped with every process of its group, and the test fails naming it, the program and the bound. Its group is killed once the driver ends too, and when `drive.py` itself is killed, so that nothing the driver started outlives the test. It depends on the Jamfile's `node-modules`, and a Jamfile without `webcpp.original` stops at it by name. b2's test `DRIVE`, which `--dump-tests` lists and `--out-xml` records, so the report sees it fail (as a run). Always run again. A lane-only program: it runs in an own lane, never in the ordinary ones (Lanes, below) |

The fixture browser_demo's `test/driver/Jamfile`, whose `drive.mjs` runs the
program, with node when it is JavaScript, and passes when it printed
`ready`:

```
import webcpp ;

webcpp.targets native emscripten ;

webcpp.original node ;
webcpp.drive driven : driven.cpp : : drive.mjs : --program ;
webcpp.lane driver : driven : native emscripten ;
```

**Libraries webcpp does not build** (`tools/webcpp.jam`), until it builds them
from `third_party/` (`CODING-GUIDELINES.md` chapter 16). A library's
`build.jam` declares each, and checks them, with three rules, which share one
computation of each candidate:

```
webcpp.external <owner> : <library> : <variable> : <directory> : <header> : <target> ;
webcpp.external-found <owner> : <library> + : <properties> * ;
webcpp.left-out <owner> ;
```

| Rule | What it gives |
| --- | --- |
| `webcpp.external` | declares that `<owner>`, a library of `libs/`, needs `<library>` on `<target>` (`native` or `emscripten`), found by its `<header>`, and returns the directory its lib targets search: `-s<variable>=<dir>` on b2's command line, the last one, a relative one made absolute against b2's working directory, never the environment; else `.local/<directory>` of the superproject when that holds `include/<header>`; else nothing, so that the compiler finds it on the system |
| `webcpp.external-found` | for the `<conditional>` of the owner's target: nothing when each library it names, on the target's target, is there (the `-s`; else `.local/<directory>`; else, natively outside Windows, `/usr/include` or `/usr/local/include`, where a system install is legitimate); a `-s` that names a directory without the header stops the build, naming it; when no candidate holds it, `<build>no`, so that the owner's targets are skipped, and one line, once per owner and library, `webcpp: <owner> is left out: <library> was not found: ...`, naming every place it looked and how to give it, so that every other library builds; with `webcpp-require-external=on` (an incidental, propagated feature), as the CI and the lint build, the build stops instead, naming the owner and the library |
| `webcpp.left-out` | why the owner's page is left out of a native build, `<library> was not found`, or nothing: the index's table then names the library with that reason and no link, so that it never links a page the build did not make |

`tools/test/external_test.py` pins them: the command line, a relative one
too, and `.local/` found, the environment never read, a wrong `-s`
refused, a missing library left out once while another library builds, the
index's row without a link, and the stop under
`webcpp-require-external=on`; `tools/lint/lint_test.py` that the lint's dry
runs require them and name the two libraries.

**Generated files** (`tools/generated/generated.jam`). A file that a script
writes from inputs it pins, such as a table of Unicode data, is committed and
written only by its generator. A library's test Jamfile declares it:

```
webcpp.generated <name> : <script> : <argument> * : <output> + ;
```

| Rule | Passes when | Notes |
| --- | --- | --- |
| `webcpp.generated` | every output that `<script>` writes equals its committed `<output>`, byte for byte | the explicit target `generated-<name>`: `tools/generated/generated.py` runs the Python script `<script>` in the Jamfile's directory as `<script> <argument>... <path>...`, each path where it writes one output, in the order given, under the Jamfile's build directory, and compares each with its committed file (`tools/oracle/compare.py --files`), naming every one that differs, with the command that writes it again, `b2 libs/<name>/test//update-generated`, every one that is missing and every one the generator did not write; it writes nothing into the tree, passing or failing. The first declaration of a Jamfile also declares the explicit target `update-generated`, which runs each of its generators the same way, in a build directory apart from the check's, so that one request may build both, and, once all have succeeded, copies their outputs beside the committed files and renames each over its own: a generator or a copy that fails leaves every committed file as it was, and a rename that fails is named with the files it replaced and those it left. Paths are relative to the Jamfile; a script that is not a `.py` file or does not exist, a name declared twice, and an output named twice, by one generator or by two of the Jamfile, stop the build. An own lane names the check, `webcpp.lane <lane> : generated-<name> ;`, which runs natively, as an oracle's does, and is of the kind `programs`, since a generator runs Python, not the original's language; the ordinary lanes never build it, since a generator may need the network. Neither target compiles anything, so they need no Boost: they leave out the Jamroot's check of Boost, and the libraries their project's requirements name, whose targets would run it |

The rules of the doc Jamfiles are in chapter 3: `webcpp.doc <library> :
<page>.adoc ;`, `webcpp.reference <library> : <requirements> * :
<also-checked> * ;` (chapter 2) and, for the superproject's index,
`webcpp.index <page>.adoc ;`. Those of an oracle's Jamfile,
`libs/<name>/test/oracle/Jamfile`, are in `CODING-GUIDELINES.md` chapter 12:
`webcpp.original`, `webcpp.twins`, `webcpp.cases` and `webcpp.lane`, which any
test or example Jamfile of a library may also declare, with the targets its
lane runs on (Lanes, below).

A library's Jamfiles, as xactor's, each whole after its licence notice.
`libs/xactor/test/Jamfile`:

```
project : requirements <library>/webcpp/xactor//xactor ;

import webcpp ;

webcpp.targets native wasip2 wasip3 ;

webcpp.headers-alone xactor : ../include ;

# xactor's guarantees, one program per file. scheduler checks the Asio
# driver where drivers.hpp declares it, outside WASI.
webcpp.run scheduler : scheduler_test.cpp ;
webcpp.run lifecycle : lifecycle_test.cpp ;
webcpp.run fuel : fuel_test.cpp ;
webcpp.run create_actor : create_actor_test.cpp ;
```

`libs/xactor/example/Jamfile`:

```
project : requirements <library>/webcpp/xactor//xactor ;

import webcpp ;

webcpp.targets native wasip2 wasip3 ;

webcpp.example xactor_quick_start.cpp ;
webcpp.example xactor_lifecycle.cpp ;
webcpp.example xactor_fuel.cpp ;
webcpp.example xactor_timers.cpp ;
webcpp.example xactor_drivers.cpp ;
# A test of actors, with lightweight_test: it prints nothing on its standard
# output, and returns boost::report_errors().
webcpp.example xactor_testing.cpp ;
# A static actor, whose behaviour is a Boost.MSM state machine.
webcpp.example xactor_msm.cpp ;
# Native only: Boost.Asio 1.92 does not compile for wasm32-wasip2 or
# wasm32-wasip3 with wasi-sdk 34 (no ESHUTDOWN, no ::pause, and a signal.h
# that stops with #error), so drivers.hpp declares asio_driver only outside
# WASI.
webcpp.example xactor_asio.cpp : : native ;
```

### Tests

How a test is written, its framework, its checks and the targets it runs on,
is `CODING-GUIDELINES.md` chapter 8.

### Lanes

A lane builds and runs one library's tests and examples for one toolset, from
scratch, and records what it built and ran:

```
b2 -a --dump-tests --out-xml=<lane>.xml toolset=<toolset> libs/<library>/test libs/<library>/example
```

A wasip2 or wasip3 lane adds `testing.launcher=wasmtime`, which applies to
every toolset of one b2 request, so a lane is always one toolset. An
emscripten lane adds nothing: b2's toolset runs its programs with node. The lane
runs from the superproject's root. `--dump-tests` is required: it lists every
test, those the lane skips included, and the report refuses a file without
it. With `--out-xml`, b2 exits 0 even when a test fails, so b2's status is
never the verdict.

**Lane names.** A lane is named after its target (`native`, `emscripten`,
`wasip2`, `wasip3`) or after the directory b2 builds its toolset in
(`gcc-14`, `gcc-15`, `clang-linux-18`, `clang-darwin-21`, `msvc-14.3`). The
CI names a native lane after its directory, and a wasm or emscripten lane
after its target. An own lane on a target (below) is named
`<target>.<library>.<lane>`, whichever directories declare it:
`wasip2.wasi.http`. No toolset directory begins with a target and a dot, so
that name is never a lane's, and its column stands beside the target's own. The report checks
every name against the toolset directory the file records: a lane named
after a target must be built for it, an own lane must be built for the
target its name begins with and list the tests of the library it names
alone, and any other name must be that directory, or the report exits 2,
naming the lane, the directory and the names it may take. A lane that built
nothing records no directory; its name is checked against the toolset its
command line names when it begins with a target, and it fails as empty.

**Lanes in parallel.** Independent lanes run at the same time, each with its
own build directory, and are read once all have finished:

```
b2 -a --dump-tests --build-dir=bin/lane-wasip2 --out-xml=wasip2.xml toolset=clang-wasip2 testing.launcher=wasmtime libs/<library>/test libs/<library>/example
```

Two concurrent b2 runs never share a build directory, and at most one of
them builds the documentation (chapter 3).

**Own lanes.** A library may also declare lanes of its own with `webcpp.lane`
(`CODING-GUIDELINES.md` chapter 12). What a lane names leaves the library's
ordinary lanes, which build only its test and example directories' other
programs. `b2 declared-lanes -d0` lists them, one line each, which ends with
the lane's kind: `original` when one of the targets it names runs the
original's language, the twins, cases, `node-modules` or `update-expected` of
an oracle's rules (`CODING-GUIDELINES.md` chapter 12) or a driven test
(`webcpp.drive`), whose CI job then installs Node, and `programs` when it runs
only what its toolset builds, such as wasi's served components, whose job
installs no Node of its own. A lane's kind is computed once every Jamfile is
loaded, so a lane may name a target its Jamfile declares after it, and it
reads only the names the lane gives itself: a lane over an alias of the
oracle's targets is of the kind `programs`, so a lane names them directly.

- **An own lane that names no target,** such as xstate's oracle lane, which
  needs Node where a toolset lane does not, is the line `<library> <lane>
  <directory> <kind>`, and runs as `b2 -a <directory>//<lane>`, whose exit
  status is its verdict. It writes no XML, and is no column of the report.
- **An own lane that names targets** is the line `<library> <lane>
  <directory> <target> <kind>` once per target, and runs on each as that
  target's lane runs, from scratch, with `--dump-tests` and `--out-xml`, in
  every directory of the library that declares a lane of that name on that
  target, in one b2 run: its tests reach the report, in a column of its own
  (above).
  wasi's served tests and served examples, the components wasmtime serves
  (`webcpp.serve`, `webcpp.serve-script`), are its lanes `http`, in `test/`
  and in `example/`, on wasip2 and wasip3, run as one own lane on each:

  ```
  webcpp.lane http
    : http_methods http_long_body http_no_content_type http_two_units
    : wasip2 wasip3 ;
  ```

  ```
  b2 -a --dump-tests --out-xml=wasip2.wasi.http.xml toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/example//http libs/wasi/test//http
  ```

  Built for a target it does not name, natively for one, such a lane stops,
  naming itself, its targets and the toolset to give (`webcpp.lane http in
  libs/wasi/test/Jamfile runs on wasip2 wasip3, and this build is for
  native: give toolset=clang-wasip2 or toolset=clang-wasip3, with
  testing.launcher=wasmtime`), rather than build nothing and read green.

  A lane-only program, served (`webcpp.serve`, `webcpp.serve-script`) or
  driven (`webcpp.drive`), always runs so: `b2 declared-lanes` stops, naming
  it, when no lane of its Jamfile names it (`webcpp.drive driven in
  libs/browser_demo/test/driver/Jamfile is in no own lane; name it in a
  webcpp.lane that names its targets`), or when its lanes leave out a target
  it is built for, so the CI's plan fails before any job runs rather than
  leave it untested.

  A lane on emscripten is declared like any other, and runs as the
  emscripten lane runs, `toolset=emscripten` with no launcher, never natively
  in its place: browser_demo's driven test is its lane `driver`, on native and
  emscripten, `emscripten.browser_demo.driver` there.

### The report (`tools/report/`)

```
python3 tools/report/report.py --lane <lane>=<lane>.xml [--lane ...] --out <dir>
```

writes the test matrix into `<dir>`: `index.html`, the libraries by lanes;
`<library>.html`, a library's tests and examples by lanes; and
`output/<lane>/...`, the log of each failure, linked from its cell. A cell is
green when every build of it passed, red with the kind of its most
significant failure (compile, compiled, link, linked, build, run, ran, not
run), and grey (n/a) when the lane did not build it. The directory is served
as the site's `report/` (below): each page's `webcpp` links the site's index,
`../`, a library's page links its documentation, `../libs/<library>/`, and the
footer links `github.com/webcpporg`.

**Its exit status is the verdict:** 0 when every lane built something (a test
or an example: an example counts as something that ran) of every library it
lists, and everything passed; 1 when a test or an example failed, an action
outside every test failed, a lane built nothing, or a lane that built
something built none of the tests and examples of a library it lists, each
named (`wasip2: nativeonly: the lane built none of its tests and examples`).
The CI puts in a lane only the libraries that declare its target, so such a
library is a fault that a grey column would hide, as when a lane run by hand
names a library that does not declare its target. It exits 2, with nothing
written, when a file cannot be read, a lane spans more than one toolset, a
lane named after a target was built for another, a lane named after no target
is not named after its toolset directory, a test lies outside `libs/<name>/`,
or a library is named `index`.

### The site (`tools/site/`, `site/`)

```
b2 site
b2 site -sWEBCPP_SITE=release -sWEBCPP_SITE_VERSION=<version>
b2 site-serve
```

builds webcpp's whole site into `bin/site/`: the site's own pages, the index
page and every library's page, the report, and the files that index them,
every link checked and every page valid HTML. `b2 site-serve` builds it and
serves it at http://127.0.0.1:8765/. The site is built for one of two
channels:

| Channel | Built with | Served at | Indexed |
| --- | --- | --- | --- |
| preview | `b2 site`, the default | https://webcpporg.github.io/, from every push to `main` | no: it is the development version, and every page carries `<meta name="robots" content="noindex">`; it writes no `sitemap.xml`, which its `robots.txt` does not name |
| release | `-sWEBCPP_SITE=release -sWEBCPP_SITE_VERSION=<version>` | https://webcpp.org/, from a release tag | yes: every page says `index, follow`, and `robots.txt` names `sitemap.xml` |

A preview page's band names the commit it was built from and its date, a
release page's the version. The options:

- `-sWEBCPP_SITE=<channel>`, `preview` or `release`; any other value stops
  the build, named. The release needs `-sWEBCPP_SITE_VERSION=<version>`, such
  as `1.0.0` or `1.0.0-rc.1`, and the preview takes none.
- `-sWEBCPP_SITE_DOCS=<dir>` names the pages `b2 doc -sWEBCPP_INDEX=site`
  built already, a superproject's root or a copy of its `doc/html` and
  `libs/<name>/doc/html`, as the CI's docs job hands them on; without it,
  `b2 site` builds them with `b2 doc`.
- `-sWEBCPP_SITE_REPORT=<dir>` names what `tools/report/report.py` wrote;
  without it, `report/` says that no results were given, as on any machine
  but the CI's.

The steps, each one of the site's npm scripts or a script of
`tools/`: `tools/site/catalogue.py` writes the catalogue, `bin/libraries.json`,
from every library's `meta/libraries.json` and the targets it declares; the
site's `build` script renders its pages with the channel, the commit, its
date, the version and the catalogue (`/webcpp/site//pages`);
`tools/ci/assemble.py` lays them out with the docs and the report into
`bin/site.tmp/`, heading each page of the docs and of the libraries with the
site's header, each library's page with the head the catalogue gives it, and
every page with noindex on the preview, then checks that every page carries
the robots of its channel; the `build` script writes the indexing files
(`site/tools/indexing.mjs`); `assemble.py --check` checks every link; the
build moves into `bin/site.builds/`, and `bin/site`, a link, is renamed onto
it in one step (the `swap` script), so that a server on `bin/site` reads one
build or the other whole and a failed step leaves the last good build; last,
`/webcpp/site//valid-html` runs html-validate over every page. `bin/site/`:

```
index.html          the home, from the site's pages/index.oui
libraries/, cpp/, javascript/, ai/, examples/, community/, search/
                    the site's other pages, each from its pages/<name>.oui
assets/             the styles, the logo, the icons and the client bundle, site.js
cards/              each page's and each library's social card, 1200 by 630
favicon.ico
docs/               the index page, built with -sWEBCPP_INDEX=site
libs/<name>/        each library's page
report/             the test matrix
robots.txt          every crawler allowed; on the release, the sitemap named
sitemap.xml         on the release alone: every page, with the date its source last changed
llms.txt            what webcpp is, and a line for each library and each page
libraries.json      the catalogue
```

**Where the site's source lives.** The site is a repository of its own,
`webcpporg/webcpporg.github.io`, checked out as the submodule `site/`, as a
library is under `libs/`: its pages in OpenUI Lang under `pages/`, the React
components they are written with and the renderer under `src/`, its node
tools under `tools/`, its tests under `test/`, its `Jamfile`, and its own
`AGENTS.md` and `README.md`. A change to it is committed in `site/` on its
`main`, then the superproject commits the submodule's new commit, as for a
library. What the superproject holds of the site is `tools/site/` and
`tools/ci/assemble.py`. Its tests are `b2 /webcpp/site//test`, the site's own
(its npm `test` and `typecheck`), and `tools/site/*_test.py` with
`tools/ci/assemble_test.py`, among the tools' tests.

### CI

The CI runs exactly the commands above. It is GitHub Actions, in two
workflows of the superproject, and every third-party action is pinned by its
full commit SHA, with its tag in a comment. It runs on every push to `main`
and on every pull request, of the superproject and of each library.

**A library's CI,** `libs/<name>/.github/workflows/ci.yml`, calls the
superproject's reusable workflow, `.github/workflows/library.yml@main`
(`on: workflow_call`, input `library`). **The superproject's CI,**
`.github/workflows/ci.yml`, calls the same workflow without a library, for
every library at the commit its submodule points to. Every job checks out
`webcpporg/webcpp` with its submodules, the caller's commit for the
superproject and `main` for a library, whose own commit then replaces
`libs/<library>`; no checkout keeps its credentials. A run on `main` is never
cancelled by the next; a pull request's newer push cancels its older run. The
jobs:

- **plan:** `python3 tools/ci/matrix.py plan [--library <name>]` runs
  `b2 declared-targets -d0` and prints the JSON matrix of lanes: one lane per
  compiler for each target a library declares, building the libraries that
  declare it. The CI never lists a library's targets by hand. Every target
  has a lane, and one that comes to have none fails the plan by name. Then
  `matrix.py own-lanes [--library <name>]` runs `b2 declared-lanes -d0` and
  prints the JSON matrix of the own lanes of that library, or of every
  library, one entry per library, lane and target: `{library, lane,
  directories}`, every directory that declares the lane there, for an own
  lane that names no target, and one per target for one that names targets,
  which adds `platform`, its target, and `id`, its name in the report; every
  entry has `kind`, the lines' (`original` when any of its directories' lanes
  runs the original), `name`, its job's, the `os`, `wasm`, `emsdk`, `cc` and
  `cxx` of the lane whose setup it shares, the target's own, and the oracle's
  Clang 18 for native and for none, `node`, true for a lane of the kind
  `original` and for one on emscripten, whose lane has Node, and `external`,
  as a lane's (below), which the job reads from it alone.
  Last, `matrix.py declares emscripten [--library <name>]` prints `true`
  when any library of the superproject declares emscripten, whatever library
  the CI runs for, else `false`: the plan's output `has-emscripten`, on
  which the docs and lint jobs install emsdk (the lint analyses every
  library, and a page builds the page of each library it links, with its
  reference). And `matrix.py external --library <name>` prints `true` when
  that library needs libraries webcpp does not build: it is in
  `EXTERNAL_DEPENDENCIES` (below), or a library of `libs/` that its
  `build.jam` or Jamfiles use (`/webcpp/<name>`), at any depth, is; else
  `false`: the plan's output `external`, on which the docs job installs
  them, since a page parses its library's headers and those it uses. Without
  `--library` it says whether any library of `libs/` needs them: the plan's
  output `lint-external`, on which the lint job, which analyses every
  library, installs them.
- **container:** when the plan has a container lane (its output
  `has-container`), the `container` action on ubuntu-24.04, which builds
  the container lanes' image when the cache does not hold it and saves it
  there, once per run; every lane's job waits for it (`needs: [plan,
  container]`), and runs when it succeeded or was skipped.
- **lanes,** one job each, which run `matrix.py lane <entry>`: it registers
  the lane's toolset in `.local/user-config.jam` with its version, prints the
  lane command and runs it, and the job uploads `<lane>.xml`:

  | Lane | Runner | Toolset |
  | --- | --- | --- |
  | `gcc-14` | ubuntu-24.04 | `gcc-14`, in a container with no network |
  | `gcc-15` | ubuntu-26.04 | `gcc-15` |
  | `clang-linux-18` | ubuntu-24.04 | `clang-18`, on libstdc++, in a container with no network |
  | `clang-linux-22` | ubuntu-26.04 | `clang-22`, on libstdc++ |
  | `clang-darwin-<version>` | macos-15 | Apple Clang, its version read from `clang++ -dumpversion` |
  | `msvc-14.3` | windows-2022 | Visual Studio 2022 |
  | `msvc-14.5` | windows-2025 | Visual Studio 2026 |
  | `emscripten` | ubuntu-24.04 | `emscripten`: Emscripten 6.0.11 from emsdk, its system libraries built in the cache the action restores, and Node 26.7.0 through the wrapper of the emsdk action (its entry's `node`) |
  | `wasip2`, `wasip3` | ubuntu-24.04 | `clang-wasip2`, `clang-wasip3`: wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0 and the WASI WIT, for every library |

  The Clang lanes on libstdc++ stay: a regression of xactor's guarantee 28 is
  caught only there. The MSVC lanes add `address-model=64
  embed-manifest-via=linker --abbreviate-paths`; b2 abbreviates each word of
  a toolset directory, and `msvc-14.3` and `msvc-14.5` are their own
  abbreviations, which `tools/ci/matrix_test.py` checks with b2's own rule.

  **The container lanes,** `gcc-14` and `clang-linux-18` (their entries'
  `container`), run the lane's b2 in a Linux container with no network but
  the loopback, so that a test that reaches beyond the machine fails there:
  every library's tests hold it, and trystero's guarantee 6 rests on it.
  The image is `tools/ci/container/Dockerfile`'s: Ubuntu 24.04, the
  runner's release, pinned by the digest of its image index, with `g++-14`,
  `clang-18`, OpenSSL's development files and Python, every package,
  `ca-certificates` included, from Ubuntu's snapshot of the archive at the
  date its `ARG SNAPSHOT` names (`Snapshot: yes` on the sources, `apt-get
  --snapshot`), so that one Dockerfile is one set of packages. The snapshot
  service is HTTPS only and the base image has no CA certificates, so apt
  reaches it once without verifying the server's certificate, to install
  `ca-certificates` from the snapshot, and verifies it from then on; apt
  checks the signed `InRelease` and every index and package against it
  whatever the transport. A `ca-certificates` from the live archive would
  pull `libssl3t64` forward, and the snapshot's `libssl-dev`, which depends
  on its own, would no longer install once Ubuntu updates OpenSSL (measured).
  Ubuntu keeps no snapshot of `ports.ubuntu.com`, so the image builds for
  x86-64 alone (measured: apt 2.8.3 refuses arm64's sources). It is tagged
  `webcpp-lane:<the first 16 hex digits of the Dockerfile's SHA-256>`. The
  `container` job, which every lane waits for, runs the `container` action
  once per run, which builds it when the cache does not hold it and saves it
  there under that digest; each container lane's job runs the action too,
  which then loads it, and `matrix.py lane` builds it only when no image of
  that tag is there. Then it runs b2 by its path, `docker run --rm --network
  none`, as the runner's user, with the superproject and b2's prefix, where
  the Boost action installs Boost's headers too, mounted at their own paths,
  and the variables of the external libraries passed on; the XML is the
  lane's, as outside a container. A lane run so needs Linux x86-64 and
  docker; the external libraries it runs against are built before, on the
  runner, with the same compilers.

  **External libraries.** A library may need libraries webcpp does not build
  yet: `EXTERNAL_DEPENDENCIES` in `tools/ci/matrix.py` names them, trystero's
  `secp256k1`, `libdatachannel` and `openssl`, each the CI's action of that
  name, until webcpp builds them from `third_party/` (`CODING-GUIDELINES.md`
  chapter 16). A lane that builds such a library, or one that uses it, has
  `external` in its entry, and so does an own lane of it; such a job runs the
  actions after the lane's own setup, by their YAML anchors in both jobs:
  `openssl`, then `secp256k1`, with `emscripten: true` on an entry that has
  emsdk, then `libdatachannel`, built with the lane's compilers, its entry's
  `cc` and `cxx` (`gcc-14` and `g++-14`; Visual Studio for MSVC, CMake's own
  choice there). An entry on emscripten has neither OpenSSL nor
  libdatachannel, which only the native backend needs. Its b2 command, the
  lane's or the own lane's, then has `webcpp-require-external=on`, so that a
  library not found stops it rather than leave the library out, and
  `-s<VARIABLE>=<dir>` for each variable the actions exported
  (`EXTERNAL_VARIABLES`), on the command line, where a library's `build.jam`
  reads them; the docs job gives its b2 the same when the plan's `external` is
  `true`. A container lane passes no variable of the environment into its
  container.
- **own lanes,** one job per entry, named and placed as the entry says
  (`name`, `os`), which runs `matrix.py own-lane <entry>`: it registers the
  lane's toolset, prints its b2 command and runs it, on the lane in each of
  its directories. A library that declares no own lane, as xactor, has no
  such job.
  - One that names no target, `Own lane (<library>, <lane>)`, on Linux x86-64
    (ubuntu-24.04): the Boost action, the Node action, and `b2 -a
    toolset=clang-18 <directory>//<lane> ...` (an oracle lane checks Boost,
    `CODING-GUIDELINES.md` chapter 12), whose exit status is the lane's
    verdict.
  - One that names a target, `Own lane (<library>, <lane>, <target>)`, on its
    entry's image: the Boost action and what the target's lane installs, the
    same steps by their YAML anchors (`&wasi-sdk`, `&wasmtime`,
    `&wit-bindgen`, `&wasi-wit`, `&emsdk`), so that a step added to a WASI or
    emscripten lane is added to it too, and the Node action; then `b2 -a
    --dump-tests --out-xml=<id>.xml toolset=<toolset> [<options>]
    <directory>//<lane> ...`, and it uploads `<id>.xml` as a lane uploads its
    XML, the artifact `lane-<id>`.
  - An own lane of the kind `original` has the CI's Node, its entry's
    `node` (`&node`): an oracle runs npm and its original with it, and a
    driven test its driver, natively too. Without it, a native one would run
    with the image's own Node, another version. One on emscripten has it as
    the emscripten lane does, which runs its programs with it; one of the
    kind `programs` on any other target, such as wasi's served lanes, has
    none.
  - An own lane whose driver meets a browser, one of `CHROME_LANES` in
    `tools/ci/matrix.py` (trystero's `interop`, natively and on emscripten),
    has its entry's `chrome`, and its job runs the `chrome` action, which
    gives it `CHROME`, a pinned Chrome rather than the image's, which moves
    with the image, so that a red interop lane is never a browser nobody
    chose.
- **docs:** with MrDocs on Linux x86-64 (it has no build for Linux arm64 or
  Intel macOS), `clang++-18`, Node, wit-bindgen and the WASI WIT (wasi's
  reference parses its bindings), `b2 -a libs/<library>/doc`, or for the
  superproject `b2 -a doc -sWEBCPP_INDEX=site`, whose pages are the site's,
  and, when the plan's `has-emscripten` is `true`, emsdk in `.local/emsdk`,
  with the sysroot in its cache and without its system libraries
  (`libraries: 'false'`), whose `em++` and `clang++` read for Emscripten's
  target the reference of a library with a header built for emscripten
  alone (`/webcpp//emscripten-reference`, chapter 2); and, when the plan's
  `external` is `true`, the
  external libraries, built with Clang 18, whose headers the reference of
  the library, or of a library it uses, parses.
- **lint:** `tools/lint/lint.sh` in eight shards (`--shard 1/8` to `8/8`),
  with wasi-sdk's clang-format and clang-tidy, Node, Clang 18 as b2's default
  toolset and the wasip2 and wasip3 toolsets after it (`matrix.py register
  clang-18 wasip2 wasip3`), wit-bindgen and the WASI WIT, which the WASI dry
  runs of the compilation database need; when the plan's `has-emscripten`
  is `true`, the emscripten toolset after them and emsdk, without its system
  libraries (`libraries: 'false'`), whose `em++` the emscripten dry run
  names and whose cache holds the sysroot an emscripten command is analysed
  with; when the plan's `lint-external` is `true`, the external libraries,
  built with Clang 18, libsecp256k1 for emscripten too when `has-emscripten` is;
  and the full history
  (`fetch-depth: 0`) of the superproject and of the library, since the
  banned-word rule reads every commit.
- **tools,** for the superproject only: every `tools/**/*_test.py`, in
  eight shards, with Clang 18 and the wasip2, wasip3 and emscripten
  toolsets, wasi-sdk, wasmtime, wit-bindgen, the WASI WIT, emsdk, Node and
  MrDocs, each failure named; the tests of the emscripten target share the
  cache the emsdk action restores (chapter 1). A suite run by
  `harness.run_cases` runs its slice of cases in every shard (`--shard
  K/N`, which a run by hand takes too); every other suite runs whole in one
  shard, in turn. A run without `--shard` that chooses no case fails, since
  it would pass having run nothing.
- **actionlint:** actionlint 1.7.12, downloaded and checked against its
  SHA-256, on every workflow of the superproject and of `libs/*`, with
  `.github/actionlint.yaml`. It also runs clean locally before a workflow
  change is committed.
- **report:** `matrix.py report --plan <matrix> --own-lanes <matrix>`
  merges the XML of every lane and of every own lane on a target with
  `tools/report/report.py` into the test matrix, uploaded as an artifact. Its
  exit status is the verdict; a planned lane or own lane on a target that
  wrote no XML fails it by name, and so does a lane's job that failed, or an
  own lane's.

`matrix.py lane` and `matrix.py own-lane` take, after `--`, more arguments
for b2, so that a lane is run locally exactly as the CI runs it, beside
others: `python3 tools/ci/matrix.py lane '<entry>' --
--build-dir=bin/lane-gcc-15`. Each registers the lane's toolset in
`.local/user-config.jam`, wasi-sdk's directory, the emsdk's and the node
wrapper each quoted as one word of Jam.

**The actions,** `tools/ci/actions/`, each a script beside its `action.yml`:

- `boost` downloads `boost_1_92_0.tar.gz` from
  `https://archives.boost.io/release/1.92.0/source/`, checked against the
  SHA-256 it records: the gzip archive, since Windows Server 2022's `tar.exe`
  has no bzip2. It builds b2 with `bootstrap`, and installs Boost's headers
  (`--with-headers`) and that b2 into a prefix cached per image. It keeps
  beside them, in `<prefix>/source`, the archive's `boost/version.hpp`, every
  `libs/<library>/src` and the marker `.webcpp-boost-source` (chapter 1). It
  writes the `using boost` line of `.local/user-config.jam`, with the
  `modules.poke : BOOST_SOURCE` that names `<prefix>/source`, and puts b2 on
  `PATH`, on Linux, macOS and Windows. The headers are installed with
  `--layout=system`, in `<prefix>/include/boost`, and b2 with
  `b2-install-layout=standard`, in `<prefix>/bin`: on Windows, Boost's default
  is the versioned layout, `<prefix>/include/boost-1_92`, and b2's the
  portable one, which ignores `--bindir` and puts b2 in the prefix itself. A
  prefix that lacks the headers, b2 or the sources after the install fails the
  action before it is cached. It refuses an empty or relative prefix: b2 given
  an empty `--prefix` installs into `/usr/local`. `install.sh source <dir>`
  fills such a tree alone, for a machine whose Boost comes from elsewhere.
- `wasi-sdk` installs wasi-sdk 34 into `.local/wasi-sdk`, `wasmtime`
  installs wasmtime 47.0.3 on `PATH`, `mrdocs` installs MrDocs 2026.9.29 into
  `.local/mrdocs`, and `node` sets up Node 26.7.0, with npm's cache keyed on
  the lock files of `tools/doc`, `tools/lint`, every library's
  `test/oracle` and the site's, `site/`.
- `wit-bindgen` installs wit-bindgen 0.62.0, the release's archive for the
  runner's system and processor (Linux and macOS, x86-64 and arm64), the
  program alone, into `.local/wit-bindgen`; `wasi-wit` installs the
  `wit/deps` directories of the crates `wasip2` 1.0.4 and `wasip3` 0.9.0,
  from `https://static.crates.io/crates/`, into `.local/wasi-wit/p2` and
  `.local/wasi-wit/p3`. Both install where the build looks when no `-s` is
  given (chapter 1), so a job's b2 commands name neither, and each replaces
  what an earlier install left.
- `emsdk` installs Emscripten 6.0.11 into `.local/emsdk`, on Linux and
  macOS, x86-64 and arm64: the emsdk repository at the commit of its tag
  `6.0.11`, fetched by its hash with git, the commit being the pin (the
  install fails unless `HEAD` is it, and keeps no `.git`), and every archive
  `emsdk install 6.0.11` installs from (the release's binaries, Node 24.19.0
  and, on macOS, Python 3.13.3), each downloaded first and checked against
  the SHA-256 it records, since emsdk checks none: emsdk runs with
  `EMSDK_KEEP_DOWNLOADS=1`, under which it installs from the files it finds
  in its `downloads/`, and one it downloads besides them fails the action,
  naming it, before `emsdk activate`, which on macOS runs the Python the
  install unpacked. A release whose binaries hold no
  `install/emscripten/node_modules` fails before emsdk runs, since emsdk
  would install them with `npm ci` from the registry. It checks that
  `emcc --version`
  names `6.0.11 (a0014542110d6078c3a1a7941fa1ddb3a2281f16)`, else fails
  naming both; installs `tools/ci/actions/emsdk/node.sh` as
  `.local/emscripten/node`, the node that the toolset runs a program with,
  which refuses `--experimental-wasm-threads` without a word (b2 1.92's
  emscripten toolset probes Node with that flag, which Node 24 and newer
  refuse with an error on standard error, in every b2's output); gives the
  later steps `EM_CACHE`, `.local/emscripten-cache` by its resolved path,
  outside the emsdk (chapter 1); and warms that cache: Emscripten checks its
  configuration the first time it meets a cache and says "Running sanity
  checks" on standard error, so the action runs `emcc --version` once, where
  the log is its own, and fails unless `em++ --version` then says nothing
  more. On a cache miss, it then builds with `embuilder` the system libraries
  the lanes link, the list `install.sh` records, measured from what a fresh
  cache builds for a C++ program linked at `-O2` and `-O0 -g`, with and
  without `-fwasm-exceptions`: libc, libc++ and libc++abi in their debug,
  exception and noexcept variants, compiler-rt, dlmalloc, the unwinder and
  the default stubs. A variant not listed is still built when a link needs
  it. The emsdk and that cache are cached together, keyed on the version, the
  runner and the action's files, so a change to the list builds a new entry.
  A job that only parses Emscripten's headers, the docs and the lint, gives
  the input `libraries: 'false'`: under the same key it restores the cache
  the lanes save, its libraries built, and on a miss it writes the sysroot
  alone (`embuilder build sysroot`) and saves nothing, since a lane that
  restored that cache would build every library again.
  A warmed cache, rather than
  `EMCC_SKIP_SANITY_CHECK=1` as the tests of the build set: the check still
  runs, once, and again when the version or the emsdk's directory changes.
  The toolset's lines are the regions `emsdk` and `emscripten` of
  `tools/ci/emsdk.jam`, which `matrix.py register emscripten` writes with
  `.local/emsdk` in place of `/path/to/emsdk` and `$(emsdk)`, and
  `.local/emscripten/node` in place of `/path/to/node` and `$(node)`, and
  which a page includes as wasi's does those of `wasi-sdk.jam` (chapter 3):

  ```
  local emsdk = /path/to/emsdk ;
  local node = /path/to/node ;
  using emscripten : : $(emsdk)/upstream/emscripten/em++ : <nodejs>$(node) ;
  ```
- `openssl`, `secp256k1` and `libdatachannel` install trystero's external
  libraries where its `build.jam` looks for them, in `.local/`, and give the
  later steps their variables (`$GITHUB_ENV`), on Linux and macOS, x86-64
  and arm64, and on Windows x86-64:
  - `openssl` lays out the runner's own OpenSSL 3 in `.local/openssl` and
    exports `OPENSSL_ROOT`: on Linux the system's, which `pkg-config` names,
    and on macOS Homebrew's `openssl@3`, each as links to its
    `include/openssl` and its two libraries, so that a compiler is given a
    directory of its own, never `/usr/include` (GCC 14 and Clang 18 given
    `-isystem /usr/include` no longer find the C library's headers through
    libstdc++'s `#include_next`, as a reference parses its dependencies'
    directories); Debian and Ubuntu keep the headers of OpenSSL's build
    configuration, `opensslconf.h` and `configuration.h`, in the
    architecture's `/usr/include/<multiarch>/openssl`, which
    `dpkg-architecture` names, and which a parse for another target, as
    trystero's reference is (chapter 2), does not search: there
    `include/openssl` is a directory of links to each header of both, and a
    layout without `opensslconf.h` fails the action; on Windows, copies of the headers and of the import
    libraries `libssl.lib` and `libcrypto.lib` of the image's installation in
    `%ProgramFiles%\OpenSSL` (from the first of `lib/VC/x64/MD`,
    `lib/VC/x64/MDd` and `lib` that holds both), whose `bin`, with the DLLs,
    it puts on `PATH`. The image's is always preferred; only an image without
    one has the installer of Shining Light Productions, pinned by the SHA-256
    that `slproweb/opensslhashes` records, which Shining Light removes once
    superseded, so that fallback then fails at `download.sh`, naming the URL,
    until the pin moves. On macOS, when the image lacks `openssl@3`, which it
    has today only as a dependency of other formulae, the action installs it
    with `brew`, which checks its bottle against its formula's SHA-256, and
    says so in its log. It fails, naming it, on one that is not OpenSSL 3.
  - `secp256k1` builds libsecp256k1 0.8.0 from GitHub's archive of its
    commit `6e2c8bc`, checked against its SHA-256, with CMake, static, with
    its `schnorrsig` and `extrakeys` modules and the small tables trystero's
    page builds it with, into `.local/secp256k1-native`, with the input
    `cc` (the lane's C compiler, else CMake's own), and with `emscripten:
    true` also with Emscripten's `emcmake`, after the emsdk action, into
    `.local/secp256k1-emscripten`; it exports `SECP256K1_ROOT` and
    `SECP256K1_EMSCRIPTEN_ROOT`. The emscripten build's cache key names the
    Emscripten version the emsdk action installs, read from its
    `install.sh`, so that a bump of emsdk builds it again.
  - `libdatachannel` clones the tag `v0.24.6`, fails unless `git rev-parse
    HEAD` is the tag's commit `6b1e2e6`, read with `gh api`, and only then
    fetches its submodules, at the commits that commit's gitlinks name, so a
    tag that moved never has git fetch from the URLs its `.gitmodules` would
    name; it builds it shared, `NO_WEBSOCKET`, `NO_EXAMPLES` and
    `NO_TESTS` on and `NO_MEDIA` off, against the OpenSSL `OPENSSL_ROOT`
    names (the `openssl` action runs first), with the inputs `cc` and `cxx`,
    into `.local/libdatachannel`; it exports `LIBDATACHANNEL_ROOT`, and on
    Windows puts its `bin`, with `datachannel.dll`, on `PATH`, where a test
    b2 runs finds it.

  The two that build are cached, keyed on the version, the runner, the
  compilers' own `--version` (else the image), the emscripten build or
  OpenSSL's version, and the action's files. On Windows they build CMake's
  Debug configuration: b2's debug variant, the lanes', builds with `/MDd`,
  whose standard types differ from `/MD`'s, and a DLL whose C++ API a
  program shares must match it. CMake names libsecp256k1's library
  `libsecp256k1.lib` there, and a program that links it statically defines
  `SECP256K1_STATIC`, which trystero's `build.jam` does on `msvc`, with
  OpenSSL's `libssl` and `libcrypto`.
- `chrome` installs Chrome for Testing's `chrome-headless-shell`
  155.0.8059.39, the Stable channel of Google's
  `last-known-good-versions-with-downloads.json` when it was pinned, from
  the URL that JSON lists, checked against the SHA-256 recorded for each
  build (Google publishes none: each is the digest of the archive
  downloaded twice, which matched the MD5 Google's storage states for it),
  into `.local/chrome-headless-shell`, on Linux x86-64 and arm64, and
  exports `CHROME`, a wrapper that runs it with `--no-sandbox`, a decision
  kept until it is measured on a runner: its sandbox needs unprivileged
  user namespaces, which Ubuntu 24.04 grants through AppArmor only to
  programs with a profile, and its archive has no setuid `chrome-sandbox`
  (in an Ubuntu 24.04 container under Docker, as an unprivileged user, it
  stops with "No usable sandbox!" without the flag, the symptom, though
  Docker refuses those namespaces for a reason of its own). The threat
  model: a sandbox confines a renderer against a hostile page, and the
  shell loads only the interop driver's own pages, on 127.0.0.1, with
  every other host unresolvable (the driver's `--host-resolver-rules`).
  The options that keep the sandbox, to measure after the first CI run,
  are an AppArmor profile granting `userns` to this shell alone, and
  `CHROME_DEVEL_SANDBOX` naming the image's setuid helper,
  `/opt/google/chrome/chrome-sandbox`.
  The headless shell sends nothing to Google of its own (measured).
- `container` gives a job the container lanes' image (above): built from
  `tools/ci/container/Dockerfile` when the cache does not hold it, saved
  there under the Dockerfile's digest, and loaded from there otherwise.
- `tools/ci/download.sh <url> <sha256> <file>` downloads each pinned file,
  and leaves no file and exits 1 when the download fails or the digest
  differs.
- `tools/ci/helpers.sh` holds what the actions share, which each
  `install.sh` sources beside `download.sh`: `digest`, the SHA-256 of files
  one after the other; `mixed`, a path as Windows programs read it;
  `configuration`, CMake's `Debug` on Windows and `Release` elsewhere; and
  `compiler_identity`, the first line of the `--version` of each of `CC`
  and `CXX` that is set, else the image. A cache key names it with the
  action's own files.

`tools/ci/actions_test.py` pins `download.sh` and `helpers.sh` (and that no
action defines a function of its own that `helpers.sh` holds), the Boost action's prefix
checks and the layouts it installs the headers and b2 in, and where the
wit-bindgen, WASI WIT and emsdk actions install what they download, what
they leave out, and their refusals: a runner no build of wit-bindgen or
emsdk is pinned for, a crate that holds no WIT, an emsdk at another commit,
a release without its `node_modules`, an archive emsdk would download
unpinned, and an emcc that is not 6.0.11; the emsdk action's node wrapper,
`EM_CACHE`, warmed cache and system libraries; for the external
libraries, the commands each action builds with, where it installs, what it
exports and its cache key (the emscripten build's from the emsdk action's
version), libdatachannel's submodules fetched only after its commit is
checked, macOS's `openssl@3` installed with `brew` when absent, and the
headers of Debian's `include/<multiarch>/openssl` linked beside the others; the
chrome action's shell and its `--no-sandbox` wrapper; the container action's
image, built and saved once, then loaded; and their refusals: an archive of libsecp256k1
whose SHA-256 is not the pinned one, a libdatachannel at another commit,
libdatachannel without `OPENSSL_ROOT`, an OpenSSL that is not 3, one
whose headers hold no `opensslconf.h`, a chrome-headless-shell whose SHA-256 is not the pinned one, and a
runner none of them builds for.

**The site.** On every run of the superproject's CI, the job `site` runs
`b2 site`, the preview ("The site", above), with the pages the docs job built
and the report, `-sWEBCPP_SITE_DOCS` and `-sWEBCPP_SITE_REPORT`, a red matrix
included. On a push to `main`, `publish` pushes that build, as the one commit
of an orphan branch `gh-pages`, to `webcpporg/webcpporg.github.io`, whose
Pages serve it at `https://webcpporg.github.io/`, with that repository's
deploy key, the secret `SITE_DEPLOY_KEY`; then `pages` deploys the
superproject's own Pages, the old address
`https://webcpporg.github.io/webcpp/`, as `tools/site/redirect.py`'s two
pages, which send every path under it to the same path at the root and
carry noindex too.

It fails, naming each fault, when a library has no page, when the index does
not link a library's page, when a page does not carry the robots of its
channel, or when a link of any page names no file of the site or no anchor
of its page: an index built without `-sWEBCPP_INDEX=site` links out of the
site, and fails.

A library's commits reach its own `main` first; then the superproject's
commit that moves its submodule is pushed to the superproject's `main`, whose
CI tests every library at the commits its submodules point to. What fails
there is fixed with a test, in the library or in the superproject, by more
commits on `main`; then each library's CI runs again, against the
superproject's new `main`. `main` is the only branch.

## 5. Changelogs and releases

### The changelog

- **Every change a user can see adds its line, in the same commit.** A
  library's goes in its `doc/changelog.adoc`, under "Unreleased"; one of the
  superproject's tools or build in the release notes of the version in
  preparation, `doc/releases/<version>.adoc`, under "Tools and build", which
  lists what changes for whoever builds, lints or tests with it: its b2
  rules and targets, the lint's rules, the CI's lanes. Agents write them:
  for webcpp, this replaces the rule of never editing a changelog.
- **A line** is one bullet: `* `, a past-tense verb (Added, Changed,
  Deprecated, Fixed, Improved, Removed or Updated), then one sentence of
  about ten words, what a user sees rather than how it was done; it may wrap
  onto the next lines. A breaking change starts with `*Breaking change:*`
  and says what the user must change; a deprecation names what replaces it;
  a fixed bug names the symptom, with its issue or pull request when there
  is one, `https://github.com/webcpporg/<library>/issues/<n>[#<n>]`. A line
  holds no cross-reference, `<<...>>`: the index page shows it too, where the
  library's anchors are not.
- **At most ten lines per library and version,** the most important first;
  a version's documentation work is one line. A library's change that no
  user sees (a test, its CI, a refactoring) gets none.
- **The file.** `[appendix]`, `[#<library>-changelog]`, `== Changelog`, a
  paragraph, then `=== Unreleased` and each released version,
  `=== webcpp <version>`, newest first, each with its anchor
  (`[#<library>-changelog-unreleased]`, `[#<library>-changelog-0-1-0]`) and
  its lines between `// tag::unreleased[]` (`// tag::<version>[]`) and its
  `end::`. A released section is never rewritten: a correction is a line of
  the next version. The lint's rule `changelog form` checks every changelog
  and release notes on every run (`tools/release/changelog.py`).
- **The release notes,** `doc/releases/<version>.adoc`, hold what belongs to
  the collection, in Boost's sections: New Libraries (one line each, what it
  is, "from <authors>"), Updated Libraries (each library a section that
  includes its changelog's region, alphabetical), Removed Libraries (what
  went out, and what replaces it), Tools and build, Compilers tested (the
  tested compiler lanes, a nonempty ordered unique subset of the CI's
  canonical lane names). List only the compilers actually tested for this
  release; a local-only release does not claim coverage of other CI lanes.
  The index page includes each, newest first. While a version
  is in preparation its notes open with the paragraph "In preparation: ..."
  and include each library's `tag=unreleased`.

### Releases

webcpp has one version, `WEBCPP_VERSION` in `<webcpp/version.hpp>`
(`libs/config`), the one place it is written: 0.1.0 is 100,
`WEBCPP_LIB_VERSION` `"0_1"`. Nothing may follow the number on its
`#define WEBCPP_VERSION` line but trailing spaces: both the Jamroot and
`tools/release/release.py` read that line, and both refuse anything else
there, a trailing comment included. The Jamroot reads it for the release
targets, which run only when named:

| Command | What it does |
| --- | --- |
| `b2 check-release` | fails, naming each fault, when a library changed since the last tag (under `include/`, `build.jam`, `wit/`, `meta/libraries.json`, `README.md`, `example/`, `doc/` or `LICENSE*`) has no line, when the notes miss a library, when a new or removed submodule is not under New or Removed Libraries, when the version disagrees anywhere, when a line or a file breaks the form above, or when the What's new's highlights break theirs |
| `b2 whats-new` | writes `doc/releases/<version>.whats-new.md`, the skeleton of the GitHub Release's body, in Boost's form: at most five highlights, a bold category and at most 20 words each; never over a file that exists |
| `b2 dist` | builds every page, then `bin/dist/webcpp-<version>.tar.gz`, reproducible (sources at the recorded commits, built pages, no dot files), and its `sha256sum` file `.tar.gz.txt` |

`check-release` reads every library's checkout, so `git submodule update
--init` runs first: a submodule of HEAD not checked out fails it by name
(`git submodule update --init libs/<name>`).

`dist` also bundles the tracked sources and upstream licences of the root
source submodules `site` and `third_party/libs/<name>`, at their recorded
commits, with no dot files. Each must be checked out at that commit and hold
no uncommitted changes. It never initializes submodules. The unused upstream
nested submodules of `third_party/libs/nativejson-benchmark` are omitted, as
`CODING-GUIDELINES.md` chapter 10 requires; its own sources, benchmark data
and licence are bundled. Every other unsupported root or nested submodule is
refused by name, so required sources are never silently omitted.

Nothing is released until the owner asks. Then:

1. In every library whose "Unreleased" holds lines, it becomes
   `=== webcpp <version>`, with its anchor and region, under a new empty
   "Unreleased"; the notes lose their paragraph "In preparation: ..." and
   include `tag=<version>`; each repository commits, and the superproject
   records each library's commit.
2. `b2 check-release` passes.
3. `b2 whats-new`; an agent writes the highlights from the notes, and the
   owner approves them before the tag.
4. `b2 -a dist`.
5. The tag `webcpp-<version>` goes on the superproject and on every library
   at the commit the superproject records. Whether it is signed is the
   owner's decision then.
6. The GitHub Release `webcpp-<version>` carries the archive, its checksum
   file and the "What's new" as its body:
   `gh release create webcpp-<version> bin/dist/webcpp-<version>.tar.gz
   bin/dist/webcpp-<version>.tar.gz.txt --title "webcpp <version>"
   --notes-file doc/releases/<version>.whats-new.md`.
7. Right after, `version.hpp` names the next version, whose notes are
   opened from `tools/release/template.adoc` (`sed -e 's/@VERSION@/<next>/g'
   -e 's/@ANCHOR@/<next with dashes>/g'`) and included first by the index.

## 6. b2 facts

Each of these was measured; each has cost time.

- **Every b2 run unsets `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH`.**
  A shell that sets them (Homebrew's often does) makes the compiler read
  another Boost before the configured one, without a word; the Jamroot
  refuses to run while one is set:

  ```
  env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 ...
  ```

- **A verdict is built from scratch,** `b2 -a`: b2 compares timestamps, and
  an edit in the same second as the last build is missed.
- **`$(` inside an action is b2's, not the shell's.** b2 expands `$(name)` in
  an action's text, so a shell `$(command)` silently becomes empty; write
  command substitution with backquotes.
- **An action of more than one command opens with `set -e`,** or a failing
  command in its middle goes unnoticed.
- **`<build>no` is conditioned on the toolset,** never on a derived feature
  such as `<target-os>`: b2's evaluation of conditional requirements then
  never settles, since `<build>no` replaces every other conditional result,
  `<target-os>wasi` among them.
- **A syntax error in a Jamfile only prints,** and every target declared
  after it silently stops existing. When a target is "not found", read the
  first lines of b2's output.
- **A subproject's target is named with its project:** `b2
  libs/xactor/test//scheduler`, `b2 libs/xactor/doc//reference`.
- **`testing.launcher` applies to every toolset of one request,** so a wasm
  lane and a native one are two b2 runs.
- **b2's emscripten toolset runs a test with node itself:**
  `testing.launcher=node` makes the command `node "node" x.js`, and webcpp
  refuses it.
- **With `--out-xml`, b2 exits 0 even when a test fails;** the report's exit
  status is the verdict, and the report needs `--dump-tests`.
- **b2 5.5.3's `--command-database` writes nothing,** so the compilation
  database comes from `tools/lint/compile_commands.py`, which reads b2's dry
  run (`b2 -n -a`).
- **Every `-s` of the command line and every variable of the environment
  is a global variable,** which `modules.peek` reads and a `user-config.jam`
  loaded after them may replace with its `modules.poke`: only `ARGV` tells
  the command line's value apart, in both its forms, `-sNAME=<value>` and
  `-s NAME=<value>`, and nothing tells user-config.jam's from the
  environment's. An environment value is split at each space, a `*PATH` one
  at the path separator, so it is joined again (`$(value:J=" ")`).
- **A free feature plays no part in a target's build directory:** an
  example's output from one launcher would read as up to date under another,
  which is why `webcpp.example` always runs again.
- **wasm32-wasip2 and wasm32-wasip3 are no system b2 knows.** The Jamroot
  adds the `<target-os>wasi` value after the toolsets are loaded, so that gcc's
  Unix link options (`--start-group`, `-Bstatic`, rpath), which
  wasm-component-ld rejects, are never chosen for it.
- **Boost's location.** `boost.use-project` takes the `using boost` of
  `user-config.jam`, else `BOOST_ROOT`, else the compiler's default paths.
  Apple's clang searches `/usr/local/include` before every `-isystem`
  directory, so clang keeps Boost's directory as `-I` and marks the headers
  `<boost/...>` names as system ones (`--system-header-prefix=boost/`).
- **A checkout path that holds a space** builds: every path b2 hands to an
  action is quoted, and the tests of the build run in such a path.
- **A target named `release-<word>` is no target.** b2 reads it as the
  variant `release` with a subfeature and stops before it builds anything
  (`"release-check" is not a known value of feature <variant>`); `debug-`
  and `profile-` likewise. The release check is `check-release`.
