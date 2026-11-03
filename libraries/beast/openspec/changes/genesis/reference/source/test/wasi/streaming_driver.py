#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Probes trailers, partial-body abandonment and cancellation on the actual HTTP host."""
from __future__ import annotations

import http.client
import os
from pathlib import Path
import socket
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'tools/component'))
import serve

component = Path(sys.argv[-1])
p3 = 'wasip3' in str(component)
flags = ['-S', 'cli,p3' if p3 else 'cli'] + (['-W', 'component-model-async'] if p3 else [])
runtime = os.environ.get('WASMTIME', 'wasmtime')
server = serve.start(runtime, flags, component)
try:
    address = serve.wait_until_serving(server, 10)
    with socket.create_connection(address, timeout=5) as channel:
        channel.sendall(b'POST /trailers HTTP/1.1\r\nHost: host\r\nTE: trailers\r\n'
                        b'Connection: close\r\nTransfer-Encoding: chunked\r\n'
                        b'Trailer: x-upload\r\n\r\n3\r\nabc\r\n3\r\ndef\r\n'
                        b'0\r\nx-upload: received\r\n\r\n')
        raw = b''
        while chunk := channel.recv(4096):
            raw += chunk
        assert b'HTTP/1.1 200' in raw, raw
        assert b'x-request-trailer: received' in raw, raw
        assert b'abcdef' in raw and b'x-complete: finished' in raw, raw
    connection = http.client.HTTPConnection(*address, timeout=5)
    connection.request('GET', '/fail-after-commit')
    try:
        response = connection.getresponse()
        assert response.status == 200
        response.read()
        raise AssertionError('truncated committed body was silently completed')
    except (http.client.IncompleteRead, http.client.RemoteDisconnected, ConnectionResetError):
        pass
    finally:
        connection.close()
    connection = http.client.HTTPConnection(*address, timeout=5)
    connection.request('POST', '/cancel-read', b'x' * 65536)
    response = connection.getresponse()
    assert response.status == 408, (response.status, response.read())
    assert b'cancelled' in response.read()
    connection.close()
    with socket.create_connection(address, timeout=5) as channel:
        channel.sendall(b'POST /read-timeout HTTP/1.1\r\nHost: host\r\nContent-Length: 99\r\n\r\na')
        response = http.client.HTTPResponse(channel)
        response.begin()
        assert response.status == 408, (response.status, response.read())
        assert b'timed out' in response.read()
    assert 'cannot drop' not in server.standard_error(), server.wrote()
    print(f'{"P3" if p3 else "P2"}: request/response trailers, failed committed body, '
          'mid-transfer cancellation and deadline passed')
except BaseException:
    print(server.wrote(), file=sys.stderr)
    raise
finally:
    serve.stop(server)
