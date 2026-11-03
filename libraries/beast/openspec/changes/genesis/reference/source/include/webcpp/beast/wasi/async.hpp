// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_BEAST_WASI_ASYNC_HPP
#define WEBCPP_BEAST_WASI_ASYNC_HPP
#include <webcpp/beast/wasi/http.hpp>
#if defined(WEBCPP_WASI_HTTP_P2) || defined(WEBCPP_WASI_HTTP_P3)
#include <boost/asio/associated_allocator.hpp>  // lint-world: handler allocation association
#include <boost/asio/associated_cancellation_slot.hpp>  // lint-world: caller cancellation
#include <boost/asio/associated_executor.hpp>  // lint-world: completion executor association
#include <boost/asio/async_result.hpp>         // lint-world: reactor-free completion tokens
#include <boost/asio/bind_allocator.hpp>       // lint-world: completion allocation association
#include <boost/asio/execution.hpp>            // lint-world: completion executor properties
#include <boost/asio/prefer.hpp>               // lint-world: associated outstanding work
#include <boost/asio/require.hpp>              // lint-world: non-inline completion dispatch
#include <memory>
#include <optional>
#include <utility>
#include <webcpp/wasi/http/poll.hpp>

namespace webcpp::beast::wasi {
namespace detail {
/** One intrusive operation in a reactor-free host readiness queue. */
struct host_operation {
    /** Next operation in the intrusive readiness queue. */
    host_operation* next = nullptr;
    /** Advances one bounded readiness step and reports final completion. */
    virtual bool poll() = 0;
    /** Destroys this node with its retained associated allocator. */
    virtual void destroy() noexcept = 0;
    /** Releases pending host state and retained handler associations. */
    virtual ~host_operation() = default;
};

/** Constructs allocated state with cleanup if its constructor throws. */
template <class T, class Allocator, class... Args>
T* allocate_host_state(Allocator& allocator, Args&&... args) {
    auto* memory = std::allocator_traits<Allocator>::allocate(allocator, 1);
    auto const release = [&allocator](T* value) {
        std::allocator_traits<Allocator>::deallocate(allocator, value, 1);
    };
    std::unique_ptr<T, decltype(release)> guard(memory, release);
    std::allocator_traits<Allocator>::construct(allocator, memory, std::forward<Args>(args)...);
    return guard.release();
}

/** Retains outstanding work on either legacy or property-based associated executors. */
template <class Executor>
struct host_work {
    /** Selects tracked work only for executors that implement property-based execution. */
    static auto retain(const Executor& value) {
        if constexpr (requires { value.on_work_started(); }) {
            return value;
        } else {
            return boost::asio::prefer(value, boost::asio::execution::outstanding_work_t::tracked);
        }
    }

    /** Executor type with outstanding work retained where supported. */
    using tracked_type = decltype(retain(std::declval<Executor>()));
    /** Associated executor retained through completion scheduling. */
    tracked_type executor;

    /** Retains associated executor work for the pending operation. */
    explicit host_work(const Executor& value) : executor(retain(value)) {
        if constexpr (requires { executor.on_work_started(); }) {
            executor.on_work_started();
        }
    }

    /** Releases retained outstanding executor work. */
    ~host_work() {
        if constexpr (requires { executor.on_work_finished(); }) {
            executor.on_work_finished();
        }
    }
};

/** Allocator-owned posted function with no transport operation. */
template <class Function, class Allocator>
struct host_job final : host_operation {
    /** Allocator rebound to this operation node. */
    using allocator_type = std::allocator_traits<Allocator>::template rebind_alloc<host_job>;
    /** Posted function retained until its queued invocation. */
    Function function;
    /** Associated allocator retained for matching deallocation. */
    allocator_type allocator;

    /** Constructs retained transfer state from caller-owned resources. */
    host_job(Function value, allocator_type alloc) : function(std::move(value)), allocator(alloc) {}

    /** Advances one bounded readiness step and reports final completion. */
    bool poll() override {
        function();
        return true;
    }

