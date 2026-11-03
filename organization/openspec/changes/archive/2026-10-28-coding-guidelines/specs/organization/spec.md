## ADDED Requirements

### Requirement: Superproject: The coding guidelines
`CODING-GUIDELINES.md` SHALL be the single source of webcpp's rules on how code is written and of every decision of its software design, each with its reason, in nineteen chapters: process; commits and text; names; the language; bringing a C library into webcpp; exceptions and RTTI; Doc Comments; tests; what the lint enforces, with the decisions of `.clang-format` and `.clang-tidy`; choosing and registering a port; fidelity; evidence (oracle and twins); porting a library step by step; the organization's design; the libraries' design, one section per library of the superproject; the roadmap; the proposals decided and not built; the proposals not decided; and the conflicts and open questions. A rule or a decision MUST be written there once, and every other file that needs it, `AGENTS.md`, `CONTRIBUTING.md`, a skill, a library's `AGENTS.md` or page, a tool's message or a proposal, points to its chapter. Chapters 1 to 15 describe the tree as it is; chapters 16 to 18 hold what is not built, each proposal with its link, its own `design.md` ruling over the summary while it is open; chapter 19 holds each conflict between a decision and a rule, or between a design document and its tree, with the owner's choices. A design decision is one bullet: the decision, its reason as its source records it or "(reason not recorded)", and its source. The change that builds a decision, or changes one, updates its chapter in the same change, moving a built proposal's decisions out of chapters 16 to 18.

#### Scenario: A library's design
- **WHEN** chapter 15 is read for xstate
- **THEN** it lists xstate's design decisions, each with its reason and its source, among them each recorded difference from XState 5.33.2

#### Scenario: A proposal is implemented
- **WHEN** the change that builds a proposal of chapter 17 is made
- **THEN** the same change moves its decisions into chapter 14 or 15 and removes its entry from chapter 17

#### Scenario: A decision that contradicts a rule
- **WHEN** a decided proposal, or local work beyond any proposal, does what a rule of chapters 1 to 13 forbids
- **THEN** chapter 19 states the rule, the decision, whether the owner approved it and where, and the choices open, and chapters 1 to 13 keep the rule as the superproject's `main` has it

#### Scenario: A rule looked for in AGENTS.md
- **WHEN** an agent looks in `AGENTS.md` for how a library raises an exception
- **THEN** `AGENTS.md` points to `CODING-GUIDELINES.md` and holds no copy of the rule

## MODIFIED Requirements

### Requirement: Superproject: The layout of the repository
The superproject SHALL hold at its root `Jamroot` (the single build configuration), `CODING-GUIDELINES.md` (the rules on how code is written and every design decision), `AGENTS.md` (how to work in the tree), `README.md`, `CONTRIBUTING.md`, `LICENSE_1_0.txt`, `.clang-format`, `.clang-tidy` and `pyrightconfig.json` (the style, checks and Python settings of every repository), `libs/`, `third_party/`, `doc/` (the index page `index.adoc` and its `Jamfile`, with `doc/releases/` for each version's release notes), `site/` (the submodule of the site), `tools/`, `.github/` (`workflows/library.yml`, `workflows/ci.yml`, `actionlint.yaml`), the git-ignored `.local/` (machine-local setup) and the git-ignored `bin/` (b2's build directory). `tools/` MUST hold `webcpp.jam` (the Jamfile API), `target.jam`, `throw_exception.cpp`, `boost_json.cpp`, `boost_test_runner.cpp`, and the directories `boost/`, `oracle/`, `lint/`, `doc/`, `example/`, `component/`, `drive/`, `generated/`, `node/`, `report/`, `test/`, `ci/`, `site/` and `release/`.

#### Scenario: The root of the published commit
- **WHEN** `../../changes/archive/2026-10-07-genesis/reference/superproject/` is listed
- **THEN** it shows `.clang-format`, `.clang-tidy`, `.gitattributes`, `.github`, `.gitignore`, `.gitmodules`, `AGENTS.md`, `CONTRIBUTING.md`, `Jamroot`, `LICENSE_1_0.txt`, `README.md`, `doc`, `libs`, `pyrightconfig.json`, `site`, `third_party` and `tools`

