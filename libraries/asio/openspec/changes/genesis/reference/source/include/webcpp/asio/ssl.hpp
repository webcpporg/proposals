// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_SSL_HPP
#define WEBCPP_ASIO_SSL_HPP

#include <webcpp/asio/config.hpp>

#ifndef __EMSCRIPTEN__
#ifndef WEBCPP_ASIO_HAS_OPENSSL
#error \
    "webcpp::asio: <webcpp/asio/ssl.hpp> needs OpenSSL natively, and it is not linked. In the program's Jamfile, add /webcpp/asio//openssl beside /webcpp/asio//asio, as in: exe app : app.cpp /webcpp/asio//asio /webcpp/asio//openssl ; b2 links it when it finds OpenSSL 3 (-sOPENSSL_ROOT=<dir> names it). (without b2: define WEBCPP_ASIO_HAS_OPENSSL=1 and link libssl and libcrypto)"
#else
#include <boost/asio/ssl.hpp>  // lint-world: native TLS with explicitly linked OpenSSL

namespace webcpp::asio {
/** Boost's native TLS types, with OpenSSL explicitly linked by the program. */
namespace ssl = boost::asio::ssl;
}  // namespace webcpp::asio
#endif
#else
#include <boost/asio/buffer.hpp>  // lint-world: certificate and key buffer signatures
#include <boost/asio/error.hpp>   // lint-world: error-code setting overloads
#include <boost/system/error_code.hpp>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace webcpp::asio::ssl {
/** The peer verification bitmask type. */
using verify_mode = int;
/** Requests no peer verification. */
inline constexpr verify_mode verify_none = 0x00;
/** Requests peer verification. */
inline constexpr verify_mode verify_peer = 0x01;
/** Requires a peer certificate when peer verification is requested. */
inline constexpr verify_mode verify_fail_if_no_peer_cert = 0x02;
/** Requests a client certificate only once. */
inline constexpr verify_mode verify_client_once = 0x04;

/** Names the TLS methods and option values compatible with Boost and OpenSSL 3. */
class context_base {
public:
    /** The TLS method names in Boost's enumeration order. */
    enum method {
        /** The sslv2 generic method. */
        sslv2,
        /** The sslv2 client method. */
        sslv2_client,
        /** The sslv2 server method. */
        sslv2_server,
        /** The sslv3 generic method. */
        sslv3,
        /** The sslv3 client method. */
        sslv3_client,
        /** The sslv3 server method. */
        sslv3_server,
        /** The tlsv1 generic method. */
        tlsv1,
        /** The tlsv1 client method. */
        tlsv1_client,
        /** The tlsv1 server method. */
        tlsv1_server,
        /** The sslv23 generic method. */
        sslv23,
        /** The sslv23 client method. */
        sslv23_client,
        /** The sslv23 server method. */
        sslv23_server,
        /** The tlsv11 generic method. */
        tlsv11,
        /** The tlsv11 client method. */
        tlsv11_client,
        /** The tlsv11 server method. */
        tlsv11_server,
        /** The tlsv12 generic method. */
        tlsv12,
        /** The tlsv12 client method. */
        tlsv12_client,
        /** The tlsv12 server method. */
        tlsv12_server,
        /** The tlsv13 generic method. */
        tlsv13,
        /** The tlsv13 client method. */
        tlsv13_client,
        /** The tlsv13 server method. */
        tlsv13_server,
        /** The tls generic method. */
        tls,
        /** The tls client method. */
        tls_client,
        /** The tls server method. */
        tls_server,
    };

    /** The certificate or key representation. */
    enum file_format {
        /** ASN.1 encoding. */
        asn1,
        /** PEM encoding. */
        pem,
    };

