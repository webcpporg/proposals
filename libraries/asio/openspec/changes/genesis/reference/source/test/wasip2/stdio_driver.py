#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Keep the guest's input open and empty while it checks readiness cancellation."""
import os
import subprocess
import sys
import tempfile

command = [os.environ.get('WASMTIME', 'wasmtime'), 'run', '-W', 'timeout=15s', sys.argv[-1]]
with tempfile.TemporaryFile() as output, tempfile.TemporaryFile() as diagnostics:
    with subprocess.Popen(command,
                          stdin=subprocess.PIPE, stdout=output, stderr=diagnostics) as guest:
        try:
            status = guest.wait(timeout=20)
        finally:
            if guest.poll() is None:
                guest.kill()
                guest.wait()
        diagnostics.seek(0)
        text = diagnostics.read().decode()
        assert status == 0, (status, text)
        output.seek(0)
        assert output.read() == b''
        print(text, end='')
print('WASIp2 supplied stream: timer cancellation, slot, destruction, '
      'allocator and executor passed')
