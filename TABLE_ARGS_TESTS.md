# Table Arguments Tests

This document describes the test suite for table argument support in Lua functions with 5+ parameters.

## Test Files

### 1. `TableArguments_spec.lua`
Tests for pure Lua functions that support table arguments.

**Functions tested:**
- `prefix()` - Text prefix with formatting
- `suffix()` - Text suffix with formatting
- `createGauge()` - Gauge creation (11 parameters)
- `createConsole()` - Console creation (7 parameters)
- `registerNamedEventHandler()` - Event handler registration
- `registerNamedTimer()` - Timer registration
- `timeframe()` - Time-based variable changes (varargs)

**Test coverage:**
- Positional arguments (backward compatibility)
- Table arguments with primary key names
- Table arguments with alternative key names
- Case-insensitive key matching

### 2. `TableArgumentsIntegration_spec.lua`
Integration tests for C++ functions exposed to Lua via the Lua C API.

**Functions tested:**
- Link functions: `echoLink()`, `insertLink()`
- Popup functions: `echoPopup()`, `insertPopup()`
- Color functions: `setCommandBackgroundColor()`, `setCommandForegroundColor()`, `setBackgroundColor()`, `setBgColor()`, `setCustomEnvColor()`
- Key binding: `permKey()`
- Mapper: `createMapper()`, `addMapEvent()`

**Test coverage:**
- Positional arguments with all parameters
- Table arguments with complete parameter sets
- Alternative key name variations
- Optional parameter handling

### 3. `TableArgumentsComplex_spec.lua`
Tests for complex functions with 10+ parameters that benefit most from table argument syntax.

**Functions tested:**
- `createMapLabel()` - 18 parameters (11 required + 7 optional)
- `setTextFormat()` - 13 parameters
- `tempComplexRegexTrigger()` - 14 parameters
- `highlightRoom()` - 10 parameters
- `createMapImageLabel()` - 9-10 parameters
- `createLabel()` - 8 parameters
- `addCustomLine()` - 6 parameters
- `createMiniConsole()` - 6 parameters
- `createCommandLine()` - 6 parameters
- `createScrollBox()` - 6 parameters

**Test coverage:**
- Minimum required parameters (positional)
- All parameters including optional ones (positional)
- Table arguments with required parameters only
- Table arguments with all optional parameters
- Mixed case table key names
- Function vs string code parameters

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

2. **Run all table argument tests:**
   ```lua
   runTests "<path>/Mudlet/src/mudlet-lua/tests/TableArguments_spec.lua"
   runTests "<path>/Mudlet/src/mudlet-lua/tests/TableArgumentsIntegration_spec.lua"
   runTests "<path>/Mudlet/src/mudlet-lua/tests/TableArgumentsComplex_spec.lua"
   ```

3. **Run all Lua tests (including table argument tests):**
   ```lua
   runTests "<path>/Mudlet/src/mudlet-lua/tests"
   ```

### Expected Results

All tests should pass, demonstrating:
- ✅ Backward compatibility with positional arguments
- ✅ Table argument support for all migrated functions
- ✅ Case-insensitive key matching
- ✅ Alternative key name recognition
- ✅ Proper parameter validation

## Test Structure

Each test follows the Busted framework pattern:

```lua
describe("Tests [function_name]() function", function()
  setup(function()
    -- Mock dependencies if needed
  end)

  it("Should accept positional arguments", function()
    local result = pcall(function_name, arg1, arg2, arg3, ...)
    assert.is_true(result)
  end)

  it("Should accept table arguments", function()
    local result = pcall(function_name, {
      param1 = value1,
      param2 = value2,
      param3 = value3,
      ...
    })
    assert.is_true(result)
  end)
end)
```

## Integration Tests

Note that some tests in `TableArgumentsIntegration_spec.lua` and `TableArgumentsComplex_spec.lua` require a full Mudlet environment and will be skipped if `mudlet` global is not available.

These tests check for function availability before running and will show as "pending" if the function is not accessible:

```lua
if not functionName then
  pending("functionName not available")
  return
end
```

## Adding New Tests

When adding table argument support to a new function:

1. Add a test suite in the appropriate spec file
2. Test both positional and table argument formats
3. Test alternative key names
4. Verify backward compatibility
5. Document the function in this README

## Related Files

- **TABLE_ARGS_MIGRATION.md** - Migration status and function list
- **src/mudlet-lua/lua/GUIUtils.lua** - Lua GUI functions
- **src/mudlet-lua/lua/IDManager.lua** - Event/timer management functions
- **src/mudlet-lua/lua/Other.lua** - Utility functions
- **src/TLuaInterpreter*.cpp** - C++ Lua API implementations
