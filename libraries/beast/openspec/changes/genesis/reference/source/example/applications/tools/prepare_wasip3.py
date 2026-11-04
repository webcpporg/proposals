# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Retain application operations with explicit WASIp3 fixture/scheduling substitutions."""

import difflib
import json
import re
import subprocess

import prepare


def adapt(relative, source):
    text = prepare.migrate(prepare.adapt_fixture(relative, source))
    text = text.replace('#include "fixture.hpp"',
                        '#include "fixture.hpp"\n#include "fixture_resolver.hpp"')
    text = text.replace("webcpp::asio::ip::tcp::resolver", "webcpp_fixture::resolver")
    text = text.replace("tcp::resolver", "webcpp_fixture::resolver")
    text = text.replace("#include <thread>", "")
    text = text.replace("#include <boost/asio.hpp>",
                        "#include <webcpp/asio.hpp>\n#include <boost/asio/strand.hpp>\n"
                        "#include <boost/asio/bind_executor.hpp>\n"
                        "#include <boost/asio/dispatch.hpp>\n"
                        "#include <boost/asio/awaitable.hpp>\n#include <boost/asio/co_spawn.hpp>\n"
                        "#include <boost/asio/as_tuple.hpp>\n#include <boost/asio/consign.hpp>\n"
                        "#include <boost/asio/bind_cancellation_slot.hpp>\n"
                        "#include <boost/asio/cancel_after.hpp>\n"
                        "#include <boost/asio/buffers_iterator.hpp>\n"
                        "#include <boost/asio/detached.hpp>")
    text = text.replace("auto const threads =", "[[maybe_unused]] auto const threads =")
    text = text.replace("auto const threads  =", "[[maybe_unused]] auto const threads =")
    text = text.replace("#include <boost/asio/signal_set.hpp>", "")
    text = re.sub(r"net::signal_set signals\(.*?\n\s*\}\);", "", text, flags=re.S)
    text = re.sub(r"(?m)^([ \t]*)std::vector<std::thread> v;.*?^\1ioc\.run\(\);",
                  r"\1ioc.run();", text, flags=re.S)
    text = re.sub(r"for\s*\(auto& t : v\)\s*t.join\(\);", "", text)
    if "system-executor" in relative.as_posix():
        text = text.replace("net::strand<net::system_executor>",
                            "net::strand<boost::asio::any_io_executor>")
        text = text.replace("session(ssl::context& ctx)",
                            "session(webcpp::asio::io_context& ioc, ssl::context& ctx)")
        text = re.sub(r"net::make_strand\(net::system_executor(?:\(\)|\{\})\)",
                      "net::make_strand(boost::asio::any_io_executor(ioc.get_executor()))", text)
        text = text.replace("std::make_shared<session>(ctx)",
                            "std::make_shared<session>(ioc, ctx)")
        text = text.replace("// The SSL context is required, and holds certificates",
                            "webcpp::asio::io_context ioc;\n"
                            "// The SSL context is required, and holds certificates")
        text = text.replace("net::query(net::system_executor(), net::execution::context).join();",
                            "ioc.run(); // lint-run: explicit single-thread application executor")
    if relative.name == "http_crawl.cpp":
        start = text.index("    // Create and launch the worker threads.")
        end = text.index("    std::cout <<", start)
        text = text[:start] + """
    // Each worker preserves the original crawl state machine on one explicit executor.
    for (int index = 0; index < threads; ++index)
        std::make_shared<worker>(report, ioc)->run();
    ioc.run(); // lint-run: advances every crawl worker and its aggregate report

""" + text[end:]
    if relative.name == "websocket_server_fast.cpp":
        begin = text.index("void\ndo_sync_session(")
        end = text.index("// Echoes back all received WebSocket messages", begin)
        text = text[:begin] + text[end:]
        text = re.sub(r"std::thread\(\s*beast::bind_front_handler\(\s*&do_sync_listen,.*?"
                      r"\.detach\(\);",
                      "std::make_shared<async_listener>(ioc, tcp::endpoint{address, port})->run();",
                      text, flags=re.S)
    if relative.name == "advanced_server_flex_awaitable.cpp":
        text = text.replace("auto signal_set = net::signal_set{ executor, SIGINT, SIGTERM };",
                            "auto deadline = net::steady_timer{executor};\n"
                            "deadline.expires_after(std::chrono::seconds(4));")
        text = text.replace("auto sig = co_await signal_set.async_wait();",
                            "co_await deadline.async_wait();")
        text = text.replace("if(sig == SIGINT)", "if(true)")
    if "int main(" in text or "\nmain(" in text:
        split = re.search(r"\bint\s+main\(", text)
        assert split is not None
        before, main = text[:split.start()], text[split.start():]
        if "/sync" in relative.as_posix() and "/server/" in relative.as_posix():
            if "http/server" in relative.as_posix():
                args = ("socket, ctx, doc_root" if "ssl" in relative.as_posix()
                        else "socket, doc_root")
                main = re.sub(r"std::thread\{std::bind\(.*?\)\}\.detach\(\);",
                              "do_session(" + args + ");", main, flags=re.S)
            else:
                args = ("std::move(socket), ctx" if "ssl" in relative.as_posix()
                        else "std::move(socket)")
                main = re.sub(r"std::thread\(.*?\)\.detach\(\);",
                              "do_session(" + args + ");", main, flags=re.S)
            main = main.replace("for(;;)", "for (unsigned accepted = 0; accepted < 1; ++accepted)")
        text = before + main
    return text


