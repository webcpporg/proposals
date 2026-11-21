## Purpose

webcpp::hash is a header-only C++20 library that gives a program one entry point for hashing and
key derivation on native, wasip2, wasip3 and Emscripten: Boost.Hash2's algorithms and HMAC by
alias, Keccak-256 and BLAKE3 in Hash2's HashAlgorithm form over XKCP and BLAKE3's C
implementation, HKDF and PBKDF2 over backends (webcpp's own over Hash2's hmac, libsodium,
BearSSL, OpenSSL) with `backends::hash2` as the default on every target, and Argon2id and scrypt
from libsodium; from increment 2, the families `digest_of`, `hasher`, `mac` and `argon2` over every
backend's hashes, MACs and KDFs, and the asynchronous family with the browser's WebCrypto. These
requirements are the structured form of `reference/spec.md` and of the build, tests and tools of
`reference/plan.md`; increments 1 to 5 and 7 are built in the library's repository, not yet
published, and increment 6 (Botan) is the stream's own, with Botan provisioned by it (design D43).

## ADDED Requirements

### Requirement: Library: header-only target, names and headers
The library SHALL be header-only C++20 in namespace `webcpp::hash` (with `webcpp::hash::backends`, `webcpp::hash::kdfs`, `webcpp::hash::digests`, `webcpp::hash::macs` and `webcpp::hash::detail`), with public headers under `<webcpp/hash/...>`, macros prefixed `WEBCPP_HASH_`, include guards `WEBCPP_HASH_<PATH>_HPP` and the error category `webcpp.hash`. A program MUST reach it through the b2 target `/webcpp/hash//hash`, an alias that gives `libs/hash/include`, Boost's headers and `/webcpp/config//config`. Every public header MUST include `<webcpp/hash/config.hpp>` first, which includes `<webcpp/config.hpp>` first.

#### Scenario: A program that only hashes links no C library
- **WHEN** a Jamfile reads `exe app : app.cpp /webcpp/hash//hash ;` and `app.cpp` computes `webcpp::hash::sha2_256` of "abc" and an HKDF on `backends::hash2`
- **THEN** it builds and runs on native, wasip2, wasip3 and Emscripten with no library of `/third_party` named or built

### Requirement: Library: configuration and exceptions
`<webcpp/hash/config.hpp>` SHALL define `WEBCPP_HASH_NO_EXCEPTIONS` exactly when `WEBCPP_HAS_EXCEPTIONS` is not defined, keep it when a developer defines it, define `WEBCPP_HASH_WEBASSEMBLY` on wasip2, wasip3 and Emscripten, declare the backend tags `backends::hash2`, `sodium`, `bearssl`, `openssl`, `xkcp`, `blake3` and `webcrypto` (on every target), the public KDF tags `kdfs::hkdf`, `kdfs::hkdf_extract`, `kdfs::hkdf_expand`, `kdfs::pbkdf2`, `kdfs::argon2id` and `kdfs::scrypt`, one per operation, and the families' tags of `digests`, `macs` and `kdfs` (spec 2.15), define `WEBCPP_HASH_HAS_THREADS` natively when `BOOST_HAS_THREADS` is defined and on Emscripten with `__EMSCRIPTEN_PTHREADS__`, never on WASI and never with `WEBCPP_HASH_NO_THREADS`, and refuse with `#error` OpenSSL on Emscripten (`/webcpp/hash//openssl`). It MUST read no profile and no override define (design D33). No function of the library SHALL throw, with exceptions on or off.

#### Scenario: The configuration test three ways
- **WHEN** `test/config_test.cpp` is built as the build asks, with `WEBCPP_NO_EXCEPTIONS`, and with `WEBCPP_HASH_NO_EXCEPTIONS` on the command line
- **THEN** each build passes, and `WEBCPP_HASH_NO_EXCEPTIONS` is defined exactly when the first build has no exceptions or the macro was given

### Requirement: Library: errors are values
Every function that can fail SHALL return `boost::system::result<T>` or `result<void>`. The enumeration `error` MUST have the fixed values `invalid_output_size = 1`, `invalid_key_size = 2`, `invalid_salt_size = 3`, `invalid_iterations = 4`, `invalid_parameters = 5`, `input_too_long = 6`, `backend_failure = 7` and `unsupported_by_backend = 8` (and `invalid_state = 9`, which increment 2 adds with the families), never reused, in a category named `webcpp.hash` with a fixed identity and no mutable state; a failed allocation inside a C library MUST be `boost::system::errc::not_enough_memory`.

#### Scenario: Every value has its message
- **WHEN** a test makes the `error_code` of each value and reads `message()` and `category().name()`
- **THEN** each message is the text of spec 2.3 and the name is `webcpp.hash`, with exceptions on and off

### Requirement: Library: every result is [[nodiscard]]
Every function of the library's `include/` that returns `boost::system::result` SHALL be declared `[[nodiscard]]`, since `boost::system::result` carries no `[[nodiscard]]` of its own and a dropped refusal would pass as a success with the output unwritten. `libs/hash/meta/lint.json` MUST opt the library into the lint's rule "nodiscard results", with that reason.

#### Scenario: A result-returning function without the attribute
- **WHEN** a header of `libs/hash/include/` declares a function that returns `boost::system::result<void>` without `[[nodiscard]]` and the lint runs
- **THEN** the rule "nodiscard results" reports it at its line and the lint fails

#### Scenario: A dropped result
- **WHEN** a program calls `webcpp::hash::hkdf<hmac_sha2_256>(output, salt, ikm, info);` and discards the result
- **THEN** the compiler warns that a `[[nodiscard]]` result is discarded

### Requirement: Library: inputs and outputs are spans, and nothing is allocated by webcpp's code
Every input SHALL be a `std::span<std::uint8_t const>` and every output a `std::span<std::uint8_t>`, sizes `std::size_t`, and a limit wider than `size_t` on wasm32 MUST be compared through `detail::exceeds(std::uint64_t, std::uint64_t)`. No function of webcpp::hash SHALL allocate, but the asynchronous family, whose operation state and completion come from the handler's associated allocator and are given back before the handler runs (spec 6.1); the C libraries that allocate inside a function (libsodium's and OpenSSL's Argon2id and scrypt, OpenSSL's KDFs, digest and MAC contexts) are the recorded departures of spec 6.2.

#### Scenario: A counting operator new sees nothing
- **WHEN** `allocation_test` replaces every form of `operator new` with a counter and calls every hash type and every KDF on `backends::hash2`, `sodium` and `bearssl`
- **THEN** the counter is zero after every call, on every target

