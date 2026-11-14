---
title: "webcpp::webrtc: implementation plan"
tags: [webcpp, webrtc, asio, emscripten, plan]
status: active
created: 2026-11-14
---

# webcpp::webrtc Implementation Plan

**Goal.** Build webcpp::webrtc as `../design.md` decides it, in three increments: (1) the
interface, the `runtime` and the native implementation over libdatachannel's C API; (2) the
browser implementation over webcpp::asio's bridge, with the interop lane; (3) trystero on it, a
change of trystero's.

**Where it stands.** Increment 1 is written: `source/` is its tree, built and tested on
2026-11-14 against the superproject's pinned libdatachannel (`STATUS.md`). Its tasks below say
what each file is and how it was proved, and point to the file instead of repeating it. Increments
2 and 3 are to do; their tasks carry the interfaces, the files, the tests and the expected output.

**Architecture.**

- One public interface, `<webcpp/webrtc/...>`, whose class templates `basic_peer_connection` and
  `basic_data_channel` hold a `std::shared_ptr` to a backend state: `detail::native::*` natively,
  `detail::browser::*` on emscripten (increment 2). The facade owns the state; posted events hold
  it weakly.
- What waits is a `detail::pending<Values...>`, made by `detail::make_operation` in the handler's
  associated allocator, with two work guards and the handler's cancellation slot
  (`detail/operation.hpp`). What an event becomes while nothing waits is `detail::state_watch` and
  `detail::event_queue` (`detail/events.hpp`). Both are the same on both targets.
- Natively, libdatachannel's C API: an object is an `int` id with a user pointer to an inbox
  (`std::weak_ptr` to the state, the executor); every callback copies and posts; destruction calls
  `rtcDelete*`, which waits for the callbacks in flight.
