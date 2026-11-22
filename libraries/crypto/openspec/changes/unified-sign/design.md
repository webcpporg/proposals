## Context

Three signing shapes coexist in webcpp::crypto (proposal.md, "Why"). P-256's synchronous names
followed secp256k1's when P-256 came first for webcpp::trystero (foundation design D36:
`sign_ecdsa`, `verify_ecdsa` over a 32-byte SHA-256 digest), while the asynchronous family
followed WebCrypto, whose ECDSA takes the message and a hash name. Ed25519 has always signed the
message (PureEdDSA, RFC 8032), with a free `verify(public_key, message, signature)`. secp256k1
signs two schemes with one key (BIP340's Schnorr over a message of any length, ECDSA over a
32-byte hash) and a recoverable ECDSA form, each a function named after its scheme.

The hazard is the 32-byte message. Every span in these signatures is a span of bytes, so a
32-byte message passed where a digest is expected compiles, signs the message unhashed, and
verifies under the same mistake; a program that moves from `async_sign` to the key pair's own
function, or from P-256 to secp256k1, changes the meaning of its argument without a diagnostic.

webcpp::hash's SHA-256 is `webcpp::hash::sha2_256`, Boost.Hash2's `boost::hash2::sha2_256`,
whose `result_type` is `boost::hash2::digest<32>`; webcpp::crypto's `/webcpp/crypto//crypto`
already brings webcpp::hash.

## Goals / Non-Goals

**Goals:** one name per operation, `sign` and `verify`, with the scheme named at every call and
the same inputs synchronously and asynchronously, on Ed25519, P-256 and secp256k1; the message as
the ordinary input; a digest only through a marker; every pair of scheme and input that is not
valid refused at compile time with a message that names why; every caller in libs/crypto moved.

**Non-goals:** deprecation shims (crypto has no release); other hashes for ECDSA (P-256 over
SHA-384, P-384, Ed25519ph as a tag), which the tags leave room for; secp256k1 in the asynchronous
family; renaming MuSig2's session functions or silent payments.

**Anti-goals:** any overload that chooses between message and digest by a span's extent or size
(a 32-byte message must never be treated as a digest, and `std::array<std::uint8_t, 32>` must
not be ambiguous); a curve inferred from a number of bytes.

## Approaches

**A. Keep both shapes and document them.** The synchronous key pairs keep the digest, the
asynchronous family keeps the message. Nothing breaks, and the hazard stays: the same bytes mean
two things across a call boundary, and only a reader of the documentation knows which.

**B. Message everywhere, the digest under another name.** `sign(message)` hashes; a second
name (`sign_digest`, `sign_prehashed`) takes a 32-byte span. The default is right, but the digest
form is still a bare span: a call that picks the wrong name compiles, and each curve and each
asynchronous function doubles its names; secp256k1's two schemes would need four names for
signing alone.

