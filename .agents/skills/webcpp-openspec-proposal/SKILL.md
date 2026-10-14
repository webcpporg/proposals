---
name: webcpp-openspec-proposal
description: Write or review a webcpp proposal: a library genesis, a change to a library, or an organization change, with its metadata, statuses and checks.
---

# Writing a webcpp proposal

Read first: `CONTRIBUTING.md` (what a proposal holds, how it is decided), `AGENTS.md`, the
organization's specification `organization/openspec/specs/`, and the superproject's `AGENTS.md`.
The generic OpenSpec skills (`openspec-propose`, `openspec-explore`, ...) apply, with the note at
their top: one OpenSpec root per library and one for the organization.

## Steps
1. Pick the root: `libraries/<name>/` (a new library starts with `openspec/config.yaml` copied from
   another library, its context naming the library) or `organization/`.
2. `cd <root> && openspec new change <id>`; a new library's id is `genesis`; a change's id says
   what it changes (`shared-connection`).
3. Write `.openspec.yaml`'s `webcpp:` block (keys in `tools/proposals.py`, `METADATA_KEYS`),
   status `draft`.
4. `proposal.md`: `## Why`, `## What Changes`, `## Capabilities`, `## Impact` with **Kind:**,
   **Implementation:**, **Status:**.
5. `design.md`: context, goals and non-goals, decisions D1.. with the alternatives rejected,
   risks, migration, open questions O1.. for the owner.
6. `specs/<capability>/spec.md`: `## ADDED|MODIFIED Requirements`, `### Requirement:` with SHALL or
   MUST, each with `#### Scenario:` (WHEN, THEN). A MODIFIED requirement copies the whole text.
7. `tasks.md`: groups of small, reviewable tasks; tests first.
8. Past `proposed`, `reference/` carries code: skill `webcpp-reference-status`.
9. For a port, read the superproject's skill `webcpp-port-methodology`.

## Check
```
cd <root> && openspec validate --all --strict --no-interactive
cd <repository root> && python3 tools/proposals.py readme && python3 tools/proposals.py check && python3 tools/proposals_test.py
```
Text rules: skill `webcpp-text-and-commits`.
