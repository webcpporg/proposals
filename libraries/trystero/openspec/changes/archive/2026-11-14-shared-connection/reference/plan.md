---
title: "webcpp::trystero, one connection shared by the rooms of an app: Implementation Plan"
tags: [webcpp, trystero, webrtc, plan]
status: draft
created: 2026-10-11
---

# webcpp::trystero, one connection shared by the rooms of an app: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** two peers that meet in several rooms of the same app share one WebRTC connection, as
Trystero 0.26.0 does: a new public `webcpp::trystero::app` owns the program's peer id and one
connection per remote peer; rooms are joined through `app::join`; each frame names its room by its
token; and every bound the specification sets holds, natively and on emscripten, with and without
exceptions, against Trystero JS in both interop lanes.

**Architecture:**
- **The connection layer** (increment 1): `detail::connection` replaces `detail::peer_link`. It owns
  one remote peer's `peer_connection` and `data_channel`, decodes every frame's envelope, keeps the
  remote's advertised tokens (64 at most), keeps up to 64 frames for 10 s for a room of the app not
  bound yet, and routes each room frame to a `detail::binding`, a room's use of the connection,
  which is the action wire's `channel_link`. `detail::app_impl` owns the connections, the self id,
  the configuration and the registry of rooms by token. In increment 1 every standalone `room`
  builds an `app_impl` of its own, so nothing public changes.
- **The app** (increment 2): `app`, `app_config`, `room_options`, `app::join` returning
  `boost::system::result<room>`; adoption of a negotiated connection, three binding paths (own
  negotiation, presence and kept frames, signaling reuse on a first meeting) under one set of
  conditions (the peer advertised the room itself, the room is not leaving, no backoff runs);
  rebinding at scheduled times after a kick or a peer's `@_leave`, with the lower id's backoff and
  the higher id's dropping window; the initiator's acceptance of an answer to any of its last 8
  challenges; the idle timer; the standalone constructor and `room_config` removed.
- **Allocation** (increment 3): `room_options::memory`, a room's own resource, the app's by
  default; the counting test covers both.
- **The tests' clock:** the memory backend gains a virtual clock (spec ruling 11), on which every
  timer of a room under the memory backend runs; `shared_test.cpp` advances it explicitly, so its
  timing cases assert exact instants and take no wall-clock time to wait.
- **Interop** (increment 4) and **the pages** (increment 5).

**Tech Stack:**
- C++20, header-only; b2 5.5.3; Boost 1.92 (Asio, JSON, System, Core's lightweight_test);
  Emscripten 6.0.11, node 26.7.0; libsecp256k1, libdatachannel 0.24.6 and OpenSSL 3 for the native
  backend, as today; Trystero JS 0.26.0 pinned in `test/oracle/package-lock.json`, headless Chrome
  for the interop lanes.
- No new dependency.

**Spec:** `~/.buzz/PLANS/WEBCPP_TRYSTERO_SHARED_SPEC.md` (status active, approved by
the owner on 2026-10-11). Its rulings 1 to 14 bind every task. Its review, with every probe:
`~/dev/webcpp/.superpowers/sdd/plan-reviews/trystero-shared-review.md`. This plan's review:
`~/dev/webcpp/.superpowers/sdd/plan-reviews/trystero-shared-plan-review.md` (findings cited as C1,
I1-I16, M1-M19, O1-O5); its fix round is applied, with the coordinator's rulings on O1-O5. Citations below are `S:`
for a section of the spec, `P/` for `libs/trystero/` and `C/` for
`libs/trystero/test/oracle/node_modules/@trystero-p2p/core/dist/`.

## Citations rechecked at trystero `8e8911f` (ruling 6)

The superproject's `main` is `f7c96d3`, which records `libs/trystero` at `8e8911f`, three commits
past `54f96d8`; the `trystero/room` regression the spec waits for (ruling 6) is fixed there
(`8e60488`, `ad416e4`, `dc68451`, `8e8911f`). `git diff --stat 54f96d8 8e8911f` touches only
`test/room_test.cpp`, so every citation of a header, a page, an example, the memory backend and the
interop holds. These moved, and the plan uses the new lines:

| Spec cites (at `54f96d8`) | At `8e8911f` |
| --- | --- |
| `room_test.cpp:760-811` (the relay-record helpers `relayed`, `relayed_signal`) | `room_test.cpp:768-803` (its comment at 768, `struct relayed` at 769) |
| `room_test.cpp:797-806` (`join_and_leave`) | `room_test.cpp:805-814` |
| `room_test.cpp:848`, `:882` (its two calls) | `room_test.cpp:863`, `:896` |
| `room_test.cpp:231-248` (`leaving_is_seen_by_the_others_and_a_return_reconnects`) | unchanged |

Task 0 reruns the check, in case `main` moves again before execution.

## Conflicts with the spec, and their rulings

Every place where this plan departs from the specification's text, with the spec's line, the
ruling and who gave it (review I1/O1). "Coordinator" is the coordinator of 2026-10-11, ruling on the
plan review's owner items; "author" is this plan's author, within the spec's rulings.

| # | Spec | Departure | Ruling | By |
| --- | --- | --- | --- | --- |
| 1 | S:6.2 last paragraph and S:6.2 case 3 cite `room_test.cpp` at `54f96d8` | the lines moved at `8e8911f` (only that file changed) | the plan cites `8e8911f` (table above); the spec keeps `54f96d8`, as its sources paragraph says | author, ruling 6 |
| 2 | S:6.2 case 22, "joined by the first rebinding time" | with N2's ordered hold, a frame released after the rebinding time holds its end's every later frame, so nothing completes before the release | joined at the later of the first rebinding time and the release; the spec was amended | author, then coordinator (round 3) |
| 3 | S:6.2 case 4, in Task 7's increment | the case needs the rebinding time and the kept-`@_leave` rule | case 4 is Task 10's (after both); see 9 | author, coordinator (I3) |
| 4 | S:6.2 case 10, first half, "with only `r2` in common the connection closes at once" | that state is reached only by leaving `r1` first, and the lower id may offer again on `r2`'s topics | the test leaves `r1`, then asserts the closing kick, and tells B's connections apart by B's own ICE server; it is Task 9's (it needs the rebinding time) | author |
| 5 | S:7, the files to update | `default_ice_servers` moves from `room.hpp` to `options.hpp`, `detail::copy_ice_servers` to `detail/ice_servers.hpp` | both moves, so that `app_impl.hpp` needs no `room.hpp`; the public function is unchanged | author |
| 6 | S:8, increment 1, "the public API is unchanged" | `options.hpp` (`app_config`, `room_options`) becomes a public header in increment 1, before `app` exists; configuration reports reach the standalone room's `on_error` until `app::on_error` | allowed: no published state has it (ruling 12), and every existing test passes unchanged | author |
| 7 | S:6.1, R4 | `memory_network::time_scale` stays `int` | the review's R4 concerned connection ids only; `time_scale` is unused by the shared tests (virtual clock) | author |
| 8 | S:6.1, ruling 11, cases 4, 20, 22, 23 | the cases ran on a scaled wall clock | a virtual clock in the memory backend; the cases assert exact virtual instants; spec ruling 11, section 6.1 and the four cases amended | coordinator (O2, I13) |
| 9 | S:6.2 case 4's narrative, "B binds by reuse on A's announcement" | unreachable: A joined `r2` first, so B binds at its own join, on A's presence | the plan's path stands (B binds at its join, times out, kicks, binds again at its rebinding time and sends presence); the spec's narrative was corrected | coordinator (O4) |
| 10 | S:6.2 case 22, third variant, "released after A's third kick" | released then, the answers reach A while it refuses, are kept 10 s and expire, so the case proves nothing | released during A's third binding; a fourth variant releases during the ninth and expects `wrong_password`; the spec was amended | coordinator (O3, I8) |
| 11 | S:6.2 case 10, second half (renegotiation after a closing kick) | the plan's first draft replaced it by a rebinding over `r1` | restored: a kick of the connection's last binding closes it, the key holder renegotiates within the room's window, the new connection is adopted and bound with a count of zero, which the test sees as a join before the 11 s a kept count would impose | coordinator (O5) |
| 12 | S:6.2 case 17, third part | deferred to `allocation_test.cpp`, whose cases drain first | restored in `shared_test.cpp` (Task 8b): the `io_context` destroyed with operations pending, after a room handle destroyed during its farewell, under the hardened build | author (I12) |
| 13 | S:6.2 case 5, "the filter holds B's presence" | an ordered hold also holds the handshake frames behind it, so they cannot reach A first; a Trystero JS peer is what sends them first | the frames are injected as from B, then B's presence lets A bind; the 64-frame bound and the 10 s expiry are tested on the connection itself (Task 2), not as the app resource's bytes, which other state of the rooms moves | author (I5) |
| 14 | S:6.4 scenario 1, "the C++ peer's app has adopted one connection" | when the two rooms' negotiations cross (S:2.4), both sides lose both and adopt again | the scenario requires one live connection on each side at the end, one `adopted` event when no crossing occurred, and at most 4 when the page's `madePeerConnections` shows a crossing (a livelock fails by name) | author (M15) |
| 15 | S:6.2 case 19, "or `wrong_password` when the test has B, the lower id, send a wrong response" | the lower id is the initiator: it sends challenges, not responses | the test has B, the higher id, send a wrong response to A's challenge; the spec is corrected to "B, the higher id, send a wrong response", so the plan no longer departs from it | author, then coordinator (spec corrected) |
| 16 | S:6.2 | `test/connection_test.cpp` and `test/memory_backend_test.cpp` are not in the spec | added: they test the connection and the hooks alone | author |
| 17 | S:6.2 cases 1, 2 and 6 in Task 6 | they pass as soon as `app` exists (adoption closes the second connection) | they are Task 5c's, where they are red for want of `app`; Task 6 keeps the cases that need presence binding | coordinator (I15) |
| 18 | Spec 4.2 / Trystero detaches before its idle close | close_everything sends presence(0) before detaching the bindings, so the presence still goes out when idle_timeout is 0 (an idle close of 0 ends the connection inside detach; Trystero's close is always a setTimeout) | Task 4's implementer, confirmed by its review (probe) |

## Global Constraints

**Repositories, the worktree and the commands:**
- Work happens in a worktree of the superproject, `~/dev/webcpp-trystero-shared`, on the branch
  `trystero-shared` from `main` `f7c96d3`; `libs/trystero` there is on its own branch,
  `shared-connection`, from `8e8911f`. Task 0 makes both.
- `~/dev/webcpp-trystero-shared/.local/` holds one link per entry of `~/dev/webcpp/.local/`, as
  `~/dev/webcpp-asio/.local/` does (`user-config.jam`, `emsdk`, `emscripten`, `emscripten-cache`,
  `secp256k1-native`, `secp256k1-emscripten`, `libdatachannel`, `openssl`, `wasi-sdk`, `wasi-wit`,
  `wit-bindgen`, `mrdocs`, `boost-source`, `env.sh`).
- Every b2 command runs from `~/dev/webcpp-trystero-shared` as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`,
  written `$B2` below. A verdict is `-a`. Each lane has its own `--build-dir`.
  No cap on concurrent builds (owner, 2026-10-11): a test that fails under load is flaky and is fixed, never protected by running less at once.
- The lanes, as the CI runs them:
  - native: `$B2 -a --build-dir=bin/lane-native libs/trystero/test libs/trystero/example`;
  - emscripten: `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/trystero/test libs/trystero/example`;
  - the oracle: `$B2 -a --build-dir=bin/lane-oracle libs/trystero/test/oracle//oracle`;
  - interop, native: `$B2 -a --build-dir=bin/lane-interop-native --dump-tests --out-xml=native.trystero.interop.xml libs/trystero/test/oracle//interop`;
  - interop, emscripten: `$B2 -a --build-dir=bin/lane-interop-emscripten --dump-tests --out-xml=emscripten.trystero.interop.xml toolset=emscripten libs/trystero/test/oracle//interop`.
- One test while working: `$B2 libs/trystero/test//shared` (and `toolset=emscripten`).
- **GCC from the start** (review I2): every task's verification builds its tests with
  `toolset=gcc-15` too (configured in `.local/user-config.jam` as `using gcc : 15 : g++-15`), the
  core tests only, since `.local/libdatachannel` is built against libc++ and a native-backend test
  does not link with libstdc++:
  `$B2 toolset=gcc-15 --build-dir=bin/gcc-15 libs/trystero/test//<test> ...`, the `<test>`s each
  task names among `protocol`, `room`, `connection`, `memory_backend`, `shared`,
  `shared_no_exceptions`, `shared_hardened` and `allocation`. The gcc lane at each increment's gate
  runs them all: `$B2 -a --build-dir=bin/lane-gcc-15 toolset=gcc-15 libs/trystero/test//protocol libs/trystero/test//room libs/trystero/test//connection libs/trystero/test//memory_backend libs/trystero/test//shared libs/trystero/test//shared_no_exceptions libs/trystero/test//shared_hardened libs/trystero/test//allocation`.
- **The hardened build** (review C1, I7): `shared_hardened` (declared in Task 5c) is `shared_test.cpp` built with
  `_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`, native only,
  as `native_websocket` is declared; and once per increment's gate, natively with clang, under
  AddressSanitizer and UndefinedBehaviorSanitizer:
  `$B2 -a --build-dir=bin/asan address-sanitizer=norecover undefined-sanitizer=norecover libs/trystero/test//shared libs/trystero/test//connection`.
  `norecover` is b2's `-fno-sanitize-recover` (`tools/features/sanitizers-feature.jam`): a report
  aborts the test, so the run fails on it; with `on`, UndefinedBehaviorSanitizer would print and
  exit 0, and b2 keeps a passing run's output out of view (review N3).
- **Virtual time** (review I13, spec ruling 11): `shared_test.cpp`, `connection_test.cpp` and the
  S:6.2 cases Task 4 adds to `room_test.cpp` run the memory backend's virtual clock, and state
  their instants exactly; the other cases of `room_test.cpp`, `native_room_test.cpp` (a real
  backend) and `allocation_test.cpp` (whose window is the wall clock's) keep the clock they have
  today. Every virtual run is bounded by `memory_network::max_steps` timers (10^6): `advance` and
  `run_until` fail the case by name past it, so a timer re-armed without delay fails a test instead
  of hanging it under node, where no b2 timeout applies (review N4). A virtual timer leaves the
  clock when it is restarted, cancelled or destroyed, so a room's resource is empty as soon as the
  room is gone (review N1; case 21 relies on it).
- The lint: `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`,
  ending `lint: clean`.

**The library's rules (`~/dev/webcpp/AGENTS.md` chapter 6, `P/AGENTS.md`, the spec):**
- Names: namespace `webcpp::trystero`, `detail` for what is not public, macros `WEBCPP_TRYSTERO_*`,
  include guards `WEBCPP_TRYSTERO_<PATH>_HPP`, `snake_case`. The wire never changes: no byte a
  Trystero JS peer sees differs (`P/AGENTS.md`, "The original, and its wire"); the oracle lane must
  stay byte-identical.
- `<webcpp/trystero/config.hpp>` first in every public header. trystero raises nothing of its own:
  no `throw`, `try` or `catch` in the new code; everything compiles with and without exceptions.
- No global or static mutable state in the library. Fixed-width types; `std::size_t` for counts and
  indices; `int` only where an interface of the standard library or Boost asks.
- Errors as values: `join` returns `boost::system::result<room>`; `error::already_joined = 14`,
  `error::too_many_rooms = 15`, never reused.
- Allocation (`P/AGENTS.md` "Allocation"): every allocation of the app goes through
  `app_config::memory`, every allocation of a room through its resource (`room_options::memory`, the
  app's when null, from increment 3); `std::allocate_shared` over
  `std::pmr::polymorphic_allocator<>`, the `handler` type for every callable, `detail::post` for
  every post; a `std::pmr` container is never copied without an allocator, a lambda's capture
  included: `peer = std::pmr::string(peer, memory_)`, never `[peer]`, whose copy takes the default
  resource (review N2; the allocation test sees it on gcc-15 and emscripten, where a 20-character
  id does not fit the short string).
- After a room has ended, the app holds nothing allocated in the room's resource (S:4.1, ruling 8).
- A function starts with its guards. A comment says what the code does not. Doc Comments complete
  for every public symbol (`~/dev/webcpp/AGENTS.md` chapter 7).
- Tests use `<boost/core/lightweight_test.hpp>` and `require.hpp`; no test reaches beyond the
  loopback.

**Values fixed by the spec (copied verbatim):**
- `app::max_rooms` 64; a remote's advertised tokens 64; frames kept for unbound rooms 64 per
  connection, all dropped 10 s after the first; `idle_timeout` 123,333 ms by default, 0 closes at
  once, a negative value replaced with 0 and reported.
- The rebinding step: the first is the larger of `handshake_timeout` and 10 s, plus 1 s (11 s by
  default), doubling per end to 5 minutes; the count lasts until a completed handshake or the
  connection's end.
- The initiator keeps the last 8 challenges per peer and room since the last completed handshake.
- A room frame is at most 16,515 bytes, a token 1 to 128 bytes.

**Commits and text:**
- In the library: `git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "trystero: <what>" -- <paths>`;
  in the superproject: `git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "<what>" -- <paths>`.
  No trailer of any kind, no agent co-author. After each commit,
  `git log -1 --format='%an %ae %cn %ce'` prints `R github.overboard312@passmail.net R github.overboard312@passmail.net`
  and `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage by path; never `git add -A`, `git add .` or `git add libs`.
- Everything written is English. No em dash (U+2014). The word the lint bans appears nowhere.
- Every new source file opens with the WebCpp.org licence notice.

**Gates (the controller alone, each with the owner's agreement):**
- **G1:** push `libs/trystero`'s `shared-connection` to `webcpporg/trystero` and merge it to `main`.
- **G2:** bump the submodule in the superproject (`git add libs/trystero`, never `git add libs`),
  push, merge.
- A subagent never pushes, merges, creates a repository, starts or cancels a CI run, or prints an
  environment variable.

**Names fixed by this plan:**

| Name | What it is |
| --- | --- |
| `include/webcpp/trystero/options.hpp` | `app_config`, `room_options` and `default_ice_servers` (Task 3; `room_options::memory` in Task 13) |
| `include/webcpp/trystero/app.hpp` | `class app` (Task 5c) |
| `include/webcpp/trystero/detail/connection.hpp` | `detail::connection` and `detail::binding` (Task 2) |
| `include/webcpp/trystero/detail/app_impl.hpp` | `detail::app_impl` and `detail::room_endpoint` (Task 3) |
| `include/webcpp/trystero/detail/rebinding.hpp` | `detail::rebinding_step`, `detail::recent_challenges`, `detail::rebind_state` (Task 9) |
| `test/support/rooms.hpp` | the tests' shared helpers: `fixture`, `run_until`, `advance`, `counting_resource`, `flooder`, `relayed`, `relayed_signal` (Task 5a; `app_config` and `join` added in Task 5c) |
| `memory_network::virtual_clock`, `advance`, `buffered` | the virtual clock and a channel's buffered bytes (Task 1) |
| `test/connection_test.cpp`, target `connection` | the connection alone (Task 2) |
| target `shared_hardened` | `shared_test.cpp` with the standard libraries' checks, native only (Task 5c) |
| `room_impl::bind_peer`, `peer_state::link` | binding a peer (the `bind<Sig>` helper keeps its name, review M11); the peer's binding, a `std::shared_ptr<detail::binding>` (a member named `binding` is rejected by GCC, review I2) |
| `test/memory_backend_test.cpp` | the memory backend's hooks (Task 1) |
| `test/shared_test.cpp`, targets `shared` and `shared_no_exceptions` | the specification's unit cases (S:6.2) |
| log lines `webrtc: connection with <peer> adopted` and `webrtc: connection with <peer> ended: <reason>` | what the interop peers turn into the events `adopted` and `ended` (Task 14a) |
| `~/dev/webcpp/.superpowers/sdd/WEBCPP_TRYSTERO_SHARED_PLAN/progress.md` | the ledger |

## Review Focus

1. **A room with its own resource destroyed while one of its sends waits on the shared channel,
   and the app's other room going on.** A waiter left in the connection would free into a dead
   resource. Task 13 (case 21) destroys such a room with a buffering channel and checks its
   resource returns to zero while the other room still exchanges.
2. **Synchronous re-entrancy into a room or a connection while it iterates.** A kept frame
   delivered during a binding can kick the peer at once (review C1); a connection's end fails every
   drain waiter, whose completion fails a transfer and may send on the ended connection; a room
   handle destroyed during its farewell leaves operations that the `io_context`'s destruction
   destroys. `on_peer_leave` is posted, so it is not one of these paths. Task 6 (the kept frame),
   Task 8a (the connection failing under a waiting transfer), Task 8b (the farewell and the
   `io_context`'s destruction), the hardened target `shared_hardened` and the sanitizer run cover
   them.
3. **Malformed envelopes on a shared connection.** A presence frame with a token of 0 or 129 bytes,
   a first byte 3, a frame of 16,516 bytes, each ends the connection for every room; a frame of
   exactly 16,515 bytes does not. Task 4 injects each.
4. **Join and leave sequences on one room id.** `join`, `async_leave`, `join` during the farewell,
   `async_leave` twice, the room destroyed during its farewell, `join` again: each completes once,
   `presence(0)` and `presence(1)` go out in that order, and `max_rooms` counts the leaving room
   until its `presence(0)`. Task 8b (cases 15 and 16) runs the sequence.
5. **Two apps of one app id in one process with the same self id.** They must never meet, make no
   negotiation loop, and not crash. Task 5c adds the case.

---

## Task 0: The worktree, the ledger and the baseline

**Files:**
- Create: `~/dev/webcpp/.superpowers/sdd/WEBCPP_TRYSTERO_SHARED_PLAN/progress.md`

- [ ] **Step 1: Make the worktree and the library's branch.**

```bash
git -C ~/dev/webcpp worktree add -b trystero-shared ~/dev/webcpp-trystero-shared f7c96d3
cd ~/dev/webcpp-trystero-shared
git submodule update --init
git -C libs/trystero switch -c shared-connection
mkdir .local && for f in ~/dev/webcpp/.local/*; do ln -s "$f" .local/; done
```

Expected: `git -C libs/trystero rev-parse --short HEAD` prints `8e8911f`, and
`git submodule status libs/trystero` names `heads/shared-connection`.

- [ ] **Step 2: Create the ledger** with the first line
  `# SDD ledger - plan: ~/.buzz/PLANS/WEBCPP_TRYSTERO_SHARED_PLAN.md`.

- [ ] **Step 3: Recheck the citations** (ruling 6): `git -C libs/trystero diff --stat 54f96d8 HEAD`
  prints only `test/room_test.cpp`; `grep -n 'void join_and_leave\|^struct relayed\|join_and_leave(f'
  libs/trystero/test/room_test.cpp` prints lines 769, 805, 863 and 896 (the table above). If `main`
  moved, update the table and every task that cites a moved line before going on.

- [ ] **Step 4: Record the baseline** in the ledger: the native, gcc-15 and emscripten lanes, the
  oracle, both interop lanes, and the lint, each from scratch (Global Constraints); the native
  interop lane runs 19 scenarios at `8e8911f` and the browser one 16 (`run.mjs:603-723`). Expected: every lane
  green, `lint: clean`. A failure found here is fixed first, in a commit of its own.

---

## Increment 1: the connection layer, one room per app

### Task 1: The memory backend's hooks

**Files:**
- Modify: `libs/trystero/test/support/memory_backend.hpp` (the ids, the channel, the peer
  connection, the network)
- Create: `libs/trystero/test/memory_backend_test.cpp`
- Modify: `libs/trystero/test/Jamfile` (the test, on both targets)

**Interfaces:**
- Produces (on `webcpp::trystero::test::memory_network`, used by Tasks 4 to 13):

```cpp
enum class verdict { deliver, drop, hold };
std::function<void(std::size_t pc, std::span<const backend::ice_server> servers)> on_made;
std::function<verdict(std::size_t from, std::size_t to, std::span<const std::uint8_t>)> filter;
void release(std::size_t to);
bool defer_opens = false;
void open(std::size_t pc);
void inject(std::size_t pc, std::span<const std::uint8_t> frame);
void fail(std::size_t pc);
std::size_t channel_capacity = 0;
std::size_t drain_per_post = 16 * 1024;
[[nodiscard]] std::size_t peer_of(std::size_t pc) const;
[[nodiscard]] std::size_t buffered(std::size_t pc) const;
bool virtual_clock = false;
std::size_t max_steps = 1'000'000;
bool advance(std::chrono::milliseconds by);
bool step(double until_ms);
[[nodiscard]] std::size_t scheduled() const;
```

  and `memory_peer_connection::id()` returning `std::size_t`; `connection_registry` keyed by
  `std::size_t`.

- [ ] **Step 1: Write the failing test** `libs/trystero/test/memory_backend_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The memory backend's hooks, which the tests of shared connections drive: a filter that delivers,
// drops or holds frames in the channel's order, deferred opens, injected frames, a forced failure,
// and a channel that buffers and drains.

#include "support/memory_backend.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <memory_resource>
#include <string_view>
#include <vector>

#include "require.hpp"

namespace asio = boost::asio;
namespace trystero = webcpp::trystero;
using trystero::test::memory_network;
using namespace std::chrono_literals;

namespace {

bool run_until(asio::io_context& io, const std::function<bool()>& done,
               std::chrono::milliseconds limit = 5s) {
    const auto end = std::chrono::steady_clock::now() + limit;
    while (!done() && std::chrono::steady_clock::now() < end) {
        io.run_for(10ms);  // lint-run: the memory network posts to it
        if (io.stopped()) {
            io.restart();
        }
    }
    return done();
}

/** Two ends of one pair, their channels and what each received. */
struct pair {
    std::shared_ptr<trystero::backend::peer_connection> offerer;
    std::shared_ptr<trystero::backend::peer_connection> answerer;
    std::shared_ptr<trystero::backend::data_channel> offer_end;
    std::shared_ptr<trystero::backend::data_channel> answer_end;
    std::vector<std::vector<std::uint8_t>> at_offerer;
    std::vector<std::vector<std::uint8_t>> at_answerer;
    std::vector<trystero::backend::connection_state> offerer_states;
    std::size_t offerer_id = 0;
    std::size_t answerer_id = 0;
};

/** Makes and pairs two peer connections, as two rooms' negotiations do. */
void connect(memory_network& net, pair& p) {
    std::vector<std::size_t> made;
    net.on_made = [&made](std::size_t pc, std::span<const trystero::backend::ice_server>) {
        made.push_back(pc);
    };
    auto* memory = std::pmr::get_default_resource();
    p.offerer = net.make_peer_connection({}, memory);
    p.answerer = net.make_peer_connection({}, memory);
    net.on_made = {};
    p.offerer_id = made.at(0);
    p.answerer_id = made.at(1);
    p.offer_end = p.offerer->create_data_channel("data");
    p.offerer->set_handlers({.ice_candidate = {}, .data_channel = {},
                             .state = [&p](trystero::backend::connection_state s) {
                                 p.offerer_states.push_back(s);
                             }});
    p.answerer->set_handlers({.ice_candidate = {},
                              .data_channel =
                                  [&p](std::shared_ptr<trystero::backend::data_channel> c) {
                                      p.answer_end = std::move(c);
                                      p.answer_end->set_handlers(
                                          {.open = {},
                                           .message =
                                               [&p](trystero::backend::bytes b) {
                                                   p.at_answerer.emplace_back(b.begin(), b.end());
                                               },
                                           .close = {},
                                           .buffered_amount_low = {}});
                                  },
                              .state = {}});
    p.offer_end->set_handlers({.open = {},
                               .message =
                                   [&p](trystero::backend::bytes b) {
                                       p.at_offerer.emplace_back(b.begin(), b.end());
                                   },
                               .close = {},
                               .buffered_amount_low = {}});
    std::pmr::string offer(memory);
    p.offerer->set_local_description(
        [&offer](trystero::backend::error_code, std::string_view, std::string_view sdp) {
            offer = sdp;
        });
    // Each side's description is the other's remote one, as the signaling hands them over.
    webcpp::test::require(BOOST_TEST(run_until(net.io(), [&] { return !offer.empty(); })));
    p.answerer->set_remote_description("offer", offer, [](trystero::backend::error_code) {});
    std::pmr::string answer(memory);
    p.answerer->set_local_description(
        [&answer](trystero::backend::error_code, std::string_view, std::string_view sdp) {
            answer = sdp;
        });
    webcpp::test::require(BOOST_TEST(run_until(net.io(), [&] { return !answer.empty(); })));
    p.offerer->set_remote_description("answer", answer, [](trystero::backend::error_code) {});
}

void ids_are_sizes_and_peer_of_pairs_them() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    BOOST_TEST(run_until(io, [&] { return p.answer_end && p.answer_end->is_open(); }));
    BOOST_TEST_EQ(net.peer_of(p.offerer_id), p.answerer_id);
    BOOST_TEST_EQ(net.peer_of(p.answerer_id), p.offerer_id);
}

void a_held_frame_holds_every_later_frame_of_its_end() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    net.filter = [](std::size_t, std::size_t, std::span<const std::uint8_t> frame) {
        return frame[0] == 'A' ? memory_network::verdict::hold : memory_network::verdict::deliver;
    };
    const std::uint8_t a[] = {'A'};
    const std::uint8_t b[] = {'B'};
    p.offer_end->send(a);
    p.offer_end->send(b);
    run_until(io, [] { return false; }, 100ms);
    BOOST_TEST(p.at_answerer.empty());  // B waits behind A, as on an ordered channel
    net.filter = {};
    net.release(p.answerer_id);
    BOOST_TEST(run_until(io, [&] { return p.at_answerer.size() == 2; }));
    BOOST_TEST(p.at_answerer.at(0).at(0) == 'A' && p.at_answerer.at(1).at(0) == 'B');
}

void a_dropped_frame_is_gone() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    net.filter = [](std::size_t, std::size_t, std::span<const std::uint8_t> frame) {
        return frame[0] == 'A' ? memory_network::verdict::drop : memory_network::verdict::deliver;
    };
    const std::uint8_t a[] = {'A'};
    const std::uint8_t b[] = {'B'};
    p.offer_end->send(a);
    p.offer_end->send(b);
    BOOST_TEST(run_until(io, [&] { return p.at_answerer.size() == 1; }));
    BOOST_TEST(p.at_answerer.at(0).at(0) == 'B');
}

void deferred_opens_open_one_end_at_a_time() {
    asio::io_context io;
    memory_network net{io};
    net.defer_opens = true;
    pair p;
    connect(net, p);
    run_until(io, [] { return false; }, 100ms);
    BOOST_TEST(!p.offer_end->is_open());
    net.open(p.offerer_id);
    BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); }));
    BOOST_TEST(!p.answer_end || !p.answer_end->is_open());
    net.open(p.answerer_id);
    BOOST_TEST(run_until(io, [&] { return p.answer_end && p.answer_end->is_open(); }));
}

void an_injected_frame_reaches_the_end_named() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    const std::uint8_t x[] = {'X'};
    net.inject(p.offerer_id, x);
    BOOST_TEST(run_until(io, [&] { return p.at_offerer.size() == 1; }));
}

void a_failed_connection_reports_failed_then_closes() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    net.fail(p.offerer_id);
    BOOST_TEST(run_until(io, [&] {
        return !p.offerer_states.empty() &&
               p.offerer_states.back() == trystero::backend::connection_state::failed;
    }));
    BOOST_TEST(run_until(io, [&] { return !p.offer_end->is_open(); }));
}

// A frame larger than drain_per_post drains byte by byte and is delivered with its last byte.
void a_frame_is_delivered_once_its_last_byte_drained() {
    asio::io_context io;
    memory_network net{io};
    net.channel_capacity = 1;
    net.drain_per_post = 1024;
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    const std::vector<std::uint8_t> frame(16 * 1024, 5);
    p.offer_end->send(frame);
    BOOST_TEST_EQ(net.buffered(p.offerer_id), frame.size());
    io.poll_one();  // one drain step
    BOOST_TEST_EQ(net.buffered(p.offerer_id), frame.size() - 1024);
    BOOST_TEST(p.at_answerer.empty());
    BOOST_TEST(run_until(io, [&] { return p.at_answerer.size() == 1; }));
    BOOST_TEST_EQ(net.buffered(p.offerer_id), 0U);
}

// A frame held for an end whose sender is gone is dropped on release (review M14).
void a_release_after_the_sender_went_drops_its_frames() {
    asio::io_context io;
    memory_network net{io};
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    net.filter = [](std::size_t, std::size_t, std::span<const std::uint8_t>) {
        return memory_network::verdict::hold;
    };
    const std::uint8_t a[] = {'A'};
    p.offer_end->send(a);
    net.filter = {};
    p.offer_end.reset();
    p.offerer.reset();  // the sending end is destroyed
    net.release(p.answerer_id);
    run_until(io, [] { return false; }, 100ms);
    BOOST_TEST(p.at_answerer.empty());
}

// The virtual clock: a timer fires only when the test advances the clock to it, in order, and
// now_ms is the clock's.
void the_virtual_clock_moves_only_when_advanced() {
    asio::io_context io;
    memory_network net{io};
    net.virtual_clock = true;
    auto* m = std::pmr::get_default_resource();
    std::vector<int> fired;
    const auto t10 = net.make_timer(m);
    const auto t5 = net.make_timer(m);
    const auto cancelled = net.make_timer(m);
    t10->start(10s, [&fired] { fired.push_back(10); });
    t5->start(5s, [&fired] { fired.push_back(5); });
    cancelled->start(1s, [&fired] { fired.push_back(1); });
    cancelled->cancel();
    io.poll();
    BOOST_TEST(fired.empty());
    BOOST_TEST_EQ(net.now_ms(), 0.0);
    net.advance(7s);
    BOOST_TEST(fired == std::vector<int>{5});
    BOOST_TEST_EQ(net.now_ms(), 7000.0);
    net.advance(3s);
    BOOST_TEST((fired == std::vector<int>{5, 10}));
    BOOST_TEST_EQ(net.now_ms(), 10000.0);
}

/** Counts the bytes it has out, from the default resource. */
class counting_resource final : public std::pmr::memory_resource {
public:
    std::size_t in_use = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        in_use += bytes;
        return std::pmr::get_default_resource()->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        in_use -= bytes;
        std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
    }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

// Review N1: a virtual timer restarted, cancelled or destroyed leaves the clock at once, so its
// memory, in the resource of the room or connection that made it, is back at once, not at its due
// time.
void a_virtual_timer_leaves_the_clock_at_once() {
    asio::io_context io;
    memory_network net{io};
    net.virtual_clock = true;
    counting_resource memory;
    auto t = net.make_timer(&memory);
    t->start(10s, [] {});
    t->start(5s, [] {});  // a restart replaces the first expiry
    BOOST_TEST_EQ(net.scheduled(), 1U);
    t->cancel();
    BOOST_TEST_EQ(net.scheduled(), 0U);
    t->start(10s, [] {});
    t.reset();  // destroyed while scheduled
    BOOST_TEST_EQ(net.scheduled(), 0U);
    BOOST_TEST_EQ(memory.in_use, 0U);  // before any advance
    BOOST_TEST(net.advance(20s));
}

// Review N4: a timer that re-arms itself without delay would never let the clock move; advance
// stops after max_steps timers and says so, and the test fails instead of hanging.
void a_timer_rearmed_without_delay_stops_the_clock() {
    asio::io_context io;
    memory_network net{io};
    net.virtual_clock = true;
    net.max_steps = 1000;
    const auto t = net.make_timer(std::pmr::get_default_resource());
    std::size_t fired = 0;
    std::function<void()> again = [&] {
        ++fired;
        t->start(0ms, again);
    };
    t->start(0ms, again);
    BOOST_TEST(!net.advance(1s));
    BOOST_TEST_EQ(fired, 1000U);
    t->cancel();
}

