// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_WEBRTC_DETAIL_NATIVE_CONNECTION_HPP
#define WEBCPP_WEBRTC_DETAIL_NATIVE_CONNECTION_HPP

#include <webcpp/webrtc/config.hpp>

#include <webcpp/webrtc/data_channel.hpp>
#include <webcpp/webrtc/detail/events.hpp>
#include <webcpp/webrtc/detail/native/capi.hpp>
#include <webcpp/webrtc/detail/native/channel.hpp>
#include <webcpp/webrtc/detail/operation.hpp>
#include <webcpp/webrtc/error.hpp>
#include <webcpp/webrtc/options.hpp>
#include <webcpp/webrtc/runtime.hpp>
#include <webcpp/webrtc/types.hpp>

#include <webcpp/asio/error.hpp>  // lint-world: Asio's codes
#include <webcpp/asio/post.hpp>   // lint-world: libdatachannel's events are posted

#include <boost/system/result.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace webcpp::webrtc::detail::native {

/**
 The state of a native peer connection: libdatachannel's connection, by its id in the C API, its
 channels, and what waits on it, on the object's executor.

 libdatachannel's callbacks run on its threads; each copies what it carries into the standard
 allocator and posts to the executor through a weak pointer (design.md, D7).
*/
template <class Executor, class Allocator>
class connection_state
    : public std::enable_shared_from_this<connection_state<Executor, Allocator>> {
public:
    /** A channel's state. */
    using channel_type = channel_state<Executor, Allocator>;
    /** A channel. */
    using data_channel = basic_data_channel<Executor, Allocator>;
    /** A description, owned. */
    using session_description = basic_session_description<Allocator>;
    /** A candidate, owned. */
    using ice_candidate = basic_ice_candidate<Allocator>;
    /** The connection's text type. */
    using string_type = detail::string<Allocator>;

    /** Makes the state of a connection not yet made in libdatachannel. */
    connection_state(runtime_slot slot, const Executor& executor, const options& opts,
                     const Allocator& allocator)
        : slot_(std::move(slot)),
          executor_(executor),
          allocator_(allocator),
          options_(opts),
          inbox_{{}, executor},
          candidates_(opts.max_queued_candidates, allocator),
          channels_(opts.max_queued_channels, allocator),
          registry_(registry_allocator(allocator)) {}

    connection_state(const connection_state&) = delete;
    connection_state& operator=(const connection_state&) = delete;
    connection_state(connection_state&&) = delete;
    connection_state& operator=(connection_state&&) = delete;

    /** Deletes libdatachannel's connection, aborting what waits. */
    // Posting the aborted completions allocates, and a failure to allocate there ends the
    // program, as in any noexcept destructor that must complete operations.
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~connection_state() { destroy(); }

    /**
     Makes libdatachannel's connection, with automatic negotiation off.

     @return `error::syntax_error` for an ICE server URL libdatachannel cannot parse,
     `error::type_error` for a bound out of the C API's range, `error::operation_error` for any
     other refusal; nothing once made.
    */
    error_code open(const configuration& config) {
        if (!fits_int(options_.max_message_size)) {
            return make_error_code(error::type_error);
        }
        using strings_allocator =
            std::allocator_traits<Allocator>::template rebind_alloc<string_type>;
        using pointers_allocator =
            std::allocator_traits<Allocator>::template rebind_alloc<const char*>;
        std::vector<string_type, strings_allocator> urls{strings_allocator(allocator_)};
        for (const ice_server& server : config.ice_servers) {
            for (const std::string_view url : server.urls) {
                urls.push_back(ice_url(url, server.username, server.credential));
            }
        }
        if (!fits_int(urls.size())) {
            return make_error_code(error::type_error);
        }
        std::vector<const char*, pointers_allocator> pointers{pointers_allocator(allocator_)};
        for (const string_type& url : urls) {
            pointers.push_back(url.c_str());
        }
        const string_type bind(config.bind_address, text_allocator());

        rtcConfiguration native{};
        native.iceServers = pointers.empty() ? nullptr : pointers.data();
        native.iceServersCount = static_cast<int>(pointers.size());
        native.bindAddress = bind.empty() ? nullptr : bind.c_str();
        native.iceTransportPolicy = config.policy == ice_transport_policy::relay
                                        ? RTC_TRANSPORT_POLICY_RELAY
                                        : RTC_TRANSPORT_POLICY_ALL;
        // A description exists only when async_set_local_description asks for one, as in the
        // browser (design.md, D15).
        native.disableAutoNegotiation = true;
        native.maxMessageSize = static_cast<int>(options_.max_message_size);
        const int pc = rtcCreatePeerConnection(&native);
        if (pc < 0) {
            return code_of(pc, error::syntax_error, error::operation_error);
        }
        pc_ = pc;
        inbox_.self = this->weak_from_this();
        rtcSetUserPointer(pc_, &inbox_);
        rtcSetLocalCandidateCallback(pc_, &on_candidate);
        rtcSetStateChangeCallback(pc_, &on_state);
        rtcSetIceStateChangeCallback(pc_, &on_ice_state);
        rtcSetGatheringStateChangeCallback(pc_, &on_gathering_state);
        rtcSetDataChannelCallback(pc_, &on_data_channel);
        return {};
    }

    /** The object's executor. */
    [[nodiscard]] const Executor& get_executor() const noexcept { return executor_; }

    /** The object's allocator. */
    [[nodiscard]] const Allocator& get_allocator() const noexcept { return allocator_; }

    /** Creates and applies the implicit local description, an offer or an answer. */
    void set_local_description(pending<session_description>& op) {
        if (closed_) {
            op.fail(make_error_code(error::invalid_state));
            return;
        }
        // Synchronous in libdatachannel: the description is set when the call returns.
        const int rc = rtcSetLocalDescription(pc_, nullptr);
        if (rc < 0) {
            op.fail(code_of(rc, error::operation_error, error::operation_error));
            return;
        }
        auto description = local_description();
        if (!description) {
            op.fail(make_error_code(error::operation_error));
            return;
        }
        op.complete({}, std::move(*description));
    }

    /** Applies the remote description `sdp` of type `type`. */
    void set_remote_description(sdp_type type, const string_type& sdp, pending<>& op) {
        if (closed_) {
            op.fail(make_error_code(error::invalid_state));
            return;
        }
        const int rc = rtcSetRemoteDescription(pc_, sdp.c_str(), type_name(type));
        if (rc < 0) {
            // invalid_argument: a description it cannot parse; another exception: a description
            // the signaling state does not admit.
            op.fail(code_of(rc, error::operation_error, error::invalid_state));
            return;
        }
        op.complete({});
    }

    /** Adds the remote candidate `candidate`; an empty one adds none. */
    void add_ice_candidate(const string_type& candidate, const std::optional<string_type>& mid,
                           pending<>& op) {
        if (closed_ || !has_remote_description()) {
            op.fail(make_error_code(error::invalid_state));
            return;
        }
        if (candidate.empty()) {
            op.complete({});
            return;
        }
        const int rc = rtcAddRemoteCandidate(pc_, candidate.c_str(), mid ? mid->c_str() : nullptr);
        if (rc < 0) {
            op.fail(code_of(rc, error::operation_error, error::operation_error));
            return;
        }
        op.complete({});
    }

    /** Waits for the next local candidate, or for the end of gathering. */
    void wait_ice_candidate(pending<ice_candidate>& op) { candidates_.wait(op); }

    /** Waits for the connection's state to change. */
    void wait_state_change(pending<peer_connection_state>& op) { state_.wait(op); }

    /** Waits for the ICE state to change. */
    void wait_ice_state_change(pending<ice_connection_state>& op) { ice_.wait(op); }

    /** Waits for the next channel the peer opened. */
    void accept_data_channel(pending<data_channel>& op) { channels_.wait(op); }

    /** Creates a channel, the W3C's `createDataChannel`. */
    boost::system::result<data_channel> create_data_channel(std::string_view label,
                                                            const data_channel_init& init) {
        if (init.max_packet_life_time && init.max_retransmits) {
            return make_error_code(error::type_error);
        }
        if ((init.negotiated && !init.id) || (init.id && *init.id == 65535)) {
            return make_error_code(error::type_error);
        }
        if (label.size() > 65535 || init.protocol.size() > 65535) {
            return make_error_code(error::type_error);
        }
        if (closed_) {
            return make_error_code(error::invalid_state);
        }
        const string_type label_text(label, text_allocator());
        const string_type protocol(init.protocol, text_allocator());
        rtcDataChannelInit native{};
        native.reliability.unordered = !init.ordered;
        if (init.max_packet_life_time) {
            native.reliability.unreliable = true;
            native.reliability.maxPacketLifeTime = *init.max_packet_life_time;
        } else if (init.max_retransmits) {
            native.reliability.unreliable = true;
            native.reliability.maxRetransmits = *init.max_retransmits;
        }
        native.protocol = protocol.c_str();
        native.negotiated = init.negotiated;
        if (init.id) {
            native.manualStream = true;
            native.stream = *init.id;
        }
        const int dc = rtcCreateDataChannelEx(pc_, label_text.c_str(), &native);
        if (dc < 0) {
            return code_of(dc, error::operation_error, error::operation_error);
        }
        return channel_access::make<data_channel>(adopt(dc));
    }

    /** The local description, once one was set. */
    [[nodiscard]] std::optional<session_description> local_description() const {
        if (pc_ < 0) {
            return std::nullopt;
        }
        string_type name(text_allocator());
        if (read_text(&rtcGetLocalDescriptionType, pc_, name) != 0) {
            return std::nullopt;
        }
        const auto type = type_of(name);
        if (!type) {
            return std::nullopt;
        }
        session_description description{*type, string_type(text_allocator())};
        if (read_text(&rtcGetLocalDescription, pc_, description.sdp) != 0) {
            return std::nullopt;
        }
        return description;
    }

    /** Whether a remote description was applied. */
    [[nodiscard]] bool has_remote_description() const {
        return pc_ >= 0 && rtcGetRemoteDescription(pc_, nullptr, 0) > 0;
    }

    /** The connection's state. */
    [[nodiscard]] peer_connection_state state() const noexcept { return state_.current(); }

    /** The ICE state. */
    [[nodiscard]] ice_connection_state ice_state() const noexcept { return ice_.current(); }

    /**
     Closes the connection, as the W3C's `close()`: no state change is delivered, what waits on
     the connection ends with `operation_aborted`, and its channels close with `error::closed`.
    */
    void close() {
        if (closed_) {
            return;
        }
        closed_ = true;
        const error_code aborted(webcpp::asio::error::operation_aborted);
        state_.close(peer_connection_state::closed, aborted);
        ice_.close(ice_connection_state::closed, aborted);
        candidates_.close(aborted);
        // Channels not yet accepted go with the queue, and close.
        channels_.close(aborted);
        // The channels drain their queues before the connection closes its transports, so that
        // no libdatachannel thread waits for room in one meanwhile.
        for (const auto& weak : registry_) {
            if (const auto channel = weak.lock()) {
                channel->connection_closed();
            }
        }
        registry_.clear();
        if (pc_ >= 0) {
            rtcClosePeerConnection(pc_);
        }
    }

    /** Deletes libdatachannel's connection, after which none of its callbacks runs. */
    void destroy() {
        close();
        if (pc_ >= 0) {
            // Blocks until every callback of the connection in flight returned.
            rtcDeletePeerConnection(pc_);
            pc_ = -1;
        }
        slot_.reset();
    }

private:
    struct inbox {
        std::weak_ptr<connection_state> self;
        Executor executor;
    };

    using registry_allocator =
        std::allocator_traits<Allocator>::template rebind_alloc<std::weak_ptr<channel_type>>;
    using text_allocator_type = string_type::allocator_type;

    [[nodiscard]] text_allocator_type text_allocator() const {
        return text_allocator_type(allocator_);
    }

    // Runs on libdatachannel's threads: posts `f`, which runs on the state if it still exists.
    template <class F>
    static void post_event(void* pointer, F f) {
        const auto& box = *static_cast<const inbox*>(pointer);
        webcpp::asio::post(box.executor, [self = box.self, f = std::move(f)]() mutable {
            if (const auto state = self.lock()) {
                f(*state);
            }
        });
    }

    static void RTC_API on_candidate(int /*pc*/, const char* candidate, const char* mid,
                                     void* pointer) {
        // Copied in the standard allocator: the user's is not touched on this thread.
        post_event(pointer, [text = std::string(candidate != nullptr ? candidate : ""),
                             media = std::string(mid != nullptr ? mid : "")](
                                connection_state& state) { state.gathered(text, media); });
    }

    static void RTC_API on_state(int /*pc*/, rtcState state, void* pointer) {
        post_event(pointer, [state](connection_state& self) {
            if (!self.closed_) {
                self.state_.update(state_of(state));
            }
        });
    }

    static void RTC_API on_ice_state(int /*pc*/, rtcIceState state, void* pointer) {
        post_event(pointer, [state](connection_state& self) {
            if (!self.closed_) {
                self.ice_.update(ice_state_of(state));
            }
        });
    }

    static void RTC_API on_gathering_state(int /*pc*/, rtcGatheringState state, void* pointer) {
        if (state == RTC_GATHERING_COMPLETE) {
            post_event(pointer, [](connection_state& self) { self.gathered({}, {}); });
        }
    }

    static void RTC_API on_data_channel(int /*pc*/, int dc, void* pointer) {
        const auto& box = *static_cast<const inbox*>(pointer);
        webcpp::asio::post(box.executor, [self = box.self, dc] {
            if (const auto state = self.lock()) {
                state->arrived(dc);
            } else {
                // The connection went: the channel goes too.
                rtcDeleteDataChannel(dc);
            }
        });
    }

    // A local candidate, or with an empty text the end of gathering, which the bound keeps a
    // place for (design.md, D5).
    void gathered(const std::string& text, const std::string& mid) {
        if (closed_) {
            return;
        }
        const bool end = text.empty();
        ice_candidate candidate{
            string_type(text, text_allocator()),
            string_type(mid, text_allocator()),
            end ? std::nullopt : std::optional<std::uint16_t>(0),
            string_type(text_allocator()),
        };
        // A candidate past the bound is dropped here; it is still in local_description().
        static_cast<void>(candidates_.push(std::move(candidate), end));
    }

    // A channel the peer opened.
    void arrived(int dc) {
        if (closed_) {
            rtcDeleteDataChannel(dc);
            return;
        }
        // Past the backlog, the refused channel is destroyed here, which closes it.
        static_cast<void>(channels_.push(channel_access::make<data_channel>(adopt(dc))));
    }

    std::shared_ptr<channel_type> adopt(int dc) {
        auto channel =
            std::allocate_shared<channel_type>(allocator_, executor_, allocator_, options_);
        channel->attach(dc);
        std::erase_if(registry_, [](const auto& weak) { return weak.expired(); });
        registry_.push_back(channel);
        return channel;
    }

    static string_type ice_url(std::string_view url, std::string_view username,
                               std::string_view credential, const text_allocator_type& allocator) {
        string_type out(url, allocator);
        // "turn:host" with a user name becomes "turn:user:credential@host", the form
        // libdatachannel parses, as trystero writes it.
        const bool turn = url.starts_with("turn:") || url.starts_with("turns:");
        if (!turn || username.empty()) {
            return out;
        }
        const auto colon = url.find(':');
        out.assign(url.substr(0, colon + 1));
        out.append(username);
        out.push_back(':');
        out.append(credential);
        out.push_back('@');
        out.append(url.substr(colon + 1));
        return out;
    }

    string_type ice_url(std::string_view url, std::string_view username,
                        std::string_view credential) const {
        return ice_url(url, username, credential, text_allocator());
    }

    runtime_slot slot_;
    Executor executor_;
    Allocator allocator_;
    options options_;
    inbox inbox_;
    int pc_ = -1;
    bool closed_ = false;
    state_watch<peer_connection_state> state_;
    state_watch<ice_connection_state> ice_;
    event_queue<ice_candidate, Allocator> candidates_;
    event_queue<data_channel, Allocator> channels_;
    std::vector<std::weak_ptr<channel_type>, registry_allocator> registry_;
};

}  // namespace webcpp::webrtc::detail::native

#endif  // WEBCPP_WEBRTC_DETAIL_NATIVE_CONNECTION_HPP
