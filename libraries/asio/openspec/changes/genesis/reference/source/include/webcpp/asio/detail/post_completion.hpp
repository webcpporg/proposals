// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_DETAIL_POST_COMPLETION_HPP
#define WEBCPP_ASIO_DETAIL_POST_COMPLETION_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/any_io_executor.hpp>  // lint-world: preserve an erased executor
#include <boost/asio/io_context.hpp>  // lint-world: recover an ordinary context executor without erasing its allocator
#include <boost/asio/post.hpp>  // lint-world: completion always enters through a post
#include <utility>

namespace webcpp::asio::detail {
/** Posts without erasing an ordinary io_context executor's allocator property. */
template <class Handler>
void post_completion(const boost::asio::any_io_executor& executor, Handler&& handler) {
    using plain = boost::asio::io_context::executor_type;
    using tracked = decltype(boost::asio::prefer(
        std::declval<plain>(), boost::asio::execution::outstanding_work_t::tracked));
    if (const auto* concrete = executor.target<plain>()) {
        boost::asio::post(*concrete, std::forward<Handler>(handler));
    } else if (const auto* concrete = executor.target<tracked>()) {
        boost::asio::post(*concrete, std::forward<Handler>(handler));
    } else {
        // Keep custom executor and strand semantics; they own their dispatch allocation policy.
        boost::asio::post(executor, std::forward<Handler>(handler));
    }
}
}  // namespace webcpp::asio::detail
#endif  // WEBCPP_ASIO_DETAIL_POST_COMPLETION_HPP
