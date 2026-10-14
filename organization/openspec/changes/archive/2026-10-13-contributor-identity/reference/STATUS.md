# Contributor identity: implementation and archival evidence

Verified on 2026-10-13 (America/New_York), without executing CI.

## Implementation

- https://github.com/webcpporg/webcpp/pull/1 merged as
  `9a268b7efa8a9b7eab8810bc024486288c689fab` on 2026-10-13 local time.
- https://github.com/webcpporg/proposals/pull/1 merged the proposal as
  `4ad28c278b8237818c0b6ece59ac7e75723b9ce9` on the same local date.
- The identity rule is unchanged in the fetched webcpp main at
  `5a441c1e90c5b242cf55c351edf6c79c37ef1f8d`. `identity-rule.md` preserves its text.
- A search of the current `tools/` and `.github/` found no enforcement of a fixed contributor
  address. The matches set throwaway test identities or the site publishing bot's own identity.

## Reproduce the read-only checks

Run from a webcpp checkout containing the published merge commit:

```sh
git show 9a268b7efa8a9b7eab8810bc024486288c689fab:AGENTS.md
git grep -n -E 'user\.email|user\.name|interpret-trailers' 5a441c1e90c5b242cf55c351edf6c79c37ef1f8d -- tools .github
```

Read chapter 11's Identity paragraph: own identity, allowed noreply address, no fixed address,
the placeholder commit command, no trailers and explicit paths are all present. The modified
specification preserves the accidental-trailer scenario and adds the external-contributor case.

## Archive

O1 to O4 are closed within the already merged scope in `../design.md`. D3's ongoing migration
policy is in `CONTRIBUTING.md`; no claim is made that all older plans were rewritten.
The change is implemented for the 0.1.0 release in preparation, consistent with the index;
no release tag is created. The main specification is synchronized and the index regenerated.
Local validation covers all OpenSpec roots, the proposal index and the proposal tool's tests.
The archive commit and merge use `[skip ci]` at the owner's explicit request.
