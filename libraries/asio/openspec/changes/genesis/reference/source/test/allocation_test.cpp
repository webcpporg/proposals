// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Allocation attribution (doc: #asio-allocation): context services and bound operations use
// the supplied allocator. The unbound-handler control catches C-library allocation bypasses.
// C allocation hooks and the stack-backed select are adapted from trystero's allocation test.

#include <webcpp/asio/io_context.hpp>
#include <webcpp/asio/steady_timer.hpp>

#include <webcpp/test/exit_status.hpp>

#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/throw_exception.hpp>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#ifdef _MSC_VER
#include <malloc.h>
#else
#include <pthread.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#include <emscripten/emscripten.h>
#include <emscripten/heap.h>
#include <poll.h>
#include <sys/select.h>
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/asio/browser/async_wait_event.hpp>
#include <webcpp/asio/browser/bridge.hpp>
#include <webcpp/asio/ip/tcp.hpp>
#include <webcpp/asio/ssl.hpp>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#endif

namespace {

// What the replaced operator new and the C library's allocation functions count
// while counting is set: global and mutable, since an operator new can reach
// nothing else.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t operator_new_calls = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t c_allocations = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool counting = false;
// Set while operator new takes its memory from the C library, which then
// counts nothing: that allocation is operator new's, counted once.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool inside_operator_new = false;
// Runtime allocation attribution, set only by the Asyncify JavaScript wrapper.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::uint8_t inside_runtime = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t runtime_allocations = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t aligned_allocations = 0;

#ifdef _MSC_VER
/** Whether the calling thread is the one the test counts on: MSVC counts every thread. */
bool on_counted_thread() noexcept {
    return true;
}

void count_this_thread() noexcept {}
#else
// The thread main runs on: the C library's allocations are reported from every thread
// (malloc_logger is process-wide, the replaced functions serve every thread), and a
// system thread allocating during a window is not the application's, nor may it touch the
// counts. Atomic, since every thread reads it.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<pthread_t> counted_thread{};

/** Whether the calling thread is the one the test counts on. */
bool on_counted_thread() noexcept {
    return pthread_equal(pthread_self(), counted_thread.load(std::memory_order_relaxed)) != 0;
}

void count_this_thread() noexcept {
    counted_thread.store(pthread_self(), std::memory_order_relaxed);
}
#endif

}  // namespace

// AddressSanitizer replaces the C library's allocation functions with its own
// allocator, which none of the hooks below sees, so a build with it counts none.
#ifdef __SANITIZE_ADDRESS__
#define WEBCPP_TEST_ASIO_ADDRESS_SANITIZER 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define WEBCPP_TEST_ASIO_ADDRESS_SANITIZER 1
#endif
#endif

// The C library's allocations, seen where a program can see them: glibc's
// functions are replaced and forward to glibc's own (__libc_*), Emscripten's
// dlmalloc defines its functions weak and keeps emscripten_builtin_*, and
// macOS reports every allocation to malloc_logger. MSVC's are not counted, nor
// any under AddressSanitizer.
#ifdef WEBCPP_TEST_ASIO_ADDRESS_SANITIZER
// Nothing of the C library is counted.
#elif defined(__GLIBC__) || defined(__EMSCRIPTEN__)
#define WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS 1
#ifdef __GLIBC__
extern "C" {
void* __libc_malloc(std::size_t size);
void* __libc_calloc(std::size_t count, std::size_t size);
void* __libc_realloc(void* pointer, std::size_t size);
void* __libc_memalign(std::size_t alignment, std::size_t size);
void __libc_free(void* pointer);
}
#define WEBCPP_TEST_C_MALLOC __libc_malloc
#define WEBCPP_TEST_C_CALLOC __libc_calloc
#define WEBCPP_TEST_C_REALLOC __libc_realloc
#define WEBCPP_TEST_C_MEMALIGN __libc_memalign
#define WEBCPP_TEST_C_FREE __libc_free
#else
#define WEBCPP_TEST_C_MALLOC emscripten_builtin_malloc
#define WEBCPP_TEST_C_CALLOC emscripten_builtin_calloc
#define WEBCPP_TEST_C_REALLOC emscripten_builtin_realloc
#define WEBCPP_TEST_C_MEMALIGN emscripten_builtin_memalign
#define WEBCPP_TEST_C_FREE emscripten_builtin_free
#endif

