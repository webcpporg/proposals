# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Real socket fixtures preserve request observations and separate client time budgets."""

import contextlib
import http.client
import io
import os
import socket
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import patch

import driver


class PausedSendSocket(socket.socket):
    def __init__(self, peer, server):
        super().__init__(fileno=peer.detach())
        self.server = server

    def sendall(self, data, flags=0):
        super().sendall(data, flags)
        headers = data.startswith(b"HTTP/")
        if headers == self.server.head:
            # The client can consume the final response bytes while the handler
            # is still inside sendall. No timing assumption controls this barrier.
            self.server.sent.set()
            if not self.server.release.wait(5):
                raise TimeoutError("test did not release the completed response write")


class PausedTcpPeer(driver.TcpPeer):
    def __init__(self, head):
        self.head = head
        self.sent = threading.Event()
        self.release = threading.Event()
        super().__init__(("127.0.0.1", 0))

    def get_request(self):
        peer, address = super().get_request()
        return PausedSendSocket(peer, self), address


class PeerObservationTest(unittest.TestCase):
    def test_completed_response_has_recorded_request(self):
        for method, body in (("GET", None), ("HEAD", None), ("POST", b"request body")):
            with self.subTest(method=method), PausedTcpPeer(method == "HEAD") as peer:
                worker = threading.Thread(target=peer.serve_forever, daemon=True)
                worker.start()
                connection = http.client.HTTPConnection("127.0.0.1", peer.socket.getsockname()[1],
                                                        timeout=3)
                try:
                    connection.request(method, "/observed", body=body,
                                       headers={"Connection": "close"})
                    response = connection.getresponse()
                    received = response.read()
                    self.assertEqual(response.status, 200)
                    expected = b"" if method == "HEAD" else body or driver.PAYLOAD
                    self.assertEqual(received, expected)
                    self.assertTrue(peer.sent.wait(3), "final sendall did not reach the barrier")
                    self.assertEqual(peer.exchanges, [{"method": method, "target": "/observed",
                                                       "body": body.decode() if body else ""}])
                finally:
                    peer.release.set()
                    connection.close()
                    peer.shutdown()
                    worker.join(timeout=3)
                    self.assertFalse(worker.is_alive(), "HTTP fixture worker did not stop")


