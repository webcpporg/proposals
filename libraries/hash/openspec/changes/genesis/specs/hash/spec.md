## Purpose

webcpp::hash is a header-only C++20 library that gives a program one entry point for hashing and
key derivation on native, wasip2, wasip3 and Emscripten: Boost.Hash2's algorithms and HMAC by
alias, Keccak-256 and BLAKE3 in Hash2's HashAlgorithm form over XKCP and BLAKE3's C
implementation, HKDF and PBKDF2 over backends (webcpp's own over Hash2's hmac, libsodium,
BearSSL, OpenSSL) with a measured default, and Argon2id and scrypt from libsodium. These
requirements are the structured form of `reference/spec.md` and of the build, tests and tools of
`reference/plan.md`; increment 1 is built in the library's repository, not yet published.

## ADDED Requirements

### Requirement: Library: header-only target, names and headers
The library SHALL be header-only C++20 in namespace `webcpp::hash` (with `webcpp::hash::backends`, `webcpp::hash::kdfs` and `webcpp::hash::detail`), with public headers under `<webcpp/hash/...>`, macros prefixed `WEBCPP_HASH_`, include guards `WEBCPP_HASH_<PATH>_HPP` and the error category `webcpp.hash`. A program MUST reach it through the b2 target `/webcpp/hash//hash`, an alias that gives `libs/hash/include`, Boost's headers and `/webcpp/config//config`. Every public header MUST include `<webcpp/hash/config.hpp>` first, which includes `<webcpp/config.hpp>` first.

#### Scenario: A program that only hashes links no C library
- **WHEN** a Jamfile reads `exe app : app.cpp /webcpp/hash//hash ;` and `app.cpp` computes `webcpp::hash::sha2_256` of "abc" and an HKDF on `backends::hash2`
- **THEN** it builds and runs on native, wasip2, wasip3 and Emscripten with no library of `/third_party` named or built

### Requirement: Library: configuration and exceptions
`<webcpp/hash/config.hpp>` SHALL define `WEBCPP_HASH_NO_EXCEPTIONS` exactly when `WEBCPP_HAS_EXCEPTIONS` is not defined, keep it when a developer defines it, define `WEBCPP_HASH_WEBASSEMBLY` on wasip2, wasip3 and Emscripten, declare the backend tags `backends::hash2`, `sodium`, `bearssl` and `openssl` and the public KDF tags `kdfs::hkdf`, `kdfs::hkdf_extract`, `kdfs::hkdf_expand`, `kdfs::pbkdf2`, `kdfs::argon2id` and `kdfs::scrypt`, one per operation, and refuse with `#error` OpenSSL on Emscripten (`/webcpp/hash//openssl` or `WEBCPP_HASH_PROFILE_BACKEND_OPENSSL`) and more than one profile define of one level. The overrides' refusals MUST NOT be in `config.hpp`: they are in `detail/overrides.hpp`, which `kdf.hpp` includes. No function of the library SHALL throw, with exceptions on or off.

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
Every input SHALL be a `std::span<std::uint8_t const>` and every output a `std::span<std::uint8_t>`, sizes `std::size_t`, and a limit wider than `size_t` on wasm32 MUST be compared through `detail::exceeds(std::uint64_t, std::uint64_t)`. No function of webcpp::hash SHALL allocate; the C libraries that allocate inside a function (libsodium's Argon2id and scrypt, OpenSSL's KDFs) are the recorded departures of spec 6.2.

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
- **WHEN** a wasip2 program that names `/third_party//bearssl` and `/third_party//sodium` and gives no profile calls `hkdf_extract<hmac_sha2_512>` and `hkdf<hmac_sha2_512>`
- **THEN** `hkdf` runs on `backends::bearssl`, the first of the size order, and `hkdf_extract` on the first backend of its own order that has the step, never `backends::bearssl`, and both compile

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

