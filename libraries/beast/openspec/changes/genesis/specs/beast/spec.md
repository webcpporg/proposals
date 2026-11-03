## Purpose

webcpp::beast lets code written against Boost.Beast, in its layers (resolve, connect, the TLS handshake, the WebSocket handshake, then reads and writes), run in a browser with the same code. Native Boost.Beast is the behavioural reference; WASIp2, WASIp3 and Emscripten are the primary portability targets. Compiled with Emscripten, the same names and interfaces drive the browser's `WebSocket` and `fetch`: the layers the browser does not expose record what the browser needs, and the last one opens the connection. What the browser cannot honour fails as a value by default. It is to webcpp::asio what Boost.Beast is to Boost.Asio. These requirements revise the historical `reference/spec.md` draft with the owner's 2026-11-02 scope: all upstream example applications on WebAssembly. The library is not built; requirements are not runtime evidence. The WebSocket, Settings, Back-pressure, Lifetime and HTTP browser adaptation contracts below apply to Emscripten; native and the explicit WASI socket backend keep upstream transport semantics. WASI host-HTTP and supplied-stream counterparts have the explicit contracts below.

## ADDED Requirements

### Requirement: Library: header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::beast`, with public headers under `<webcpp/beast/...>`, one per responsibility, macros prefixed `WEBCPP_BEAST_`, for native, WASIp2, WASIp3 and Emscripten. Each target SHALL be declared supported only after its transport dependencies and executable tests pass. Missing Asio support for WASIp2 raw sockets MUST be tracked as prerequisite work for that backend, not a gate on a host-HTTP backend that does not use sockets.

#### Scenario: A program names the library like Boost.Beast
- **WHEN** a program writes `namespace beast = webcpp::beast;` where it wrote `namespace beast = boost::beast;`
- **THEN** the supported profiles compile with the names of Boost.Beast's layered code, and an unavailable profile reports its specific dependency rather than silently selecting another target

### Requirement: Library: build target and dependencies
`build.jam` SHALL declare `project /webcpp/beast` and a public alias `beast` whose usage requirements include the library's headers, the required executor integration, applicable Boost headers and `/webcpp/config//config`. Backend-specific targets or properties MUST select the dependencies: WASI host HTTP uses webcpp::wasi's component bindings, while a socket profile uses the relevant Asio transport and, for guest-owned TLS, the shared OpenSSL/CA targets. Host HTTP MUST NOT inherit socket/TLS build prerequisites merely through a common alias. The alias MUST carry the `webcpp-beast-unsupported` feature's define (see the Config requirements) in its usage requirements.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/beast//beast` and includes a public header
- **THEN** it compiles and links without any library of webcpp::beast's own being built

### Requirement: Library: repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (`"port-of": null`), `README.md`, `AGENTS.md` (only what is specific to beast), `LICENSE_1_0.txt` (BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file), `doc/changelog.adoc`, and a CI workflow calling the superproject's.

#### Scenario: The superproject's lint accepts the layout
- **WHEN** the superproject's lint reads `libs/beast`
- **THEN** it finds each of these files and reports no rule failed

### Requirement: Library: the project's rules hold from the first commit
The library SHALL be built and tested with exceptions on and off (`WEBCPP_BEAST_NO_EXCEPTIONS`), on both Emscripten suspension variants, with allocators the program gives, Boost before custom code, outside interfaces wrapped and fixed-width types. Errors MUST be values; an exception is raised only through `boost::throw_exception` and only with exceptions enabled.

#### Scenario: The library works without exceptions
- **WHEN** a program using the WebSocket client is built with exceptions off and a connection fails
- **THEN** the handler receives an `error_code`, and nothing is thrown

### Requirement: Names: Beast's network-free names are Beast's own
Native network-free names (buffers, HTTP messages, parsers, serializers, close reasons and errors) SHALL remain upstream Beast types unless specified otherwise. Wasm SHALL preserve their required public operations and semantics. Whether Wasm reuses those upstream types or supplies compatible types remains an explicit dependency decision before implementation; it MUST NOT be inferred from using a host transport.

#### Scenario: An alias is the same type
- **WHEN** a native test compares `webcpp::beast::flat_buffer` with `boost::beast::flat_buffer` through `std::is_same_v`
- **THEN** the assertion holds

### Requirement: Names: ssl names are webcpp::asio's
A program SHALL write `webcpp::asio` where it wrote `boost::asio`, `ssl` included (`namespace ssl = webcpp::asio::ssl;`). webcpp::asio owns the `ssl` names, which are Boost's in native/socket backends and a record of the program's TLS intent in the browser. Host-service backends MUST validate that intent against host capabilities and MUST NOT imply that guest OpenSSL settings configure host TLS.

#### Scenario: The browser stream reads the recorded settings
- **WHEN** on Emscripten a program sets `ctx.set_verify_mode(ssl::verify_peer)` on a `webcpp::asio::ssl::context` and passes it to an `ssl_stream<tcp_stream>`
- **THEN** the stream's TLS handshake judges that recorded setting

### Requirement: Names: tcp_stream
`tcp_stream` SHALL be Beast's own on native and supported WASI socket-backend profiles that reuse upstream types. On Emscripten it MUST be a browser stream of webcpp's that records the resolver's entry and keeps Beast's timeout interface (`expires_after`, `expires_never`) on a Boost timer. HTTP message operations dispatch directly to fetch; an explicit byte-stream adapter is a separate compatibility path (see the HTTP requirements).

#### Scenario: Native tcp_stream is Beast's
- **WHEN** a native test compares `webcpp::beast::tcp_stream` with `boost::beast::tcp_stream`
- **THEN** they are the same type

### Requirement: Names: ssl_stream
`ssl_stream<tcp_stream>` SHALL be Beast's own on native and supported WASI socket-backend profiles that reuse upstream types. On Emscripten it MUST be a browser stream of webcpp's that records the TLS settings and makes the scheme `wss` or `https`.

#### Scenario: The TLS stream makes the scheme secure
- **WHEN** on Emscripten `ws.next_layer().async_handshake(ssl::stream_base::client)` succeeds on a `websocket::stream<ssl_stream<tcp_stream>>`
- **THEN** the WebSocket handshake opens a URL whose scheme is `wss`

### Requirement: Names: websocket::stream natively
`websocket::stream<NextLayer>` SHALL on native and supported WASI socket-backend profiles that reuse upstream types be a class derived from Beast's, adding the allocator constructor `template <class Allocator, class... Args> explicit stream(std::allocator_arg_t, const Allocator& allocator, Args&&... next_layer_args)`, which is present on every supported implementation of the stream. A host-HTTP profile MUST NOT imply support for WebSocket upgrades or a guest protocol engine.

