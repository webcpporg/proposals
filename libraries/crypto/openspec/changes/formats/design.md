## Context

This change builds on two others, merged before it:

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
the bytes its encoder writes (foundation D30).

## Intent

A call site should read as what it does, and a mistake should not compile:

```cpp
auto const pem_text = crypto::encode(crypto::pem(crypto::spki), public_key);   // the label is right by construction
auto const key = crypto::decode<p256::keypair>(crypto::pem(crypto::pkcs8), text);
auto const der = crypto::encode(crypto::der, signature);                       // only an ECDSA signature compiles
```

Three properties motivate the change: readability (one verb per direction, the format named at the
call as the scheme is), ease (no buffer to size, no label to spell, no per-curve name to find), and
compile-time errors (a format the type does not have is a diagnostic that lists the ones it has,
not a missing name).

## Goals / Non-Goals

**Goals:** one `encode` and one `decode` for every key and signature format of Ed25519, X25519,
P-256 and secp256k1 (`raw`, `spki`, `pkcs8`, `der`, `pem(F)`), in four mirrored forms with one
implementation; format tags as values, PEM composing over the DER formats with its label taken from
them; a fixed result with no heap, and a span form; a concept that gates every pair with the type's
list of formats in the diagnostic; one list of formats from which the concepts, the tests and the
guide derive, built so that JWK joins later as one more tag with no change to any interface; every
caller in libs/crypto moved; the old names removed.

**Non-goals:** JWK (deferred by the owner, see "Deferred: JWK"); X.509 and certificates (another
session's change); asynchronous forms of encode and decode (pure functions over bytes the program
holds); compressed PEM; encrypted PKCS#8 (PKCS#5, `ENCRYPTED PRIVATE KEY`); curves and key types
crypto does not have today (P-384, P-521, RSA, and the keys only OpenSSL or Botan have): each joins
the formats with its own change; a public base64 or base64url function.

**Anti-goals:** a format chosen at run time (an enum or a string) on the main path, which would
lose the compile-time size and the compile-time refusal; any function that infers a curve or a
format from a number of bytes; a PEM label a caller spells.

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

**G (chosen).** Tags as values passed first, the free function as the main form with mirrored
members, a fixed result, one list, concepts with the list in the diagnostic.

## Owner decisions (2026-11-22, binding)

First round:

1. **Format tags are values passed as the first argument**, as the schemes are: `crypto::raw`,
   `crypto::spki`, `crypto::pkcs8`, `crypto::der` (an ECDSA signature), and `crypto::pem(F)`,
   which composes over `spki` or `pkcs8` and takes its label (`PUBLIC KEY`, `PRIVATE KEY`) from the
   inner format, so that the label can never be wrong. One list of formats, as `all_schemes`, from
   which the concepts and the documentation derive. (`crypto::jwk` was in this list; decision 14
   defers it.)
2. **Free functions are the main form**, `crypto::encode(format, value)` and
   `crypto::decode<T>(format, input)`, with mirrored members `value.encode(format)` and
   `T::decode(format, input)`, one implementation.
3. **Return values, option (b)**: a ready value with no heap, `result<encoded<N>>`, a fixed buffer
   whose N is the format's maximum for that type at compile time, with the actual length and
   `bytes()` / `text()` views; plus an overload writing into a caller's `std::span`.
4. **A concept gates every pair** (`encodes<Format, T>`, `decodes<Format, T>`), with a message
   listing the formats valid for that type. Public keys: `raw`, `spki`, `pem(spki)`; key pairs:
   `pkcs8`, `pem(pkcs8)`; a key pair's `raw` is its `secret()` (not duplicated; ruled in R2); ECDSA
   signatures (P-256, secp256k1): `der`; Ed25519 and Schnorr signatures refuse `der`.
5. **Old names are removed with no aliases**: `encode_spki`, `decode_spki`, `encode_pkcs8`,
   `decode_pkcs8`, `to_der`, `from_der`, `pem::armour` and `pem::unarmour` as public API, and the
   `max_*_size` constants the new form supersedes; every caller migrates.
6. *(JWK in this change: superseded by decision 14.)*
7. **Out of scope**: X.509 and certificates, asynchronous forms of encode and decode, compressed
   PEM, encrypted PKCS#8.

Second round, answering the first draft's open questions:

8. *(A constant-time reader for a private JWK's text: deferred with JWK by decision 14; recorded
   in "Deferred: JWK".)*