void a_buffering_channel_drains_and_says_so() {
    asio::io_context io;
    memory_network net{io};
    net.channel_capacity = 1;
    net.drain_per_post = 1024;
    pair p;
    connect(net, p);
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return p.offer_end->is_open(); })));
    bool low = false;
    p.offer_end->set_handlers({.open = {}, .message = {}, .close = {},
                               .buffered_amount_low = [&low] { low = true; }});
    const std::vector<std::uint8_t> big(100'000, 7);
    p.offer_end->send(big);
    BOOST_TEST_EQ(p.offer_end->buffered_amount(), big.size());
    BOOST_TEST(run_until(io, [&] { return low; }));
    BOOST_TEST_LE(p.offer_end->buffered_amount(), trystero::backend::data_channel::low_threshold);
    BOOST_TEST(run_until(io, [&] { return p.at_answerer.size() == 1; }));
}

}  // namespace

int main() {
    ids_are_sizes_and_peer_of_pairs_them();
    a_held_frame_holds_every_later_frame_of_its_end();
    a_dropped_frame_is_gone();
    deferred_opens_open_one_end_at_a_time();
    an_injected_frame_reaches_the_end_named();
    a_failed_connection_reports_failed_then_closes();
    a_buffering_channel_drains_and_says_so();
    a_frame_is_delivered_once_its_last_byte_drained();
    a_release_after_the_sender_went_drops_its_frames();
    the_virtual_clock_moves_only_when_advanced();
    a_virtual_timer_leaves_the_clock_at_once();
    a_timer_rearmed_without_delay_stops_the_clock();
    return boost::report_errors();
}
```

  Add to `libs/trystero/test/Jamfile`, after `webcpp.run room : room_test.cpp ;`:

```jam
# The memory backend's hooks, which the tests of shared connections drive.
webcpp.run memory_backend : memory_backend_test.cpp ;
```

- [ ] **Step 2: Run it to see it fail.**
  Run: `$B2 libs/trystero/test//memory_backend`
  Expected: compile errors naming `on_made`, `filter`, `peer_of`, `defer_opens`, `inject`, `fail`,
  `scheduled`, `max_steps`, `channel_capacity`.

- [ ] **Step 3: Implement the hooks** in `test/support/memory_backend.hpp`.
  - `connection_registry` becomes `std::pmr::map<std::size_t, memory_peer_connection*>`;
    `memory_peer_connection`'s `id_`, its constructor's `id` and `id()` become `std::size_t`;
    `remote_id_` becomes `std::optional<std::size_t>` (no remote yet when empty), with
    `remote_id()` returning it and `has_remote_description()` returning `remote_id_.has_value()`;
    `memory_network::next_pc_` and `find` take `std::size_t`. The description's text stays
    `"mem:<id>"`; `set_remote_description` parses it with `std::from_chars` into a `std::size_t`.
  - `memory_data_channel` gains `memory_network* net_` and `std::size_t pc_`, set by a new
    `void bind_to(memory_network& net, std::size_t pc)` that `create_data_channel` and `connect_to`
    call for each end, and a queue of frames it was sent and has not delivered:

```cpp
    std::size_t buffered_amount() const override { return buffered_; }

    /** The frame goes through the network's filter, in this end's order, then arrives at the other
     * end as a copy in the other end's resource, as a received frame is. With a channel capacity,
     * the frames queue and drain by `drain_per_post` bytes per post. */
    void send(std::span<const std::uint8_t> frame) override;
```

    and the network's part:

```cpp
    /** What a filter does with a frame one end sends to the other. */
    enum class verdict { deliver, drop, hold };

    /** Called when a connection is made, before make_peer_connection returns, with its id and the
     * ICE servers it was made with: a test gives each app ICE servers of its own to tell them
     * apart. */
    std::function<void(std::size_t pc, std::span<const backend::ice_server> servers)> on_made;
    /** Called with the sending end, the receiving end and the frame; empty: every frame delivered.
     * A held frame holds every later frame from that end behind it, unfiltered until released, as
     * a real data channel delivers in order. */
    std::function<verdict(std::size_t from, std::size_t to, std::span<const std::uint8_t>)> filter;
    /** Delivers the frames held for `to`, in the order they were sent, each then filtered again. */
    void release(std::size_t to);
    /** When true, a pair's channels open only through open(), one end at a time. */
    bool defer_opens = false;
    /** Opens the channel of `pc`, whose pair is connected. */
    void open(std::size_t pc);
    /** Delivers `frame` on the channel of `pc`, as if its other end had sent it. */
    void inject(std::size_t pc, std::span<const std::uint8_t> frame);
    /** Reports `failed` on `pc`, then closes its channel. */
    void fail(std::size_t pc);
    /** 0: buffered_amount() stays 0, as today. Otherwise each channel holds what it was sent, up to
     * no limit, and delivers `drain_per_post` bytes per post, firing buffered_amount_low at the
     * threshold; buffered_amount() is the bytes sent and not yet delivered. */
    std::size_t channel_capacity = 0;
    /** How many bytes a buffering channel delivers per post. */
    std::size_t drain_per_post = std::size_t{16} * 1024;
    /** The other end of `pc`, once the two are paired; `pc` itself until then. */
    [[nodiscard]] std::size_t peer_of(std::size_t pc) const;
```

    with the private state, in the default resource and touched only when a test sets a hook (the
    allocation test sets none):

```cpp
    struct held_frame {
        std::size_t from;
        std::vector<std::uint8_t> bytes;
    };
    std::map<std::size_t, std::deque<held_frame>> held_;            // by the receiving end
    std::map<std::size_t, std::weak_ptr<memory_data_channel>> deferred_;  // ends waiting for open()
```

  - `memory_data_channel::send` (its body replaces `memory_backend.hpp:326-340`):

```cpp
inline void memory_data_channel::send(std::span<const std::uint8_t> frame) {
    const auto remote = remote_.lock();
    if (!open_ || !remote) {
        return;
    }
    bytes_sent += frame.size();
    net_->route(pc_, remote->pc_, frame);
}
```

    and the network routes it:

```cpp
inline void memory_network::route(std::size_t from, std::size_t to,
                                   std::span<const std::uint8_t> frame) {
    // A frame behind a held one waits too: the channel delivers in order.
    if (const auto it = held_.find(to); it != held_.end() && !it->second.empty()) {
        it->second.push_back({from, {frame.begin(), frame.end()}});
        return;
    }
    switch (filter ? filter(from, to, frame) : verdict::deliver) {
        case verdict::drop:
            return;
        case verdict::hold:
            held_[to].push_back({from, {frame.begin(), frame.end()}});
            return;
        case verdict::deliver:
            break;
    }
    deliver(from, to, frame);
}

inline void memory_network::release(std::size_t to) {
    auto queue = std::exchange(held_[to], {});
    held_.erase(to);
    for (const auto& h : queue) {
        route(h.from, to, h.bytes);
    }
}
```

    `deliver(from, to, frame)` finds the sending end's channel (through `find(from)`), and drops the
    frame when that end is gone (a release after the sender was destroyed, review M14); then it
    either posts the copy to the receiving channel's `message` handler as today, or, with
    `channel_capacity` set, appends the copy (in the receiving end's resource, as today's copy is)
    to the sending channel's queue, adds its size to `buffered_`, and starts the drain if none
    runs. The drain is byte-accurate (spec section 6.1): a posted step drains up to
    `drain_per_post` bytes from the head of the queue, keeping how much of the head frame has
    drained; a frame is delivered once its last byte has drained, so a frame larger than
    `drain_per_post` takes several steps; the step subtracts what it drained from `buffered_`,
    calls `buffered_amount_low` once `buffered_` falls to `low_threshold` or below from above it,
    and posts itself again while the queue holds bytes. `buffered(pc)` returns the
    `buffered_amount()` of the channel of `pc`, or 0.
  - The virtual clock (spec ruling 11, review I13). Its state is owned by the network and reached
    by its timers weakly, so that a timer outliving the network touches nothing; it is made, in the
    default resource, by the first virtual timer, so a test that sets no clock (the allocation
    test) allocates nothing for it:

```cpp
/** The virtual clock: the timers scheduled on it, each once, by due time and then by the order of
 * scheduling. A timer leaves when it fires, and when it is restarted, cancelled or destroyed, so
 * that nothing keeps it, or its memory in the resource of the room or connection that made it,
 * past that call (review N1). */
struct virtual_clock_state {
    using key = std::pair<double, std::uint64_t>;
    std::map<key, memory_timer*> due;
    std::uint64_t scheduled = 0;
    double now_ms = 0;
};
```

    `memory_network` holds `std::shared_ptr<virtual_clock_state> clock_`, and `make_timer` passes
    a `memory_timer` the `std::weak_ptr` of it (made then if null) when `virtual_clock` is set; a
    wall-clock timer gets none and keeps today's path. A virtual `memory_timer` holds that weak
    reference, `std::optional<virtual_clock_state::key> key_` and its handler `fire_`:

```cpp
    void start(std::chrono::milliseconds delay, handler<void()> fire) override {
        // (the wall-clock path, today's, when the timer has no clock)
        unschedule();  // a restart replaces the pending expiry
        fire_ = std::move(fire);
        if (const auto c = clock_.lock()) {
            key_ = virtual_clock_state::key{c->now_ms + static_cast<double>(delay.count()),
                                            c->scheduled++};
            c->due.emplace(*key_, this);
        }
    }

    /** Never runs the handler, as a cancelled timer of every backend does; frees it now. */
    void cancel() override {
        // (the wall-clock path, today's, when the timer has no clock)
        unschedule();
        fire_ = {};
    }

    ~memory_timer() override { unschedule(); }

    /** Called by the clock, which has removed the timer's entry: posts the handler, as Asio
     * completes a timer. */
    void expire() {
        key_.reset();
        boost::asio::post(io_, [f = std::move(fire_)]() mutable { f(); });
    }

private:
    void unschedule() {
        if (const auto c = clock_.lock(); c && key_) {
            c->due.erase(*key_);
        }
        key_.reset();
    }
```

    The network runs them, and bounds every run (review N4):

```cpp
/** Runs the io_context's ready work, then fires the earliest timer due at or before `until_ms`,
 * moving the clock to it; false, with the clock at `until_ms`, when none is due. */
inline bool memory_network::step(double until_ms) {
    io_.restart();
    io_.poll();
    if (!clock_ || clock_->due.empty() || clock_->due.begin()->first.first > until_ms) {
        if (clock_) {
            clock_->now_ms = std::max(clock_->now_ms, until_ms);
        }
        return false;
    }
    const auto it = clock_->due.begin();
    clock_->now_ms = std::max(clock_->now_ms, it->first.first);
    memory_timer* const timer = it->second;
    clock_->due.erase(it);
    timer->expire();
    io_.restart();
    io_.poll();
    return true;
}

/** Moves the clock by `by`, firing every timer due meanwhile, in order; false when it stopped after
 * `max_steps` timers without reaching its end, which only a timer re-armed without delay does. */
inline bool memory_network::advance(std::chrono::milliseconds by) {
    const double until = now_ms() + static_cast<double>(by.count());
    for (std::size_t i = 0; i < max_steps; ++i) {
        if (!step(until)) {
            io_.restart();
            io_.poll();
            return true;
        }
    }
    return false;
}

/** How many timers wait on the virtual clock. */
inline std::size_t memory_network::scheduled() const { return clock_ ? clock_->due.size() : 0; }
```

    `now_ms()` returns the clock's `now_ms` (0 before its first timer) when `virtual_clock` is
    set, and `unix_seconds` follows it; else today's scaled wall clock. `virtual_clock_state` is
    declared before `memory_timer`, and the timer's members that call the network after it.
  - `open(pc)`: posts `opened()` on the channel of `pc` taken from `deferred_`. `connect_to`
    (`memory_backend.hpp:639-664`) records both ends in `deferred_` instead of posting their
    `opened()` when `defer_opens` is true.
  - `inject(pc, frame)`: posts, to the channel of `pc`, its `message` handler with a copy of
    `frame` in that end's resource, if the channel is open.
  - `fail(pc)`: posts the `state` handler of `pc` with `connection_state::failed`, then
    `close()` on its channel.
  - `make_peer_connection` calls `on_made(pc->id(), servers)` after recording the connection.
  - `peer_of(pc)`: `find(pc)`'s `remote_id()`, or `pc` when it has none.

- [ ] **Step 4: Run it to see it pass, and the suites that use the backend.**
  Run: `$B2 libs/trystero/test//memory_backend libs/trystero/test//room libs/trystero/test//allocation`
  and the same with `toolset=emscripten` and with `toolset=gcc-15 --build-dir=bin/gcc-15`.
  Expected: all pass; `allocation` unchanged (no hook set, no virtual clock, nothing allocated
  outside the resources).

- [ ] **Step 5: Commit.**

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: the memory backend's hooks, frames held in the channel's order" \
  -- test/support/memory_backend.hpp test/memory_backend_test.cpp test/Jamfile
```

### Task 2: `detail::connection` and `detail::binding`

**Files:**
- Create: `libs/trystero/include/webcpp/trystero/detail/connection.hpp`
- Modify: `libs/trystero/test/Jamfile` (nothing: `trystero/detail/*.hpp` is already compiled alone,
  `test/Jamfile` "headers-alone")
- Test: `libs/trystero/test/memory_backend_test.cpp` gains nothing; the connection is tested
  through rooms in Task 4. This task's own test is the headers-alone build and a direct test below.
- Create: `libs/trystero/test/connection_test.cpp`; Modify: `libs/trystero/test/Jamfile`

**Interfaces:**
- Consumes: Task 1's `memory_network` hooks; `protocol::decode_envelope`, `protocol::presence_frame`
  (`P/include/webcpp/trystero/protocol/envelope.hpp:77-148`); `backend::data_channel::low_threshold`.
- Produces:

```cpp
namespace webcpp::trystero::detail {

/** What a connection asks of its app. Every handler is in the app's resource. */
struct connection_callbacks {
    /** The remote advertised (`present`) or withdrew a room's token. */
    handler<void(std::string_view peer, std::string_view token, bool present)> presence;
    /** Whether to keep a frame of `token`, which no room bound to this connection has: the app
     * says no for a token of no room of it, and for a room whose dropping window runs. */
    handler<bool(std::string_view peer, std::string_view token)> keeps;
    /** A frame of `token` was kept. */
    handler<void(std::string_view peer, std::string_view token)> kept;
    /** The connection ended, with the reason; called once. */
    handler<void(std::string_view peer, std::string_view reason)> ended;
};

class binding;

class connection : public std::enable_shared_from_this<connection> {
public:
    static constexpr std::size_t max_remote_rooms = 64;
    static constexpr std::size_t max_kept_frames = 64;
    static constexpr std::chrono::milliseconds kept_frames_ttl{10'000};

    connection(backend::factory& factory, std::string_view peer,
               std::shared_ptr<backend::peer_connection> pc,
               std::shared_ptr<backend::data_channel> channel, connection_callbacks cb,
               logger log, std::pmr::memory_resource* memory);
    void start(std::span<const std::string_view> tokens);
    [[nodiscard]] std::string_view peer() const;
    [[nodiscard]] bool live() const;
    [[nodiscard]] bool advertised(std::string_view token) const;
    void send_presence(std::string_view token, bool present);
    bool send(std::span<const std::uint8_t> frame);
    [[nodiscard]] std::size_t buffered_amount() const;
    void attach(binding& b);
    void detach(binding& b, bool after_kick, std::chrono::milliseconds idle_timeout);
    void wait_drained(binding& b);
    std::pmr::deque<backend::bytes> take_kept(std::string_view token);
    void end(std::string_view reason);
};

class binding final : public channel_link, public std::enable_shared_from_this<binding> {
public:
    binding(const std::shared_ptr<connection>& c, std::string_view token,
            handler<void(backend::bytes&&, std::span<const std::uint8_t>)> deliver,
            std::pmr::memory_resource* room_memory);
    ~binding();
    [[nodiscard]] std::string_view token() const;
    [[nodiscard]] std::shared_ptr<connection> shared_connection() const;
    bool send(std::span<const std::uint8_t> room_frame) override;
    [[nodiscard]] std::size_t buffered_amount() const override;
    void wait_drained(handler<void(bool)> done) override;
    void deliver(backend::bytes&& frame, std::span<const std::uint8_t> action);
    void drained(bool open);
    void detach(bool after_kick, std::chrono::milliseconds idle_timeout);
};

}  // namespace webcpp::trystero::detail
```

- [ ] **Step 1: Write the failing test** `libs/trystero/test/connection_test.cpp`: two connections
  over a memory pair, one binding on each side for token `"T1"`, a frame of `T1` delivered to the
  binding, a frame of `T2` with no binding offered to `keeps`/`kept`, a presence frame reported to
  `presence`, a 65th token ending the connection, a malformed frame ending it, the drain order.

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The shared connection alone, over a memory pair: routing by token, kept frames, presence, the
// bound on advertised rooms, malformed frames, and the order of the drain waiters.

#include <webcpp/trystero/detail/connection.hpp>
#include <webcpp/trystero/protocol/envelope.hpp>

#include "support/memory_backend.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>

#include <functional>
#include <string>
#include <vector>

#include "require.hpp"

namespace asio = boost::asio;
namespace trystero = webcpp::trystero;
namespace detail = webcpp::trystero::detail;
using namespace std::chrono_literals;

namespace {

bool run_until(asio::io_context& io, const std::function<bool()>& done,
               std::chrono::milliseconds limit = 5s) {
    const auto end = std::chrono::steady_clock::now() + limit;
    while (!done() && std::chrono::steady_clock::now() < end) {
        io.run_for(10ms);  // lint-run: the memory network posts to it
        if (io.stopped()) {
            io.restart();
        }
    }
    return done();
}

/** What one side's connection reported. */
struct side {
    std::shared_ptr<detail::connection> c;
    std::vector<std::string> presence;  // "+T" or "-T"
    std::vector<std::string> kept;
    std::vector<std::string> delivered;  // tokens
    std::string ended;
};

detail::connection_callbacks callbacks(side& s) {
    auto* m = std::pmr::get_default_resource();
    const std::pmr::polymorphic_allocator<> a(m);
    return {
        .presence = {std::allocator_arg, a,
                     [&s](std::string_view, std::string_view t, bool present) {
                         s.presence.push_back((present ? "+" : "-") + std::string(t));
                     }},
        .keeps = {std::allocator_arg, a,
                  [](std::string_view, std::string_view t) { return t != "nobody"; }},
        .kept = {std::allocator_arg, a,
                 [&s](std::string_view, std::string_view t) { s.kept.emplace_back(t); }},
        .ended = {std::allocator_arg, a,
                  [&s](std::string_view, std::string_view why) { s.ended = why; }},
    };
}

/** Two connections over a memory pair, each started with the tokens given. */
struct link_pair {
    asio::io_context io;
    trystero::test::memory_network net{io};
    side a;
    side b;

    link_pair(std::span<const std::string_view> a_tokens,
              std::span<const std::string_view> b_tokens);
};
```

  `link_pair`'s constructor:

```cpp
link_pair::link_pair(std::span<const std::string_view> a_tokens,
                     std::span<const std::string_view> b_tokens) {
    net.virtual_clock = true;  // the keep timer waits for advance()
    auto* m = std::pmr::get_default_resource();
    const std::pmr::polymorphic_allocator<> alloc(m);
    auto offerer = net.make_peer_connection({}, m);
    auto answerer = net.make_peer_connection({}, m);
    auto offer_end = offerer->create_data_channel("data");
    std::shared_ptr<trystero::backend::data_channel> answer_end;
    answerer->set_handlers(
        {.ice_candidate = {},
         .data_channel = [&answer_end](std::shared_ptr<trystero::backend::data_channel> c) {
             answer_end = std::move(c);
         },
         .state = {}});
    std::pmr::string offer(m);
    std::pmr::string answer(m);
    offerer->set_local_description(
        [&offer](trystero::backend::error_code, std::string_view, std::string_view sdp) { offer = sdp; });
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return !offer.empty(); })));
    answerer->set_remote_description("offer", offer, [](trystero::backend::error_code) {});
    answerer->set_local_description(
        [&answer](trystero::backend::error_code, std::string_view, std::string_view sdp) { answer = sdp; });
    webcpp::test::require(BOOST_TEST(run_until(io, [&] { return !answer.empty(); })));
    offerer->set_remote_description("answer", answer, [](trystero::backend::error_code) {});
    webcpp::test::require(BOOST_TEST(run_until(io, [&] {
        return offer_end->is_open() && answer_end && answer_end->is_open();
    })));
    a.c = std::allocate_shared<detail::connection>(alloc, net, "B", offerer, offer_end, callbacks(a),
                                                   detail::logger{}, m);
    b.c = std::allocate_shared<detail::connection>(alloc, net, "A", answerer, answer_end,
                                                   callbacks(b), detail::logger{}, m);
    a.c->start(a_tokens);
    b.c->start(b_tokens);
}
```

  The cases:

```cpp
void start_advertises_every_token() {
    const std::string_view a_tokens[] = {"T1", "T2"};
    const std::string_view b_tokens[] = {"T1"};
    link_pair p(a_tokens, b_tokens);
    BOOST_TEST(run_until(p.io, [&] { return p.b.presence.size() == 2 && p.a.presence.size() == 1; }));
    BOOST_TEST(p.b.c->advertised("T1") && p.b.c->advertised("T2") && !p.b.c->advertised("T3"));
}

void a_bound_token_reaches_its_binding_and_another_is_kept() {
    const std::string_view tokens[] = {"T1"};
    link_pair p(tokens, tokens);
    std::vector<std::string> got;
    auto* m = std::pmr::get_default_resource();
    const std::pmr::polymorphic_allocator<> alloc(m);
    const auto bound = std::allocate_shared<detail::binding>(
        alloc, p.b.c, "T1",
        trystero::handler<void(trystero::backend::bytes&&, std::span<const std::uint8_t>)>(
            std::allocator_arg, alloc,
            [&got](trystero::backend::bytes&&, std::span<const std::uint8_t> action) {
                got.emplace_back(action.begin(), action.end());
            }),
        m);
    const std::uint8_t action[] = {2, 0};
    p.a.c->send(trystero::protocol::room_frame("T1", action, m));
    p.a.c->send(trystero::protocol::room_frame("T2", action, m));
    p.a.c->send(trystero::protocol::room_frame("nobody", action, m));
    BOOST_TEST(run_until(p.io, [&] { return got.size() == 1 && p.b.kept.size() == 1; }));
    BOOST_TEST_EQ(p.b.kept.at(0), "T2");
    BOOST_TEST_EQ(p.b.c->take_kept("T2").size(), 1U);
    BOOST_TEST(p.b.c->take_kept("nobody").empty());
}

void a_65th_advertised_token_ends_the_connection() {
    const std::string_view tokens[] = {"T1"};
    link_pair p(tokens, tokens);
    auto* m = std::pmr::get_default_resource();
    for (int i = 0; i < 64; ++i) {
        p.a.c->send(trystero::protocol::presence_frame("X" + std::to_string(i), true, m));
    }
    BOOST_TEST(run_until(p.io, [&] { return !p.b.ended.empty(); }));
}

void a_malformed_frame_ends_the_connection() {
    const std::string_view tokens[] = {"T1"};
    link_pair p(tokens, tokens);
    const std::uint8_t bad[] = {3, 0, 1, 'x'};
    p.a.c->send(bad);
    BOOST_TEST(run_until(p.io, [&] { return !p.b.ended.empty(); }));
}

void kept_frames_are_bounded_and_dropped_after_10_s() {
    const std::string_view tokens[] = {"T1"};
    link_pair p(tokens, tokens);
    auto* m = std::pmr::get_default_resource();
    const std::uint8_t action[] = {2, 0};
    for (int i = 0; i < 65; ++i) {
        p.a.c->send(trystero::protocol::room_frame("T2", action, m));
    }
    BOOST_TEST(run_until(p.io, [&] { return p.b.kept.size() == 64; }));
    BOOST_TEST_EQ(p.b.c->kept_count(), 64U);
    p.net.advance(9'999ms);
    BOOST_TEST_EQ(p.b.c->kept_count(), 64U);
    p.net.advance(1ms);  // 10 s after the first was kept
    BOOST_TEST_EQ(p.b.c->kept_count(), 0U);
    BOOST_TEST(p.b.c->take_kept("T2").empty());
}

// A connection whose channel is closing or closed is not live, so the app does not reuse it
// (review M6).
void a_closing_channel_is_not_live() {
    const std::string_view tokens[] = {"T1"};
    link_pair p(tokens, tokens);
    BOOST_TEST(p.a.c->live());
    p.net.fail(p.net.peer_of(0));
    BOOST_TEST(run_until(p.io, [&] { return !p.a.c->live(); }));
}
```

  `p.a.c->send` is the raw connection send a binding uses; `main` runs the six and returns
  `boost::report_errors()`. Add to `test/Jamfile`:

```jam
# The shared connection alone, over a memory pair.
webcpp.run connection : connection_test.cpp ;
```

- [ ] **Step 2: Run it to see it fail.** `$B2 libs/trystero/test//connection`: Expected: no file
  `webcpp/trystero/detail/connection.hpp`.

- [ ] **Step 3: Write `detail/connection.hpp`.** Its comment block and the two classes:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TRYSTERO_DETAIL_CONNECTION_HPP
#define WEBCPP_TRYSTERO_DETAIL_CONNECTION_HPP

#include <webcpp/trystero/config.hpp>

