// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

//! A loopback-only Wasmtime test host with a process-owned TLS root store.

use bytes::Bytes;
use http_body_util::{BodyExt, Full};
use rustls::pki_types::{CertificateDer, PrivateKeyDer, PrivatePkcs8KeyDer, ServerName};
use std::future::Future;
use std::pin::pin;
use std::sync::Arc;
use std::time::Duration;
use tokio::net::{TcpListener, TcpStream};
use wasmtime::component::{Component, Linker, ResourceTable};
use wasmtime::{Config, Engine, Store};
use wasmtime_wasi::{WasiCtx, WasiCtxView, WasiView};
use wasmtime_wasi_http::io::TokioIo;
use wasmtime_wasi_http::{
    Error, RequestOptions, WasiBody, WasiHttpCtx, WasiHttpCtxView, WasiHttpHooks, WasiHttpView,
};

type IoResult = Box<dyn Future<Output = Result<(), Error>> + Send>;
type SendResult = Result<(http::Response<WasiBody>, IoResult), Error>;

#[derive(Clone)]
struct Trust {
    config: Arc<rustls::ClientConfig>,
}

impl WasiHttpHooks for Trust {
    fn send_request(
        &mut self,
        request: http::Request<WasiBody>,
        options: Option<RequestOptions>,
        _result: IoResult,
    ) -> Box<dyn Future<Output = SendResult> + Send> {
        let config = self.config.clone();
        Box::new(async move { send(request, options, config).await })
    }
}

// This test transport deliberately admits only HTTPS loopback requests. It uses
// rustls's normal chain and name verifier, never a custom certificate verifier.
// Request options cap connection and first-byte waits; the driver bounds the
// whole fixture process. Production requests still use Wasmtime's stock host.
async fn send(
    mut request: http::Request<WasiBody>,
    options: Option<RequestOptions>,
    config: Arc<rustls::ClientConfig>,
) -> SendResult {
    let authority = request
        .uri()
        .authority()
        .ok_or(Error::HttpRequestUriInvalid)?;
    if request.uri().scheme_str() != Some("https")
        || !matches!(authority.host(), "localhost" | "127.0.0.1")
    {
        return Err(Error::HttpRequestUriInvalid);
    }
    let port = authority.port_u16().ok_or(Error::HttpRequestUriInvalid)?;
    let name = ServerName::try_from(authority.host().to_owned())
        .map_err(|_| Error::HttpRequestUriInvalid)?;
    let limit = Duration::from_secs(10);
    let connect_timeout = options
        .and_then(|value| value.connect_timeout)
        .unwrap_or(limit)
        .min(limit);
    let first_byte_timeout = options
        .and_then(|value| value.first_byte_timeout)
        .unwrap_or(limit)
        .min(limit);
    let connect = async {
        let stream = TcpStream::connect((std::net::Ipv4Addr::LOCALHOST, port))
            .await
            .map_err(Error::Connect)?;
        tokio_rustls::TlsConnector::from(config)
            .connect(name, stream)
            .await
            .map_err(|error| {
                eprintln!("fixture-tls-error: {error:?}");
                Error::Tls(error)
            })
    };
    let stream = tokio::time::timeout(connect_timeout, connect)
        .await
        .map_err(|_| Error::ConnectionTimeout)??;
    let (mut sender, connection) =
        hyper::client::conn::http1::handshake(TokioIo::new(stream)).await?;
    *request.uri_mut() = http::Uri::builder()
        .path_and_query(
            request
                .uri()
                .path_and_query()
                .map_or("/", |value| value.as_str()),
        )
        .build()
        .map_err(|_| Error::HttpRequestUriInvalid)?;
    let response = async {
        tokio::time::timeout(first_byte_timeout, sender.send_request(request))
            .await
            .map_err(|_| Error::ConnectionReadTimeout)?
            .map_err(|error| {
                eprintln!("fixture-request-error: {error:?}");
                Error::from(error)
            })
    };
    let mut response = pin!(response);
    let mut connection = Box::pin(connection);
    // Drive the connection while awaiting headers, then transfer that same
    // connection future to Wasmtime so body resources retain its lifetime.
    let (response, completed) = tokio::select! {
        response = response.as_mut() => (response?, false),
        result = connection.as_mut() => {
            result.map_err(|error| {
                eprintln!("fixture-connection-error: {error:?}");
                Error::from(error)
            })?;
            (response.await?, true)
        },
    };
    Ok((
        response.map(|body| body.map_err(Error::from).boxed_unsync()),
        Box::new(async move {
            if !completed {
                connection.await?;
            }
            Ok(())
        }),
    ))
}

struct State {
    wasi: WasiCtx,
    http: WasiHttpCtx,
    table: ResourceTable,
    trust: Trust,
}

impl WasiView for State {
    fn ctx(&mut self) -> WasiCtxView<'_> {
        WasiCtxView {
            ctx: &mut self.wasi,
            table: &mut self.table,
        }
    }
}

impl WasiHttpView for State {
    fn http(&mut self) -> WasiHttpCtxView<'_> {
        WasiHttpCtxView {
            ctx: &mut self.http,
            table: &mut self.table,
            hooks: &mut self.trust,
        }
    }
}

