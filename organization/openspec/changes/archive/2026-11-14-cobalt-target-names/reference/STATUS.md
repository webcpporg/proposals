# Cobalt's target names: implementation and archival evidence

Verified on 2026-11-14 against the published merge commits, without running the CI.

## Implementation

- https://github.com/webcpporg/webcpp/pull/13 merged as
  `eb0819805cb890b006ededef925115e4fdc54497`: the Jamroot and `tools/boost/Jamfile` no longer
  declare `boost_cobalt` nor `boost_cobalt_io_ssl`, and
  `tools/test/cobalt_ownership_test.py`, `test_only_the_cobalt_library_names_cobalt`, checks
  that a program naming `/webcpp/asio//cobalt`, `/webcpp/asio//cobalt_ssl`,
  `/webcpp//boost_cobalt` or `/webcpp//boost_cobalt_io_ssl` stops with "Unable to find file or
  target named", while `/webcpp/cobalt//cobalt` builds.
- https://github.com/webcpporg/asio/pull/3 merged as
  `6c4e2c85fe6e875bbf67db095df2497b93bf455b`: Asio's `build.jam` names no Cobalt target.
- https://github.com/webcpporg/cobalt/pull/1 merged as
  `5d51c4aa89e563070aa244e0635139758d762f97`: `build.jam` declares `project /webcpp/cobalt`
  with `core`, `cobalt`, `io_ssl` and `cobalt_ssl`, built from the sources `sources.json` lists
  through the project `/webcpp/cobalt-sources`.
- https://github.com/webcpporg/proposals/pull/9 merged as
  `18ba2b6cb2c2646d38ec4dbc62e799aa80576a54`: Cobalt's and Asio's genesis proposals record the
  decision.

## What each modified requirement was checked against

- "Build: The aggregates and the explicit targets": the Jamroot's `explicit` declaration at
  `eb08198` lists `boost_version throw_exception boost_json boost_url boost_charconv boost_locale
  node-test emscripten-reference` before the aggregates and the notfile targets. `node-test` has
  been in that list since webcpp `5d63e95`.
- "Build: Boost's compiled libraries from its sources": the Jamroot's aliases checked against the
  sources are `boost_url`, `boost_charconv`, `boost_locale` and `boost_context`; none is a Cobalt
  target. `boost_context` is a non-goal of this change (`../design.md`).
- "Build: What Boost.Locale and Boost.Charconv give on each target" and "Library: build.jam":
  Cobalt's `README.md` at `5d51c4a` names its canonical targets, `/webcpp/cobalt//core` for a
  custom main and `/webcpp/cobalt//cobalt` for `co_main`, with the optional `io_ssl` and
  `cobalt_ssl`.

## Reproduce the read-only checks

From a webcpp checkout that holds the merge commit:

```sh
git show eb08198:Jamroot | grep -n -A3 '^explicit boost_version'
git show eb08198:Jamroot | grep -n 'for local name in boost_'
git grep -n 'boost_cobalt' eb08198 -- Jamroot tools/boost
git show eb08198:tools/test/cobalt_ownership_test.py | grep -n -A20 'def test_only_the_cobalt_library_names_cobalt'
```

The third command prints nothing. The others print the list of explicit targets, the four
compiled aliases and the test of the removed names.

## Archive

The change is implemented for the 0.1.0 release in preparation, consistent with the index; no
release tag is created. Archiving writes the four modified requirements into the organization's
main specification and the index is regenerated. Local validation covers the organization's
OpenSpec root, the proposal index and the proposal tool's tests.
