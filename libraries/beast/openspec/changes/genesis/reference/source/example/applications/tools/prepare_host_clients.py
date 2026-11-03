# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Explicit host HTTP client counterparts, retaining each application's request semantics."""

import json
import re
import subprocess
from pathlib import Path

import prepare


METHODS = '''
    const http::verb methods[] = {http::verb::get, http::verb::head, http::verb::post,
                                 http::verb::put, http::verb::patch, http::verb::delete_};
    for (const auto method : methods) {
        http::request<http::string_body> request{method, "/fixture", 11};
        request.set(http::field::host, authority);
        if (method == http::verb::post) request.body() = "Some random post data";
        if (method == http::verb::put) request.body() = "Some random put data";
        if (method == http::verb::delete_) request.body() = "Some random delete data";
        if (method == http::verb::patch) request.body() = "Some random patch data";
        request.prepare_payload();
        http::response<http::string_body> response;
        peer.perform(request, response, error);
        if (error || response.result() != http::status::ok) return failure(error);
        if (method != http::verb::head && !request.body().empty() &&
            response.body() != request.body()) return failure(error);
        ++completed;
    }
'''


def crawl(source):
    report = source[source.index("// This structure aggregates"):
                    source.index("// Performs HTTP GET requests")]
    report = re.sub(r"net::strand<\s*net::io_context::executor_type> strand_;",
                    "wasi::http_context& context_;", report)
    report = report.replace("std::atomic<std::size_t>", "std::size_t")
    report = report.replace("std::size_t index_;", "std::size_t index_ = 0;")
    report = report.replace("std::vector<char const*>", "std::vector<crawl_target>")
    report = re.sub(r"crawl_report\(net::io_context& ioc\)\s*:\s*"
                    r"strand_\(ioc.get_executor\(\)\)\s*, index_\(0\)\s*, "
                    r"hosts_\(urls_large_data\(\)\)",
                    "crawl_report(wasi::http_context& context, "
                    "const std::vector<crawl_target>& hosts)\n"
                    ": context_(context), hosts_(hosts)", report)
    report = re.sub(r"net::post\(\s*strand_,", "context_.get_executor().post(", report)
    report = report.replace("[&, f]", "[this, f]")
    report = report.replace("            });", "            }, std::allocator<void>{});")
    report = report.replace("char const*\n    get_host()", "crawl_target const*\n    get_host()")
    report = report.replace("return hosts_[n];", "return &hosts_[n];")
    report = report.replace("if(n >= hosts_.size())\n            return nullptr;",
                            "if(n >= hosts_.size()) { return nullptr; }")
    report = re.sub(r"(for\(auto const& result : report.status_codes\))\s*(os <<.*?;\n)",
                    r"\1 { \2 }\n", report, flags=re.S)
    report = report.replace("on the strand.", "on the serialized host queue.")
    report = report.replace("    // Counts the number of timer failures",
                            "    [[nodiscard]] std::size_t completed() const { return count_; }\n\n"
                            "    // Counts the number of timer failures")
    template = Path(__file__).with_name("host_crawl.cpp.in").read_text()
    return template.replace("@REPORT@", report)


