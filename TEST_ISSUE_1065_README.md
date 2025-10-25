# Testing Issue #1065 Fix with Actual Mudlet Instance

This directory contains a Python telnet server that allows you to test the fix for issue #1065 with a real running instance of Mudlet.

**GitHub Issue:** https://github.com/Mudlet/Mudlet/issues/1065

## The Bug

The original code in `cTelnet::processSocketData()` had two critical issues:

1. **Off-by-one error**: Placed null terminator at `in_buffer[amount + 1]` instead of `in_buffer[amount]`
2. **Premature buffer access**: Modified buffer before validating the `amount` parameter

This could cause:
- Garbage bytes at position `[amount]` to be processed as data
- String operations to read uninitialized memory
- Potential crashes or data corruption in edge cases

## Testing Approach

Instead of simulating the bug, this test uses a real Python telnet server that Mudlet connects to. The server sends carefully crafted data patterns designed to expose the bug if it still existed:

- Messages at critical buffer boundaries (31, 32, 33, 64, 128 bytes)
- Marker bytes to detect if garbage data is being processed
- Repeating patterns to verify correct truncation
- Null terminator placement validation

## Prerequisites

- Python 3.x (standard library only, no external dependencies)
- Mudlet (the version you want to test)
- Both running on the same machine (or adjust HOST if remote)

## Step-by-Step Testing Instructions

### 1. Start the Test Server

```bash
cd /path/to/Mudlet
python3 test_issue_1065_server.py
```

You should see:

```
======================================================================
Mudlet Issue #1065 Test Server
======================================================================
Listening on localhost:4000

Waiting for Mudlet to connect...
```

### 2. Launch Mudlet

Start Mudlet application.

### 3. Create Test Profile

In Mudlet:
1. Click "New" to create a new profile
2. Enter profile details:
   - **Profile name**: Issue1065Test (or any name you like)
   - **Server address**: `localhost`
   - **Port**: `4000`
3. Click "Connect"

### 4. Observe the Test Output

Once connected, the server will automatically run a test suite sending various data patterns. You should see output in Mudlet's main window:

```
======================================================================
Mudlet Issue #1065 Test Suite
Testing: Off-by-one error in buffer handling
======================================================================

TEST 1: Buffer Boundary Test
----------------------------------------------------------------------
[31 bytes (just before boundary)] XXXX...
[32 bytes (common boundary)] XXXX...
[33 bytes (just after boundary)] XXXX...
...

TEST 2: Marker Byte Test
----------------------------------------------------------------------
Messages below should end with '|END' marker
If you see garbage after |END, the bug exists!

Test message 32 bytes .............|END
...

TEST 3: Pattern Recognition Test
----------------------------------------------------------------------
012345678901234567890123456789|OK
ABCDEFGHIJABCDEFGHIJABCDEFGHIJ|OK
...

TEST 4: Null Terminator Handling
----------------------------------------------------------------------
Testing correct null terminator placement...

Message_001:ABCDEFGHIJKLMNOPQRST_______|
Message_002:0123456789ABCDEFGHIJ_______|
...

======================================================================
ALL TESTS COMPLETE
======================================================================
```

### 5. Verify Results

**✅ PASS - Bug is Fixed (Expected Result)**

All test messages display correctly:
- All markers (`|END`, `|OK`, `|`) appear at the correct positions
- No garbage characters appear after markers
- Messages are not truncated unexpectedly
- All 4 test suites complete without issues

**❌ FAIL - Bug Still Exists (Unexpected Result)**

If the bug still existed, you would see:
- Garbage characters appearing after end markers
- Random bytes appearing where they shouldn't
- Messages truncated at wrong positions
- Possible crashes when processing certain buffer sizes

### 6. Check Server Console

The Python server console will show:

```
✓ Mudlet connected from ('127.0.0.1', XXXXX)

Starting buffer handling tests...

✓ Sent telnet negotiations
Running Test 1: Buffer Boundary Test
  Sent: 31 bytes (just before boundary)
  Sent: 32 bytes (common boundary)
  Sent: 33 bytes (just after boundary)
  ...
  ✓ Test 1 complete

Running Test 2: Marker Byte Test
  ...
  ✓ Test 2 complete

...

======================================================================
Tests complete! Connection will remain open.
Check Mudlet output for test results.
Press Ctrl+C to shutdown server.
======================================================================
```

### 7. Cleanup

Press `Ctrl+C` in the server terminal to stop the test server.

## Understanding the Tests

### Test 1: Buffer Boundary Test
Sends messages at critical sizes (31, 32, 33, 63, 64, 127, 128 bytes). These sizes are chosen because:
- 32, 64, 128 are common buffer boundaries
- The off-by-one error would be most visible at these boundaries
- The bug would leave byte at position [amount] uninitialized

### Test 2: Marker Byte Test
Sends messages with clear end markers (`|END`). If the null terminator is placed incorrectly:
- The garbage byte at position [amount] might appear after the marker
- You'd see something like: `Test message|END?` (where ? is garbage)

### Test 3: Pattern Recognition Test
Sends repeating patterns. If truncation happens at the wrong position:
- The pattern would be cut off incorrectly
- The `|OK` marker would be missing or have garbage after it

### Test 4: Null Terminator Handling
Validates that null terminator placement doesn't affect subsequent data:
- Messages should end exactly at the `|` marker
- No premature truncation
- No extra bytes being processed

## Expected Results for Fixed Code

With the fix applied (as of the commits in this branch), all tests should **PASS**. The fix ensures:

1. Error conditions checked BEFORE buffer access:
   ```cpp
   if (amount == -1 || amount == 0) {
       in_buffer[0] = '\0';
       return;
   }
   ```

2. Null terminator at correct position:
   ```cpp
   in_buffer[amount] = '\0';  // NOT amount + 1
   ```

## Troubleshooting

**Server won't start - "Address already in use"**
- Another process is using port 4000
- Either stop that process or edit `PORT = 4000` in the script to use a different port
- Remember to update the port in Mudlet connection settings too

**Mudlet can't connect**
- Verify the server is running and shows "Waiting for Mudlet to connect..."
- Check that you're using `localhost` and port `4000` in Mudlet
- Try `127.0.0.1` instead of `localhost` if having DNS issues

**No output in Mudlet**
- Ensure Mudlet successfully connected (server console shows "Mudlet connected")
- Check that Mudlet's main window is visible and scrolled to the bottom
- The server waits 0.5 seconds between tests - be patient

## Files

- `test_issue_1065_server.py` - The telnet test server
- `TEST_ISSUE_1065_README.md` - This file
- `test/cTelnetBufferTest.cpp` - Unit test (simulates logic)
- `test/ISSUE_1065_TEST_README.md` - Unit test documentation

## References

- **Original Issue**: https://github.com/Mudlet/Mudlet/issues/1065
- **Fixed Code**: `src/ctelnet.cpp` lines 4020-4024
- **Function**: `cTelnet::processSocketData(char* in_buffer, int amount, const bool loopbackTesting)`