9. *(A bounded-memory parse mode in document: withdrawn by decision 14; the deferred JWK's bounds
   come from JSON Schema instead.)*
10. **Typed signatures are a separate small change merged first** (crypto/uniform-types); this
    change builds on them.
11. **The uncompressed point is the default of `spki` and `raw` on every curve**, the compressed
    point always available by name; secp256k1's SPKI default changes.
12. *(A private JWK given to a public key's decode is refused: deferred with JWK, kept as its
    rule.)*
13. **X25519 is typed by uniform-types, so this change covers it**; its old functions are removed
    as the others are.

Change of plan, the same day:

14. **JWK leaves this change**: it ships `raw`, `spki`, `pkcs8`, `der` and `pem(F)` only, the list
    designed so that `jwk` joins later as one more tag with no interface change. JWK comes after
    webcpp::json_schema gains document support, so that a JWK's shape and limits are a JSON Schema
    that document applies while parsing (the chain is in "Deferred: JWK"). The document
    bounded-parse change is not written.

Third round, the same day:

15. **The deferred JWK keeps crypto's own constant-time reader for a private JWK's `d`**: the
    JSON Schema that json_schema on document applies handles a JWK's shape and limits only
    (members, constants, sizes, unknown members), never `d`'s secret bytes, which crypto's small
    reader reads in constant time as "Deferred: JWK" describes.

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
- `pkcs8_t` and `pkcs8`, `der_t` and `der`.
- `pem_t<Inner>` and `crypto::pem`, an object whose `constexpr operator()(Inner)` returns
  `pem_t<Inner>{}` for an `Inner` that is `spki_t<Form>` or `pkcs8_t` and stops the build for any
  other with "PEM carries a DER key: pem(crypto::spki) or pem(crypto::pkcs8)". `pem_t<Inner>`
  holds its label as a `static constexpr std::string_view label`, `"PUBLIC KEY"` for every
  `spki_t<Form>` and `"PRIVATE KEY"` for `pkcs8_t`; no other way to name a label exists.

The one list, beside the tags: `detail::format_list<...>` and `detail::all_formats`, a
`format_list` of `raw_t<default_form_t>`, `spki_t<default_form_t>`, `pkcs8_t`, `der_t`,
`pem_t<spki_t<default_form_t>>` and `pem_t<pkcs8_t>`. The point forms are modifiers of a listed
format, not entries; `detail::base_format_t<F>` maps `raw_t<compressed_t>` to
`raw_t<default_form_t>` and so on. `detail::is_format_v` derives from the list; a new format is one
line in the list and one codec per type that has it.

Each value type declares the formats it has in one place, beside its codecs, as the curves declare
their schemes: `detail::formats_of<T>` is a `format_list`, every member of which must be in
`all_formats` (a `static_assert` beside the declaration); `detail::forms_of<Format, T>` lists the
point forms the type accepts.

**Room for JWK (decision 14).** Nothing in the interface assumes a closed set of formats or of input
kinds: a format's input kind is a trait of the format (`detail::input_of<Format>`: a byte span for
the binary formats, `std::string_view` for the text ones), `encoded`'s form is a trait of the pair,
and the concepts fold over the list. JWK joins as a tag `jwk_t` / `crypto::jwk`, one line in
`all_formats`, `jwk_t` in the `formats_of` of the types that have it, its codecs in a header of its
own, and, if it decodes from a document value, one more input kind for that tag; no existing call,
signature or concept changes. A test adds a stand-in text format to a copy of the list and checks
that the concepts, `encoded` and the free functions take it with no change, so that the room stays
real.

### D2. The four forms, the result, and where they live

| | Encode | Decode |
|---|---|---|
| Free (main) | `crypto::encode(format, value)` returns `result<encoded<N, Form>>` | `crypto::decode<T>(format, input)` returns `result<T>` |
| Free, caller's memory | `crypto::encode(format, value, output)` returns `result<std::size_t>` | (the free form already writes nothing to the heap) |
| Member | `value.encode(format)` | `T::decode(format, input)` |

One implementation: `detail::codec<Format, T>` holds `static encode_into(value, span)` and
`static decode(input)`; the free functions call it, the members call the free functions.