#### Scenario: The native stream derives from Beast's
- **WHEN** a native static assertion tests `std::is_base_of_v<boost::beast::websocket::stream<boost::beast::tcp_stream>, webcpp::beast::websocket::stream<webcpp::beast::tcp_stream>>`
- **THEN** it holds

### Requirement: Names: websocket::stream on Emscripten is selected by its next layer
On Emscripten, `websocket::stream<NextLayer>` over the browser `tcp_stream` or `ssl_stream` SHALL be webcpp's browser WebSocket. Over a supported explicit byte-stream next layer (a relay socket, a WebRTC channel's byte stream), it MUST use a guest WebSocket protocol engine. Upstream Beast is the proposed engine for that opt-in path; its inclusion remains subject to the Wasm dependency decision and MUST NOT become a prerequisite for the browser-native WebSocket backend.

#### Scenario: A byte stream gets a guest WebSocket engine
- **WHEN** on Emscripten a program instantiates `websocket::stream` over a user byte stream
- **THEN** the selected guest protocol implementation frames the bytes itself, and the dependency is explicit

### Requirement: Names: HTTP algorithms
HTTP operations SHALL retain their supported Beast-shaped signatures, completion-token behaviour and error/allocator contracts. Host-service backends MAY dispatch or overload those operations to map messages directly to fetch or wasi:http; they MUST NOT require a simulated TCP connection or a serialize-parse round trip as an architectural prerequisite. Explicit parser/serializer byte-stream adapters SHALL remain separately tested compatibility paths.

#### Scenario: HTTP dispatch uses the host service
- **WHEN** a supported HTTP request/response operation uses a host-service backend
- **THEN** its request is sent through fetch or wasi:http and its completion preserves the documented application contract, without requiring an upstream Beast network operation

### Requirement: Names: set_server_name
The library SHALL provide `set_server_name(ssl_stream&, std::string_view host)` returning `boost::system::result<void>`. Native and WASI socket profiles with guest-owned TLS MUST set SNI through OpenSSL. Host-service profiles MUST record the requested host and validate it against the eventual URL authority; a differing SNI name that the host cannot honour MUST follow the unsupported-setting policy. Validation MAY be deferred until that authority is known. Host profiles MUST NOT require OpenSSL or claim to configure host TLS directly. This replaces `SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host)`, since Boost 1.92 has no SNI helper and there is no OpenSSL handle in a browser.

#### Scenario: SNI delegated to the browser
- **WHEN** on Emscripten a program calls `set_server_name(ws.next_layer(), "echo.example.org")`
- **THEN** the requested host is recorded and a handshake for that same URL host succeeds through browser-owned TLS without guest OpenSSL

#### Scenario: Host TLS cannot use a different SNI name
- **WHEN** under `ERROR` a host-service request uses a URL authority different from the host recorded by `set_server_name`
- **THEN** the dependent operation completes with `operation_not_supported` before sending the request

### Requirement: Names: teardown for webcpp::asio's sockets
`teardown` and `async_teardown` for webcpp::asio's sockets SHALL be declared in the sockets' namespace, so that Beast finds them by argument-dependent lookup. Natively none is needed, because webcpp::asio's sockets are Boost's.

#### Scenario: Beast finds the teardown
- **WHEN** a `websocket::stream` over a webcpp::asio socket is closed on Emscripten
- **THEN** Beast's `async_teardown` call resolves to webcpp::beast's declaration by argument-dependent lookup

### Requirement: Resolver: the browser streams accept webcpp::asio's entry
On Emscripten the browser streams SHALL accept the single entry webcpp::asio's resolver completes with, made with `basic_resolver_results::create(endpoint, host, service)`: `host_name()` and `service_name()` are the arguments, and the endpoint has an unspecified address and the service's port. The port MUST be the number given, or 80 for `ws` and `http`, 443 for `wss` and `https`; another service name fails with `boost::asio::error::service_not_found`. A plain socket's connect on such an endpoint fails with `operation_not_supported` (decision of the owner, 2026-10-10, option A).

#### Scenario: The resolver's port for a scheme
- **WHEN** on Emscripten `resolver.async_resolve("echo.example.org", "wss")` completes
- **THEN** the single entry's `service_name()` is `"wss"` and its endpoint port is 443

#### Scenario: Another service name
- **WHEN** `resolver.async_resolve("echo.example.org", "gopher")` is called
- **THEN** it completes with `boost::asio::error::service_not_found`

### Requirement: WebSocket: connecting records, it does not connect
`get_lowest_layer(ws).async_connect(results)` SHALL record the first entry's host and service and complete at once. `async_connect(endpoint)` MUST record the endpoint's address as the host (an IPv6 address in brackets) and its port, and an endpoint with an unspecified address MUST fail with `webcpp::beast::browser_errc::no_host`, because the browser connects by name or by address and this one is neither.

#### Scenario: An IPv6 endpoint
- **WHEN** `async_connect` is called with the endpoint `[::1]:8080`
- **THEN** the recorded host is `[::1]` and the recorded port is 8080

#### Scenario: An unspecified address
- **WHEN** `async_connect` is called with an endpoint whose address is unspecified
- **THEN** it completes with `webcpp::beast::browser_errc::no_host`

### Requirement: WebSocket: the TLS handshake judges the record
`ws.next_layer().async_handshake(ssl::stream_base::client)` SHALL judge the recorded TLS settings (see the Settings requirements) and record that the scheme is `wss`; the browser does TLS and verifies the certificate.

#### Scenario: A client certificate is refused
- **WHEN** under `WEBCPP_BEAST_UNSUPPORTED_ERROR` a program calls `ctx.use_certificate_file(...)` and then the TLS handshake
- **THEN** the handshake completes with `boost::asio::error::operation_not_supported` and the connection stays as it was

### Requirement: WebSocket: the handshake opens the browser WebSocket
`ws.async_handshake(host, target)` SHALL open `new WebSocket("<scheme>://<host>:<port><target>", subprotocols)` from the recorded host and port and the handshake's target, with `binaryType = "arraybuffer"`, and complete on `open` or with the error of the Errors requirements. The `host` argument is the `Host` field, which the browser sets itself: it MUST be honoured when it equals the URL's host or `host:port`, and unsupported otherwise.

#### Scenario: Host equals the URL's host
- **WHEN** the recorded host is `echo.example.org`, port 8080, and `async_handshake("echo.example.org:8080", "/")` is called
- **THEN** the browser WebSocket is opened for `ws://echo.example.org:8080/` and the handshake completes on `open`

