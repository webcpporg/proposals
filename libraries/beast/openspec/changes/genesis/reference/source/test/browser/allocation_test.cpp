// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Count global allocation from stream construction through the drained close completion.
// Supplied resources allocate directly, so every unbound stream allocation is observable.
#include <emscripten/val.h>
#include <array>
#include <boost/asio/bind_allocator.hpp>
#include <boost/throw_exception.hpp>
#include <cstdio>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <webcpp/asio.hpp>
#include <webcpp/beast/browser/http_byte_stream.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/ssl.hpp>
#include <webcpp/beast/websocket.hpp>

namespace {
// Global replacement operators can reach only process-wide test counters.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool measuring = false;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t escaped = 0;

void* storage(std::size_t size, std::size_t alignment) {
    alignment = std::max(alignment, alignof(std::max_align_t));
    return std::aligned_alloc(
        alignment, ((std::max(size, std::size_t{1}) + alignment - 1) / alignment) * alignment);
}

void* allocate(std::size_t size, std::size_t alignment) {
    if (measuring) {
        ++escaped;
    }
    auto* pointer = storage(size, alignment);
    if (pointer == nullptr) {
        boost::throw_exception(std::bad_alloc());
    }
    return pointer;
}

struct resource final : std::pmr::memory_resource {
    std::size_t live = 0;
    std::size_t total = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        ++total;
        live += size;
        auto* pointer = storage(size, alignment);
        if (pointer == nullptr) {
            boost::throw_exception(std::bad_alloc());
        }
        return pointer;
    }

