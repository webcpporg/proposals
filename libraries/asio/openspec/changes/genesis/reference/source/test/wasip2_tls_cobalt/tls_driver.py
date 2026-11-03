# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Drive the shared real TLS peer against the WASIp2 public consumer."""
from pathlib import Path
import subprocess
import sys

script = Path(__file__).resolve().parents[4] / 'tools/openssl/test/tls.py'
subprocess.run([sys.executable, str(script), sys.argv[-1], '--wasip2', *sys.argv[1:-1]],
               check=True, timeout=170)