#### Scenario: A different Host
- **WHEN** under `ERROR` `async_handshake("other.example.org", "/")` is called for the recorded host `echo.example.org`
- **THEN** it completes with `operation_not_supported`

### Requirement: WebSocket: reading a whole message
`ws.async_read(buffer)` SHALL complete with the next whole message, copied once into the dynamic buffer: binary from the `ArrayBuffer`, text with `TextEncoder.encodeInto` into the WebAssembly memory, the view taken again after a memory growth. `got_text()` and `got_binary()` report the message's kind.

#### Scenario: A text message
- **WHEN** the server sends the text message `hello` and the program reads it into a `flat_buffer`
- **THEN** the buffer holds the five bytes `hello` and `got_text()` is true

### Requirement: WebSocket: reading in pieces
`ws.async_read_some(buffer, n)` and `async_read_some(MutableBufferSequence)` SHALL return the next message in pieces of at most `n` bytes, or the buffers' size, and `is_message_done()` MUST be true after the last piece.

#### Scenario: A message in two pieces
- **WHEN** a 10-byte message is read with `async_read_some(buffer, 6)` twice
- **THEN** the pieces are 6 and 4 bytes and `is_message_done()` is false after the first and true after the second

### Requirement: WebSocket: writing a message
`ws.async_write(buffers)` SHALL send a string (decoded from UTF-8) when `text()` is set and an `ArrayBuffer` when `binary()` is, and complete once the message is handed to the browser (it counts toward the send bound).

#### Scenario: A binary message
- **WHEN** a program calls `ws.binary(true)` and writes the three bytes `01 02 03`
- **THEN** the peer receives one binary message of those bytes

### Requirement: WebSocket: partial writes become one message
`ws.async_write_some(fin, buffers)` SHALL buffer the pieces until `fin` is true and then send them as one message: the peer receives the same message, and fragmentation is framing the browser does not expose. The buffered bytes MUST count toward the send bound.

#### Scenario: Two pieces, one message
- **WHEN** a program writes `ab` with `fin` false and then `cd` with `fin` true
- **THEN** the peer receives one message `abcd`

### Requirement: WebSocket: closing
`ws.async_close(reason)` SHALL call the browser's `close(code, reason)` and complete on `close`. The codes the browser accepts, 1000 and 3000 to 4999, are honoured; any other (Beast's `going_away`, 1001, and the rest of the codes of `boost/beast/websocket/rfc6455.hpp`) is unsupported. The reason fits: Beast's `reason_string` holds at most 123 UTF-8 bytes.

#### Scenario: A code the browser refuses
- **WHEN** under `ERROR` a program calls `async_close(websocket::close_code::going_away)`
- **THEN** the call completes with `operation_not_supported` and the connection stays open

#### Scenario: A close with an unsupported code under IGNORE
- **WHEN** under `IGNORE` the same call is made
- **THEN** the connection closes with code 1000

### Requirement: WebSocket: the longest message
`read_message_max` SHALL be enforced by the library: a longer message fails with `websocket::error::message_too_big` and closes the connection without a code, since the browser refuses Beast's 1009.

#### Scenario: A message past the limit
- **WHEN** `read_message_max` is 1024 and the server sends 2048 bytes in one message
- **THEN** the read completes with `websocket::error::message_too_big` and the connection is closed

### Requirement: WebSocket: timeouts on a Boost timer
`get_lowest_layer(ws).expires_after(d)` and `expires_never()` SHALL keep Beast's timeout interface on the stream's next operation, on a Boost timer: when it fires, the operation completes with `beast::error::timeout` and the connection is closed.

#### Scenario: A read that outlasts expires_after
- **WHEN** a program calls `expires_after(100ms)` and starts a read the server never answers
- **THEN** the read completes with `beast::error::timeout` and the browser socket is closed

### Requirement: WebSocket: completion tokens and the executor
Every operation SHALL take any completion token, through `boost::asio::async_initiate`, and its handler MUST run on webcpp::asio's io_context, posted from the browser's events by webcpp::asio's bridge.

#### Scenario: A handler runs on the io_context
- **WHEN** the browser's `open` event fires for a pending handshake
- **THEN** the handshake's handler is posted to the stream's executor and runs inside `io_context::run`, never inside the event

### Requirement: Settings: every setting is classified
Each setting of Beast's and Asio's that a program can set SHALL be classified by outcome: honoured where the browser does the same thing, unsupported where it cannot, and ignored only where the browser does an equivalent thing of its own, with the reason. Each case MUST be named in the documentation's table of differences.

#### Scenario: The table names every case
- **WHEN** the documentation page is built
- **THEN** its table of differences lists each unsupported and each ignored setting of this specification

### Requirement: Settings: TLS settings the browser honours
SNI through `set_server_name`, `ssl::verify_peer`, `ctx.set_default_verify_paths()`, `set_verify_callback(ssl::host_name_verification(h))` when `h` is the URL's host, and a version-flexible client method (`tls_client` or `sslv23_client`) SHALL be accepted as delegating negotiation to the browser. This delegation MUST NOT claim that the program controls the browser's enabled versions or cipher suites. The browser stream MUST tell `host_name_verification` from any other callback by its type.

#### Scenario: Host name verification for the URL's host
- **WHEN** a program sets `ssl::host_name_verification("echo.example.org")` and connects to that host
- **THEN** the TLS handshake succeeds

### Requirement: Settings: TLS settings the browser cannot honour
An exact-version method (`tlsv12_client`, `tlsv13_client`, or an older fixed version), `ssl::verify_none`, any other verify callback, a client certificate or private key (`use_certificate*`, `use_private_key*`), added roots (`add_certificate_authority`, `load_verify_file`, `add_verify_path`), and `host_name_verification` for a host other than the URL's SHALL be unsupported, because the browser verifies by its own rules.

#### Scenario: TLS 1.3 only
- **WHEN** under `ERROR` a program uses `ssl::context ctx{ssl::context::tlsv13_client}` and runs the TLS handshake
- **THEN** it completes with `operation_not_supported`

#### Scenario: TLS 1.2 only
- **WHEN** under `ERROR` a program uses `ssl::context ctx{ssl::context::tlsv12_client}` and runs the TLS handshake
- **THEN** it completes with `operation_not_supported`, instead of claiming that the browser enforced TLS 1.2