def generate(formatter):
    manifest = json.loads((prepare.APPLICATIONS / "manifest.json").read_text())
    deltas = []
    for original in sorted(prepare.UPSTREAM.rglob("*")):
        if original.suffix not in (".cpp", ".hpp"):
            continue
        relative = original.relative_to(prepare.UPSTREAM)
        source = original.read_text()
        destination = prepare.APPLICATIONS / "wasip3" / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        text = adapt(relative, source)
        formatted = subprocess.run([formatter, f"--assume-filename={destination}"],
                                   input=text, text=True, capture_output=True, check=True).stdout
        prepare.write_changed(destination, formatted)
        deltas.extend(difflib.unified_diff(source.splitlines(True), formatted.splitlines(True),
                                         fromfile=f"upstream/{relative}",
                                         tofile=f"wasip3/{relative}"))
    (prepare.APPLICATIONS / "wasip3.patch").write_text("".join(deltas))
    rows = [row for row in manifest["applications"] if not row["local_socket"] and
            not row["stackful"]]
    project = prepare.APPLICATIONS.parents[1] / "test/applications/wasip3"
    project.mkdir(exist_ok=True)
    lines = [prepare.JAM_NOTICE, "import webcpp ;\nwebcpp.targets wasip3 ;\n",
             "webcpp.original python3 : .. ;\n"]
    for row in rows:
        sources = " ".join("../../../example/applications/wasip3/" + source
                           for source in row["sources"])
        library = "tls" if row["tls"] else "sockets"
        lines.append(f"webcpp.drive {row['id']} : {sources}\n"
                     f"  : <library>/webcpp/beast//{library} <exception-handling>on\n"
                     "    <webcpp-drive-timeout>90\n"
                     "    <include>../../../example/applications/wasip3\n"
                     "    <include>../../../example/applications <threading>single\n"
                     "    <define>BOOST_ASIO_HAS_CO_AWAIT=1\n"
                     "    <define>BOOST_ASIO_HAS_STD_COROUTINE=1\n"
                     f"  : ../driver.py : {row['id']} : wasip3 ;\n")
    lines.append("webcpp.lane applications-p3 :\n  " +
                 "\n  ".join(row["id"] for row in rows) + "\n  : wasip3 ;\n")
    prepare.write_changed(project / "Jamfile", "".join(lines))
    (prepare.APPLICATIONS / "wasip3/mapping.json").write_text(json.dumps({
        "applications": [row["id"] for row in rows],
        "adaptations": [
            "Fixture resolver posts localhost; original asynchronous DNS remains blocked.",
            "Worker pools, system_executor and detached sessions share an explicit io_context.",
            "OS signal cancellation becomes an awaitable deadline or driver-controlled shutdown.",
            "Sync server accepts one complete connection; driver bounds asynchronous listeners.",
        ],
    }, indent=2) + "\n")