#include <webcpp/trystero/backend.hpp>
#include <webcpp/trystero/detail/action_wire.hpp>
#include <webcpp/trystero/detail/log.hpp>
#include <webcpp/trystero/handler.hpp>
#include <webcpp/trystero/protocol/envelope.hpp>

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace webcpp::trystero::detail {

/** What a connection asks of its app. Every handler is in the app's resource. */
struct connection_callbacks {
    /** The remote advertised (`present`) or withdrew a room's token. */
    handler<void(std::string_view peer, std::string_view token, bool present)> presence;
    /** Whether to keep a frame of `token`, which no room bound to this connection has. */
    handler<bool(std::string_view peer, std::string_view token)> keeps;
    /** A frame of `token` was kept. */
    handler<void(std::string_view peer, std::string_view token)> kept;
    /** The connection ended, with the reason; called once. */
    handler<void(std::string_view peer, std::string_view reason)> ended;
};

class binding;

/**
 * One remote peer's connection, shared by every room of the app bound to it: it decodes each
 * frame's envelope, routes a room frame to the binding of its token, keeps a few frames for a room
 * of the app not bound yet, and records the rooms the remote advertises.
 *
 * | Frame                               | Done                                                      |
 * | does not decode                     | the connection ends, for every room                      |
 * | presence(1, T), T new               | recorded; a 65th distinct token ends the connection      |
 * | presence(0, T)                      | forgotten; frames kept for T dropped                     |
 * | room frame, T bound                 | to the binding of T                                      |
 * | room frame, T kept by the app       | kept: 64 per connection, all dropped 10 s after the first |
 * | room frame, any other T             | dropped                                                  |
 *
 * Tip: it holds plain pointers to its bindings, which remove themselves before they go, so that it
 * keeps nothing of a room's resource after the room has ended (the spec's section 4.1).
 */
class connection : public std::enable_shared_from_this<connection> {
public:
    static constexpr std::size_t max_remote_rooms = 64;
    static constexpr std::size_t max_kept_frames = 64;
    static constexpr std::chrono::milliseconds kept_frames_ttl{10'000};

    connection(backend::factory& factory, std::string_view peer,
               std::shared_ptr<backend::peer_connection> pc,
               std::shared_ptr<backend::data_channel> channel, connection_callbacks cb, logger log,
               std::pmr::memory_resource* memory)
        : factory_(factory), memory_(memory), peer_(peer, memory), pc_(std::move(pc)),
          channel_(std::move(channel)), cb_(std::move(cb)), log_(std::move(log)),
          remote_tokens_(memory), bound_(memory), kept_(memory), drain_order_(memory),
          attached_(memory) {}

    /** Takes over the channel's and the connection's events, which replaces the handlers a room's
     * negotiation set, and advertises `tokens`. */
    void start(std::span<const std::string_view> tokens) {
        const auto weak = weak_from_this();
        const std::pmr::polymorphic_allocator<> a(memory_);
        channel_->set_handlers({
            .open = {},
            .message = {std::allocator_arg, a,
                        [weak](backend::bytes frame) {
                            if (const auto self = weak.lock()) {
                                self->on_message(std::move(frame));
                            }
                        }},
            .close = {std::allocator_arg, a,
                      [weak] {
                          if (const auto self = weak.lock()) {
                              self->end("data channel closed");
                          }
                      }},
            .buffered_amount_low = {std::allocator_arg, a,
                                    [weak] {
                                        if (const auto self = weak.lock()) {
                                            self->on_drained(true);
                                        }
                                    }},
        });
        pc_->set_handlers({
            .ice_candidate = {},
            .data_channel = {},
            .state = {std::allocator_arg, a,
                      [weak](backend::connection_state state) {
                          if (const auto self = weak.lock()) {
                              self->on_state(state);
                          }
                      }},
        });
        log_("webrtc", "connection with ", peer_, " adopted");
        for (const auto token : tokens) {
            send_presence(token, true);
        }
    }

    [[nodiscard]] std::string_view peer() const { return peer_; }

    /** Not stale, as Trystero says it: not ended, and its channel neither closing nor closed. A
     * connection is adopted once its channel is open, so "still opening" does not arise here
     * (review M6). */
    [[nodiscard]] bool live() const { return !ended_ && channel_->is_open(); }

    /** How many frames are kept for rooms not bound yet. */
    [[nodiscard]] std::size_t kept_count() const { return kept_.size(); }

    [[nodiscard]] bool advertised(std::string_view token) const {
        return remote_tokens_.contains(token);
    }

    void send_presence(std::string_view token, bool present) {
        if (!ended_ && channel_->is_open()) {
            channel_->send(protocol::presence_frame(token, present, memory_));
        }
    }

    bool send(std::span<const std::uint8_t> frame) {
        if (ended_ || !channel_->is_open()) {
            return false;
        }
        channel_->send(frame);
        return true;
    }

    [[nodiscard]] std::size_t buffered_amount() const {
        return ended_ ? 0 : channel_->buffered_amount();
    }

    /** Registers a room's binding by its token, and keeps the connection from idling. */
    void attach(binding& b);

    /** Removes a binding; with none left, ends the connection after a kick or a zero idle time,
     * else after `idle_timeout`. */
    void detach(binding& b, bool after_kick, std::chrono::milliseconds idle_timeout);

    /** Gives the binding a place in the drain order, once. */
    void wait_drained(binding& b) {
        if (std::find(drain_order_.begin(), drain_order_.end(), &b) == drain_order_.end()) {
            drain_order_.push_back(&b);
        }
    }

    /** The frames kept for `token`, in their order; they are no longer kept. */
    std::pmr::deque<backend::bytes> take_kept(std::string_view token) {
        std::pmr::deque<backend::bytes> out(memory_);
        for (auto it = kept_.begin(); it != kept_.end();) {
            if (it->first == token) {
                out.push_back(std::move(it->second));
                it = kept_.erase(it);
            } else {
                ++it;
            }
        }
        if (kept_.empty()) {
            kept_timer_.reset();
            // A cleared deque keeps blocks (libc++ keeps up to two): a new one frees them (I5).
            kept_ = decltype(kept_)(memory_);
        }
        return out;
    }

    /** Ends the connection for every room, once: its handlers cleared, its bindings let go, the
     * connection and its channel closed, `ended` called last. */
    void end(std::string_view reason);

private:
    friend class binding;
    void remove(binding& b);
    void on_message(backend::bytes&& frame);
    void on_state(backend::connection_state state);
    void on_drained(bool open);
    void keep(std::string_view token, backend::bytes&& frame);

    backend::factory& factory_;
    std::pmr::memory_resource* memory_;
    std::pmr::string peer_;
    std::shared_ptr<backend::peer_connection> pc_;
    std::shared_ptr<backend::data_channel> channel_;
    connection_callbacks cb_;
    logger log_;
    std::pmr::set<std::pmr::string, std::less<>> remote_tokens_;
    std::pmr::map<std::pmr::string, binding*, std::less<>> bound_;
    std::pmr::deque<std::pair<std::pmr::string, backend::bytes>> kept_;
    std::shared_ptr<backend::timer> kept_timer_;
    std::shared_ptr<backend::timer> idle_timer_;
    std::shared_ptr<backend::timer> disconnect_timer_;
    std::pmr::deque<binding*> drain_order_;
    std::pmr::set<binding*> attached_;
    bool ended_ = false;
};
```

  `on_message`, the routing table of the comment:

```cpp
inline void connection::on_message(backend::bytes&& frame) {
    const auto envelope = protocol::decode_envelope(frame);
    if (!envelope) {
        end("invalid or incompatible room frame");
        return;
    }
    const std::pmr::string token(envelope->token, memory_);
    if (envelope->type == protocol::envelope::kind::presence) {
        if (envelope->present) {
            if (!remote_tokens_.contains(token) && remote_tokens_.size() >= max_remote_rooms) {
                end("too many advertised rooms");
                return;
            }
            remote_tokens_.insert(token);
        } else {
            remote_tokens_.erase(token);
            std::erase_if(kept_, [&token](const auto& k) { return k.first == token; });
        }
        // The app may end this connection from inside the call: it stays alive until it returns.
        const auto self = shared_from_this();
        cb_.presence(peer_, token, envelope->present);
        return;
    }
    if (const auto it = bound_.find(token); it != bound_.end()) {
        const auto action = envelope->payload;
        it->second->deliver(std::move(frame), action);
        return;
    }
    if (cb_.keeps(peer_, token)) {
        keep(token, std::move(frame));
    }
}
```

  The same `self` guard precedes every callback the connection makes; `end` moves `ended` out of
  `cb_` before calling it, and keeps the other callbacks, which a call after `ended_` no longer
  reaches.

  `keep`:

```cpp
inline void connection::keep(std::string_view token, backend::bytes&& frame) {
    if (kept_.size() >= max_kept_frames) {
        return;
    }
    kept_.emplace_back(std::pmr::string(token, memory_), std::move(frame));
    if (!kept_timer_) {
        // All kept frames go 10 s after the first, as Trystero's (C/shared-peer.mjs:425-428).
        kept_timer_ = factory_.make_timer(memory_);
        kept_timer_->start(kept_frames_ttl,
                           {std::allocator_arg, std::pmr::polymorphic_allocator<>(memory_),
                            [weak = weak_from_this()] {
                                if (const auto self = weak.lock()) {
                                    self->kept_ = decltype(self->kept_)(self->memory_);
                                    self->kept_timer_.reset();
                                }
                            }});
    }
    const auto self = shared_from_this();
    cb_.kept(peer_, token);
}
```

  `on_state` is `peer_link::on_state` (`P/include/webcpp/trystero/detail/peer_link.hpp:188-208`)
  with `end` for `peer_link::end`. `on_drained(open)`:

```cpp
inline void connection::on_drained(bool open) {
    // Every binding waiting at this drain gets its turn, in the order it began to wait. A turn may
    // detach another binding of the snapshot, even destroy it: each is checked before its turn
    // (review M5).
    auto order = std::exchange(drain_order_, std::pmr::deque<binding*>(memory_));
    for (auto* b : order) {
        if (attached_.contains(b)) {
            b->drained(open);
        }
    }
}
```

  `attach`, `detach` and `end`, written after `binding` in the same header:

```cpp
inline void connection::attach(binding& b) {
    bound_.insert_or_assign(std::pmr::string(b.token(), memory_), &b);
    attached_.insert(&b);
    idle_timer_.reset();
}

/** Forgets a binding, and nothing else: its maps, its place in the drain order. */
inline void connection::remove(binding& b) {
    if (const auto it = bound_.find(b.token()); it != bound_.end() && it->second == &b) {
        bound_.erase(it);
    }
    attached_.erase(&b);
    std::erase(drain_order_, &b);
}

inline void connection::detach(binding& b, bool after_kick, std::chrono::milliseconds idle_timeout) {
    remove(b);
    if (ended_ || !bound_.empty()) {
        return;
    }
    if (after_kick || idle_timeout <= std::chrono::milliseconds::zero()) {
        end(after_kick ? "its last room kicked the peer" : "no room left");
        return;
    }
    idle_timer_ = factory_.make_timer(memory_);
    idle_timer_->start(idle_timeout,
                       {std::allocator_arg, std::pmr::polymorphic_allocator<>(memory_),
                        [weak = weak_from_this()] {
                            if (const auto self = weak.lock()) {
                                self->end("idle");
                            }
                        }});
}

inline void connection::end(std::string_view reason_view) {
    if (ended_) {
        return;
    }
    ended_ = true;
    const std::pmr::string reason(reason_view, memory_);
    log_("webrtc", "connection with ", peer_, " ended: ", reason);
    const auto self = shared_from_this();
    idle_timer_.reset();
    kept_timer_.reset();
    disconnect_timer_.reset();
    channel_->set_handlers({});
    pc_->set_handlers({});
    channel_->close();
    pc_->close();
    // The bindings are let go before anyone is told: a room may destroy its binding, or itself,
    // from inside `ended` (Trystero's clear, C/shared-peer.mjs:170-180). A waiter's failure may
    // detach another binding of the snapshot: each is checked first (review M5).
    bound_.clear();
    auto order = std::exchange(drain_order_, std::pmr::deque<binding*>(memory_));
    for (auto* b : order) {
        if (attached_.contains(b)) {
            b->drained(false);
        }
    }
    attached_.clear();
    kept_ = decltype(kept_)(memory_);
    auto ended = std::move(cb_.ended);
    ended(peer_, reason);
}
```

  `binding`:

```cpp
/**
 * A room's use of one connection: the action wire's link to a peer, whose frames carry the room's
 * token. It lives in the room's resource and owns its drain waiters; the connection holds only a
 * pointer to it, which it removes when it detaches.
 */
class binding final : public channel_link, public std::enable_shared_from_this<binding> {
public:
    binding(const std::shared_ptr<connection>& c, std::string_view token,
            handler<void(backend::bytes&&, std::span<const std::uint8_t>)> deliver,
            std::pmr::memory_resource* room_memory)
        : connection_(c), token_(token, room_memory), deliver_(std::move(deliver)),
          waiters_(room_memory) {
        c->attach(*this);
    }

    binding(const binding&) = delete;
    binding& operator=(const binding&) = delete;

    /** Leaves the connection quietly: its waiters freed uncalled, its place in the connection
     * forgotten, and nothing ended, idled, closed or called back, since it may run while the
     * io_context is destroyed (review I10; the spec's section 4.2, M10). */
    ~binding() {
        if (std::exchange(detached_, true)) {
            return;
        }
        waiters_.clear();
        deliver_ = {};
        if (const auto c = connection_.lock()) {
            c->remove(*this);
        }
    }

    [[nodiscard]] std::string_view token() const { return token_; }

    [[nodiscard]] std::shared_ptr<connection> shared_connection() const {
        return detached_ ? nullptr : connection_.lock();
    }

    bool send(std::span<const std::uint8_t> room_frame) override {
        const auto c = shared_connection();
        return c && c->send(room_frame);
    }

    [[nodiscard]] std::size_t buffered_amount() const override {
        const auto c = shared_connection();
        return c ? c->buffered_amount() : 0;
    }

    void wait_drained(handler<void(bool)> done) override {
        const auto c = shared_connection();
        if (!c || !c->live()) {
            done(false);
            return;
        }
        if (c->buffered_amount() <= backend::data_channel::low_threshold) {
            done(true);
            return;
        }
        waiters_.push_back(std::move(done));
        c->wait_drained(*this);
    }

    /** A room frame of this binding's token, from the connection. */
    void deliver(backend::bytes&& frame, std::span<const std::uint8_t> action) {
        if (!detached_) {
            deliver_(std::move(frame), action);
        }
    }

    /** This binding's turn at a drain: each of its waiters, in its order. */
    void drained(bool open) {
        for (auto& w : std::exchange(waiters_, decltype(waiters_)(waiters_.get_allocator()))) {
            w(open);
        }
    }

    /** Leaves the connection: every waiter is told `false` and freed, the delivery dropped, and
     * the connection idles or ends when no binding is left. */
    void detach(bool after_kick, std::chrono::milliseconds idle_timeout) {
        if (std::exchange(detached_, true)) {
            return;
        }
        drained(false);
        deliver_ = {};
        if (const auto c = connection_.lock()) {
            c->detach(*this, after_kick, idle_timeout);
        }
    }

private:
    std::weak_ptr<connection> connection_;
    std::pmr::string token_;
    handler<void(backend::bytes&&, std::span<const std::uint8_t>)> deliver_;
    std::pmr::vector<handler<void(bool)>> waiters_;
    bool detached_ = false;
};
```

- [ ] **Step 4: Run it to see it pass.**
  Run: `$B2 libs/trystero/test//connection`, with `toolset=emscripten`, and with
  `toolset=gcc-15 --build-dir=bin/gcc-15`; and the headers alone: `$B2 libs/trystero/test` (its
  `headers-alone` targets compile `detail/connection.hpp` alone).
  Expected: pass.

- [ ] **Step 5: Commit.**

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: a connection shared by bindings, routed by room token" \
  -- include/webcpp/trystero/detail/connection.hpp test/connection_test.cpp test/Jamfile
```

### Task 3: `app_config`, `room_options` and `detail::app_impl`

**Files:**
- Create: `libs/trystero/include/webcpp/trystero/options.hpp`
- Create: `libs/trystero/include/webcpp/trystero/detail/app_impl.hpp`
- Test: through the rooms, in Task 4.

**Interfaces:**
- Consumes: Task 2's `connection`, `binding`, `connection_callbacks`.
- Produces:

```cpp
namespace webcpp::trystero {

struct app_config {
    std::pmr::string app_id = {};
    std::pmr::string self_id = {};
    std::pmr::vector<std::pmr::string> relay_urls = {};
    std::size_t relay_redundancy = 5;
    std::pmr::vector<backend::ice_server> ice_servers = {};
    std::chrono::milliseconds idle_timeout{123'333};
    detail::logger::sink log = {};
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();
};

struct room_options {
    std::pmr::string password = {};
    std::chrono::milliseconds handshake_timeout{10'000};
    std::size_t max_receive_bytes = std::size_t{256} * 1024 * 1024;
};

}  // namespace webcpp::trystero

namespace webcpp::trystero::detail {

class room_endpoint {
public:
    virtual void presence_changed(std::string_view peer, bool present) = 0;
    [[nodiscard]] virtual bool keeps_frames_from(std::string_view peer) const = 0;
    virtual void frame_kept(std::string_view peer) = 0;
    virtual void connection_ended(std::string_view peer, std::string_view reason) = 0;
protected:
    room_endpoint() = default;
    room_endpoint(const room_endpoint&) = default;
    room_endpoint& operator=(const room_endpoint&) = default;
    ~room_endpoint() = default;
};

class app_impl : public std::enable_shared_from_this<app_impl> {
public:
    using executor_type = boost::asio::any_io_executor;
    app_impl(executor_type ex, backend::factory& factory, app_config config);
    [[nodiscard]] const executor_type& get_executor() const;
    [[nodiscard]] backend::factory& factory() const;
    [[nodiscard]] std::pmr::memory_resource* memory() const;
    [[nodiscard]] const logger& log() const;
    [[nodiscard]] std::string_view app_id() const;
    [[nodiscard]] std::string_view self_id() const;
    [[nodiscard]] std::span<const std::pmr::string> relay_urls() const;
    [[nodiscard]] std::span<const backend::ice_server> ice_servers() const;
    [[nodiscard]] std::chrono::milliseconds idle_timeout() const;
    [[nodiscard]] std::span<const std::pmr::string> setup_reports() const;
    void add_user();
    void release_user();
    [[nodiscard]] std::size_t counted_rooms() const;
    [[nodiscard]] std::shared_ptr<room_endpoint> find_room(std::string_view room_id) const;
    void register_room(std::string_view room_id, std::string_view token,
                       const std::shared_ptr<room_endpoint>& room);
    void unregister_room(std::string_view token, const room_endpoint* room);
    [[nodiscard]] std::shared_ptr<connection> find(std::string_view peer) const;
    [[nodiscard]] std::pmr::vector<std::shared_ptr<connection>> live_connections() const;
    std::shared_ptr<connection> adopt(std::string_view peer,
                                      std::shared_ptr<backend::peer_connection> pc,
                                      std::shared_ptr<backend::data_channel> channel);
};

}  // namespace webcpp::trystero::detail
```

- [ ] **Step 1: Write `options.hpp`.** `app_config` and `room_options` exactly as the Interfaces,
  with Doc Comments taken from `room_config`'s members (`P/include/webcpp/trystero/room.hpp:99-183`):
  `app_id`, `relay_urls`, `relay_redundancy`, `ice_servers`, `self_id` (now "this program's id in
  every room of the app, Trystero's `selfId`; empty means 20 random characters, drawn once for the
  app"; the paragraph advising an id per room goes), `log` ("where the app's and its rooms'
  diagnostics go"), `memory` ("the resource every allocation of the app goes through: its state, its
  connections, the peer connections and data channels its rooms' negotiations make, the frames
  received on them, the sentences of its log"); `idle_timeout` ("how long a connection with no room
  bound to it after a departure stays open, Trystero's `sharedPeerIdleMs`; 0 closes it at once");
  `password`, `handshake_timeout`, `max_receive_bytes` from `room_config`. `room_options` has no
  `memory` until Task 13.

- [ ] **Step 2: Write `detail/app_impl.hpp`.** Its constructor takes over what `room_impl`'s
  constructor and `set_aside_unusable_configuration` do today with the app's part of the
  configuration (`room.hpp:578-598,1037-1062`). The public `default_ice_servers`
  (`room.hpp:216-234`) moves, unchanged, to `options.hpp`, and `detail::copy_ice_servers`
  (`signaling.hpp:44-56`) to `detail/ice_servers.hpp`, which already holds `lowercase_scheme` and
  `ice_server_fault`, so that `app_impl.hpp` reaches all four without including `room.hpp`. Each
  report is kept as a sentence in `setup_reports_` (the app's resource) and logged; until Task 5c the
  standalone room posts them to its own `on_error`, from Task 5c `app::on_error` does.

```cpp
/**
 * What a room of the app is to the app: the calls a connection's events become. A room implements
 * it; the app reaches it through the registry, which the room leaves before it ends.
 */
class room_endpoint {
public:
    /** The remote advertised this room's token (`present`) or withdrew it. */
    virtual void presence_changed(std::string_view peer, bool present) = 0;
    /** Whether the connection keeps this room's frames from `peer` while the room is not bound to
     * it: false while the room drops them (its dropping window). */
    [[nodiscard]] virtual bool keeps_frames_from(std::string_view peer) const = 0;
    /** A frame of this room's token from `peer` was kept. */
    virtual void frame_kept(std::string_view peer) = 0;
    /** The app's connection to `peer` ended. */
    virtual void connection_ended(std::string_view peer, std::string_view reason) = 0;

protected:
    room_endpoint() = default;
    room_endpoint(const room_endpoint&) = default;
    room_endpoint& operator=(const room_endpoint&) = default;
    ~room_endpoint() = default;
};

/**
 * The state an `app` and its rooms share: the configuration, the self id, the rooms by token, and
 * one connection per remote peer. Every allocation goes through `app_config::memory`.
 *
 * Tip: its users are the `app` handle and every room not yet ended; when the last goes, it ends
 * every connection at that call. Its destructor ends nothing: it may run while the io_context is
 * destroyed, from the last operation pending there (the spec's section 4.2, "The app's users").
 */
class app_impl : public std::enable_shared_from_this<app_impl> {
public:
    using executor_type = boost::asio::any_io_executor;

    /** Copies the configuration into its resource, draws the self id unless it names one, and sets
     * aside what no peer could use, each kept as a report (the room's today, room.hpp:578-598,
     * 1037-1062). */
    app_impl(executor_type ex, backend::factory& factory, app_config config)
        : memory_(config.memory), ex_(std::move(ex)), factory_(factory),
          log_(std::move(config.log), memory_), app_id_(config.app_id, memory_),
          self_id_(config.self_id, memory_), relay_urls_(config.relay_urls, memory_),
          ice_servers_(copy_ice_servers(config.ice_servers, memory_)),
          idle_timeout_(config.idle_timeout), setup_reports_(memory_), rooms_(memory_),
          connections_(memory_) {
        if (self_id_.size() > protocol::max_peer_id_bytes) {
            self_id_.clear();
            set_aside("self_id is longer than any peer accepts, so the app uses a random one");
        }
        if (self_id_.empty()) {
            self_id_ = random_id(protocol::peer_id_size);
        }
        if (idle_timeout_ < std::chrono::milliseconds::zero()) {
            idle_timeout_ = std::chrono::milliseconds::zero();
            set_aside("idle_timeout is negative, so the app closes an unused connection at once");
        }
        if (ice_servers_.empty()) {
            ice_servers_ = default_ice_servers(memory_);
        }
        for (auto& server : ice_servers_) {
            for (auto& url : server.urls) {
                lowercase_scheme(url);
            }
        }
        for (auto it = ice_servers_.begin(); it != ice_servers_.end();) {
            const auto fault = ice_server_fault(*it);
            if (fault.empty()) {
                ++it;
                continue;
            }
            std::pmr::string what("the ICE server ", memory_);
            what += it->urls.empty() ? std::string_view("(no URL)") : std::string_view(it->urls[0]);
            what += ' ';
            what += fault;
            what += ", so the app goes on without it";
            set_aside(what);
            it = ice_servers_.erase(it);
        }
        if (relay_urls_.empty()) {
            for (const auto url : protocol::derive_relays(app_id_, protocol::default_nostr_relays,
                                                          config.relay_redundancy, memory_)) {
                relay_urls_.emplace_back(url);
            }
        }
    }

    [[nodiscard]] const executor_type& get_executor() const { return ex_; }
    [[nodiscard]] backend::factory& factory() const { return factory_; }
    [[nodiscard]] std::pmr::memory_resource* memory() const { return memory_; }
    [[nodiscard]] const logger& log() const { return log_; }
    [[nodiscard]] std::string_view app_id() const { return app_id_; }
    [[nodiscard]] std::string_view self_id() const { return self_id_; }
    [[nodiscard]] std::span<const std::pmr::string> relay_urls() const { return relay_urls_; }
    [[nodiscard]] std::span<const backend::ice_server> ice_servers() const { return ice_servers_; }
    [[nodiscard]] std::chrono::milliseconds idle_timeout() const { return idle_timeout_; }
    [[nodiscard]] std::span<const std::pmr::string> setup_reports() const { return setup_reports_; }

    [[nodiscard]] std::pmr::vector<std::shared_ptr<connection>> live_connections() const {
        std::pmr::vector<std::shared_ptr<connection>> out(memory_);
        for (const auto& [peer, c] : connections_) {
            if (c->live()) {
                out.push_back(c);
            }
        }
        return out;
    }

    void add_user() { ++users_; }

    void release_user() {
        if (users_ == 0 || --users_ != 0) {
            return;
        }
        auto connections = std::exchange(connections_, decltype(connections_)(memory_));
        for (auto& [peer, c] : connections) {
            c->end("the app has no room left");
        }
    }

    [[nodiscard]] std::size_t counted_rooms() const { return rooms_.size(); }

    /** Registers a room by a weak reference (the spec's section 4.1): its control block is in the
     * room's resource, so the room unregisters before its state goes, in every way it ends, its
     * destructor included (review I10). */
    void register_room(std::string_view room_id, std::string_view token,
                       const std::shared_ptr<room_endpoint>& room) {
        rooms_.insert_or_assign(std::pmr::string(token, memory_),
                                entry{std::pmr::string(room_id, memory_), room, room.get()});
    }

    /** Forgets the room of `token`, if it is still `room`'s entry (review M7). */
    void unregister_room(std::string_view token, const room_endpoint* room) {
        if (const auto it = rooms_.find(token); it != rooms_.end() && it->second.raw == room) {
            rooms_.erase(it);
        }
    }

    [[nodiscard]] std::shared_ptr<room_endpoint> find_room(std::string_view room_id) const {
        for (const auto& [token, e] : rooms_) {
            if (e.room_id == room_id) {
                return e.room.lock();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::shared_ptr<connection> find(std::string_view peer) const {
        const auto it = connections_.find(peer);
        return it != connections_.end() && it->second->live() ? it->second : nullptr;
    }

    /** The connection a room's negotiation opened becomes the app's, or, when the app already has
     * a live one to that peer, is closed and the existing one returned (C/shared-peer.mjs:105-108). */
    std::shared_ptr<connection> adopt(std::string_view peer,
                                      std::shared_ptr<backend::peer_connection> pc,
                                      std::shared_ptr<backend::data_channel> channel) {
        if (auto existing = find(peer)) {
            channel->set_handlers({});
            pc->set_handlers({});
            channel->close();
            pc->close();
            log_("webrtc", "a second connection with ", peer, " closed: the first is shared");
            return existing;
        }
        const auto weak = weak_from_this();
        auto c = std::allocate_shared<connection>(
            allocator(), factory_, peer, std::move(pc), std::move(channel),
            connection_callbacks{
                .presence = bind<void(std::string_view, std::string_view, bool)>(
                    [weak](std::string_view p, std::string_view token, bool present) {
                        if (const auto self = weak.lock()) {
                            if (const auto room = self->room_of(token)) {
                                room->presence_changed(p, present);
                            }
                        }
                    }),
                .keeps = bind<bool(std::string_view, std::string_view)>(
                    [weak](std::string_view p, std::string_view token) {
                        const auto self = weak.lock();
                        const auto room = self ? self->room_of(token) : nullptr;
                        return room != nullptr && room->keeps_frames_from(p);
                    }),
                .kept = bind<void(std::string_view, std::string_view)>(
                    [weak](std::string_view p, std::string_view token) {
                        if (const auto self = weak.lock()) {
                            if (const auto room = self->room_of(token)) {
                                room->frame_kept(p);
                            }
                        }
                    }),
                .ended = bind<void(std::string_view, std::string_view)>(
                    [weak](std::string_view p, std::string_view reason) {
                        if (const auto self = weak.lock()) {
                            self->on_connection_end(p, reason);
                        }
                    }),
            },
            log_, memory_);
        connections_.insert_or_assign(std::pmr::string(peer, memory_), c);
        std::pmr::vector<std::string_view> tokens(memory_);
        for (const auto& [token, e] : rooms_) {
            tokens.emplace_back(token);
        }
        c->start(tokens);
        return c;
    }

private:
    struct entry {
        std::pmr::string room_id;
        std::weak_ptr<room_endpoint> room;
        const room_endpoint* raw;  // identity only, for unregister_room; never dereferenced
    };

    std::shared_ptr<room_endpoint> room_of(std::string_view token) const {
        const auto it = rooms_.find(token);
        return it == rooms_.end() ? nullptr : it->second.room.lock();
    }

    void on_connection_end(std::string_view peer_view, std::string_view reason_view) {
        const std::pmr::string peer(peer_view, memory_);
        const std::pmr::string reason(reason_view, memory_);
        connections_.erase(peer);
        // A room may leave the registry from inside the call: the rooms are locked first, each
        // kept alive across its call.
        std::pmr::vector<std::shared_ptr<room_endpoint>> rooms(memory_);
        for (const auto& [token, e] : rooms_) {
            if (auto r = e.room.lock()) {
                rooms.push_back(std::move(r));
            }
        }
        for (const auto& room : rooms) {
            room->connection_ended(peer, reason);
        }
    }

    void set_aside(std::string_view what) {
        log_("app", what);
        setup_reports_.emplace_back(what);
    }

    std::pmr::string random_id(std::size_t n) {
        std::array<std::uint8_t, 64> random{};
        factory_.random(std::span(random).first(n));
        std::pmr::string id(n, '0', memory_);
        for (std::size_t i = 0; i < n; ++i) {
            id[i] = protocol::id_alphabet[random[i] % protocol::id_alphabet.size()];
        }
        return id;
    }

    [[nodiscard]] std::pmr::polymorphic_allocator<> allocator() const { return {memory_}; }

    template <class Signature, class F>
    handler<Signature> bind(F&& f) const {
        return handler<Signature>(std::allocator_arg, allocator(), std::forward<F>(f));
    }

    std::pmr::memory_resource* memory_;
    executor_type ex_;
    backend::factory& factory_;
    logger log_;
    std::pmr::string app_id_;
    std::pmr::string self_id_;
    std::pmr::vector<std::pmr::string> relay_urls_;
    std::pmr::vector<backend::ice_server> ice_servers_;
    std::chrono::milliseconds idle_timeout_;
    std::pmr::vector<std::pmr::string> setup_reports_;
    std::pmr::map<std::pmr::string, entry, std::less<>> rooms_;
    std::pmr::map<std::pmr::string, std::shared_ptr<connection>, std::less<>> connections_;
    std::size_t users_ = 0;
};
```

- [ ] **Step 3: Compile both headers alone.**
  Run: `$B2 libs/trystero/test` (headers-alone) natively, with `toolset=emscripten` and with
  `toolset=gcc-15 --build-dir=bin/gcc-15`.
  Expected: they compile; no test changes yet.

- [ ] **Step 4: Commit.**

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: the app's state, its configuration and the rooms' options" \
  -- include/webcpp/trystero/options.hpp include/webcpp/trystero/detail/app_impl.hpp
```

### Task 4: The room on the connection layer; `peer_link` removed

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`room_impl`, the standalone `room`
  constructor)
- Modify: `libs/trystero/include/webcpp/trystero/detail/signaling.hpp` (the connection resource)
- Delete: `libs/trystero/include/webcpp/trystero/detail/peer_link.hpp`
- Modify: `libs/trystero/include/webcpp/trystero/detail/log.hpp:29-38` (the comment names the app's
  log), `libs/trystero/include/webcpp/trystero/detail/action_wire.hpp:104` (the `channel_link`
  comment names `detail::peer_link`, review M12: it names `detail::binding`),
  `libs/trystero/meta/include-boundaries.json` only if it names `peer_link` (it does not today)
- Test: `libs/trystero/test/room_test.cpp` (cases 12 and 13 of S:6.2 for one room; Review Focus 3;
  the virtual-clock helpers)

**Interfaces:**
- Consumes: Tasks 2 and 3.
- Produces: `detail::room_impl` implements `detail::room_endpoint`;
  `room_impl(std::shared_ptr<app_impl> app, room_options options, std::string_view room_id,
  std::pmr::memory_resource* memory)`; `room_impl::peer_state::link` is a
  `std::shared_ptr<detail::binding>` (a member named `binding` would change the meaning of the
  type's name inside `room_impl`, which GCC rejects, review I2); `room_impl::bind_peer(peer, c)`
  binds a peer (the `bind<Sig>` helper of `room.hpp:880-884` keeps its name, since `room` and
  `action` call it, review M11). `signaling`'s constructor gains, after `memory`,
  `std::pmr::memory_resource* connection_memory`, the resource `make_peer_connection` is given
  (`signaling.hpp:386`).

- [ ] **Step 1: Write the failing tests** in `room_test.cpp` (one room per app, the standalone
  constructor; the injection hook of Task 1), before `main` and called from it. They are S:6.2
  cases, so they run on the virtual clock (review I13): `room_test.cpp`'s `fixture` gains a
  constructor that sets it, and two helpers beside today's `run_until(io, ...)`, which Task 5a
  moves to `test/support/rooms.hpp` with the rest; the existing cases keep the wall clock:

```cpp
struct fixture {
    boost::asio::io_context io;
    memory_network net{io};

    /** A memory network with two relays; with `virtual_time`, on the virtual clock (Task 1). */
    explicit fixture(bool virtual_time = false) {
        net.virtual_clock = virtual_time;
        net.relay(relay_a);
        net.relay(relay_b);
    }
};

/** Runs until `done` holds, for at most `limit`: of the virtual clock when the fixture has one,
 * moving it from timer to timer, else of the wall clock. On the virtual clock it fires at most
 * `max_steps` timers, and fails the case by name past them (review N4): a timer re-armed without
 * delay would otherwise hold the clock still forever. */
inline bool run_until(fixture& f, const std::function<bool()>& done,
                      std::chrono::milliseconds limit = std::chrono::seconds(10)) {
    if (!f.net.virtual_clock) {
        return run_until(f.io, done, limit);
    }
    const double end = f.net.now_ms() + static_cast<double>(limit.count());
    for (std::size_t steps = 0; !done(); ++steps) {
        if (steps == f.net.max_steps) {
            BOOST_ERROR("the virtual clock fired max_steps timers without moving past them");
            return false;
        }
        f.io.restart();
        f.io.poll();
        if (done() || !f.net.step(end)) {
            break;
        }
    }
    return done();
}

/** Moves the virtual clock by `by`, firing every timer due meanwhile; fails the case by name when
 * the clock stopped at its step bound (review N4). */
inline void advance(fixture& f, std::chrono::milliseconds by) {
    if (!f.net.advance(by)) {
        BOOST_ERROR("the virtual clock fired max_steps timers without moving past them");
    }
}
```

```cpp
// A connection's end is every room's (one room here): a forced failure, a frame that does not
// decode, and a 65th advertised token each end it; the peers then meet again over a new one.
void a_connections_end_is_seen_and_the_peers_meet_again() {
    for (int way = 0; way < 3; ++way) {
        fixture f{true};
        trystero::room a(f.io.get_executor(), f.net, config("NNNNNNNNNNNNNNNNNNNN"), "ends");
        std::vector<std::size_t> a_pcs;
        f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server>) {
            a_pcs.push_back(pc);
        };
        trystero::room b(f.io.get_executor(), f.net, config("OOOOOOOOOOOOOOOOOOOO"), "ends");
        int joins = 0;
        int leaves = 0;
        a.on_peer_join([&](std::string_view) { ++joins; });
        a.on_peer_leave([&](std::string_view) { ++leaves; });
        webcpp::test::require(BOOST_TEST(run_until(f, [&] { return joins == 1; })));
        const auto open_pc = f.net.live_connections();  // the pair, both ends
        BOOST_TEST_EQ(open_pc, 2U);
        // A's end of the pair: the first connection made after A's room existed is A's offer, or
        // the peer of B's; peer_of finds the other.
        const std::size_t a_end = a_pcs.front();
        if (way == 0) {
            f.net.fail(a_end);
        } else if (way == 1) {
            const std::uint8_t bad[] = {3, 0, 1, 'x'};
            f.net.inject(a_end, bad);
        } else {
            for (int i = 0; i < 65; ++i) {
                f.net.inject(a_end, trystero::protocol::presence_frame("X" + std::to_string(i), true));
            }
        }
        BOOST_TEST(run_until(f, [&] { return leaves == 1; }));
        BOOST_TEST(run_until(f, [&] { return joins == 2; }, 20s));
    }
}

// Case 13: a frame naming a token no room of the app has is dropped, the connection stays, and the
// app's resource keeps none of them (64 kept frames of 4,000 bytes would hold over 256 KB).
void a_frame_of_an_unknown_token_is_dropped() {
    counting_resource memory;
    fixture f{true};
    auto a_config = config("PPPPPPPPPPPPPPPPPPPP");
    a_config.memory = &memory;
    trystero::room a(f.io.get_executor(), f.net, std::move(a_config), "unknown");
    std::vector<std::size_t> pcs;
    f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server>) {
        pcs.push_back(pc);
    };
    trystero::room b(f.io.get_executor(), f.net, config("QQQQQQQQQQQQQQQQQQQQ"), "unknown");
    int leaves = 0;
    a.on_peer_leave([&](std::string_view) { ++leaves; });
    webcpp::test::require(BOOST_TEST(run_until(f, [&] { return a.peers().size() == 1; })));
    const std::vector<std::uint8_t> action(4'000, 2);
    advance(f, 300ms);
    const auto before = memory.in_use;
    for (int i = 0; i < 1000; ++i) {
        f.net.inject(pcs.front(), trystero::protocol::room_frame("unknown-token", action));
    }
    advance(f, 300ms);
    BOOST_TEST_EQ(leaves, 0);
    BOOST_TEST_EQ(a.peers().size(), 1U);
    BOOST_TEST_LT(memory.in_use, before + std::size_t{64} * 1024);
}

// Review Focus 3: malformed envelopes end the connection; a frame of exactly 16,515 bytes does not.
void malformed_envelopes_end_the_connection() {
    const auto zero_token = std::vector<std::uint8_t>{2, 1, 0, 0};
    std::vector<std::uint8_t> long_token{2, 1, 0, 129};
    long_token.resize(4 + 129, 'x');
    const auto kind_3 = std::vector<std::uint8_t>{3, 1, 0, 1, 'x'};
    std::vector<std::uint8_t> too_big{1, 0, 1, 'x'};
    too_big.resize(16'516, 0);
    for (const auto& bad : {zero_token, long_token, kind_3, too_big}) {
        fixture f{true};
        trystero::room a(f.io.get_executor(), f.net, config("RRRRRRRRRRRRRRRRRRRR"), "bad");
        std::vector<std::size_t> pcs;
        f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server>) {
            pcs.push_back(pc);
        };
        trystero::room b(f.io.get_executor(), f.net, config("SSSSSSSSSSSSSSSSSSSS"), "bad");
        int leaves = 0;
        a.on_peer_leave([&](std::string_view) { ++leaves; });
        webcpp::test::require(BOOST_TEST(run_until(f, [&] { return a.peers().size() == 1; })));
        f.net.inject(pcs.front(), bad);
        BOOST_TEST(run_until(f, [&] { return leaves == 1; }));
    }
    fixture f{true};
    trystero::room a(f.io.get_executor(), f.net, config("RRRRRRRRRRRRRRRRRRRR"), "bad");
    std::vector<std::size_t> pcs;
    f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server>) {
        pcs.push_back(pc);
    };
    trystero::room b(f.io.get_executor(), f.net, config("SSSSSSSSSSSSSSSSSSSS"), "bad");
    int leaves = 0;
    a.on_peer_leave([&](std::string_view) { ++leaves; });
    webcpp::test::require(BOOST_TEST(run_until(f, [&] { return a.peers().size() == 1; })));
    std::vector<std::uint8_t> largest{1, 0, 1, 'x'};
    largest.resize(16'515, 0);
    f.net.inject(pcs.front(), largest);
    advance(f, 300ms);
    BOOST_TEST_EQ(leaves, 0);
}
```

  The connection a room's `on_made` sees first is the one its own negotiation makes when its id is
  the lower (`N` < `O`, `P` < `Q`, `R` < `S`), which is A's end; each case names A's ids lower for
  that reason.

  `counting_resource` is `room_test.cpp`'s own (`room_test.cpp:368-388`), with `in_use`.

- [ ] **Step 2: Run them to see the one red.**
  Run: `$B2 libs/trystero/test//room`
  Expected (review I15): `a_connections_end_is_seen_and_the_peers_meet_again` fails in its third
  way only, the 65th advertised token: today's link ignores a presence frame of another token
  (`peer_link.hpp:175-177`), so `leaves` stays 0. Its first two ways, the unknown-token case
  (today's link ignores another token) and the malformed-envelope cases (`decode_envelope` refuses
  each, and the link ends) pass already: they are guards that pin behaviour this task must keep,
  not red steps.

- [ ] **Step 3: Move the room onto the layer.** In `room.hpp`:
  - `room_impl` derives from `detail::room_endpoint` too. Its members gain
    `std::shared_ptr<app_impl> app_` and `room_options options_`; `config_`'s app part moves to the
    app: `self_id()` returns `app_->self_id()`, `relay_urls()` `app_->relay_urls()`; the signaling is
    given `app_->ice_servers()` and `app_->memory()` as its connection resource; the strategy is given
    `app_->relay_urls()`; `log_` is `app_->log()`.
  - `peer_state::link` becomes `std::shared_ptr<detail::binding> link` (the name stays `link`:
    GCC rejects a member named `binding` of type `std::shared_ptr<binding>`, review I2);
    `link_to` returns `action_wire::link{std::weak_ptr<channel_link>(it->second.link)}`.
  - `on_connected` (`room.hpp:1084-1128`) becomes:

```cpp
    void on_connected(std::string_view peer_view, std::shared_ptr<backend::peer_connection> pc,
                      std::shared_ptr<backend::data_channel> channel) {
        if (left_) {
            channel->set_handlers({});
            pc->set_handlers({});
            channel->close();
            pc->close();
            return;
        }
        const std::pmr::string peer(peer_view, memory_);
        const auto c = app_->adopt(peer, std::move(pc), std::move(channel));
        bind_peer(peer, c);
    }

    /** Binds this room to `c` for `peer`, as the spec's section 4.2 orders it: the negotiation
     * marked connected, the peer's state and handshake timer, then the handshake's first message.
     * Task 6 delivers the frames the connection kept before that message.
     *
     * Tip: the peer's id is copied first, since every step may run code that removes the peer's
     * entry (a kick); the entry is never held by reference across such a step (review C1). Each
     * lambda keeps its own copy in the room's resource, named: a plain copy of a std::pmr::string
     * takes the default resource (review N2). */
    void bind_peer(std::string_view peer_view, const std::shared_ptr<connection>& c) {
        const std::pmr::string peer(peer_view, memory_);
        const auto weak = weak_from_this();
        signaling_->bound(peer);
        {
            auto& state = peers_.try_emplace(peer, allocator()).first->second;
            state.link = std::allocate_shared<detail::binding>(
                allocator(), c, token_,
                bind<void(backend::bytes&&, std::span<const std::uint8_t>)>(
                    [weak, peer = std::pmr::string(peer, memory_)](
                        backend::bytes&& frame, std::span<const std::uint8_t> action) {
                        if (const auto self = weak.lock()) {
                            self->wire_->handle(peer, std::move(frame), action);
                        }
                    }),
                memory_);
            state.handshake_timer = factory_.make_timer(memory_);
            state.handshake_timer->start(options_.handshake_timeout,
                                         bind<void()>([weak, peer = std::pmr::string(peer, memory_)] {
                                             const auto self = weak.lock();
                                             if (!self) {
                                                 return;
                                             }
                                             self->report(peer,
                                                          make_error_code(error::handshake_timed_out),
                                                          "handshake timed out");
                                             self->kick(peer, "handshake timed out");
                                         }));
        }
        if (const auto it = peers_.find(peer); it != peers_.end()) {
            start_handshake(it->first, it->second);
        }
    }
```
  - `remove_peer` (`room.hpp:1264-1290`) takes, in place of `close_link`, how the binding ended:

```cpp
    /** How a binding ended: the peer's @_leave, its presence(0), a kick of this room, the
     * connection's end, or this room's own leave. */
    enum class end_kind { peer_left, withdrawn, kick, connection_end, room_end };

    void remove_peer(std::string_view peer_view, std::string_view reason, end_kind kind) {
        const auto it = peers_.find(peer_view);
        if (it == peers_.end()) {
            return;
        }
        // The id is moved out of the map before the entry goes: `peer_view` may refer to it.
        const auto node = peers_.extract(it);
        std::pmr::string peer = std::move(node.key());
        auto& state = node.mapped();
        log_("room", "peer ", peer, " gone: ", reason);
        if (kind != end_kind::connection_end) {
            // The connection idles after a departure and ends after a kick of its last room.
            state.link->detach(kind == end_kind::kick, app_->idle_timeout());
        }
        for (auto& ping : state.pings) {
            complete(std::move(ping.handler), make_error_code(error::peer_disconnected),
                     std::chrono::milliseconds{});
        }
        wire_->clear_peer(peer, make_error_code(error::peer_disconnected));
        if (left_) {
            return;
        }
        signaling_->forget(peer);
        // Announcements find peers without a connection; over a live one every rebinding goes
        // through it (the spec's ruling 9).
        if (!app_->find(peer)) {
            strategy_->restart_announcements();
        }
        if (state.active && on_leave_) {
            post([f = on_leave_, peer = std::move(peer)] { (*f)(peer); });
        }
    }
```

    With one room per app the connection is gone after a kick, so today's tests see the restart
    they expect.
  - `kick` sends `@_leave` then `remove_peer(peer, reason, end_kind::kick)`.
  - `on_incoming`'s `@_leave` branch calls `remove_peer(..., end_kind::peer_left)`.
  - `close_everything` (`room.hpp:1292-1318`) detaches every binding as a departure
    (`state.link->detach(false, app_->idle_timeout())`, the `room_end` of `remove_peer`), sends
    `presence(0, token_)` on every live connection of the app (`app_->live_connections()`), then
    `app_->unregister_room(token_, this)` and, once (a flag `released_`), `app_->release_user()`.
  - `room_impl`'s destructor (review I10) only unregisters and lets go quietly, since it may run
    while the `io_context` is destroyed: it calls `app_->unregister_room(token_, this)` (a no-op
    when `close_everything` did it), and destroys its peers' bindings, whose destructors leave
    their connection without ending, idling or calling anything (Task 2); it never ends a
    connection and never releases the app's user count (the spec's section 4.2, M10).
  - The `room_endpoint` overrides, for one room per app in this increment:

```cpp
    void presence_changed(std::string_view peer, bool present) override {
        if (!present && peers_.contains(peer)) {
            remove_peer(peer, "peer left the room", end_kind::withdrawn);
        }
        // Binding on presence is increment 2's (Task 6).
    }

    [[nodiscard]] bool keeps_frames_from(std::string_view /*peer*/) const override { return true; }

    void frame_kept(std::string_view /*peer*/) override {}

    void connection_ended(std::string_view peer, std::string_view reason) override {
        if (peers_.contains(peer)) {
            remove_peer(peer, reason, end_kind::connection_end);
        }
    }
```

  - `start()` registers the room, by a weak reference (the spec's section 4.1):
    `app_->register_room(room_id_, token_, std::static_pointer_cast<room_endpoint>(shared_from_this()))`
    and `app_->add_user()`, before the strategy starts.
  - The standalone `room(ex, factory, room_config, room_id)` splits its configuration: an
    `app_config{.app_id, .self_id, .relay_urls, .relay_redundancy, .ice_servers, .log, .memory}`
    for `std::allocate_shared<app_impl>(...)`, and a `room_options{.password,
    .handshake_timeout, .max_receive_bytes}`; the room posts each of
    `app_->setup_reports()` to its `on_error` with `error::invalid_configuration` and an empty peer,
    as `report_configuration` does today (`room.hpp:1066-1073`), so that
    `unusable_ice_servers_are_dropped_and_reported` and `long_peer_ids_are_bounded` pass unchanged.
  - Delete `detail/peer_link.hpp` and its include.
  In `signaling.hpp`: the constructor takes `std::pmr::memory_resource* connection_memory` after
  `memory` and stores it; `make_connection` passes it to `factory_.make_peer_connection`; a new
  member keeps an entry connected while a room is bound to the peer:

```cpp
    /** A room is bound to `peer`: its negotiation ends, its entry stays marked connected, and every
     * later signal of that peer is ignored until `forget` (the spec's ruling 9, I1). */
    void bound(std::string_view peer) {
        auto it = peers_.find(peer);
        if (it == peers_.end()) {
            it = peers_.try_emplace(std::pmr::string(peer, memory_), allocator()).first;
        }
        reset_offer(it->second);
        reset_answer(it->second);
        it->second.connected = true;
    }
```

- [ ] **Step 4: Run every suite.**
  Run: the native, gcc-15 and emscripten lanes, and both interop lanes, each with its own
  `--build-dir` (Global Constraints; review M18).
  Expected: every existing case passes unchanged, the new ones pass; the native interop lane's 19
  scenarios and the browser lane's 16 pass (review M2).

- [ ] **Step 5: Lint.** Expected: `lint: clean`.

- [ ] **Step 6: Commit.**

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: rooms on a shared connection layer, one room per app" \
  -- include/webcpp/trystero/room.hpp include/webcpp/trystero/detail/signaling.hpp \
     include/webcpp/trystero/detail/peer_link.hpp include/webcpp/trystero/detail/log.hpp \
     include/webcpp/trystero/detail/action_wire.hpp test/room_test.cpp
```

---

## Increment 2: the app

### Task 5a: The tests' shared helpers, and the virtual clock in them

**Files:**
- Create: `libs/trystero/test/support/rooms.hpp`
- Modify: `libs/trystero/test/room_test.cpp` (its helpers move out; `config()` stays)

**Interfaces:**
- Consumes: Task 1's virtual clock; Task 4's virtual-clock helpers.
- Produces, in `webcpp::trystero::test`: `relay_a`, `relay_b`, `app_id`; `struct fixture` with
  `explicit fixture(bool virtual_time = false)`; `bool run_until(fixture&, const
  std::function<bool()>&, std::chrono::milliseconds limit = 10s)` (virtual time when the fixture
  has a virtual clock, else wall time, as today); `bool run_until(boost::asio::io_context&, ...)`
  (today's, for the wall clock); `void advance(fixture&, std::chrono::milliseconds)`;
  `counting_resource`; `flooder`; `relayed`; `relayed_signal`.

- [ ] **Step 1: Move the helpers.** `test/support/rooms.hpp` takes `relay_a`, `relay_b`, `app_id`,
  `fixture`, both `run_until`s, `advance`, `counting_resource`, `flooder`, `relayed`,
  `relayed_signal` from `room_test.cpp` (`room_test.cpp:46-89,368-484,768-803` at `8e8911f`, and
  the `fixture` constructor, `run_until(fixture&, ...)` and `advance` that Task 4 added),
  unchanged but for `flooder`'s constructor, which waits with `run_until(f, ...)` so that it works
  on either clock.

- [ ] **Step 2: Run** `room` natively, with `toolset=emscripten` and with gcc-15. Expected: every
  case passes as before (a refactor: no new behaviour, no red step).
- [ ] **Step 3: Commit.**

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: the room tests' helpers shared, with a virtual clock" \
  -- test/support/rooms.hpp test/room_test.cpp
```

### Task 5b: The two new errors

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/error.hpp`
- Test: `libs/trystero/test/room_test.cpp`

**Interfaces:**
- Produces: `error::already_joined = 14` ("the app has already joined a room of that id"),
  `error::too_many_rooms = 15` ("the app holds as many rooms as a peer accepts, 64"); the message of
  `invalid_configuration` becomes "the configuration is unusable".

- [ ] **Step 1: Write the failing test** in `room_test.cpp`:

```cpp
// The errors' values are fixed and never reused; their messages name what failed.
void the_errors_have_their_values() {
    BOOST_TEST_EQ(static_cast<int>(trystero::error::already_joined), 14);
    BOOST_TEST_EQ(static_cast<int>(trystero::error::too_many_rooms), 15);
    BOOST_TEST_EQ(make_error_code(trystero::error::already_joined).message(),
                  "the app has already joined a room of that id");
    BOOST_TEST_EQ(make_error_code(trystero::error::too_many_rooms).message(),
                  "the app holds as many rooms as a peer accepts, 64");
    BOOST_TEST_EQ(make_error_code(trystero::error::invalid_configuration).message(),
                  "the configuration is unusable");
}
```

- [ ] **Step 2: Run it to see it fail.** `$B2 libs/trystero/test//room`: Expected: no member
  `already_joined`.
- [ ] **Step 3: Implement** the two enumerators after `invalid_configuration` (13) and their
  messages in `error_category_impl::message`, each with its Doc Comment; `invalid_configuration`'s
  comment names the app's configuration.
- [ ] **Step 4: Run** `room` natively, on emscripten and with gcc-15. Expected: pass.
- [ ] **Step 5: Commit** `"trystero: the errors already_joined and too_many_rooms"`
  (`include/webcpp/trystero/error.hpp test/room_test.cpp`).

### Task 5c: `app`, `app::join`, and the standalone room removed, every user moved

Ruling 12: the removal and the migration go in one commit.

**Files:**
- Create: `libs/trystero/include/webcpp/trystero/app.hpp`
- Modify: `libs/trystero/include/webcpp/trystero.hpp` (include `app.hpp` and `options.hpp`)
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (remove `room_config` and the standalone
  constructor; `room` gets a private constructor from `std::shared_ptr<detail::room_impl>`,
  `friend class app`)
- Modify: `libs/trystero/test/support/rooms.hpp` (`app_config`, `join`)
- Create: `libs/trystero/test/shared_test.cpp`; Modify: `libs/trystero/test/Jamfile`
- Modify, to `app`: `test/room_test.cpp`, `test/native_room_test.cpp`, `test/allocation_test.cpp`,
  `example/quick_start.cpp`, `example/native_quick_start.cpp`, `example/chat/chat.cpp`,
  `test/oracle/interop/peer.cpp`, `test/oracle/interop/native_peer.cpp`

**Interfaces:**
- Consumes: Tasks 3, 4, 5a and 5b.
- Produces:

```cpp
namespace webcpp::trystero {
class app {
public:
    using executor_type = boost::asio::any_io_executor;
    static constexpr std::size_t max_rooms = 64;
    app(executor_type ex, backend::factory& factory, app_config config);
    app(app&& other) noexcept;
    app& operator=(app&& other) noexcept;
    app(const app&) = delete;
    app& operator=(const app&) = delete;
    ~app();
    [[nodiscard]] const executor_type& get_executor() const;
    [[nodiscard]] std::string_view app_id() const;
    [[nodiscard]] std::string_view self_id() const;
    [[nodiscard]] std::span<const std::pmr::string> relay_urls() const;
    template <class F>
        requires std::is_invocable_v<F&, std::string_view, boost::system::error_code,
                                     std::string_view>
    void on_error(F&& f);
    boost::system::result<room> join(std::string_view room_id, room_options options = {});
};
}  // namespace webcpp::trystero
```

  and, in `shared_test.cpp`, `struct two_apps` (a virtual-clock `fixture f`, `app a`, `app b`, their
  rooms, `all_joined()`) and `void joined_in(two_apps&, std::initializer_list<const char*>)`, which
  later tasks use.

- [ ] **Step 1: Write the failing tests** `test/shared_test.cpp`: S:6.2 cases 15 (its first half),
  18 and Review Focus 5, and the sharing cases that hold as soon as `app` exists (1, 2 and 6, which
  the plan's first draft put in Task 6, where they were not red; review I15), each on the virtual
  clock:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Rooms of one app sharing one connection per remote peer, over the memory backend: the cases of
// the specification's section 6.2 (doc: #trystero-design-sharing).

#include <webcpp/trystero.hpp>

#include "support/memory_backend.hpp"
#include "support/rooms.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "require.hpp"

namespace trystero = webcpp::trystero;
using namespace webcpp::trystero::test;
using namespace std::chrono_literals;

namespace {

// Case 18: every room reports the app's id; an over-long one is replaced and reported.
void every_room_has_the_apps_id() {
    fixture f{true};
    trystero::app a(f.io.get_executor(), f.net, app_config("AAAAAAAAAAAAAAAAAAAA"));
    auto r1 = join(a, "r1");
    auto r2 = join(a, "r2");
    BOOST_TEST_EQ(r1.self_id(), "AAAAAAAAAAAAAAAAAAAA");
    BOOST_TEST_EQ(r2.self_id(), a.self_id());
    auto long_id = app_config(std::string(65, 'x'));
    long_id.idle_timeout = -1ms;
    std::vector<boost::system::error_code> reports;
    trystero::app b(f.io.get_executor(), f.net, std::move(long_id));
    b.on_error([&](std::string_view peer, boost::system::error_code ec, std::string_view) {
        BOOST_TEST(peer.empty());
        reports.push_back(ec);
    });
    BOOST_TEST_EQ(b.self_id().size(), trystero::protocol::peer_id_size);
    BOOST_TEST(run_until(f, [&] { return reports.size() == 2; }));
    for (const auto ec : reports) {
        BOOST_TEST(ec == trystero::error::invalid_configuration);
    }
}

// Case 15: a room joined twice, the 65th room.
void joining_is_refused_twice_and_beyond_64() {
    fixture f{true};
    trystero::app a(f.io.get_executor(), f.net, app_config("BBBBBBBBBBBBBBBBBBBB"));
    auto r = join(a, "r");
    const auto again = a.join("r");
    BOOST_TEST(!again && again.error() == trystero::error::already_joined);
    std::vector<trystero::room> rooms;
    for (int i = 1; i < 64; ++i) {
        rooms.push_back(join(a, "x" + std::to_string(i)));
    }
    const auto too_many = a.join("x64");
    BOOST_TEST(!too_many && too_many.error() == trystero::error::too_many_rooms);
}

// Review Focus 5: two apps of one app id with the same self id never meet, and nothing loops.
void two_apps_with_one_id_never_meet() {
    fixture f{true};
    trystero::app a(f.io.get_executor(), f.net, app_config("CCCCCCCCCCCCCCCCCCCC"));
    trystero::app b(f.io.get_executor(), f.net, app_config("CCCCCCCCCCCCCCCCCCCC"));
    auto ra = join(a, "same");
    auto rb = join(b, "same");
    advance(f, 2s);
    BOOST_TEST(ra.peers().empty() && rb.peers().empty());
    BOOST_TEST_EQ(f.net.made_connections, 0U);
}

```

  The sharing cases, with the helpers later tasks use:

```cpp
/** Two apps, A and B, on the virtual clock, and their rooms. */
struct two_apps {
    fixture f{true};
    trystero::app a;
    trystero::app b;
    std::vector<trystero::room> a_rooms;
    std::vector<trystero::room> b_rooms;

    two_apps(std::string_view a_id, std::string_view b_id)
        : a(f.io.get_executor(), f.net, app_config(a_id)),
          b(f.io.get_executor(), f.net, app_config(b_id)) {}

    bool all_joined() const {
        const auto one = [](const trystero::room& r) { return r.peers().size() == 1; };
        return std::ranges::all_of(a_rooms, one) && std::ranges::all_of(b_rooms, one);
    }
};

/** Joins A and B to each room given, in that order, and waits until every room of both has the
 * other. */
void joined_in(two_apps& t, std::initializer_list<const char*> rooms) {
    for (const auto* id : rooms) {
        t.a_rooms.push_back(join(t.a, id));
        t.b_rooms.push_back(join(t.b, id));
    }
    webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); })));
}

// Case 1: two rooms, one connection; each room's action of one name reaches only its room, both
// ways.
void two_rooms_share_one_connection() {
    two_apps t("DDDDDDDDDDDDDDDDDDDD", "EEEEEEEEEEEEEEEEEEEE");
    joined_in(t, {"r1", "r2"});
    BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 2U; }));  // one pair
    std::vector<std::string> at_b[2];
    std::vector<std::string> at_a[2];
    std::vector<trystero::action<std::string>> actions;
    for (int i = 0; i < 2; ++i) {
        actions.push_back(t.b_rooms[i].make_action<std::string>("chat"));
        actions.back().on_message([&at_b, i](std::string s, const trystero::message_info&) {
            at_b[i].push_back(std::move(s));
        });
        actions.push_back(t.a_rooms[i].make_action<std::string>("chat"));
        actions.back().on_message([&at_a, i](std::string s, const trystero::message_info&) {
            at_a[i].push_back(std::move(s));
        });
    }
    t.a_rooms[0].make_action<std::string>("chat").async_send("a1", [](boost::system::error_code) {});
    t.a_rooms[1].make_action<std::string>("chat").async_send("a2", [](boost::system::error_code) {});
    t.b_rooms[0].make_action<std::string>("chat").async_send("b1", [](boost::system::error_code) {});
    t.b_rooms[1].make_action<std::string>("chat").async_send("b2", [](boost::system::error_code) {});
    BOOST_TEST(run_until(t.f, [&] {
        return at_b[0].size() == 1 && at_b[1].size() == 1 && at_a[0].size() == 1 &&
               at_a[1].size() == 1;
    }));
    BOOST_TEST(at_b[0].at(0) == "a1" && at_b[1].at(0) == "a2");
    BOOST_TEST(at_a[0].at(0) == "b1" && at_a[1].at(0) == "b2");
}

// Case 2: three rooms, joined in another order on each side, one connection.
void three_rooms_share_one_connection() {
    two_apps t("FFFFFFFFFFFFFFFFFFFF", "GGGGGGGGGGGGGGGGGGGG");
    for (const auto* id : {"r1", "r2", "r3"}) {
        t.a_rooms.push_back(join(t.a, id));
    }
    for (const auto* id : {"r3", "r1", "r2"}) {
        t.b_rooms.push_back(join(t.b, id));
    }
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }));
    BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 2U; }));
}