### Requirement: Settings: stream timeouts
`stream_base::timeout` SHALL be honoured for `handshake_timeout`, and for `idle_timeout` with `keep_alive_pings` false: the connection closes after the interval without activity, as Beast's, where activity is messages and close frames (the browser answers the peer's pings itself and shows none). `idle_timeout` with `keep_alive_pings` true MUST be unsupported, because the browser sends no ping on request. `suggested(role_type::client)`, a 30 s handshake timeout with no idle timeout and no pings, MUST be honoured whole.

#### Scenario: Idle timeout with pings
- **WHEN** under `ERROR` a program sets a timeout with `keep_alive_pings` true and runs the WebSocket handshake
- **THEN** the handshake completes with `operation_not_supported`

#### Scenario: suggested client timeout
- **WHEN** a program sets `stream_base::timeout::suggested(role_type::client)`
- **THEN** the handshake completes normally and a handshake slower than 30 s fails with `beast::error::timeout`

### Requirement: Settings: ping, pong and control callbacks
`async_ping` and `async_pong` SHALL be unsupported. `control_callback` MUST be honoured for close frames, whose code and reason the `close` event carries; pings and pongs are never reported.

#### Scenario: A ping
- **WHEN** under `ERROR` a program calls `ws.async_ping({})`
- **THEN** it completes with `operation_not_supported` and the connection stays as it was

### Requirement: Settings: the decorator
`set_option(stream_base::decorator)` SHALL be called by the library on an empty `websocket::request_type` at the handshake: `Sec-WebSocket-Protocol` becomes the constructor's subprotocols and is honoured; any other field (`User-Agent`) is unsupported, since the browser sets its own.

#### Scenario: A subprotocol in the decorator
- **WHEN** the decorator sets `Sec-WebSocket-Protocol: chat`
- **THEN** the browser WebSocket is opened with the subprotocol `chat`

#### Scenario: A User-Agent in the decorator
- **WHEN** under `ERROR` the decorator sets `User-Agent`
- **THEN** the WebSocket handshake completes with `operation_not_supported`

### Requirement: Settings: ignored options
`permessage_deflate`, `auto_fragment` and `write_buffer_bytes` SHALL be ignored, on both settings, because the browser negotiates compression and frames messages itself, and the messages the peer receives are the same.

#### Scenario: Compression requested
- **WHEN** a program enables `permessage_deflate` and writes a message
- **THEN** no error is reported and the peer receives the same message

### Requirement: Settings: ERROR and IGNORE
Under `WEBCPP_BEAST_UNSUPPORTED_ERROR` (the default) an unsupported setting SHALL make the operation that depends on it complete with `boost::asio::error::operation_not_supported` (the TLS handshake for a TLS setting, the WebSocket handshake for the decorator and the timeout, the call itself for `async_ping`, `async_pong` and `async_close`), and the connection MUST stay as it was. Under `WEBCPP_BEAST_UNSUPPORTED_IGNORE` the setting MUST be ignored, and an unsupported close code closes with 1000.

#### Scenario: Same program, two settings
- **WHEN** a program that sets `User-Agent` in its decorator is built once with each setting
- **THEN** the handshake fails with `operation_not_supported` under `ERROR` and succeeds under `IGNORE`

### Requirement: Back-pressure: sending is bounded
The browser write SHALL check that the WebSocket is OPEN and report failures as error values. It MUST NOT block the browser event loop or claim peer delivery when the browser has only accepted a message for sending. The library MUST bound `bufferedAmount` (default 4 MiB, an option), and a write that would pass the bound fails with `webcpp::beast::browser_errc::send_limit` and closes the connection.

#### Scenario: A writer faster than the server
- **WHEN** a program writes messages in a loop to a server that does not read, until `bufferedAmount` would pass 4 MiB
- **THEN** that write completes with `browser_errc::send_limit` and the stream is closed

### Requirement: Back-pressure: receiving is queued and bounded
Messages SHALL arrive whether a read is pending or not and wait in a queue in the stream's allocator, bounded by an option; past the bound the stream closes without a code and the next read completes with `webcpp::beast::browser_errc::receive_limit`. A queued message MUST NOT keep `io_context::run` alive.

#### Scenario: A reader that does not read
- **WHEN** the server sends messages past the queue bound while the program reads none
- **THEN** the stream closes without a code and the next `async_read` completes with `browser_errc::receive_limit`

#### Scenario: A queued message and run
- **WHEN** a message is queued and no operation is pending
- **THEN** `io_context::run` returns

### Requirement: Lifetime: cancellation by type
Cancellation SHALL act per operation and per cancellation type. `async_handshake` under `terminal` or `partial` closes the browser socket (`close()`) and completes with `operation_aborted`, and under `total` is not supported, since an opening cannot be undone. `async_read` and `async_read_some` complete with `operation_aborted` under every type and the message stays queued. `async_write` and `async_write_some` are not affected, since the message is handed to the browser when the operation starts. `async_close` under `terminal` or `partial` completes with `operation_aborted` while the browser's close goes on, and under `total` is not supported.

#### Scenario: A cancelled read keeps its message
- **WHEN** a pending `async_read` is cancelled with `terminal` and a message then arrives
- **THEN** the read completed with `operation_aborted`, and the next `async_read` returns that message

### Requirement: Lifetime: the destructor
The destructor SHALL complete every pending operation with `operation_aborted` (posted, so that no pending work keeps `run` alive for good), close the browser socket with `close()` without a code, and remove its registrations from webcpp::asio's bridge, so that a late event finds nothing.

#### Scenario: A late event after destruction
- **WHEN** a stream is destroyed with a read pending and the browser then delivers a message for it
- **THEN** the read handler ran with `operation_aborted` and the late event finds no registration and does nothing

### Requirement: Allocation: operations and stream state
Operation state SHALL use the handler's associated allocator, as Beast's does. The stream's own state (the receive queue, the registrations, the buffered fragments) MUST use the allocator given to the allocator constructor, and `std::allocator<std::byte>` without it. Natively Beast's own state uses Beast's allocation, which the documentation names.

#### Scenario: The counting test
- **WHEN** a counting resource is passed to the stream's allocator constructor and a session of handshake, writes, reads and close runs in the browser on both variants
- **THEN** no allocation of the stream's state goes outside the given allocator

