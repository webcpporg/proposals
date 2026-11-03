## Why

webcpp's rules and its design decisions are spread over many files. The superproject's
`AGENTS.md`, 2,714 lines in thirteen chapters, mixes the rules of how code is written (names, the
C++ rules, exceptions, the lint, Doc Comments, tests, fidelity, the oracle, the process, the text
of commits) with how to work in the tree (prerequisites, commands, the reference's generation,
pages, lanes, the report, the site, CI, releases, b2 facts). The decisions behind each library,
and behind the organization itself, are in some twenty-four `design.md` files of this repository,
in each library's `AGENTS.md` and in the pages of each library, and the decisions taken and not
yet built are in the roadmap and in the open proposals. A contributor, human or agent, who wants
to know how code is written in webcpp and why it is designed the way it is has no one place to
read, and a decision restated in two places drifts: the asio work already contradicts the
rulebook in places that no file records (`design.md`, D6).

The owner asked for one document, `CODING-GUIDELINES.md` at the superproject's root, that holds
every decision on how code is written and every decision of the software design, and for every
file that pointed to the rulebook to point to it.

## What Changes

- The superproject gains `CODING-GUIDELINES.md`, the single source of the rules on how code is
  written and of every design decision, each with its reason, in nineteen chapters: working
  (process, commits and text), writing code (names, the language, C libraries, exceptions and
  RTTI, Doc Comments, tests, the lint with the decisions of `.clang-format` and `.clang-tidy`),
  libraries and ports (choosing a port, fidelity, the oracle and twins, porting step by step),
  design (the organization's, and each library's as built), and what is not built yet (the
  roadmap, the decided proposals, the proposals not decided, and the open conflicts).
- The superproject's `AGENTS.md` keeps how to work in the tree, in six chapters (what webcpp is,
  the API reference, documentation, tests and lanes and the report and the site and CI,
  changelogs and releases, b2 facts), and points to the guidelines for every rule. Its text is
  moved, not rewritten: every moved paragraph is the same, and every "chapter N" is rewritten to
  the chapter's new home.
- Every live file that cited the rulebook cites the new chapter: `CONTRIBUTING.md`, `README.md`,
  `doc/index.adoc`, the agent skills, the comments and messages of `Jamroot` and `tools/` (the
  lint's changelog rule, the release tooling) and their tests; each library's `AGENTS.md` and the
  site's; and, in this repository, `CONTRIBUTING.md`, `AGENTS.md`, the skills and every root's
  `openspec/config.yaml`. Archived proposals, recorded patches, plans and reference trees are
  history and are not changed.
- A library's `AGENTS.md` links both files, keeps only what an agent needs to work on that
  library, and a design decision it states with its reason moves to the guidelines' chapter 15.
- The organization's specification: the rulebook becomes the two files; the guidelines, their
  chapters and how a decision enters and leaves them are added; the roadmap moves to the
  guidelines' chapter 16; the requirements that cite a moved chapter cite its new home; and the
  site's community page links the guidelines.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `organization`: "Superproject: The coding guidelines" is added; "Superproject: The layout of
  the repository", "Superproject: The rulebook and who reads it", "Superproject: b2 facts",
  "Build: What Boost.Locale and Boost.Charconv give on each target", "Library: The files of a
  library", "Library: Allocation", "Organization: The roadmap that is not built" and
  "Superproject: Agent skills" are modified.
- `site`: "Site: The community page and the search page" is modified.

## Impact

- **Kind:** the organization (the rules' home, in the superproject, every library, the site and
  this repository).
- **Implementation:** branches `coding-guidelines` of https://github.com/webcpporg/webcpp, of each
  library's repository and of https://github.com/webcpporg/webcpporg.github.io, and of
  https://github.com/webcpporg/proposals. No behaviour of the build, the lint or a library changes;
  the lint's and the release tooling's messages cite the new chapter, with their tests.
- **Status:** accepted by the owner on 2026-10-26, with the answers to O1 to O3 in `design.md`.
  The reference is `reference/CODING-GUIDELINES.md` and `reference/AGENTS.md`, the two files as
  the superproject will hold them, made from `AGENTS.md` at `ddb6cb2` (`reference/STATUS.md`).
- **Other proposals:** every open proposal keeps its text; its decisions are indexed in the
  guidelines' chapters 17 and 18 and move into chapters 14 and 15 when it is implemented. The
  conflicts the asio proposal and its local work raise are recorded in chapter 19, for the owner
  to decide; this change decides none of them.
