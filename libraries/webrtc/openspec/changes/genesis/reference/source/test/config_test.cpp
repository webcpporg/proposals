// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/webrtc/config.hpp> defines WEBCPP_WEBRTC_NO_EXCEPTIONS exactly when webcpp has no
// exceptions, and keeps the developer's own definition in a build that has them; it selects the
// unsupported policy, error by default. test/Jamfile builds this file three times: as the build
// asks; with WEBCPP_NO_EXCEPTIONS, which WEBCPP_TEST_WEBRTC_ALL_OFF tells this file; and with the
// macro itself defined, which WEBCPP_TEST_WEBRTC_DEVELOPER_DEFINED tells this file.

#include <webcpp/webrtc/config.hpp>

#ifndef WEBCPP_CONFIG_HPP
#error "config.hpp does not include <webcpp/config.hpp>"
#endif

#ifdef WEBCPP_TEST_WEBRTC_ALL_OFF
#ifndef WEBCPP_WEBRTC_NO_EXCEPTIONS
#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_WEBRTC_NO_EXCEPTIONS"
#endif
#elif defined(WEBCPP_TEST_WEBRTC_DEVELOPER_DEFINED)
#ifndef WEBCPP_WEBRTC_NO_EXCEPTIONS
#error "config.hpp drops WEBCPP_WEBRTC_NO_EXCEPTIONS, which the developer defined"
#endif
#elif defined(WEBCPP_HAS_EXCEPTIONS) == defined(WEBCPP_WEBRTC_NO_EXCEPTIONS)
#error "WEBCPP_WEBRTC_NO_EXCEPTIONS is not defined exactly when webcpp has no exceptions"
#endif

#if !defined(WEBCPP_WEBRTC_UNSUPPORTED_ERROR) == !defined(WEBCPP_WEBRTC_UNSUPPORTED_IGNORE)
#error "config.hpp selects exactly one unsupported policy"
#endif