**C. One name, the scheme always named, the digest only through a marker type (chosen).**
`key.sign(scheme, message)` and `key.sign(scheme, crypto::prehashed{digest})` are one name with
two input types that never convert into each other. The scheme is a tag object that names the
algorithm and, for ECDSA, its hash (`ecdsa_sha256`), as WebCrypto's `{name: "ECDSA", hash:
"SHA-256"}` does: the hash is explicit at the call site, the scheme fixes the digest's size, and a
scheme that takes no digest (BIP340's Schnorr, pure Ed25519) refuses the marker by type. The same
forms go to `async_sign` and `async_verify`, checked by `can_sign` and `can_verify`. secp256k1,
whose one key signs two schemes, needs nothing of its own: every curve names its scheme.

Option C was settled in two steps on 2026-11-22. The owner first chose a message-or-marker input
with a hash-parameterized marker (`prehashed<Hash>`) and a scheme-less `sign(input)` for the
curves of one scheme, then brought secp256k1 in with scheme tags, and finally decided that the
scheme is always required, that the ECDSA tag names its hash, and that the marker therefore takes
no hash parameter. A Bitcoin sighash (SHA-256d) is honestly `prehashed` under `ecdsa_sha256`: it
is the SHA-256 of a SHA-256, so no separate marker for protocol digests is needed.

Option C was chosen because it removes the hazard by type rather than by name or by document,
keeps one name per operation across curves and across the synchronous and asynchronous families,
and lets later schemes (`ecdsa_sha384`, an Ed25519ph tag) join without changing any call.

## Decisions

### D1. The header and the tags

`<webcpp/crypto/signing.hpp>` (namespace `webcpp::crypto`) holds what the curves share: the tag
types `eddsa_t`, `ecdsa_sha256_t`, `ecdsa_sha256_recoverable_t` and `schnorr_t`, each an empty
struct with an explicit default constructor, and their objects `inline constexpr eddsa_t
eddsa{}` and so on; `prehashed<N>` and its deduction guides; the traits that say which scheme
takes which input; and the concepts `can_sign<Keypair, Scheme, Input>` and
`can_verify<Scheme, Input, PublicKey>`. Each curve's header includes it and declares, for its
own key types, which schemes they have. A tag's explicit constructor keeps `{}` from standing
for a scheme.

### D2. The inputs

- A message is anything that converts to `std::span<std::uint8_t const>`: a span, a
  `std::array`, a `std::vector`, of any size, empty included.
- `prehashed<N>` is an aggregate `{ std::span<std::uint8_t const, N> digest; }`, N never
  `std::dynamic_extent`, deduced from a `std::array<std::uint8_t, N>`, a
  `std::span<std::uint8_t (const), N>` or a `boost::hash2::digest<N>`. No conversion turns a
  span or an array into a marker, or a marker into a span; a dynamic span is refused at the
  marker, with the fixed-size span to make instead.

### D3. The valid pairs

| Scheme | Key pair and public key | Message | `prehashed{digest}` |
|---|---|---|---|
| `eddsa` | Ed25519: `curves::ed25519::basic_keypair<B>`, 32 bytes | signed as given (PureEdDSA) | refused |
| `ecdsa_sha256` | P-256: `basic_keypair<B>`, `basic_public_key<B>` | SHA-256 inside | 32 bytes |
| `ecdsa_sha256` | secp256k1: `basic_keypair<B>`, `public_key` | SHA-256 inside | 32 bytes |
| `ecdsa_sha256_recoverable` | secp256k1: `basic_keypair<B>` (`recover` gives the key) | SHA-256 inside | 32 bytes |

ECDSA's signing gives low S on both curves (s at most (n - 1) / 2): libsecp256k1 signs low S,
and P-256's RFC 6979 signature is normalized after signing (its s is public once signed, and is
declassified before the comparison for the constant-time lane). Verification differs on purpose:
secp256k1 refuses a high S (libsecp256k1's rule, Bitcoin's policy), P-256 accepts both (FIPS
186-5 and WebCrypto, whose own signatures may be high). A protocol that requires low S from a
P-256 peer checks `is_low_s` before `verify`.
| `schnorr` | secp256k1: `basic_keypair<B>`, 32-byte x-only key | signed as given (BIP340) | refused |

Every other pair stops the build: a scheme the key's curve does not have, `eddsa` or `schnorr`
with a marker, a marker whose N is not 32, a marker on a backend that cannot sign a digest. The
message form of ECDSA hashes with webcpp::hash's `sha2_256` inside the public function's timing
guard. The signature is 64 bytes for every scheme, 65 for `ecdsa_sha256_recoverable` (r, s,
the recovery id).

### D4. The synchronous calls

- **Members**: `key.sign(scheme, input)` on every key pair, returning
  `result<std::array<std::uint8_t, N>>`, N the scheme's signature size. secp256k1 adds two
  overloads that keep a reason of their own: `sign(ecdsa_sha256, input, extra_entropy)` (hedged
  ECDSA, libsecp256k1's `ndata`, 32 bytes) and `sign(schnorr, message, aux_random)` (BIP340's
  auxiliary randomness, the caller's 32 bytes); `sign(schnorr, message)` draws those 32 bytes
  from `random_device`, as BIP340 recommends, and returns `entropy_unavailable` when it cannot.
- **Free**: `crypto::verify(scheme, signature, input, public_key)`, in namespace
  `webcpp::crypto`, returning `result<void>`. The public key's type says the curve: Ed25519's is
  its 32 bytes, BIP340's the 32-byte x-only key, and ECDSA's a validated key object on both
  curves (`p256::basic_public_key<B>`, `secp256k1::public_key` from `parse_public_key`), so that
  `ecdsa_sha256` never infers its curve from a number of bytes. Ed25519's verification policy and
  backend stay template parameters: `crypto::verify<curves::ed25519::strict, backends::sodium>
  (eddsa, ...)`.
- **Recovery**: `curves::secp256k1::recover(ecdsa_sha256_recoverable, signature, input)`, the same
  inputs as signing.
- **Ed25519ph**: the incremental signer `ph` stays; its `verify(signature, public_key)` takes the
  signature first, as every `verify` does.

Every pair that is not valid stops the build in a `static_assert` whose message names the key's
curve and the schemes it has, or the scheme and why it takes no digest. Every public function that
calls a backend holds `detail::timing_guard` first and joins its backend's coverage test; the
first guard a call enters is its own (the data-independent-timing change's D3 as built).

### D4b. The signature's format helpers

DER, low S and the 64-byte form are formats of a signature, not schemes, and need the curve's
order: they stay in each curve's namespace with the same names on both curves. `to_der(signature,
output)` returns the length written (`result<std::size_t>`, allocation-free, as both curves had
it), `from_der(der)` returns `result<std::array<std::uint8_t, 64>>`, `normalize_low_s(signature)`
returns `result<std::array<std::uint8_t, 64>>` (`invalid_signature` for an r or an s that is 0 or
not below n), and `is_low_s(signature)` returns `bool`, true exactly when 0 < r < n and
0 < s <= (n - 1) / 2. `normalize_ecdsa` (both curves) and P-256's `check_low_s` are renamed; the
recoverable signature keeps its id and `recover`.

### D4c. Bitcoin's sighash

A Bitcoin legacy or segwit v0 sighash is SHA-256d of the transaction's serialization, a protocol's
digest: crypto holds primitives only (foundation D24), so a program computes it with webcpp::hash
and signs `prehashed{sighash}` under `ecdsa_sha256`, which is honest, the sighash being the SHA-256
of a SHA-256. No `ecdsa_sha256d` scheme exists. The new example `bitcoin_sighash.cpp` shows it; the
Schnorr side (a taproot sighash, a BIP340 tagged hash, passed as the message to `schnorr`) is in
`musig_taproot.cpp` and `taproot_tweak.cpp`.

### D5. The asynchronous family

`async_sign(executor, key, scheme, input, token)` and `async_verify(executor, scheme, signature,
input, public_key, token)`, for Ed25519 (`eddsa`, its key 32 bytes, its `Policy` and `Backend`
template parameters kept) and P-256 (`ecdsa_sha256`, its key a `basic_public_key<Backend>`), with
the synchronous call's inputs and its completion signatures. Each checks `can_sign` or
`can_verify` in a `static_assert` before anything else, after the existing refusal of an
algorithm WebCrypto lacks.

`detail::signs_prehashed_v<Backend>` is `!detail::asynchronous_backend_v<Backend>`: WebCrypto's
`sign` and `verify` take the message and a hash name, with no prehash mode, so the browser's
backend of part B refuses a marker at compile time. The traits match the key types by partial
specialization (`basic_keypair<Backend>`, `basic_public_key<Backend>`), so that checking a
concept never instantiates a key pair's class.

secp256k1 stays out: the asynchronous family's rule (foundation D23) is the algorithms WebCrypto
has; the message test `async_sign_secp256k1` keeps its refusal.

### D6. What is removed

Ed25519's `basic_keypair::sign(message)` and free `curves::ed25519::verify`; P-256's
`basic_keypair::sign_ecdsa` and free `verify_ecdsa`; secp256k1's `basic_keypair::sign_ecdsa`
(both overloads), `sign_schnorr` and `sign_recoverable`, and the free `verify_ecdsa`,
`verify_schnorr` and the old `recover(signature, hash)`. No alias or deprecated shim remains.
`to_der`, `from_der`, ECDH, the key formats and MuSig2 stay; `normalize_ecdsa` and `check_low_s` are renamed (D4b).

### D7. Callers and generated files

Every caller in libs/crypto moves in the same change: tests, examples (`musig_network`,
`musig_taproot` and the others), the page, `doc/tables/crypto.json` and the files it generates
(rerun, never edited), the constant-time harnesses and their suppressions, the benches, the
message tests and the MSVC tests. Measured data (`doc/tables/sizes.json`,
`doc/performance.json`) is never edited by hand; the size programs and bench cases keep their
names and measure the same operation (the digest form where they measured a digest), and their
numbers are measured again in the final quiet pass the data-independent-timing change left open.

## Rulings

- **R1 (owner, 2026-11-22): secp256k1 is in this change.** It supersedes the coordinator's first
  scope ruling of the same day, which kept secp256k1's scheme-named API out because one key signs
  two schemes and its ECDSA digest is not bound to SHA-256 (Bitcoin's sighash). Both reasons are
  answered: every call names its scheme, and a sighash, SHA-256d, is a SHA-256 digest.
- **R2 (owner, 2026-11-22): the scheme is always required.** No scheme-less `sign(input)` or
  `verify(signature, input, key)` exists on any curve; this supersedes the intermediate shape of
  the same day (scheme-less forms for single-scheme curves, `prehashed<Hash>`, a
  `protocol_digest<N>` marker), which never reached code.
- **R3: the free `verify` is `crypto::verify`,** in the library's namespace, as the owner wrote it;
  overloads per scheme and public key type, from each curve's header. Ed25519's free `verify`
  leaves `curves::ed25519`.
- **R4: ECDSA's public key is a validated object on both curves.** secp256k1's ECDSA verification
  took SEC 1 bytes; it now takes the `secp256k1::public_key` that `parse_public_key` makes, so that
  raw bytes never decide between P-256 and secp256k1 under the one tag `ecdsa_sha256`.
- **R5: secp256k1's Schnorr without auxiliary randomness draws it** from `random_device`, and an
  overload takes the caller's 32 bytes (test vectors, programs that draw their own); hedged ECDSA
  keeps its explicit entropy as a third argument. Both stay `sign`.
- **R6: `recover` takes the scheme and the same inputs as signing,**
  `recover(ecdsa_sha256_recoverable, signature, input)`, so that the 32-byte hazard leaves
  secp256k1 entirely; its name stays.
- **R7: MuSig2 and silent payments keep their names.** `partial_sign`,
  `partial_signature_verify` and `partial_signature_aggregate` are steps of a protocol session,
  not a key pair's signature, none is called `sign`, and none clashes; the aggregate signature is
  BIP340's and is checked with `crypto::verify(schnorr, ...)`. Silent payments sign nothing.
- **R8: secp256k1 stays out of the asynchronous family** (D5): adding it would change the family's
  rule, which this change does not.
- **R9: the marker deduces from a Boost.Hash2 digest too,** so that `prehashed{hasher.result()}`
  reads as it is meant; like every span, the marker refers to its digest and must not outlive it.
- **R10: no browser backend exists yet** (part B); its refusal is the trait of D5, tested with a
  stand-in backend whose `asynchronous_backend_v` is true.
- **R11: Ed25519ph's `ph::verify` takes the signature first,** for the one order of D4; `ph` is
  not given a tag in this change.
- **R12 (owner, 2026-11-22): ECDSA signs low S on both curves; verification keeps each curve's
  policy** (D3); the format helpers get one set of names (D4b); the sighash stays out (D4c).
- **R13: `to_der` keeps its output span** on both curves (`to_der(signature, output)`), the
  owner's `to_der(sig)` read as the name: it allocates nothing and both curves already had it.
  `is_low_s` returns `bool`, its name being a question; an r or an s out of range answers false.

## Risks / Trade-offs

- Ethereum signs Keccak-256 digests with recoverable ECDSA. Under `ecdsa_sha256_recoverable` a
  Keccak-256 digest is a 32-byte marker the scheme cannot tell from a SHA-256 one: it signs and
  recovers correctly, and the tag's name says SHA-256. A tag `ecdsa_keccak256_recoverable` is the
  honest name; it is an open item, not part of this change.
- An out-of-tree caller of the removed names stops compiling: webcpp::trystero's port of Trystero
  0.27 is the only known one, and it adapts after the merge.
- Hashing inside the call adds SHA-256's cost to the message form, which WebCrypto pays too.

## Migration Plan

One change on the branches `crypto/sign-unified` of webcpporg/crypto, webcpporg/proposals and the
superproject; local verification on every lane; merged by the controller; one CI run at the end by
the coordinator.

## Open Questions

- Whether to add `ecdsa_keccak256_recoverable` for Ethereum (Risks).