    /** The TLS option bitmask type. */
    using options = std::uint64_t;
    /** Requests the OpenSSL 3 compatibility workaround bits. */
    static constexpr options default_workarounds = 0x80000850ULL;
    /** Retains the OpenSSL 3 legacy single-use DH option. */
    static constexpr options single_dh_use = 0;
    /** Retains the OpenSSL 3 legacy SSLv2 disabling option. */
    static constexpr options no_sslv2 = 0;
    /** Requests disabling SSLv3. */
    static constexpr options no_sslv3 = 0x02000000ULL;
    /** Requests disabling TLSv1. */
    static constexpr options no_tlsv1 = 0x04000000ULL;
    /** Requests disabling TLSv1.1. */
    static constexpr options no_tlsv1_1 = 0x10000000ULL;
    /** Requests disabling TLSv1.2. */
    static constexpr options no_tlsv1_2 = 0x08000000ULL;
    /** Requests disabling TLSv1.3. */
    static constexpr options no_tlsv1_3 = 0x20000000ULL;
    /** Requests disabling compression. */
    static constexpr options no_compression = 0x00020000ULL;
    /** The legacy context-level verification bitmask alias. */
    using verify_mode = ssl::verify_mode;
    /** The legacy context-level verify_none value. */
    static constexpr verify_mode verify_none = ssl::verify_none;
    /** The legacy context-level verify_peer value. */
    static constexpr verify_mode verify_peer = ssl::verify_peer;
    /** The legacy context-level verify_fail_if_no_peer_cert value. */
    static constexpr verify_mode verify_fail_if_no_peer_cert = ssl::verify_fail_if_no_peer_cert;
    /** The legacy context-level verify_client_once value. */
    static constexpr verify_mode verify_client_once = ssl::verify_client_once;

protected:
    /** Prevents deletion through the constants-only base. */
    ~context_base() = default;
};

/** Names the handshake roles for browser stream adapters. */
class stream_base {
public:
    /** Which endpoint starts the handshake. */
    enum handshake_type {
        /** The client role. */
        client,
        /** The server role. */
        server,
    };

protected:
    /** Prevents deletion through the constants-only base. */
    ~stream_base() = default;
};

/** Owns the host name requested for certificate verification. */
class host_name_verification {
public:
    /**
     Copies the verification host so temporary input storage is safe.
     @param host The expected host name.
    */
    explicit host_name_verification(std::string_view host) : host_(host) {}

    /**
     Returns the owned host name as a view.
     @return A view valid until this object changes or is destroyed.
    */
    [[nodiscard]] std::string_view host() const noexcept { return host_; }

private:
    std::string host_;
};

/** Identifies the recorded verification callback without executing it. */
enum class verify_callback_kind : std::uint8_t {
    /** No callback was supplied. */
    none,
    /** A host-name verification request. */
    host_name_verification,
    /** Any other verification callback. */
    other,
};

/**
 Records TLS settings without implementing TLS or reading certificate files.

 Browser stream adapters decide which settings their transport can honor.
 @see "Explanation: TLS settings", in the guide.
*/
class context : public context_base {
public:
    /**
     Constructs an empty record using the default memory resource.
     @param selected The requested TLS method.
    */
    explicit context(method selected) : context(selected, std::pmr::get_default_resource()) {}

    /**
     Constructs a record whose verification host uses resource.
     @param selected The requested TLS method.
     @param resource The host storage resource, or null for the default resource.
    */
    context(method selected, std::pmr::memory_resource* resource)
        : method_(selected),
          host_(resource != nullptr ? resource : std::pmr::get_default_resource()) {}

    /** Transfers the record and its host storage.
     @param other The source record.
    */
    context(context&& other) noexcept = default;
    /** Moves the record, retaining the destination's memory resource.
     @param other The source record.
     @return This record.
    */
    // PMR assignment can allocate when the source and destination resources differ.
    // NOLINTBEGIN(bugprone-unsafe-to-allow-exceptions,performance-noexcept-move-constructor)
    context& operator=(context&& other) noexcept(
        std::is_nothrow_move_assignable_v<std::pmr::string>) = default;
    // NOLINTEND(bugprone-unsafe-to-allow-exceptions,performance-noexcept-move-constructor)
    /** A context cannot be copied. */
    context(const context&) = delete;
    /** A context cannot be copy-assigned. */
    context& operator=(const context&) = delete;
    /** Releases the recorded host storage. */
    ~context() = default;