    /** Destroys this node with its retained associated allocator. */
    void destroy() noexcept override {
        auto copy = allocator;
        std::allocator_traits<allocator_type>::destroy(copy, this);
        std::allocator_traits<allocator_type>::deallocate(copy, this, 1);
    }
};
}  // namespace detail

/** Drives bounded host readiness operations without a socket reactor or worker thread.

    poll performs one bounded scan and never waits for data. run yields the current
    component task between scans. Callers keep messages and resources alive until
    their completion handler runs. Destroying the context abandons pending handlers
    and cancels their host resource operations before releasing state. */
class http_context {
    detail::host_operation* first_ = nullptr;
    detail::host_operation* last_ = nullptr;
    std::size_t size_ = 0;
    std::size_t work_ = 0;

public:
    /** An executor that queues work on its owning host context. */
    class executor_type {
        http_context* context_;

    public:
        /** Refers to one host context.
            @param context Owning execution context. */
        explicit executor_type(http_context& context) noexcept : context_(&context) {}

        /** Returns the owning context.
            @return The owning host context. */
        [[nodiscard]] http_context& context() const noexcept { return *context_; }

        /** Retains externally associated outstanding work. */
        void on_work_started() const noexcept { ++context_->work_; }

        /** Releases externally associated outstanding work. */
        void on_work_finished() const noexcept { --context_->work_; }

        /** Queues a function using the supplied allocator.
            @tparam Function Nullary callable.
            @tparam Allocator State allocator.
            @param function Work to execute on a subsequent poll.
            @param allocator Allocator used for the queue node. */
        template <class Function, class Allocator>
        void post(Function&& function, const Allocator& allocator) const {
            using job = detail::host_job<std::decay_t<Function>, Allocator>;
            typename job::allocator_type alloc(allocator);
            context_->enqueue(
                detail::allocate_host_state<job>(alloc, std::forward<Function>(function), alloc));
        }

        /** Queues a continuation without inline invocation.
            @tparam Function Nullary callable.
            @tparam Allocator State allocator.
            @param function Work to execute.
            @param allocator Allocator for the queue node. */
        template <class Function, class Allocator>
        void defer(Function&& function, const Allocator& allocator) const {
            post(std::forward<Function>(function), allocator);
        }

        /** Queues dispatch work without inline invocation.
            @tparam Function Nullary callable.
            @tparam Allocator State allocator.
            @param function Work to execute.
            @param allocator Allocator for the queue node. */
        template <class Function, class Allocator>
        void dispatch(Function&& function, const Allocator& allocator) const {
            post(std::forward<Function>(function), allocator);
        }

        /** Compares context identity.
            @param other Executor to compare.
            @return Whether the executors share a context. */
        bool operator==(const executor_type& other) const noexcept {
            return context_ == other.context_;
        }
    };

    /** Constructs an empty host context. */
    http_context() = default;
    /** Forbids duplicated ownership of pending operations. */
    http_context(const http_context&) = delete;
    /** Forbids duplicated ownership of pending operations. */
    http_context& operator=(const http_context&) = delete;

    /** Cancels and releases pending operations without invoking abandoned handlers. */
    ~http_context() {
        while (first_ != nullptr) {
            auto* value = first_;
            first_ = value->next;
            value->destroy();
        }
    }

    /** Returns the executor of this context.
        @return A borrowing executor. */
    executor_type get_executor() noexcept { return executor_type(*this); }

    /** Takes ownership of an intrusive readiness operation.
        @param value Allocator-owned operation, with its matching destroy function. */
    void enqueue(detail::host_operation* value) {
        value->next = nullptr;
        if (last_ != nullptr) {
            last_->next = value;
        } else {
            first_ = value;
        }
        last_ = value;
        ++size_;
    }

    /** Polls each initially queued operation once without waiting for readiness.
        @return The number of completed operations or posted functions. */
    std::size_t poll() {
        std::size_t completed = 0;
        auto count = size_;
        while (((count--) != 0u) && (first_ != nullptr)) {
            auto* value = first_;
            first_ = value->next;
            --size_;
            if (first_ == nullptr) {
                last_ = nullptr;
            }
            auto const destroy = [](detail::host_operation* operation) { operation->destroy(); };
            std::unique_ptr<detail::host_operation, decltype(destroy)> guard(value, destroy);
            if (value->poll()) {
                ++completed;
            } else {
                enqueue(guard.release());
            }
        }
        return completed;
    }

