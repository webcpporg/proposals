# Isolated host TLS tests

The fixture tests the Beast WASI HTTP client through Wasmtime 50.0.1's P2 and
P3 imports with normal TLS chain and hostname verification. It creates a new
two-day CA and a server certificate whose only subject alternative name is
`DNS:localhost`. It never changes system trust, the user's browser profile,
or the production guest's networking implementation.

The driver makes actual guest requests over the fixture's loopback HTTP
front. Both `client::perform` and `async_perform` must reject an untrusted
CA, accept the trusted certificate at `localhost`, and reject that same
trusted certificate at `127.0.0.1`. Successful requests round-trip a 64 KiB
binary body and a response trailer. Negative cases must send no HTTP request
to the TLS peer and must report the expected underlying certificate failure.
The same lane independently runs all six generated HTTPS client application
counterparts against the owned TLS peer. Each must complete its original
payload check, then reject an unknown CA and a trusted wrong hostname.

For `doc-ssl-client`, the peer also requires a client certificate signed by
the ephemeral CA. The test host receives a separate client-authentication
certificate and private key as process-owned policy, and the peer confirms
that it authenticated that identity. A second attempt without a client
identity must fail with `CertificateRequired`. This preserves mutual TLS
without exposing OpenSSL or a guest-selected client identity through WASI.

Build the test host with Rust 1.97.0 or newer and put it on `PATH`:

```sh
cargo build --locked --manifest-path libs/beast/test/applications/host-tls/Cargo.toml \
  --target-dir .local/host-tls
export PATH="$PWD/.local/host-tls/debug:$PATH"
b2 libs/beast/test/applications/host-tls//applications-host-tls \
  toolset=clang-wasip2,clang-wasip3
```

`WEBCPP_WASI_TLS_FIXTURE` can name the executable instead. The generated
`Cargo.lock` pins the Rust dependency graph; the Wasmtime crates are also
pinned explicitly to `50.0.1` in the manifest.
The executable accepts `p2|p3 COMPONENT ROOT.der|none`, followed optionally
by `CLIENT.der CLIENT-KEY.der` for the mutual TLS fixture.

The stock Wasmtime CLI constructs its TLS root store from its embedded
`webpki_roots` list. It has no CLI option for an ephemeral test root. This
small embedding uses the supported `WasiHttpHooks::send_request` extension
point and a process-owned rustls root store. All component bindings, resource
tables, incoming request adapters, and P2/P3 body handling are Wasmtime's.
The test transport accepts only `https` authorities `localhost` and
`127.0.0.1`, connects only to loopback, and leaves rustls's ordinary verifier
enabled. Its connection and first-byte waits are bounded, and the test
driver bounds and cleans up the whole host process. The embedding is a
test fixture, not a replacement general-purpose Wasmtime CLI.

The same `trust.py` helper creates fresh full-Chromium profiles. `create DIR`
returns paths to the ephemeral CA and leaf files as JSON. `chrome PROFILE
CA.der` seeds the new profile's `Default/ServerCertificate` database using
Chromium's version 1 schema and trusted-CA metadata. Both destinations must
be empty. Full Chromium 156 reads these profile roots in headless mode; the
separate `headless_shell` executable does not supply that profile service.
No certificate-error or SPKI bypass flag is needed. Callers remove the
temporary certificate directory and profile after stopping the browser.

Primary implementation references:

- [Wasmtime 50.0.1 default HTTPS transport](https://github.com/bytecodealliance/wasmtime/blob/v50.0.1/crates/wasi-http/src/default_send_request.rs)
- [Wasmtime 50.0.1 HTTP hooks](https://github.com/bytecodealliance/wasmtime/blob/v50.0.1/crates/wasi-http/src/ctx.rs)
- [Chromium 155.0.8059.39 certificate database schema](https://chromium.googlesource.com/chromium/src/+/155.0.8059.39/components/server_certificate_database/server_certificate_database.cc)
- [Chromium 155.0.8059.39 certificate trust metadata](https://chromium.googlesource.com/chromium/src/+/155.0.8059.39/components/server_certificate_database/server_certificate_database.proto)
- [Chromium profile certificate service](https://chromium.googlesource.com/chromium/src/+/155.0.8059.39/chrome/browser/net/server_certificate_database_service_factory.cc)