namespace {

void* observe_allocation(void* pointer, bool aligned = false) {
    if (on_counted_thread() && counting && !inside_operator_new) {
        if (inside_runtime != 0) {
            ++runtime_allocations;
        } else if (aligned) {
            ++aligned_allocations;
        } else {
            ++c_allocations;
        }
    }
    return pointer;
}

}  // namespace

// The C library's allocation functions, replaced for this program alone: the C library declares
// them with parameter names of its own, and cert-dcl58-cpp takes a replacement of what <cstdlib>
// declares for a change to namespace std.
// NOLINTBEGIN(readability-inconsistent-declaration-parameter-name,cert-dcl58-cpp)
extern "C" {
void* malloc(std::size_t size) {
    return observe_allocation(WEBCPP_TEST_C_MALLOC(size));
}

void* calloc(std::size_t count, std::size_t size) {
    return observe_allocation(WEBCPP_TEST_C_CALLOC(count, size));
}

void* realloc(void* pointer, std::size_t size) {
    return observe_allocation(WEBCPP_TEST_C_REALLOC(pointer, size));
}

void free(void* pointer) {
    WEBCPP_TEST_C_FREE(pointer);
}

void* memalign(std::size_t alignment, std::size_t size) {
    return observe_allocation(WEBCPP_TEST_C_MEMALIGN(alignment, size), true);
}

void* aligned_alloc(std::size_t alignment, std::size_t size) {
    return observe_allocation(WEBCPP_TEST_C_MEMALIGN(alignment, size), true);
}

int posix_memalign(void** out, std::size_t alignment, std::size_t size) {
    if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0) {
        return EINVAL;
    }
    void* pointer = observe_allocation(WEBCPP_TEST_C_MEMALIGN(alignment, size), true);
    if (pointer == nullptr) {
        return ENOMEM;
    }
    *out = pointer;
    return 0;
}
}
// NOLINTEND(readability-inconsistent-declaration-parameter-name,cert-dcl58-cpp)

#ifdef __EMSCRIPTEN__
// Emscripten's select() is a poll() over an array of pollfd it callocs on every
// call (system/lib/libc/musl/src/select/select.c), and Asio's reactor calls it on
// every wait of the io_context the test drives. This one keeps the array on the stack, so
// that the count sees the application's allocations rather than the C library's wait.
namespace {

unsigned requested_event(int fd, const fd_set* set, unsigned event) {
    return set != nullptr && FD_ISSET(fd, set) ? event : 0U;
}

void clear_set(fd_set* set) {
    if (set != nullptr) {
        FD_ZERO(set);
    }
}

int ready_event(const pollfd& descriptor, fd_set* set, unsigned events) {
    if (set != nullptr && (static_cast<unsigned>(descriptor.revents) & events) != 0) {
        FD_SET(descriptor.fd, set);
        return 1;
    }
    return 0;
}

}  // namespace

// select's C declaration requires a mutable timeval pointer, although this implementation
// does not change it.
extern "C" int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
                      timeval* timeout) {  // NOLINT(misc-const-correctness)
    if (nfds < 0 || nfds > FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }
    std::array<pollfd, FD_SETSIZE> fds{};
    int n = 0;
    for (int fd = 0; fd < nfds; ++fd) {
        const unsigned events = requested_event(fd, readfds, POLLIN) |
                                requested_event(fd, writefds, POLLOUT) |
                                requested_event(fd, exceptfds, POLLPRI);
        if (events != 0) {
            fds[n++] = pollfd{.fd = fd, .events = static_cast<short>(events), .revents = 0};
        }
    }
    const int milliseconds =
        timeout == nullptr ? -1
                           : static_cast<int>((timeout->tv_sec * 1000) + (timeout->tv_usec / 1000));
    const int ready = poll(fds.data(), static_cast<nfds_t>(n), milliseconds);
    if (ready < 0) {
        return ready;
    }
    clear_set(readfds);
    clear_set(writefds);
    clear_set(exceptfds);
    int count = 0;
    for (int i = 0; i < n && ready > 0; ++i) {
        count += ready_event(fds[i], readfds, POLLIN | POLLHUP | POLLERR);
        count += ready_event(fds[i], writefds, POLLOUT | POLLERR);
        count += ready_event(fds[i], exceptfds, POLLPRI);
    }
    return count;
}
#endif

