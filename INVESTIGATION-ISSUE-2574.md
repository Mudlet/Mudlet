# Investigation Report: GitHub Issue #2574

**Issue:** `tempComplexRegexTrigger` does not do anything with fg/bg color arguments (5 and 6)
**Reporter:** SlySven
**Date Reported:** May 27, 2019
**Investigation Date:** October 21, 2025
**Status:** ✅ **CONFIRMED - Issue is still present**

## Summary

The bug reported in 2019 is **still present** in the current codebase. The `tempComplexRegexTrigger()` function reads the 5th and 6th arguments (foreground and background colors) but never actually stores them in the trigger object, making color-based triggering non-functional despite the API accepting these parameters.

## Detailed Analysis

### Bug Location

- **File:** `src/TLuaInterpreterMudletObjects.cpp`
- **Function:** `TLuaInterpreter::tempComplexRegexTrigger()`
- **Lines:** 2206-2220 (color reading), 2277-2281 (pattern type setting)

### What the Code Currently Does

```cpp
// Lines 2206-2220: Reads color arguments into local variables
bool colorTrigger;
QString fgColor;
if (lua_isnumber(L, 5)) {
    colorTrigger = false;
} else {
    colorTrigger = true;
    fgColor = lua_tostring(L, 5);  // ⚠️ Read but NEVER used
}

QString bgColor;
if (lua_isnumber(L, 6)) {
    colorTrigger = false;
} else {
    bgColor = lua_tostring(L, 6);  // ⚠️ Read but NEVER used
}

// Lines 2277-2281: Sets pattern type but doesn't store colors
if (colorTrigger) {
    propertyList << REGEX_COLOR_PATTERN;  // ✅ Type is set
} else {
    propertyList << REGEX_PERL;
}
// ⚠️ fgColor and bgColor are NEVER passed to the trigger object!
```

**The Problem:** The variables `fgColor` and `bgColor` are read from Lua arguments but are completely unused after that. They're never stored in the `TTrigger` object (`pT`).

### Comparison with Working Implementation

The `tempAnsiColorTrigger()` function (lines 1791-1884) shows how color triggers should work:

```cpp
// tempAnsiColorTrigger properly handles colors:
int ansiFgColor = TTrigger::scmIgnored;
int ansiBgColor = TTrigger::scmIgnored;
// ... reads ANSI color codes as integers ...

// Properly stores colors by calling:
const int triggerID = pLuaInterpreter->startTempColorTrigger(ansiFgColor, ansiBgColor, code, expiryCount);
```

Which then calls `pT->setupTmpColorTrigger(fg, bg)` (TLuaInterpreter.cpp:6371) which properly creates the color pattern using `TTrigger::createColorPattern()` and stores it in `mColorPatternList`.

## Root Cause

The issue has two fundamental problems:

1. **Missing Color Storage:** The function never calls `setupTmpColorTrigger()` or `setupColorTrigger()` to actually store the color values

2. **Type Mismatch:**
   - `tempComplexRegexTrigger` accepts **string** colors (arguments 5 & 6)
   - But `setupTmpColorTrigger()` expects **integer ANSI codes** (0-255, plus special values)
   - There's no conversion mechanism from string color names to ANSI codes in the codebase

## Impact

- **Functionality:** Arguments 5 and 6 of `tempComplexRegexTrigger` are completely non-functional
- **Documentation:** The documentation claims these parameters trigger on foreground/background colors, but this has never worked
- **User Experience:** Users attempting to use color triggers via this function will find they don't work, with no error messages to indicate why
- **Priority:** Low (as noted in original issue) - the working `tempAnsiColorTrigger()` function provides color trigger functionality

## Technical Details

### Color Trigger System Architecture

- Color triggers work with **ANSI color codes** (integers 0-255, plus special values `TTrigger::scmIgnored` and `TTrigger::scmDefault`)
- Colors are stored in `TColorTable` objects via `createColorPattern(int ansiFg, int ansiBg)`
- The pattern text is encoded as `ANSI_COLORS_F{###}_B{###}` format
- No reverse mapping exists from QColor/string names to ANSI codes

### Relevant Code References

- Color pattern creation: `TTrigger::createColorPattern()` at `src/TTrigger.cpp:1149`
- Color trigger setup: `TTrigger::setupTmpColorTrigger()` at `src/TTrigger.cpp:1218`
- Working example: `TLuaInterpreter::tempAnsiColorTrigger()` at `src/TLuaInterpreterMudletObjects.cpp:1791`
- ANSI color constants: `TTrigger::scmIgnored` and `TTrigger::scmDefault` defined in `src/TTrigger.h:160-161`
- Host color conversion: `Host::getAnsiColor()` at `src/Host.h:333` (converts ANSI code → QColor, but no reverse exists)

## Recommendations

The issue should remain open as it's definitely still present. Two potential approaches to fix:

### Option A: Remove Non-Functional Parameters (Simpler)

- Remove or deprecate arguments 5 and 6 from `tempComplexRegexTrigger`
- Update documentation to direct users to `tempAnsiColorTrigger()` for color-based triggering
- This acknowledges the current reality and prevents user confusion

### Option B: Implement Proper Color Support (More Complex)

- Implement string-to-ANSI color conversion
- Call `setupColorTrigger()` with the converted values
- Challenge: ANSI colors are discrete (0-255) while string colors can be arbitrary RGB values
- Would need to define how named colors map to ANSI codes

**Recommendation:** Given the low priority and the existence of the working `tempAnsiColorTrigger()` function, **Option A** may be more pragmatic.

## Conclusion

GitHub issue #2574 from 2019 remains valid and unresolved in the current codebase. The color arguments (5 and 6) to `tempComplexRegexTrigger()` are read but never stored, making them non-functional. Users who need color-based triggers should use `tempAnsiColorTrigger()` instead.
