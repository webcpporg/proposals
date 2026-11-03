# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Exercise retained fast/small handlers and host-enforced mutual TLS."""

import http.client
import http.server
import json
import os
from pathlib import Path
import re
import ssl
import sys
import tempfile
import threading
from typing import cast

ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "tools/component"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import driver as fixtures
import serve


class Forward(http.server.BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_GET(self):
        gateway = cast(Gateway, self.server)
        gateway.forwarded += 1
        connection = http.client.HTTPConnection(*gateway.backend, timeout=5)
        try:
            connection.request("GET", self.path)
            response = connection.getresponse()
            body = response.read()
            self.send_response(response.status)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        finally:
            connection.close()


class Gateway(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, backend: tuple[str, int], directory: Path):
        self.backend = backend
        self.forwarded = 0
        super().__init__(("127.0.0.1", 0), Forward)
        context = fixtures.tls_context(directory, server=True)
        context.verify_mode = ssl.CERT_REQUIRED
        context.load_verify_locations(cafile=str(directory / "ca.crt"))
        self.socket = context.wrap_socket(self.socket, server_side=True)


def exchange(address, method, path, context=None):
    connection = (http.client.HTTPSConnection(*address, timeout=5, context=context)
                  if context else http.client.HTTPConnection(*address, timeout=5))
    try:
        connection.request(method, path)
        response = connection.getresponse()
        return response.status, response.read()
    finally:
        connection.close()


def mutual_tls(address: tuple[str, int], directory: Path):
    fixtures.certificates(directory)
    gateway = Gateway(address, directory)
    worker = threading.Thread(target=gateway.serve_forever, daemon=True)
    worker.start()
    frontend = ("localhost", gateway.server_port)
    try:
        uncredentialed = ssl.create_default_context(cafile=str(directory / "ca.crt"))
        try:
            exchange(frontend, "GET", "/", uncredentialed)
        except (ssl.SSLError, ConnectionError, http.client.RemoteDisconnected):
            pass
        else:
            raise AssertionError("host accepted a client without a certificate")
        assert gateway.forwarded == 0, "unauthenticated request reached the WASI handler"
        status, body = exchange(frontend, "GET", "/", fixtures.tls_context(directory))
        assert (status, body) == (200, b"Hello!"), (status, body)
        assert gateway.forwarded == 1
        return [{"status": status, "body": body.decode(), "mutual_tls": "host-enforced",
                 "missing_certificate": "rejected-before-handler"}]
    finally:
        gateway.shutdown()
        gateway.server_close()
        worker.join(timeout=5)
        assert not worker.is_alive(), "TLS frontend failed to stop"


def ordinary(name, address, payload):
    cases = (("GET", "/index.html", 200), ("GET", "/missing", 404),
             ("GET", "/../secret", 404), ("HEAD", "/index.html", 400),
             ("POST", "/index.html", 400)) if name == "http-server-fast" else (
                 ("GET", "/count", 200), ("GET", "/time", 200),
                 ("GET", "/missing", 404), ("POST", "/count", 400))
    observations = []
    for method, target, expected in cases:
        status, body = exchange(address, method, target)
        assert status == expected, (method, target, status, body)
        if status == 200 and name == "http-server-fast":
            assert body == payload, body
        elif target == "/count" and method == "GET":
            assert re.search(rb"There have been [1-9][0-9]* requests so far", body), body
        elif target == "/time":
            assert re.search(rb"current time is [1-9][0-9]* seconds", body), body
        observations.append({"method": method, "target": target, "status": status,
                             "body": body.decode()})
    return observations


def main():
    name, component = sys.argv[-2], Path(sys.argv[-1]).resolve()
    p3 = "wasip3" in str(component)
    runtime = os.environ.get("WASMTIME", "wasmtime")
    flags = ["-S", "cli,p3" if p3 else "cli"]
    if p3:
        flags += ["-W", "component-model-async"]
    payload = b"beast-application-fixture\n"
    with tempfile.TemporaryDirectory(prefix="beast-host-extra-") as temporary:
        directory = Path(temporary)
        (directory / "index.html").write_bytes(payload)
        flags += ["--dir", f"{directory}::."]
        server = serve.start(runtime, flags, component)
        try:
            address = serve.wait_until_serving(server, 10)
            observations = (mutual_tls(address, directory) if name == "doc-ssl-server"
                            else ordinary(name, address, payload))
        except BaseException:
            print(server.wrote(), file=sys.stderr)
            raise
        finally:
            serve.stop(server)
    print(json.dumps({"application": name, "backend": "wasi-http-p3" if p3 else "wasi-http-p2",
                      "exchanges": observations, "result": "passed"}, sort_keys=True))


if __name__ == "__main__":
    main()
