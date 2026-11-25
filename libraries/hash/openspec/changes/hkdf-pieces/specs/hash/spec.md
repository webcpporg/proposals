## ADDED Requirements

### Requirement: HKDF over pieces: the overloads
`<webcpp/hash/hkdf.hpp>` SHALL provide `byte_pieces` (declared in `<webcpp/hash/byte_pieces.hpp>`, which it includes), byte spans written `{a, b, c}` at the call, and the overloads `hkdf_extract(salt, byte_pieces input_key_material)`, `hkdf_expand(output_key_material, pseudorandom_key, byte_pieces application_info)` and `hkdf(output_key_material, salt, byte_pieces input_key_material, byte_pieces application_info)`, each giving its single-buffer form's bytes for the pieces' concatenation.

#### Scenario: HPKE's labeled extract
- **WHEN** a program calls `hkdf_extract<hmac_sha2_256>(salt, {prefix, suite_id, label, secret})`
- **THEN** the pseudorandom key equals `hkdf_extract<hmac_sha2_256>` of the four spans concatenated

#### Scenario: Empty and many pieces
- **WHEN** RFC 5869's vectors run with each input split into one piece per byte, with empty pieces between them, and as an empty list and as one empty piece where the input is empty
- **THEN** every output equals the vector's

### Requirement: HKDF over pieces: braced lists at the call
A braced list of one `std::span<std::uint8_t const>`, `{x}`, SHALL name the single-buffer overload wherever one is viable (the language's rule), and `byte_pieces{x}` the form over pieces with one piece. Where both overloads take braces, `{}` and a braced list of one element of another type (`{vec}`, `{array}`) are ambiguous and MUST NOT compile: a caller passes the element itself, an empty span, `byte_pieces{}` or `byte_pieces{x}`.

#### Scenario: Which braced lists compile
- **WHEN** a program calls `hkdf<hmac_sha2_256>(output, salt, {}, {})`, or `hkdf<hmac_sha2_256>(output, salt, {vec}, {vec})` for a `std::vector<std::uint8_t> vec`
- **THEN** neither compiles, while `hkdf<hmac_sha2_256>(output, salt, byte_pieces{}, byte_pieces{})`, `hkdf<hmac_sha2_256>(output, salt, vec, vec)` and, for a `std::span<std::uint8_t const> bytes`, `hkdf<hmac_sha2_256>(output, salt, byte_pieces{bytes}, byte_pieces{bytes})` do

### Requirement: HKDF over pieces: no cap
The pieces forms SHALL accept an input keying material, an application info and a number of pieces of any length a span holds, on every backend, refusing only RFC 5869's output size and pseudorandom key size and the backend's HMAC key limit on the salt.

#### Scenario: A long info on every synchronous backend
- **WHEN** `hkdf_expand<hmac_sha2_256>` runs with a 1 MiB application info in pieces on `backends::hash2`, `sodium`, `bearssl`, `openssl` and `botan`
- **THEN** each succeeds and gives an RFC 5869 computation over the concatenated bytes

### Requirement: HKDF over pieces: no copy and no allocation on the synchronous backends
On the synchronous backends (`backends::hash2`, `sodium`, `bearssl`, `openssl` and `botan`) the pieces forms MUST feed each piece to the backend's streaming HMAC, in slices its length type holds, copy no piece, and allocate nothing in hash's code. On `backends::webcrypto` the pieces are gathered into JavaScript memory, as the WebCrypto requirement states.

#### Scenario: A Welcome-sized info
- **WHEN** `hkdf_expand<hmac_sha2_256>` runs with a 1 MiB application info in four pieces under a counting `operator new` on `backends::hash2`, `sodium` and `bearssl`
- **THEN** it succeeds, no allocation is counted, and the output equals an RFC 5869 computation over the concatenated bytes

### Requirement: HKDF over pieces: the secrets wiped
Every HMAC state, block and intermediate pseudorandom key that hash's code holds in a pieces form MUST be wiped, on success and on failure; WebCrypto's imported keys (`CryptoKey`), which WebCrypto gives no way to zero, are excepted.

#### Scenario: The wipes of a call
- **WHEN** a pieces form runs on a synchronous backend in a test built with hash's wipe record
- **THEN** every HMAC state, block and pseudorandom key the engine held is recorded as wiped, and no wiped byte is left non-zero

### Requirement: HKDF over pieces: on every backend that has the HMAC
The pieces forms SHALL exist on `backends::hash2` (the default), `sodium`, `bearssl`, `openssl` and `botan`, over each backend's incremental HMAC, wherever the backend has that HMAC; `backend_has_v` of the tags `kdfs::hkdf_pieces`, `kdfs::hkdf_extract_pieces` and `kdfs::hkdf_expand_pieces` MUST equal the backend's HMAC cell, and `doc/capabilities.json` MUST hold the three rows.

#### Scenario: BearSSL's Extract over pieces
- **WHEN** a program calls `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, {a, b})`
- **THEN** it compiles and gives `backends::hash2`'s result, while `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, ikm)` stays refused

### Requirement: HKDF over pieces: the asynchronous forms
`async.hpp` SHALL declare `async_hkdf_extract` and `async_hkdf_expand`, each with a single-buffer and a pieces overload, and a pieces overload of `async_hkdf`, over the hmacs `async_hkdf` takes, completing with `void(boost::system::error_code)`. They MUST read the list and the pieces before the initiating call returns, on every backend: a synchronous backend computes in the initiating call and completes with the code alone, and `backends::webcrypto` gathers the pieces in it.

#### Scenario: A temporary list
- **WHEN** a program calls `async_hkdf<hmac_sha2_256>(executor, output, salt, {a, b}, {c, d}, token)` and overwrites `a` to `d` before running the executor
- **THEN** the completion reports success and `output` holds the HKDF of the original bytes

### Requirement: HKDF over pieces: WebCrypto over its HMAC
On `backends::webcrypto` the asynchronous pieces forms SHALL compute HKDF over `subtle.sign("HMAC", ...)`, not WebCrypto's HKDF: each input's pieces gathered in the initiating call into one JavaScript buffer of its own (which WebCrypto copies again at each call, by its specification), nothing on the C++ heap of hash's own, no 1,024-byte limit, an empty salt imported as the digest size of zeros, and Expand signing once per output block.

#### Scenario: A long info in the browser
- **WHEN** `async_hkdf<hmac_sha2_256, backends::webcrypto>` runs with a 64 KiB application info in pieces
- **THEN** it completes with the bytes `backends::hash2` gives, where the single-buffer form refuses it with `input_too_long`

### Requirement: HKDF over pieces: WebCrypto's buffers
On `backends::webcrypto` a gathered total past 2^53 - 1 bytes, or a buffer the JavaScript runtime refuses, MUST complete with `backend_failure` and a zeroed output, and every JavaScript buffer that held a secret MUST be zeroed once its last reader settles.

#### Scenario: A gather the runtime refuses
- **WHEN** an asynchronous pieces form on `backends::webcrypto` meets a gathered total past its bound, or a runtime that refuses the buffer
- **THEN** it completes with `backend_failure` and the output is all zeros

### Requirement: HKDF over pieces: the single-buffer forms unchanged
The single-buffer `hkdf_extract`, `hkdf_expand`, `hkdf` and `async_hkdf` SHALL keep their behaviour, backends and errors, with their application info capped at `hkdf_max_application_info_size`, 32,768 bytes, and at 1,024 on `backends::webcrypto`; the cap MUST NOT apply to the pieces forms.

#### Scenario: The cap is the single-buffer form's
- **WHEN** `hkdf_expand<hmac_sha2_256>` is given a 32,769-byte application info as one span, then as `{info}`, and then as `byte_pieces{info}`
- **THEN** the first two take the single-buffer form and return `input_too_long`, and the third succeeds

### Requirement: Names: public names spelled out
Public names of webcpp::hash SHALL NOT be abbreviations, an algorithm's proper name excepted: HKDF's parameters MUST be `salt`, `input_key_material`, `pseudorandom_key`, `application_info` and `output_key_material`; the constant MUST be `hkdf_max_application_info_size`; `scrypt_parameters` MUST hold `cost`, `block_size`, `parallelism`, and `argon2_parameters` `memory_kibibytes`.

#### Scenario: scrypt's costs
- **WHEN** a program writes `scrypt(output, password, salt, {.cost = 32768, .block_size = 8, .parallelism = 1})`
- **THEN** it compiles and gives RFC 7914's result for N = 32768, r = 8, p = 1
