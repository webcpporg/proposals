# Agent skills: implementation and verification

Recorded on 2026-10-13 (America/New_York).

The owner requested review, resolution of O1 to O5, correction of remaining defects, merge of
webcpp PR #2 and archival of this proposal. The decisions are in `../design.md`.

## Implementation

- Proposals PR #2 merged as `101f6e132302c65cbbd4266d3a1dde8e7932a3f8`.
- Webcpp PR #2 now contains the review fixes in
  `a971191c4c9a6fe4c0722b307bca893f82294b3a`, merged as `5a441c1e90c5b242cf55c351edf6c79c37ef1f8d`.
  The full CI run https://github.com/webcpporg/webcpp/actions/runs/38507470157 was
  cancelled at the explicit request of the owner to run no CI during this session. The Agent skills
  workflow (run 38507470100) had already passed before that instruction.
- This follow-up repairs the custom skills in proposals, adds metadata validation, preserves
  OpenSpec's MIT notice and records the final design decisions.

## Verification

The original metadata was read with a real YAML parser before editing. Four superproject skills
and two proposals skills failed on unquoted colon-space descriptions. After quoting, all five
superproject skills and all nine proposals skills parse with matching names.

The exact Python script in each Agent skills workflow was run locally with PyYAML 6.0.3.
It accepted valid metadata and rejected invalid YAML, a wrong name, absent or blank descriptions,
missing delimiters and a non-mapping value. The script is preserved as `verify_skills.py`:
run it from the repository being checked after installing PyYAML 6.0.3 in a virtual environment.
The generated OpenSpec skills were not edited.

The shared text skills are byte-identical. Both new workflows pass actionlint. The superproject
changes pass whitespace, source-licence and release-note checks. Independent review checked the
chapter and command references, then reviewed the corrections without remaining blockers.

## Completion

The owner-authorized review and implementation are complete. Both original PRs are merged.
The owner explicitly requested no CI during this session. The running full CI was cancelled;
subsequent commits and merges use `[skip ci]`. No passing full-CI result is claimed for the
final head; acceptance rests on the local checks and the independent review documented above.
The organization specification now contains the Agent skills requirement, the metadata validation
scenario and the narrow MIT documentation exception, preserving the original collection scenarios.
All tasks are checked, and the proposal is archived as `2026-10-13-agent-skills` with status
`implemented`, release `0.1.0` and the immutable webcpp merge commit above. As in the existing
index, 0.1.0 means the release in preparation, not a published tag.

The index was regenerated with `python3 tools/proposals.py readme`. Every OpenSpec root passed
strict validation and the proposal tool's 17 tests passed. The final archive and its validation
are reviewed in https://github.com/webcpporg/proposals/pull/3.
