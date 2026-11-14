# Tasks

## 1. The implementation (merged on 2026-11-14)

- [x] 1.1 The superproject's Jamroot and `tools/boost/Jamfile` declare no Cobalt target, and `tools/test/cobalt_ownership_test.py` checks that a program naming a removed name stops (webcpp#13, `eb08198`)
- [x] 1.2 Asio declares no Cobalt target (asio#3), and Cobalt keeps its four targets under `/webcpp/cobalt` (cobalt#1)
- [x] 1.3 Cobalt's and Asio's genesis proposals record that each Cobalt target has one name (proposals#9)

## 2. The organization's specification

- [x] 2.1 Write the proposal, the design, these tasks and the four modified requirements
- [x] 2.2 Check each modified requirement against the Jamroot and `tools/boost/Jamfile` at webcpp `eb08198`, and record the evidence in `reference/STATUS.md`
- [x] 2.3 Validate with `openspec validate --all --strict --no-interactive`, `tools/proposals.py readme`, `tools/proposals.py check` and `tools/proposals_test.py`
- [x] 2.4 Archive the change, which writes the modified requirements into `specs/organization/spec.md`
