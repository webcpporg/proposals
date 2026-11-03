#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Exercise actual supplied stdin/stdout channels without guest socket grants."""
import base64
import hashlib
import http.client
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from typing import cast
import socket

name, binary = sys.argv[-2], Path(sys.argv[-1]).resolve()


def command(directory):
    result = [str(binary)]
    if binary.suffix == '.wasm':
        result = [os.environ.get('WASMTIME', 'wasmtime'),
                  'run', '-S', 'cli', '--dir', f'{directory}::.']
        if 'wasip3' in str(binary):
            result += ['-S', 'p3', '-W', 'component-model-async', '-W', 'component-model-threading']
        result += [str(binary)]
    return result


class ResponseSource:
    def __init__(self, data):
        self.data = io.BytesIO(data)
    def makefile(self, *args):
        return self.data


def read_exact(pipe, count):
    result = b''
    while len(result) != count:
        chunk = pipe.read(count - len(result))
        assert chunk, ('unexpected EOF', len(result), count)
        result += chunk
    return result


def receive_frame(pipe):
    first, second = read_exact(pipe, 2)
    length = second & 127
    if length == 126:
        length = struct.unpack('!H', read_exact(pipe, 2))[0]
    elif length == 127:
        length = struct.unpack('!Q', read_exact(pipe, 8))[0]
    mask = read_exact(pipe, 4) if second & 128 else b''
    body = read_exact(pipe, length)
    if mask:
        body = bytes(value ^ mask[i % 4] for i, value in enumerate(body))
    return first, body


def send_frame(pipe, opcode, payload, masked):
    marker = 128 if masked else 0
    if len(payload) < 126:
        header = bytes([128 | opcode, marker | len(payload)])
    elif len(payload) < 65536:
        header = bytes([128 | opcode, marker | 126]) + struct.pack('!H', len(payload))
    else:
        header = bytes([128 | opcode, marker | 127]) + struct.pack('!Q', len(payload))
    if masked:
        mask = b'1234'
        header += mask
        payload = bytes(value ^ mask[i % 4] for i, value in enumerate(payload))
    pipe.write(header + payload)
    pipe.flush()


def receive_headers(pipe):
    result = b''
    while not result.endswith(b'\r\n\r\n'):
        result += read_exact(pipe, 1)
        assert len(result) < 65536
    return result


