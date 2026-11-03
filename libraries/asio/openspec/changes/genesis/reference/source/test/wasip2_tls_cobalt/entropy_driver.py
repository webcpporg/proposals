# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Require actual host entropy, independent keys and fail-closed RNG failures."""
import os
import subprocess
import sys

launcher = [os.environ.get('WASMTIME', 'wasmtime'), 'run', '-W', 'timeout=15s', sys.argv[-1]]
keys = set()
for _ in range(3):
    result = subprocess.run(launcher, capture_output=True, text=True, timeout=20, check=True)
    print(result.stdout, end='')
    assert 'PASS random, reseed, EC P-256, ECDSA, tamper rejection:' in result.stdout
    key = next(line for line in result.stdout.splitlines() if line.startswith('PUBLIC_KEY='))
    assert key not in keys, 'Independent processes generated the same public key'
    keys.add(key)
result = subprocess.run([*launcher, 'fail-entropy'], capture_output=True, text=True,
                        timeout=20, check=True)
print(result.stdout, end='')
assert 'PASS fail-closed:' in result.stdout, result.stdout + result.stderr
