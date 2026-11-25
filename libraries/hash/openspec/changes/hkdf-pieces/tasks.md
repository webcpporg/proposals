# Tasks

Implemented before crypto's `hpke` (owner decision 6; design, Migration Plan). Each task writes its
tests first; the library builds at the end of every task. A plan with code goes in
`reference/plan.md` when the proposal is accepted.

Built (2026-11-23 to 2026-11-25) in the plan's seven tasks, on the branches `hash/hkdf-pieces` of
webcpporg/hash and the superproject (the pointer, `tools/node-test/node-pre.js`), each task
reviewed clean, then a whole-branch review and its fix round. Where the code differs from the
design or the plan, `design.md` says so under "As built".

## 1. The names

- [x] 1.1 Rename HKDF's parameters, `hkdf_max_info_size`, the `detail` names that mirror them, `scrypt_parameters`' and `argon2_parameters`' fields; the pages, the examples, `doc/capabilities.json`'s notes and the message tests; the generated files rerun (D7, B3, B4)

## 2. The pieces forms

- [x] 2.1 `byte_pieces`, its call `{a, b, c}` built on every lane's compiler, the alias or the view type (D1, B2)
- [x] 2.2 The tags `kdfs::hkdf_pieces`, `hkdf_extract_pieces`, `hkdf_expand_pieces`, their `backend_has_v` from the HMAC cells, their defaults, the capability table's rows and `capabilities_test` (D4)
- [x] 2.3 The three overloads and their guards, `detail::hkdf_pieces_engine` on `backends::hash2`; the equality, long-input and allocation tests (D1, D2, D3, D8)
- [x] 2.4 The engines of `sodium`, `bearssl`, `openssl` and `botan` over their HMAC, each with its equality, long-input and allocation tests (D4, D8)

## 3. The asynchronous forms and WebCrypto

- [x] 3.1 `async_hkdf_extract`, `async_hkdf_expand` (both overloads) and `async_hkdf`'s pieces overload on the synchronous backends; the temporary-list test (D6, B5)
- [x] 3.2 `backends::webcrypto`'s pieces engine over `subtle.sign("HMAC", ...)`: the gathered copy, Expand's chain, the zero fill; the browser lane's tests against `backends::hash2` and node's HKDF, and the promise count (D5, D8)

## 4. Timing, the documentation and the delivery

- [x] 4.1 The timing coverage registries and `timing_during` gain the pieces forms (D8)
- [x] 4.2 The pages (`kdfs.adoc`, `async.adoc`, the WebCrypto page, `allocation.adoc` from release builds), the changelog's source (D9)
- [ ] 4.3 Full local verification on every lane (native clang and gcc, MSVC 14.3 and 14.5, wasip2, wasip3, Emscripten and the browser lane, the Linux lint), pull requests, merge, one CI at the end
  - [x] Native clang (debug and release), gcc-15, wasip2, wasip3, Emscripten (debug and release,
    `async_webcrypto` under node), the C suites, the tools' tests, the doc tables and MrDocs pass
  - [x] MSVC 14.3 and 14.5, x64 and arm64: 795 passed, 0 failed each, after the fix of their
    findings (the C2102 and C4459 errors, `/bigobj` for `timing_coverage_botan`)
  - [x] The Linux lint `--full` and the macOS lint clean
  - [x] The Emscripten flake of `async_heavy` (node issue #64274's deadlock in `process.exit`)
    worked around in the superproject's `node-pre.js`; hash's and crypto's Emscripten lanes rerun
  - [x] The whole-branch review's findings fixed, or recorded as follow-ups below
  - [ ] Every other library's Emscripten lanes rerun on the new `node-pre.js`, which every test
    under node links
  - [ ] The pull requests in webcpporg/hash, webcpporg/proposals and webcpporg/webcpp (the
    pointer of libs/hash); merge commits; one CI at the end, green on GitHub

## Open

- [ ] BearSSL's refusal of the single-buffer `hkdf_extract` and `hkdf_expand` could also name the
  forms over `byte_pieces`, which exist there now (its text and its two message tests).
- [ ] Expand's guard calls the shared output-and-salt guard with a salt of 0 bytes; an output
  guard of its own would read better.
- [ ] `async_hkdf_extract`'s helper zeroes the pseudorandom key on any error of `hkdf_extract`,
  relying on its own prechecks being exactly `hkdf_extract`'s guards.
- [ ] The wipes on a backend's failure have no fault-injection seam on `hash2`, `sodium` and
  `bearssl`, whose failure paths are near unreachable (OpenSSL's and Botan's are tested).
- [ ] `node-pre.js` calls `process.exit` on node's next turn again, without the 10 s grace, once
  the pinned node is a release with node PR #66171.
