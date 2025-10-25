-- Test script for issue #823: setLabelOnEnter, setLabelOnLeave,
-- setLabelClickCallback & setLabelReleaseCallback do not handle
-- lua nil or boolean datatype

print("Testing label callback functions with nil and boolean arguments...")

-- Create a test label
createLabel("testLabel", 100, 100, 100, 30, 1)

-- Test 1: Try to set callback with a function (this should work)
print("\nTest 1: Setting callback with a function (expected to work)")
local success1, err1 = pcall(function()
    setLabelClickCallback("testLabel", function() print("clicked!") end)
end)
print("  Result:", success1 and "SUCCESS" or "FAILED")
if not success1 then
    print("  Error:", err1)
end

-- Test 2: Try to clear callback with nil (this should fail with current code)
print("\nTest 2: Setting callback with nil (expected to fail with bug)")
local success2, err2 = pcall(function()
    setLabelClickCallback("testLabel", nil)
end)
print("  Result:", success2 and "SUCCESS" or "FAILED")
if not success2 then
    print("  Error:", err2)
end

-- Test 3: Try to set callback with boolean true (this should fail with current code)
print("\nTest 3: Setting callback with boolean true (expected to fail with bug)")
local success3, err3 = pcall(function()
    setLabelClickCallback("testLabel", true)
end)
print("  Result:", success3 and "SUCCESS" or "FAILED")
if not success3 then
    print("  Error:", err3)
end

-- Test 4: Try to set callback with boolean false (this should fail with current code)
print("\nTest 4: Setting callback with boolean false (expected to fail with bug)")
local success4, err4 = pcall(function()
    setLabelClickCallback("testLabel", false)
end)
print("  Result:", success4 and "SUCCESS" or "FAILED")
if not success4 then
    print("  Error:", err4)
end

-- Test the other three affected functions
print("\n\nTesting other affected functions:")

print("\nTest 5: setLabelOnEnter with nil")
local success5, err5 = pcall(function()
    setLabelOnEnter("testLabel", nil)
end)
print("  Result:", success5 and "SUCCESS" or "FAILED")
if not success5 then
    print("  Error:", err5)
end

print("\nTest 6: setLabelOnLeave with nil")
local success6, err6 = pcall(function()
    setLabelOnLeave("testLabel", nil)
end)
print("  Result:", success6 and "SUCCESS" or "FAILED")
if not success6 then
    print("  Error:", err6)
end

print("\nTest 7: setLabelReleaseCallback with nil")
local success7, err7 = pcall(function()
    setLabelReleaseCallback("testLabel", nil)
end)
print("  Result:", success7 and "SUCCESS" or "FAILED")
if not success7 then
    print("  Error:", err7)
end

-- Cleanup
hideWindow("testLabel")

print("\n\nConclusion:")
if not (success2 or success3 or success4 or success5 or success6 or success7) then
    print("CONFIRMED: Issue #823 still exists!")
    print("None of the label callback functions accept nil or boolean values.")
else
    print("Issue #823 appears to be fixed - some functions accepted nil/boolean.")
end
