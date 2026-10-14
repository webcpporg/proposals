# Implemented identity rule

Source: webcpp merge commit `9a268b7efa8a9b7eab8810bc024486288c689fab`, `AGENTS.md`, chapter 11.

- **Identity.** Every contributor, human or agent, maintainer or external,
  commits under their own git identity: a name and an e-mail address they
  control, of which a GitHub noreply address
  (`<id>+<login>@users.noreply.github.com`) is one. An agent commits under
  the identity of the account it acts for or of its own account, never under
  another person's; these rules fix no address. Either way, with no trailer
  of any kind (no `Co-authored-by`, no `Signed-off-by`, no agent name):

  ```
  git -c user.name=<name> -c user.email=<address> commit -- <paths>
  ```

  Stage and commit with explicit paths, never `git add -A`. A subject is
  plain: what the change does, prefixed by the part it touches
  (`tools/doc: ...`, `xactor: ...`).
