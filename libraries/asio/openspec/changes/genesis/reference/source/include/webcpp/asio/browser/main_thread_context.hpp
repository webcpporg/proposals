// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_MAIN_THREAD_CONTEXT_HPP
#define WEBCPP_ASIO_BROWSER_MAIN_THREAD_CONTEXT_HPP

#include <webcpp/asio/config.hpp>

#if !WEBCPP_ASIO_HAS_BROWSER_WORKERS && !defined(WEBCPP_ASIO_REFERENCE)
#error \
    "webcpp::asio::browser::main_thread_context requires Emscripten pthreads; select threading=multi"
#else

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <boost/asio/detail/throw_exception.hpp>  // lint-world: Asio's exception policy
#include <boost/asio/execution_context.hpp>  // lint-world: owns the main-thread executor service
#include <boost/assert.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/scope/scope_exit.hpp>

#include <memory>
#include <mutex>
#include <new>
#include <utility>

namespace webcpp::asio::browser {

/**
 An execution context that posts C++ handlers onto the browser's main runtime thread.

 Construct and destroy it on the main browser thread. Its executors may be copied to
 workers and used with post, dispatch, defer and bind_executor. Handlers execute in
 ordinary browser callbacks and must not suspend WebAssembly's stack or call an
 io_context run family.
 JavaScript values remain on their owning thread: obtain DOM values inside the main
 handler, and exchange ordinary C++ data with worker contexts through their executors.

 Destruction discards queued handlers without invoking them. As with an Asio context,
 the context must outlive every call that uses its executors. The Emscripten runtime
 must remain alive until pending work finishes. The SDK owns proxy-queue notification
 storage; handler storage uses the allocator supplied to the executor.
 If moving or invoking a callable throws, remaining queued callables and further
 queued submissions are discarded. The exception propagates to the browser's
 callback boundary; this context does not promise queue recovery after such a failure.
*/
class main_thread_context : public boost::asio::execution_context {
    struct operation : boost::intrusive::list_base_hook<> {
        void (*finish)(operation*, bool);

        explicit operation(void (*function)(operation*, bool)) : finish(function) {}
    };

    template <class Function, class Allocator>
    struct handler_operation final : operation {
        // This is storage ownership, not a uses_allocator construction protocol.
        using storage_allocator =
            std::allocator_traits<Allocator>::template rebind_alloc<handler_operation>;

        handler_operation(Function handler, const storage_allocator& storage)
            : operation(&complete), function(std::move(handler)), allocator(storage) {}

        static void complete(operation* base, bool invoke) {
            auto* self = static_cast<handler_operation*>(base);
            const auto release = [](handler_operation* value) {
                auto allocator = value->allocator;
                std::allocator_traits<storage_allocator>::destroy(allocator, value);
                std::allocator_traits<storage_allocator>::deallocate(allocator, value, 1);
            };
            std::unique_ptr<handler_operation, decltype(release)> owned(self, release);
            if (!invoke) {
                return;
            }
            // A throwing move still destroys the pending operation. Successful moves
            // release its allocation before invoking the callable, as Asio requires.
            auto function = std::move(self->function);
            owned.reset();
            std::move(function)();
        }

        Function function;
        storage_allocator allocator;
    };

    // This state can outlive the Asio service when a handler destroys its context.
    // Allocate it with a copy of the original allocator, not execution_context's
    // allocator adapter, whose implementation is destroyed with that context.
    class control {
    public:
        control() : queue_(em_proxying_queue_create()) {
            if (queue_ == nullptr) {
                boost::asio::detail::throw_exception(std::bad_alloc());
            }
        }

        ~control() { em_proxying_queue_destroy(queue_); }

