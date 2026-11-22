## Context

This change builds on crypto/sign-unified (not merged when this was written): typed public keys
(`ed25519::public_key`, `p256::basic_public_key<B>`, `secp256k1::public_key`), the concept
`crypto::keypair`, scheme tags as values (`crypto::ecdsa_sha256`, ...), one list of schemes
(`detail::all_schemes`), the backend first among template parameters, and four mirrored forms per
operation (member and free, synchronous and asynchronous; its D9).

The formats as they are on that branch:

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

webcpp::document (`libs/document`) is a JavaScript-compatible JSON document: `parse`, a
streaming parser with selectors (`basic_parser::on`, `observe`), storages that own a growing
image (`basic_owned<Allocator>`) or use a buffer of the caller's (`basic_external<Allocator>`),
and an allocator for the working state of every operation.

## Intent

A call site should read as what it does, and a mistake should not compile:

```cpp
auto const pem_text = crypto::encode(crypto::pem(crypto::spki), public_key);   // the label is right by construction
auto const key = crypto::decode<p256::keypair>(crypto::jwk, text);             // a JWK into a key pair
auto const der = crypto::encode(crypto::der, signature);                       // only an ECDSA signature compiles
```

Three properties motivate the change: readability (one verb per direction, the format named at the
call as the scheme is), ease (no buffer to size, no label to spell, no per-curve name to find), and
compile-time errors (a format the type does not have is a diagnostic that lists the ones it has,
not a missing name).

## Goals / Non-Goals

**Goals:** one `encode` and one `decode` for every key and signature format of Ed25519, P-256 and
secp256k1, in four mirrored forms with one implementation; format tags as values, PEM composing
over the DER formats with its label taken from them; a fixed result with no heap, and a span form;
a concept that gates every pair with the type's list of formats in the diagnostic; one list of
formats from which the concepts, the tests and the guide derive; JWK in its own header and target,
encoding with no heap and decoding text in bounded working memory; RFC 7638's thumbprint; every
caller in libs/crypto moved; the old names removed.

