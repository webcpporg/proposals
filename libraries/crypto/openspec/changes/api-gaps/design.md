## Context

Read on the library's tree of 2026-11-20 with the change `data-independent-timing` applied
(`libs/crypto` of the superproject, the worktree `crypto-dit/webcpp`):

- `ciphers/gcm.hpp`: `seal` runs `engine::start`, `engine::run`, `engine::tag`; on a failure it
  calls `engine::abandon` and returns, leaving `data` as far as `run` got. `open` zeroes `data`
  on any failure after its guards. `ciphers/aead.hpp`'s `seal`, `hybrid/box.hpp`'s `seal` and
  `hybrid/sealed_box.hpp`'s `seal` zero their output on every engine failure ("a backend may
  fail after it began to encrypt, and neither the plaintext nor bytes that look sealed are
  left"). `update` abandons the message on a failure and leaves its piece as `run` left it.
  BearSSL's GCM engine never fails; OpenSSL's can (every `EVP_*` call is checked); the mock
  backend (`test/support/mock_backend.hpp`) fails each engine call on demand.
- `gcm`'s incremental seal is `begin_seal`, `update`, `result`; the genesis's requirement "GCM:
  no incremental open" leaves an incremental open to "a later increment ... only with the
  plaintext withheld until the tag verifies". The engine contract is `init`, `start(state,
  is_sealing, iv, additional_data)`, `run(state, data)` (in place), `tag`, `check(state,
  tag)`, `move`, `abandon`, `reset`, on BearSSL (`br_gcm_*`) and OpenSSL (`EVP_CipherInit_ex`,
  `EVP_CipherUpdate`, `EVP_DecryptFinal_ex`).
- `ciphers/aes.hpp`: `aes128<Backend = default_backend>`, where `default_backend` is
  `default_backend_t<families::gcm>` (`detail/defaults.hpp`, generated): OpenSSL natively,
  BearSSL on wasip2, wasip3 and Emscripten. `gcm<Cipher>` takes its backend from
  `Cipher::backend_type`, so `aes256_gcm = gcm<aes256<>>` needs `aes256<>` to name GCM's
  default. BearSSL's `block_engine` refuses `make`, `encrypt` and `decrypt` in member templates
  so that `gcm<aes256<backends::bearssl>>` can still name its cipher. `doc/tables/crypto.json`
  gives the block cipher the default `null` on every target ("`aes256<>` takes AES-GCM's
  default as its argument, and its `make` does not compile where that is BearSSL"); the
  genesis's requirement "Block cipher: BearSSL has none, and aes256 has no default of its own"
  made it so, "because `aes256<>` must stay the cipher of `aes256_gcm`".
- `curves/secp256k1.hpp`: `basic_keypair::make(secret, memory_resource)` checks the secret with
  `secp256k1_ec_seckey_verify`, draws the context's seed from `random_device`, and makes the
  context through the resource. `ed25519::basic_keypair::generate()` and
  `kx_keypair::generate()` draw a `secret<32>` and call `make`, mapping any failure of the
  generator to `entropy_unavailable`.
- `curves/secp256k1/tweak.hpp`: `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`,
  `pubkey_tweak_add`, `pubkey_tweak_mul` and `pubkey_combine` return `result<>`;
  `pubkey_negate` returns `public_key` and drops libsecp256k1's return value.
  `basic_keypair::ecdh_x` returns `result<secret<32>>` (commit `398acb8`, increment 2).
- `test/constant_time/dudect.hpp`: `measure` (one run of `count` measurements), `largest_t`
  (the largest |t| over the whole set and 100 croppings, the |t| alone), `twice` (a second seed
  when the first exceeds 10, the smaller kept), `choose` (the class-blind preparation),
  `verdict` (unjudged where `WEBCPP_TEST_DUDECT_UNJUDGED`, which the lane's Jamfile defines when
  `RUNNER_ENVIRONMENT=github-hosted`). The programs `dudect_bearssl`, `dudect_sodium`,
  `dudect_planted` and `dudect_secp256k1` print one human line of |t| values. b2's
  `testing.run` writes `<name>.output` (the program's output, then `EXIT STATUS: <n>`) beside
  the program (`<name>` natively, `<name>.wasm` on wasip2) and prints it only on a failure.
- The superproject's `.github/workflows/library.yml`, job `own-lanes`, runs a library's own
  lane with `python3 tools/ci/matrix.py own-lane`; it already has steps for one library's lane
  (beast's, webrtc's). The wasip2 lane's launcher is `tools/boost/asio-wasip2/run.sh`.

## Goals / Non-Goals

**Goals:** each of the six items closed with a test that fails before the change; the new
public functions hold the timing guard and are in the coverage tests; dudect judged on hosted
runners again, by a rule that a single unlucky process cannot fail, and every high value kept
with its evidence.

**Non-goals:** settling the cause of the 69.8 (it needs a Zen 3 machine of one's own); a
single-pass incremental open (it cannot withhold plaintext); a block-cipher default chosen by
measurement; generate() for any other key pair; the asynchronous family (the foundation's
increment 4).

**Anti-goals:** plaintext reaching the caller before its tag is verified; a default that does
not compile on a target; a lane that fails on one noisy process, or passes on a value nobody
sees.

## Decisions

### D1. A failed seal zeroes `data`, in `seal` and in `update`

`seal` zeroes the whole of `data` through `detail::wipe` whenever `engine::start`, `engine::run`
or `engine::tag` fails, before `engine::abandon`, as `aead::seal` does; `update` zeroes its
piece when `engine::run` fails. A guard's refusal (`invalid_state`, `invalid_iv_size`,
`message_too_long`) still leaves `data` untouched. The pieces an earlier `update` returned are
the caller's and are not reachable; the documentation says that an error ends the message and
its pieces have no tag. The zeroing is in `gcm.hpp`, the generic layer, so it holds on every
backend; the mock backend's failures test it, since BearSSL never fails and OpenSSL fails only
where a test cannot make it.

- **Alternative:** zero only on `run` and `tag` failures (`start` fails before writing). One rule
  for every engine failure is what `aead` does, and costs nothing.

### D2. GCM opens incrementally in two passes; nothing decrypted is released before the tag

The interface, members of `gcm<Cipher>`:

- `begin_open(iv, additional_data)`: the guards of `begin_seal`; keeps the IV (at most
  `max_iv_size`, 128 bytes, public) in the object; starts the engine for decryption with the IV
  and the whole additional data.
- `authenticate(std::span<std::uint8_t const> ciphertext_piece)`: the first pass, read only.
  The piece is copied in blocks of 256 bytes into a buffer of the object's call frame, decrypted
  there by `engine::run`, which also hashes the ciphertext, and the buffer is wiped after each
  block; the caller's bytes are never written.
- `verify(std::span<std::uint8_t const> tag)`: `engine::check`. On a match the engine is
  abandoned and started again for decryption with the kept IV and no additional data, and the
  object enters its second pass, remembering the authenticated length. On a mismatch,
  `authentication_failed`, and the message ends.
- `decrypt(std::span<std::uint8_t> piece)`: the second pass, in place; GCM's keystream depends
  on the key and the IV alone, so the second start needs no additional data, and the tag the
  engine computes then is never read. The pieces' sum must not pass the authenticated length
  (`invalid_input_size` otherwise, the piece untouched, the message ended).
- `end_open()`: ends the message; `invalid_input_size` when fewer bytes were decrypted than
  authenticated (the message still ends).

The object's state becomes one of none, sealing, authenticating, decrypting (an enumeration in
place of `in_message_`); `update` and `result` take sealing only, `authenticate` and `verify`
authenticating only, `decrypt` and `end_open` decrypting only, and anything else gives
`invalid_state`, ending a message in progress as `begin_seal` does today. A one-shot `seal` or
`open` during any message gives `invalid_state` and leaves it going. A move abandons any phase.
Any error ends the message; a failure of the engine in `decrypt` zeroes its piece.

- **The trade-off.** The ciphertext is read twice and decrypted twice (the first pass decrypts
  into a scratch block to compute GHASH through the backends' only interface), so an open costs
  about two one-shot opens. The second pass must read the same bytes as the first: if they
  change between the passes (a file another process writes), the second pass decrypts bytes
  that were never authenticated, and GCM's keystream makes such a change a chosen change of the
  plaintext. Only the caller can ensure it: the documentation says to read from storage no one
  else writes during the open, or to seal a stream that cannot be read twice in records, each
  opened whole, as the guide already teaches.
- **Alternatives:** decrypting piece by piece and releasing it before the tag (refused by the
  genesis); keeping the plaintext in the object until the tag (unbounded memory, which is the
  problem being solved); a second pass that recomputes the tag and compares it at `end_open`
  (it needs the additional data again, and it detects a change only after the plaintext was
  released); GHASH computed by webcpp over the ciphertext (webcpp writes no cryptography of its
  own; BearSSL's `br_gcm_context` internals are not its interface).
- **Engine contract:** unchanged. Every new function is in `gcm.hpp`.

### D3. The AES block cipher gets a default per target, resolved apart from GCM's

`doc/tables/crypto.json`'s family `aes_block` gets the default OpenSSL natively and ctaes on
wasip2, wasip3 and Emscripten, by the reasons AES-GCM's default gives: OpenSSL's assembly uses
the CPU's AES instructions on x86-64 and ARMv8 (on MSVC, built `no-asm`, the same rule with the
same note as GCM's); on WebAssembly ctaes is constant time by design (bitsliced) where OpenSSL
is built `no-asm` and table-driven, and OpenSSL is not built for Emscripten. The generator then
writes `default_backend_of<families::aes_block>`, the "not linked" refusals and the alias
`/webcpp/crypto//aes_block-default`, as for every family with a default.

