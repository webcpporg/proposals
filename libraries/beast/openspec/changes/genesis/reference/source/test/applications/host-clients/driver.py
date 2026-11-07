# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Observe every host client request and response against a bounded owned peer."""

import http.client
import http.server
import importlib.abc
import importlib.util
import json
import os
import re
import sys
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "tools/component"))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import serve
specification = importlib.util.spec_from_file_location(
    "application_peers", Path(__file__).resolve().parent.parent / "driver.py")
assert specification is not None
assert isinstance(specification.loader, importlib.abc.InspectLoader)
driver = importlib.util.module_from_spec(specification)
specification.loader.exec_module(driver)


class CrawlState:
    def __init__(self):
        self.lock = threading.Lock()
        self.ready = threading.Event()
        self.active = 0
        self.maximum = 0
        self.exchanges = []


class CrawlPeer(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, state):
        self.state = state
        super().__init__(("127.0.0.1", 0), CrawlHandler)


class CrawlHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, format, *args):
        pass

    def do_GET(self):
        assert isinstance(self.server, CrawlPeer)
        state = self.server.state
        codes = {"/ok": 200, "/missing": 404, "/busy": 503, "/created": 201, "/again": 200}
        with state.lock:
            state.active += 1
            state.maximum = max(state.maximum, state.active)
            state.exchanges.append({"authority": self.headers["Host"], "target": self.path})
            if state.active == 2:
                state.ready.set()
        try:
            if self.path in ("/ok", "/missing"):
                assert state.ready.wait(3), "crawler did not start two independent workers"
            body = b"beast-application-fixture\n"
            self.send_response(codes.get(self.path, 200))
            self.send_header("Content-Length", str(len(body) + (7 if self.path == "/short" else 0)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(body)
            self.wfile.flush()
            self.close_connection = True
        finally:
            with state.lock:
                state.active -= 1


def check_crawl(connection, peers, state):
    first = f"localhost:{peers[0].server_port}"
    second = f"127.0.0.1:{peers[1].server_port}"
    destinations = [(first, "/ok"), (second, "/missing"), (first, "/busy"),
                    (second, "/short"), (first, "/created"), (second, "/again")]
    connection.request("POST", "/", body="".join(f"{host} {path}\n" for host, path in destinations),
                       headers={"X-Beast-Peer": first, "Content-Type": "text/plain"})
    response = connection.getresponse()
    body = response.read().decode()
    assert response.status == 200, (response.status, body)
    counters = dict(re.findall(r"^\s*(Timer|Resolve|Connect|Write|Read|Success)\s*:\s*(\d+)$",
                               body, re.M))
    expected = {"Timer": "0", "Resolve": "0", "Connect": "0", "Write": "0",
                "Read": "1", "Success": "5"}
    assert counters == expected, ("missing aggregate crawl report", body, state.exchanges)
    statuses = dict(re.findall(r"^\s*(\d{3}): (\d+) \(", body, re.M))
    assert statuses == {"200": "2", "201": "1", "404": "1", "503": "1"}, body
    assert response.getheader("X-Crawl-Completed") == "6", body
    assert response.getheader("X-Crawl-Workers") == "2", body
    assert state.maximum == 2, state.maximum
    assert sorted((entry["authority"], entry["target"]) for entry in state.exchanges) == \
        sorted(destinations), state.exchanges
    connection.request("POST", "/", body=f"{first} /ok\n" * 33)
    rejected = connection.getresponse()
    assert rejected.status == 400, (rejected.status, rejected.read())
    rejected.read()
    assert len(state.exchanges) == 6, state.exchanges
    return {"requests": state.exchanges, "counters": counters, "statuses": statuses,
            "maximum_active_requests": state.maximum, "bounded_input_rejected": True}


def crawl(component, runtime, flags):
    state = CrawlState()
    peers = [CrawlPeer(state), CrawlPeer(state)]
    workers = [threading.Thread(target=peer.serve_forever, daemon=True) for peer in peers]
    for worker in workers:
        worker.start()
    server = serve.start(runtime, flags, component)
    try:
        address = serve.wait_until_serving(server, 10)
        connection = http.client.HTTPConnection(*address, timeout=15)
        try:
            return check_crawl(connection, peers, state)
        finally:
            connection.close()
    except BaseException:
        print(server.wrote(), file=sys.stderr)
        raise
    finally:
        serve.stop(server)
        for peer in peers:
            peer.shutdown()
            peer.server_close()
        for worker in workers:
            worker.join(timeout=3)


def main():
    name = sys.argv[-2]
    component = Path(sys.argv[-1]).resolve()
    p3 = "wasip3" in str(component)
    runtime = os.environ.get("WASMTIME", "wasmtime")
    flags = ["-S", "cli,p3" if p3 else "cli", "-S", "inherit-network=y"]
    if p3:
        flags += ["-W", "component-model-async"]
    if name == "http-crawl":
        result = crawl(component, runtime, flags)
        print(json.dumps({"application": name, "backend": "host-http-p3" if p3 else "host-http-p2",
                          "exceptions": "off" if "exception-handling-off" in str(component)
                          else "on",
                          "result": "passed", **result}, sort_keys=True))
        return
    peer = driver.TcpPeer(("127.0.0.1", 0))
    worker = threading.Thread(target=peer.serve_forever, daemon=True)
    worker.start()
    server = serve.start(str(runtime), flags, component)
    try:
        address = serve.wait_until_serving(server, 10)
        connection = http.client.HTTPConnection(*address, timeout=5)
        try:
            connection.request("GET", "/", headers={
                "X-Beast-Peer": f"127.0.0.1:{peer.server_address[1]}"})
            response = connection.getresponse()
            body = response.read()
            expected = 6 if name == "http-client-methods" else 1
            assert response.status == 200, (response.status, body)
            assert body == f"completed={expected}".encode(), body
            assert len(peer.exchanges) == expected, peer.exchanges
            if name == "http-client-methods":
                assert [entry["method"] for entry in peer.exchanges] == [
                    "GET", "HEAD", "POST", "PUT", "PATCH", "DELETE"]
            if name == "json_client":
                assert json.loads(peer.exchanges[0]["body"]) == {
                    "type": "test", "content": "pure awesomeness"}
        finally:
            connection.close()
    except BaseException:
        print(server.wrote(), file=sys.stderr)
        raise
    finally:
        serve.stop(server)
        peer.shutdown()
        peer.server_close()
        worker.join(timeout=3)
    print(json.dumps({"application": name, "backend": "host-http-p3" if p3 else "host-http-p2",
                      "result": "passed", "requests": peer.exchanges}, sort_keys=True))


if __name__ == "__main__":
    main()
