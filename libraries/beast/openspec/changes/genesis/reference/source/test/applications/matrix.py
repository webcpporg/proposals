# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Materialize all required application/profile cells without shrinking the denominator."""

import argparse
import json
import re
from pathlib import Path

import inventory

PROFILES = (
    "native-original-threads-eh-on", "native-port-threads-eh-on", "native-port-single-eh-on",
    "native-port-single-eh-off", "native-port-threads-eh-off",
    "wasip2-socket-eh-on", "wasip2-socket-eh-off",
    "wasip3-socket-single-eh-on", "wasip3-socket-single-eh-off",
    "wasip3-socket-cooperative-eh-on", "wasip3-socket-cooperative-eh-off",
    "wasip2-host-http-eh-on", "wasip2-host-http-eh-off",
    "wasip3-host-http-eh-on", "wasip3-host-http-eh-off",
    "wasip2-supplied-stream-eh-on", "wasip2-supplied-stream-eh-off",
    "wasip3-supplied-stream-eh-on", "wasip3-supplied-stream-eh-off",
    "browser-jspi-eh-on", "browser-jspi-eh-off",
    "browser-asyncify-eh-on", "browser-asyncify-eh-off",
)


def base_cell(row, profile):
    state = "planned"
    reason = "No measured evidence for this profile."
    if profile.startswith("wasip2-socket"):
        state = "blocked"
        reason = ("Asio P2 core is available; guest-socket application adapters still require "
                  "P2 integration and validation.")
        if row["tls"]:
            reason = ("Asio P2 provides guest OpenSSL; Beast guest-TLS applications still "
                      "require P2 integration and validation.")
        elif row["stackful"]:
            reason = "Boost.Context has no verified WASI backend."
    if "socket" in profile and row["local_socket"]:
        state, reason = "platform-unavailable", "WASI has no Unix-domain socket namespace."
    if profile.startswith("wasip3-socket") and row["stackful"]:
        state, reason = "blocked", "Boost.Context has no verified WASI backend."
    if profile.startswith("native") and profile.endswith("eh-off") and row["stackful"]:
        state = "blocked"
        reason = "Boost.Context application cancellation requires stack unwinding."
    if profile == "wasip3-socket-cooperative-eh-on":
        state, reason = "blocked", "SDK 34 cooperative sysroot has no usable exception runtime."
    if profile.startswith("browser"):
        if row["server"] or row["local_socket"]:
            state = "platform-unavailable"
            reason = "Browser has no TCP/Unix listener or raw channel."
        elif row["stackful"]:
            state, reason = "blocked", "Browser suspension is not a Boost.Context backend."
        elif row["id"] == "doc-ssl-client":
            state, reason = "blocked", "Browser cannot select the upstream mutual-TLS identity."
    if "host-http" in profile and (not row["server"] or row["path"].startswith("websocket/")):
        state, reason = "planned", "Requires an explicit host HTTP application counterpart."
    if "host-http" in profile and row["id"] == "echo-op":
        state, reason = "platform-unavailable", "Raw TCP echo has no HTTP host operation."
    if "host-http" in profile and (row["path"].startswith("websocket/") or
                                   row["id"] == "websocket-chat-multi"):
        state = "platform-unavailable"
        reason = "WASI HTTP cannot expose WebSocket upgrade streams."
    if "supplied-stream" in profile and not row["local_socket"]:
        state, reason = "platform-unavailable", "No original Unix-domain transport to replace."
    return {"compile": state, "run": state, "reason": reason, "adaptations": [], "evidence": []}


def normalized_run(path):
    for line in reversed(path.read_text().splitlines()):
        position = line.find('{"application"')
        if position >= 0:
            result = json.loads(line[position:])
            return {key: value for key, value in result.items()
                    if key not in ("command", "commands", "startup_seconds")}
    raise ValueError(f"No observed application result in {path}")


