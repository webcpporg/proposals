---
name: webcpp-reference-status
description: Prepare or read a proposal's reference/ directory (plan, source, STATUS.md), report task progress, or resume an interrupted implementation.
---

# reference/ and STATUS.md

What `reference/` holds is in `CONTRIBUTING.md` ("What a proposal holds"); when code is required
is `WITHOUT_CODE` in `tools/proposals.py`. Models:
`libraries/json_schema/openspec/changes/genesis/reference/` (partly built) and
`libraries/xstate/openspec/changes/archive/*-genesis/reference/` (implemented).

## Layout
- `plan.md`: every task with its files, interfaces, steps, code and expected output; `tasks.md`
  cites it (`plan: Task 7, Step 2`).
- `source/`: the library's tree, or the first tree a plan starts from; `source-wip/` uncommitted work.
- `spec.md`, `design-source.md`, `research/`, `spike.md`: the original specification and the
  measurements behind decisions.
- `STATUS.md`: the date, where the work stopped task by task, what to redo, "How to resume".

## Progress of every open change
```
for t in $(find . -name tasks.md -not -path '*/archive/*'); do
  echo "$t $(grep -c -- '- \[x\]' "$t")/$(grep -cE -- '- \[( |x)\]' "$t")"; done
```
Then read each `.openspec.yaml` (status, decided), the first unchecked tasks and `STATUS.md`.

## Rules
- A box is checked only for work done and committed.
- Recorded patches and archived material are history: never edit them to change what they record.
- A repository made from `reference/source/` starts from one import commit (superproject `AGENTS.md`, chapter 11).
