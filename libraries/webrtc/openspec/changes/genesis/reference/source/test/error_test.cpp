// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The category webcpp.webrtc and its ten values, fixed and never reused (spec: "Errors: the error
// category and its ten values").

#include <webcpp/webrtc/error.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/error_code.hpp>

#include <cstring>

namespace webrtc = webcpp::webrtc;

namespace {

void the_category() {
    const boost::system::error_code ec = webrtc::error::closed;
    BOOST_TEST(std::strcmp(ec.category().name(), "webcpp.webrtc") == 0);
    BOOST_TEST_EQ(ec.value(), 6);
    BOOST_TEST(ec == webrtc::error::closed);
    BOOST_TEST(!ec.message().empty());
}

void the_values() {
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::invalid_state), 1);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::operation_error), 2);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::invalid_access), 3);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::type_error), 4);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::syntax_error), 5);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::closed), 6);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::message_too_large), 7);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::buffered_amount_exceeded), 8);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::receive_overflow), 9);
    BOOST_TEST_EQ(static_cast<int>(webrtc::error::too_many_peer_connections), 10);
}

void every_value_has_a_message() {
    for (int value = 1; value <= 10; ++value) {
        BOOST_TEST(webrtc::webrtc_category().message(value) !=
                   webrtc::webrtc_category().message(0));
    }
}

}  // namespace

int main() {
    the_category();
    the_values();
    every_value_has_a_message();
    return boost::report_errors();
}