- In the browser, one `intrusive_registration` of the io_context's bridge per object, `EM_ASM`
  imports that catch JavaScript's exceptions in JavaScript, arrivals staged and charged on
  arrival, one notification posted per batch (webcpp::beast's browser WebSocket, design.md D8).

**Tech stack.** C++20; b2 5.5.3; Boost 1.92 (System, Asio through webcpp::asio, Core
lightweight_test); webcpp::config, webcpp::asio; libdatachannel 0.24.6 at `6b1e2e6` with libjuice,
libsrtp, usrsctp and plog, and OpenSSL 3.5.9, all built by the superproject from
`third_party/`; Emscripten 6.0.11 and Node for increment 2, headless Chrome for its interop lane;
wasi-sdk 34's clang-format and clang-tidy for the lint.

## Global constraints

- **Where the code lives.** The repository `webcpporg/webrtc`, created by the owner once the
  proposal is accepted, starts from one import commit of `source/` (`CODING-GUIDELINES.md`
  chapter 2) and is checked out as the superproject's submodule `libs/webrtc`. Until then
  `source/` is built by linking it as `libs/webrtc` in a worktree of the superproject, never
  committed there (Task 1.1).
- **Rules.** `CODING-GUIDELINES.md` chapters 1 to 13, and the library's `source/AGENTS.md`. Every
  test is seen failing before the code that makes it pass, and only a build from scratch,
  `b2 -a`, counts as evidence.
- **Commits.** Under the committer's own identity, with explicit paths and no trailer:
  `git -c user.name=<name> -c user.email=<address> commit -- <paths>`; the subject is prefixed
  `webrtc:`.
- **Text.** English; no em dash; the licence notice of chapter 2 on every source file.
- **Commands.** From the superproject's root, with `.local/user-config.jam` as `AGENTS.md`
  chapter 1 sets it up and `CPATH`, `CPLUS_INCLUDE_PATH` and `C_INCLUDE_PATH` unset:

  ```
  b2 --user-config=.local/user-config.jam -a -j10 threading=multi libs/webrtc/test
  ```

## File structure

| Path | What it holds |
| --- | --- |
| `build.jam` | `/webcpp/webrtc//headers`, `/webcpp/webrtc//webrtc` (natively with `/webcpp/third_party//libdatachannel`), the feature `webcpp-webrtc-unsupported` |
| `meta/libraries.json` | the library's entry |
| `include/webcpp/webrtc.hpp` | the convenience header |
| `include/webcpp/webrtc/config.hpp` | `WEBCPP_WEBRTC_NO_EXCEPTIONS`, the unsupported policy, the `#error` under WASI |
| `include/webcpp/webrtc/error.hpp` | `error`, the category `webcpp.webrtc` |
| `include/webcpp/webrtc/types.hpp` | the enumerations, the input views, the owned outputs |
| `include/webcpp/webrtc/options.hpp` | `options`, `runtime_options` |
| `include/webcpp/webrtc/runtime.hpp` | `runtime`, `detail::runtime_slot` |
| `include/webcpp/webrtc/data_channel.hpp` | `basic_data_channel` |
| `include/webcpp/webrtc/peer_connection.hpp` | `basic_peer_connection` |
| `include/webcpp/webrtc/detail/operation.hpp` | `pending`, `operation`, `make_operation` |
| `include/webcpp/webrtc/detail/events.hpp` | `state_watch`, `event_queue` |
| `include/webcpp/webrtc/detail/native/capi.hpp` | the C API's include, codes, states, texts |
| `include/webcpp/webrtc/detail/native/channel.hpp` | `channel_state`, `read_message` |
| `include/webcpp/webrtc/detail/native/connection.hpp` | `connection_state` |
| `include/webcpp/webrtc/detail/browser/*.hpp` | increment 2: `imports.hpp`, `channel.hpp`, `connection.hpp`, `census.hpp` |
| `test/Jamfile`, `test/.clang-tidy` | the tests; the tests' clang-tidy options, asio's |
| `test/require.hpp`, `test/pair.hpp` | `require`; two peers over the loopback, signalled in memory |
| `test/*_test.cpp` | config, error, types, events, native, allocation, lifetime, stall |
| `doc/`, `bench/` | increment 1, Tasks 1.12 and 1.11: the library's page, the receive benchmark |

## Increment 1: the interface, the runtime and the native implementation

### Task 1.1: The repository and its build (tasks.md 1.1)

**Files:** `build.jam`, `meta/libraries.json`, `include/webcpp/webrtc/config.hpp`,
`test/Jamfile`, `test/config_test.cpp`, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`.

**Interfaces.** `/webcpp/webrtc//headers` (include directory, webcpp::config, webcpp::asio's
headers, the unsupported policy's define); `/webcpp/webrtc//webrtc` (adds webcpp::asio and,
natively, `/webcpp/third_party//libdatachannel`, whose usage requirements bring `RTC_STATIC`, the
include directory and the archives, and which refuses `threading=single`).

**Steps.**

1. Make a worktree of the superproject from `origin/main`, initialize `libs/config`, `libs/asio`
   and, recursively, `third_party/libs/libdatachannel` and `third_party/libs/openssl`, and link
   `source/` as `libs/webrtc` (the Jamroot finds a library by its `build.jam`).
2. Write `config_test.cpp` (three builds: as the build asks, `WEBCPP_NO_EXCEPTIONS`, the macro
   itself), see it fail without `config.hpp`, then write `config.hpp`.
