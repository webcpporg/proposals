# site Specification

## Purpose
The webcpp site is where webcpp shows its idea, "where C++ and the web meet", and leads its reader
to the libraries' documentation and, later, to live examples. It speaks to C++ developers,
JavaScript developers, crypto developers and AI agents, is built with webcpp (a JavaScript
application of OpenUI Lang pages rendered to static HTML), and is the output of one command,
`b2 site`, in two channels: the preview, https://webcpporg.github.io/, built from every push to
`main`, and the release, https://webcpp.org/, built from a release tag. This capability is the site as
built by its increment 1: the site repository at `7a4c03f` (`../../changes/archive/2026-10-07-genesis/reference/site/`) and the superproject's
`tools/site/`, `tools/ci/assemble.py` and CI jobs at `c53f3f9`. The later increments are the change
`site-increments-4-5` and the roadmap.

## Requirements

### Requirement: Site: Two channels and one command
The site SHALL be built for one of two channels by one command, `b2 site` at the superproject's root, into `bin/site/`: the preview (the default), the development version, served at https://webcpporg.github.io/ from every push to `main`, and the release, built with `-sWEBCPP_SITE=release -sWEBCPP_SITE_VERSION=<version>` and served at https://webcpp.org/ from a release tag. Any other value of `WEBCPP_SITE` MUST stop the build, naming the two channels; the release needs a version such as `1.0.0` or `1.0.0-rc.1` (matching `^[0-9]+[.][0-9]+[.][0-9]+(-[0-9A-Za-z.-]+)?$`) and the preview takes none. A preview page's band names the commit it was built from and its date ("Preview of main at `<commit7>`, `<date>`"), a release page's the version ("webcpp `<version>`").

#### Scenario: A release without a version
- **WHEN** `b2 site -sWEBCPP_SITE=release` runs
- **THEN** the build stops saying that a release's pages carry its version and giving `-sWEBCPP_SITE_VERSION=<version>`, such as 1.0.0

#### Scenario: A version on the preview
- **WHEN** `b2 site -sWEBCPP_SITE_VERSION=1.0.0` runs
- **THEN** the build stops saying the preview carries none

#### Scenario: The bands
- **WHEN** the preview is built at commit `0123456...` dated 2026-10-10
- **THEN** every page's band reads "Preview of main at `0123456`, 2026-10-10", and a release 1.2.3's reads "webcpp 1.2.3" with no commit

### Requirement: Site: The options of b2 site
`b2 site` SHALL accept `-sWEBCPP_SITE_DOCS=<dir>`, which names the pages `b2 doc -sWEBCPP_INDEX=site` built already (a superproject's root or a copy of its `doc/html` and `libs/<name>/doc/html`, as the CI's docs job hands them on; the directory must exist or the build stops saying so), without which `b2 site` depends on and runs `b2 doc`; and `-sWEBCPP_SITE_REPORT=<dir>`, which names what `tools/report/report.py` wrote, without which `report/` says that no results were given, as on any machine but the CI's. A `-s` value is joined at its spaces and made absolute against b2's working directory.

#### Scenario: No report given
- **WHEN** `b2 site` runs on a developer machine
- **THEN** `bin/site/report/index.html` is one page saying no results were given

#### Scenario: A missing docs directory
- **WHEN** `-sWEBCPP_SITE_DOCS=/nowhere` is given
- **THEN** the build stops with "-sWEBCPP_SITE_DOCS=/nowhere does not exist"

### Requirement: Site: The site's repository
The site's source SHALL be a repository of its own, `webcpporg/webcpporg.github.io`, BSL-1.0, checked out in the superproject as the submodule `site/` as a library is under `libs/`: a project that uses webcpp the way any user would. A change to it is committed in `site/` on its `main`, then the superproject commits the submodule's new commit by path. What the superproject holds of the site is `tools/site/` (`site.jam`, `catalogue.py`, `redirect.py` and their tests) and `tools/ci/assemble.py`. The site's tests are `b2 /webcpp/site//test` (its npm `test` and `typecheck`), and `tools/site/*_test.py` with `tools/ci/assemble_test.py` among the tools' tests. The repository holds `AGENTS.md`, `README.md`, `Jamfile`, `package.json` and `package-lock.json`, `tsconfig.json`, `pages/`, `src/`, `assets/`, `design/`, `seo/queries.json`, `tools/`, `test/` and `.github/workflows/site.yml`.

#### Scenario: The site is not checked out
- **WHEN** a clone has no `site/Jamfile`
- **THEN** `b2 site` and `b2 site-serve` stop saying so, with `git submodule update --init site`, and every other target builds

### Requirement: Site: The rules of the site's repository
The site's `AGENTS.md` SHALL hold only what is specific to the repository and tell the reader to read the superproject's first: pages are written in OpenUI Lang under `pages/`; components are React, each with its props validated by a zod schema (the design system the pages' components import); the client bundle the site ships is a production build with no development warning and no unminified source; a link between pages is relative, never absolute, because the site serves from whichever path it is published under; and nothing on a page is typed twice: a value the build already knows, such as a library's name or a count from its own build, is read from it rather than written again on the page.

#### Scenario: An absolute link
- **WHEN** a page writes `Link("a", "/libraries/")`
- **THEN** rendering fails with an error saying a link must be relative

### Requirement: Site: What b2 site does
`b2 site` SHALL run these steps in order, each failing the build with its faults named: (1) `site-declared.txt`, the `<library> <target>` pairs the libraries declare (`webcpp.declared`, as `declared-targets` prints them); (2) `libraries.json`, the catalogue, written by `tools/site/catalogue.py`; (3) the site's `context.json` (the channel, the superproject's commit and its date, the version or null, and the catalogue) by `write-context`; (4) the site's pages (`/webcpp/site//pages`: `node-modules`, the bundles, the pages rendered with their cards, assets, icons and the client bundle, and the two headers `docs-header.html` and `site-header.html`); (5) `tools/ci/assemble.py` lays the site out in `bin/site.tmp/`; (6) the indexing files (`npm run build -- --index`); (7) `assemble.py --check` checks every link; (8) the build is moved into a directory of `bin/site.builds/` and the link `bin/site` is swapped onto it in one rename; (9) `/webcpp/site//valid-html` checks every page of `bin/site/`. Every step writes `bin/site.tmp`, never `bin/site`, so a failed step leaves `bin/site` as it was, the previous build or none.

