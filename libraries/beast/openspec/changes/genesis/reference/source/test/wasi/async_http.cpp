// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/deferred.hpp>
#include <memory_resource>
#include <webcpp/beast/wasi/async.hpp>

namespace host = webcpp::wasi::http;
namespace wasi = webcpp::beast::wasi;
namespace http = webcpp::beast::http;

namespace {
struct completion_resource final : std::pmr::memory_resource {
    std::size_t active = 0;

    ~completion_resource() override {
        if (active != 0) {
            std::abort();
        }
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto* result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++active;
        return result;
    }

    void do_deallocate(void* value, std::size_t bytes, std::size_t alignment) override {
        --active;
        std::pmr::new_delete_resource()->deallocate(value, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

void check_completion_lifetime() {
    wasi::http_context context;
    auto resource = std::make_unique<completion_resource>();
    bool completed = false;
    context.get_executor().post(
        [&] {
            resource.reset();  // Operation storage must already have been released.
            completed = true;
        },
        std::pmr::polymorphic_allocator<char>(resource.get()));
    if (resource->active != 1 || context.run() != 1 || !completed || resource) {
        std::abort();
    }
}

struct allocation_count {
    std::size_t active = 0, calls = 0;
};

template <class T>
struct counting_allocator {
    using value_type = T;
    allocation_count* count;

    explicit counting_allocator(allocation_count& value) : count(&value) {}

    template <class U>
    explicit counting_allocator(const counting_allocator<U>& other) : count(other.count) {}

    T* allocate(std::size_t size) {
        ++count->active;
        ++count->calls;
        return std::allocator<T>{}.allocate(size);
    }

    void deallocate(T* value, std::size_t size) {
        --count->active;
        std::allocator<T>{}.deallocate(value, size);
    }

    template <class U>
    bool operator==(const counting_allocator<U>& other) const {
        return count == other.count;
    }
};

struct test_session {
    host::incoming_request incoming;
    host::response_out out;
    std::string path;
    wasi::http_context context;
    wasi::http_context completion_context;
    allocation_count allocations;
    counting_allocator<char> allocator{allocations};
    std::unique_ptr<completion_resource> receive_resource;
    http::request<http::string_body> request;
    http::response<http::string_body> response{http::status::ok, 11};
    http::request<http::string_body> slow_request{http::verb::post, "/slow", 11};
    http::request<http::string_body> fast_request{http::verb::post, "/fast", 11};
    http::response<http::string_body> slow_reply;
    http::response<http::string_body> fast_reply;
    http::fields slow_trailers;
    http::fields fast_trailers;
    http::response<http::buffer_body> failed_response{http::status::ok, 11};
    boost::asio::cancellation_signal cancellation;
    std::optional<wasi::client> client;
    std::string authority;
    bool initiated = false;
    bool received = false;
    bool completed = false;
    unsigned clients_done = 0;
    bool fast_first = false;

    test_session(host::request_handle raw, host::response_out output)
        : incoming(raw), out(output), path(incoming.target()) {}

    void send() {
        response.prepare_payload();
        wasi::async_send(
            context, out, response,
            boost::asio::bind_allocator(
                allocator, [this](boost::system::error_code error, std::size_t count) {
                    if (error || count != response.body().size() || allocations.active != 0) {
                        std::abort();
                    }
                    completed = true;
                }));
    }

    void client_done(bool fast, boost::system::error_code error, std::size_t count) {
        auto& reply = fast ? fast_reply : slow_reply;
        if (error || count != request.body().size() || reply.body() != request.body()) {
            std::abort();
        }
        if (++clients_done == 1) {
            fast_first = fast;
        }
        if (clients_done == 2) {
            if (slow_trailers["x-reply"] != "finished" || fast_trailers["x-reply"] != "finished") {
                std::abort();
            }
            response.set("x-fast-first", fast_first ? "yes" : "no");
            response.body() = fast_reply.body();
            send();
        }
    }

    void fail_output() {
        failed_response.content_length(100);
        failed_response.body().data = const_cast<char*>("abc");
        failed_response.body().size = 3;
        failed_response.body().more = true;
        wasi::async_send(context, out, failed_response,
                         boost::asio::bind_allocator(
                             allocator, [this](boost::system::error_code ec, std::size_t bytes) {
                                 if (ec != http::error::need_buffer || bytes != 3) {
                                     std::abort();
                                 }
                                 completed = true;
                             }));
    }

    void cancel_client() {
        authority = std::string(request["x-peer"]);
        client.emplace("http", authority);
        slow_request.body() = request.body();
        slow_request.prepare_payload();
        if (path == "/abandon") {
            {
                wasi::http_context abandoned;
                wasi::async_perform(
                    abandoned, *client, slow_request, slow_reply,
                    boost::asio::bind_allocator(
                        allocator, [](boost::system::error_code, std::size_t) { std::abort(); }));
                abandoned.poll();
            }
            response.body() = "abandoned";
            send();
        } else {
            wasi::transfer_options limits;
            if (path == "/client-timeout") {
                limits.expires_after(30000000);
            }
            wasi::async_perform(
                context, *client, slow_request, slow_reply,
                boost::asio::bind_cancellation_slot(
                    cancellation.slot(),
                    boost::asio::bind_allocator(allocator,
                                                [this](boost::system::error_code ec, std::size_t) {
                                                    if (!ec) {
                                                        std::abort();
                                                    }
                                                    response.result(http::status::request_timeout);
                                                    response.body() = ec.message();
                                                    send();
                                                })),
                limits);
            if (path == "/cancel-client") {
                context.poll();
                cancellation.emit(boost::asio::cancellation_type::all);
            }
        }
    }

    void concurrent_clients(boost::system::error_code error) {
        if (error) {
            std::abort();
        }
        authority = std::string(request["x-peer"]);
        client.emplace("http", authority);
        slow_request.body() = request.body();
        fast_request.body() = request.body();
        slow_request.prepare_payload();
        fast_request.prepare_payload();
        wasi::async_perform(
            context, *client, slow_request, slow_reply,
            boost::asio::bind_allocator(allocator,
                                        [this](boost::system::error_code ec, std::size_t bytes) {
                                            client_done(false, ec, bytes);
                                        }),
            wasi::transfer_options(), &slow_trailers);
        wasi::async_perform(
            context, *client, fast_request, fast_reply,
            boost::asio::bind_allocator(allocator,
                                        [this](boost::system::error_code ec, std::size_t bytes) {
                                            client_done(true, ec, bytes);
                                        }),
            wasi::transfer_options(), &fast_trailers);
    }

    void receive_done(boost::system::error_code error, std::size_t count) {
        if (!initiated || count != request.body().size() || allocations.active != 0) {
            std::abort();
        }
        received = true;
        if (path == "/failed-output") {
            fail_output();
            return;
        }
        if (path == "/client-timeout" || path == "/cancel-client" || path == "/abandon") {
            cancel_client();
            return;
        }
        if (path == "/client") {
            concurrent_clients(error);
            return;
        }
        response.result(error ? http::status::request_timeout : http::status::ok);
        response.body() = error ? error.message() : request.body();
        send();
    }

    void run() {
        wasi::transfer_options options;
        if (path == "/timeout") {
            options.expires_after(30000000);
        }
        auto deferred =
            wasi::async_receive(context, incoming, request, boost::asio::deferred, options);
        if (allocations.calls != 0) {
            std::abort();
        }

        if (path == "/pmr") {
            receive_resource = std::make_unique<completion_resource>();
            std::move(deferred)(boost::asio::bind_executor(
                completion_context.get_executor(),
                boost::asio::bind_allocator(
                    std::pmr::polymorphic_allocator<char>(receive_resource.get()),
                    [this](boost::system::error_code error, std::size_t count) {
                        receive_resource.reset();
                        receive_done(error, count);
                    })));
        } else {
            std::move(deferred)(boost::asio::bind_executor(
                completion_context.get_executor(),
                boost::asio::bind_allocator(
                    allocator, boost::asio::bind_cancellation_slot(
                                   cancellation.slot(),
                                   [this](boost::system::error_code error, std::size_t count) {
                                       receive_done(error, count);
                                   }))));
        }
        initiated = true;
        if (path == "/cancel") {
            context.get_executor().post(
                [this] { cancellation.emit(boost::asio::cancellation_type::all); }, allocator);
        }
        context.run();
        if (received) {
            std::abort();  // Completion is bound to the other executor.
        }
        completion_context.run();
        context.run();
        const std::size_t minimum_calls = path == "/pmr" ? 2 : 4;
        if (!received || !completed || allocations.active != 0 || receive_resource ||
            allocations.calls < minimum_calls) {
            std::abort();
        }
    }
};

void handle(host::request_handle raw, host::response_out out) {
    check_completion_lifetime();
    test_session session(raw, out);
    session.run();
}
}  // namespace
#ifdef WEBCPP_WASI_HTTP_P2
extern "C" void exports_wasi_http_incoming_handler_handle(
    exports_wasi_http_incoming_handler_own_incoming_request_t request,
    exports_wasi_http_incoming_handler_own_response_outparam_t out) {
    handle(request, out);
}
#else
extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle(
    exports_wasi_http_handler_own_request_t request) {
    handle(request, {});
    return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;
}

extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle_callback(
    webcpp_wasi_http_event_t* /*unused*/) {
    return WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT;
}
#endif