    /** Runs until no owned operation or associated work remains.
        @return The number of completed operations or posted functions. */
    std::size_t run() {
        std::size_t completed = 0;
        while ((first_ != nullptr) || (work_ != 0u)) {
            completed += poll();
            if ((first_ != nullptr) || (work_ != 0u)) {
                webcpp::wasi::http::detail::yield_polling_task();
            }
        }
        return completed;
    }
};

namespace detail {
/** Posts a completion to its associated executor without inline invocation. */
template <class Executor, class Allocator, class Function>
void host_post(const Executor& executor, const Allocator& allocator, Function&& function) {
    if constexpr (requires { executor.post(std::forward<Function>(function), allocator); }) {
        executor.post(std::forward<Function>(function), allocator);
    } else {
        boost::asio::require(executor, boost::asio::execution::blocking_t::never)
            .execute(boost::asio::bind_allocator(allocator, std::forward<Function>(function)));
    }
}

/** Owns handler-associated allocation, cancellation and completion dispatch. */
template <class State, class Handler>
struct host_async_operation final : host_operation {
    /** Allocator rebound to this operation node. */
    using allocator_type = std::allocator_traits<
        boost::asio::associated_allocator_t<Handler>>::template rebind_alloc<host_async_operation>;
    /** Handler-associated executor with the host executor fallback. */
    using executor_type = boost::asio::associated_executor_t<Handler, http_context::executor_type>;
    /** Associated allocator retained for matching deallocation. */
    allocator_type allocator;
    /** Completion handler retained until the operation finishes. */
    Handler handler;
    /** Associated executor retained through completion scheduling. */
    executor_type executor;
    /** Outstanding associated executor work retained through completion posting. */
    host_work<executor_type> work;
    /** Associated cancellation slot disconnected before state release. */
    boost::asio::associated_cancellation_slot_t<Handler> slot;
    /** Cancellation observed on the component task. */
    bool cancelled = false;
    /** Owned bounded-step transfer state. */
    State state;

    /** Constructs retained state and its handler associations. */
    template <class... Args>
    host_async_operation(allocator_type alloc, Handler value, http_context& context, Args&&... args)
        : allocator(alloc),
          handler(std::move(value)),
          executor(boost::asio::get_associated_executor(handler, context.get_executor())),
          work(executor),
          slot(boost::asio::get_associated_cancellation_slot(handler)),
          state(std::forward<Args>(args)...) {
        if (slot.is_connected()) {
            slot.assign([this](boost::asio::cancellation_type type) {
                if (type != boost::asio::cancellation_type::none) {
                    cancelled = true;
                }
            });
        }
    }

    /** Releases pending host state and retained handler associations. */
    ~host_async_operation() override {
        if (slot.is_connected()) {
            slot.clear();
        }
    }

    /** Advances one bounded readiness step and reports final completion. */
    bool poll() override {
        boost::system::error_code error;
        if (!state.poll(cancelled, error)) {
            return false;
        }
        if (slot.is_connected()) {
            slot.clear();
        }
        auto const count = state.total;
        auto const alloc = boost::asio::get_associated_allocator(handler);
        host_post(executor, alloc,
                  [value = std::move(handler), error, count]() mutable { value(error, count); });
        return true;
    }

