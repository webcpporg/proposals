// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_ASYNC_AWAIT_HPP
#define WEBCPP_ASIO_BROWSER_ASYNC_AWAIT_HPP

#include <webcpp/asio/config.hpp>

#include <boost/asio/any_io_executor.hpp>  // lint-world: the operation's executor
#include <boost/asio/append.hpp>  // lint-world: preserve handler associations when posting values
#include <boost/asio/associated_allocator.hpp>  // lint-world: operation storage belongs to its handler
#include <boost/asio/associated_cancellation_slot.hpp>  // lint-world: cancel an outstanding operation
#include <boost/asio/associated_executor.hpp>  // lint-world: retain the handler executor while pending
#include <boost/asio/async_result.hpp>         // lint-world: completion token initiation
#include <boost/asio/error.hpp>                // lint-world: Asio's cancellation error
#include <boost/asio/executor_work_guard.hpp>  // lint-world: keep both contexts alive until completion
#include <boost/asio/post.hpp>  // lint-world: completions only post to the event loop
#include <boost/smart_ptr/allocate_unique.hpp>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/browser/error.hpp>

namespace webcpp::asio::browser {
namespace detail {
/** One Promise wait, owned by its bridge until completion or shutdown. */
template <class Handler, class Executor>
class await_operation final : public registration {
public:
    /** Allocates this operation with the handler's associated allocator. */
    using storage_allocator = std::allocator_traits<
        boost::asio::associated_allocator_t<Handler>>::template rebind_alloc<await_operation>;

    /** Constructs a pending wait that owns outstanding work on executor. */
    await_operation(bridge& owner, const Executor& executor, Handler handler)
        : bridge_(owner),
          work_(boost::asio::prefer(executor, boost::asio::execution::outstanding_work_t::tracked)),
          completion_work_(boost::asio::get_associated_executor(handler, executor)),
          handler_(std::move(handler)) {}

    /** Removes a registered wait, including when initiation throws. */
    ~await_operation() {
        if (id_ != 0) {
            bridge_.remove(id_);
            boost::asio::get_associated_cancellation_slot(handler_).clear();
        }
    }

    /** Not copyable: the bridge and cancellation callback refer to this address. */
    await_operation(const await_operation&) = delete;
    /** Not copy-assignable: the registration has one owner. */
    await_operation& operator=(const await_operation&) = delete;
    /** Not movable: registered callbacks refer to this address. */
    await_operation(await_operation&&) = delete;
    /** Not move-assignable: registered callbacks refer to this address. */
    await_operation& operator=(await_operation&&) = delete;

    /** Registers settlement and cancellation reactions for promise. */
    void start(const emscripten::val& promise) {
        id_ = bridge_.add(*this);
        auto slot = boost::asio::get_associated_cancellation_slot(handler_);
        if (slot.is_connected()) {
            slot.assign([this](boost::asio::cancellation_type kind) {
                if ((kind & boost::asio::cancellation_type::all) !=
                    boost::asio::cancellation_type::none) {
                    complete(boost::asio::error::operation_aborted, emscripten::val::undefined());
                }
            });
        }
        webcpp_asio_await(promise.as_handle(), bridge_.function(id_).as_handle());
    }

    /** Posts the resolved value or rejection reason and releases this registration. */
    bool deliver(const emscripten::val& envelope) override {
        const bool fulfilled = envelope["ok"].as<bool>();
        complete(fulfilled ? boost::system::error_code()
                           : error::make_error_code(error::promise_rejected),
                 envelope["value"]);
        return true;
    }

    /** Destroys the pending handler without calling it when its context shuts down. */
    void shutdown() noexcept override {
        destroy(storage_allocator(boost::asio::get_associated_allocator(handler_)));
    }

private:
    /** Removes registration before freeing state and posting its completion. */
    void complete(boost::system::error_code ec, emscripten::val value) {
        boost::asio::get_associated_cancellation_slot(handler_).clear();
        bridge_.remove(id_);
        id_ = 0;
        const storage_allocator allocator(boost::asio::get_associated_allocator(handler_));
        const auto cleanup = [allocator](await_operation* operation) {
            operation->destroy(allocator);
        };
        std::unique_ptr<await_operation, decltype(cleanup)> owned(this, cleanup);
        auto handler = std::move(handler_);
        const auto executor = work_;
        const auto completion_work = completion_work_;
        owned.reset();
        boost::asio::post(executor, boost::asio::append(std::move(handler), ec, std::move(value)));
    }

    /** Destroys and deallocates this wait through its associated allocator. */
    void destroy(storage_allocator allocator) noexcept {
        std::allocator_traits<storage_allocator>::destroy(allocator, this);
        std::allocator_traits<storage_allocator>::deallocate(allocator, this, 1);
    }

    bridge& bridge_;
    decltype(boost::asio::prefer(std::declval<Executor>(),
                                 boost::asio::execution::outstanding_work_t::tracked)) work_;
    boost::asio::executor_work_guard<boost::asio::associated_executor_t<Handler, Executor>>
        completion_work_;
    Handler handler_;
    std::uint64_t id_ = 0;
};

/** Allocates and starts a wait once the completion token initiates it. */
struct initiate_await {
    /** Starts the operation with handler, executor and promise. */
    template <class Handler, class Executor>
    void operator()(Handler&& handler, const Executor& executor,
                    const emscripten::val& promise) const {
        using operation = await_operation<std::decay_t<Handler>, Executor>;
        auto& context = boost::asio::query(executor, boost::asio::execution::context);
        auto& owner = boost::asio::use_service<bridge>(context);
        const typename operation::storage_allocator allocator(
            boost::asio::get_associated_allocator(handler));
        auto pending = boost::allocate_unique<operation>(allocator, owner, executor,
                                                         std::forward<Handler>(handler));
        pending->start(promise);
        (void)pending.release();
    }
};
}  // namespace detail

/**
 Asynchronously receives a JavaScript Promise's value or rejection reason.

 Completion is posted through the bridge, never invoked inline. Terminal,
 partial or total cancellation posts operation_aborted and undefined; it does
 not cancel the JavaScript Promise. A pending wait owns outstanding work.
 Destroying its context destroys its handler without calling it. Operation
 storage uses the handler's associated allocator.

 @tparam Executor The concrete or type-erased executor of the owning context.
 @tparam CompletionToken A token for void(boost::system::error_code, emscripten::val).
 @param executor The executor whose context owns the wait.
 @param promise A Promise or a value normalized by JavaScript Promise.resolve.
 @param token The completion token.
 @return The result determined by token.
 @see "Promises and events", in the guide.
*/
template <class CompletionToken, class Executor = boost::asio::any_io_executor>
auto async_await(const Executor& executor, emscripten::val promise, CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken,
                                       void(boost::system::error_code, emscripten::val)>(
        detail::initiate_await{}, token, executor, std::move(promise));
}
}  // namespace webcpp::asio::browser

#endif  // WEBCPP_ASIO_BROWSER_ASYNC_AWAIT_HPP
