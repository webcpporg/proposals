// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_WASI_STREAM_HPP
#define WEBCPP_BEAST_WASI_STREAM_HPP

#include <webcpp/beast/config.hpp>
#include <webcpp/beast/http.hpp>

#include <boost/asio/buffer.hpp>               // lint-world: memory buffer sequences only
#include <boost/beast/core/buffers_range.hpp>  // lint-world: memory buffer iteration only
#include <boost/system/error_code.hpp>

#ifndef _WIN32
#include <fcntl.h>   // lint-world: explicit borrowed descriptor stream
#include <poll.h>    // lint-world: readiness for supplied descriptors, never socket creation
#include <unistd.h>  // lint-world: stdin/stdout and explicit descriptor I/O
#ifndef __wasi__
#include <pthread.h>  // lint-world: calling-thread signal mask, no worker thread
#include <signal.h>   // lint-world: scoped SIGPIPE handling for borrowed output
#endif
#endif
#include <algorithm>
#include <cerrno>
#include <chrono>  // lint-world: explicit stream deadlines
#include <cstddef>
#include <span>
#include <type_traits>

#ifdef __wasi__
#include <webcpp/wasi/stdio.hpp>
#endif

namespace webcpp::beast::wasi {

/** Cancellation and a deadline for a supplied channel; no background worker is created. */
struct stream_control {
    /** Optional cancellation predicate. */
    bool (*cancelled)(void*) = nullptr;
    /** Opaque cancellation predicate state. */
    void* cancellation_context = nullptr;
    /** Absolute deadline, or the maximum time point for none. */
    std::chrono::steady_clock::time_point deadline =   // lint-world: explicit channel deadline
        std::chrono::steady_clock::time_point::max();  // lint-world: explicit deadline

    /** Reports cancellation or expiry.
    @return An empty error value on success, otherwise the failure. */
    [[nodiscard]] boost::system::error_code check() const {
        if ((cancelled != nullptr) && cancelled(cancellation_context)) {
            return boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
        }
        if (std::chrono::steady_clock::now() >= deadline) {  // lint-world: explicit deadline
            return boost::system::errc::make_error_code(boost::system::errc::timed_out);
        }
        return {};
    }
};

/** A borrowed pair of nonblocking host channels.

    A zero-byte successful read means EOF. A write of zero bytes means would-block.
    Providers report would_block when unready and wait handles readiness, deadline and
    cancellation. The adapter never buffers more than the caller supplies, never closes
    a channel, and promises no path addressing, socket options or peer credentials. */
class supplied_stream {
public:
    /** Partial nonblocking read callback.
    @return The number of bytes transferred before completion or failure. */
    using read_function = std::size_t (*)(void*, std::span<char>, boost::system::error_code&);
    /** Partial nonblocking write callback.
    @return The number of bytes transferred before completion or failure. */
    using write_function = std::size_t (*)(void*, std::span<const char>,
                                           boost::system::error_code&);
    /** Readiness callback, with writing true for output.
    @return The corresponding Boost.System error value. */
    using wait_function = void (*)(void*, bool, const stream_control&, boost::system::error_code&);

private:
    void* context_;
    read_function read_{};
    write_function write_{};
    wait_function wait_;
    stream_control control_;

public:
    /** Borrows a caller-supplied channel pair.
        @param context State shared by channel callbacks.
        @param reader Nonblocking partial read callback.
        @param writer Nonblocking partial write callback.
        @param wait Readiness and cancellation callback.
        @param control Initial cancellation and deadline. */
    supplied_stream(void* context, read_function reader, write_function writer, wait_function wait,
                    stream_control control = {})
        : context_(context), read_(reader), write_(writer), wait_(wait), control_(control) {}

    /** Sets transfer cancellation and deadline.
        @param value Settings for subsequent operations. */
    void control(stream_control value) { control_ = value; }

    /** Reads a partial mutable buffer sequence.
        @tparam MutableBuffers Asio mutable buffer sequence.
        @param buffers Destination buffers.
        @param error Receives transfer failure or end_of_stream.
    @return The number of bytes transferred before completion or failure. */
    template <class MutableBuffers>
    std::size_t read_some(const MutableBuffers& buffers, boost::system::error_code& error) {
        for (auto const buffer : boost::beast::buffers_range_ref(buffers)) {
            if (buffer.size() == 0) {
                continue;
            }
            for (;;) {
                error = control_.check();
                if (error) {
                    return 0;
                }
                auto const count =
                    read_(context_, {static_cast<char*>(buffer.data()), buffer.size()}, error);
                if (error == boost::system::errc::make_error_code(
                                 boost::system::errc::operation_would_block) ||
                    error == boost::system::errc::make_error_code(
                                 boost::system::errc::resource_unavailable_try_again)) {
                    wait_(context_, false, control_, error);
                    if (error) {
                        return 0;
                    }
                    continue;
                }
                if (!error && count == 0) {
                    error = http::error::end_of_stream;
                }
                return count;
            }
        }
        error.clear();
        return 0;
    }

