# webcpp::webrtc: where the proposal stands, and how to resume

Written 2026-11-14, when the draft of 2026-10-13 was brought up to date and given its plan and
its reference code. It was proposed the same day on webcpporg/proposals#12 (tasks.md 0.8), and
the owner accepted it that day: its status is `accepted`, with `decided: 2026-11-14` and
`release: null`.

## What exists

- `spec.md`: the original specification of 2026-10-10, kept as written. `../design.md` (revised
  2026-11-14) supersedes it where they differ, and lists every difference in "What changed since
  the specification".
- `plan.md`: the implementation plan, in three increments, one task per step.
- `source/`: the reference code of increment 1, the library's tree as its repository will start:
  `build.jam`, `meta/libraries.json`, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`, the public
  headers (`webrtc.hpp`, `config.hpp`, `error.hpp`, `types.hpp`, `options.hpp`, `runtime.hpp`,
  `data_channel.hpp`, `peer_connection.hpp`), the platform-free machinery (`detail/operation.hpp`,
  `detail/events.hpp`), the native backend over libdatachannel's C API
  (`detail/native/capi.hpp`, `channel.hpp`, `connection.hpp`), and the tests (`test/`).
- The owner's decisions of 2026-11-14: a library of its own, not inside webcpp::beast
  (design.md, D1); libdatachannel's MPL-2.0 accepted (D16); the native backend on libdatachannel's
  C API, so that the library works with exceptions on and off (D4).

## Built, run and measured, 2026-11-14

On macOS 27.0 (arm64), Apple clang 21.0.0, Boost 1.92.0, b2 5.5.3, in a worktree of the
superproject at `eb08198` (`origin/main`: asio `6c4e2c8`, beast `6c53f85`, cobalt `5d51c4a`,
trystero `e9326e9`, libdatachannel `6b1e2e6`, OpenSSL `45e844f`), with `source/` linked as
`libs/webrtc`:

- **The provider.** `b2 ... threading=multi /webcpp/third_party//libdatachannel` built OpenSSL and
  libdatachannel's static archives (libdatachannel itself in 13.8 s).
- **The suite, from scratch.** `b2 --user-config=.local/user-config.jam -a -j10 threading=multi
  libs/webrtc/test`: 25 targets, all `**passed**`: `config`, `config_all_off`,
  `config_developer_defined`, `error`, `types`, `events`, `events_no_exceptions`, `native`,
  `native_no_exceptions`, `allocation`, `lifetime`, `stall`, and the 13 headers-alone units, in
  about 70 s.
- **Without exceptions.** `native_no_exceptions` compiles its translation unit with
  `-fno-exceptions` and links the archives the superproject builds with `-fexceptions`; it passes
  whole. Before the reference code, a probe of 60 lines built with `-fno-exceptions` against the
  same pinned libdatachannel connected two peers over the loopback through the C API, received
  "hello", and got `RTC_ERR_INVALID` (-1) for the ICE server URL `stun:`, with no exception
  (design.md, D4).
- **ThreadSanitizer.** `b2 ... threading=multi thread-sanitizer=on
  define=WEBCPP_TEST_WEBRTC_REPEAT=100 libs/webrtc/test//lifetime libs/webrtc/test//native`: both
  passed, no report, 100 rounds of destruction while the peer floods. libdatachannel's archives
  are not instrumented: what it checks is this library's side of each hand-off.
- **A never-read channel stalls the others** (tasks.md 1.10): 3,000 messages on a channel never
  read, then one on a second channel of the same connection: it did not arrive in 3 s; destroying
  the first channel's far end released it, and that destruction returned in under 2 s
  (`test/stall_test.cpp`, `test/lifetime_test.cpp`).
- **The lint, as far as one library goes.** wasi-sdk 34's `clang-format` (23.1) with the
  superproject's `.clang-format`: every file formatted. Its `clang-tidy` with the superproject's
  `.clang-tidy` and `test/.clang-tidy` (asio's), over a compilation database of the suite's 25
  translation units from b2's dry run (the native commands with `--target=arm64-apple-macosx15.0`,
  the SDK's sysroot and libc++), and once more over the aggregate unit with `-fno-exceptions`: no
  finding. The whole `tools/lint/lint.sh` was not run: it lints the whole tree.
- **Re-verified in libdatachannel at `6b1e2e6`:** `RECV_QUEUE_LIMIT` 1,024 and
  `DEFAULT_LOCAL_MAX_MESSAGE_SIZE` 256 KiB (`src/impl/internals.hpp:41-46`); the queue's push
  waits for room (`src/impl/queue.hpp:93`); `DataChannel::close()` does not stop the queue
  (`src/impl/datachannel.cpp:97-113`) and a closed channel queues nothing new (`incoming`); the C
  API's `wrap()` (`src/capi.cpp:217-228`); `rtcDeletePeerConnection` and `rtcDelete` block until
  the callbacks in flight return (`DOC.md`); `setLocalDescription` sets the description before it
  returns (`src/peerconnection.cpp:83-160`); a new channel inherits its connection's user pointer
  (`rtcCreateDataChannelEx`, `rtcSetDataChannelCallback`), which the backend replaces at once.

## What remains

- tasks.md 1.11: the C API's cost on receipt, measured against the C++ API's `receive()`.
- tasks.md 1.12: the documentation (the library's page, Doc Comments checked by MrDocs).
- tasks.md 1.13: the repository `webcpporg/webrtc` from one import commit of `source/`, the
  superproject's change (submodule, CI lanes; webrtc moved from chapter 17 to chapter 15 of
  `CODING-GUIDELINES.md`), the whole lint, the review. The libdatachannel target's refusal of
  `threading=single` already names libdatachannel's own threads, for every consumer
  (webcpporg/webcpp#15, merged 2026-11-14).
- Increments 2 and 3 (tasks.md groups 2 and 3).
- Open questions O1 to O3 (design.md).

The superproject's `CODING-GUIDELINES.md` summarizes this revision since webcpporg/webcpp#15;
webcpporg/webcpp#16 moves it from chapter 18 to chapter 17, accepted and not built.

## How to resume

1. Read `../design.md` whole, then `plan.md` and `../tasks.md`; `spec.md` only for history.
2. Make a worktree of the superproject from `origin/main`; initialize `libs/config`, `libs/asio`
   and, recursively, `third_party/libs/libdatachannel` and `third_party/libs/openssl`; set up
   `.local/` as the superproject's `AGENTS.md` chapter 1 says; link this `source/` as
   `libs/webrtc`.
3. Run the commands above; expect the same 25 passes.
4. Go on with tasks.md 1.11, then 1.12 and 1.13 once the owner creates the repository (the
   proposal is accepted); then increment 2, then trystero's change.
