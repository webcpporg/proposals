## Why

The organization's rules fix one personal identity, a name and an e-mail address, for every commit
a maintainer or a maintainer's agent makes, and write that address into the superproject's
`AGENTS.md`, into the organization's specification and into the command every contributor is told
to run. A rule that names one person's address cannot be followed by anyone else without
impersonating them, ties every agent to one mailbox, and keeps personal data in files whose own
rules forbid it. The rule webcpp actually needs is about the form of a commit, not about whose name
is on it: no trailer, explicit paths, a plain English subject.

## What Changes

- The requirement "Superproject: Commit identity and form" is modified: every contributor, human
  or agent, maintainer or external, commits under their own git identity, a name and an e-mail
  address they control (a GitHub noreply address is allowed). No address is fixed by the rules.
- Everything else of the requirement stays: no trailer of any kind (no `Co-authored-by`, no
  `Signed-off-by`, no agent name), explicit paths and never `git add -A`, a plain subject
  prefixed by the part it touches, no last paragraph that git reads as a trailer, English, no em
  dash. Files still hold no personal data.
- The superproject's `AGENTS.md`, chapter 11 "Identity", is rewritten to the same rule, with a
  command that passes the contributor's own identity instead of a fixed one.
- History is not rewritten: existing commits, archived proposals and recorded patches keep the
  identity they were made with.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `organization`: the requirement "Superproject: Commit identity and form" is modified.

## Impact

- **Kind:** the organization (the superproject's rules and the organization's specification).
- **Implementation:** the superproject's `AGENTS.md`, chapter 11, in a pull request to
  https://github.com/webcpporg/webcpp from the branch `contributor-identity`. No tool, lint rule
  or workflow checks the author's address, so none changes; the lint's check of the banned word
  in every commit's author, committer and message stays.
- **Status:** implemented in the 0.1.0 release in preparation; archived on 2026-10-13.
  The implementation is merge commit `9a268b7efa8a9b7eab8810bc024486288c689fab`
  from https://github.com/webcpporg/webcpp/pull/1; see `reference/STATUS.md`.
- **Other proposals:** older proposal reference material may still spell a fixed identity.
  It follows the current specification under design D3. Update commands when an open proposal
  is next revised; preserve archived material and recorded patches. `meta/libraries.json`'s
  `authors` and `maintainers` are not commit identity and remain outside this change (design O2).