// Case 6: both rooms negotiate at once; one connection stays, both rooms join over it.
void a_second_connection_is_dropped() {
    two_apps t("JJJJJJJJJJJJJJJJJJJJ", "KKKKKKKKKKKKKKKKKKKK");
    joined_in(t, {"r1", "r2"});
    BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 2U; }));
}

}  // namespace

int main() {
    every_room_has_the_apps_id();
    joining_is_refused_twice_and_beyond_64();
    two_apps_with_one_id_never_meet();
    two_rooms_share_one_connection();
    three_rooms_share_one_connection();
    a_second_connection_is_dropped();
    return boost::report_errors();
}
```

  Each later task adds its cases before the closing `}  // namespace` and calls them from `main`.

  Add to `test/Jamfile`,
  after the `room` test:

```jam
# Rooms of one app that share one connection per remote peer, the specification's cases; once as
# the build asks, once without exceptions, on both targets; and, natively, once with the standard
# libraries' checks, so that a kept frame that kicks during a binding, or a waiter failed during a
# connection's end, fails at once when it touches what it removed (review C1, I7).
webcpp.run shared : shared_test.cpp ;
webcpp.run shared_no_exceptions : shared_test.cpp : <exception-handling>off ;
webcpp.run shared_hardened : shared_test.cpp
  : <define>_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE <define>_GLIBCXX_ASSERTIONS
  : native ;
```

- [ ] **Step 2: Run it to see it fail.** `$B2 libs/trystero/test//shared`: Expected: no
  `webcpp::trystero::app` (every case is red for want of the API).

- [ ] **Step 3: Write `app.hpp`.**

```cpp
/**
 * This program in one app: one peer id, one connection per remote peer, shared by the rooms it
 * joins, as a Trystero JS page shares them.
 *
 * Tip: the `app` and every room of it share one state; destroying the `app` while rooms remain
 * changes nothing for them. Every allocation of the app goes through `app_config::memory`, which
 * must outlive the app, its rooms and every operation they leave pending.
 *
 * @see "Rooms and actions", in the guide.
 */
class app {
public:
    /** The executor every handler and completion of the app and its rooms runs on. */
    using executor_type = boost::asio::any_io_executor;

    /** The most rooms an app holds at once: a peer drops a connection that advertises more
     * (Trystero's literal 64, C/shared-peer.mjs:457). */
    static constexpr std::size_t max_rooms = 64;

    /**
     * Makes an app: draws its id unless the configuration names one, and keeps the configuration.
     *
     * @param ex The executor of the app and of its rooms.
     * @param factory The backend, which must outlive the app and its rooms.
     * @param config The configuration, copied into `config.memory`.
     * @pre `config.memory` is not null.
     */
    app(executor_type ex, backend::factory& factory, app_config config)
        : impl_(make(std::move(ex), factory, std::move(config))) {
        impl_->add_user();
    }

    /** Takes the state of `other`, which may then only be destroyed or assigned. */
    app(app&& other) noexcept = default;

    /** Releases the state it held, then takes `other`'s; its rooms go on. */
    app& operator=(app&& other) noexcept {
        if (this != &other) {
            if (impl_) {
                impl_->release_user();
            }
            impl_ = std::move(other.impl_);
        }
        return *this;
    }

    app(const app&) = delete;
    app& operator=(const app&) = delete;

    /** Releases the state; its rooms go on, and their connections with them. */
    ~app() {
        if (impl_) {
            impl_->release_user();
        }
    }

    // get_executor, app_id, self_id, relay_urls: impl_'s, each with BOOST_ASSERT(impl_).

    /**
     * Sets what is called with each part of the configuration the app set aside, with an empty peer
     * id and `error::invalid_configuration`; reports made before it is set are posted to it.
     */
    template <class F>
        requires std::is_invocable_v<F&, std::string_view, boost::system::error_code,
                                     std::string_view>
    void on_error(F&& f);

    /**
     * Joins a room of the app, Trystero's `joinRoom` for an app already configured.
     *
     * @param room_id The room's id, read during the call.
     * @param options The password, the handshake's timeout and the receive bound of the room.
     * @return The room, or `error::already_joined` while a room of that id is joined (Task 8b: one in
     * its farewell is finished at once first), or `error::too_many_rooms` when 64 rooms count.
     */
    boost::system::result<room> join(std::string_view room_id, room_options options = {}) {
        BOOST_ASSERT(impl_);
        if (impl_->find_room(room_id)) {
            return error::already_joined;  // Task 8b finishes a room in its farewell first
        }
        if (impl_->counted_rooms() >= max_rooms) {
            return error::too_many_rooms;
        }
        return room(detail::room_impl::make(impl_, std::move(options), room_id));
    }

private:
    static std::shared_ptr<detail::app_impl> make(executor_type ex, backend::factory& factory,
                                                  app_config config) {
        std::pmr::memory_resource* const memory = config.memory;
        BOOST_ASSERT_MSG(memory != nullptr, "app_config::memory names no resource");
        return std::allocate_shared<detail::app_impl>(std::pmr::polymorphic_allocator<>(memory),
                                                      std::move(ex), factory, std::move(config));
    }

    std::shared_ptr<detail::app_impl> impl_;
};
```

  `room_impl::make(app, options, room_id)` allocates the room in the app's resource
  (`room_options::memory` arrives in Task 13), and calls `start()`. `app::on_error` stores the
  handler in the app's state and posts it every sentence of `setup_reports()`, then every later
  one. The `room` class loses its public constructor and gets
  `explicit room(std::shared_ptr<detail::room_impl> impl)` private, `friend class app;`; its
  `self_id()`, `relay_urls()` and `get_executor()` return the app's; its destructor and move
  assignment are unchanged. `room_config` and `room_impl::adopt` go.

- [ ] **Step 4: Move every user to `app`.**
  - `test/support/rooms.hpp` (Task 5a) gains, in place of `room_test.cpp`'s `config(self_id,
    password)`:

```cpp
inline trystero::app_config app_config(std::string_view self_id) {
    trystero::app_config c;
    c.app_id = app_id;
    c.relay_urls = {relay_a, relay_b};
    c.self_id = self_id;
    if (std::getenv("WEBCPP_TEST_TRYSTERO_VERBOSE") != nullptr) {
        c.log = [id = std::pmr::string(self_id)](std::string_view category, std::string_view text) {
            std::cerr << "  [" << id << "] " << category << ": " << text << "\n";
        };
    }
    return c;
}

/** Joins `room_id`, which must succeed: a failure ends the test. */
inline trystero::room join(trystero::app& a, std::string_view room_id,
                           trystero::room_options options = {}) {
    auto r = a.join(room_id, std::move(options));
    webcpp::test::require(BOOST_TEST(r.has_value()));
    return std::move(*r);
}
```

  - `room_test.cpp`: each peer is an `app` and its room a `join`; `config(id, password)` becomes
    `app_config(id)` and `{.password = password}`. The cases in which a peer leaves and comes back
    with the same id expect a new negotiation, which an idle connection of the same `app` would
    replace (S:6.2, last paragraph): `leaving_is_seen_by_the_others_and_a_return_reconnects`
    (`room_test.cpp:231-248`) makes B's `app` inside the inner scope, so that it is destroyed with
    B's room; `join_and_leave` (`room_test.cpp:805-814`) makes B's `app` inside the function, for
    `a_replayed_answer_is_refused` and `a_replayed_offer_is_refused` (calls at `:863`, `:896`); the
    unusable-ICE and long-id cases read `app::on_error`.
  - `native_room_test.cpp`: the same; `leaving_is_seen_over_a_real_connection`
    (`native_room_test.cpp:158`) makes the leaver's `app` in the inner scope.
  - `allocation_test.cpp`: `config_in` (`allocation_test.cpp:511-528`, between `tag::config[]`
    and `end::config[]`, which `allocation.adoc` includes) becomes two functions there, every part
    in the counting resource as `config_in` built the room's, so that no temporary of the default
    resource enters the counted window (review I11):

```cpp
// tag::config[]
/** An app's configuration, every part of it in `memory`, made before the window. */
trystero::app_config app_config_in(counting_resource& memory, std::string_view self_id,
                                   std::string_view relay, std::size_t& lines) {
    return trystero::app_config{
        .app_id = std::pmr::string("allocation", &memory),
        .self_id = std::pmr::string(self_id, &memory),
        .relay_urls =
            std::pmr::vector<std::pmr::string>({std::pmr::string(relay, &memory)}, &memory),
        .ice_servers = std::pmr::vector<trystero::backend::ice_server>(&memory),
        .log =
            {
                std::allocator_arg,
                std::pmr::polymorphic_allocator<>(&memory),
                [&lines](std::string_view /*category*/, std::string_view /*text*/) { ++lines; },
            },
        .memory = &memory,
    };
}

/** A room's options, its password in `memory`. */
trystero::room_options options_in(counting_resource& memory) {
    return trystero::room_options{.password = std::pmr::string("a password", &memory)};
}

// end::config[]
```

    and each room of the existing case comes from an `app` made of `app_config_in`, joined with
    `options_in`.
  - `example/quick_start.cpp`, between `tag::application[]` and `end::application[]`; the
    example tests the result, as S:4.10 says (review M4):

```cpp
struct application {
    asio::io_context io;
    trystero::browser::event_loop loop{io};
    trystero::browser::backend backend{loop};
    trystero::app app{loop.get_executor(), backend, {.app_id = "webcpp-trystero-quick-start"}};
    std::optional<trystero::room> room;
    std::optional<trystero::action<std::string>> chat;
    std::optional<trystero::action<std::string>> echo;

    /** Joins the lobby and makes the actions; false, with the reason printed, when the app
     * refuses (a room of that id joined already, or 64 rooms). */
    bool join() {
        auto joined = app.join("lobby");
        if (!joined) {
            std::printf("not joined: %s\n", joined.error().message().c_str());
            return false;
        }
        room.emplace(std::move(*joined));
        chat.emplace(room->make_action<std::string>("chat"));
        echo.emplace(room->make_action<std::string>("echo"));
        return true;
    }
};
```

    and `main` (between `tag::main[]` and `end::main[]`) starts with
    `if (!app->join()) { return 1; }` and writes `app->room->`, `app->chat->` and `app->echo->`
    where it wrote `app->room.`, `app->chat.` and `app->echo.`; the coroutine takes
    `*app->echo`. `example/native_quick_start.cpp` the same way, its `main` returning 1 when
    `join` fails; `example/chat/chat.cpp`'s `join_room` makes the app
    once (`c.app`, a `std::unique_ptr<trystero::app>` with the log of today's `config.log`), then
    `auto joined = c.app->join(room_id);` and returns after `ui("log", "chat", "not joined: " +
    joined.error().message())` when it fails.
  - `test/oracle/interop/peer.cpp`: `page` gains `std::unique_ptr<trystero::app> app`; `start()`
    makes it from the URL (`config_of_page()` returns an `app_config`, its `password` moving to a
    `room_options`), then `app.room = std::make_unique<trystero::room>(std::move(*app.app->join(param("room"), options_of_page())))`;
    `join_second()` joins `room2` through the same app, so that the two rooms share the app's id.
  - `test/oracle/interop/native_peer.cpp`: `main` makes `trystero::app app(io.get_executor(),
    backend, app_config_of(args))` and `trystero::room room = std::move(*app.join(args["room"],
    options_of(args)))`; `second_room` joins `room2` through the same `app`; `replace` moves
    `std::move(*app.join(args["room"] + "-replaced", options_of(args)))` over the joined room.

- [ ] **Step 5: Run every suite.** The native, gcc-15 and emscripten lanes (`shared`,
  `shared_no_exceptions` and, natively, `shared_hardened` among them), the oracle, both interop
  lanes. Expected: everything passes. The interop's `twoRoomsScenario` (`run.mjs:584-601`) passes
  with one id for both C++ rooms, since they now share one connection, which its comment's last
  sentence denied; Task 14a rewrites it.
- [ ] **Step 6: Lint**, then **commit**:

```bash
git -C libs/trystero -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "trystero: an app joins rooms; room_config and the standalone room removed" \
  -- include/webcpp/trystero.hpp include/webcpp/trystero/app.hpp \
     include/webcpp/trystero/room.hpp include/webcpp/trystero/detail/app_impl.hpp \
     test/support/rooms.hpp test/shared_test.cpp test/Jamfile test/room_test.cpp \
     test/native_room_test.cpp test/allocation_test.cpp example/quick_start.cpp \
     example/native_quick_start.cpp example/chat/chat.cpp test/oracle/interop/peer.cpp \
     test/oracle/interop/native_peer.cpp
```

### Task 6: Binding by presence and by kept frames, safely

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`try_bind`, `deliver_kept`,
  `presence_changed`, `frame_kept`, the end of `start`)
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 5 and 12 with two rooms; review C1)

**Interfaces:**
- Consumes: Task 5c (`two_apps`, `joined_in`, the virtual clock).
- Produces: in `shared_test.cpp`, `ends`, `internal_name` and `internal_frame`; `room_impl::try_bind(std::string_view peer, bind_event event)`, `enum class bind_event
  { joined, presence, frame, reuse, rebinding }`, the conditions of S:4.2 (the peer advertised the
  token on that live connection; the room is not leaving; no backoff runs, always false until
  Task 9), and `room_impl::deliver_kept(const std::pmr::string& peer, connection& c, const
  detail::binding* mine)`.

- [ ] **Step 1: Write the failing tests:**

```cpp
/** A's end of the first pair made, recorded by on_made: A, the lower id, offers first. */
struct ends {
    std::vector<std::size_t> made;
    explicit ends(two_apps& t) {
        t.f.net.on_made = [this](std::size_t pc, std::span<const trystero::backend::ice_server>) {
            made.push_back(pc);
        };
    }
    [[nodiscard]] std::size_t a() const { return made.at(0); }
};

/** The action an inline room frame of `room` carries (`@_hsdata`, `@_leave`, `chat`, ...), or
 * empty. */
std::string internal_name(std::span<const std::uint8_t> frame, std::string_view room) {
    const auto env = trystero::protocol::decode_envelope(frame);
    if (!env || env->type != trystero::protocol::envelope::kind::room ||
        env->token != trystero::protocol::room_token(app_id, room)) {
        return {};
    }
    const auto d = trystero::protocol::decode_action(env->payload);
    const auto* p = d.frame ? std::get_if<trystero::protocol::proposal>(&*d.frame) : nullptr;
    return p != nullptr ? std::string(p->name) : std::string();
}

/** A room frame of `room`'s token holding the inline action `name`, with `json` as its payload
 * (none when empty), as a peer's room sends an internal action. */
trystero::protocol::bytes internal_frame(std::string_view room, std::string_view name,
                                         std::string_view json = {}) {
    const auto token = trystero::protocol::room_token(app_id, room);
    const std::span<const std::uint8_t> payload(reinterpret_cast<const std::uint8_t*>(json.data()),
                                                json.size());
    auto frame = trystero::protocol::begin_room_frame(
        token, trystero::protocol::inline_header_size + payload.size());
    trystero::protocol::append_inline(frame, name,
                                      json.empty() ? trystero::protocol::payload_format::text
                                                   : trystero::protocol::payload_format::json,
                                      {}, payload);
    return frame;
}

// Case 5: frames of r2 that reach A before B's presence, as a Trystero JS peer sends its handshake
// before its presence (C/shared-peer.mjs:90-98): A keeps them, and once B's presence binds r2 it
// delivers them after making B's state, so the handshake completes. (The 64-frame bound and the
// 10 s expiry are the connection's, tested in connection_test.cpp; conflict 13.)
void frames_before_presence_are_kept() {
    two_apps t("HHHHHHHHHHHHHHHHHHHH", "IIIIIIIIIIIIIIIIIIII");
    const ends e(t);
    joined_in(t, {"r1"});
    t.a_rooms.push_back(join(t.a, "r2"));
    advance(t.f, 200ms);
    t.f.net.inject(e.a(), internal_frame("r2", "@_hsready"));
    advance(t.f, 50ms);
    BOOST_TEST(t.a_rooms[1].peers().empty());  // kept, not bound: B never advertised r2
    t.b_rooms.push_back(join(t.b, "r2"));
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }));
}

// Review C1: a kept frame that makes A kick B while A binds (an undecodable action frame of r2,
// then B's presence for r2): A kicks B from r2 only, nothing else happens, and nothing touches
// the peer's removed state (shared_hardened and the sanitizers see it).
void a_kept_frame_that_kicks_during_a_binding() {
    two_apps t("HHHHHHHHHHHHHHHHHHH1", "IIIIIIIIIIIIIIIIIII2");
    const ends e(t);
    joined_in(t, {"r1"});
    t.a_rooms.push_back(join(t.a, "r2"));
    advance(t.f, 200ms);
    std::size_t leaves_sent = 0;
    t.f.net.filter = [&](std::size_t from, std::size_t, std::span<const std::uint8_t> frame) {
        leaves_sent += from == e.a() && internal_name(frame, "r2") == "@_leave" ? 1 : 0;
        return trystero::test::memory_network::verdict::deliver;
    };
    const auto token = trystero::protocol::room_token(app_id, "r2");
    const std::uint8_t garbage[] = {9, 9, 9};
    t.f.net.inject(e.a(), trystero::protocol::room_frame(token, garbage));
    t.f.net.inject(e.a(), trystero::protocol::presence_frame(token, true));
    BOOST_TEST(run_until(t.f, [&] { return leaves_sent == 1; }));
    BOOST_TEST(t.a_rooms[1].peers().empty());
    BOOST_TEST_EQ(t.a_rooms[0].peers().size(), 1U);
    BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
}

// Case 12 with two rooms: a forced failure, a frame that does not decode, and a 65th advertised
// token each end the connection for both rooms, which then meet again over one new connection.
void a_connections_end_is_every_rooms() {
    for (int way = 0; way < 3; ++way) {
        two_apps t("JJJJJJJJJJJJJJJJJJJ1", "KKKKKKKKKKKKKKKKKKK2");
        const ends e(t);
        joined_in(t, {"r1", "r2"});
        int leaves = 0;
        for (auto& r : t.a_rooms) {
            r.on_peer_leave([&leaves](std::string_view) { ++leaves; });
        }
        if (way == 0) {
            t.f.net.fail(e.a());
        } else if (way == 1) {
            const std::uint8_t bad[] = {3, 0, 1, 'x'};
            t.f.net.inject(e.a(), bad);
        } else {
            for (int i = 0; i < 65; ++i) {
                t.f.net.inject(e.a(), trystero::protocol::presence_frame("X" + std::to_string(i), true));
            }
        }
        BOOST_TEST(run_until(t.f, [&] { return leaves == 2; }));
        BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 30s));
        BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 2U; }));
    }
}
```

- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//shared`. Expected (review I15):
  case 5 and the C1 case fail because A never binds `r2` on B's presence (no presence path yet:
  `presence_changed` only unbinds, Task 4), so `all_joined()` and `leaves_sent == 1` time out;
  case 12's two-room form passes already (Task 4's connection end reaches every room): it is a
  guard.

