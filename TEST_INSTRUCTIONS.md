# Testing the 10,000 Item Table Save Limit Feature

## Quick Start

### Method 1: Using the Test Script

1. **Load the test script in Mudlet:**
   ```lua
   dofile("/home/user/Mudlet/test_table_save_limit.lua")
   ```
   Or copy-paste the entire contents of `test_table_save_limit.lua` into Mudlet's input line

2. **Open the Variables editor:**
   - Go to **Toolbox → Variables** (or press `Alt+2` on Windows/Linux, `Cmd+2` on macOS)

3. **Verify the behavior:**
   - Expand each test variable in the tree
   - Try to check/uncheck the save checkbox for each variable
   - Hover over disabled (grayed-out) checkboxes to see the tooltip

4. **Clean up when done:**
   ```lua
   cleanup()
   ```

### Method 2: Quick Manual Test

Paste these commands one at a time into Mudlet:

```lua
-- Create a table with 100 items (should be saveable)
small_test = {}
for i = 1, 100 do small_test[i] = i end
```

```lua
-- Create a table with 10,001 items (should NOT be saveable)
large_test = {}
for i = 1, 10001 do large_test[i] = i end
```

Then open the Variables editor and check the behavior.

## What to Look For

### ✓ Saveable Variables (Checkbox Enabled)
- `test_small_table` - 100 items
- `test_at_limit` - 10,000 items (right at the limit)
- `test_nested_ok` - 5,000 total items (100 tables × 50 items each)
- `test_mixed_types` - 500 items with mixed types

**Expected UI:**
- Checkbox is **enabled** (not grayed out)
- Can be checked and unchecked
- Tooltip shows: *"Checked variables will be saved and loaded with your profile."*

### ✗ Non-Saveable Variables (Checkbox Disabled)
- `test_over_limit` - 10,001 items
- `test_large_table` - 50,000 items
- `test_nested_over` - 10,200 total items (200 tables × 51 items each)
- `test_deep_nested` - 10,400 total items (20 × 20 × 26 nested)

**Expected UI:**
- Checkbox is **disabled and grayed out**
- Cannot be checked
- Tooltip shows helpful message like:
  ```
  This table has 10,001 items, exceeding the 10,000 item limit
  for saved variables. Use table.save() and table.load() instead
  for better performance with large tables. Learn more
  ```
- Item count is formatted according to system locale (e.g., "10,001" or "10.001" depending on locale)
- "Learn more" is a clickable link to the Mudlet documentation

## Testing the Recommended Alternative

To test that `table.save()` and `table.load()` work properly with large tables:

```lua
-- Save a large table using table.save()
table.save(getMudletHomeDir() .. "/large_test.dat", large_test)
echo("✓ Large table saved successfully\n")

-- Clear the variable
large_test = nil

-- Load it back
table.load(getMudletHomeDir() .. "/large_test.dat")
echo("✓ Large table loaded successfully\n")

-- Verify it worked
if large_test and #large_test == 10001 then
    echo("✓ Verification passed: table has " .. #large_test .. " items\n")
else
    echo("✗ Verification failed\n")
end
```

## Edge Cases to Test

1. **Exactly at the limit:**
   ```lua
   exactly_10k = {}
   for i = 1, 10000 do exactly_10k[i] = i end
   ```
   Should be **saveable** ✓

2. **One over the limit:**
   ```lua
   one_over = {}
   for i = 1, 10001 do one_over[i] = i end
   ```
   Should be **not saveable** ✗

3. **Empty nested tables:**
   ```lua
   nested_empty = {}
   for i = 1, 100 do nested_empty[i] = {} end
   ```
   Should be **saveable** (only 100 items total) ✓

4. **Functions (already non-saveable):**
   ```lua
   func_test = function() echo("test") end
   ```
   Should be **not saveable** with message: *"Lua functions cannot be saved."*

## Tooltip Message Verification

For large tables, the tooltip should include:
1. ✓ Exact item count (formatted with locale-appropriate thousands separator)
2. ✓ Clear explanation of the 10,000 limit
3. ✓ Bold text for `table.save()` and `table.load()`
4. ✓ Clickable "Learn more" link to documentation
5. ✓ Professional, helpful tone (not technical error message)

## Performance Note

Creating the test variables (especially `test_large_table` with 50,000 items) may take a few seconds. This is normal and demonstrates why saving such large tables through the UI would cause performance issues.

## Cleanup

Remove all test variables:
```lua
cleanup()
```

Or manually:
```lua
test_small_table = nil
test_at_limit = nil
test_over_limit = nil
test_large_table = nil
test_nested_over = nil
test_nested_ok = nil
test_deep_nested = nil
test_mixed_types = nil
```

Then refresh the Variables editor to see them disappear.
