# Tasks

## 0. The proposal

- [x] 0.1 Write the proposal, the design, the tasks and the modified requirement; validate with `openspec validate --all --strict` and `tools/proposals.py check`
- [x] 0.2 Record O1 to O4 as archival decisions preserving the already merged scope, under the owner's request
- [x] 0.3 Record acceptance through the merged implementation PR and the owner's instruction to archive; the intermediate `proposed` step is superseded

## 1. The superproject

- [x] 1.1 `AGENTS.md`, chapter 11 "Identity": every contributor commits under their own git identity, a noreply address allowed; the command passes the contributor's identity; no trailer, explicit paths, unchanged
- [x] 1.2 Check that no tool, lint rule or workflow checks the author's address (`git grep` of the old address and of `user.email` in `tools/` and `.github/`)
- [x] 1.3 The superproject's pull request reviewed and merged by the owner

## 2. After the decision

- [x] 2.1 Record D3's ongoing migration policy in `CONTRIBUTING.md`: update old commands when each open proposal is next revised; no bulk rewrite is required
- [x] 2.2 Archive the change, which writes the modified requirement into `specs/organization/spec.md`