#### Scenario: The root of main
- **WHEN** the superproject's root is listed after this change
- **THEN** it shows `CODING-GUIDELINES.md` beside `AGENTS.md`, `CONTRIBUTING.md` and `README.md`

#### Scenario: Ignored files
- **WHEN** `.gitignore` is read
- **THEN** it ignores `.local/`, `bin/`, `node_modules`, `.node-modules/`, `doc/html/`, `__pycache__/` and `.DS_Store`, and `.gitattributes` sets `* text=auto eol=lf` and `*.bat text eol=crlf`

### Requirement: Superproject: The rulebook and who reads it
webcpp's rulebook SHALL be two files at the superproject's root, written for coding agents and human contributors alike, which together hold every rule needed to port a library or change one, so that an agent with no other context can do it by following them: `CODING-GUIDELINES.md`, the single source of the rules on how code is written and of every design decision (requirement "Superproject: The coding guidelines"), and `AGENTS.md`, how to work in the tree. An agent reads the guidelines first, then `AGENTS.md`, then the library's own `AGENTS.md`. `AGENTS.md` describes the tree as it is, with what is still to come only in the guidelines' roadmap, to which every other mention of it refers, and has six chapters: what webcpp is, the API reference, documentation, tests/lanes/the report/the site/CI, changelogs and releases, and b2 facts. A chapter is cited as "chapter N" within its file and as "`CODING-GUIDELINES.md` chapter N" or "`AGENTS.md` chapter N" from the other. Each library's `AGENTS.md` MUST hold only what is specific to that library (the original and its version, how its oracle drives it, its cautions) and link the two files as `../../AGENTS.md` and `../../CODING-GUIDELINES.md`; `CONTRIBUTING.md` summarises the rules for external contributors, each line naming the guidelines' chapter that holds the rule, and, where it and the guidelines differ, the guidelines rule.

#### Scenario: A library's AGENTS.md
- **WHEN** `libs/config/AGENTS.md` is read
- **THEN** it begins "Working on config", tells the reader to read webcpp's `CODING-GUIDELINES.md` and `AGENTS.md` first, and holds only config's own rules (macros only, `version.hpp` outside `webcpp/config/`, the version written once, the target taken from the compiler)

#### Scenario: Every command runs from the root
- **WHEN** `AGENTS.md` or `CODING-GUIDELINES.md` shows a command
- **THEN** it runs from the superproject's root, in a shell where `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` are unset

### Requirement: Superproject: b2 facts
The build SHALL account for these measured b2 facts, which `AGENTS.md` chapter 6 records: every b2 run unsets `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` (the Jamroot refuses to run while one is set); a verdict is built from scratch with `-a`; `$(` inside an action is b2's, not the shell's, so shell command substitution is written with backquotes; an action of more than one command opens with `set -e`; `<build>no` is conditioned on the toolset, never on a derived feature such as `<target-os>`; a syntax error in a Jamfile only prints, so a target "not found" means the first lines of output are read; a subproject's target is named with its project (`b2 libs/xactor/test//scheduler`); `testing.launcher` applies to every toolset of one request, so a wasm lane and a native lane are two runs; b2's emscripten toolset runs a test with node itself and `testing.launcher=node` is refused; with `--out-xml` b2 exits 0 even when a test fails and the report's exit status is the verdict (it needs `--dump-tests`); b2 5.5.3's `--command-database` writes nothing so the compilation database comes from `tools/lint/compile_commands.py` reading `b2 -n -a`; every `-s` and environment variable is a global that `ARGV` alone tells apart; a free feature plays no part in a target's build directory; wasm32-wasip2 and wasm32-wasip3 are no system b2 knows, so the Jamroot adds `<target-os>wasi` after the toolsets load; a checkout path with a space builds; a target named `release-<word>`, `debug-<word>` or `profile-<word>` is no target, so the release check is `check-release`.

#### Scenario: A shell with CPATH
- **WHEN** b2 runs in a shell that sets `CPATH`
- **THEN** the Jamroot stops with "CPATH is set; unset it, or the compiler reads headers from it before the configured Boost"

#### Scenario: A release target name
- **WHEN** a target is declared `release-check`
- **THEN** b2 reads it as the variant `release` with a subfeature and stops, which is why the target is `check-release`

