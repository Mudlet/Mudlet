#!/usr/bin/env python3
"""Silent, recording game server for Mudlet's busted networking specs.

Mudlet only reaches its "connected" state by way of a real socket, and several
guards in the Lua API distinguish connected from disconnected. Specs run with
--offline, so without a server on the other end that branch is unreachable and
those guards can only ever be checked in their disconnected form. This fixture
is the other end.

Two properties make it useful, and both are deliberate:

  It never negotiates. It accepts the connection and then says nothing at all,
  so every telnet option Mudlet offers goes unanswered. MSDP, GMCP and the rest
  therefore stay disabled for as long as the connection lives. That is not a
  limitation to work around - it is the state under test. sysConnectionEvent is
  raised on TCP connect, before any negotiation has happened, so it is exactly
  what a package's connect handler sees, and a guard that demands a negotiated
  protocol there rejects work a real server would have accepted.

  It records. Everything Mudlet writes is kept as hex, so a spec can assert on
  the actual bytes rather than on a return value that merely claims they were
  sent.

Channels, all inside the directory named by ``MUDLET_TEST_TELNET_DIR`` (one
environment variable, read by both this process and the specs):

  port          the OS-assigned listening port, written once the socket is
                accepting. Ephemeral rather than a fixed one: CI jobs and
                parallel local worktrees would collide.
  capture.json  what this server has seen, rewritten atomically after every
                change so a reader never gets a torn file.

A spec reads ``received`` (hex, oldest first) and looks for its own bytes in the
tail past whatever length it noted beforehand. Connections are served one at a
time; a new one replaces the old and clears the buffer, with ``connections``
counting them so a spec can tell a reconnect from a stall.
"""

import json
import os
import selectors
import socket
import sys

# Enough for the negotiation Mudlet opens with plus anything a spec sends, while
# still bounding the capture file over a whole suite run.
MAX_CAPTURE_BYTES = 65536
POLL_SECONDS = 0.01


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
            # One connection at a time: an earlier one that never went away
            # would keep appending to the same buffer a later spec is reading.
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
            for key, _ in self.selector.select(POLL_SECONDS):
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
        # The specs read the port file to decide whether there is a server worth
        # connecting to, so one that is going away has to take it with it.
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
    server = TelnetServer(directory)
    try:
        server.run()
    finally:
        server.forget_port()
    return 0


if __name__ == "__main__":
    sys.exit(main())
