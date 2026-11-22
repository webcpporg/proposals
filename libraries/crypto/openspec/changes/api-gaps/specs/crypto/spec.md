## ADDED Requirements

### Requirement: GCM: a failed seal zeroes data
When the backend fails after `seal`'s guards (`engine::start`, `engine::run` or `engine::tag`), `gcm::seal` SHALL zero the whole of `data` through `volatile` stores before it returns the error, on every backend, as `aead::seal` does; `update` SHALL zero its piece when the backend fails. A guard's refusal MUST leave `data` untouched.

#### Scenario: The tag's computation fails
- **WHEN** `seal` runs on the mock backend with its tag call failing, over 16 bytes of 0x5A
- **THEN** it returns `backend_failure`, `data` is all zero, and the message is abandoned through the engine

#### Scenario: A guard refuses
- **WHEN** `seal` is given an IV of 129 bytes over 16 bytes of 0x5A
- **THEN** it returns `invalid_iv_size` and `data` still holds 0x5A

### Requirement: GCM: the incremental open, in two passes
`gcm` SHALL offer `begin_open(iv, additional_data)`, `authenticate(ciphertext_piece)` over a read-only span, `verify(tag)`, `decrypt(piece)` in place and `end_open()`, each returning `result<void>`. `authenticate` MUST NOT write the caller's bytes; `decrypt` MUST be refused with `invalid_state` until `verify` has matched, so that no byte of plaintext reaches the caller before the tag is verified. The pieces given to `decrypt` MUST NOT sum to more than those given to `authenticate` (`invalid_input_size`, the piece untouched), and `end_open` MUST return `invalid_input_size` when they sum to less. Every error MUST end the message; a backend's failure in `decrypt` zeroes its piece. `begin_open` MUST check `begin_seal`'s guards, `verify` a tag of 16 bytes (`invalid_tag_size`), and a message longer than `max_message_size` summed over `authenticate` MUST give `message_too_long`. A one-shot `seal` or `open` during an incremental open MUST give `invalid_state` and leave it going; a move abandons it. The documentation MUST say that the two passes must read the same ciphertext, that a stream which cannot be read twice is sealed in records, and that the open costs about two one-shot opens.

#### Scenario: Pieces of every size
- **WHEN** a message sealed one shot is authenticated in pieces of 1 byte up to the whole, verified, then decrypted in pieces of other sizes
- **THEN** `end_open` succeeds and the plaintext equals the sealed message, on BearSSL, OpenSSL and the mock backend

#### Scenario: A wrong tag releases nothing
- **WHEN** every piece is authenticated and `verify` is given the tag with one bit changed
- **THEN** it returns `authentication_failed`, `decrypt` then returns `invalid_state`, and the caller's ciphertext is unchanged

#### Scenario: Decrypt before verify
- **WHEN** `decrypt` is called after `authenticate` and before `verify`
- **THEN** it returns `invalid_state`, the piece is untouched, and the message has ended

### Requirement: secp256k1: a key pair generated
`curves::secp256k1::basic_keypair` SHALL offer `static result<basic_keypair> generate(std::pmr::memory_resource* = std::pmr::get_default_resource())`, which draws a 32-byte secret key from `random_device` into a `secret<32>` and makes the key pair from it; 32 bytes that are not a secret key MUST be drawn again, at most four draws in all, after which, and on any failure of the generator, it returns `entropy_unavailable`.

#### Scenario: The first draw is not a secret key
- **WHEN** the test seam's source answers 32 bytes of 0xFF, then 32 bytes of 0x01, then the context's seed
- **THEN** `generate` returns the key pair of the secret key of 32 bytes 0x01

#### Scenario: The generator fails
- **WHEN** the test seam's source fails
- **THEN** `generate` returns `entropy_unavailable` and allocates nothing

### Requirement: secp256k1: the tweaks share one shape
`pubkey_negate` SHALL return `result<public_key>`, `noexcept`, returning `backend_failure` should libsecp256k1 refuse a parsed key, as `seckey_tweak_add`, `seckey_tweak_mul`, `seckey_negate`, `pubkey_tweak_add`, `pubkey_tweak_mul` and `pubkey_combine` return `result<>`. `basic_keypair::ecdh`, `ecdh_x` and `ecdh_point` SHALL return `result<secret<32>>`, `result<secret<32>>` and `result<secret<65>>`, pinned by a compile-time check.

#### Scenario: Negation twice
- **WHEN** a parsed key P is negated twice
- **THEN** both results hold values and the second equals P

### Requirement: Constant time: dudect's evidence and its A/A control
Each dudect program SHALL print, per case, `dudect case=<name> t=<value> threshold=10 judged=<yes|no>`, and for a case whose |t| is at least 10 the counts and means of both classes at the cropping that gave its largest |t|, that cropping, and the low 12 bits of the addresses of the buffers the case measures; each program SHALL print once the CPU's model and microcode where the system tells them and the clock's measured resolution. Each program except `dudect_planted` SHALL measure an A/A control case, its classes prepared by the same instructions with the same value, judged as every case. Each program SHALL accept `--case <name>` to measure that case alone.