def create(evidence):
    manifest = inventory.load()
    inventory.validate(manifest)
    cells = {row["id"]: {profile: base_cell(row, profile) for profile in PROFILES}
             for row in manifest["applications"]}
    native_file = evidence / "native-applications/results.json"
    if native_file.exists():
        native = json.loads(native_file.read_text())
        for name, variants in native["applications"].items():
            if name not in cells:
                raise ValueError(f"Unknown measured application {name}")
            for variant, result in variants.items():
                inventory.validate_result(result)
                profile = "native-original-threads-eh-on" if variant == "oracle" \
                    else "native-port-threads-eh-on"
                cells[name][profile] = result
            if all(inventory.application_passed(variants.get(variant, {}))
                   for variant in ("oracle", "port")):
                oracle = normalized_run(evidence / f"native-applications/oracle/{name}.run.log")
                port = normalized_run(evidence / f"native-applications/port/{name}.run.log")
                if oracle != port:
                    raise ValueError(f"{name}: normalized native oracle and port differ")
                cells[name]["native-port-threads-eh-on"]["oracle_comparison"] = "passed"
    mapping = json.loads((inventory.LIBRARY / "example/applications/host/mapping.json").read_text())
    for abi in ("p2", "p3"):
        log = evidence / f"applications-host-{abi}.log"
        if not log.exists():
            continue
        observations = [json.loads(line) for line in log.read_text().splitlines()
                        if line.startswith('{"application"')]
        for result in observations:
            name = result["application"]
            if name not in mapping["applications"]:
                raise ValueError(f"Unmapped host application {name}")
            command = ("b2 -a --user-config=.local/user-config.jam toolset=clang-wasi" + abi +
                       " libs/beast/test/applications/host//" + name)
            cells[name][f"wasi{abi}-host-http-eh-on"] = {
                "compile": "passed", "run": "passed", "compile_command": command,
                "run_command": command, "evidence": [str(log)],
                "adaptations": [mapping["adaptation"]],
                "scope": "HTTP application handler; host owns listener/TLS; no WebSocket upgrade",
            }
    add_measurements(cells, evidence)
    return {"upstream_commit": inventory.COMMIT, "applications_count": len(cells),
            "profiles": PROFILES, "applications": cells}


def observations(log):
    result = []
    for line in log.read_text().splitlines():
        if line.startswith('{"application"'):
            result.append(json.loads(line))
    return result


def measured(command, log, adaptations, scope):
    return {"compile": "passed", "run": "passed", "compile_command": command,
            "run_command": command, "evidence": [log.name], "adaptations": adaptations,
            "scope": scope}


