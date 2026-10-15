// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <cstdio>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/test/exit_status.hpp>

int main() {
    webcpp::asio::io_context io;
    webcpp::asio::ip::tcp::resolver resolver(io);
    int failures = 0;
    resolver.async_resolve(
        "example.com", "wss", [&](boost::system::error_code ec, const auto& results) {
            if (ec || results.size() != 1) {
                ++failures;
                return;
            }
            const auto& entry = *results.begin();
            const auto endpoint = entry.endpoint();
            std::printf("%s %s %u unspecified: %s\n", entry.host_name().c_str(),
                        entry.service_name().c_str(), static_cast<unsigned>(endpoint.port()),
                        endpoint.address().is_unspecified() ? "true" : "false");
        });
    io.run();  // lint-run: main waits for the posted resolver result
    return webcpp::test::exit_status(failures);
}

// end::program[]
