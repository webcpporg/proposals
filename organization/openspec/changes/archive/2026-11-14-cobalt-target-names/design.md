## Context

Until 2026-11-06 webcpp built Boost.Cobalt from Boost's own sources, as `/webcpp//boost_cobalt`
in `tools/boost/Jamfile`, and the organization's genesis specified it with Boost.URL,
Boost.Charconv and Boost.Locale. Cobalt then became a library of its own, `webcpporg/cobalt` at
`libs/cobalt`, which ships its sources (`libs/cobalt/sources.json`) and declares its targets in
`/webcpp/cobalt`; the old names were kept as aliases that forwarded to the new ones. On 2026-11-14
the owner decided that each Cobalt target has one name, and the aliases at the superproject's root
and in Asio were removed (webcpp `eb08198`, Asio `6c4e2c8`, Cobalt `5d51c4a`), with a test,
`tools/test/cobalt_ownership_test.py`, that a program naming a removed name stops. Cobalt's and
Asio's genesis proposals were updated on the same day. The organization's main specification is
the one text that still names `boost_cobalt`.

## Goals / Non-Goals

**Goals:**

- The organization's specification names no removed Cobalt target and says where Cobalt's
  targets are.
- Each modified requirement describes the Jamroot and `tools/boost/Jamfile` as they are at webcpp
  `eb08198`.

**Non-Goals:**

- Restating Cobalt's targets, configurations or exception behaviour: they are Cobalt's own
  specification's.
- `/webcpp//boost_context`, which the Jamroot builds from Boost's sources since Beast's
  integration (webcpp `91af39e`) and which the organization's specification has never named.
  Specifying it, on which targets it builds and whether it is explicit at the root, needs its own
  change.
- Any change to the build, the tools or the libraries.

## Decisions

### D1. The list of explicit targets is the Jamroot's

The requirement's list is copied from the Jamroot's `explicit` declaration at `eb08198`, so it
drops `boost_cobalt` and also names `node-test`, which the Jamroot has declared since the test
under node (webcpp `5d63e95`) and the specification never listed.

Rejected: removing `boost_cobalt` alone. The requirement would still disagree with the Jamroot it
describes, in a list this change rewrites anyway.

### D2. Cobalt's own specification owns what Cobalt builds

The organization's specification says only that Cobalt is a library whose targets are under
`/webcpp/cobalt`, and that the root and `/webcpp/boost-libraries` declare none. The sentence
"Boost.Cobalt is built for native programs and has no build without exceptions" is removed rather
than rewritten: it described the removed build from Boost's sources, and Cobalt's genesis now
specifies its targets ("Explicit configuration and runtime capability matrix") and its exceptions
("Exceptions follow the caller with precise limitations").

Rejected: rewriting the sentence for the shipped library. Two texts would describe one library's
targets and drift apart, which is how this sentence became stale.

### D3. A dependency on Cobalt is a dependency on a library

"Library: build.jam" names Cobalt among the libraries a library depends on by its target, with
`<library>/webcpp/cobalt//core` as the example, and a scenario that lists the four targets. Which
of them a library or a program needs (`core` for a custom main, `cobalt` for `co_main`, `io_ssl`
and `cobalt_ssl` for TLS) is Cobalt's documentation's.

### D4. Recorded as implemented, and archived in the same change

The decision is the owner's, of 2026-11-14, and its implementation is merged in the superproject
(webcpp#13), Asio (asio#3), Cobalt (cobalt#1) and the proposals (proposals#9). Nothing is left to
implement but the specification itself, which archiving writes. The change is therefore
`implemented`, in the 0.1.0 release in preparation as every implemented organization change is,
and archived in the pull request that proposes it; merging that pull request is the owner's
acceptance.

Rejected: an open change with status `accepted`. The main specification would keep naming the
removed targets until a later archive, with no work left in between.

## Risks / Trade-offs

- [A reader of an older proposal finds `/webcpp//boost_cobalt`] -> archived proposals and plans
  keep their text as records; b2 stops with the name of the target, and Cobalt's genesis lists
  the four removed names and their replacement.
- [The specification still omits `/webcpp//boost_context`] -> named as a non-goal here so that
  its own change can specify it.

## Migration Plan

None: programs that named a removed target already stop since the implementation was merged, and
the specification follows the build.
