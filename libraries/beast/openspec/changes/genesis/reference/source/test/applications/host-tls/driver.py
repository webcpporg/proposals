# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Prove P2/P3 HTTPS trust and hostname checks through real guest requests."""

from __future__ import annotations

import http.client
import http.server
import json
import os
import shutil
import ssl
import subprocess
import sys
import tempfile
import threading
from pathlib import Path

import trust

ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "tools/component"))
import serve


class Peer(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    requests: list[bytes] = []
    authenticated: list[bool] = []

    def do_POST(self) -> None:
        body = self.rfile.read(int(self.headers["Content-Length"]))
        assert self.path == "/tls-echo"
        assert self.headers["Content-Type"] == "application/octet-stream"
        self.reply(201, body)

    def do_GET(self) -> None:
        assert self.path == "/fixture"
        self.reply(200, b"beast-application-fixture\n")

    def reply(self, status: int, body: bytes) -> None:
        self.requests.append(body)
        assert isinstance(self.connection, ssl.SSLSocket)
        self.authenticated.append(bool(self.connection.getpeercert()))
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Transfer-Encoding", "chunked")
        self.send_header("Trailer", "x-tls-finished")
        self.end_headers()
        self.wfile.write(f"{len(body):x}\r\n".encode() + body +
                         b"\r\n0\r\nx-tls-finished: verified\r\n\r\n")

    def log_message(self, format: str, *args: object) -> None:
        pass


def start(fixture: str, component: Path, root: str, identity: list[str]) -> serve.Server:
    profile = "p3" if "wasip3" in str(component) else "p2"
    process = subprocess.Popen(
        [sys.executable, "-c", serve.KEEPER, fixture, profile, str(component), root, *identity],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        start_new_session=True)
    server = serve.Server(process)

    def read_errors() -> None:
        assert process.stderr is not None
        for raw in process.stderr:
            line = raw.decode("utf-8", errors="replace")
            server.errors.append(line)
            found = serve.SERVING.search(line)
            if found and server.address is None:
                server.address = (found.group(1), int(found.group(2)))
                server.said.set()
        server.said.set()

    def read_output() -> None:
        assert process.stdout is not None
        server.output.extend(process.stdout)

    for reader in (read_errors, read_output):
        thread = threading.Thread(target=reader, daemon=True)
        server.readers.append(thread)
        thread.start()
    return server


def check(component: Path, fixture: str, root: str, authority: str,
          expected_error: str | None, application: str, identity: list[str]) -> dict[str, str]:
    server = start(fixture, component, root, identity)
    payload = bytes(range(256)) * 256
    before = len(Peer.requests)
    modes = ("sync", "async") if application == "contract" else ("application",)
    try:
        address = serve.wait_until_serving(server, 30)
        for mode in modes:
            connection = http.client.HTTPConnection(*address, timeout=15)
            try:
                connection.request("POST", f"/{mode}", payload,
                                   {"x-peer": authority, "X-Beast-Peer": authority,
                                    "X-Beast-Scheme": "https"})
                response = connection.getresponse()
                body = response.read()
                if expected_error is None:
                    if application == "contract":
                        assert response.status == 201 and body == payload, (response.status, body)
                        assert response.getheader("x-tls-trailer") == "verified"
                    else:
                        assert response.status == 200 and body == b"completed=1", (
                            response.status, body)
                else:
                    assert response.status == 502, (response.status, body)
                    # An early TLS failure can close a pending guest upload
                    # before the response future exposes its TLS error code.
                    assert (b"wasi:http error-code" in body or
                            body.endswith(b"host HTTP stream closed")), body
            finally:
                connection.close()
        assert len(Peer.requests) - before == (len(modes) if expected_error is None else 0)
        if expected_error is None and identity:
            assert all(Peer.authenticated[before:]), "peer did not authenticate the client"
    except BaseException:
        print(server.wrote(), file=sys.stderr)
        raise
    finally:
        serve.stop(server)
    if expected_error is not None:
        assert server.standard_error().count(expected_error) == len(modes), server.wrote()
    else:
        assert "fixture-tls-error:" not in server.standard_error(), server.wrote()
    return {"case": expected_error or "trusted localhost", "result": "passed",
            "operations": ",".join(modes), "authority": authority}


def main() -> None:
    component = Path(sys.argv[-1]).resolve()
    application = sys.argv[-2] if len(sys.argv) > 2 else "contract"
    fixture = os.environ.get("WEBCPP_WASI_TLS_FIXTURE", "webcpp-beast-host-tls-fixture")
    if shutil.which(fixture) is None:
        raise SystemExit("build host-tls/Cargo.toml --locked and put the fixture on PATH, "
                         "or set WEBCPP_WASI_TLS_FIXTURE")
    with tempfile.TemporaryDirectory(prefix="webcpp-host-tls-") as temporary:
        certificates = trust.create_certificates(Path(temporary) / "certificates")
        peer = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Peer)
        peer.daemon_threads = True
        tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        tls.load_cert_chain(certificates["cert"], certificates["key"])
        identity = []
        if application == "doc-ssl-client":
            tls.load_verify_locations(certificates["ca"])
            tls.verify_mode = ssl.CERT_REQUIRED
            identity = [certificates["client_der"], certificates["client_key_der"]]
        peer.socket = tls.wrap_socket(peer.socket, server_side=True)
        worker = threading.Thread(target=peer.serve_forever, daemon=True)
        worker.start()
        try:
            correct = f"localhost:{peer.server_port}"
            wrong = f"127.0.0.1:{peer.server_port}"
            results = [check(component, fixture, certificates["der"], correct, None,
                             application, identity),
                       check(component, fixture, "none", correct, "UnknownIssuer",
                             application, identity),
                       check(component, fixture, certificates["der"], wrong, "NotValidForName",
                             application, identity)]
            if identity:
                results.append(check(component, fixture, certificates["der"], correct,
                                     "CertificateRequired", application, []))
        finally:
            peer.shutdown()
            peer.server_close()
            worker.join(timeout=3)
        print(json.dumps({"backend": "host-http-p3" if "wasip3" in str(component)
                         else "host-http-p2", "tls_verification": "normal rustls",
                         "runtime": "Wasmtime 50.0.1 isolated test embedding",
                         "application": application, "mutual_tls": bool(identity),
                         "exceptions": "off" if "exception-handling-off" in str(component)
                         else "on", "threading": "cooperative" if "threading-multi" in
                         str(component) else "single",
                         "cases": results}, sort_keys=True))


if __name__ == "__main__":
    main()