### Requirement: The default backend: one profile, per-algorithm overrides
`config.hpp` SHALL read webcpp's umbrella profile (`WEBCPP_PROFILE_FASTEST`, `_SMALLEST`, `_PORTABLE`) and SHALL take at most one library profile define: none (`auto`: natively the fastest, on WebAssembly the smallest), `WEBCPP_HASH_PROFILE_FASTEST`, `WEBCPP_HASH_PROFILE_SMALLEST`, `WEBCPP_HASH_PROFILE_PORTABLE` (the first that has the algorithm of hash2, xkcp, blake3, sodium, bearssl) or `WEBCPP_HASH_PROFILE_BACKEND_<NAME>`, and per-algorithm overrides `WEBCPP_HASH_DEFAULT_BACKEND_<ALGORITHM>_<NAME>`; `default_backend<Algorithm, Hmac>` (and `default_kdf_backend<Hmac, Kdf = kdfs::hkdf>`, the same) MUST be the override's backend, else the profile's choice among the linked backends that have the algorithm, by the build (target, `HAS` macros) and the measured orders of `detail/ranking.hpp`, chosen per operation: the first backend of the order that the program links and that has that operation over that hmac. The precedence MUST be: per-algorithm override, library profile, umbrella profile, the build's measured default. Two profiles of one level (each `#error` naming both defines), two overrides of one algorithm, or OpenSSL selected on Emscripten MUST be an `#error`; on MSVC the profile and each override MUST carry `#pragma detect_mismatch`. The overrides, their refusals and their `detect_mismatch` (`webcpp_hash_default_backend_<algorithm>`) MUST be in `detail/overrides.hpp`, generated by `tools/hash/measure.py --overrides` from `doc/capabilities.json` and included by `kdf.hpp`, the one reader of the overrides; an override of HKDF applies to `hkdf_extract`, `hkdf_expand` and `hkdf`.

#### Scenario: Natively with OpenSSL, auto
- **WHEN** a native program that names `/webcpp/hash//openssl` and gives no profile reads `default_kdf_backend<hmac_ripemd_128>`
- **THEN** it is `backends::hash2`, since OpenSSL lacks RIPEMD-128; and `default_kdf_backend<hmac_sha2_256>` is the faster of OpenSSL and hash2 in the committed native throughput order (hash2 in the provisional order of increment 1)

#### Scenario: On wasip2 with libsodium, for PBKDF2
- **WHEN** a wasip2 program that names `/third_party//sodium` reads `default_kdf_backend<hmac_sha2_256, kdfs::pbkdf2>`
- **THEN** it is a backend that has PBKDF2 (`backends::hash2`), never `backends::sodium`

#### Scenario: One switch for the whole program
- **WHEN** a native program that links libsodium and OpenSSL is built with `WEBCPP_HASH_PROFILE_PORTABLE`
- **THEN** every default is a backend present on all four targets (HKDF over SHA-256 is `hash2` or `sodium` by the portable order, never `openssl`)

#### Scenario: The umbrella and the precedence
- **WHEN** a native program that links libsodium and OpenSSL is built with `WEBCPP_PROFILE_SMALLEST`, `WEBCPP_HASH_PROFILE_PORTABLE` and `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_OPENSSL`
- **THEN** `hkdf<hmac_sha2_256>` runs on OpenSSL (the override), `pbkdf2<hmac_sha2_256>` on `backends::hash2` (the library's portable profile over the umbrella's smallest), and with only `WEBCPP_PROFILE_SMALLEST` both follow the measured size order

#### Scenario: Two profiles at once
- **WHEN** a program defines `WEBCPP_HASH_PROFILE_FASTEST` and `WEBCPP_HASH_PROFILE_SMALLEST`
- **THEN** compilation fails with "webcpp::hash: WEBCPP_HASH_PROFILE_FASTEST and WEBCPP_HASH_PROFILE_SMALLEST are both defined. Keep one."

#### Scenario: An override that cannot serve
- **WHEN** a program defines `WEBCPP_HASH_DEFAULT_BACKEND_PBKDF2_SODIUM` and calls `pbkdf2<hmac_sha2_512>`
- **THEN** compilation fails with spec 3.3's text for PBKDF2 on `backends::sodium`

#### Scenario: Two overrides of one algorithm
- **WHEN** a program defines `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_HASH2` and `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_SODIUM` and includes `<webcpp/hash/hkdf.hpp>`
- **THEN** compilation fails at `detail/overrides.hpp` with "webcpp::hash: WEBCPP_HASH_DEFAULT_BACKEND_HKDF_HASH2 and WEBCPP_HASH_DEFAULT_BACKEND_HKDF_SODIUM are both defined. Keep one.", and a translation unit that includes only `<webcpp/hash/algorithms.hpp>` compiles