**Non-goals:** X.509 and certificates (another session's change); asynchronous forms of encode and
decode (they are pure functions over bytes the program holds); compressed PEM; encrypted PKCS#8
(PKCS#5, `ENCRYPTED PRIVATE KEY`); X25519's typing (an open owner question, R11 and O1); JWK's `RSA`,
`oct` and P-384/P-521 keys (crypto has no such keys); JWS, JWE and JWT themselves; a public
base64url function.

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
members, a fixed result, one list, concepts with the list in the diagnostic, JWK in its own target.

## Owner decisions (2026-11-22, binding)

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
   signatures refuse `der`. X25519: ruled in R11, with the owner's open question on aligning it.
5. **Old names are removed with no aliases**: `encode_spki`, `decode_spki`, `encode_pkcs8`,
   `decode_pkcs8`, `to_der`, `from_der`, `pem::armour` and `pem::unarmour` as public API, and the
   `max_*_size` constants the new form supersedes; every caller migrates.
6. **JWK is in this change** (RFC 7517, 7518, 8037, 8812; EC P-256 and secp256k1 with `x`, `y` and
   optionally `d`; OKP Ed25519 and X25519 with `x` and optionally `d`; base64url without padding),
   in its own header `<webcpp/crypto/jwk.hpp>` and target `/webcpp/crypto//jwk`, which pulls
   webcpp::document; the core of crypto does not depend on document. Encoding to text: a fixed
   buffer, no heap, canonical member order (lexicographic, no white space), so that the text equals
   RFC 7638's thumbprint input; maximum sizes computed. Encoding to a document in the storage the
   user picks. Decoding text: unbounded input, bounded working memory, through document's
   streaming parser with selectors pulling only `kty`, `crv`, `x`, `y` and `d` into fixed buffers,
   its working state in a `std::pmr::monotonic_buffer_resource` over a fixed buffer with
   `null_memory_resource` upstream, exhaustion an error (`jwk_too_large`); duplicate members among
   the fields read refused; unknown members ignored; `kty`, `crv` and size mismatches refused; a
   public JWK given to a key pair's decode refused. Decoding an existing document value with no new
   allocation. A thumbprint (RFC 7638) with webcpp::hash's SHA-256.
7. **Out of scope**: X.509 and certificates, asynchronous forms of encode and decode, compressed
   PEM, encrypted PKCS#8.

## Decisions

### D1. The header, the tags and the one list

`<webcpp/crypto/formats.hpp>` (namespace `webcpp::crypto`) declares the format tags, each an empty
type with an explicit default constructor and a `constexpr` object, as `signing.hpp` declares the
schemes:

- `raw_t<Form>` and `raw`, `spki_t<Form>` and `spki`, where `Form` is a point form tag:
  `default_form_t` (the format's own form, below), `compressed_t`, `uncompressed_t`, `xonly_t`,
  with objects `crypto::compressed`, `crypto::uncompressed`, `crypto::xonly`;
  `raw(crypto::compressed)` and `spki(crypto::compressed)` are `constexpr` calls that return
  `raw_t<compressed_t>{}` and `spki_t<compressed_t>{}`.
- `pkcs8_t` and `pkcs8`, `der_t` and `der`, `jwk_t` and `jwk`.
- `pem_t<Inner>` and `crypto::pem`, an object whose `constexpr operator()(Inner)` returns
  `pem_t<Inner>{}` for an `Inner` that is `spki_t<Form>` or `pkcs8_t` and stops the build for any
  other with "PEM carries a DER key: pem(crypto::spki) or pem(crypto::pkcs8)". `pem_t<Inner>`
  holds its label as a `static constexpr std::string_view label`, `"PUBLIC KEY"` for every
  `spki_t<Form>` and `"PRIVATE KEY"` for `pkcs8_t`; no other way to name a label exists.

The one list, beside the tags: `detail::format_list<...>` and

```
using all_formats = format_list<raw_t<default_form_t>, spki_t<default_form_t>, pkcs8_t, der_t,
                                jwk_t, pem_t<spki_t<default_form_t>>, pem_t<pkcs8_t>>;
```

The point forms are modifiers of a listed format, not separate entries; `detail::base_format_t<F>`
maps `raw_t<compressed_t>` to `raw_t<default_form_t>` and so on. `detail::is_format_v` derives from
the list; a new format is one line in the list and one specialization per type that has it.

Each value type declares the formats it has in one place, beside its class, as unified-sign's
curves declare their schemes: `detail::formats_of<T>` is a `format_list`, every member of which
must be in `all_formats` (a `static_assert` beside the declaration); for a point form,
`detail::forms_of<Format, T>` lists the forms the type accepts.

### D2. The four forms and the result

| | Encode | Decode |
|---|---|---|
| Free (main) | `crypto::encode(format, value)` returns `result<encoded<N, Form>>` | `crypto::decode<T>(format, input)` returns `result<T>` |
| Free, caller's memory | `crypto::encode(format, value, output)` returns `result<std::size_t>` | (the free form already writes nothing to the heap) |
| Member | `value.encode(format)` | `T::decode(format, input)` |

One implementation: `detail::codec<Format, T>` holds `static encode_into(value, span)` and
`static decode(input)`; the free functions call it, the members call the free functions. The
members are declared in each class as member templates with a deduced return type and defined in
`formats.hpp`, as unified-sign's D9 defines the asynchronous members in `async.hpp`: a curve's
header does not include `formats.hpp`, and calling a member without it is the compiler's "deduced
return type used before it is defined".

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
on it; a `T` that has none (`ed25519::public_key`, `secp256k1::public_key`) decodes on its family's
default, as `make` does. PEM and JWK's base64 run on `families::pem`'s backend (libsodium's
constant-time codec), the same as today, which a program using either links.

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
64 bytes are the signature"), `der` on a recoverable signature ("a recoverable signature has no DER form"), `jwk` without
`<webcpp/crypto/jwk.hpp>` ("include <webcpp/crypto/jwk.hpp> and link /webcpp/crypto//jwk"),
`pem(der)` and `pem(jwk)` (D1). A test of message compilation (`webcpp.compile-diagnostic`) pins
each.

### D4. The valid pairs

| Type | `raw` | `spki` | `pem(spki)` | `pkcs8` | `pem(pkcs8)` | `der` | `jwk` |
|---|---|---|---|---|---|---|---|
| `ed25519::public_key` | 32 | 44 | 113 | - | - | - | 79 (OKP) |
| `p256::basic_public_key<B>` | 65 (uncompressed) | 91 | 178 | - | - | - | 126 (EC) |
| `secp256k1::public_key` | 65; `compressed` 33; `xonly` 32 | 88; `compressed` 56 | 174; `compressed` 130 | - | - | - | 130 (EC) |
| `ed25519::basic_keypair<B>` | 32, secret (R2) | - | - | 48 | 119 | - | 129 (OKP, `d`) |
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
all; a PKCS#8 of 138 bytes is 184 characters in 3 lines, 28 before and 26 after, 241 in all. The
JWK sizes are D7's.

Decoding takes the same pairs. Each decoder keeps today's strictness (foundation D30): it accepts
exactly what its encoder writes, with two existing exceptions kept: secp256k1's `spki` decodes
either point form, as today, and its `raw` decodes 33 or 65 bytes, as `public_key::make` does;
`raw(xonly)` decodes 32 bytes as BIP340's lift of x, the key of even y (R5).

### D5. The point forms

The default form of every EC format is the uncompressed point: `raw` of a P-256 key is
WebCrypto's `raw` (65 bytes), `spki` writes what OpenSSL's `i2d_PUBKEY`, node and WebCrypto write.
`crypto::compressed` is offered where the type has it (secp256k1 today; P-256 only on backends
that have the compressed form, OpenSSL and Botan, and refused at compile time on BearSSL as the
foundation's `basic_public_key` is), `crypto::xonly` on secp256k1's `raw` only. The public key's
accessors `bytes()`, `uncompressed()`, `compressed()` and `xonly()` stay: they are the value's own
bytes, which `raw` calls, not a second format API.

### D6. Typed signatures and DER

DER needs the curve (P-256's `to_der` refuses r or s not below n; secp256k1's parses through
libsecp256k1), and the owner's pairs need Ed25519 and Schnorr signatures to refuse it; a bare
`std::array<std::uint8_t, 64>` says neither. So each `sign` returns a type of its own:
`p256::ecdsa_signature`, `secp256k1::ecdsa_signature`, `secp256k1::recoverable_signature` (65),
`secp256k1::schnorr_signature`, `ed25519::signature`. Each is a regular value holding its bytes,
made by `make(bytes)` (the size checked, nothing else: range is verification's judgment, as
unified-sign's R16 rules for Ed25519's key), with `bytes()` returning the array, and an implicit
conversion to `std::span<std::uint8_t const, N>`, so that every `crypto::verify` and `recover` of
unified-sign, which takes that span, takes the typed signature unchanged, and a signature from the
wire is still verified from its bytes. `normalize_low_s` and `is_low_s` take the curve's
`ecdsa_signature` (`normalize_low_s` returns one). The asynchronous family's completion signatures
carry the typed signature. `encode(der, signature)` writes strict DER (8 to 72 bytes);
`decode<p256::ecdsa_signature>(der, bytes)` reads strict DER only, the same code as today's
`to_der` and `from_der` behind the new names.

### D7. JWK: the members and the text

| Key | Members, in the order written | Public | Private |
|---|---|---|---|
| Ed25519 | `crv` `"Ed25519"`, [`d`], `kty` `"OKP"`, `x` | 79 | 129 |
| X25519 (when typed, R11) | `crv` `"X25519"`, [`d`], `kty` `"OKP"`, `x` | 78 | 128 |
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

### D8. JWK: decoding a text in bounded memory

`crypto::decode<T>(crypto::jwk, text)` and `crypto::decode<T>(crypto::jwk, text, working)`, the
second with a caller's `std::span<std::byte> working` (16-aligned), the first with a buffer of
`crypto::jwk_working_size` bytes (16 KiB by default, R9) on the stack. The working buffer is split
in two regions: the document's image, through document's external storage (`pmr::external`: the
image never grows, a full image is `errc::capacity`), and the parser's working state, through a
`std::pmr::monotonic_buffer_resource` over the other region with `std::pmr::null_memory_resource()`
upstream. The parser is `document::basic_parser<default_traits, pmr::external>`, given the whole
text by `finish_with`, so that Boost.JSON's parser never suspends and nothing is allocated in the
global heap (document's documentation of `finish_with`); a counting `operator new` test proves zero
allocations. `parse_options::max_depth` is 16 (a JWK's members are at depth 1, the deepest known
member, `x5c` or `key_ops`, at 2) and `max_bytes` is the bound of R10.

One selector, `$.*`, registered with `on`, receives every member of the root as it completes and
takes it out of the document afterwards (its blocks zeroed and freed in the image); the callback
copies the value of `kty`, `crv`, `x`, `y` and `d`, when it is a string, into fixed buffers (`kty`
and `crv` 16 bytes, `x`, `y` and `d` 64 bytes: a longer value is already wrong and is refused),
counts each of the five, and ignores every other member. Then:

- a root that is not an object, a text that is not one JSON value, or any of the five present
  twice: `invalid_encoding`;
- `kty` missing, not a string, or not the type's (`"EC"` for P-256 and secp256k1, `"OKP"` for
  Ed25519); `crv` missing or not the type's; `x` (and `y` for EC) missing; any of them not a
  string: `invalid_encoding`;
- `x`, `y` or `d` not strict base64url without padding of exactly 32 bytes (43 characters, the
  unused bits zero, which libsodium's decoder checks): `invalid_encoding`;
- a key pair's decode without `d`: `invalid_encoding` (a public JWK is not a key pair);
- a public key's decode with `d`: `invalid_encoding` (R7);
- a key pair whose `x` and `y` are not `d`'s public key: `invalid_encoding`, as PKCS#8's decode
  refuses a public key that is not the scalar's;
- the point off the curve, a scalar 0 or not below n: `invalid_encoding`, as `spki` and `pkcs8`
  map `make`'s refusals;
- the working buffer exhausted or `max_bytes` exceeded (`errc::capacity` from document):
  `jwk_too_large`, a new code of `crypto::error`.

Unknown members, `alg`, `use`, `key_ops`, `ext` and `kid` included, are ignored, whatever their
value, duplicates included. Every byte that held `d`'s text or its value (the fixed buffers, both
regions of the working buffer) is wiped before the call returns, on every path.

**Verified in document's code (this change's analysis, libs/document at 284ac01).** The streaming
parser builds the document node by node; a selector only chooses which values are delivered, and
`on` removes a delivered value after its callback. A value no selector matches stays in the
document, and the text of every string, key and number is accumulated whole in the handler's
pending buffers (`detail/handler.hpp`: `on_string_part` and `on_key_part` append to `text_` and
`key_`, allocated through the storage's allocator) before the value is stored. So a 10 KB `kid`
is buffered in full, in the working state (its text, in a monotonic resource where each growth of
the buffer leaves the old one behind, about twice its size in all) and in the image (its stored
string) until its member completes; the selector `$.*` frees the image's copy right after, but
the working state's high-water mark stays. The bound therefore applies to skipped members too:
the largest member's key and value, not only the five fields read, must fit in the working buffer,
and a JWK with a large unknown member (a 10 KB `kid`, an `x5c` chain) is `jwk_too_large` under the
default buffer. A program that reads such JWKs passes a larger `working` span, or decodes from a
document it parsed itself (D9). An option in document's parser that skips the text of unselected
values without buffering would restore unbounded input in bounded memory; it is a change to
document, recorded as open question O3.

**And without exceptions.** document turns an allocator's failure into `errc::capacity` only with
exceptions; in a build without them (`WEBCPP_DOCUMENT_NO_EXCEPTIONS`), `null_memory_resource`'s
failure ends the program (`detail/allocation.hpp`). crypto supports builds without exceptions, so
the working state must never be exhausted: R10 bounds the text with `max_bytes`, which document
checks before it parses, so that every text it accepts fits.

### D9. JWK and documents

- **Decode from a value**: `crypto::decode<T>(crypto::jwk, value)`, `value` any
  `document::basic_const_value<Traits, Storage>` (a JWK Set's `keys[i]`, a JWT header's `jwk`
  member), reads the five members with the same rules and allocates nothing: it asks
  `contains(name)` before `operator[]`, because a handle of a missing member holds its key in the
  document's allocator (`value.hpp`), and reads a present string as a `std::string_view` into the
  image. Duplicates cannot be seen: the document kept the last of a repeated key, as JavaScript
  does, when it was parsed; a program that must refuse duplicates decodes the text (D8).
- **Encode to a new document**: `crypto::encode<Document>(crypto::jwk, value, arguments...)`,
  `Document` a `document::basic_document<Traits, Storage>`, `arguments` forwarded to
  `Document::create` (an allocator, a maximum, for an owned document) or to the external form (a
  buffer); the members are set in D7's order, so that `stringify()` gives D7's text. It allocates
  in the storage the program picked, and nothing elsewhere.
- **Encode into a value**: `crypto::encode(crypto::jwk, value, target)`, `target` a
  `document::basic_value<Traits, Storage>` (a new element of a JWK Set's `keys`), set to the object.

A private JWK in a document is the private key in that document's image: the guide says so, and
points to document's `pmr` storages over a resource that wipes on free.

### D10. The thumbprint

`crypto::thumbprint(key)` returns `result<std::array<std::uint8_t, 32>>`, the SHA-256
(`webcpp::hash::sha2_256`) of the public JWK's text (D7) of a public key, or of a key pair's public
key: RFC 7638 with SHA-256, its only widely used hash, so that no hash parameter is needed now
(R8). It needs no document and lives in `jwk.hpp` with the rest of JWK.

### D11. WebCrypto and non-extractable keys

Encode and decode are synchronous: they read bytes the program holds. A browser backend (part B of
the asynchronous family) whose key pair lives in a WebCrypto `CryptoKey` holds no secret bytes, and
a non-extractable key has none to give. `encodes<F, K>` is therefore false for a private format
(`pkcs8`, `pem(pkcs8)`, a key pair's `jwk` and `raw`) on a backend whose
`detail::asynchronous_backend_v` is true, with a message naming WebCrypto's `exportKey`; the public
formats of such a key are computed from the public key the backend keeps. When part B lands, its
own change decides whether an `async_export` exists; nothing here blocks it.

### D12. The JWK target and the core

`<webcpp/crypto/formats.hpp>` (core) declares `jwk_t`, its row in `all_formats` and the pairs in
`formats_of<T>`, so that the concepts are the same whether `jwk.hpp` is included or not.
`<webcpp/crypto/jwk.hpp>` holds the implementation, includes document, and specializes
`detail::codec<jwk_t, T>`; a call of a JWK pair without it stops at a `static_assert` naming the
header and the target (D3), never at a link error. The Jamfile gains
`alias jwk : : : : <library>/webcpp/crypto//crypto <library>/webcpp/document//document ;`;
`/webcpp/crypto//crypto` does not name document, and a test builds every core header without
document's include path.

### D13. What is removed and what moves

Removed with no alias: on Ed25519, P-256 and secp256k1 `encode_spki`, `decode_spki`,
`encode_pkcs8`, `decode_pkcs8`, `max_spki_size`, `max_pkcs8_size`; secp256k1's
`spki_compressed_size` and `spki_uncompressed_size`; on P-256 and secp256k1 `to_der`, `from_der`
and `max_der_size`; the namespace `crypto::pem`, whose name `crypto::pem` the format takes:
`armour`, `unarmour`, `public_key_label`, `private_key_label`, `armoured_size` and `algorithm`
move into `detail` (the family `families::pem` and its backend stay, in `crypto.json` and the
capability tables). Each curve's `formats.hpp` keeps its fixed layouts and holds its
`detail::codec` specializations. Every caller in libs/crypto moves in the same change: tests,
examples, the page, the constant-time harnesses and suppressions, the benches, the message tests,
the MSVC tests, `doc/tables/crypto.json` and its generated files (rerun, never edited). secp256k1's
private member template `public_key::encoded<N>` is renamed so that it never shadows
`crypto::encoded` inside the class.

## Rulings

- **R1: `raw`'s default is the uncompressed point on every EC curve and `spki`'s too**, with
  `crypto::compressed` as a modifier. This changes secp256k1's SPKI default (compressed today) to
  what OpenSSL, node and WebCrypto write; one rule across curves outweighs Bitcoin's habit, which
  `spki(compressed)` keeps one word away.
- **R2: a key pair's `raw` is `secret()` and `make` under the format's name**: `encode(raw, kp)`
  calls `kp.secret()` and returns its bytes as `encoded<32, secret_binary>`, `decode<K>(raw, bytes)`
  calls `K::make(bytes)`; one implementation each, no second path, so that generic code over
  formats round-trips every type. `secret()` and `make` stay the documented primary forms.
- **R3: typed signatures** (D6). The owner's pairs (DER for ECDSA, refused for Ed25519 and
  Schnorr) cannot be checked on a bare array, and DER needs the curve; the types convert to the
  span `crypto::verify` takes, so unified-sign's verification is unchanged.
- **R4: decoding is typed by its input**: binary formats take a byte span, text formats a
  `std::string_view`, JWK also a document value; nothing sniffs PEM from DER.
- **R5: secp256k1's `raw(xonly)` decodes** to the even-y key (BIP340's lift_x), so that a Nostr
  key in 32 bytes becomes a typed key in one call; `spki` has no x-only form.
- **R6: the encoder writes only the required JWK members** (D7), never `alg`, `use`, `key_ops`,
  `kid` or `ext`; WebCrypto's `importKey("jwk")` accepts such a JWK. The decoder ignores them, so a
  WebCrypto export with `key_ops` and `ext` decodes; usage restrictions are the program's concern,
  as with SPKI.
- **R7: a private JWK given to a public key's decode is refused** (`invalid_encoding`), not read
  for its public part: the program that expected a public key and got `d` has leaked a secret
  somewhere, and silently accepting it would let it be stored or logged as public. The program that
  wants the public part decodes the key pair and asks it for its public key.
- **R8: `crypto::thumbprint(key)` is SHA-256 only**, as RFC 7638's examples and every JOSE
  library default; a hash parameter or RFC 9278's URI form can join later without changing a call.
- **R9: the default working buffer is 16 KiB** (`crypto::jwk_working_size`), on the stack, half
  for the image and half for the working state: WebCrypto's exports are under 300 bytes, and
  16 KiB stays within the default stacks of Emscripten and wasi-sdk (64 KiB) with room to spare. The
  largest member it admits is measured in task 4.3 and documented; programs with larger JWKs pass
  their own span.
- **R10: the text is bounded by `parse_options::max_bytes`**, set from the working buffer's size
  (a quarter of it by default, 4 KiB, to be confirmed by task 4.3's measurement), so that no text
  document accepts can exhaust the working state, which a build without exceptions could not
  survive (D8). This makes the owner's "unbounded input" a bounded one in fact; O3 records the
  document change that would lift it.
- **R11: X25519 stays out of this change** (owner decision 4 leaves it open). Its keys are untyped
  (`std::array<std::uint8_t, 32>`, `secret<32>`), and typing them is the X25519 alignment the owner
  has an open question on. Its four format functions keep their names until it joins, the one
  exception to decision 5; when its typed public key and key pair exist, they gain the same rows as
  Ed25519 (`raw`, `spki`, `pem(spki)`, `jwk` OKP 78; `pkcs8`, `pem(pkcs8)`, `jwk` 128) and the old
  names go. D7 already fixes X25519's JWK text.
- **R12: JWK shares the PEM family's base64 backend** (libsodium's constant-time codec), so that
  `d` is never decoded by a variable-time table; a program that uses JWK links libsodium, as one
  that uses PEM does.
- **R13: a document value's decode cannot refuse duplicates** (D9); the guide says so.

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
auto const back = crypto::decode<p256::keypair>(crypto::jwk, jwk_text->text());
auto const in_a_set = crypto::decode<p256::public_key>(crypto::jwk, jwks["keys"][0]);
auto const id = crypto::thumbprint(*key.public_key());

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
| `decode` of a JWK text | none (proven by a counting `operator new`; `finish_with` never suspends Boost.JSON) | the working buffer, 16 KiB by default or the caller's span, wiped on return |
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

- **Parser attack surface.** JWK decoding brings a JSON parser (document over Boost.JSON) to an
  input that comes from the network, and PEM, DER, SPKI and PKCS#8 decoders read untrusted bytes.
  Every decoder gets a fuzz target from the first commit (`test/fuzz/decode_<format>.cpp`, one per
  format and type family, libFuzzer natively, each asserting no crash, no allocation, the
  round-trip property for what it accepts and the wipe of the working buffer), seeded with
  Wycheproof's and the RFCs' vectors and WebCrypto's exports, run in CI for a bounded time and
  locally for longer before the merge.
- **The skipped-member bound** (D8) makes large JWKs `jwk_too_large` by default; documented, with
  the span form and decoding from a document as the ways out, and O3 as the fix.
- **Typed signatures** widen the change into signing's return types; the implicit conversion keeps
  every verification call as unified-sign wrote it, and the migration is mechanical.
- **secp256k1's SPKI default changes** (R1); a program that stored compressed SPKIs still decodes
  them.
- **A deduced-return member without `formats.hpp`** gives the compiler's message, not ours, as
  the asynchronous members do; the guide says which header to include.
- **Code size**: one codec per pair is a template instantiated only when called; the JWK decoder
  pulls document and Boost.JSON's parser, which is why it has a target of its own.

## Migration Plan

Implemented after crypto/sign-unified merges, on branches `crypto/formats` of webcpporg/crypto,
webcpporg/proposals and the superproject; then webcpp::trystero's phase 3 of the 0.27 port adapts.
Local verification on every lane, merged by the controller, one CI run at the end by the
coordinator.

## Open Questions

- **O1: X25519.** Should X25519 gain its typed public key and key pair in this change (trivial for
  the public key, the key pair being the alignment question), or stay out with its four old
  functions kept until the alignment change (R11, chosen here)?
- **O2: typed signatures** (R3) change what `sign` returns on every curve. Accept them here, or
  move them to a small change of their own merged first?
- **O3: unbounded JWK input.** document buffers the text of unselected values (D8), so the input
  is bounded in fact (R10). Ask document for a parser option that skips unselected values without
  buffering, and lift R10 then?
- **O4: secp256k1's SPKI default** (R1): uncompressed on every curve, or keep compressed on
  secp256k1?
- **O5: a public JWK given to a key pair is refused; a private JWK given to a public key** is
  refused too (R7). Confirm, or read the public part?