#elif defined(__APPLE__)
#define WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS 1

// libsystem_malloc's hook, which it calls on every allocation and release: a global of its own,
// which the test sets.
extern "C" {
using malloc_logger_type = void(std::uint32_t type, std::uintptr_t, std::uintptr_t, std::uintptr_t,
                                std::uintptr_t, std::uint32_t);
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern malloc_logger_type* malloc_logger;
}

namespace {

constexpr std::uint32_t malloc_log_allocate = 2;

void count_c_allocation(std::uint32_t type, std::uintptr_t /*zone*/, std::uintptr_t /*size*/,
                        std::uintptr_t /*other*/, std::uintptr_t /*result*/,
                        std::uint32_t /*frames*/) {
    if (on_counted_thread() && counting && !inside_operator_new &&
        (type & malloc_log_allocate) != 0) {
        if (inside_runtime != 0) {
            ++runtime_allocations;
        } else {
            ++c_allocations;
        }
    }
}

}  // namespace
#endif

namespace {

/** Memory for operator new, from the C library, uncounted there. */
void* take(std::size_t size, std::size_t alignment) {
#ifdef _MSC_VER
    return _aligned_malloc(size, alignment);
#elif defined(WEBCPP_TEST_C_MEMALIGN)
    return WEBCPP_TEST_C_MEMALIGN(alignment, size);
#else
    // Only the measured thread touches this flag; accessing thread-local storage here can
    // allocate on macOS and recursively enter malloc_logger before the flag exists.
    const bool measured = on_counted_thread();
    const bool previous = measured ? std::exchange(inside_operator_new, true) : false;
    void* pointer = std::aligned_alloc(alignment, size);
    if (measured) {
        inside_operator_new = previous;
    }
    return pointer;
#endif
}

void release(void* pointer) noexcept {
#ifdef _MSC_VER
    _aligned_free(pointer);
#elif defined(WEBCPP_TEST_C_FREE)
    WEBCPP_TEST_C_FREE(pointer);
#else
    // Only the measured thread touches this flag; accessing thread-local storage here can
    // allocate on macOS and recursively enter malloc_logger before the flag exists.
    const bool measured = on_counted_thread();
    const bool previous = measured ? std::exchange(inside_operator_new, true) : false;
    std::free(pointer);
    if (measured) {
        inside_operator_new = previous;
    }
#endif
}

/** Uncounted, aligned storage; a zero-size request still owns a non-null block. */
void* allocate_storage(std::size_t size, std::size_t alignment) noexcept {
    alignment = alignment < sizeof(void*) ? sizeof(void*) : alignment;
    if (size > std::numeric_limits<std::size_t>::max() - (alignment - 1)) {
        return nullptr;
    }
    const std::size_t rounded =
        size == 0 ? alignment : (size + alignment - 1) / alignment * alignment;
    return take(rounded, alignment);
}

/** Counts a global allocation once, even when its storage comes from the C library. */
void* allocate(std::size_t size, std::size_t alignment) noexcept {
    if (on_counted_thread() && counting) {
        ++operator_new_calls;
    }
    return allocate_storage(size, alignment);
}

void* allocate_or_throw(std::size_t size, std::size_t alignment) {
    void* pointer = allocate(size, alignment);
    if (pointer == nullptr) {
        boost::throw_exception(std::bad_alloc());
    }
    return pointer;
}

/** A standard allocator whose rebinds share a counter, bypassing the global hooks. */
template <class T>
struct counted {
    using value_type = T;
    std::size_t* given;

