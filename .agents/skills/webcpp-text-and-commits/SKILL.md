---
name: webcpp-text-and-commits
description: "Before writing any file, commit message or proposal text for a webcpporg repository: the rules on language, forbidden text, commit form and outward actions."
---

# Text and commits in webcpp

The rules are in the superproject's `CODING-GUIDELINES.md`, chapter 2 "Commits and text"
(https://github.com/webcpporg/webcpp/blob/main/CODING-GUIDELINES.md), and a changelog's form in its
`AGENTS.md`, chapter 5. This is a checklist, not a copy.

## Before writing a file
1. English only, in files, comments and commit messages.
2. No em dash (U+2014): use a plain dash. No banned word (the regex `veru[s]`, any case).
3. No personal data: no home directory paths (write `/Users/you/` as the placeholder), no
   private addresses. Name people only by the identity they publish under.
4. A source file opens with the licence notice of chapter 2, in its language's comment form.

## Before committing
1. Commit under your own git identity, a name and address you control (a GitHub noreply address
   is fine), as chapter 2 says; never under another person's identity.
2. No trailer of any kind: no `Co-authored-by`, no `Signed-off-by`, no agent name.
3. Stage explicit paths, never `git add -A`; a submodule's commit is recorded, never its files.
4. Subject: plain, says what the change does, prefixed by the part it touches (`xactor: ...`).
5. Check: `git log -1 --format=%B | git interpret-trailers --parse` prints nothing. A last paragraph
   that starts with `word:` (`std::` does) is read as a trailer: start it otherwise.
6. A change a user can see adds its changelog line in the same commit (`AGENTS.md` chapter 5,
   "The changelog").

## Outward actions
Pushing, merging, creating repositories, changing settings and touching credentials belong to the
owner, unless the owner asks for that specific action. An outside change is a pull request from a fork.