### Requirement: Boost.Hash2: every algorithm by alias
`<webcpp/hash/algorithms.hpp>` SHALL declare, as aliases of Boost.Hash2 1.92's types, `md5_128`, `sha1_160`, `sha2_224`, `sha2_256`, `sha2_384`, `sha2_512`, `sha2_512_224`, `sha2_512_256`, `sha3_224`, `sha3_256`, `sha3_384`, `sha3_512`, `shake_128`, `shake_256`, `ripemd_128`, `ripemd_160`, `blake2b_512`, `blake2s_256`, `fnv1a_32`, `fnv1a_64`, `siphash_32`, `siphash_64`, `xxhash_32`, `xxhash_64`, `xxh3_128`, `murmur3_32`, `murmur3_128` and `spooky2_128`, the alias template `digest<N>`, and using-declarations of `hash_append`, `hash_append_range`, `hash_append_size`, `hash_append_range_and_size`, `hash_append_unordered_range` and `get_integral_result`. No wrapper type MUST stand between a user and Hash2.

#### Scenario: The aliases are Hash2's types
- **WHEN** a test checks `std::is_same_v<webcpp::hash::sha2_256, boost::hash2::sha2_256>` for each alias, and hashes a `std::string` "abc" with `webcpp::hash::hash_append` into `webcpp::hash::sha2_256`
- **THEN** every check holds and the digest equals Hash2's for the same call

### Requirement: Boost.Hash2: HMAC by alias
`<webcpp/hash/hmac.hpp>` SHALL declare `template <class H> using hmac = boost::hash2::hmac<H>;` and the aliases `hmac_md5_128`, `hmac_sha1_160`, the six `hmac_sha2_*`, the four `hmac_sha3_*`, `hmac_ripemd_128`, `hmac_ripemd_160`, `hmac_blake2b_512` and `hmac_blake2s_256` of Hash2's. The documentation MUST say that a MAC is compared with `webcpp::crypto::equal`, not `digest<N>::operator==`.

#### Scenario: RFC 4231's first case
- **WHEN** `webcpp::hash::hmac_sha2_256` is keyed with 20 bytes of `0x0b` and given "Hi There"
- **THEN** the result is `b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7`

### Requirement: Keccak-256: Hash2's form over XKCP's permutation
`<webcpp/hash/keccak.hpp>` SHALL declare `keccak_256` with `result_type` `boost::hash2::digest<32>`, `block_size` 136, the default, `std::uint64_t` and bytes constructors, `update` and `result`, whose permutation is XKCP's Keccak-p[1600] (`KeccakP-1600-opt64.c`, the "generic64" profile) reached through webcpp's adapter `src/xkcp_adapter.c`, and whose absorbing, padding (`0x01`, `0x80`) and squeezing MUST give the results of Boost.Hash2's `detail::keccak_base<0x01, 512, 256>` for every sequence of calls. No XKCP header SHALL reach a user's translation unit.

#### Scenario: The published digests
- **WHEN** `keccak_256` hashes the empty message and "abc"
- **THEN** the digests are `c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470` and `4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45`

#### Scenario: Hash2's sponge is the oracle
- **WHEN** a test runs the same random sequence of `update` (lengths 0 to 1,000) and `result` calls on `keccak_256` and on `boost::hash2::detail::keccak_base<0x01, 512, 256>`, with a fixed, printed seed
- **THEN** every result is equal, on every target

### Requirement: Keccak-256: an alias once Boost.Hash2 has it
The documentation SHALL state that a public `keccak_256` contributed to Boost.Hash2 is the long-term plan, after which `webcpp::hash::keccak_256` becomes its alias and stops needing `/third_party//xkcp`, with no change of results; the plan MUST prepare the contribution's text, and opening it is the owner's action.

#### Scenario: The contribution is ready
- **WHEN** the plan's last task is done
- **THEN** `libs/hash/doc/upstream.adoc` holds the text of the contribution and the library's page links it

### Requirement: BLAKE3: Hash2's form over the official C implementation
`<webcpp/hash/blake3.hpp>` SHALL declare `blake3` over BLAKE3 1.8.7's `c/blake3.c` with `result_type` `digest<32>`, `block_size` 64, `key_size` 32, the default, `std::uint64_t` and bytes constructors (a bytes seed keys the hash with its first 32 bytes, zero-padded, the rest absorbed and finalized), `keyed(std::span<std::uint8_t const, 32>)`, `derive_key(std::string_view)`, `update`, `result()` (the digest, then the next 32 bytes of the extendable output at each call, reset when input is added), `squeeze(output, position)` and `reset()`. The type MUST be copyable.

#### Scenario: BLAKE3's official vectors
- **WHEN** `blake3_vectors_test` reads `test_vectors/test_vectors.json` at the pin and checks every case's `hash`, `keyed_hash` and `derive_key` at full length, once through `squeeze` and once through repeated `result()`
- **THEN** every case matches on native, wasip2, wasip3 and Emscripten

