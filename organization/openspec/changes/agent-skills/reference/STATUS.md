# Agent skills: implementation and verification

Recorded on 2026-10-13 (America/New_York).

The owner requested review, resolution of O1 to O5, correction of remaining defects, merge of
webcpp PR #2 and archival of this proposal. The decisions are in `../design.md`.

## Implementation

- Proposals PR #2 merged as `101f6e132302c65cbbd4266d3a1dde8e7932a3f8`.
- Webcpp PR #2 now contains the review fixes in
  `a971191c4c9a6fe4c0722b307bca893f82294b3a`; its final CI and merge are pending.
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

## Remaining

Wait for the final webcpp CI, merge PR #2, record its immutable merge commit, synchronize the
specification, mark this proposal implemented for the 0.1.0 release in preparation, regenerate
the index, validate every OpenSpec root and merge this follow-up. No release tag is created.
