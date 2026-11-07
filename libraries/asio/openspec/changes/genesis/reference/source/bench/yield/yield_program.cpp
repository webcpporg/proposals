// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Measure the actual library yield; production headers remain unchanged.
#include <webcpp/asio/detail/browser_imports.hpp>
#include <webcpp/asio/io_context.hpp>
#include <webcpp/test/exit_status.hpp>

#include <boost/asio/post.hpp>

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string_view>
#include <vector>

namespace {

double now_ms() {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// clang-format off
EM_JS(void, bench_phase, (int active), {
    if (Module['benchPhase']) Module['benchPhase'](Boolean(active));
});
EM_JS(void, bench_publish, (const double* data, std::size_t count, double elapsed,
                           std::size_t operations, std::uint32_t checksum), {
    Module['benchData']({pairs: Array.from(HEAPF64.subarray(data / 8, data / 8 + count * 2)),
                         elapsed_ms: elapsed, operations, checksum});
});
EM_JS(void, bench_sync_call, (), { globalThis.benchSyncCalls++; });
// clang-format on

struct chain_state {
    webcpp::asio::io_context* context;
    std::vector<double>* pairs;
    std::size_t limit;
    std::size_t total;
    std::size_t completed = 0;
    std::size_t recorded = 0;
    std::uint32_t work;
    std::uint32_t checksum = 1;
};

// Function-pointer posting avoids a recursive functor instantiation.
void next_handler(chain_state* state) {
    if (state->completed != 0 && state->completed % state->limit == 0) {
        (*state->pairs)[2 * state->recorded + 1] = now_ms();
        ++state->recorded;
    }
    for (std::uint32_t i = 0; i < state->work; ++i) {
        state->checksum = state->checksum * 1664525U + 1013904223U;
    }
    ++state->completed;
    if (state->completed < state->total) {
        boost::asio::post(*state->context, std::bind_front(&next_handler, state));
        if (state->completed % state->limit == 0) {
            (*state->pairs)[2 * state->recorded] = now_ms();
        }
    }
}

}  // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE double bench_cpp_now() {
    return now_ms();
}

EMSCRIPTEN_KEEPALIVE double bench_cpp_wall() {
    return std::chrono::duration<double, std::milli>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

EMSCRIPTEN_KEEPALIVE std::uint32_t bench_cpp_touch(std::uint32_t value) {
    return value + 1;
}
}

int main(int argc, char** argv) {
    if (argc != 6) {
        std::puts("expected: mode limit samples work control");
        return webcpp::test::exit_status(2);
    }
    const std::string_view mode{argv[1]};
    const auto limit = static_cast<std::size_t>(std::strtoul(argv[2], nullptr, 10));
    const auto samples = static_cast<std::size_t>(std::strtoul(argv[3], nullptr, 10));
    const auto work = static_cast<std::uint32_t>(std::strtoul(argv[4], nullptr, 10));
    const bool control = std::string_view{argv[5]} == "1";
    if (limit == 0 || samples == 0 || samples > 100000 || limit > 10000 || work > 4096) {
        return webcpp::test::exit_status(3);
    }
    std::vector<double> pairs(samples * 2);
    if (mode != "clock" && mode != "sync") {
        // Warm the actual suspension path outside the recorded interval.
        for (int i = 0; i < 512; ++i)
            webcpp_asio_yield();
    }
    if (mode == "chain") {
        // Warm dispatch and allocation before measuring either chain variant.
        std::vector<double> warm_pairs(20);
        webcpp::asio::io_context warm_context{
            webcpp::asio::io_context_options{.max_handlers_per_turn = 1000}};
        chain_state warm{.context = &warm_context,
                         .pairs = &warm_pairs,
                         .limit = 1000,
                         .total = 10001,
                         .work = work};
        boost::asio::post(warm_context, std::bind_front(&next_handler, &warm));
        warm_context.run();  // lint-run: bounded benchmark warmup
        if (warm.completed != 10001 || warm.recorded != 10)
            return webcpp::test::exit_status(6);
    }
    bench_phase(1);
    const double started = now_ms();
    std::size_t operations = samples;
    std::uint32_t checksum = 0;
    if (mode == "direct" || mode == "clock" || mode == "sync") {
        for (std::size_t i = 0; i < samples; ++i) {
            pairs[2 * i] = now_ms();
            if (mode == "direct")
                webcpp_asio_yield();
            if (mode == "sync")
                bench_sync_call();
            pairs[2 * i + 1] = now_ms();
        }
    } else if (mode == "chain") {
        webcpp::asio::io_context context{webcpp::asio::io_context_options{
            .max_handlers_per_turn = control ? std::numeric_limits<std::size_t>::max() : limit}};
        operations = samples * limit + 1;
        chain_state state{.context = &context,
                          .pairs = &pairs,
                          .limit = limit,
                          .total = operations,
                          .work = work};
        boost::asio::post(context, std::bind_front(&next_handler, &state));
        context.run();  // lint-run: the benchmark's bounded event loop
        if (state.completed != operations || state.recorded != samples) {
            return webcpp::test::exit_status(4);
        }
        checksum = state.checksum;
    } else {
        return webcpp::test::exit_status(5);
    }
    const double elapsed = now_ms() - started;
    bench_phase(0);
    bench_publish(pairs.data(), samples, elapsed, operations, checksum);
    return webcpp::test::exit_status(0);
}
