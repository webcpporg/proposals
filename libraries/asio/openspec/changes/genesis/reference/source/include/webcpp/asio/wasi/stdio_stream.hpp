// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#ifndef WEBCPP_ASIO_WASI_STDIO_STREAM_HPP
#define WEBCPP_ASIO_WASI_STDIO_STREAM_HPP

#include <webcpp/asio/config.hpp>
#if defined(__MRDOCS__) || defined(WEBCPP_ASIO_REFERENCE)
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>

namespace webcpp::asio::wasi {
/** A WASIp2 CLI AsyncReadStream and AsyncWriteStream driven by pollables.

    Owns resource handles, without closing libc's stdin/stdout descriptors.
    Only one read and one write may be outstanding at once. Operations retain
    buffers until completion and preserve allocator, executor and cancellation
    associations. Host imports may still block according to the host's behavior.
    @see "Build for WASIp2", in the guide. */
class stdio_stream {
public:
    /** The executor used for readiness. */
    using executor_type = boost::asio::io_context::executor_type;
    /** This stream is its own lowest layer. */
    using lowest_layer_type = stdio_stream;
    /** Acquires CLI resources using the executor's context allocator.
        @param executor Drives asynchronous readiness. */
    explicit stdio_stream(executor_type const& executor);
    /** Cancels pending operations and releases resources after their completion. */
    ~stdio_stream();
    /** Forbids copied resource ownership. */
    stdio_stream(stdio_stream const&) = delete;
    /** Forbids copied resource ownership. */
    stdio_stream& operator=(stdio_stream const&) = delete;
    /** Transfers resource ownership. */
    stdio_stream(stdio_stream&&);
    /** Forbids replacing a stream that may have pending operations. */
    stdio_stream& operator=(stdio_stream&&) = delete;
    /** Returns the readiness executor.
        @return The executor. */
    [[nodiscard]] executor_type get_executor() const noexcept;
    /** Returns this stream.
        @return This stream. */
    lowest_layer_type& lowest_layer() noexcept;
    /** Cancels outstanding operations; subsequent operations remain usable. */
    void cancel();
    /** Cancels outstanding operations and disables subsequent operations. */
    void close();
    /** Reads a partial sequence asynchronously.
        @tparam Buffers Mutable buffer sequence.
        @tparam Token Completion token for void(error_code, size_t).
        @param buffers Destination storage, alive until completion.
        @param token Receives the error and byte count.
        @return The completion token's result. */
    template <class Buffers, class Token>
    auto async_read_some(Buffers const& buffers, Token&& token);
    /** Writes a partial sequence asynchronously.
        @tparam Buffers Const buffer sequence.
        @tparam Token Completion token for void(error_code, size_t).
        @param buffers Source storage, alive until completion.
        @param token Receives the error and byte count.
        @return The completion token's result. */
    template <class Buffers, class Token>
    auto async_write_some(Buffers const& buffers, Token&& token);
    /** Reads synchronously.
        @tparam Buffers Mutable buffer sequence.
        @param buffers Destination bytes.
        @param error Receives failure.
        @return Bytes read. */
    template <class Buffers>
    std::size_t read_some(Buffers const& buffers, boost::system::error_code& error);
    /** Writes synchronously.
        @tparam Buffers Const buffer sequence.
        @param buffers Source bytes.
        @param error Receives failure.
        @return Bytes written. */
    template <class Buffers>
    std::size_t write_some(Buffers const& buffers, boost::system::error_code& error);
    /** Reads synchronously and throws on failure.
        @tparam Buffers Mutable buffer sequence.
        @param buffers Destination bytes.
        @return Bytes read. */
    template <class Buffers>
    std::size_t read_some(Buffers const& buffers);
    /** Writes synchronously and throws on failure.
        @tparam Buffers Const buffer sequence.
        @param buffers Source bytes.
        @return Bytes written. */
    template <class Buffers>
    std::size_t write_some(Buffers const& buffers);
};

/** Cancels the channel for Beast timeouts.
    @param stream Channel to close. */
void beast_close_socket(stdio_stream& stream);
}  // namespace webcpp::asio::wasi
#elif !defined(__wasip2__)
#error "webcpp::asio::wasi::stdio_stream currently requires WASIp2"
#else
#include <wasi/wasip2.h>
#include <algorithm>
#include <boost/asio/append.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <cstring>
#include <memory>
#include <span>
#include <utility>
#include <webcpp/asio/detail/wasi_pollable_descriptor.hpp>