- [ ] **Step 3: Implement.**

```cpp
    enum class bind_event { joined, presence, frame, reuse, rebinding };

    /** Binds `peer` if the spec's conditions hold (section 4.2): the app holds a live connection to
     * it, on which it advertised this room's token itself; this room is not leaving; no backoff of
     * this room runs for it; and it is not bound already. */
    bool try_bind(std::string_view peer, bind_event event) {
        if (left_ || peers_.contains(peer)) {
            return false;
        }
        const auto c = app_->find(peer);
        if (!c || !c->advertised(token_) || refusing(peer)) {
            return false;
        }
        if (event == bind_event::reuse || event == bind_event::rebinding) {
            c->send_presence(token_, true);
        }
        bind_peer(peer, c);
        return true;
    }

    void presence_changed(std::string_view peer, bool present) override {
        if (present) {
            try_bind(peer, bind_event::presence);
        } else if (peers_.contains(peer)) {
            remove_peer(peer, "peer left the room", end_kind::withdrawn);
        }
    }

    void frame_kept(std::string_view peer) override { try_bind(peer, bind_event::frame); }

    /** Delivers the frames `c` kept for this room, in order. A frame may make the room kick the
     * peer at once (an action-level violation, a malformed handshake message, a `@_leave`), which
     * removes the peer's entry: delivery stops as soon as the entry is gone or holds another
     * binding (review C1). Task 10 adds the rule for a kept `@_leave`. */
    void deliver_kept(const std::pmr::string& peer, connection& c, const detail::binding* mine) {
        auto frames = c.take_kept(token_);
        for (auto& frame : frames) {
            if (const auto it = peers_.find(peer); it == peers_.end() || it->second.link.get() != mine) {
                return;
            }
            const auto envelope = protocol::decode_envelope(frame);
            const auto action = envelope->payload;
            wire_->handle(peer, std::move(frame), action);
        }
    }
```

  `refusing(peer)` returns `false` until Task 9. `bind_peer` (Task 4) gains, between the handshake
  timer and the handshake's first message, the kept frames, each step looking the entry up again
  (review C1):

```cpp
        const detail::binding* mine = nullptr;
        {
            auto& state = peers_.try_emplace(peer, allocator()).first->second;
            // ... the binding and the handshake timer, as in Task 4 ...
            mine = state.link.get();
        }
        deliver_kept(peer, *c, mine);
        // A kept frame may have removed the peer: the handshake starts only on the state this
        // binding made.
        if (const auto it = peers_.find(peer); it != peers_.end() && it->second.link.get() == mine) {
            start_handshake(it->first, it->second);
        }
```

  At the end of `start()` (review M8: `bind_peer` needs `wire_` and `signaling_`, which `start`
  makes), the room sends `presence(1, token_)` on every live connection of the app and calls
  `try_bind(c->peer(), bind_event::joined)` for each. `on_connected` (Task 4) keeps binding the
  room whose own negotiation opened, whether or not the peer advertised the token yet (the
  negotiation proves the room key).
- [ ] **Step 4: Run** `shared`, `shared_hardened` (natively), `room`, `allocation`, natively, on
  emscripten and with gcc-15; and once `$B2 -a --build-dir=bin/asan address-sanitizer=norecover
  undefined-sanitizer=norecover libs/trystero/test//shared`. Expected: pass, with no sanitizer report.
- [ ] **Step 5: Commit** `"trystero: rooms bind on a peer's presence and its kept frames"`
  (`include/webcpp/trystero/room.hpp test/shared_test.cpp`).

### Task 7: Signaling reuse, the binding conditions, and a bound room's negotiation

**Carried from Task 4's review (controller, 2026-10-11):** `connection_ended` names the connection that ended; erase `met_` only when it was counted on that connection, so a late end of an old connection never allows a second reuse binding.

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/detail/signaling.hpp` (`callbacks::claim`,
  `handle`)
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`claim`, the met flag)
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 3 and 24)

**Interfaces:**
- Consumes: Task 6's `try_bind`.
- Produces: `signaling::callbacks::claim`, a `handler<bool(std::string_view peer)>` the signaling
  calls once a message has parsed and names another peer (`signaling.hpp:169-174`); `true` means
  the message goes no further.

- [ ] **Step 1: Write the failing tests:**

```cpp
/** Every relay text of `topics` the watcher reads: whether an offer or an answer was published. */
struct offers_on {
    std::vector<relayed> seen;
    bool offer_or_answer() const {
        return std::ranges::any_of(seen, [](const relayed& r) {
            return !r.offer.empty() || !r.answer.empty();
        });
    }
};

// Case 3: a late join, both directions: r2 joins over the existing connection, no connection is
// made, and no offer and no answer is published on r2's topics for the whole case.
void a_late_join_binds_over_the_connection() {
    for (const bool a_first : {true, false}) {
        two_apps t("LLLLLLLLLLLLLLLLLLLL", "MMMMMMMMMMMMMMMMMMMM");
        t.a_rooms.push_back(join(t.a, "r1"));
        t.b_rooms.push_back(join(t.b, "r1"));
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); })));
        flooder watcher(t.f, "r2");
        offers_on r2;
        watcher.on_text = [&](std::string_view text) {
            if (auto s = relayed_signal(text)) {
                r2.seen.push_back(std::move(*s));
            }
        };
        watcher.watch(trystero::protocol::root_topic(app_id, "r2"));
        watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.a.self_id()));
        watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.b.self_id()));
        const auto made = t.f.net.made_connections;
        if (a_first) {
            t.a_rooms.push_back(join(t.a, "r2"));
            advance(t.f, 500ms);
            t.b_rooms.push_back(join(t.b, "r2"));
        } else {
            t.b_rooms.push_back(join(t.b, "r2"));
            advance(t.f, 500ms);
            t.a_rooms.push_back(join(t.a, "r2"));
        }
        BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }));
        advance(t.f, 70s);  // past r2's regular announcements, at once, 233, 533, 1333, 5333 ms, 60 s
        BOOST_TEST_EQ(t.f.net.made_connections, made);
        BOOST_TEST(!r2.offer_or_answer());
    }
}

// Case 24: an announcement forged in X's name in r2, before X joins r2, binds nothing, reports
// nothing and publishes no offer; X then joins within a round trip (no timer runs meanwhile).
void a_forged_announcement_binds_nothing() {
    two_apps t("PPPPPPPPPPPPPPPPPPPP", "QQQQQQQQQQQQQQQQQQQQ");
    joined_in(t, {"r1"});
    t.a_rooms.push_back(join(t.a, "r2"));
    std::vector<boost::system::error_code> errors;
    t.a_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
        errors.push_back(ec);
    });
    const auto made = t.f.net.made_connections;
    flooder forger(t.f, "r2");
    forger.announce_as(t.b.self_id(), 3);
    advance(t.f, 15s);
    BOOST_TEST(t.a_rooms[1].peers().empty());
    BOOST_TEST(errors.empty());
    BOOST_TEST_EQ(t.f.net.made_connections, made);
    t.b_rooms.push_back(join(t.b, "r2"));
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 1ms));
}
```

  Both run on the virtual clock (`two_apps`): no wait depends on the wall clock. Case 24 holds this
  task's condition "the peer advertised the room itself"; case 4, which also needs the rebinding
  time and the kept-`@_leave` rule, is Task 10's (review I3).
- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//shared`. Expected (review I15):
  case 24 fails, `made_connections` grows: A's `r2` signaling offers to the forged announcement,
  since the app's connection to B is not consulted yet. Case 3 is a guard: the presence crosses the
  channel in one post and the announcement crosses the relay in several, so both rooms are usually
  bound, and their negotiation entries marked connected (Task 4's `signaling::bound`), before an
  announcement reaches them; it holds that order from now on.
- [ ] **Step 3: Implement.** In `signaling.hpp`, `callbacks` gains:

```cpp
        /**
         * Whether the room takes a parsed message from `peer` itself, and the negotiation does not:
         * the app holds a live connection to that peer (the room may bind on it, by reuse).
         */
        handler<bool(std::string_view peer)> claim;
```

  and `handle` (`signaling.hpp:165-197`) calls it right after the self check:

```cpp
        if (!message || message->peer_id == self_id_) {
            return;
        }
        if (cb_.claim(message->peer_id)) {
            return;
        }
```

  In `room.hpp`, the signaling's `claim` is:

```cpp
                .claim = bind<bool(std::string_view)>([weak](std::string_view peer) {
                    const auto self = weak.lock();
                    return self && self->claim(peer);
                }),
```

  with:

```cpp
    /** A message from `peer` reached this room's topics: when the app holds a live connection to
     * it, the negotiation stops there, and the room binds by reuse on the first meeting only
     * (the spec's section 4.2, path 3). */
    bool claim(std::string_view peer) {
        if (!app_->find(peer)) {
            return false;
        }
        if (!met(peer)) {
            try_bind(peer, bind_event::reuse);
        }
        return true;
    }
```

  `met(peer)` reads the `rebind_state` of Task 9; until Task 9, a room keeps a
  `std::pmr::set<std::pmr::string, std::less<>> met_` of peers it has bound over their current
  connection, erased in `connection_ended`, and `bind_peer` inserts into it.
- [ ] **Step 4: Run** `shared` and `room` natively, on emscripten and with gcc-15, and
  `shared_hardened` natively. Expected: pass.
- [ ] **Step 5: Commit** `"trystero: a connected peer's signals bind by reuse, its presence first"`
  (`include/webcpp/trystero/detail/signaling.hpp include/webcpp/trystero/room.hpp test/shared_test.cpp`).

### Task 8a: Departures, kicks and a connection's end, one room at a time

**Carried from Task 4's review (controller, 2026-10-11):** the review's mutants R2 and R3 (when announcements restart) get their tests here.

The review found the departure cases green before any code of this task (I15): the behaviour is
Tasks 2, 4 and 6's. They are pinned here, in a commit of tests alone, so that Task 8b's farewell
and Task 9's rebinding cannot break them unseen; a failing one is a defect of those tasks, fixed
there before going on. Task 8b, the farewell, is the red part (the review's suggested split).

**Files:**
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 8, 9, 10's per-room half and 11;
  Review Focus 2's connection failing under a waiting transfer)
- Modify: the code only if a case fails

**Interfaces:**
- Consumes: Task 6's `ends` and `internal_name`.
- Produces: `poll_until`, `is_presence` and `drop_first_challenge` in `shared_test.cpp`.

- [ ] **Step 1: Write the tests.**

```cpp
/** Runs one ready handler at a time until `done` holds, for at most `limit` handlers, moving no
 * clock. Tip: a buffering channel's drain posts itself again, so io_context::poll would drain it
 * whole; this one stops while the channel is still full. */
bool poll_until(fixture& f, const std::function<bool()>& done, std::size_t limit = 1'000'000) {
    for (std::size_t i = 0; i < limit && !done(); ++i) {
        f.io.restart();
        if (f.io.poll_one() == 0) {
            break;
        }
    }
    return done();
}

/** Whether `frame` is a presence frame of `room` saying `present`. */
bool is_presence(std::span<const std::uint8_t> frame, std::string_view room, bool present) {
    const auto env = trystero::protocol::decode_envelope(frame);
    return env && env->type == trystero::protocol::envelope::kind::presence &&
           env->token == trystero::protocol::room_token(app_id, room) && env->present == present;
}

// Case 8: one room leaves (by async_leave, by its destruction, by a room moved over it): only that
// room sees the peer go, presence(0, r1) arrives, r2 keeps exchanging, the connection stays, and a
// send in the room left completes with room_left.
void one_room_leaves() {
    for (int way = 0; way < 3; ++way) {
        two_apps t("RRRRRRRRRRRRRRRRRRRR", "SSSSSSSSSSSSSSSSSSSS");
        const ends e(t);
        joined_in(t, {"r1", "r2"});
        int b_r1_leaves = 0;
        int b_r2_leaves = 0;
        t.b_rooms[0].on_peer_leave([&](std::string_view) { ++b_r1_leaves; });
        t.b_rooms[1].on_peer_leave([&](std::string_view) { ++b_r2_leaves; });
        bool withdrawn = false;
        t.f.net.filter = [&](std::size_t from, std::size_t, std::span<const std::uint8_t> frame) {
            withdrawn = withdrawn || (from == e.a() && is_presence(frame, "r1", false));
            return trystero::test::memory_network::verdict::deliver;
        };
        auto r1_chat = t.a_rooms[0].make_action<std::string>("chat");
        if (way == 0) {
            bool done = false;
            t.a_rooms[0].async_leave([&](boost::system::error_code) { done = true; });
            BOOST_TEST(run_until(t.f, [&] { return done; }));
        } else if (way == 1) {
            t.a_rooms.erase(t.a_rooms.begin());
        } else {
            t.a_rooms[0] = join(t.a, "elsewhere");
        }
        BOOST_TEST(run_until(t.f, [&] { return b_r1_leaves == 1 && withdrawn; }));
        advance(t.f, 1s);
        BOOST_TEST_EQ(b_r2_leaves, 0);
        BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
        std::vector<std::string> got;
        auto in_r2 = t.b_rooms[1].make_action<std::string>("chat");
        in_r2.on_message([&](std::string s, const trystero::message_info&) { got.push_back(s); });
        auto& a_r2 = way == 1 ? t.a_rooms[0] : t.a_rooms[1];  // the erase moved r2 to the front
        a_r2.make_action<std::string>("chat").async_send("still", [](boost::system::error_code) {});
        BOOST_TEST(run_until(t.f, [&] { return got.size() == 1; }));
        if (way == 0) {
            std::optional<boost::system::error_code> sent;
            r1_chat.async_send("late", [&](boost::system::error_code ec) { sent = ec; });
            BOOST_TEST(run_until(t.f, [&] { return sent.has_value(); }));
            BOOST_TEST(*sent == trystero::error::room_left);
        }
    }
}
```

```cpp
// Case 9: the peer leaves every room: the connection closes after idle_timeout (200 ms), not
// before, on the virtual clock; a room joined again within it reuses it; with 0 it closes at once.
void a_peer_leaving_every_room_idles_the_connection() {
    for (const auto idle : {200ms, 0ms}) {
        fixture f{true};
        auto a_config = app_config("TTTTTTTTTTTTTTTTTTTT");
        a_config.idle_timeout = idle;
        trystero::app a(f.io.get_executor(), f.net, std::move(a_config));
        trystero::app b(f.io.get_executor(), f.net, app_config("UUUUUUUUUUUUUUUUUUUU"));
        auto a1 = join(a, "r1");
        auto a2 = join(a, "r2");
        std::optional<trystero::room> b1 = join(b, "r1");
        std::optional<trystero::room> b2 = join(b, "r2");
        webcpp::test::require(BOOST_TEST(run_until(f, [&] {
            return a1.peers().size() == 1 && a2.peers().size() == 1;
        })));
        b1.reset();
        b2.reset();
        BOOST_TEST(run_until(f, [&] { return a1.peers().empty() && a2.peers().empty(); }, 1ms));
        if (idle == 0ms) {
            BOOST_TEST(run_until(f, [&] { return f.net.live_connections() == 0; }, 1ms));
            continue;
        }
        advance(f, 100ms);
        BOOST_TEST_EQ(f.net.live_connections(), 2U);
        const auto made = f.net.made_connections;
        b1 = join(b, "r1");  // within the idle time: the connection is reused
        BOOST_TEST(run_until(f, [&] { return a1.peers().size() == 1; }, 1ms));
        BOOST_TEST_EQ(f.net.made_connections, made);
        b1.reset();
        BOOST_TEST(run_until(f, [&] { return a1.peers().empty(); }, 1ms));
        advance(f, 199ms);
        BOOST_TEST_EQ(f.net.live_connections(), 2U);  // not before
        advance(f, 1ms);
        BOOST_TEST(run_until(f, [&] { return f.net.live_connections() == 0; }, 1ms));
    }
}
```

  B's `app` stays alive while its rooms go, so B's end idles too, on B's default 123,333 ms: A's
  end closes first, at 200 ms, and the memory pair reports the close to B, so
  `live_connections() == 0` counts both ends.

```cpp
// Case 10, its per-room half: a kick is per room. B's r2 has another password and A the lower id:
// A reports wrong_password in r2 only and r1 stays joined over the same connection. Then a
// handshake that times out: B the lower id, its challenge dropped once by the filter; B reports
// handshake_timed_out in r2 only, 10 s after binding, and r1 stays. (The rest of case 10, the kick
// that closes the connection and the renegotiation after it, needs the rebinding times: Task 9.)
void a_kick_is_per_room() {
    {
        two_apps t("AAAAAAAAAAAAAAAAAAA1", "BBBBBBBBBBBBBBBBBBB2");
        joined_in(t, {"r1"});
        std::vector<boost::system::error_code> r2_errors;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "one"}));
        t.a_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            r2_errors.push_back(ec);
        });
        const auto made = t.f.net.made_connections;
        t.b_rooms.push_back(join(t.b, "r2", {.password = "two"}));
        BOOST_TEST(run_until(t.f, [&] { return !r2_errors.empty(); }));
        BOOST_TEST(r2_errors.at(0) == trystero::error::wrong_password);
        BOOST_TEST_EQ(t.a_rooms[0].peers().size(), 1U);
        BOOST_TEST_EQ(t.b_rooms[0].peers().size(), 1U);
        BOOST_TEST_EQ(t.f.net.made_connections, made);
        BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
    }
    {
        two_apps t("BBBBBBBBBBBBBBBBBBB1", "AAAAAAAAAAAAAAAAAAA2");  // B the lower id
        joined_in(t, {"r1"});
        drop_first_challenge(t);
        std::vector<boost::system::error_code> b_errors;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "same", .handshake_timeout = 30s}));
        const double joined = t.f.net.now_ms();
        t.b_rooms.push_back(join(t.b, "r2", {.password = "same"}));
        t.b_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            b_errors.push_back(ec);
        });
        BOOST_TEST(run_until(t.f, [&] { return !b_errors.empty(); }, 11s));
        BOOST_TEST(b_errors.at(0) == trystero::error::handshake_timed_out);
        BOOST_TEST_EQ(t.f.net.now_ms() - joined, 10'000.0);
        BOOST_TEST_EQ(t.a_rooms[0].peers().size(), 1U);
        BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
    }
}
```

  with, written before `a_kick_is_per_room` in the file, the filter it and Task 9 use:

```cpp
/** Drops the first @_hsdata of r2 on any end, B's challenge when B is the lower id. */
void drop_first_challenge(two_apps& t) {
    auto dropped = std::make_shared<bool>(false);
    t.f.net.filter = [dropped](std::size_t, std::size_t, std::span<const std::uint8_t> frame) {
        if (*dropped || internal_name(frame, "r2") != "@_hsdata") {
            return trystero::test::memory_network::verdict::deliver;
        }
        *dropped = true;
        return trystero::test::memory_network::verdict::drop;
    };
}

// Case 11: an action-level kick is per room: an undecodable action frame of r2 kicks B from r2
// only; r1 keeps exchanging.
void an_action_level_kick_is_per_room() {
    two_apps t("CCCCCCCCCCCCCCCCCCC1", "DDDDDDDDDDDDDDDDDDD2");
    const ends e(t);
    joined_in(t, {"r1", "r2"});
    int r2_leaves = 0;
    t.a_rooms[1].on_peer_leave([&](std::string_view) { ++r2_leaves; });
    const std::uint8_t garbage[] = {9, 9, 9};
    t.f.net.inject(e.a(), trystero::protocol::room_frame(
                              trystero::protocol::room_token(app_id, "r2"), garbage));
    BOOST_TEST(run_until(t.f, [&] { return r2_leaves == 1; }));
    std::vector<std::string> got;
    auto in_r1 = t.b_rooms[0].make_action<std::string>("chat");
    in_r1.on_message([&](std::string s, const trystero::message_info&) { got.push_back(s); });
    t.a_rooms[0].make_action<std::string>("chat").async_send("still", [](boost::system::error_code) {});
    BOOST_TEST(run_until(t.f, [&] { return got.size() == 1; }));
    BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
}

// Review Focus 2: the connection fails while a transfer of r2 waits on the full channel. Its
// end fails the drain waiter, whose completion fails the transfer and may send on the ended
// connection: the transfer completes once with an error, both rooms see B leave, and they meet
// again. Run under shared_hardened and the sanitizers too.
void a_connection_failing_under_a_waiting_transfer() {
    two_apps t("EEEEEEEEEEEEEEEEEEE1", "FFFFFFFFFFFFFFFFFFF2");
    t.f.net.channel_capacity = 1;
    const ends e(t);
    joined_in(t, {"r1", "r2"});
    auto receiver = t.b_rooms[1].make_action<trystero::bytes>("big");
    receiver.on_message([](trystero::bytes, const trystero::message_info&) {});
    int completions = 0;
    std::optional<boost::system::error_code> sent;
    t.a_rooms[1].make_action<trystero::bytes>("big").async_send(
        trystero::bytes(1'000'000, std::uint8_t{4}), [&](boost::system::error_code ec) {
            ++completions;
            sent = ec;
        });
    webcpp::test::require(BOOST_TEST(poll_until(t.f, [&] {
        return t.f.net.buffered(e.a()) > 64 * 1024;  // a chunk waits on the drain
    })));
    int leaves = 0;
    for (auto& r : t.a_rooms) {
        r.on_peer_leave([&leaves](std::string_view) { ++leaves; });
    }
    t.f.net.fail(e.a());
    BOOST_TEST(run_until(t.f, [&] { return leaves == 2 && sent.has_value(); }));
    BOOST_TEST(*sent == trystero::error::peer_disconnected);
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 30s));
    BOOST_TEST_EQ(completions, 1);
}
```

  Each is called from `main`.
- [ ] **Step 2: Run them.** `$B2 libs/trystero/test//shared libs/trystero/test//shared_hardened`
  natively, then `shared` on emscripten and with gcc-15, and once under the sanitizers
  (`$B2 -a --build-dir=bin/asan address-sanitizer=norecover undefined-sanitizer=norecover
  libs/trystero/test//shared`). Expected: pass, with no sanitizer report: these are guards. A
  failure is a defect of the task that brought the behaviour (Task 2 for the drain waiter, 4 for
  the departures and the idle time, 6 for the per-room kick), fixed there.
- [ ] **Step 3: Commit** `"trystero: departures and kicks are per room, pinned"`
  (`test/shared_test.cpp`).

### Task 8b: The farewell, joining during it, and the app's users

**Carried from Task 4's review (controller, 2026-10-11):** the first case with two rooms on one connection pins end_kind::room_end against a kick (the mutant survives with one room per app).

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`leave`, `abandon`,
  `finish_farewell`, `leaving`), `libs/trystero/include/webcpp/trystero/detail/app_impl.hpp`
  (`room_endpoint` gains `leaving` and `finish_farewell`),
  `libs/trystero/include/webcpp/trystero/app.hpp` (`join` finishes a farewell)
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 15, 16, 17; Review Focus 2's farewell
  and `io_context` paths; Review Focus 4)

**Interfaces:**
- Consumes: Tasks 5c and 8a.
- Produces: on `room_endpoint`, `[[nodiscard]] virtual bool leaving() const = 0;` (the 99 ms
  farewell runs) and `virtual void finish_farewell() = 0;` (ends it now: `presence(0)` sent, the
  room unregistered, every pending leave completion posted).

- [ ] **Step 1: Write the failing tests.**

```cpp
// Case 15, after the leave: join of a room id succeeds once async_leave has completed.
void a_room_left_can_be_joined_again() {
    fixture f{true};
    trystero::app a(f.io.get_executor(), f.net, app_config("GGGGGGGGGGGGGGGGGGG1"));
    auto r = join(a, "r");
    bool done = false;
    r.async_leave([&](boost::system::error_code) { done = true; });
    BOOST_TEST(run_until(f, [&] { return done; }));
    BOOST_TEST(a.join("r").has_value());
}

// Review Focus 4 and case 15's farewell half: join, async_leave twice, join during the farewell
// (which finishes it: both leaves complete, once each), the room destroyed during its farewell,
// join again. presence(0) and presence(1) go out in that order, and max_rooms counts a leaving
// room until its presence(0).
void join_and_leave_sequences_on_one_id() {
    two_apps t("IIIIIIIIIIIIIIIIIII1", "JJJJJJJJJJJJJJJJJJJ2");
    const ends e(t);
    joined_in(t, {"lobby"});
    std::vector<bool> presences;  // A's presence frames of r, in the order sent
    t.f.net.filter = [&](std::size_t from, std::size_t, std::span<const std::uint8_t> frame) {
        if (from == e.a() && is_presence(frame, "r", true)) {
            presences.push_back(true);
        } else if (from == e.a() && is_presence(frame, "r", false)) {
            presences.push_back(false);
        }
        return trystero::test::memory_network::verdict::deliver;
    };
    std::vector<trystero::room> others;
    for (int i = 0; i < 62; ++i) {
        others.push_back(join(t.a, "x" + std::to_string(i)));  // with lobby and r: 64 rooms
    }
    std::optional<trystero::room> r(join(t.a, "r"));
    t.b_rooms.push_back(join(t.b, "r"));
    webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return r->peers().size() == 1; })));
    int first = 0;
    int second = 0;
    r->async_leave([&](boost::system::error_code ec) { first += ec ? 100 : 1; });
    r->async_leave([&](boost::system::error_code ec) { second += ec ? 100 : 1; });
    const auto refused = t.a.join("x62");
    BOOST_TEST(!refused && refused.error() == trystero::error::too_many_rooms);  // r still counts
    advance(t.f, 50ms);
    BOOST_TEST_EQ(first + second, 0);  // the second leave waits for the farewell with the first
    BOOST_TEST((presences == std::vector<bool>{true}));
    auto again = t.a.join("r");  // finishes r's farewell at once
    webcpp::test::require(BOOST_TEST(again.has_value()));
    BOOST_TEST((presences == std::vector<bool>{true, false, true}));
    BOOST_TEST(run_until(t.f, [&] { return first == 1 && second == 1; }, 1ms));
    r.reset();  // the old handle, its room ended: nothing more
    std::optional<trystero::room> r2(std::move(*again));
    BOOST_TEST(run_until(t.f, [&] { return r2->peers().size() == 1; }));
    int third = 0;
    r2->async_leave([&](boost::system::error_code ec) { third += ec ? 100 : 1; });
    r2.reset();  // destroyed during its farewell: the farewell ends at once
    BOOST_TEST((presences == std::vector<bool>{true, false, true, false}));
    BOOST_TEST(t.a.join("r").has_value());
    BOOST_TEST((presences == std::vector<bool>{true, false, true, false, true}));
    advance(t.f, 1s);
    BOOST_TEST_EQ(first, 1);
    BOOST_TEST_EQ(second, 1);
    BOOST_TEST_EQ(third, 1);
}
```

  The room the last `join` returns is destroyed at once (its result is a temporary), which sends
  its `presence(0)` after the assertions; the case reads nothing after it.

```cpp
// Case 16: during a room's 99 ms farewell, B's presence and B's frames for it bind nothing
// (B binds A on A's earlier presence and is dropped when A's presence(0) goes out); and a
// connection the leaving room's own negotiation opens is closed, not adopted.
void the_farewell_binds_nothing() {
    {
        two_apps t("KKKKKKKKKKKKKKKKKKK1", "LLLLLLLLLLLLLLLLLLL2");
        joined_in(t, {"r1"});
        t.a_rooms.push_back(join(t.a, "r2"));
        bool left = false;
        t.a_rooms[1].async_leave([&](boost::system::error_code) { left = true; });
        int b_joins = 0;
        t.b_rooms.push_back(join(t.b, "r2"));  // B's presence, announcement and handshake now
        t.b_rooms[1].on_peer_join([&](std::string_view) { ++b_joins; });
        BOOST_TEST(run_until(t.f, [&] { return left; }));
        advance(t.f, 1s);
        BOOST_TEST(t.a_rooms[1].peers().empty());
        BOOST_TEST(t.b_rooms[1].peers().empty());
        BOOST_TEST_EQ(b_joins, 0);
        BOOST_TEST_EQ(t.f.net.live_connections(), 2U);
    }
    {
        fixture f{true};
        std::vector<std::string> a_log;
        auto a_config = app_config("MMMMMMMMMMMMMMMMMMM1");
        a_config.log = [&a_log](std::string_view, std::string_view text) { a_log.emplace_back(text); };
        trystero::app a(f.io.get_executor(), f.net, std::move(a_config));
        trystero::app b(f.io.get_executor(), f.net, app_config("NNNNNNNNNNNNNNNNNNN2"));
        std::vector<std::size_t> made;
        f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server>) {
            made.push_back(pc);
        };
        f.net.defer_opens = true;
        auto a2 = join(a, "r2");
        auto b2 = join(b, "r2");
        webcpp::test::require(BOOST_TEST(run_until(f, [&] {
            return made.size() == 2 && f.net.peer_of(made[0]) == made[1];
        })));
        bool left = false;
        a2.async_leave([&](boost::system::error_code) { left = true; });
        f.net.open(made[0]);
        f.net.open(made[1]);
        BOOST_TEST(run_until(f, [&] { return left; }));
        advance(f, 1s);
        BOOST_TEST(std::ranges::none_of(a_log, [](const std::string& line) {
            return line.find(" adopted") != std::string::npos;
        }));
        BOOST_TEST(b2.peers().empty());
        BOOST_TEST(run_until(f, [&] { return f.net.live_connections() == 0; }, 1s));
    }
}

// Case 17: the app handle destroyed before its rooms: they go on; the last room destroyed: every
// connection closes at that call.
void the_last_user_closes_every_connection() {
    fixture f{true};
    std::vector<std::string> a_log;
    auto a_config = app_config("OOOOOOOOOOOOOOOOOOO1");
    a_config.log = [&a_log](std::string_view, std::string_view text) { a_log.emplace_back(text); };
    std::optional<trystero::app> a(std::in_place, f.io.get_executor(), f.net, std::move(a_config));
    trystero::app b(f.io.get_executor(), f.net, app_config("PPPPPPPPPPPPPPPPPPP2"));
    std::optional<trystero::room> a1(join(*a, "r1"));
    std::optional<trystero::room> a2(join(*a, "r2"));
    auto b1 = join(b, "r1");
    auto b2 = join(b, "r2");
    webcpp::test::require(BOOST_TEST(run_until(f, [&] {
        return a1->peers().size() == 1 && a2->peers().size() == 1;
    })));
    const auto ended = [&a_log] {
        return std::ranges::count_if(a_log, [](const std::string& line) {
            return line.find(" ended: ") != std::string::npos;
        });
    };
    a.reset();
    a1.reset();
    advance(f, 1s);
    BOOST_TEST_EQ(a2->peers().size(), 1U);
    BOOST_TEST_EQ(ended(), 0);
    a2.reset();  // the last user
    BOOST_TEST_EQ(ended(), 1);  // at that call, before the io_context runs
    BOOST_TEST(run_until(f, [&] { return f.net.live_connections() == 0; }, 1ms));
}

// Case 17's third part, and Review Focus 2 (review I10): a room handle destroyed during its
// farewell, the handles gone, then the io_context destroyed with the rooms' operations pending:
// nothing is closed, logged or posted from a destructor, and the app's resource reaches zero.
void the_io_context_destroyed_with_operations_pending() {
    counting_resource memory;
    std::vector<std::string> late;
    bool destroying = false;
    {
        std::optional<fixture> f(std::in_place, true);
        auto a_config = app_config("QQQQQQQQQQQQQQQQQQQ1");
        a_config.memory = &memory;
        a_config.log = [&](std::string_view, std::string_view text) {
            if (destroying) {
                late.emplace_back(text);
            }
        };
        std::optional<trystero::app> a(std::in_place, f->io.get_executor(), f->net,
                                       std::move(a_config));
        std::optional<trystero::app> b(std::in_place, f->io.get_executor(), f->net,
                                       app_config("RRRRRRRRRRRRRRRRRRR2"));
        std::optional<trystero::room> a1(join(*a, "r1"));
        std::optional<trystero::room> a2(join(*a, "r2"));
        std::optional<trystero::room> b1(join(*b, "r1"));
        webcpp::test::require(BOOST_TEST(run_until(*f, [&] { return a1->peers().size() == 1; })));
        a1->async_leave([](boost::system::error_code) {});
        a1.reset();  // during its farewell: ended at once, its completion posted
        a2->async_ping(b->self_id(), [](boost::system::error_code, std::chrono::milliseconds) {});
        a2.reset();
        b1.reset();
        a.reset();
        b.reset();
        destroying = true;
        f.reset();  // the network, then the io_context and the operations still pending in it
    }
    BOOST_TEST(late.empty());
    BOOST_TEST_EQ(memory.in_use, 0U);
}
```

- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//shared`. Expected (review I15):
  `join_and_leave_sequences_on_one_id` fails at `first + second == 0` (today a second
  `async_leave` completes at once, `room.hpp:812-815`) and at `again.has_value()` (`join` refuses
  a room in its farewell with `already_joined`, Task 5c). The others are guards of Tasks 3 to 6
  (the refusal while leaving, the last user, the quiet destructors) and pass.
- [ ] **Step 3: Implement.** In `room.hpp`, the leave keeps its completions in the room's resource
  (`std::pmr::vector<completion<void(boost::system::error_code)>> leave_completions_`) and a flag
  `ended_` beside `left_` (review I9):

```cpp
    /** Says `\@_leave` to every peer, then ends the room after Trystero's 99 ms. A leave asked
     * during the farewell completes with the first; one asked once the room ended, at once. */
    void leave(completion<void(boost::system::error_code)> handler) {
        if (ended_) {
            complete(std::move(handler), boost::system::error_code{});
            return;
        }
        leave_completions_.push_back(std::move(handler));
        if (std::exchange(left_, true)) {
            return;
        }
        log_("room", "leaving room ", room_id_);
        for (const auto& [id, p] : peers_) {
            send_internal("@_leave", id);
        }
        // Trystero gives the farewell 99 ms to leave before closing the connections. The timer
        // holds the room weakly: a handle destroyed meanwhile ends the farewell itself (abandon).
        leave_timer_ = factory_.make_timer(memory_);
        leave_timer_->start(std::chrono::milliseconds(99), bind<void()>([weak = weak_from_this()] {
                                if (const auto self = weak.lock()) {
                                    self->finish_farewell();
                                }
                            }));
    }

    /** The room handle was destroyed: leaves at once, without the farewell's grace, or ends the
     * farewell that runs. */
    void abandon() {
        if (!std::exchange(left_, true)) {
            for (const auto& [id, p] : peers_) {
                send_internal("@_leave", id);
            }
        }
        finish_farewell();
    }

    [[nodiscard]] bool leaving() const override { return left_ && !ended_; }

    /** Ends the room now: its presence(0) sent, the room unregistered and the app's user released
     * (close_everything), then every pending leave completed, once. */
    void finish_farewell() override {
        if (std::exchange(ended_, true)) {
            return;
        }
        close_everything();
        auto completions = std::move(leave_completions_);
        leave_completions_.clear();
        for (auto& h : completions) {
            complete(std::move(h), boost::system::error_code{});
        }
    }
```

  `room_endpoint` (Task 3) gains the two pure virtual members of the Interfaces above, each with
  its Doc Comment. `app::join` (Task 5c) becomes:

```cpp
        if (const auto existing = impl_->find_room(room_id)) {
            if (!existing->leaving()) {
                return error::already_joined;
            }
            // Trystero's rejoin (C/strategy.mjs:38): the old room's presence(0) goes before the
            // new room's presence(1).
            existing->finish_farewell();
        }
```

  and its Doc Comment loses "(Task 8b: ...)". `room_impl`'s destructor stays Task 4's: it
  unregisters (a no-op once `close_everything` did) and lets its bindings go quietly, never
  ending a connection, logging or posting (review I10).
