// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_SSL_HPP
#define WEBCPP_BEAST_SSL_HPP

#include <webcpp/beast/config.hpp>

#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP3)
#include <webcpp/asio/ssl.hpp>

#include <boost/beast/ssl.hpp>  // lint-world: explicit guest-owned TLS stream
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <string_view>

namespace webcpp::beast {
/** Upstream TLS stream, linked through the explicit guest TLS target. */
using boost::beast::ssl_stream;

/**
 Sets the TLS Server Name Indication hostname on a guest-owned TLS stream.

 @tparam NextLayer The TLS stream's underlying stream type.
 @param stream The TLS stream before its client handshake.
 @param host The nonempty hostname, at most 255 bytes, without embedded NUL.
 @return Success, invalid_argument for an invalid length or embedded NUL, or an OpenSSL error.
 @note Copies the view before calling OpenSSL, so its storage need not be NUL-terminated.
 This sets SNI only; peer and hostname verification remain the application's TLS policy.
 Host-managed HTTPS uses a separate backend and does not call this helper.
*/
template <class NextLayer>
[[nodiscard]] boost::system::result<void> set_server_name(ssl_stream<NextLayer>& stream,
                                                          std::string_view host) {
    if (host.empty() || host.size() > 255 || host.find('\0') != std::string_view::npos) {
        return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
    }
    std::array<char, 256> name{};
    std::ranges::copy(host, name.begin());
    if (SSL_set_tlsext_host_name(stream.native_handle(), name.data()) != 1) {
        return boost::system::error_code(static_cast<int>(ERR_get_error()),
                                         boost::asio::error::get_ssl_category());
    }
    return {};
}
}  // namespace webcpp::beast
#elif defined(WEBCPP_TARGET_EMSCRIPTEN)
#include <webcpp/beast/browser/ssl_stream.hpp>
#else
#error "Guest-owned Beast TLS currently requires native or WASIp3 and /webcpp/beast//tls"
#endif

#endif