    explicit counted(std::size_t* counter) : given(counter) {}

    // Allocator rebind conversions must be implicit (Allocator requirements).
    template <class U>
    // NOLINTNEXTLINE(misc-explicit-constructor)
    counted(const counted<U>& other) : given(other.given) {}

    T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            boost::throw_exception(std::bad_alloc());
        }
        auto* pointer = static_cast<T*>(allocate_storage(n * sizeof(T), alignof(T)));
        if (pointer == nullptr) {
            boost::throw_exception(std::bad_alloc());
        }
        ++*given;
        return pointer;
    }

    void deallocate(T* pointer, std::size_t /*n*/) noexcept { release(pointer); }

    template <class U>
    bool operator==(const counted<U>& other) const noexcept {
        return given == other.given;
    }
};

/** Counts only the tested scope, including object destruction; reports are outside it. */
class allocation_window {
public:
    allocation_window() {
        operator_new_calls = 0;
        c_allocations = 0;
        aligned_allocations = 0;
        runtime_allocations = 0;
        counting = true;
    }

    ~allocation_window() { counting = false; }

    allocation_window(const allocation_window&) = delete;
    allocation_window& operator=(const allocation_window&) = delete;
};

/** Marks only Asyncify's allocation of unwind data; returns whether the variant uses it. */
bool mark_the_runtime_s_allocations() {
#ifdef __EMSCRIPTEN__
    // clang-format off
    const int variant = EM_ASM_INT({
        if (typeof Asyncify !== 'object') {
            return -1;
        }
        if (typeof Asyncify.allocateData !== 'function') {
            return typeof Asyncify.isAsyncExport === 'function' ? 0 : -1;
        }
        const flag = $0;
        const allocate = Asyncify.allocateData;
        Asyncify.allocateData = function(...values) {
            const previous = HEAPU8[flag];
            HEAPU8[flag] = 1;
            try {
                return allocate.apply(this, values);
            } finally {
                HEAPU8[flag] = previous;
            }
        };
        return 1;
    }, &inside_runtime);
    // clang-format on
    BOOST_TEST_GE(variant, 0);
    return variant == 1;
#else
    return false;
#endif
}

void report(const char* name, std::size_t given = 0) {
    std::printf("%s: given=%zu new=%zu c=%zu aligned=%zu runtime=%zu\n", name, given,
                operator_new_calls, c_allocations, aligned_allocations, runtime_allocations);
}

#ifdef __EMSCRIPTEN__
/** A host-storage resource using the same uncounted backing as counted<T>. */
class counted_resource final : public std::pmr::memory_resource {
public:
    explicit counted_resource(std::size_t& allocations) : allocations_(allocations) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* result = allocate_storage(bytes, alignment);
        if (result == nullptr) {
            boost::throw_exception(std::bad_alloc());
        }
        ++allocations_;
        return result;
    }

    void do_deallocate(void* pointer, std::size_t /*bytes*/, std::size_t /*alignment*/) override {
        release(pointer);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::size_t& allocations_;
};

void tls_host_storage_uses_the_resource() {
    namespace ssl = webcpp::asio::ssl;
    // The request object owns its input independently of the context's recorded copy.
    // Construct it before measuring the context, then transfer it without another copy.
    ssl::host_name_verification verification(
        "a-long-host-name-that-exceeds-small-string-storage.example.com");
    std::size_t given = 0;
    counted_resource resource(given);
    bool recorded = false;
    {
        const allocation_window window;
        ssl::context context(ssl::context::tlsv12_client, &resource);
        boost::system::error_code ec;
        context.set_verify_callback(std::move(verification), ec);
        recorded = !ec && context.verification_host() ==
                              "a-long-host-name-that-exceeds-small-string-storage.example.com";
    }
    report("TLS host allocator", given);
    BOOST_TEST(recorded);
    BOOST_TEST_EQ(given, 1U);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_EQ(c_allocations, 0U);
    BOOST_TEST_EQ(aligned_allocations, 0U);
}

