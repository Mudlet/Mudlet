#!/usr/bin/env python3
"""Minimal fixture HTTP server for Mudlet's busted networking specs.

Serves the sibling ``http-fixtures/`` directory over localhost so the Lua test
suite can exercise getHTTP/downloadFile against a real, local endpoint instead
of the public internet.

Requests below ``/echo`` are answered by an echo endpoint instead of from disk:
it accepts GET and every verb this handler has no method of its own for
(postHTTP/putHTTP/deleteHTTP/customHTTP all need one) and reports the method,
path, request headers and body it received back in the response body, which is
what lets a spec prove that what Mudlet put on the wire is what the caller
asked for. HEAD is the one exception: it keeps serving files from disk.

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

ECHO_PATH = "/echo"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FIXTURES_DIR, **kwargs)

    def log_message(self, *args):
        # Keep CI logs quiet; the tests assert on effects, not on server chatter.
        pass

    def end_headers(self):
        # Both are sent on every response, including the static file ones, so a
        # spec can assert that the response headers and cookies tables Mudlet
        # builds reach Lua.
        self.send_header("X-Mudlet-Fixture", "1")
        self.send_header("Set-Cookie", "mudlet-fixture=1; Path=/")
        super().end_headers()

    def do_GET(self):
        if self.echo_requested():
            self.echo()
            return
        super().do_GET()

    def __getattr__(self, name):
        # BaseHTTPRequestHandler dispatches "VERB /path" to a do_VERB method and
        # answers 501 when there is none. Only GET and HEAD have one, so every
        # other verb - POST/PUT/DELETE plus whatever customHTTP() invents - is
        # routed here and handled by the echo endpoint. Matching against the
        # verb being dispatched keeps a mistyped attribute elsewhere in this
        # class an AttributeError instead of silently becoming an echo.
        # __dict__ rather than self.command: the attribute only exists once a
        # request line has been parsed, and reading it through the instance
        # would come straight back here.
        if name.startswith("do_") and name == "do_%s" % self.__dict__.get("command"):
            return self.echo
        raise AttributeError(name)

    def echo_requested(self):
        return self.path == ECHO_PATH or self.path.startswith(ECHO_PATH + "/") or self.path.startswith(ECHO_PATH + "?")

    def echo(self):
        if not self.echo_requested():
            self.send_error(404, "Not Found", "only %s answers this method" % ECHO_PATH)
            return

        try:
            length = int(self.headers.get("Content-Length") or 0)
        except ValueError:
            length = 0
        body = self.rfile.read(length) if length > 0 else b""

        lines = ["method=%s" % self.command, "path=%s" % self.path]
        for name, value in self.headers.items():
            lines.append("header:%s=%s" % (name.lower(), value))
        # Body last: it is the only part that may itself contain newlines.
        lines.append("body=%s" % body.decode("utf-8", "replace"))
        payload = "\n".join(lines).encode("utf-8")

        self.send_response(200)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


def main():
    # Single-threaded on purpose: the specs issue one request at a time, and
    # HTTP/1.0 (the default here) closes each connection, so no request can
    # block another.
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
