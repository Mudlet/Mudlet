#!/usr/bin/env lua
--[[
Self-test for check-lua-error-strands.lua.

A scanner that has quietly stopped matching anything reports a clean tree and
looks exactly like success, so CI runs this first: the bad fixture must produce
one finding per known shape, and the good fixture must produce none.
]]

local here = arg[0]:match("^(.*)/[^/]*$") or "."
local scanner = here .. "/check-lua-error-strands.lua"
local fixtures = here .. "/lua-error-strand-fixtures"

-- arg[-1] is the argv entry immediately before the script, so the scanner runs
-- under the same interpreter as this test - "lua" from gh-actions-lua, or
-- "lua5.1" from a Debian package. A flag there (lua -W test.lua) would be
-- picked up instead, so fall back on anything that is not a path.
local interpreter = arg[-1]
if not interpreter or interpreter:match("^%-") then interpreter = "lua" end

-- Single quotes, not Lua's %q: %q escapes " and \ but leaves $ and backticks
-- for the shell to expand.
local function shellQuote(text)
  return "'" .. text:gsub("'", "'\\''") .. "'"
end

local function run(file)
  local command = ("%s %s %s 2>&1; echo EXIT:$?"):format(
    shellQuote(interpreter), shellQuote(scanner), shellQuote(fixtures .. "/" .. file))
  local pipe = io.popen(command)
  local out = pipe:read("*a")
  pipe:close()
  return out, tonumber(out:match("EXIT:(%d+)%s*$"))
end

local failures = {}
local function check(condition, message)
  if not condition then failures[#failures + 1] = message end
end

-- Every line of bad.cpp marked "// STRAND" must be reported. Deriving the
-- expectations from the fixture rather than listing line numbers here means an
-- edit to the fixture cannot silently stop checking a shape - which listing
-- them did twice while this was being written.
local mustCatch = {}
for line in io.lines(fixtures .. "/bad.cpp") do
  mustCatch[#mustCatch + 1] = line
end
do
  local marked = {}
  for number, text in ipairs(mustCatch) do
    if text:match("//%s*STRAND%s*$") then marked[#marked + 1] = number end
  end
  mustCatch = marked
end
check(#mustCatch > 0, "bad.cpp has no // STRAND markers - nothing would be checked")

local badOutput, badStatus = run("bad.cpp")
-- The property the whole workflow rests on, and the only one the text
-- assertions below cannot see: findings have to make the scanner fail.
check(badStatus == 1,
      ("bad.cpp exited %s, not 1 - the scanner no longer fails CI on a finding")
      :format(tostring(badStatus)))
for _, number in ipairs(mustCatch) do
  check(badOutput:match("bad%.cpp:" .. number .. ":"),
        ("bad.cpp:%d is marked // STRAND but was not reported"):format(number))
end

-- and nothing beyond them: an unmarked finding is either a new shape that wants
-- a marker, or a false positive
for number in badOutput:gmatch("bad%.cpp:(%d+):") do
  local wanted = false
  for _, marked in ipairs(mustCatch) do
    if tonumber(number) == marked then wanted = true end
  end
  check(wanted, ("bad.cpp:%s was reported but carries no // STRAND marker"):format(number))
end

local goodOutput, goodStatus = run("good.cpp")
check(goodStatus == 0,
      ("good.cpp exited %s, not 0 - a clean file is being failed")
      :format(tostring(goodStatus)))
check(goodOutput:match("No heap objects stranded"),
      "good.cpp reported a finding, so the scanner has false positives:\n" .. goodOutput)

-- A file the scanner never entered is "clean" for the wrong reason, and reads
-- exactly like a real pass. Truncating this fixture during an edit produced
-- precisely that, so hold it to having actually scanned something.
local goodFunctions = tonumber(goodOutput:match("(%d+) functions scanned")) or 0
check(goodFunctions >= 6,
      ("good.cpp scanned %d functions, expected at least 6 - it is passing because it was not read")
      :format(goodFunctions))

if #failures > 0 then
  print("Scanner self-test FAILED:")
  for _, f in ipairs(failures) do print("  - " .. f) end
  print("\nbad.cpp output was:\n" .. badOutput)
  os.exit(1)
end

print(("Scanner self-test passed: %d bad shapes caught, good fixture clean.")
      :format(#mustCatch))
