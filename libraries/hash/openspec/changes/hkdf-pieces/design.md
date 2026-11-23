## Context

webcpp::hash's HKDF today (`include/webcpp/hash/hkdf.hpp` on `main`):

- `hkdf_extract<Hmac, Backend>(salt_bytes, ikm_bytes)` returns `result<Hmac::result_type>`;
  `hkdf_expand<Hmac, Backend>(output_bytes, prk_bytes, info_bytes)` and
  `hkdf<Hmac, Backend>(output_bytes, salt_bytes, ikm_bytes, info_bytes)` return `result<void>`;
  each input one `std::span<std::uint8_t const>`.
- `hkdf_max_info_size`, 32,768 bytes (OpenSSL 3.5's limit), caps Expand's info on every backend;
  `detail::backend_limits<Backend>` adds each backend's own (`backends::webcrypto`'s `hkdf_info`
  is 1,024 bytes: node refuses a longer HKDF info).
- The engines, `detail::hkdf_engine<Backend, Hmac>`: `backends::hash2` computes RFC 5869 on
  Boost.Hash2's `hmac<H>`; `sodium` calls `crypto_kdf_hkdf_sha256_*` and `_sha512_*`; `bearssl`
  `br_hkdf_*`, which keeps the pseudorandom key inside its context, so BearSSL has `hkdf` but no
  `hkdf_extract` or `hkdf_expand`; `openssl` `EVP_KDF` "HKDF"; `botan` Botan's "HKDF".
- `async.hpp` has `async_hkdf` alone, over the hmacs WebCrypto has (SHA-1, SHA-256, SHA-384,
  SHA-512); on `backends::webcrypto` it imports the input keying material and calls
  `deriveBits({name: "HKDF", hash, salt, info}, key, bits)`; on a synchronous backend it runs
  `hkdf` in the initiating call and completes through `asio::post`.
- Every function holds `detail::timing_guard` first (change data-independent-timing), and hash's
  own code allocates nothing (`doc/allocation.adoc`, test `allocation`); OpenSSL's and Botan's own
  allocations are theirs, counted and documented.

crypto's `hpke` change needs RFC 9180's labeled HKDF: Extract over `"HPKE-v1" || suite_id ||
label || ikm`, Expand over `I2OSP(L, 2) || "HPKE-v1" || suite_id || label || info`, where MLS's
info is kilobytes, with no cap and no heap. Its first draft computed HKDF on HMAC inside crypto
(its ruling R1); the owner decided instead that webcpp::hash offers the pieces.

## Goals / Non-Goals

**Goals:** HKDF whose input keying material and application info are read in pieces, with no
cap and no allocation by hash's code, on every backend and in the asynchronous family, giving the
same bytes as the single-buffer forms; spelled-out public names across hash.

