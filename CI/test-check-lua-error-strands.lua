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

-- arg[-1] is the interpreter the standalone lua binary was invoked as, so the
-- scanner runs under the same one whether that is "lua" (CI, via
-- gh-actions-lua) or "lua5.1" (Debian and Ubuntu packages).
local interpreter = arg[-1] or "lua"

local function run(file)
  local pipe = io.popen(("%q %q %q 2>&1"):format(interpreter, scanner, fixtures .. "/" .. file))
  local out = pipe:read("*a")
  pipe:close()
  return out
end

local failures = {}
local function check(condition, message)
  if not condition then failures[#failures + 1] = message end
end

-- every shape the scanner must catch, and the line it sits on in bad.cpp
local mustCatch = {
  {5,  "a local QString live across the next argument's getVerifiedString"},
  {12, "a QByteArray temporary inside the raiser's own argument list"},
  {20, "a QByteArray live across a raw lua_error"},
}

local badOutput = run("bad.cpp")
for _, want in ipairs(mustCatch) do
  check(badOutput:match("bad%.cpp:" .. want[1] .. ":"),
        ("bad.cpp:%d not reported - %s"):format(want[1], want[2]))
end

local goodOutput = run("good.cpp")
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
