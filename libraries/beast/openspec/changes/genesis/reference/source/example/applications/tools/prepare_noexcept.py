# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Preserve successful application flows with explicit fail-stop error semantics."""

import json
import re
import subprocess

import prepare


def masked(text):
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda match: " " * len(match[0]), text, flags=re.S)


def end_block(text, opening):
    depth = 1
    cursor = opening + 1
    while depth:
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    return cursor


def observe_normal_close(text):
    if "void do_session(" not in text or "ws.read(buffer);" not in text:
        return text
    text = text.replace('#include "failure.hpp"',
                        '#include "failure.hpp"\n#include "close_observation.hpp"')
    return text.replace("ws.read(buffer);", """beast::error_code read_error;
            ws.read(buffer, read_error);
            if (read_error == websocket::error::closed) {
                webcpp_fixture::observe_normal_close();
                return;
            }
            if (read_error) {
                webcpp_fixture::fail_stop();
            }""")


def adapt(text, browser=False):
    while match := re.search(r"\bcatch\s*\(", masked(text)):
        opening = text.index("{", match.end())
        end = end_block(masked(text), opening)
        text = text[:match.start()] + text[end:]
    text = re.sub(r"\btry\s*(?=\{)", "", text)
    text = re.sub(r"std::rethrow_exception\([^;]*\)", "webcpp_fixture::fail_stop()", text)
    while match := re.search(r"\bthrow\b", masked(text)):
        end = text.index(";", match.end())
        text = text[:match.start()] + "webcpp_fixture::fail_stop()" + text[end:]
    text = text.replace('#include "fixture.hpp"',
                        '#include "fixture.hpp"\n#include "failure.hpp"')
    if browser:
        text = text.replace("resolver.resolve(", "webcpp_fixture::resolve(resolver, ")
        text = text.replace("ctx.set_verify_mode(ssl::verify_peer);",
                            "{ boost::system::error_code verify_error; "
                            "ctx.set_verify_mode(ssl::verify_peer, verify_error); "
                            "if (verify_error) webcpp_fixture::fail_stop(); }")
    return observe_normal_close(text)


def generate(formatter):
    manifest = json.loads((prepare.APPLICATIONS / "manifest.json").read_text())
    variants = (("port", "noexcept-native"), ("wasip3", "noexcept-single"),
                ("browser", "noexcept-browser"))
    for original_variant, variant in variants:
        for original in sorted((prepare.APPLICATIONS / original_variant).rglob("*")):
            if original.suffix not in (".cpp", ".hpp"):
                continue
            relative = original.relative_to(prepare.APPLICATIONS / original_variant)
            destination = prepare.APPLICATIONS / variant / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            text = adapt(original.read_text(), original_variant == "browser")
            text = subprocess.run([formatter, f"--assume-filename={destination}"], input=text,
                                  text=True, capture_output=True, check=True).stdout
            prepare.write_changed(destination, text)
    rows = [row for row in manifest["applications"] if not row["stackful"]]
    project = prepare.APPLICATIONS.parents[1] / "test/applications/noexcept"
    project.mkdir(exist_ok=True)
    lines = [prepare.JAM_NOTICE, "import webcpp ;\nwebcpp.targets native wasip3 ;\n",
             "webcpp.original python3 : .. ;\n"]
    native, single, cooperative, checked = [], [], [], []
    for row in manifest["applications"]:
        modes = [("wasip3", "checked")]
        if not row["stackful"]:
            modes += [("noexcept-native", "threaded"), ("noexcept-single", "single")]
        if not row["local_socket"] and not row["stackful"]:
            modes.append(("noexcept-single", "cooperative"))
        for variant, mode in modes:
            name = f"{mode}-{row['id']}"
            groups = {"threaded": native, "single": single, "cooperative": cooperative,
                      "checked": checked}
            groups[mode].append(name)
            targets = ("wasip3" if mode == "cooperative" else "native" if
                       mode in ("threaded", "checked") or row["local_socket"] else "native wasip3")
            sources = " ".join(f"../../../example/applications/{variant}/" + source
                               for source in row["sources"])
            library = "tls" if row["tls"] else "sockets"
            threading = "multi" if mode in ("threaded", "cooperative") else "single"
            exceptions = "on" if mode == "checked" else "off"
            context = " <library>/webcpp//boost_context" if row["stackful"] else ""
            compile_budget = ("    <toolset>clang-wasip3:<webcpp-drive-timeout>90\n"
                              if "wasip3" in targets else "")
            lines.append(f"webcpp.drive {name} : {sources}\n"
                         f"  : <library>/webcpp/beast//{library}{context}\n"
                         f"    <exception-handling>{exceptions}\n"
                         f"{compile_budget}"
                         f"    <include>../../../example/applications/{variant}\n"
                         f"    <include>../../../example/applications <threading>{threading}\n"
                         "    <define>BOOST_ASIO_HAS_CO_AWAIT=1\n"
                         "    <define>BOOST_ASIO_HAS_STD_COROUTINE=1\n"
                         f"  : ../driver.py : {row['id']} : {targets} ;\n")
    lines.append("webcpp.lane applications-noexcept :\n  " + "\n  ".join(native) +
                 "\n  : native ;\n")
    lines.append("webcpp.lane applications-single-noexcept :\n  " + "\n  ".join(single) +
                 "\n  : native wasip3 ;\n")
    lines.append("webcpp.lane applications-cooperative-noexcept :\n  " +
                 "\n  ".join(cooperative) + "\n  : wasip3 ;\n")
    lines.append("webcpp.lane applications-single :\n  " + "\n  ".join(checked) +
                 "\n  : native ;\n")
    prepare.write_changed(project / "Jamfile", "".join(lines))
    browser = json.loads((prepare.APPLICATIONS / "browser/mapping.json").read_text())
    source_jam = (project.parent / "browser/Jamfile").read_text()
    source_jam = source_jam.replace("applications/browser/", "applications/noexcept-browser/")
    source_jam = source_jam.replace("<include>../../../example/applications/browser\n",
                                    "<include>../../../example/applications/noexcept-browser\n")
    source_jam = source_jam.replace("<exception-handling>on", "<exception-handling>off")
    source_jam = source_jam.replace(": run.mjs :", ": ../browser/run.mjs :")
    destination = project.parent / "browser-noexcept"
    destination.mkdir(exist_ok=True)
    prepare.write_changed(destination / "Jamfile", source_jam)
    (prepare.APPLICATIONS / "noexcept-single/mapping.json").write_text(json.dumps({
        "applications": [row["id"] for row in rows],
        "browser_applications": browser["applications"],
        "adaptations": ["Explicit throw/rethrow sites terminate with a diagnostic and abort.",
                        "Catch blocks are removed; error_code paths and successful flows retained.",
                        "Sync WebSocket servers observe normal close "
                        "through error_code and return.",
                        "Boost.Context stackful source remains blocked in exception-off modes.",
                        "Single-thread variants additionally apply explicit WASIp3 scheduling."],
    }, indent=2) + "\n")