### Requirement: Errors: the category webcpp.beast
Failures with no counterpart in Beast or Asio SHALL be in webcpp::beast's own category, named `webcpp.beast`, with fixed values never reused: `invalid_url` (1), `blocked` (2), `connection_failed` (3), `send_limit` (4), `receive_limit` (5), `fetch_failed` (6), `no_host` (7), as members of `webcpp::beast::browser_errc`, convertible to `boost::system::error_code`. On native and profiles reusing upstream types, `webcpp::beast::error` SHALL remain the upstream Beast error enum, including `timeout`, without changed values or category. Other profiles MUST preserve the documented error names, meanings and comparisons; exact cross-library identity is part of the pending Wasm dependency decision. Browser-only errors MUST NOT replace the timeout enum.

#### Scenario: Values are fixed
- **WHEN** a test reads the integer value of each `webcpp::beast::browser_errc` enumerator
- **THEN** they are 1 to 7 in the order above, and the category's `name()` is `"webcpp.beast"`

#### Scenario: Upstream timeout keeps its identity
- **WHEN** on native or a profile reusing upstream types an application converts `webcpp::beast::error::timeout` and `webcpp::beast::browser_errc::invalid_url` to error codes
- **THEN** timeout retains Boost.Beast's type and category, and invalid_url belongs to the distinct `webcpp.beast` category

### Requirement: Errors: browser outcomes by name
The browser's outcomes SHALL map to error codes: a constructor throwing `SyntaxError` (a URL or subprotocol it rejects) to `invalid_url`; one throwing `SecurityError` to `blocked`; `error` then `close` 1006 before `open` (refused, DNS, TLS, a blocked port) to `connection_failed`; `close` with a code after `open` to `websocket::error::closed`, with `ws.reason()` holding the code and the reason, as Beast's; `close` 1006 after `open` to `boost::asio::error::connection_reset`; a timeout to `beast::error::timeout`; a message past `read_message_max` to `websocket::error::message_too_big`; a setting the browser cannot honour to `boost::asio::error::operation_not_supported`.

#### Scenario: A refused connection
- **WHEN** the server refuses the connection and the browser reports `error` then `close` 1006 before `open`
- **THEN** the handshake completes with `webcpp::beast::browser_errc::connection_failed`

#### Scenario: The peer closes with a code
- **WHEN** the server closes with code 4000 and reason `bye` after `open`
- **THEN** a pending read completes with `websocket::error::closed`, and `ws.reason().code` is 4000 and its reason is `bye`

#### Scenario: An invalid URL
- **WHEN** the recorded host is not a valid URL host and the handshake is made
- **THEN** it completes with `webcpp::beast::browser_errc::invalid_url`

### Requirement: HTTP: direct host operations and explicit byte adapters
The browser HTTP backend SHALL support the specified message operations, including asynchronous read/write, header-only reads and body limits, through fetch. Applications explicitly using parser/serializer byte-stream interfaces SHALL receive a separately documented adapter; the default host path MUST NOT depend on reconstructing a complete wire exchange merely to call Beast's implementation. This is the second increment.

#### Scenario: Beast's client over fetch
- **WHEN** the twin of Beast's `http_client_async` runs in the browser against a local server
- **THEN** it produces the same application status and decoded payload as natively, with documented browser header/framing differences

### Requirement: HTTP: writing makes one fetch
A complete request SHALL make one fetch, whether supplied directly as a message or completed through the explicit byte-stream adapter: the method, the URL from the recorded scheme, host and port and the request's target, the fields `fetch` allows, and the body, buffered whole (a chunked body is sent with its length: the server receives the same bytes). `redirect` is `"follow"` and `mode` is `"cors"`.

#### Scenario: A GET
- **WHEN** a program writes `GET /index.html HTTP/1.1` with `Host` equal to the recorded host
- **THEN** one `fetch` is made for `<scheme>://<host>:<port>/index.html` with method GET, `redirect: "follow"` and `mode: "cors"`

### Requirement: HTTP: response mapping and optional synthetic framing
The direct host backend SHALL return response metadata and a decoded body through compatible message operations. If the explicit byte-stream adapter is selected, it SHALL expose a synthetic HTTP/1.1 representation: the status line, with Beast's reason phrase for the status when the response has none (HTTP/2), the fields `fetch` exposes, and the decoded body. Responses that permit a body SHALL use `Transfer-Encoding: chunked`, omitting the original `Content-Length` and `Content-Encoding`. Responses to HEAD and status 204/304 SHALL end after the headers, with no body, chunk terminator or synthetic Transfer-Encoding. The facade MUST remember the request method; HEAD callers retain Beast's `response_parser::skip(true)` convention. The omission of representation metadata for these bodyless responses MUST be documented, and no leftover framing bytes may corrupt the next exchange.

#### Scenario: A response without a reason phrase
- **WHEN** the explicit byte-stream adapter receives a fetch response with status 200 and an empty status text
- **THEN** the bytes read begin `HTTP/1.1 200 OK` and carry `Transfer-Encoding: chunked`

#### Scenario: A bodyless response followed by another exchange
- **WHEN** the byte-stream adapter client performs HEAD or receives 204/304 and then performs GET on the same facade
- **THEN** the first response leaves no body or chunk terminator in the input buffer and the next response parses correctly

### Requirement: HTTP: request fields
`Host` SHALL be honoured when it is the URL's host or `host:port` (the browser sets it from the URL), unsupported otherwise. `Content-Length` and `Transfer-Encoding` are the framing of the bytes and MUST be honoured. `Connection` and `Keep-Alive` MUST be ignored, on both settings, since connection reuse is the browser's and the exchange is the same. `User-Agent` and every other field the browser forbids or sets itself SHALL be unsupported.

#### Scenario: User-Agent
- **WHEN** under `ERROR` a request carries `User-Agent: beast`
- **THEN** the write completes with `operation_not_supported`

#### Scenario: Connection is ignored
- **WHEN** a request carries `Connection: close`
- **THEN** no error is reported on either setting and the fetch is made

### Requirement: HTTP: redirects
`fetch` follows redirects and the program would see the final response, not the 3xx it asked for. When `response.redirected` is true, the read SHALL complete with `operation_not_supported` under `ERROR`, and with the final response under `IGNORE`.

#### Scenario: A redirected response
- **WHEN** the server answers a request with a 302 to another path and the final response is 200
- **THEN** under `ERROR` the read completes with `operation_not_supported`, and under `IGNORE` it yields the 200 response

### Requirement: HTTP: methods and bodies the browser cannot send
`CONNECT`, and a request whose body the browser cannot send, SHALL be unsupported.

#### Scenario: CONNECT
- **WHEN** a program writes a `CONNECT` request on the browser stream
- **THEN** the write completes with `operation_not_supported` under `ERROR`

