# Supplied stream counterparts

These four explicit counterparts use a bidirectional stdin/stdout channel supplied by the host. They have no Unix socket path, connect or accept call, socket options, peer credentials, guest DNS, guest TCP, or guest TLS. stdout carries protocol bytes; stderr carries application output and diagnostics.

| Upstream application | Retained application behavior | Executed profiles |
| --- | --- | --- |
| `http/client/async-local` | GET target, Host/User-Agent, complete response and partial-response failure | Native POSIX, WASIp2, WASIp3 |
| `http/server/async-local` | Original `mime_type`, `path_cat`, `handle_request`, file/empty/string bodies, HEAD, errors, keep-alive and pipelined overread | Native POSIX, WASIp2, WASIp3 |
| `websocket/client/async-local` | HTTP upgrade decorator, text request/reply, normal close handshake | Native POSIX, WASIp3 |
| `websocket/server/async-local` | Upgrade decorator, text/binary echo, ping/pong, peer close handshake | Native POSIX, WASIp3 |

The supplied channel uses synchronous partial operations with readiness and cancellation. This is an explicit execution-model adaptation of the original asynchronous socket sessions, not asynchronous API parity. The WebSocket examples use the upstream protocol engine with an executor for its associated state. They do not run a socket reactor or receive network grants. WASIp2 lacks the supported guest WebSocket engine and is not declared for those two counterparts.

`prepare.py <clang-format>` extracts the immutable upstream HTTP server handler, preserving its author notice, and replaces `message_generator` with a variant of the same response body types. The variant lets the supplied stream write the selected body without requiring socket message-generator algorithms. Do not edit the generated server source manually.

`test/applications/streams/driver.py` supplies both directions of the channel. HTTP tests check routing, HEAD and content length, traversal rejection, missing files, unsupported methods, large file contents, pipelining, and truncated responses. WebSocket tests check actual upgrade and close handshakes, a large text message, 128 KiB of binary content and ping/pong. Every process has an outer driver timeout. The native POSIX rows pass with exceptions on and off; WASIp2 HTTP rows pass with exceptions on and off; all WASIp3 rows pass with exceptions on and off in single mode and with exceptions off in cooperative mode.

WASIp3 cooperative mode requires Wasmtime's component-model-threading feature because the SDK's cooperative libc may use context slot 1. It does not create a background thread in these applications. WASIp3 cooperative exceptions-on is outside the SDK's available runtime profiles.

The standard-descriptor adapter is unavailable on Windows: its constructor reports a platform diagnostic, while the header and generic `supplied_stream` remain usable. Windows callers can provide nonblocking channel callbacks. These four stdio applications are excluded only on that native OS.
