# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Run the retained upstream file handler through each WASI host HTTP ABI."""

import http.client
import json
import os
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "tools/component"))
import serve


def main():
    name = sys.argv[-2]
    component = Path(sys.argv[-1]).resolve()
    p3 = "wasip3" in str(component)
    runtime = os.environ.get("WASMTIME", "wasmtime")
    flags = ["-S", "cli,p3" if p3 else "cli"]
    if p3:
        flags += ["-W", "component-model-async"]
    payload = b"beast-application-fixture\n"
    with tempfile.TemporaryDirectory(prefix="beast-host-application-") as temporary:
        directory = Path(temporary)
        (directory / "index.html").write_bytes(payload)
        flags += ["--dir", f"{directory}::."]
        server = serve.start(str(runtime), flags, component)
        observations = []
        try:
            address = serve.wait_until_serving(server, 10)
            for method, path, expected in (("GET", "/", 200), ("GET", "/index.html", 200),
                                           ("HEAD", "/index.html", 200),
                                           ("GET", "/absent", 404),
                                           ("GET", "/../secret", 400),
                                           ("POST", "/index.html", 400)):
                connection = http.client.HTTPConnection(*address, timeout=5)
                try:
                    connection.request(method, path)
                    response = connection.getresponse()
                    body = response.read()
                    assert response.status == expected, (method, path, response.status, body)
                    if response.status == 200:
                        assert body == (b"" if method == "HEAD" else payload), body
                    observations.append({"method": method, "target": path,
                                         "status": response.status, "body": body.decode()})
                finally:
                    connection.close()
        except BaseException:
            print(server.wrote(), file=sys.stderr)
            raise
        finally:
            serve.stop(server)
    print(json.dumps({"application": name, "backend": "wasi-http-p3" if p3 else "wasi-http-p2",
                      "exchanges": observations, "result": "passed"}, sort_keys=True))


if __name__ == "__main__":
    main()
