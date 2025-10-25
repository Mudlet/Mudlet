#!/usr/bin/env python3
"""
Demonstration script for Mudlet issue #1065: Off-by-one error in cTelnet::processSocketData()

This script simulates the buffer handling logic to demonstrate:
1. The off-by-one error in the original code
2. The premature buffer access before error checking
3. The correct behavior after the fix

GitHub Issue: https://github.com/Mudlet/Mudlet/issues/1065
"""

import sys


class BufferSimulator:
    """Simulates C char array behavior"""

    def __init__(self, size):
        self.size = size
        # Initialize with recognizable garbage data (simulate uninitialized memory)
        self.data = [ord('?')] * size

    def __setitem__(self, index, value):
        if index < 0 or index >= self.size:
            raise IndexError(f"Buffer overflow! Attempted to write at index {index} (buffer size: {self.size})")
        if isinstance(value, str):
            value = ord(value)
        self.data[index] = value

    def __getitem__(self, index):
        if index < 0 or index >= self.size:
            raise IndexError(f"Buffer overflow! Attempted to read from index {index} (buffer size: {self.size})")
        return self.data[index]

    def get_string(self, max_len=None):
        """Extract null-terminated string from buffer"""
        result = []
        for i in range(max_len or self.size):
            if self.data[i] == 0:
                break
            result.append(chr(self.data[i]))
        return ''.join(result)

    def show_region(self, start, end):
        """Show buffer contents in a region"""
        output = []
        for i in range(start, end):
            if self.data[i] == 0:
                output.append("\\0")
            elif self.data[i] == ord('?'):
                output.append("??")
            else:
                output.append(chr(self.data[i]))
        return f"[{', '.join(output)}]"


def simulate_buggy_code(amount):
    """Simulates the BUGGY original code from issue #1065"""
    print(f"\n{'='*70}")
    print(f"BUGGY CODE: amount = {amount}")
    print('='*70)

    BUFFER_SIZE = 100000
    in_buffer = BufferSimulator(BUFFER_SIZE + 10)

    # Simulate reading some data
    test_data = "Hello, World!Testing123"
    if amount > 0:
        for i in range(min(amount, len(test_data))):
            in_buffer[i] = test_data[i]

    print(f"1. Read {amount} bytes from socket")
    print(f"2. Buffer contains data at indices 0-{amount-1 if amount > 0 else 'N/A'}")

    try:
        # THE BUG: Writing null terminator BEFORE checking amount
        print(f"3. Writing null terminator at index {amount + 1}...")
        in_buffer[amount + 1] = '\0'  # Off-by-one AND premature access!

        print(f"   ❌ Problem: Left index {amount} uninitialized!")
        if amount > 0:
            print(f"   Buffer[{amount-2}:{amount+3}] = {in_buffer.show_region(max(0, amount-2), min(BUFFER_SIZE+10, amount+3))}")

        # Error checking happens AFTER buffer modification
        if amount == -1:
            print(f"4. Detected error (amount == -1), returning...")
            return
        if amount == 0:
            print(f"4. No data available (amount == 0), returning...")
            return

        print(f"4. Processing continues with corrupted buffer state")

    except IndexError as e:
        print(f"   ❌ BUFFER OVERFLOW: {e}")
        return


def simulate_fixed_code(amount):
    """Simulates the FIXED code"""
    print(f"\n{'='*70}")
    print(f"FIXED CODE: amount = {amount}")
    print('='*70)

    BUFFER_SIZE = 100000
    in_buffer = BufferSimulator(BUFFER_SIZE + 10)

    # Simulate reading some data
    test_data = "Hello, World!Testing123"
    if amount > 0:
        for i in range(min(amount, len(test_data))):
            in_buffer[i] = test_data[i]

    print(f"1. Read {amount} bytes from socket")
    print(f"2. Buffer contains data at indices 0-{amount-1 if amount > 0 else 'N/A'}")

    try:
        # THE FIX: Check for errors FIRST
        print(f"3. Checking amount value...")
        if amount == -1 or amount == 0:
            print(f"   ✓ Detected error/no-data (amount == {amount})")
            print(f"   ✓ Setting null terminator at index 0 for safety")
            in_buffer[0] = '\0'
            print(f"   ✓ Returning early")
            return

        # Only write null terminator for valid data
        print(f"4. Writing null terminator at index {amount}...")
        in_buffer[amount] = '\0'
        print(f"   ✓ Correct! Null terminator placed right after valid data")
        if amount > 0:
            print(f"   Buffer[{amount-2}:{amount+3}] = {in_buffer.show_region(max(0, amount-2), min(BUFFER_SIZE+10, amount+3))}")
            extracted = in_buffer.get_string(amount + 5)
            print(f"   Extracted string: \"{extracted}\"")

        print(f"5. Processing continues safely")

    except IndexError as e:
        print(f"   ❌ BUFFER OVERFLOW: {e}")
        return


def main():
    print("=" * 70)
    print("Issue #1065: Off-by-one error in cTelnet::processSocketData()")
    print("=" * 70)

    test_cases = [
        (32, "Normal case: 32 bytes read"),
        (0, "Edge case: No data available"),
        (-1, "Error case: Socket read error"),
    ]

    for amount, description in test_cases:
        print(f"\n\n{'#' * 70}")
        print(f"# Test Case: {description}")
        print(f"{'#' * 70}")

        simulate_buggy_code(amount)
        simulate_fixed_code(amount)

    print("\n\n" + "=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print("""
The buggy code has TWO critical issues:

1. OFF-BY-ONE ERROR:
   - Writes null terminator at index (amount + 1) instead of (amount)
   - Leaves index (amount) with uninitialized garbage data
   - For 32 bytes read, valid data is at [0-31], null should be at [32]
   - But buggy code puts it at [33], leaving [32] uninitialized

2. PREMATURE BUFFER ACCESS:
   - Accesses buffer BEFORE checking if amount is valid
   - When amount == -1, writes to in_buffer[0] (by luck, safe but wrong)
   - When amount == 0, writes to in_buffer[1] (wrong, should return early)
   - Error checking comes AFTER the buffer modification

The fix addresses both issues:
   ✓ Checks amount value BEFORE any buffer access
   ✓ Returns early for error cases (amount <= 0)
   ✓ Places null terminator at correct index (amount, not amount+1)
""")


if __name__ == "__main__":
    main()