def generate(formatter):
    manifest = json.loads((prepare.APPLICATIONS / "manifest.json").read_text())
    rows = [row for row in manifest["applications"] if not row["server"] and
            not row["id"].startswith("websocket-")]
    destination = prepare.APPLICATIONS / "host-clients/json_body.hpp"
    destination.parent.mkdir(exist_ok=True)
    body = (prepare.UPSTREAM / "http/client/body/json_body.hpp").read_text()
    body = prepare.NOTICE + body.replace("<boost/beast/http.hpp>", "<webcpp/beast/http.hpp>")
    body = subprocess.run([formatter, f"--assume-filename={destination}"], input=body,
                          text=True, capture_output=True, check=True).stdout
    prepare.write_changed(destination, body)
    for row in rows:
        source = (prepare.UPSTREAM / row["entrypoint"]).read_text()
        notice = source[:source.find("#include")]
        includes = ""
        operation = '''
    http::request<http::empty_body> request{http::verb::get, "/fixture", 11};
    request.set(http::field::host, authority);
    http::response<http::string_body> response;
    peer.perform(request, response, error);
    if (error || response.result() != http::status::ok ||
        response.body() != "beast-application-fixture\\n") return failure(error);
    ++completed;
'''
        if row["id"] == "http-client-methods":
            operation = METHODS
        if row["id"] == "json_client":
            includes = '#include "json_body.hpp"\n#include <boost/json/src.hpp>\n'
            operation = '''
    http::request<json_body> request{http::verb::post, "/post", 11};
    request.set(http::field::host, authority);
    request.set(http::field::content_type, "application/json");
    request.body() = {{"type", "test"}, {"content", "pure awesomeness"}};
    request.prepare_payload();
    request.chunked(false); // Host owns framing for the original unsized JSON Body.
    http::response<json_body> response;
    peer.perform(request, response, error);
    if (error || response.result() != http::status::ok ||
        response.body() != request.body()) return failure(error);
    ++completed;
'''
        text = prepare.NOTICE + notice + '''
#include <webcpp/beast/wasi/http.hpp>
#include <string>
''' + includes + '''
namespace beast = webcpp::beast;
namespace http = beast::http;
http::response<http::string_body> failure(boost::system::error_code error) {
    http::response<http::string_body> response{http::status::bad_gateway, 11};
    response.body() = "application peer exchange failed: " + error.message();
    response.prepare_payload();
    return response;
}
http::response<http::string_body> handle(http::request<http::string_body> incoming) {
    const auto authority_field = incoming["X-Beast-Peer"];
    const std::string authority(authority_field.data(), authority_field.size());
    const auto scheme_field = incoming["X-Beast-Scheme"];
    const std::string scheme = scheme_field.empty() ? "http" :
        std::string(scheme_field.data(), scheme_field.size());
    beast::wasi::client peer{scheme, authority};
    boost::system::error_code error;
    unsigned completed = 0;
''' + operation + '''
    http::response<http::string_body> result{http::status::ok, 11};
    result.body() = "completed=" + std::to_string(completed);
    result.prepare_payload();
    return result;
}
WEBCPP_BEAST_WASI_HTTP_EXPORT(handle)
'''
        if row["id"] == "http-crawl":
            text = prepare.NOTICE + notice + crawl(source)
        destination = prepare.APPLICATIONS / "host-clients" / f"{row['id']}.cpp"
        destination.parent.mkdir(exist_ok=True)
        text = subprocess.run([formatter, f"--assume-filename={destination}"], input=text,
                              text=True, capture_output=True, check=True).stdout
        prepare.write_changed(destination, text)
    project = prepare.APPLICATIONS.parents[1] / "test/applications/host-clients"
    project.mkdir(exist_ok=True)
    lines = [prepare.JAM_NOTICE, "import webcpp ;\nwebcpp.targets wasip2 wasip3 ;\n",
             "webcpp.original python3 : .. ;\n"]
    for row in rows:
        name = row["id"]
        lines.append(f"webcpp.drive {name} : "
                     f"../../../example/applications/host-clients/{name}.cpp\n"
                     "  : <library>/webcpp/beast//wasi-http <linkflags>-mexec-model=reactor\n"
                     "    <include>../../../example/applications\n"
                     f"  : driver.py : {name} : wasip2 wasip3 ;\n")
    lines.append("webcpp.lane applications-host-clients :\n  " +
                 "\n  ".join(row["id"] for row in rows) + "\n  : wasip2 wasip3 ;\n")
    prepare.write_changed(project / "Jamfile", "".join(lines))
    (prepare.APPLICATIONS / "host-clients/mapping.json").write_text(json.dumps({
        "applications": [row["id"] for row in rows],
        "adaptations": ["Incoming host invocation replaces command-line launch.",
                        "Host HTTP perform replaces original socket scheduling and HTTP framing.",
                        "GET, six methods and JSON Body retained. Crawler preserves the upstream "
                        "report and two asynchronous get-next/complete/aggregate workers over "
                        "up to 32 distinct controlled authority/path pairs. Host HTTP combines "
                        "transport phases; complete exchange failures increment Read, while "
                        "the separate socket phase counters are unmeasured.",
                        "TLS/Unix transport become host-owned; the base fixture measures HTTP "
                        "payloads. Separate configured-host HTTPS fixtures verify CA trust, "
                        "hostname rejection, and the documentation client's mutual TLS.",
                        "Stackful source compatibility remains blocked without Boost.Context."],
    }, indent=2) + "\n")