namespace webcpp::asio::wasi {
/** A byte stream over WASIp2 CLI input and output.

    Asynchronous operations wait on native pollables through Asio. They preserve
    associated executors, allocators and cancellation. The stream acquires its own
    resource handles; it never closes libc's stdin/stdout descriptors. Keep buffers
    alive until completion. Only one read and one write may be outstanding at once.
    A host implementation may still block inside an otherwise nonblocking import.
*/
class stdio_stream {
public:
    /** Executor that drives stream readiness. */
    using executor_type = boost::asio::io_context::executor_type;
    /** This stream has no additional transport layer. */
    using lowest_layer_type = stdio_stream;

private:
    struct state {
        executor_type executor;
        boost::asio::posix::stream_descriptor input_wait;
        boost::asio::posix::stream_descriptor output_wait;
        detail::wasi_pollable_descriptor input_storage{};
        detail::wasi_pollable_descriptor output_storage{};
        streams_own_input_stream_t input = stdin_get_stdin();
        streams_own_output_stream_t output = stdout_get_stdout();
        poll_own_pollable_t input_ready =
            streams_method_input_stream_subscribe(streams_borrow_input_stream(input));
        poll_own_pollable_t output_ready =
            streams_method_output_stream_subscribe(streams_borrow_output_stream(output));
        boost::system::error_code initialization_error;
        std::uint64_t generation = 0;
        bool closed = false;

        explicit state(const executor_type& value)
            : executor(value), input_wait(value), output_wait(value) {
            auto const assign = [&](auto& storage, auto& descriptor, auto ready) {
                storage.handle = ready.__handle;
                int const fd = webcpp_asio_wasi_pollable_descriptor(&storage);
                if (fd < 0) {
                    initialization_error.assign(errno, boost::system::system_category());
                } else {
                    boost::system::error_code error;
                    descriptor.assign(fd, error);
                    if (error) {
                        ::close(fd);
                        initialization_error = error;
                    }
                }
            };
            assign(input_storage, input_wait, input_ready);
            assign(output_storage, output_wait, output_ready);
        }

        ~state() {
            boost::system::error_code ignored;
            // The descriptor is closed even when close reports an error.
            // NOLINTNEXTLINE(bugprone-unused-return-value)
            static_cast<void>(input_wait.close(ignored));
            // The descriptor is closed even when close reports an error.
            // NOLINTNEXTLINE(bugprone-unused-return-value)
            static_cast<void>(output_wait.close(ignored));
            poll_pollable_drop_own(input_ready);
            poll_pollable_drop_own(output_ready);
            streams_input_stream_drop_own(input);
            streams_output_stream_drop_own(output);
        }

        void cancel() {
            ++generation;
            boost::system::error_code ignored;
            // Only an unassigned descriptor can fail; generation still cancels the operation.
            // NOLINTNEXTLINE(bugprone-unused-return-value)
            static_cast<void>(input_wait.cancel(ignored));
            // Only an unassigned descriptor can fail; generation still cancels the operation.
            // NOLINTNEXTLINE(bugprone-unused-return-value)
            static_cast<void>(output_wait.cancel(ignored));
        }