#### Scenario: HKDF's override on BearSSL and the two steps
- **WHEN** a program that links BearSSL defines `WEBCPP_HASH_DEFAULT_BACKEND_HKDF_BEARSSL` and calls `hkdf<hmac_sha2_256>` and `hkdf_extract<hmac_sha2_256>`
- **THEN** `hkdf` runs on `backends::bearssl` and `hkdf_extract` fails to compile with spec 3.3's text for the two steps on `backends::bearssl`

### Requirement: The defaults come from webcpp's own measurements, regenerated at every pin move
For every target, every algorithm of the capability table and every backend that has it there, a program under `libs/hash/test/measure/` SHALL be built and its size measured (`variant=release optimization=space`), and natively, and on WebAssembly where it runs within the lane's bound, its throughput (`optimization=speed`, median of five runs); `tools/hash/measure.py` MUST write `doc/measurements.json`, `doc/measurements.adoc` and `include/webcpp/hash/detail/ranking.hpp` (an order by size and an order by throughput per target and operation, `hkdf_extract` and `hkdf_expand` each with its own); the own lane `measure` MUST fail when a regenerated size order differs from the committed one, naming `python3 tools/hash/measure.py --write`. A first measurement MUST order by the median of the invocations' medians alone, with no clause of its own, and a committed throughput order changes only past a 10 percent difference in the median. The data MUST record the machine, whether it was loaded, and the native toolchain's identity (compiler, operating system, processor, strip); a lane whose native toolchain differs MUST build and check every program and skip the native order comparison with one printed line, and WebAssembly orders are always compared. Increment 1's native throughput orders are provisional: measured on one loaded macOS arm64 machine, the native HKDF order rests on medians under 1 percent apart, and they MUST be re-measured on an idle machine before a release.

#### Scenario: A pin move without regeneration
- **WHEN** a pin of libsodium, BearSSL, OpenSSL, XKCP or BLAKE3 moves and a size order changes while `ranking.hpp` is unchanged
- **THEN** the `measure` lane fails and names the command that regenerates the files

#### Scenario: Another native toolchain
- **WHEN** the `measure` lane runs natively with a compiler, operating system or processor other than `doc/measurements.json`'s `native_toolchain`
- **THEN** it builds and checks every program, prints one line saying the native order comparison is skipped, and still compares the WebAssembly orders

#### Scenario: A release after a loaded measurement
- **WHEN** a release of webcpp::hash is prepared while `doc/measurements.json` records `loaded: true`
- **THEN** the throughput is re-measured on an idle machine and the orders regenerated before the release

### Requirement: The throughput of PBKDF2-HMAC-SHA-512
`libs/hash/bench/pbkdf2_sha512.cpp` SHALL measure PBKDF2-HMAC-SHA-512 at 1,000,000 iterations and 64 bytes of output, the median of three runs, for `backends::hash2`, `backends::openssl` (native, wasip2, wasip3) and libsodium's HMAC-SHA-512 in RFC 8018's loop (libsodium has no PBKDF2), and `doc/performance.adoc` MUST record the results with the machine, compiler, target and date.

#### Scenario: The table is filled
- **WHEN** the throughput task is done
- **THEN** `doc/performance.adoc` holds a time for each of the three on native, and for each that exists on wasip2, wasip3 and Emscripten

### Requirement: Argon2id from libsodium
`argon2id<Backend = backends::sodium>(output, password, salt, argon2id_parameters)` SHALL compute Argon2id version 0x13 with one lane, no secret and no associated data through `crypto_pwhash_argon2id`, refusing in order an output shorter than 16 or longer than 4,294,967,295 bytes (`invalid_output_size`), a salt of another size than 16 (`invalid_salt_size`), a password longer than 4,294,967,295 bytes (`input_too_long`), and passes outside 1 to 4,294,967,295 or memory below 8,192, not a multiple of 1,024 or above libsodium's maximum (`invalid_parameters`). `argon2id_interactive`, `argon2id_moderate` and `argon2id_sensitive` MUST equal libsodium's presets, checked by `static_assert`.

#### Scenario: RFC 9106 through libsodium's core
- **WHEN** a test that includes libsodium's private `argon2.h` runs RFC 9106 section 5.3's Argon2id case (t 3, m 32 KiB, p 4, with secret and associated data) through `argon2_ctx`
- **THEN** the tag is the RFC's

#### Scenario: A salt of 15 bytes
- **WHEN** `argon2id` is called with a 15-byte salt
- **THEN** it returns `invalid_salt_size` and `output` is untouched

