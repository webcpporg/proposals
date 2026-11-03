# webcpp::beast

Beast-compatible HTTP and WebSocket interfaces for native and WebAssembly applications.
The facade reuses Boost.Beast 1.92's data types and native/WASIp3 socket algorithms.
Browser WebSocket/fetch and WASI host HTTP are separate adapters: the environment
owns their transport and TLS policy. The guide distinguishes source-compatible
programs from explicit application counterparts and records their verification.

Use this repository inside the WebCPP superproject at `libs/beast`.

```sh
unset CPATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH
b2 -a libs/beast/test libs/beast/example
b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/beast/test libs/beast/example
b2 -a toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh libs/beast/test libs/beast/example
b2 -a toolset=emscripten libs/beast/test libs/beast/example
```

Host HTTP and real-browser tests have dedicated lanes. Data tests under Node do
not establish browser transport behaviour. Native/WASIp3 socket tests use real
loopback peers; TLS tests use source-built OpenSSL and fresh local certificates.

Build dependencies are explicit: `/webcpp/beast//beast` for messages,
`/webcpp/beast//sockets` for guest networking and `/webcpp/beast//tls` for guest TLS.
Select `/webcpp/beast//browser` for browser-owned transport,
`/webcpp/beast//wasi-http` for host HTTP, or `/webcpp/beast//wasi-streams` for
explicit supplied-stream counterparts. `beast.hpp` omits optional transport/TLS
headers so constructing an HTTP message never brings in a socket reactor.

See [the guide](doc/beast.adoc) for the feature matrix and commands. API reference
comes from the public headers through MrDocs. Build locally with `b2 -a libs/beast/doc`.

Distributed under the [Boost Software License 1.0](LICENSE_1_0.txt).
