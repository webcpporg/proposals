# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Real HTTP responses must never become visible before their request observation."""

import http.client
import socket
import threading
import unittest

import driver


class PausedSendSocket(socket.socket):
    def __init__(self, peer, server):
        super().__init__(fileno=peer.detach())
        self.server = server

    def sendall(self, data, flags=0):
        super().sendall(data, flags)
        headers = data.startswith(b"HTTP/")
        if headers == self.server.head:
            # The client can consume the final response bytes while the handler
            # is still inside sendall. No timing assumption controls this barrier.
            self.server.sent.set()
            if not self.server.release.wait(5):
                raise TimeoutError("test did not release the completed response write")


class PausedTcpPeer(driver.TcpPeer):
    def __init__(self, head):
        self.head = head
        self.sent = threading.Event()
        self.release = threading.Event()
        super().__init__(("127.0.0.1", 0))

    def get_request(self):
        peer, address = super().get_request()
        return PausedSendSocket(peer, self), address


class PeerObservationTest(unittest.TestCase):
    def test_completed_response_has_recorded_request(self):
        for method, body in (("GET", None), ("HEAD", None), ("POST", b"request body")):
            with self.subTest(method=method), PausedTcpPeer(method == "HEAD") as peer:
                worker = threading.Thread(target=peer.serve_forever, daemon=True)
                worker.start()
                connection = http.client.HTTPConnection("127.0.0.1", peer.socket.getsockname()[1],
                                                        timeout=3)
                try:
                    connection.request(method, "/observed", body=body,
                                       headers={"Connection": "close"})
                    response = connection.getresponse()
                    received = response.read()
                    self.assertEqual(response.status, 200)
                    expected = b"" if method == "HEAD" else body or driver.PAYLOAD
                    self.assertEqual(received, expected)
                    self.assertTrue(peer.sent.wait(3), "final sendall did not reach the barrier")
                    self.assertEqual(peer.exchanges, [{"method": method, "target": "/observed",
                                                       "body": body.decode() if body else ""}])
                finally:
                    peer.release.set()
                    connection.close()
                    peer.shutdown()
                    worker.join(timeout=3)
                    self.assertFalse(worker.is_alive(), "HTTP fixture worker did not stop")


if __name__ == "__main__":
    unittest.main()