    /** Writes a partial constant buffer sequence.
        @tparam ConstBuffers Asio constant buffer sequence.
        @param buffers Source buffers.
        @param error Receives any transfer failure.
    @return The number of bytes transferred before completion or failure. */
    template <class ConstBuffers>
    std::size_t write_some(const ConstBuffers& buffers, boost::system::error_code& error) {
        for (auto const buffer : boost::beast::buffers_range_ref(buffers)) {
            if (buffer.size() == 0) {
                continue;
            }
            for (;;) {
                error = control_.check();
                if (error) {
                    return 0;
                }
                auto const count = write_(
                    context_, {static_cast<const char*>(buffer.data()), buffer.size()}, error);
                if ((!error && count == 0) ||
                    error == boost::system::errc::make_error_code(
                                 boost::system::errc::operation_would_block) ||
                    error == boost::system::errc::make_error_code(
                                 boost::system::errc::resource_unavailable_try_again)) {
                    wait_(context_, true, control_, error);
                    if (error) {
                        return 0;
                    }
                    continue;
                }
                return count;
            }
        }
        error.clear();
        return 0;
    }
};

#ifndef _WIN32
/** stdin/stdout or a supplied descriptor pair, borrowed exclusively for this object's lifetime.

    Descriptors are set nonblocking and restored on destruction, never closed. stdout is
    protocol-only; diagnostics belong on stderr. Readiness and cancellation are checked every
    ten milliseconds while blocked. This does not provide Unix-domain socket semantics. */
class stdio_stream {
    int input_;
    int output_;
    int input_flags_{};
    int output_flags_{};
    boost::system::error_code setup_error_;
    supplied_stream stream_;
#ifdef __wasi__
    webcpp::wasi::stdio_channel host_streams_;

    static std::size_t translate(webcpp::wasi::stream_result result,
                                 boost::system::error_code& error) {
        switch (result.status) {
            case webcpp::wasi::stream_status::ready: error.clear(); break;
            case webcpp::wasi::stream_status::blocked:
                error = boost::system::errc::make_error_code(
                    boost::system::errc::operation_would_block);
                break;
            case webcpp::wasi::stream_status::eof: error = http::error::end_of_stream; break;
            case webcpp::wasi::stream_status::failed:
                error = boost::system::errc::make_error_code(boost::system::errc::io_error);
                break;
        }
        return result.count;
    }
#endif
    static std::size_t input(void* context, std::span<char> buffer,
                             boost::system::error_code& error) {
#ifdef __wasi__
        auto& self = *static_cast<stdio_stream*>(context);
#else
        const auto& self = *static_cast<stdio_stream*>(context);
#endif
        error = self.setup_error_;
        if (error) {
            return 0;
        }
#ifdef __wasi__
        if (self.input_ == STDIN_FILENO) {
            return translate(self.host_streams_.read(buffer), error);
        }
#endif
        const auto count =
            ::read(self.input_, buffer.data(), std::min<std::size_t>(buffer.size(), 4096));
        if (count >= 0) {
            error.clear();
            return static_cast<std::size_t>(count);
        }
        error =
            errno == EAGAIN || errno == EINTR
                ? boost::system::errc::make_error_code(boost::system::errc::operation_would_block)
                : boost::system::error_code(errno, boost::system::generic_category());
        return 0;
    }

    static std::ptrdiff_t write_descriptor(int descriptor, const void* data, std::size_t size) {
#ifdef __wasi__
        return ::write(descriptor, data, size);
#else
        // Block only this thread while writing, then restore its original mask.
        // An already pending SIGPIPE belongs to the caller and must remain pending.
        sigset_t selected{};
        sigset_t previous{};
        sigset_t pending{};
        static_cast<void>(sigemptyset(&selected));
        static_cast<void>(sigaddset(&selected, SIGPIPE));
        const auto failure = ::pthread_sigmask(SIG_BLOCK, &selected, &previous);
        if (failure != 0) {
            errno = failure;
            return -1;
        }
        static_cast<void>(::sigpending(&pending));
        const bool already_pending = sigismember(&pending, SIGPIPE) == 1;
        const auto count = ::write(descriptor, data, size);
        const auto saved_errno = errno;
        if (count < 0 && saved_errno == EPIPE && !already_pending) {
            static_cast<void>(::sigpending(&pending));
            if (sigismember(&pending, SIGPIPE) == 1) {
                int caught = 0;
                static_cast<void>(::sigwait(&selected, &caught));
            }
        }
        static_cast<void>(::pthread_sigmask(SIG_SETMASK, &previous, nullptr));
        errno = saved_errno;
        return count;
#endif
    }

