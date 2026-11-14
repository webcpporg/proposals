## MODIFIED Requirements

### Requirement: Build: The aggregates and the explicit targets
The Jamroot SHALL declare the aggregates `test`, `example`, `doc` and `install` over the registered libraries' directories, the notfile targets `declared-targets`, `declared-lanes`, `site`, `site-serve`, `check-release`, `whats-new` and `dist` (the site targets when `site/` is checked out, else `site` and `site-serve` stop with "site/, the site's repository (webcpporg/webcpporg.github.io), is not checked out ... git submodule update --init site"), and the explicit targets `boost_version`, `throw_exception`, `boost_json`, `boost_url`, `boost_charconv`, `boost_locale`, `node-test` and `emscripten-reference`. The Jamroot MUST NOT declare a Cobalt target, nor an alias to one: each of Cobalt's targets has one name, under `/webcpp/cobalt`. `declared-targets` and `declared-lanes` MUST read only Jamfiles and need neither Boost nor Node: they leave the Boost check out with `<webcpp-boost-check>off`, as every target that compiles nothing does.

#### Scenario: declared-targets needs no Boost
- **WHEN** `b2 -d0 declared-targets` runs on a CI job that installed no Boost
- **THEN** it prints the `<library> <target>` pairs and nothing else

#### Scenario: The site is not checked out
- **WHEN** `b2 site` runs without `site/Jamfile`
- **THEN** it stops with the message that `site/` is not checked out and the command `git submodule update --init site`

#### Scenario: No Cobalt target at the root
- **WHEN** a program names `/webcpp//boost_cobalt` or `/webcpp//boost_cobalt_io_ssl`
- **THEN** b2 stops with "Unable to find file or target named" and that name, and the program names `/webcpp/cobalt//cobalt` or `/webcpp/cobalt//cobalt_ssl` instead

### Requirement: Build: Boost's compiled libraries from its sources
A compiled Boost library that a webcpp library needs SHALL be built by webcpp from Boost's own sources for every target, as `/webcpp//boost_url`, `/webcpp//boost_charconv` and `/webcpp//boost_locale` (`tools/boost/Jamfile`, a project `/webcpp/boost-libraries`, each Jamroot alias checking the sources first), static and built once per variant; a library adds e.g. `<library>/webcpp//boost_url` to its target's usage requirements and never `/boost//url` (which would link the system's) nor `/webcpp/boost-libraries//boost_url`. The sources are a tree with Boost's `boost/version.hpp` and `libs/<library>/src` for each library, found at `-sBOOST_SOURCE=<dir>` on b2's command line (which wins), else `BOOST_SOURCE` as a `user-config.jam` `modules.poke` or the environment gives it, else `.local/boost-source`, which `bash tools/ci/actions/boost/install.sh source .local/boost-source` fills from the archive the CI uses, checked against its SHA-256 and marked by `.webcpp-boost-source`; the script replaces only a directory that holds the marker and refuses any other by name before downloading. When a target needs the tree and it is missing, its version is not the configured Boost's, the configured version is unknown (Boost on the default include path), or it lacks the library's sources, the build MUST stop naming why and the fix. Boost.Cobalt is not one of these libraries: webcpp's library Cobalt ships its own sources (`libs/cobalt/sources.json`) and builds them in its own project, `/webcpp/cobalt`, so neither the Jamroot nor `/webcpp/boost-libraries` declares a Cobalt target.

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
`/webcpp//boost_locale` SHALL be Boost.Locale without ICU and `/webcpp//boost_charconv` Boost.Charconv, with these behaviours (`CODING-GUIDELINES.md` chapter 10): native Linux builds the posix and std backends with glibc's iconv; native macOS the posix and std backends with the system's libiconv given as `-liconv` by the target; native Windows (MSVC) the winapi and std backends, not measured until the first CI run that builds it; wasip2 and wasip3 the posix and std backends with wasi-libc's iconv, formatting in the C locale's form, a catalog read from a directory the test gives with `webcpp.data`; emscripten the posix and std backends with Emscripten's iconv, single-threaded, Boost.Locale's sources alone compiled with `BOOST_HAS_PTHREADS` so Boost.Thread's mutex builds, with no public header of Boost.Locale depending on it (checked by `tools/test/boost_libraries_test.py`); any target with `exception-handling=off` refuses Boost.Locale by name (61 `throw`, 14 `try` in its sources, 8 `throw` in its headers) while Boost.Charconv builds.

#### Scenario: Locale on wasip2
- **WHEN** a test using Boost.Locale runs with `toolset=clang-wasip2`
- **THEN** it formats numbers, currencies and dates in the C locale's form and reads a message catalog from the directory given by `webcpp.data`

#### Scenario: Charconv without exceptions
- **WHEN** `/webcpp//boost_charconv` is built with `exception-handling=off`
- **THEN** it builds, as measured natively and on wasip2

### Requirement: Library: build.jam
A library's `build.jam` SHALL start with the licence notice, declare `project /webcpp/<name> ;` and one target, named after the library, that adds its headers and dependencies: `alias <name> : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;`. Every library adds config's target since its `config.hpp` includes `<webcpp/config.hpp>`; a dependency on another library is its target (`<library>/webcpp/xactor//xactor`), Cobalt's included (`<library>/webcpp/cobalt//core`); Boost.URL, Boost.Charconv and Boost.Locale are `<library>/webcpp//boost_url`, `//boost_charconv` and `//boost_locale`; Boost.JSON is `<library>/webcpp//boost_json`; a library that builds components declares its worlds' bindings there with `webcpp.wit-bindings`. The dependencies SHOULD sit between `tag::dependencies[]` and `end::dependencies[]` markers which the page includes with `include::{library-root}/build.jam[tag=dependencies]`.

#### Scenario: config's build.jam
- **WHEN** `libs/config/build.jam` is read
- **THEN** it declares `project /webcpp/config ;` and `alias config : : : : <include>include <library>/boost//headers ;`

#### Scenario: A dependency on another library
- **WHEN** xstate's actor layer depends on xactor
- **THEN** its target lists `<library>/webcpp/xactor//xactor` among its usage requirements

#### Scenario: A dependency on Cobalt
- **WHEN** a library or a program depends on Cobalt
- **THEN** it lists one of Cobalt's targets under `/webcpp/cobalt` (`//core`, `//cobalt`, `//io_ssl` or `//cobalt_ssl`), never a name of the superproject's root or of Asio