### Requirement: BLAKE3: portable on WebAssembly, SIMD dispatch natively
`/third_party//blake3` SHALL compile `blake3.c`, `blake3_dispatch.c` and `blake3_portable.c` on every target, the SSE2, SSE4.1, AVX2 and AVX-512 intrinsics files natively on x86-64 (each with its instruction set's flag) and `blake3_neon.c` natively on arm64 with `BLAKE3_USE_NEON=1`, and on WebAssembly define `BLAKE3_NO_SSE2`, `BLAKE3_NO_SSE41`, `BLAKE3_NO_AVX2` and `BLAKE3_NO_AVX512`; the restriction of these files to their instruction sets MUST be recorded beside the sources and in CODING-GUIDELINES.md chapter 10's row, as an exception to chapter 5 rule 2.

#### Scenario: The canary names the profile
- **WHEN** `nm` lists `/third_party//blake3`'s archive on native x86-64, native arm64 and wasip2
- **THEN** it holds `webcpp_hash_blake3_f3149ec_x86__add_third_party_blake3_to_your_link`, `..._neon__...` and `..._portable__...` respectively

### Requirement: Key derivation: backends as a template parameter
`hkdf_extract`, `hkdf_expand`, `hkdf` and `pbkdf2` SHALL be function templates whose first parameter is a Boost.Hash2 hmac over a `digest<N>` result and whose second is the backend, defaulting to the backend chosen for the operation itself: `default_kdf_backend<Hmac, kdfs::hkdf_extract>` for `hkdf_extract`, `default_kdf_backend<Hmac, kdfs::hkdf_expand>` for `hkdf_expand`, `default_kdf_backend<Hmac>` (`kdfs::hkdf`) for `hkdf` and `default_kdf_backend<Hmac, kdfs::pbkdf2>` for `pbkdf2`; `argon2id` and `scrypt` default to `default_backend<kdfs::argon2id>` and `default_backend<kdfs::scrypt>`. The backends MUST be `backends::hash2`, `backends::sodium`, `backends::bearssl` and `backends::openssl`, named after the library that does the work, and `backend_has_v<Backend, Kdf, Hmac>` MUST state the table of spec 3.2 for every tag of `kdfs`, `hkdf_extract` and `hkdf_expand` false for `backends::bearssl`. The public functions SHALL do every guard; a backend's header specialises only the engines in `webcpp::hash::detail`.

#### Scenario: One call, four backends
- **WHEN** a native program that links libsodium, BearSSL and OpenSSL calls `hkdf<hmac_sha2_256, B>` with RFC 5869 A.1's inputs for each of the four backends `B`
- **THEN** each returns A.1's 42-byte OKM

#### Scenario: A step never defaults to a backend that lacks it
- **WHEN** a wasip2 program that names `/third_party//bearssl` and `/third_party//sodium` calls `hkdf_extract<hmac_sha2_512>` and `hkdf<hmac_sha2_512>`
- **THEN** both compile and run on `backends::hash2`, and `backend_has_v<backends::bearssl, kdfs::hkdf_extract, hmac_sha2_512>` is false

### Requirement: Key derivation: a backend's narrower lengths are its limits
A backend whose C interface holds a length in a type narrower than `std::size_t` SHALL declare its limits in `detail::backend_limits<Backend>` (`detail/engines.hpp`), one member per input and output of each operation, the primary template having no limit beyond the spec's. The public functions MUST read it after the spec's own guards and before any call of the backend, refusing a longer output with `invalid_output_size` and a longer input with `input_too_long`, the output untouched. `backends::openssl` MUST limit HKDF's salt and PRK and PBKDF2's password, salt and output to `INT_MAX` bytes, the lengths OpenSSL 3.5.9 holds in an `int` (a PBKDF2 output of 2^31 bytes crashed it), and its documentation MUST state them; `backends::hash2` accepts these lengths.

#### Scenario: PBKDF2 one byte past OpenSSL's int
- **WHEN** a native test calls `pbkdf2<hmac_sha2_256, backends::openssl>` with an output span of `INT_MAX + 1` bytes, never dereferenced
- **THEN** it returns `invalid_output_size` before OpenSSL is called, and with a salt span of `INT_MAX + 1` bytes it returns `input_too_long`

#### Scenario: Within the limit
- **WHEN** the same test derives a key on `backends::openssl` with short inputs
- **THEN** it succeeds and writes the output, and `backends::hash2` has no limit of its own (its `backend_limits` is the primary template's)

### Requirement: Key derivation: a backend without the function refuses at compile time
A call of a KDF on a backend that lacks it, or over an hmac the backend lacks, SHALL fail to compile with exactly the text of spec 3.3 that names the backends that have it, raised by a `static_assert` of the engine's primary template or of the backend's refusing specialisation, each text tested in a directory of its own.

#### Scenario: PBKDF2 on libsodium
- **WHEN** a program calls `pbkdf2<hmac_sha2_512, backends::sodium>(...)`
- **THEN** compilation fails with "webcpp::hash: pbkdf2 is not in backends::sodium: libsodium has no PBKDF2. The backends that have pbkdf2: backends::hash2 (every hmac) and backends::openssl (every hmac but hmac_ripemd_128)."

#### Scenario: The two steps on BearSSL
- **WHEN** a program calls `hkdf_expand<hmac_sha2_256, backends::bearssl>(...)`
- **THEN** compilation fails with spec 3.3's text that names `hkdf`, `backends::hash2`, `backends::sodium` and `backends::openssl`

### Requirement: Key derivation: backends::hash2 is webcpp's construction, under conditions
`backends::hash2` SHALL implement RFC 5869's extract and expand and RFC 8018's PBKDF2 over `boost::hash2::hmac<H>` for every Hash2 hmac whose result is a `digest<N>`, on every target, in at most 150 lines for both, with the RFCs' step names in comments, wiping every intermediate (T blocks, U values, keyed hmac copies) before it returns and comparing no secret. It is the departure from "webcpp writes no cryptography" the owner recorded on 2026-11-14, and the documentation MUST state it with its conditions.

#### Scenario: Wiped intermediates
- **WHEN** a test built with the seam `WEBCPP_HASH_DETAIL_TEST_WIPE_RECORD` calls `pbkdf2<hmac_sha2_256, backends::hash2>` with 2 iterations
- **THEN** the seam records a wipe of every U block, the accumulator and both keyed hmac copies

### Requirement: HKDF: the interface and the guards
`hkdf_extract(salt, ikm)` SHALL return `result<typename Hmac::result_type>`; `hkdf_expand(output, prk, info)` and `hkdf(output, salt, ikm, info)` SHALL return `result<void>`. Before any work, `hkdf_expand` and `hkdf` MUST refuse an empty `output` or one longer than 255 times the digest size (`invalid_output_size`), `hkdf_expand` a PRK of another size than the digest's (`invalid_key_size`), and both an info longer than `hkdf_max_info_size`, 32,768 bytes (`input_too_long`), on every backend, then the backend's own limits (`backend_limits`); a guard's refusal leaves `output` untouched and a backend's failure zeroes it.

#### Scenario: RFC 5869 on every backend
- **WHEN** the seven cases of RFC 5869's appendix A run through `hkdf_extract`, `hkdf_expand` and `hkdf` on every backend that has the hmac and the function, on every target that backend has
- **THEN** every PRK and OKM matches the RFC

#### Scenario: The longest output and one byte more
- **WHEN** `hkdf<hmac_sha2_256>` is asked for 8,160 bytes and then for 8,161
- **THEN** the first succeeds and the second returns `invalid_output_size` with `output` untouched, on every backend

### Requirement: PBKDF2: the interface and the guards
`pbkdf2(output, password, salt, iterations)` SHALL return `result<void>`, refuse an empty `output` or one longer than (2^32 - 1) times the digest size (`invalid_output_size`) and `iterations == 0` (`invalid_iterations`) before any work, then the backend's own limits (`backend_limits`), and run OpenSSL with `pkcs5 = 1` so that SP 800-132's lower bounds never apply.

#### Scenario: RFC 6070 and RFC 7914
- **WHEN** RFC 6070's six SHA-1 cases and RFC 7914 section 11's two SHA-256 cases run through `pbkdf2` on `backends::hash2` and `backends::openssl`
- **THEN** every derived key matches, the 16,777,216-iteration case included, in its own program

### Requirement: The default backend is backends::hash2 for every key derivation
`default_kdf_backend<Hmac, Kdf = kdfs::hkdf>` and `default_backend<Algorithm, Hmac = void>` SHALL keep their names and signatures, and for `kdfs::hkdf`, `kdfs::hkdf_extract`, `kdfs::hkdf_expand` and `kdfs::pbkdf2` over every hmac they MUST be `backends::hash2`, on native, wasip2, wasip3 and Emscripten, whatever libraries the program links; their answer MUST NOT depend on the hmac, the link set or the target. For `kdfs::argon2id`, `kdfs::scrypt` and `kdfs::blake2b_subkey`, which `backends::hash2` does not offer, `default_backend` MUST be `backends::sodium` (OpenSSL's Argon2id and scrypt, from increment 4, are reached only by naming them). No measurement SHALL decide a default (design D33).

#### Scenario: HKDF's expand step with libsodium linked
- **WHEN** a native program that names `/third_party//sodium` calls `hkdf_expand<hmac_sha2_256>` with RFC 5869 A.1's PRK and info
- **THEN** `default_kdf_backend<hmac_sha2_256, kdfs::hkdf_expand>` is `backends::hash2` and the call returns A.1's OKM computed by `backends::hash2`

#### Scenario: PBKDF2 with OpenSSL linked
- **WHEN** a native program that names `/webcpp/hash//openssl` calls `pbkdf2<hmac_sha2_256>`
- **THEN** it runs on `backends::hash2`, and `default_kdf_backend<hmac_sha2_256, kdfs::pbkdf2>` is `backends::hash2`

#### Scenario: Every library linked, every target
- **WHEN** `default_backend_test` is built on native, wasip2, wasip3 and Emscripten with every library the target has, and reads `default_kdf_backend<Hmac, Kdf>` for each hmac and each of `kdfs::hkdf`, `kdfs::hkdf_extract`, `kdfs::hkdf_expand` and `kdfs::pbkdf2`
- **THEN** every answer is `backends::hash2`, and `default_backend<kdfs::argon2id>` and `default_backend<kdfs::scrypt>` are `backends::sodium`

#### Scenario: Argon2id without libsodium
- **WHEN** a program calls `argon2id` (or `scrypt`) with the default backend and does not name `/third_party//sodium`, as the messages `argon2id_default_not_linked` and `scrypt_default_not_linked` do
- **THEN** compilation fails at the primary template of the engine in `detail/engines.hpp` with "webcpp::hash: backends::sodium needs libsodium. Add /third_party//sodium to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//sodium ;"

### Requirement: BearSSL, OpenSSL and libsodium's HKDF only when named in the type
`backends::bearssl`, `backends::openssl` and the HKDF of `backends::sodium` SHALL stay, with their vectors, agreement tests, limits and refusals as before, and SHALL be reached only by naming the backend in the call's type, as `hkdf<hmac_sha2_256, backends::openssl>`; neither `backends::bearssl` nor `backends::openssl` MUST ever be a default, on any target or link set, for any function or tag (design D34).

#### Scenario: OpenSSL named in the type
- **WHEN** a native program that names `/webcpp/hash//openssl` calls `hkdf<hmac_sha2_256, backends::openssl>` and `hkdf<hmac_sha2_256>` with RFC 5869 A.1's inputs
- **THEN** the first runs on `backends::openssl`, the second on `backends::hash2`, and both return A.1's OKM

#### Scenario: BearSSL named in the type
- **WHEN** a wasip2 program that names `/third_party//bearssl` calls `hkdf<hmac_sha2_256, backends::bearssl>` and `hkdf<hmac_sha2_256>`
- **THEN** the first runs on `backends::bearssl` and the second on `backends::hash2`

### Requirement: No define changes a default
webcpp::hash SHALL have no profile and no per-algorithm override: it MUST NOT read `WEBCPP_HASH_PROFILE_FASTEST`, `WEBCPP_HASH_PROFILE_SMALLEST`, `WEBCPP_HASH_PROFILE_PORTABLE`, `WEBCPP_HASH_PROFILE_BACKEND_<NAME>`, webcpp's umbrella `WEBCPP_PROFILE_FASTEST`, `_SMALLEST` and `_PORTABLE`, or `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`, and a former profile or override define MUST have no effect. The library MUST hold no `detail/ranking.hpp`, no `detail/overrides.hpp` and no `#pragma detect_mismatch` of a profile or of an override.

#### Scenario: A former profile define
- **WHEN** a native program that names `/webcpp/hash//openssl` is built with `WEBCPP_HASH_PROFILE_FASTEST` (or `WEBCPP_PROFILE_SMALLEST`, or `WEBCPP_HASH_PROFILE_BACKEND_OPENSSL`) and calls `pbkdf2<hmac_sha2_256>`
- **THEN** it compiles with no diagnostic from webcpp::hash and runs on `backends::hash2`, as without the define

#### Scenario: Former override defines
- **WHEN** `default_backend_former_defines`, `default_backend_test` built with `WEBCPP_HASH_PROFILE_FASTEST`, `WEBCPP_HASH_PROFILE_SMALLEST`, `WEBCPP_HASH_PROFILE_BACKEND_BEARSSL`, `WEBCPP_PROFILE_FASTEST`, `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_SODIUM` and `WEBCPP_HASH_DEFAULT_BACKEND_PBKDF2_BEARSSL` and every library linked, runs on native, wasip2, wasip3 and Emscripten
- **THEN** every default stays `backends::hash2` for the key derivations and `backends::sodium` for `argon2id` and `scrypt`

#### Scenario: Two former profiles at once
- **WHEN** a program defines `WEBCPP_HASH_PROFILE_FASTEST` and `WEBCPP_HASH_PROFILE_SMALLEST`
- **THEN** it compiles, and every default is `backends::hash2`

### Requirement: The measured table is dated reference documentation
`doc/measurements.json` SHALL be kept as a reference measured on 2026-11-16, labelled so, which decides no default and orders no backend, and `doc/measurements.adoc` SHALL be rendered from it by `tools/hash/doc_tables.py` with that label, without the orders and without a default per key derivation; the guide's comparison tables MAY read them. `tools/hash/doc_tables.py --check`, run at every documentation build, MUST fail when `measurements.json`'s digest does not match its content. No lane SHALL re-measure or compare them: the own lane `measure`, `tools/hash/measure.py` (every mode) and its test, the CI matrix entry "Own lane (hash, measure)" and `libs/hash/test/measure` MUST NOT exist.

#### Scenario: The reference is labelled
- **WHEN** a reader opens `doc/measurements.adoc`
- **THEN** it says that the table was measured on 2026-11-16, that it is a reference, and that it decides no default

#### Scenario: A hand edit of the reference
- **WHEN** a number of `doc/measurements.json` is changed by hand and the documentation is built
- **THEN** `tools/hash/doc_tables.py --check` fails, since the digest no longer matches the content

#### Scenario: A pin move
- **WHEN** a pin of libsodium, BearSSL, OpenSSL, XKCP or BLAKE3 moves
- **THEN** no lane re-measures sizes or throughput, no default changes, and the measured table keeps its date

### Requirement: The throughput of PBKDF2-HMAC-SHA-512
`libs/hash/bench/pbkdf2_sha512.cpp` SHALL measure PBKDF2-HMAC-SHA-512 at 1,000,000 iterations and 64 bytes of output, the median of three runs, for `backends::hash2`, `backends::openssl` (native, wasip2, wasip3) and libsodium's HMAC-SHA-512 in RFC 8018's loop (libsodium has no PBKDF2), and `doc/performance.adoc` MUST record the results with the machine, compiler, target and date, measured by hand with `bench/collect.py`; the benchmark's correctness program `pbkdf2_sha512_key`, which checks the derived keys and never the time, MUST run in every lane of `libs/hash/test`.

#### Scenario: The table is filled
- **WHEN** the throughput task is done
- **THEN** `doc/performance.adoc` holds a time for each of the three on native, and for each that exists on wasip2, wasip3 and Emscripten

### Requirement: Argon2id from libsodium
`argon2id<Backend = default_backend<kdfs::argon2id>>(output, password, salt, argon2id_parameters)`, whose default is `backends::sodium`, SHALL compute Argon2id version 0x13 with one lane, no secret and no associated data through `crypto_pwhash_argon2id` (or OpenSSL's "ARGON2ID" with `backends::openssl` named, from increment 4), refusing in order an output shorter than 16 or longer than 4,294,967,295 bytes (`invalid_output_size`), a salt of another size than 16 (`invalid_salt_size`), a password longer than 4,294,967,295 bytes (`input_too_long`), and passes outside 1 to 4,294,967,295 or memory below 8,192, not a multiple of 1,024 or above the engine's maximum (`invalid_parameters`), then the backend's `detail::backend_limits`. `argon2id_interactive`, `argon2id_moderate` and `argon2id_sensitive` MUST equal libsodium's presets, checked by `static_assert`.

#### Scenario: RFC 9106 through libsodium's core
- **WHEN** a test that includes libsodium's private `argon2.h` runs RFC 9106 section 5.3's Argon2id case (t 3, m 32 KiB, p 4, with secret and associated data) through `argon2_ctx`
- **THEN** the tag is the RFC's

#### Scenario: A salt of 15 bytes
- **WHEN** `argon2id` is called with a 15-byte salt
- **THEN** it returns `invalid_salt_size` and `output` is untouched

### Requirement: scrypt from libsodium
`scrypt<Backend = default_backend<kdfs::scrypt>>(output, password, salt, scrypt_parameters{n, r, p})`, whose default is `backends::sodium`, SHALL compute RFC 7914's scrypt through `crypto_pwhash_scryptsalsa208sha256_ll` (or OpenSSL's "SCRYPT" with `backends::openssl` named, from increment 4, refusing an r p of 2^24 or more), refusing an empty output or one past (2^32 - 1) times 32 bytes (`invalid_output_size`) and N not a power of two greater than 1, N not below 2^(16 r), r or p of 0, r times p not below 2^30, or 128 r (N + p) bytes that do not fit `size_t` (`invalid_parameters`); libsodium's ENOMEM MUST be `errc::not_enough_memory`.

#### Scenario: RFC 7914's vectors
- **WHEN** RFC 7914 section 12's four cases run through `scrypt`
- **THEN** each derived key matches, the 1 GiB case in a program of its own on native and on the WebAssembly targets where the plan measured it runs

### Requirement: Memory-hard functions allocate inside libsodium, a recorded departure
The documentation SHALL state that libsodium allocates Argon2id's and scrypt's memory (`mmap` or `malloc`) outside any allocator of the program's, as OpenSSL does for its own (increment 4), that this departure from CODING-GUIDELINES.md chapter 4 is the owner's, and that the Argon2 reference implementation's allocation callbacks would route Argon2id's memory through a `std::pmr::memory_resource` as a later `backends::argon2`, not built.

#### Scenario: The departure is on the page
- **WHEN** a reader opens the library's "Allocation" section
- **THEN** it names libsodium's allocation in `argon2id` and `scrypt`, OpenSSL's in its KDFs, and the alternative

### Requirement: Build: the C libraries file by file, with webcpp::hash's canaries
`third_party/Jamfile` SHALL build `/third_party//xkcp` (XKCP at `d20c8fc`, `KeccakP-1600-opt64.c` and, from increment 3, `KeccakSponge.c`, `KeccakHash.c`, `SP800-185.c`, `TurboSHAKE.c` and `KangarooTwelve.c`, webcpp's adapter and `config.h` with the `XKCP_has_*` of those components), `/third_party//blake3` (BLAKE3 1.8.7 at `f3149ec`) and `/third_party//sodium` (libsodium 1.0.22 at `77e1ce5`, every `.c` of `src/libsodium/`, with upstream's `builds/msvc/version.h` copied into the build directory and the defines libsodium's own `configure` finds for each target, `DEV_MODE` off, without assembly) as static libraries from submodules under `third_party/libs/`, never edited, each with webcpp::hash's canary `src/canary_<name>.c`, `-ffunction-sections -fdata-sections`, `<warnings>off`, and usage requirements giving `WEBCPP_HASH_HAS_<NAME>=1` and the removal of unused sections; `/third_party//sodium`'s usage requirements MUST also give `-sALLOW_MEMORY_GROWTH=1` to an Emscripten link, since Argon2id and scrypt allocate the memory their costs name past Emscripten's fixed 16 MiB. `/third_party//bearssl` MUST gain webcpp::hash's canary and `WEBCPP_HASH_HAS_BEARSSL=1`.

#### Scenario: libsodium's archive
- **WHEN** `/third_party//sodium` is built natively with `-a -d+2`
- **THEN** b2 compiles 142 C files (the 141 of `src/libsodium/` and the canary), none with `-std=`, each with `-DCONFIGURED=1 -DSODIUM_STATIC=1 -DNATIVE_LITTLE_ENDIAN=1`, and `nm` of the archive lists `webcpp_hash_sodium_77e1ce5_configured__add_third_party_sodium_to_your_link`

#### Scenario: Argon2id on Emscripten without knowing the flag
- **WHEN** an Emscripten program names `/third_party//sodium` and calls `argon2id` with `argon2id_interactive` (64 MiB)
- **THEN** it links with `-sALLOW_MEMORY_GROWTH=1` from the library's usage requirements and derives the key under node

### Requirement: Build: three layers stop a wrong build
A header whose functions call a C library SHALL `#error` with the Jamfile line of spec 5.7 when its `WEBCPP_HASH_HAS_<NAME>` is missing; every such function MUST read its library's canary, so that a program linked without it, or with another commit or configuration, fails to link with the canary's name; and on MSVC the header and the canary MUST carry `#pragma detect_mismatch("webcpp_hash_backend_<name>", "<name>-<commit7>-<configuration>")`.

#### Scenario: keccak_256 without XKCP
- **WHEN** a program includes `<webcpp/hash/keccak.hpp>` and does not name `/third_party//xkcp`
- **THEN** compilation fails with "webcpp::hash: keccak_256 needs XKCP. Add /third_party//xkcp to the program's Jamfile: exe app : app.cpp /webcpp/hash//hash /third_party//xkcp ;"

#### Scenario: The define without the library
- **WHEN** a program defines `WEBCPP_HASH_HAS_SODIUM=1` itself, calls `argon2id` and links no libsodium
- **THEN** the link fails naming `webcpp_hash_sodium_77e1ce5_configured__add_third_party_sodium_to_your_link`

### Requirement: Build: OpenSSL through /webcpp/hash//openssl
`libs/hash/build.jam` SHALL declare `openssl`, an alias that on native, wasip2 and wasip3 links `/webcpp/third_party//openssl` (the superproject's one source-built OpenSSL 3.5.9) and defines `WEBCPP_HASH_HAS_OPENSSL=1`, and on Emscripten stops every source with the `#error` of spec 2.2 through a forced include. A program that names it and `/webcpp/crypto//openssl` MUST link one copy of libcrypto.

#### Scenario: OpenSSL on Emscripten
- **WHEN** a Jamfile names `/webcpp/hash//openssl` and is built with `toolset=emscripten`
- **THEN** the build stops with "webcpp::hash: OpenSSL is not built for Emscripten. In the program's Jamfile, remove /webcpp/hash//openssl: hkdf and pbkdf2 then run on backends::hash2."

### Requirement: Build: a library named and not called adds no byte
A program that names `/third_party//xkcp`, `/third_party//blake3`, `/third_party//sodium` or `/third_party//bearssl` without calling it SHALL be byte-identical to the same program without it, on every target, proved by `webcpp.same-program`. On Emscripten, naming `/third_party//sodium` changes the program by its `-sALLOW_MEMORY_GROWTH=1` alone: the proof there MUST give the program without libsodium that flag explicitly and require the two to be identical.

#### Scenario: libsodium named, not called
- **WHEN** `webcpp.same-program` builds a program that hashes with `sha2_256`, once with `/third_party//sodium` and once without, on native, wasip2 and wasip3
- **THEN** the two outputs are identical byte for byte

#### Scenario: libsodium named, not called, on Emscripten
- **WHEN** `unused_sodium_emscripten` builds the same program once with `/third_party//sodium` and once without it but with `<linkflags>-sALLOW_MEMORY_GROWTH=1`
- **THEN** the two outputs are identical byte for byte, so the flag is the only difference

### Requirement: Tests: agreement between backends and an outside oracle
`agreement_hkdf_test` and `agreement_pbkdf2_test` SHALL compare, on 1,000 random cases from a fixed, printed seed (empty salt, IKM, info and password always included), every backend with every other natively on every hmac both have, `backends::hash2`, `sodium`, `bearssl` and `openssl` on wasip2 and wasip3, and `backends::hash2`, `sodium` and `bearssl` on Emscripten; `webcrypto_oracle_test` SHALL compare HKDF and PBKDF2 over SHA-1, SHA-256, SHA-384 and SHA-512 with node's `crypto.subtle.deriveBits` on Emscripten.

#### Scenario: WebCrypto agrees
- **WHEN** `webcrypto_oracle_test` runs under node with 200 random cases
- **THEN** every output of `backends::hash2`, `sodium` and `bearssl` equals WebCrypto's

### Requirement: Tests: the C libraries' own suites on webcpp's build
The own lane `c-suites` SHALL run, on every target, libsodium's `test/default` programs with their `.exp` files, every test on every target, each run in a writable directory of the build where `cmptest.h` writes its `.res` file, XKCP's `tests/UnitTests` with webcpp's `config.h` and `--SnP`, and BLAKE3's `c/main.c`, unmodified, with `BLAKE3_TESTING` over every case of `test_vectors.json` through the lane's programs `blake3_c_suite` and `blake3_c_suite_corrupted` of `libs/hash/test/c_suites/` (which replaced the plan's `tools/test/blake3_c_suite.py`), on webcpp's builds of the three libraries.

#### Scenario: libsodium's HKDF and pwhash tests
- **WHEN** the lane runs on wasip2
- **THEN** `kdf_hkdf`, `pwhash_argon2id` and `pwhash_scrypt_ll` are among the tests run, and every test run passes

#### Scenario: BLAKE3's own program, and a difference caught
- **WHEN** the lane runs `blake3_c_suite` and `blake3_c_suite_corrupted` on native, wasip2, wasip3 and Emscripten
- **THEN** `blake3_c_suite` matches every line `c/main.c` prints for every case (no argument, `--keyed`, `--derive-key`, each with `--length`), and `blake3_c_suite_corrupted`, whose one expected line is changed, passes only when exactly that line is reported

### Requirement: Tests: every target, exceptions on and off, every message
Every test program SHALL be declared through the test Jamfile's rule `hash-run`, twice (the second `<exception-handling>off`, suffix `_no_exceptions`), on every target its libraries have; every text of spec 3.3 and 5.7 MUST have a test in a directory of its own (`webcpp.compile-diagnostic` on clang, a fixture of `tools/test/hash_jamfiles_test.py` for Emscripten, `webcpp.link-fail` for each canary); the lanes native, wasip2, wasip3, Emscripten and gcc-15 MUST pass from scratch locally, and MSVC and Apple clang in the CI at the end.

#### Scenario: A lane from scratch
- **WHEN** `$B2 -a --build-dir=bin/lane-wasip3 toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh libs/hash/test libs/hash/example` runs
- **THEN** every test and its `_no_exceptions` twin passes

### Requirement: Documentation
The library's page SHALL open the chapter of each namespace (digests, MACs, key derivations, password hashing, the non-cryptographic hashes) with two comparison tables generated by `tools/hash/doc_tables.py` from `doc/capabilities.json` and the measurements, "What each of X is" and "How to choose among X", each led by the name and each of fewer than eight columns, so that no comparison is a matrix that scrolls in its box; it SHALL give one short section per algorithm (what it is, an example that is a program of `libs/hash/example/` built on every target, its pitfalls), in the manner of noble's README without its text; per backend its Jamfile line, its column of the capability table rendered from `doc/capabilities.json`, what it allocates and every message with its fix; the default backend, `backends::hash2` (`backends::sodium` for Argon2id and scrypt), and how to name another backend in the type, in the chapter `choosing_backend.adoc`, and the measured table as a reference dated 2026-11-16 that decides nothing; the departures with their conditions; and teaching chapters by use case (peer-to-peer networks, cryptocurrencies, privacy), framed as raising security, with webcpp's own code as examples. Text adapted from libsodium's documentation (ISC, `jedisct1/libsodium-doc` at `6d1ea27`) MUST say so and `NOTICE.md` MUST keep its copyright notice and licence; Wikipedia MUST only be cited, never copied.

#### Scenario: An agent writes the Jamfile
- **WHEN** an agent reads the page's backend table to derive PBKDF2-HMAC-SHA-512 in a program for Emscripten
- **THEN** it writes `exe app : app.cpp /webcpp/hash//hash ;` and `pbkdf2<hmac_sha2_512>`, which builds and runs

#### Scenario: No comparison is a matrix
- **WHEN** `hash_doc_tables_test` generates every chapter's comparison, with the measurements and without them
- **THEN** each chapter has two tables led by `Name`, each of fewer than eight columns, the second with the measured columns only when there are measurements

### Requirement: Increments by value, and a capability table
The library SHALL be built in the seven increments of spec 1.5, each usable alone and merged before the next (increment 6 provisions Botan itself, design D43): 1 (Hash2, `keccak_256`, `blake3`, HKDF, PBKDF2, Argon2id, scrypt, the default backend, the measured table as dated reference), 2 (libsodium's whole hashing surface), 3 (XKCP's Keccak family), 4 (OpenSSL's), 5 (BearSSL's), 6 (Botan 3's, provisioned by the increment), 7 (the browser backend). `doc/capabilities.json` (layout version 5) MUST hold spec 3.5's table and, for each function row, its `"default"` backend (`hash2`, `xkcp`, `blake3`, `sodium`, or `null` for a type and for a tag with no default), and a test MUST compare it with `backend_has_v` for every cell and with `default_backend` for every default, and each increment's first task MUST check its column against the library at its pin.

#### Scenario: The table and the code agree
- **WHEN** `capabilities_test` reads `doc/capabilities.json` and evaluates `backend_has_v` for every function and backend
- **THEN** every cell and every default agrees, and a cell changed in one place alone fails the test

### Requirement: Families for every backend's hashes, MACs and KDFs
From increment 2 on, `digest_of<Digest, Backend>` (named so beside Boost.Hash2's `digest<N>`), `hasher<Digest, Backend>` (`make`, `update`, `finish`, move-only, wiped, `invalid_state` after the finalisation), `mac<Mac, Backend>` and `argon2<Variant, Backend>` with `argon2_parameters` (passes, memory in KiB, lanes, secret, associated data) SHALL reach every backend's implementation over the tags of `digests`, `macs` and `kdfs`, with `digest_options` and `mac_options` per tag, the guards in one place and engines per backend; a parameter a backend does not take MUST return `error::unsupported_by_backend`. The Hash2-form types stay Hash2's own, `keccak_256` and `blake3`.

#### Scenario: Keccak-256 three ways
- **WHEN** a native program computes Keccak-256 of "abc" with `keccak_256`, `digest_of<digests::keccak_256, backends::xkcp>` and `digest_of<digests::keccak_256, backends::openssl>`
- **THEN** the three digests are `4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45`

#### Scenario: Argon2 lanes on libsodium
- **WHEN** `argon2<kdfs::argon2id, backends::sodium>` is called with `lanes = 4`
- **THEN** it returns `unsupported_by_backend`, and the same call on `backends::openssl` returns RFC 9106 section 5.3's tag when given its inputs

### Requirement: libsodium's hashing surface (increment 2)
`backends::sodium` SHALL provide BLAKE2b through `crypto_generichash_blake2b_salt_personal` with RFC 7693's guards (key 0 to 64 bytes, salt and personalisation empty or of 16 bytes, output 1 to 64 bytes, the output's size an option), SipHash-2-4 and SipHash-2-4x (`crypto_shorthash_siphash24`, `_siphashx24`), BLAKE2b subkeys (`crypto_kdf_blake2b_derive_from_key`), Argon2i (`crypto_pwhash_argon2i`), SHAKE128/256 and TurboSHAKE128/256 (`crypto_xof_*`), SHA-256, SHA-512, SHA3-256, SHA3-512 and HMAC-SHA-256/512 as digests and MACs (no HMAC-SHA-512/256, which libsodium computes as HMAC-SHA-512 cut to 32 bytes), each checked against its official vectors and libsodium's own tests, and against every other backend that has it.

#### Scenario: BLAKE2b with personalisation
- **WHEN** `digest_of<digests::blake2b, backends::sodium>` runs the BLAKE2 reference's keyed vectors (`blake2b-kat.txt`)
- **THEN** every output matches

### Requirement: XKCP's Keccak family (increment 3)
`backends::xkcp` SHALL provide Keccak-224/256/384/512, SHA3-224/256/384/512, SHAKE128/256, cSHAKE128/256, KMAC128/256 and their XOF forms, TupleHash and ParallelHash (SP 800-185), KangarooTwelve (KT128, KT256) and TurboSHAKE128/256 from XKCP's high-level files at the pin, each against XKCP's own test vectors and NIST's SP 800-185 examples, and agreeing with Hash2, libsodium and OpenSSL where they have the function.

#### Scenario: SP 800-185's KMAC samples
- **WHEN** `mac<macs::kmac_128, backends::xkcp>` runs NIST's KMAC samples
- **THEN** every output matches

### Requirement: OpenSSL's, BearSSL's and Botan's surfaces (increments 4 to 6)
`backends::openssl` SHALL provide every EVP digest of the default provider (KECCAK-224 to -512 included), KMAC, BLAKE2 MACs, scrypt, Argon2d/i/id with every RFC 9106 parameter, KBKDF, SSKDF and the X9.63 KDF on native, wasip2 and wasip3; `backends::bearssl` its MD5, SHA-1, SHA-224/256/384/512, MD5-SHA1, SHAKE and HMAC; `backends::botan` (native and Emscripten, exceptions only) the functions of spec 3.5's Botan column, as read at Botan 3.14.1 (design D44). Each MUST pass the official vectors of every function and agree with every other backend that has it.

#### Scenario: RFC 9106 through the public interface
- **WHEN** `argon2<kdfs::argon2id, backends::openssl>` runs RFC 9106 section 5.3's case
- **THEN** the tag is the RFC's on native, wasip2 and wasip3

### Requirement: The browser backend through a parallel asynchronous family (increment 7)
`<webcpp/hash/async.hpp>` SHALL declare `async_digest`, `async_hmac`, `async_hkdf` and `async_pbkdf2`, each an Asio asynchronous operation in webcpp::asio's model (initiated through `async_initiate`, completion `void(boost::system::error_code)`, any completion token: callback, `use_awaitable`, Cobalt's, `use_future`), whose `default_async_backend<X>` is `default_backend<X>` on every target (design D35): on a synchronous backend the cheap functions MUST run it and complete through `asio::post`, never inside the initiating call. `backends::webcrypto`, named in the type on Emscripten, serves SHA-1/256/384/512, HMAC, HKDF and PBKDF2 over them, awaiting WebCrypto's promise through `webcpp::asio::browser::async_await`, with an executor whose context is an `io_context` (`invalid_parameters` otherwise) and WebCrypto's limits (spec 11.2); off Emscripten naming it MUST fail to compile. The synchronous functions MUST never use `backends::webcrypto`; naming it in one MUST fail to compile with spec 11.2's text, on every target.

#### Scenario: The same call, three tokens
- **WHEN** an Emscripten test under node calls `async_hkdf<hmac_sha2_256, backends::webcrypto>` with an `io_context`'s executor and RFC 5869 A.1's inputs once with a callback, once with `use_awaitable` and once with `use_future`
- **THEN** each completes with no error and A.1's OKM, through WebCrypto

#### Scenario: The default is the synchronous one
- **WHEN** `capabilities_test` compares `default_async_backend<X>` with `default_backend<X>` for every tag, on Emscripten too
- **THEN** they are the same type, and `async_hkdf<hmac_sha2_256>` runs on `backends::hash2`, completing through a post

#### Scenario: Cheap functions only where the browser serves them
- **WHEN** a program looks for an `async_` variant of a cheap function WebCrypto lacks, as `async_digest<digests::blake2b>`
- **THEN** it does not compile, and `async_hkdf<hmac_sha2_256>` does, on every target

#### Scenario: Never inline elsewhere
- **WHEN** a native test calls `async_pbkdf2<hmac_sha2_512>` with a callback that records whether the initiating call has returned
- **THEN** the callback runs after the initiating call returned, from the executor, with RFC-correct output

### Requirement: Heavy functions run on another thread, only in a build with threads
`config.hpp` SHALL define `WEBCPP_HASH_HAS_THREADS` natively when `BOOST_HAS_THREADS` is defined (every native build, b2's `threading=single` included) and on Emscripten when `__EMSCRIPTEN_PTHREADS__` is defined, never on WASI, unless `WEBCPP_HASH_NO_THREADS` is defined. With it, `async_argon2id`, `async_argon2`, `async_scrypt` and `async_pbkdf2` (with a synchronous backend), and every later deliberately slow function's `async_` variant, MUST run the work on the executor the caller gives (natively a `webcpp::asio::thread_pool`'s; on Emscripten a pool of Web Workers) and complete through the completion token on the handler's associated executor. Without it, a call of any of them MUST fail to compile with "webcpp::hash: <name> runs on another thread and needs a build with threads (natively, remove WEBCPP_HASH_NO_THREADS; on Emscripten, link with -pthread). Without threads, call <synchronous name>.", except `async_pbkdf2` on `backends::webcrypto`, which needs no thread. The operation's state MUST come from the handler's associated allocator and be given back before the handler runs; a cancellation before the work starts MUST complete at once with `operation_aborted`.

#### Scenario: Argon2id off the caller's thread
- **WHEN** a native test built with `threading=multi` calls `async_argon2id` with a `thread_pool`'s executor, the interactive costs and a callback bound to its own `io_context`, and runs a 10 ms timer on that `io_context`
- **THEN** the work runs on a pool thread, the timer fires before the derivation completes, and the result equals `argon2id`'s

#### Scenario: Without threads
- **WHEN** a native program built with `WEBCPP_HASH_NO_THREADS`, or a wasip2 program, calls `async_scrypt`
- **THEN** compilation fails with "webcpp::hash: async_scrypt runs on another thread and needs a build with threads (natively, remove WEBCPP_HASH_NO_THREADS; on Emscripten, link with -pthread). Without threads, call scrypt."

#### Scenario: A page that never blocks
- **WHEN** an Emscripten program built with `-pthread` calls `async_argon2id` with a pool of Web Workers under node
- **THEN** the derivation runs on a worker, the main thread's timer keeps firing, and the completion reaches the main thread

### Requirement: A family's default is the first of hash2, xkcp, blake3 and sodium that offers the tag
The default backend of a tag of `digests`, `macs` or `kdfs` that Boost.Hash2 does not have whole SHALL be the first of `backends::hash2`, `backends::xkcp`, `backends::blake3` and `backends::sodium`, in that order, whose engine serves the family's whole contract for the tag (every option and output size the family's guards accept); if none does, the first of them with a partial cell; if none has a cell, `detail::no_default`, whose engines MUST refuse at compile time with "webcpp::hash: <qualified tag> has no default backend. Name one of: <the backends of its row>." (design D34, the owner's decision of 2026-11-17). `backends::bearssl`, `backends::openssl`, `backends::botan` and `backends::webcrypto` MUST never be a default. The defaults are written once and MUST NOT change between increments; no measurement SHALL decide them.

#### Scenario: SHAKE defaults to Boost.Hash2
- **WHEN** a program calls `digest_of<digests::shake_256>` with an output of 200 bytes and every library linked
- **THEN** it runs on `backends::hash2`, whose SHAKE is a whole extendable output (design D37), and the output is the first 200 bytes of SHAKE256's stream, as every other backend that has SHAKE gives them

#### Scenario: A tag only OpenSSL has
- **WHEN** a native program that links OpenSSL calls `argon2<kdfs::argon2d>` without naming a backend
- **THEN** compilation fails with "webcpp::hash: kdfs::argon2d has no default backend. Name one of: backends::openssl.", and `argon2<kdfs::argon2d, backends::openssl>` compiles

#### Scenario: The order is checked
- **WHEN** `capabilities_test` reads each family row's `"default"` in `doc/capabilities.json`
- **THEN** each is `null` or one of hash2, xkcp, blake3 and sodium, equals `default_backend`, and no backend before it in that order has the tag whole

### Requirement: Measured tables come from release builds only
Every measured table SHALL be built only from release builds (design D36, the owner's decision of 2026-11-17): `doc/performance.adoc`'s times through `bench/collect.py`, and the allocation counts of `doc/measurements.allocation.json` and `doc/allocation.adoc` through `bench/allocation_collect.py` from `bench/Jamfile`'s `allocation_release` and `allocation_openssl_release` at `variant=release`. Each collector MUST read the variant from b2's build directory, record it, and refuse data that is not from a release build, its `--check` included; the lanes' allocation tests MUST keep their variant and never be collected.

#### Scenario: A debug build offered to the collector
- **WHEN** `bench/allocation_collect.py` is given the output of `allocation_test` from a lane's debug build directory
- **THEN** it exits with an error naming that output, whose build is not `variant=release` or does not say, and writes nothing