/** Each delivered event posts through its associated allocator, never runs inline. */
class counted_registration final : public webcpp::asio::browser::registration {
public:
    counted_registration(webcpp::asio::io_context& io, std::size_t* given, std::size_t* received)
        : io_(io), given_(given), received_(received) {}

    bool deliver(const emscripten::val& /*event*/) override {
        boost::asio::post(
            io_, boost::asio::bind_allocator(counted<void>(given_), [this] { ++*received_; }));
        return false;
    }

    void shutdown() noexcept override {}

private:
    webcpp::asio::io_context& io_;
    std::size_t* given_;
    std::size_t* received_;
};
#endif

// tag::supplied-allocator[]
void the_user_s_allocator_takes_everything() {
    std::size_t given = 0;
    std::size_t posted = 0;
    std::size_t timed = 0;
    bool timer_succeeded = false;
#ifdef __EMSCRIPTEN__
    std::size_t received = 0;
    std::size_t promised = 0;
    std::size_t events = 0;
    std::size_t connected = 0;
    bool values_correct = true;
#endif
    {
        const allocation_window window;
        webcpp::asio::io_context io(std::allocator_arg, counted<void>(&given));
        webcpp::asio::steady_timer timer(io, std::chrono::milliseconds(5));
        timer.async_wait(
            boost::asio::bind_allocator(counted<void>(&given), [&](boost::system::error_code ec) {
                ++timed;
                timer_succeeded = !ec;
            }));
        for (std::size_t i = 0; i < 10; ++i) {
            boost::asio::post(
                io, boost::asio::bind_allocator(counted<void>(&given), [&posted] { ++posted; }));
        }
#ifdef __EMSCRIPTEN__
        webcpp::asio::ip::tcp::socket socket(io);
        socket.async_connect(
            {boost::asio::ip::address_v4::loopback(), 80},
            boost::asio::bind_allocator(counted<void>(&given), [&](boost::system::error_code ec) {
                ++connected;
                values_correct =
                    values_correct && ec == boost::asio::error::operation_not_supported;
            }));
        counted_registration listener(io, &given, &received);
        auto& bridge = boost::asio::use_service<webcpp::asio::browser::bridge>(io);
        const std::uint64_t key = bridge.add(listener);
        const emscripten::val deliver = bridge.function(key);
        deliver(1);
        deliver(2);
        const auto target = emscripten::val::global("EventTarget").new_();
        webcpp::asio::browser::async_wait_event(
            io.get_executor(), target, "message",
            boost::asio::bind_allocator(counted<void>(&given),
                                        [&](boost::system::error_code ec, const emscripten::val&) {
                                            values_correct = values_correct && !ec;
                                            ++events;
                                        }));
        target.call<bool>("dispatchEvent",
                          emscripten::val::global("Event").new_(emscripten::val("message")));
        for (bool rejected : {false, true}) {
            auto promise = emscripten::val::global("Promise").call<emscripten::val>(
                rejected ? "reject" : "resolve", 42);
            webcpp::asio::browser::async_await(
                io.get_executor(), promise,
                boost::asio::bind_allocator(
                    counted<void>(&given),
                    [&, rejected](boost::system::error_code ec, emscripten::val value) {
                        ++promised;
                        values_correct =
                            values_correct && value.as<int>() == 42 &&
                            (rejected ? ec == webcpp::asio::browser::error::promise_rejected : !ec);
                    }));
        }
#endif
        io.run();  // lint-run: the test's event loop
#ifdef __EMSCRIPTEN__
        bridge.remove(key);
#endif
    }
    report("supplied allocator", given);
    BOOST_TEST_EQ(posted, 10U);
    BOOST_TEST_EQ(timed, 1U);
    BOOST_TEST(timer_succeeded);
#ifdef __EMSCRIPTEN__
    BOOST_TEST_EQ(received, 2U);
    BOOST_TEST_EQ(promised, 2U);
    BOOST_TEST_EQ(events, 1U);
    BOOST_TEST_EQ(connected, 1U);
    BOOST_TEST(values_correct);
#endif
    BOOST_TEST_GT(given, 0U);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_EQ(c_allocations, 0U);
    BOOST_TEST_EQ(aligned_allocations, 0U);
}

