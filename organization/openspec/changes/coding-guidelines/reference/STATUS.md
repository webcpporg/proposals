# coding-guidelines: where the proposal stands, and how to resume

Written 2026-10-26, when the proposal and its reference were made.

## Where it stands

The owner accepted the proposal on 2026-10-26, with the answers to O1 to O3 in `../design.md`,
and asked for tasks 1 to 4 without a push. The proposal is committed on the branch
`coding-guidelines` of this repository; the implementation's state is below, under
"Implementation".

| What | Where |
| --- | --- |
| The proposal | `../proposal.md`, `../design.md` (D1 to D9, O1 to O3), `../tasks.md` |
| The changed requirements | `../specs/organization/spec.md` (one added, eight modified), `../specs/site/spec.md` (one modified) |
| The guidelines, as the superproject will hold them | `CODING-GUIDELINES.md`, 8,893 lines, nineteen chapters |
| `AGENTS.md`, as the superproject will hold it | `AGENTS.md`, 1,817 lines, six chapters |
| The split of the rulebook | `split_rulebook.py`, run on `AGENTS.md` at `ddb6cb2` |
| Every citation the split rewrote | `citations.txt`: the old line, the old citation, the line's new home and the new citation |

## Implementation (2026-10-26)

Tasks 1 to 4 are done and committed locally, each repository on its branch `coding-guidelines`;
nothing is pushed, by the owner's instruction, and no CI ran.

| Repository | Commits on `coding-guidelines` |
| --- | --- |
| webcpp | `0166320` the two files; `04872bb` `CONTRIBUTING.md`, `README.md`, `doc/index.adoc` and the five skills; `72076aa` the comments and messages of the Jamroot and the tools |
| config | `279eed7` `AGENTS.md` and `doc/version.adoc` |
| xactor, xstate, pratt, wasi, idna, lexical, document, trystero | `3062eb0`, `eb96163`, `0c243d6`, `b702fef`, `19bf135`, `7b1032a`, `951e4e1`, `b28ccc9`: each `AGENTS.md` |
| webcpporg.github.io (the site) | `b7e2fe8` `AGENTS.md`, the community page and its test |
| proposals | the commit that records this section: `CONTRIBUTING.md`, `AGENTS.md`, three skills and every root's `openspec/config.yaml` |

The superproject's branch does not move its submodules: each library's and the site's new commit
is recorded by the superproject once it is merged in that repository (task 2.10).

**Design D7, as implemented.** The specifications of config, xactor, pratt and wasi require
their `AGENTS.md` to state the library's own rules, and D9 leaves them unchanged, so no
library's `AGENTS.md` loses content: each links both files and says that chapter 15 gives its
design decisions, and chapter 15 says a library's `AGENTS.md` keeps the rules its specification
asks of it (`../design.md`, D7).

**Verified, locally, on macOS arm64.**

- `tools/lint/lint.sh` with wasi-sdk 34's clang-format and clang-tidy, over the superproject and
  every library on its branch: `lint: clean`, clang-tidy over 342 translation units.
- Every `tools/**/*_test.py`, each as `python3 <path>`: 32 of 32 pass, among them
  `tools/ci/matrix_test.py`, which reads `AGENTS.md`.
- `b2 -a doc`: 97 targets, the index linking `CODING-GUIDELINES.md`.
- `b2 -a /webcpp/site//test`: the new test failed first (96 of 97), then 97 of 97 with the
  page changed; the target runs the site's typecheck too.
- In this repository: `openspec validate --all --strict --no-interactive` in all fifteen roots,
  `tools/proposals.py readme`, `tools/proposals.py check` and `tools/proposals_test.py`.
- No test pinned the citations the tools' messages and comments carry, so none was written
  first for them; the lint and the tools' tests pass with them changed.

**The checkout, for the record.** `libs/asio` in the superproject's main checkout is an older
local copy of the library, not a submodule and ignored by git, whose `build.jam` names a feature
only the asio branch defines: with it there, every b2 command of that checkout fails. It was
moved to `.local/` for the runs above and moved back after them, unchanged. The external
libraries trystero needs were linked into that checkout's `.local/` from the earlier checkout's,
as its other toolchains already were.

**Left:** 1.7's and 2.10's CI, the owner's pushes and merges in the migration plan's order
(each library and the site, then the superproject with its submodules moved, then this
repository), and the archive (task 5).

## How the two files were made

1. **The split (design D2, D3).** `python3 split_rulebook.py <AGENTS.md at ddb6cb2> <dir>` cuts
   the rulebook by chapter and section into the body of each file and rewrites every citation:
   102 lines, none it could not place. A paragraph that a citation made longer than 78 columns
   is wrapped again. Four sentences that changed file were edited (its table `FIXES`): the
   process rule on the tests of `tools/`, the checklist's two citations of chapters 9 and 8,
   the link a library's `AGENTS.md` gives, and the layout's line for `AGENTS.md`, beside which
   the layout gains `CODING-GUIDELINES.md`.
2. **The introductions** of both files, the pointers left where chapter 7's rules and chapter
   9's "Tests" were, chapter 9's section on `.clang-format` and `.clang-tidy` (from the two
   files' own comments), and chapter 1's new rule that a change updates the decision it alters
   were written for this proposal.
3. **The design chapters (D4) and chapters 17 and 18 (D5)** were written by reading, library by
   library: the archived and open `design.md`, the specification as implemented, the reference
   specifications and plans where a doubt needed them, each library's `AGENTS.md`, its pages
   and its headers, at the commits the superproject's `ddb6cb2` pins; asio at its local branch
   `work/task14-guide` (`f5fd7a8` and the uncommitted tree) and the superproject branch
   `work/asio-task9-review` with its uncommitted work. Each bullet names its source. The
   organization's bullet on the rulebook was brought to this proposal's decision.
4. **Chapter 19 (D6)** gathers what the reading found against the rules and against the trees,
   with asio's twenty-five conflicts, each cited by the chapter of its rule.

## What was verified

- Every paragraph of `AGENTS.md` at `ddb6cb2` is in one of the two files once citations are
  read alike, but for the four edits above, the renumbered headings and the layout's lines that
  a citation made too long, which were wrapped again.
- Every "chapter N" of either file names a chapter that exists in the file it names.
- Neither file holds the em dash or the banned word; `split_rulebook.py` parses.
- Four findings of chapter 19 were checked again against the tree: lexical's path in chapter 1's
  table, the lint rule missing from chapter 9's table, trystero's `config.hpp` against chapter
  16, and xstate's `.at()` against its guarantee 15. The others are as the reading recorded
  them, each with its path.
- `openspec validate --all --strict --no-interactive` in `organization/`,
  `tools/proposals.py readme`, `tools/proposals.py check` and `tools/proposals_test.py`.

## What was not done

- Tasks 1 to 5 when the proposal was written: they are recorded under "Implementation".
- The design chapters were not reviewed line by line by a second reader against their sources.
- The lint of the superproject was not run on the two files, which are Markdown: its rules that
  read Markdown are the em dash and the banned word, both checked above.

## How to resume

1. The decision is taken (2026-10-26).
2. In a worktree of the superproject from `main`, check that `AGENTS.md` is still `ddb6cb2`'s;
   if it changed, run `split_rulebook.py` on the new one, carry the change into the two files,
   and check `citations.txt` again. Then copy the two files to the root and follow `../tasks.md`
   from group 1.
3. A branch that changes `AGENTS.md` meanwhile (`work/asio-task9-review` does) carries each hunk
   to its new home by design D2's table when it is rebased.