    void do_deallocate(void* pointer, std::size_t size, std::size_t /*unused*/) override {
        live -= size;
        std::free(pointer);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct registration final : webcpp::asio::browser::intrusive_registration {
    int delivered = 0;
    bool stopped = false;

    bool deliver(const emscripten::val& /*unused*/) override {
        ++delivered;
        return false;
    }

    void shutdown() noexcept override { stopped = true; }
};

bool registration_lifetimes(resource& context_memory, resource& stream_memory) {
    const auto stream_allocator = std::pmr::polymorphic_allocator<std::byte>(&stream_memory);
    std::array<std::shared_ptr<registration>, 32> registrations;
    std::array<std::uint64_t, 32> keys{};
    auto late = emscripten::val::undefined();
    {
        webcpp::asio::io_context context(
            std::allocator_arg, std::pmr::polymorphic_allocator<std::byte>(&context_memory));
        auto& bridge = boost::asio::use_service<webcpp::asio::browser::bridge>(context);
        const auto initial = context_memory.total;
        for (std::size_t i = 0; i != registrations.size(); ++i) {
            registrations[i] = std::allocate_shared<registration>(stream_allocator);
            keys[i] = bridge.add(*registrations[i]);
        }
        if (context_memory.total != initial) {
            return false;
        }
        late = bridge.function(keys.front());
        late(emscripten::val::undefined());
        bridge.remove(keys.front());
        late(emscripten::val::undefined());
        if (registrations.front()->delivered != 1) {
            return false;
        }
    }
    late(emscripten::val::undefined());
    if (registrations.front()->delivered != 1 || registrations.front()->stopped) {
        return false;
    }
    return std::all_of(registrations.begin() + 1, registrations.end(),
                       [](const auto& value) { return value->stopped; });
}

bool http_allocations(webcpp::asio::io_context& io,
                      const webcpp::asio::ip::tcp::resolver::results_type& addresses,
                      std::string_view authority, resource& stream_memory,
                      resource& operation_memory) {
    namespace beast = webcpp::beast;
    const auto stream_allocator = std::pmr::polymorphic_allocator<std::byte>(&stream_memory);
    const auto operation_allocator = std::pmr::polymorphic_allocator<std::byte>(&operation_memory);
    boost::system::error_code ec;
    bool failed = false;
    {
        beast::tcp_stream transport(std::allocator_arg, stream_allocator, io);
        transport.connect(
            {boost::asio::ip::make_address("2001:db8:1234:5678:9abc:def0:1234:5678"), 80}, ec);
        failed = failed || static_cast<bool>(ec);
        std::printf("allocation after numeric IPv6: outside=%zu\n", escaped);
    }
    {
        beast::tcp_stream transport(std::allocator_arg, stream_allocator, io);
        transport.connect(addresses, ec);
        beast::browser::http_byte_stream adapter(transport);
        constexpr std::string_view payload =
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        std::pmr::string request("POST /echo HTTP/1.1\r\nHost: ", &stream_memory);
        request.append(authority);
        request.append("\r\nContent-Length: 64\r\nX-Allocator-Field: ");
        request.append(payload);
        request.append("\r\n\r\n");
        request.append(payload);
        const auto count = adapter.write_some(boost::asio::buffer(request), ec);
        failed = failed || static_cast<bool>(ec) || count != request.size();
        std::array<char, 2048> response{};
        bool received = false;
        adapter.async_read_some(
            boost::asio::buffer(response),
            boost::asio::bind_allocator(operation_allocator, [&](auto error, auto size) {
                received = !error && std::string_view(response.data(), size).find(payload) !=
                                         std::string_view::npos;
                failed = failed || static_cast<bool>(error);
                if (!received) {
                    std::printf("HTTP byte read: error=%d category=%s bytes=%zu\n", error.value(),
                                error.category().name(), size);
                }
            }));
        io.run();  // lint-run: controlled peer and enclosing driver deadline
        failed = failed || !received;
        std::printf("allocation after HTTP byte adapter: outside=%zu\n", escaped);
    }
    io.restart();
    {
        using allocator = std::pmr::polymorphic_allocator<char>;
        using body = beast::http::basic_string_body<char, std::char_traits<char>, allocator>;
        using fields = beast::http::basic_fields<allocator>;
        const auto storage = allocator(&stream_memory);
        beast::http::request<body, fields> request(
            std::piecewise_construct, std::make_tuple(storage), std::make_tuple(storage));
        beast::http::response<body, fields> response(
            std::piecewise_construct, std::make_tuple(storage), std::make_tuple(storage));
        boost::beast::basic_flat_buffer<allocator> buffer(storage);
        request.method(beast::http::verb::post);
        request.target("/echo");
        request.set(beast::http::field::host, authority);
        request.body().assign(128, 'x');
        request.set("X-Allocator-Field", request.body());
        request.prepare_payload();
        beast::tcp_stream transport(std::allocator_arg, stream_allocator, io);
        transport.connect(addresses, ec);
        beast::http::write(transport, request, ec);
        failed = failed || static_cast<bool>(ec);
        bool received = false;
        beast::http::async_read(
            transport, buffer, response,
            boost::asio::bind_allocator(operation_allocator, [&](auto error, auto size) {
                received = !error && size == 128 && response.body() == request.body();
                if (!received) {
                    std::printf("direct HTTP read: error=%d category=%s bytes=%zu\n", error.value(),
                                error.category().name(), size);
                }
            }));
        io.run();  // lint-run: controlled peer and enclosing driver deadline
        failed = failed || !received;
        std::printf("allocation after direct HTTP: outside=%zu\n", escaped);
    }
    return !failed;
}
}  // namespace

// NOLINTBEGIN(readability-inconsistent-declaration-parameter-name): standard replacement signatures
void* operator new(std::size_t size) {
    return allocate(size, alignof(std::max_align_t));
}

void* operator new[](std::size_t size) {
    return allocate(size, alignof(std::max_align_t));
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t /*unused*/) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t /*unused*/) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::align_val_t /*unused*/) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::align_val_t /*unused*/) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t /*unused*/, std::align_val_t /*unused*/) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t /*unused*/,
                       std::align_val_t /*unused*/) noexcept {
    std::free(pointer);
}

// NOLINTEND(readability-inconsistent-declaration-parameter-name)

