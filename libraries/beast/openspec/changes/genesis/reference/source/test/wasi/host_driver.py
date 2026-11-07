#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Real host HTTP exchanges, bounded and independent of guest socket grants."""
from __future__ import annotations

import http.client
import http.server
import os
from pathlib import Path
import sys
import threading
import time

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'tools/component'))
import serve


class Peer(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def do_POST(self) -> None:
        if self.path == '/slow':
            time.sleep(0.4)
        upload = ''
        if self.headers.get('Transfer-Encoding') == 'chunked':
            body = b''
            while True:
                size = int(self.rfile.readline().strip(), 16)
                if size == 0:
                    while line := self.rfile.readline().strip():
                        if line.lower().startswith(b'x-upload:'):
                            upload = line.split(b':', 1)[1].strip().decode()
                    break
                body += self.rfile.read(size)
                assert self.rfile.read(2) == b'\r\n'
        else:
            body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
        self.send_response(201)
        self.send_header('Content-Type', 'text/plain')
        self.send_header('Transfer-Encoding', 'chunked')
        self.send_header('Trailer', 'x-reply')
        self.send_header('x-test', self.headers.get('x-test', ''))
        self.send_header('x-upload', upload)
        self.end_headers()
        try:
            self.wfile.write(f'{len(body):x}\r\n'.encode() + body +
                             b'\r\n0\r\nx-reply: finished\r\n\r\n')
        except (BrokenPipeError, ConnectionResetError):
            pass

    def log_message(self, format: str, *args: object) -> None:
        pass


def main() -> None:
    component = Path(sys.argv[-1])
    p3 = 'wasip3' in str(component)
    flags = ['-S', 'cli,p3' if p3 else 'cli']
    if p3:
        flags += ['-W', 'component-model-async']
    wasmtime = os.environ.get('WASMTIME', 'wasmtime')
    peer = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Peer)
    peer.daemon_threads = True
    thread = threading.Thread(target=peer.serve_forever, daemon=True)
    thread.start()
    server = serve.start(wasmtime, flags, component)
    try:
        address = serve.wait_until_serving(server, 10)
        def ask(path: str, body: bytes = b'', **headers: str) -> tuple[int, dict[str, str], bytes]:
            connection = http.client.HTTPConnection(*address, timeout=5)
            try:
                connection.request('POST', path, body, headers)
                response = connection.getresponse()
                return response.status, dict(response.getheaders()), response.read()
            finally:
                connection.close()
        payload = bytes(range(256)) * 1024
        status, headers, result = ask('/echo?raw=%2F', payload, **{'x-test': 'fields survive'})
        assert status == 200 and result == payload, (status, result[:300])
        assert headers['x-method'] == 'POST' and headers['x-target'] == '/echo?raw=%2F'
        assert headers['x-test'] == 'fields survive'
        peer_header = {'x-peer': f'127.0.0.1:{peer.server_port}'}
        status, headers, result = ask('/client', payload, **peer_header)
        assert status == 201 and result == payload, (status, result[:300])
        assert headers['x-trailer'] == 'finished', headers
        status, headers, result = ask('/client-trailers', payload, **peer_header)
        assert status == 201 and result == payload and headers['x-upload'] == 'uploaded', (
            status, headers, result[:200])
        status, _, result = ask('/client-timeout', b'timeout', **peer_header)
        assert status == 502 and b'timed out' in result, (status, result)
        status, _, _ = ask('/limit', b'x' * (1024 * 1024 + 1))
        assert status == 413, status
        for _ in range(10):
            assert ask('/again')[0] == 200
        print(f'{"P3" if p3 else "P2"}: incoming/outgoing 256 KiB, exposed headers, '
              'body limit and repeated requests passed')
    except BaseException:
        print(server.wrote(), file=sys.stderr)
        raise
    finally:
        serve.stop(server)
        peer.shutdown()
        peer.server_close()
        thread.join(timeout=2)


if __name__ == '__main__':
    main()
