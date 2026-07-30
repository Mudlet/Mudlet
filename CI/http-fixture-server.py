#!/usr/bin/env python3
"""Minimal fixture HTTP server for Mudlet's busted networking specs.

Serves the sibling ``http-fixtures/`` directory over localhost so the Lua test
suite can exercise getHTTP/downloadFile against a real, local endpoint instead
of the public internet.

An OS-assigned (ephemeral) port is used rather than a fixed one: Mudlet CI may
run several jobs on the same machine, and a hard-coded port would risk
collisions there. The chosen port is written to the file named by the
``MUDLET_TEST_HTTP_PORT_FILE`` environment variable so the launching CI step can
forward it to Mudlet as ``MUDLET_TEST_HTTP_PORT``.
"""

import http.server
import os
import socketserver

FIXTURES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "http-fixtures")


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FIXTURES_DIR, **kwargs)

    def log_message(self, *args):
        # Keep CI logs quiet; the tests assert on effects, not on server chatter.
        pass


def main():
    with socketserver.TCPServer(("127.0.0.1", 0), QuietHandler) as httpd:
        port = httpd.server_address[1]
        port_file = os.environ.get("MUDLET_TEST_HTTP_PORT_FILE")
        if port_file:
            # Write then rename so the launcher never reads a torn/empty port.
            tmp_file = port_file + ".tmp"
            with open(tmp_file, "w", encoding="utf-8") as handle:
                handle.write(str(port))
            os.replace(tmp_file, port_file)
        print(f"Serving Mudlet test fixtures on http://127.0.0.1:{port}", flush=True)
        httpd.serve_forever()


if __name__ == "__main__":
    main()
