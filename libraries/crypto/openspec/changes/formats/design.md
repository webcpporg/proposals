## Context

This change builds on three others, merged before it:

- **crypto/sign-unified, as merged**: typed public keys (`ed25519::public_key`,
  `p256::basic_public_key<B>`, `secp256k1::public_key`), scheme tags as values
  (`crypto::ecdsa_sha256`, ...), one list of schemes (`detail::all_schemes`), the backend first
  among template parameters, and four mirrored forms per operation (member and free, synchronous
  and asynchronous; its D9).
- **crypto/uniform-types** (`libraries/crypto/openspec/changes/uniform-types/`): a type for every
  signature (`ed25519::signature`, `p256::ecdsa_signature`, `secp256k1::ecdsa_signature`,
  `recoverable_signature`, `schnorr_signature`), X25519's typed keys (`x25519::public_key`,
  `x25519::basic_keypair<B>`), and the key pair concepts split into `keypair`, `signing_keypair`
  and `agreement_keypair`.
- **document/bounded-parse** (`libraries/document/openspec/changes/bounded-parse/`): a parse mode
  of document's streaming parser that skips the values no selector can match without buffering or
  building them, keeps its working state in the caller's memory, and reports exhaustion as an
  error value, with or without exceptions.

The formats as they are on crypto/sign-unified:

| Curve | Public key | Key pair | Signature |
|---|---|---|---|
| Ed25519 | `encode_spki` (44), `decode_spki`; `public_key::make`, `bytes()` (32) | `encode_pkcs8` (48), `decode_pkcs8`; `make(seed)`, `secret()` | 64-byte array |
| X25519 | `encode_spki` (44), `decode_spki` giving `std::array<uint8_t, 32>` | `encode_pkcs8` (48) of a `secret<32>`, `decode_pkcs8` giving `secret<32>` | none |
| P-256 | `encode_spki` (91), `decode_spki`; `basic_public_key::make`, `uncompressed()` (65) | `encode_pkcs8` (138), `decode_pkcs8`; `make(scalar)`, `secret()` | 64-byte array; `to_der` (8 to 72), `from_der` |
| secp256k1 | `encode_spki` (56 compressed by default, 88 uncompressed), `decode_spki` (either); `public_key::make` (33 or 65), `compressed()`, `uncompressed()`, `xonly()` | `encode_pkcs8` (135), `decode_pkcs8`; `make`, `secret()` | 64-byte array (65 recoverable); `to_der`, `from_der` |

and the root's `pem::armour(label, der, out)` and `pem::unarmour(label, text, out)` (RFC 7468,
64-character lines, LF, strict), on libsodium's constant-time base64 (`families::pem`). Every
encoder writes into a caller's span sized by a per-curve constant; every decoder accepts exactly
the bytes its encoder writes (foundation D30). The foundation's requirement "Key formats" says
"JWK MUST NOT be offered".

## Intent

A call site should read as what it does, and a mistake should not compile:

```cpp
auto const pem_text = crypto::encode(crypto::pem(crypto::spki), public_key);   // the label is right by construction
auto const key = crypto::decode<p256::keypair>(crypto::jwk, text);             // a JWK into a key pair, d in constant time
auto const der = crypto::encode(crypto::der, signature);                       // only an ECDSA signature compiles
```

Three properties motivate the change: readability (one verb per direction, the format named at the
call as the scheme is), ease (no buffer to size, no label to spell, no per-curve name to find), and
compile-time errors (a format the type does not have is a diagnostic that lists the ones it has,
not a missing name).

## Goals / Non-Goals

**Goals:** one `encode` and one `decode` for every key and signature format of Ed25519, X25519,
P-256 and secp256k1, in four mirrored forms with one implementation; format tags as values, PEM
composing over the DER formats with its label taken from them; a fixed result with no heap, and a
span form; a concept that gates every pair with the type's list of formats in the diagnostic; one
list of formats from which the concepts, the tests and the guide derive; JWK in its own header and
target, encoding with no heap, a private JWK's text read in constant time and constant memory, a
public JWK's text read in bounded memory whatever its size; RFC 7638's thumbprint; every caller in
libs/crypto moved; the old names removed.

**Non-goals:** X.509 and certificates (another session's change); asynchronous forms of encode and
decode (pure functions over bytes the program holds); compressed PEM; encrypted PKCS#8 (PKCS#5,
`ENCRYPTED PRIVATE KEY`); curves and key types crypto does not have today (P-384, P-521, RSA, and
the keys only OpenSSL or Botan have): each joins the formats with its own change; JWK's `RSA` and
`oct` keys; JWS, JWE and JWT themselves; a public base64url function.

**Anti-goals:** a format chosen at run time (an enum or a string) on the main path, which would
lose the compile-time size and the compile-time refusal; any function that infers a curve or a
format from a number of bytes; a PEM label a caller spells; a private key's `d` read by a parser
whose time depends on it.

## Approaches

**A. The format as a template parameter, `crypto::encode<crypto::spki>(key)`.** It reads well for
a plain tag, but a composed or parameterized format (`pem(spki)`, `spki(compressed)`) becomes a
type expression (`encode<pem_t<spki_t>>`) or a class-type template argument whose diagnostics are
poor; a member form in a dependent context needs `key.template encode<spki>()`; and the schemes,
which this change mirrors, are values. Rejected.

**B. Members only, `key.encode(crypto::spki)` and `T::decode(...)`.** The member reads well and is
kept as a mirror, but generic code (a test over every pair, a program that stores keys of several
curves) wants one free name, and the owner chose the free function as the main form. Rejected as
the only form.

