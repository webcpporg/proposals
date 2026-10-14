## Context

Coding agents read `AGENTS.md` at a repository's root, and several of them also load skills: a
directory per skill with a `SKILL.md` whose front matter (`name`, `description`) says when it
applies. `.agents/skills/` is the vendor-neutral place, and OpenSpec 1.10.0 writes its own skills
there with `openspec init --tools agents`. The proposals repository has several OpenSpec roots
(`organization/openspec/` and `libraries/<name>/openspec/`) and no root `openspec/`.

## Goals / Non-Goals

**Goals:**

- A skill per recurring task, in the repository whose rules it applies.
- Skills that point to the rules (chapter and section) and give the order of the work, so that a
  rule is written once.
- The checks that already guard every file guard the skills too.

**Non-Goals:**

- Moving a rule out of `AGENTS.md`, `CONTRIBUTING.md` or the specification into a skill.
- Configuration of any one agent product (`.claude/`, `.cursor/`): the neutral layout only.
- Skills in each library's repository (O3).

## Decisions

### D1. The layout `.agents/skills/<name>/SKILL.md`

Each skill is a directory named in kebab case with a `SKILL.md` that opens with YAML front matter
holding `name` (equal to the directory) and `description` (when the skill applies), then a short
Markdown body. Rejected: one directory per agent product, which would duplicate every skill.

### D2. A skill points to the rules and does not restate them

A skill names the chapter or the file that holds a rule and lists the steps and commands of the
task; the rule's text stays where it is, and the rules win where a skill disagrees. Rejected: skills
that copy the rules, which would drift from `AGENTS.md` at the first change of a rule.

### D3. One text skill, written twice

`webcpp-text-and-commits` applies to both repositories and is a file in each, identical. Rejected:
a single source fetched or linked from the other repository: a symbolic link across repositories
does not exist in a clone, and an agent reads the files of the repository it is in. The duplicate
is short; O1 asks whether a check should keep the two equal.

### D4. OpenSpec's skills, generated and adapted

`openspec init --tools agents` (1.10.0), run in a copy of the repository, writes the six skills and
`.agents/skills/.openspec-target`, and also creates `openspec/config.yaml` at the root, which this
repository must not have: the copy showed that the skills assume one root. The generated skills
are therefore committed with two changes made by hand, and `init` is never run in the repository
itself:

- a note at the top of each: there are several roots, every `openspec` command runs from the root
  of the change (`openspec new change` and `openspec status` were seen working from
  `libraries/<name>/`), and webcpp's metadata and checks apply on top;
- every em dash replaced by a plain dash, since `tools/proposals.py check` refuses it.

The front matter OpenSpec writes, `license: MIT` and its `metadata` included, is kept, so that the
origin of the text stays visible. Updating them is running `init` again in a copy and reapplying
the two changes.

### D5. No licence notice on a skill

The licence notice of the superproject's `AGENTS.md`, chapter 11, applies to source files by
suffix, and `.md` is none of them (the lint's rule `licence notice` reads the same list); a skill,
like `AGENTS.md` and `README.md`, carries none. The repository's licence covers it.

### D6. A pointer in each `AGENTS.md`

The superproject's `AGENTS.md` names the skills in its introduction. The proposals repository had
no `AGENTS.md`; it gains a short one, in the form every library's has (read the superproject's
first, then what is specific here), rather than a paragraph in `CONTRIBUTING.md`, which is
written for people proposing a library.

### D7. The checks

`tools/proposals.py check` already scans every file but `.git` and `node_modules`, so a skill
holding forbidden text fails it; a test in `tools/proposals_test.py` pins that for
`.agents/skills/`. The superproject's lint reads every tracked file (`git ls-files --cached
--others --exclude-standard`) for the em dash and the banned word, so it covers `.agents/` without
change.

## Risks / Trade-offs

- [A skill goes stale when a chapter is renamed] -> skills cite chapters by number and title, and a
  rule change that touches a chapter a skill cites updates the skill in the same commit (O2).
- [OpenSpec's generated text carries the MIT licence in a BSL-1.0 repository] -> it is generated
  documentation, kept with its front matter naming its licence; O4 asks the owner to confirm.

## Migration Plan

1. Merge both pull requests; nothing else moves.
2. On archive, the requirement joins `specs/organization/spec.md`.

## Open Questions

- **O1.** Should a check (the superproject's lint or `tools/proposals.py`) refuse the two copies of
  `webcpp-text-and-commits` when they differ?
- **O2.** Should the lint check that every `SKILL.md` has front matter whose `name` equals its
  directory, and that every `AGENTS.md` chapter a skill cites exists?
- **O3.** Should each library's repository carry skills of its own (for instance xstate's oracle),
  or do the superproject's suffice?
- **O4.** Is OpenSpec's generated text under MIT acceptable in the repository, or should webcpp
  write its own OpenSpec skills instead?
- **O5.** Should `AGENTS.md`, chapter 11's licence notice list, name `SKILL.md` explicitly as a
  file without a notice?
