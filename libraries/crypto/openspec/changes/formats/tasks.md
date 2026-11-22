# Tasks

Implemented after crypto/sign-unified merges and before phase 3 of webcpp::trystero's port of
Trystero 0.27. Each task writes its tests first; the library builds at the end of every task, and
every caller of a removed name moves in the task that removes it. A plan with code goes in
`reference/plan.md` when the proposal is accepted.

## 1. The shared vocabulary

- [ ] 1.1 `<webcpp/crypto/formats.hpp>`: the tags, the point forms, `pem(inner)` and its label, `detail::all_formats`, `formats_of`, `forms_of`, the concepts `encodes` and `decodes`, the refusals with the type's list in the failed requirement, and their message tests (design D1, D3)
- [ ] 1.2 `crypto::encoded<N, Form>`, `max_encoded_size`, the wiping of the secret forms, the free `encode`, `decode` and the span form over `detail::codec`, the member definitions; a counting `operator new` test harness shared by every later task (D2)

## 2. Typed signatures

- [ ] 2.1 `ed25519::signature`, `p256::ecdsa_signature`, `secp256k1::ecdsa_signature`, `recoverable_signature` and `schnorr_signature`; every `sign` returns them, `crypto::verify` and `recover` take them through the span, the asynchronous completions carry them, `normalize_low_s` and `is_low_s` take them; every caller moved (D6, R3)

## 3. Each curve's codecs, the old names removed

- [ ] 3.1 Ed25519: `raw`, `spki`, `pkcs8`, `pem(F)`; its signature's `raw`; `encode_spki`, `decode_spki`, `encode_pkcs8`, `decode_pkcs8` and the constants removed; callers moved (D4, D13)
- [ ] 3.2 P-256: the same, and the ECDSA signature's `raw` and `der`; `to_der`, `from_der`, `max_der_size` removed; `compressed` refused on BearSSL (D4, D5)
- [ ] 3.3 secp256k1: the same, the point forms (uncompressed by default, R1), `raw(xonly)` decoding to the even-y key (R5), the recoverable and Schnorr signatures' `raw`; the private `public_key::encoded<N>` renamed (D13)
- [ ] 3.4 PEM: the namespace `crypto::pem` moved into `detail`, `families::pem` kept, the capability tables and `crypto.json` regenerated (D13)
- [ ] 3.5 The browser backend's stand-in: the private formats refused on a backend whose `asynchronous_backend_v` is true (D11)

## 4. JWK

- [ ] 4.1 The target `/webcpp/crypto//jwk`, `<webcpp/crypto/jwk.hpp>`, the refusal without it, and a test that builds every core header without document's include path (D12)
- [ ] 4.2 Encoding to text: the canonical members and sizes of D7 for Ed25519, P-256 and secp256k1, through the PEM family's base64url; RFC 8037's appendix and WebCrypto's `importKey("jwk")` as oracles (D7)
- [ ] 4.3 Decoding text: the external image and the monotonic working state over the fixed buffer, the one selector, the five fields, the refusals, `jwk_too_large`, the wipe; measure the largest skipped member and the `max_bytes` that never exhausts the working state, in builds with and without exceptions, and record both in the guide (D8, R9, R10)
- [ ] 4.4 Documents: decoding a value with no allocation, encoding into a new document and into a value (D9)
- [ ] 4.5 `crypto::thumbprint` with RFC 7638's and RFC 8037's vectors (D10, R8)

## 5. Fuzzing

- [ ] 5.1 A fuzz target per decoder (`raw`, `spki`, `pkcs8`, `der`, `pem(spki)`, `pem(pkcs8)`, JWK text, JWK document value), each seeded, asserting no crash, no heap, the round trip and the wipe; a bounded CI run, a longer local run before the merge (Risks)

## 6. The page, the generated files and the delivery

- [ ] 6.1 The guide's chapter on formats with its table generated from `detail::all_formats`, the before and after, JWK's bounds and the private JWK in a document; `crypto.json` and its generated files; the changelog line
- [ ] 6.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end
- [ ] 6.3 webcpp::trystero's phase 3 told of the new names (its stream adapts)