def add_measurements(cells, evidence):
    examples = inventory.LIBRARY / "example/applications"
    tls_clients = {row["id"] for row in inventory.load()["applications"]
                   if row["tls"] and not row["server"]}
    p3_map = json.loads((examples / "wasip3/mapping.json").read_text())
    browser_map = json.loads((examples / "browser/mapping.json").read_text())
    noeh_map = json.loads((examples / "noexcept-single/mapping.json").read_text())
    client_map = json.loads((examples / "host-clients/mapping.json").read_text())
    log = evidence / "applications-p3.log"
    if log.exists():
        for record in observations(log):
            name = record["application"]
            command = "b2 toolset=clang-wasip3 libs/beast/test/applications/wasip3//" + name
            cells[name]["wasip3-socket-single-eh-on"] = measured(
                command, log, p3_map["adaptations"], "Socket application counterpart")
    for filename in ("applications-native-noeh.log", "applications-p3-noeh.log",
                     "applications-native-single.log", "applications-native-single-fast.log",
                     "applications-p3-cooperative-noeh.log"):
        log = evidence / filename
        if not log.exists():
            continue
        for record in observations(log):
            name = record["application"]
            command = record.get("command", record.get("commands", [[]])[0])
            threaded = any("/threaded-" in part for part in command)
            native = "native" in filename
            profile = ("native-port-threads-eh-off" if threaded else
                       "native-port-single-eh-off") if native else "wasip3-socket-single-eh-off"
            target = ("threaded-" if threaded else "single-") + name
            if filename.startswith("applications-native-single"):
                profile, target = "native-port-single-eh-on", "checked-" + name
            elif "cooperative" in filename:
                profile, target = "wasip3-socket-cooperative-eh-off", "cooperative-" + name
            command = "b2 " + ("" if native else "toolset=clang-wasip3 ")
            command += "libs/beast/test/applications/noexcept//" + target
            adaptations = noeh_map["adaptations"]
            scope = "Fail-stop application counterpart"
            if profile == "native-port-single-eh-on":
                adaptations = p3_map["adaptations"]
                scope = "Single-executor application counterpart; original exception handling"
                if name == "websocket-server-fast":
                    adaptations += ["The synchronous listener uses the callback state machine "
                                    "on the same executor; three endpoint exchanges retained."]
            cells[name][profile] = measured(command, log, adaptations, scope)
    for filename in ("applications-browser.log", "applications-browser-both.log",
                     "applications-browser-both-green.log", "applications-browser-fix.log",
                     "browser-methods-green.log", "browser-profile-trust-green.log"):
        log = evidence / filename
        if not log.exists():
            continue
        passed_paths = re.findall(r"\*\*passed\*\* (\S+)", log.read_text())
        for record in observations(log):
            name = record["application"]
            backend = record.get("backend")
            if backend not in ("jspi", "asyncify"):
                continue
            for off in (False, True):
                directory = "browser-noexcept" if off else "browser"
                if not any(f"/applications/{directory}/run-{name}.test/" in path
                           for path in passed_paths):
                    continue
                profile = f"browser-{backend}-eh-" + ("off" if off else "on")
                command = "b2 toolset=emscripten libs/beast/test/applications/"
                command += f"{directory}//run-{name}"
                adaptations = browser_map["adaptations"] + (noeh_map["adaptations"] if off else [])
                cells[name][profile] = measured(command, log, adaptations,
                                                "Browser application counterpart")
                if "tls_fixture" in record:
                    cells[name][profile]["tls_fixture"] = record["tls_fixture"]
    for abi in ("p2", "p3"):
        for off in (False, True):
            suffix = "-noeh" if off else ""
            log = evidence / f"applications-host-clients-{abi}{suffix}.log"
            if not off and (evidence / f"applications-host-clients-{abi}-green.log").exists():
                log = evidence / f"applications-host-clients-{abi}-green.log"
            if off:
                log = evidence / f"applications-host-{abi}-noeh.log"
            if log.exists():
                for record in observations(log):
                    name = record["application"]
                    if name not in client_map["applications"]:
                        continue
                    command = f"b2 toolset=clang-wasi{abi} "
                    command += "exception-handling=off " if off else ""
                    command += "libs/beast/test/applications/host-clients//" + name
                    cell = measured(command, log, client_map["adaptations"],
                                    "Host HTTP payload counterpart")
                    if name == "http-crawl":
                        cell["run"] = "blocked"
                        cell["reason"] = (
                            "The count-only fixture did not measure the crawl workflow."
                        )
                    if name in tls_clients:
                        cell["run"] = "blocked"
                        cell["http_payload"] = "passed"
                        cell["tls_validation"] = "unverified"
                        cell["reason"] = "Selected Wasmtime fixture has no configured HTTPS trust."
                    cells[name][f"wasi{abi}-host-http-eh-" + ("off" if off else "on")] = cell
            if off:
                log = evidence / f"applications-host-{abi}-noeh.log"
                if log.exists():
                    for record in observations(log):
                        name = record["application"]
                        if name in client_map["applications"]:
                            continue
                        command = f"b2 toolset=clang-wasi{abi} exception-handling=off "
                        command += "libs/beast/test/applications/host//" + name
                        cells[name][f"wasi{abi}-host-http-eh-off"] = measured(
                            command, log, ["Host listener and original file handler variant."],
                            "Host HTTP handler counterpart")
        for filename in (f"applications-stdio-{abi}.log", f"applications-stdio-{abi}-noeh.log"):
            log = evidence / filename
            if not log.exists():
                continue
            records = observations(log)
            passed = re.findall(r"\*\*passed\*\* (\S+)", log.read_text())
            for record in records:
                name = record["application"]
                for path in passed:
                    if f"/{name}.test/" not in path:
                        continue
                    off = "/exception-handling-off/" in path
                    profile = f"wasi{abi}-supplied-stream-eh-" + ("off" if off else "on")
                    command = f"b2 toolset=clang-wasi{abi} "
                    command += "exception-handling=off " if off else ""
                    command += "libs/beast/test/applications/streams//" + name
                    cells[name][profile] = measured(command, log,
                        ["Original Unix transport replaced by supplied bidirectional stdio.",
                         ("P2 pollables preserve asynchronous partial operations." if
                          abi == "p2" and name.startswith("websocket-") else
                          "Explicit synchronous partial operations replace callbacks.")],
                        "Supplied-stream application counterpart")
    log = evidence / "host-extra-green.log"
    if log.exists():
        mapping = json.loads((examples / "host-extra/mapping.json").read_text())
        passed = re.findall(r"\*\*passed\*\* (\S+)", log.read_text())
        for record in observations(log):
            name = record["application"]
            if name not in {item["id"] for item in mapping["applications"]}:
                raise ValueError(f"Unmapped host-extra application {name}")
            abi = record["backend"].removeprefix("wasi-http-")
            for off in (False, True):
                if not any(f"/{name}.test/" in path and f"-wasi{abi}/" in path and
                           ("/exception-handling-off/" in path) == off for path in passed):
                    continue
                command = f"b2 toolset=clang-wasi{abi} "
                command += "exception-handling=off " if off else ""
                command += "libs/beast/test/applications/host-extra//" + name
                cell = measured(command, log, mapping["adaptations"],
                                "Host HTTP handler counterpart")
                if name == "doc-ssl-server":
                    cell["tls_validation"] = "host frontend mutual TLS measured"
                    cell["scope"] += "; host authenticates before invoking the guest handler"
                cells[name][f"wasi{abi}-host-http-eh-" + ("off" if off else "on")] = cell
    add_host_tls(cells, evidence, client_map)
    add_host_crawl(cells, evidence, client_map)


