// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_TLS_ERROR_PEER_HPP
#define WEBCPP_TEST_TLS_ERROR_PEER_HPP

#include <boost/config.hpp>
#include <boost/system/error_code.hpp>

namespace webcpp::test {
#ifdef WEBCPP_TEST_ERROR_EXPORT
BOOST_SYMBOL_EXPORT
#else
BOOST_SYMBOL_IMPORT
#endif
boost::system::error_code tls_error_from_library(int value);
}  // namespace webcpp::test
#endif