### Requirement: Build: What Boost.Locale and Boost.Charconv give on each target
`/webcpp//boost_locale` SHALL be Boost.Locale without ICU and `/webcpp//boost_charconv` Boost.Charconv, with these behaviours (`CODING-GUIDELINES.md` chapter 10): native Linux builds the posix and std backends with glibc's iconv; native macOS the posix and std backends with the system's libiconv given as `-liconv` by the target; native Windows (MSVC) the winapi and std backends, not measured until the first CI run that builds it; wasip2 and wasip3 the posix and std backends with wasi-libc's iconv, formatting in the C locale's form, a catalog read from a directory the test gives with `webcpp.data`; emscripten the posix and std backends with Emscripten's iconv, single-threaded, Boost.Locale's sources alone compiled with `BOOST_HAS_PTHREADS` so Boost.Thread's mutex builds, with no public header of Boost.Locale depending on it (checked by `tools/test/boost_libraries_test.py`); any target with `exception-handling=off` refuses Boost.Locale by name (61 `throw`, 14 `try` in its sources, 8 `throw` in its headers) while Boost.Charconv builds. Boost.Cobalt is built for native programs and has no build without exceptions.

#### Scenario: Locale on wasip2
- **WHEN** a test using Boost.Locale runs with `toolset=clang-wasip2`
- **THEN** it formats numbers, currencies and dates in the C locale's form and reads a message catalog from the directory given by `webcpp.data`

#### Scenario: Charconv without exceptions
- **WHEN** `/webcpp//boost_charconv` is built with `exception-handling=off`
- **THEN** it builds, as measured natively and on wasip2

