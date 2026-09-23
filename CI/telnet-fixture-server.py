#!/usr/bin/env python3
"""Silent, recording game server for Mudlet's busted networking specs.

Specs run with --offline, so the connected branch of the Lua API's send guards
is otherwise unreachable. This fixture is the other end of a real socket.

It never negotiates: it accepts and then says nothing, so every telnet option
Mudlet offers goes unanswered and MSDP, GMCP and the rest stay disabled for the
life of the connection. That is the state under test - sysConnectionEvent is
raised on TCP connect, which is what a package's connect handler sees.

Channels, inside the directory named by ``MUDLET_TEST_TELNET_DIR``:

  port          the listening port, written once the socket is accepting.
                Ephemeral: CI jobs and parallel worktrees would collide.
  capture.json  ``received`` is hex, cleared on each new connection; rewritten
                atomically so a reader never gets a torn file.

The kernel completes the handshake from the listen backlog before accept() runs,
so a connected client can be writing while ``received`` still holds the previous
connection's bytes. accept() clears it and bumps ``connections`` in the same
write, so a spec that notes the count before connecting can tell the two apart.
"""

import json
import os
import selectors
import signal
import socket
import sys

MAX_CAPTURE_BYTES = 65536


class TelnetServer:
    def __init__(self, directory):
        self.directory = directory
        self.capture_path = os.path.join(directory, "capture.json")
        self.port_path = os.path.join(directory, "port")

        self.received = bytearray()
        self.connections = 0
        self.peer = None

        self.selector = selectors.DefaultSelector()
        self.connection = None

        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(4)
        self.listener.setblocking(False)
        self.port = self.listener.getsockname()[1]

    # -- capture ------------------------------------------------------------

    def write_capture(self):
        payload = {
            "port": self.port,
            "connections": self.connections,
            "connected": self.connection is not None,
            "peer": self.peer,
            # Hex rather than raw text: this is a binary protocol, and JSON has
            # no way to carry bytes that are not valid UTF-8.
            "received": self.received.hex(),
            "bytes": len(self.received),
        }
        tmp_path = self.capture_path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8") as handle:
            json.dump(payload, handle)
        os.replace(tmp_path, self.capture_path)

    # -- connection ---------------------------------------------------------

    def accept(self):
        connection, address = self.listener.accept()
        connection.setblocking(False)
        connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        if self.connection is not None:
            # One at a time: a stale connection would keep appending to the
            # buffer a later spec is reading.
            self.close_connection()
        self.connection = connection
        self.selector.register(connection, selectors.EVENT_READ)
        self.received.clear()
        self.connections += 1
        self.peer = "%s:%d" % address
        self.write_capture()

    def close_connection(self):
        if self.connection is None:
            return
        try:
            self.selector.unregister(self.connection)
        except (KeyError, ValueError):
            pass
        try:
            self.connection.close()
        except OSError:
            pass
        self.connection = None
        self.write_capture()

    def read(self):
        try:
            chunk = self.connection.recv(4096)
        except BlockingIOError:
            # BlockingIOError is an OSError, so it has to be let through before
            # the catch-all below turns a spurious wakeup into a disconnect.
            return
        except OSError:
            chunk = b""
        if not chunk:
            self.close_connection()
            return
        self.received.extend(chunk)
        del self.received[:-MAX_CAPTURE_BYTES]
        self.write_capture()

    # -- main loop ----------------------------------------------------------

    def run(self):
        self.selector.register(self.listener, selectors.EVENT_READ)
        self.write_capture()
        self.write_port()
        print("telnet fixture listening on 127.0.0.1:%d" % self.port, flush=True)
        while True:
            for key, _ in self.selector.select():
                if key.fileobj is self.listener:
                    self.accept()
                elif key.fileobj is self.connection:
                    self.read()

    def write_port(self):
        # Written last, and atomically, so its presence means the server is
        # already accepting connections.
        tmp_path = self.port_path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8") as handle:
            handle.write(str(self.port))
        os.replace(tmp_path, self.port_path)

    def forget_port(self):
        # A port file outliving the server sends the specs at a dead socket, so
        # they fail with "never connected" instead of "no fixture running".
        try:
            os.remove(self.port_path)
        except OSError:
            pass


def main():
    directory = os.environ.get("MUDLET_TEST_TELNET_DIR")
    if not directory:
        print("MUDLET_TEST_TELNET_DIR is not set", file=sys.stderr)
        return 1
    os.makedirs(directory, exist_ok=True)
    # Both consumers stop the fixture with SIGTERM, whose default handler would
    # skip the cleanup below.
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
    server = TelnetServer(directory)
    try:
        server.run()
    finally:
        server.forget_port()
    return 0


if __name__ == "__main__":
    sys.exit(main())