The default template argument of `aes128`, `aes192` and `aes256` becomes a tag,
`backends::family_default`, which names no library (not to be confused with `crypto::default_backend`, AES-GCM's default): `basic_aes<KeySize, Backend>` reads its
backend as `detail::resolved_backend_t<Backend, families::aes_block>`, and `gcm<Cipher>` reads
its own as `detail::resolved_backend_t<typename Cipher::named_backend_type, families::gcm>`,
where `resolved_backend_t<backends::family_default, F>` is `default_backend_t<F>` and any other
backend is itself. So `aes256<>` is the block cipher's default, `gcm<aes256<>>`, `aes128_gcm`
and `aes256_gcm` stay AES-GCM's, and `gcm<aes256<backends::bearssl>>` is BearSSL as before.
`aes.hpp` includes the block default's backend header when the program links it, at its end,
as `aes256_gcm.hpp` does for GCM's.

- **Why a tag and not the block default as the argument:** with `aes256<> =
  aes256<default_backend_t<families::aes_block>>`, `gcm<aes256<>>` would be ctaes's GCM on
  WebAssembly, which does not exist; the tag lets each family resolve the same spelling to its
  own default.
- **What breaks:** `std::is_same_v<aes256<>, aes256<backends::openssl>>` is false now (its
  `backend_type` is the same); version 0.1's tests that assert it change. A program that called
  `aes256<>::make` on WebAssembly did not compile and now runs ctaes.
- **Alternatives:** keep "no default" and make `aes256<>::make` refuse on every target (a
  consistent refusal, but the owner asked for a fix and a reader expects `<>` to work);
  measure (D32 rules measurements out).

### D4. `basic_keypair::generate` draws again for a secret that is not one

`static result<basic_keypair> generate(std::pmr::memory_resource* = get_default_resource())`,
not `noexcept` (as `make`, whose resource may throw): draws a `secret<32>` from `random_device`
(any failure, `not_supported` included, is `entropy_unavailable`, as ed25519's), calls `make`,
and when `make` answers `invalid_secret_key` (32 bytes that are zero or not below the group
order, probability about 2^-128) draws again, at most 4 draws in all; a fourth refusal means
the generator is broken, and `generate` returns `entropy_unavailable`. `make` then draws the
context's seed, as it does for any key pair.

- **Alternative:** `secp256k1_keypair_create` on a random secret without a check (it returns 0
  on an invalid one, which would surface as `backend_failure`, a wrong error).

### D5. `pubkey_negate` returns `result<public_key>`; `ecdh_x` is pinned

`pubkey_negate` returns `result<public_key>` and stays `noexcept`, returning `backend_failure`
when `secp256k1_ec_pubkey_negate` returns 0, which libsecp256k1 v0.8.0 does not for a parsed
key, so that the seven functions of `tweak.hpp` have one shape and compose in the same code.
`ecdh_x` already returns `result<secret<32>>`; a `static_assert` in `secp256k1_ecdh_test.cpp`
pins the three ECDH forms' types. `serialize_public_key`, `ellswift_encode` and
`ellswift_decode`, which also cannot fail, keep their plain values: each is an encoding, total
by construction (ElligatorSwift decodes every 64 bytes to a point, BIP 324), and none sits
among neighbours of one shape that a caller composes as the tweaks are.

### D6. dudect's lines, evidence, control and one-case runs

- **One line per case, read by a script:** `dudect case=<name> t=<|t| with 2 decimals>
  threshold=10 judged=<yes|no>`; `judged=no` only for X25519's agreement on wasip2 (the open
  finding). The human line stays.
- **Evidence when |t| >= 10:** `largest_t` returns a `t_statistic` with the |t|, the cropping
  that gave it (`-1` for the whole set, else 0 to 99, and its limit in ns) and each class's
  count and mean there. The harness prints `dudect evidence case=<name> crop=<k> limit_ns=<x>
  class0_n=<n> class0_mean_ns=<x> class1_n=<n> class1_mean_ns=<x>`, then once per program
  `dudect machine cpu="<model>" microcode=<value> clock_resolution_ns=<x>` and per case
  `dudect addresses case=<name> <label>=0x<low 12 bits> ...` for the buffers the case names
  (its input, its output and its object). The CPU's model and microcode are `/proc/cpuinfo`'s
  `model name` and `microcode` on Linux, `machdep.cpu.brand_string` on macOS (microcode
  `unavailable`), `unavailable` under WebAssembly; the clock's resolution is the smallest
  nonzero difference of 10,000 pairs of `steady_clock::now()` read back to back, serialised.
- **An A/A control per program:** the same operation as one of its cases, prepared by
  `choose<..., control = true>`, which draws and blends exactly as `choose` does but with both
  classes' value the fixed one: `chacha_open_aa` in `dudect_bearssl` (ChaCha20-Poly1305's
  open, the case that read 69.8), `x25519_agree_aa` in `dudect_sodium`, `ecdh_x_aa` in
  `dudect_secp256k1`. Its |t| is judged as every case's: an A/A over 10 says the measurement on
  that machine is broken, which must fail. `dudect_planted` has none (it measures one case by
  design).