**Non-goals:** pieces for the salt or the pseudorandom key (each is an HMAC key, hashed whole by
HMAC's definition when longer than a block, and HPKE's are short and contiguous); pieces for
PBKDF2, the standard KDFs (KBKDF, SSKDF, X9.63) or `mac` (a later change if a consumer needs
them; `mac` streams through `hasher` already for digests); lifting the single-buffer forms' caps
(they keep calling their backends' own HKDF); a type other than byte spans for pieces.

**Anti-goals:** a copy of the pieces into a buffer of hash's; an allocation by hash's code; a
pieces form that is missing on a backend that has the HMAC.

## Owner decisions (2026-11-23, binding)

1. **Overloads over pieces**, nothing concatenated: `byte_pieces` =
   `std::span<std::span<std::uint8_t const> const>` (the builder may refine the spelling;
   callers write `{a, b, c}`). `hkdf_extract(salt, byte_pieces input_key_material)` returns the
   pseudorandom key; `hkdf_expand(output_key_material, pseudorandom_key, byte_pieces
   application_info)`; `hkdf(output_key_material, salt, byte_pieces input_key_material,
   byte_pieces application_info)`; and the `async_` forms.
2. **No cap and no heap**: the pieces forms cap neither the application info nor the input
   keying material, and never allocate on the C++ heap; each piece is fed to the streaming HMAC.
3. **Every backend**: Hash2 (the default), OpenSSL, BearSSL, libsodium and Botan over their
   incremental HMAC. WebCrypto over WebCrypto's HMAC (`subtle.sign("HMAC", ...)`), not its HKDF:
   the pieces are gathered into the JavaScript copy every WebCrypto call already makes (no extra
   copy, no C++ heap, no 1,024-byte node cap); Expand makes one HMAC call per output block.
4. **Spelled-out names**: a public name is never abbreviated (an algorithm's proper name, as
   `hkdf`, `hmac` or `sha256`, excepted): `ikm`/`ikm_bytes` -> `input_key_material`,
   `prk`/`prk_bytes` -> `pseudorandom_key`, `info`/`info_bytes` -> `application_info`, HKDF's
   output -> `output_key_material`, `ctx` -> `context`, and every other abbreviated public
   parameter name of hash's public headers. The single-buffer forms keep working under the new
   names; their 32,768-byte cap stays, for them only.
5. **Tests**: the pieces forms equal the single-buffer forms (RFC 5869's vectors, random splits,
   empty pieces, many pieces), an application info of several kilobytes, a counting
   `operator new` that proves no heap, on every backend including WebCrypto (the browser lane)
   and in the asynchronous forms.
6. **Order**: this change lands before crypto's `hpke`, which uses it.

## Decisions

### D1. `byte_pieces` and the three overloads

In `hkdf.hpp`, namespace `webcpp::hash`:

```cpp
/** Byte sequences read one after another, as if concatenated: `{a, b, c}` at the call. */
using byte_pieces = std::span<std::span<std::uint8_t const> const>;

template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_extract_pieces>>
[[nodiscard]] result<typename detail::hmac_result<Hmac>::type> hkdf_extract(
    std::span<std::uint8_t const> salt, byte_pieces input_key_material);

template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_expand_pieces>>
[[nodiscard]] result<void> hkdf_expand(std::span<std::uint8_t> output_key_material,
                                       std::span<std::uint8_t const> pseudorandom_key,
                                       byte_pieces application_info);

template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_pieces>>
[[nodiscard]] result<void> hkdf(std::span<std::uint8_t> output_key_material,
                                std::span<std::uint8_t const> salt,
                                byte_pieces input_key_material, byte_pieces application_info);
```

A call `hkdf_extract<hmac_sha2_256>(salt, {prefix, label, secret})` binds the braced list to a
temporary array of spans that lives until the end of the full expression, which outlasts the
call. With one span the single-buffer overload is chosen (`{ikm}` and `ikm` both name it, the
same bytes either way); with two or more only the pieces overload is viable. No overload mixes a
single span with pieces: a caller passes a lone span as `{span}` (B1).

If one of the lanes' compilers (clang, gcc, MSVC 14.3 and 14.5, Emscripten's clang) refuses
`{a, b, c}` for `std::span<std::span<std::uint8_t const> const>` in C++20 (the span constructor
from an array reference deduces the bound from the braced list; C++26 adds
`span(std::initializer_list)`), `byte_pieces` becomes a small view type of hash's with a
`std::initializer_list` constructor, a constructor from any contiguous range of byte spans, and
`begin`, `end`, `size`; the name, the call syntax and every decision below are unchanged (B2).

### D2. The computation, on the streaming HMAC

RFC 5869 with every concatenation replaced by successive `update`s of one HMAC:

- **Extract**: `PRK = HMAC(salt, piece_1, piece_2, ..., piece_n)`; an empty salt is `HashLen`
  zero bytes (RFC 5869, 2.2), which every backend's HMAC keys as the same key as an empty one,
  and which is what WebCrypto, refusing an empty HMAC key, is given (D5).