    static std::size_t output(void* context, std::span<const char> buffer,
                              boost::system::error_code& error) {
        const auto& self = *static_cast<stdio_stream*>(context);
        error = self.setup_error_;
        if (error) {
            return 0;
        }
#ifdef __wasi__
        if (self.output_ == STDOUT_FILENO) {
            return translate(self.host_streams_.write(buffer), error);
        }
#endif
        const auto count = write_descriptor(self.output_, buffer.data(),
                                            std::min<std::size_t>(buffer.size(), 4096));
        if (count >= 0) {
            error.clear();
            return static_cast<std::size_t>(count);
        }
        error =
            errno == EAGAIN || errno == EINTR
                ? boost::system::errc::make_error_code(boost::system::errc::operation_would_block)
                : boost::system::error_code(errno, boost::system::generic_category());
        return 0;
    }

    static void wait(void* context, bool writing, const stream_control& control,
                     boost::system::error_code& error) {
        const auto& self = *static_cast<stdio_stream*>(context);
#ifdef __wasi__
        if ((!writing && self.input_ == STDIN_FILENO) ||
            (writing && self.output_ == STDOUT_FILENO)) {
            error = control.check();
            return;
        }
#endif
        pollfd descriptor{
            .fd = writing ? self.output_ : self.input_,
            .events = static_cast<short>(writing ? POLLOUT : POLLIN),
            .revents = 0,
        };
        for (;;) {
            error = control.check();
            if (error) {
                return;
            }
            const auto ready = ::poll(&descriptor, 1, 10);
            if (ready > 0) {
                error.clear();
                return;
            }
            if (ready < 0 && errno != EINTR) {
                error = {errno, boost::system::generic_category()};
                return;
            }
        }
    }

public:
    /** Acquires standard streams or borrows an exclusive descriptor pair.
        @param input_descriptor Input descriptor, normally stdin.
        @param output_descriptor Output descriptor, normally stdout.
        @param control Initial cancellation and deadline. */
    explicit stdio_stream(int input_descriptor = STDIN_FILENO,
                          int output_descriptor = STDOUT_FILENO, stream_control control = {})
        : input_(input_descriptor),
          output_(output_descriptor),
          input_flags_(::fcntl(input_, F_GETFL)),
          output_flags_(::fcntl(output_, F_GETFL)),
          stream_(this, input, output, wait, control) {
#ifdef __wasi__
        if (input_ == STDIN_FILENO && output_ == STDOUT_FILENO) {
            return;
        }
#endif
        if (input_flags_ == -1 || output_flags_ == -1 ||
            ::fcntl(input_, F_SETFL,
                    static_cast<int>(static_cast<unsigned int>(input_flags_) |
                                     static_cast<unsigned int>(O_NONBLOCK))) == -1 ||
            ::fcntl(output_, F_SETFL,
                    static_cast<int>(static_cast<unsigned int>(output_flags_) |
                                     static_cast<unsigned int>(O_NONBLOCK))) == -1) {
            setup_error_ = {errno, boost::system::generic_category()};
        }
    }

    /** Forbids duplicated channel ownership. */
    stdio_stream(const stdio_stream&) = delete;
    /** Forbids duplicated channel ownership. */
    stdio_stream& operator=(const stdio_stream&) = delete;

    /** Restores borrowed descriptor flags and releases host resources. */
    ~stdio_stream() {
#ifdef __wasi__
        if (input_ == STDIN_FILENO && output_ == STDOUT_FILENO) {
            return;
        }
#endif
        if (input_flags_ != -1) {
            static_cast<void>(::fcntl(input_, F_SETFL, input_flags_));
        }
        if (output_flags_ != -1) {
            static_cast<void>(::fcntl(output_, F_SETFL, output_flags_));
        }
    }

    /** Sets transfer cancellation and deadline.
        @param value Settings for subsequent operations. */
    void control(stream_control value) { stream_.control(value); }

