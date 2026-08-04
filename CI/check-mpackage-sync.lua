#!/usr/bin/env lua
--[[
Check packaged .mpackage archives against their checked-in sources.

Mudlet installs the .mpackage archive, not the loose config.lua/.xml files
sitting next to it, so editing a source file without rebuilding the archive
silently changes nothing at all.

Some of these packages are also published to the package repository
(Mudlet/mudlet-package-repository), which offers updates by comparing the
version in config.lua. A content change that keeps the old version number
never reaches players who installed the package with mpkg.

Run with no arguments to check archive contents. Pass --base-ref to also
require a version bump for any package whose contents changed:

  lua CI/check-mpackage-sync.lua --base-ref origin/development
]]

-- packages built from the loose sources next to them - the two must agree
local sourcedPackages = {
  "src/mudlet-lua/lua/base-ui/mudlet-base-ui.mpackage",
  "src/mudlet-lua/lua/generic-mapper/generic_mapper.mpackage",
  "src/mudlet-lua/lua/gui-drop/gui-drop.mpackage",
}

-- packages the repository syncs weekly, where mpkg needs a version bump to
-- offer the update - see update-core-packages.yml in the package repository
local publishedPackages = {
  "src/deleteOldProfiles.mpackage",
  "src/echo.mpackage",
  "src/enable-accessibility.mpackage",
  "src/mudlet-lua/lua/base-ui/mudlet-base-ui.mpackage",
  "src/mudlet-lua/lua/generic-mapper/generic_mapper.mpackage",
  "src/run-lua-code.mpackage",
}

local errors = {}
local warnings = {}

local function contains(list, wanted)
  for _, item in ipairs(list) do
    if item == wanted then return true end
  end
  return false
end

local function quote(argument)
  return "'" .. argument:gsub("'", "'\\''") .. "'"
end

local function capture(command)
  local pipe = assert(io.popen(command, "r"))
  local output = pipe:read("*a")
  pipe:close()
  return output
end

local function readFile(path)
  local file = io.open(path, "rb")
  if not file then return nil end
  local contents = file:read("*a")
  file:close()
  return contents
end

-- Every entry in the archive, mapped to its bytes. Directory entries, which
-- zipinfo lists with a trailing slash, are not files and are skipped.
local function contentsOf(archive)
  local members = {}
  for name in capture("unzip -Z1 " .. quote(archive)):gmatch("[^\n]+") do
    if not name:match("/$") then
      members[name] = capture(string.format("unzip -p %s %s", quote(archive), quote(name)))
    end
  end
  return members
end

local function sameContents(one, other)
  for name, bytes in pairs(one) do
    if other[name] ~= bytes then return false end
  end
  for name in pairs(other) do
    if one[name] == nil then return false end
  end
  return true
end

local function versionOf(members)
  for line in (members["config.lua"] or ""):gmatch("[^\n]+") do
    local version = line:match("^version%s*=%s*(.-)%s*$")
    if version then
      return (version:gsub("^[%[\"']+", ""):gsub("[%]\"']+$", ""))
    end
  end
  return nil
end

-- Sortable form of a version, tolerating parts like "2" or "1.0.0rc1"
local function versionParts(version)
  local parts = {}
  for part in version:gmatch("[^.]+") do
    parts[#parts + 1] = {tonumber(part:match("%d+")) or 0, part}
  end
  return parts
end

local function isNewer(candidate, existing)
  local new, old = versionParts(candidate), versionParts(existing)
  for index = 1, math.max(#new, #old) do
    local newPart = new[index] or {0, ""}
    local oldPart = old[index] or {0, ""}
    if newPart[1] ~= oldPart[1] then return newPart[1] > oldPart[1] end
    if newPart[2] ~= oldPart[2] then return newPart[2] > oldPart[2] end
  end
  return false
end

-- Archive contents at baseRef, or nil if the package is new there
local function contentsAtBaseRef(path, baseRef)
  local temporary = os.tmpname()
  local archive = capture(string.format("git show %s 2>/dev/null", quote(baseRef .. ":" .. path)))
  if archive == "" then
    os.remove(temporary)
    return nil
  end

  local file = assert(io.open(temporary, "wb"))
  file:write(archive)
  file:close()
  local members = contentsOf(temporary)
  os.remove(temporary)
  return members
end

-- Every member with a file of the same name beside the archive must match it
local function checkSourcesMatch(path, members, enforced)
  local directory = path:match("^(.*)/[^/]+$")
  for name, packaged in pairs(members) do
    local source = directory .. "/" .. name
    local onDisk = readFile(source)
    if onDisk and onDisk ~= packaged then
      local complaint = string.format("%s does not match %s - rebuild the archive after editing the source", path, source)
      table.insert(enforced and errors or warnings, complaint)
    end
  end
end

local function checkVersionBumped(path, members, baseRef)
  local was = contentsAtBaseRef(path, baseRef)
  if not was or sameContents(was, members) then return end

  local old, new = versionOf(was), versionOf(members)
  if not new then
    table.insert(errors, string.format("%s has no version in its config.lua", path))
  elseif old and not isNewer(new, old) then
    table.insert(errors, string.format("%s changed but is still version %s - bump it so mpkg offers the update", path, new))
  end
end

local baseRef
for index = 1, #arg do
  if arg[index] == "--base-ref" then
    baseRef = arg[index + 1]
  elseif arg[index]:match("^%-%-base%-ref=") then
    baseRef = arg[index]:match("=(.*)$")
  end
end

local checked = {}
for _, path in ipairs(sourcedPackages) do
  table.insert(checked, path)
end
for _, path in ipairs(publishedPackages) do
  if not contains(checked, path) then table.insert(checked, path) end
end
table.sort(checked)

for _, path in ipairs(checked) do
  if not readFile(path) then
    table.insert(errors, string.format("%s is listed in this script but does not exist", path))
  else
    local members = contentsOf(path)
    checkSourcesMatch(path, members, contains(sourcedPackages, path))
    if baseRef and contains(publishedPackages, path) then
      checkVersionBumped(path, members, baseRef)
    end
  end
end

for _, warning in ipairs(warnings) do
  print("warning: " .. warning)
end
for _, message in ipairs(errors) do
  print("error: " .. message)
end

if #errors > 0 then
  print(string.format("\n%d problem(s) found. Rebuild an archive with:", #errors))
  print("  cd <package directory> && zip <name>.mpackage config.lua <name>.xml")
  os.exit(1)
end

print("mpackage archives match their sources.")
