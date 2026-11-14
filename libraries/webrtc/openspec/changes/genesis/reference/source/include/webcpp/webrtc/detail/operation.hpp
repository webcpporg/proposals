// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DETAIL_OPERATION_HPP
#define WEBCPP_WEBRTC_DETAIL_OPERATION_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/asio/append.hpp>                        // lint-world: completions are posted
#include <webcpp/asio/associated_allocator.hpp>          // lint-world: an operation's storage
#include <webcpp/asio/associated_cancellation_slot.hpp>  // lint-world: per-operation cancellation
#include <webcpp/asio/associated_executor.hpp>           // lint-world: the handler's executor
#include <webcpp/asio/cancellation_type.hpp>             // lint-world: per-operation cancellation
#include <webcpp/asio/error.hpp>                         // lint-world: Asio's codes
#include <webcpp/asio/executor_work_guard.hpp>           // lint-world: pending work keeps run alive
#include <webcpp/asio/post.hpp>                          // lint-world: completions are posted

#include <boost/system/error_code.hpp>

#include <memory>
#include <tuple>
#include <utility>

namespace webcpp::webrtc::detail {

/** The error type of every completion. */
using boost::system::error_code;

/**
 A pending operation that completes once, posting its handler with an error and `Values`.

 Its owner, an object's state, keeps it in a slot (a pointer) while it waits; the operation clears
 that slot when it ends, whoever ends it: the state, a cancellation, or the io_context's shutdown.
*/
template <class... Values>
class pending {
public:
    pending(const pending&) = delete;
    pending& operator=(const pending&) = delete;
    pending(pending&&) = delete;
    pending& operator=(pending&&) = delete;

    /** Posts the handler with `ec` and `values`, and frees the operation. */
    void complete(error_code ec, Values... values) {
        unlink();
        do_complete(ec, std::move(values)...);
    }

    /** Posts the handler with `ec` and the values the operation was made with, and frees it. */
    void fail(error_code ec) {
        unlink();
        do_fail(ec);
    }

    /** Frees the operation without calling its handler, as Asio's shutdown destroys operations. */
    void discard() noexcept {
        unlink();
        do_discard();
    }

    /** Puts the operation in `slot`, which it clears when it ends. */
    void link(pending*& slot) noexcept {
        slot_ = &slot;
        slot = this;
    }

protected:
    pending() = default;
    ~pending() = default;

private:
    /** Posts the handler with `ec` and `values`, and frees the operation. */
    virtual void do_complete(error_code ec, Values... values) = 0;
    /** Posts the handler with `ec` and the values the operation was made with, and frees it. */
    virtual void do_fail(error_code ec) = 0;
    /** Frees the operation without calling its handler. */
    virtual void do_discard() noexcept = 0;

    void unlink() noexcept {
        if (slot_ != nullptr && *slot_ == this) {
            *slot_ = nullptr;
        }
        slot_ = nullptr;
    }

    pending** slot_ = nullptr;
};

/**
 The operation of a handler: its storage in the handler's associated allocator, outstanding work
 on the object's executor and on the handler's, and the handler's cancellation slot.
*/
template <class Handler, class IoExecutor, class... Values>
class operation final : public pending<Values...> {
    using handler_executor = webcpp::asio::associated_executor_t<Handler, IoExecutor>;
    using allocator_type = std::allocator_traits<
        webcpp::asio::associated_allocator_t<Handler>>::template rebind_alloc<operation>;

public:
    /**
     Makes the operation of `handler`, in its associated allocator.

     @param handler The completion handler.
     @param executor The object's executor, which the completion is posted to.
     @param fallback The values a failure completes with.
     @param supported The cancellation types the operation honours; none installs no handler.
     @return The operation, which frees itself when it ends.
    */
    static operation& make(Handler handler, const IoExecutor& executor,
                           std::tuple<Values...> fallback,
                           webcpp::asio::cancellation_type supported) {
        allocator_type allocator(webcpp::asio::get_associated_allocator(handler));
        operation* storage = std::allocator_traits<allocator_type>::allocate(allocator, 1);
        std::allocator_traits<allocator_type>::construct(allocator, storage, std::move(handler),
                                                         executor, std::move(fallback));
        storage->start(supported);
        return *storage;
    }

