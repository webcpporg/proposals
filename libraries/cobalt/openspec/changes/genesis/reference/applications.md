# Pinned application inventory

Inspected 2026-11-04 at [Boost.Cobalt 1.92.0](https://github.com/boostorg/cobalt/tree/8b1cecba91d0e868ee0947b40fb04e3594680297/example).
The 12 original C++ files and three supporting files are preserved unchanged in
`source/upstream/example/`. `applications.json` records their source hashes and dependencies;
`profiles.json` defines 28 starting profiles; `application-matrix.json` contains all 336
application/profile cells. No cell claims a fresh application run.

`M` below means feasible integration or verification remains missing. `B` means a named
SDK/runtime/dependency blocker. `N` means the original semantics/mode is not applicable.
The global profile blockers and exact EH/thread variants are expanded in JSON; the compact
table describes application eligibility, not supported/tested results.

| Upstream file | Jam registration | Native | Browser/Emscripten | WASIp2 S | WASIp3 S / cooperative M-off |
| --- | --- | --- | --- | --- | --- |
| `channel.cpp` | Active | M | M | M | M |
| `delay.cpp` | Active | M | M | M | M |
| `delay_op.cpp` | Active | M | M | M | M |
| `echo_server.cpp` | Active | M EH-on; original N EH-off | N raw TCP listener; counterpart M with explicit role/transport change | M EH-on; value-error counterpart M | S EH-on M; result counterpart M where SDK permits |
| `http.cpp` | Absent | M with Asio TLS/Beast | N original guest TCP/OpenSSL; browser-transport counterpart M | M with shared TLS and fixture | M with shared TLS and fixture |
| `outcome.cpp` | Active | M | M | M | M |
| `python.cpp` | Absent | B until pinned CPython/nanobind fixture; original script NameError; EH-off original N | B matching Wasm host absent; script defect remains | B matching host absent; script defect remains | B matching host absent; script defect remains |
| `signals.cpp` | Absent | M | M, ordinary in-process Signals2 | M | M |
| `spsc.cpp` | Absent | M in B2 S and M; probe both | M in pthread profiles; S N | N needs workers; P2 M blocked | S N; cooperative M-off M |
| `thread.cpp` | Active | M in B2 S/M with EH-on; EH-off counterpart M | M with pthread lifecycle/EH-on; EH-off counterpart M; S N | N needs workers; P2 M blocked | S N; original M/EH-on SDK blocked; M-off counterpart M |
| `thread_pool.cpp` | Active | M in B2 S/M with EH-on; EH-off counterpart M | M with pthread/custom-main lifecycle; EH-off counterpart M; S N | N needs workers; P2 M blocked | S N; original M/EH-on SDK blocked; M-off counterpart M |
| `ticker.cpp` | Commented | M EH-on with controlled feed/client; EH-off counterpart M | N full native TCP server; browser client/routing counterpart M | M EH-on with controlled peers | S EH-on M; EH-off counterpart M |

Native `threading=single` does not remove Boost thread support, and the existing compiled
recipe includes `thread.cpp` for both native S and M. All three worker examples therefore
require real probes in both native profiles. Explicitly disabling Asio threads is a separate
unverified profile, not a reinterpretation of B2 S. Browser/WASI S profiles do disable threads.
Node uses the same browser-target transport contract in this integration; it cannot certify
raw browser sockets, DOM behavior or browser policy. Alternative Node-specific adapters
would receive separate identities.

## Per-example adaptation and assertions

| Example | Retained C++ logic | Required observable checks and adaptation |
| --- | --- | --- |
| channel | Producer, `channel<int>`, read/write and close | Ordered 0..3 in unchanged case; bounded parameterized page derivative for progress/cancel; close and pending-cancel outcomes |
| delay | `steady_timer`, executor, `use_op` | Valid/invalid input wrapper, real delayed completion, cancellation and restart; no promise of exact timer deadline |
| delay_op | `op`, ready/initiate and completion handler | Expired-ready and pending-initiation paths; no duplicate completion; timer cancellation |
| echo_server | Generator listener, stream sockets, `wait_group`, `with`, composed write | Controlled local TCP client, partial/empty transfer, concurrency bound, finite shutdown and pending accept/read cancellation; value-error EH-off source separately |
| http | Resolver, TLS stream, Beast HTTP via `use_op` | Deterministic verified HTTPS fixture, response assertion, DNS/connect/handshake/read failure and cancellation; browser adapter changes recorded |
| outcome | Real lazy and eager Outcome awaitables | Each produces 11; runtime assertion outside timed loops; a bounded repeated page derivative gives observable progress |
| python | CPython asyncio executor, nanobind awaitable/generator bridge | Preserve original script NameError; separately patch its three module-name references, then test real generator sequence, promise 42 and error/cancel propagation; dependency blockers stay separate |
| signals | Actual Signals2 connection, awaiter ownership and result cache | Value 42, connection cleanup, event before/after cancellation, no stale resume; no process-signal interpretation |
| spsc | Lockfree queue, atomic awaiters, producer `cobalt::thread` | Preserve original 100000001 values plus sentinel for full run; finite configurable derivative with exact sequence/count/checksum, full/empty backpressure and worker cleanup |
| thread | Concurrent request/response channels, worker coroutines and join | Correct additions, worker-thread witness, orderly channel close, cancellation and no worker leak; EH-off error-value derivative |
| thread_pool | Custom `main`, pool, strands, spawn/task work | Correct additions, pool execution and join; page host must avoid blocking its main event loop; no accidental replacement runner; pending-task issue probe |
| ticker | Subscription map/channels, JSON reader, race/join, session limits | Controlled market-feed fixture and local WebSocket clients; subscribe/update/reject/unsubscribe, backpressure, cancel and close; full listener remains unavailable in browser |

Upstream CMake scans all 12 sources, conditionally includes Python when dependencies exist
and ticker when Boost.JSON is a target, and has its own link assumptions. WebCPP's manifest
must express per-example dependencies instead of copying incidental global OpenSSL linkage
onto every example. Existing source is tested unchanged where feasible before adaptation.
If an upstream source itself is defective, preserve the failure and a clearly identified fix.

The pinned `python.py` imports `boost_cobalt_example_python` but uses
`boost_async_example_python` three times. Its unchanged script fails with `NameError` before
calling the first binding method. `python-script-check.json` records a Python 3.9.6 run with
only an empty import stub for the correctly named module. This isolates the script defect;
it does not run the C++ extension or establish Python/nanobind availability. Task 5.4 keeps
the original baseline failure and a separately hashed corrected script. Reproduce this
limited check from `reference/` without installing any dependencies:

```python
import runpy
import sys
import types

sys.modules['boost_cobalt_example_python'] = types.ModuleType('boost_cobalt_example_python')
try:
    runpy.run_path('source/upstream/example/python.py', run_name='__main__')
except NameError as error:
    assert str(error) == "name 'boost_async_example_python' is not defined"
else:
    raise AssertionError('Expected the original script module-name failure')
```

## Benchmark sources

All five upstream benchmarks are preserved and hashed: `post.cpp`, `immediate.cpp`,
`channel.cpp`, `parallel.cpp` and `monotonic.cpp`. Audit them before use: most compare
Cobalt and Asio, some conditionally add Boost.Context stackful arms, and monotonic names
private allocator internals. The pinned build does not supply a verified Wasm Context
backend. A skipped stackful arm is recorded as a dependency blocker; it is never attributed
to Cobalt coroutines or silently included in comparative totals. Benchmark bodies need
bounded input, correctness witnesses, repeated measurements and the Release harness in D8
before their output is publishable evidence.

## Browser page acceptance

Initial pages cover `delay`/`delay_op`, `channel` and `signals`; Outcome and worker/SPSC
pages expand the set under their feasible profiles. Each shows current source/adaptation,
selected runtime profile and visible pending/completed/cancelled/failed states. The UI
controls actual C++ execution. A repeated derivative must not be labeled unchanged upstream.
Browser E2E verifies input and keyboard access, start twice, cancellation while pending,
restart, responsive heartbeat, bounded logs, narrow viewport layout and cleanup on unload.
No fake progress, inaccessible cancel control or stale result from an earlier run is accepted.
