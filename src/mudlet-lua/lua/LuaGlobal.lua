----------------------------------------------------------------------------------
--- Mudlet Lua packages loader
----------------------------------------------------------------------------------

-- set to true (or provide a global with the same name and also set to true) to
-- report on the determination of what path to use to load the other Mudlet
-- and Geyser provided Lua files...
local debugLoading = debugLoading or false

if package.loaded["rex_pcre"] then
  rex = require "rex_pcre"
end
if package.loaded["lpeg"] then
  lpeg = require "lpeg"
end
if package.loaded["zip"] then
  zip = require "zip"
end
if package.loaded["lfs"] then
  lfs = require "lfs"
end

-- TODO this is required by DB.lua, so we might load it all at one place
--if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

json_to_value = yajl.to_value
gmcp = {}
mssp = {}

function __gmcp_merge_gmcp_sub_tables( a, key )
  local _m = a.__needMerge;
  for k, v in pairs(_m) do
    a[key][k] = v;
  end
  a.__needMerge = nil
end


function unzip( what, dest )
  -- cecho("\n<blue>unpacking package:<"..what.."< to <"..dest..">\n")
  local z, err = zip.open( what )

  if not z then
    cecho("\nerror unpacking: " .. err)
    return
  end

  local createdDirs = {}
  for file in z:files() do
    local _f, err = z:open( file.filename )
    local _data = _f:read("*a")
    local _path = dest .. file.filename
    local _dir = string.split( file.filename, '/' )
    local created = dest;
    for k, v in ipairs( _dir ) do
      if k < #_dir then
        created = created .. '/' .. v;
        if not table.contains( createdDirs, created ) then
          table.insert( createdDirs, created );
          lfs.mkdir( created );
          -- cecho("<red>--> creating dir:" .. created .. "\n");
        end
      elseif file.uncompressed_size == 0 then
        if not table.contains( createdDirs, created ) then
          -- cecho("<red>--> creating dir:" .. file.filename .. "\n")
          table.insert( createdDirs, created );
          lfs.mkdir( file.filename )
        end
      end
    end
    local _path = dest .. file.filename
    if file.uncompressed_size > 0 then
      local out = io.open( _path, "wb" )
      if out then
        -- cecho("<green>unpacking file:".._path.."\n")
        out:write( _data )
        out:close()
      else
        cecho("<red>ERROR: can't write file:" .. _path .. "\n")
      end
    end
    _f:close();
  end
  z:close()
end



function onConnect()
end

function handleWindowResizeEvent()
end

local packages = {
  "StringUtils.lua",
  "TableUtils.lua",
  -- "Logging.lua", -- never documented and fails to load now
  "DebugTools.lua",
  "DB.lua",
  "geyser/Geyser.lua",
  "geyser/GeyserGeyser.lua",
  "geyser/GeyserUtil.lua",
  "geyser/GeyserColor.lua",
  "geyser/GeyserSetConstraints.lua",
  "geyser/GeyserContainer.lua",
  "geyser/GeyserWindow.lua",
  "geyser/GeyserLabel.lua",
  "geyser/GeyserGauge.lua",
  "geyser/GeyserMiniConsole.lua",
  "geyser/GeyserMapper.lua",
  "geyser/GeyserReposition.lua",
  "geyser/GeyserHBox.lua",
  "geyser/GeyserVBox.lua",
  -- TODO probably don't need to load this file
  "geyser/GeyserTests.lua",
  "GUIUtils.lua",
  "Other.lua",
  "GMCP.lua",
  "KeyCodes.lua",
  "TTSValues.lua"
}

-- on windows LuaGlobal gets loaded with the current directory set to mudlet.exe's location
-- on Mac, it's set to LuaGlobals location - or the Applications folder, or something else...
-- So work out where to load Lua files from using some heuristics
-- Addition of "../src/" to front of path allows things to work when "Shadow Building"
-- option of Qt Creator is used (separates object code from source code in directories
-- beside the latter, allowing parallel builds against different Qt Library versions
-- or {Release|Debug} types).  A further "../../src/" does the same when the
-- qmake CONFIG has both "debug_and_release" (default on Windows) and
-- "debug_and_release_target" (default on all platforms) options which puts the
-- executable in a further sub-directory down when shadow building!
-- TODO: extend to support common Lua code being placed in system shared directory
-- tree as ought to happen for *nix install builds.
local prefixes = { "./../../src/mudlet-lua/lua/",
                   "./../src/mudlet-lua/lua/",
                   "./../Resources/mudlet-lua/lua/",
                   "./mudlet-lua/lua/",
                   "./lua/",
                   "mudlet.app/Contents/Resources/mudlet-lua/lua/" }

-- add default search paths coming from the C++ side as well
if getMudletLuaDefaultPaths then
  for _, path in ipairs(getMudletLuaDefaultPaths()) do
    prefixes[#prefixes + 1] = path
  end
end

-- this is based on directory separators only ever being '/' or '\\' which does
-- seem to cover the cases that we are likely to encounter...!
for i, path in ipairs(prefixes) do
  prefixes[i] = string.gsub(path, '[/\\]', package.config:sub(1,1))
  if debugLoading then
    echo([[Directory separator conversion: "]] .. path .. [[" becomes: "]] .. prefixes[i] .. [["
]])
  end
end

if debugLoading then
  echo([[Current directory is: "]] .. lfs.currentdir() .. [[".
]])
end

local prefix
for i = 1, #prefixes do
  -- lfs.attributes returns a table if the given file-system object exists
  if debugLoading then
    echo([[Testing: "]] .. prefixes[i] .. [[LuaGlobal.lua"...]])
  end
  if lfs.attributes(prefixes[i] .. "LuaGlobal.lua") then
    if debugLoading then
      echo(" found something!\n")
    end
    prefix = prefixes[i]
    break
  else
    if debugLoading then
      echo(" not found.\n")
    end
  end
end

if not prefix then
  echo([[Error locating Lua files from LuaGlobal.lua - we are looking from ']] .. lfs.currentdir() .. [['.
]])
  return
end

if debugLoading then
  echo([[Locating other Lua files from LuaGlobal.lua - we are looking from ']] .. lfs.currentdir() .. [['.
]])
end

for _, package in ipairs(packages) do
  local result, msg = pcall(dofile, prefix .. package)
  if not result then
    echo("Error attempting to load file: " .. package .. ": " .. msg .. "\n")
  end
end
