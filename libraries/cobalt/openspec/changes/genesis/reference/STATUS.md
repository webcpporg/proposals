# Cobalt genesis status

Updated 2026-11-06. Status is `implementing`, with `decided: 2026-11-06` and `release: null`.
Rodrigo approved the proposal and requested execution in this session. Implementation has
started with task 1.1's baseline in the existing isolated superproject checkout, on branch
`work/cobalt-genesis`. The independent library is not yet extracted, implemented or released.
All 26 implementation tasks remain unchecked until their evidence and work are committed.

## Implementation authorization and baseline

- Owner approval: "gostei, pode aprovar e executar", received on 2026-11-06.
- Superproject starting commit: `d3a67959cbe52e6d972ef575be8870cc79da1312`.
- Asio starting commit: `db4f7fc6afc516fdef057614cc971cdc39484099`, as pinned by the superproject.
- Local execution record: `.superpowers/sdd/plan-reference/progress.md` in the superproject.
- Historical exports below remain unchanged; fresh results will identify their actual inputs.

## Starting evidence

- `source/` contains 70 selected unmodified source files from exact commits. The export
  records repository, revision, original path, mode and SHA-256 in `provenance.json`.
  Its contextual README is not part of the export manifest.
- Asio source baseline: `db4f7fc6afc516fdef057614cc971cdc39484099`.
- Superproject source baseline: `d994334102d138e2847b7e8463fb1c06a859bd42`.
- Boost.Cobalt 1.92.0 tag resolves to `8b1cecba91d0e868ee0947b40fb04e3594680297`.
  The public tag archive supplied examples missing from the installed Boost source subset.
  All 26 Cobalt `.cpp` files in that installed source subset were byte-compared with the
  corresponding upstream commit files and matched; this verifies source identity, not execution.
- `applications.json` inventories all 12 upstream C++ examples and the supporting files,
  plus all 5 upstream benchmark sources. `applications.md` explains the required adaptations.
- `profiles.json` defines 28 starting target/runtime/EH/thread/suspension tuples;
  `application-matrix.json` expands all 336 application/profile cells. They are planning
  classifications, not fresh conformance results; none is marked `supported-tested`.
- `python-script-check.json` records the unchanged Python driver's module-name `NameError`
  using a minimal import stub. This isolated script check does not load C++/nanobind or
  provide application conformance evidence; the original exported script remains unchanged.

Existing integration code demonstrates where implementation starts, not whether the
independent library or every example works. Existing Asio status prose, historical size
tables and remote CI outcomes have not been copied into this proposal as current evidence.
The known issue-287 caution and SDK restrictions are bounded design inputs to reproduce,
not an excuse to ban every no-EH or threaded application.

## Task state

| Tasks | State and next evidence |
| --- | --- |
| 1.1-1.3 | In progress: owner approval recorded; collecting baseline commands before inventory and runtime probes |
| 2.1-2.3 | Not started: independent target skeleton, extraction, compatibility aliases and graph/link isolation |
| 3.1-3.3 | Not started: guards, runner/coroutine contracts and operation-specific EH evidence |
| 4.1-4.3 | Not started: worker, Cobalt I/O and optional shared-provider TLS witnesses |
| 5.1-5.4 | Not started: every original example and separately identified counterpart |
| 6.1-6.3 | Not started: real C++ browser pages, runtime matrix and end-user/visual verification |
| 7.1-7.3 | Not started: Release workload/profile audit and raw size/performance/memory observations |
| 8.1-8.2 | Not started: library guide/reference, ownership references, discovery and installation |
| 9.1-9.2 | Not started: fresh complete candidate review, authorized CI and integration |

## Proposal checks

The following checks passed on 2026-11-04:

- OpenSpec 1.10.0 `validate --all --strict --no-interactive`, from `libraries/cobalt/`:
  one change passed, zero failed. `status --change genesis` reports all four planning
  artifacts complete. This is planning completeness, not implementation completion.
- Every one of the 70 exported files matched its SHA-256 and executable mode.
- The inventory matched all 12 original C++ examples and 5 benchmark files/hashes;
  28 unique profiles expanded into exactly 336 unique application/profile cells.
- All 26 unchecked implementation tasks matched their detailed plan entries in order.
- Scoped proposal metadata and forbidden-text checks passed; the working tree contained
  only the new `libraries/cobalt/` proposal files from this work.

The initial matrix contains 167 `implementation-missing`, 48 `sdk-runtime-blocked` and
121 `not-applicable` original-source cells, with separate counterpart states. These counts
describe planning classifications, not passed/failed tests. None has runtime evidence.

Reproduce the scoped strict OpenSpec check from `libraries/cobalt/`:

```sh
OPENSPEC_TELEMETRY=0 openspec validate --all --strict --no-interactive
OPENSPEC_TELEMETRY=0 openspec status --change genesis
```

Check reference bytes and inventory from this `reference/` directory:

```python
import hashlib
import json
from pathlib import Path

root = Path('.')
manifest = json.loads((root / 'provenance.json').read_text())
for entry in manifest['files']:
    source = root / entry['path']
    assert hashlib.sha256(source.read_bytes()).hexdigest() == entry['sha256']
apps = json.loads((root / 'applications.json').read_text())['applications']
profiles = json.loads((root / 'profiles.json').read_text())['profiles']
cells = json.loads((root / 'application-matrix.json').read_text())['cells']
assert {a['path'] for a in apps} == {
    str(p.relative_to(root / 'source/upstream'))
    for p in (root / 'source/upstream/example').glob('*.cpp')
}
assert len(apps) == 12 and len(profiles) == 28 and len(cells) == 336
assert {(c['application'], c['profile']) for c in cells} == {
    (a['id'], p['id']) for a in apps for p in profiles
}
assert all(c['status'] != 'supported-tested' and not c['evidence'] for c in cells)
```

Repository-wide index regeneration and checks belong to the integrating change:
`python3 tools/proposals.py readme`, `python3 tools/proposals.py check` and
`python3 tools/proposals_test.py`. These validate the proposal repository, not a Cobalt
implementation. No product build, CI or performance result is implied.

## How to resume

1. Read `../proposal.md`, `../design.md`, `../specs/cobalt/spec.md`, `applications.md`,
   `plan.md` and the current superproject rules. Keep the proposed defaults distinct from
   accepted decisions and read O1-O3 before choosing runtime fixtures or workload sizes.
2. Acceptance and the implementation request were received on 2026-11-06. Resume at the
   first incomplete task in the execution record; preserve the original source export.
3. Verify the export hashes and rebase the baseline deliberately if implementation commits
   have changed. Record new provenance instead of silently carrying historical results.
4. Complete tests-first extraction and validation in task order. The plan identifies files,
   interfaces, steps and expected outputs for every task. Update only committed completed
   tasks; keep unsupported and pending matrix cells explicit.
5. Preserve Asio default 100, one shared TLS provider, the compatibility target names and
   source forms. Require separate authorization for repository creation/publication and
   implementation-stage CI where applicable; no release is authorized by this proposal.