### Requirement: HTTP: the stream's socket
`stream.socket().shutdown(...)` and `close()` SHALL end the stream: no further request is made, and they succeed. The browser stream's `socket()` MUST be a member of webcpp::beast's that offers what Beast's examples call on it: `shutdown`, `close`, `is_open`, `cancel`, `get_executor`.

#### Scenario: Shutdown after a response
- **WHEN** a program calls `stream.socket().shutdown(tcp::socket::shutdown_both, ec)` after reading a response, as `http_client_async` does
- **THEN** `ec` holds no error and no further request is made

### Requirement: HTTP: failed fetches
A `fetch` that rejects SHALL complete the read with `webcpp::beast::browser_errc::fetch_failed`; CORS and mixed content are among its causes, which the browser does not tell apart.

#### Scenario: A rejected fetch
- **WHEN** `fetch` rejects with `TypeError`
- **THEN** the read completes with `browser_errc::fetch_failed`

### Requirement: Config: the unsupported-setting macros
`WEBCPP_BEAST_UNSUPPORTED_ERROR` and `WEBCPP_BEAST_UNSUPPORTED_IGNORE` SHALL be read in `<webcpp/beast/config.hpp>`, which every public header includes first, with `WEBCPP_BEAST_NO_EXCEPTIONS`. `ERROR` MUST be defined when neither is, and both defined MUST be an `#error`.

#### Scenario: Both defined
- **WHEN** a translation unit defines both macros and includes a public header
- **THEN** compilation stops with an `#error`

#### Scenario: Neither defined
- **WHEN** a translation unit defines neither
- **THEN** `WEBCPP_BEAST_UNSUPPORTED_ERROR` is defined after `config.hpp`

### Requirement: Config: the b2 feature
A b2 feature `webcpp-beast-unsupported=error|ignore` (propagated) SHALL drive the macros from webcpp::beast's usage requirements, as `<webcpp-beast-unsupported>ignore:<define>WEBCPP_BEAST_UNSUPPORTED_IGNORE`, so that a whole program builds with one setting.

#### Scenario: A program built with ignore
- **WHEN** a program is built with `webcpp-beast-unsupported=ignore`
- **THEN** every translation unit it links is compiled with `WEBCPP_BEAST_UNSUPPORTED_IGNORE`

### Requirement: Config: the browser classes are tagged by the setting
The browser classes SHALL live in an inline namespace named after the setting, so different settings name different types. B2 SHALL propagate one setting throughout a program. A mismatch across an externally linked interface that passes a configured stream MUST fail to link; independent functions that do not exchange such types are not guaranteed to be diagnosed by the linker.

#### Scenario: Two units disagree
- **WHEN** one translation unit defines a function taking a configured stream under `ERROR`, another calls that function under `IGNORE`, and both are linked
- **THEN** linking fails with undefined or duplicate symbols naming the different inline namespaces

### Requirement: Config: the configuration test
`test/config_test.cpp` SHALL be built under each setting and with exceptions on and off, and check each case.

#### Scenario: The matrix is built
- **WHEN** the library's tests are built
- **THEN** `config_test` exists once per setting, and with exceptions on and off

### Requirement: Testing: native lanes
The native lanes SHALL test that the names are Beast's types (or derive from them) by static assertions, and that the guide's programs run against a local echo server.

#### Scenario: The guide's program runs natively
- **WHEN** the native lane runs a guide program against the local echo server
- **THEN** it exits 0 and prints the message echoed

### Requirement: Testing: the echo servers
The lanes SHALL use a native Beast echo server built by the native lane (no npm dependency), plain and TLS, because node has no WebSocket server built in. `wss` MUST use a local CA that node trusts through `NODE_EXTRA_CA_CERTS` and headless Chrome through the lane's certificate options.

#### Scenario: wss under node
- **WHEN** the Emscripten lane under node connects to the TLS echo server
- **THEN** the connection succeeds with the local CA trusted through `NODE_EXTRA_CA_CERTS`

### Requirement: Testing: Emscripten lanes under node
The Emscripten lanes SHALL run under node 26, on both suspension variants, and test: the layers (WebSocket requirements), the URL and `Host` rules, the settings under both configurations, cancellation and the destructor, back-pressure through the queue, a server that closes, refuses, or sends a message past `read_message_max`, every error of the Errors requirements that node can produce, allocation counts, and the HTTP client against the native server. They MUST NOT test CORS or mixed content, which node does not enforce.

#### Scenario: A server that closes
- **WHEN** the lane's server closes the connection with code 4001
- **THEN** the pending read completes with `websocket::error::closed` on both variants

### Requirement: Testing: the browser lane
The browser lane SHALL run the same programs in headless Chrome, and test what only a browser shows: CORS and mixed-content failures (and, with its own increment, `WebSocketStream`).

#### Scenario: A CORS failure
- **WHEN** the HTTP client in headless Chrome requests a server that sends no CORS headers
- **THEN** the read completes with `webcpp::beast::browser_errc::fetch_failed`

### Requirement: Testing: twins of the examples
Every runnable example SHALL be compared with its native upstream reference. The three original guide clients remain initial smoke tests, but MUST NOT be the acceptance boundary: the entire pinned inventory is required. Compare exact output where the platform exposes the same representation; otherwise compare status, exposed application headers and decoded payload with explicit normalization rules, retaining raw output.

#### Scenario: A twin's output
- **WHEN** a twin runs natively and then in the browser against the same echo server
- **THEN** application payloads and observable outcomes match, and any framing or browser-controlled-field normalization is recorded rather than hiding a behavioural difference

### Requirement: Examples: Beast's examples, setting by setting
For Beast 1.92's examples the twins use, the specification SHALL state what each needs under `ERROR` and under `IGNORE`. `websocket_client_async`: the decorator setting `User-Agent` fails the handshake under `ERROR` (the twin leaves it out), and runs unchanged under `IGNORE`. `websocket_client_async_ssl`: the SNI line becomes `set_server_name`; `host_name_verification`, `verify_peer` and `suggested(client)` are honoured; a fixed `tlsv12_client` fails under `ERROR`, and the adapted twin explicitly selects the browser-owned `tls_client` policy; the decorator and `load_root_certificates` (added roots) fail the handshakes under `ERROR`, and the twin leaves the decorator out and calls `ctx.set_default_verify_paths()` instead; under `IGNORE` the SNI line changes and the report explicitly says fixed TLS-version and custom-root requests were not applied. `http_client_async`: `Host` is honoured when it is the URL's host, `User-Agent` fails the write under `ERROR` (the twin leaves it out), and `shutdown` is honoured.