    /** Destroys this node with its retained associated allocator. */
    void destroy() noexcept override {
        auto alloc = allocator;
        std::allocator_traits<allocator_type>::destroy(alloc, this);
        std::allocator_traits<allocator_type>::deallocate(alloc, this, 1);
    }
};

/** Allocates one composed operation through the completion handler's allocator. */
template <class State, class Handler, class... Args>
void start_host_operation(http_context& context, Handler&& handler, Args&&... args) {
    using operation = host_async_operation<State, std::decay_t<Handler>>;
    typename operation::allocator_type allocator(boost::asio::get_associated_allocator(handler));
    context.enqueue(allocate_host_state<operation>(allocator, allocator,
                                                   std::forward<Handler>(handler), context,
                                                   std::forward<Args>(args)...));
}

/** Keeps decoded-body state and a stable pending read buffer in one allocation. */
template <bool Request, class Body, class Fields, class Trailers>
struct host_read_message {
    /** Caller-owned Beast message retained by reference. */
    http::message<Request, Body, Fields>& message;
    /** Optional trailer fields retained until body completion. */
    Trailers* trailers;
    /** Copied body limit, cancellation predicate and deadline. */
    transfer_options options;
    /** Stable bounded payload storage borrowed by pending host reads. */
    std::array<char, 4096> buffer{};
    /** Caller-selected BodyReader retained across partial input. */
    std::optional<typename Body::reader> reader;
    /** Owned nonblocking host input task. */
    std::optional<host::read_task> source;
    /** Logical body payload bytes transferred so far. */
    std::size_t total = 0;

    /** Constructs retained transfer state from caller-owned resources. */
    host_read_message(http::message<Request, Body, Fields>& value, Trailers* final_fields,
                      transfer_options limits)
        : message(value), trailers(final_fields), options(limits) {}

    /** Acquires the host body and initializes the caller-selected body algorithm. */
    void start(host::body_reader body, boost::system::error_code& error) {
        source.emplace(std::move(body));
        reader.emplace(message.base(), message.body());
        reader->init(boost::none, error);
    }

    /** Advances one bounded readiness step and reports final completion. */
    bool poll(bool cancelled, boost::system::error_code& error) {
        if (cancelled) {
            error = error_code({.code = host::message_errc::cancelled});
            return true;
        }
        assert(source.has_value());
        assert(reader.has_value());
        host::message_error failure;
        if (!source->eof()) {
            std::size_t count = 0;
            if (!source->read(buffer, count, options, failure)) {
                return false;
            }
            error = error_code(failure);
            if (error) {
                return true;
            }
            total += reader->put(boost::asio::buffer(buffer.data(), count), error);
            return static_cast<bool>(error);
        }
        if (!source->finish(
                [&](std::string_view name, std::string_view value) {
                    if (trailers) {
                        trailers->insert(view(name), view(value));
                    }
                },
                options, failure)) {
            return false;
        }
        error = error_code(failure);
        if (!error) {
            reader->finish(error);
        }
        return true;
    }
};

/** Keeps BodyWriter buffers stable across nonblocking partial host writes. */
template <bool Request, class Body, class Fields, class Trailers>
struct host_write_message {
    /** Caller-owned Beast message retained by reference. */
    http::message<Request, Body, Fields>& message;
    /** Copied body limit, cancellation predicate and deadline. */
    transfer_options options;
    /** Optional trailer fields retained until body completion. */
    std::optional<host::message_fields> trailers;
    /** Caller-selected BodyWriter retained across partial output. */
    std::optional<typename Body::writer> writer;
    /** Optional BodyWriter buffer sequence and continuation marker. */
    using chunk_type = decltype(std::declval<typename Body::writer&>().get(
        std::declval<boost::system::error_code&>()));
    /** Current BodyWriter buffers retained until every byte is consumed. */
    chunk_type chunk;
    /** Bytes already consumed from the current buffer sequence. */
    std::size_t offset = 0;
    /** Whether only trailer completion remains. */
    bool ending = false;
    /** Logical body payload bytes transferred so far. */
    std::size_t total = 0;
    /** Owned nonblocking host output task. */
    std::optional<host::write_task> target;

    /** Constructs retained transfer state from caller-owned resources. */
    host_write_message(http::message<Request, Body, Fields>& value, transfer_options limits)
        : message(value), options(limits) {}

    /** Acquires the host body and initializes the caller-selected body algorithm. */
    void start(host::body_writer body, const Trailers* final_fields,
               boost::system::error_code& error) {
        target.emplace(std::move(body));
        if (final_fields) {
            trailers.emplace(copy_fields(*final_fields, error));
        }
        if (error) {
            return;
        }
        writer.emplace(message.base(), message.body());
        writer->init(error);
    }