- **Expand**: `T(i) = HMAC(PRK, T(i - 1), piece_1, ..., piece_n, i)`, `T(0)` empty, the output
  the first `L` bytes of `T(1) || T(2) || ...`. The HMAC keyed by the PRK is made once and
  copied for each block where the backend allows (Hash2's object, OpenSSL's `EVP_MAC_CTX_dup`,
  BearSSL's `br_hmac_key_context`, libsodium's state struct, Botan's `clear`-and-rekey otherwise).
- **hkdf**: Extract, then Expand, under one guard, the PRK wiped between them.

Empty pieces are fed as zero-length updates (or skipped: the result is the same, and the backend
decides which its HMAC accepts). Every HMAC state, every `T(i)` and the PRK of `hkdf` are wiped
(`detail::wipe_object`, or the backend's own cleanse: `OPENSSL_cleanse`, `sodium_memzero`,
Botan's secure allocator) once their result is taken. Nothing is copied but the HMAC's own
state, which lives in the engine's frame.

### D3. The guards and the errors of the pieces forms

Each pieces form holds `detail::timing_guard` first (data-independent-timing's D1), then:

- `hkdf_expand` and `hkdf`: an empty output or one longer than `255 * HashLen`,
  `invalid_output_size`; `hkdf_expand`: a pseudorandom key of another size than the digest's,
  `invalid_key_size`. These are RFC 5869's, the same as the single-buffer forms'.
- No check on the application info, the input keying material or the number of pieces, on any
  backend: no `hkdf_max_application_info_size`, no `backend_limits::hkdf_info` or `hkdf_ikm`.
- A backend's HMAC limits apply as `mac`'s do (`backend_limits`' MAC members): the salt and the
  pseudorandom key are HMAC keys; a piece is passed to the HMAC's `update` in chunks the backend's
  length type holds (a backend whose update takes an `int` or a `long` is fed in slices of at most
  its maximum), so no piece is ever refused for its size.
- The results are the single-buffer forms': a guard's refusal leaves the output untouched, a
  backend's failure zeroes it (`backend_failure`; on OpenSSL `memory_functions_not_installed`
  where hash's OpenSSL rule applies).

### D4. The backends and their capability cells

The pieces forms are three operations of their own in the capability table, so that the existing
tags keep their meaning (BearSSL has `kdfs::hkdf` but not `kdfs::hkdf_extract`):
`kdfs::hkdf_pieces`, `kdfs::hkdf_extract_pieces` and `kdfs::hkdf_expand_pieces` in `config.hpp`.
For each backend `B` and hmac `H`, `backend_has_v<B, kdfs::hkdf_*_pieces, H>` is
`backend_has_v<B, macs::hmac<D>>` for `H`'s digest `D`: a pieces form exists exactly where the
backend's HMAC does. `default_kdf_backend<H, kdfs::hkdf_*_pieces>` is `backends::hash2`, as for
every HKDF tag (the genesis's D33). `doc/capabilities.json` gains the three rows under `kdfs`,
each cell with its engine's entry point, and `capabilities_test` checks the equality with the
HMAC cells.

| Backend | HMAC used | Notes |
|---|---|---|
| `hash2` (default) | Boost.Hash2's `hmac<H>`, `update` per piece | every hmac |
| `sodium` | `crypto_auth_hmacsha256_*`, `crypto_auth_hmacsha512_*` (init, update, final) | the hmacs of its MAC cells; libsodium's own HKDF takes Expand's info whole, so it is not used |
| `bearssl` | `br_hmac_key_init`, `br_hmac_init`, `br_hmac_update`, `br_hmac_out` | gains Extract and Expand over pieces; its single-buffer `hkdf_extract` and `hkdf_expand` stay refused (unchanged) |
| `openssl` | `EVP_MAC` "HMAC": `EVP_MAC_init`, `EVP_MAC_update`, `EVP_MAC_final`, `EVP_MAC_CTX_dup` | OpenSSL's allocations are its own, counted by `allocation_openssl` |
| `botan` | `Botan::MessageAuthenticationCode` "HMAC(<hash>)" | Botan's objects are Botan's (`operator new`, its secure allocator), as `doc/allocation.adoc` states for every Botan call |
| `webcrypto` | `subtle.sign("HMAC", ...)`, asynchronous forms only | D5 |

Each backend's header specialises a new engine, `detail::hkdf_pieces_engine<Backend, Hmac>`,
with `extract(salt, pieces)`, `expand(output, prk, pieces)` and `extract_and_expand`; the
public functions do every guard (the genesis's rule). A backend without the HMAC is refused by
the same generated refusal its MAC cell has, naming the backends that have it.

### D5. WebCrypto

`backends::webcrypto` is asynchronous only (genesis spec 11.2); the pieces forms reach it through
`async_hkdf_extract`, `async_hkdf_expand` and `async_hkdf` (D6), over the four hmacs WebCrypto
has. They use WebCrypto's HMAC, never its HKDF:

- **Gathering.** Every WebCrypto call already copies its input into JavaScript memory
  (`webcrypto_bytes`, a `slice` of a `typed_memory_view`). The pieces form allocates that copy
  once at its total size (`new Uint8Array(total)`) and `set`s each piece at its offset: the same
  single copy, made in JavaScript, nothing on the C++ heap, and no cap but JavaScript's
  `ArrayBuffer` length.
- **Extract**: `importKey("raw", salt or HashLen zeros, {name: "HMAC", hash}, false, ["sign"])`,
  then `sign("HMAC", key, gathered)`: two promises, as `async_hmac`.
- **Expand**: one JavaScript buffer of `HashLen + |info| + 1` bytes, the gathered info at offset
  `HashLen`; `importKey` of the pseudorandom key, then for each block `i` one
  `sign("HMAC", key, view)`, where the view is `info || i` for the first block and
  `T(i - 1) || info || i` after it (WebCrypto copies its input at the call, so the buffer is
  rewritten for the next block once the promise is made). Each `T(i)` is copied into the output
  when its promise settles. 1 + `ceil(L / HashLen)` promises, at most 256.
- **hkdf**: Extract's two promises, then Expand's chain, the pseudorandom key kept in JavaScript
  memory between them.
- The JavaScript buffers that held secrets are filled with zeros after the last call; JavaScript
  promises no more than that, which the WebCrypto page states already.
- `allocation`'s WebCrypto count holds as it does: each promise awaited takes webcpp::asio's one
  block for its bridge; the test checks exactly that many and nothing more for each pieces form.

The single-buffer `async_hkdf` on `backends::webcrypto` keeps WebCrypto's HKDF and its 1,024-byte
info limit; the guide's WebCrypto page says to call the pieces form, `{info}`, for a longer one.

### D6. The asynchronous forms

`async.hpp` gains, each over `hmac_sha1_160`, `hmac_sha2_256`, `hmac_sha2_384` and
`hmac_sha2_512` as `async_hkdf` is, with completion `void(boost::system::error_code)` and the
family's executor rules:

| Function | Overloads |
|---|---|
| `async_hkdf_extract(executor, pseudorandom_key, salt, input_key_material, token)` | `input_key_material` a span or `byte_pieces`; `pseudorandom_key` an output span of exactly the digest size (`invalid_output_size` otherwise) |
| `async_hkdf_expand(executor, output_key_material, pseudorandom_key, application_info, token)` | `application_info` a span (capped as `hkdf_expand`'s) or `byte_pieces` |
| `async_hkdf(executor, output_key_material, salt, input_key_material, application_info, token)` | both inputs spans (unchanged) or both `byte_pieces` |

On a synchronous backend each runs its synchronous function in the initiating call and completes
through `asio::post` (the cheap functions' rule, genesis spec 11). On `backends::webcrypto` the
pieces are gathered in the initiating call (D5). So on every backend **the list and the pieces are
read before the initiating call returns**: a braced list `{a, b, c}`, a temporary, is safe, and the
bytes may change after it; the output spans stay the caller's until completion, as in every
asynchronous call. A test changes every piece after the initiating call and checks the result.

### D7. The names

Every public parameter of hash's headers was read for abbreviations. Renamed:

| Where | Before | After |
|---|---|---|
| `hkdf_extract`, `hkdf`, `async_hkdf*` | `ikm`, `ikm_bytes` | `input_key_material` |
| `hkdf_expand`, `async_hkdf_expand` | `prk`, `prk_bytes` | `pseudorandom_key` |
| `hkdf_expand`, `hkdf`, `async_hkdf*` | `info`, `info_bytes` | `application_info` |
| `hkdf_expand`, `hkdf`, `async_hkdf*` | `output`, `output_bytes` | `output_key_material` |
| `hkdf_extract`, `hkdf`, `async_hkdf*` | `salt_bytes` | `salt` (the async forms' name; B3) |
| `hkdf.hpp` | `hkdf_max_info_size` | `hkdf_max_application_info_size` |
| `scrypt_parameters` | `n`, `r`, `p` | `cost`, `block_size`, `parallelism` (B4) |
| `argon2_parameters` | `memory_kib` | `memory_kibibytes` (B4) |

Kept, each a full word or a proper name of its standard: `salt`, `password`, `key`, `secret`,
`label`, `context`, `customisation`, `personalisation`, `seed_value`, `master_key`, `subkey_id`
(`id` as RFC 9180's and crypto's `pre_shared_key{key, id}` keep it), `fixed_info` (SP 800-56C's
FixedInfo), `shared_info` (ANSI X9.63's SharedInfo), `xof` (the term of art), `counter_bits`.
No public name is `ctx`: libsodium's `ctx` reaches hash's code only inside `detail`, where the
rule does not apply but where new code follows it anyway. `detail` names that mirror a public one
(`backend_limits::hkdf_ikm`, `hkdf_prk`, `hkdf_info`, the engines' parameters) are renamed with
it, so that a reader sees one name per thing. The documentation, `doc/capabilities.json`'s notes
and the examples use the new names; the generated files are rerun.

### D8. Tests

- **Equality with the single-buffer forms** (`hkdf_pieces_test`), on every backend that has both
  (and against `backends::hash2`'s single-buffer forms on BearSSL, which has no single-buffer
  Extract or Expand): RFC 5869's seven vectors with each input split at every byte offset into
  two pieces and into one piece per byte; 1,000 random cases from a fixed, printed seed, each
  input split at random points into 0 to 16 pieces, empty pieces among them; an empty list; a
  list of 10,000 one-byte pieces.
- **Long inputs**: an application info of 4 KiB, 32 KiB + 1 and 1 MiB, and an input keying
  material of 1 MiB, through every backend; up to 32,768 bytes compared with the single-buffer
  forms, beyond with `backends::hash2`'s pieces form on another split and with an independent
  RFC 5869 computation over Boost.Hash2's `hmac<H>` on the concatenated bytes in the test.
- **No heap**: test `allocation` gains the three pieces forms on `hash2`, `sodium` and `bearssl`,
  with 4 KiB and 1 MiB inputs and with refusals, asserting zero calls of every `operator new`
  and of the C library's allocators; `allocation_openssl` and `allocation_botan` count and print
  their libraries' own, as for every other call; the WebCrypto count is D5's.
- **WebCrypto** (the browser lane, Emscripten under node and in the browser runner): the three
  asynchronous pieces forms against `backends::hash2` and against node's
  `crypto.subtle.deriveBits` HKDF within its 1,024-byte info, then with a 64 KiB info against
  `backends::hash2`; the gathering checked by a piece list whose bytes change after the
  initiating call.
- **Asynchronous forms**: each of the new forms on a synchronous backend with a callback,
  `use_awaitable` and `use_future`, completing through a post; refusals; the temporary-list case
  of D6.
- **Timing**: the coverage tests' registries gain a call per pieces cell (the table's new rows
  fail the tests until written, data-independent-timing's D5); `timing_during` gains the three
  pieces forms.
- **Names**: a compile test spells every renamed field with designated initializers; the message
  tests of the genesis that quote a parameter name are updated.
- Every lane: native (clang, gcc), MSVC 14.3 and 14.5, wasip2, wasip3, Emscripten, the Linux
  lint.

### D9. Documentation

`doc/kdfs.adoc` gains "HKDF over pieces" (what it is for, the overloads, no cap, no heap, the
`{a, b, c}` call, the table of where each form runs, HPKE's labeled inputs as the example);
`doc/async.adoc` the three asynchronous HKDF forms; the WebCrypto page the HMAC-based pieces forms
and the 1,024-byte limit of the single-buffer form only; `doc/allocation.adoc` the pieces rows
from release builds (`allocation_release`, never a debug build); the changelog's source a line.
Every page uses D7's names.

## Builder's calls

Settled while reading the change as a builder would; the plan may change them without returning
to the owner.

- **B1: no mixed overloads.** One span and one list (`hkdf(out, salt, ikm, {a, b})`) finds no
  overload; the caller writes `{ikm}`. Two more overloads per function would double the set for
  no consumer.
- **B2: `byte_pieces`' spelling** is the alias where every lane's compiler takes `{a, b, c}`, a
  view type of hash's otherwise (D1); the plan's first task builds the call on every lane and
  records which.
- **B3: `salt_bytes` becomes `salt`** in HKDF's functions, so that HKDF's parameters are named
  alike in its synchronous and asynchronous forms. The `_bytes` suffix of the other KDFs
  (`password_bytes`, `salt_bytes`, `output_bytes` of `pbkdf2`, `scrypt`, `argon2id`) is not an
  abbreviation and stays.
- **B4: the KDF parameter fields.** `n`, `r` and `p` are RFC 7914's symbols, not words;
  `memory_kib` abbreviates kibibytes. They become `cost`, `block_size`, `parallelism` and
  `memory_kibibytes`, each field's comment keeping the RFC's symbol. Owner decision 1.
- **B5: the asynchronous Extract and Expand** join with both overloads, single-buffer and
  pieces, so that every synchronous HKDF function has its asynchronous form; on WebCrypto the
  single-buffer `async_hkdf_extract` and `async_hkdf_expand` run the pieces engine with one piece
  (WebCrypto has no separate HKDF steps).
- **B6: no measurement of speed** in this change: a pieces form makes the same HMAC calls as
  hash2's single-buffer form; a later throughput table is a release build's, never a debug
  build's.

## Risks / Trade-offs

- [The pieces forms on `sodium`, `openssl`, `bearssl` and `botan` do not use the library's own
  HKDF] -> they compute HKDF on the library's HMAC, the same primitive; the vectors and the
  equality tests pin them to the single-buffer forms, which do.
- [A program passes a list whose spans dangle] -> the list is read in the call (and, for the
  asynchronous forms, in the initiating call); the pieces' bytes are the caller's as every span
  is.
- [Renamed fields break a program's designated initializers] -> hash has no release yet; the
  changelog lists every rename.
- [WebCrypto's Expand makes up to 256 promises] -> one per 32 to 64 bytes of output; HKDF outputs
  are short (HPKE's at most `255 * HashLen` and typically one or two blocks).

## Migration Plan

Branches `hash/hkdf-pieces` of webcpporg/hash, webcpporg/proposals and the superproject (the
pointer); local verification on every lane; merged by the controller before crypto's `hpke`
starts; one CI at the end.

## Owner Decisions on the Open Questions (2026-11-23)

1. **The KDF parameter fields (B4)**: spelled out, `scrypt_parameters{cost, block_size,
   parallelism}` and `argon2_parameters::memory_kibibytes`, each field's comment keeping the
   RFC's symbol.
2. **`fixed_info`, `shared_info` and `subkey_id` (D7)**: kept as their standards name them.
3. **The change as written**: approved.