#### Scenario: The SSL example under ERROR
- **WHEN** `websocket_client_async_ssl` is built on Emscripten under `ERROR` with `load_root_certificates` unchanged
- **THEN** its TLS handshake completes with `operation_not_supported`, and the twin with `set_default_verify_paths()` succeeds

### Requirement: Documentation: the page and its table of differences
The library SHALL have a documentation page with a guide, a reference, and a table of differences that names each unsupported and ignored case, and the guide's examples MUST be built and run by the tests.

#### Scenario: The page builds
- **WHEN** the documentation target is built
- **THEN** it succeeds and its table of differences lists the settings of the Settings requirements


### Requirement: Portability: source changes are inspectable
The library SHALL prioritize existing Boost.Beast application source and preserve its protocol and programming model. Each example MUST retain an inspectable upstream-to-webcpp diff. An application rewritten from stackful to C++20 coroutines, synchronous to callbacks, or a system executor to an event loop MUST be reported as a separate adapted counterpart, never evidence that the original model works.

#### Scenario: A rewritten coroutine is not original compatibility
- **WHEN** a C++20 coroutine counterpart passes but the original `spawn` example cannot link Boost.Context
- **THEN** the original remains blocked with that dependency, and the counterpart is reported separately

### Requirement: Portability: all upstream example applications are accounted for
The acceptance inventory SHALL contain every executable in Boost.Beast tag `boost-1.92.0`, commit `7c1e061f91e2ef542217b76286c314d006c0c8fc`: 53 executable targets found in `example/**/Jamfile`, cross-checked with their entry points and CMake definitions. It MUST include source paths, supporting files, fixtures and documentation snippet harnesses. Every target/profile row SHALL record compilation and execution separately, adaptations, dependency restrictions, exact commands, versions and evidence.

#### Scenario: An example is omitted
- **WHEN** the upstream manifest contains an executable absent from webcpp's coverage manifest
- **THEN** the coverage check fails and names the omitted path and target

#### Scenario: A compile-only or fallback program
- **WHEN** an example links but is never run, or a feature guard selects a fallback main that performs no exchange
- **THEN** it is not reported as a passed application or a working browser port

### Requirement: Portability: target and variant matrix
Documentation and test reports SHALL enumerate native, WASIp2, WASIp3 and Emscripten, with each applicable threading and exception setting, and JSPI/Asyncify separately for Emscripten. Every feature and example row MUST distinguish planned, passed, failed, blocked and platform-unavailable results. Unavailable or unimplemented combinations MUST retain their reasons and MUST NOT silently disappear from totals. Cooperative threads MUST NOT be described as CPU-parallel threads.

#### Scenario: WASIp3 cooperative exceptions are requested
- **WHEN** a build selects WASIp3 cooperative threads and exceptions with SDK 34's tested sysroot lacking its exception runtime
- **THEN** configuration fails with that specific limitation and the row remains visible in the report

#### Scenario: WASIp2 raw sockets are not yet implemented
- **WHEN** the current Asio dependency lacks a WASIp2 socket backend
- **THEN** the WASIp2 raw-socket profile remains a required blocked workstream with linked prerequisite tasks, while the independent host-HTTP profile retains its own status

### Requirement: WASI: host services and stream backends are independent
WASIp2 and WASIp3 SHALL offer a host-HTTP backend that maps application requests and responses to wasi:http. It MUST reuse and extend webcpp::wasi's component machinery without depending on an Asio TCP reactor, DNS workers or guest OpenSSL merely to send/receive HTTP. A separate socket/stream backend SHALL serve applications requiring raw transport semantics; its socket/reactor/resolver fixes belong to Asio or wasi-libc and its guest-owned TLS belongs to the shared TLS build. Each backend and ABI version MUST have independent evidence.

#### Scenario: WASIp2 host HTTP without raw sockets
- **WHEN** an HTTP application is built for the WASIp2 host-HTTP profile while the Asio TCP backend is absent
- **THEN** it can be built and tested through wasi:http, and a missing raw-socket dependency does not disable that workstream

#### Scenario: A WASI socket server serves a browser client
- **WHEN** an explicitly selected WASI socket-backend server has loopback listen permission and a browser client connects
- **THEN** a real HTTP or WebSocket exchange completes and the report names each endpoint's backend

#### Scenario: A host capability is missing
- **WHEN** the selected backend lacks permission or a host interface needed by an example
- **THEN** the application reports the specific error without hanging or silently switching backend

### Requirement: WASI: resolver compatibility is not inferred
In the socket backend, the single-thread WASIp3 async resolver's current `operation_not_supported` behaviour SHALL be tested and documented. Examples that use a pre-resolved endpoint or synchronous resolution instead MUST declare that adaptation while retaining the original hostname for HTTP Host, SNI and certificate verification. Original asynchronous DNS compatibility MUST remain blocked until the backend provides it.

#### Scenario: An address is supplied to bypass DNS
- **WHEN** a single-thread WASIp3 HTTPS example uses a fixture's numeric endpoint but the application hostname is `localhost`
- **THEN** TLS still verifies `localhost`, the report labels the resolver adaptation, and it does not claim that async DNS passed

### Requirement: Examples: synchronous browser clients yield to the browser
Synchronous client examples SHALL have a suspending transport facade that preserves their synchronous call sequence on JSPI and Asyncify. Waiting MUST allow browser events to run without busy-waiting. The facade MUST NOT recursively invoke `io_context::run` from a running handler; unsupported reentrant entry MUST be diagnosed. Until its probes and applications pass, synchronous browser compatibility SHALL remain blocked.

#### Scenario: A delayed synchronous read
- **WHEN** a synchronous client runs from the application's outer entry point and waits for a delayed response
- **THEN** a browser heartbeat runs during the wait, the original call resumes with the expected bytes, and the test passes independently on JSPI and Asyncify

### Requirement: Examples: execution dependencies are explicit
Stackful coroutine examples SHALL test Boost.Coroutine/Context, including unwinding and exception restrictions, separately from C++20 awaitables and stackless state-machine coroutines. Examples using a system executor, threads, signal handlers or Unix-domain sockets SHALL name those dependencies and test the available profiles. The ignore policy MUST NOT mask a missing execution backend or transport.

#### Scenario: Asyncify is available but Context is not
- **WHEN** Asyncify builds a callback example but the stackful example has no usable Boost.Context backend
- **THEN** the stackful example remains blocked and the report names the missing backend

