#!/usr/bin/env python3
"""Fake Discord IPC server for Mudlet's Lua self-tests.

Speaks enough of the discord-rpc wire protocol that the bundled
libdiscord-rpc library believes a Discord client is running, reports a
logged-in user, and accepts rich presence updates. Every frame the library
sends is appended to a capture file so the Lua specs can assert on the
SET_ACTIVITY payload that actually reached "Discord", rather than on the
return value of the setter that produced it.

Wire framing: [opcode:uint32 LE][length:uint32 LE][json payload]
  opcode 0 = HANDSHAKE (client -> server)
  opcode 1 = FRAME     (both directions)
  opcode 2 = CLOSE
  opcode 3 = PING
  opcode 4 = PONG

Capture file format: one JSON object per line,
  {"op": <opcode>, "payload": <parsed frame> | {"raw": "<undecodable text>"}}
Records are appended under a lock to an O_APPEND descriptor so a spec reading
the file concurrently never sees a half-written or spliced line.

The C++ equivalent used by TDiscordModeTest is
test/functional_tests/DiscordIpcServerStub.cpp - keep the two in step.

Usage:
  discord-ipc-fixture.py --ready-file <path> [--runtime-dir <dir>]
                         [--capture-file <path>] [--username <name>]

It prints (and, with --ready-file, writes) shell-style KEY=VALUE lines naming
the runtime directory and capture file once it is listening. Point
XDG_RUNTIME_DIR at the former before Mudlet starts: discord-rpc's reconnect
backoff is process-global and survives Discord_Shutdown, so a server that only
appears after the first failed connection attempt can cost up to ~120s.
"""

import argparse
import json
import os
import socket
import struct
import sys
import tempfile
import threading

SOCKET_FILE_NAME = "discord-ipc-0"

OP_HANDSHAKE = 0
OP_FRAME = 1
OP_CLOSE = 2
OP_PING = 3
OP_PONG = 4

# discord-rpc's own send buffer is 16KB, so anything near this is nonsense:
MAXIMUM_FRAME_BYTES = 1024 * 1024


def ready_payload(username):
    return {
        "cmd": "DISPATCH",
        "evt": "READY",
        "data": {
            "v": 1,
            "config": {
                "cdn_host": "cdn.discordapp.com",
                "api_endpoint": "//discord.com/api",
                "environment": "production",
            },
            "user": {
                "id": "111111111111111111",
                "username": username,
                "discriminator": "0",
                "global_name": username,
                "avatar": None,
                "bot": False,
                "flags": 0,
                "premium_type": 0,
            },
        },
    }


class CaptureLog:
    def __init__(self, path):
        self._fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o644)
        self._lock = threading.Lock()

    def append(self, opcode, payload_bytes):
        try:
            payload = json.loads(payload_bytes.decode("utf-8"))
        except (UnicodeDecodeError, ValueError):
            payload = {"raw": payload_bytes.decode("utf-8", "replace")}
        line = (json.dumps({"op": opcode, "payload": payload}, sort_keys=True) + "\n").encode("utf-8")
        # os.write() may write less than it was given, and discord-rpc's
        # reconnects overlap connections, so two serve_connection() threads can
        # be here at once. Without the lock one record's remainder could land
        # after another record's first write, splicing both into one malformed
        # line that framesAfter() would silently drop.
        with self._lock:
            while line:
                line = line[os.write(self._fd, line):]


def frame(opcode, payload_bytes):
    return struct.pack("<II", opcode, len(payload_bytes)) + payload_bytes


def read_exact(conn, count):
    buf = b""
    while len(buf) < count:
        chunk = conn.recv(count - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf


def serve_connection(conn, capture, username):
    # discord-rpc drops the socket without a CLOSE frame when it shuts down or
    # switches application ID, so an aborted connection is routine here.
    with conn:
        try:
            while True:
                header = read_exact(conn, 8)
                if not header:
                    return
                opcode, length = struct.unpack("<II", header)
                if length > MAXIMUM_FRAME_BYTES:
                    # A desynchronised stream would otherwise have us block on a
                    # length that never arrives, with the specs polling a capture
                    # file that has quietly stopped growing.
                    sys.stdout.write("discord-ipc-fixture: refusing a %d byte frame, closing the connection\n" % length)
                    sys.stdout.flush()
                    return
                payload = read_exact(conn, length) if length else b""
                if payload is None:
                    return
                capture.append(opcode, payload)
                if opcode == OP_HANDSHAKE:
                    conn.sendall(frame(OP_FRAME, json.dumps(ready_payload(username)).encode("utf-8")))
                elif opcode == OP_PING:
                    conn.sendall(frame(OP_PONG, payload))
                elif opcode == OP_CLOSE:
                    return
                # opcode 1 (FRAME, e.g. SET_ACTIVITY) needs no reply - the real
                # Discord client answers it, but discord-rpc ignores the answer.
        except OSError as error:
            sys.stdout.write("discord-ipc-fixture: connection ended: %s\n" % error)
            sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(description="Fake Discord IPC server for Mudlet's Lua self-tests")
    parser.add_argument("--runtime-dir", help="directory to create the discord-ipc-0 socket in (default: a fresh short temporary directory)")
    parser.add_argument("--capture-file", help="file to append captured frames to (default: discord-frames.jsonl inside the runtime directory)")
    parser.add_argument("--username", default="MudletSelfTest", help="username the READY dispatch reports as logged in")
    parser.add_argument("--ready-file", help="file to write the KEY=VALUE handover lines to once listening")
    args = parser.parse_args()

    runtime_dir = args.runtime_dir
    if runtime_dir:
        os.makedirs(runtime_dir, exist_ok=True)
        # Qt refuses to use an XDG_RUNTIME_DIR that anyone else can read, and
        # falls back with a warning:
        os.chmod(runtime_dir, 0o700)
    else:
        # Deliberately short: sizeof(sockaddr_un::sun_path) is 108 on Linux and
        # 104 on macOS, and the whole socket path has to fit.
        runtime_dir = tempfile.mkdtemp(prefix="mdxdg-")

    socket_path = os.path.join(runtime_dir, SOCKET_FILE_NAME)
    if len(socket_path) >= 100:
        sys.stderr.write("discord-ipc-fixture: socket path %s is too long for AF_UNIX\n" % socket_path)
        return 1

    capture_file = args.capture_file or os.path.join(runtime_dir, "discord-frames.jsonl")
    # Truncate any capture left over from an earlier run so the specs never see
    # frames from a previous suite:
    with open(capture_file, "w"):
        pass
    capture = CaptureLog(capture_file)

    if os.path.exists(socket_path):
        os.unlink(socket_path)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(socket_path)
    server.listen(8)

    handover = "MUDLET_TEST_DISCORD_RUNTIME_DIR=%s\nMUDLET_TEST_DISCORD_CAPTURE_FILE=%s\n" % (runtime_dir, capture_file)
    if args.ready_file:
        with open(args.ready_file, "w") as handle:
            handle.write(handover)
    sys.stdout.write(handover)
    sys.stdout.write("discord-ipc-fixture: listening on %s as Discord user %s\n" % (socket_path, args.username))
    sys.stdout.flush()

    while True:
        try:
            conn, _ = server.accept()
        except OSError as error:
            # Staying up matters: every spec still to run would otherwise wait
            # out its timeout against a capture file nobody is writing to.
            sys.stdout.write("discord-ipc-fixture: accept failed: %s\n" % error)
            sys.stdout.flush()
            continue
        threading.Thread(target=serve_connection, args=(conn, capture, args.username), daemon=True).start()


if __name__ == "__main__":
    sys.exit(main())
