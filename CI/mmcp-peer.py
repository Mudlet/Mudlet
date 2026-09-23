#!/usr/bin/env python3
"""Scripted MMCP chat peer for Mudlet's busted networking specs.

Mudlet's mmcp.* Lua API only has observable effects when a chat peer is on the
other end of a socket, so the specs need a real peer rather than a mock. This is
that peer: it accepts the call Mudlet places with mmcp.call(), completes the
MudMaster handshake, records every protocol command Mudlet sends, and sends
commands back when the specs ask it to.

It lives in its own file rather than inside http-fixture-server.py because the
two share nothing: that one is a stock static file server, this one is a
stateful binary protocol peer. A single process would also mean one fixture
failing takes the other down with it.

Three channels, all inside the directory named by ``MUDLET_TEST_MMCP_DIR`` (one
environment variable, read by both this process and Mudlet):

  port          the OS-assigned listening port, written once the socket is
                accepting. Ephemeral rather than MMCP's default 4050: CI jobs
                and parallel local worktrees would collide on a fixed port.
  capture.json  everything this peer has seen, rewritten atomically after every
                change so a reader never gets a torn file.
  commands/     one JSON file per instruction from the specs, picked up in
                numeric order and deleted once carried out.

Only one call is held at a time: a new one replaces the old. Effects that need
two peers at once (mmcp.setPrivate's filtering, mmcp.serve's forwarding, a
non-empty connection or peek list) are out of reach until this grows a second
listening port.

Wire format, from src/MMCP.h and src/MMCPClient.cpp:

  Mudlet -> peer on connect:  "CHAT:<name>\\n<address><port padded to width 5>"
  peer -> Mudlet, accepting:  "YES:<name>\\n"   (or "NO:<name>\\n" to refuse)
  either direction after:     <command byte><payload><0xff>
"""

import json
import os
import selectors
import socket
import sys

# Command bytes, from the MMCPChatCommand enum in src/MMCP.h. Commands not
# listed here are still recorded, by their numeric code.
COMMAND_NAMES = {
    1: "NameChange",
    2: "RequestConnections",
    3: "ConnectionList",
    4: "TextEveryone",
    5: "TextPersonal",
    6: "TextGroup",
    7: "Message",
    8: "DoNotDisturb",
    19: "Version",
    26: "PingRequest",
    27: "PingResponse",
    28: "PeekConnections",
    29: "PeekList",
    30: "Snoop",
    31: "SnoopData",
    32: "SnoopColor",
    40: "SideChannel",
}

END = 0xFF
PING_REQUEST = 26
PING_RESPONSE = 27
VERSION = 19

# Mudlet only sends side channel data to peers whose version string says they
# are Mudlet (MMCPServer::sendSideChannel), so claim to be one.
PEER_VERSION = "Mudlet 0.0.0-busted-peer"
PEER_NAME = "BustedPeer"

# Enough history for a spec to look back over a few steps without the capture
# file growing without bound over a whole suite run.
MAX_EVENTS = 200
POLL_SECONDS = 0.01


