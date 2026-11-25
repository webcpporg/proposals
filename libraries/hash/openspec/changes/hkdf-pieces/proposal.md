## Why

webcpp::hash's HKDF takes each input whole: `hkdf_extract(salt, ikm)`, `hkdf_expand(output, prk,
info)` and `hkdf(output, salt, ikm, info)`, one contiguous span per input, with Expand's info
capped at `hkdf_max_info_size`, 32,768 bytes, on every backend, and at 1,024 bytes on
`backends::webcrypto`, where node refuses a longer HKDF info. A caller whose input is a
concatenation must first copy it into one buffer: a heap allocation, or a cap of its own.

crypto's HPKE (`libraries/crypto/openspec/changes/hpke/`, RFC 9180) is such a caller. Its
`LabeledExtract` hashes `"HPKE-v1" || suite_id || label || ikm` and its `LabeledExpand`
`I2OSP(L, 2) || "HPKE-v1" || suite_id || label || info`, and MLS (RFC 9420) puts the serialized
group context in HPKE's info, several kilobytes for a Welcome. HPKE must run with no cap and no
heap. On 2026-11-23 the owner decided that webcpp::hash gains an HKDF that reads its inputs in
pieces, which HPKE then calls, rather than HPKE computing HKDF on HMAC itself; and that hash's
public names are spelled out.

## What Changes

- **`byte_pieces`** in `webcpp::hash`: a sequence of byte spans read one after another as if
  concatenated, written `{a, b, c}` at the call. As built, a view type of hash's (design B2's
  fallback) in its own header, `<webcpp/hash/byte_pieces.hpp>`, which `hkdf.hpp` includes
  (design, "As built").
- **Three overloads that read pieces**:
  `hkdf_extract<Hmac, Backend>(salt, byte_pieces input_key_material)` returning the pseudorandom
  key; `hkdf_expand<Hmac, Backend>(output_key_material, pseudorandom_key, byte_pieces
  application_info)`; `hkdf<Hmac, Backend>(output_key_material, salt, byte_pieces
  input_key_material, byte_pieces application_info)`. Each feeds every piece to the backend's
  streaming HMAC in turn; nothing is concatenated, nothing is capped (no limit on the input key
  material, the application info or the number of pieces), and hash's code allocates nothing.
- **On every backend**: `backends::hash2` (the default), `sodium`, `bearssl`, `openssl` and
  `botan` over their incremental HMAC, wherever the backend has that HMAC (BearSSL's included,
  whose own HKDF has no separate Extract and Expand); `backends::webcrypto` through the
  asynchronous forms, over WebCrypto's HMAC (`subtle.sign("HMAC", ...)`), not WebCrypto's HKDF:
  the pieces are gathered into one JavaScript copy per input, made in the initiating call (which
  WebCrypto copies again at each call, by its specification), with no C++ heap of hash's own and
  no 1,024-byte cap, and Expand signs once per output block.
- **The asynchronous forms**: `async_hkdf` gains the pieces overload, and `async_hkdf_extract`
  and `async_hkdf_expand` join the family, each with its single-buffer and its pieces overload;
  every synchronous HKDF function then has its asynchronous form.
- **Spelled-out public names** (owner rule: a public name is never abbreviated; an algorithm's
  proper name, as `hkdf`, `hmac` or `sha256`, is kept): `ikm` and `ikm_bytes` become
  `input_key_material`, `prk` and `prk_bytes` `pseudorandom_key`, `info` and `info_bytes`
  `application_info`, HKDF's `output` and `output_bytes` `output_key_material`, its
  `salt_bytes` `salt`; `hkdf_max_info_size` becomes `hkdf_max_application_info_size`;
  `scrypt_parameters`' `n`, `r` and `p` become `cost`, `block_size` and `parallelism`;
  `argon2_parameters::memory_kib` becomes `memory_kibibytes`. No public name of hash is `ctx`
  today; the rule keeps it so.
- **The single-buffer forms keep working** under the new parameter names, with their 32,768-byte
  cap on the application info and WebCrypto's 1,024 on `async_hkdf`'s; the cap is theirs alone.
- **Tests**: the pieces forms equal the single-buffer forms on RFC 5869's vectors, on random
  splits, with empty pieces and with many pieces; an application info of several kilobytes; a
  counting `operator new` that proves no allocation; on every backend, `backends::webcrypto` in
  the browser lane, and in the asynchronous forms.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `hash`: HKDF over input pieces, with no cap on every backend and in the asynchronous family,
  and no heap and no copy of the inputs on the synchronous backends; HKDF's and the KDF
  parameters' public names spelled out.

## Impact

- **Kind:** a wrapper (`"port-of": null`), as the genesis.
- **Breaking:** before hash's first release, in names and in HKDF's overloads: the constant
  `hkdf_max_info_size` and the fields of `scrypt_parameters` and `argon2_parameters` are renamed
  (a program that writes `{.n = 32768, .r = 8, .p = 1}` writes `{.cost = 32768, .block_size = 8,
  .parallelism = 1}`); parameter names change no call. As built, the pieces overloads also make
  a lone `{}` and a braced list of one element that is not exactly a
  `std::span<std::uint8_t const>` (`{vec}`, `{array}`) ambiguous where both overloads take braces:
  `hkdf(out, salt, {}, {})`, `async_hkdf(ex, out, salt, {}, {}, token)` and `{vec}` inputs no
  longer compile, and the caller passes the vector itself, an empty span or `byte_pieces{...}`
  (design, "As built", R9 and R11).
- **Order:** before crypto's `hpke`, which calls these overloads; independent of every other open
  change.
- **Implementation:** webcpporg/hash (`include/webcpp/hash/hkdf.hpp`, `byte_pieces.hpp`,
  `config.hpp`, `kdf.hpp`, `async.hpp`, `scrypt.hpp`, `argon2.hpp`, the backends' headers,
  `test/`, `doc/`, `doc/capabilities.json` and its generated files), the superproject's pointer
  and `tools/node-test/node-pre.js`.
- **Status:** written on 2026-11-23 from the owner's decisions and approved by the owner on
  2026-11-23 (`decided`); `implementing` since 2026-11-25: built on the branches
  `hash/hkdf-pieces` of webcpporg/hash and the superproject, verified locally on hash's lanes
  (`tasks.md`, 4.3); the other libraries' Emscripten lanes, the pull requests, their merges and
  the CI on GitHub remain.
