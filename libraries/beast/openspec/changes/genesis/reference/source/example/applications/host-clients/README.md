# Host HTTP client counterparts

The host owns HTTP transport, DNS and TLS. These counterparts preserve request and
response semantics through the host API; they do not claim unchanged socket scheduling.
The mapping records each adaptation. The separate `test/applications/host-tls` lane
measures actual HTTPS trust and the documentation client's host-owned mutual TLS.

The crawler uses the upstream `crawl_report`, counters, status histogram and report
formatting, extracted directly by `tools/prepare_host_clients.py`. Two asynchronous
workers retain the original get-next, complete, aggregate and repeat flow. A single
`wasi::http_context` serializes reporting and advances both host exchanges without
operating-system threads. Worker and message state remain alive through completion.

Each incoming request body supplies one `authority /absolute-path` pair per line.
The component validates the entire input before starting work, accepts at most 32
entries of at most 2048 characters, limits each response to 64 KiB, and gives every
exchange a five-second deadline. The result is the original text crawl report plus
`X-Crawl-Completed` and `X-Crawl-Workers` headers.

The host API combines DNS, connect, request write and response read into one exchange.
A failed exchange increments the report's `Read` counter; the original separate
`Timer`, `Resolve`, `Connect` and `Write` counters remain zero and are unmeasured.
This is an explicit transport-phase adaptation. Non-200 HTTP statuses are successful
reads and enter the status histogram, as in the upstream crawler. A failed exchange
continues to the next target instead of terminating the crawl.

The bounded test owns two loopback HTTP peers and supplies six distinct authority/path
pairs. A synchronization barrier requires two requests to be active simultaneously.
The peer responses yield two 200s, one 201, one 404, one 503 and one truncated body.
The driver verifies five successful reads, one read failure, all six destinations
exactly once, and continued work after the failure. A second request containing 33
entries must return 400 without contacting either peer.

Run the four ABI/exception profiles with configured SDK 34 and Wasmtime 50:

```sh
b2 toolset=clang-wasip2,clang-wasip3 exception-handling=on,off \
  libs/beast/test/applications/host-clients//http-crawl
```

`WASMTIME` overrides the runtime executable; otherwise the driver uses `wasmtime` on
`PATH`. The driver bounds readiness and HTTP waits, then stops the component process,
closes both peers and joins their workers.
