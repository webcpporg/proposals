## Why

The owner decided on 2026-11-14 that the build names each of Cobalt's targets once, under
`/webcpp/cobalt` (`core`, `cobalt`, `io_ssl` and `cobalt_ssl`), and removed the four legacy names
that forwarded to them: `/webcpp/asio//cobalt`, `/webcpp/asio//cobalt_ssl`, `/webcpp//boost_cobalt`
and `/webcpp//boost_cobalt_io_ssl`. The superproject, Asio, Cobalt, the Cobalt and Asio genesis
proposals, `AGENTS.md` and `CODING-GUIDELINES.md` were updated and merged that day. The
organization's main specification still names `boost_cobalt` in three requirements, and was
already stale before the decision: it says that `/webcpp//boost_cobalt` is built from Boost's own
sources in `tools/boost/Jamfile`, while since 2026-11-06 Cobalt has shipped its own sources
(`libs/cobalt/sources.json`) in its own project. A reader of the specification would name a
target that b2 no longer finds.

## What Changes

- "Build: The aggregates and the explicit targets": the explicit targets are the Jamroot's at
  webcpp `eb08198` (`boost_version`, `throw_exception`, `boost_json`, `boost_url`,
  `boost_charconv`, `boost_locale`, `node-test` and `emscripten-reference`), without
  `boost_cobalt`; the Jamroot declares no Cobalt target nor an alias to one, with a scenario for
  a program that still names a removed root name.
- "Build: Boost's compiled libraries from its sources": the libraries built from Boost's sources
  are `/webcpp//boost_url`, `/webcpp//boost_charconv` and `/webcpp//boost_locale`; Boost.Cobalt
  is not one of them, since Cobalt ships its own sources and builds them in `/webcpp/cobalt`.
- "Build: What Boost.Locale and Boost.Charconv give on each target": the closing sentence on
  Boost.Cobalt, which described the removed `/webcpp//boost_cobalt` build, is removed; Cobalt's
  targets and its exceptions are its own specification's.
- "Library: build.jam": a dependency on Cobalt is one of its targets under `/webcpp/cobalt`, like
  a dependency on any other library, and Boost.Cobalt leaves the list of Boost's compiled
  libraries.
- Nothing in the build, the tools or the libraries changes: the implementation is already merged.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `organization`: the requirements "Build: The aggregates and the explicit targets", "Build:
  Boost's compiled libraries from its sources", "Build: What Boost.Locale and Boost.Charconv give
  on each target" and "Library: build.jam" are modified.

## Impact

- **Kind:** the organization (the organization's specification of the superproject's build).
- **Implementation:** already merged on 2026-11-14 by the owner's decision of that day:
  https://github.com/webcpporg/webcpp/pull/13 (merge commit
  `eb0819805cb890b006ededef925115e4fdc54497`), https://github.com/webcpporg/asio/pull/3,
  https://github.com/webcpporg/cobalt/pull/1 and https://github.com/webcpporg/proposals/pull/9;
  see `reference/STATUS.md`.
- **Status:** implemented in the 0.1.0 release in preparation; archived on 2026-11-14, which
  writes the modified requirements into `specs/organization/spec.md`.
- **Other proposals:** Cobalt's genesis (`libraries/cobalt/openspec/changes/genesis/`) holds
  the requirement that each Cobalt target has one name, and Asio's genesis the requirement that
  Asio declares no Cobalt target; this change only aligns the organization's specification with
  them.
