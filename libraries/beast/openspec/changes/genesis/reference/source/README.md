# webcpp::beast

Beast-compatible HTTP and WebSocket interfaces for native and WebAssembly applications.
The implementation is in progress. The initial backend reuses Boost.Beast 1.92's data
types and native/WASIp3 socket algorithms; browser WebSocket/fetch and WASI host HTTP
are planned separate adapters.

Use this repository inside the WebCPP superproject at `libs/beast`.

```sh
unset CPATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH
b2 -a libs/beast/test libs/beast/example
b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/beast/test libs/beast/example
b2 -a toolset=clang-wasip3 testing.launcher=tools/boost/asio-wasip3/run.sh libs/beast/test libs/beast/example
b2 -a toolset=emscripten libs/beast/test libs/beast/example
```

The WASIp2 and Emscripten commands currently exercise data interfaces, not network
transport. Native/WASIp3 tests additionally exercise HTTP, binary WebSocket echo,
closing handshakes, timeouts and the optional TLS SNI helper.

Build dependencies are explicit: `/webcpp/beast//beast` for messages,
`/webcpp/beast//sockets` for guest networking and `/webcpp/beast//tls` for guest TLS.
`beast.hpp` omits the optional transport/TLS headers.

See [the guide](doc/beast.adoc) for the feature matrix and commands. API reference
comes from the public headers through MrDocs. Build locally with `b2 -a libs/beast/doc`.
No release or published documentation is claimed yet.

Distributed under the [Boost Software License 1.0](LICENSE_1_0.txt).