int main() {
    namespace net = webcpp::asio;
    namespace beast = webcpp::beast;
    resource context_memory;
    resource stream_memory;
    resource operation_memory;
    const auto context_allocator = std::pmr::polymorphic_allocator<std::byte>(&context_memory);
    const auto stream_allocator = std::pmr::polymorphic_allocator<std::byte>(&stream_memory);
    const auto operation_allocator = std::pmr::polymorphic_allocator<std::byte>(&operation_memory);
    net::io_context io(std::allocator_arg, context_allocator);
    net::ip::tcp::resolver resolver(io);
    boost::system::error_code ec;
    const auto port = (emscripten::val::global("beastPeerPort").isUndefined()
                           ? emscripten::val::global("location")["port"]
                           : emscripten::val::global("beastPeerPort"))
                          .as<std::string>();
    const auto addresses = resolver.resolve("127.0.0.1", port, ec);
    constexpr std::string_view long_host = "allocator-host-with-a-long-name.example.invalid";
    const auto named_addresses = resolver.resolve(long_host, port, ec);
    const net::ssl::host_name_verification verification(long_host);
    net::ssl::context tls(net::ssl::context::tls_client);
    tls.set_verify_mode(net::ssl::verify_peer, ec);
    tls.set_verify_callback(verification, ec);
    const std::string authority = "127.0.0.1:" + port;
    std::array<char, 128> incoming{};
    bool done = false;
    bool failed = false;
    measuring = true;
    failed = !registration_lifetimes(context_memory, stream_memory);
    failed = !http_allocations(io, addresses, authority, stream_memory, operation_memory) || failed;
    io.restart();
    {
        beast::ssl_stream<beast::tcp_stream> metadata(std::allocator_arg, stream_allocator, io,
                                                      tls);
        beast::get_lowest_layer(metadata).connect(named_addresses, ec);
        metadata.set_verify_callback(verification);
        failed = failed || !beast::set_server_name(metadata, long_host);
        metadata.handshake(net::ssl::stream_base::client, ec);
        failed = failed || static_cast<bool>(ec);
    }
    {
        beast::websocket::stream<beast::tcp_stream> socket(std::allocator_arg, stream_allocator,
                                                           io);
        socket.set_option(beast::websocket::stream_base::decorator([](auto& request) {
            request.set(beast::http::field::sec_websocket_protocol,
                        "allocator-subprotocol-with-a-long-name-to-defeat-small-string-storage");
        }));
        beast::get_lowest_layer(socket).connect(addresses, ec);
        const auto on_close = boost::asio::bind_allocator(operation_allocator, [&](auto error) {
            done = !error;
            failed = failed || static_cast<bool>(error);
            if (error) {
                std::printf("WebSocket close: error=%d category=%s\n", error.value(),
                            error.category().name());
            }
        });
        const auto on_read =
            boost::asio::bind_allocator(operation_allocator, [&](auto error, auto size) {
                failed = failed || static_cast<bool>(error) || size != 128;
                if (error || size != 128) {
                    std::printf("WebSocket read: error=%d category=%s bytes=%zu\n", error.value(),
                                error.category().name(), size);
                }
                socket.async_close(
                    beast::websocket::close_reason{beast::websocket::close_code::normal,
                                                   "allocator-close-reason-with-a-long-name"},
                    on_close);
            });
        const auto on_write =
            boost::asio::bind_allocator(operation_allocator, [&](auto error, auto size) {
                failed = failed || static_cast<bool>(error) || size != 128;
                if (error || size != 128) {
                    std::printf("WebSocket write: error=%d category=%s bytes=%zu\n", error.value(),
                                error.category().name(), size);
                }
                socket.async_read_some(boost::asio::buffer(incoming), on_read);
            });
        const auto on_handshake = boost::asio::bind_allocator(operation_allocator, [&](auto error) {
            failed = failed || static_cast<bool>(error);
            if (error) {
                std::printf("WebSocket handshake: error=%d category=%s\n", error.value(),
                            error.category().name());
            }
            socket.async_write(boost::asio::buffer(incoming), on_write);
        });
        socket.async_handshake(
            authority, "/allocator-session-with-a-long-target-to-defeat-small-string-storage",
            on_handshake);
        io.run();  // lint-run: controlled peers and the enclosing driver deadline
    }
    io.restart();
    io.run();  // lint-run: drain cancellation completions before checking retained allocation
    measuring = false;
    std::printf(
        "allocation counts: outside=%zu stream=%zu operation=%zu context=%zu retained=%zu\n",
        escaped, stream_memory.total, operation_memory.total, context_memory.total,
        stream_memory.live);
    if (failed || !done || escaped != 0 || stream_memory.live != 0 || operation_memory.live != 0) {
        return 1;
    }
    std::puts("browser allocator: ok");
}
