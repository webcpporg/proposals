# Status: accepted, 2026-11-03

The owner accepted the proposal and requested merge on 2026-11-03. No generator implementation,
repository rename or runtime compatibility measurement has been completed.

## Reference baseline

`source/` contains selected existing tracked source files, copied exactly from commit
`2ba5116d2d6827b0afbb383bca14ea73fa77fdc7` of https://github.com/webcpporg/webcpp. These are implementation starting points,
not a prototype of the proposed generator or evidence that the new behaviour works. The existing
copyright and licence notices are preserved. All implementation tasks remain unchecked.

## How to resume

Read proposal.md, design.md and tasks.md, reconcile the accepted wasi redesign, then resolve the
remaining backend/profile, dependency and migration choices before coding their affected tasks.
The library proposal and organization wasm-tools proposal must be implemented together where
registration and installation cross repository boundaries. BDK-specific APIs remain in the example.