        void post(operation* pending, const std::shared_ptr<control>& self) {
            std::unique_lock lock(mutex_);
            if (stopped_) {
                lock.unlock();
                pending->finish(pending, false);
                return;
            }
            pending_.push_back(*pending);
            if (active_ || notification_) {
                return;
            }
            // One notification drains our queue. It owns the control even if the
            // context is destroyed before the SDK starts delivering that notification.
            notification_ = self;
            if (!emscripten_proxy_async(queue_, emscripten_main_runtime_thread_id(), &deliver,
                                        this)) {
                notification_.reset();
                pending_.erase(pending_.iterator_to(*pending));
                lock.unlock();
                pending->finish(pending, false);
                boost::asio::detail::throw_exception(std::bad_alloc());
            }
        }

        void stop() {
            boost::intrusive::list<operation> discarded;
            bool drain = false;
            {
                const std::scoped_lock lock(mutex_);
                stopped_ = true;
                discarded.splice(discarded.end(), pending_);
                drain = !active_ && notification_ != nullptr;
            }
            while (!discarded.empty()) {
                auto& pending = discarded.front();
                discarded.pop_front();
                pending.finish(&pending, false);
            }
            // The SDK rejects recursive queue execution. Active delivery already
            // retains this control, and will observe the now-empty stopped queue.
            if (drain) {
                emscripten_proxy_execute_queue(queue_);
            }
        }

    private:
        static void deliver(void* argument) {
            auto& state = *static_cast<control*>(argument);
            std::shared_ptr<control> self;
            {
                const std::scoped_lock lock(state.mutex_);
                self = std::move(state.notification_);
                state.active_ = true;
            }
            bool finished = false;
            const boost::scope::scope_exit cleanup([&] {
                if (!finished) {
                    // A throwing callable must not strand queued allocations or an
                    // active flag. The exception still propagates to the browser.
                    self->stop();
                    const std::scoped_lock lock(self->mutex_);
                    self->active_ = false;
                }
            });
            for (;;) {
                operation* pending = nullptr;
                {
                    const std::scoped_lock lock(self->mutex_);
                    if (self->pending_.empty()) {
                        // Atomically release the drain so a concurrent post schedules
                        // a fresh notification rather than missing this wakeup.
                        self->active_ = false;
                        finished = true;
                        return;
                    }
                    pending = &self->pending_.front();
                    self->pending_.pop_front();
                }
                pending->finish(pending, true);
            }
        }

        em_proxying_queue* queue_;
        std::mutex mutex_;
        boost::intrusive::list<operation> pending_;
        std::shared_ptr<control> notification_;
        bool stopped_ = false;
        bool active_ = false;
    };

    class service final : public boost::asio::execution_context::service {
    public:
        // Boost's service identity has an empty constructor and no dynamic dependencies.
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,bugprone-throwing-static-initialization,cert-err58-cpp,bugprone-dynamic-static-initializers)
        static inline boost::asio::execution_context::id id;

        explicit service(boost::asio::execution_context& context)
            : boost::asio::execution_context::service(context) {}

        template <class Allocator>
        void start(const Allocator& allocator) {
            state_ = std::allocate_shared<control>(allocator);
        }

        template <class Function, class Allocator>
        void post(Function function, const Allocator& allocator) {
            using pending_type = handler_operation<Function, Allocator>;
            typename pending_type::storage_allocator storage(allocator);
            auto* pending =
                std::allocator_traits<typename pending_type::storage_allocator>::allocate(storage,
                                                                                          1);
            // The guard also handles an exception from the user's function move constructor.
            const auto release = [storage](pending_type* value) {
                auto allocator_copy = storage;
                std::allocator_traits<typename pending_type::storage_allocator>::deallocate(
                    allocator_copy, value, 1);
            };
            std::unique_ptr<pending_type, decltype(release)> raw(pending, release);
            std::allocator_traits<typename pending_type::storage_allocator>::construct(
                storage, pending, std::move(function), storage);
            (void)raw.release();
            const auto state = state_;
            state->post(pending, state);
        }

    private:
        void shutdown() override {
            BOOST_ASSERT_MSG(emscripten_is_main_browser_thread(),
                             "main_thread_context must be destroyed on the browser main thread");
            if (state_) {
                state_->stop();
                state_.reset();
            }
        }

