// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_APPLICATION_FIXTURE_RESOLVER_HPP
#define WEBCPP_BEAST_APPLICATION_FIXTURE_RESOLVER_HPP

#include <boost/asio/append.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/ip/basic_resolver_results.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <string>
#include <string_view>
#include <webcpp/asio.hpp>

namespace webcpp_fixture {
// The application's DNS dependency is explicit: this fixture only resolves its local peer.
// It does not claim WASI provides an asynchronous DNS worker or a system executor.
class resolver {
public:
    using executor_type = boost::asio::any_io_executor;
    using results_type = boost::asio::ip::basic_resolver_results<webcpp::asio::ip::tcp>;

    explicit resolver(executor_type executor) : executor_(std::move(executor)) {}

    explicit resolver(webcpp::asio::io_context& context) : resolver(context.get_executor()) {}

    [[nodiscard]] executor_type get_executor() const { return executor_; }

    static void cancel() {}

    static results_type resolve(std::string_view host, std::string_view port,
                                boost::system::error_code& ec) {
        if (host != "localhost" && host != "127.0.0.1") {
            ec = boost::asio::error::host_not_found;
            return {};
        }
        unsigned value = 0;
        for (const char digit : port) {
            if (digit < '0' || digit > '9' || value > 6553) {
                ec = boost::asio::error::service_not_found;
                return {};
            }
            value = (value * 10) + static_cast<unsigned>(digit - '0');
        }
        if (port.empty() || value > 65535) {
            ec = boost::asio::error::service_not_found;
            return {};
        }
        ec.clear();
        return results_type::create(
            {boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(value)},
            std::string(host), std::string(port));
    }

    static results_type resolve(std::string_view host, std::string_view port) {
        boost::system::error_code ec;
        auto result = resolve(host, port, ec);
        if (ec) {
            boost::throw_exception(boost::system::system_error(ec));
        }
        return result;
    }

    template <class Token = boost::asio::default_completion_token_t<executor_type>>
    auto async_resolve(std::string_view host, std::string_view port, Token&& token = {}) {
        return boost::asio::async_initiate<Token, void(boost::system::error_code, results_type)>(
            // Value parameters retain the upstream callback and coroutine ownership contract.
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            [this](auto handler, std::string name, std::string service) {
                boost::system::error_code ec;
                auto result = resolve(name, service, ec);
                boost::asio::post(executor_,
                                  boost::asio::append(std::move(handler), ec, std::move(result)));
            },
            token, std::string(host), std::string(port));
    }

private:
    executor_type executor_;
};
}  // namespace webcpp_fixture
#endif
