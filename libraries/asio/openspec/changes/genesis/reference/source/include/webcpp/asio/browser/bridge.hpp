// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_ASIO_BROWSER_BRIDGE_HPP
#define WEBCPP_ASIO_BROWSER_BRIDGE_HPP

#include <webcpp/asio/config.hpp>

#include <webcpp/asio/detail/browser_imports.hpp>

#include <emscripten/val.h>
#include <boost/asio/execution_context.hpp>  // lint-world: a service of the io_context
#include <boost/intrusive/avl_set.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include <cstdint>
#include <functional>
#include <utility>

namespace webcpp::asio::browser {

/**
 What an event delivered through the bridge reaches: it posts, nothing else.

 Its owner (an operation, a stream, a channel) allocates it as it chooses, adds it to the bridge,
 and removes it before destroying it.

 @see "Browser events", in the guide.
*/
class registration {
public:
    /**
     Posts what event completes or notifies to the io_context.

     It never polls, runs or dispatches, and may free the registration when it returns true.

     @param event The JavaScript value the event's function was called with.
     @return Whether the registration is done, so that the bridge removes it.
    */
    virtual bool deliver(const emscripten::val& event) = 0;

    /** The shutdown of the bridge's io_context: destroys what is pending without calling it. */
    virtual void shutdown() noexcept = 0;

protected:
    /** Constructs a registration. */
    registration() = default;
    /** Destroys a registration, which only its owner does. */
    ~registration() = default;
    /** Copies a registration. */
    registration(const registration&) = default;
    /**
     Copies a registration.

     @return This registration.
    */
    registration& operator=(const registration&) = default;
};

/**
 A registration whose lookup node lives in its owner's allocation.

 The bridge indexes it without allocating memory. Remove it before destruction,
 exactly as for registration; it may not be registered in two bridges at once.
*/
class intrusive_registration : public registration,
                               public boost::intrusive::avl_set_base_hook<
                                   boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
    friend class bridge;
    std::uint64_t bridge_key_ = 0;

protected:
    /** Constructs an unregistered node. */
    intrusive_registration() = default;
    /** Destroys an already removed node. */
    ~intrusive_registration() = default;

public:
    /** A registered node cannot be copied. */
    intrusive_registration(const intrusive_registration&) = delete;
    /** A registered node cannot be assigned. */
    intrusive_registration& operator=(const intrusive_registration&) = delete;
};

/**
 The io_context's bridge: the JavaScript functions through which browser events reach it, each
 delivering to one registration by a key that is never reused.

 A service of the io_context, made with its allocator by
 boost::asio::use_service<bridge>(io), and shut down and destroyed with it. On emscripten alone.

 @see "Browser events", in the guide.
*/
class bridge : public boost::asio::execution_context::service {
public:
    // Boost.Asio's key, by address, and mutable as Boost declares it. Its constructor is empty,
    // `id() {}`: it throws nothing and reads no other static, so that its initialization can
    // neither fail nor come too early.
    /** The service's identity, which Boost.Asio keys services by. */
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,bugprone-throwing-static-initialization,cert-err58-cpp,bugprone-dynamic-static-initializers)
    static inline boost::asio::execution_context::id id;

    /**
     Constructs the bridge of context, with nothing registered.

     @param context The io_context it serves.
    */
    explicit bridge(boost::asio::execution_context& context)
        : boost::asio::execution_context::service(context),
          registrations_(map::allocator_type(context)),
          token_(emscripten::val::object()) {
        token_.set("alive", true);
        token_.set("depth", 0);
    }

    /** Not copyable: the functions it made deliver to it by its address. */
    bridge(const bridge&) = delete;
    /** Not copy-assignable: the functions it made deliver to it by its address. */
    bridge& operator=(const bridge&) = delete;
    /** Not movable: the functions it made deliver to it by its address. */
    bridge(bridge&&) = delete;
    /** Not move-assignable: the functions it made deliver to it by its address. */
    bridge& operator=(bridge&&) = delete;

    /** Destroys the bridge: its functions deliver nothing from now on. */
    ~bridge() override { token_.set("alive", false); }

    /**
     Registers r, which must stay alive until it is removed, done, or shut down.

     @param r The registration.
     @return Its key: the first is 1, and none is ever given twice.
    */
    [[nodiscard]] std::uint64_t add(registration& r) {
        const std::uint64_t key = next_++;
        registrations_.emplace(key, &r);
        return key;
    }

    /**
     Registers an owner-allocated node without allocating lookup storage.
     @param r Unregistered node that remains alive until removal or shutdown.
     @return Its unique key, from the same sequence as ordinary registrations.
    */
    [[nodiscard]] std::uint64_t add(intrusive_registration& r) {
        r.bridge_key_ = next_++;
        intrusive_.insert(r);
        return r.bridge_key_;
    }

    /**
     A JavaScript function that delivers its argument to the registration key.

     Called once key is removed, once the io_context is destroyed, or once the runtime aborted,
     with Emscripten's ABORT flag set, it does nothing.

     @param key A key add returned.
     @return The function, to install as a listener or a promise's reaction.
    */
    [[nodiscard]] emscripten::val function(std::uint64_t key) {
        return emscripten::val::take_ownership(webcpp_asio_function(token_.as_handle(), this, key));
    }

    /**
     Removes the registration key, whose events find nothing from now on.

     @param key A key add returned; one already removed is ignored.
    */
    void remove(std::uint64_t key) noexcept {
        registrations_.erase(key);
        const auto found = intrusive_.find(key, node_less{});
        if (found != intrusive_.end()) {
            intrusive_.erase(found);
        }
    }

    /**
     Whether the bridge is delivering an event, which the run family refuses to run inside.

     Tip: the bridge's JavaScript functions count the deliveries in progress and put the count
     back however a delivery ends, a JavaScript error included, so that nothing in C++, which such
     an error may leave without running its destructors, keeps it.

     @return True while a function of the bridge delivers, an outer one around a nested one
     included.
    */
    [[nodiscard]] bool delivering() const { return token_["depth"].as<int>() != 0; }