Where each part lives (the builder's check, R14): `formats.hpp` holds the tags, the list, the
concepts, `encoded` and the free functions, generic over `detail::codec`, and includes no curve and
no backend. Each curve's `curves/<curve>/formats.hpp` includes its curve and `formats.hpp`,
specializes `detail::codec` and `formats_of` for its types, and defines its types' members, which
each class declares as member templates with a deduced return type (as unified-sign's D9 declares
the asynchronous members): a curve's own header never includes a formats header, and calling a
member without its curve's `formats.hpp` is the compiler's "deduced return type used before it is
defined". So a program that uses P-256's formats includes `curves/p256/formats.hpp` and pulls no
other backend, and the existing tests that build each family without the others' backends ("builds
without X") stay and gain the formats headers.

`input` is `std::span<std::uint8_t const>` for a binary format (`raw`, `spki`, `pkcs8`, `der`)
and `std::string_view` for a text format (`pem(F)`); each decode takes exactly its kind (the other
does not convert), so that PEM text is never read as DER.

`crypto::encoded<N, Form>` is a class template with `Form` one of `encoded_form::binary`, `text`,
`secret_binary`, `secret_text`:

- it holds `std::array<std::uint8_t, N>` (or `char` for text) and its length;
- `bytes()` returns `std::span<std::uint8_t const>` of the length, on every form;
- `text()` returns `std::string_view`, on the text forms only (a requires clause);
- `size()`, `max_size()` (N, `static constexpr`), and nothing that converts it implicitly;
- the secret forms wipe their storage on destruction and on move as `secret<N>` does (the detail
  wipe), are move-only, and `clone()` copies explicitly, as `secret<N>` does.

The form follows from the pair: `pkcs8`, `pem(pkcs8)` and a key pair's `raw` are secret; everything
else is not. `crypto::max_encoded_size<Format, T>` is N; it replaces every `max_*_size` constant.
The span overload takes `std::span<std::uint8_t>` for a binary format and `std::span<char>` for a
text format, refuses one shorter than `max_encoded_size` with `invalid_input_size`, and writes
nothing on any error, as today's encoders.

Backends: a `T` that names its backend (`p256::basic_public_key<B>`, `basic_keypair<B>`) decodes
on it; a `T` that has none (`ed25519::public_key`, `x25519::public_key`, `secp256k1::public_key`)
decodes on its family's default, as `from_bytes` does. PEM's base64 runs on `families::pem`'s backend
(libsodium's constant-time codec), as today, which a program using PEM links.

### D3. The concepts and the diagnostic

`crypto::encodes<Format, T>` is true when `base_format_t<Format>` is in `formats_of<T>`, the form
is in `forms_of<Format, T>`, and the backend can export it (D7). `crypto::decodes<Format, T>` is
the same for reading. Each free function and member checks its concept in a `static_assert`
before anything else, its message fixed text ("this type has no such format; the formats it has
are the format_list named in this assertion") and its condition
`detail::format_refused<Format, T, detail::formats_of_t<T>>`, whose template arguments every
compiler prints in the failed requirement, so that the list of formats the type has appears in the
diagnostic without a string written by hand. Pairs with a reason of their own add a sentence:
`der` on Ed25519's or BIP340's signature ("Ed25519 and BIP340 signatures have no DER form; their
64 bytes are the signature"), `der` on a recoverable signature ("a recoverable signature has no
DER form"), a type whose curve's `formats.hpp` is not included ("include
<webcpp/crypto/curves/<curve>/formats.hpp>"), and `pem(der)` (D1). A test of message compilation
(`webcpp.compile-diagnostic`) pins each.

### D4. The valid pairs

| Type | `raw` | `spki` | `pem(spki)` | `pkcs8` | `pem(pkcs8)` | `der` |
|---|---|---|---|---|---|---|
| `ed25519::public_key` | 32 | 44 | 113 | - | - | - |
| `x25519::public_key` | 32 | 44 | 113 | - | - | - |
| `p256::basic_public_key<B>` | 65; `compressed` 33 | 91; `compressed` 59 | 178; `compressed` 134 | - | - | - |
| `secp256k1::public_key` | 65; `compressed` 33; `xonly` 32 | 88; `compressed` 56 | 174; `compressed` 130 | - | - | - |
| `ed25519::basic_keypair<B>` | 32, secret (R2) | - | - | 48 | 119 | - |
| `x25519::basic_keypair<B>` | 32, secret (R2) | - | - | 48 | 119 | - |
| `p256::basic_keypair<B>` | 32, secret (R2) | - | - | 138 | 241 | - |
| `secp256k1::basic_keypair<B>` | 32, secret (R2) | - | - | 135 | 237 | - |
| `p256::ecdsa_signature` | 64 | - | - | - | - | 72 (8 to 72) |
| `secp256k1::ecdsa_signature` | 64 | - | - | - | - | 72 (8 to 72) |
| `secp256k1::recoverable_signature` | 65 | - | - | - | - | - |
| `secp256k1::schnorr_signature` | 64 | - | - | - | - | - |
| `ed25519::signature` | 64 | - | - | - | - | - |

Each number is `max_encoded_size`, the bytes or characters `encode` writes; every pair marked `-`
stops the build. The sizes of PEM are `armoured_size(label, der)` of today's `pem.hpp`: for P-256,
an SPKI of 91 bytes is 124 base64 characters in 2 lines, 27 characters before and 25 after, 178 in
all; a PKCS#8 of 138 bytes is 184 characters in 3 lines, 28 before and 26 after, 241 in all.
P-256's compressed SPKI is the same AlgorithmIdentifier around a BIT STRING of 34 bytes, 59 in all.

Decoding takes the same pairs. Each decoder keeps today's strictness (foundation D30): it accepts
exactly what its encoder writes. A modifier names the one form encoded; a decode with the default
form accepts the point forms the type's `from_bytes` accepts (secp256k1's `spki` and `raw` take 33 or 65
bytes, as today, and so do P-256's now), and `raw(xonly)` decodes 32 bytes as BIP340's lift of x,
the key of even y (R5).

### D5. The point forms (owner decision 11)

The default form of every EC format is the uncompressed point: `raw` of a P-256 key is
WebCrypto's `raw` (65 bytes), `spki` writes what OpenSSL's `i2d_PUBKEY`, node and WebCrypto write.
`crypto::compressed` is offered on every EC curve and every backend: encoding writes the parity
byte and x, which needs no backend; decoding a compressed P-256 point on BearSSL, which has no
point decompression, computes y with a fixed-width square root modulo p
(p = 3 mod 4, y = (x^3 - 3x + b)^((p + 1) / 4)) on public data in crypto, then checks the point as
`from_bytes` does (R16); OpenSSL decompresses itself. `crypto::xonly` is secp256k1's `raw` only. The
public key's accessors `bytes()`, `uncompressed()`, `compressed()` and `xonly()` stay: they are
the value's own bytes, which `raw` calls, not a second format API; P-256's `basic_public_key` gains
`compressed()` on every backend.

### D6. DER on the typed signatures

The signature types come from uniform-types. DER needs the curve (P-256's encoder refuses r or s
not below n; secp256k1's parses through libsecp256k1), which the type gives, and Ed25519's,
BIP340's and the recoverable signature refuse `der` by type. `encode(der, signature)` writes strict
DER (8 to 72 bytes); `decode<p256::ecdsa_signature>(der, bytes)` reads strict DER only, the same
code as today's `to_der` and `from_der` behind the new names.

### D7. WebCrypto and non-extractable keys

Encode and decode are synchronous: they read bytes the program holds. A browser backend (part B of
the asynchronous family) whose key pair lives in a WebCrypto `CryptoKey` holds no secret bytes, and
a non-extractable key has none to give. `encodes<F, K>` is therefore false for a private format
(`pkcs8`, `pem(pkcs8)`, a key pair's `raw`) on a backend whose `detail::asynchronous_backend_v` is
true, with a message naming WebCrypto's `exportKey`; the public formats of such a key are computed
from the public key the backend keeps. When part B lands, its own change decides whether an
`async_export` exists; nothing here blocks it.

### D8. What is removed and what moves

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

The numbers of the first drafts are kept; R3 (typed signatures) moved to crypto/uniform-types, R11
(X25519 kept out) was withdrawn by owner decision 13, and the JWK rulings (R6 to R10, R12, R13)
moved to "Deferred: JWK" with decision 14.

- **R1** (owner decision 11): the uncompressed point is the default of `raw` and `spki` on every EC
  curve; `crypto::compressed` names the other form everywhere.
- **R2: a key pair's `raw` is `secret()` and `from_secret` under the format's name**: `encode(raw, kp)`
  calls `kp.secret()` and returns its bytes as `encoded<32, secret_binary>`, `decode<K>(raw, bytes)`
  calls `K::from_secret(bytes)`; one implementation each, so that generic code over formats round-trips
  every type. `secret()` and `from_secret` stay the documented primary forms.
- **R4: decoding is typed by its input**: binary formats take a byte span, text formats a
  `std::string_view`; nothing sniffs PEM from DER.
- **R5: secp256k1's `raw(xonly)` decodes** to the even-y key (BIP340's lift_x), so that a Nostr
  key in 32 bytes becomes a typed key in one call; `spki` has no x-only form.
- **R14** (the builder's check): the members and codecs live in each `curves/<curve>/formats.hpp`;
  `formats.hpp` includes no curve and no backend; the "builds without X" tests stay and cover the
  formats headers.
- **R15** (the builder's check): the scope is the curves crypto has today, Ed25519, X25519, P-256
  and secp256k1; the roadmap the foundation carried in its format requirements (P-384, P-521, RSA,
  Botan, "every key type" on OpenSSL) is removed from the requirements this change modifies, each
  to join the formats in the change that brings it.
- **R16: P-256's compressed point is decoded by crypto on BearSSL**, a square root on public data
  (D5), its result checked by `from_bytes`; Wycheproof's and OpenSSL's compressed points are its oracle.
- **R17: the list leaves room for JWK** (D1), proven by a test with a stand-in text format.

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

// X25519 and the point forms, new.
auto const x = crypto::encode(crypto::spki, *x25519_key.public_key());
auto const small = crypto::encode(crypto::raw(crypto::compressed), p256_public_key);

// A caller that manages memory.
std::array<std::uint8_t, crypto::max_encoded_size<crypto::spki_t<>, p256::public_key>> out{};
auto const written = crypto::encode(crypto::spki, public_key, out);
```

## Allocation analysis

| Operation | Heap | Stack |
|---|---|---|
| `encode` of every format | none | the result, N bytes (at most 241, P-256's `pem(pkcs8)`), and a DER buffer of at most 138 bytes inside `pem(F)`, wiped for `pkcs8` |
| `encode` into a span | none | the DER buffer of `pem(F)` |
| `decode` of `raw`, `spki`, `pkcs8`, `der` | none | today's: a re-encoding buffer to check strictness, wiped for `pkcs8` |
| `decode` of `pem(F)` | none | the DER buffer, at most 138 bytes, wiped for `pkcs8` |

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
(`protocol/identity.hpp`, `parse_user_key`), building the key from the coordinates. That leniency
is Trystero's protocol, not RFC 7517, so phase 3 of the port does not wait for crypto's JWK: it
keeps its own reading and builds the typed keys through this change's `raw` forms
(`p256::keypair::from_secret(d)`, `p256::public_key::from_bytes` of `0x04 || x || y`); when crypto's JWK lands,
webcpp::trystero can use it where it exchanges a full JWK with WebCrypto.

## Deferred: JWK

The owner deferred JWK on 2026-11-22 (decision 14). This section records what the first drafts
designed, so that the later change starts from it, and the chain it waits for.

**The chain.** JWK comes last of three steps, none of them written here:

1. **json_schema's genesis** (`libraries/json_schema/openspec/changes/genesis/`, a draft: "a JSON
   Schema validator for every dialect"): drafts 4 to 2020-12 and OpenAPI 3.0, an IR compiled into a
   flat plan run by a small VM, errors as values, allocation only through the program's
   allocators, user vocabularies; today its parser dependency is Boost.JSON.
2. **json_schema on document**, the step document's genesis names as its open question O1 ("moving
   xstate, trystero and json_schema from Boost.JSON to document: each is its own proposal"): the
   validator reads webcpp::document values, and document's streaming parser applies a compiled
   schema while it parses, so that what the schema forbids (a member not allowed, a string longer
   than its `maxLength`, a value of the wrong type or constant) is refused as it streams, before it
   is buffered, and what the schema does not need is skipped. This schema-driven, bounded parsing
   replaces the bounded-parse mode the second draft asked of document.
3. **crypto's JWK** (owner decision 15 divides the work: the schema checks shape and limits,
   crypto's own small reader reads a private key's `d` in constant time): each key type's JWK is a
   JSON Schema (required `kty`, `crv`, `x`, `y` and,
   for a private key, `d`; `kty` and `crv` as `const`; `x`, `y` and `d` with `minLength` and
   `maxLength` 43 and a base64url `pattern` or a custom `format`; `additionalProperties` true, or
   bounded by `maxProperties` and a `maxLength` on unknown members; possibly a small vocabulary or
   meta keywords of crypto's own for "base64url of N bytes" and "secret"), compiled once, which
   document applies while parsing a JWK. JWK joins this change's list as `crypto::jwk` with no
   interface change (D1).

**What the first drafts designed, kept for that change** (the details are in this change's history
on branch `crypto/formats`):

- RFC 7517, 7518, 8037 and 8812: EC P-256 and secp256k1 (`x`, `y`, `d`), OKP Ed25519 and X25519
  (`x`, `d`), base64url without padding of 32 bytes, 43 characters.
- Its own header `<webcpp/crypto/jwk.hpp>` and target `/webcpp/crypto//jwk`, the only part of
  crypto that depends on document.
- Encoding to text into the fixed result with no heap: members in lexicographic order, no white
  space, nothing but the required members (no `alg`, `use`, `key_ops`, `kid`, `ext`), so that the
  public text is RFC 7638's thumbprint input. Sizes: Ed25519 79 public, 129 private; X25519 78 and
  128; P-256 126 and 176; secp256k1 130 and 180. Encoding to a document or into a document value,
  in the storage the program picks.
- Decoding: duplicates of the five members refused, unknown members ignored, `kty`, `crv` and sizes
  checked, a public JWK refused as a key pair, a private JWK refused as a public key (owner decision
  12), `x` and `y` required to be `d`'s public key, exhaustion an error value (`jwk_too_large`),
  never termination, with or without exceptions.
- A private JWK's `d` read in constant time (owner decision 8): after `"d":"`, 43 bytes copied
  without a branch on them, the next byte required to be `"`, decoded by the constant-time
  base64url, an escape in `d` refused; traced by the constant-time lane as PKCS#8 is; every byte
  that held `d` wiped. Owner decision 15 settles who does it: crypto's own small reader.
- Decoding from an existing document value (a JWK Set's element, a JWT header's member) with no
  allocation (`contains` before `operator[]`, since a handle of a missing member holds its key);
  the rule that a private JWK read out of a program's document had its `d` through document's
  parser, not constant time.
- `crypto::thumbprint(key)`: RFC 7638 with SHA-256, on webcpp::hash.
- JWK's base64url on the PEM family's constant-time backend.
- What document does today, verified in libs/document at 284ac01: its streaming parser builds the
  whole document (selectors only choose deliveries), buffers the text of every string and key whole
  before storing it (`detail/handler.hpp`, `on_string_part`, `on_key_part`), so a 10 KB unknown
  member costs its full size; and without exceptions an allocator's failure ends the program
  (`detail/allocation.hpp`). Step 2 of the chain is where both are answered.

## Risks / Trade-offs

- **Parser attack surface.** The PEM, DER, SPKI, PKCS#8 and raw decoders read untrusted bytes.
  Every decoder gets a fuzz target from the first commit (`test/fuzz/decode_<format>.cpp`, one per
  format and type family, libFuzzer natively, each asserting no crash, no allocation, the
  round-trip property for what it accepts and the wipe of its buffers), seeded with Wycheproof's
  and the RFCs' vectors and WebCrypto's exports; run in CI for a bounded time and locally for
  longer before the merge.
- **secp256k1's SPKI default changes** (R1); a program that stored compressed SPKIs still decodes
  them.
- **P-256's square root on BearSSL** (R16) is new arithmetic, on public data only, checked by the
  point validation that follows it.
- **A deduced-return member without its curve's `formats.hpp`** gives the compiler's message, not
  ours, as the asynchronous members do; the guide says which header to include.
- **JWK waits for two other changes**; programs that need JWK now (webcpp::trystero) read their own
  shapes with document and build typed keys through `raw`.

## Migration Plan

crypto/sign-unified merges; then crypto/uniform-types; then this change, on branches
`crypto/formats` of webcpporg/crypto, webcpporg/proposals and the superproject; then
webcpp::trystero's phase 3 of the 0.27 port. JWK follows the chain of "Deferred: JWK". Local
verification on every lane, merged by the controller, one CI run at the end by the coordinator.

## Open Questions

None: the owner's decisions 1 to 15 settle the change and the deferred JWK's division of work.
