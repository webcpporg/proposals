# Contributing a proposal

webcpp grows by proposals. A library, or a change to one, enters webcpp only after its proposal
is accepted here, and it is built from that proposal. This page says what a proposal is, how to
submit one, and how it is decided.

## Read first

1. The organization's genesis, `organization/openspec/changes/archive/*-genesis/` and
   `organization/openspec/specs/`: how webcpp is organized, built, documented, tested and
   released, and what every library must be. A proposal that does not fit it is not accepted.
2. The superproject's [AGENTS.md](https://github.com/webcpporg/webcpp/blob/main/AGENTS.md), the
   rulebook every library follows.
3. One finished proposal of the kind you write: `libraries/lexical/openspec/changes/` holds a
   genesis (implemented) and a change (draft); `libraries/json_schema/openspec/changes/genesis/`
   a new library partly built.

## Kinds of library

| Kind | What it is | What proves it |
| --- | --- | --- |
| `own` | A design of webcpp's own. | Its specification, its tests and the references it cites (RFCs, standards). |
| `port` | A port of another project, faithful to a pinned version of it (`port_of`). | That project, run as an oracle against the port where it can be. |
| `wrapper` | webcpp's interface over another library, or over the platform. | The wrapped library's own behaviour, on every target webcpp builds for. |

## A new library, or a change

- **A new library** is proposed by its genesis: a new directory `libraries/<name>/` with an
  OpenSpec root, and the proposal in `libraries/<name>/openspec/changes/genesis/`, whose
  requirements add a capability named after the library.
- **A change to an existing library** is a proposal in that library's root,
  `libraries/<name>/openspec/changes/<change-id>/`, whose requirements add to or modify the
  library's capability. Its id says what it changes (`shared-connection`, `strategies`).
- A change to the organization itself (the superproject, its build, its rules, the site) is a
  proposal in `organization/openspec/changes/<change-id>/`.

## What a proposal holds

- `proposal.md`: `## Why`, `## What Changes`, `## Capabilities`, `## Impact` (with **Kind**,
  **Implementation** and **Status**).
- `design.md`: the context, goals and non-goals, the decisions with the alternatives rejected,
  the risks, the migration and the open questions.
- `tasks.md`: the work, in groups, each task small enough to review.
- `specs/<capability>/spec.md`: requirements (`### Requirement:`, a SHALL or MUST), each with
  scenarios (`#### Scenario:`, WHEN and THEN).
- `.openspec.yaml`: the metadata the index reads (`library`, `title`, `kind`, `port_of`,
  `status`, `release`, `implementation`, `decided`).
- `reference/`: **the reference code an implementer starts from**, which every proposal carries
  before it is proposed (a prototype, the code of a plan's tasks, or the library's tree), its
  original specification and plan when there are any, and `STATUS.md`.

Everything is written in English, without the em dash. A proposal names no person but by the
identity they publish under, and holds no private data.

## How a proposal is decided

1. **Open a pull request** that adds the proposal, with status `draft` while it is written and
   `proposed` when it is ready, and the link to the repository where its implementation lives or
   will live. The CI checks it (`tools/proposals.py check` and OpenSpec's strict validation).
2. **Discussion** happens in public, on the pull request.
3. **Decision.** The maintainers decide whether a proposal is accepted, after its public
   discussion; an accepted proposal is merged with status `accepted` and its date in `decided`.
   A rejected or withdrawn one is archived with that status, so the reasons stay readable.
4. **Implementation.** The library is built from the proposal, in its own repository, under the
   superproject's rules; the proposal's status becomes `implementing`, and `implemented`, with
   the release it entered, when it is merged into the superproject and released. The maintainers
   then archive it, which writes the library's specification under `specs/`.

## Commit identity in older plans

Every contributor follows the current superproject `AGENTS.md`, chapter 11, using the identity
of the account they act for or their own account. An older plan's fixed identity is historical
context, not an instruction to impersonate its author. Update that command when the open proposal
is next revised. Existing commits, archived proposals and recorded patches keep their original
identity; no bulk rewrite is required.

## Grants and the DAO

webcpp funds work through grants governed by its members. The governance, its rules and the
platform it runs on are described at webcpp.org/dao, which a future proposal of the
organization specifies. Until then, proposals are decided by the maintainers as above.

## Checking a proposal locally

```
npm install --global @fission-ai/openspec@1.10.0
cd libraries/<name> && openspec validate --all --strict --no-interactive
cd ../.. && python3 tools/proposals.py readme && python3 tools/proposals.py check
```
