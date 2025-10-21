# Table Arguments Tests

This document describes the test coverage for table argument support in Lua functions with 5+ parameters.

## Test Organization

Table argument tests are integrated into existing domain-specific test files rather than in standalone test files. This keeps tests organized by module and makes them easier to find.

### 1. `GUIUtils_spec.lua`
Tests for GUI-related Lua functions that support table arguments.

**Functions tested:**
- `prefix()` - Text prefix with formatting (5 parameters)
- `suffix()` - Text suffix with formatting (5 parameters)
- `createGauge()` - Gauge creation (11 parameters)
- `createConsole()` - Console creation (7 parameters)

**Test coverage:**
- Positional arguments (backward compatibility)
- Table arguments with primary key names
- Table arguments with alternative key names

**Test location:** Added to the end of the existing file before the TODO comment.

### 2. `IDManager_spec.lua`
Tests for event and timer management functions.

**Functions tested:**
- `registerNamedEventHandler()` - Event handler registration (5 parameters)
- `registerNamedTimer()` - Timer registration (5 parameters)

**Test coverage:**
- Positional arguments with all parameters
- Table arguments with complete parameter sets
- Alternative key name variations (userName, handlerName, eventName, functionReference)
- Integration with existing spy assertions

**Test location:** Added to existing describe blocks for each function.

### 3. `Other_spec.lua`
Tests for miscellaneous utility functions.

**Functions tested:**
- `timeframe()` - Time-based variable changes (varargs, 3+ parameters)

**Test coverage:**
- Positional arguments with minimum parameters
- Positional arguments with additional varargs
- Table arguments with timerlist
- Alternative key names (name/variable, trueTime, nilTime, timers/timerlist)
- Function as vname parameter

**Test location:** New describe block added before the TODO comment.

## Running the Tests

### Prerequisites

1. Install Busted (Lua testing framework):
   ```bash
   # Ubuntu/Debian
   sudo apt-get install luarocks
   sudo luarocks install busted

   # macOS
   brew install luarocks
   luarocks install busted
   ```

2. Have Mudlet compiled and ready to run

### Execution

Tests must be run from within Mudlet using the "Mudlet self-test" profile:

1. **Open Mudlet** and connect to the "Mudlet self-test" profile
   - Type "Mudlet self-test" in the connection dialog

2. **Run tests by module:**
   ```lua
   -- Test GUI functions (prefix, suffix, createGauge, createConsole)
   runTests "<path>/Mudlet/src/mudlet-lua/tests/GUIUtils_spec.lua"

   -- Test event/timer managers (registerNamedEventHandler, registerNamedTimer)
   runTests "<path>/Mudlet/src/mudlet-lua/tests/IDManager_spec.lua"

   -- Test utility functions (timeframe)
   runTests "<path>/Mudlet/src/mudlet-lua/tests/Other_spec.lua"
   ```

3. **Run all Lua tests:**
   ```lua
   runTests "<path>/Mudlet/src/mudlet-lua/tests"
   ```

### Expected Results

All tests should pass, demonstrating:
- ✅ Backward compatibility with positional arguments
- ✅ Table argument support for all migrated Lua functions
- ✅ Case-insensitive key matching
- ✅ Alternative key name recognition
- ✅ Proper parameter validation

## C++ Function Tests

C++ functions exposed to Lua require a full Mudlet environment and GUI to test properly. These include:

**Functions that need manual/integration testing:**
- Link functions: `echoLink()`, `insertLink()` (5 params)
- Popup functions: `echoPopup()`, `insertPopup()` (5 params)
- Color functions: `setCommandBackgroundColor()`, `setCommandForegroundColor()`, `setBackgroundColor()`, `setBgColor()`, `setCustomEnvColor()` (5 params)
- Key binding: `permKey()` (4-5 params with optional modifier)
- Mapper functions: `createMapper()`, `addMapEvent()`, `createMapLabel()`, `createMapImageLabel()`, `highlightRoom()`, `addCustomLine()` (5-18 params)
- UI functions: `setTextFormat()`, `createLabel()`, `createMiniConsole()`, `createCommandLine()`, `createScrollBox()`, `tempComplexRegexTrigger()` (6-14 params)

**Manual testing approach:**
1. Open Mudlet with a test profile
2. Test each function with both positional and table arguments
3. Verify backward compatibility
4. Check alternative key names work
5. Validate error messages for missing required parameters

## Test Structure

Each test follows the Busted framework pattern:

```lua
describe("Tests [function_name]() table argument support", function()
  setup(function()
    -- Mock dependencies if needed
    _G.someFunction = function() end
  end)

  it("should accept positional arguments", function()
    local result = pcall(function_name, arg1, arg2, arg3, ...)
    assert.is_true(result)
  end)

  it("should accept table arguments", function()
    local result = pcall(function_name, {
      param1 = value1,
      param2 = value2,
      param3 = value3,
      ...
    })
    assert.is_true(result)
  end)

  it("should accept table arguments with alternative key names", function()
    local result = pcall(function_name, {
      alternateParam1 = value1,
      alternateParam2 = value2,
      ...
    })
    assert.is_true(result)
  end)
end)
```

## Adding New Tests

When adding table argument support to a new function:

1. Add tests to the appropriate existing spec file:
   - GUI functions → `GUIUtils_spec.lua`
   - Event/timer functions → `IDManager_spec.lua`
   - Utility functions → `Other_spec.lua`
   - UI/C++ functions → May need integration tests

2. Test both positional and table argument formats
3. Test alternative key names
4. Verify backward compatibility
5. Update this documentation

## Summary

- **Total Lua functions with tests:** 7
  - GUIUtils.lua: 4 functions
  - IDManager.lua: 2 functions
  - Other.lua: 1 function

- **C++ functions (manual/integration testing):** 31+ functions
  - Require full Mudlet environment
  - Need GUI interaction for validation

- **Test coverage:** All 38 functions with 5+ parameters support table arguments, with 7 having automated unit tests and 31+ requiring integration/manual testing.

## Related Files

- **TABLE_ARGS_MIGRATION.md** - Migration status and function list
- **src/mudlet-lua/lua/GUIUtils.lua** - Lua GUI functions
- **src/mudlet-lua/lua/IDManager.lua** - Event/timer management functions
- **src/mudlet-lua/lua/Other.lua** - Utility functions
- **src/TLuaInterpreter*.cpp** - C++ Lua API implementations
