#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""An external watchdog verifies idle waits without relying on a guest timer."""
import os
import select
import subprocess
import sys

for arguments in ([], ['reactor']):
    command = [os.environ.get('WASMTIME', 'wasmtime'), 'run', '-W', 'fuel=10000000',
               sys.argv[-1], *arguments]
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as guest:
        assert guest.stdout is not None
        try:
            assert select.select([guest.stdout], [], [], 10)[0], 'guest did not start'
            line = guest.stdout.readline()
            if line != b'ready\n':
                _, errors = guest.communicate(timeout=5)
                raise AssertionError(('guest startup', line, errors))
            try:
                output, errors = guest.communicate(timeout=0.3)
            except subprocess.TimeoutExpired:
                pass
            else:
                raise AssertionError(('idle run exited', guest.returncode, output, errors))
        finally:
            if guest.poll() is None:
                guest.kill()
            guest.wait()
print('Guard-only and reactor idle waits stayed blocked without exhausting fuel')
