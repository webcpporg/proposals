#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Drains both protocol pipes concurrently, with an outer deadline."""
from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys

component = Path(sys.argv[-1])
command = [str(component)]
if component.suffix == '.wasm':
    runtime = os.environ.get('WASMTIME', 'wasmtime')
    command = [runtime, 'run', '-S', 'cli']
    if 'wasip3' in str(component):
        command += ['-S', 'p3', '-W', 'component-model-async']
    command.append(str(component))
body = bytes(range(256)) * 1024
request = (b'POST /echo HTTP/1.1\r\nHost: stream\r\nContent-Length: ' +
           str(len(body)).encode() + b'\r\n\r\n' + body)
result = subprocess.run(command, input=request, capture_output=True, timeout=10, check=False)
assert result.returncode == 0, (result.returncode, result.stderr)
header, actual = result.stdout.split(b'\r\n\r\n', 1)
assert header.startswith(b'HTTP/1.1 200 OK\r\n'), header
assert actual == body, (len(actual), len(body))
assert not result.stderr, result.stderr
print('Supplied stdin/stdout: 256 KiB HTTP exchange passed; diagnostics separated')

# Keep stdout deliberately undrained until the operation reports cancellation.
with subprocess.Popen(command + ['--cancel-output'], stdin=subprocess.DEVNULL,
                      stdout=subprocess.PIPE, stderr=subprocess.PIPE) as pending:
    try:
        # Wasmtime's CLI stdout may block inside a host write. Resume draining after
        # a bounded pause so guest cancellation can be observed without hanging.
        import time
        time.sleep(0.2)
        output, diagnostics = pending.communicate(timeout=5)
        assert pending.returncode == 0
        assert diagnostics == b'cancelled blocked output\n'
        assert len(output) <= 1024 * 1024, len(output)
    finally:
        if pending.poll() is None:
            pending.kill()
            pending.wait()
# A truncated message remains a parser failure, rather than a successful empty request.
truncated = subprocess.run(command, input=b'POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\nabc',
                           capture_output=True, timeout=5, check=False)
assert truncated.returncode == 1 and b'partial message' in truncated.stderr, truncated
print('Blocked output cancellation and partial-message EOF passed')

# Closing the output peer must report a write failure, never terminate the process by SIGPIPE.
if component.suffix != '.wasm':
    for mode in ([], ['--preserve-mask'], ['--preserve-signal']):
        with subprocess.Popen(command + mode, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as closed:
            assert closed.stdin is not None and closed.stdout is not None
            assert closed.stderr is not None
            closed.stdout.close()
            closed.stdin.write(b'POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc')
            closed.stdin.close()
            code = closed.wait(timeout=5)
            diagnostics = closed.stderr.read()
            assert code == 2 and diagnostics.startswith(b'write: '), (code, diagnostics)
    print('Closed output peer reports an error without SIGPIPE termination')
