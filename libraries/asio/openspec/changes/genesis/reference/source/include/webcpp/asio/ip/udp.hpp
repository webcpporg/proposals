// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_IP_UDP_HPP
#define WEBCPP_ASIO_IP_UDP_HPP
#include <webcpp/asio/config.hpp>

#include <webcpp/asio/ip/basic_resolver.hpp>
#ifndef __EMSCRIPTEN__
#include <boost/asio/ip/udp.hpp>  // lint-world: native protocol alias

namespace webcpp::asio::ip {
/** Boost's native UDP protocol. */
using boost::asio::ip::udp;
}  // namespace webcpp::asio::ip
#else
#include <boost/asio/detail/socket_types.hpp>  // lint-world: portable protocol constants
#include <boost/asio/ip/basic_endpoint.hpp>  // lint-world: Boost endpoint layout for the browser protocol
#include <webcpp/asio/basic_datagram_socket.hpp>

namespace webcpp::asio::ip {
/** The browser UDP protocol, with operations supplied by a transport. */
class udp {
public:
    /** The endpoint type. */
    using endpoint = boost::asio::ip::basic_endpoint<udp>;
    /** The resolver type. */
    using resolver = basic_resolver<udp>;
    /** The socket type. */
    using socket = basic_datagram_socket<udp>;

    /** Returns the IPv4 protocol.
     @return The IPv4 protocol. */
    static udp v4() noexcept { return udp(BOOST_ASIO_OS_DEF(AF_INET)); }

    /** Returns the IPv6 protocol.
     @return The IPv6 protocol. */
    static udp v6() noexcept { return udp(BOOST_ASIO_OS_DEF(AF_INET6)); }

    /** Returns the socket type constant.
     @return The socket type. */
    // Match the InternetProtocol member signature used by Boost.
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    [[nodiscard]] int type() const noexcept { return BOOST_ASIO_OS_DEF(SOCK_DGRAM); }

    /** Returns the protocol number.
     @return The protocol number. */
    // Match the InternetProtocol member signature used by Boost.
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    [[nodiscard]] int protocol() const noexcept { return BOOST_ASIO_OS_DEF(IPPROTO_UDP); }

    /** Returns the address family.
     @return The address family. */
    [[nodiscard]] int family() const noexcept { return family_; }

    /** Compares address families.
     @param a The first protocol.
     @param b The second protocol.
     *
     @return Whether they match. */
    friend bool operator==(const udp& a, const udp& b) noexcept = default;

private:
    /** Constructs a protocol for an address family. */
    explicit udp(int family) : family_(family) {}

    int family_;
};
}  // namespace webcpp::asio::ip
#endif
#endif  // WEBCPP_ASIO_IP_UDP_HPP
