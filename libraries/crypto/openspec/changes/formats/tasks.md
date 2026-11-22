# Tasks

Implemented after crypto/sign-unified and crypto/uniform-types merge, and before phase 3 of webcpp::trystero's port of Trystero 0.27. Each task writes its tests first; the
library builds at the end of every task, and every caller of a removed name moves in the task that
removes it. A plan with code goes in `reference/plan.md` when the proposal is accepted.

## 1. The shared vocabulary

- [ ] 1.1 `<webcpp/crypto/formats.hpp>`, including no curve and no backend, with the stand-in text format that proves the room for JWK (R17): the tags, the point forms, `pem(inner)` and its label, `detail::all_formats`, `formats_of`, `forms_of`, the concepts `encodes` and `decodes`, the refusals with the type's list in the failed requirement, and their message tests (design D1, D3)
- [ ] 1.2 `crypto::encoded<N, Form>`, `max_encoded_size`, the wiping of the secret forms, the free `encode`, `decode` and the span form over `detail::codec`; a counting `operator new` test harness shared by every later task (D2)

## 2. Each curve's codecs and members in its own formats header, the old names removed

- [ ] 2.1 Ed25519: `raw`, `spki`, `pkcs8`, `pem(F)`, its signature's `raw`, the members; the old functions and constants removed; callers moved; the "builds without X" tests extended to the formats header (D2, D4, D8, R14)
- [ ] 2.2 X25519: the same over uniform-types' typed keys; its old functions removed (D4, owner decision 13)
- [ ] 2.3 P-256: the same, the ECDSA signature's `raw` and `der`, `compressed` on every backend with the square root on BearSSL against Wycheproof's and OpenSSL's compressed points; `to_der`, `from_der`, `max_der_size` removed (D5, D6, R16)
- [ ] 2.4 secp256k1: the same, the point forms (uncompressed by default), `raw(xonly)` decoding to the even-y key, the recoverable and Schnorr signatures' `raw`; the private `public_key::encoded<N>` renamed (D4, R1, R5, D8)
- [ ] 2.5 PEM: the namespace `crypto::pem` moved into `detail`, `families::pem` kept, the capability tables and `crypto.json` regenerated (D8)
- [ ] 2.6 The browser backend's stand-in: the private formats refused on a backend whose `asynchronous_backend_v` is true (D7)

## 3. Fuzzing

- [ ] 3.1 A fuzz target per decoder (`raw`, `spki`, `pkcs8`, `der`, `pem(spki)`, `pem(pkcs8)`), each seeded, asserting no crash, no heap, the round trip and the wipe; a bounded CI run, a longer local run before the merge (Risks)

## 4. The page, the generated files and the delivery

- [ ] 4.1 The guide's chapter on formats with its table generated from `detail::all_formats`, the before and after, and the deferred JWK's chain; `crypto.json` and its generated files; the changelog line
- [ ] 4.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end
- [ ] 4.3 webcpp::trystero's phase 3 told of the new names (its stream adapts)