private:
    // The one caller of deliver: the export the bridge's JavaScript functions call, which count
    // the deliveries in progress around it.
    friend void ::webcpp_asio_deliver(void* bridge, std::uint64_t key, emscripten::EM_VAL event);

    /** Delivers event to the registration key, and removes it when it is done. */
    void deliver(std::uint64_t key, const emscripten::val& event) {
        const auto found = registrations_.find(key);
        if (found == registrations_.end()) {
            const auto node = intrusive_.find(key, node_less{});
            if (node != intrusive_.end() && node->deliver(event)) {
                remove(key);
            }
            return;
        }
        // A registration that is done may have freed itself, and one added meanwhile may have
        // moved the map's entries: it is erased by its key.
        if (found->second->deliver(event)) {
            registrations_.erase(key);
        }
    }

    using map = boost::unordered_flat_map<
        std::uint64_t, registration*, boost::hash<std::uint64_t>, std::equal_to<>,
        boost::asio::execution_context::allocator<std::pair<const std::uint64_t, registration*>>>;

    struct node_less {
        bool operator()(const intrusive_registration& a, const intrusive_registration& b) const {
            return a.bridge_key_ < b.bridge_key_;
        }

        bool operator()(std::uint64_t a, const intrusive_registration& b) const {
            return a < b.bridge_key_;
        }

        bool operator()(const intrusive_registration& a, std::uint64_t b) const {
            return a.bridge_key_ < b;
        }
    };

    using nodes =
        boost::intrusive::avl_set<intrusive_registration, boost::intrusive::compare<node_less>,
                                  boost::intrusive::constant_time_size<false>>;

    /** Stops the functions and shuts down every registration still held, without calling it. */
    void shutdown() override {
        token_.set("alive", false);
        auto pending = std::move(registrations_);
        for (auto& [key, r] : pending) {
            r->shutdown();
        }
        while (!intrusive_.empty()) {
            auto& r = *intrusive_.begin();
            intrusive_.erase(intrusive_.begin());
            r.shutdown();
        }
    }

    map registrations_;
    nodes intrusive_;
    // {alive, depth}: whether the bridge exists, which every function of the bridge reads first,
    // and how many of its deliveries are in progress, which its functions put back however a
    // delivery ends and delivering() reads.
    emscripten::val token_;
    std::uint64_t next_ = 1;
};

}  // namespace webcpp::asio::browser

#endif  // WEBCPP_ASIO_BROWSER_BRIDGE_HPP