### Requirement: Examples: browser server limitations are explicit
Browser APIs SHALL NOT be represented as TCP listeners, Unix-domain sockets or arbitrary raw byte sockets. Server and local-socket examples MUST remain in the inventory with the missing facility and a diagnostic test. Their native/WASI versions and browser-facing client counterparts SHALL be tested where supported, without claiming the original server executes inside the browser.

#### Scenario: The chat application is used from a browser
- **WHEN** its server runs in WASI and two browser clients exchange messages
- **THEN** the application test records WASI server and browser client success, while browser server execution remains unavailable

### Requirement: Testing: bounded deterministic application exchanges
Runnable examples SHALL execute real exchanges against controlled local peers with readiness checks, deadlines, deterministic fixture data and cleanup. Tests MUST cover successful payloads, TLS verification and hostname failures, timeout, cancellation during transfer, peer closure and shutdown errors where applicable. The crawler MUST use a bounded local corpus, not public-internet crawling. Browser success requires an actual browser run, not Node alone.

#### Scenario: A peer never replies
- **WHEN** a fixture deliberately withholds a response
- **THEN** the test verifies the expected timeout or cancellation, stops within its deadline, and leaves no server or browser process running

### Requirement: Testing: security settings are not weakened for compatibility
TLS examples SHALL use verified peers and explicit trust configuration. Native and WASI guest-TLS socket backends SHALL use shared source-built TLS with embedded or replacement CAs. Browser and WASI host-HTTP backends SHALL use the selected host's TLS/trust policy; guest CA settings MUST NOT be claimed effective there without an exposed and tested host capability. Adding a test CA MUST NOT disable verification globally. The test report MUST distinguish a genuine trust path from any test-environment certificate exception.

#### Scenario: The peer certificate names another host
- **WHEN** the fixture presents a trusted certificate for the wrong application hostname
- **THEN** the verified client refuses it on every supported TLS profile

### Requirement: Documentation: portable programs and reproducible evidence
The documentation SHALL use Diataxis, AsciiDoc and MrDocs, with tutorials, task guides, API reference and explanations. It MUST publish both feature and example matrices, exact compile and runtime commands, source adaptations, capability grants and expected output from the tested examples. Original and adapted rows MUST remain visible. A host-service or stream counterpart can fulfil the requested application use case, but MUST NOT be reported as exact source/transport parity. Promised backends with unresolved implementation blockers MUST NOT be reported complete.

#### Scenario: A reader chooses an example
- **WHEN** the reader selects a target, threading mode, exception mode and suspension variant
- **THEN** the documentation shows its supported commands and evidence, or a precise unresolved dependency or platform limitation


### Requirement: Backends: use the environment implementation
The implementation SHALL use browser WebSocket/fetch and WASI HTTP/stream facilities where they implement the application's required operation. Backend selection MUST be visible in build configuration or the transport type, with no silent fallback after a failure. Host HTTP operation support MUST be tested independently from the raw-socket profile. Eliminating upstream network execution MUST NOT be conflated with eliminating its headers or data types; the latter remains a separate explicit dependency decision.

#### Scenario: Browser WebSocket delegates framing
- **WHEN** an application uses the browser WebSocket backend
- **THEN** messages go through the JavaScript WebSocket API and no guest TCP connection or duplicate WebSocket wire handshake is required

### Requirement: WASI HTTP: adapt server entry and retain application logic
The host-HTTP server adapter SHALL map each incoming host request to the application's compatible request handler and map its response back to the host. P2 incoming-handler/response-outparam and P3 async-handler ABIs MUST be tested separately. The migration MUST show the changed bootstrap/listener plumbing and preserve the request-routing and response-building logic where applicable. It MUST NOT report a host HTTP request as an actual socket accept or promise physical connection identity, bind/listen options, keep-alive ownership, TLS inspection or upgrades the host does not expose.

#### Scenario: A native listener becomes a host HTTP component
- **WHEN** a server example is adapted to wasi:http
- **THEN** the host owns the listening port, the application's request handler serves the controlled HTTP exchange, and the result is recorded as a successful host-adapted server with its source diff

#### Scenario: The response body fails after headers
- **WHEN** a streaming host response is cancelled or fails after its headers have been committed
- **THEN** the remaining body is failed or cancelled, no second response is committed, and all resources are released within the owning ABI's lifetime

### Requirement: WASI HTTP: complete messages and operation contracts
The adapter SHALL cover methods, targets, exposed headers, status, streaming bodies, body limits and trailers where provided by the pinned host interface. It MUST preserve handler executor/allocator association and define completion byte counts as logical API counts rather than claiming the host's actual wire bytes. The existing method/target and whole-response helper alone MUST NOT be counted as complete Beast compatibility. Timeout/cancellation and unsupported fields/features MUST be tested against the selected host contract.

#### Scenario: A request body arrives in chunks
- **WHEN** a host delivers a request with headers and a multi-chunk body to an adapted server
- **THEN** the application sees the documented fields and payload, respects its body limit and back-pressure contract, and emits one response through the host

### Requirement: Streams: explicit counterparts for absent Unix-domain sockets
Examples using Unix-domain stream sockets SHALL have a useful counterpart over a host-supplied input/output pair, including stdin/stdout where wired to a peer. The adapter MUST document ownership, partial I/O, readiness, EOF, back-pressure and cancellation. It MUST NOT infer socket features from file descriptors or promise Unix path addressing, peer credentials, descriptor passing or multiple accepted connections. Protocol bytes MUST be separate from diagnostics.

#### Scenario: A local HTTP exchange uses standard streams
- **WHEN** the driver wires a client's output to the server's input and the server's output to the client's input
- **THEN** the adapted example exchanges the expected HTTP message through the two streams, stdout carries only protocol bytes, stderr carries diagnostics, and the original Unix-domain profile remains separately labelled

#### Scenario: A peer stops reading
- **WHEN** the stream fixture stops draining output and later cancels the operation
- **THEN** the adapter respects bounded buffering and the documented cancellation result, and the driver terminates without a deadlock

### Requirement: Evidence: application portability and exact parity are separate
The 53-example manifest SHALL record the original implementation and each requested environment-native or stream counterpart, with the same application-level expected exchange where meaningful. It MUST distinguish exact/source compatibility, application portability with a documented adaptation, and unavailable low-level facilities. A valid adapted counterpart SHALL count toward the application deliverable; it MUST NOT erase restrictions on the original transport or expand the upstream executable denominator.

#### Scenario: Unix sockets are unavailable but the stream counterpart works
- **WHEN** the original local-socket application cannot run but its supplied-stream counterpart passes the exchange tests
- **THEN** the matrix reports application portability as passed with adaptation and Unix-domain transport compatibility as unavailable