class MMCPPeer:
    def __init__(self, directory):
        self.directory = directory
        self.commands_dir = os.path.join(directory, "commands")
        self.capture_path = os.path.join(directory, "capture.json")
        self.port_path = os.path.join(directory, "port")

        self.events = []
        self.seq = 0
        self.connections = 0
        self.caller = None
        self.name = PEER_NAME
        self.version = PEER_VERSION
        self.accept_calls = True

        self.selector = selectors.DefaultSelector()
        self.connection = None
        self.state = "idle"
        self.buffer = bytearray()

        self.listener = self.listen()
        self.port = self.listener.getsockname()[1]
        # A second address that only ever records who dialled it and hangs up.
        # It is how a spec sees that Mudlet acted on an address a peer handed it
        # (a connection list), which is otherwise reported to the console alone.
        self.sink = self.listen()
        self.dial_port = self.sink.getsockname()[1]

    @staticmethod
    def listen():
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("127.0.0.1", 0))
        server.listen(4)
        server.setblocking(False)
        return server

    # -- capture ------------------------------------------------------------

    def record(self, event):
        self.seq += 1
        event["seq"] = self.seq
        self.events.append(event)
        del self.events[:-MAX_EVENTS]
        self.write_capture()

    def write_capture(self):
        payload = {
            "seq": self.seq,
            "port": self.port,
            "dial_port": self.dial_port,
            "connections": self.connections,
            "connected": self.connection is not None,
            "accepting": self.accept_calls,
            "name": self.name,
            "version": self.version,
            "caller": self.caller,
            "events": self.events,
        }
        # Write then rename so a spec reading mid-update sees the old file
        # rather than half of the new one.
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
            # Only one call at a time: an earlier caller that never went away
            # would otherwise keep receiving what the specs meant for this one.
            self.close_connection("replaced by a new call")
        self.connection = connection
        self.selector.register(connection, selectors.EVENT_READ)
        self.state = "handshake"
        self.buffer.clear()
        self.caller = None
        self.connections += 1
        self.record({"type": "connect", "from": "%s:%d" % address})

    def accept_sink(self):
        connection, address = self.sink.accept()
        connection.close()
        self.record({"type": "dialled", "from": "%s:%d" % address})

    def close_connection(self, reason):
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
        self.state = "idle"
        self.buffer.clear()
        self.record({"type": "disconnect", "reason": reason})

    def send(self, data):
        if self.connection is None:
            self.record({"type": "send_failed", "reason": "no connection"})
            return False
        try:
            self.connection.sendall(data)
        except OSError as error:
            self.record({"type": "send_failed", "reason": str(error)})
            return False
        return True

    def send_command(self, code, payload):
        return self.send(bytes([code]) + payload + bytes([END]))

    def read(self):
        try:
            chunk = self.connection.recv(4096)
        except BlockingIOError:
            # BlockingIOError is an OSError, so it has to be let through before
            # the catch-all below turns a socket error into an end of call.
            return
        except OSError:
            chunk = b""
        if not chunk:
            self.close_connection("closed by Mudlet")
            return
        self.buffer.extend(chunk)
        if self.state == "handshake":
            self.handle_handshake()
        if self.state == "connected":
            self.handle_commands()

    def handle_handshake(self):
        newline = self.buffer.find(b"\n")
        if newline == -1:
            return
        if not self.buffer.startswith(b"CHAT:"):
            self.record({"type": "bad_handshake", "raw": self.buffer.decode("latin-1")})
            self.close_connection("handshake did not start with CHAT:")
            return
        # The address and port that follow the newline have no terminator of
        # their own; Mudlet writes them in the same call as the name and parses
        # an incoming call the same way, taking the last 5 bytes as the port.
        rest = bytes(self.buffer[newline + 1:])
        if len(rest) < 5:
            return

        caller_name = bytes(self.buffer[5:newline]).decode("latin-1")
        raw = bytes(self.buffer).decode("latin-1")
        self.buffer.clear()
        self.caller = {
            "name": caller_name,
            "address": rest[:-5].decode("latin-1"),
            "port": rest[-5:].decode("latin-1").strip(),
            "raw": raw,
        }
        self.record({"type": "handshake", "caller": self.caller})

        if not self.accept_calls:
            self.send(("NO:%s\n" % self.name).encode("latin-1"))
            self.close_connection("call refused")
            return

        self.state = "connected"
        # One write: Mudlet reads whatever has arrived when it handles the
        # acceptance, and only understands a command tacked onto the end of it
        # if the command is complete.
        self.send(("YES:%s\n" % self.name).encode("latin-1")
                  + bytes([VERSION]) + self.version.encode("latin-1") + bytes([END]))

    def handle_commands(self):
        while True:
            end = self.buffer.find(bytes([END]))
            if end == -1:
                return
            code = self.buffer[0]
            payload = bytes(self.buffer[1:end])
            del self.buffer[:end + 1]
            self.on_command(code, payload)

    def on_command(self, code, payload):
        self.record({
            "type": "command",
            "code": code,
            "name": COMMAND_NAMES.get(code, "Unknown"),
            "text": payload.decode("latin-1"),
            "hex": payload.hex(),
        })
        if code == PING_REQUEST:
            # What a real peer does, and what lets a spec watch a full ping
            # round trip rather than only the outgoing half.
            self.send_command(PING_RESPONSE, payload)

    # -- commands from the specs -------------------------------------------

    def poll_commands(self):
        try:
            names = os.listdir(self.commands_dir)
        except OSError:
            return
        queued = []
        for name in names:
            stem, extension = os.path.splitext(name)
            # Specs write "<n>.json.tmp" and rename it into place, so a partly
            # written command is never picked up.
            if extension == ".json" and stem.isdigit():
                queued.append((int(stem), name))
        for _, name in sorted(queued):
            path = os.path.join(self.commands_dir, name)
            try:
                with open(path, encoding="utf-8") as handle:
                    command = json.load(handle)
            except (OSError, ValueError) as error:
                command = None
                self.record({"type": "command_error", "file": name, "error": str(error)})
            try:
                os.remove(path)
            except OSError:
                pass
            if command is not None:
                self.run_command(command)

    def run_command(self, command):
        try:
            self.dispatch_command(command)
        except (OSError, TypeError, ValueError) as error:
            # A malformed command is one spec's problem. Dying over it would
            # leave every later spec waiting on a peer that is no longer there.
            self.record({"type": "command_error", "error": str(error)})

    def dispatch_command(self, command):
        action = command.get("action")
        if action == "send":
            text = command.get("text", "")
            self.send_command(int(command.get("code", 0)), text.encode("latin-1", "replace"))
        elif action == "send_hex":
            self.send(bytes.fromhex(command.get("hex", "")))
        elif action == "close":
            self.close_connection("closed on request")
        elif action == "accept":
            self.accept_calls = bool(command.get("accept", True))
        else:
            self.record({"type": "command_error", "error": "unknown action: %r" % (action,)})
            return
        self.record({"type": "command_done", "action": action})

    # -- main loop ----------------------------------------------------------

    def run(self):
        os.makedirs(self.commands_dir, exist_ok=True)
        self.selector.register(self.listener, selectors.EVENT_READ)
        self.selector.register(self.sink, selectors.EVENT_READ)
        self.write_capture()
        self.write_port()
        print("MMCP peer '%s' listening on 127.0.0.1:%d, sink on %d"
              % (self.name, self.port, self.dial_port), flush=True)
        while True:
            for key, _ in self.selector.select(POLL_SECONDS):
                if key.fileobj is self.listener:
                    self.accept()
                elif key.fileobj is self.sink:
                    self.accept_sink()
                elif key.fileobj is self.connection:
                    self.read()
            self.poll_commands()

    def write_port(self):
        # Written last, and atomically, so its presence means the peer is
        # already accepting connections.
        tmp_path = self.port_path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8") as handle:
            handle.write(str(self.port))
        os.replace(tmp_path, self.port_path)

    def forget_port(self):
        # The specs read the port file to decide whether there is a peer worth
        # talking to, so a peer that is going away has to take it with it:
        # otherwise every spec waits out its handshake timeout against a socket
        # nobody is listening on.
        try:
            os.remove(self.port_path)
        except OSError:
            pass


def main():
    directory = os.environ.get("MUDLET_TEST_MMCP_DIR")
    if not directory:
        print("MUDLET_TEST_MMCP_DIR is not set", file=sys.stderr)
        return 1
    os.makedirs(directory, exist_ok=True)
    peer = MMCPPeer(directory)
    try:
        peer.run()
    finally:
        peer.forget_port()
    return 0


if __name__ == "__main__":
    sys.exit(main())