class ClientBudgetTest(unittest.TestCase):
    def run_client(self, script, startup: float = 5, protocol: float = 1, tls=False):
        with tempfile.TemporaryDirectory(prefix="beast-client-budget-") as temporary:
            directory = Path(temporary)
            if tls:
                driver.certificates(directory)
            context = driver.tls_context(directory, server=True) if tls else None
            with driver.TcpPeer(("127.0.0.1", 0), context) as peer:
                worker = threading.Thread(target=peer.serve_forever, daemon=True)
                worker.start()
                command = [sys.executable, "-c", script, str(peer.socket.getsockname()[1])]
                try:
                    started = time.monotonic()
                    with contextlib.redirect_stderr(io.StringIO()):
                        try:
                            return driver.run_connected_client(
                                command, directory, os.environ.copy(), peer, startup, protocol)
                        except TimeoutError:
                            # A stalled protocol cannot borrow the larger startup
                            # allowance. The generous gap avoids a tight timing test.
                            if protocol < startup:
                                self.assertLess(time.monotonic() - started, startup)
                            raise
                finally:
                    peer.shutdown()
                    worker.join(timeout=3)
                    self.assertFalse(worker.is_alive(), "client fixture did not stop")

    def test_startup_does_not_consume_protocol_budget(self):
        # A real child deliberately starts later than the complete protocol budget.
        # The request still has to complete through the real HTTP socket fixture.
        result = self.run_client(
            "import http.client, sys, threading\n"
            "threading.Event().wait(1.5)\n"
            "peer = http.client.HTTPConnection('127.0.0.1', int(sys.argv[1]))\n"
            "peer.request('GET', '/fixture', headers={'Connection': 'close'})\n"
            "print(peer.getresponse().read().decode())\n")
        self.assertEqual(result.returncode, 0)
        self.assertIn(driver.PAYLOAD.strip(), result.stdout)

    def test_accepted_client_cannot_spend_unused_startup_budget(self):
        for tls in (False, True):
            with self.subTest(tls=tls):
                # With TLS enabled, this stalls before the handshake. Acceptance
                # must already have switched to the strict protocol deadline.
                with self.assertRaisesRegex(TimeoutError, "client protocol timed out"):
                    self.run_client(
                        "import socket, sys, threading\n"
                        "peer = socket.create_connection(('127.0.0.1', int(sys.argv[1])))\n"
                        "threading.Event().wait(30)\n", tls=tls)

    def test_client_without_connection_exhausts_startup_budget(self):
        with self.assertRaisesRegex(TimeoutError, "client startup timed out"):
            self.run_client("import threading; threading.Event().wait(30)",
                            startup=0.25, protocol=3)

    def test_timely_acceptance_survives_delayed_supervision(self):
        for delayed_publication in (False, True):
            with self.subTest(delayed_publication=delayed_publication), \
                    tempfile.TemporaryDirectory(prefix="beast-acceptance-") as temporary, \
                    driver.TcpPeer(("127.0.0.1", 0)) as peer:
                directory = Path(temporary)
                release = threading.Event()
                publish = peer.accepted.set
                real_clock = time.monotonic
                supervisor = threading.get_ident()
                calls = 0
                started = 0.0

                def publish_acceptance():
                    if delayed_publication and not release.wait(5):
                        raise TimeoutError("acceptance publication was not released")
                    publish()

                def clock():
                    nonlocal calls, started
                    if threading.get_ident() != supervisor:
                        return real_clock()
                    calls += 1
                    if calls == 1:
                        started = real_clock()
                    elif calls == 2:
                        # The loop has observed an unset Event. Let the actual
                        # HTTP child connect while this supervisor is paused.
                        (directory / "start").touch()
                        until = real_clock() + 3
                        while not peer.accepted_at or (not delayed_publication and
                                                       not (directory / "response").exists()):
                            self.assertLess(real_clock(), until, "HTTP peer did not accept")
                            threading.Event().wait(0.01)
                        self.assertLess(peer.accepted_at, started + 2)
                        threading.Event().wait(max(0, started + 2.1 - real_clock()))
                    elif calls == 3:
                        # The timestamp already exists while Event publication
                        # is delayed. Only entering the protocol phase releases it.
                        release.set()
                    return real_clock()

                script = (
                    "import http.client, pathlib, sys, threading\n"
                    "while not pathlib.Path('start').exists(): threading.Event().wait(0.01)\n"
                    "peer = http.client.HTTPConnection('127.0.0.1', int(sys.argv[1]))\n"
                    "peer.request('GET', '/fixture', headers={'Connection': 'close'})\n"
                    "body = peer.getresponse().read()\n"
                    "pathlib.Path('response').write_bytes(body)\n"
                    "print(body.decode())\n")
                command = [sys.executable, "-c", script, str(peer.socket.getsockname()[1])]
                worker = threading.Thread(target=peer.serve_forever, daemon=True)
                worker.start()
                try:
                    with patch.object(driver.time, "monotonic", clock), \
                            patch.object(peer.accepted, "set", publish_acceptance), \
                            contextlib.redirect_stderr(io.StringIO()):
                        result = driver.run_connected_client(
                            command, directory, os.environ.copy(), peer, 2, 5)
                    self.assertEqual(result.returncode, 0)
                    self.assertIn(driver.PAYLOAD.strip(), result.stdout)
                finally:
                    release.set()
                    peer.shutdown()
                    worker.join(timeout=3)
                    self.assertFalse(worker.is_alive(), "acceptance fixture did not stop")

    def test_early_exit_preserves_diagnostic(self):
        with self.assertRaisesRegex(RuntimeError, "status 7.*deliberate failure"):
            self.run_client("import sys; sys.stderr.write('deliberate failure'); sys.exit(7)")


