// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//
// Explicit supplied-channel counterpart of http/client/async-local.
// The original GET/header construction and complete response are retained;
// readiness is driven by the host stream instead of a Unix socket executor.

#include <boost/beast/version.hpp>
#include <cstdio>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/wasi/stream.hpp>

int main(int argc, const char* const* argv) {
    namespace http = webcpp::beast::http;
    if (argc != 3) {
        return 64;
    }
    webcpp::beast::wasi::stdio_stream channel;
    http::request<http::empty_body> request{http::verb::get, argv[2], 11};
    request.set(http::field::host, argv[1]);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    boost::system::error_code error;
    webcpp::beast::wasi::write(channel, request, error);
    if (error) {
        return 1;
    }
    webcpp::beast::flat_buffer input;
    http::response_parser<http::string_body> parser;
    webcpp::beast::wasi::read(channel, input, parser, error);
    if (error) {
        static_cast<void>(std::fprintf(stderr, "read: %s\n", error.message().c_str()));
        return 2;
    }
    const auto& response = parser.get();
    static_cast<void>(std::fprintf(stderr, "status=%u\n", response.result_int()));
    static_cast<void>(std::fwrite(response.body().data(), 1, response.body().size(), stderr));
    return 0;
}
