// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_CONFIG_HPP
#define WEBCPP_BEAST_CONFIG_HPP

#include <webcpp/config.hpp>

#ifdef __MRDOCS__
/**
 Records the facade's exception policy for host-adapter implementations.

 Defined automatically when webcpp has no exceptions, or explicitly by the application.
 This initial socket facade has no throwing overloads of its own. Inherited
 upstream overloads and data types retain Boost's own exception configuration.
*/
#define WEBCPP_BEAST_NO_EXCEPTIONS
#undef WEBCPP_BEAST_NO_EXCEPTIONS
/** Reports unsupported host operations as errors, the default policy. */
#define WEBCPP_BEAST_UNSUPPORTED_ERROR
#undef WEBCPP_BEAST_UNSUPPORTED_ERROR
/** Allows documented unsupported settings to be ignored by host adapters. */
#define WEBCPP_BEAST_UNSUPPORTED_IGNORE
#undef WEBCPP_BEAST_UNSUPPORTED_IGNORE
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_BEAST_NO_EXCEPTIONS)
#define WEBCPP_BEAST_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_BEAST_UNSUPPORTED_ERROR) && defined(WEBCPP_BEAST_UNSUPPORTED_IGNORE)
#error "Select only one WEBCPP_BEAST_UNSUPPORTED_ERROR or WEBCPP_BEAST_UNSUPPORTED_IGNORE policy"
#elif !defined(WEBCPP_BEAST_UNSUPPORTED_ERROR) && !defined(WEBCPP_BEAST_UNSUPPORTED_IGNORE)
/** Reports unsupported host operations as errors, the default policy. */
#define WEBCPP_BEAST_UNSUPPORTED_ERROR
#endif

#endif
