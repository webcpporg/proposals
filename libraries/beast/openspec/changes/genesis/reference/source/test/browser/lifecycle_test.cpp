// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/asio.hpp>
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>

#include <emscripten/val.h>
#include <boost/asio/buffer.hpp>
#define BOOST_LIGHTWEIGHT_TEST_OSTREAM std::cout
#include <boost/core/lightweight_test.hpp>
#include <cstdio>
#include <memory_resource>
#include <string>

namespace {
namespace beast = webcpp::beast;
namespace net = webcpp::asio;

struct resource final : std::pmr::memory_resource {
    std::size_t allocations = 0;
    std::size_t live = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        ++allocations;
        live += size;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* pointer, std::size_t size, std::size_t alignment) override {
        live -= size;
        std::pmr::new_delete_resource()->deallocate(pointer, size, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

bool await_observation(const char* name) {
    net::io_context observed;
    bool ready = false;
    net::browser::async_await(
        observed.get_executor(), emscripten::val::global(name),
        [&](auto ec, const auto& value) { ready = !ec && value.template as<bool>(); });
    observed.run();  // lint-run: only the observer runs while the WebSocket executor stays paused
    return BOOST_TEST(ready);
}

void observe_sockets() {
    // Real native WebSocket objects retain their browser implementation and peer traffic.
    // clang-format off
    EM_ASM({
        const Original = globalThis.WebSocket;
        globalThis.beastLifecycleSockets = [];
        globalThis.WebSocket = class extends Original {
            constructor(...args) {
                super(...args);
                globalThis.beastLifecycleSockets.push(this);
            }
        };
    });
// clang-format on
}

void cancel_open_before_drain(const std::string& port) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    beast::websocket::stream<beast::tcp_stream> socket(io);
    beast::get_lowest_layer(socket).connect(resolver.resolve("127.0.0.1", port, ec), ec);
    int completed = 0;
    socket.async_handshake("127.0.0.1:" + port, "/echo", [&](auto error) {
        ec = error;
        ++completed;
    });
    // Observe the real peer opening while the stream's executor remains paused.
    // clang-format off
    EM_ASM({
        const socket = globalThis.beastLifecycleSockets.at(-1);
        globalThis.beastLifecycleOpened = new Promise(resolve => {
            const timer = setTimeout(() => resolve(false), 5000);
            socket.addEventListener('open', () => {
                clearTimeout(timer);
                resolve(true);
            }, {once: true});
        });
    });
    // clang-format on
    if (!await_observation("beastLifecycleOpened")) {
        return;
    }
    BOOST_TEST_EQ(completed, 0);
    beast::get_lowest_layer(socket).cancel();
    io.restart();
    io.poll();  // lint-run: drain the queued opening without yielding to later host close events
    BOOST_TEST_EQ(completed, 1);
    BOOST_TEST(ec == boost::asio::error::operation_aborted);
    BOOST_TEST(!socket.is_open());
    socket.handshake("127.0.0.1:" + port, "/echo", ec);
    if (!BOOST_TEST(!ec)) {
        return;
    }
    socket.close(beast::websocket::close_code::normal, ec);
    BOOST_TEST(!ec);
    std::puts("cancel before opening dispatch: completed");
}

void paused_ingress(const std::string& port, bool payload) {
    resource storage;
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    beast::websocket::stream<beast::tcp_stream> socket(
        std::allocator_arg, std::pmr::polymorphic_allocator<std::byte>(&storage), io);
    const std::size_t budget = payload ? 64 : 256;
    socket.set_option(beast::websocket::queue_limits{.send = 1024, .receive = budget});
    beast::get_lowest_layer(socket).connect(resolver.resolve("127.0.0.1", port, ec), ec);
    bool completed = false;
    socket.async_handshake("127.0.0.1:" + port, payload ? "/retained-flood" : "/empty-flood",
                           [&](auto error) {
                               ec = error;
                               completed = true;
                           });
    // clang-format off
    EM_ASM({
        const socket = globalThis.beastLifecycleSockets.at(-1);
        globalThis.beastLifecycleBurst = new Promise(resolve => {
            let count = 0;
            const timer = setTimeout(() => resolve(false), 5000);
            const done = () => { clearTimeout(timer); resolve(true); };
            socket.addEventListener('message', () => { if (++count === 64) done(); });
            socket.addEventListener('close', done, {once: true});
        });
    });
    // clang-format on
    const auto baseline = storage.allocations;
    const auto baseline_bytes = storage.live;
    if (!await_observation("beastLifecycleBurst")) {
        return;
    }
    BOOST_TEST(!completed);
    // Every accepted item consumes a record even with an empty payload. Only
    // those bounded records and a fixed executor notification may allocate.
    const auto growth = storage.allocations - baseline;
    std::printf("paused ingress: allocation growth=%zu budget=%zu\n", growth, budget);
    BOOST_TEST_LE(growth, (budget / beast::detail::browser::message_record_bytes) + 2);
    BOOST_TEST_LE(storage.live - baseline_bytes, budget + 256);
    const auto host_socket =
        emscripten::val::global("beastLifecycleSockets").call<emscripten::val>("at", -1);
    BOOST_TEST_GE(host_socket["readyState"].as<unsigned>(), 2U);
    io.restart();
    io.run();  // lint-run: native peer has completed the bounded burst and closing handshake
    BOOST_TEST(completed);
    BOOST_TEST(!ec);
    beast::flat_buffer buffer;
    socket.read(buffer, ec);
    BOOST_TEST(ec == beast::browser_errc::receive_limit);
}

void reuse(const std::string& port, bool reconnect) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    const auto endpoints = resolver.resolve("127.0.0.1", port, ec);
    beast::websocket::stream<beast::tcp_stream> socket(io);
    socket.text(true);
    socket.read_message_max(1234);
    int decorated = 0;
    int closed = 0;
    socket.set_option(
        beast::websocket::stream_base::decorator([&](beast::websocket::request_type& request) {
            ++decorated;
            request.set(beast::http::field::sec_websocket_protocol, "lifecycle");
        }));
    socket.control_callback([&](auto, auto) { ++closed; });
    beast::get_lowest_layer(socket).connect(endpoints, ec);
    for (int cycle = 0; cycle != 2; ++cycle) {
        if (cycle != 0 && reconnect) {
            beast::get_lowest_layer(socket).connect(endpoints, ec);
        }
        socket.handshake("127.0.0.1:" + port, "/echo", ec);
        if (!BOOST_TEST(!ec)) {
            return;
        }
        BOOST_TEST_EQ(socket.read_message_max(), 1234U);
        BOOST_TEST_EQ(emscripten::val::global("beastLifecycleSockets")
                          .call<emscripten::val>("at", -1)["protocol"]
                          .as<std::string>(),
                      "lifecycle");
        socket.write(boost::asio::buffer("round trip", 10), ec);
        BOOST_TEST(!ec);
        beast::flat_buffer buffer;
        socket.read(buffer, ec);
        BOOST_TEST(!ec);
        BOOST_TEST(socket.got_text());
        BOOST_TEST_EQ(beast::buffers_to_string(buffer.data()), "round trip");
        socket.close(beast::websocket::close_code::normal, ec);
        BOOST_TEST(!ec);
        io.restart();
        io.poll();  // lint-run: retire old cancellation notifications before reusing the same
                    // stream
    }
    BOOST_TEST_EQ(decorated, 2);
    BOOST_TEST_EQ(closed, 2);
    std::printf("reuse reconnect=%d: completed\n", static_cast<int>(reconnect));
}

void stale_generation(const std::string& port) {
    net::io_context io;
    boost::system::error_code ec;
    net::ip::tcp::resolver resolver(io);
    const auto endpoints = resolver.resolve("127.0.0.1", port, ec);
    beast::websocket::stream<beast::tcp_stream> socket(io);
    beast::get_lowest_layer(socket).connect(endpoints, ec);
    socket.handshake("127.0.0.1:" + port, "/echo", ec);
    if (!BOOST_TEST(!ec)) {
        return;
    }
    // The peer echo is queued in the old connection while only the observer runs.
    // Keep the old JS callback too, to exercise a late detached registration key.
    // clang-format off
    EM_ASM({
        const socket = globalThis.beastLifecycleSockets.at(-1);
        globalThis.beastLifecycleLate = socket.onmessage;
        globalThis.beastLifecycleOldMessage = new Promise(resolve => {
            const timer = setTimeout(() => resolve(false), 5000);
            socket.addEventListener('message', () => {
                clearTimeout(timer);
                resolve(true);
            }, {once: true});
        });
    });
    // clang-format on
    socket.write(boost::asio::buffer("old connection", 14), ec);
    if (!BOOST_TEST(!ec) || !await_observation("beastLifecycleOldMessage")) {
        return;
    }
    beast::get_lowest_layer(socket).close();
    beast::get_lowest_layer(socket).connect(endpoints, ec);
    socket.handshake("127.0.0.1:" + port, "/echo", ec);
    if (!BOOST_TEST(!ec)) {
        return;
    }
    EM_ASM({ globalThis.beastLifecycleLate(new MessageEvent('message', {data : 'late'})); });
    socket.write(boost::asio::buffer("new connection", 14), ec);
    BOOST_TEST(!ec);
    beast::flat_buffer buffer;
    socket.read(buffer, ec);
    BOOST_TEST(!ec);
    BOOST_TEST_EQ(beast::buffers_to_string(buffer.data()), "new connection");
    // Reconnection must also restore lowest-layer cancellation and close ownership.
    bool cancelled = false;
    buffer.consume(buffer.size());
    socket.async_read(buffer, [&](auto error, auto size) {
        cancelled = error == boost::asio::error::operation_aborted && size == 0;
    });
    beast::get_lowest_layer(socket).close();
    io.restart();
    io.run();  // lint-run: drain the posted cancellation and stale connection notifications
    BOOST_TEST(cancelled);
}

}  // namespace

int main() {
    observe_sockets();
    const auto port = emscripten::val::global("beastPeerPort").as<std::string>();
    cancel_open_before_drain(port);
    paused_ingress(port, false);
    paused_ingress(port, true);
    reuse(port, false);
    reuse(port, true);
    stale_generation(port);
    if (boost::report_errors() != 0) {
        return 1;
    }
    std::puts("browser lifecycle: ok");
}
