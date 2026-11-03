#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Run Beast exchanges with fresh local CA/server fixtures for TLS 1.2 and 1.3."""
from pathlib import Path
import runpy
import sys

driver = Path(__file__).resolve().parents[4] / 'tools/openssl/test/tls.py'
sys.argv = [str(driver), sys.argv[-1], '--state-test']
runpy.run_path(str(driver), run_name='__main__')
