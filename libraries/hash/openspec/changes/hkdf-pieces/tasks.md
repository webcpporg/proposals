# Tasks

Implemented before crypto's `hpke` (owner decision 6; design, Migration Plan). Each task writes its
tests first; the library builds at the end of every task. A plan with code goes in
`reference/plan.md` when the proposal is accepted.

## 1. The names

- [ ] 1.1 Rename HKDF's parameters, `hkdf_max_info_size`, the `detail` names that mirror them, `scrypt_parameters`' and `argon2_parameters`' fields; the pages, the examples, `doc/capabilities.json`'s notes and the message tests; the generated files rerun (D7, B3, B4)

## 2. The pieces forms

- [ ] 2.1 `byte_pieces`, its call `{a, b, c}` built on every lane's compiler, the alias or the view type (D1, B2)
- [ ] 2.2 The tags `kdfs::hkdf_pieces`, `hkdf_extract_pieces`, `hkdf_expand_pieces`, their `backend_has_v` from the HMAC cells, their defaults, the capability table's rows and `capabilities_test` (D4)
- [ ] 2.3 The three overloads and their guards, `detail::hkdf_pieces_engine` on `backends::hash2`; the equality, long-input and allocation tests (D1, D2, D3, D8)
- [ ] 2.4 The engines of `sodium`, `bearssl`, `openssl` and `botan` over their HMAC, each with its equality, long-input and allocation tests (D4, D8)

## 3. The asynchronous forms and WebCrypto

- [ ] 3.1 `async_hkdf_extract`, `async_hkdf_expand` (both overloads) and `async_hkdf`'s pieces overload on the synchronous backends; the temporary-list test (D6, B5)
- [ ] 3.2 `backends::webcrypto`'s pieces engine over `subtle.sign("HMAC", ...)`: the gathered copy, Expand's chain, the zero fill; the browser lane's tests against `backends::hash2` and node's HKDF, and the promise count (D5, D8)

## 4. Timing, the documentation and the delivery

- [ ] 4.1 The timing coverage registries and `timing_during` gain the pieces forms (D8)
- [ ] 4.2 The pages (`kdfs.adoc`, `async.adoc`, the WebCrypto page, `allocation.adoc` from release builds), the changelog's source (D9)
- [ ] 4.3 Full local verification on every lane (native clang and gcc, MSVC 14.3 and 14.5, wasip2, wasip3, Emscripten and the browser lane, the Linux lint), pull requests, merge, one CI at the end