- [ ] **Step 4: Run** `shared`, `shared_hardened`, `room`, `native_room` and `allocation`,
  natively, on emscripten (not `native_room` nor `shared_hardened`) and with gcc-15 (the core
  targets); and `shared` once under the sanitizers. Expected: pass, no sanitizer report.
- [ ] **Step 5: Commit** `"trystero: the farewell ends once, and a join during it finishes it"`
  (`include/webcpp/trystero/room.hpp include/webcpp/trystero/detail/app_impl.hpp include/webcpp/trystero/app.hpp test/shared_test.cpp`).

### Task 9: Rebinding times, the lower id's backoff and the higher id's dropping window

**Carried from Task 4's review (controller, 2026-10-11):** `connection_ended` names the connection that ended; erase or reset `rebinds_` only when it was counted on that connection, so a late end of an old connection never shortens a backoff.

**Files:**
- Create: `libs/trystero/include/webcpp/trystero/detail/rebinding.hpp`
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp`
- Test: `libs/trystero/test/shared_test.cpp` (the step, S:6.2 case 20 and the rest of case 10;
  cases 4 and 23 need Task 10's kept-`@_leave` rule and are Task 10's, review I3)

**Interfaces:**
- Produces:

```cpp
namespace webcpp::trystero::detail {

/** The time from a binding's end to the room's next binding of that peer, for the `ends`-th end
 * since their last completed handshake: the larger of `handshake_timeout` and 10 s, plus 1 s,
 * doubled per end, to at most 5 minutes (the spec's section 4.2). Precondition: `ends >= 1`
 * (the count is incremented before the step is read; review M19). */
constexpr std::chrono::milliseconds rebinding_step(std::chrono::milliseconds handshake_timeout,
                                                   std::uint32_t ends);

/** The last challenges an initiator sent one peer in one room since their last completed
 * handshake, at most `capacity`. */
class recent_challenges {
public:
    static constexpr std::size_t capacity = 8;
    explicit recent_challenges(std::pmr::polymorphic_allocator<> a);
    void push(std::pmr::string challenge);
    template <class Matches> [[nodiscard]] bool any_of(Matches&& m) const;
    void clear();
};

/** What a room keeps of one peer over the app's live connection to it. */
struct rebind_state {
    explicit rebind_state(std::pmr::polymorphic_allocator<> a);
    std::uint32_t ends = 0;          // since the last completed handshake
    bool met = false;                // bound once over this connection
    bool refusing = false;           // the lower id, after its own kick, until the rebinding time
    bool dropping = false;           // the higher id, after its own kick, until it sends presence
    std::shared_ptr<backend::timer> timer;
    recent_challenges challenges;
};

}  // namespace webcpp::trystero::detail
```

- [ ] **Step 1: Write the failing tests.** The step is pure:

```cpp
void the_rebinding_step() {
    using trystero::detail::rebinding_step;
    BOOST_TEST(rebinding_step(10s, 1) == 11s);
    BOOST_TEST(rebinding_step(10s, 2) == 22s);
    BOOST_TEST(rebinding_step(10s, 3) == 44s);
    BOOST_TEST(rebinding_step(10s, 5) == 176s);
    BOOST_TEST(rebinding_step(10s, 6) == 300s);
    BOOST_TEST(rebinding_step(10s, 40) == 300s);
    BOOST_TEST(rebinding_step(2s, 1) == 11s);   // never below Trystero's 10 s keep time
    BOOST_TEST(rebinding_step(30s, 1) == 31s);
}
```

  Case 20, the rebinding rate, in both id orders, exactly on the virtual clock:

```cpp
// Case 20: the lower id reports wrong_password at 0, 11, 33 and 77 s after the first, exactly,
// and the fifth at 165 s, not before; the higher id reports nothing; r2's topics carry the regular
// announcements only, no offer and no answer.
void a_wrong_password_rebinds_at_the_steps() {
    for (const bool a_lower : {true, false}) {
        two_apps t(a_lower ? "AAAAAAAAAAAAAAAAAAA1" : "ZZZZZZZZZZZZZZZZZZZ1",
                   a_lower ? "ZZZZZZZZZZZZZZZZZZZ2" : "AAAAAAAAAAAAAAAAAAA2");
        joined_in(t, {"r1"});
        flooder watcher(t.f, "r2");
        std::size_t r2_announcements = 0;
        bool offer_or_answer = false;
        watcher.on_text = [&](std::string_view text) {
            if (auto s = relayed_signal(text)) {
                r2_announcements += s->announcement ? 1 : 0;
                offer_or_answer = offer_or_answer || !s->offer.empty() || !s->answer.empty();
            }
        };
        watcher.watch(trystero::protocol::root_topic(app_id, "r2"));
        watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.a.self_id()));
        watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.b.self_id()));
        std::vector<double> a_reports;
        std::vector<double> b_reports;
        const auto recorder = [&t](std::vector<double>& to) {
            return [&t, &to](std::string_view, boost::system::error_code ec, std::string_view) {
                if (ec == trystero::error::wrong_password) {
                    to.push_back(t.f.net.now_ms());
                }
            };
        };
        t.a_rooms.push_back(join(t.a, "r2", {.password = "right"}));
        t.b_rooms.push_back(join(t.b, "r2", {.password = "wrong"}));
        t.a_rooms[1].on_error(recorder(a_reports));
        t.b_rooms[1].on_error(recorder(b_reports));
        const auto& lower = a_lower ? a_reports : b_reports;
        const auto& higher = a_lower ? b_reports : a_reports;
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return !lower.empty(); }, 1s)));
        const double first = lower.front();
        advance(t.f, std::chrono::milliseconds(static_cast<std::int64_t>(first + 100'000 - t.f.net.now_ms())));
        const std::size_t announced_in_100_s = r2_announcements;
        advance(t.f, 64'999ms);  // to 164.999 s after the first
        BOOST_TEST_EQ(lower.size(), 4U);
        const double expected[] = {0, 11'000, 33'000, 77'000};
        for (std::size_t i = 0; i < std::min<std::size_t>(lower.size(), 4); ++i) {
            BOOST_TEST_EQ(lower[i] - first, expected[i]);
        }
        BOOST_TEST(run_until(t.f, [&] { return lower.size() == 5; }, 1ms));
        BOOST_TEST_EQ(lower.back() - first, 165'000.0);
        BOOST_TEST(higher.empty());
        BOOST_TEST(!offer_or_answer);
        // Each room's regular cadence on the relay the watcher reads: at once, after 233, 533,
        // 1333 and 5333 ms, then every 60 s: at most 6 each in 100 s, and no restart after a kick.
        BOOST_TEST_LE(announced_in_100_s, 12U);
    }
}
```

  The rest of case 10 (review I12, owner item O5): a kick that closes the connection, in both of
  the spec's forms:

```cpp
// Case 10, continued. (a) B's r2 has another password, A the lower id, and r1 is then left on
// both sides: A binds r2 again at its rebinding time, 11 s after its first kick, kicks B, and with
// no room left in common that kick closes the connection at once; B, whose r2 signals do not open
// with A's key, makes no new connection. (b) Only r2 in common, B the lower id, its challenge
// dropped once: B's timeout kick closes the connection, the rebinding counts go with it, and B
// renegotiates through r2's signaling: the new connection is adopted and bound with a count of
// zero, so the rooms join within 5 s of the kick, before the 11 s a kept count would impose.
void a_kick_that_closes_the_connection() {
    {
        two_apps t("AAAAAAAAAAAAAAAAAAK1", "BBBBBBBBBBBBBBBBBBK2");
        // B's connections carry B's own ICE server, so the test can tell them apart.
        auto b_config = app_config("BBBBBBBBBBBBBBBBBBK2");
        b_config.ice_servers = {{.urls = {"stun:b.test"}, .username = {}, .credential = {}}};
        t.b = trystero::app(t.f.io.get_executor(), t.f.net, std::move(b_config));
        std::size_t b_made = 0;
        t.f.net.on_made = [&](std::size_t, std::span<const trystero::backend::ice_server> servers) {
            b_made += !servers.empty() && servers[0].urls.at(0) == "stun:b.test" ? 1 : 0;
        };
        joined_in(t, {"r1"});
        std::vector<double> kicks;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "one"}));
        t.a_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            if (ec == trystero::error::wrong_password) {
                kicks.push_back(t.f.net.now_ms());
            }
        });
        t.b_rooms.push_back(join(t.b, "r2", {.password = "two"}));
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return kicks.size() == 1; }, 1s)));
        t.b_rooms.erase(t.b_rooms.begin());  // r1 left on both sides: r2 is all they share
        t.a_rooms.erase(t.a_rooms.begin());
        const auto b_made_before = b_made;
        BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 0; }, 12s));
        BOOST_TEST_EQ(kicks.size(), 2U);
        BOOST_TEST_EQ(t.f.net.now_ms() - kicks.front(), 11'000.0);
        advance(t.f, 60s);
        BOOST_TEST_EQ(b_made, b_made_before);
        BOOST_TEST(t.a_rooms[0].peers().empty());
    }
    {
        two_apps t("BBBBBBBBBBBBBBBBBBL1", "AAAAAAAAAAAAAAAAAAL2");  // B the lower id
        drop_first_challenge(t);
        std::vector<double> b_timeouts;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "same", .handshake_timeout = 30s}));
        t.b_rooms.push_back(join(t.b, "r2", {.password = "same"}));
        t.b_rooms[0].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            if (ec == trystero::error::handshake_timed_out) {
                b_timeouts.push_back(t.f.net.now_ms());
            }
        });
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return !b_timeouts.empty(); }, 30s)));
        const auto made = t.f.net.made_connections;
        BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 5s));
        BOOST_TEST_LT(t.f.net.now_ms() - b_timeouts.front(), 5'000.0);
        BOOST_TEST_GE(t.f.net.made_connections, made + 2);  // a new pair, adopted
        BOOST_TEST(run_until(t.f, [&] { return t.f.net.live_connections() == 2U; }, 1ms));
        BOOST_TEST_EQ(b_timeouts.size(), 1U);
    }
}
```

- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//shared`. Expected (review I15):
  `rebinding.hpp` is missing; once it exists with the step alone, case 20 sees no second report
  (nothing rebinds a kicked peer over a live connection) and case 10 (a) times out on
  `live_connections() == 0` (the idle connection would close only at 123,333 ms). Case 10 (b) is a
  guard: it passes before this task and pins that a connection's end erases the counts this task
  adds.
- [ ] **Step 3: Implement `rebinding.hpp`:**

```cpp
constexpr std::chrono::milliseconds rebinding_step(std::chrono::milliseconds handshake_timeout,
                                                   std::uint32_t ends) {
    BOOST_ASSERT(ends >= 1);
    constexpr std::chrono::milliseconds keep_time{10'000};  // Trystero's (C/data-limits.mjs:4)
    constexpr std::chrono::milliseconds margin{1'000};
    constexpr std::chrono::milliseconds cap{300'000};
    auto step = std::max(handshake_timeout, keep_time) + margin;
    for (std::uint32_t i = 1; i < ends && step < cap; ++i) {
        step *= 2;
    }
    return std::min(step, cap);
}
```

  `recent_challenges` is a `std::pmr::deque<std::pmr::string>` that drops its front past 8.
  In `room.hpp`, every lookup is a `find` (review M9: `operator[]` inserts and does not compile
  in a `const` member; `at` throws):
  - `rebinds_`: `std::pmr::map<std::pmr::string, rebind_state, std::less<>>`, an entry made by
    `bind_peer` (`met = true`), erased in `connection_ended` (the spec's section 4.2: a
    connection's end erases the room's counts for that peer, which case 10 (b) pins); `met_` of
    Task 7 goes, and `met(peer)` reads the entry.

```cpp
    [[nodiscard]] bool met(std::string_view peer) const {
        const auto it = rebinds_.find(peer);
        return it != rebinds_.end() && it->second.met;
    }

    [[nodiscard]] bool refusing(std::string_view peer) const {
        const auto it = rebinds_.find(peer);
        return it != rebinds_.end() && it->second.refusing;
    }

    [[nodiscard]] bool keeps_frames_from(std::string_view peer) const override {
        const auto it = rebinds_.find(peer);
        return it == rebinds_.end() || !it->second.dropping;
    }
```

  - `remove_peer`, after the binding is detached and before `signaling_->forget(peer)`, when the
    end is `end_kind::kick` or `end_kind::peer_left` and the connection is still live
    (`app_->find(peer)`), calls `schedule_rebinding(peer, kind == end_kind::kick)`:

```cpp
    /** After a binding with `peer` ended over a live connection, by a kick of either side: the
     * next binding at one step (detail::rebinding_step); meanwhile the lower id refuses after its
     * own kick, and the higher id drops the peer's frames after its own kick, until it sends
     * presence (the spec's section 4.2, ruling 9). */
    void schedule_rebinding(std::string_view peer, bool own_kick) {
        auto& r = rebinds_.try_emplace(std::pmr::string(peer, memory_), allocator()).first->second;
        ++r.ends;
        const bool lower = app_->self_id() < peer;
        r.refusing = own_kick && lower;
        r.dropping = own_kick && !lower;
        r.timer = factory_.make_timer(memory_);
        r.timer->start(rebinding_step(options_.handshake_timeout, r.ends),
                       bind<void()>([weak = weak_from_this(), peer = std::pmr::string(peer, memory_)] {
                           if (const auto self = weak.lock()) {
                               self->on_rebinding_time(peer);
                           }
                       }));
    }

    void on_rebinding_time(std::string_view peer) {
        const auto it = rebinds_.find(peer);
        if (it == rebinds_.end()) {
            return;
        }
        it->second.refusing = false;
        if (peers_.contains(peer)) {
            return;  // something bound it before its time
        }
        if (const auto c = app_->find(peer); c && c->advertised(token_)) {
            it->second.dropping = false;
            try_bind(peer, bind_event::rebinding);
        }
    }
```

  - `presence_changed(peer, true)`, for a room whose `dropping` runs for that peer: clear
    `dropping`, then `try_bind(peer, bind_event::reuse)`, which sends presence (its reply) and
    binds.
  - A completed handshake (`maybe_activate`, `room.hpp:1195-1207`) resets `ends` to 0 and clears
    `challenges` of the peer's entry, found with `find`.
- [ ] **Step 4: Run** `shared` and `shared_hardened` natively, `shared` on emscripten and with
  gcc-15. Expected: pass. The binary's run time is the CPU's alone: case 20 covers 165 s of
  virtual time in the steps of its timers.
- [ ] **Step 5: Commit** `"trystero: rebinding times after a kick, refused by the lower id alone"`
  (`include/webcpp/trystero/detail/rebinding.hpp include/webcpp/trystero/room.hpp test/shared_test.cpp`).

### Task 10: The handshake across a rebinding, and a kept `@_leave`

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`start_handshake`,
  `on_handshake_data`, `deliver_kept`)
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 4, 22 with its eight variants and its
  third and fourth, 23 and 25)

**Interfaces:**
- Consumes: Task 9's `rebind_state::challenges`; Task 6's `internal_name`.
- Produces: the initiator accepts a correct answer to any of its last 8 challenges since the last
  completed handshake; a response that matches none is ignored before the current binding's
  challenge is sent and is `wrong_password` after; the responder answers one challenge per binding
  (unchanged); a kept `@_leave` discards itself and every frame kept before it (R3).

- [ ] **Step 1: Write the failing tests.** Every case runs on the virtual clock and states its
  instants exactly (review I13, owner item O2).

```cpp
/** Holds, at the receiving end, the first frame from `from` that carries the internal action
 * `message` of r2, and, as the channel delivers in order, every later frame of that end (Task 1).
 * Counts, in `challenges`, every @_hsdata of r2 that `counted` sends. */
void hold_first(two_apps& t, std::size_t from, std::string message, std::size_t counted = 0,
                std::size_t* challenges = nullptr) {
    auto held = std::make_shared<bool>(false);
    t.f.net.filter = [from, message = std::move(message), held, counted, challenges](
                         std::size_t sender, std::size_t, std::span<const std::uint8_t> frame) {
        const auto name = internal_name(frame, "r2");
        if (challenges != nullptr && sender == counted && name == "@_hsdata") {
            ++*challenges;
        }
        if (*held || sender != from || name != message) {
            return trystero::test::memory_network::verdict::deliver;
        }
        *held = true;
        return trystero::test::memory_network::verdict::hold;
    };
}

// Case 4: binding needs the peer's own presence. The filter drops B's presence(1, r2) while B
// joins r2, A having joined it first: B binds at its join, on A's presence, and its handshake
// frames reach A, which keeps them and does not bind; B's announcements in r2 bind nothing and
// start no negotiation. B's handshake times out 10 s after its join and B, the higher id, kicks;
// at its rebinding time, 21 s after its join, B binds again and sends its presence, which goes
// through now: A binds at that instant. (Owner item O4: the spec's narrative now reads so.)
void binding_needs_the_peers_own_presence() {
    two_apps t("NNNNNNNNNNNNNNNNNNNN", "OOOOOOOOOOOOOOOOOOOO");
    const ends e(t);
    joined_in(t, {"r1"});
    t.a_rooms.push_back(join(t.a, "r2"));
    advance(t.f, 300ms);
    bool drop = true;
    t.f.net.filter = [&](std::size_t from, std::size_t, std::span<const std::uint8_t> frame) {
        return drop && from != e.a() && is_presence(frame, "r2", true)
                   ? trystero::test::memory_network::verdict::drop
                   : trystero::test::memory_network::verdict::deliver;
    };
    flooder watcher(t.f, "r2");
    bool offer_or_answer = false;
    watcher.on_text = [&](std::string_view text) {
        if (auto s = relayed_signal(text)) {
            offer_or_answer = offer_or_answer || !s->offer.empty() || !s->answer.empty();
        }
    };
    watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.a.self_id()));
    watcher.watch(trystero::protocol::peer_topic(app_id, "r2", t.b.self_id()));
    std::vector<boost::system::error_code> b_errors;
    const double joined = t.f.net.now_ms();
    t.b_rooms.push_back(join(t.b, "r2"));
    t.b_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
        b_errors.push_back(ec);
    });
    advance(t.f, 10'999ms);
    BOOST_TEST(t.a_rooms[1].peers().empty());
    BOOST_TEST((b_errors == std::vector<boost::system::error_code>{
                    make_error_code(trystero::error::handshake_timed_out)}));
    drop = false;
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 11s));
    BOOST_TEST_EQ(t.f.net.now_ms() - joined, 21'000.0);
    BOOST_TEST(!offer_or_answer);
}
```

  Case 22, its eight variants: both id orders, with and without a password, the held frame
  released 2 s after the kick (before the rebinding time) and 13 s after it (after). The side that
  must not time out has `handshake_timeout` 30 s (ruling 14, N2). Both rooms are joined at the
  later of the rebinding time, 11 s after the kick, and the release, exactly:

```cpp
void a_transient_failure_converges() {
    for (const bool password : {true, false}) {
        for (const bool lower_times_out : {true, false}) {
            for (const bool release_after_rebinding : {false, true}) {
                two_apps t("AAAAAAAAAAAAAAAAAAD1", "ZZZZZZZZZZZZZZZZZZD2");  // A the lower id
                const ends e(t);
                joined_in(t, {"r1"});
                const auto a_end = e.a();
                const auto b_end = t.f.net.peer_of(a_end);
                // A times out on B's answer (without a password, B's @_hsready); B on A's
                // challenge (or A's @_hsready).
                hold_first(t, lower_times_out ? b_end : a_end, password ? "@_hsdata" : "@_hsready");
                const trystero::room_options patient{.password = password ? "p" : "",
                                                     .handshake_timeout = 30s};
                const trystero::room_options normal{.password = password ? "p" : ""};
                std::vector<boost::system::error_code> a_errors;
                std::vector<boost::system::error_code> b_errors;
                t.a_rooms.push_back(join(t.a, "r2", lower_times_out ? normal : patient));
                t.b_rooms.push_back(join(t.b, "r2", lower_times_out ? patient : normal));
                t.a_rooms[1].on_error(
                    [&](std::string_view, boost::system::error_code ec, std::string_view) {
                        a_errors.push_back(ec);
                    });
                t.b_rooms[1].on_error(
                    [&](std::string_view, boost::system::error_code ec, std::string_view) {
                        b_errors.push_back(ec);
                    });
                const auto& timed_out = lower_times_out ? a_errors : b_errors;
                webcpp::test::require(
                    BOOST_TEST(run_until(t.f, [&] { return !timed_out.empty(); }, 11s)));
                const double kick = t.f.net.now_ms();
                const std::chrono::milliseconds release_after = release_after_rebinding ? 13s : 2s;
                advance(t.f, release_after);
                t.f.net.filter = {};
                t.f.net.release(lower_times_out ? a_end : b_end);  // held at the receiving end
                BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 12s));
                BOOST_TEST_EQ(t.f.net.now_ms() - kick,
                              static_cast<double>(std::max<std::int64_t>(11'000, release_after.count())));
                BOOST_TEST_EQ(timed_out.size(), 1U);
                BOOST_TEST(timed_out.at(0) == trystero::error::handshake_timed_out);
                BOOST_TEST_EQ(a_errors.size() + b_errors.size(), 1U);  // no other report
            }
        }
    }
}
```

  Its third and fourth variants (owner item O3, review I8): B's answers are held across A's first
  two bindings and released during A's third, once A has sent its third challenge; then across
  eight bindings, released during the ninth:

```cpp
// Case 22's third and fourth variants: an answer to a challenge two bindings old passes a bound
// initiator; one to a challenge nine bindings old, no longer among the last 8, is wrong_password.
void an_answer_to_an_old_challenge() {
    for (const std::size_t binding : {3U, 9U}) {
        two_apps t("AAAAAAAAAAAAAAAAAAJ1", "ZZZZZZZZZZZZZZZZZZJ2");
        const ends e(t);
        joined_in(t, {"r1"});
        const auto a_end = e.a();
        const auto b_end = t.f.net.peer_of(a_end);
        std::size_t a_challenges = 0;
        hold_first(t, b_end, "@_hsdata", a_end, &a_challenges);  // B's first answer and after
        std::vector<boost::system::error_code> a_errors;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "p"}));
        t.b_rooms.push_back(join(t.b, "r2", {.password = "p", .handshake_timeout = 30s}));
        t.a_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            a_errors.push_back(ec);
        });
        // A's bindings start at 0, 21, 53, 107, 205, 391, 701, 1011 and 1321 s.
        webcpp::test::require(
            BOOST_TEST(run_until(t.f, [&] { return a_challenges == binding; }, 1400s)));
        BOOST_TEST_EQ(std::ranges::count(a_errors, trystero::error::handshake_timed_out),
                      static_cast<std::ptrdiff_t>(binding - 1));
        t.f.net.filter = {};
        t.f.net.release(a_end);
        const auto wrong = [&] {
            return std::ranges::count(a_errors, trystero::error::wrong_password);
        };
        if (binding == 3) {
            BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 1ms));
            BOOST_TEST_EQ(wrong(), 0);
        } else {
            BOOST_TEST(run_until(t.f, [&] { return wrong() == 1; }, 1ms));
            BOOST_TEST(t.a_rooms[1].peers().empty());
        }
    }
}
```

  Case 23, a corrected password, in both id orders:

```cpp
// Case 23: A the lower id, B's r2 password wrong: after A's third kick, at 33 s, B leaves r2 and
// joins it with the right password at 35 s; B joins at 77 s, A's rebinding time, exactly, having
// reported at most two handshake_timed_out. With B the lower id and its own password wrong, B's
// new join joins at once: its rebinding count was its old room's.
void a_corrected_password_rejoins() {
    {
        two_apps t("AAAAAAAAAAAAAAAAAAB1", "ZZZZZZZZZZZZZZZZZZB2");
        joined_in(t, {"r1"});
        std::vector<double> a_kicks;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "right"}));
        t.a_rooms[1].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            if (ec == trystero::error::wrong_password) {
                a_kicks.push_back(t.f.net.now_ms());
            }
        });
        std::optional<trystero::room> b2(join(t.b, "r2", {.password = "wrong"}));
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return a_kicks.size() == 3; }, 40s)));
        const double first = a_kicks.front();
        BOOST_TEST_EQ(a_kicks.back() - first, 33'000.0);
        advance(t.f, 2s);
        b2.reset();
        b2.emplace(join(t.b, "r2", {.password = "right"}));
        std::vector<boost::system::error_code> b_errors;
        b2->on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            b_errors.push_back(ec);
        });
        BOOST_TEST(run_until(t.f, [&] { return b2->peers().size() == 1; }, 45s));
        BOOST_TEST_EQ(t.f.net.now_ms() - first, 77'000.0);
        BOOST_TEST_LE(std::ranges::count(b_errors, trystero::error::handshake_timed_out), 2);
        BOOST_TEST_EQ(a_kicks.size(), 3U);
    }
    {
        two_apps t("ZZZZZZZZZZZZZZZZZZC1", "AAAAAAAAAAAAAAAAAAC2");  // B the lower id
        joined_in(t, {"r1"});
        std::vector<boost::system::error_code> b_errors;
        t.a_rooms.push_back(join(t.a, "r2", {.password = "right"}));
        std::optional<trystero::room> b2(join(t.b, "r2", {.password = "wrong"}));
        b2->on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            b_errors.push_back(ec);
        });
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] { return !b_errors.empty(); }, 1s)));
        advance(t.f, 3s);
        b2.reset();
        b2.emplace(join(t.b, "r2", {.password = "right"}));
        BOOST_TEST(run_until(t.f, [&] { return b2->peers().size() == 1; }, 1ms));
    }
}

// Case 25: before A's r2 binds B, inject delivers on A's end B's @_hsready and @_leave of r2; then
// B's presence is let through: A binds, discards both, and the handshake completes on B's next
// frames, with no leave: A's log names no peer gone.
void a_kept_leave_discards_what_came_before() {
    std::vector<std::string> a_log;
    auto a_config = app_config("AAAAAAAAAAAAAAAAAAE1");
    a_config.log = [&a_log](std::string_view, std::string_view text) { a_log.emplace_back(text); };
    two_apps t("AAAAAAAAAAAAAAAAAAE1", "ZZZZZZZZZZZZZZZZZZE2");
    t.a = trystero::app(t.f.io.get_executor(), t.f.net, std::move(a_config));
    const ends e(t);
    joined_in(t, {"r1"});
    bool hold_presence = true;
    t.f.net.filter = [&](std::size_t, std::size_t, std::span<const std::uint8_t> frame) {
        return hold_presence && is_presence(frame, "r2", true)
                   ? trystero::test::memory_network::verdict::hold
                   : trystero::test::memory_network::verdict::deliver;
    };
    t.a_rooms.push_back(join(t.a, "r2"));
    t.b_rooms.push_back(join(t.b, "r2"));
    for (const auto* name : {"@_hsready", "@_leave"}) {
        t.f.net.inject(e.a(), internal_frame("r2", name));
    }
    advance(t.f, 200ms);
    BOOST_TEST(t.a_rooms[1].peers().empty());
    hold_presence = false;
    t.f.net.release(e.a());
    t.f.net.release(t.f.net.peer_of(e.a()));
    BOOST_TEST(run_until(t.f, [&] { return t.all_joined(); }, 1ms));
    BOOST_TEST(std::ranges::none_of(a_log, [](const std::string& line) {
        return line.find(" gone: ") != std::string::npos;
    }));
}
```

- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//shared`. Expected (review I15):
  the password variants of case 22 in which an answer to an earlier challenge reaches A (A times
  out and the release comes before the rebinding time: the kept answer is checked against the new
  binding's challenge, still empty; B times out and the release comes after it: B answers the held
  first challenge, A checks the second) report a false `wrong_password` (`room.hpp:1157-1163`
  compares with the current challenge only), and so does the third variant; case 25 finds a
  `gone:` line (the kept `@_leave` is delivered and removes B). The other variants of case 22, its
  fourth (`wrong_password` either way), and cases 4 and 23 are guards: they hold with the kept
  frames' 10 s expiry and pin the R3 rule's effect from now on.
- [ ] **Step 3: Implement.** `peer_state` gains `bool challenge_sent = false;`. `start_handshake`
  (`room.hpp:1130-1142`), for the initiator, pushes the new challenge into the peer's
  `rebind_state::challenges` (found with `find`; `bind_peer` made the entry) and sets
  `state.challenge_sent = true` after sending; it sends the challenge even when a kept answer has
  already passed the initiator. `on_handshake_data` (`room.hpp:1144-1177`), for the initiator:

```cpp
        if (app_->self_id() < peer) {
            const auto h = field("h");
            const auto r = rebinds_.find(peer);
            const bool proves =
                field("__trystero_pw") == "response" && r != rebinds_.end() &&
                r->second.challenges.any_of([&](std::string_view c) {
                    return h == protocol::handshake_proof(c, options_.password, app_->app_id(),
                                                          room_id_, memory_);
                });
            if (proves) {
                pass_local(peer);
                return;
            }
            if (!it->second.challenge_sent) {
                return;  // a late answer of an ended binding, before this one's challenge
            }
            fail_handshake(peer);
            return;
        }
```

  The guard at the top of `on_handshake_data` (`room.hpp:1145-1147`) keeps returning once
  `local_passed` (one answer per binding for the responder; nothing more checked by a passed
  initiator). `deliver_kept` (Task 6) gains the R3 rule, keeping its guard (review C1):

```cpp
    /** Whether a kept frame is a `\@_leave`, which ends a binding of the other side. */
    static bool is_leave(std::span<const std::uint8_t> frame) {
        const auto e = protocol::decode_envelope(frame);
        const auto d = e ? protocol::decode_action(e->payload) : protocol::decode_result{};
        const auto* p = d.frame ? std::get_if<protocol::proposal>(&*d.frame) : nullptr;
        return p != nullptr && p->name == "@_leave";
    }

    /** Delivers the frames `c` kept for this room, in order, but a kept @_leave and everything
     * kept before it, which belong to a binding of the other side that has ended (the spec's
     * ruling 14, R3). Stops once the peer's entry is gone or holds another binding (review C1). */
    void deliver_kept(const std::pmr::string& peer, connection& c, const detail::binding* mine) {
        auto frames = c.take_kept(token_);
        std::size_t first = 0;
        for (std::size_t i = 0; i < frames.size(); ++i) {
            if (is_leave(frames[i])) {
                first = i + 1;
            }
        }
        for (std::size_t i = first; i < frames.size(); ++i) {
            if (const auto it = peers_.find(peer); it == peers_.end() || it->second.link.get() != mine) {
                return;
            }
            const auto envelope = protocol::decode_envelope(frames[i]);
            const auto action = envelope->payload;
            wire_->handle(peer, std::move(frames[i]), action);
        }
    }
```

- [ ] **Step 4: Run** `shared` and `shared_hardened` natively, `shared` on emscripten and with
  gcc-15, and `shared` once under the sanitizers. Expected: pass.
- [ ] **Step 5: Commit** `"trystero: a late answer to a recent challenge passes; a kept leave is stale"`
  (`include/webcpp/trystero/room.hpp test/shared_test.cpp`).

### Task 11: Crossed connections, flood isolation and the drain order

