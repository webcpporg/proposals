# Working on beast

Read the superproject's [AGENTS.md](../../AGENTS.md) and
[CODING-GUIDELINES.md](../../CODING-GUIDELINES.md) first.

- This is a compatibility wrapper, with Boost 1.92 as its reference. Its complete
  proposal is in `proposals/libraries/beast/openspec/changes/genesis` in the organization.
- Portable message headers must not include a socket reactor. `http.hpp` contains
  data, parsers and serializers; `http/read.hpp` and `http/write.hpp` are explicit
  byte-stream operations and browser message overloads. WASI host APIs live in
  `wasi/http.hpp` and depend on independent host bindings.
- `/webcpp/beast//beast` supplies portable headers. `//sockets` selects native or
  WASIp3 guest transport; `//tls` selects source-built OpenSSL explicitly.
- `/webcpp/beast//browser`, `//wasi-http` and `//wasi-streams` select independent
  environment adapters. Their runtime tests are separate from the socket tests.
  Keep source compatibility, application counterparts and unavailable facilities
  explicit in the matrix; compilation alone never establishes runtime parity.
- Socket tests use real loopback peers and five-second outer deadlines. WASIp3
  execution uses the shared network-capable launcher, never a mocked socket.
- Test both exception modes. The native/WASIp3 allocator constructor preserves
  upstream stream-state allocation; it must not claim custom stream allocation.
- The example's expected output is captured from its upstream compile, selecting
  `WEBCPP_TEST_UPSTREAM_BEAST`; do not hand-edit that output.
- Browser-native WebSocket and fetch must remain independent of guest protocol
  engines. Host-managed TLS never inherits guest OpenSSL or CA settings silently.
- Preserve `example/upstream` exactly at its pinned commit. The inventory checks
  every file hash before lint exempts that corpus's upstream style and notices.
  Change generated application adaptations through their generator, never by
  editing its outputs. Both the adapted sources and their generator are linted.
- TLS shutdown cancellation tests synchronize on the peer's observed close_notify.
  Cancelling a lowest layer between two internal socket operations is not proof
  that a pending shutdown read was cancelled. WebSocket close already tears down
  TLS and its underlying socket; do not call TLS shutdown again afterwards.