    /** Advances one bounded readiness step and reports final completion. */
    bool poll(bool cancelled, boost::system::error_code& error) {
        if (cancelled) {
            error = error_code({.code = host::message_errc::cancelled});
            return true;
        }
        error = error_code(options.check());
        if (error) {
            return true;
        }
        assert(target.has_value());
        assert(writer.has_value());
        host::message_error failure;
        if (ending) {
            if (!target->finish(trailers ? &*trailers : nullptr, failure)) {
                return false;
            }
            error = error_code(failure);
            return true;
        }
        if (!chunk) {
            chunk = writer->get(error);
            offset = 0;
        }
        if (error) {
            return true;
        }
        if (!chunk) {
            ending = true;
            return false;
        }
        std::size_t skipped = 0;
        for (auto const buffer : boost::beast::buffers_range_ref(chunk->first)) {
            if (skipped + buffer.size() <= offset) {
                skipped += buffer.size();
                continue;
            }
            const auto start = offset - skipped;
            std::string_view const bytes(static_cast<const char*>(buffer.data()) + start,
                                         buffer.size() - start);
            std::size_t count = 0;
            if (!target->write(bytes, count, options, failure)) {
                return false;
            }
            offset += count;
            total += count;
            error = error_code(failure);
            return static_cast<bool>(error);
        }
        ending = !chunk->second;
        chunk.reset();
        return false;
    }
};

/** Composes incoming metadata, decoded body and trailers. */
template <class Body, class Fields, class Trailers>
struct host_receive_state {
    /** Incoming request or response resource retained by this state. */
    host::incoming_request& incoming;
    /** Bounded message-body transfer state. */
    host_read_message<true, Body, Fields, Trailers> body;
    /** Whether the transfer has acquired its host body resource. */
    bool started = false;
    /** Logical body payload bytes transferred so far. */
    std::size_t total = 0;

    /** Constructs retained transfer state from caller-owned resources. */
    host_receive_state(host::incoming_request& source, http::request<Body, Fields>& request,
                       transfer_options options, Trailers* trailers)
        : incoming(source), body(request, trailers, options) {}

    /** Advances one bounded readiness step and reports final completion. */
    bool poll(bool cancelled, boost::system::error_code& error) {
        if (!started) {
            if (cancelled) {
                error = error_code({.code = host::message_errc::cancelled});
                return true;
            }
            auto& request = body.message;
            request.version(11);
            request.method_string(view(incoming.method()));
            request.target(view(incoming.target()));
            incoming.headers().for_each([&](std::string_view name, std::string_view value) {
                request.insert(view(name), view(value));
            });
            if (!incoming.authority().empty() && request.find(http::field::host) == request.end()) {
                request.set(http::field::host, view(incoming.authority()));
            }
            host::message_error failure;
            auto source = incoming.consume(failure);
            error = error_code(failure);
            if (error) {
                return true;
            }
            body.start(std::move(source), error);
            started = true;
            if (error) {
                return true;
            }
        }
        auto const done = body.poll(cancelled, error);
        total = body.total;
        return done;
    }
};

/** Composes a single response commitment, bounded body writes and trailers. */
template <class Body, class Fields, class Trailers>
struct host_send_state {
    /** Response commitment capability consumed at most once. */
    host::response_out output;
    /** Bounded message-body transfer state. */
    host_write_message<false, Body, Fields, Trailers> body;
    /** Optional trailer fields retained until body completion. */
    const Trailers* trailers;
    /** Whether the transfer has acquired its host body resource. */
    bool started = false;
    /** Logical body payload bytes transferred so far. */
    std::size_t total = 0;

    /** Constructs retained transfer state from caller-owned resources. */
    host_send_state(host::response_out out, http::response<Body, Fields>& response,
                    transfer_options options, const Trailers* fields)
        : output(out), body(response, options), trailers(fields) {}

    /** Releases pending host state and retained handler associations. */
    ~host_send_state() {
        if (!started) {
            host::refuse_response(output);
        }
    }

