// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/beast/_experimental/test/stream.hpp>
#include <webcpp/asio.hpp>
#include <webcpp/asio/browser/async_await.hpp>
#include <webcpp/beast/browser/http_byte_stream.hpp>
#include <webcpp/beast/core.hpp>
#include <webcpp/beast/http.hpp>
#include <webcpp/beast/http/read.hpp>
#include <webcpp/beast/http/write.hpp>
#include <webcpp/beast/tcp_stream.hpp>
#include <webcpp/beast/websocket.hpp>
#include <webcpp/beast/websocket/guest.hpp>

#include <emscripten/val.h>
#include <array>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cstdio>
#include <memory_resource>
#include <string>
#include <webcpp/beast/ssl.hpp>

namespace {
namespace beast = webcpp::beast;
namespace net = webcpp::asio;

int fail(int line) {
    std::printf("check failed at line %d\n", line);
    return 1;
}

bool check(boost::system::error_code ec) {
    if (ec) {
        std::printf("error: %s\n", ec.message().c_str());
    }
    return !ec;
}

struct counting_resource final : std::pmr::memory_resource {
    std::size_t live = 0;
    std::size_t total = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        live += size;
        ++total;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* value, std::size_t size, std::size_t alignment) override {
        live -= size;
        std::pmr::new_delete_resource()->deallocate(value, size, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct alignas(64) aligned_decorator {
    void operator()(beast::websocket::request_type& request) const {
        request.set(boost::beast::http::field::sec_websocket_protocol, "chat");
    }
};

struct fixture {
    net::io_context io;
    net::ip::tcp::resolver resolver{io};
    boost::system::error_code ec;
    const std::string port = (emscripten::val::global("beastPeerPort").isUndefined()
                                  ? emscripten::val::global("location")["port"]
                                  : emscripten::val::global("beastPeerPort"))
                                 .as<std::string>();
    const net::ip::tcp::resolver::results_type entries = resolver.resolve("127.0.0.1", port, ec);
    const std::string host = "127.0.0.1:" + port;

    bool connect(auto& socket) {
        beast::get_lowest_layer(socket).connect(entries, ec);
        if (!check(ec)) {
            return false;
        }
        socket.handshake(host, "/echo", ec);
        return check(ec);
    }

    int async_clients() {
        beast::websocket::stream<beast::tcp_stream> ws(io);
        beast::flat_buffer buffer;
        bool done = false;
        bool failed = false;
        const auto check = [&](boost::system::error_code ec) {
            if (ec) {
                std::printf("error: %s\n", ec.message().c_str());
                failed = true;
            }
            return !ec;
        };
        const auto on_read = [&](auto error, auto count) {
            if (!check(error) || count != 5 || !ws.got_text() ||
                beast::buffers_to_string(buffer.data()) != "hello") {
                return;
            }
            ws.async_close(beast::websocket::close_code::normal,
                           [&](auto close_error) { done = check(close_error); });
        };
        const auto on_write = [&](auto error, auto count) {
            if (check(error) && count == 5) {
                ws.async_read(buffer, on_read);
            }
        };
        const auto on_handshake = [&](auto error) {
            if (check(error)) {
                ws.text(true);
                ws.async_write(boost::asio::buffer("hello", 5), on_write);
            }
        };
        resolver.async_resolve("127.0.0.1", port, [&](auto error, const auto& resolved) {
            if (!check(error)) {
                return;
            }
            beast::get_lowest_layer(ws).async_connect(resolved, [&](auto connected, const auto&) {
                if (check(connected)) {
                    ws.async_handshake("127.0.0.1:" + port, "/echo", on_handshake);
                }
            });
        });
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!done || failed) {
            return fail(__LINE__);
        }
        beast::tcp_stream http(io);
        http.connect(resolver.resolve("127.0.0.1", port, ec), ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::http::request<beast::http::string_body> request{beast::http::verb::post, "/http",
                                                               11};
        request.set(beast::http::field::host, "127.0.0.1:" + port);
        request.body() = "payload";
        request.prepare_payload();
        beast::http::response<beast::http::string_body> response;
        bool fetched = false;
        beast::http::async_write(http, request, [&](auto write_error, auto) {
            if (!check(write_error)) {
                return;
            }
            beast::http::async_read(http, buffer, response, [&](auto read_error, auto) {
                fetched = check(read_error) && response.result_int() == 200 &&
                          response.body() == "payload";
            });
        });
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!fetched) {
            return fail(__LINE__);
        }

        return 0;
    }

    int binary_fragments_and_cancel() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        socket.binary(true);
        const std::array<unsigned char, 3> data{1, 2, 3};
        socket.write(boost::asio::buffer(data), ec);
        beast::flat_buffer result;
        socket.read(result, ec);
        if (!check(ec) || !socket.got_binary() || result.size() != 3 ||
            beast::buffers_to_string(result.data()) != std::string("\1\2\3", 3)) {
            return fail(__LINE__);
        }
        socket.text(true);
        socket.write_some(false, boost::asio::buffer("ab", 2), ec);
        socket.write_some(true, boost::asio::buffer("cd", 2), ec);
        result.consume(result.size());
        std::size_t piece = 0;
        socket.async_read_some(result, 3, [&](auto error, auto count) {
            ec = error;
            piece = count;
        });
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!check(ec) || piece != 3 || socket.is_message_done()) {
            return fail(__LINE__);
        }
        socket.async_read_some(result, 3, [&](auto error, auto count) {
            ec = error;
            piece = count;
        });
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!check(ec) || piece != 1 || !socket.is_message_done() ||
            beast::buffers_to_string(result.data()) != "abcd") {
            return fail(__LINE__);
        }
        bool cancelled = false;
        bool inline_call = true;
        boost::asio::cancellation_signal signal;
        result.consume(result.size());
        socket.async_read(
            result, boost::asio::bind_cancellation_slot(signal.slot(), [&](auto error, auto count) {
                cancelled =
                    error == boost::asio::error::operation_aborted && count == 0 && !inline_call;
            }));
        signal.emit(boost::asio::cancellation_type::total);
        inline_call = false;
        socket.write(boost::asio::buffer("after cancel", 12), ec);
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        socket.read(result, ec);
        if (!cancelled || !check(ec) || beast::buffers_to_string(result.data()) != "after cancel") {
            return fail(__LINE__);
        }
        socket.close(beast::websocket::close_code::going_away, ec);
#ifdef WEBCPP_BEAST_UNSUPPORTED_ERROR
        if (ec != boost::asio::error::operation_not_supported || !socket.is_open()) {
            return fail(__LINE__);
        }
        socket.close(beast::websocket::close_code::normal, ec);
#endif
        if (!check(ec)) {
            return fail(__LINE__);
        }

