## ADDED Requirements

### Requirement: HKDF over pieces: the overloads
`<webcpp/hash/hkdf.hpp>` SHALL declare `byte_pieces`, a sequence of byte spans written `{a, b, c}` at the call, and the overloads `hkdf_extract(salt, byte_pieces input_key_material)`, `hkdf_expand(output_key_material, pseudorandom_key, byte_pieces application_info)` and `hkdf(output_key_material, salt, byte_pieces input_key_material, byte_pieces application_info)`, each giving the bytes its single-buffer form gives for the concatenation of the pieces.

#### Scenario: HPKE's labeled extract
- **WHEN** a program calls `hkdf_extract<hmac_sha2_256>(salt, {prefix, suite_id, label, secret})`
- **THEN** the pseudorandom key equals `hkdf_extract<hmac_sha2_256>` of the four spans concatenated

#### Scenario: Empty and many pieces
- **WHEN** RFC 5869's vectors run with each input split into one piece per byte, with empty pieces between them, and as an empty list where the input is empty
- **THEN** every output equals the vector's

### Requirement: HKDF over pieces: no cap and no allocation
The pieces forms SHALL accept an input keying material, an application info and a number of pieces of any length a span holds, on every backend, refusing only RFC 5869's output size and pseudorandom key size; they MUST feed each piece to the backend's streaming HMAC, copy no piece, and allocate nothing in hash's code. Every HMAC state, block and intermediate pseudorandom key MUST be wiped.

#### Scenario: A Welcome-sized info
- **WHEN** `hkdf_expand<hmac_sha2_256>` runs with a 1 MiB application info in four pieces under a counting `operator new` on `backends::hash2`, `sodium` and `bearssl`
- **THEN** it succeeds, no allocation is counted, and the output equals an RFC 5869 computation over the concatenated bytes

### Requirement: HKDF over pieces: on every backend that has the HMAC
The pieces forms SHALL exist on `backends::hash2` (the default), `sodium`, `bearssl`, `openssl` and `botan`, over each backend's incremental HMAC, wherever the backend has that HMAC; `backend_has_v` of the tags `kdfs::hkdf_pieces`, `kdfs::hkdf_extract_pieces` and `kdfs::hkdf_expand_pieces` MUST equal the backend's HMAC cell, and `doc/capabilities.json` MUST hold the three rows.

#### Scenario: BearSSL's Extract over pieces
- **WHEN** a program calls `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, {a, b})`
- **THEN** it compiles and gives `backends::hash2`'s result, while `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, ikm)` stays refused

### Requirement: HKDF over pieces: the asynchronous forms
`async.hpp` SHALL declare `async_hkdf_extract` and `async_hkdf_expand`, each with a single-buffer and a pieces overload, and a pieces overload of `async_hkdf`, over the hmacs `async_hkdf` takes, completing with `void(boost::system::error_code)`. They MUST read the list and the pieces before the initiating call returns, on every backend.

#### Scenario: A temporary list
- **WHEN** a program calls `async_hkdf<hmac_sha2_256>(executor, output, salt, {a, b}, {c, d}, token)` and overwrites `a` to `d` before running the executor
- **THEN** the completion reports success and `output` holds the HKDF of the original bytes

### Requirement: HKDF over pieces: WebCrypto over its HMAC
On `backends::webcrypto` the asynchronous pieces forms SHALL compute HKDF over `subtle.sign("HMAC", ...)`, not WebCrypto's HKDF: the pieces gathered into the one JavaScript copy of each call, nothing on the C++ heap, no 1,024-byte limit, an empty salt imported as the digest size of zeros, and Expand signing once per output block.

#### Scenario: A long info in the browser
- **WHEN** `async_hkdf<hmac_sha2_256, backends::webcrypto>` runs with a 64 KiB application info in pieces
- **THEN** it completes with the bytes `backends::hash2` gives, where the single-buffer form refuses it with `input_too_long`

### Requirement: HKDF over pieces: the single-buffer forms unchanged
The single-buffer `hkdf_extract`, `hkdf_expand`, `hkdf` and `async_hkdf` SHALL keep their behaviour, backends and errors, with their application info capped at `hkdf_max_application_info_size`, 32,768 bytes, and at 1,024 on `backends::webcrypto`; the cap MUST NOT apply to the pieces forms.

#### Scenario: The cap is the single-buffer form's
- **WHEN** `hkdf_expand<hmac_sha2_256>` is given a 32,769-byte application info as one span and then as `{info}`
- **THEN** the first returns `input_too_long` and the second succeeds

### Requirement: Names: public names spelled out
Public names of webcpp::hash SHALL NOT be abbreviations, an algorithm's proper name excepted: HKDF's parameters MUST be `salt`, `input_key_material`, `pseudorandom_key`, `application_info` and `output_key_material`; the constant MUST be `hkdf_max_application_info_size`; `scrypt_parameters` MUST hold `cost`, `block_size`, `parallelism`, and `argon2_parameters` `memory_kibibytes`.

#### Scenario: scrypt's costs
- **WHEN** a program writes `scrypt(output, password, salt, {.cost = 32768, .block_size = 8, .parallelism = 1})`
- **THEN** it compiles and gives RFC 7914's result for N = 32768, r = 8, p = 1
