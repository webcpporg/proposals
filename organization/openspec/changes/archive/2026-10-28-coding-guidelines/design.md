## Context

The superproject's `AGENTS.md` is webcpp's rulebook (organization requirement "Superproject: The
rulebook and who reads it"): thirteen chapters, 2,714 lines at `ddb6cb2`, written so that an agent
with no other context can port a library by following it. Its chapters 2 to 7, 10 and 11 are
rules a change must keep; chapters 1, 7 (in part), 8, 9, 11 (in part) and 12 say how to work in
the tree; chapter 13 is the roadmap. The decisions of design behind those rules, and behind each
library, are elsewhere: the `design.md` of every proposal (twenty-four files, archived and open),
each library's `AGENTS.md` (1,292 lines over ten files), each library's pages (`guarantees.adoc`,
`differences.adoc`, `allocation.adoc`, trystero's `design.adoc`), and the comments of
`.clang-format` and `.clang-tidy`.

The owner decided on 2026-10-26 that one file, `CODING-GUIDELINES.md` at the superproject's root,
holds every rule on how code is written and every decision of the software design, as their
single source; that every file that cited the rulebook cites it instead; and that the change
follows the usual rite of a proposal.

## Goals / Non-Goals

**Goals:**

- One place to read how code is written in webcpp and why it is designed as it is, for an agent
  with no other context and for a human contributor.
- Each rule and each decision written once, with its reason; every other file points to it.
- The text of the rules unchanged by the move, so that the move can be reviewed as a move.
- The decisions not yet built, and the conflicts between them and the rules, visible in the same
  file, with the proposal that holds each in full.

**Non-Goals:**

- Changing a rule, or deciding a conflict: the conflicts found are recorded (D6), not resolved.
- Rewriting the archived proposals, the recorded patches, the plans or the reference trees, which
  are history.
- A new lint rule (O3).

## Decisions

### D1. Two files: the guidelines hold the rules and the decisions; AGENTS.md, how to work

`CODING-GUIDELINES.md` holds every rule a change must keep and every decision of design, each
with its reason. `AGENTS.md` holds how to work in the tree: what webcpp is and where everything
is, the prerequisites and commands, how the reference is generated, the pages, the lanes, the
report, the site and CI, the changelogs and releases, and the facts of b2. Each points to the
other; an agent reads the guidelines first, then `AGENTS.md`, then the library's own `AGENTS.md`.

Rejected: keeping `AGENTS.md` as the rulebook and adding a consolidated copy, which would be two
sources that drift at the first change of a rule; one file per topic, which loses the property the
rulebook was written for, one file an agent reads whole.

### D2. The line between the two, chapter by chapter

A rule a change must satisfy, or a decision with its reason, goes to the guidelines; how to do
something in the tree, or a reference of a tool's interface, stays in `AGENTS.md`. Applied to the
thirteen chapters at `ddb6cb2`:

| Old chapter of AGENTS.md | New home |
| --- | --- |
| 1. What webcpp is | `AGENTS.md` 1 |
| 2. Choosing and registering a port | guidelines 10 |
| 3. Names | guidelines 3 |
| 4. Fidelity | guidelines 11 |
| 5. Evidence: oracle and twins | guidelines 12 |
| 6. C++ rules: the language | guidelines 4 |
| 6. C++ rules: bringing a C library | guidelines 5 |
| 6. C++ rules: exceptions and RTTI | guidelines 6 |
| 6. C++ rules: what the lint enforces | guidelines 9, with a section on the decisions of `.clang-format` and `.clang-tidy` |
| 7. Doc Comments: the format, what every public symbol needs | guidelines 7 |
| 7. Doc Comments: generation, commands, after MrDocs, where the reference sits | `AGENTS.md` 2, "The API reference" |
| 8. Documentation | `AGENTS.md` 3 |
| 9. Tests, lanes, the report, the site and CI: "Tests" | guidelines 8 |
| 9. The rest of the chapter | `AGENTS.md` 4, with a pointer where "Tests" was |
| 10. Process | guidelines 1 |
| 10. Porting a library, step by step | guidelines 13 |
| 11. Commits and text | guidelines 2 |
| 11. The changelog, Releases | `AGENTS.md` 5, "Changelogs and releases" |
| 12. b2 facts | `AGENTS.md` 6 |
| 13. Roadmap | guidelines 16 |

Chapter 1's "Why webcpp" stays in `AGENTS.md`: it says what webcpp is for, and the organization's
requirement "The collection and its model" reads the targets from chapter 1. Chapter 8, the
pages, stays too: a page is not code, and its rules are what `doc-check` enforces.

Rejected: moving every chapter but chapter 1 (the Jamfile API, the lanes, the CI), which would
leave `AGENTS.md` without the reference an agent needs to run anything; keeping chapter 6 whole in
`AGENTS.md` and moving only the design decisions, which would split the C++ rules from their
reasons.

### D3. The text is moved, not rewritten; every citation is rewritten

Every moved paragraph keeps its words. Every "chapter N" in either file is rewritten to the
chapter's new home: "chapter N" within a file, "`CODING-GUIDELINES.md` chapter N" or "`AGENTS.md`
chapter N" across, and "guidelines chapter N" inside a code block. A paragraph that a rewritten
citation made longer than 78 columns is wrapped again; a sentence that said "this file" or "this
rulebook" and changed file says which file it means. `reference/split_rulebook.py` does it from
`AGENTS.md` at `ddb6cb2`, with its table of the citations whose old chapter alone does not say
their new home (chapter 6, split four ways; chapter 7, split two ways; chapter 9's tests;
chapter 11's changelog), and fails on a citation it cannot place.

Rejected: rewriting the moved rules in a new voice, which would make the move impossible to
review as a move; keeping the old numbers with stub chapters, which would leave thirteen headings
of which seven say "moved".

### D4. The design chapters: the organization's and each library's, as built

Chapter 14 holds the organization's cross-cutting decisions (the superproject, the build, the
targets and toolchains, the documentation, tests and CI, releases, proposals and decisions,
agents) and the site's; chapter 15 holds each library's, for the nine libraries of 0.1.0. A design
decision is a choice that shapes a library's interface, behaviour, data layout, dependencies,
targets, limits, error model, allocation, concurrency or proof, where another choice was possible;
each is one bullet, with its reason as written in its source and the source's path, and "(reason
not recorded)" where none is written. They were gathered from the archived `design.md`, the
specification as implemented, the library's `AGENTS.md` and its pages, and checked against the
library's tree at the superproject's `ddb6cb2`; where a design document and the tree disagree, the
chapter says what the tree does, and the disagreement is recorded in chapter 19.

The archived proposals stay as they are: they are the record of how a decision was made, and the
guidelines are where it lives now. Rejected: linking each library's `design.md` instead of writing
its decisions, which would leave the decisions spread over twenty-four files of another
repository, some of them no longer what the tree does.

### D5. What is not built: chapters 16 to 18, and how a decision moves

Chapter 16 is the roadmap, moved from `AGENTS.md` chapter 13. Chapter 17 holds the proposals the
owner accepted and that are not built (asio, crypto, the wasi redesign, the site's increments 4
and 5), each with its decisions; chapter 18 the proposals not decided (beast, webrtc,
json_schema, lexical's strategies, trystero's bounded parse), each briefly, with its open
questions. While a proposal is open, its own `design.md` holds it in full and rules over the
summary in chapters 17 and 18. The change that builds a proposal moves its decisions into chapters
1 to 15 and out of chapters 16 to 18, in the same change, and its archive keeps the record.

Rejected: leaving proposals out of the guidelines until built, which would hide the decisions
already taken (asio's run family, crypto's backends) from the one file a contributor reads;
making chapters 17 and 18 authoritative over an open proposal, which would make every revision of
a proposal a change of two repositories.

### D6. Conflicts are recorded, not resolved

Gathering the decisions found places where a decided proposal, or local work beyond any proposal,
contradicts the rules as `AGENTS.md` states them at `ddb6cb2`: asynchronous code on xactor against
asio's io_context; Boost.Asio on WASIp3 against the rulebook's statement that it does not compile
there and against asio's own non-goal; OpenSSL built from source against "a library webcpp does
not build"; the CA bundle against the submodule rule; and the discrepancies between design
documents and trees. Chapter 19 lists each, with what the rule says, what the work does, whether
the owner approved it and where, and the choices open. The guidelines state each rule as the
superproject's `main` has it; a conflict is resolved by the owner, in the proposal that owns it,
and that proposal changes the rule.

### D7. A library's AGENTS.md, CONTRIBUTING.md and the skills point; they do not restate

A library's `AGENTS.md` links `../../AGENTS.md` and `../../CODING-GUIDELINES.md`, keeps what an
agent needs to work on that library (the original and its version, how its oracle drives it, its
cautions, its commands), and a design decision it states with its reason moves to chapter 15 and
leaves a pointer. `CONTRIBUTING.md` keeps its one-line summary of each rule, which names the
guidelines' chapter: a summary line that names its chapter is a pointer, and `CONTRIBUTING.md`
still says the guidelines rule where the two seem to differ. Each skill cites the chapter of the
file that now holds its rules.

**As implemented (2026-10-26).** The specifications of config, xactor, pratt and wasi, which D9
leaves unchanged, require their `AGENTS.md` to state the library's own rules (config's four
bullets; xactor's, pratt's and wasi's lists of what binds a change). Moving those rules out
would break the specifications, so each library's `AGENTS.md` keeps its content: it links both
files, its introduction says that chapter 15 gives the library's design decisions with their
reasons, and chapter 15 says that a library's `AGENTS.md` keeps the rules its specification
asks of it. A rule and the decision behind it may then be stated in both; where they disagree,
the tree is checked and chapter 19 records it.

### D8. What is updated, and what is history

Updated: every live file of the superproject, the libraries, the site and this repository that
cites the rulebook or one of its chapters (`proposal.md` lists them). Not updated: archived
proposals, recorded patches (`superproject-patches/`), plans (`reference/plan.md`) and reference
trees (`reference/source/`), which record what was true when they were written. An open proposal's
own `proposal.md`, `design.md`, `tasks.md` and specification keep their citations until the
proposal is next revised, as `CONTRIBUTING.md` already says of the commit identity in older plans;
D2's table, archived with this change, maps an old citation to its new home.

### D9. The specifications

The organization's capability changes where it names the rulebook (`specs/organization/spec.md`).
The site's capability changes where its community page links it (`specs/site/spec.md`). The
libraries' specifications cite each library's own `AGENTS.md`, not the rulebook, and xactor's
"points to the superproject's `AGENTS.md`" stays true; none of them changes. The organization's
`## Purpose` names "one rulebook (`AGENTS.md`)"; OpenSpec's archive writes requirements only, so
the archive task updates the purpose's words by hand, as the archive of `agent-skills` did for its
notice.

## Risks / Trade-offs

- **A branch that changes AGENTS.md meets the move.** `work/asio-task9-review` changes chapter 6
  (bringing a C library, OpenSSL) and chapter 2 (Boost's compiled libraries) in its working tree.
  When it is rebased onto `main`, each hunk goes to the chapter's new home (D2's table); the
  conflicts it raises are chapter 19's.
- **A citation the script places wrongly.** Each override of D3 is a line of the script's table,
  and `reference/STATUS.md` lists every citation it rewrote; the review reads the list.
- **The design chapters lag the tree.** A change that alters a decision and does not update
  chapter 15 leaves it stale. Mitigation: the process rule of chapter 1 says a change updates the
  decision it alters, in the same change, and the review checks it; O3 asks whether a tool should.

## Migration Plan

1. The owner decides this proposal (`proposed`, then `accepted`).
2. The superproject: `CODING-GUIDELINES.md` and `AGENTS.md` from `reference/`, then every citation
   of the superproject, the tools' messages and their tests; green.
3. Each library, then the site: its `AGENTS.md`, its pages' citations; green per repository.
4. This repository: `CONTRIBUTING.md`, `AGENTS.md`, the skills, every root's `config.yaml`.
5. The owner's pushes and merges (outward actions are the owner's), then the archive.

## Open Questions

The owner accepted the proposal on 2026-10-26 and answered the three questions as recommended.

- **O1.** Chapter 18 summarises the proposals not decided. Should a draft be in the guidelines at
  all before the owner decides it, or only listed by name and link? **Answer:** a draft keeps its
  short summary in chapter 18, with its open questions; its proposal holds it in full.
- **O2.** Each conflict of chapter 19 is a decision of its own for the owner, in the proposal that
  owns it (asio's, or a new one for the WASIp3, OpenSSL and CA work, which no proposal holds).
  **Answer:** each conflict is decided later, on its own, in asio's proposal or in a new proposal
  for the WASIp3 port, OpenSSL built from source and the CA bundle; this change records them.
- **O3.** Should the lint check that every "chapter N" citation in the superproject, the
  libraries and the site resolves to a heading of the file it names, and that a library's
  `AGENTS.md` links both files? **Answer:** yes, as a change of its own after this one; this
  change adds no lint rule.
