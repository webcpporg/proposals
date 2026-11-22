# Tasks

Implemented after crypto/sign-unified, crypto/uniform-types and document/bounded-parse merge, and
before phase 3 of webcpp::trystero's port of Trystero 0.27. Each task writes its tests first; the
library builds at the end of every task, and every caller of a removed name moves in the task that
removes it. A plan with code goes in `reference/plan.md` when the proposal is accepted.

## 1. The shared vocabulary

- [ ] 1.1 `<webcpp/crypto/formats.hpp>`, including no curve and no backend: the tags, the point forms, `pem(inner)` and its label, `detail::all_formats`, `formats_of`, `forms_of`, the concepts `encodes` and `decodes`, the refusals with the type's list in the failed requirement, and their message tests (design D1, D3)
- [ ] 1.2 `crypto::encoded<N, Form>`, `max_encoded_size`, the wiping of the secret forms, the free `encode`, `decode` and the span form over `detail::codec`; a counting `operator new` test harness shared by every later task (D2)

## 2. Each curve's codecs and members in its own formats header, the old names removed

- [ ] 2.1 Ed25519: `raw`, `spki`, `pkcs8`, `pem(F)`, its signature's `raw`, the members; the old functions and constants removed; callers moved; the "builds without X" tests extended to the formats header (D2, D4, D13, R14)
- [ ] 2.2 X25519: the same over uniform-types' typed keys; its old functions removed (D4, owner decision 13)
- [ ] 2.3 P-256: the same, the ECDSA signature's `raw` and `der`, `compressed` on every backend with the square root on BearSSL against Wycheproof's and OpenSSL's compressed points; `to_der`, `from_der`, `max_der_size` removed (D5, D6, R16)
- [ ] 2.4 secp256k1: the same, the point forms (uncompressed by default), `raw(xonly)` decoding to the even-y key, the recoverable and Schnorr signatures' `raw`; the private `public_key::encoded<N>` renamed (D4, R1, R5, D13)
- [ ] 2.5 PEM: the namespace `crypto::pem` moved into `detail`, `families::pem` kept, the capability tables and `crypto.json` regenerated (D13)
- [ ] 2.6 The browser backend's stand-in: the private formats refused on a backend whose `asynchronous_backend_v` is true (D11)

## 3. JWK

- [ ] 3.1 The target `/webcpp/crypto//jwk`, `<webcpp/crypto/jwk.hpp>`, the refusal without it, and a test that builds every core header without document's include path (D12)
- [ ] 3.2 Encoding to text: the canonical members and sizes of D7 for the four curves, through the PEM family's base64url; RFC 8037's appendix and WebCrypto's `importKey("jwk")` as oracles (D7)
- [ ] 3.3 The private reader: JSON's grammar in constant memory, escapes in keys and public values, `d` read without a branch on its bytes, the wipe; its constant-time harness in the lane for the four key pair types (D8, R10)
- [ ] 3.4 The public reader over document's bounded-parse mode: the five selectors, the `bounded_resource` on the stack, `jwk_too_large`, the optional `jwk_options`; measure the working buffer's need and record it in the guide (D8, R9)
- [ ] 3.5 Documents: decoding a value with no allocation, the private-key rule in its documentation, encoding into a new document and into a value (D9)
- [ ] 3.6 `crypto::thumbprint` with RFC 7638's and RFC 8037's vectors (D10, R8)

## 4. Fuzzing

- [ ] 4.1 A fuzz target per decoder (`raw`, `spki`, `pkcs8`, `der`, `pem(spki)`, `pem(pkcs8)`, private JWK text, public JWK text, JWK document value) and the differential target over both JWK readers, each seeded, asserting no crash, no heap, the round trip and the wipe; a bounded CI run, a longer local run before the merge (Risks)

## 5. The page, the generated files and the delivery

- [ ] 5.1 The guide's chapter on formats with its table generated from `detail::all_formats`, the before and after, JWK's two readers and the private JWK in a document; `crypto.json` and its generated files; the changelog line
- [ ] 5.2 Full local verification on every lane (native, Emscripten, wasip2, wasip3, MSVC, gcc, the constant-time lane, the Linux lint), pull requests, merge, one CI at the end
- [ ] 5.3 webcpp::trystero's phase 3 told of the new names (its stream adapts)
