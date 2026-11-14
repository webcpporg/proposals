// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Two native peers in one process, bound to the loopback and signalled in memory: the first
// offers, the second answers, and each trickles its candidates to the other until its gathering
// ends. Every wait it starts stops re-arming once its work is done, so that io_context::run
// returns when a test's exchange is over.

#ifndef WEBCPP_TEST_WEBRTC_PAIR_HPP
#define WEBCPP_TEST_WEBRTC_PAIR_HPP

#include <webcpp/webrtc.hpp>

#include "require.hpp"

#include <webcpp/asio/io_context.hpp>  // lint-world: the tests' event loop

#include <boost/core/lightweight_test.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace webcpp::test {

namespace webrtc = webcpp::webrtc;

/** The loopback, without which libdatachannel offers no candidate two local peers can use. */
inline constexpr std::string_view loopback = "127.0.0.1";

/** How long one exchange may take before a test gives up on it. */
inline constexpr std::chrono::seconds exchange_limit{20};

/** A configuration with host candidates on the loopback only, no STUN, as CI allows. */
inline webrtc::configuration loopback_configuration() {
    webrtc::configuration config;
    config.bind_address = loopback;
    return config;
}

/**
 Sends the candidates one peer gathers to the other, holding them until the other has a remote
 description, and stops at the end of gathering.
*/
template <class PeerConnection>
class trickle {
public:
    using candidate_type = PeerConnection::ice_candidate;

    trickle(PeerConnection& from, PeerConnection& to) : from_(from), to_(to) {}

    /** Starts waiting for the first candidate. */
    void start() {
        from_.async_wait_ice_candidate([this](webrtc::detail::error_code ec, candidate_type c) {
            if (ec) {
                return;
            }
            const bool end = c.candidate.empty();
            if (end) {
                ended = true;
            } else {
                ++gathered;
            }
            if (ready_) {
                add(c);
            } else {
                waiting_.push_back(std::move(c));
            }
            if (!end) {
                start();
            }
        });
    }

    /** The other peer has a remote description: what was held goes now. */
    void ready() {
        ready_ = true;
        for (const auto& c : waiting_) {
            add(c);
        }
        waiting_.clear();
    }

    /** Local candidates gathered, without the end. */
    int gathered = 0;
    /** Whether the end of gathering arrived. */
    bool ended = false;
    /** Candidates the other peer refused. */
    int refused = 0;

private:
    void add(const candidate_type& c) {
        to_.async_add_ice_candidate(c.view(), [this](webrtc::detail::error_code ec) {
            if (ec) {
                ++refused;
            }
        });
    }

    PeerConnection& from_;
    PeerConnection& to_;
    std::vector<candidate_type> waiting_;
    bool ready_ = false;
};

/**
 Negotiates `a` and `b`: an offer from `a`, the answer from `b`, candidates both ways; calls
 `done` with the error of the first step that failed, or none once both descriptions are set.
*/
template <class PeerConnection>
void negotiate(PeerConnection& a, PeerConnection& b, trickle<PeerConnection>& ab,
               trickle<PeerConnection>& ba,
               const std::function<void(webrtc::detail::error_code)>& done) {
    using description = PeerConnection::session_description;
    ab.start();
    ba.start();
    a.async_set_local_description(
        [&a, &b, &ab, &ba, done](webrtc::detail::error_code ec, const description& offer) {
            if (ec) {
                done(ec);
                return;
            }
            BOOST_TEST(offer.type == webrtc::sdp_type::offer);
            b.async_set_remote_description(
                offer.view(), [&a, &b, &ab, &ba, done](webrtc::detail::error_code ec) {
                    if (ec) {
                        done(ec);
                        return;
                    }
                    ab.ready();
                    b.async_set_local_description(
                        [&a, &ba, done](webrtc::detail::error_code ec, const description& answer) {
                            if (ec) {
                                done(ec);
                                return;
                            }
                            BOOST_TEST(answer.type == webrtc::sdp_type::answer);
                            a.async_set_remote_description(
                                answer.view(), [&ba, done](webrtc::detail::error_code ec) {
                                    if (!ec) {
                                        ba.ready();
                                    }
                                    done(ec);
                                });
                        });
                });
        });
}

/** An io_context, a runtime, and two peers on the loopback, made with `opts`. */
struct pair {
    explicit pair(const webrtc::options& opts = webrtc::options(),
                  const webrtc::runtime_options& rt_opts = webrtc::runtime_options())
        : rt(rt_opts) {
        auto first =
            webrtc::peer_connection::make(rt, io.get_executor(), loopback_configuration(), opts);
        auto second =
            webrtc::peer_connection::make(rt, io.get_executor(), loopback_configuration(), opts);
        require(BOOST_TEST(first.has_value()));
        require(BOOST_TEST(second.has_value()));
        a.emplace(std::move(*first));
        b.emplace(std::move(*second));
        ab.emplace(*a, *b);
        ba.emplace(*b, *a);
    }

    /** Negotiates the two peers, trickling candidates both ways. */
    void negotiate(const std::function<void(webrtc::detail::error_code)>& done) {
        test::negotiate(*a, *b, *ab, *ba, done);
    }

    /** Runs the exchange until no work is left, or the limit passes. */
    void run() {
        io.run_for(exchange_limit);  // lint-run: the peers' events come from libdatachannel
    }

    webcpp::asio::io_context io;
    webrtc::runtime rt;
    std::optional<webrtc::peer_connection> a;
    std::optional<webrtc::peer_connection> b;
    std::optional<trickle<webrtc::peer_connection>> ab;
    std::optional<trickle<webrtc::peer_connection>> ba;
};

/**
 Opens one channel labelled `label` from `p.a` to `p.b`, and calls `opened` with both ends once
 both are open.
*/
inline void open_channel(pair& p, std::string_view label,
                         std::function<void(webrtc::data_channel, webrtc::data_channel)> opened,
                         const webrtc::data_channel_init& init = {}) {
    auto created = p.a->create_data_channel(label, init);
    require(BOOST_TEST(created.has_value()));

    struct state {
        std::optional<webrtc::data_channel> local;
        std::optional<webrtc::data_channel> remote;
        bool local_open = false;
        bool remote_open = false;
        std::function<void(webrtc::data_channel, webrtc::data_channel)> opened;

        void check() {
            if (local_open && remote_open) {
                opened(std::move(*local), std::move(*remote));
            }
        }
    };

    const auto s = std::make_shared<state>(state{
        .local = std::move(*created),
        .remote = std::nullopt,
        .local_open = false,
        .remote_open = false,
        .opened = std::move(opened),
    });
    s->local->async_wait_open([s](webrtc::detail::error_code ec) {
        BOOST_TEST(!ec);
        s->local_open = !ec;
        s->check();
    });
    p.b->async_accept_data_channel([s](webrtc::detail::error_code ec, webrtc::data_channel dc) {
        if (!BOOST_TEST(!ec)) {
            return;
        }
        s->remote.emplace(std::move(dc));
        s->remote->async_wait_open([s](webrtc::detail::error_code ec) {
            BOOST_TEST(!ec);
            s->remote_open = !ec;
            s->check();
        });
    });
    p.negotiate([](webrtc::detail::error_code ec) { BOOST_TEST(!ec); });
}

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_WEBRTC_PAIR_HPP