    /**
     Records the verification callback kind and, for host verification, its name.
     @tparam VerifyCallback The callback type.
     @param callback The verification request; it is never invoked here.
     @param ec Receives success.
    */
    template <class VerifyCallback>
    void set_verify_callback(VerifyCallback callback, boost::system::error_code& ec) {
        if constexpr (std::is_same_v<std::remove_cvref_t<VerifyCallback>, host_name_verification>) {
            host_ = callback.host();
            callback_ = verify_callback_kind::host_name_verification;
        } else {
            host_.clear();
            callback_ = verify_callback_kind::other;
        }
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records the verification callback without an error-code argument.
     @tparam VerifyCallback The callback type.
     @param callback The verification request.
    */
    template <class VerifyCallback>
    void set_verify_callback(VerifyCallback callback) {
        boost::system::error_code ec;
        set_verify_callback(std::move(callback), ec);
    }
#endif
    /**
     Records the requested verification mode.
     @param mode The requested verification bitmask.
     @param ec Receives success.
    */
    void set_verify_mode(verify_mode mode, boost::system::error_code& ec) {
        verify_mode_ = mode;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records the requested verification mode.
     @param mode The requested verification bitmask.
    */
    void set_verify_mode(verify_mode mode) {
        boost::system::error_code ec;
        set_verify_mode(mode, ec);
    }
#endif
    /**
     Records a request for default verification paths.
     @param ec Receives success.
    */
    void set_default_verify_paths(boost::system::error_code& ec) {
        default_paths_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a request for default verification paths.
    */
    void set_default_verify_paths() {
        boost::system::error_code ec;
        set_default_verify_paths(ec);
    }
#endif
    /**
     Adds requested TLS option bits.
     @param value The option bits to add.
     @param ec Receives success.
    */
    void set_options(options value, boost::system::error_code& ec) {
        options_ |= value;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Adds requested TLS option bits.
     @param value The option bits to add.
    */
    void set_options(options value) {
        boost::system::error_code ec;
        set_options(value, ec);
    }
#endif
    /**
     Records a request to add certificate roots.
     @param ca The certificate authority data.
     @param ec Receives success.
    */
    void add_certificate_authority([[maybe_unused]] const boost::asio::const_buffer& ca,
                                   boost::system::error_code& ec) {
        roots_added_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a request to add certificate roots.
     @param ca The certificate authority data.
    */
    void add_certificate_authority(const boost::asio::const_buffer& ca) {
        boost::system::error_code ec;
        add_certificate_authority(ca, ec);
    }
#endif
    /**
     Records a root-file request without accessing the file.
     @param filename The requested root certificate file.
     @param ec Receives success.
    */
    void load_verify_file([[maybe_unused]] std::string_view filename,
                          boost::system::error_code& ec) {
        roots_added_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a root-file request without accessing the file.
     @param filename The requested root certificate file.
    */
    void load_verify_file(std::string_view filename) {
        boost::system::error_code ec;
        load_verify_file(filename, ec);
    }
#endif
    /**
     Records a root-directory request without accessing it.
     @param path The requested root certificate directory.
     @param ec Receives success.
    */
    void add_verify_path([[maybe_unused]] std::string_view path, boost::system::error_code& ec) {
        roots_added_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a root-directory request without accessing it.
     @param path The requested root certificate directory.
    */
    void add_verify_path(std::string_view path) {
        boost::system::error_code ec;
        add_verify_path(path, ec);
    }
#endif
    /**
     Records a client certificate request.
     @param certificate The certificate bytes.
     @param format The certificate encoding.
     @param ec Receives success.
    */
    void use_certificate([[maybe_unused]] const boost::asio::const_buffer& certificate,
                         [[maybe_unused]] file_format format, boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a client certificate request.
     @param certificate The certificate bytes.
     @param format The certificate encoding.
    */
    void use_certificate(const boost::asio::const_buffer& certificate, file_format format) {
        boost::system::error_code ec;
        use_certificate(certificate, format, ec);
    }
#endif
    /**
     Records a certificate-file request without accessing it.
     @param filename The requested certificate file.
     @param format The certificate encoding.
     @param ec Receives success.
    */
    void use_certificate_file([[maybe_unused]] std::string_view filename,
                              [[maybe_unused]] file_format format, boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a certificate-file request without accessing it.
     @param filename The requested certificate file.
     @param format The certificate encoding.
    */
    void use_certificate_file(std::string_view filename, file_format format) {
        boost::system::error_code ec;
        use_certificate_file(filename, format, ec);
    }
#endif
    /**
     Records a client certificate chain request.
     @param chain The certificate chain bytes.
     @param ec Receives success.
    */
    void use_certificate_chain([[maybe_unused]] const boost::asio::const_buffer& chain,
                               boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a client certificate chain request.
     @param chain The certificate chain bytes.
    */
    void use_certificate_chain(const boost::asio::const_buffer& chain) {
        boost::system::error_code ec;
        use_certificate_chain(chain, ec);
    }
#endif
    /**
     Records a chain-file request without accessing it.
     @param filename The requested chain file.
     @param ec Receives success.
    */
    void use_certificate_chain_file([[maybe_unused]] std::string_view filename,
                                    boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a chain-file request without accessing it.
     @param filename The requested chain file.
    */
    void use_certificate_chain_file(std::string_view filename) {
        boost::system::error_code ec;
        use_certificate_chain_file(filename, ec);
    }
#endif
    /**
     Records a client private key request.
     @param key The private key bytes.
     @param format The key encoding.
     @param ec Receives success.
    */
    void use_private_key([[maybe_unused]] const boost::asio::const_buffer& key,
                         [[maybe_unused]] file_format format, boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a client private key request.
     @param key The private key bytes.
     @param format The key encoding.
    */
    void use_private_key(const boost::asio::const_buffer& key, file_format format) {
        boost::system::error_code ec;
        use_private_key(key, format, ec);
    }
#endif
    /**
     Records a private-key-file request without accessing it.
     @param filename The requested key file.
     @param format The key encoding.
     @param ec Receives success.
    */
    void use_private_key_file([[maybe_unused]] std::string_view filename,
                              [[maybe_unused]] file_format format, boost::system::error_code& ec) {
        credentials_ = true;
        ec.clear();
    }
#ifndef WEBCPP_ASIO_NO_EXCEPTIONS
    /**
     Records a private-key-file request without accessing it.
     @param filename The requested key file.
     @param format The key encoding.
    */
    void use_private_key_file(std::string_view filename, file_format format) {
        boost::system::error_code ec;
        use_private_key_file(filename, format, ec);
    }
#endif
    /**
     Returns the recorded setting.
     @return The requested TLS method.
    */
    [[nodiscard]] method recorded_method() const noexcept { return method_; }

    /**
     Returns the recorded setting.
     @return The requested verification mode.
    */
    [[nodiscard]] verify_mode recorded_verify_mode() const noexcept { return verify_mode_; }

    /**
     Returns the recorded setting.
     @return Whether default verification paths were requested.
    */
    [[nodiscard]] bool default_verify_paths() const noexcept { return default_paths_; }

    /**
     Returns the recorded setting.
     @return The verification callback kind.
    */
    [[nodiscard]] verify_callback_kind recorded_callback() const noexcept { return callback_; }

    /**
     Returns the recorded setting.
     @return The recorded verification host.
    */
    [[nodiscard]] std::string_view verification_host() const noexcept { return host_; }

    /**
     Returns the recorded setting.
     @return Whether additional certificate roots were requested.
    */
    [[nodiscard]] bool roots_added() const noexcept { return roots_added_; }

    /**
     Returns the recorded setting.
     @return Whether client credentials were requested.
    */
    [[nodiscard]] bool client_credentials() const noexcept { return credentials_; }

    /**
     Returns the recorded setting.
     @return The accumulated TLS option bits.
    */
    [[nodiscard]] options recorded_options() const noexcept { return options_; }

private:
    method method_;
    std::pmr::string host_;
    verify_mode verify_mode_ = verify_none;
    options options_ = 0;
    verify_callback_kind callback_ = verify_callback_kind::none;
    bool default_paths_ = false;
    bool roots_added_ = false;
    bool credentials_ = false;
};
}  // namespace webcpp::asio::ssl
#endif
#endif  // WEBCPP_ASIO_SSL_HPP