def add_host_crawl(cells, evidence, mapping):
    log = evidence / "host-crawl-green.log"
    if not log.exists():
        return
    for record in observations(log):
        expected = {"Timer": "0", "Resolve": "0", "Connect": "0", "Write": "0",
                    "Read": "1", "Success": "5"}
        requests = record.get("requests", [])
        destinations = {(item["authority"], item["target"]) for item in requests}
        authorities = {item["authority"] for item in requests}
        if (record.get("result") != "passed" or record.get("counters") != expected or
                record.get("maximum_active_requests") != 2 or
                record.get("statuses") != {"200": "2", "201": "1", "404": "1", "503": "1"} or
                len(requests) != 6 or len(destinations) != 6 or len(authorities) != 2 or
                not record.get("bounded_input_rejected")):
            raise ValueError("Missing bounded host crawler workflow evidence")
        abi = record["backend"].removeprefix("host-http-")
        exceptions = record["exceptions"]
        command = f"b2 toolset=clang-wasi{abi} exception-handling={exceptions} "
        command += "libs/beast/test/applications/host-clients//http-crawl"
        cell = measured(command, log, mapping["adaptations"],
                        "Two-worker host crawler with distinct targets and aggregate report")
        cell["observed_report"] = record
        cells["http-crawl"][f"wasi{abi}-host-http-eh-{exceptions}"] = cell


