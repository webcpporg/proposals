# Working on beast

Read the superproject's [AGENTS.md](../../AGENTS.md) and
[CODING-GUIDELINES.md](../../CODING-GUIDELINES.md) first.

- This is a compatibility wrapper, with Boost 1.92 as its reference. Its complete
  proposal is in `proposals/libraries/beast/openspec/changes/genesis` in the organization.
- Portable message headers must not include a socket reactor. `http.hpp` contains
  data, parsers and serializers; `http/read.hpp` and `http/write.hpp` are explicit
  byte-stream operations. Their host-operation overloads are subsequent work.
- `/webcpp/beast//beast` supplies portable headers. `//sockets` selects native or
  WASIp3 guest transport; `//tls` selects source-built OpenSSL explicitly.
- Do not advertise browser WebSocket/fetch or WASI host HTTP as implemented by the
  current socket baseline. Keep planned and verified matrix entries separate.
- Socket tests use real loopback peers and five-second outer deadlines. WASIp3
  execution uses the shared network-capable launcher, never a mocked socket.
- Test both exception modes. The native/WASIp3 allocator constructor preserves
  upstream stream-state allocation; it must not claim custom stream allocation.
- The example's expected output is captured from its upstream compile, selecting
  `WEBCPP_TEST_UPSTREAM_BEAST`; do not hand-edit that output.
- Browser-native WebSocket and fetch must remain independent of guest protocol
  engines. Host-managed TLS never inherits guest OpenSSL or CA settings silently.