    /** Reads a partial input span.
        @tparam Buffers Asio mutable buffer sequence.
        @param buffers Destination buffers.
        @param error Receives any transfer failure.
    @return The number of bytes transferred before completion or failure. */
    template <class Buffers>
    std::size_t read_some(const Buffers& buffers, boost::system::error_code& error) {
        return stream_.read_some(buffers, error);
    }

    /** Writes a partial output span.
        @tparam Buffers Asio constant buffer sequence.
        @param buffers Source buffers.
        @param error Receives any transfer failure.
    @return The number of bytes transferred before completion or failure. */
    template <class Buffers>
    std::size_t write_some(const Buffers& buffers, boost::system::error_code& error) {
        return stream_.write_some(buffers, error);
    }
};

#else
/** Unsupported standard-descriptor adapter on Windows.

    Use supplied_stream with caller-owned nonblocking read, write and wait callbacks. */
class stdio_stream {
public:
    /** Rejects construction where borrowed POSIX descriptor readiness is unavailable.
        @tparam Platform Deferred platform diagnostic.
        @param input_descriptor Input descriptor.
        @param output_descriptor Output descriptor.
        @param control Initial cancellation and deadline. */
    template <class Platform = void>
    explicit stdio_stream(int input_descriptor = 0, int output_descriptor = 1,
                          stream_control control = stream_control()) {
        static_cast<void>(input_descriptor);
        static_cast<void>(output_descriptor);
        static_cast<void>(control);
        static_assert(!std::is_same_v<Platform, Platform>,
                      "stdio_stream requires WASI or POSIX descriptors; on Windows use "
                      "supplied_stream callbacks");
    }
};
#endif

/** Parses one message and retains overread in the caller's buffer.

    Reads fit the buffer's remaining maximum capacity. If an incomplete parser
    needs more input while that capacity is exhausted, reports buffer_overflow.
    @tparam Stream Supplied stream type.
    @tparam DynamicBuffer Dynamic buffer type.
    @tparam Request Whether the parser reads a request.
    @tparam Body Beast body type.
    @tparam Allocator Parser fields allocator.
    @param stream Input channel.
    @param buffer Retained input and overread.
    @param parser Application parser with its limits.
    @param error Receives channel or parser failure.
    @return The number of bytes transferred before completion or failure. */
template <class Stream, class DynamicBuffer, bool Request, class Body, class Allocator>
std::size_t read(Stream& stream, DynamicBuffer& buffer,
                 http::parser<Request, Body, Allocator>& parser, boost::system::error_code& error) {
    error.clear();
    parser.eager(true);
    std::size_t total = 0;
    while (!parser.is_done()) {
        if (buffer.size() != 0) {
            const auto consumed = parser.put(buffer.data(), error);
            buffer.consume(consumed);
            total += consumed;
            if (error == http::error::need_more) {
                error.clear();
            }
            if (error || parser.is_done()) {
                break;
            }
        }
        const auto available = std::min<std::size_t>(4096, buffer.max_size() - buffer.size());
        if (available == 0) {
            error = http::error::buffer_overflow;
            break;
        }
        const auto transferred = stream.read_some(buffer.prepare(available), error);
        buffer.commit(transferred);
        if (error == http::error::end_of_stream) {
            if (parser.got_some()) {
                parser.put_eof(error);
            } else {
                error = http::error::end_of_stream;
            }
        }
        if (error || parser.is_done()) {
            break;
        }
    }
    return total;
}

/** Serializes one message and returns actual channel bytes.
    @tparam Stream Supplied stream type.
    @tparam Request Whether the message is a request.
    @tparam Body Beast body type.
    @tparam Fields Beast fields type.
    @param stream Output channel.
    @param message Application message.
    @param error Receives channel or serializer failure.
    @return The number of bytes transferred before completion or failure. */
template <class Stream, bool Request, class Body, class Fields>
std::size_t write(Stream& stream, http::message<Request, Body, Fields>& message,
                  boost::system::error_code& error) {
    error.clear();
    http::serializer<Request, Body, Fields> serializer(message);
    std::size_t total = 0;
    while (!serializer.is_done()) {
        std::size_t transferred = 0;
        serializer.next(error, [&](boost::system::error_code& next_error, const auto& buffers) {
            transferred = stream.write_some(buffers, next_error);
        });
        if (error) {
            break;
        }
        serializer.consume(transferred);
        total += transferred;
    }
    return total;
}
}  // namespace webcpp::beast::wasi

#endif
