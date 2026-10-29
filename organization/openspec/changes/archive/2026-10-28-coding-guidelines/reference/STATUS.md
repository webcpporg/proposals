# coding-guidelines: where the proposal stands

Written 2026-10-26, when the proposal and its reference were made; brought up to date on
2026-10-28 (America/New_York), when the change was merged everywhere and archived.

## Where it stands

The owner accepted the proposal on 2026-10-26, with the answers to O1 to O3 in `../design.md`,
and asked for tasks 1 to 4 without a push; on 2026-10-27 the owner asked for the pushes, each
repository's CI, the superproject's pointers and the archive. Every pull request is merged, all
tasks are checked, and the change is archived with status `implemented`, release `0.1.0` and
the superproject's merge commit `a4c770e`. As in the index, 0.1.0 is the release in
preparation, which `<webcpp/version.hpp>` names, not a published tag. The merges are below,
under "Merged".

| What | Where |
| --- | --- |
| The proposal | `../proposal.md`, `../design.md` (D1 to D9, O1 to O3), `../tasks.md` |
| The changed requirements | `../specs/organization/spec.md` (one added, eight modified), `../specs/site/spec.md` (one modified) |
| The guidelines, as the superproject will hold them | `CODING-GUIDELINES.md`, 8,893 lines, nineteen chapters |
| `AGENTS.md`, as the superproject will hold it | `AGENTS.md`, 1,817 lines, six chapters |
| The split of the rulebook | `split_rulebook.py`, run on `AGENTS.md` at `ddb6cb2` |
| Every citation the split rewrote | `citations.txt`: the old line, the old citation, the line's new home and the new citation |

## Implementation (2026-10-26)

Tasks 1 to 4 were done and committed locally, each repository on its branch
`coding-guidelines`, before any push, by the owner's instruction; "Merged" below gives the
commits each pull request added after its CI.

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

**The checkout, for the record.** `libs/asio` in the superproject's main checkout was an older
local copy of the library (`a179adc`), not a submodule and ignored by git, whose `build.jam`
names a feature only the asio branch defines: with it there, every b2 command of that checkout
fails. It was moved to `.local/` for the runs above and moved back after them, unchanged; on
2026-10-27 the owner had it moved out of the checkout, intact, to `../asio-stale` beside the
superproject. The external libraries trystero needs were linked into that checkout's `.local/`
from the earlier checkout's, as its other toolchains already were.

## Merged (2026-10-27 and 2026-10-28)

In the migration plan's order: the site and each library, then the superproject with its
submodules moved, then this repository. Each pull request was merged with its CI green, and
each merge commit's tree is the tree its CI tested.

| Pull request | Merged as | What it added to the commits above |
| --- | --- | --- |
| webcpporg.github.io#2 | `d3aa48d` | |
| trystero#2 | `a9af932` | `0ef1641`, `9120af6`, `10c8866`: the CI's findings below |
| webcpp#5 | `9b4fa4e` | `891b4d6`, `7c78407`: trystero's two fixes, recorded |
| config#1, xactor#1, xstate#1, pratt#1, wasi#1, document#1 | `cf72ef8`, `611b6cc`, `5e1b58c`, `c46eb1b`, `983119b`, `3f78d0b` | |
| idna#2 | `2250caa` | `e324c90`: the race below |
| lexical#1 | `1a0556d` | `fc728d7`: the race below; `0eccb7d`: the tracked Python bytecode cache leaves the repository, which `.gitignore` now ignores |
| webcpp#6 | `a4c770e` | `81c5fe5`: `download.sh` below; `390496e`: every library and the site at its merge (task 2.10) |

**What the CI found, each reproduced before it was fixed.** None came from the change itself;
each was fixed where it was found, with a test where one could hold it.

- **trystero, the lint on Linux.** clang-tidy against libstdc++ asks `app_impl`'s and
  `room_impl`'s sink parameters to be references, which it does not against the macOS libc++.
  Reproduced by running the lint's compilation database through clang-tidy with gcc 15's
  libstdc++ headers; the parameters are exempt with their reason, as `app.hpp` already does.
- **trystero, the allocation test on MSVC.** MSVC's `std::list` allocates a node even when
  empty, and in debug a 16-byte container proxy, so the test counted global allocations its
  objects made while alive. The test backend's queue now takes its nodes from the test's memory
  resource, the second test's objects are destroyed before the counts are read, and a count
  that differs prints each allocation's size and `_ITERATOR_DEBUG_LEVEL`.
- **lexical and idna, the generated lane.** `b2 -j` runs lexical's two generators at once on
  one empty Unicode cache: both wrote a missing file through the same `<file>.partial`, so the
  second rename found nothing and the lane failed. Eight
  processes writing one path failed 1,220 and 1,200 of 1,600 writes, in lexical's and idna's
  `write()`; each now writes through a partial file of its own and renames it, and the same
  runs fail none.
- **The superproject, `tools/ci/download.sh`.** A server that dropped the connection failed the
  lane: `curl --retry` does not try a connection error again, `--retry-all-errors` does. A new
  case of `tools/ci/actions_test.py`, a loopback server that drops the first connection, failed
  with curl's exit 56 and passes with the flag.

Two failures were not the code's: `snapshot.ubuntu.com` answered 502 and 503 for a while,
and the jobs passed once it recovered; and trystero's `native_room` failed once on the MSVC 14.5
lane and passed when run again, as it did 30 times of 30 locally. Its instability was left to a
task of its own.

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

- Tasks 1 to 5 when the proposal was written: they are recorded under "Implementation" and
  "Merged".
- The design chapters were not reviewed line by line by a second reader against their sources.
- The lint of the superproject was not run on the two files, which are Markdown: its rules that
  read Markdown are the em dash and the banned word, both checked above.

## Completion

All tasks are checked. The archive wrote the added and modified requirements into
`specs/organization/spec.md` and `specs/site/spec.md`, and the organization's `## Purpose`
was changed by hand to name both files (design D9). A branch that changes the superproject's
`AGENTS.md` and was cut before `9b4fa4e` (`work/asio-task9-review` is one) carries each hunk to
its new home by design D2's table when it is rebased.
