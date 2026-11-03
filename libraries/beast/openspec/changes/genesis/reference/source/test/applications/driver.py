# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Bounded, local HTTP, WebSocket, TLS, Unix socket and chat application peers."""

import argparse
import base64
import contextlib
import hashlib
import http.client
import json
import os
import re
import signal
import socket
import socketserver
import ssl
import struct
import subprocess
import tempfile
import threading
import time
from pathlib import Path
from typing import cast

import inventory

PAYLOAD = b"beast-application-fixture\n"
WEBSOCKET_PAYLOAD = b"Beast fixture echo"


def receive_exact(peer, count):
    result = bytearray()
    while len(result) < count:
        data = peer.recv(count - len(result))
        if not data:
            raise EOFError(f"peer closed after {len(result)} of {count} bytes")
        result.extend(data)
    return bytes(result)


def receive_headers(peer):
    result = bytearray()
    while not result.endswith(b"\r\n\r\n"):
        result.extend(receive_exact(peer, 1))
        if len(result) > 65536:
            raise ValueError("headers exceed fixture limit")
    lines = bytes(result).decode("latin1").split("\r\n")
    fields = dict(line.split(": ", 1) for line in lines[1:] if ": " in line)
    return lines[0], {key.lower(): value for key, value in fields.items()}


def send_frame(peer, payload, opcode=1, masked=False):
    header = bytes([128 | opcode])
    flag = 128 if masked else 0
    if len(payload) < 126:
        header += bytes([flag | len(payload)])
    else:
        header += bytes([flag | 126]) + struct.pack("!H", len(payload))
    if masked:
        key = b"mask"
        payload = bytes(value ^ key[index % 4] for index, value in enumerate(payload))
        header += key
    peer.sendall(header + payload)


def receive_frame(peer):
    first, second = receive_exact(peer, 2)
    length = second & 127
    if length == 126:
        length = struct.unpack("!H", receive_exact(peer, 2))[0]
    elif length == 127:
        length = struct.unpack("!Q", receive_exact(peer, 8))[0]
    if length > 1048576:
        raise ValueError("WebSocket frame exceeds fixture limit")
    mask = receive_exact(peer, 4) if second & 128 else b""
    data = receive_exact(peer, length)
    if mask:
        data = bytes(value ^ mask[index % 4] for index, value in enumerate(data))
    return first & 15, data