**C. Per-format names, as today.** Twelve names per curve and a constant per buffer; an invalid
pair is a missing name; PEM's label is a string. Rejected (the reason for this change).

**D. Pipes and stream manipulators, `key | crypto::pem(crypto::spki)` or
`out << crypto::pem << key`.** A manipulator is hidden state in the stream between calls, an error
becomes a stream flag or an exception instead of a `result`, the output goes through a stream that
allocates and costs size on wasm, and a pipe of `|` reads as a bitwise or to a C++ reader. Rejected
for hidden state and error handling.

**E. A run-time format, `encode(format_id::spki, key)`.** No compile-time maximum, so no fixed
result; an invalid pair is a run-time error. Rejected.

**F. Return values.** (a) a heap container (`std::vector`, `std::string`): simple, but a heap
allocation per call on paths that allocate nothing today, and a secret on the heap; (b) a fixed
buffer sized at compile time, with a span overload for callers that manage memory (chosen by the
owner); (c) the span alone: today's form, which makes every caller size a buffer.

**G. Reading a JWK text.** (a) document's streaming parser as it is: it buffers and builds every
member, selected or not, and in a build without exceptions an exhausted memory resource ends the
program (D8's finding), so a bound becomes a size cap and `d` goes through code whose time
depends on it; (b) one reader of crypto's own for every JWK: constant time and constant memory,
but a second JSON reader for the public path, which document serves; (c) crypto's reader for a
private JWK, document's bounded-parse mode for a public one (chosen by the owner).

**H (chosen).** Tags as values passed first, the free function as the main form with mirrored
members, a fixed result, one list, concepts with the list in the diagnostic, JWK in its own target
with G (c).

## Owner decisions (2026-11-22, binding)

First round:

1. **Format tags are values passed as the first argument**, as the schemes are: `crypto::raw`,
   `crypto::spki`, `crypto::pkcs8`, `crypto::der` (an ECDSA signature), `crypto::jwk`, and
   `crypto::pem(F)`, which composes over `spki` or `pkcs8` and takes its label (`PUBLIC KEY`,
   `PRIVATE KEY`) from the inner format, so that the label can never be wrong. One list of formats,
   as `all_schemes`, from which the concepts and the documentation derive.
2. **Free functions are the main form**, `crypto::encode(format, value)` and
   `crypto::decode<T>(format, input)`, with mirrored members `value.encode(format)` and
   `T::decode(format, input)`, one implementation.
3. **Return values, option (b)**: a ready value with no heap, `result<encoded<N>>`, a fixed buffer
   whose N is the format's maximum for that type at compile time, with the actual length and
   `bytes()` / `text()` views; plus an overload writing into a caller's `std::span`.
4. **A concept gates every pair** (`encodes<Format, T>`, `decodes<Format, T>`), with a message
   listing the formats valid for that type. Public keys: `raw`, `spki`, `pem(spki)`, `jwk`; key
   pairs: `pkcs8`, `pem(pkcs8)`, `jwk` (private, with `d`); a key pair's `raw` is its `secret()`
   (not duplicated; ruled in R2); ECDSA signatures (P-256, secp256k1): `der`; Ed25519 and Schnorr
   signatures refuse `der`.
5. **Old names are removed with no aliases**: `encode_spki`, `decode_spki`, `encode_pkcs8`,
   `decode_pkcs8`, `to_der`, `from_der`, `pem::armour` and `pem::unarmour` as public API, and the
   `max_*_size` constants the new form supersedes; every caller migrates.
6. **JWK is in this change** (RFC 7517, 7518, 8037, 8812; EC P-256 and secp256k1 with `x`, `y` and
   optionally `d`; OKP Ed25519 and X25519 with `x` and optionally `d`; base64url without padding),
   in its own header `<webcpp/crypto/jwk.hpp>` and target `/webcpp/crypto//jwk`, which pulls
   webcpp::document; the core of crypto does not depend on document. Encoding to text: a fixed
   buffer, no heap, canonical member order (lexicographic, no white space), so that the text equals
   RFC 7638's thumbprint input; maximum sizes computed. Encoding to a document in the storage the
   user picks. Decoding text in bounded working memory, exhaustion an error (`jwk_too_large`);
   duplicate members among the fields read refused; unknown members ignored; `kty`, `crv` and size
   mismatches refused; a public JWK given to a key pair's decode refused. Decoding an existing
   document value with no new allocation. A thumbprint (RFC 7638) with webcpp::hash's SHA-256.
7. **Out of scope**: X.509 and certificates, asynchronous forms of encode and decode, compressed
   PEM, encrypted PKCS#8.

Second round, answering the first draft's open questions:

8. **A private JWK's text is decoded in constant time** by a small JWK reader of crypto's own,
   which handles `d` in constant time (base64url never needs an escape) and is traced by the
   constant-time lane as PKCS#8 is. webcpp::document stays for a public JWK's decode, for encoding
   to a document and for decoding an existing document value; a private JWK read out of a
   program's document has had its `d` through document's parser, which is not constant time, and
   the rule is stated (D9).
9. **document gains a bounded-memory parse mode first** (document/bounded-parse): unselected
   values skipped without being buffered, exhaustion an error instead of termination. A size cap
   is an option, never the safety mechanism.
10. **Typed signatures are a separate small change merged first** (crypto/uniform-types); this
    change builds on them.
11. **The uncompressed point is the default of `spki` and `raw` on every curve**, the compressed
    point always available by name; secp256k1's SPKI default changes.
12. **A private JWK given to a public key's decode is refused** with an error.
13. **X25519 is typed by uniform-types, so this change covers it**; its old functions are removed
    as the others are.

## Decisions

### D1. The header, the tags and the one list

`<webcpp/crypto/formats.hpp>` (namespace `webcpp::crypto`) declares the format tags, each an empty
type with an explicit default constructor and a `constexpr` object, as `signing.hpp` declares the
schemes:

- `raw_t<Form>` and `raw`, `spki_t<Form>` and `spki`, where `Form` is a point form tag:
  `default_form_t` (the uncompressed point, D5), `compressed_t`, `uncompressed_t`, `xonly_t`, with
  objects `crypto::compressed`, `crypto::uncompressed`, `crypto::xonly`; `raw(crypto::compressed)`
  and `spki(crypto::compressed)` are `constexpr` calls that return `raw_t<compressed_t>{}` and
  `spki_t<compressed_t>{}`.
- `pkcs8_t` and `pkcs8`, `der_t` and `der`, `jwk_t` and `jwk`.
- `pem_t<Inner>` and `crypto::pem`, an object whose `constexpr operator()(Inner)` returns
  `pem_t<Inner>{}` for an `Inner` that is `spki_t<Form>` or `pkcs8_t` and stops the build for any
  other with "PEM carries a DER key: pem(crypto::spki) or pem(crypto::pkcs8)". `pem_t<Inner>`
  holds its label as a `static constexpr std::string_view label`, `"PUBLIC KEY"` for every
  `spki_t<Form>` and `"PRIVATE KEY"` for `pkcs8_t`; no other way to name a label exists.

The one list, beside the tags: `detail::format_list<...>` and `detail::all_formats`, a
`format_list` of `raw_t<default_form_t>`, `spki_t<default_form_t>`, `pkcs8_t`, `der_t`, `jwk_t`,
`pem_t<spki_t<default_form_t>>` and `pem_t<pkcs8_t>`. The point forms are modifiers of a listed
format, not entries; `detail::base_format_t<F>` maps `raw_t<compressed_t>` to
`raw_t<default_form_t>` and so on. `detail::is_format_v` derives from the list; a new format is one
line in the list and one codec per type that has it.

Each value type declares the formats it has in one place, beside its codecs, as the curves declare
their schemes: `detail::formats_of<T>` is a `format_list`, every member of which must be in
`all_formats` (a `static_assert` beside the declaration); `detail::forms_of<Format, T>` lists the
point forms the type accepts.

### D2. The four forms, the result, and where they live

| | Encode | Decode |
|---|---|---|
| Free (main) | `crypto::encode(format, value)` returns `result<encoded<N, Form>>` | `crypto::decode<T>(format, input)` returns `result<T>` |
| Free, caller's memory | `crypto::encode(format, value, output)` returns `result<std::size_t>` | (the free form already writes nothing to the heap) |
| Member | `value.encode(format)` | `T::decode(format, input)` |

One implementation: `detail::codec<Format, T>` holds `static encode_into(value, span)` and
`static decode(input)`; the free functions call it, the members call the free functions.

Where each part lives (the builder's check, a ruling, R14): `formats.hpp` holds the tags, the list,
the concepts, `encoded` and the free functions, generic over `detail::codec`, and includes no curve
and no backend. Each curve's `curves/<curve>/formats.hpp` includes its curve and `formats.hpp`,
specializes `detail::codec` and `formats_of` for its types, and defines its types' members, which
each class declares as member templates with a deduced return type (as unified-sign's D9 declares
the asynchronous members): a curve's own header never includes a formats header, and calling a
member without its curve's `formats.hpp` is the compiler's "deduced return type used before it is
defined". So a program that uses P-256's formats includes `curves/p256/formats.hpp` and pulls no
other backend, and the existing tests that build each family without the others' backends ("builds
without X") stay and gain the formats headers.

`input` is `std::span<std::uint8_t const>` for a binary format (`raw`, `spki`, `pkcs8`, `der`)
and `std::string_view` for a text format (`pem(F)`, `jwk`); each decode takes exactly its form
(the other does not convert), so that PEM text is never read as DER. A JWK also decodes from a
document value (D9).

`crypto::encoded<N, Form>` is a class template with `Form` one of `encoded_form::binary`, `text`,
`secret_binary`, `secret_text`:

- it holds `std::array<std::uint8_t, N>` (or `char` for text) and its length;
- `bytes()` returns `std::span<std::uint8_t const>` of the length, on every form;
- `text()` returns `std::string_view`, on the text forms only (a requires clause);
- `size()`, `max_size()` (N, `static constexpr`), and nothing that converts it implicitly;
- the secret forms wipe their storage on destruction and on move as `secret<N>` does (the detail
  wipe), are move-only, and `clone()` copies explicitly, as `secret<N>` does.

The form follows from the pair: `pkcs8`, `pem(pkcs8)`, a key pair's `jwk` and a key pair's `raw`
are secret; everything else is not. `crypto::max_encoded_size<Format, T>` is N; it replaces every
`max_*_size` constant. The span overload takes `std::span<std::uint8_t>` for a binary format and
`std::span<char>` for a text format, refuses one shorter than `max_encoded_size` with
`invalid_input_size`, and writes nothing on any error, as today's encoders.

Backends: a `T` that names its backend (`p256::basic_public_key<B>`, `basic_keypair<B>`) decodes
on it; a `T` that has none (`ed25519::public_key`, `x25519::public_key`, `secp256k1::public_key`)
decodes on its family's default, as `make` does. PEM and JWK's base64 run on `families::pem`'s
backend (libsodium's constant-time codec), as today, which a program using either links.

### D3. The concepts and the diagnostic

`crypto::encodes<Format, T>` is true when `base_format_t<Format>` is in `formats_of<T>`, the form
is in `forms_of<Format, T>`, and the backend can export it (D11). `crypto::decodes<Format, T>` is
the same for reading. Each free function and member checks its concept in a `static_assert`
before anything else, its message fixed text ("this type has no such format; the formats it has
are the format_list named in this assertion") and its condition
`detail::format_refused<Format, T, detail::formats_of_t<T>>`, whose template arguments every
compiler prints in the failed requirement, so that the list of formats the type has appears in the
diagnostic without a string written by hand. Pairs with a reason of their own add a sentence:
`der` on Ed25519's or BIP340's signature ("Ed25519 and BIP340 signatures have no DER form; their
64 bytes are the signature"), `der` on a recoverable signature ("a recoverable signature has no
DER form"), `jwk` without `<webcpp/crypto/jwk.hpp>` ("include <webcpp/crypto/jwk.hpp> and link
/webcpp/crypto//jwk"), a type whose curve's `formats.hpp` is not included ("include
<webcpp/crypto/curves/<curve>/formats.hpp>"), `pem(der)` and `pem(jwk)` (D1). A test of message
compilation (`webcpp.compile-diagnostic`) pins each.

### D4. The valid pairs

| Type | `raw` | `spki` | `pem(spki)` | `pkcs8` | `pem(pkcs8)` | `der` | `jwk` |
|---|---|---|---|---|---|---|---|
| `ed25519::public_key` | 32 | 44 | 113 | - | - | - | 79 (OKP) |
| `x25519::public_key` | 32 | 44 | 113 | - | - | - | 78 (OKP) |
| `p256::basic_public_key<B>` | 65; `compressed` 33 | 91; `compressed` 59 | 178; `compressed` 134 | - | - | - | 126 (EC) |
| `secp256k1::public_key` | 65; `compressed` 33; `xonly` 32 | 88; `compressed` 56 | 174; `compressed` 130 | - | - | - | 130 (EC) |
| `ed25519::basic_keypair<B>` | 32, secret (R2) | - | - | 48 | 119 | - | 129 (OKP, `d`) |
| `x25519::basic_keypair<B>` | 32, secret (R2) | - | - | 48 | 119 | - | 128 (OKP, `d`) |
| `p256::basic_keypair<B>` | 32, secret (R2) | - | - | 138 | 241 | - | 176 (EC, `d`) |
| `secp256k1::basic_keypair<B>` | 32, secret (R2) | - | - | 135 | 237 | - | 180 (EC, `d`) |
| `p256::ecdsa_signature` | 64 | - | - | - | - | 72 (8 to 72) | - |
| `secp256k1::ecdsa_signature` | 64 | - | - | - | - | 72 (8 to 72) | - |
| `secp256k1::recoverable_signature` | 65 | - | - | - | - | - | - |
| `secp256k1::schnorr_signature` | 64 | - | - | - | - | - | - |
| `ed25519::signature` | 64 | - | - | - | - | - | - |

Each number is `max_encoded_size`, the bytes or characters `encode` writes; every pair marked `-`
stops the build. The sizes of PEM are `armoured_size(label, der)` of today's `pem.hpp`: for P-256,
an SPKI of 91 bytes is 124 base64 characters in 2 lines, 27 characters before and 25 after, 178 in
all; a PKCS#8 of 138 bytes is 184 characters in 3 lines, 28 before and 26 after, 241 in all.
P-256's compressed SPKI is the same AlgorithmIdentifier around a BIT STRING of 34 bytes, 59 in all.
The JWK sizes are D7's.

Decoding takes the same pairs. Each decoder keeps today's strictness (foundation D30): it accepts
exactly what its encoder writes. A modifier names the one form encoded; a decode with the default
form accepts the point forms the type's `make` accepts (secp256k1's `spki` and `raw` take 33 or 65
bytes, as today, and so do P-256's now), and `raw(xonly)` decodes 32 bytes as BIP340's lift of x,
the key of even y (R5).

### D5. The point forms (owner decision 11)

The default form of every EC format is the uncompressed point: `raw` of a P-256 key is
WebCrypto's `raw` (65 bytes), `spki` writes what OpenSSL's `i2d_PUBKEY`, node and WebCrypto write.
`crypto::compressed` is offered on every EC curve and every backend: encoding writes the parity
byte and x, which needs no backend; decoding a compressed P-256 point on BearSSL, which has no
point decompression, computes y with a fixed-width square root modulo p
(p = 3 mod 4, y = (x^3 - 3x + b)^((p + 1) / 4)) on public data in crypto, then checks the point as
`make` does (R16); OpenSSL decompresses itself. `crypto::xonly` is secp256k1's `raw` only. The
public key's accessors `bytes()`, `uncompressed()`, `compressed()` and `xonly()` stay: they are
the value's own bytes, which `raw` calls, not a second format API; P-256's `basic_public_key` gains
`compressed()` on every backend.

### D6. DER on the typed signatures

The signature types come from uniform-types. DER needs the curve (P-256's encoder refuses r or s
not below n; secp256k1's parses through libsecp256k1), which the type gives, and Ed25519's,
BIP340's and the recoverable signature refuse `der` by type. `encode(der, signature)` writes strict
DER (8 to 72 bytes); `decode<p256::ecdsa_signature>(der, bytes)` reads strict DER only, the same
code as today's `to_der` and `from_der` behind the new names.

### D7. JWK: the members and the text

| Key | Members, in the order written | Public | Private |
|---|---|---|---|
| Ed25519 | `crv` `"Ed25519"`, [`d`], `kty` `"OKP"`, `x` | 79 | 129 |
| X25519 | `crv` `"X25519"`, [`d`], `kty` `"OKP"`, `x` | 78 | 128 |
| P-256 | `crv` `"P-256"`, [`d`], `kty` `"EC"`, `x`, `y` | 126 | 176 |
| secp256k1 | `crv` `"secp256k1"`, [`d`], `kty` `"EC"`, `x`, `y` | 130 | 180 |

`x`, `y` and `d` are base64url without padding of 32 bytes, 43 characters each; EC's `x` and `y`
are the uncompressed point's coordinates and `d` the scalar (RFC 7518 section 6.2), OKP's `x` the
public key's 32 bytes and `d` the seed or secret key (RFC 8037). The text is one JSON object, no
white space, members in lexicographic order of their names, which is RFC 7638's order: the public
JWK's text is byte for byte the thumbprint's input; a private JWK's thumbprint is its public key's
(R8). The P-256 public text is `{"crv":"P-256","kty":"EC","x":"` (31 characters), 43, `","y":"`
(7), 43, `"}` (2): 126; the private one adds `"d":"` (5), 43 and `",` (2): 176. The encoder writes
no other member (`alg`, `use`, `key_ops`, `kid`, `ext`): a program that needs them builds a
document (D9) and sets them.

The encoder writes through `families::pem`'s backend (libsodium's constant-time base64url, its
`URLSAFE_NO_PADDING` variant), into the fixed result, with no heap; the text of a private JWK is
`encoded_form::secret_text`.

### D8. JWK: decoding a text

`crypto::decode<T>(crypto::jwk, text)` reads the text by one of two readers, chosen by `T` at
compile time; both apply the same rules:

- a root that is not an object, a text that is not one JSON value, or any of `kty`, `crv`, `x`,
  `y`, `d` present twice: `invalid_encoding`;
- `kty` missing, not a string, or not the type's (`"EC"` for P-256 and secp256k1, `"OKP"` for
  Ed25519 and X25519); `crv` missing or not the type's; `x` (and `y` for EC) missing; any of them
  not a string: `invalid_encoding`;
- `x`, `y` or `d` not strict base64url without padding of exactly 32 bytes (43 characters, the
  unused bits zero, which libsodium's decoder checks): `invalid_encoding`;
- a key pair's decode without `d`: `invalid_encoding` (a public JWK is not a key pair);
- a public key's decode with `d`: `invalid_encoding` (owner decision 12);
- a key pair whose `x` (and `y`) are not `d`'s public key: `invalid_encoding`, as PKCS#8's decode
  refuses a public key that is not the scalar's;
- the point off the curve, a scalar 0 or not below n: `invalid_encoding`, as `spki` and `pkcs8`
  map `make`'s refusals;
- memory exhausted, or the program's optional cap passed: `jwk_too_large`, a new code of
  `crypto::error`.

Unknown members, `alg`, `use`, `key_ops`, `ext` and `kid` included, are ignored, whatever their
value and size, duplicates included. An optional `crypto::jwk_options` (`max_bytes`, no limit by
default; `max_depth`, 64 by default) is a policy a program may add, never what keeps the decoder
safe.

**A private JWK: crypto's own reader** (owner decision 8). `decode<K>(jwk, text)` for a key pair
`K` runs `detail::jwk_reader`, a strict JSON reader of a few hundred lines in `jwk.hpp`, which uses
no document and no allocator: a cursor, a fixed bit stack of `max_depth` bits (object or array at
each level), fixed buffers of 16 bytes for `kty` and `crv`, 64 for `x` and `y`, and 43 for `d`. It
validates all of JSON's grammar (strings and their escapes, UTF-8, numbers, literals, nesting) on
the public bytes and skips every unknown member as it reads it, so that memory is constant whatever
the input's size. Keys are compared as they are read, their escapes decoded, so `"d"` is `d`.
`d`'s value is read without a branch on its bytes: after `"d":"` the reader copies the next 43
bytes, requires the 44th to be `"`, and decodes the 43 with the constant-time base64url, whose
validity is one flag computed over all of them; an escape in `d` is therefore refused, which no
base64url text needs (R10). The time of the read depends on the text's length and structure,
public data, never on `d`. The constant-time lane traces it as it traces PKCS#8: a harness marks
the 43 bytes of `d` in a valid text as secret and runs the decode under the lane's tools, with
the existing suppressions only. Every byte that held `d`, its text or its value, is wiped before
the call returns, on every path.

**A public JWK: document's bounded-parse mode** (owner decision 9). `decode<P>(jwk, text)` for a
public key `P` runs document's bounded parser (document/bounded-parse) with five selectors,
`$.kty`, `$.crv`, `$.x`, `$.y` and `$.d` (the last only to refuse it), over a
`document::bounded_resource` on a stack buffer of `crypto::jwk_working_size` bytes (R9) or a span
the program passes (`decode<P>(jwk, text, working)`). Members no selector names are skipped by the
parser without being buffered or built, whatever their size; a key is compared as it streams and
never buffered unless a selector names it. Each delivery copies its string into the fixed buffers
above and counts it, so a duplicate is seen. The parser reports an exhausted buffer as
`errc::capacity`, an error value with or without exceptions, which is `jwk_too_large`; it is given
the whole text at once, so nothing reaches the global heap. If a private JWK reaches this path, its
`d` is delivered, refused, and the buffer that held it wiped; the guide says to decode a private
JWK as a key pair.

**What document does today** (verified in libs/document at 284ac01, the reason for owner decision
9). The streaming parser builds the document node by node; a selector only chooses which values
are delivered, and `on` removes a delivered value after its callback. A value no selector matches
stays in the document, and the text of every string, key and number is accumulated whole in the
handler's pending buffers (`detail/handler.hpp`: `on_string_part` and `on_key_part` append to
`text_` and `key_`, allocated through the storage's allocator) before the value is stored. So a
10 KB `kid` is buffered in full, in the working state and in the image, and the bound of a working
buffer applies to skipped members too. And document turns an allocator's failure into
`errc::capacity` only with exceptions; without them (`WEBCPP_DOCUMENT_NO_EXCEPTIONS`), a
`null_memory_resource` upstream ends the program (`detail/allocation.hpp`). The first draft
bounded the text with `max_bytes` to stay safe; that reasoning is withdrawn, the bounded-parse mode
removes both problems, and a size cap remains only as `jwk_options::max_bytes`.

### D9. JWK and documents

- **Decode from a value**: `crypto::decode<T>(crypto::jwk, value)`, `value` any
  `document::basic_const_value<Traits, Storage>` (a JWK Set's `keys[i]`, a JWT header's `jwk`
  member), reads the five members with D8's rules and allocates nothing: it asks
  `contains(name)` before `operator[]`, because a handle of a missing member holds its key in the
  document's allocator (`value.hpp`), and reads a present string as a `std::string_view` into the
  image. Duplicates cannot be seen: the document kept the last of a repeated key, as JavaScript
  does, when it was parsed (R13).
- **The rule for a private JWK in a document** (owner decision 8): decoding a key pair from a
  document value is offered, and crypto's part of it, from the `std::string_view` of `d` on, is
  constant time; but `d`'s text already went through document's parser, whose time depends on it
  (UTF-8 validation, escape handling, copies), and through the document's image. So a private key
  whose `d` must stay in constant time from end to end is decoded from its text (D8), never from a
  document; the guide and the function's documentation say so in those words, and a program that
  keeps private JWKs in documents accepts that their parse was not constant time.
- **Encode to a new document**: `crypto::encode<Document>(crypto::jwk, value, arguments...)`,
  `Document` a `document::basic_document<Traits, Storage>`, `arguments` forwarded to
  `Document::create` (an allocator, a maximum, for an owned document) or to the external form (a
  buffer); the members are set in D7's order, so that `stringify()` gives D7's text. It allocates
  in the storage the program picked, and nothing elsewhere.
- **Encode into a value**: `crypto::encode(crypto::jwk, value, target)`, `target` a
  `document::basic_value<Traits, Storage>` (a new element of a JWK Set's `keys`), set to the object.

A private JWK in a document is the private key in that document's image, and setting it copies
`d`'s text through document's code: the same rule applies, and the guide points to document's
`pmr` storages over a resource that wipes on free.

### D10. The thumbprint

`crypto::thumbprint(key)` returns `result<std::array<std::uint8_t, 32>>`, the SHA-256
(`webcpp::hash::sha2_256`) of the public JWK's text (D7) of a public key, or of a key pair's public
key: RFC 7638 with SHA-256 (R8). It needs no document and lives in `jwk.hpp` with the rest of JWK.

### D11. WebCrypto and non-extractable keys

Encode and decode are synchronous: they read bytes the program holds. A browser backend (part B of
the asynchronous family) whose key pair lives in a WebCrypto `CryptoKey` holds no secret bytes, and
a non-extractable key has none to give. `encodes<F, K>` is therefore false for a private format
(`pkcs8`, `pem(pkcs8)`, a key pair's `jwk` and `raw`) on a backend whose
`detail::asynchronous_backend_v` is true, with a message naming WebCrypto's `exportKey`; the public
formats of such a key are computed from the public key the backend keeps. When part B lands, its
own change decides whether an `async_export` exists; nothing here blocks it.

### D12. The JWK target and the core

`<webcpp/crypto/formats.hpp>` (core) declares `jwk_t` and its row in `all_formats`, and each
curve's `formats.hpp` lists `jwk_t` in `formats_of<T>`, so that the concepts are the same whether
`jwk.hpp` is included or not. `<webcpp/crypto/jwk.hpp>` holds the JWK codecs, the reader of D8 and
the thumbprint, includes document, and specializes `detail::codec<jwk_t, T>` for every type whose
curve's `formats.hpp` it sees; a call of a JWK pair without it stops at a `static_assert` naming
the header and the target (D3), never at a link error. The Jamfile gains
`alias jwk : : : : <library>/webcpp/crypto//crypto <library>/webcpp/document//document ;`;
`/webcpp/crypto//crypto` does not name document, and a test builds every core header without
document's include path.

### D13. What is removed and what moves

Removed with no alias: on Ed25519, X25519, P-256 and secp256k1 `encode_spki`, `decode_spki`,
`encode_pkcs8`, `decode_pkcs8`, `max_spki_size`, `max_pkcs8_size`; secp256k1's
`spki_compressed_size` and `spki_uncompressed_size`; on P-256 and secp256k1 `to_der`, `from_der`
and `max_der_size`; the namespace `crypto::pem`, whose name `crypto::pem` the format takes:
`armour`, `unarmour`, `public_key_label`, `private_key_label`, `armoured_size` and `algorithm`
move into `detail` (the family `families::pem` and its backend stay, in `crypto.json` and the
capability tables). Each curve's `formats.hpp` keeps its fixed layouts and holds its codecs. Every
caller in libs/crypto moves in the same change: tests, examples, the page, the constant-time
harnesses and suppressions, the benches, the message tests, the MSVC tests,
`doc/tables/crypto.json` and its generated files (rerun, never edited). secp256k1's private member
template `public_key::encoded<N>` is renamed so that it never shadows `crypto::encoded` inside the
class.

## Rulings

The first draft's R3 (typed signatures) moved to crypto/uniform-types and its R11 (X25519 kept
out) was withdrawn by owner decision 13; the other numbers are kept.

- **R1** (owner decision 11, kept as the first draft ruled): the uncompressed point is the default
  of `raw` and `spki` on every EC curve; `crypto::compressed` names the other form everywhere.
- **R2: a key pair's `raw` is `secret()` and `make` under the format's name**: `encode(raw, kp)`
  calls `kp.secret()` and returns its bytes as `encoded<32, secret_binary>`, `decode<K>(raw, bytes)`
  calls `K::make(bytes)`; one implementation each, so that generic code over formats round-trips
  every type. `secret()` and `make` stay the documented primary forms.
- **R4: decoding is typed by its input**: binary formats take a byte span, text formats a
  `std::string_view`, JWK also a document value; nothing sniffs PEM from DER.
- **R5: secp256k1's `raw(xonly)` decodes** to the even-y key (BIP340's lift_x), so that a Nostr
  key in 32 bytes becomes a typed key in one call; `spki` has no x-only form.
- **R6: the encoder writes only the required JWK members** (D7), never `alg`, `use`, `key_ops`,
  `kid` or `ext`; WebCrypto's `importKey("jwk")` accepts such a JWK. The decoders ignore them, so a
  WebCrypto export with `key_ops` and `ext` decodes; usage restrictions are the program's concern,
  as with SPKI.
- **R7** (owner decision 12): a private JWK given to a public key's decode is `invalid_encoding`.
- **R8: `crypto::thumbprint(key)` is SHA-256 only**, as RFC 7638's examples and every JOSE
  library default; a hash parameter or RFC 9278's URI form can join later without changing a call.
- **R9: `crypto::jwk_working_size` is 4 KiB** for the public reader's buffer, on the stack: with
  unselected members skipped, it holds the selectors, their callbacks, the frames of `max_depth`
  and the five selected strings, each at most 64 bytes; task 4.3 measures the need and the guide
  records it. The private reader needs no buffer.
- **R10: the private reader refuses an escape in `d`** and decodes escapes everywhere else (keys,
  `kty`, `crv`, `x`, `y`), so that both readers agree on every text but one whose `d` is escaped,
  which the public reader refuses anyway; a differential fuzz target checks that they agree.
- **R12: JWK shares the PEM family's base64 backend** (libsodium's constant-time codec), so that
  `d` is never decoded by a variable-time table; a program that uses JWK links libsodium, as one
  that uses PEM does.
- **R13: a document value's decode cannot refuse duplicates** (D9); the guide says so.
- **R14** (the builder's check): the members and codecs live in each `curves/<curve>/formats.hpp`;
  `formats.hpp` includes no curve and no backend; the "builds without X" tests stay and cover the
  formats headers.
- **R15** (the builder's check): the scope is the curves crypto has today, Ed25519, X25519, P-256
  and secp256k1; the roadmap the foundation carried in its format requirements (P-384, P-521, RSA,
  Botan, "every key type" on OpenSSL) is removed from the requirements this change modifies, each
  to join the formats in the change that brings it.
- **R16: P-256's compressed point is decoded by crypto on BearSSL**, a square root on public data
  (D5), its result checked by `make`; Wycheproof's and OpenSSL's compressed points are its oracle.

## Before and after

```cpp
// Before: a P-256 public key to PEM.
std::array<std::uint8_t, p256::max_spki_size> der{};
auto const der_size = p256::encode_spki(public_key, der);
std::array<char, pem::armoured_size(pem::public_key_label.size(), p256::max_spki_size)> text{};
auto const text_size = pem::armour(pem::public_key_label, std::span(der).first(*der_size), text);

// After.
auto const text = crypto::encode(crypto::pem(crypto::spki), public_key);  // text->text()
auto const same = public_key.encode(crypto::pem(crypto::spki));

// Before: a key pair from PKCS#8 PEM.
std::array<std::uint8_t, p256::max_pkcs8_size> der{};
auto const n = pem::unarmour(pem::private_key_label, pem_text, der);
auto const key = p256::decode_pkcs8(std::span(der).first(*n));

// After.
auto const key = crypto::decode<p256::keypair>(crypto::pem(crypto::pkcs8), pem_text);
auto const same = p256::keypair::decode(crypto::pem(crypto::pkcs8), pem_text);

// Before: a signature to DER.
std::array<std::uint8_t, p256::max_der_size> der{};
auto const size = p256::to_der(signature, der);

// After.
auto const der = crypto::encode(crypto::der, *key.sign(crypto::ecdsa_sha256, message));

// Refused at compile time, the diagnostic naming format_list<raw_t<default_form_t>>.
auto const no = crypto::encode(crypto::der, *ed_key.sign(crypto::eddsa, message));

// JWK, new.
auto const jwk_text = crypto::encode(crypto::jwk, key);                  // 176 bytes, secret text
auto const back = crypto::decode<p256::keypair>(crypto::jwk, jwk_text->text());   // d in constant time
auto const pub = crypto::decode<p256::public_key>(crypto::jwk, peer_text);        // any size, bounded memory
auto const in_a_set = crypto::decode<p256::public_key>(crypto::jwk, jwks["keys"][0]);
auto const id = crypto::thumbprint(*key.public_key());

// X25519, new.
auto const x = crypto::encode(crypto::spki, *x25519_key.public_key());

// A caller that manages memory.
std::array<std::uint8_t, crypto::max_encoded_size<crypto::spki_t<>, p256::public_key>> out{};
auto const written = crypto::encode(crypto::spki, public_key, out);
```

## Allocation analysis

| Operation | Heap | Stack and other memory |
|---|---|---|
| `encode` of every format | none | the result, N bytes (at most 241, P-256's `pem(pkcs8)`), and a DER buffer of at most 138 bytes inside `pem(F)`, wiped for `pkcs8` |
| `encode` into a span | none | the DER buffer of `pem(F)` |
| `decode` of `raw`, `spki`, `pkcs8`, `der` | none | today's: a re-encoding buffer to check strictness, wiped for `pkcs8` |
| `decode` of `pem(F)` | none | the DER buffer, at most 138 bytes, wiped for `pkcs8` |
| `decode` of a private JWK text | none | the reader's cursor, its bit stack and fixed buffers, under 256 bytes, wiped; constant whatever the input's size |
| `decode` of a public JWK text | none (whole text given at once; proven by a counting `operator new`) | the working buffer, 4 KiB by default or the caller's span; constant whatever the input's size |
| `decode` of a JWK document value | none (`contains` before `operator[]`) | fixed buffers of 64 bytes, wiped |
| `encode<Document>(jwk, ...)`, `encode(jwk, value, target)` | the document's storage, which the program picked | none |
| `thumbprint` | none | the public JWK text, 130 bytes at most, and the hash's state |

secp256k1's decoders keep their existing behaviour (libsecp256k1's static context, no allocation).
Every public function that calls a backend keeps `detail::timing_guard` first and joins its
backend's coverage test, as the data-independent-timing change requires.

## Trystero 0.27

Trystero 0.27.0 (the pinned checkout the trystero-027 stream's oracle records from,
`packages/core/src/identity.ts`) uses JWK through WebCrypto: it generates its P-256 signing keys
and exports them with `exportKey('jwk')` (`generateP256KeyMaterial`), imports a user identity with
`importKey('jwk', {crv, d, ext, key_ops, kty, x, y})` (`importUserIdentity`), carries public keys
on the wire as the compact text `x.y` of the JWK's two coordinates (`jwkToCompactPub`,
`compactPubToJwk`, `pubJwkToUncompressedBytes`), and stores records with a `signPubJwk`
(`identity-idb.ts`). Its exported user key is `trystero_uk_` and the base64url of the JSON
`{d, x, y}`, a JWK without `kty` and `crv`, and its import also takes a JWK text whose `kty` and
`crv` may be absent. webcpp::trystero reads that today with webcpp::document itself
(`protocol/identity.hpp`, `parse_user_key`). That leniency is Trystero's protocol, not RFC 7517:
`crypto::jwk` stays strict (`kty` and `crv` required), and webcpp::trystero keeps its own reading
of the user key, building the key pair through crypto's typed forms; it can use `crypto::jwk` where
it exchanges a full JWK with WebCrypto, and the typed `raw` and JWK coordinates for the compact
text. Phase 3 of the port adapts after this change.

## Risks / Trade-offs

- **Parser attack surface.** JWK decoding reads network input with two readers (crypto's and
  document's bounded mode), and PEM, DER, SPKI and PKCS#8 decoders read untrusted bytes. Every
  decoder gets a fuzz target from the first commit (`test/fuzz/decode_<format>.cpp`, one per
  format and type family, libFuzzer natively, each asserting no crash, no allocation, the
  round-trip property for what it accepts and the wipe of its buffers), seeded with Wycheproof's
  and the RFCs' vectors and WebCrypto's exports, plus a differential target that feeds the same
  texts to both JWK readers and requires the same verdict (R10); run in CI for a bounded time and
  locally for longer before the merge.
- **A second JSON reader** in crypto is code to keep correct; it is small, strict, fuzzed against
  document, and exists because `d` must not pass through a reader whose time depends on it.
- **secp256k1's SPKI default changes** (R1); a program that stored compressed SPKIs still decodes
  them.
- **P-256's square root on BearSSL** (R16) is new arithmetic, on public data only, checked by the
  point validation that follows it.
- **A deduced-return member without its curve's `formats.hpp`** gives the compiler's message, not
  ours, as the asynchronous members do; the guide says which header to include.
- **Code size**: one codec per pair is a template instantiated only when called; the JWK decoders
  pull document and Boost.JSON's parser, which is why JWK has a target of its own.

## Migration Plan

crypto/sign-unified merges; then crypto/uniform-types and document/bounded-parse, in parallel; then
this change, on branches `crypto/formats` of webcpporg/crypto, webcpporg/proposals and the
superproject; then webcpp::trystero's phase 3 of the 0.27 port. Local verification on every lane,
merged by the controller, one CI run at the end by the coordinator.

## Open Questions

None for this change: the first draft's five were answered on 2026-11-22 (owner decisions 8 to
13).
