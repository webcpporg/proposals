# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Keep original client state machines over browser-owned WebSocket and fetch."""

import difflib
import json
import re
import subprocess

import prepare
import prepare_wasip3


def closing(text, start, left="(", right=")"):
    depth = 1
    cursor = start + 1
    while depth:
        depth += (text[cursor] == left) - (text[cursor] == right)
        cursor += 1
    return cursor


def adapt(relative, source):
    text = prepare_wasip3.adapt(relative, source)
    text = text.replace('#include "fixture_resolver.hpp"',
                        "#include <boost/asio/use_awaitable.hpp>")
    text = text.replace("webcpp_fixture::resolver", "webcpp::asio::ip::tcp::resolver")
    text = text.replace("#include <boost/asio/ssl.hpp>", "#include <webcpp/asio/ssl.hpp>")
    text = text.replace("#include <boost/beast/websocket/ssl.hpp>", "")
    text = text.replace('#include "common/root_certificates.hpp"', "")
    text = re.sub(r"namespace ssl\s*= boost::asio::ssl", "namespace ssl = webcpp::asio::ssl", text)
    text = re.sub(r"namespace ssl\s*= net::ssl", "namespace ssl = webcpp::asio::ssl", text)
    text = text.replace("ssl::stream<", "beast::ssl_stream<")
    text = text.replace("beast::ssl_stream<tcp::socket>", "beast::ssl_stream<beast::tcp_stream>")
    text = text.replace("net::connect(beast::get_lowest_layer(ws), results)",
                        "beast::get_lowest_layer(ws).connect(results)")
    text = re.sub(r"[\w.]+\.set\(\s*http::field::user_agent,.*?\);", "", text, flags=re.S)
    text = text.replace("ssl::context::tlsv12_client", "ssl::context::tls_client")
    text = text.replace("load_root_certificates(ctx);",
                        "// Browser trust store owns TLS authorities.")
    text = text.replace("websocket::stream<tcp::socket>", "websocket::stream<beast::tcp_stream>")
    text = text.replace("net::connect(ws.next_layer(), results)",
                        "ws.next_layer().connect(results)")
    while match := re.search(r"if\s*\(\s*!\s*SSL_set_tlsext_host_name", text):
        call = text.index("(", text.index("SSL_set_tlsext_host_name", match.start()))
        call_end = closing(text, call)
        stream, host = text[call + 1:call_end - 1].split(",", 1)
        stream = stream.replace(".native_handle()", "").strip()
        start = text.index("{", call_end)
        end = closing(text, start, "{", "}")
        replacement = f"beast::set_server_name({stream}, {host.strip()}).value();"
        text = text[:match.start()] + replacement + text[end:]
    while match := re.search(r"[\w.]+\.set_option\(\s*websocket::stream_base::decorator", text):
        start = text.index("(", match.start())
        end = closing(text, start)
        text = text[:match.start()] + "/* Browser owns handshake User-Agent. */" + text[end + 1:]
    text = text.replace("ec == net::error::eof || ec == ssl::error::stream_truncated",
                        "ec == net::error::eof")
    text = text.replace("ec && ec != net::error::eof && ec != ssl::error::stream_truncated", "ec")
    text = text.replace("ec != net::ssl::error::stream_truncated", "bool(ec)")
    text = text.replace("ec != ssl::error::stream_truncated", "bool(ec)")
    # Explicit tokens preserve the C++20 coroutine model on browser transports.
    cursor = 0
    while True:
        start = text.find("co_await ", cursor)
        if start < 0:
            break
        stop = text.find(";", start)
        expression = text[start:stop]
        operation = re.search(r"async_\w+\s*\(", expression)
        if operation:
            opening = start + expression.index("(", operation.start())
            end = closing(text, opening)
            text = text[:end - 1] + ", net::use_awaitable" + text[end - 1:]
            cursor = end + len(", net::use_awaitable")
        else:
            cursor = stop + 1
    text = text.replace("net::as_tuple, net::use_awaitable",
                        "net::as_tuple(net::use_awaitable)")
    if relative.name == "urls_large_data.cpp":
        text = text.replace('"localhost"', '"127.0.0.1"')
    if relative.name == "http_client_methods.cpp":
        text = text.replace("beast::tcp_stream & stream",
                            "beast::browser::http_byte_stream<beast::tcp_stream> & stream")
        text = text.replace("beast::tcp_stream stream(ioc);", "beast::tcp_stream transport(ioc);\n"
                            "beast::browser::http_byte_stream stream(transport);")
        text = text.replace("stream.connect(results);", "transport.connect(results);")
        text = text.replace("stream.socket().shutdown", "transport.socket().shutdown")
        text = text.replace('#include "fixture.hpp"',
                            '#include "fixture.hpp"\n'
                            '#include <webcpp/beast/browser/http_byte_stream.hpp>')
    text = re.sub(r"\bmain\s*\(", "application_main(", text)
    return text