struct Host {
    engine: Engine,
    component: Component,
    linker: Linker<State>,
    trust: Trust,
    p3: bool,
}

impl Host {
    async fn handle(
        &self,
        request: http::Request<hyper::body::Incoming>,
    ) -> wasmtime::Result<http::Response<Full<Bytes>>> {
        let mut store = Store::new(
            &self.engine,
            State {
                wasi: WasiCtx::builder().inherit_stderr().build(),
                http: WasiHttpCtx::new(),
                table: ResourceTable::new(),
                trust: self.trust.clone(),
            },
        );
        if self.p3 {
            let service = wasmtime_wasi_http::p3::bindings::Service::instantiate_async(
                &mut store,
                &self.component,
                &self.linker,
            )
            .await?;
            let (request, io) =
                wasmtime_wasi_http::p3::Request::from_http(&mut store.data_mut().trust, request);
            store
                .run_concurrent(async |accessor| {
                    let (response, ()) = tokio::try_join!(
                        async {
                            let response =
                                service.handle(accessor, request).await?.map_err(|error| {
                                    wasmtime::format_err!("guest HTTP error: {error:?}")
                                })?;
                            let response = accessor
                                .with(|store| response.into_http(store, async { Ok(()) }))?;
                            let (parts, body) = response.into_parts();
                            let body = body.collect().await?;
                            wasmtime::Result::Ok(http::Response::from_parts(
                                parts,
                                Full::new(body.to_bytes()),
                            ))
                        },
                        async { io.await.map_err(wasmtime::Error::from) }
                    )?;
                    Ok(response)
                })
                .await?
        } else {
            let proxy = wasmtime_wasi_http::p2::bindings::Proxy::instantiate_async(
                &mut store,
                &self.component,
                &self.linker,
            )
            .await?;
            let (sender, receiver) = tokio::sync::oneshot::channel();
            let request = store.data_mut().http().new_incoming_request(
                wasmtime_wasi_http::p2::bindings::http::types::Scheme::Http,
                request,
            )?;
            let output = store.data_mut().http().new_response_outparam(sender)?;
            let ((), response) = tokio::try_join!(
                async {
                    proxy
                        .wasi_http_incoming_handler()
                        .call_handle(&mut store, request, output)
                        .await
                },
                async {
                    let response = receiver.await??;
                    let (parts, body) = response.into_parts();
                    let body = body.collect().await?;
                    wasmtime::Result::Ok(http::Response::from_parts(
                        parts,
                        Full::new(body.to_bytes()),
                    ))
                }
            )?;
            Ok(response)
        }
    }
}

#[tokio::main]
async fn main() -> wasmtime::Result<()> {
    let arguments: Vec<_> = std::env::args().collect();
    if !matches!(arguments.len(), 4 | 6) || !matches!(arguments[1].as_str(), "p2" | "p3") {
        wasmtime::bail!(
            "usage: webcpp-beast-host-tls-fixture <p2|p3> <component> <root.der|none> [client.der client-key.der]"
        );
    }
    let mut roots = rustls::RootCertStore::empty();
    if arguments[3] != "none" {
        roots.add(CertificateDer::from(std::fs::read(&arguments[3])?))?;
    }
    let builder = rustls::ClientConfig::builder().with_root_certificates(roots);
    let config = if arguments.len() == 6 {
        builder.with_client_auth_cert(
            vec![CertificateDer::from(std::fs::read(&arguments[4])?)],
            PrivateKeyDer::Pkcs8(PrivatePkcs8KeyDer::from(std::fs::read(&arguments[5])?)),
        )?
    } else {
        builder.with_no_client_auth()
    };
    let trust = Trust {
        config: Arc::new(config),
    };
    let mut config = Config::new();
    config
        .wasm_component_model_async(true)
        .wasm_component_model_threading(true)
        .wasm_exceptions(true);
    let engine = Engine::new(&config)?;
    let component = Component::from_file(&engine, &arguments[2])?;
    let mut linker = Linker::new(&engine);
    wasmtime_wasi::p2::add_to_linker_async(&mut linker)?;
    wasmtime_wasi::p3::add_to_linker(&mut linker)?;
    wasmtime_wasi_http::p2::add_only_http_to_linker_async(&mut linker)?;
    wasmtime_wasi_http::p3::add_to_linker(&mut linker)?;
    let host = Arc::new(Host {
        engine,
        component,
        linker,
        trust,
        p3: arguments[1] == "p3",
    });
    let listener = TcpListener::bind((std::net::Ipv4Addr::LOCALHOST, 0)).await?;
    eprintln!("Serving HTTP on http://{}/", listener.local_addr()?);
    loop {
        let (connection, _) = listener.accept().await?;
        let host = host.clone();
        tokio::spawn(async move {
            let result = hyper::server::conn::http1::Builder::new()
                .serve_connection(
                    TokioIo::new(connection),
                    hyper::service::service_fn(move |request| {
                        let host = host.clone();
                        async move { host.handle(request).await }
                    }),
                )
                .await;
            if let Err(error) = result {
                eprintln!("fixture-http-error: {error:?}");
            }
        });
    }
}
