// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
// Distributed under the Boost Software License, Version 1.0.

// Boost 1.92's entry-point setup with webcpp's bounded run loop. These definitions replace
// Boost.Cobalt's src/main.cpp; compiling them probes the internal main_promise interface.
#include <webcpp/asio/io_context.hpp>

#include <boost/asio/post.hpp>        // lint-world: starts the application's coroutine
#include <boost/asio/signal_set.hpp>  // lint-world: preserves Cobalt's signal cancellation
#include <boost/cobalt/main.hpp>

#include <csignal>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace boost::cobalt::detail {

auto main_promise::final_suspend() noexcept -> std::suspend_never {
    system::error_code ec;
    if (signal_set != nullptr) {
        // Preserve Boost's nonthrowing cleanup; the return duplicates ec.
        // NOLINTNEXTLINE(bugprone-unused-return-value)
        signal_set->cancel(ec);
    }
    return {};
}

int main_promise::run_main(::boost::cobalt::main mn) {
    int result = -1;
    {
        webcpp::asio::io_context ctx{BOOST_ASIO_CONCURRENCY_HINT_1};
        boost::cobalt::this_thread::set_executor(ctx.get_executor());
        mn.promise->result = &result;
        mn.promise->exec.emplace(ctx.get_executor());
        mn.promise->exec_ = mn.promise->exec->get_executor();
        const auto coroutine = std::coroutine_handle<main_promise>::from_promise(*mn.promise);
        asio::basic_signal_set<executor_type> signals{ctx, SIGINT, SIGTERM};
        mn.promise->signal_set = &signals;

        struct signal_wait {
            asio::basic_signal_set<executor_type>& signals;
            asio::cancellation_signal& cancellation;

            void operator()(system::error_code ec, int signal) const {
                if (signal == SIGINT) {
                    cancellation.emit(asio::cancellation_type::total);
                }
                if (signal == SIGTERM) {
                    cancellation.emit(asio::cancellation_type::terminal);
                }
                if (!ec) {
                    signals.async_wait(*this);
                }
            }
        };

        signals.async_wait(signal_wait{.signals = signals, .cancellation = mn.promise->signal});
        asio::post(ctx.get_executor(), [coroutine] { coroutine.resume(); });
        ctx.run();  // lint-run: Cobalt's application event loop
    }
#ifdef __EMSCRIPTEN__
    // Destroy the context before exiting; a suspended Asyncify main loses a returned status.
    if (emscripten_has_asyncify() == 1) {
        std::exit(result);
    }
#endif
    return result;
}

}  // namespace boost::cobalt::detail
