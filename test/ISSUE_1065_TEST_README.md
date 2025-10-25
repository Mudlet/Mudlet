# Test for Issue #1065: Off-by-one Error in cTelnet::processSocketData()

## Overview

This directory contains a unit test (`cTelnetBufferTest.cpp`) that validates the fix for the off-by-one buffer indexing error in `cTelnet::processSocketData()`.

**GitHub Issue:** https://github.com/Mudlet/Mudlet/issues/1065

## The Bug

The original code had two critical issues:

1. **Off-by-one error**: Placed null terminator at `in_buffer[amount + 1]` instead of `in_buffer[amount]`
2. **Premature buffer access**: Modified buffer before validating the `amount` parameter

### Original Buggy Code
```cpp
in_buffer[amount + 1] = '\0';  // ❌ Off-by-one AND accessed before validation
if (amount == -1) {
    return;
}
if (amount == 0) {
    return;
}
```

### Fixed Code
```cpp
if (amount == -1 || amount == 0) {
    in_buffer[0] = '\0';
    return;
}
in_buffer[amount] = '\0';  // ✓ Correct position
```

## Building and Running the Test

### Prerequisites
- Qt 6.8.2 or later
- CMake 3.25.1 or later
- C++20 compatible compiler

### Build Instructions

From the Mudlet root directory:

```bash
# Create and enter the test build directory
mkdir -p build/test
cd build/test

# Configure the build
cmake ../../test

# Build the test
cmake --build .

# Run the specific test for issue #1065
./cTelnetBufferTest
```

### Running with CTest

You can also run the test through CTest:

```bash
cd build/test
ctest -R cTelnetBufferTest -V
```

The `-V` flag provides verbose output showing all the test details.

## What the Test Validates

### 1. Buffer Termination (`testBufferTerminationCorrect`)

Tests that the null terminator is correctly placed for various buffer sizes:
- Normal case: 32 bytes
- Edge case: 1 byte
- Large buffer: 1000 bytes
- No data: amount = 0
- Read error: amount = -1

For each case, it verifies:
- Null terminator is at the correct index
- String length matches the amount read
- No premature null terminators in the data region

### 2. Buggy Behavior Demonstration (`testBuggyBehaviorWouldFail`)

Shows exactly what the buggy code would have done:
- Places null at `amount + 1` leaving `amount` with garbage data
- Compares buggy vs. fixed behavior side-by-side

### 3. Premature Access Prevention (`testPrematureAccessPrevention`)

Validates that error conditions are checked BEFORE buffer access:
- When `amount = -1` (socket error)
- When `amount = 0` (no data available)

Shows that the buggy code would access the buffer before validation.

## Expected Output

When run successfully, you should see output like:

```
********* Start testing of cTelnetBufferTest *********
Testing: Normal case: 32 bytes read
  ✓ Null terminator correctly placed at index 32
  ✓ String length (32) matches amount (32)
  ✓ No premature null terminators in data region [0,31]
PASS   : cTelnetBufferTest::testBufferTerminationCorrect(Normal 32 bytes)

=== Demonstrating what the BUGGY code would have done ===
Buggy code: Placed null terminator at index 33
  Problem: Index 32 still contains garbage: ?
Fixed code: Placed null terminator at index 32
  ✓ Index 32 correctly set to null terminator

Demonstration complete: Fix prevents off-by-one error
PASS   : cTelnetBufferTest::testBuggyBehaviorWouldFail()

...

Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted
********* Finished testing of cTelnetBufferTest *********
```

## Alternative: Python Demonstration

If you cannot build the C++ test (e.g., missing Qt dependencies), you can run the Python demonstration script instead:

```bash
python3 test_issue_1065.py
```

This script simulates the buffer handling logic and clearly demonstrates:
- The off-by-one error
- The premature buffer access issue
- How the fix resolves both problems

## Commit History

The fix was applied in commit that modified `src/ctelnet.cpp`:
- Reordered logic to check `amount` before buffer access
- Changed null terminator placement from `amount + 1` to `amount`
- Added early return for error cases with safe buffer state

## References

- Original issue: https://github.com/Mudlet/Mudlet/issues/1065
- Fixed in: `src/ctelnet.cpp` line 4020-4024
- Test code: `test/cTelnetBufferTest.cpp`