#### Scenario: An A/A control on a sound machine
- **WHEN** `dudect_bearssl` runs natively on a machine of one's own
- **THEN** it prints `dudect case=chacha_open_aa` with t below 10 and judges it

#### Scenario: One case alone
- **WHEN** `dudect_sodium --case x25519_agree` runs
- **THEN** it measures and prints that case alone, and `--case unknown` exits with 2 and lists the names

### Requirement: Constant time: dudect printed, warned and judged on hosted runners
The CI SHALL print every dudect program's output of the constant-time lane and emit a GitHub warning annotation for every case whose |t| is at least 10, `dudect_planted` excepted. On a runner GitHub hosts, a judged case whose |t| is at least 10 SHALL be measured again in two fresh processes, and the lane MUST fail exactly when at least two of the three values are at least 10, naming the case and its three values. On a machine of one's own the lane SHALL judge each program as before, in one process.

#### Scenario: One noisy process
- **WHEN** on a hosted runner `chacha_open` reads 69.8 and its two fresh runs read 3.1 and 2.7
- **THEN** a warning annotation shows the 69.8 with its evidence, and the lane passes

#### Scenario: A persistent value
- **WHEN** on a hosted runner `equal` reads 14.0, then 12.5 and 3.0 in its fresh runs
- **THEN** the lane fails, naming `equal` and the three values

## MODIFIED Requirements

### Requirement: Block cipher: BearSSL has none, and aes256 has no default of its own
`aes256<backends::bearssl>::make` SHALL fail to compile with the `static_assert` text "webcpp::crypto: ciphers::aes256<backends::bearssl>: BearSSL exports AES only inside its modes, with no block cipher. Name a backend that exports it, as in ciphers::aes256<backends::ctaes> or ciphers::aes256<backends::openssl>, and add its library to the program's Jamfile." The block cipher SHALL have a default backend of its own per target, OpenSSL natively and ctaes on wasip2, wasip3 and Emscripten, which `aes128<>`, `aes192<>` and `aes256<>` name through the tag `backends::family_default`; `gcm<aes256<>>`, `aes128_gcm` and `aes256_gcm` MUST still name AES-GCM's default, since each family resolves the tag to its own default.

#### Scenario: make on BearSSL
- **WHEN** a program calls `aes256<backends::bearssl>::make(key)`
- **THEN** compilation fails with exactly that text, from a test in a directory of its own

#### Scenario: The block cipher's default on WebAssembly
- **WHEN** a wasip2 program that links ctaes and BearSSL calls `aes256<>::make` and `aes256_gcm::make`
- **THEN** the first is ctaes's block cipher and the second BearSSL's AES-GCM, and both compile

### Requirement: GCM: no incremental open
`gcm` SHALL NOT offer a decryption that returns any plaintext before the tag is verified: its incremental open authenticates every piece first and decrypts only after `verify` matched (the requirement "GCM: the incremental open, in two passes"). The incremental seal takes the IV and the additional data at its start, because Botan's public interface takes the associated data whole, before the nonce.

#### Scenario: What the incremental seal produced
- **WHEN** a message sealed with `begin_seal`, `update` and `result` is received
- **THEN** it is opened by the one-shot `open`, or by `begin_open`, `authenticate`, `verify`, `decrypt` and `end_open`

### Requirement: Default backend: one per family and target, by a stated reason
Each family SHALL have one default backend per target, `default_backend_t<Family>`, written with its reason in the family's row of `doc/tables/crypto.json` and never changed by the backends a program links: AES-GCM OpenSSL natively and BearSSL on wasip2, wasip3 and Emscripten; the AES block cipher OpenSSL natively and ctaes on wasip2, wasip3 and Emscripten; the ChaCha20-Poly1305 AEADs, Ed25519, X25519, the boxes, secretbox, secretstream and the rest of libsodium's families libsodium; secp256k1 libsecp256k1; the P-curves and RSA BearSSL; AES-CTR, CBC, CCM and EAX BearSSL until OpenSSL has them, then AES-GCM's rule; the families only OpenSSL and Botan have OpenSSL on native, wasip2 and wasip3 and Botan on Emscripten; BLS12-381, KZG, BN254 and Shamir their only backend (`design.md` D32). A program that uses a default it has not linked MUST stop at compile time with a message that names the Jamfile line linking it and the type that names another backend, from a test of its own. The documentation MUST give each default with its reason.

#### Scenario: Ed25519 without libsodium
- **WHEN** a program links only BearSSL and uses `curves::ed25519::keypair`
- **THEN** compilation stops naming `/third_party//sodium` and `curves::ed25519::basic_keypair<Backend>` with the backends that have Ed25519

#### Scenario: A native program links both AES-GCM backends
- **WHEN** a native program links OpenSSL and BearSSL and uses `ciphers::aes256_gcm`
- **THEN** its backend is OpenSSL, and `ciphers::gcm<ciphers::aes256<backends::bearssl>>` is BearSSL

#### Scenario: The block cipher's default not linked
- **WHEN** a native program links ctaes alone and calls `ciphers::aes256<>::make`
- **THEN** compilation stops naming `/webcpp/crypto//aes_block-default` and `ciphers::aes256<backends::ctaes>`
