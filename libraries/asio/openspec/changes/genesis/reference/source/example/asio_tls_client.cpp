// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
// tag::program[]
#include <cstdio>
#include <webcpp/asio/tls_client_context.hpp>
#include <webcpp/test/exit_status.hpp>

// main's standard signature requires char**, even though it does not modify argv.
// NOLINTNEXTLINE(misc-const-correctness)
int main(int argc, char** argv) {
    // An optional argument contains replacement PEM bytes, not a filename.
    // Keep the prepared context mutable for subsequent TLS stream construction.
    // NOLINTNEXTLINE(misc-const-correctness)
    auto prepared = argc == 1 ? webcpp::asio::make_tls_client_context()
                              : webcpp::asio::make_tls_client_context(argv[1]);
    if (!prepared) {
        const auto message = prepared.error().message();
        (void)std::fprintf(stderr, "%s\n", message.c_str());
        return webcpp::test::exit_status(1);
    }
    // Pass *prepared to TLS streams and keep it alive while those streams exist.
    std::puts(argc == 1 ? "TLS client ready: embedded authorities"
                        : "TLS client ready: replacement authorities");
    return webcpp::test::exit_status(0);
}

// end::program[]
