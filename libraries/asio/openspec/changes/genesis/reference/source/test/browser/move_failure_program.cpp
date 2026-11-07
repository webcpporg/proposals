// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Catch the deliberately thrown handler move at the caller's SDK boundary, so
// the real browser queue can finish normally and allocation release is observable.
#include <webcpp/asio/browser/main_thread_context.hpp>
#include <webcpp/test/exit_status.hpp>

#include <emscripten.h>
#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdio>
#include <exception>
#include <memory>
#include <memory_resource>

namespace {
struct move_failure : std::exception {
    bool* caught;

    explicit move_failure(bool* observed) : caught(observed) {}

    [[nodiscard]] const char* what() const noexcept override { return "deliberate handler move"; }
};

struct forwarded_call {
    void (*function)(void*);
    void* argument;
};

void catch_move_failure(void* argument) {
    const std::unique_ptr<forwarded_call> pending(static_cast<forwarded_call*>(argument));
    try {
        pending->function(pending->argument);
    } catch (const move_failure& failure) {
        *failure.caught = true;
    }
}

class counting_resource final : public std::pmr::memory_resource {
public:
    int live = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++live;
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct throwing_move {
    bool* armed;
    bool* caught;
    int* calls;

    throwing_move(bool& enabled, bool& observed, int& invoked)
        : armed(&enabled), caught(&observed), calls(&invoked) {}

    // Deliberately exercise the executor's exceptional move cleanup.
    // NOLINTNEXTLINE(bugprone-exception-escape,performance-noexcept-move-constructor)
    throwing_move(throwing_move&& other)
        : armed(other.armed), caught(other.caught), calls(other.calls) {
        if (*armed) {
            throw move_failure{caught};
        }
    }

    throwing_move(const throwing_move&) = delete;
    throwing_move& operator=(const throwing_move&) = delete;
    throwing_move& operator=(throwing_move&&) = delete;
    ~throwing_move() = default;

    void operator()() const { ++*calls; }
};

EM_ASYNC_JS(void, settle_move_failure, (),
            { await new Promise(function(resolve) { setTimeout(resolve, 30); }); });
}  // namespace

// These are the linker's required ABI spellings, used only by this fixture.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" bool __real_emscripten_proxy_async(em_proxying_queue* queue, pthread_t thread,
                                              void (*function)(void*), void* argument);

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" bool __wrap_emscripten_proxy_async(em_proxying_queue* queue, pthread_t thread,
                                              void (*function)(void*), void* argument) {
    auto forwarded = std::make_unique<forwarded_call>(function, argument);
    if (!__real_emscripten_proxy_async(queue, thread, catch_move_failure, forwarded.get())) {
        return false;
    }
    // The queued SDK callback now owns this trampoline.
    (void)forwarded.release();  // NOLINT(bugprone-unused-return-value)
    return true;
}

int main() {
    counting_resource allocations;
    bool armed = false;
    bool caught = false;
    int calls = 0;
    {
        webcpp::asio::browser::main_thread_context context;
        boost::asio::post(
            context.get_executor(),
            boost::asio::bind_allocator(std::pmr::polymorphic_allocator<void>(&allocations),
                                        throwing_move(armed, caught, calls)));
        boost::asio::post(
            context.get_executor(),
            boost::asio::bind_allocator(std::pmr::polymorphic_allocator<void>(&allocations),
                                        [&] { ++calls; }));
        armed = true;
        settle_move_failure();
        BOOST_TEST(caught);
        BOOST_TEST_EQ(calls, 0);
        BOOST_TEST_EQ(allocations.live, 0);
    }
    BOOST_TEST_EQ(allocations.live, 0);
    std::puts("worker throwing move cleanup complete");
    return webcpp::test::exit_status(boost::report_errors());
}