    /** Constructs the operation; `make` is the way to make one. */
    operation(Handler handler, IoExecutor executor, std::tuple<Values...> fallback)
        : handler_(std::move(handler)),
          executor_(std::move(executor)),
          work_(executor_),
          handler_work_(webcpp::asio::get_associated_executor(handler_, executor_)),
          fallback_(std::move(fallback)) {}

    operation(const operation&) = delete;
    operation& operator=(const operation&) = delete;
    operation(operation&&) = delete;
    operation& operator=(operation&&) = delete;
    ~operation() = default;

private:
    void start(webcpp::asio::cancellation_type supported) {
        auto slot = webcpp::asio::get_associated_cancellation_slot(handler_);
        if (slot.is_connected() && supported != webcpp::asio::cancellation_type::none) {
            // The handler runs where the cancellation is emitted, on the object's executor as
            // Asio requires of an I/O object; it ends the operation, which clears the slot, and
            // touches nothing of itself afterwards.
            slot.assign([this, supported](webcpp::asio::cancellation_type type) {
                if ((type & supported) != webcpp::asio::cancellation_type::none) {
                    this->fail(webcpp::asio::error::operation_aborted);
                }
            });
        }
    }

    void do_complete(error_code ec, Values... values) override { finish(ec, std::move(values)...); }

    void do_fail(error_code ec) override {
        std::apply([this, ec](Values&... values) { finish(ec, std::move(values)...); }, fallback_);
    }

    void do_discard() noexcept override {
        webcpp::asio::get_associated_cancellation_slot(handler_).clear();
        destroy(allocator_type(webcpp::asio::get_associated_allocator(handler_)));
    }

    template <class... Args>
    void finish(error_code ec, Args&&... values) {
        webcpp::asio::get_associated_cancellation_slot(handler_).clear();
        // The allocator is taken before the handler moves out, which may leave it without one.
        const allocator_type allocator(webcpp::asio::get_associated_allocator(handler_));
        // Post before the work guards go with the operation: the queued completion keeps run
        // alive from then on.
        webcpp::asio::post(executor_, webcpp::asio::append(std::move(handler_), ec,
                                                           std::forward<Args>(values)...));
        destroy(allocator);
    }

    void destroy(allocator_type allocator) noexcept {
        std::allocator_traits<allocator_type>::destroy(allocator, this);
        std::allocator_traits<allocator_type>::deallocate(allocator, this, 1);
    }

    Handler handler_;
    IoExecutor executor_;
    webcpp::asio::executor_work_guard<IoExecutor> work_;
    webcpp::asio::executor_work_guard<handler_executor> handler_work_;
    std::tuple<Values...> fallback_;
};

/** The cancellation types a wait honours: it consumes nothing when cancelled. */
inline constexpr webcpp::asio::cancellation_type wait_cancellation =
    webcpp::asio::cancellation_type::terminal | webcpp::asio::cancellation_type::partial |
    webcpp::asio::cancellation_type::total;

/**
 Makes the operation of `handler` and returns it as the pending operation of its signature.

 @param handler The completion handler.
 @param executor The object's executor.
 @param fallback The values a failure completes with.
 @param supported The cancellation types the operation honours.
 @return The operation.
*/
template <class... Values, class Handler, class IoExecutor>
pending<Values...>& make_operation(Handler&& handler, const IoExecutor& executor,
                                   std::tuple<Values...> fallback,
                                   webcpp::asio::cancellation_type supported) {
    using type = operation<std::decay_t<Handler>, IoExecutor, Values...>;
    return type::make(std::forward<Handler>(handler), executor, std::move(fallback), supported);
}

}  // namespace webcpp::webrtc::detail

#endif  // WEBCPP_WEBRTC_DETAIL_OPERATION_HPP