- **`--case <name>`:** the program measures that case alone (and prints the machine line), so a
  case is measured again in a fresh process; an unknown name exits with 2 and the list of
  names.

### D7. The CI prints, warns and, on hosted runners, judges two of three

`libs/crypto/test/constant_time/dudect_judge.py --bin <dir> --target <native|wasip2>
[--rerun] [--launcher <command>]` finds every `dudect_*.output` under the directory, prints
each in a `::group::`, emits `::warning title=dudect <program> <case> (<target>)::<line>` for
every case line with t >= 10 (the planted program excepted, whose high value is its pass), and
with `--rerun` runs each such judged case's program twice more (`<program> --case <name>`, on
wasip2 through the launcher with `<program>.wasm`), reading the case's line of each; the case
fails when at least two of the three values are >= 10, and the script exits 1 naming each
failed case with its three values. Without `--rerun` it only prints and warns (exit 0).

The superproject's `library.yml` runs it after the own lane, for crypto's constant-time lane,
`if: always()`, with `--rerun` when `runner.environment == 'github-hosted'`. The programs keep
`WEBCPP_TEST_DUDECT_UNJUDGED` on hosted runners, so that b2 does not fail a case the judge then
passes; their `verdict` line says that the CI's judge decides. On a machine of one's own the
lane judges as today, in one process.