def add_host_tls(cells, evidence, mapping):
    log = evidence / "host-tls-results.json"
    if not log.exists():
        return
    results = json.loads(log.read_text())["results"]
    for record in sorted(results, key=lambda item: item["threading"] == "cooperative"):
        name = record["application"]
        if name == "contract":
            continue
        if name not in mapping["applications"]:
            raise ValueError(f"Unmapped TLS application {name}")
        required = {"trusted localhost", "UnknownIssuer", "NotValidForName"}
        if name == "doc-ssl-client":
            required.add("CertificateRequired")
        if not required.issubset({case["case"] for case in record["cases"]}):
            raise ValueError(f"Missing TLS controls: {name}")
        if name == "doc-ssl-client" and not record["mutual_tls"]:
            raise ValueError("Documentation client did not measure mutual TLS")
        if any(case["result"] != "passed" for case in record["cases"]):
            raise ValueError(f"Failed TLS application control: {name}")
        abi = record["backend"].removeprefix("host-http-")
        exceptions = record["exceptions"]
        command = f"b2 toolset=clang-wasi{abi} exception-handling={exceptions} "
        command += "threading=multi " if record["threading"] == "cooperative" else ""
        command += "libs/beast/test/applications/host-tls//tls-" + name
        profile = f"wasi{abi}-host-http-eh-{exceptions}"
        if record["threading"] == "cooperative":
            cells[name][profile].setdefault("additional_measurements", []).append({
                "configuration": "cooperative", "command": command,
                "evidence": record["evidence"], "cases": record["cases"]})
            continue
        cell = measured(command, log, mapping["adaptations"],
                        "HTTPS application counterpart with configured host trust")
        cell["tls_validation"] = record["tls_verification"]
        cell["tls_cases"] = record["cases"]
        cell["mutual_tls"] = record["mutual_tls"]
        cell["runtime"] = record["runtime"]
        cell["http_payload"] = "passed"
        cells[name][profile] = cell