**Files:**
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 cases 7 and 14)
- Modify: the code only if a case fails (the behaviour is Tasks 2 to 8b's)

- [ ] **Step 1: Write the tests.**

```cpp
// Case 7: crossed connections recover: A's end of r1's pair and B's end of r2's open first; both
// sides lose both, then both rooms join again within 15 s of virtual time, over one connection.
void crossed_connections_recover() {
    fixture f{true};
    auto a_config = app_config("AAAAAAAAAAAAAAAAAAF1");
    a_config.ice_servers = {{.urls = {"stun:a.test"}, .username = {}, .credential = {}}};
    auto b_config = app_config("ZZZZZZZZZZZZZZZZZZF2");
    b_config.ice_servers = {{.urls = {"stun:b.test"}, .username = {}, .credential = {}}};
    trystero::app a(f.io.get_executor(), f.net, std::move(a_config));
    trystero::app b(f.io.get_executor(), f.net, std::move(b_config));
    std::vector<std::size_t> a_made;
    f.net.on_made = [&](std::size_t pc, std::span<const trystero::backend::ice_server> servers) {
        if (!servers.empty() && servers[0].urls.at(0) == "stun:a.test") {
            a_made.push_back(pc);
        }
    };
    f.net.defer_opens = true;
    auto a1 = join(a, "r1");
    auto b1 = join(b, "r1");
    webcpp::test::require(BOOST_TEST(run_until(f, [&] {
        return !a_made.empty() && f.net.peer_of(a_made[0]) != a_made[0];
    })));
    const auto a_r1 = a_made[0];
    const auto b_r1 = f.net.peer_of(a_r1);
    auto a2 = join(a, "r2");
    auto b2 = join(b, "r2");
    webcpp::test::require(BOOST_TEST(run_until(f, [&] {
        return a_made.size() >= 2 && f.net.peer_of(a_made[1]) != a_made[1];
    })));
    const auto a_r2 = a_made[1];
    const auto b_r2 = f.net.peer_of(a_r2);
    f.net.open(a_r1);
    f.net.open(b_r2);
    advance(f, 100ms);
    f.net.open(b_r1);
    f.net.open(a_r2);
    f.net.defer_opens = false;
    const double opened = f.net.now_ms();
    BOOST_TEST(run_until(f, [&] {
        return a1.peers().size() == 1 && a2.peers().size() == 1 && b1.peers().size() == 1 &&
               b2.peers().size() == 1;
    }, 15s));
    BOOST_TEST_LE(f.net.now_ms() - opened, 15'000.0);
    BOOST_TEST(run_until(f, [&] { return f.net.live_connections() == 2U; }));
}

// Case 14: flood isolation. B floods r1 with 10,000 inline messages to an action without a handler
// (A's queue keeps 1024) while r2 pings and chats: every r2 message arrives and every ping
// completes without error while r1's flood is queued. (The spec's "within 1 s" is not asserted:
// on the virtual clock no time passes while posts run, so every round trip measures 0; review N5.)
// Then, with a buffering channel, B sends a 5 MB transfer in r1 and, once
// the channel holds more than 64 KiB, 20 chats in r2, which must wait for the drain (review I6):
// none enters the channel at once, and every chat completes before the transfer does.
void a_flood_in_one_room_does_not_starve_another() {
    two_apps t("AAAAAAAAAAAAAAAAAAG1", "ZZZZZZZZZZZZZZZZZZG2");
    const ends e(t);
    joined_in(t, {"r1", "r2"});
    const auto b_end = t.f.net.peer_of(e.a());
    static_cast<void>(t.a_rooms[0].make_action<std::string>("unread"));  // made, no handler: queued
    auto flood = t.b_rooms[0].make_action<std::string>("unread");
    for (int i = 0; i < 10'000; ++i) {
        flood.async_send("x", [](boost::system::error_code) {});
    }
    std::vector<std::string> r2_got;
    auto r2_in = t.a_rooms[1].make_action<std::string>("chat");
    r2_in.on_message([&](std::string s, const trystero::message_info&) { r2_got.push_back(s); });
    auto r2_out = t.b_rooms[1].make_action<std::string>("chat");
    for (int i = 0; i < 100; ++i) {
        r2_out.async_send(std::to_string(i), [](boost::system::error_code) {});
    }
    std::vector<std::chrono::milliseconds> pings;
    for (int i = 0; i < 5; ++i) {
        t.b_rooms[1].async_ping(t.a.self_id(),
                                [&](boost::system::error_code ec, std::chrono::milliseconds rtt) {
                                    if (!ec) {
                                        pings.push_back(rtt);
                                    }
                                });
    }
    // A ping is recorded only without an error: five of them, and every r2 message, arrive while
    // r1's 10,000 are queued at A.
    BOOST_TEST(run_until(t.f, [&] { return pings.size() == 5 && r2_got.size() == 100; }, 10s));
    t.f.net.channel_capacity = 1;
    bool transfer_done = false;
    std::size_t chats_done = 0;
    bool chats_first = false;
    auto big_in = t.a_rooms[0].make_action<trystero::bytes>("big");
    big_in.on_message([](trystero::bytes, const trystero::message_info&) {});
    t.b_rooms[0].make_action<trystero::bytes>("big").async_send(
        trystero::bytes(5'000'000, std::uint8_t{1}),
        [&](boost::system::error_code) { transfer_done = true; });
    webcpp::test::require(
        BOOST_TEST(poll_until(t.f, [&] { return t.f.net.buffered(b_end) > 64 * 1024; })));
    r2_got.clear();
    const auto full = t.f.net.buffered(b_end);
    for (int i = 0; i < 20; ++i) {
        r2_out.async_send("c", [&](boost::system::error_code) {
            if (++chats_done == 20) {
                chats_first = !transfer_done;
            }
        });
    }
    BOOST_TEST_EQ(t.f.net.buffered(b_end), full);  // the chats wait for the drain, not inline
    BOOST_TEST(run_until(t.f, [&] { return transfer_done && chats_done == 20; }, 60s));
    BOOST_TEST(chats_first);
    BOOST_TEST_EQ(r2_got.size(), 20U);
}
```

- [ ] **Step 2: Run them.** `$B2 libs/trystero/test//shared`, natively, on emscripten and with
  gcc-15. Expected: pass: guards of Tasks 2 to 8b. A failure is a defect of the task that brought
  the behaviour, fixed there, with a note in the ledger.
- [ ] **Step 3: Commit** `"trystero: crossed connections recover and floods stay in their room"`
  (`test/shared_test.cpp` and whatever the fix touched).

### Task 12: A passwordless room is open to the app's other rooms; increment 2's gates

**Files:**
- Test: `libs/trystero/test/shared_test.cpp` (S:6.2 case 19, both its forms)

- [ ] **Step 1: Write the test** (ruling 7: the behaviour as Trystero JS has it). A is the lower
  id, the initiator in `vault`, so that `wrong_password` is A's to report (conflict 15):

```cpp
// Case 19: B, in lobby only, sends A presence(1, T) and an @_hsready for each token A advertised:
// A's passwordless secret reports B joined and sends B its broadcasts; A's vault, with a password,
// never does, and reports the handshake's failure: handshake_timed_out 10 s after binding, or
// wrong_password when B's end sends a wrong response to A's challenge.
void a_passwordless_room_is_open_to_the_apps_other_rooms() {
    for (const bool wrong_response : {false, true}) {
        two_apps t("AAAAAAAAAAAAAAAAAAH1", "ZZZZZZZZZZZZZZZZZZH2");
        const ends e(t);
        t.a_rooms.push_back(join(t.a, "lobby"));
        t.a_rooms.push_back(join(t.a, "secret"));
        t.a_rooms.push_back(join(t.a, "vault", {.password = "pw"}));
        t.b_rooms.push_back(join(t.b, "lobby"));
        webcpp::test::require(BOOST_TEST(run_until(t.f, [&] {
            return t.a_rooms[0].peers().size() == 1 && t.b_rooms[0].peers().size() == 1;
        })));
        std::size_t secret_broadcasts = 0;
        t.f.net.filter = [&](std::size_t from, std::size_t, std::span<const std::uint8_t> frame) {
            secret_broadcasts += from == e.a() && internal_name(frame, "secret") == "chat" ? 1 : 0;
            return trystero::test::memory_network::verdict::deliver;
        };
        int secret_joins = 0;
        t.a_rooms[1].on_peer_join([&](std::string_view) { ++secret_joins; });
        std::vector<boost::system::error_code> vault_errors;
        t.a_rooms[2].on_error([&](std::string_view, boost::system::error_code ec, std::string_view) {
            vault_errors.push_back(ec);
        });
        const double injected = t.f.net.now_ms();
        for (const auto* room : {"secret", "vault"}) {
            t.f.net.inject(e.a(), trystero::protocol::presence_frame(
                                      trystero::protocol::room_token(app_id, room), true));
            t.f.net.inject(e.a(), internal_frame(room, "@_hsready"));
        }
        if (wrong_response) {
            // After A's challenge, which A sent on binding vault at the presence above.
            t.f.net.inject(e.a(), internal_frame("vault", "@_hsdata",
                                                 R"({"__trystero_pw":"response","h":"x"})"));
        }
        BOOST_TEST(run_until(t.f, [&] { return secret_joins == 1; }, 1ms));
        t.a_rooms[1].make_action<std::string>("chat").async_send("to all",
                                                                 [](boost::system::error_code) {});
        BOOST_TEST(run_until(t.f, [&] { return secret_broadcasts == 1; }, 1ms));
        BOOST_TEST(run_until(t.f, [&] { return !vault_errors.empty(); }, 11s));
        if (wrong_response) {
            BOOST_TEST(vault_errors.at(0) == trystero::error::wrong_password);
            BOOST_TEST_EQ(t.f.net.now_ms(), injected);
        } else {
            BOOST_TEST(vault_errors.at(0) == trystero::error::handshake_timed_out);
            BOOST_TEST_EQ(t.f.net.now_ms() - injected, 10'000.0);
        }
        BOOST_TEST(t.a_rooms[2].peers().empty());
    }
}
```

  `peers()` lists active peers only, as today, so `vault`'s stays empty while its handshake runs.
- [ ] **Step 2: Run it.** Expected: pass, a guard of Tasks 6 and 10 (ruling 7 keeps Trystero's
  behaviour; no code of this task changes it).
- [ ] **Step 3: Increment 2's gates.** From scratch: the native and emscripten lanes (both run
  `shared` and `shared_no_exceptions`, the native one `shared_hardened` too), the gcc-15 lane, the
  sanitizer run, the oracle, both interop lanes (the 19 native and 16 browser scenarios), the
  reference documentation (`$B2 libs/trystero/doc`, MrDocs strict, so that a comment naming a
  removed symbol, `room_config` in `handler.hpp:44` among them, fails here and not at Task 15;
  review M17) and the lint (Global Constraints). Expected: green, no sanitizer report,
  `lint: clean`. Record each in the ledger.
- [ ] **Step 4: Commit** `"trystero: a passwordless room open to the app's other rooms, as Trystero's"`
  (`test/shared_test.cpp`, and the comments the documentation build named).

---

## Increment 3: allocation

### Task 13: `room_options::memory`, the payload copied once, and the counting test

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/options.hpp` (`room_options::memory`)
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`room_impl::make` uses it;
  `payload_traits<pmr_bytes>::decode` copies once), `.../app.hpp`
- Test: `libs/trystero/test/allocation_test.cpp` (S:6.3), `libs/trystero/test/shared_test.cpp`
  (S:6.2 case 21; Review Focus 1)

**Interfaces:**
- Consumes: Task 5c's `app_config_in` and `options_in`; Task 8a's `poll_until`; Task 1's virtual
  timers, which leave the clock when destroyed, so `r2`'s resource is empty once `r2` is (review N1).
- Produces: `std::pmr::memory_resource* room_options::memory = nullptr;` ("the resource every
  allocation of the room goes through: its state, bindings, negotiations, relays, actions, frames it
  writes, transfers and JSON; null means the app's; it must outlive the room and every operation the
  room leaves pending").

- [ ] **Step 1: Write the failing tests.** `allocation_test.cpp`'s `counting_resource`
  (`allocation_test.cpp:320-350`) gains a count of the allocations of one range of sizes, so that
  a payload's copies can be told from everything else by a size nothing else has:

```cpp
    /** Allocations of a size in [sized_from, sized_to) are also counted in `sized`. */
    std::size_t sized_from = 0;
    std::size_t sized_to = 0;
    std::size_t sized = 0;
```

  (`do_allocate` adds `sized += bytes >= sized_from && bytes < sized_to ? 1 : 0;`). Then a case
  after
  `a_room_allocates_through_its_resource_from_join_to_destruction`:

```cpp
/**
 * Two apps, each with two rooms, one room in the app's resource and one in a resource of its own,
 * exchange binary payloads: from construction until the io_context drains, operator new sees only
 * the rooms' JSON storage counters and the C library nothing. A payload received by the room with
 * its own resource is copied into that resource once, the received frame staying the app's; by the
 * room in the app's resource, the payload taken out of the frame is moved into the value (doc:
 * #trystero-allocation).
 */
