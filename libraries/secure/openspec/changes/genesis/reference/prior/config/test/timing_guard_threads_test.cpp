// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// DIT is a thread's own state, and a guard never changes another thread's: while a guard holds DIT
// on this thread, another thread reads it clear; while another thread holds a guard, this thread
// reads it clear. Each other thread is started before the guard it reads against, so that it
// cannot inherit the state at its creation (Linux's clone copies PSTATE). Built natively, with
// threads.

#include <webcpp/config/detail/timing_guard.hpp>
#include <webcpp/test/exit_status.hpp>
#include <webcpp/test/timing_dit.hpp>

#include <boost/core/lightweight_test.hpp>

#include <future>
#include <thread>
#include <utility>

namespace {

using webcpp::config::detail::cpu_has_dit;
using webcpp::config::detail::dit_is_set;
using webcpp::config::detail::timing_guard;
using webcpp::test::force_dit;

// A guard on this thread leaves another thread's DIT clear.
void other_thread_untouched() {
    force_dit(false);
    std::promise<void> guard_entered;
    std::promise<bool> other_reading;
    std::thread other([entered = guard_entered.get_future(), &other_reading]() mutable {
        entered.wait();
        other_reading.set_value(dit_is_set());
    });
    {
        timing_guard const guard;
        guard_entered.set_value();
        BOOST_TEST(!other_reading.get_future().get());
        BOOST_TEST_EQ(dit_is_set(), cpu_has_dit());
    }
    other.join();
    BOOST_TEST(!dit_is_set());
}

// A guard on another thread leaves this thread's DIT clear, while it holds it and after.
void this_thread_untouched() {
    force_dit(false);
    std::promise<void> other_guarded;
    std::promise<void> release;
    std::thread other([&other_guarded, released = release.get_future()]() mutable {
        timing_guard const guard;
        other_guarded.set_value();
        released.wait();
    });
    other_guarded.get_future().wait();
    BOOST_TEST(!dit_is_set());
    release.set_value();
    other.join();
    BOOST_TEST(!dit_is_set());
}

}  // namespace

int main() {
    other_thread_untouched();
    this_thread_untouched();
    return webcpp::test::exit_status(boost::report_errors());
}