def write_document(matrix, destination):
    lines = ["// Copyright (c) 2026 WebCpp.org\n//\n",
             "// Distributed under the Boost Software License, Version 1.0. (See\n",
             "// accompanying file LICENSE_1_0.txt or copy at\n",
             "// https://www.boost.org/LICENSE_1_0.txt)\n//\n",
             "// Generated by test/applications/matrix.py. Do not edit.\n\n",
             "[[application-profile-matrix]]\n=== Application profile evidence\n\n",
             "All 53 upstream executables remain in every profile. Each cell gives the ",
             "compile/run result independently. P = measured pass, F = measured failure, ",
             "B = blocked, U = unavailable, N = no measured result.\n\n",
             "A passing counterpart preserves the measured application operations. It does ",
             "not establish unchanged source, scheduling, DNS, TLS policy or failure semantics. ",
             "Native oracle comparison covers payloads/status/modes, excluding fixture addresses ",
             "and elapsed time. Local native measurements use macOS ARM64; other operating ",
             "systems require their CI runs. Raw results and commands are retained by the ",
             "drivers.\n\n",
             "[cols=\"1,5\",options=\"header\"]\n|===\n|Profile |Configuration\n"]
    for index, profile in enumerate(PROFILES, 1):
        lines.append(f"|P{index:02} |`{profile}`\n")
    lines.append("|===\n\n")
    codes = {"passed": "P", "failed": "F", "blocked": "B",
             "platform-unavailable": "U", "planned": "N"}
    # Separate groups keep the generated matrix readable while retaining every profile/row.
    for begin in range(0, len(PROFILES), 6):
        profiles = PROFILES[begin:begin + 6]
        lines.append('[cols="4,' + ','.join('1' for _ in profiles) +
                     '",options="header"]\n|===\n|Application ')
        lines.append(' '.join(f"|P{index + 1:02} C/R" for index in
                              range(begin, begin + len(profiles))) + "\n")
        for name, cells in matrix["applications"].items():
            lines.append("|`" + name + "` " + " ".join(
                "|" + codes[cells[profile]["compile"]] + "/" + codes[cells[profile]["run"]]
                for profile in profiles) + "\n")
        lines.append("|===\n\n")
    lines += ["==== Adaptations and limits\n\n",
              "* WASIp3 uses a fixture resolver posting the known local endpoint. DNS worker ",
              "threads remain unverified. Worker/system executors use an explicit event loop.\n",
              "* Host HTTP preserves file handlers or request/body operations. Host listeners, ",
              "framing and TLS are distinct from guest sockets. Six HTTPS client counterparts "
              "run through a configured Wasmtime host with an isolated CA, rejecting unknown "
              "issuers and trusted certificates for the wrong hostname. The documentation "
              "client additionally measures host-owned mutual TLS and missing-identity "
              "rejection. The documentation server measures a host mutual-TLS frontend.\n",
              "* The host crawler retains upstream report aggregation and two asynchronous "
              "workers over bounded authority/path inputs. Its fixture uses two peers, six "
              "paths, four response statuses, and a truncated-response failure followed by "
              "continued work. Host HTTP combines transport phases: failures count as Read; "
              "separate DNS/connect/write/timer phase counts are unmeasured.\n",
              "* Supplied streams replace Unix-domain addressing with caller-provided stdio. ",
              "P2 WebSocket counterparts retain asynchronous partial operations "
              "through Asio pollables; "
              "the other supplied counterparts use synchronous partial operations.\n",
              "* Browser fetch/WebSocket own DNS, framing and TLS. Guest CA lists, protocol ",
              "pinning and handshake User-Agent are removed explicitly. HTTPS fixtures install "
              "an ephemeral CA in an isolated Chrome profile. A fresh untrusted profile rejects "
              "the certificate; the trusted profile rejects its incorrect IP hostname. No "
              "certificate-error bypass or system trust-store modification is used.\n",
              "* Exception-off counterparts retain error-code paths and successful operations; ",
              "formerly throwing paths report failure and abort. Stackful Context remains "
              "blocked.\n",
              "* Asio P2 provides sockets and guest TLS, but the Beast guest-socket and "
              "guest-TLS application corpus still needs P2 integration and validation. "
              "Browser listeners and WASI Unix-domain socket addressing remain unavailable. ",
              "WASIp3 cooperative EH-on lacks the toolchain exception runtime.\n\n",
              "==== Reproduction\n\n[source,bash]\n----\n",
              "python3 libs/beast/test/applications/inventory.py --check\n",
              "python3 libs/beast/example/applications/tools/prepare.py --clang-format "
              "clang-format\n",
              "b2 libs/beast/test/applications//applications\n",
              "b2 toolset=clang-wasip3 libs/beast/test/applications/wasip3//applications-p3\n",
              "b2 toolset=emscripten libs/beast/test/applications/browser//applications-browser\n",
              "b2 toolset=clang-wasip2 libs/beast/test/applications/host//applications-host\n",
              "b2 toolset=clang-wasip2 "
              "libs/beast/test/applications/host-clients//applications-host-clients\n",
              "cargo build --locked --manifest-path "
              "libs/beast/test/applications/host-tls/Cargo.toml --target-dir .local/host-tls\n",
              "export PATH=\"$PWD/.local/host-tls/debug:$PATH\"\n",
              "b2 toolset=clang-wasip2 "
              "libs/beast/test/applications/host-tls//applications-host-tls\n",
              "b2 libs/beast/test/applications/noexcept//applications-noexcept\n",
              "b2 toolset=clang-wasip3 "
              "libs/beast/test/applications/noexcept//applications-single-noexcept\n",
              "b2 toolset=emscripten "
              "libs/beast/test/applications/browser-noexcept//applications-browser\n",
              "b2 toolset=clang-wasip3 "
              "libs/beast/test/applications/streams//applications-streams\n",
              "b2 toolset=clang-wasip3 "
              "libs/beast/test/applications/streams//applications-websocket-streams\n",
              "b2 libs/beast/test/applications/chat//applications-chat\n----\n\n",
              "Configure pinned toolchains with the standard B2 user configuration. Clear ",
              "CPATH/C_INCLUDE_PATH/CPLUS_INCLUDE_PATH. Runtime peers bind loopback ports, ",
              "wait for readiness, enforce deadlines, and reap every process.\n"]
    destination.write_text("".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--document", type=Path)
    parser.add_argument("--require", choices=PROFILES, action="append", default=[])
    args = parser.parse_args()
    matrix = create(args.evidence.resolve())
    args.output.write_text(json.dumps(matrix, indent=2) + "\n")
    if args.document:
        write_document(matrix, args.document)
    missing = [(name, profile) for name, profiles in matrix["applications"].items()
               for profile in args.require if not inventory.application_passed(profiles[profile])]
    if missing:
        raise SystemExit("Missing passing evidence: " + ", ".join(f"{a}/{p}" for a, p in missing))
    passed = sum(inventory.application_passed(cell)
                 for profiles in matrix["applications"].values() for cell in profiles.values())
    print(f"53 upstream applications, {len(PROFILES)} profiles, {passed} measured passing cells")


if __name__ == "__main__":
    main()
