#!/usr/bin/env lua
--[[
Self-test for update-autocompletion.lua.

The scraper reads the wiki manual's HTML, so a change in what MediaWiki wraps a
heading in lands straight in the names the script editor offers - which is how all
669 of them came to have "[edit | edit source]" appended, "addAreaName" reading as
"addAreaName[edit | edit source]" (#10816). Nothing about that fails: the update
runs green and the list is simply unusable.

The fixtures below are a frozen copy of that markup, taken from
https://wiki.mudlet.org/w/Manual:Lua_Functions on 19 September 2026, when the wiki
reported itself as MediaWiki 1.35.0. They test this script against a recorded page:
a change on the wiki itself cannot be seen from here, and is what the count guards
in the generator are for.
]]

local here = arg[0]:match("^(.*)[/\\][^/\\]*$") or "."
local generator = here .. "/update-autocompletion.lua"
-- os.tmpname() hands back an unwritable drive-relative path on Windows
local outputPath = (os.getenv("TMPDIR") or os.getenv("TEMP") or ".") .. "/mudlet-autocompletion-selftest.json"

-- the markup MediaWiki puts after the headline inside the same <h2>
local editSection = '<span class="mw-editsection"><span class="mw-editsection-bracket">[</span>'
    .. '<a href="/index.php?title=Manual:Test&amp;veaction=edit&amp;section=T-1" class="mw-editsection-visualeditor" title="Edit section: ">edit</a>'
    .. '<span class="mw-editsection-divider"> | </span>'
    .. '<a href="/index.php?title=Manual:Test&amp;action=edit&amp;section=T-1" title="Edit section: ">edit source</a>'
    .. '<span class="mw-editsection-bracket">]</span></span>'

local function heading(id, text, chrome)
  return ('<h2><span class="mw-headline" id="%s">%s</span>%s</h2>'):format(id, text, chrome and editSection or "")
end

local encoded = "<<the function table, encoded>>"

-- a self-test that stopped part way reads like a passing one, so the last line reports how much ran
local checks, pages = 0, 0

-- Runs the generator itself, not a copy of its patterns; its two rocks are stubbed so the fixture
-- stands in for the wiki and the output table is captured here.
local function run(fixture)
  pages = pages + 1
  local scraped
  local warnings, output = {}, {}
  local realRequire, realPrint = require, print

  local headers = {get = function(_, field) return field == ":status" and "200" or nil end}
  local stream = {get_body_as_string = function() return fixture end}
  local httpRequest = {new_from_uri = function() return {go = function() return headers, stream end} end}
  local lunajson = {
    encode = function(functions)
      scraped = functions
      return encoded
    end,
    -- enough of a decode for the generator to count the list it is replacing
    decode = function(text)
      local entries = {}
      for key, usage in text:gmatch('"([^"]+)"%s*:%s*"([^"]*)"') do
        entries[key] = usage
      end
      return entries
    end,
  }

  require = function(name)
    if name == "http.request" then
      return httpRequest
    elseif name == "lunajson" then
      return lunajson
    end
    return realRequire(name)
  end
  print = function(message)
    output[#output + 1] = tostring(message)
    if type(message) == "string" and message:match("^::warning::") then
      warnings[#warnings + 1] = message
    end
  end

  arg = {outputPath}
  -- pcall so a generator error fails a check instead of aborting before any ran
  local ok, err = pcall(dofile, generator)
  require, print = realRequire, realPrint

  local written
  local file = io.open(outputPath, "r")
  if file then
    written = file:read("*a")
    file:close()
    os.remove(outputPath)
  end
  return {ok = ok, err = tostring(err), scraped = scraped or {}, warnings = warnings, output = output, written = written}
end

local failures = {}
local function check(condition, message)
  checks = checks + 1
  if not condition then failures[#failures + 1] = message end
end

local function countEntries(list)
  local entries = 0
  for _ in pairs(list) do entries = entries + 1 end
  return entries
end

local function warningsOfKind(result, kind)
  local matched = {}
  for _, warning in ipairs(result.warnings) do
    if warning:match("^::warning::" .. kind .. ": ") then
      matched[#matched + 1] = warning
    end
  end
  return matched
end

os.remove(outputPath)

-- 1. the heading shapes the manual actually uses
local manual = run(table.concat({
  heading("addAreaName", "addAreaName", true),
  '<dl><dt>areaID = addAreaName(areaName)</dt>',
  -- a heading wrapped in a link, which is why the scraper strips tags at all
  heading("cecho", '<a href="/w/Manual:UI_Functions#cecho" title="cecho">cecho</a>', true),
  '<dl><dt>cecho([window], text)</dt>',
  -- a heading with no section-edit link: the strip must not depend on one being there
  heading("send", "send", false),
  '<dl><dt>send(command, showOnScreen)</dt>',
  -- namespaced functions are deliberately left out of the list, and silently
  heading("db:add", "db:add", true),
  '<dl><dt>db:add(sheet, table)</dt>',
  -- documented twice: the first signature wins and the run is warned about it
  heading("addAreaName_2", "addAreaName", true),
  '<dl><dt>areaID = addAreaName(areaName, areaID)</dt>',
  -- a prose signature (no <dl><dt>) must not take the next section's (live sendCmdLine/setConsoleBufferSize case)
  heading("sendCmdLine", "sendCmdLine", true),
  '<p><b>sendCmdLine(command)</b> - puts text on the command line.</p>',
  heading("setConsoleBufferSize", "setConsoleBufferSize", true),
  '<dl><dt>setConsoleBufferSize([consoleName], linesLimit)</dt>',
  -- a signature the wiki has marked up: the hint is shown as text, so tags cannot stay in it
  heading("createLabel", "createLabel", true),
  '<dl><dt><b>labelID</b> = createLabel(<i>name</i>)</dt>',
}, "\n"))

check(manual.ok, "the generator failed on the manual's own heading markup: " .. manual.err)
local expected = {
  addAreaName = "areaID = addAreaName(areaName)",
  cecho = "cecho([window], text)",
  send = "send(command, showOnScreen)",
  setConsoleBufferSize = "setConsoleBufferSize([consoleName], linesLimit)",
  createLabel = "labelID = createLabel(name)",
}
for name, usage in pairs(expected) do
  check(manual.scraped[name] == usage,
        string.format("%s should be listed as %q, got %q", name, usage, tostring(manual.scraped[name])))
end
for name, usage in pairs(manual.scraped) do
  check(not name:match("[%[%]<>]"), string.format("%q carries wiki markup into the name", name))
  check(not usage:match("[<>]"), string.format("the hint for %q carries wiki markup: %q", name, usage))
end
check(countEntries(manual.scraped) == 5,
      string.format("expected 5 functions in the list, got %d", countEntries(manual.scraped)))
check(manual.scraped["db:add"] == nil, "db:add is namespaced and should not be offered")
check(manual.scraped["sendCmdLine"] == nil,
      "sendCmdLine documents no signature, so it must be left out rather than given the next function's")
check(#warningsOfKind(manual, "duplicate heading") == 1
        and manual.warnings[1]:match("addAreaName is documented more than once"),
      "the duplicate heading should have been reported once by name, got: " .. table.concat(manual.warnings, " / "))
check(#warningsOfKind(manual, "unusable name") == 0,
      "nothing in the manual fixture is an unusable name, got: " .. table.concat(manual.warnings, " / "))
check(manual.written == encoded,
      string.format("the generator should have written the encoded list, wrote %q", tostring(manual.written)))

-- 2. chrome the strip does not recognise: the name is dropped, loudly
local chrome = run(table.concat({
  heading("send", "send", true),
  '<dl><dt>send(command, showOnScreen)</dt>',
  '<h2><span class="mw-headline" id="raiseEvent">raiseEvent</span>'
    .. '<span class="mw-editsection-v2"><span>[</span>edit<span>]</span></span></h2>',
  '<dl><dt>raiseEvent(name, ...)</dt>',
  -- leaked chrome with its own colon looks namespaced; only the positive namespace test keeps it warned
  '<h2><span class="mw-headline" id="addAreaName">addAreaName</span>'
    .. '<span class="mw-editsection-v2">[edit | Manual:edit source]</span></h2>',
  '<dl><dt>areaID = addAreaName(areaName)</dt>',
  -- a trailing separator is a mangled heading, so it warns rather than passing as db:add
  heading("db:add:", "db:add:", true),
  '<dl><dt>db:add(sheet, table)</dt>',
}, "\n"))

check(chrome.ok, "the generator failed on the unusable-name fixture: " .. chrome.err)
check(chrome.scraped["raiseEvent"] == nil and chrome.scraped["addAreaName"] == nil,
      "a name the strip left chrome on must not reach the list")
check(countEntries(chrome.scraped) == 1, "only send should have survived the unusable-name fixture")
local unusable = warningsOfKind(chrome, "unusable name")
check(#unusable == 3, string.format("all three unusable names should have been reported, got %d: %s",
                                    #unusable, table.concat(chrome.warnings, " / ")))
local reported = table.concat(unusable, " ")
check(reported:match("raiseEvent") and reported:match("addAreaName") and reported:match("db:add:"),
      "the warnings should name the headings they came from: " .. table.concat(unusable, " / "))

-- 3. the MediaWiki 1.43+ heading shape (a wrapping div, no mw-headline), which nothing matches
local moved = run(table.concat({
  '<div class="mw-heading mw-heading2"><h2 id="addAreaName">addAreaName</h2>' .. editSection .. '</div>',
  '<dl><dt>areaID = addAreaName(areaName)</dt>',
  '<div class="mw-heading mw-heading2"><h2 id="send">send</h2>' .. editSection .. '</div>',
  '<dl><dt>send(command, showOnScreen)</dt>',
}, "\n"))

check(not moved.ok, "a page whose heading markup no longer matches must fail the run, not write an empty list")
check(moved.err:match("heading markup"), "the failure should say the heading markup changed, got: " .. moved.err)
check(moved.written == nil, "nothing should have been written: " .. tostring(moved.written))

-- 4. a scrape that comes back shorter than the list it is replacing
local previous = io.open(outputPath, "w")
previous:write('{\n')
for entry = 1, 40 do
  previous:write(string.format('    "func%d": "func%d()",\n', entry, entry))
end
previous:write('    "send": "send(command, showOnScreen)"\n}\n')
previous:close()
local shorter = run(table.concat({
  heading("send", "send", true),
  '<dl><dt>send(command, showOnScreen)</dt>',
}, "\n"))

check(not shorter.ok, "a list that lost most of its functions must fail the run")
check(shorter.err:match("down from 41"), "the failure should say what it was replacing, got: " .. shorter.err)
check(shorter.written ~= nil and shorter.written:match('"func1"'),
      "the list being replaced must be left alone when the run fails")
os.remove(outputPath)

-- 5. unreadable headings too few for keepRatio to notice: the allowance must catch them on its own
local function pageMissing(broken)
  local page = {}
  for entry = 1, 160 do
    page[#page + 1] = heading("func" .. entry, "func" .. entry, true)
    page[#page + 1] = ("<dl><dt>func%d()</dt>"):format(entry)
  end
  for entry = 1, broken do
    -- chrome the strip does not recognise, so the section-edit text stays in the name
    page[#page + 1] = ('<h2><span class="mw-headline" id="broken%d">broken%d</span>'):format(entry, entry)
      .. '<span class="mw-editsection-v2">[edit | edit source]</span></h2>'
    page[#page + 1] = ("<dl><dt>broken%d()</dt>"):format(entry)
  end
  return table.concat(page, "\n")
end

os.remove(outputPath)
local quirk = run(pageMissing(5))
check(quirk.ok, "a handful of unreadable headings is a wiki quirk and must not fail the run: " .. quirk.err)
local quirkWarnings = warningsOfKind(quirk, "unusable name")
check(#quirkWarnings == 5,
      string.format("the five unreadable headings should each have been reported, got %d", #quirkWarnings))
check(countEntries(quirk.scraped) == 160,
      string.format("expected the 160 readable functions, got %d", countEntries(quirk.scraped)))

local gaps = run(pageMissing(8))
check(not gaps.ok, "more unreadable headings than the allowance must fail the run, not publish the list without them")
check(gaps.err:match("heading markup"), "the failure should say the heading markup changed, got: " .. gaps.err)
check(gaps.written == nil, "nothing should have been written: " .. tostring(gaps.written))
os.remove(outputPath)

if #failures > 0 then
  for _, failure in ipairs(failures) do
    io.stderr:write("FAIL: " .. failure .. "\n")
  end
  os.exit(1)
end

print(string.format("update-autocompletion.lua scrapes function names cleanly: %d checks over %d recorded pages.", checks, pages))