def wrapper(row):
    name = row["id"]
    host = '"localhost"' if row["tls"] else '"127.0.0.1"'
    tls = "global(\"beastSecurePort\")" if row["tls"] else 'global("location")["port"]'
    argument = '"Beast fixture echo"' if name.startswith("websocket-") else '"/http"'
    expected = '"Beast fixture echo"' if name.startswith("websocket-") else '"fetch body"'
    arguments = f'{{"{name}", {host}, port, {argument}}}'
    if name == "json_client":
        arguments, expected = '{"json_client"}', '"pure awesomeness"'
    elif name == "http-crawl":
        arguments, expected = '{"http-crawl", "2"}', '"Success : 3"'
    methods = name == "http-client-methods"
    if methods:
        arguments, expected = '{"http-client-methods", method}', '"200 OK"'
    begin = 'for (const auto* method : {"get", "head", "post", "put", "patch", "delete"}) {' \
        if methods else "{"
    return f"""
#include <emscripten/val.h>
#include <sstream>
#include <vector>
int main() {{
    const auto port = emscripten::val::{tls}.as<std::string>();
    if (setenv("WEBCPP_BEAST_FIXTURE_HOST", {host}, 1) != 0) {{
        return 1;
    }}
    if (setenv("WEBCPP_BEAST_FIXTURE_PORT", port.c_str(), 1) != 0) {{
        return 1;
    }}
    {begin}
        std::vector<std::string> storage{arguments};
        std::vector<char*> arguments;
        for (auto& value : storage) arguments.push_back(value.data());
        std::ostringstream output;
        auto* original = std::cout.rdbuf(output.rdbuf());
        const auto result = application_main(static_cast<int>(arguments.size()), arguments.data());
        std::cout.rdbuf(original);
        std::cout << output.str();
        if (result != 0 || output.str().find({expected}) == std::string::npos) return 1;
    }}
    std::cout << "application {name}: passed\\n";
    return 0;
}}
"""


def generate(formatter):
    manifest = json.loads((prepare.APPLICATIONS / "manifest.json").read_text())
    rows = [row for row in manifest["applications"] if not row["server"] and
            not row["local_socket"] and not row["stackful"] and row["id"] != "doc-ssl-client"]
    needed = {source for row in rows for source in row["sources"]}
    needed |= {"http/client/body/json_body.hpp", "http/client/crawl/urls_large_data.hpp"}
    entrypoints = {row["entrypoint"]: row for row in rows}
    deltas = []
    for name in sorted(needed):
        original = prepare.UPSTREAM / name
        source = original.read_text()
        text = adapt(original.relative_to(prepare.UPSTREAM), source)
        if name in entrypoints:
            text += wrapper(entrypoints[name])
        destination = prepare.APPLICATIONS / "browser" / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        text = subprocess.run([formatter, f"--assume-filename={destination}"], input=text,
                              text=True, capture_output=True, check=True).stdout
        prepare.write_changed(destination, text)
        deltas.extend(difflib.unified_diff(source.splitlines(True), text.splitlines(True),
                                         fromfile=f"upstream/{name}", tofile=f"browser/{name}"))
    (prepare.APPLICATIONS / "browser.patch").write_text("".join(deltas))
    project = prepare.APPLICATIONS.parents[1] / "test/applications/browser"
    project.mkdir(exist_ok=True)
    lines = [prepare.JAM_NOTICE, "import webcpp ;\nwebcpp.targets emscripten ;\n",
             "webcpp.original node : .. ;\n"]
    for row in rows:
        sources = " ".join("../../../example/applications/browser/" + source
                           for source in row["sources"])
        lines.append(f"webcpp.variants {row['id']} : {sources}\n"
                     "  : <library>/webcpp/beast//browser <exception-handling>on\n"
                     "    <include>../../../example/applications/browser\n"
                     "    <include>../../../example/applications <threading>single\n"
                     "    <define>BOOST_ASIO_HAS_CO_AWAIT=1\n"
                     "    <define>BOOST_ASIO_HAS_STD_COROUTINE=1 ;\n"
                     f"webcpp.drive run-{row['id']} : .//{row['id']}\n"
                     f"  : <dependency>{row['id']}.variants.json <webcpp-drive-timeout>120\n"
                     f"  : run.mjs : --program {row['id']} --expected "
                     f'"application {row["id"]}: passed"\n'
                     "  : emscripten ;\n")
    lines.append("webcpp.lane applications-browser :\n  " +
                 "\n  ".join("run-" + row["id"] for row in rows) + "\n  : emscripten ;\n")
    prepare.write_changed(project / "Jamfile", "".join(lines))
    (prepare.APPLICATIONS / "browser/mapping.json").write_text(json.dumps({
        "applications": [row["id"] for row in rows],
        "adaptations": ["Browser owns DNS, HTTP framing, WebSocket wire protocol and TLS trust.",
                        "TLS fixture trusts an ephemeral CA only in a fresh Chromium profile.",
                        "No certificate exception is used; untrusted CA and trusted wrong-host "
                        "rejection controls accompany HTTPS/WSS exchanges.",
                        "Guest CA lists, TLS version pinning, and handshake User-Agent removed.",
                        "System executor and crawler threads use explicit io_context scheduling.",
                        "Awaitable clients retain co_await with explicit use_awaitable tokens.",
                        "Driver injects the local browser endpoint and validates response output."],
    }, indent=2) + "\n")