        return 0;
    }

    int cancelled_read_deadline() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        for (int overload = 0; overload != 2; ++overload) {
            beast::get_lowest_layer(socket).expires_after(std::chrono::hours(1));
            beast::flat_buffer dynamic;
            std::array<char, 32> fixed{};
            boost::asio::cancellation_signal signal;
            bool cancelled = false;
            auto completion =
                boost::asio::bind_cancellation_slot(signal.slot(), [&](auto error, auto count) {
                    cancelled = error == boost::asio::error::operation_aborted && count == 0;
                });
            if (overload == 0) {
                socket.async_read(dynamic, std::move(completion));
            } else {
                socket.async_read_some(boost::asio::buffer(fixed), std::move(completion));
            }
            signal.emit(boost::asio::cancellation_type::total);
            io.restart();
            io.poll();
            if (!cancelled || !io.stopped() || !socket.is_open()) {
                return fail(__LINE__);
            }
            beast::get_lowest_layer(socket).expires_never();
            socket.write(boost::asio::buffer("after cancel", 12), ec);
            socket.read(dynamic, ec);
            if (!check(ec) || beast::buffers_to_string(dynamic.data()) != "after cancel") {
                return fail(__LINE__);
            }
        }
        socket.close(beast::websocket::close_code::normal, ec);
        return check(ec) ? 0 : fail(__LINE__);
    }

    int message_limit() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        socket.read_message_max(2);
        socket.write(boost::asio::buffer("long", 4), ec);
        beast::flat_buffer result;
        socket.read(result, ec);
        if (ec != beast::websocket::error::message_too_big || socket.is_open()) {
            return fail(__LINE__);
        }

        return 0;
    }

    int send_limit() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        socket.set_option(beast::websocket::queue_limits{.send = 2, .receive = 1024});
        socket.write(boost::asio::buffer("long", 4), ec);
        if (ec != beast::browser_errc::send_limit || socket.is_open()) {
            return fail(__LINE__);
        }

        return 0;
    }

    int receive_limit() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        socket.set_option(beast::websocket::queue_limits{.send = 1024, .receive = 2});
        socket.write(boost::asio::buffer("long", 4), ec);
        beast::flat_buffer result;
        socket.read(result, ec);
        if (ec != beast::browser_errc::receive_limit || socket.is_open()) {
            return fail(__LINE__);
        }

        return 0;
    }

    int retained_receive_limit() {
        for (const bool partial : {false, true}) {
            beast::websocket::stream<beast::tcp_stream> socket(io);
            socket.set_option(beast::websocket::queue_limits{.send = 1024, .receive = 256});
            boost::asio::steady_timer guard(io, std::chrono::seconds(5));
            bool closed = false;
            socket.control_callback([&](beast::websocket::frame_type kind, auto) {
                if (kind == beast::websocket::frame_type::close) {
                    closed = true;
                    guard.cancel();
                }
            });
            guard.async_wait([](auto) {});
            beast::get_lowest_layer(socket).connect(entries, ec);
            socket.handshake(host, partial ? "/retained-flood" : "/empty-flood", ec);
            if (!check(ec)) {
                return fail(__LINE__);
            }
            beast::flat_buffer result;
            if (partial) {
                std::size_t received = 0;
                socket.async_read_some(result, 1, [&](auto error, auto count) {
                    ec = error;
                    received = count;
                    socket.write(boost::asio::buffer("next", 4), ec);
                });
                io.restart();
                io.run();  // lint-run: bounded by the five-second local-peer guard
                if (!check(ec) || received != 1) {
                    return fail(__LINE__);
                }
            } else {
                io.restart();
                io.run();  // lint-run: bounded by the five-second local-peer guard
            }
            socket.read(result, ec);
            if (!closed || ec != beast::browser_errc::receive_limit || socket.is_open()) {
                std::printf("queue partial=%d closed=%d open=%d error=%s bytes=%zu\n",
                            static_cast<int>(partial), static_cast<int>(closed),
                            static_cast<int>(socket.is_open()), ec.message().c_str(),
                            result.size());
                return fail(__LINE__);
            }
        }
        return 0;
    }

    int deadline() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        if (!connect(socket)) {
            return fail(__LINE__);
        }
        beast::get_lowest_layer(socket).expires_after(std::chrono::milliseconds(10));
        beast::flat_buffer result;
        socket.read(result, ec);
        if (ec != beast::error::timeout || socket.is_open()) {
            return fail(__LINE__);
        }

        return 0;
    }

    int destruction() {
        beast::flat_buffer result;
        bool cancelled = false;
        {
            beast::websocket::stream<beast::tcp_stream> socket(io);
            if (!connect(socket)) {
                return fail(__LINE__);
            }
            socket.async_read(result, [&](auto error, auto count) {
                cancelled = error == boost::asio::error::operation_aborted && count == 0;
            });
        }
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!cancelled) {
            return fail(__LINE__);
        }

        return 0;
    }

    int tls_policy() {
        net::ssl::context tls(net::ssl::context::tls_client);
        tls.set_verify_mode(net::ssl::verify_peer, ec);
        beast::ssl_stream<beast::tcp_stream> stream(io, tls);
        beast::get_lowest_layer(stream).connect(entries, ec);
        if (!beast::set_server_name(stream, "127.0.0.1")) {
            return fail(__LINE__);
        }
        stream.handshake(net::ssl::stream_base::client, ec);
        if (!check(ec) || !beast::get_lowest_layer(stream).secure()) {
            return fail(__LINE__);
        }
        if (!beast::set_server_name(stream, "other.invalid")) {
            return fail(__LINE__);
        }
        stream.handshake(net::ssl::stream_base::client, ec);
#ifdef WEBCPP_BEAST_UNSUPPORTED_ERROR
        if (ec != boost::asio::error::operation_not_supported) {
            return fail(__LINE__);
        }
#else
        if (!check(ec)) {
            return fail(__LINE__);
        }
#endif

        return 0;
    }

    int native_tls() {
        const auto secure_port = emscripten::val::global("beastSecurePort").as<std::string>();
        const auto secure_entries = resolver.resolve("127.0.0.1", secure_port, ec);
        const auto authority = "127.0.0.1:" + secure_port;
        net::ssl::context tls(net::ssl::context::tls_client);
        tls.set_verify_mode(net::ssl::verify_peer, ec);
        beast::tcp_stream record(io);
        record.connect(secure_entries, ec);
        beast::ssl_stream<beast::tcp_stream> secure(std::move(record), tls);
        secure.handshake(net::ssl::stream_base::client, ec);
        beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> socket(std::move(secure));
        socket.handshake(authority, "/echo", ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        socket.write(boost::asio::buffer("secure", 6), ec);
        beast::flat_buffer message;
        socket.read(message, ec);
        if (!check(ec) || beast::buffers_to_string(message.data()) != "secure") {
            return fail(__LINE__);
        }
        socket.close(beast::websocket::close_code::normal, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::ssl_stream<beast::tcp_stream> transport(io, tls);
        beast::get_lowest_layer(transport).connect(secure_entries, ec);
        transport.handshake(net::ssl::stream_base::client, ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
        request.set(beast::http::field::host, authority);
        beast::http::write(transport, request, ec);
        beast::http::response<beast::http::string_body> response;
        beast::http::read(transport, message, response, ec);
        if (!check(ec) || response.body() != "fetch body") {
            return fail(__LINE__);
        }
        return 0;
    }

    int host_errors() {
        beast::websocket::stream<beast::tcp_stream> socket(io);
        socket.handshake(host, "/echo", ec);
        if (ec != beast::browser_errc::no_host) {
            return fail(__LINE__);
        }
        beast::get_lowest_layer(socket).connect(entries, ec);
        socket.handshake(host, "/echo#fragment", ec);
        if (ec != beast::browser_errc::invalid_url) {
            return fail(__LINE__);
        }
        socket.handshake(host, "/refuse", ec);
        if (ec != beast::browser_errc::connection_failed) {
            return fail(__LINE__);
        }
        beast::websocket::stream<beast::tcp_stream> closing(io);
        beast::get_lowest_layer(closing).connect(entries, ec);
        closing.handshake(host, "/close", ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer message;
        closing.read(message, ec);
        if (ec != beast::websocket::error::closed || closing.reason().code != 4001) {
            return fail(__LINE__);
        }
        beast::tcp_stream transport(io);
        transport.connect(resolver.resolve("127.0.0.1", "1", ec), ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
        beast::http::write(transport, request, ec);
        beast::http::response<beast::http::string_body> response;
        beast::http::read(transport, message, response, ec);
        return ec == beast::browser_errc::fetch_failed ? 0 : fail(__LINE__);
    }

    int queued_close_deadline() {
        namespace detail = beast::detail::browser;
        const auto state =
            detail::make_websocket_state(io.get_executor(), std::allocator<std::byte>());
        state->opened = true;
        state->timeout.idle_timeout = boost::beast::websocket::stream_base::none();
        state->key = state->bridge->add(*state);
        bool timed_out = false;
        boost::asio::steady_timer rescue(io);
        detail::start<false>(
            state->close, io.get_executor(),
            [&](auto error) {
                timed_out = error == beast::error::timeout;
                rescue.cancel();
            },
            [](auto&, auto&) {}, boost::asio::cancellation_type::none, [] {});
        state->arm(detail::clock_type::now() + std::chrono::milliseconds(20));
        // The bridge has already queued this message when close becomes pending.
        auto event = emscripten::val::object();
        event.set("type", "message");
        event.set("data", "queued before close");
        state->deliver(event);
        rescue.expires_after(std::chrono::milliseconds(200));
        rescue.async_wait([state](auto error) {
            if (!error) {
                state->finish(boost::asio::error::operation_aborted);
            }
        });
        io.restart();
        io.run();  // lint-run: both the close deadline and rescue deadline are bounded
        return timed_out ? 0 : fail(__LINE__);
    }

    int string_body_get() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        beast::browser::http_byte_stream adapter(transport);
        beast::http::request<beast::http::string_body> request;
        request.method(beast::http::verb::get);
        request.target("/http");
        request.set(beast::http::field::host, host);
        boost::beast::http::write(adapter, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer wire;
        beast::http::response<beast::http::dynamic_body> response;
        boost::beast::http::read(adapter, wire, response, ec);
        if (!check(ec) || beast::buffers_to_string(response.body().data()) != "fetch body") {
            return fail(__LINE__);
        }

        return 0;
    }

    int head_headers() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        beast::browser::http_byte_stream adapter(transport);
        beast::http::request<beast::http::string_body> request;
        request.method(beast::http::verb::head);
        request.target("/http");
        request.set(beast::http::field::host, host);
        beast::http::write(adapter, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer wire;
        beast::http::response_parser<beast::http::dynamic_body> response;
        beast::http::read_header(adapter, wire, response, ec);
        if (!check(ec) || !response.is_header_done() || wire.size() != 0) {
            return fail(__LINE__);
        }

        return 0;
    }

    int early_fetch_headers() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/headers",
                                                              11};
        beast::http::write(transport, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer unused;
        beast::http::response<beast::http::string_body> response;
        bool headers = false;
        beast::http::async_read_header(transport, unused, response, [&](auto error, auto count) {
            headers =
                !error && count != 0 && response.result_int() == 200 && response.body().empty();
            EM_ASM({ fetch('http://127.0.0.1:' + globalThis.beastPeerPort + '/release'); });
        });
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        beast::http::read(transport, unused, response, ec);
        if (!headers || !check(ec) || response.body() != "released body") {
            return fail(__LINE__);
        }

        return 0;
    }

    int fetch_cancellation() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        transport.expires_after(std::chrono::hours(1));
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
        beast::http::write(transport, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer buffer;
        beast::http::response<beast::http::string_body> response;
        boost::asio::cancellation_signal signal;
        bool cancelled = false;
        beast::http::async_read(
            transport, buffer, response,
            boost::asio::bind_cancellation_slot(signal.slot(), [&](auto error, auto count) {
                cancelled = error == boost::asio::error::operation_aborted && count == 0;
            }));
        signal.emit(boost::asio::cancellation_type::total);
        io.restart();
        io.poll();
        // No future deadline should retain work after the fetch was cancelled.
        if (!cancelled || !io.stopped()) {
            return fail(__LINE__);
        }
        return 0;
    }

    int fetch_cancelled_generation() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        // Observe completion of the real response stream without running this io_context.
        // The next macrotask follows the import's final queued body delivery.
        // clang-format off
        EM_ASM({
            const original = globalThis.fetch;
            globalThis.beastFetchFinished = new Promise(resolve => {
                const timer = setTimeout(() => resolve(false), 5000);
                globalThis.fetch = (...args) => {
                    globalThis.fetch = original;
                    return original(...args).then(response => {
                        const getReader = response.body.getReader.bind(response.body);
                        response.body.getReader = () => {
                            const reader = getReader();
                            const read = reader.read.bind(reader);
                            reader.read = () => read().then(chunk => {
                                if (chunk.done) {
                                    clearTimeout(timer);
                                    setTimeout(() => resolve(true), 0);
                                }
                                return chunk;
                            });
                            return reader;
                        };
                        return response;
                    });
                };
            });
        });
        // clang-format on
        beast::http::request<beast::http::string_body> request{beast::http::verb::post, "/http",
                                                               11};
        request.body() = "old exchange";
        request.prepare_payload();
        beast::http::write(transport, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::flat_buffer buffer;
        beast::http::response<beast::http::string_body> old_response;
        boost::asio::cancellation_signal signal;
        bool cancelled = false;
        beast::http::async_read(
            transport, buffer, old_response,
            boost::asio::bind_cancellation_slot(signal.slot(), [&](auto error, auto count) {
                cancelled = error == boost::asio::error::operation_aborted && count == 0;
            }));
        net::io_context observed;
        bool queued = false;
        net::browser::async_await(
            observed.get_executor(), emscripten::val::global("beastFetchFinished"),
            [&](auto error, const emscripten::val& value) { queued = !error && value.as<bool>(); });
        observed.run();  // lint-run: bounded by the five-second response observation
        if (!queued) {
            return fail(__LINE__);
        }
        signal.emit(boost::asio::cancellation_type::total);
        request.body() = "new exchange";
        request.prepare_payload();
        beast::http::write(transport, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::http::response<beast::http::string_body> response;
        beast::http::read(transport, buffer, response, ec);
        if (!cancelled || !check(ec) || response.body() != "new exchange" ||
            !old_response.body().empty()) {
            std::printf("cancelled fetch returned: %s\n", response.body().c_str());
            return fail(__LINE__);
        }
        return 0;
    }

    int reconnect() {
        beast::tcp_stream transport(io);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
        beast::flat_buffer buffer;
        beast::http::response<beast::http::string_body> response;
        for (int attempt = 0; attempt != 2; ++attempt) {
            transport.connect(entries, ec);
            beast::http::write(transport, request, ec);
            if (!check(ec)) {
                return fail(__LINE__);
            }
            beast::http::read(transport, buffer, response, ec);
            if (!check(ec) || response.body() != "fetch body") {
                return fail(__LINE__);
            }
            transport.close();
        }

        return 0;
    }

    int head_then_get() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        beast::browser::http_byte_stream adapter(transport);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::head, "/http", 11};
        request.set(beast::http::field::host, host);
        boost::beast::http::write(adapter, request, ec);
        if (!check(ec)) {
            return fail(__LINE__);
        }
        beast::http::response_parser<beast::http::string_body> parser;
        parser.skip(true);
        beast::flat_buffer wire;
        boost::beast::http::read(adapter, wire, parser, ec);
        if (!check(ec) || !parser.is_done() || wire.size() != 0) {
            return fail(__LINE__);
        }
        request.method(beast::http::verb::get);
        boost::beast::http::write(adapter, request, ec);
        beast::http::response_parser<beast::http::string_body> next;
        next.body_limit(2);
        boost::beast::http::read(adapter, wire, next, ec);
        if (ec != beast::http::error::body_limit) {
            return fail(__LINE__);
        }

        return 0;
    }

    int fetch_policy() {
        beast::tcp_stream stream(io);
        stream.connect(entries, ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/http", 11};
        request.set(beast::http::field::host, host);
        request.set(beast::http::field::user_agent, "beast test");
        beast::http::write(stream, request, ec);
#ifdef WEBCPP_BEAST_UNSUPPORTED_ERROR
        if (ec != boost::asio::error::operation_not_supported) {
            return fail(__LINE__);
        }
        request.erase(beast::http::field::user_agent);
        beast::http::write(stream, request, ec);
#endif
        beast::http::response<beast::http::string_body> result;
        beast::flat_buffer buffer;
        beast::http::read(stream, buffer, result, ec);
        if (!check(ec) || result.body() != "fetch body") {
            return fail(__LINE__);
        }
        request.target("/redirect");
        beast::http::write(stream, request, ec);
        beast::http::read(stream, buffer, result, ec);
#ifdef WEBCPP_BEAST_UNSUPPORTED_ERROR
        if (ec != boost::asio::error::operation_not_supported) {
            return fail(__LINE__);
        }
#else
        if (!check(ec) || result.body() != "fetch body") {
            return fail(__LINE__);
        }
#endif
        request.target("/http");
        stream.response_body_limit(2);
        beast::http::write(stream, request, ec);
        beast::http::read(stream, buffer, result, ec);
        if (ec != beast::http::error::body_limit) {
            return fail(__LINE__);
        }
        stream.close(ec);
        beast::http::write(stream, request, ec);
        if (ec != boost::asio::error::not_connected) {
            return fail(__LINE__);
        }

        return 0;
    }

    int fetch_forbidden_headers() {
        struct header_case {
            const char* name;
            const char* value;
            bool forbidden;
        };

        const std::array cases{
            header_case{
                .name = "Set-Cookie",
                .value = "test=value",
                .forbidden = true,
            },
            header_case{
                .name = "x-hTtP-mEthOd",
                .value = "CONNECT",
                .forbidden = true,
            },
            header_case{
                .name = "X-HTTP-Method-Override",
                .value = "GET,\tTrAcE",
                .forbidden = true,
            },
            header_case{
                .name = "X-Method-Override",
                .value = "PATCH, TRACK",
                .forbidden = true,
            },
            header_case{
                .name = "X-Method-Override",
                .value = R"("x, TRACK", CONNECT)",
                .forbidden = true,
            },
            header_case{
                .name = "X-Method-Override",
                .value = R"(GET, "TRACE")",
                .forbidden = false,
            },
            header_case{
                .name = "X-HTTP-Method",
                .value = "TRACKER",
                .forbidden = false,
            },
            header_case{
                .name = "X-HTTP-Method-Override",
                .value = "GET, PATCH",
                .forbidden = false,
            },
            header_case{
                .name = "X-Method-Override",
                .value = R"("escaped\", TRACK", POST)",
                .forbidden = false,
            },
        };
        for (const auto& test : cases) {
            beast::tcp_stream transport(io);
            transport.connect(entries, ec);
            beast::http::request<beast::http::empty_body> request{beast::http::verb::get,
                                                                  "/request-headers", 11};
            request.set(test.name, test.value);
            // Observe the host-call boundary as well as what reaches the real HTTP peer.
            // clang-format off
            EM_ASM({
                globalThis.beastPolicyOriginalFetch = globalThis.fetch;
                globalThis.beastPolicyHeaders = undefined;
                globalThis.fetch = (url, init) => {
                    globalThis.fetch = globalThis.beastPolicyOriginalFetch;
                    globalThis.beastPolicyHeaders = init.headers;
                    return globalThis.fetch(url, init);
                };
            });
            // clang-format on
            beast::http::write(transport, request, ec);
            EM_ASM({ globalThis.fetch = globalThis.beastPolicyOriginalFetch; });
            const auto headers = emscripten::val::global("beastPolicyHeaders");
#ifdef WEBCPP_BEAST_UNSUPPORTED_ERROR
            if (test.forbidden) {
                if (ec != boost::asio::error::operation_not_supported || !headers.isUndefined()) {
                    return fail(__LINE__);
                }
                continue;
            }
#endif
            if (!check(ec) || headers.isUndefined() ||
                headers["length"].as<unsigned>() != (test.forbidden ? 0u : 1u)) {
                return fail(__LINE__);
            }
            beast::flat_buffer buffer;
            beast::http::response<beast::http::string_body> response;
            beast::http::read(transport, buffer, response, ec);
            if (!check(ec) ||
                (test.forbidden ? !response.body().empty()
                                : response.body().find(test.value) == std::string::npos)) {
                return fail(__LINE__);
            }
        }
        return 0;
    }

    int fetch_reused_reason() {
        beast::tcp_stream transport(io);
        transport.connect(entries, ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get,
                                                              "/custom-reason", 11};
        beast::flat_buffer buffer;
        beast::http::response<beast::http::string_body> response;
        beast::http::write(transport, request, ec);
        beast::http::read(transport, buffer, response, ec);
        if (!check(ec) || response.reason() != "Fixture custom") {
            return fail(__LINE__);
        }
        request.target("/empty-reason");
        beast::http::write(transport, request, ec);
        beast::http::read(transport, buffer, response, ec);
        if (!check(ec) || response.reason() != "OK" || response.body() != "empty reason") {
            std::printf("reused response reason: %s\n", std::string(response.reason()).c_str());
            return fail(__LINE__);
        }
        return 0;
    }

    int cors_failure() {
        if (!emscripten::val::global("beastNodeRuntime").isUndefined()) {
            return 0;
        }
        beast::tcp_stream stream(io);
        const auto cross_port = emscripten::val::global("beastCorsPort").as<std::string>();
        stream.connect(resolver.resolve("127.0.0.1", cross_port, ec), ec);
        beast::http::request<beast::http::empty_body> request{beast::http::verb::get, "/", 11};
        request.set(beast::http::field::host, "127.0.0.1:" + cross_port);
        beast::http::write(stream, request, ec);
        beast::http::response<beast::http::string_body> result;
        beast::flat_buffer buffer;
        beast::http::read(stream, buffer, result, ec);
        if (ec != beast::browser_errc::fetch_failed) {
            return fail(__LINE__);
        }

        return 0;
    }

    int guest_protocol() {
        beast::flat_buffer buffer;
        using memory_stream = boost::beast::test::stream;
        static_assert(std::is_base_of_v<boost::beast::websocket::stream<memory_stream>,
                                        beast::websocket::stream<memory_stream>>);
        beast::websocket::stream<memory_stream> client(io);
        bool decorated = false;
        client.set_option(
            beast::websocket::stream_base::decorator([&](beast::websocket::request_type& request) {
                request.set(beast::http::field::user_agent, "guest typed decorator");
                decorated = true;
            }));
        client.control_callback([](beast::websocket::frame_type, beast::string_view) {});
        beast::websocket::stream<memory_stream> server(io);
        server.set_option(
            beast::websocket::stream_base::decorator([](beast::websocket::response_type& response) {
                response.set(beast::http::field::server, "guest server decorator");
            }));
        client.next_layer().connect(server.next_layer());
        beast::flat_buffer incoming;
        bool echoed = false;
        server.async_accept([&](auto accept_error) {
            if (!check(accept_error)) {
                return;
            }
            server.async_read(incoming, [&](auto read_error, auto) {
                if (!check(read_error)) {
                    return;
                }
                server.async_write(incoming.data(),
                                   [&](auto write_error, auto) { check(write_error); });
            });
        });
        beast::websocket::response_type response;
        client.async_handshake(response, "memory", "/", [&](auto handshake_error) {
            if (!check(handshake_error)) {
                return;
            }
            client.async_write(boost::asio::buffer("guest", 5), [&](auto write_error, auto) {
                if (!check(write_error)) {
                    return;
                }
                client.async_read(buffer, [&](auto read_error, auto count) {
                    echoed = check(read_error) && count == 5;
                });
            });
        });
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (!echoed || !decorated ||
            response[beast::http::field::server] != "guest server decorator") {
            return fail(__LINE__);
        }

        return 0;
    }

    int allocator_lifetime() {
        counting_resource resource;
        {
            beast::websocket::stream<beast::tcp_stream> socket(
                std::allocator_arg, std::pmr::polymorphic_allocator<std::byte>(&resource), io);
            socket.set_option(beast::websocket::stream_base::decorator(aligned_decorator{}));
            if (!connect(socket)) {
                return fail(__LINE__);
            }
            socket.close(beast::websocket::close_code::normal, ec);
            if (!check(ec) || resource.total == 0) {
                return fail(__LINE__);
            }
        }
        io.restart();
        io.run();  // lint-run: controlled browser peers with an outer driver deadline
        if (resource.live != 0) {
            return fail(__LINE__);
        }

        return 0;
    }
};
}  // namespace

int main() {
    const std::array tests{
        &fixture::async_clients,
        &fixture::binary_fragments_and_cancel,
        &fixture::message_limit,
        &fixture::send_limit,
        &fixture::receive_limit,
        &fixture::deadline,
        &fixture::cancelled_read_deadline,
        &fixture::retained_receive_limit,
        &fixture::destruction,
        &fixture::tls_policy,
        &fixture::native_tls,
        &fixture::host_errors,
        &fixture::queued_close_deadline,
        &fixture::string_body_get,
        &fixture::head_headers,
        &fixture::early_fetch_headers,
        &fixture::fetch_cancellation,
        &fixture::fetch_cancelled_generation,
        &fixture::reconnect,
        &fixture::head_then_get,
        &fixture::fetch_policy,
        &fixture::fetch_forbidden_headers,
        &fixture::fetch_reused_reason,
        &fixture::cors_failure,
        &fixture::guest_protocol,
        &fixture::allocator_lifetime,
    };
    for (const auto test : tests) {
        fixture state;
        if ((state.*test)() != 0) {
            return 1;
        }
    }
    std::puts("browser websocket and fetch: ok");
}
