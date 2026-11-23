# hkdf-pieces Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** webcpp::hash gains HKDF over input pieces (`byte_pieces`, `{a, b, c}` at the call) with
no cap and no heap on every backend that has the HMAC and in the asynchronous family, and its
public names are spelled out.

**Architecture:** A view type `byte_pieces` (B2: `std::span` does not take `{a, b, c}` in C++20,
probed with clang 21 and gcc 15) feeds each piece to the backend's streaming HMAC; a new engine
`detail::hkdf_pieces_engine<Backend, Hmac>` per backend, three new `kdfs::` tags with their own
capability rows, three asynchronous functions, and WebCrypto over `subtle.sign("HMAC", ...)`.
The renames come first, so that every later task writes the new names.

**Tech Stack:** C++20 header-only, b2, Boost.Hash2 1.92, libsodium 1.0.22, BearSSL, OpenSSL
3.5.9, Botan 3.14.1, webcpp::asio, Emscripten (node's WebCrypto), Python tools of the
superproject.

**Spec:** `libraries/hash/openspec/changes/hkdf-pieces/` of webcpporg/proposals: `proposal.md`,
`design.md` (D1 to D9, B1 to B6, the owner's decisions) and `specs/hash/spec.md`. Read both
design.md and spec.md before any task.

**Trees:** the superproject worktree on branch `hash/hkdf-pieces` (its `libs/hash` on branch
`hash/hkdf-pieces` of webcpporg/hash). Every command runs from the superproject's root with
`CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` unset; `B2` below means
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.
Commits in `libs/hash` go to the hash branch; commits of superproject files (`tools/`) to the
superproject branch. The superproject's gitlink is moved by the controller at the end, not by a
task.

## Global Constraints

- Everything written to files is in English. Never the em dash character (U+2014); write "-".
- Commits carry no trailer at all: no `Co-Authored-By`, no "Generated with". One subject line in
  the repository's style (`hash: ...`), optionally a body.
- Never hand-edit a generated file: `doc/generated/*`, `doc/backends.adoc`, `doc/measurements.adoc`,
  `doc/llms.txt` are written by `python3 tools/hash/doc_tables.py --write`; check with `--check`.
  `doc/changelog.adoc` is the changelog's source and is written by hand (AGENTS.md chapter 5:
  at most ten lines per version, past-tense verb, `*Breaking change:*` prefix where it applies).
- Public names are never abbreviated (an algorithm's proper name, as `hkdf`, `hmac`, `sha256`,
  excepted): `input_key_material`, `pseudorandom_key`, `application_info`,
  `output_key_material`, `salt`; `hkdf_max_application_info_size`; `scrypt_parameters{cost,
  block_size, parallelism}`; `argon2_parameters::memory_kibibytes`. Kept: `password`, `key`,
  `secret`, `label`, `context`, `customisation`, `personalisation`, `seed_value`, `master_key`,
  `subkey_id`, `fixed_info`, `shared_info`, `xof`, `counter_bits`, and the `_bytes` suffix of
  pbkdf2, scrypt, argon2id's parameters (B3). New code in `detail` follows the rule too.
- Every function that computes on a key or a key derivation's secret holds
  `detail::timing_guard const timing;` as its first statement (libs/hash/AGENTS.md).
- The pieces forms refuse only RFC 5869's: an output of 0 bytes or more than `255 * HashLen`
  (`invalid_output_size`), and for `hkdf_expand` a pseudorandom key of another size than
  `HashLen` (`invalid_key_size`); then the backend's HMAC key limits as `mac` reads them
  (`backend_limits`' MAC members, e.g. OpenSSL's `INT_MAX` key). No limit on the input keying
  material, the application info or the number of pieces. A guard's refusal leaves the output
  untouched; a backend's failure zeroes it.
- hash's code allocates nothing in the pieces forms (no `operator new`, no C allocator) on
  hash2, sodium and bearssl; OpenSSL's and Botan's allocations are theirs, counted and printed.
- Every HMAC state, every `T(i)` and the PRK between Extract and Expand are wiped
  (`detail::wipe_object` / `detail::wipe`, or the library's cleanse) on success and on failure.
- A piece longer than the backend's update length type is fed in slices that type holds.
- Lint warnings and flaky tests, even pre-existing, are defects to fix in the task that meets
  them. Measurements (allocation.adoc's tables) come from release builds only
  (`variant=release`, `allocation_release`), never debug.
- Tests go through `hash-run` in `libs/hash/test/Jamfile` (with and without exceptions).
- **The Linux lint, every task (owner decision, 2026-11-23):** before reporting DONE, every
  task runs the Linux lint on every file it touched, through the heavy-lane queue:
  `<heavy.sh> hkdf-pieces <linux-lint>/run.sh --files <every touched file> <superproject>`
  (the controller gives the absolute paths), and fixes every finding in the task's own commits,
  all findings of one run in one commit, then one more run to confirm; never one run per
  one-line fix. A task reaches review lint-clean. Every task's Step 4 includes this run.
- A single test target while coding is fine directly; full lanes (`b2 -a` over a whole library's
  test and example directories), the lint and MSVC are run by the controller, not by a task.

## Review Focus

1. **A named `byte_pieces` variable dangles.** `byte_pieces parts{a, b}; hkdf(..., parts, ...)`
   keeps a pointer into an `initializer_list`'s array that died at the end of the declaration.
   The constructor from `std::initializer_list` is `[[clang::lifetimebound]]` where the compiler
   has the attribute (`WEBCPP_HASH_LIFETIMEBOUND`), the documentation says to pass a braced list
   at the call or to name a `std::array` of spans, and Task 2's test passes a `std::array` and a
   `std::vector` of spans through the range constructor.
2. **A deferred completion token outlives the temporary list.** `async_hkdf(ex, out, salt,
   {a, b}, {c, d}, asio::deferred)` launched later must not read the dead list: the pieces
   overloads read the pieces before `async_initiate` (on a synchronous backend they run the
   synchronous function in the call itself and post only the code; on WebCrypto they gather into
   JavaScript memory in the call). Task 5's test uses `asio::deferred` and `use_awaitable` with
   lists and pieces overwritten before the operation runs.
3. **`{x}` with a lone span picks the single-buffer form** (D1, the language's rule: a braced
   list of one element of the parameter's own type is that element). A caller who needs the
   uncapped form with one piece writes `byte_pieces{x}` (Ruling R1). Task 2's test pins both:
   `hkdf_expand(out, prk, {info})` with 32,769 bytes returns `input_too_long`, and
   `hkdf_expand(out, prk, byte_pieces{info})` succeeds.
4. **A piece above 4 GiB or above `INT_MAX`** on a backend whose update length is narrower
   (BearSSL and libsodium take `size_t`/`unsigned long long`, but a wasm32 `size_t` is 32 bits;
   OpenSSL's `EVP_MAC_update` takes `size_t`): the engines slice by the backend's length type;
   each engine's test feeds a piece through a slicing helper with the slice size lowered by a
   test seam (`detail::hkdf_pieces_slice<Backend>` as a variable template the test can read; the
   test checks a 1 MiB piece equals hash2's result, and the slicing code is unit-tested with a
   small maximum).
5. **An empty piece list and empty pieces** on every backend, including WebCrypto, whose HMAC
   `importKey` refuses an empty key: an empty salt becomes `HashLen` zero bytes (D2), and an
   empty `sign` input is valid. Every engine test runs `{}`, `{empty}`, `{empty, a, empty}`.

---

### Task 1: The names

**Files:**
- Modify: `libs/hash/include/webcpp/hash/hkdf.hpp`, `async.hpp`, `scrypt.hpp`, `argon2.hpp`,
  `detail/engines.hpp`, `backends/{hash2,sodium,bearssl,openssl,botan,webcrypto}.hpp` (their HKDF
  engines' parameters, `backend_limits` members, their scrypt/argon2 engines' field reads)
- Modify: every test, example and page of `libs/hash` that spells a renamed name (a grep for
  `memory_kib`, `hkdf_max_info_size`, `\.n *=`, `\.r *=`, `\.p *=`, `\bikm\b`, `\bprk\b`,
  `ikm_bytes`, `prk_bytes`, `info_bytes`, `hkdf_ikm`, `hkdf_prk`, `hkdf_info`, `max_memory_kib`
  finds them), `libs/hash/doc/capabilities.json` and `guide.json` notes,
  `libs/hash/test/messages/*` (their sources and expected texts)
- Modify (superproject): `tools/test/fixtures/hash_jamfiles/*/app.cpp` that spell
  `memory_kib`, `.n`, `.r`, `.p`
- Create: `libs/hash/test/names_test.cpp` and its `hash-run names` line in `test/Jamfile`

**Interfaces:**
- Produces: the D7 names, used by every later task:
  - `hkdf_extract<Hmac, Backend>(std::span<std::uint8_t const> salt, std::span<std::uint8_t const> input_key_material)`
  - `hkdf_expand<Hmac, Backend>(std::span<std::uint8_t> output_key_material, std::span<std::uint8_t const> pseudorandom_key, std::span<std::uint8_t const> application_info)`
  - `hkdf<Hmac, Backend>(output_key_material, salt, input_key_material, application_info)`
  - `async_hkdf<Hmac, Backend>(executor, output_key_material, salt, input_key_material, application_info, token)`
  - `inline constexpr std::size_t hkdf_max_application_info_size = 32768;`
  - `detail::hkdf_guards<Hmac, Backend>(output_size, salt_size, input_key_material_size, application_info_size)`
  - `backend_limits<B>::hkdf_salt`, `hkdf_input_key_material`, `hkdf_pseudorandom_key`,
    `hkdf_application_info`, `hkdf_output` (renamed from `hkdf_ikm`, `hkdf_prk`, `hkdf_info`)
  - `scrypt_parameters{std::uint64_t cost; std::uint32_t block_size; std::uint32_t parallelism;}`,
    each comment keeping RFC 7914's symbol (`N`, `r`, `p`)
  - `argon2_parameters::memory_kibibytes` (comment keeps RFC 9106's `m`), and the `detail` names
    that mirror it (`argon2_min_memory_kibibytes_per_lane`, `max_memory_kibibytes`)

- [ ] **Step 1: Write the failing test `names_test.cpp`**

```cpp
// The spelled-out names compile (design D7): designated initializers of every renamed field, and
// the renamed constant.
static_assert(webcpp::hash::hkdf_max_application_info_size == 32768);
int main() {
    std::array<std::uint8_t, 64> output{};
    auto const password = bytes("password");
    auto const salt = bytes("NaCl");
    // RFC 7914, section 12, the second vector: N = 1024, r = 8, p = 16.
    auto done = webcpp::hash::scrypt(output, password, salt,
                                     {.cost = 1024, .block_size = 8, .parallelism = 16});
    BOOST_TEST(done.has_value());
    BOOST_TEST_EQ(hex(output), "fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b373162"
                               "2eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640");
    webcpp::hash::argon2_parameters const parameters{.passes = 3, .memory_kibibytes = 32,
                                                      .lanes = 4};
    BOOST_TEST_EQ(parameters.memory_kibibytes, 32U);
    return boost::report_errors();
}
```
(Use the test's usual support headers, `support/hex.hpp`; the scrypt call names its default
backend as `scrypt_test.cpp` does and links `/third_party//sodium` in the Jamfile line. Check the
field order of `argon2_parameters` in `argon2.hpp` and write the designated initializers in it.)

- [ ] **Step 2: Run it, expect a compile failure** (`.cost` is not a member)

Run: `B2 toolset=clang libs/hash/test//names`
Expected: error naming `cost`.

- [ ] **Step 3: Rename** every name in the Interfaces block, the parameter documentation
  (`@param`) with it, the `detail` names that mirror them, and the call sites in `libs/hash`
  (include, test, example, doc, capabilities.json, guide.json) and the superproject's fixtures.
  Public parameter names of HKDF become D7's; `salt_bytes` becomes `salt` in HKDF only (B3).
  Read every public parameter of hash's headers once for any other abbreviation D7 missed; record
  each one found, renamed or kept with its reason, in the report. Then run
  `python3 tools/hash/doc_tables.py --write` and commit the generated files it changes.

- [ ] **Step 4: Run the touched tests and the tools' tests**

Run: `B2 toolset=clang libs/hash/test//names libs/hash/test//scrypt libs/hash/test//argon2id libs/hash/test//hkdf_hash2 libs/hash/test//hkdf_guards libs/hash/test//capabilities libs/hash/example`
then `python3 tools/hash/doc_tables.py --check`,
`WEBCPP_USER_CONFIG=.local/user-config.jam python3 tools/test/hash_jamfiles_test.py`,
`python3 tools/test/hash_doc_tables_test.py`, and the message tests (`libs/hash/test/messages`).
Expected: all pass; `grep -rnE "memory_kib\b|hkdf_max_info_size|\bikm_bytes|\bprk_bytes|\binfo_bytes" libs/hash tools/test/fixtures` prints nothing.

- [ ] **Step 5: Commit** (hash branch: `hash: public names spelled out (input_key_material, pseudorandom_key, application_info, scrypt's cost, block_size and parallelism, argon2's memory_kibibytes)`; superproject branch: `tools: the hash fixtures use hash's spelled-out names`), and add the changelog line (a `*Breaking change:*` line folded with the pieces line of Task 7 is allowed; Task 7 finalises the section).

---

### Task 2: `byte_pieces`, the pieces tags and the pieces forms on `backends::hash2`

**Files:**
- Modify: `libs/hash/include/webcpp/hash/config.hpp` (tags), `kdf.hpp` (`backend_has_v`,
  defaults), `hkdf.hpp` (`byte_pieces`, the three overloads), `detail/engines.hpp` (primary
  `hkdf_pieces_engine` with its refusal), `backends/hash2.hpp` (its specialisation)
- Modify: `libs/hash/doc/capabilities.json` (three rows), `tools/hash/doc_tables.py` and
  `tools/test/hash_doc_tables_test.py` (superproject) if the rows need a new field,
  `libs/hash/test/capabilities_test.cpp`, `libs/hash/test/support/timing_coverage.hpp`,
  `libs/hash/test/allocation_test.cpp`, `libs/hash/test/Jamfile`
- Create: `libs/hash/test/hkdf_pieces_test.cpp`, `libs/hash/test/support/hkdf_pieces.hpp`
  (the equality harness every engine's test reuses), `libs/hash/test/byte_pieces_test.cpp`

**Interfaces:**
- Consumes: Task 1's names.
- Produces:

```cpp
namespace webcpp::hash {
/** Byte sequences read one after another, as if concatenated: `{a, b, c}` at the call. */
class byte_pieces {
public:
    using value_type = std::span<std::uint8_t const>;
    constexpr byte_pieces() noexcept = default;
    constexpr byte_pieces(std::initializer_list<value_type> pieces WEBCPP_HASH_LIFETIMEBOUND) noexcept;
    template <std::ranges::contiguous_range Range>   // of value_type, not byte_pieces itself
    constexpr byte_pieces(Range const& pieces WEBCPP_HASH_LIFETIMEBOUND) noexcept;
    [[nodiscard]] constexpr value_type const* begin() const noexcept;
    [[nodiscard]] constexpr value_type const* end() const noexcept;
    [[nodiscard]] constexpr std::size_t size() const noexcept;      // the number of pieces
    [[nodiscard]] constexpr bool empty() const noexcept;
    [[nodiscard]] constexpr std::uint64_t total_size() const noexcept;  // the bytes of all pieces
};
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_extract_pieces>>
[[nodiscard]] boost::system::result<typename detail::hmac_result<Hmac>::type> hkdf_extract(
    std::span<std::uint8_t const> salt, byte_pieces input_key_material);
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_expand_pieces>>
[[nodiscard]] boost::system::result<void> hkdf_expand(std::span<std::uint8_t> output_key_material,
    std::span<std::uint8_t const> pseudorandom_key, byte_pieces application_info);
template <class Hmac, class Backend = default_kdf_backend<Hmac, kdfs::hkdf_pieces>>
[[nodiscard]] boost::system::result<void> hkdf(std::span<std::uint8_t> output_key_material,
    std::span<std::uint8_t const> salt, byte_pieces input_key_material,
    byte_pieces application_info);
namespace kdfs { struct hkdf_pieces; struct hkdf_extract_pieces; struct hkdf_expand_pieces; }
}
namespace webcpp::hash::detail {
// Each specialisation: noexcept static members, called after the public guards; wipes every
// intermediate; never allocates.
template <class Backend, class Hmac> struct hkdf_pieces_engine;
//   static result<typename Hmac::result_type> extract(std::span<std::uint8_t const> salt, byte_pieces input_key_material) noexcept;
//   static result<void> expand(std::span<std::uint8_t> output_key_material, std::span<std::uint8_t const> pseudorandom_key, byte_pieces application_info) noexcept;
//   static result<void> extract_and_expand(std::span<std::uint8_t> output_key_material, std::span<std::uint8_t const> salt, byte_pieces input_key_material, byte_pieces application_info) noexcept;
}
```
  `WEBCPP_HASH_LIFETIMEBOUND` is `[[clang::lifetimebound]]` under clang (`__has_cpp_attribute`),
  `[[msvc::lifetimebound]]` under MSVC where it exists, else empty; defined in `config.hpp`.
  `backend_has_v<backends::hash2, kdfs::hkdf_pieces | hkdf_extract_pieces | hkdf_expand_pieces,
  Hmac>` equals `backend_has_v<backends::hash2, kdfs::hkdf, Hmac>`; the other backends' cells
  come with their engines (Tasks 3, 4, 6). `default_backend` of each pieces tag is
  `backends::hash2`.
  The capability table gains three rows in the `hkdf` group: name `hkdf_extract`,
  `hkdf_expand`, `hkdf` with `"form": "pieces"` and tags `kdfs::hkdf_extract_pieces`,
  `kdfs::hkdf_expand_pieces`, `kdfs::hkdf_pieces` (Ruling R2), header `webcpp/hash/hkdf.hpp`,
  default `hash2`, the hash2 cell over every hmac with "other", `constant_time` as the HKDF
  hash2 cell. Raise the table's `version` and describe `form` in `layout`; `doc_tables.py` keys a
  function row by `(name, form)` and renders the pieces rows as "`hkdf_extract` over pieces";
  `capabilities_test` and `timing_coverage` key the same way.

- [ ] **Step 1: Write the failing tests**

`byte_pieces_test.cpp` (compiled on every lane, the B2 record):
```cpp
// {a, b, c} binds to byte_pieces; one span picks the single-buffer overload (D1, B1); a
// std::array and a std::vector of spans pass through the range constructor; the pieces form has
// no cap (Review Focus 3).
std::array<std::uint8_t, 3> const x{1, 2, 3};
std::span<std::uint8_t const> const a{x}, b{x}, c{x};
static_assert(std::is_same_v<decltype(hash::hkdf_extract<hash::hmac_sha2_256>(a, {a, b, c})),
                             boost::system::result<hash::hmac_sha2_256::result_type>>);
BOOST_TEST_EQ(hash::byte_pieces({a, b, c}).size(), 3U);
BOOST_TEST_EQ(hash::byte_pieces({a, b, c}).total_size(), 9U);
std::array<std::span<std::uint8_t const>, 2> const named{a, b};
std::vector<std::span<std::uint8_t const>> const listed{a, b, c};
BOOST_TEST_EQ(hash::byte_pieces(named).size(), 2U);
BOOST_TEST_EQ(hash::byte_pieces(listed).size(), 3U);
std::vector<std::uint8_t> const long_info(32769, 0x5a);
std::array<std::uint8_t, 32> prk{};
std::array<std::uint8_t, 42> out{};
BOOST_TEST(hash::hkdf_expand<hash::hmac_sha2_256>(out, prk, {std::span(long_info)}).error() ==
           hash::error::input_too_long);   // {x} is the single-buffer form
BOOST_TEST(hash::hkdf_expand<hash::hmac_sha2_256>(out, prk,
                                                   hash::byte_pieces{std::span(long_info)}));
```

`support/hkdf_pieces.hpp`, `template <class Hmac, class Backend, class Oracle = backends::hash2>
void check_hkdf_pieces(std::string_view backend_name)`, and `hkdf_pieces_test.cpp` calling it
for `backends::hash2` over `hmac_sha1_160`, `hmac_sha2_256`, `hmac_sha2_512`, `hmac_sha3_256`,
`hmac_md5_128` and one "other" hmac (`boost::hash2::hmac<boost::hash2::sha2_384>`):
  - RFC 5869's seven vectors (`support/rfc5869.hpp`), each input split at every byte offset
    into two pieces, and into one piece per byte with an empty piece between every two; the
    vectors whose input is empty also as the empty list `{}`; PRK and OKM equal the vector's.
  - 1,000 random cases from the fixed seed `0x686b64662d706965` (printed), each input of 0 to 300
    bytes split at random points into 0 to 16 pieces (empty pieces among them), outputs of 1 to
    `255 * HashLen` bytes; each of `hkdf_extract`, `hkdf_expand`, `hkdf` over pieces equals the
    single-buffer form of `Oracle` on the concatenation (for BearSSL, Task 3, the Oracle is
    hash2 for Extract and Expand).
  - A list of 10,000 one-byte pieces equals the single-buffer form.
  - Long inputs: an application info of 4 KiB, 32 KiB + 1 and 1 MiB and an input keying
    material of 1 MiB, each in four pieces: up to 32,768 bytes equal to the single-buffer form;
    beyond it, equal to an RFC 5869 computation written in the harness over
    `boost::hash2::hmac<H>` on the concatenated bytes (`reference_hkdf`, 20 lines, independent
    of hash's code) and to the same backend on another split.
  - The guards: an output of 0 and of `255 * HashLen + 1` bytes return `invalid_output_size`
    with the output untouched (filled with `0xa5` before, still `0xa5` after); `hkdf_expand` with
    a pseudorandom key of `HashLen - 1` bytes returns `invalid_key_size`.

`capabilities_test.cpp`: the three pieces rows exist, each cell equals `backend_has_v` of its
tag, each pieces cell's hmacs equal the backend's HMAC cells (the hmac `macs::hmac<D>` of each
column's digest), and the default is hash2.

`allocation_test.cpp`: the three pieces forms on hash2 over `hmac_sha2_256` with 4 KiB and 1 MiB
inputs in four pieces and with each refusal, under the counter; zero `operator new` and zero C
allocator calls; the per-backend total asserted exactly, as the file does for every backend.

`support/timing_coverage.hpp`: `cover_hkdf_extract_pieces`, `cover_hkdf_expand_pieces`,
`cover_hkdf_pieces` (valid input, then a refused one: an empty output for Expand and hkdf, none
for Extract, which refuses nothing before the backend), registered from the pieces cells as the
single-buffer ones are.

- [ ] **Step 2: Run them, expect failure**

Run: `B2 toolset=clang libs/hash/test//byte_pieces libs/hash/test//hkdf_pieces libs/hash/test//capabilities libs/hash/test//allocation libs/hash/test//timing_coverage_hash2`
Expected: compile errors (`byte_pieces` undeclared).

- [ ] **Step 3: Implement** `byte_pieces`, the tags, the cells, the three overloads (the guards
  of Global Constraints, then the engine; `hkdf` wipes the PRK between the steps through the
  engine's `extract_and_expand`), the primary `hkdf_pieces_engine` (refusals: WebCrypto's
  "asynchronous" text naming `async_hkdf_extract`, `async_hkdf_expand` and `async_hkdf`, and
  the no-HMAC text naming the backends that have the pieces forms, the same backends as the
  HMAC cells), and hash2's engine:

```text
extract(salt, pieces):  h = hmac<H>(salt.empty() ? HashLen zeros : salt); for p in pieces: h.update(p); return h.result()
expand(out, prk, info): keyed = hmac<H>(prk); t = {}; for i = 1..ceil(L/HashLen):
                         h = keyed (a copy); h.update(t); for p in info: h.update(p); h.update({i}); t = h.result();
                         copy min(HashLen, rest) of t to out; wipe h; then wipe t and keyed
```
  The single-buffer overloads are unchanged in behaviour.

- [ ] **Step 4: Run the tests and the tools' tests**

Run: the Step 2 command, `python3 tools/hash/doc_tables.py --check` (after `--write`),
`python3 tools/test/hash_doc_tables_test.py`, and `B2 toolset=gcc-15` for `byte_pieces` and
`hkdf_pieces`. Expected: all pass, no warning.

- [ ] **Step 5: Commit** (hash: `hash: hkdf over byte pieces on backends::hash2, no cap and no heap`; superproject, if doc_tables changed: `tools: hash's doc tables read a function row's form`).

---

### Task 3: The pieces engines of `sodium` and `bearssl`

**Files:**
- Modify: `libs/hash/include/webcpp/hash/backends/sodium.hpp`, `backends/bearssl.hpp`,
  `kdf.hpp` (their cells), `doc/capabilities.json` (their cells in the three pieces rows),
  `test/allocation_test.cpp`, `test/Jamfile`
- Create: `libs/hash/test/hkdf_pieces_sodium_test.cpp`, `hkdf_pieces_bearssl_test.cpp`

**Interfaces:**
- Consumes: Task 2's `byte_pieces`, tags, `hkdf_pieces_engine`, `check_hkdf_pieces`.
- Produces: `hkdf_pieces_engine<backends::sodium, hmac_sha2_256 | hmac_sha2_512>` over
  `crypto_auth_hmacsha256_{init,update,final}` / `crypto_auth_hmacsha512_*` (the state struct
  copied per block, wiped with `sodium_memzero`); `hkdf_pieces_engine<backends::bearssl, H>` for
  BearSSL's six hmacs over `br_hmac_key_init`, `br_hmac_init`, `br_hmac_update`, `br_hmac_out`
  (the `br_hmac_key_context` made once, `br_hmac_context` per block). The cells:
  `backend_has_v<sodium | bearssl, kdfs::hkdf_*_pieces, H>` equal their HMAC cells
  (`kdfs::hkdf`'s hmacs for sodium; BearSSL's six for all three, Extract and Expand included,
  while the single-buffer `hkdf_extract` and `hkdf_expand` stay refused).

- [ ] **Step 1: Write the failing tests**: each test calls `check_hkdf_pieces<H, B>` for every
  hmac of the backend's cells (BearSSL's Oracle `backends::hash2`), plus the spec's scenario
  `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, {a, b})` equal to hash2's, and the
  single-buffer `hkdf_extract<hmac_sha2_256, backends::bearssl>(salt, ikm)` still refused (a
  `test/messages` case exists for it; keep it). `allocation_test.cpp`: the three pieces forms
  on sodium and bearssl, 4 KiB and 1 MiB in four pieces and the refusals, zero allocations,
  exact totals; the spec scenario "A Welcome-sized info" (1 MiB application info in four pieces
  through `hkdf_expand<hmac_sha2_256>` on hash2, sodium and bearssl) is one of them.
- [ ] **Step 2: Run, expect failure**: `B2 toolset=clang libs/hash/test//hkdf_pieces_sodium libs/hash/test//hkdf_pieces_bearssl`
- [ ] **Step 3: Implement** both engines and cells; the timing coverage of sodium and bearssl
  picks the cells up from the table (`timing_coverage_sodium`, `timing_coverage_bearssl`).
- [ ] **Step 4: Run**: Step 2's targets plus `capabilities`, `allocation`,
  `timing_coverage_sodium`, `timing_coverage_bearssl`, on clang and on `toolset=emscripten`
  for the two engine tests. Expected: pass.
- [ ] **Step 5: Commit**: `hash: hkdf over byte pieces on backends::sodium and backends::bearssl`.

---

### Task 4: The pieces engines of `openssl` and `botan`

**Files:**
- Modify: `libs/hash/include/webcpp/hash/backends/openssl.hpp`, `backends/botan.hpp`,
  `doc/capabilities.json`, `test/allocation_openssl_test.cpp`, `test/allocation_botan_test.cpp`,
  `test/Jamfile`
- Create: `libs/hash/test/hkdf_pieces_openssl_test.cpp`, `hkdf_pieces_botan_test.cpp`

**Interfaces:**
- Consumes: Task 2's interfaces.
- Produces: `hkdf_pieces_engine<backends::openssl, H>` for `openssl_hmac_v<H>` over `EVP_MAC`
  "HMAC" (`EVP_MAC_CTX_new`, `EVP_MAC_init` with the digest name, `EVP_MAC_update`,
  `EVP_MAC_final`, `EVP_MAC_CTX_dup` per block, `EVP_MAC_CTX_free`, `OPENSSL_cleanse` of every
  buffer; hash's OpenSSL rules: the error queue left clean, `memory_functions_not_installed`
  where the other OpenSSL calls return it, `errc::not_enough_memory` on an allocation failure);
  `hkdf_pieces_engine<backends::botan, H>` for `botan_hmac_v<H>` over
  `Botan::MessageAuthenticationCode::create_or_throw("HMAC(<hash>)")`, `set_key`, `update`,
  `final` (re-keyed per block with `clear` + `set_key`), every exception caught and turned into
  the family's error as the other Botan engines do. Cells equal the backends' `kdfs::hkdf`
  cells (`openssl_hmac_v`, `botan_hmac_v`).

- [ ] **Step 1: Write the failing tests**: `check_hkdf_pieces` over every hmac of each cell;
  `openssl_out_of_memory_test.cpp` gains the three pieces forms (each allocation failed in turn:
  `not_enough_memory` or success, the output zeroed on failure, nothing leaked);
  `openssl_error_queue_test.cpp` the three forms leave the queue empty;
  `allocation_openssl_test.cpp` and `allocation_botan_test.cpp` print the libraries' own counts
  for the three forms (one `record` line each, as their other rows).
- [ ] **Step 2: Run, expect failure**: `B2 toolset=clang libs/hash/test//hkdf_pieces_openssl libs/hash/test//hkdf_pieces_botan`
- [ ] **Step 3: Implement** both engines and cells.
- [ ] **Step 4: Run** Step 2's targets plus `capabilities`, `capabilities_botan`,
  `timing_coverage_openssl`, `timing_coverage_botan`, `openssl_out_of_memory`,
  `openssl_error_queue`, `allocation_openssl`, `allocation_botan`; the two engine tests also on
  `toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh`. Expected: pass.
- [ ] **Step 5: Commit**: `hash: hkdf over byte pieces on backends::openssl and backends::botan`.

---

### Task 5: The asynchronous forms on the synchronous backends

**Files:**
- Modify: `libs/hash/include/webcpp/hash/async.hpp`, `test/async_post_test.cpp`,
  `test/timing_async_test.cpp`, `test/allocation_test.cpp`, `test/async_cobalt_test.cpp` (one
  pieces call through Cobalt), `test/support/timing_coverage.hpp` if it lists async functions,
  `doc/capabilities.json` (`layout`'s `asynchronous_only` text names the new functions)

**Interfaces:**
- Consumes: Tasks 1-4.
- Produces (each `template <class Hmac, class Backend = default_async_backend<kdfs::...,
  Hmac>, class Executor, class CompletionToken>`, completion `void(boost::system::error_code)`,
  over the four hmacs of `webcrypto_hmac_v`, the others refused by a `static_assert` naming the
  synchronous function):

```cpp
auto async_hkdf_extract(Executor const& executor, std::span<std::uint8_t> pseudorandom_key,
                        std::span<std::uint8_t const> salt,
                        std::span<std::uint8_t const> input_key_material, CompletionToken&& token);
auto async_hkdf_extract(Executor const& executor, std::span<std::uint8_t> pseudorandom_key,
                        std::span<std::uint8_t const> salt, byte_pieces input_key_material,
                        CompletionToken&& token);
auto async_hkdf_expand(Executor const& executor, std::span<std::uint8_t> output_key_material,
                       std::span<std::uint8_t const> pseudorandom_key,
                       std::span<std::uint8_t const> application_info, CompletionToken&& token);
auto async_hkdf_expand(Executor const& executor, std::span<std::uint8_t> output_key_material,
                       std::span<std::uint8_t const> pseudorandom_key,
                       byte_pieces application_info, CompletionToken&& token);
auto async_hkdf(Executor const& executor, std::span<std::uint8_t> output_key_material,
                std::span<std::uint8_t const> salt, byte_pieces input_key_material,
                byte_pieces application_info, CompletionToken&& token);
```
  Defaults: `kdfs::hkdf_extract`, `kdfs::hkdf_expand`, `kdfs::hkdf` for the single-buffer
  forms, the `_pieces` tags for the pieces forms. `async_hkdf_extract`'s `pseudorandom_key`
  must be exactly `HashLen` bytes, else `invalid_output_size` (output untouched).
  On a synchronous backend: the single-buffer forms run their synchronous function through
  `complete_by_post` as `async_hkdf` does; **the pieces forms run the synchronous function
  before `async_initiate`, in the call itself, and post only the resulting code** (a new
  `detail::complete_with_code(executor, token, code)`), so that no token, `asio::deferred`
  included, keeps the list (Review Focus 2, Ruling R4). On `backends::webcrypto` every form goes
  to `detail::async_engine<Backend>::template hkdf_extract<Hmac>(...)`,
  `hkdf_expand<Hmac>(...)`, `hkdf<Hmac>(...)` (pieces) (Task 6); the non-Emscripten
  `async_engine` refusal gains those members.

- [ ] **Step 1: Write the failing tests** in `async_post_test.cpp`: each new form over
  `hmac_sha2_256` on hash2 (and once on sodium) with a callback, `use_awaitable` and
  `use_future`, equal to the synchronous result and completing through a post (the handler not
  run inside the call); each refusal (empty output, a `pseudorandom_key` output of 31 bytes for
  Extract, a 31-byte key for Expand) completing with its error, output untouched; the spec's
  temporary-list scenario: `async_hkdf<hmac_sha2_256>(executor, output, salt, {a, b}, {c, d},
  token)`, then `a` to `d` overwritten before `run()`: success and the HKDF of the original
  bytes; the same with `asio::deferred`, the deferred operation launched after the overwrite and
  after the list's full-expression ended. `timing_async_test.cpp`: each new form enters the
  guard. `allocation_test.cpp`: the new forms with the arena-bound handler allocate nothing
  outside the arena, which gets every block back before the handler runs.
- [ ] **Step 2: Run, expect failure**: `B2 toolset=clang libs/hash/test//async_post libs/hash/test//timing_async`
- [ ] **Step 3: Implement** the five overloads and `complete_with_code`.
- [ ] **Step 4: Run**: Step 2's targets plus `allocation`, `async_cobalt`, `capabilities`, on
  clang, `gcc-15` and `toolset=clang-wasip2 testing.launcher=tools/boost/asio-wasip2/run.sh`.
- [ ] **Step 5: Commit**: `hash: async_hkdf_extract, async_hkdf_expand and async_hkdf over byte pieces`.

---

### Task 6: WebCrypto's pieces engine over `subtle.sign("HMAC", ...)`

**Files:**
- Modify: `libs/hash/include/webcpp/hash/backends/webcrypto.hpp`, `doc/capabilities.json`
  (webcrypto cells: the three pieces rows and the single-buffer `hkdf_extract`, `hkdf_expand`
  rows, over the four hmacs, asynchronous only, no `partial`), `test/capabilities_test.cpp`
  (`check_webcrypto_cells`), `test/async_webcrypto_test.cpp`, `test/allocation_test.cpp`
  (`async_webcrypto`)

**Interfaces:**
- Consumes: Task 5's dispatch: `async_engine<backends::webcrypto>::hkdf_extract<Hmac>(executor,
  pseudorandom_key, salt, byte_pieces, token)`, `::hkdf_expand<Hmac>(executor,
  output_key_material, pseudorandom_key, byte_pieces, token)`, `::hkdf<Hmac>(executor,
  output_key_material, salt, byte_pieces, byte_pieces, token)` (pieces), plus the existing
  single-buffer `::hkdf`. The single-buffer `async_hkdf_extract` and `async_hkdf_expand` call
  the pieces members with one piece (B5); they keep `hkdf_max_application_info_size` (32,768)
  and not the 1,024 of `async_hkdf` (Ruling R3).
- Produces: a composed operation (`async_compose`, awaiting each promise through
  `browser::async_await` as `start_webcrypto` does) whose state holds `emscripten::val`s only:
  - **Gathering (in the initiating call):** `new Uint8Array(total)` and `set(piece, offset)` for
    each piece (one `typed_memory_view` per piece); nothing on the C++ heap beyond the
    operation's own state, which comes from the handler's allocator as every async state does.
  - **Extract:** `importKey("raw", salt or HashLen zeros, {name: "HMAC", hash}, false,
    ["sign"])`, then `sign("HMAC", key, gathered)`; result copied into `pseudorandom_key`.
  - **Expand:** a buffer of `HashLen + |info| + 1` bytes with the gathered info at offset
    `HashLen`; `importKey` of the pseudorandom key, then for block `i` one `sign` over the view
    `info || i` (i = 1) or `T(i-1) || info || i`; each `T(i)` copied into the output when its
    promise settles and into the buffer for the next block.
  - **hkdf:** Extract's two promises, then Expand's chain, the PRK kept in JavaScript memory.
  - Every JavaScript buffer that held a secret is `fill(0)` after the last call, on success,
    failure and cancellation. Cancellation and failure follow `start_webcrypto`'s rules
    (`operation_aborted` at once with the output zeroed; a late settlement writes nothing;
    `backend_failure` on a rejected promise, output zeroed).
  - The guards run first (`timing_guard`, then RFC 5869's), refusals completing through the
    event loop with the output untouched, as every webcrypto call does.

- [ ] **Step 1: Write the failing tests** in `async_webcrypto_test.cpp`: the three pieces forms
  and the two new single-buffer forms over SHA-1, SHA-256, SHA-384, SHA-512, with a callback,
  `use_awaitable` and `use_future`: RFC 5869 A.1 split into pieces; 100 random cases each
  against `backends::hash2`'s pieces form (inputs split into 0 to 8 pieces, empty pieces and
  empty salts among them); node's own `crypto.subtle.deriveBits` HKDF (through the existing
  single-buffer `async_hkdf<..., webcrypto>`) equal to the pieces form for infos up to 1,024
  bytes; the spec scenario: a 64 KiB application info in pieces completes with hash2's bytes
  while the single-buffer `async_hkdf<hmac_sha2_256, webcrypto>` with the same 64 KiB returns
  `input_too_long`; a piece list whose bytes change after the initiating call gives the
  original's result; a cancellation during Expand's chain completes with `operation_aborted`,
  output zeroed. `allocation_test.cpp`'s `async_webcrypto`: each pieces form calls the C
  allocator exactly once per promise it awaits (Extract 2, Expand `1 + ceil(L / HashLen)`,
  hkdf `3 + ceil(L / HashLen)`) and `operator new` never. `capabilities_test`: the webcrypto
  cells of the five rows true over the four hmacs, `backend_has_v` still false for webcrypto.
- [ ] **Step 2: Run, expect failure**: `B2 toolset=emscripten libs/hash/test//async_webcrypto libs/hash/test//capabilities_emscripten`
- [ ] **Step 3: Implement** the operation and the three engine members.
- [ ] **Step 4: Run**: Step 2's targets plus `allocation` and `webcrypto_oracle` on
  `toolset=emscripten`, debug and `variant=release`. Expected: pass.
- [ ] **Step 5: Commit**: `hash: backends::webcrypto's hkdf over byte pieces, on WebCrypto's HMAC`.

---

### Task 7: Timing during, the pages and the changelog

**Files:**
- Modify: `libs/hash/test/timing_during_test.cpp` (the three pieces forms set DIT during the
  HMAC calls: the seam's check runs inside the engine on hash2), `libs/hash/doc/kdfs.adoc`,
  `doc/async.adoc`, the WebCrypto page (the page that documents `backends::webcrypto`, under
  `doc/`), `doc/allocation.adoc` and `doc/measurements.allocation.json` (through
  `bench/allocation_collect.py` from `allocation_release`, release builds only),
  `doc/changelog.adoc`, `doc/guide.json` if a new public name needs its entry,
  `libs/hash/example/` (Create `hkdf_pieces.cpp`: HPKE's `LabeledExtract` written as
  `hkdf_extract<hmac_sha2_256>(salt, {hpke_v1, suite_id, label, ikm})`, its `example/Jamfile`
  line and its `.expected`, used by kdfs.adoc's include)

- [ ] **Step 1:** `timing_during_test.cpp` gains the three pieces forms; run it, expect failure
  until registered; register; pass.
- [ ] **Step 2:** The pages: `kdfs.adoc` "HKDF over pieces" (what it is for; the overloads; no
  cap, no heap; the `{a, b, c}` call; `byte_pieces{x}` for one piece; a named list as a
  `std::array` of spans, never a `byte_pieces` variable; the table of where each form runs from
  the generated backends table; HPKE's labeled inputs as the example, included from
  `example/hkdf_pieces.cpp`); `async.adoc` the three asynchronous HKDF functions and the rule
  that the pieces are read in the call; the WebCrypto page: the pieces forms over HMAC, the
  1,024-byte limit of the single-buffer `async_hkdf` alone, and `byte_pieces{info}` for a longer
  info; `allocation.adoc`: the pieces rows regenerated from `allocation_release` output with
  `bench/allocation_collect.py`. Every page uses D7's names.
- [ ] **Step 3:** The changelog's "Unreleased": one line `* Added HKDF over byte pieces
  (hkdf_extract, hkdf_expand, hkdf and their async forms), with no cap and no heap on every
  backend.` and one `* *Breaking change:* ...` line naming the renames (scrypt's `cost`,
  `block_size`, `parallelism`, argon2's `memory_kibibytes`, `hkdf_max_application_info_size`),
  within the ten-line limit (fold an existing line where needed; `tools/release/changelog.py`
  checks the form).
- [ ] **Step 4: Run**: `B2 toolset=clang libs/hash/test//timing_during libs/hash/example`,
  `python3 tools/hash/doc_tables.py --check`, the doc build (`B2 libs/hash/doc`), and
  `python3 tools/release/changelog.py` (or the lint rule `changelog form`). Expected: pass.
- [ ] **Step 5: Commit**: `hash: the guide documents hkdf over byte pieces and its asynchronous forms`.

---

## Rulings (the controller's, while planning)

- **R1:** `{x}` with a lone span names the single-buffer overload (D1; the language's rule), so
  the spec scenario "then as `{info}`" is written `byte_pieces{info}`; the guide says so. Cost if
  wrong: a pair of overloads later, no break.
- **R2:** the pieces rows keep their functions' names with `"form": "pieces"` and their own
  tags, since the header checks a row's name against a declared template; the table's version
  rises. Cost if wrong: a rename of three rows.
- **R3:** WebCrypto's single-buffer `async_hkdf_extract` and `async_hkdf_expand` run over HMAC
  (B5) and keep hash's 32,768-byte cap of the single-buffer forms, not WebCrypto's 1,024, which
  is `deriveBits`' and only `async_hkdf`'s. Cost if wrong: one guard.
- **R4:** the asynchronous pieces forms read (sync backends: compute) in the call itself,
  before `async_initiate`, so no completion token can keep the list past the full-expression.
  Cost if wrong: with `asio::deferred` the computation happens at the call rather than at launch.
- **R5:** hash's "browser lane" is the Emscripten lane under node (node's `crypto.subtle`);
  hash has no browser runner of its own.
- **R6:** crypto on `main` calls `hkdf` positionally and spells none of the renamed names: no
  crypto branch.