### Requirement: Library: The files of a library
A library `libs/<name>/` SHALL be a repository of its own with: `build.jam` (project `/webcpp/<name>`), `include/webcpp/<name>.hpp` and `include/webcpp/<name>/...`, `wit/` for a library that builds components, `test/` (a `Jamfile`, the tests, `.clang-tidy` when the tests need one, `test/oracle/` for a port's oracle), `example/` (a `Jamfile`, the programs and their `.expected` outputs, a served program's `.requests`), `doc/` (a `Jamfile`, the page `<name>.adoc` and its sections, `mrdocs.yml`, `counts.py` when the page counts what only it holds, and `changelog.adoc` as the page's last appendix), `meta/libraries.json`, `meta/include-boundaries.json` when it has boundaries, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`, `LICENSE-<ORIGIN>.txt` for a port that derives from code, `.gitignore` (at least `doc/html/`, and `node_modules` and `.node-modules/` with an oracle), `.gitattributes`, an optional `.clang-format` that inherits the root's (`InheritParentConfig`) and adds only what its code needs with the reason in a comment (wasi names the macros its main is written between with `MacroBlockBegin` and `MacroBlockEnd`), and `.github/workflows/ci.yml`. A library has no Jamroot. The `README.md` MUST say what the library is, how to build and test it inside the superproject, where its page is published and its licence; its `AGENTS.md` holds only what is specific to it, with links to `../../AGENTS.md` and `../../CODING-GUIDELINES.md`, and its design decisions are in the guidelines' chapter 15.

#### Scenario: lexical's tree
- **WHEN** `libs/lexical` is listed at `e286ca4`
- **THEN** it holds `AGENTS.md`, `build.jam`, `doc`, `example`, `include`, `LICENSE_1_0.txt`, `meta`, `README.md`, `test` and `tools`, and its `.gitignore` holds `doc/html/`, `node_modules` and `.node-modules/`

#### Scenario: A library with no Jamroot
- **WHEN** a contributor adds `libs/<name>/Jamroot`
- **THEN** it is wrong: the superproject's Jamroot is every library's build configuration

### Requirement: Library: Allocation
A library SHALL avoid dynamic allocation as far as its job allows and let its user customize the allocator of what it does allocate, so that it can be used with Boost.Interprocess. A data structure that makes sense to share (a parsed JSON document, a state machine's definition) takes an `Allocator` template parameter, allocates through `std::allocator_traits` of it and holds every pointer as the allocator's `pointer` type (`offset_ptr`) or as an offset, never an absolute address, a vtable or a `std::function`, so that it can be built in a shared segment and used in place by another process that maps it elsewhere. Everything else (actors, rooms, sockets, handlers) MUST route every allocation through an allocator its user gives, a template parameter or a `std::pmr::memory_resource`. Each library's page says which of its types are shareable and a test that counts allocations proves none goes elsewhere; a library written from now on is born with this rule (pratt through an `Allocator`, trystero through `std::pmr`), and the older libraries are brought to it by the review of the roadmap (`CODING-GUIDELINES.md` chapter 16).

#### Scenario: trystero's counting test
- **WHEN** a `std::pmr::memory_resource` is given in a room's configuration
- **THEN** a counting test proves, natively and on emscripten, that every allocation of a room reaches it

### Requirement: Organization: The roadmap that is not built
The organization SHALL keep, in chapter 16 of `CODING-GUIDELINES.md`, what it does not have yet, and no other chapter of the guidelines or of `AGENTS.md` may state it as present: every variant tested in three levels of CI (fast on every push, nightly on main, release); the review of the existing libraries against the rules of exceptions, variants, allocators, the order of what a library builds on, writing for the web, the newest Boost library of a kind, wrapped outside libraries and fixed-width types (the lint then checking the last); asynchronous libraries on xactor with Boost.Asio and Boost.Cobalt; webcpp::asio, webcpp::beast, webcpp::webrtc and webcpp::crypto; the libraries web developers use most (QuickJS, libp2p, HTTP fetch); documentation for web developers and AI developers and integrations with the AIs; external libraries built from `third_party/`; every library on `<webcpp/config.hpp>` (trystero held with config's publication, wasi with its redesign); the allocator rule applied to xactor, xstate and wasi; the compiled Boost libraries' remaining gaps (Boost.Locale and Boost.Cobalt without exceptions, Windows not measured, a Boost.Test suite compiling the framework's header-only form); bundled toolchains; and the site's increments 2 to 8.

#### Scenario: A rule not yet true
- **WHEN** the CI levels are asked about
- **THEN** `CODING-GUIDELINES.md` chapter 16 says they are a roadmap item and the CI today runs one level on every push

### Requirement: Superproject: Agent skills
The superproject and the proposals repository SHALL keep agent skills in `.agents/skills/<name>/SKILL.md`, one directory per skill named in kebab case, each `SKILL.md` opening with YAML front matter whose `name` equals the directory and whose `description` says when the skill applies. A skill MUST point to the rules that apply (the chapter of the superproject's `CODING-GUIDELINES.md` or `AGENTS.md`, `CONTRIBUTING.md` or the organization's specification) and list the steps and commands of its task, without restating those rules at length; where a skill and a rule disagree, the rule wins. Skills follow the rules on text of every file: English, no em dash, no banned word, no personal data. The proposals repository holds OpenSpec's own skills, generated by OpenSpec 1.10.0 and adapted to its several OpenSpec roots, and `openspec init` is never run in the repository itself, since it would create a root `openspec/`. Each repository's `AGENTS.md` names its skills. CI MUST parse skill front matter as YAML and reject invalid syntax, a name that differs from its kebab-case directory, or a missing or blank string description. Markdown files, including `SKILL.md`, need no webcpp source-file licence header; upstream notices MUST be preserved.

#### Scenario: An agent writes a proposal
- **WHEN** an agent in the proposals repository is asked to propose a change to a library
- **THEN** the skill `webcpp-openspec-proposal` sends it to `CONTRIBUTING.md` and the organization's specification, and it runs `openspec` from `libraries/<name>/`

#### Scenario: A skill with forbidden text
- **WHEN** a `SKILL.md` under `.agents/skills/` of the proposals repository holds an em dash
- **THEN** `python3 tools/proposals.py check` fails, naming the file and the line

#### Scenario: A skill and a rule disagree
- **WHEN** a chapter of the superproject's `CODING-GUIDELINES.md` changes a rule a skill cites
- **THEN** the chapter is followed, and the skill is updated to point to it

#### Scenario: Invalid skill metadata
- **WHEN** a skill's description contains an unquoted colon and space, its name differs from its directory, or its description is absent or blank
- **THEN** the Agent skills workflow fails and names the skill
