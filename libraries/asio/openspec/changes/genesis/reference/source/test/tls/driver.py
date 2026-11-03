# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Run the shared real-peer TLS fixtures from the Asio recurring test lane."""

from pathlib import Path
import subprocess
import sys


def main() -> None:
    mode, executable = sys.argv[1:]
    program = str(Path(executable).resolve())
    root = Path(__file__).resolve().parents[4]
    if mode in ('entropy', 'threads'):
        command = ([str(root / 'tools/boost/asio-wasip3/run.sh'), program]
                   if program.endswith('.wasm') else [program])
        subprocess.run(command, check=True, timeout=30)
        if mode == 'entropy':
            subprocess.run([*command, 'fail-entropy'], check=True, timeout=30)
        return
    options = {'client': ['--require-sni'], 'embedded': ['--embedded'],
               'cobalt': ['--embedded', '--require-sni'], 'state': ['--state-test'],
               'cancel': ['--state-test']}
    fixture = root / 'tools/openssl/test/tls.py'
    subprocess.run([sys.executable, str(fixture), program, *options[mode]],
                   check=True, timeout=150)


if __name__ == '__main__':
    main()
