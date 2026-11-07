#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Exercise real host asynchronous HTTP operations and completion ownership."""
import http.client
import http.server
import os
from pathlib import Path
import sys
import socket
import threading
from host_driver import Peer

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'tools/component'))
import serve

component = Path(sys.argv[-1])
p3 = 'wasip3' in str(component)
flags = ['-S', 'cli,p3' if p3 else 'cli']
if p3:
    flags += ['-W', 'component-model-async', '-W', 'component-model-threading']
runtime = os.environ.get('WASMTIME', 'wasmtime')
peer = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Peer)
peer.daemon_threads = True
threading.Thread(target=peer.serve_forever, daemon=True).start()
server = serve.start(runtime, flags, component)
try:
    address = serve.wait_until_serving(server, 10)
    payload = bytes(range(256)) * 1024
    for route in ('/echo', '/pmr'):
        connection = http.client.HTTPConnection(*address, timeout=10)
        connection.request('POST', route, payload)
        response = connection.getresponse()
        assert response.status == 200 and response.read() == payload
        connection.close()
    connection = http.client.HTTPConnection(*address, timeout=10)
    connection.request('POST', '/client', payload, {'x-peer': f'127.0.0.1:{peer.server_port}'})
    response = connection.getresponse()
    assert response.status == 200 and response.read() == payload
    assert response.getheader('x-fast-first') == 'yes'
    connection.close()
    for path, expected, content in [('/client-timeout', 408, b'timed out'),
                                    ('/cancel-client', 408, b'cancelled'),
                                    ('/abandon', 200, b'abandoned')]:
        connection = http.client.HTTPConnection(*address, timeout=10)
        connection.request('POST', path, b'pending', {'x-peer': f'127.0.0.1:{peer.server_port}'})
        response = connection.getresponse()
        assert response.status == expected and content in response.read()
        connection.close()
    connection = http.client.HTTPConnection(*address, timeout=5)
    connection.request('GET', '/failed-output')
    try:
        response = connection.getresponse()
        assert response.status == 200
        response.read()
        raise AssertionError('failed committed asynchronous body was completed')
    except (http.client.IncompleteRead, http.client.RemoteDisconnected, ConnectionResetError):
        pass
    finally:
        connection.close()
    for path, text in [('/cancel', b'cancelled'), ('/timeout', b'timed out')]:
        with socket.create_connection(address, timeout=5) as channel:
            channel.sendall((f'POST {path} HTTP/1.1\r\nHost: host\r\n'
                             'Content-Length: 10000\r\n\r\nabc').encode())
            reply = http.client.HTTPResponse(channel)
            reply.begin()
            assert reply.status == 408 and text in reply.read()
    print('Async host: deferred token, associated executor/allocator, cancellation, deadline, '
          'PMR release inside completion, 256 KiB echo and concurrent fast/slow client '
          'with trailers passed')
except BaseException:
    print(server.wrote(), file=sys.stderr)
    raise
finally:
    serve.stop(server)
    peer.shutdown()
    peer.server_close()