class ServerCompilationTest(unittest.TestCase):
    def run_server(self, compile_delay: float = 0, compiler_failure=False, stalled=False):
        with tempfile.TemporaryDirectory(prefix="beast-server-compile-") as temporary:
            directory = Path(temporary)
            (directory / "index.html").write_bytes(driver.PAYLOAD)
            executable = directory / "server.wasm"
            executable.write_text(
                "import http.server, socket, sys, threading\n"
                "port = int(sys.argv[2])\n" +
                ("peer = socket.socket()\n"
                 "peer.bind(('127.0.0.1', port))\n"
                 "peer.listen()\n"
                 "connection, address = peer.accept()\n"
                 "threading.Event().wait(30)\n" if stalled else
                 "http.server.HTTPServer(('127.0.0.1', port), "
                 "http.server.SimpleHTTPRequestHandler).serve_forever()\n"))
            runtime = directory / "runtime.py"
            # Model the compiler boundary, then execute an actual HTTP server.
            # Compilation is intentionally slower than the readiness deadline.
            runtime.write_text(
                "import pathlib, runpy, shutil, sys, threading\n"
                "arguments = sys.argv[1:]\n"
                "if arguments[0] == 'compile':\n"
                f"    threading.Event().wait({compile_delay!r})\n"
                f"    if {compiler_failure!r}:\n"
                "        print('deliberate compiler failure', file=sys.stderr)\n"
                "        sys.exit(7)\n"
                "    destination = arguments[arguments.index('-o') + 1]\n"
                "    shutil.copyfile(arguments[-1], destination)\n"
                "    sys.exit(0)\n"
                "module = next(index for index, value in enumerate(arguments) "
                "if value.endswith(('.wasm', '.cwasm')))\n"
                "if arguments[module].endswith('.wasm'):\n"
                f"    threading.Event().wait({compile_delay!r})\n"
                "else:\n"
                "    assert '--allow-precompiled' in arguments\n"
                "pathlib.Path('guest-started').touch()\n"
                "sys.argv = arguments[module:]\n"
                "runpy.run_path(sys.argv[0], run_name='__main__')\n")
            row = next(row for row in driver.inventory.load()["applications"]
                       if row["id"] == "http-server-sync")
            real_connect = driver.connect
            real_popen = driver.subprocess.Popen

            def runtime_popen(command, *args, **kwargs):
                # Windows cannot execute a Python script through its shebang.
                # Keep real child processes, changing only this fixture's launcher.
                if command[0] == str(runtime):
                    command = [sys.executable, *command]
                return real_popen(command, *args, **kwargs)

            def bounded_connect(port, path, context, deadline, process):
                peer = real_connect(port, path, context, 0.75, process)
                peer.settimeout(0.25)
                return peer

            output = io.StringIO()
            started = time.monotonic()
            with patch.dict(os.environ, {"WASMTIME": str(runtime)}), \
                    patch.object(driver.subprocess, "Popen", runtime_popen), \
                    patch.object(driver, "connect", bounded_connect), \
                    contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                try:
                    return driver.drive_server(row, executable, directory, os.environ.copy(), [])
                except TimeoutError:
                    if stalled:
                        self.assertLess(time.monotonic() - started, 5)
                    raise
                except RuntimeError:
                    self.assertFalse((directory / "guest-started").exists())
                    raise

    def test_compilation_does_not_consume_listener_readiness(self):
        result = self.run_server(compile_delay=1.5)
        self.assertEqual(result["exchanges"], [{"status": 200,
                                              "body": "beast-application-fixture\n"}])

    def test_compiler_failure_preserves_diagnostic_without_starting_guest(self):
        with self.assertRaisesRegex(RuntimeError, "status 7.*deliberate compiler failure"):
            self.run_server(compiler_failure=True)

    def test_ready_listener_cannot_spend_unused_startup_budget(self):
        with self.assertRaises(TimeoutError):
            self.run_server(stalled=True)


if __name__ == "__main__":
    unittest.main()
