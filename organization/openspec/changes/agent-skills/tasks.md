# Tasks

## 0. The proposal

- [x] 0.1 Run `openspec init --tools agents` (1.10.0) in a copy of the repository and record what it writes and what does not fit (design D4)
- [x] 0.2 Write the proposal, the design, the tasks and the added requirement; validate with `openspec validate --all --strict` and `tools/proposals.py check`
- [x] 0.3 Resolve O1 to O5 under the owner's delegated decision and record the answers in `design.md`
- [x] 0.4 Record the owner's decision to finish, merge and archive on 2026-10-13; the intermediate `proposed` step is superseded by this decision

## 1. The proposals repository

- [x] 1.1 `.agents/skills/openspec-*`: OpenSpec's six skills and `.openspec-target`, each with the note on several roots and without the em dash
- [x] 1.2 `.agents/skills/webcpp-openspec-proposal`, `webcpp-reference-status`, `webcpp-text-and-commits`
- [x] 1.3 `AGENTS.md`: read the superproject's first; the roots; the checks; the skills
- [x] 1.4 `tools/proposals_test.py`: forbidden text in a skill is found

## 2. The superproject

- [x] 2.1 `.agents/skills/webcpp-library-conventions`, `webcpp-port-methodology`, `webcpp-build-and-lanes`, `webcpp-docs-site-release`, `webcpp-text-and-commits`
- [x] 2.2 `AGENTS.md`'s introduction names the skills
- [x] 2.3 Check that the lint's em dash and banned word rules read `.agents/` (every tracked file) and that the licence rule does not apply to `.md`
- [ ] 2.4 The lint run green on the branch (`lint: clean`), by CI

## 3. After the decision

- [ ] 3.1 Both pull requests reviewed and merged by the owner
- [ ] 3.2 Archive the change, which writes the requirement into `specs/organization/spec.md`

## 4. Review corrections

- [x] 4.1 Fix and parse every custom skill's YAML front matter in both repositories
- [x] 4.2 Make every build-example command use the required clean environment
- [x] 4.3 Add and locally validate the Agent skills workflow, including invalid-input fixtures
- [x] 4.4 Compare the shared text skill in both repositories and review the chapter references
- [x] 4.5 Preserve OpenSpec's full MIT notice and record the specification's licence exception
- [x] 4.6 Clarify the Markdown notice exemption in chapter 11