def websocket_connect(peer):
    key = base64.b64encode(b"0123456789abcdef").decode()
    peer.sendall(("GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                  "Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
                  f"Sec-WebSocket-Key: {key}\r\n\r\n").encode())
    status, headers = receive_headers(peer)
    assert status.startswith("HTTP/1.1 101"), status
    expected = base64.b64encode(hashlib.sha1(
        (key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()).decode()
    assert headers["sec-websocket-accept"] == expected


class Peer(socketserver.BaseRequestHandler):
    def handle(self):
        peer = self.request
        peer.settimeout(5)
        try:
            while True:
                start, headers = receive_headers(peer)
                if headers.get("upgrade", "").lower() == "websocket":
                    accept = base64.b64encode(hashlib.sha1(
                        (headers["sec-websocket-key"] +
                         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest())
                    peer.sendall(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
                                 b"Connection: Upgrade\r\nSec-WebSocket-Accept: " +
                                 accept + b"\r\n\r\n")
                    while True:
                        opcode, data = receive_frame(peer)
                        send_frame(peer, data, opcode)
                        if opcode == 8:
                            if isinstance(peer, ssl.SSLSocket):
                                peer.unwrap()
                            else:
                                peer.shutdown(socket.SHUT_WR)
                                while peer.recv(1024):
                                    pass
                            return
                method, target, _ = start.split(" ", 2)
                size = int(headers.get("content-length", "0"))
                body = receive_exact(peer, size) if size else b""
                if headers.get("transfer-encoding") == "chunked":
                    chunks = []
                    while True:
                        line = bytearray()
                        while not line.endswith(b"\r\n"):
                            line.extend(receive_exact(peer, 1))
                        length = int(bytes(line).strip().split(b";", 1)[0], 16)
                        if length == 0:
                            assert receive_exact(peer, 2) == b"\r\n"
                            break
                        chunks.append(receive_exact(peer, length))
                        assert receive_exact(peer, 2) == b"\r\n"
                    body = b"".join(chunks)
                response = body if body else PAYLOAD
                media = "application/json" if headers.get("content-type") == "application/json" \
                    else "text/plain"
                peer.sendall(("HTTP/1.1 200 OK\r\nContent-Type: " + media +
                              "\r\nContent-Length: " + str(len(response)) +
                              "\r\nX-Beast-Fixture: local\r\n\r\n").encode())
                if method != "HEAD":
                    peer.sendall(response)
                cast(TcpPeer, self.server).exchanges.append({"method": method, "target": target,
                                                            "body": body.decode("utf8")})
                if headers.get("connection", "").lower() == "close":
                    return
        except (EOFError, ConnectionError, TimeoutError, ssl.SSLError):
            return


class TcpPeer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, address, context=None):
        self.context = context
        self.exchanges = []
        super().__init__(address, Peer)

    def get_request(self):
        peer, address = super().get_request()
        if self.context:
            peer = self.context.wrap_socket(peer, server_side=True)
        return peer, address


class UnixPeer(socketserver.ThreadingUnixStreamServer):
    daemon_threads = True

    def __init__(self, path):
        self.exchanges = []
        super().__init__(path, Peer)


def certificates(directory):
    def openssl(*arguments):
        subprocess.run(["openssl", *arguments], cwd=directory, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
    (directory / "ca.cnf").write_text(
        "[req]\ndistinguished_name=dn\n[dn]\nCN=Beast fixture CA\n[v3_ca]\n"
        "basicConstraints=critical,CA:true\nkeyUsage=critical,keyCertSign,cRLSign\n"
        "subjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid:always\n")
    openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2",
            "-subj", "/CN=Beast fixture CA", "-keyout", "ca.key", "-out", "ca.crt",
            "-config", "ca.cnf", "-extensions", "v3_ca")
    (directory / "extensions.cnf").write_text(
        "subjectAltName=DNS:localhost,IP:127.0.0.1\n"
        "extendedKeyUsage=serverAuth,clientAuth\n"
        "basicConstraints=critical,CA:false\nkeyUsage=critical,digitalSignature,keyEncipherment\n"
        "subjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid,issuer\n")
    for name in ("server", "client"):
        openssl("req", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost",
                "-keyout", f"{name}.key", "-out", f"{name}.csr")
        openssl("x509", "-req", "-in", f"{name}.csr", "-CA", "ca.crt", "-CAkey", "ca.key",
                "-CAcreateserial", "-days", "2", "-extfile", "extensions.cnf",
                "-out", f"{name}.crt")
    # Preserve the upstream public DH parameters, never its private identity.
    support = (inventory.UPSTREAM / "common/server_certificate.hpp").read_text()
    dh = support[support.index('"-----BEGIN DH PARAMETERS'):]
    dh = dh[:dh.index(";")]
    value = "".join(json.loads(part) for part in re.findall(r'"[^"\n]*"', dh))
    (directory / "dh4096.pem").write_text(value)


def tls_context(directory, server=False):
    if server:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(directory / "server.crt", directory / "server.key")
    else:
        context = ssl.create_default_context(cafile=str(directory / "ca.crt"))
        context.load_cert_chain(directory / "client.crt", directory / "client.key")
    return context


def connect(port, path=None, context=None, deadline=5, process=None):
    until = time.monotonic() + deadline
    while True:
        if process is not None and process.poll() is not None:
            raise RuntimeError(f"listener exited before readiness: status {process.returncode}")
        peer = socket.socket(socket.AF_UNIX if path else socket.AF_INET)
        peer.settimeout(5)
        try:
            peer.connect(str(path) if path else ("127.0.0.1", port))
            return context.wrap_socket(peer, server_hostname="localhost") if context else peer
        except (ConnectionRefusedError, FileNotFoundError) as error:
            peer.close()
            if time.monotonic() >= until:
                endpoint = path if path else f"127.0.0.1:{port}"
                raise TimeoutError(
                    f"listener {endpoint} was not ready within {deadline} seconds"
                ) from error
            time.sleep(0.02)


def available_port():
    with socket.socket() as peer:
        peer.bind(("127.0.0.1", 0))
        return peer.getsockname()[1]


def server_arguments(row, port, directory, path):
    name = row["id"]
    if name == "doc-ssl-server":
        return []
    if row["local_socket"]:
        return [str(path)] + ([str(directory)] if name.startswith("http-") else []) + ["1"]
    args = ["127.0.0.1", str(port)]
    if name == "echo-op" or name == "http-server-small":
        return args
    if name.startswith("http-") or name.startswith("advanced-") or name == "websocket-chat-multi":
        args.append(str(directory))
    args.append("1")
    if name == "http-server-fast":
        args.append("block")
    source = (inventory.UPSTREAM / row["entrypoint"]).read_text()
    count = re.search(r"argc\s*!=\s*(\d+)", source)
    if count and len(args) == int(count.group(1)):
        args.pop()
    return args


def application_command(executable, arguments, directory, environment):
    command = [str(executable), *arguments]
    if executable.suffix != ".wasm":
        return command
    runtime = os.environ.get("WASMTIME", "wasmtime")
    prefix = [str(runtime), "run", "-S", "p3", "-S", "inherit-network=y", "-S", "tcp=y",
              "-S", "allow-ip-name-lookup=y", "-W", "component-model-async=y",
              "-W", "component-model-threading=y", "-W", "timeout=15s",
              "--dir", str(directory), "--dir", f"{directory}::."]
    for key, value in environment.items():
        if key.startswith("WEBCPP_BEAST_FIXTURE_"):
            prefix += ["--env", f"{key}={value}"]
    return prefix + command


def drive_server(row, executable, directory, environment, command_prefix):
    port = available_port()
    path = directory / "server.sock" if row["local_socket"] else None
    environment["WEBCPP_BEAST_FIXTURE_PORT"] = str(port)
    observe_close = row["id"] in ("websocket-server-sync", "websocket-server-sync-ssl") and \
        executable.stem.startswith(("threaded-", "single-", "cooperative-"))
    closed = directory / "session-closed"
    if observe_close:
        environment["WEBCPP_BEAST_FIXTURE_CLOSED"] = str(closed)
    command = command_prefix + application_command(
        executable, server_arguments(row, port, directory, path), directory, environment)
    process = subprocess.Popen(command, cwd=directory, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    # Wasmtime compiles the component before it can listen. Keep that startup
    # allowance separate from each five-second protocol exchange.
    startup_budget = 15 if executable.suffix == ".wasm" else 5
    startup_started = time.monotonic()
    result = []
    try:
        context = tls_context(directory) if row["tls"] else None
        with contextlib.closing(connect(port, path, context, startup_budget, process)) as peer:
            startup_seconds = time.monotonic() - startup_started
            name = row["id"]
            if name == "echo-op":
                peer.sendall(PAYLOAD)
                assert receive_exact(peer, len(PAYLOAD)) == PAYLOAD
                process.wait(timeout=5)
                assert process.returncode == 0
                result.append({"echo": PAYLOAD.decode()})
            elif name.startswith("websocket-"):
                websocket_connect(peer)
                if name == "websocket-chat-multi":
                    with contextlib.closing(connect(port)) as second:
                        websocket_connect(second)
                        send_frame(peer, WEBSOCKET_PAYLOAD, masked=True)
                        assert receive_frame(peer) == (1, WEBSOCKET_PAYLOAD)
                        assert receive_frame(second) == (1, WEBSOCKET_PAYLOAD)
                        send_frame(second, b"participant two", masked=True)
                        assert receive_frame(peer) == (1, b"participant two")
                        assert receive_frame(second) == (1, b"participant two")
                        send_frame(second, struct.pack("!H", 1000), 8, True)
                        assert receive_frame(second)[0] == 8
                    result.append({"participants": 2, "broadcasts": 2})
                else:
                    for opcode in (1, 2):
                        send_frame(peer, WEBSOCKET_PAYLOAD, opcode, True)
                        assert receive_frame(peer) == (opcode, WEBSOCKET_PAYLOAD)
                    result.append({"text": True, "binary": True})
                send_frame(peer, struct.pack("!H", 1000), 8, True)
                assert receive_frame(peer)[0] == 8
            else:
                target = "/count" if name == "http-server-small" else "/index.html"
                if name == "doc-ssl-server":
                    target = "/"
                peer.sendall(f"GET {target} HTTP/1.1\r\nHost: localhost\r\n"
                             "Connection: close\r\n\r\n".encode())
                response = http.client.HTTPResponse(peer)
                response.begin()
                body = response.read()
                assert response.status == 200, (response.status, body)
                expected = b"Hello!" if name == "doc-ssl-server" else PAYLOAD
                if name == "http-server-small":
                    assert b"requests so far" in body, body
                else:
                    assert body == expected, body
                result.append({"status": response.status, "body": body.decode()})
            if isinstance(peer, ssl.SSLSocket):
                peer.unwrap()
        if observe_close:
            until = time.monotonic() + 5
            while not closed.exists():
                if process.poll() is not None:
                    assert closed.exists(), "server exited before observing normal close"
                    break
                if time.monotonic() >= until:
                    raise TimeoutError("server did not observe normal WebSocket close within 5s")
                time.sleep(0.01)
            assert closed.read_text() == "closed\n"
            if not executable.stem.startswith("threaded-"):
                assert process.wait(timeout=5) == 0
            result.append({"normal_close_observed": True})
        if row["id"] == "websocket-server-fast":
            for offset in (1, 2):
                with contextlib.closing(connect(port + offset)) as peer:
                    websocket_connect(peer)
                    send_frame(peer, WEBSOCKET_PAYLOAD, masked=True)
                    assert receive_frame(peer) == (1, WEBSOCKET_PAYLOAD)
                    send_frame(peer, struct.pack("!H", 1000), 8, True)
                    assert receive_frame(peer)[0] == 8
            result.append({"listeners": ["sync", "async", "stackful"]})
    finally:
        graceful = (row["id"] == "advanced-server-flex-awaitable" and
                    executable.suffix != ".wasm" and os.name != "nt" and
                    not executable.stem.startswith(("checked-", "single-")))
        if process.poll() is None:
            if graceful:
                process.send_signal(signal.SIGINT)
            else:
                process.terminate()
        try:
            output, errors = process.communicate(timeout=7 if graceful else 3)
        except subprocess.TimeoutExpired:
            process.kill()
            output, errors = process.communicate(timeout=3)
        print(output.decode(errors="replace"), end="")
        print(errors.decode(errors="replace"), end="")
        allowed = (0,) if graceful else (0, 1) if os.name == "nt" else (0, -signal.SIGTERM)
        assert process.returncode in allowed, (row["id"], process.returncode, errors)
    return {"command": command, "exchanges": result,
            "startup_seconds": startup_seconds, "startup_budget_seconds": startup_budget,
            "exchange_timeout_seconds": 5,
            "shutdown": "driver terminated and reaped listener"}


def drive_client(row, executable, directory, environment, command_prefix):
    path = directory / "peer.sock" if row["local_socket"] else None
    context = tls_context(directory, server=True) if row["tls"] else None
    server = UnixPeer(str(path)) if path else TcpPeer(("127.0.0.1", 0), context)
    port = 8080 if path else cast(TcpPeer, server).server_address[1]
    environment["WEBCPP_BEAST_FIXTURE_PORT"] = str(port)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    commands = []
    try:
        name = row["id"]
        if name in ("json_client", "doc-ssl-client"):
            variants = [[]]
        elif name == "http-client-methods":
            variants = [[method] for method in ("get", "head", "post", "put", "patch", "delete")]
        elif name == "http-crawl":
            variants = [["2"]]
        else:
            variants = [["localhost", str(port), WEBSOCKET_PAYLOAD.decode()
                         if name.startswith("websocket-") else "/fixture"]]
            if path:
                variants[0].insert(0, str(path))
        for arguments in variants:
            command = command_prefix + application_command(
                executable, arguments, directory, environment)
            result = subprocess.run(command, cwd=directory, env=environment,
                                    capture_output=True, timeout=8)
            print(result.stdout.decode(errors="replace"), end="")
            print(result.stderr.decode(errors="replace"), end="")
            assert result.returncode == 0, (name, result.returncode, result.stderr)
            if name.startswith("websocket-"):
                assert WEBSOCKET_PAYLOAD in result.stdout, result.stdout
            elif name == "http-crawl":
                assert b"Success : 3" in result.stdout, result.stdout
                assert b"Resolve : 0" in result.stdout, result.stdout
            elif name == "json_client":
                assert b"pure awesomeness" in result.stdout, result.stdout
            elif name != "http-client-methods":
                assert PAYLOAD.strip() in result.stdout, result.stdout
            commands.append(command)
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=3)
    return {"commands": commands, "requests": server.exchanges}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("application")
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    row = next(row for row in inventory.load()["applications"] if row["id"] == args.application)
    executable = args.executable.resolve()
    environment = os.environ.copy()
    for key in ("CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH"):
        environment.pop(key, None)
    with tempfile.TemporaryDirectory(prefix="beast-app-") as temporary:
        directory = Path(temporary)
        (directory / "index.html").write_bytes(PAYLOAD)
        if row["tls"]:
            certificates(directory)
        environment.update({
            "WEBCPP_BEAST_FIXTURE_HOST": "localhost",
            "WEBCPP_BEAST_FIXTURE_CA": str(directory / "ca.crt"),
            "WEBCPP_BEAST_FIXTURE_CERT": str(directory / "server.crt"),
            "WEBCPP_BEAST_FIXTURE_KEY": str(directory / "server.key"),
        })
        result = (drive_server if row["server"] else drive_client)(
            row, executable, directory, environment, [])
        print("\n" + json.dumps({"application": row["id"], "result": "passed", **result},
                               sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
