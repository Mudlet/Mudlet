#!/usr/bin/env lua
-- Turns gcovr's CSV export into the two tables the coverage baseline needs:
-- a per-file listing sorted by uncovered-line MASS (not percentage) and a
-- subsystem rollup. Reads coverage.csv from the directory it is given.
--
-- Usage: lua rollup.lua <coverage.csv> [outdir]

local csvPath = arg[1] or "coverage.csv"
local outDir = arg[2] or "."

-- gcovr's CSV has no embedded commas in our paths, so a plain split is enough.
local function splitCsv(line)
  local fields = {}
  for field in (line .. ","):gmatch("([^,]*),") do
    fields[#fields + 1] = field
  end
  return fields
end

-- First match wins, so order matters: the mapper dialogs have to be claimed by
-- the map bucket before the generic dlg* pattern sees them.
local subsystems = {
  {name = "Map + mapper widgets", patterns = {
    "^TMap", "^TRoom", "^TArea", "^T2DMap", "^dlgMapper", "^dlgMapLabel", "^dlgRoomExits",
    "^dlgRoomProperties", "^mapInfoContributorManager", "^glwidget", "^modern_glwidget",
    "^CameraController", "^ShaderManager", "^RenderCommand", "^LabelTextureCache",
    "^exitstreewidget", "^CustomLine", "^RoomContextMenuHandler", "^RoomMove",
    "^PanInteractionHandler", "^MiddleMousePanHandler", "^SelectionRectangleHandler",
    "^LabelInteractionHandler", "^TAstar",
  }},
  {name = "Telnet + MXP + MMCP + GMCP", patterns = {
    "^ctelnet", "^MMCP", "^TMxp", "^MxpTag", "^GMCPAuthenticator", "^TEntity",
    "^TTextCodec", "^TEncoding",
  }},
  {name = "Text pipeline (TBuffer/TConsole/TTextEdit)", patterns = {
    "^TBuffer", "^TConsole", "^TMainConsole", "^TTextEdit", "^TCommandLine", "^TLinkStore",
    "^TDebug", "^TTextBox", "^TStringUtils", "^THyperlink", "^TAccessible", "^TScrollBox",
    "^TSplitter", "^UntrustedText", "^widechar_width", "^TTextProperties",
  }},
  {name = "Lua API (TLuaInterpreter*)", patterns = {
    "^TLuaInterpreter", "^LuaInterface", "^LuaLiteral",
  }},
  {name = "Scripting engine (triggers/aliases/timers/keys/scripts/vars)", patterns = {
    "^TTrigger", "^TAlias", "^TTimer", "^TKey%.", "^TKeySequenceEdit", "^TScript", "^TAction",
    "^TVar", "Unit%.", "^TMatchState", "^Tree%.h", "^TEvent%.h", "^EAction",
  }},
  {name = "Editor + edbee glue", patterns = {
    "^dlgSourceEditor", "^dlgTriggerEditor", "^dlgTriggerPatternEdit", "^TriggerHighlighter",
    "^TrailingWhitespaceMarker", "^Editor", "^dlgColorTrigger", "^dlgComposer", "^dlgNotepad",
    "^dlg.*MainArea", "^dlgSystemMessageArea", "^SingleLineTextEdit",
  }},
  {name = "Dialogs (dlg*)", patterns = {"^dlg"}},
  {name = "Updater + dblsqd", patterns = {"^updater", "^src/updater/", "^Feed%.", "^Release%.", "^SemVer%.", "^UpdateDialog", "^sparkleupdater"}},
  {name = "Media (TMedia)", patterns = {"^TMedia"}},
  {name = "Discord", patterns = {"^discord"}},
  {name = "IRC client", patterns = {"^dlgIRC", "^ircmessageformatter"}},
  {name = "Persistence (XML import/export)", patterns = {"^XMLexport", "^XMLimport"}},
  {name = "Host + mudlet core", patterns = {
    "^Host", "^mudlet%.", "^main%.", "^MudletInstanceCoordinator", "^EventLoopPump",
    "^FontManager", "^GeometryManager", "^ShortcutsManager", "^ResourceManager",
    "^CredentialManager", "^SecureStringUtils", "^OAuthClientFlow", "^SentryWrapper",
    "^FileOpenHandler", "^DarkTheme", "^AltFocusMenuBarDisable", "^TForkedProcess",
    "^TUiTour", "^TFeatureCallout", "^TTabBar", "^TToolBar", "^TDockWidget",
    "^TDetachedWindow", "^TEasyButtonBar", "^TFlipButton", "^TLabel", "^TTreeWidget",
    "^WideComboBox", "^PackageItemDelegate", "^GifTracker", "^LsanHooks", "^emptyFile",
    "^enums%.h", "^utils%.h", "^TGameDetails", "^TMapView",
  }},
}

local function bucketFor(path)
  -- Match on the path relative to src/ so nested directories keep their prefix.
  local rel = path:gsub("^src/", "")
  for _, subsystem in ipairs(subsystems) do
    for _, pattern in ipairs(subsystem.patterns) do
      if rel:match(pattern) or path:match(pattern) then
        return subsystem.name
      end
    end
  end
  return "Other"
end

local rows = {}
local header
for line in io.lines(csvPath) do
  if not header then
    header = splitCsv(line)
    -- Columns are read by position below; a gcovr that reorders them would
    -- otherwise produce a silently zeroed report (tonumber(...) or 0).
    local expected = { "filename", "line_total", "line_covered", nil, "branch_total", "branch_covered" }
    for i, name in pairs(expected) do
      if header[i] ~= name then
        error(("unexpected gcovr CSV header: column %d is %q, expected %q"):format(i, tostring(header[i]), name))
      end
    end
  else
    local f = splitCsv(line)
    local path = f[1]
    if path and path ~= "" then
      local lineTotal = tonumber(f[2]) or 0
      local lineCovered = tonumber(f[3]) or 0
      local branchTotal = tonumber(f[5]) or 0
      local branchCovered = tonumber(f[6]) or 0
      rows[#rows + 1] = {
        path = path,
        lineTotal = lineTotal,
        lineCovered = lineCovered,
        uncovered = lineTotal - lineCovered,
        branchTotal = branchTotal,
        branchCovered = branchCovered,
        bucket = bucketFor(path),
      }
    end
  end
end

local function pct(covered, total)
  if total == 0 then
    return "n/a"
  end
  return string.format("%.1f%%", 100 * covered / total)
end

-- Overall
local all = {lineTotal = 0, lineCovered = 0, branchTotal = 0, branchCovered = 0}
for _, r in ipairs(rows) do
  all.lineTotal = all.lineTotal + r.lineTotal
  all.lineCovered = all.lineCovered + r.lineCovered
  all.branchTotal = all.branchTotal + r.branchTotal
  all.branchCovered = all.branchCovered + r.branchCovered
end

local out = io.open(outDir .. "/rollup.md", "w")
out:write("# Mudlet C++ coverage baseline\n\n")
out:write(string.format("Files measured: %d\n\n", #rows))
out:write(string.format("Overall src/ lines: %d covered / %d instrumented = %s\n",
  all.lineCovered, all.lineTotal, pct(all.lineCovered, all.lineTotal)))
out:write(string.format("Overall src/ branches: %d covered / %d = %s\n\n",
  all.branchCovered, all.branchTotal, pct(all.branchCovered, all.branchTotal)))

-- Subsystem rollup
local buckets = {}
local order = {}
for _, r in ipairs(rows) do
  local b = buckets[r.bucket]
  if not b then
    b = {name = r.bucket, lineTotal = 0, lineCovered = 0, branchTotal = 0, branchCovered = 0, files = 0}
    buckets[r.bucket] = b
    order[#order + 1] = b
  end
  b.lineTotal = b.lineTotal + r.lineTotal
  b.lineCovered = b.lineCovered + r.lineCovered
  b.branchTotal = b.branchTotal + r.branchTotal
  b.branchCovered = b.branchCovered + r.branchCovered
  b.files = b.files + 1
end
table.sort(order, function(a, b) return (a.lineTotal - a.lineCovered) > (b.lineTotal - b.lineCovered) end)

out:write("## Subsystem rollup (sorted by uncovered lines)\n\n")
out:write("| Subsystem | Files | Lines | Covered | Uncovered | Line % | Branch % |\n")
out:write("| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n")
for _, b in ipairs(order) do
  out:write(string.format("| %s | %d | %d | %d | %d | %s | %s |\n",
    b.name, b.files, b.lineTotal, b.lineCovered, b.lineTotal - b.lineCovered,
    pct(b.lineCovered, b.lineTotal), pct(b.branchCovered, b.branchTotal)))
end

-- Per-file, by uncovered mass
table.sort(rows, function(a, b)
  if a.uncovered ~= b.uncovered then return a.uncovered > b.uncovered end
  return a.path < b.path
end)

out:write("\n## Per-file, sorted by uncovered-line mass\n\n")
out:write("| # | File | Lines | Covered | Uncovered | Line % | Subsystem |\n")
out:write("| ---: | --- | ---: | ---: | ---: | ---: | --- |\n")
for i, r in ipairs(rows) do
  out:write(string.format("| %d | %s | %d | %d | %d | %s | %s |\n",
    i, r.path, r.lineTotal, r.lineCovered, r.uncovered, pct(r.lineCovered, r.lineTotal), r.bucket))
end
out:close()

print(string.format("overall: %d/%d lines = %s ; %d/%d branches = %s ; %d files",
  all.lineCovered, all.lineTotal, pct(all.lineCovered, all.lineTotal),
  all.branchCovered, all.branchTotal, pct(all.branchCovered, all.branchTotal), #rows))
print("wrote " .. outDir .. "/rollup.md")
