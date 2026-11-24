# webcpp::secure: where the implementation stands, and what to start from

Written 2026-11-24, with the genesis. Nothing is built in secure's name yet. The owner created the
repository `webcpporg/secure` on 2026-11-24 and its submodule `libs/secure`. `prior/` holds the code
an implementer ports, exact copies of files committed elsewhere, none of them renamed yet:

| `prior/` | Copied from | What it is |
| --- | --- | --- |
| `config/include/webcpp/config/detail/timing_guard.hpp` | webcpporg/config, branch `config/timing-guard` at `ba72ad4` (local, never pushed) | The guard of config's withdrawn change `timing-guard`: crypto's guard with hash's `engaged` flag, the one seam, Windows on ARM64 (`_ReadStatusReg`, `_WriteStatusReg`, `__isb`, the SEH probe). Named `webcpp::config::detail` and `WEBCPP_CONFIG_DETAIL_*`; becomes `webcpp::secure` and `WEBCPP_SECURE_*` (design D2, D3). |
| `config/test/timing_guard_*_test.cpp` | the same commit | Its tests: plain, nesting, seam, threads, Windows. |
| `node-test/include/webcpp/test/timing_seam.hpp`, `timing_dit.hpp` | the superproject webcpporg/webcpp, branch `config/timing-guard` at `83c9a31` (local) | The seam recorder and the DIT helper that branch shared in `tools/node-test`. secure ports them into its own `test/support/` (design D9); the shared place is open (O1). |
| `crypto/include/webcpp/crypto/{erase_secret,constant_time_equal,secret}.hpp`, `detail/{wipe,declassify}.hpp` | webcpporg/crypto `main` at `3314a1d` | The public names that move to secure with no alias (design D4 to D7). |
| `crypto/test/{erase_secret,constant_time_equal,declassify_compile}_test.cpp` | the same commit | Their tests, which move with them. |

## The withdrawn change's verification, which the port inherits

- Natively (clang, this Mac): config 27 tests, hash 295, crypto 541 passed, 0 failed, with every
  guard site of hash (28) and crypto (187 in 26 headers) moved onto the shared guard.
- The Windows on ARM64 branch, once, on the owner's Windows 11 ARM64 VM: MSVC 14.3 and 14.5, arm64
  and x64, debug builds; config's tests and example passed with 0 known and 0 new failures on each
  of the four lanes; on arm64 `cpu_has_dit()` was true and DIT stayed set across sleeps inside a
  guard.
- Not run: a release build on the VM, clang-cl ARM64, a CPU without FEAT_DIT (the probe's filter
  path), crypto's dudect on Windows ARM64, gcc 15, WASI, Emscripten and the constant-time lane for
  the last tasks of that branch. Its measurement of the guard's cost was taken under load and is
  not reused (design D11).

The withdrawn branch's commits (config `ba72ad4`, hash `43b6732`, crypto `b0c1f4f`, superproject
`83c9a31`) are a worked example of moving every guard site of hash and crypto by a rename, for
their migrations, which the owner folded into this delivery on 2026-11-24 (tasks.md sections 7
and 8). The plan is `plan.md`, beside this file.
