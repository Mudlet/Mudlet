#!/usr/bin/env python3
"""
Test server for Mudlet issue #1065: Off-by-one error in cTelnet::processSocketData()

This is a telnet server that Mudlet can connect to. It sends specific data patterns
designed to expose the off-by-one buffer handling bug if it still existed.

GitHub Issue: https://github.com/Mudlet/Mudlet/issues/1065

USAGE:
    1. Run this script:
       python3 test_issue_1065_server.py

    2. Launch Mudlet and create a new profile with:
       Server: localhost
       Port: 4000

    3. Connect and observe the output in Mudlet's main window

    4. The server will send test messages and report results
"""

import socket
import threading
import time
import sys

# Test configuration
HOST = 'localhost'
PORT = 4000

# Telnet protocol constants
IAC = bytes([255])   # Interpret As Command
WILL = bytes([251])
WONT = bytes([252])
DO = bytes([253])
DONT = bytes([254])
GA = bytes([249])    # Go Ahead
ECHO = bytes([1])
SUPPRESS_GA = bytes([3])


class TelnetTestServer:
    """Telnet server that tests buffer handling in Mudlet"""

    def __init__(self, host=HOST, port=PORT):
        self.host = host
        self.port = port
        self.server_socket = None
        self.running = False

    def start(self):
        """Start the telnet server"""
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(1)
            self.running = True

            print("=" * 70)
            print("Mudlet Issue #1065 Test Server")
            print("=" * 70)
            print(f"Listening on {self.host}:{self.port}")
            print("\nWaiting for Mudlet to connect...")
            print("\nIn Mudlet:")
            print("  1. Create a new profile")
            print(f"  2. Server: {self.host}")
            print(f"  3. Port: {self.port}")
            print("  4. Connect")
            print("\n" + "=" * 70 + "\n")

            while self.running:
                try:
                    client_socket, address = self.server_socket.accept()
                    print(f"\n✓ Mudlet connected from {address}")

                    # Handle client in a separate thread
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, address)
                    )
                    client_thread.daemon = True
                    client_thread.start()

                except KeyboardInterrupt:
                    print("\n\nShutting down server...")
                    break
                except Exception as e:
                    if self.running:
                        print(f"Error accepting connection: {e}")

        finally:
            self.running = False
            if self.server_socket:
                self.server_socket.close()

    def handle_client(self, client_socket, address):
        """Handle connected Mudlet client"""
        try:
            print("\nStarting buffer handling tests...\n")

            # Send telnet negotiations
            self.send_telnet_negotiation(client_socket)
            time.sleep(0.1)

            # Run test suite
            self.run_test_suite(client_socket)

            # Keep connection alive for observation
            print("\n" + "=" * 70)
            print("Tests complete! Connection will remain open.")
            print("Check Mudlet output for test results.")
            print("Press Ctrl+C to shutdown server.")
            print("=" * 70 + "\n")

            # Keep connection alive
            while self.running:
                time.sleep(1)

        except Exception as e:
            print(f"Error handling client: {e}")
        finally:
            try:
                client_socket.close()
                print(f"\nClient {address} disconnected")
            except OSError as e:
                # Socket already closed or other socket error
                print(f"Note: Error closing socket: {e}")

    def send_telnet_negotiation(self, client_socket):
        """Send initial telnet protocol negotiations"""
        # Negotiate telnet options
        negotiations = [
            IAC + WILL + SUPPRESS_GA,
            IAC + WILL + ECHO,
        ]

        for neg in negotiations:
            client_socket.send(neg)

        print("✓ Sent telnet negotiations")

    def send_message(self, client_socket, message):
        """Send a message to the client"""
        if isinstance(message, str):
            message = message.encode('utf-8')
        client_socket.send(message)

    def send_line(self, client_socket, line):
        """Send a line of text with newline"""
        self.send_message(client_socket, line + "\r\n")

    def run_test_suite(self, client_socket):
        """Run the complete test suite"""

        # Welcome message
        self.send_line(client_socket, "=" * 70)
        self.send_line(client_socket, "Mudlet Issue #1065 Test Suite")
        self.send_line(client_socket, "Testing: Off-by-one error in buffer handling")
        self.send_line(client_socket, "=" * 70)
        self.send_line(client_socket, "")
        time.sleep(0.5)

        # Test 1: Boundary test with specific sizes
        self.test_buffer_boundaries(client_socket)
        time.sleep(0.5)

        # Test 2: Marker byte test
        self.test_marker_bytes(client_socket)
        time.sleep(0.5)

        # Test 3: Pattern recognition test
        self.test_pattern_recognition(client_socket)
        time.sleep(0.5)

        # Test 4: Null byte handling
        self.test_null_byte_handling(client_socket)
        time.sleep(0.5)

        # Final message
        self.send_line(client_socket, "")
        self.send_line(client_socket, "=" * 70)
        self.send_line(client_socket, "ALL TESTS COMPLETE")
        self.send_line(client_socket, "=" * 70)
        self.send_line(client_socket, "")
        self.send_line(client_socket, "If you see all test messages correctly above,")
        self.send_line(client_socket, "then issue #1065 is FIXED!")
        self.send_line(client_socket, "")

    def test_buffer_boundaries(self, client_socket):
        """Test various buffer sizes around critical boundaries"""
        print("Running Test 1: Buffer Boundary Test")

        self.send_line(client_socket, "TEST 1: Buffer Boundary Test")
        self.send_line(client_socket, "-" * 70)

        # Test various sizes that might trigger the off-by-one error
        test_sizes = [
            (31, "31 bytes (just before boundary)"),
            (32, "32 bytes (common boundary)"),
            (33, "33 bytes (just after boundary)"),
            (63, "63 bytes"),
            (64, "64 bytes"),
            (127, "127 bytes"),
            (128, "128 bytes"),
        ]

        for size, description in test_sizes:
            # Create a message with exact size (minus the CRLF)
            padding_needed = size - len(description) - 4  # -4 for "[] " and space
            if padding_needed > 0:
                message = f"[{description}] " + "X" * padding_needed
            else:
                message = f"[{description}]"

            self.send_line(client_socket, message)
            print(f"  Sent: {description}")
            time.sleep(0.2)

        self.send_line(client_socket, "")
        print("  ✓ Test 1 complete\n")

    def test_marker_bytes(self, client_socket):
        """Send messages with marker bytes to detect garbage data processing"""
        print("Running Test 2: Marker Byte Test")

        self.send_line(client_socket, "TEST 2: Marker Byte Test")
        self.send_line(client_socket, "-" * 70)
        self.send_line(client_socket, "Messages below should end with '|END' marker")
        self.send_line(client_socket, "If you see garbage after |END, the bug exists!")
        self.send_line(client_socket, "")

        # Send messages of exact lengths with clear end markers
        # If the bug exists, the byte at position [amount] would be garbage
        # and might appear after our intended end marker

        for size in [32, 64, 128]:
            marker = "|END"
            padding_size = size - len(f"Test message {size} bytes ") - len(marker)
            message = f"Test message {size} bytes " + "." * padding_size + marker

            self.send_line(client_socket, message)
            print(f"  Sent: {size} byte message with end marker")
            time.sleep(0.2)

        self.send_line(client_socket, "")
        print("  ✓ Test 2 complete\n")

    def test_pattern_recognition(self, client_socket):
        """Send repeating patterns to check for truncation issues"""
        print("Running Test 3: Pattern Recognition Test")

        self.send_line(client_socket, "TEST 3: Pattern Recognition Test")
        self.send_line(client_socket, "-" * 70)

        # Repeating pattern that should be clearly visible
        patterns = [
            ("0123456789" * 3 + "|OK", "30-char repeating digit pattern"),
            ("ABCDEFGHIJ" * 3 + "|OK", "30-char repeating alpha pattern"),
            ("0123456789" * 6 + "|OK", "60-char repeating digit pattern"),
        ]

        for pattern, description in patterns:
            self.send_line(client_socket, f"{pattern}")
            print(f"  Sent: {description}")
            time.sleep(0.2)

        self.send_line(client_socket, "")
        print("  ✓ Test 3 complete\n")

    def test_null_byte_handling(self, client_socket):
        """Test how null terminator placement affects data processing"""
        print("Running Test 4: Null Byte Handling Test")

        self.send_line(client_socket, "TEST 4: Null Terminator Handling")
        self.send_line(client_socket, "-" * 70)
        self.send_line(client_socket, "Testing correct null terminator placement...")
        self.send_line(client_socket, "")

        # Send carefully crafted messages
        # With the bug, the null terminator would be at wrong position
        # potentially causing the next byte to be included in processing

        test_messages = [
            "Message_001:ABCDEFGHIJKLMNOPQRST",
            "Message_002:0123456789ABCDEFGHIJ",
            "Message_003:TestDataWithNoSpaces!",
        ]

        for msg in test_messages:
            # Pad to specific size
            padded = msg + "_" * (40 - len(msg)) + "|"
            self.send_line(client_socket, padded)
            print(f"  Sent: {msg[:30]}...")
            time.sleep(0.2)

        self.send_line(client_socket, "")
        self.send_line(client_socket, "If all messages above display correctly,")
        self.send_line(client_socket, "null terminator placement is correct!")
        self.send_line(client_socket, "")
        print("  ✓ Test 4 complete\n")


def main():
    """Main entry point"""
    server = TelnetTestServer()

    try:
        server.start()
    except KeyboardInterrupt:
        print("\n\nServer stopped by user")
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
