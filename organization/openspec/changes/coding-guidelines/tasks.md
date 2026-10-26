# Tasks

## 0. The proposal

- [x] 0.1 Write the proposal, the design, the tasks and the changed requirements of `organization` and `site`; validate with `openspec validate --all --strict` and `tools/proposals.py check`
- [x] 0.2 `reference/`: `CODING-GUIDELINES.md` and `AGENTS.md` as the superproject will hold them, made by `split_rulebook.py` from `AGENTS.md` at `ddb6cb2`, with the design chapters gathered and checked against each tree (design D3, D4), and `STATUS.md` with the citations rewritten
- [x] 0.3 The owner's decision: `proposed`, then `accepted` with its date in `decided`, and the answers to O1 to O3 in `design.md`

## 1. The superproject (branch `coding-guidelines` from `main`)

- [x] 1.1 `CODING-GUIDELINES.md` and `AGENTS.md` from `reference/`, checked again against `AGENTS.md` at `main` for any change after `ddb6cb2` (design D3)
- [x] 1.2 `CONTRIBUTING.md`: the rulebook is the two files; each summary line names its guidelines chapter (design D7)
- [x] 1.3 `README.md` and `doc/index.adoc`: the rules are `CODING-GUIDELINES.md`
- [x] 1.4 The five skills under `.agents/skills/`: each cites the chapter of the file that now holds its rules
- [x] 1.5 Comments of `Jamroot`, `tools/webcpp.jam`, `tools/ci/matrix.py`, `tools/test/harness.py`, `tools/test/boost_libraries_test.py` and `tools/release/release.py` that cite a moved chapter
- [x] 1.6 Tests first: the lint's changelog message and `tools/release/changelog.py`'s docstring cite `AGENTS.md` chapter 5; see `tools/lint/lint_test.py` and `tools/release/changelog_test.py` fail on the old citation where they pin it, then change `tools/lint/lint.sh`, `tools/lint/rules.py`, `tools/release/changelog.py`, `tools/release/scratch.py` and `tools/release/release.py`
- [ ] 1.7 Green: `lint: clean`; every `tools/**/*_test.py`; `b2 -a doc`; the superproject's CI

## 2. The libraries (one branch `coding-guidelines` in each repository)

- [x] 2.1 config: `AGENTS.md` links both files (design D7); `doc/version.adoc`'s citation; lint clean
- [x] 2.2 xactor: `AGENTS.md` links both files, and its introduction says chapter 15 gives its design decisions (design D7, as implemented: a library's own rules stay, as its specification asks); lint clean
- [x] 2.3 xstate: likewise
- [x] 2.4 pratt: likewise
- [x] 2.5 wasi: likewise
- [x] 2.6 idna: likewise
- [x] 2.7 lexical: likewise
- [x] 2.8 document: likewise
- [x] 2.9 trystero: likewise
- [ ] 2.10 Each library's CI green, and the superproject's `main` pointing to each

## 3. The site

- [x] 3.1 `site/AGENTS.md` tells the reader to read the superproject's `CODING-GUIDELINES.md` and `AGENTS.md` first
- [x] 3.2 Tests first: the community page links `CODING-GUIDELINES.md` (the rules) beside `CONTRIBUTING.md` and `AGENTS.md`; the site's tests and checks green

## 4. This repository

- [x] 4.1 `CONTRIBUTING.md`: "Read first" names `CODING-GUIDELINES.md`; a note maps old chapter citations of open proposals through design D2 (design D8)
- [x] 4.2 `AGENTS.md` and the skills `webcpp-openspec-proposal`, `webcpp-reference-status` and `webcpp-text-and-commits` cite the new chapters; the shared text skill stays identical to the superproject's
- [x] 4.3 Every root's `openspec/config.yaml`: its context names `CODING-GUIDELINES.md` as the rules and `AGENTS.md` as how to work
- [x] 4.4 `openspec validate --all --strict` in every root, `tools/proposals.py readme`, `tools/proposals.py check` and `tools/proposals_test.py`

## 5. After the decision

- [ ] 5.1 The owner's pushes and merges, in the order of the migration plan
- [ ] 5.2 Archive the change, which writes the requirements into `specs/organization/spec.md` and `specs/site/spec.md`; update the organization's `## Purpose` by hand (design D9)
