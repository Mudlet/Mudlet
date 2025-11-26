--[[
    Test script for the 10,000 item table save limit feature

    This script creates various test tables to verify the UI behavior:
    1. Small tables (should be saveable)
    2. Tables at the 10k limit (should be saveable)
    3. Tables exceeding 10k limit (should NOT be saveable with helpful tooltip)
    4. Nested tables exceeding 10k total items (should NOT be saveable)

    Usage:
    1. Copy this script to Mudlet's input line or save as a script
    2. Run it to create test variables
    3. Open the Variables editor (Toolbox > Variables)
    4. Inspect each test variable's checkbox behavior and tooltips
    5. Run cleanup() to remove all test variables when done
]]

-- Test 1: Small table (should be saveable)
echo("\n=== Creating Test Variables ===\n")
echo("1. Creating 'test_small_table' with 100 items...\n")
test_small_table = {}
for i = 1, 100 do
    test_small_table[i] = "item_" .. i
end
echo("   ✓ Should be SAVEABLE (100 items)\n")

-- Test 2: Table exactly at limit (should be saveable)
echo("2. Creating 'test_at_limit' with exactly 10,000 items...\n")
test_at_limit = {}
for i = 1, 10000 do
    test_at_limit[i] = i
end
echo("   ✓ Should be SAVEABLE (10,000 items)\n")

-- Test 3: Table just over the limit (should NOT be saveable)
echo("3. Creating 'test_over_limit' with 10,001 items...\n")
test_over_limit = {}
for i = 1, 10001 do
    test_over_limit[i] = i
end
echo("   ✗ Should NOT be saveable (10,001 items)\n")
echo("   → Tooltip should show: 'This table has 10,001 items...'\n")

-- Test 4: Table well over the limit (should NOT be saveable)
echo("4. Creating 'test_large_table' with 50,000 items...\n")
test_large_table = {}
for i = 1, 50000 do
    test_large_table[i] = "data_" .. i
end
echo("   ✗ Should NOT be saveable (50,000 items)\n")
echo("   → Tooltip should show: 'This table has 50,000 items...'\n")

-- Test 5: Nested tables that exceed limit in total
echo("5. Creating 'test_nested_over' with nested structure (200 x 51 = 10,200 total items)...\n")
test_nested_over = {}
for i = 1, 200 do
    test_nested_over[i] = {}
    for j = 1, 51 do
        test_nested_over[i][j] = string.format("nest_%d_%d", i, j)
    end
end
echo("   ✗ Should NOT be saveable (10,200 total items including nested)\n")
echo("   → Tooltip should show: 'This table has 10,200 items...'\n")

-- Test 6: Nested tables within limit
echo("6. Creating 'test_nested_ok' with nested structure (100 x 50 = 5,000 total items)...\n")
test_nested_ok = {}
for i = 1, 100 do
    test_nested_ok[i] = {}
    for j = 1, 50 do
        test_nested_ok[i][j] = string.format("data_%d_%d", i, j)
    end
end
echo("   ✓ Should be SAVEABLE (5,000 total items including nested)\n")

-- Test 7: Deep nesting with limit exceeded
echo("7. Creating 'test_deep_nested' with 3 levels (20 x 20 x 26 = 10,400 total items)...\n")
test_deep_nested = {}
for i = 1, 20 do
    test_deep_nested[i] = {}
    for j = 1, 20 do
        test_deep_nested[i][j] = {}
        for k = 1, 26 do
            test_deep_nested[i][j][k] = string.format("deep_%d_%d_%d", i, j, k)
        end
    end
end
echo("   ✗ Should NOT be saveable (10,400 total items)\n")

-- Test 8: Mixed types table (should be saveable if within limit)
echo("8. Creating 'test_mixed_types' with 500 mixed-type items...\n")
test_mixed_types = {}
for i = 1, 500 do
    if i % 5 == 0 then
        test_mixed_types["key_" .. i] = {nested = "value"}
    elseif i % 3 == 0 then
        test_mixed_types[i] = true
    else
        test_mixed_types[i] = "value_" .. i
    end
end
echo("   ✓ Should be SAVEABLE (500 items with mixed types)\n")

echo("\n=== Test Variables Created ===\n")
echo("Open the Variables editor to verify the behavior:\n")
echo("  → Menu: Toolbox > Variables\n")
echo("  → Or press: Alt+2 (Windows/Linux) or Cmd+2 (macOS)\n\n")

echo("Expected behavior:\n")
echo("  ✓ test_small_table: Checkbox enabled, can be checked\n")
echo("  ✓ test_at_limit: Checkbox enabled, can be checked\n")
echo("  ✓ test_nested_ok: Checkbox enabled, can be checked\n")
echo("  ✓ test_mixed_types: Checkbox enabled, can be checked\n")
echo("  ✗ test_over_limit: Checkbox DISABLED (grayed), helpful tooltip\n")
echo("  ✗ test_large_table: Checkbox DISABLED (grayed), helpful tooltip\n")
echo("  ✗ test_nested_over: Checkbox DISABLED (grayed), helpful tooltip\n")
echo("  ✗ test_deep_nested: Checkbox DISABLED (grayed), helpful tooltip\n\n")

echo("Tooltips for disabled variables should:\n")
echo("  • Show the exact item count in localized format\n")
echo("  • Explain the 10,000 item limit\n")
echo("  • Recommend using table.save() and table.load()\n")
echo("  • Include a clickable 'Learn more' link\n\n")

echo("When done testing, run: cleanup()\n")

-- Cleanup function
function cleanup()
    echo("\n=== Cleaning up test variables ===\n")
    test_small_table = nil
    test_at_limit = nil
    test_over_limit = nil
    test_large_table = nil
    test_nested_over = nil
    test_nested_ok = nil
    test_deep_nested = nil
    test_mixed_types = nil
    echo("✓ All test variables removed\n")
    echo("Refresh the Variables editor to see the changes\n")
end

echo("=== Quick verification commands ===\n")
echo("Get item counts:\n")
echo('  lua display(#test_small_table)       -- Should show: 100\n')
echo('  lua display(#test_at_limit)          -- Should show: 10000\n')
echo('  lua display(#test_over_limit)        -- Should show: 10001\n')
echo('  lua display(#test_large_table)       -- Should show: 50000\n')
echo("\nNote: Nested table counts need recursive counting (done by Mudlet internally)\n")