        std::size_t read(boost::asio::mutable_buffer buffer,
                         boost::system::error_code& error) const {
            wasip2_list_u8_t bytes{};
            streams_stream_error_t failure{};
            if (!streams_method_input_stream_read(streams_borrow_input_stream(input),
                                                  std::min<std::size_t>(buffer.size(), 65536),
                                                  &bytes, &failure)) {
                if (failure.tag == STREAMS_STREAM_ERROR_CLOSED) {
                    error = boost::asio::error::eof;
                } else {
                    error = boost::asio::error::fault;
                }
                streams_stream_error_free(&failure);
                return 0;
            }
            auto const count = bytes.len;
            if (count != 0u) {
                std::memcpy(buffer.data(), bytes.ptr, count);
            }
            wasip2_list_u8_free(&bytes);
            error = (count != 0u) ? boost::system::error_code{} : boost::asio::error::would_block;
            return count;
        }

        std::size_t write(boost::asio::const_buffer buffer, boost::system::error_code& error) {
            std::uint64_t capacity = 0;
            streams_stream_error_t failure{};
            if (!streams_method_output_stream_check_write(streams_borrow_output_stream(output),
                                                          &capacity, &failure)) {
                streams_stream_error_free(&failure);
                error = boost::asio::error::broken_pipe;
                return 0;
            }
            if (capacity == 0) {
                error = boost::asio::error::would_block;
                return 0;
            }
            auto const count = std::min(
                buffer.size(), static_cast<std::size_t>(std::min<std::uint64_t>(capacity, 4096)));
            wasip2_list_u8_t bytes{
                .ptr = reinterpret_cast<std::uint8_t*>(const_cast<void*>(buffer.data())),
                .len = count,
            };
            if (!streams_method_output_stream_write(streams_borrow_output_stream(output), &bytes,
                                                    &failure)) {
                streams_stream_error_free(&failure);
                error = boost::asio::error::broken_pipe;
                return 0;
            }
            // Request delivery before the protocol switches from writing to reading.
            // A subsequent check-write/pollable wait observes flush completion.
            if (!streams_method_output_stream_flush(streams_borrow_output_stream(output),
                                                    &failure)) {
                streams_stream_error_free(&failure);
                error = boost::asio::error::broken_pipe;
                return count;
            }
            error.clear();
            return count;
        }
    };

    std::shared_ptr<state> state_;

    template <bool Writing, class Buffer>
    struct transfer_operation {
        std::shared_ptr<state> shared;
        Buffer first;
        std::uint64_t generation = 0;

        // Initiation only posts. Resumption has a separate signature so the
        // composed operation cannot complete recursively inside initiation.
        template <class Self>
        void operator()(Self& self) {
            generation = shared->generation;
            self.reset_cancellation_state(boost::asio::enable_total_cancellation());
            boost::asio::post(shared->executor,
                              boost::asio::append(std::move(self), boost::system::error_code{}));
        }

        template <class Self>
        void operator()(Self& self, boost::system::error_code error) {
            if (self.cancelled() != boost::asio::cancellation_type::none || shared->closed ||
                generation != shared->generation) {
                error = boost::asio::error::operation_aborted;
            }
            if (!error) {
                error = shared->initialization_error;
            }
            if (error || first.size() == 0) {
                self.complete(error, 0);
                return;
            }
            std::size_t count = 0;
            if constexpr (Writing) {
                count = shared->write(first, error);
            } else {
                count = shared->read(first, error);
            }
            if (error != boost::asio::error::would_block) {
                self.complete(error, count);
                return;
            }
            auto& descriptor = Writing ? shared->output_wait : shared->input_wait;
            descriptor.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                                  std::move(self));
        }
    };

    template <bool Writing, class Buffers, class Token>
    auto async_some(Buffers const& buffers, Token&& token) {
        using buffer_type =
            std::conditional_t<Writing, boost::asio::const_buffer, boost::asio::mutable_buffer>;
        buffer_type first;
        for (auto it = boost::asio::buffer_sequence_begin(buffers),
                  end = boost::asio::buffer_sequence_end(buffers);
             it != end; ++it) {
            if (boost::asio::buffer_size(*it) != 0) {
                first = buffer_type(*it);
                break;
            }
        }
        return boost::asio::async_compose<Token, void(boost::system::error_code, std::size_t)>(
            transfer_operation<Writing, buffer_type>{.shared = state_, .first = first}, token,
            state_->executor);
    }

