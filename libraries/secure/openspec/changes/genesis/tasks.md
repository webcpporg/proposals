# Tasks

Each task writes its tests first; the library builds and its tests pass on every local lane at the
end of every task. A plan with code goes in `reference/plan.md` when the proposal is accepted. The
owner's gates come first: the repository `webcpporg/secure` and the submodule `libs/secure`
(chapter 10's checklist, items 1 and 2); until then the library is a local repository at
`libs/secure`. The prior code to port is in `reference/prior/` (`reference/STATUS.md`).

## 1. The library's skeleton

- [ ] 1.1 The repository's files (chapter 10's checklist, items 3 to 7): `meta/libraries.json` with `"port-of": null`, `build.jam` with `secure` (D1), `LICENSE_1_0.txt`, `README.md`, `AGENTS.md`, `.gitattributes`, `.gitignore`, `.github/workflows/ci.yml`, `test/Jamfile`, `example/Jamfile`, `doc/Jamfile` and `doc/changelog.adoc`; the index page's entry
- [ ] 1.2 `config.hpp` and its test three ways (exceptions, `WEBCPP_NO_EXCEPTIONS`, `WEBCPP_SECURE_NO_EXCEPTIONS`); `CODING-GUIDELINES.md` chapter 17's entry (D1)

## 2. The guard

- [ ] 2.1 `timing_guard.hpp` on Apple's systems and Linux, ported from `reference/prior/config/include/` with `WEBCPP_SECURE_*` names in `webcpp::secure`; secure's `test/support/timing_seam.hpp` and `timing_dit.hpp`; `timing_guard_test`, `_nesting_`, `_seam_`, `_threads_` and the header-alone test (D2, D9, D10)
- [ ] 2.2 Windows on ARM64: the intrinsics, the SEH probe, `timing_guard_windows_test`, run on the Windows ARM64 lane with MSVC 14.3 and 14.5, debug and release (D3, D11)

## 3. Memory and comparison

- [ ] 3.1 `detail/wipe.hpp` and `erase_secret.hpp`, moved from crypto with `erase_secret_test` (D4)
- [ ] 3.2 `secret.hpp`, moved from crypto, with `secret_test` (D6)
- [ ] 3.3 `constant_time_equal.hpp` over byte ranges, with `constant_time_equal_test` and its seam check (D5)
- [ ] 3.4 `declassify.hpp`, secure's `test/support/constant_time_marks.hpp`, `declassify_compile_test` (D7, D9)

## 4. The constant-time blocks

- [ ] 4.1 `constant_time.hpp`: `value_barrier`, the masks, `select`, `lookup`, `select_bytes` and the byte comparisons, with the exhaustive and random tests against a plain reference (D8, D10)
- [ ] 4.2 The allocation test over every function; the convenience header and its test (D1, D10)

## 5. The constant-time lane

- [ ] 5.1 The lane `constant-time`: memcheck with the operands undefined, MemorySanitizer where crypto runs it, dudect with its A/A control, the planted branching `select` and indexing `lookup` that must fail (D10)

## 6. The documentation and the delivery

- [ ] 6.1 The page: what secure is, the guard, erase, `secret<N>`, `constant_time_equal`, `declassify`, the blocks and their limits, the changelog; every example built and run; the guard's cost measured on release builds on an idle machine (D11)
- [ ] 6.2 Full local verification on every lane (native clang and gcc 15, MSVC 14.3 and 14.5, the Windows ARM64 lane, wasip2, wasip3, Emscripten, the constant-time lane, the Linux lint), exceptions on and off; the review; pull requests the owner merges; one CI at the end

Not tasks of this genesis, each a change of its own library after secure: hash's guard and wipe on
secure's; crypto's callers on secure's names, its copies deleted with no alias; and the place of
the shared test support, once the owner decides (design O1).