    /** Advances one bounded readiness step and reports final completion. */
    bool poll(bool cancelled, boost::system::error_code& error) {
        if (!started) {
            if (cancelled) {
                error = error_code({.code = host::message_errc::cancelled});
                return true;
            }
            auto fields = copy_fields(body.message.base(), error);
            if (error) {
                return true;
            }
            host::message_error failure;
            auto target = host::commit_response(output, body.message.result_int(),
                                                std::move(fields), failure);
            started = true;
            error = error_code(failure);
            if (error) {
                return true;
            }
            body.start(std::move(target), trailers, error);
            if (error) {
                return true;
            }
        }
        auto const done = body.poll(cancelled, error);
        total = body.total;
        return done;
    }
};

/** Composes outgoing headers, upload, response headers and decoded response body. */
template <class RequestBody, class RequestFields, class ResponseBody, class ResponseFields,
          class Trailers>
struct host_client_state {
    /** Borrowed host HTTP scheme for the pending exchange. */
    std::string_view scheme;
    /** Borrowed host authority for the pending exchange. */
    std::string_view authority;
    /** Outgoing host exchange retained until upload and response finish. */
    host::outgoing_exchange exchange;
    /** Incoming request or response resource retained by this state. */
    host::incoming_response incoming;
    /** Bounded outgoing request-body state. */
    host_write_message<true, RequestBody, RequestFields, Trailers> upload;
    /** Bounded incoming response-body state. */
    host_read_message<false, ResponseBody, ResponseFields, Trailers> download;
    /** Response-header readiness operation borrowing the exchange. */
    host::response_task response;
    /** Optional caller-owned request trailers. */
    const Trailers* upload_trailers;
    /** Current upload, response-header or download stage. */
    unsigned stage = 0;
    /** Logical body payload bytes transferred so far. */
    std::size_t total = 0;

    /** Constructs retained transfer state from caller-owned resources. */
    host_client_state(client& connection, http::request<RequestBody, RequestFields>& request,
                      http::response<ResponseBody, ResponseFields>& reply, transfer_options options,
                      Trailers* response_trailers, const Trailers* request_trailers)
        : scheme(connection.scheme()),
          authority(connection.authority()),
          upload(request, options),
          download(reply, response_trailers, options),
          response(exchange),
          upload_trailers(request_trailers) {}