- **Why fresh processes, not a third in-process run:** the 69.8 read on both seeds in one
  process; the hypothesis is a layout effect, which a new process (new addresses) varies.
- **Why a script in the CI, not in the program:** wasip2 has no processes to start, and the
  hosted runner is the CI's knowledge; the script runs natively for both targets.
- **Alternatives:** b2's `--verbose-test` (prints every test of the lane, and annotates
  nothing); `webcpp.drive` with a driver (a 30-second bound and no launcher for wasip2).

### D8. Every new public function holds the timing guard

`begin_open`, `authenticate`, `verify`, `decrypt`, `end_open` and secp256k1's `generate` start
with `detail::timing_guard const timing;` and are called by `cover_gcm_messages`
(`test/support/timing_coverage.hpp`) and `timing_coverage_secp256k1_test.cpp`, with valid input
and with input refused before the backend. `pubkey_negate` keeps its guard; its check reads the
result.

## Risks / Trade-offs

- A caller of the incremental open that lets the ciphertext change between its passes decrypts
  unauthenticated bytes (D2). Mitigated by the documentation, the names (`authenticate`, then
  `verify`, then `decrypt`) and the length check; it cannot be closed without a second tag.
- Hosted-runner judgment can still fail on a machine whose effect persists across processes;
  that failure is then a real finding to investigate, with its evidence in the log.
