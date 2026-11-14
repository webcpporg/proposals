// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DETAIL_NATIVE_CHANNEL_HPP
#define WEBCPP_WEBRTC_DETAIL_NATIVE_CHANNEL_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/detail/native/capi.hpp>
#include <webcpp/webrtc/detail/operation.hpp>
#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/types.hpp>

#include <webcpp/asio/buffer.hpp>  // lint-world: copies a message into the caller's buffer
#include <webcpp/asio/error.hpp>   // lint-world: Asio's codes
#include <webcpp/asio/post.hpp>    // lint-world: libdatachannel's events are posted

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace webcpp::webrtc::detail::native {

/** What one attempt to read a message found. */
enum class read_status {
    /** No message is queued. */
    empty,
    /** One message was appended to the caller's buffer. */
    message,
    /** The message could not be read; it stays queued. */
    failed,
};

/** The outcome of one attempt to read a message. */
struct read_result {
    /** What the attempt found. */
    read_status status = read_status::empty;
    /** The message's size in bytes, without a text's terminating zero. */
    std::size_t size = 0;
    /** Whether the message was text. */
    bool text = false;
    /** Why it could not be read. */
    error_code ec;
};

/**
 Appends the next message of the channel `dc` to the dynamic buffer `opaque` points to.

 The message is copied once into the buffer when the buffer offers one contiguous region for it,
 and through storage of the channel's allocator otherwise. A message larger than the room left in
 the buffer stays queued, and the attempt fails with `no_buffer_space`, as webcpp::beast's does.
*/
template <class DynamicBuffer, class Allocator>
read_result read_message(void* opaque, int dc, const Allocator& allocator) {
    auto& buffer = *static_cast<DynamicBuffer*>(opaque);
    int size = 0;
    int rc = rtcReceiveMessage(dc, nullptr, &size);
    if (rc == RTC_ERR_NOT_AVAIL) {
        return {};
    }
    if (rc < 0) {
        return {
            .status = read_status::failed,
            .size = 0,
            .text = false,
            .ec = make_error_code(error::operation_error),
        };
    }
    // A binary message has a positive size; a text one a negative size that counts its zero.
    const bool text = size < 0;
    const std::size_t length =
        text ? static_cast<std::size_t>(-(size + 1)) : static_cast<std::size_t>(size);
    const std::size_t needed = text ? length + 1 : length;
    const std::size_t room = buffer.max_size() - buffer.size();
    if (length > room) {
        return {
            .status = read_status::failed,
            .size = 0,
            .text = false,
            .ec = error_code(webcpp::asio::error::no_buffer_space),
        };
    }
    if (needed == 0) {
        // An empty binary message: the C API discards a message only into a buffer.
        char sink = 0;
        int capacity = 1;
        rc = rtcReceiveMessage(dc, &sink, &capacity);
        if (rc < 0) {
            return {
                .status = read_status::failed,
                .size = 0,
                .text = false,
                .ec = make_error_code(error::operation_error),
            };
        }
        return {.status = read_status::message, .size = 0, .text = false, .ec = {}};
    }
    if (needed <= room) {
        const auto prepared = buffer.prepare(needed);
        const auto first = *webcpp::asio::buffer_sequence_begin(prepared);
        if (first.size() >= needed) {
            int capacity = static_cast<int>(needed);
            rc = rtcReceiveMessage(dc, static_cast<char*>(first.data()), &capacity);
            if (rc < 0) {
                return {
                    .status = read_status::failed,
                    .size = 0,
                    .text = false,
                    .ec = make_error_code(error::operation_error),
                };
            }
            buffer.commit(length);
            return {.status = read_status::message, .size = length, .text = text, .ec = {}};
        }
    }
    using storage_allocator = std::allocator_traits<Allocator>::template rebind_alloc<char>;
    std::vector<char, storage_allocator> storage(needed, storage_allocator(allocator));
    int capacity = static_cast<int>(needed);
    rc = rtcReceiveMessage(dc, storage.data(), &capacity);
    if (rc < 0) {
        return {
            .status = read_status::failed,
            .size = 0,
            .text = false,
            .ec = make_error_code(error::operation_error),
        };
    }
    buffer.commit(webcpp::asio::buffer_copy(buffer.prepare(length),
                                            webcpp::asio::buffer(storage.data(), length)));
    return {.status = read_status::message, .size = length, .text = text, .ec = {}};
}

/**
 The state of a native data channel: libdatachannel's channel, by its id in the C API, and what
 waits on it, on the object's executor.

 libdatachannel's callbacks run on its threads; each posts to the executor through a weak pointer,
 so that one posted before the channel went finds nothing (design.md, D7).
*/
template <class Executor, class Allocator>
class channel_state : public std::enable_shared_from_this<channel_state<Executor, Allocator>> {
public:
    /** The channel's text type. */
    using string_type = detail::string<Allocator>;

    /** Makes the state of a channel not yet attached to libdatachannel. */
    channel_state(const Executor& executor, const Allocator& allocator, const options& opts)
        : executor_(executor),
          allocator_(allocator),
          label_(typename string_type::allocator_type(allocator)),
          inbox_{{}, executor},
          max_message_size_(opts.max_message_size),
          max_buffered_amount_(opts.max_buffered_amount) {}

    channel_state(const channel_state&) = delete;
    channel_state& operator=(const channel_state&) = delete;
    channel_state(channel_state&&) = delete;
    channel_state& operator=(channel_state&&) = delete;

    /** Deletes libdatachannel's channel, aborting what waits. */
    // Posting the aborted completions allocates, and a failure to allocate there ends the
    // program, as in any noexcept destructor that must complete operations.
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~channel_state() { destroy(); }

    /**
     Takes over libdatachannel's channel `dc`: its user pointer, its callbacks, its label and the
     state it is in already, since it may have opened, or closed, before.
    */
    void attach(int dc) {
        dc_ = dc;
        inbox_.self = this->weak_from_this();
        rtcSetUserPointer(dc_, &inbox_);
        rtcSetOpenCallback(dc_, &on_open);
        rtcSetClosedCallback(dc_, &on_closed);
        rtcSetBufferedAmountLowCallback(dc_, &on_buffered_amount_low);
        rtcSetAvailableCallback(dc_, &on_available);
        static_cast<void>(read_text(&rtcGetDataChannelLabel, dc_, label_));
        if (rtcIsClosed(dc_)) {
            ready_ = data_channel_state::closed;
        } else if (rtcIsOpen(dc_)) {
            ready_ = data_channel_state::open;
        }
    }

    /** The object's executor. */
    [[nodiscard]] const Executor& get_executor() const noexcept { return executor_; }

    /** The object's allocator. */
    [[nodiscard]] const Allocator& get_allocator() const noexcept { return allocator_; }

    /** Waits for the channel to open; at once when it is. */
    void wait_open(pending<>& op) {
        if (open_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (ready_ == data_channel_state::open) {
            op.complete({});
            return;
        }
        if (ready_ != data_channel_state::connecting) {
            op.fail(make_error_code(error::closed));
            return;
        }
        op.link(open_);
    }

    /** Waits for the channel to close; at once when it is. */
    void wait_close(pending<>& op) {
        if (close_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (ready_ == data_channel_state::closed) {
            op.complete({});
            return;
        }
        op.link(close_);
    }

    /** Waits for the buffered amount to fall to the threshold; at once when it is there. */
    void wait_buffered_amount_low(pending<>& op) {
        if (low_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (ready_ == data_channel_state::closing || ready_ == data_channel_state::closed) {
            op.fail(make_error_code(error::closed));
            return;
        }
        if (buffered_amount() <= threshold_) {
            op.complete({});
            return;
        }
        op.link(low_);
    }

    /** Waits for the next message, appended to `buffer`, which outlives the wait. */
    template <class DynamicBuffer>
    void receive(DynamicBuffer& buffer, pending<std::size_t, bool>& op) {
        if (receive_ != nullptr) {
            op.fail(webcpp::asio::error::already_started);
            return;
        }
        if (dc_ < 0 || ready_ == data_channel_state::closing) {
            op.fail(make_error_code(error::closed));
            return;
        }
        target_ = &buffer;
        read_ = &read_message<DynamicBuffer, Allocator>;
        op.link(receive_);
        attempt();
        if (receive_ != nullptr && ready_ == data_channel_state::closed) {
            receive_->fail(make_error_code(error::closed));
        }
    }

    /**
     Sends one message, binary or text, at once or not at all.

     @return The failure, or nothing once libdatachannel queued it.
    */
    error_code send(const char* data, std::size_t size, bool text) {
        if (dc_ < 0 || ready_ != data_channel_state::open) {
            return make_error_code(error::invalid_state);
        }
        const int peer = rtcMaxMessageSize(dc_);
        const std::size_t limit = peer > 0
                                      ? std::min(max_message_size_, static_cast<std::size_t>(peer))
                                      : max_message_size_;
        if (size > limit || !fits_int(size)) {
            return make_error_code(error::message_too_large);
        }
        const std::size_t queued = buffered_amount();
        if (size > max_buffered_amount_ || queued > max_buffered_amount_ - size) {
            return make_error_code(error::buffered_amount_exceeded);
        }
        int rc = 0;
        if (text) {
            // The C API takes a text as a zero-terminated string, which cannot hold a zero.
            const std::string_view view(data, size);
            if (view.find('\0') != std::string_view::npos) {
                return {webcpp::asio::error::operation_not_supported};
            }
            const string_type terminated(view, typename string_type::allocator_type(allocator_));
            rc = rtcSendMessage(dc_, terminated.c_str(), -1);
        } else {
            rc = rtcSendMessage(dc_, data, static_cast<int>(size));
        }
        if (rc < 0) {
            // It closed meanwhile, on libdatachannel's thread, or refused the message.
            return code_of(rc, error::operation_error, error::invalid_state);
        }
        return {};
    }

    /** Sets the threshold of `async_wait_buffered_amount_low`. */
    void set_buffered_amount_low_threshold(std::size_t threshold) {
        threshold_ = threshold;
        if (dc_ >= 0) {
            rtcSetBufferedAmountLowThreshold(
                dc_, static_cast<int>(std::min<std::size_t>(threshold, INT_MAX)));
        }
        buffered_amount_low();
    }

    /** The bytes queued to send. */
    [[nodiscard]] std::size_t buffered_amount() const {
        const int amount = dc_ >= 0 ? rtcGetBufferedAmount(dc_) : 0;
        return amount > 0 ? static_cast<std::size_t>(amount) : 0;
    }

    /** The channel's state. */
    [[nodiscard]] data_channel_state ready_state() const noexcept { return ready_; }

    /** The channel's label. */
    [[nodiscard]] std::string_view label() const noexcept { return label_; }

    /** The channel's SCTP stream id, once it has one. */
    [[nodiscard]] std::optional<std::uint16_t> id() const {
        const int stream = dc_ >= 0 ? rtcGetDataChannelStream(dc_) : -1;
        if (stream < 0 || stream > 65535) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(stream);
    }

    /**
     Closes the channel by its owner: what waits ends with `operation_aborted`, but
     `async_wait_close`, and unread messages are discarded, as the W3C discards a message
     received once the channel left "open".
    */
    void close() {
        if (dc_ < 0 || ready_ == data_channel_state::closing ||
            ready_ == data_channel_state::closed) {
            return;
        }
        ready_ = data_channel_state::closing;
        abort(webcpp::asio::error::operation_aborted);
        rtcClose(dc_);
        drain();
    }

    /** Closes the channel because its connection closed: what waits ends with `error::closed`. */
    void connection_closed() {
        if (dc_ < 0 || ready_ == data_channel_state::closed) {
            return;
        }
        ready_ = data_channel_state::closed;
        abort(make_error_code(error::closed));
        rtcClose(dc_);
        drain();
        if (close_ != nullptr) {
            close_->complete({});
        }
    }

    /** Deletes libdatachannel's channel, after which none of its callbacks runs. */
    void destroy() {
        if (dc_ < 0) {
            return;
        }
        ready_ = data_channel_state::closed;
        abort(webcpp::asio::error::operation_aborted);
        if (close_ != nullptr) {
            close_->fail(webcpp::asio::error::operation_aborted);
        }
        rtcClose(dc_);
        drain();
        // Blocks until every callback of the channel in flight returned; none runs afterwards.
        rtcDeleteDataChannel(dc_);
        dc_ = -1;
    }

private:
    struct inbox {
        std::weak_ptr<channel_state> self;
        Executor executor;
    };

    using event = void (channel_state::*)();

    // Runs on libdatachannel's threads: posts, through the weak pointer, and touches nothing of
    // the user's allocator; the post carries Asio's default allocator.
    static void post_event(void* pointer, event what) {
        const auto& box = *static_cast<const inbox*>(pointer);
        webcpp::asio::post(box.executor, [self = box.self, what] {
            if (const auto state = self.lock()) {
                ((*state).*what)();
            }
        });
    }

    static void RTC_API on_open(int /*id*/, void* pointer) {
        post_event(pointer, &channel_state::opened);
    }

    static void RTC_API on_closed(int /*id*/, void* pointer) {
        post_event(pointer, &channel_state::closed);
    }

    static void RTC_API on_buffered_amount_low(int /*id*/, void* pointer) {
        post_event(pointer, &channel_state::buffered_amount_low);
    }

    static void RTC_API on_available(int /*id*/, void* pointer) {
        post_event(pointer, &channel_state::attempt);
    }

    void opened() {
        if (dc_ < 0 || ready_ != data_channel_state::connecting) {
            return;
        }
        ready_ = data_channel_state::open;
        if (open_ != nullptr) {
            open_->complete({});
        }
    }

    // The channel closed: by the peer, by its transport, or as its owner asked. Messages that
    // arrived before the peer closed it are still delivered, then receives fail with `closed`.
    void closed() {
        if (dc_ < 0) {
            return;
        }
        ready_ = data_channel_state::closed;
        if (open_ != nullptr) {
            open_->fail(make_error_code(error::closed));
        }
        if (low_ != nullptr) {
            low_->fail(make_error_code(error::closed));
        }
        attempt();
        if (receive_ != nullptr) {
            receive_->fail(make_error_code(error::closed));
        }
        if (close_ != nullptr) {
            close_->complete({});
        }
    }

    void buffered_amount_low() {
        if (low_ != nullptr && buffered_amount() <= threshold_) {
            low_->complete({});
        }
    }

    // Reads one message for the pending receive, if one is queued.
    void attempt() {
        if (receive_ == nullptr || dc_ < 0) {
            return;
        }
        const read_result result = read_(target_, dc_, allocator_);
        switch (result.status) {
            case read_status::message: receive_->complete({}, result.size, result.text); break;
            case read_status::failed: receive_->fail(result.ec); break;
            case read_status::empty: break;
        }
    }

    void abort(error_code ec) {
        if (open_ != nullptr) {
            open_->fail(ec);
        }
        if (receive_ != nullptr) {
            receive_->fail(ec);
        }
        if (low_ != nullptr) {
            low_->fail(ec);
        }
    }

    // Discards what is queued in libdatachannel, which also releases a libdatachannel thread
    // waiting for room in a full queue. A closed channel queues nothing new, so this ends.
    void drain() {
        using storage_allocator = std::allocator_traits<Allocator>::template rebind_alloc<char>;
        std::vector<char, storage_allocator> sink{storage_allocator(allocator_)};
        for (;;) {
            int size = 0;
            if (rtcReceiveMessage(dc_, nullptr, &size) != RTC_ERR_SUCCESS) {
                return;
            }
            const auto needed = static_cast<std::size_t>(size < 0 ? -size : size) + 1;
            if (sink.size() < needed) {
                sink.resize(needed);
            }
            int capacity = static_cast<int>(sink.size());
            if (rtcReceiveMessage(dc_, sink.data(), &capacity) != RTC_ERR_SUCCESS) {
                return;
            }
        }
    }

    Executor executor_;
    Allocator allocator_;
    string_type label_;
    inbox inbox_;
    int dc_ = -1;
    data_channel_state ready_ = data_channel_state::connecting;
    std::size_t max_message_size_ = 0;
    std::size_t max_buffered_amount_ = 0;
    std::size_t threshold_ = 0;
    pending<>* open_ = nullptr;
    pending<>* close_ = nullptr;
    pending<>* low_ = nullptr;
    pending<std::size_t, bool>* receive_ = nullptr;
    void* target_ = nullptr;
    read_result (*read_)(void*, int, const Allocator&) = nullptr;
};

}  // namespace webcpp::webrtc::detail::native

#endif  // WEBCPP_WEBRTC_DETAIL_NATIVE_CHANNEL_HPP
