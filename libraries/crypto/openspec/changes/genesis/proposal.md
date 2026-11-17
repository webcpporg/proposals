## Why

A C++ program on the web needs cryptography that Boost does not provide: authenticated
encryption of messages, a random number source it can trust, a constant-time comparison of tags,
and secp256k1 signatures. Boost.Hash2 covers hashing and HMAC, and nothing else. Every program
of webcpp that needs the rest (webcpp::trystero seals its signalling messages with AES-GCM and
signs Nostr events with BIP340 Schnorr) has so far carried its own code: a sealer for the
native build, another for the browser, a call to the browser's WebCrypto behind a promise, an
external libsecp256k1 and a direct use of OpenSSL.

webcpp needs one library that runs the same C++ on every target it has (native on Linux, macOS
and Windows, wasip2, wasip3 and Emscripten), whose algorithms are the proven C libraries
themselves and not webcpp's own port of them, whose errors are values, that allocates nothing
in the program's way, and that stops a wrong build with an instruction instead of a linker
puzzle. The owner decided its shape on 2026-10-10 and 2026-10-11, approved its specification on
2026-10-11, and it has a plan of 43 tasks in five increments. On 2026-11-14 the owner set the
first version, 0.1, to increment 1 without its heaviest parts, and increment 2, and the plan was
revised against the superproject of that day (`design.md` D22). Version 0.1 is merged
(`webcpporg/crypto` `main` at `d242be8`, recorded by the superproject's `main`). On 2026-11-17
the owner decided that increments 3 to 5 are built without trystero's migration, beside the
foundation change in one order (`design.md` D23, and the foundation's `reference/roadmap.md`).

## What Changes

- **New library `webcpp::crypto`**, a wrapper (webcpp's interface over other libraries),
  header-only C++20, with five increments:
  1. the foundation and AES-GCM on BearSSL and OpenSSL: `config.hpp`, `error`, `equal`,
     `random_device`, `aes128/192/256`, `gcm<Cipher>` (one shot and an incremental seal),
     OpenSSL's memory functions as an option of the config, the C libraries' build, the three
     layers that stop a wrong build, the tests, sizes and throughput, the documentation;
  2. libsecp256k1 and ctaes: `secp256k1::keypair`, `verify_schnorr`, `verify_ecdsa`, ECDH, and
     the AES block cipher on ctaes;
  3. OpenSSL's memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake, in
     crypto's own tests (trystero's migration, the increment's first purpose, dropped by the
     owner on 2026-11-17);
  4. ChaCha20-Poly1305 on BearSSL and OpenSSL, built in the foundation's increment 1 over its
     AEAD template;
  5. Botan, with exceptions only, built in the foundation's increment 7 as a pinned submodule
     (it was to be found on the machine, native only).
- **The interface is shaped as Boost.Hash2's:** algorithm types with `static constexpr` sizes,
  modes as templates over a cipher (`gcm<aes256<>>`), tags as `boost::hash2::digest<16>`, and
  the backend a template parameter whose default `config.hpp` chooses per target. Where Hash2
  cannot be followed, a keyed object is made by `static make(...)`, which returns
  `boost::system::result<T>`.
- **Backends are the C libraries themselves** (BearSSL, OpenSSL, ctaes, libsecp256k1) and
  Botan, never a port. BearSSL, libsecp256k1 and ctaes are built from their own sources, file
  by file, by the superproject's `third_party/Jamfile`; OpenSSL is the superproject's
  source-built provider (OpenSSL 3.5.9, static), which a program links through
  `/webcpp/crypto//openssl`; Botan is found on the machine. A program names
  `/webcpp/crypto//crypto` and its backend's target in its own Jamfile.
- **A wrong build stops with an instruction,** in three layers: a compile-time `#error` or
  `static_assert` that says exactly what to add, a link-time canary symbol whose name is the
  instruction, and `#pragma detect_mismatch` on MSVC.
- **The default backend is the smallest binary on the web and the fastest on servers:** BearSSL
  on WebAssembly, OpenSSL natively when linked, BearSSL natively otherwise.
- **Entropy always takes the safest path:** one source per platform (`getentropy`, or
  `BCryptGenRandom` on Windows), no generator in user space, no weaker fallback.
- **OpenSSL's memory functions are an option of the config:** with
  `WEBCPP_CRYPTO_OPENSSL_MEMORY_RESOURCE` a program names a `std::pmr::memory_resource` and
  every allocation of OpenSSL in the process goes to it. Without the option OpenSSL's and
  Botan's `make` allocate outside the program's allocator, a departure the owner accepted.
- **webcpp::trystero** names `/third_party//secp256k1` since version 0.1 (its build alone);
  its sealer, entropy and signer are not changed by this library's work (the owner's decision
  of 2026-11-17).
- **The superproject gains** the second project id `/third_party`, the incidental feature
  `webcpp-own`, `webcpp.external-root` and `webcpp.external-looked`, the rule
  `webcpp.same-program`, a Python test of the tools that builds the user's Jamfiles as fixtures,
  lint rules on the entropy header and the canary's pins, NIST's test archives pinned by URL and
  SHA-256, and the nightly level of the CI's own lanes.

## Capabilities

### New Capabilities

- `crypto`: webcpp::crypto, authenticated encryption (AES-GCM, ChaCha20-Poly1305), the AES
  block cipher, a `random_device`, a constant-time `equal` and secp256k1 (BIP340, ECDSA, ECDH)
  over C backends, with its interface, error values, backends, build, default backend per
  target, entropy rule, OpenSSL memory option, tests, documentation and increments.

### Modified Capabilities

None.

## Impact

- **Kind:** a wrapper (`"port-of": null`): webcpp's interface over BearSSL, OpenSSL, ctaes,
  libsecp256k1 and Botan, none of which is ported. Repository `webcpporg/crypto` (not created;
  the owner creates it, plan gate G1), checked out as the submodule `libs/crypto` (gate G2),
  BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** version 0.1 (the plan's Tasks 0 to 14, 17 to 19, 21, 24 to 30a,
  `design.md` D22) is merged into `webcpporg/crypto`'s `main` (`d242be8`) and recorded by the
  superproject's `main`; it is recorded as built once the CI is green on both (task 7.3). What
  remains is the memory option (Tasks 15, 16, 33 in crypto's tests, 34), the owner-gated Task
  25b and NIST CAVP; Tasks 20, 22, 23, 35, 38 and 39 are built by the foundation change, and
  Tasks 31, 32 and 37 are not built (`design.md` D23).
  **The reference code is the code written in the tasks of `reference/plan.md`:** Task 1
  (the skeleton, `build.jam`, the test Jamfile's `crypto-run`), Task 2 (`error.hpp`), Task 3
  (`equal.hpp`, `detail/wipe.hpp`), Task 4 (`random_device.hpp`, `detail/entropy.hpp`,
  `detail/windows.hpp`), Task 5 (`third_party/Jamfile` for BearSSL, `canary_bearssl.c`,
  `detail/canary.h`), Task 6 (`config.hpp`, `backends/bearssl.hpp`, the message tests), Tasks 7
  and 8 (`aes.hpp`, `gcm.hpp`), Task 12 (`backends/openssl.hpp`), Task 14 (`aes256_gcm.hpp`),
  Task 15 (the memory adapter), Task 27 (`secp256k1.hpp`), Task 29 (`backends/ctaes.hpp`), Task
  35 (`chacha20_poly1305.hpp`) and Task 38 (`backends/botan.hpp`), with the tests, Jamfiles and
  tools the other tasks give. `reference/spec.md` is the full specification and
  `reference/STATUS.md` says where things stand and how to resume.
- **Dependencies:** Boost 1.92 (System, Hash2, Core's `lightweight_test`, JSON in tests);
  BearSSL at `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` from the mirror `webcpporg/bearssl`
  (MIT); libsecp256k1 v0.8.0 (MIT); ctaes as Bitcoin Core v31.1 ships it (MIT); OpenSSL 3.5.9,
  the superproject's source-built provider (Apache-2.0); Botan 3 (3.13.0 measured), found on the machine
  (BSD-2-Clause); for tests, Wycheproof, BIP340's test vectors, NIST CAVP's archives, Valgrind,
  MemorySanitizer, wasmtime, node and Chrome. The superproject's `tools/webcpp.jam`,
  `third_party/Jamfile` and the asio track's test support (`webcpp::test::exit_status`).
- **Status:** accepted on 2026-10-11 (the owner approved the specification), revised on
  2026-11-14 for version 0.1 and on 2026-11-17 for increments 3 to 5; implementing, version 0.1
  merged, no release.
- **Other proposals:** `libraries/trystero` links this library's libsecp256k1 build from
  version 0.1 (Task 30a), and nothing else of it is changed by this library's work;
  `libraries/crypto/openspec/changes/foundation` builds this change's increments 4 and 5 and
  Tasks 20, 22 and 23, and its `reference/roadmap.md` is the order of all the remaining work; `libraries/asio` shares the
  source-built OpenSSL with this library and supplies the test support every test here uses; `libraries/webrtc` and `libraries/beast` may use this
  library later; `organization` carries the superproject's rulebook and CI levels these tasks
  extend.
