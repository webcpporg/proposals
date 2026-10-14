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
is short; O1 below requires an explicit local comparison when either copy changes.

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
like `AGENTS.md` and `README.md`, carries no webcpp source-file notice. Chapter 11 now
explicitly names this exemption. Webcpp-authored skills are BSL-1.0; generated OpenSpec
skills retain MIT and the copyright and permission notice in `THIRD_PARTY_NOTICES.md`.

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
change. Both repositories also run `.github/workflows/skills.yml` on pull requests and main:
PyYAML 6.0.3 reads actual YAML and rejects malformed front matter, a missing or blank description,
and a name that differs from its kebab-case directory. The parser is installed in an isolated
virtual environment. This catches the unquoted colon descriptions found during review.

## Risks / Trade-offs

- [A skill goes stale when a chapter is renamed] -> skills cite chapters by number and title, and a
  rule change that touches a chapter a skill cites updates the skill in the same commit (O2); prose references are checked in review.
- [OpenSpec's generated text carries the MIT licence in a BSL-1.0 repository] -> it is generated
  documentation, kept with its front matter and full upstream notice; O4 records the exception.

## Migration Plan

1. Merge both pull requests; nothing else moves.
2. On archive, the requirement joins `specs/organization/spec.md`.

## Resolved Questions

Decided on 2026-10-13 under the owner's instruction to resolve O1 to O5, finish the implementation,
merge webcpp PR #2 and archive this proposal. These are the decisions taken for that request.

- **O1. Local comparison, no cross-repository network gate.** Keep the two
  `webcpp-text-and-commits` files identical. When either changes, compare the two checked-out
  files with `cmp` and update both in the same change. CI must not depend on the changing head
  of another repository or on a particular checkout layout. This review compared both files.
- **O2. Validate metadata in CI; review prose references.** Each repository runs the Agent skills
  workflow with a real YAML parser, checking delimiters, a mapping, a matching kebab-case name
  and a nonempty string description. Chapter references remain a review responsibility: they
  appear in prose, not a machine-readable reference field. A rule change updates affected
  skills in the same change. All references in the five superproject skills were reviewed.
- **O3. Shared skills only.** The superproject and proposals skills suffice for this change.
  A library keeps its specific rules in its own `AGENTS.md`. A future unique recurring workflow
  may justify a library skill; do not copy these shared skills into each library.
- **O4. Retain upstream MIT documentation.** Keep the six generated OpenSpec skills and their
  upstream metadata unchanged. `THIRD_PARTY_NOTICES.md` preserves OpenSpec 1.10.0's complete
  copyright and permission notice. The organization's specification explicitly exempts this
  generated documentation from its otherwise BSL-1.0-only rule; webcpp-authored text stays BSL-1.0.
- **O5. State the Markdown exemption.** Chapter 11 now explicitly says that Markdown files,
  including `SKILL.md`, need no webcpp source-file notice. Upstream notices are still preserved.
  The licence lint's suffix rules do not change.

## Review fixes

- Quote the descriptions of all custom skills as YAML strings. Four superproject descriptions
  and two proposals descriptions originally failed to parse because they contained `: `.
- Unset `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` once before the build example block,
  so every command runs in the required environment.
- Add the metadata workflow in both repositories. Its exact script was tested against real
  skills and fixtures for invalid YAML, wrong names, absent or blank descriptions, missing
  delimiters and non-mapping front matter.

The contributor identity rule is already in the superproject's main through webcpp PR #1;
the text skill now follows the current rulebook without waiting on a separate implementation.
