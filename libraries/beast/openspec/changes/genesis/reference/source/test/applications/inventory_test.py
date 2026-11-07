# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""The acceptance denominator comes from upstream, never successful builds."""

import copy
import json
import runpy
import tempfile
import unittest
from pathlib import Path

import inventory

MATRIX = runpy.run_path(str(Path(__file__).with_name("matrix.py")))


class InventoryTest(unittest.TestCase):
    def test_every_upstream_entrypoint_is_covered(self):
        manifest = inventory.load()
        inventory.validate(manifest)
        self.assertEqual(len(manifest["applications"]), 53)

    def test_missing_application_fails_by_name(self):
        manifest = copy.deepcopy(inventory.load())
        removed = manifest["applications"].pop()
        with self.assertRaisesRegex(ValueError, removed["id"]):
            inventory.validate(manifest)

    def test_cmake_alias_is_explicit(self):
        rows = {row["id"]: row for row in inventory.load()["applications"]}
        self.assertEqual(rows["json_client"]["cmake_target"], "http-client-json")

    def test_awaitables_do_not_require_stackful_context(self):
        rows = {row["id"]: row for row in inventory.discover()}
        self.assertFalse(rows["http-client-awaitable"]["stackful"])
        self.assertTrue(rows["http-client-coro"]["stackful"])

    def test_compile_only_never_passes_execution(self):
        record = {"compile": "passed", "run": "planned", "evidence": []}
        self.assertFalse(inventory.application_passed(record))

    def test_pass_without_commands_and_evidence_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "evidence"):
            inventory.validate_result({"compile": "passed", "run": "passed"})

    def test_https_requires_all_trust_controls(self):
        record = {"application": "http-client-sync-ssl", "threading": "single",
                  "backend": "host-http-p2", "exceptions": "on", "cases": [],
                  "tls_verification": "normal rustls", "mutual_tls": False,
                  "runtime": "fixture"}
        with tempfile.TemporaryDirectory() as temporary:
            evidence = Path(temporary)
            (evidence / "host-tls-results.json").write_text(json.dumps({"results": [record]}))
            cells = {"http-client-sync-ssl": {}}
            mapping = {"applications": ["http-client-sync-ssl"], "adaptations": []}
            with self.assertRaisesRegex(ValueError, "Missing TLS controls"):
                MATRIX["add_host_tls"](cells, evidence, mapping)

    def test_mutual_tls_requires_missing_identity_rejection(self):
        record = {"application": "doc-ssl-client", "threading": "single",
                  "backend": "host-http-p2", "exceptions": "on", "mutual_tls": True,
                  "tls_verification": "normal rustls", "runtime": "fixture",
                  "cases": [{"case": case, "result": "passed"} for case in
                            ("trusted localhost", "UnknownIssuer", "NotValidForName")]}
        with tempfile.TemporaryDirectory() as temporary:
            evidence = Path(temporary)
            (evidence / "host-tls-results.json").write_text(json.dumps({"results": [record]}))
            cells = {"doc-ssl-client": {}}
            mapping = {"applications": ["doc-ssl-client"], "adaptations": []}
            with self.assertRaisesRegex(ValueError, "Missing TLS controls"):
                MATRIX["add_host_tls"](cells, evidence, mapping)

    def test_count_only_crawler_does_not_establish_workflow_parity(self):
        record = {"application": "http-crawl", "result": "passed", "completed": 3}
        with tempfile.TemporaryDirectory() as temporary:
            evidence = Path(temporary)
            (evidence / "host-crawl-green.log").write_text(json.dumps(record) + "\n")
            with self.assertRaisesRegex(ValueError, "Missing bounded host crawler"):
                MATRIX["add_host_crawl"]({"http-crawl": {}}, evidence, {"adaptations": []})

    def test_observed_startup_time_does_not_change_behavior(self):
        record = {"application": "echo-op", "result": "passed", "command": ["oracle"],
                  "exchanges": [{"echo": "payload"}], "startup_seconds": 0.02,
                  "startup_budget_seconds": 5, "exchange_timeout_seconds": 5}
        with tempfile.TemporaryDirectory() as temporary:
            oracle, port = (Path(temporary) / name for name in ("oracle.log", "port.log"))
            oracle.write_text(json.dumps(record) + "\n")
            record.update(command=["port"], startup_seconds=0.03)
            port.write_text(json.dumps(record) + "\n")
            self.assertEqual(MATRIX["normalized_run"](oracle), MATRIX["normalized_run"](port))
            for key, value in (("startup_budget_seconds", 15), ("exchange_timeout_seconds", 10),
                               ("exchanges", [{"echo": "different payload"}])):
                with self.subTest(key=key):
                    changed = {**record, key: value}
                    port.write_text(json.dumps(changed) + "\n")
                    self.assertNotEqual(MATRIX["normalized_run"](oracle),
                                        MATRIX["normalized_run"](port))


if __name__ == "__main__":
    unittest.main()
