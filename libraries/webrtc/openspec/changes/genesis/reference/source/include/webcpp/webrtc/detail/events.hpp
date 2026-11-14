// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DETAIL_EVENTS_HPP
#define WEBCPP_WEBRTC_DETAIL_EVENTS_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/detail/operation.hpp>
#include <webcpp/webrtc/error.hpp>

#include <webcpp/asio/error.hpp>  // lint-world: Asio's codes

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <utility>

// What an event becomes while no wait of its kind is pending (design.md, D5), common to both
// backends and testable without a platform: each class holds at most one wait, completes it from
// what it keeps, and runs on the object's executor only.

namespace webcpp::webrtc::detail {

/**
 A state whose changes coalesce while no wait is pending: a wait completes with the current state
 once it differs from the one the previous wait completed with, or from the first enumerator.
*/
template <class State>
class state_watch {
public:
    /** The current state. */
    [[nodiscard]] State current() const noexcept { return current_; }

    /** Starts a wait, which completes once the state differs from the one last delivered. */
    void wait(pending<State>& op) {
        if (waiter_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (closed_) {
            op.fail(make_error_code(error::closed));
            return;
        }
        op.link(waiter_);
        deliver();
    }

    /** Records a change, which completes the pending wait if it is one. */
    void update(State state) {
        if (closed_) {
            return;
        }
        current_ = state;
        deliver();
    }

    /**
     Ends the watch: the state reads `state`, nothing more is delivered, the wait fails with `ec`.
    */
    void close(State state, error_code ec) {
        current_ = state;
        closed_ = true;
        if (waiter_ != nullptr) {
            waiter_->fail(ec);
        }
    }

private:
    void deliver() {
        if (waiter_ != nullptr && current_ != delivered_) {
            delivered_ = current_;
            waiter_->complete({}, current_);
        }
    }

    State current_{};
    State delivered_{};
    pending<State>* waiter_ = nullptr;
    bool closed_ = false;
};

/**
 Events kept in order while no wait is pending, in the object's allocator, up to a bound; an
 exempt event, as the end of gathering, has a place of its own past it.
*/
template <class T, class Allocator>
class event_queue {
public:
    /** Makes an empty queue of at most `bound` events that are not exempt. */
    event_queue(std::size_t bound, const Allocator& allocator)
        : queue_(allocator_type(allocator)), bound_(bound) {}

    /** Starts a wait, which completes with the oldest event kept, or with the next one. */
    void wait(pending<T>& op) {
        if (waiter_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (!queue_.empty()) {
            T value = std::move(queue_.front().value);
            if (!queue_.front().exempt) {
                --counted_;
            }
            queue_.pop_front();
            op.complete({}, std::move(value));
            return;
        }
        if (closed_) {
            op.fail(make_error_code(error::closed));
            return;
        }
        op.link(waiter_);
    }

    /**
     Hands `value` to the pending wait, or keeps it.

     @return Empty when it was delivered or kept; the value itself when the bound refused it, for
     the caller to dispose of (a channel past the backlog is closed).
    */
    std::optional<T> push(T value, bool exempt = false) {
        if (closed_) {
            return std::optional<T>(std::move(value));
        }
        if (waiter_ != nullptr) {
            waiter_->complete({}, std::move(value));
            return std::nullopt;
        }
        if (!exempt && counted_ >= bound_) {
            return std::optional<T>(std::move(value));
        }
        if (!exempt) {
            ++counted_;
        }
        queue_.push_back(entry{std::move(value), exempt});
        return std::nullopt;
    }

    /** The number of events kept. */
    [[nodiscard]] std::size_t size() const noexcept { return queue_.size(); }

    /** Ends the queue: what it keeps is dropped, and the wait fails with `ec`. */
    void close(error_code ec) {
        closed_ = true;
        counted_ = 0;
        queue_.clear();
        if (waiter_ != nullptr) {
            waiter_->fail(ec);
        }
    }

private:
    struct entry {
        T value;
        bool exempt = false;
    };

    using allocator_type = std::allocator_traits<Allocator>::template rebind_alloc<entry>;

    std::deque<entry, allocator_type> queue_;
    std::size_t bound_ = 0;
    std::size_t counted_ = 0;
    pending<T>* waiter_ = nullptr;
    bool closed_ = false;
};

}  // namespace webcpp::webrtc::detail

#endif  // WEBCPP_WEBRTC_DETAIL_EVENTS_HPP
