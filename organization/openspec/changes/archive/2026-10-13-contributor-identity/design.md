## Context

The organization's genesis (`changes/archive/2026-10-07-genesis/`) fixed one identity for every
commit of a maintainer and of the maintainer's agents, and the specification
(`specs/organization/spec.md`, "Superproject: Commit identity and form") and the superproject's
`AGENTS.md`, chapter 11, carry it with its address and the command that sets it. External
contributors already commit under their own name and address. The other parts of the rule, the
form of a commit, are what the lint and the reviews rely on.

## Goals / Non-Goals

**Goals:**

- One rule for every contributor: commit under your own git identity.
- Keep every rule about the form of a commit and about personal data in files.
- Remove the fixed address from the rules a contributor reads and runs.

**Non-Goals:**

- Rewriting history: no commit is amended, rebased or re-signed, and no archived proposal or
  recorded patch is edited.
- Changing `meta/libraries.json`'s `authors` and `maintainers` (O2 below).
- Requiring signed commits (O3 below).

## Decisions

### D1. Each contributor commits under their own identity

A contributor, human or agent, commits under a name and an e-mail address they control; a GitHub
noreply address (`<id>+<login>@users.noreply.github.com`) is allowed and is the suggested choice
for anyone who does not want to publish an address. An agent commits under the identity of the
account it acts for or of its own account, never under another person's.

Rejected: keeping one fixed identity for the maintainer and the maintainer's agents. It cannot be
followed by a second maintainer or by an agent of another account without impersonation, and it
keeps a personal address in the rules.

Rejected: a shared organization identity (such as `webcpp <noreply@webcpp.org>`). It hides who made
a change, which the review and the history need, and the organization has no mailbox behind it.

### D2. The form of a commit is unchanged

No trailer of any kind, explicit paths, a plain subject prefixed by the part it touches, no last
paragraph that starts with `word:`, English, no em dash. These are about the history's
readability and are independent of whose name is on a commit.

### D3. History and recorded material are not rewritten

Existing commits keep their author and committer. Archived proposals, the organization's genesis
reference and the patches recorded under `reference/` of open proposals keep the identity they
were made with: they are records of what was done, and changing them would make them disagree with
the commits they describe. Plans of open proposals that spell the old command are read under the
new rule once it is accepted; each is updated when its own proposal is next revised.

Rejected: a sweep that edits every occurrence of the old address in the repository. It would
rewrite records, touch proposals this change does not own, and gain nothing a reader of the rule
needs.

### D4. No tool checks the author's address

The superproject's tools check no author address (the lint checks only the banned word in each
commit's author, committer and message, and the tests commit under their own throwaway
identities), so the change is to the rules alone.

## Risks / Trade-offs

- [A contributor commits with a machine's default identity, such as an employer's address] ->
  `AGENTS.md` tells them to pass their identity explicitly, and the reviewer checks the author of
  a pull request's commits.
- [Agents of one account commit under different identities] -> the rule names the account an agent
  acts for; the owner may later fix one identity per agent account in that account's own
  configuration, outside the repositories.

## Migration Plan

1. Accept this change and merge the superproject's `AGENTS.md` pull request.
2. From then on, every new commit follows the new rule; nothing else is migrated.
3. On archive, the requirement's new text replaces the old one in `specs/organization/spec.md`.

## Archival decisions

Recorded on 2026-10-13 when the owner requested verification and archival if implemented,
without running CI. These decisions preserve the scope already merged in webcpp PR #1;
they do not introduce new implementation requirements.

- **O1. No fixed address in the current commit rule.** The merged chapter 11 uses placeholders
  and each contributor's own identity. History, archived material and recorded patches retain
  their original identity as D3 requires. A maintainer contact in library metadata is a different
  concern, addressed by O2.
- **O2. Library metadata remains outside this change.** Keep `authors` and `maintainers` as they
  are. Changing contact fields across libraries would need a separate proposal; this archive
  makes no new choice of personal, noreply or organization contact.
- **O3. No signing requirement is introduced.** The merged rule does not require signed commits.
  No GitHub settings are changed by this proposal.
- **O4. No new CI check.** Commit form remains a review responsibility. Adding enforcement of
  the no-trailer rule is outside this documentation change. Existing lint behavior is unchanged,
  and no CI is executed for this archive, as the owner requested.

## Migration completion

D3 is the ongoing policy, not a bulk migration gate: an open proposal's old command is read
under the current rulebook and is updated when that proposal is next revised. This policy is
now also recorded in `CONTRIBUTING.md` so it remains visible after archival. No unrelated plan,
archived proposal, recorded patch or existing commit was rewritten.
