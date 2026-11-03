// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_HTTP_HPP
#define WEBCPP_BEAST_HTTP_HPP

#include <webcpp/beast/config.hpp>

#include <boost/beast/http/buffer_body.hpp>   // lint-world: caller-owned memory buffers only
#include <boost/beast/http/dynamic_body.hpp>  // lint-world: memory buffers only
#include <boost/beast/http/empty_body.hpp>    // lint-world: a body without a payload
#include <boost/beast/http/file_body.hpp>     // lint-world: explicit file-backed HTTP bodies
#include <boost/beast/http/message.hpp>       // lint-world: HTTP message data only
#include <boost/beast/http/parser.hpp>        // lint-world: parses caller-supplied bytes only
#include <boost/beast/http/serializer.hpp>    // lint-world: serializes into caller-owned buffers
#include <boost/beast/http/string_body.hpp>   // lint-world: memory buffers only

namespace webcpp::beast::http {
/** Upstream HTTP fields with an application-supplied allocator. */
using boost::beast::http::basic_fields;
/** Upstream string body with an application-supplied allocator. */
using boost::beast::http::basic_string_body;
/** Upstream body that reads or writes caller-owned buffers. */
using boost::beast::http::buffer_body;
/** Upstream dynamically buffered body. */
using boost::beast::http::dynamic_body;
/** Upstream body for messages without a payload. */
using boost::beast::http::empty_body;
/** Upstream HTTP parser and serializer errors. */
using boost::beast::http::error;
/** Upstream HTTP field names. */
using boost::beast::http::field;
/** Upstream HTTP fields. */
using boost::beast::http::fields;
/** Upstream file-backed message body. */
using boost::beast::http::file_body;
/** Upstream request or response header. */
using boost::beast::http::header;
/** Upstream HTTP request or response message. */
using boost::beast::http::message;
/** Upstream incremental message parser. */
using boost::beast::http::parser;
/** Upstream HTTP request. */
using boost::beast::http::request;
/** Upstream HTTP request header. */
using boost::beast::http::request_header;
/** Upstream incremental request parser. */
using boost::beast::http::request_parser;
/** Upstream incremental request serializer. */
using boost::beast::http::request_serializer;
/** Upstream HTTP response. */
using boost::beast::http::response;
/** Upstream HTTP response header. */
using boost::beast::http::response_header;
/** Upstream incremental response parser. */
using boost::beast::http::response_parser;
/** Upstream incremental response serializer. */
using boost::beast::http::response_serializer;
/** Upstream incremental message serializer. */
using boost::beast::http::serializer;
/** Upstream HTTP status codes. */
using boost::beast::http::status;
/** Upstream string message body. */
using boost::beast::http::string_body;
/** Parses a method name while preserving upstream method semantics. */
using boost::beast::http::string_to_verb;
/** Upstream HTTP methods. */
using boost::beast::http::verb;
}  // namespace webcpp::beast::http

#endif
