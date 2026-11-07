// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_BROWSER_DETAIL_SUPPORT_HPP
#define WEBCPP_BEAST_BROWSER_DETAIL_SUPPORT_HPP

#include <webcpp/beast/config.hpp>

#include <algorithm>
#include <boost/asio/append.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/associated_allocator.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/associated_cancellation_slot.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/associated_executor.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/async_result.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/bind_allocator.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/error.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/asio/executor_work_guard.hpp>  // lint-world: browser operation scheduling and handler associations
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <chrono>
#include <limits>
#include <memory>
#include <memory_resource>
#include <utility>
#include <webcpp/asio/detail/post_completion.hpp>
#include <webcpp/beast/browser/error.hpp>

// NOLINTNEXTLINE(modernize-concat-nested-namespaces): policy selects an inline namespace
namespace webcpp::beast::detail {
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
inline namespace unsupported_ignore {
#else
inline namespace unsupported_error {
#endif
namespace browser {
/** Monotonic clock used only to enforce browser operation deadlines. */
using clock_type = std::chrono::steady_clock;  // lint-world: browser operation deadlines
/**
 Whether advisory browser limitations are ignored in this translation unit.
 */
inline constexpr bool ignore_unsupported =
#ifdef WEBCPP_BEAST_UNSUPPORTED_IGNORE
    true;
#else
    false;
#endif
/**
 Error value shared by browser operations.
 */
using boost::system::error_code;
/**
 Executor used to schedule stream operations and their completions.
 */
using executor_type = boost::asio::any_io_executor;

/**
 Maps an unsupported request to the selected compatibility policy.
 */
inline error_code unsupported(bool condition = true) {
    return condition && !ignore_unsupported
               ? error_code(boost::asio::error::operation_not_supported)
               : error_code{};
}

/**
 Throws a system error when the supplied error code is nonzero.
 */
inline void throw_error(error_code ec) {
    if (ec) {
        boost::throw_exception(boost::system::system_error(ec));
    }
}

/**
 Posts a completion while preserving the handler associations.
 */
template <class Handler, class... Values>
void post(const executor_type& executor, Handler&& handler, Values&&... values) {
    webcpp::asio::detail::post_completion(
        executor,
        boost::asio::append(std::forward<Handler>(handler), std::forward<Values>(values)...));
}

/**
 Erases the stream allocator while retaining it for all variable-sized state.
 */
template <class Allocator>
class allocator_resource final : public std::pmr::memory_resource {
    using unit = std::max_align_t;
    using allocator_type = std::allocator_traits<Allocator>::template rebind_alloc<unit>;
    allocator_type allocator_;

    static std::size_t units(std::size_t bytes, std::size_t alignment) {
        const auto overhead =
            std::max(alignment, alignof(unit*)) + sizeof(unit*) + sizeof(unit) - 1;
        if (bytes > std::numeric_limits<std::size_t>::max() - overhead) {
            boost::throw_exception(std::bad_alloc());
        }
        return (bytes + overhead) / sizeof(unit);
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        alignment = std::max(alignment, alignof(unit*));
        const auto count = units(bytes, alignment);
        auto* storage = std::allocator_traits<allocator_type>::allocate(allocator_, count);
        void* value = reinterpret_cast<std::byte*>(storage) + sizeof(unit*);
        auto available = (count * sizeof(unit)) - sizeof(unit*);
        void* aligned = std::align(alignment, bytes, value, available);
        static_cast<unit**>(aligned)[-1] = storage;
        return aligned;
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        auto* storage = static_cast<unit**>(pointer)[-1];
        std::allocator_traits<allocator_type>::deallocate(allocator_, storage,
                                                          units(bytes, alignment));
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

public:
    /**
     Initializes retained state and its executor or allocation resource.
     */
    explicit allocator_resource(const Allocator& allocator) : allocator_(allocator) {}
};

/**
 Keeps a resource alive until the last queued allocation is released.
 */
template <class T>
struct owning_allocator {
    /**
     Element type allocated by this allocator.
     */
    using value_type = T;
    /**
     Allocation resource kept alive while queued callbacks can access storage.
     */
    std::shared_ptr<std::pmr::memory_resource> resource;

    /**
     Initializes retained state and its executor or allocation resource.
     */
    explicit owning_allocator(std::shared_ptr<std::pmr::memory_resource> value)
        : resource(std::move(value)) {}

    /**
     Initializes retained state and its executor or allocation resource.
     */
    template <class U>
    explicit owning_allocator(const owning_allocator<U>& other) : resource(other.resource) {}

    /**
     Allocates aligned storage for the requested number of elements.
     */
    T* allocate(std::size_t count) {
        return static_cast<T*>(resource->allocate(count * sizeof(T), alignof(T)));
    }

    /**
     Returns element storage to the retained resource.
     */
    void deallocate(T* pointer, std::size_t count) {
        resource->deallocate(pointer, count * sizeof(T), alignof(T));
    }

    /**
     Reports whether two allocators share the same resource.
     */
    template <class U>
    bool operator==(const owning_allocator<U>& other) const {
        return resource == other.resource;
    }
};

/**
 Type-erased completion owned by its associated handler allocator.
 */
struct pending {
    /**
     Releases operation storage and posts exactly one completion.
     */
    virtual void complete(error_code ec = {}, std::size_t count = 0) = 0;
    /**
     Releases operation storage without invoking the handler during context destruction.
     */
    virtual void discard() = 0;
    /**
     Releases registration and retained state during destruction.
     */
    virtual ~pending() = default;
};

/**
 Owns a pending handler, its work guards, and cancellation registration.
 */
template <class Handler, bool Sized, class Action>
class operation final : public pending {
    using allocator_type = std::allocator_traits<
        boost::asio::associated_allocator_t<Handler>>::template rebind_alloc<operation>;
    Handler handler_;
    Action action_;
    pending*& slot_;
    executor_type executor_;
    boost::asio::executor_work_guard<executor_type> work_;
    boost::asio::executor_work_guard<boost::asio::associated_executor_t<Handler, executor_type>>
        completion_work_;

    void destroy() {
        allocator_type allocator(boost::asio::get_associated_allocator(handler_));
        std::allocator_traits<allocator_type>::destroy(allocator, this);
        std::allocator_traits<allocator_type>::deallocate(allocator, this, 1);
    }

public:
    /**
     Initializes retained state and its executor or allocation resource.
     */
    operation(Handler handler, Action action, pending*& slot, executor_type executor)
        : handler_(std::move(handler)),
          action_(std::move(action)),
          slot_(slot),
          executor_(std::move(executor)),
          work_(executor_),
          completion_work_(boost::asio::get_associated_executor(handler_, executor_)) {
        slot_ = this;
    }

    /**
     Installs cancellation handling for a pending operation.
     */
    void start(boost::asio::cancellation_type mask, const auto& cancel) {
        auto cancellation = boost::asio::get_associated_cancellation_slot(handler_);
        if (cancellation.is_connected()) {
            cancellation.assign([this, mask, cancel](auto kind) {
                if ((kind & mask) != boost::asio::cancellation_type::none) {
                    cancel();
                    complete(boost::asio::error::operation_aborted);
                }
            });
        }
    }

    /**
     Releases operation storage without invoking the handler during context destruction.
     */
    void discard() override {
        boost::asio::get_associated_cancellation_slot(handler_).clear();
        slot_ = nullptr;
        destroy();
    }

    /**
     Releases operation storage and posts exactly one completion.
     */
    void complete(error_code ec = {}, std::size_t count = 0) override {
        boost::asio::get_associated_cancellation_slot(handler_).clear();
        slot_ = nullptr;
        action_(ec, count);
        allocator_type allocator(boost::asio::get_associated_allocator(handler_));
        auto handler = std::move(handler_);
        const auto executor = executor_;
        // Post before releasing work; the queued completion then owns the wakeup.
        if constexpr (Sized) {
            post(executor, std::move(handler), ec, count);
        } else {
            post(executor, std::move(handler), ec);
        }
        std::allocator_traits<allocator_type>::destroy(allocator, this);
        std::allocator_traits<allocator_type>::deallocate(allocator, this, 1);
    }
};

/**
 Installs cancellation handling for a pending operation.
 */
template <bool Sized, class Handler, class Action, class Cancel>
void start(pending*& slot, const executor_type& executor, Handler&& handler, Action action,
           boost::asio::cancellation_type mask, const Cancel& cancel) {
    using type = operation<std::decay_t<Handler>, Sized, Action>;
    using allocator_type = std::allocator_traits<
        boost::asio::associated_allocator_t<std::decay_t<Handler>>>::template rebind_alloc<type>;
    allocator_type allocator(boost::asio::get_associated_allocator(handler));
    auto* object = std::allocator_traits<allocator_type>::allocate(allocator, 1);
    std::allocator_traits<allocator_type>::construct(
        allocator, object, std::forward<Handler>(handler), std::move(action), slot, executor);
    object->start(mask, std::move(cancel));
}
}  // namespace browser
}
}  // namespace webcpp::beast::detail
#endif