// end::supplied-allocator[]

/** A real suspension proves runtime attribution without depending on a timer's deadline. */
void the_counters_separate_runtime_allocations(bool asyncify) {
#if defined(__EMSCRIPTEN__) && defined(WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS)
    {
        const allocation_window window;
        emscripten_sleep(0);
    }
    report("explicit suspension");
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_EQ(c_allocations + aligned_allocations, 0U);
    BOOST_TEST_EQ(inside_runtime, 0U);
    if (asyncify) {
        BOOST_TEST_GT(runtime_allocations, 0U);
    } else {
        BOOST_TEST_EQ(runtime_allocations, 0U);
    }
#else
    (void)asyncify;
#endif
}

void the_counters_see_an_unbound_handler() {
#ifdef WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS
    std::size_t given = 0;
    std::size_t ran = 0;
    webcpp::asio::io_context io(std::allocator_arg, counted<void>(&given));
    {
        const allocation_window window;
        boost::asio::post(io, [&ran] { ++ran; });
    }
    report("unbound handler");
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_GT(c_allocations + aligned_allocations, 0U);
    BOOST_TEST_EQ(runtime_allocations, 0U);
    io.run();  // lint-run: the control's event loop, outside its allocation window
    BOOST_TEST_EQ(ran, 1U);
#else
    std::puts("unbound handler: C allocation counting unavailable on this platform/build");
#endif
}

/** Tests one direct global allocation call, which cannot be elided like a new-expression. */
void check_new_form(auto make, bool array, bool aligned) {
    void* pointer = nullptr;
    {
        const allocation_window window;
        pointer = make();
    }
    BOOST_TEST(pointer != nullptr);
    BOOST_TEST_EQ(operator_new_calls, 1U);
    BOOST_TEST_EQ(c_allocations + aligned_allocations, 0U);
    if (aligned) {
        BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(pointer) % 64, 0U);
        if (array) {
            ::operator delete[](pointer, std::align_val_t{64});
        } else {
            ::operator delete(pointer, std::align_val_t{64});
        }
    } else if (array) {
        ::operator delete[](pointer);
    } else {
        ::operator delete(pointer);
    }
}

void the_counters_see_every_new_form() {
    check_new_form([] { return ::operator new(37); }, false, false);
    check_new_form([] { return ::operator new[](37); }, true, false);
    check_new_form([] { return ::operator new(37, std::nothrow); }, false, false);
    check_new_form([] { return ::operator new[](37, std::nothrow); }, true, false);
    check_new_form([] { return ::operator new(37, std::align_val_t{64}); }, false, true);
    check_new_form([] { return ::operator new[](37, std::align_val_t{64}); }, true, true);
    check_new_form([] { return ::operator new(37, std::align_val_t{64}, std::nothrow); }, false,
                   true);
    check_new_form([] { return ::operator new[](37, std::align_val_t{64}, std::nothrow); }, true,
                   true);
    std::puts("global new controls: all eight forms counted once");
}

void the_counters_see_c_and_aligned_allocations() {
#ifdef WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS
    void* volatile pointer = nullptr;
    void* volatile resized = nullptr;
    {
        const allocation_window window;
        pointer = std::malloc(48);
        resized = std::realloc(pointer, 96);
    }
    BOOST_TEST(resized != nullptr);
    std::free(resized != nullptr ? resized : pointer);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_GE(c_allocations, 2U);
    {
        const allocation_window window;
        pointer = std::calloc(2, 48);
    }
    BOOST_TEST(pointer != nullptr);
    std::free(pointer);
    BOOST_TEST_GT(c_allocations, 0U);
    {
        const allocation_window window;
        pointer = std::aligned_alloc(64, 128);
    }
    BOOST_TEST(pointer != nullptr);
    std::free(pointer);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_GT(c_allocations + aligned_allocations, 0U);
    void* aligned_pointer = nullptr;
    int status = 0;
    {
        const allocation_window window;
        status = posix_memalign(&aligned_pointer, 64, 128);
    }
    BOOST_TEST_EQ(status, 0);
    BOOST_TEST(aligned_pointer != nullptr);
    BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(aligned_pointer) % 64, 0U);
    std::free(aligned_pointer);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_GT(c_allocations + aligned_allocations, 0U);
