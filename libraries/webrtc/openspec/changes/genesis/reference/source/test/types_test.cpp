// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The value types: their defaults, the views of the outputs, and the outputs' allocator (spec:
// "Types: ...", "Options are webcpp's own bounds").

#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/types.hpp>

#include <boost/core/lightweight_test.hpp>

#include <memory_resource>
#include <string>

namespace webrtc = webcpp::webrtc;

namespace {

void defaults() {
    const webrtc::data_channel_init init;
    BOOST_TEST(init.ordered);
    BOOST_TEST(!init.negotiated);
    BOOST_TEST(!init.max_packet_life_time);
    BOOST_TEST(!init.max_retransmits);
    BOOST_TEST(!init.id);
    BOOST_TEST(init.protocol.empty());
    const webrtc::configuration config;
    BOOST_TEST(config.policy == webrtc::ice_transport_policy::all);
    BOOST_TEST(config.bind_address.empty());
    BOOST_TEST(config.ice_servers.empty());
}

void option_defaults() {
    const webrtc::options opts;
    BOOST_TEST_EQ(opts.max_queued_candidates, 64u);
    BOOST_TEST_EQ(opts.max_queued_channels, 16u);
    BOOST_TEST_EQ(opts.max_message_size, 262144u);
    BOOST_TEST_EQ(opts.max_buffered_amount, 16777216u);
    BOOST_TEST_EQ(opts.max_received_bytes, 16777216u);
    const webrtc::runtime_options rt;
    BOOST_TEST_EQ(rt.max_peer_connections, 256u);
    BOOST_TEST_EQ(rt.thread_pool_size, 0u);
}

void enumerations_start_at_new() {
    BOOST_TEST(webrtc::peer_connection_state{} == webrtc::peer_connection_state::new_);
    BOOST_TEST(webrtc::ice_connection_state{} == webrtc::ice_connection_state::new_);
    BOOST_TEST(webrtc::data_channel_state{} == webrtc::data_channel_state::connecting);
}

void a_description_round_trip() {
    webrtc::session_description d{.type = webrtc::sdp_type::answer, .sdp = "v=0\r\n"};
    const webrtc::session_description_view view = d.view();
    BOOST_TEST(view.type == webrtc::sdp_type::answer);
    BOOST_TEST(view.sdp == d.sdp);
    BOOST_TEST(view.sdp.data() == d.sdp.data());
}

void a_candidate_view() {
    const webrtc::ice_candidate c{
        .candidate = "candidate:1 1 UDP 1 127.0.0.1 9 typ host",
        .sdp_mid = "0",
        .sdp_m_line_index = 0,
        .username_fragment = "",
    };
    const webrtc::ice_candidate_view view = c.view();
    BOOST_TEST(view.candidate == c.candidate);
    BOOST_TEST(view.sdp_mid && *view.sdp_mid == "0");
    BOOST_TEST(view.sdp_m_line_index && *view.sdp_m_line_index == 0);
    const webrtc::ice_candidate no_mid{
        .candidate = "",
        .sdp_mid = "",
        .sdp_m_line_index = std::nullopt,
        .username_fragment = "",
    };
    BOOST_TEST(!no_mid.view().sdp_mid);
}

void a_candidate_under_a_pmr_allocator() {
    std::pmr::monotonic_buffer_resource resource;
    using allocator = std::pmr::polymorphic_allocator<std::byte>;
    const allocator a(&resource);
    const webrtc::basic_ice_candidate<allocator> c{
        .candidate = webrtc::detail::string<allocator>(std::string(64, 'c'), a),
        .sdp_mid = webrtc::detail::string<allocator>("0", a),
        .sdp_m_line_index = 0,
        .username_fragment = webrtc::detail::string<allocator>(a),
    };
    BOOST_TEST(c.candidate.get_allocator().resource() == &resource);
    BOOST_TEST(c.sdp_mid.get_allocator().resource() == &resource);
}

}  // namespace

int main() {
    defaults();
    option_defaults();
    enumerations_start_at_new();
    a_description_round_trip();
    a_candidate_view();
    a_candidate_under_a_pmr_allocator();
    return boost::report_errors();
}