    template <bool Writing, class Buffers>
    std::size_t sync_some(Buffers const& buffers, boost::system::error_code& error) {
        if (state_->closed) {
            error = boost::asio::error::operation_aborted;
            return 0;
        }
        if (state_->initialization_error) {
            error = state_->initialization_error;
            return 0;
        }
        for (auto it = boost::asio::buffer_sequence_begin(buffers),
                  end = boost::asio::buffer_sequence_end(buffers);
             it != end; ++it) {
            if (boost::asio::buffer_size(*it) == 0) {
                continue;
            }
            for (;;) {
                std::size_t count = 0;
                if constexpr (Writing) {
                    count = state_->write(boost::asio::const_buffer(*it), error);
                } else {
                    count = state_->read(boost::asio::mutable_buffer(*it), error);
                }
                if (error != boost::asio::error::would_block) {
                    return count;
                }
                poll_method_pollable_block(
                    poll_borrow_pollable(Writing ? state_->output_ready : state_->input_ready));
            }
        }
        error.clear();
        return 0;
    }

public:
    /** Acquires independent CLI resource handles using the executor's context allocator. */
    explicit stdio_stream(const executor_type& executor)
        : state_(std::allocate_shared<state>(
              boost::asio::execution_context::allocator<state>(executor.context()), executor)) {}

    /** Cancels pending operations without closing the application's descriptors. */
    ~stdio_stream() {
        if (state_) {
            state_->closed = true;
            state_->cancel();
        }
    }

    stdio_stream(stdio_stream const&) = delete;
    stdio_stream& operator=(stdio_stream const&) = delete;
    stdio_stream(stdio_stream&&) = default;
    stdio_stream& operator=(stdio_stream&&) = delete;

    /** Returns the executor. */
    [[nodiscard]] executor_type get_executor() const noexcept { return state_->executor; }

    /** Returns this stream, which is its own lowest layer. */
    lowest_layer_type& lowest_layer() noexcept { return *this; }

    /** Cancels outstanding operations. Subsequent operations remain usable. */
    void cancel() { state_->cancel(); }

    /** Cancels and disables subsequent operations. */
    void close() {
        state_->closed = true;
        state_->cancel();
    }

    /** Reads a partial buffer sequence asynchronously. */
    template <class Buffers, class Token>
    auto async_read_some(Buffers const& buffers, Token&& token) {
        return async_some<false>(buffers, std::forward<Token>(token));
    }

    /** Writes a partial buffer sequence asynchronously. */
    template <class Buffers, class Token>
    auto async_write_some(Buffers const& buffers, Token&& token) {
        return async_some<true>(buffers, std::forward<Token>(token));
    }

    /** Reads synchronously, reporting failures through error. */
    template <class Buffers>
    std::size_t read_some(Buffers const& buffers, boost::system::error_code& error) {
        return sync_some<false>(buffers, error);
    }

    /** Writes synchronously, reporting failures through error. */
    template <class Buffers>
    std::size_t write_some(Buffers const& buffers, boost::system::error_code& error) {
        return sync_some<true>(buffers, error);
    }

    /** Reads synchronously, throwing on failure when exceptions are enabled. */
    template <class Buffers>
    std::size_t read_some(Buffers const& buffers) {
        boost::system::error_code error;
        auto count = read_some(buffers, error);
        if (error) {
            boost::throw_exception(boost::system::system_error(error));
        }
        return count;
    }

    /** Writes synchronously, throwing on failure when exceptions are enabled. */
    template <class Buffers>
    std::size_t write_some(Buffers const& buffers) {
        boost::system::error_code error;
        auto count = write_some(buffers, error);
        if (error) {
            boost::throw_exception(boost::system::system_error(error));
        }
        return count;
    }
};

/** Allows Beast timeouts to cancel the supplied channel without socket shutdown. */
inline void beast_close_socket(stdio_stream& stream) {
    stream.close();
}
}  // namespace webcpp::asio::wasi
#endif
#endif
