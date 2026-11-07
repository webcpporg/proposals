# Selected starting source

These files are unmodified exports, not a standalone Cobalt implementation. The complete
file list, repository URLs, source paths, revisions, modes and SHA-256 values are in
`../provenance.json`. This README is authored context and is not an exported source file.

| Directory | Origin | Purpose |
| --- | --- | --- |
| `asio/` | webcpporg/asio at `db4f7fc6afc516fdef057614cc971cdc39484099` | Existing Cobalt runner, build entry points, selected tests and tutorial source |
| `superproject/` | webcpporg/webcpp at `d994334102d138e2847b7e8463fb1c06a859bd42` | Cobalt overlays/manifests/recipes, source acquisition pin and selected TLS witnesses |
| `upstream/` | boostorg/cobalt at `8b1cecba91d0e868ee0947b40fb04e3594680297`, tag `boost-1.92.0` | All example/benchmark files plus selected runner/task/exception/build sources |

The upstream archive came from
<https://codeload.github.com/boostorg/cobalt/tar.gz/8b1cecba91d0e868ee0947b40fb04e3594680297>.
Its observed SHA-256 is recorded in provenance; it identifies the downloaded archive,
not an independent upstream release-signature verification. The source acquisition recipe
also records the superproject's official Boost 1.92.0 source archive URL and checksum.

Preserve all upstream notices. The shared Boost Software License text is retained at
`superproject/LICENSE_1_0.txt`. Do not rewrite these exported files to implement the new
library or update their historical paths. An implementation starts in `libs/cobalt` and a
later source export replaces this selected starting snapshot with explicit provenance.

Partial exports intentionally retain references to files outside this directory. Build/run
them in their matching source trees or recreate the documented implementation environment;
this reference directory is not advertised as a self-contained build. Documentation/status
pages with historical support tables were not copied as current Cobalt evidence.