    /** Advances one bounded readiness step and reports final completion. */
    bool poll(bool cancelled, boost::system::error_code& error) {
        if (cancelled) {
            error = error_code({.code = host::message_errc::cancelled});
            return true;
        }
        error = error_code(upload.options.check());
        if (error) {
            return true;
        }
        if (stage == 0) {
            auto headers = copy_fields(upload.message.base(), error, authority);
            if (error) {
                return true;
            }
            host::message_error failure;
            auto writer =
                exchange.start(scheme, authority, view(upload.message.method_string()),
                               view(upload.message.target()), std::move(headers), failure);
            error = error_code(failure);
            if (error) {
                return true;
            }
            upload.start(std::move(writer), upload_trailers, error);
            if (error) {
                return true;
            }
            stage = 1;
        }
        if (stage == 1) {
            if (!upload.poll(cancelled, error)) {
                return false;
            }
            if (error) {
                return true;
            }
            stage = 2;
        }
        if (stage == 2) {
            host::message_error failure;
            if (!response.poll(incoming, download.options, failure)) {
                return false;
            }
            error = error_code(failure);
            if (error) {
                return true;
            }
            download.message.version(11);
            download.message.result(incoming.status());
            incoming.headers().for_each([&](std::string_view name, std::string_view value) {
                download.message.insert(view(name), view(value));
            });
            auto reader = incoming.consume(failure);
            error = error_code(failure);
            if (error) {
                return true;
            }
            download.start(std::move(reader), error);
            if (error) {
                return true;
            }
            stage = 3;
        }
        auto const done = download.poll(cancelled, error);
        total = download.total;
        return done;
    }
};
}  // namespace detail

/** Asynchronously reads a complete host request with bounded readiness steps.
    @tparam Body Destination body type.
    @tparam Fields Destination fields type.
    @tparam CompletionToken Token accepting void(error_code, size_t).
    @tparam Trailers Trailer fields type.
    @param context Context driving host readiness.
    @param source Incoming resource kept alive through completion.
    @param request Initially empty destination kept alive through completion.
    @param token Completion token preserving executor, allocator and cancellation slot.
    @param options Body limit, cancellation and deadline.
    @param trailers Optional destination kept alive through completion.
    @return The completion token's result. */
template <class Body, class Fields, class CompletionToken, class Trailers = http::fields>
auto async_receive(http_context& context, detail::host::incoming_request& source,
                   http::request<Body, Fields>& request, CompletionToken&& token,
                   const transfer_options& options = transfer_options(),
                   Trailers* trailers = nullptr) {
    return boost::asio::async_initiate<CompletionToken,
                                       void(boost::system::error_code, std::size_t)>(
        [&context, &source, &request, options, trailers](auto&& handler) {
            detail::start_host_operation<detail::host_receive_state<Body, Fields, Trailers>>(
                context, std::forward<decltype(handler)>(handler), source, request, options,
                trailers);
        },
        token);
}

/** Asynchronously commits and writes one host response with bounded readiness steps.
    @tparam Body Source body type.
    @tparam Fields Source fields type.
    @tparam CompletionToken Token accepting void(error_code, size_t).
    @tparam Trailers Trailer fields type.
    @param context Context driving host readiness.
    @param output Active response commitment capability.
    @param response Source message kept alive through completion.
    @param token Completion token preserving executor, allocator and cancellation slot.
    @param options Cancellation and deadline.
    @param trailers Optional source kept alive through completion.
    @return The completion token's result. */
template <class Body, class Fields, class CompletionToken, class Trailers = http::fields>
auto async_send(http_context& context, detail::host::response_out output,
                http::response<Body, Fields>& response, CompletionToken&& token,
                const transfer_options& options = transfer_options(),
                const Trailers* trailers = nullptr) {
    return boost::asio::async_initiate<CompletionToken,
                                       void(boost::system::error_code, std::size_t)>(
        [&context, output, &response, options, trailers](auto&& handler) {
            detail::start_host_operation<detail::host_send_state<Body, Fields, Trailers>>(
                context, std::forward<decltype(handler)>(handler), output, response, options,
                trailers);
        },
        token);
}

/** Asynchronously performs a host client exchange with bounded readiness steps.
    @tparam RequestBody Outgoing body type.
    @tparam RequestFields Outgoing fields type.
    @tparam ResponseBody Incoming body type.
    @tparam ResponseFields Incoming fields type.
    @tparam CompletionToken Token accepting void(error_code, size_t).
    @tparam Trailers Trailer fields type.
    @param context Context driving host readiness.
    @param connection Host client whose metadata storage remains alive through completion.
    @param request Source message kept alive through completion.
    @param response Initially empty destination kept alive through completion.
    @param token Completion token preserving executor, allocator and cancellation slot.
    @param options Body limit, cancellation and deadline.
    @param response_trailers Optional destination kept alive through completion.
    @param request_trailers Optional source kept alive through completion.
    @return The completion token's result, with decoded response payload byte count. */
template <class RequestBody, class RequestFields, class ResponseBody, class ResponseFields,
          class CompletionToken, class Trailers = http::fields>
auto async_perform(http_context& context, client& connection,
                   http::request<RequestBody, RequestFields>& request,
                   http::response<ResponseBody, ResponseFields>& response, CompletionToken&& token,
                   const transfer_options& options = transfer_options(),
                   Trailers* response_trailers = nullptr,
                   const Trailers* request_trailers = nullptr) {
    return boost::asio::async_initiate<CompletionToken,
                                       void(boost::system::error_code, std::size_t)>(
        [&context, &connection, &request, &response, options, response_trailers,
         request_trailers](auto&& handler) {
            detail::start_host_operation<detail::host_client_state<
                RequestBody, RequestFields, ResponseBody, ResponseFields, Trailers>>(
                context, std::forward<decltype(handler)>(handler), connection, request, response,
                options, response_trailers, request_trailers);
        },
        token);
}
}  // namespace webcpp::beast::wasi
#endif
#endif
