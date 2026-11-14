# webcpp::webrtc

The W3C WebRTC data-channel API in Asio's model: peer connections and data channels whose
operations take completion tokens and run on the user's executor, written once and run natively
(over libdatachannel's C API) and, from increment 2, in the browser (over its own
`RTCPeerConnection` and `RTCDataChannel`). Media is out of scope.

```cpp
#include <webcpp/webrtc.hpp>

namespace webrtc = webcpp::webrtc;

webcpp::asio::io_context io;
webrtc::runtime rt;  // libdatachannel's threads, and the cap of connections
auto pc = webrtc::peer_connection::make(rt, io.get_executor(), webrtc::configuration{});
auto channel = pc->create_data_channel("chat");
pc->async_set_local_description([&](boost::system::error_code ec,
                                    webrtc::session_description offer) {
    // send offer.view() to the peer through your signalling
});
pc->async_wait_ice_candidate([&](boost::system::error_code ec, webrtc::ice_candidate c) {
    // trickle c.view() to the peer; an empty candidate ends gathering
});
```

A program uses `/webcpp/webrtc//webrtc`. Natively it brings the superproject's pinned
libdatachannel 0.24.6 (`/webcpp/third_party//libdatachannel`) with its OpenSSL, and needs
`threading=multi`; it works with exceptions on and off.

This tree is the reference code of the proposal `libraries/webrtc/openspec/changes/genesis` in
`webcpporg/proposals`: increment 1, the interface and the native implementation, with its tests.
Its design, its plan and where it stands are there.

Distributed under the Boost Software License, Version 1.0. libdatachannel is used unmodified
under the MPL-2.0, with its nested dependencies under their own licences.