        std::shared_ptr<control> state_;
    };

public:
    /** A standard Asio executor for nonsuspending handlers on the browser main thread. */
    class executor_type {
    public:
        /** Constructs an empty executor, usable only for assignment or comparison. */
        executor_type() = default;

        /**
         Returns the context, which must still be alive.

         @return The execution context owning this executor.
        */
        [[nodiscard]] main_thread_context& context() const noexcept {
            return static_cast<main_thread_context&>(owner_->context());
        }

        /** The browser runtime lifetime is controlled by the application. */
        void on_work_started() const noexcept {
        }  // NOLINT(readability-convert-member-functions-to-static)

        /** The browser runtime lifetime is controlled by the application. */
        void on_work_finished() const noexcept {
        }  // NOLINT(readability-convert-member-functions-to-static)

        /**
         Dispatches inline on the browser main thread, otherwise posts.

         @tparam Function A nullary callable that does not suspend WebAssembly's stack.
         @tparam Allocator A standard allocator for pending handler storage.
         @param function The callable.
         @param allocator The allocator used when dispatch must post.
        */
        template <class Function, class Allocator>
        void dispatch(Function&& function, const Allocator& allocator) const {
            if (emscripten_is_main_browser_thread()) {
                std::forward<Function>(function)();
                return;
            }
            post(std::forward<Function>(function), allocator);
        }

        /**
         Posts without invoking inline, using allocator for pending handler storage.

         @tparam Function A nullary callable that does not suspend WebAssembly's stack.
         @tparam Allocator A standard allocator for pending handler storage.
         @param function The callable.
         @param allocator The pending handler's allocator.
        */
        template <class Function, class Allocator>
        void post(Function&& function, const Allocator& allocator) const {
            owner_->post(std::forward<Function>(function), allocator);
        }

        /**
         Defers through the same main-thread queue as post.

         @tparam Function A nullary callable that does not suspend WebAssembly's stack.
         @tparam Allocator A standard allocator for pending handler storage.
         @param function The callable.
         @param allocator The pending handler's allocator.
        */
        template <class Function, class Allocator>
        void defer(Function&& function, const Allocator& allocator) const {
            post(std::forward<Function>(function), allocator);
        }

        /**
         Compares the contexts owning two executors.

         @return True when the executors belong to the same context.
        */
        friend bool operator==(const executor_type&, const executor_type&) = default;

    private:
        friend class main_thread_context;

        explicit executor_type(service& owner) : owner_(&owner) {}

        service* owner_ = nullptr;
    };

    /** Constructs a queue on the browser's main runtime thread. */
    main_thread_context() : service_(boost::asio::use_service<service>(*this)) {
        service_.start(std::allocator<void>());
        BOOST_ASSERT_MSG(emscripten_is_main_browser_thread(),
                         "main_thread_context must be constructed on the browser main thread");
    }

    /**
     Constructs a main-thread context whose Asio service storage uses allocator.

     @tparam Allocator A standard allocator for context-owned C++ state.
     @param allocator The allocator for context-owned C++ state. The Emscripten SDK
     allocates its own proxy queue and notifications separately.
     If a callback destroys this context, its allocator resource must remain alive
     until that callback returns and the retained queue control is released.
    */
    template <class Allocator>
    main_thread_context(std::allocator_arg_t /*tag*/, const Allocator& allocator)
        : boost::asio::execution_context(std::allocator_arg, allocator),
          service_(boost::asio::use_service<service>(*this)) {
        service_.start(allocator);
        BOOST_ASSERT_MSG(emscripten_is_main_browser_thread(),
                         "main_thread_context must be constructed on the browser main thread");
    }

    /**
     Returns an executor that can safely be copied to workers.

     @return An executor submitting work to this context on the browser main thread.
    */
    [[nodiscard]] executor_type get_executor() noexcept { return executor_type(service_); }

private:
    service& service_;
};

}  // namespace webcpp::asio::browser

#endif
#endif  // WEBCPP_ASIO_BROWSER_MAIN_THREAD_CONTEXT_HPP