3. Build the provider once: `b2 ... threading=multi /webcpp/third_party//libdatachannel`
   (OpenSSL then libdatachannel's static archives; 13.8 s for libdatachannel on 2026-11-14).

**Expected output.** `**passed**` for `config`, `config_all_off`, `config_developer_defined`.

### Task 1.2: The value types (tasks.md 1.2)

**Files:** `include/webcpp/webrtc/types.hpp`, `test/types_test.cpp`.

**Interfaces.** `sdp_type`, `peer_connection_state`, `ice_connection_state`,
`data_channel_state`, `ice_transport_policy`; `ice_server`, `configuration`,
`session_description_view`, `ice_candidate_view`, `data_channel_init` (every member with a `{}`
default member initializer, so that a designated initializer leaving one out never warns);
`basic_session_description<Allocator>` and `basic_ice_candidate<Allocator>` with `view()`;
`session_description`, `ice_candidate`.

**Tests.** Defaults; the enumerations' first value; a description's view round trip; a
candidate's view, an empty mid as no mid; a candidate's strings under a polymorphic allocator.

**Expected output.** `**passed** ... types.test`.

### Task 1.3: Options and errors (tasks.md 1.3)

**Files:** `include/webcpp/webrtc/options.hpp`, `include/webcpp/webrtc/error.hpp`,
`test/error_test.cpp`.

**Interfaces.** `options` (64, 16, 256 KiB, 16 MiB, 16 MiB), `runtime_options` (256, 0);
`enum class error` 1 to 10, `webrtc_category()` named `webcpp.webrtc`, `make_error_code`,
`boost::system::is_error_code_enum<error>`.

**Expected output.** `**passed** ... error.test`.

### Task 1.4: The event machinery, without a platform (tasks.md 1.4)

**Files:** `include/webcpp/webrtc/detail/operation.hpp`, `include/webcpp/webrtc/detail/events.hpp`,
`test/events_test.cpp`; the Jamfile builds it as the build asks and with
`<exception-handling>off`.

**Interfaces.**

```cpp
template <class... Values> class pending {          // completes once, then frees itself
public:
    void complete(error_code, Values...);            // posts the handler
    void fail(error_code);                           // posts it with the fallback values
    void discard() noexcept;                         // frees it without calling the handler
    void link(pending*& slot) noexcept;              // the slot it clears when it ends
};
template <class... Values, class Handler, class IoExecutor>
pending<Values...>& make_operation(Handler&&, const IoExecutor&, std::tuple<Values...> fallback,
                                   webcpp::asio::cancellation_type supported);
template <class State> class state_watch;            // coalesces; one wait; close
template <class T, class Allocator> class event_queue;  // bounded; exempt entries; close
```

**Tests.** States coalesce; no change keeps a wait pending; a second wait of a kind completes with
`already_started`; a cancelled wait consumes nothing; the bound keeps a place for the end of
gathering; a satisfied wait is posted, not run inside the call; `close` aborts the wait and refuses
the next with `error::closed`; an operation is allocated through its handler's allocator.

**Expected output.** `**passed**` for `events` and `events_no_exceptions`.

**The C API's codes**, per call, which Tasks 1.6 and 1.7 apply (design.md, D10):

| Call | `RTC_ERR_INVALID` | `RTC_ERR_FAILURE` |
| --- | --- | --- |
| `rtcCreatePeerConnection` | `syntax_error` (an ICE server URL) | `operation_error` |
| `rtcSetLocalDescription` | `operation_error` | `operation_error` (no channel to negotiate) |
| `rtcSetRemoteDescription` | `operation_error` (cannot be parsed, as Chromium) | `invalid_state` (the signaling state refuses it) |
| `rtcAddRemoteCandidate` | `operation_error` | `operation_error` |
| `rtcCreateDataChannelEx` | `operation_error` | `operation_error` |
| `rtcSendMessage` | `operation_error` | `invalid_state` (closed meanwhile) |
| `rtcReceiveMessage` | `operation_error` | `operation_error` |

Checked before the call, as the W3C checks them: a closed connection (`invalid_state`); a
candidate before a remote description (`invalid_state`); both reliability limits, `negotiated`
without an id, id 65535, a label or protocol past 65,535 bytes (`type_error`); a send on a channel
not open (`invalid_state`), past the message limit (`message_too_large`) or the buffered bound
(`buffered_amount_exceeded`); natively, a text holding a zero (`operation_not_supported`).

### Task 1.5: The native runtime (tasks.md 1.5)

**Files:** `include/webcpp/webrtc/runtime.hpp`.

**Interfaces.** `runtime(const runtime_options& = {})` calls `rtcSetThreadPoolSize` when the size
is not 0, then `rtcPreload`; `~runtime()` calls `rtcCleanup`, which waits for libdatachannel's
threads. `get_options()`, `peer_connections()`. `detail::runtime_slot` takes a place in the count
(`std::atomic`, a member), or none past `max_peer_connections`, and gives it back when the
connection's state goes. One runtime at a time natively is a precondition (design.md, D6).

**Tests.** In `native_test.cpp`, `errors_as_values`: a cap of 1 refuses a second connection
with `too_many_peer_connections`, and the count returns to 0 once the first goes;
`destruction_and_moves`: the count follows a move and a destruction.

### Task 1.6: The native peer connection (tasks.md 1.6)

**Files:** `include/webcpp/webrtc/detail/native/capi.hpp`,
`include/webcpp/webrtc/detail/native/connection.hpp`,
`include/webcpp/webrtc/peer_connection.hpp`.

**Interfaces.** `basic_peer_connection<Executor = webcpp::asio::any_io_executor, Allocator =
std::allocator<std::byte>>`: `make(runtime&, const Executor&, const configuration&, const
options& = options(), const Allocator& = Allocator())` returning
`boost::system::result<basic_peer_connection>`; move only; `async_set_local_description`
(terminal cancellation), `async_set_remote_description` and `async_add_ice_candidate` (their
inputs copied during the call, in the object's allocator, so that a deferred token reads nothing
of the caller's afterwards), `async_wait_ice_candidate`, `async_wait_state_change`,
`async_wait_ice_connection_state_change`, `async_accept_data_channel`, `create_data_channel`,
`local_description`, `has_remote_description`, `state`, `ice_state`, `close`.

**What the native state does.** `open` builds `rtcConfiguration` (URLs in libdatachannel's
`turn:user:credential@host` form, `bindAddress`, `iceTransportPolicy`, `disableAutoNegotiation`,
`maxMessageSize`), makes the connection, sets the user pointer to its inbox and the callbacks of
the local candidate, the state, the ICE state, the gathering state (`RTC_GATHERING_COMPLETE` is the
end of candidates, exempt from the bound) and the incoming channel. `rtcSetLocalDescription` is
synchronous in libdatachannel, so the description is read back at once. `close` closes the state
watches with `operation_aborted`, drops the queued candidates and channels, closes every live
channel with `error::closed` (each draining its queue before the connection's transports close),
then `rtcClosePeerConnection`; the destructor then calls `rtcDeletePeerConnection`.

**Tests.** `native_test.cpp`: `a_full_exchange` (offer, answer, candidates to their end both ways
with none refused, the state reaching `connected`, the local description holding `a=candidate`),
`close_fires_no_state_change`, `destruction_and_moves`, `run_returns_once_closed`,
`errors_as_values` (a malformed URL is `syntax_error`; a candidate before a description is
`invalid_state`; adding a channel produces no description).

### Task 1.7: The native data channel (tasks.md 1.7)

**Files:** `include/webcpp/webrtc/detail/native/channel.hpp`,
`include/webcpp/webrtc/data_channel.hpp`.

**Interfaces.** `basic_data_channel<Executor, Allocator>`: `async_wait_open`, `async_wait_close`,
`async_receive(DynamicBuffer&, token)` (Asio's first-version dynamic buffers),
`async_send(ConstBufferSequence, token)`, `async_send_text`, `async_wait_buffered_amount_low`,
`send`, `send_text`, `set_buffered_amount_low_threshold`, `buffered_amount`, `ready_state`,
`label`, `id`, `close`.

**What the native state does.** `attach` sets the user pointer and the callbacks (open, closed,
buffered amount low, available), reads the label, and takes the state the channel already has.
No message callback is set, so libdatachannel keeps messages in its own queue (pull mode).
`read_message` peeks the size, refuses a message larger than the room left in the caller's buffer
with `no_buffer_space` (it stays queued), and copies into one contiguous region when the buffer
offers one, through storage of the channel's allocator otherwise. The peer closing the channel
still delivers what arrived before; the owner's `close()` and the connection's close discard what
is unread, as the W3C discards a message received once the channel left "open", by draining the
queue, which also releases a libdatachannel thread waiting for room.

**Tests.** `native_test.cpp`: binary and text both ways with `is_text`, the label on both sides,
`received_before_the_first_receive`, `a_negotiated_channel` (id 7 on both, nothing accepted),
`bounds` (`message_too_large`, `buffered_amount_exceeded`, the channel staying open,
`no_buffer_space` then the message read into a larger buffer), `a_channel_closed_by_its_owner`.

### Task 1.8: Exceptions (tasks.md 1.8)

**Files:** `test/Jamfile` (`native_no_exceptions`, `events_no_exceptions`, the headers-alone
calls without `<exception-handling>on`).

**What proves it.** The whole native suite, built with `exception-handling=off` (`-fno-exceptions`
on its translation units), passes against the archives the superproject builds with
`-fexceptions` (`tools/libdatachannel/build.py`); the C API's `wrap()` keeps libdatachannel's
exceptions inside. No header of the library throws, tries or catches.

**Expected output.** `**passed** ... native_no_exceptions.test`.

### Task 1.9: Allocation and lifetime (tasks.md 1.9)

**Files:** `test/allocation_test.cpp`, `test/lifetime_test.cpp`.

**Tests.** A connection pair over a polymorphic allocator on a counting resource: the state, the
queues and the outputs allocate there (a description's text has that resource), and no allocation
of it happens on another thread than the io_context's. `destroyed_while_messages_arrive`: the
answering peer and its channel are destroyed while the offering peer floods them, repeated
(`WEBCPP_TEST_WEBRTC_REPEAT`, 10 by default, 100 under ThreadSanitizer);
`a_flooded_channel_is_destroyed_promptly`: 3,000 messages on a channel never read, then its
destruction under two seconds.

**ThreadSanitizer.**

```
b2 --user-config=.local/user-config.jam -j10 threading=multi thread-sanitizer=on \
   define=WEBCPP_TEST_WEBRTC_REPEAT=100 libs/webrtc/test//lifetime libs/webrtc/test//native
```

Expected: both `**passed**`, no `WARNING: ThreadSanitizer` in their output. libdatachannel's
archives are not instrumented: what TSan sees is this library's side of every hand-off.

### Task 1.10: A never-read channel and the others (tasks.md 1.10)

**Files:** `test/stall_test.cpp`.

Measured on 2026-11-14: with 3,000 messages on a channel never read, a message sent on a second
channel of the same connection does not arrive in three seconds; destroying the first channel's
far end releases it. The test asserts that finding, so that a change in libdatachannel fails it
and the documentation is changed with it. Documentation: a program reads every channel it holds,
or closes it.

**Expected output.** `**passed** ... stall.test`.

### Task 1.11: The C API's cost on receipt (tasks.md 1.11)

**Files:** `bench/Jamfile`, `bench/receive_bench.cpp`.

A benchmark program receives 100,000 messages of 1 KiB and of 64 KiB through `rtcReceiveMessage`
and, for comparison only, through the C++ API's `receive()`, in a release build, and reports
messages per second and allocations per message (a replaced global `operator new` counting).
Expected: one more allocation and copy per message through the C API (`Channel::peek` returns a
copy); the documentation states the measured ratio, and the comparison program is not shipped.

### Task 1.12: Documentation (tasks.md 1.12)

**Files:** `doc/` (AsciiDoc, Diataxis: a tutorial with two peers, how-to pages for signalling and
pacing, the reference by MrDocs, explanations), the Doc Comments of every public symbol.

**Content the proposal fixes.** libdatachannel's justification and licence (design.md, D16); the
table of differences (design.md, D14, with the native text holding a zero and the stalled
connection added); the per-operation cancellation table (D11); the error table of Task 1.4; the
runtime's precondition (D6); exceptions (D4); allocation and what is outside the allocator (D12).

**Expected output.** `b2 -a doc` and the doc check clean; MrDocs strict reports nothing.

### Task 1.13: Lint, lanes and review (tasks.md 1.13)

1. `clang-format` and `clang-tidy` of wasi-sdk 34 with the superproject's configuration and
   `test/.clang-tidy`: clean on 2026-11-14 for every translation unit of the test suite and the
   headers-alone units, with and without exceptions (`STATUS.md` says how it was run); the whole
   lint, `tools/lint/lint.sh`, runs once the library is a submodule.
2. The superproject's change: register `libs/webrtc` (`.gitmodules`, `meta`, the CI's lanes),
   and move webrtc from chapter 18 to chapter 15 of `CODING-GUIDELINES.md`. `third_party`'s
   libdatachannel refusal of `threading=single` is already worded for every consumer, not for
   trystero alone (webcpporg/webcpp#15).
3. The review of the increment, then the owner's merge.

## Increment 2: the browser implementation and the interop lane

### Task 2.1: The browser imports (tasks.md 2.1)

**Files:** `include/webcpp/webrtc/detail/browser/imports.hpp`.

**Interfaces.** `EM_ASM` functions in inline functions, as webcpp::beast's
`browser/detail/imports.hpp`: `webcpp_webrtc_make(config, callback)` (an `RTCPeerConnection`
whose `onicecandidate`, `onconnectionstatechange`, `oniceconnectionstatechange`, `ondatachannel`
call the bridge function with `{type, ...}`; a thrown exception returned as its `name`),
`webcpp_webrtc_set_local(pc, callback)`, `webcpp_webrtc_set_remote(pc, type, sdp, callback)`,
`webcpp_webrtc_add_candidate(pc, init, callback)` (each promise's settlement delivered as an event,
its rejection as the DOMException's `name`), `webcpp_webrtc_channel(pc, label, init)`,
`webcpp_webrtc_send(dc, data, size, text)`, `webcpp_webrtc_close(object)`,
`webcpp_webrtc_message_size`, `webcpp_webrtc_copy` (beast's two, for `ArrayBuffer` and text).
Every handler set to null by `detach`.

**Tests.** `browser_imports_test.cpp`, both suspension variants: an exception thrown in each
import arrives as its name, and C++ is never re-entered.

### Task 2.2: The page's census (tasks.md 2.2)

**Files:** `include/webcpp/webrtc/detail/browser/census.hpp`, the emscripten `runtime`.

**Interfaces.** The census lives in JavaScript, on the page, under
`Symbol.for("webcpp.webrtc.census")`: a count of every `RTCPeerConnection` from its construction
until a `FinalizationRegistry` sees it collected, as trystero's is today (`browser/src.hpp:67`,
`browser/backend.hpp:951-960`). `make` past `max_peer_connections` fails with
`too_many_peer_connections`.

**Tests.** `browser_runtime_test.cpp`: the cap, and a freed slot once the census counts the
object collected (a forced collection under node's `--expose-gc`).

### Task 2.3: The browser peer connection (tasks.md 2.3)

**Files:** `include/webcpp/webrtc/detail/browser/connection.hpp`; the facade selects it under
`WEBCPP_TARGET_EMSCRIPTEN`.

**What it does.** The state is an `intrusive_registration` of the bridge (one key per object); its
`deliver` stages each event and posts at most one notification, with a generation number; the
notification feeds `state_watch` and `event_queue` on the executor. `close()` sets the handlers to
null, closes the object, fails the watches with `operation_aborted`; the bridge's `shutdown`
discards what waits. Errors by name (design.md, D10). `bind_address` follows the unsupported
policy (Task 2.5).

**Tests.** `browser_test.cpp`, two peers in one page, every case of increment 1's native suite
that applies, in both suspension variants and with exceptions on and off.

### Task 2.4: The browser data channel (tasks.md 2.4)

**Files:** `include/webcpp/webrtc/detail/browser/channel.hpp`.

**What it does.** Messages are staged in `deliver` into a queue in the object's allocator, charged
at once to `max_received_bytes` with each message's record (beast's `message_record_bytes`); past
it the channel closes and `async_receive` completes with `receive_overflow`; a received message past
`max_message_size` closes it with `message_too_large`. A send checks `readyState`, the size and
`bufferedAmount` before `send`, so the channel stays open on a refusal.

**Tests.** `browser_channel_test.cpp`: overflow, an oversized receipt, the refusals, the
buffered-amount wait, the close paths, a channel accepted after its messages arrived.

### Task 2.5: What the browser cannot honour (tasks.md 2.5)

**Files:** `build.jam` (the feature), `config.hpp`, the browser classes inside
`inline namespace unsupported_error` or `unsupported_ignore`.

**Tests.** `bind_address` fails `make` with `operation_not_supported` under the default and is
ignored under `webcpp-webrtc-unsupported=ignore`; two translation units with different policies
that pass a channel between them fail to link (beast's D9 test, copied).

### Task 2.6: The emscripten lanes (tasks.md 2.6)

```
b2 --user-config=.local/user-config.jam -a toolset=emscripten webcpp-suspend=jspi libs/webrtc/test
b2 --user-config=.local/user-config.jam -a toolset=emscripten webcpp-suspend=asyncify libs/webrtc/test
```

Expected: every browser test `**passed**` in both, and in their `exception-handling=off` twins.

### Task 2.7: The interop lane (tasks.md 2.7)

**Files:** `test/interop/` (a native peer, a page, a local relay over webcpp::beast's WebSocket),
an own lane in `test/Jamfile`.

**What it does.** Chrome headless with `--allow-loopback-in-peer-connection` and
`--disable-features=WebRtcHideLocalIpsWithMdns`; the native peer binds to 127.0.0.1; the relay
carries the descriptions and candidates as JSON. The native peer sends "ping", the page answers
"pong", in both suspension variants.

**Expected output.** The lane's two runs `**passed**`.

### Task 2.8: Lint, lanes, review (tasks.md 2.8)

The whole lint with the emscripten toolset; every lane at the owner's CI levels; the review; the
documentation's table of differences checked against the browser's behaviour.

## Increment 3: trystero on webcpp::webrtc

A change of trystero's own (`libraries/trystero/openspec/changes/<id>`), proposed when
increment 2 is merged. Its tasks:

1. **Replace the backend's WebRTC.** `backend::peer_connection`, `backend::data_channel`,
   `native/peer_connection.hpp`, `native/data_channel.hpp` and the WebRTC part of
   `browser/backend.hpp` go; the room's code holds `webrtc::basic_peer_connection` and
   `webrtc::basic_data_channel` over a polymorphic allocator on the app's resource, as
   design.md's table "trystero's backend, part by part" maps them.
2. **Keep trystero's policies.** Dropping text messages and binaries larger than a room frame
   (on the executor, after `async_receive`); the bound of 64 frames and 10 seconds on frames no
   room has read; the per-minute window of connections; the merge of the two states; the shared
   connection and its adoption, with one reader per channel.
3. **Prove it.** trystero's native and browser suites pass unchanged, its allocation test counts
   the same resource, and `webcpp::webrtc` is recorded as its dependency in `build.jam`,
   `meta/libraries.json` and its documentation.