void rooms_of_one_app_allocate_through_their_resources() {
    counting_resource network_memory;
    counting_resource a_memory;
    counting_resource a_room_memory;
    counting_resource z_memory;
    counting_resource z_room_memory;
    std::size_t lines = 0;
    // Every part of the configurations in the counting resources, before the window (review I11).
    auto a_config = app_config_in(a_memory, "AAAAAAAAAAAAAAAAAAAA", "wss://relay.test", lines);
    auto z_config = app_config_in(z_memory, "zzzzzzzzzzzzzzzzzzzz", "wss://relay.test", lines);
    auto a_own = options_in(a_room_memory);
    a_own.memory = &a_room_memory;
    auto z_own = options_in(z_room_memory);
    z_own.memory = &z_room_memory;
    // An inline payload of a size no other allocation has: its frame is at most 256 bytes longer.
    constexpr std::size_t payload_size = 12'345;
    const trystero::pmr_bytes to_shared(payload_size, std::uint8_t{1}, &a_memory);
    const trystero::pmr_bytes to_own(payload_size, std::uint8_t{2}, &a_room_memory);
    {
        boost::asio::io_context io;  // after the resources: destroyed before them
        trystero::test::memory_network network{io, 1, &network_memory};
        network.relay("wss://relay.test");
        warm(io);
        counting = true;
        {
            trystero::app a(io.get_executor(), network, std::move(a_config));
            trystero::app z(io.get_executor(), network, std::move(z_config));
            auto a1 = std::move(*a.join("shared", options_in(a_memory)));
            auto a2 = std::move(*a.join("own", std::move(a_own)));
            auto z1 = std::move(*z.join("shared", options_in(z_memory)));
            auto z2 = std::move(*z.join("own", std::move(z_own)));
            std::size_t got = 0;
            auto in_shared = z1.make_action<trystero::pmr_bytes>("b");
            auto in_own = z2.make_action<trystero::pmr_bytes>("b");
            in_shared.on_message([&](const trystero::pmr_bytes& data, const trystero::message_info&) {
                got += data.get_allocator().resource() == &z_memory ? 1 : 0;
            });
            in_own.on_message([&](const trystero::pmr_bytes& data, const trystero::message_info&) {
                got += data.get_allocator().resource() == &z_room_memory ? 1 : 0;
            });
            BOOST_TEST(run_until(io, [&] {
                return a1.peers().size() == 1 && a2.peers().size() == 1 &&
                       z1.peers().size() == 1 && z2.peers().size() == 1;
            }));
            for (auto* m : {&z_memory, &z_room_memory}) {
                m->sized_from = payload_size;
                m->sized_to = payload_size + 256;
            }
            a2.make_action<trystero::pmr_bytes>("b").async_send(
                trystero::pmr_bytes(to_own, &a_room_memory),
                boost::asio::bind_allocator(std::pmr::polymorphic_allocator<>(&a_room_memory),
                                            [](boost::system::error_code) {}));
            BOOST_TEST(run_until(io, [&] { return got == 1; }));
            BOOST_TEST_EQ(z_room_memory.sized, 1U);  // copied once, out of the frame
            BOOST_TEST_EQ(z_memory.sized, 1U);       // the received frame, and nothing more
            z_memory.sized = 0;
            a1.make_action<trystero::pmr_bytes>("b").async_send(
                trystero::pmr_bytes(to_shared, &a_memory),
                boost::asio::bind_allocator(std::pmr::polymorphic_allocator<>(&a_memory),
                                            [](boost::system::error_code) {}));
            BOOST_TEST(run_until(io, [&] { return got == 2; }));
            BOOST_TEST_EQ(z_memory.sized, 2U);  // the frame, the payload taken out and moved on
        }
        run_for(io, 300);  // the io_context runs out of the rooms' work
    }
    counting = false;
    BOOST_TEST_EQ(global_allocations, 4U);  // the four rooms' JSON storage counters
    BOOST_TEST_EQ(c_allocations, 0U);
    for (auto* m : {&a_memory, &a_room_memory, &z_memory, &z_room_memory}) {
        BOOST_TEST_EQ(m->outstanding, 0U);
        BOOST_TEST_EQ(m->foreign, 0U);
    }
    global_allocations = 0;
    c_allocations = 0;
}
```

  (`warm`, `run_for`, `run_until`, `counting`, `global_allocations`, `c_allocations` and
  `counting_resource`'s `outstanding` and `foreign` are the test's own,
  `allocation_test.cpp:58-67,320-365,484-530`; `a_config` and the others are made before
  `counting` is set, as `config_in` was. The payload values are made before the window too and
  copied into the room's resource at the send, as an application holding them would.)

  In `shared_test.cpp`, case 21 (Review Focus 1). B's `r2` has a receiver for `big`, so the
  transfer is accepted and its chunks really wait on the drain (review I6):

```cpp
// Case 21: A's r2, in a resource of its own, sends 1 MB that waits on the full channel; A destroys
// r2 while r1 goes on: once the posts have run, r2's resource has no bytes out, and r1 still
// exchanges.
void a_rooms_own_resource_returns_to_zero() {
    counting_resource r2_memory;
    two_apps t("AAAAAAAAAAAAAAAAAAI1", "ZZZZZZZZZZZZZZZZZZI2");
    const ends e(t);
    t.a_rooms.push_back(join(t.a, "r1"));
    t.b_rooms.push_back(join(t.b, "r1"));
    std::optional<trystero::room> r2(join(t.a, "r2", {.memory = &r2_memory}));
    t.b_rooms.push_back(join(t.b, "r2"));
    webcpp::test::require(BOOST_TEST(run_until(t.f, [&] {
        return t.all_joined() && r2->peers().size() == 1;
    })));
    auto big_in = t.b_rooms[1].make_action<trystero::bytes>("big");
    big_in.on_message([](trystero::bytes, const trystero::message_info&) {});
    t.f.net.channel_capacity = 1;
    r2->make_action<trystero::bytes>("big").async_send(trystero::bytes(1'000'000, std::uint8_t{3}),
                                                       [](boost::system::error_code) {});
    webcpp::test::require(BOOST_TEST(
        poll_until(t.f, [&] { return t.f.net.buffered(e.a()) > 64 * 1024; })));
    r2.reset();
    advance(t.f, 1s);
    BOOST_TEST_EQ(r2_memory.in_use, 0U);
    std::vector<std::string> got;
    auto chat = t.b_rooms[0].make_action<std::string>("chat");
    chat.on_message([&](std::string s, const trystero::message_info&) { got.push_back(s); });
    t.a_rooms[0].make_action<std::string>("chat").async_send("after", [](boost::system::error_code) {});
    BOOST_TEST(run_until(t.f, [&] { return got.size() == 1; }));
}
```

- [ ] **Step 2: Run to see them fail.** `$B2 libs/trystero/test//allocation libs/trystero/test//shared`.
  Expected: `room_options` has no `memory` (both). With the member alone (step 3's first bullet),
  the allocation case fails at `z_memory.sized == 1` for the room with its own resource: 2
  (`decode` takes the payload out of the frame in the app's resource, then copies it into the
  room's, `room.hpp:437-441` with `incoming::take_payload`, `action_wire.hpp:79-88`).
- [ ] **Step 3: Implement.**
  - `room_impl::make(app, options, room_id)` allocates the room in
    `options.memory ? options.memory : app->memory()`, and the room uses that resource for
    everything of its own (today's `memory_`); the binding and its waiters are in it (Task 2
    already takes the room's resource); the connection and received frames stay in the app's.
  - `payload_traits<pmr_bytes>::decode` copies once when the frame's resource is not the room's:

```cpp
    /**
     * The message's payload as this type, in the room's resource: the message's buffer moved when
     * the payload starts it and the resources agree, else one copy of the payload into the room's.
     */
    static std::optional<pmr_bytes> decode(incoming& m, std::pmr::memory_resource* memory) {
        if (m.format != protocol::payload_format::binary) {
            return std::nullopt;
        }
        if (m.buffer.get_allocator().resource() != memory) {
            const auto p = m.payload();
            return pmr_bytes(p.begin(), p.end(), memory);
        }
        return pmr_bytes(std::move(m).take_payload(), memory);
    }
```

- [ ] **Step 4: Run** `allocation` and `shared` natively, on emscripten and with gcc-15,
  `shared_hardened` natively, and `shared` once under the sanitizers. Expected: pass.
- [ ] **Step 5: Increment 3's gates.** The native, emscripten and gcc-15 lanes from scratch, the
  oracle, both interop lanes, the lint. Expected: green, `lint: clean`. Record in the ledger.
- [ ] **Step 6: Commit** `"trystero: a room's own resource, the app's by default; a payload copied once"`
  (`include/webcpp/trystero/options.hpp include/webcpp/trystero/room.hpp include/webcpp/trystero/app.hpp test/allocation_test.cpp test/shared_test.cpp`).

---

## Increment 4: interop

### Task 14a: The interop peers' and the page's parameters, events and hooks; scenario 1

The review's split (I16): this task gives the C++ peers and the page what the scenarios of S:6.4
need, keeps every existing scenario green, and replaces `twoRoomsScenario` by scenario 1; Task 14b
adds the sharing scenarios, Task 14c the security and timing ones. Every piece is written out
(review I14).

**Files:**
- Modify: `libs/trystero/test/oracle/interop/peer.cpp`, `native_peer.cpp`, `js.html`, `run.mjs`

**Interfaces:**
- Produces, for `run.mjs`:
  - the C++ peers' parameters (both peers): `room3` (a third room of the app), `room3Delay` (ms
    before the third join, 0 by default), `password2` (room2's password, `password`'s by default),
    `password3` (room3's, none by default), `idle` (the app's `idle_timeout`, in ms), `handshake`
    (every room's `handshake_timeout`, in ms), `ping2` (any value: ping each peer of room2 every
    second); the events `adopted` and `ended` (from the app's log lines `connection with <peer>
    adopted` and `connection with <peer> ended: <reason>`), and every event of room2 and room3 with
    `room: 'second'` and `room: 'third'`; room2's and room3's `chat` echo what they receive, as the
    first room's does; the native peer's command `leave-room` (leaves the first room and goes on);
  - the page's parameters: `room3`, `password2`, `password3`, `idle`
    (`_test_only_sharedPeerIdleMs`, `C/strategy.mjs:44`), `handshake` (the callbacks'
    `handshakeTimeoutMs`, `C/strategy.mjs:32`), `hold-hsdata`; its functions
    `window.connectionStates()`, `window.advertisedCount()`, `window.roomFrames(roomId)`,
    `window.echoPresence()`, `window.leave2()`, `window.chat1(text)`, `window.chat2(text)`,
    `window.flood5000()`;
  - in `run.mjs`: a page's and a native peer's `leaveRoom()`; `inRoom`, `roomComplete`,
    `connectionCounts`, `crossedBound`, `overBound`, `instants`.

- [ ] **Step 1: The browser C++ peer**, `peer.cpp` (as Task 5c left it). The table of parameters
  gains:

```cpp
// | room3           | a third room of the same app, optional                           |
// | room3Delay      | the ms before joining room3, optional (0)                        |
// | password2       | room2's password, optional (password's)                          |
// | password3       | room3's password, optional (none)                                |
// | idle            | the app's idle_timeout in ms, optional (123,333)                 |
// | handshake       | every room's handshake_timeout in ms, optional (10,000)          |
// | ping2           | any value: ping each peer of room2 every second, optional        |
```

  `page`'s three members of the second room (`second`, `second_chat`, `second_file`) become two
  `extra_room`s and two timers:

```cpp
/** A further room of the app, room2 or room3, and its actions; its events carry `room: <which>`. */
struct extra_room {
    std::string which;
    std::unique_ptr<trystero::room> room;
    std::unique_ptr<trystero::action<std::string>> chat;
    std::unique_ptr<trystero::action<trystero::bytes>> file;
};
```

```cpp
    extra_room second{.which = "second", .room = {}, .chat = {}, .file = {}};
    extra_room third{.which = "third", .room = {}, .chat = {}, .file = {}};
    std::shared_ptr<trystero::backend::timer> third_delay;
    std::shared_ptr<trystero::backend::timer> pinger;
```

  Beside `param`:

```cpp
/** Milliseconds from a parameter's digits, or `fallback` when it has none. */
std::chrono::milliseconds millis(std::string_view text, std::chrono::milliseconds fallback) {
    std::int64_t value = 0;
    const char* const end = text.data() + text.size();
    const auto [at, ec] = std::from_chars(text.data(), end, value);
    return text.empty() || ec != std::errc{} || at != end ? fallback
                                                          : std::chrono::milliseconds(value);
}

/** The events the driver counts connections by, from the app's log lines
 * `connection with <peer> adopted` and `connection with <peer> ended: <reason>`. */
void record_connection(std::string_view category, std::string_view text) {
    constexpr std::string_view prefix = "connection with ";
    constexpr std::string_view adopted = " adopted";
    constexpr std::string_view ended = " ended: ";
    if (category != "webrtc" || !text.starts_with(prefix)) {
        return;
    }
    const auto rest = text.substr(prefix.size());
    if (rest.ends_with(adopted)) {
        record("adopted", {{"peer", rest.substr(0, rest.size() - adopted.size())}});
    } else if (const auto at = rest.find(ended); at != std::string_view::npos) {
        record("ended", {{"peer", rest.substr(0, at)}, {"reason", rest.substr(at + ended.size())}});
    }
}
```

  (`<charconv>`, `<chrono>`, `<cstdint>` and `<system_error>` join the includes.) `log` calls
  `record_connection(category, text);` first. `config_of_page()` (Task 5c) sets
  `config.idle_timeout = millis(param("idle"), std::chrono::milliseconds(123'333));`, and
  `options_of_page` takes the room's password:

```cpp
/** A room's options: `password`, and the page's handshake timeout. */
trystero::room_options options_of_page(std::string_view password) {
    return {.password = std::pmr::string(password),
            .handshake_timeout = millis(param("handshake"), std::chrono::milliseconds(10'000))};
}
```

  `join_second()` becomes `join_extra`, for room2 and room3, whose chat echoes:

```cpp
/**
 * Joins `room_id`, a further room of the app, as `extra`: it sends each peer that joins a chat
 * message and a 100 KB file, echoes each chat message it receives, and records what it sees with
 * `room: <which>`.
 */
void join_extra(extra_room& extra, std::string_view room_id, std::string_view password) {
    page& app = the_page();
    auto joined = app.app->join(room_id, options_of_page(password));
    if (!joined) {
        record("error", {{"room", extra.which}, {"peer", ""}, {"error", joined.error().message()},
                         {"what", "join"}});
        return;
    }
    extra.room = std::make_unique<trystero::room>(std::move(*joined));
    extra.chat = std::make_unique<trystero::action<std::string>>(
        extra.room->make_action<std::string>("chat"));
    extra.file = std::make_unique<trystero::action<trystero::bytes>>(
        extra.room->make_action<trystero::bytes>("file"));
    record("joined", {{"room", extra.which}, {"self", extra.room->self_id()}});
    extra.room->on_peer_join([&extra](std::string_view peer_view) {
        const trystero::peer_id peer(peer_view);
        record("peer-join", {{"room", extra.which}, {"peer", peer}});
        extra.chat->async_send("hello from C++",
                               trystero::send_options{.targets = {peer}, .metadata = {}},
                               [](boost::system::error_code) {});
        trystero::bytes big(100'000);
        for (std::size_t i = 0; i < big.size(); ++i) {
            big[i] = static_cast<std::uint8_t>((i * 13) % 256);
        }
        extra.file->async_send(std::move(big),
                               trystero::send_options{.targets = {peer}, .metadata = {}},
                               [](boost::system::error_code) {});
    });
    extra.room->on_peer_leave([&extra](std::string_view peer) {
        record("peer-leave", {{"room", extra.which}, {"peer", peer}});
    });
    extra.room->on_error([&extra](std::string_view peer, boost::system::error_code ec,
                                  std::string_view what) {
        record("error",
               {{"room", extra.which}, {"peer", peer}, {"error", ec.message()}, {"what", what}});
    });
    extra.chat->on_message([&extra](std::string text, const trystero::message_info& info) {
        record("chat", {{"room", extra.which}, {"from", info.from}, {"text", text}});
        if (!text.starts_with("echo: ")) {
            extra.chat->async_send("echo: " + text,
                                   trystero::send_options{.targets = {info.from}, .metadata = {}},
                                   [](boost::system::error_code) {});
        }
    });
    extra.file->on_message([&extra](const trystero::bytes& data,
                                    const trystero::message_info& info) {
        record("file", {
                           {"room", extra.which},
                           {"from", info.from},
                           {"size", data.size()},
                           {"checksum", checksum(data)},
                       });
    });
}

/** Pings every peer of room2, then again a second later, recording each with `room: second`. */
void ping_second() {
    page& app = the_page();
    if (app.second.room) {
        for (const auto& peer : app.second.room->peers()) {
            app.second.room->async_ping(
                peer, [peer = trystero::peer_id(peer)](boost::system::error_code ec,
                                                       std::chrono::milliseconds rtt) {
                    record("ping", {{"room", "second"},
                                    {"peer", peer},
                                    {"error", ec ? ec.message() : ""},
                                    {"ms", rtt.count()}});
                });
        }
    }
    app.pinger->start(std::chrono::seconds(1), [] { ping_second(); });
}
```

  and the end of `start()` replaces `if (!param("room2").empty()) { join_second(); }` with:

```cpp
    if (!param("room2").empty()) {
        const std::string password2 = param("password2");
        join_extra(app.second, param("room2"), password2.empty() ? param("password") : password2);
    }
    if (!param("room3").empty()) {
        app.third_delay = app.backend.make_timer(std::pmr::get_default_resource());
        app.third_delay->start(millis(param("room3Delay"), std::chrono::milliseconds(0)),
                               [] { join_extra(the_page().third, param("room3"), param("password3")); });
    }
    if (!param("ping2").empty()) {
        app.pinger = app.backend.make_timer(std::pmr::get_default_resource());
        ping_second();
    }
```

  The first room is joined with `options_of_page(param("password"))`.
- [ ] **Step 2: The native C++ peer**, `native_peer.cpp` (as Task 5c left it). The table gains the
  same seven rows as `--room3`, `--room3Delay`, `--password2`, `--password3`, `--idle`,
  `--handshake`, `--ping2`, and its header comment names the command `leave-room`. It takes
  `millis` and `record_connection` as `peer.cpp` has them (a test program of its own, as the two
  peers already repeat `record` and `checksum`); `app_config_of(args)`'s log lambda calls
  `record_connection(category, text);` before writing to `std::cerr`, and it sets
  `config.idle_timeout = millis(args["idle"], std::chrono::milliseconds(123'333));`;
  `options_of(args)` becomes:

```cpp
/** A room's options: `password`, and the driver's handshake timeout. */
trystero::room_options options_of(std::map<std::string, std::string>& args,
                                  std::string_view password) {
    return {.password = std::pmr::string(password),
            .handshake_timeout = millis(args["handshake"], std::chrono::milliseconds(10'000))};
}
```

  `second_room` becomes `extra_room`, for room2 and room3, whose chat echoes:

```cpp
/**
 * A further room of the app: it sends each peer that joins a chat message and a 100 KB file,
 * echoes each chat message, and records what it sees with `room: <which>`.
 */
struct extra_room {
    std::string which;
    trystero::room room;
    trystero::action<std::string> chat = room.make_action<std::string>("chat");
    trystero::action<trystero::bytes> file = room.make_action<trystero::bytes>("file");

    extra_room(trystero::app& app, std::string which_room, std::string_view room_id,
               trystero::room_options options)
        : which(std::move(which_room)), room(std::move(*app.join(room_id, std::move(options)))) {
        record("joined", {{"room", which}, {"self", room.self_id()}});
        room.on_peer_join([this](std::string_view peer_view) {
            const trystero::peer_id peer(peer_view);
            record("peer-join", {{"room", which}, {"peer", peer}});
            chat.async_send("hello from C++",
                            trystero::send_options{.targets = {peer}, .metadata = {}},
                            [](boost::system::error_code) {});
            trystero::bytes big(100'000);
            for (std::size_t i = 0; i < big.size(); ++i) {
                big[i] = static_cast<std::uint8_t>((i * 13) % 256);
            }
            file.async_send(std::move(big),
                            trystero::send_options{.targets = {peer}, .metadata = {}},
                            [](boost::system::error_code) {});
        });
        room.on_peer_leave([this](std::string_view peer) {
            record("peer-leave", {{"room", which}, {"peer", peer}});
        });
        room.on_error([this](std::string_view peer, boost::system::error_code ec,
                             std::string_view what) {
            record("error", {{"room", which}, {"peer", peer}, {"error", ec.message()}, {"what", what}});
        });
        chat.on_message([this](std::string text, const trystero::message_info& info) {
            record("chat", {{"room", which}, {"from", info.from}, {"text", text}});
            if (!text.starts_with("echo: ")) {
                chat.async_send("echo: " + text,
                                trystero::send_options{.targets = {info.from}, .metadata = {}},
                                [](boost::system::error_code) {});
            }
        });
        file.on_message([this](const trystero::bytes& data, const trystero::message_info& info) {
            record("file", {
                               {"room", which},
                               {"from", info.from},
                               {"size", data.size()},
                               {"checksum", checksum(data)},
                           });
        });
    }
};
```

  In `main`, after the first room's handlers:

```cpp
    const std::string password2 = args["password2"].empty() ? args["password"] : args["password2"];
    const std::unique_ptr<extra_room> second =
        args["room2"].empty()
            ? nullptr
            : std::make_unique<extra_room>(app, "second", args["room2"], options_of(args, password2));
    std::unique_ptr<extra_room> third;
    boost::asio::steady_timer third_delay(io);
    if (!args["room3"].empty()) {
        third_delay.expires_after(millis(args["room3Delay"], std::chrono::milliseconds(0)));
        third_delay.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                third = std::make_unique<extra_room>(app, "third", args["room3"],
                                                     options_of(args, args["password3"]));
            }
        });
    }
    // With --ping2, each peer of room2 is pinged every second.
    boost::asio::steady_timer pinger(io);
    std::function<void()> ping_second = [&] {
        for (const auto& peer : second->room.peers()) {
            second->room.async_ping(peer, [peer = trystero::peer_id(peer)](
                                              boost::system::error_code ec,
                                              std::chrono::milliseconds rtt) {
                record("ping", {{"room", "second"},
                                {"peer", peer},
                                {"error", ec ? ec.message() : ""},
                                {"ms", rtt.count()}});
            });
        }
        pinger.expires_after(std::chrono::seconds(1));
        pinger.async_wait([&](boost::system::error_code ec) {
            if (!ec) {
                ping_second();
            }
        });
    };
    if (second && !args["ping2"].empty()) {
        ping_second();
    }
```

  and the command reader, before its `replace` branch:

```cpp
                if (command.starts_with("leave-room")) {
                    room.async_leave([](boost::system::error_code ec) {
                        record("left", {{"room", "first"}, {"error", ec ? ec.message() : ""}});
                    });
                    read_command();
                    return;
                }
```

  (`leave-room` is tested before `leave`, which it starts with.) The first room is joined with
  `options_of(args, args["password"])`, and `replace` with the same.
- [ ] **Step 3: The page**, `js.html`. Its `textMessages` block (`js.html:62-74`, which declares
  `const channels = []` at line 66) is replaced by the following, which declares `channels` once
  and keeps `text-messages` working (review I14: a second `const channels` is a `SyntaxError`
  that stops the page):

```js
// Every data channel of the page's connections, the ones it opens (createDataChannel) and the ones it accepts (the
// datachannel event, below), and the room tokens the remote peers advertised on them: presence frames,
// [2, present, length (uint16 BE), token], as Trystero's wrapRoomPresenceFrame writes them. With `text-messages`,
// the open ones also carry the text messages of that scenario.
const textMessages = params.get('text-messages')
const channels = []
const advertised = new Set()
const receivedRoomFrames = new Map()
const encoder = new TextEncoder()
const decoder = new TextDecoder()
const watchChannel = channel => {
    channels.push(channel)
    channel.addEventListener('message', ({data}) => {
        if (!(data instanceof ArrayBuffer)) return
        const b = new Uint8Array(data)
        if (b.length < 3) return
        if (b[0] === 1) {
            const token = decoder.decode(b.subarray(3, 3 + ((b[1] << 8) | b[2])))
            receivedRoomFrames.set(token, (receivedRoomFrames.get(token) ?? 0) + 1)
            return
        }
        if (b.length < 4 || b[0] !== 2) return
        const token = decoder.decode(b.subarray(4, 4 + ((b[2] << 8) | b[3])))
        if (b[1] === 1) advertised.add(token)
        else advertised.delete(token)
    })
}
const create = RTCPeerConnection.prototype.createDataChannel
RTCPeerConnection.prototype.createDataChannel = function (...args) {
    const channel = create.apply(this, args)
    watchChannel(channel)
    return channel
}
```

  The `RTCPeerConnection` wrapper (`js.html:53-61`) watches the channels it accepts:

```js
window.RTCPeerConnection = class extends PeerConnection {
    constructor(...args) {
        super(...args)
        connections.push(this)
        this.addEventListener('datachannel', ({channel}) => watchChannel(channel))
    }
}
```

  After the block above, before `checksum` (`encoder` and `decoder` must exist when `hold-hsdata`
  computes its token at once), the options every `joinRoom` takes:

```js
// `idle` and `handshake` reach every room: Trystero's test-only idle time of a shared connection
// (C/strategy.mjs:44), and its handshake timeout, a callbacks option (C/strategy.mjs:32).
const idle = params.get('idle')
const handshake = params.get('handshake')
const configOf = password => ({
    appId: params.get('app'),
    // No ICE server: Chrome gathers its host candidates alone, and asks no public STUN server.
    rtcConfig: {iceServers: []},
    ...(password ? {password} : {}),
    ...(relays ? {relayConfig: {urls: relays.split(',')}} : {}),
    ...(idle ? {_test_only_sharedPeerIdleMs: Number(idle)} : {}),
})
const callbacksOf = which => ({
    onJoinError: details => record('error', {...(which ? {room: which} : {}), peer: details.peerId, error: details.error}),
    ...(handshake ? {handshakeTimeoutMs: Number(handshake)} : {}),
})

// The token that names a room in a data channel's envelope: the hexadecimal SHA-256 of "Trystero:<app>:<room>".
const tokenOf = async roomId => Array.from(
    new Uint8Array(await crypto.subtle.digest('SHA-256', encoder.encode(`Trystero:${params.get('app')}:${roomId}`))),
    b => b.toString(16).padStart(2, '0')).join('')

// With `hold-hsdata`, the page's first @_hsdata room frame of room2 is held for 12 s, with every later frame of that
// channel behind it, as a data channel delivers in order (S:6.4 scenario 9). An action frame starts with the version
// (2), the kind and format byte, and the name, NUL-padded to 32 bytes (P/include/webcpp/trystero/protocol/
// action_wire.hpp:142-152), after the envelope's 3 bytes and token.
if (params.get('hold-hsdata')) {
    const token2 = await tokenOf(params.get('room2'))
    const channelSend = RTCDataChannel.prototype.send
    const bytesOf = data => data instanceof ArrayBuffer ? new Uint8Array(data)
        : ArrayBuffer.isView(data) ? new Uint8Array(data.buffer, data.byteOffset, data.byteLength) : null
    const isHsdata = data => {
        const b = bytesOf(data)
        if (!b || b.length < 3 || b[0] !== 1) return false
        const n = (b[1] << 8) | b[2]
        if (decoder.decode(b.subarray(3, 3 + n)) !== token2) return false
        return decoder.decode(b.subarray(3 + n + 2, 3 + n + 34)).replace(/\0+$/, '') === '@_hsdata'
    }
    let held = null
    RTCDataChannel.prototype.send = function (data) {
        if (held?.channel === this) return void held.queue.push(data)
        if (held === null && isHsdata(data)) {
            held = {channel: this, queue: [data]}
            record('held', {what: '@_hsdata'})
            setTimeout(() => {
                const {channel, queue} = held
                held = {channel: null, queue: []}
                for (const d of queue) channelSend.call(channel, d)
                record('released', {count: queue.length})
            }, 12_000)
            return
        }
        channelSend.call(this, data)
    }
}
```

  `join()` passes `configOf(params.get('password'))` and `callbacksOf(null)` in place of its two
  literal objects, and keeps its first room's chat action in a module-level `let firstChat`
  (`firstChat = chat` after `const chat = room.makeAction('chat')`). The `room2` block
  (`js.html:136-162`) is replaced by:

```js
// With `room2` and `room3`, this page also joins those rooms of the same app, as one Trystero JS page does: Trystero
// keeps one connection per app and peer id, which the rooms share. Their events carry `room: 'second'` and `'third'`.
const joinExtra = (which, roomId, password) => {
    const extra = joinRoom(configOf(password), roomId, callbacksOf(which))
    const chat = extra.makeAction('chat')
    const file = extra.makeAction('file')
    chat.onMessage = (text, {peerId}) => record('chat', {room: which, from: peerId, text})
    file.onMessage = (data, {peerId}) => {
        const bytes = new Uint8Array(data)
        record('file', {room: which, from: peerId, size: bytes.length, checksum: checksum(bytes)})
    }
    extra.onPeerJoin = peer => {
        record('peer-join', {room: which, peer})
        chat.send('hello from Trystero JS', {target: peer})
        file.send(new Uint8Array(100_000).map((_, i) => (i * 7) % 256), {target: peer})
    }
    extra.onPeerLeave = peer => record('peer-leave', {room: which, peer})
    return {room: extra, chat}
}
const second = params.get('room2')
    ? joinExtra('second', params.get('room2'), params.get('password2') ?? params.get('password'))
    : null
const third = params.get('room3') ? joinExtra('third', params.get('room3'), params.get('password3')) : null
```

  and, after `window.leave`:

```js
// The scenarios' hooks (run.mjs, S:6.4).
window.leave2 = () => second.room.leave().then(() => record('left', {room: 'second', error: ''}))
window.chat1 = text => firstChat.send(text)
window.chat2 = text => second.chat.send(text)
window.flood5000 = () => {
    for (let i = 0; i < 5000; ++i) firstChat.send('flood ' + i)
    record('flooded', {count: 5000})
}
window.connectionStates = () => connections.map(c => c.connectionState)
window.advertisedCount = () => advertised.size
window.roomFrames = async roomId => receivedRoomFrames.get(await tokenOf(roomId)) ?? 0
// Sends, on every open channel, presence(1, T) and an @_hsready room frame for every token T the remote peers
// advertised: what a peer in one room can send for the app's other rooms (ruling 7). The @_hsready frame is an inline
// action frame (version 2, kind and format 0, the name NUL-padded to 32 bytes, metadata length 0, no payload) in the
// room envelope [1, length (uint16 BE), token].
const presenceFrame = token => {
    const t = encoder.encode(token)
    return Uint8Array.of(2, 1, t.length >> 8, t.length & 0xff, ...t)
}
const hsreadyFrame = token => {
    const t = encoder.encode(token)
    const frame = new Uint8Array(3 + t.length + 36)
    frame.set([1, t.length >> 8, t.length & 0xff])
    frame.set(t, 3)
    frame.set([2, 0], 3 + t.length)
    frame.set(encoder.encode('@_hsready'), 3 + t.length + 2)
    return frame
}
window.echoPresence = () => {
    const open = channels.filter(c => c.readyState === 'open')
    for (const channel of open) {
        for (const token of advertised) {
            channel.send(presenceFrame(token))
            channel.send(hsreadyFrame(token))
        }
    }
    record('echoed', {channels: open.length, tokens: advertised.size})
    return advertised.size
}
```

  (`third` is kept so that its room lives with the page; nothing else reads it.)
- [ ] **Step 4: The driver**, `run.mjs`. `openPage`'s object gains
  `leaveRoom: () => evaluate('window.leave()'),` and `openNative`'s
  `leaveRoom: async () => child.stdin.write('leave-room\n'),`. `twoRoomsScenario`
  (`run.mjs:578-601`, its comment included) is replaced by:

```js
// ---- rooms of one app sharing one connection (S:6.4) ---------------------------------------------------------------
const inRoom = (events, which) => events.filter(e => (e.room ?? 'first') === which)
const roomComplete = (events, which, file) => {
    const mine = inRoom(events, which)
    return mine.some(e => e.kind === 'peer-join') && mine.some(e => e.kind === 'chat') &&
        mine.some(e => e.kind === 'file' && e.size === file.size && e.checksum === file.checksum)
}
// What each side did with connections: the C++ peer's made (Chromium's count in cpp.html, or the native backend's
// made-connection events) and adopted (its app's log), the page's made and connected (its RTCPeerConnection wrapper).
const connectionCounts = async (cpp, cppKind, js) => ({
    cppMade: cppKind === 'native'
        ? (await cpp.events()).filter(e => e.kind === 'made-connection').length
        : await cpp.value('window.madePeerConnections'),
    cppAdopted: (await cpp.events()).filter(e => e.kind === 'adopted').length,
    jsMade: await js.value('window.connectionStates().length'),
    jsConnected: await js.value("window.connectionStates().filter(s => s === 'connected').length"),
})
// Rooms that negotiate at once can meet the crossed race of S:2.4, where both sides lose both connections and meet
// again: past this many connections on a side the scenario fails naming the counts, so that a livelock fails by name.
const crossedBound = 4
const overBound = c => c.cppMade > crossedBound || c.jsMade > crossedBound || c.cppAdopted > crossedBound
// Watches `count()` for `ms`, noting the driver's time of each increase: the instants of a peer's reports, within
// the 250 ms of a poll.
const instants = async (count, ms) => {
    const end = Date.now() + ms
    const seen = []
    while (Date.now() < end && deaths === deathsBefore) {
        const n = await count()
        while (seen.length < n) seen.push(Date.now())
        await new Promise(r => setTimeout(r, 250))
    }
    return seen
}
// The exchanges of the first room and of room2, both ways.
const twoRoomsComplete = async (cpp, js) => {
    const c = await cpp.events(), j = await js.events()
    return exchangeComplete(inRoom(c, 'first'), false) && exchangeComplete(inRoom(j, 'first'), true) &&
        roomComplete(c, 'second', fromJs) && roomComplete(j, 'second', fromCpp)
}

// 1. Two rooms on one connection, no password: both rooms' exchanges complete both ways; at the end the page holds
// one connected RTCPeerConnection, and the C++ peer's app adopted one connection, or, when the page made more than one
// (a crossing), at most crossedBound (the plan's conflict 14).
const twoRoomsOneConnectionScenario = (name, cppKind) =>
    scenario(name, async open => {
        const params = {room: room(), room2: room(), ...relayParams}
        const cpp = await open(cppKind, params)
        const js = await open('js', params)
        const ok = await until(() => twoRoomsComplete(cpp, js), 60_000)
        const counts = await connectionCounts(cpp, cppKind, js)
        const facts = `connections ${JSON.stringify(counts)}`
        if (overBound(counts)) return `${facts}: past the bound of ${crossedBound}`
        if (!ok) return `the two rooms did not both complete in 60 s; ${facts}`
        if (counts.jsConnected !== 1) return `the page does not hold exactly one connected connection; ${facts}`
        if (counts.jsMade === 1 && counts.cppAdopted !== 1) return `no crossing, yet ${counts.cppAdopted} adoptions; ${facts}`
        return null
    })
```

  and in `scenarios()` each `twoRoomsScenario(...)` line becomes
  `await twoRoomsOneConnectionScenario('C++ and Trystero JS share one connection between two rooms', 'cpp')`
  and `await twoRoomsOneConnectionScenario('Native C++ and Trystero JS share one connection between two rooms', 'native')`.
- [ ] **Step 5: Run both interop lanes** (Global Constraints), each with its `--build-dir`.
  Expected: the browser lane's 16 scenarios and the native lane's 19 pass, scenario 1 among them;
  the oracle lane stays byte-identical.
- [ ] **Step 6: Commit** `"trystero: the interop peers and page for shared connections"`
  (`test/oracle/interop/peer.cpp test/oracle/interop/native_peer.cpp test/oracle/interop/js.html test/oracle/interop/run.mjs`).

### Task 14b: Sharing scenarios 2, 3, 5 and 6

**Files:**
- Modify: `libs/trystero/test/oracle/interop/run.mjs`

- [ ] **Step 1: Write the scenarios**, after scenario 1:

```js
// 2. Three rooms on one connection: the C++ peer joins room3 5 s after the others, over the connection already there
// (by presence): room3 completes both ways, and no side makes a connection for it.
const threeRoomsScenario = (name, cppKind) =>
    scenario(name, async open => {
        const params = {room: room(), room2: room(), room3: room(), ...relayParams}
        const cpp = await open(cppKind, {...params, room3Delay: 5000})
        const js = await open('js', params)
        const joined3 = async () => inRoom(await cpp.events(), 'third').some(e => e.kind === 'joined')
        if (!(await until(joined3, 20_000))) return 'the C++ peer did not join room3'
        const before = await connectionCounts(cpp, cppKind, js)
        const ok = await until(async () => (await twoRoomsComplete(cpp, js)) &&
            roomComplete(await cpp.events(), 'third', fromJs) && roomComplete(await js.events(), 'third', fromCpp), 60_000)
        const after = await connectionCounts(cpp, cppKind, js)
        const facts = `connections when room3 joined ${JSON.stringify(before)}, at the end ${JSON.stringify(after)}`
        if (overBound(after)) return `${facts}: past the bound of ${crossedBound}`
        if (!ok) return `the three rooms did not all complete in 60 s; ${facts}`
        if (before.jsConnected !== 1) return `the connection was not up when the C++ peer joined room3; ${facts}`
        if (after.cppMade !== before.cppMade || after.jsMade !== before.jsMade) return `room3 made a connection; ${facts}`
        return after.jsConnected === 1 ? null : `the page does not hold one connected connection; ${facts}`
    })

// 3. A room the page joined first: the page joins room and room2; 15 s later the C++ peer joins room, then room2:
// room2 joins within 5 s of the C++ peer's join of it, over the connection, by the page's presence.
const pageFirstScenario = (name, cppKind) =>
    scenario(name, async open => {
        const params = {room: room(), room2: room(), ...relayParams}
        const js = await open('js', params)
        await new Promise(r => setTimeout(r, 15_000))
        const cpp = await open(cppKind, params)
        const joined2 = async () => inRoom(await cpp.events(), 'second').some(e => e.kind === 'joined')
        if (!(await until(joined2, 10_000))) return 'the C++ peer did not join room2'
        const met = async () => inRoom(await cpp.events(), 'second').some(e => e.kind === 'peer-join') &&
            inRoom(await js.events(), 'second').some(e => e.kind === 'peer-join')
        const ok = await until(met, 5_000)
        const counts = await connectionCounts(cpp, cppKind, js)
        if (!ok) return `room2 did not join within 5 s of the C++ peer's join; connections ${JSON.stringify(counts)}`
        if (!(await until(() => twoRoomsComplete(cpp, js), 30_000))) return 'the two rooms did not complete their exchanges'
        return counts.jsConnected === 1 ? null : `connections ${JSON.stringify(counts)}`
    })

// 5. One room leaving, either side: the leaver leaves room; the other side sees peer-leave in room only; a chat sent in
// room2 afterwards crosses both ways (the C++ room2 echoes it); no connection is made.
const oneRoomLeavingScenario = (name, cppKind) =>
    scenario(name, async open => {
        for (const leaver of ['cpp', 'js']) {
            const params = {room: room(), room2: room(), ...relayParams}
            const cpp = await open(cppKind, params)
            const js = await open('js', params)
            if (!(await until(() => twoRoomsComplete(cpp, js), 60_000))) return `${leaver}: the two rooms did not complete in 60 s`
            const before = await connectionCounts(cpp, cppKind, js)
            const [goes, stays] = leaver === 'cpp' ? [cpp, js] : [js, cpp]
            await goes.leaveRoom()
            const seen = async () => inRoom(await stays.events(), 'first').some(e => e.kind === 'peer-leave')
            if (!(await until(seen, 10_000))) return `${leaver} left room, and the other side saw no peer-leave in it`
            const text = `after ${leaver} left`
            await js.value(`window.chat2(${JSON.stringify(text)})`)
            const crossed = async () =>
                inRoom(await cpp.events(), 'second').some(e => e.kind === 'chat' && e.text === text) &&
                inRoom(await js.events(), 'second').some(e => e.kind === 'chat' && e.text === `echo: ${text}`)
            const ok = await until(crossed, 10_000)
            const after = await connectionCounts(cpp, cppKind, js)
            const facts = `connections before ${JSON.stringify(before)}, after ${JSON.stringify(after)}`
            if (!ok) return `${leaver}: a chat in room2 did not cross both ways after the leave; ${facts}`
            const second = [...inRoom(await cpp.events(), 'second'), ...inRoom(await js.events(), 'second')]
            if (second.some(e => e.kind === 'peer-leave')) return `${leaver}: room2 saw a peer leave; ${facts}`
            if (after.cppMade !== before.cppMade || after.jsMade !== before.jsMade) return `${leaver}: a connection was made; ${facts}`
        }
        return null
    })

// 6. A peer leaving every room: both sides are given an idle time of 2 s; the page leaves both rooms: the C++ peer
// records both leaves, and the connection closes on both sides after the idle time, not before.
const leavingEveryRoomScenario = (name, cppKind) =>
    scenario(name, async open => {
        const params = {room: room(), room2: room(), idle: 2000, ...relayParams}
        const cpp = await open(cppKind, params)
        const js = await open('js', params)
        if (!(await until(() => twoRoomsComplete(cpp, js), 60_000))) return 'the two rooms did not complete in 60 s'
        await js.value('window.leave()')
        await js.value('window.leave2()')
        const bothLeft = async () => {
            const c = await cpp.events()
            return inRoom(c, 'first').some(e => e.kind === 'peer-leave') && inRoom(c, 'second').some(e => e.kind === 'peer-leave')
        }
        if (!(await until(bothLeft, 10_000))) return 'the C++ peer did not see the page leave both rooms'
        const leftAt = Date.now()
        const closed = async () =>
            (await js.value("window.connectionStates().filter(s => s === 'connected').length")) === 0 &&
            (await cpp.events()).some(e => e.kind === 'ended')
        if (!(await until(closed, 10_000))) return 'the connection did not close on both sides within 10 s of the leaves'
        const seconds = (Date.now() - leftAt) / 1000
        return seconds >= 1.5 ? null : `the connection closed ${seconds} s after the leaves, before the idle time of 2 s`
    })
```

  and in `scenarios()`, after each scenario 1 line, for `'cpp'` and for `'native'` (names with
  "Native C++" for the native peer):

```js
            await threeRoomsScenario('Three rooms share one connection, the third joined later by presence', 'cpp')
            await pageFirstScenario('A room the page joined first binds over the connection within 5 s', 'cpp')
            await oneRoomLeavingScenario('One room leaving, either side, leaves the other room and the connection', 'cpp')
            await leavingEveryRoomScenario('A peer leaving every room closes the connection after the idle time', 'cpp')
```

  (The 1.5 s of scenario 6 is the idle time less the driver's polling and the two pages'
  timer slack; the spec's "not before" is about the idle time, which the unit case 9 measures to
  the millisecond on the virtual clock.)
- [ ] **Step 2: Run both interop lanes.** Expected: every scenario passes (browser 20, native 23).
- [ ] **Step 3: Commit** `"trystero: interop of rooms sharing a connection, joining and leaving"`
  (`test/oracle/interop/run.mjs`).

### Task 14c: Security and timing scenarios 4, 7, 8 and 9

**Files:**
- Modify: `libs/trystero/test/oracle/interop/run.mjs`

- [ ] **Step 1: Write the scenarios**, after scenario 6:

```js
// 4. A passwordless room is open to the app's other rooms (ruling 7): the C++ peer joins room, a passwordless room2
// and room3 with a password; the page joins room only, then sends presence and @_hsready for every token the C++ peer
// advertised (window.echoPresence): the C++ peer's room2 reports the page joined and sends it its messages; its room3
// never reports it joined.
const openToOtherRoomsScenario = (name, cppKind) =>
    scenario(name, async open => {
        const rooms = {room: room(), room2: room(), room3: room()}
        const cpp = await open(cppKind, {...rooms, password3: 'vault', ...relayParams})
        const js = await open('js', {room: rooms.room, ...relayParams})
        const ok = await until(async () => exchangeComplete(inRoom(await cpp.events(), 'first'), false) &&
            exchangeComplete(inRoom(await js.events(), 'first'), true), 30_000)
        if (!ok) return 'room did not complete in 30 s'
        if (!(await until(async () => (await js.value('window.advertisedCount()')) >= 3, 10_000))) {
            return `the C++ peer advertised ${await js.value('window.advertisedCount()')} tokens, not 3`
        }
        const echoed = await js.value('window.echoPresence()')
        const joined2 = async () => inRoom(await cpp.events(), 'second').some(e => e.kind === 'peer-join')
        if (!(await until(joined2, 15_000))) return `the C++ peer's room2 did not report the page joined (${echoed} tokens echoed)`
        const sent2 = async () => (await js.value(`window.roomFrames(${JSON.stringify(rooms.room2)})`)) > 0
        if (!(await until(sent2, 10_000))) return "the C++ peer's room2 sent the page nothing"
        await new Promise(r => setTimeout(r, 15_000))
        return inRoom(await cpp.events(), 'third').some(e => e.kind === 'peer-join')
            ? "the C++ peer's room3, with a password, reported the page joined"
            : null
    })

// 7. A wrong password in room2, in both id orders (ruling 9). With the C++ peer the lower id, it reports
// wrong_password in room2 3 times in 60 s, at about 0, 11 and 33 s from the first. With it the higher id, the page
// records Trystero's password error at most 4 times in 60 s, and the C++ peer reports nothing in room2. In both, room
// keeps exchanging, and no connection is made.
const wrongPasswordScenario = (name, cppKind) =>
    scenario(name, async open => {
        const wrong = 'the peer uses another room password'
        for (const cppSelf of ['00000000000000000000', 'zzzzzzzzzzzzzzzzzzzz']) {
            const lower = cppSelf.startsWith('0')
            const params = {room: room(), room2: room(), ...relayParams}
            const cpp = await open(cppKind, {...params, self: cppSelf})
            const js = await open('js', {...params, password2: 'another'})
            const cppReports = async () => inRoom(await cpp.events(), 'second').filter(e => e.kind === 'error').length
            const cppWrong = async () => inRoom(await cpp.events(), 'second').filter(e => e.kind === 'error' && e.error === wrong).length
            const jsReports = async () => inRoom(await js.events(), 'second').filter(e => e.kind === 'error').length
            // Watched from the start, so that the first report, which can come before room completes, has its time.
            const watching = instants(lower ? cppWrong : jsReports, 60_000)
            const ok = await until(async () => exchangeComplete(inRoom(await cpp.events(), 'first'), false) &&
                exchangeComplete(inRoom(await js.events(), 'first'), true), 30_000)
            if (!ok) return `${cppSelf}: room did not complete in 30 s`
            const before = await connectionCounts(cpp, cppKind, js)
            const seen = await watching
            const offsets = seen.map(t => (t - seen[0]) / 1000)
            if (lower) {
                const expected = [0, 11, 33]
                const near = offsets.length === 3 && offsets.every((s, i) => Math.abs(s - expected[i]) <= 3)
                if (!near) return `the C++ peer, the lower id, reported wrong_password at ${offsets.join(', ')} s, not about 0, 11 and 33`
            } else {
                if (seen.length < 1 || seen.length > 4) return `the page reported ${seen.length} password errors in 60 s, not 1 to 4`
                if ((await cppReports()) !== 0) return 'the C++ peer, the higher id, reported in room2'
            }
            const text = `still here ${cppSelf}`
            await js.value(`window.chat1(${JSON.stringify(text)})`)
            const echoed = async () => inRoom(await js.events(), 'first').some(e => e.kind === 'chat' && e.text === `echo: ${text}`)
            if (!(await until(echoed, 10_000))) return `${cppSelf}: room stopped exchanging`
            const after = await connectionCounts(cpp, cppKind, js)
            if (after.cppMade !== before.cppMade || after.jsMade !== before.jsMade) {
                return `${cppSelf}: a connection was made: before ${JSON.stringify(before)}, after ${JSON.stringify(after)}`
            }
        }
        return null
    })

// 8. A flood in one room: the page sends 5,000 messages on room's chat while the C++ peer pings room2 every second:
// every ping completes within 2 s, and the C++ peer stays joined in both rooms.
const roomFloodScenario = (name, cppKind) =>
    scenario(name, async open => {
        const params = {room: room(), room2: room(), ...relayParams}
        const cpp = await open(cppKind, {...params, ping2: 1})
        const js = await open('js', params)
        if (!(await until(() => twoRoomsComplete(cpp, js), 60_000))) return 'the two rooms did not complete in 60 s'
        const pingsOf = async () => inRoom(await cpp.events(), 'second').filter(e => e.kind === 'ping')
        const before = (await pingsOf()).length
        await js.value('window.flood5000()')
        await new Promise(r => setTimeout(r, 10_000))
        const pings = (await pingsOf()).slice(before)
        if (pings.length < 5) return `only ${pings.length} pings of room2 completed in the 10 s after the flood began`
        const slow = pings.filter(e => e.error !== '' || e.ms > 2000)
        if (slow.length) return `pings of room2 failed or took over 2 s during the flood: ${JSON.stringify(slow)}`
        return (await cpp.events()).some(e => e.kind === 'peer-leave') ? 'the C++ peer saw the page leave a room' : null
    })

// 9. A transient handshake failure with Trystero JS, in both id orders (ruling 9): room2 with a password; the page
// holds its first @_hsdata of room2 for 12 s (hold-hsdata) and waits 30 s for a handshake, so that the C++ peer, with
// its 10 s, times out first. With the page the higher id its held answer is accepted; with the page the lower id its
// held challenge is dropped in the C++ peer's window. Both are joined within 15 s of the C++ peer's kick, or, when the
// page bound by reuse inside that window (the first residual case of S:4.2), within 50 s, the page having reported
// one handshake timeout. No password error on either side, and room keeps exchanging.
const transientHandshakeScenario = (name, cppKind) =>
    scenario(name, async open => {
        for (const cppSelf of ['00000000000000000000', 'zzzzzzzzzzzzzzzzzzzz']) {
            const pageLower = !cppSelf.startsWith('0')
            const params = {room: room(), room2: room(), password2: 'p', ...relayParams}
            const cpp = await open(cppKind, {...params, self: cppSelf})
            const js = await open('js', {...params, 'hold-hsdata': '1', handshake: 30000})
            const cppTimeouts = async () => inRoom(await cpp.events(), 'second')
                .filter(e => e.kind === 'error' && e.error === 'the handshake timed out').length
            if (!(await until(async () => (await cppTimeouts()) >= 1, 30_000))) return `${cppSelf}: the C++ peer did not time out in room2`
            const met = async () => inRoom(await cpp.events(), 'second').some(e => e.kind === 'peer-join') &&
                inRoom(await js.events(), 'second').some(e => e.kind === 'peer-join')
            let ok = await until(met, 15_000)
            const pageTimeouts = async () => inRoom(await js.events(), 'second')
                .filter(e => e.kind === 'error' && String(e.error).includes('timed out')).length
            if (!ok && pageLower) ok = (await until(met, 35_000)) && (await pageTimeouts()) === 1
            if (!ok) return `${cppSelf}: room2 did not join in time; the page reported ${await pageTimeouts()} timeouts`
            if ((await cppTimeouts()) !== 1) return `${cppSelf}: the C++ peer timed out ${await cppTimeouts()} times`
            const passwordErrors = [...inRoom(await cpp.events(), 'second'), ...inRoom(await js.events(), 'second')]
                .filter(e => e.kind === 'error' && String(e.error).includes('password'))
            if (passwordErrors.length) return `${cppSelf}: password errors ${JSON.stringify(passwordErrors)}`
            const text = `still here ${cppSelf}`
            await js.value(`window.chat1(${JSON.stringify(text)})`)
            const echoed = async () => inRoom(await js.events(), 'first').some(e => e.kind === 'chat' && e.text === `echo: ${text}`)
            if (!(await until(echoed, 10_000))) return `${cppSelf}: room stopped exchanging`
        }
        return null
    })
```

  and in `scenarios()`, after scenario 6's lines, for `'cpp'` and for `'native'`:

```js
            await openToOtherRoomsScenario('A passwordless room is open to the app\'s other rooms, a password\'s is not', 'cpp')
            await wrongPasswordScenario('A wrong password in one room, in both id orders, paced by the rebinding times', 'cpp')
            await roomFloodScenario('A flood in one room does not delay the pings of another', 'cpp')
            await transientHandshakeScenario('A transient handshake failure converges, in both id orders', 'cpp')
```

- [ ] **Step 2: Run both interop lanes.** Expected: every scenario passes (browser 24, native
  27), each printed with its time; scenario 7 runs about 2 minutes per lane (60 s per id order).
- [ ] **Step 3: Increment 4's gates.** The native, emscripten and gcc-15 lanes, the oracle, both
  interop lanes, the lint, from scratch. Expected: green, `lint: clean`. Record in the ledger.
- [ ] **Step 4: Commit** `"trystero: interop of a shared connection's security and timing"`
  (`test/oracle/interop/run.mjs`).

---

## Increment 5: the pages

### Task 15: The page, the rulebook and the comments

**Files:**
- Modify: `libs/trystero/doc/differences.adoc`, `roadmap.adoc`, `usage.adoc`, `protocol.adoc`,
  `design.adoc`, `allocation.adoc`, `guarantees.adoc`, `testing.adoc`, `overview.adoc`,
  `quick-start.adoc`, `browser.adoc`, `native.adoc`
- Modify: `libs/trystero/README.md`, `libs/trystero/AGENTS.md`
- Modify: the header comments S:7 names (the documentation build of Task 12's gate has named
  the ones MrDocs rejects; these are the rest): `handler.hpp:44`, `json_resource.hpp:29`,
  `detail/log.hpp:29,38`, `native/backend.hpp:105,135`, `browser/backend.hpp:899`,
  `native/peer_connection.hpp:251`, `error.hpp` (`invalid_configuration`)
- Modify (superproject): `AGENTS.md` (`W/:668-671`)

- [ ] **Step 1: `differences.adoc`.** Delete the row at lines 50-53. Replace the paragraph "A peer
  id per room, not per app" (lines 74-84) with a table of S:5 in AsciiDoc, its rows as the spec
  states them (scope of sharing and the self id; reuse; rebinding; announcements; presence after
  binding; an answer to an earlier challenge; frames after the higher id's kick; joining a room
  already joined; rooms per app; a connection whose last room was kicked; relay sockets; relay and
  ICE configuration; `idle_timeout`; frames waiting for a handler; media; passive rooms; crossed
  duplicate connections), each with its "Why".
- [ ] **Step 2: `roadmap.adoc`.** Item 2 loses ", one connection shared between several rooms of an
  app".
- [ ] **Step 3: `usage.adoc`.** "Joining" becomes:

```asciidoc
[#trystero-usage-joining]
=== Joining

An `app` is this program in one application: one peer id, and one connection to each remote
peer, which every room of the app it shares with that peer uses, as a Trystero JS page shares
them. Constructing it takes the executor every handler and completion runs on, the backend, which
must outlive it, and its configuration, `app_config`, moved. A room is joined with `join`, which
returns it, or an error: `error::already_joined` while a room of that id is joined in the app, and
`error::too_many_rooms` when 64 rooms count, since a peer drops a connection that advertises more.
```

  followed by the tables of `app_config` (S:4.5, with the `self_id` row: "one id for every room of
  the app; rooms that must not be linked to one program use an `app` each, S:4.12") and
  `room_options`, then a subsection:

```asciidoc
[#trystero-usage-migration]
==== Moving from `room_config`

`room(executor, backend, room_config, room_id)` and `room_config` are gone: a room is joined
through an `app`.
```

  with the migration table of S:4.5 (each row of it), and the "Peers" paragraph gains the sentences
  of S:4.2 and S:7: what a kick does over a shared connection, when the peer is bound again, and
  that over a shared connection only the lower id reports a wrong password.
- [ ] **Step 4: `protocol.adoc`.** "The data channel" (lines 73-79) gains the routing table of
  S:4.3 and what presence does; "Bounded signaling" (lines 114-143) gains the bounds of S:4.9 and
  the rebinding times; "What remains" (lines 160-185) gains S:4.12's items: a passwordless room is
  not private from the peers of the app's other rooms, with the mechanism (presence gives its token,
  the relays its topics, the token alone binds), and a private room takes a password or an `app` of
  its own; the password exposure, accepted as Trystero JS has it; what presence reveals; the linking
  of an app's rooms by its id; a forged announcement binds no connected peer; the two residual cases
  of S:4.2 with their bounds. Its sentence that a watcher without the key cannot join
  (lines 166-176) is narrowed to "a watcher not connected to the app".
- [ ] **Step 5: `design.adoc`.** A section `[#trystero-design-sharing]` "Connections shared by the
  rooms of an app": the ownership table of S:4.1 and its three bullets; the layers' table names the
  app (`app.hpp`, `detail/app_impl.hpp`, `detail/connection.hpp`). `shared_test.cpp`'s first
  comment cites this anchor.
- [ ] **Step 6: `allocation.adoc`.** "One resource per room" becomes "The app's resource and the
  room's": S:4.8's bullets, the log's sentences in the app's resource, and the order of the
  resources' lifetimes (`app_config::memory` and `room_options::memory` declared before the
  `io_context`). The page's include of `allocation_test.cpp`'s `tag::config[]` now shows
  `app_config_in` and `options_in` (Task 5c); the sentences around it say that every part of an
  app's configuration is made in the counted resource before the window, and that a received
  binary payload is copied once into a room's own resource (Task 13).
- [ ] **Step 7: `guarantees.adoc`.** Invariant 3: "A peer with another password never joins: its
  signals do not open, and over a connection shared with another room its handshake fails." New
  invariants, numbered after the last: one connection per remote peer per app, shared by its rooms
  (held by `shared` cases 1 to 3 and the interop's first scenario); a room's departure leaves the
  app's other rooms joined (`shared` case 8, interop 5); a room with a password never admits a peer
  of another room of the app without that password (`shared` case 19, interop 4).
- [ ] **Step 8: the other pages and files.** `testing.adoc` (the `connection`, `memory_backend`,
  `shared`, `shared_no_exceptions` and `shared_hardened` tests, the memory backend's virtual
  clock and byte-accurate drain, the sanitizer run, and the interop's scenarios 1 to 9),
  `overview.adoc` (an app per program in one
  application, its resource and the rooms'), `quick-start.adoc` (the app in the browser program and
  the native one: the examples' code of Task 5c), `browser.adoc` and `native.adoc` (the backends'
  bounds count every room of a program; peer connections are made in the app's resource; a room's
  window, `native.adoc:86-90`), `README.md` (the example and the paragraph on `room_config`,
  `README.md:31,42`), `AGENTS.md` (trystero's: the interop scenarios at `AGENTS.md:205-206`, the
  allocation rules at `:261`, bounded signaling, the memory backend's hooks), the header comments
  listed in Files, and the superproject's `AGENTS.md` (`AGENTS.md:668-671`: "a
  `std::pmr::memory_resource` given in its `app_config`, and in a room's `room_options`, reaches
  every allocation").
- [ ] **Step 9: Build the page.** `$B2 libs/trystero/doc`. Expected: the page builds, MrDocs strict
  passes (`app`, `app_config`, `room_options`, the new enumerators documented), and `doc-check`
  passes. Then the lint: `lint: clean`.
- [ ] **Step 10: Commit** in the library `"trystero: the page, for an app's shared connections"`
  (every file above under `libs/trystero/`), and in the superproject, by path:

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "AGENTS.md: trystero's resources, the app's and a room's" -- AGENTS.md
```

### Task 16: Green, review and the gates

- [ ] **Step 1:** Every lane from scratch, as the CI runs them (Global Constraints): native,
  emscripten, the gcc-15 lane, the oracle, both interop lanes (browser 24 scenarios, native 27),
  the sanitizer run of `shared` and `connection`, `python3 tools/ci/matrix.py plan` and the
  trystero entries it lists (gcc-14 and gcc-15, clang with libstdc++, MSVC 14.3 and 14.5 are the
  CI's: the plan's lanes stand in for them locally, and the CI run itself is the controller's), the
  lint, the page. Expected: all green, no sanitizer report, `lint: clean`; the oracle
  byte-identical (no wire value changed). Record each in the ledger.
- [ ] **Step 2:** A whole-branch review against the spec (S:4 to S:9), this plan's Review Focus and
  its conflicts table; fix what it finds, each in its own commit.
- [ ] **Step 3:** The gates G1 and G2 (Global Constraints), by the controller with the owner's
  agreement: push and merge `libs/trystero` first, then bump the submodule in the superproject
  (`git add libs/trystero`) with the message `libs/trystero: one connection shared by the rooms of
  an app`, and push it.

---

## Spec coverage

| Spec | Task |
| --- | --- |
| S:4.1 ownership, references (a weak registry, quiet destructors) | 2, 3, 4, 8b, 13 |
| S:4.2 adoption, binding paths, conditions, order (the kept frames' guard) | 4, 6, 7 |
| S:4.2 unbinding, connection's end, app's users | 4, 8a, 8b |
| S:4.2 rebinding, backoff, dropping window, pairings | 9 |
| S:4.2 handshake across a rebinding, kept `@_leave` | 10 |
| S:4.3 routing | 2, 4 |
| S:4.4 self id | 3, 5c |
| S:4.5 API, errors, compatibility | 5b, 5c |
| S:4.6, 4.7 executors, joining twice, the farewell | 5c, 8b |
| S:4.8 memory | 3, 5c, 13 |
| S:4.9 bounds (64 rooms, 64 tokens, 64 kept frames, 8 challenges) | 2, 4, 5c, 8b, 10 |
| S:4.10 errors and exceptions | 5b, 5c, 12 (`shared_no_exceptions`) |
| S:4.11 backends | 4, 15 |
| S:4.12 security | 12 (case 19), 14c (scenario 4), 15 |
| S:5 differences | 15 |
| S:6.1 hooks, the virtual clock, the byte-accurate drain | 1 |
| S:6.2 cases 1-25 | 4 (12 and 13 for one room), 5c (1, 2, 6, 15's first half, 18), 6 (5, 12 for two rooms), 7 (3, 24), 8a (8, 9, 10's per-room half, 11), 8b (15, 16, 17), 9 (20, 10's closing kick and renegotiation), 10 (4, 22 with its eight variants and its third and fourth, 23, 25), 11 (7, 14), 12 (19, both forms), 13 (21) |
| Review Focus 1-5 | 13 (1); 6, 8a, 8b and `shared_hardened` (2); 4 (3); 8b (4); 5c (5) |
| Plan review C1 (a kept frame that kicks during a binding) | 6 (the guard and its case), 10 (kept with the R3 rule) |
| S:6.3 allocation | 5c (`app_config_in`, `options_in`), 13 |
| S:6.4 interop 1-10 | 14a (1, 10), 14b (2, 3, 5, 6), 14c (4, 7, 8, 9) |
| S:7 pages and files | 4 (`action_wire.hpp:104`), 12 (the documentation build), 15 |
| S:8 increments | 1-4; 5a-12; 13; 14a-14c; 15 |