- The A/A cases lengthen each dudect program by one case (about a sixth of `dudect_bearssl`).

## Migration Plan

`pubkey_negate`'s callers add `.value()` or check the result (the library's tests and none of
its examples). `aes256<>`'s type identity changes; its `backend_type` natively does not. The
changelog's first line gains a clause (the ten-line rule, as the change
`data-independent-timing` did).

## As built (2026-11-21)

The change was built on the branch `crypto/gaps` of the library and the superproject, by the plan
`reference/plan.md`, with each task reviewed and a final review of the whole branch. What differs
from the decisions above, or adds to them:

- **D2, one rule for a call out of phase.** Every member of the incremental seal and open checks
  its phase the same way: with no message it only answers `invalid_state`; in another phase it
  abandons that message, then answers. So `update` and `result` during an open end it, as
  `begin_seal` does. The example `opened_in_two_passes` keeps its "file" in a `std::stringstream`,
  since the WASI launchers give a program no directory, and says to discard the plaintext written
  so far when `decrypt` or `end_open` fails. The allocation tests of OpenSSL and BearSSL pin that
  the two-pass open allocates nothing per message (on OpenSSL `verify` restarts the context with
  `EVP_CipherInit_ex`). The error reference names the open's sources of `invalid_state`,
  `invalid_tag_size` and `invalid_input_size`.
- **D3, `gcm.hpp` includes `aes.hpp`**, whose resolver it names. The WebAssembly row of the block
  cipher's default cites `default_bearssl` (a type check, since it links no ctaes) and
  `default_both_wasm` links ctaes; the message test of the default not linked runs natively (ctaes
  alone), on wasip2 and wasip3 (OpenSSL alone), and on Emscripten as the superproject's fixture
  `aes_block_default_not_linked_on_emscripten`, with `aes_block_default` the fixture of the alias.
  The page states that `aes256<>` is not the type `aes256<backends::openssl>`. A program that
  names `aes256<backends::openssl>` natively without linking OpenSSL is now refused by the
  default's text, as GCM's already was: a wording of the generator's to improve later.
- **D4, the bound is `basic_keypair::max_draws`** (4), a public constant the test reads; an
  all-zero draw is tested as well as one above the group order.
- **D5, the ECDH pin already existed** in `secp256k1_ecdh_test.cpp`; nothing was added for it.
- **D6, every decision is on the value as printed,** rounded to two decimals (`printed`), so a
  |t| of 9.996 prints `t=10.00`, with its evidence, and fails. An unjudged case (X25519's
  agreement on wasip2) is measured once, since a second seed changes no verdict. On arm64 Linux,
  whose `/proc/cpuinfo` has no `model name`, the machine line names the CPU by its implementer,
  part, variant and revision. The addresses print three hex digits. What the lane read (Apple M5
  Max, natively, release, 2026-11-21 UTC, on a machine under a load average of 95 to 235, so
  provisional): the A/A controls 1.61, 2.42 and 1.95; `dudect_planted` 92.88 and 53.20; on wasip2
  X25519's agreement 53.53 (not judged), its A/A 2.58, `ecdh_x` 1.90 and `ecdh_x_aa` 1.97.
- **D7, the judge also skips `dudect_harness`,** whose values are made up and which b2 judges
  itself. Under wasmtime `dudect_secp256k1` (two cases of 500,000 measurements) and
  `dudect_sodium` take several minutes, close to the launcher's 600 seconds: the CI gives
  crypto's constant-time lane and the judge `WEBCPP_WASIP2_TEST_TIMEOUT=1800`, the job 120
  minutes in place of 60, and the judge's own subprocesses 2400 seconds, so the launcher's bound
  fires first; the step runs unless the job was cancelled. The release notes of 0.1.0 list it
  under "Tools and build".
- **Open items.** The final measurement pass, after the streams of 2026-11-20 merge, times both
  wasip2 programs against the 1800-second bound and measures the lane's values on a quiet
  machine; the hosted constant-time lane's first judged run is read then (`tasks.md` 4.3).

## Open Questions

None: the owner approved the six items and rules the rest on autopilot; each ruling above is
recorded as made on 2026-11-20.
