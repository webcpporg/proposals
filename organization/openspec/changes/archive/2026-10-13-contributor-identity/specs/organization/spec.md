## MODIFIED Requirements

### Requirement: Superproject: Commit identity and form
Every contributor, human or agent, maintainer or external, SHALL commit under their own git identity: a name and an e-mail address they control, of which a GitHub noreply address (`<id>+<login>@users.noreply.github.com`) is one; an agent commits under the identity of the account it acts for or of its own account, never under another person's, and the rules fix no address. Every commit SHALL carry no trailer of any kind (no `Co-authored-by`, no `Signed-off-by`, no agent name), and is made by `git -c user.name=<name> -c user.email=<address> commit -- <paths>` or with that identity configured. Commits MUST stage explicit paths and never `git add -A`; a subject is plain, says what the change does, and is prefixed by the part it touches (`tools/doc: ...`, `xactor: ...`); a last body paragraph MUST NOT start with `word:` since git reads it as a trailer (`std::` does). Every commit message is English, with no em dash. Existing commits are not rewritten to change their identity.

#### Scenario: A commit by an agent
- **WHEN** an agent acting for an account commits a change to `tools/lint`
- **THEN** `git log -1 --format='%an %ae'` prints the name and address of that account's identity and `git log -1 --format=%B | git interpret-trailers --parse` prints nothing

#### Scenario: A commit by an external contributor
- **WHEN** a contributor commits under their GitHub noreply address
- **THEN** the commit is accepted as to its identity, and is reviewed for its form alone

#### Scenario: An accidental trailer
- **WHEN** the last paragraph of a message starts with `std::`
- **THEN** git would read it as a trailer, so the paragraph is rewritten to start otherwise
