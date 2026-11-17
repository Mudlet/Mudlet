#!/usr/bin/env lua
--[[
Checks the names of the events Mudlet raises against the standard in
docs/CONTRIBUTING.md ("Event names").

A name fails when it:
  1. comes from C++ and does not start with "sys";
  2. ends in "Event";
  3. does not end in a verb form: its last word (for a phrasal verb such as
     HandedOver, the word before the particle) neither ends in -ed or -ing nor
     is an irregular past participle from IRREGULAR_VERBS;
  4. uses "Toggled" or "Triggered".
Whether the words fit - Pressed or Clicked, one Changed event or a pair - is
left to review. Protocol events (gmcp.*, msdp.*, channel102Message ...) follow
their protocol's names and are not checked.

Names read:
  - C++: every camelCase literal and named constant in an mArgumentList
    append, the name argument of the helpers in NAME_HELPERS, and the old
    names in Host's oldEventNames() table;
  - Lua: raiseEvent("sys...") in src/mudlet-lua/lua and the bundled packages.
A name a script chose (raiseEvent() from Lua, menu and link events) is not
Mudlet's to name and is skipped.

Names that already break a rule are kept on CI/event-names-legacy.txt, one per
line, so a name added there shows up in review. A listed name that nothing
raises any more, or that follows the standard, fails too, so the list stays
the exact backlog of names still waiting for a standard one.

The script editor offers the names in eventNames::systemEvents() (in
src/EventNames.cpp), kept by hand. A name Mudlet raises that the list lacks
fails, and so does a name on it that nothing raises.

Usage: lua CI/check-event-names.lua
         scans the tree against CI/event-names-legacy.txt
       lua CI/check-event-names.lua --legacy FILE [--editor-list FILE] file ...
         scans only the files given (the self-test's fixtures)
       lua CI/check-event-names.lua --list
         prints every name read, where it was found and its verdict
Exits 1 when a name breaks a rule or the legacy list is out of step, 2 when it
cannot run.
]]

local LEGACY_FILE = "CI/event-names-legacy.txt"
-- The names the script editor offers when an event handler is added
local EDITOR_LIST_FILE = "src/EventNames.cpp"

-- Helpers that build the event from a name argument; the first string literal
-- in the call is that name. raiseSettingChangedEvent() is not one: its literal
-- is a setting key.
local NAME_HELPERS = {"raiseProtocolEvent", "raiseSpeechEvent", "raiseSpeechEventOn"}

-- Synced from other projects (.github/workflows/update-3rdparty.yml); a change
-- has to go upstream, so their names are not Mudlet's to fix here
local SKIPPED_PATHS = {"^src/packages/mpkg/", "^src/mudlet%-mapper%.xml$"}

local PROTOCOL_PREFIXES = {"gmcp", "msdp", "mssp", "mxp", "atcp"}
local PROTOCOL_NAMES = {channel102Message = true}

-- Past participles that end in neither -ed nor -ing. Add one here when an event
-- needs it; review sees the addition like any other
local IRREGULAR_VERBS = {}
for word in ([[Begun Bound Broken Built Chosen Done Drawn Found Given Gone Held Hidden Kept Known Lost Made Put
               Quit Read Reset Seen Sent Set Shown Shrunk Shut Split Swept Taken Thrown Undone Unset Won Woken
               Withdrawn Written]]):gmatch("%a+") do
  IRREGULAR_VERBS[word] = true
end

-- The verb of a phrasal one (HandedOver, LoggedIn) comes before its particle
local PARTICLES = {Over = true, Out = true, In = true, Up = true, Down = true, Off = true, On = true, Back = true, Away = true}

local function fromProtocol(name)
  if PROTOCOL_NAMES[name] then
    return true
  end
  for _, prefix in ipairs(PROTOCOL_PREFIXES) do
    if name == prefix or name:sub(1, #prefix + 1) == prefix .. "." then
      return true
    end
  end
  return false
end

local function brokenRules(name, fromCpp)
  local broken = {}
  if fromCpp and not name:match("^sys") then
    broken[#broken + 1] = "does not start with sys"
  end
  local words = {}
  for word in name:gmatch("[A-Z][a-z0-9]*") do
    words[#words + 1] = word
  end
  local lastWord = words[#words] or name
  local verb = lastWord
  if PARTICLES[lastWord] and #words > 1 then
    verb = words[#words - 1]
  end
  if lastWord == "Event" then
    broken[#broken + 1] = "ends in Event"
  elseif not (verb:match("ed$") or verb:match("ing$") or IRREGULAR_VERBS[verb]) then
    local ending = verb == lastWord and lastWord or verb .. lastWord
    broken[#broken + 1] = ("ends in %q, not in a verb form"):format(ending)
  end
  if name:match("Toggled") or name:match("Triggered") then
    broken[#broken + 1] = "uses Toggled or Triggered"
  end
  return broken
end

-- Code with comments taken out, so a name in a comment is not read as raised.
-- inBlock carries a /* */ comment over from the previous line.
local function codeOnly(line, inBlock)
  local out, i, quote = {}, 1, nil
  while i <= #line do
    local c, pair = line:sub(i, i), line:sub(i, i + 1)
    if inBlock then
      if pair == "*/" then
        inBlock, i = false, i + 2
      else
        i = i + 1
      end
    elseif quote then
      out[#out + 1] = c
      if c == "\\" then
        out[#out + 1] = line:sub(i + 1, i + 1)
        i = i + 2
      else
        if c == quote then
          quote = nil
        end
        i = i + 1
      end
    elseif pair == "//" then
      break
    elseif pair == "/*" then
      inBlock, i = true, i + 2
    else
      if c == '"' or c == "'" then
        quote = c
      end
      out[#out + 1] = c
      i = i + 1
    end
  end
  return table.concat(out), inBlock
end

local function literalsIn(text)
  local found = {}
  for literal in text:gmatch('"([^"\\]*)"') do
    found[#found + 1] = literal
  end
  return found
end

-- An event name is one identifier; a literal holding anything else is an
-- argument or a format string
local function looksLikeName(literal)
  return literal:match("^[%a_][%w_%.]*$") ~= nil
end

-- What an mArgumentList holds besides the name - "viewing", "main", a protocol
-- like "GMCP" - is never camelCase, and every event name is, so this picks the
-- name out of whichever append a branch raises it with
local function looksLikeEventName(literal)
  return literal:match("^[a-z][a-z0-9]*[A-Z][%w]*$") ~= nil
end

-- The statement starting at line n, up to its semicolon, for a call or a
-- ternary that is wrapped onto the next lines
local function statementFrom(lines, n, start)
  local text = lines[n]:sub(start)
  local last = n
  while not text:find(";") and last < n + 6 and lines[last + 1] do
    last = last + 1
    text = text .. " " .. lines[last]
  end
  return text:match("^[^;]*") or text
end

local raised = {}  -- name -> {file, line, fromCpp}
local filesScanned = 0
local oldNameTableFound = false

local function noteRaised(name, file, line, fromCpp)
  if not raised[name] then
    raised[name] = {file = file, line = line, fromCpp = fromCpp}
  end
end

-- identifier -> literal, for names kept in a constant. Only Mudlet's names for
-- dedicated constants (csName, scmName), as a common local such as "name" is
-- defined with an unrelated literal somewhere in the tree.
local constants = {}

local function readCodeLines(path)
  local handle = io.open(path, "r")
  if not handle then
    io.stderr:write(("cannot read %s\n"):format(path))
    os.exit(2)
  end
  local lines, inBlock = {}, false
  for raw in handle:lines() do
    lines[#lines + 1], inBlock = codeOnly(raw, inBlock)
  end
  handle:close()
  return lines
end

local function collectConstants(lines)
  for _, code in ipairs(lines) do
    if code:match("const") then
      local name, literal = code:match('([%a_][%w_]*)%s*[{(=]%s*[%w_:]*%(?%s*"([^"]+)"')
      if name and (name:match("^cs%u") or name:match("^scm%u")) and looksLikeName(literal) then
        constants[name] = literal
      end
    end
  end
end

local function scanCpp(path, lines)
  local inOldNameTable = false
  for n, code in ipairs(lines) do
    if code:match("oldEventNames%(%)%s*$") or code:match("oldEventNames%(%)%s*{") then
      inOldNameTable = true
    end
    if inOldNameTable then
      local standard, old = code:match('{%s*qsl%("([^"]+)"%)%s*,%s*qsl%("([^"]+)"%)%s*}')
      if old then
        oldNameTableFound = true
        noteRaised(old, path, n, true)
      end
      if code:match("^}") then
        inOldNameTable = false
      end
    end

    local s = code:find("mArgumentList%s*%.append%s*%(") or code:find("mArgumentList%s*<<")
    if s then
      local statement = statementFrom(lines, n, s)
      for _, literal in ipairs(literalsIn(statement)) do
        if looksLikeEventName(literal) then
          noteRaised(literal, path, n, true)
        end
      end
      for identifier in statement:gmatch("([%a_][%w_]*)") do
        if constants[identifier] then
          noteRaised(constants[identifier], path, n, true)
        end
      end
    end

    for _, helper in ipairs(NAME_HELPERS) do
      local s = code:find(helper .. "%s*%(")
      if s and not code:match("::" .. helper .. "%s*%(") then
        local literal = literalsIn(statementFrom(lines, n, s))[1]
        if literal and looksLikeName(literal) then
          noteRaised(literal, path, n, true)
        end
      end
    end
  end
end

local function scanLua(path)
  local handle = io.open(path, "r")
  if not handle then
    io.stderr:write(("cannot read %s\n"):format(path))
    os.exit(2)
  end
  local n = 0
  for line in handle:lines() do
    n = n + 1
    local code = line:gsub("&quot;", '"'):gsub("%-%-.*$", "")
    for name in code:gmatch("raiseEvent%s*%(%s*[\"'](sys[%w_]*)[\"']") do
      noteRaised(name, path, n, false)
    end
  end
  handle:close()
end

local function skipped(path)
  for _, pattern in ipairs(SKIPPED_PATHS) do
    if path:match(pattern) then
      return true
    end
  end
  return false
end

local function listFiles(command)
  local files = {}
  local pipe = io.popen(command)
  for path in pipe:lines() do
    path = path:gsub("^%./", "")
    if not skipped(path) then
      files[#files + 1] = path
    end
  end
  pipe:close()
  table.sort(files)
  return files
end

-- arguments
local legacyFile, editorListFile, targets, listOnly = nil, nil, {}, false
do
  local i = 1
  while i <= #arg do
    if arg[i] == "--list" then
      listOnly = true
      i = i + 1
    elseif arg[i] == "--editor-list" then
      editorListFile = arg[i + 1]
      i = i + 2
    elseif arg[i] == "--legacy" then
      legacyFile = arg[i + 1]
      i = i + 2
    else
      targets[#targets + 1] = arg[i]
      i = i + 1
    end
  end
end
local wholeTree = #targets == 0

local cppFiles, luaFiles = {}, {}
if wholeTree then
  legacyFile = legacyFile or LEGACY_FILE
  editorListFile = editorListFile or EDITOR_LIST_FILE
  cppFiles = listFiles("find src \\( -name '*.cpp' -o -name '*.h' \\) 2>/dev/null")
  luaFiles = listFiles("find src/mudlet-lua/lua src/packages \\( -name '*.lua' -o -name '*.xml' \\) 2>/dev/null")
else
  for _, path in ipairs(targets) do
    if path:match("%.cpp$") or path:match("%.h$") then
      cppFiles[#cppFiles + 1] = path
    else
      luaFiles[#luaFiles + 1] = path
    end
  end
end

-- Nothing to scan reads exactly like a clean tree, so say so instead
if #cppFiles + #luaFiles == 0 then
  io.stderr:write("no sources to scan - run this from the repository root\n")
  os.exit(2)
end

local cppLines = {}
for _, path in ipairs(cppFiles) do
  cppLines[path] = readCodeLines(path)
  collectConstants(cppLines[path])
end
for _, path in ipairs(cppFiles) do
  scanCpp(path, cppLines[path])
  filesScanned = filesScanned + 1
end
for _, path in ipairs(luaFiles) do
  scanLua(path)
  filesScanned = filesScanned + 1
end

-- A table renamed or reshaped would drop every old name from the scan, and the
-- legacy list would then report them all as raised by nothing
if wholeTree and not oldNameTableFound then
  io.stderr:write("Host's oldEventNames() table was not found, so the old event names cannot be read\n")
  os.exit(2)
end

if listOnly then
  local sorted = {}
  for name in pairs(raised) do
    sorted[#sorted + 1] = name
  end
  table.sort(sorted, function(a, b)
    return a:lower() < b:lower()
  end)
  for _, name in ipairs(sorted) do
    local where = raised[name]
    local verdict = fromProtocol(name) and "protocol" or table.concat(brokenRules(name, where.fromCpp), ", ")
    print(("%s\t%s:%d\t%s"):format(name, where.file, where.line, verdict == "" and "ok" or verdict))
  end
  os.exit(0)
end

local legacy, legacyLines = {}, {}
if legacyFile then
  local handle = io.open(legacyFile, "r")
  if not handle then
    io.stderr:write(("cannot read the legacy list %s\n"):format(legacyFile))
    os.exit(2)
  end
  local n = 0
  for line in handle:lines() do
    n = n + 1
    local name = line:gsub("#.*$", ""):match("^%s*(%S+)")
    if name then
      legacy[name] = true
      legacyLines[name] = n
    end
  end
  handle:close()
end

-- The literals of systemEvents()'s list, from its declaration to the closing brace
local editorList, editorListLines = {}, {}
if editorListFile then
  local handle = io.open(editorListFile, "r")
  if not handle then
    io.stderr:write(("cannot read the script editor's list %s\n"):format(editorListFile))
    os.exit(2)
  end
  local n, inList, found = 0, false, false
  for line in handle:lines() do
    n = n + 1
    local code = codeOnly(line, false)
    if code:match("QStringList%s+systemEvents%s*%(%s*%)") then
      inList, found = true, true
    end
    if inList then
      for name in code:gmatch('qsl%("([^"]+)"%)') do
        editorList[name] = true
        editorListLines[name] = n
      end
      if code:match("};") then
        inList = false
      end
    end
  end
  handle:close()
  -- A renamed function would leave the list empty, and every name would then be reported missing
  if not found then
    io.stderr:write(("systemEvents() was not found in %s, so the script editor's list cannot be read\n"):format(editorListFile))
    os.exit(2)
  end
end

local findings = {}
local names, legacyUsed = 0, 0
for name, where in pairs(raised) do
  if not fromProtocol(name) then
    names = names + 1
    local broken = brokenRules(name, where.fromCpp)
    if #broken > 0 and not legacy[name] then
      findings[#findings + 1] = {file = where.file, line = where.line, detail = ("%s %s"):format(name, table.concat(broken, ", "))}
    elseif #broken > 0 then
      legacyUsed = legacyUsed + 1
    elseif legacy[name] then
      findings[#findings + 1] = {file = legacyFile, line = legacyLines[name], detail = ("%s follows the standard, so it does not belong on the legacy list"):format(name)}
    end
  end
end
for name in pairs(legacy) do
  if not raised[name] then
    findings[#findings + 1] = {file = legacyFile, line = legacyLines[name], detail = ("%s is on the legacy list, but nothing raises it"):format(name)}
  end
end

if editorListFile then
  for name, where in pairs(raised) do
    if not editorList[name] and not fromProtocol(name) then
      findings[#findings + 1] = {file = where.file, line = where.line, detail = ("%s is raised, but the script editor's list (systemEvents() in %s) does not offer it"):format(name, EDITOR_LIST_FILE)}
    end
  end
  for name in pairs(editorList) do
    if not raised[name] then
      findings[#findings + 1] = {file = editorListFile, line = editorListLines[name], detail = ("%s is in the script editor's list, but nothing raises it"):format(name)}
    end
  end
end

table.sort(findings, function(a, b)
  if a.file ~= b.file then
    return a.file < b.file
  end
  return a.line < b.line
end)

for _, finding in ipairs(findings) do
  print(("%s:%d: %s"):format(finding.file, finding.line, finding.detail))
end

if #findings > 0 then
  print("")
  print(("%d event name problem(s)."):format(#findings))
  print('Name a new event as docs/CONTRIBUTING.md ("Event names") describes, and add it to')
  print(("systemEvents() in %s. An existing name kept on purpose goes on"):format(EDITOR_LIST_FILE))
  print(("%s, where review sees it in the diff."):format(LEGACY_FILE))
  os.exit(1)
end
print(("All %d event names follow the standard or are on the legacy list%s (%d legacy, %d files scanned).")
      :format(names, editorListFile and ", and the script editor's list offers each" or "", legacyUsed, filesScanned))
