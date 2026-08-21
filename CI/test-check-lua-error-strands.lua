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

-- every shape the scanner must catch, keyed by the line of the raise (not of
-- the declaration, which is where it is tempting to point)
local mustCatch = {
  {5,  "a local QString live across the next argument's getVerifiedString"},
  {12, "a QByteArray temporary inside the raiser's own argument list"},
  {20, "a QByteArray live across a raw lua_error"},
  {29, "a local live in a function whose signature wraps onto a second line"},
  {40, "a pre-gate on a different argument must not suppress the raise"},
  {52, "an isEmpty() early-return guard leaves the variable non-empty, not free"},
  {59, "an auto-typed local holding a QString"},
  {67, "a container declared empty and filled afterwards"},
  {76, "parseCommandsOrFunctionsTable raises like its two siblings"},
  {90, "a checker inside a closed branch does not dominate the raise"},
  {100, "a wrapped signature that is also indented, as inline members are"},
}

local badOutput, badStatus = run("bad.cpp")
-- The property the whole workflow rests on, and the only one the text
-- assertions below cannot see: findings have to make the scanner fail.
check(badStatus == 1,
      ("bad.cpp exited %s, not 1 - the scanner no longer fails CI on a finding")
      :format(tostring(badStatus)))
for _, want in ipairs(mustCatch) do
  check(badOutput:match("bad%.cpp:" .. want[1] .. ":"),
        ("bad.cpp:%d not reported - %s"):format(want[1], want[2]))
end

local goodOutput, goodStatus = run("good.cpp")
check(goodStatus == 0,
      ("good.cpp exited %s, not 0 - a clean file is being failed")
      :format(tostring(goodStatus)))
check(goodOutput:match("No heap objects stranded"),
      "good.cpp reported a finding, so the scanner has false positives:\n" .. goodOutput)

if #failures > 0 then
  print("Scanner self-test FAILED:")
  for _, f in ipairs(failures) do print("  - " .. f) end
  print("\nbad.cpp output was:\n" .. badOutput)
  os.exit(1)
end

print(("Scanner self-test passed: %d bad shapes caught, good fixture clean.")
      :format(#mustCatch))