def websocket_exchange(cmd, directory, client):
    message = 'supplied-WebSocket-echo' * 512
    with subprocess.Popen(cmd + (['fixture', message] if client else []), cwd=directory,
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE) as peer:
        assert peer.stdin is not None and peer.stdout is not None
        try:
            if client:
                header = receive_headers(peer.stdout)
                assert header.startswith(b'GET / HTTP/1.1\r\n')
                fields = dict(line.split(b': ', 1) for line in header.strip().split(b'\r\n')[1:])
                accept = base64.b64encode(hashlib.sha1(fields[b'Sec-WebSocket-Key'] +
                    b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest())
                peer.stdin.write(b'HTTP/1.1 101 Switching Protocols\r\n'
                                 b'Upgrade: websocket\r\nConnection: upgrade\r\n'
                                 b'Sec-WebSocket-Accept: ' + accept + b'\r\n\r\n')
                peer.stdin.flush()
                first, body = receive_frame(peer.stdout)
                actual = body
                while not first & 128:
                    first, body = receive_frame(peer.stdout)
                    actual += body
                assert actual == message.encode()
                send_frame(peer.stdin, 1, actual, False)
                first, body = receive_frame(peer.stdout)
                assert first & 15 == 8
                send_frame(peer.stdin, 8, body, False)
            else:
                peer.stdin.write(b'GET / HTTP/1.1\r\nHost: fixture\r\nUpgrade: websocket\r\n'
                                 b'Connection: upgrade\r\nSec-WebSocket-Version: 13\r\n'
                                 b'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n')
                peer.stdin.flush()
                header = receive_headers(peer.stdout)
                assert header.startswith(b'HTTP/1.1 101 Switching Protocols\r\n')
                assert b'websocket-server-async-local' in header
                for opcode, payload in [(1, message.encode()), (2, bytes(range(256)) * 512)]:
                    send_frame(peer.stdin, opcode, payload, True)
                    first, body = receive_frame(peer.stdout)
                    assert first & 15 == opcode
                    actual = body
                    while not first & 128:
                        first, body = receive_frame(peer.stdout)
                        actual += body
                    assert actual == payload
                send_frame(peer.stdin, 9, b'ping', True)
                assert receive_frame(peer.stdout) == (138, b'ping')
                send_frame(peer.stdin, 8, struct.pack('!H', 1000), True)
                first, body = receive_frame(peer.stdout)
                assert first & 15 == 8 and body[:2] == struct.pack('!H', 1000)
            peer.stdin.close()
            peer.stdin = None
            remaining, diagnostics = peer.communicate(timeout=10)
            assert peer.returncode == 0, (peer.returncode, diagnostics)
            assert not remaining
            assert diagnostics == (message.encode() + b'\n' if client else b'')
        finally:
            if peer.poll() is None:
                peer.kill()
                peer.wait()


payload = b'beast-supplied-application\n' * 4096
with tempfile.TemporaryDirectory(prefix='beast-supplied-') as temporary:
    directory = Path(temporary)
    (directory / 'index.html').write_bytes(payload)
    cmd = command(directory)
    if name == 'http-client-async-local':
        incoming = (b'HTTP/1.1 200 OK\r\nContent-Length: ' +
                    str(len(payload)).encode() + b'\r\n\r\n' + payload)
        result = subprocess.run(cmd + ['fixture', '/index.html'], input=incoming,
                                cwd=directory, capture_output=True, timeout=15)
        assert result.returncode == 0, result.stderr
        assert result.stdout.startswith(b'GET /index.html HTTP/1.1\r\nHost: fixture\r\n')
        assert b'User-Agent: Boost.Beast/' in result.stdout
        assert result.stderr == b'status=200\n' + payload, result.stderr[:100]
        # The client preserves partial-message failure rather than succeeding early.
        bad = subprocess.run(cmd + ['fixture', '/'], input=incoming[:-10], cwd=directory,
                             capture_output=True, timeout=10)
        assert bad.returncode != 0 and b'partial message' in bad.stderr
    elif name == 'http-server-async-local':
        for method, target, status in [('GET', '/', 200), ('GET', '/index.html', 200),
                                      ('HEAD', '/index.html', 200), ('GET', '/absent', 404),
                                      ('GET', '/../private', 400), ('POST', '/', 400)]:
            request = (f'{method} {target} HTTP/1.1\r\nHost: fixture\r\n'
                       'Connection: close\r\n\r\n').encode()
            result = subprocess.run(cmd, input=request, cwd=directory,
                                    capture_output=True, timeout=15)
            assert result.returncode == 0, result.stderr
            source = cast(socket.socket, ResponseSource(result.stdout))
            response = http.client.HTTPResponse(source, method=method)
            response.begin()
            body = response.read()
            assert response.status == status, (method, target, result.stdout)
            if status == 200:
                assert body == (b'' if method == 'HEAD' else payload)
                assert response.getheader('Content-Length') == str(len(payload))
                assert response.getheader('Content-Type') == 'text/html'
            assert not result.stderr, result.stderr
        # Two pipelined requests prove retained overread and persistent request handling.
        pipeline = (b'GET / HTTP/1.1\r\nHost: fixture\r\n\r\n'
                    b'GET /absent HTTP/1.1\r\nHost: fixture\r\nConnection: close\r\n\r\n')
        result = subprocess.run(cmd, input=pipeline, cwd=directory, capture_output=True, timeout=15)
        assert result.returncode == 0 and result.stdout.count(b'HTTP/1.1 ') == 2
        assert b'HTTP/1.1 200 OK\r\n' in result.stdout
        assert b'HTTP/1.1 404 Not Found\r\n' in result.stdout
    elif name.startswith('websocket-'):
        websocket_exchange(cmd, directory, name == 'websocket-client-async-local')
    else:
        raise AssertionError(name)
print(json.dumps({'application': name, 'backend': 'supplied-stdio', 'result': 'passed'}))