### Requirement: scrypt from libsodium
`scrypt<Backend = backends::sodium>(output, password, salt, scrypt_parameters{n, r, p})` SHALL compute RFC 7914's scrypt through `crypto_pwhash_scryptsalsa208sha256_ll`, refusing an empty output or one past (2^32 - 1) times 32 bytes (`invalid_output_size`) and N not a power of two greater than 1, N not below 2^(16 r), r or p of 0, r times p not below 2^30, or 128 r (N + p) bytes that do not fit `size_t` (`invalid_parameters`); libsodium's ENOMEM MUST be `errc::not_enough_memory`.

#### Scenario: RFC 7914's vectors
- **WHEN** RFC 7914 section 12's four cases run through `scrypt`
- **THEN** each derived key matches, the 1 GiB case in a program of its own on native and on the WebAssembly targets where the plan measured it runs

### Requirement: Memory-hard functions allocate inside libsodium, a recorded departure
The documentation SHALL state that libsodium allocates Argon2id's and scrypt's memory (`mmap` or `malloc`) outside any allocator of the program's, that this departure from CODING-GUIDELINES.md chapter 4 is the owner's, and that the Argon2 reference implementation's allocation callbacks would route Argon2id's memory through a `std::pmr::memory_resource` as a later `backends::argon2`, not built.

#### Scenario: The departure is on the page
- **WHEN** a reader opens the library's "Allocation" section
- **THEN** it names libsodium's allocation in `argon2id` and `scrypt`, OpenSSL's in its KDFs, and the alternative

