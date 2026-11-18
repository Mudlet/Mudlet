#!/usr/bin/env lua
--[[
Self-test for check-event-names.lua.

A checker that has quietly stopped reading names passes every tree, which looks
exactly like success, so CI runs this first: each line the bad fixtures mark as
BAD has to be reported, nothing else in them may be, and the good fixtures have
to pass with their names actually read.
]]

local here = arg[0]:match("^(.*)/[^/]*$") or "."
local checker = here .. "/check-event-names.lua"
local fixtures = here .. "/event-names-fixtures"

-- arg[-1] is the interpreter this test runs under ("lua" from gh-actions-lua,
-- "lua5.1" from a distribution), so the checker runs under the same one
local interpreter = arg[-1]
if not interpreter or interpreter:match("^%-") then
  interpreter = "lua"
end

local function shellQuote(text)
  return "'" .. text:gsub("'", "'\\''") .. "'"
end

local function run(legacy, editorList, files)
  local parts = {shellQuote(interpreter), shellQuote(checker), "--legacy", shellQuote(fixtures .. "/" .. legacy),
                 "--editor-list", shellQuote(fixtures .. "/" .. editorList)}
  for _, file in ipairs(files) do
    parts[#parts + 1] = shellQuote(fixtures .. "/" .. file)
  end
  local pipe = io.popen(table.concat(parts, " ") .. " 2>&1; echo EXIT:$?")
  local out = pipe:read("*a")
  pipe:close()
  return out, tonumber(out:match("EXIT:(%d+)%s*$"))
end

local failures = {}
local function check(condition, message)
  if not condition then
    failures[#failures + 1] = message
  end
end

-- The marked lines are read from the fixtures rather than listed here, so an
-- edit to a fixture cannot stop a case being checked
local MARKERS = {["bad.cpp"] = "//%s*BAD%s*$", ["bad.lua"] = "%-%-%s*BAD%s*$", ["legacy.txt"] = "#%s*BAD%s*$",
                 ["editor-list-bad.cpp"] = "//%s*BAD%s*$"}
local marked = {}
for file, marker in pairs(MARKERS) do
  local n = 0
  for line in io.lines(fixtures .. "/" .. file) do
    n = n + 1
    if line:match(marker) then
      marked[#marked + 1] = {file = file, line = n}
    end
  end
end
check(#marked >= 17, ("the bad fixtures mark %d lines as BAD, fewer than the 17 cases they hold"):format(#marked))

local badOutput, badStatus = run("legacy.txt", "editor-list-bad.cpp", {"bad.cpp", "bad.lua"})
check(badStatus == 1, ("the bad fixtures exited %s, not 1 - a broken name no longer fails CI"):format(tostring(badStatus)))
for _, mark in ipairs(marked) do
  check(badOutput:find(("%s:%d:"):format(mark.file, mark.line), 1, true), ("%s:%d is marked BAD but was not reported"):format(mark.file, mark.line))
end
for file, number in badOutput:gmatch("([%w%-]+%.%w+):(%d+):") do
  local wanted = false
  for _, mark in ipairs(marked) do
    if mark.file == file and mark.line == tonumber(number) then
      wanted = true
    end
  end
  check(wanted, ("%s:%s was reported but is not marked BAD"):format(file, number))
end

local goodOutput, goodStatus = run("legacy-good.txt", "editor-list-good.cpp", {"good.cpp", "good.lua"})
check(goodStatus == 0, ("the good fixtures exited %s, not 0 - a name that follows the standard is failed:\n%s"):format(tostring(goodStatus), goodOutput))
-- Passing because nothing was read looks the same as passing, so hold the run
-- to the names the good fixtures raise
local names = tonumber(goodOutput:match("All (%d+) event names")) or 0
check(names >= 13, ("the good fixtures gave %d names, fewer than the 13 they raise - the checker reads less than it should"):format(names))

if #failures > 0 then
  for _, failure in ipairs(failures) do
    print("FAIL: " .. failure)
  end
  os.exit(1)
end
print(("check-event-names.lua self-test passed (%d marked cases, %d good names)."):format(#marked, names))
