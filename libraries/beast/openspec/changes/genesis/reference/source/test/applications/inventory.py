# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Audit the immutable Boost.Beast executable and supporting-source inventory."""

import argparse
import hashlib
import json
import re
from pathlib import Path

LIBRARY = Path(__file__).resolve().parents[2]
UPSTREAM = LIBRARY / "example/upstream"
MANIFEST = LIBRARY / "example/applications/manifest.json"
COMMIT = "7c1e061f91e2ef542217b76286c314d006c0c8fc"
STATES = {"planned", "passed", "failed", "blocked", "platform-unavailable"}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def discover(root=UPSTREAM):
    applications = []
    for jam in sorted(root.rglob("Jamfile")):
        source = re.sub(r"#[^\n]*", "", jam.read_text())
        for target, sources in re.findall(r"\bexe\s+([\w-]+)\s*:\s*([^:;]+)", source):
            sources = [word for word in sources.split() if word.endswith(".cpp")]
            cmake = jam.with_name("CMakeLists.txt").read_text()
            matches = re.findall(r"add_executable\(\s*([\w-]+)([^)]*)\)", cmake)
            corresponding = [name for name, body in matches if all(s in body for s in sources)]
            if len(corresponding) != 1:
                raise ValueError(f"{target}: CMake source list does not match {sources}")
            entrypoints = [s for s in sources if re.search(
                r"\bmain\s*\(", (jam.parent / s).read_text())]
            if len(entrypoints) != 1:
                raise ValueError(f"{target}: expected one entrypoint, found {entrypoints}")
            path = jam.parent.relative_to(root).as_posix()
            text = "\n".join((jam.parent / s).read_text() for s in sources)
            applications.append({
                "id": target,
                "path": path,
                "cmake_target": corresponding[0],
                "sources": [f"{path}/{s}" for s in sources],
                "entrypoint": f"{path}/{entrypoints[0]}",
                "tls": "ssl" in path or "flex" in path,
                "stackful": "<boost/asio/spawn.hpp>" in text,
                "local_socket": "-local" in path,
                "server": "/server" in path or target in ("echo-op", "doc-ssl-server"),
                "dependencies": sorted({word for word, token in (
                    ("threads", "std::thread"), ("signals", "signal_set"),
                    ("system-executor", "system_executor"), ("stackful", "<boost/asio/spawn.hpp>"),
                    ("local-socket", "local::stream_protocol"), ("awaitable", "co_await"),
                ) if token in text}),
            })
    return applications


def load():
    return json.loads(MANIFEST.read_text())


def application_passed(record):
    return record.get("compile") == "passed" and record.get("run") == "passed"


def validate_result(record):
    for stage in ("compile", "run"):
        if record.get(stage) not in STATES:
            raise ValueError(f"invalid {stage} status: {record.get(stage)}")
        if record[stage] == "passed" and not record.get(f"{stage}_command"):
            raise ValueError(f"passed {stage} needs command evidence")
    if application_passed(record) and not record.get("evidence"):
        raise ValueError("passed application needs runtime evidence")


def validate(manifest, root=UPSTREAM):
    if manifest["upstream"]["commit"] != COMMIT:
        raise ValueError("upstream commit is not Boost.Beast 1.92.0")
    actual = {row["id"]: row for row in discover(root)}
    recorded = {row["id"]: row for row in manifest["applications"]}
    if len(recorded) != len(manifest["applications"]):
        raise ValueError("duplicate application row")
    missing = actual.keys() - recorded.keys()
    extra = recorded.keys() - actual.keys()
    if missing or extra:
        raise ValueError(f"missing applications: {sorted(missing)}; extra: {sorted(extra)}")
    if len(actual) != 53:
        raise ValueError(f"pinned upstream has {len(actual)} executables, expected 53")
    for name, row in actual.items():
        for key, value in row.items():
            if recorded[name].get(key) != value:
                raise ValueError(f"{name}: {key} disagrees with upstream")
    files = {p.relative_to(root).as_posix(): digest(p) for p in root.rglob("*") if p.is_file()}
    if files != manifest["upstream"]["sha256"]:
        changed = sorted(k for k in files.keys() | manifest["upstream"]["sha256"].keys()
                         if files.get(k) != manifest["upstream"]["sha256"].get(k))
        raise ValueError(f"immutable upstream corpus changed: {changed}")
    return len(actual)


def initialize():
    manifest = {
        "schema": 1,
        "upstream": {
            "repository": "https://github.com/boostorg/beast",
            "tag": "boost-1.92.0",
            "commit": COMMIT,
            "sha256": {p.relative_to(UPSTREAM).as_posix(): digest(p)
                       for p in sorted(UPSTREAM.rglob("*")) if p.is_file()},
        },
        "applications": discover(),
        "support": {
            "documentation_snippets": "doc/http_examples.hpp",
            "tls": ["common/root_certificates.hpp", "common/server_certificate.hpp"],
            "json": "http/client/body/json_body.hpp",
            "chat_html": "websocket/server/chat-multi/chat_client.html",
            "crawler": ["http/client/crawl/urls_large_data.hpp",
                        "http/client/crawl/urls_large_data.cpp"],
            "allocator": "http/server/fast/fields_alloc.hpp",
        },
        "normalization": {
            "http": "Compare status, application headers and decoded body. Keep raw output; "
                    "ignore Date, Server and transport framing only.",
            "websocket": "Compare text/binary mode, payload bytes and close outcome.",
            "crawler": "Ignore elapsed wall time only; compare all counters/status buckets.",
        },
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--initialize", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.initialize:
        initialize()
    count = validate(load())
    print(f"Boost.Beast {COMMIT}: {count} applications; all source hashes and entrypoints agree")


if __name__ == "__main__":
    main()