### Requirement: Build: the C libraries file by file, with webcpp::hash's canaries
`third_party/Jamfile` SHALL build `/third_party//xkcp` (XKCP at `d20c8fc`, `KeccakP-1600-opt64.c`, webcpp's adapter and `config.h`), `/third_party//blake3` (BLAKE3 1.8.7 at `f3149ec`) and `/third_party//sodium` (libsodium 1.0.22 at `77e1ce5`, every `.c` of `src/libsodium/`, with upstream's `builds/msvc/version.h` copied into the build directory and the defines libsodium's own `configure` finds for each target, `DEV_MODE` off, without assembly) as static libraries from submodules under `third_party/libs/`, never edited, each with webcpp::hash's canary `src/canary_<name>.c`, `-ffunction-sections -fdata-sections`, `<warnings>off`, and usage requirements giving `WEBCPP_HASH_HAS_<NAME>=1` and the removal of unused sections; `/third_party//sodium`'s usage requirements MUST also give `-sALLOW_MEMORY_GROWTH=1` to an Emscripten link, since Argon2id and scrypt allocate the memory their costs name past Emscripten's fixed 16 MiB. `/third_party//bearssl` MUST gain webcpp::hash's canary and `WEBCPP_HASH_HAS_BEARSSL=1`.

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
The library's page SHALL open the chapter of each namespace (digests, MACs, key derivations, password hashing, the non-cryptographic hashes) with two comparison tables generated by `tools/hash/doc_tables.py` from `doc/capabilities.json` and the measurements, "What each of X is" and "How to choose among X", each led by the name and each of fewer than eight columns, so that no comparison is a matrix that scrolls in its box; it SHALL give one short section per algorithm (what it is, an example that is a program of `libs/hash/example/` built on every target, its pitfalls), in the manner of noble's README without its text; per backend its Jamfile line, its column of the capability table rendered from `doc/capabilities.json`, what it allocates and every message with its fix; the profile, the overrides and the measured table; the departures with their conditions; and teaching chapters by use case (peer-to-peer networks, cryptocurrencies, privacy), framed as raising security, with webcpp's own code as examples. Text adapted from libsodium's documentation (ISC, `jedisct1/libsodium-doc` at `6d1ea27`) MUST say so and `NOTICE.md` MUST keep its copyright notice and licence; Wikipedia MUST only be cited, never copied.

#### Scenario: An agent writes the Jamfile
- **WHEN** an agent reads the page's backend table to derive PBKDF2-HMAC-SHA-512 in a program for Emscripten
- **THEN** it writes `exe app : app.cpp /webcpp/hash//hash ;` and `pbkdf2<hmac_sha2_512>`, which builds and runs

#### Scenario: No comparison is a matrix
- **WHEN** `hash_doc_tables_test` generates every chapter's comparison, with the measurements and without them
- **THEN** each chapter has two tables led by `Name`, each of fewer than eight columns, the second with the measured columns only when there are measurements

### Requirement: Increments by value, and a capability table
The library SHALL be built in the seven increments of spec 1.5, each usable alone and merged before the next: 1 (Hash2, `keccak_256`, `blake3`, HKDF, PBKDF2, Argon2id, scrypt, the profile, the measured table), 2 (libsodium's whole hashing surface), 3 (XKCP's Keccak family), 4 (OpenSSL's), 5 (BearSSL's), 6 (Botan 3's, after webcpp::crypto provisions Botan), 7 (the browser backend). `doc/capabilities.json` MUST hold spec 3.5's table, a test MUST compare it with `backend_has_v` for every cell, and each increment's first task MUST check its column against the library at its pin.

#### Scenario: The table and the code agree
- **WHEN** `capabilities_test` reads `doc/capabilities.json` and evaluates `backend_has_v` for every function and backend
- **THEN** every cell agrees, and a cell changed in one place alone fails the test

### Requirement: Families for every backend's hashes, MACs and KDFs
From increment 2 on, `digest<Digest, Backend>`, `hasher<Digest, Backend>` (`make`, `update`, `finish`, move-only, wiped, `invalid_state` after the finalisation), `mac<Mac, Backend>` and `argon2<Variant, Backend>` with `argon2_parameters` (passes, memory in KiB, lanes, secret, associated data) SHALL reach every backend's implementation over the tags of `digests`, `macs` and `kdfs`, with `digest_options` and `mac_options` per tag, the guards in one place and engines per backend; a parameter a backend does not take MUST return `error::unsupported_by_backend`. The Hash2-form types stay Hash2's own, `keccak_256` and `blake3`.

#### Scenario: Keccak-256 three ways
- **WHEN** a native program computes Keccak-256 of "abc" with `keccak_256`, `digest<digests::keccak_256, backends::xkcp>` and `digest<digests::keccak_256, backends::openssl>`
- **THEN** the three digests are `4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45`

#### Scenario: Argon2 lanes on libsodium
- **WHEN** `argon2<kdfs::argon2id, backends::sodium>` is called with `lanes = 4`
- **THEN** it returns `unsupported_by_backend`, and the same call on `backends::openssl` returns RFC 9106 section 5.3's tag when given its inputs

### Requirement: libsodium's hashing surface (increment 2)
`backends::sodium` SHALL provide BLAKE2b through `crypto_generichash_blake2b_salt_personal` (key 0 or 16 to 64 bytes, salt and personalisation of 16 bytes, output 16 to 64 bytes), SipHash-2-4 and SipHash-2-4x (`crypto_shorthash_siphash24`, `_siphashx24`), BLAKE2b subkeys (`crypto_kdf_blake2b_derive_from_key`), Argon2i (`crypto_pwhash_argon2i`), SHAKE128/256 and TurboSHAKE128/256 (`crypto_xof_*`), SHA-256, SHA-512, SHA3-256, SHA3-512 and HMAC-SHA-256/512/512-256 as digests and MACs, each checked against its official vectors and libsodium's own tests, and against every other backend that has it.

#### Scenario: BLAKE2b with personalisation
- **WHEN** `digest<digests::blake2b, backends::sodium>` runs the BLAKE2 reference's keyed vectors (`blake2b-kat.txt`)
- **THEN** every output matches

### Requirement: XKCP's Keccak family (increment 3)
`backends::xkcp` SHALL provide Keccak-224/256/384/512, SHA3-224/256/384/512, SHAKE128/256, cSHAKE128/256, KMAC128/256 and their XOF forms, TupleHash and ParallelHash (SP 800-185), KangarooTwelve (KT128, KT256) and TurboSHAKE128/256 from XKCP's high-level files at the pin, each against XKCP's own test vectors and NIST's SP 800-185 examples, and agreeing with Hash2, libsodium and OpenSSL where they have the function.

#### Scenario: SP 800-185's KMAC samples
- **WHEN** `mac<macs::kmac_128, backends::xkcp>` runs NIST's KMAC samples
- **THEN** every output matches

### Requirement: OpenSSL's, BearSSL's and Botan's surfaces (increments 4 to 6)
`backends::openssl` SHALL provide every EVP digest of the default provider (KECCAK-224 to -512 included), KMAC, BLAKE2 MACs, scrypt, Argon2d/i/id with every RFC 9106 parameter, KBKDF, SSKDF and the X9.63 KDF on native, wasip2 and wasip3; `backends::bearssl` its MD5, SHA-1, SHA-224/256/384/512, MD5-SHA1, SHAKE and HMAC; `backends::botan` (native, exceptions only, after webcpp::crypto provisions Botan) the functions of spec 3.5's Botan column that its first task confirms at Botan's pin. Each MUST pass the official vectors of every function and agree with every other backend that has it.

#### Scenario: RFC 9106 through the public interface
- **WHEN** `argon2<kdfs::argon2id, backends::openssl>` runs RFC 9106 section 5.3's case
- **THEN** the tag is the RFC's on native, wasip2 and wasip3

### Requirement: The browser backend through a parallel asynchronous family (increment 7)
`<webcpp/hash/async.hpp>` SHALL declare `async_digest`, `async_hmac`, `async_hkdf` and `async_pbkdf2`, each an Asio asynchronous operation in webcpp::asio's model (initiated through `async_initiate`, completion `void(boost::system::error_code)`, any completion token: callback, `use_awaitable`, Cobalt's, `use_future`), whose `default_async_backend` is `backends::webcrypto` on Emscripten for SHA-1/256/384/512, HMAC, HKDF and PBKDF2 over them, awaiting WebCrypto's promise through `webcpp::asio::browser::async_await`; off Emscripten, or with a synchronous backend named, the operation MUST run that backend and complete through `asio::post`, never inside the initiating call. The synchronous functions MUST never use `backends::webcrypto`; naming it in one MUST fail to compile with spec 11.2's text.

#### Scenario: The same call, three tokens
- **WHEN** an Emscripten test under node calls `async_hkdf<hmac_sha2_256>` with RFC 5869 A.1's inputs once with a callback, once with `use_awaitable` and once with `use_future`
- **THEN** each completes with no error and A.1's OKM, through WebCrypto

#### Scenario: Cheap functions only where the browser serves them
- **WHEN** a program looks for an `async_` variant of a cheap function WebCrypto lacks, as `async_digest<digests::blake2b>`
- **THEN** it does not compile, and `async_hkdf<hmac_sha2_256>` does, on every target

#### Scenario: Never inline elsewhere
- **WHEN** a native test calls `async_pbkdf2<hmac_sha2_512>` with a callback that records whether the initiating call has returned
- **THEN** the callback runs after the initiating call returned, from the executor, with RFC-correct output

### Requirement: Heavy functions run on another thread, only in a build with threads
`config.hpp` SHALL define `WEBCPP_HASH_HAS_THREADS` natively when `BOOST_HAS_THREADS` is defined and on Emscripten when `__EMSCRIPTEN_PTHREADS__` is defined, never on WASI, unless `WEBCPP_HASH_NO_THREADS` is defined. With it, `async_argon2id`, `async_argon2`, `async_scrypt` and `async_pbkdf2` (with a synchronous backend), and every later deliberately slow function's `async_` variant, MUST run the work on the executor the caller gives (natively a `webcpp::asio::thread_pool`'s; on Emscripten a pool of Web Workers) and complete through the completion token on the handler's associated executor. Without it, a call of any of them MUST fail to compile with "webcpp::hash: <name> runs on another thread and needs a build with threads (natively b2 threading=multi; on Emscripten -pthread). Without threads, call <synchronous name>.", except `async_pbkdf2` on `backends::webcrypto`, which needs no thread.

#### Scenario: Argon2id off the caller's thread
- **WHEN** a native test built with `threading=multi` calls `async_argon2id` with a `thread_pool`'s executor, the interactive costs and a callback bound to its own `io_context`, and runs a 10 ms timer on that `io_context`
- **THEN** the work runs on a pool thread, the timer fires before the derivation completes, and the result equals `argon2id`'s

#### Scenario: Without threads
- **WHEN** a program built with b2's default `threading=single` calls `async_scrypt`
- **THEN** compilation fails with "webcpp::hash: async_scrypt runs on another thread and needs a build with threads (natively b2 threading=multi; on Emscripten -pthread). Without threads, call scrypt."

#### Scenario: A page that never blocks
- **WHEN** an Emscripten program built with `-pthread` calls `async_argon2id` with a pool of Web Workers under node
- **THEN** the derivation runs on a worker, the main thread's timer keeps firing, and the completion reaches the main thread