#### Scenario: A failed step
- **WHEN** a page fails to render during `b2 site`
- **THEN** `bin/site` still points to the last build that passed every step, or does not exist

#### Scenario: A deleted page
- **WHEN** a page is deleted from `pages/`
- **THEN** the next `b2 site` has no such page, because the rendered pages are removed first

### Requirement: Site: The layout of bin/site
`bin/site/` SHALL hold: `index.html` (the home, from `pages/index.oui`); `libraries/`, `cpp/`, `javascript/`, `ai/`, `examples/`, `community/` and `search/` (each from its `pages/<name>.oui`); `assets/` (the styles, the logo, the icons and the client bundle `site.js`); `cards/` (each page's and each library's social card, 1200 by 630); `favicon.ico`; `docs/` (the index page, built with `-sWEBCPP_INDEX=site`); `libs/<name>/` (each library's page); `report/` (the test matrix); `robots.txt`; `sitemap.xml` on the release alone; `llms.txt`; and `libraries.json` (the catalogue). A place the site's own pages already hold (`docs/`, `libs/`, `report/`) fails the assembly.

#### Scenario: The site's directories
- **WHEN** a preview is built with five libraries
- **THEN** `bin/site/libs/` holds five directories, `bin/site/docs/index.html` exists, and `bin/site/sitemap.xml` does not

### Requirement: Site: The build swap and the server
`bin/site` SHALL be a symbolic link (a junction on Windows) swapped onto each finished build with one rename by the site's `swap` script (`tools/swap.mjs`, `pointTo`): `swap.mjs <link> <build>` makes `<link>` a link to `<build>`, an existing directory of `<link>.builds/`, with a relative target so that a checkout moved elsewhere still reads it, then removes every other `build-*` of that directory (the one the link pointed to and any left by a run that stopped between building and swapping) and nothing else. A `<build>` that is not an existing directory of `<link>.builds/` is refused with exit 2 before anything is touched; a link that is a real directory (the layout before it was a link) is replaced; a `${link}.next-<pid>` left by a crashed run is cleaned up, and only a link of that form. A server reading through the link resolves it on every request and reads one build or the other whole, never half of one.

#### Scenario: A server during a run of rebuilds
- **WHEN** a server on `bin/site` answers requests while `b2 site` swaps in new builds
- **THEN** every request is answered 200, never 404 or 503

#### Scenario: A build outside the builds directory
- **WHEN** `swap.mjs bin/site /tmp/elsewhere` runs
- **THEN** it exits 2 and touches nothing

### Requirement: Site: b2 site-serve and the serve script
`b2 site-serve` SHALL build `bin/site/` and serve it at http://127.0.0.1:8765/, printing the address as the Jamroot loads (b2 shows an action's output only when it ends). The `serve` script (`tools/serve.mjs <dir> [port]`, also `npm run serve -- <dir>`) MUST serve a built site with node's `http` module alone on 127.0.0.1, default port 8765, with each file's type (`.html`, `.css`, `.js` and `.mjs` as `text/javascript`, `.json`, `.webmanifest` as `application/manifest+json`, `.wasm` as `application/wasm`, `.svg`, `.png`, `.ico`, `.txt`, `.xml`, `.woff2`) and without caching; a directory is its `index.html`, reached with its final slash (a path without it is redirected there); nothing outside the directory is served, an escaped slash never climbs out, and no request-target is answered 500; a request-target is read as a path (leading slashes collapse by a redirect, an absolute-form target is served by its path per RFC 9112, any other is 400). A relative `<dir>` is read from the directory npm was started in (`INIT_CWD`). A root that has been there and goes briefly missing (the swap's own gap on Windows) is waited for, five retries 20 ms apart, then answered 503, never a 404 that would misreport a build in progress, and a file missing within a present root is retried the same way; neither case nor a fault of a stream ends the process.

#### Scenario: A directory without its slash
- **WHEN** a client requests `/libs/xactor`
- **THEN** it is redirected to `/libs/xactor/`

#### Scenario: A traversal
- **WHEN** a client requests `/..%2f..%2fetc/passwd`
- **THEN** nothing outside the site is served and the answer is not 500

#### Scenario: A root that never held anything
- **WHEN** a request names a path in a root that has never been there
- **THEN** the answer is a plain 404 at once, without waiting

### Requirement: Site: The site's npm scripts and how b2 calls them
The site's `package.json` SHALL define the scripts `build` (`node tools/build.mjs`), `pretest` (`node tools/build.mjs --bundle`, so that npm alone never reads a stale `build/server.mjs`), `test` (`node --test 'test/**/*_test.mjs'`), `typecheck` (`tsc --noEmit`), `dev` (`node tools/dev.mjs`), `serve` (`node tools/serve.mjs`) and `swap` (`node tools/swap.mjs`), with `private: true`, `type: module` and `license: BSL-1.0`, its dependencies pinned by `package-lock.json`: `@openuidev/react-lang` 0.4.1, `react` and `react-dom` 19.3.0, `zod` 4.6.5, and as development dependencies `@expo-google-fonts/inter` 0.4.2, `@resvg/resvg-js` 2.6.2, `@types/react` and `@types/react-dom` 19.3.0, `esbuild` 0.28.2, `html-validate` 11.16.2, `typescript` 7.0.2 and `xstate` 5.33.2. b2 MUST call these scripts through one wrapper rule, `site.npm <script> : <arguments>` (`npm --prefix "<site>" run <script> -- "<arguments>"`), and never reimplement them; every target whose action runs one depends on `/webcpp/site//node-modules`, the site's packages installed by `tools/node/install.py` (`npm ci` with telemetry off). node and npm are those on `PATH`, node at version 26.7.0.

#### Scenario: Pinned versions
- **WHEN** `test/package_test.mjs` reads the lockfile
- **THEN** it finds the versions above

#### Scenario: A stale bundle
- **WHEN** `npm test` runs after a component changed
- **THEN** `pretest` rebuilds the bundles first

### Requirement: Site: Two ways into the repository
The site's `README.md` SHALL have a section for each of two readers and neither MUST send its reader through the other's tools. For JavaScript developers: Path 1, the examples and pages with npm alone (`OPENUI_TELEMETRY_DISABLED=1 npm ci`, `npm run dev`, `npm test`, `npm run typecheck`; `npm run dev` builds the pages with the fixture catalogue, serves them at http://127.0.0.1:8765/ and rebuilds on a change under `pages/`, `src/` or `assets/`, using the examples' wasm built in the superproject's `bin/site/` when present and otherwise the wasm the published preview serves, downloaded once into `build/dev/` and checked against the preview's `precache.json`); Path 2, the whole project, step by step for someone who has never built C++ (B2 5.5.3, Boost 1.92.0, node 26.7.0 and Python; Emscripten 6.0.11 from emsdk; MrDocs 2026.9.29; `~/user-config.jam` lines; `git clone --recursive https://github.com/webcpporg/webcpp` and `b2 site-serve`). For C++ developers: b2 alone (`b2 site`, `b2 site -sWEBCPP_SITE=release -sWEBCPP_SITE_VERSION=<version>`, `b2 site-serve`, `b2 /webcpp/site//test`), where the site's C++ will live (`app/`, `include/webcpp_site/`, `examples/`), and the prerequisites, node among them as one more tool b2 runs; npm's commands, `package.json` and the bundler are never named in this section. `test/readme_test.mjs` MUST keep this true: every npm command the README shows is an npm command or a script of `package.json`, every b2 command names a target the build declares, the C++ section names no npm, `package.json` or bundler, the JavaScript section has both paths, `dev` takes the examples' wasm from `bin/site` without the network when the superproject built it, otherwise downloads the preview's once checked against its `precache.json`, and refuses a downloaded wasm whose hash is not the one `precache.json` lists.

#### Scenario: A README command that is no script
- **WHEN** the README shows `npm run frobnicate`
- **THEN** `readme_test` fails

#### Scenario: A tampered wasm
- **WHEN** `dev` downloads a wasm whose SHA-256 differs from the preview's `precache.json`
- **THEN** it refuses it

### Requirement: Site: The dev script
`npm run dev` (`tools/dev.mjs [port]`) SHALL build the site's own pages with `test/fixtures/catalogue.json` and a fixed context into a fresh directory under `build/dev/builds/` each time, serve them through `build/dev/site`, a symlink (a junction on Windows) swapped onto the new one with one rename (`pointTo`), on 127.0.0.1 by default on port 8765, and build again on a change under `pages/`, `src/` or `assets/`; a build that fails is reported and the last good one stays served, the previous build is removed only once the swap succeeded, and a request fired in every gap of a rebuild still gets the site, never a 404. `pointTo` MUST recover when `out` is a real directory left by an earlier layout and clean a stale next link. The examples' wasm is served from `build/dev/wasm/`; the download from the preview is the one access to the network outside `npm ci`, and happens only here, never in a build or a test.

#### Scenario: A rebuild during requests
- **WHEN** requests are fired in every gap of dev's own rebuild
- **THEN** each gets the site, never a 404

### Requirement: Site: Pages are OpenUI Lang rendered to static HTML
Each page SHALL be written in OpenUI Lang under `pages/` (`index.oui` is the home, `<name>.oui` the page `<name>/`) and rendered at build time by the same renderer and components the browser uses (`renderToString`), into `<out>/<path>/index.html`, then hydrated in the browser. Each document MUST be a shell of four elements in this order and nothing else inside `<body>`: `<noscript>` (one message saying the site needs JavaScript), `<div id="root">` (OpenUI's rendered tree: the header, the band, `<main>`, the footer), `<script type="text/openui" id="page-source">` (the page's OpenUI Lang source, which cannot close its script element) and `<script type="application/json" id="site-context">`. What `#root` holds equals what OpenUI renders from the carried source and context. A page that names an unknown component fails with `<file>:<line>: <message>` and exit 1; a link from the server's root is refused; the pages' markup is their components': no string of the source holds a tag. The components, each with its zod props, are `Page` (root), `Header`, `Band`, `Footer` (the Page's own, which no block may hold), `Hero`, `Section`, `Text`, `Code`, `Link`, `Doors`, `Targets`, `LibraryTable`, `LibraryList`, `Roadmap` and `SearchForm`.

#### Scenario: An unknown component
- **WHEN** `pages/bad.oui` is `root = Page("x", "y", [a])` and `a = Nope("z")` on line 2
- **THEN** rendering throws a PageError whose file is `pages/bad.oui` and line 2, naming `Nope`

#### Scenario: The shell
- **WHEN** `ai/index.html` is read
- **THEN** its body is the noscript message, `<div id="root">...</div>`, the page source script and the site context script, in that order

### Requirement: Site: The bundles are production builds without telemetry
`node tools/build.mjs --bundle` SHALL bundle `src/server.tsx` into `build/server.mjs` (which renders the pages and which the tests import) and `src/client.tsx` into `build/client/site.js` (which the browser runs), both with `process.env.NODE_ENV` defined as `"production"` (OpenUI's web entry mounts a devtools widget, fetched from a CDN, in development, and only the define drops it), OpenUI and zod bundled into the server too so that its test reads the code the browser runs. lang-core's telemetry (which posts to PostHog when the environment asks) MUST be replaced by `src/openui_telemetry.ts`, which does nothing: no bundle carries the analytics' host or key, so no reader's browser and no build can reach them, and a lang-core whose `index.mjs` no longer has the expected shape stops the build naming it. Each bundle is replaced whole by one rename (written to `.next` then renamed), so that a reader of `build/server.mjs` never sees one empty or half written. The only hosts a URL in a bundle may name are those the test lists with a reason: `webcpporg.github.io`, `webcpp.org`, `github.com`, `www.boost.org`, `www.google.com`, `schema.org`, `json-schema.org`, `www.w3.org`, `react.dev`, `zod.dev`, `example.com`.

#### Scenario: No devtools or CDN
- **WHEN** the bundles are searched
- **THEN** they hold no `@openuidev/devtools`, no `https://cdn`, `unpkg` or `esm.sh`, and no `posthog` or `phc_` key

#### Scenario: A bundle held open
- **WHEN** a reader holds `build/server.mjs` open while it is rebuilt
- **THEN** the bundle is replaced by one rename and the reader's file is never truncated

### Requirement: Site: The pages of the site
The site SHALL have exactly eight pages: the home (`''`, "Where C++ and the web meet", the hero with links "See the libraries" and "Get started", the three places the same code runs, the three doors, and an examples section), `libraries/`, `cpp/`, `javascript/`, `ai/`, `examples/`, `community/` and `search/`. The navigation in the header MUST be `Libraries`, `C++`, `JavaScript`, `AI`, `Examples`, `Report`, `Community` and `Search`, each relative to the page, the current one with `aria-current="page"`, folding under a `<details class="menu">` Menu on a phone with no script. The footer says webcpp is free software under the Boost Software License 1.0 with its source on GitHub. A page has one `<h1>`: its Hero's title when its first block is a Hero, else its own.

#### Scenario: The eight pages
- **WHEN** `pages/` is listed
- **THEN** the pages are `''`, `ai/`, `community/`, `cpp/`, `examples/`, `javascript/`, `libraries/` and `search/`

#### Scenario: The header from a deep page
- **WHEN** the page at `libraries/` renders
- **THEN** its header links `../libraries/`, `../cpp/`, `../javascript/` and `../ai/`

### Requirement: Site: The home and the three doors
The home SHALL open with the hero "Where C++ and the web meet" and the lead "webcpp brings the best of C++ to the web, and the libraries web developers use most to C++, for everyone who builds decentralized applications.", then "The same code, wherever it runs" (the three places: the browser compiled to WebAssembly with Emscripten beside the page's JavaScript; a server as a WASI component in a WebAssembly container, run by wasmtime or a cloud; native, compiled by the reader's own compiler), the doors `Doors` (three links "For C++ developers" to `cpp/`, "For JavaScript developers" to `javascript/` and "For AI developers" to `ai/`, each with a sentence) and an examples section linking `examples/`.

#### Scenario: The home links the doors
- **WHEN** the home is rendered
- **THEN** its `<main>` has `<h1>Where C++ and the web meet</h1>` and links `cpp/`, `javascript/`, `ai/` and `examples/`

### Requirement: Site: The libraries page and the catalogue
The libraries page SHALL list every library of the catalogue in a table as Boost lists its own, with the columns Library (linked to its page), Description, Category, Targets and Original (the original a port follows with its version, linked, or "webcpp's own"), a stacked layout with each cell's `data-label` on a phone, and a section "Bring a library" that links `community/`. The catalogue, `libraries.json`, MUST be written by `tools/site/catalogue.py --libs <libs-dir> --declared <file> --out <libraries.json>` from every library's `meta/libraries.json` (read as `tools/doc/libraries.py` reads them) and the targets the build declares: a JSON array of objects `{key, name, description, category, targets, port_of, page, repository}` sorted by `key`, with `indent=4`, `sort_keys=True`, `ensure_ascii=False` and a final newline; `key` must be the directory's name; `port_of` is null or `{name, language, version, url, licence}`; `page` is `libs/<key>/` and `repository` `https://github.com/webcpporg/<key>`; a library's `targets` are the declared lines' targets for it, sorted and without duplicates; on an error it writes nothing, prints `catalogue.py: <file>: <what>` and exits 1.

#### Scenario: One row per library
- **WHEN** the catalogue holds five libraries
- **THEN** the libraries page has five `<tr>` in its `<tbody>`, each with `<a href="../libs/<key>/">`

#### Scenario: A key that is not the directory
- **WHEN** `libs/xactor/meta/libraries.json` has key `actors`
- **THEN** `catalogue.py` exits 1 naming the file

### Requirement: Site: The C++ door
The page `cpp/` ("C++ for WebAssembly: Emscripten, WASI and wasmtime") SHALL teach getting started (install Boost, b2 and a C++20 compiler at the versions the README gives; `git clone --recursive https://github.com/webcpporg/webcpp`, `cd webcpp`, `b2 test`; a b2 project uses a library through `/webcpp/<name>//<name>`, CMake or a plain compiler uses the headers `b2 install --prefix=<dir>` copies), the three ways of building (native with `b2 test example`; WASI with `b2 toolset=clang-wasip2 testing.launcher=wasmtime test example` and wasip3; Emscripten with `b2 toolset=emscripten libs/<name>/test libs/<name>/example`; `b2 declared-targets -d0`), link the README's WebAssembly section and the documentation's index (`docs/`), and list every library of the catalogue with its page (`LibraryList`), never typing a library's name.

#### Scenario: The C++ door links every library
- **WHEN** the page renders with the catalogue
- **THEN** it links `../docs/` and `../libs/<key>/` for every library

### Requirement: Site: The JavaScript door
The page `javascript/` ("Use C++ from JavaScript, in the browser") SHALL list, with `LibraryList("emscripten")`, exactly the libraries that build for emscripten and no other, point to the chat page of trystero's repository (a room that is C++ compiled to WebAssembly meeting its peers over public Nostr relays), and carry a roadmap of what is coming: trystero's JavaScript API (`joinRoom`, `makeAction`, `onPeerJoin` backed by C++, so that a page changes only its import), Trystero's own examples from trystero.dev starting with collaborative cursors, JavaScript functions exported to C++, sizes measured (each library's WebAssembly beside its JavaScript original's, raw, gzip and brotli) and the same code on a server.

#### Scenario: Only emscripten libraries
- **WHEN** the catalogue has libraries with and without the target `emscripten`
- **THEN** the page's `<main>` links `../<page>` for a library exactly when its targets include `emscripten`

### Requirement: Site: The AI door
The page `ai/` ("llms.txt and libraries.json, for AI agents") SHALL explain the two files the site writes at its root for agents, link `llms.txt` and `libraries.json`, and carry a roadmap (integrations with the AIs, documentation written for AI developers), because AI agents are first-class readers of webcpp: they use what it publishes to find libraries of high quality.

#### Scenario: The two links
- **WHEN** the AI door renders
- **THEN** it links `llms.txt` and `libraries.json` relative to the page

### Requirement: Site: The examples page
The page `examples/` ("Examples: C++ and JavaScript, step by step") SHALL say that each example will run live in the visualizer beside a page that explains it, and list as a roadmap, in this order, the examples on their way: the calculator (JavaScript calls C++ with the user's input, which pratt parses and evaluates exact in decimal), the state machine (the same XState JSON run by webcpp's xstate in WebAssembly, beside the browser's own state), C++ calls the browser, the collaborative cursors (Trystero's own example unchanged but for its import), file transfer and video chat (as trystero's roadmap delivers what they need) and on the server (the same program as a WASI component served by wasmtime, and natively). Nothing on the page claims a live example exists.

#### Scenario: The roadmap list
- **WHEN** the examples page renders
- **THEN** its roadmap lists six entries, the first the calculator and the last on the server

### Requirement: Site: The community page and the search page
The page `community/` SHALL link CONTRIBUTING.md, CODING-GUIDELINES.md (the rules and the design decisions), AGENTS.md (how to work in the tree) and the section "Bringing a library" on `https://github.com/webcpporg/webcpp/blob/main/` and the organization's issues, saying a library joins webcpp through an issue that proposes it. The page `search/` SHALL hold a form with `action="https://www.google.com/search"` and `method="get"` whose first `q` is a hidden `site:<channel host>` (`site:webcpporg.github.io` on the preview) and whose second is the reader's own search box (`required`, `autocomplete` off), so that Google joins the two into one query; opened as `search/?q=<terms>`, the JSON-LD's SearchAction, the box starts with those terms. The Programmable Search element replaces the form once the owner creates its engine.

#### Scenario: The search form on the preview
- **WHEN** the search page renders on the preview
- **THEN** its form posts to Google with `<input type="hidden" name="q" value="site:webcpporg.github.io">` and a search input named `q`

#### Scenario: The rules linked from the community page
- **WHEN** the community page renders
- **THEN** it links `https://github.com/webcpporg/webcpp/blob/main/CODING-GUIDELINES.md` as the rules, beside `CONTRIBUTING.md` and `AGENTS.md`

### Requirement: Site: The head of every page
Every page's head SHALL be written by the shell from the page's `Page(title, description, ...)` and the catalogue: `<html lang="en">`; `<title>` of at most 60 characters, the page's subject first, then " - webcpp"; `<meta name="description">` of 120 to 160 characters (a page outside the limits fails with its file and a message naming the description); `<link rel="canonical">`, absolute by channel (`https://webcpporg.github.io/<path>` or `https://webcpp.org/<path>`); one `<meta name="robots">`; Open Graph (`og:title`, `og:description`, `og:url`, `og:type` website, `og:site_name` webcpp, `og:image` with `og:image:width` 1200, `og:image:height` 630 and `og:image:alt`); `twitter:card` `summary_large_image` with its title, description, image and alt; and one JSON-LD `@graph` with `WebSite` (with a `SearchAction` to `search/?q={search_term_string}`), `Organization` (webcpp, `logo` the site's `assets/logo.svg`, `sameAs` `https://github.com/webcpporg`), `WebPage` and `BreadcrumbList` from the page's path; on the libraries page also an `ItemList` of `SoftwareSourceCode` (one per library: `programmingLanguage` "C++", `codeRepository`, `license` `https://www.boost.org/LICENSE_1_0.txt`, `runtimePlatform` from its targets in the order WebAssembly, Emscripten, WASI 0.2, WASI 0.3, native, `isBasedOn` the original of a port). A page has one `<h1>`, headings in order and every image an `alt`. `<meta name="keywords">` is not written.

#### Scenario: A short description
- **WHEN** a page's description is "Too short."
- **THEN** rendering fails with a PageError whose file is the page and whose message mentions the description

#### Scenario: The graph of an ordinary page
- **WHEN** any page but the libraries page is rendered
- **THEN** its graph holds `BreadcrumbList`, `Organization`, `WebPage` and `WebSite` and nothing else

### Requirement: Site: The preview is not indexed
Every page of the preview channel SHALL carry exactly one robots meta, `<meta name="robots" content="noindex">`, because it is the development version (owner, 2026-10-11), and the release's exactly one, `index, follow`, never `noindex`; the preview writes no `sitemap.xml`, and its `robots.txt` still lets every crawler in and names no sitemap, so that a crawler reads each page's noindex. Every page `assemble.py` lays out (of `docs/`, `libs/` and `report/`, the report's pages as `report.py` wrote them included) MUST get `<meta name="robots" content="noindex">` in its head on the preview, and then every page of the site, the site's own too, is checked: on the preview each must carry noindex and nothing else of robots, on the release none may carry noindex, and each page that does not hold is named. Google's search and Search Console belong to webcpp.org, not the preview.

#### Scenario: A preview page
- **WHEN** any page of a preview build is read
- **THEN** its only robots meta is `noindex`

#### Scenario: A library page on the release
- **WHEN** a library's page is laid out for the release
- **THEN** its robots meta is `index, follow`

### Requirement: Site: The indexing files
After the site is laid out, `tools/indexing.mjs --site <out> --base <url> --catalogue <libraries.json> --channel <preview|release> [--superproject <dir>]` SHALL write into `<out>`: `robots.txt` (`User-agent: *` / `Allow: /`, then `User-agent: Google-Extended` / `Allow: /`, and on the release `Sitemap: <base>sitemap.xml`); `sitemap.xml` on the release alone (every `<out>/**/index.html` as `<base><path>`, sorted, each with `<lastmod>` the date, `YYYY-MM-DD`, of the last commit that changed its source: the `.oui` under the site's `pages/`, with `--superproject` a library's repository for its page and `doc/` for `docs/`; a page with no known or committed source has none); `llms.txt` (`# webcpp`, a quote of the home's description, `## Libraries` with a line per library, `## Pages` with a line per other page with its title and description, `## Data` with `libraries.json`); and `libraries.json` (the catalogue copied as it is). `<base>` is the channel's absolute URL and MUST end with `/`, the one place the site's URLs are absolute; a base without its final slash and a channel other than preview or release fail, naming the option. The same options are those of `build.mjs --index`.

#### Scenario: The release's robots.txt
- **WHEN** the release is indexed with base `https://webcpp.org/`
- **THEN** `robots.txt` allows every crawler, names `Google-Extended`, and ends with `Sitemap: https://webcpp.org/sitemap.xml`

#### Scenario: A base without a slash
- **WHEN** `--base https://webcpp.org` is given
- **THEN** the script exits 2 saying the base is not an absolute http(s) URL ending with "/"

### Requirement: Site: The queries the site answers
`seo/queries.json` SHALL list the searches the site answers, each `{"query": ..., "page": ...}` with a page of `pages/`, including "C++ for the web", "C++ in the browser" and "WebAssembly containers C++" (the home), "C++ WebAssembly libraries", "header-only C++20 libraries", "Emscripten C++ library", "C++ state machine XState" and "C++ Pratt parser" (`libraries/`), "WASI C++" and "C++ wasmtime" (`cpp/`), "C++ WebRTC", "C++ peer to peer browser", "use C++ from JavaScript" and "call JavaScript from C++" (`javascript/`), "llms.txt C++ libraries" (`ai/`) and "C++ examples in the browser" (`examples/`). A test MUST check that each query's words (less a, an, the, and, or, of, for, to, in, on, from, with, by) appear in its page's title, `<h1>` or description. Each library adds the queries of its field.

#### Scenario: A query the page never says
- **WHEN** a query's page has none of its words in the title, `<h1>` and description
- **THEN** `seo_test` fails naming the query and the word

### Requirement: Site: Social cards, icons and the logo
The build SHALL render a social card per page and per library (`cards/<path or home>.png`, `cards/libs/<key>.png`, 1200 by 630) from `assets/card.svg` with the logo's drawing and the page's title set in Inter (SIL OFL 1.1, `@expo-google-fonts/inter`, pinned by the lockfile and loaded alone, `loadSystemFonts: false`) by `@resvg/resvg-js`, so that a card is the same bytes on every machine; it renders the icons from `assets/logo.svg` (`assets/icons/icon-accent-192.png` and `-512.png`, `apple-touch-icon.png` at 180 px, `favicon.ico` with PNGs at 16, 32 and 48 px, and `assets/logo-light.svg`, the mark in the light theme's colours alone), so that no image is drawn twice. The logo is the owner's choice of 2026-10-11 (concept b2b): the globe in ink with the braces' arms, the `< >` in the accent, the accent violet (`#5b3fd9` light, `#9d8cff` dark); on the app icons' accent ground the angle brackets are black (3 to 1 at least against the ground, since the near-black ink would give 2.53). The app icons are square and full-bleed with the mark inside a maskable icon's safe zone (80%).

#### Scenario: A card
- **WHEN** `cards/cpp.png` is read
- **THEN** it is a PNG whose IHDR says 1200 by 630, identical bytes on every render

#### Scenario: Contrast of the mark on the accent
- **WHEN** the app icon is drawn
- **THEN** the angle brackets are black on the accent ground

### Requirement: Site: The theme and mobile first
The site's stylesheets (`assets/chrome.css`, `assets/site.css`) SHALL be mobile first with layouts written from the narrowest screen up (360 px, then breakpoints at 768, 1280 and 1440), light and dark by `prefers-color-scheme` (the dark accent `#9d8cff`, the fill `#6246ea`), with `prefers-reduced-motion` honoured, system fonts and no web fonts. Every colour of text MUST be 4.5 to 1 at least on the background and on the surface in both themes (`--text`, `--muted`, `--accent` on `--background` and `--surface`, and white on `--accent-fill`); a pointer's hover never draws the keyboard's focus ring (no rule with `:hover` sets `outline`); the sections' body copy is muted on the opening page alone (`:has(> .hero)`), never on a page of content. The libraries' pages get `assets/chrome.css` alone, which styles the site's header and nothing else, so that a page Asciidoctor styled keeps its look.

#### Scenario: Contrast
- **WHEN** `theme_test` computes the contrast of every text colour in both themes
- **THEN** each is at least 4.5

#### Scenario: Hover and focus
- **WHEN** a rule's selector contains `:hover`
- **THEN** it declares no `outline`

### Requirement: Site: The libraries' pages in the site
`tools/ci/assemble.py --pages DIR --docs ROOT --report DIR|- --chrome FILE --site-chrome FILE --catalogue FILE --base URL --channel preview|release --out DIR` SHALL lay out the site in DIR (empty or nonexistent): the site's own pages with `assets/` and `cards/`, `docs/` (what `ROOT/doc/html` holds), `libs/<name>/` (what `ROOT/libs/<name>/doc/html` holds) and `report/` (the test matrix, or with `--report -` one page saying no results were given). Every page of `docs/` and `libs/` and the report's page of the second kind gets the site's header inserted after its `<body>` with each relative link rebased from the page and `<link rel="stylesheet">` to `assets/chrome.css` before its `</head>`; Asciidoctor's pages, light alone, get `--chrome` (the light mark), the report's page of the second kind (the site's own style, turning dark with the reader's theme) gets `--site-chrome`. A library's page gets the head the site's pages have, from the catalogue: the page's own title `<name>: <subject>` as Asciidoctor wrote it, then " - webcpp", at most 60 characters (a page whose title does not start with "<name>: ", or is longer, fails naming the library); the catalogue's description; canonical, Open Graph and Twitter card with the image `cards/libs/<key>.png`; and JSON-LD `SoftwareSourceCode` (`isBasedOn` the original of a port), `TechArticle` and `BreadcrumbList`. Exit 0 with the site written; 1 when an input is missing or disagrees or a page's robots does not agree with its channel, each fault named; 2 on a usage error or when DIR exists and is not empty.

#### Scenario: A library title too long
- **WHEN** a library's page title with " - webcpp" exceeds 60 characters
- **THEN** `assemble.py` exits 1 naming the library

#### Scenario: Rebased header links
- **WHEN** the site's header is inserted into `libs/xactor/index.html`
- **THEN** its links read `../../libraries/` and so on, relative from the page

### Requirement: Site: The link check
`tools/ci/assemble.py --check DIR` SHALL check the site in DIR once the indexing files, which the pages link, are written: the home links `libraries/` and `libraries/index.html` links every library of `libs/`; every `href` and `src` of every page that is not a URL (no scheme, not `//`) names a file of the site, a directory standing for its `index.html`, and its fragment, if any, an id or a name in that page; a path from the server's root fails because the site works at any address, so every link is relative; an index built without `-sWEBCPP_INDEX=site` links out of the site and fails. It exits 0 when every link resolves and 1 when one does not, each named.

#### Scenario: A root-relative link
- **WHEN** a page links `/libraries/`
- **THEN** the check fails naming the page

#### Scenario: A missing fragment
- **WHEN** a link names `libs/pratt/#pratt-nonesuch`
- **THEN** the check fails, naming the page and the fragment

### Requirement: Site: Valid HTML in two passes
`/webcpp/site//valid-html` SHALL check, after `site-pages` has moved `bin/site` into place, that every page of `bin/site/` is valid HTML with html-validate run twice: the site's own pages (every `*.html` outside `docs/` and `libs/`) take every rule of `.htmlvalidate.json`'s `html-validate:recommended` but six turned off, each for a reason in the Jamfile (`doctype-style`, the shell writes `<!doctype html>`; `void-style`, React serialises a void element self-closed; `no-inline-style`, React's hydration-fade overlay; `attribute-boolean-style`, React writes `attr=""`; `attr-case`, React keeps the DOM property's spelling for `autoComplete` and `spellCheck`; `form-dup-name`, the search form's two fields share `name="q"` on purpose), and `docs/` and `libs/*/`, the libraries' Asciidoctor pages, take `.htmlvalidate.docs.json`'s with `no-trailing-whitespace` off there alone (a listing of pratt's guide shows literal trailing whitespace and tabs as the parser's own example output, inside a `<pre>`). It finds pages with `find -H` (which follows `bin/site`) and `-print0 | xargs -0`, never writing a configuration into `bin/site`, and runs each pass only once there is a page for it.

#### Scenario: A page outside docs with trailing whitespace
- **WHEN** a site-own page has trailing whitespace
- **THEN** the build fails naming the file and the rule

#### Scenario: Trailing whitespace in a library's pre
- **WHEN** a library page shows a `<pre>` with trailing whitespace
- **THEN** it passes, since `no-trailing-whitespace` is off for `docs/` and `libs/`

### Requirement: Site: The site's own tests
`b2 /webcpp/site//test` SHALL run the site's `npm test` (which bundles first) and `npm run typecheck`, and the site's `test/` MUST hold `build_test.mjs`, `cards_test.mjs`, `dev_test.mjs`, `icons_test.mjs`, `indexing_test.mjs`, `package_test.mjs`, `pages_test.mjs`, `readme_test.mjs`, `render_test.mjs`, `seo_test.mjs`, `serve_test.mjs`, `swap_test.mjs` and `theme_test.mjs` with `site.mjs` and `fixtures/catalogue.json`. The site's own workflow `.github/workflows/site.yml` runs, on every push and pull request (concurrency per ref, `cancel-in-progress` on pull requests), `npm ci` (with `OPENUI_TELEMETRY_DISABLED=1` and `DO_NOT_TRACK=1`), `npm test`, `npm run typecheck`, builds the pages with the fixture catalogue and this commit and its date, and validates each page with the rules `b2 site` checks them with; nothing is published by it.

#### Scenario: The site's workflow
- **WHEN** a pull request changes `pages/ai.oui`
- **THEN** the Site workflow runs the tests, the type check, the build with the fixture catalogue and html-validate, and publishes nothing

### Requirement: Site: CI builds and publishes the preview
The superproject's `ci.yml` SHALL, after the library workflow, run the job `site` (when the docs job built the pages, `if: always() && needs.ci.outputs.docs == 'true'`, whatever the matrix says, a red matrix included): check out with submodules, install Boost and Node, download the artifacts `docs` (into `bin/pages`) and `report` (into `bin/report`), run `b2 -a toolset=clang-18 site -sWEBCPP_SITE_DOCS=bin/pages -sWEBCPP_SITE_REPORT=bin/report` after `python3 tools/ci/matrix.py register clang-18`, and upload the build `bin/site` links to as the artifact `site` (7 days, hidden files included). On a push to `main` of `webcpporg/webcpp`, the job `publish` MUST commit the artifact as it is, with a `.nojekyll`, as the one commit "The preview of main at <sha>" of an orphan branch `gh-pages` and force-push that branch alone to `webcpporg/webcpporg.github.io`, whose Pages serve it at https://webcpporg.github.io/, using that repository's deploy key (the secret `SITE_DEPLOY_KEY`, written to a mode-600 file, never printed, removed afterwards; GitHub's host key pinned so ssh trusts no host it has not been told of; a missing secret fails the job); runs queue rather than cancel so an older site never replaces a newer one. `gh-pages` is build output, replaced whole.

#### Scenario: A push to main
- **WHEN** `webcpporg/webcpp`'s `main` receives a push
- **THEN** the site job builds the site, `publish` force-pushes it to `gh-pages` of `webcpporg/webcpporg.github.io`, and the preview is served at the root

#### Scenario: A fork's main
- **WHEN** a fork's `main` receives a push
- **THEN** `publish` does not run, since only `webcpporg/webcpp` holds the deploy key

### Requirement: Site: The old address redirects
The job `pages` SHALL, once `publish` has succeeded, deploy the superproject's own Pages, the old address https://webcpporg.github.io/webcpp/, as the two pages of `python3 tools/site/redirect.py --to https://webcpporg.github.io/ --prefix /webcpp/ --out bin/redirect`: `index.html` and `404.html`, which GitHub Pages serves for the prefix itself and for every other path under it, each holding a script that replaces the location with the same path without the prefix at the new address, its query and its fragment kept (`/webcpp/libs/pratt/#pratt-limits` becomes `https://webcpporg.github.io/libs/pratt/#pratt-limits`), and, for a browser that runs no script, a meta refresh to the new root and a link to it; its canonical link names the new root and it carries noindex, as every page of the development version does. `--to` MUST be an https address ending with a slash and `--prefix` a path between slashes, each of letters, digits and `. _ ~ -` alone; any other value is a usage error with exit 2 and nothing written. The script's function `target(location)` stands between `/* target: begin */` and `/* target: end */` so that `redirect_test.py` can run it alone, and both pages are valid HTML by the site's own rules.

#### Scenario: A deep link at the old address
- **WHEN** a browser opens `https://webcpporg.github.io/webcpp/libs/pratt/#pratt-limits`
- **THEN** it is sent to `https://webcpporg.github.io/libs/pratt/#pratt-limits`

#### Scenario: A bad prefix
- **WHEN** `--prefix /web cpp/` is given
- **THEN** `redirect.py` exits 2 and writes nothing

### Requirement: Site: What the site does not do yet
The site as built SHALL ship no live example, no visualizer, no WebAssembly of its own, no service worker, no web manifest and no precache list, and its pages MUST say so: its examples are a roadmap, the JavaScript door lists what is coming, and `npm run dev` fetches the examples' wasm only when the preview or the superproject's `bin/site/` has some. It makes no connection on load beyond the page's own origin: no server, no runtime API, no cookie, and the only third-party destination is Google's search, reached by the reader's submit on the search page. The release channel at https://webcpp.org/ is built by the same command but not yet published (it waits for the first release and the DNS move), Google's Programmable Search engine and Search Console property are the release site's, and the site's increments 2 to 8 are the roadmap.

#### Scenario: The service worker is absent
- **WHEN** `bin/site/` of this commit is listed
- **THEN** it holds no `sw.js`, no `manifest.webmanifest` and no `precache.json`

#### Scenario: The reader searches
- **WHEN** a reader submits the search form
- **THEN** the browser navigates to google.com/search with the site restriction, and nothing contacted Google before