#if defined(__GLIBC__) || defined(__EMSCRIPTEN__)
    {
        const allocation_window window;
        pointer = memalign(64, 128);
    }
    BOOST_TEST(pointer != nullptr);
    BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(pointer) % 64, 0U);
    std::free(pointer);
    BOOST_TEST_EQ(operator_new_calls, 0U);
    BOOST_TEST_GT(aligned_allocations, 0U);
#endif
    std::puts("C allocation controls: ordinary and platform aligned entry points observed");
#else
    std::puts("C allocation controls: unavailable on this platform/build");
#endif
}

}  // namespace

// The standard declares implementation-reserved parameter names; these are program replacements.
// NOLINTBEGIN(readability-inconsistent-declaration-parameter-name)
void* operator new(std::size_t size) {
    return allocate_or_throw(size, alignof(std::max_align_t));
}

void* operator new(std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size, alignof(std::max_align_t));
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_or_throw(size, static_cast<std::size_t>(alignment));
}

void* operator new(std::size_t size, std::align_val_t alignment,
                   const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer) noexcept {
    release(pointer);
}

void operator delete(void* pointer, std::size_t /*size*/) noexcept {
    release(pointer);
}

void operator delete(void* pointer, std::align_val_t /*alignment*/) noexcept {
    release(pointer);
}

void operator delete(void* pointer, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    release(pointer);
}

void operator delete(void* pointer, const std::nothrow_t& /*tag*/) noexcept {
    release(pointer);
}

void operator delete(void* pointer, std::align_val_t /*alignment*/,
                     const std::nothrow_t& /*tag*/) noexcept {
    release(pointer);
}

void* operator new[](std::size_t size) {
    return allocate_or_throw(size, alignof(std::max_align_t));
}

void* operator new[](std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size, alignof(std::max_align_t));
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_or_throw(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment,
                     const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete[](void* pointer) noexcept {
    release(pointer);
}

void operator delete[](void* pointer, std::size_t /*size*/) noexcept {
    release(pointer);
}

void operator delete[](void* pointer, std::align_val_t /*alignment*/) noexcept {
    release(pointer);
}

void operator delete[](void* pointer, std::size_t /*size*/,
                       std::align_val_t /*alignment*/) noexcept {
    release(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t& /*tag*/) noexcept {
    release(pointer);
}

void operator delete[](void* pointer, std::align_val_t /*alignment*/,
                       const std::nothrow_t& /*tag*/) noexcept {
    release(pointer);
}

// NOLINTEND(readability-inconsistent-declaration-parameter-name)

int main() {
    count_this_thread();
#if defined(WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS) && defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    auto* previous_logger = malloc_logger;
    malloc_logger = &count_c_allocation;
#endif
#ifndef __EMSCRIPTEN__
    // Darwin's first access to Asio's thread-call-stack TLS asks dyld for storage. Initialize
    // that thread runtime explicitly; the measured context and every operation are still new.
    {
        boost::asio::io_context startup;
        BOOST_TEST(!startup.get_executor().running_in_this_thread());
    }
#endif
    const bool asyncify = mark_the_runtime_s_allocations();
    the_counters_see_every_new_form();
    the_counters_see_c_and_aligned_allocations();
    the_counters_separate_runtime_allocations(asyncify);
    the_user_s_allocator_takes_everything();
#ifdef __EMSCRIPTEN__
    tls_host_storage_uses_the_resource();
#endif
    the_counters_see_an_unbound_handler();
#if defined(WEBCPP_TEST_ASIO_COUNTS_C_ALLOCATIONS) && defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    malloc_logger = previous_logger;
#endif
    return webcpp::test::exit_status(boost::report_errors());
}
