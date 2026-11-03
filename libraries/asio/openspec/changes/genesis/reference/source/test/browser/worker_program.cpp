// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The worker waits in the real reactor. Timer completion crosses to the browser main
// executor, touches the DOM and posts back to wake the original worker context.
#include <webcpp/asio/browser/main_thread_context.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/test/exit_status.hpp>

#include <emscripten.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/core/lightweight_test.hpp>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <memory>

namespace {
struct allocation_counts {
    int allocated = 0;
    int live = 0;
};

template <class T>
struct tracking_allocator {
    using value_type = T;
    allocation_counts* counts;

    explicit tracking_allocator(allocation_counts& value) : counts(&value) {}

    template <class U>
    explicit tracking_allocator(const tracking_allocator<U>& other) : counts(other.counts) {}

    T* allocate(std::size_t count) {
        ++counts->allocated;
        ++counts->live;
        return std::allocator<T>{}.allocate(count);
    }

    void deallocate(T* value, std::size_t count) {
        --counts->live;
        std::allocator<T>{}.deallocate(value, count);
    }

    template <class U>
    bool operator==(const tracking_allocator<U>& other) const {
        return counts == other.counts;
    }
};

struct results {
    webcpp::asio::browser::main_thread_context main;
    std::atomic<int> done{0};
    std::atomic<int> errors{0};
    int callbacks = 0;
    int cancellations = 0;
};

void* run_worker(void* pointer) {
    auto& result = *static_cast<results*>(pointer);
    webcpp::asio::io_context io;
    auto work = boost::asio::make_work_guard(io);
    int returned = 0;
    const auto return_to_worker = [&] {
        boost::asio::post(io, [&] {
            if (emscripten_is_main_browser_thread()) {
                ++result.errors;
            }
            if (++returned == 2) {
                work.reset();
            }
        });
    };
    boost::asio::cancellation_signal signal;
    boost::asio::steady_timer cancelled(io, std::chrono::hours(1));
    cancelled.async_wait(boost::asio::bind_cancellation_slot(
        signal.slot(), boost::asio::bind_executor(result.main.get_executor(), [&](auto ec) {
            if (ec != boost::asio::error::operation_aborted ||
                !emscripten_is_main_browser_thread()) {
                ++result.errors;
            }
            ++result.cancellations;
            return_to_worker();
        })));
    signal.emit(boost::asio::cancellation_type::all);
    boost::asio::steady_timer timer(io, std::chrono::milliseconds(30));
    timer.async_wait(boost::asio::bind_executor(result.main.get_executor(), [&](auto ec) {
        if (ec || !emscripten_is_main_browser_thread()) {
            ++result.errors;
        }
        MAIN_THREAD_EM_ASM({ document.body.dataset.workerHandoff = 'complete'; });
        ++result.callbacks;
#ifdef WEBCPP_TEST_WORKER_AFFINITY
        // An invalid main-thread runner is rejected before reading the worker's JS handles.
        if (io.run() != 0) {  // lint-run: reject main-thread entry before suspension
            ++result.errors;
        }
#endif
        return_to_worker();
    }));
    if (io.run() != 4) {  // lint-run: worker-owned context, real timer and wakeup
        ++result.errors;
    }
    result.done.store(1);
    return nullptr;
}

EM_ASYNC_JS(void, wait_worker, (const void* done), {
    const deadline = performance.now() + 5000;
    while (!Atomics.load(HEAP32, done >>> 2)) {
        if (performance.now() > deadline)
            throw new Error('worker context did not finish');
        await new Promise(function(resolve) { setTimeout(resolve, 5); });
    }
});

EM_ASYNC_JS(void, settle, (),
            { await new Promise(function(resolve) { setTimeout(resolve, 30); }); });
}  // namespace

int main() {
    results result;
    pthread_t thread{};
    BOOST_TEST_EQ(pthread_create(&thread, nullptr, run_worker, &result), 0);
    wait_worker(&result.done);
    int joined = EBUSY;
    for (int attempt = 0; attempt < 100 && joined == EBUSY; ++attempt) {
        joined = pthread_tryjoin_np(thread, nullptr);
        if (joined == EBUSY) {
            settle();
        }
    }
    BOOST_TEST_EQ(joined, 0);
    BOOST_TEST_EQ(result.errors.load(), 0);
    BOOST_TEST_EQ(result.cancellations, 1);
    BOOST_TEST_EQ(result.callbacks, 1);
    BOOST_TEST_EQ(
        EM_ASM_INT({ return Object.is(document.body.dataset.workerHandoff, 'complete'); }), 1);

    allocation_counts allocations;
    int allocated_calls = 0;
    boost::asio::post(result.main.get_executor(),
                      boost::asio::bind_allocator(tracking_allocator<void>(allocations), [&] {
                          BOOST_TEST_EQ(allocations.live, 0);
                          ++allocated_calls;
                      }));
    BOOST_TEST_EQ(allocated_calls, 0);
    BOOST_TEST(allocations.allocated > 0);
    settle();
    BOOST_TEST_EQ(allocated_calls, 1);

    int discarded = 0;
    std::weak_ptr<int> lifetime;
    {
        webcpp::asio::browser::main_thread_context shutdown;
        const auto kept = std::make_shared<int>(42);
        lifetime = kept;
        boost::asio::post(shutdown.get_executor(),
                          boost::asio::bind_allocator(tracking_allocator<void>(allocations),
                                                      [kept, &discarded] { ++discarded; }));
    }
    BOOST_TEST(lifetime.expired());
    BOOST_TEST_EQ(allocations.live, 0);
    settle();
    BOOST_TEST_EQ(discarded, 0);
    allocation_counts context_allocations;
    {
        const webcpp::asio::browser::main_thread_context allocated_context(
            std::allocator_arg, tracking_allocator<void>(context_allocations));
        BOOST_TEST(context_allocations.allocated > 0);
    }
    BOOST_TEST_EQ(context_allocations.live, 0);
    allocation_counts reentrant_context_allocations;
    allocation_counts reentrant_handler_allocations;
    int reentrant_calls = 0;
    std::weak_ptr<int> discarded_lifetime;
    auto reentrant = std::make_unique<webcpp::asio::browser::main_thread_context>(
        std::allocator_arg, tracking_allocator<void>(reentrant_context_allocations));
    boost::asio::post(
        reentrant->get_executor(),
        boost::asio::bind_allocator(tracking_allocator<void>(reentrant_handler_allocations), [&] {
            ++reentrant_calls;
            reentrant.reset();
            BOOST_TEST(discarded_lifetime.expired());
            BOOST_TEST_EQ(reentrant_handler_allocations.live, 0);
        }));
    {
        const auto held = std::make_shared<int>(42);
        discarded_lifetime = held;
        boost::asio::post(
            reentrant->get_executor(),
            boost::asio::bind_allocator(tracking_allocator<void>(reentrant_handler_allocations),
                                        [held, &reentrant_calls] { ++reentrant_calls; }));
    }
    settle();
    BOOST_TEST_EQ(reentrant_calls, 1);
    BOOST_TEST_EQ(reentrant_context_allocations.live, 0);
    BOOST_TEST_EQ(reentrant_handler_allocations.live, 0);
    std::printf("worker handoff and shutdown complete\n");
    return webcpp::test::exit_status(boost::report_errors());
}
