// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_TLS_CLIENT_CONTEXT_HPP
#define WEBCPP_ASIO_TLS_CLIENT_CONTEXT_HPP

#include <webcpp/asio/config.hpp>

#include <boost/system/result.hpp>
#include <string>
#include <string_view>
#include <webcpp/asio/ssl.hpp>

#ifndef WEBCPP_TARGET_EMSCRIPTEN
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <limits>
#include <memory>
#include <webcpp/asio/detail/ca_certificates.hpp>
#endif

namespace webcpp::asio {
/** Errors produced while preparing an independently owned TLS client context. */
enum class tls_context_error : int {
    /** The replacement contains no CAs, invalid PEM, or non-certificate data. */
    invalid_ca_bundle = 1,
    /** OpenSSL could not allocate or initialize the requested context or trust store. */
    initialization_failed = 2,
    /** The browser owns TLS and cannot use an OpenSSL client context. */
    unsupported_target = 3,
};

namespace detail {
/** The stable category for TLS context preparation failures. */
class tls_context_category final : public boost::system::error_category {
public:
    /** Keeps error identity across hidden shared-library boundaries. */
    constexpr tls_context_category() noexcept
        : boost::system::error_category(0x6173696f2e746c73ULL) {}

    /** Returns the category name. */
    const char* name() const noexcept override { return "webcpp.asio.tls"; }

    /** Describes a context error without exposing certificate contents. */
    std::string message(int value) const override {
        switch (static_cast<tls_context_error>(value)) {
            case tls_context_error::invalid_ca_bundle:
                return "invalid or empty CA certificate bundle";
            case tls_context_error::initialization_failed:
                return "TLS client context initialization failed";
            case tls_context_error::unsupported_target:
                return "TLS is managed by the browser on this target";
        }
        return "unknown TLS client context error";
    }
};
}  // namespace detail

/**
 Converts a TLS context preparation error to its stable error code.

 Category equality is stable across native shared-library boundaries, including
 hidden visibility. The library defining a returned category must remain loaded
 while a caller uses the error code or its category's virtual functions.
 @param value The context preparation error.
 @return The corresponding code in the webcpp.asio.tls category.
*/
inline boost::system::error_code make_error_code(tls_context_error value) noexcept {
    static const detail::tls_context_category category;
    return {static_cast<int>(value), category};
}

/**
 Creates a verifying TLS client context trusting only the supplied authorities.

 Native and WASIp3 contexts require TLS 1.2 or newer and enable peer verification.
 Input consists of one or more PEM CA certificates separated only by whitespace.
 The certificates are copied into an independent OpenSSL store; input need not
 outlive this call. No system or bundled roots are added. No context is returned
 if any certificate is invalid. Configure hostname verification and SNI on each
 connection. OpenSSL owns allocations; no Asio allocator is used for its store.

 @param ca_pem The complete replacement PEM authority set.
 @return The context, or a tls_context_error; browser targets return unsupported_target.
 @see "How-to: configure trusted certificate authorities", in the guide.
*/
inline boost::system::result<ssl::context> make_tls_client_context(std::string_view ca_pem) {
#ifdef WEBCPP_TARGET_EMSCRIPTEN
    (void)ca_pem;
    return make_error_code(tls_context_error::unsupported_target);
#else
    constexpr std::string_view whitespace = " \t\r\n";
    constexpr std::string_view begin = "-----BEGIN CERTIFICATE-----";
    constexpr std::string_view end = "-----END CERTIFICATE-----";
    if (ca_pem.find_first_not_of(whitespace) == std::string_view::npos) {
        return make_error_code(tls_context_error::invalid_ca_bundle);
    }
    SSL_CTX* raw = SSL_CTX_new(TLS_client_method());
    if (raw == nullptr) {
        return make_error_code(tls_context_error::initialization_failed);
    }
    // The context must remain movable into result; const would prevent that.
    // NOLINTNEXTLINE(misc-const-correctness)
    ssl::context context(raw);
    if (SSL_CTX_set_min_proto_version(raw, TLS1_2_VERSION) != 1) {
        return make_error_code(tls_context_error::initialization_failed);
    }
    SSL_CTX_set_options(raw, SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_verify(raw, SSL_VERIFY_PEER, nullptr);
    while (true) {
        const auto next = ca_pem.find_first_not_of(whitespace);
        if (next == std::string_view::npos) {
            break;
        }
        ca_pem.remove_prefix(next);
        const auto finish = ca_pem.find(end);
        if (!ca_pem.starts_with(begin) || finish == std::string_view::npos) {
            return make_error_code(tls_context_error::invalid_ca_bundle);
        }
        const auto length = finish + end.size();
        // Parentheses prevent the Windows max macro from expanding this function name.
        // NOLINTNEXTLINE(readability-redundant-parentheses)
        if (length > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            return make_error_code(tls_context_error::invalid_ca_bundle);
        }
        const std::unique_ptr<BIO, decltype(&BIO_free)> input(
            BIO_new_mem_buf(ca_pem.data(), static_cast<int>(length)), &BIO_free);
        if (!input) {
            return make_error_code(tls_context_error::initialization_failed);
        }
        unsigned char* der = nullptr;
        long der_length = 0;
        if (PEM_bytes_read_bio(
                &der, &der_length, nullptr, PEM_STRING_X509, input.get(),
                [](char*, int, int, void*) { return 0; }, nullptr) != 1) {
            return make_error_code(tls_context_error::invalid_ca_bundle);
        }
        const auto free_der = [](unsigned char* bytes) { OPENSSL_free(bytes); };
        const std::unique_ptr<unsigned char, decltype(free_der)> decoded(der, free_der);
        const unsigned char* cursor = decoded.get();
        const std::unique_ptr<X509, decltype(&X509_free)> cert(
            d2i_X509(nullptr, &cursor, der_length), &X509_free);
        if (!cert || cursor != decoded.get() + der_length || X509_check_ca(cert.get()) <= 0) {
            return make_error_code(tls_context_error::invalid_ca_bundle);
        }
        if (X509_STORE_add_cert(SSL_CTX_get_cert_store(raw), cert.get()) != 1) {
            return make_error_code(tls_context_error::initialization_failed);
        }
        ca_pem.remove_prefix(length);
    }
    return context;
#endif
}

/**
 Creates a verifying TLS client context using WebCPP's embedded Mozilla CA snapshot.

 No certificate files, system store, environment settings or network update are
 read. Updating embedded roots requires rebuilding and redeploying the program.
 The protocol floor, allocation and per-connection requirements are the same as
 the replacement overload. Link /webcpp/asio//tls to supply its dependencies.

 @return The context, or a tls_context_error; browser targets return unsupported_target.
 @see "How-to: configure trusted certificate authorities", in the guide.
*/
inline boost::system::result<ssl::context> make_tls_client_context() {
#ifdef WEBCPP_TARGET_EMSCRIPTEN
    return make_error_code(tls_context_error::unsupported_target);
#else
    return make_tls_client_context(webcpp::asio::detail::ca_certificates::pem);
#endif
}
}  // namespace webcpp::asio
#endif
